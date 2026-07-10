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

#ifndef TWINE_SCENE_MOVE3D_H
#define TWINE_SCENE_MOVE3D_H

#include "twine/scene/actor.h"

namespace TwinE {

class TwinEEngine;

// LIB386/3D/MOVE.CPP - timer-based distance/angle stepping
void changeSpeedMove(TwinEEngine *engine, MoveStruct *move, int32 speed);
void restartMove(TwinEEngine *engine, MoveStruct *move);
void initMove(TwinEEngine *engine, MoveStruct *move, int32 speed);
int32 getDeltaMove(TwinEEngine *engine, MoveStruct *move);
void initBoundAngleMove(TwinEEngine *engine, BoundMoveStruct *bound, int32 speed, int32 start, int32 end);
void changeSpeedBoundAngleMove(TwinEEngine *engine, BoundMoveStruct *bound, int32 speed, int32 end);
int32 getBoundAngleMove(TwinEEngine *engine, BoundMoveStruct *bound);
int32 getSpeedMove(const MoveStruct *move);

} // namespace TwinE

#endif
