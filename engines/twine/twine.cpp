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
		if (loopCurrentKey == Keys::Pause) {
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
	for (int32 a = 0; a < _scene->sceneNumActors; a++) {
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

/** Original audio frequency */
#define ORIGINAL_GAME_FREQUENCY 11025
/** High quality audio frequency */
#define HIGH_QUALITY_FREQUENCY 44100

#if 0
/** Main SDL screen surface buffer */
SDL_Surface *screen = NULL;
/** Auxiliar SDL screen surface buffer */
SDL_Surface *screenBuffer = NULL;
/** SDL screen color */
SDL_Color screenColors[256];
/** Auxiliar surface table  */
SDL_Surface *surfaceTable[16];

TTF_Font *font;
#endif

#if 0
int TwinEEngine::sdlInitialize() {
	uint8 *keyboard;
	int32 size;
	int32 i;
	int32 freq;

	Uint32 rmask, gmask, bmask;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
#endif

	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
		error("Couldn't initialize SDL: %s\n", SDL_GetError());
	}

	// Verify if we want to use high quality sounds
	if (cfgfile.Sound > 1)
		freq = HIGH_QUALITY_FREQUENCY;
	else
		freq = ORIGINAL_GAME_FREQUENCY;

	if (Mix_OpenAudio(freq, AUDIO_S16, 2, 256) < 0) {
		error("Mix_OpenAudio: %s\n", Mix_GetError());
	}

	Mix_AllocateChannels(32);

	SDL_WM_SetCaption("Little Big Adventure: TwinEngine", "twin-e");
	SDL_PumpEvents();

	keyboard = SDL_GetKeyState(&size);

	keyboard[SDLK_RETURN] = 0;

	screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE);

	if (screen == NULL) {
		error("Couldn't set 640x480x8 video mode: %s\n\n", SDL_GetError());
	}

	for (i = 0; i < 16; i++) {
		surfaceTable[i] = SDL_CreateRGBSurface(SDL_SWSURFACE, SCREEN_WIDTH, SCREEN_HEIGHT, 32, rmask, gmask, bmask, 0);
	}

	return 0;
}
#endif

/** Deplay certain seconds till proceed - Can skip delay
	@param time time in seconds to delay */
void TwinEEngine::delaySkip(uint32 time) {
#if 0
	uint32 startTicks = _engine->_system->getMillis();
	uint32 stopTicks = 0;
	_engine->_keyboard.skipIntro = 0;
	do {
		readKeys();
		if (_engine->_keyboard.skipIntro == 1) {
			break;
		}
		if (_engine->shouldQuit()) {
			break;
		}
		stopTicks = _engine->_system->getMillis() - startTicks;
		_engine->_system->delayMillis(1);
		//lbaTime++;
	} while (stopTicks <= time);
#endif
}

/** Set a new palette in the SDL screen buffer
	@param palette palette to set */
void TwinEEngine::setPalette(uint8 *palette) {
#if 0
	SDL_Color *screenColorsTemp = (SDL_Color *)palette;

	SDL_SetColors(screenBuffer, screenColorsTemp, 0, 256);
	SDL_BlitSurface(screenBuffer, NULL, screen, NULL);
	SDL_UpdateRect(screen, 0, 0, 0, 0);
#endif
}

/** Fade screen from black to white */
void TwinEEngine::fadeBlackToWhite() {
#if 0
	int32 i;

	SDL_Color colorPtr[256];

	SDL_UpdateRect(screen, 0, 0, 0, 0);

	for (i = 0; i < 256; i += 3) {
		memset(colorPtr, i, sizeof(colorPtr));
		SDL_SetPalette(screen, SDL_PHYSPAL, colorPtr, 0, 256);
	}
#endif
}

/** Blit surface in the screen */
void TwinEEngine::flip() {
#if 0
	SDL_BlitSurface(screenBuffer, NULL, screen, NULL);
	SDL_UpdateRect(screen, 0, 0, 0, 0);
#endif
}

/** Blit surface in the screen in a determinate area
	@param left left position to start copy
	@param top top position to start copy
	@param right right position to start copy
	@param bottom bottom position to start copy */
