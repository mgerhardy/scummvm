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

#ifndef TWINE_SCENE_FLOW_H
#define TWINE_SCENE_FLOW_H

#include "common/scummsys.h"

namespace TwinE {

class TwinEEngine;

#define FLOW_WAIT_COOR 1
#define FLOW_EXTRA_SPRITE 2
#define FLOW_EXTRA_OBJ 4
#define FLOW_EXTRA_POF 8

#define MAX_FLOWS 10
#define MAX_FLOW_DOTS 100
#define MAX_FLOW_EXTRA_DOTS 10

enum FlowDotMode : int32 {
	kFlowDotDead = 0,
	kFlowDotDisplay = 1,
	kFlowDotWait = 2
};

struct FlowTemplate {
	int16 alpha = 0;
	int16 beta = 0;
	int16 ouvertureAlpha = 0;
	int16 ouvertureBeta = 0;
	int16 xMin = 0;
	int16 yMin = 0;
	int16 zMin = 0;
	int16 xMax = 0;
	int16 yMax = 0;
	int16 zMax = 0;
	int16 delay = 0;
	int16 speed = 0;
	int16 weight = 0;
	int16 nbDot = 0;
	int16 bank = 0;
	uint8 coul = 0;
	uint8 range = 0;
	uint32 flags = 0;
};

struct FlowDot {
	int32 x = 0;
	int32 y = 0;
	int32 z = 0;
	int32 vx = 0;
	int32 vy = 0;
	int32 vz = 0;
	int32 delay = 0;
	int32 weight = 0;
	int32 color = 0;
	FlowDotMode mode = kFlowDotDead;
};

struct ParticleFlow {
	int32 flag = 0;
	int32 owner = 0;
	int32 numPoint = 0;
	int32 nbDot = 0;
	int32 orgX = 0;
	int32 orgY = 0;
	int32 orgZ = 0;
	int32 xMin = 0;
	int32 yMin = 0;
	int32 zMin = 0;
	int32 xMax = 0;
	int32 yMax = 0;
	int32 zMax = 0;
	uint32 flowTimerStart = 0;
	FlowDot dots[MAX_FLOW_DOTS];
};

class Flow {
private:
	TwinEEngine *_engine = nullptr;
	FlowTemplate *_templates = nullptr;
	ParticleFlow _flows[MAX_FLOWS];

	ParticleFlow *getFreeSlot();
	const FlowTemplate *getTemplate(int32 index) const;
	bool animParticleFlow(ParticleFlow *flow);
	bool renderParticleFlow(ParticleFlow *flow);

public:
	Flow(TwinEEngine *engine);
	~Flow();

	bool init();
	void reset();

	bool createParticleFlow(int32 flag, int32 owner, int32 numPoint,
	                      int32 orgX, int32 orgY, int32 orgZ, int32 beta, int32 numFlow);
	bool createExtraParticleFlow(int32 type, int32 owner, int32 num, int32 num2,
	                             int32 orgX, int32 orgY, int32 orgZ, int32 beta, int32 numFlow,
	                             int32 hitForce, int32 scale, int32 transparent, int32 tempo);

	void animAll();
	void renderAll();
};

} // namespace TwinE

#endif
