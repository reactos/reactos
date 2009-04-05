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

static const RGBQUAD EGAColorsQuads[16] = {
/* rgbBlue, rgbGreen, rgbRed, rgbReserved */
    { 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x80, 0x00 },
    { 0x00, 0x80, 0x00, 0x00 },
    { 0x00, 0x80, 0x80, 0x00 },
    { 0x80, 0x00, 0x00, 0x00 },
    { 0x80, 0x00, 0x80, 0x00 },
    { 0x80, 0x80, 0x00, 0x00 },
    { 0x80, 0x80, 0x80, 0x00 },
    { 0xc0, 0xc0, 0xc0, 0x00 },
    { 0x00, 0x00, 0xff, 0x00 },
    { 0x00, 0xff, 0x00, 0x00 },
    { 0x00, 0xff, 0xff, 0x00 },
    { 0xff, 0x00, 0x00, 0x00 },
    { 0xff, 0x00, 0xff, 0x00 },
    { 0xff, 0xff, 0x00, 0x00 },
    { 0xff, 0xff, 0xff, 0x00 }
};

static const RGBQUAD DefLogPaletteQuads[20] = { /* Copy of Default Logical Palette */
/* rgbBlue, rgbGreen, rgbRed, rgbReserved */
    { 0x00, 0x00, 0x00, 0x00 },
    { 0x00, 0x00, 0x80, 0x00 },
    { 0x00, 0x80, 0x00, 0x00 },
    { 0x00, 0x80, 0x80, 0x00 },
    { 0x80, 0x00, 0x00, 0x00 },
    { 0x80, 0x00, 0x80, 0x00 },
    { 0x80, 0x80, 0x00, 0x00 },
    { 0xc0, 0xc0, 0xc0, 0x00 },
    { 0xc0, 0xdc, 0xc0, 0x00 },
    { 0xf0, 0xca, 0xa6, 0x00 },
    { 0xf0, 0xfb, 0xff, 0x00 },
    { 0xa4, 0xa0, 0xa0, 0x00 },
    { 0x80, 0x80, 0x80, 0x00 },
    { 0x00, 0x00, 0xf0, 0x00 },
    { 0x00, 0xff, 0x00, 0x00 },
    { 0x00, 0xff, 0xff, 0x00 },
    { 0xff, 0x00, 0x00, 0x00 },
    { 0xff, 0x00, 0xff, 0x00 },
    { 0xff, 0xff, 0x00, 0x00 },
    { 0xff, 0xff, 0xff, 0x00 }
};


UINT APIENTRY
IntSetDIBColorTable(HDC hDC, UINT StartIndex, UINT Entries, CONST RGBQUAD *Colors)
{
   PDC dc;
   PSURFACE psurf;
   PPALGDI PalGDI;
   UINT Index;
   ULONG biBitCount;

   if (!(dc = DC_LockDc(hDC))) return 0;
   if (dc->dctype == DC_TYPE_INFO)
   {
      DC_UnlockDc(dc);
      return 0;
   }

   psurf = SURFACE_LockSurface(dc->rosdc.hBitmap);
   if (psurf == NULL)
   {
      DC_UnlockDc(dc);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
   }

   if (psurf->hSecure == NULL)
   {
      SURFACE_UnlockSurface(psurf);
      DC_UnlockDc(dc);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
   }

   biBitCount = BitsPerFormat(psurf->SurfObj.iBitmapFormat);
   if (biBitCount <= 8 && StartIndex < (1 << biBitCount))
   {
      if (StartIndex + Entries > (1 << biBitCount))
         Entries = (1 << biBitCount) - StartIndex;

      PalGDI = PALETTE_LockPalette(psurf->hDIBPalette);
      if (PalGDI == NULL)
      {
          SURFACE_UnlockSurface(psurf);
          DC_UnlockDc(dc);
          SetLastWin32Error(ERROR_INVALID_HANDLE);
          return 0;
      }

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

   /* Mark the brushes invalid */
   dc->pdcattr->ulDirty_ |= DIRTY_FILL|DIRTY_LINE|DIRTY_BACKGROUND|DIRTY_TEXT;

   SURFACE_UnlockSurface(psurf);
   DC_UnlockDc(dc);

   return Entries;
}

UINT APIENTRY
IntGetDIBColorTable(HDC hDC, UINT StartIndex, UINT Entries, RGBQUAD *Colors)
{
   PDC dc;
   PSURFACE psurf;
   PPALGDI PalGDI;
   UINT Index;
   ULONG biBitCount;

   if (!(dc = DC_LockDc(hDC))) return 0;
   if (dc->dctype == DC_TYPE_INFO)
   {
      DC_UnlockDc(dc);
      return 0;
   }

   psurf = SURFACE_LockSurface(dc->rosdc.hBitmap);
   if (psurf == NULL)
   {
      DC_UnlockDc(dc);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
   }

   if (psurf->hSecure == NULL)
   {
      SURFACE_UnlockSurface(psurf);
      DC_UnlockDc(dc);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
   }

   biBitCount = BitsPerFormat(psurf->SurfObj.iBitmapFormat);
   if (biBitCount <= 8 &&
       StartIndex < (1 << biBitCount))
   {
      if (StartIndex + Entries > (1 << biBitCount))
         Entries = (1 << biBitCount) - StartIndex;

      PalGDI = PALETTE_LockPalette(psurf->hDIBPalette);
      if (PalGDI == NULL)
      {
          SURFACE_UnlockSurface(psurf);
          DC_UnlockDc(dc);
          SetLastWin32Error(ERROR_INVALID_HANDLE);
          return 0;
      }

      for (Index = StartIndex;
           Index < StartIndex + Entries && Index < PalGDI->NumColors;
           Index++)
      {
         Colors[Index - StartIndex].rgbRed = PalGDI->IndexedColors[Index].peRed;
         Colors[Index - StartIndex].rgbGreen = PalGDI->IndexedColors[Index].peGreen;
         Colors[Index - StartIndex].rgbBlue = PalGDI->IndexedColors[Index].peBlue;
         Colors[Index - StartIndex].rgbReserved = 0;
      }
      PALETTE_UnlockPalette(PalGDI);
   }
   else
      Entries = 0;

   SURFACE_UnlockSurface(psurf);
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
  SURFACE  *bitmap;
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
  if (!(bitmap = SURFACE_LockSurface(hBitmap)))
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
      SURFACE_UnlockSurface(bitmap);
      SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
      return 0;
  }

  SourceSurf = EngLockSurface((HSURF)SourceBitmap);
  if (NULL == SourceSurf)
  {
      EngDeleteSurface((HSURF)SourceBitmap);
      SURFACE_UnlockSurface(bitmap);
      SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
      return 0;
  }

  // Use hDIBPalette if it exists
  if (bitmap->hDIBPalette)
  {
    DDB_Palette = bitmap->hDIBPalette;
  }
  else
  {
    // Destination palette obtained from the hDC
    DDB_Palette = DC->ppdev->DevInfo.hpalDefault;
  }
  hDCPalette = PALETTE_LockPalette(DDB_Palette);
  if (NULL == hDCPalette)
    {
      EngUnlockSurface(SourceSurf);
      EngDeleteSurface((HSURF)SourceBitmap);
      SURFACE_UnlockSurface(bitmap);
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return 0;
    }
  DDB_Palette_Type = hDCPalette->Mode;
  PALETTE_UnlockPalette(hDCPalette);

  // Source palette obtained from the BITMAPINFO
  DIB_Palette = BuildDIBPalette ( (PBITMAPINFO)bmi, (PINT)&DIB_Palette_Type );
  if (NULL == DIB_Palette)
    {
      EngUnlockSurface(SourceSurf);
      EngDeleteSurface((HSURF)SourceBitmap);
      SURFACE_UnlockSurface(bitmap);
      SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
      return 0;
    }

  // Determine XLATEOBJ for color translation
  XlateObj = IntEngCreateXlate(DDB_Palette_Type, DIB_Palette_Type, DDB_Palette, DIB_Palette);
  if (NULL == XlateObj)
    {
      PALETTE_FreePaletteByHandle(DIB_Palette);
      EngUnlockSurface(SourceSurf);
      EngDeleteSurface((HSURF)SourceBitmap);
      SURFACE_UnlockSurface(bitmap);
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
    result = SourceSize.cy;
// or
//    result = abs(bmi->bmiHeader.biHeight) - StartScan;
  }

  // Clean up
  EngDeleteXlate(XlateObj);
  PALETTE_FreePaletteByHandle(DIB_Palette);
  EngUnlockSurface(SourceSurf);
  EngDeleteSurface((HSURF)SourceBitmap);

//  if (ColorUse == DIB_PAL_COLORS)
//    WinFree((LPSTR)lpRGB);

  SURFACE_UnlockSurface(bitmap);

  return result;
}

