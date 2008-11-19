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
    DX_WINDBG_trace();

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
    HBITMAP hImage = NULL;

    LONG cbBuffer = 0;
    LPDWORD pixels = NULL;

    HDC hMemDC = NULL;
    HBITMAP hDCBmp = NULL;
    BITMAP bm = {0};

    DX_WINDBG_trace();

    /* ToDo tell ddraw internal this surface are locked */
    /* ToDo add support for dwFlags */


    /* Get our hdc for the surface */
    hDC = (HDC)lpLockData->lpDDSurface->lpSurfMore->lpDD_lcl->hDC;

    if (hDC != NULL)
    {
        /* Create a memory bitmap to store a copy of current hdc surface */

        if (!lpLockData->bHasRect)
        {
            
            hImage = CreateCompatibleBitmap (hDC, lpLockData->lpDDSurface->lpGbl->wWidth, lpLockData->lpDDSurface->lpGbl->wHeight);
        }
        else
        {
            hImage = CreateCompatibleBitmap (hDC, lpLockData->rArea.right, lpLockData->rArea.bottom);
        }

        /* Create a memory hdc so we can draw on our current memory bitmap */
        hMemDC = CreateCompatibleDC(hDC);

        if (hMemDC != NULL)
        {
            /* Select our memory bitmap to our memory hdc */
            hDCBmp = (HBITMAP) SelectObject (hMemDC, hImage);

            /* Get our memory bitmap information */
            GetObject(hImage, sizeof(BITMAP), &bm);

            if (!lpLockData->bHasRect)
            {
                BitBlt (hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, hDC, 0, 0, SRCCOPY);
            }
            else
            {
                BitBlt (hMemDC, lpLockData->rArea.top, lpLockData->rArea.left, lpLockData->rArea.right, lpLockData->rArea.bottom, hDC, 0, 0, SRCCOPY);
            }

            SelectObject (hMemDC, hDCBmp);

            /* Alloc memory buffer at usermode for the bitmap pixel data  */
            cbBuffer = bm.bmWidthBytes * bm.bmHeight ;
            pixels = (PDWORD) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbBuffer );

            if (pixels != NULL)
            {
                /* Get the bitmap bits */
                GetBitmapBits(hImage,cbBuffer,pixels);

                /* Setup return value */
                lpLockData->ddRVal = DD_OK;
                lpLockData->lpSurfData = pixels;
            }
        }
    }


    /* Free the pixels buffer if we fail */
    if ( (pixels != NULL) &&
         (lpLockData->ddRVal != DD_OK) )
    {
        HeapFree(GetProcessHeap(), 0, pixels );
    }

    /* Cleanup after us */
    if (hImage != NULL)
    {
        DeleteObject (hImage);
    }

    if (hMemDC != NULL)
    {
        DeleteDC (hMemDC);
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
    HDC hDC;
    BITMAP bm;
    PDWORD pixels = NULL;
    HGDIOBJ hBmp;
    BITMAPINFO bmi;
    int retvalue = 0;

    /* Get our hdc for the surface */
    hDC = (HDC)lpUnLockData->lpDDSurface->lpSurfMore->lpDD_lcl->hDC;

    /* Get our bitmap handle from hdc, we need it if we want extract the Bitmap pixel data */
    hBmp = GetCurrentObject(hDC, OBJ_BITMAP);

    /* Get our bitmap information from hBmp, we need it if we want extract the Bitmap pixel data */
    if (GetObject(hBmp, sizeof(BITMAP), &bm) )
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
            /* Upload the bitmap bits data from palete bitmap */
            retvalue = SetDIBits(hDC, hBmp, 0, bm.bmHeight, pixels, &bmi, DIB_PAL_COLORS);
        }
        else
        {
            /* Upload the bitmap bits data from RGB bitmap */
            retvalue = SetDIBits(hDC, hBmp, 0, bm.bmHeight, pixels, &bmi, DIB_RGB_COLORS);
        }
    }
    if (retvalue)
    {
        lpUnLockData->ddRVal = DD_OK;
    }
    return DDHAL_DRIVER_HANDLED;
}

DWORD CALLBACK HelDdSurfUpdateOverlay(LPDDHAL_UPDATEOVERLAYDATA lpUpDateOveryLayData)
{
    DX_STUB;
}

