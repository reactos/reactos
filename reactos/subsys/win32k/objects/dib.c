#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <win32k/bitmaps.h>
#include <win32k/debug.h>
#include <debug.h>
#include "../eng/objects.h"
#include <ntos/minmax.h>

UINT STDCALL W32kSetDIBColorTable(HDC  hDC,
                           UINT  StartIndex,
                           UINT  Entries,
                           CONST RGBQUAD  *Colors)
{
  PDC dc;
  PALETTEENTRY * palEntry;
  PPALOBJ palette;
  RGBQUAD *end;

  if (!(dc = AccessUserObject(hDC))) return 0;

  if (!(palette = AccessUserObject(dc->DevInfo.hpalDefault)))
  {
//    GDI_ReleaseObj( hdc );
    return 0;
  }

  // Transfer color info

  if (dc->w.bitsPerPixel <= 8) {
    palEntry = palette->logpalette->palPalEntry + StartIndex;
    if (StartIndex + Entries > (1 << dc->w.bitsPerPixel))
      Entries = (1 << dc->w.bitsPerPixel) - StartIndex;

    if (StartIndex + Entries > palette->logpalette->palNumEntries)
      Entries = palette->logpalette->palNumEntries - StartIndex;

    for (end = Colors + Entries; Colors < end; palEntry++, Colors++)
    {
      palEntry->peRed   = Colors->rgbRed;
      palEntry->peGreen = Colors->rgbGreen;
      palEntry->peBlue  = Colors->rgbBlue;
    }
  } else {
    Entries = 0;
  }

//  GDI_ReleaseObj(dc->DevInfo.hpalDefault);
//  GDI_ReleaseObj(hdc);

  return Entries;
}

// Converts a DIB to a device-dependent bitmap
INT STDCALL W32kSetDIBits(HDC  hDC,
                   HBITMAP  hBitmap,
                   UINT  StartScan,
                   UINT  ScanLines,
                   CONST VOID  *Bits,
                   CONST BITMAPINFO  *bmi,
                   UINT  ColorUse)
{
  DC *dc;
  BITMAPOBJ *bitmap;
  HBITMAP SourceBitmap, DestBitmap;
  INT result;
  BOOL copyBitsResult;
  PSURFOBJ DestSurf, SourceSurf;
  PSURFGDI DestGDI;
  SIZEL	SourceSize;
  POINTL ZeroPoint;
  RECTL	DestRect;
  PXLATEOBJ XlateObj;
  PPALGDI hDCPalette;
  RGBQUAD *lpRGB;
  HPALETTE DDB_Palette, DIB_Palette;
  USHORT DDB_Palette_Type, DIB_Palette_Type;


  // Check parameters
  if (!(dc = DC_HandleToPtr(hDC))) return 0;

  if (!(bitmap = (BITMAPOBJ *)GDIOBJ_HandleToPtr(hBitmap, GO_BITMAP_MAGIC)))
  {
    // GDI_ReleaseObj(hDC);
    return 0;
  }

  // Get RGB values
  if (ColorUse == DIB_PAL_COLORS) 
    lpRGB = DIB_MapPaletteColors(hDC, bmi);
  else
    lpRGB = &bmi->bmiColors[0];

  // Create a temporary surface for the destination bitmap
  DestSurf   = ExAllocatePool(PagedPool, sizeof(SURFOBJ));
  DestGDI    = ExAllocatePool(PagedPool, sizeof(SURFGDI));
  DestBitmap = CreateGDIHandle(DestGDI, DestSurf);

  BitmapToSurf(hDC, DestGDI, DestSurf, bitmap);

  // Create source surface
  SourceSize.cx = bmi->bmiHeader.biWidth;
  SourceSize.cy = bmi->bmiHeader.biHeight;
  SourceBitmap = EngCreateBitmap(SourceSize, DIB_GetDIBWidthBytes(SourceSize.cx, bmi->bmiHeader.biBitCount),
                                 BitmapFormat(bmi->bmiHeader.biBitCount, bmi->bmiHeader.biCompression),
                                 0, Bits);
  SourceSurf = AccessUserObject(SourceBitmap);

  // Destination palette obtained from the hDC
  hDCPalette = AccessInternalObject(dc->DevInfo.hpalDefault);
  DDB_Palette_Type = hDCPalette->Mode;
  DDB_Palette = dc->DevInfo.hpalDefault;

  // Source palette obtained from the BITMAPINFO
  DIB_Palette = BuildDIBPalette(bmi, &DIB_Palette_Type);

  // Determine XLATEOBJ for color translation
  XlateObj = EngCreateXlate(DDB_Palette_Type, DIB_Palette_Type, DDB_Palette, DIB_Palette);

  // Determine destination rectangle and source point
  ZeroPoint.x = 0;
  ZeroPoint.y = 0;
  DestRect.top	= 0;
  DestRect.left	= 0;
  DestRect.right	= SourceSize.cx;
  DestRect.bottom	= SourceSize.cy;

  copyBitsResult = EngCopyBits(DestSurf, SourceSurf, NULL, XlateObj, &DestRect, &ZeroPoint);

  // Clean up
  EngDeleteSurface(SourceBitmap);
  EngDeleteSurface(DestBitmap);

//  if (ColorUse == DIB_PAL_COLORS) 
//    WinFree((LPSTR)lpRGB);

//  GDI_ReleaseObj(hBitmap); unlock?
//  GDI_ReleaseObj(hDC);

  return result;
}

