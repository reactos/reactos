#ifndef _WIN32K_PATH_H
#define _WIN32K_PATH_H

BOOL FASTCALL PATH_Arc (PDC dc, INT x1, INT y1, INT x2, INT y2, INT xStart, INT yStart, INT xEnd, INT yEnd);
BOOL FASTCALL PATH_AssignGdiPath (GdiPath *pPathDest, const GdiPath *pPathSrc);
VOID FASTCALL PATH_DestroyGdiPath (GdiPath *pPath);
BOOL FASTCALL PATH_Ellipse (PDC dc, INT x1, INT y1, INT x2, INT y2);
VOID FASTCALL PATH_EmptyPath (GdiPath *pPath);
VOID FASTCALL PATH_InitGdiPath (GdiPath *pPath);
BOOL FASTCALL PATH_LineTo (PDC dc, INT x, INT y);
BOOL FASTCALL PATH_MoveTo (PDC dc);
BOOL FASTCALL PATH_PolyBezier (PDC dc, const POINT *pts, DWORD cbPoints);
BOOL FASTCALL PATH_PolyBezierTo (PDC dc, const POINT *pts, DWORD cbPoints);
BOOL FASTCALL PATH_Polygon (PDC dc, const POINT *pts, DWORD cbPoints);
BOOL FASTCALL PATH_Polyline (PDC dc, const POINT *pts, DWORD cbPoints);
BOOL FASTCALL PATH_PolylineTo (PDC dc, const POINT *pts, DWORD cbPoints);
BOOL FASTCALL PATH_PolyPolygon ( PDC dc, const POINT* pts, const INT* counts, UINT polygons);
BOOL FASTCALL PATH_PolyPolyline( PDC dc, const POINT* pts, const DWORD* counts, DWORD polylines);
BOOL FASTCALL PATH_Rectangle (PDC dc, INT x1, INT y1, INT x2, INT y2);
BOOL FASTCALL PATH_RoundRect (PDC dc, INT x1, INT y1, INT x2, INT y2, INT xradius, INT yradius);
BOOL FASTCALL PATH_PathToRegion (const GdiPath *pPath, INT nPolyFillMode, HRGN *pHrgn);

#endif /* _WIN32K_PATH_H */
