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

#include "twine/parser/anim.h"
#include "common/memstream.h"

namespace TwinE {

void AnimData::loadBoneFrame(KeyFrame &keyframe, Common::SeekableReadStream &stream) {
	BoneFrame boneframe;
	boneframe.type = (BoneType)stream.readSint16LE();
	boneframe.x = stream.readSint16LE();
	boneframe.y = stream.readSint16LE();
	boneframe.z = stream.readSint16LE();
	keyframe.boneframes.push_back(boneframe);
}

void AnimData::loadKeyFramesLBA2(Common::SeekableReadStream &stream) {
	const uint16 nbFrames = _numKeyframes;
	const uint16 nbGroups = _numBoneframes;

	_keyframes.reserve(nbFrames);
	for (uint16 frameIdx = 0; frameIdx < nbFrames; ++frameIdx) {
		KeyFrame keyframe;
		keyframe.length = stream.readUint16LE();
		keyframe.x = stream.readSint16LE();
		keyframe.y = stream.readSint16LE();
		keyframe.z = stream.readSint16LE();
		stream.readSint16LE(); // reserved

		// Group 0 (master) — not copied to CurrentFrame in the original engine
		keyframe.animMasterRot = stream.readSint16LE();
		keyframe.animStepAlpha = stream.readSint16LE();
		keyframe.animStepBeta = stream.readSint16LE();
		keyframe.animStepGamma = stream.readSint16LE();

		keyframe.boneframes.clear();
		keyframe.boneframes.reserve(nbGroups);

		BoneFrame masterBone;
		masterBone.type = BoneType::TYPE_ROTATE;
		masterBone.x = keyframe.animStepAlpha;
		masterBone.y = keyframe.animStepBeta;
		masterBone.z = keyframe.animStepGamma;
		keyframe.boneframes.push_back(masterBone);

		for (uint16 groupIdx = 1; groupIdx < nbGroups; ++groupIdx) {
			loadBoneFrame(keyframe, stream);
		}

		_keyframes.push_back(keyframe);
		assert(keyframe.boneframes.size() == (uint)nbGroups);
	}
}

void AnimData::loadKeyFrames(Common::SeekableReadStream &stream) {
	for (uint16 i = 0U; i < _numKeyframes; ++i) {
		KeyFrame keyframe;
		keyframe.length = stream.readUint16LE();
		keyframe.x = stream.readSint16LE();
		keyframe.y = stream.readSint16LE();
		keyframe.z = stream.readSint16LE();

		keyframe.animMasterRot = stream.readSint16LE();
		keyframe.animStepAlpha = stream.readSint16LE();
		keyframe.animStepBeta = stream.readSint16LE();
		keyframe.animStepGamma = stream.readSint16LE();
		stream.seek(-8, SEEK_CUR);

		for (uint16 j = 0U; j < _numBoneframes; ++j) {
			loadBoneFrame(keyframe, stream);
		}

		_keyframes.push_back(keyframe);
		assert(keyframe.boneframes.size() == (uint)_numBoneframes);
	}
}

void AnimData::reset() {
	_keyframes.clear();
}

bool AnimData::loadFromStream(Common::SeekableReadStream &stream, bool lba1) {
	reset();
	_numKeyframes = stream.readUint16LE();
	_numBoneframes = stream.readUint16LE();
	_loopFrame = stream.readUint16LE();
	stream.readUint16LE();

	if (lba1) {
		loadKeyFrames(stream);
	} else {
		loadKeyFramesLBA2(stream);
	}

	return !stream.err();
}

const Common::Array<KeyFrame>& AnimData::getKeyframes() const {
	return _keyframes;
}

const KeyFrame* AnimData::getKeyframe(uint index) const {
	if (index >= _numKeyframes) {
		return nullptr;
	}
	return &_keyframes[index];
}

uint16 AnimData::getLoopFrame() const { // GetBouclageAnim
	return _loopFrame;
}

uint16 AnimData::getNumBoneframes() const {
	return _numBoneframes;
}

} // namespace TwinE
