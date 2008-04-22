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

/*
 * @implemented
 */
UINT
WINAPI
RealizePalette(HDC hDC) /* [in] Handle of device context */
{
#if 0
// Handle something other than a normal dc object.
 if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
 {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_(hDC);
    else
    {
      HPALETTE Pal = GetDCObject(hDC, GDI_OBJECT_TYPE_PALETTE);
      PLDC pLDC = GdiGetLDC((HDC) Pal);
      if ( !pLDC ) return FALSE;
      if (pLDC->iType == LDC_EMFLDC) return EMFDRV_(Pal);
      return FALSE;
    }
 }
#endif
 return UserRealizePalette(hDC);
}

/*
 * @implemented
 */
BOOL
STDCALL
ResizePalette(
	HPALETTE	hPalette,
	UINT		nEntries
	)
{
  return NtGdiResizePalette(hPalette, nEntries);
}

/*
 * @implemented
 */
UINT
WINAPI
SetDIBColorTable(HDC hDC,
                 UINT iStartIndex,
                 UINT cEntries,
                 const RGBQUAD *pColors)
{
    UINT retValue=0;

    if (cEntries)
    {
        retValue = NtGdiDoPalette(hDC, iStartIndex, cEntries, (RGBQUAD*)pColors, GdiPalSetColorTable, TRUE);
    }

    return retValue;
}

/*
 * @implemented
 */
BOOL
STDCALL
UpdateColors(
	HDC	hdc
	)
{
  ((PW32CLIENTINFO)NtCurrentTeb()->Win32ClientInfo)->cSpins = 0;
  return NtGdiUpdateColors(hdc);
}

/* EOF */