INT STDCALL W32kSetDIBitsToDevice(HDC  hDC,
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

}

UINT STDCALL W32kGetDIBColorTable(HDC  hDC,
                           UINT  StartIndex,
                           UINT  Entries,
                           RGBQUAD  *Colors)
{
  UNIMPLEMENTED;
}

// Converts a device-dependent bitmap to a DIB
INT STDCALL W32kGetDIBits(HDC  hDC,
                   HBITMAP hBitmap,
                   UINT  StartScan,
                   UINT  ScanLines,
                   LPVOID  Bits,
                   LPBITMAPINFO   bi,
                   UINT  Usage)
{
  UNIMPLEMENTED;
}

INT STDCALL W32kStretchDIBits(HDC  hDC,
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
  UNIMPLEMENTED;
}

LONG STDCALL W32kGetBitmapBits(HBITMAP  hBitmap,
                        LONG  Count,
                        LPVOID  Bits)
{
  PBITMAPOBJ  bmp;
  LONG  height, ret;
  
  bmp = BITMAPOBJ_HandleToPtr (hBitmap);
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
    BITMAPOBJ_UnlockBitmap (hBitmap);
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
  BITMAPOBJ_UnlockBitmap (hBitmap);

  return  ret;
}

// The CreateDIBitmap function creates a device-dependent bitmap (DDB) from a DIB and, optionally, sets the bitmap bits
// The DDB that is created will be whatever bit depth your reference DC is
HBITMAP STDCALL W32kCreateDIBitmap(HDC hdc, const BITMAPINFOHEADER *header,
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

  // Check if we should create a monochrome or color bitmap.
  // We create a monochrome bitmap only if it has exactly 2
  // colors, which are black followed by white, nothing else.
  // In all other cases, we create a color bitmap.

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
      // Note : If the first color of the colormap is white 
      // followed by black, we have to create a color bitmap. 
      // If we don't the white will be displayed in black later on!
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
      DbgPrint("(%ld): wrong size for data\n", data->bmiHeader.biSize );
      return 0;
    }
  }

  // Now create the bitmap

  if (fColor)
  {
    handle = W32kCreateCompatibleBitmap(RetrieveDisplayHDC(), width, height);
  }
  else handle = W32kCreateBitmap(width, height, 1, 1, NULL);

  if (!handle) return 0;

  if (init == CBM_INIT)
  {
    W32kSetDIBits(hdc, handle, 0, height, bits, data, coloruse);
  }

  return handle;
}

HBITMAP STDCALL W32kCreateDIBSection(HDC hDC,
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
    hDC = W32kCreateCompatableDC(0);
    bDesktopDC = TRUE;
  }

  if ((dc = DC_HandleToPtr(hDC)))
  {
    hbitmap = DIB_CreateDIBSection(dc, bmi, Usage, Bits, hSection, dwOffset, 0);
    DC_UnlockDC (hDC);
  }

  if (bDesktopDC)
    W32kDeleteDC(hDC);

  return hbitmap;
}

