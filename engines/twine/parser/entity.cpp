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

#include "twine/parser/entity.h"
#include "common/stream.h"
#include "twine/resources/resources.h"
#include "twine/shared.h"

namespace TwinE {

bool EntityData::loadBody(Common::SeekableReadStream &stream, bool lba1) {
	EntityBody body;
	body.index = stream.readByte();
	const int64 recordStart = stream.pos() - 1;
	uint8 recordSize = stream.readByte();
	if (!lba1) {
		if (recordSize > 0) {
			recordSize += 1;
		}
	}
	body.hqrBodyIndex = (int16)stream.readUint16LE();
	if (!body.body.loadFromHQR(TwineResource(Resources::HQR_BODY_FILE, body.hqrBodyIndex), lba1)) {
		error("Failed to load body with index: %i", body.hqrBodyIndex);
	}
	if (lba1) {
		const uint8 numActions = stream.readByte();
		for (uint8 i = 0U; i < numActions; ++i) {
			if ((ActionType)stream.readByte() == ActionType::ACTION_ZV) {
				body.actorBoundingBox.hasBoundingBox = true;
				body.actorBoundingBox.bbox.mins.x = stream.readSint16LE();
				body.actorBoundingBox.bbox.mins.y = stream.readSint16LE();
				body.actorBoundingBox.bbox.mins.z = stream.readSint16LE();
				body.actorBoundingBox.bbox.maxs.x = stream.readSint16LE();
				body.actorBoundingBox.bbox.maxs.y = stream.readSint16LE();
				body.actorBoundingBox.bbox.maxs.z = stream.readSint16LE();
			}
		}
	} else {
		const uint8 hasCollisionBox = stream.readByte();
		if (hasCollisionBox == 1) {
			const ActionType actionType = (ActionType)stream.readByte();
			if (actionType == ActionType::ACTION_ZV) {
				body.actorBoundingBox.hasBoundingBox = true;
				body.actorBoundingBox.bbox.mins.x = stream.readSint16LE();
				body.actorBoundingBox.bbox.mins.y = stream.readSint16LE();
				body.actorBoundingBox.bbox.mins.z = stream.readSint16LE();
				body.actorBoundingBox.bbox.maxs.x = stream.readSint16LE();
				body.actorBoundingBox.bbox.maxs.y = stream.readSint16LE();
				body.actorBoundingBox.bbox.maxs.z = stream.readSint16LE();
			}
		}
	}
	_bodies.push_back(body);
	stream.seek(recordStart + recordSize);
	return !stream.err();
}

bool EntityData::loadAnim(Common::SeekableReadStream &stream, bool lba1) {
	EntityAnim anim;
	const int64 recordStart = stream.pos();
	if (lba1) {
		anim.animation = (AnimationTypes)stream.readByte();
	} else {
		anim.animation = (AnimationTypes)stream.readUint16LE();
	}
	uint8 recordSize = stream.readByte();
	if (!lba1) {
		if (recordSize > 0) {
			recordSize += 2;
		} else {
			recordSize = 6;
		}
	}
	anim.animIndex = stream.readSint16LE();
	const uint8 numActions = stream.readByte();
	for (uint8 i = 0U; i < numActions; ++i) {
		EntityAnim::Action action;
		action.type = (ActionType)stream.readByte();
		action.animFrame = stream.readByte();

		if (lba1) {
			switch (action.type) {
			case ActionType::ACTION_HITTING:
				action.strength = stream.readByte();
				break;
			case ActionType::ACTION_SAMPLE:
				action.sampleIndex = stream.readSint16LE();
				break;
			case ActionType::ACTION_SAMPLE_FREQ:
				action.sampleIndex = stream.readSint16LE();
				action.frequency = stream.readSint16LE();
				break;
			case ActionType::ACTION_THROW_MAGIC_BALL:
				action.yHeight = stream.readSint16LE();
				action.xAngle = ToAngle(stream.readSint16LE());
				action.xRotPoint = stream.readSint16LE();
				action.extraAngle = stream.readByte();
				break;
			case ActionType::ACTION_SAMPLE_REPEAT:
				action.sampleIndex = stream.readSint16LE();
				action.repeat = stream.readSint16LE();
				break;
			case ActionType::ACTION_THROW_SEARCH:
				action.yHeight = stream.readSint16LE();
				action.spriteIndex = stream.readByte();
				action.targetActor = stream.readByte();
				action.finalAngle = stream.readSint16LE();
				action.strength = stream.readByte();
				break;
			case ActionType::ACTION_THROW_EXTRA_BONUS:
			case ActionType::ACTION_THROW_ALPHA:
				action.yHeight = stream.readSint16LE();
				action.spriteIndex = stream.readByte();
				action.xAngle = ToAngle(stream.readSint16LE());
				action.yAngle = ToAngle(stream.readSint16LE());
				action.xRotPoint = stream.readSint16LE();
				action.extraAngle = ToAngle(stream.readByte());
				action.strength = stream.readByte();
				break;
			case ActionType::ACTION_LEFT_STEP:
			case ActionType::ACTION_RIGHT_STEP:
			case ActionType::ACTION_HERO_HITTING:
				break;
			case ActionType::ACTION_SAMPLE_STOP:
				action.sampleIndex = stream.readByte();
				stream.skip(1);
				break;
			case ActionType::ACTION_THROW_3D:
			case ActionType::ACTION_THROW_3D_ALPHA:
				action.distanceX = stream.readSint16LE();
				action.distanceY = stream.readSint16LE();
				action.distanceZ = stream.readSint16LE();
				action.spriteIndex = stream.readByte();
				action.xAngle = ToAngle(stream.readSint16LE());
				action.yAngle = ToAngle(stream.readSint16LE());
				action.xRotPoint = stream.readSint16LE();
				action.extraAngle = ToAngle(stream.readByte());
				action.strength = stream.readByte();
				break;
			case ActionType::ACTION_THROW_3D_SEARCH:
				action.distanceX = stream.readSint16LE();
				action.distanceY = stream.readSint16LE();
				action.distanceZ = stream.readSint16LE();
				action.spriteIndex = stream.readByte();
				action.targetActor = stream.readByte();
				action.finalAngle = ToAngle(stream.readSint16LE());
				action.strength = stream.readByte();
				break;
			case ActionType::ACTION_THROW_3D_MAGIC:
				action.distanceX = stream.readSint16LE();
				action.distanceY = stream.readSint16LE();
				action.distanceZ = stream.readSint16LE();
				action.xAngle = stream.readSint16LE();
				action.yAngle = stream.readSint16LE();
				action.finalAngle = stream.readByte();
				break;
			case ActionType::ACTION_ZV:
			default:
				break;
			}
		} else {
			switch (action.type) {
			case ActionType::ACTION_ZV:
				action.bbox.mins.x = stream.readSint16LE();
				action.bbox.mins.y = stream.readSint16LE();
				action.bbox.mins.z = stream.readSint16LE();
				action.bbox.maxs.x = stream.readSint16LE();
				action.bbox.maxs.y = stream.readSint16LE();
				action.bbox.maxs.z = stream.readSint16LE();
				break;
			case ActionType::ACTION_ZV_ANIMIT:
				break;
			case ActionType::ACTION_SUPER_HIT:
				action.strength = stream.readByte();
				stream.skip(9);
				break;
			case ActionType::ACTION_HITTING:
				action.strength = stream.readByte();
				break;
			case ActionType::ACTION_SAMPLE:
				action.sampleIndex = stream.readUint16LE();
				break;
			case ActionType::ACTION_NEW_SAMPLE:
				action.sampleIndex = stream.readUint16LE();
				action.decal = stream.readUint16LE();
				action.sampleVolume = stream.readByte();
				action.frequency = stream.readUint16LE();
				break;
			case ActionType::ACTION_SAMPLE_FREQ:
				action.sampleIndex = stream.readUint16LE();
				action.decal = stream.readUint16LE();
				break;
			case ActionType::ACTION_THROW_EXTRA_BONUS:
				action.yHeight = stream.readSint16LE();
				action.spriteIndex = stream.readByte();
				action.alpha = stream.readSint16LE();
				action.beta = stream.readSint16LE();
				action.speed = stream.readSint16LE();
				action.weight = stream.readSByte();
				action.strength = stream.readByte();
				break;
			case ActionType::ACTION_THROW_MAGIC_BALL:
				action.unk1 = stream.readUint16LE();
				action.unk2 = stream.readUint16LE();
				action.unk3 = stream.readUint16LE();
				action.unk4 = stream.readByte();
				break;
			case ActionType::ACTION_SAMPLE_REPEAT:
				action.sampleIndex = stream.readUint16LE();
				action.repeat = stream.readUint16LE();
				action.decal = stream.readUint16LE();
				action.sampleVolume = stream.readByte();
				action.frequency = stream.readUint16LE();
				break;
			case ActionType::ACTION_THROW_SEARCH:
				action.yHeight = stream.readSint16LE();
				action.spriteIndex = stream.readByte();
				action.targetActor = stream.readByte();
				action.speed = stream.readSint16LE();
				action.strength = stream.readByte();
				break;
			case ActionType::ACTION_THROW_ALPHA:
				action.yHeight = stream.readSint16LE();
				action.spriteIndex = stream.readByte();
				action.alpha = stream.readSint16LE();
				action.beta = stream.readSint16LE();
				action.speed = stream.readSint16LE();
				action.weight = stream.readSByte();
				action.strength = stream.readByte();
				break;
			case ActionType::ACTION_SAMPLE_STOP:
				action.sampleIndex = stream.readUint16LE();
				stream.skip(1);
				break;
			case ActionType::ACTION_LEFT_STEP:
			case ActionType::ACTION_RIGHT_STEP:
			case ActionType::ACTION_HERO_HITTING:
				break;
			case ActionType::ACTION_THROW_3D:
			case ActionType::ACTION_THROW_3D_ALPHA:
				action.distanceX = stream.readSint16LE();
				action.distanceY = stream.readSint16LE();
				action.distanceZ = stream.readSint16LE();
				action.spriteIndex = stream.readByte();
				action.alpha = stream.readSint16LE();
				action.beta = stream.readSint16LE();
				action.speed = stream.readSint16LE();
				action.weight = stream.readSByte();
				action.strength = stream.readByte();
				break;
			case ActionType::ACTION_THROW_3D_SEARCH:
				action.distanceX = stream.readSint16LE();
				action.distanceY = stream.readSint16LE();
				action.distanceZ = stream.readSint16LE();
				action.spriteIndex = stream.readByte();
				action.targetActor = stream.readByte();
				action.speed = stream.readSint16LE();
				action.strength = stream.readByte();
				break;
			case ActionType::ACTION_THROW_3D_MAGIC:
				stream.skip(11);
				break;
			case ActionType::ACTION_THROW_OBJ_3D:
				action.distanceX = stream.readSint16LE();
				action.distanceY = stream.readSint16LE();
				action.distanceZ = stream.readSint16LE();
				action.modelIndex = stream.readSint16LE();
				action.alpha = stream.readSint16LE();
				action.beta = stream.readSint16LE();
				action.speed = stream.readSint16LE();
				action.weight = stream.readSByte();
				action.strength = stream.readByte();
				break;
			case ActionType::ACTION_FLOW_3D:
				action.distanceX = stream.readSint16LE();
				action.distanceY = stream.readSint16LE();
				action.distanceZ = stream.readSint16LE();
				action.strength = stream.readSByte();
				break;
			case ActionType::ACTION_THROW_DART:
				action.distanceY = stream.readSint16LE();
				action.alpha = stream.readSint16LE();
				action.speed = stream.readSint16LE();
				action.weight = stream.readSByte();
				break;
			case ActionType::ACTION_SHIELD:
				action.lastAnimFrame = stream.readByte();
				break;
			case ActionType::ACTION_SAMPLE_MAGIC:
				break;
			case ActionType::ACTION_THROW_3D_CONQUE:
				stream.skip(7);
				break;
			case ActionType::ACTION_IMPACT:
				action.strength = stream.readSint16LE();
				break;
			case ActionType::ACTION_RENVOIE:
			case ActionType::ACTION_TRANSPARENT:
				break;
			case ActionType::ACTION_RENVOYABLE:
				action.strength = stream.readByte();
				break;
			case ActionType::ACTION_SCALE:
				action.scale = stream.readSint32LE();
				break;
			case ActionType::ACTION_LEFT_JUMP:
			case ActionType::ACTION_RIGHT_JUMP:
			case ActionType::ACTION_THROW_FOUDRE:
				break;
			case ActionType::ACTION_IMPACT_3D:
				stream.skip(8);
				break;
			case ActionType::ACTION_THROW_MAGIC_EXTRA:
				stream.skip(8);
				break;
			case ActionType::ACTION_PATH:
			case ActionType::ACTION_FLOW:
			default:
				break;
			}
		}
		if ((uint8)action.type == 0xFF) {
			break;
		}
		if (action.type > ActionType::ACTION_THROW_FOUDRE) {
			warning("Unknown action type %d in animation %d", (int)action.type, (int)anim.animation);
			break;
		}
		anim._actions.push_back(action);
	}
	_animations.push_back(anim);
	if (lba1) {
		stream.seek(recordStart + 1 + recordSize);
	} else {
		stream.seek(recordStart + recordSize);
	}
	return !stream.err();
}

void EntityData::reset() {
	_animations.clear();
	_bodies.clear();
}

bool EntityData::loadFromStream(Common::SeekableReadStream &stream, bool lba1) {
	reset();
	do {
		const uint8 opcode = stream.readByte();
		if (opcode == 1) {
			if (!loadBody(stream, lba1)) {
				return false;
			}
		} else if (opcode == 3) {
			if (!loadAnim(stream, lba1)) {
				return false;
			}
		} else if (opcode == 0xFF) {
			break;
		} else {
			// Match FICHE.CPP default: skip unknown record types
			stream.readByte(); // gen
			const uint8 skip = stream.readByte();
			stream.skip(skip);
		}
	} while (!stream.eos() && !stream.err());

	return true;
}

const Common::Array<EntityAnim::Action> *EntityData::getActions(AnimationTypes animation) const {
	for (const EntityAnim &anim : _animations) {
		if (anim.animation == animation) {
			if (anim._actions.empty()) {
				return nullptr;
			}
			return &anim._actions;
		}
	}
	return nullptr;
}

BodyData &EntityData::getBody(int index) {
	for (EntityBody &body : _bodies) {
		if (body.index == index) {
			return body.body;
		}
	}
	error("Could not find body for index: %i", index);
}

const EntityBody *EntityData::getEntityBody(const int index) const {
	for (const EntityBody &body : _bodies) {
		if (body.index == index) {
			return &body;
		}
	}
	return nullptr;
}

int32 EntityData::getAnimIndex(AnimationTypes animation) const {
	for (const EntityAnim &anim : _animations) {
		if (anim.animation == animation) {
			return anim.animIndex;
		}
	}
	return -1;
}

} // End of namespace TwinE
