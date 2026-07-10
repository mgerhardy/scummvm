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

#include "twine/holoplan_v2.h"
#include "common/algorithm.h"
#include "common/array.h"
#include "twine/audio/sound.h"
#include "twine/menu/interface.h"
#include "twine/parser/body.h"
#include "twine/parser/entity.h"
#include "twine/renderer/shadeangletab.h"
#include "twine/renderer/renderer.h"
#include "twine/renderer/screens.h"
#include "twine/resources/hqr.h"
#include "twine/resources/resources.h"
#include "twine/scene/actor.h"
#include "twine/scene/gamestate.h"
#include "twine/scene/scene.h"
#include "twine/shared.h"
#include "twine/text.h"
#include "twine/twine.h"

namespace TwinE {

#define HQR_HOLOMAP_BEGIN_MAP 18
#define HQR_HOLOMAP_FLECHE 10
#define HQR_HOLOMAP_LOFLECHE 11

static const int32 kBodyFactorScale[] = {
	2048, 2048, 2048, 1024, 2048, 768,
	1024, 1024, 1024, 768, 768, 1024
};

static const int32 kArrowFactorScale[] = {
	256, 256, 256, 128, 256, 96,
	128, 128, 128, 96, 96, 128
};

HoloPlanV2::HoloPlanV2(TwinEEngine *engine, HolomapV2 *holomap) : _engine(engine), _holomap(holomap) {
}

HoloPlanV2::~HoloPlanV2() {
	free(_mapImage);
	free(_mapData);
}

int32 HoloPlanV2::scaleBody(int32 island) const {
	if (island >= 0 && island < (int)ARRAYSIZE(kBodyFactorScale)) {
		return kBodyFactorScale[island];
	}
	return HOLOPLAN_ZOOM_A;
}

void HoloPlanV2::initPlan(int32 island) {
	free(_mapImage);
	free(_mapData);
	_mapImage = _mapData = nullptr;

	int32 mapIndex = HQR_HOLOMAP_BEGIN_MAP + 2 * island;
	const bool tempeteFinie = _engine->_gameState->hasGameFlag(253) >= 2;
	const bool celebration = _engine->_gameState->hasGameFlag(79);
	if (island == 0 && tempeteFinie) {
		mapIndex = HQR_HOLOMAP_BEGIN_MAP + 2 * 12;
	} else if (island == 5 && celebration) {
		mapIndex = HQR_HOLOMAP_BEGIN_MAP + 2 * 13;
	}

	_mapImageSize = HQR::getAllocEntry(&_mapImage, Resources::HQR_HOLOMAP_FILE, mapIndex);
	_mapData = (uint8 *)malloc(32);
	if (_mapData) {
		HQR::getEntry(_mapData, Resources::HQR_HOLOMAP_FILE, mapIndex + 1);
	}

	_bigArrowBody.loadFromHQR(Resources::HQR_HOLOMAP_FILE, HQR_HOLOMAP_FLECHE, false);
	_smallArrowBody.loadFromHQR(Resources::HQR_HOLOMAP_FILE, HQR_HOLOMAP_LOFLECHE, false);

	const ActorStruct *hero = _engine->_scene->getActor(OWN_ACTOR_SCENE_INDEX);
	if (hero && hero->_entityDataPtr) {
		_twinsenBody = hero->_entityDataPtr->getBody(hero->_body);
	}
}

void HoloPlanV2::drawSortedEntry(const HoloPlanSortEntry &entry) {
	Common::Rect dummy;
	switch (entry.type) {
	case 0: {
		const ActorStruct *hero = _engine->_scene->getActor(OWN_ACTOR_SCENE_INDEX);
		if (hero) {
			_engine->_renderer->affObjetIso(entry.x, entry.y, entry.zWorld, 0, hero->_beta, 0, _twinsenBody, dummy);
		}
		break;
	}
	case 1:
	case 2: {
		const HolomapV2::Location &loc = _holomap->getLocation(entry.index);
		const BodyData &arrowBody = (entry.type == 1) ? _bigArrowBody : _smallArrowBody;
		_engine->_renderer->affObjetIso(loc.X, entry.y, loc.Z, 0, 0, 0, arrowBody, dummy);
		break;
	}
	default:
		break;
	}
}

void HoloPlanV2::initObjectifIsland() {
	_numObjectif = -1;
	_sizeRet = 16;

	const int32 currentIsland = _engine->_scene->_island;
	if (currentIsland != _zoomedIsland) {
		for (int i = HOLO_MAX_OBJECTIF; i < HOLO_MAX_OBJECTIF + HOLO_MAX_CUBE; ++i) {
			const HolomapV2::Location &loc = _holomap->getLocation(i);
			if ((loc.FlagHolo & 1) && loc.Island == (uint8)_zoomedIsland) {
				_numObjectif = i;
				break;
			}
		}
	} else {
		_numObjectif = HOLO_MAX_OBJECTIF + _engine->_scene->_numCube;
	}

	if (_numObjectif >= 0 && _numObjectif < HOLO_MAX_ARROW) {
		const HolomapV2::Location &loc = _holomap->getLocation(_numObjectif);
		const IVec3 &world = _engine->_renderer->longWorldRot(loc.X, loc.Y, loc.Z);
		IVec3 proj;
		if (_engine->_renderer->longProjectPoint(world, proj)) {
			_xpObjectif = proj.x;
			_ypObjectif = proj.y;
		}
	}
}

void HoloPlanV2::drawReticule() {
	const int32 cx = _xpObjectif;
	const int32 cy = _ypObjectif;
	const int32 sz = _sizeRet;
	_engine->_menu->drawRectBorders(cx - sz, cy - sz, cx + sz, cy + sz, 15, 15);
	if (_sizeRet > 6) {
		_sizeRet -= 2;
	}
}

void HoloPlanV2::drawListHoloPlan() {
	if (_mapImage == nullptr) {
		return;
	}

	const Common::Rect rect(0, 0, _engine->width() - 1, _engine->height() - 1);
	_engine->_interface->box(rect, COLOR_BLACK);

	const int32 imageX0 = (_engine->width() - 640) / 2;
	const int32 imageY0 = (_engine->height() - 480) / 2;
	const int32 imageX1 = imageX0 + 639;
	const int32 imageY1 = imageY0 + 479;

	for (int32 y = 0; y < 480; ++y) {
		const int32 dstY = imageY0 + y;
		if (dstY < 0 || dstY >= _engine->height()) {
			continue;
		}
		uint8 *dst = (uint8 *)_engine->_workVideoBuffer.getBasePtr(imageX0, dstY);
		const uint8 *src = _mapImage + y * 640;
		const int32 copyW = MIN(640, _engine->width() - imageX0);
		if (copyW > 0) {
			memcpy(dst, src, copyW);
		}
	}

	_engine->_menu->drawRectBorders(imageX0, imageY0, imageX1, imageY1, 15, 15);
	_engine->_menu->drawRectBorders(imageX0, imageY1, imageX1, imageY1, 15, 15);
	_engine->_menu->drawRectBorders(imageX0, imageY0, imageX0, imageY1, 15, 15);
	_engine->_menu->drawRectBorders(imageX1, imageY0, imageX1, imageY1, 15, 15);

	Common::Array<HoloPlanSortEntry> entries;

	for (int i = HOLO_MAX_OBJECTIF; i < HOLO_MAX_ARROW; ++i) {
		const HolomapV2::Location &loc = _holomap->getLocation(i);
		if (!(loc.FlagHolo & 1) || loc.Island != (uint8)_zoomedIsland) {
			continue;
		}

		int32 alt = loc.Y + 2048;
		if (i == _numObjectif) {
			alt += trigSin((_engine->timerRef * 2) & (LBAAngles::ANGLE_360 - 1), _engine->isLBA2()) >> 3;
		} else {
			alt += trigSin((_engine->timerRef * 4) & (LBAAngles::ANGLE_360 - 1), _engine->isLBA2()) >> 4;
		}

		const IVec3 &world = _engine->_renderer->longWorldRot(loc.X, alt, loc.Z);
		IVec3 proj;
		if (!_engine->_renderer->longProjectPoint(world, proj)) {
			continue;
		}

		HoloPlanSortEntry entry;
		entry.z = world.z;
		entry.x = loc.X;
		entry.y = alt;
		entry.zWorld = loc.Z;
		entry.type = (i == _numObjectif) ? 1 : 2;
		entry.index = i;
		entries.push_back(entry);
	}

	if (_zoomedIsland == _engine->_scene->_island) {
		ActorStruct *hero = _engine->_scene->getActor(OWN_ACTOR_SCENE_INDEX);
		int32 xt = hero->_posObj.x;
		int32 yt = hero->_posObj.y;
		int32 zt = hero->_posObj.z;
		if (_engine->_scene->_isOutsideScene) {
			xt += _engine->_scene->_currentCubeX * HOLOPLAN_SCE;
			zt += _engine->_scene->_currentCubeY * HOLOPLAN_SCE;
		} else {
			const HolomapV2::Location &loc = _holomap->getLocation(HOLO_MAX_OBJECTIF + _engine->_scene->_numCube);
			xt = loc.X;
			yt = loc.Y;
			zt = loc.Z;
		}

		const IVec3 &world = _engine->_renderer->longWorldRot(xt, yt, zt);
		IVec3 proj;
		if (_engine->_renderer->longProjectPoint(world, proj)) {
			HoloPlanSortEntry entry;
			entry.z = world.z;
			entry.x = xt;
			entry.y = yt;
			entry.zWorld = zt;
			entry.type = 0;
			entry.index = -1;
			entries.push_back(entry);
		}
	}

	Common::sort(entries.begin(), entries.end(), [](const HoloPlanSortEntry &a, const HoloPlanSortEntry &b) {
		return a.z < b.z;
	});

	for (uint i = 0; i < entries.size(); ++i) {
		drawSortedEntry(entries[i]);
	}
}

void HoloPlanV2::searchObjectifIsland(int32 dir) {
	int32 best = 9999;
	int32 next = -1;

	for (int i = HOLO_MAX_OBJECTIF; i < HOLO_MAX_OBJECTIF + HOLO_MAX_CUBE; ++i) {
		const HolomapV2::Location &loc = _holomap->getLocation(i);
		if (!(loc.FlagHolo & 1) || i == _numObjectif || loc.Island != (uint8)_zoomedIsland) {
			continue;
		}

		const IVec3 &world = _engine->_renderer->longWorldRot(loc.X, loc.Y, loc.Z);
		IVec3 proj;
		if (!_engine->_renderer->longProjectPoint(world, proj)) {
			continue;
		}

		if ((dir & 1) && proj.x > _xpObjectif) continue;
		if ((dir & 2) && proj.y > _ypObjectif) continue;
		if ((dir & 4) && proj.x < _xpObjectif) continue;
		if ((dir & 8) && proj.y < _ypObjectif) continue;

		const int32 dd = ABS(proj.x - _xpObjectif) + ABS(proj.y - _ypObjectif);
		if (dd < best) {
			best = dd;
			next = i;
		}
	}

	if (next != -1) {
		_numObjectif = next;
		const HolomapV2::Location &loc = _holomap->getLocation(next);
		const IVec3 &world = _engine->_renderer->longWorldRot(loc.X, loc.Y, loc.Z);
		IVec3 proj;
		if (_engine->_renderer->longProjectPoint(world, proj)) {
			_xpObjectif = proj.x;
			_ypObjectif = proj.y;
		}
		_sizeRet = 16;
	}
}

void HoloPlanV2::holoPlan(int32 island) {
	_zoomedIsland = island;
	_flagHoloEnd = false;

	_engine->saveTimer(false);
	_engine->_interface->unsetClip();

	initPlan(island);

	int32 orgMx = 0;
	int32 orgMz = 0;
	int32 alpha = 0;
	int32 beta = 0;
	int32 distance = 6000;
	if (_mapData) {
		const uint8 *pt = _mapData;
		orgMx = pt[0] * HOLOPLAN_SCE + pt[1];
		orgMz = pt[2] * HOLOPLAN_SCE + pt[3];
		alpha = pt[4] * 16;
		beta = pt[5] * 16;
		distance = *(const int16 *)(pt + 6);
	}

	const int32 cameraPosX = _engine->width() / 2;
	const int32 cameraPosY = _engine->height() * 220 / 480;
	_engine->_renderer->setProjection(cameraPosX, cameraPosY, 1024, 700, 700);
	_engine->_renderer->setFollowCamera(orgMx, 0, orgMz, alpha, beta, 0, distance);
	_engine->_renderer->setLightVector(alpha, beta, 0);

	initObjectifIsland();
	_engine->_input->enableKeyMap(holomapKeyMapId);

	for (;;) {
		FrameMarker frame(_engine);
		_engine->readKeys();
		if (_engine->shouldQuit() || _engine->_input->toggleAbortAction() || _flagHoloEnd) {
			break;
		}

		if (_engine->_input->toggleActionIfActive(TwinEActionType::UIEnter)) {
			break;
		}

		if (_engine->_input->isActionActive(TwinEActionType::HolomapLeft)) {
			searchObjectifIsland(1);
		} else if (_engine->_input->isActionActive(TwinEActionType::HolomapRight)) {
			searchObjectifIsland(4);
		} else if (_engine->_input->isActionActive(TwinEActionType::HolomapUp)) {
			searchObjectifIsland(2);
		} else if (_engine->_input->isActionActive(TwinEActionType::HolomapDown)) {
			searchObjectifIsland(8);
		}

		drawListHoloPlan();
		drawReticule();
		_engine->copyBlockPhys(_engine->rect());
	}

	_engine->_input->enableKeyMap(mainKeyMapId);
	_engine->restoreTimer();
}

} // namespace TwinE
