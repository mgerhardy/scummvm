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

#include "holomap.h"
#include "gamestate.h"

/** Set Holomap location position
	@location Scene where position must be set */
void setHolomapPosition(int32 location) {
	holomapFlags[location] = 0x81;
}

/** Clear Holomap location position
	@location Scene where position must be cleared */
void clearHolomapPosition(int32 location) {
	holomapFlags[location] &= 0x7E;
	holomapFlags[location] |= 0x40;
}
