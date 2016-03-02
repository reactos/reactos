/* file.h - Additional file attributes

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

#ifndef _FILE_H
#define _FILE_H

#include "msdos_fs.h"

typedef enum { fdt_none, fdt_drop, fdt_undelete } FD_TYPE;

typedef struct _fptr {
    char name[MSDOS_NAME];
    FD_TYPE type;
    struct _fptr *first;	/* first entry */
    struct _fptr *next;		/* next file in directory */
} FDSC;

extern FDSC *fp_root;

char *file_name(unsigned char *fixed);

/* Returns a pointer to a pretty-printed representation of a fixed MS-DOS file
   name. */

int file_cvt(unsigned char *name, unsigned char *fixed);

/* Converts a pretty-printed file name to the fixed MS-DOS format. Returns a
   non-zero integer on success, zero on failure. */

void file_add(char *path, FD_TYPE type);

/* Define special attributes for a path. TYPE can be either FDT_DROP or
   FDT_UNDELETE. */

FDSC **file_cd(FDSC ** curr, char *fixed);

/* Returns a pointer to the directory descriptor of the subdirectory FIXED of
   CURR, or NULL if no such subdirectory exists. */

FD_TYPE file_type(FDSC ** curr, char *fixed);

/* Returns the attribute of the file FIXED in directory CURR or FDT_NONE if no
   such file exists or if CURR is NULL. */

void file_modify(FDSC ** curr, char *fixed);

/* Performs the necessary operation on the entry of CURR that is named FIXED. */

void file_unused(void);

/* Displays warnings for all unused file attributes. */

#endif
