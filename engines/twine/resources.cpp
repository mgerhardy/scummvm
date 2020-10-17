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

#include "twine/resources.h"
#include "twine/animations.h"
#include "twine/scene.h"
#include "twine/screens.h"
#include "twine/sdlengine.h"
#include "twine/sound.h"
#include "twine/text.h"

namespace TwinE {

void Resources::initPalettes() {
	// Init standard palette
	_engine->_hqrdepack->hqrGetallocEntry(&_engine->_screens->mainPalette, HQR_RESS_FILE, RESSHQR_MAINPAL);
	_engine->_screens->convertPalToRGBA(_engine->_screens->mainPalette, _engine->_screens->mainPaletteRGBA);

	memcpy(_engine->_screens->palette, _engine->_screens->mainPalette, NUMOFCOLORS * 3);

	_engine->_screens->convertPalToRGBA(_engine->_screens->palette, _engine->_screens->paletteRGBA);
	setPalette(_engine->_screens->paletteRGBA);

	// We use it now
	_engine->_screens->palCustom = 0;
}

void Resources::preloadSprites() {
	int32 i;
	int32 numEntries = _engine->_hqrdepack->hqrNumEntries(HQR_SPRITES_FILE) - 1;

	for (i = 0; i < numEntries; i++) {
		_engine->_actor->spriteSizeTable[i] = _engine->_hqrdepack->hqrGetallocEntry(&_engine->_actor->spriteTable[i], HQR_SPRITES_FILE, i);
	}
}

void Resources::preloadAnimations() {
	int32 i;
	int32 numEntries = _engine->_hqrdepack->hqrNumEntries(HQR_ANIM_FILE) - 1;

	for (i = 0; i < numEntries; i++) {
		_engine->_animations->animSizeTable[i] = _engine->_hqrdepack->hqrGetallocEntry(&_engine->_animations->animTable[i], HQR_ANIM_FILE, i);
	}
}

void Resources::preloadSamples() {
	int32 i;
	int32 numEntries = _engine->_hqrdepack->hqrNumEntries(HQR_SAMPLES_FILE) - 1;

	for (i = 0; i < numEntries; i++) {
		_engine->_sound->samplesSizeTable[i] = _engine->_hqrdepack->hqrGetallocEntry(&_engine->_sound->samplesTable[i], HQR_SAMPLES_FILE, i);
	}
}

void Resources::preloadInventoryItems() {
	int32 i;
	int32 numEntries = _engine->_hqrdepack->hqrNumEntries(HQR_INVOBJ_FILE) - 1;

	for (i = 0; i < numEntries; i++) {
		inventorySizeTable[i] = _engine->_hqrdepack->hqrGetallocEntry(&inventoryTable[i], HQR_INVOBJ_FILE, i);
	}
}

void Resources::initResources() {
	// Menu and in-game palette
	initPalettes();

	// load LBA font
	_engine->_hqrdepack->hqrGetallocEntry(&_engine->_text->fontPtr, HQR_RESS_FILE, RESSHQR_LBAFONT);

	_engine->_text->setFontParameters(2, 8);
	_engine->_text->setFontColor(14);
	_engine->_text->setTextCrossColor(136, 143, 2);

	_engine->_hqrdepack->hqrGetallocEntry(&_engine->_scene->spriteShadowPtr, HQR_RESS_FILE, RESSHQR_SPRITESHADOW);

	// load sprite actors bounding box data
	_engine->_hqrdepack->hqrGetallocEntry(&_engine->_scene->spriteBoundingBoxPtr, HQR_RESS_FILE, RESSHQR_SPRITEBOXDATA);

	preloadSprites();
	preloadAnimations();
	//preloadSamples();
	preloadInventoryItems();
}

} // namespace TwinE
