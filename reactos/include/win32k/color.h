
#ifndef __WIN32K_COLOR_H
#define __WIN32K_COLOR_H

BOOL  W32kAnimatePalette(HPALETTE  hpal,
                         UINT  StartIndex,
                         UINT  Entries,
                         CONST PPALETTEENTRY  ppe);

HPALETTE  W32kCreateHalftonePalette(HDC  hDC);

HPALETTE  W32kCreatePalette(CONST PLOGPALETTE lgpl);

BOOL  W32kGetColorAdjustment(HDC  hDC,
                             LPCOLORADJUSTMENT  ca);

COLORREF  W32kGetNearestColor(HDC  hDC,
                              COLORREF  Color);

UINT  W32kGetNearestPaletteIndex(HPALETTE  hpal,
                                 COLORREF  Color);

UINT  W32kGetPaletteEntries(HPALETTE  hpal,
                            UINT  StartIndex,
                            UINT  Entries,
                            LPPALETTEENTRY  pe);

UINT  W32kGetSystemPaletteEntries(HDC  hDC,
                                  UINT  StartIndex,
                                  UINT  Entries,
                                  LPPALETTEENTRY  pe);

UINT  W32kGetSystemPaletteUse(HDC  hDC);

UINT  W32kRealizePalette(HDC  hDC);

BOOL  W32kResizePalette(HPALETTE  hpal,
                        UINT  Entries);

HPALETTE  W32kSelectPalette(HDC  hDC,
                            HPALETTE  hpal,
                            BOOL  ForceBackground);

BOOL  W32kSetColorAdjustment(HDC  hDC,
                             CONST LPCOLORADJUSTMENT  ca);

UINT  W32kSetPaletteEntries(HPALETTE  hpal,
                            UINT  Start,
                            UINT  Entries,
                            CONST LPPALETTEENTRY  pe);

UINT  W32kSetSystemPaletteUse(HDC  hDC,
                              UINT  Usage);

BOOL  W32kUnrealizeObject(HGDIOBJ  hgdiobj);

BOOL  W32kUpdateColors(HDC  hDC);

#endif