HBITMAP DIB_CreateDIBSection(
  PDC dc, BITMAPINFO *bmi, UINT usage,
  LPVOID *bits, HANDLE section,
  DWORD offset, DWORD ovr_pitch)
{
  HBITMAP res = 0;
  BITMAPOBJ *bmp = NULL;
  DIBSECTION *dib = NULL;
  int *colorMap = NULL;
  int nColorMap;
  
  // Fill BITMAP32 structure with DIB data
  BITMAPINFOHEADER *bi = &bmi->bmiHeader;
  INT effHeight, totalSize;
  BITMAP bm;
  
  DbgPrint("format (%ld,%ld), planes %d, bpp %d, size %ld, colors %ld (%s)\n",
	bi->biWidth, bi->biHeight, bi->biPlanes, bi->biBitCount,
	bi->biSizeImage, bi->biClrUsed, usage == DIB_PAL_COLORS? "PAL" : "RGB");
  
  effHeight = bi->biHeight >= 0 ? bi->biHeight : -bi->biHeight;
  bm.bmType = 0;
  bm.bmWidth = bi->biWidth;
  bm.bmHeight = effHeight;
  bm.bmWidthBytes = ovr_pitch ? ovr_pitch : DIB_GetDIBWidthBytes(bm.bmWidth, bi->biBitCount);

  bm.bmPlanes = bi->biPlanes;
  bm.bmBitsPixel = bi->biBitCount;
  bm.bmBits = NULL;
  
  // Get storage location for DIB bits.  Only use biSizeImage if it's valid and
  // we're dealing with a compressed bitmap.  Otherwise, use width * height.
  totalSize = bi->biSizeImage && bi->biCompression != BI_RGB
    ? bi->biSizeImage : bm.bmWidthBytes * effHeight;

/*
  if (section)
    bm.bmBits = MapViewOfFile(section, FILE_MAP_ALL_ACCESS, 
			      0L, offset, totalSize);
  else if (ovr_pitch && offset)
    bm.bmBits = (LPVOID) offset;
  else { */
    offset = 0;
/*    bm.bmBits = VirtualAlloc(NULL, totalSize, 
			     MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE); */

  bm.bmBits = ExAllocatePool(NonPagedPool, totalSize);

/*  } */

  if (usage == DIB_PAL_COLORS) memcpy(bmi->bmiColors, (UINT *)DIB_MapPaletteColors(dc, bmi), sizeof(UINT *));

  // Allocate Memory for DIB and fill structure
  if (bm.bmBits)
    dib = ExAllocatePool(PagedPool, sizeof(DIBSECTION));
    RtlZeroMemory(dib, sizeof(DIBSECTION));

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
    res = W32kCreateDIBitmap(dc->hSelf, bi, 0, NULL, bmi, usage);
    if (res)
    {
      bmp = BITMAPOBJ_HandleToPtr (res);
      if (bmp)
      {
        bmp->dib = (DIBSECTION *) dib;
      }
    }
  }

  // Clean up in case of errors
  if (!res || !bmp || !dib || !bm.bmBits || (bm.bmBitsPixel <= 8 && !colorMap))
  {
    DbgPrint("got an error res=%08x, bmp=%p, dib=%p, bm.bmBits=%p\n", res, bmp, dib, bm.bmBits);
/*      if (bm.bmBits)
      {
      if (section)
        UnmapViewOfFile(bm.bmBits), bm.bmBits = NULL;
      else if (!offset)
      VirtualFree(bm.bmBits, 0L, MEM_RELEASE), bm.bmBits = NULL;
    } */
      
    if (colorMap) { ExFreePool(colorMap); colorMap = NULL; }
    if (dib) { ExFreePool(dib); dib = NULL; }
    if (bmp) { BITMAPOBJ_UnlockBitmap(res); bmp = NULL; }
    if (res) { GDIOBJ_FreeObject(res, GO_BITMAP_MAGIC); res = 0; }
  }

  // Install fault handler, if possible
/*  if (bm.bmBits)
  {
    if (VIRTUAL_SetFaultHandler(bm.bmBits, DIB_FaultHandler, (LPVOID)res))
    {
      if (section || offset)
      {
        DIB_DoProtectDIBSection( bmp, PAGE_READWRITE );
        if (dib) dib->status = DIB_AppMod;
      }
      else
      {
        DIB_DoProtectDIBSection( bmp, PAGE_READONLY );
        if (dib) dib->status = DIB_InSync;
      }
    }
  } */

  // Return BITMAP handle and storage location
  if (bmp) BITMAPOBJ_UnlockBitmap(res);
  if (bm.bmBits && bits) *bits = bm.bmBits;
  return res;
}

/***********************************************************************
 *           DIB_GetDIBWidthBytes
 *
 * Return the width of a DIB bitmap in bytes. DIB bitmap data is 32-bit aligned.
 * http://www.microsoft.com/msdn/sdk/platforms/doc/sdk/win32/struc/src/str01.htm
 * 11/16/1999 (RJJ) lifted from wine
 */
int DIB_GetDIBWidthBytes(int  width, int  depth)
{
  int words;
  
  switch(depth)
  {
    case 1:  words = (width + 31) / 32; break;
    case 4:  words = (width + 7) / 8; break;
    case 8:  words = (width + 3) / 4; break;
    case 15:
    case 16: words = (width + 1) / 2; break;
    case 24: words = (width * 3 + 3)/4; break;
      
    default:
      DPRINT("(%d): Unsupported depth\n", depth );
      /* fall through */
    case 32:
      words = width;
  }
  return 4 * words;
}

