/*
 * $Id: dib.c,v 1.47 2004/05/15 08:52:25 navaraf Exp $
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

UINT STDCALL
NtGdiSetDIBColorTable(HDC hDC, UINT StartIndex, UINT Entries, CONST RGBQUAD *Colors)
{
   PDC dc;
   PBITMAPOBJ BitmapObj;

   if (!(dc = DC_LockDc(hDC))) return 0;

   BitmapObj = BITMAPOBJ_LockBitmap(dc->w.hBitmap);
   if (BitmapObj == NULL)
   {
      DC_UnlockDc(hDC);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
   }

   if (BitmapObj->dib == NULL)
   {
      BITMAPOBJ_UnlockBitmap(dc->w.hBitmap);
      DC_UnlockDc(hDC);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
   }

   if (BitmapObj->dib->dsBmih.biBitCount <= 8 &&
       StartIndex > (1 << BitmapObj->dib->dsBmih.biBitCount))
   {
      if (StartIndex + Entries > (1 << BitmapObj->dib->dsBmih.biBitCount))
         Entries = (1 << BitmapObj->dib->dsBmih.biBitCount) - StartIndex;

      MmCopyFromCaller(BitmapObj->ColorMap + StartIndex, Colors, Entries * sizeof(RGBQUAD));

      /* Rebuild the palette. */
      NtGdiDeleteObject(dc->w.hPalette);
      dc->w.hPalette = PALETTE_AllocPalette(PAL_INDEXED,
         1 << BitmapObj->dib->dsBmih.biBitCount,
         (PULONG)BitmapObj->ColorMap, 0, 0, 0);
   }
   else
      Entries = 0;

   BITMAPOBJ_UnlockBitmap(dc->w.hBitmap);
   DC_UnlockDc(hDC);

   return Entries;
}

UINT STDCALL
NtGdiGetDIBColorTable(HDC hDC, UINT StartIndex, UINT Entries, RGBQUAD *Colors)
{
   PDC dc;
   PBITMAPOBJ BitmapObj;

   if (!(dc = DC_LockDc(hDC))) return 0;

   BitmapObj = BITMAPOBJ_LockBitmap(dc->w.hBitmap);
   if (BitmapObj == NULL)
   {
      DC_UnlockDc(hDC);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
   }

   if (BitmapObj->dib == NULL)
   {
      BITMAPOBJ_UnlockBitmap(dc->w.hBitmap);
      DC_UnlockDc(hDC);
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
   }

   if (BitmapObj->dib->dsBmih.biBitCount <= 8 &&
       StartIndex > (1 << BitmapObj->dib->dsBmih.biBitCount))
   {
      if (StartIndex + Entries > (1 << BitmapObj->dib->dsBmih.biBitCount))
         Entries = (1 << BitmapObj->dib->dsBmih.biBitCount) - StartIndex;

      MmCopyToCaller(Colors, BitmapObj->ColorMap + StartIndex, Entries * sizeof(RGBQUAD));
   }
   else
      Entries = 0;

   BITMAPOBJ_UnlockBitmap(dc->w.hBitmap);
   DC_UnlockDc(hDC);

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
  HBITMAP     SourceBitmap, DestBitmap;
  INT         result = 0;
  BOOL        copyBitsResult;
  SURFOBJ    *DestSurf, *SourceSurf;
  PSURFGDI    DestGDI;
  SIZEL       SourceSize;
  POINTL      ZeroPoint;
  RECTL       DestRect;
  XLATEOBJ   *XlateObj;
  PPALGDI     hDCPalette;
  //RGBQUAD  *lpRGB;
  HPALETTE    DDB_Palette, DIB_Palette;
  ULONG       DDB_Palette_Type, DIB_Palette_Type;
  const BYTE *vBits = (const BYTE*)Bits;
  INT         scanDirection = 1, DIBWidth;

  // Check parameters
  if (!(bitmap = BITMAPOBJ_LockBitmap(hBitmap)))
  {
    return 0;
  }

  // Get RGB values
  //if (ColorUse == DIB_PAL_COLORS)
  //  lpRGB = DIB_MapPaletteColors(hDC, bmi);
  //else
  //  lpRGB = &bmi->bmiColors[0];

  // Create a temporary surface for the destination bitmap
  DestBitmap = BitmapToSurf(bitmap, DC->GDIDevice);

  DestSurf   = (SURFOBJ*) AccessUserObject( (ULONG)DestBitmap );
  DestGDI    = (PSURFGDI) AccessInternalObject( (ULONG)DestBitmap );

  // Create source surface
  SourceSize.cx = bmi->bmiHeader.biWidth;
  SourceSize.cy = abs(bmi->bmiHeader.biHeight);

  // Determine width of DIB
  DIBWidth = DIB_GetDIBWidthBytes(SourceSize.cx, bmi->bmiHeader.biBitCount);

  // Determine DIB Vertical Orientation
  if(bmi->bmiHeader.biHeight > 0)
  {
    scanDirection = -1;
    vBits += DIBWidth * bmi->bmiHeader.biHeight - DIBWidth;
  }

  SourceBitmap = EngCreateBitmap(SourceSize,
                                 DIBWidth * scanDirection,
                                 BitmapFormat(bmi->bmiHeader.biBitCount, bmi->bmiHeader.biCompression),
                                 0,
                                 (PVOID)vBits );
  SourceSurf = (SURFOBJ*)AccessUserObject((ULONG)SourceBitmap);

  // Destination palette obtained from the hDC
  hDCPalette = PALETTE_LockPalette(DC->DevInfo->hpalDefault);
  if (NULL == hDCPalette)
    {
      EngDeleteSurface((HSURF)SourceBitmap);
      EngDeleteSurface((HSURF)DestBitmap);
      BITMAPOBJ_UnlockBitmap(hBitmap);
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return 0;
    }
  DDB_Palette_Type = hDCPalette->Mode;
  DDB_Palette = DC->DevInfo->hpalDefault;
  PALETTE_UnlockPalette(DC->DevInfo->hpalDefault);

  // Source palette obtained from the BITMAPINFO
  DIB_Palette = BuildDIBPalette ( (PBITMAPINFO)bmi, (PINT)&DIB_Palette_Type );
  if (NULL == DIB_Palette)
    {
      EngDeleteSurface((HSURF)SourceBitmap);
      EngDeleteSurface((HSURF)DestBitmap);
      BITMAPOBJ_UnlockBitmap(hBitmap);
      SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
      return 0;
    }

  // Determine XLATEOBJ for color translation
  XlateObj = IntEngCreateXlate(DDB_Palette_Type, DIB_Palette_Type, DDB_Palette, DIB_Palette);
  if (NULL == XlateObj)
    {
      PALETTE_FreePalette(DIB_Palette);
      EngDeleteSurface((HSURF)SourceBitmap);
      EngDeleteSurface((HSURF)DestBitmap);
      BITMAPOBJ_UnlockBitmap(hBitmap);
      SetLastWin32Error(ERROR_NO_SYSTEM_RESOURCES);
      return 0;
    }

  // Zero point
  ZeroPoint.x = 0;
  ZeroPoint.y = 0;

  // Determine destination rectangle
  DestRect.top	= 0;
  DestRect.left	= 0;
  DestRect.right	= SourceSize.cx;
  DestRect.bottom	= SourceSize.cy;

  copyBitsResult = EngCopyBits(DestSurf, SourceSurf, NULL, XlateObj, &DestRect, &ZeroPoint);

  // If it succeeded, return number of scanlines copies
  if(copyBitsResult == TRUE)
  {
    result = SourceSize.cy - 1;
  }

  // Clean up
  EngDeleteXlate(XlateObj);
  PALETTE_FreePalette(DIB_Palette);
  EngDeleteSurface((HSURF)SourceBitmap);
  EngDeleteSurface((HSURF)DestBitmap);

//  if (ColorUse == DIB_PAL_COLORS)
//    WinFree((LPSTR)lpRGB);

  BITMAPOBJ_UnlockBitmap(hBitmap);

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

  Ret = IntSetDIBits(Dc, hBitmap, StartScan, ScanLines, Bits, bmi, ColorUse);

  DC_UnlockDc(hDC);

  return Ret;
}

