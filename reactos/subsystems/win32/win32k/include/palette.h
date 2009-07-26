#ifndef _WIN32K_PALETTE_H
#define _WIN32K_PALETTE_H

#include <include/dc.h>

#define NO_MAPPING

#define PALETTE_FIXED    0x0001 /* read-only colormap - have to use XAllocColor (if not virtual) */
#define PALETTE_VIRTUAL  0x0002 /* no mapping needed - pixel == pixel color */

#define PALETTE_PRIVATE  0x1000 /* private colormap, identity mapping */
#define PALETTE_WHITESET 0x2000

// Palette mode flags
#ifndef __WINDDI_H // Defined in ddk/winddi.h
#define PAL_INDEXED         0x00000001 // Indexed palette
#define PAL_BITFIELDS       0x00000002 // Bit fields used for DIB, DIB section
#define PAL_RGB             0x00000004 // Red, green, blue
#define PAL_BGR             0x00000008 // Blue, green, red
#define PAL_CMYK            0x00000010 // Cyan, magenta, yellow, black
#endif
#define PAL_DC              0x00000100
#define PAL_FIXED           0x00000200 // Can't be changed
#define PAL_FREE            0x00000400
#define PAL_MANAGED         0x00000800
#define PAL_NOSTATIC        0x00001000
#define PAL_MONOCHROME      0x00002000 // Two colors only
#define PAL_BRUSHHACK       0x00004000
#define PAL_DIBSECTION      0x00008000 // Used for a DIB section
#define PAL_NOSTATIC256     0x00010000
#define PAL_HT              0x00100000 // Halftone palette
#define PAL_RGB16_555       0x00200000 // 16-bit RGB in 555 format
#define PAL_RGB16_565       0x00400000 // 16-bit RGB in 565 format
#define PAL_GAMMACORRECTION 0x00800000 // Correct colors


typedef struct _PALETTE
{
  /* Header for all gdi objects in the handle table.
     Do not (re)move this. */
  BASEOBJECT    BaseObject;

  PALOBJ PalObj;
  XLATEOBJ *logicalToSystem;
  HPALETTE Self;
  ULONG Mode; // PAL_INDEXED, PAL_BITFIELDS, PAL_RGB, PAL_BGR
  ULONG NumColors;
  PALETTEENTRY *IndexedColors;
  ULONG RedMask;
  ULONG GreenMask;
  ULONG BlueMask;
  HDEV  hPDev;
} PALETTE, *PPALETTE;

extern PALETTE gpalRGB, gpalBGR;


HPALETTE FASTCALL PALETTE_AllocPalette(ULONG Mode,
                                       ULONG NumColors,
                                       ULONG *Colors,
                                       ULONG Red,
                                       ULONG Green,
                                       ULONG Blue);
HPALETTE FASTCALL PALETTE_AllocPaletteIndexedRGB(ULONG NumColors,
                                                 CONST RGBQUAD *Colors);
#define  PALETTE_FreePalette(pPalette)  GDIOBJ_FreeObj((POBJ)pPalette, GDIObjType_PAL_TYPE)
#define  PALETTE_FreePaletteByHandle(hPalette)  GDIOBJ_FreeObjByHandle((HGDIOBJ)hPalette, GDI_OBJECT_TYPE_PALETTE)
#define  PALETTE_LockPalette(hPalette) ((PPALETTE)GDIOBJ_LockObj((HGDIOBJ)hPalette, GDI_OBJECT_TYPE_PALETTE))
#define  PALETTE_UnlockPalette(pPalette) GDIOBJ_UnlockObjByPtr((POBJ)pPalette)

#define  PALETTE_ShareLockPalette(hpal) \
  ((PPALETTE)GDIOBJ_ShareLockObj((HGDIOBJ)hpal, GDI_OBJECT_TYPE_PALETTE))
#define  PALETTE_ShareUnlockPalette(ppal)  \
  GDIOBJ_ShareUnlockObjByPtr(&ppal->BaseObject)

BOOL INTERNAL_CALL PALETTE_Cleanup(PVOID ObjectBody);

HPALETTE FASTCALL PALETTE_Init (VOID);
VOID     FASTCALL PALETTE_ValidateFlags (PALETTEENTRY* lpPalE, INT size);
#ifndef NO_MAPPING
INT      APIENTRY  PALETTE_SetMapping(PALOBJ* palPtr, UINT uStart, UINT uNum, BOOL mapOnly);
#endif
INT      FASTCALL PALETTE_ToPhysical (PDC dc, COLORREF color);

INT FASTCALL PALETTE_GetObject(PPALETTE pGdiObject, INT cbCount, LPLOGBRUSH lpBuffer);
ULONG NTAPI PALETTE_ulGetNearestPaletteIndex(PALETTE* ppal, ULONG iColor);
VOID NTAPI PALETTE_vGetBitMasks(PPALETTE ppal, PULONG pulColors);

PPALETTEENTRY FASTCALL ReturnSystemPalette (VOID);
HPALETTE FASTCALL GdiSelectPalette(HDC, HPALETTE, BOOL);

FORCEINLINE
ULONG
PALETTE_ulGetRGBColorFromIndex(PPALETTE ppal, ULONG ulIndex)
{
    return RGB(ppal->IndexedColors[ulIndex].peRed,
               ppal->IndexedColors[ulIndex].peGreen,
               ppal->IndexedColors[ulIndex].peBlue);
}

#endif /* not _WIN32K_PALETTE_H */