void TwinEEngine::copyBlockPhys(int32 left, int32 top, int32 right, int32 bottom) {
#if 0
	SDL_Rect rectangle;

	rectangle.x = left;
	rectangle.y = top;
	rectangle.w = right - left + 1;
	rectangle.h = bottom - top + 1;

	SDL_BlitSurface(screenBuffer, &rectangle, screen, &rectangle);
	SDL_UpdateRect(screen, left, top, right - left + 1, bottom - top + 1);
#endif
}

/** Create SDL screen surface
	@param buffer screen buffer to blit surface
	@param width screen width size
	@param height screen height size */
void TwinEEngine::initScreenBuffer(uint8 *buffer, int32 width, int32 height) {
#if 0
	screenBuffer = SDL_CreateRGBSurfaceFrom(buffer, width, height, 8, SCREEN_WIDTH, 0, 0, 0, 0);
#endif
}
#if 0

/** Cross fade feature
	@param buffer screen buffer
	@param palette new palette to cross fade */
void TwinEEngine::crossFade(uint8 *buffer, uint8 *palette) {
	int32 i;
	SDL_Surface *backupSurface;
	SDL_Surface *newSurface;
	SDL_Surface *tempSurface;
	Uint32 rmask, gmask, bmask;
	//	Uint32 amask;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
#endif

	backupSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, SCREEN_WIDTH, SCREEN_HEIGHT, 32, rmask, gmask, bmask, 0);
	newSurface = SDL_CreateRGBSurface(SDL_SWSURFACE | SDL_SRCALPHA, SCREEN_WIDTH, SCREEN_HEIGHT, 32, rmask, gmask, bmask, 0);

	tempSurface = SDL_CreateRGBSurfaceFrom(buffer, SCREEN_WIDTH, SCREEN_HEIGHT, 8, SCREEN_WIDTH, 0, 0, 0, 0);
	SDL_SetColors(tempSurface, (SDL_Color *)palette, 0, 256);

	SDL_BlitSurface(screen, NULL, backupSurface, NULL);
	SDL_BlitSurface(tempSurface, NULL, newSurface, NULL);

	for (i = 0; i < 8; i++) {
		SDL_BlitSurface(backupSurface, NULL, surfaceTable[i], NULL);
		SDL_SetAlpha(newSurface, SDL_SRCALPHA | SDL_RLEACCEL, i * 32);
		SDL_BlitSurface(newSurface, NULL, surfaceTable[i], NULL);
		SDL_BlitSurface(surfaceTable[i], NULL, screen, NULL);
		SDL_UpdateRect(screen, 0, 0, 0, 0);
		delaySkip(50);
	}

	SDL_BlitSurface(newSurface, NULL, screen, NULL);
	SDL_UpdateRect(screen, 0, 0, 0, 0);

	SDL_FreeSurface(backupSurface);
	SDL_FreeSurface(newSurface);
	SDL_FreeSurface(tempSurface);
}
#endif

/** Switch between window and fullscreen modes */
void TwinEEngine::toggleFullscreen() {
#if 0
	_engine->cfgfile.FullScreen = 1 - _engine->cfgfile.FullScreen;
	SDL_FreeSurface(screen);

	_engine->_redraw->reqBgRedraw = 1;

	if (_engine->cfgfile.FullScreen) {
		screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE);
		copyScreen(workVideoBuffer, frontVideoBuffer);
		SDL_ShowCursor(1);
	} else {
		screen = SDL_SetVideoMode(SCREEN_WIDTH, SCREEN_HEIGHT, 32, SDL_SWSURFACE | SDL_FULLSCREEN);
		copyScreen(workVideoBuffer, frontVideoBuffer);
	}
#endif
}

