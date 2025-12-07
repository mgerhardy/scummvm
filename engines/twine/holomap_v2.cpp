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

#define STEP_ANGLE (32 * 4)
#define SLIG (((4096 / STEP_ANGLE) + 1) * 2)
#define MAX_BUF_HOLO_MAP 40000

#include "twine/holomap_v2.h"
#include "common/algorithm.h"
#include "twine/input.h"
#include "twine/renderer/renderer.h"
#include "twine/renderer/screens.h"
#include "twine/resources/hqr.h"
#include "twine/resources/resources.h"
#include "twine/twine.h"

namespace TwinE {

bool HolomapV2::setHoloPos(int32 locationIdx) {
	if (locationIdx >= MAX_OBJECTIF + MAX_CUBE) {
		return false;
	}

	_locations[locationIdx].flagHolo |= 1;    // Activ
	_locations[locationIdx].flagHolo &= 0xFD; // Not Asked

	if (locationIdx < MAX_OBJECTIF) {
		_numObjectif = locationIdx;
		return true;
	}
	return false;
}

bool HolomapV2::loadLocations() {
	// _locations[MAX_OBJECTIF + 67].FlagHolo = 1; //	Desert Globe
	if (HQR::getEntry((uint8 *)_locations, Resources::HQR_HOLOMAP_FILE, 12) == 0) { // HQR_ARROWBIN
		return false;
	}
	// Fix endianness if necessary? The struct has int32s.
	// Assuming Little Endian for now as LBA2 was PC. ScummVM handles endianness usually but raw reads might need swapping.
	// For now, let's assume it works or I'll fix it later.
	for (int i = 0; i < MAX_OBJECTIF + MAX_CUBE; ++i) {
		_locations[i].x = (int32)FROM_LE_32(_locations[i].x);
		_locations[i].y = (int32)FROM_LE_32(_locations[i].y);
		_locations[i].z = (int32)FROM_LE_32(_locations[i].z);
		_locations[i].alpha = (int32)FROM_LE_32(_locations[i].alpha);
		_locations[i].beta = (int32)FROM_LE_32(_locations[i].beta);
		_locations[i].alt = (int32)FROM_LE_32(_locations[i].alt);
		_locations[i].mess = (int32)FROM_LE_32(_locations[i].mess);
	}
	return true;
}

const char *HolomapV2::getLocationName(int index) const {
	if (index >= 0 && index < ARRAYSIZE(_locations)) {
		// TODO: return _locations[index].;
	}
	return "";
}

void HolomapV2::clrHoloPos(int32 locationIdx) {
	if (locationIdx >= MAX_OBJECTIF + MAX_CUBE) {
		return;
	}

	_locations[locationIdx].flagHolo &= 0xFE; // UnActiv
}

void HolomapV2::holoTraj(int32 trajectoryIndex) {
	// TODO: Implement trajectory rendering
}

void HolomapV2::initHoloDatas() {
	// TODO: Load resources
}

void HolomapV2::holoMap() {
	initHoloDatas();

	// TODO: Setup initial state

	while (!_engine->shouldQuit()) {
		FrameMarker frame(_engine);
		_engine->_input->readKeys();
		if (_engine->_input->toggleAbortAction()) {
			break;
		}

		// TODO: Handle input (arrows, enter, etc.)

		drawHolomap();
	}
}

void HolomapV2::drawHolomap() {
	if (_holoMode == 0) {
		holoSpace();
	} else {
		holoPlan(_zoomedIsland);
	}
}

void HolomapV2::holoSpace() {
	computeCoorGlobe();

	int32 numPoints = 0;
	uint16 *ptrc = (uint16 *)_ptrCoorGlobe;
	const uint8 *ptrUV = _ptrMapping;
	int32 cols = (2048 / STEP_ANGLE) + 1;

	for (int i = 0; i < SLIG; i++) {
		for (int j = 0; j < cols; j++) {
			int16 x = (int16)*ptrc++;
			int16 y = (int16)*ptrc++;
			int16 z = (int16)*ptrc++;

			IVec3 proj = _engine->_renderer->projectPoint(IVec3(x, y, z));
			_projectedPoints[numPoints].x = (int16)proj.x;
			_projectedPoints[numPoints].y = (int16)proj.y;
			_projectedPoints[numPoints].z = z;
			_projectedPoints[numPoints].u = (int16)*ptrUV++;
			_projectedPoints[numPoints].v = (int16)*ptrUV++;
			numPoints++;
		}
	}

	int32 numPolys = 0;
	for (int i = 0; i < SLIG - 1; i++) {
		for (int j = 0; j < cols - 1; j++) {
			int p1 = i * cols + j;
			int p2 = i * cols + j + 1;
			int p3 = (i + 1) * cols + j + 1;
			int p4 = (i + 1) * cols + j;

			_polygons[numPolys].p1 = (int16)p1;
			_polygons[numPolys].p2 = (int16)p2;
			_polygons[numPolys].p3 = (int16)p3;
			_polygons[numPolys].p4 = (int16)p4;
			_polygons[numPolys].z = (int16)((_projectedPoints[p1].z + _projectedPoints[p2].z + _projectedPoints[p3].z + _projectedPoints[p4].z) / 4);
			numPolys++;
		}
	}

	Common::sort(_polygons, _polygons + numPolys, [](const HolomapPoly &a, const HolomapPoly &b) {
		return a.z < b.z;
	});

	for (int i = 0; i < numPolys; i++) {
		const HolomapPoly &poly = _polygons[i];
		ComputedVertex v[4];
		ComputedVertex t[4];

		v[0].x = _projectedPoints[poly.p1].x;
		v[0].y = _projectedPoints[poly.p1].y;
		v[1].x = _projectedPoints[poly.p2].x;
		v[1].y = _projectedPoints[poly.p2].y;
		v[2].x = _projectedPoints[poly.p3].x;
		v[2].y = _projectedPoints[poly.p3].y;
		v[3].x = _projectedPoints[poly.p4].x;
		v[3].y = _projectedPoints[poly.p4].y;

		t[0].x = _projectedPoints[poly.p1].u;
		t[0].y = _projectedPoints[poly.p1].v;
		t[1].x = _projectedPoints[poly.p2].u;
		t[1].y = _projectedPoints[poly.p2].v;
		t[2].x = _projectedPoints[poly.p3].u;
		t[2].y = _projectedPoints[poly.p3].v;
		t[3].x = _projectedPoints[poly.p4].u;
		t[3].y = _projectedPoints[poly.p4].v;

		ComputedVertex v1[3] = {v[0], v[1], v[2]};
		ComputedVertex t1[3] = {t[0], t[1], t[2]};
		_engine->_renderer->asmTexturedTriangleNoClip(v1, t1, _tabPlanet[_destination].ptrTexture, 65536);

		ComputedVertex v2[3] = {v[0], v[2], v[3]};
		ComputedVertex t2[3] = {t[0], t[2], t[3]};
		_engine->_renderer->asmTexturedTriangleNoClip(v2, t2, _tabPlanet[_destination].ptrTexture, 65536);
	}
}

void HolomapV2::holoPlan(int32 numplan) {
	// TODO: Render Plan
}

void HolomapV2::computeCoorGlobe() {
	const uint8 *ptrv = _tabPlanet[_destination].ptrHeightMap;
	uint16 *ptrc = (uint16 *)_ptrCoorGlobe;

	for (int i = 0; i < SLIG; i++) {
		int32 alpha = (i * STEP_ANGLE) - 1024;

		for (int j = 0; j < ((2048 / STEP_ANGLE) + 1); j++) {
			int32 beta = j * STEP_ANGLE;

			int32 val = *ptrv++;
			int32 normal = val * 2 + 1000;

			IVec2 res = _engine->_renderer->rotate(normal, 0, alpha);
			int32 nx = res.x;
			int32 ny = res.y;

			res = _engine->_renderer->rotate(nx, 0, beta);
			int32 x = res.x;
			int32 z = res.y;
			int32 y = ny;

			IVec3 worldPos(x, y, z);
			IVec3 finalPos = _engine->_renderer->worldRotatePoint(worldPos);

			*ptrc++ = (uint16)finalPos.x;
			*ptrc++ = (uint16)finalPos.y;
			*ptrc++ = (uint16)finalPos.z;
		}
	}
}

HolomapV2::~HolomapV2() {
	if (_ptrMapping)
		free(_ptrMapping);
	if (_ptrCoorGlobe)
		free(_ptrCoorGlobe);
	if (_projectedPoints)
		free(_projectedPoints);
	if (_polygons)
		free(_polygons);
	if (_bufFleche)
		free(_bufFleche);
	if (_bufLoFleche)
		free(_bufLoFleche);
	if (_bufBuggy)
		free(_bufBuggy);
	if (_bufDyno)
		free(_bufDyno);
	for (int i = 0; i < MAX_PLANET; i++) {
		if (_tabPlanet[i].ptrHeightMap)
			free(_tabPlanet[i].ptrHeightMap);
		if (_tabPlanet[i].ptrTexture)
			free(_tabPlanet[i].ptrTexture);
	}
}

void HolomapV2::initHoloMalloc() {
	_ptrCoorGlobe = (uint16 *)malloc(MAX_BUF_HOLO_MAP);
	_projectedPoints = (HolomapProjectedPos *)malloc(sizeof(HolomapProjectedPos) * 2000);
	_polygons = (HolomapPoly *)malloc(sizeof(HolomapPoly) * 2000);

	HQR::getAllocEntry(&_ptrMapping, Resources::HQR_HOLOMAP_FILE, 0); // HQR_COORMAPP_HMM

	// Twinsun
	HQR::getAllocEntry(&_tabPlanet[0].ptrHeightMap, Resources::HQR_HOLOMAP_FILE, 1); // HQR_TWINSUN_HMT
	HQR::getAllocEntry(&_tabPlanet[0].ptrTexture, Resources::HQR_HOLOMAP_FILE, 2);   // HQR_TWINSUN_HMG
	_tabPlanet[0].rayon = 1000;
	_tabPlanet[0].zoom = 8000;

	// Moon
	HQR::getAllocEntry(&_tabPlanet[1].ptrHeightMap, Resources::HQR_HOLOMAP_FILE, 3); // HQR_MOON_HMT
	HQR::getAllocEntry(&_tabPlanet[1].ptrTexture, Resources::HQR_HOLOMAP_FILE, 4);   // HQR_MOON_HMG
	_tabPlanet[1].rayon = 250;
	_tabPlanet[1].zoom = 2000;

	// Zeelich
	HQR::getAllocEntry(&_tabPlanet[2].ptrHeightMap, Resources::HQR_HOLOMAP_FILE, 5); // HQR_ZEELICH_HMT
	HQR::getAllocEntry(&_tabPlanet[2].ptrTexture, Resources::HQR_HOLOMAP_FILE, 6);   // HQR_ZEELICH_HMG
	_tabPlanet[2].rayon = 500;
	_tabPlanet[2].zoom = 6000;

	// Sous-Gaz (Undergas?) - Planet 3?
	HQR::getAllocEntry(&_tabPlanet[3].ptrHeightMap, Resources::HQR_HOLOMAP_FILE, 7);
	HQR::getAllocEntry(&_tabPlanet[3].ptrTexture, Resources::HQR_HOLOMAP_FILE, 8);
	_tabPlanet[3].rayon = 500;
	_tabPlanet[3].zoom = 2000; // ZOOM_SUN?

	HQR::getAllocEntry(&_bufFleche, Resources::HQR_HOLOMAP_FILE, 10);   // HQR_FLECHE
	HQR::getAllocEntry(&_bufLoFleche, Resources::HQR_HOLOMAP_FILE, 11); // HQR_LOFLECHE
	// HQR_ARROWBIN (12) is loaded in loadLocations
	HQR::getAllocEntry(&_bufBuggy, Resources::HQR_HOLOMAP_FILE, 13); // HQR_BUGGY
	HQR::getAllocEntry(&_bufDyno, Resources::HQR_HOLOMAP_FILE, 14);  // HQR_DYNO
}

} // namespace TwinE
