

#include "xboxdisp.h"

BOOL APIENTRY
DrvEnableDirectDraw(DHPDEV dhpdev,
                    DD_CALLBACKS *pCallbacks,
                    DD_SURFACECALLBACKS *pSurfaceCallbacks,
                    DD_PALETTECALLBACKS *pPaletteCallbacks)
{
    RtlZeroMemory(pCallbacks, sizeof(*pCallbacks));
    RtlZeroMemory(pSurfaceCallbacks, sizeof(*pSurfaceCallbacks));
    RtlZeroMemory(pPaletteCallbacks, sizeof(*pPaletteCallbacks));

    pCallbacks->dwSize        = sizeof(*pCallbacks);
    pSurfaceCallbacks->dwSize = sizeof(*pSurfaceCallbacks);
    pPaletteCallbacks->dwSize = sizeof(*pPaletteCallbacks);
    return TRUE;
}

VOID APIENTRY
DrvDisableDirectDraw(DHPDEV dhpdev)
{
}
