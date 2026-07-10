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

#include "twine/parser/texture.h"

namespace TwinE {

void BodyTextureData::reset() {
	_pages.clear();
}

bool BodyTextureData::loadFromStream(Common::SeekableReadStream &stream, bool lba1) {
	reset();
	if (lba1) {
		return false;
	}
	const int32 size = stream.size();
	if (size <= 0) {
		return false;
	}
	const int32 numPages = (size + kPageSize - 1) / kPageSize;
	_pages.resize(numPages * kPageSize, 0);
	for (int32 i = 0; i < numPages; ++i) {
		const int32 toRead = MIN<int32>(kPageSize, size - i * kPageSize);
		stream.read(_pages.data() + i * kPageSize, toRead);
	}
	return !stream.err();
}

const uint8 *BodyTextureData::getPage(int page) const {
	if (page < 0 || page >= pageCount()) {
		return nullptr;
	}
	return _pages.data() + page * kPageSize;
}

const uint8 *BodyTextureData::getAtOffset(uint32 offset) const {
	if (_pages.empty() || offset >= _pages.size()) {
		return nullptr;
	}
	return _pages.data() + offset;
}

} // namespace TwinE
