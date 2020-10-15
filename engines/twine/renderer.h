/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef TWINE_RENDERER_H
#define TWINE_RENDERER_H

#include "common/scummsys.h"

namespace TwinE {
class TwinEEngine;
class Renderer {
private:
	TwinEEngine *_engine;

	typedef struct renderTabEntry {
		int16 depth;
		int16 renderType;
		uint8 *dataPtr;
	} renderTabEntry;

	typedef struct pointTab {
		int16 X;
		int16 Y;
		int16 Z;
	} pointTab;

	typedef struct elementEntry {
		int16 firstPoint;  // data1
		int16 numOfPoints; // data2
		int16 basePoint;   // data3
		int16 baseElement; // param
		int16 flag;
		int16 rotateZ;
		int16 rotateY;
		int16 rotateX;
		int32 numOfShades; // field_10
		int32 field_14;
		int32 field_18;
		int32 Y;
		int32 field_20;
		int16 field_24;
	} elementEntry;

	typedef struct lineCoordinates {
		int32 data;
		int16 x1;
		int16 y1;
		int16 x2;
		int16 y2;
	} lineCoordinates;

	typedef struct lineData {
		int32 data;
		int16 p1;
		int16 p2;
	} lineData;

	typedef struct polyHeader {
		uint8 renderType; //FillVertic_AType
		uint8 numOfVertex;
		int16 colorIndex;
	} polyHeader;

	typedef struct polyVertexHeader {
		int16 shadeEntry;
		int16 dataOffset;
	} polyVertexHeader;

	typedef struct computedVertex {
		int16 shadeValue;
		int16 x;
		int16 y;
	} computedVertex;

	typedef struct bodyHeaderStruct {
		int16 bodyFlag;
		int16 unk0;
		int16 unk1;
		int16 unk2;
		int16 unk3;
		int16 unk4;
		int16 unk5;
		int16 offsetToData;
		int8 *ptrToKeyFrame;
		int32 keyFrameTime;
	} bodyHeaderStruct;

	typedef struct vertexData {
		uint8 param;
		int16 x;
		int16 y;
	} vertexData;

	typedef union packed16 {
		struct {
			uint8 al;
			uint8 ah;
		} bit;
		uint16 temp;
	} packed16;

	int32 renderAnimatedModel(uint8 *bodyPtr);
	void circleFill(int32 x, int32 y, int32 radius, int8 color);
	int32 renderModelElements(uint8 *pointer);
	void getBaseRotationPosition(int32 X, int32 Y, int32 Z);
	void getCameraAnglePositions(int32 X, int32 Y, int32 Z);
	void applyRotation(int32 *tempMatrix, int32 *currentMatrix);
	void applyPointsRotation(uint8 *firstPointsPtr, int32 numPoints, pointTab *destPoints, int32 *rotationMatrix);
	void processRotatedElement(int32 rotZ, int32 rotY, int32 rotX, elementEntry *elemPtr);
	void applyPointsTranslation(uint8 *firstPointsPtr, int32 numPoints, pointTab *destPoints, int32 *translationMatrix);
	void processTranslatedElement(int32 rotX, int32 rotY, int32 rotZ, elementEntry *elemPtr);
	void translateGroup(int16 ax, int16 bx, int16 cx);

	// ---- variables ----

	int32 baseMatrixRotationX;
	int32 baseMatrixRotationY;
	int32 baseMatrixRotationZ;

	int32 baseTransPosX; // setSomething2Var1
	int32 baseTransPosY; // setSomething2Var2
	int32 baseTransPosZ; // setSomething2Var3

	int32 baseRotPosX; // setSomething3Var12
	int32 baseRotPosY; // setSomething3Var14
	int32 baseRotPosZ; // setSomething3Var16

	int32 cameraPosX; // cameraVar1
	int32 cameraPosY; // cameraVar2
	int32 cameraPosZ; // cameraVar3

	// ---

