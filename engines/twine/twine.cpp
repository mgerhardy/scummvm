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

#include "twine/twine.h"
#include "common/debug.h"
#include "common/error.h"
#include "common/system.h"
#include "common/textconsole.h"
#include "gui/debugger.h"
#include "twine/actor.h"
#include "twine/animations.h"
#include "twine/collision.h"
#include "twine/debug.h"
#include "twine/debug_grid.h"
#include "twine/debug_scene.h"
#include "twine/extra.h"
#include "twine/filereader.h"
#include "twine/flamovies.h"
#include "twine/gamestate.h"
#include "twine/grid.h"
#include "twine/holomap.h"
#include "twine/hqrdepack.h"
#include "twine/interface.h"
#include "twine/keyboard.h"
#include "twine/menu.h"
#include "twine/menuoptions.h"
#include "twine/movements.h"
#include "twine/music.h"
#include "twine/redraw.h"
#include "twine/renderer.h"
#include "twine/resources.h"
#include "twine/scene.h"
#include "twine/screens.h"
#include "twine/script_life.h"
#include "twine/script_move.h"
#include "twine/sdlengine.h"
#include "twine/sound.h"
#include "twine/text.h"

namespace TwinE {

enum InventoryItems {
	kiHolomap = 0,
	kiMagicBall = 1,
	kiUseSabre = 2,
	kiBookOfBu = 5,
	kiProtoPack = 12,
	kiPinguin = 14,
	kiBonusList = 26,
	kiCloverLeaf = 27
};

/** Engine current version */
const char *ENGINE_VERSION = "0.2.0";

/** Engine configuration filename */
const char *CONFIG_FILENAME = "lba.cfg";

/** Engine install setup filename

	This file contains informations about the game version.
	This is only used for original games. For mod games project you can
	used \a lba.cfg file \b Version tag. If this tag is set for original game
	it will be used instead of \a setup.lst file. */
const char *SETUP_FILENAME = "setup.lst";

/** Configuration types at \a lba.cfg file

	Fill this with all needed configurations at \a lba.cfg file.
	This engine version allows new type of configurations.
	Check new config lines at \a lba.cfg file after the first game execution */
static char CFGList[][22] = {
    "Language:",
    "LanguageCD:",
    "FlagDisplayText:",
    "FlagKeepVoice:",
    "SvgaDriver:",
    "MidiDriver:",
    "MidiExec:",
    "MidiBase:",
    "MidiType:",
    "MidiIRQ:",
    "MidiDMA:", // 10
    "WaveDriver:",
    "WaveExec:",
    "WaveBase:",
    "WaveIRQ:",
    "WaveDMA:",
    "WaveRate:",
    "MixerDriver:",
    "MixerBase:",
    "WaveVolume:",
    "VoiceVolume:", // 20
    "MusicVolume:",
    "CDVolume:",
    "LineVolume:",
    "MasterVolume:",
    "Version:",
    "FullScreen:",
    "UseCD:",
    "Sound:",
    "Movie:",
    "CrossFade:", // 30
    "Fps:",
    "Debug:",
    "UseAutoSaving:",
    "CombatAuto:",
    "Shadow:",
    "SceZoom:",
    "FillDetails:",
    "InterfaceStyle",
    "WallCollision" // 39
};

static char LanguageTypes[][10] = {
    "English",
    "Francais",
    "Deutsch",
    "Espanol",
    "Italiano",
    "Portugues"};

void TwinEEngine::allocVideoMemory() {
	const size_t videoBufferSize = (SCREEN_WIDTH * SCREEN_HEIGHT) * sizeof(uint8);
	workVideoBuffer = (uint8 *)malloc(videoBufferSize);
	frontVideoBuffer = frontVideoBufferbis = (uint8 *)malloc(videoBufferSize);
	initScreenBuffer(frontVideoBuffer, SCREEN_WIDTH, SCREEN_HEIGHT);

	int32 j = 0;
	int32 k = 0;
	for (int32 i = SCREEN_HEIGHT; i > 0; i--) {
		screenLookupTable[j] = k;
		j++;
		k += SCREEN_WIDTH;
	}

	// initVideoVar1 = -1;
}

int TwinEEngine::getConfigTypeIndex(int8 *lineBuffer) {
	int32 i;
	char buffer[256];
	char *ptr;

	strcpy(buffer, (char *)lineBuffer);

	ptr = strchr(buffer, ' ');

	if (ptr) {
		*ptr = 0;
	}

	const int32 length = sizeof(CFGList) / 22;
	for (i = 0; i < length; i++) {
		if (strlen(CFGList[i])) {
			if (!strcmp(CFGList[i], buffer)) {
				return i;
			}
		}
	}

	return -1;
}

int TwinEEngine::getLanguageTypeIndex(int8 *language) {
	int32 i;
	char buffer[256];
	char *ptr;

	strcpy(buffer, (char *)language);

	ptr = strchr(buffer, ' ');

	if (ptr) {
		*ptr = 0;
	}

	const int32 length = sizeof(LanguageTypes) / 10;
	for (i = 0; i < length; i++) {
		if (strlen(LanguageTypes[i])) {
			if (!strcmp(LanguageTypes[i], buffer)) {
				return i;
			}
		}
	}

	return 0; // English
}

// TODO: use ConfMan
void TwinEEngine::initConfigurations() {
	FileReader fr;
	char buffer[256], tmp[16];
	int32 cfgtype = -1;

	if (fropen2(&fr, CONFIG_FILENAME, "rb") != 0) {
		error("Error: Can't find config file %s\n", CONFIG_FILENAME);
	}

	frfeed(&fr);

	// make sure it quit when it reaches the end of file
	while (frread(&fr, buffer, 256)) {
		*strchr((char *)buffer, 0x0D0A) = 0;
		cfgtype = getConfigTypeIndex((int8 *)buffer);
		if (cfgtype != -1) {
			switch (cfgtype) {
			case 0:
				sscanf((const char *)buffer, "Language: %s", cfgfile.Language);
				cfgfile.LanguageId = getLanguageTypeIndex(cfgfile.Language);
				break;
			case 1:
				sscanf((const char *)buffer, "LanguageCD: %s", cfgfile.LanguageCD);
				cfgfile.LanguageCDId = getLanguageTypeIndex(cfgfile.Language) + 1;
				break;
			case 2:
				sscanf((const char *)buffer, "FlagDisplayText: %s", cfgfile.FlagDisplayTextStr);
				if (!strcmp((char *)cfgfile.FlagDisplayTextStr, "ON")) {
					cfgfile.FlagDisplayText = 1;
				} else {
					cfgfile.FlagDisplayText = 0;
				}
				break;
			case 3:
				sscanf((const char *)buffer, "FlagKeepVoice: %s", cfgfile.FlagKeepVoiceStr);
				break;
			case 8:
				sscanf((const char *)buffer, "MidiType: %s", tmp);
				if (strcmp((char *)tmp, "auto") == 0) {
#if 0 // TODO: mgerhardy - scummvm port to filesystem
					fd_test = fcaseopen(HQR_MIDI_MI_WIN_FILE, "rb");
					if (fd_test) {
						fclose(fd_test);
						cfgfile.MidiType = 1;
					} else
#endif
					cfgfile.MidiType = 0;
				} else if (strcmp((char *)tmp, "midi") == 0)
					cfgfile.MidiType = 1;
				else
					cfgfile.MidiType = 0;
				break;
			case 19:
				sscanf((const char *)buffer, "WaveVolume: %d", &cfgfile.WaveVolume);
				cfgfile.VoiceVolume = cfgfile.WaveVolume;
				break;
			case 20:
				sscanf((const char *)buffer, "VoiceVolume: %d", &cfgfile.VoiceVolume);
				break;
			case 21:
				sscanf((const char *)buffer, "MusicVolume: %d", &cfgfile.MusicVolume);
				break;
			case 22:
				sscanf((const char *)buffer, "CDVolume: %d", &cfgfile.CDVolume);
				break;
			case 23:
				sscanf((const char *)buffer, "LineVolume: %d", &cfgfile.LineVolume);
				break;
			case 24:
				sscanf((const char *)buffer, "MasterVolume: %d", &cfgfile.MasterVolume);
				break;
			case 25:
				sscanf((const char *)buffer, "Version: %d", &cfgfile.Version);
				break;
			case 26:
				sscanf((const char *)buffer, "FullScreen: %d", &cfgfile.FullScreen);
				break;
			case 27:
				sscanf((const char *)buffer, "UseCD: %d", &cfgfile.UseCD);
				break;
			case 28:
				sscanf((const char *)buffer, "Sound: %d", &cfgfile.Sound);
				break;
			case 29:
				sscanf((const char *)buffer, "Movie: %d", &cfgfile.Movie);
				break;
			case 30:
				sscanf((const char *)buffer, "CrossFade: %d", &cfgfile.CrossFade);
				break;
			case 31:
				sscanf((const char *)buffer, "Fps: %d", &cfgfile.Fps);
				break;
			case 32:
				sscanf((const char *)buffer, "Debug: %d", &cfgfile.Debug);
				break;
			case 33:
				sscanf((const char *)buffer, "UseAutoSaving: %d", &cfgfile.UseAutoSaving);
				break;
			case 34:
				sscanf((const char *)buffer, "CombatAuto: %d", &cfgfile.AutoAgressive);
				break;
			case 35:
				sscanf((const char *)buffer, "Shadow: %d", &cfgfile.ShadowMode);
				break;
			case 36:
				sscanf((const char *)buffer, "SceZoom: %d", &cfgfile.SceZoom);
				break;
			case 37:
				sscanf((const char *)buffer, "FillDetails: %d", &cfgfile.FillDetails);
				break;
			case 38:
				sscanf((const char *)buffer, "InterfaceStyle: %d", &cfgfile.InterfaceStyle);
				break;
			case 39:
				sscanf((const char *)buffer, "WallCollision: %d", &cfgfile.WallCollision);
				break;
			}
		}
	}

	if (!cfgfile.Fps)
		cfgfile.Fps = DEFAULT_FRAMES_PER_SECOND;
}

void TwinEEngine::initEngine() {
	// getting configuration file
	initConfigurations();

	// Show engine information
	debug("TwinEngine v%s\n\n", ENGINE_VERSION);
	debug("(c)2002 The TwinEngine team. Refer to AUTHORS file for further details.\n");
	debug("Released under the terms of the GNU GPL license version 2 (or, at your opinion, any later). See COPYING file.\n\n");
	debug("The original Little Big Adventure game is:\n\t(c)1994 by Adeline Software International, All Rights Reserved.\n\n");

	allocVideoMemory();
	_screens->clearScreen();

	// Toggle fullscreen if Fullscreen flag is set
	toggleFullscreen();

	// Check if LBA CD-Rom is on drive
	_music->initCdrom();

	// Display company logo
	_screens->adelineLogo();

	// verify game version screens
	if (cfgfile.Version == EUROPE_VERSION) {
		// Little Big Adventure screen
		_screens->loadImageDelay(RESSHQR_LBAIMG, 3);
		// Electronic Arts Logo
		_screens->loadImageDelay(RESSHQR_EAIMG, 2);
	} else if (cfgfile.Version == USA_VERSION) {
		// Relentless screen
		_screens->loadImageDelay(RESSHQR_RELLENTIMG, 3);
		// Electronic Arts Logo
		_screens->loadImageDelay(RESSHQR_EAIMG, 2);
	} else if (cfgfile.Version == MODIFICATION_VERSION) {
		// Modification screen
		_screens->loadImageDelay(RESSHQR_RELLENTIMG, 2);
	}

	_flaMovies->playFlaMovie(FLA_DRAGON3);

	_screens->loadMenuImage(1);

	_menu->mainMenu();
}

void TwinEEngine::initMCGA() {
	_redraw->drawInGameTransBox = 1;
}

void TwinEEngine::initSVGA() {
	_redraw->drawInGameTransBox = 0;
}

void TwinEEngine::initAll() {
	_grid->blockBuffer = (uint8 *)malloc(64 * 64 * 25 * 2 * sizeof(uint8));
	_animations->animBuffer1 = _animations->animBuffer2 = (uint8 *)malloc(5000 * sizeof(uint8));
	memset(_sound->samplesPlaying, -1, sizeof(int32) * NUM_CHANNELS);
	memset(_menu->itemAngle, 256, sizeof(_menu->itemAngle)); // reset inventory items angles

	_redraw->bubbleSpriteIndex = SPRITEHQR_DIAG_BUBBLE_LEFT;
	_redraw->bubbleActor = -1;
	_text->showDialogueBubble = 1;

	_text->currentTextBank = -1;
	_menu->currMenuTextIndex = -1;
	_menu->currMenuTextBank = -1;
	_actor->autoAgressive = 1;

	_scene->sceneHero = &_scene->sceneActors[0];

	_redraw->renderLeft = 0;
	_redraw->renderTop = 0;
	_redraw->renderRight = SCREEN_TEXTLIMIT_RIGHT;
	_redraw->renderBottom = SCREEN_TEXTLIMIT_BOTTOM;
	// Set clip to fullscreen by default, allows main menu to render properly after load
	_interface->resetClip();

	rightMouse = 0;
	leftMouse = 0;

	_resources->initResources();

	initSVGA();
}

// AUX FUNC

int8 *TwinEEngine::ITOA(int32 number) {
	int32 numDigits = 1;
	char *text;

	if (number >= 10 && number <= 99) {
		numDigits = 2;
	} else if (number >= 100 && number <= 999) {
		numDigits = 3;
	}

	text = (char *)malloc(sizeof(char) * (numDigits + 1));
	sprintf(text, "%d", number);
	return (int8 *)text;
}

TwinEEngine::TwinEEngine(OSystem *system, Common::Language language, uint32 flags)
    : Engine(system), _gameLang(language), _gameFlags(flags), _rnd("twine") {
	setDebugger(new GUI::Debugger());
	_actor = new Actor(this);
	_animations = new Animations(this);
	_collision = new Collision(this);
	_extra = new Extra(this);
	_gameState = new GameState(this);
	_grid = new Grid(this);
	_movements = new Movements(this);
	_hqrdepack = new HQRDepack(this);
	_interface = new Interface(this);
	_menu = new Menu(this);
	_flaMovies = new FlaMovies(this);
	_menuOptions = new MenuOptions(this);
	_music = new Music(this);
	_redraw = new Redraw(this);
	_renderer = new Renderer(this);
	_resources = new Resources(this);
	_scene = new Scene(this);
	_screens = new Screens(this);
	_scriptLife = new ScriptLife(this);
	_scriptMove = new ScriptMove(this);
	_holomap = new Holomap(this);
	_sound = new Sound(this);
	_text = new Text(this);
	_debugGrid = new DebugGrid(this);
	_debug = new Debug(this);
	_debugScene = new DebugScene(this);
}

TwinEEngine::~TwinEEngine() {
	delete _actor;
	delete _animations;
	delete _collision;
	delete _extra;
	delete _gameState;
	delete _grid;
	delete _movements;
	delete _hqrdepack;
	delete _interface;
	delete _menu;
	delete _flaMovies;
	delete _menu;
	delete _music;
	delete _redraw;
	delete _renderer;
	delete _resources;
	delete _scene;
	delete _screens;
	delete _scriptLife;
	delete _scriptMove;
	delete _holomap;
	delete _sound;
	delete _text;
	delete _debugGrid;
	delete _debug;
	delete _debugScene;
}

bool TwinEEngine::hasFeature(EngineFeature f) const {
	return false;
}

Common::Error TwinEEngine::run() {
	syncSoundSettings();
	initAll();
	initEngine();
	_music->stopTrackMusic();
	_music->stopMidiMusic();
	return Common::kNoError;
}

int TwinEEngine::getRandomNumber(uint max) {
	return _rnd.getRandomNumber(max);
}

void TwinEEngine::freezeTime() {
	if (!isTimeFreezed)
		saveFreezedTime = lbaTime;
	isTimeFreezed++;
}

void TwinEEngine::unfreezeTime() {
	--isTimeFreezed;
	if (isTimeFreezed == 0)
		lbaTime = saveFreezedTime;
}

void TwinEEngine::processActorSamplePosition(int32 actorIdx) {
	int32 channelIdx;
	ActorStruct *actor = &_scene->sceneActors[actorIdx];
	channelIdx = _sound->getActorChannel(actorIdx);
	_sound->setSamplePosition(channelIdx, actor->x, actor->y, actor->z);
}

int32 TwinEEngine::runGameEngine() { // mainLoopInteration
	readKeys();

	if (_scene->needChangeScene > -1) {
		_scene->changeScene();
	}

	previousLoopPressedKey = loopPressedKey;
	_keyboard.key = _keyboard.pressedKey;
	loopPressedKey = _keyboard.skippedKey;
	loopCurrentKey = _keyboard.skipIntro;

	_debug->processDebug(loopCurrentKey);

	if (_menuOptions->canShowCredits != 0) {
		// TODO: if current music playing != 8, than play_track(8);
		if (_keyboard.skipIntro != 0) {
			return 0;
		}
		if (_keyboard.pressedKey != 0) {
			return 0;
		}
		if (_keyboard.skippedKey != 0) {
			return 0;
		}
	} else {
		// Process give up menu - Press ESC
		if (_keyboard.skipIntro == 1 && _scene->sceneHero->life > 0 && _scene->sceneHero->entity != -1 && !_scene->sceneHero->staticFlags.bIsHidden) {
			freezeTime();
			if (_menu->giveupMenu()) {
				unfreezeTime();
				_redraw->redrawEngineActions(1);
				freezeTime();
				_gameState->saveGame(); // auto save game
				quitGame = 0;
				cfgfile.Quit = 0;
				unfreezeTime();
				return 0;
			}
			unfreezeTime();
			_redraw->redrawEngineActions(1);
		}

		// Process options menu - Press F6
		if (loopCurrentKey == 0x40) {
			int tmpLangCD = cfgfile.LanguageCDId;
			freezeTime();
			_sound->pauseSamples();
			_menu->OptionsMenuSettings[5] = 15;
			cfgfile.LanguageCDId = 0;
			_text->initTextBank(0);
			_menu->optionsMenu();
			cfgfile.LanguageCDId = tmpLangCD;
			_text->initTextBank(_text->currentTextBank + 3);
			//TODO: play music
			_sound->resumeSamples();
			unfreezeTime();
			_redraw->redrawEngineActions(1);
		}

		// inventory menu
		loopInventoryItem = -1;
		if (loopCurrentKey == 0x36 && _scene->sceneHero->entity != -1 && _scene->sceneHero->controlMode == kManual) {
			freezeTime();
			_menu->processInventoryMenu();

			switch (loopInventoryItem) {
			case kiHolomap:
				warning("Use Inventory [kiHolomap] not implemented!\n");
				break;
			case kiMagicBall:
				if (_gameState->usingSabre == 1) {
					_actor->initModelActor(0, 0);
				}
				_gameState->usingSabre = 0;
				break;
			case kiUseSabre:
				if (_scene->sceneHero->body != GAMEFLAG_HAS_SABRE) {
					if (_actor->heroBehaviour == kProtoPack) {
						_actor->setBehaviour(kNormal);
					}
					_actor->initModelActor(GAMEFLAG_HAS_SABRE, 0);
					_animations->initAnim(24, 1, 0, 0);

					_gameState->usingSabre = 1;
				}
				break;
			case kiBookOfBu: {
				int32 tmpFlagDisplayText;

				_screens->fadeToBlack(_screens->paletteRGBA);
				_screens->loadImage(RESSHQR_INTROSCREEN1IMG, 1);
				_text->initTextBank(2);
				_text->newGameVar4 = 0;
				_text->textClipFull();
				_text->setFontCrossColor(15);
				tmpFlagDisplayText = cfgfile.FlagDisplayText;
				cfgfile.FlagDisplayText = 1;
				_text->drawTextFullscreen(161);
				cfgfile.FlagDisplayText = tmpFlagDisplayText;
				_text->textClipSmall();
				_text->newGameVar4 = 1;
				_text->initTextBank(_text->currentTextBank + 3);
				_screens->fadeToBlack(_screens->paletteRGBACustom);
				_screens->clearScreen();
				flip();
				setPalette(_screens->paletteRGBA);
				_screens->lockPalette = 1;
			} break;
			case kiProtoPack:
				if (_gameState->gameFlags[GAMEFLAG_BOOKOFBU]) {
					_scene->sceneHero->body = 0;
				} else {
					_scene->sceneHero->body = 1;
				}

				if (_actor->heroBehaviour == kProtoPack) {
					_actor->setBehaviour(kNormal);
				} else {
					_actor->setBehaviour(kProtoPack);
				}
				break;
			case kiPinguin: {
				ActorStruct *pinguin = &_scene->sceneActors[_scene->mecaPinguinIdx];

				pinguin->x = _renderer->destX + _scene->sceneHero->x;
				pinguin->y = _scene->sceneHero->y;
				pinguin->z = _renderer->destZ + _scene->sceneHero->z;
				pinguin->angle = _scene->sceneHero->angle;

				_movements->rotateActor(0, 800, pinguin->angle);

				if (!_collision->checkCollisionWithActors(_scene->mecaPinguinIdx)) {
					pinguin->life = 50;
					pinguin->body = -1;
					_actor->initModelActor(0, _scene->mecaPinguinIdx);
					pinguin->dynamicFlags.bIsDead = 0; // &= 0xDF
					pinguin->brickShape = 0;
					_movements->moveActor(pinguin->angle, pinguin->angle, pinguin->speed, &pinguin->move);
					_gameState->gameFlags[GAMEFLAG_MECA_PINGUIN] = 0; // byte_50D89 = 0;
					pinguin->info0 = lbaTime + 1500;
				}
			} break;
			case kiBonusList: {
				int32 tmpLanguageCDIdx;
				tmpLanguageCDIdx = cfgfile.LanguageCDId;
				unfreezeTime();
				_redraw->redrawEngineActions(1);
				freezeTime();
				cfgfile.LanguageCDId = 0;
				_text->initTextBank(2);
				_text->textClipFull();
				_text->setFontCrossColor(15);
				_text->drawTextFullscreen(162);
				_text->textClipSmall();
				cfgfile.LanguageCDId = tmpLanguageCDIdx;
				_text->initTextBank(_text->currentTextBank + 3);
			} break;
			case kiCloverLeaf:
				if (_scene->sceneHero->life < 50) {
					if (_gameState->inventoryNumLeafs > 0) {
						_scene->sceneHero->life = 50;
						_gameState->inventoryMagicPoints = _gameState->magicLevelIdx * 20;
						_gameState->inventoryNumLeafs--;
						_redraw->addOverlay(koInventoryItem, 27, 0, 0, 0, koNormal, 3);
					}
				}
				break;
			}

			unfreezeTime();
			_redraw->redrawEngineActions(1);
		}

		// Process behaviour menu - Press CTRL and F1..F4 Keys
		if ((loopCurrentKey == 0x1D || loopCurrentKey == 0x3B || loopCurrentKey == 0x3C || loopCurrentKey == 0x3D || loopCurrentKey == 0x3E) && _scene->sceneHero->entity != -1 && _scene->sceneHero->controlMode == kManual) {
			if (loopCurrentKey != 0x1D) {
				_actor->heroBehaviour = loopCurrentKey - 0x3B;
			}
			freezeTime();
			_menu->processBehaviourMenu();
			unfreezeTime();
			_redraw->redrawEngineActions(1);
		}

		// use Proto-Pack
		if (loopCurrentKey == 0x24 && _gameState->gameFlags[GAMEFLAG_PROTOPACK] == 1) {
			if (_gameState->gameFlags[GAMEFLAG_BOOKOFBU]) {
				_scene->sceneHero->body = 0;
			} else {
				_scene->sceneHero->body = 1;
			}

			if (_actor->heroBehaviour == kProtoPack) {
				_actor->setBehaviour(kNormal);
			} else {
				_actor->setBehaviour(kProtoPack);
			}
		}

		// Press Enter to Recenter Screen
		if ((loopPressedKey & 2) && !disableScreenRecenter) {
			_grid->newCameraX = _scene->sceneActors[_scene->currentlyFollowedActor].x >> 9;
			_grid->newCameraY = _scene->sceneActors[_scene->currentlyFollowedActor].y >> 8;
			_grid->newCameraZ = _scene->sceneActors[_scene->currentlyFollowedActor].z >> 9;
			_redraw->reqBgRedraw = 1;
		}

		// TODO: draw holomap

		// Process Pause - Press P
		if (loopCurrentKey == 0x19) {
			freezeTime();
			_text->setFontColor(15);
			_text->drawText(5, 446, (const int8 *)"Pause"); // no key for pause in Text Bank
			copyBlockPhys(5, 446, 100, 479);
			do {
				readKeys();
				if (shouldQuit()) {
					break;
				}
				g_system->delayMillis(10);
			} while (_keyboard.skipIntro != 0x19 && !_keyboard.pressedKey);
			unfreezeTime();
			_redraw->redrawEngineActions(1);
		}
	}

	loopActorStep = _movements->getRealValue(&loopMovePtr);
	if (!loopActorStep) {
		loopActorStep = 1;
	}

	_movements->setActorAngle(0, -256, 5, &loopMovePtr);
	disableScreenRecenter = 0;

	_scene->processEnvironmentSound();

	// Reset HitBy state
	for (a = 0; a < _scene->sceneNumActors; a++) {
		_scene->sceneActors[a].hitBy = -1;
	}

	_extra->processExtras();

	for (int32 a = 0; a < _scene->sceneNumActors; a++) {
		ActorStruct *actor = &_scene->sceneActors[a];

		if (!actor->dynamicFlags.bIsDead) {
			if (actor->life == 0) {
				if (a == 0) { // if its hero who died
					_animations->initAnim(kLandDeath, 4, 0, 0);
					actor->controlMode = 0;
				} else {
					_sound->playSample(37, getRandomNumber(2000) + 3096, 1, actor->x, actor->y, actor->z, a);

					if (a == _scene->mecaPinguinIdx) {
						_extra->addExtraExplode(actor->x, actor->y, actor->z);
					}
				}

				if (actor->bonusParameter & 0x1F0 && !(actor->bonusParameter & 1)) {
					_actor->processActorExtraBonus(a);
				}
			}

			_movements->processActorMovements(a);

			actor->collisionX = actor->x;
			actor->collisionY = actor->y;
			actor->collisionZ = actor->z;

			if (actor->positionInMoveScript != -1) {
				_scriptMove->processMoveScript(a);
			}

			_animations->processActorAnimations(a);

			if (actor->staticFlags.bIsZonable) {
				_scene->processActorZones(a);
			}

			if (actor->positionInLifeScript != -1) {
				_scriptLife->processLifeScript(a);
			}

			processActorSamplePosition(a);

			if (quitGame != -1) {
				return quitGame;
			}

			if (actor->staticFlags.bCanDrown) {
				int32 brickSound;
				brickSound = _grid->getBrickSoundType(actor->x, actor->y - 1, actor->z);
				actor->brickSound = brickSound;

				if ((brickSound & 0xF0) == 0xF0) {
					if ((brickSound & 0xF) == 1) {
						if (a) { // all other actors
							int32 rnd = getRandomNumber(2000) + 3096;
							_sound->playSample(0x25, rnd, 1, actor->x, actor->y, actor->z, a);
							if (actor->bonusParameter & 0x1F0) {
								if (!(actor->bonusParameter & 1)) {
									_actor->processActorExtraBonus(a);
								}
								actor->life = 0;
							}
						} else { // if Hero
							if (_actor->heroBehaviour != 4 || (brickSound & 0x0F) != actor->anim) {
								if (!_actor->cropBottomScreen) {
									_animations->initAnim(kDrawn, 4, 0, 0);
									_renderer->projectPositionOnScreen(actor->x - _grid->cameraX, actor->y - _grid->cameraY, actor->z - _grid->cameraZ);
									_actor->cropBottomScreen = _renderer->projPosY;
								}
								_renderer->projectPositionOnScreen(actor->x - _grid->cameraX, actor->y - _grid->cameraY, actor->z - _grid->cameraZ);
								actor->controlMode = 0;
								actor->life = -1;
								_actor->cropBottomScreen = _renderer->projPosY;
								actor->staticFlags.bCanDrown |= 0x10;
							}
						}
					}
				}
			}

			if (actor->life <= 0) {
				if (!a) { // if its Hero
					if (actor->dynamicFlags.bAnimEnded) {
						if (_gameState->inventoryNumLeafs > 0) { // use clover leaf automaticaly
							_scene->sceneHero->x = _scene->newHeroX;
							_scene->sceneHero->y = _scene->newHeroY;
							_scene->sceneHero->z = _scene->newHeroZ;

							_scene->needChangeScene = _scene->currentSceneIdx;
							_gameState->inventoryMagicPoints = _gameState->magicLevelIdx * 20;

							_grid->newCameraX = (_scene->sceneHero->x >> 9);
							_grid->newCameraY = (_scene->sceneHero->y >> 8);
							_grid->newCameraZ = (_scene->sceneHero->z >> 9);

							_scene->heroPositionType = kReborn;

							_scene->sceneHero->life = 50;
							_redraw->reqBgRedraw = 1;
							_screens->lockPalette = 1;
							_gameState->inventoryNumLeafs--;
							_actor->cropBottomScreen = 0;
						} else { // game over
							_gameState->inventoryNumLeafsBox = 2;
							_gameState->inventoryNumLeafs = 1;
							_gameState->inventoryMagicPoints = _gameState->magicLevelIdx * 20;
							_actor->heroBehaviour = _actor->previousHeroBehaviour;
							actor->angle = _actor->previousHeroAngle;
							actor->life = 50;

							if (_scene->previousSceneIdx != _scene->currentSceneIdx) {
								_scene->newHeroX = -1;
								_scene->newHeroY = -1;
								_scene->newHeroZ = -1;
								_scene->currentSceneIdx = _scene->previousSceneIdx;
							}

							_gameState->saveGame();
							_gameState->processGameoverAnimation();
							quitGame = 0;
							return 0;
						}
					}
				} else {
					_actor->processActorCarrier(a);
					actor->dynamicFlags.bIsDead = 1;
					actor->entity = -1;
					actor->zone = -1;
				}
			}

			if (_scene->needChangeScene != -1) {
				return 0;
			}
		}
	}

	// recenter screen automatically
	if (!disableScreenRecenter && !_debugGrid->useFreeCamera) {
		ActorStruct *actor = &_scene->sceneActors[_scene->currentlyFollowedActor];
		_renderer->projectPositionOnScreen(actor->x - (_grid->newCameraX << 9),
		                                   actor->y - (_grid->newCameraY << 8),
		                                   actor->z - (_grid->newCameraZ << 9));
		if (_renderer->projPosX < 80 || _renderer->projPosX > 539 || _renderer->projPosY < 80 || _renderer->projPosY > 429) {
			_grid->newCameraX = ((actor->x + 0x100) >> 9) + (((actor->x + 0x100) >> 9) - _grid->newCameraX) / 2;
			_grid->newCameraY = actor->y >> 8;
			_grid->newCameraZ = ((actor->z + 0x100) >> 9) + (((actor->z + 0x100) >> 9) - _grid->newCameraZ) / 2;

			if (_grid->newCameraX >= 64) {
				_grid->newCameraX = 63;
			}

			if (_grid->newCameraZ >= 64) {
				_grid->newCameraZ = 63;
			}

			_redraw->reqBgRedraw = 1;
		}
	}

	_redraw->redrawEngineActions(_redraw->reqBgRedraw);

	// workaround to fix hero redraw after drowning
	if (_actor->cropBottomScreen && _redraw->reqBgRedraw == 1) {
		_scene->sceneHero->staticFlags.bIsHidden = 1;
		_redraw->redrawEngineActions(1);
		_scene->sceneHero->staticFlags.bIsHidden = 0;
	}

	_scene->needChangeScene = -1;
	_redraw->reqBgRedraw = 0;

	return 0;
}

bool TwinEEngine::gameEngineLoop() { // mainLoop
	uint32 start;

	_redraw->reqBgRedraw = 1;
	_screens->lockPalette = 1;
	_movements->setActorAngle(0, -256, 5, &loopMovePtr);

	while (quitGame == -1) {
		start = g_system->getMillis();

		while (g_system->getMillis() < start + cfgfile.Fps) {
			if (runGameEngine()) {
				return true;
			}
			g_system->delayMillis(10);
		}
		lbaTime++;
		if (shouldQuit()) {
			break;
		}
	}
	return false;
}

} // namespace TwinE
