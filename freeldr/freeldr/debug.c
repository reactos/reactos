/*
 *  FreeLoader
 *  Copyright (C) 2001  Brian Palmer  <brianp@sginet.com>
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

#include "freeldr.h"
#include "debug.h"
#include "stdlib.h"

ULONG	DebugPrintMask = DPRINT_WARNING | DPRINT_MEMORY;

void DebugPrint(ULONG Mask, char *format, ...)
{
	int *dataptr = (int *) &format;
	char c, *ptr, str[16];
	char buffer[512];
	char *p = buffer;
	
	// Mask out unwanted debug messages
	if (!(Mask & DebugPrintMask))
	{
		return;
	}

	dataptr++;

	while ((c = *(format++)))
	{
	  if (c != '%')
	  {
	*p = c;
	p++;
	  }
	  else
	switch (c = *(format++))
	  {
	  case 'd': case 'u': case 'x':
		*convert_to_ascii(str, c, *((unsigned long *) dataptr++)) = 0;

		ptr = str;

		while (*ptr)
		{
		  *p = *(ptr++);
		  p++;
		}
		break;

	  case 'c':
		*p = (*(dataptr++))&0xff;
		p++;
		break;

	  case 's':
		ptr = (char *)(*(dataptr++));

		while ((c = *(ptr++)))
		{
		  *p = c;
		  p++;
		}
		break;
	  }
	}
	*p=0;


	print(buffer);
}
