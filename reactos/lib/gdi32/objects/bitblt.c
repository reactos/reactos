#ifdef UNICODE
#undef UNICODE
#endif

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/kapi.h>
#include <debug.h>

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
	return W32kBitBlt(hDCDest, XDest, YDest, Width, Height, hDCSrc, XSrc, YSrc, ROP);
}

HBITMAP
STDCALL
CreateBitmap(INT  Width,
	INT  Height,
	UINT  Planes,
	UINT  BitsPerPel,
	CONST VOID *Bits)
{
	return W32kCreateBitmap(Width, Height, Planes, BitsPerPel, Bits);
}

HBITMAP
STDCALL
CreateBitmapIndirect(CONST BITMAP  *BM)
{
	return W32kCreateBitmapIndirect(BM);
}

HBITMAP
STDCALL
CreateCompatibleBitmap(HDC hDC,
	INT  Width,
	INT  Height)
{
	return W32kCreateCompatibleBitmap(hDC, Width, Height);
}

HBITMAP
STDCALL
CreateDiscardableBitmap(HDC  hDC,
	INT  Width,
	INT  Height)
{
	return W32kCreateDiscardableBitmap(hDC, Width, Height);
}

HBITMAP
STDCALL
CreateDIBitmap(HDC  hDC,
	CONST BITMAPINFOHEADER  *bmih,
	DWORD  Init,
	CONST VOID  *bInit,
	CONST BITMAPINFO  *bmi,
	UINT  Usage)
{
	return W32kCreateDIBitmap(hDC, bmih, Init, bInit, bmi, Usage);
}

LONG
STDCALL
GetBitmapBits(HBITMAP  hBitmap,
	LONG  Count,
	LPVOID  Bits)
{
	return W32kGetBitmapBits(hBitmap, Count, Bits);
}

BOOL
STDCALL
GetBitmapDimensionEx(HBITMAP  hBitmap,
	LPSIZE  Dimension)
{
	return W32kGetBitmapDimensionEx(hBitmap, Dimension);
}

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
	return W32kGetDIBits(hDC, hBitmap, StartScan, ScanLines, Bits, bi, Usage);
}

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
	return W32kMaskBlt(hDCDest, XDest, YDest, Width, Height, hDCSrc, XSrc, YSrc, hMaskBitmap, xMask, yMask, ROP);
}

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
	return W32kPlgBlt(hDCDest, Point, hDCSrc, XSrc, YSrc, Width, Height, hMaskBitmap, xMask, yMask);
}

LONG
STDCALL
SetBitmapBits(HBITMAP  hBitmap,
	DWORD  Bytes,
	CONST VOID *Bits)
{
	return W32kSetBitmapBits(hBitmap, Bytes, Bits);
}

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
	return W32kSetDIBits(hDC, hBitmap, StartScan, ScanLines, Bits, bmi, ColorUse);
}

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
	return W32kSetDIBitsToDevice(hDC, XDest, YDest, Width, Height, XSrc, YSrc, StartScan, ScanLines,
		Bits, bmi, ColorUse);
}

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
	}
	
	DPRINT1("FIXME: StretchBlt can only Blt, not Stretch!\n");
	return FALSE;
}

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
	return W32kStretchDIBits(hDC, XDest, YDest, DestWidth, DestHeight, XSrc, YSrc,
		SrcWidth, SrcHeight, Bits, BitsInfo, Usage, ROP);
}

HBITMAP 
STDCALL 
CreateDIBSection(HDC hDC,
	CONST BITMAPINFO  *bmi,
	UINT  Usage,
	VOID  *Bits,
	HANDLE  hSection,
	DWORD  dwOffset)
{
	return W32kCreateDIBSection(hDC, bmi, Usage, Bits, hSection, dwOffset);
}

COLORREF 
STDCALL 
SetPixel(HDC  hDC,
	INT  X,
	INT  Y,
	COLORREF  Color)
{
	return W32kSetPixel(hDC, X, Y, Color);
}

BOOL STDCALL
PatBlt(HDC hDC, INT Top, INT Left, INT Width, INT Height, ULONG Rop)
{
  return(W32kPatBlt(hDC, Top, Left, Width, Height, Rop));
}




/*

BOOL STDCALL W32kExtFloodFill(HDC  hDC, INT  XStart, INT  YStart, COLORREF  Color, UINT  FillType)
BOOL STDCALL W32kFloodFill(HDC  hDC, INT  XStart, INT  YStart, COLORREF  Fill)
UINT STDCALL W32kGetDIBColorTable(HDC  hDC, UINT  StartIndex, UINT  Entries, RGBQUAD  *Colors)
COLORREF STDCALL W32kGetPixel(HDC  hDC,
                       INT  XPos,
                       INT  YPos)
BOOL STDCALL W32kSetBitmapDimensionEx(HBITMAP  hBitmap,
                               INT  Width,
                               INT  Height,
                               LPSIZE  Size)
UINT STDCALL W32kSetDIBColorTable(HDC  hDC,
                           UINT  StartIndex,
                           UINT  Entries,
                           CONST RGBQUAD  *Colors)
BOOL STDCALL W32kSetPixelV(HDC  hDC,
                    INT  X,
                    INT  Y,
                    COLORREF  Color)
BOOL STDCALL W32kStretchBlt(HDC  hDCDest,
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
