

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/dc.h>
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
  PDC  dc;
  int  ret;
  
  dc = DC_HandleToPtr (hDC);
  if (!dc)
    {
      return 0;
    }

  ret = dc->w.ArcDirection;
  DC_UnlockDC (hDC);
  
  return ret;
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
  PDC  dc;
  INT  nOldDirection;

  dc = DC_HandleToPtr (hDC);
  if (!dc)
    {
      return 0;
    }
  if (ArcDirection != AD_COUNTERCLOCKWISE && ArcDirection != AD_CLOCKWISE)
    {
//      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
    }

  nOldDirection = dc->w.ArcDirection;
  dc->w.ArcDirection = ArcDirection;

  return nOldDirection;
}

