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

static const USHORT HatchBrushes[NB_HATCH_STYLES][8] =
{
  {0x00, 0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00}, /* HS_HORIZONTAL */
  {0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08}, /* HS_VERTICAL   */
  {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80}, /* HS_FDIAGONAL  */
  {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01}, /* HS_BDIAGONAL  */
  {0x08, 0x08, 0x08, 0x08, 0xff, 0x08, 0x08, 0x08}, /* HS_CROSS      */
  {0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81}  /* HS_DIAGCROSS  */
};

BOOL INTERNAL_CALL
BRUSH_Cleanup(PVOID ObjectBody)
{
  PGDIBRUSHOBJ pBrush = (PGDIBRUSHOBJ)ObjectBody;
  if(pBrush->flAttrs & (GDIBRUSH_IS_HATCH | GDIBRUSH_IS_BITMAP))
  {
    ASSERT(pBrush->hbmPattern);
    GDIOBJ_SetOwnership(pBrush->hbmPattern, PsGetCurrentProcess());
    NtGdiDeleteObject(pBrush->hbmPattern);
  }

  /* Free the kmode styles array of EXTPENS */
  if (pBrush->pStyle)
  {
    ExFreePool(pBrush->pStyle);
  }

  return TRUE;
}

INT FASTCALL
BRUSH_GetObject (PGDIBRUSHOBJ BrushObject, INT Count, LPLOGBRUSH Buffer)
{
   if( Buffer == NULL ) return sizeof(LOGBRUSH);
   if (Count == 0) return 0;

   /* Set colour */
    Buffer->lbColor = BrushObject->BrushAttr.lbColor;

    /* set Hatch */
    if ((BrushObject->flAttrs & GDIBRUSH_IS_HATCH)!=0)
    {
        /* FIXME : this is not the right value */
        Buffer->lbHatch = (LONG)BrushObject->hbmPattern;
    }
    else
    {
        Buffer->lbHatch = 0;
    }

    Buffer->lbStyle = 0;

    /* Get the type of style */
    if ((BrushObject->flAttrs & GDIBRUSH_IS_SOLID)!=0)
    {
        Buffer->lbStyle = BS_SOLID;
    }
    else if ((BrushObject->flAttrs & GDIBRUSH_IS_NULL)!=0)
    {
        Buffer->lbStyle = BS_NULL; // BS_HOLLOW
    }
    else if ((BrushObject->flAttrs & GDIBRUSH_IS_HATCH)!=0)
    {
        Buffer->lbStyle = BS_HATCHED;
    }
    else if ((BrushObject->flAttrs & GDIBRUSH_IS_BITMAP)!=0)
    {
        Buffer->lbStyle = BS_PATTERN;
    }
    else if ((BrushObject->flAttrs & GDIBRUSH_IS_DIB)!=0)
    {
        Buffer->lbStyle = BS_DIBPATTERN;
    }

    /* FIXME
    else if ((BrushObject->flAttrs & )!=0)
    {
        Buffer->lbStyle = BS_INDEXED;
    }
    else if ((BrushObject->flAttrs & )!=0)
    {
        Buffer->lbStyle = BS_DIBPATTERNPT;
    }
    */

    /* FIXME */
    return sizeof(LOGBRUSH);
}


XLATEOBJ* FASTCALL
IntGdiCreateBrushXlate(PDC Dc, GDIBRUSHOBJ *BrushObj, BOOLEAN *Failed)
{
   XLATEOBJ *Result = NULL;
   SURFACE * psurf;
   HPALETTE hPalette = NULL;

   psurf = SURFACE_LockSurface(Dc->rosdc.hBitmap);
   if (psurf)
   {
      hPalette = psurf->hDIBPalette;
      SURFACE_UnlockSurface(psurf);
   }
   if (!hPalette) hPalette = pPrimarySurface->DevInfo.hpalDefault;

   if (BrushObj->flAttrs & GDIBRUSH_IS_NULL)
   {
      Result = NULL;
      *Failed = FALSE;
   }
   else if (BrushObj->flAttrs & GDIBRUSH_IS_SOLID)
   {
      Result = IntEngCreateXlate(0, PAL_RGB, hPalette, NULL);
      *Failed = FALSE;
   }
   else
   {
      SURFACE *Pattern = SURFACE_LockSurface(BrushObj->hbmPattern);
      if (Pattern == NULL)
         return NULL;

      /* Special case: 1bpp pattern */
      if (Pattern->SurfObj.iBitmapFormat == BMF_1BPP)
      {
         PDC_ATTR Dc_Attr = Dc->pDc_Attr;
         if (!Dc_Attr) Dc_Attr = &Dc->Dc_Attr;

         if (Dc->rosdc.bitsPerPixel != 1)
            Result = IntEngCreateSrcMonoXlate(hPalette, Dc_Attr->crBackgroundClr, BrushObj->BrushAttr.lbColor);
      }
      else if (BrushObj->flAttrs & GDIBRUSH_IS_DIB)
      {
         Result = IntEngCreateXlate(0, 0, hPalette, Pattern->hDIBPalette);
      }

      SURFACE_UnlockSurface(Pattern);
      *Failed = FALSE;
   }

   return Result;
}

