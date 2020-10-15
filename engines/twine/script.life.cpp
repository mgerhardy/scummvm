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
#include <string.h>

#include "actor.h"
#include "animations.h"
#include "collision.h"
#include "flamovies.h"
#include "gamestate.h"
#include "grid.h"
#include "holomap.h"
#include "interface.h"
#include "keyboard.h"
#include "movements.h"
#include "music.h"
#include "redraw.h"
#include "renderer.h"
#include "resources.h"
#include "scene.h"
#include "screens.h"
#include "script.life.h"
#include "sdlengine.h"
#include "sound.h"
#include "text.h"

namespace TwinE {

/** Returns:
	   -1 - Need implementation
		0 - Completed
		1 - Break script */
typedef int32 ScriptLifeFunc(int32 actorIdx, ActorStruct *actor);

typedef struct ScriptLifeFunction {
	const uint8 *name;
	ScriptLifeFunc *function;
} ScriptLifeFunction;

#define MAPFUNC(name, func) \
	{ (uint8 *)name, func }

/** Script condition operators */
enum LifeScriptOperators {
	/*==*/kEqualTo = 0,
	/*> */ kGreaterThan = 1,
	/*< */ kLessThan = 2,
	/*>=*/kGreaterThanOrEqualTo = 3,
	/*<=*/kLessThanOrEqualTo = 4,
	/*!=*/kNotEqualTo = 5
};

/** Script condition command opcodes */
enum LifeScriptConditions {
	/*0x00*/ kcCOL = 0,
	/*0x01*/ kcCOL_OBJ = 1,
	/*0x02*/ kcDISTANCE = 2,
	/*0x03*/ kcZONE = 3,
	/*0x04*/ kcZONE_OBJ = 4,
	/*0x05*/ kcBODY = 5,
	/*0x06*/ kcBODY_OBJ = 6,
	/*0x07*/ kcANIM = 7,
	/*0x08*/ kcANIM_OBJ = 8,
	/*0x09*/ kcL_TRACK = 9,
	/*0x0A*/ kcL_TRACK_OBJ = 10,
	/*0x0B*/ kcFLAG_CUBE = 11,
	/*0x0C*/ kcCONE_VIEW = 12,
	/*0x0D*/ kcHIT_BY = 13,
	/*0x0E*/ kcACTION = 14,
	/*0x0F*/ kcFLAG_GAME = 15,
	/*0x10*/ kcLIFE_POINT = 16,
	/*0x11*/ kcLIFE_POINT_OBJ = 17,
	/*0x12*/ kcNUM_LITTLE_KEYS = 18,
	/*0x13*/ kcNUM_GOLD_PIECES = 19,
	/*0x14*/ kcBEHAVIOUR = 20,
	/*0x15*/ kcCHAPTER = 21,
	/*0x16*/ kcDISTANCE_3D = 22,
	/*0x17 - 23 unused */
	/*0x18 - 24 unused */
	/*0x19*/ kcUSE_INVENTORY = 25,
	/*0x1A*/ kcCHOICE = 26,
	/*0x1B*/ kcFUEL = 27,
	/*0x1C*/ kcCARRIED_BY = 28,
	/*0x1D*/ kcCDROM = 29
};

/** Returns:
	   -1 - Need implementation
		1 - Condition value size (1 byte)
		2 - Condition value size (2 byes) */
int32 ScriptLife::processLifeConditions(ActorStruct *actor) {
	int32 conditionOpcode, conditionValueSize;

	conditionValueSize = 1;
	conditionOpcode = *(scriptPtr++);

	switch (conditionOpcode) {
	case kcCOL:
		if (actor->life <= 0) {
			_engine->_scene->currentScriptValue = -1;
		} else {
			_engine->_scene->currentScriptValue = actor->collision;
		}
		break;
	case kcCOL_OBJ: {
		int32 actorIdx = *(scriptPtr++);
		if (_engine->_scene->sceneActors[actorIdx].life <= 0) {
			_engine->_scene->currentScriptValue = -1;
		} else {
			_engine->_scene->currentScriptValue = _engine->_scene->sceneActors[actorIdx].collision;
		}
	} break;
	case kcDISTANCE: {
		ActorStruct *otherActor;
		int32 actorIdx = *(scriptPtr++);
		conditionValueSize = 2;
		otherActor = &_engine->_scene->sceneActors[actorIdx];
		if (!otherActor->dynamicFlags.bIsDead) {
			if (ABS(actor->Y - otherActor->Y) >= 1500) {
				_engine->_scene->currentScriptValue = MAX_TARGET_ACTOR_DISTANCE;
			} else {
				// Returns int32, so we check for integer overflow
				int32 distance = _engine->_movements->getDistance2D(actor->X, actor->Z, otherActor->X, otherActor->Z);
				if (ABS(distance) > MAX_TARGET_ACTOR_DISTANCE) {
					_engine->_scene->currentScriptValue = MAX_TARGET_ACTOR_DISTANCE;
				} else {
					_engine->_scene->currentScriptValue = distance;
				}
			}
		} else {
			_engine->_scene->currentScriptValue = MAX_TARGET_ACTOR_DISTANCE;
		}
	} break;
	case kcZONE:
		_engine->_scene->currentScriptValue = actor->zone;
		break;
	case kcZONE_OBJ: {
		int32 actorIdx = *(scriptPtr++);
		_engine->_scene->currentScriptValue = _engine->_scene->sceneActors[actorIdx].zone;
	} break;
	case kcBODY:
		_engine->_scene->currentScriptValue = actor->body;
		break;
	case kcBODY_OBJ: {
		int32 actorIdx = *(scriptPtr++);
		_engine->_scene->currentScriptValue = _engine->_scene->sceneActors[actorIdx].body;
	} break;
	case kcANIM:
		_engine->_scene->currentScriptValue = actor->anim;
		break;
	case kcANIM_OBJ: {
		int32 actorIdx = *(scriptPtr++);
		_engine->_scene->currentScriptValue = _engine->_scene->sceneActors[actorIdx].anim;
	} break;
	case kcL_TRACK:
		_engine->_scene->currentScriptValue = actor->labelIdx;
		break;
	case kcL_TRACK_OBJ: {
		int32 actorIdx = *(scriptPtr++);
		_engine->_scene->currentScriptValue = _engine->_scene->sceneActors[actorIdx].labelIdx;
	} break;
	case kcFLAG_CUBE: {
		int32 flagIdx = *(scriptPtr++);
		_engine->_scene->currentScriptValue = _engine->_scene->sceneFlags[flagIdx];
	} break;
	case kcCONE_VIEW: {
		int32 newAngle;
		int32 targetActorIdx;
		ActorStruct *targetActor;

		newAngle = 0;
		targetActorIdx = *(scriptPtr++);
		targetActor = &_engine->_scene->sceneActors[targetActorIdx];

		conditionValueSize = 2;

		if (!targetActor->dynamicFlags.bIsDead) {
			if (ABS(targetActor->Y - actor->Y) < 1500) {
				newAngle = _engine->_movements->getAngleAndSetTargetActorDistance(actor->X, actor->Z, targetActor->X, targetActor->Z);
				if (ABS(_engine->_movements->targetActorDistance) > MAX_TARGET_ACTOR_DISTANCE) {
					_engine->_movements->targetActorDistance = MAX_TARGET_ACTOR_DISTANCE;
				}
			} else {
				_engine->_movements->targetActorDistance = MAX_TARGET_ACTOR_DISTANCE;
			}

			if (!targetActorIdx) {
				int32 heroAngle;

				heroAngle = actor->angle + 0x480 - newAngle + 0x400;
				heroAngle &= 0x3FF;

				if (ABS(heroAngle) > 0x100) {
					_engine->_scene->currentScriptValue = MAX_TARGET_ACTOR_DISTANCE;
				} else {
					_engine->_scene->currentScriptValue = _engine->_movements->targetActorDistance;
				}
			} else {
				if (_engine->_actor->heroBehaviour == kDiscrete) {
					int32 heroAngle;

					heroAngle = actor->angle + 0x480 - newAngle + 0x400;
					heroAngle &= 0x3FF;

					if (ABS(heroAngle) > 0x100) {
						_engine->_scene->currentScriptValue = MAX_TARGET_ACTOR_DISTANCE;
					} else {
						_engine->_scene->currentScriptValue = _engine->_movements->targetActorDistance;
					}
				} else {
					_engine->_scene->currentScriptValue = _engine->_movements->targetActorDistance;
				}
			}
		} else {
			_engine->_scene->currentScriptValue = MAX_TARGET_ACTOR_DISTANCE;
		}
	} break;
	case kcHIT_BY:
		_engine->_scene->currentScriptValue = actor->hitBy;
		break;
	case kcACTION:
		_engine->_scene->currentScriptValue = _engine->_movements->heroAction;
		break;
	case kcFLAG_GAME: {
		int32 flagIdx = *(scriptPtr++);
		if (!_engine->_gameState->gameFlags[GAMEFLAG_INVENTORY_DISABLED] ||
		    (_engine->_gameState->gameFlags[GAMEFLAG_INVENTORY_DISABLED] && flagIdx >= 28)) {
			_engine->_scene->currentScriptValue = _engine->_gameState->gameFlags[flagIdx];
		} else {
			if (flagIdx == GAMEFLAG_INVENTORY_DISABLED) {
				_engine->_scene->currentScriptValue = _engine->_gameState->gameFlags[flagIdx];
			} else {
				_engine->_scene->currentScriptValue = 0;
			}
		}
	} break;
	case kcLIFE_POINT:
		_engine->_scene->currentScriptValue = actor->life;
		break;
	case kcLIFE_POINT_OBJ: {
		int32 actorIdx = *(scriptPtr++);
		_engine->_scene->currentScriptValue = _engine->_scene->sceneActors[actorIdx].life;
	} break;
	case kcNUM_LITTLE_KEYS:
		_engine->_scene->currentScriptValue = _engine->_gameState->inventoryNumKeys;
		break;
	case kcNUM_GOLD_PIECES:
		conditionValueSize = 2;
		_engine->_scene->currentScriptValue = _engine->_gameState->inventoryNumKashes;
		break;
	case kcBEHAVIOUR:
		_engine->_scene->currentScriptValue = _engine->_actor->heroBehaviour;
		break;
	case kcCHAPTER:
		_engine->_scene->currentScriptValue = _engine->_gameState->gameChapter;
		break;
	case kcDISTANCE_3D: {
		int32 targetActorIdx;
		ActorStruct *targetActor;

		targetActorIdx = *(scriptPtr++);
		targetActor = &_engine->_scene->sceneActors[targetActorIdx];

		conditionValueSize = 2;

		if (!targetActor->dynamicFlags.bIsDead) {
			// Returns int32, so we check for integer overflow
			int32 distance = _engine->_movements->getDistance3D(actor->X, actor->Y, actor->Z, targetActor->X, targetActor->Y, targetActor->Z);
			if (ABS(distance) > MAX_TARGET_ACTOR_DISTANCE) {
				_engine->_scene->currentScriptValue = MAX_TARGET_ACTOR_DISTANCE;
			} else {
				_engine->_scene->currentScriptValue = distance;
			}
		} else {
			_engine->_scene->currentScriptValue = MAX_TARGET_ACTOR_DISTANCE;
		}
	} break;
	case 23: // unused
	case 24:
		break;
	case kcUSE_INVENTORY: {
		int32 item = *(scriptPtr++);

		if (!_engine->_gameState->gameFlags[GAMEFLAG_INVENTORY_DISABLED]) {
			if (item == _engine->loopInventoryItem) {
				_engine->_scene->currentScriptValue = 1;
			} else {
				if (_engine->_gameState->inventoryFlags[item] == 1 && _engine->_gameState->gameFlags[item] == 1) {
					_engine->_scene->currentScriptValue = 1;
				} else {
					_engine->_scene->currentScriptValue = 0;
				}
			}

			if (_engine->_scene->currentScriptValue == 1) {
				_engine->_redraw->addOverlay(koInventoryItem, item, 0, 0, 0, koNormal, 3);
			}
		} else {
			_engine->_scene->currentScriptValue = 0;
		}
	} break;
	case kcCHOICE:
		conditionValueSize = 2;
		_engine->_scene->currentScriptValue = _engine->_gameState->choiceAnswer;
		break;
	case kcFUEL:
		_engine->_scene->currentScriptValue = _engine->_gameState->inventoryNumGas;
		break;
	case kcCARRIED_BY:
		_engine->_scene->currentScriptValue = actor->standOn;
		break;
	case kcCDROM:
		_engine->_scene->currentScriptValue = 1;
		break;
	default:
		error("Actor condition opcode %d\n", conditionOpcode);
		break;
	}

	return conditionValueSize;
}

/** Returns:
	   -1 - Need implementation
		0 - Condition false
		1 - Condition true */
int32 ScriptLife::processLifeOperators(int32 valueSize) {
	int32 operatorCode, conditionValue;

	operatorCode = *(scriptPtr++);

	if (valueSize == 1) {
		conditionValue = *(scriptPtr++);
	} else if (valueSize == 2) {
		conditionValue = *((int16 *)scriptPtr);
		scriptPtr += 2;
	} else {
		error("Unknown operator value size %d\n", valueSize);
		return 0;
	}

	switch (operatorCode) {
	case kEqualTo:
		if (_engine->_scene->currentScriptValue == conditionValue) {
			return 1;
		}
		break;
	case kGreaterThan:
		if (_engine->_scene->currentScriptValue > conditionValue) {
			return 1;
		}
		break;
	case kLessThan:
		if (_engine->_scene->currentScriptValue < conditionValue) {
			return 1;
		}
		break;
	case kGreaterThanOrEqualTo:
		if (_engine->_scene->currentScriptValue >= conditionValue) {
			return 1;
		}
		break;
	case kLessThanOrEqualTo:
		if (_engine->_scene->currentScriptValue <= conditionValue) {
			return 1;
		}
		break;
	case kNotEqualTo:
		if (_engine->_scene->currentScriptValue != conditionValue) {
			return 1;
		}
		break;
	default:
		error("Actor operator opcode %d\n", operatorCode);
		break;
	}

	return 0;
}

/** Life script command definitions */

/* For unused opcodes */
int32 ScriptLife::lEMPTY(int32 actorIdx, ActorStruct *actor) {
	return 0;
}

/*0x00*/
int32 ScriptLife::lEND(int32 actorIdx, ActorStruct *actor) {
	actor->positionInLifeScript = -1;
	return 1; // break script
}

/*0x01*/
int32 ScriptLife::lNOP(int32 actorIdx, ActorStruct *actor) {
	scriptPtr++;
	return 0;
}

/*0x02*/
int32 ScriptLife::lSNIF(int32 actorIdx, ActorStruct *actor) {
	int32 valueSize = processLifeConditions(actor);
	if (!processLifeOperators(valueSize)) {
		*opcodePtr = 13; // SWIF
	}
	scriptPtr = actor->lifeScript + *((int16 *)scriptPtr); // condition offset
	return 0;
}

/*0x03*/
int32 ScriptLife::lOFFSET(int32 actorIdx, ActorStruct *actor) {
	scriptPtr = actor->lifeScript + *((int16 *)scriptPtr); // offset
	return 0;
}

/*0x04*/
int32 ScriptLife::lNEVERIF(int32 actorIdx, ActorStruct *actor) {
	int32 valueSize = processLifeConditions(actor);
	processLifeOperators(valueSize);
	scriptPtr = actor->lifeScript + *((int16 *)scriptPtr); // condition offset
	return 0;
}

/*0x06*/
int32 ScriptLife::lNO_IF(int32 actorIdx, ActorStruct *actor) {
	return 0;
}

/*0x0A*/
int32 ScriptLife::lLABEL(int32 actorIdx, ActorStruct *actor) {
	scriptPtr++;
	return 0;
}

/*0x0B*/
int32 ScriptLife::lRETURN(int32 actorIdx, ActorStruct *actor) {
	return 1; // break script
}

/*0x0C*/
int32 ScriptLife::lIF(int32 actorIdx, ActorStruct *actor) {
	int32 valueSize = processLifeConditions(actor);
	if (!processLifeOperators(valueSize)) {
		scriptPtr = actor->lifeScript + *((int16 *)scriptPtr); // condition offset
	} else {
		scriptPtr += 2;
	}

	return 0;
}

/*0x0D*/
int32 ScriptLife::lSWIF(int32 actorIdx, ActorStruct *actor) {
	int32 valueSize = processLifeConditions(actor);
	if (!processLifeOperators(valueSize)) {
		scriptPtr = actor->lifeScript + *((int16 *)scriptPtr); // condition offset
	} else {
		scriptPtr += 2;
		*opcodePtr = 2; // SNIF
	}

	return 0;
}

/*0x0E*/
int32 ScriptLife::lONEIF(int32 actorIdx, ActorStruct *actor) {
	int32 valueSize = processLifeConditions(actor);
	if (!processLifeOperators(valueSize)) {
		scriptPtr = actor->lifeScript + *((int16 *)scriptPtr); // condition offset
	} else {
		scriptPtr += 2;
		*opcodePtr = 4; // NEVERIF
	}

	return 0;
}

/*0x0F*/
int32 ScriptLife::lELSE(int32 actorIdx, ActorStruct *actor) {
	scriptPtr = actor->lifeScript + *((int16 *)scriptPtr); // offset
	return 0;
}

/*0x11*/
int32 ScriptLife::lBODY(int32 actorIdx, ActorStruct *actor) {
	int32 bodyIdx = *(scriptPtr);
	_engine->_actor->initModelActor(bodyIdx, actorIdx);
	scriptPtr++;
	return 0;
}

/*0x12*/
int32 ScriptLife::lBODY_OBJ(int32 actorIdx, ActorStruct *actor) {
	int32 otherActorIdx = *(scriptPtr++);
	int32 otherBodyIdx = *(scriptPtr++);
	_engine->_actor->initModelActor(otherBodyIdx, otherActorIdx);
	return 0;
}

/*0x13*/
int32 ScriptLife::lANIM(int32 actorIdx, ActorStruct *actor) {
	int32 animIdx = *(scriptPtr++);
	_engine->_animations->initAnim(animIdx, 0, 0, actorIdx);
	return 0;
}

/*0x14*/
int32 ScriptLife::lANIM_OBJ(int32 actorIdx, ActorStruct *actor) {
	int32 otherActorIdx = *(scriptPtr++);
	int32 otherAnimIdx = *(scriptPtr++);
	_engine->_animations->initAnim(otherAnimIdx, 0, 0, otherActorIdx);
	return 0;
}

/*0x15*/
int32 ScriptLife::lSET_LIFE(int32 actorIdx, ActorStruct *actor) {
	actor->positionInLifeScript = *((int16 *)scriptPtr); // offset
	scriptPtr += 2;
	return 0;
}

/*0x16*/
int32 ScriptLife::lSET_LIFE_OBJ(int32 actorIdx, ActorStruct *actor) {
	int32 otherActorIdx = *(scriptPtr++);
	_engine->_scene->sceneActors[otherActorIdx].positionInLifeScript = *((int16 *)scriptPtr); // offset
	scriptPtr += 2;
	return 0;
}

/*0x17*/
int32 ScriptLife::lSET_TRACK(int32 actorIdx, ActorStruct *actor) {
	actor->positionInMoveScript = *((int16 *)scriptPtr); // offset
	scriptPtr += 2;
	return 0;
}

/*0x18*/
int32 ScriptLife::lSET_TRACK_OBJ(int32 actorIdx, ActorStruct *actor) {
	int32 otherActorIdx = *(scriptPtr++);
	_engine->_scene->sceneActors[otherActorIdx].positionInMoveScript = *((int16 *)scriptPtr); // offset
	scriptPtr += 2;
	return 0;
}

/*0x19*/
int32 ScriptLife::lMESSAGE(int32 actorIdx, ActorStruct *actor) {
	int32 textIdx = *((int16 *)scriptPtr);
	scriptPtr += 2;

	_engine->freezeTime();
	if (showDialogueBubble) {
		_engine->_redraw->drawBubble(actorIdx);
	}
	setFontCrossColor(actor->talkColor);
	_engine->_scene->talkingActor = actorIdx;
	drawTextFullscreen(textIdx);
	_engine->unfreezeTime();
	_engine->_redraw->redrawEngineActions(1);

	return 0;
}

/*0x1A*/
int32 ScriptLife::lFALLABLE(int32 actorIdx, ActorStruct *actor) {
	int32 flag = *(scriptPtr++);
	actor->staticFlags.bCanFall = flag & 1;
	return 0;
}

/*0x1B*/
int32 ScriptLife::lSET_DIRMODE(int32 actorIdx, ActorStruct *actor) {
	int32 controlMode = *(scriptPtr++);

	actor->controlMode = controlMode;
	if (controlMode == kFollow) {
		actor->followedActor = *(scriptPtr++);
	}

	return 0;
}

/*0x1C*/
int32 ScriptLife::lSET_DIRMODE_OBJ(int32 actorIdx, ActorStruct *actor) {
	int32 otherActorIdx = *(scriptPtr++);
	int32 controlMode = *(scriptPtr++);

	_engine->_scene->sceneActors[otherActorIdx].controlMode = controlMode;
	if (controlMode == kFollow) {
		_engine->_scene->sceneActors[otherActorIdx].followedActor = *(scriptPtr++);
	}

	return 0;
}

/*0x1D*/
int32 ScriptLife::lCAM_FOLLOW(int32 actorIdx, ActorStruct *actor) {
	int32 followedActorIdx;
	followedActorIdx = *(scriptPtr++);

	if (_engine->_scene->currentlyFollowedActor != followedActorIdx) {
		_engine->_grid->newCameraX = _engine->_scene->sceneActors[followedActorIdx].X >> 9;
		_engine->_grid->newCameraY = _engine->_scene->sceneActors[followedActorIdx].Y >> 8;
		_engine->_grid->newCameraZ = _engine->_scene->sceneActors[followedActorIdx].Z >> 9;

		_engine->_scene->currentlyFollowedActor = followedActorIdx;
		_engine->_redraw->reqBgRedraw = 1;
	}

	return 0;
}

/*0x1E*/
int32 ScriptLife::lSET_BEHAVIOUR(int32 actorIdx, ActorStruct *actor) {
	int32 behavior = *(scriptPtr++);

	_engine->_animations->initAnim(kStanding, 0, 255, 0);
	_engine->_actor->setBehaviour(behavior);

	return 0;
}

/*0x1F*/
int32 ScriptLife::lSET_FLAG_CUBE(int32 actorIdx, ActorStruct *actor) {
	int32 flagIdx = *(scriptPtr++);
	int32 flagValue = *(scriptPtr++);

	_engine->_scene->sceneFlags[flagIdx] = flagValue;

	return 0;
}

/*0x20*/
int32 ScriptLife::lCOMPORTEMENT(int32 actorIdx, ActorStruct *actor) {
	scriptPtr++;
	return 0;
}

/*0x21*/
int32 ScriptLife::lSET_COMPORTEMENT(int32 actorIdx, ActorStruct *actor) {
	actor->positionInLifeScript = *((int16 *)scriptPtr);
	scriptPtr += 2;
	return 0;
}

/*0x22*/
int32 ScriptLife::lSET_COMPORTEMENT_OBJ(int32 actorIdx, ActorStruct *actor) {
	int32 otherActorIdx = *(scriptPtr++);

	_engine->_scene->sceneActors[otherActorIdx].positionInLifeScript = *((int16 *)scriptPtr);
	scriptPtr += 2;

	return 0;
}

/*0x23*/
int32 ScriptLife::lEND_COMPORTEMENT(int32 actorIdx, ActorStruct *actor) {
	return 1; // break
}

/*0x24*/
int32 ScriptLife::lSET_FLAG_GAME(int32 actorIdx, ActorStruct *actor) {
	int32 flagIdx = *(scriptPtr++);
	int32 flagValue = *(scriptPtr++);

	_engine->_gameState->gameFlags[flagIdx] = flagValue;

	return 0;
}

/*0x25*/
int32 ScriptLife::lKILL_OBJ(int32 actorIdx, ActorStruct *actor) {
	int32 otherActorIdx = *(scriptPtr++);

	_engine->_actor->processActorCarrier(otherActorIdx);
	_engine->_scene->sceneActors[otherActorIdx].dynamicFlags.bIsDead = 1;
	_engine->_scene->sceneActors[otherActorIdx].entity = -1;
	_engine->_scene->sceneActors[otherActorIdx].zone = -1;
	_engine->_scene->sceneActors[otherActorIdx].life = 0;

	return 0;
}

/*0x26*/
int32 ScriptLife::lSUICIDE(int32 actorIdx, ActorStruct *actor) {
	_engine->_actor->processActorCarrier(actorIdx);
	actor->dynamicFlags.bIsDead = 1;
	actor->entity = -1;
	actor->zone = -1;
	actor->life = 0;

	return 0;
}

/*0x27*/
int32 ScriptLife::lUSE_ONE_LITTLE_KEY(int32 actorIdx, ActorStruct *actor) {
	_engine->_gameState->inventoryNumKeys--;

	if (_engine->_gameState->inventoryNumKeys < 0) {
		_engine->_gameState->inventoryNumKeys = 0;
	}

	_engine->_redraw->addOverlay(koSprite, SPRITEHQR_KEY, 0, 0, 0, koFollowActor, 1);

	return 0;
}

/*0x28*/
int32 ScriptLife::lGIVE_GOLD_PIECES(int32 actorIdx, ActorStruct *actor) {
	int16 kashes, i, hideRange;
	int16 oldNumKashes = _engine->_gameState->inventoryNumKashes;

	hideRange = 0;

	kashes = *((int16 *)scriptPtr);
	scriptPtr += 2;

	_engine->_gameState->inventoryNumKashes -= kashes;
	if (_engine->_gameState->inventoryNumKashes < 0) {
		_engine->_gameState->inventoryNumKashes = 0;
	}

	_engine->_redraw->addOverlay(koSprite, SPRITEHQR_KASHES, 10, 15, 0, koNormal, 3);

	for (i = 0; i < OVERLAY_MAX_ENTRIES; i++) {
		OverlayListStruct *overlay = &_engine->_redraw->overlayList[i];
		if (overlay->info0 != -1 && overlay->type == koNumberRange) {
			overlay->info0 = _engine->_collision->getAverageValue(overlay->info1, overlay->info0, 100, overlay->lifeTime - _engine->lbaTime - 50);
			overlay->info1 = _engine->_gameState->inventoryNumKashes;
			overlay->lifeTime = _engine->lbaTime + 150;
			hideRange = 1;
			break;
		}
	}

	if (!hideRange) {
		_engine->_redraw->addOverlay(koNumberRange, oldNumKashes, 50, 20, _engine->_gameState->inventoryNumKashes, koNormal, 3);
	}

	return 0;
}

/*0x29*/
int32 ScriptLife::lEND_LIFE(int32 actorIdx, ActorStruct *actor) {
	actor->positionInLifeScript = -1;
	return 1; // break;
}

/*0x2A*/
int32 ScriptLife::lSTOP_L_TRACK(int32 actorIdx, ActorStruct *actor) {
	actor->pausedTrackPtr = actor->currentLabelPtr;
	actor->positionInMoveScript = -1;
	return 0;
}

/*0x2B*/
int32 ScriptLife::lRESTORE_L_TRACK(int32 actorIdx, ActorStruct *actor) {
	actor->positionInMoveScript = actor->pausedTrackPtr;
	return 0;
}

/*0x2C*/
int32 ScriptLife::lMESSAGE_OBJ(int32 actorIdx, ActorStruct *actor) {
	int32 otherActorIdx = *(scriptPtr++);
	int32 textIdx = *((int16 *)scriptPtr);
	scriptPtr += 2;

	_engine->freezeTime();
	if (showDialogueBubble) {
		_engine->_redraw->drawBubble(otherActorIdx);
	}
	setFontCrossColor(_engine->_scene->sceneActors[otherActorIdx].talkColor);
	_engine->_scene->talkingActor = otherActorIdx;
	drawTextFullscreen(textIdx);
	_engine->unfreezeTime();
	_engine->_redraw->redrawEngineActions(1);

	return 0;
}

/*0x2D*/
int32 ScriptLife::lINC_CHAPTER(int32 actorIdx, ActorStruct *actor) {
	_engine->_gameState->gameChapter++;
	return 0;
}

/*0x2E*/
int32 ScriptLife::lFOUND_OBJECT(int32 actorIdx, ActorStruct *actor) {
	int32 item = *(scriptPtr++);

	_engine->freezeTime();
	_engine->_gameState->processFoundItem(item);
	_engine->unfreezeTime();
	_engine->_redraw->redrawEngineActions(1);

	return 0;
}

/*0x2F*/
int32 ScriptLife::lSET_DOOR_LEFT(int32 actorIdx, ActorStruct *actor) {
	int32 distance = *((int16 *)scriptPtr);
	scriptPtr += 2;

	actor->angle = 0x300;
	actor->X = actor->lastX - distance;
	actor->dynamicFlags.bIsSpriteMoving = 0;
	actor->speed = 0;

	return 0;
}

/*0x30*/
int32 ScriptLife::lSET_DOOR_RIGHT(int32 actorIdx, ActorStruct *actor) {
	int32 distance = *((int16 *)scriptPtr);
	scriptPtr += 2;

	actor->angle = 0x100;
	actor->X = actor->lastX + distance;
	actor->dynamicFlags.bIsSpriteMoving = 0;
	actor->speed = 0;

	return 0;
}

/*0x31*/
int32 ScriptLife::lSET_DOOR_UP(int32 actorIdx, ActorStruct *actor) {
	int32 distance = *((int16 *)scriptPtr);
	scriptPtr += 2;

	actor->angle = 0x200;
	actor->Z = actor->lastZ - distance;
	actor->dynamicFlags.bIsSpriteMoving = 0;
	actor->speed = 0;

	return 0;
}

/*0x32*/
int32 ScriptLife::lSET_DOOR_DOWN(int32 actorIdx, ActorStruct *actor) {
	int32 distance = *((int16 *)scriptPtr);
	scriptPtr += 2;

	actor->angle = 0;
	actor->Z = actor->lastZ + distance;
	actor->dynamicFlags.bIsSpriteMoving = 0;
	actor->speed = 0;

	return 0;
}

/*0x33*/
int32 ScriptLife::lGIVE_BONUS(int32 actorIdx, ActorStruct *actor) {
	int32 flag = *(scriptPtr++);

	if (actor->bonusParameter & 0x1F0) {
		_engine->_actor->processActorExtraBonus(actorIdx);
	}

	if (flag != 0) {
		actor->bonusParameter |= 1;
	}

	return 0;
}

/*0x34*/
int32 ScriptLife::lCHANGE_CUBE(int32 actorIdx, ActorStruct *actor) {
	int32 sceneIdx = *(scriptPtr++);
	needChangeScene = sceneIdx;
	heroPositionType = kScene;
	return 0;
}

/*0x35*/
int32 ScriptLife::lOBJ_COL(int32 actorIdx, ActorStruct *actor) {
	int32 collision = *(scriptPtr++);
	if (collision != 0) {
		actor->staticFlags.bComputeCollisionWithObj = 1;
	} else {
		actor->staticFlags.bComputeCollisionWithObj = 0;
	}
	return 0;
}

/*0x36*/
int32 ScriptLife::lBRICK_COL(int32 actorIdx, ActorStruct *actor) {
	int32 collision = *(scriptPtr++);

	actor->staticFlags.bComputeCollisionWithBricks = 0;
	actor->staticFlags.bComputeLowCollision = 0;

	if (collision == 1) {
		actor->staticFlags.bComputeCollisionWithBricks = 1;
	} else if (collision == 2) {
		actor->staticFlags.bComputeCollisionWithBricks = 1;
		actor->staticFlags.bComputeLowCollision = 1;
	}
	return 0;
}

/*0x37*/
int32 ScriptLife::lOR_IF(int32 actorIdx, ActorStruct *actor) {
	int32 valueSize = processLifeConditions(actor);
	if (processLifeOperators(valueSize)) {
		scriptPtr = actor->lifeScript + *((int16 *)scriptPtr); // condition offset
	} else {
		scriptPtr += 2;
	}

	return 0;
}

/*0x38*/
int32 ScriptLife::lINVISIBLE(int32 actorIdx, ActorStruct *actor) {
	actor->staticFlags.bIsHidden = *(scriptPtr++);
	return 0;
}

/*0x39*/
int32 ScriptLife::lZOOM(int32 actorIdx, ActorStruct *actor) {
	zoomScreen = *(scriptPtr++);

	if (zoomScreen && !drawInGameTransBox && cfgfile.SceZoom) {
		fadeToBlack(mainPaletteRGBA);
		initMCGA();
		setBackPal();
		lockPalette = 1;
	} else if (!zoomScreen && drawInGameTransBox) {
		fadeToBlack(mainPaletteRGBA);
		initSVGA();
		setBackPal();
		lockPalette = 1;
		_engine->_redraw->reqBgRedraw = 1;
	}

	return 0;
}

/*0x3A*/
int32 ScriptLife::lPOS_POINT(int32 actorIdx, ActorStruct *actor) {
	int32 trackIdx = *(scriptPtr++);

	destX = sceneTracks[trackIdx].X;
	destY = sceneTracks[trackIdx].Y;
	destZ = sceneTracks[trackIdx].Z;

	actor->X = destX;
	actor->Y = destY;
	actor->Z = destZ;

	return 0;
}

/*0x3B*/
int32 ScriptLife::lSET_MAGIC_LEVEL(int32 actorIdx, ActorStruct *actor) {
	magicLevelIdx = *(scriptPtr++);
	inventoryMagicPoints = magicLevelIdx * 20;
	return 0;
}

/*0x3C*/
int32 ScriptLife::lSUB_MAGIC_POINT(int32 actorIdx, ActorStruct *actor) {
	inventoryMagicPoints = *(scriptPtr++);
	if (inventoryMagicPoints < 0) {
		inventoryMagicPoints = 0;
	}
	return 0;
}

/*0x3D*/
int32 ScriptLife::lSET_LIFE_POINT_OBJ(int32 actorIdx, ActorStruct *actor) {
	int32 otherActorIdx = *(scriptPtr++);
	int32 lifeValue = *(scriptPtr++);

	_engine->_scene->sceneActors[otherActorIdx].life = lifeValue;

	return 0;
}

/*0x3E*/
int32 ScriptLife::lSUB_LIFE_POINT_OBJ(int32 actorIdx, ActorStruct *actor) {
	int32 otherActorIdx = *(scriptPtr++);
	int32 lifeValue = *(scriptPtr++);

	_engine->_scene->sceneActors[otherActorIdx].life -= lifeValue;

	if (_engine->_scene->sceneActors[otherActorIdx].life < 0) {
		_engine->_scene->sceneActors[otherActorIdx].life = 0;
	}

	return 0;
}

/*0x3F*/
int32 ScriptLife::lHIT_OBJ(int32 actorIdx, ActorStruct *actor) {
	int32 otherActorIdx = *(scriptPtr++);
	int32 strengthOfHit = *(scriptPtr++);
	hitActor(actorIdx, otherActorIdx, strengthOfHit, _engine->_scene->sceneActors[otherActorIdx].angle);
	return 0;
}

/*0x40*/
int32 ScriptLife::lPLAY_FLA(int32 actorIdx, ActorStruct *actor) {
	int8 *movie = (int8 *)scriptPtr;
	int32 nameSize = strlen(movie);
	scriptPtr += nameSize + 1;

	playFlaMovie(movie);
	setPalette(paletteRGBA);
	clearScreen();
	flip();

	return 0;
}

/*0x41*/
int32 ScriptLife::lPLAY_MIDI(int32 actorIdx, ActorStruct *actor) {
	int32 midiIdx = *(scriptPtr++);
	playMidiMusic(midiIdx, 0); // TODO: improve this
	return 0;
}

/*0x42*/
int32 ScriptLife::lINC_CLOVER_BOX(int32 actorIdx, ActorStruct *actor) {
	if (_engine->_gameState->inventoryNumLeafsBox < 10) {
		_engine->_gameState->inventoryNumLeafsBox++;
	}
	return 0;
}

/*0x43*/
int32 ScriptLife::lSET_USED_INVENTORY(int32 actorIdx, ActorStruct *actor) {
	int32 item = *(scriptPtr++);
	if (item < 24) {
		_engine->_gameState->inventoryFlags[item] = 1;
	}
	return 0;
}

/*0x44*/
int32 ScriptLife::lADD_CHOICE(int32 actorIdx, ActorStruct *actor) {
	int32 choiceIdx = *((int16 *)scriptPtr);
	scriptPtr += 2;
	gameChoices[numChoices++] = choiceIdx;
	return 0;
}

/*0x45*/
int32 ScriptLife::lASK_CHOICE(int32 actorIdx, ActorStruct *actor) {
	int32 choiceIdx = *((int16 *)scriptPtr);
	scriptPtr += 2;

	_engine->freezeTime();
	if (showDialogueBubble) {
		_engine->_redraw->drawBubble(actorIdx);
	}
	setFontCrossColor(actor->talkColor);
	processGameChoices(choiceIdx);
	numChoices = 0;
	_engine->unfreezeTime();
	_engine->_redraw->redrawEngineActions(1);

	return 0;
}

/*0x46*/
int32 ScriptLife::lBIG_MESSAGE(int32 actorIdx, ActorStruct *actor) {
	int32 textIdx = *((int16 *)scriptPtr);
	scriptPtr += 2;

	_engine->freezeTime();
	textClipFull();
	if (showDialogueBubble) {
		_engine->_redraw->drawBubble(actorIdx);
	}
	setFontCrossColor(actor->talkColor);
	_engine->_scene->talkingActor = actorIdx;
	drawTextFullscreen(textIdx);
	textClipSmall();
	_engine->unfreezeTime();
	_engine->_redraw->redrawEngineActions(1);

	return 0;
}

/*0x47*/
int32 ScriptLife::lINIT_PINGOUIN(int32 actorIdx, ActorStruct *actor) {
	int32 pingouinActor = *(scriptPtr++);
	_engine->_scene->sceneActors[pingouinActor].dynamicFlags.bIsDead = 1;
	mecaPinguinIdx = pingouinActor;
	_engine->_scene->sceneActors[pingouinActor].entity = -1;
	_engine->_scene->sceneActors[pingouinActor].zone = -1;
	return 0;
}

/*0x48*/
int32 ScriptLife::lSET_HOLO_POS(int32 actorIdx, ActorStruct *actor) {
	int32 location = *(scriptPtr++);

	setHolomapPosition(location);
	if (_engine->_gameState->gameFlags[GAMEFLAG_HAS_HOLOMAP]) {
		_engine->_redraw->addOverlay(koInventoryItem, 0, 0, 0, 0, koNormal, 3);
	}

	return 0;
}

/*0x49*/
int32 ScriptLife::lCLR_HOLO_POS(int32 actorIdx, ActorStruct *actor) {
	int32 location = *(scriptPtr++);
	clearHolomapPosition(location);
	return 0;
}

/*0x4A*/
int32 ScriptLife::lADD_FUEL(int32 actorIdx, ActorStruct *actor) {
	_engine->_gameState->inventoryNumGas += *(scriptPtr++);
	if (_engine->_gameState->inventoryNumGas > 100) {
		_engine->_gameState->inventoryNumGas = 100;
	}
	return 0;
}

/*0x4B*/
int32 ScriptLife::lSUB_FUEL(int32 actorIdx, ActorStruct *actor) {
	_engine->_gameState->inventoryNumGas -= *(scriptPtr++);
	if (_engine->_gameState->inventoryNumGas < 0) {
		_engine->_gameState->inventoryNumGas = 0;
	}
	return 0;
}

/*0x4C*/
int32 ScriptLife::lSET_GRM(int32 actorIdx, ActorStruct *actor) {
	cellingGridIdx = *(scriptPtr++);
	initCellingGrid(cellingGridIdx);
	return 0;
}

/*0x4D*/
int32 ScriptLife::lSAY_MESSAGE(int32 actorIdx, ActorStruct *actor) {
	int16 textEntry = *((int16 *)scriptPtr);
	scriptPtr += 2;

	_engine->_redraw->addOverlay(koText, textEntry, 0, 0, actorIdx, koFollowActor, 2);

	_engine->freezeTime();
	initVoxToPlay(textEntry);
	_engine->unfreezeTime();

	return 0;
}

/*04E*/
int32 ScriptLife::lSAY_MESSAGE_OBJ(int32 actorIdx, ActorStruct *actor) {
	int32 otherActorIdx = *(scriptPtr++);
	int16 textEntry = *((int16 *)scriptPtr);
	scriptPtr += 2;

	_engine->_redraw->addOverlay(koText, textEntry, 0, 0, otherActorIdx, koFollowActor, 2);

	_engine->freezeTime();
	initVoxToPlay(textEntry);
	_engine->unfreezeTime();

	return 0;
}

/*0x4F*/
int32 ScriptLife::lFULL_POINT(int32 actorIdx, ActorStruct *actor) {
	sceneHero->life = 50;
	inventoryMagicPoints = magicLevelIdx * 20;
	return 0;
}

/*0x50*/
int32 ScriptLife::lBETA(int32 actorIdx, ActorStruct *actor) {
	int32 newAngle = *((int16 *)scriptPtr);
	scriptPtr += 2;
	actor->angle = newAngle;
	clearRealAngle(actor);
	return 0;
}

/*0x51*/
int32 ScriptLife::lGRM_OFF(int32 actorIdx, ActorStruct *actor) {
	if (cellingGridIdx != -1) {
		useCellingGrid = -1;
		cellingGridIdx = -1;
		createGridMap();
		_engine->_redraw->redrawEngineActions(1);
	}

	return 0;
}

/*0x52*/
int32 ScriptLife::lFADE_PAL_RED(int32 actorIdx, ActorStruct *actor) {
	_engine->freezeTime();
	fadePalRed(mainPaletteRGBA);
	useAlternatePalette = 0;
	_engine->unfreezeTime();
	return 0;
}

/*0x53*/
int32 ScriptLife::lFADE_ALARM_RED(int32 actorIdx, ActorStruct *actor) {
	_engine->freezeTime();
	hqrGetEntry(palette, HQR_RESS_FILE, RESSHQR_ALARMREDPAL);
	convertPalToRGBA(palette, paletteRGBA);
	fadePalRed(paletteRGBA);
	useAlternatePalette = 1;
	_engine->unfreezeTime();
	return 0;
}

/*0x54*/
int32 ScriptLife::lFADE_ALARM_PAL(int32 actorIdx, ActorStruct *actor) {
	_engine->freezeTime();
	hqrGetEntry(palette, HQR_RESS_FILE, RESSHQR_ALARMREDPAL);
	convertPalToRGBA(palette, paletteRGBA);
	adjustCrossPalette(paletteRGBA, mainPaletteRGBA);
	useAlternatePalette = 0;
	_engine->unfreezeTime();
	return 0;
}

/*0x55*/
int32 ScriptLife::lFADE_RED_PAL(int32 actorIdx, ActorStruct *actor) {
	_engine->freezeTime();
	fadeRedPal(mainPaletteRGBA);
	useAlternatePalette = 0;
	_engine->unfreezeTime();
	return 0;
}

/*0x56*/
int32 ScriptLife::lFADE_RED_ALARM(int32 actorIdx, ActorStruct *actor) {
	_engine->freezeTime();
	hqrGetEntry(palette, HQR_RESS_FILE, RESSHQR_ALARMREDPAL);
	convertPalToRGBA(palette, paletteRGBA);
	fadeRedPal(paletteRGBA);
	useAlternatePalette = 1;
	_engine->unfreezeTime();
	return 0;
}

/*0x57*/
int32 ScriptLife::lFADE_PAL_ALARM(int32 actorIdx, ActorStruct *actor) {
	_engine->freezeTime();
	hqrGetEntry(palette, HQR_RESS_FILE, RESSHQR_ALARMREDPAL);
	convertPalToRGBA(palette, paletteRGBA);
	adjustCrossPalette(mainPaletteRGBA, paletteRGBA);
	useAlternatePalette = 1;
	_engine->unfreezeTime();
	return 0;
}

/*0x58*/
int32 ScriptLife::lEXPLODE_OBJ(int32 actorIdx, ActorStruct *actor) {
	int32 otherActorIdx = *(scriptPtr++);
	ActorStruct *otherActor = &_engine->_scene->sceneActors[otherActorIdx];

	addExtraExplode(otherActor->X, otherActor->Y, otherActor->Z); // RECHECK this

	return 0;
}

/*0x59*/
int32 ScriptLife::lBUBBLE_ON(int32 actorIdx, ActorStruct *actor) {
	showDialogueBubble = 1;
	return 0;
}

/*0x5A*/
int32 ScriptLife::lBUBBLE_OFF(int32 actorIdx, ActorStruct *actor) {
	showDialogueBubble = 1;
	return 0;
}

/*0x5B*/
int32 ScriptLife::lASK_CHOICE_OBJ(int32 actorIdx, ActorStruct *actor) {
	int32 otherActorIdx = *(scriptPtr++);
	int32 choiceIdx = *((int16 *)scriptPtr);
	scriptPtr += 2;

	_engine->freezeTime();
	if (showDialogueBubble) {
		_engine->_redraw->drawBubble(otherActorIdx);
	}
	setFontCrossColor(_engine->_scene->sceneActors[otherActorIdx].talkColor);
	processGameChoices(choiceIdx);
	numChoices = 0;
	_engine->unfreezeTime();
	_engine->_redraw->redrawEngineActions(1);

	return 0;
}

/*0x5C*/
int32 ScriptLife::lSET_DARK_PAL(int32 actorIdx, ActorStruct *actor) {
	_engine->freezeTime();
	hqrGetEntry(palette, HQR_RESS_FILE, RESSHQR_DARKPAL);
	if (!lockPalette) {
		convertPalToRGBA(palette, paletteRGBA);
		setPalette(paletteRGBA);
	}
	useAlternatePalette = 1;
	_engine->unfreezeTime();
	return 0;
}

/*0x5D*/
int32 ScriptLife::lSET_NORMAL_PAL(int32 actorIdx, ActorStruct *actor) {
	useAlternatePalette = 0;
	if (!lockPalette) {
		setPalette(mainPaletteRGBA);
	}
	return 0;
}

/*0x5E*/
int32 ScriptLife::lMESSAGE_SENDELL(int32 actorIdx, ActorStruct *actor) {
	int32 tmpFlagDisplayText;

	_engine->freezeTime();
	fadeToBlack(paletteRGBA);
	loadImage(25, 1);
	textClipFull();
	setFontCrossColor(15);
	newGameVar4 = 0;
	tmpFlagDisplayText = cfgfile.FlagDisplayText;
	cfgfile.FlagDisplayText = 1;
	drawTextFullscreen(6);
	newGameVar4 = 1;
	textClipSmall();
	fadeToBlack(paletteRGBACustom);
	clearScreen();
	setPalette(paletteRGBA);
	cfgfile.FlagDisplayText = tmpFlagDisplayText;

	do {
		readKeys();
	} while (skipIntro || skippedKey);

	_engine->unfreezeTime();

	return 0;
}

/*0x5F*/
int32 ScriptLife::lANIM_SET(int32 actorIdx, ActorStruct *actor) {
	int32 animIdx = *(scriptPtr++);

	actor->anim = -1;
	actor->previousAnimIdx = -1;
	_engine->_animations->initAnim(animIdx, 0, 0, actorIdx);

	return 0;
}

/*0x60*/
int32 ScriptLife::lHOLOMAP_TRAJ(int32 actorIdx, ActorStruct *actor) {
	scriptPtr++; // TODO
	return -1;
}

/*0x61*/
int32 ScriptLife::lGAME_OVER(int32 actorIdx, ActorStruct *actor) {
	sceneHero->dynamicFlags.bAnimEnded = 1;
	sceneHero->life = 0;
	_engine->_gameState->inventoryNumLeafs = 0;
	return 1; // break
}

/*0x62*/
int32 ScriptLife::lTHE_END(int32 actorIdx, ActorStruct *actor) {
	quitGame = 1;
	_engine->_gameState->inventoryNumLeafs = 0;
	sceneHero->life = 50;
	inventoryMagicPoints = 80;
	currentSceneIdx = 113;
	_engine->_actor->heroBehaviour = previousHeroBehaviour;
	newHeroX = -1;
	sceneHero->angle = previousHeroAngle;
	saveGame();
	return 1; // break;
}

/*0x63*/
int32 ScriptLife::lMIDI_OFF(int32 actorIdx, ActorStruct *actor) {
	stopMidiMusic();
	return 0;
}

/*0x64*/
int32 ScriptLife::lPLAY_CD_TRACK(int32 actorIdx, ActorStruct *actor) {
	int32 track = *(scriptPtr++);
	playTrackMusic(track);
	return 0;
}

/*0x65*/
int32 ScriptLife::lPROJ_ISO(int32 actorIdx, ActorStruct *actor) {
	setOrthoProjection(311, 240, 512);
	setBaseTranslation(0, 0, 0);
	setBaseRotation(0, 0, 0);
	setLightVector(alphaLight, betaLight, 0);
	return 0;
}

/*0x66*/
int32 ScriptLife::lPROJ_3D(int32 actorIdx, ActorStruct *actor) {
	copyScreen(frontVideoBuffer, workVideoBuffer);
	flip();
	changeRoomVar10 = 0;

	setCameraPosition(320, 240, 128, 1024, 1024);
	setCameraAngle(0, 1500, 0, 25, -128, 0, 13000);
	setLightVector(896, 950, 0);

	initTextBank(1);

	return 0;
}

/*0x67*/
int32 ScriptLife::lTEXT(int32 actorIdx, ActorStruct *actor) {
	int32 textSize, textBoxRight;
	int32 textIdx = *((int16 *)scriptPtr);
	scriptPtr += 2;

	if (drawVar1 < 440) {
		if (cfgfile.Version == USA_VERSION) {
			if (!textIdx) {
				textIdx = 16;
			}
		}

		getMenuText(textIdx, textStr);
		textSize = textBoxRight = getTextSize(textStr);
		setFontColor(15);
		drawText(0, drawVar1, textStr);
		if (textSize > 639) {
			textBoxRight = 639;
		}

		drawVar1 += 40;
		copyBlockPhys(0, drawVar1, textBoxRight, drawVar1);
	}

	return 0;
}

/*0x68*/
int32 ScriptLife::lCLEAR_TEXT(int32 actorIdx, ActorStruct *actor) {
	drawVar1 = 0;
	drawSplittedBox(0, 0, 639, 240, 0);
	copyBlockPhys(0, 0, 639, 240);
	return 0;
}

/*0x69*/
int32 ScriptLife::lBRUTAL_EXIT(int32 actorIdx, ActorStruct *actor) {
	quitGame = 0;
	return 1; // break
}

static const ScriptLifeFunction function_map[] = {
    /*0x00*/ MAPFUNC("END", lEND),
    /*0x01*/ MAPFUNC("NOP", lNOP),
    /*0x02*/ MAPFUNC("SNIF", lSNIF),
    /*0x03*/ MAPFUNC("OFFSET", lOFFSET),
    /*0x04*/ MAPFUNC("NEVERIF", lNEVERIF),
    /*0x05*/ MAPFUNC("", lEMPTY), // unused
    /*0x06*/ MAPFUNC("NO_IF", lNO_IF),
    /*0x07*/ MAPFUNC("", lEMPTY), // unused
    /*0x08*/ MAPFUNC("", lEMPTY), // unused
    /*0x09*/ MAPFUNC("", lEMPTY), // unused
    /*0x0A*/ MAPFUNC("LABEL", lLABEL),
    /*0x0B*/ MAPFUNC("RETURN", lRETURN),
    /*0x0C*/ MAPFUNC("IF", lIF),
    /*0x0D*/ MAPFUNC("SWIF", lSWIF),
    /*0x0E*/ MAPFUNC("ONEIF", lONEIF),
    /*0x0F*/ MAPFUNC("ELSE", lELSE),
    /*0x10*/ MAPFUNC("ENDIF", lEMPTY), // unused
    /*0x11*/ MAPFUNC("BODY", lBODY),
    /*0x12*/ MAPFUNC("BODY_OBJ", lBODY_OBJ),
    /*0x13*/ MAPFUNC("ANIM", lANIM),
    /*0x14*/ MAPFUNC("ANIM_OBJ", lANIM_OBJ),
    /*0x15*/ MAPFUNC("SET_LIFE", lSET_LIFE),
    /*0x16*/ MAPFUNC("SET_LIFE_OBJ", lSET_LIFE_OBJ),
    /*0x17*/ MAPFUNC("SET_TRACK", lSET_TRACK),
    /*0x18*/ MAPFUNC("SET_TRACK_OBJ", lSET_TRACK_OBJ),
    /*0x19*/ MAPFUNC("MESSAGE", lMESSAGE),
    /*0x1A*/ MAPFUNC("FALLABLE", lFALLABLE),
    /*0x1B*/ MAPFUNC("SET_DIRMODE", lSET_DIRMODE),
    /*0x1C*/ MAPFUNC("SET_DIRMODE_OBJ", lSET_DIRMODE_OBJ),
    /*0x1D*/ MAPFUNC("CAM_FOLLOW", lCAM_FOLLOW),
    /*0x1E*/ MAPFUNC("SET_BEHAVIOUR", lSET_BEHAVIOUR),
    /*0x1F*/ MAPFUNC("SET_FLAG_CUBE", lSET_FLAG_CUBE),
    /*0x20*/ MAPFUNC("COMPORTEMENT", lCOMPORTEMENT),
    /*0x21*/ MAPFUNC("SET_COMPORTEMENT", lSET_COMPORTEMENT),
    /*0x22*/ MAPFUNC("SET_COMPORTEMENT_OBJ", lSET_COMPORTEMENT_OBJ),
    /*0x23*/ MAPFUNC("END_COMPORTEMENT", lEND_COMPORTEMENT),
    /*0x24*/ MAPFUNC("SET_FLAG_GAME", lSET_FLAG_GAME),
    /*0x25*/ MAPFUNC("KILL_OBJ", lKILL_OBJ),
    /*0x26*/ MAPFUNC("SUICIDE", lSUICIDE),
    /*0x27*/ MAPFUNC("USE_ONE_LITTLE_KEY", lUSE_ONE_LITTLE_KEY),
    /*0x28*/ MAPFUNC("GIVE_GOLD_PIECES", lGIVE_GOLD_PIECES),
    /*0x29*/ MAPFUNC("END_LIFE", lEND_LIFE),
    /*0x2A*/ MAPFUNC("STOP_L_TRACK", lSTOP_L_TRACK),
    /*0x2B*/ MAPFUNC("RESTORE_L_TRACK", lRESTORE_L_TRACK),
    /*0x2C*/ MAPFUNC("MESSAGE_OBJ", lMESSAGE_OBJ),
    /*0x2D*/ MAPFUNC("INC_CHAPTER", lINC_CHAPTER),
    /*0x2E*/ MAPFUNC("FOUND_OBJECT", lFOUND_OBJECT),
    /*0x2F*/ MAPFUNC("SET_DOOR_LEFT", lSET_DOOR_LEFT),
    /*0x30*/ MAPFUNC("SET_DOOR_RIGHT", lSET_DOOR_RIGHT),
    /*0x31*/ MAPFUNC("SET_DOOR_UP", lSET_DOOR_UP),
    /*0x32*/ MAPFUNC("SET_DOOR_DOWN", lSET_DOOR_DOWN),
    /*0x33*/ MAPFUNC("GIVE_BONUS", lGIVE_BONUS),
    /*0x34*/ MAPFUNC("CHANGE_CUBE", lCHANGE_CUBE),
    /*0x35*/ MAPFUNC("OBJ_COL", lOBJ_COL),
    /*0x36*/ MAPFUNC("BRICK_COL", lBRICK_COL),
    /*0x37*/ MAPFUNC("OR_IF", lOR_IF),
    /*0x38*/ MAPFUNC("INVISIBLE", lINVISIBLE),
    /*0x39*/ MAPFUNC("ZOOM", lZOOM),
    /*0x3A*/ MAPFUNC("POS_POINT", lPOS_POINT),
    /*0x3B*/ MAPFUNC("SET_MAGIC_LEVEL", lSET_MAGIC_LEVEL),
    /*0x3C*/ MAPFUNC("SUB_MAGIC_POINT", lSUB_MAGIC_POINT),
    /*0x3D*/ MAPFUNC("SET_LIFE_POINT_OBJ", lSET_LIFE_POINT_OBJ),
    /*0x3E*/ MAPFUNC("SUB_LIFE_POINT_OBJ", lSUB_LIFE_POINT_OBJ),
    /*0x3F*/ MAPFUNC("HIT_OBJ", lHIT_OBJ),
    /*0x40*/ MAPFUNC("PLAY_FLA", lPLAY_FLA),
    /*0x41*/ MAPFUNC("PLAY_MIDI", lPLAY_MIDI),
    /*0x42*/ MAPFUNC("INC_CLOVER_BOX", lINC_CLOVER_BOX),
    /*0x43*/ MAPFUNC("SET_USED_INVENTORY", lSET_USED_INVENTORY),
    /*0x44*/ MAPFUNC("ADD_CHOICE", lADD_CHOICE),
    /*0x45*/ MAPFUNC("ASK_CHOICE", lASK_CHOICE),
    /*0x46*/ MAPFUNC("BIG_MESSAGE", lBIG_MESSAGE),
    /*0x47*/ MAPFUNC("INIT_PINGOUIN", lINIT_PINGOUIN),
    /*0x48*/ MAPFUNC("SET_HOLO_POS", lSET_HOLO_POS),
    /*0x49*/ MAPFUNC("CLR_HOLO_POS", lCLR_HOLO_POS),
    /*0x4A*/ MAPFUNC("ADD_FUEL", lADD_FUEL),
    /*0x4B*/ MAPFUNC("SUB_FUEL", lSUB_FUEL),
    /*0x4C*/ MAPFUNC("SET_GRM", lSET_GRM),
    /*0x4D*/ MAPFUNC("SAY_MESSAGE", lSAY_MESSAGE),
    /*0x4E*/ MAPFUNC("SAY_MESSAGE_OBJ", lSAY_MESSAGE_OBJ),
    /*0x4F*/ MAPFUNC("FULL_POINT", lFULL_POINT),
    /*0x50*/ MAPFUNC("BETA", lBETA),
    /*0x51*/ MAPFUNC("GRM_OFF", lGRM_OFF),
    /*0x52*/ MAPFUNC("FADE_PAL_RED", lFADE_PAL_RED),
    /*0x53*/ MAPFUNC("FADE_ALARM_RED", lFADE_ALARM_RED),
    /*0x54*/ MAPFUNC("FADE_ALARM_PAL", lFADE_ALARM_PAL),
    /*0x55*/ MAPFUNC("FADE_RED_PAL", lFADE_RED_PAL),
    /*0x56*/ MAPFUNC("FADE_RED_ALARM", lFADE_RED_ALARM),
    /*0x57*/ MAPFUNC("FADE_PAL_ALARM", lFADE_PAL_ALARM),
    /*0x58*/ MAPFUNC("EXPLODE_OBJ", lEXPLODE_OBJ),
    /*0x59*/ MAPFUNC("BUBBLE_ON", lBUBBLE_ON),
    /*0x5A*/ MAPFUNC("BUBBLE_OFF", lBUBBLE_OFF),
    /*0x5B*/ MAPFUNC("ASK_CHOICE_OBJ", lASK_CHOICE_OBJ),
    /*0x5C*/ MAPFUNC("SET_DARK_PAL", lSET_DARK_PAL),
    /*0x5D*/ MAPFUNC("SET_NORMAL_PAL", lSET_NORMAL_PAL),
    /*0x5E*/ MAPFUNC("MESSAGE_SENDELL", lMESSAGE_SENDELL),
    /*0x5F*/ MAPFUNC("ANIM_SET", lANIM_SET),
    /*0x60*/ MAPFUNC("HOLOMAP_TRAJ", lHOLOMAP_TRAJ),
    /*0x61*/ MAPFUNC("GAME_OVER", lGAME_OVER),
    /*0x62*/ MAPFUNC("THE_END", lTHE_END),
    /*0x63*/ MAPFUNC("MIDI_OFF", lMIDI_OFF),
    /*0x64*/ MAPFUNC("PLAY_CD_TRACK", lPLAY_CD_TRACK),
    /*0x65*/ MAPFUNC("PROJ_ISO", lPROJ_ISO),
    /*0x66*/ MAPFUNC("PROJ_3D", lPROJ_3D),
    /*0x67*/ MAPFUNC("TEXT", lTEXT),
    /*0x68*/ MAPFUNC("CLEAR_TEXT", lCLEAR_TEXT),
    /*0x69*/ MAPFUNC("BRUTAL_EXIT", lBRUTAL_EXIT)};

/** Process actor move script
	@param actorIdx Current processed actor index */
void ScriptLife::processLifeScript(int32 actorIdx) {
	int32 end, scriptOpcode;
	ActorStruct *actor;

	actor = &_engine->_scene->sceneActors[actorIdx];
	scriptPtr = actor->lifeScript + actor->positionInLifeScript;

	end = -2;

	do {
		opcodePtr = scriptPtr;
		scriptOpcode = *(scriptPtr++);

		if (scriptOpcode <= 105) {
			end = function_map[scriptOpcode].function(actorIdx, actor);
		} else {
			error("Actor %d with wrong offset/opcode - Offset: %d\n", actorIdx, actor->positionInLifeScript);
		}

		if (end < 0) { // show error message
			warning("Actor %d Life script [%s] not implemented\n", actorIdx, function_map[scriptOpcode].name);
		}

	} while (end != 1);
}

} // namespace TwinE
