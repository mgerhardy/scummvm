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

#include <stdio.h>

#include "collision.h"
#include "common/util.h"
#include "extra.h"
#include "gamestate.h"
#include "grid.h"
#include "interface.h"
#include "movements.h"
#include "redraw.h"
#include "renderer.h"
#include "resources.h"
#include "scene.h"
#include "sound.h"
#include "twine.h"

namespace TwinE {

/** Hit Stars shape info */
static const int16 hitStarsShapeTable[] = {
    10,
    0,
    -20,
    4,
    -6,
    19,
    -6,
    7,
    2,
    12,
    16,
    0,
    7,
    -12,
    16,
    -7,
    2,
    -19,
    -6,
    -4,
    -6};

/** Explode Cloud shape info */
static const int16 explodeCloudShapeTable[] = {
    18,
    0,
    -20,
    6,
    -16,
    8,
    -10,
    14,
    -12,
    20,
    -4,
    18,
    4,
    12,
    4,
    16,
    8,
    8,
    16,
    2,
    12,
    -4,
    18,
    -10,
    16,
    -12,
    8,
    -16,
    10,
    -20,
    4,
    -12,
    -8,
    -6,
    -6,
    -10,
    -12};

Extra::Extra(TwinEEngine *engine) : _engine(engine) {}

int32 Extra::addExtra(int32 actorIdx, int32 X, int32 Y, int32 Z, int32 info0, int32 targetActor, int32 maxSpeed, int32 strengthOfHit) {
	int32 i;

	for (i = 0; i < EXTRA_MAX_ENTRIES; i++) {
		ExtraListStruct *extra = &extraList[i];
		if (extra->info0 == -1) {
			extra->info0 = info0;
			extra->type = 0x80;
			extra->info1 = 0;
			extra->X = X;
			extra->Y = Y;
			extra->Z = Z;
			extra->actorIdx = actorIdx;
			extra->lifeTime = targetActor;
			extra->destZ = maxSpeed;
			extra->strengthOfHit = strengthOfHit;

			_engine->_movements->setActorAngle(0, maxSpeed, 50, &extra->trackActorMove);
			extra->angle = _engine->_movements->getAngleAndSetTargetActorDistance(X, Z, sceneActors[targetActor].X, sceneActors[targetActor].Z);
			return i;
		}
	}
	return -1;
}

/** Add extra explosion
	@param X Explostion X coordinate
	@param Y Explostion Y coordinate
	@param Z Explostion Z coordinate */
int32 Extra::addExtraExplode(int32 X, int32 Y, int32 Z) {
	int32 i;

	for (i = 0; i < EXTRA_MAX_ENTRIES; i++) {
		ExtraListStruct *extra = &extraList[i];
		if (extra->info0 == -1) {
			extra->info0 = 0x61;
			extra->type = 0x1001;
			extra->info1 = 0;
			extra->X = X;
			extra->Y = Y;
			extra->Z = Z;
			extra->actorIdx = 0x28;
			extra->lifeTime = _engine->lbaTime;
			extra->strengthOfHit = 0;
			return i;
		}
	}
	return -1;
}

/** Reset all used extras */
void Extra::resetExtras() {
	int32 i;

	for (i = 0; i < EXTRA_MAX_ENTRIES; i++) {
		ExtraListStruct *extra = &extraList[i];
		extra->info0 = -1;
		extra->info1 = 1;
	}
}

void Extra::throwExtra(ExtraListStruct *extra, int32 var1, int32 var2, int32 var3, int32 var4) { // InitFly
	extra->type |= 2;

	extra->lastX = extra->X;
	extra->lastY = extra->Y;
	extra->lastZ = extra->Z;

	_engine->_movements->rotateActor(var3, 0, var1);

	extra->destY = -destZ;

	_engine->_movements->rotateActor(0, destX, var2);

	extra->destX = destX;
	extra->destZ = destZ;

	extra->angle = var4;
	extra->lifeTime = _engine->lbaTime;
}

void Extra::addExtraSpecial(int32 X, int32 Y, int32 Z, int32 type) { // InitSpecial
	int32 i;
	int16 flag = 0x8000 + type;

	for (i = 0; i < EXTRA_MAX_ENTRIES; i++) {
		ExtraListStruct *extra = &extraList[i];
		if (extra->info0 == -1) {
			extra->info0 = flag;
			extra->info1 = 0;

			if (type == kHitStars) {
				extra->type = 9;

				extra->X = X;
				extra->Y = Y;
				extra->Z = Z;

				// same as InitFly
				throwExtra(extra, _engine->getRandomNumber(0x100) + 0x80, _engine->getRandomNumber(0x400), 50, 20);

				extra->strengthOfHit = 0;
				extra->lifeTime = _engine->lbaTime;
				extra->actorIdx = 100;

				return;
			} else if (type == kExplodeCloud) {
				extra->type = 1;

				extra->X = X;
				extra->Y = Y;
				extra->Z = Z;

				extra->strengthOfHit = 0;
				extra->lifeTime = _engine->lbaTime;
				extra->actorIdx = 5;

				return;
			}
		}
	}
}

int32 Extra::addExtraBonus(int32 X, int32 Y, int32 Z, int32 param, int32 angle, int32 type, int32 bonusAmount) { // ExtraBonus
	int32 i;

	for (i = 0; i < EXTRA_MAX_ENTRIES; i++) {
		ExtraListStruct *extra = &extraList[i];
		if (extra->info0 == -1) {
			extra->info0 = type;
			extra->type = 0x4071;

			/*if(type == SPRITEHQR_KEY) {
				extra->type = 0x4030;
			}*/

			extra->X = X;
			extra->Y = Y;
			extra->Z = Z;

			// same as InitFly
			throwExtra(extra, param, angle, 40, 15);

			extra->strengthOfHit = 0;
			extra->lifeTime = _engine->lbaTime;
			extra->actorIdx = 1000;
			extra->info1 = bonusAmount;

			return i;
		}
	}

	return -1;
}

int32 Extra::addExtraThrow(int32 actorIdx, int32 X, int32 Y, int32 Z, int32 sprite, int32 var2, int32 var3, int32 var4, int32 var5, int32 strengthOfHit) { // ThrowExtra
	int32 i;

	for (i = 0; i < EXTRA_MAX_ENTRIES; i++) {
		ExtraListStruct *extra = &extraList[i];
		if (extra->info0 == -1) {
			extra->info0 = sprite;
			extra->type = 0x210C;
			extra->X = X;
			extra->Y = Y;
			extra->Z = Z;

			// same as InitFly
			throwExtra(extra, var2, var3, var4, var5);

			extra->strengthOfHit = strengthOfHit;
			extra->lifeTime = _engine->lbaTime;
			extra->actorIdx = actorIdx;
			extra->info1 = 0;

			return i;
		}
	}

	return -1;
}

int32 Extra::addExtraAiming(int32 actorIdx, int32 X, int32 Y, int32 Z, int32 spriteIdx, int32 targetActorIdx, int32 maxSpeed, int32 strengthOfHit) { // ExtraSearch
	int32 i;

	for (i = 0; i < EXTRA_MAX_ENTRIES; i++) {
		ExtraListStruct *extra = &extraList[i];
		if (extra->info0 == -1) {
			extra->info0 = spriteIdx;
			extra->type = 0x80;
			extra->info1 = 0;
			extra->X = X;
			extra->Y = Y;
			extra->Z = Z;
			extra->actorIdx = actorIdx;
			extra->lifeTime = targetActorIdx;
			extra->destZ = maxSpeed;
			extra->strengthOfHit = strengthOfHit;
			_engine->_movements->setActorAngle(0, maxSpeed, 50, &extra->trackActorMove);
			extra->angle = _engine->_movements->getAngleAndSetTargetActorDistance(X, Z, sceneActors[targetActorIdx].X, sceneActors[targetActorIdx].Z);

			return i;
		}
	}

	return -1;
}

// cseg01:00018168
int32 Extra::findExtraKey() {
	int32 i;

	for (i = 0; i < EXTRA_MAX_ENTRIES; i++) {
		ExtraListStruct *extra = &extraList[i];
		if (extra->info0 == SPRITEHQR_KEY) {
			return i;
		}
	}

	return -1;
}

// cseg01:00018250
int32 Extra::addExtraAimingAtKey(int32 actorIdx, int32 X, int32 Y, int32 Z, int32 spriteIdx, int32 extraIdx) { // addMagicBallAimingAtKey
	int32 i;

	for (i = 0; i < EXTRA_MAX_ENTRIES; i++) {
		ExtraListStruct *extra = &extraList[i];
		if (extra->info0 == -1) {
			extra->info0 = spriteIdx;
			extra->type = 0x200;
			extra->info1 = 0;
			extra->X = X;
			extra->Y = Y;
			extra->Z = Z;
			extra->actorIdx = extraIdx;
			extra->destZ = 0x0FA0;
			extra->strengthOfHit = 0;
			_engine->_movements->setActorAngle(0, 0x0FA0, 50, &extra->trackActorMove);
			extra->angle = _engine->_movements->getAngleAndSetTargetActorDistance(X, Z, extraList[extraIdx].X, extraList[extraIdx].Z);

			return i;
		}
	}

	return -1;
}

void Extra::addExtraThrowMagicball(int32 X, int32 Y, int32 Z, int32 param1, int32 angle, int32 param2, int32 param3) { // ThrowMagicBall
	int32 ballSprite = -1;
	int32 ballStrength = 0;
	int32 extraIdx = -1;

	switch (_engine->_gameState->magicLevelIdx) {
	case 0:
	case 1:
		ballSprite = 1;
		ballStrength = 4;
		break;
	case 2:
		ballSprite = 42;
		ballStrength = 6;
		break;
	case 3:
		ballSprite = 43;
		ballStrength = 8;
		break;
	case 4:
		ballSprite = 13;
		ballStrength = 10;
		break;
	}

	_engine->_gameState->magicBallNumBounce = ((_engine->_gameState->inventoryMagicPoints - 1) / 20) + 1;
	if (_engine->_gameState->inventoryMagicPoints == 0) {
		_engine->_gameState->magicBallNumBounce = 0;
	}

	extraIdx = findExtraKey();
	if (extraIdx != -1) { // there is a key to aim
		_engine->_gameState->magicBallNumBounce = 5;
	}

	switch (_engine->_gameState->magicBallNumBounce) {
	case 0:
		_engine->_gameState->magicBallIdx = addExtraThrow(0, X, Y, Z, ballSprite, param1, angle, param2, param3, ballStrength);
		break;
	case 1:
		_engine->_gameState->magicBallAuxBounce = 4;
		_engine->_gameState->magicBallIdx = addExtraThrow(0, X, Y, Z, ballSprite, param1, angle, param2, param3, ballStrength);
		break;
	case 2:
	case 3:
	case 4:
		_engine->_gameState->magicBallNumBounce = 1;
		_engine->_gameState->magicBallAuxBounce = 4;
		_engine->_gameState->magicBallIdx = addExtraThrow(0, X, Y, Z, ballSprite, param1, angle, param2, param3, ballStrength);
		break;
	case 5:
		_engine->_gameState->magicBallIdx = addExtraAimingAtKey(0, X, Y, Z, ballSprite, extraIdx);
		break;
	}

	if (_engine->_gameState->inventoryMagicPoints > 0) {
		_engine->_gameState->inventoryMagicPoints--;
	}
}

void Extra::drawSpecialShape(const int16 *shapeTable, int32 X, int32 Y, int32 color, int32 angle, int32 size) {
	int16 currentShapeTable;
	int16 var_8;
	int16 temp1;
	int32 computedX;
	int32 computedY;
	int32 oldComputedX;
	int32 oldComputedY;
	int32 numEntries;
	int32 currentX;
	int32 currentY;

	currentShapeTable = *(shapeTable++);

	var_8 = ((*(shapeTable++)) * size) >> 4;
	temp1 = ((*(shapeTable++)) * size) >> 4;

	renderLeft = 0x7D00;
	renderRight = -0x7D00;
	renderTop = 0x7D00;
	renderBottom = -0x7D00;

	_engine->_movements->rotateActor(var_8, temp1, angle);

	computedX = destX + X;
	computedY = destZ + Y;

	if (computedX < renderLeft)
		renderLeft = computedX;

	if (computedX > renderRight)
		renderRight = computedX;

	if (computedY < renderTop)
		renderTop = computedY;

	if (computedY > renderBottom)
		renderBottom = computedY;

	numEntries = 1;

	currentX = computedX;
	currentY = computedY;

	while (numEntries < currentShapeTable) {
		var_8 = ((*(shapeTable++)) * size) >> 4;
		temp1 = ((*(shapeTable++)) * size) >> 4;

		oldComputedX = currentX;
		oldComputedY = currentY;

		projPosX = currentX;
		projPosY = currentY;

		_engine->_movements->rotateActor(var_8, temp1, angle);

		currentX = destX + X;
		currentY = destZ + Y;

		if (currentX < renderLeft)
			renderLeft = currentX;

		if (currentX > renderRight)
			renderRight = currentX;

		if (currentY < renderTop)
			renderTop = currentY;

		if (currentY > renderBottom)
			renderBottom = currentY;

		projPosX = currentX;
		projPosY = currentY;

		drawLine(oldComputedX, oldComputedY, currentX, currentY, color);

		numEntries++;

		currentX = projPosX;
		currentY = projPosY;
	}

	projPosX = currentX;
	projPosY = currentY;
	drawLine(currentX, currentY, computedX, computedY, color);
}

void Extra::drawExtraSpecial(int32 extraIdx, int32 X, int32 Y) {
	int32 specialType;
	ExtraListStruct *extra = &extraList[extraIdx];

	specialType = extra->info0 & 0x7FFF;

	switch (specialType) {
	case kHitStars:
		drawSpecialShape(hitStarsShapeTable, X, Y, 15, (_engine->lbaTime << 5) & 0x300, 4);
		break;
	case kExplodeCloud: {
		int32 cloudTime = 1 + _engine->lbaTime - extra->lifeTime;

		if (cloudTime > 32) {
			cloudTime = 32;
		}

		drawSpecialShape(explodeCloudShapeTable, X, Y, 15, 0, cloudTime);
	} break;
	}
}

void Extra::processMagicballBounce(ExtraListStruct *extra, int32 X, int32 Y, int32 Z) {
	if (_engine->_grid->getBrickShape(X, extra->Y, Z)) {
		extra->destY = -extra->destY;
	}
	if (_engine->_grid->getBrickShape(extra->X, Y, Z)) {
		extra->destX = -extra->destX;
	}
	if (_engine->_grid->getBrickShape(X, Y, extra->Z)) {
		extra->destZ = -extra->destZ;
	}

	extra->X = X;
	extra->lastX = X;
	extra->Y = Y;
	extra->lastY = Y;
	extra->Z = Z;
	extra->lastZ = Z;

	extra->lifeTime = _engine->lbaTime;
}

/** Process extras */
void Extra::processExtras() {
	int32 i;

	int32 currentExtraX = 0;
	int32 currentExtraY = 0;
	int32 currentExtraZ = 0;
	int32 currentExtraSpeedX = 0;
	int32 currentExtraSpeedY = 0;

	for (i = 0; i < EXTRA_MAX_ENTRIES; i++) {
		ExtraListStruct *extra = &extraList[i];
		if (extra->info0 != -1) {
			// process extra life time
			if (extra->type & 0x1) {
				if (extra->actorIdx + extra->lifeTime <= _engine->lbaTime) {
					extra->info0 = -1;
					continue;
				}
			}
			// reset extra
			if (extra->type & 0x800) {
				extra->info0 = -1;
				continue;
			}
			//
			if (extra->type & 0x1000) {
				extra->info0 = _engine->_collision->getAverageValue(97, 100, 30, _engine->lbaTime - extra->lifeTime);
				continue;
			}
			// process extra moving
			if (extra->type & 0x2) {
				currentExtraX = extra->X;
				currentExtraY = extra->Y;
				currentExtraZ = extra->Z;

				currentExtraSpeedX = extra->destX * (_engine->lbaTime - extra->lifeTime);
				extra->X = currentExtraSpeedX + extra->lastX;

				currentExtraSpeedY = extra->destY * (_engine->lbaTime - extra->lifeTime);
				currentExtraSpeedY += extra->lastY;
				extra->Y = currentExtraSpeedY - ABS(((extra->angle * (_engine->lbaTime - extra->lifeTime)) * (_engine->lbaTime - extra->lifeTime)) >> 4);

				extra->Z = extra->destZ * (_engine->lbaTime - extra->lifeTime) + extra->lastZ;

				// check if extra is out of scene
				if (extra->Y < 0 || extra->X < 0 || extra->X > 0x7E00 || extra->Z < 0 || extra->Z > 0x7E00) {
					// if extra is Magic Ball
					if (i == _engine->_gameState->magicBallIdx) {
						int32 spriteIdx = SPRITEHQR_MAGICBALL_YELLOW_TRANS;

						if (extra->info0 == SPRITEHQR_MAGICBALL_GREEN) {
							spriteIdx = SPRITEHQR_MAGICBALL_GREEN_TRANS;
						}
						if (extra->info0 == SPRITEHQR_MAGICBALL_RED) {
							spriteIdx = SPRITEHQR_MAGICBALL_RED_TRANS;
						}

						_engine->_gameState->magicBallIdx = addExtra(-1, extra->X, extra->Y, extra->Z, spriteIdx, 0, 10000, 0);
					}

					// if can take extra on ground
					if (extra->type & 0x20) {
						extra->type &= 0xFFED;
					} else {
						extra->info0 = -1;
					}

					continue;
				}
			}
			//
			if (extra->type & 0x4000) {
				if (_engine->lbaTime - extra->lifeTime > 40) {
					extra->type &= 0xBFFF;
				}
				continue;
			}
			// process actor target hit
			if (extra->type & 0x80) {
				int32 actorIdx, actorIdxAttacked, tmpAngle, angle;

				actorIdxAttacked = extra->lifeTime;
				actorIdx = extra->actorIdx;

				currentExtraX = sceneActors[actorIdxAttacked].X;
				currentExtraY = sceneActors[actorIdxAttacked].Y + 1000;
				currentExtraZ = sceneActors[actorIdxAttacked].Z;

				tmpAngle = _engine->_movements->getAngleAndSetTargetActorDistance(extra->X, extra->Z, currentExtraX, currentExtraZ);
				angle = (tmpAngle - extra->angle) & 0x3FF;

				if (angle > 400 && angle < 600) {
					if (extra->strengthOfHit) {
						_engine->_actor->hitActor(actorIdx, actorIdxAttacked, extra->strengthOfHit, -1);
					}

					if (i == _engine->_gameState->magicBallIdx) {
						_engine->_gameState->magicBallIdx = -1;
					}

					extra->info0 = -1;
					continue;
				} else {
					const int32 angle2 = _engine->_movements->getAngleAndSetTargetActorDistance(extra->Y, 0, currentExtraY, _engine->_movements->targetActorDistance);

					int32 pos = _engine->_movements->getRealAngle(&extra->trackActorMove);

					if (!pos) {
						pos = 1;
					}

					_engine->_movements->rotateActor(pos, 0, angle2);
					extra->Y -= destZ;

					_engine->_movements->rotateActor(0, destX, tmpAngle);
					extra->X += destX;
					extra->Z += destZ;

					_engine->_movements->setActorAngle(0, extra->destZ, 50, &extra->trackActorMove);

					if (actorIdxAttacked == _engine->_collision->checkExtraCollisionWithActors(extra, actorIdx)) {
						if (i == _engine->_gameState->magicBallIdx) {
							_engine->_gameState->magicBallIdx = -1;
						}

						extra->info0 = -1;
						continue;
					}
				}
			}
			// process magic ball extra aiming for key
			if (extra->type & 0x200) {
				int32 actorIdx, tmpAngle, angle;
				//				int32 actorIdxAttacked = extra->lifeTime;
				ExtraListStruct *extraKey = &extraList[extra->actorIdx];
				actorIdx = extra->actorIdx;

				tmpAngle = _engine->_movements->getAngleAndSetTargetActorDistance(extra->X, extra->Z, extraKey->X, extraKey->Z);
				angle = (tmpAngle - extra->angle) & 0x3FF;

				if (angle > 400 && angle < 600) {
					playSample(97, 0x1000, 1, sceneHero->X, sceneHero->Y, sceneHero->Z, 0);

					if (extraKey->info1 > 1) {
						projectPositionOnScreen(extraKey->X - _engine->_grid->cameraX, extraKey->Y - _engine->_grid->cameraY, extraKey->Z - _engine->_grid->cameraZ);
						addOverlay(koNumber, extraKey->info1, projPosX, projPosY, koNormal, 0, 2);
					}

					addOverlay(koSprite, SPRITEHQR_KEY, 10, 30, koNormal, 0, 2);

					_engine->_gameState->inventoryNumKeys += extraKey->info1;
					extraKey->info0 = -1;

					extra->info0 = -1;
					_engine->_gameState->magicBallIdx = addExtra(-1, extra->X, extra->Y, extra->Z, SPRITEHQR_KEY, 0, 8000, 0);
					continue;
				} else {
					int32 angle2, pos;

					angle2 = _engine->_movements->getAngleAndSetTargetActorDistance(extra->Y, 0, extraKey->Y, _engine->_movements->targetActorDistance);
					pos = _engine->_movements->getRealAngle(&extra->trackActorMove);

					if (!pos) {
						pos = 1;
					}

					_engine->_movements->rotateActor(pos, 0, angle2);
					extra->Y -= destZ;

					_engine->_movements->rotateActor(0, destX, tmpAngle);
					extra->X += destX;
					extra->Z += destZ;

					_engine->_movements->setActorAngle(0, extra->destZ, 50, &extra->trackActorMove);

					if (actorIdx == _engine->_collision->checkExtraCollisionWithExtra(extra, _engine->_gameState->magicBallIdx)) {
						playSample(97, 0x1000, 1, sceneHero->X, sceneHero->Y, sceneHero->Z, 0);

						if (extraKey->info1 > 1) {
							projectPositionOnScreen(extraKey->X - _engine->_grid->cameraX, extraKey->Y - _engine->_grid->cameraY, extraKey->Z - _engine->_grid->cameraZ);
							addOverlay(koNumber, extraKey->info1, projPosX, projPosY, koNormal, 0, 2);
						}

						addOverlay(koSprite, SPRITEHQR_KEY, 10, 30, koNormal, 0, 2);

						_engine->_gameState->inventoryNumKeys += extraKey->info1;
						extraKey->info0 = -1;

						extra->info0 = -1;
						_engine->_gameState->magicBallIdx = addExtra(-1, extra->X, extra->Y, extra->Z, SPRITEHQR_KEY, 0, 8000, 0);
						continue;
					}
				}
				if (extraKey->info0 == -1) {
					int32 spriteIdx = SPRITEHQR_MAGICBALL_YELLOW_TRANS;

					if (extra->info0 == SPRITEHQR_MAGICBALL_GREEN) {
						spriteIdx = SPRITEHQR_MAGICBALL_GREEN_TRANS;
					}
					if (extra->info0 == SPRITEHQR_MAGICBALL_RED) {
						spriteIdx = SPRITEHQR_MAGICBALL_RED_TRANS;
					}

					extra->info0 = -1;
					_engine->_gameState->magicBallIdx = addExtra(-1, extra->X, extra->Y, extra->Z, spriteIdx, 0, 8000, 0);
					continue;
				}
			}
			// process extra collision with actors
			if (extra->type & 0x4) {
				if (_engine->_collision->checkExtraCollisionWithActors(extra, extra->actorIdx) != -1) {
					// if extra is Magic Ball
					if (i == _engine->_gameState->magicBallIdx) {
						int32 spriteIdx = SPRITEHQR_MAGICBALL_YELLOW_TRANS;

						if (extra->info0 == SPRITEHQR_MAGICBALL_GREEN) {
							spriteIdx = SPRITEHQR_MAGICBALL_GREEN_TRANS;
						}
						if (extra->info0 == SPRITEHQR_MAGICBALL_RED) {
							spriteIdx = SPRITEHQR_MAGICBALL_RED_TRANS;
						}

						_engine->_gameState->magicBallIdx = addExtra(-1, extra->X, extra->Y, extra->Z, spriteIdx, 0, 10000, 0);
					}

					extra->info0 = -1;
					continue;
				}
			}
			// process extra collision with scene ground
			if (extra->type & 0x8) {
				int32 process = 0;

				if (_engine->_collision->checkExtraCollisionWithBricks(currentExtraX, currentExtraY, currentExtraZ, extra->X, extra->Y, extra->Z)) {
					// if not touch the ground
					if (!(extra->type & 0x2000)) {
						process = 1;
					}
				} else {
					// if touch the ground
					if (extra->type & 0x2000) {
						extra->type &= 0xDFFF; // set flag out of ground
					}
				}

				if (process) {
					// show explode cloud
					if (extra->type & 0x100) {
						addExtraSpecial(currentExtraX, currentExtraY, currentExtraZ, kExplodeCloud);
					}
					// if extra is magic ball
					if (i == _engine->_gameState->magicBallIdx) {
						// FIXME: add constant for sample index
						playSample(86, _engine->getRandomNumber(300) + 3946, 1, extra->X, extra->Y, extra->Z, -1);

						// cant bounce with not magic points
						if (_engine->_gameState->magicBallNumBounce <= 0) {
							int32 spriteIdx = SPRITEHQR_MAGICBALL_YELLOW_TRANS;

							if (extra->info0 == SPRITEHQR_MAGICBALL_GREEN) {
								spriteIdx = SPRITEHQR_MAGICBALL_GREEN_TRANS;
							}
							if (extra->info0 == SPRITEHQR_MAGICBALL_RED) {
								spriteIdx = SPRITEHQR_MAGICBALL_RED_TRANS;
							}

							_engine->_gameState->magicBallIdx = addExtra(-1, extra->X, extra->Y, extra->Z, spriteIdx, 0, 10000, 0);

							extra->info0 = -1;
							continue;
						}

						// if has magic points
						if (_engine->_gameState->magicBallNumBounce == 1) {
							if (!_engine->_gameState->magicBallAuxBounce--) {
								int32 spriteIdx = SPRITEHQR_MAGICBALL_YELLOW_TRANS;

								if (extra->info0 == SPRITEHQR_MAGICBALL_GREEN) {
									spriteIdx = SPRITEHQR_MAGICBALL_GREEN_TRANS;
								}
								if (extra->info0 == SPRITEHQR_MAGICBALL_RED) {
									spriteIdx = SPRITEHQR_MAGICBALL_RED_TRANS;
								}

								_engine->_gameState->magicBallIdx = addExtra(-1, extra->X, extra->Y, extra->Z, spriteIdx, 0, 10000, 0);

								extra->info0 = -1;
								continue;
							} else {
								processMagicballBounce(extra, currentExtraX, currentExtraY, currentExtraZ);
							}
						}
					} else {
						extra->info0 = -1;
						continue;
					}
				}
			}
			// extra stop moving while collision with bricks
			if (extra->type & 0x10) {
				int32 process = 0;

				if (_engine->_collision->checkExtraCollisionWithBricks(currentExtraX, currentExtraY, currentExtraZ, extra->X, extra->Y, extra->Z)) {
					// if not touch the ground
					if (!(extra->type & 0x2000)) {
						process = 1;
					}
				} else {
					// if touch the ground
					if (extra->type & 0x2000) {
						extra->type &= 0xDFFF; // set flag out of ground
					}
				}

				if (process) {
					int16 *spriteBounds;

					spriteBounds = (int16 *)(spriteBoundingBoxPtr + extra->info0 * 16 + 8);
					extra->Y = (_engine->_collision->collisionY << 8) + 0x100 - *(spriteBounds);
					extra->type &= 0xFFED;
					continue;
				}
			}
			// get extras on ground
			if ((extra->type & 0x20) && !(extra->type & 0x2)) {
				// if hero touch extra
				if (_engine->_collision->checkExtraCollisionWithActors(extra, -1) == 0) {
					// FIXME: add constant for sample index
					playSample(97, 0x1000, 1, extra->X, extra->Y, extra->Z, -1);

					if (extra->info1 > 1 && !(_engine->loopPressedKey & 2)) {
						projectPositionOnScreen(extra->X - _engine->_grid->cameraX, extra->Y - _engine->_grid->cameraY, extra->Z - _engine->_grid->cameraZ);
						addOverlay(koNumber, extra->info1, projPosX, projPosY, 158, koNormal, 2);
					}

					addOverlay(koSprite, extra->info0, 10, 30, 0, koNormal, 2);

					if (extra->info0 == SPRITEHQR_KASHES) {
						_engine->_gameState->inventoryNumKashes += extra->info1;
						if (_engine->_gameState->inventoryNumKashes > 999) {
							_engine->_gameState->inventoryNumKashes = 999;
						}
					}

					if (extra->info0 == SPRITEHQR_LIFEPOINTS) {
						sceneHero->life += extra->info1;
						if (sceneHero->life > 50) {
							sceneHero->life = 50;
						}
					}

					if (extra->info0 == SPRITEHQR_MAGICPOINTS && _engine->_gameState->magicLevelIdx) {
						_engine->_gameState->inventoryMagicPoints += extra->info1 * 2;
						if (_engine->_gameState->inventoryMagicPoints > _engine->_gameState->magicLevelIdx * 20) {
							_engine->_gameState->inventoryMagicPoints = _engine->_gameState->magicLevelIdx * 20;
						}
					}

					if (extra->info0 == SPRITEHQR_KEY) {
						_engine->_gameState->inventoryNumKeys += extra->info1;
					}

					if (extra->info0 == SPRITEHQR_CLOVERLEAF) {
						_engine->_gameState->inventoryNumLeafs += extra->info1;
						if (_engine->_gameState->inventoryNumLeafs > _engine->_gameState->inventoryNumLeafsBox) {
							_engine->_gameState->inventoryNumLeafs = _engine->_gameState->inventoryNumLeafsBox;
						}
					}

					extra->info0 = -1;
				}
			}
		}
	}
}

} // namespace TwinE