INT STDCALL
NtGdiSetDIBitsToDevice(
	HDC  hDC,
	INT  XDest,
	INT  YDest,
	DWORD  Width,
	DWORD  Height,
	INT  XSrc,
	INT  YSrc,
	UINT  StartScan,
	UINT  ScanLines,
	CONST VOID  *Bits,
	CONST BITMAPINFO  *bmi,
	UINT  ColorUse)
{
  UNIMPLEMENTED;
  return 0;
}

// Converts a device-dependent bitmap to a DIB
#if 0
INT STDCALL NtGdiGetDIBits(HDC  hDC,
                   HBITMAP hBitmap,
                   UINT  StartScan,
                   UINT  ScanLines,
                   LPVOID  Bits,
                   LPBITMAPINFO UnsafeInfo,
                   UINT  Usage)
#else
INT STDCALL NtGdiGetDIBits(
    HDC hdc,         /* [in]  Handle to device context */
    HBITMAP hbitmap, /* [in]  Handle to bitmap */
    UINT startscan,  /* [in]  First scan line to set in dest bitmap */
    UINT lines,      /* [in]  Number of scan lines to copy */
    LPVOID bits,       /* [out] Address of array for bitmap bits */
    BITMAPINFO * info, /* [out] Address of structure with bitmap data */
    UINT coloruse)   /* [in]  RGB or palette index */
