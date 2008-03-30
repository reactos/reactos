#include "precomp.h"

#define NDEBUG
#include <debug.h>

/*
 * Return the full scan size for a bitmap.
 *
 * Based on Wine, Utils.c and Windows Graphics Prog pg 595
 */
UINT
FASTCALL
DIB_BitmapMaxBitsSize( PBITMAPINFO Info, UINT ScanLines )
{
    UINT MaxBits = 0;

    if (Info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
       PBITMAPCOREHEADER Core = (PBITMAPCOREHEADER)Info;
       MaxBits = Core->bcBitCount * Core->bcPlanes * Core->bcWidth;
    }
    else  /* assume BITMAPINFOHEADER */
    {
       if ((Info->bmiHeader.biCompression) && (Info->bmiHeader.biCompression != BI_BITFIELDS))
           return Info->bmiHeader.biSizeImage;
    // Planes are over looked by Yuan. I guess assumed always 1.
       MaxBits = Info->bmiHeader.biBitCount * Info->bmiHeader.biPlanes * Info->bmiHeader.biWidth;
    }
    MaxBits = ((MaxBits + 31) & ~31 ) / 8; // From Yuan, ScanLineSize = (Width * bitcount + 31)/32
    return (MaxBits * ScanLines);  // ret the full Size.
}

/*
 * DIB_GetBitmapInfo is complete copy of wine cvs 2/9-2006
 * from file dib.c from gdi32.dll or orginal version
 * did not calc the info right for some headers.
 */
INT
STDCALL
DIB_GetBitmapInfo(const BITMAPINFOHEADER *header,
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

/*
 * @implemented
 */
HBITMAP STDCALL
CreateDIBSection(
   HDC hDC,
   CONST BITMAPINFO *BitmapInfo,
   UINT Usage,
   VOID **Bits,
   HANDLE hSection,
   DWORD dwOffset)
{
   PBITMAPINFO pConvertedInfo;
   UINT ConvertedInfoSize;
   HBITMAP hBitmap = NULL;

   pConvertedInfo = ConvertBitmapInfo(BitmapInfo, Usage,
                                      &ConvertedInfoSize, FALSE);
   if (pConvertedInfo)
   {
      hBitmap = NtGdiCreateDIBSection(hDC, hSection, dwOffset, pConvertedInfo, Usage, 0, 0, 0, Bits);
      if (BitmapInfo != pConvertedInfo)
         RtlFreeHeap(RtlGetProcessHeap(), 0, pConvertedInfo);
   }

   return hBitmap;
}

/*
 * @implemented
 */
BOOL
STDCALL
BitBlt(HDC hdcDest,      /* handle to destination DC */
       int nXOriginDest, /* x-coord of destination upper-left corner */
       int nYOriginDest, /* y-coord of destination upper-left corner */
       int nWidthDest,   /* width of destination rectangle */
       int nHeightDest,  /* height of destination rectangle */
       HDC hdcSrc,       /* handle to source DC */
       int nXSrc,        /* x-coordinate of source upper-left corner */
       int nYSrc,        /* y-coordinate of source upper-left corner */
       DWORD dwRop)      /* raster operation code */
{
    return NtGdiBitBlt(hdcDest,
                       nXOriginDest,
                       nYOriginDest,
                       nWidthDest,
                       nHeightDest,
                       hdcSrc,
                       nXSrc,
                       nYSrc,
                       dwRop,
                       0,
                       0);
}

/*
 * @implemented
 */
BOOL STDCALL
StretchBlt(
   HDC hdcDest,      /* handle to destination DC */
   int nXOriginDest, /* x-coord of destination upper-left corner */
   int nYOriginDest, /* y-coord of destination upper-left corner */
   int nWidthDest,   /* width of destination rectangle */
   int nHeightDest,  /* height of destination rectangle */
   HDC hdcSrc,       /* handle to source DC */
   int nXOriginSrc,  /* x-coord of source upper-left corner */
   int nYOriginSrc,  /* y-coord of source upper-left corner */
   int nWidthSrc,    /* width of source rectangle */
   int nHeightSrc,   /* height of source rectangle */
   DWORD dwRop)      /* raster operation code */

{
   if ((nWidthDest != nWidthSrc) || (nHeightDest != nHeightSrc))
   {
      return NtGdiStretchBlt(hdcDest, nXOriginDest, nYOriginDest, nWidthDest,
                             nHeightDest, hdcSrc, nXOriginSrc, nYOriginSrc,
                             nWidthSrc, nHeightSrc, dwRop, 0);
   }

   return NtGdiBitBlt(hdcDest, nXOriginDest, nYOriginDest, nWidthDest,
                      nHeightDest, hdcSrc, nXOriginSrc, nYOriginSrc, dwRop, 0, 0);
}

/*
 * @implemented
 */
HBITMAP WINAPI
CreateBitmapIndirect(const BITMAP *pbm)
{
   if (pbm)
   {
      return NtGdiCreateBitmap(pbm->bmWidth,
                               pbm->bmHeight,
                               pbm->bmPlanes,
                               pbm->bmBitsPixel,
                               pbm->bmBits);
   }
   return NULL;
}

HBITMAP WINAPI
CreateDiscardableBitmap(
   HDC  hDC,
   INT  Width,
   INT  Height)
{
   return  CreateCompatibleBitmap(hDC, Width, Height);
}


HBITMAP WINAPI
CreateCompatibleBitmap(
   HDC  hDC,
   INT  Width,
   INT  Height)
{
    /* FIXME some part shall be done in user mode */
   return  NtGdiCreateCompatibleBitmap(hDC, Width, Height);
}



INT
STDCALL
GetDIBits(
    HDC hDC,
    HBITMAP hbmp,
    UINT uStartScan,
    UINT cScanLines,
    LPVOID lpvBits,
    LPBITMAPINFO lpbmi,
    UINT uUsage)
{
    INT ret = 0;

    if (hDC == NULL || !GdiIsHandleValid((HGDIOBJ)hDC))
    {
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }
    else if (lpbmi == NULL)
    {
        // XP doesn't run this check and results in a
        // crash in DIB_BitmapMaxBitsSize, we'll be more forgiving
        GdiSetLastError(ERROR_INVALID_PARAMETER);
    }
    else
    {
        UINT cjBmpScanSize = DIB_BitmapMaxBitsSize(lpbmi, cScanLines);

        ret = NtGdiGetDIBitsInternal(hDC,
                                     hbmp,
                                     uStartScan,
                                     cScanLines,
                                     lpvBits,
                                     lpbmi,
                                     uUsage,
                                     cjBmpScanSize,
                                     0);
    }

    return ret;
}

/*
 * @implemented
 */
HBITMAP
STDCALL
CreateDIBitmap( HDC hDC,
                const BITMAPINFOHEADER *Header,
                DWORD Init,
                LPCVOID Bits,
                const BITMAPINFO *Data,
                UINT ColorUse)
{
 LONG width, height, compr, dibsize;
 WORD planes, bpp;

 if (DIB_GetBitmapInfo(Header, &width, &height, &planes, &bpp, &compr, &dibsize) == -1)
 {
    GdiSetLastError(ERROR_INVALID_PARAMETER);
    return NULL;
 }

 return NtGdiCreateDIBitmapInternal(hDC,
                                    width,
                                    height,
                                    Init,
                                    (LPBYTE)Bits,
                                    (PBITMAPINFO)Data,
                                    ColorUse,
                                    bpp,
                                    dibsize,
                                    0,
                                    0);
}

