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

#ifndef TWINE_TWINE_H
#define TWINE_TWINE_H

#include "common/random.h"
#include "engines/engine.h"

#include "actor.h"

namespace TwinE {

/** Definition for European version */
#define EUROPE_VERSION 0
/** Definition for American version */
#define USA_VERSION 1
/** Definition for Modification version */
#define MODIFICATION_VERSION 2

/** Original screen width */
#define DEFAULT_SCREEN_WIDTH 640
/** Original screen height */
#define DEFAULT_SCREEN_HEIGHT 480
/** Scale screen to double size */
#define SCALE 1
/** Original screen width */
#define SCREEN_WIDTH DEFAULT_SCREEN_WIDTH *SCALE
/** Original screen height */
#define SCREEN_HEIGHT DEFAULT_SCREEN_HEIGHT *SCALE
/** Default frames per second */
#define DEFAULT_FRAMES_PER_SECOND 19

/** Number of colors used in the game */
#define NUMOFCOLORS 256

/** Configuration file structure

	Used in the engine to load/use certain parts of code according with
	this settings. Check \a lba.cfg file for valid values for each settings.\n
	All the settings with (*) means they are new and only exist in this engine. */
typedef struct ConfigFile {
	/** Language name */
	int8 Language[10];
	/** Language CD name */
	int8 LanguageCD[10];
	/** Language Identification according with Language setting. */
	int32 LanguageId;
	/** Language Identification according with Language setting. */
	int32 LanguageCDId;
	/** Enable/Disable game dialogues */
	int8 FlagDisplayTextStr[3];
	/** Enable/Disable game dialogues */
	int32 FlagDisplayText;
	/** Save voice files on hard disk */
	int8 FlagKeepVoiceStr[3];
	/** Save voice files on hard disk */
	int32 FlagKeepVoice;
	/** Type of music file to be used */
	int8 MidiType;
	/** Wave volume */
	int32 WaveVolume;
	/** Chacters voices volume */
	int32 VoiceVolume;
	/** Music volume */
	int32 MusicVolume;
	/** CD volume */
	int32 CDVolume;
	/** Line-In volume */
	int32 LineVolume;
	/** Main volume controller */
	int32 MasterVolume;
	/** *Game version */
	int32 Version;
	/** To allow fullscreen or window mode. */
	int32 FullScreen;
	/** If you want to use the LBA CD or not */
	int32 UseCD;
	/** Allow various sound types */
	int32 Sound;
	/** Allow various movie types */
	int32 Movie;
	/** Use cross fade effect while changing images, or be as the original */
	int32 CrossFade;
	/** Flag used to keep the game frames per second */
	int32 Fps;
	/** Flag to display game debug */
	int32 Debug;
	/** Use original autosaving system or save when you want */
	int32 UseAutoSaving;
	/** Shadow mode type */
	int32 ShadowMode;
	/** AutoAgressive mode type */
	int32 AutoAgressive;
	/** SceZoom mode type */
	int32 SceZoom;
	/** FillDetails mode type */
	int32 FillDetails;
	/** Flag to quit the game */
	int32 Quit;
	/** Flag to change interface style */
	int32 InterfaceStyle;
	/** Flag to toggle Wall Collision */
	int32 WallCollision;
} ConfigFile;

class Actor;
class Animations;
class Collision;
class Extra;
class GameState;
class Grid;
class Movements;
class HQRDepack;
class Interface;
class Menu;
class FlaMovies;
class MenuOptions;
class Music;
class Redraw;
class Renderer;
class Resources;
class Scene;
class Screens;

class TwinEEngine : public Engine {
public:
	TwinEEngine(OSystem *system, Common::Language language, uint32 flags);
	~TwinEEngine() override;

	Common::Error run() override;
	bool hasFeature(EngineFeature f) const override;

	Actor *_actor;
	Animations *_animations;
	Collision *_collision;
	Extra *_extra;
	GameState *_gameState;
	Grid *_grid;
	Movements *_movements;
	HQRDepack *_hqrdepack;
	Interface *_interface;
	Menu *_menu;
	FlaMovies *_flaMovies;
	MenuOptions *_menuOptions;
	Music *_music;
	Redraw *_redraw;
	Renderer *_renderer;
	Resources *_resources;
	Scene *_scene;
	Screens *_screens;

	/** Configuration file structure
	 * Contains all the data used in the engine to configurated the game in particulary ways. */
	ConfigFile cfgfile;

	/** CD Game directory */
	const char *cdDir;

	int32 isTimeFreezed = 0;
	int32 saveFreezedTime = 0;

	void initEngine();
	void initMCGA();
	void initSVGA();

	int8 *ITOA(int32 number);
	void initConfigurations();
	int getLanguageTypeIndex(int8 *language);
	int getConfigTypeIndex(int8 *lineBuffer);

	void allocVideoMemory();
	int getRandomNumber(uint max = 0x7FFF);
	int32 quitGame;
	int32 lbaTime;

	int16 leftMouse;
	int16 rightMouse;

	/** Work video buffer */
	uint8 *workVideoBuffer;
	/** Main game video buffer */
	uint8 *frontVideoBuffer;
	/** Auxiliar game video buffer */
	uint8 *frontVideoBufferbis;

	/** temporary screen table */
	int32 screenLookupTable[2000];

	ActorMoveStruct loopMovePtr; // mainLoopVar1

	int32 loopPressedKey;         // mainLoopVar5
	int32 previousLoopPressedKey; // mainLoopVar6
	int32 loopCurrentKey;         // mainLoopVar7
	int32 loopInventoryItem;      // mainLoopVar9

	int32 loopActorStep; // mainLoopVar17

	/** Disable screen recenter */
	int16 disableScreenRecenter;

	int32 zoomScreen;

	void freezeTime();
	void unfreezeTime();

	int32 gameEngineLoop();

	Common::RandomSource _rnd;
	Common::Language _gameLang;
	uint32 _gameFlags;
	int _startSlot;
};

} // namespace TwinE

#endif
