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

#ifndef TWINE_FLAMOVIES_H
#define TWINE_FLAMOVIES_H

#include "common/scummsys.h"
#include "twine/filereader.h"

namespace TwinE {

/** FLA movie directory */
#define FLA_DIR "fla/"
/** Original FLA screen width */
#define FLASCREEN_WIDTH 320
/** Original FLA screen height */
#define FLASCREEN_HEIGHT 200

/** FLA movie header structure */
typedef struct FLAHeaderStruct {
	/** FLA version */
	int8 version[6];
	/** Number of frames */
	int32 numOfFrames;
	/** Frames per second */
	int8 speed;
	/** Unknown var1 */
	int8 var1;
	/** Frame width */
	int16 xsize;
	/** Frame height */
	int16 ysize;
} FLAHeaderStruct;

/** FLA movie frame structure */
typedef struct FLAFrameDataStruct {
	/** Current frame size */
	int8 videoSize;
	/** Dummy variable */
	int8 dummy;
	/** Unknown frameVar0 */
	int32 frameVar0;
} FLAFrameDataStruct;

/** FLA movie sample structure */
typedef struct FLASampleStruct {
	/** Number os samples */
	int16 sampleNum;
	/** Sample frequency */
	int16 freq;
	/** Numbers of time to repeat */
	int16 repeat;
	/** Dummy variable */
	int8 dummy;
	/** Unknown x */
	uint8 x;
	/** Unknown y */
	uint8 y;
} FLASampleStruct;

/** FLA Frame Opcode types */
enum FlaFrameOpcode {
	kLoadPalette = 0,
	kFade = 1,
	kPlaySample = 2,
	kStopSample = 4,
	kDeltaFrame = 5,
	kKeyFrame = 7
};

class TwinEEngine;

class FlaMovies {
private:
	TwinEEngine *_engine;

	/** Auxiliar FLA fade out variable */
	int32 _fadeOut;
	/** Auxiliar FLA fade out variable to count frames between the fade */
	int32 fadeOutFrames;

	/** FLA movie sample auxiliar table */
	int32 flaSampleTable[100];
	/** Number of samples in FLA movie */
	int32 samplesInFla;
	/** Auxiliar work video buffer */
	uint8 *workVideoBufferCopy;
	/** FLA movie header data */
	FLAHeaderStruct flaHeaderData;
	/** FLA movie header data */
	FLAFrameDataStruct frameData;

	FileReader frFla;

	void drawKeyFrame(uint8 *ptr, int32 width, int32 height);
	void drawDeltaFrame(uint8 *ptr, int32 width);
	void scaleFla2x();
	void processFrame();

public:
	FlaMovies(TwinEEngine *engine);

	/** FLA movie file buffer */
	unsigned char flaBuffer[FLASCREEN_WIDTH * FLASCREEN_HEIGHT];

	/** Play FLA movies
	@param flaName FLA movie name */
	void playFlaMovie(const char *flaName);
};

} // namespace TwinE

#endif
