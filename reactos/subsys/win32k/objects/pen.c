

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/pen.h>

// #define NDEBUG
#include <internal/debug.h>

HPEN  W32kCreatePen(INT  PenStyle,
                    INT  Width,
                    COLORREF  Color)
{
  UNIMPLEMENTED;
}

HPEN  W32kCreatePenIndirect(CONST PLOGPEN  lgpn)
{
  UNIMPLEMENTED;
}

HPEN  W32kExtCreatePen(DWORD  PenStyle,
                       DWORD  Width,
                       CONST PLOGBRUSH  lb,
                       DWORD  StyleCount,
                       CONST PDWORD  Style)
{
  UNIMPLEMENTED;
}



