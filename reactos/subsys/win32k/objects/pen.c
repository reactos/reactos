/*
 * ReactOS Win32 Subsystem
 *
 * Copyright (C) 1998 - 2004 ReactOS Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: pen.c,v 1.14 2004/04/05 21:26:25 navaraf Exp $
 */

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <ddk/winddi.h>
#include <win32k/pen.h>
#include <win32k/bitmaps.h>
#include <include/error.h>
#include <internal/safe.h>
#define NDEBUG
#include <win32k/debug1.h>

/* PRIVATE FUNCTIONS **********************************************************/

HPEN FASTCALL
IntGdiCreatePenIndirect(PLOGPEN LogPen)
{
   HPEN hPen;
   PGDIBRUSHOBJ PenObject;
   static const WORD wPatternAlternate[] = {0x5555};
  
   if (LogPen->lopnStyle > PS_INSIDEFRAME)
      return 0;

   hPen = PENOBJ_AllocPen();
   if (!hPen)
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      DPRINT("Can't allocate pen\n");
      return 0;
   }

   PenObject = PENOBJ_LockPen(hPen);  
   PenObject->ptPenWidth = LogPen->lopnWidth;
   PenObject->ulPenStyle = LogPen->lopnStyle;
   PenObject->BrushAttr.lbColor = LogPen->lopnColor;
   PenObject->BrushObject.iSolidColor = LogPen->lopnColor;
   PenObject->flAttrs = GDIBRUSH_IS_OLDSTYLEPEN;
   switch (LogPen->lopnStyle)
   {
      case PS_NULL:
         PenObject->flAttrs |= GDIBRUSH_IS_NULL;
         break;

      case PS_SOLID:
         PenObject->flAttrs |= GDIBRUSH_IS_SOLID;
         break;

      case PS_ALTERNATE:
         PenObject->flAttrs |= GDIBRUSH_IS_BITMAP;
         PenObject->hbmPattern = NtGdiCreateBitmap(8, 1, 1, 1, wPatternAlternate);
         break;

      default:
         UNIMPLEMENTED;
   }

   PENOBJ_UnlockPen(hPen);
  
   return hPen;
}

/* PUBLIC FUNCTIONS ***********************************************************/

HPEN STDCALL
NtGdiCreatePen(
   INT PenStyle,
   INT Width,
   COLORREF Color)
{
  LOGPEN LogPen;

  LogPen.lopnStyle = PenStyle;
  LogPen.lopnWidth.x = Width;
  LogPen.lopnWidth.y = 0;
  LogPen.lopnColor = Color;

  return IntGdiCreatePenIndirect(&LogPen);
}

HPEN STDCALL
NtGdiCreatePenIndirect(CONST PLOGPEN LogPen)
{
   LOGPEN SafeLogPen;
   NTSTATUS Status;
  
   Status = MmCopyFromCaller(&SafeLogPen, LogPen, sizeof(LOGPEN));
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return 0;
   }
  
   return IntGdiCreatePenIndirect(&SafeLogPen);
}

HPEN STDCALL
NtGdiExtCreatePen(
   DWORD PenStyle,
   DWORD Width,
   CONST PLOGBRUSH LogBrush,
   DWORD StyleCount,
   CONST PDWORD Style)
{
   UNIMPLEMENTED;
}

/* EOF */
