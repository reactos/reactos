#ifndef _WIN32K_COLOR_H
#define _WIN32K_COLOR_H

#define NB_RESERVED_COLORS              20 /* number of fixed colors in system palette */

const PALETTEENTRY* FASTCALL COLOR_GetSystemPaletteTemplate (VOID);
COLORREF STDCALL COLOR_LookupNearestColor (PALETTEENTRY* palPalEntry, INT size, COLORREF color);
INT STDCALL COLOR_PaletteLookupExactIndex (PALETTEENTRY* palPalEntry, INT size, COLORREF col);
INT STDCALL COLOR_PaletteLookupPixel(PALETTEENTRY *palPalEntry, INT size, XLATEOBJ *XlateObj, COLORREF col, BOOL skipReserved);

#endif /* _WIN32K_COLOR_H */