VOID FASTCALL
IntGdiInitBrushInstance(GDIBRUSHINST *BrushInst, PGDIBRUSHOBJ BrushObj, XLATEOBJ *XlateObj)
{
   ASSERT(BrushInst);
   ASSERT(BrushObj);
   if (BrushObj->flAttrs & GDIBRUSH_IS_NULL)
   {
      BrushInst->BrushObject.iSolidColor = 0;
   }
   else if (BrushObj->flAttrs & GDIBRUSH_IS_SOLID)
   {
         BrushInst->BrushObject.iSolidColor = XLATEOBJ_iXlate(XlateObj, BrushObj->BrushAttr.lbColor);
   }
   else
   {
      BrushInst->BrushObject.iSolidColor = 0xFFFFFFFF;
   }

   BrushInst->BrushObject.pvRbrush = BrushObj->ulRealization;
   BrushInst->BrushObject.flColorType = 0;
   BrushInst->GdiBrushObject = BrushObj;
   BrushInst->XlateObject = XlateObj;
}

/**
 * @name CalculateColorTableSize
 *
 * Internal routine to calculate the number of color table entries.
 *
 * @param BitmapInfoHeader
 *        Input bitmap information header, can be any version of
 *        BITMAPINFOHEADER or BITMAPCOREHEADER.
 *
 * @param ColorSpec
 *        Pointer to variable which specifiing the color mode (DIB_RGB_COLORS
 *        or DIB_RGB_COLORS). On successful return this value is normalized
 *        according to the bitmap info.
 *
 * @param ColorTableSize
 *        On successful return this variable is filled with number of
 *        entries in color table for the image with specified parameters.
 *
 * @return
 *    TRUE if the input values together form a valid image, FALSE otherwise.
 */

BOOL APIENTRY
CalculateColorTableSize(
   CONST BITMAPINFOHEADER *BitmapInfoHeader,
   UINT *ColorSpec,
   UINT *ColorTableSize)
{
   WORD BitCount;
   DWORD ClrUsed;
   DWORD Compression;

   /*
    * At first get some basic parameters from the passed BitmapInfoHeader
    * structure. It can have one of the following formats:
    * - BITMAPCOREHEADER (the oldest one with totally different layout
    *                     from the others)
    * - BITMAPINFOHEADER (the standard and most common header)
    * - BITMAPV4HEADER (extension of BITMAPINFOHEADER)
    * - BITMAPV5HEADER (extension of BITMAPV4HEADER)
    */

   if (BitmapInfoHeader->biSize == sizeof(BITMAPCOREHEADER))
   {
      BitCount = ((LPBITMAPCOREHEADER)BitmapInfoHeader)->bcBitCount;
      ClrUsed = 0;
      Compression = BI_RGB;
   }
   else
   {
      BitCount = BitmapInfoHeader->biBitCount;
      ClrUsed = BitmapInfoHeader->biClrUsed;
      Compression = BitmapInfoHeader->biCompression;
   }

   switch (Compression)
   {
      case BI_BITFIELDS:
         if (*ColorSpec == DIB_PAL_COLORS)
            *ColorSpec = DIB_RGB_COLORS;

         if (BitCount != 16 && BitCount != 32)
            return FALSE;

         /*
          * For BITMAPV4HEADER/BITMAPV5HEADER the masks are included in
          * the structure itself (bV4RedMask, bV4GreenMask, and bV4BlueMask).
          * For BITMAPINFOHEADER the color masks are stored in the palette.
          */

         if (BitmapInfoHeader->biSize > sizeof(BITMAPINFOHEADER))
            *ColorTableSize = 0;
         else
            *ColorTableSize = 3;

         return TRUE;

      case BI_RGB:
         switch (BitCount)
         {
            case 1:
               *ColorTableSize = ClrUsed ? min(ClrUsed, 2) : 2;
               return TRUE;

            case 4:
               *ColorTableSize = ClrUsed ? min(ClrUsed, 16) : 16;
               return TRUE;

            case 8:
               *ColorTableSize = ClrUsed ? min(ClrUsed, 256) : 256;
               return TRUE;

            default:
               if (*ColorSpec == DIB_PAL_COLORS)
                  *ColorSpec = DIB_RGB_COLORS;
               if (BitCount != 16 && BitCount != 24 && BitCount != 32)
                  return FALSE;
               *ColorTableSize = ClrUsed;
               return TRUE;
         }

      case BI_RLE4:
         if (BitCount == 4)
         {
            *ColorTableSize = ClrUsed ? min(ClrUsed, 16) : 16;
            return TRUE;
         }
         return FALSE;

      case BI_RLE8:
         if (BitCount == 8)
         {
            *ColorTableSize = ClrUsed ? min(ClrUsed, 256) : 256;
            return TRUE;
         }
         return FALSE;

      case BI_JPEG:
      case BI_PNG:
         *ColorTableSize = ClrUsed;
         return TRUE;

      default:
         return FALSE;
   }
}

