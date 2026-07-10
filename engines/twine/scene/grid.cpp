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

#include "common/endian.h"
#include "common/memstream.h"
#include "common/textconsole.h"
#include "twine/debugger/debug_state.h"
#include "twine/menu/interface.h"
#include "twine/parser/blocklibrary.h"
#include "twine/renderer/redraw.h"
#include "twine/renderer/renderer.h"
#include "twine/renderer/screens.h"
#include "twine/resources/resources.h"
#include "twine/resources/bkg.h"
#include "twine/scene/actor.h"
#include "twine/scene/collision.h"
#include "twine/scene/graph.h"
#include "twine/scene/exterior.h"
#include "twine/scene/grid.h"
#include "twine/scene/scene.h"
#include "twine/shared.h"
#include "twine/twine.h"

#define CELLING_GRIDS_START_INDEX 120
#define LBA2_GRI_HEADER_SIZE 34

namespace TwinE {

namespace {

bool isWaterBlock(const BlockDataEntry *blockPtr, bool lba2) {
	if (lba2) {
		return blockPtr->groundType == 1;
	}
	return blockPtr->brickType == WATER_BRICK;
}

} // namespace

Grid::Grid(TwinEEngine *engine) : _engine(engine) {
	_blockBufferSize = SIZE_CUBE_X * SIZE_CUBE_Z * SIZE_CUBE_Y * sizeof(BlockEntry);
	_bufCube = (uint8 *)malloc(_blockBufferSize);
}

Grid::~Grid() {
	free(_bufCube);
	for (int32 i = 0; i < ARRAYSIZE(_brickMaskTable); i++) {
		free(_brickMaskTable[i]);
	}
	for (int32 i = 0; i < ARRAYSIZE(_bufferBrick); i++) {
		free(_bufferBrick[i]);
	}
	free(_currentGrid);
	free(_nbBrickColon);
	free(_listBrickColon);
}

void Grid::init(int32 w, int32 h) {
	const int32 numbrickentries = (1 + (w + 24) / 24);
	const size_t brickDataBufferSize = numbrickentries * MAX_BRICKS * sizeof(BrickEntry);
	_listBrickColon = (BrickEntry *)malloc(brickDataBufferSize);
	_brickInfoBufferSize = numbrickentries * sizeof(int16);
	_nbBrickColon = (int16 *)malloc(_brickInfoBufferSize);
}

void Grid::copyMask(int32 index, int32 x, int32 y, const Graphics::ManagedSurface &buffer) {
	if (_engine->_debugState->_disableGridRendering) {
		return;
	}
	uint8 *ptr = _brickMaskTable[index];

	int32 left = x + *(ptr + 2);
	int32 top = y + *(ptr + 3);
	int32 right = *ptr + left - 1;
	int32 bottom = *(ptr + 1) + top - 1;

	if (left > _engine->_interface->_clip.right || right < _engine->_interface->_clip.left || bottom < _engine->_interface->_clip.top || top > _engine->_interface->_clip.bottom) {
		return;
	}

	ptr += 4;

	int32 absX = left;
	int32 absY = top;

	int32 vSize = (bottom - top) + 1;

	if (vSize <= 0) {
		return;
	}

	int32 offset = -((right - left) - _engine->width()) - 1;

	right++;
	bottom++;

	// if line on top aren't in the blitting area...
	if (absY < _engine->_interface->_clip.top) {
		int numOfLineToRemove = _engine->_interface->_clip.top - absY;

		vSize -= numOfLineToRemove;
		if (vSize <= 0) {
			return;
		}

		absY += numOfLineToRemove;

		do {
			int lineDataSize;

			lineDataSize = *(ptr++);
			ptr += lineDataSize;
		} while (--numOfLineToRemove);
	}

	// reduce the vSize to remove lines on bottom
	if (absY + vSize - 1 > _engine->_interface->_clip.bottom) {
		vSize = _engine->_interface->_clip.bottom - absY + 1;
		if (vSize <= 0) {
			return;
		}
	}

	uint8 *outPtr = (uint8 *)_engine->_frontVideoBuffer.getBasePtr(left, absY);
	const uint8 *inPtr = (const uint8 *)buffer.getBasePtr(left, absY);

	do {
		int32 height = *(ptr++);

		do {
			int32 width = *(ptr++); // skip size
			outPtr += width;
			inPtr += width;

			absX += width;

			height--;
			if (!height) {
				break;
			}

			width = *(ptr++); // copy size

			for (int32 j = 0; j < width; j++) {
				if (absX >= _engine->_interface->_clip.left && absX <= _engine->_interface->_clip.right) {
					*outPtr = *inPtr;
				}

				absX++;
				outPtr++;
				inPtr++;
			}
		} while (--height);

		absX = left;

		outPtr += offset;
		inPtr += offset;
	} while (--vSize);
}

const BrickEntry *Grid::getBrickEntry(int32 j, int32 i) const {
	return &_listBrickColon[j * MAX_BRICKS + i];
}

void Grid::drawOverBrick(int32 x, int32 y, int32 z) {
	const int32 startCol = ((_engine->_interface->_clip.left + 24) / 24) - 1;
	const int32 endCol = ((_engine->_interface->_clip.right + 24) / 24);

	for (int32 col = startCol; col <= endCol; col++) {
		for (int32 i = 0; i < _nbBrickColon[col]; i++) {
			const BrickEntry *currBrickEntry = getBrickEntry(col, i);

			if (currBrickEntry->posY + 38 > _engine->_interface->_clip.top && currBrickEntry->posY <= _engine->_interface->_clip.bottom && currBrickEntry->y >= y) {
				if (currBrickEntry->x + currBrickEntry->z > z + x) {
					copyMask(currBrickEntry->index, (col * 24) - 24, currBrickEntry->posY, _engine->_workVideoBuffer);
				}
			}
		}
	}
}

void Grid::drawOverBrick3(int32 x, int32 y, int32 z) {
	const int32 startCol = ((_engine->_interface->_clip.left + 24) / 24) - 1;
	const int32 endCol = (_engine->_interface->_clip.right + 24) / 24;

	for (int32 col = startCol; col <= endCol; col++) {
		for (int32 i = 0; i < _nbBrickColon[col]; i++) {
			const BrickEntry *currBrickEntry = getBrickEntry(col, i);

			if (currBrickEntry->posY + 38 > _engine->_interface->_clip.top && currBrickEntry->posY <= _engine->_interface->_clip.bottom && currBrickEntry->y >= y) {
				if (currBrickEntry->x == x && currBrickEntry->z == z) {
					copyMask(currBrickEntry->index, (col * 24) - 24, currBrickEntry->posY, _engine->_workVideoBuffer);
				}

				if (currBrickEntry->x > x || currBrickEntry->z > z) {
					copyMask(currBrickEntry->index, (col * 24) - 24, currBrickEntry->posY, _engine->_workVideoBuffer);
				}
			}
		}
	}
}

void Grid::processGridMask(const uint8 *buffer, uint8 *ptr) {
	const uint8 width = *buffer++;
	uint8 height = *buffer++;
	const uint8 offsetX = *buffer++;
	const uint8 offsetY = *buffer++;
	const int32 maxY = offsetY + height;

	*ptr++ = width;
	*ptr++ = height;
	*ptr++ = offsetX;
	*ptr++ = offsetY;

	uint8 *targetPtrPos = ptr;

	for (int32 y = offsetY; y < maxY; ++y) {
		uint8 numOfBlock = 0;
		uint8 opaquePixels = 0;
		uint8 *numOfBlockTargetPtr = targetPtrPos;

		targetPtrPos++;

		const uint8 numRuns = *buffer++;

		// the first time isn't skip. the skip size is 0 in that case
		if (bits(*buffer, 6, 2) != 0) {
			*targetPtrPos++ = 0;
			numOfBlock++;
		}

		for (uint8 run = 0; run < numRuns; ++run) {
			const uint8 runSpec = *buffer++;
			const uint8 runLength = bits(runSpec, 0, 6) + 1;
			const uint8 type = bits(runSpec, 6, 2);
			if (type == 2) {
				opaquePixels += runLength;
				buffer++;
			} else if (type == 1) { // 0x40
				opaquePixels += runLength;
				buffer += runLength;
			} else { // skip (type 3)
				if (opaquePixels) {
					*targetPtrPos++ = opaquePixels; // write down the number of pixel passed so far
					numOfBlock++;
					opaquePixels = 0;
				}
				*targetPtrPos++ = runLength; // write skip
				numOfBlock++;
			}
		}

		if (opaquePixels) {
			*targetPtrPos++ = opaquePixels;
			numOfBlock++;

			opaquePixels = 0;
		}

		*numOfBlockTargetPtr = numOfBlock;
	}
}

void Grid::createGridMask() {
	for (int32 b = 0; b < NUM_BRICKS; b++) {
		if (!_brickUsageTable[b]) {
			continue;
		}
		if (_brickMaskTable[b]) {
			free(_brickMaskTable[b]);
		}
		_brickMaskTable[b] = (uint8 *)malloc(_brickSizeTable[b]);
		processGridMask(_bufferBrick[b], _brickMaskTable[b]);
	}
}

void Grid::getSpriteSize(int32 offset, int32 *width, int32 *height, const uint8 *spritePtr) {
	spritePtr += READ_LE_INT32(spritePtr + offset * 4);

	*width = *spritePtr;
	*height = *(spritePtr + 1);
}

void Grid::loadGridBricks() {
	uint32 firstBrick = 60000;
	uint32 lastBrick = 0;
	uint32 currentBllEntryIdx = 1;

	memset(_brickSizeTable, 0, sizeof(_brickSizeTable));
	memset(_brickUsageTable, 0, sizeof(_brickUsageTable));

	// get block libraries usage bits
	const uint8 *ptrToBllBits;
	if (_engine->isLBA2()) {
		ptrToBllBits = _currentGrid + 2; // T_GRI_HEADER::UsedBlock
	} else {
		ptrToBllBits = _currentGrid + (_currentGridSize - 32);
	}

	// for all bits under the 32bytes (256bits)
	for (uint32 i = 1; i < 256; i++) {
		const uint8 currentBitByte = *(ptrToBllBits + (i / 8));
		const uint8 currentBitMask = 1 << (7 - (i & 7));

		if (currentBitByte & currentBitMask) {
			const BlockData *currentBllPtr = getBlockLibrary(currentBllEntryIdx);
			for (const BlockDataEntry &entry : currentBllPtr->entries) {
				uint16 brickIdx = entry.brickIdx;
				if (!brickIdx) {
					continue;
				}
				brickIdx--;
				if (brickIdx <= firstBrick) {
					firstBrick = brickIdx;
				}

				if (brickIdx > lastBrick) {
					lastBrick = brickIdx;
				}

				_brickUsageTable[brickIdx] = 1;
			}
		}
		++currentBllEntryIdx;
	}

	if (_engine->isLBA2()) {
		Bkg::init();
	}

	for (uint32 i = firstBrick; i <= lastBrick; i++) {
		if (!_brickUsageTable[i]) {
			free(_bufferBrick[i]);
			_bufferBrick[i] = nullptr;
			continue;
		}
		if (_engine->isLBA2()) {
			_brickSizeTable[i] = Bkg::loadEntry(&_bufferBrick[i], Bkg::brickEntryIndex(i));
		} else {
			_brickSizeTable[i] = HQR::getAllocEntry(&_bufferBrick[i], Resources::HQR_LBA_BRK_FILE, i);
		}
		if (_brickSizeTable[i] == 0) {
			warning("Failed to load isometric brick index %i", i);
		}
	}
}

void Grid::decompColumn(const uint8 *gridEntry, uint32 gridEntrySize, uint8 *dest, uint32 destSize) { // DecompColonne
	Common::MemoryReadStream stream(gridEntry, gridEntrySize);
	Common::MemoryWriteStream outstream(dest, destSize);
	int32 brickCount = stream.readByte();

	do {
		const int32 flag = stream.readByte();
		const int32 blockCount = bits(flag, 0, 6) + 1;
		const int32 type = bits(flag, 6, 2);
		if (type == 0) {
			for (int32 i = 0; i < blockCount; i++) {
				outstream.writeUint16LE(0);
			}
		} else if (type == 1) { // 0x40
			for (int32 i = 0; i < blockCount; i++) {
				const uint8 layout = stream.readByte() - 1;
				const uint8 blockIdx = stream.readByte();
				const uint16 packed = (uint16)((layout + 1) & 0xff) | ((uint16)blockIdx << 8);
				outstream.writeUint16LE(packed);
			}
		} else if (type == 2) {
			const uint16 gridIdx = stream.readUint16LE();
			for (int32 i = 0; i < blockCount; i++) {
				outstream.writeUint16LE(gridIdx);
			}
		} else if (type == 3) {
			warning("Unsupported grid block type 3");
			for (int32 i = 0; i < blockCount; i++) {
				outstream.writeUint16LE(0);
			}
		} else {
			warning("Unknown grid block type %i", type);
			for (int32 i = 0; i < blockCount; i++) {
				outstream.writeUint16LE(0);
			}
		}
		assert(!outstream.err());
		assert(!stream.err());
	} while (--brickCount);
}

void Grid::calcGraphMsk(const uint8 *gridEntry, uint32 gridEntrySize, uint8 *dest, uint32 destSize) {
	Common::MemoryReadStream stream(gridEntry, gridEntrySize);
	Common::SeekableMemoryWriteStream outstream(dest, destSize);
	int32 brickCount = stream.readByte();

	do {
		const int32 flag = stream.readByte();
		const int32 blockCount = bits(flag, 0, 6) + 1;
		const int32 type = bits(flag, 6, 2);

		if (type == 0) {
			for (int32 i = 0; i < blockCount; i++) {
				outstream.seek(outstream.pos() + 2);
			}
		} else if (type == 1) {
			for (int32 i = 0; i < blockCount; i++) {
				outstream.writeUint16LE(stream.readUint16LE());
			}
		} else {
			const uint16 gridIdx = stream.readUint16LE();
			for (int32 i = 0; i < blockCount; i++) {
				outstream.writeUint16LE(gridIdx);
			}
		}
		assert(!outstream.err());
		assert(!stream.err());
	} while (--brickCount);
}

void Grid::copyMapToCube() {
	int32 blockOffset = 0;
	const int32 offsetTableBase = _engine->isLBA2() ? LBA2_GRI_HEADER_SIZE : 0;

	for (int32 z = 0; z < SIZE_CUBE_Z; z++) {
		const int32 gridIdx = z * SIZE_CUBE_X;

		for (int32 x = 0; x < SIZE_CUBE_X; x++) {
			const int32 gridOffset = READ_LE_UINT16(_currentGrid + offsetTableBase + 2 * (x + gridIdx));
			decompColumn(_currentGrid + offsetTableBase + gridOffset, _currentGridSize - offsetTableBase - gridOffset, _bufCube + blockOffset, _blockBufferSize - blockOffset);
			blockOffset += 2 * SIZE_CUBE_Y;
		}
	}
}

void Grid::createCellingGridMap(const uint8 *gridPtr, int32 gridPtrSize) { // MixteMapToCube
	int32 currGridOffset = 0;
	int32 blockOffset = 0;

	for (int32 z = 0; z < SIZE_CUBE_Z; z++) {
		const uint8 *tempGridPtr = gridPtr + currGridOffset;

		for (int32 x = 0; x < SIZE_CUBE_X; x++) {
			const int gridOffset = READ_LE_UINT16(tempGridPtr);
			tempGridPtr += 2;
			calcGraphMsk(gridPtr + gridOffset, gridPtrSize - gridOffset, _bufCube + blockOffset, _blockBufferSize - blockOffset);
			blockOffset += 2 * SIZE_CUBE_Y;
		}
		currGridOffset += SIZE_CUBE_X + SIZE_CUBE_Z;
	}
}

bool Grid::initGrid(int32 index) {
	if (_engine->isLBA2()) {
		Bkg::init();
		const int32 sceneryIndex = Bkg::getSceneryIndex(index);
		_currentGridSize = Bkg::loadEntry(&_currentGrid, Bkg::gridEntryIndex(sceneryIndex));
		if (_currentGridSize == 0) {
			warning("Failed to load grid index: %i from %s", index, Resources::HQR_LBA_BKG_FILE);
			return false;
		}

		const uint8 myBll = _currentGrid[0];
		uint8 *bllBuf = nullptr;
		const int32 bllSize = Bkg::loadEntry(&bllBuf, Bkg::blockLibraryEntryIndex(myBll));
		if (bllSize == 0 || bllBuf == nullptr) {
			warning("Failed to load block library %i for grid %i", myBll, index);
			return false;
		}
		Common::MemoryReadStream bllStream(bllBuf, bllSize, DisposeAfterUse::YES);
		if (!_currentBlockLibrary.loadFromStream(bllStream, false)) {
			warning("Failed to parse block library %i for grid %i", myBll, index);
			return false;
		}
	} else {
		// load grids from file
		_currentGridSize = HQR::getAllocEntry(&_currentGrid, Resources::HQR_LBA_GRI_FILE, index);
		if (_currentGridSize == 0) {
			warning("Failed to load grid index: %i", index);
			return false;
		}

		// load layouts from file
		if (!_currentBlockLibrary.loadFromHQR(Resources::HQR_LBA_BLL_FILE, index, true)) {
			warning("Failed to load block library index: %i", index);
			return false;
		}
	}

	loadGridBricks();

	createGridMask();

	copyMapToCube();

	return true;
}

bool Grid::initCellingGrid(int32 index) { // IncrustGrm
	uint8 *gridPtr = nullptr;

	// load grids from file
	const int realIndex = index + CELLING_GRIDS_START_INDEX;
	const int32 gridSize = HQR::getAllocEntry(&gridPtr, Resources::HQR_LBA_GRI_FILE, realIndex);
	if (gridSize == 0) {
		warning("Failed to load grid index %i", realIndex);
		return false;
	}

	createCellingGridMap(gridPtr, gridSize);
	free(gridPtr);
	_engine->_redraw->_firstTime = true;
	return true;
}

bool Grid::drawGraph(int32 index, int32 posX, int32 posY) { // AffGraph
	return drawGraph(posX, posY, _bufferBrick[index], false);
}

bool Grid::drawGraph(int32 index, int32 posX, int32 posY, const uint8 *ptr) { // AffGraph
	ptr = ptr + READ_LE_INT32(ptr + index * 4);                               // GetGraph, GetMask
	return drawGraph(posX, posY, ptr, true);
}

bool Grid::drawSprite(int32 posX, int32 posY, const SpriteData &ptr, int spriteIndex) {
	const int32 left = posX + ptr.offsetX(spriteIndex);
	if (left >= _engine->_interface->_clip.right) {
		return false;
	}
	const int32 right = ptr.surface(spriteIndex).w + left;
	if (right < _engine->_interface->_clip.left) {
		return false;
	}
	const int32 top = posY + ptr.offsetY(spriteIndex);
	if (top >= _engine->_interface->_clip.bottom) {
		return false;
	}
	const int32 bottom = ptr.surface(spriteIndex).h + top;
	if (bottom < _engine->_interface->_clip.top) {
		return false;
	}

	const Common::Point pos(left, top);
	_engine->_frontVideoBuffer.transBlitFrom(ptr.surface(spriteIndex), pos);
	return true;
}

bool Grid::drawGraph(int32 posX, int32 posY, const uint8 *pGraph, bool isSprite) {
	if (_engine->_debugState->_disableGridRendering) {
		return false;
	}
	const Common::Rect &clip = _engine->_interface->_clip;
	TwineScreen &frontVideoBuffer = _engine->_frontVideoBuffer;
	return TwinE::drawGraph(posX, posY, pGraph, isSprite, frontVideoBuffer, clip);
}

const uint8 *Grid::getBlockBufferGround(const IVec3 &pos, int32 &ground) {
	if (_engine->isLBA2() && _engine->_exterior->isActive()) {
		ground = _engine->_exterior->getAltitude(pos.x, pos.z);
		return nullptr;
	}

	const IVec3 &collision = updateCollisionCoordinates(pos.x, pos.y, pos.z);
	const uint8 *ptr = _bufCube + collision.y * 2 + collision.x * SIZE_CUBE_Y * 2 + collision.z * SIZE_CUBE_X * SIZE_CUBE_Y * 2;

	int32 collisionY = collision.y;
	while (collisionY) {
		if (READ_LE_INT16(ptr)) { // found the ground - sizeof(BlockEntry);
			break;
		}
		collisionY--;
		ptr -= sizeof(int16);
	}

	_engine->_collision->_collision.y = collisionY;
	ground = (int16)((collisionY + 1) * SIZE_BRICK_Y);

	return ptr;
}

const BlockDataEntry *Grid::getAdrBlock(int32 blockIdx, int32 brickIdx) const {
	const BlockData *blockPtr = getBlockLibrary(blockIdx);
	return &blockPtr->entries[brickIdx];
}

const BlockData *Grid::getBlockLibrary(int32 blockIdx) const {
	return _currentBlockLibrary.getLayout(blockIdx - 1);
}

void Grid::map2Screen(int32 x, int32 y, int32 z, int32 &posx, int32 &posy) const {
	posx = (x - z) * 24 + _engine->width() / 2 - SIZE_CUBE_X / 2;
	posy = ((x + z) * 12) - (y * 15) + _engine->height() / 2 - SIZE_CUBE_Y;
}

void Grid::drawBrickBlock(int32 blockIdx, int32 brickBlockIdx, int32 x, int32 y, int32 z) { // AffBrickBlock
	const BlockDataEntry *blockPtr = getAdrBlock(blockIdx, brickBlockIdx);
	const uint8 brickShape = blockPtr->brickShape;
	const uint8 brickSound = blockPtr->brickType;
	const uint16 numBrick = blockPtr->brickIdx;
	if (!numBrick) {
		return;
	}

	int32 brickPixelPosX = 0;
	int32 brickPixelPosY = 0;

	map2Screen(x - _startCube.x, y - _startCube.y, z - _startCube.z, brickPixelPosX, brickPixelPosY);

	if (brickPixelPosX < -24) {
		return;
	}
	if (brickPixelPosX >= _engine->width()) {
		return;
	}
	if (brickPixelPosY < -38) {
		return;
	}
	if (brickPixelPosY >= _engine->height()) {
		return;
	}

	// draw the background brick
	drawGraph(numBrick - 1, brickPixelPosX, brickPixelPosY);

	int32 col = (brickPixelPosX + 24) / 24;

	if (_nbBrickColon[col] >= MAX_BRICKS) {
		warning("GRID: brick buffer exceeded");
		return;
	}

	BrickEntry *pColonBrick = &_listBrickColon[col * MAX_BRICKS + _nbBrickColon[col]];

	pColonBrick->x = x;
	pColonBrick->y = y;
	pColonBrick->z = z;
	pColonBrick->posX = brickPixelPosX;
	pColonBrick->posY = brickPixelPosY;
	pColonBrick->index = numBrick - 1;
	pColonBrick->shape = brickShape;
	pColonBrick->sound = brickSound;

	_nbBrickColon[col]++;
}

bool Grid::isExteriorActive() const {
	return _engine->isLBA2() && _engine->_exterior->isActive();
}

void Grid::updateExteriorCamera(bool recenterBeta) {
	const ActorStruct *hero = _engine->_scene->getActor(_engine->_scene->_numObjFollow);
	if (hero == nullptr) {
		return;
	}

	int32 lookBeta = hero->_beta;
	if (_addBetaCam != 0) {
		lookBeta = ClampAngle(lookBeta + _addBetaCam);
	}

	if (recenterBeta) {
		_betaCam = ClampAngle(LBAAngles::ANGLE_180 - lookBeta);
	}

	const IVec2 offset = _engine->_renderer->rotate(0, LBA2_DISTANCE_VISEE, lookBeta);
	_vueOffsetX = hero->_posObj.x + offset.x;
	_vueOffsetY = hero->_posObj.y;
	_vueOffsetZ = hero->_posObj.z + offset.y;

	_startCube.x = _vueOffsetX / SIZE_BRICK_XZ;
	_startCube.y = _vueOffsetY / SIZE_BRICK_Y;
	_startCube.z = _vueOffsetZ / SIZE_BRICK_XZ;

	_worldCube.x = _startCube.x * SIZE_BRICK_XZ;
	_worldCube.y = _startCube.y * SIZE_BRICK_Y;
	_worldCube.z = _startCube.z * SIZE_BRICK_XZ;

	const int32 w = _engine->width();
	const int32 h = _engine->height();
	int32 nearClip = LBA2_CLIP_NEAR;
	if (h > 480) {
		nearClip = LBA2_CLIP_NEAR * 480 / h;
	}

	_engine->_renderer->setProjection(w / 2, h / 2, nearClip, LBA2_CHAMP_X, LBA2_CHAMP_Z);
	_engine->_renderer->setFollowCamera(_vueOffsetX, _vueOffsetY, _vueOffsetZ,
		_alphaCam, _betaCam, _gammaCam, _vueDistance);

	const IVec3 &projPos = _engine->_renderer->projectPoint(0, 0, 0);
	_engine->_redraw->_projPosScreen.x = projPos.x;
	_engine->_redraw->_projPosScreen.y = projPos.y;
}

IVec3 Grid::actorRenderPos(const ActorStruct *actor) const {
	if (isExteriorActive()) {
		return actor->posObj();
	}
	return actor->posObj() - _worldCube;
}

int32 Grid::actorRenderDepth(const ActorStruct *actor) const {
	if (isExteriorActive()) {
		const IVec3 &world = _engine->_renderer->longWorldRot(actor->_posObj.x, actor->_posObj.y, actor->_posObj.z);
		return _engine->_renderer->getCameraRotation().z - world.z;
	}
	return actor->_posObj.x - _worldCube.x + actor->_posObj.z - _worldCube.z;
}

bool Grid::projectActorPoint(const ActorStruct *actor, IVec3 &proj) const {
	if (isExteriorActive()) {
		const IVec3 &world = _engine->_renderer->longWorldRot(actor->_posObj.x, actor->_posObj.y, actor->_posObj.z);
		return _engine->_renderer->longProjectPoint(world, proj);
	}
	proj = _engine->_renderer->projectPoint(actorRenderPos(actor));
	return true;
}

void Grid::redrawGrid() { // AffGrille
	if (isExteriorActive()) {
		updateExteriorCamera(false);
		_engine->_exterior->redraw();
		return;
	}

	_engine->_renderer->setIsoProjection(_engine->width() / 2 - 8 - 1, _engine->height() / 2, SIZE_BRICK_XZ);
	_engine->_renderer->setPosCamera(0, 0, 0);
	_engine->_renderer->setAngleCamera(LBAAngles::ANGLE_0, LBAAngles::ANGLE_0, LBAAngles::ANGLE_0);

	_worldCube.x = _startCube.x * SIZE_BRICK_XZ;
	_worldCube.y = _startCube.y * SIZE_BRICK_Y;
	_worldCube.z = _startCube.z * SIZE_BRICK_XZ;

	const IVec3 &projPos = _engine->_renderer->projectPoint(-_worldCube);
	_engine->_redraw->_projPosScreen.x = projPos.x;
	_engine->_redraw->_projPosScreen.y = projPos.y;

	memset(_nbBrickColon, 0, _brickInfoBufferSize);

	if (!_engine->_scene->_flagRenderGrid) {
		return;
	}

	_engine->_screens->clearScreen();

	for (int32 z = 0; z < SIZE_CUBE_Z; z++) {
		for (int32 x = 0; x < SIZE_CUBE_X; x++) {
			for (int32 y = 0; y < SIZE_CUBE_Y; y++) {
				const BlockEntry entry = getBlockEntry(x, y, z);
				if (entry.blockIdx) {
					drawBrickBlock(entry.blockIdx, entry.brickBlockIdx, x, y, z);
				}
			}
		}
	}
}

BlockEntry Grid::getBlockEntry(int32 xmap, int32 ymap, int32 zmap) const {
	const uint8 *pCube = _bufCube;
	const int32 size = 2; // sizeof(BlockEntry);
	pCube += xmap * SIZE_CUBE_Y * size;
	pCube += ymap * size;
	pCube += zmap * (SIZE_CUBE_X * SIZE_CUBE_Y * size);

	BlockEntry entry;
	entry.blockIdx = *pCube;
	entry.brickBlockIdx = *(pCube + 1);
	return entry;
}

ShapeType Grid::worldColBrick(int32 x, int32 y, int32 z) {
	if (_engine->isLBA2() && _engine->_exterior->isActive()) {
		const int32 groundY = _engine->_exterior->getAltitude(x, z);
		if (groundY > y) {
			return _engine->_exterior->giveTerrainCol(x, z) ? ShapeType::kSolid : ShapeType::kNone;
		}
		return ShapeType::kNone;
	}

	const IVec3 &collision = updateCollisionCoordinates(x, y, z);

	if (collision.x < 0 || collision.x >= SIZE_CUBE_X || collision.z < 0 || collision.z >= SIZE_CUBE_Z) {
		return ShapeType::kNone;
	}

	if (collision.y <= -1) {
		return ShapeType::kSolid;
	}

	if (collision.y >= SIZE_CUBE_Y) {
		return ShapeType::kNone;
	}

	const BlockEntry entry = getBlockEntry(collision.x, collision.y, collision.z);
	if (entry.blockIdx) {
		const BlockDataEntry *blockPtr = getAdrBlock(entry.blockIdx, entry.brickBlockIdx);
		return (ShapeType)blockPtr->brickShape;
	}
	return (ShapeType)entry.brickBlockIdx; // eventually transparent color
}

const IVec3 &Grid::updateCollisionCoordinates(int32 x, int32 y, int32 z) {
	_engine->_collision->_collision.x = (x + DEMI_BRICK_XZ) >> 9;
	_engine->_collision->_collision.y = y >> 8;
	_engine->_collision->_collision.z = (z + DEMI_BRICK_XZ) >> 9;
	return _engine->_collision->_collision;
}

bool Grid::shouldCheckWaterCol(int32 actorIdx) const {
	if (actorIdx == OWN_ACTOR_SCENE_INDEX) {
		ActorStruct *ptrobj = _engine->_scene->getActor(actorIdx);
		if (_engine->_actor->_heroBehaviour != HeroBehaviourType::kProtoPack
		 && ptrobj->_flags.bComputeCollisionWithFloor
		 && !ptrobj->_flags.bIsInvisible
		 && !ptrobj->_workFlags.bIsFalling
		 && ptrobj->_carryBy == -1) {
			return true;
		}
	}

	return false;
}

ShapeType Grid::worldColBrickFull(int32 x, int32 y, int32 z, int32 y2, int32 actorIdx) {
	if (_engine->isLBA2() && _engine->_exterior->isActive()) {
		const int32 groundY = _engine->_exterior->getAltitude(x, z);
		if (groundY > y) {
			if (_engine->_exterior->giveTerrainCol(x, z)) {
				return ShapeType::kSolid;
			}
			if (_engine->_exterior->testDecorCollision(x, y, z)) {
				ActorStruct *actor = _engine->_scene->getActor(actorIdx);
				if (actor != nullptr) {
					actor->setCollision(ShapeType::kSolid);
					actor->_brickSound |= 0x80;
				}
				return ShapeType::kSolid;
			}
		}
		return ShapeType::kNone;
	}

	const IVec3 &collision = updateCollisionCoordinates(x, y, z);

	if (collision.x < 0 || collision.x >= SIZE_CUBE_X || collision.z < 0 || collision.z >= SIZE_CUBE_Z) {
		return ShapeType::kNone;
	}

	if (collision.y <= -1) {
		return ShapeType::kSolid;
	}

	bool checkWater = shouldCheckWaterCol(actorIdx);
	uint8 *pCube = _bufCube;
	pCube += collision.x * SIZE_CUBE_Y * 2;
	pCube += collision.y * 2;
	pCube += collision.z * (SIZE_CUBE_X * SIZE_CUBE_Y * 2);

	uint8 block = *pCube;

	ShapeType brickShape;
	const uint8 tmpBrickIdx = *(pCube + 1);
	if (block) {
		const BlockDataEntry *blockPtr = getAdrBlock(block, tmpBrickIdx);
		if (checkWater && isWaterBlock(blockPtr, _engine->isLBA2())) {
			brickShape = ShapeType::kSolid; // full collision
		} else {
			brickShape = (ShapeType)blockPtr->brickShape;
		}
	} else {
		brickShape = (ShapeType)tmpBrickIdx; // maybe transparency
		if (checkWater) {
			uint8 *pCode = pCube;
			for (y = collision.y - 1; y >= 0; y--) {
				pCode -= 2;
				uint8 code = *pCode;
				if (code) {
					const BlockDataEntry *blockPtr = getAdrBlock(block, 0);
					if (isWaterBlock(blockPtr, _engine->isLBA2())) {
						// Special check mount funfrock
						if (_engine->_scene->_numCube != LBA1SceneId::Polar_Island_on_the_rocky_peak) {
							// full collision
							return ShapeType::kSolid;
						}
					}
					break; // stop parsing at first encountered brick
				}
			}
		}
	}

	int32 ymax = (y2 + (SIZE_BRICK_Y - 1)) / SIZE_BRICK_Y;
	// check full height
	for (y = collision.y; ymax > 0 && y < (SIZE_CUBE_Y - 1); --ymax, y++) {
		pCube += 2;
		if (READ_LE_INT16(pCube)) {
			return ShapeType::kSolid;
		}
	}

	return brickShape;
}

uint8 Grid::worldCodeBrick(int32 x, int32 y, int32 z) {
	if (_engine->isLBA2() && _engine->_exterior->isActive()) {
		return _engine->_exterior->getTerrainBrickCode(x, y, z);
	}

	uint8 code = 0xF0U;
	if (y > -1) {
		const IVec3 &collision = updateCollisionCoordinates(x, y, z);

		const BlockEntry entry = getBlockEntry(collision.x, collision.y, collision.z);
		if (entry.blockIdx) {
			const BlockDataEntry *blockPtr = getAdrBlock(entry.blockIdx, entry.brickBlockIdx);
			code = blockPtr->brickType;
		}
	}

	return code;
}

void Grid::centerOnActor(const ActorStruct *actor, bool recenterCamera) {
	_startCube.x = actor->_posObj.x / SIZE_BRICK_XZ;
	if (recenterCamera) {
		// CameraCenter(0) / CAM_FOLLOW / manual recenter (INTEXT.CPP, GERELIFE.CPP)
		_startCube.y = (actor->_posObj.y + SIZE_BRICK_Y) / SIZE_BRICK_Y;
	} else {
		// CameraCenter(1) on scene load (INTEXT.CPP)
		_startCube.y = actor->_posObj.y / SIZE_BRICK_Y;
	}
	_startCube.z = actor->_posObj.z / SIZE_BRICK_XZ;
	if (isExteriorActive()) {
		updateExteriorCamera(true);
	}
	_engine->_redraw->_firstTime = true;
}

void Grid::centerScreenOnActor() {
	if (_engine->_cameraZone) {
		return;
	}
	if (_engine->_debugState->_useFreeCamera) {
		return;
	}

	ActorStruct *actor = _engine->_scene->getActor(_engine->_scene->_numObjFollow);
	const IVec3 projPos = _engine->_renderer->projectPoint(actor->_posObj.x - (_startCube.x * SIZE_BRICK_XZ),
	                                   actor->_posObj.y - (_startCube.y * SIZE_BRICK_Y),
	                                   actor->_posObj.z - (_startCube.z * SIZE_BRICK_XZ));
	// TODO: these border values should get scaled for higher resolutions
	if (projPos.x < 80 || projPos.x >= _engine->width() - 60 || projPos.y < 80 || projPos.y >= _engine->height() - 50) {
		const int32 xm = actor->_posObj.x / SIZE_BRICK_XZ;
		const int32 zm = actor->_posObj.z / SIZE_BRICK_XZ;

		_startCube.x = xm + (xm - _startCube.x) / 2;
		_startCube.y = actor->_posObj.y / SIZE_BRICK_Y;
		_startCube.z = zm + (zm - _startCube.z) / 2;

		if (_startCube.x >= SIZE_CUBE_X) {
			_startCube.x = SIZE_CUBE_X - 1;
		}

		if (_startCube.z >= SIZE_CUBE_Z) {
			_startCube.z = SIZE_CUBE_Z - 1;
		}

		_engine->_redraw->_firstTime = true;
	}
}

} // namespace TwinE
