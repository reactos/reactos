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
/* $Id: paint.c,v 1.11 2004/05/10 17:07:20 weiden Exp $ */
#include <w32k.h>

BOOL
STDCALL
NtGdiGdiFlush(VOID)
{
  UNIMPLEMENTED;
}

DWORD
STDCALL
NtGdiGdiGetBatchLimit(VOID)
{
  UNIMPLEMENTED;
}

DWORD
STDCALL
NtGdiGdiSetBatchLimit(DWORD  Limit)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
NtGdiGetBoundsRect(HDC  hDC,
                        LPRECT  Bounds,
                        UINT  Flags)
{
  DPRINT("stub");
  return  DCB_RESET;   /* bounding rectangle always empty */
}

UINT
STDCALL
NtGdiSetBoundsRect(HDC  hDC,
                        CONST PRECT  Bounds,
                        UINT  Flags)
{
  DPRINT("stub");
  return  DCB_DISABLE;   /* bounding rectangle always empty */
}
/* EOF */
