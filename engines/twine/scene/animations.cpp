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

#include "twine/scene/animations.h"
#include "common/util.h"
#include "twine/audio/sound.h"
#include "twine/debugger/debug_state.h"
#include "twine/parser/anim.h"
#include "twine/parser/entity.h"
#include "twine/renderer/renderer.h"
#include "twine/resources/resources.h"
#include "twine/scene/buggy.h"
#include "twine/scene/collision.h"
#include "twine/scene/dart.h"
#include "twine/scene/flow.h"
#include "twine/scene/extra.h"
#include "twine/scene/gamestate.h"
#include "twine/scene/grid.h"
#include "twine/scene/movements.h"
#include "twine/scene/scene.h"
#include "twine/scene/actor.h"
#include "twine/scene/wagon.h"
#include "twine/shared.h"

namespace TwinE {

static const int32 magicLevelStrengthOfHit[] = {
	MagicballStrengthType::kNoBallStrength,
	MagicballStrengthType::kYellowBallStrength,
	MagicballStrengthType::kGreenBallStrength,
	MagicballStrengthType::kRedBallStrength,
	MagicballStrengthType::kFireBallStrength,
	0};

static bool isAnimRotateBone(uint16 type, bool lba2) {
	if (lba2) {
		return (type & (uint16)BoneType::TYPE_TRANSLATE) == 0;
	}
	return type == (uint16)BoneType::TYPE_ROTATE;
}

static bool isAnimTranslateBone(uint16 type, bool lba2) {
	if (lba2) {
		return (type & (uint16)BoneType::TYPE_TRANSLATE) != 0;
	}
	return type == (uint16)BoneType::TYPE_TRANSLATE;
}

static int32 nextAnimFrameIndex(const AnimData &animData, int32 frameIdx) {
	const int32 numFrames = (int32)animData.getNbFramesAnim();
	int32 next = frameIdx + 1;
	if (next >= numFrames) {
		next = animData.getLoopFrame();
	}
	return next;
}

static int16 patchInterAngleLBA2(int16 last, int16 next, uint32 interpolator) {
	int32 diff = (next - last) & 0xFFF;
	if (diff != 0) {
		diff = SignExt12((int16)diff);
		diff = (diff * (int32)interpolator) >> 16;
		last = (int16)((last + diff) & 0xFFF);
	}
	return last;
}

static int16 patchInterStepLBA2(int16 last, int16 next, uint32 interpolator) {
	int32 diff = (int32)next - (int32)last;
	if (diff != 0) {
		diff = (diff * (int32)interpolator) >> 16;
		last = (int16)(last + diff);
	}
	return last;
}

static bool isStoredKeyframe(const KeyFrame *keyframe, const KeyFrame *animKeyframeBuf, int32 bufSize) {
	if (keyframe == nullptr) {
		return false;
	}
	for (int32 i = 0; i < bufSize; ++i) {
		if (keyframe == &animKeyframeBuf[i]) {
			return true;
		}
	}
	return false;
}

static void resetAnimTimerState(AnimTimerDataStruct *animTimerDataPtr) {
	animTimerDataPtr->ptr = nullptr;
	animTimerDataPtr->time = 0;
	animTimerDataPtr->lastAnimStepX = 0;
	animTimerDataPtr->lastAnimStepY = 0;
	animTimerDataPtr->lastAnimStepZ = 0;
	animTimerDataPtr->lastAnimStepAlpha = 0;
	animTimerDataPtr->lastAnimStepBeta = 0;
	animTimerDataPtr->lastAnimStepGamma = 0;
	animTimerDataPtr->interpolator = 0;
	animTimerDataPtr->skipBoneInterp = false;
}

Animations::Animations(TwinEEngine *engine) : _engine(engine) {
}

int32 Animations::searchAnim(AnimationTypes animIdx, int32 actorIdx) {
	ActorStruct *actor = _engine->_scene->getActor(actorIdx);
	const int32 bodyAnimIndex = actor->_entityDataPtr->getAnimIndex(animIdx);
	if (bodyAnimIndex != -1) {
		_currentActorAnimExtraPtr = animIdx;
	}
	return bodyAnimIndex;
}

int16 Animations::patchInterAngle(int32 deltaTime, int32 keyFrameLength, int16 newAngle1, int16 lastAngle1) const {
	const int16 lastAngle = ClampAngle(lastAngle1);
	const int16 nextAngle = ClampAngle(newAngle1);

	int16 angleDiff = nextAngle - lastAngle;

	int16 computedAngle;
	if (angleDiff) {
		if (angleDiff < -LBAAngles::ANGLE_180) {
			angleDiff += LBAAngles::ANGLE_360;
		} else if (angleDiff > LBAAngles::ANGLE_180) {
			angleDiff -= LBAAngles::ANGLE_360;
		}

		computedAngle = lastAngle + (angleDiff * deltaTime) / keyFrameLength;
	} else {
		computedAngle = lastAngle;
	}

	return ClampAngle(computedAngle);
}

int16 Animations::patchInterStep(int32 deltaTime, int32 keyFrameLength, int16 newPos, int16 lastPos) const {
	int16 distance = newPos - lastPos;

	int16 computedPos;
	if (distance) {
		computedPos = lastPos + (distance * deltaTime) / keyFrameLength;
	} else {
		computedPos = lastPos;
	}

	return computedPos;
}

bool Animations::doSetInterAnimObjet(int32 framedest, const AnimData &animData, BodyData &pBody, AnimTimerDataStruct *ptranimdest, bool global) {
	if (!pBody.isAnimated()) {
		return false;
	}

	const int16 numBones = pBody.getNumBones();
	int32 numOfBonesInAnim = animData.getNumBoneframes();
	if (numOfBonesInAnim > numBones) {
		numOfBonesInAnim = numBones;
	}

	if (_engine->isLBA2()) {
		if (ptranimdest->skipBoneInterp) {
			ptranimdest->skipBoneInterp = false;
			return false;
		}

		if (numOfBonesInAnim <= 1) {
			return false;
		}

		const uint32 interpolator = ptranimdest->interpolator;
		const bool fromStoredPose = isStoredKeyframe(ptranimdest->ptr, _animKeyframeBuf, ARRAYSIZE(_animKeyframeBuf));
		if (interpolator == 0 && !fromStoredPose) {
			return false;
		}

		const KeyFrame *animKeyFrame = animData.getKeyframe(framedest);
		if (animKeyFrame == nullptr) {
			return false;
		}

		const KeyFrame *lastKeyFrame;
		const KeyFrame *nextKeyFrame;
		if (fromStoredPose) {
			lastKeyFrame = ptranimdest->ptr;
			nextKeyFrame = animKeyFrame;
		} else {
			lastKeyFrame = animKeyFrame;
			const int32 nextFrameIdx = nextAnimFrameIndex(animData, framedest);
			nextKeyFrame = animData.getKeyframe(nextFrameIdx);
		}
		if (lastKeyFrame == nullptr || nextKeyFrame == nullptr) {
			return false;
		}

		const int32 numGroups = MIN<int32>(numOfBonesInAnim, (int32)MIN(lastKeyFrame->boneframes.size(), nextKeyFrame->boneframes.size()));

		for (int32 boneIdx = 1; boneIdx < numGroups; ++boneIdx) {
			BoneFrame *boneState = pBody.getBoneState(boneIdx);
			const BoneFrame &lastBone = lastKeyFrame->boneframes[boneIdx];
			const BoneFrame &nextBone = nextKeyFrame->boneframes[boneIdx];

			boneState->type = nextBone.type;
			const uint16 boneType = (uint16)nextBone.type;
			if (boneType == (uint16)BoneType::TYPE_ROTATE) {
				boneState->x = patchInterAngleLBA2(lastBone.x, nextBone.x, interpolator);
				boneState->y = patchInterAngleLBA2(lastBone.y, nextBone.y, interpolator);
				boneState->z = patchInterAngleLBA2(lastBone.z, nextBone.z, interpolator);
			} else {
				boneState->x = patchInterStepLBA2(lastBone.x, nextBone.x, interpolator);
				boneState->y = patchInterStepLBA2(lastBone.y, nextBone.y, interpolator);
				boneState->z = patchInterStepLBA2(lastBone.z, nextBone.z, interpolator);
			}
		}

		if (fromStoredPose && interpolator >= 0x10000) {
			ptranimdest->ptr = animKeyFrame;
		}

		(void)global;
		return false;
	}

	const KeyFrame *keyFrame = animData.getKeyframe(framedest);
	const int32 timeDest = keyFrame->length;

	const KeyFrame *lastKeyFramePtr = ptranimdest->ptr;
	int32 remainingFrameTime = ptranimdest->time;
	if (lastKeyFramePtr == nullptr) {
		lastKeyFramePtr = keyFrame;
		remainingFrameTime = timeDest;
	}
	const int32 time = _engine->timerRef - remainingFrameTime;
	if (time >= timeDest) {
		ptranimdest->ptr = keyFrame;

		if (global) {
			ptranimdest->time = _engine->timerRef;

			_animStep.x = keyFrame->x;
			_animStep.y = keyFrame->y;
			_animStep.z = keyFrame->z;
			_animMasterRot = keyFrame->animMasterRot;
			_animStepAlpha = ToAngle(keyFrame->animStepAlpha);
			_animStepBeta = ToAngle(keyFrame->animStepBeta);
			_animStepGamma = ToAngle(keyFrame->animStepGamma);
		}

		copyKeyFrameToState(keyFrame, pBody, numOfBonesInAnim);

		return true;
	}

	if (global) {
		_animStep.x = keyFrame->x;
		_animStep.y = keyFrame->y;
		_animStep.z = keyFrame->z;
		_animMasterRot = keyFrame->animMasterRot;
		_animStepAlpha = (keyFrame->animStepAlpha * time) / timeDest;
		_animStepBeta = (keyFrame->animStepBeta * time) / timeDest;
		_animStepGamma = (keyFrame->animStepGamma * time) / timeDest;
	}
	if (numOfBonesInAnim <= 1) {
		return false;
	}

	int16 boneIdx = 1;
	int16 tmpNumOfPoints = MIN<int16>(lastKeyFramePtr->boneframes.size() - 1, numOfBonesInAnim - 1);
	do {
		BoneFrame *boneState = pBody.getBoneState(boneIdx);
		const BoneFrame &boneFrame = keyFrame->boneframes[boneIdx];
		const BoneFrame &lastBoneFrame = lastKeyFramePtr->boneframes[boneIdx];

		boneState->type = boneFrame.type;
		const uint16 boneType = (uint16)boneFrame.type;
		if (isAnimRotateBone(boneType, _engine->isLBA2())) {
			boneState->x = patchInterAngle(time, timeDest, boneFrame.x, lastBoneFrame.x);
			boneState->y = patchInterAngle(time, timeDest, boneFrame.y, lastBoneFrame.y);
			boneState->z = patchInterAngle(time, timeDest, boneFrame.z, lastBoneFrame.z);
		} else if (isAnimTranslateBone(boneType, _engine->isLBA2())) {
			boneState->x = patchInterStep(time, timeDest, boneFrame.x, lastBoneFrame.x);
			boneState->y = patchInterStep(time, timeDest, boneFrame.y, lastBoneFrame.y);
			boneState->z = patchInterStep(time, timeDest, boneFrame.z, lastBoneFrame.z);
		} else if (boneType == (uint16)BoneType::TYPE_ZOOM) {
			boneState->x = patchInterStep(time, timeDest, boneFrame.x, lastBoneFrame.x);
			boneState->y = patchInterStep(time, timeDest, boneFrame.y, lastBoneFrame.y);
			boneState->z = patchInterStep(time, timeDest, boneFrame.z, lastBoneFrame.z);
		} else {
			error("Unsupported animation rotation mode %d", boneFrame.type);
		}

		++boneIdx;
	} while (--tmpNumOfPoints);

	return false;
}

void Animations::setAnimObjet(int32 keyframeIdx, const AnimData &animData, BodyData &bodyData, AnimTimerDataStruct *animTimerDataPtr) {
	if (!bodyData.isAnimated()) {
		return;
	}

	const int32 numOfKeyframeInAnim = animData.getKeyframes().size();
	if (keyframeIdx < 0 || keyframeIdx >= numOfKeyframeInAnim) {
		return;
	}

	const KeyFrame *keyFrame = animData.getKeyframe(keyframeIdx);

	_animStep.x = keyFrame->x;
	_animStep.y = keyFrame->y;
	_animStep.z = keyFrame->z;

	_animMasterRot = keyFrame->animMasterRot;
	_animStepBeta = ToAngle(keyFrame->animStepBeta);

	resetAnimTimerState(animTimerDataPtr);
	animTimerDataPtr->ptr = animData.getKeyframe(keyframeIdx);
	animTimerDataPtr->time = _engine->timerRef;
	animTimerDataPtr->lastNbGroups = animData.getNumBoneframes();

	const int16 numBones = bodyData.getNumBones();

	int16 numOfBonesInAnim = animData.getNumBoneframes();
	if (numOfBonesInAnim > numBones) {
		numOfBonesInAnim = numBones;
	}

	copyKeyFrameToState(keyFrame, bodyData, numOfBonesInAnim);
}

void Animations::setAnimFrame(ActorStruct *actor, uint32 frame) {
	if (actor->_body == -1 || actor->_anim == -1 || actor->_entityDataPtr == nullptr) {
		return;
	}

	const AnimData &animData = _engine->_resources->_animData[actor->_anim];
	if (frame >= (uint32)animData.getNbFramesAnim()) {
		return;
	}

	BodyData &bodyData = actor->_entityDataPtr->getBody(actor->_body);
	if (!bodyData.isAnimated()) {
		return;
	}

	AnimTimerDataStruct *animTimerDataPtr = &bodyData._animTimerData;
	resetAnimTimerState(animTimerDataPtr);
	animTimerDataPtr->time = _engine->timerRef;
	animTimerDataPtr->skipBoneInterp = true;

	const KeyFrame *keyFrame = animData.getKeyframe((int32)frame);
	if (keyFrame == nullptr) {
		return;
	}

	animTimerDataPtr->ptr = keyFrame;
	actor->_frame = (int16)frame;

	_animStep.x = keyFrame->x;
	_animStep.y = keyFrame->y;
	_animStep.z = keyFrame->z;
	_animMasterRot = keyFrame->animMasterRot;
	_animStepBeta = ToAngle(keyFrame->animStepBeta);

	int16 numOfBonesInAnim = animData.getNumBoneframes();
	if (numOfBonesInAnim > bodyData.getNumBones()) {
		numOfBonesInAnim = bodyData.getNumBones();
	}
	copyKeyFrameToState(keyFrame, bodyData, numOfBonesInAnim);

	actor->_workFlags.bAnimNewFrame = 1;
	actor->_workFlags.bAnimEnded = 0;
}

void Animations::stockInterAnim(const BodyData &bodyData, AnimTimerDataStruct *animTimerDataPtr, const AnimData *newAnimData) {
	if (!bodyData.isAnimated()) {
		return;
	}

	if (_animKeyframeBufIdx >= ARRAYSIZE(_animKeyframeBuf)) {
		_animKeyframeBufIdx = 0;
	}
	animTimerDataPtr->time = _engine->timerRef;
	KeyFrame *keyframe = &_animKeyframeBuf[_animKeyframeBufIdx++];
	animTimerDataPtr->ptr = keyframe;
	copyStateToKeyFrame(keyframe, bodyData);

	const int32 oldNbGroups = (int32)keyframe->boneframes.size();
	animTimerDataPtr->lastNbGroups = oldNbGroups;

	if (newAnimData != nullptr) {
		const int32 newNbGroups = newAnimData->getNumBoneframes();
		const KeyFrame *frame0 = newAnimData->getKeyframe(0);
		if (frame0 != nullptr && newNbGroups > oldNbGroups) {
			keyframe->boneframes.reserve(newNbGroups);
			for (int32 i = oldNbGroups; i < newNbGroups && i < (int32)frame0->boneframes.size(); ++i) {
				keyframe->boneframes.push_back(frame0->boneframes[i]);
			}
		}
	}
}

void Animations::setInterAnimObjetLBA2(int32 keyframeIdx, const AnimData &animData, BodyData &bodyData, AnimTimerDataStruct *animTimerDataPtr) {
	// INTFRAME.CPP: only interpolate while INTERDEP reported motion (FLAG_CHANGE + interpolator)
	if (animTimerDataPtr->skipBoneInterp) {
		animTimerDataPtr->skipBoneInterp = false;
		return;
	}
	if (animTimerDataPtr->interpolator == 0 && !isStoredKeyframe(animTimerDataPtr->ptr, _animKeyframeBuf, ARRAYSIZE(_animKeyframeBuf))) {
		return;
	}
	(void)doSetInterAnimObjet(keyframeIdx, animData, bodyData, animTimerDataPtr, false);
}

void Animations::copyStateToKeyFrame(KeyFrame *keyframe, const BodyData &bodyData) const {
	const int32 numBones = bodyData.getNumBones();
	keyframe->boneframes.clear();
	keyframe->boneframes.reserve(numBones);
	for (int32 i = 0; i < numBones; ++i) {
		const BoneFrame *boneState = bodyData.getBoneState(i);
		keyframe->boneframes.push_back(*boneState);
	}
}

void Animations::copyKeyFrameToState(const KeyFrame *keyframe, BodyData &bodyData, int32 numBones) const {
	const int32 numAnimBones = MIN<int32>(numBones, (int32)keyframe->boneframes.size());
	// boneframes[0] is the anim master group; body bone 0 uses the actor orientation.
	for (int32 i = 1; i < numAnimBones; ++i) {
		*bodyData.getBoneState(i) = keyframe->boneframes[i];
	}
}

bool Animations::setInterDepObjet(int32 keyframeIdx, const AnimData &animData, AnimTimerDataStruct *animTimerDataPtr, ActorStruct *actor) {
	const KeyFrame *keyFrame = animData.getKeyframe(keyframeIdx);
	if (keyFrame == nullptr) {
		return false;
	}
	const int32 timeDest = keyFrame->length;

	if (_engine->timerRef < animTimerDataPtr->time) {
		animTimerDataPtr->time = _engine->timerRef;
		return false;
	}

	int32 remainingFrameTime = animTimerDataPtr->time;
	if (animTimerDataPtr->ptr == nullptr) {
		remainingFrameTime = _engine->timerRef;
		animTimerDataPtr->time = _engine->timerRef;
	}
	const int32 time = _engine->timerRef - remainingFrameTime;

	_animMasterRot = keyFrame->animMasterRot;

	uint32 interpolator = 0;
	bool keyFramePassed = false;
	if (time >= timeDest) {
		interpolator = 0x10000;
		keyFramePassed = true;
		animTimerDataPtr->ptr = keyFrame;
		animTimerDataPtr->time = _engine->timerRef;
	} else if (timeDest > 0) {
		interpolator = (uint32)(((time << 16) + ((timeDest + 1) >> 1)) / timeDest);
	}

	if (_engine->isLBA2() && actor != nullptr) {
		if (_animMasterRot & 1) {
			const int32 alphaStep = SignExt12(keyFrame->animStepAlpha);
			const int32 betaStep = SignExt12(keyFrame->animStepBeta);
			const int32 gammaStep = SignExt12(keyFrame->animStepGamma);

			const int32 alphaDelta = (int32)(((int64)interpolator * alphaStep) >> 16);
			actor->_alpha = ClampAngle(actor->_alpha + alphaDelta - animTimerDataPtr->lastAnimStepAlpha);
			animTimerDataPtr->lastAnimStepAlpha = alphaDelta;

			const int32 betaDelta = (int32)(((int64)interpolator * betaStep) >> 16);
			actor->_beta = ClampAngle(actor->_beta + betaDelta - animTimerDataPtr->lastAnimStepBeta);
			animTimerDataPtr->lastAnimStepBeta = betaDelta;
			_animStepBeta = betaDelta;

			const int32 gammaDelta = (int32)(((int64)interpolator * gammaStep) >> 16);
			actor->_gamma = ClampAngle(actor->_gamma + gammaDelta - animTimerDataPtr->lastAnimStepGamma);
			animTimerDataPtr->lastAnimStepGamma = gammaDelta;
		}

		const int32 xDelta = (int32)(((int64)interpolator * keyFrame->x) >> 16);
		const int32 yDelta = (int32)(((int64)interpolator * keyFrame->y) >> 16);
		const int32 zDelta = (int32)(((int64)interpolator * keyFrame->z) >> 16);

		const int32 rotX = xDelta - animTimerDataPtr->lastAnimStepX;
		const int32 rotY = yDelta - animTimerDataPtr->lastAnimStepY;
		const int32 rotZ = zDelta - animTimerDataPtr->lastAnimStepZ;

		animTimerDataPtr->lastAnimStepX = xDelta;
		animTimerDataPtr->lastAnimStepY = yDelta;
		animTimerDataPtr->lastAnimStepZ = zDelta;

		_animStep = _engine->_renderer->rotateRootAnimStep(actor->_alpha, actor->_beta, actor->_gamma, rotX, rotY, rotZ);

		animTimerDataPtr->interpolator = interpolator;
		if (keyFramePassed) {
			animTimerDataPtr->skipBoneInterp = true;
			animTimerDataPtr->interpolator = 0;
			animTimerDataPtr->lastAnimStepX = 0;
			animTimerDataPtr->lastAnimStepY = 0;
			animTimerDataPtr->lastAnimStepZ = 0;
			animTimerDataPtr->lastAnimStepAlpha = 0;
			animTimerDataPtr->lastAnimStepBeta = 0;
			animTimerDataPtr->lastAnimStepGamma = 0;
		}

		return keyFramePassed;
	}

	if (time >= timeDest) {
		_animStep.x = keyFrame->x;
		_animStep.y = keyFrame->y;
		_animStep.z = keyFrame->z;
		_animStepAlpha = ToAngle(keyFrame->animStepAlpha);
		_animStepBeta = ToAngle(keyFrame->animStepBeta);
		_animStepGamma = ToAngle(keyFrame->animStepGamma);
		animTimerDataPtr->ptr = animData.getKeyframe(keyframeIdx);
		animTimerDataPtr->time = _engine->timerRef;
		return true;
	}

	_animStep.x = (keyFrame->x * time) / timeDest;
	_animStep.y = (keyFrame->y * time) / timeDest;
	_animStep.z = (keyFrame->z * time) / timeDest;
	_animStepAlpha = ToAngle((keyFrame->animStepAlpha * time) / timeDest);
	_animStepBeta = ToAngle((keyFrame->animStepBeta * time) / timeDest);
	_animStepGamma = ToAngle((keyFrame->animStepGamma * time) / timeDest);

	return false;
}

void Animations::processAnimActions(int32 actorIdx) { // GereAnimAction
	ActorStruct *actor = _engine->_scene->getActor(actorIdx);
	if (actor->_entityDataPtr == nullptr || actor->_ptrAnimAction == AnimationTypes::kAnimNone) {
		return;
	}

	const Common::Array<EntityAnim::Action> *actions = actor->_entityDataPtr->getActions(actor->_ptrAnimAction);
	if (actions == nullptr) {
		return;
	}
	for (const EntityAnim::Action &action : *actions) {
		debugC(1, TwinE::kDebugAnimation, "Execute animation action %d for actor %d", (int)action.type, actorIdx);
		switch (action.type) {
		case ActionType::ACTION_HITTING:
			if (action.animFrame - 1 == actor->_frame) {
				actor->_hitForce = action.strength;
				actor->_workFlags.bIsHitting = 1;
			}
			break;
		case ActionType::ACTION_SAMPLE:
			if (action.animFrame == actor->_frame) {
				_engine->_sound->mixSample3D(action.sampleIndex, 0x1000, 1, actor->posObj(), actorIdx);
			}
			break;
		case ActionType::ACTION_SAMPLE_FREQ:
			if (action.animFrame == actor->_frame) {
				const uint16 pitchBend = 0x1000 + _engine->getRandomNumber(action.frequency) - (action.frequency / 2);
				_engine->_sound->mixSample3D(action.sampleIndex, pitchBend, 1, actor->posObj(), actorIdx);
			}
			break;
		case ActionType::ACTION_THROW_EXTRA_BONUS:
			if (action.animFrame == actor->_frame) {
				_engine->_extra->throwExtra(actorIdx, actor->_posObj.x, actor->_posObj.y + action.yHeight, actor->_posObj.z, action.spriteIndex, action.xAngle, actor->_beta + action.yAngle, action.xRotPoint, action.extraAngle, action.strength);
			}
			break;
		case ActionType::ACTION_THROW_MAGIC_BALL:
			if (_engine->_gameState->_magicBall == -1 && action.animFrame == actor->_frame) {
				_engine->_extra->addExtraThrowMagicball(actor->_posObj.x, actor->_posObj.y + action.yHeight, actor->_posObj.z, action.xAngle, actor->_beta + action.yAngle, action.xRotPoint, action.extraAngle);
			}
			break;
		case ActionType::ACTION_SAMPLE_REPEAT:
			if (action.animFrame == actor->_frame) {
				_engine->_sound->mixSample3D(action.sampleIndex, 0x1000, action.repeat, actor->posObj(), actorIdx);
			}
			break;
		case ActionType::ACTION_THROW_SEARCH:
			if (action.animFrame == actor->_frame) {
				_engine->_extra->addExtraAiming(actorIdx, actor->_posObj.x, actor->_posObj.y + action.yHeight, actor->_posObj.z, action.spriteIndex, action.targetActor, action.finalAngle, action.strength);
			}
			break;
		case ActionType::ACTION_THROW_ALPHA:
			if (action.animFrame == actor->_frame) {
				_engine->_extra->throwExtra(actorIdx, actor->_posObj.x, actor->_posObj.y + action.yHeight, actor->_posObj.z, action.spriteIndex, action.xAngle, actor->_beta + action.yAngle, action.xRotPoint, action.extraAngle, action.strength);
			}
			break;
		case ActionType::ACTION_SAMPLE_STOP:
			if (action.animFrame == actor->_frame) {
				_engine->_sound->stopSample(action.sampleIndex);
			}
			break;
		case ActionType::ACTION_LEFT_STEP:
			if (action.animFrame == actor->_frame && (actor->_brickSound & 0xF0U) != 0xF0U) {
				const int16 sampleIdx = (actor->_brickSound & 0x0FU) + Samples::WalkFloorBegin;
				const uint16 pitchBend = 0x1000 + _engine->getRandomNumber(1000) - 500;
				_engine->_sound->mixSample3D(sampleIdx, pitchBend, 1, actor->posObj(), actorIdx);
			}
			break;
		case ActionType::ACTION_RIGHT_STEP:
			if (action.animFrame == actor->_frame && (actor->_brickSound & 0xF0U) != 0xF0U) {
				const int16 sampleIdx = (actor->_brickSound & 0x0FU) + Samples::WalkFloorRightBegin;
				const uint16 pitchBend = 0x1000 + _engine->getRandomNumber(1000) - 500;
				_engine->_sound->mixSample3D(sampleIdx, pitchBend, 1, actor->posObj(), actorIdx);
			}
			break;
		case ActionType::ACTION_HERO_HITTING:
			if (action.animFrame - 1 == actor->_frame) {
				actor->_hitForce = magicLevelStrengthOfHit[_engine->_gameState->_magicLevelIdx];
				actor->_workFlags.bIsHitting = 1;
			}
			break;
		case ActionType::ACTION_THROW_3D:
			if (action.animFrame == actor->_frame) {
				const IVec2 &destPos = _engine->_renderer->rotate(action.distanceX, action.distanceZ, actor->_beta);

				const int32 throwX = destPos.x + actor->_posObj.x;
				const int32 throwY = action.distanceY + actor->_posObj.y;
				const int32 throwZ = destPos.y + actor->_posObj.z;

				_engine->_extra->throwExtra(actorIdx, throwX, throwY, throwZ, action.spriteIndex,
				                               action.xAngle, action.yAngle + actor->_beta, action.xRotPoint, action.extraAngle, action.strength);
			}
			break;
		case ActionType::ACTION_THROW_3D_ALPHA:
			if (action.animFrame == actor->_frame) {
				const int32 distance = getDistance2D(actor->posObj(), _engine->_scene->_sceneHero->posObj());
				const int32 newAngle = _engine->_movements->getAngle(actor->_posObj.y, 0, _engine->_scene->_sceneHero->_posObj.y, distance);

				const IVec2 &destPos = _engine->_renderer->rotate(action.distanceX, action.distanceZ, actor->_beta);

				const int32 throwX = destPos.x + actor->_posObj.x;
				const int32 throwY = action.distanceY + actor->_posObj.y;
				const int32 throwZ = destPos.y + actor->_posObj.z;

				_engine->_extra->throwExtra(actorIdx, throwX, throwY, throwZ, action.spriteIndex,
				                               action.xAngle + newAngle, action.yAngle + actor->_beta, action.xRotPoint, action.extraAngle, action.strength);
			}
			break;
		case ActionType::ACTION_THROW_3D_SEARCH:
			if (action.animFrame == actor->_frame) {
				const IVec2 &destPos = _engine->_renderer->rotate(action.distanceX, action.distanceZ, actor->_beta);
				const int32 x = actor->_posObj.x + destPos.x;
				const int32 y = actor->_posObj.y + action.distanceY;
				const int32 z = actor->_posObj.z + destPos.y;
				_engine->_extra->addExtraAiming(actorIdx, x, y, z, action.spriteIndex,
				                                action.targetActor, action.finalAngle, action.strength);
			}
			break;
		case ActionType::ACTION_THROW_3D_MAGIC:
			if (_engine->_gameState->_magicBall == -1 && action.animFrame == actor->_frame) {
				const IVec2 &destPos = _engine->_renderer->rotate(action.distanceX, action.distanceZ, actor->_beta);
				const int32 x = actor->_posObj.x + destPos.x;
				const int32 y = actor->_posObj.y + action.distanceY;
				const int32 z = actor->_posObj.z + destPos.y;
				_engine->_extra->addExtraThrowMagicball(x, y, z, action.xAngle, actor->_beta, action.yAngle, action.finalAngle);
			}
			break;
		case ActionType::ACTION_THROW_DART:
			if (_engine->isLBA2() && action.animFrame == actor->_frame) {
				_engine->_dart->throwDart(actor->_posObj.x, actor->_posObj.y + action.distanceY, actor->_posObj.z,
				                          action.xAngle, actor->_beta, action.speed, action.weight);
			}
			break;
		case ActionType::ACTION_FLOW_3D:
			if (_engine->isLBA2() && action.animFrame == actor->_frame) {
				const IVec2 &destPos = _engine->_renderer->rotate(action.distanceX, action.distanceZ, actor->_beta);
				const int32 throwX = destPos.x + actor->_posObj.x;
				const int32 throwY = action.distanceY + actor->_posObj.y;
				const int32 throwZ = destPos.y + actor->_posObj.z;
				_engine->_flow->createParticleFlow(0, actorIdx, 0, throwX, throwY, throwZ, actor->_beta, (int32)action.strength);
			}
			break;
		case ActionType::ACTION_ZV:
		default:
			break;
		}
	}
}

bool Animations::initAnim(AnimationTypes genNewAnim, AnimType flag, AnimationTypes genNextAnim, int32 actorIdx) {
	ActorStruct *actor = _engine->_scene->getActor(actorIdx);
	if (actor->_body == -1) {
		return false;
	}

	if (actor->_flags.bSprite3D) {
		return false;
	}

	if (genNewAnim == actor->_genAnim && actor->_anim != -1) {
		return true;
	}

	if (genNextAnim == AnimationTypes::kNoAnim && actor->_flagAnim != AnimType::kAnimationAllThen) {
		genNextAnim = actor->_genAnim;
	}

	int32 newanim = searchAnim(genNewAnim, actorIdx);

	if (newanim == -1) {
		newanim = searchAnim(AnimationTypes::kStanding, actorIdx);
		if (newanim == -1) {
			error("Could not find anim index for 'standing' (actor %i)", actorIdx);
		}
	}

	if (flag != AnimType::kAnimationSet && actor->_flagAnim == AnimType::kAnimationAllThen) {
		actor->_nextGenAnim = genNewAnim;
		return false;
	}

	if (flag == AnimType::kAnimationInsert) {
		flag = AnimType::kAnimationAllThen;

		genNextAnim = actor->_genAnim;

		if (genNextAnim == AnimationTypes::kThrowBall || genNextAnim == AnimationTypes::kFall || genNextAnim == AnimationTypes::kLanding || genNextAnim == AnimationTypes::kLandingHit) {
			genNextAnim = AnimationTypes::kStanding;
		}
	}

	if (flag == AnimType::kAnimationSet) {
		flag = AnimType::kAnimationAllThen;
	}

	BodyData &bodyData = actor->_entityDataPtr->getBody(actor->_body);
	if (actor->_anim == -1) {
		// if no previous animation
		setAnimObjet(0, _engine->_resources->_animData[newanim], bodyData, &bodyData._animTimerData);
	} else {
		// interpolation between animations (ObjectInitAnim + STOFRAME)
		const AnimData &newAnimData = _engine->_resources->_animData[newanim];
		const AnimData &oldAnimData = _engine->_resources->_animData[actor->_anim];
		AnimTimerDataStruct &timerData = bodyData._animTimerData;
		resetAnimTimerState(&timerData);
		timerData.lastNbGroups = oldAnimData.getNumBoneframes();
		stockInterAnim(bodyData, &timerData, &newAnimData);

		const int32 oldNbGroups = timerData.lastNbGroups;
		const int32 newNbGroups = newAnimData.getNumBoneframes();
		if (newNbGroups > oldNbGroups) {
			const KeyFrame *frame0 = newAnimData.getKeyframe(0);
			if (frame0 != nullptr) {
				for (int32 i = oldNbGroups; i < newNbGroups && i < (int32)frame0->boneframes.size(); ++i) {
					if (i < (int32)bodyData.getNumBones()) {
						*bodyData.getBoneState(i) = frame0->boneframes[i];
					}
				}
			}
		}
	}

	actor->_anim = newanim;
	actor->_genAnim = genNewAnim;
	actor->_nextGenAnim = genNextAnim;
	actor->_ptrAnimAction = _currentActorAnimExtraPtr;

	actor->_flagAnim = flag;
	actor->_frame = 0;

	actor->_workFlags.bIsHitting = 0;
	actor->_workFlags.bAnimEnded = 0;
	actor->_workFlags.bAnimNewFrame = 1;

	processAnimActions(actorIdx);

	actor->_animStepBeta = LBAAngles::ANGLE_0;
	actor->_animStep = IVec3();

	debugC(1, TwinE::kDebugAnimation, "Change animation for actor %d to %d", actorIdx, newanim);

	return true;
}

void Animations::doAnim(int32 actorIdx) {
	ActorStruct *actor = _engine->_scene->getActor(actorIdx);

	if (actor->_body == -1) {
		return;
	}

	const IVec3 &oldPos = actor->_oldPos;

	IVec3 &processActor = actor->_processActor;
	if (actor->_flags.bSprite3D) {
		if (actor->_hitForce) {
			actor->_workFlags.bIsHitting = 1;
		}

		processActor = actor->posObj();

		if (!actor->_workFlags.bIsFalling) {
			if (actor->_srot) {
				int32 xAxisRotation = actor->realAngle.getRealValueFromTime(_engine->timerRef);
				if (!xAxisRotation) {
					if (actor->realAngle.endValue > 0) {
						xAxisRotation = 1;
					} else {
						xAxisRotation = -1;
					}
				}

				const IVec2 xRotPos = _engine->_renderer->rotate(xAxisRotation, 0, actor->_spriteActorRotation);

				processActor.y = actor->_posObj.y - xRotPos.y;

				const IVec2 destPos = _engine->_renderer->rotate(0, xRotPos.x, actor->_beta);

				processActor.x = actor->_posObj.x + destPos.x;
				processActor.z = actor->_posObj.z + destPos.y;

				_engine->_movements->initRealValue(LBAAngles::ANGLE_0, actor->_srot, LBAAngles::ANGLE_17, &actor->realAngle);

				if (actor->_workFlags.bIsSpriteMoving) {
					if (actor->_doorWidth) { // open door
						if (getDistance2D(processActor.x, processActor.z, actor->_animStep.x, actor->_animStep.z) >= actor->_doorWidth) {
							if (actor->_beta == LBAAngles::ANGLE_0) { // down
								processActor.z = actor->_animStep.z + actor->_doorWidth;
							} else if (actor->_beta == LBAAngles::ANGLE_90) { // right
								processActor.x = actor->_animStep.x + actor->_doorWidth;
							} else if (actor->_beta == LBAAngles::ANGLE_180) { // up
								processActor.z = actor->_animStep.z - actor->_doorWidth;
							} else if (actor->_beta == LBAAngles::ANGLE_270) { // left
								processActor.x = actor->_animStep.x - actor->_doorWidth;
							}

							actor->_workFlags.bIsSpriteMoving = 0;
							actor->_srot = 0;
						}
					} else { // close door
						bool updatePos = false;

						if (actor->_beta == LBAAngles::ANGLE_0) { // down
							if (processActor.z <= actor->_animStep.z) {
								updatePos = true;
							}
						} else if (actor->_beta == LBAAngles::ANGLE_90) { // right
							if (processActor.x <= actor->_animStep.x) {
								updatePos = true;
							}
						} else if (actor->_beta == LBAAngles::ANGLE_180) { // up
							if (processActor.z >= actor->_animStep.z) {
								updatePos = true;
							}
						} else if (actor->_beta == LBAAngles::ANGLE_270) { // left
							if (processActor.x >= actor->_animStep.x) {
								updatePos = true;
							}
						}

						if (updatePos) {
							processActor = actor->_animStep;

							actor->_workFlags.bIsSpriteMoving = 0;
							actor->_srot = 0;
						}
					}
				}
			}

			if (actor->_flags.bCanBePushed) {
				processActor += actor->_animStep;

				if (actor->_flags.bUseMiniZv) {
					processActor.x = ((processActor.x / (SIZE_BRICK_XZ / 4)) * (SIZE_BRICK_XZ / 4));
					processActor.z = ((processActor.z / (SIZE_BRICK_XZ / 4)) * (SIZE_BRICK_XZ / 4));
				}

				actor->_animStep = IVec3();
			}
		}
	} else { // 3D actor
		if (actor->_anim != -1) {
			const AnimData &animData = _engine->_resources->_animData[actor->_anim];

			bool keyFramePassed = false;
			BodyData &bodyData = actor->_entityDataPtr->getBody(actor->_body);
			if (bodyData.isAnimated()) {
				keyFramePassed = setInterDepObjet(actor->_frame, animData, &bodyData._animTimerData, actor);
			}

			if (_animMasterRot) {
				actor->_workFlags.bIsRotationByAnim = 1;
			} else {
				actor->_workFlags.bIsRotationByAnim = 0;
			}

			if (!_engine->isLBA2()) {
				actor->_beta = ClampAngle(actor->_beta + _animStepBeta - actor->_animStepBeta);
				actor->_animStepBeta = _animStepBeta;
			} else {
				actor->_animStepBeta = _animStepBeta;
			}

			if (_engine->isLBA2() && actor->_move == ControlMode::kWagon) {
				processActor = actor->posObj();
				_engine->_wagon->DoAnimWagon(actor);
			} else if (_engine->isLBA2() && (actor->_move == ControlMode::kBuggy || actor->_move == ControlMode::kBuggyManual)) {
				processActor = actor->posObj();
				_engine->_buggy->doAnimBuggy(actor);
			} else if (_engine->isLBA2()) {
				processActor = actor->posObj() + _animStep;
			} else {
				const IVec2 &destPos = _engine->_renderer->rotate(_animStep.x, _animStep.z, actor->_beta);
				_animStep.x = destPos.x;
				_animStep.z = destPos.y;
				processActor = actor->posObj() + _animStep - actor->_animStep;
			}

			actor->_animStep = _animStep;

			actor->_workFlags.bAnimEnded = 0;
			actor->_workFlags.bAnimNewFrame = 0;

			if (keyFramePassed) {
				actor->_frame++;
				actor->_workFlags.bAnimNewFrame = 1;

				if (_engine->isLBA2() && bodyData.isAnimated()) {
					int32 numOfBonesInAnim = animData.getNumBoneframes();
					if (numOfBonesInAnim > bodyData.getNumBones()) {
						numOfBonesInAnim = bodyData.getNumBones();
					}
					const KeyFrame *newFrame = animData.getKeyframe(actor->_frame);
					if (newFrame != nullptr) {
						copyKeyFrameToState(newFrame, bodyData, numOfBonesInAnim);
					}
				}

				// if actor have animation actions to process
				processAnimActions(actorIdx);

				int16 numKeyframe = actor->_frame;
				if (numKeyframe == (int16)animData.getNbFramesAnim()) {
					actor->_workFlags.bIsHitting = 0;

					if (actor->_flagAnim == AnimType::kAnimationTypeRepeat) {
						actor->_frame = animData.getLoopFrame();
						if (bodyData.isAnimated()) {
							const KeyFrame *loopFrame = animData.getKeyframe(actor->_frame);
							if (loopFrame != nullptr) {
								int32 loopBones = animData.getNumBoneframes();
								if (loopBones > bodyData.getNumBones()) {
									loopBones = bodyData.getNumBones();
								}
								copyKeyFrameToState(loopFrame, bodyData, loopBones);
							}
						}
					} else {
						actor->_genAnim = actor->_nextGenAnim;
						actor->_anim = searchAnim(actor->_genAnim, actorIdx);

						if (actor->_anim == -1) {
							actor->_anim = searchAnim(AnimationTypes::kStanding, actorIdx);
							actor->_genAnim = AnimationTypes::kStanding;
						}

						actor->_ptrAnimAction = _currentActorAnimExtraPtr;

						actor->_flagAnim = AnimType::kAnimationTypeRepeat;
						actor->_frame = 0;
						actor->_hitForce = 0;
					}

					processAnimActions(actorIdx);

					actor->_workFlags.bAnimEnded = 1;
				}

				actor->_animStepBeta = LBAAngles::ANGLE_0;

				actor->_animStep = IVec3();
			}
		}
	}

	Collision* collision = _engine->_collision;
	// actor standing on another actor
	if (actor->_carryBy != -1) {
		const ActorStruct *standOnActor = _engine->_scene->getActor(actor->_carryBy);
		processActor -= standOnActor->_oldPos;
		processActor += standOnActor->posObj();

		if (!collision->checkZvOnZv(actorIdx, actor->_carryBy)) {
			actor->_carryBy = -1; // no longer standing on other actor
		}
	}

	// actor falling Y speed
	if (actor->_workFlags.bIsFalling) {
		processActor = oldPos;
		processActor.y += _engine->_stepFalling; // add step to fall
	}

	// actor collisions with bricks
	uint32 col1 = 0; 	/** Cause damage in current processed actor */
	if (actor->_flags.bComputeCollisionWithBricks) {
		ShapeType col = _engine->_grid->worldColBrick(oldPos);

		if (col != ShapeType::kNone) {
			if (col == ShapeType::kSolid) {
				processActor.y = (processActor.y / SIZE_BRICK_Y) * SIZE_BRICK_Y + SIZE_BRICK_Y; // go upper
				actor->_posObj.y = processActor.y;
			} else {
				collision->reajustPos(processActor, col);
			}
		}

		if (actor->_flags.bComputeCollisionWithObj) {
			collision->checkObjCol(actorIdx);
		}

		if (actor->_carryBy != -1 && actor->_workFlags.bIsFalling) {
			collision->receptionObj(actorIdx);
		}

		collision->setCollisionPos(processActor);

		if (IS_HERO(actorIdx) && !actor->_flags.bComputeLowCollision) {
			// check hero collisions with bricks
			col1 |= collision->doCornerReajustTwinkel(actor, actor->_boundingBox.mins.x, actor->_boundingBox.mins.y, actor->_boundingBox.mins.z, 1);
			col1 |= collision->doCornerReajustTwinkel(actor, actor->_boundingBox.maxs.x, actor->_boundingBox.mins.y, actor->_boundingBox.mins.z, 2);
			col1 |= collision->doCornerReajustTwinkel(actor, actor->_boundingBox.maxs.x, actor->_boundingBox.mins.y, actor->_boundingBox.maxs.z, 4);
			col1 |= collision->doCornerReajustTwinkel(actor, actor->_boundingBox.mins.x, actor->_boundingBox.mins.y, actor->_boundingBox.maxs.z, 8);
		} else {
			// check other actors collisions with bricks
			col1 |= collision->doCornerReajust(actor, actor->_boundingBox.mins.x, actor->_boundingBox.mins.y, actor->_boundingBox.mins.z, 1);
			col1 |= collision->doCornerReajust(actor, actor->_boundingBox.maxs.x, actor->_boundingBox.mins.y, actor->_boundingBox.mins.z, 2);
			col1 |= collision->doCornerReajust(actor, actor->_boundingBox.maxs.x, actor->_boundingBox.mins.y, actor->_boundingBox.maxs.z, 4);
			col1 |= collision->doCornerReajust(actor, actor->_boundingBox.mins.x, actor->_boundingBox.mins.y, actor->_boundingBox.maxs.z, 8);
		}

		// process wall hit while running
		if (col1 && !actor->_workFlags.bIsFalling && IS_HERO(actorIdx) && _engine->_actor->_heroBehaviour == HeroBehaviourType::kAthletic && actor->_genAnim == AnimationTypes::kForward) {
			IVec2 destPos = _engine->_renderer->rotate(actor->_boundingBox.mins.x, actor->_boundingBox.mins.z, actor->_beta + LBAAngles::ANGLE_315 + LBAAngles::ANGLE_180);

			destPos.x += processActor.x;
			destPos.y += processActor.z;

			if (destPos.x >= 0 && destPos.y >= 0 && destPos.x <= SCENE_SIZE_MAX && destPos.y <= SCENE_SIZE_MAX) {
				if (_engine->_grid->worldColBrick(destPos.x, processActor.y + SIZE_BRICK_Y, destPos.y) != ShapeType::kNone && _engine->_cfgfile.WallCollision) { // avoid wall hit damage
					_engine->_extra->initSpecial(actor->_posObj.x, actor->_posObj.y + 1000, actor->_posObj.z, ExtraSpecialType::kHitStars);
					initAnim(AnimationTypes::kBigHit, AnimType::kAnimationAllThen, AnimationTypes::kStanding, actorIdx);

					if (IS_HERO(actorIdx)) {
						_engine->_movements->_lastJoyFlag = true;
					}

					actor->addLife(-1);
				}
			}
		}

		col = _engine->_grid->worldColBrick(processActor);
		actor->setCollision(col);

		if (col != ShapeType::kNone) {
			if (col == ShapeType::kSolid) {
				if (actor->_workFlags.bIsFalling) {
					collision->receptionObj(actorIdx);
					processActor.y = (collision->_collision.y * SIZE_BRICK_Y) + SIZE_BRICK_Y;
				} else {
					if (IS_HERO(actorIdx) && _engine->_actor->_heroBehaviour == HeroBehaviourType::kAthletic && actor->_genAnim == AnimationTypes::kForward && _engine->_cfgfile.WallCollision) { // avoid wall hit damage
						_engine->_extra->initSpecial(actor->_posObj.x, actor->_posObj.y + 1000, actor->_posObj.z, ExtraSpecialType::kHitStars);
						initAnim(AnimationTypes::kBigHit, AnimType::kAnimationAllThen, AnimationTypes::kStanding, actorIdx);
						_engine->_movements->_lastJoyFlag = true;
						actor->addLife(-1);
					}

					// no Z coordinate issue
					if (_engine->_grid->worldColBrick(processActor.x, processActor.y, oldPos.z) != ShapeType::kNone) {
						if (_engine->_grid->worldColBrick(oldPos.x, processActor.y, processActor.z) != ShapeType::kNone) {
							return;
						} else {
							processActor.x = oldPos.x;
						}
					} else {
						processActor.z = oldPos.z;
					}
				}
			} else {
				if (actor->_workFlags.bIsFalling) {
					collision->receptionObj(actorIdx);
				}

				collision->reajustPos(processActor, col);
			}

			if (actor->_workFlags.bIsFalling) {
				debugC(1, TwinE::kDebugCollision, "Actor %d reset falling", actorIdx);
			}
			actor->_workFlags.bIsFalling = 0;
		} else {
			if (actor->_flags.bObjFallable && actor->_carryBy == -1) {
				col = _engine->_grid->worldColBrick(processActor.x, processActor.y - 1, processActor.z);

				if (col != ShapeType::kNone) {
					if (actor->_workFlags.bIsFalling) {
						collision->receptionObj(actorIdx);
					}

					collision->reajustPos(processActor, col);
				} else {
					if (!actor->_workFlags.bIsRotationByAnim) {
						debugC(1, TwinE::kDebugCollision, "Actor %d is falling", actorIdx);
						actor->_workFlags.bIsFalling = 1;

						if (IS_HERO(actorIdx) && _engine->_scene->_startYFalling == 0) {
							_engine->_scene->_startYFalling = processActor.y;
							int32 y = processActor.y - 1 - SIZE_BRICK_Y;
							while (y > 0 && ShapeType::kNone == _engine->_grid->worldColBrick(processActor.x, y, processActor.z)) {
								y -= SIZE_BRICK_Y;
							}

							y = (y + SIZE_BRICK_Y) & ~(SIZE_BRICK_Y - 1);
							int32 fallHeight = processActor.y - y;

							if (fallHeight <= (2 * SIZE_BRICK_Y) && actor->_genAnim == AnimationTypes::kForward) {
								actor->_workFlags.bWasWalkingBeforeFalling = 1;
							} else {
								initAnim(AnimationTypes::kFall, AnimType::kAnimationTypeRepeat, AnimationTypes::kNoAnim, actorIdx);
							}
						} else {
							initAnim(AnimationTypes::kFall, AnimType::kAnimationTypeRepeat, AnimationTypes::kNoAnim, actorIdx);
						}
					}
				}
			}
		}

		// if under the map, than die
		if (collision->_collision.y == -1) {
			actor->setLife(0);
		}
	} else {
		if (actor->_flags.bComputeCollisionWithObj) {
			collision->checkObjCol(actorIdx);
		}
	}

	if (col1) {
		actor->setBrickCausesDamage();
	}

	// check and fix actor bounding position
	if (processActor.x < 0) {
		processActor.x = 0;
	}

	if (processActor.y < 0) {
		processActor.y = 0;
	}

	if (processActor.z < 0) {
		processActor.z = 0;
	}

	if (processActor.x > SCENE_SIZE_MAX) {
		processActor.x = SCENE_SIZE_MAX;
	}

	if (processActor.z > SCENE_SIZE_MAX) {
		processActor.z = SCENE_SIZE_MAX;
	}

	actor->_posObj = processActor;
}

} // namespace TwinE