// FIXME by Removing NtGdiSetDIBits!!! 
// This is a victim of the Win32k Initialization BUG!!!!!
// Converts a DIB to a device-dependent bitmap
INT APIENTRY
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
  NTSTATUS Status = STATUS_SUCCESS;
  UINT cjBits;

  if (!Bits) return 0;

  _SEH2_TRY
  {  // FYI: We converted from CORE in gdi.
     ProbeForRead(bmi, sizeof(BITMAPINFO), 1); 
     cjBits = bmi->bmiHeader.biBitCount * bmi->bmiHeader.biPlanes * bmi->bmiHeader.biWidth;
     cjBits = ((cjBits + 31) & ~31 ) / 8;
     cjBits *= ScanLines;
     ProbeForRead(Bits, cjBits, 1);
  } 
  _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
  {
     Status = _SEH2_GetExceptionCode();
  }
  _SEH2_END

  if (!NT_SUCCESS(Status))
  {
     return 0;
  }

  Dc = DC_LockDc(hDC);
  if (NULL == Dc)
  {
     SetLastWin32Error(ERROR_INVALID_HANDLE);
     return 0;
  }
  if (Dc->dctype == DC_TYPE_INFO)
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
    HBITMAP hSourceBitmap = NULL;
    SURFOBJ *pDestSurf, *pSourceSurf = NULL;
    SURFACE *pSurf;
    RECTL rcDest;
    POINTL ptSource;
    INT DIBWidth;
    SIZEL SourceSize;
    XLATEOBJ *XlateObj = NULL;
    PPALGDI pDCPalette;
    HPALETTE DDBPalette, DIBPalette = NULL;
    ULONG DDBPaletteType, DIBPaletteType;

    if (!Bits) return 0;

    _SEH2_TRY
    {
        ProbeForRead(bmi, cjMaxInfo , 1); 
        ProbeForRead(Bits, cjMaxBits, 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
       Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END

    if (!NT_SUCCESS(Status))
    {
       return 0;
    }

    pDC = DC_LockDc(hDC);
    if (!pDC)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return 0;
    }
    if (pDC->dctype == DC_TYPE_INFO)
    {
        DC_UnlockDc(pDC);
        return 0;
    }

    /* Use destination palette obtained from the DC by default */
    DDBPalette = pDC->ppdev->DevInfo.hpalDefault;

    /* Try to use hDIBPalette if it exists */
    pSurf = SURFACE_LockSurface(pDC->rosdc.hBitmap);
    if (pSurf && pSurf->hDIBPalette)
    {
        DDBPalette = pSurf->hDIBPalette;
        SURFACE_UnlockSurface(pSurf);
    }

    pDestSurf = EngLockSurface((HSURF)pDC->rosdc.hBitmap);

    rcDest.left = XDest;
    rcDest.top = YDest;
    if (bTransformCoordinates)
    {
        CoordLPtoDP(pDC, (LPPOINT)&rcDest);
    }
    rcDest.left += pDC->ptlDCOrig.x;
    rcDest.top += pDC->ptlDCOrig.y;
    rcDest.right = rcDest.left + Width;
    rcDest.bottom = rcDest.top + Height;
    ptSource.x = XSrc;
    ptSource.y = YSrc;


    SourceSize.cx = bmi->bmiHeader.biWidth;
    SourceSize.cy = ScanLines; // this one --> abs(bmi->bmiHeader.biHeight) - StartScan
    DIBWidth = DIB_GetDIBWidthBytes(SourceSize.cx, bmi->bmiHeader.biBitCount);

    hSourceBitmap = EngCreateBitmap(SourceSize,
                                    DIBWidth,
                                    BitmapFormat(bmi->bmiHeader.biBitCount, bmi->bmiHeader.biCompression),
                                    bmi->bmiHeader.biHeight < 0 ? BMF_TOPDOWN : 0,
                                    (PVOID) Bits);
    if (!hSourceBitmap)
    {
        SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
        Status = STATUS_NO_MEMORY;
        goto Exit;
    }

    pSourceSurf = EngLockSurface((HSURF)hSourceBitmap);
    if (!pSourceSurf)
    {
       Status = STATUS_UNSUCCESSFUL;
       goto Exit;
    }

    /* Obtain destination palette */
    pDCPalette = PALETTE_LockPalette(DDBPalette);
    if (!pDCPalette)
    {
       SetLastWin32Error(ERROR_INVALID_HANDLE);
       Status = STATUS_UNSUCCESSFUL;
       goto Exit;
    }

    DDBPaletteType = pDCPalette->Mode;
    PALETTE_UnlockPalette(pDCPalette);

    DIBPalette = BuildDIBPalette(bmi, (PINT)&DIBPaletteType);
    if (!DIBPalette)
    {
       SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
       Status = STATUS_NO_MEMORY;
       goto Exit;
    }

    /* Determine XlateObj */
    XlateObj = IntEngCreateXlate(DDBPaletteType, DIBPaletteType, DDBPalette, DIBPalette);
    if (!XlateObj)
    {
       SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
       Status = STATUS_NO_MEMORY;
       goto Exit;
    }

    /* Copy the bits */
    Status = IntEngBitBlt(pDestSurf,
                          pSourceSurf,
                          NULL,
                          pDC->rosdc.CombinedClip,
                          XlateObj,
                          &rcDest,
                          &ptSource,
                          NULL,
                          NULL,
                          NULL,
                          ROP3_TO_ROP4(SRCCOPY));
Exit:
    if (NT_SUCCESS(Status))
    {
         /* FIXME: Should probably be only the number of lines actually copied */
        ret = ScanLines; // this one --> abs(Info->bmiHeader.biHeight) - StartScan;
    }

    if (pSourceSurf) EngUnlockSurface(pSourceSurf);
    if (hSourceBitmap) EngDeleteSurface((HSURF)hSourceBitmap);
    if (XlateObj) EngDeleteXlate(XlateObj);
    if (DIBPalette) PALETTE_FreePaletteByHandle(DIBPalette);
    EngUnlockSurface(pDestSurf);
    DC_UnlockDc(pDC);

    return ret;
}


