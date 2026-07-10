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

#include "twine/renderer/renderer.h"
#include "common/util.h"
#include <cmath>
#include "twine/menu/interface.h"
#include "twine/parser/body.h"
#include "twine/parser/anim.h"
#include "twine/renderer/redraw.h"
#include "twine/renderer/shadeangletab.h"
#include "twine/resources/resources.h"
#include "twine/scene/actor.h"
#include "twine/scene/grid.h"
#include "twine/scene/movements.h"
#include "twine/shared.h"
#include "twine/twine.h"

namespace TwinE {

#define RENDERTYPE_DRAWLINE 0
#define RENDERTYPE_DRAWPOLYGON 1
#define RENDERTYPE_DRAWSPHERE 2

Renderer::Renderer(TwinEEngine *engine) : _engine(engine) {
}

Renderer::~Renderer() {
	free(_tabVerticG);
	free(_tabVerticD);
	free(_tabCoulG);
	free(_tabCoulD);
	free(_tabMapU0);
	free(_tabMapV0);
	free(_tabMapU1);
	free(_tabMapV1);
	free(_tabPerspW0);
	free(_tabPerspW1);
	free(_tabPerspUW0);
	free(_tabPerspUW1);
	free(_tabPerspVW0);
	free(_tabPerspVW1);
}

void Renderer::init(int32 w, int32 h) {
	size_t size = _engine->height() * sizeof(int16);

	_tabVerticG = (int16 *)malloc(size);
	memset(_tabVerticG, 0, size);
	_tabVerticD = (int16 *)malloc(size);
	memset(_tabVerticD, 0, size);
	_tabCoulG = (int16 *)malloc(size);
	memset(_tabCoulG, 0, size);
	_tabCoulD = (int16 *)malloc(size);
	memset(_tabCoulD, 0, size);
	_tabMapU0 = (int16 *)malloc(size);
	memset(_tabMapU0, 0, size);
	_tabMapV0 = (int16 *)malloc(size);
	memset(_tabMapV0, 0, size);
	_tabMapU1 = (int16 *)malloc(size);
	memset(_tabMapU1, 0, size);
	_tabMapV1 = (int16 *)malloc(size);
	memset(_tabMapV1, 0, size);
	const size_t size32 = _engine->height() * sizeof(int32);
	_tabPerspW0 = (int32 *)malloc(size32);
	memset(_tabPerspW0, 0, size32);
	_tabPerspW1 = (int32 *)malloc(size32);
	memset(_tabPerspW1, 0, size32);
	_tabPerspUW0 = (int32 *)malloc(size32);
	memset(_tabPerspUW0, 0, size32);
	_tabPerspUW1 = (int32 *)malloc(size32);
	memset(_tabPerspUW1, 0, size32);
	_tabPerspVW0 = (int32 *)malloc(size32);
	memset(_tabPerspVW0, 0, size32);
	_tabPerspVW1 = (int32 *)malloc(size32);
	memset(_tabPerspVW1, 0, size32);

	_tabx0 = _tabMapU0;
	_tabx1 = _tabMapU1;
}

void Renderer::projIso(IVec3 &pos, int32 x, int32 y, int32 z) {
	pos.x = (int16)((((x - z) * 24) / ISO_SCALE) + _projectionCenter.x);
	pos.y = (int16)(((((x + z) * 12) - (y * 30)) / ISO_SCALE) + _projectionCenter.y);
	pos.z = 0;
}

bool Renderer::longProjectPoint(const IVec3 &rotatedWorld, IVec3 &proj) {
	if (_typeProj == TYPE_ISO) {
		projIso(proj, rotatedWorld.x, rotatedWorld.y, rotatedWorld.z);
		return true;
	}
	if (_cameraRot.z - rotatedWorld.z <= 0) {
		proj.x = 0;
		proj.y = 0;
		proj.z = 0;
		return false;
	}
	proj = projectPoint(rotatedWorld);
	return true;
}

void Renderer::drawTexturedGroundTriangle(const ComputedVertex screenCoords[3], const ComputedVertex texCoords[3], uint8 renderType, const uint8 *texture, int16 flatShade, uint16 repMask) {
	renderTexturedTriangle(screenCoords, texCoords, renderType, texture, flatShade, repMask);
}

IVec3 Renderer::projectPoint(int32 cX, int32 cY, int32 cZ) { // ProjettePoint
	IVec3 pos;
	if (_typeProj == TYPE_ISO) {
		projIso(pos, cX, cY, cZ);
		return pos;
	}

	if (_cameraRot.z - cZ < 0) {
		pos.x = 0;
		pos.y = 0;
		pos.z = 0;
		return pos;
	}

	cX -= _cameraRot.x;
	cY -= _cameraRot.y;
	cZ = _cameraRot.z - cZ;

	int32 posZ = cZ + _kFactor;
	if (posZ <= 0) {
		posZ = 0x7FFF;
	}

	pos.x = (cX * _lFactorX) / posZ + _projectionCenter.x;
	pos.y = (-cY * _lFactorY) / posZ + _projectionCenter.y;
	pos.z = posZ;
	return pos;
}

void Renderer::setProjection(int32 x, int32 y, int32 kfact, int32 lfactx, int32 lfacty) {
	_projectionCenter.x = x;
	_projectionCenter.y = y;

	_kFactor = kfact;
	_lFactorX = lfactx;
	_lFactorY = lfacty;

	_typeProj = TYPE_3D;
}

void Renderer::setPosCamera(int32 x, int32 y, int32 z) {
	_cameraPos.x = x;
	_cameraPos.y = y;
	_cameraPos.z = z;
}

void Renderer::setIsoProjection(int32 x, int32 y, int32 scale) {
	_projectionCenter.x = x;
	_projectionCenter.y = y;
	_projectionCenter.z = scale; // not used - IsoScale is always 512

	_typeProj = TYPE_ISO;
}

void Renderer::clearPolySpans(int32 yMin, int32 yMax) {
	if (yMin < 0) {
		yMin = 0;
	}
	const int32 maxY = _engine->height() - 1;
	if (yMax > maxY) {
		yMax = maxY;
	}
	if (yMin > yMax) {
		return;
	}

	const int32 count = yMax - yMin + 1;
	memset(&_tabVerticG[yMin], 0x7F, count * sizeof(int16));
	memset(&_tabVerticD[yMin], 0x80, count * sizeof(int16));
	memset(&_tabCoulG[yMin], 0, count * sizeof(int16));
	memset(&_tabCoulD[yMin], 0, count * sizeof(int16));
	memset(&_tabMapU0[yMin], 0, count * sizeof(int16));
	memset(&_tabMapV0[yMin], 0, count * sizeof(int16));
	memset(&_tabMapU1[yMin], 0, count * sizeof(int16));
	memset(&_tabMapV1[yMin], 0, count * sizeof(int16));
	if (_tabPerspUW0) {
		memset(&_tabPerspUW0[yMin], 0, count * sizeof(int32));
		memset(&_tabPerspVW0[yMin], 0, count * sizeof(int32));
		memset(&_tabPerspW0[yMin], 0, count * sizeof(int32));
		memset(&_tabPerspUW1[yMin], 0, count * sizeof(int32));
		memset(&_tabPerspVW1[yMin], 0, count * sizeof(int32));
		memset(&_tabPerspW1[yMin], 0, count * sizeof(int32));
	}
}

void Renderer::flipMatrix() { // FlipMatrice
	SWAP(_matrixWorld.row1.y, _matrixWorld.row2.x);
	SWAP(_matrixWorld.row1.z, _matrixWorld.row3.x);
	SWAP(_matrixWorld.row2.z, _matrixWorld.row3.y);
}

IVec3 Renderer::setInverseAngleCamera(int32 alpha, int32 beta, int32 gamma) {
	setAngleCamera(alpha, beta, gamma);
	flipMatrix();
	_cameraRot = longWorldRot(_cameraPos.x, _cameraPos.y, _cameraPos.z);
	return _cameraRot;
}

IVec3 Renderer::setAngleCamera(int32 alpha, int32 beta, int32 gamma) {
	const bool lba2 = _engine->isLBA2();
	const int32 nSin = trigSin(alpha, lba2);
	const int32 nCos = trigCos(alpha, lba2);
	int32 nSin2 = trigSin(gamma, lba2);
	int32 nCos2 = trigCos(gamma, lba2);

	_matrixWorld.row1.x = nCos2;
	_matrixWorld.row1.y = -nSin2;
	_matrixWorld.row2.x = (nSin2 * nCos) >> 14;
	_matrixWorld.row2.y = (nCos2 * nCos) >> 14;
	_matrixWorld.row3.x = (nSin2 * nSin) >> 14;
	_matrixWorld.row3.y = (nCos2 * nSin) >> 14;

	nSin2 = trigSin(beta, lba2);
	nCos2 = trigCos(beta, lba2);

	int32 h = _matrixWorld.row1.x;
	_matrixWorld.row1.x = (nCos2 * h) >> 14;
	_matrixWorld.row1.z = (nSin2 * h) >> 14;

	h = _matrixWorld.row2.x;
	_matrixWorld.row2.x = ((nCos2 * h) + (nSin2 * nSin)) >> 14;
	_matrixWorld.row2.z = ((nSin2 * h) - (nCos2 * nSin)) >> 14;

	h = _matrixWorld.row3.x;
	_matrixWorld.row3.x = ((nCos2 * h) - (nSin2 * nCos)) >> 14;
	_matrixWorld.row3.z = ((nCos2 * nCos) + (nSin2 * h)) >> 14;

	_cameraRot = longWorldRot(_cameraPos.x, _cameraPos.y, _cameraPos.z);

	if (_engine->isLBA2()) {
		recomputeLight();
	}

	return _cameraRot;
}

IVec3 Renderer::worldRotatePoint(const IVec3& vec) {
	const int32 vx = (_matrixWorld.row1.x * vec.x + _matrixWorld.row1.y * vec.y + _matrixWorld.row1.z * vec.z) >> 14;
	const int32 vy = (_matrixWorld.row2.x * vec.x + _matrixWorld.row2.y * vec.y + _matrixWorld.row2.z * vec.z) >> 14;
	const int32 vz = (_matrixWorld.row3.x * vec.x + _matrixWorld.row3.y * vec.y + _matrixWorld.row3.z * vec.z) >> 14;
	return IVec3(vx, vy, vz);
}

IVec3 Renderer::longWorldRot(int32 x, int32 y, int32 z) {
	const int64 vx = ((int64)_matrixWorld.row1.x * (int64)x + (int64)_matrixWorld.row1.y * (int64)y + (int64)_matrixWorld.row1.z * (int64)z) >> 14;
	const int64 vy = ((int64)_matrixWorld.row2.x * (int64)x + (int64)_matrixWorld.row2.y * (int64)y + (int64)_matrixWorld.row2.z * (int64)z) >> 14;
	const int64 vz = ((int64)_matrixWorld.row3.x * (int64)x + (int64)_matrixWorld.row3.y * (int64)y + (int64)_matrixWorld.row3.z * (int64)z) >> 14;
	return IVec3((int32)vx, (int32)vy, (int32)vz);
}

IVec3 Renderer::longInverseRot(int32 x, int32 y, int32 z) {
	const int64 vx = ((int64)_matrixWorld.row1.x * (int64)x + (int64)_matrixWorld.row2.x * (int64)y + (int64)_matrixWorld.row3.x * (int64)z) >> 14;
	const int64 vy = ((int64)_matrixWorld.row1.y * (int64)x + (int64)_matrixWorld.row2.y * (int64)y + (int64)_matrixWorld.row3.y * (int64)z) >> 14;
	const int64 vz = ((int64)_matrixWorld.row1.z * (int64)x + (int64)_matrixWorld.row2.z * (int64)y + (int64)_matrixWorld.row3.z * (int64)z) >> 14;
	return IVec3((int32)vx, (int32)vy, (int32)vz);
}

IVec3 Renderer::rotateRootAnimStep(int32 alpha, int32 beta, int32 gamma, int32 x, int32 y, int32 z) {
	IMatrix3x3 identity;
	identity.row1 = IVec3(16384, 0, 0);
	identity.row2 = IVec3(0, 16384, 0);
	identity.row3 = IVec3(0, 0, 16384);
	IMatrix3x3 stepMatrix;
	rotMatIndex2(&stepMatrix, &identity, IVec3(alpha, beta, gamma));
	return rot(stepMatrix, x, y, z);
}

IVec3 Renderer::inverseRotPoint(const IMatrix3x3 &matrix, int32 x, int32 y, int32 z) {
	const int64 vx = ((int64)matrix.row1.x * (int64)x + (int64)matrix.row2.x * (int64)y + (int64)matrix.row3.x * (int64)z) >> 14;
	const int64 vy = ((int64)matrix.row1.y * (int64)x + (int64)matrix.row2.y * (int64)y + (int64)matrix.row3.y * (int64)z) >> 14;
	const int64 vz = ((int64)matrix.row1.z * (int64)x + (int64)matrix.row2.z * (int64)y + (int64)matrix.row3.z * (int64)z) >> 14;
	return IVec3((int32)vx, (int32)vy, (int32)vz);
}

void Renderer::recomputeLight() {
	_normalLight = rot(_matrixWorld, _normalLightLocal.x, _normalLightLocal.y, _normalLightLocal.z);
}

IVec3 Renderer::rot(const IMatrix3x3 &matrix, int32 x, int32 y, int32 z) {
	const int32 vx = (matrix.row1.x * x + matrix.row1.y * y + matrix.row1.z * z) >> 14;
	const int32 vy = (matrix.row2.x * x + matrix.row2.y * y + matrix.row2.z * z) >> 14;
	const int32 vz = (matrix.row3.x * x + matrix.row3.y * y + matrix.row3.z * z) >> 14;
	return IVec3(vx, vy, vz);
}

void Renderer::setFollowCamera(int32 targetX, int32 targetY, int32 targetZ, int32 cameraAlpha, int32 cameraBeta, int32 cameraGamma, int32 cameraZoom) {
	_cameraPos.x = targetX;
	_cameraPos.y = targetY;
	_cameraPos.z = targetZ;

	setAngleCamera(cameraAlpha, cameraBeta, cameraGamma);
	_cameraRot.z += cameraZoom;

	_cameraPos = longInverseRot(_cameraRot.x, _cameraRot.y, _cameraRot.z);
}

IVec2 Renderer::rotate(int32 side, int32 forward, int32 angle) const {
	if (angle) {
		const bool lba2 = _engine->isLBA2();
		const int32 nSin = trigSin(angle, lba2);
		const int32 nCos = trigCos(angle, lba2);

		const int32 x0 = ((side * nCos) + (forward * nSin)) >> 14;
		const int32 y0 = ((forward * nCos) - (side * nSin)) >> 14;
		return IVec2(x0, y0);
	}
	return IVec2(side, forward);
}

void Renderer::rotMatIndex2(IMatrix3x3 *pDest, const IMatrix3x3 *pSrc, const IVec3 &angleVec) {
	IMatrix3x3 tmp;
	const int32 lAlpha = angleVec.x;
	const int32 lBeta = angleVec.y;
	const int32 lGamma = angleVec.z;
	const bool lba2 = _engine->isLBA2();

	if (lAlpha) {
		int32 nSin = trigSin(lAlpha, lba2);
		int32 nCos = trigCos(lAlpha, lba2);

		pDest->row1.x = pSrc->row1.x;
		pDest->row2.x = pSrc->row2.x;
		pDest->row3.x = pSrc->row3.x;

		pDest->row1.y = (pSrc->row1.z * nSin + pSrc->row1.y * nCos) >> 14;
		pDest->row1.z = (pSrc->row1.z * nCos - pSrc->row1.y * nSin) >> 14;
		pDest->row2.y = (pSrc->row2.z * nSin + pSrc->row2.y * nCos) >> 14;
		pDest->row2.z = (pSrc->row2.z * nCos - pSrc->row2.y * nSin) >> 14;
		pDest->row3.y = (pSrc->row3.z * nSin + pSrc->row3.y * nCos) >> 14;
		pDest->row3.z = (pSrc->row3.z * nCos - pSrc->row3.y * nSin) >> 14;
		pSrc = pDest;
	}

	if (lGamma) {
		int32 nSin = trigSin(lGamma, lba2);
		int32 nCos = trigCos(lGamma, lba2);

		tmp.row1.z = pSrc->row1.z;
		tmp.row2.z = pSrc->row2.z;
		tmp.row3.z = pSrc->row3.z;

		tmp.row1.x = (pSrc->row1.y * nSin + pSrc->row1.x * nCos) >> 14;
		tmp.row1.y = (pSrc->row1.y * nCos - pSrc->row1.x * nSin) >> 14;
		tmp.row2.x = (pSrc->row2.y * nSin + pSrc->row2.x * nCos) >> 14;
		tmp.row2.y = (pSrc->row2.y * nCos - pSrc->row2.x * nSin) >> 14;
		tmp.row3.x = (pSrc->row3.y * nSin + pSrc->row3.x * nCos) >> 14;
		tmp.row3.y = (pSrc->row3.y * nCos - pSrc->row3.x * nSin) >> 14;

		pSrc = &tmp;
	}

	if (lBeta) {
		int32 nSin = trigSin(lBeta, lba2);
		int32 nCos = trigCos(lBeta, lba2);

		if (pSrc == pDest) {
			tmp.row1.x = pSrc->row1.x;
			tmp.row1.z = pSrc->row1.z;
			tmp.row2.x = pSrc->row2.x;
			tmp.row2.z = pSrc->row2.z;
			tmp.row3.x = pSrc->row3.x;
			tmp.row3.z = pSrc->row3.z;
			pSrc = &tmp;
		} else {
			pDest->row1.y = pSrc->row1.y;
			pDest->row2.y = pSrc->row2.y;
			pDest->row3.y = pSrc->row3.y;
		}

		pDest->row1.x = (pSrc->row1.x * nCos - pSrc->row1.z * nSin) >> 14;
		pDest->row1.z = (pSrc->row1.x * nSin + pSrc->row1.z * nCos) >> 14;
		pDest->row2.x = (pSrc->row2.x * nCos - pSrc->row2.z * nSin) >> 14;
		pDest->row2.z = (pSrc->row2.x * nSin + pSrc->row2.z * nCos) >> 14;
		pDest->row3.x = (pSrc->row3.x * nCos - pSrc->row3.z * nSin) >> 14;
		pDest->row3.z = (pSrc->row3.x * nSin + pSrc->row3.z * nCos) >> 14;
	} else if (pSrc != pDest) {
		*pDest = *pSrc;
	}
}

static bool isAnimRotateBone(uint16 type, bool lba2) {
	if (lba2) {
		return (type & (uint16)BoneType::TYPE_TRANSLATE) == 0;
	}
	return type == (uint16)BoneType::TYPE_ROTATE;
}

static bool isAnimTranslateBone(uint16 type, bool lba2) {
	if (lba2) {
		return (type & (uint16)BoneType::TYPE_TRANSLATE) != 0;
	}
	return type == (uint16)BoneType::TYPE_TRANSLATE;
}

bool isPolygonVisible(const ComputedVertex *vertices) { // TestVuePoly
	const int32 a = ((int32)vertices[0].y - (int32)vertices[2].y) * ((int32)vertices[1].x - (int32)vertices[0].x);
	const int32 b = ((int32)vertices[1].y - (int32)vertices[0].y) * ((int32)vertices[0].x - (int32)vertices[2].x);
	if (a <= b) {
		return false;
	}
	return true;
}

void Renderer::rotList(const Common::Array<BodyVertex> &vertices, int32 firstPoint, int32 numPoints, I16Vec3 *destPoints, const IMatrix3x3 *rotationMatrix, const IVec3 &destPos) {
	for (int32 i = 0; i < numPoints; ++i) {
		const BodyVertex &vertex = vertices[i + firstPoint];
		destPoints->x = (int16)(((rotationMatrix->row1.x * vertex.x + rotationMatrix->row1.y * vertex.y + rotationMatrix->row1.z * vertex.z) >> 14) + destPos.x);
		destPoints->y = (int16)(((rotationMatrix->row2.x * vertex.x + rotationMatrix->row2.y * vertex.y + rotationMatrix->row2.z * vertex.z) >> 14) + destPos.y);
		destPoints->z = (int16)(((rotationMatrix->row3.x * vertex.x + rotationMatrix->row3.y * vertex.y + rotationMatrix->row3.z * vertex.z) >> 14) + destPos.z);

		destPoints++;
	}
}

// LBA2 RotTransList: matrix rotation plus InitMatrixTrans offset (AFF_OBJ.CPP)
void Renderer::rotTransList(const Common::Array<BodyVertex> &vertices, int32 firstPoint, int32 numPoints, I16Vec3 *destPoints, const IMatrix3x3 *matrix, const IVec3 &trans) {
	for (int32 i = 0; i < numPoints; ++i) {
		const BodyVertex &vertex = vertices[i + firstPoint];
		destPoints->x = (int16)(((matrix->row1.x * vertex.x + matrix->row1.y * vertex.y + matrix->row1.z * vertex.z) >> 14) + trans.x);
		destPoints->y = (int16)(((matrix->row2.x * vertex.x + matrix->row2.y * vertex.y + matrix->row2.z * vertex.z) >> 14) + trans.y);
		destPoints->z = (int16)(((matrix->row3.x * vertex.x + matrix->row3.y * vertex.y + matrix->row3.z * vertex.z) >> 14) + trans.z);

		destPoints++;
	}
}

// RotateGroupe
void Renderer::processRotatedElement(IMatrix3x3 *targetMatrix, int32 boneIdx, const Common::Array<BodyVertex> &vertices, int32 alpha, int32 beta, int32 gamma, const BodyBone &bone, ModelData *modelData) {
	const int32 firstPoint = bone.firstVertex;
	const int32 numOfPoints = bone.numVertices;
	const IVec3 renderAngle(ClampAngle(alpha), ClampAngle(beta), ClampAngle(gamma));

	if (!numOfPoints) {
		warning("RENDER WARNING: No points in this model!");
	}

	if (_engine->isLBA2()) {
		// AFF_OBJ.CPP: RotateMatrix + InitMatrixTrans + RotTransList
		const IMatrix3x3 *parentMatrix;
		IVec3 trans(0, 0, 0);
		if (boneIdx == 0) {
			parentMatrix = &_matrixWorld;
		} else {
			assert(bone.parent < ARRAYSIZE(_matricesTable));
			parentMatrix = &_matricesTable[bone.parent];
			const I16Vec3 &pivot = modelData->computedPoints[bone.vertex];
			trans = IVec3(pivot.x, pivot.y, pivot.z);
		}

		rotMatIndex2(targetMatrix, parentMatrix, renderAngle);
		rotTransList(vertices, firstPoint, numOfPoints, &modelData->computedPoints[firstPoint], targetMatrix, trans);
		return;
	}

	const IMatrix3x3 *currentMatrix;
	IVec3 destPos;
	if (bone.isRoot()) {
		currentMatrix = &_matrixWorld;
	} else {
		const int32 pointIdx = bone.vertex;
		const int32 matrixIndex = bone.parent;
		assert(matrixIndex >= 0 && matrixIndex < ARRAYSIZE(_matricesTable));
		currentMatrix = &_matricesTable[matrixIndex];

		destPos = modelData->computedPoints[pointIdx];
	}

	rotMatIndex2(targetMatrix, currentMatrix, renderAngle);
	rotList(vertices, firstPoint, numOfPoints, &modelData->computedPoints[firstPoint], targetMatrix, destPos);
}

void Renderer::transRotList(const Common::Array<BodyVertex> &vertices, int32 firstPoint, int32 numPoints, I16Vec3 *destPoints, const IMatrix3x3 *translationMatrix, const IVec3 &angleVec, const IVec3 &destPos) {
	for (int32 i = 0; i < numPoints; ++i) {
		const BodyVertex &vertex = vertices[i + firstPoint];
		const int16 tmpX = (int16)(vertex.x + angleVec.x);
		const int16 tmpY = (int16)(vertex.y + angleVec.y);
		const int16 tmpZ = (int16)(vertex.z + angleVec.z);

		destPoints->x = ((translationMatrix->row1.x * tmpX + translationMatrix->row1.y * tmpY + translationMatrix->row1.z * tmpZ) >> 14) + destPos.x;
		destPoints->y = ((translationMatrix->row2.x * tmpX + translationMatrix->row2.y * tmpY + translationMatrix->row2.z * tmpZ) >> 14) + destPos.y;
		destPoints->z = ((translationMatrix->row3.x * tmpX + translationMatrix->row3.y * tmpY + translationMatrix->row3.z * tmpZ) >> 14) + destPos.z;

		destPoints++;
	}
}

// TranslateGroupe
void Renderer::translateGroup(IMatrix3x3 *targetMatrix, int32 boneIdx, const Common::Array<BodyVertex> &vertices, int32 rotX, int32 rotY, int32 rotZ, const BodyBone &bone, ModelData *modelData) {
	const IVec3 renderAngle(rotX, rotY, rotZ);
	IVec3 destPos;

	if (_engine->isLBA2()) {
		// AFF_OBJ.CPP: RotatePoint(mat, stepX, stepY, stepZ) + CopyMatrix + InitMatrixTrans + RotTransList
		const IMatrix3x3 *parentMatrix;
		IVec3 pivot(0, 0, 0);
		if (boneIdx == 0) {
			parentMatrix = &_matrixWorld;
		} else {
			assert(bone.parent < ARRAYSIZE(_matricesTable));
			parentMatrix = &_matricesTable[bone.parent];
			const I16Vec3 &p = modelData->computedPoints[bone.vertex];
			pivot = IVec3(p.x, p.y, p.z);
		}

		const IVec3 animOffset = rot(*parentMatrix, rotX, rotY, rotZ);
		*targetMatrix = *parentMatrix;
		const IVec3 trans(pivot.x + animOffset.x, pivot.y + animOffset.y, pivot.z + animOffset.z);
		rotTransList(vertices, bone.firstVertex, bone.numVertices, &modelData->computedPoints[bone.firstVertex], targetMatrix, trans);
		return;
	}

	if (bone.isRoot()) {
		*targetMatrix = _matrixWorld;
	} else {
		destPos = modelData->computedPoints[bone.vertex];
		*targetMatrix = _matricesTable[bone.parent];
	}
	transRotList(vertices, bone.firstVertex, bone.numVertices, &modelData->computedPoints[bone.firstVertex], targetMatrix, renderAngle, destPos);
}

void Renderer::zoomGroup(IMatrix3x3 *targetMatrix, int32 boneIdx, const BodyBone &bone, const BoneFrame *boneData, ModelData *modelData) {
	const int32 zoomX = (int32)(uint8)((int16)boneData->x + 256);
	const int32 zoomY = (int32)(uint8)((int16)boneData->y + 256);
	const int32 zoomZ = (int32)(uint8)((int16)boneData->z + 256);

	I16Vec3 *pt = &modelData->computedPoints[bone.firstVertex];
	for (int32 i = 0; i < bone.numVertices; ++i, ++pt) {
		pt->x = (int16)((pt->x * zoomX) >> 8);
		pt->y = (int16)((pt->y * zoomY) >> 8);
		pt->z = (int16)((pt->z * zoomZ) >> 8);
	}

	if (boneIdx == 0) {
		*targetMatrix = _matrixWorld;
	} else {
		*targetMatrix = _matricesTable[bone.parent];
	}
}

void Renderer::setLightVector(int32 angleX, int32 angleY, int32 angleZ) {
	const int32 normalUnit = _engine->isLBA2() ? 15360 : 64; // LIB_NORMAL_UNIT vs LBA1
	if (_engine->isLBA2()) {
		IMatrix3x3 identity;
		identity.row1 = IVec3(16384, 0, 0);
		identity.row2 = IVec3(0, 16384, 0);
		identity.row3 = IVec3(0, 0, 16384);
		IMatrix3x3 lightMatrix;
		rotMatIndex2(&lightMatrix, &identity, IVec3(angleX, angleY, 0));
		_normalLightLocal = inverseRotPoint(lightMatrix, 0, 0, normalUnit);
		recomputeLight();
		return;
	}

	const IVec3 renderAngle(angleX, angleY, angleZ);
	IMatrix3x3 rotationMatrix;
	rotMatIndex2(&rotationMatrix, &_matrixWorld, renderAngle);
	_normalLight = rot(rotationMatrix, 0, 0, normalUnit - 5);
}

namespace {

bool usesGouraudShade(int16 polyRenderType) {
	const int16 baseType = polyRenderType >= POLYGONTYPE_TEXTURE_PERSP ? (int16)(polyRenderType - 3) : polyRenderType;
	if (baseType == POLYGONTYPE_TEXTURE || baseType == POLYGONTYPE_TEXTURE_FLAT) {
		return false;
	}
	return baseType >= POLYGONTYPE_GOURAUD;
}

bool usesPerspectiveTexture(int16 polyRenderType) {
	return polyRenderType >= POLYGONTYPE_TEXTURE_PERSP && polyRenderType <= POLYGONTYPE_TEXTURE_FLAT_PERSP;
}

int16 baseTextureRenderType(int16 polyRenderType) {
	if (polyRenderType == POLYGONTYPE_TEXTURE_PERSP) {
		return POLYGONTYPE_TEXTURE;
	}
	if (polyRenderType == POLYGONTYPE_TEXTURE_GOURAUD_PERSP) {
		return POLYGONTYPE_TEXTURE_GOURAUD;
	}
	if (polyRenderType == POLYGONTYPE_TEXTURE_FLAT_PERSP) {
		return POLYGONTYPE_TEXTURE_FLAT;
	}
	return polyRenderType;
}

uint8 mapLba2TextureRenderType(uint8 polyType) {
	const bool persp = polyType >= 16;
	switch (polyType) {
	case 8:
	case 12:
	case 16:
	case 20:
		return persp ? POLYGONTYPE_TEXTURE_PERSP : POLYGONTYPE_TEXTURE;
	case 9:
	case 13:
	case 17:
	case 21:
		return persp ? POLYGONTYPE_TEXTURE_FLAT_PERSP : POLYGONTYPE_TEXTURE_FLAT;
	default:
		return persp ? POLYGONTYPE_TEXTURE_GOURAUD_PERSP : POLYGONTYPE_TEXTURE_GOURAUD;
	}
}

int32 computePerspectiveW(int32 rotatedZ, int32 posZWr) {
	int32 z = rotatedZ + posZWr;
	if (z <= 0) {
		z = 1;
	}
	return (int32)((int64)W_NORM / z);
}

void mapBodyTextureUV(int16 &mapU, int16 &mapV, uint16 u, uint16 v, bool lba2) {
	if (lba2) {
		mapU = (int16)u;
		mapV = (int16)v;
	} else {
		mapU = (int16)((uint16)u << 8);
		mapV = (int16)((uint16)v << 8);
	}
}

uint32 buildEnvMappedUv(const IVec3 &rotated, uint16 scale) {
	const int32 scaleX = (int32)(scale & 0xFFu);
	const int32 scaleY = (int32)((uint32)scale >> 8);
	const int32 mapU = ((int32)(uint16)(rotated.x + 0x4000) * scaleX) >> 4;
	const uint32 packedMapV = (((uint32)((int32)(uint16)(rotated.y + 0x4000) * scaleY)) << 12) & 0xFFFF0000u;
	return (uint32)mapU | packedMapV;
}

void mapEnvTextureUV(int16 &mapU, int16 &mapV, uint32 packedUv) {
	mapU = (int16)(packedUv & 0xFFFF);
	mapV = (int16)(packedUv >> 16);
}

byte shadeTexturedPixel(byte texel, int16 light) {
	if (texel == 0) {
		return 0;
	}
	const int16 shade = CLIP<int16>(light >> 8, 0, 15);
	return (byte)((texel & 0xF0) | shade);
}

uint16 computeNormalLight(const BodyNormal &normal, const IMatrix3x3 &matrix, const IVec3 &cameraLight, bool lba2Format) {
	const int32 x = (int32)normal.x;
	const int32 y = (int32)normal.y;
	const int32 z = (int32)normal.z;

	int32 intensity = 0;
	if (lba2Format) {
		// LightListF: inverse-rotate camera light into bone space, then dot with normal.
		const int32 lx = (matrix.row1.x * cameraLight.x + matrix.row2.x * cameraLight.y + matrix.row3.x * cameraLight.z) >> 14;
		const int32 ly = (matrix.row1.y * cameraLight.x + matrix.row2.y * cameraLight.y + matrix.row3.y * cameraLight.z) >> 14;
		const int32 lz = (matrix.row1.z * cameraLight.x + matrix.row2.z * cameraLight.y + matrix.row3.z * cameraLight.z) >> 14;
		intensity = (int32)(((int64)lx * x + (int64)ly * y + (int64)lz * z) >> 16);
		if (intensity < 0) {
			intensity = 0;
		}
	} else {
		intensity += matrix.row1.x * x + matrix.row1.y * y + matrix.row1.z * z;
		intensity += matrix.row2.x * x + matrix.row2.y * y + matrix.row2.z * z;
		intensity += matrix.row3.x * x + matrix.row3.y * y + matrix.row3.z * z;
		if (intensity > 0) {
			intensity >>= 14;
			if (normal.prenormalizedRange != 0) {
				intensity /= normal.prenormalizedRange;
			}
		} else {
			intensity = 0;
		}
	}

	if (intensity < 0) {
		intensity = 0;
	}

	if (lba2Format) {
		return (uint16)(MIN<int32>(intensity, 255) << 8);
	}
	return (uint16)intensity;
}

uint8 lba2BaseColour(const BodyPolygon &polygon) {
	return (uint8)((polygon.colorIndex << 4) | (polygon.intensity & 0x0f));
}

uint8 lba2VertexColour(const int16 *normalTable, const BodyPolygon &polygon, uint16 vertexIndex) {
	const uint8 baseColour = lba2BaseColour(polygon);
	if (vertexIndex >= 500) {
		return baseColour;
	}
	return (uint8)((baseColour + (normalTable[vertexIndex] >> 8)) & 0xff);
}

} // namespace

int16 Renderer::leftClip(int16 polyRenderType, ComputedVertex **offTabPoly, int32 numVertices, ComputedVertex **offTabTexPoly) {
	const Common::Rect &clip = _engine->_interface->_clip;
	ComputedVertex *pTabPolyClip = offTabPoly[1];
	ComputedVertex *pTabPoly = offTabPoly[0];
	ComputedVertex *pTexTabPolyClip = offTabTexPoly ? offTabTexPoly[1] : nullptr;
	ComputedVertex *pTexTabPoly = offTabTexPoly ? offTabTexPoly[0] : nullptr;
	int16 newNbPoints = 0;

	// invert the pointers to continue on the clipped vertices in the next method
	offTabPoly[0] = pTabPolyClip;
	offTabPoly[1] = pTabPoly;
	if (offTabTexPoly) {
		offTabTexPoly[0] = pTexTabPolyClip;
		offTabTexPoly[1] = pTexTabPoly;
	}

	for (; numVertices > 0; --numVertices, pTabPoly++) {
		const ComputedVertex *p0 = pTabPoly;
		const ComputedVertex *p1 = p0 + 1;
		const ComputedVertex *p0tex = pTexTabPoly;
		const ComputedVertex *p1tex = pTexTabPoly ? pTexTabPoly + 1 : nullptr;

		// clipFlag :
		// 0x00 : none clipped
		// 0x01 : point 0 clipped
		// 0x02 : point 1 clipped
		// 0x03 : both clipped
		uint8 clipFlag = (p1->x < clip.left) ? 2 : 0;

		if (p0->x < clip.left) {
			if (clipFlag) {
				if (pTexTabPoly) {
					++pTexTabPoly;
				}
				continue; // both clipped, skip point 0
			}
			clipFlag |= 1;
		} else {
			// point 0 not clipped, store it
			*pTabPolyClip++ = *pTabPoly;
			if (pTexTabPolyClip) {
				*pTexTabPolyClip++ = *pTexTabPoly;
			}
			++newNbPoints;
		}

		if (clipFlag) {
			// point 0 or 1 is clipped, apply clipping
			const ComputedVertex *pt0 = p0;
			const ComputedVertex *pt1 = p1;
			const ComputedVertex *pt0tex = p0tex;
			const ComputedVertex *pt1tex = p1tex;
			if (p1->x >= p0->x) {
				pt0 = p1;
				pt1 = pTabPoly;
				pt0tex = p1tex;
				pt1tex = p0tex;
			}

			const int32 dx = pt1->x - pt0->x;
			const int32 dy = pt1->y - pt0->y;
			const int32 dxClip = clip.left - pt0->x;

			pTabPolyClip->y = (int16)(pt0->y + ((dxClip * dy) / dx));
			pTabPolyClip->x = (int16)clip.left;

			if (usesGouraudShade(polyRenderType)) {
				pTabPolyClip->intensity = (int16)(pt0->intensity + (((pt1->intensity - pt0->intensity) * dxClip) / dx));
			}

			if (pTexTabPolyClip) {
				pTexTabPolyClip->x = (int16)(pt0tex->x + (((pt1tex->x - pt0tex->x) * dxClip) / dx));
				pTexTabPolyClip->y = (int16)(pt0tex->y + (((pt1tex->y - pt0tex->y) * dxClip) / dx));
				++pTexTabPolyClip;
			}

			++pTabPolyClip;
			++newNbPoints;
		}

		if (pTexTabPoly) {
			++pTexTabPoly;
		}
	}

	// copy first vertex to the end
	*pTabPolyClip = *offTabPoly[0];
	if (pTexTabPolyClip) {
		*pTexTabPolyClip = *offTabTexPoly[0];
	}
	return newNbPoints;
}

int16 Renderer::rightClip(int16 polyRenderType, ComputedVertex **offTabPoly, int32 numVertices, ComputedVertex **offTabTexPoly) {
	const Common::Rect &clip = _engine->_interface->_clip;
	ComputedVertex *pTabPolyClip = offTabPoly[1];
	ComputedVertex *pTabPoly = offTabPoly[0];
	ComputedVertex *pTexTabPolyClip = offTabTexPoly ? offTabTexPoly[1] : nullptr;
	ComputedVertex *pTexTabPoly = offTabTexPoly ? offTabTexPoly[0] : nullptr;
	int16 newNbPoints = 0;

	offTabPoly[0] = pTabPolyClip;
	offTabPoly[1] = pTabPoly;
	if (offTabTexPoly) {
		offTabTexPoly[0] = pTexTabPolyClip;
		offTabTexPoly[1] = pTexTabPoly;
	}

	for (; numVertices > 0; --numVertices, pTabPoly++) {
		const ComputedVertex *p0 = pTabPoly;
		const ComputedVertex *p1 = p0 + 1;
		const ComputedVertex *p0tex = pTexTabPoly;
		const ComputedVertex *p1tex = pTexTabPoly ? pTexTabPoly + 1 : nullptr;

		uint8 clipFlag = (p1->x > clip.right) ? 2 : 0;

		if (p0->x > clip.right) {
			if (clipFlag) {
				if (pTexTabPoly) {
					++pTexTabPoly;
				}
				continue;
			}
			clipFlag |= 1;
		} else {
			*pTabPolyClip++ = *pTabPoly;
			if (pTexTabPolyClip) {
				*pTexTabPolyClip++ = *pTexTabPoly;
			}
			++newNbPoints;
		}

		if (clipFlag) {
			const ComputedVertex *pt0 = p0;
			const ComputedVertex *pt1 = p1;
			const ComputedVertex *pt0tex = p0tex;
			const ComputedVertex *pt1tex = p1tex;
			if (p1->x >= p0->x) {
				pt0 = p1;
				pt1 = pTabPoly;
				pt0tex = p1tex;
				pt1tex = p0tex;
			}

			const int32 dx = pt1->x - pt0->x;
			const int32 dy = pt1->y - pt0->y;
			const int32 dxClip = clip.right - pt0->x;

			pTabPolyClip->y = (int16)(pt0->y + ((dxClip * dy) / dx));
			pTabPolyClip->x = (int16)clip.right;

			if (usesGouraudShade(polyRenderType)) {
				pTabPolyClip->intensity = (int16)(pt0->intensity + (((pt1->intensity - pt0->intensity) * dxClip) / dx));
			}

			if (pTexTabPolyClip) {
				pTexTabPolyClip->x = (int16)(pt0tex->x + (((pt1tex->x - pt0tex->x) * dxClip) / dx));
				pTexTabPolyClip->y = (int16)(pt0tex->y + (((pt1tex->y - pt0tex->y) * dxClip) / dx));
				++pTexTabPolyClip;
			}

			++pTabPolyClip;
			++newNbPoints;
		}

		if (pTexTabPoly) {
			++pTexTabPoly;
		}
	}

	*pTabPolyClip = *offTabPoly[0];
	if (pTexTabPolyClip) {
		*pTexTabPolyClip = *offTabTexPoly[0];
	}
	return newNbPoints;
}

int16 Renderer::topClip(int16 polyRenderType, ComputedVertex **offTabPoly, int32 numVertices, ComputedVertex **offTabTexPoly) {
	const Common::Rect &clip = _engine->_interface->_clip;
	ComputedVertex *pTabPolyClip = offTabPoly[1];
	ComputedVertex *pTabPoly = offTabPoly[0];
	ComputedVertex *pTexTabPolyClip = offTabTexPoly ? offTabTexPoly[1] : nullptr;
	ComputedVertex *pTexTabPoly = offTabTexPoly ? offTabTexPoly[0] : nullptr;
	int16 newNbPoints = 0;

	offTabPoly[0] = pTabPolyClip;
	offTabPoly[1] = pTabPoly;
	if (offTabTexPoly) {
		offTabTexPoly[0] = pTexTabPolyClip;
		offTabTexPoly[1] = pTexTabPoly;
	}

	for (; numVertices > 0; --numVertices, pTabPoly++) {
		const ComputedVertex *p0 = pTabPoly;
		const ComputedVertex *p1 = p0 + 1;
		const ComputedVertex *p0tex = pTexTabPoly;
		const ComputedVertex *p1tex = pTexTabPoly ? pTexTabPoly + 1 : nullptr;

		uint8 clipFlag = (p1->y < clip.top) ? 2 : 0;

		if (p0->y < clip.top) {
			if (clipFlag) {
				if (pTexTabPoly) {
					++pTexTabPoly;
				}
				continue;
			}
			clipFlag |= 1;
		} else {
			*pTabPolyClip++ = *pTabPoly;
			if (pTexTabPolyClip) {
				*pTexTabPolyClip++ = *pTexTabPoly;
			}
			++newNbPoints;
		}

		if (clipFlag) {
			const ComputedVertex *pt0 = p0;
			const ComputedVertex *pt1 = p1;
			const ComputedVertex *pt0tex = p0tex;
			const ComputedVertex *pt1tex = p1tex;
			if (p1->y >= p0->y) {
				pt0 = p1;
				pt1 = pTabPoly;
				pt0tex = p1tex;
				pt1tex = p0tex;
			}

			const int32 dx = pt1->x - pt0->x;
			const int32 dy = pt1->y - pt0->y;
			const int32 dyClip = clip.top - pt0->y;

			pTabPolyClip->x = (int16)(pt0->x + ((dyClip * dx) / dy));
			pTabPolyClip->y = (int16)clip.top;

			if (usesGouraudShade(polyRenderType)) {
				pTabPolyClip->intensity = (int16)(pt0->intensity + (((pt1->intensity - pt0->intensity) * dyClip) / dy));
			}

			if (pTexTabPolyClip) {
				pTexTabPolyClip->x = (int16)(pt0tex->x + (((pt1tex->x - pt0tex->x) * dyClip) / dy));
				pTexTabPolyClip->y = (int16)(pt0tex->y + (((pt1tex->y - pt0tex->y) * dyClip) / dy));
				++pTexTabPolyClip;
			}

			++pTabPolyClip;
			++newNbPoints;
		}

		if (pTexTabPoly) {
			++pTexTabPoly;
		}
	}

	*pTabPolyClip = *offTabPoly[0];
	if (pTexTabPolyClip) {
		*pTexTabPolyClip = *offTabTexPoly[0];
	}
	return newNbPoints;
}

int16 Renderer::bottomClip(int16 polyRenderType, ComputedVertex **offTabPoly, int32 numVertices, ComputedVertex **offTabTexPoly) {
	const Common::Rect &clip = _engine->_interface->_clip;
	ComputedVertex *pTabPolyClip = offTabPoly[1];
	ComputedVertex *pTabPoly = offTabPoly[0];
	ComputedVertex *pTexTabPolyClip = offTabTexPoly ? offTabTexPoly[1] : nullptr;
	ComputedVertex *pTexTabPoly = offTabTexPoly ? offTabTexPoly[0] : nullptr;
	int16 newNbPoints = 0;

	offTabPoly[0] = pTabPolyClip;
	offTabPoly[1] = pTabPoly;
	if (offTabTexPoly) {
		offTabTexPoly[0] = pTexTabPolyClip;
		offTabTexPoly[1] = pTexTabPoly;
	}

	for (; numVertices > 0; --numVertices, pTabPoly++) {
		const ComputedVertex *p0 = pTabPoly;
		const ComputedVertex *p1 = p0 + 1;
		const ComputedVertex *p0tex = pTexTabPoly;
		const ComputedVertex *p1tex = pTexTabPoly ? pTexTabPoly + 1 : nullptr;

		uint8 clipFlag = (p1->y > clip.bottom) ? 2 : 0;

		if (p0->y > clip.bottom) {
			if (clipFlag) {
				if (pTexTabPoly) {
					++pTexTabPoly;
				}
				continue;
			}
			clipFlag |= 1;
		} else {
			*pTabPolyClip++ = *pTabPoly;
			if (pTexTabPolyClip) {
				*pTexTabPolyClip++ = *pTexTabPoly;
			}
			++newNbPoints;
		}

		if (clipFlag) {
			const ComputedVertex *pt0 = p0;
			const ComputedVertex *pt1 = p1;
			const ComputedVertex *pt0tex = p0tex;
			const ComputedVertex *pt1tex = p1tex;
			if (p1->y >= p0->y) {
				pt0 = p1;
				pt1 = pTabPoly;
				pt0tex = p1tex;
				pt1tex = p0tex;
			}

			const int32 dx = pt1->x - pt0->x;
			const int32 dy = pt1->y - pt0->y;
			const int32 dyClip = clip.bottom - pt0->y;

			pTabPolyClip->x = (int16)(pt0->x + ((dyClip * dx) / dy));
			pTabPolyClip->y = (int16)clip.bottom;

			if (usesGouraudShade(polyRenderType)) {
				pTabPolyClip->intensity = (int16)(pt0->intensity + (((pt1->intensity - pt0->intensity) * dyClip) / dy));
			}

			if (pTexTabPolyClip) {
				pTexTabPolyClip->x = (int16)(pt0tex->x + (((pt1tex->x - pt0tex->x) * dyClip) / dy));
				pTexTabPolyClip->y = (int16)(pt0tex->y + (((pt1tex->y - pt0tex->y) * dyClip) / dy));
				++pTexTabPolyClip;
			}

			++pTabPolyClip;
			++newNbPoints;
		}

		if (pTexTabPoly) {
			++pTexTabPoly;
		}
	}

	*pTabPolyClip = *offTabPoly[0];
	if (pTexTabPolyClip) {
		*pTexTabPolyClip = *offTabTexPoly[0];
	}
	return newNbPoints;
}

int32 Renderer::computePolyMinMax(int16 polyRenderType, ComputedVertex **offTabPoly, int32 numVertices, int16 &ymin, int16 &ymax, ComputedVertex **offTabTexPoly) {
	int16 xmin = SCENE_SIZE_MAX;
	int16 xmax = SCENE_SIZE_MIN;

	ymin = SCENE_SIZE_MAX;
	ymax = SCENE_SIZE_MIN;

	ComputedVertex* pTabPoly = offTabPoly[0];
	for (int32 i = 0; i < numVertices; i++) {
		if (pTabPoly[i].x < xmin) {
			xmin = pTabPoly[i].x;
		}
		if (pTabPoly[i].x > xmax) {
			xmax = pTabPoly[i].x;
		}
		if (pTabPoly[i].y < ymin) {
			ymin = pTabPoly[i].y;
		}
		if (pTabPoly[i].y > ymax) {
			ymax = pTabPoly[i].y;
		}
	}

	const Common::Rect &clip = _engine->_interface->_clip;
	// no vertices
	if (ymin > ymax || xmax < clip.left || xmin > clip.right || ymax < clip.top || ymin > clip.bottom) {
		debug(10, "Clipped %i:%i:%i:%i, clip rect(%i:%i:%i:%i)", xmin, ymin, xmax, ymax, clip.left, clip.top, clip.right, clip.bottom);
		return 0;
	}

	pTabPoly[numVertices] = *offTabPoly[0];

	bool hasBeenClipped = false;

	int32 clippedNumVertices = numVertices;
	if (xmin < clip.left) {
		clippedNumVertices = leftClip(polyRenderType, offTabPoly, clippedNumVertices, offTabTexPoly);
		if (!clippedNumVertices) {
			return 0;
		}

		hasBeenClipped = true;
	}

	if (xmax > clip.right) {
		clippedNumVertices = rightClip(polyRenderType, offTabPoly, clippedNumVertices, offTabTexPoly);
		if (!clippedNumVertices) {
			return 0;
		}

		hasBeenClipped = true;
	}

	if (ymin < clip.top) {
		clippedNumVertices = topClip(polyRenderType, offTabPoly, clippedNumVertices, offTabTexPoly);
		if (!clippedNumVertices) {
			return 0;
		}

		hasBeenClipped = true;
	}

	if (ymax > clip.bottom) {
		clippedNumVertices = bottomClip(polyRenderType, offTabPoly, clippedNumVertices, offTabTexPoly);
		if (!clippedNumVertices) {
			return 0;
		}

		hasBeenClipped = true;
	}

	if (hasBeenClipped) {
		// search the new Ymin or Ymax
		ymin = 32767;
		ymax = -32768;

		for (int32 n = 0; n < clippedNumVertices; ++n) {
			if (offTabPoly[0][n].y < ymin) {
				ymin = offTabPoly[0][n].y;
			}

			if (offTabPoly[0][n].y > ymax) {
				ymax = offTabPoly[0][n].y;
			}
		}

		if (ymin >= ymax) {
			return 0; // No valid polygon after clipping
		}
	}

	return clippedNumVertices;
}

bool Renderer::computePoly(int16 polyRenderType, const ComputedVertex *vertices, int32 numVertices, int16 &vtop, int16 &vbottom) {
	assert(numVertices < ARRAYSIZE(_clippedPolygonVertices1));
	for (int i = 0; i < numVertices; ++i) {
		_clippedPolygonVertices1[i] = vertices[i];
	}

	ComputedVertex *offTabPoly[] = {_clippedPolygonVertices1, _clippedPolygonVertices2};

	numVertices = computePolyMinMax(polyRenderType, offTabPoly, numVertices, vtop, vbottom);
	if (numVertices == 0) {
		return false;
	}

	ComputedVertex *pTabPoly = offTabPoly[0];
	ComputedVertex *p0;
	ComputedVertex *p1;
	int16 *pVertic = nullptr;
	int16 *pCoul;
	int32 incY = -1;
	int32 dx, dy, x, y, dc;
	int32 step, reminder;

	// Drawing lines between vertices
	for (; numVertices > 0; --numVertices, pTabPoly++) {
		pCoul = nullptr;
		p0 = pTabPoly;
		p1 = p0 + 1;

		dy = p1->y - p0->y;
		if (dy == 0) {
			// forget same Y points
			continue;
		} else if (dy > 0) {
			// Y therefore goes down left buffer
			if (p0->x <= p1->x) {
				incY = 1;
			} else {
				p0 = p1;
				p1 = pTabPoly;
				incY = -1;
			}

			pVertic = &_tabVerticG[p0->y];

			if (usesGouraudShade(polyRenderType)) {
				pCoul = &_tabCoulG[p0->y];
			}
		} else if (dy < 0) {
			dy = -dy;

			if (p0->x <= p1->x) {
				p0 = p1;
				p1 = pTabPoly;
				incY = 1;
			} else {
				incY = -1;
			}

			pVertic = &_tabVerticD[p0->y];

			if (usesGouraudShade(polyRenderType)) {
				pCoul = &_tabCoulD[p0->y];
			}
		}

		dx = (p1->x - p0->x) << 16;

		step = dx / dy;
		reminder = ((dx % dy) >> 1) + 0x7FFF;

		dx = step >> 16; // recovery part high division (entire)
		step &= 0xFFFF;  // preserves lower part (mantissa)
		x = p0->x;

		for (y = dy; y >= 0; --y) {
			*pVertic = (int16)x;
			pVertic += incY;
			x += dx;
			reminder += step;
			if (reminder & 0xFFFF0000) {
				x += reminder >> 16;
				reminder &= 0xFFFF;
			}
		}

		if (pCoul) {
			dc = (p1->intensity - p0->intensity) << 8;
			step = dc / dy;
			reminder = ((((dc % dy) >> 1) + 0x7F) & 0xFF) | (p0->intensity << 8);

			for (y = dy; y >= 0; --y) {
				*pCoul = (int16)reminder;
				pCoul += incY;
				reminder += step;
			}
		}
	}

	return true;
}

void Renderer::svgaPolyCopper(int16 vtop, int16 Ymax, uint16 color) const {
	const int screenWidth = _engine->width();
	int16 xMin, xMax;
	int16 y = vtop;
	byte *pDestLine = (uint8 *)_engine->_frontVideoBuffer.getBasePtr(0, y);
	byte *pDest;
	int16 *pVerticG = &_tabVerticG[y];
	int16 *pVerticD = &_tabVerticD[y];
	int32 sens = 1;

	for (; y <= Ymax; y++) {
		xMin = *pVerticG++;
		xMax = *pVerticD++;
		pDest = pDestLine + xMin;

		for (; xMin <= xMax; xMin++) {
			*pDest++ = (byte)color;
		}

		color += sens;
		if (!(color & 0xF)) {
			sens = -sens;
			if (sens < 0) {
				color += sens;
			}
		}

		pDestLine += screenWidth;
	}
}

void Renderer::svgaPolyBopper(int16 vtop, int16 Ymax, uint16 color) const {
	const int screenWidth = _engine->width();
	int16 xMin, xMax;
	int16 y = vtop;
	byte *pDestLine = (uint8 *)_engine->_frontVideoBuffer.getBasePtr(0, y);
	byte *pDest;
	int16 *pVerticG = &_tabVerticG[y];
	int16 *pVerticD = &_tabVerticD[y];
	int32 sens = 1;
	int32 line = 2;

	for (; y <= Ymax; y++) {
		xMin = *pVerticG++;
		xMax = *pVerticD++;
		pDest = pDestLine + xMin;

		for (; xMin <= xMax; xMin++) {
			*pDest++ = (byte)color;
		}

		line--;
		if (!line) {
			line = 2;
			color += sens;
			if (!(color & 0xF)) {
				sens = -sens;
				if (sens < 0) {
					color += sens;
				}
			}
		}

		pDestLine += screenWidth;
	}
}

void Renderer::svgaPolyTriste(int16 vtop, int16 Ymax, uint16 color) const {
	const int screenWidth = _engine->width();
	int16 xMin, xMax;
	int16 y = vtop;
	byte *pDestLine = (uint8 *)_engine->_frontVideoBuffer.getBasePtr(0, vtop);
	byte *pDest;
	int16 *pVerticG = &_tabVerticG[y];
	int16 *pVerticD = &_tabVerticD[y];

	for (; y <= Ymax; y++) {
		xMin = *pVerticG++;
		xMax = *pVerticD++;
		pDest = pDestLine + xMin;

		for (; xMin <= xMax; xMin++) {
			*pDest++ = (byte)color;
		}

		pDestLine += screenWidth;
	}
}

#define ROL8(x,b) (byte)(((x) << (b)) | ((x) >> (8 - (b))))
#define ROL16(x, b) (((x) << (b)) | ((x) >> (16 - (b))))

void Renderer::svgaPolyTele(int16 vtop, int16 Ymax, uint16 color) const {
	const int screenWidth = _engine->width();
	int16 xMin, xMax;
	int16 y = vtop;
	int16 acc = 17371;
	byte *pDestLine = (uint8 *)_engine->_frontVideoBuffer.getBasePtr(0, vtop);
	byte *pDest;
	int16 *pVerticG = &_tabVerticG[y];
	int16 *pVerticD = &_tabVerticD[y];
	uint16 col;

	color &= 0xFF;

	for (; y <= Ymax; y++) {
		xMin = *pVerticG++;
		xMax = *pVerticD++;
		pDest = pDestLine + xMin;
		col = xMin;

		for (; xMin <= xMax; xMin++) {
			col = ((col + acc) & 0xFF03) + (uint16)color;
			acc = ROL16(acc, 2) + 1;

			*pDest++ = (byte)col;
		}

		pDestLine += screenWidth;
	}
}

void Renderer::svgaPolyTrans(int16 vtop, int16 Ymax, uint16 color) const {
	const int screenWidth = _engine->width();
	int16 xMin, xMax;
	int16 y = vtop;
	byte *pDestLine = (uint8 *)_engine->_frontVideoBuffer.getBasePtr(0, vtop);
	byte *pDest;
	int16 *pVerticG = &_tabVerticG[y];
	int16 *pVerticD = &_tabVerticD[y];

	color &= 0xF0;

	for (; y <= Ymax; y++) {
		xMin = *pVerticG++;
		xMax = *pVerticD++;
		pDest = pDestLine + xMin;

		for (; xMin <= xMax; xMin++) {
			*pDest = (byte)color | (*pDest & 0x0F);
			pDest++;
		}

		pDestLine += screenWidth;
	}
}

// Used e.g for the legs of the horse or the ears of most characters
void Renderer::svgaPolyTrame(int16 vtop, int16 Ymax, uint16 color) const {
	const int screenWidth = _engine->width();
	int16 xMin, xMax;
	int16 y = vtop;
	byte *pDestLine = (uint8 *)_engine->_frontVideoBuffer.getBasePtr(0, vtop);
	byte *pDest;
	int16 *pVerticG = &_tabVerticG[y];
	int16 *pVerticD = &_tabVerticD[y];
	int32 pair = 0;

	for (; y <= Ymax; y++) {
		xMin = *pVerticG++;
		xMax = *pVerticD++;
		pDest = pDestLine + xMin;

		xMax = ((xMax - xMin) + 1) / 2;
		if (xMax > 0) {
			pair ^= 1; // paire/impair
			if ((xMin & 1) ^ pair) {
				pDest++;
			}

			for (; xMax > 0; xMax--) {
				*pDest = (byte)color;
				pDest += 2;
			}
		}

		pDestLine += screenWidth;
	}
}

void Renderer::svgaPolyGouraud(int16 vtop, int16 Ymax) const {
	const int screenWidth = _engine->width();
	int16 xMin, xMax;
	int16 y = vtop;
	int16 start, end, step;
	byte *pDestLine = (uint8 *)_engine->_frontVideoBuffer.getBasePtr(0, y);
	byte *pDest;
	int16 *pVerticG = &_tabVerticG[y];
	int16 *pVerticD = &_tabVerticD[y];
	int16 *pCoulG = &_tabCoulG[y];
	int16 *pCoulD = &_tabCoulD[y];

	for (; y <= Ymax; y++) {
		xMin = *pVerticG++;
		xMax = *pVerticD++;
		start = *pCoulG++;
		end = *pCoulD++;
		pDest = pDestLine + xMin;

		xMax -= xMin;

		if (xMax == 0) {
			*pDest = (byte)((end + start) >> 9);
		} else if (xMax <= 2) {
			pDest[xMax--] = (byte)(end >> 8);
			if (xMax) {
				pDest[xMax--] = (byte)((end + start) >> 9);
			}
			*pDest = (byte)(start >> 8);
		} else {
			step = (end - start) / xMax;

			for (; xMax >= 0; xMax--) {
				*pDest++ = (byte)(start >> 8);
				start += step;
			}
		}

		pDestLine += screenWidth;
	}
}

// used for the most of the heads of the characters and the horse body
void Renderer::svgaPolyDith(int16 vtop, int16 Ymax) const {
	const int screenWidth = _engine->width();
	int16 xMin, xMax;
	int16 y = vtop;
	int16 start, end, step, delta, impair;
	byte *pDestLine = (uint8 *)_engine->_frontVideoBuffer.getBasePtr(0, y);
	byte *pDest;
	int16 *pVerticG = &_tabVerticG[y];
	int16 *pVerticD = &_tabVerticD[y];
	int16 *pCoulG = &_tabCoulG[y];
	int16 *pCoulD = &_tabCoulD[y];

	for (; y <= Ymax; y++) {
		xMin = *pVerticG++;
		xMax = *pVerticD++;
		start = *pCoulG++;
		end = *pCoulD++;
		pDest = pDestLine + xMin;

		xMax -= xMin;
		delta = end - start;

		if (xMax == 0) {
			// rcr ax,1
			*pDest = (byte)(((int32)end + start) >> 9);
		} else if (xMax <= 2) {
			step = start;

			if (xMax == 2) // if( !(xMax & 1) )
			{
				delta = (delta >> 1) | (delta & 0x8000); // sar ax,1
				step &= 0xFF;
				step = start + ROL8(step, 1);
				*pDest++ = (byte)(step >> 8);
				start += delta;
			}

			step = start + (step & 0xFF);
			*pDest++ = (byte)(step >> 8);

			start += delta;
			step &= 0xFF;
			step = start + ROL8(step, 1);
			*pDest = (byte)(step >> 8);
		} else {
			delta /= xMax;
			step = start;
			impair = xMax & 1;
			xMax = (xMax + 1) >> 1;

			if (!impair) {
				step &= 0xFF;
				step = start + ROL8(step, xMax & 7);
				*pDest++ = (byte)(step >> 8);
				start += delta;
			}

			for (; xMax > 0; xMax--) {
				step &= 0xFF;
				step += start;
				*pDest++ = (byte)(step >> 8);
				start += delta;
				step &= 0xFF;
				step = start + ROL8(step, xMax & 7);
				*pDest++ = (byte)(step >> 8);
				start += delta;
			}
		}

		pDestLine += screenWidth;
	}
}

void Renderer::svgaPolyMarbre(int16 vtop, int16 Ymax, uint16 color) const {
	const int screenWidth = _engine->width();
	int16 xMin, xMax;
	int16 y = vtop;
	byte *pDestLine = (uint8 *)_engine->_frontVideoBuffer.getBasePtr(0, y);
	byte *pDest;
	int16 *pVerticG = &_tabVerticG[y];
	int16 *pVerticD = &_tabVerticD[y];

	// color contains 2 colors: 0xFF start, 0xFF00 end
	uint16 start = (color & 0xFF) << 8;
	uint16 end = color & 0xFF00;
	uint16 delta = end - start + 1; // delta intensity
	int32 step, dc;

	for (; y <= Ymax; y++) {
		xMin = *pVerticG++;
		xMax = *pVerticD++;
		pDest = pDestLine + xMin;

		dc = xMax - xMin;
		if (dc == 0) {
			// just one
			*pDest++ = (byte)(end >> 8);
		} else if (dc > 0) {
			step = delta / (dc + 1);
			color = start;

			for (; xMin <= xMax; xMin++) {
				*pDest++ = (byte)(color >> 8);
				color += step;
			}
		}

		pDestLine += screenWidth;
	}
}

void Renderer::svgaPolyTriche(int16 vtop, int16 Ymax, uint16 color) const {
	const int screenWidth = _engine->width();
	int16 xMin, xMax;
	int16 y = vtop;
	byte *pDestLine = (uint8 *)_engine->_frontVideoBuffer.getBasePtr(0, y);
	byte *pDest;
	int16 *pVerticG = &_tabVerticG[y];
	int16 *pVerticD = &_tabVerticD[y];
	int16 *pCoulG = &_tabCoulG[y];

	for (; y <= Ymax; y++) {
		xMin = *pVerticG++;
		xMax = *pVerticD++;
		pDest = pDestLine + xMin;

		color = (*pCoulG++) >> 8;
		for (; xMin <= xMax; xMin++) {
			*pDest++ = (byte)color;
		}

		pDestLine += screenWidth;
	}
}

void Renderer::renderPolygons(const CmdRenderPolygon &polygon, ComputedVertex *vertices) {
	int16 vtop, vbottom;
	uint8 renderType = polygon.renderType;
	if (computePoly(renderType, vertices, polygon.numVertices, vtop, vbottom)) {
		clearPolySpans(vtop, vbottom);
		fillVertices(vtop, vbottom, renderType, polygon.colorIndex);
	}
}

bool Renderer::computeTexturedPoly(int16 polyRenderType, const ComputedVertex *screenVerts, const ComputedVertex *texVerts, int32 numVertices, int16 &vtop, int16 &vbottom, ComputedVertex *&outScreen, ComputedVertex *&outTex, int32 &outCount) {
	assert(numVertices < ARRAYSIZE(_clippedPolygonVertices1));
	for (int32 i = 0; i < numVertices; ++i) {
		_clippedPolygonVertices1[i] = screenVerts[i];
		_clippedTexCoords1[i] = texVerts[i];
	}

	ComputedVertex *offTabPoly[] = {_clippedPolygonVertices1, _clippedPolygonVertices2};
	ComputedVertex *offTabTexPoly[] = {_clippedTexCoords1, _clippedTexCoords2};

	outCount = computePolyMinMax(polyRenderType, offTabPoly, numVertices, vtop, vbottom, offTabTexPoly);
	if (outCount == 0) {
		return false;
	}

	outScreen = offTabPoly[0];
	outTex = offTabTexPoly[0];

	if (usesGouraudShade(polyRenderType)) {
		ComputedVertex *pTabPoly = outScreen;
		ComputedVertex *p0;
		ComputedVertex *p1;
		int16 *pCoul;
		int32 incY = -1;
		int32 dy, dc;
		int32 step, reminder;
		int32 edgeCount = outCount;

		for (; edgeCount > 0; --edgeCount, pTabPoly++) {
			p0 = pTabPoly;
			p1 = p0 + 1;

			dy = p1->y - p0->y;
			if (dy == 0) {
				continue;
			} else if (dy > 0) {
				if (p0->x <= p1->x) {
					incY = 1;
				} else {
					p0 = p1;
					p1 = pTabPoly;
					incY = -1;
				}
				pCoul = &_tabCoulG[p0->y];
			} else {
				dy = -dy;
				if (p0->x <= p1->x) {
					p0 = p1;
					p1 = pTabPoly;
					incY = 1;
				} else {
					incY = -1;
				}
				pCoul = &_tabCoulD[p0->y];
			}

			dc = (p1->intensity - p0->intensity) << 8;
			step = dc / dy;
			reminder = ((((dc % dy) >> 1) + 0x7F) & 0xFF) | (p0->intensity << 8);

			for (int32 y = dy; y >= 0; --y) {
				*pCoul = (int16)reminder;
				pCoul += incY;
				reminder += step;
			}
		}
	}

	return true;
}

void Renderer::renderTexturedTriangle(const ComputedVertex screenCoords[3], const ComputedVertex texCoords[3], uint8 renderType, const uint8 *texture, int16 flatShade, uint16 repMask, const int32 perspW[3]) {
	const bool perspective = perspW != nullptr && usesPerspectiveTexture(renderType);
	const int16 baseRenderType = baseTextureRenderType(renderType);

	int16 vtop = 0;
	int16 vbottom = 0;
	ComputedVertex *screenVerts = nullptr;
	ComputedVertex *texVerts = nullptr;
	int32 clippedCount = 0;
	if (!computeTexturedPoly(baseRenderType, screenCoords, texCoords, 3, vtop, vbottom, screenVerts, texVerts, clippedCount)) {
		return;
	}

	int32 lymin = vtop;
	int32 lymax = vbottom;

	if (perspective && clippedCount == 3) {
		clearPolySpans(lymin, lymax);
		for (int32 i = 0; i < clippedCount; ++i) {
			fillHolomapTrianglesPersp(screenVerts[i], screenVerts[i + 1], texVerts[i], texVerts[i + 1], perspW[i], perspW[i + 1], lymin, lymax);
		}
		fillBodyTextPolyNoClip(lymin, lymax, texture, baseRenderType, flatShade, repMask, true);
		return;
	}

	clearPolySpans(lymin, lymax);
	for (int32 i = 0; i < clippedCount; ++i) {
		fillHolomapTriangles(screenVerts[i], screenVerts[i + 1], texVerts[i], texVerts[i + 1], lymin, lymax);
	}

	fillBodyTextPolyNoClip(lymin, lymax, texture, baseRenderType, flatShade, repMask, false);
}

void Renderer::renderTexturedPolygons(const CmdRenderTexturedPolygon &polygon, ComputedVertex *screenVerts, ComputedVertex *texVerts, const int32 *perspW) {
	const uint8 *texture = _engine->_resources->getBodyTexture().getAtOffset(polygon.textureOffset);
	if (!texture) {
		texture = _engine->_resources->getBodyTexture().getPage(0);
	}
	if (!texture) {
		return;
	}

	if (polygon.numVertices == 3) {
		renderTexturedTriangle(screenVerts, texVerts, polygon.renderType, texture, polygon.pad, polygon.repMask, perspW);
	} else if (polygon.numVertices == 4) {
		const int32 w0 = perspW ? perspW[0] : 0;
		const int32 w1 = perspW ? perspW[1] : 0;
		const int32 w2 = perspW ? perspW[2] : 0;
		const int32 w3 = perspW ? perspW[3] : 0;
		const int32 tri0W[3] = {w0, w1, w2};
		const int32 tri1W[3] = {w0, w2, w3};
		const ComputedVertex tri0Screen[3] = {screenVerts[0], screenVerts[1], screenVerts[2]};
		const ComputedVertex tri1Screen[3] = {screenVerts[0], screenVerts[2], screenVerts[3]};
		const ComputedVertex tri0Tex[3] = {texVerts[0], texVerts[1], texVerts[2]};
		const ComputedVertex tri1Tex[3] = {texVerts[0], texVerts[2], texVerts[3]};
		renderTexturedTriangle(tri0Screen, tri0Tex, polygon.renderType, texture, polygon.pad, polygon.repMask, perspW ? tri0W : nullptr);
		renderTexturedTriangle(tri1Screen, tri1Tex, polygon.renderType, texture, polygon.pad, polygon.repMask, perspW ? tri1W : nullptr);
	}
}

void Renderer::fillBodyTextPolyNoClip(int32 yMin, int32 yMax, const uint8 *texture, uint8 renderType, int16 flatShade, uint16 repMask, bool perspective) {
	if (yMin < 0 || yMin >= _engine->_frontVideoBuffer.h) {
		return;
	}
	const int screenWidth = _engine->width();
	byte *pDestLine = (byte *)_engine->_frontVideoBuffer.getBasePtr(0, yMin);

	const int16 *pVerticG = &_tabVerticG[yMin];
	const int16 *pVerticD = &_tabVerticD[yMin];
	const uint16 *pU0 = (const uint16 *)&_tabMapU0[yMin];
	const uint16 *pV0 = (const uint16 *)&_tabMapV0[yMin];
	const uint16 *pU1 = (const uint16 *)&_tabMapU1[yMin];
	const uint16 *pV1 = (const uint16 *)&_tabMapV1[yMin];
	const int16 *pCoulG = &_tabCoulG[yMin];
	const int16 *pCoulD = &_tabCoulD[yMin];
	const bool gouraud = renderType == POLYGONTYPE_TEXTURE_GOURAUD;
	const bool flat = renderType == POLYGONTYPE_TEXTURE_FLAT;

	if (perspective) {
		const int32 *pUw0 = &_tabPerspUW0[yMin];
		const int32 *pVw0 = &_tabPerspVW0[yMin];
		const int32 *pW0 = &_tabPerspW0[yMin];
		const int32 *pUw1 = &_tabPerspUW1[yMin];
		const int32 *pVw1 = &_tabPerspVW1[yMin];
		const int32 *pW1 = &_tabPerspW1[yMin];

		yMax -= yMin;
		for (; yMax >= 0; yMax--) {
			int16 xMin = *pVerticG++;
			int16 xMax = *pVerticD++;
			xMax -= xMin;

			int64 uw = *pUw0++;
			int64 vw = *pVw0++;
			int64 w = *pW0++;
			const int64 uw1 = *pUw1++;
			const int64 vw1 = *pVw1++;
			const int64 w1 = *pW1++;
			int32 light = flat ? (flatShade << 8) : (gouraud ? *pCoulG++ : 0);
			int32 lightEnd = gouraud ? *pCoulD++ : light;

			if (xMax > 0) {
				byte *pDest = pDestLine + xMin;
				int64 uwstep = (uw1 - uw + xMax / 2) / xMax;
				int64 vwstep = (vw1 - vw + xMax / 2) / xMax;
				int64 wstep = (w1 - w + xMax / 2) / xMax;
				int32 lightStep = gouraud ? (lightEnd - light) / xMax : 0;

				for (; xMax > 0; xMax--) {
					if (w != 0) {
						const int32 u = (int32)(uw / w);
						const int32 v = (int32)(vw / w);
						const uint32 idx = ((u >> 8) & (repMask & 0xFF)) | (v & ((repMask >> 8) << 8));
						const byte texel = texture[idx];
						if (texel != 0) {
							if (renderType == POLYGONTYPE_TEXTURE) {
								*pDest = texel;
							} else {
								*pDest = shadeTexturedPixel(texel, (int16)light);
							}
						}
					}
					++pDest;
					uw += uwstep;
					vw += vwstep;
					w += wstep;
					light += lightStep;
				}
			} else if (xMax == 0) {
				byte *pDest = pDestLine + xMin;
				if (w != 0) {
					const int32 u = (int32)(uw / w);
					const int32 v = (int32)(vw / w);
					const uint32 idx = ((u >> 8) & (repMask & 0xFF)) | (v & ((repMask >> 8) << 8));
					const byte texel = texture[idx];
					if (texel != 0) {
						if (renderType == POLYGONTYPE_TEXTURE) {
							*pDest = texel;
						} else {
							*pDest = shadeTexturedPixel(texel, (int16)light);
						}
					}
				}
			}

			pDestLine += screenWidth;
		}
		return;
	}

	yMax -= yMin;

	for (; yMax >= 0; yMax--) {
		int16 xMin = *pVerticG++;
		int16 xMax = *pVerticD++;
		xMax -= xMin;

		uint32 u0, v0;
		int32 u, v;
		u = u0 = *pU0++;
		v = v0 = *pV0++;
		uint32 u1 = *pU1++;
		uint32 v1 = *pV1++;
		int32 light = flat ? (flatShade << 8) : (gouraud ? *pCoulG++ : 0);
		int32 lightEnd = gouraud ? *pCoulD++ : light;

		if (xMax > 0) {
			byte *pDest = pDestLine + xMin;

			int32 ustep = ((int32)u1 - (int32)u0 + 1) / xMax;
			int32 vstep = ((int32)v1 - (int32)v0 + 1) / xMax;
			int32 lightStep = gouraud ? (lightEnd - light) / xMax : 0;

			for (; xMax > 0; xMax--) {
				const uint32 idx = ((u >> 8) & (repMask & 0xFF)) | (v & ((repMask >> 8) << 8));
				const byte texel = texture[idx];
				if (texel != 0) {
					if (renderType == POLYGONTYPE_TEXTURE) {
						*pDest = texel;
					} else {
						*pDest = shadeTexturedPixel(texel, (int16)light);
					}
				}
				++pDest;

				u += ustep;
				v += vstep;
				light += lightStep;
			}
		} else if (xMax == 0) {
			byte *pDest = pDestLine + xMin;
			const uint32 idx = ((u >> 8) & (repMask & 0xFF)) | (v & ((repMask >> 8) << 8));
			const byte texel = texture[idx];
			if (texel != 0) {
				if (renderType == POLYGONTYPE_TEXTURE) {
					*pDest = texel;
				} else {
					*pDest = shadeTexturedPixel(texel, (int16)light);
				}
			}
		}

		pDestLine += screenWidth;
	}
}

void Renderer::fillVertices(int16 vtop, int16 vbottom, uint8 renderType, uint16 color) {
	switch (renderType) {
	case POLYGONTYPE_FLAT:
		svgaPolyTriste(vtop, vbottom, color);
		break;
	case POLYGONTYPE_TELE:
		if (_engine->_cfgfile.PolygonDetails == 0) {
			svgaPolyTriste(vtop, vbottom, color);
		} else {
			svgaPolyTele(vtop, vbottom, color);
		}
		break;
	case POLYGONTYPE_COPPER:
		svgaPolyCopper(vtop, vbottom, color);
		break;
	case POLYGONTYPE_BOPPER:
		svgaPolyBopper(vtop, vbottom, color);
		break;
	case POLYGONTYPE_TRANS:
		svgaPolyTrans(vtop, vbottom, color);
		break;
	case POLYGONTYPE_TRAME: // raster
		svgaPolyTrame(vtop, vbottom, color);
		break;
	case POLYGONTYPE_GOURAUD:
		if (_engine->_cfgfile.PolygonDetails == 0) {
			svgaPolyTriche(vtop, vbottom, color);
		} else {
			svgaPolyGouraud(vtop, vbottom);
		}
		break;
	case POLYGONTYPE_DITHER:
		if (_engine->_cfgfile.PolygonDetails == 0) {
			svgaPolyTriche(vtop, vbottom, color);
		} else if (_engine->_cfgfile.PolygonDetails == 1) {
			svgaPolyGouraud(vtop, vbottom);
		} else {
			svgaPolyDith(vtop, vbottom);
		}
		break;
	case POLYGONTYPE_MARBLE:
		svgaPolyMarbre(vtop, vbottom, color);
		break;
	default:
		warning("RENDER WARNING: Unsupported render type %d", renderType);
		break;
	}
}

bool Renderer::computeSphere(int32 x, int32 y, int32 radius, int &vtop, int &vbottom) {
	if (radius <= 0) {
		return false;
	}
	int16 left = (int16)(x - radius);
	int16 right = (int16)(x + radius);
	int16 bottom = (int16)(y + radius);
	int16 top = (int16)(y - radius);
	const Common::Rect &clip = _engine->_interface->_clip;
	int16 cleft = clip.left;
	int16 cright = clip.right;
	int16 ctop = clip.top;
	int16 cbottom = clip.bottom;

	if (left <= cright && right >= cleft && bottom <= cbottom && top >= ctop) {
		if (left < cleft) {
			left = cleft;
		}
		if (bottom > cbottom) {
			bottom = cbottom;
		}
		if (right > cright) {
			right = cright;
		}
		if (top < ctop) {
			top = ctop;
		}

		int32 r = 0;
		int32 acc = -radius;

		while (r <= radius) {
			int32 x1 = x - radius;
			if (x1 < cleft) {
				x1 = cleft;
			}

			int32 x2 = x + radius;
			if (x2 > cright) {
				x2 = cright;
			}

			int32 ny = y - r;
			if ((ny >= ctop) && (ny <= cbottom)) {
				_tabVerticG[ny] = (int16)x1;
				_tabVerticD[ny] = (int16)x2;
			}

			ny = y + r;
			if ((ny >= ctop) && (ny <= cbottom)) {
				_tabVerticG[ny] = (int16)x1;
				_tabVerticD[ny] = (int16)x2;
			}

			if (acc < 0) {
				acc += r;
				if (acc >= 0) {
					x1 = x - r;
					if (x1 < cleft) {
						x1 = cleft;
					}

					x2 = x + r;
					if (x2 > cright) {
						x2 = cright;
					}

					ny = y - radius;
					if ((ny >= ctop) && (ny <= cbottom)) {
						_tabVerticG[ny] = (int16)x1;
						_tabVerticD[ny] = (int16)x2;
					}

					ny = y + radius;
					if ((ny >= ctop) && (ny <= cbottom)) {
						_tabVerticG[ny] = (int16)x1;
						_tabVerticD[ny] = (int16)x2;
					}

					--radius;
					acc -= radius;
				}
			}

			++r;
		}

		vtop = top;
		vbottom = bottom;

		return true;
	}

	return false;
}

uint8 *Renderer::prepareSpheres(const Common::Array<BodySphere> &spheres, int32 &numOfPrimitives, RenderCommand **renderCmds, uint8 *renderBufferPtr, ModelData *modelData) {
	for (const BodySphere &sphere : spheres) {
		CmdRenderSphere *cmd = (CmdRenderSphere *)(void*)renderBufferPtr;
		cmd->color = sphere.color;
		cmd->polyRenderType = sphere.fillType;
		cmd->radius = sphere.radius;
		const int16 centerIndex = sphere.vertex;
		cmd->x = modelData->flattenPoints[centerIndex].x;
		cmd->y = modelData->flattenPoints[centerIndex].y;
		cmd->z = modelData->flattenPoints[centerIndex].z;

		(*renderCmds)->depth = modelData->flattenPoints[centerIndex].z;
		(*renderCmds)->renderType = RENDERTYPE_DRAWSPHERE;
		(*renderCmds)->dataPtr = renderBufferPtr;
		(*renderCmds)++;

		renderBufferPtr += sizeof(CmdRenderSphere);
	}
	numOfPrimitives += spheres.size();
	return renderBufferPtr;
}

uint8 *Renderer::prepareLines(const Common::Array<BodyLine> &lines, int32 &numOfPrimitives, RenderCommand **renderCmds, uint8 *renderBufferPtr, ModelData *modelData) {
	const uint8 *const bufferEnd = _renderCoordinatesBuffer + sizeof(_renderCoordinatesBuffer);
	const Common::Rect &clip = _engine->_interface->_clip;

	for (const BodyLine &line : lines) {
		const int32 point1Index = line.vertex1;
		const int32 point2Index = line.vertex2;
		if (point1Index < 0 || point2Index < 0 || point1Index >= 800 || point2Index >= 800) {
			continue;
		}

		const I16Vec3 &p1 = modelData->flattenPoints[point1Index];
		const I16Vec3 &p2 = modelData->flattenPoints[point2Index];

		// Skip degenerate or off-screen lines (behind-camera verts project to -32768)
		if (p1.x <= -30000 || p1.y <= -30000 || p2.x <= -30000 || p2.y <= -30000) {
			continue;
		}
		if (p1.x < clip.left - 200 && p2.x < clip.left - 200) {
			continue;
		}
		if (p1.x > clip.right + 200 && p2.x > clip.right + 200) {
			continue;
		}
		if (p1.y < clip.top - 200 && p2.y < clip.top - 200) {
			continue;
		}
		if (p1.y > clip.bottom + 200 && p2.y > clip.bottom + 200) {
			continue;
		}

		if (renderBufferPtr + (int32)sizeof(CmdRenderLine) > bufferEnd) {
			break;
		}

		if (numOfPrimitives >= (int32)ARRAYSIZE(_renderCmds)) {
			break;
		}

		CmdRenderLine *cmd = (CmdRenderLine *)(void*)renderBufferPtr;
		cmd->colorIndex = line.color;
		cmd->x1 = p1.x;
		cmd->y1 = p1.y;
		cmd->x2 = p2.x;
		cmd->y2 = p2.y;
		(*renderCmds)->depth = MAX(p1.z, p2.z);
		(*renderCmds)->renderType = RENDERTYPE_DRAWLINE;
		(*renderCmds)->dataPtr = renderBufferPtr;
		(*renderCmds)++;

		renderBufferPtr += sizeof(CmdRenderLine);
		numOfPrimitives++;
	}
	return renderBufferPtr;
}

uint8 *Renderer::preparePolygons(const BodyData &bodyData, int32 &numOfPrimitives, RenderCommand **renderCmds, uint8 *renderBufferPtr, ModelData *modelData) {
	const Common::Array<BodyPolygon> &polygons = bodyData.getPolygons();
	const bool hasBodyTexture = _engine->_resources->getBodyTexture().pageCount() > 0;
	const uint8 *const bufferEnd = _renderCoordinatesBuffer + sizeof(_renderCoordinatesBuffer);
	const bool usePerspectiveW = _typeProj == TYPE_3D;

	for (const BodyPolygon &polygon : polygons) {
		uint8 materialType = polygon.materialType;
		if (materialType == MAT_TEXTURE && polygon.hasTexture && !hasBodyTexture) {
			materialType = MAT_GOURAUD;
		}
		const uint8 numVertices = polygon.indices.size();
		assert(numVertices <= 16);

		if (polygon.isEnvironment && hasBodyTexture) {
			const uint8 renderType = mapLba2TextureRenderType(polygon.polyType);
			const uint32 textureInfo = bodyData.getTextureHandle(polygon.textureIndex);
			const uint16 textureOffset = (uint16)(textureInfo & 0xffff);
			const uint16 repMask = (uint16)(textureInfo >> 16);

			int16 flatShade = 0;
			if (baseTextureRenderType(renderType) == POLYGONTYPE_TEXTURE_FLAT) {
				const int32 lightIdx = (int32)polygon.normalIndex;
				if (lightIdx >= 0 && lightIdx < (int32)ARRAYSIZE(modelData->normalTable)) {
					flatShade = (int16)(modelData->normalTable[lightIdx] >> 8);
				}
			}

			int16 zMax = -32000;
			CmdRenderTexturedPolygon *destinationPolygon = (CmdRenderTexturedPolygon *)(void *)renderBufferPtr;
			destinationPolygon->renderType = renderType;
			destinationPolygon->numVertices = numVertices;
			destinationPolygon->textureIndex = (uint8)polygon.textureIndex;
			destinationPolygon->textureOffset = textureOffset;
			destinationPolygon->repMask = repMask;
			destinationPolygon->pad = (uint8)(flatShade & 0xff);

			renderBufferPtr += sizeof(CmdRenderTexturedPolygon);

			ComputedVertex *screenVertices = (ComputedVertex *)(void *)renderBufferPtr;
			renderBufferPtr += numVertices * sizeof(ComputedVertex);
			ComputedVertex *texVertices = (ComputedVertex *)(void *)renderBufferPtr;
			renderBufferPtr += numVertices * sizeof(ComputedVertex);
			int32 *perspW = nullptr;
			if (usePerspectiveW && usesPerspectiveTexture(renderType)) {
				if (renderBufferPtr + (int32)(sizeof(CmdRenderTexturedPolygon) + numVertices * (sizeof(ComputedVertex) * 2 + sizeof(int32))) > bufferEnd) {
					break;
				}
				perspW = (int32 *)(void *)renderBufferPtr;
				renderBufferPtr += numVertices * sizeof(int32);
			}

			for (int k = 0; k < numVertices; ++k) {
				const uint16 vertexIndex = polygon.indices[k];
				const I16Vec3 *point = &modelData->flattenPoints[vertexIndex];
				const I16Vec3 *rotPoint = &modelData->computedPoints[vertexIndex];

				screenVertices[k].x = point->x;
				screenVertices[k].y = point->y;
				zMax = MAX<int16>(zMax, point->z);

				if (baseTextureRenderType(renderType) == POLYGONTYPE_TEXTURE_GOURAUD) {
					screenVertices[k].intensity = lba2VertexColour(modelData->normalTable, polygon, vertexIndex);
				} else if (baseTextureRenderType(renderType) == POLYGONTYPE_TEXTURE_FLAT) {
					screenVertices[k].intensity = flatShade;
				} else {
					screenVertices[k].intensity = 0;
				}

				if (vertexIndex < bodyData.getNormals().size()) {
					const BodyNormal &normal = bodyData.getNormal((int16)vertexIndex);
					const uint16 boneGrp = normal.prenormalizedRange;
					const IMatrix3x3 &boneMatrix = boneGrp < ARRAYSIZE(_matricesTable) ? _matricesTable[boneGrp] : _matricesTable[0];
					const IVec3 rotated = rot(boneMatrix, normal.x, normal.y, normal.z);
					mapEnvTextureUV(texVertices[k].x, texVertices[k].y, buildEnvMappedUv(rotated, polygon.envScale));
				}

				if (perspW != nullptr) {
					perspW[k] = computePerspectiveW(rotPoint->z, _modelPosWr.z);
				}
			}

			if (!isPolygonVisible(screenVertices)) {
				renderBufferPtr = (uint8 *)destinationPolygon;
				continue;
			}

			if (numOfPrimitives >= (int32)ARRAYSIZE(_renderCmds)) {
				break;
			}
			numOfPrimitives++;
			(*renderCmds)->depth = zMax;
			(*renderCmds)->renderType = RENDERTYPE_DRAWTEXTUREDPOLYGON;
			(*renderCmds)->dataPtr = (uint8 *)destinationPolygon;
			(*renderCmds)++;
			continue;
		}

		if (materialType == MAT_TEXTURE && polygon.hasTexture && hasBodyTexture) {
			const uint8 renderType = mapLba2TextureRenderType(polygon.polyType);
			const uint32 textureInfo = bodyData.getTextureHandle(polygon.textureIndex);
			const uint16 textureOffset = (uint16)(textureInfo & 0xffff);
			const uint16 repMask = (uint16)(textureInfo >> 16);

			int16 flatShade = 0;
			if (baseTextureRenderType(renderType) == POLYGONTYPE_TEXTURE_FLAT) {
				const int32 lightIdx = (int32)polygon.normalIndex;
				if (lightIdx >= 0 && lightIdx < (int32)ARRAYSIZE(modelData->normalTable)) {
					flatShade = (int16)(modelData->normalTable[lightIdx] >> 8);
				}
			}

			const int numTris = (numVertices == 4) ? 2 : 1;
			static const uint8 triIndices[2][3] = {{0, 1, 2}, {0, 2, 3}};

			for (int tri = 0; tri < numTris; ++tri) {
				const int triVerts = 3;
				if (renderBufferPtr + (int32)(sizeof(CmdRenderTexturedPolygon) + triVerts * (sizeof(ComputedVertex) * 2 + (usePerspectiveW && usesPerspectiveTexture(renderType) ? sizeof(int32) : 0))) > bufferEnd) {
					break;
				}
				int16 zMax = -32000;
				CmdRenderTexturedPolygon *destinationPolygon = (CmdRenderTexturedPolygon *)(void *)renderBufferPtr;
				destinationPolygon->renderType = renderType;
				destinationPolygon->numVertices = triVerts;
				destinationPolygon->textureIndex = (uint8)polygon.textureIndex;
				destinationPolygon->textureOffset = textureOffset;
				destinationPolygon->repMask = repMask;
				destinationPolygon->pad = (uint8)(flatShade & 0xff);

				renderBufferPtr += sizeof(CmdRenderTexturedPolygon);

				ComputedVertex *screenVertices = (ComputedVertex *)(void *)renderBufferPtr;
				renderBufferPtr += triVerts * sizeof(ComputedVertex);
				ComputedVertex *texVertices = (ComputedVertex *)(void *)renderBufferPtr;
				renderBufferPtr += triVerts * sizeof(ComputedVertex);
				int32 *perspW = nullptr;
				if (usePerspectiveW && usesPerspectiveTexture(renderType)) {
					perspW = (int32 *)(void *)renderBufferPtr;
					renderBufferPtr += triVerts * sizeof(int32);
				}

				for (int k = 0; k < triVerts; ++k) {
					const uint8 corner = triIndices[tri][k];
					const uint16 vertexIndex = polygon.indices[corner];
					const I16Vec3 *point = &modelData->flattenPoints[vertexIndex];
					const I16Vec3 *rotPoint = &modelData->computedPoints[vertexIndex];

					screenVertices[k].x = point->x;
					screenVertices[k].y = point->y;
					zMax = MAX<int16>(zMax, point->z);

					if (baseTextureRenderType(renderType) == POLYGONTYPE_TEXTURE_GOURAUD) {
						screenVertices[k].intensity = lba2VertexColour(modelData->normalTable, polygon, vertexIndex);
					} else if (baseTextureRenderType(renderType) == POLYGONTYPE_TEXTURE_FLAT) {
						screenVertices[k].intensity = flatShade;
					} else {
						screenVertices[k].intensity = 0;
					}

					mapBodyTextureUV(texVertices[k].x, texVertices[k].y, polygon.u[corner], polygon.v[corner], _engine->isLBA2());

					if (perspW != nullptr) {
						perspW[k] = computePerspectiveW(rotPoint->z, _modelPosWr.z);
					}
				}

				if (!isPolygonVisible(screenVertices)) {
					renderBufferPtr = (uint8 *)destinationPolygon;
					continue;
				}

				if (numOfPrimitives >= (int32)ARRAYSIZE(_renderCmds)) {
					break;
				}
				numOfPrimitives++;
				(*renderCmds)->depth = zMax;
				(*renderCmds)->renderType = RENDERTYPE_DRAWTEXTUREDPOLYGON;
				(*renderCmds)->dataPtr = (uint8 *)destinationPolygon;
				(*renderCmds)++;
			}
			continue;
		}

		int16 zMax = -32000;

		if (renderBufferPtr + (int32)(sizeof(CmdRenderPolygon) + numVertices * sizeof(ComputedVertex)) > bufferEnd) {
			break;
		}

		CmdRenderPolygon *destinationPolygon = (CmdRenderPolygon *)(void*)renderBufferPtr;
		destinationPolygon->numVertices = numVertices;

		renderBufferPtr += sizeof(CmdRenderPolygon);

		ComputedVertex *const vertices = (ComputedVertex *)(void*)renderBufferPtr;

		renderBufferPtr += destinationPolygon->numVertices * sizeof(ComputedVertex);

		ComputedVertex *vertex = vertices;

		if (materialType >= MAT_GOURAUD) {
			destinationPolygon->renderType = polygon.materialType - (MAT_GOURAUD - MAT_FLAT);
			destinationPolygon->colorIndex = lba2BaseColour(polygon);

			for (int16 idx = 0; idx < numVertices; ++idx) {
				const uint16 vertexIndex = polygon.indices[idx];
				const I16Vec3 *point = &modelData->flattenPoints[vertexIndex];
				int16 shadeValue;
				if (_engine->isLBA2() || polygon.normals.empty()) {
					shadeValue = lba2VertexColour(modelData->normalTable, polygon, vertexIndex);
				} else {
					const uint16 shadeEntry = polygon.normals[idx];
					shadeValue = polygon.intensity + modelData->normalTable[shadeEntry];
				}

				vertex->intensity = shadeValue;
				vertex->x = point->x;
				vertex->y = point->y;
				zMax = MAX(zMax, point->z);
				++vertex;
			}
		} else {
			if (materialType >= MAT_FLAT) {
				// only 1 shade value is used
				destinationPolygon->renderType = materialType - MAT_FLAT;
				int16 shadeValue;
				if (_engine->isLBA2() || polygon.normals.empty()) {
					const uint8 baseColour = lba2BaseColour(polygon);
					const uint16 lightIdx = polygon.normalIndex;
					const uint16 light = lightIdx < ARRAYSIZE(modelData->normalTable) ? modelData->normalTable[lightIdx] : 0;
					shadeValue = (baseColour + (light >> 8)) & 0xff;
				} else {
					shadeValue = polygon.intensity;
					if (!polygon.normals.empty()) {
						shadeValue += modelData->normalTable[polygon.normals[0]];
					} else if (numVertices > 0) {
						shadeValue += modelData->normalTable[polygon.indices[0]];
					}
				}
				destinationPolygon->colorIndex = shadeValue;
			} else {
				// no shade is used
				destinationPolygon->renderType = materialType;
				destinationPolygon->colorIndex = polygon.intensity;
			}

			for (int16 idx = 0; idx < numVertices; ++idx) {
				const uint16 vertexIndex = polygon.indices[idx];
				const I16Vec3 *point = &modelData->flattenPoints[vertexIndex];

				vertex->intensity = destinationPolygon->colorIndex;
				vertex->x = point->x;
				vertex->y = point->y;
				zMax = MAX<int16>(zMax, point->z);
				++vertex;
			}
		}

		if (!isPolygonVisible(vertices)) {
			renderBufferPtr = (uint8 *)destinationPolygon;
			continue;
		}

		numOfPrimitives++;

		if (numOfPrimitives >= (int32)ARRAYSIZE(_renderCmds)) {
			break;
		}
		(*renderCmds)->depth = zMax;
		(*renderCmds)->renderType = RENDERTYPE_DRAWPOLYGON;
		(*renderCmds)->dataPtr = (uint8 *)destinationPolygon;
		(*renderCmds)++;
	}

	return renderBufferPtr;
}

const Renderer::RenderCommand *Renderer::depthSortRenderCommands(int32 numOfPrimitives, const BodyData &bodyData) {
	if (!bodyData.noSort) {
		// AFF_OBJ.CPP: painter's algorithm (back-to-front) unless a z-buffer is active.
		// ScummVM has no polygon z-buffer, so always sort descending depth like QuickSort().
		(void)bodyData;
		Common::sort(&_renderCmds[0], &_renderCmds[numOfPrimitives], [](const RenderCommand &lhs, const RenderCommand &rhs) { return lhs.depth > rhs.depth; });
	}
	return _renderCmds;
}

bool Renderer::renderObjectIso(const BodyData &bodyData, RenderCommand **renderCmds, ModelData *modelData, Common::Rect &modelRect) {
	int32 numOfPrimitives = 0;
	uint8 *renderBufferPtr = _renderCoordinatesBuffer;
	renderBufferPtr = preparePolygons(bodyData, numOfPrimitives, renderCmds, renderBufferPtr, modelData);
	renderBufferPtr = prepareLines(bodyData.getLines(), numOfPrimitives, renderCmds, renderBufferPtr, modelData);
	prepareSpheres(bodyData.getSpheres(), numOfPrimitives, renderCmds, renderBufferPtr, modelData);

	if (numOfPrimitives == 0) {
		return false;
	}
	const RenderCommand *cmds = depthSortRenderCommands(numOfPrimitives, bodyData);

	int32 primitiveCounter = numOfPrimitives;

	do {
		int16 type = cmds->renderType;
		uint8 *pointer = cmds->dataPtr;

		switch (type) {
		case RENDERTYPE_DRAWLINE: {
			const CmdRenderLine *lineCoords = (const CmdRenderLine *)(const void*)pointer;
			int32 x1 = lineCoords->x1;
			int32 y1 = lineCoords->y1;
			int32 x2 = lineCoords->x2;
			int32 y2 = lineCoords->y2;
			const Common::Rect &clip = _engine->_interface->_clip;
			if (x1 < clip.left - 500 && x2 < clip.left - 500) {
				break;
			}
			if (x1 > clip.right + 500 && x2 > clip.right + 500) {
				break;
			}
			if (y1 < clip.top - 500 && y2 < clip.top - 500) {
				break;
			}
			if (y1 > clip.bottom + 500 && y2 > clip.bottom + 500) {
				break;
			}
			x1 = CLIP(x1, (int32)clip.left, (int32)clip.right);
			x2 = CLIP(x2, (int32)clip.left, (int32)clip.right);
			y1 = CLIP(y1, (int32)clip.top, (int32)clip.bottom);
			y2 = CLIP(y2, (int32)clip.top, (int32)clip.bottom);
			_engine->_interface->drawLine(x1, y1, x2, y2, lineCoords->colorIndex);
			break;
		}
		case RENDERTYPE_DRAWPOLYGON: {
			const CmdRenderPolygon *header = (const CmdRenderPolygon *)(const void*)pointer;
			ComputedVertex *vertices = (ComputedVertex *)(void*)(pointer + sizeof(CmdRenderPolygon));
			renderPolygons(*header, vertices);
			break;
		}
		case RENDERTYPE_DRAWTEXTUREDPOLYGON: {
			const CmdRenderTexturedPolygon *header = (const CmdRenderTexturedPolygon *)(const void *)pointer;
			ComputedVertex *screenVertices = (ComputedVertex *)(void *)(pointer + sizeof(CmdRenderTexturedPolygon));
			ComputedVertex *texVertices = screenVertices + header->numVertices;
			const int32 *perspW = nullptr;
			if (header->renderType >= POLYGONTYPE_TEXTURE_PERSP && header->renderType <= POLYGONTYPE_TEXTURE_FLAT_PERSP) {
				perspW = (const int32 *)(const void *)(texVertices + header->numVertices);
			}
			renderTexturedPolygons(*header, screenVertices, texVertices, perspW);
			break;
		}
		case RENDERTYPE_DRAWSPHERE: {
			const CmdRenderSphere *sphere = (const CmdRenderSphere *)(const void*)pointer;
			int32 radius = sphere->radius;

			if (_typeProj == TYPE_ISO) {
				// * sqrt(sx+sy) / 512 (isometric scale)
				radius = (radius * 34) / ISO_SCALE;
			} else {
				int32 delta = _kFactor + sphere->z;
				if (delta == 0) {
					break;
				}
				radius = (sphere->radius * _lFactorX) / delta;
			}

			if (sphere->x + radius > modelRect.right) {
				modelRect.right = sphere->x + radius;
			}

			if (sphere->x - radius < modelRect.left) {
				modelRect.left = sphere->x - radius;
			}

			if (sphere->y + radius > modelRect.bottom) {
				modelRect.bottom = sphere->y + radius;
			}

			if (sphere->y - radius < modelRect.top) {
				modelRect.top = sphere->y - radius;
			}

			int vtop = -1;
			int vbottom = -1;
			if (computeSphere(sphere->x, sphere->y, radius, vtop, vbottom)) {
				fillVertices(vtop, vbottom, sphere->polyRenderType, sphere->color);
			}
			break;
		}
		default:
			break;
		}

		cmds++;
	} while (--primitiveCounter);
	return true;
}

void Renderer::projectModelPoints(ModelData *modelData, int32 numVertices, const IVec3 &poswr, Common::Rect &modelRect) {
	const I16Vec3 *pointPtr = &modelData->computedPoints[0];
	I16Vec3 *pointPtrDest = &modelData->flattenPoints[0];

	if (_typeProj == TYPE_ISO) {
		for (int32 i = 0; i < numVertices; ++i) {
			const int32 coX = pointPtr->x + poswr.x;
			const int32 coY = pointPtr->y + poswr.y;
			const int32 coZ = -(pointPtr->z + poswr.z);

			pointPtrDest->x = (int16)((coX + coZ) * 24 / ISO_SCALE + _projectionCenter.x);
			pointPtrDest->y = (int16)((((coX - coZ) * 12) - coY * 30) / ISO_SCALE + _projectionCenter.y + 1);
			pointPtrDest->z = (int16)(coZ - coX - coY);

			if (pointPtrDest->x < modelRect.left) {
				modelRect.left = pointPtrDest->x;
			}
			if (pointPtrDest->x > modelRect.right) {
				modelRect.right = pointPtrDest->x;
			}
			if (pointPtrDest->y < modelRect.top) {
				modelRect.top = pointPtrDest->y;
			}
			if (pointPtrDest->y > modelRect.bottom) {
				modelRect.bottom = pointPtrDest->y;
			}

			++pointPtr;
			++pointPtrDest;
		}
	} else {
		for (int32 i = 0; i < numVertices; ++i) {
			int32 coZ = _kFactor - (pointPtr->z + poswr.z);
			if (coZ <= 0) {
				pointPtrDest->x = -32768;
				pointPtrDest->y = -32768;
				pointPtrDest->z = -32768;
				++pointPtr;
				++pointPtrDest;
				continue;
			}

			int32 coX = (((pointPtr->x + poswr.x) * _lFactorX) / coZ) + _projectionCenter.x;
			if (coX > 0xFFFF) {
				coX = 0x7FFF;
			}
			pointPtrDest->x = (int16)coX;
			if (pointPtrDest->x < modelRect.left) {
				modelRect.left = pointPtrDest->x;
			}
			if (pointPtrDest->x > modelRect.right) {
				modelRect.right = pointPtrDest->x;
			}

			int32 coY = _projectionCenter.y - (((pointPtr->y + poswr.y) * _lFactorY) / coZ);
			if (coY > 0xFFFF) {
				coY = 0x7FFF;
			}
			pointPtrDest->y = (int16)coY;
			if (pointPtrDest->y < modelRect.top) {
				modelRect.top = pointPtrDest->y;
			}
			if (pointPtrDest->y > modelRect.bottom) {
				modelRect.bottom = pointPtrDest->y;
			}

			if (coZ > 0xFFFF) {
				coZ = 0x7FFF;
			}
			pointPtrDest->z = (int16)coZ;

			++pointPtr;
			++pointPtrDest;
		}
	}
}

void Renderer::applyBodyLighting(ModelData *modelData, const BodyData &bodyData, bool perBone) {
	memset(modelData->normalTable, 0, sizeof(modelData->normalTable));

	const bool lba2Format = _engine->isLBA2();
	const int32 numVertices = (int32)bodyData.getNumVertices();
	const Common::Array<BodyNormal> &normFaces = bodyData.getNormFaces();
	int32 faceNormalIndex = 0;

	if (!perBone) {
		const IMatrix3x3 &matrix = lba2Format ? _matricesTable[0] : (_matricesTable[0] * _normalLight);
		for (int32 i = 0; i < (int32)bodyData.getNormals().size() && i < (int32)ARRAYSIZE(modelData->normalTable); ++i) {
			modelData->normalTable[i] = computeNormalLight(bodyData.getNormal(i), matrix, _normalLight, lba2Format);
		}
		for (int32 i = 0; i < (int32)normFaces.size(); ++i) {
			const int32 lightIdx = numVertices + i;
			if (lightIdx < (int32)ARRAYSIZE(modelData->normalTable)) {
				modelData->normalTable[lightIdx] = computeNormalLight(normFaces[i], matrix, _normalLight, lba2Format);
			}
		}
		return;
	}

	const int32 numBones = (int32)bodyData.getNumBones();
	int32 vertexNormalOffset = 0;
	for (int32 boneIdx = 0; boneIdx < numBones; ++boneIdx) {
		const BodyBone &bone = bodyData.getBone(boneIdx);
		const IMatrix3x3 &matrix = lba2Format ? _matricesTable[boneIdx] : (_matricesTable[boneIdx] * _normalLight);

		for (int32 i = 0; i < bone.numVertices; ++i) {
			const int32 vtx = bone.firstVertex + i;
			const int32 normalIdx = vertexNormalOffset + i;
			if (vtx >= 0 && vtx < (int32)ARRAYSIZE(modelData->normalTable) &&
			    normalIdx >= 0 && normalIdx < (int32)bodyData.getNormals().size()) {
				modelData->normalTable[vtx] = computeNormalLight(bodyData.getNormal(normalIdx), matrix, _normalLight, lba2Format);
			}
		}
		vertexNormalOffset += bone.numVertices;

		for (int32 i = 0; i < bone.numNormals; ++i) {
			if (faceNormalIndex >= (int32)normFaces.size()) {
				break;
			}
			const int32 lightIdx = numVertices + faceNormalIndex;
			if (lightIdx < (int32)ARRAYSIZE(modelData->normalTable)) {
				modelData->normalTable[lightIdx] = computeNormalLight(normFaces[faceNormalIndex], matrix, _normalLight, lba2Format);
			}
			++faceNormalIndex;
		}
	}
}

void Renderer::displayStaticModel(ModelData *modelData, const BodyData &bodyData, const IVec3 &angleVec, const IVec3 &poswr, Common::Rect &modelRect) {
	const int32 numVertices = (int32)bodyData.getNumVertices();
	const Common::Array<BodyVertex> &vertices = bodyData.getVertices();

	rotMatIndex2(&_matricesTable[0], &_matrixWorld, angleVec);
	if (_engine->isLBA2()) {
		rotTransList(vertices, 0, numVertices, &modelData->computedPoints[0], &_matricesTable[0], IVec3(0, 0, 0));
	} else {
		rotList(vertices, 0, numVertices, &modelData->computedPoints[0], &_matricesTable[0], IVec3(0, 0, 0));
	}
	projectModelPoints(modelData, numVertices, poswr, modelRect);
	applyBodyLighting(modelData, bodyData, false);
}

void Renderer::animModel(ModelData *modelData, const BodyData &bodyData, RenderCommand *renderCmds, const IVec3 &angleVec, const IVec3 &poswr, Common::Rect &modelRect) {
	const int32 numVertices = bodyData.getNumVertices();
	const int32 numBones = bodyData.getNumBones();

	const Common::Array<BodyVertex> &vertices = bodyData.getVertices();

	IMatrix3x3 *modelMatrix = &_matricesTable[0];

	const BodyBone &firstBone = bodyData.getBone(0);
	processRotatedElement(modelMatrix, 0, vertices, angleVec.x, angleVec.y, angleVec.z, firstBone, modelData);

	int32 numOfPrimitives = 0;

	if (numBones - 1 != 0) {
		numOfPrimitives = numBones - 1;
		int boneIdx = 1;
		modelMatrix = &_matricesTable[boneIdx];

		do {
			const BodyBone &bone = bodyData.getBone(boneIdx);
			const BoneFrame *boneData = bodyData.getBoneState(boneIdx);

			const uint16 boneType = (uint16)boneData->type;
			if (_engine->isLBA2()) {
				if (boneType & (uint16)BoneType::TYPE_TRANSLATE) {
					translateGroup(modelMatrix, boneIdx, vertices, boneData->x, boneData->y, boneData->z, bone, modelData);
				} else {
					processRotatedElement(modelMatrix, boneIdx, vertices, boneData->x, boneData->y, boneData->z, bone, modelData);
				}
			} else if (isAnimRotateBone(boneType, false)) {
				processRotatedElement(modelMatrix, boneIdx, vertices, boneData->x, boneData->y, boneData->z, bone, modelData);
			} else if (isAnimTranslateBone(boneType, false)) {
				translateGroup(modelMatrix, boneIdx, vertices, boneData->x, boneData->y, boneData->z, bone, modelData);
			} else if (boneType == (uint16)BoneType::TYPE_ZOOM) {
				zoomGroup(modelMatrix, boneIdx, bone, boneData, modelData);
			}

			++modelMatrix;
			++boneIdx;
		} while (--numOfPrimitives);
	}

	projectModelPoints(modelData, numVertices, poswr, modelRect);
	applyBodyLighting(modelData, bodyData, true);
}

bool Renderer::affObjetIso(int32 x, int32 y, int32 z, int32 alpha, int32 beta, int32 gamma, const BodyData &bodyData, Common::Rect &modelRect) {
	IVec3 renderAngle;
	renderAngle.x = alpha;
	renderAngle.y = beta;
	renderAngle.z = gamma;

	// model render size reset
	modelRect.left = SCENE_SIZE_MAX;
	modelRect.top = SCENE_SIZE_MAX;
	modelRect.right = SCENE_SIZE_MIN;
	modelRect.bottom = SCENE_SIZE_MIN;

	IVec3 poswr; // PosXWr, PosYWr, PosZWr
	if (_typeProj == TYPE_3D) {
		poswr = longWorldRot(x, y, z) - _cameraRot;
	} else {
		poswr.x = x;
		poswr.y = y;
		poswr.z = z;
	}

	_modelPosWr = poswr;
	_activeModelData = &_modelData;

	RenderCommand *renderCmds = _renderCmds;
	if (!bodyData.isAnimated() && bodyData.getNumBones() <= 1) {
		displayStaticModel(&_modelData, bodyData, renderAngle, poswr, modelRect);
	} else {
		animModel(&_modelData, bodyData, renderCmds, renderAngle, poswr, modelRect);
	}
	if (!renderObjectIso(bodyData, &renderCmds, &_modelData, modelRect)) {
		modelRect.right = -1;
		modelRect.bottom = -1;
		modelRect.left = -1;
		modelRect.top = -1;
		return false;
	}
	return true;
}

bool Renderer::affObjetIsoAlphaBeta(int32 x, int32 y, int32 z, int32 alpha, int32 beta, int32 gamma, const BodyData &bodyData, Common::Rect &modelRect) {
	const IMatrix3x3 savedWorld = _matrixWorld;
	IMatrix3x3 betaGammaWorld;
	rotMatIndex2(&betaGammaWorld, &savedWorld, IVec3(0, beta, gamma));
	_matrixWorld = betaGammaWorld;
	const bool result = affObjetIso(x, y, z, alpha, 0, 0, bodyData, modelRect);
	_matrixWorld = savedWorld;
	return result;
}

void Renderer::drawObj3D(const Common::Rect &rect, int32 y, int32 angle, const BodyData &bodyData, RealValue &move) {
	int32 boxLeft = rect.left;
	int32 boxTop = rect.top;
	int32 boxRight = rect.right;
	int32 boxBottom = rect.bottom;
	const int32 ypos = (boxBottom + boxTop) / 2;
	const int32 xpos = (boxRight + boxLeft) / 2;

	setIsoProjection(xpos, ypos, 0);
	_engine->_interface->setClip(rect);

	Common::Rect dummy;
	if (angle == -1) {
		angle = move.getRealAngle(_engine->timerRef);
		if (move.timeValue == 0) {
			_engine->_movements->initRealAngle(angle, angle - LBAAngles::ANGLE_90, LBAAngles::ANGLE_17, &move);
		}
	}
	affObjetIso(0, y, 0, LBAAngles::ANGLE_0, angle, LBAAngles::ANGLE_0, bodyData, dummy);
}

void Renderer::draw3dObject(int32 x, int32 y, const BodyData &bodyData, int32 angle, int32 cameraZoom) {
	setProjection(x, y, 128, 200, 200);
	setFollowCamera(0, 0, 0, 60, 0, 0, cameraZoom);

	Common::Rect dummy;
	affObjetIso(0, 0, 0, LBAAngles::ANGLE_0, angle, LBAAngles::ANGLE_0, bodyData, dummy);
}

void Renderer::fillHolomapTriangle(int16 *pDest, int32 x0, int32 y0, int32 x1, int32 y1) {
	uint32 dx, step, reminder;
	if (y0 > y1) {
		SWAP(x0, x1);
		SWAP(y0, y1);
	}

	y1 -= y0;
	pDest += y0;

	if (x0 <= x1) {
		dx = (x1 - x0) << 16;

		step = dx / y1;
		reminder = ((dx % y1) >> 1) + 0x7FFF;

		x1 = step >> 16;
		step &= 0xFFFF;

		for (; y1 >= 0; --y1) {
			*pDest++ = (int16)x0;
			x0 += x1;
			if (reminder & 0xFFFF0000) {
				x0 += reminder >> 16;
				reminder &= 0xFFFF;
			}
			reminder += step;
		}
	} else {
		dx = (x0 - x1) << 16;

		step = dx / y1;
		reminder = ((dx % y1) >> 1) + 0x7FFF;

		x1 = step >> 16;
		step &= 0xFFFF;

		for (; y1 >= 0; --y1) {
			*pDest++ = (int16)x0;
			x0 -= x1;
			if (reminder & 0xFFFF0000) {
				x0 += reminder >> 16;
				reminder &= 0xFFFF;
			}
			reminder -= step;
		}
	}
}

void Renderer::fillHolomapTriangle32(int32 *pDest, int32 x0, int32 y0, int32 x1, int32 y1) {
	uint32 dx, step, reminder;
	if (y0 > y1) {
		SWAP(x0, x1);
		SWAP(y0, y1);
	}

	y1 -= y0;
	pDest += y0;

	if (y1 <= 0) {
		*pDest = x0;
		return;
	}

	if (x0 <= x1) {
		dx = (uint32)(x1 - x0) << 16;
		step = dx / (uint32)y1;
		reminder = ((dx % (uint32)y1) >> 1) + 0x7FFF;
		x1 = (int32)(step >> 16);
		step &= 0xFFFF;

		for (; y1 >= 0; --y1) {
			*pDest++ = x0;
			x0 += x1;
			if (reminder & 0xFFFF0000) {
				x0 += (int32)(reminder >> 16);
				reminder &= 0xFFFF;
			}
			reminder += step;
		}
	} else {
		dx = (uint32)(x0 - x1) << 16;
		step = dx / (uint32)y1;
		reminder = ((dx % (uint32)y1) >> 1) + 0x7FFF;
		x1 = (int32)(step >> 16);
		step &= 0xFFFF;

		for (; y1 >= 0; --y1) {
			*pDest++ = x0;
			x0 -= x1;
			if (reminder & 0xFFFF0000) {
				x0 += (int32)(reminder >> 16);
				reminder &= 0xFFFF;
			}
			reminder -= step;
		}
	}
}

void Renderer::fillHolomapTrianglesPersp(const ComputedVertex &vertex0, const ComputedVertex &vertex1, const ComputedVertex &texCoord0, const ComputedVertex &texCoord1, int32 w0, int32 w1, int32 &lymin, int32 &lymax) {
	const int32 y0 = vertex0.y;
	const int32 y1 = vertex1.y;
	const int64 uw0 = (int64)texCoord0.x * w0;
	const int64 vw0 = (int64)texCoord0.y * w0;
	const int64 uw1 = (int64)texCoord1.x * w1;
	const int64 vw1 = (int64)texCoord1.y * w1;

	if (y0 < y1) {
		if (y0 < lymin) {
			lymin = y0;
		}
		if (y1 > lymax) {
			lymax = y1;
		}
		fillHolomapTriangle(_tabVerticG, vertex0.x, y0, vertex1.x, y1);
		fillHolomapTriangle32(_tabPerspUW0, (int32)uw0, y0, (int32)uw1, y1);
		fillHolomapTriangle32(_tabPerspVW0, (int32)vw0, y0, (int32)vw1, y1);
		fillHolomapTriangle32(_tabPerspW0, w0, y0, w1, y1);
	} else if (y0 > y1) {
		if (y0 > lymax) {
			lymax = y0;
		}
		if (y1 < lymin) {
			lymin = y1;
		}
		fillHolomapTriangle(_tabVerticD, vertex0.x, y0, vertex1.x, y1);
		fillHolomapTriangle32(_tabPerspUW1, (int32)uw0, y0, (int32)uw1, y1);
		fillHolomapTriangle32(_tabPerspVW1, (int32)vw0, y0, (int32)vw1, y1);
		fillHolomapTriangle32(_tabPerspW1, w0, y0, w1, y1);
	}
}

void Renderer::fillHolomapTriangles(const ComputedVertex &vertex0, const ComputedVertex &vertex1, const ComputedVertex &texCoord0, const ComputedVertex &texCoord1, int32 &lymin, int32 &lymax) {
	const int32 y0 = vertex0.y;
	const int32 y1 = vertex1.y;

	if (y0 < y1) {
		if (y0 < lymin) {
			lymin = y0;
		}
		if (y1 > lymax) {
			lymax = y1;
		}
		fillHolomapTriangle(_tabVerticG, vertex0.x, y0, vertex1.x, y1);
		fillHolomapTriangle(_tabMapU0, (int32)(uint16)texCoord0.x, y0, (int32)(uint16)texCoord1.x, y1);
		fillHolomapTriangle(_tabMapV0, (int32)(uint16)texCoord0.y, y0, (int32)(uint16)texCoord1.y, y1);
	} else if (y0 > y1) {
		if (y0 > lymax) {
			lymax = y0;
		}
		if (y1 < lymin) {
			lymin = y1;
		}
		fillHolomapTriangle(_tabVerticD, vertex0.x, y0, vertex1.x, y1);
		fillHolomapTriangle(_tabMapU1, (int32)(uint16)texCoord0.x, y0, (int32)(uint16)texCoord1.x, y1);
		fillHolomapTriangle(_tabMapV1, (int32)(uint16)texCoord0.y, y0, (int32)(uint16)texCoord1.y, y1);
	}
}

void Renderer::asmTexturedTriangleNoClip(const ComputedVertex vertexCoordinates[3], const ComputedVertex textureCoordinates[3], const uint8 *holomapImage, uint32 holomapImageSize) {
	int32 lymin = 32000;
	int32 lymax = -32000;
	fillHolomapTriangles(vertexCoordinates[0], vertexCoordinates[1], textureCoordinates[0], textureCoordinates[1], lymin, lymax);
	fillHolomapTriangles(vertexCoordinates[1], vertexCoordinates[2], textureCoordinates[1], textureCoordinates[2], lymin, lymax);
	fillHolomapTriangles(vertexCoordinates[2], vertexCoordinates[0], textureCoordinates[2], textureCoordinates[0], lymin, lymax);
	fillTextPolyNoClip(lymin, lymax, holomapImage, holomapImageSize);
}

void Renderer::fillTextPolyNoClip(int32 yMin, int32 yMax, const uint8 *holomapImage, uint32 holomapImageSize) {
	if (yMin < 0 || yMin >= _engine->_frontVideoBuffer.h) {
		return;
	}
	const int screenWidth = _engine->width();
	uint8 *pDestLine = (uint8 *)_engine->_frontVideoBuffer.getBasePtr(0, yMin);

	const int16 *pVerticG = &_tabVerticG[yMin];
	const int16 *pVerticD = &_tabVerticD[yMin];
	const uint16 *pU0 = (const uint16 *)&_tabMapU0[yMin];
	const uint16 *pV0 = (const uint16 *)&_tabMapV0[yMin];
	const uint16 *pU1 = (const uint16 *)&_tabMapU1[yMin];
	const uint16 *pV1 = (const uint16 *)&_tabMapV1[yMin];

	yMax -= yMin;

	for (; yMax >= 0; yMax--) {
		int16 xMin = *pVerticG++;
		int16 xMax = *pVerticD++;
		xMax -= xMin;

		uint32 u0, v0;
		int32 u, v;
		u = u0 = *pU0++;
		v = v0 = *pV0++;
		uint32 u1 = *pU1++;
		uint32 v1 = *pV1++;

		if (xMax > 0) {
			byte *pDest = pDestLine + xMin;

			int32 ustep = ((int32)u1 - (int32)u0 + 1) / xMax;
			int32 vstep = ((int32)v1 - (int32)v0 + 1) / xMax;

			for (; xMax > 0; xMax--) {
				uint32 idx = ((u >> 8) & 0xFF) | (v & 0xFF00); // u0&0xFF00=column*256, v0&0xFF00 = line*256
				*pDest++ = holomapImage[idx];

				u += ustep;
				v += vstep;
			}
		}

		pDestLine += screenWidth;
	}
}

} // namespace TwinE
