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

#include "twine/scene/exterior.h"
#include "twine/parser/body.h"
#include "twine/renderer/renderer.h"
#include "twine/renderer/screens.h"
#include "twine/resources/hqr.h"
#include "twine/resources/resources.h"
#include "twine/scene/gamestate.h"
#include "twine/scene/scene.h"
#include "twine/scene/scene.h"
#include "twine/twine.h"

namespace TwinE {

static const char *const kIslandNames[] = {
	"citadel", "sendell", "desert", "emeraude", "otringal", "celebrat",
	"platform", "mosquibe", "knartas", "ilotcx", "ascence", "souscelb"
};

static const int kSkySeaIndices[] = {
	RESSHQR_SKYSEA0, RESSHQR_SKYSEA1, RESSHQR_SKYSEA2, RESSHQR_SKYSEA3,
	RESSHQR_SKYSEA4, RESSHQR_SKYSEA5, RESSHQR_SKYSEA6, RESSHQR_SKYSEA7,
	RESSHQR_SKYSEA8, RESSHQR_SKYSEA9, RESSHQR_SKYSEA10, RESSHQR_SKYSEA5
};

Exterior::Exterior(TwinEEngine *engine) : _engine(engine) {
}

Exterior::~Exterior() {
	free(_islandMap);
	free(_groundTexture);
	free(_objTexture);
	free(_skySeaTexture);
	free(_cubeInfos);
	free(_decorList);
	free(_mapPolyGround);
	free(_listTexDef);
	free(_mapSommetY);
	free(_mapIntensity);
}

bool Exterior::isActive() const {
	return _engine->isLBA2() && _engine->_scene->_isOutsideScene;
}

void Exterior::init() {
	_lastIsland = -1;
	_islandName[0] = '\0';
}

void Exterior::resolveIslandName(int32 island) {
	const bool tempeteFinie = _engine->_gameState->hasGameFlag(253) >= 2;
	const bool celebration = _engine->_gameState->hasGameFlag(79);

	if (island == 0) {
		Common::strlcpy(_islandName, tempeteFinie ? "citabau" : "citadel", sizeof(_islandName));
	} else if (island == 5 && celebration) {
		Common::strlcpy(_islandName, "celebra2", sizeof(_islandName));
	} else if (island >= 0 && island < (int)ARRAYSIZE(kIslandNames)) {
		Common::strlcpy(_islandName, kIslandNames[island], sizeof(_islandName));
	} else {
		Common::strlcpy(_islandName, "citadel", sizeof(_islandName));
	}
}

bool Exterior::loadIslandFile(const char *baseName) {
	free(_islandMap);
	free(_groundTexture);
	free(_objTexture);
	_islandMap = _groundTexture = _objTexture = nullptr;

	char filename[64];
	Common::sprintf_s(filename, "%s.ile", baseName);

	_islandMap = (uint8 *)malloc(EXT_SIZE_MAIN_MAP * EXT_SIZE_MAIN_MAP);
	_groundTexture = (uint8 *)malloc(256 * 256);
	_objTexture = (uint8 *)malloc(256 * 256);
	if (!_islandMap || !_groundTexture || !_objTexture) {
		return false;
	}

	if (HQR::getEntry(_islandMap, filename, 0) <= 0 ||
	    HQR::getEntry(_groundTexture, filename, 1) <= 0 ||
	    HQR::getEntry(_objTexture, filename, 2) <= 0) {
		warning("Exterior::loadIslandFile(): failed to load %s", filename);
		return false;
	}

	return true;
}

void Exterior::loadSkySea(int32 island) {
	free(_skySeaTexture);
	_skySeaTexture = (uint8 *)malloc(256 * 256);
	if (_skySeaTexture == nullptr) {
		return;
	}

	int32 index = RESSHQR_SKYSEA0;
	if (island == 0 && _engine->_gameState->hasGameFlag(253) >= 2) {
		index = RESSHQR_SKYSEA00;
	} else if (island >= 0 && island < (int)ARRAYSIZE(kSkySeaIndices)) {
		index = kSkySeaIndices[island];
	}

	if (HQR::getEntry(_skySeaTexture, Resources::HQR_RESS_FILE, index) <= 0) {
		memset(_skySeaTexture, 0, 256 * 256);
	}
}

int32 Exterior::getCubeHqrIndex(int32 cubeX, int32 cubeY) const {
	if (_islandMap == nullptr) {
		return -1;
	}
	const int32 mapIndex = cubeY * EXT_SIZE_MAIN_MAP + cubeX;
	const uint8 cubeId = _islandMap[mapIndex] & 127;
	if (!cubeId) {
		return -1;
	}
	return EXT_HQR_START_CUBE + EXT_HQR_STEP_CUBE * (cubeId - 1);
}

bool Exterior::loadCubeData(int32 cubeX, int32 cubeY) {
	const int32 index = getCubeHqrIndex(cubeX, cubeY);
	if (index < 0) {
		_cubeBitField = 0xFFFF;
		return false;
	}

	char filename[64];
	Common::sprintf_s(filename, "%s.ile", _islandName);

	free(_cubeInfos);
	free(_decorList);
	free(_mapPolyGround);
	free(_listTexDef);
	free(_mapSommetY);
	free(_mapIntensity);
	_cubeInfos = nullptr;
	_decorList = nullptr;
	_mapPolyGround = nullptr;
	_listTexDef = nullptr;
	_mapSommetY = nullptr;
	_mapIntensity = nullptr;

	uint8 *cubeInfosPtr = nullptr;
	uint8 *mapPolyGroundPtr = nullptr;
	uint8 *listTexDefPtr = nullptr;
	uint8 *mapSommetYPtr = nullptr;
	uint8 *mapIntensityPtr = nullptr;

	if (HQR::getAllocEntry(&cubeInfosPtr, filename, index + 0) <= 0 ||
	    HQR::getAllocEntry(&mapPolyGroundPtr, filename, index + 2) <= 0 ||
	    HQR::getAllocEntry(&mapSommetYPtr, filename, index + 4) <= 0) {
		free(cubeInfosPtr);
		free(mapPolyGroundPtr);
		free(mapSommetYPtr);
		warning("Exterior::loadCubeData(%i,%i): failed to load cube data", cubeX, cubeY);
		return false;
	}

	_cubeInfos = (int32 *)cubeInfosPtr;
	_mapPolyGround = (ExteriorHalfPoly *)mapPolyGroundPtr;
	_mapSommetY = (int16 *)mapSommetYPtr;

	if (HQR::getAllocEntry(&listTexDefPtr, filename, index + 3) > 0) {
		_listTexDef = (ExteriorHalfTex *)listTexDefPtr;
	}
	if (HQR::getAllocEntry(&mapIntensityPtr, filename, index + 5) > 0) {
		_mapIntensity = mapIntensityPtr;
	}

	if (_cubeInfos == nullptr || _mapPolyGround == nullptr || _mapSommetY == nullptr) {
		warning("Exterior::loadCubeData(%i,%i): failed to load cube data", cubeX, cubeY);
		return false;
	}

	const int32 nbDecors = _cubeInfos[EXT_INFO_NB_DECORS];
	if (nbDecors != 0) {
		uint8 *decorPtr = nullptr;
		if (HQR::getAllocEntry(&decorPtr, filename, index + 1) > 0) {
			_decorList = decorPtr;
		}
	}

	_alphaLight = _cubeInfos[EXT_INFO_ALPHA_LIGHT] & 0xFFFF;
	_cubeBitField = (uint16)(_cubeInfos[EXT_INFO_ALPHA_LIGHT] >> 16);
	_betaLight = _cubeInfos[EXT_INFO_BETA_LIGHT];
	_skyY = _cubeInfos[EXT_INFO_SKY_Y];
	_clipZFar = _cubeInfos[EXT_INFO_CLIP_Z_FAR];
	_startZFog = _cubeInfos[EXT_INFO_START_Z_FOG];

	if (!_clipZFar) {
		_clipZFar = 48000;
	}
	if (!_startZFog) {
		_startZFog = 12000;
	}

	_engine->_scene->_alphaLight = (int16)_alphaLight;
	_engine->_scene->_betaLight = (int16)_betaLight;

	parseDecorList();
	return true;
}

void Exterior::parseDecorList() {
	_decors.clear();
	if (_decorList == nullptr || _cubeInfos == nullptr) {
		return;
	}

	const int32 nbDecors = _cubeInfos[EXT_INFO_NB_DECORS];
	const uint8 *ptr = _decorList;
	_decors.reserve(nbDecors);
	for (int32 i = 0; i < nbDecors; ++i) {
		ExteriorDecor decor;
		decor.bodyField = (int32)READ_LE_UINT32(ptr); ptr += 4;
		decor.x = (int32)READ_LE_UINT32(ptr); ptr += 4;
		decor.y = (int32)READ_LE_UINT32(ptr); ptr += 4;
		decor.z = (int32)READ_LE_UINT32(ptr); ptr += 4;
		decor.codeJeu = (int32)READ_LE_UINT32(ptr); ptr += 4;
		decor.beta = (int32)READ_LE_UINT32(ptr); ptr += 4;
		decor.xMin = (int32)READ_LE_UINT32(ptr); ptr += 4;
		decor.yMin = (int32)READ_LE_UINT32(ptr); ptr += 4;
		decor.zMin = (int32)READ_LE_UINT32(ptr); ptr += 4;
		decor.xMax = (int32)READ_LE_UINT32(ptr); ptr += 4;
		decor.yMax = (int32)READ_LE_UINT32(ptr); ptr += 4;
		decor.zMax = (int32)READ_LE_UINT32(ptr); ptr += 4;
		_decors.push_back(decor);
	}
}

bool Exterior::isDecorVisible(const ExteriorDecor &decor) const {
	if (decor.bodyField & DEC_INVISIBLE) {
		return false;
	}
	const int32 varIdx = decor.beta >> 16;
	if (varIdx == 0) {
		return true;
	}
	const int32 absIdx = ABS(varIdx);
	if (absIdx >= NUM_GAME_FLAGS) {
		return true;
	}
	const int32 varValue = _engine->_gameState->hasGameFlag((uint8)absIdx);
	if (varIdx < 0) {
		return varValue == 0;
	}
	return varValue != 0;
}

const BodyData *Exterior::getDecorBody(int32 bodyIndex) {
	if (bodyIndex < 0) {
		return nullptr;
	}
	if (!_decorBodies.contains(bodyIndex)) {
		BodyData body;
		char filename[64];
		Common::sprintf_s(filename, "%s.ile", _islandName);
		if (!body.loadFromHQR(filename, bodyIndex, false)) {
			warning("Exterior::getDecorBody(): failed to load body %i from %s", bodyIndex, filename);
			return nullptr;
		}
		_decorBodies.setVal(bodyIndex, body);
	}
	return &_decorBodies.getVal(bodyIndex);
}

void Exterior::drawDecors() {
	if (_decors.empty()) {
		return;
	}

	struct SortEntry {
		int32 z;
		int32 index;
	};
	SortEntry sortList[200];
	int32 nbSort = 0;

	for (uint i = 0; i < _decors.size() && nbSort < 200; ++i) {
		const ExteriorDecor &decor = _decors[i];
		if (!isDecorVisible(decor)) {
			continue;
		}

		const IVec3 &world = _engine->_renderer->longWorldRot(decor.x, decor.y, decor.z);
		const IVec3 &cameraRot = _engine->_renderer->getCameraRotation();
		const int32 zr = cameraRot.z - world.z;
		if (zr <= 128 || zr >= _clipZFar) {
			continue;
		}

		IVec3 proj;
		if (!_engine->_renderer->longProjectPoint(world, proj)) {
			continue;
		}

		sortList[nbSort].z = zr;
		sortList[nbSort].index = (int32)i;
		++nbSort;
	}

	if (nbSort == 0) {
		return;
	}

	Common::sort(sortList, sortList + nbSort,
		[](const SortEntry &a, const SortEntry &b) { return a.z < b.z; });

	const int32 clipZStart = _clipZFar - _startZFog;
	for (int32 i = 0; i < nbSort; ++i) {
		const ExteriorDecor &decor = _decors[sortList[i].index];
		const int32 bodyIndex = decor.bodyField & 0xFFFF;
		const BodyData *body = getDecorBody(bodyIndex);
		if (body == nullptr) {
			continue;
		}

		const uint8 fog = (uint8)boundRuleThree(0, 15, clipZStart, sortList[i].z - _startZFog);
		(void)fog;

		Common::Rect dummy;
		_engine->_renderer->affObjetIso(decor.x, decor.y, decor.z, 0, decor.beta & 0xFFFF, 0, *body, dummy);
	}
}

bool Exterior::testDecorCollision(int32 x, int32 y, int32 z) const {
	for (const ExteriorDecor &decor : _decors) {
		if (!isDecorVisible(decor)) {
			continue;
		}
		if (decor.xMin > x || decor.xMax < x) {
			continue;
		}
		if (decor.yMin > y || decor.yMax < y) {
			continue;
		}
		if (decor.zMin > z || decor.zMax < z) {
			continue;
		}
		return true;
	}
	return false;
}

uint8 Exterior::getTerrainCodeJeu(int32 x, int32 z) const {
	if (_mapPolyGround == nullptr) {
		return 0;
	}
	const int32 xi = (x >> 9) & (EXT_NB_COTE - 1);
	const int32 zi = (z >> 9) & (EXT_NB_COTE - 1);
	const int32 tri = getTriangleIndex(x, z);
	return (uint8)_mapPolyGround[(zi * EXT_NB_COTE + xi) * 2 + tri].codeJeu;
}

uint8 Exterior::getTerrainBrickCode(int32 x, int32 y, int32 z) const {
	if (_mapPolyGround == nullptr) {
		return 0xF0;
	}
	const int32 groundY = getAltitude(x, z, 0);
	if (groundY > y) {
		return 0xF0;
	}
	const int32 xi = (x >> 9) & (EXT_NB_COTE - 1);
	const int32 zi = (z >> 9) & (EXT_NB_COTE - 1);
	const int32 tri = getTriangleIndex(x, z);
	const ExteriorHalfPoly &poly = _mapPolyGround[(zi * EXT_NB_COTE + xi) * 2 + tri];
	return (uint8)((poly.codeJeu << 4) | poly.sampleStep);
}

void Exterior::initGrid() {
	const int32 island = _engine->_scene->_island;
	resolveIslandName(island);

	if (_lastIsland != island || strcmp(_loadedIslandName, _islandName) != 0) {
		loadSkySea(island);
		if (!loadIslandFile(_islandName)) {
			error("Exterior::initGrid(): failed to load island %s", _islandName);
		}
		_lastIsland = island;
		Common::strlcpy(_loadedIslandName, _islandName, sizeof(_loadedIslandName));
	}

	loadCubeData(_engine->_scene->_currentCubeX, _engine->_scene->_currentCubeY);

	_zBufferWidth = _engine->width();
	_zBufferHeight = _engine->height();
	_zBuffer.resize(_zBufferWidth * _zBufferHeight, 0xFFFF);
}

void Exterior::clearTerrainZBuf() {
	_engine->_screens->clearScreen();
	const uint16 fill = 0xFFFF;
	Common::fill(_zBuffer.begin(), _zBuffer.end(), fill);
}

void Exterior::projectTerrainVertices() {
	const IVec3 &cameraRot = _engine->_renderer->getCameraRotation();
	const int32 nearClip = 128;

	for (int z = 0; z <= EXT_NB_COTE; ++z) {
		for (int x = 0; x <= EXT_NB_COTE; ++x) {
			ExteriorVertex &v = _vertices[z * (EXT_NB_COTE + 1) + x];
			const int32 wx = x * 512;
			const int32 wz = z * 512;
			const int32 wy = _mapSommetY[z * (EXT_NB_COTE + 1) + x];

			const IVec3 &world = _engine->_renderer->longWorldRot(wx, wy, wz);
			const int32 zr = cameraRot.z - world.z;

			if (zr <= nearClip) {
				v.visible = 0;
				continue;
			}

			IVec3 proj;
			if (zr >= _clipZFar || !_engine->_renderer->longProjectPoint(world, proj)) {
				v.visible = 0;
				continue;
			}

			v.x2d = (int16)proj.x;
			v.y2d = (int16)proj.y;
			v.zrot = zr;
			v.visible = 16;
		}
	}
}

void Exterior::rasterizeTriangleZ(int32 x0, int32 y0, int32 z0, int32 x1, int32 y1, int32 z1, int32 x2, int32 y2, int32 z2) {
	const int32 w = _zBufferWidth;
	const int32 h = _zBufferHeight;

	int32 minY = MIN(MIN(y0, y1), y2);
	int32 maxY = MAX(MAX(y0, y1), y2);
	minY = MAX(minY, 0);
	maxY = MIN(maxY, h - 1);

	for (int32 y = minY; y <= maxY; ++y) {
		for (int32 x = 0; x < w; ++x) {
			const int32 denom = (y1 - y2) * (x0 - x2) + (x2 - x1) * (y0 - y2);
			if (denom == 0) {
				continue;
			}
			const int32 w0 = ((y1 - y2) * (x - x2) + (x2 - x1) * (y - y2)) * 65536 / denom;
			const int32 w1 = ((y2 - y0) * (x - x2) + (x0 - x2) * (y - y2)) * 65536 / denom;
			const int32 w2 = 65536 - w0 - w1;
			if (w0 < 0 || w1 < 0 || w2 < 0) {
				continue;
			}
			const int32 z = (z0 * w0 + z1 * w1 + z2 * w2) >> 16;
			const int32 px = (x0 * w0 + x1 * w1 + x2 * w2) >> 16;
			if (px < 0 || px >= w) {
				continue;
			}
			uint16 &zb = _zBuffer[y * w + px];
			if (z < zb) {
				zb = (uint16)MIN(z, 0xFFFF);
			}
		}
	}
}

void Exterior::drawTerrainTriangle(const ExteriorVertex &v0, const ExteriorVertex &v1, const ExteriorVertex &v2,
                                   const ExteriorHalfPoly &poly, const ExteriorHalfTex *texDef, uint8 fogColor) {
	if (!(v0.visible & v1.visible & v2.visible)) {
		return;
	}

	rasterizeTriangleZ(v0.x2d, v0.y2d, v0.zrot, v1.x2d, v1.y2d, v1.zrot, v2.x2d, v2.y2d, v2.zrot);

	if (!poly.polyFlag && !poly.texFlag) {
		return;
	}

	uint8 color = (uint8)((poly.bank << 4) + 11 + fogColor);
	if (poly.polyFlag == 1) {
		const int i0 = _mapIntensity[0] & 15;
		const int i1 = _mapIntensity[0] & 15;
		const int i2 = _mapIntensity[0] & 15;
		color = (uint8)(color + ((i0 + i1 + i2) * 21845) / (256 * 3));
	}

	ComputedVertex screen[3];
	screen[0].x = v0.x2d;
	screen[0].y = v0.y2d;
	screen[1].x = v1.x2d;
	screen[1].y = v1.y2d;
	screen[2].x = v2.x2d;
	screen[2].y = v2.y2d;

	if (poly.texFlag && texDef && _groundTexture) {
		ComputedVertex tex[3];
		tex[0].x = texDef->tx0;
		tex[0].y = texDef->ty0;
		tex[1].x = texDef->tx1;
		tex[1].y = texDef->ty1;
		tex[2].x = texDef->tx2;
		tex[2].y = texDef->ty2;
		_engine->_renderer->drawTexturedGroundTriangle(screen, tex, 4, _groundTexture, color, 0xFFFF);
	} else if (poly.polyFlag) {
		_engine->_workVideoBuffer.drawLine(v0.x2d, v0.y2d, v1.x2d, v1.y2d, color);
		_engine->_workVideoBuffer.drawLine(v1.x2d, v1.y2d, v2.x2d, v2.y2d, color);
		_engine->_workVideoBuffer.drawLine(v2.x2d, v2.y2d, v0.x2d, v0.y2d, color);
	}
}

void Exterior::drawTerrain() {
	if (_mapPolyGround == nullptr || _mapSommetY == nullptr) {
		return;
	}

	projectTerrainVertices();

	for (int z = 0; z < EXT_NB_COTE; ++z) {
		for (int x = 0; x < EXT_NB_COTE; ++x) {
			const ExteriorHalfPoly *poly = &_mapPolyGround[(z * EXT_NB_COTE + x) * 2];
			const ExteriorVertex &v00 = _vertices[z * (EXT_NB_COTE + 1) + x];
			const ExteriorVertex &v10 = _vertices[(z + 1) * (EXT_NB_COTE + 1) + x];
			const ExteriorVertex &v01 = _vertices[z * (EXT_NB_COTE + 1) + x + 1];
			const ExteriorVertex &v11 = _vertices[(z + 1) * (EXT_NB_COTE + 1) + x + 1];

			const uint8 fog = (uint8)boundRuleThree(0, 15, _clipZFar - _startZFog, v00.zrot - _startZFog);

			if (poly[0].polyFlag || poly[0].texFlag) {
				const ExteriorHalfTex *tex = poly[0].texFlag ? &_listTexDef[poly[0].indexTex] : nullptr;
				if (poly[0].sens == 0) {
					drawTerrainTriangle(v00, v10, v11, poly[0], tex, fog);
				} else {
					drawTerrainTriangle(v00, v01, v11, poly[0], tex, fog);
				}
			}

			if (poly[1].polyFlag || poly[1].texFlag) {
				const ExteriorHalfTex *tex = poly[1].texFlag ? &_listTexDef[poly[1].indexTex] : nullptr;
				if (poly[1].sens == 0) {
					drawTerrainTriangle(v11, v01, v00, poly[1], tex, fog);
				} else {
					drawTerrainTriangle(v10, v11, v00, poly[1], tex, fog);
				}
			}
		}
	}
}

void Exterior::drawSkySea() {
	if (_skySeaTexture == nullptr) {
		return;
	}

	const int32 w = _engine->width();
	const int32 h = _engine->height();
	const int32 horizon = boundRuleThree(h / 4, h * 3 / 4, _clipZFar, _skyY);

	for (int32 y = 0; y < horizon && y < h; ++y) {
		const int32 ty = (y * 255) / MAX(horizon, 1);
		uint8 *dst = (uint8 *)_engine->_workVideoBuffer.getBasePtr(0, y);
		const uint8 *src = _skySeaTexture + ty * 256;
		memcpy(dst, src, MIN(w, 256));
		if (w > 256) {
			memcpy(dst + 256, src, w - 256);
		}
	}
}

void Exterior::redraw() {
	if (!isActive()) {
		return;
	}

	_engine->_renderer->setLightVector(_alphaLight, _betaLight, 0);
	clearTerrainZBuf();
	drawSkySea();
	drawTerrain();
	drawDecors();
}

int32 Exterior::getTriangleIndex(int32 x, int32 z) const {
	const int32 xi = (x >> 9) & (EXT_NB_COTE - 1);
	const int32 zi = (z >> 9) & (EXT_NB_COTE - 1);
	const ExteriorHalfPoly &poly = _mapPolyGround[(zi * EXT_NB_COTE + xi) * 2];
	const int32 lx = x & 511;
	const int32 lz = z & 511;
	if (poly.sens == 0) {
		return (lz < lx) ? 0 : 1;
	}
	return (lz < (512 - lx)) ? 0 : 1;
}

int32 Exterior::getAltitude(int32 x, int32 z, int32 waterCode) const {
	if (_mapSommetY == nullptr || _mapPolyGround == nullptr) {
		return 0;
	}

	const int32 xi = x >> 9;
	const int32 zi = z >> 9;
	if (xi < 0 || zi < 0 || xi >= EXT_NB_COTE || zi >= EXT_NB_COTE) {
		return -1;
	}

	const int32 lx = x & 511;
	const int32 lz = z & 511;
	const int32 dz0 = zi * (EXT_NB_COTE + 1);
	const int32 dz1 = dz0 + (EXT_NB_COTE + 1);

	int32 cj = waterCode;
	if (cj == -1) {
		cj = getTerrainCodeJeu(x, z);
	}

	int32 y0 = _mapSommetY[dz0 + xi];
	int32 y1 = _mapSommetY[dz1 + xi];
	int32 y2 = _mapSommetY[dz1 + xi + 1];
	int32 y3 = _mapSommetY[dz0 + xi + 1];

	if ((cj == CJ_FOOT_WATER || cj == CJ_WATER) && _mapIntensity != nullptr) {
		const uint8 *i = &_mapIntensity[dz0 + xi];
		y0 += (i[0] >> 4) * -200;
		y1 += (i[EXT_NB_COTE + 1] >> 4) * -200;
		y2 += (i[EXT_NB_COTE + 2] >> 4) * -200;
		y3 += (i[1] >> 4) * -200;
	}

	const ExteriorHalfPoly &poly = _mapPolyGround[(zi * EXT_NB_COTE + xi) * 2];
	if (poly.sens == 0) {
		if (lx < lz) {
			return y0 + ((y1 - y0) * lz + (y2 - y1) * lx) / 512;
		}
		return y0 + ((y3 - y0) * lx + (y2 - y3) * lz) / 512;
	}
	if (511 - lx > lz) {
		return y0 + ((y3 - y0) * lx + (y1 - y0) * lz) / 512;
	}
	return y1 + ((y2 - y1) * lx + (y3 - y2) * (511 - lz)) / 512;
}

bool Exterior::giveTerrainCol(int32 x, int32 z) const {
	if (_mapPolyGround == nullptr) {
		return false;
	}
	const int32 xi = (x >> 9) & (EXT_NB_COTE - 1);
	const int32 zi = (z >> 9) & (EXT_NB_COTE - 1);
	const int32 tri = getTriangleIndex(x, z);
	const ExteriorHalfPoly &poly = _mapPolyGround[(zi * EXT_NB_COTE + xi) * 2 + tri];
	return poly.col != 0;
}

int32 Exterior::lineRain(int32 x0, int32 y0, int32 z0, int32 x1, int32 y1, int32 z1, int32 color) {
	const int32 w = _zBufferWidth;
	const int32 h = _zBufferHeight;
	if (w <= 0 || h <= 0 || _zBuffer.empty()) {
		return 0;
	}

	int32 cx0 = x0;
	int32 cy0 = y0;
	int32 cx1 = x1;
	int32 cy1 = y1;
	int32 cz0 = z0 << 16;
	int32 cz1 = z1 << 16;

	const int32 clipXMin = 0;
	const int32 clipYMin = 0;
	const int32 clipXMax = w - 1;
	const int32 clipYMax = h - 1;

	auto clipEndpoints = [&]() -> bool {
		const int32 dz = cz1 - cz0;
		int32 dx = cx1 - cx0;
		int32 dy = cy1 - cy0;
		if (dx == 0 && dy == 0) {
			return cx0 >= clipXMin && cx0 <= clipXMax && cy0 >= clipYMin && cy0 <= clipYMax;
		}
		for (int pass = 0; pass < 2; ++pass) {
			if (cx0 < clipXMin) {
				if (cx1 < clipXMin) return false;
				const int32 t = cx0 - clipXMin;
				cz0 -= (int64)dz * t / dx;
				cy0 -= (int64)dy * t / dx;
				cx0 = clipXMin;
			} else if (cx0 > clipXMax) {
				if (cx1 > clipXMax) return false;
				const int32 t = cx0 - clipXMax;
				cz0 -= (int64)dz * t / dx;
				cy0 -= (int64)dy * t / dx;
				cx0 = clipXMax;
			}
			if (cy0 < clipYMin) {
				if (cy1 < clipYMin) return false;
				const int32 t = cy0 - clipYMin;
				cz0 -= (int64)dz * t / dy;
				cx0 -= (int64)dx * t / dy;
				cy0 = clipYMin;
			} else if (cy0 > clipYMax) {
				if (cy1 > clipYMax) return false;
				const int32 t = cy0 - clipYMax;
				cz0 -= (int64)dz * t / dy;
				cx0 -= (int64)dx * t / dy;
				cy0 = clipYMax;
			}
			if (cx1 < clipXMin) {
				const int32 t = cx1 - clipXMin;
				cz1 -= (int64)dz * t / dx;
				cy1 -= (int64)dy * t / dx;
				cx1 = clipXMin;
			} else if (cx1 > clipXMax) {
				const int32 t = cx1 - clipXMax;
				cz1 -= (int64)dz * t / dx;
				cy1 -= (int64)dy * t / dx;
				cx1 = clipXMax;
			}
			if (cy1 < clipYMin) {
				const int32 t = cy1 - clipYMin;
				cz1 -= (int64)dz * t / dy;
				cx1 -= (int64)dx * t / dy;
				cy1 = clipYMin;
			} else if (cy1 > clipYMax) {
				const int32 t = cy1 - clipYMax;
				cz1 -= (int64)dz * t / dy;
				cx1 -= (int64)dx * t / dy;
				cy1 = clipYMax;
			}
			dx = cx1 - cx0;
			dy = cy1 - cy0;
		}
		return true;
	};

	if (!clipEndpoints()) {
		return 0;
	}

	uint8 c = (uint8)(color & 0xFF);
	int32 result = 0;

	const int32 dx = ABS(cx1 - cx0);
	const int32 dy = ABS(cy1 - cy0);
	const int32 dz = cz1 - cz0;
	const int32 steps = MAX(dx, dy);
	if (steps == 0) {
		if (cx0 >= 0 && cy0 >= 0 && cx0 < w && cy0 < h) {
			const int32 zb = _zBuffer[cy0 * w + cx0];
			const int32 diff = zb - ((cz0 >> 16) - 100);
			if (diff >= 0) {
				if (diff < 200) result |= 2;
				_engine->_workVideoBuffer.setPixel(cx0, cy0, c);
				result |= 1;
			}
		}
		return result;
	}

	const int32 sx = (cx0 < cx1) ? 1 : -1;
	const int32 sy = (cy0 < cy1) ? 1 : -1;
	int32 err = dx - dy;
	int32 x = cx0;
	int32 y = cy0;
	int32 z = cz0;

	for (int32 i = 0; i <= steps; ++i) {
		if (x >= 0 && y >= 0 && x < w && y < h) {
			const int32 zHigh = z >> 16;
			const int32 zb = _zBuffer[y * w + x];
			const int32 diff = zb - (zHigh - 100);
			if (diff >= 0) {
				if (diff < 200) {
					result |= 2;
				}
				_engine->_workVideoBuffer.setPixel(x, y, c);
				result |= 1;
			}
		}

		if (x == cx1 && y == cy1) {
			break;
		}

		const int32 e2 = 2 * err;
		if (e2 > -dy) {
			err -= dy;
			x += sx;
			z += dz * sx / MAX(ABS(cx1 - cx0), 1);
		}
		if (e2 < dx) {
			err += dx;
			y += sy;
			z += dz * sy / MAX(ABS(cy1 - cy0), 1);
		}
	}

	return result;
}

} // namespace TwinE
