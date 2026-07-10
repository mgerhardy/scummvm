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

#ifndef TWINE_SCENE_EXTERIOR_H
#define TWINE_SCENE_EXTERIOR_H

#include "common/array.h"
#include "common/hashmap.h"
#include "common/scummsys.h"
#include "twine/parser/body.h"
#include "twine/shared.h"

namespace TwinE {

class TwinEEngine;

#define EXT_NB_COTE 64
#define EXT_SIZE_MAIN_MAP 16
#define EXT_HQR_START_CUBE 3
#define EXT_HQR_STEP_CUBE 6

#define EXT_INFO_ALPHA_LIGHT 0
#define EXT_INFO_BETA_LIGHT 1
#define EXT_INFO_NB_DECORS 2
#define EXT_INFO_SKY_Y 3
#define EXT_INFO_START_Z_FOG 4
#define EXT_INFO_CLIP_Z_FAR 5

struct ExteriorHalfPoly {
	uint32 bank : 4;
	uint32 texFlag : 2;
	uint32 polyFlag : 2;
	uint32 sampleStep : 4;
	uint32 codeJeu : 4;
	uint32 sens : 1;
	uint32 col : 1;
	uint32 dummy : 1;
	uint32 indexTex : 13;
};

struct ExteriorHalfTex {
	uint16 tx0;
	uint16 ty0;
	uint16 tx1;
	uint16 ty1;
	uint16 tx2;
	uint16 ty2;
};

struct ExteriorVertex {
	int16 x2d = 0;
	int16 y2d = 0;
	int32 zrot = 0;
	uint8 visible = 0;
};

struct ExteriorDecor {
	int32 bodyField = 0;
	int32 x = 0;
	int32 y = 0;
	int32 z = 0;
	int32 codeJeu = 0;
	int32 beta = 0;
	int32 xMin = 0;
	int32 yMin = 0;
	int32 zMin = 0;
	int32 xMax = 0;
	int32 yMax = 0;
	int32 zMax = 0;
};

class Exterior {
private:
	TwinEEngine *_engine = nullptr;

	char _islandName[32]{};
	int32 _lastIsland = -1;
	char _loadedIslandName[32]{};

	uint8 *_islandMap = nullptr;
	uint8 *_groundTexture = nullptr;
	uint8 *_objTexture = nullptr;
	uint8 *_skySeaTexture = nullptr;

	int32 *_cubeInfos = nullptr;
	uint8 *_decorList = nullptr;
	ExteriorHalfPoly *_mapPolyGround = nullptr;
	ExteriorHalfTex *_listTexDef = nullptr;
	int16 *_mapSommetY = nullptr;
	uint8 *_mapIntensity = nullptr;

	Common::Array<ExteriorDecor> _decors;
	Common::HashMap<int, BodyData> _decorBodies;

	Common::Array<uint16> _zBuffer;
	int32 _zBufferWidth = 0;
	int32 _zBufferHeight = 0;

	int32 _clipZFar = 48000;
	int32 _startZFog = 12000;
	int32 _skyY = 0;
	int32 _alphaLight = 0;
	int32 _betaLight = 0;
	uint16 _cubeBitField = 0xFFFF;

	ExteriorVertex _vertices[(EXT_NB_COTE + 1) * (EXT_NB_COTE + 1)];

	bool loadIslandFile(const char *baseName);
	void resolveIslandName(int32 island);
	int32 getCubeHqrIndex(int32 cubeX, int32 cubeY) const;
	bool loadCubeData(int32 cubeX, int32 cubeY);
	void loadSkySea(int32 island);
	void clearTerrainZBuf();
	void projectTerrainVertices();
	void drawTerrain();
	void drawSkySea();
	void rasterizeTriangleZ(int32 x0, int32 y0, int32 z0, int32 x1, int32 y1, int32 z1, int32 x2, int32 y2, int32 z2);
	void drawTerrainTriangle(const ExteriorVertex &v0, const ExteriorVertex &v1, const ExteriorVertex &v2,
	                         const ExteriorHalfPoly &poly, const ExteriorHalfTex *texDef, uint8 fogColor);
	int32 getTriangleIndex(int32 x, int32 z) const;
	void parseDecorList();
	const BodyData *getDecorBody(int32 bodyIndex);
	void drawDecors();
	bool isDecorVisible(const ExteriorDecor &decor) const;

public:
	Exterior(TwinEEngine *engine);
	~Exterior();

	bool isActive() const;
	void init();
	void initGrid();
	void redraw();

	int32 getAltitude(int32 x, int32 z, int32 waterCode = 0) const;
	bool giveTerrainCol(int32 x, int32 z) const;
	uint8 getTerrainCodeJeu(int32 x, int32 z) const;
	uint8 getTerrainBrickCode(int32 x, int32 y, int32 z) const;
	bool testDecorCollision(int32 x, int32 y, int32 z) const;

	int32 clipZFar() const { return _clipZFar; }
	int32 startZFog() const { return _startZFog; }
	const uint16 *zBuffer() const { return _zBuffer.data(); }
	int32 zBufferWidth() const { return _zBufferWidth; }
	int32 zBufferHeight() const { return _zBufferHeight; }

	int32 lineRain(int32 x0, int32 y0, int32 z0, int32 x1, int32 y1, int32 z1, int32 color);
};

} // namespace TwinE

#endif
