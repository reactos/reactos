
#ifndef __WIN32K_COORD_H
#define __WIN32K_COORD_H

BOOL  W32kCombineTransform(LPXFORM  XformResult,
                           CONST LPXFORM  xform1,
                           CONST LPXFORM  xform2);

BOOL  W32kDPtoLP(HDC  hDC,
                 LPPOINT  Points,
                 int  Count);

int  W32kGetGraphicsMode(HDC  hDC);

BOOL  W32kGetWorldTransform(HDC  hDC,
                            LPXFORM  Xform);

BOOL  W32kLPtoDP(HDC  hDC,
                 LPPOINT  Points,
                 int  Count);

BOOL  W32kModifyWorldTransform(HDC  hDC,
                               CONST LPXFORM  Xform,
                               DWORD  Mode);

BOOL  W32kOffsetViewportOrgEx(HDC  hDC,
                              int  XOffset,
                              int  YOffset,
                              LPPOINT  Point);

BOOL  W32kOffsetWindowOrgEx(HDC  hDC,
                            int  XOffset,
                            int  YOffset,
                            LPPOINT  Point);

BOOL  W32kScaleViewportExtEx(HDC  hDC,
                             int  Xnum,
                             int  Xdenom,
                             int  Ynum,
                             int  Ydenom,
                             LPSIZE  Size);

BOOL  W32kScaleWindowExtEx(HDC  hDC,
                           int  Xnum,
                           int  Xdenom,
                           int  Ynum,
                           int  Ydenom,
                           LPSIZE  Size);

int  W32kSetGraphicsMode(HDC  hDC,
                         int  Mode);

int  W32kSetMapMode(HDC  hDC,
                    int  MapMode);

BOOL  W32kSetViewportExtEx(HDC  hDC,
                           int  XExtent,
                           int  YExtent,
                           LPSIZE  Size);

BOOL  W32kSetViewportOrgEx(HDC  hDC,
                           int  X,
                           int  Y,
                           LPPOINT  Point);

BOOL  W32kSetWindowExtEx(HDC  hDC,
                         int  XExtent,
                         int  YExtent,
                         LPSIZE  Size);

BOOL  W32kSetWindowOrgEx(HDC  hDC,
                         int  X,
                         int  Y,
                         LPPOINT  Point);

BOOL  W32kSetWorldTransform(HDC  hDC,
                            CONST LPXFORM  Xform);

#endif

