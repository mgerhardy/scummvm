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

#ifndef TWINE_HOLOMAPV2_H
#define TWINE_HOLOMAPV2_H

#include "twine/holomap.h"

#define MAX_OBJECTIF 50
#define MAX_CUBE 255
#define MAX_PLANET 7

namespace TwinE {

/**
 * The Holomap shows the hero position. The arrows (@c RESSHQR_HOLOARROWMDL) represent important places in your quest - they automatically disappear once that part of
 * the quest is done (@c clrHoloPos()). You can rotate the holoamp by pressing ctrl+cursor keys - but only using the cursor keys, you can scroll through the
 * text for the visible arrows.
 */
class HolomapV2 : public Holomap {
private:
	using Super = Holomap;

public:
	HolomapV2(TwinEEngine *engine) : Super(engine) {}
	virtual ~HolomapV2();

	struct Location {
		int32 x = 0; // Position Island X Y Z
		int32 y = 0;
		int32 z = 0;
		int32 alpha = 0; // Position Planet Alpha, Beta and Altitude
		int32 beta = 0;
		int32 alt = 0;
		int32 mess = 0;
		int8 objFix = 0;    // Eventual Obj Inventory 3D (FREE NOT USED!)
		uint8 flagHolo = 0u; // Flag for Planet display, active, etc.
		uint8 planet = 0u;
		uint8 island = 0u;
	};
	static_assert(sizeof(Location) == 32, "Invalid Location size");
	Location _locations[MAX_OBJECTIF + MAX_CUBE];

	uint32 _decalTimerRef[MAX_OBJECTIF + MAX_CUBE];

	int32 _numObjectif = -1;
	int32 _nextObjectif = -1;
	int32 _oldObjectif = -2;

	int32 _holoAlpha = 0;
	int32 _holoBeta = 0;
	int32 _holoGamma = 0;

	int32 _destAlpha = 0;
	int32 _destBeta = 0;

	int32 _zoomedIsland = -1;
	int32 _destination = 0; // P_TWINSUN

	int32 _holoMode = 0; // 0 Globe, 1 Plan

	uint8 _rotPal[(32 + 31) * 3];
	int16 _rotPalPos = 0;

	uint8 *_ptrMapping = nullptr;
	uint8 *_ptrGlobe = nullptr;
	uint16 *_ptrCoorGlobe = nullptr;
	uint8 *_bufFleche = nullptr;
	uint8 *_bufLoFleche = nullptr;
	uint8 *_bufBuggy = nullptr;
	uint8 *_bufDyno = nullptr;

	struct Planet {
		int32 xSpace = 0;
		int32 ySpace = 0;
		int32 zSpace = 0;
		int16 rayon = 0;
		int16 zoom = 0;
		uint8 *ptrTexture = nullptr; // HMG
		uint8 *ptrHeightMap = nullptr; // HMT
	};
	Planet _tabPlanet[MAX_PLANET];

	void initHoloMalloc();
	void holoPlan(int32 numplan);
	void computeCoorGlobe();
	void drawHolomap();
	void holoSpace();

	/**
	 * Set Holomap location position
	 * @param locationIdx Scene where position must be set
	 */
	bool setHoloPos(int32 locationIdx) override;

	bool loadLocations() override;

	const char *getLocationName(int index) const override;

	/**
	 * Clear Holomap location position
	 * @param locationIdx Scene where position must be cleared
	 */
	void clrHoloPos(int32 locationIdx) override;

	void holoTraj(int32 trajectoryIndex) override;

	/** Load Holomap content */
	void initHoloDatas() override;

	/** Main holomap process loop */
	void holoMap() override;

	struct HolomapProjectedPos {
		int16 x, y, z;
		int16 u, v;
	};
	HolomapProjectedPos *_projectedPoints = nullptr;

	struct HolomapPoly {
		int16 z;
		int16 p1, p2, p3, p4; // Indices into _projectedPoints
	};
	HolomapPoly *_polygons = nullptr;
};

} // namespace TwinE

#endif