#endif
{
#if 0
  BITMAPINFO Info;
  BITMAPCOREHEADER *Core;
  PBITMAPOBJ BitmapObj;
  INT Result;
  NTSTATUS Status;
  PDC DCObj;
  PPALGDI PalGdi;
  struct
    {
    BITMAPINFO Info;
    DWORD BitFields[3];
    } InfoWithBitFields;
  DWORD *BitField;
  DWORD InfoSize;

  BitmapObj = BITMAPOBJ_LockBitmap(hBitmap);
  if (NULL == BitmapObj)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return 0;
    }

  RtlZeroMemory(&Info, sizeof(BITMAPINFO));
  Status = MmCopyFromCaller(&(Info.bmiHeader.biSize),
                            &(UnsafeInfo->bmiHeader.biSize),
                            sizeof(DWORD));
  if (! NT_SUCCESS(Status))
    {
    SetLastNtError(Status);
    BITMAPOBJ_UnlockBitmap(hBitmap);
    return 0;
    }

  /* If the bits are not requested, UnsafeInfo can point to either a
     BITMAPINFOHEADER or a BITMAPCOREHEADER */
  if (sizeof(BITMAPINFOHEADER) != Info.bmiHeader.biSize &&
      (sizeof(BITMAPCOREHEADER) != Info.bmiHeader.biSize ||
       NULL != Bits))
    {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      BITMAPOBJ_UnlockBitmap(hBitmap);
      return 0;
    }

  Status = MmCopyFromCaller(&(Info.bmiHeader),
                            &(UnsafeInfo->bmiHeader),
                            Info.bmiHeader.biSize);
  if (! NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
      BITMAPOBJ_UnlockBitmap(hBitmap);
      return 0;
    }

  if (NULL == Bits)
    {
      if (sizeof(BITMAPINFOHEADER) == Info.bmiHeader.biSize)
	{
	  if (0 != Info.bmiHeader.biBitCount)
	    {
              DPRINT("NtGdiGetDIBits(): This operation isn't fully implemented yet.");
	      /*UNIMPLEMENTED;*/
	    }

	  Info.bmiHeader.biWidth = BitmapObj->bitmap.bmWidth;
	  Info.bmiHeader.biHeight = BitmapObj->bitmap.bmHeight;
	  Info.bmiHeader.biPlanes = BitmapObj->bitmap.bmPlanes;
	  Info.bmiHeader.biBitCount = BitmapObj->bitmap.bmBitsPixel;
	  Info.bmiHeader.biCompression = BI_RGB;
	  Info.bmiHeader.biSizeImage = BitmapObj->bitmap.bmHeight * BitmapObj->bitmap.bmWidthBytes;
	}
      else
	{
	  Core = (BITMAPCOREHEADER *)(&Info.bmiHeader);
	  if (0 != Core->bcBitCount)
	    {
	      UNIMPLEMENTED;
	    }

	  Core->bcWidth = BitmapObj->bitmap.bmWidth;
	  Core->bcHeight = BitmapObj->bitmap.bmHeight;
	  Core->bcPlanes = BitmapObj->bitmap.bmPlanes;
	  Core->bcBitCount = BitmapObj->bitmap.bmBitsPixel;
	}

      Status = MmCopyToCaller(UnsafeInfo, &Info, Info.bmiHeader.biSize);
      if (! NT_SUCCESS(Status))
	{
	  SetLastNtError(Status);
	  BITMAPOBJ_UnlockBitmap(hBitmap);
	  return 0;
	}
      Result = 1;
    }
  else if (0 == StartScan && Info.bmiHeader.biHeight == (LONG) (StartScan + ScanLines) &&
           Info.bmiHeader.biWidth == BitmapObj->bitmap.bmWidth &&
           Info.bmiHeader.biHeight == BitmapObj->bitmap.bmHeight &&
           Info.bmiHeader.biPlanes == BitmapObj->bitmap.bmPlanes &&
           Info.bmiHeader.biBitCount == BitmapObj->bitmap.bmBitsPixel &&
           8 < Info.bmiHeader.biBitCount)
    {
      Info.bmiHeader.biSizeImage = BitmapObj->bitmap.bmHeight * BitmapObj->bitmap.bmWidthBytes;
      Status = MmCopyToCaller(Bits, BitmapObj->bitmap.bmBits, Info.bmiHeader.biSizeImage);
      if (! NT_SUCCESS(Status))
	{
	  SetLastNtError(Status);
	  BITMAPOBJ_UnlockBitmap(hBitmap);
	  return 0;
	}
      RtlZeroMemory(&InfoWithBitFields, sizeof(InfoWithBitFields));
      RtlCopyMemory(&(InfoWithBitFields.Info), &Info, sizeof(BITMAPINFO));
      if (BI_BITFIELDS == Info.bmiHeader.biCompression)
	{
	  DCObj = DC_LockDc(hDC);
	  if (NULL == DCObj)
	    {
	      SetLastWin32Error(ERROR_INVALID_HANDLE);
	      BITMAPOBJ_UnlockBitmap(hBitmap);
	      return 0;
	    }
	  PalGdi = PALETTE_LockPalette(DCObj->w.hPalette);
	  BitField = (DWORD *) ((char *) &InfoWithBitFields + InfoWithBitFields.Info.bmiHeader.biSize);
	  BitField[0] = PalGdi->RedMask;
	  BitField[1] = PalGdi->GreenMask;
	  BitField[2] = PalGdi->BlueMask;
	  PALETTE_UnlockPalette(DCObj->w.hPalette);
	  InfoSize = InfoWithBitFields.Info.bmiHeader.biSize + 3 * sizeof(DWORD);
	  DC_UnlockDc(hDC);
	}
      else
	{
	  InfoSize = Info.bmiHeader.biSize;
	}
      Status = MmCopyToCaller(UnsafeInfo, &InfoWithBitFields, InfoSize);
      if (! NT_SUCCESS(Status))
	{
	  SetLastNtError(Status);
	  BITMAPOBJ_UnlockBitmap(hBitmap);
	  return 0;
	}
    }
  else
    {
    UNIMPLEMENTED;
    }

  BITMAPOBJ_UnlockBitmap(hBitmap);

  return Result;
