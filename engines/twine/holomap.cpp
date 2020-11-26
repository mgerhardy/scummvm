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

#include "twine/holomap.h"
#include "common/memstream.h"
#include "common/types.h"
#include "twine/gamestate.h"
#include "twine/hqr.h"
#include "twine/interface.h"
#include "twine/renderer.h"
#include "twine/resources.h"
#include "twine/scene.h"
#include "twine/screens.h"
#include "twine/sound.h"
#include "twine/text.h"
#include "twine/twine.h"

namespace TwinE {

Holomap::Holomap(TwinEEngine *engine) : _engine(engine) {}

bool Holomap::loadLocations() {
	uint8 *locationsPtr;
	const int32 locationsSize = HQR::getAllocEntry(&locationsPtr, Resources::HQR_RESS_FILE, RESSHQR_HOLOARROWINFO);
	if (locationsSize == 0) {
		warning("Could not find holomap locations at index %i in %s", RESSHQR_HOLOARROWINFO, Resources::HQR_RESS_FILE);
		return false;
	}

	Common::MemoryReadStream stream(locationsPtr, locationsSize, DisposeAfterUse::YES);
	_numLocations = locationsSize / sizeof(Location);
	if (_numLocations > NUM_LOCATIONS) {
		warning("Amount of locations (%i) exceeds the maximum of %i", _numLocations, NUM_LOCATIONS);
		return false;
	}

	for (int i = 0; i < _numLocations; i++) {
		_locations[i].x = stream.readUint16LE();
		_locations[i].y = stream.readUint16LE();
		_locations[i].z = stream.readUint16LE();
		_locations[i].textIndex = stream.readUint16LE();
	}
	return true;
}

void Holomap::setHolomapPosition(int32 locationIdx) {
	assert(locationIdx >= 0 && locationIdx <= ARRAYSIZE(_engine->_gameState->holomapFlags));
	_engine->_gameState->holomapFlags[locationIdx] = 0x81;
}

void Holomap::clearHolomapPosition(int32 locationIdx) {
	assert(locationIdx >= 0 && locationIdx <= ARRAYSIZE(_engine->_gameState->holomapFlags));
	_engine->_gameState->holomapFlags[locationIdx] &= 0x7E;
	_engine->_gameState->holomapFlags[locationIdx] |= 0x40;
}

void Holomap::loadGfxSub(uint8 *modelPtr) {
	// TODO
}

void Holomap::loadGfxSub1() {
	// TODO
}

void Holomap::loadGfxSub2() {
	// TODO
}

void Holomap::loadHolomapGFX() {
	uint8 *videoPtr3 = (uint8 *)_engine->workVideoBuffer.getBasePtr(174, 12);
	uint8 *videoPtr4 = (uint8 *)_engine->workVideoBuffer.getBasePtr(78, 13);
	uint8 *videoPtr5 = (uint8 *)_engine->workVideoBuffer.getBasePtr(334, 115);

	HQR::getEntry(videoPtr3, Resources::HQR_RESS_FILE, RESSHQR_HOLOSURFACE);
	HQR::getEntry(videoPtr4, Resources::HQR_RESS_FILE, RESSHQR_HOLOIMG);

	uint8 *videoPtr6 = videoPtr5 + HQR::getEntry(videoPtr5, Resources::HQR_RESS_FILE, RESSHQR_HOLOTWINMDL);
	uint8 *videoPtr7 = videoPtr6 + HQR::getEntry(videoPtr6, Resources::HQR_RESS_FILE, RESSHQR_HOLOARROWMDL);
	uint8 *videoPtr8 = videoPtr7 + HQR::getEntry(videoPtr7, Resources::HQR_RESS_FILE, RESSHQR_HOLOTWINARROWMDL);

	loadGfxSub(videoPtr5);
	loadGfxSub(videoPtr6);
	loadGfxSub(videoPtr7);

	loadGfxSub(videoPtr8);

	// TODO:
	// uint8 *videoPtr1 = (uint8 *)_engine->workVideoBuffer.getPixels();
	// uint8 *videoPtr2 = videoPtr1 + 4488;
	// uint8 *videoPtr11 = videoPtr8 + HQR::getEntry(videoPtr8, Resources::HQR_RESS_FILE, RESSHQR_HOLOPOINTMDL);
	// uint8 *videoPtr10 = videoPtr11 + 4488;
	// uint8 *videoPtr12 = videoPtr10 + HQR::getEntry(videoPtr10, Resources::HQR_RESS_FILE, RESSHQR_HOLOARROWINFO);
	// uint8 *videoPtr13 = videoPtr12 + HQR::getEntry(videoPtr12, Resources::HQR_RESS_FILE, RESSHQR_HOLOPOINTANIM);

	_engine->_screens->loadCustomPalette(RESSHQR_HOLOPAL);

	int32 j = 576;
	for (int32 i = 0; i < 96; i += 3, j += 3) {
		paletteHolomap[i + 0] = _engine->_screens->palette[j + 0];
		paletteHolomap[i + 1] = _engine->_screens->palette[j + 1];
		paletteHolomap[i + 2] = _engine->_screens->palette[j + 2];
	}

	j = 576;
	for (int32 i = 96; i < 189; i += 3, j += 3) {
		paletteHolomap[i + 0] = _engine->_screens->palette[j + 0];
		paletteHolomap[i + 1] = _engine->_screens->palette[j + 1];
		paletteHolomap[i + 2] = _engine->_screens->palette[j + 2];
	}

	loadGfxSub1();
	loadGfxSub2();

	needToLoadHolomapGFX = 0;
}

void Holomap::drawHolomapTitle(int32 width, int32 height) {
	// TODO
#if 0
	int32 v2;  // ebp@0
	int32 v3;  // ebx@1
	int32 v5;  // [sp+0h] [bp-118h]@1
	int32 v6;  // [sp+4h] [bp-114h]@1
	int32 v7;  // [sp+100h] [bp-18h]@1
	int32 v8;  // [sp+104h] [bp-14h]@1
	int16 v9;  // [sp+108h] [bp-10h]@1
	int16 v10; // [sp+10Ch] [bp-Ch]@1
	int16 v11; // [sp+110h] [bp-8h]@1
	int16 v12; // [sp+114h] [bp-4h]@1
	int16 v13; // [sp+12Ch] [bp+14h]@1

	v12 = width - 315;
	v10 = width + 315;
	v11 = height - 20;
	v5 = *(_DWORD *)"HoloMap";
	v6 = *(_DWORD *)"Map";
	v9 = height + 20;
	v13 = width - SizeFont((int)&v5) / 2;
	CoulFont(0xCAu);
	v3 = (int32)(height - 18);
	v8 = v3 - 1;
	Font(v2);
	Font(v2);
	v7 = v13 + 1;
	Font(v2);
	v8 = v3 + 1;
	Font(v2);
	Font(v2);
	Font(v2);
	Font(v2);
	Font(v2);
	CoulFont(0xFu);
	Font(v2);
	_engine->copyBlockPhys(v2);
#endif
}

void Holomap::drawHolomapTrajectory(int32 trajectoryIndex) {
	debug("Draw trajectory index %i", trajectoryIndex);
	// TODO
#if 0
	int32 v5;      // ebp@0
	int32 v6;      // edx@4
	int32 v7;      // ecx@4
	int32 v8;      // ebx@4
	int32 v9;      // eax@4
	int16 v10;     // si@6
	unint3216 v11; // si@6
	int32 v12;     // edi@6
	int16 v13;     // bx@6
	int32 v14;     // ebx@6
	int32 v15;     // edx@6
	int32 v16;     // ecx@6
	int16 v17;     // si@6
	int16 v18;     // ax@11
	int32 v19;     // edx@16
	int32 v20;     // edx@22
	int32 v21;     // ecx@22
	int32 v23;     // [sp-4h] [bp-30h]@2
	int32 v24;     // [sp+0h] [bp-2Ch]@6
	int32 v25;     // [sp+4h] [bp-28h]@1
	int32 v26;     // [sp+8h] [bp-24h]@1
	int16 v27;     // [sp+10h] [bp-1Ch]@6
	int16 v28;     // [sp+18h] [bp-14h]@1
	int16 v29;     // [sp+24h] [bp-8h]@6

	_engine->freezeTime();
	v26 = reinitVar1;
	v28 = 1;
	v25 = reinitVar2;
	if (useAlternatePalette)
		v23 = (int)palette;
	else
		v23 = menuPal;
	FadeToBlack(v23);
	UnSetClip();
	Cls();
	_engine->flip(v6, v7, a3, v5);
	loadHolomapGFX(v5);
	v8 = 0;
	v9 = videoPtr12;
	if (a5) {
		do {
			++v8;
			v9 += 4 * *(_WORD *)(v9 + 12) + 14;
		} while (v8 != a5);
	}
	v10 = *(_WORD *)(v9 + 4);
	word_5CDB0 = *(_WORD *)(v9 + 6);
	word_5CDB2 = *(_WORD *)(v9 + 8);
	v11 = 2 * v10 + 31;
	v29 = *(_WORD *)(v9 + 12);
	v12 = videoPtr13;
	word_5CDB4 = *(_WORD *)(v9 + 10);
	v13 = Load_HQR((int)"ress.hqr", videoPtr13, v11);
	loadGfxSub(v5);
	v14 = v12 + v13;
	Load_HQR((int)"ress.hqr", v14, v11 + 1);
	setCameraPosition(400, 240, 128, 1024, 1024);
	setCameraAngle(v5);
	sub_24344();
	Flip(v15, v16, v14, v5);
	v27 = 0;
	v17 = 0;
	sub_25138();
	v24 = time;
	while (skipIntro != 1) {
		waitRetrace();
		if (!v28) {
			setPalette2(192, 32, (int)&palette2[3 * needToLoadHolomapGFX++]);
			if (needToLoadHolomapGFX == 32)
				needToLoadHolomapGFX = 0;
		}
		v18 = GetRealAngle((int)&timeVar);
		if (!timeVar.numOfStep)
			setActorAngleSafe(v18, v18 - 256, 500, (int)&timeVar);
		if (SetInterAnimObjet(v5)) {
			++v27;
			if (v27 == GetNbFramesAnim(v14))
				v27 = GetBouclageAnim(v14);
		}
		setCameraPosition(100, 400, 128, 900, 900);
		setCameraAngle(v5);
		SetLightVector(v5);
		Box(v5);
		AffObjetIso(v19, v5);
		_engine->copyBlockPhys(v5);
		setCameraPosition(400, 240, 128, 1024, 1024);
		setCameraAngle(v5);
		SetLightVector(v5);
		if (v24 + 40 <= (unint32)time) {
			v24 = time;
			if (v17 >= v29 && v17 > v29)
				break;
			++v17;
			sub_25138();
		}
		if (v28) {
			v28 = 0;
			FadeToPal((int)palette);
		}
	}
	FadeToBlack((int)palette);
	Cls();
	Flip(v20, v21, v14, v5);
	reinitVar1 = v26;
	reinitVar2 = v25;
	reinitAll1(v5);
	_engine->unfreezeTime();
#endif
}

#if 0
int32 sub_2475C(int32 a1) {
	int32 result = a1;
	if (a1 == -1) {
		result = 150;
	}
	while (1) {
		--result;
		if (result < 0) {
			break;
		}
		if (GV14[result] & 0x81) {
			return result;
		}
	}
	return -1;
}
#endif

void Holomap::processHolomap() {
	ScopedEngineFreeze freeze(_engine);

	// TODO memcopy palette

	const int32 alphaLightTmp = _engine->_scene->alphaLight;
	const int32 betaLightTmp = _engine->_scene->betaLight;

	_engine->_screens->fadeToBlack(_engine->_screens->paletteRGBA);
	_engine->_sound->stopSamples();
	_engine->_interface->resetClip();
	_engine->_screens->clearScreen();
	_engine->flip();
	_engine->_screens->copyScreen(_engine->frontVideoBuffer, _engine->workVideoBuffer);

	loadHolomapGFX();
	drawHolomapTitle(SCREEN_WIDTH / 2, 25);
	_engine->_renderer->setCameraPosition(SCREEN_WIDTH / 2, 190, 128, 1024, 1024);

	_engine->_text->initTextBank(TextBankId::Inventory_Intro_and_Holomap);
	_engine->_text->setFontCrossColor(9);

	// TODO

	_engine->_text->drawTextBoxBackground = true;
	_engine->_screens->fadeToBlack(_engine->_screens->paletteRGBA);
	_engine->_scene->alphaLight = alphaLightTmp;
	_engine->_scene->betaLight = betaLightTmp;
	_engine->_gameState->initEngineVars();

	_engine->_text->initTextBank(_engine->_scene->sceneTextBank + 3);

	// TODO memcopy reset palette

#if 0
	int v0;        // ebp@0
	int16 v1;      // bx@1
	int v2;        // eax@2
	char v3;       // dl@2
	int v4;        // eax@3
	int v5;        // edx@3
	int v6;        // edx@3
	int v7;        // ecx@3
	int v8;        // eax@3
	int v9;        // eax@3
	int32 v10;     // esi@3
	int32 v11;     // eax@24
	int v12;       // ebx@29
	int16 v13;     // bx@31
	int16 v14;     // bx@31
	int v15;       // eax@31
	int32 v16;     // eax@33
	int v17;       // ebx@34
	int16 v18;     // bx@36
	int16 v19;     // bx@36
	int v20;       // eax@36
	int16 v21;     // di@45
	int16 v22;     // bx@45
	int16 v23;     // bx@62
	int v24;       // eax@63
	char v25;      // dl@63
	byte v27[768]; // [sp+0h] [bp-348h]@2
	int v28;       // [sp+300h] [bp-48h]@3
	int v29;       // [sp+304h] [bp-44h]@3
	int v30;       // [sp+308h] [bp-40h]@3
	int v31;       // [sp+30Ch] [bp-3Ch]@3
	int v32;       // [sp+310h] [bp-38h]@3
	int v33;       // [sp+314h] [bp-34h]@3
	int v34;       // [sp+318h] [bp-30h]@45
	int16 v35;     // [sp+31Ch] [bp-2Ch]@3
	int16 v36;     // [sp+320h] [bp-28h]@3
	int16 v37;     // [sp+324h] [bp-24h]@1
	int16 v38;     // [sp+328h] [bp-20h]@1
	int16 v39[2];  // [sp+32Ch] [bp-1Ch]@3
	int16 v40;     // [sp+330h] [bp-18h]@3
	int16 v41;     // [sp+334h] [bp-14h]@3
	int16 v42;     // [sp+338h] [bp-10h]@3
	int16 v43;     // [sp+33Ch] [bp-Ch]@1
	int16 v44;     // [sp+340h] [bp-8h]@1
	int16 v45[2];  // [sp+344h] [bp-4h]@3

	v38 = 1;
	v44 = 3;
	v1 = 0;
	v37 = 0;
	v43 = 0;
	_engine->freezeTime();
	do {
		v2 = v1;
		v3 = palette[v1++];
		v27[v2] = v3;
	} while (v1 < 768);
	v30 = reinitVar1;
	v29 = reinitVar2;
	LOWORD(v4) = FadeToBlack(menuPal);
	HQ_StopSample(v4, v5, v1);
	UnSetClip();
	Cls();
	Flip(v6, v7, v1, v0);
	CopyScreen((const void *)frontVideoBuffer, (void *)workVideoBuffer);
	loadHolomapGFX(v0);
	drawHolomapName(320, 25);
	setCameraPosition(320, 190, 128, 1024, 1024);
	v8 = languageCD1;
	languageCD1 = 0;
	v28 = v8;
	InitDial(2);
	TestCoulDial(9);
	v9 = 8 * currentRoom;
	v40 = 1;
	v32 = *(_WORD *)(v9 + videoPtr10 + 6);
	v31 = time;
	v35 = *(_WORD *)(v9 + videoPtr10) & 0x3FF;
	LOWORD(v9) = *(_WORD *)(videoPtr10 + v9 + 2);
	BYTE1(v9) &= 3u;
	v41 = v9;
	v33 = currentRoom;
	v42 = v35;
	v39[0] = v35;
	v10 = 0;
	v36 = v9;
	v45[0] = v9;
	while (skipIntro != 1 && skipIntro != 35 && !(key1 & 2)) {
		if (v43) {
			if (!printTextVar12 && !key1)
				v43 = 0;
		} else {
			mainLoopVar7 = skipIntro;
			mainLoopVar5 = key1;
			key = printTextVar12;
			if (key1 & 4) {
				if (v44 != 1 && !v10) {
					if (key & 1)
						v39[0] -= 8;
					if (key & 2)
						v39[0] += 8;
					if (key & 4)
						v45[0] -= 8;
					if (key & 8)
						v45[0] += 8;
				}
				HIBYTE(v39[0]) &= 3u;
				HIBYTE(v45[0]) &= 3u;
			} else {
				if (key & 8) {
					v11 = v33;
					while (1) {
						++v11;
						if (v11 >= 150)
							break;
						if (GV14[v11] & 0x81)
							goto LABEL_28;
					}
					v11 = -1;
				LABEL_28:
					v33 = v11;
					if (v11 == -1) {
						v42 = v39[0];
						v36 = v45[0];
						v12 = videoPtr10;
						v31 = time;
						v11 = currentRoom;
					} else {
						v42 = v39[0];
						v36 = v45[0];
						v31 = time;
						v12 = videoPtr10;
					}
					v13 = *(_WORD *)(v12 + 8 * v11);
					HIBYTE(v13) &= 3u;
					v35 = v13;
					v14 = *(_WORD *)(videoPtr10 + 8 * v11 + 2);
					HIBYTE(v14) &= 3u;
					v41 = v14;
					v15 = *(_WORD *)(videoPtr10 + 8 * v11 + 6);
					v44 = 3;
					v32 = v15;
					v10 = 1;
					v43 = 1;
				}
				if (key & 4) {
					v16 = sub_2475C(v33);
					v33 = v16;
					if (v16 == -1) {
						v42 = v39[0];
						v36 = v45[0];
						v17 = videoPtr10;
						v31 = time;
						v16 = currentRoom;
					} else {
						v42 = v39[0];
						v36 = v45[0];
						v31 = time;
						v17 = videoPtr10;
					}
					v18 = *(_WORD *)(v17 + 8 * v16);
					HIBYTE(v18) &= 3u;
					v35 = v18;
					v19 = *(_WORD *)(videoPtr10 + 8 * v16 + 2);
					HIBYTE(v19) &= 3u;
					v41 = v19;
					v20 = *(_WORD *)(videoPtr10 + 8 * v16 + 6);
					v44 = 3;
					v32 = v20;
					v10 = 1;
					v43 = 1;
				}
			}
		}
		if (v10) {
			v39[0] = BoundRegleTrois(v42, v35, 75, time - v31);
			v40 = 1;
			v45[0] = BoundRegleTrois(v36, v41, 75, time - v31);
		}
		if (!v38) {
			setPalette2(192, 32, (int)&palette2[3 * needToLoadHolomapGFX++]);
			if (needToLoadHolomapGFX == 32)
				needToLoadHolomapGFX = 0;
		}
		if (v44 != 1)
			v40 = 1;
		if (v40 == 1) {
			Box(v0);
			v21 = v45[0];
			v22 = v39[0];
			v34 = v37;
			sub_268CB(v0);
			SetLightVector(v0);
			sub_244EC(v22, v21, v34, 0);
			sub_268CB(v0);
			setSomething3Var12 = 0;
			setSomething3Var14 = 0;
			setSomething3Var16 = 9500;
			sub_24344();
			v40 = 0;
			sub_244EC(v22, v21, v34, 1);
			if (v10)
				Rect();
			CopyBlockPhys(v0);
		}
		if (v10 && v35 == v39[0] && v41 == v45[0])
			v10 = 0;
		if (v44 == 3) {
			v44 = 0;
			OpenDial(v32);
		}
		if (v44 != 2)
			v44 = printText10();
		if (mainLoopVar5 & 1) {
			if (v44 == 2)
				v44 = 0;
			else
				OpenDial(v32);
		}
		if (v38) {
			v38 = 0;
			FadeToPal((int)palette);
		}
	}
	newGameVar4 = 1;
	FadeToBlack((int)palette);
	reinitVar1 = v30;
	reinitVar2 = v29;
	reinitAll1(v0);
	languageCD1 = v28;
	v23 = 0;
	InitDial(currentTextBank + 3);
	do {
		v24 = v23;
		v25 = v27[v23++];
		palette[v24] = v25;
	} while (v23 < 768);
	_engine->unfreezeTime();
#endif
}

} // namespace TwinE
