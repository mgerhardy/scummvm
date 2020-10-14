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

#include "twine.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "actor.h"
#include "animations.h"
#include "common/textconsole.h"
#include "fcaseopen.h"
#include "flamovies.h"
#include "gamestate.h"
#include "grid.h"
#include "hqrdepack.h"
#include "interface.h"
#include "keyboard.h"
#include "menu.h"
#include "music.h"
#include "redraw.h"
#include "renderer.h"
#include "resources.h"
#include "scene.h"
#include "screens.h"
#include "sdlengine.h"
#include "sound.h"
#include "text.h"

#include "animations.h"
#include "collision.h"
#include "debug.grid.h"
#include "extra.h"
#include "gamestate.h"
#include "grid.h"
#include "hqrdepack.h"
#include "interface.h"
#include "keyboard.h"
#include "menu.h"
#include "menuoptions.h"
#include "movements.h"
#include "redraw.h"
#include "renderer.h"
#include "resources.h"
#include "scene.h"
#include "screens.h"
#include "script.life.h"
#include "script.move.h"
#include "sdlengine.h"
#include "sound.h"
#include "text.h"

#ifdef GAMEMOD
#include "debug.h"
#endif

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
const int8 *ENGINE_VERSION = (const int8 *)"0.2.0";

/** Engine configuration filename */
const int8 *CONFIG_FILENAME = (const int8 *)"lba.cfg";

/** Engine install setup filename

	This file contains informations about the game version.
	This is only used for original games. For mod games project you can
	used \a lba.cfg file \b Version tag. If this tag is set for original game
	it will be used instead of \a setup.lst file. */
const int8 *SETUP_FILENAME = (const int8 *)"setup.lst";

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
    "Français",
    "Deutsch",
    "Español",
    "Italiano",
    "Português"};

/** Allocate video memory, both front and back buffers */
void TwinEEngine::allocVideoMemory() {
	int32 i, j, k;

	workVideoBuffer = (uint8 *)malloc((SCREEN_WIDTH * SCREEN_HEIGHT) * sizeof(uint8));
	frontVideoBuffer = frontVideoBufferbis = (uint8 *)malloc(sizeof(uint8) * SCREEN_WIDTH * SCREEN_HEIGHT);
	initScreenBuffer(frontVideoBuffer, SCREEN_WIDTH, SCREEN_HEIGHT);

	j = 0;
	k = 0;
	for (i = SCREEN_HEIGHT; i > 0; i--) {
		screenLookupTable[j] = k;
		j++;
		k += SCREEN_WIDTH;
	}

	// initVideoVar1 = -1;
}

/** Gets configuration type index from lba.cfg config file
	@param lineBuffer buffer with config line
	@return config type index */
int TwinEEngine::getConfigTypeIndex(int8 *lineBuffer) {
	int32 i;
	char buffer[256];
	char *ptr;

	strcpy(buffer, (char*)lineBuffer);

	ptr = strchr(buffer, ' ');

	if (ptr) {
		*ptr = 0;
	}

	for (i = 0; i < (sizeof(CFGList) / 22); i++) {
		if (strlen(CFGList[i])) {
			if (!strcmp(CFGList[i], buffer)) {
				return i;
			}
		}
	}

	return -1;
}

/** Gets configuration type index from lba.cfg config file
	@param lineBuffer buffer with config line
	@return config type index */
int TwinEEngine::getLanguageTypeIndex(int8 *language) {
	int32 i;
	char buffer[256];
	char *ptr;

	strcpy(buffer, (char*)language);

	ptr = strchr(buffer, ' ');

	if (ptr) {
		*ptr = 0;
	}

	for (i = 0; i < (sizeof(LanguageTypes) / 10); i++) {
		if (strlen(LanguageTypes[i])) {
			if (!strcmp(LanguageTypes[i], buffer)) {
				return i;
			}
		}
	}

	return 0; // English
}

