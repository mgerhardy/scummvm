/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef TWINE_RESOURCES_BKG_H
#define TWINE_RESOURCES_BKG_H

#include "common/array.h"
#include "common/scummsys.h"

namespace TwinE {

namespace Bkg {

/** LBA2 merged background archive header (T_BKG_HEADER). */
struct Header {
	uint16 griStart = 1;
	uint16 grmStart = 149;
	uint16 bllStart = 179;
	uint16 brkStart = 197;
	uint16 maxBrk = 0;
	uint16 forbidenBrick = 0;
};

/** Scene map entry (TabAllCube / grids2.ts). */
struct SceneMapEntry {
	uint8 opcode = 0;
	uint8 sceneryIndex = 0;
	uint8 libraryIndex = 0;
	bool isIsland = false;
};

/** Load partition table from lba_bkg.hqr entry 0 (falls back to GOG layout). */
void init();

const Header &getHeader();

/** Parse scene map from lba_bkg entry grmStart + sceneIndex. */
Common::Array<SceneMapEntry> loadSceneMap(int32 sceneIndex);

/** Primary scenery index for a scene (TabAllCube indirection). */
int32 getSceneryIndex(int32 sceneIndex);

/** HQR entry index for a scene grid (after TabAllCube indirection is applied by the caller). */
int32 gridEntryIndex(int32 sceneryIndex);

/** HQR entry index for a block library (My_Bll from T_GRI_HEADER). */
int32 blockLibraryEntryIndex(int32 myBll);

/** HQR entry index for a brick. */
int32 brickEntryIndex(int32 brickIndex);

/** Load a raw entry from lba_bkg.hqr. */
int32 loadEntry(uint8 **ptr, int32 index);

} // namespace Bkg

} // namespace TwinE

#endif
