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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>
#include <SDL_mixer.h>

#include "common/debug.h"
#include "common/textconsole.h"
#include "hqrdepack.h"
#include "music.h"
#include "resources.h"
#include "sdlengine.h"
#include "twine.h"
#include "xmidi.h"

namespace TwinE {

/** MP3 music folder */
#define MUSIC_FOLDER "music"
/** LBA1 default number of tracks */
#define NUM_CD_TRACKS 10
/** Number of miliseconds to fade music */
#define FADE_MS 500

/** SDL CD variable interface */
#if TODO_SDL_CD
SDL_CD *cdrom;
/** CD drive letter */
const int8 *cdname;
#endif

/** SDL_Mixer track variable interface */
Mix_Music *current_track;

/** Auxiliar midi pointer to  */
uint8 *midiPtr;

/** Music volume
	@param current volume number */
void Music::musicVolume(int32 volume) {
	// div 2 because LBA use 255 range and SDL_mixer use 128 range
	Mix_VolumeMusic(volume / 2);
}

/** Fade music in
	@param loops number of*/
void Music::musicFadeIn(int32 loops, int32 ms) {
	Mix_FadeInMusic(current_track, loops, ms);
	musicVolume(_engine->cfgfile.MusicVolume);
}

/** Fade music out
	@param ms number of miliseconds to fade*/
void Music::musicFadeOut(int32 ms) {
	while (!Mix_FadeOutMusic(ms) && Mix_PlayingMusic()) {
		SDL_Delay(100);
	}
	Mix_HaltMusic();
	Mix_RewindMusic();
	musicVolume(_engine->cfgfile.MusicVolume);
}

/** Play CD music
	@param track track number to play */
void Music::playTrackMusicCd(int32 track) {
	if (!_engine->cfgfile.UseCD) {
		return;
	}
#if TODO_SDL_CD
	if (cdrom->numtracks == 10) {
		if (CD_INDRIVE(SDL_CDStatus(cdrom)))
			SDL_CDPlayTracks(cdrom, track, 0, 1, 0);
	}
#endif
}

/** Stop CD music */
void Music::stopTrackMusicCd() {
	if (!_engine->cfgfile.UseCD) {
		return;
	}

#if TODO_SDL_CD
	if (cdrom != NULL) {
		SDL_CDStop(cdrom);
	}
#endif
}

/** Generic play music, according with settings it plays CD or MP3 instead
	@param track track number to play */
void Music::playTrackMusic(int32 track) {
	if (!_engine->cfgfile.Sound) {
		return;
	}

	if (track == currentMusic)
		return;
	currentMusic = track;

	stopMusic();
	playTrackMusicCd(track);
}

/** Generic stop music according with settings */
void Music::stopTrackMusic() {
	if (!_engine->cfgfile.Sound) {
		return;
	}

	musicFadeOut(FADE_MS);
	stopTrackMusicCd();
}

/** Play MIDI music
	@param midiIdx music index under mini_mi_win.hqr*/
void Music::playMidiMusic(int32 midiIdx, int32 loop) {
	uint8 *dos_midi_ptr;
	int32 midiSize;
	char filename[256];
	SDL_RWops *rw;

	if (!_engine->cfgfile.Sound) {
		return;
	}

	if (midiIdx == currentMusic) {
		return;
	}

	stopMusic();
	currentMusic = midiIdx;

	if (_engine->cfgfile.MidiType == 0)
		sprintf(filename, "%s", HQR_MIDI_MI_DOS_FILE);
	else
		sprintf(filename, "%s", HQR_MIDI_MI_WIN_FILE);

	if (midiPtr) {
		musicFadeOut(FADE_MS / 2);
		stopMidiMusic();
	}

	midiSize = _engine->_hqrdepack->hqrGetallocEntry(&midiPtr, (int8 *)filename, midiIdx);

	if (_engine->cfgfile.Sound == 1 && _engine->cfgfile.MidiType == 0) {
		midiSize = convert_to_midi(midiPtr, midiSize, &dos_midi_ptr);
		free(midiPtr);
		midiPtr = dos_midi_ptr;
	}

	rw = SDL_RWFromMem(midiPtr, midiSize);

	current_track = Mix_LoadMUS_RW(rw, 0);

	musicFadeIn(1, FADE_MS);

	musicVolume(_engine->cfgfile.MusicVolume);

	if (Mix_PlayMusic(current_track, loop) == -1)
		warning("Error while playing music: %d \n", midiIdx);
}

/** Stop MIDI music */
void Music::stopMidiMusic() {
	if (!_engine->cfgfile.Sound) {
		return;
	}

	if (current_track != NULL) {
		Mix_FreeMusic(current_track);
		current_track = NULL;
		if (midiPtr != NULL)
			free(midiPtr);
	}
}

/** Initialize CD-Rom */
int Music::initCdrom() {
#if TODO_SDL_CD
	int32 numOfCDROM;
	int32 cdNum;

	if (!_engine->cfgfile.Sound) {
		return 0;
	}

	numOfCDROM = SDL_CDNumDrives();

	if (_engine->cfgfile.Debug)
		debug("Found %d CDROM devices\n", numOfCDROM);

	if (!numOfCDROM) {
		warning("No CDROM devices available\n");
		return 0;
	}

	for (cdNum = 0; cdNum < numOfCDROM; cdNum++) {
		cdname = SDL_CDName(cdNum);
		if (_engine->cfgfile.Debug)
			debug("Testing drive %s\n", cdname);
		cdrom = SDL_CDOpen(cdNum);
		if (!cdrom) {
			if (_engine->cfgfile.Debug)
				warning("Couldn't open CD drive: %s", SDL_GetError());
		} else {
			SDL_CDStatus(cdrom);
			if (cdrom->numtracks == NUM_CD_TRACKS) {
				debug("Assuming that it is LBA cd... %s", cdname);
				_engine->cdDir = "LBA";
				_engine->cfgfile.UseCD = 1;
				return 1;
			}
		}
		// not found the right CD
		_engine->cfgfile.UseCD = 0;
		SDL_CDClose(cdrom);
	}

	cdrom = NULL;

	warning("Can't find LBA CD!");
#endif
	return 0;
}

/** Stop MIDI and Track music */
void Music::stopMusic() {
	stopTrackMusic();
	stopMidiMusic();
}

} // namespace TwinE