/** Init configuration file \a lba.cfg */
void TwinEEngine::initConfigurations() {
	FILE *fd, *fd_test;
	int8 buffer[256], tmp[16];
	int32 cfgtype = -1;

	fd = fcaseopen(CONFIG_FILENAME, "rb");
	if (!fd)
		error("Error: Can't find config file %s\n", CONFIG_FILENAME);

	// make sure it quit when it reaches the end of file
	while (fgets((char*)buffer, 256, fd) != NULL) {
		*strchr((char*)buffer, 0x0D0A) = 0;
		cfgtype = getConfigTypeIndex(buffer);
		if (cfgtype != -1) {
			switch (cfgtype) {
			case 0:
				sscanf((const char*)buffer, "Language: %s", cfgfile.Language);
				cfgfile.LanguageId = getLanguageTypeIndex(cfgfile.Language);
				break;
			case 1:
				sscanf((const char*)buffer, "LanguageCD: %s", cfgfile.LanguageCD);
				cfgfile.LanguageCDId = getLanguageTypeIndex(cfgfile.Language) + 1;
				break;
			case 2:
				sscanf((const char*)buffer, "FlagDisplayText: %s", cfgfile.FlagDisplayTextStr);
				if (!strcmp((char*)cfgfile.FlagDisplayTextStr, "ON")) {
					cfgfile.FlagDisplayText = 1;
				} else {
					cfgfile.FlagDisplayText = 0;
				}
				break;
			case 3:
				sscanf((const char*)buffer, "FlagKeepVoice: %s", cfgfile.FlagKeepVoiceStr);
				break;
			case 8:
				sscanf((const char*)buffer, "MidiType: %s", tmp);
				if (strcmp((char*)tmp, "auto") == 0) {
					fd_test = fcaseopen(HQR_MIDI_MI_WIN_FILE, "rb");
					if (fd_test) {
						fclose(fd_test);
						cfgfile.MidiType = 1;
					} else
						cfgfile.MidiType = 0;
				} else if (strcmp((char*)tmp, "midi") == 0)
					cfgfile.MidiType = 1;
				else
					cfgfile.MidiType = 0;
				break;
			case 19:
				sscanf((const char*)buffer, "WaveVolume: %d", &cfgfile.WaveVolume);
				cfgfile.VoiceVolume = cfgfile.WaveVolume;
				break;
			case 20:
				sscanf((const char*)buffer, "VoiceVolume: %d", &cfgfile.VoiceVolume);
				break;
			case 21:
				sscanf((const char*)buffer, "MusicVolume: %d", &cfgfile.MusicVolume);
				break;
			case 22:
				sscanf((const char*)buffer, "CDVolume: %d", &cfgfile.CDVolume);
				break;
			case 23:
				sscanf((const char*)buffer, "LineVolume: %d", &cfgfile.LineVolume);
				break;
			case 24:
				sscanf((const char*)buffer, "MasterVolume: %d", &cfgfile.MasterVolume);
				break;
			case 25:
				sscanf((const char*)buffer, "Version: %d", &cfgfile.Version);
				break;
			case 26:
				sscanf((const char*)buffer, "FullScreen: %d", &cfgfile.FullScreen);
				break;
			case 27:
				sscanf((const char*)buffer, "UseCD: %d", &cfgfile.UseCD);
				break;
			case 28:
				sscanf((const char*)buffer, "Sound: %d", &cfgfile.Sound);
				break;
			case 29:
				sscanf((const char*)buffer, "Movie: %d", &cfgfile.Movie);
				break;
			case 30:
				sscanf((const char*)buffer, "CrossFade: %d", &cfgfile.CrossFade);
				break;
			case 31:
				sscanf((const char*)buffer, "Fps: %d", &cfgfile.Fps);
				break;
			case 32:
				sscanf((const char*)buffer, "Debug: %d", &cfgfile.Debug);
				break;
			case 33:
				sscanf((const char*)buffer, "UseAutoSaving: %d", &cfgfile.UseAutoSaving);
				break;
			case 34:
				sscanf((const char*)buffer, "CombatAuto: %d", &cfgfile.AutoAgressive);
				break;
			case 35:
				sscanf((const char*)buffer, "Shadow: %d", &cfgfile.ShadowMode);
				break;
			case 36:
				sscanf((const char*)buffer, "SceZoom: %d", &cfgfile.SceZoom);
				break;
			case 37:
				sscanf((const char*)buffer, "FillDetails: %d", &cfgfile.FillDetails);
				break;
			case 38:
				sscanf((const char*)buffer, "InterfaceStyle: %d", &cfgfile.InterfaceStyle);
				break;
			case 39:
				sscanf((const char*)buffer, "WallCollision: %d", &cfgfile.WallCollision);
				break;
			}
		}
	}

	if (!cfgfile.Fps)
		cfgfile.Fps = DEFAULT_FRAMES_PER_SECOND;

	fclose(fd);
}

