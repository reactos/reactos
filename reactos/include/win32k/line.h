#ifndef __WIN32K_LINE_H
#define __WIN32K_LINE_H

BOOL
STDCALL
W32kAngleArc(HDC  hDC,
                   int  X,
                   int  Y,
                   DWORD  Radius,
                   FLOAT  StartAngle,
                   FLOAT  SweepAngle);

BOOL
STDCALL
W32kArc(HDC  hDC,
              int  LeftRect,
              int  TopRect,
              int  RightRect, 
              int  BottomRect,
              int  XStartArc,
              int  YStartArc,
              int  XEndArc,  
              int  YEndArc);

BOOL
STDCALL
W32kArcTo(HDC  hDC,
                int  LeftRect,
                int  TopRect,
                int  RightRect,
                int  BottomRect,
                int  XRadial1,
                int  YRadial1,
                int  XRadial2,
                int  YRadial2);

int
STDCALL
W32kGetArcDirection(HDC  hDC);

BOOL
STDCALL
W32kLineTo(HDC  hDC,
                 int  XEnd,
                 int  YEnd);

BOOL
STDCALL
W32kMoveToEx(HDC  hDC,
                   int  X,
                   int  Y,
                   LPPOINT  Point);

BOOL
STDCALL
W32kPolyBezier(HDC  hDC,
                     CONST LPPOINT  pt,
                     DWORD  Count);

BOOL
STDCALL
W32kPolyBezierTo(HDC  hDC,
                       CONST LPPOINT  pt,
                       DWORD  Count);

BOOL
STDCALL
W32kPolyDraw(HDC  hDC,
                   CONST LPPOINT  pt,
                   CONST LPBYTE  Types,
                   int  Count);

BOOL
STDCALL
W32kPolyline(HDC  hDC,
                   CONST LPPOINT  pt,
                   int  Count);

BOOL
STDCALL
W32kPolylineTo(HDC  hDC,
                     CONST LPPOINT  pt,
                     DWORD  Count);

BOOL
STDCALL
W32kPolyPolyline(HDC  hDC,
                       CONST LPPOINT  pt,
                       CONST LPDWORD  PolyPoints,
                       DWORD  Count);

int
STDCALL
W32kSetArcDirection(HDC  hDC,
                         int  ArcDirection);

#endif