/** Pressed key map - scanCodeTab1 */
static const uint8 pressedKeyMap[] = {
    0x48, // 0
    0x50,
    0x4B,
    0x4D,
    0x47,
    0x49,
    0x51,
    0x4F, // 7

    0x39, // 8
    0x1C,
    0x1D,
    0x38,
    0x53,
    0x2A,
    0x36, // 14

    0x3B, // 15
    0x3C,
    0x3D,
    0x3E,
    0x3F,
    0x40, // LBAKEY_F6
    0x41,
    0x42,
    0x43,
    0x44,
    0x57,
    0x58,
    0x2A,
    0x0, // 28
};
static_assert(ARRAYSIZE(pressedKeyMap) == 29, "Expected size of key map");

/** Pressed key char map - scanCodeTab2 */
static const uint16 pressedKeyCharMap[] = {
    0x0100, // up
    0x0200, // down
    0x0400, // left
    0x0800, // right
    0x0500, // home
    0x0900, // pageup
    0x0A00, // pagedown
    0x0600, // end

    0x0101, // space bar
    0x0201, // enter
    0x0401, // ctrl
    0x0801, // alt
    0x1001, // del
    0x2001, // left shift
    0x2001, // right shift

    0x0102, // F1
    0x0202, // F2
    0x0402, // F3
    0x0802, // F4
    0x1002, // F5
    0x2002, // F6
    0x4002, // F7
    0x8002, // F8

    0x0103, // F9
    0x0203, // F10
    0x0403, // ?
    0x0803, // ?
    0x00FF, // left shift
    0x00FF,
    0x0,
    0x0,
};
static_assert(ARRAYSIZE(pressedKeyCharMap) == 31, "Expected size of key char map");

