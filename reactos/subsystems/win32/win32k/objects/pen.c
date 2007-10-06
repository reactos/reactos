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
 * $Id$
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

HPEN FASTCALL
IntGdiCreatePenIndirect(PLOGPEN LogPen)
{
   HPEN hPen;
   PGDIBRUSHOBJ PenObject;
   static const WORD wPatternAlternate[] = {0x5555};
   static const WORD wPatternDash[] = {0x0F0F};
   static const WORD wPatternDot[] = {0x3333};

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
   /* FIXME - Handle PenObject == NULL!!! */
   PenObject->ptPenWidth = LogPen->lopnWidth;
   PenObject->ulPenStyle = LogPen->lopnStyle;
   PenObject->BrushAttr.lbColor = LogPen->lopnColor;
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
         PenObject->hbmPattern = NtGdiCreateBitmap(8, 1, 1, 1, (LPBYTE)wPatternAlternate);
         break;

      case PS_DOT:
         PenObject->flAttrs |= GDIBRUSH_IS_BITMAP;
         PenObject->hbmPattern = NtGdiCreateBitmap(8, 1, 1, 1, (LPBYTE)wPatternDot);
         break;

      case PS_DASH:
         PenObject->flAttrs |= GDIBRUSH_IS_BITMAP;
         PenObject->hbmPattern = NtGdiCreateBitmap(8, 1, 1, 1, (LPBYTE)wPatternDash);
         break;

      case PS_INSIDEFRAME:
         /* FIXME: does it need some additional work? */
         PenObject->flAttrs |= GDIBRUSH_IS_SOLID;
         break;

      default:
         DPRINT1("FIXME: IntGdiCreatePenIndirect is UNIMPLEMENTED\n");
   }

   PENOBJ_UnlockPen(PenObject);

   return hPen;
}

/* PUBLIC FUNCTIONS ***********************************************************/

HPEN STDCALL
NtGdiCreatePen(
   INT PenStyle,
   INT Width,
   COLORREF Color,
   IN HBRUSH hbr)
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
   NTSTATUS Status = STATUS_SUCCESS;

   _SEH_TRY
   {
     ProbeForRead(LogPen,
                  sizeof(LOGPEN),
                  1);
     SafeLogPen = *LogPen;
   }
   _SEH_HANDLE
   {
     Status = _SEH_GetExceptionCode();
   }
   _SEH_END;

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
   IN ULONG iBrushStyle,
   IN ULONG ulColor,
   IN ULONG_PTR lClientHatch,
   IN ULONG_PTR lHatch,
   DWORD StyleCount,
   PULONG Style,
   IN ULONG cjDIB,
   IN BOOL bOldStylePen,
   IN OPTIONAL HBRUSH hbrush)
{
    LOGPEN LogPen;

   if (PenStyle & PS_USERSTYLE)
      PenStyle = (PenStyle & ~PS_STYLE_MASK) | PS_SOLID;

   LogPen.lopnStyle = PenStyle & PS_STYLE_MASK;
   LogPen.lopnWidth.x = Width;
   LogPen.lopnColor = ulColor;

   return IntGdiCreatePenIndirect(&LogPen);
}

/* EOF */
