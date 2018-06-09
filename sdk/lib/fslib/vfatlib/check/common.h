/* common.h - Common functions

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

#ifndef _COMMON_H
#define _COMMON_H

#ifndef __REACTOS__
void die(const char *msg, ...)
    __attribute((noreturn, format(printf, 1, 2)));
#else
DECLSPEC_NORETURN // __attribute((noreturn))
void die_func(const char *msg, ...);
#define die(msg, ...)   \
do {                    \
    die_func("DIE! (%s:%d) " msg "\n", __RELFILE__, __LINE__, ##__VA_ARGS__);  \
} while (0)
#endif

/* Displays a prinf-style message and terminates the program. */

#ifndef __REACTOS__
void pdie(const char *msg, ...)
    __attribute((noreturn, format(printf, 1, 2)));
#else
DECLSPEC_NORETURN // __attribute((noreturn))
void pdie_func(const char *msg, ...);
#define pdie(msg, ...)   \
do {                    \
    pdie_func("P-DIE! (%s:%d) " msg "\n", __RELFILE__, __LINE__, ##__VA_ARGS__);  \
} while (0)
#endif

/* Like die, but appends an error message according to the state of errno. */

#ifndef __REACTOS__
void *alloc(int size);
#else
void *vfalloc(int size);
void *vfcalloc(int size, int count);
void vffree(void *ptr);
#endif

/* mallocs SIZE bytes and returns a pointer to the data. Terminates the program
   if malloc fails. */

void *qalloc(void **root, int size);

/* Like alloc, but registers the data area in a list described by ROOT. */

void qfree(void **root);

/* Deallocates all qalloc'ed data areas described by ROOT. */

#ifndef __REACTOS__
int min(int a, int b);
#endif

/* Returns the smaller integer value of a and b. */

char get_key(const char *valid, const char *prompt);

/* Displays PROMPT and waits for user input. Only characters in VALID are
   accepted. Terminates the program on EOF. Returns the character. */

#endif
