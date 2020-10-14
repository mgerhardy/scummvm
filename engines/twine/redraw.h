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

#ifndef TWINE_REDRAW_H
#define TWINE_REDRAW_H

#include "common/scummsys.h"

namespace TwinE {

#define OVERLAY_MAX_ENTRIES 10

class TwinEEngine;
class Redraw {
private:
	TwinEEngine *_engine;
	typedef struct DrawListStruct {
		int16 posValue;
		uint16 index; // field_2
		uint16 X;
		uint16 Y;
		uint16 Z;
		uint16 field_A;
		uint16 field_C;
		uint16 field_E;
		uint16 field_10;
	} DrawListStruct;

	/** Draw list array to grab the necessary */
	DrawListStruct drawList[150];

	typedef struct RedrawStruct {
		uint16 left;
		uint16 top;
		uint16 right;
		uint16 bottom;
	} RedrawStruct;

	RedrawStruct currentRedrawList[300];
	RedrawStruct nextRedrawList[300];

	int16 overlayRotation = 0;
	void addRedrawCurrentArea(int32 left, int32 top, int32 right, int32 bottom);
	void moveNextAreas();
	void sortDrawingList(DrawListStruct *list, int32 listSize);
	void updateOverlayTypePosition(int16 X1, int16 Y1, int16 X2, int16 Y2);

public:
	Redraw(TwinEEngine *engine) : _engine(engine) {}

	/** Auxiliar object render left position on screen */
	int32 renderLeft;
	/** Auxiliar object render right position on screen */
	int32 renderRight;
	/** Auxiliar object render top position on screen */
	int32 renderTop;
	/** Auxiliar object render bottom position on screen */
	int32 renderBottom;

	int16 drawInGameTransBox;

	/** Request background redraw */
	int16 reqBgRedraw;

	/** Current number of redraw regions in the screen */
	int32 currNumOfRedrawBox; // fullRedrawVar8
	/** Number of redraw regions in the screen */
	int32 numOfRedrawBox;

	/** Save last actor that bubble dialog icon */
	int32 bubbleActor;
	int32 bubbleSpriteIndex;

	enum OverlayType {
		koSprite = 0,
		koNumber = 1,
		koNumberRange = 2,
		koInventoryItem = 3,
		koText = 4
	};

	enum OverlayPosType {
		koNormal = 0,
		koFollowActor = 1
	};

	/** Overlay list structure */
	typedef struct OverlayListStruct {
		int16 type;
		int16 info0; // sprite/3d model entry | number | number range
		int16 X;
		int16 Y;
		int16 info1; // followed actor | total coins
		int16 posType;
		int16 lifeTime;
	} OverlayListStruct;

	OverlayListStruct overlayList[OVERLAY_MAX_ENTRIES];

	void addOverlay(int16 type, int16 info0, int16 X, int16 Y, int16 info1, int16 posType, int16 lifeTime);

	/** Add a certain region to redraw list array
	@param left start width to redraw the region
	@param top start height to redraw the region
	@param right end width to redraw the region
	@param bottom end height to redraw the region */
	void addRedrawArea(int32 left, int32 top, int32 right, int32 bottom);

	/** Flip currentRedrawList regions in the screen

	This only updates small areas in the screen so few CPU processor is used */
	void flipRedrawAreas();

	/** Blit/Update all screen regions in the currentRedrawList */
	void blitBackgroundAreas();

	/** This is responsible for the entire game screen redraw
	@param bgRedraw true if we want to redraw background grid, false if we want to update certain screen areas */
	void redrawEngineActions(int32 bgRedraw);

	/** Draw dialogue sprite image */
	void drawBubble(int32 actorIdx);

	void zoomScreenScale();
};

} // namespace TwinE

#endif
