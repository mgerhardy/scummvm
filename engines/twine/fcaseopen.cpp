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

#include "fcaseopen.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <direct.h>
#else
#include <dirent.h>
#include <unistd.h>
#endif

#ifndef WIN32

// r must have strlen(path) + 2 bytes
static int casepath(char const *path, char *r) {
	size_t l = strlen(path);
	char *p = (char*)alloca(l + 1);
	strcpy(p, path);
	size_t rl = 0;

	DIR *d;
	if (p[0] == '/') {
		d = opendir("/");
		p = p + 1;
	} else {
		d = opendir(".");
		r[0] = '.';
		r[1] = 0;
		rl = 1;
	}

	int last = 0;
	char *c = strsep(&p, "/");
	while (c) {
		if (!d) {
			return 0;
		}

		if (last) {
			closedir(d);
			return 0;
		}

		r[rl] = '/';
		rl += 1;
		r[rl] = 0;

		struct dirent *e = readdir(d);
		while (e) {
			if (strcasecmp(c, e->d_name) == 0) {
				strcpy(r + rl, e->d_name);
				rl += strlen(e->d_name);

				closedir(d);
				d = opendir(r);

				break;
			}

			e = readdir(d);
		}

		if (!e) {
			strcpy(r + rl, c);
			rl += strlen(c);
			last = 1;
		}

		c = strsep(&p, "/");
	}

	if (d)
		closedir(d);
	return 1;
}
#endif

FILE *fcaseopen(char const *path, char const *mode) {
	FILE *f = fopen(path, mode);
#ifndef WIN32
	if (!f) {
		char *r = (char*)alloca(strlen(path) + 2);
		if (casepath(path, r)) {
			f = fopen(r, mode);
		}
	}
#endif
	return f;
}

void casechdir(char const *path) {
#ifndef WIN32
	char *r = (char*)alloca(strlen(path) + 2);
	if (casepath(path, r)) {
		chdir(r);
	} else {
		errno = ENOENT;
	}
#else
	_chdir(path);
#endif
}
