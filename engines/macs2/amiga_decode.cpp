/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "macs2/amiga_decode.h"

#include "common/compression/powerpacker.h"
#include "common/endian.h"
#include "common/hashmap.h"
#include "common/util.h"

namespace Macs2 {

static const uint16 kPlanes = 6;

bool parseAmigaMxoo(const byte *mxoo, uint32 mxooSize, AmigaMxooInfo &out) {
	out = AmigaMxooInfo();
	if (!mxoo || mxooSize < 24)
		return false;
	if (READ_BE_UINT32(mxoo) != MKTAG('M', 'X', 'O', 'O'))
		return false;

	out.scriptOffset = READ_BE_UINT32(mxoo + 4);
	out.stringOffset = READ_BE_UINT32(mxoo + 8);
	out.bodyOffset = 12;
	if (out.scriptOffset > mxooSize || out.stringOffset > mxooSize)
		return false;
	out.bodySize = out.scriptOffset > out.bodyOffset ? out.scriptOffset - out.bodyOffset : 0;
	if (out.bodySize < 0x66)
		return false;

	const byte *body = mxoo + out.bodyOffset;
	if (READ_BE_UINT16(body + 0x0C) != 0x0101)
		return false;

	for (uint i = 0; i < 21; i++)
		out.slotOffsets[i] = READ_BE_UINT32(body + 0x0E + i * 4);
	out.extraOffset = READ_BE_UINT32(body + 0x62);
	return true;
}

bool inspectAmigaAnimSlot(const byte *mxoo, uint32 mxooSize, uint32 bodyRelativeOffset, AmigaAnimSlotInfo &out) {
	out = AmigaAnimSlotInfo();
	if (!mxoo || bodyRelativeOffset == 0 || bodyRelativeOffset == 0xFFFFFFFF)
		return false;

	const uint32 absOff = 12 + bodyRelativeOffset;
	if (absOff + 16 > mxooSize)
		return false;

	const byte *p = mxoo + absOff;
	out.headerHint = READ_BE_UINT16(p + 0);
	out.seqPos = READ_BE_UINT16(p + 2);
	out.repeatCounter = READ_BE_UINT16(p + 4);
	out.loopStart = READ_BE_UINT16(p + 6);

	// Sequence length follows the same pattern as the DOS blob: headerHint is often
	// the frame count; sequence payload is max(hint - 2, 0) bytes, padded to even.
	const uint16 seqBytes = (out.headerHint >= 2) ? (uint16)(out.headerHint - 2) : 0;
	const uint16 seqPadded = (seqBytes + 1) & ~1;
	const uint32 metaOff = absOff + 8 + seqPadded;
	if (metaOff + 6 > mxooSize)
		return false;

	out.frameCount = READ_BE_UINT16(mxoo + metaOff + 0);
	out.width = READ_BE_UINT16(mxoo + metaOff + 2);
	out.height = READ_BE_UINT16(mxoo + metaOff + 4);
	out.pixelOffset = metaOff + 6;
	out.headerSize = out.pixelOffset - absOff;

	if (out.frameCount == 0 || out.width == 0 || out.height == 0)
		return false;
	if (out.width > 320 || out.height > 200 || out.frameCount > 256)
		return false;

	const uint32 rowBytes = (out.width + 7) / 8;
	const uint32 planeBytes = rowBytes * out.height;
	const uint32 pixelsNeeded = (uint32)out.frameCount * planeBytes * kPlanes;
	if (out.pixelOffset + pixelsNeeded > mxooSize)
		return false;

	out.valid = true;
	return true;
}

bool decodeAmigaPlanarFrame(const byte *planar, uint16 width, uint16 height, uint16 frameIndex,
							uint16 frameCount, Common::Array<byte> &outPixels) {
	if (!planar || width == 0 || height == 0 || frameIndex >= frameCount)
		return false;

	const uint32 rowBytes = (width + 7) / 8;
	const uint32 planeBytes = rowBytes * height;
	const uint32 frameBytes = planeBytes * kPlanes;
	const byte *frameBase = planar + (uint32)frameIndex * frameBytes;

	outPixels.clear();
	outPixels.resize((uint32)width * height);
	Common::fill(outPixels.begin(), outPixels.end(), 0);

	// Anim slots store 6 bitplanes; colors use planes 0..4 (indices 0..31).
	// Plane 5 is not applied as a sprite mask here — doing so clears almost all
	// opaque pixels in the demo's character anims. Color 0 remains transparent
	// for the existing DOS draw path.
	for (uint16 y = 0; y < height; y++) {
		for (uint16 x = 0; x < width; x++) {
			byte color = 0;
			const uint32 bitIndex = (uint32)x & 7;
			const uint32 byteInRow = (uint32)x >> 3;
			for (uint16 plane = 0; plane < 5; plane++) {
				const byte *planeRow = frameBase + plane * planeBytes + (uint32)y * rowBytes;
				if (planeRow[byteInRow] & (0x80 >> bitIndex))
					color |= (byte)(1 << plane);
			}
			outPixels[(uint32)y * width + x] = color;
		}
	}
	return true;
}

bool convertAmigaAnimSlotToDosBlob(const byte *mxoo, uint32 mxooSize, uint32 bodyRelativeOffset,
								   Common::Array<byte> &outBlob) {
	outBlob.clear();
	AmigaAnimSlotInfo info;
	if (!inspectAmigaAnimSlot(mxoo, mxooSize, bodyRelativeOffset, info) || !info.valid)
		return false;

	const byte *planar = mxoo + info.pixelOffset;

	// DOS AnimBlobView layout (LE):
	//   +0x00 unknown, +0x02 seqPos, +0x04 repeat, +0x06 loopStart, +0x08 delay,
	//   +0x0A seqLenMinusOne; sequence at +0x0C (N bytes);
	//   frames at +0x0B + (N+1) = +0x0C + N: uint16 frameCount, then per-frame records
	// Frame: ox,oy,unk,w,h (LE) + chunky pixels
	Common::Array<byte> sequence;
	for (uint16 i = 0; i < info.frameCount; i++)
		sequence.push_back((byte)(10 + i));

	const uint32 seqLenMinusOne = sequence.size();
	const uint32 frameDataOffset = 0x0C + seqLenMinusOne;
	uint32 total = frameDataOffset + 2; // + frameCount word
	for (uint16 f = 0; f < info.frameCount; f++)
		total += 10 + (uint32)info.width * info.height;

	outBlob.resize(total);
	byte *dst = outBlob.data();
	WRITE_LE_UINT16(dst + 0x00, info.headerHint);
	WRITE_LE_UINT16(dst + 0x02, info.seqPos ? info.seqPos : 1);
	WRITE_LE_UINT16(dst + 0x04, info.repeatCounter);
	WRITE_LE_UINT16(dst + 0x06, info.loopStart);
	WRITE_LE_UINT16(dst + 0x08, 0); // delay
	WRITE_LE_UINT16(dst + 0x0A, (uint16)seqLenMinusOne);
	if (!sequence.empty())
		memcpy(dst + 0x0C, sequence.data(), sequence.size());

	WRITE_LE_UINT16(dst + frameDataOffset, info.frameCount);
	uint32 off = frameDataOffset + 2;
	for (uint16 f = 0; f < info.frameCount; f++) {
		Common::Array<byte> pixels;
		if (!decodeAmigaPlanarFrame(planar, info.width, info.height, f, info.frameCount, pixels))
			return false;
		WRITE_LE_UINT16(dst + off + 0, 0); // ox
		WRITE_LE_UINT16(dst + off + 2, 0); // oy
		WRITE_LE_UINT16(dst + off + 4, 0); // unk
		WRITE_LE_UINT16(dst + off + 6, info.width);
		WRITE_LE_UINT16(dst + off + 8, info.height);
		memcpy(dst + off + 10, pixels.data(), pixels.size());
		off += 10 + pixels.size();
	}
	return true;
}

bool extractAmigaScript(const byte *mxoo, uint32 mxooSize, Common::Array<byte> &outScript) {
	outScript.clear();
	AmigaMxooInfo info;
	if (!parseAmigaMxoo(mxoo, mxooSize, info))
		return false;
	if (info.scriptOffset + 4 > mxooSize)
		return false;
	// Script section: u16BE pad, u16BE size, then bytecode
	const uint16 size = READ_BE_UINT16(mxoo + info.scriptOffset + 2);
	const uint32 dataOff = info.scriptOffset + 4;
	if (dataOff + size > mxooSize)
		return false;
	outScript.resize(size);
	if (size)
		memcpy(outScript.data(), mxoo + dataOff, size);
	return true;
}

bool extractAmigaStringBlock(const byte *mxoo, uint32 mxooSize, Common::Array<byte> &outStrings) {
	outStrings.clear();
	AmigaMxooInfo info;
	if (!parseAmigaMxoo(mxoo, mxooSize, info))
		return false;
	// String section: u16BE pad, u16BE size, then length-prefixed plaintext entries.
	// Script string offsets are relative to the entries (after the 4-byte header).
	if (info.stringOffset + 4 > mxooSize)
		return false;
	const uint16 size = READ_BE_UINT16(mxoo + info.stringOffset + 2);
	const uint32 dataOff = info.stringOffset + 4;
	if (dataOff + size > mxooSize)
		return false;
	outStrings.resize(size);
	if (size)
		memcpy(outStrings.data(), mxoo + dataOff, size);
	return true;
}

bool convertAmigaSimpleSpriteToDosBlob(const byte *mxoo, uint32 mxooSize, Common::Array<byte> &outBlob) {
	outBlob.clear();
	if (!mxoo || mxooSize < 32 || READ_BE_UINT32(mxoo) != MKTAG('M', 'X', 'O', 'O'))
		return false;

	const uint32 scriptOff = READ_BE_UINT32(mxoo + 4);
	if (scriptOff <= 12 || scriptOff > mxooSize)
		return false;

	const byte *body = mxoo + 12;
	const uint32 bodyLen = scriptOff - 12;
	if (bodyLen < 20 || body[10] != 1 || body[11] != 1)
		return false;

	const uint16 frames = READ_BE_UINT16(body + 14);
	const uint16 width = READ_BE_UINT16(body + 16);
	const uint16 height = READ_BE_UINT16(body + 18);
	if (frames == 0 || width == 0 || width > 320 || height == 0 || height > 200)
		return false;

	const uint32 rowBytes = (width + 7) / 8;
	const uint32 planeSize = rowBytes * height;
	const uint32 pixelBytes = bodyLen - 20;
	if (planeSize == 0 || pixelBytes < planeSize * 5)
		return false;
	const uint32 numPlanes = pixelBytes / planeSize;
	if (numPlanes < 5 || numPlanes > 6 || pixelBytes != planeSize * numPlanes)
		return false;

	Common::Array<byte> pixels;
	pixels.resize((uint32)width * height);
	Common::fill(pixels.begin(), pixels.end(), 0);
	const byte *pixData = body + 20;
	for (uint32 plane = 0; plane < 5 && plane < numPlanes; plane++) {
		const uint32 pOff = plane * planeSize;
		for (uint16 y = 0; y < height; y++) {
			for (uint32 bx = 0; bx < rowBytes; bx++) {
				const byte b = pixData[pOff + y * rowBytes + bx];
				for (int bit = 0; bit < 8; bit++) {
					const uint16 x = (uint16)(bx * 8 + bit);
					if (x < width && (b & (0x80 >> bit)))
						pixels[y * width + x] |= (byte)(1 << plane);
				}
			}
		}
	}
	if (numPlanes >= 6) {
		const uint32 pOff = 5 * planeSize;
		for (uint16 y = 0; y < height; y++) {
			for (uint32 bx = 0; bx < rowBytes; bx++) {
				const byte b = pixData[pOff + y * rowBytes + bx];
				for (int bit = 0; bit < 8; bit++) {
					const uint16 x = (uint16)(bx * 8 + bit);
					if (x < width && (b & (0x80 >> bit)) == 0)
						pixels[y * width + x] = 0;
				}
			}
		}
	}

	// seqLenMinusOne=0 -> sequenceLength=1 -> frame data at 0x0C:
	// uint16 frameCount, then ox,oy,unk,w,h + pixels.
	const uint32 total = 0x0C + 2 + 10 + (uint32)width * height;
	outBlob.resize(total);
	byte *dst = outBlob.data();
	WRITE_LE_UINT16(dst + 0x00, frames);
	WRITE_LE_UINT16(dst + 0x02, 1);
	WRITE_LE_UINT16(dst + 0x04, 0);
	WRITE_LE_UINT16(dst + 0x06, 0);
	WRITE_LE_UINT16(dst + 0x08, 0);
	WRITE_LE_UINT16(dst + 0x0A, 0); // seqLenMinusOne
	WRITE_LE_UINT16(dst + 0x0C, 1); // frameCount
	WRITE_LE_UINT16(dst + 0x0E + 0, 0);
	WRITE_LE_UINT16(dst + 0x0E + 2, 0);
	WRITE_LE_UINT16(dst + 0x0E + 4, 0);
	WRITE_LE_UINT16(dst + 0x0E + 6, width);
	WRITE_LE_UINT16(dst + 0x0E + 8, height);
	memcpy(dst + 0x0E + 10, pixels.data(), pixels.size());
	return true;
}

bool convertAmigaPortraitAtlasToDosBlob(const byte *mxoo, uint32 mxooSize, uint32 bodyRelativeExtraOffset,
										Common::Array<byte> &outBlob) {
	outBlob.clear();
	if (!mxoo || bodyRelativeExtraOffset == 0 || bodyRelativeExtraOffset == 0xFFFFFFFF)
		return false;

	const uint32 absExtra = 12 + bodyRelativeExtraOffset;
	const uint16 atlasW = 320;
	const uint16 atlasH = 72;
	const uint16 frameW = 80;
	const uint16 frameH = 72;
	const uint16 frameCount = 4;
	const uint32 rowBytes = (atlasW + 7) / 8;
	const uint32 planeBytes = rowBytes * atlasH;
	const uint32 portraitBytes = planeBytes * 5; // 14400
	if (absExtra + portraitBytes > mxooSize)
		return false;

	const byte *src = mxoo + absExtra;
	Common::Array<byte> atlas;
	atlas.resize((uint32)atlasW * atlasH);
	Common::fill(atlas.begin(), atlas.end(), 0);
	for (uint16 y = 0; y < atlasH; y++) {
		for (uint16 x = 0; x < atlasW; x++) {
			byte color = 0;
			const uint32 bitIndex = (uint32)x & 7;
			const uint32 byteInRow = (uint32)x >> 3;
			for (uint16 plane = 0; plane < 5; plane++) {
				const byte *planeRow = src + plane * planeBytes + (uint32)y * rowBytes;
				if (planeRow[byteInRow] & (0x80 >> bitIndex))
					color |= (byte)(1 << plane);
			}
			atlas[(uint32)y * atlasW + x] = color;
		}
	}

	// DOS anim blob: seqLenMinusOne = frameCount, sequence at +0x0C, frames follow.
	const uint32 seqLenMinusOne = frameCount;
	const uint32 frameDataOffset = 0x0C + seqLenMinusOne;
	const uint32 frameBytes = 10 + (uint32)frameW * frameH;
	const uint32 total = frameDataOffset + 2 + (uint32)frameCount * frameBytes;
	outBlob.resize(total);
	byte *dst = outBlob.data();
	WRITE_LE_UINT16(dst + 0x00, frameCount);
	WRITE_LE_UINT16(dst + 0x02, 1);
	WRITE_LE_UINT16(dst + 0x04, 0);
	WRITE_LE_UINT16(dst + 0x06, 0);
	WRITE_LE_UINT16(dst + 0x08, 0);
	WRITE_LE_UINT16(dst + 0x0A, (uint16)seqLenMinusOne);
	for (uint16 i = 0; i < frameCount; i++)
		dst[0x0C + i] = (byte)(10 + i);

	WRITE_LE_UINT16(dst + frameDataOffset, frameCount);
	uint32 off = frameDataOffset + 2;
	for (uint16 f = 0; f < frameCount; f++) {
		WRITE_LE_UINT16(dst + off + 0, 0);
		WRITE_LE_UINT16(dst + off + 2, 0);
		WRITE_LE_UINT16(dst + off + 4, 0);
		WRITE_LE_UINT16(dst + off + 6, frameW);
		WRITE_LE_UINT16(dst + off + 8, frameH);
		byte *pixDst = dst + off + 10;
		for (uint16 y = 0; y < frameH; y++) {
			memcpy(pixDst + (uint32)y * frameW,
				   atlas.data() + (uint32)y * atlasW + (uint32)f * frameW,
				   frameW);
		}
		off += frameBytes;
	}
	return true;
}

static bool decompressPp20ToBuffer(const byte *src, uint32 srcLen, Common::Array<byte> &out) {
	out.clear();
	if (!src || srcLen < 12 || READ_BE_UINT32(src) != MKTAG('P', 'P', '2', '0'))
		return false;

	uint32 outLen = 0;
	byte *unpacked = Common::PowerPackerStream::unpackBuffer(src, srcLen, outLen);
	if (!unpacked || outLen == 0) {
		delete[] unpacked;
		return false;
	}
	out.resize(outLen);
	memcpy(out.data(), unpacked, outLen);
	delete[] unpacked;
	return true;
}

bool decodeAmigaMxmmSceneBackground(const byte *mxmm, uint32 mxmmSize,
									Common::Array<byte> &outPixels,
									byte outPaletteRgb[768],
									uint &outColorCount) {
	outPixels.clear();
	outColorCount = 0;
	if (outPaletteRgb) {
		for (uint i = 0; i < 768; i++)
			outPaletteRgb[i] = 0;
	}
	if (!mxmm || mxmmSize < 14 || !outPaletteRgb)
		return false;
	if (READ_BE_UINT32(mxmm) != MKTAG('M', 'X', 'M', 'M'))
		return false;

	const uint32 chunk0Size = READ_BE_UINT32(mxmm + 10);
	if (chunk0Size == 0 || 14 + chunk0Size > mxmmSize)
		return false;

	const byte *chunk0 = mxmm + 14;
	Common::Array<byte> screen;
	if (READ_BE_UINT32(chunk0) == MKTAG('P', 'P', '2', '0')) {
		if (!decompressPp20ToBuffer(chunk0, chunk0Size, screen))
			return false;
	} else {
		screen.resize(chunk0Size);
		memcpy(screen.data(), chunk0, chunk0Size);
	}

	if (screen.size() < kAmigaSceneScreenSize)
		return false;

	Common::Array<byte> planarIndex;
	planarIndex.resize((uint)kAmigaSceneWidth * kAmigaSceneHeight);
	Common::fill(planarIndex.begin(), planarIndex.end(), 0);

	const byte *planes = screen.data();
	const uint32 rowBytes = kAmigaSceneWidth / 8; // 40
	for (uint16 plane = 0; plane < kAmigaScenePlanes; plane++) {
		const byte *planeBase = planes + (uint32)plane * kAmigaScenePlaneBytes;
		for (uint16 y = 0; y < kAmigaSceneHeight; y++) {
			for (uint32 bx = 0; bx < rowBytes; bx++) {
				const byte b = planeBase[(uint32)y * rowBytes + bx];
				for (int bit = 0; bit < 8; bit++) {
					const uint16 x = (uint16)(bx * 8 + bit);
					if (b & (0x80 >> bit))
						planarIndex[(uint32)y * kAmigaSceneWidth + x] |= (byte)(1 << plane);
				}
			}
		}
	}

	const byte *copper = screen.data() + kAmigaSceneCopperOffset;
	uint16 base16[16];
	for (uint i = 0; i < 16; i++)
		base16[i] = READ_BE_UINT16(copper + i * 2);
	const byte *lineColors = copper + 0x20; // 200 × 16 × u16BE

	auto amiga12ToRgb8 = [](uint16 rgb, byte &r, byte &g, byte &b) {
		const byte r4 = (rgb >> 8) & 0xF;
		const byte g4 = (rgb >> 4) & 0xF;
		const byte b4 = rgb & 0xF;
		r = (byte)(r4 * 17);
		g = (byte)(g4 * 17);
		b = (byte)(b4 * 17);
	};

	auto buildPal32 = [&](uint16 y, byte pal32[32][3]) {
		amiga12ToRgb8(base16[0], pal32[0][0], pal32[0][1], pal32[0][2]);
		for (uint i = 0; i < 16; i++) {
			const uint16 c = READ_BE_UINT16(lineColors + (uint32)y * 32 + i * 2);
			amiga12ToRgb8(c, pal32[1 + i][0], pal32[1 + i][1], pal32[1 + i][2]);
		}
		for (uint i = 1; i < 16; i++)
			amiga12ToRgb8(base16[i], pal32[16 + i][0], pal32[16 + i][1], pal32[16 + i][2]);
	};

	// Reserve 0..31 for Amiga COLOR registers (sprites) and 32..63 for EHB.
	byte staticPal[32][3];
	buildPal32(0, staticPal);
	for (uint i = 0; i < 32; i++) {
		outPaletteRgb[i * 3 + 0] = staticPal[i][0];
		outPaletteRgb[i * 3 + 1] = staticPal[i][1];
		outPaletteRgb[i * 3 + 2] = staticPal[i][2];
	}
	for (uint i = 0; i < 32; i++) {
		outPaletteRgb[(32 + i) * 3 + 0] = (byte)(staticPal[i][0] / 2);
		outPaletteRgb[(32 + i) * 3 + 1] = (byte)(staticPal[i][1] / 2);
		outPaletteRgb[(32 + i) * 3 + 2] = (byte)(staticPal[i][2] / 2);
	}
	outColorCount = 64;

	Common::HashMap<uint32, byte> colorToIndex;
	for (uint i = 0; i < 64; i++) {
		const uint32 key = ((uint32)outPaletteRgb[i * 3 + 0] << 16) |
						   ((uint32)outPaletteRgb[i * 3 + 1] << 8) |
						   outPaletteRgb[i * 3 + 2];
		if (!colorToIndex.contains(key))
			colorToIndex[key] = (byte)i;
	}

	outPixels.resize((uint)kAmigaSceneWidth * kAmigaSceneHeight);
	for (uint16 y = 0; y < kAmigaSceneHeight; y++) {
		byte pal32[32][3];
		buildPal32(y, pal32);

		for (uint16 x = 0; x < kAmigaSceneWidth; x++) {
			byte idx = planarIndex[(uint32)y * kAmigaSceneWidth + x];
			byte r, g, b;
			if (idx >= 32) {
				const byte base = (byte)(idx - 32);
				r = (byte)(pal32[base][0] / 2);
				g = (byte)(pal32[base][1] / 2);
				b = (byte)(pal32[base][2] / 2);
			} else {
				r = pal32[idx][0];
				g = pal32[idx][1];
				b = pal32[idx][2];
			}

			const uint32 key = ((uint32)r << 16) | ((uint32)g << 8) | b;
			byte outIdx;
			if (colorToIndex.contains(key)) {
				outIdx = colorToIndex[key];
			} else if (outColorCount < 256) {
				outIdx = (byte)outColorCount;
				colorToIndex[key] = outIdx;
				outPaletteRgb[outIdx * 3 + 0] = r;
				outPaletteRgb[outIdx * 3 + 1] = g;
				outPaletteRgb[outIdx * 3 + 2] = b;
				outColorCount++;
			} else {
				outIdx = idx < 64 ? idx : (byte)(idx & 31);
			}
			outPixels[(uint32)y * kAmigaSceneWidth + x] = outIdx;
		}
	}

	return outColorCount > 0;
}

static bool amigaMxmmIterChunks(const byte *mxmm, uint32 mxmmSize, uint32 &outTrailerOff,
								Common::Array<const byte *> *outChunkPtrs,
								Common::Array<uint32> *outChunkSizes) {
	outTrailerOff = 0;
	if (!mxmm || mxmmSize < 14 || READ_BE_UINT32(mxmm) != MKTAG('M', 'X', 'M', 'M'))
		return false;

	uint32 pos = 10;
	while (pos + 4 <= mxmmSize) {
		const uint32 chunkSize = READ_BE_UINT32(mxmm + pos);
		if (chunkSize == 0)
			break;
		if (pos + 4 + chunkSize > mxmmSize)
			break;
		if (outChunkPtrs && outChunkSizes) {
			outChunkPtrs->push_back(mxmm + pos + 4);
			outChunkSizes->push_back(chunkSize);
		}
		pos += 4 + chunkSize;
	}
	outTrailerOff = pos;
	return true;
}

static bool decompressAmigaChunkToBuffer(const byte *chunk, uint32 chunkSize, Common::Array<byte> &out) {
	out.clear();
	if (!chunk || chunkSize == 0)
		return false;
	if (READ_BE_UINT32(chunk) == MKTAG('P', 'P', '2', '0'))
		return decompressPp20ToBuffer(chunk, chunkSize, out);
	out.resize(chunkSize);
	memcpy(out.data(), chunk, chunkSize);
	return true;
}

bool extractAmigaMxmmSceneScript(const byte *mxmm, uint32 mxmmSize,
								 Common::Array<byte> &outScript,
								 Common::Array<byte> &outStrings) {
	outScript.clear();
	outStrings.clear();

	uint32 trailerOff = 0;
	if (!amigaMxmmIterChunks(mxmm, mxmmSize, trailerOff, nullptr, nullptr))
		return false;
	if (trailerOff >= mxmmSize)
		return false;

	const byte *trailer = mxmm + trailerOff;
	uint32 trailerSize = mxmmSize - trailerOff;
	uint32 pos = 0;

	// Skip leading empty u32 markers (optional trailing chunk sizes of 0).
	while (pos + 8 <= trailerSize && READ_BE_UINT32(trailer + pos) == 0)
		pos += 4;

	if (pos + 8 > trailerSize)
		return false;

	const uint32 scriptSize = READ_BE_UINT32(trailer + pos);
	pos += 4;
	pos += 4; // unknown / alternate size field — not the plaintext string length
	if (scriptSize == 0 || pos + scriptSize > trailerSize)
		return false;

	const byte *script = trailer + pos;
	// Accept empty/tiny stubs (e.g. single 0x18) and real scene scripts.
	if (scriptSize >= 2) {
		const byte op = script[0];
		const byte len = script[1];
		if (len > 32 || (uint32)2 + len > scriptSize)
			return false;
		if (op != 0x04 && op != 0x05 && op != 0x01 && op != 0x0F && op != 0x0C && op != 0x18 && op != 0x07)
			return false;
	}

	outScript.resize(scriptSize);
	memcpy(outScript.data(), script, scriptSize);
	pos += scriptSize;

	if (pos + 4 <= trailerSize) {
		const uint32 stringBytes = READ_BE_UINT32(trailer + pos);
		if (stringBytes > 0 && pos + 4 + stringBytes <= trailerSize) {
			// Store entries only (script string offsets are relative to the entry stream).
			outStrings.resize(stringBytes);
			memcpy(outStrings.data(), trailer + pos + 4, stringBytes);
		}
	}

	return !outScript.empty();
}

bool extractAmigaMxmmSceneMaps(const byte *mxmm, uint32 mxmmSize,
							   Common::Array<byte> &outPathfinding,
							   Common::Array<byte> &outDepth,
							   Common::Array<byte> &outShadow) {
	outPathfinding.clear();
	outDepth.clear();
	outShadow.clear();

	uint32 trailerOff = 0;
	Common::Array<const byte *> chunkPtrs;
	Common::Array<uint32> chunkSizes;
	if (!amigaMxmmIterChunks(mxmm, mxmmSize, trailerOff, &chunkPtrs, &chunkSizes))
		return false;
	if (chunkPtrs.size() < 2)
		return false;

	const uint32 kMapBytes = (uint32)kAmigaSceneWidth * kAmigaSceneHeight; // 64000
	Common::Array<byte> maps[3];
	uint mapCount = 0;

	// Chunk0 is the planar screen; subsequent 64000-byte buffers are maps.
	for (uint i = 1; i < chunkPtrs.size() && mapCount < 3; i++) {
		Common::Array<byte> decoded;
		if (!decompressAmigaChunkToBuffer(chunkPtrs[i], chunkSizes[i], decoded))
			continue;
		if (decoded.size() != kMapBytes)
			continue;
		maps[mapCount++] = Common::move(decoded);
	}

	if (mapCount == 0)
		return false;

	// Demo MM_0004: chunk1 = walkability-like, chunk2 = often empty.
	// Map to DOS surfaces: pathfinding, depth, shadow.
	if (mapCount >= 1)
		outPathfinding = Common::move(maps[0]);
	if (mapCount >= 2)
		outDepth = Common::move(maps[1]);
	if (mapCount >= 3)
		outShadow = Common::move(maps[2]);
	return true;
}

} // End of namespace Macs2
