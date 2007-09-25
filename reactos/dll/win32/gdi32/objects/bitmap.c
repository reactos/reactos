#include "precomp.h"

//#define NDEBUG
//#include <debug.h>

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