#else
    PDC dc;
    PBITMAPOBJ bmp;
    int i;

    if (!info) return 0;
    if (!(dc = DC_LockDc( hdc ))) return 0;
    if (!(bmp = BITMAPOBJ_LockBitmap(hbitmap)))
    {
        DC_UnlockDc( hdc );
	return 0;
    }

    /* Transfer color info */

    if (info->bmiHeader.biBitCount <= 8 && info->bmiHeader.biBitCount > 0 ) {

	info->bmiHeader.biClrUsed = 0;

	/* If the bitmap object already has a dib section at the
	   same color depth then get the color map from it */
	if (bmp->dib && bmp->dib->dsBm.bmBitsPixel == info->bmiHeader.biBitCount) {
	    NtGdiGetDIBColorTable(hdc, 0, 1 << info->bmiHeader.biBitCount, info->bmiColors);
	}
        else {
            if(info->bmiHeader.biBitCount >= bmp->bitmap.bmBitsPixel) {
                /* Generate the color map from the selected palette */
                PALETTEENTRY * palEntry;
                PPALGDI palette;
                if (!(palette = PALETTE_LockPalette(dc->w.hPalette))) {
                    DC_UnlockDc( hdc );
                    BITMAPOBJ_UnlockBitmap( hbitmap );
                    return 0;
                }
                palEntry = palette->IndexedColors;
                for (i = 0; i < (1 << bmp->bitmap.bmBitsPixel); i++, palEntry++) {
                    if (coloruse == DIB_RGB_COLORS) {
                        info->bmiColors[i].rgbRed      = palEntry->peRed;
                        info->bmiColors[i].rgbGreen    = palEntry->peGreen;
                        info->bmiColors[i].rgbBlue     = palEntry->peBlue;
                        info->bmiColors[i].rgbReserved = 0;
                    }
                    else ((WORD *)info->bmiColors)[i] = (WORD)i;
                }
                PALETTE_UnlockPalette( dc->w.hPalette );
            } else {
                switch (info->bmiHeader.biBitCount) {
                case 1:
                    info->bmiColors[0].rgbRed = info->bmiColors[0].rgbGreen =
                        info->bmiColors[0].rgbBlue = 0;
                    info->bmiColors[0].rgbReserved = 0;
                    info->bmiColors[1].rgbRed = info->bmiColors[1].rgbGreen =
                        info->bmiColors[1].rgbBlue = 0xff;
                    info->bmiColors[1].rgbReserved = 0;
                    break;

                case 4:
                    memcpy(info->bmiColors, COLOR_GetSystemPaletteTemplate(), NB_RESERVED_COLORS * sizeof(PALETTEENTRY));
                    break;

                case 8:
                    {
                        INT r, g, b;
                        RGBQUAD *color;

                        memcpy(info->bmiColors, COLOR_GetSystemPaletteTemplate(),
                               10 * sizeof(RGBQUAD));
                        memcpy(info->bmiColors + 246, COLOR_GetSystemPaletteTemplate() + 10,
                               10 * sizeof(RGBQUAD));
                        color = info->bmiColors + 10;
                        for(r = 0; r <= 5; r++) /* FIXME */
                            for(g = 0; g <= 5; g++)
                                for(b = 0; b <= 5; b++) {
                                    color->rgbRed =   (r * 0xff) / 5;
                                    color->rgbGreen = (g * 0xff) / 5;
                                    color->rgbBlue =  (b * 0xff) / 5;
                                    color->rgbReserved = 0;
                                    color++;
                                }
                    }
                }
            }
        }
    }

    if (bits && lines)
    {
        /* If the bitmap object already have a dib section that contains image data, get the bits from it */
        if(bmp->dib && bmp->dib->dsBm.bmBitsPixel >= 15 && info->bmiHeader.biBitCount >= 15)
        {
            /*FIXME: Only RGB dibs supported for now */
            unsigned int srcwidth = bmp->dib->dsBm.bmWidth, srcwidthb = bmp->dib->dsBm.bmWidthBytes;
            int dstwidthb = DIB_GetDIBWidthBytes( info->bmiHeader.biWidth, info->bmiHeader.biBitCount );
            LPBYTE dbits = bits, sbits = (LPBYTE) bmp->dib->dsBm.bmBits + (startscan * srcwidthb);
            unsigned int x, y;

            if ((info->bmiHeader.biHeight < 0) ^ (bmp->dib->dsBmih.biHeight < 0))
            {
                dbits = (LPBYTE)bits + (dstwidthb * (lines-1));
                dstwidthb = -dstwidthb;
            }

            switch( info->bmiHeader.biBitCount ) {

	    case 15:
            case 16: /* 16 bpp dstDIB */
                {
                    LPWORD dstbits = (LPWORD)dbits;
                    WORD rmask = 0x7c00, gmask= 0x03e0, bmask = 0x001f;

                    /* FIXME: BI_BITFIELDS not supported yet */

                    switch(bmp->dib->dsBm.bmBitsPixel) {

                    case 16: /* 16 bpp srcDIB -> 16 bpp dstDIB */
                        {
                            /* FIXME: BI_BITFIELDS not supported yet */
                            for (y = 0; y < lines; y++, dbits+=dstwidthb, sbits+=srcwidthb)
                                memcpy(dbits, sbits, srcwidthb);
                        }
                        break;

                    case 24: /* 24 bpp srcDIB -> 16 bpp dstDIB */
                        {
                            LPBYTE srcbits = sbits;

                            for( y = 0; y < lines; y++) {
                                for( x = 0; x < srcwidth; x++, srcbits += 3)
                                    *dstbits++ = ((srcbits[0] >> 3) & bmask) |
                                                 (((WORD)srcbits[1] << 2) & gmask) |
                                                 (((WORD)srcbits[2] << 7) & rmask);

                                dstbits = (LPWORD)(dbits+=dstwidthb);
                                srcbits = (sbits += srcwidthb);
                            }
                        }
                        break;

                    case 32: /* 32 bpp srcDIB -> 16 bpp dstDIB */
                        {
                            LPDWORD srcbits = (LPDWORD)sbits;
                            DWORD val;

                            for( y = 0; y < lines; y++) {
                                for( x = 0; x < srcwidth; x++ ) {
                                    val = *srcbits++;
                                    *dstbits++ = (WORD)(((val >> 3) & bmask) | ((val >> 6) & gmask) |
                                                       ((val >> 9) & rmask));
                                }
                                dstbits = (LPWORD)(dbits+=dstwidthb);
                                srcbits = (LPDWORD)(sbits+=srcwidthb);
                            }
                        }
                        break;

                    default: /* ? bit bmp -> 16 bit DIB */
                        DPRINT1("FIXME: 15/16 bit DIB %d bit bitmap\n",
                        bmp->bitmap.bmBitsPixel);
                        break;
                    }
                }
                break;

            case 24: /* 24 bpp dstDIB */
                {
                    LPBYTE dstbits = dbits;

                    switch(bmp->dib->dsBm.bmBitsPixel) {

                    case 16: /* 16 bpp srcDIB -> 24 bpp dstDIB */
                        {
                            LPWORD srcbits = (LPWORD)sbits;
                            WORD val;

                            /* FIXME: BI_BITFIELDS not supported yet */
                            for( y = 0; y < lines; y++) {
                                for( x = 0; x < srcwidth; x++ ) {
                                    val = *srcbits++;
                                    *dstbits++ = (BYTE)(((val << 3) & 0xf8) | ((val >> 2) & 0x07));
                                    *dstbits++ = (BYTE)(((val >> 2) & 0xf8) | ((val >> 7) & 0x07));
                                    *dstbits++ = (BYTE)(((val >> 7) & 0xf8) | ((val >> 12) & 0x07));
                                }
                                dstbits = (LPBYTE)(dbits+=dstwidthb);
                                srcbits = (LPWORD)(sbits+=srcwidthb);
                            }
                        }
                        break;

                    case 24: /* 24 bpp srcDIB -> 24 bpp dstDIB */
                        {
                            for (y = 0; y < lines; y++, dbits+=dstwidthb, sbits+=srcwidthb)
                                memcpy(dbits, sbits, srcwidthb);
                        }
                        break;

                    case 32: /* 32 bpp srcDIB -> 24 bpp dstDIB */
                        {
                            LPBYTE srcbits = (LPBYTE)sbits;

                            for( y = 0; y < lines; y++) {
                                for( x = 0; x < srcwidth; x++, srcbits++ ) {
                                    *dstbits++ = *srcbits++;
                                    *dstbits++ = *srcbits++;
                                    *dstbits++ = *srcbits++;
                                }
                                dstbits=(LPBYTE)(dbits+=dstwidthb);
                                srcbits = (LPBYTE)(sbits+=srcwidthb);
                            }
                        }
                        break;

                    default: /* ? bit bmp -> 24 bit DIB */
                        DPRINT1("FIXME: 24 bit DIB %d bit bitmap\n",
                              bmp->bitmap.bmBitsPixel);
                        break;
                    }
                }
                break;

            case 32: /* 32 bpp dstDIB */
                {
                    LPDWORD dstbits = (LPDWORD)dbits;

                    /* FIXME: BI_BITFIELDS not supported yet */

                    switch(bmp->dib->dsBm.bmBitsPixel) {
                        case 16: /* 16 bpp srcDIB -> 32 bpp dstDIB */
                        {
                            LPWORD srcbits = (LPWORD)sbits;
                            DWORD val;

                            /* FIXME: BI_BITFIELDS not supported yet */
                            for( y = 0; y < lines; y++) {
                                for( x = 0; x < srcwidth; x++ ) {
                                    val = (DWORD)*srcbits++;
                                    *dstbits++ = ((val << 3) & 0xf8) | ((val >> 2) & 0x07) |
                                                 ((val << 6) & 0xf800) | ((val << 1) & 0x0700) |
                                                 ((val << 9) & 0xf80000) | ((val << 4) & 0x070000);
                                }
                                dstbits=(LPDWORD)(dbits+=dstwidthb);
                                srcbits=(LPWORD)(sbits+=srcwidthb);
                            }
                        }
                        break;

                    case 24: /* 24 bpp srcDIB -> 32 bpp dstDIB */
                        {
                            LPBYTE srcbits = sbits;

                            for( y = 0; y < lines; y++) {
                                for( x = 0; x < srcwidth; x++, srcbits+=3 )
                                    *dstbits++ = ((DWORD)*srcbits) & 0x00ffffff;
                                dstbits=(LPDWORD)(dbits+=dstwidthb);
                                srcbits=(sbits+=srcwidthb);
                            }
                        }
                        break;

                    case 32: /* 32 bpp srcDIB -> 16 bpp dstDIB */
                        {
                            /* FIXME: BI_BITFIELDS not supported yet */
                            for (y = 0; y < lines; y++, dbits+=dstwidthb, sbits+=srcwidthb)
                                memcpy(dbits, sbits, srcwidthb);
                        }
                        break;

                    default: /* ? bit bmp -> 32 bit DIB */
                        DPRINT1("FIXME: 32 bit DIB %d bit bitmap\n",
                        bmp->bitmap.bmBitsPixel);
                        break;
                    }
                }
                break;

            default: /* ? bit DIB */
                DPRINT1("FIXME: Unsupported DIB depth %d\n", info->bmiHeader.biBitCount);
                break;
            }
        }
        /* Otherwise, get bits from the XImage */
        else
        {
            UNIMPLEMENTED;
        }
    }
    else if( info->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER) )
    {
	/* fill in struct members */

        if( info->bmiHeader.biBitCount == 0)
	{
	    info->bmiHeader.biWidth = bmp->bitmap.bmWidth;
	    info->bmiHeader.biHeight = bmp->bitmap.bmHeight;
	    info->bmiHeader.biPlanes = 1;
	    info->bmiHeader.biBitCount = bmp->bitmap.bmBitsPixel;
	    info->bmiHeader.biSizeImage =
                             DIB_GetDIBImageBytes( bmp->bitmap.bmWidth,
						   bmp->bitmap.bmHeight,
						   bmp->bitmap.bmBitsPixel );
	    info->bmiHeader.biCompression = 0;
	}
	else
	{
	    info->bmiHeader.biSizeImage = DIB_GetDIBImageBytes(
					       info->bmiHeader.biWidth,
					       info->bmiHeader.biHeight,
					       info->bmiHeader.biBitCount );
	}
    }

    DC_UnlockDc( hdc );
    BITMAPOBJ_UnlockBitmap( hbitmap );

    return lines;
