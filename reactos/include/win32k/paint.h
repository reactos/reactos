
#ifndef __WIN32K_PAINT_H
#define __WIN32K_PAINT_H

BOOL  W32kGdiFlush(VOID);

DWORD  W32kGdiGetBatchLimit(VOID);

DWORD  W32kGdiSetBatchLimit(DWORD  Limit);

UINT  W32kGetBoundsRect(HDC  hDC,
                        LPRECT  Bounds,
                        UINT  Flags);

COLORREF  W32kSetBkColor(HDC  hDC,
                         COLORREF  Color);

UINT  W32kSetBoundsRect(HDC  hDC,
                        CONST PRECT  Bounds,
                        UINT  Flags);

BOOL  W32kAbortPath(HDC  hDC);

BOOL  W32kBeginPath(HDC  hDC);

BOOL  W32kCloseFigure(HDC  hDC);

BOOL  W32kEndPath(HDC  hDC);

BOOL  W32kFillPath(HDC  hDC);

BOOL  W32kFlattenPath(HDC  hDC);

BOOL  W32kGetMiterLimit(HDC  hDC, PFLOAT  Limit);

INT  W32kGetPath(HDC  hDC,
                 PPOINT  Points,
                 PBYTE  Types,
                 INT  Size);

HRGN  W32kPathToRegion(HDC  hDC);


BOOL  W32kSetMiterLimit(HDC  hDC,
                        FLOAT  NewLimit,
                        PFLOAT  OldLimit);


BOOL  W32kStrokeAndFillPath(HDC  hDC);

BOOL  W32kStrokePath(HDC  hDC);

BOOL  W32kWidenPath(HDC  hDC);

#endif

