BOOL STDCALL  PATH_Arc (HDC hdc, INT x1, INT y1, INT x2, INT y2, INT xStart, INT yStart, INT xEnd, INT yEnd);
BOOL FASTCALL PATH_AssignGdiPath (GdiPath *pPathDest, const GdiPath *pPathSrc);
VOID FASTCALL PATH_DestroyGdiPath (GdiPath *pPath);
BOOL STDCALL  PATH_Ellipse (HDC hdc, INT x1, INT y1, INT x2, INT y2);
VOID STDCALL  PATH_EmptyPath (GdiPath *pPath);
VOID FASTCALL PATH_InitGdiPath (GdiPath *pPath);
BOOL STDCALL  PATH_LineTo (HDC hdc, INT x, INT y);
BOOL FASTCALL PATH_MoveTo (HDC hdc);
BOOL STDCALL  PATH_PolyBezier (HDC hdc, const POINT *pts, DWORD cbPoints);
BOOL STDCALL  PATH_PolyBezierTo (HDC hdc, const POINT *pts, DWORD cbPoints);
BOOL STDCALL  PATH_Polygon (HDC hdc, const POINT *pts, DWORD cbPoints);
BOOL STDCALL  PATH_Polyline (HDC hdc, const POINT *pts, DWORD cbPoints);
BOOL STDCALL  PATH_PolylineTo (HDC hdc, const POINT *pts, DWORD cbPoints);
BOOL STDCALL  PATH_PolyPolygon ( HDC hdc, const POINT* pts, const INT* counts, UINT polygons);
BOOL STDCALL  PATH_PolyPolyline( HDC hdc, const POINT* pts, const DWORD* counts, DWORD polylines);
BOOL STDCALL  PATH_Rectangle (HDC hdc, INT x1, INT y1, INT x2, INT y2);
BOOL STDCALL  PATH_PathToRegion (const GdiPath *pPath, INT nPolyFillMode, HRGN *pHrgn);
#ifdef _WIN32K_PATH_INTERNAL
BOOL STDCALL  PATH_AddEntry (GdiPath *pPath, const POINT *pPoint, BYTE flags);
BOOL STDCALL  PATH_AddFlatBezier (GdiPath *pPath, POINT *pt, BOOL closed);
BOOL STDCALL  PATH_DoArcPart (GdiPath *pPath, FLOAT_POINT corners[], double angleStart, double angleEnd, BOOL addMoveTo);
BOOL FASTCALL PATH_FlattenPath (GdiPath *pPath);
BOOL FASTCALL PATH_GetPathFromHDC (HDC hdc, GdiPath **ppPath);
VOID STDCALL  PATH_NormalizePoint (FLOAT_POINT corners[], const FLOAT_POINT *pPoint, double *pX, double *pY);
BOOL STDCALL  PATH_PathToRegion(const GdiPath *pPath, INT nPolyFillMode, HRGN *pHrgn);
BOOL STDCALL  PATH_ReserveEntries (GdiPath *pPath, INT numEntries);
VOID STDCALL  PATH_ScaleNormalizedPoint (FLOAT_POINT corners[], double x, double y, POINT *pPoint);
#endif