#endif
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
                       CONST VOID  *Bits,
                       CONST BITMAPINFO  *BitsInfo,
                       UINT  Usage,
                       DWORD  ROP)
{
   HBITMAP hBitmap, hOldBitmap;
   HDC hdcMem;

   if (!Bits || !BitsInfo)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return 0;
   }	

   hdcMem = NtGdiCreateCompatableDC(hDC);
   hBitmap = NtGdiCreateCompatibleBitmap(hDC, BitsInfo->bmiHeader.biWidth,
                                         BitsInfo->bmiHeader.biHeight);
   hOldBitmap = NtGdiSelectObject(hdcMem, hBitmap);

   if (BitsInfo->bmiHeader.biCompression == BI_RLE4 ||
       BitsInfo->bmiHeader.biCompression == BI_RLE8)
   {
      /* copy existing bitmap from destination dc */
      NtGdiStretchBlt(hdcMem, XSrc, abs(BitsInfo->bmiHeader.biHeight) - SrcHeight - YSrc,
                      SrcWidth, SrcHeight, hDC, XDest, YDest, DestWidth, DestHeight,
                      ROP);
   }

   NtGdiSetDIBits(hdcMem, hBitmap, 0, BitsInfo->bmiHeader.biHeight, Bits,
                  BitsInfo, Usage);

   /* Origin for DIBitmap may be bottom left (positive biHeight) or top
      left (negative biHeight) */
   NtGdiStretchBlt(hDC, XDest, YDest, DestWidth, DestHeight,
                   hdcMem, XSrc, abs(BitsInfo->bmiHeader.biHeight) - SrcHeight - YSrc,
                   SrcWidth, SrcHeight, ROP);

   NtGdiSelectObject(hdcMem, hOldBitmap);
   NtGdiDeleteDC(hdcMem);
   NtGdiDeleteObject(hBitmap);

   return SrcHeight;
}

