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

#include "common/scummsys.h"

namespace TwinE {

/** Pressed key map - scanCodeTab1 */
extern uint8 pressedKeyMap[29];
/** Pressed key char map - scanCodeTab2 */
extern uint16 pressedKeyCharMap[31];

/** Skipped key - key1 */
int16 skippedKey;
/** Pressed key - printTextVar12 */
int16 pressedKey;
//int printTextVar13;
/** Skip intro variable */
int16 skipIntro;
/** Current key value */
int16 currentKey;
/** Auxiliar key value */
int16 key;

int32 heroPressedKey;
int32 heroPressedKey2;

} // namespace TwinE

#endif
