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

#include "common/array.h"
#include "twine/scene/pof.h"
#include "twine/renderer/renderer.h"
#include "twine/resources/hqr.h"
#include "twine/resources/resources.h"
#include "twine/twine.h"

namespace TwinE {

Pof::Pof(TwinEEngine *engine) : _engine(engine) {
}

Pof::~Pof() {
	free(_buffer);
	_buffer = nullptr;
}

bool Pof::init() {
	free(_buffer);
	_buffer = nullptr;
	const int32 size = HQR::getAllocEntry(&_buffer, Resources::HQR_RESS_FILE, RESSHQR_POF);
	return size > 0 && _buffer != nullptr;
}

bool Pof::display(int32 x, int32 y, int32 z, int32 numPof, int32 scale, int32 rotation) const {
	if (_buffer == nullptr || numPof < 0) {
		return false;
	}

	const uint32 numPofs = READ_LE_UINT32(_buffer);
	if ((uint32)numPof >= numPofs) {
		return false;
	}

	const uint32 offset = READ_LE_UINT32(_buffer + 4 + numPof * 4);
	const uint8 *ptr = _buffer + offset;
	if (ptr >= _buffer + 4 + numPofs * 4) {
		return false;
	}

	const IVec3 &world = _engine->_renderer->longWorldRot(x, y, z);
	IVec3 projTop;
	if (!_engine->_renderer->longProjectPoint(world, projTop)) {
		return false;
	}

	const IVec3 &worldBase = _engine->_renderer->longWorldRot(x, y + 1000, z);
	IVec3 projBase;
	if (!_engine->_renderer->longProjectPoint(worldBase, projBase)) {
		return false;
	}

	const int32 xpc = projTop.x * 65536 + 32768;
	const int32 ypc = projTop.y * 65536 + 32768;
	const int32 scale3d = scale ? ((projTop.y - projBase.y) * 65536 + scale / 2) / scale : 65536;

	const uint8 color = *ptr++;
	const uint8 numPoints = *ptr++;
	Common::Array<IVec2> points(numPoints);

	const int16 *coords = (const int16 *)ptr;
	ptr += numPoints * 2 * sizeof(int16);

	for (uint8 p = 0; p < numPoints; ++p) {
		int32 px = coords[p * 2];
		int32 py = coords[p * 2 + 1];
		if (rotation != 0) {
			const IVec2 &rot = _engine->_renderer->rotate(px, py, rotation);
			px = rot.x;
			py = rot.y;
		}
		points[p].x = (xpc + px * scale3d) / 65536;
		points[p].y = (ypc + py * scale3d) / 65536;
	}

	uint8 numLines = *ptr++;
	while (numLines > 0) {
		const uint8 idx0 = *ptr++;
		const uint8 idx1 = *ptr++;
		if (idx0 < numPoints && idx1 < numPoints) {
			_engine->_workVideoBuffer.drawLine(points[idx0].x, points[idx0].y,
			                                  points[idx1].x, points[idx1].y, color);
		}
		--numLines;
	}

	return true;
}

} // namespace TwinE
