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

#include "twine/scene/flow.h"
#include "twine/renderer/renderer.h"
#include "twine/resources/hqr.h"
#include "twine/resources/resources.h"
#include "twine/scene/actor.h"
#include "twine/scene/extra.h"
#include "twine/scene/scene.h"
#include "twine/twine.h"

namespace TwinE {

Flow::Flow(TwinEEngine *engine) : _engine(engine) {
	reset();
}

Flow::~Flow() {
	free(_templates);
	_templates = nullptr;
}

void Flow::reset() {
	for (int i = 0; i < MAX_FLOWS; ++i) {
		_flows[i].nbDot = 0;
	}
}

bool Flow::init() {
	free(_templates);
	_templates = nullptr;

	const int32 size = HQR::getAllocEntry((uint8 **)&_templates, Resources::HQR_RESS_FILE, RESSHQR_FLOW);
	if (size <= 0 || _templates == nullptr) {
		warning("Flow::init(): failed to load flow templates");
		return false;
	}
	reset();
	return true;
}

const FlowTemplate *Flow::getTemplate(int32 index) const {
	if (_templates == nullptr || index < 0) {
		return nullptr;
	}
	return &_templates[index];
}

ParticleFlow *Flow::getFreeSlot() {
	for (int i = 0; i < MAX_FLOWS; ++i) {
		if (_flows[i].nbDot == 0) {
			return &_flows[i];
		}
	}
	return nullptr;
}

bool Flow::createParticleFlow(int32 flag, int32 owner, int32 numPoint,
                              int32 orgX, int32 orgY, int32 orgZ, int32 beta, int32 numFlow) {
	ParticleFlow *flow = getFreeSlot();
	if (flow == nullptr) {
		return false;
	}

	const FlowTemplate *tmpl = getTemplate(numFlow);
	if (tmpl == nullptr) {
		return false;
	}

	int32 nbDot = tmpl->nbDot;
	if (nbDot > MAX_FLOW_DOTS) {
		nbDot = MAX_FLOW_DOTS;
	}
	if (nbDot <= 0) {
		return false;
	}

	flow->flag = flag;
	flow->xMin = tmpl->xMin;
	flow->yMin = tmpl->yMin;
	flow->zMin = tmpl->zMin;
	flow->xMax = tmpl->xMax;
	flow->yMax = tmpl->yMax;
	flow->zMax = tmpl->zMax;
	flow->flowTimerStart = _engine->timerRef;
	flow->nbDot = nbDot;

	if (!(flag & FLOW_WAIT_COOR)) {
		flow->orgX = orgX;
		flow->orgY = orgY;
		flow->orgZ = orgZ;
	} else {
		flow->owner = owner;
		flow->numPoint = numPoint;
	}

	int32 demiSpeed = tmpl->speed >> 1;
	if (!demiSpeed) {
		demiSpeed = 1;
	}
	int32 demiWeight = tmpl->weight >> 1;
	if (!demiWeight) {
		demiWeight = 1;
	}
	const int32 demiOuvertureAlpha = tmpl->ouvertureAlpha >> 1;
	const int32 demiOuvertureBeta = tmpl->ouvertureBeta >> 1;
	beta = (beta + tmpl->beta) & (LBAAngles::ANGLE_360 - 1);

	const int32 bank = tmpl->bank * 16;

	for (int32 n = 0; n < nbDot; ++n) {
		FlowDot &dot = flow->dots[n];
		if (!(flag & FLOW_WAIT_COOR)) {
			dot.x = orgX;
			dot.y = orgY;
			dot.z = orgZ;
		}

		const int32 vitesse = _engine->getRandomNumber(demiSpeed) + tmpl->speed;
		dot.weight = _engine->getRandomNumber(demiWeight) + tmpl->weight;

		const int32 alpha = tmpl->alpha + _engine->getRandomNumber(tmpl->ouvertureAlpha) - demiOuvertureAlpha;
		const IVec2 &rot1 = _engine->_renderer->rotate(vitesse, 0, alpha);
		dot.vy = -rot1.y;

		const int32 betaVal = beta + _engine->getRandomNumber(tmpl->ouvertureBeta) - demiOuvertureBeta;
		const IVec2 &rot2 = _engine->_renderer->rotate(0, rot1.x, betaVal);
		dot.vx = rot2.x;
		dot.vz = rot2.y;

		dot.delay = tmpl->delay ? _engine->getRandomNumber(tmpl->delay) : 0;
		dot.color = bank + _engine->getRandomNumber(tmpl->range) + tmpl->coul;
		dot.mode = kFlowDotWait;
	}

	return true;
}

bool Flow::createExtraParticleFlow(int32 type, int32 owner, int32 num, int32 num2,
                                   int32 orgX, int32 orgY, int32 orgZ, int32 beta, int32 numFlow,
                                   int32 hitForce, int32 scale, int32 transparent, int32 tempo) {
	const FlowTemplate *tmpl = getTemplate(numFlow);
	if (tmpl == nullptr) {
		return false;
	}

	int32 nbDot = tmpl->nbDot;
	if (nbDot > MAX_FLOW_EXTRA_DOTS) {
		nbDot = MAX_FLOW_EXTRA_DOTS;
	}

	int32 demiSpeed = tmpl->speed >> 1;
	if (!demiSpeed) {
		demiSpeed = 1;
	}
	int32 demiWeight = tmpl->weight >> 1;
	if (!demiWeight) {
		demiWeight = 1;
	}
	const int32 demiOuvertureAlpha = tmpl->ouvertureAlpha >> 1;
	const int32 demiOuvertureBeta = tmpl->ouvertureBeta >> 1;
	beta = (beta + tmpl->beta) & (LBAAngles::ANGLE_360 - 1);

	for (int32 n = 0; n < nbDot; ++n) {
		const int32 vitesse = _engine->getRandomNumber(demiSpeed) + tmpl->speed;
		const int32 poids = _engine->getRandomNumber(demiWeight) + tmpl->weight;
		const int32 delay = tmpl->delay ? _engine->getRandomNumber(tmpl->delay) : 0;
		const int32 alpha = tmpl->alpha + _engine->getRandomNumber(tmpl->ouvertureAlpha) - demiOuvertureAlpha;
		const int32 betaVal = beta + _engine->getRandomNumber(tmpl->ouvertureBeta) - demiOuvertureBeta;

		int32 extraIdx = -1;
		switch (type) {
		case FLOW_EXTRA_OBJ:
			extraIdx = _engine->_extra->throwExtraObj(owner, orgX, orgY, orgZ, (int16)num,
			                                         alpha, betaVal, vitesse, -1, poids, hitForce);
			break;
		case FLOW_EXTRA_SPRITE:
			if (num == num2) {
				extraIdx = _engine->_extra->throwExtra(owner, orgX, orgY, orgZ, (int16)num,
				                                       alpha, betaVal, vitesse, (uint8)poids, (uint8)hitForce);
				if (extraIdx != -1) {
					_engine->_extra->_extraList[extraIdx].extraAlpha = scale;
				}
			} else {
				extraIdx = _engine->_extra->initExtraAnimSprite(owner, orgX, orgY, orgZ, (int16)num, (int16)num2,
				                                                tempo, scale, transparent, hitForce);
				if (extraIdx != -1) {
					ExtraListStruct &extra = _engine->_extra->_extraList[extraIdx];
					extra.type |= ExtraType::FLY;
					_engine->_extra->initFly(&extra, alpha, betaVal, vitesse, poids);
				}
			}
			break;
		case FLOW_EXTRA_POF: {
			const int32 speedRot = (num2 == -1) ? vitesse : num2;
			extraIdx = _engine->_extra->initExtraPof(orgX, orgY, orgZ, (int16)num,
			                                         alpha, betaVal, vitesse, poids,
			                                         scale, transparent, tempo, speedRot, 0);
			break;
		}
		default:
			break;
		}

		if (extraIdx != -1 && delay) {
			ExtraListStruct &extra = _engine->_extra->_extraList[extraIdx];
			extra.type |= ExtraType::WAIT_SOME_TIME;
			extra.payload.lifeTime = (int16)(_engine->timerRef + delay);
		}
	}

	return true;
}

bool Flow::animParticleFlow(ParticleFlow *flow) {
	if (flow == nullptr || flow->nbDot == 0) {
		return false;
	}

	const int32 elapsed = _engine->timerRef - (int32)flow->flowTimerStart;
	int32 orgX = flow->orgX;
	int32 orgY = flow->orgY;
	int32 orgZ = flow->orgZ;

	if (flow->flag & FLOW_WAIT_COOR) {
		ActorStruct *actor = _engine->_scene->getActor(flow->owner);
		if (actor == nullptr || actor->_lifePoint <= 0) {
			flow->nbDot = 0;
			return false;
		}
		orgX = actor->_posObj.x;
		orgY = actor->_posObj.y;
		orgZ = actor->_posObj.z;
		flow->orgX = orgX;
		flow->orgY = orgY;
		flow->orgZ = orgZ;
	}

	bool active = false;
	for (int32 n = 0; n < flow->nbDot; ++n) {
		FlowDot &dot = flow->dots[n];
		if (dot.mode == kFlowDotDead) {
			continue;
		}

		if (dot.mode == kFlowDotWait) {
			if ((int32)dot.delay <= elapsed) {
				dot.mode = kFlowDotDisplay;
				if (flow->flag & FLOW_WAIT_COOR) {
					dot.x = orgX;
					dot.y = orgY;
					dot.z = orgZ;
				}
			} else {
				active = true;
				continue;
			}
		}

		if (dot.mode == kFlowDotDisplay) {
			dot.x += dot.vx;
			dot.y += dot.vy;
			dot.z += dot.vz;
			dot.vy -= dot.weight;

			if (dot.y < orgY + flow->yMin || dot.x < orgX + flow->xMin || dot.z < orgZ + flow->zMin ||
			    dot.y > orgY + flow->yMax || dot.x > orgX + flow->xMax || dot.z > orgZ + flow->zMax) {
				dot.mode = kFlowDotDead;
			} else {
				active = true;
			}
		}
	}

	if (!active) {
		flow->nbDot = 0;
	}
	return active;
}

bool Flow::renderParticleFlow(ParticleFlow *flow) {
	if (flow == nullptr || flow->nbDot == 0) {
		return false;
	}

	bool drawn = false;
	for (int32 n = 0; n < flow->nbDot; ++n) {
		const FlowDot &dot = flow->dots[n];
		if (dot.mode != kFlowDotDisplay) {
			continue;
		}

		const IVec3 &world = _engine->_renderer->longWorldRot(dot.x, dot.y, dot.z);
		IVec3 proj;
		if (!_engine->_renderer->longProjectPoint(world, proj)) {
			continue;
		}

		const int32 size = 2;
		const Common::Rect r(proj.x - size, proj.y - size, proj.x + size, proj.y + size);
		_engine->_workVideoBuffer.fillRect(r, (uint8)(dot.color & 0xFF));
		drawn = true;
	}
	return drawn;
}

void Flow::animAll() {
	for (int i = 0; i < MAX_FLOWS; ++i) {
		if (_flows[i].nbDot != 0) {
			animParticleFlow(&_flows[i]);
		}
	}
}

void Flow::renderAll() {
	for (int i = 0; i < MAX_FLOWS; ++i) {
		if (_flows[i].nbDot != 0) {
			renderParticleFlow(&_flows[i]);
		}
	}
}

} // namespace TwinE
