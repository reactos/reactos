/* $Id: bitblt.c,v 1.21 2004/04/09 20:03:13 navaraf Exp $
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
BitBlt(HDC  hDCDest,
	INT  XDest,
	INT  YDest,
	INT  Width,
	INT  Height,
	HDC  hDCSrc,
	INT  XSrc,
	INT  YSrc,
	DWORD  ROP)
{
	return NtGdiBitBlt(hDCDest, XDest, YDest, Width, Height, hDCSrc, XSrc, YSrc, ROP);
}


/*
 * @implemented
 */
HBITMAP
STDCALL
CreateBitmap(INT  Width,
	INT  Height,
	UINT  Planes,
	UINT  BitsPerPel,
	CONST VOID *Bits)
{
	return NtGdiCreateBitmap(Width, Height, Planes, BitsPerPel, Bits);
}


/*
 * @implemented
 */
HBITMAP
STDCALL
CreateBitmapIndirect(CONST BITMAP  *BM)
{
	return NtGdiCreateBitmapIndirect(BM);
}


/*
 * @implemented
 */
HBITMAP
STDCALL
CreateCompatibleBitmap(HDC hDC,
	INT  Width,
	INT  Height)
{
	return NtGdiCreateCompatibleBitmap(hDC, Width, Height);
}


/*
 * @implemented
 */
HBITMAP
STDCALL
CreateDiscardableBitmap(HDC  hDC,
	INT  Width,
	INT  Height)
{
	return NtGdiCreateDiscardableBitmap(hDC, Width, Height);
}


/*
 * @implemented
 */
HBITMAP
STDCALL
CreateDIBitmap(HDC  hDC,
	CONST BITMAPINFOHEADER  *bmih,
	DWORD  Init,
	CONST VOID  *bInit,
	CONST BITMAPINFO  *bmi,
	UINT  Usage)
{
	return NtGdiCreateDIBitmap(hDC, bmih, Init, bInit, bmi, Usage);
}


/*
 * @implemented
 */
LONG
STDCALL
GetBitmapBits(HBITMAP  hBitmap,
	LONG  Count,
	LPVOID  Bits)
{
	return NtGdiGetBitmapBits(hBitmap, Count, Bits);
}


/*
 * @implemented
 */
BOOL
STDCALL
GetBitmapDimensionEx(HBITMAP  hBitmap,
	LPSIZE  Dimension)
{
	return NtGdiGetBitmapDimensionEx(hBitmap, Dimension);
}


/*
 * @implemented
 */
int
STDCALL
GetDIBits(HDC  hDC,
	HBITMAP hBitmap,
	UINT  StartScan,
	UINT  ScanLines,
	LPVOID  Bits,
	LPBITMAPINFO   bi,
	UINT  Usage)
{
	return NtGdiGetDIBits(hDC, hBitmap, StartScan, ScanLines, Bits, bi, Usage);
}


/*
 * @implemented
 */
BOOL
STDCALL
MaskBlt(HDC  hDCDest,
	INT  XDest,
	INT  YDest,
	INT  Width,
	INT  Height,
	HDC  hDCSrc,
	INT  XSrc,
	INT  YSrc,
	HBITMAP  hMaskBitmap,
	INT  xMask,
	INT  yMask,
	DWORD  ROP)
{
	return NtGdiMaskBlt(hDCDest, XDest, YDest, Width, Height, hDCSrc, XSrc, YSrc, hMaskBitmap, xMask, yMask, ROP);
}


/*
 * @implemented
 */
BOOL
STDCALL
PlgBlt(HDC  hDCDest,
	CONST POINT  *Point,
	HDC  hDCSrc, 
	INT  XSrc,  
	INT  YSrc,  
	INT  Width, 
	INT  Height,
	HBITMAP  hMaskBitmap,
	INT  xMask,      
	INT  yMask)
{
	return NtGdiPlgBlt(hDCDest, Point, hDCSrc, XSrc, YSrc, Width, Height, hMaskBitmap, xMask, yMask);
}


/*
 * @implemented
 */
LONG
STDCALL
SetBitmapBits(HBITMAP  hBitmap,
	DWORD  Bytes,
	CONST VOID *Bits)
{
	return NtGdiSetBitmapBits(hBitmap, Bytes, Bits);
}


/*
 * @implemented
 */
int
STDCALL
SetDIBits(HDC  hDC,
	HBITMAP  hBitmap,
	UINT  StartScan,
	UINT  ScanLines,
	CONST VOID  *Bits,
	CONST BITMAPINFO  *bmi,
	UINT  ColorUse)
{
	return NtGdiSetDIBits(hDC, hBitmap, StartScan, ScanLines, Bits, bmi, ColorUse);
}


/*
 * @implemented
 */
