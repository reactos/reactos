
#ifndef __WIN32K_LINE_H
#define __WIN32K_LINE_H

BOOL  W32kAngleArc(HDC  hDC,
                   int  X,
                   int  Y,
                   DWORD  Radius,
                   FLOAT  StartAngle,
                   FLOAT  SweepAngle);

BOOL  W32kArc(HDC  hDC,
              int  LeftRect,
              int  TopRect,
              int  RightRect, 
              int  BottomRect,
              int  XStartArc,
              int  YStartArc,
              int  XEndArc,  
              int  YEndArc);

BOOL  W32kArcTo(HDC  hDC,
                int  LeftRect,
                int  TopRect,
                int  RightRect,
                int  BottomRect,
                int  XRadial1,
                int  YRadial1,
                int  XRadial2,
                int  YRadial2);

int  W32kGetArcDirection(HDC  hDC);

BOOL  W32kLineTo(HDC  hDC,
                 int  XEnd,
                 int  YEnd);

BOOL  W32kMoveToEx(HDC  hDC,
                   int  X,
                   int  Y,
                   LPPOINT  Point);

BOOL  W32kPolyBezier(HDC  hDC,
                     CONST LPPOINT  pt,
                     DWORD  Count);

BOOL  W32kPolyBezierTo(HDC  hDC,
                       CONST LPPOINT  pt,
                       DWORD  Count);

BOOL  W32kPolyDraw(HDC  hDC,
                   CONST LPPOINT  pt,
                   CONST LPBYTE  Types,
                   int  Count);

BOOL  W32kPolyline(HDC  hDC,
                   CONST LPPOINT  pt,
                   int  Count);

BOOL  W32kPolylineTo(HDC  hDC,
                     CONST LPPOINT  pt,
                     DWORD  Count);

BOOL  W32kPolyPolyline(HDC  hDC,
                       CONST LPPOINT  pt,
                       CONST LPDWORD  PolyPoints,
                       DWORD  Count);

int  W32kSetArcDirection(HDC  hDC,
                         int  ArcDirection);

#endif
