
#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/color.h>

// #define NDEBUG
#include <internal/debug.h>

BOOL  W32kAnimatePalette(HPALETTE  hpal,
                         UINT  StartIndex,
                         UINT  Entries,
                         CONST PPALETTEENTRY  ppe)
{
  UNIMPLEMENTED;
}

HPALETTE  W32kCreateHalftonePalette(HDC  hDC)
{
  UNIMPLEMENTED;
}

HPALETTE  W32kCreatePalette(CONST PLOGPALETTE lgpl)
{
  UNIMPLEMENTED;
}

BOOL  W32kGetColorAdjustment(HDC  hDC,
                             LPCOLORADJUSTMENT  ca)
{
  UNIMPLEMENTED;
}

COLORREF  W32kGetNearestColor(HDC  hDC,
                              COLORREF  Color)
{
  UNIMPLEMENTED;
}

UINT  W32kGetNearestPaletteIndex(HPALETTE  hpal,
                                 COLORREF  Color)
{
  UNIMPLEMENTED;
}

UINT  W32kGetPaletteEntries(HPALETTE  hpal,
                            UINT  StartIndex,
                            UINT  Entries,
                            LPPALETTEENTRY  pe)
{
  UNIMPLEMENTED;
}

UINT  W32kGetSystemPaletteEntries(HDC  hDC,
                                  UINT  StartIndex,
                                  UINT  Entries,
                                  LPPALETTEENTRY  pe)
{
  UNIMPLEMENTED;
}

UINT  W32kGetSystemPaletteUse(HDC  hDC)
{
  UNIMPLEMENTED;
}

UINT  W32kRealizePalette(HDC  hDC)
{
  UNIMPLEMENTED;
}

BOOL  W32kResizePalette(HPALETTE  hpal,
                        UINT  Entries)
{
  UNIMPLEMENTED;
}

HPALETTE  W32kSelectPalette(HDC  hDC,
                            HPALETTE  hpal,
                            BOOL  ForceBackground)
{
  UNIMPLEMENTED;
}

BOOL  W32kSetColorAdjustment(HDC  hDC,
                             CONST LPCOLORADJUSTMENT  ca)
{
  UNIMPLEMENTED;
}

UINT  W32kSetPaletteEntries(HPALETTE  hpal,
                            UINT  Start,
                            UINT  Entries,
                            CONST LPPALETTEENTRY  pe)
{
  UNIMPLEMENTED;
}

UINT  W32kSetSystemPaletteUse(HDC  hDC,
                              UINT  Usage)
{
  UNIMPLEMENTED;
}

BOOL  W32kUnrealizeObject(HGDIOBJ  hgdiobj)
{
  UNIMPLEMENTED;
}

BOOL  W32kUpdateColors(HDC  hDC)
{
  UNIMPLEMENTED;
}

