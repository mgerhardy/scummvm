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

#include "twine/scene/wagon.h"
#include "twine/scene/move3d.h"
#include "twine/parser/anim.h"
#include "twine/parser/body.h"
#include "twine/renderer/renderer.h"
#include "twine/scene/gamestate.h"
#include "twine/scene/grid.h"
#include "twine/scene/scene.h"
#include "twine/twine.h"

namespace TwinE {

namespace {
// Straight rails
static constexpr int32 RAIL_NORD_SUD = 55;
static constexpr int32 RAIL_EST_OUEST = 54;
static constexpr int32 RAIL_UP_NORD = 64;
static constexpr int32 RAIL_UP_SUD = 66;
static constexpr int32 RAIL_UP_EST = 67;
static constexpr int32 RAIL_UP_OUEST = 65;

// Curves
static constexpr int32 RAIL_NORD_EST = 52;
static constexpr int32 RAIL_NORD_OUEST = 51;
static constexpr int32 RAIL_SUD_EST = 50;
static constexpr int32 RAIL_SUD_OUEST = 53;

// Switches
static constexpr int32 RAIL_NORD_NORD_EST = 57;
static constexpr int32 RAIL_NORD_NORD_OUEST = 56;
static constexpr int32 RAIL_SUD_SUD_EST = 58;
static constexpr int32 RAIL_SUD_SUD_OUEST = 59;
static constexpr int32 RAIL_OUEST_OUEST_SUD = 60;
static constexpr int32 RAIL_OUEST_OUEST_NORD = 61;
static constexpr int32 RAIL_EST_EST_SUD = 62;
static constexpr int32 RAIL_EST_EST_NORD = 63;

// Esmer equivalents
static constexpr int32 RAIL_E_NORD_SUD = 15;
static constexpr int32 RAIL_E_EST_OUEST = 16;
static constexpr int32 RAIL_E_UP_NORD = 59;
static constexpr int32 RAIL_E_UP_SUD = 60;
static constexpr int32 RAIL_E_UP_EST = 53;
static constexpr int32 RAIL_E_UP_OUEST = 58;
static constexpr int32 RAIL_E_NORD_EST = 17;
static constexpr int32 RAIL_E_NORD_OUEST = 19;
static constexpr int32 RAIL_E_SUD_EST = 20;
static constexpr int32 RAIL_E_SUD_OUEST = 18;
static constexpr int32 RAIL_E_NORD_NORD_EST = 67;
static constexpr int32 RAIL_E_NORD_NORD_OUEST = 68;
static constexpr int32 RAIL_E_SUD_SUD_EST = 51;
static constexpr int32 RAIL_E_SUD_SUD_OUEST = 61;
static constexpr int32 RAIL_E_OUEST_OUEST_SUD = 22;
static constexpr int32 RAIL_E_OUEST_OUEST_NORD = 21;
static constexpr int32 RAIL_E_EST_EST_SUD = 63;
static constexpr int32 RAIL_E_EST_EST_NORD = 65;

static constexpr int32 GAMEFLAG_PLANETE_ESMER = 254;
static constexpr int32 PAS_ESSIEU = 30;

static bool isSwitchZoneOn(const TwinEEngine *engine, const ActorStruct *actor) {
	if (actor->_railZoneIdx < 0) {
		return false;
	}
	const ZoneStruct &zone = engine->_scene->_sceneZones[actor->_railZoneIdx];
	return zone.infoData.generic.info1 != 0;
}

} // namespace

void Wagon::DoAnimWagon(ActorStruct *ptrobj) {
	if (ptrobj == nullptr) {
		return;
	}

	if (ptrobj->_body != -1 && ptrobj->_entityDataPtr) {
		BodyData &bodyData = ptrobj->_entityDataPtr->getBody(ptrobj->_body);
		if (bodyData.isAnimated()) {
			AdjustEssieuWagonAvant(ptrobj, ptrobj->SizeSHit);
			AdjustEssieuWagonArriere(ptrobj, ptrobj->SizeSHit);

			if (ptrobj->_srot) {
				const int32 va = ptrobj->_srot / (15 * 20);
				BoneFrame *wheelBone0 = bodyData.getBoneState(2);
				BoneFrame *wheelBone1 = bodyData.getBoneState(3);
				if (wheelBone0 && wheelBone1) {
					wheelBone0->x = (int16)(_engine->timerRef * va);
					wheelBone1->x = (int16)(_engine->timerRef * va);
				}
			}
		}
	}

	if (!ptrobj->_srot) {
		return;
	}

	int32 &info = ptrobj->_delayInMillis;
	int32 &info1 = ptrobj->_cropTop;
	const int32 &info2 = ptrobj->_cropRight;
	const int32 &info3 = ptrobj->_cropBottom;
	IVec3 &processActor = ptrobj->_processActor;

	switch (info) {
	case 0: // straight rail
		if (info1) {
			initMove(_engine, &ptrobj->_boundAngle.move, ptrobj->_srot);
			info1 = 0;
		}

		{
			const int32 distance = getDeltaMove(_engine, &ptrobj->_boundAngle.move);
			const IVec2 dest = _engine->_renderer->rotate(0, distance, ptrobj->_beta);
			processActor.x = ptrobj->_posObj.x + dest.x;
			processActor.z = ptrobj->_posObj.z + dest.y;
		}
		break;

	case -1:
	case 1: // curve or switch
		if (info1 != 1) {
			const int32 speed = (51200 * ptrobj->_srot) / 60319;  // TODO: magic numbers
			const int32 endBeta = ptrobj->_beta + (LBAAngles::ANGLE_90) * info;
			initBoundAngleMove(_engine, &ptrobj->_boundAngle, speed, ptrobj->_beta, endBeta);
			info1 = 1;
		}

		{
			const int32 angle = getBoundAngleMove(_engine, &ptrobj->_boundAngle);
			const IVec2 dest = _engine->_renderer->rotate(768 * info, 0, angle);  // TODO: LBAAngles
			processActor.x = info2 - dest.x;
			processActor.z = info3 - dest.y;
			ptrobj->_beta = angle;

			if (!getSpeedMove(&ptrobj->_boundAngle.move)) {
				info1 = -1;
			}
		}
		break;

	default:
		break;
	}
}

void Wagon::DoDirWagon(ActorStruct *ptrobj) {
	if (ptrobj == nullptr) {
		return;
	}

	int32 brick = _engine->_grid->worldCodeBrick(ptrobj->_posObj.x, ptrobj->_posObj.y - 1, ptrobj->_posObj.z);
	brick = GetNumBrickWagon(brick);

	int32 &info = ptrobj->_delayInMillis; // Info
	int32 &info1 = ptrobj->_cropTop;      // Info1
	int32 &info2 = ptrobj->_cropRight;    // Info2
	int32 &info3 = ptrobj->_cropBottom;   // Info3
	int32 &sprite = ptrobj->_sprite;      // Sprite
	const int32 angle90 = LBAAngles::ANGLE_90;
	const int32 angle180 = LBAAngles::ANGLE_180;
	const int32 angle270 = LBAAngles::ANGLE_270;
	const int32 beta = ClampAngle(ptrobj->_beta);

	auto alignBetaToQuadrant = [&]() {
		ptrobj->_beta = (ptrobj->_beta / angle90) * angle90;
	};
	auto centerX = [&]() {
		ptrobj->_posObj.x = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + DEMI_BRICK_XZ + 1;
	};
	auto centerZ = [&]() {
		ptrobj->_posObj.z = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + DEMI_BRICK_XZ + 1;
	};

	switch (brick) {
	case RAIL_NORD_EST:
		if (info1 != 1) {
			alignBetaToQuadrant();
			if (beta <= angle180) {
				info = -1;
				info2 = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + 2 * SIZE_BRICK_XZ;
				info3 = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + SIZE_BRICK_XZ - 1;
			} else {
				info = 1;
				info2 = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + SIZE_BRICK_XZ - 1;
				info3 = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + 2 * SIZE_BRICK_XZ;
			}
			sprite = 0;
		}
		break;
	case RAIL_SUD_EST:
		if (info1 != 1) {
			alignBetaToQuadrant();
			if (beta < angle180) {
				info = 1;
				info2 = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + 2 * SIZE_BRICK_XZ;
				info3 = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + 1;
			} else {
				info = -1;
				info2 = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + SIZE_BRICK_XZ - 1;
				info3 = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ - SIZE_BRICK_XZ - 1;
			}
			sprite = 0;
		}
		break;
	case RAIL_NORD_OUEST:
		if (info1 != 1) {
			alignBetaToQuadrant();
			if (beta <= angle90) {
				info = -1;
				info2 = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + 1;
				info3 = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + 2 * SIZE_BRICK_XZ;
			} else {
				info = 1;
				info2 = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ - SIZE_BRICK_XZ - 1;
				info3 = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + SIZE_BRICK_XZ - 1;
			}
			sprite = 0;
		}
		break;
	case RAIL_SUD_OUEST:
		if (info1 != 1) {
			alignBetaToQuadrant();
			if (beta <= angle180 && beta >= angle90) {
				info = 1;
				info2 = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + 1;
				info3 = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ - SIZE_BRICK_XZ - 1;
			} else {
				info = -1;
				info2 = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ - SIZE_BRICK_XZ - 1;
				info3 = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + 1;
			}
			sprite = 0;
		}
		break;
	case RAIL_NORD_NORD_EST:
		if (!sprite) {
			if (beta == 0) {
				sprite = 1;
			} else if (beta == angle180) {
				sprite = isSwitchZoneOn(_engine, ptrobj) ? 2 : 3;
			} else {
				sprite = 4;
			}
		}
		switch (sprite) {
		case 1:
		case 3:
			info = 0;
			info2 = 0;
			centerX();
			break;
		case 2:
			if (info1 != 1) {
				info = -1;
				info2 = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + 2 * SIZE_BRICK_XZ;
				info3 = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + SIZE_BRICK_XZ - 1;
			}
			break;
		case 4:
			if (info1 != 1) {
				info = 1;
				info2 = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + SIZE_BRICK_XZ - 1;
				info3 = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + 2 * SIZE_BRICK_XZ;
			}
			break;
		default:
			break;
		}
		break;
	case RAIL_NORD_NORD_OUEST:
		if (!sprite) {
			if (beta == 0) {
				sprite = 1;
			} else if (beta == angle180) {
				sprite = isSwitchZoneOn(_engine, ptrobj) ? 2 : 3;
			} else {
				sprite = 4;
			}
		}
		switch (sprite) {
		case 1:
		case 3:
			info = 0;
			info2 = 0;
			centerX();
			break;
		case 2:
			if (info1 != 1) {
				info = 1;
				info2 = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ - SIZE_BRICK_XZ - 1;
				info3 = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + SIZE_BRICK_XZ - 1;
			}
			break;
		case 4:
			if (info1 != 1) {
				info = -1;
				info2 = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + 1;
				info3 = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + 2 * SIZE_BRICK_XZ;
			}
			break;
		default:
			break;
		}
		break;
	case RAIL_SUD_SUD_EST:
		if (!sprite) {
			if (beta == angle180) {
				sprite = 1;
			} else if (beta == 0) {
				sprite = isSwitchZoneOn(_engine, ptrobj) ? 2 : 3;
			} else {
				sprite = 4;
			}
		}
		switch (sprite) {
		case 1:
		case 3:
			info = 0;
			info2 = 0;
			centerX();
			break;
		case 2:
			if (info1 != 1) {
				info = 1;
				info2 = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + 2 * SIZE_BRICK_XZ;
				info3 = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + 1;
			}
			break;
		case 4:
			if (info1 != 1 && beta != LBAAngles::ANGLE_270) {
				info = -1;
				info2 = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + SIZE_BRICK_XZ - 1;
				info3 = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ - SIZE_BRICK_XZ - 1;
			}
			break;
		default:
			break;
		}
		break;
	case RAIL_SUD_SUD_OUEST:
		if (!sprite) {
			if (beta == angle180) {
				sprite = 1;
			} else if (beta == 0) {
				sprite = isSwitchZoneOn(_engine, ptrobj) ? 2 : 3;
			} else {
				sprite = 4;
			}
		}
		switch (sprite) {
		case 1:
		case 3:
			info = 0;
			info2 = 0;
			centerX();
			break;
		case 2:
			if (info1 != 1) {
				info = -1;
				info2 = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ - SIZE_BRICK_XZ - 1;
				info3 = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + 1;
			}
			break;
		case 4:
			if (info1 != 1) {
				info = 1;
				info2 = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + 1;
				info3 = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ - SIZE_BRICK_XZ - 1;
			}
			break;
		default:
			break;
		}
		break;
	case RAIL_EST_EST_NORD:
		if (!sprite) {
			if (beta == angle270) {
				sprite = 1;
			} else if (beta == angle90) {
				sprite = isSwitchZoneOn(_engine, ptrobj) ? 2 : 3;
			} else {
				sprite = 4;
			}
		}
		switch (sprite) {
		case 1:
		case 3:
			info = 0;
			info2 = 0;
			centerZ();
			break;
		case 2:
			if (info1 != 1) {
				info = 1;
				info2 = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + 1;
				info3 = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ - SIZE_BRICK_XZ - 1;
			}
			break;
		case 4:
			if (info1 != 1) {
				info = -1;
				info2 = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ - SIZE_BRICK_XZ - 1;
				info3 = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + 1;
			}
			break;
		default:
			break;
		}
		break;
	case RAIL_EST_EST_SUD:
		if (!sprite) {
			if (beta == angle270) {
				sprite = 1;
			} else if (beta == angle90) {
				sprite = isSwitchZoneOn(_engine, ptrobj) ? 2 : 3;
			} else {
				sprite = 4;
			}
		}
		switch (sprite) {
		case 1:
		case 3:
			info = 0;
			info2 = 0;
			centerZ();
			break;
		case 2:
			if (info1 != 1) {
				info = -1;
				info2 = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + 1;
				info3 = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + 2 * SIZE_BRICK_XZ;
			}
			break;
		case 4:
			if (info1 != 1) {
				info = 1;
				info2 = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ - SIZE_BRICK_XZ - 1;
				info3 = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + SIZE_BRICK_XZ - 1;
			}
			break;
		default:
			break;
		}
		break;
	case RAIL_OUEST_OUEST_NORD:
		if (!sprite) {
			if (beta == angle90) {
				sprite = 1;
			} else if (beta == angle270) {
				sprite = isSwitchZoneOn(_engine, ptrobj) ? 2 : 3;
			} else {
				sprite = 4;
			}
		}
		switch (sprite) {
		case 1:
		case 3:
			info = 0;
			info2 = 0;
			centerZ();
			break;
		case 2:
			if (info1 != 1) {
				info = -1;
				info2 = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + SIZE_BRICK_XZ - 1;
				info3 = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ - SIZE_BRICK_XZ - 1;
			}
			break;
		case 4:
			if (info1 != 1) {
				info = 1;
				info2 = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + 2 * SIZE_BRICK_XZ;
				info3 = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + 1;
			}
			break;
		default:
			break;
		}
		break;
	case RAIL_OUEST_OUEST_SUD:
		if (!sprite) {
			if (beta == angle90) {
				sprite = 1;
			} else if (beta == angle270) {
				sprite = isSwitchZoneOn(_engine, ptrobj) ? 2 : 3;
			} else {
				sprite = 4;
			}
		}
		switch (sprite) {
		case 1:
		case 3:
			info = 0;
			info2 = 0;
			centerZ();
			break;
		case 2:
			if (info1 != 1) {
				info = 1;
				info2 = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + SIZE_BRICK_XZ - 1;
				info3 = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + 2 * SIZE_BRICK_XZ;
			}
			break;
		case 4:
			if (info1 != 1) {
				info = -1;
				info2 = (ptrobj->_posObj.x / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + 2 * SIZE_BRICK_XZ;
				info3 = (ptrobj->_posObj.z / SIZE_BRICK_XZ) * SIZE_BRICK_XZ + SIZE_BRICK_XZ - 1;
			}
			break;
		default:
			break;
		}
		break;
	case RAIL_EST_OUEST:
		sprite = 0;
		info = 0;
		info2 = 0;
		centerZ();
		break;
	case RAIL_NORD_SUD:
	case RAIL_UP_NORD:
	case RAIL_UP_SUD:
		sprite = 0;
		info = 0;
		info2 = 0;
		centerX();
		break;
	case RAIL_UP_OUEST:
	case RAIL_UP_EST:
		sprite = 0;
		info = 0;
		info2 = 0;
		centerZ();
		break;
	default:
		break;
	}

	ptrobj->SizeSHit = (int16)brick;
}

int32 Wagon::GetNumBrickWagon(int32 brick) {
	if (_engine->_gameState->hasGameFlag(GAMEFLAG_PLANETE_ESMER)) {
		switch (brick) {
		case RAIL_E_NORD_SUD: return RAIL_NORD_SUD;
		case RAIL_E_EST_OUEST: return RAIL_EST_OUEST;
		case RAIL_E_UP_NORD: return RAIL_UP_NORD;
		case RAIL_E_UP_SUD: return RAIL_UP_SUD;
		case RAIL_E_UP_EST: return RAIL_UP_EST;
		case RAIL_E_UP_OUEST: return RAIL_UP_OUEST;
		case RAIL_E_NORD_EST: return RAIL_NORD_EST;
		case RAIL_E_NORD_OUEST: return RAIL_NORD_OUEST;
		case RAIL_E_SUD_EST: return RAIL_SUD_EST;
		case RAIL_E_SUD_OUEST: return RAIL_SUD_OUEST;
		case RAIL_E_NORD_NORD_EST: return RAIL_NORD_NORD_EST;
		case RAIL_E_NORD_NORD_OUEST: return RAIL_NORD_NORD_OUEST;
		case RAIL_E_SUD_SUD_EST: return RAIL_SUD_SUD_EST;
		case RAIL_E_SUD_SUD_OUEST: return RAIL_SUD_SUD_OUEST;
		case RAIL_E_OUEST_OUEST_SUD: return RAIL_OUEST_OUEST_SUD;
		case RAIL_E_OUEST_OUEST_NORD: return RAIL_OUEST_OUEST_NORD;
		case RAIL_E_EST_EST_SUD: return RAIL_EST_EST_SUD;
		case RAIL_E_EST_EST_NORD: return RAIL_EST_EST_NORD;
		default:
			break;
		}
	}
	return brick;
}

void Wagon::AdjustEssieuWagonAvant(ActorStruct *ptrobj, int32 brickw) {
	if (ptrobj == nullptr || ptrobj->_body == -1 || !ptrobj->_entityDataPtr) {
		return;
	}

	BodyData &bodyData = ptrobj->_entityDataPtr->getBody(ptrobj->_body);
	if (!bodyData.isAnimated()) {
		return;
	}

	BoneFrame *frontAxle = bodyData.getBoneState(0);
	if (frontAxle == nullptr) {
		return;
	}

	bool diff = false;
	const IVec2 offset = _engine->_renderer->rotate(0, 400, ptrobj->_beta);
	int32 brick = _engine->_grid->worldCodeBrick(ptrobj->_posObj.x + offset.x, ptrobj->_posObj.y - 1, ptrobj->_posObj.z + offset.y);
	if (!brick) {
		brick = _engine->_grid->worldCodeBrick(ptrobj->_posObj.x + offset.x, ptrobj->_posObj.y - SIZE_BRICK_Y - 1, ptrobj->_posObj.z + offset.y);
		diff = true;
	}
	brick = GetNumBrickWagon(brick);

	const int32 angle0 = LBAAngles::ANGLE_0;
	const int32 angle90 = LBAAngles::ANGLE_90;
	const int32 angle180 = LBAAngles::ANGLE_180;
	const int32 angle270 = LBAAngles::ANGLE_270;

	switch (brick) {
	case RAIL_UP_SUD:
		frontAxle->type = BoneType::TYPE_TRANSLATE;
		if (ptrobj->_beta == angle0) {
			frontAxle->y = PAS_ESSIEU;
		} else {
			const int32 zess = ptrobj->_posObj.z + offset.y;
			int32 yess = ptrobj->_posObj.y;
			if (diff || brickw != RAIL_UP_SUD) {
				yess -= SIZE_BRICK_Y;
			}
			const int32 y = boundRuleThree(0, SIZE_BRICK_Y, SIZE_BRICK_XZ, zess - (zess & ~511));
			const int32 ey = ptrobj->_posObj.y - (yess & ~255) - 50;
			frontAxle->y = (int16)(-ABS(y - ey));
		}
		break;

	case RAIL_UP_NORD:
		frontAxle->type = BoneType::TYPE_TRANSLATE;
		if (ptrobj->_beta == angle180) {
			frontAxle->y = PAS_ESSIEU;
		} else {
			const int32 zess = ptrobj->_posObj.z + offset.y;
			int32 yess = ptrobj->_posObj.y;
			if (diff || brickw != RAIL_UP_NORD) {
				yess -= SIZE_BRICK_Y;
			}
			const int32 y = boundRuleThree(SIZE_BRICK_Y, 0, SIZE_BRICK_XZ, zess - (zess & ~511));
			const int32 ey = ptrobj->_posObj.y - (yess & ~255) - 50;
			frontAxle->y = (int16)(-ABS(y - ey));
		}
		break;

	case RAIL_UP_EST:
		frontAxle->type = BoneType::TYPE_TRANSLATE;
		if (ptrobj->_beta == angle90) {
			frontAxle->y = PAS_ESSIEU;
		} else {
			const int32 xess = ptrobj->_posObj.x + offset.x;
			int32 yess = ptrobj->_posObj.y;
			if (diff || brickw != RAIL_UP_EST) {
				yess -= SIZE_BRICK_Y;
			}
			const int32 y = boundRuleThree(0, SIZE_BRICK_Y, SIZE_BRICK_XZ, xess - (xess & ~511));
			const int32 ey = ptrobj->_posObj.y - (yess & ~255) - 50;
			frontAxle->y = (int16)(-ABS(y - ey));
		}
		break;

	case RAIL_UP_OUEST:
		frontAxle->type = BoneType::TYPE_TRANSLATE;
		if (ptrobj->_beta == angle270 || brickw == RAIL_NORD_OUEST) {
			frontAxle->y = PAS_ESSIEU;
		} else {
			const int32 xess = ptrobj->_posObj.x + offset.x;
			int32 yess = ptrobj->_posObj.y;
			if (diff || brickw != RAIL_UP_OUEST) {
				yess -= SIZE_BRICK_Y;
			}
			const int32 y = boundRuleThree(SIZE_BRICK_Y, 0, SIZE_BRICK_XZ, xess - (xess & ~511));
			const int32 ey = ptrobj->_posObj.y - (yess & ~255) - 50;
			frontAxle->y = (int16)(-ABS(y - ey));
		}
		break;

	default:
		frontAxle->type = BoneType::TYPE_TRANSLATE;
		if (ptrobj->_wagonHitX < 0) {
			const int32 ey = ptrobj->_posObj.y - (ptrobj->_posObj.y & ~255);
			frontAxle->y = (int16)(-ey);
		} else {
			frontAxle->y = 0;
		}
		break;
	}

	ptrobj->_wagonHitX = frontAxle->y;
}

void Wagon::AdjustEssieuWagonArriere(ActorStruct *ptrobj, int32 brickw) {
	if (ptrobj == nullptr || ptrobj->_body == -1 || !ptrobj->_entityDataPtr) {
		return;
	}

	BodyData &bodyData = ptrobj->_entityDataPtr->getBody(ptrobj->_body);
	if (!bodyData.isAnimated()) {
		return;
	}

	BoneFrame *rearAxle = bodyData.getBoneState(1);
	if (rearAxle == nullptr) {
		return;
	}

	bool diff = false;
	const IVec2 offset = _engine->_renderer->rotate(0, -400, ptrobj->_beta);
	int32 brick = _engine->_grid->worldCodeBrick(ptrobj->_posObj.x + offset.x, ptrobj->_posObj.y, ptrobj->_posObj.z + offset.y);
	if (!brick) {
		brick = _engine->_grid->worldCodeBrick(ptrobj->_posObj.x + offset.x, ptrobj->_posObj.y - SIZE_BRICK_Y - 1, ptrobj->_posObj.z + offset.y);
		diff = true;
	}
	brick = GetNumBrickWagon(brick);

	const int32 angle0 = LBAAngles::ANGLE_0;
	const int32 angle90 = LBAAngles::ANGLE_90;
	const int32 angle180 = LBAAngles::ANGLE_180;
	const int32 angle270 = LBAAngles::ANGLE_270;

	switch (brickw) {
	case RAIL_UP_SUD:
		switch (brick) {
		case RAIL_UP_SUD:
			rearAxle->type = BoneType::TYPE_TRANSLATE;
			if (ptrobj->_beta == angle180) {
				rearAxle->y = PAS_ESSIEU;
			} else {
				const int32 zess = ptrobj->_posObj.z + offset.y;
				int32 yess = ptrobj->_posObj.y;
				if (diff) {
					yess -= SIZE_BRICK_Y;
				}
				const int32 y = boundRuleThree(0, SIZE_BRICK_Y, SIZE_BRICK_XZ, zess - (zess & ~511));
				const int32 ey = ptrobj->_posObj.y - (yess & ~255) - 50;
				rearAxle->y = (int16)(-ABS(y - ey));
			}
			break;
		default:
			rearAxle->type = BoneType::TYPE_TRANSLATE;
			if (ptrobj->_beta == angle0) {
				rearAxle->y = (int16)(-(ptrobj->_posObj.y - (ptrobj->_posObj.y & ~255)));
			} else {
				rearAxle->y = PAS_ESSIEU;
			}
			break;
		}
		break;

	case RAIL_UP_NORD:
		switch (brick) {
		case RAIL_UP_NORD:
			rearAxle->type = BoneType::TYPE_TRANSLATE;
			if (ptrobj->_beta == angle0) {
				rearAxle->y = PAS_ESSIEU;
			} else {
				const int32 zess = ptrobj->_posObj.z + offset.y;
				int32 yess = ptrobj->_posObj.y;
				if (diff) {
					yess -= SIZE_BRICK_Y;
				}
				const int32 y = boundRuleThree(SIZE_BRICK_Y, 0, SIZE_BRICK_XZ, zess - (zess & ~511));
				const int32 ey = ptrobj->_posObj.y - (yess & ~255) - 50;
				rearAxle->y = (int16)(-ABS(y - ey));
			}
			break;
		default:
			rearAxle->type = BoneType::TYPE_TRANSLATE;
			if (ptrobj->_beta == angle180) {
				rearAxle->y = (int16)(-(ptrobj->_posObj.y - (ptrobj->_posObj.y & ~255)));
			} else {
				rearAxle->y = PAS_ESSIEU;
			}
			break;
		}
		break;

	case RAIL_UP_EST:
		switch (brick) {
		case RAIL_UP_EST:
			rearAxle->type = BoneType::TYPE_TRANSLATE;
			if (ptrobj->_beta == angle270) {
				rearAxle->y = PAS_ESSIEU;
			} else {
				const int32 xess = ptrobj->_posObj.x + offset.x;
				int32 yess = ptrobj->_posObj.y;
				if (diff) {
					yess -= SIZE_BRICK_Y;
				}
				const int32 y = boundRuleThree(0, SIZE_BRICK_Y, SIZE_BRICK_XZ, xess - (xess & ~511));
				const int32 ey = ptrobj->_posObj.y - (yess & ~255) - 50;
				rearAxle->y = (int16)(-ABS(y - ey));
			}
			break;
		default:
			rearAxle->type = BoneType::TYPE_TRANSLATE;
			if (ptrobj->_beta == angle90) {
				rearAxle->y = (int16)(-(ptrobj->_posObj.y - (ptrobj->_posObj.y & ~255)));
			} else {
				rearAxle->y = PAS_ESSIEU;
			}
			break;
		}
		break;

	case RAIL_UP_OUEST:
		switch (brick) {
		case RAIL_UP_OUEST:
			rearAxle->type = BoneType::TYPE_TRANSLATE;
			if (ptrobj->_beta == angle90) {
				rearAxle->y = PAS_ESSIEU;
			} else {
				const int32 xess = ptrobj->_posObj.x + offset.x;
				int32 yess = ptrobj->_posObj.y;
				if (diff) {
					yess -= SIZE_BRICK_Y;
				}
				const int32 y = boundRuleThree(SIZE_BRICK_Y, 0, SIZE_BRICK_XZ, xess - (xess & ~511));
				const int32 ey = ptrobj->_posObj.y - (yess & ~255) - 50;
				rearAxle->y = (int16)(-ABS(y - ey));
			}
			break;
		default:
			rearAxle->type = BoneType::TYPE_TRANSLATE;
			if (ptrobj->_beta == angle270) {
				rearAxle->y = (int16)(-(ptrobj->_posObj.y - (ptrobj->_posObj.y & ~255)));
			} else {
				rearAxle->y = PAS_ESSIEU;
			}
			break;
		}
		break;

	default:
		switch (brick) {
		case RAIL_UP_SUD:
			if (ptrobj->_wagonHitZ < 0) {
				rearAxle->type = BoneType::TYPE_TRANSLATE;
				const int32 zess = ptrobj->_posObj.z + offset.y;
				int32 yess = ptrobj->_posObj.y - SIZE_BRICK_Y;
				const int32 y = boundRuleThree(0, SIZE_BRICK_Y, SIZE_BRICK_XZ, zess - (zess & ~511));
				const int32 ey = ptrobj->_posObj.y - (yess & ~255) - 50;
				rearAxle->y = (int16)(-ABS(y - ey));
			}
			break;

		case RAIL_UP_NORD:
			if (ptrobj->_wagonHitZ < 0) {
				rearAxle->type = BoneType::TYPE_TRANSLATE;
				const int32 zess = ptrobj->_posObj.z + offset.y;
				int32 yess = ptrobj->_posObj.y - SIZE_BRICK_Y;
				const int32 y = boundRuleThree(SIZE_BRICK_Y, 0, SIZE_BRICK_XZ, zess - (zess & ~511));
				const int32 ey = ptrobj->_posObj.y - (yess & ~255) - 50;
				rearAxle->y = (int16)(-ABS(y - ey));
			}
			break;

		case RAIL_UP_EST:
			if (ptrobj->_wagonHitZ < 0) {
				rearAxle->type = BoneType::TYPE_TRANSLATE;
				const int32 xess = ptrobj->_posObj.x + offset.x;
				int32 yess = ptrobj->_posObj.y - SIZE_BRICK_Y;
				const int32 y = boundRuleThree(0, SIZE_BRICK_Y, SIZE_BRICK_XZ, xess - (xess & ~511));
				const int32 ey = ptrobj->_posObj.y - (yess & ~255) - 50;
				rearAxle->y = (int16)(-ABS(y - ey));
			}
			break;

		case RAIL_UP_OUEST:
			if (ptrobj->_wagonHitZ < 0) {
				rearAxle->type = BoneType::TYPE_TRANSLATE;
				const int32 xess = ptrobj->_posObj.x + offset.x;
				int32 yess = ptrobj->_posObj.y - SIZE_BRICK_Y;
				const int32 y = boundRuleThree(SIZE_BRICK_Y, 0, SIZE_BRICK_XZ, xess - (xess & ~511));
				const int32 ey = ptrobj->_posObj.y - (yess & ~255) - 50;
				rearAxle->y = (int16)(-ABS(y - ey));
			}
			break;

		default:
			rearAxle->type = BoneType::TYPE_TRANSLATE;
			rearAxle->y = 0;
			break;
		}
		break;
	}

	ptrobj->_wagonHitZ = rearAxle->y;
}

} // namespace TwinE
