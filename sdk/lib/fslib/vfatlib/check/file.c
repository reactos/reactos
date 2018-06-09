/* file.c - Additional file attributes

   Copyright (C) 1993 Werner Almesberger <werner.almesberger@lrc.di.epfl.ch>
   Copyright (C) 1998 Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de>
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

/* FAT32, VFAT, Atari format support, and various fixes additions May 1998
 * by Roman Hodek <Roman.Hodek@informatik.uni-erlangen.de> */

#include "vfatlib.h"

#define NDEBUG
#include <debug.h>

FDSC *fp_root = NULL;

static void put_char(char **p, unsigned char c)
{
#ifndef __REACTOS__
    if (dos_char_to_printable(p, c))
	return;
#endif
    if ((c >= ' ' && c < 0x7f) || c >= 0xa0)
	*(*p)++ = c;
    else {
	*(*p)++ = '\\';
	*(*p)++ = '0' + (c >> 6);
	*(*p)++ = '0' + ((c >> 3) & 7);
	*(*p)++ = '0' + (c & 7);
    }
}

/**
 * Construct the "pretty-printed" representation of the name in a short directory entry.
 *
 * @param[in]    fixed  Pointer to name[0] of a DIR_ENT
 *
 * @return  Pointer to static string containing pretty "8.3" equivalent of the
 *          name in the directory entry.
 */
char *file_name(unsigned char *fixed)
{
    static char path[MSDOS_NAME * 4 + 2];
    char *p;
    int i, j;

    p = path;
    for (i = j = 0; i < 8; i++)
	if (fixed[i] != ' ') {
	    while (j++ < i)
		*p++ = ' ';
	    put_char(&p, fixed[i]);
	}
    if (strncmp((const char *)(fixed + 8), "   ", 3)) {
	*p++ = '.';
	for (i = j = 0; i < 3; i++)
	    if (fixed[i + 8] != ' ') {
		while (j++ < i)
		    *p++ = ' ';
		put_char(&p, fixed[i + 8]);
	    }
    }
    *p = 0;
    return path;
}

int file_cvt(unsigned char *name, unsigned char *fixed)
{
    unsigned char c;
    int size, ext, cnt;

    size = 8;
    ext = 0;
    while (*name) {
	c = *name;
	if (c < ' ' || c > 0x7e || strchr("*?<>|\"/", c)) {
	    printf("Invalid character in name. Use \\ooo for special "
		   "characters.\n");
	    return 0;
	}
	if (c == '.') {
	    if (ext) {
		printf("Duplicate dots in name.\n");
		return 0;
	    }
	    while (size--)
		*fixed++ = ' ';
	    size = 3;
	    ext = 1;
	    name++;
	    continue;
	}
	if (c == '\\') {
	    c = 0;
	    name++;
	    for (cnt = 3; cnt; cnt--) {
		if (*name < '0' || *name > '7') {
		    printf("Expected three octal digits.\n");
		    return 0;
		}
		c = c * 8 + *name++ - '0';
	    }
	    name--;
	}
	if (islower(c))
	    c = toupper(c);
	if (size) {
	    *fixed++ = c;
	    size--;
	}
	name++;
    }
    if (*name || size == 8)
	return 0;
    if (!ext) {
	while (size--)
	    *fixed++ = ' ';
	size = 3;
    }
    while (size--)
	*fixed++ = ' ';
    return 1;
}

void file_add(char *path, FD_TYPE type)
{
    FDSC **current, *walk;
    char name[MSDOS_NAME];
    char *here;

    current = &fp_root;
    if (*path != '/')
	die("%s: Absolute path required.", path);
    path++;
    while (1) {
	if ((here = strchr(path, '/')))
	    *here = 0;
	if (!file_cvt((unsigned char *)path, (unsigned char *)name))
	    exit(2);
	for (walk = *current; walk; walk = walk->next)
	    if (!here && (!strncmp(name, walk->name, MSDOS_NAME) || (type ==
								     fdt_undelete
								     &&
								     !strncmp
								     (name + 1,
								      walk->name
								      + 1,
								      MSDOS_NAME
								      - 1))))
		die("Ambiguous name: \"%s\"", path);
	    else if (here && !strncmp(name, walk->name, MSDOS_NAME))
		break;
	if (!walk) {
	    walk = alloc(sizeof(FDSC));
	    strncpy(walk->name, name, MSDOS_NAME);
	    walk->type = here ? fdt_none : type;
	    walk->first = NULL;
	    walk->next = *current;
	    *current = walk;
	}
	current = &walk->first;
	if (!here)
	    break;
	*here = '/';
	path = here + 1;
    }
}

FDSC **file_cd(FDSC ** curr, char *fixed)
{
    FDSC **walk;

    if (!curr || !*curr)
	return NULL;
    for (walk = curr; *walk; walk = &(*walk)->next)
	if (!strncmp((*walk)->name, fixed, MSDOS_NAME) && (*walk)->first)
	    return &(*walk)->first;
    return NULL;
}

static FDSC **file_find(FDSC ** dir, char *fixed)
{
    if (!dir || !*dir)
	return NULL;
    if (*(unsigned char *)fixed == DELETED_FLAG) {
	while (*dir) {
	    if (!strncmp((*dir)->name + 1, fixed + 1, MSDOS_NAME - 1)
		&& !(*dir)->first)
		return dir;
	    dir = &(*dir)->next;
	}
	return NULL;
    }
    while (*dir) {
	if (!strncmp((*dir)->name, fixed, MSDOS_NAME) && !(*dir)->first)
	    return dir;
	dir = &(*dir)->next;
    }
    return NULL;
}

/* Returns the attribute of the file FIXED in directory CURR or FDT_NONE if no
   such file exists or if CURR is NULL. */
FD_TYPE file_type(FDSC ** curr, char *fixed)
{
    FDSC **this;

    if ((this = file_find(curr, fixed)))
	return (*this)->type;
    return fdt_none;
}

void file_modify(FDSC ** curr, char *fixed)
{
    FDSC **this, *next;

    if (!(this = file_find(curr, fixed)))
	die("Internal error: file_find failed");
    switch ((*this)->type) {
    case fdt_drop:
	printf("Dropping %s\n", file_name((unsigned char *)fixed));
	*(unsigned char *)fixed = DELETED_FLAG;
	break;
    case fdt_undelete:
	*fixed = *(*this)->name;
	printf("Undeleting %s\n", file_name((unsigned char *)fixed));
	break;
    default:
	die("Internal error: file_modify");
    }
    next = (*this)->next;
    free(*this);
    *this = next;
}

static void report_unused(FDSC * this)
{
    FDSC *next;

    while (this) {
	next = this->next;
	if (this->first)
	    report_unused(this->first);
	else if (this->type != fdt_none)
	    printf("Warning: did not %s file %s\n", this->type == fdt_drop ?
		   "drop" : "undelete", file_name((unsigned char *)this->name));
	free(this);
	this = next;
    }
}

void file_unused(void)
{
    report_unused(fp_root);
}
