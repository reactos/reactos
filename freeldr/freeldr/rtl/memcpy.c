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
void *memcpy(void *to, const void *from, size_t count)
{
  __asm__( \
	"or	%%ecx,%%ecx\n\t"\
	"jz	.L1\n\t"	\
	"cld\n\t"		\
	"rep\n\t"		\
	"movsb\n\t"		\
	".L1:\n\t"
	  :
	  : "D" (to), "S" (from), "c" (count));
  return to;
}
#else
void *memcpy(void *to, const void *from, size_t count)
{
	unsigned int	i;
	char*			buf1 = to;
	const char*		buf2 = from;

	for (i=0; i<count; i++)
	{
		buf1[i] = buf2[i];
	}

	return to;
}
#endif
