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

#include "twine/scene/dart.h"
#include "common/stream.h"
#include "twine/audio/sound.h"
#include "twine/parser/body.h"
#include "twine/renderer/redraw.h"
#include "twine/resources/resources.h"
#include "twine/scene/collision.h"
#include "twine/scene/extra.h"
#include "twine/scene/gamestate.h"
#include "twine/scene/grid.h"
#include "twine/scene/scene.h"

namespace TwinE {

void Dart::InitDarts() {
	int32 x0 = -64, x1 = 64, y0 = -64, y1 = 64, z0 = -64, z1 = 64;
	if (_dartBody.loadFromHQR(TwineResource(Resources::HQR_BODY_FILE, BODY_3D_DART), false)) {
		x0 = _dartBody.bbox.mins.x;
		x1 = _dartBody.bbox.maxs.x;
		y0 = _dartBody.bbox.mins.y;
		y1 = _dartBody.bbox.maxs.y;
		z0 = _dartBody.bbox.mins.z;
		z1 = _dartBody.bbox.maxs.z;
	} else {
		warning("Failed to load dart body bounds for index %i", BODY_3D_DART);
	}

	// Average
	int32 size = ((x1 - x0) + (z1 - z0)) / 4;

	T_DART *ptrd = ListDart;

	for (uint32 t = 0; t < MAX_DARTS; t++, ptrd++) {
		ptrd->Body = BODY_3D_DART;

		ptrd->XMin = -size;
		ptrd->XMax = size;
		ptrd->YMin = y0;
		ptrd->YMax = y1;
		ptrd->ZMin = -size;
		ptrd->ZMax = size;

		ptrd->Flags = 0;
		ptrd->NumCube = -1;
	}
}

int32 Dart::GetDart() {
	T_DART *ptrd;
	int32 t;

	ptrd = ListDart;

	for (t = 0; t < MAX_DARTS; t++, ptrd++) {
		if (ptrd->Flags & DART_TAKEN) {
			return t;
		}
	}

	return -1;
}

void Dart::TakeAllDarts() {
	T_DART *ptrd;
	int32 n;

	ptrd = ListDart;

	for (n = 0; n < MAX_DARTS; n++, ptrd++) {
		ptrd->Flags |= DART_TAKEN;
	}

	_engine->_gameState->setDarts(MAX_DARTS);
}

void Dart::CheckDartCol(ActorStruct *ptrobj) {
	int32 n;
	T_DART *ptrd;
	int32 x0, y0, z0, x1, y1, z1;
	int32 xt0, yt0, zt0, xt1, yt1, zt1;

	if (ptrobj->_flags.bIsInvisible)
		return;

	x0 = ptrobj->_posObj.x + ptrobj->_boundingBox.mins.x;
	x1 = ptrobj->_posObj.x + ptrobj->_boundingBox.maxs.x;
	y0 = ptrobj->_posObj.y + ptrobj->_boundingBox.mins.y;
	y1 = ptrobj->_posObj.y + ptrobj->_boundingBox.maxs.y;
	z0 = ptrobj->_posObj.z + ptrobj->_boundingBox.mins.z;
	z1 = ptrobj->_posObj.z + ptrobj->_boundingBox.maxs.z;

	ptrd = ListDart;

	for (n = 0; n < MAX_DARTS; n++, ptrd++) {
		if (ptrd->NumCube == _engine->_scene->_numCube && !(ptrd->Flags & DART_TAKEN)) {
			xt0 = ptrd->PosX + ptrd->XMin;
			xt1 = ptrd->PosX + ptrd->XMax;
			yt0 = ptrd->PosY + ptrd->YMin;
			yt1 = ptrd->PosY + ptrd->YMax;
			zt0 = ptrd->PosZ + ptrd->ZMin;
			zt1 = ptrd->PosZ + ptrd->ZMax;

			if (x0 < xt1 && x1 > xt0 && y0 < yt1 && y1 > yt0 && z0 < zt1 && z1 > zt0) {
				ptrd->Flags |= DART_TAKEN;

				_engine->_gameState->addDart();

				const IVec3 dartPos(ptrd->PosX, ptrd->PosY, ptrd->PosZ);
				_engine->_sound->mixSample3D(SAMPLE_BONUS_TROUVE, 0x1000, 1, dartPos, -1);

				_engine->_redraw->addOverlay(OverlayType::koSprite, SPRITE_DART, 15, 30, 0, OverlayPosType::koNormal, 2, true);
			}
		}
	}
}

int32 Dart::throwDart(int32 x, int32 y, int32 z, int32 alpha, int32 beta, int32 speed, int32 weight) {
	const int32 extraIdx = _engine->_extra->throwExtraObj(OWN_ACTOR_SCENE_INDEX, x, y, z, BODY_3D_DART, alpha, beta, speed, -1, weight, DEGATS_DART);
	if (extraIdx != -1) {
		ExtraListStruct *extra = &_engine->_extra->_extraList[extraIdx];
		extra->type |= ExtraType::DART;
		if (_engine->_gameState->hasItem(InventoryItems::kiDart)) {
			_engine->_gameState->subtractDart();
		}
	}
	return extraIdx;
}

void Dart::placeDartFromExtra(const ExtraListStruct *extra, int32 oldX, int32 oldY, int32 oldZ) {
	const int32 dartIdx = GetDart();
	if (dartIdx == -1 || extra == nullptr) {
		return;
	}

	T_DART *ptrd = &ListDart[dartIdx];
	IVec3 pos = extra->pos;

	if (_engine->_grid->worldColBrick(oldX, extra->pos.y - 1, oldZ) != ShapeType::kNone) {
		const ShapeType col = _engine->_grid->worldColBrick(pos.x, pos.y - SIZE_BRICK_Y, pos.z);
		if (col != ShapeType::kNone && col != ShapeType::kSolid) {
			_engine->_collision->reajustPos(pos, col);
		}
	}

	ptrd->Flags &= ~DART_TAKEN;
	ptrd->NumCube = _engine->_scene->_numCube;
	ptrd->PosX = pos.x;
	ptrd->PosY = pos.y;
	ptrd->PosZ = pos.z;
	ptrd->Beta = extra->extraBeta;
	ptrd->Alpha = extra->extraAlpha;
}

void Dart::saveState(Common::WriteStream *stream) const {
	for (uint32 i = 0; i < MAX_DARTS; ++i) {
		const T_DART &dart = ListDart[i];
		stream->writeSint32LE(dart.PosX);
		stream->writeSint32LE(dart.PosY);
		stream->writeSint32LE(dart.PosZ);
		stream->writeSint32LE(dart.Alpha);
		stream->writeSint32LE(dart.Beta);
		stream->writeSint32LE(dart.NumCube);
		stream->writeUint32LE(dart.Flags);
	}
}

bool Dart::loadState(Common::SeekableReadStream *stream) {
	for (uint32 i = 0; i < MAX_DARTS; ++i) {
		T_DART &dart = ListDart[i];
		dart.PosX = stream->readSint32LE();
		dart.PosY = stream->readSint32LE();
		dart.PosZ = stream->readSint32LE();
		dart.Alpha = stream->readSint32LE();
		dart.Beta = stream->readSint32LE();
		dart.NumCube = stream->readSint32LE();
		dart.Flags = stream->readUint32LE();
	}
	return !stream->err();
}

} // namespace TwinE
