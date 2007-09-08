#ifndef _WIN32K_PALETTE_H
#define _WIN32K_PALETTE_H

#define NO_MAPPING

#define PALETTE_FIXED    0x0001 /* read-only colormap - have to use XAllocColor (if not virtual) */
#define PALETTE_VIRTUAL  0x0002 /* no mapping needed - pixel == pixel color */

#define PALETTE_PRIVATE  0x1000 /* private colormap, identity mapping */
#define PALETTE_WHITESET 0x2000

typedef struct {
    int shift;
    int scale;
    int max;
} ColorShifts;

typedef struct _PALGDI {
  HGDIOBJ     hHmgr;
  PVOID       pvEntry;
  ULONG       lucExcLock;
  ULONG       Tid;

  PALOBJ PalObj;
  XLATEOBJ *logicalToSystem;
  HPALETTE Self;
  ULONG Mode; // PAL_INDEXED, PAL_BITFIELDS, PAL_RGB, PAL_BGR
  ULONG NumColors;
  PALETTEENTRY *IndexedColors;
  ULONG RedMask;
  ULONG GreenMask;
  ULONG BlueMask;
} PALGDI, *PPALGDI;

HPALETTE FASTCALL PALETTE_AllocPalette(ULONG Mode,
                                       ULONG NumColors,
                                       ULONG *Colors,
                                       ULONG Red,
                                       ULONG Green,
                                       ULONG Blue);
HPALETTE FASTCALL PALETTE_AllocPaletteIndexedRGB(ULONG NumColors,
                                                 CONST RGBQUAD *Colors);
#define  PALETTE_FreePalette(hPalette)  GDIOBJ_FreeObj(GdiHandleTable, (HGDIOBJ)hPalette, GDI_OBJECT_TYPE_PALETTE)
#define  PALETTE_LockPalette(hPalette) ((PPALGDI)GDIOBJ_LockObj(GdiHandleTable, (HGDIOBJ)hPalette, GDI_OBJECT_TYPE_PALETTE))
#define  PALETTE_UnlockPalette(pPalette) GDIOBJ_UnlockObjByPtr(GdiHandleTable, pPalette)
BOOL INTERNAL_CALL PALETTE_Cleanup(PVOID ObjectBody);

HPALETTE FASTCALL PALETTE_Init (VOID);
VOID     FASTCALL PALETTE_ValidateFlags (PALETTEENTRY* lpPalE, INT size);
#ifndef NO_MAPPING
INT      STDCALL  PALETTE_SetMapping(PALOBJ* palPtr, UINT uStart, UINT uNum, BOOL mapOnly);
#endif
INT      FASTCALL PALETTE_ToPhysical (PDC dc, COLORREF color);

INT FASTCALL PALETTE_GetObject(PPALGDI pGdiObject, INT cbCount, LPLOGBRUSH lpBuffer);

PPALETTEENTRY FASTCALL ReturnSystemPalette (VOID);

#endif /* _WIN32K_PALETTE_H */
