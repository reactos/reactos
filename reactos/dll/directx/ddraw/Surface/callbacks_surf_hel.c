/* $Id$
*
* COPYRIGHT: See COPYING in the top level directory
* PROJECT: ReactOS DirectX
* FILE: ddraw/surface/callbacks_surf_hel.c
* PURPOSE: HEL Callbacks For Surface APIs
* PROGRAMMER: Magnus Olsen
*
*/
#include "rosdraw.h"
DWORD CALLBACK HelDdSurfAddAttachedSurface(LPDDHAL_ADDATTACHEDSURFACEDATA lpDestroySurface)
{
DX_STUB;
}
DWORD CALLBACK HelDdSurfBlt(LPDDHAL_BLTDATA lpBltData)
{
if (lpBltData->dwFlags & DDBLT_COLORFILL)
{
HBRUSH hbr = CreateSolidBrush(lpBltData->bltFX.dwFillColor );
FillRect( (HDC)lpBltData->lpDDDestSurface->lpSurfMore->lpDD_lcl->hDC,
(CONST RECT *)&lpBltData->rDest,
hbr);
DeleteObject(hbr);
lpBltData->ddRVal = DD_OK;
}

else if (lpBltData->dwFlags & DDBLT_ROP)
{

BitBlt( (HDC)lpBltData->lpDDDestSurface->lpSurfMore->lpDD_lcl->hDC,
lpBltData->rDest.top,
lpBltData->rDest.left,
lpBltData->rDest.right,
lpBltData->rDest.bottom,
(HDC)lpBltData->lpDDSrcSurface->lpSurfMore->lpDD_lcl->hDC,
lpBltData->rSrc.top,
lpBltData->rSrc.right,
lpBltData->bltFX.dwROP);
lpBltData->ddRVal = DD_OK;
}
return DDHAL_DRIVER_HANDLED;
}
DWORD CALLBACK HelDdSurfDestroySurface(LPDDHAL_DESTROYSURFACEDATA lpDestroySurfaceData)
{
DX_STUB;
}
DWORD CALLBACK HelDdSurfFlip(LPDDHAL_FLIPDATA lpFlipData)
{
DX_STUB;
}
DWORD CALLBACK HelDdSurfGetBltStatus(LPDDHAL_GETBLTSTATUSDATA lpGetBltStatusData)
{
DX_STUB;
}
DWORD CALLBACK HelDdSurfGetFlipStatus(LPDDHAL_GETFLIPSTATUSDATA lpGetFlipStatusData)
{
DX_STUB;
}
DWORD CALLBACK HelDdSurfLock(LPDDHAL_LOCKDATA lpLockData)
{
    HDC hDC;
    BITMAP bm;
    PDWORD pixels = NULL;
    HGDIOBJ hBmp;
    BITMAPINFO bmi;
    int retvalue = 0;

    /* ToDo tell ddraw internal this surface are locked */
    /* ToDo add support for dwFlags */

    /* Get our hdc for the surface */
    hDC = (HDC)lpLockData->lpDDSurface->lpSurfMore->lpDD_lcl->hDC;

    /* Get our bitmap handle from hdc, we need it if we want extract the Bitmap pixel data */
    hBmp = GetCurrentObject(hDC, OBJ_BITMAP);

    /* Get our bitmap information from hBmp, we need it if we want extract the Bitmap pixel data */
    if (GetObject(hBmp, sizeof(BITMAP), &bm) )
    {
        /* Alloc memory buffer at usermode for the bitmap pixel data  */
        pixels = (PDWORD) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bm.bmWidth * bm.bmHeight * (bm.bmBitsPixel*bm.bmBitsPixel ) );

        if (pixels != NULL)
        {
            /* Zero out all members in bmi so no junk data are left */
            ZeroMemory(&bmi, sizeof(BITMAPINFO));

            /* Setup BITMAPINFOHEADER for bmi header */
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = bm.bmWidth;
            bmi.bmiHeader.biHeight = bm.bmHeight;
            bmi.bmiHeader.biPlanes = bm.bmPlanes;
            bmi.bmiHeader.biBitCount = bm.bmBitsPixel;
            bmi.bmiHeader.biCompression = BI_RGB;

            /* Check if it the bitmap is palete or not */
            if ( bm.bmBitsPixel <= 8)
            {
                /* Extract the bitmap bits data from palete bitmap */
                retvalue = GetDIBits(hDC, hBmp, 0, bm.bmHeight, pixels, &bmi, DIB_PAL_COLORS);
            }
            else
            {
                /* Extract the bitmap bits data from RGB bitmap */
                retvalue = GetDIBits(hDC, hBmp, 0, bm.bmHeight, pixels, &bmi, DIB_PAL_COLORS);
            }

            /* Check see if we susccess it to get the memory pointer and fill it for the bitmap pixel data */
            if (retvalue)
            {
                /* Check see if the are special area it should have been extracted or not */
                if (!lpLockData->bHasRect)
                {
                    /* The all bitmap pixel data must return */
                    lpLockData->lpSurfData = pixels;
                    lpLockData->ddRVal = DD_OK;
                }
                else
                {
                    /* Only the lpLockData->rArea bitmap data should be return */
                    DX_STUB;
                }
            }
        }
    }

    /* Free the pixels buffer if we fail */
    if ( (pixels != NULL) &&
         (lpLockData->ddRVal != DD_OK) )
    {
        HeapFree(GetProcessHeap(), 0, pixels );
    }

    return DDHAL_DRIVER_HANDLED;
}

DWORD CALLBACK HelDdSurfreserved4(DWORD *lpPtr)
{
/*
This api is not doucment by MS So I leave it
as stub.
*/
DX_STUB;
}
DWORD CALLBACK HelDdSurfSetClipList(LPDDHAL_SETCLIPLISTDATA lpSetClipListData)
{
DX_STUB;
}
DWORD CALLBACK HelDdSurfSetColorKey(LPDDHAL_SETCOLORKEYDATA lpSetColorKeyData)
{
DX_STUB;
}
DWORD CALLBACK HelDdSurfSetOverlayPosition(LPDDHAL_SETOVERLAYPOSITIONDATA lpSetOverlayPositionData)
{
DX_STUB;
}
DWORD CALLBACK HelDdSurfSetPalette(LPDDHAL_SETPALETTEDATA lpSetPaletteData)
{
DX_STUB;
}
DWORD CALLBACK HelDdSurfUnlock(LPDDHAL_UNLOCKDATA lpUnLockData)
{
DX_STUB;
}
DWORD CALLBACK HelDdSurfUpdateOverlay(LPDDHAL_UPDATEOVERLAYDATA lpUpDateOveryLayData)
{
DX_STUB;
}
