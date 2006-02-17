#ifndef _WIN32K_COLOR_H
#define _WIN32K_COLOR_H

#ifndef CLR_INVALID
#define CLR_INVALID         0xffffffff
#endif
#define PC_SYS_USED     0x80		/* palentry is used (both system and logical) */
#define PC_SYS_RESERVED 0x40		/* system palentry is not to be mapped to */
#define PC_SYS_MAPPED   0x10		/* logical palentry is a direct alias for system palentry */

#define NB_RESERVED_COLORS              20 /* number of fixed colors in system palette */

const PALETTEENTRY* FASTCALL COLOR_GetSystemPaletteTemplate (VOID);
COLORREF STDCALL COLOR_LookupNearestColor (PALETTEENTRY* palPalEntry, INT size, COLORREF color);
INT STDCALL COLOR_PaletteLookupExactIndex (PALETTEENTRY* palPalEntry, INT size, COLORREF col);
INT STDCALL COLOR_PaletteLookupPixel(PALETTEENTRY *palPalEntry, INT size, XLATEOBJ *XlateObj, COLORREF col, BOOL skipReserved);


#endif /* _WIN32K_COLOR_H */
