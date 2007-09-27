/*
 * $Id$
 *
 * ReactOS W32 Subsystem
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
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
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

UINT STDCALL
IntSetDIBColorTable(HDC hDC, UINT StartIndex, UINT Entries, CONST RGBQUAD *Colors)
{
   PDC dc;
   PBITMAPOBJ BitmapObj;
   PPALGDI PalGDI;
   UINT Index;

   if (!(dc = DC_LockDc(hDC))) return 0;
   if (dc->IsIC)
   {
      DC_UnlockDc(dc);
      return 0;
   }

   BitmapObj = BITMAPOBJ_LockBitmap(dc->w.hBitmap);
   if (BitmapObj == NULL)
   {
      DC_UnlockDc(dc);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
   }

   if (BitmapObj->dib == NULL)
   {
      BITMAPOBJ_UnlockBitmap(BitmapObj);
      DC_UnlockDc(dc);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
   }

   if (BitmapObj->dib->dsBmih.biBitCount <= 8 &&
       StartIndex < (1 << BitmapObj->dib->dsBmih.biBitCount))
   {
      if (StartIndex + Entries > (1 << BitmapObj->dib->dsBmih.biBitCount))
         Entries = (1 << BitmapObj->dib->dsBmih.biBitCount) - StartIndex;

      PalGDI = PALETTE_LockPalette(BitmapObj->hDIBPalette);

      for (Index = StartIndex;
           Index < StartIndex + Entries && Index < PalGDI->NumColors;
           Index++)
      {
         PalGDI->IndexedColors[Index].peRed = Colors[Index - StartIndex].rgbRed;
         PalGDI->IndexedColors[Index].peGreen = Colors[Index - StartIndex].rgbGreen;
         PalGDI->IndexedColors[Index].peBlue = Colors[Index - StartIndex].rgbBlue;
      }
      PALETTE_UnlockPalette(PalGDI);
   }
   else
      Entries = 0;

   BITMAPOBJ_UnlockBitmap(BitmapObj);
   DC_UnlockDc(dc);

   return Entries;
}

UINT STDCALL
IntGetDIBColorTable(HDC hDC, UINT StartIndex, UINT Entries, RGBQUAD *Colors)
{
   PDC dc;
   PBITMAPOBJ BitmapObj;
   PPALGDI PalGDI;
   UINT Index;

   if (!(dc = DC_LockDc(hDC))) return 0;
   if (dc->IsIC)
   {
      DC_UnlockDc(dc);
      return 0;
   }

   BitmapObj = BITMAPOBJ_LockBitmap(dc->w.hBitmap);
   if (BitmapObj == NULL)
   {
      DC_UnlockDc(dc);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
   }

   if (BitmapObj->dib == NULL)
   {
      BITMAPOBJ_UnlockBitmap(BitmapObj);
      DC_UnlockDc(dc);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
   }

   if (BitmapObj->dib->dsBmih.biBitCount <= 8 &&
       StartIndex < (1 << BitmapObj->dib->dsBmih.biBitCount))
   {
      if (StartIndex + Entries > (1 << BitmapObj->dib->dsBmih.biBitCount))
         Entries = (1 << BitmapObj->dib->dsBmih.biBitCount) - StartIndex;

      PalGDI = PALETTE_LockPalette(BitmapObj->hDIBPalette);

      for (Index = StartIndex;
           Index < StartIndex + Entries && Index < PalGDI->NumColors;
           Index++)
      {
         Colors[Index - StartIndex].rgbRed = PalGDI->IndexedColors[Index].peRed;
         Colors[Index - StartIndex].rgbGreen = PalGDI->IndexedColors[Index].peGreen;
         Colors[Index - StartIndex].rgbBlue = PalGDI->IndexedColors[Index].peBlue;
      }
      PALETTE_UnlockPalette(PalGDI);
   }
   else
      Entries = 0;

   BITMAPOBJ_UnlockBitmap(BitmapObj);
   DC_UnlockDc(dc);

   return Entries;
}

// Converts a DIB to a device-dependent bitmap
static INT FASTCALL
IntSetDIBits(
	PDC   DC,
	HBITMAP  hBitmap,
	UINT  StartScan,
	UINT  ScanLines,
	CONST VOID  *Bits,
	CONST BITMAPINFO  *bmi,
	UINT  ColorUse)
{
  BITMAPOBJ  *bitmap;
  HBITMAP     SourceBitmap;
  INT         result = 0;
  BOOL        copyBitsResult;
  SURFOBJ    *DestSurf, *SourceSurf;
  SIZEL       SourceSize;
  POINTL      ZeroPoint;
  RECTL       DestRect;
  XLATEOBJ   *XlateObj;
  PPALGDI     hDCPalette;
  //RGBQUAD    *lpRGB;
  HPALETTE    DDB_Palette, DIB_Palette;
  ULONG       DDB_Palette_Type, DIB_Palette_Type;
  INT         DIBWidth;

  // Check parameters
  if (!(bitmap = BITMAPOBJ_LockBitmap(hBitmap)))
  {
    return 0;
  }

  // Get RGB values
  //if (ColorUse == DIB_PAL_COLORS)
  //  lpRGB = DIB_MapPaletteColors(hDC, bmi);
  //else
  //  lpRGB = &bmi->bmiColors;

  DestSurf = &bitmap->SurfObj;

  // Create source surface
  SourceSize.cx = bmi->bmiHeader.biWidth;
  SourceSize.cy = ScanLines;

  // Determine width of DIB
  DIBWidth = DIB_GetDIBWidthBytes(SourceSize.cx, bmi->bmiHeader.biBitCount);

  SourceBitmap = EngCreateBitmap(SourceSize,
                                 DIBWidth,
                                 BitmapFormat(bmi->bmiHeader.biBitCount, bmi->bmiHeader.biCompression),
                                 bmi->bmiHeader.biHeight < 0 ? BMF_TOPDOWN : 0,
                                 (PVOID) Bits);
  if (0 == SourceBitmap)
  {
      BITMAPOBJ_UnlockBitmap(bitmap);
      SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
      return 0;
  }

  SourceSurf = EngLockSurface((HSURF)SourceBitmap);
  if (NULL == SourceSurf)
  {
      EngDeleteSurface((HSURF)SourceBitmap);
      BITMAPOBJ_UnlockBitmap(bitmap);
      SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
      return 0;
  }

  // Destination palette obtained from the hDC
  hDCPalette = PALETTE_LockPalette(DC->DevInfo->hpalDefault);
  if (NULL == hDCPalette)
    {
      EngUnlockSurface(SourceSurf);
      EngDeleteSurface((HSURF)SourceBitmap);
      BITMAPOBJ_UnlockBitmap(bitmap);
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return 0;
    }
  DDB_Palette_Type = hDCPalette->Mode;
  DDB_Palette = DC->DevInfo->hpalDefault;
  PALETTE_UnlockPalette(hDCPalette);

  // Source palette obtained from the BITMAPINFO
  DIB_Palette = BuildDIBPalette ( (PBITMAPINFO)bmi, (PINT)&DIB_Palette_Type );
  if (NULL == DIB_Palette)
    {
      EngUnlockSurface(SourceSurf);
      EngDeleteSurface((HSURF)SourceBitmap);
      BITMAPOBJ_UnlockBitmap(bitmap);
      SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
      return 0;
    }

  // Determine XLATEOBJ for color translation
  XlateObj = IntEngCreateXlate(DDB_Palette_Type, DIB_Palette_Type, DDB_Palette, DIB_Palette);
  if (NULL == XlateObj)
    {
      PALETTE_FreePalette(DIB_Palette);
      EngUnlockSurface(SourceSurf);
      EngDeleteSurface((HSURF)SourceBitmap);
      BITMAPOBJ_UnlockBitmap(bitmap);
      SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
      return 0;
    }

  // Zero point
  ZeroPoint.x = 0;
  ZeroPoint.y = 0;

  // Determine destination rectangle
  DestRect.left	= 0;
  DestRect.top	= abs(bmi->bmiHeader.biHeight) - StartScan - ScanLines;
  DestRect.right	= SourceSize.cx;
  DestRect.bottom	= DestRect.top + ScanLines;

  copyBitsResult = EngCopyBits(DestSurf, SourceSurf, NULL, XlateObj, &DestRect, &ZeroPoint);

  // If it succeeded, return number of scanlines copies
  if(copyBitsResult == TRUE)
  {
    result = SourceSize.cy - 1;
  }

  // Clean up
  EngDeleteXlate(XlateObj);
  PALETTE_FreePalette(DIB_Palette);
  EngUnlockSurface(SourceSurf);
  EngDeleteSurface((HSURF)SourceBitmap);

//  if (ColorUse == DIB_PAL_COLORS)
//    WinFree((LPSTR)lpRGB);

  BITMAPOBJ_UnlockBitmap(bitmap);

  return result;
}

// Converts a DIB to a device-dependent bitmap
INT STDCALL
NtGdiSetDIBits(
	HDC  hDC,
	HBITMAP  hBitmap,
	UINT  StartScan,
	UINT  ScanLines,
	CONST VOID  *Bits,
	CONST BITMAPINFO  *bmi,
	UINT  ColorUse)
{
  PDC Dc;
  INT Ret;

  Dc = DC_LockDc(hDC);
  if (NULL == Dc)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return 0;
    }
  if (Dc->IsIC)
    {
      DC_UnlockDc(Dc);
      return 0;
    }

  Ret = IntSetDIBits(Dc, hBitmap, StartScan, ScanLines, Bits, bmi, ColorUse);

  DC_UnlockDc(Dc);

  return Ret;
}

W32KAPI
INT
APIENTRY
NtGdiSetDIBitsToDeviceInternal(
    IN HDC hDC,
    IN INT XDest,
    IN INT YDest,
    IN DWORD Width,
    IN DWORD Height,
    IN INT XSrc,
    IN INT YSrc,
    IN DWORD StartScan,
    IN DWORD ScanLines,
    IN LPBYTE Bits,
    IN LPBITMAPINFO bmi,
    IN DWORD ColorUse,
    IN UINT cjMaxBits,
    IN UINT cjMaxInfo,
    IN BOOL bTransformCoordinates,
    IN OPTIONAL HANDLE hcmXform
)
{
    INT ret = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    PDC pDC;
    HBITMAP hSourceBitmap;
    SURFOBJ *pDestSurf, *pSourceSurf;
    RECTL rcDest;
    POINTL ptSource;
    INT DIBWidth;
    SIZEL SourceSize;
    XLATEOBJ *XlateObj = NULL;

    pDC = DC_LockDc(hDC);
    if (!pDC)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return 0;
    }
    if (pDC->IsIC)
    {
        DC_UnlockDc(pDC);
        return 0;
    }

    pDestSurf = EngLockSurface((HSURF)pDC->w.hBitmap);
    if (!pDestSurf)
    {
        /* FIXME: SetLastError ? */
        DC_UnlockDc(pDC);
        return 0;
    }

    SourceSize.cx = bmi->bmiHeader.biWidth;
    SourceSize.cy = ScanLines;
    DIBWidth = DIB_GetDIBWidthBytes(SourceSize.cx, bmi->bmiHeader.biBitCount);

    rcDest.left = XDest;
    rcDest.top = YDest;
    rcDest.right = XDest + Width;
    rcDest.bottom = YDest + Height;
    ptSource.x = XSrc;
    ptSource.y = YSrc;

    /* Enter SEH, as the bits are user mode */
    _SEH_TRY
    {
        ProbeForRead(Bits, DIBWidth * abs(bmi->bmiHeader.biHeight), 1);
        hSourceBitmap = EngCreateBitmap(SourceSize,
                                        DIBWidth,
                                        BitmapFormat(bmi->bmiHeader.biBitCount, bmi->bmiHeader.biCompression),
                                        bmi->bmiHeader.biHeight < 0 ? BMF_TOPDOWN : 0,
                                        (PVOID) Bits);
        if (!hSourceBitmap)
        {
            SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
            Status = STATUS_NO_MEMORY;
            _SEH_LEAVE;
        }

        pSourceSurf = EngLockSurface((HSURF)hSourceBitmap);
        if (!pSourceSurf)
        {
            EngDeleteSurface((HSURF)hSourceBitmap);
            Status = STATUS_UNSUCCESSFUL;
            _SEH_LEAVE;
        }

        /* FIXME: handle XlateObj */
        XlateObj = NULL;

        /* Copy the bits */
        Status = IntEngBitBlt(pDestSurf,
                              pSourceSurf,
                              NULL,
                              pDC->CombinedClip,
                              XlateObj,
                              &rcDest,
                              &ptSource,
                              NULL,
                              NULL,
                              NULL,
                              ROP3_TO_ROP4(SRCCOPY));

        EngUnlockSurface(pSourceSurf);
        EngDeleteSurface((HSURF)hSourceBitmap);
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END

    if (NT_SUCCESS(Status))
    {
        /* FIXME: Should probably be only the number of lines actually copied */
        ret = ScanLines;
    }

    EngUnlockSurface(pDestSurf);
    DC_UnlockDc(pDC);

    return ret;
}

