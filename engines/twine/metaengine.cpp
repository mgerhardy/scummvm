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

#include "common/savefile.h"
#include "common/system.h"
#include "common/fs.h"
#include "engines/advancedDetector.h"
#include "base/plugins.h"

#include "twine/twine.h"

class TwinEMetaEngine : public AdvancedMetaEngine {
public:
	const char *getName() const override {
		return "TwinE";
	}

	bool hasFeature(MetaEngineFeature f) const override {
		return false;
	}

	bool createInstance(OSystem *syst, Engine **engine, const ADGameDescription *desc) const override {
		if (desc) {
			*engine = new TwinE::TwinEEngine(syst, desc->language, desc->flags);
		}
		return desc != nullptr;
	}
};

#if PLUGIN_ENABLED_DYNAMIC(TwinE)
	REGISTER_PLUGIN_DYNAMIC(TwinE, PLUGIN_TYPE_ENGINE, TwinEMetaEngine);
#else
	REGISTER_PLUGIN_STATIC(TwinE, PLUGIN_TYPE_ENGINE, TwinEMetaEngine);
#endif
