#ifndef _WIN32K_PATH_H
#define _WIN32K_PATH_H

BOOL INTERNAL_CALL PATH_Arc (PDC dc, INT x1, INT y1, INT x2, INT y2, INT xStart, INT yStart, INT xEnd, INT yEnd);
BOOL INTERNAL_CALL PATH_AssignGdiPath (GdiPath *pPathDest, const GdiPath *pPathSrc);
VOID INTERNAL_CALL PATH_DestroyGdiPath (GdiPath *pPath);
BOOL INTERNAL_CALL PATH_Ellipse (PDC dc, INT x1, INT y1, INT x2, INT y2);
VOID INTERNAL_CALL PATH_EmptyPath (GdiPath *pPath);
VOID INTERNAL_CALL PATH_InitGdiPath (GdiPath *pPath);
BOOL INTERNAL_CALL PATH_LineTo (PDC dc, INT x, INT y);
BOOL INTERNAL_CALL PATH_MoveTo (PDC dc);
BOOL INTERNAL_CALL PATH_PolyBezier (PDC dc, const POINT *pts, DWORD cbPoints);
BOOL INTERNAL_CALL PATH_PolyBezierTo (PDC dc, const POINT *pts, DWORD cbPoints);
BOOL INTERNAL_CALL PATH_Polygon (PDC dc, const POINT *pts, DWORD cbPoints);
BOOL INTERNAL_CALL PATH_Polyline (PDC dc, const POINT *pts, DWORD cbPoints);
BOOL INTERNAL_CALL PATH_PolylineTo (PDC dc, const POINT *pts, DWORD cbPoints);
BOOL INTERNAL_CALL PATH_PolyPolygon ( PDC dc, const POINT* pts, const INT* counts, UINT polygons);
BOOL INTERNAL_CALL PATH_PolyPolyline( PDC dc, const POINT* pts, const DWORD* counts, DWORD polylines);
BOOL INTERNAL_CALL PATH_Rectangle (PDC dc, INT x1, INT y1, INT x2, INT y2);
BOOL INTERNAL_CALL PATH_RoundRect (PDC dc, INT x1, INT y1, INT x2, INT y2, INT xradius, INT yradius);
BOOL INTERNAL_CALL PATH_PathToRegion (const GdiPath *pPath, INT nPolyFillMode, HRGN *pHrgn);

#endif /* _WIN32K_PATH_H */
