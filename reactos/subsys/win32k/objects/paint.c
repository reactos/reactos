

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <win32k/paint.h>

// #define NDEBUG
#include <internal/debug.h>

BOOL  W32kGdiFlush(VOID)
{
  UNIMPLEMENTED;
}

DWORD GdiGetBatchLimit(VOID)
{
  UNIMPLEMENTED;
}

DWORD  W32kGdiSetBatchLimit(DWORD  Limit)
{
  UNIMPLEMENTED;
}

UINT  W32kGetBoundsRect(HDC  hDC,
                        LPRECT  Bounds,
                        UINT  Flags)
{
  UNIMPLEMENTED;
}

COLORREF  W32kSetBkColor(HDC  hDC,
                         COLORREF  Color)
{
  UNIMPLEMENTED;
}

UINT  W32kSetBoundsRect(HDC  hDC,
                        CONST PRECT  Bounds,
                        UINT  Flags)
{
  UNIMPLEMENTED;
}

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

BOOL  W32kGetMiterLimit(HDC  hDC, PFLOAT  Limit)
{
  UNIMPLEMENTED;
}

INT  W32kGetPath(HDC  hDC,
                 PPOINT  Points,
                 PBYTE  Types,
                 INT  Size)
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


