/* common.c - Common functions

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

typedef struct _link {
    void *data;
    struct _link *next;
} LINK;

#ifdef __REACTOS__
DECLSPEC_NORETURN // __attribute((noreturn))
void exit(int exitcode)
{
    // DbgBreakPoint();
    NtTerminateProcess(NtCurrentProcess(), exitcode);

    /* Should never get here */
    while(1);
}

DECLSPEC_NORETURN // __attribute((noreturn))
void die_func(const char *msg, ...) // die
#else
void die(const char *msg, ...)
#endif
{
    va_list args;

    va_start(args, msg);
#ifndef __REACTOS__
    vfprintf(stderr, msg, args);
#else
    DPRINT1("Unrecoverable problem!\n");
    VfatPrintV((char*)msg, args);
#endif
    va_end(args);
#ifndef __REACTOS__
    fprintf(stderr, "\n");
#endif
    exit(1);
}

#ifdef __REACTOS__
DECLSPEC_NORETURN // __attribute((noreturn))
void pdie_func(const char *msg, ...) // pdie
#else
void pdie(const char *msg, ...)
#endif
{
    va_list args;

    va_start(args, msg);
#ifndef __REACTOS__
    vfprintf(stderr, msg, args);
#else
    DPRINT1("Unrecoverable problem!\n");
    VfatPrintV((char*)msg, args);
#endif
    va_end(args);
#ifndef __REACTOS__
    fprintf(stderr, ":%s\n", strerror(errno));
#endif
    exit(1);
}

#ifndef __REACTOS__
void *alloc(int size)
{
    void *this;

    if ((this = malloc(size)))
	return this;
    pdie("malloc");
    return NULL;		/* for GCC */
}
#else
void *vfalloc(int size)
{
    void *ptr;

    ptr = RtlAllocateHeap(RtlGetProcessHeap(), 0, size);
    if (ptr == NULL)
    {
        DPRINT1("Allocation failed!\n");
        pdie("malloc");
        return NULL;
    }

    return ptr;
}

void *vfcalloc(int size, int count)
{
    void *ptr;

    ptr = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, size * count);
    if (ptr == NULL)
    {
        DPRINT1("Allocation failed!\n");
        return NULL;
    }

    return ptr;
}

void vffree(void *ptr)
{
    RtlFreeHeap(RtlGetProcessHeap(), 0, ptr);
}
#endif

void *qalloc(void **root, int size)
{
    LINK *link;

#ifndef __REACTOS__
    link = alloc(sizeof(LINK));
#else
    link = vfalloc(sizeof(LINK));
#endif
    link->next = *root;
    *root = link;
#ifndef __REACTOS__
    return link->data = alloc(size);
#else
    return link->data = vfalloc(size);
#endif
}

void qfree(void **root)
{
    LINK *this;

    while (*root) {
	this = (LINK *) * root;
	*root = this->next;
#ifndef __REACTOS__
	free(this->data);
	free(this);
#else
	vffree(this->data);
	vffree(this);
#endif
    }
}

#ifdef __REACTOS__
#ifdef min
#undef min
#endif
#endif
int min(int a, int b)
{
    return a < b ? a : b;
}

char get_key(const char *valid, const char *prompt)
{
#ifndef __REACTOS__
    int ch, okay;

    while (1) {
	if (prompt)
	    printf("%s ", prompt);
	fflush(stdout);
	while (ch = getchar(), ch == ' ' || ch == '\t') ;
	if (ch == EOF)
	    exit(1);
	if (!strchr(valid, okay = ch))
	    okay = 0;
	while (ch = getchar(), ch != '\n' && ch != EOF) ;
	if (ch == EOF)
	    exit(1);
	if (okay)
	    return okay;
	printf("Invalid input.\n");
    }
#else
    return 0;
#endif
}
