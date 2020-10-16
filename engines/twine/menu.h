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

#ifndef TWINE_MENU_H
#define TWINE_MENU_H

#include "twine/actor.h"
#include "twine/sdlengine.h"

namespace TwinE {

class Menu {
private:
	TwinEEngine *_engine;

	/** Hero behaviour menu entity */
	uint8 *behaviourEntity;
	/** Behaviour menu anim state */
	int16 behaviourAnimState[4]; // winTab
	/** Behaviour menu anim data pointer */
	AnimTimerDataStruct behaviourAnimData[4];

	int32 inventorySelectedColor;
	int32 inventorySelectedItem; // currentSelectedObjectInInventory

	void drawButtonGfx(int32 width, int32 topheight, int32 id, int32 value, int32 mode);
	void plasmaEffectRenderFrame();
	void drawButton(int16 *menuSettings, int32 mode);
	int32 advoptionsMenu();
	int32 volumeMenu();
	int32 savemanageMenu();
	void drawInfoMenu(int16 left, int16 top);
	void drawBehaviour(int16 behaviour, int32 angle, int16 cantDrawBox);
	void drawInventoryItems();
	void drawBehaviourMenu(int32 angle);
	void drawItem(int32 item);
	void drawMagicItemsBox(int32 left, int32 top, int32 right, int32 bottom, int32 color);

public:
	Menu(TwinEEngine *engine);

	int32 currMenuTextIndex;
	int32 currMenuTextBank;
	int8 currMenuTextBuffer[256];

	int16 itemAngle[255]; // objectRotation

	/** Options Menu Settings

	Used to create the options menu. */
	static int16 OptionsMenuSettings[];

	/** Behaviour menu move pointer */
	ActorMoveStruct moveMenu;

	/** Plasma Effect pointer to file content: RESS.HQR:51 */
	uint8 *plasmaEffectPtr;

	/** Process the plasma effect
	@param top top height where the effect will be draw in the front buffer
	@param color plasma effect start color */
	void processPlasmaEffect(int32 top, int32 color);

	/** Draw the entire button box
	@param left start width to draw the button
	@param top start height to draw the button
	@param right end width to draw the button
	@param bottom end height to draw the button */
	void drawBox(int32 left, int32 top, int32 right, int32 bottom);

	/** Draws inside buttons transparent area
	@param left start width to draw the button
	@param top start height to draw the button
	@param right end width to draw the button
	@param bottom end height to draw the button
	@param colorAdj index to adjust the transparent box color */
	void drawTransparentBox(int32 left, int32 top, int32 right, int32 bottom, int32 colorAdj);

	/** Where the main menu options are processed
	@param menuSettings menu settings array with the information to build the menu options
	@return pressed menu button identification */
	int32 processMenu(int16 *menuSettings);

	/** Used to run the main menu */
	void mainMenu();

	/** Used to run the in-game give-up menu */
	int32 giveupMenu();

	/** Used to run the options menu */
	int32 optionsMenu();

	/** Process hero behaviour menu */
	void processBehaviourMenu();

	/** Process in-game inventory menu */
	void processInventoryMenu();
};

} // namespace TwinE

#endif
