

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/debug.h>
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
  FIXME("stub");
  return  DCB_RESET;   /* bounding rectangle always empty */
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
  FIXME("stub");
  return  DCB_DISABLE;   /* bounding rectangle always empty */
}