/***********************************************************************
 *           DIB_GetDIBImageBytes
 *
 * Return the number of bytes used to hold the image in a DIB bitmap.
 * 11/16/1999 (RJJ) lifted from wine
 */

int DIB_GetDIBImageBytes (int  width, int  height, int  depth)
{
  return DIB_GetDIBWidthBytes( width, depth ) * (height < 0 ? -height : height);
}

/***********************************************************************
 *           DIB_BitmapInfoSize
 *
 * Return the size of the bitmap info structure including color table.
 * 11/16/1999 (RJJ) lifted from wine
 */

int DIB_BitmapInfoSize (const BITMAPINFO * info, WORD coloruse)
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

int DIB_GetBitmapInfo( const BITMAPINFOHEADER *header, DWORD *width,
                              int *height, WORD *bpp, WORD *compr )
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
  DbgPrint("(%ld): wrong size for header\n", header->biSize );
  return -1;
}

// Converts a Device Independent Bitmap (DIB) to a Device Dependant Bitmap (DDB)
// The specified Device Context (DC) defines what the DIB should be converted to
PBITMAPOBJ DIBtoDDB(HGLOBAL hPackedDIB, HDC hdc) // FIXME: This should be removed. All references to this function should
						 // change to W32kSetDIBits
{
  HBITMAP hBmp = 0;
  PBITMAPOBJ pBmp = NULL;
  DIBSECTION *dib;
  LPBYTE pbits = NULL;

  // Get a pointer to the packed DIB's data
  // pPackedDIB = (LPBYTE)GlobalLock(hPackedDIB);
  dib = hPackedDIB;

  pbits = (dib + DIB_BitmapInfoSize(&dib->dsBmih, DIB_RGB_COLORS));

  // Create a DDB from the DIB
  hBmp = W32kCreateDIBitmap(hdc, &dib->dsBmih, CBM_INIT, (LPVOID)pbits, &dib->dsBmih, DIB_RGB_COLORS);

  // GlobalUnlock(hPackedDIB);

  // Retrieve the internal Pixmap from the DDB
  pBmp = (BITMAPOBJ *)GDIOBJ_HandleToPtr(hBmp, GO_BITMAP_MAGIC);

  return pBmp;
}

RGBQUAD *DIB_MapPaletteColors(PDC dc, LPBITMAPINFO lpbmi)
{
  RGBQUAD *lpRGB;
  int nNumColors,i;
  DWORD *lpIndex;
  PPALOBJ palObj;

  palObj = AccessUserObject(dc->DevInfo.hpalDefault);

  if (palObj == NULL) {
//      RELEASEDCINFO(hDC);
    return NULL;
  }

  nNumColors = 1 << lpbmi->bmiHeader.biBitCount;
  if (lpbmi->bmiHeader.biClrUsed)
    nNumColors = min(nNumColors, lpbmi->bmiHeader.biClrUsed);

  lpRGB = (RGBQUAD *)ExAllocatePool(NonPagedPool, sizeof(RGBQUAD) * nNumColors);
  lpIndex = (DWORD *)&lpbmi->bmiColors[0];

  for (i=0; i<nNumColors; i++) {
    lpRGB[i].rgbRed = palObj->logpalette->palPalEntry[*lpIndex].peRed;
    lpRGB[i].rgbGreen = palObj->logpalette->palPalEntry[*lpIndex].peGreen;
    lpRGB[i].rgbBlue = palObj->logpalette->palPalEntry[*lpIndex].peBlue;
    lpIndex++;
  }
//    RELEASEDCINFO(hDC);
//    RELEASEPALETTEINFO(hPalette);
  return lpRGB;
}

HPALETTE BuildDIBPalette(BITMAPINFO *bmi, PINT paletteType)
{
  BYTE bits;

  // Determine Bits Per Pixel
  bits = bmi->bmiHeader.biBitCount;

  // Determine paletteType from Bits Per Pixel
  if(bits <= 8)
  {
    *paletteType = PAL_INDEXED;
  } else
  if(bits < 24)
  {
    *paletteType = PAL_BITFIELDS;
  } else {
    *paletteType = PAL_RGB; // FIXME: This could be BGR, must still check
  }

  return EngCreatePalette(*paletteType, bmi->bmiHeader.biClrUsed, bmi->bmiColors, 0, 0, 0);
}
