/* $Id: bitblt.c,v 1.22 2004/04/25 14:46:54 weiden Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/gdi32/object/bitblt.c
 * PURPOSE:         
 * PROGRAMMER:
 */

#ifdef UNICODE
#undef UNICODE
#endif

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/kapi.h>
#include <debug.h>

/*
 * @implemented
 */
BOOL
STDCALL
BitBlt(HDC  hdcDest,
	INT  nXDest,
	INT  nYDest,
	INT  nWidth,
	INT  nHeight,
	HDC  hdcSrc,
	INT  nXSrc,
	INT  nYSrc,
	DWORD dwRop)
{
  return NtGdiBitBlt(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, dwRop);
}


/*
 * @implemented
 */
HBITMAP
STDCALL
CreateBitmap(INT  nWidth,
	INT  nHeight,
	UINT  cPlanes,
	UINT  cBitsPerPel,
	CONST VOID *lpvBits)
{
  return NtGdiCreateBitmap(nWidth, nHeight, cPlanes, cBitsPerPel, lpvBits);
}


/*
 * @implemented
 */
HBITMAP
STDCALL
CreateBitmapIndirect(CONST BITMAP  *lpbm)
{
  return NtGdiCreateBitmapIndirect(lpbm);
}


/*
 * @implemented
 */
HBITMAP
STDCALL
CreateCompatibleBitmap(HDC hdc,
	INT  nWidth,
	INT  nHeight)
{
  return NtGdiCreateCompatibleBitmap(hdc, nWidth, nHeight);
}


/*
 * @implemented
 */
HBITMAP
STDCALL
CreateDiscardableBitmap(HDC  hdc,
	INT  nWidth,
	INT  nHeight)
{
  return NtGdiCreateDiscardableBitmap(hdc, nWidth, nHeight);
}


/*
 * @implemented
 */
HBITMAP
STDCALL
CreateDIBitmap(HDC  hdc,
	CONST BITMAPINFOHEADER  *lpbmih,
	DWORD  fdwInit,
	CONST VOID  *lpbInit,
	CONST BITMAPINFO  *lpbmi,
	UINT  fuUsage)
{
  return NtGdiCreateDIBitmap(hdc, lpbmih, fdwInit, lpbInit, lpbmi, fuUsage);
}


/*
 * @implemented
 */
LONG
STDCALL
GetBitmapBits(HBITMAP  hbmp,
	LONG  cbBuffer,
	LPVOID  lpvBits)
{
  return NtGdiGetBitmapBits(hbmp, cbBuffer, lpvBits);
}


/*
 * @implemented
 */
BOOL
STDCALL
GetBitmapDimensionEx(HBITMAP  hBitmap,
	LPSIZE  lpDimension)
{
  return NtGdiGetBitmapDimensionEx(hBitmap, lpDimension);
}


/*
 * @implemented
 */
int
STDCALL
GetDIBits(HDC  hdc,
	HBITMAP hbmp,
	UINT  uStartScan,
	UINT  cScanLines,
	LPVOID  lpvBits,
	LPBITMAPINFO   lpbi,
	UINT  uUsage)
{
  return NtGdiGetDIBits(hdc, hbmp, uStartScan, cScanLines, lpvBits, lpbi, uUsage);
}


/*
 * @implemented
 */
BOOL
STDCALL
MaskBlt(HDC  hdcDest,
	INT  nXDest,
	INT  nYDest,
	INT  nWidth,
	INT  nHeight,
	HDC  hdcSrc,
	INT  nXSrc,
	INT  nYSrc,
	HBITMAP  hbmMask,
	INT  xMask,
	INT  yMask,
	DWORD dwRop)
{
  return NtGdiMaskBlt(hdcDest, nXDest, nYDest, nWidth, nHeight, hdcSrc, nXSrc, nYSrc, hbmMask, xMask, yMask, dwRop);
}


/*
 * @implemented
 */
BOOL
STDCALL
PlgBlt(HDC  hdcDest,
	CONST POINT  *lpPoint,
	HDC  hdcSrc, 
	INT  nXSrc,  
	INT  nYSrc,  
	INT  nWidth, 
	INT  nHeight,
	HBITMAP  hbmMask,
	INT  xMask,      
	INT  yMask)
{
  return NtGdiPlgBlt(hdcDest, lpPoint, hdcSrc, nXSrc, nYSrc, nWidth, nHeight, hbmMask, xMask, yMask);
}


/*
 * @implemented
 */
LONG
STDCALL
SetBitmapBits(HBITMAP  hbmp,
	DWORD  cBytes,
	CONST VOID *lpBits)
{
  return NtGdiSetBitmapBits(hbmp, cBytes, lpBits);
}


/*
 * @implemented
 */
int
STDCALL
SetDIBits(HDC  hdc,
	HBITMAP  hbmp,
	UINT  uStartScan,
	UINT  cScanLines,
	CONST VOID  *lpvBits,
	CONST BITMAPINFO  *lpbmi,
	UINT  fuColorUse)
{
  return NtGdiSetDIBits(hdc, hbmp, uStartScan, cScanLines, lpvBits, lpbmi, fuColorUse);
}