/* Converts a device-dependent bitmap to a DIB */
INT APIENTRY
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
    PDC Dc;
    SURFACE *psurf = NULL;
    HBITMAP hDestBitmap = NULL;
    HPALETTE hSourcePalette = NULL;
    HPALETTE hDestPalette = NULL;
    PPALGDI SourcePalette = NULL;
    PPALGDI DestPalette = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Result = 0;
    BOOL bPaletteMatch = FALSE;
    PBYTE ChkBits = Bits;
    PVOID ColorPtr;
    RGBQUAD *rgbQuads;
    ULONG SourcePaletteType = 0;
    ULONG DestPaletteType;
    ULONG Index;

    DPRINT("Entered NtGdiGetDIBitsInternal()\n");

    if ( (Usage && Usage != DIB_PAL_COLORS) ||
         !Info ||
         !hBitmap )
       return 0;

    // if ScanLines == 0, no need to copy Bits.
    if (!ScanLines) ChkBits = NULL;

    _SEH2_TRY
    {
        ProbeForWrite(Info, Info->bmiHeader.biSize, 1); // Comp for Core.
        if (ChkBits) ProbeForWrite(ChkBits, MaxBits, 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
       Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END

    if (!NT_SUCCESS(Status))
    {
       return 0;
    }

    Dc = DC_LockDc(hDC);
    if (Dc == NULL) return 0;
    if (Dc->dctype == DC_TYPE_INFO)
    {
        DC_UnlockDc(Dc);
        return 0;
    }
    DC_UnlockDc(Dc);

    /* Get a pointer to the source bitmap object */
    psurf = SURFACE_LockSurface(hBitmap);
    if (psurf == NULL)
       return 0;

    hSourcePalette = psurf->hDIBPalette;
    if (!hSourcePalette)
    {
        hSourcePalette = pPrimarySurface->DevInfo.hpalDefault;
    }

    ColorPtr = ((PBYTE)Info + Info->bmiHeader.biSize);
    rgbQuads = (RGBQUAD *)ColorPtr;

    /* Copy palette information
     * Always create a palette for 15 & 16 bit. */
    if (Info->bmiHeader.biBitCount == BitsPerFormat(psurf->SurfObj.iBitmapFormat) &&
        Info->bmiHeader.biBitCount != 15 && Info->bmiHeader.biBitCount != 16)
    {
      hDestPalette = hSourcePalette;
      bPaletteMatch = TRUE;
    }
    else
      hDestPalette = BuildDIBPalette(Info, (PINT)&DestPaletteType); //hDestPalette = Dc->DevInfo->hpalDefault;

    SourcePalette = PALETTE_LockPalette(hSourcePalette);
    /* FIXME - SourcePalette can be NULL!!! Don't assert here! */
    ASSERT(SourcePalette);
    SourcePaletteType = SourcePalette->Mode;
    PALETTE_UnlockPalette(SourcePalette);

    if (bPaletteMatch)
    {
      DestPalette = PALETTE_LockPalette(hDestPalette);
      /* FIXME - DestPalette can be NULL!!!! Don't assert here!!! */
      DPRINT("DestPalette : %p\n", DestPalette);
      ASSERT(DestPalette);
      DestPaletteType = DestPalette->Mode;
    }
    else
    {
      DestPalette = SourcePalette;
    }

    /* Copy palette. */
    /* FIXME: This is largely incomplete. ATM no Core!*/
    switch(Info->bmiHeader.biBitCount)
    {
      case 1:
      case 4:
      case 8:
         Info->bmiHeader.biClrUsed = 0;
         if ( psurf->hSecure && 
              BitsPerFormat(psurf->SurfObj.iBitmapFormat) == Info->bmiHeader.biBitCount)
         {
            if (Usage == DIB_RGB_COLORS)
            {
                if (DestPalette->NumColors != 1 << Info->bmiHeader.biBitCount)
                   Info->bmiHeader.biClrUsed = DestPalette->NumColors;
                for (Index = 0;
                     Index < (1 << Info->bmiHeader.biBitCount) && Index < DestPalette->NumColors;
                     Index++)
                {
                   rgbQuads[Index].rgbRed   = DestPalette->IndexedColors[Index].peRed;
                   rgbQuads[Index].rgbGreen = DestPalette->IndexedColors[Index].peGreen;
                   rgbQuads[Index].rgbBlue  = DestPalette->IndexedColors[Index].peBlue;
                   rgbQuads[Index].rgbReserved = 0;
                }
            }
            else
            {
               PWORD Ptr = ColorPtr;
               for (Index = 0;
                    Index < (1 << Info->bmiHeader.biBitCount);
                    Index++)
               {
                  Ptr[Index] = (WORD)Index;
               }
            }
         }
         else
         {
            if (Usage == DIB_PAL_COLORS)
            {
               PWORD Ptr = ColorPtr;
               for (Index = 0;
                    Index < (1 << Info->bmiHeader.biBitCount);
                    Index++)
               {
                  Ptr[Index] = (WORD)Index;
               }                   
            }
            else if (Info->bmiHeader.biBitCount > 1  && bPaletteMatch)
            {
               for (Index = 0;
                    Index < (1 << Info->bmiHeader.biBitCount) && Index < DestPalette->NumColors;
                    Index++)
               {
                  Info->bmiColors[Index].rgbRed   = DestPalette->IndexedColors[Index].peRed;
                  Info->bmiColors[Index].rgbGreen = DestPalette->IndexedColors[Index].peGreen;
                  Info->bmiColors[Index].rgbBlue  = DestPalette->IndexedColors[Index].peBlue;
                  Info->bmiColors[Index].rgbReserved = 0;
               }
            }
            else
            {
               switch(Info->bmiHeader.biBitCount)
               {
                  case 1:
                     rgbQuads[0].rgbRed = rgbQuads[0].rgbGreen = rgbQuads[0].rgbBlue = 0;
                     rgbQuads[0].rgbReserved = 0;
                     rgbQuads[1].rgbRed = rgbQuads[1].rgbGreen = rgbQuads[1].rgbBlue = 0xff;
                     rgbQuads[1].rgbReserved = 0;
                     break;
                  case 4:
                     RtlCopyMemory(ColorPtr, EGAColorsQuads, sizeof(EGAColorsQuads));
                     break;
                  case 8:
                  {
                     INT r, g, b;
                     RGBQUAD *color;

                     RtlCopyMemory(rgbQuads, DefLogPaletteQuads, 10 * sizeof(RGBQUAD));
                     RtlCopyMemory(rgbQuads + 246, DefLogPaletteQuads + 10, 10 * sizeof(RGBQUAD));
                     color = rgbQuads + 10;
                     for(r = 0; r <= 5; r++) /* FIXME */
                        for(g = 0; g <= 5; g++)
                           for(b = 0; b <= 5; b++)
                           {
                              color->rgbRed =   (r * 0xff) / 5;
                              color->rgbGreen = (g * 0xff) / 5;
                              color->rgbBlue =  (b * 0xff) / 5;
                              color->rgbReserved = 0;
                              color++;
                           }
                  }
                  break;
               }
            }
         }

      case 15:
         if (Info->bmiHeader.biCompression == BI_BITFIELDS)
         {
            ((PDWORD)Info->bmiColors)[0] = 0x7c00;
            ((PDWORD)Info->bmiColors)[1] = 0x03e0;
            ((PDWORD)Info->bmiColors)[2] = 0x001f;
         }
         break;

      case 16:
         if (Info->bmiHeader.biCompression == BI_BITFIELDS)
         {
            ((PDWORD)Info->bmiColors)[0] = 0xf800;
            ((PDWORD)Info->bmiColors)[1] = 0x07e0;
            ((PDWORD)Info->bmiColors)[2] = 0x001f;
         }
         break;

      case 24:
      case 32:
         if (Info->bmiHeader.biCompression == BI_BITFIELDS)
         {
            ((PDWORD)Info->bmiColors)[0] = 0xff0000;
            ((PDWORD)Info->bmiColors)[1] = 0x00ff00;
            ((PDWORD)Info->bmiColors)[2] = 0x0000ff;
         }
         break;
    }

    if (bPaletteMatch)
      PALETTE_UnlockPalette(DestPalette);

    /* fill out the BITMAPINFO struct */
    if (!ChkBits)
    {  // Core or not to Core? We have converted from Core in Gdi~ so?
       if (Info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
       {
          BITMAPCOREHEADER* coreheader = (BITMAPCOREHEADER*) Info;
          coreheader->bcWidth = psurf->SurfObj.sizlBitmap.cx;
          coreheader->bcPlanes = 1;
          coreheader->bcBitCount = BitsPerFormat(psurf->SurfObj.iBitmapFormat);
          coreheader->bcHeight = psurf->SurfObj.sizlBitmap.cy;
          if (psurf->SurfObj.lDelta > 0)
             coreheader->bcHeight = -coreheader->bcHeight;
       }

       if (Info->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER))
       {
          Info->bmiHeader.biWidth = psurf->SurfObj.sizlBitmap.cx;
          Info->bmiHeader.biHeight = psurf->SurfObj.sizlBitmap.cy;
          Info->bmiHeader.biPlanes = 1;
          Info->bmiHeader.biBitCount = BitsPerFormat(psurf->SurfObj.iBitmapFormat);
          switch (psurf->SurfObj.iBitmapFormat)
          {
              /* FIXME: What about BI_BITFIELDS? */
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
          /* Image size has to be calculated */
          Info->bmiHeader.biSizeImage = DIB_GetDIBWidthBytes(Info->bmiHeader.biWidth,
                                        Info->bmiHeader.biBitCount) * Info->bmiHeader.biHeight;
          Info->bmiHeader.biXPelsPerMeter = 0; /* FIXME */
          Info->bmiHeader.biYPelsPerMeter = 0; /* FIXME */
          Info->bmiHeader.biClrUsed = 0;
          Info->bmiHeader.biClrImportant = 1 << Info->bmiHeader.biBitCount; /* FIXME */
          /* Report negtive height for top-down bitmaps. */
          if (psurf->SurfObj.lDelta > 0)
             Info->bmiHeader.biHeight = -Info->bmiHeader.biHeight;
       }
       Result = psurf->SurfObj.sizlBitmap.cy;
    }
    else
    {
       SIZEL DestSize;
       POINTL SourcePoint;

//
// If we have a good dib pointer, why not just copy bits from there w/o XLATE'ing them.
//
       /* Create the destination bitmap too for the copy operation */
       if (StartScan > psurf->SurfObj.sizlBitmap.cy)
       {
          goto cleanup;
       }
       else
       {
          ScanLines = min(ScanLines, psurf->SurfObj.sizlBitmap.cy - StartScan);
          DestSize.cx = psurf->SurfObj.sizlBitmap.cx;
          DestSize.cy = ScanLines;

          hDestBitmap = NULL;

          if (Info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
          {
             BITMAPCOREHEADER* coreheader = (BITMAPCOREHEADER*) Info;
             hDestBitmap = EngCreateBitmap(DestSize,
                                           DIB_GetDIBWidthBytes(DestSize.cx, coreheader->bcBitCount),
                                           BitmapFormat(coreheader->bcBitCount, BI_RGB),
                                           0 < coreheader->bcHeight ? 0 : BMF_TOPDOWN,
                                           Bits);
          }

          if (Info->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER))
          {
             Info->bmiHeader.biSizeImage = DIB_GetDIBWidthBytes(DestSize.cx,
                                           Info->bmiHeader.biBitCount) * DestSize.cy;

             hDestBitmap = EngCreateBitmap(DestSize,
                                           DIB_GetDIBWidthBytes(DestSize.cx, Info->bmiHeader.biBitCount),
                                           BitmapFormat(Info->bmiHeader.biBitCount, Info->bmiHeader.biCompression),
                                           0 < Info->bmiHeader.biHeight ? 0 : BMF_TOPDOWN,
                                           Bits);
          }

          if (hDestBitmap == NULL)
             goto cleanup;
       }

       if (NT_SUCCESS(Status))
       {
          XLATEOBJ *XlateObj;
          SURFOBJ *DestSurfObj;
          RECTL DestRect;

          XlateObj = IntEngCreateXlate(DestPaletteType,
                                       SourcePaletteType,
                                       hDestPalette,
                                       hSourcePalette);

          SourcePoint.x = 0;
          SourcePoint.y = psurf->SurfObj.sizlBitmap.cy - (StartScan + ScanLines);

          /* Determine destination rectangle */
          DestRect.top = 0;
          DestRect.left = 0;
          DestRect.right = DestSize.cx;
          DestRect.bottom = DestSize.cy;

          DestSurfObj = EngLockSurface((HSURF)hDestBitmap);

          if (EngCopyBits( DestSurfObj,
                          &psurf->SurfObj,
                           NULL,
                           XlateObj,
                          &DestRect,
                          &SourcePoint))
          {
             DPRINT("GetDIBits %d \n",abs(Info->bmiHeader.biHeight) - StartScan);
             Result = ScanLines;
          }

          EngDeleteXlate(XlateObj);
          EngUnlockSurface(DestSurfObj);
       }    
    }
cleanup:
    if (hDestBitmap != NULL)
        EngDeleteSurface((HSURF)hDestBitmap);

    if (hDestPalette != NULL && bPaletteMatch == FALSE)
        PALETTE_FreePaletteByHandle(hDestPalette);

    SURFACE_UnlockSurface(psurf);

    DPRINT("leaving NtGdiGetDIBitsInternal\n");

    return Result;
}

INT
APIENTRY
NtGdiStretchDIBitsInternal(
    HDC  hDC,
    INT  XDest,
    INT  YDest,
    INT  DestWidth,
    INT  DestHeight,
    INT  XSrc,
    INT  YSrc,
    INT  SrcWidth,
    INT  SrcHeight,
    LPBYTE Bits,
    LPBITMAPINFO BitsInfo,
    DWORD  Usage,
    DWORD  ROP,
    UINT cjMaxInfo,
    UINT cjMaxBits,
    HANDLE hcmXform)
{
   HBITMAP hBitmap, hOldBitmap = NULL;
   HDC hdcMem;
   HPALETTE hPal = NULL;
   PDC pDC;
   BOOL Hit = FALSE;

   if (!Bits || !BitsInfo)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
   }

   _SEH2_TRY
   {
      ProbeForRead(BitsInfo, cjMaxInfo, 1);
      ProbeForRead(Bits, cjMaxBits, 1);
   }
   _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
   {
      Hit = TRUE;
   }
   _SEH2_END

   if (Hit)
   {
      DPRINT1("NtGdiStretchDIBitsInternal fail to read BitMapInfo: %x or Bits: %x\n",BitsInfo,Bits);
      return 0;
   }

   hdcMem = NtGdiCreateCompatibleDC(hDC);
   if (hdcMem == NULL)
   {
       DPRINT1("NtGdiCreateCompatibleDC fail create hdc\n");
       return 0;
   }

   hBitmap = NtGdiCreateCompatibleBitmap( hDC,
                                      abs(BitsInfo->bmiHeader.biWidth),
                                      abs(BitsInfo->bmiHeader.biHeight));
   if (hBitmap == NULL)
   {
       DPRINT1("NtGdiCreateCompatibleBitmap fail create bitmap\n");
       DPRINT1("hDC : 0x%08x \n", hDC);
       DPRINT1("BitsInfo->bmiHeader.biWidth : 0x%08x \n", BitsInfo->bmiHeader.biWidth);
       DPRINT1("BitsInfo->bmiHeader.biHeight : 0x%08x \n", BitsInfo->bmiHeader.biHeight);
       return 0;
   }

   /* Select the bitmap into hdcMem, and save a handle to the old bitmap */
   hOldBitmap = NtGdiSelectBitmap(hdcMem, hBitmap);

   if(Usage == DIB_PAL_COLORS)
   {
      hPal = NtGdiGetDCObject(hDC, GDI_OBJECT_TYPE_PALETTE);
      hPal = GdiSelectPalette(hdcMem, hPal, FALSE);
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

   pDC = DC_LockDc(hdcMem);
   if (pDC != NULL) 
   {
        /* Note BitsInfo->bmiHeader.biHeight is the number of scanline, 
         * if it negitve we getting to many scanline for scanline is UINT not
         * a INT, so we need make the negtive value to positve and that make the
         * count correct for negtive bitmap, TODO : we need testcase for this api */
         IntSetDIBits(pDC, hBitmap, 0, abs(BitsInfo->bmiHeader.biHeight), Bits,
                  BitsInfo, Usage);

        DC_UnlockDc(pDC);
   }


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

    /* cleanup */
   if(hPal)
      GdiSelectPalette(hdcMem, hPal, FALSE);

   if (hOldBitmap)
      NtGdiSelectBitmap(hdcMem, hOldBitmap);

   NtGdiDeleteObjectApp(hdcMem);

   GreDeleteObject(hBitmap);

   return SrcHeight;
}


HBITMAP
FASTCALL
IntCreateDIBitmap(PDC Dc,
                  INT width,
                  INT height,
                  UINT bpp,
                  DWORD init,
                  LPBYTE bits,
                  PBITMAPINFO data,
                  DWORD coloruse)
{
  HBITMAP handle;
  BOOL fColor;

  // Check if we should create a monochrome or color bitmap. We create a monochrome bitmap only if it has exactly 2
  // colors, which are black followed by white, nothing else. In all other cases, we create a color bitmap.

  if (bpp != 1) fColor = TRUE;
  else if ((coloruse != DIB_RGB_COLORS) || (init != CBM_INIT) || !data) fColor = FALSE;
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
    handle = IntGdiCreateBitmap(width,
                                height,
                                1,
                                1,
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
HBITMAP
APIENTRY
NtGdiCreateDIBitmapInternal(IN HDC hDc,
                            IN INT cx,
                            IN INT cy,
                            IN DWORD fInit,
                            IN OPTIONAL LPBYTE pjInit,
                            IN OPTIONAL LPBITMAPINFO pbmi,
                            IN DWORD iUsage,
                            IN UINT cjMaxInitInfo,
                            IN UINT cjMaxBits,
                            IN FLONG fl,
                            IN HANDLE hcmXform)
{
  PDC Dc;
  HBITMAP Bmp;
  UINT bpp;

  if (!hDc) // CreateBitmap
  {  // Should use System Bitmap DC hSystemBM, with CreateCompatibleDC for this.
     hDc = IntGdiCreateDC(NULL, NULL, NULL, NULL,FALSE);
     if (!hDc)
     {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return NULL;
     }

     Dc = DC_LockDc(hDc);
     if (!Dc)
     {
        NtGdiDeleteObjectApp(hDc);
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return NULL;
     }
     bpp = 1;
     Bmp = IntCreateDIBitmap(Dc, cx, cy, bpp, fInit, pjInit, pbmi, iUsage);

     DC_UnlockDc(Dc);
     NtGdiDeleteObjectApp(hDc);
  }
  else // CreateCompatibleBitmap
  {
     Dc = DC_LockDc(hDc);
     if (!Dc)
     {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return NULL;
     }
    /* pbmi == null
       First create an un-initialised bitmap.  The depth of the bitmap
       should match that of the hdc and not that supplied in bmih.
     */
     if (pbmi)
        bpp = pbmi->bmiHeader.biBitCount;
     else
     {
        if (Dc->dctype != DC_TYPE_MEMORY )
           bpp = IntGdiGetDeviceCaps(Dc, BITSPIXEL);
        else
        {
           DIBSECTION dibs;
           INT Count;           
           SURFACE *psurf = SURFACE_LockSurface(Dc->rosdc.hBitmap);
           Count = BITMAP_GetObject(psurf, sizeof(dibs), &dibs);
           if (!Count)
              bpp = 1;
           else
           {
              if (Count == sizeof(BITMAP))
              /* A device-dependent bitmap is selected in the DC */
                 bpp = dibs.dsBm.bmBitsPixel;
              else
              /* A DIB section is selected in the DC */
                 bpp = dibs.dsBmih.biBitCount;
           }
           SURFACE_UnlockSurface(psurf);           
        }
     }
     Bmp = IntCreateDIBitmap(Dc, cx, cy, bpp, fInit, pjInit, pbmi, iUsage);
     DC_UnlockDc(Dc);
  }
  return Bmp;
}


HBITMAP APIENTRY NtGdiCreateDIBSection(HDC hDC,
                              IN OPTIONAL HANDLE hSection,
                              IN DWORD dwOffset,
                              IN LPBITMAPINFO bmi,
                              DWORD Usage,
                              IN UINT cjHeader,
                              IN FLONG fl,
                              IN ULONG_PTR dwColorSpace,
                              PVOID *Bits)
{
  HBITMAP hbitmap = 0;
  DC *dc;
  BOOL bDesktopDC = FALSE;

  if (!bmi) return hbitmap; // Make sure.

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

HBITMAP APIENTRY
DIB_CreateDIBSection(
  PDC dc,
  BITMAPINFO *bmi,
  UINT usage,
  LPVOID *bits,
  HANDLE section,
  DWORD offset,
  DWORD ovr_pitch)
{
  HBITMAP res = 0;
  SURFACE *bmp = NULL;
  void *mapBits = NULL;
  PDC_ATTR pdcattr;

  // Fill BITMAP32 structure with DIB data
  BITMAPINFOHEADER *bi = &bmi->bmiHeader;
  INT effHeight;
  ULONG totalSize;
  BITMAP bm;
  SIZEL Size;
  RGBQUAD *lpRGB;
  HANDLE hSecure;
  DWORD dsBitfields[3] = {0};

  DPRINT("format (%ld,%ld), planes %d, bpp %d, size %ld, colors %ld (%s)\n",
	bi->biWidth, bi->biHeight, bi->biPlanes, bi->biBitCount,
	bi->biSizeImage, bi->biClrUsed, usage == DIB_PAL_COLORS? "PAL" : "RGB");

  /* CreateDIBSection should fail for compressed formats */
  if (bi->biCompression == BI_RLE4 || bi->biCompression == BI_RLE8)
  {
    return (HBITMAP)NULL;
  }

  pdcattr = dc->pdcattr;

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
     SYSTEM_BASIC_INFORMATION Sbi;
     NTSTATUS Status;
     DWORD mapOffset;
     LARGE_INTEGER SectionOffset;
     SIZE_T mapSize;

     Status = ZwQuerySystemInformation ( SystemBasicInformation,
                                         &Sbi,
                                         sizeof Sbi,
                                         0);
     if (!NT_SUCCESS(Status))
     {
        return NULL;
     }

     mapOffset = offset - (offset % Sbi.AllocationGranularity);
     mapSize = bi->biSizeImage + (offset - mapOffset);

     SectionOffset.LowPart  = mapOffset;
     SectionOffset.HighPart = 0;

     Status = ZwMapViewOfSection ( section,
                                   NtCurrentProcess(),
                                   &mapBits,
                                   0,
                                   0,
                                   &SectionOffset,
                                   &mapSize,
                                   ViewShare,
                                   0,
                                   PAGE_READWRITE);
     if (!NT_SUCCESS(Status))
     {
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return NULL;
     }

     if (mapBits) bm.bmBits = (char *)mapBits + (offset - mapOffset);
  }
  else if (ovr_pitch && offset)
    bm.bmBits = (LPVOID) offset;
  else
  {
     offset = 0;
     bm.bmBits = EngAllocUserMem( totalSize, 0 );
  }

//  hSecure = MmSecureVirtualMemory(bm.bmBits, totalSize, PAGE_READWRITE);
  hSecure = (HANDLE)0x1; // HACK OF UNIMPLEMENTED KERNEL STUFF !!!!

  if(usage == DIB_PAL_COLORS)
    lpRGB = DIB_MapPaletteColors(dc, bmi);
  else
    lpRGB = bmi->bmiColors;

    /* Set dsBitfields values */
    if ( usage == DIB_PAL_COLORS || bi->biBitCount <= 8)
    {
      dsBitfields[0] = dsBitfields[1] = dsBitfields[2] = 0;
    }
    else if (bi->biCompression == BI_RGB)
    {
        switch(bi->biBitCount)
        {
          case 15:
          case 16:
            dsBitfields[0] = (bi->biCompression == BI_BITFIELDS) ? *(DWORD *)lpRGB       : 0x7c00;
            dsBitfields[1] = (bi->biCompression == BI_BITFIELDS) ? *((DWORD *)lpRGB + 1) : 0x03e0;
            dsBitfields[2] = (bi->biCompression == BI_BITFIELDS) ? *((DWORD *)lpRGB + 2) : 0x001f;
            break;

          case 24:
          case 32:
            dsBitfields[0] = (bi->biCompression == BI_BITFIELDS) ? *(DWORD *)lpRGB       : 0xff0000;
            dsBitfields[1] = (bi->biCompression == BI_BITFIELDS) ? *((DWORD *)lpRGB + 1) : 0x00ff00;
            dsBitfields[2] = (bi->biCompression == BI_BITFIELDS) ? *((DWORD *)lpRGB + 2) : 0x0000ff;
            break;
        }
    }
    else
    {
        dsBitfields[0] = ((DWORD*)bmi->bmiColors)[0];
        dsBitfields[1] = ((DWORD*)bmi->bmiColors)[1];
        dsBitfields[2] = ((DWORD*)bmi->bmiColors)[2];
    }

    // Create Device Dependent Bitmap and add DIB pointer
    Size.cx = bm.bmWidth;
    Size.cy = abs(bm.bmHeight);
    res = IntCreateBitmap( Size,
                           bm.bmWidthBytes,
                           BitmapFormat(bi->biBitCount * bi->biPlanes, bi->biCompression),
                           BMF_DONTCACHE | BMF_USERMEM | BMF_NOZEROINIT |
                           (bi->biHeight < 0 ? BMF_TOPDOWN : 0),
                           bm.bmBits);
    if ( !res )
    {
        if (lpRGB != bmi->bmiColors)
        {
            ExFreePoolWithTag(lpRGB, TAG_COLORMAP);
        }
        SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
	return NULL;
    }
    bmp = SURFACE_LockSurface(res);
    if (NULL == bmp)
    {
        if (lpRGB != bmi->bmiColors)
        {
            ExFreePoolWithTag(lpRGB, TAG_COLORMAP);
        }
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        GreDeleteObject(bmp);
        return NULL;
    }

    /* WINE NOTE: WINE makes use of a colormap, which is a color translation
                  table between the DIB and the X physical device. Obviously,
                  this is left out of the ReactOS implementation. Instead,
                  we call NtGdiSetDIBColorTable. */
    bi->biClrUsed = 0;
    /* set number of entries in bmi.bmiColors table */
    if(bi->biBitCount == 1) { bi->biClrUsed = 2; } else
    if(bi->biBitCount == 4) { bi->biClrUsed = 16; } else
    if(bi->biBitCount == 8) { bi->biClrUsed = 256; }

    bmp->hDIBSection = section;
    bmp->hSecure = hSecure;
    bmp->dwOffset = offset;
    bmp->flFlags = BITMAPOBJ_IS_APIBITMAP;
    bmp->dsBitfields[0] = dsBitfields[0];
    bmp->dsBitfields[1] = dsBitfields[1];
    bmp->dsBitfields[2] = dsBitfields[2];
    bmp->biClrUsed = bi->biClrUsed;
    bmp->biClrImportant = bi->biClrImportant;

    if (bi->biClrUsed != 0)
      bmp->hDIBPalette = PALETTE_AllocPaletteIndexedRGB(bi->biClrUsed, lpRGB);
    else
      bmp->hDIBPalette = PALETTE_AllocPalette(PAL_BITFIELDS, 0, NULL,
                                              dsBitfields[0],
                                              dsBitfields[1],
                                              dsBitfields[2]);

  // Clean up in case of errors
  if (!res || !bmp || !bm.bmBits)
  {
    DPRINT("got an error res=%08x, bmp=%p, bm.bmBits=%p\n", res, bmp, bm.bmBits);
      if (bm.bmBits)
      {
         // MmUnsecureVirtualMemory(hSecure); // FIXME: Implement this!
         if (section)
         {
            ZwUnmapViewOfSection(NtCurrentProcess(), mapBits);
            bm.bmBits = NULL;
         }
         else
            if (!offset)
               EngFreeUserMem(bm.bmBits), bm.bmBits = NULL;
    }

    if (bmp) { bmp = NULL; }
    if (res) { SURFACE_FreeSurfaceByHandle(res); res = 0; }
  }

  if (lpRGB != bmi->bmiColors)
  {
      ExFreePoolWithTag(lpRGB, TAG_COLORMAP);
  }

  if (bmp)
  {
      SURFACE_UnlockSurface(bmp);
  }

  // Return BITMAP handle and storage location
  if (NULL != bm.bmBits && NULL != bits)
  {
      *bits = bm.bmBits;
  }

//  if (res) pdcattr->ulDirty_ |= DC_DIBSECTION;

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

INT APIENTRY DIB_GetDIBImageBytes (INT  width, INT height, INT depth)
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

RGBQUAD * FASTCALL
DIB_MapPaletteColors(PDC dc, CONST BITMAPINFO* lpbmi)
{
  RGBQUAD *lpRGB;
  ULONG nNumColors,i;
  USHORT *lpIndex;
  PPALGDI palGDI;

  palGDI = PALETTE_LockPalette(dc->dclevel.hpal);

  if (NULL == palGDI)
    {
      return NULL;
    }

  if (palGDI->Mode != PAL_INDEXED)
    {
      PALETTE_UnlockPalette(palGDI);
      return NULL;
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
      lpRGB[i].rgbReserved = 0;
      lpIndex++;
    }
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
  else if(bmi->bmiHeader.biCompression == BI_BITFIELDS)
    {
      *paletteType = PAL_BITFIELDS;
      RedMask = ((ULONG *)bmi->bmiColors)[0];
      GreenMask = ((ULONG *)bmi->bmiColors)[1];
      BlueMask = ((ULONG *)bmi->bmiColors)[2];
    }
  else if(bits < 24)
    {
      *paletteType = PAL_BITFIELDS;
      RedMask = 0x7c00;
      GreenMask = 0x03e0;
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
