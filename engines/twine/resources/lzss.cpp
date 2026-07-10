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

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "twine/resources/lzss.h"
#include "common/textconsole.h"
#include "common/array.h"

namespace TwinE {

void expandLZ(uint8 *dst, const uint8 *src, uint32 decompSize, uint32 minBloc) {
	uint32 dstOffset = 0;
	uint32 srcOffset = 0;

	while (dstOffset < decompSize) {
		const uint8 flag = src[srcOffset++];
		uint8 bit = flag;
		for (int i = 0; i < 8; i++) {
			if (dstOffset >= decompSize) {
				break;
			}
			if ((bit & 0x1) != 0) {
				dst[dstOffset++] = src[srcOffset++];
			} else {
				const uint32 blockLength = (src[srcOffset] & 0x0f) + minBloc;
				const uint32 blockOffset = ((uint32)(src[srcOffset + 1]) << 4) | (src[srcOffset] >> 4);
				srcOffset += 2;
				for (uint32 j = 0; j < blockLength; j++) {
					dst[dstOffset] = dst[dstOffset - blockOffset - 1];
					dstOffset++;
					if (dstOffset >= decompSize) {
						break;
					}
				}
			}
			bit >>= 1;
		}
	}
}

LzssReadStream::LzssReadStream(Common::ReadStream *indata, uint32 mode, uint32 realsize) {
	_outLzssBufData = new uint8[realsize]();
	decodeLZSS(indata, mode, realsize);
	_size = realsize;
	_pos = 0;
	delete indata;
}

LzssReadStream::~LzssReadStream() {
	delete[] _outLzssBufData;
}

void LzssReadStream::decodeLZSS(Common::ReadStream *in, uint32 mode, uint32 dataSize) {
	if (in->eos() || in->err() || dataSize == 0) {
		_err = dataSize > 0;
		return;
	}

	Common::Array<uint8> compBuf;
	if (Common::SeekableReadStream *seekable = dynamic_cast<Common::SeekableReadStream *>(in)) {
		const int64 compSize = seekable->size();
		if (compSize > 0) {
			compBuf.resize((uint)compSize);
			if (seekable->read(compBuf.data(), compSize) != compSize) {
				_err = true;
				return;
			}
		}
	}
	if (compBuf.empty()) {
		while (!in->eos() && !in->err()) {
			compBuf.push_back(in->readByte());
		}
	}
	if (compBuf.empty()) {
		_err = true;
		return;
	}

	expandLZ(_outLzssBufData, compBuf.data(), dataSize, mode + 1);
}

bool LzssReadStream::eos() const {
	return _pos >= _size;
}

uint32 LzssReadStream::read(void *buf, uint32 dataSize) {
	if (dataSize > _size - _pos) {
		_err = true;
		return 0;
	}

	memcpy(buf, &_outLzssBufData[_pos], dataSize);
	_pos += dataSize;

	return dataSize;
}

bool LzssReadStream::seek(int64 offset, int whence) {
	if (whence == SEEK_SET) {
		_pos = offset;
	} else if (whence == SEEK_CUR) {
		_pos += offset;
	}
	return true;
}

} // namespace TwinE