HBRUSH APIENTRY
IntGdiCreateDIBBrush(
   CONST BITMAPINFO *BitmapInfo,
   UINT ColorSpec,
   UINT BitmapInfoSize,
   CONST VOID *PackedDIB)
{
   HBRUSH hBrush;
   PGDIBRUSHOBJ BrushObject;
   HBITMAP hPattern;
   ULONG_PTR DataPtr;
   UINT PaletteEntryCount;
   PSURFACE psurfPattern;
   INT PaletteType;

   if (BitmapInfo->bmiHeader.biSize < sizeof(BITMAPINFOHEADER))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return NULL;
   }

   if (!CalculateColorTableSize(&BitmapInfo->bmiHeader, &ColorSpec,
                                &PaletteEntryCount))
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return NULL;
   }

   DataPtr = (ULONG_PTR)BitmapInfo + BitmapInfo->bmiHeader.biSize;
   if (ColorSpec == DIB_RGB_COLORS)
      DataPtr += PaletteEntryCount * sizeof(RGBQUAD);
   else
      DataPtr += PaletteEntryCount * sizeof(USHORT);

   hPattern = IntGdiCreateBitmap(BitmapInfo->bmiHeader.biWidth,
                                BitmapInfo->bmiHeader.biHeight,
                                BitmapInfo->bmiHeader.biPlanes,
                                BitmapInfo->bmiHeader.biBitCount,
                                (PVOID)DataPtr);
   if (hPattern == NULL)
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }

   psurfPattern = SURFACE_LockSurface(hPattern);
   ASSERT(psurfPattern != NULL);
   psurfPattern->hDIBPalette = BuildDIBPalette(BitmapInfo, &PaletteType);
   SURFACE_UnlockSurface(psurfPattern);

   BrushObject = BRUSHOBJ_AllocBrushWithHandle();
   if (BrushObject == NULL)
   {
      NtGdiDeleteObject(hPattern);
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }
   hBrush = BrushObject->BaseObject.hHmgr;

   BrushObject->flAttrs |= GDIBRUSH_IS_BITMAP | GDIBRUSH_IS_DIB;
   BrushObject->hbmPattern = hPattern;
   /* FIXME: Fill in the rest of fields!!! */

   GDIOBJ_SetOwnership(hPattern, NULL);

   BRUSHOBJ_UnlockBrush(BrushObject);

   return hBrush;
}

HBRUSH APIENTRY
IntGdiCreateHatchBrush(
   INT Style,
   COLORREF Color)
{
   HBRUSH hBrush;
   PGDIBRUSHOBJ BrushObject;
   HBITMAP hPattern;

   if (Style < 0 || Style >= NB_HATCH_STYLES)
   {
      return 0;
   }

   hPattern = IntGdiCreateBitmap(8, 8, 1, 1, (LPBYTE)HatchBrushes[Style]);
   if (hPattern == NULL)
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }

   BrushObject = BRUSHOBJ_AllocBrushWithHandle();
   if (BrushObject == NULL)
   {
      NtGdiDeleteObject(hPattern);
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }
   hBrush = BrushObject->BaseObject.hHmgr;

   BrushObject->flAttrs |= GDIBRUSH_IS_HATCH;
   BrushObject->hbmPattern = hPattern;
   BrushObject->BrushAttr.lbColor = Color & 0xFFFFFF;

   GDIOBJ_SetOwnership(hPattern, NULL);

   BRUSHOBJ_UnlockBrush(BrushObject);

   return hBrush;
}

