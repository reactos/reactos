#ifndef _WIN32K_INTGDI_H
#define _WIN32K_INTGDI_H

/* Brush functions */

HBRUSH FASTCALL
IntGdiCreateBrushIndirect(PLOGBRUSH lb);

HBRUSH FASTCALL
IntGdiCreateDIBPatternBrush(HGLOBAL hDIBPacked,
                            UINT    ColorSpec);

HBRUSH FASTCALL
IntGdiCreateDIBPatternBrushPt(CONST VOID *PackedDIB,
                              UINT       Usage);

BOOL FASTCALL
IntPatBlt(
   PDC dc,
   INT XLeft,
   INT YLeft,
   INT Width,
   INT Height,
   DWORD ROP,
   PGDIBRUSHOBJ BrushObj);

/* Pen functions */

HPEN FASTCALL
IntGdiCreatePenIndirect(PLOGPEN lgpn);

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
                   LPDWORD PolyPoints,
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
                  LPINT   PolyCounts,
                  int     Count);

/* Rgn functions */

int FASTCALL
IntGdiGetClipBox(HDC    hDC,
			     LPRECT rc);

HRGN FASTCALL REGION_CropRgn(HRGN hDst, HRGN hSrc, const PRECT lpRect, PPOINT lpPt);

HRGN FASTCALL UnsafeIntCreateRectRgnIndirect(CONST PRECT rc);
INT FASTCALL UnsafeIntGetRgnBox(PROSRGNDATA Rgn, LPRECT pRect);
VOID FASTCALL UnsafeIntUnionRectWithRgn(PROSRGNDATA RgnDest, CONST PRECT Rect);
BOOL FASTCALL UnsafeIntRectInRegion(PROSRGNDATA Rgn, CONST LPRECT rc);

/* DC functions */

BOOL FASTCALL
IntGdiGetDCOrgEx(DC *dc, LPPOINT  Point);

INT FASTCALL
IntGdiGetObject(HANDLE handle, INT count, LPVOID buffer);

HDC FASTCALL
IntGdiCreateDC(PUNICODE_STRING Driver,
               PUNICODE_STRING Device,
               PUNICODE_STRING Output,
               CONST PDEVMODEW InitData);

COLORREF FASTCALL
IntGetDCColor(HDC hDC, ULONG Object);

COLORREF FASTCALL
IntSetDCColor(HDC hDC, ULONG Object, COLORREF Color);

/* Coord functions */

BOOL FASTCALL
IntGdiCombineTransform(LPXFORM XFormResult,
                       LPXFORM xform1,
                       LPXFORM xform2);

#endif /* _WIN32K_INTGDI_H */