/* Converts a device-dependent bitmap to a DIB */
INT STDCALL
NtGdiGetDIBitsInternal(HDC hDC,
                       HBITMAP hBitmap,
                       UINT StartScan,
                       UINT ScanLines,
                       LPBYTE Bits,
                       LPBITMAPINFO Info,
                       UINT Usage,
                       UINT MaxBits,
                       UINT MaxInfo)
{
    BITMAPOBJ *BitmapObj;
    SURFOBJ *DestSurfObj;
    XLATEOBJ *XlateObj;
    HBITMAP DestBitmap;
    SIZEL DestSize;
    HPALETTE hSourcePalette;
    HPALETTE hDestPalette;
    PPALGDI SourcePalette;
    PPALGDI DestPalette;
    ULONG SourcePaletteType;
    ULONG DestPaletteType;
    PDC Dc;
    POINTL SourcePoint;
    RECTL DestRect;
    ULONG Result = 0;
    ULONG Index;

    /* Get handle for the palette in DC. */
    Dc = DC_LockDc(hDC);
    if (Dc == NULL)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return 0;
    }
    if (Dc->IsIC)
    {
        DC_UnlockDc(Dc);
        return 0;
    }

    hSourcePalette = Dc->w.hPalette;
    hDestPalette = Dc->w.hPalette; // unsure of this (Ged)
    DC_UnlockDc(Dc);

    /* Get pointer to the source bitmap object. */
    BitmapObj = BITMAPOBJ_LockBitmap(hBitmap);
    if (BitmapObj == NULL)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return 0;
    }

    if (Bits == NULL)
    {
        if (Info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
        {
            BITMAPCOREHEADER* coreheader = (BITMAPCOREHEADER*) Info;
            coreheader->bcWidth =BitmapObj->SurfObj.sizlBitmap.cx;
            coreheader->bcPlanes = 1;
            coreheader->bcBitCount =  BitsPerFormat(BitmapObj->SurfObj.iBitmapFormat);

            coreheader->bcHeight = BitmapObj->SurfObj.sizlBitmap.cy;

            if (BitmapObj->SurfObj.lDelta > 0)
                coreheader->bcHeight = -coreheader->bcHeight;

            Result = BitmapObj->SurfObj.sizlBitmap.cy;
        }

        if (Info->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
        {
            Info->bmiHeader.biWidth = BitmapObj->SurfObj.sizlBitmap.cx;
            Info->bmiHeader.biHeight = BitmapObj->SurfObj.sizlBitmap.cy;
            /* Report negtive height for top-down bitmaps. */
            if (BitmapObj->SurfObj.lDelta > 0)
                Info->bmiHeader.biHeight = -Info->bmiHeader.biHeight;
            Info->bmiHeader.biPlanes = 1;
            Info->bmiHeader.biBitCount = BitsPerFormat(BitmapObj->SurfObj.iBitmapFormat);
            if (Info->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
            {
                switch (BitmapObj->SurfObj.iBitmapFormat)
                {
                    case BMF_1BPP:
                    case BMF_4BPP:
                    case BMF_8BPP:
                    case BMF_16BPP:
                    case BMF_24BPP:
                    case BMF_32BPP:
                        Info->bmiHeader.biCompression = BI_RGB;
                        break;
                    case BMF_4RLE:
                        Info->bmiHeader.biCompression = BI_RLE4;
                        break;
                    case BMF_8RLE:
                        Info->bmiHeader.biCompression = BI_RLE8;
                        break;
                    case BMF_JPEG:
                        Info->bmiHeader.biCompression = BI_JPEG;
                        break;
                    case BMF_PNG:
                        Info->bmiHeader.biCompression = BI_PNG;
                        break;
                }

                Info->bmiHeader.biSizeImage = BitmapObj->SurfObj.cjBits;
                Info->bmiHeader.biXPelsPerMeter = 0; /* FIXME */
                Info->bmiHeader.biYPelsPerMeter = 0; /* FIXME */
                Info->bmiHeader.biClrUsed = 0;
                Info->bmiHeader.biClrImportant = 1 << Info->bmiHeader.biBitCount; /* FIXME */
                Result = BitmapObj->SurfObj.sizlBitmap.cy;
            }
        }
    }
    else
    {
        if (StartScan > BitmapObj->SurfObj.sizlBitmap.cy)
        {
            Result = 0;
        }
        else
        {
            ScanLines = min(ScanLines, BitmapObj->SurfObj.sizlBitmap.cy - StartScan);
            DestSize.cx = BitmapObj->SurfObj.sizlBitmap.cx;
            DestSize.cy = ScanLines;

            DestBitmap = NULL;
            if (Info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
            {
                BITMAPCOREHEADER* coreheader = (BITMAPCOREHEADER*) Info;

                DestBitmap = EngCreateBitmap(DestSize,
                                             DIB_GetDIBWidthBytes(DestSize.cx, coreheader->bcBitCount),
                                             BitmapFormat(coreheader->bcBitCount, BI_RGB),
                                             0 < coreheader->bcHeight ? 0 : BMF_TOPDOWN,
                                             Bits);
            }

            if (Info->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
            {
                INT one, two, three;

                one = DIB_GetDIBWidthBytes(DestSize.cx, Info->bmiHeader.biBitCount),
                two = ((DestSize.cx * Info->bmiHeader.biBitCount + 31) & ~31) >> 3;
                three = DestSize.cx * (Info->bmiHeader.biBitCount >> 3);

                DestBitmap = EngCreateBitmap(DestSize,
                                            /* DIB_GetDIBWidthBytes(DestSize.cx, Info->bmiHeader.biBitCount), */
                                            DestSize.cx * (Info->bmiHeader.biBitCount >> 3), /* HACK */
                                            BitmapFormat(Info->bmiHeader.biBitCount, Info->bmiHeader.biCompression),
                                            0 < Info->bmiHeader.biHeight ? 0 : BMF_TOPDOWN,
                                            Bits);
            }

            if(DestBitmap == NULL)
            {
                BITMAPOBJ_UnlockBitmap(BitmapObj);
                return 0;
            }

            DestSurfObj = EngLockSurface((HSURF)DestBitmap);

            SourcePalette = PALETTE_LockPalette(hSourcePalette);
            /* FIXME - SourcePalette can be NULL!!! Don't assert here! */
            ASSERT(SourcePalette);
            SourcePaletteType = SourcePalette->Mode;
            PALETTE_UnlockPalette(SourcePalette);

            DestPalette = PALETTE_LockPalette(hDestPalette);
            /* FIXME - DestPalette can be NULL!!!! Don't assert here!!! */
            //ASSERT(DestPalette);
            DestPaletteType = DestPalette->Mode;

            /* Copy palette. */
            /* FIXME: This is largely incomplete. */
            if (Info->bmiHeader.biBitCount <= 8)
            {
                if (Usage == DIB_RGB_COLORS)
                {
                    for (Index = 0;
                        Index < (1 << Info->bmiHeader.biBitCount) && Index < DestPalette->NumColors;
                        Index++)
                    {
                        Info->bmiColors[Index].rgbRed = DestPalette->IndexedColors[Index].peRed;
                        Info->bmiColors[Index].rgbGreen = DestPalette->IndexedColors[Index].peGreen;
                        Info->bmiColors[Index].rgbBlue =  DestPalette->IndexedColors[Index].peBlue;
                    }
                }

                if (Usage == DIB_PAL_COLORS)
                {
                   DPRINT1("GetDIBits with DIB_PAL_COLORS isn't implemented yet\n");
                }
            }

            PALETTE_UnlockPalette(DestPalette);

            XlateObj = IntEngCreateXlate(DestPaletteType, SourcePaletteType, hDestPalette, hSourcePalette);

            SourcePoint.x = 0;
            SourcePoint.y = BitmapObj->SurfObj.sizlBitmap.cy - (StartScan + ScanLines);

            /* Determine destination rectangle */
            DestRect.top = 0;
            DestRect.left = 0;
            DestRect.right = DestSize.cx;
            DestRect.bottom = DestSize.cy;

            if (EngCopyBits(DestSurfObj,
                            &BitmapObj->SurfObj,
                            NULL,
                            XlateObj,
                            &DestRect,
                            &SourcePoint))
            {
                Result = ScanLines;
            }

            EngDeleteXlate(XlateObj);
            EngUnlockSurface(DestSurfObj);
        }
    }

    BITMAPOBJ_UnlockBitmap(BitmapObj);

    return Result;
}

INT STDCALL NtGdiStretchDIBits(HDC  hDC,
                               INT  XDest,
                               INT  YDest,
                               INT  DestWidth,
                               INT  DestHeight,
                               INT  XSrc,
                               INT  YSrc,
                               INT  SrcWidth,
                               INT  SrcHeight,
                               CONST VOID *Bits,
                               CONST BITMAPINFO *BitsInfo,
                               UINT  Usage,
                               DWORD  ROP)
{
   HBITMAP hBitmap, hOldBitmap;
   HDC hdcMem;
   HPALETTE hPal = NULL;

   if (!Bits || !BitsInfo)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
   }

   hdcMem = NtGdiCreateCompatibleDC(hDC);
   hBitmap = NtGdiCreateCompatibleBitmap(hDC, BitsInfo->bmiHeader.biWidth,
                                         BitsInfo->bmiHeader.biHeight);
   hOldBitmap = NtGdiSelectObject(hdcMem, hBitmap);

   if(Usage == DIB_PAL_COLORS)
   {
      hPal = NtGdiGetDCObject(hDC, GDI_OBJECT_TYPE_PALETTE);
      hPal = NtUserSelectPalette(hdcMem, hPal, FALSE);
   }

   if (BitsInfo->bmiHeader.biCompression == BI_RLE4 ||
       BitsInfo->bmiHeader.biCompression == BI_RLE8)
   {
      /* copy existing bitmap from destination dc */
      if (SrcWidth == DestWidth && SrcHeight == DestHeight)
         NtGdiBitBlt(hdcMem, XSrc, abs(BitsInfo->bmiHeader.biHeight) - SrcHeight - YSrc,
                     SrcWidth, SrcHeight, hDC, XDest, YDest, ROP, 0, 0);
      else
         NtGdiStretchBlt(hdcMem, XSrc, abs(BitsInfo->bmiHeader.biHeight) - SrcHeight - YSrc,
                         SrcWidth, SrcHeight, hDC, XDest, YDest, DestWidth, DestHeight,
                         ROP, 0);
   }

   NtGdiSetDIBits(hdcMem, hBitmap, 0, BitsInfo->bmiHeader.biHeight, Bits,
                  BitsInfo, Usage);

   /* Origin for DIBitmap may be bottom left (positive biHeight) or top
      left (negative biHeight) */
   if (SrcWidth == DestWidth && SrcHeight == DestHeight)
      NtGdiBitBlt(hDC, XDest, YDest, DestWidth, DestHeight,
                  hdcMem, XSrc, abs(BitsInfo->bmiHeader.biHeight) - SrcHeight - YSrc,
                  ROP, 0, 0);
   else
      NtGdiStretchBlt(hDC, XDest, YDest, DestWidth, DestHeight,
                      hdcMem, XSrc, abs(BitsInfo->bmiHeader.biHeight) - SrcHeight - YSrc,
                      SrcWidth, SrcHeight, ROP, 0);

   if(hPal)
      NtUserSelectPalette(hdcMem, hPal, FALSE);

   NtGdiSelectObject(hdcMem, hOldBitmap);
   NtGdiDeleteObjectApp(hdcMem);
   NtGdiDeleteObject(hBitmap);

   return SrcHeight;
}


HBITMAP FASTCALL
IntCreateDIBitmap(PDC Dc, const BITMAPINFOHEADER *header,
                  DWORD init, LPCVOID bits, const BITMAPINFO *data,
                  UINT coloruse)
{
  HBITMAP handle;
 
  LONG width;
  LONG height;
  WORD planes; 
  WORD bpp;
  LONG compr;
  LONG dibsize;        
  BOOL fColor;
  SIZEL size;


  if (DIB_GetBitmapInfo( header, &width, &height, &planes, &bpp, &compr, &dibsize ) == -1) return 0;  

    
  // Check if we should create a monochrome or color bitmap. We create a monochrome bitmap only if it has exactly 2
  // colors, which are black followed by white, nothing else. In all other cases, we create a color bitmap.

  if (bpp != 1) fColor = TRUE;
  else if ((coloruse != DIB_RGB_COLORS) ||
           (init != CBM_INIT) || !data) fColor = FALSE;
  else
  {
    if (data->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
    {
      const RGBQUAD *rgb = data->bmiColors;
      DWORD col = RGB( rgb->rgbRed, rgb->rgbGreen, rgb->rgbBlue );

      // Check if the first color of the colormap is black
      if ((col == RGB(0, 0, 0)))
      {
        rgb++;
        col = RGB( rgb->rgbRed, rgb->rgbGreen, rgb->rgbBlue );

        // If the second color is white, create a monochrome bitmap
        fColor =  (col != RGB(0xff,0xff,0xff));
      }
    else fColor = TRUE;
  }
  else if (data->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
  {
    RGBTRIPLE *rgb = ((BITMAPCOREINFO *)data)->bmciColors;
     DWORD col = RGB( rgb->rgbtRed, rgb->rgbtGreen, rgb->rgbtBlue);

    if ((col == RGB(0,0,0)))
    {
      rgb++;
      col = RGB( rgb->rgbtRed, rgb->rgbtGreen, rgb->rgbtBlue );
      fColor = (col != RGB(0xff,0xff,0xff));
    }
    else fColor = TRUE;
  }
  else
  {
      DPRINT("(%ld): wrong size for data\n", data->bmiHeader.biSize );
      return 0;
    }
  }

  // Now create the bitmap
  if (fColor)
  {
    handle = IntCreateCompatibleBitmap(Dc, width, height);
  }
  else
  {
    size.cx = width;
    size.cy = abs(height);

    handle = IntCreateBitmap(size, DIB_GetDIBWidthBytes(width, 1), BMF_1BPP,
                             (height < 0 ? BMF_TOPDOWN : 0) | BMF_NOZEROINIT,
                             NULL);
  }

  if (height < 0)
    height = -height;

  if (NULL != handle && CBM_INIT == init)
  {
    IntSetDIBits(Dc, handle, 0, height, bits, data, coloruse);
  }

  return handle;
}

// The CreateDIBitmap function creates a device-dependent bitmap (DDB) from a DIB and, optionally, sets the bitmap bits
// The DDB that is created will be whatever bit depth your reference DC is
HBITMAP STDCALL NtGdiCreateDIBitmap(HDC hDc, const BITMAPINFOHEADER *Header,
                                    DWORD Init, LPCVOID Bits, const BITMAPINFO *Data,
                                    UINT ColorUse)
{
  PDC Dc;
  HBITMAP Bmp;
  
  if (Header == NULL)
  {
	  return NULL;
  }

  if (Header->biSize == 0)
  {
	  return NULL;
  }

  if (NULL == hDc)
  {     
	   BITMAPINFOHEADER *change_Header = (BITMAPINFOHEADER *)Header;
       hDc =  IntGdiCreateDC(NULL, NULL, NULL, NULL,FALSE);
       if (hDc == NULL)
       {         
          SetLastWin32Error(ERROR_INVALID_HANDLE);
          return NULL;
       }
      Dc = DC_LockDc(hDc);
      if (Dc == NULL)
      {  
          NtGdiDeleteObjectApp(hDc);          
          SetLastWin32Error(ERROR_INVALID_HANDLE);
          return NULL;
      }
      
	  change_Header->biBitCount = 1;
	  change_Header->biPlanes = 1;

      Bmp = IntCreateDIBitmap(Dc, Header, Init, Bits, Data, ColorUse);                      
      DC_UnlockDc(Dc);
      NtGdiDeleteObjectApp(hDc);  
    }
    else
    {
       Dc = DC_LockDc(hDc);
       if (Dc == NULL)
       {                    
          SetLastWin32Error(ERROR_INVALID_HANDLE);
          return NULL;
       }
       Bmp = IntCreateDIBitmap(Dc, Header, Init, Bits, Data, ColorUse);
       DC_UnlockDc(Dc);
    }

  

  return Bmp;
}

HBITMAP STDCALL NtGdiCreateDIBSection(HDC hDC,
                              IN OPTIONAL HANDLE hSection,
                              IN DWORD dwOffset,
                              IN LPBITMAPINFO bmi,
                              DWORD  Usage,
                              IN UINT cjHeader,
                              IN FLONG fl,
                              IN ULONG_PTR dwColorSpace,
                              PVOID *Bits)
{
  HBITMAP hbitmap = 0;
  DC *dc;
  BOOL bDesktopDC = FALSE;

  // If the reference hdc is null, take the desktop dc
  if (hDC == 0)
  {
    hDC = NtGdiCreateCompatibleDC(0);
    bDesktopDC = TRUE;
  }

  if ((dc = DC_LockDc(hDC)))
  {
    hbitmap = DIB_CreateDIBSection ( dc, (BITMAPINFO*)bmi, Usage, Bits,
      hSection, dwOffset, 0);
    DC_UnlockDc(dc);
  }
  else
  {
    SetLastWin32Error(ERROR_INVALID_HANDLE);
  }

  if (bDesktopDC)
    NtGdiDeleteObjectApp(hDC);

  return hbitmap;
}

HBITMAP STDCALL
DIB_CreateDIBSection(
  PDC dc, BITMAPINFO *bmi, UINT usage,
  LPVOID *bits, HANDLE section,
  DWORD offset, DWORD ovr_pitch)
{
  HBITMAP res = 0;
  BITMAPOBJ *bmp = NULL;
  DIBSECTION *dib = NULL;

  // Fill BITMAP32 structure with DIB data
  BITMAPINFOHEADER *bi = &bmi->bmiHeader;
  INT effHeight;
  ULONG totalSize;
  UINT Entries = 0;
  BITMAP bm;
  SIZEL Size;
  RGBQUAD *lpRGB;

  DPRINT("format (%ld,%ld), planes %d, bpp %d, size %ld, colors %ld (%s)\n",
	bi->biWidth, bi->biHeight, bi->biPlanes, bi->biBitCount,
	bi->biSizeImage, bi->biClrUsed, usage == DIB_PAL_COLORS? "PAL" : "RGB");

  effHeight = bi->biHeight >= 0 ? bi->biHeight : -bi->biHeight;
  bm.bmType = 0;
  bm.bmWidth = bi->biWidth;
  bm.bmHeight = effHeight;
  bm.bmWidthBytes = ovr_pitch ? ovr_pitch : (ULONG) DIB_GetDIBWidthBytes(bm.bmWidth, bi->biBitCount);

  bm.bmPlanes = bi->biPlanes;
  bm.bmBitsPixel = bi->biBitCount;
  bm.bmBits = NULL;

  // Get storage location for DIB bits.  Only use biSizeImage if it's valid and
  // we're dealing with a compressed bitmap.  Otherwise, use width * height.
  totalSize = bi->biSizeImage && bi->biCompression != BI_RGB
    ? bi->biSizeImage : (ULONG) (bm.bmWidthBytes * effHeight);

  if (section)
  {
/*    bm.bmBits = MapViewOfFile(section, FILE_MAP_ALL_ACCESS,
			      0L, offset, totalSize); */
    DbgPrint("DIB_CreateDIBSection: Cannot yet handle section DIBs\n");
    SetLastWin32Error(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
  }
  else if (ovr_pitch && offset)
    bm.bmBits = (LPVOID) offset;
  else {
    offset = 0;
    bm.bmBits = EngAllocUserMem(totalSize, 0);
  }

  if(usage == DIB_PAL_COLORS)
    lpRGB = DIB_MapPaletteColors(dc, bmi);
  else
    lpRGB = bmi->bmiColors;

  // Allocate Memory for DIB and fill structure
  if (bm.bmBits)
  {
    dib = ExAllocatePoolWithTag(PagedPool, sizeof(DIBSECTION), TAG_DIB);
    if (dib != NULL) RtlZeroMemory(dib, sizeof(DIBSECTION));
  }

  if (dib)
  {
    dib->dsBm = bm;
    dib->dsBmih = *bi;
    dib->dsBmih.biSizeImage = totalSize;

    /* Set dsBitfields values */
    if ( usage == DIB_PAL_COLORS || bi->biBitCount <= 8)
    {
      dib->dsBitfields[0] = dib->dsBitfields[1] = dib->dsBitfields[2] = 0;
    }
    else switch(bi->biBitCount)
    {
      case 16:
        dib->dsBitfields[0] = (bi->biCompression == BI_BITFIELDS) ? *(DWORD *)lpRGB : 0x7c00;
        dib->dsBitfields[1] = (bi->biCompression == BI_BITFIELDS) ? *((DWORD *)lpRGB + 1) : 0x03e0;
        dib->dsBitfields[2] = (bi->biCompression == BI_BITFIELDS) ? *((DWORD *)lpRGB + 2) : 0x001f;        break;

      case 24:
        dib->dsBitfields[0] = 0xff0000;
        dib->dsBitfields[1] = 0x00ff00;
        dib->dsBitfields[2] = 0x0000ff;
        break;

      case 32:
        dib->dsBitfields[0] = (bi->biCompression == BI_BITFIELDS) ? *(DWORD *)lpRGB : 0xff0000;
        dib->dsBitfields[1] = (bi->biCompression == BI_BITFIELDS) ? *((DWORD *)lpRGB + 1) : 0x00ff00;
        dib->dsBitfields[2] = (bi->biCompression == BI_BITFIELDS) ? *((DWORD *)lpRGB + 2) : 0x0000ff;
        break;
    }
    dib->dshSection = section;
    dib->dsOffset = offset;
  }

  // Create Device Dependent Bitmap and add DIB pointer
  if (dib)
  {
    Size.cx = bm.bmWidth;
    Size.cy = abs(bm.bmHeight);
    res = IntCreateBitmap(Size, bm.bmWidthBytes,
                          BitmapFormat(bi->biBitCount * bi->biPlanes, bi->biCompression),
                          BMF_DONTCACHE | BMF_USERMEM | BMF_NOZEROINIT |
                          (bi->biHeight < 0 ? BMF_TOPDOWN : 0),
                          bm.bmBits);
    if (! res)
      {
        if (lpRGB != bmi->bmiColors)
          {
            ExFreePool(lpRGB);
          }
        SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
	return NULL;
      }
    bmp = BITMAPOBJ_LockBitmap(res);
    if (NULL == bmp)
      {
        if (lpRGB != bmi->bmiColors)
          {
            ExFreePool(lpRGB);
          }
	SetLastWin32Error(ERROR_INVALID_HANDLE);
	NtGdiDeleteObject(bmp);
	return NULL;
      }
    bmp->dib = (DIBSECTION *) dib;
    bmp->flFlags = BITMAPOBJ_IS_APIBITMAP;

    /* WINE NOTE: WINE makes use of a colormap, which is a color translation table between the DIB and the X physical
                  device. Obviously, this is left out of the ReactOS implementation. Instead, we call
                  NtGdiSetDIBColorTable. */
    if(bi->biBitCount == 1) { Entries = 2; } else
    if(bi->biBitCount == 4) { Entries = 16; } else
    if(bi->biBitCount == 8) { Entries = 256; }

    if (Entries)
      bmp->hDIBPalette = PALETTE_AllocPaletteIndexedRGB(Entries, lpRGB);
    else
      bmp->hDIBPalette = PALETTE_AllocPalette(PAL_BITFIELDS, 0, NULL,
                                              dib->dsBitfields[0],
                                              dib->dsBitfields[1],
                                              dib->dsBitfields[2]);
  }

  // Clean up in case of errors
  if (!res || !bmp || !dib || !bm.bmBits)
  {
    DPRINT("got an error res=%08x, bmp=%p, dib=%p, bm.bmBits=%p\n", res, bmp, dib, bm.bmBits);
/*      if (bm.bmBits)
      {
      if (section)
        UnmapViewOfFile(bm.bmBits), bm.bmBits = NULL;
      else if (!offset)
      VirtualFree(bm.bmBits, 0L, MEM_RELEASE), bm.bmBits = NULL;
    } */

    if (dib) { ExFreePool(dib); dib = NULL; }
    if (bmp) { bmp = NULL; }
    if (res) { BITMAPOBJ_FreeBitmap(res); res = 0; }
  }

  if (lpRGB != bmi->bmiColors)
    {
      ExFreePool(lpRGB);
    }

  if (bmp)
    {
      BITMAPOBJ_UnlockBitmap(bmp);
    }

  // Return BITMAP handle and storage location
  if (NULL != bm.bmBits && NULL != bits)
    {
      *bits = bm.bmBits;
    }

  return res;
}

/***********************************************************************
 *           DIB_GetDIBWidthBytes
 *
 * Return the width of a DIB bitmap in bytes. DIB bitmap data is 32-bit aligned.
 * http://www.microsoft.com/msdn/sdk/platforms/doc/sdk/win32/struc/src/str01.htm
 * 11/16/1999 (RJJ) lifted from wine
 */
INT FASTCALL DIB_GetDIBWidthBytes (INT width, INT depth)
{
  return ((width * depth + 31) & ~31) >> 3;
}

/***********************************************************************
 *           DIB_GetDIBImageBytes
 *
 * Return the number of bytes used to hold the image in a DIB bitmap.
 * 11/16/1999 (RJJ) lifted from wine
 */

INT STDCALL DIB_GetDIBImageBytes (INT  width, INT height, INT depth)
{
  return DIB_GetDIBWidthBytes( width, depth ) * (height < 0 ? -height : height);
}

/***********************************************************************
 *           DIB_BitmapInfoSize
 *
 * Return the size of the bitmap info structure including color table.
 * 11/16/1999 (RJJ) lifted from wine
 */

INT FASTCALL DIB_BitmapInfoSize (const BITMAPINFO * info, WORD coloruse)
{
  int colors;

  if (info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
  {
    BITMAPCOREHEADER *core = (BITMAPCOREHEADER *)info;
    colors = (core->bcBitCount <= 8) ? 1 << core->bcBitCount : 0;
    return sizeof(BITMAPCOREHEADER) + colors * ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBTRIPLE) : sizeof(WORD));
  }
  else  /* assume BITMAPINFOHEADER */
  {
    colors = info->bmiHeader.biClrUsed;
    if (!colors && (info->bmiHeader.biBitCount <= 8)) colors = 1 << info->bmiHeader.biBitCount;
    return sizeof(BITMAPINFOHEADER) + colors * ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBQUAD) : sizeof(WORD));
  }
}

/*
 * DIB_GetBitmapInfo is complete copy of wine cvs 2/9-2006 
 * from file dib.c from gdi32.dll or orginal version
 * did not calc the info right for some headers.  
 */

INT STDCALL 
DIB_GetBitmapInfo( const BITMAPINFOHEADER *header, 
				  PLONG width, 
				  PLONG height, 
				  PWORD planes, 
				  PWORD bpp, 
				  PLONG compr, 
				  PLONG size )
{  

  if (header->biSize == sizeof(BITMAPCOREHEADER))
  {
     BITMAPCOREHEADER *core = (BITMAPCOREHEADER *)header;
     *width  = core->bcWidth;
     *height = core->bcHeight;
     *planes = core->bcPlanes;
     *bpp    = core->bcBitCount;
     *compr  = 0;
     *size   = 0;
     return 0;
  }

  if (header->biSize == sizeof(BITMAPINFOHEADER))
  {
     *width  = header->biWidth;
     *height = header->biHeight;
     *planes = header->biPlanes;
     *bpp    = header->biBitCount;
     *compr  = header->biCompression;
     *size   = header->biSizeImage;
     return 1;
  }

  if (header->biSize == sizeof(BITMAPV4HEADER))
  {
      BITMAPV4HEADER *v4hdr = (BITMAPV4HEADER *)header;
      *width  = v4hdr->bV4Width;
      *height = v4hdr->bV4Height;
      *planes = v4hdr->bV4Planes;
      *bpp    = v4hdr->bV4BitCount;
      *compr  = v4hdr->bV4V4Compression;
      *size   = v4hdr->bV4SizeImage;
      return 4;
  }

  if (header->biSize == sizeof(BITMAPV5HEADER))
  {
      BITMAPV5HEADER *v5hdr = (BITMAPV5HEADER *)header;
      *width  = v5hdr->bV5Width;
      *height = v5hdr->bV5Height;
      *planes = v5hdr->bV5Planes;
      *bpp    = v5hdr->bV5BitCount;
      *compr  = v5hdr->bV5Compression;
      *size   = v5hdr->bV5SizeImage;
      return 5;
  }
  DPRINT("(%ld): wrong size for header\n", header->biSize );
  return -1;
}

// Converts a Device Independent Bitmap (DIB) to a Device Dependant Bitmap (DDB)
// The specified Device Context (DC) defines what the DIB should be converted to
PBITMAPOBJ FASTCALL DIBtoDDB(HGLOBAL hPackedDIB, HDC hdc) // FIXME: This should be removed. All references to this function should
						 // change to NtGdiSetDIBits
{
  HBITMAP hBmp = 0;
  PBITMAPOBJ pBmp = NULL;
  DIBSECTION *dib;
  LPBYTE pbits = NULL;

  // Get a pointer to the packed DIB's data
  // pPackedDIB = (LPBYTE)GlobalLock(hPackedDIB);
  dib = hPackedDIB;

  pbits = (LPBYTE)(dib + DIB_BitmapInfoSize((BITMAPINFO*)&dib->dsBmih, DIB_RGB_COLORS));

  // Create a DDB from the DIB
  hBmp = NtGdiCreateDIBitmap ( hdc, &dib->dsBmih, CBM_INIT,
    (LPVOID)pbits, (BITMAPINFO*)&dib->dsBmih, DIB_RGB_COLORS);

  // GlobalUnlock(hPackedDIB);

  // Retrieve the internal Pixmap from the DDB
  pBmp = BITMAPOBJ_LockBitmap(hBmp);

  return pBmp;
}

RGBQUAD * FASTCALL
DIB_MapPaletteColors(PDC dc, CONST BITMAPINFO* lpbmi)
{
  RGBQUAD *lpRGB;
  ULONG nNumColors,i;
  USHORT *lpIndex;
  PPALGDI palGDI;

  palGDI = PALETTE_LockPalette(dc->w.hPalette);

  if (NULL == palGDI)
    {
//      RELEASEDCINFO(hDC);
      return NULL;
    }

  if (palGDI->Mode != PAL_INDEXED)
    {
      PALETTE_UnlockPalette(palGDI);
      palGDI = PALETTE_LockPalette(dc->PalIndexed);
      if (palGDI->Mode != PAL_INDEXED)
        {
          return NULL;
        }
    }

  nNumColors = 1 << lpbmi->bmiHeader.biBitCount;
  if (lpbmi->bmiHeader.biClrUsed)
    {
      nNumColors = min(nNumColors, lpbmi->bmiHeader.biClrUsed);
    }

  lpRGB = (RGBQUAD *)ExAllocatePoolWithTag(PagedPool, sizeof(RGBQUAD) * nNumColors, TAG_COLORMAP);
  if (lpRGB == NULL)
  {
     PALETTE_UnlockPalette(palGDI);
     return NULL;   
  }
  
  lpIndex = (USHORT *)&lpbmi->bmiColors[0];

  for (i = 0; i < nNumColors; i++)
    {
      lpRGB[i].rgbRed = palGDI->IndexedColors[*lpIndex].peRed;
      lpRGB[i].rgbGreen = palGDI->IndexedColors[*lpIndex].peGreen;
      lpRGB[i].rgbBlue = palGDI->IndexedColors[*lpIndex].peBlue;
      lpIndex++;
    }
//    RELEASEDCINFO(hDC);
  PALETTE_UnlockPalette(palGDI);

  return lpRGB;
}

HPALETTE FASTCALL
BuildDIBPalette (CONST BITMAPINFO *bmi, PINT paletteType)
{
  BYTE bits;
  ULONG ColorCount;
  PALETTEENTRY *palEntries = NULL;
  HPALETTE hPal;
  ULONG RedMask, GreenMask, BlueMask;

  // Determine Bits Per Pixel
  bits = bmi->bmiHeader.biBitCount;

  // Determine paletteType from Bits Per Pixel
  if (bits <= 8)
    {
      *paletteType = PAL_INDEXED;
      RedMask = GreenMask = BlueMask = 0;
    }
  else if(bits < 24)
    {
      *paletteType = PAL_BITFIELDS;
      RedMask = 0xf800;
      GreenMask = 0x07e0;
      BlueMask = 0x001f;
    }
  else
    {
      *paletteType = PAL_BGR;
      RedMask = 0xff0000;
      GreenMask = 0x00ff00;
      BlueMask = 0x0000ff;
    }

  if (bmi->bmiHeader.biClrUsed == 0)
    {
      ColorCount = 1 << bmi->bmiHeader.biBitCount;
    }
  else
    {
      ColorCount = bmi->bmiHeader.biClrUsed;
    }

  if (PAL_INDEXED == *paletteType)
    {
      hPal = PALETTE_AllocPaletteIndexedRGB(ColorCount, (RGBQUAD*)bmi->bmiColors);
    }
  else
    {
      hPal = PALETTE_AllocPalette(*paletteType, ColorCount,
                                  (ULONG*) palEntries,
                                  RedMask, GreenMask, BlueMask );
    }

  return hPal;
}

/* EOF */
