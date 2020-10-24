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

#ifndef TWINE_KEYBOARD_H
#define TWINE_KEYBOARD_H

#include "common/keyboard.h"
#include "common/scummsys.h"
#include "common/util.h"

namespace TwinE {

enum TwinEActionType {
	Pause,
	NextRoom,
	PreviousRoom,
	ApplyCellingGrid,
	IncreaseCellingGridIndex,
	DecreaseCellingGridIndex,
	DebugGridCameraPressUp,
	DebugGridCameraPressDown,
	DebugGridCameraPressLeft,
	DebugGridCameraPressRight,
	QuickBehaviourNormal,
	QuickBehaviourAthletic,
	QuickBehaviourAggressive,
	QuickBehaviourDiscreet,
	ExecuteBehaviourAction,
	BehaviourMenu,
	OptionsMenu,
	RecenterScreenOnTwinsen,
	UseSelectedObject,
	ThrowMagicBall,
	MoveForward,
	MoveBackward,
	TurnRight,
	TurnLeft,
	UseProtoPack,
	OpenHolomap,
	InventoryMenu,
	SpecialAction,
	Escape,
	PageUp,

	Max
};

static constexpr const struct ActionMapping {
	TwinEActionType action;
	uint8 localKey;
} twineactions[] = {
    {Pause, 0x19},
    {NextRoom, 0x13},
    {PreviousRoom, 0x21},
    {ApplyCellingGrid, 0x14},
    {IncreaseCellingGridIndex, 0x22},
    {DecreaseCellingGridIndex, 0x30},
    {DebugGridCameraPressUp, 0x2E},
    {DebugGridCameraPressDown, 0x2C},
    {DebugGridCameraPressLeft, 0x1F},
    {DebugGridCameraPressRight, 0x2D},
    {QuickBehaviourNormal, 0x3B},
    {QuickBehaviourAthletic, 0x3C},
    {QuickBehaviourAggressive, 0x3D},
    {QuickBehaviourDiscreet, 0x3E},
    {ExecuteBehaviourAction, 0x39},
    {BehaviourMenu, 0x1D},
    {OptionsMenu, 0x40},
    {RecenterScreenOnTwinsen, 0x1C},
    {UseSelectedObject, 0x1C},
    {ThrowMagicBall, 0x38},
    {MoveForward, 0x48},
    {MoveBackward, 0x50},
    {TurnRight, 0x4D},
    {TurnLeft, 0x4B},
    {UseProtoPack, 0x24},
    {OpenHolomap, 0x23},
    {InventoryMenu, 0x36},
    {SpecialAction, 0x11},
    {Escape, 0x01},
    {PageUp, 0x49} // TODO: used for what?
};

static_assert(ARRAYSIZE(twineactions) == TwinEActionType::Max, "Unexpected action mapping array size");

class TwinEEngine;

class Input {
private:
	friend class TwinEEngine;
	TwinEEngine *_engine;
	bool _hitEnter = false;

public:
	Input(TwinEEngine *engine);

	bool actionStates[TwinEActionType::Max]{false};
	int16 skippedKey = 0;
	int16 pressedKey = 0;
	int16 internalKeyCode = 0;
	int16 currentKey = 0;
	int16 key = 0;
	int32 heroPressedKey = 0;
	int32 heroPressedKey2 = 0;
	int16 leftMouse = 0;
	int16 rightMouse = 0;

	bool isAnyKeyPressed() const;

	bool isPressed(Common::KeyCode keycode);

	void readKeys();
};

} // namespace TwinE

#endif
