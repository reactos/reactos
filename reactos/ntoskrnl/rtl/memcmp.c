/*
 *  ReactOS kernel
 *  Copyright (C) 2000 David Welch <welch@cwcom.net>
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
/*
 * FILE:            ntoskrnl/rtl/memcmp.c
 * PURPOSE:         Implements memcmp function
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 01/10/00
 */

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


int memcmp(const void* _s1, const void* _s2, size_t n)
{
   unsigned int i;
   const char* s1;
   const char* s2;
   
   s1 = (const char *)_s1;
   s2 = (const char *)_s2;
   for (i = 0; i < n; i++)
     {
	if ((*s1) != (*s2))
	  {
	     return((*s1) - (*s2));
	  }
	s1++;
	s2++;
     }
   return(0);
}
