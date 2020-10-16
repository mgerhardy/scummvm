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

#ifndef TWINE_SDLENGINE_H
#define TWINE_SDLENGINE_H

#include "common/scummsys.h"
#include "twine/debug.h"

namespace TwinE {

struct SDL_Surface;

/** Main SDL screen surface buffer */
extern SDL_Surface *screen;

/** SDL initializer
	@return SDL init state */
int sdlInitialize();

/** Frames per second sdl delay */
void fpsCycles(int32 fps);

/** Deplay certain seconds till proceed
	@param time time in seconds to delay */
void sdldelay(uint32 time);

/** Deplay certain seconds till proceed - Can also Skip this delay
	@param time time in seconds to delay */
void delaySkip(uint32 time);

/** Set a new palette in the SDL screen buffer
	@param palette palette to set */
void setPalette(uint8 *palette);

/** Fade screen from black to white */
void fadeBlackToWhite();

/** Blit surface in the screen */
void flip();

/** Blit surface in the screen in a determinate area
	@param left left position to start copy
	@param top top position to start copy
	@param right right position to start copy
	@param bottom bottom position to start copy */
void copyBlockPhys(int32 left, int32 top, int32 right, int32 bottom);

/** Create SDL screen surface
	@param buffer screen buffer to blit surface
	@param width screen width size
	@param height screen height size */
void initScreenBuffer(uint8 *buffer, int32 width, int32 height);

/** Cross fade feature
	@param buffer screen buffer
	@param palette new palette to cross fade*/
void crossFade(uint8 *buffer, uint8 *palette);

/** Switch between window and fullscreen modes */
void toggleFullscreen();

/** Handle keyboard pressed keys */
void readKeys();

/** Display SDL text in screen
	@param X X coordinate in screen
	@param Y Y coordinate in screen
	@param string text to display
	@param center if the text should be centered accoding with the giving positions */
void ttfDrawText(int32 X, int32 Y, int8 *string, int32 center);

/** Gets SDL mouse positions
	@param mouseData structure that contains mouse position info */
void getMousePositions(MouseStatusStruct *mouseData);

} // namespace TwinE

#endif
