

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
//#include <win32k/debug.h>
#include <win32k/paint.h>

#define NDEBUG
#include <win32k/debug1.h>

BOOL
STDCALL
W32kGdiFlush(VOID)
{
  UNIMPLEMENTED;
}

DWORD
STDCALL
W32kGdiGetBatchLimit(VOID)
{
  UNIMPLEMENTED;
}

DWORD
STDCALL
W32kGdiSetBatchLimit(DWORD  Limit)
{
  UNIMPLEMENTED;
}

UINT
STDCALL
W32kGetBoundsRect(HDC  hDC,
                        LPRECT  Bounds,
                        UINT  Flags)
{
  DPRINT("stub");
  return  DCB_RESET;   /* bounding rectangle always empty */
}

UINT
STDCALL
W32kSetBoundsRect(HDC  hDC,
                        CONST PRECT  Bounds,
                        UINT  Flags)
{
  DPRINT("stub");
  return  DCB_DISABLE;   /* bounding rectangle always empty */
}

