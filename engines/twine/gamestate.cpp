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

#include "twine/gamestate.h"
#include "common/system.h"
#include "twine/animations.h"
#include "twine/collision.h"
#include "common/textconsole.h"
#include "twine/extra.h"
#include "twine/filereader.h"
#include "twine/grid.h"
#include "twine/interface.h"
#include "twine/keyboard.h"
#include "twine/menu.h"
#include "twine/menuoptions.h"
#include "twine/music.h"
#include "twine/redraw.h"
#include "twine/renderer.h"
#include "twine/resources.h"
#include "twine/scene.h"
#include "twine/screens.h"
#include "twine/sound.h"
#include "twine/text.h"
#include "twine/actor.h"
#include "twine/twine.h"

namespace TwinE {

#define SAVE_DIR "save/"

GameState::GameState(TwinEEngine* engine) : _engine(engine) {}

void GameState::initEngineProjections() { // reinitAll1
	_engine->_renderer->setOrthoProjection(311, 240, 512);
	_engine->_renderer->setBaseTranslation(0, 0, 0);
	_engine->_renderer->setBaseRotation(0, 0, 0);
	_engine->_renderer->setLightVector(_engine->_scene->alphaLight, _engine->_scene->betaLight, 0);
}

void GameState::initSceneVars() {
	int32 i;

	_engine->_extra->resetExtras();

	for (i = 0; i < OVERLAY_MAX_ENTRIES; i++) {
		_engine->_redraw->overlayList[i].info0 = -1;
	}

	for (i = 0; i < NUM_SCENES_FLAGS; i++) {
		_engine->_scene->sceneFlags[i] = 0;
	}

	for (i = 0; i < NUM_GAME_FLAGS; i++) {
		gameFlags[i] = 0;
	}

	for (i = 0; i < NUM_INVENTORY_ITEMS; i++) {
		inventoryFlags[i] = 0;
	}

	_engine->_scene->sampleAmbiance[0] = -1;
	_engine->_scene->sampleAmbiance[1] = -1;
	_engine->_scene->sampleAmbiance[2] = -1;
	_engine->_scene->sampleAmbiance[3] = -1;

	_engine->_scene->sampleRepeat[0] = 0;
	_engine->_scene->sampleRepeat[1] = 0;
	_engine->_scene->sampleRepeat[2] = 0;
	_engine->_scene->sampleRepeat[3] = 0;

	_engine->_scene->sampleRound[0] = 0;
	_engine->_scene->sampleRound[1] = 0;
	_engine->_scene->sampleRound[2] = 0;
	_engine->_scene->sampleRound[3] = 0;

	for (i = 0; i < 150; i++) {
		holomapFlags[i] = 0;
	}

	_engine->_scene->sceneNumActors = 0;
	_engine->_scene->sceneNumZones = 0;
	_engine->_scene->sceneNumTracks = 0;

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

	_engine->_scene->sceneHero->body = 0;
	_engine->_scene->sceneHero->life = 50;
	_engine->_scene->sceneHero->talkColor = 4;
}

void GameState::initEngineVars(int32 save) { // reinitAll
	_engine->_interface->resetClip();

	_engine->_scene->alphaLight = 896;
	_engine->_scene->betaLight = 950;
	initEngineProjections();
	initSceneVars();
	initHeroVars();

	_engine->_scene->newHeroX = 0x2000;
	_engine->_scene->newHeroY = 0x1800;
	_engine->_scene->newHeroZ = 0x2000;

	_engine->_scene->currentSceneIdx = -1;
	_engine->_scene->needChangeScene = 0;
	_engine->quitGame = -1;
	_engine->_scene->mecaPinguinIdx = -1;
	_engine->_menuOptions->canShowCredits = 0;

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

	_engine->_text->currentTextBank = 0;
	_engine->_scene->currentlyFollowedActor = 0;
	_engine->_actor->heroBehaviour = 0;
	_engine->_actor->previousHeroAngle = 0;
	_engine->_actor->previousHeroBehaviour = 0;

	if (save == -1) {
		loadGame();
		if (_engine->_scene->newHeroX == -1) {
			_engine->_scene->heroPositionType = kNoPosition;
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

	// TODO: use Common::Serializer here
	frread(&fr, &data, 1); // save game id

	do {
		frread(&fr, &data, 1); // get save player name characters
		*(namePtr++) = data;
	} while (data);

	frread(&fr, &data, 1); // number of game flags, always 0xFF
	frread(&fr, gameFlags, data);
	frread(&fr, &_engine->_scene->needChangeScene, 1); // scene index
	frread(&fr, &gameChapter, 1);

	frread(&fr, &_engine->_actor->heroBehaviour, 1);
	_engine->_actor->previousHeroBehaviour = _engine->_actor->heroBehaviour;
	frread(&fr, &_engine->_scene->sceneHero->life, 1);
	frread(&fr, &inventoryNumKashes, 2);
	frread(&fr, &magicLevelIdx, 1);
	frread(&fr, &inventoryMagicPoints, 1);
	frread(&fr, &inventoryNumLeafsBox, 1);
	frread(&fr, &_engine->_scene->newHeroX, 2);
	frread(&fr, &_engine->_scene->newHeroY, 2);
	frread(&fr, &_engine->_scene->newHeroZ, 2);
	frread(&fr, &_engine->_scene->sceneHero->angle, 2);
	_engine->_actor->previousHeroAngle = _engine->_scene->sceneHero->angle;
	frread(&fr, &_engine->_scene->sceneHero->body, 1);

	frread(&fr, &data, 1); // number of holomap locations, always 150
	frread(&fr, holomapFlags, data);

	frread(&fr, &inventoryNumGas, 1);

	frread(&fr, &data, 1); // number of used inventory items, always 28
	frread(&fr, inventoryFlags, data);

	frread(&fr, &inventoryNumLeafs, 1);
	frread(&fr, &usingSabre, 1);

	frclose(&fr);

	_engine->_scene->currentSceneIdx = -1;
	_engine->_scene->heroPositionType = kReborn;
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

	frwrite(&fr, &_engine->_scene->currentSceneIdx, 1, 1);
	frwrite(&fr, &gameChapter, 1, 1);
	frwrite(&fr, &_engine->_actor->heroBehaviour, 1, 1);
	frwrite(&fr, &_engine->_scene->sceneHero->life, 1, 1);
	frwrite(&fr, &inventoryNumKashes, 2, 1);
	frwrite(&fr, &magicLevelIdx, 1, 1);
	frwrite(&fr, &inventoryMagicPoints, 1, 1);
	frwrite(&fr, &inventoryNumLeafsBox, 1, 1);
	frwrite(&fr, &_engine->_scene->newHeroX, 2, 1);
	frwrite(&fr, &_engine->_scene->newHeroY, 2, 1);
	frwrite(&fr, &_engine->_scene->newHeroZ, 2, 1);
	frwrite(&fr, &_engine->_scene->sceneHero->angle, 2, 1);
	frwrite(&fr, &_engine->_scene->sceneHero->body, 1, 1);

	data = 150; // number of holomap locations
	frwrite(&fr, &data, 1, 1);
	frwrite(&fr, holomapFlags, 150, 1);

	frwrite(&fr, &inventoryNumGas, 1, 1);

	data = 28; // number of inventory items
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

	_engine->_grid->newCameraX = (_engine->_scene->sceneHero->X + 0x100) >> 9;
	_engine->_grid->newCameraY = (_engine->_scene->sceneHero->Y + 0x100) >> 8;
	_engine->_grid->newCameraZ = (_engine->_scene->sceneHero->Z + 0x100) >> 9;

	// Hide hero in scene
	_engine->_scene->sceneHero->staticFlags.bIsHidden = 1;
	_engine->_redraw->redrawEngineActions(1);
	_engine->_scene->sceneHero->staticFlags.bIsHidden = 0;

	_engine->_screens->copyScreen(_engine->frontVideoBuffer, _engine->workVideoBuffer);

	itemCameraX = _engine->_grid->newCameraX << 9;
	itemCameraY = _engine->_grid->newCameraY << 8;
	itemCameraZ = _engine->_grid->newCameraZ << 9;

	_engine->_renderer->renderIsoModel(_engine->_scene->sceneHero->X - itemCameraX, _engine->_scene->sceneHero->Y - itemCameraY, _engine->_scene->sceneHero->Z - itemCameraZ, 0, 0x80, 0, _engine->_actor->bodyTable[_engine->_scene->sceneHero->entity]);
	_engine->_interface->setClip(_engine->_redraw->renderLeft, _engine->_redraw->renderTop, _engine->_redraw->renderRight, _engine->_redraw->renderBottom);

	itemX = (_engine->_scene->sceneHero->X + 0x100) >> 9;
	itemY = _engine->_scene->sceneHero->Y >> 8;
	if (_engine->_scene->sceneHero->brickShape & 0x7F) {
		itemY++;
	}
	itemZ = (_engine->_scene->sceneHero->Z + 0x100) >> 9;

	_engine->_grid->drawOverModelActor(itemX, itemY, itemZ);
	flip();

	_engine->_renderer->projectPositionOnScreen(_engine->_scene->sceneHero->X - itemCameraX, _engine->_scene->sceneHero->Y - itemCameraY, _engine->_scene->sceneHero->Z - itemCameraZ);
	_engine->_renderer->projPosY -= 150;

	boxTopLeftX = _engine->_renderer->projPosX - 65;
	boxTopLeftY = _engine->_renderer->projPosY - 65;

	boxBottomRightX = _engine->_renderer->projPosX + 65;
	boxBottomRightY = _engine->_renderer->projPosY + 65;

	_engine->_sound->playSample(41, 0x1000, 1, 0x80, 0x80, 0x80, -1);

	// process vox play
	{
		int32 tmpLanguageCDId;
		_engine->_music->stopMusic();
		tmpLanguageCDId = _engine->cfgfile.LanguageCDId;
		//_engine->cfgfile.LanguageCDId = 0; // comented so we can init vox bank
		_engine->_text->initTextBank(2);
		_engine->cfgfile.LanguageCDId = tmpLanguageCDId;
	}

	_engine->_interface->resetClip();
	_engine->_text->initText(item);
	_engine->_text->initDialogueBox();

	textState = 1;
	quitItem = 0;

	if (_engine->cfgfile.LanguageCDId) {
		_engine->_text->initVoxToPlay(item);
	}

	currentAnim = _engine->_animations->animTable[_engine->_animations->getBodyAnimIndex(kFoundItem, 0)];

	tmpAnimTimer = _engine->_scene->sceneHero->animTimerData;

	_engine->_animations->animBuffer2 += _engine->_animations->stockAnimation(_engine->_animations->animBuffer2, _engine->_actor->bodyTable[_engine->_scene->sceneHero->entity], &_engine->_scene->sceneHero->animTimerData);
	if (_engine->_animations->animBuffer1 + 4488 < _engine->_animations->animBuffer2) {
		_engine->_animations->animBuffer2 = _engine->_animations->animBuffer1;
	}

	currentAnimState = 0;

	_engine->_renderer->prepareIsoModel(_engine->_resources->inventoryTable[item]);
	_engine->_redraw->numOfRedrawBox = 0;

	while (!quitItem) {
		_engine->_interface->resetClip();
		_engine->_redraw->currNumOfRedrawBox = 0;
		_engine->_redraw->blitBackgroundAreas();
		_engine->_interface->drawTransparentBox(boxTopLeftX, boxTopLeftY, boxBottomRightX, boxBottomRightY, 4);

		_engine->_interface->setClip(boxTopLeftX, boxTopLeftY, boxBottomRightX, boxBottomRightY);

		_engine->_menu->itemAngle[item] += 8;

		_engine->_renderer->renderInventoryItem(_engine->_renderer->projPosX, _engine->_renderer->projPosY, _engine->_resources->inventoryTable[item], _engine->_menu->itemAngle[item], 10000);

		_engine->_menu->drawBox(boxTopLeftX, boxTopLeftY, boxBottomRightX, boxBottomRightY);
		_engine->_redraw->addRedrawArea(boxTopLeftX, boxTopLeftY, boxBottomRightX, boxBottomRightY);
		_engine->_interface->resetClip();
		initEngineProjections();

		if (_engine->_animations->setModelAnimation(currentAnimState, currentAnim, _engine->_actor->bodyTable[_engine->_scene->sceneHero->entity], &_engine->_scene->sceneHero->animTimerData)) {
			currentAnimState++; // keyframe
			if (currentAnimState >= _engine->_animations->getNumKeyframes(currentAnim)) {
				currentAnimState = _engine->_animations->getStartKeyframe(currentAnim);
			}
		}

		_engine->_renderer->renderIsoModel(_engine->_scene->sceneHero->X - itemCameraX, _engine->_scene->sceneHero->Y - itemCameraY, _engine->_scene->sceneHero->Z - itemCameraZ, 0, 0x80, 0, _engine->_actor->bodyTable[_engine->_scene->sceneHero->entity]);
		_engine->_interface->setClip(_engine->_redraw->renderLeft, _engine->_redraw->renderTop, _engine->_redraw->renderRight, _engine->_redraw->renderBottom);
		_engine->_grid->drawOverModelActor(itemX, itemY, itemZ);
		_engine->_redraw->addRedrawArea(_engine->_redraw->renderLeft, _engine->_redraw->renderTop, _engine->_redraw->renderRight, _engine->_redraw->renderBottom);

		if (textState) {
			_engine->_interface->resetClip();
			textState = _engine->_text->printText10();
		}

		if (textState == 0 || textState == 2) {
			_engine->_system->delayMillis(15);
		}

		_engine->_redraw->flipRedrawAreas();

		readKeys();
		if (_engine->_keyboard.skippedKey) {
			if (!textState) {
				quitItem = 1;
			}

			if (textState == 2) {
				textState = 1;
			}
		}

		_engine->lbaTime++;
	}

	while (_engine->_text->playVoxSimple(_engine->_text->currDialTextEntry)) {
		readKeys();
		if (_engine->_keyboard.skipIntro == 1) {
			break;
		}
		delaySkip(1);
	}

	initEngineProjections();
	_engine->_text->initTextBank(_engine->_text->currentTextBank + 3);

	/*do {
		readKeys();
		delaySkip(1);
	} while (!skipIntro);*/

	if (_engine->cfgfile.LanguageCDId && _engine->_sound->isSamplePlaying(_engine->_text->currDialTextEntry)) {
		_engine->_text->stopVox(_engine->_text->currDialTextEntry);
	}

	_engine->_scene->sceneHero->animTimerData = tmpAnimTimer;
}

void GameState::processGameChoices(int32 choiceIdx) {
	int32 i;
	_engine->_screens->copyScreen(_engine->frontVideoBuffer, _engine->workVideoBuffer);

	gameChoicesSettings[0] = 0;          // Current loaded button (button number)
	gameChoicesSettings[1] = numChoices; // Num of buttons
	gameChoicesSettings[2] = 0;          // Buttons box height
	gameChoicesSettings[3] = _engine->_text->currentTextBank + 3;

	if (numChoices > 0) {
		for (i = 0; i < numChoices; i++) {
			gameChoicesSettings[i * 2 + 4] = 0;
			gameChoicesSettings[i * 2 + 5] = gameChoices[i];
		}
	}

	_engine->_text->drawAskQuestion(choiceIdx);

	_engine->_menu->processMenu(gameChoicesSettings);
	choiceAnswer = gameChoices[gameChoicesSettings[0]];

	// get right VOX entry index
	if (_engine->cfgfile.LanguageCDId) {
		_engine->_text->initVoxToPlay(choiceAnswer);
		while (_engine->_text->playVoxSimple(_engine->_text->currDialTextEntry))
			;
		_engine->_text->stopVox(_engine->_text->currDialTextEntry);

		_engine->_text->hasHiddenVox = 0;
		_engine->_text->voxHiddenIndex = 0;
	}
}

void GameState::processGameoverAnimation() { // makeGameOver
	int32 tmpLbaTime, startLbaTime;

	tmpLbaTime = _engine->lbaTime;

	// workaround to fix hero redraw after drowning
	_engine->_scene->sceneHero->staticFlags.bIsHidden = 1;
	_engine->_redraw->redrawEngineActions(1);
	_engine->_scene->sceneHero->staticFlags.bIsHidden = 0;

	// TODO: drawInGameTransBox
	setPalette(_engine->_screens->paletteRGBA);
	_engine->_screens->copyScreen(_engine->frontVideoBuffer, _engine->workVideoBuffer);
	uint8 *gameOverPtr = (uint8 *)malloc(_engine->_hqrdepack->hqrEntrySize(HQR_RESS_FILE, RESSHQR_GAMEOVERMDL));
	_engine->_hqrdepack->hqrGetEntry(gameOverPtr, HQR_RESS_FILE, RESSHQR_GAMEOVERMDL);

	if (gameOverPtr) {
		int32 avg, cdot;

		_engine->_renderer->prepareIsoModel(gameOverPtr);
		_engine->_sound->stopSamples();
		_engine->_music->stopMidiMusic(); // stop fade music
		_engine->_renderer->setCameraPosition(320, 240, 128, 200, 200);
		startLbaTime = _engine->lbaTime;
		_engine->_interface->setClip(120, 120, 519, 359);

		while (_engine->_keyboard.skipIntro != 1 && (_engine->lbaTime - startLbaTime) <= 0x1F4) {
			readKeys();

			avg = _engine->_collision->getAverageValue(40000, 3200, 500, _engine->lbaTime - startLbaTime);
			cdot = _engine->_screens->crossDot(1, 1024, 100, (_engine->lbaTime - startLbaTime) % 0x64);
			_engine->_interface->blitBox(120, 120, 519, 359, (int8 *)_engine->workVideoBuffer, 120, 120, (int8 *)_engine->frontVideoBuffer);
			_engine->_renderer->setCameraAngle(0, 0, 0, 0, -cdot, 0, avg);
			_engine->_renderer->renderIsoModel(0, 0, 0, 0, 0, 0, gameOverPtr);
			copyBlockPhys(120, 120, 519, 359);

			_engine->lbaTime++;
			_engine->_system->delayMillis(15);
		}

		_engine->_sound->playSample(37, _engine->getRandomNumber(2000) + 3096, 1, 0x80, 0x80, 0x80, -1);
		_engine->_interface->blitBox(120, 120, 519, 359, (int8 *)_engine->workVideoBuffer, 120, 120, (int8 *)_engine->frontVideoBuffer);
		_engine->_renderer->setCameraAngle(0, 0, 0, 0, 0, 0, 3200);
		_engine->_renderer->renderIsoModel(0, 0, 0, 0, 0, 0, gameOverPtr);
		copyBlockPhys(120, 120, 519, 359);

		delaySkip(2000);

		_engine->_interface->resetClip();
		free(gameOverPtr);
		_engine->_screens->copyScreen(_engine->workVideoBuffer, _engine->frontVideoBuffer);
		flip();
		initEngineProjections();

		_engine->lbaTime = tmpLbaTime;
	}
}

} // namespace TwinE
