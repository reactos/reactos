

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/brush.h>

// #define NDEBUG
#include <internal/debug.h>

BOOL
STDCALL
W32kAbortPath(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kBeginPath(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kCloseFigure(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kEndPath(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kFillPath(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kFlattenPath(HDC  hDC)
{
  UNIMPLEMENTED;
}


BOOL
STDCALL
W32kGetMiterLimit(HDC  hDC,
                        PFLOAT  Limit)
{
  UNIMPLEMENTED;
}

INT
STDCALL
W32kGetPath(HDC  hDC,
                 LPPOINT  Points,
                 LPBYTE  Types,
                 INT  nSize)
{
  UNIMPLEMENTED;
}

HRGN
STDCALL
W32kPathToRegion(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kSetMiterLimit(HDC  hDC,
                        FLOAT  NewLimit,
                        PFLOAT  OldLimit)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kStrokeAndFillPath(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kStrokePath(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL
STDCALL
W32kWidenPath(HDC  hDC)
{
  UNIMPLEMENTED;
}


