#ifndef _WIN32K_COLOR_H
#define _WIN32K_COLOR_H

#define NB_RESERVED_COLORS              20 /* number of fixed colors in system palette */

const PALETTEENTRY* INTERNAL_CALL COLOR_GetSystemPaletteTemplate (VOID);
COLORREF INTERNAL_CALL COLOR_LookupNearestColor (PALETTEENTRY* palPalEntry, INT size, COLORREF color);
INT INTERNAL_CALL COLOR_PaletteLookupExactIndex (PALETTEENTRY* palPalEntry, INT size, COLORREF col);
INT INTERNAL_CALL COLOR_PaletteLookupPixel(PALETTEENTRY *palPalEntry, INT size, XLATEOBJ *XlateObj, COLORREF col, BOOL skipReserved);

#endif /* _WIN32K_COLOR_H */
