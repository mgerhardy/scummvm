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

#include "gamestate.h"
#include "animations.h"
#include "collision.h"
#include "common/textconsole.h"
#include "extra.h"
#include "filereader.h"
#include "grid.h"
#include "interface.h"
#include "keyboard.h"
#include "menu.h"
#include "menuoptions.h"
#include "music.h"
#include "redraw.h"
#include "renderer.h"
#include "resources.h"
#include "scene.h"
#include "screens.h"
#include "sound.h"
#include "text.h"
#include "actor.h"
#include "twine.h"

namespace TwinE {

#define SAVE_DIR "save/"

GameState::GameState(TwinEEngine* engine) : _engine(engine) {}

/** Initialize engine 3D projections */
void GameState::initEngineProjections() { // reinitAll1
	setOrthoProjection(311, 240, 512);
	setBaseTranslation(0, 0, 0);
	setBaseRotation(0, 0, 0);
	setLightVector(alphaLight, betaLight, 0);
}

/** Initialize variables */
void GameState::initSceneVars() {
	int32 i;

	_engine->_extra->resetExtras();

	for (i = 0; i < OVERLAY_MAX_ENTRIES; i++) {
		overlayList[i].info0 = -1;
	}

	for (i = 0; i < NUM_SCENES_FLAGS; i++) {
		sceneFlags[i] = 0;
	}

	for (i = 0; i < NUM_GAME_FLAGS; i++) {
		gameFlags[i] = 0;
	}

	for (i = 0; i < NUM_INVENTORY_ITEMS; i++) {
		inventoryFlags[i] = 0;
	}

	sampleAmbiance[0] = -1;
	sampleAmbiance[1] = -1;
	sampleAmbiance[2] = -1;
	sampleAmbiance[3] = -1;

	sampleRepeat[0] = 0;
	sampleRepeat[1] = 0;
	sampleRepeat[2] = 0;
	sampleRepeat[3] = 0;

	sampleRound[0] = 0;
	sampleRound[1] = 0;
	sampleRound[2] = 0;
	sampleRound[3] = 0;

	for (i = 0; i < 150; i++) {
		holomapFlags[i] = 0;
	}

	sceneNumActors = 0;
	sceneNumZones = 0;
	sceneNumTracks = 0;

	_engine->_actor->currentPositionInBodyPtrTab = 0;
}

void GameState::initHeroVars() { // reinitAll3
	_engine->_actor->resetActor(0);    // reset Hero

	magicBallIdx = -1;

	inventoryNumLeafsBox = 2;
	inventoryNumLeafs = 2;
	inventoryNumKashes = 0;
	inventoryNumKeys = 0;
	inventoryMagicPoints = 0;

	usingSabre = 0;

	sceneHero->body = 0;
	sceneHero->life = 50;
	sceneHero->talkColor = 4;
}

/** Initialize all engine variables */
void GameState::initEngineVars(int32 save) { // reinitAll
	_engine->_interface->resetClip();

	alphaLight = 896;
	betaLight = 950;
	initEngineProjections();
	initSceneVars();
	initHeroVars();

	newHeroX = 0x2000;
	newHeroY = 0x1800;
	newHeroZ = 0x2000;

	currentSceneIdx = -1;
	needChangeScene = 0;
	_engine->quitGame = -1;
	mecaPinguinIdx = -1;
	canShowCredits = 0;

	inventoryNumLeafs = 0;
	inventoryNumLeafsBox = 2;
	inventoryMagicPoints = 0;
	inventoryNumKashes = 0;
	inventoryNumKeys = 0;
	inventoryNumGas = 0;

	_engine->_actor->cropBottomScreen = 0;

	magicLevelIdx = 0;
	usingSabre = 0;

	gameChapter = 0;

	currentTextBank = 0;
	currentlyFollowedActor = 0;
	_engine->_actor->heroBehaviour = 0;
	_engine->_actor->previousHeroAngle = 0;
	_engine->_actor->previousHeroBehaviour = 0;

	if (save == -1) {
		loadGame();
		if (newHeroX == -1) {
			heroPositionType = kNoPosition;
		}
	}
}

void GameState::loadGame() {
	FileReader fr;
	uint8 data;
	int8 *namePtr;

	if (!fropen2(&fr, SAVE_DIR "S9999.LBA", "rb")) {
		warning("Can't load S9999.LBA saved game!\n");
		return;
	}

	namePtr = savePlayerName;

	frread(&fr, &data, 1); // save game id

	do {
		frread(&fr, &data, 1); // get save player name characters
		*(namePtr++) = data;
	} while (data);

	frread(&fr, &data, 1); // number of game flags, always 0xFF
	frread(&fr, gameFlags, data);
	frread(&fr, &needChangeScene, 1); // scene index
	frread(&fr, &gameChapter, 1);

	frread(&fr, &_engine->_actor->heroBehaviour, 1);
	_engine->_actor->previousHeroBehaviour = _engine->_actor->heroBehaviour;
	frread(&fr, &sceneHero->life, 1);
	frread(&fr, &inventoryNumKashes, 2);
	frread(&fr, &magicLevelIdx, 1);
	frread(&fr, &inventoryMagicPoints, 1);
	frread(&fr, &inventoryNumLeafsBox, 1);
	frread(&fr, &newHeroX, 2);
	frread(&fr, &newHeroY, 2);
	frread(&fr, &newHeroZ, 2);
	frread(&fr, &sceneHero->angle, 2);
	_engine->_actor->previousHeroAngle = sceneHero->angle;
	frread(&fr, &sceneHero->body, 1);

	frread(&fr, &data, 1); // number of holomap locations, always 0x96
	frread(&fr, holomapFlags, data);

	frread(&fr, &inventoryNumGas, 1);

	frread(&fr, &data, 1); // number of used inventory items, always 0x1C
	frread(&fr, inventoryFlags, data);

	frread(&fr, &inventoryNumLeafs, 1);
	frread(&fr, &usingSabre, 1);

	frclose(&fr);

	currentSceneIdx = -1;
	heroPositionType = kReborn;
}

void GameState::saveGame() {
	FileReader fr;
	uint8 data;

	if (!fropen2(&fr, SAVE_DIR "S9999.LBA", "wb+")) {
		warning("Can't save S9999.LBA saved game!\n");
		return;
	}

	data = 0x03;
	frwrite(&fr, &data, 1, 1);

	data = 0x00;
	frwrite(&fr, "TwinEngineSave", 15, 1);

	data = 0xFF; // number of game flags
	frwrite(&fr, &data, 1, 1);
	frwrite(&fr, gameFlags, 255, 1);

	frwrite(&fr, &currentSceneIdx, 1, 1);
	frwrite(&fr, &gameChapter, 1, 1);
	frwrite(&fr, &_engine->_actor->heroBehaviour, 1, 1);
	frwrite(&fr, &sceneHero->life, 1, 1);
	frwrite(&fr, &inventoryNumKashes, 2, 1);
	frwrite(&fr, &magicLevelIdx, 1, 1);
	frwrite(&fr, &inventoryMagicPoints, 1, 1);
	frwrite(&fr, &inventoryNumLeafsBox, 1, 1);
	frwrite(&fr, &newHeroX, 2, 1);
	frwrite(&fr, &newHeroY, 2, 1);
	frwrite(&fr, &newHeroZ, 2, 1);
	frwrite(&fr, &sceneHero->angle, 2, 1);
	frwrite(&fr, &sceneHero->body, 1, 1);

	data = 0x96; // number of holomap locations
	frwrite(&fr, &data, 1, 1);
	frwrite(&fr, holomapFlags, 150, 1);

	frwrite(&fr, &inventoryNumGas, 1, 1);

	data = 0x1C; // number of inventory items
	frwrite(&fr, &data, 1, 1);
	frwrite(&fr, inventoryFlags, 28, 1);

	frwrite(&fr, &inventoryNumLeafs, 1, 1);
	frwrite(&fr, &usingSabre, 1, 1);

	frclose(&fr);
}

void GameState::processFoundItem(int32 item) {
	int32 itemCameraX, itemCameraY, itemCameraZ; // objectXYZ
	int32 itemX, itemY, itemZ;                   // object2XYZ
	int32 boxTopLeftX, boxTopLeftY, boxBottomRightX, boxBottomRightY;
	int32 textState, quitItem, currentAnimState;
	uint8 *currentAnim;
	AnimTimerDataStruct tmpAnimTimer;

	_engine->_grid->newCameraX = (sceneHero->X + 0x100) >> 9;
	_engine->_grid->newCameraY = (sceneHero->Y + 0x100) >> 8;
	_engine->_grid->newCameraZ = (sceneHero->Z + 0x100) >> 9;

	// Hide hero in scene
	sceneHero->staticFlags.bIsHidden = 1;
	redrawEngineActions(1);
	sceneHero->staticFlags.bIsHidden = 0;

	copyScreen(_engine->frontVideoBuffer, _engine->workVideoBuffer);

	itemCameraX = _engine->_grid->newCameraX << 9;
	itemCameraY = _engine->_grid->newCameraY << 8;
	itemCameraZ = _engine->_grid->newCameraZ << 9;

	renderIsoModel(sceneHero->X - itemCameraX, sceneHero->Y - itemCameraY, sceneHero->Z - itemCameraZ, 0, 0x80, 0, _engine->_actor->bodyTable[sceneHero->entity]);
	_engine->_interface->setClip(renderLeft, renderTop, renderRight, renderBottom);

	itemX = (sceneHero->X + 0x100) >> 9;
	itemY = sceneHero->Y >> 8;
	if (sceneHero->brickShape & 0x7F) {
		itemY++;
	}
	itemZ = (sceneHero->Z + 0x100) >> 9;

	_engine->_grid->drawOverModelActor(itemX, itemY, itemZ);
	flip();

	projectPositionOnScreen(sceneHero->X - itemCameraX, sceneHero->Y - itemCameraY, sceneHero->Z - itemCameraZ);
	projPosY -= 150;

	boxTopLeftX = projPosX - 65;
	boxTopLeftY = projPosY - 65;

	boxBottomRightX = projPosX + 65;
	boxBottomRightY = projPosY + 65;

	playSample(41, 0x1000, 1, 0x80, 0x80, 0x80, -1);

	// process vox play
	{
		int32 tmpLanguageCDId;
		stopMusic();
		tmpLanguageCDId = _engine->cfgfile.LanguageCDId;
		//_engine->cfgfile.LanguageCDId = 0; // comented so we can init vox bank
		initTextBank(2);
		_engine->cfgfile.LanguageCDId = tmpLanguageCDId;
	}

	_engine->_interface->resetClip();
	initText(item);
	initDialogueBox();

	textState = 1;
	quitItem = 0;

	if (_engine->cfgfile.LanguageCDId) {
		initVoxToPlay(item);
	}

	currentAnim = _engine->_animations->animTable[_engine->_animations->getBodyAnimIndex(kFoundItem, 0)];

	tmpAnimTimer = sceneHero->animTimerData;

	_engine->_animations->animBuffer2 += _engine->_animations->stockAnimation(_engine->_animations->animBuffer2, _engine->_actor->bodyTable[sceneHero->entity], &sceneHero->animTimerData);
	if (_engine->_animations->animBuffer1 + 4488 < _engine->_animations->animBuffer2) {
		_engine->_animations->animBuffer2 = _engine->_animations->animBuffer1;
	}

	currentAnimState = 0;

	prepareIsoModel(inventoryTable[item]);
	numOfRedrawBox = 0;

	while (!quitItem) {
		_engine->_interface->resetClip();
		currNumOfRedrawBox = 0;
		blitBackgroundAreas();
		_engine->_interface->drawTransparentBox(boxTopLeftX, boxTopLeftY, boxBottomRightX, boxBottomRightY, 4);

		_engine->_interface->setClip(boxTopLeftX, boxTopLeftY, boxBottomRightX, boxBottomRightY);

		_engine->_menu->itemAngle[item] += 8;

		renderInventoryItem(projPosX, projPosY, inventoryTable[item], _engine->_menu->itemAngle[item], 10000);

		_engine->_menu->drawBox(boxTopLeftX, boxTopLeftY, boxBottomRightX, boxBottomRightY);
		addRedrawArea(boxTopLeftX, boxTopLeftY, boxBottomRightX, boxBottomRightY);
		_engine->_interface->resetClip();
		initEngineProjections();

		if (_engine->_animations->setModelAnimation(currentAnimState, currentAnim, _engine->_actor->bodyTable[sceneHero->entity], &sceneHero->animTimerData)) {
			currentAnimState++; // keyframe
			if (currentAnimState >= _engine->_animations->getNumKeyframes(currentAnim)) {
				currentAnimState = _engine->_animations->getStartKeyframe(currentAnim);
			}
		}

		renderIsoModel(sceneHero->X - itemCameraX, sceneHero->Y - itemCameraY, sceneHero->Z - itemCameraZ, 0, 0x80, 0, _engine->_actor->bodyTable[sceneHero->entity]);
		_engine->_interface->setClip(renderLeft, renderTop, renderRight, renderBottom);
		_engine->_grid->drawOverModelActor(itemX, itemY, itemZ);
		addRedrawArea(renderLeft, renderTop, renderRight, renderBottom);

		if (textState) {
			_engine->_interface->resetClip();
			textState = printText10();
		}

		if (textState == 0 || textState == 2) {
			sdldelay(15);
		}

		flipRedrawAreas();

		readKeys();
		if (skippedKey) {
			if (!textState) {
				quitItem = 1;
			}

			if (textState == 2) {
				textState = 1;
			}
		}

		_engine->lbaTime++;
	}

	while (playVoxSimple(currDialTextEntry)) {
		readKeys();
		if (skipIntro == 1) {
			break;
		}
		delaySkip(1);
	}

	initEngineProjections();
	initTextBank(currentTextBank + 3);

	/*do {
		readKeys();
		delaySkip(1);
	} while (!skipIntro);*/

	if (_engine->cfgfile.LanguageCDId && isSamplePlaying(currDialTextEntry)) {
		stopVox(currDialTextEntry);
	}

	sceneHero->animTimerData = tmpAnimTimer;
}

void GameState::processGameChoices(int32 choiceIdx) {
	int32 i;
	copyScreen(_engine->frontVideoBuffer, _engine->workVideoBuffer);

	gameChoicesSettings[0] = 0;          // Current loaded button (button number)
	gameChoicesSettings[1] = numChoices; // Num of buttons
	gameChoicesSettings[2] = 0;          // Buttons box height
	gameChoicesSettings[3] = currentTextBank + 3;

	if (numChoices > 0) {
		for (i = 0; i < numChoices; i++) {
			gameChoicesSettings[i * 2 + 4] = 0;
			gameChoicesSettings[i * 2 + 5] = gameChoices[i];
		}
	}

	drawAskQuestion(choiceIdx);

	_engine->_menu->processMenu(gameChoicesSettings);
	choiceAnswer = gameChoices[gameChoicesSettings[0]];

	// get right VOX entry index
	if (_engine->cfgfile.LanguageCDId) {
		initVoxToPlay(choiceAnswer);
		while (playVoxSimple(currDialTextEntry))
			;
		stopVox(currDialTextEntry);

		hasHiddenVox = 0;
		voxHiddenIndex = 0;
	}
}

void GameState::processGameoverAnimation() { // makeGameOver
	int32 tmpLbaTime, startLbaTime;

	tmpLbaTime = _engine->lbaTime;

	// workaround to fix hero redraw after drowning
	sceneHero->staticFlags.bIsHidden = 1;
	redrawEngineActions(1);
	sceneHero->staticFlags.bIsHidden = 0;

	// TODO: drawInGameTransBox
	setPalette(paletteRGBA);
	copyScreen(_engine->frontVideoBuffer, _engine->workVideoBuffer);
	uint8 *gameOverPtr = (uint8 *)malloc(_engine->_hqrdepack->hqrEntrySize(HQR_RESS_FILE, RESSHQR_GAMEOVERMDL));
	_engine->_hqrdepack->hqrGetEntry(gameOverPtr, HQR_RESS_FILE, RESSHQR_GAMEOVERMDL);

	if (gameOverPtr) {
		int32 avg, cdot;

		prepareIsoModel(gameOverPtr);
		stopSamples();
		stopMidiMusic(); // stop fade music
		setCameraPosition(320, 240, 128, 200, 200);
		startLbaTime = _engine->lbaTime;
		_engine->_interface->setClip(120, 120, 519, 359);

		while (skipIntro != 1 && (_engine->lbaTime - startLbaTime) <= 0x1F4) {
			readKeys();

			avg = _engine->_collision->getAverageValue(40000, 3200, 500, _engine->lbaTime - startLbaTime);
			cdot = crossDot(1, 1024, 100, (_engine->lbaTime - startLbaTime) % 0x64);
			_engine->_interface->blitBox(120, 120, 519, 359, (int8 *)_engine->workVideoBuffer, 120, 120, (int8 *)_engine->frontVideoBuffer);
			setCameraAngle(0, 0, 0, 0, -cdot, 0, avg);
			renderIsoModel(0, 0, 0, 0, 0, 0, gameOverPtr);
			copyBlockPhys(120, 120, 519, 359);

			_engine->lbaTime++;
			sdldelay(15);
		}

		playSample(37, _engine->getRandomNumber(2000) + 3096, 1, 0x80, 0x80, 0x80, -1);
		_engine->_interface->blitBox(120, 120, 519, 359, (int8 *)_engine->workVideoBuffer, 120, 120, (int8 *)_engine->frontVideoBuffer);
		setCameraAngle(0, 0, 0, 0, 0, 0, 3200);
		renderIsoModel(0, 0, 0, 0, 0, 0, gameOverPtr);
		copyBlockPhys(120, 120, 519, 359);

		delaySkip(2000);

		_engine->_interface->resetClip();
		free(gameOverPtr);
		copyScreen(_engine->workVideoBuffer, _engine->frontVideoBuffer);
		flip();
		initEngineProjections();

		_engine->lbaTime = tmpLbaTime;
	}
}

} // namespace TwinE
