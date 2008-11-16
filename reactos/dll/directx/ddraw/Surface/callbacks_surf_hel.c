/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 ddraw/surface/callbacks_surf_hel.c
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

    if (lpBltData->dwFlags & DDBLT_COLORFILL)
    {

        HBRUSH hbr = CreateSolidBrush(lpBltData->bltFX.dwFillColor );

        FillRect( (HDC)lpBltData->lpDDDestSurface->lpSurfMore->lpDD_lcl->hDC, 
                  (CONST RECT *)&lpBltData->rDest,
                  hbr);

        DeleteObject(hbr);

        lpBltData->ddRVal = DD_OK;
     }
    
     else if (lpBltData->dwFlags  & DDBLT_ROP)
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
    DX_STUB;
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

