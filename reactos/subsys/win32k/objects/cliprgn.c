

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/cliprgn.h>

// #define NDEBUG
#include <internal/debug.h>

int STDCALL W32kExcludeClipRect(HDC  hDC,
                         int  LeftRect,
                         int  TopRect,
                         int  RightRect,
                         int  BottomRect)
{
  UNIMPLEMENTED;
}

int STDCALL W32kExtSelectClipRgn(HDC  hDC,
                          HRGN  hrgn,
                          int  fnMode)
{
  UNIMPLEMENTED;
}

int STDCALL W32kGetClipBox(HDC  hDC,
                    LPRECT  rc)
{
  UNIMPLEMENTED;
}

int STDCALL W32kGetMetaRgn(HDC  hDC,
                    HRGN  hrgn)
{
  UNIMPLEMENTED;
}

int STDCALL W32kIntersectClipRect(HDC  hDC,
                           int  LeftRect,
                           int  TopRect,
                           int  RightRect,
                           int  BottomRect)
{
  UNIMPLEMENTED;
}

int STDCALL W32kOffsetClipRgn(HDC  hDC,
                       int  XOffset,
                       int  YOffset)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kPtVisible(HDC  hDC,
                    int  X,
                    int  Y)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kRectVisible(HDC  hDC,
                      CONST PRECT  rc)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kSelectClipPath(HDC  hDC,
                         int  Mode)
{
  UNIMPLEMENTED;
}

int STDCALL W32kSelectClipRgn(HDC  hDC,
                       HRGN  hrgn)
{
  UNIMPLEMENTED;
}

int STDCALL W32kSetMetaRgn(HDC  hDC)
{
  UNIMPLEMENTED;
}