/** Initialize LBA engine */
void TwinEEngine::initEngine() {
	// getting configuration file
	initConfigurations();

	// Show engine information
	printf("TwinEngine v%s\n\n", ENGINE_VERSION);
	printf("(c)2002 The TwinEngine team. Refer to AUTHORS file for further details.\n");
	printf("Released under the terms of the GNU GPL license version 2 (or, at your opinion, any later). See COPYING file.\n\n");
	printf("The original Little Big Adventure game is:\n\t(c)1994 by Adeline Software International, All Rights Reserved.\n\n");

	allocVideoMemory();
	clearScreen();

	// Toggle fullscreen if Fullscreen flag is set
	toggleFullscreen();

	// Check if LBA CD-Rom is on drive
	initCdrom();

	// Display company logo
	adelineLogo();

	// verify game version screens
	if (cfgfile.Version == EUROPE_VERSION) {
		// Little Big Adventure screen
		loadImageDelay(RESSHQR_LBAIMG, 3);
		// Electronic Arts Logo
		loadImageDelay(RESSHQR_EAIMG, 2);
	} else if (cfgfile.Version == USA_VERSION) {
		// Relentless screen
		loadImageDelay(RESSHQR_RELLENTIMG, 3);
		// Electronic Arts Logo
		loadImageDelay(RESSHQR_EAIMG, 2);
	} else if (cfgfile.Version == MODIFICATION_VERSION) {
		// Modification screen
		loadImageDelay(RESSHQR_RELLENTIMG, 2);
	}

	playFlaMovie(FLA_DRAGON3);

	loadMenuImage(1);

	mainMenu();
}

void TwinEEngine::initMCGA() {
	drawInGameTransBox = 1;
}

void TwinEEngine::initSVGA() {
	drawInGameTransBox = 0;
}

/** Initialize all needed stuffs at first time running engine */
void TwinEEngine::initAll() {
	blockBuffer = (uint8 *)malloc(64 * 64 * 25 * 2 * sizeof(uint8));
	animBuffer1 = animBuffer2 = (uint8 *)malloc(5000 * sizeof(uint8));
	memset(samplesPlaying, -1, sizeof(int32) * NUM_CHANNELS);
	memset(itemAngle, 256, sizeof(itemAngle)); // reset inventory items angles

	bubbleSpriteIndex = SPRITEHQR_DIAG_BUBBLE_LEFT;
	bubbleActor = -1;
	showDialogueBubble = 1;

	currentTextBank = -1;
	currMenuTextIndex = -1;
	currMenuTextBank = -1;
	autoAgressive = 1;

	sceneHero = &sceneActors[0];

	renderLeft = 0;
	renderTop = 0;
	renderRight = SCREEN_TEXTLIMIT_RIGHT;
	renderBottom = SCREEN_TEXTLIMIT_BOTTOM;
	// Set clip to fullscreen by default, allows main menu to render properly after load
	resetClip();

	rightMouse = 0;
	leftMouse = 0;

	initResources();

	initSVGA();
}

// AUX FUNC

