

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <win32k/line.h>

// #define NDEBUG
#include <internal/debug.h>

BOOL  W32kAngleArc(HDC  hDC,
                   int  X,
                   int  Y,
                   DWORD  Radius,
                   FLOAT  StartAngle,
                   FLOAT  SweepAngle)
{
  UNIMPLEMENTED;
}

BOOL  W32kArc(HDC  hDC,
              int  LeftRect,
              int  TopRect,
              int  RightRect, 
              int  BottomRect,
              int  XStartArc,
              int  YStartArc,
              int  XEndArc,  
              int  YEndArc)
{
  UNIMPLEMENTED;
}

BOOL  W32kArcTo(HDC  hDC,
                int  LeftRect,
                int  TopRect,
                int  RightRect,
                int  BottomRect,
                int  XRadial1,
                int  YRadial1,
                int  XRadial2,
                int  YRadial2)
{
  UNIMPLEMENTED;
}

int  W32kGetArcDirection(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL  W32kLineTo(HDC  hDC,
                 int  XEnd,
                 int  YEnd)
{
  UNIMPLEMENTED;
}

BOOL  W32kMoveToEx(HDC  hDC,
                   int  X,
                   int  Y,
                   LPPOINT  Point)
{
  UNIMPLEMENTED;
}

BOOL  W32kPolyBezier(HDC  hDC,
                     CONST LPPOINT  pt,
                     DWORD  Count)
{
  UNIMPLEMENTED;
}

BOOL  W32kPolyBezierTo(HDC  hDC,
                       CONST LPPOINT  pt,
                       DWORD  Count)
{
  UNIMPLEMENTED;
}

BOOL  W32kPolyDraw(HDC  hDC,
                   CONST LPPOINT  pt,
                   CONST LPBYTE  Types,
                   int  Count)
{
  UNIMPLEMENTED;
}

BOOL  W32kPolyline(HDC  hDC,
                   CONST LPPOINT  pt,
                   int  Count)
{
  UNIMPLEMENTED;
}

BOOL  W32kPolylineTo(HDC  hDC,
                     CONST LPPOINT  pt,
                     DWORD  Count)
{
  UNIMPLEMENTED;
}

BOOL  W32kPolyPolyline(HDC  hDC,
                       CONST LPPOINT  pt,
                       CONST LPDWORD  PolyPoints,
                       DWORD  Count)
{
  UNIMPLEMENTED;
}

int  W32kSetArcDirection(HDC  hDC,
                         int  ArcDirection)
{
  UNIMPLEMENTED;
}

