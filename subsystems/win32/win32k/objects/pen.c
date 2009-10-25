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

PBRUSH
FASTCALL
PEN_LockPen(HGDIOBJ hBMObj)
{
   if (GDI_HANDLE_GET_TYPE(hBMObj) == GDI_OBJECT_TYPE_EXTPEN)
      return GDIOBJ_LockObj( hBMObj, GDI_OBJECT_TYPE_EXTPEN);
   else
      return GDIOBJ_LockObj( hBMObj, GDI_OBJECT_TYPE_PEN);
}

PBRUSH
FASTCALL
PEN_ShareLockPen(HGDIOBJ hBMObj)
{
   if (GDI_HANDLE_GET_TYPE(hBMObj) == GDI_OBJECT_TYPE_EXTPEN)
      return GDIOBJ_ShareLockObj( hBMObj, GDI_OBJECT_TYPE_EXTPEN);
   else
      return GDIOBJ_ShareLockObj( hBMObj, GDI_OBJECT_TYPE_PEN);
}

HPEN APIENTRY
IntGdiExtCreatePen(
   DWORD dwPenStyle,
   DWORD dwWidth,
   IN ULONG ulBrushStyle,
   IN ULONG ulColor,
   IN ULONG_PTR ulClientHatch,
   IN ULONG_PTR ulHatch,
   DWORD dwStyleCount,
   PULONG pStyle,
   IN ULONG cjDIB,
   IN BOOL bOldStylePen,
   IN OPTIONAL HBRUSH hbrush)
{
   HPEN hPen;
   PBRUSH pbrushPen;
   static const BYTE PatternAlternate[] = {0x55, 0x55, 0x55};
   static const BYTE PatternDash[] = {0xFF, 0xFF, 0xC0};
   static const BYTE PatternDot[] = {0xE3, 0x8E, 0x38};
   static const BYTE PatternDashDot[] = {0xFF, 0x81, 0xC0};
   static const BYTE PatternDashDotDot[] = {0xFF, 0x8E, 0x38};

   dwWidth = abs(dwWidth);

   if ( (dwPenStyle & PS_STYLE_MASK) == PS_NULL)
   {
      return StockObjects[NULL_PEN];
   }

   if (bOldStylePen)
   {
      pbrushPen = PEN_AllocPenWithHandle();
   }
   else
   {
      pbrushPen = PEN_AllocExtPenWithHandle();
   }

   if (!pbrushPen)
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      DPRINT("Can't allocate pen\n");
      return 0;
   }
   hPen = pbrushPen->BaseObject.hHmgr;

   // If nWidth is zero, the pen is a single pixel wide, regardless of the current transformation.
   if ((bOldStylePen) && (!dwWidth) && (dwPenStyle & PS_STYLE_MASK) != PS_SOLID)
      dwWidth = 1;

   pbrushPen->ptPenWidth.x = dwWidth;
   pbrushPen->ptPenWidth.y = 0;
   pbrushPen->ulPenStyle = dwPenStyle;
   pbrushPen->BrushAttr.lbColor = ulColor;
   pbrushPen->ulStyle = ulBrushStyle;
   // FIXME: copy the bitmap first ?
   pbrushPen->hbmClient = (HANDLE)ulClientHatch;
   pbrushPen->dwStyleCount = dwStyleCount;
   pbrushPen->pStyle = pStyle;

   pbrushPen->flAttrs = bOldStylePen? GDIBRUSH_IS_OLDSTYLEPEN : GDIBRUSH_IS_PEN;

   // If dwPenStyle is PS_COSMETIC, the width must be set to 1.
   if ( !(bOldStylePen) && ((dwPenStyle & PS_TYPE_MASK) == PS_COSMETIC) && ( dwWidth != 1) )
      goto ExitCleanup;

   switch (dwPenStyle & PS_STYLE_MASK)
   {
      case PS_NULL:
         pbrushPen->flAttrs |= GDIBRUSH_IS_NULL;
         break;

      case PS_SOLID:
         pbrushPen->flAttrs |= GDIBRUSH_IS_SOLID;
         break;

      case PS_ALTERNATE:
         pbrushPen->flAttrs |= GDIBRUSH_IS_BITMAP;
         pbrushPen->hbmPattern = IntGdiCreateBitmap(24, 1, 1, 1, (LPBYTE)PatternAlternate);
         break;

      case PS_DOT:
         pbrushPen->flAttrs |= GDIBRUSH_IS_BITMAP;
         pbrushPen->hbmPattern = IntGdiCreateBitmap(24, 1, 1, 1, (LPBYTE)PatternDot);
         break;

      case PS_DASH:
         pbrushPen->flAttrs |= GDIBRUSH_IS_BITMAP;
         pbrushPen->hbmPattern = IntGdiCreateBitmap(24, 1, 1, 1, (LPBYTE)PatternDash);
         break;

      case PS_DASHDOT:
         pbrushPen->flAttrs |= GDIBRUSH_IS_BITMAP;
         pbrushPen->hbmPattern = IntGdiCreateBitmap(24, 1, 1, 1, (LPBYTE)PatternDashDot);
         break;

      case PS_DASHDOTDOT:
         pbrushPen->flAttrs |= GDIBRUSH_IS_BITMAP;
         pbrushPen->hbmPattern = IntGdiCreateBitmap(24, 1, 1, 1, (LPBYTE)PatternDashDotDot);
         break;

      case PS_INSIDEFRAME:
         pbrushPen->flAttrs |= (GDIBRUSH_IS_SOLID|GDIBRUSH_IS_INSIDEFRAME);
         break;

      case PS_USERSTYLE:
         if ((dwPenStyle & PS_TYPE_MASK) == PS_COSMETIC)
         {
            /* FIXME: PS_USERSTYLE workaround */
            DPRINT1("PS_COSMETIC | PS_USERSTYLE not handled\n");
            pbrushPen->flAttrs |= GDIBRUSH_IS_SOLID;
            break;
         }
         else
         {
            UINT i;
            BOOL has_neg = FALSE, all_zero = TRUE;

            for(i = 0; (i < dwStyleCount) && !has_neg; i++)
            {
                has_neg = has_neg || (((INT)(pStyle[i])) < 0);
                all_zero = all_zero && (pStyle[i] == 0);
            }

            if(all_zero || has_neg)
            {
                goto ExitCleanup;
            }
         }
         /* FIXME: what style here? */
         pbrushPen->flAttrs |= 0;
         break;

      default:
         DPRINT1("IntGdiExtCreatePen unknown penstyle %x\n", dwPenStyle);
   }
   PEN_UnlockPen(pbrushPen);
   return hPen;

