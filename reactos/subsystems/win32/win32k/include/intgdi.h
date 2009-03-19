#ifndef _WIN32K_INTGDI_H
#define _WIN32K_INTGDI_H

#include "region.h"

/* Brush functions */

extern HDC hSystemBM;
extern HSEMAPHORE hsemDriverMgmt;

XLATEOBJ* FASTCALL
IntGdiCreateBrushXlate(PDC Dc, GDIBRUSHOBJ *BrushObj, BOOLEAN *Failed);

XLATEOBJ*
FASTCALL
IntCreateXlateForBlt(PDC pDCDest, PDC pDCSrc, SURFACE* pDestSurf, SURFACE* pSrcSurf);

VOID FASTCALL
IntGdiInitBrushInstance(GDIBRUSHINST *BrushInst, PGDIBRUSHOBJ BrushObj, XLATEOBJ *XlateObj);

HBRUSH APIENTRY
IntGdiCreateDIBBrush(
   CONST BITMAPINFO *BitmapInfo,
   UINT ColorSpec,
   UINT BitmapInfoSize,
   CONST VOID *PackedDIB);

HBRUSH APIENTRY
IntGdiCreateHatchBrush(
   INT Style,
   COLORREF Color);

HBRUSH APIENTRY
IntGdiCreatePatternBrush(
   HBITMAP hBitmap);

HBRUSH APIENTRY
IntGdiCreateSolidBrush(
   COLORREF Color);

HBRUSH APIENTRY
IntGdiCreateNullBrush(VOID);

BOOL FASTCALL
IntPatBlt(
   PDC dc,
   INT XLeft,
   INT YLeft,
   INT Width,
   INT Height,
   DWORD ROP,
   PGDIBRUSHOBJ BrushObj);

VOID FASTCALL
IntGdiSetSolidBrushColor(HBRUSH hBrush, COLORREF Color);

/* Pen functions */

HPEN APIENTRY
IntGdiExtCreatePen(DWORD, DWORD, IN ULONG, IN ULONG, IN ULONG_PTR, IN ULONG_PTR, DWORD, PULONG, IN ULONG, IN BOOL, IN OPTIONAL HBRUSH);

VOID FASTCALL
IntGdiSetSolidPenColor(HPEN hPen, COLORREF Color);

/* Line functions */

BOOL FASTCALL
IntGdiLineTo(DC  *dc,
             int XEnd,
             int YEnd);

BOOL FASTCALL
IntGdiMoveToEx(DC      *dc,
               int     X,
               int     Y,
               LPPOINT Point);

BOOL FASTCALL
IntGdiPolyBezier(DC      *dc,
                 LPPOINT pt,
                 DWORD   Count);

BOOL FASTCALL
IntGdiPolyline(DC      *dc,
               LPPOINT pt,
               int     Count);

BOOL FASTCALL
IntGdiPolyBezierTo(DC      *dc,
                   LPPOINT pt,
                   DWORD   Count);

BOOL FASTCALL
IntGdiPolyPolyline(DC      *dc,
                   LPPOINT pt,
                   PULONG PolyPoints,
                   DWORD   Count);

BOOL FASTCALL
IntGdiPolylineTo(DC      *dc,
                 LPPOINT pt,
                 DWORD   Count);

BOOL FASTCALL
IntGdiArc(DC  *dc,
          int LeftRect,
          int TopRect,
          int RightRect,
          int BottomRect,
          int XStartArc,
          int YStartArc,
          int XEndArc,
          int YEndArc);

INT FASTCALL
IntGdiGetArcDirection(DC *dc);

/* Shape functions */

BOOL FASTCALL
IntGdiPolygon(PDC    dc,
              PPOINT UnsafePoints,
              int    Count);

BOOL FASTCALL
IntGdiPolyPolygon(DC      *dc,
                  LPPOINT Points,
                  PULONG  PolyCounts,
                  int     Count);

BOOL FASTCALL IntGdiGradientFill(DC *dc,
    PTRIVERTEX pVertex,
    ULONG uVertex,
    PVOID pMesh, ULONG uMesh, ULONG ulMode);

/* DC functions */

BOOL FASTCALL
IntGdiGetDCOrg(PDC pDC, PPOINTL pPointl);

