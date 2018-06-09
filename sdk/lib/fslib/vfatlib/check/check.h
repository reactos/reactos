/* check.h - Check and repair a PC/MS-DOS filesystem

   Copyright (C) 1993 Werner Almesberger <werner.almesberger@lrc.di.epfl.ch>
   Copyright (C) 2008-2014 Daniel Baumann <mail@daniel-baumann.ch>

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program. If not, see <http://www.gnu.org/licenses/>.

   The complete text of the GNU General Public License
   can be found in /usr/share/common-licenses/GPL-3 file.
*/

#ifndef _CHECK_H
#define _CHECK_H

off_t alloc_rootdir_entry(DOS_FS * fs, DIR_ENT * de, const char *pattern, int gen_name);

/* Allocate a free slot in the root directory for a new file. If gen_name is
   true, the file name is constructed after 'pattern', which must include a %d
   type format for printf and expand to exactly 11 characters. The name
   actually used is written into the 'de' structure, the rest of *de is cleared.
   The offset returned is to where in the filesystem the entry belongs. */

int scan_root(DOS_FS * fs);

/* Scans the root directory and recurses into all subdirectories. See check.c
   for all the details. Returns a non-zero integer if the filesystem has to
   be checked again. */

#endif