ExitCleanup:
   SetLastWin32Error(ERROR_INVALID_PARAMETER);
   pbrushPen->pStyle = NULL;
   PEN_UnlockPen(pbrushPen);
   if (bOldStylePen)
      PEN_FreePenByHandle(hPen);
   else
      PEN_FreeExtPenByHandle(hPen);
   return NULL;
}

VOID FASTCALL
IntGdiSetSolidPenColor(HPEN hPen, COLORREF Color)
{
  PBRUSH pbrushPen;

  pbrushPen = PEN_LockPen(hPen);
  if (pbrushPen)
  {
    if (pbrushPen->flAttrs & GDIBRUSH_IS_SOLID)
    {
      pbrushPen->BrushAttr.lbColor = Color & 0xFFFFFF;
    }
    PEN_UnlockPen(pbrushPen);
  }
}

INT APIENTRY
PEN_GetObject(PBRUSH pbrushPen, INT cbCount, PLOGPEN pBuffer)
{
   PLOGPEN pLogPen;
   PEXTLOGPEN pExtLogPen;
   INT cbRetCount;

   if (pbrushPen->flAttrs & GDIBRUSH_IS_OLDSTYLEPEN)
   {
      cbRetCount = sizeof(LOGPEN);
      if (pBuffer)
      {

         if (cbCount < cbRetCount) return 0;

         if ( (pbrushPen->ulPenStyle & PS_STYLE_MASK) == PS_NULL && 
               cbCount == sizeof(EXTLOGPEN))
         {
            pExtLogPen = (PEXTLOGPEN)pBuffer; 
            pExtLogPen->elpPenStyle = pbrushPen->ulPenStyle;
            pExtLogPen->elpWidth = 0;
            pExtLogPen->elpBrushStyle = pbrushPen->ulStyle;
            pExtLogPen->elpColor = pbrushPen->BrushAttr.lbColor;
            pExtLogPen->elpHatch = 0;
            pExtLogPen->elpNumEntries = 0;
            cbRetCount = sizeof(EXTLOGPEN);
         }
         else
         {
            pLogPen = (PLOGPEN)pBuffer;
            pLogPen->lopnWidth = pbrushPen->ptPenWidth;
            pLogPen->lopnStyle = pbrushPen->ulPenStyle;
            pLogPen->lopnColor = pbrushPen->BrushAttr.lbColor;
         }
      }
   }
   else
   {
      // FIXME: Can we trust in dwStyleCount being <= 16?
      cbRetCount = sizeof(EXTLOGPEN) - sizeof(DWORD) + pbrushPen->dwStyleCount * sizeof(DWORD);
      if (pBuffer)
      {
         INT i;

         if (cbCount < cbRetCount) return 0;
         pExtLogPen = (PEXTLOGPEN)pBuffer;
         pExtLogPen->elpPenStyle = pbrushPen->ulPenStyle;
         pExtLogPen->elpWidth = pbrushPen->ptPenWidth.x;
         pExtLogPen->elpBrushStyle = pbrushPen->ulStyle;
         pExtLogPen->elpColor = pbrushPen->BrushAttr.lbColor;
         pExtLogPen->elpHatch = (ULONG_PTR)pbrushPen->hbmClient;
         pExtLogPen->elpNumEntries = pbrushPen->dwStyleCount;
         for (i = 0; i < pExtLogPen->elpNumEntries; i++)
         {
            pExtLogPen->elpStyleEntry[i] = pbrushPen->pStyle[i];
         }
      }
   }

   return cbRetCount;
}


