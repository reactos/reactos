#ifndef __WIN32K_PATH_H
#define __WIN32K_PATH_H

BOOL STDCALL W32kAbortPath(HDC  hDC);

BOOL STDCALL W32kBeginPath(HDC  hDC);

BOOL STDCALL W32kCloseFigure(HDC  hDC);

BOOL STDCALL W32kEndPath(HDC  hDC);

BOOL STDCALL W32kFillPath(HDC  hDC);

BOOL STDCALL W32kFlattenPath(HDC  hDC);

BOOL STDCALL W32kGetMiterLimit(HDC  hDC,
                        PFLOAT  Limit);

INT STDCALL W32kGetPath(HDC  hDC,
                 LPPOINT  Points,
                 LPBYTE  Types,
                 INT  nSize);

HRGN STDCALL W32kPathToRegion(HDC  hDC);

BOOL STDCALL W32kSetMiterLimit(HDC  hDC,
                        FLOAT  NewLimit,
                        PFLOAT  OldLimit);

BOOL STDCALL W32kStrokeAndFillPath(HDC  hDC);

BOOL STDCALL W32kStrokePath(HDC  hDC);

BOOL STDCALL W32kWidenPath(HDC  hDC);

#endif