int8 *TwinEEngine::ITOA(int32 number) {
	int32 numDigits = 1;
	int8 *text;

	if (number >= 10 && number <= 99) {
		numDigits = 2;
	} else if (number >= 100 && number <= 999) {
		numDigits = 3;
	}

	text = (int8 *)malloc(sizeof(int8) * (numDigits + 1));
	sprintf(text, "%d", number);
	return text;
}

TwinEEngine::TwinEEngine(OSystem *system, Common::Language language, uint32 flags)
    : Engine(system), _gameLang(language), _gameFlags(flags), _rnd("twine") {
	_actor = new Actor(this);
	_animations = new Animations(this);
	_collision = new Collision(this);
	_extra = new Extra(this);
	_gameState = new GameState(this);
	_grid = new Grid(this);
	_movements = new Movements(this);
}

TwinEEngine::~TwinEEngine() {
}

bool TwinEEngine::hasFeature(EngineFeature f) const {
	return false;
}

Common::Error TwinEEngine::run() {
	initGraphics(kScreenWidth, kScreenHeight);
	syncSoundSettings();
	initAll();
	initEngine();
	stopTrackMusic();
	stopMidiMusic();
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
	ActorStruct *actor = &sceneActors[actorIdx];
	channelIdx = getActorChannel(actorIdx);
	setSamplePosition(channelIdx, actor->X, actor->Y, actor->Z);
}

/** Game engine main loop
	@return true if we want to show credit sequence */
