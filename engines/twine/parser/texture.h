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

#ifndef TWINE_PARSER_TEXTURE_H
#define TWINE_PARSER_TEXTURE_H

#include "common/array.h"
#include "common/stream.h"
#include "twine/parser/parser.h"

namespace TwinE {

/** LBA2 body texture atlas page (256x256 palette indices, index 0 = transparent). */
class BodyTextureData : public Parser {
private:
	static constexpr int kPageSize = 256 * 256;
	Common::Array<uint8> _pages;

protected:
	void reset() override;

public:
	bool loadFromStream(Common::SeekableReadStream &stream, bool lba1) override;

	int pageCount() const {
		return _pages.size() / kPageSize;
	}

	const uint8 *getPage(int page) const;
	const uint8 *getAtOffset(uint32 offset) const;
};

} // namespace TwinE

#endif
