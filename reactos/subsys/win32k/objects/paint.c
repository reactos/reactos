

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <win32k/paint.h>

// #define NDEBUG
#include <internal/debug.h>

BOOL  W32kGdiFlush(VOID)
{
  UNIMPLEMENTED;
}

DWORD  W32kGdiGetBatchLimit(VOID)
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

