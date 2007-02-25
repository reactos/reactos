#ifndef _WIN32K_PATH_H
#define _WIN32K_PATH_H

#define PATH_IsPathOpen(path) ((path).state==PATH_Open)

BOOL FASTCALL PATH_Arc (PDC dc, INT x1, INT y1, INT x2, INT y2, INT xStart, INT yStart, INT xEnd, INT yEnd, INT lines);
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
BOOL FASTCALL PATH_RoundRect(DC *dc, INT x1, INT y1, INT x2, INT y2, INT ell_width, INT ell_height);
BOOL FASTCALL PATH_PathToRegion (GdiPath *pPath, INT nPolyFillMode, HRGN *pHrgn);

VOID FASTCALL IntGdiCloseFigure(PDC pDc);

#endif /* _WIN32K_PATH_H */
