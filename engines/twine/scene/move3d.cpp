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

#include "twine/scene/move3d.h"
#include "twine/twine.h"

namespace TwinE {

namespace {

int32 getDeltaAccMove(int32 *acc) {
	if (*acc >= 1000 || *acc <= -1000) {
		const int32 quot = *acc / 1000;
		*acc -= quot * 1000;
		return quot;
	}
	return 0;
}

int32 adjustSpeedAngle(int32 speed, int32 start, int32 end) {
	int32 diff = (end & 4095) - (start & 4095);
	if (diff == 0) {
		return 0;
	}

	if (diff < 0) {
		diff = -diff;
		speed = -speed;
	}

	return (diff < 2048) ? speed : -speed;
}

} // namespace

void changeSpeedMove(TwinEEngine *engine, MoveStruct *move, int32 speed) {
	const uint32 timer = engine->timerRef;
	const uint32 delta = timer - move->lastTimer;

	if (delta || move->speed) {
		move->acc += (int32)(timer - move->lastTimer) * move->speed;
		move->lastTimer = timer;
	}

	move->speed = speed;
}

void restartMove(TwinEEngine *engine, MoveStruct *move) {
	move->acc = 500;
	move->lastTimer = engine->timerRef;
}

void initMove(TwinEEngine *engine, MoveStruct *move, int32 speed) {
	restartMove(engine, move);
	changeSpeedMove(engine, move, speed);
}

int32 getDeltaMove(TwinEEngine *engine, MoveStruct *move) {
	const uint32 timer = engine->timerRef;
	const uint32 delta = timer - move->lastTimer;

	if (delta || move->speed) {
		move->acc += (int32)(timer - move->lastTimer) * move->speed;
		move->lastTimer = timer;
		return getDeltaAccMove(&move->acc);
	}

	return 0;
}

void initBoundAngleMove(TwinEEngine *engine, BoundMoveStruct *bound, int32 speed, int32 start, int32 end) {
	changeSpeedMove(engine, &bound->move, adjustSpeedAngle(speed, start, end));
	bound->cur = start & 4095;
	bound->end = end & 4095;
}

void changeSpeedBoundAngleMove(TwinEEngine *engine, BoundMoveStruct *bound, int32 speed, int32 end) {
	changeSpeedMove(engine, &bound->move, adjustSpeedAngle(speed, bound->cur, end));
	bound->end = end & 4095;
}

int32 getBoundAngleMove(TwinEEngine *engine, BoundMoveStruct *bound) {
	const int32 temp = getDeltaMove(engine, &bound->move);
	int32 cur = bound->cur;

	if (temp) {
		int32 end = bound->end;

		if (bound->move.speed > 0) {
			if (cur > end) {
				end += 4096;
			}

			cur += temp;

			if (cur >= end) {
				cur = end;
				bound->move.speed = 0;
			}
		} else {
			if (cur < end) {
				end -= 4096;
			}

			cur += temp;

			if (cur <= end) {
				cur = end;
				bound->move.speed = 0;
			}
		}

		cur &= 4095;
		bound->cur = cur;
	}

	return cur;
}

int32 getSpeedMove(const MoveStruct *move) {
	return move->speed;
}

} // namespace TwinE
