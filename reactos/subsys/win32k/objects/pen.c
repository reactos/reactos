#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/pen.h>

// #define NDEBUG
#include <win32k/debug1.h>

HPEN
STDCALL
W32kCreatePen(INT PenStyle, INT Width, COLORREF Color)
{
  LOGPEN logpen;

  logpen.lopnStyle = PenStyle; 
  logpen.lopnWidth.x = Width;
  logpen.lopnWidth.y = 0;
  logpen.lopnColor = Color;

  return W32kCreatePenIndirect(&logpen);
}

HPEN
STDCALL
W32kCreatePenIndirect(CONST PLOGPEN lgpn)
{
  PPENOBJ penPtr;
  HPEN    hpen;

  if (lgpn->lopnStyle > PS_INSIDEFRAME) return 0;

  penPtr = PENOBJ_AllocPen();
  hpen   = PENOBJ_PtrToHandle(penPtr);
  if (!hpen) return 0;
  PENOBJ_LockPen(hpen);

  penPtr->logpen.lopnStyle = lgpn->lopnStyle;
  penPtr->logpen.lopnWidth = lgpn->lopnWidth;
  penPtr->logpen.lopnColor = lgpn->lopnColor;

  PENOBJ_UnlockPen(hpen);

  return hpen;
}

HPEN
STDCALL
W32kExtCreatePen(DWORD  PenStyle,
                       DWORD  Width,
                       CONST PLOGBRUSH  lb,
                       DWORD  StyleCount,
                       CONST PDWORD  Style)
{
  UNIMPLEMENTED;
}
