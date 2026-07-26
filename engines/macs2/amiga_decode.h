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

#ifndef MACS2_AMIGA_DECODE_H
#define MACS2_AMIGA_DECODE_H

#include "common/array.h"
#include "common/scummsys.h"

namespace Macs2 {

/**
 * Amiga MXOO object body layout (after the 12-byte MXOO header):
 *   +0x00..0x0B  padding
 *   +0x0C        signature 0x0101
 *   +0x0E        21 x uint32BE slot offsets (0 / 0xFFFFFFFF = empty)
 *   +0x62        uint32BE offset of extra/portrait section (often end of anims)
 *
 * Each anim slot is planar 6-plane frame data with a short BE header.
 * Script bytecode is identical to DOS (LE operands). Strings are plaintext
 * with uint16BE length prefixes (no XOR cipher).
 *
 * Game object index = Amiga OO resource id + 1 (OO_0000 -> object 1).
 */
struct AmigaMxooInfo {
	uint32 scriptOffset = 0;
	uint32 stringOffset = 0;
	uint32 slotOffsets[21];
	uint32 extraOffset = 0;
	uint32 bodyOffset = 12; // start of body within MXOO
	uint32 bodySize = 0;

	AmigaMxooInfo() {
		for (uint i = 0; i < 21; i++)
			slotOffsets[i] = 0xFFFFFFFF;
	}
};

struct AmigaAnimSlotInfo {
	uint16 frameCount = 0;
	uint16 width = 0;
	uint16 height = 0;
	uint16 seqPos = 0;
	uint16 repeatCounter = 0;
	uint16 loopStart = 0;
	uint16 headerHint = 0;
	uint32 pixelOffset = 0; // absolute offset in MXOO of first planar byte
	uint32 headerSize = 0;  // bytes from slot start to pixel data
	bool valid = false;
};

bool parseAmigaMxoo(const byte *mxoo, uint32 mxooSize, AmigaMxooInfo &out);
bool inspectAmigaAnimSlot(const byte *mxoo, uint32 mxooSize, uint32 bodyRelativeOffset, AmigaAnimSlotInfo &out);

/** Decode one frame of planar Amiga anim data to chunky 8bpp (color planes 0..4). */
bool decodeAmigaPlanarFrame(const byte *planar, uint16 width, uint16 height, uint16 frameIndex,
							uint16 frameCount, Common::Array<byte> &outPixels);

/**
 * Convert an Amiga anim slot into a DOS-compatible animation blob so the
 * existing AnimBlobView / renderer path can consume it.
 */
bool convertAmigaAnimSlotToDosBlob(const byte *mxoo, uint32 mxooSize, uint32 bodyRelativeOffset,
								   Common::Array<byte> &outBlob);

/** Extract script bytecode (without the MXOO script section header). */
bool extractAmigaScript(const byte *mxoo, uint32 mxooSize, Common::Array<byte> &outScript);

/**
 * Extract Amiga string entries (u16BE length + plaintext), skipping the
 * MXOO string-section header. Offsets used by scripts are relative to this block.
 */
bool extractAmigaStringBlock(const byte *mxoo, uint32 mxooSize, Common::Array<byte> &outStrings);

/** Convert a simple planar MXOO sprite (cursor/icon) into a 1-frame DOS anim blob. */
bool convertAmigaSimpleSpriteToDosBlob(const byte *mxoo, uint32 mxooSize, Common::Array<byte> &outBlob);

/**
 * Convert Amiga dialogue portrait atlas (body+0x62) into a DOS anim blob.
 * Demo atlases are 320×72×5 planar = four 80×72 faces side-by-side.
 */
bool convertAmigaPortraitAtlasToDosBlob(const byte *mxoo, uint32 mxooSize, uint32 bodyRelativeExtraOffset,
										Common::Array<byte> &outBlob);

/** Decompressed MXMM chunk0 screen buffer size (6×8000 planes + copper block). */
enum : uint32 {
	kAmigaSceneScreenSize = 54432, // 0xD4A0
	kAmigaSceneCopperOffset = 0xBB80,
	kAmigaSceneCopperSize = 0x1920, // 16 base colors + 200×16 line colors
	kAmigaScenePlaneBytes = 8000 // 40×200
};
enum : uint16 {
	kAmigaSceneWidth = 320,
	kAmigaSceneHeight = 200,
	kAmigaScenePlanes = 6 // BPLCON0 = 0x6200 → EHB
};

/**
 * Decode MXMM scene package chunk0 into chunky 8bpp 320×200 pixels and an RGB8
 * palette (up to 256 entries).
 *
 * Palette layout (matches Amiga copper / sprite drawing):
 * - indices 0..31: COLOR00..31 from the copper base block + first scanline
 * - indices 32..63: Extra HalfBrite of 0..31 (BPLCON0 0x6200)
 * - indices 64..*: extra colors needed for per-scanline copper differences
 *
 * Character/OO sprites use planes 0..4 against COLOR00..31, so 0..31 must stay
 * stable Amiga hardware colors.
 */
bool decodeAmigaMxmmSceneBackground(const byte *mxmm, uint32 mxmmSize,
									Common::Array<byte> &outPixels,
									byte outPaletteRgb[768],
									uint &outColorCount);

/**
 * Extract scene script + string block from an MXMM package trailer.
 *
 * After the size-prefixed chunks (BG / maps / MXCC / …), the trailer is:
 *   [optional u32 zero markers]
 *   u32BE scriptSize
 *   u32BE unknown (ignored)
 *   script[scriptSize]          — DOS-identical LE bytecode
 *   [optional] u32BE stringBytes + u16BE-length plaintext entries
 *
 * Ghidra: load_scene_mxmm reads this after planar BG / map chunks.
 * Script-visible scene ids are MM_resource_id + 1 (changeScene subtracts 1
 * before MM lookup; curScene is set to mmId+1 after load).
 */
bool extractAmigaMxmmSceneScript(const byte *mxmm, uint32 mxmmSize,
								 Common::Array<byte> &outScript,
								 Common::Array<byte> &outStrings);

/**
 * Decode MXMM map chunks (320×200) after chunk0 into pathfinding / depth /
 * shadow surfaces when present. Missing chunks leave the destination unchanged.
 */
bool extractAmigaMxmmSceneMaps(const byte *mxmm, uint32 mxmmSize,
							   Common::Array<byte> &outPathfinding,
							   Common::Array<byte> &outDepth,
							   Common::Array<byte> &outShadow);

} // End of namespace Macs2

#endif // MACS2_AMIGA_DECODE_H
