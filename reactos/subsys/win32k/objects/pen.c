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
/* $Id: pen.c,v 1.13 2003/12/12 15:47:37 weiden Exp $ */
#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/pen.h>
#include <include/error.h>
#include <internal/safe.h>

#define NDEBUG
#include <win32k/debug1.h>

HPEN FASTCALL
IntGdiCreatePenIndirect(PLOGPEN lgpn)
{
  HPEN hpen;
  PPENOBJ penPtr;
  
  if (lgpn->lopnStyle > PS_INSIDEFRAME) return 0;

  hpen = PENOBJ_AllocPen();
  if (!hpen) return 0;

  penPtr   = PENOBJ_LockPen( hpen );
  ASSERT( penPtr );

  penPtr->logpen.lopnStyle = lgpn->lopnStyle;
  penPtr->logpen.lopnWidth = lgpn->lopnWidth;
  penPtr->logpen.lopnColor = lgpn->lopnColor;
  PENOBJ_UnlockPen( hpen );
  
  return hpen;
}

/******************************************************************************/

HPEN
STDCALL
NtGdiCreatePen(INT PenStyle, INT Width, COLORREF Color)
{
  LOGPEN logpen;

  logpen.lopnStyle = PenStyle;
  logpen.lopnWidth.x = Width;
  logpen.lopnWidth.y = 0;
  logpen.lopnColor = Color;

  return IntGdiCreatePenIndirect(&logpen);
}

HPEN
STDCALL
NtGdiCreatePenIndirect(CONST PLOGPEN lgpn)
{
  LOGPEN   Safelgpn;
  NTSTATUS Status;
  
  Status = MmCopyFromCaller(&Safelgpn, lgpn, sizeof(LOGPEN));
  if(!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
    return 0;
  }
  
  return IntGdiCreatePenIndirect(&Safelgpn);
}

HPEN
STDCALL
NtGdiExtCreatePen(DWORD  PenStyle,
                       DWORD  Width,
                       CONST PLOGBRUSH  lb,
                       DWORD  StyleCount,
                       CONST PDWORD  Style)
{
  UNIMPLEMENTED;
}
/* EOF */