INT FASTCALL
IntGdiGetObject(HANDLE handle, INT count, LPVOID buffer);

HDC FASTCALL
IntGdiCreateDC(PUNICODE_STRING Driver,
               PUNICODE_STRING Device,
               PVOID pUMdhpdev,
               CONST PDEVMODEW InitData,
               BOOL CreateAsIC);

/* Coord functions */

BOOL FASTCALL
IntGdiCombineTransform(LPXFORM XFormResult,
                       LPXFORM xform1,
                       LPXFORM xform2);

/* Stock objects */

BOOL FASTCALL
IntSetSysColors(UINT nColors, INT *Elements, COLORREF *Colors);

BOOL FASTCALL
IntGetSysColorBrushes(HBRUSH *Brushes, UINT nBrushes);

HGDIOBJ FASTCALL
IntGetSysColorBrush(INT Object);

BOOL FASTCALL
IntGetSysColorPens(HPEN *Pens, UINT nPens);

BOOL FASTCALL
IntGetSysColors(COLORREF *Colors, UINT nColors);

DWORD FASTCALL
IntGetSysColor(INT nIndex);

/* Other Stuff */

INT FASTCALL
IntGdiGetDeviceCaps(PDC dc, INT Index);

INT
FASTCALL
IntGdiEscape(PDC    dc,
             INT    Escape,
             INT    InSize,
             LPCSTR InData,
             LPVOID OutData);

NTSTATUS
FASTCALL
IntEnumDisplaySettings(
  IN PUNICODE_STRING pDeviceName  OPTIONAL,
  IN DWORD iModeNum,
  IN OUT LPDEVMODEW pDevMode,
  IN DWORD dwFlags);

LONG
FASTCALL
IntChangeDisplaySettings(
  IN PUNICODE_STRING pDeviceName  OPTIONAL,
  IN LPDEVMODEW pDevMode,
  IN DWORD dwflags,
  IN PVOID lParam  OPTIONAL);

HBITMAP
FASTCALL
IntCreateCompatibleBitmap(PDC Dc,
                          INT Width,
                          INT Height);

HBITMAP APIENTRY
IntGdiCreateBitmap(
    INT  Width,
    INT  Height,
    UINT  Planes,
    UINT  BitsPixel,
    IN OPTIONAL LPBYTE pBits);

HDC APIENTRY IntGdiGetDCState(HDC  hDC);

WORD APIENTRY IntGdiSetHookFlags(HDC hDC, WORD Flags);

VOID APIENTRY IntGdiSetDCState ( HDC hDC, HDC hDCSave );

LONG APIENTRY IntSetBitmapBits(PSURFACE bmp, DWORD  Bytes, IN PBYTE Bits);

LONG APIENTRY IntGetBitmapBits(PSURFACE bmp, DWORD Bytes, OUT PBYTE Bits);

UINT APIENTRY IntSetDIBColorTable(HDC hDC, UINT StartIndex, UINT Entries, CONST RGBQUAD *Colors);

UINT APIENTRY IntGetDIBColorTable(HDC hDC, UINT StartIndex, UINT Entries, RGBQUAD *Colors);

UINT APIENTRY
IntAnimatePalette(HPALETTE hPal, UINT StartIndex,
   UINT NumEntries, CONST PPALETTEENTRY PaletteColors);

UINT APIENTRY
IntGetPaletteEntries(HPALETTE  hpal,
                     UINT  StartIndex,
                     UINT  Entries,
                     LPPALETTEENTRY  pe);

UINT APIENTRY
IntSetPaletteEntries(HPALETTE  hpal,
                      UINT  Start,
                      UINT  Entries,
                      CONST LPPALETTEENTRY  pe);

UINT APIENTRY
IntGetSystemPaletteEntries(HDC  hDC,
                           UINT  StartIndex,
                           UINT  Entries,
                           LPPALETTEENTRY  pe);
UINT APIENTRY
IntGetDIBColorTable(HDC hDC, UINT StartIndex, UINT Entries, RGBQUAD *Colors);

UINT APIENTRY
IntSetDIBColorTable(HDC hDC, UINT StartIndex, UINT Entries, CONST RGBQUAD *Colors);

#endif /* _WIN32K_INTGDI_H */