int32 TwinEEngine::runGameEngine() { // mainLoopInteration
	int32 a;
	readKeys();

	if (needChangeScene > -1) {
		changeScene();
	}

	previousLoopPressedKey = loopPressedKey;
	key = pressedKey;
	loopPressedKey = skippedKey;
	loopCurrentKey = skipIntro;

#ifdef GAMEMOD
	processDebug(loopCurrentKey);
#endif

	if (canShowCredits != 0) {
		// TODO: if current music playing != 8, than play_track(8);
		if (skipIntro != 0) {
			return 0;
		}
		if (pressedKey != 0) {
			return 0;
		}
		if (skippedKey != 0) {
			return 0;
		}
	} else {
		// Process give up menu - Press ESC
		if (skipIntro == 1 && sceneHero->life > 0 && sceneHero->entity != -1 && !sceneHero->staticFlags.bIsHidden) {
			freezeTime();
			if (giveupMenu()) {
				unfreezeTime();
				redrawEngineActions(1);
				freezeTime();
				saveGame(); // auto save game
				quitGame = 0;
				cfgfile.Quit = 0;
				unfreezeTime();
				return 0;
			} else {
				unfreezeTime();
				redrawEngineActions(1);
			}
		}

		// Process options menu - Press F6
		if (loopCurrentKey == 0x40) {
			int tmpLangCD = cfgfile.LanguageCDId;
			freezeTime();
			pauseSamples();
			OptionsMenuSettings[5] = 15;
			cfgfile.LanguageCDId = 0;
			initTextBank(0);
			optionsMenu();
			cfgfile.LanguageCDId = tmpLangCD;
			initTextBank(currentTextBank + 3);
			//TODO: play music
			resumeSamples();
			unfreezeTime();
			redrawEngineActions(1);
		}

		// inventory menu
		loopInventoryItem = -1;
		if (loopCurrentKey == 0x36 && sceneHero->entity != -1 && sceneHero->controlMode == kManual) {
			freezeTime();
			processInventoryMenu();

			switch (loopInventoryItem) {
			case kiHolomap:
				printf("Use Inventory [kiHolomap] not implemented!\n");
				break;
			case kiMagicBall:
				if (usingSabre == 1) {
					initModelActor(0, 0);
				}
				usingSabre = 0;
				break;
			case kiUseSabre:
				if (sceneHero->body != GAMEFLAG_HAS_SABRE) {
					if (heroBehaviour == kProtoPack) {
						setBehaviour(kNormal);
					}
					initModelActor(GAMEFLAG_HAS_SABRE, 0);
					initAnim(24, 1, 0, 0);

					usingSabre = 1;
				}
				break;
			case kiBookOfBu: {
				int32 tmpFlagDisplayText;

				fadeToBlack(paletteRGBA);
				loadImage(RESSHQR_INTROSCREEN1IMG, 1);
				initTextBank(2);
				newGameVar4 = 0;
				textClipFull();
				setFontCrossColor(15);
				tmpFlagDisplayText = cfgfile.FlagDisplayText;
				cfgfile.FlagDisplayText = 1;
				drawTextFullscreen(161);
				cfgfile.FlagDisplayText = tmpFlagDisplayText;
				textClipSmall();
				newGameVar4 = 1;
				initTextBank(currentTextBank + 3);
				fadeToBlack(paletteRGBACustom);
				clearScreen();
				flip();
				setPalette(paletteRGBA);
				lockPalette = 1;
			} break;
			case kiProtoPack:
				if (gameFlags[GAMEFLAG_BOOKOFBU]) {
					sceneHero->body = 0;
				} else {
					sceneHero->body = 1;
				}

				if (heroBehaviour == kProtoPack) {
					setBehaviour(kNormal);
				} else {
					setBehaviour(kProtoPack);
				}
				break;
			case kiPinguin: {
				ActorStruct *pinguin = &sceneActors[mecaPinguinIdx];

				pinguin->X = destX + sceneHero->X;
				pinguin->Y = sceneHero->Y;
				pinguin->Z = destZ + sceneHero->Z;
				pinguin->angle = sceneHero->angle;

				rotateActor(0, 800, pinguin->angle);

				if (!checkCollisionWithActors(mecaPinguinIdx)) {
					pinguin->life = 50;
					pinguin->body = -1;
					initModelActor(0, mecaPinguinIdx);
					pinguin->dynamicFlags.bIsDead = 0; // &= 0xDF
					pinguin->brickShape = 0;
					moveActor(pinguin->angle, pinguin->angle, pinguin->speed, &pinguin->move);
					gameFlags[GAMEFLAG_MECA_PINGUIN] = 0; // byte_50D89 = 0;
					pinguin->info0 = lbaTime + 1500;
				}
			} break;
			case kiBonusList: {
				int32 tmpLanguageCDIdx;
				tmpLanguageCDIdx = cfgfile.LanguageCDId;
				unfreezeTime();
				redrawEngineActions(1);
				freezeTime();
				cfgfile.LanguageCDId = 0;
				initTextBank(2);
				textClipFull();
				setFontCrossColor(15);
				drawTextFullscreen(162);
				textClipSmall();
				cfgfile.LanguageCDId = tmpLanguageCDIdx;
				initTextBank(currentTextBank + 3);
			} break;
			case kiCloverLeaf:
				if (sceneHero->life < 50) {
					if (inventoryNumLeafs > 0) {
						sceneHero->life = 50;
						inventoryMagicPoints = magicLevelIdx * 20;
						inventoryNumLeafs--;
						addOverlay(koInventoryItem, 27, 0, 0, 0, koNormal, 3);
					}
				}
				break;
			}

			unfreezeTime();
			redrawEngineActions(1);
		}

		// Process behaviour menu - Press CTRL and F1..F4 Keys
		if ((loopCurrentKey == 0x1D || loopCurrentKey == 0x3B || loopCurrentKey == 0x3C || loopCurrentKey == 0x3D || loopCurrentKey == 0x3E) && sceneHero->entity != -1 && sceneHero->controlMode == kManual) {
			if (loopCurrentKey != 0x1D) {
				heroBehaviour = loopCurrentKey - 0x3B;
			}
			freezeTime();
			processBehaviourMenu();
			unfreezeTime();
			redrawEngineActions(1);
		}

		// use Proto-Pack
		if (loopCurrentKey == 0x24 && gameFlags[GAMEFLAG_PROTOPACK] == 1) {
			if (gameFlags[GAMEFLAG_BOOKOFBU]) {
				sceneHero->body = 0;
			} else {
				sceneHero->body = 1;
			}

			if (heroBehaviour == kProtoPack) {
				setBehaviour(kNormal);
			} else {
				setBehaviour(kProtoPack);
			}
		}

		// Press Enter to Recenter Screen
		if ((loopPressedKey & 2) && !disableScreenRecenter) {
			newCameraX = sceneActors[currentlyFollowedActor].X >> 9;
			newCameraY = sceneActors[currentlyFollowedActor].Y >> 8;
			newCameraZ = sceneActors[currentlyFollowedActor].Z >> 9;
			reqBgRedraw = 1;
		}

		// TODO: draw holomap

		// Process Pause - Press P
		if (loopCurrentKey == 0x19) {
			freezeTime();
			setFontColor(15);
			drawText(5, 446, (int8 *)"Pause"); // no key for pause in Text Bank
			copyBlockPhys(5, 446, 100, 479);
			do {
				readKeys();
				SDL_Delay(10);
			} while (skipIntro != 0x19 && !pressedKey);
			unfreezeTime();
			redrawEngineActions(1);
		}
	}

	loopActorStep = getRealValue(&loopMovePtr);
	if (!loopActorStep) {
		loopActorStep = 1;
	}

	setActorAngle(0, -256, 5, &loopMovePtr);
	disableScreenRecenter = 0;

	processEnvironmentSound();

	// Reset HitBy state
	for (a = 0; a < sceneNumActors; a++) {
		sceneActors[a].hitBy = -1;
	}

	processExtras();

	for (a = 0; a < sceneNumActors; a++) {
		ActorStruct *actor = &sceneActors[a];

		if (!actor->dynamicFlags.bIsDead) {
			if (actor->life == 0) {
				if (a == 0) { // if its hero who died
					initAnim(kLandDeath, 4, 0, 0);
					actor->controlMode = 0;
				} else {
					playSample(37, Rnd(2000) + 3096, 1, actor->X, actor->Y, actor->Z, a);

					if (a == mecaPinguinIdx) {
						addExtraExplode(actor->X, actor->Y, actor->Z);
					}
				}

				if (actor->bonusParameter & 0x1F0 && !(actor->bonusParameter & 1)) {
					processActorExtraBonus(a);
				}
			}

			processActorMovements(a);

			actor->collisionX = actor->X;
			actor->collisionY = actor->Y;
			actor->collisionZ = actor->Z;

			if (actor->positionInMoveScript != -1) {
				processMoveScript(a);
			}

			processActorAnimations(a);

			if (actor->staticFlags.bIsZonable) {
				processActorZones(a);
			}

			if (actor->positionInLifeScript != -1) {
				processLifeScript(a);
			}

			processActorSamplePosition(a);

			if (quitGame != -1) {
				return quitGame;
			}

			if (actor->staticFlags.bCanDrown) {
				int32 brickSound;
				brickSound = getBrickSoundType(actor->X, actor->Y - 1, actor->Z);
				actor->brickSound = brickSound;

				if ((brickSound & 0xF0) == 0xF0) {
					if ((brickSound & 0xF) == 1) {
						if (a) { // all other actors
							int32 rnd = Rnd(2000) + 3096;
							playSample(0x25, rnd, 1, actor->X, actor->Y, actor->Z, a);
							if (actor->bonusParameter & 0x1F0) {
								if (!(actor->bonusParameter & 1)) {
									processActorExtraBonus(a);
								}
								actor->life = 0;
							}
						} else { // if Hero
							if (heroBehaviour != 4 || (brickSound & 0x0F) != actor->anim) {
								if (!cropBottomScreen) {
									initAnim(kDrawn, 4, 0, 0);
									projectPositionOnScreen(actor->X - cameraX, actor->Y - cameraY, actor->Z - cameraZ);
									cropBottomScreen = projPosY;
								}
								projectPositionOnScreen(actor->X - cameraX, actor->Y - cameraY, actor->Z - cameraZ);
								actor->controlMode = 0;
								actor->life = -1;
								cropBottomScreen = projPosY;
								actor->staticFlags.bCanDrown |= 0x10;
							}
						}
					}
				}
			}

			if (actor->life <= 0) {
				if (!a) { // if its Hero
					if (actor->dynamicFlags.bAnimEnded) {
						if (inventoryNumLeafs > 0) { // use clover leaf automaticaly
							sceneHero->X = newHeroX;
							sceneHero->Y = newHeroY;
							sceneHero->Z = newHeroZ;

							needChangeScene = currentSceneIdx;
							inventoryMagicPoints = magicLevelIdx * 20;

							newCameraX = (sceneHero->X >> 9);
							newCameraY = (sceneHero->Y >> 8);
							newCameraZ = (sceneHero->Z >> 9);

							heroPositionType = kReborn;

							sceneHero->life = 50;
							reqBgRedraw = 1;
							lockPalette = 1;
							inventoryNumLeafs--;
							cropBottomScreen = 0;
						} else { // game over
							inventoryNumLeafsBox = 2;
							inventoryNumLeafs = 1;
							inventoryMagicPoints = magicLevelIdx * 20;
							heroBehaviour = previousHeroBehaviour;
							actor->angle = previousHeroAngle;
							actor->life = 50;

							if (previousSceneIdx != currentSceneIdx) {
								newHeroX = -1;
								newHeroY = -1;
								newHeroZ = -1;
								currentSceneIdx = previousSceneIdx;
							}

							saveGame();
							processGameoverAnimation();
							quitGame = 0;
							return 0;
						}
					}
				} else {
					processActorCarrier(a);
					actor->dynamicFlags.bIsDead = 1;
					actor->entity = -1;
					actor->zone = -1;
				}
			}

			if (needChangeScene != -1) {
				return 0;
			}
		}
	}

	// recenter screen automatically
	if (!disableScreenRecenter && !useFreeCamera) {
		ActorStruct *actor = &sceneActors[currentlyFollowedActor];
		projectPositionOnScreen(actor->X - (newCameraX << 9),
		                        actor->Y - (newCameraY << 8),
		                        actor->Z - (newCameraZ << 9));
		if (projPosX < 80 || projPosX > 539 || projPosY < 80 || projPosY > 429) {
			newCameraX = ((actor->X + 0x100) >> 9) + (((actor->X + 0x100) >> 9) - newCameraX) / 2;
			newCameraY = actor->Y >> 8;
			newCameraZ = ((actor->Z + 0x100) >> 9) + (((actor->Z + 0x100) >> 9) - newCameraZ) / 2;

			if (newCameraX >= 64) {
				newCameraX = 63;
			}

			if (newCameraZ >= 64) {
				newCameraZ = 63;
			}

			reqBgRedraw = 1;
		}
	}

	redrawEngineActions(reqBgRedraw);

	// workaround to fix hero redraw after drowning
	if (cropBottomScreen && reqBgRedraw == 1) {
		sceneHero->staticFlags.bIsHidden = 1;
		redrawEngineActions(1);
		sceneHero->staticFlags.bIsHidden = 0;
	}

	needChangeScene = -1;
	reqBgRedraw = 0;

	return 0;
}

/** Game engine main loop
	@return true if we want to show credit sequence */
int32 TwinEEngine::gameEngineLoop() { // mainLoop
	uint32 start;

	reqBgRedraw = 1;
	lockPalette = 1;
	setActorAngle(0, -256, 5, &loopMovePtr);

	while (quitGame == -1) {
		start = SDL_GetTicks();

		while (SDL_GetTicks() < start + cfgfile.Fps) {
			if (runGameEngine())
				return 1;
			SDL_Delay(1);
		}
		lbaTime++;
	}
	return 0;
}

} // namespace TwinE