HBRUSH APIENTRY
IntGdiCreatePatternBrush(
   HBITMAP hBitmap)
{
   HBRUSH hBrush;
   PGDIBRUSHOBJ BrushObject;
   HBITMAP hPattern;

   hPattern = BITMAP_CopyBitmap(hBitmap);
   if (hPattern == NULL)
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }

   BrushObject = BRUSHOBJ_AllocBrushWithHandle();
   if (BrushObject == NULL)
   {
      NtGdiDeleteObject(hPattern);
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }
   hBrush = BrushObject->BaseObject.hHmgr;

   BrushObject->flAttrs |= GDIBRUSH_IS_BITMAP;
   BrushObject->hbmPattern = hPattern;
   /* FIXME: Fill in the rest of fields!!! */

   GDIOBJ_SetOwnership(hPattern, NULL);

   BRUSHOBJ_UnlockBrush(BrushObject);

   return hBrush;
}

HBRUSH APIENTRY
IntGdiCreateSolidBrush(
   COLORREF Color)
{
   HBRUSH hBrush;
   PGDIBRUSHOBJ BrushObject;

   BrushObject = BRUSHOBJ_AllocBrushWithHandle();
   if (BrushObject == NULL)
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }
   hBrush = BrushObject->BaseObject.hHmgr;

   BrushObject->flAttrs |= GDIBRUSH_IS_SOLID;

   BrushObject->BrushAttr.lbColor = Color;
   /* FIXME: Fill in the rest of fields!!! */

   BRUSHOBJ_UnlockBrush(BrushObject);

   return hBrush;
}

HBRUSH APIENTRY
IntGdiCreateNullBrush(VOID)
{
   HBRUSH hBrush;
   PGDIBRUSHOBJ BrushObject;

   BrushObject = BRUSHOBJ_AllocBrushWithHandle();
   if (BrushObject == NULL)
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }
   hBrush = BrushObject->BaseObject.hHmgr;

   BrushObject->flAttrs |= GDIBRUSH_IS_NULL;
   BRUSHOBJ_UnlockBrush(BrushObject);

   return hBrush;
}

HBRUSH
FASTCALL
IntGdiSelectBrush(
    PDC pDC,
    HBRUSH hBrush)
{
    PDC_ATTR pDc_Attr;
    HBRUSH hOrgBrush;
    PGDIBRUSHOBJ pBrush;
    XLATEOBJ *XlateObj;
    BOOLEAN bFailed;

    if (pDC == NULL || hBrush == NULL) return NULL;

    pDc_Attr = pDC->pDc_Attr;
    if(!pDc_Attr) pDc_Attr = &pDC->Dc_Attr;

    pBrush = BRUSHOBJ_LockBrush(hBrush);
    if (pBrush == NULL)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return NULL;
    }

    XlateObj = IntGdiCreateBrushXlate(pDC, pBrush, &bFailed);
    BRUSHOBJ_UnlockBrush(pBrush);
    if(bFailed)
    {
        return NULL;
    }

    hOrgBrush = pDc_Attr->hbrush;
    pDc_Attr->hbrush = hBrush;

    if (pDC->rosdc.XlateBrush != NULL)
    {
        EngDeleteXlate(pDC->rosdc.XlateBrush);
    }
    pDC->rosdc.XlateBrush = XlateObj;

    pDc_Attr->ulDirty_ &= ~DC_BRUSH_DIRTY;

    return hOrgBrush;
}


/* PUBLIC FUNCTIONS ***********************************************************/