/*
 * @implemented
 */
int
STDCALL
SetDIBitsToDevice(HDC  hdc,
	INT  XDest,
	INT  YDest,
	DWORD  dwWidth,
	DWORD  dwHeight,
	INT  XSrc,
	INT  YSrc,
	UINT  uStartScan,
	UINT  cScanLines,
	CONST VOID  *lpvBits,
	CONST BITMAPINFO  *lpbmi,
	UINT  fuColorUse)
{
  return NtGdiSetDIBitsToDevice(hdc, XDest, YDest, dwWidth, dwHeight, XSrc, YSrc, uStartScan, cScanLines,
		lpvBits, lpbmi, fuColorUse);
}


/*
 * @implemented
 */
BOOL
STDCALL
StretchBlt(
           HDC hdcDest,      // handle to destination DC
           int nXOriginDest, // x-coord of destination upper-left corner
           int nYOriginDest, // y-coord of destination upper-left corner
           int nWidthDest,   // width of destination rectangle
           int nHeightDest,  // height of destination rectangle
           HDC hdcSrc,       // handle to source DC
           int nXOriginSrc,  // x-coord of source upper-left corner
           int nYOriginSrc,  // y-coord of source upper-left corner
           int nWidthSrc,    // width of source rectangle
           int nHeightSrc,   // height of source rectangle
           DWORD dwRop       // raster operation code
	)
{
  if ((nWidthDest != nWidthSrc) || (nHeightDest != nHeightSrc))
  {
    return NtGdiStretchBlt(hdcDest, nXOriginDest, nYOriginDest, nWidthDest, nHeightDest,
                           hdcSrc, nXOriginSrc, nYOriginSrc, nWidthSrc, nHeightSrc, dwRop);
  }
  
  return BitBlt(hdcDest, nXOriginDest, nYOriginDest, nWidthDest, nHeightDest, hdcSrc,
                nXOriginSrc, nYOriginSrc, dwRop);
}


/*
 * @implemented
 */
int
STDCALL
StretchDIBits(HDC  hdc,
	INT  XDest,
	INT  YDest,
	INT  nDestWidth,
	INT  nDestHeight,
	INT  XSrc,
	INT  YSrc,
	INT  nSrcWidth,
	INT  nSrcHeight,
	CONST VOID  *lpBits,
	CONST BITMAPINFO  *lpBitsInfo,
	UINT  iUsage,
	DWORD dwRop)
{
  return NtGdiStretchDIBits(hdc, XDest, YDest, nDestWidth, nDestHeight, XSrc, YSrc,
                            nSrcWidth, nSrcHeight, lpBits, lpBitsInfo, iUsage, dwRop);
}


/*
 * @implemented
 */
HBITMAP 
STDCALL 
CreateDIBSection(HDC hdc,
	CONST BITMAPINFO  *pbmi,
	UINT  iUsage,
	VOID  **ppvBits,
	HANDLE  hSection,
	DWORD  dwOffset)
{
  return NtGdiCreateDIBSection(hdc, pbmi, iUsage, ppvBits, hSection, dwOffset);
}


/*
 * @implemented
 */
COLORREF 
STDCALL 
SetPixel(HDC  hdc,
	INT  X,
	INT  Y,
	COLORREF crColor)
{
  return NtGdiSetPixel(hdc, X, Y, crColor);
}


/*
 * @implemented
 */
BOOL
STDCALL
SetPixelV ( HDC hdc, int X, int Y, COLORREF crColor )
{
  return NtGdiSetPixelV(hdc, X, Y, crColor);
}


/*
 * @implemented
 */
COLORREF
STDCALL
GetPixel(HDC hdc, int nXPos, int nYPos)
{
  return NtGdiGetPixel(hdc, nXPos, nYPos);
}


/*
 * @implemented
 */
BOOL STDCALL
PatBlt(HDC hdc, INT nXLeft, INT nYLeft, INT nWidth, INT nHeight, DWORD dwRop)
{
  return NtGdiPatBlt(hdc, nXLeft, nYLeft, nWidth, nHeight, dwRop);
}

/*
 * @implemented
 */
BOOL 
STDCALL 
PolyPatBlt(HDC hdc, DWORD dwRop, PPATRECT lppRects, int cRects, ULONG Reserved)
{
  return NtGdiPolyPatBlt(hdc, dwRop, lppRects, cRects, Reserved);
}

/******************************************************************************
 *           GdiTransparentBlt [GDI32.@]
 *
 * @implemented
 */
WINBOOL
STDCALL
GdiTransparentBlt(HDC hdcDst, int nXOriginDest, int nYOriginDest, int nWidthDest, int nHeightDest,
                  HDC hdcSrc, int nXOriginSrc, int nYOriginSrc, int nWidthSrc, int nHeightSrc,
                  COLORREF crTransparent)
{
  return (WINBOOL)NtGdiTransparentBlt(hdcDst, nXOriginDest, nYOriginDest, nWidthDest, nHeightDest, hdcSrc, 
                                      nXOriginSrc, nYOriginSrc, nWidthSrc, nHeightSrc, crTransparent);
}

