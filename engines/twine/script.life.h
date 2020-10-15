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

#ifndef TWINE_SCRIPTLIFE_H
#define TWINE_SCRIPTLIFE_H

#include "common/scummsys.h"

namespace TwinE {

#define MAX_TARGET_ACTOR_DISTANCE 0x7D00

class ActorStruct;
class TwinEEngine;

class ScriptLife {
private:
	TwinEEngine *_engine;

	uint8 *scriptPtr; // local script pointer
	uint8 *opcodePtr; // local opcode script pointer

	int32 drawVar1;
	int8 textStr[256]; // string

	int32 processLifeConditions(ActorStruct *actor);
	int32 processLifeOperators(int32 valueSize);
	int32 lEMPTY(int32 actorIdx, ActorStruct *actor);
	int32 lEND(int32 actorIdx, ActorStruct *actor);
	int32 lNOP(int32 actorIdx, ActorStruct *actor);
	int32 lSNIF(int32 actorIdx, ActorStruct *actor);
	int32 lOFFSET(int32 actorIdx, ActorStruct *actor);
	int32 lNEVERIF(int32 actorIdx, ActorStruct *actor);
	int32 lNO_IF(int32 actorIdx, ActorStruct *actor);
	int32 lLABEL(int32 actorIdx, ActorStruct *actor);
	int32 lRETURN(int32 actorIdx, ActorStruct *actor);
	int32 lIF(int32 actorIdx, ActorStruct *actor);
	int32 lSWIF(int32 actorIdx, ActorStruct *actor);
	int32 lONEIF(int32 actorIdx, ActorStruct *actor);
	int32 lELSE(int32 actorIdx, ActorStruct *actor);
	int32 lBODY(int32 actorIdx, ActorStruct *actor);
	int32 lBODY_OBJ(int32 actorIdx, ActorStruct *actor);
	int32 lANIM(int32 actorIdx, ActorStruct *actor);
	int32 lANIM_OBJ(int32 actorIdx, ActorStruct *actor);
	int32 lSET_LIFE(int32 actorIdx, ActorStruct *actor);
	int32 lSET_LIFE_OBJ(int32 actorIdx, ActorStruct *actor);
	int32 lSET_TRACK(int32 actorIdx, ActorStruct *actor);
	int32 lSET_TRACK_OBJ(int32 actorIdx, ActorStruct *actor);
	int32 lMESSAGE(int32 actorIdx, ActorStruct *actor);
	int32 lFALLABLE(int32 actorIdx, ActorStruct *actor);
	int32 lSET_DIRMODE(int32 actorIdx, ActorStruct *actor);
	int32 lSET_DIRMODE_OBJ(int32 actorIdx, ActorStruct *actor);
	int32 lCAM_FOLLOW(int32 actorIdx, ActorStruct *actor);
	int32 lSET_BEHAVIOUR(int32 actorIdx, ActorStruct *actor);
	int32 lSET_FLAG_CUBE(int32 actorIdx, ActorStruct *actor);
	int32 lCOMPORTEMENT(int32 actorIdx, ActorStruct *actor);
	int32 lSET_COMPORTEMENT(int32 actorIdx, ActorStruct *actor);
	int32 lSET_COMPORTEMENT_OBJ(int32 actorIdx, ActorStruct *actor);
	int32 lEND_COMPORTEMENT(int32 actorIdx, ActorStruct *actor);
	int32 lSET_FLAG_GAME(int32 actorIdx, ActorStruct *actor);
	int32 lKILL_OBJ(int32 actorIdx, ActorStruct *actor);
	int32 lSUICIDE(int32 actorIdx, ActorStruct *actor);
	int32 lUSE_ONE_LITTLE_KEY(int32 actorIdx, ActorStruct *actor);
	int32 lGIVE_GOLD_PIECES(int32 actorIdx, ActorStruct *actor);
	int32 lEND_LIFE(int32 actorIdx, ActorStruct *actor);
	int32 lSTOP_L_TRACK(int32 actorIdx, ActorStruct *actor);
	int32 lRESTORE_L_TRACK(int32 actorIdx, ActorStruct *actor);
	int32 lMESSAGE_OBJ(int32 actorIdx, ActorStruct *actor);
	int32 lINC_CHAPTER(int32 actorIdx, ActorStruct *actor);
	int32 lFOUND_OBJECT(int32 actorIdx, ActorStruct *actor);
	int32 lSET_DOOR_LEFT(int32 actorIdx, ActorStruct *actor);
	int32 lSET_DOOR_RIGHT(int32 actorIdx, ActorStruct *actor);
	int32 lSET_DOOR_UP(int32 actorIdx, ActorStruct *actor);
	int32 lSET_DOOR_DOWN(int32 actorIdx, ActorStruct *actor);
	int32 lGIVE_BONUS(int32 actorIdx, ActorStruct *actor);
	int32 lCHANGE_CUBE(int32 actorIdx, ActorStruct *actor);
	int32 lOBJ_COL(int32 actorIdx, ActorStruct *actor);
	int32 lBRICK_COL(int32 actorIdx, ActorStruct *actor);
	int32 lOR_IF(int32 actorIdx, ActorStruct *actor);
	int32 lINVISIBLE(int32 actorIdx, ActorStruct *actor);
	int32 lZOOM(int32 actorIdx, ActorStruct *actor);
	int32 lPOS_POINT(int32 actorIdx, ActorStruct *actor);
	int32 lSET_MAGIC_LEVEL(int32 actorIdx, ActorStruct *actor);
	int32 lSUB_MAGIC_POINT(int32 actorIdx, ActorStruct *actor);
	int32 lSET_LIFE_POINT_OBJ(int32 actorIdx, ActorStruct *actor);
	int32 lSUB_LIFE_POINT_OBJ(int32 actorIdx, ActorStruct *actor);
	int32 lHIT_OBJ(int32 actorIdx, ActorStruct *actor);
	int32 lPLAY_FLA(int32 actorIdx, ActorStruct *actor);
	int32 lPLAY_MIDI(int32 actorIdx, ActorStruct *actor);
	int32 lINC_CLOVER_BOX(int32 actorIdx, ActorStruct *actor);
	int32 lSET_USED_INVENTORY(int32 actorIdx, ActorStruct *actor);
	int32 lADD_CHOICE(int32 actorIdx, ActorStruct *actor);
	int32 lASK_CHOICE(int32 actorIdx, ActorStruct *actor);
	int32 lBIG_MESSAGE(int32 actorIdx, ActorStruct *actor);
	int32 lINIT_PINGOUIN(int32 actorIdx, ActorStruct *actor);
	int32 lSET_HOLO_POS(int32 actorIdx, ActorStruct *actor);
	int32 lCLR_HOLO_POS(int32 actorIdx, ActorStruct *actor);
	int32 lADD_FUEL(int32 actorIdx, ActorStruct *actor);
	int32 lSUB_FUEL(int32 actorIdx, ActorStruct *actor);
	int32 lSET_GRM(int32 actorIdx, ActorStruct *actor);
	int32 lSAY_MESSAGE(int32 actorIdx, ActorStruct *actor);
	int32 lSAY_MESSAGE_OBJ(int32 actorIdx, ActorStruct *actor);
	int32 lFULL_POINT(int32 actorIdx, ActorStruct *actor);
	int32 lBETA(int32 actorIdx, ActorStruct *actor);
	int32 lGRM_OFF(int32 actorIdx, ActorStruct *actor);
	int32 lFADE_PAL_RED(int32 actorIdx, ActorStruct *actor);
	int32 lFADE_ALARM_RED(int32 actorIdx, ActorStruct *actor);
	int32 lFADE_ALARM_PAL(int32 actorIdx, ActorStruct *actor);
	int32 lFADE_RED_PAL(int32 actorIdx, ActorStruct *actor);
	int32 lFADE_RED_ALARM(int32 actorIdx, ActorStruct *actor);
	int32 lFADE_PAL_ALARM(int32 actorIdx, ActorStruct *actor);
	int32 lEXPLODE_OBJ(int32 actorIdx, ActorStruct *actor);
	int32 lBUBBLE_ON(int32 actorIdx, ActorStruct *actor);
	int32 lBUBBLE_OFF(int32 actorIdx, ActorStruct *actor);
	int32 lASK_CHOICE_OBJ(int32 actorIdx, ActorStruct *actor);
	int32 lSET_DARK_PAL(int32 actorIdx, ActorStruct *actor);
	int32 lSET_NORMAL_PAL(int32 actorIdx, ActorStruct *actor);
	int32 lMESSAGE_SENDELL(int32 actorIdx, ActorStruct *actor);
	int32 lANIM_SET(int32 actorIdx, ActorStruct *actor);
	int32 lHOLOMAP_TRAJ(int32 actorIdx, ActorStruct *actor);
	int32 lGAME_OVER(int32 actorIdx, ActorStruct *actor);
	int32 lTHE_END(int32 actorIdx, ActorStruct *actor);
	int32 lMIDI_OFF(int32 actorIdx, ActorStruct *actor);
	int32 lPLAY_CD_TRACK(int32 actorIdx, ActorStruct *actor);
	int32 lPROJ_ISO(int32 actorIdx, ActorStruct *actor);
	int32 lPROJ_3D(int32 actorIdx, ActorStruct *actor);
	int32 lTEXT(int32 actorIdx, ActorStruct *actor);
	int32 lCLEAR_TEXT(int32 actorIdx, ActorStruct *actor);
	int32 lBRUTAL_EXIT(int32 actorIdx, ActorStruct *actor);

public:
	ScriptLife(TwinEEngine *engine) : _engine(engine) {}

	/** Process actor life script
	@param actorIdx Current processed actor index */
	void processLifeScript(int32 actorIdx);
};

} // namespace TwinE

#endif
