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

#include "menu.h"
#include "actor.h"
#include "animations.h"
#include "common/scummsys.h"
#include "gamestate.h"
#include "grid.h"
#include "hqrdepack.h"
#include "interface.h"
#include "keyboard.h"
#include "menuoptions.h"
#include "movements.h"
#include "music.h"
#include "redraw.h"
#include "renderer.h"
#include "resources.h"
#include "scene.h"
#include "screens.h"
#include "sdlengine.h"
#include "sound.h"
#include "text.h"
#include "twine.h"

namespace TwinE {

/** Main menu background image number
	Used when returning from credit sequence to redraw the main menu background image */
uint32 kPlasmaEffectFilesize = 262176;

/** Menu buttons width */
uint16 kMainMenuButtonWidth = 320;
/** Used to calculate the spanning between button and screen */
uint16 kMainMenuButtonSpan = 550;

/** Main menu types */
enum MainMenuType {
	kNewGame = 20,
	kContinueGame = 21,
	kOptions = 23,
	kQuit = 22,
	kBackground = 9999
};

/** Give up menu types */
enum GiveUpMenuType {
	kContinue = 28,
	kGiveUp = 27 // quit
};

/** Options menu types */
enum OptionsMenuType {
	kReturnGame = 15,
	kReturnMenu = 26,
	kVolume = 30,
	kSaveManage = 46,
	kAdvanced = 47
};

/** Volume menu types */
enum VolumeMenuType {
	kMusicVolume = 1,
	kSoundVolume = 2,
	kCDVolume = 3,
	kLineVolume = 4,
	kMasterVolume = 5
};

/** Main Menu Settings

	Used to create the game main menu. */
int16 MainMenuSettings[] = {
    0,   // Current loaded button (button number)
    4,   // Num of buttons
    200, // Buttons box height ( is used to calc the height where the first button will appear )
    0,   // unused
    0,
    20, // new game
    0,
    21, // continue game
    0,
    23, // options
    0,
    22, // quit
};

/** Give Up Menu Settings

	Used to create the in-game menu. */
int16 GiveUpMenuSettings[] = {
    0,   // Current loaded button (button number)
    2,   // Num of buttons
    240, // Buttons box height ( is used to calc the height where the first button will appear )
    0,   // unused
    0,
    28, // continue game
    0,
    27, // quit game
};

/** Give Up Menu Settings

	Used to create the in-game menu. This menu have one extra item to save the game */
int16 GiveUpMenuSettingsWithSave[] = {
    0,   // Current loaded button (button number)
    3,   // Num of buttons
    240, // Buttons box height ( is used to calc the height where the first button will appear )
    0,   // unused
    0,
    28, // continue game
    0,
    16, // save game
    0,
    27, // quit game
};

/** Options Menu Settings

	Used to create the options menu. */
int16 Menu::OptionsMenuSettings[] = {
    0, // Current loaded button (button number)
    4, // Num of buttons
    0, // Buttons box height ( is used to calc the height where the first button will appear )
    0, // unused
    0,
    24, // return to previous menu
    0,
    30, // volume settings
    0,
    46, // save game management
    0,
    47, // advanced options
};

/** Advanced Options Menu Settings

	Used to create the advanced options menu. */
int16 AdvOptionsMenuSettings[] = {
    0, // Current loaded button (button number)
    5, // Num of buttons
    0, // Buttons box height ( is used to calc the height where the first button will appear )
    0, // unused
    0,
    26, // return to main menu
    0,
    4, // aggressive mode (manual|auto)
    6,
    31, // Polygon detail (full|medium|low)
    7,
    32, // Shadows (all|character|no)
    8,
    33, // scenary zoon (on|off)
};

/** Save Game Management Menu Settings

	Used to create the save game management menu. */
int16 SaveManageMenuSettings[] = {
    0, // Current loaded button (button number)
    3, // Num of buttons
    0, // Buttons box height ( is used to calc the height where the first button will appear )
    0, // unused
    0,
    26, // return to main menu
    0,
    41, // copy saved game
    0,
    45, // delete saved game
};

/** Volume Menu Settings

	Used to create the volume menu. */
int16 VolumeMenuSettings[] = {
    0, // Current loaded button (button number)
    7, // Num of buttons
    0, // Buttons box height ( is used to calc the height where the first button will appear )
    0, // unused
    0,
    26, // return to main menu
    1,
    10, // music volume
    2,
    11, // sfx volume
    3,
    12, // cd volume
    4,
    13, // line-in volume
    5,
    14, // master volume
    0,
    16, // save parameters
};

#define PLASMA_WIDTH 320
#define PLASMA_HEIGHT 50
#define SCREEN_W 640

Menu::Menu(TwinEEngine *engine) : _engine(engine) {}

void Menu::plasmaEffectRenderFrame() {
	int16 c;
	int32 i, j;
	uint8 *dest;
	uint8 *src;

	for (j = 1; j < PLASMA_HEIGHT - 1; j++) {
		for (i = 1; i < PLASMA_WIDTH - 1; i++) {
			/*Here we calculate the average of all 8 neighbour pixel values*/

			c = plasmaEffectPtr[(i - 1) + (j - 1) * PLASMA_WIDTH];  //top-left
			c += plasmaEffectPtr[(i + 0) + (j - 1) * PLASMA_WIDTH]; //top
			c += plasmaEffectPtr[(i + 1) + (j - 1) * PLASMA_WIDTH]; //top-right

			c += plasmaEffectPtr[(i - 1) + (j + 0) * PLASMA_WIDTH]; //left
			c += plasmaEffectPtr[(i + 1) + (j + 0) * PLASMA_WIDTH]; //right

			c += plasmaEffectPtr[(i - 1) + (j + 1) * PLASMA_WIDTH]; // bottom-left
			c += plasmaEffectPtr[(i + 0) + (j + 1) * PLASMA_WIDTH]; // bottom
			c += plasmaEffectPtr[(i + 1) + (j + 1) * PLASMA_WIDTH]; // bottom-right

			c = (c >> 3) | ((c & 0x0003) << 13); /* And the 2 least significant bits are used as a
              randomizing parameter for statistically fading the flames */

			if (!(c & 0x6500) &&
			    (j >= (PLASMA_HEIGHT - 4) || c > 0)) {
				c--; /*fade this pixel*/
			}

			/* plot the pixel using the calculated color */
			plasmaEffectPtr[i + (PLASMA_HEIGHT + j) * PLASMA_WIDTH] = (uint8)c;
		}
	}

	// flip the double-buffer while scrolling the effect vertically:
	dest = plasmaEffectPtr;
	src = plasmaEffectPtr + (PLASMA_HEIGHT + 1) * PLASMA_WIDTH;
	for (i = 0; i < PLASMA_HEIGHT * PLASMA_WIDTH; i++)
		*(dest++) = *(src++);
}

/** Process the plasma effect
	@param top top height where the effect will be draw in the front buffer
	@param color plasma effect start color */
void Menu::processPlasmaEffect(int32 top, int32 color) {
	uint8 *in;
	uint8 *out;
	int32 i, j, target;
	uint8 c;
	uint8 max_value = color + 15;

	plasmaEffectRenderFrame();

	in = plasmaEffectPtr + 5 * PLASMA_WIDTH;
	out = _engine->frontVideoBuffer + _engine->screenLookupTable[top];

	for (i = 0; i < 25; i++) {
		for (j = 0; j < kMainMenuButtonWidth; j++) {
			c = in[i * kMainMenuButtonWidth + j] / 2 + color;
			if (c > max_value)
				c = max_value;

			/* 2x2 squares sharing the same pixel color: */
			target = 2 * (i * SCREEN_W + j);
			out[target] = c;
			out[target + 1] = c;
			out[target + SCREEN_W] = c;
			out[target + SCREEN_W + 1] = c;
		}
	}
}

/** Draw the entire button box
	@param left start width to draw the button
	@param top start height to draw the button
	@param right end width to draw the button
	@param bottom end height to draw the button */
void Menu::drawBox(int32 left, int32 top, int32 right, int32 bottom) {
	_engine->_interface->drawLine(left, top, right, top, 79);         // top line
	_engine->_interface->drawLine(left, top, left, bottom, 79);       // left line
	_engine->_interface->drawLine(right, ++top, right, bottom, 73);   // right line
	_engine->_interface->drawLine(++left, bottom, right, bottom, 73); // bottom line
}

/** Draws main menu button
	@param width menu button width
	@param topheight is the height between the top of the screen and the first button
	@param id current button identification from menu settings
	@param value current button key pressed value
	@param mode flag to know if should draw as a hover button or not */
void Menu::drawButtonGfx(int32 width, int32 topheight, int32 id, int32 value, int32 mode) {
	int32 right;
	int32 top;
	int32 left;
	int32 bottom2;
	int32 bottom;
	int32 textSize;
	int8 dialText[256];
	/*
	 * int CDvolumeRemaped; int musicVolumeRemaped; int masterVolumeRemaped; int lineVolumeRemaped;
	 * int waveVolumeRemaped;
	 */

	memset(dialText, 0, sizeof(dialText));

	left = width - kMainMenuButtonSpan / 2;
	right = width + kMainMenuButtonSpan / 2;

	// topheigh is the center Y pos of the button
	top = topheight - 25;              // this makes the button be 50 height
	bottom = bottom2 = topheight + 25; // ||

	if (mode != 0) {
		if (id <= 5 && id >= 1) {
			int32 newWidth = 0;

			switch (id) {
			case 1: {
				if (_engine->cfgfile.MusicVolume > 255)
					_engine->cfgfile.MusicVolume = 255;
				if (_engine->cfgfile.MusicVolume < 0)
					_engine->cfgfile.MusicVolume = 0;
				newWidth = _engine->_screens->crossDot(left, right, 255, _engine->cfgfile.MusicVolume);
				break;
			}
			case 2: {
				if (_engine->cfgfile.WaveVolume > 255)
					_engine->cfgfile.WaveVolume = 255;
				if (_engine->cfgfile.WaveVolume < 0)
					_engine->cfgfile.WaveVolume = 0;
				newWidth = _engine->_screens->crossDot(left, right, 255, _engine->cfgfile.WaveVolume);
				break;
			}
			case 3: {
				if (_engine->cfgfile.CDVolume > 255)
					_engine->cfgfile.CDVolume = 255;
				if (_engine->cfgfile.CDVolume < 0)
					_engine->cfgfile.CDVolume = 0;
				newWidth = _engine->_screens->crossDot(left, right, 255, _engine->cfgfile.CDVolume);
				break;
			}
			case 4: {
				if (_engine->cfgfile.LineVolume > 255)
					_engine->cfgfile.LineVolume = 255;
				if (_engine->cfgfile.LineVolume < 0)
					_engine->cfgfile.LineVolume = 0;
				newWidth = _engine->_screens->crossDot(left, right, 255, _engine->cfgfile.LineVolume);
				break;
			}
			case 5: {
				if (_engine->cfgfile.MasterVolume > 255)
					_engine->cfgfile.MasterVolume = 255;
				if (_engine->cfgfile.MasterVolume < 0)
					_engine->cfgfile.MasterVolume = 0;
				newWidth = _engine->_screens->crossDot(left, right, 255, _engine->cfgfile.MasterVolume);
				break;
			}
			};

			processPlasmaEffect(top, 80);
			if (!(_engine->getRandomNumber() % 5)) {
				plasmaEffectPtr[_engine->getRandomNumber() % 140 * 10 + 1900] = 255;
			}
			_engine->_interface->drawSplittedBox(newWidth, top, right, bottom, 68);
		} else {
			processPlasmaEffect(top, 64);
			if (!(_engine->getRandomNumber() % 5)) {
				plasmaEffectPtr[_engine->getRandomNumber() % 320 * 10 + 6400] = 255;
			}
		}

		if (id <= 5 && id >= 1) {
			// implement this
		}
	} else {
		_engine->_interface->blitBox(left, top, right, bottom, (int8 *)_engine->workVideoBuffer, left, top, (int8 *)_engine->frontVideoBuffer);
		drawTransparentBox(left, top, right, bottom2, 4);
	}

	drawBox(left, top, right, bottom);

	_engine->_text->setFontColor(15);
	_engine->_text->setFontParameters(2, 8);
	_engine->_text->getMenuText(value, dialText);
	textSize = _engine->_text->getTextSize(dialText);
	_engine->_text->drawText(width - (textSize / 2), topheight - 18, dialText);

	// TODO: make volume buttons

	copyBlockPhys(left, top, right, bottom);
}

/** Process the menu button draw
	@param data menu settings array
	@param mode flag to know if should draw as a hover button or not */
void Menu::drawButton(int16 *menuSettings, int32 mode) {
	int32 buttonNumber;
	int32 maxButton;
	int16 *localData = menuSettings;
	int32 topHeight;
	uint8 menuItemId;
	uint16 menuItemValue; // applicable for sound menus, to save the volume/sound bar
	int8 currentButton;

	buttonNumber = *localData;
	localData += 1;
	maxButton = *localData;
	localData += 1;
	topHeight = *localData;
	localData += 2;

	if (topHeight == 0) {
		topHeight = 35;
	} else {
		topHeight = topHeight - (((maxButton - 1) * 6) + ((maxButton)*50)) / 2;
	}

	if (maxButton <= 0) {
		return;
	}

	currentButton = 0;

	do {
		// get menu item settings
		menuItemId = (uint8)*localData;
		localData += 1;
		menuItemValue = *localData;
		localData += 1;
		if (mode != 0) {
			if (currentButton == buttonNumber) {
				drawButtonGfx(kMainMenuButtonWidth, topHeight, menuItemId, menuItemValue, 1);
			}
		} else {
			if (currentButton == buttonNumber) {
				drawButtonGfx(kMainMenuButtonWidth, topHeight, menuItemId, menuItemValue, 1);
			} else {
				drawButtonGfx(kMainMenuButtonWidth, topHeight, menuItemId, menuItemValue, 0);
			}
		}

		currentButton++;
		topHeight += 56; // increase button top height

		// slow down the CPU
		sdldelay(1);
	} while (currentButton < maxButton);
}

/** Where the main menu options are processed
	@param menuSettings menu settings array with the information to build the menu options
	@return pressed menu button identification */
int32 Menu::processMenu(int16 *menuSettings) {
	int32 localTime;
	int32 numEntry;
	int32 buttonNeedRedraw;
	int32 maxButton;
	int16 *localData = menuSettings;
	int16 currentButton;
	int16 id;
	int32 musicChanged;
	int32 buttonReleased = 1;

	musicChanged = 0;

	buttonNeedRedraw = 1;

	numEntry = localData[1];
	currentButton = 0; // localData[0];
	localTime = _engine->lbaTime;
	maxButton = numEntry - 1;

	readKeys();

	do {
		// if its on main menu
		if (localData == MainMenuSettings) {
			if (_engine->lbaTime - localTime <= 11650) {
				if (skipIntro == 46)
					if (skippedKey != 32)
						return kBackground;
			} else {
				return kBackground;
			}
		}

		if (pressedKey == 0) {
			buttonReleased = 1;
		}

		if (buttonReleased) {
			key = pressedKey;

			if (((uint8)key & 2)) { // on arrow key down
				currentButton++;
				if (currentButton == numEntry) { // if current button is the last, than next button is the first
					currentButton = 0;
				}
				buttonNeedRedraw = 1;
				buttonReleased = 0;
			}

			if (((uint8)key & 1)) { // on arrow key up
				currentButton--;
				if (currentButton < 0) { // if current button is the first, than previous button is the last
					currentButton = maxButton;
				}
				buttonNeedRedraw = 1;
				buttonReleased = 0;
			}

			if (*(localData + 8) <= 5) {                   // if its a volume button
				id = *(localData + currentButton * 2 + 4); // get button parameters from settings array

				switch (id) {
				case kMusicVolume: {
					if (((uint8)key & 4)) { // on arrow key left
						_engine->cfgfile.MusicVolume -= 4;
					}
					if (((uint8)key & 8)) { // on arrow key right
						_engine->cfgfile.MusicVolume += 4;
					}
					_engine->_music->musicVolume(_engine->cfgfile.MusicVolume);
					break;
				}
				case kSoundVolume: {
					if (((uint8)key & 4)) { // on arrow key left
						_engine->cfgfile.WaveVolume -= 4;
					}
					if (((uint8)key & 8)) { // on arrow key right
						_engine->cfgfile.WaveVolume += 4;
					}
					_engine->_sound->sampleVolume(-1, _engine->cfgfile.WaveVolume);
					break;
				}
				case kCDVolume: {
					if (((uint8)key & 4)) { // on arrow key left
						_engine->cfgfile.CDVolume -= 4;
					}
					if (((uint8)key & 8)) { // on arrow key right
						_engine->cfgfile.CDVolume += 4;
					}
					break;
				}
				case kLineVolume: {
					if (((uint8)key & 4)) { // on arrow key left
						_engine->cfgfile.LineVolume -= 4;
					}
					if (((uint8)key & 8)) { // on arrow key right
						_engine->cfgfile.LineVolume += 4;
					}
					break;
				}
				case kMasterVolume: {
					if (((uint8)key & 4)) { // on arrow key left
						_engine->cfgfile.MasterVolume -= 4;
					}
					if (((uint8)key & 8)) { // on arrow key right
						_engine->cfgfile.MasterVolume += 4;
					}
					_engine->_music->musicVolume(_engine->cfgfile.MusicVolume);
					_engine->_sound->sampleVolume(-1, _engine->cfgfile.WaveVolume);
					break;
				}
				default:
					break;
				}
			}
		}

		if (buttonNeedRedraw == 1) {
			*localData = currentButton;

			drawButton(localData, 0); // current button
			do {
				readKeys();
				drawButton(localData, 1);
			} while (pressedKey == 0 && skippedKey == 0 && skipIntro == 0);
			buttonNeedRedraw = 0;
		} else {
			if (musicChanged) {
				// TODO: update volume settings
			}

			buttonNeedRedraw = 0;
			drawButton(localData, 1);
			readKeys();
			// WARNING: this is here to prevent a fade bug while quit the menu
			_engine->_screens->copyScreen(_engine->workVideoBuffer, _engine->frontVideoBuffer);
		}
	} while (!(skippedKey & 2) && !(skippedKey & 1));

	currentButton = *(localData + 5 + currentButton * 2); // get current browsed button

	readKeys();

	return currentButton;
}

/** Used to run the advanced options menu */
int32 Menu::advoptionsMenu() {
	int32 ret = 0;

	_engine->_screens->copyScreen(_engine->workVideoBuffer, _engine->frontVideoBuffer);

	do {
		switch (processMenu(AdvOptionsMenuSettings)) {
		case kReturnMenu: {
			ret = 1; // quit option menu
			break;
		}
		//TODO: add other options
		default:
			break;
		}
	} while (ret != 1);

	_engine->_screens->copyScreen(_engine->workVideoBuffer, _engine->frontVideoBuffer);
	flip();

	return 0;
}

/** Used to run the save game management menu */
int32 Menu::savemanageMenu() {
	int32 ret = 0;

	_engine->_screens->copyScreen(_engine->workVideoBuffer, _engine->frontVideoBuffer);

	do {
		switch (processMenu(SaveManageMenuSettings)) {
		case kReturnMenu: {
			ret = 1; // quit option menu
			break;
		}
		//TODO: add other options
		default:
			break;
		}
	} while (ret != 1);

	_engine->_screens->copyScreen(_engine->workVideoBuffer, _engine->frontVideoBuffer);
	flip();

	return 0;
}

/** Used to run the volume menu */
int32 Menu::volumeMenu() {
	int32 ret = 0;

	_engine->_screens->copyScreen(_engine->workVideoBuffer, _engine->frontVideoBuffer);

	do {
		switch (processMenu(VolumeMenuSettings)) {
		case kReturnMenu: {
			ret = 1; // quit option menu
			break;
		}
		//TODO: add other options
		default:
			break;
		}
	} while (ret != 1);

	_engine->_screens->copyScreen(_engine->workVideoBuffer, _engine->frontVideoBuffer);
	flip();

	return 0;
}

/** Used to run the options menu */
int32 Menu::optionsMenu() {
	int32 ret = 0;

	_engine->_screens->copyScreen(_engine->workVideoBuffer, _engine->frontVideoBuffer);

	_engine->_sound->stopSamples();
	//_engine->_music->playCDtrack(9);

	do {
		switch (processMenu(OptionsMenuSettings)) {
		case kReturnGame:
		case kReturnMenu: {
			ret = 1; // quit option menu
			break;
		}
		case kVolume: {
			_engine->_screens->copyScreen(_engine->workVideoBuffer, _engine->frontVideoBuffer);
			flip();
			volumeMenu();
			break;
		}
		case kSaveManage: {
			_engine->_screens->copyScreen(_engine->workVideoBuffer, _engine->frontVideoBuffer);
			flip();
			savemanageMenu();
			break;
		}
		case kAdvanced: {
			_engine->_screens->copyScreen(_engine->workVideoBuffer, _engine->frontVideoBuffer);
			flip();
			advoptionsMenu();
			break;
		}
		default:
			break;
		}
	} while (ret != 1);

	_engine->_screens->copyScreen(_engine->workVideoBuffer, _engine->frontVideoBuffer);
	flip();

	return 0;
}

/** Used to run the main menu */
void Menu::mainMenu() {
	_engine->_sound->stopSamples();

	_engine->_screens->copyScreen(_engine->frontVideoBuffer, _engine->workVideoBuffer);

	// load menu effect file only once
	plasmaEffectPtr = (uint8 *)malloc(kPlasmaEffectFilesize);
	memset(plasmaEffectPtr, 0, kPlasmaEffectFilesize);
	_engine->_hqrdepack->hqrGetEntry(plasmaEffectPtr, HQR_RESS_FILE, RESSHQR_PLASMAEFFECT);

	while (!_engine->cfgfile.Quit) {
		_engine->_text->initTextBank(0);

		_engine->_music->playTrackMusic(9); // LBA's Theme
		_engine->_sound->stopSamples();

		switch (processMenu(MainMenuSettings)) {
		case kNewGame: {
			_engine->_menuOptions->newGameMenu();
			break;
		}
		case kContinueGame: {
			_engine->_menuOptions->continueGameMenu();
			break;
		}
		case kOptions: {
			_engine->_screens->copyScreen(_engine->workVideoBuffer, _engine->frontVideoBuffer);
			flip();
			OptionsMenuSettings[5] = kReturnMenu;
			optionsMenu();
			break;
		}
		case kQuit: {
			_engine->cfgfile.Quit = 1;
			break;
		}
		case kBackground: {
			_engine->_screens->loadMenuImage(1);
		}
		}
		fpsCycles(_engine->cfgfile.Fps);
	}
}

/** Used to process give up menu while playing game */
int32 Menu::giveupMenu() {
	//int32 saveLangue=0;
	int32 menuId;
	int16 *localMenu;

	_engine->_screens->copyScreen(_engine->frontVideoBuffer, _engine->workVideoBuffer);
	_engine->_sound->pauseSamples();

	if (_engine->cfgfile.UseAutoSaving == 1)
		localMenu = GiveUpMenuSettings;
	else
		localMenu = GiveUpMenuSettingsWithSave;

	do {
		//saveLangue = languageCD1;
		//languageCD1 = 0;
		_engine->_text->initTextBank(0);

		menuId = processMenu(localMenu);

		//languageCD1 = saveLangue;

		_engine->_text->initTextBank(_engine->_text->currentTextBank + 3);

		fpsCycles(_engine->cfgfile.Fps);
	} while (menuId != kGiveUp && menuId != kContinue);

	if (menuId == kGiveUp) {
		_engine->_sound->stopSamples();
		return 1;
	}

	_engine->_sound->resumeSamples();
	return 0;
}

void Menu::drawInfoMenu(int16 left, int16 top) {
	int32 boxLeft, boxTop, boxRight, boxBottom;
	int32 newBoxLeft, newBoxLeft2, i;

	_engine->_interface->resetClip();
	drawBox(left, top, left + 450, top + 80);
	_engine->_interface->drawSplittedBox(left + 1, top + 1, left + 449, top + 79, 0);

	newBoxLeft2 = left + 9;

	_engine->_grid->drawSprite(0, newBoxLeft2, top + 13, _engine->_actor->spriteTable[SPRITEHQR_LIFEPOINTS]);

	boxRight = left + 325;
	newBoxLeft = left + 25;
	boxLeft = _engine->_screens->crossDot(newBoxLeft, boxRight, 50, _engine->_scene->sceneHero->life);

	boxTop = top + 10;
	boxBottom = top + 25;
	_engine->_interface->drawSplittedBox(newBoxLeft, boxTop, boxLeft, boxBottom, 91);
	drawBox(left + 25, top + 10, left + 324, top + 10 + 14);

	if (!_engine->_gameState->gameFlags[GAMEFLAG_INVENTORY_DISABLED] && _engine->_gameState->gameFlags[GAMEFLAG_TUNIC]) {
		_engine->_grid->drawSprite(0, newBoxLeft2, top + 36, _engine->_actor->spriteTable[SPRITEHQR_MAGICPOINTS]);
		if (_engine->_gameState->magicLevelIdx > 0) {
			_engine->_interface->drawSplittedBox(newBoxLeft, top + 35, _engine->_screens->crossDot(newBoxLeft, boxRight, 80, _engine->_gameState->inventoryMagicPoints), top + 50, 75);
		}
		drawBox(left + 25, top + 35, left + _engine->_gameState->magicLevelIdx * 80 + 20, top + 35 + 15);
	}

	boxLeft = left + 340;

	/** draw coin sprite */
	_engine->_grid->drawSprite(0, boxLeft, top + 15, _engine->_actor->spriteTable[SPRITEHQR_KASHES]);
	_engine->_text->setFontColor(155);
	_engine->_text->drawText(left + 370, top + 5, _engine->ITOA(_engine->_gameState->inventoryNumKashes));

	/** draw key sprite */
	_engine->_grid->drawSprite(0, boxLeft, top + 55, _engine->_actor->spriteTable[SPRITEHQR_KEY]);
	_engine->_text->setFontColor(155);
	_engine->_text->drawText(left + 370, top + 40, _engine->ITOA(_engine->_gameState->inventoryNumKeys));

	// prevent
	if (_engine->_gameState->inventoryNumLeafs > _engine->_gameState->inventoryNumLeafsBox) {
		_engine->_gameState->inventoryNumLeafs = _engine->_gameState->inventoryNumLeafsBox;
	}

	// Clover leaf boxes
	for (i = 0; i < _engine->_gameState->inventoryNumLeafsBox; i++) {
		_engine->_grid->drawSprite(0, _engine->_screens->crossDot(left + 25, left + 325, 10, i), top + 58, _engine->_actor->spriteTable[SPRITEHQR_CLOVERLEAFBOX]);
	}

	// Clover leafs
	for (i = 0; i < _engine->_gameState->inventoryNumLeafs; i++) {
		_engine->_grid->drawSprite(0, _engine->_screens->crossDot(left + 25, left + 325, 10, i) + 2, top + 60, _engine->_actor->spriteTable[SPRITEHQR_CLOVERLEAF]);
	}

	copyBlockPhys(left, top, left + 450, top + 135);
}

void Menu::drawBehaviour(int16 behaviour, int32 angle, int16 cantDrawBox) {
	uint8 *currentAnim;
	int32 boxLeft, boxTop, boxRight, boxBottom, currentAnimState;
	int8 dialText[256];

	boxLeft = behaviour * 110 + 110;
	boxRight = boxLeft + 99;
	boxTop = 110;
	boxBottom = 229;

	currentAnim = _engine->_animations->animTable[_engine->_actor->heroAnimIdx[behaviour]];
	currentAnimState = behaviourAnimState[behaviour];

	if (_engine->_animations->setModelAnimation(currentAnimState, currentAnim, behaviourEntity, &behaviourAnimData[behaviour])) {
		currentAnimState++; // keyframe
		if (currentAnimState >= _engine->_animations->getNumKeyframes(currentAnim)) {
			currentAnimState = _engine->_animations->getStartKeyframe(currentAnim);
		}
		behaviourAnimState[behaviour] = currentAnimState;
	}

	if (cantDrawBox == 0) {
		drawBox(boxLeft - 1, boxTop - 1, boxRight + 1, boxBottom + 1);
	}

	_engine->_interface->saveClip();
	_engine->_interface->resetClip();

	if (behaviour != _engine->_actor->heroBehaviour) { // unselected
		_engine->_interface->drawSplittedBox(boxLeft, boxTop, boxRight, boxBottom, 0);
	} else { // selected
		_engine->_interface->drawSplittedBox(boxLeft, boxTop, boxRight, boxBottom, 69);

		// behaviour menu title
		_engine->_interface->drawSplittedBox(110, 239, 540, 279, 0);
		drawBox(110, 239, 540, 279);

		_engine->_text->setFontColor(15);

		if (_engine->_actor->heroBehaviour == 2 && _engine->_actor->autoAgressive == 1) {
			_engine->_text->getMenuText(4, dialText);
		} else {
			_engine->_text->getMenuText(_engine->_actor->heroBehaviour, dialText);
		}

		_engine->_text->drawText((650 - _engine->_text->getTextSize(dialText)) / 2, 240, dialText);
	}

	_engine->_renderer->renderBehaviourModel(boxLeft, boxTop, boxRight, boxBottom, -600, angle, behaviourEntity);

	copyBlockPhys(boxLeft, boxTop, boxRight, boxBottom);
	copyBlockPhys(110, 239, 540, 279);

	_engine->_interface->loadClip();
}

void Menu::drawBehaviourMenu(int32 angle) {
	drawBox(100, 100, 550, 290);
	drawTransparentBox(101, 101, 549, 289, 2);

	_engine->_animations->setAnimAtKeyframe(behaviourAnimState[kNormal], _engine->_animations->animTable[_engine->_actor->heroAnimIdx[kNormal]], behaviourEntity, &behaviourAnimData[kNormal]);
	drawBehaviour(kNormal, angle, 0);

	_engine->_animations->setAnimAtKeyframe(behaviourAnimState[kAthletic], _engine->_animations->animTable[_engine->_actor->heroAnimIdx[kAthletic]], behaviourEntity, &behaviourAnimData[kAthletic]);
	drawBehaviour(kAthletic, angle, 0);

	_engine->_animations->setAnimAtKeyframe(behaviourAnimState[kAggressive], _engine->_animations->animTable[_engine->_actor->heroAnimIdx[kAggressive]], behaviourEntity, &behaviourAnimData[kAggressive]);
	drawBehaviour(kAggressive, angle, 0);

	_engine->_animations->setAnimAtKeyframe(behaviourAnimState[kDiscrete], _engine->_animations->animTable[_engine->_actor->heroAnimIdx[kDiscrete]], behaviourEntity, &behaviourAnimData[kDiscrete]);
	drawBehaviour(kDiscrete, angle, 0);

	drawInfoMenu(100, 300);

	copyBlockPhys(100, 100, 550, 290);
}

/** Process hero behaviour menu */
void Menu::processBehaviourMenu() {
	int32 tmpLanguageCD;
	int32 tmpTextBank;
	int32 tmpHeroBehaviour;
	int32 tmpTime;

	if (_engine->_actor->heroBehaviour == kProtoPack) {
		_engine->_sound->stopSamples();
		_engine->_actor->setBehaviour(kNormal);
	}

	behaviourEntity = _engine->_actor->bodyTable[_engine->_scene->sceneHero->entity];

	_engine->_actor->heroAnimIdx[kNormal] = _engine->_actor->heroAnimIdxNORMAL;
	_engine->_actor->heroAnimIdx[kAthletic] = _engine->_actor->heroAnimIdxATHLETIC;
	_engine->_actor->heroAnimIdx[kAggressive] = _engine->_actor->heroAnimIdxAGGRESSIVE;
	_engine->_actor->heroAnimIdx[kDiscrete] = _engine->_actor->heroAnimIdxDISCRETE;

	_engine->_movements->setActorAngleSafe(_engine->_scene->sceneHero->angle, _engine->_scene->sceneHero->angle - 256, 50, &moveMenu);

	_engine->_screens->copyScreen(_engine->frontVideoBuffer, _engine->workVideoBuffer);

	tmpLanguageCD = _engine->cfgfile.LanguageCDId;
	_engine->cfgfile.LanguageCDId = 0;

	tmpTextBank = _engine->_text->currentTextBank;
	_engine->_text->currentTextBank = -1;

	_engine->_text->initTextBank(0);

	drawBehaviourMenu(_engine->_scene->sceneHero->angle);

	tmpHeroBehaviour = _engine->_actor->heroBehaviour;

	_engine->_animations->setAnimAtKeyframe(behaviourAnimState[_engine->_actor->heroBehaviour], _engine->_animations->animTable[_engine->_actor->heroAnimIdx[_engine->_actor->heroBehaviour]], behaviourEntity, &behaviourAnimData[_engine->_actor->heroBehaviour]);

	readKeys();

	tmpTime = _engine->lbaTime;

	while (skippedKey & 4 || (skipIntro >= 59 && skipIntro <= 62)) {
		readKeys();
		key = pressedKey;

		if (key & 8) {
			_engine->_actor->heroBehaviour++;
		}

		if (key & 4) {
			_engine->_actor->heroBehaviour--;
		}

		if (_engine->_actor->heroBehaviour < 0) {
			_engine->_actor->heroBehaviour = 3;
		}

		if (_engine->_actor->heroBehaviour >= 4) {
			_engine->_actor->heroBehaviour = 0;
		}

		if (tmpHeroBehaviour != _engine->_actor->heroBehaviour) {
			drawBehaviour(tmpHeroBehaviour, _engine->_scene->sceneHero->angle, 1);
			tmpHeroBehaviour = _engine->_actor->heroBehaviour;
			_engine->_movements->setActorAngleSafe(_engine->_scene->sceneHero->angle, _engine->_scene->sceneHero->angle - 256, 50, &moveMenu);
			_engine->_animations->setAnimAtKeyframe(behaviourAnimState[_engine->_actor->heroBehaviour], _engine->_animations->animTable[_engine->_actor->heroAnimIdx[_engine->_actor->heroBehaviour]], behaviourEntity, &behaviourAnimData[_engine->_actor->heroBehaviour]);

			while (pressedKey) {
				readKeys();
				drawBehaviour(_engine->_actor->heroBehaviour, -1, 1);
			}
		}

		drawBehaviour(_engine->_actor->heroBehaviour, -1, 1);

		fpsCycles(50);
		_engine->lbaTime++;
	}

	_engine->lbaTime = tmpTime;

	_engine->_actor->setBehaviour(_engine->_actor->heroBehaviour);
	_engine->_gameState->initEngineProjections();

	_engine->_text->currentTextBank = tmpTextBank;
	_engine->_text->initTextBank(_engine->_text->currentTextBank + 3);

	_engine->cfgfile.LanguageCDId = tmpLanguageCD;
}

/** Draw the entire button box
	@param left start width to draw the button
	@param top start height to draw the button
	@param right end width to draw the button
	@param bottom end height to draw the button */
void Menu::drawMagicItemsBox(int32 left, int32 top, int32 right, int32 bottom, int32 color) { // Rect
	_engine->_interface->drawLine(left, top, right, top, color);                              // top line
	_engine->_interface->drawLine(left, top, left, bottom, color);                            // left line
	_engine->_interface->drawLine(right, ++top, right, bottom, color);                        // right line
	_engine->_interface->drawLine(++left, bottom, right, bottom, color);                      // bottom line
}

void Menu::drawItem(int32 item) {
	int32 itemX = (item / 4) * 85 + 64;
	int32 itemY = (item & 3) * 75 + 52;

	int32 left = itemX - 37;
	int32 right = itemX + 37;
	int32 top = itemY - 32;
	int32 bottom = itemY + 32;

	_engine->_interface->drawSplittedBox(left, top, right, bottom,
	                                     inventorySelectedItem == item ? inventorySelectedColor : 0);

	if (_engine->_gameState->gameFlags[item] && !_engine->_gameState->gameFlags[GAMEFLAG_INVENTORY_DISABLED] && item < NUM_INVENTORY_ITEMS) {
		_engine->_renderer->prepareIsoModel(_engine->_resources->inventoryTable[item]);
		itemAngle[item] += 8;
		_engine->_renderer->renderInventoryItem(itemX, itemY, _engine->_resources->inventoryTable[item], itemAngle[item], 15000);

		if (item == 15) { // has GAS
			_engine->_text->setFontColor(15);
			_engine->_text->drawText(left + 3, top + 32, _engine->ITOA(_engine->_gameState->inventoryNumGas));
		}
	}

	drawBox(left, top, right, bottom);
	copyBlockPhys(left, top, right, bottom);
}

void Menu::drawInventoryItems() {
	int32 item;

	drawTransparentBox(17, 10, 622, 320, 4);
	drawBox(17, 10, 622, 320);
	drawMagicItemsBox(110, 18, 188, 311, 75);
	copyBlockPhys(17, 10, 622, 320);

	for (item = 0; item < NUM_INVENTORY_ITEMS; item++) {
		drawItem(item);
	}
}

/** Process in-game inventory menu */
void Menu::processInventoryMenu() {
	int32 di = 1;
	int32 prevSelectedItem, tmpLanguageCD, bx, tmpAlphaLight, tmpBetaLight;

	tmpAlphaLight = _engine->_scene->alphaLight;
	tmpBetaLight = _engine->_scene->betaLight;

	_engine->_screens->copyScreen(_engine->frontVideoBuffer, _engine->workVideoBuffer);

	_engine->_renderer->setLightVector(896, 950, 0);

	inventorySelectedColor = 68;

	if (_engine->_gameState->inventoryNumLeafs > 0) {
		_engine->_gameState->gameFlags[GAMEFLAG_HAS_CLOVER_LEAF] = 1;
	}

	drawInventoryItems();

	tmpLanguageCD = _engine->cfgfile.LanguageCDId;
	_engine->cfgfile.LanguageCDId = 0;

	_engine->_text->initTextBank(2);

	bx = 3;

	_engine->_text->setFontCrossColor(4);
	_engine->_text->initDialogueBox();

	while (skipIntro != 1) {
		readKeys();
		prevSelectedItem = inventorySelectedItem;

		if (!di) {
			key = pressedKey;
			_engine->loopPressedKey = skippedKey;
			_engine->loopCurrentKey = skipIntro;

			if (key != 0 || skippedKey != 0) {
				di = 1;
			}
		} else {
			_engine->loopCurrentKey = 0;
			key = 0;
			_engine->loopPressedKey = 0;
			if (!pressedKey && !skippedKey) {
				di = 0;
			}
		}

		if (_engine->loopCurrentKey == 1 || _engine->loopPressedKey & 0x20)
			break;

		if (key & 2) { // down
			inventorySelectedItem++;
			if (inventorySelectedItem >= NUM_INVENTORY_ITEMS) {
				inventorySelectedItem = 0;
			}
			drawItem(prevSelectedItem);
			bx = 3;
		}

		if (key & 1) { // up
			inventorySelectedItem--;
			if (inventorySelectedItem < 0) {
				inventorySelectedItem = NUM_INVENTORY_ITEMS - 1;
			}
			drawItem(prevSelectedItem);
			bx = 3;
		}

		if (key & 4) { // left
			inventorySelectedItem -= 4;
			if (inventorySelectedItem < 0) {
				inventorySelectedItem += NUM_INVENTORY_ITEMS;
			}
			drawItem(prevSelectedItem);
			bx = 3;
		}

		if (key & 8) { // right
			inventorySelectedItem += 4;
			if (inventorySelectedItem >= NUM_INVENTORY_ITEMS) {
				inventorySelectedItem -= NUM_INVENTORY_ITEMS;
			}
			drawItem(prevSelectedItem);
			bx = 3;
		}

		if (bx == 3) {
			_engine->_text->initInventoryDialogueBox();

			if (_engine->_gameState->gameFlags[inventorySelectedItem] == 1 && !_engine->_gameState->gameFlags[GAMEFLAG_INVENTORY_DISABLED] && inventorySelectedItem < NUM_INVENTORY_ITEMS) {
				_engine->_text->initText(inventorySelectedItem + 100);
			} else {
				_engine->_text->initText(128);
			}
			bx = 0;
		}

		if (bx != 2) {
			bx = _engine->_text->printText10();
		}

		// TRICKY: 3D model rotation delay - only apply when no text is drawing
		if (bx == 0 || bx == 2) {
			sdldelay(15);
		}

		if (_engine->loopPressedKey & 1) {
			if (bx == 2) {
				_engine->_text->initInventoryDialogueBox();
				bx = 0;
			} else {
				if (_engine->_gameState->gameFlags[inventorySelectedItem] == 1 && !_engine->_gameState->gameFlags[GAMEFLAG_INVENTORY_DISABLED] && inventorySelectedItem < NUM_INVENTORY_ITEMS) {
					_engine->_text->initInventoryDialogueBox();
					_engine->_text->initText(inventorySelectedItem + 100);
				}
			}
		}

		drawItem(inventorySelectedItem);

		if ((_engine->loopPressedKey & 2) && _engine->_gameState->gameFlags[inventorySelectedItem] == 1 && !_engine->_gameState->gameFlags[GAMEFLAG_INVENTORY_DISABLED] && inventorySelectedItem < NUM_INVENTORY_ITEMS) {
			_engine->loopInventoryItem = inventorySelectedItem;
			inventorySelectedColor = 91;
			drawItem(inventorySelectedItem);
			break;
		}
	}

	_engine->_text->printTextVar13 = 0;

	_engine->_scene->alphaLight = tmpAlphaLight;
	_engine->_scene->betaLight = tmpBetaLight;

	_engine->_gameState->initEngineProjections();

	_engine->cfgfile.LanguageCDId = tmpLanguageCD;

	_engine->_text->initTextBank(_engine->_text->currentTextBank + 3);

	while (skipIntro != 0 && skippedKey != 0) {
		readKeys();
		sdldelay(1);
	}
}

} // namespace TwinE
