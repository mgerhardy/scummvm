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

#include "debug.scene.h"
#include "grid.h"
#include "interface.h"
#include "redraw.h"
#include "renderer.h"
#include "scene.h"
#include "twine.h"

namespace TwinE {

DebugScene::DebugScene(TwinEEngine *engine) : _engine(engine) {}

void DebugScene::drawBoundingBoxProjectPoints(ScenePoint *pPoint3d, ScenePoint *pPoint3dProjected) {
	projectPositionOnScreen(pPoint3d->X, pPoint3d->Y, pPoint3d->Z);

	pPoint3dProjected->X = projPosX;
	pPoint3dProjected->Y = projPosY;
	pPoint3dProjected->Z = projPosZ;

	if (renderLeft > projPosX)
		renderLeft = projPosX;

	if (renderRight < projPosX)
		renderRight = projPosX;

	if (renderTop > projPosY)
		renderTop = projPosY;

	if (renderBottom < projPosY)
		renderBottom = projPosY;
}

int32 DebugScene::checkZoneType(int32 type) {
	switch (type) {
	case 0:
		if (typeZones & 0x01)
			return 1;
		break;
	case 1:
		if (typeZones & 0x02)
			return 1;
		break;
	case 2:
		if (typeZones & 0x04)
			return 1;
		break;
	case 3:
		if (typeZones & 0x08)
			return 1;
		break;
	case 4:
		if (typeZones & 0x10)
			return 1;
		break;
	case 5:
		if (typeZones & 0x20)
			return 1;
		break;
	case 6:
		if (typeZones & 0x40)
			return 1;
		break;
	default:
		break;
	}

	return 0;
}

void DebugScene::displayZones(int16 pKey) {
	if (showingZones == 1) {
		int z;
		ZoneStruct *zonePtr = sceneZones;
		for (z = 0; z < sceneNumZones; z++) {
			zonePtr = &sceneZones[z];

			if (checkZoneType(zonePtr->type)) {
				ScenePoint frontBottomLeftPoint;
				ScenePoint frontBottomRightPoint;

				ScenePoint frontTopLeftPoint;
				ScenePoint frontTopRightPoint;

				ScenePoint backBottomLeftPoint;
				ScenePoint backBottomRightPoint;

				ScenePoint backTopLeftPoint;
				ScenePoint backTopRightPoint;

				ScenePoint frontBottomLeftPoint2D;
				ScenePoint frontBottomRightPoint2D;

				ScenePoint frontTopLeftPoint2D;
				ScenePoint frontTopRightPoint2D;

				ScenePoint backBottomLeftPoint2D;
				ScenePoint backBottomRightPoint2D;

				ScenePoint backTopLeftPoint2D;
				ScenePoint backTopRightPoint2D;

				uint8 color;

				// compute the points in 3D

				frontBottomLeftPoint.X = zonePtr->bottomLeft.X - _engine->_grid->cameraX;
				frontBottomLeftPoint.Y = zonePtr->bottomLeft.Y - _engine->_grid->cameraY;
				frontBottomLeftPoint.Z = zonePtr->topRight.Z - _engine->_grid->cameraZ;

				frontBottomRightPoint.X = zonePtr->topRight.X - _engine->_grid->cameraX;
				frontBottomRightPoint.Y = zonePtr->bottomLeft.Y - _engine->_grid->cameraY;
				frontBottomRightPoint.Z = zonePtr->topRight.Z - _engine->_grid->cameraZ;

				frontTopLeftPoint.X = zonePtr->bottomLeft.X - _engine->_grid->cameraX;
				frontTopLeftPoint.Y = zonePtr->topRight.Y - _engine->_grid->cameraY;
				frontTopLeftPoint.Z = zonePtr->topRight.Z - _engine->_grid->cameraZ;

				frontTopRightPoint.X = zonePtr->topRight.X - _engine->_grid->cameraX;
				frontTopRightPoint.Y = zonePtr->topRight.Y - _engine->_grid->cameraY;
				frontTopRightPoint.Z = zonePtr->topRight.Z - _engine->_grid->cameraZ;

				backBottomLeftPoint.X = zonePtr->bottomLeft.X - _engine->_grid->cameraX;
				backBottomLeftPoint.Y = zonePtr->bottomLeft.Y - _engine->_grid->cameraY;
				backBottomLeftPoint.Z = zonePtr->bottomLeft.Z - _engine->_grid->cameraZ;

				backBottomRightPoint.X = zonePtr->topRight.X - _engine->_grid->cameraX;
				backBottomRightPoint.Y = zonePtr->bottomLeft.Y - _engine->_grid->cameraY;
				backBottomRightPoint.Z = zonePtr->bottomLeft.Z - _engine->_grid->cameraZ;

				backTopLeftPoint.X = zonePtr->bottomLeft.X - _engine->_grid->cameraX;
				backTopLeftPoint.Y = zonePtr->topRight.Y - _engine->_grid->cameraY;
				backTopLeftPoint.Z = zonePtr->bottomLeft.Z - _engine->_grid->cameraZ;

				backTopRightPoint.X = zonePtr->topRight.X - _engine->_grid->cameraX;
				backTopRightPoint.Y = zonePtr->topRight.Y - _engine->_grid->cameraY;
				backTopRightPoint.Z = zonePtr->bottomLeft.Z - _engine->_grid->cameraZ;

				// project all points

				drawBoundingBoxProjectPoints(&frontBottomLeftPoint, &frontBottomLeftPoint2D);
				drawBoundingBoxProjectPoints(&frontBottomRightPoint, &frontBottomRightPoint2D);
				drawBoundingBoxProjectPoints(&frontTopLeftPoint, &frontTopLeftPoint2D);
				drawBoundingBoxProjectPoints(&frontTopRightPoint, &frontTopRightPoint2D);
				drawBoundingBoxProjectPoints(&backBottomLeftPoint, &backBottomLeftPoint2D);
				drawBoundingBoxProjectPoints(&backBottomRightPoint, &backBottomRightPoint2D);
				drawBoundingBoxProjectPoints(&backTopLeftPoint, &backTopLeftPoint2D);
				drawBoundingBoxProjectPoints(&backTopRightPoint, &backTopRightPoint2D);

				// draw all lines

				color = 15 * 3 + zonePtr->type * 16;

				// draw front part
				drawLine(frontBottomLeftPoint2D.X, frontBottomLeftPoint2D.Y, frontTopLeftPoint2D.X, frontTopLeftPoint2D.Y, color);
				drawLine(frontTopLeftPoint2D.X, frontTopLeftPoint2D.Y, frontTopRightPoint2D.X, frontTopRightPoint2D.Y, color);
				drawLine(frontTopRightPoint2D.X, frontTopRightPoint2D.Y, frontBottomRightPoint2D.X, frontBottomRightPoint2D.Y, color);
				drawLine(frontBottomRightPoint2D.X, frontBottomRightPoint2D.Y, frontBottomLeftPoint2D.X, frontBottomLeftPoint2D.Y, color);

				// draw top part
				drawLine(frontTopLeftPoint2D.X, frontTopLeftPoint2D.Y, backTopLeftPoint2D.X, backTopLeftPoint2D.Y, color);
				drawLine(backTopLeftPoint2D.X, backTopLeftPoint2D.Y, backTopRightPoint2D.X, backTopRightPoint2D.Y, color);
				drawLine(backTopRightPoint2D.X, backTopRightPoint2D.Y, frontTopRightPoint2D.X, frontTopRightPoint2D.Y, color);
				drawLine(frontTopRightPoint2D.X, frontTopRightPoint2D.Y, frontTopLeftPoint2D.X, frontTopLeftPoint2D.Y, color);

				// draw back part
				drawLine(backBottomLeftPoint2D.X, backBottomLeftPoint2D.Y, backTopLeftPoint2D.X, backTopLeftPoint2D.Y, color);
				drawLine(backTopLeftPoint2D.X, backTopLeftPoint2D.Y, backTopRightPoint2D.X, backTopRightPoint2D.Y, color);
				drawLine(backTopRightPoint2D.X, backTopRightPoint2D.Y, backBottomRightPoint2D.X, backBottomRightPoint2D.Y, color);
				drawLine(backBottomRightPoint2D.X, backBottomRightPoint2D.Y, backBottomLeftPoint2D.X, backBottomLeftPoint2D.Y, color);

				// draw bottom part
				drawLine(frontBottomLeftPoint2D.X, frontBottomLeftPoint2D.Y, backBottomLeftPoint2D.X, backBottomLeftPoint2D.Y, color);
				drawLine(backBottomLeftPoint2D.X, backBottomLeftPoint2D.Y, backBottomRightPoint2D.X, backBottomRightPoint2D.Y, color);
				drawLine(backBottomRightPoint2D.X, backBottomRightPoint2D.Y, frontBottomRightPoint2D.X, frontBottomRightPoint2D.Y, color);
				drawLine(frontBottomRightPoint2D.X, frontBottomRightPoint2D.Y, frontBottomLeftPoint2D.X, frontBottomLeftPoint2D.Y, color);
			}
		}
	}
}

} // namespace TwinE