/* PUBLIC FUNCTIONS ***********************************************************/

HPEN APIENTRY
NtGdiCreatePen(
   INT PenStyle,
   INT Width,
   COLORREF Color,
   IN HBRUSH hbr)
{
   if ( PenStyle < PS_SOLID || PenStyle > PS_INSIDEFRAME )
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return NULL;
   }

   return IntGdiExtCreatePen(PenStyle,
                             Width,
                             BS_SOLID,
                             Color,
                             0,
                             0,
                             0,
                             NULL,
                             0,
                             TRUE,
                             hbr);
}

HPEN APIENTRY
NtGdiExtCreatePen(
   DWORD dwPenStyle,
   DWORD ulWidth,
   IN ULONG ulBrushStyle,
   IN ULONG ulColor,
   IN ULONG_PTR ulClientHatch,
   IN ULONG_PTR ulHatch,
   DWORD dwStyleCount,
   PULONG pUnsafeStyle,
   IN ULONG cjDIB,
   IN BOOL bOldStylePen,
   IN OPTIONAL HBRUSH hBrush)
{
   NTSTATUS Status = STATUS_SUCCESS;
   DWORD* pSafeStyle = NULL;
   HPEN hPen;

   if ((int)dwStyleCount < 0) return 0;
   if (dwStyleCount > 16)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
   }

   if (dwStyleCount > 0)
   {
      pSafeStyle = ExAllocatePoolWithTag(NonPagedPool, dwStyleCount * sizeof(DWORD), TAG_PENSTYLES);
      if (!pSafeStyle)
      {
         SetLastNtError(ERROR_NOT_ENOUGH_MEMORY);
         return 0;
      }
      _SEH2_TRY
      {
         ProbeForRead(pUnsafeStyle, dwStyleCount * sizeof(DWORD), 1);
         RtlCopyMemory(pSafeStyle,
                       pUnsafeStyle,
                       dwStyleCount * sizeof(DWORD));
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         Status = _SEH2_GetExceptionCode();
      }
      _SEH2_END
      if(!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         ExFreePoolWithTag(pSafeStyle, TAG_PENSTYLES);
         return 0;
      }
   }

   if (ulBrushStyle == BS_PATTERN)
   {
      _SEH2_TRY
      {
         ProbeForRead((PVOID)ulHatch, cjDIB, 1);
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         Status = _SEH2_GetExceptionCode();
      }
      _SEH2_END
      if(!NT_SUCCESS(Status))
      {
         SetLastNtError(Status);
         if (pSafeStyle) ExFreePoolWithTag(pSafeStyle, TAG_PENSTYLES);
         return 0;
      }
   }

   hPen = IntGdiExtCreatePen(dwPenStyle,
                                ulWidth,
                           ulBrushStyle,
                                ulColor,
                          ulClientHatch,
                                ulHatch,
                           dwStyleCount,
                             pSafeStyle,
                                  cjDIB,
                           bOldStylePen,
                                 hBrush);

   if (!hPen && pSafeStyle)
   {
      ExFreePoolWithTag(pSafeStyle, TAG_PENSTYLES);
   }
   return hPen;
}



/* EOF */