HBRUSH APIENTRY
NtGdiCreateDIBBrush(
   IN PVOID BitmapInfoAndData,
   IN FLONG ColorSpec,
   IN UINT BitmapInfoSize,
   IN BOOL  b8X8,
   IN BOOL bPen,
   IN PVOID PackedDIB)
{
   BITMAPINFO *SafeBitmapInfoAndData;
   NTSTATUS Status = STATUS_SUCCESS;
   HBRUSH hBrush;

   SafeBitmapInfoAndData = EngAllocMem(FL_ZERO_MEMORY, BitmapInfoSize, TAG_DIB);
   if (SafeBitmapInfoAndData == NULL)
   {
      SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
      return NULL;
   }

   _SEH2_TRY
   {
      ProbeForRead(BitmapInfoAndData,
                   BitmapInfoSize,
                   1);
      RtlCopyMemory(SafeBitmapInfoAndData,
                    BitmapInfoAndData,
                    BitmapInfoSize);
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Status = _SEH2_GetExceptionCode();
   }
   _SEH2_END;

   if (!NT_SUCCESS(Status))
   {
      EngFreeMem(SafeBitmapInfoAndData);
      SetLastNtError(Status);
      return 0;
   }

   hBrush = IntGdiCreateDIBBrush(SafeBitmapInfoAndData, ColorSpec,
                                 BitmapInfoSize, PackedDIB);

   EngFreeMem(SafeBitmapInfoAndData);

   return hBrush;
}

HBRUSH APIENTRY
NtGdiCreateHatchBrushInternal(
   ULONG Style,
   COLORREF Color,
   BOOL bPen)
{
   return IntGdiCreateHatchBrush(Style, Color);
}

HBRUSH APIENTRY
NtGdiCreatePatternBrushInternal(
   HBITMAP hBitmap,
   BOOL bPen,
   BOOL b8x8)
{
   return IntGdiCreatePatternBrush(hBitmap);
}

HBRUSH APIENTRY
NtGdiCreateSolidBrush(COLORREF Color,
                      IN OPTIONAL HBRUSH hbr)
{
   return IntGdiCreateSolidBrush(Color);
}

/*
 * NtGdiSetBrushOrg
 *
 * The NtGdiSetBrushOrg function sets the brush origin that GDI assigns to
 * the next brush an application selects into the specified device context.
 *
 * Status
 *    @implemented
 */

BOOL APIENTRY
NtGdiSetBrushOrg(HDC hDC, INT XOrg, INT YOrg, LPPOINT Point)
{
   PDC dc;
   PDC_ATTR Dc_Attr;

   dc = DC_LockDc(hDC);
   if (dc == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
   }
   Dc_Attr = dc->pDc_Attr;
   if (!Dc_Attr) Dc_Attr = &dc->Dc_Attr;

   if (Point != NULL)
   {
      NTSTATUS Status = STATUS_SUCCESS;
      POINT SafePoint;
      SafePoint.x = Dc_Attr->ptlBrushOrigin.x;
      SafePoint.y = Dc_Attr->ptlBrushOrigin.y;
      _SEH2_TRY
      {
         ProbeForWrite(Point,
                       sizeof(POINT),
                       1);
         *Point = SafePoint;
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         Status = _SEH2_GetExceptionCode();
      }
      _SEH2_END;

      if(!NT_SUCCESS(Status))
      {
        DC_UnlockDc(dc);
        SetLastNtError(Status);
        return FALSE;
      }
   }
   Dc_Attr->ptlBrushOrigin.x = XOrg;
   Dc_Attr->ptlBrushOrigin.y = YOrg;
   DC_UnlockDc(dc);
   return TRUE;
}

VOID FASTCALL
IntGdiSetSolidBrushColor(HBRUSH hBrush, COLORREF Color)
{
  PGDIBRUSHOBJ BrushObject;

  BrushObject = BRUSHOBJ_LockBrush(hBrush);
  if (BrushObject->flAttrs & GDIBRUSH_IS_SOLID)
  {
      BrushObject->BrushAttr.lbColor = Color & 0xFFFFFF;
  }
  BRUSHOBJ_UnlockBrush(BrushObject);
}

 /*
 * @implemented
 */
HBRUSH
APIENTRY
NtGdiSelectBrush(
    IN HDC hDC,
    IN HBRUSH hBrush)
{
    PDC pDC;
    HBRUSH hOrgBrush;

    if (hDC == NULL || hBrush == NULL) return NULL;

    pDC = DC_LockDc(hDC);
    if (!pDC)
    {
        return NULL;
    }

    hOrgBrush = IntGdiSelectBrush(pDC,hBrush);

    DC_UnlockDc(pDC);

    return hOrgBrush;
}

/* EOF */