int
STDCALL
SetDIBitsToDevice(HDC  hDC,
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
	return NtGdiSetDIBitsToDevice(hDC, XDest, YDest, Width, Height, XSrc, YSrc, StartScan, ScanLines,
		Bits, bmi, ColorUse);
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
	//SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	if ( (nWidthDest==nWidthSrc) && (nHeightDest==nHeightSrc) )
	{
	        return 	BitBlt(hdcDest,
				nXOriginDest,  // x-coord of destination upper-left corner
				nYOriginDest,  // y-coord of destination upper-left corner
				nWidthDest,  // width of destination rectangle
				nHeightDest, // height of destination rectangle
				hdcSrc,  // handle to source DC
				nXOriginSrc,   // x-coordinate of source upper-left corner
				nYOriginSrc,   // y-coordinate of source upper-left corner
				dwRop  // raster operation code
				);
	} else
	{
		DPRINT1("FIXME: StretchBlt is in development phase now...\n");
		return NtGdiStretchBlt(hdcDest,
					nXOriginDest,
					nYOriginDest,
					nWidthDest,
					nHeightDest,
					hdcSrc,
					nXOriginSrc,
					nYOriginSrc,
					nWidthSrc,
					nHeightSrc,
					dwRop);	
	}
}


/*
 * @implemented
 */
int
STDCALL
StretchDIBits(HDC  hDC,
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
	return NtGdiStretchDIBits(hDC, XDest, YDest, DestWidth, DestHeight, XSrc, YSrc,
		SrcWidth, SrcHeight, Bits, BitsInfo, Usage, ROP);
}


/*
 * @implemented
 */
HBITMAP 
STDCALL 
CreateDIBSection(HDC hDC,
	CONST BITMAPINFO  *bmi,
	UINT  Usage,
	PVOID  *Bits,
	HANDLE  hSection,
	DWORD  dwOffset)
{
	return NtGdiCreateDIBSection(hDC, bmi, Usage, Bits, hSection, dwOffset);
}


/*
 * @implemented
 */
COLORREF 
STDCALL 
SetPixel(HDC  hDC,
	INT  X,
	INT  Y,
	COLORREF  Color)
{
	return NtGdiSetPixel(hDC, X, Y, Color);
}


/*
 * @implemented
 */
BOOL
STDCALL
SetPixelV ( HDC hDC, int XPos, int YPos, COLORREF Color )
{
	return NtGdiSetPixelV ( hDC, XPos, YPos, Color );
}


/*
 * @implemented
 */
COLORREF
STDCALL
GetPixel(HDC hDC, int X, int Y)
{
   return NtGdiGetPixel(hDC, X, Y);
}


/*
 * @implemented
 */
BOOL STDCALL
PatBlt(HDC hDC, INT Left, INT Top, INT Width, INT Height, ULONG Rop)
{
   return NtGdiPatBlt(hDC, Left, Top, Width, Height, Rop);
}

/*
 * @implemented
 */
BOOL 
STDCALL 
PolyPatBlt(HDC hDC,DWORD dwRop,PPATRECT pRects,int cRects,ULONG Reserved)
{
	return NtGdiPolyPatBlt(hDC,dwRop,pRects,cRects,Reserved);
}

/******************************************************************************
 *           GdiTransparentBlt [GDI32.@]
 *
 * @implemented
 */
WINBOOL
STDCALL
GdiTransparentBlt(HDC hdcDst, int xDst, int yDst, int cxDst, int cyDst,
                  HDC hdcSrc, int xSrc, int ySrc, int cxSrc, int cySrc,
                  COLORREF TransColor)
{
  return (WINBOOL)NtGdiTransparentBlt(hdcDst, xDst, yDst, cxDst, cyDst, hdcSrc, 
                                      xSrc, ySrc, cxSrc, cySrc, TransColor);
}

/*

BOOL STDCALL NtGdiExtFloodFill(HDC  hDC, INT  XStart, INT  YStart, COLORREF  Color, UINT  FillType)
BOOL STDCALL NtGdiFloodFill(HDC  hDC, INT  XStart, INT  YStart, COLORREF  Fill)
UINT STDCALL NtGdiGetDIBColorTable(HDC  hDC, UINT  StartIndex, UINT  Entries, RGBQUAD  *Colors)
COLORREF STDCALL NtGdiGetPixel(HDC  hDC,
                       INT  XPos,
                       INT  YPos)
BOOL STDCALL NtGdiSetBitmapDimensionEx(HBITMAP  hBitmap,
                               INT  Width,
                               INT  Height,
                               LPSIZE  Size)
UINT STDCALL NtGdiSetDIBColorTable(HDC  hDC,
                           UINT  StartIndex,
                           UINT  Entries,
                           CONST RGBQUAD  *Colors)
BOOL STDCALL NtGdiSetPixelV(HDC  hDC,
                    INT  X,
                    INT  Y,
                    COLORREF  Color)
BOOL STDCALL NtGdiStretchBlt(HDC  hDCDest,
                     INT  XOriginDest,
                     INT  YOriginDest,
                     INT  WidthDest,
                     INT  HeightDest,
                     HDC  hDCSrc,
                     INT  XOriginSrc,
                     INT  YOriginSrc,
                     INT  WidthSrc,    
                     INT  HeightSrc, 
                     DWORD  ROP)

INT BITMAPOBJ_GetWidthBytes (INT bmWidth, INT bpp)
HBITMAP  BITMAPOBJ_CopyBitmap(HBITMAP  hBitmap)
int DIB_GetDIBWidthBytes(int  width, int  depth)
int DIB_GetDIBImageBytes (int  width, int  height, int  depth)
int DIB_BitmapInfoSize (const BITMAPINFO * info, WORD coloruse)

*/
