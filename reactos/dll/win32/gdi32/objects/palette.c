#include "precomp.h"

#define NDEBUG
#include <debug.h>

BOOL
WINAPI
AnimatePalette(HPALETTE hpal,
               UINT iStartIndex,
               UINT cEntries,
               const PALETTEENTRY *ppe)
{
    return NtGdiDoPalette(hpal, iStartIndex, cEntries, (PALETTEENTRY*)ppe, GdiPalAnimate, TRUE);
}

HPALETTE
WINAPI
CreatePalette(CONST LOGPALETTE * plpal)
{
    return NtGdiCreatePaletteInternal((LPLOGPALETTE)plpal, plpal->palNumEntries);
}

/*
 * @implemented
 */
UINT
WINAPI
GetPaletteEntries(HPALETTE hpal,
                  UINT iStartIndex,
                  UINT cEntries,
                  LPPALETTEENTRY ppe)
{
    return NtGdiDoPalette(hpal, iStartIndex, cEntries, ppe, GdiPalGetEntries, FALSE);
}

UINT
WINAPI
SetPaletteEntries(HPALETTE hpal,
                  UINT iStartIndex,
                  UINT cEntries,
                  const PALETTEENTRY *ppe)
{
    return NtGdiDoPalette(hpal, iStartIndex, cEntries, (PALETTEENTRY*)ppe, GdiPalSetEntries, TRUE);
}

UINT
WINAPI
GetSystemPaletteEntries(HDC hDC,
                        UINT iStartIndex,
                        UINT cEntries,
                        LPPALETTEENTRY ppe)
{
    return NtGdiDoPalette(hDC, iStartIndex, cEntries, ppe, GdiPalGetSystemEntries, FALSE);
}

UINT
WINAPI
GetDIBColorTable(HDC hDC,
                 UINT iStartIndex,
                 UINT cEntries,
                 RGBQUAD *pColors)
{
    return NtGdiDoPalette(hDC, iStartIndex, cEntries, pColors, GdiPalGetColorTable, FALSE);
}

UINT
WINAPI
SetDIBColorTable(HDC hDC,
                 UINT iStartIndex,
                 UINT cEntries,
                 const RGBQUAD *pColors)
{
    return NtGdiDoPalette(hDC, iStartIndex, cEntries, (RGBQUAD*)pColors, GdiPalSetColorTable, TRUE);
}

/* EOF */
