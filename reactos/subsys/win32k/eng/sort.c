/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
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
/* $Id: sort.c,v 1.2 2003/07/12 14:00:05 gvg Exp $ */

#include <stdlib.h>
#include <ddk/winddi.h>

/*
 * @implemented
 */
void STDCALL
EngSort(IN OUT PBYTE Buf, IN ULONG ElemSize, IN ULONG ElemCount, IN SORTCOMP CompFunc)
{
  qsort(Buf, ElemCount, ElemSize, CompFunc);
}

/* EOF */
