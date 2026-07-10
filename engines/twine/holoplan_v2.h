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

#ifndef TWINE_HOLOPLANV2_H
#define TWINE_HOLOPLANV2_H

#include "twine/holomap_v2.h"

#include "twine/parser/body.h"

namespace TwinE {

#define HOLOPLAN_SCE 32768
#define HOLOPLAN_ZOOM_A 2048
#define HOLOPLAN_ZOOM_D 256

class HoloPlanV2 {
private:
	TwinEEngine *_engine = nullptr;
	HolomapV2 *_holomap = nullptr;

	int32 _zoomedIsland = 0;
	int32 _numObjectif = -1;
	int32 _xpObjectif = 0;
	int32 _ypObjectif = 0;
	int32 _sizeRet = 16;
	bool _flagHoloEnd = false;

	uint8 *_mapImage = nullptr;
	int32 _mapImageSize = 0;
	uint8 *_mapData = nullptr;

	BodyData _bigArrowBody;
	BodyData _smallArrowBody;
	BodyData _twinsenBody;

	struct HoloPlanSortEntry {
		int32 z = 0;
		int32 x = 0;
		int32 y = 0;
		int32 zWorld = 0;
		int32 type = 0; // 0=twinsen, 1=big arrow, 2=small arrow
		int32 index = 0;
	};

	void drawSortedEntry(const HoloPlanSortEntry &entry);
	void initPlan(int32 island);
	void initObjectifIsland();
	void drawListHoloPlan();
	void drawReticule();
	void searchObjectifIsland(int32 dir);
	int32 scaleBody(int32 island) const;

public:
	HoloPlanV2(TwinEEngine *engine, HolomapV2 *holomap);
	~HoloPlanV2();

	void holoPlan(int32 island);
};

} // namespace TwinE

#endif
