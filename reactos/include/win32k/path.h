
#ifndef __WIN32K_PATH_H
#define __WIN32K_PATH_H

BOOL  W32kAbortPath(HDC  hDC);

BOOL  W32kBeginPath(HDC  hDC);

BOOL  W32kCloseFigure(HDC  hDC);

BOOL  W32kEndPath(HDC  hDC);

BOOL  W32kFillPath(HDC  hDC);

BOOL  W32kFlattenPath(HDC  hDC);

BOOL  W32kGetMiterLimit(HDC  hDC,
                        PFLOAT  Limit);

INT  W32kGetPath(HDC  hDC,
                 LPPOINT  Points,
                 LPBYTE  Types,
                 INT  nSize);

HRGN  W32kPathToRegion(HDC  hDC);

BOOL  W32kSetMiterLimit(HDC  hDC,
                        FLOAT  NewLimit,
                        PFLOAT  OldLimit);

BOOL  W32kStrokeAndFillPath(HDC  hDC);

BOOL  W32kStrokePath(HDC  hDC);

BOOL  W32kWidenPath(HDC  hDC);



#endif

