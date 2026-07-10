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

#include "twine/resources/bkg.h"
#include "twine/resources/hqr.h"
#include "twine/resources/resources.h"
#include "common/array.h"
#include "common/endian.h"
#include "common/textconsole.h"
#include "common/util.h"
#include "twine/shared.h"

namespace TwinE {

namespace Bkg {

static Header g_header;
static bool g_initialized = false;

static bool isValidHeader(const Header &h, int32 numEntries) {
	if (h.griStart >= (uint16)numEntries || h.bllStart >= (uint16)numEntries || h.brkStart >= (uint16)numEntries) {
		return false;
	}
	if (h.griStart > h.grmStart || h.grmStart > h.bllStart || h.bllStart > h.brkStart) {
		return false;
	}
	return true;
}

void init() {
	if (g_initialized) {
		return;
	}

	g_initialized = true;

	const int32 numEntries = HQR::numEntries(Resources::HQR_LBA_BKG_FILE);
	if (numEntries <= 0) {
		warning("Bkg: Could not open %s", Resources::HQR_LBA_BKG_FILE);
		return;
	}

	uint8 *headerBuf = nullptr;
	const int32 headerSize = HQR::getAllocEntry(&headerBuf, Resources::HQR_LBA_BKG_FILE, 0);
	if (headerSize >= 28) {
		Header candidate;
		candidate.griStart = READ_LE_UINT16(headerBuf + 0);
		candidate.grmStart = READ_LE_UINT16(headerBuf + 2);
		candidate.bllStart = READ_LE_UINT16(headerBuf + 4);
		candidate.brkStart = READ_LE_UINT16(headerBuf + 6);
		candidate.maxBrk = READ_LE_UINT16(headerBuf + 8);
		candidate.forbidenBrick = READ_LE_UINT16(headerBuf + 10);
		if (isValidHeader(candidate, numEntries + 1)) {
			g_header = candidate;
			free(headerBuf);
			debugC(1, TwinE::kDebugResources, "Bkg: loaded header Gri=%u Bll=%u Brk=%u",
				g_header.griStart, g_header.bllStart, g_header.brkStart);
			return;
		}
	}
	free(headerBuf);

	// GOG classic layout (verified against LBA_BKG.HQR)
	g_header.griStart = 1;
	g_header.grmStart = 149;
	g_header.bllStart = 179;
	g_header.brkStart = 197;
	g_header.maxBrk = (uint16)MAX(0, numEntries - g_header.brkStart - 1);
	debugC(1, TwinE::kDebugResources, "Bkg: using default GOG partition table");
}

const Header &getHeader() {
	return g_header;
}

int32 gridEntryIndex(int32 sceneryIndex) {
	return g_header.griStart + sceneryIndex;
}

Common::Array<SceneMapEntry> loadSceneMap(int32 sceneIndex) {
	Common::Array<SceneMapEntry> map;
	init();

	uint8 *buf = nullptr;
	const int32 size = loadEntry(&buf, g_header.grmStart + sceneIndex);
	if (size <= 0 || buf == nullptr) {
		return map;
	}

	// Scene map entries are small; large entries are not scene maps.
	if (size > 512) {
		free(buf);
		return map;
	}

	for (int32 offset = 0; offset + 1 < size; offset += 2) {
		const uint8 opcode = buf[offset];
		const uint8 sceneryIndex = buf[offset + 1];
		if (opcode == 0) {
			break;
		}

		SceneMapEntry entry;
		entry.opcode = opcode;
		entry.sceneryIndex = sceneryIndex;
		entry.isIsland = opcode == 2;

		uint8 *gridBuf = nullptr;
		const int32 gridSize = loadEntry(&gridBuf, gridEntryIndex(sceneryIndex));
		if (gridSize > 0 && gridBuf != nullptr) {
			entry.libraryIndex = gridBuf[0];
		}
		free(gridBuf);

		map.push_back(entry);
	}
	free(buf);
	return map;
}

int32 getSceneryIndex(int32 sceneIndex) {
	const Common::Array<SceneMapEntry> map = loadSceneMap(sceneIndex);
	if (!map.empty()) {
		return map[0].sceneryIndex;
	}
	return sceneIndex;
}

int32 blockLibraryEntryIndex(int32 myBll) {
	return g_header.bllStart + myBll;
}

int32 brickEntryIndex(int32 brickIndex) {
	return g_header.brkStart + brickIndex;
}

int32 loadEntry(uint8 **ptr, int32 index) {
	return HQR::getAllocEntry(ptr, Resources::HQR_LBA_BKG_FILE, index);
}

} // namespace Bkg

} // namespace TwinE