	int32 renderAngleX; // _angleX
	int32 renderAngleY; // _angleY
	int32 renderAngleZ; // _angleZ

	int32 renderX; // _X
	int32 renderY; // _Y
	int32 renderZ; // _Z

	// ---

	int32 baseMatrix[3 * 3];

	int32 numOfPrimitives;

	int32 numOfPoints;
	int32 numOfElements;
	uint8 *pointsPtr;
	uint8 *elementsPtr;
	uint8 *elementsPtr2;

	uint8 *pri2Ptr2;

	int32 matricesTable[271];
	uint8 *currentMatrixTableEntry;

	int32 *shadePtr;
	int32 shadeMatrix[9];
	int32 lightX;
	int32 lightY;
	int32 lightZ;

	pointTab computedPoints[800]; // _projectedPointTable
	pointTab flattenPoints[800];  // _flattenPointTable
	int16 shadeTable[500];

	int16 primitiveCounter;
	renderTabEntry *renderTabEntryPtr;
	renderTabEntry *renderTabEntryPtr2;
	renderTabEntry *renderTabSortedPtr;

	renderTabEntry renderTab[1000];
	renderTabEntry renderTabSorted[1000];
	uint8 renderTab7[10000];

	uint8 *renderV19; // RECHECK THIS

	// render polygon vars
	int16 pRenderV3[96];
	int16 *pRenderV2;

	int16 vleft;
	int16 vtop;
	int16 vright;
	int16 vbottom;

	uint8 oldVertexParam;
	uint8 vertexParam1;
	uint8 vertexParam2;

	int16 polyTab[960];
	int16 polyTab2[960];
	int32 renderLoop;
	// end render polygon vars

public:
	Renderer(TwinEEngine *engine) : _engine(engine) {}

	int32 isUsingOrhoProjection;

	int16 projPosXScreen; // fullRedrawVar1
	int16 projPosYScreen; // fullRedrawVar2
	int16 projPosZScreen; // fullRedrawVar3
	int16 projPosX;
	int16 projPosY;
	int16 projPosZ;

	int32 orthoProjX; // setSomethingVar1
	int32 orthoProjY; // setSomethingVar2
	int32 orthoProjZ; // setSomethingVar2

	int32 destX;
	int32 destY;
	int32 destZ;

	const int16 *shadeAngleTab3; // tab3

	int16 polyRenderType; //FillVertic_AType;
	int32 numOfVertex;
	int16 vertexCoordinates[193];
	int16 *pRenderV1;

	void setLightVector(int32 angleX, int32 angleY, int32 angleZ);

	int32 computePolygons();
	void renderPolygons(int32 ecx, int32 edi);

	void prepareIsoModel(uint8 *bodyPtr); // loadGfxSub

	int32 projectPositionOnScreen(int32 cX, int32 cY, int32 cZ);
	void setCameraPosition(int32 X, int32 Y, int32 cX, int32 cY, int32 cZ);
	void setCameraAngle(int32 transPosX, int32 transPosY, int32 transPosZ, int32 rotPosX, int32 rotPosY, int32 rotPosZ, int32 param6);
	void setBaseTranslation(int32 X, int32 Y, int32 Z);
	void setBaseRotation(int32 X, int32 Y, int32 Z);
	void setOrthoProjection(int32 X, int32 Y, int32 Z);

	int32 renderIsoModel(int32 X, int32 Y, int32 Z, int32 angleX, int32 angleY, int32 angleZ, uint8 *bodyPtr);

	void copyActorInternAnim(uint8 *bodyPtrSrc, uint8 *bodyPtrDest);

	void renderBehaviourModel(int32 boxLeft, int32 boxTop, int32 boxRight, int32 boxBottom, int32 Y, int32 angle, uint8 *entityPtr);

	void renderInventoryItem(int32 X, int32 Y, uint8 *itemBodyPtr, int32 angle, int32 param);
};

} // namespace TwinE

#endif
