#ifndef __WIN32K_LINE_H
#define __WIN32K_LINE_H

BOOL
STDCALL
NtGdiAngleArc(HDC  hDC,
                   int  X,
                   int  Y,
                   DWORD  Radius,
                   FLOAT  StartAngle,
                   FLOAT  SweepAngle);

BOOL
STDCALL
NtGdiArc(HDC  hDC,
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
NtGdiArcTo(HDC  hDC,
                int  LeftRect,
                int  TopRect,
                int  RightRect,
                int  BottomRect,
                int  XRadial1,
                int  YRadial1,
                int  XRadial2,
                int  YRadial2);

INT
FASTCALL
IntGetArcDirection ( PDC dc );

INT
STDCALL
NtGdiGetArcDirection ( HDC hDC );

BOOL
STDCALL
NtGdiLineTo(HDC  hDC,
           int  XEnd,
           int  YEnd );

BOOL
STDCALL
NtGdiMoveToEx(HDC  hDC,
                   int  X,
                   int  Y,
                   LPPOINT  Point);

BOOL
STDCALL
NtGdiPolyBezier(HDC  hDC,
                     CONST LPPOINT  pt,
                     DWORD  Count);

BOOL
STDCALL
NtGdiPolyBezierTo(HDC  hDC,
                       CONST LPPOINT  pt,
                       DWORD  Count);

BOOL
STDCALL
NtGdiPolyDraw(HDC  hDC,
                   CONST LPPOINT  pt,
                   CONST LPBYTE  Types,
                   int  Count);

BOOL
STDCALL
NtGdiPolyline(HDC  hDC,
                   CONST LPPOINT  pt,
                   int  Count);

BOOL
STDCALL
NtGdiPolylineTo(HDC  hDC,
                     CONST LPPOINT  pt,
                     DWORD  Count);

BOOL
STDCALL
NtGdiPolyPolyline(HDC  hDC,
                       CONST LPPOINT  pt,
                       CONST LPDWORD  PolyPoints,
                       DWORD  Count);

int
STDCALL
NtGdiSetArcDirection(HDC  hDC,
                         int  ArcDirection);

#endif
