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

#include "ags/shared/gfx/image.h"
#include "ags/lib/allegro.h"
#include "common/str.h"
#include "common/textconsole.h"

namespace AGS3 {

BITMAP *load_bmp_pf(PACKFILE *f, color *pal) {
	error("TODO: load_bmp_pf");
}

BITMAP *load_pcx_pf(PACKFILE *f, color *pal) {
	error("TODO: load_pcx_pf");
}

BITMAP *load_tga_pf(PACKFILE *f, color *pal) {
	error("TODO: load_tga_pf");
}

BITMAP *load_bmp(const char *filename, color *pal) {
	error("TODO: load_bmp");
}

BITMAP *load_lbm(const char *filename, color *pal) {
	error("TODO: load_lbm");
}

BITMAP *load_pcx(const char *filename, color *pal) {
	error("TODO: load_pcx");
}

BITMAP *load_tga(const char *filename, color *pal) {
	error("TODO: load_tga");
}

BITMAP *load_bitmap(const char *filename, color *pal) {
	Common::String fname(filename);

	if (fname.hasSuffixIgnoreCase(".bmp"))
		return load_bmp(filename, pal);
	else if (fname.hasSuffixIgnoreCase(".lbm"))
		return load_lbm(filename, pal);
	else if (fname.hasSuffixIgnoreCase(".pcx"))
		return load_pcx(filename, pal);
	else if (fname.hasSuffixIgnoreCase(".tga"))
		return load_tga(filename, pal);
	else
		error("Unknown image file - %s", filename);
}

int save_bitmap(const char *filename, BITMAP *bmp, const RGB *pal) {
	error("TODO: save_bitmap");
}

} // namespace AGS