LONG STDCALL NtGdiGetBitmapBits(HBITMAP  hBitmap,
                        LONG  Count,
                        LPVOID  Bits)
{
  PBITMAPOBJ  bmp;
  LONG  height, ret;

  bmp = BITMAPOBJ_LockBitmap (hBitmap);
  if (!bmp)
  {
    return 0;
  }

  /* If the bits vector is null, the function should return the read size */
  if (Bits == NULL)
  {
    return bmp->bitmap.bmWidthBytes * bmp->bitmap.bmHeight;
  }

  if (Count < 0)
  {
    DPRINT ("(%ld): Negative number of bytes passed???\n", Count);
    Count = -Count;
  }

  /* Only get entire lines */
  height = Count / bmp->bitmap.bmWidthBytes;
  if (height > bmp->bitmap.bmHeight)
  {
    height = bmp->bitmap.bmHeight;
  }
  Count = height * bmp->bitmap.bmWidthBytes;
  if (Count == 0)
  {
    DPRINT("Less then one entire line requested\n");
    return  0;
  }

  DPRINT("(%08x, %ld, %p) %dx%d %d colors fetched height: %ld\n",
         hBitmap, Count, Bits, bmp->bitmap.bmWidth, bmp->bitmap.bmHeight,
         1 << bmp->bitmap.bmBitsPixel, height );
#if 0
  /* FIXME: Call DDI CopyBits here if available  */
  if(bmp->DDBitmap)
  {
    DPRINT("Calling device specific BitmapBits\n");
    if(bmp->DDBitmap->funcs->pBitmapBits)
    {
      ret = bmp->DDBitmap->funcs->pBitmapBits(hbitmap, bits, count,
                                              DDB_GET);
    }
    else
    {
      ERR_(bitmap)("BitmapBits == NULL??\n");
      ret = 0;
    }
  }
  else
#endif
  {
    if(!bmp->bitmap.bmBits)
    {
      DPRINT ("Bitmap is empty\n");
      ret = 0;
    }
    else
    {
      memcpy(Bits, bmp->bitmap.bmBits, Count);
      ret = Count;
    }
  }

  return  ret;
}

