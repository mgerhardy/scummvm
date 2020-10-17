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

#include "common/debug.h"
#include "twine/debug.grid.h"
#include "twine/grid.h"
#include "twine/redraw.h"
#include "twine/scene.h"
#include "twine/twine.h"

namespace TwinE {

DebugGrid::DebugGrid(TwinEEngine *engine) : _engine(engine) {}

void DebugGrid::changeGridCamera(int16 pKey) {
	if (useFreeCamera) {
		// Press up - more X positions
		if (pKey == 0x2E) {
			_engine->_grid->newCameraZ--;
			_engine->_redraw->reqBgRedraw = 1;
		}

		// Press down - less X positions
		if (pKey == 0x2C) {
			_engine->_grid->newCameraZ++;
			_engine->_redraw->reqBgRedraw = 1;
		}

		// Press left - less Z positions
		if (pKey == 0x1F) {
			_engine->_grid->newCameraX--;
			_engine->_redraw->reqBgRedraw = 1;
		}

		// Press right - more Z positions
		if (pKey == 0x2D) {
			_engine->_grid->newCameraX++;
			_engine->_redraw->reqBgRedraw = 1;
		}
	}
}

void DebugGrid::changeGrid(int16 pKey) {
	if (canChangeScenes) {
		// Press up - more X positions
		if (pKey == 0x13) {
			_engine->_scene->currentSceneIdx++;
			if (_engine->_scene->currentSceneIdx > NUM_SCENES)
				_engine->_scene->currentSceneIdx = 0;
			_engine->_scene->needChangeScene = _engine->_scene->currentSceneIdx;
			_engine->_redraw->reqBgRedraw = 1;
		}

		// Press down - less X positions
		if (pKey == 0x21) {
			_engine->_scene->currentSceneIdx--;
			if (_engine->_scene->currentSceneIdx < 0)
				_engine->_scene->currentSceneIdx = NUM_SCENES;
			_engine->_scene->needChangeScene = _engine->_scene->currentSceneIdx;
			_engine->_redraw->reqBgRedraw = 1;
		}

		if (_engine->cfgfile.Debug && (pKey == 'f' || pKey == 'r'))
			debug("\nGrid index changed: %d\n", _engine->_scene->needChangeScene);
	}
}

void DebugGrid::applyCellingGrid(int16 pKey) {
	// Increase celling grid index
	if (pKey == 0x22) {
		_engine->_grid->cellingGridIdx++;
		if (_engine->_grid->cellingGridIdx > 133)
			_engine->_grid->cellingGridIdx = 133;
	}
	// Decrease celling grid index
	if (pKey == 0x30) {
		_engine->_grid->cellingGridIdx--;
		if (_engine->_grid->cellingGridIdx < 0)
			_engine->_grid->cellingGridIdx = 0;
	}

	// Enable/disable celling grid
	if (pKey == 0x14 && _engine->_grid->useCellingGrid == -1) {
		_engine->_grid->useCellingGrid = 1;
		//createGridMap();
		_engine->_grid->initCellingGrid(_engine->_grid->cellingGridIdx);
		if (_engine->cfgfile.Debug && pKey == 0x14)
			debug("\nEnable Celling Grid index: %d\n", _engine->_grid->cellingGridIdx);
		_engine->_scene->needChangeScene = -2; // tricky to make the fade
	} else if (pKey == 0x14 && _engine->_grid->useCellingGrid == 1) {
		_engine->_grid->useCellingGrid = -1;
		_engine->_grid->createGridMap();
		_engine->_redraw->reqBgRedraw = 1;
		if (_engine->cfgfile.Debug && pKey == 0x14)
			debug("\nDisable Celling Grid index: %d\n", _engine->_grid->cellingGridIdx);
		_engine->_scene->needChangeScene = -2; // tricky to make the fade
	}
}

} // namespace TwinE
