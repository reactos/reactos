
#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/color.h>

// #define NDEBUG
#include <internal/debug.h>

BOOL STDCALL W32kAnimatePalette(HPALETTE  hpal,
                         UINT  StartIndex,
                         UINT  Entries,
                         CONST PPALETTEENTRY  ppe)
{
  UNIMPLEMENTED;
}

HPALETTE STDCALL W32kCreateHalftonePalette(HDC  hDC)
{
  UNIMPLEMENTED;
}

HPALETTE STDCALL W32kCreatePalette(CONST PLOGPALETTE lgpl)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kGetColorAdjustment(HDC  hDC,
                             LPCOLORADJUSTMENT  ca)
{
  UNIMPLEMENTED;
}

COLORREF STDCALL W32kGetNearestColor(HDC  hDC,
                              COLORREF  Color)
{
  UNIMPLEMENTED;
}

UINT STDCALL W32kGetNearestPaletteIndex(HPALETTE  hpal,
                                 COLORREF  Color)
{
  UNIMPLEMENTED;
}

UINT STDCALL W32kGetPaletteEntries(HPALETTE  hpal,
                            UINT  StartIndex,
                            UINT  Entries,
                            LPPALETTEENTRY  pe)
{
  UNIMPLEMENTED;
}

UINT STDCALL W32kGetSystemPaletteEntries(HDC  hDC,
                                  UINT  StartIndex,
                                  UINT  Entries,
                                  LPPALETTEENTRY  pe)
{
  UNIMPLEMENTED;
}

UINT STDCALL W32kGetSystemPaletteUse(HDC  hDC)
{
  UNIMPLEMENTED;
}

UINT STDCALL W32kRealizePalette(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kResizePalette(HPALETTE  hpal,
                        UINT  Entries)
{
  UNIMPLEMENTED;
}

HPALETTE STDCALL W32kSelectPalette(HDC  hDC,
                            HPALETTE  hpal,
                            BOOL  ForceBackground)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kSetColorAdjustment(HDC  hDC,
                             CONST LPCOLORADJUSTMENT  ca)
{
  UNIMPLEMENTED;
}

UINT STDCALL W32kSetPaletteEntries(HPALETTE  hpal,
                            UINT  Start,
                            UINT  Entries,
                            CONST LPPALETTEENTRY  pe)
{
  UNIMPLEMENTED;
}

UINT STDCALL W32kSetSystemPaletteUse(HDC  hDC,
                              UINT  Usage)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kUnrealizeObject(HGDIOBJ  hgdiobj)
{
  UNIMPLEMENTED;
}

BOOL STDCALL W32kUpdateColors(HDC  hDC)
{
  UNIMPLEMENTED;
}

