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

#ifndef TWINE_SCRIPTMOVE_H
#define TWINE_SCRIPTMOVE_H

#include "common/scummsys.h"

namespace TwinE {

class ActorMoveStruct;
class TwinEEngine;
class ActorStruct;

class ScriptMove {
private:
	TwinEEngine *_engine;

	uint8 *scriptPtr;
	int32 continueMove;
	int32 scriptPosition;
	ActorMoveStruct *move;
	int32 numRepeatSample = 1;
	int32 mEND(int32 actorIdx, ActorStruct *actor);
	int32 mNOP(int32 actorIdx, ActorStruct *actor);
	int32 mBODY(int32 actorIdx, ActorStruct *actor);
	int32 mANIM(int32 actorIdx, ActorStruct *actor);
	int32 mGOTO_POINT(int32 actorIdx, ActorStruct *actor);
	int32 mWAIT_ANIM(int32 actorIdx, ActorStruct *actor);
	int32 mLOOP(int32 actorIdx, ActorStruct *actor);
	int32 mANGLE(int32 actorIdx, ActorStruct *actor);
	int32 mPOS_POINT(int32 actorIdx, ActorStruct *actor);
	int32 mLABEL(int32 actorIdx, ActorStruct *actor);
	int32 mGOTO(int32 actorIdx, ActorStruct *actor);
	int32 mSTOP(int32 actorIdx, ActorStruct *actor);
	int32 mGOTO_SYM_POINT(int32 actorIdx, ActorStruct *actor);
	int32 mWAIT_NUM_ANIM(int32 actorIdx, ActorStruct *actor);
	int32 mSAMPLE(int32 actorIdx, ActorStruct *actor);
	int32 mGOTO_POINT_3D(int32 actorIdx, ActorStruct *actor);
	int32 mSPEED(int32 actorIdx, ActorStruct *actor);
	int32 mBACKGROUND(int32 actorIdx, ActorStruct *actor);
	int32 mWAIT_NUM_SECOND(int32 actorIdx, ActorStruct *actor);
	int32 mNO_BODY(int32 actorIdx, ActorStruct *actor);
	int32 mBETA(int32 actorIdx, ActorStruct *actor);
	int32 mOPEN_LEFT(int32 actorIdx, ActorStruct *actor);
	int32 mOPEN_RIGHT(int32 actorIdx, ActorStruct *actor);
	int32 mOPEN_UP(int32 actorIdx, ActorStruct *actor);
	int32 mOPEN_DOWN(int32 actorIdx, ActorStruct *actor);
	int32 mCLOSE(int32 actorIdx, ActorStruct *actor);
	int32 mWAIT_DOOR(int32 actorIdx, ActorStruct *actor);
	int32 mSAMPLE_RND(int32 actorIdx, ActorStruct *actor);
	int32 mSAMPLE_ALWAYS(int32 actorIdx, ActorStruct *actor);
	int32 mSAMPLE_STOP(int32 actorIdx, ActorStruct *actor);
	int32 mPLAY_FLA(int32 actorIdx, ActorStruct *actor);
	int32 mREPEAT_SAMPLE(int32 actorIdx, ActorStruct *actor);
	int32 mSIMPLE_SAMPLE(int32 actorIdx, ActorStruct *actor);
	int32 mFACE_HERO(int32 actorIdx, ActorStruct *actor);
	int32 mANGLE_RND(int32 actorIdx, ActorStruct *actor);

public:
	ScriptMove(TwinEEngine *engine) : _engine(engine) {}

	/** Process actor move script
	@param actorIdx Current processed actor index */
	void processMoveScript(int32 actorIdx);
};

} // namespace TwinE

#endif