static HBITMAP FASTCALL
IntCreateDIBitmap(PDC Dc, const BITMAPINFOHEADER *header,
                  DWORD init, LPCVOID bits, const BITMAPINFO *data,
                  UINT coloruse)
{
  HBITMAP handle;
  BOOL fColor;
  DWORD width;
  int height;
  WORD bpp;
  WORD compr;

  if (DIB_GetBitmapInfo( header, &width, &height, &bpp, &compr ) == -1) return 0;
  if (height < 0) height = -height;

  // Check if we should create a monochrome or color bitmap. We create a monochrome bitmap only if it has exactly 2
  // colors, which are black followed by white, nothing else. In all other cases, we create a color bitmap.

  if (bpp != 1) fColor = TRUE;
  else if ((coloruse != DIB_RGB_COLORS) ||
           (init != CBM_INIT) || !data) fColor = FALSE;
  else
  {
    if (data->bmiHeader.biSize == sizeof(BITMAPINFOHEADER))
    {
      RGBQUAD *rgb = data->bmiColors;
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
    handle = NtGdiCreateBitmap( width, height, 1, 1, NULL);
  }

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

  Dc = DC_LockDc(hDc);
  if (NULL == Dc)
    {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return NULL;
    }

  Bmp = IntCreateDIBitmap(Dc, Header, Init, Bits, Data, ColorUse);

  DC_UnlockDc(hDc);

  return Bmp;
}

HBITMAP STDCALL NtGdiCreateDIBSection(HDC hDC,
                              CONST BITMAPINFO  *bmi,
                              UINT  Usage,
                              VOID  *Bits,
                              HANDLE  hSection,
                              DWORD  dwOffset)
{
  HBITMAP hbitmap = 0;
  DC *dc;
  BOOL bDesktopDC = FALSE;

  // If the reference hdc is null, take the desktop dc
  if (hDC == 0)
  {
    hDC = NtGdiCreateCompatableDC(0);
    bDesktopDC = TRUE;
  }

  if ((dc = DC_LockDc(hDC)))
  {
    hbitmap = DIB_CreateDIBSection ( dc, (BITMAPINFO*)bmi, Usage, Bits,
      hSection, dwOffset, 0);
    DC_UnlockDc(hDC);
  }

  if (bDesktopDC)
    NtGdiDeleteDC(hDC);

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

  DPRINT1("format (%ld,%ld), planes %d, bpp %d, size %ld, colors %ld (%s)\n",
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
/*    bm.bmBits = MapViewOfFile(section, FILE_MAP_ALL_ACCESS,
			      0L, offset, totalSize); */
    DbgPrint("DIB_CreateDIBSection: Cannot yet handle section DIBs\n");
  else if (ovr_pitch && offset)
    bm.bmBits = (LPVOID) offset;
  else {
    offset = 0;
    bm.bmBits = EngAllocUserMem(totalSize, 0);
  }

/*  bm.bmBits = ExAllocatePool(NonPagedPool, totalSize); */

  if(usage == DIB_PAL_COLORS) memcpy(bmi->bmiColors, (UINT *)DIB_MapPaletteColors(dc, bmi), sizeof(UINT *));

  // Allocate Memory for DIB and fill structure
  if (bm.bmBits)
  {
    dib = ExAllocatePoolWithTag(PagedPool, sizeof(DIBSECTION), TAG_DIB);
    RtlZeroMemory(dib, sizeof(DIBSECTION));
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
        dib->dsBitfields[0] = (bi->biCompression == BI_BITFIELDS) ? *(DWORD *)bmi->bmiColors : 0x7c00;
        dib->dsBitfields[1] = (bi->biCompression == BI_BITFIELDS) ? *((DWORD *)bmi->bmiColors + 1) : 0x03e0;
        dib->dsBitfields[2] = (bi->biCompression == BI_BITFIELDS) ? *((DWORD *)bmi->bmiColors + 2) : 0x001f;
        break;

      case 24:
        dib->dsBitfields[0] = 0xff;
        dib->dsBitfields[1] = 0xff00;
        dib->dsBitfields[2] = 0xff0000;
        break;

      case 32:
        dib->dsBitfields[0] = (bi->biCompression == BI_BITFIELDS) ? *(DWORD *)bmi->bmiColors : 0xff;
        dib->dsBitfields[1] = (bi->biCompression == BI_BITFIELDS) ? *((DWORD *)bmi->bmiColors + 1) : 0xff00;
        dib->dsBitfields[2] = (bi->biCompression == BI_BITFIELDS) ? *((DWORD *)bmi->bmiColors + 2) : 0xff0000;
        break;
    }
    dib->dshSection = section;
    dib->dsOffset = offset;
  }

  // Create Device Dependent Bitmap and add DIB pointer
  if (dib)
  {
    res = IntCreateDIBitmap(dc, bi, 0, NULL, bmi, usage);
    if (! res)
      {
	return NULL;
      } 
    bmp = BITMAPOBJ_LockBitmap(res);
    if (NULL == bmp)
      {
	NtGdiDeleteObject(bmp);
	return NULL;
      }
    bmp->dib = (DIBSECTION *) dib;
    /* Install user-mode bits instead of kernel-mode bits */
    ExFreePool(bmp->bitmap.bmBits);
    bmp->bitmap.bmBits = bm.bmBits;

    /* WINE NOTE: WINE makes use of a colormap, which is a color translation table between the DIB and the X physical
                  device. Obviously, this is left out of the ReactOS implementation. Instead, we call
                  NtGdiSetDIBColorTable. */
    if(bi->biBitCount == 1) { Entries = 2; } else
    if(bi->biBitCount == 4) { Entries = 16; } else
    if(bi->biBitCount == 8) { Entries = 256; }

    bmp->ColorMap = ExAllocatePoolWithTag(NonPagedPool, sizeof(RGBQUAD)*Entries, TAG_COLORMAP);
    RtlCopyMemory(bmp->ColorMap, bmi->bmiColors, sizeof(RGBQUAD)*Entries);
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

  if (bmp)
    {
      BITMAPOBJ_UnlockBitmap(res);
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
  int words;

  switch(depth)
  {
    case 1:  words = (width + 31) >> 5; break;
    case 4:  words = (width + 7) >> 3; break;
    case 8:  words = (width + 3) >> 2; break;
    case 15:
    case 16: words = (width + 1) >> 1; break;
    case 24: words = (width * 3 + 3) >> 2; break;

    default:
      DPRINT("(%d): Unsupported depth\n", depth );
      /* fall through */
    case 32:
      words = width;
  }
  return words << 2;
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

INT STDCALL DIB_GetBitmapInfo (const BITMAPINFOHEADER *header,
			       PDWORD width,
			       PINT height,
			       PWORD bpp,
			       PWORD compr)
{
  if (header->biSize == sizeof(BITMAPINFOHEADER))
  {
    *width  = header->biWidth;
    *height = header->biHeight;
    *bpp    = header->biBitCount;
    *compr  = header->biCompression;
    return 1;
  }
  if (header->biSize == sizeof(BITMAPCOREHEADER))
  {
    BITMAPCOREHEADER *core = (BITMAPCOREHEADER *)header;
    *width  = core->bcWidth;
    *height = core->bcHeight;
    *bpp    = core->bcBitCount;
    *compr  = 0;
    return 0;
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
  DWORD *lpIndex;
  PPALGDI palGDI;

  palGDI = PALETTE_LockPalette(dc->DevInfo->hpalDefault);

  if (NULL == palGDI)
    {
//      RELEASEDCINFO(hDC);
      return NULL;
    }

  nNumColors = 1 << lpbmi->bmiHeader.biBitCount;
  if (lpbmi->bmiHeader.biClrUsed)
    {
      nNumColors = min(nNumColors, lpbmi->bmiHeader.biClrUsed);
    }

  lpRGB = (RGBQUAD *)ExAllocatePoolWithTag(NonPagedPool, sizeof(RGBQUAD) * nNumColors, TAG_COLORMAP);
  lpIndex = (DWORD *)&lpbmi->bmiColors[0];

  for (i = 0; i < nNumColors; i++)
    {
      lpRGB[i].rgbRed = palGDI->IndexedColors[*lpIndex].peRed;
      lpRGB[i].rgbGreen = palGDI->IndexedColors[*lpIndex].peGreen;
      lpRGB[i].rgbBlue = palGDI->IndexedColors[*lpIndex].peBlue;
      lpIndex++;
    }
//    RELEASEDCINFO(hDC);
  PALETTE_UnlockPalette(dc->DevInfo->hpalDefault);

  return lpRGB;
}

PPALETTEENTRY STDCALL
DIBColorTableToPaletteEntries (
	PPALETTEENTRY palEntries,
	const RGBQUAD *DIBColorTable,
	ULONG ColorCount
	)
{
  ULONG i;

  for (i = 0; i < ColorCount; i++)
    {
      palEntries->peRed   = DIBColorTable->rgbRed;
      palEntries->peGreen = DIBColorTable->rgbGreen;
      palEntries->peBlue  = DIBColorTable->rgbBlue;
      palEntries++;
      DIBColorTable++;
    }

  return palEntries;
}

HPALETTE FASTCALL
BuildDIBPalette (PBITMAPINFO bmi, PINT paletteType)
{
  BYTE bits;
  ULONG ColorCount;
  PALETTEENTRY *palEntries = NULL;
  HPALETTE hPal;

  // Determine Bits Per Pixel
  bits = bmi->bmiHeader.biBitCount;

  // Determine paletteType from Bits Per Pixel
  if (bits <= 8)
    {
      *paletteType = PAL_INDEXED;
    }
  else if(bits < 24)
    {
      *paletteType = PAL_BITFIELDS;
    }
  else
    {
      *paletteType = PAL_BGR;
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
      palEntries = ExAllocatePoolWithTag(NonPagedPool, sizeof(PALETTEENTRY)*ColorCount, TAG_COLORMAP);
      DIBColorTableToPaletteEntries(palEntries, bmi->bmiColors, ColorCount);
    }
  hPal = PALETTE_AllocPalette( *paletteType, ColorCount, (ULONG*)palEntries, 0, 0, 0 );
  if (NULL != palEntries)
    {
      ExFreePool(palEntries);
    }

  return hPal;
}

/* EOF */
