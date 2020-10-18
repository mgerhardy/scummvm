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

#include "twine/sound.h"
#include "common/system.h"
#include "twine/collision.h"
#include "twine/flamovies.h"
#include "twine/grid.h"
#include "twine/hqrdepack.h"
#include "twine/movements.h"
#include "twine/resources.h"
#include "twine/text.h"
#include "twine/twine.h"

namespace TwinE {

void Sound::sampleVolume(int32 chan, int32 volume) {
	_engine->_system->getMixer()->setChannelVolume(samplesPlaying[chan], volume / 2);
}

void Sound::playFlaSample(int32 index, int32 frequency, int32 repeat, int32 x, int32 y) {
	if (!_engine->cfgfile.Sound) {
		return;
	}

	char sampfile[256];

	sprintf(sampfile, FLA_DIR "%s", HQR_FLASAMP_FILE);

	uint8 *sampPtr;
	int32 sampSize = _engine->_hqrdepack->hqrGetallocEntry(&sampPtr, sampfile, index);

	// Fix incorrect sample files first byte
	if (*sampPtr != 'C') {
		*sampPtr = 'C';
	}

	channelIdx = getFreeSampleChannelIndex();
	if (channelIdx != -1) {
		samplesPlaying[channelIdx] = index;
	}

	sampleVolume(channelIdx, _engine->cfgfile.WaveVolume);

#if 0 // TODO
	SDL_RWops *rw = SDL_RWFromMem(sampPtr, sampSize);
	sample = Mix_LoadWAV_RW(rw, 1);
	if (Mix_PlayChannel(channelIdx, sample, repeat - 1) == -1)
		error("Error while playing VOC: Sample %d", index);
#endif

	free(sampPtr);
}

void Sound::setSamplePosition(int32 chan, int32 x, int32 y, int32 z) {
	int32 distance;
	distance = ABS(_engine->_movements->getDistance3D(_engine->_grid->newCameraX << 9, _engine->_grid->newCameraY << 8, _engine->_grid->newCameraZ << 9, x, y, z));
	distance = _engine->_collision->getAverageValue(0, distance, 10000, 255);
	if (distance > 255) { // don't play it if its to far away
		distance = 255;
	}

#if 0 // TODO
	Mix_SetDistance(chan, distance);
#endif
}

void Sound::playSample(int32 index, int32 frequency, int32 repeat, int32 x, int32 y, int32 z, int32 actorIdx) {
	if (!_engine->cfgfile.Sound) {
		return;
	}
	uint8 *sampPtr;
	int32 sampSize = _engine->_hqrdepack->hqrGetallocEntry(&sampPtr, HQR_SAMPLES_FILE, index);

	// Fix incorrect sample files first byte
	if (*sampPtr != 'C') {
		*sampPtr = 'C';
	}

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

#if 0 // TODO
		SDL_RWops *rw = SDL_RWFromMem(sampPtr, sampSize);
		sample = Mix_LoadWAV_RW(rw, 1);
		if (Mix_PlayChannel(channelIdx, sample, repeat - 1) == -1)
			error("Error while playing VOC: Sample %d \n", index);
#endif
	}

	free(sampPtr);
}

void Sound::resumeSamples() {
	if (!_engine->cfgfile.Sound) {
		return;
	}
	_engine->_system->getMixer()->pauseAll(false);

}

void Sound::pauseSamples() {
	if (!_engine->cfgfile.Sound) {
		return;
	}
	_engine->_system->getMixer()->pauseAll(true);
}

void Sound::stopSamples() {
	if (!_engine->cfgfile.Sound) {
		return;
	}
	memset(samplesPlaying, -1, sizeof(int32) * NUM_CHANNELS);
#if 0 // TODO
	Mix_HaltChannel(-1);
	//clean up
	Mix_FreeChunk(sample);
	sample = NULL; //make sure we free it
#endif
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

void Sound::stopSample(int32 index) {
	if (!_engine->cfgfile.Sound) {
		return;
	}
	int32 stopChannel = getSampleChannel(index);
	if (stopChannel != -1) {
		removeSampleChannel(stopChannel);
#if 0 // TODO
		Mix_HaltChannel(stopChannel);
		//clean up
		Mix_FreeChunk(sample);
		sample = NULL; //make sure we free it
#endif
	}
}

int32 Sound::isChannelPlaying(int32 chan) {
	if (chan != -1) {
#if 0 // TODO
		if (Mix_Playing(chan)) {
			return 1;
		}
#endif
		removeSampleChannel(chan);
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
	if (!_engine->cfgfile.Sound) {
		return;
	}
	int32 sampSize = 0;
	uint8 *sampPtr = 0;

	sampSize = _engine->_hqrdepack->hqrGetallocVoxEntry(&sampPtr, _engine->_text->currentVoxBankFile, index, _engine->_text->voxHiddenIndex);

	// Fix incorrect sample files first byte
	if (*sampPtr != 'C') {
		_engine->_text->hasHiddenVox = *sampPtr;
		_engine->_text->voxHiddenIndex++;
		*sampPtr = 'C';
	}

	channelIdx = getFreeSampleChannelIndex();

	// only play if we have a free channel, otherwise we won't be able to control the sample
	if (channelIdx != -1) {
		samplesPlaying[channelIdx] = index;

		sampleVolume(channelIdx, _engine->cfgfile.VoiceVolume - 1);

#if 0 // TODO
		SDL_RWops *rw = SDL_RWFromMem(sampPtr, sampSize);
		sample = Mix_LoadWAV_RW(rw, 1);
		if (Mix_PlayChannel(channelIdx, sample, 0) == -1)
			error("Error while playing VOC: Sample %d \n", index);
#endif
	}

	free(sampPtr);
}

} // namespace TwinE
