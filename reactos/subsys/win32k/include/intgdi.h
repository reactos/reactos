#ifndef _WIN32K_INTGDI_H
#define _WIN32K_INTGDI_H

/* Brush functions */

XLATEOBJ* INTERNAL_CALL
IntGdiCreateBrushXlate(PDC Dc, GDIBRUSHOBJ *BrushObj, BOOLEAN *Failed);

VOID INTERNAL_CALL
IntGdiInitBrushInstance(GDIBRUSHINST *BrushInst, PGDIBRUSHOBJ BrushObj, XLATEOBJ *XlateObj);

HBRUSH INTERNAL_CALL
IntGdiCreateBrushIndirect(PLOGBRUSH lb);

HBRUSH INTERNAL_CALL
IntGdiCreateDIBPatternBrush(HGLOBAL hDIBPacked,
                            UINT    ColorSpec);

HBRUSH INTERNAL_CALL
IntGdiCreateDIBPatternBrushPt(CONST VOID *PackedDIB,
                              UINT       Usage);

BOOL INTERNAL_CALL
IntPatBlt(
   PDC dc,
   INT XLeft,
   INT YLeft,
   INT Width,
   INT Height,
   DWORD ROP,
   PGDIBRUSHOBJ BrushObj);

/* Pen functions */

HPEN INTERNAL_CALL
IntGdiCreatePenIndirect(PLOGPEN lgpn);

/* Line functions */

BOOL INTERNAL_CALL
IntGdiLineTo(DC  *dc,
             int XEnd,
             int YEnd);

BOOL INTERNAL_CALL
IntGdiMoveToEx(DC      *dc,
               int     X,
               int     Y,
               LPPOINT Point);

BOOL INTERNAL_CALL
IntGdiPolyBezier(DC      *dc,
                 LPPOINT pt,
                 DWORD   Count);

BOOL INTERNAL_CALL
IntGdiPolyline(DC      *dc,
               LPPOINT pt,
               int     Count);

BOOL INTERNAL_CALL
IntGdiPolyBezierTo(DC      *dc,
                   LPPOINT pt,
                   DWORD   Count);

BOOL INTERNAL_CALL
IntGdiPolyPolyline(DC      *dc,
                   LPPOINT pt,
                   LPDWORD PolyPoints,
                   DWORD   Count);

BOOL INTERNAL_CALL
IntGdiPolylineTo(DC      *dc,
                 LPPOINT pt,
                 DWORD   Count);

BOOL INTERNAL_CALL
IntGdiArc(DC  *dc,
          int LeftRect,
          int TopRect,
          int RightRect,
          int BottomRect,
          int XStartArc,
          int YStartArc,
          int XEndArc,
          int YEndArc);

INT INTERNAL_CALL
IntGdiGetArcDirection(DC *dc);

/* Shape functions */

BOOL INTERNAL_CALL
IntGdiPolygon(PDC    dc,
              PPOINT UnsafePoints,
              int    Count);

BOOL INTERNAL_CALL
IntGdiPolyPolygon(DC      *dc,
                  LPPOINT Points,
                  LPINT   PolyCounts,
                  int     Count);

/* Rgn functions */

int INTERNAL_CALL
IntGdiGetClipBox(HDC    hDC,
			     LPRECT rc);

HRGN INTERNAL_CALL REGION_CropRgn(HRGN hDst, HRGN hSrc, const PRECT lpRect, PPOINT lpPt);
void INTERNAL_CALL REGION_UnionRectWithRegion(const RECT *rect, ROSRGNDATA *rgn);
INT INTERNAL_CALL UnsafeIntGetRgnBox(PROSRGNDATA Rgn, LPRECT pRect);
BOOL INTERNAL_CALL UnsafeIntRectInRegion(PROSRGNDATA Rgn, CONST LPRECT rc);

#define UnsafeIntCreateRectRgnIndirect(prc) \
  NtGdiCreateRectRgn((prc)->left, (prc)->top, (prc)->right, (prc)->bottom)

#define UnsafeIntUnionRectWithRgn(rgndest, prc) \
  REGION_UnionRectWithRegion((prc), (rgndest))

/* DC functions */

BOOL INTERNAL_CALL
IntGdiGetDCOrgEx(DC *dc, LPPOINT  Point);

INT INTERNAL_CALL
IntGdiGetObject(HANDLE handle, INT count, LPVOID buffer);

HDC INTERNAL_CALL
IntGdiCreateDC(PUNICODE_STRING Driver,
               PUNICODE_STRING Device,
               PUNICODE_STRING Output,
               CONST PDEVMODEW InitData);

COLORREF INTERNAL_CALL
IntGetDCColor(HDC hDC, ULONG Object);

COLORREF INTERNAL_CALL
IntSetDCColor(HDC hDC, ULONG Object, COLORREF Color);

/* Coord functions */

BOOL INTERNAL_CALL
IntGdiCombineTransform(LPXFORM XFormResult,
                       LPXFORM xform1,
                       LPXFORM xform2);

/* Stock objects */

BOOL INTERNAL_CALL
IntSetSysColors(UINT nColors, INT *Elements, COLORREF *Colors);

BOOL INTERNAL_CALL
IntGetSysColorBrushes(HBRUSH *Brushes, UINT nBrushes);

BOOL INTERNAL_CALL
IntGetSysColorPens(HPEN *Pens, UINT nPens);

BOOL INTERNAL_CALL
IntGetSysColors(COLORREF *Colors, UINT nColors);

#endif /* _WIN32K_INTGDI_H */