/** Handle keyboard pressed keys */
void TwinEEngine::readKeys() {
	if (shouldQuit()) {
		_keyboard.skipIntro = 1;
		_keyboard.skippedKey = 1;
		return;
	}
#if 0
	SDL_Event event;
	int32 localKey;
	int32 i, j, size;
	int32 find = 0;
	uint8 *keyboard;

	localKey = 0;
	_engine->_keyboard.skippedKey = 0;
	_engine->_keyboard.skipIntro = 0;

	SDL_PumpEvents();

	keyboard = SDL_GetKeyState(&size);

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			sdlClose();
			break;
		case SDL_MOUSEBUTTONDOWN:
			switch (event.button.button) {
			case SDL_BUTTON_RIGHT:
				_engine->rightMouse = 1;
				break;
			case SDL_BUTTON_LEFT:
				_engine->leftMouse = 1;
				break;
			}
			break;
		case SDL_KEYUP:
			_engine->_keyboard.pressedKey = 0;
			break;
		case SDL_KEYDOWN:
			switch (event.key.keysym.sym) {
			case SDLK_ESCAPE:
				localKey = 0x1;
				break;
			case SDLK_SPACE:
				localKey = 0x39;
				break;
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
				localKey = 0x1C;
				break;
			case SDLK_LSHIFT:
			case SDLK_RSHIFT:
				localKey = 0x36;
				break;
			case SDLK_LALT:
			case SDLK_RALT:
				localKey = 0x38;
				break;
			case SDLK_LCTRL:
			case SDLK_RCTRL:
				localKey = 0x1D;
				break;
			case SDLK_PAGEUP:
				localKey = 0x49;
				break;
			case SDLK_p: // pause
				localKey = Keys::Pause;
				break;
			case SDLK_h: // holomap
				localKey = 0x23;
				break;
			case SDLK_j:
				localKey = 0x24;
				break;
			case SDLK_w: // Especial key to do the action
				localKey = 0x11;
				break;
			case SDLK_F1:
				localKey = 0x3B;
				break;
			case SDLK_F2:
				localKey = 0x3C;
				break;
			case SDLK_F3:
				localKey = 0x3D;
				break;
			case SDLK_F4:
				localKey = 0x3E;
				break;
			case SDLK_F6:
				localKey = 0x40;
				break;
			case SDLK_F12:
				toggleFullscreen();
				break;
			default:
				break;
			}
			if (_engine->cfgFile.Debug) {
				switch (event.key.keysym.sym) {
				case SDLK_r: // next room
					localKey = Keys::NextRoom;
					break;
				case SDLK_f: // previous room
					localKey = Keys::PreviousRoom;
					break;
				case SDLK_t: // apply celling grid
					localKey = Keys::ApplyCellingGrid;
					break;
				case SDLK_g: // increase celling grid index
					localKey = Keys::IncreaseCellingGridIndex;
					break;
				case SDLK_b: // decrease celling grid index
					localKey = Keys::DecreaseCellingGridIndex;
					break;
				default:
					break;
				}
			}
			break;
		}
	}

	for (j = 0; j < size; j++) {
		if (keyboard[j]) {
			switch (j) {
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
				localKey = 0x1C;
				break;
			case SDLK_SPACE:
				localKey = 0x39;
				break;
			case SDLK_UP:
			case SDLK_KP8:
				localKey = 0x48;
				break;
			case SDLK_DOWN:
			case SDLK_KP2:
				localKey = 0x50;
				break;
			case SDLK_LEFT:
			case SDLK_KP4:
				localKey = 0x4B;
				break;
			case SDLK_RIGHT:
			case SDLK_KP6:
				localKey = 0x4D;
				break;
			case SDLK_LCTRL:
			case SDLK_RCTRL:
				localKey = 0x1D;
				break;
			/*case SDLK_LSHIFT:
			case SDLK_RSHIFT:
				localKey = 0x36;
				break;*/
			case SDLK_LALT:
			case SDLK_RALT:
				localKey = 0x38;
				break;
			case SDLK_F1:
				localKey = 0x3B;
				break;
			case SDLK_F2:
				localKey = 0x3C;
				break;
			case SDLK_F3:
				localKey = 0x3D;
				break;
			case SDLK_F4:
				localKey = 0x3E;
				break;
			default:
				break;
			}
			if (_engine->cfgFile.Debug) {
				switch (keyboard[j]) {
				// change grid camera
				case SDLK_s:
					localKey = 0x1F;
					break;
				case SDLK_x:
					localKey = 0x2D;
					break;
				case SDLK_z:
					localKey = 0x2C;
					break;
				case SDLK_c:
					localKey = 0x2E;
					break;
				}
			}
		}

		bool found = false;
		for (i = 0; i < 28; i++) {
			if (_engine->_keyboard.pressedKeyMap[i] == localKey) {
				find = i;
				found = true;
				break;
			}
		}

		if (found) {
			int16 temp = pressedKeyCharMap[find];
			uint8 temp2 = temp & 0x00FF;

			if (temp2 == 0) {
				// pressed valid keys
				if (!(localKey & 0x80)) {
					_engine->_keyboard.pressedKey |= (temp & 0xFF00) >> 8;
				} else {
					_engine->_keyboard.pressedKey &= -((temp & 0xFF00) >> 8);
				}
			}
			// pressed inactive keys
			else {
				_engine->_keyboard.skippedKey |= (temp & 0xFF00) >> 8;
			}
		}

		//if (!found) {
		_engine->_keyboard.skipIntro = localKey;
		//}
	}
#endif
}

void TwinEEngine::ttfDrawText(int32 x, int32 y, const char *string, int32 center) {
#if 0 // TODO
	SDL_Color white = {0xFF, 0xFF, 0xFF, 0};
	SDL_Color *forecol = &white;
	SDL_Rect rectangle;
	SDL_Surface *text = TTF_RenderText_Solid(font, string, *forecol);

	if (center) {
		rectangle.x = x - (text->w / 2);
	} else {
		rectangle.x = x;
	}
	rectangle.y = y - 2;
	rectangle.w = text->w;
	rectangle.h = text->h;

	SDL_BlitSurface(text, NULL, screenBuffer, &rectangle);
	SDL_FreeSurface(text);
#endif
}

void TwinEEngine::getMousePositions(MouseStatusStruct *mouseData) {
#if 0 // TODO:
	SDL_GetMouseState(&mouseData->X, &mouseData->Y);
	mouseData->left = _engine->leftMouse;
	mouseData->right = _engine->rightMouse;

	_engine->leftMouse = 0;
	_engine->rightMouse = 0;
#endif
}

} // namespace TwinE
