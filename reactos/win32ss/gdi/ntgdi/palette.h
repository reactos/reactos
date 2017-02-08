#pragma once

#ifdef _MSC_VER
#pragma warning(disable:4200) // zero-sized array in struct/union
#endif // _MSC_VER

// Palette mode flags
enum _PALFLAGS
{
#ifndef _WINDDI_ // Defined in ddk/winddi.h
    PAL_INDEXED         = 0x00000001, // Indexed palette
    PAL_BITFIELDS       = 0x00000002, // Bit fields used for DIB, DIB section
    PAL_RGB             = 0x00000004, // Red, green, blue
    PAL_BGR             = 0x00000008, // Blue, green, red
    PAL_CMYK            = 0x00000010, // Cyan, magenta, yellow, black
#endif
    PAL_DC              = 0x00000100,
    PAL_FIXED           = 0x00000200, // Can't be changed
    PAL_FREE            = 0x00000400,
    PAL_MANAGED         = 0x00000800,
    PAL_NOSTATIC        = 0x00001000,
    PAL_MONOCHROME      = 0x00002000, // Two colors only
    PAL_BRUSHHACK       = 0x00004000,
    PAL_DIBSECTION      = 0x00008000, // Used for a DIB section
    PAL_NOSTATIC256     = 0x00010000,
    PAL_HT              = 0x00100000, // Halftone palette
    PAL_RGB16_555       = 0x00200000, // 16-bit RGB in 555 format
    PAL_RGB16_565       = 0x00400000, // 16-bit RGB in 565 format
    PAL_GAMMACORRECTION = 0x00800000, // Correct colors
};

typedef struct _PALETTE
{
    /* Header for all gdi objects in the handle table.
       Do not (re)move this. */
    BASEOBJECT    BaseObject;

    PALOBJ PalObj;
    XLATEOBJ *logicalToSystem;
    FLONG flFlags; // PAL_INDEXED, PAL_BITFIELDS, PAL_RGB, PAL_BGR
    ULONG NumColors;
    PALETTEENTRY *IndexedColors;
    ULONG RedMask;
    ULONG GreenMask;
    ULONG BlueMask;
    ULONG ulRedShift;
    ULONG ulGreenShift;
    ULONG ulBlueShift;
    HDEV  hPDev;
    PALETTEENTRY apalColors[0];
} PALETTE, *PPALETTE;

extern PALETTE gpalRGB, gpalBGR, gpalRGB555, gpalRGB565, *gppalMono, *gppalDefault;
extern PPALETTE appalSurfaceDefault[];

#define  PALETTE_UnlockPalette(pPalette) GDIOBJ_vUnlockObject((POBJ)pPalette)
#define  PALETTE_ShareLockPalette(hpal) \
  ((PPALETTE)GDIOBJ_ShareLockObj((HGDIOBJ)hpal, GDI_OBJECT_TYPE_PALETTE))
#define  PALETTE_ShareUnlockPalette(ppal)  \
  GDIOBJ_vDereferenceObject(&ppal->BaseObject)

INIT_FUNCTION
NTSTATUS
NTAPI
InitPaletteImpl(VOID);

PPALETTE
NTAPI
PALETTE_AllocPalette(
    _In_ ULONG iMode,
    _In_ ULONG cColors,
    _In_opt_ const PALETTEENTRY* pEntries,
    _In_ FLONG flRed,
    _In_ FLONG flGreen,
    _In_ FLONG flBlue);

PPALETTE
NTAPI
PALETTE_AllocPalWithHandle(
    _In_ ULONG iMode,
    _In_ ULONG cColors,
    _In_opt_ const PALETTEENTRY* pEntries,
    _In_ FLONG flRed,
    _In_ FLONG flGreen,
    _In_ FLONG flBlue);

VOID
FASTCALL
PALETTE_ValidateFlags(
    PALETTEENTRY* lpPalE,
    INT size);

INT
FASTCALL
PALETTE_GetObject(
    PPALETTE pGdiObject,
    INT cbCount,
    LPLOGBRUSH lpBuffer);

ULONG
NTAPI
PALETTE_ulGetNearestPaletteIndex(
    PPALETTE ppal,
    ULONG iColor);

ULONG
NTAPI
PALETTE_ulGetNearestIndex(
    PPALETTE ppal,
    ULONG iColor);

ULONG
NTAPI
PALETTE_ulGetNearestBitFieldsIndex(
    PPALETTE ppal,
    ULONG ulColor);

VOID
NTAPI
PALETTE_vGetBitMasks(
    PPALETTE ppal,
    PULONG pulColors);

VOID
NTAPI
PALETTE_vCleanup(PVOID ObjectBody);

FORCEINLINE
ULONG
CalculateShift(ULONG ulMask1, ULONG ulMask2)
{
    ULONG ulShift1, ulShift2;
    BitScanReverse(&ulShift1, ulMask1);
    BitScanReverse(&ulShift2, ulMask2);
    ulShift2 -= ulShift1;
    if ((INT)ulShift2 < 0) ulShift2 += 32;
    return ulShift2;
}

FORCEINLINE
ULONG
PALETTE_ulGetRGBColorFromIndex(PPALETTE ppal, ULONG ulIndex)
{
    if (ulIndex >= ppal->NumColors) return 0;
    return RGB(ppal->IndexedColors[ulIndex].peRed,
               ppal->IndexedColors[ulIndex].peGreen,
               ppal->IndexedColors[ulIndex].peBlue);
}

FORCEINLINE
VOID
PALETTE_vSetRGBColorForIndex(PPALETTE ppal, ULONG ulIndex, COLORREF crColor)
{
    if (ulIndex >= ppal->NumColors) return;
    ppal->IndexedColors[ulIndex].peRed = GetRValue(crColor);
    ppal->IndexedColors[ulIndex].peGreen = GetGValue(crColor);
    ppal->IndexedColors[ulIndex].peBlue = GetBValue(crColor);
}

HPALETTE
NTAPI
GreCreatePaletteInternal(
    IN LPLOGPALETTE pLogPal,
    IN UINT cEntries);

