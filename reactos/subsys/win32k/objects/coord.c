

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <win32k/coord.h>

// #define NDEBUG
#include <internal/debug.h>

BOOL  W32kCombineTransform(LPXFORM  XformResult,
                           CONST LPXFORM  xform1,
                           CONST LPXFORM  xform2)
{
  UNIMPLEMENTED;
}

BOOL  W32kDPtoLP(HDC  hDC,
                 LPPOINT  Points,
                 int  Count)
{
  UNIMPLEMENTED;
}

int  W32kGetGraphicsMode(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL  W32kGetWorldTransform(HDC  hDC,
                            LPXFORM  Xform)
{
  UNIMPLEMENTED;
}

BOOL  W32kLPtoDP(HDC  hDC,
                 LPPOINT  Points,
                 int  Count)
{
  UNIMPLEMENTED;
}

BOOL  W32kModifyWorldTransform(HDC  hDC,
                               CONST LPXFORM  Xform,
                               DWORD  Mode)
{
  UNIMPLEMENTED;
}

BOOL  W32kOffsetViewportOrgEx(HDC  hDC,
                              int  XOffset,
                              int  YOffset,
                              LPPOINT  Point)
{
  UNIMPLEMENTED;
}

BOOL  W32kOffsetWindowOrgEx(HDC  hDC,
                            int  XOffset,
                            int  YOffset,
                            LPPOINT  Point)
{
  UNIMPLEMENTED;
}

BOOL  W32kScaleViewportExtEx(HDC  hDC,
                             int  Xnum,
                             int  Xdenom,
                             int  Ynum,
                             int  Ydenom,
                             LPSIZE  Size)
{
  UNIMPLEMENTED;
}

BOOL  W32kScaleWindowExtEx(HDC  hDC,
                           int  Xnum,
                           int  Xdenom,
                           int  Ynum,
                           int  Ydenom,
                           LPSIZE  Size)
{
  UNIMPLEMENTED;
}

int  W32kSetGraphicsMode(HDC  hDC,
                         int  Mode)
{
  UNIMPLEMENTED;
}

int  W32kSetMapMode(HDC  hDC,
                    int  MapMode)
{
  UNIMPLEMENTED;
}

BOOL  W32kSetViewportExtEx(HDC  hDC,
                           int  XExtent,
                           int  YExtent,
                           LPSIZE  Size)
{
  UNIMPLEMENTED;
}

BOOL  W32kSetViewportOrgEx(HDC  hDC,
                           int  X,
                           int  Y,
                           LPPOINT  Point)
{
  UNIMPLEMENTED;
}

BOOL  W32kSetWindowExtEx(HDC  hDC,
                         int  XExtent,
                         int  YExtent,
                         LPSIZE  Size)
{
  UNIMPLEMENTED;
}

BOOL  W32kSetWindowOrgEx(HDC  hDC,
                         int  X,
                         int  Y,
                         LPPOINT  Point)
{
  UNIMPLEMENTED;
}

BOOL  W32kSetWorldTransform(HDC  hDC,
                            CONST LPXFORM  Xform)
{
  UNIMPLEMENTED;
}


