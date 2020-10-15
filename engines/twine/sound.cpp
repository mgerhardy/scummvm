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

#include <SDL_mixer.h>

#include "collision.h"
#include "flamovies.h"
#include "grid.h"
#include "hqrdepack.h"
#include "movements.h"
#include "resources.h"
#include "sound.h"
#include "text.h"
#include "twine.h"

namespace TwinE {

/** Samples chunk variable */
Mix_Chunk *sample;

/** Sample volume
	@param chan sample channel
	@param volume sample volume number */
void Sound::sampleVolume(int32 chan, int32 volume) {
	Mix_Volume(chan, volume / 2);
}

/** Play FLA movie samples
	@param index sample index under flasamp.hqr file
	@param frequency frequency used to play the sample
	@param repeat number of times to repeat the sample
	@param x unknown x variable
	@param y unknown y variable */
void Sound::playFlaSample(int32 index, int32 frequency, int32 repeat, int32 x, int32 y) {
	if (_engine->cfgfile.Sound) {
		int32 sampSize = 0;
		char sampfile[256];
		SDL_RWops *rw;
		uint8 *sampPtr;

		sprintf(sampfile, FLA_DIR "%s", HQR_FLASAMP_FILE);

		sampSize = _engine->_hqrdepack->hqrGetallocEntry(&sampPtr, sampfile, index);

		// Fix incorrect sample files first byte
		if (*sampPtr != 'C')
			*sampPtr = 'C';

		rw = SDL_RWFromMem(sampPtr, sampSize);
		sample = Mix_LoadWAV_RW(rw, 1);

		channelIdx = getFreeSampleChannelIndex();
		if (channelIdx != -1) {
			samplesPlaying[channelIdx] = index;
		}

		sampleVolume(channelIdx, _engine->cfgfile.WaveVolume);

		if (Mix_PlayChannel(channelIdx, sample, repeat - 1) == -1)
			error("Error while playing VOC: Sample %d \n", index);

		free(sampPtr);
	}
}

void Sound::setSamplePosition(int32 chan, int32 x, int32 y, int32 z) {
	int32 distance;
	distance = ABS(_engine->_movements->getDistance3D(_engine->_grid->newCameraX << 9, _engine->_grid->newCameraY << 8, _engine->_grid->newCameraZ << 9, x, y, z));
	distance = _engine->_collision->getAverageValue(0, distance, 10000, 255);
	if (distance > 255) { // don't play it if its to far away
		distance = 255;
	}

	Mix_SetDistance(chan, distance);
}

/** Play samples
	@param index sample index under flasamp.hqr file
	@param frequency frequency used to play the sample
	@param repeat number of times to repeat the sample
	@param x unknown x variable
	@param y unknown y variable
	@param z unknown z variable */
void Sound::playSample(int32 index, int32 frequency, int32 repeat, int32 x, int32 y, int32 z, int32 actorIdx) {
	if (_engine->cfgfile.Sound) {
		int32 sampSize = 0;
		SDL_RWops *rw;
		uint8 *sampPtr;

		sampSize = _engine->_hqrdepack->hqrGetallocEntry(&sampPtr, HQR_SAMPLES_FILE, index);

		// Fix incorrect sample files first byte
		if (*sampPtr != 'C')
			*sampPtr = 'C';

		rw = SDL_RWFromMem(sampPtr, sampSize);
		sample = Mix_LoadWAV_RW(rw, 1);

		channelIdx = getFreeSampleChannelIndex();

		// only play if we have a free channel, otherwise we won't be able to control the sample
		if (channelIdx != -1) {
			samplesPlaying[channelIdx] = index;
			sampleVolume(channelIdx, _engine->cfgfile.WaveVolume);

			if (actorIdx != -1) {
				setSamplePosition(channelIdx, x, y, z);

				// save the actor index for the channel so we can check the position
				samplesPlayingActors[channelIdx] = actorIdx;
			}

			if (Mix_PlayChannel(channelIdx, sample, repeat - 1) == -1)
				error("Error while playing VOC: Sample %d \n", index);
		}

		free(sampPtr);
	}
}

/** Resume samples */
void Sound::resumeSamples() {
	if (_engine->cfgfile.Sound) {
		Mix_Resume(-1);
		/*if (cfgfile.Debug)
			printf("Resume VOC samples\n");*/
	}
}

/** Pause samples */
void Sound::pauseSamples() {
	if (_engine->cfgfile.Sound) {
		Mix_HaltChannel(-1);
		/*if (cfgfile.Debug)
			printf("Pause VOC samples\n");*/
	}
}

/** Stop samples */
void Sound::stopSamples() {
	if (_engine->cfgfile.Sound) {
		memset(samplesPlaying, -1, sizeof(int32) * NUM_CHANNELS);
		Mix_HaltChannel(-1);
		//clean up
		Mix_FreeChunk(sample);
		sample = NULL; //make sure we free it
		               /*if (cfgfile.Debug)
			printf("Stop VOC samples\n");*/
	}
}

int32 Sound::getActorChannel(int32 index) {
	int32 c = 0;
	for (c = 0; c < NUM_CHANNELS; c++) {
		if (samplesPlayingActors[c] == index) {
			return c;
		}
	}
	return -1;
}

int32 Sound::getSampleChannel(int32 index) {
	int32 c = 0;
	for (c = 0; c < NUM_CHANNELS; c++) {
		if (samplesPlaying[c] == index) {
			return c;
		}
	}
	return -1;
}

void Sound::removeSampleChannel(int32 c) {
	samplesPlaying[c] = -1;
	samplesPlayingActors[c] = -1;
}

/** Stop samples */
void Sound::stopSample(int32 index) {
	if (_engine->cfgfile.Sound) {
		int32 stopChannel = getSampleChannel(index);
		if (stopChannel != -1) {
			removeSampleChannel(stopChannel);
			Mix_HaltChannel(stopChannel);
			//clean up
			Mix_FreeChunk(sample);
			sample = NULL; //make sure we free it
			               /*if (cfgfile.Debug)
				printf("Stop VOC samples\n");*/
		}
	}
}

int32 Sound::isChannelPlaying(int32 chan) {
	if (chan != -1) {
		if (Mix_Playing(chan)) {
			return 1;
		} else {
			removeSampleChannel(chan);
		}
	}
	return 0;
}

int32 Sound::isSamplePlaying(int32 index) {
	const int32 chan = getSampleChannel(index);
	return isChannelPlaying(chan);
}

int32 Sound::getFreeSampleChannelIndex() {
	int i = 0;
	for (i = 0; i < NUM_CHANNELS; i++) {
		if (samplesPlaying[i] == -1) {
			return i;
		}
	}
	//FIXME if didn't find any, lets free what is not in use
	for (i = 0; i < NUM_CHANNELS; i++) {
		if (samplesPlaying[i] != -1) {
			isChannelPlaying(i);
		}
	}
	return -1;
}

void Sound::playVoxSample(int32 index) {
	if (_engine->cfgfile.Sound) {
		int32 sampSize = 0;
		SDL_RWops *rw;
		uint8 *sampPtr = 0;

		sampSize = _engine->_hqrdepack->hqrGetallocVoxEntry(&sampPtr, _engine->_text->currentVoxBankFile, index, _engine->_text->voxHiddenIndex);

		// Fix incorrect sample files first byte
		if (*sampPtr != 'C') {
			_engine->_text->hasHiddenVox = *sampPtr;
			_engine->_text->voxHiddenIndex++;
			*sampPtr = 'C';
		}

		rw = SDL_RWFromMem(sampPtr, sampSize);
		sample = Mix_LoadWAV_RW(rw, 1);

		channelIdx = getFreeSampleChannelIndex();

		// only play if we have a free channel, otherwise we won't be able to control the sample
		if (channelIdx != -1) {
			samplesPlaying[channelIdx] = index;

			sampleVolume(channelIdx, _engine->cfgfile.VoiceVolume - 1);

			if (Mix_PlayChannel(channelIdx, sample, 0) == -1)
				error("Error while playing VOC: Sample %d \n", index);
		}

		free(sampPtr);
	}
}

} // namespace TwinE
