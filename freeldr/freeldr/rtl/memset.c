/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <freeldr.h>

#ifdef __i386__
void *memset(void *src, int val, size_t count)
{
	__asm__( \
		"or	%%ecx,%%ecx\n\t"\
		"jz	.L1\n\t"	\
		"cld\t\n"		\
		"rep\t\n"		\
		"stosb\t\n"		\
		".L1:\n\t"
		: 
		: "D" (src), "c" (count), "a" (val));
	return src;
}
#else
void *memset(void *src, int val, size_t count)
{
	unsigned int	i;
	unsigned char*	buf1 = src;

	for (i=0; i<count; i++)
	{
		buf1[i] = val;
	}

	return src;
}
#endif
