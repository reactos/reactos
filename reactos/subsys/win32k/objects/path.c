

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/brush.h>

// #define NDEBUG
#include <internal/debug.h>

BOOL  W32kAbortPath(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL  W32kBeginPath(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL  W32kCloseFigure(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL  W32kEndPath(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL  W32kFillPath(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL  W32kFlattenPath(HDC  hDC)
{
  UNIMPLEMENTED;
}


BOOL  W32kGetMiterLimit(HDC  hDC,
                        PFLOAT  Limit)
{
  UNIMPLEMENTED;
}

INT  W32kGetPath(HDC  hDC,
                 LPPOINT  Points,
                 LPBYTE  Types,
                 INT  nSize)
{
  UNIMPLEMENTED;
}

HRGN  W32kPathToRegion(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL  W32kSetMiterLimit(HDC  hDC,
                        FLOAT  NewLimit,
                        PFLOAT  OldLimit)
{
  UNIMPLEMENTED;
}

BOOL  W32kStrokeAndFillPath(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL  W32kStrokePath(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL  W32kWidenPath(HDC  hDC)
{
  UNIMPLEMENTED;
}


