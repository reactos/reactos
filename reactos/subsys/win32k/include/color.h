#ifndef _WIN32K_COLOR_H
#define _WIN32K_COLOR_H

const PALETTEENTRY* FASTCALL COLOR_GetSystemPaletteTemplate (VOID);
COLORREF STDCALL COLOR_LookupNearestColor (PALETTEENTRY* palPalEntry, INT size, COLORREF color);
INT STDCALL COLOR_PaletteLookupExactIndex (PALETTEENTRY* palPalEntry, INT size, COLORREF col);
INT STDCALL COLOR_PaletteLookupPixel(PALETTEENTRY *palPalEntry, INT size, PXLATEOBJ XlateObj, COLORREF col, BOOL skipReserved);

#endif /* _WIN32K_COLOR_H */
