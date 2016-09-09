/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 dll/directx/ddraw/Surface/callbacks_surf_hel.c
 * PURPOSE:              HEL Callbacks For Surface APIs
 * PROGRAMMER:           Magnus Olsen
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

    /* ToDo tell ddraw internal this surface is locked */
    /* ToDo add support for dwFlags */


    /* Get our hdc for the active window */
    hDC = GetDC((HWND)lpLockData->lpDDSurface->lpSurfMore->lpDD_lcl->hFocusWnd);

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

            /* Allocate memory buffer for the bitmap pixel data  */
            cbBuffer = bm.bmWidthBytes * bm.bmHeight ;
            pixels = (PDWORD) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbBuffer );

            if (pixels != NULL)
            {
                /* Get the bitmap bits */
                GetBitmapBits(hImage,cbBuffer,pixels);

                /* Fixme HACK - check which member stores the HEL bitmap buffer */
                lpLockData->lpDDSurface->lpSurfMore->lpDDRAWReserved2 = pixels;

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
    /* This api is not doucmented by MS, keep it stubbed */
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
    HBITMAP hImage = NULL;

    HDC hMemDC = NULL;
    HBITMAP hDCBmp = NULL;
    BITMAP bm = {0};

    DX_WINDBG_trace();

    /* Get our hdc for the active window */
    hDC = GetDC((HWND)lpUnLockData->lpDDSurface->lpSurfMore->lpDD_lcl->hFocusWnd);

    if (hDC != NULL)
    {
        /* Create a memory bitmap to store a copy of current hdc surface */

        /* fixme the rcarea are not store in the struct yet so the data will look corupted */
        hImage = CreateCompatibleBitmap (hDC, lpUnLockData->lpDDSurface->lpGbl->wWidth, lpUnLockData->lpDDSurface->lpGbl->wHeight);

        /* Create a memory hdc so we can draw on our current memory bitmap */
        hMemDC = CreateCompatibleDC(hDC);

        if (hMemDC != NULL)
        {
            /* Select our memory bitmap to our memory hdc */
            hDCBmp = (HBITMAP) SelectObject (hMemDC, hImage);

            /* Get our memory bitmap information */
            GetObject(hImage, sizeof(BITMAP), &bm);

            SetBitmapBits(hImage,bm.bmWidthBytes * bm.bmHeight, lpUnLockData->lpDDSurface->lpSurfMore->lpDDRAWReserved2);

            BitBlt (hDC, 0, 0, bm.bmWidth, bm.bmHeight, hMemDC, 0, 0, SRCCOPY);

            SelectObject (hMemDC, hDCBmp);

            /* Setup return value */
             lpUnLockData->ddRVal = DD_OK;
        }
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

    if (lpUnLockData->lpDDSurface->lpSurfMore->lpDDRAWReserved2 != NULL)
    {
        HeapFree(GetProcessHeap(), 0, lpUnLockData->lpDDSurface->lpSurfMore->lpDDRAWReserved2 );
    }

    return DDHAL_DRIVER_HANDLED;
}

DWORD CALLBACK HelDdSurfUpdateOverlay(LPDDHAL_UPDATEOVERLAYDATA lpUpDateOveryLayData)
{
    DX_STUB;
}

