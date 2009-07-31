#ifndef __WIN32K_PALOBJ_H
#define __WIN32K_PALOBJ_H

typedef struct _PALETTE
{
  BASEOBJECT    BaseObject;

  PALOBJ PalObj;
  //XLATEOBJ *logicalToSystem;
  HPALETTE Self;
  ULONG Mode; // PAL_INDEXED, PAL_BITFIELDS, PAL_RGB, PAL_BGR
  ULONG NumColors;
  PALETTEENTRY *IndexedColors;
  ULONG RedMask;
  ULONG GreenMask;
  ULONG BlueMask;
  //HDEV  hPDev;
} PALETTE, *PPALETTE;

HPALETTE FASTCALL PALETTE_AllocPalette(ULONG Mode,
                                       ULONG NumColors,
                                       ULONG *Colors,
                                       ULONG Red,
                                       ULONG Green,
                                       ULONG Blue);

HPALETTE FASTCALL
PALETTE_AllocPaletteIndexedRGB(ULONG NumColors,
                               CONST RGBQUAD *Colors);

RGBQUAD * NTAPI
DIB_MapPaletteColors(PDC dc, CONST BITMAPINFO* lpbmi);

HPALETTE NTAPI
BuildDIBPalette(CONST BITMAPINFO *bmi, PINT paletteType);

VOID
FORCEINLINE
PALETTE_FreePaletteByHandle(HGDIOBJ hPalette)
{
    GDIOBJ_FreeObjByHandle(hPalette, GDI_OBJECT_TYPE_PALETTE);
}

#define  PALETTE_LockPalette(hPalette) ((PPALETTE)GDIOBJ_LockObj((HGDIOBJ)hPalette, GDI_OBJECT_TYPE_PALETTE))
#define  PALETTE_UnlockPalette(pPalette) GDIOBJ_UnlockObjByPtr((PBASEOBJECT)pPalette)


#endif
