

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <win32k/bitmaps.h>

// #define NDEBUG
#include <internal/debug.h>

BOOL  W32kBitBlt(HDC  hDCDest,
                 INT  XDest,
                 INT  YDest,
                 INT  Width,
                 INT  Height,
                 HDC  hDCSrc,
                 INT  XSrc,
                 INT  YSrc,
                 DWORD  ROP)
{
  UNIMPLEMENTED;
}

HBITMAP  W32kCreateBitmap(INT  Width,
                          INT  Height,
                          UINT  Planes,
                          UINT  BitsPerPel,
                          CONST VOID *Bits)
{
  UNIMPLEMENTED;
}

HBITMAP  W32kCreateBitmapIndirect(CONST BITMAP  *BM)
{
  UNIMPLEMENTED;
}

HBITMAP  W32kCreateDIBitmap(HDC  hDC,
                            CONST BITMAPINFOHEADER  *bmih,
                            DWORD  Init,
                            CONST VOID  *bInit,
                            CONST BITMAPINFO  *bmi,
                            UINT  Usage)
{
  UNIMPLEMENTED;
}

HBITMAP  W32kCreateDIBSection(HDC hDC,
                              CONST BITMAPINFO  *bmi,
                              UINT  Usage,
                              VOID  *Bits,
                              HANDLE  hSection,
                              DWORD  dwOffset)
{
  UNIMPLEMENTED;
}

HBITMAP  W32kCreateDiscardableBitmap(HDC  hDC,
                                     INT  Width,
                                     INT  Height)
{
  UNIMPLEMENTED;
}

BOOL W32kExtFloodFill(HDC  hDC,
                      INT  XStart,
                      INT  YStart,
                      COLORREF  Color, 
                      UINT  FillType)
{
  UNIMPLEMENTED;
}

BOOL  W32kFloodFill(HDC  hDC,
                    INT  XStart,
                    INT  YStart,
                    COLORREF  Fill)
{
  UNIMPLEMENTED;
}

LONG  W32kGetBitmapBits(HBITMAP  hBitmap,
                        LONG  Buffer,
                        LPVOID  Bits)
{
  UNIMPLEMENTED;
}

BOOL  W32kGetBitmapDimensionEx(HBITMAP  hBitmap,
                               LPSIZE  Dimension)
{
  UNIMPLEMENTED;
}

UINT  W32kGetDIBColorTable(HDC  hDC,
                           UINT  StartIndex,
                           UINT  Entries,
                           RGBQUAD  *Colors)
{
  UNIMPLEMENTED;
}

INT  W32kGetDIBits(HDC  hDC,
                   HBITMAP hBitmap,
                   UINT  StartScan,
                   UINT  ScanLines,
                   LPVOID  Bits,
                   LPBITMAPINFO   bi,
                   UINT  Usage)
{
  UNIMPLEMENTED;
}

COLORREF  W32kGetPixel(HDC  hDC,
                       INT  XPos,
                       INT  YPos)
{
  UNIMPLEMENTED;
}

BOOL  W32kMaskBlt(HDC  hDCDest,
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
  UNIMPLEMENTED;
}

BOOL W32kPlgBlt(HDC  hDCDest,
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
  UNIMPLEMENTED;
}

LONG  W32kSetBitmapBits(HBITMAP  hBitmap,
                        DWORD  Bytes,
                        CONST VOID *Bits)
{
  UNIMPLEMENTED;
}

BOOL  W32kSetBitmapDimensionEx(HBITMAP  hBitmap,
                               INT  Width,
                               INT  Height,
                               LPSIZE  Size)
{
  UNIMPLEMENTED;
}

UINT  W32kSetDIBColorTable(HDC  hDC,
                           UINT  StartIndex,
                           UINT  Entries,
                           CONST RGBQUAD  *Colors)
{
  UNIMPLEMENTED;
}

INT  W32kSetDIBits(HDC  hDC,
                   HBITMAP  hBitmap,
                   UINT  StartScan,
                   UINT  ScanLines,
                   CONST VOID  *Bits,
                   CONST BITMAPINFO  *bmi,
                   UINT  ColorUse)
{
  UNIMPLEMENTED;
}

INT  W32kSetDIBitsToDevice(HDC  hDC,
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
}

COLORREF  W32kSetPixel(HDC  hDC,
                       INT  X,
                       INT  Y,
                       COLORREF  Color)
{
  UNIMPLEMENTED;
}

BOOL  W32kSetPixelV(HDC  hDC,
                    INT  X,
                    INT  Y,
                    COLORREF  Color)
{
  UNIMPLEMENTED;
}

BOOL  W32kStretchBlt(HDC  hDCDest,
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
{
  UNIMPLEMENTED;
}

INT  W32kStretchDIBits(HDC  hDC,
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

