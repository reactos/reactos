/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS GDI32
 * PURPOSE:          GDI DirectX inteface
 * FILE:             lib/gdi32/misc/gdientry.c
 * PROGRAMERS:       Alex Ionescu (alex@relsoft.net)
 *                   Magnus Olsen (magnus@greatlord.com)
 */

/* INCLUDES ******************************************************************/

#include "precomp.h"
#include <ddraw.h>
#include <ddrawi.h>
#include <ddrawint.h>
#include <ddrawgdi.h>
#include <ntgdi.h>

/* DATA **********************************************************************/

HANDLE ghDirectDraw;
ULONG gcDirectDraw;

#define GetDdHandle(Handle) ((HANDLE)Handle ? (HANDLE)Handle : ghDirectDraw)


/* CALLBACKS *****************************************************************/

/*
 * Dd Surface Callbacks
 */
DWORD
WINAPI
DdAddAttachedSurface(LPDDHAL_ADDATTACHEDSURFACEDATA Attach)
{
    /* Call win32k */
    return NtGdiDdAddAttachedSurface((HANDLE)Attach->lpDDSurface->hDDSurface,
                                      (HANDLE)Attach->lpSurfAttached->hDDSurface,
                                      (PDD_ADDATTACHEDSURFACEDATA)Attach);
}

DWORD
WINAPI
DdBlt(LPDDHAL_BLTDATA Blt)
{
    HANDLE Surface = 0;

    /* Use the right surface */
    if (Blt->lpDDSrcSurface) Surface = (HANDLE)Blt->lpDDSrcSurface->hDDSurface;

    /* Call win32k */
    return NtGdiDdBlt((HANDLE)Blt->lpDDDestSurface->hDDSurface, Surface, (PDD_BLTDATA)Blt);
}

DWORD
APIENTRY
DdDestroySurface(LPDDHAL_DESTROYSURFACEDATA pDestroySurface)
{
    DWORD Return = DDHAL_DRIVER_NOTHANDLED;
    BOOL RealDestroy = TRUE;
    LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal;

    /* Get the local surface */
    pSurfaceLocal = pDestroySurface->lpDDSurface;

    /* Make sure there's a surface */
    if (pSurfaceLocal->hDDSurface)
    {
        /* Check if we shoudl really destroy it */
        if ((pSurfaceLocal->dwFlags & DDRAWISURF_DRIVERMANAGED) &&
            (pSurfaceLocal->dwFlags & DDRAWISURF_INVALID))
        {
            RealDestroy = FALSE;
        }
        
        /* Call win32k */
        Return = NtGdiDdDestroySurface((HANDLE)pSurfaceLocal->hDDSurface,
                                       RealDestroy);
    }

    return Return;
}

DWORD
WINAPI
DdFlip(LPDDHAL_FLIPDATA Flip)
{
    /* Call win32k */
	
    return NtGdiDdFlip( (HANDLE)Flip->lpSurfCurr->hDDSurface,
                        (HANDLE)Flip->lpSurfTarg->hDDSurface,
   /* FIXME  the two last should be current left handler */ 
						(HANDLE)Flip->lpSurfCurr->hDDSurface,
                        (HANDLE)Flip->lpSurfTarg->hDDSurface,
                        (PDD_FLIPDATA)Flip);
}

DWORD
WINAPI
DdLock(LPDDHAL_LOCKDATA Lock)
{
    /* Call win32k */	
    return NtGdiDdLock((HANDLE)Lock->lpDDSurface->hDDSurface,
                        (PDD_LOCKDATA)Lock,
						(HANDLE)Lock->lpDDSurface->hDC);
}

DWORD
WINAPI
DdUnlock(LPDDHAL_UNLOCKDATA Unlock)
{
    /* Call win32k */
    return NtGdiDdUnlock((HANDLE)Unlock->lpDDSurface->hDDSurface,
                          (PDD_UNLOCKDATA)Unlock);
}

DWORD
WINAPI
DdGetBltStatus(LPDDHAL_GETBLTSTATUSDATA GetBltStatus)
{
    /* Call win32k */
    return NtGdiDdGetBltStatus((HANDLE)GetBltStatus->lpDDSurface->hDDSurface,
                                (PDD_GETBLTSTATUSDATA)GetBltStatus);
}

DWORD
WINAPI
DdGetFlipStatus(LPDDHAL_GETFLIPSTATUSDATA GetFlipStatus)
{
    /* Call win32k */
    return NtGdiDdGetFlipStatus((HANDLE)GetFlipStatus->lpDDSurface->hDDSurface,
                                 (PDD_GETFLIPSTATUSDATA)GetFlipStatus);
}

DWORD
APIENTRY
DdUpdateOverlay(LPDDHAL_UPDATEOVERLAYDATA UpdateOverlay)
{
    /* We have to handle this manually here */
    if (UpdateOverlay->dwFlags & DDOVER_KEYDEST)
    {
        /* Use the override */
        UpdateOverlay->dwFlags &= ~DDOVER_KEYDEST;
        UpdateOverlay->dwFlags |=  DDOVER_KEYDESTOVERRIDE;

        /* Set the overlay */
        UpdateOverlay->overlayFX.dckDestColorkey =
            UpdateOverlay->lpDDDestSurface->ddckCKDestOverlay;
    }
    if (UpdateOverlay->dwFlags & DDOVER_KEYSRC)
    {
        /* Use the override */
        UpdateOverlay->dwFlags &= ~DDOVER_KEYSRC;
        UpdateOverlay->dwFlags |=  DDOVER_KEYSRCOVERRIDE;

        /* Set the overlay */
        UpdateOverlay->overlayFX.dckSrcColorkey =
            UpdateOverlay->lpDDSrcSurface->ddckCKSrcOverlay;
    }

    /* Call win32k */
    return NtGdiDdUpdateOverlay((HANDLE)UpdateOverlay->lpDDDestSurface->hDDSurface,
                                 (HANDLE)UpdateOverlay->lpDDSrcSurface->hDDSurface,
                                 (PDD_UPDATEOVERLAYDATA)UpdateOverlay);
}

DWORD
APIENTRY
DdSetOverlayPosition(LPDDHAL_SETOVERLAYPOSITIONDATA SetOverlayPosition)
{
    /* Call win32k */
    return NtGdiDdSetOverlayPosition((HANDLE)SetOverlayPosition->
                                      lpDDSrcSurface->hDDSurface,
                                      (HANDLE)SetOverlayPosition->
                                      lpDDDestSurface->hDDSurface,
                                      (PDD_SETOVERLAYPOSITIONDATA)
                                      SetOverlayPosition);
}

/*
 * Dd Callbacks
 */
DWORD
WINAPI
DdWaitForVerticalBlank(LPDDHAL_WAITFORVERTICALBLANKDATA WaitForVerticalBlank)
{
    /* Call win32k */
    return NtGdiDdWaitForVerticalBlank(GetDdHandle(
                                       WaitForVerticalBlank->lpDD->hDD),
                                       (PDD_WAITFORVERTICALBLANKDATA)
                                       WaitForVerticalBlank);
}

DWORD
WINAPI
DdCanCreateSurface(LPDDHAL_CANCREATESURFACEDATA CanCreateSurface)
{
    /* Call win32k */
    return NtGdiDdCanCreateSurface(GetDdHandle(CanCreateSurface->lpDD->hDD),
                                   (PDD_CANCREATESURFACEDATA)CanCreateSurface);
}

DWORD
APIENTRY
DdCreateSurface(LPDDHAL_CREATESURFACEDATA pCreateSurface)
{
    DWORD Return = DDHAL_DRIVER_NOTHANDLED;
    ULONG SurfaceCount = pCreateSurface->dwSCnt;
    DD_SURFACE_LOCAL DdSurfaceLocal;
    DD_SURFACE_MORE DdSurfaceMore;
    DD_SURFACE_GLOBAL DdSurfaceGlobal;

    HANDLE hPrevSurface, hSurface;


    PDD_SURFACE_LOCAL pDdSurfaceLocal = NULL;
    PDD_SURFACE_MORE pDdSurfaceMore = NULL;
    PDD_SURFACE_GLOBAL pDdSurfaceGlobal = NULL;

    PDD_SURFACE_LOCAL ptmpDdSurfaceLocal = NULL;
    PDD_SURFACE_MORE ptmpDdSurfaceMore = NULL;
    PDD_SURFACE_GLOBAL ptmpDdSurfaceGlobal = NULL;
    PHANDLE phSurface = NULL, puhSurface = NULL;
    ULONG i;
    LPDDSURFACEDESC pSurfaceDesc = NULL;

    /* Check how many surfaces there are */
    if (SurfaceCount != 1)
    {
        /* We'll have to allocate more data, our stack isn't big enough */

    }
    else
    {
        /* We'll use what we have on the stack */
        pDdSurfaceLocal = &DdSurfaceLocal;
        pDdSurfaceMore = &DdSurfaceMore;
        pDdSurfaceGlobal = &DdSurfaceGlobal;
        phSurface = &hPrevSurface;
        puhSurface = &hSurface;
        
        /* Clear the structures */
        RtlZeroMemory(&DdSurfaceLocal, sizeof(DdSurfaceLocal));
        RtlZeroMemory(&DdSurfaceGlobal, sizeof(DdSurfaceGlobal));
        RtlZeroMemory(&DdSurfaceMore, sizeof(DdSurfaceMore));  
    }

    /* check if we got a surface or not */
    if (SurfaceCount!=0)
    {
        /* Loop for each surface */
        ptmpDdSurfaceGlobal = pDdSurfaceGlobal;
        ptmpDdSurfaceLocal = pDdSurfaceLocal;
        ptmpDdSurfaceMore = pDdSurfaceMore;
        pSurfaceDesc = pCreateSurface->lpDDSurfaceDesc;

        for (i = 0; i < SurfaceCount; i++)
        {
            LPDDRAWI_DDRAWSURFACE_LCL lcl = pCreateSurface->lplpSList[i];
            LPDDRAWI_DDRAWSURFACE_GBL gpl = pCreateSurface->lplpSList[i]->lpGbl;

            phSurface[i] = (HANDLE)lcl->hDDSurface;
            ptmpDdSurfaceLocal->ddsCaps.dwCaps = lcl->ddsCaps.dwCaps;

            ptmpDdSurfaceLocal->dwFlags = (ptmpDdSurfaceLocal->dwFlags & 
                                          (0xB0000000 | DDRAWISURF_INMASTERSPRITELIST |
                                           DDRAWISURF_HELCB | DDRAWISURF_FRONTBUFFER |
                                           DDRAWISURF_BACKBUFFER | DDRAWISURF_INVALID |
                                           DDRAWISURF_DCIBUSY | DDRAWISURF_DCILOCK)) |
                                          (lcl->dwFlags & DDRAWISURF_DRIVERMANAGED);

            ptmpDdSurfaceGlobal->wWidth = gpl->wWidth;
            ptmpDdSurfaceGlobal->wHeight = gpl->wHeight;
            ptmpDdSurfaceGlobal->lPitch = gpl->lPitch;
            ptmpDdSurfaceGlobal->fpVidMem = gpl->fpVidMem;
            ptmpDdSurfaceGlobal->dwBlockSizeX = gpl->dwBlockSizeX;
            ptmpDdSurfaceGlobal->dwBlockSizeY = gpl->dwBlockSizeY;

            if (lcl->dwFlags & DDRAWISURF_HASPIXELFORMAT)
            {
                RtlCopyMemory( &ptmpDdSurfaceGlobal->ddpfSurface ,
                               &gpl->ddpfSurface, 
                               sizeof(DDPIXELFORMAT));

                ptmpDdSurfaceGlobal->ddpfSurface.dwSize = sizeof(DDPIXELFORMAT);
            }
            else
            {
                RtlCopyMemory( &ptmpDdSurfaceGlobal->ddpfSurface ,
                               &gpl->lpDD->vmiData.ddpfDisplay,
                               sizeof(DDPIXELFORMAT));
            }

            if (lcl->lpSurfMore)
            {
                ptmpDdSurfaceMore->ddsCapsEx.dwCaps2 = lcl->lpSurfMore->ddsCapsEx.dwCaps2;
                ptmpDdSurfaceMore->ddsCapsEx.dwCaps3 = lcl->lpSurfMore->ddsCapsEx.dwCaps3;
                ptmpDdSurfaceMore->ddsCapsEx.dwCaps4 = lcl->lpSurfMore->ddsCapsEx.dwCaps4;
                ptmpDdSurfaceMore->dwSurfaceHandle = (DWORD) pCreateSurface->lplpSList[i]->dbnOverlayNode.object_int;
            }

            /* FIXME count to next SurfaceCount for 
               ptmpDdSurfaceGlobal = pDdSurfaceGlobal;
               ptmpDdSurfaceLocal = pDdSurfaceLocal;
               ptmpDdSurfaceMore = pDdSurfaceMore;

               we only support one surface create at moment
             */
        }
    }

    /* Call win32k now */
    pCreateSurface->ddRVal = DDERR_GENERIC;

    Return = NtGdiDdCreateSurface(GetDdHandle(pCreateSurface->lpDD->hDD),
                                    (HANDLE *)phSurface,
                                     pSurfaceDesc,
                                     pDdSurfaceGlobal,
                                     pDdSurfaceLocal,
                                     pDdSurfaceMore,
                                     (PDD_CREATESURFACEDATA)pCreateSurface,
                                     puhSurface);

    if (SurfaceCount == 0)
    {
        pCreateSurface->ddRVal = DDERR_GENERIC;
    }
    else
    {
        ptmpDdSurfaceMore = pDdSurfaceMore;
        ptmpDdSurfaceGlobal = pDdSurfaceGlobal;
        ptmpDdSurfaceLocal = pDdSurfaceLocal;

        for (i=0;i<SurfaceCount;i++)
        {
            LPDDRAWI_DDRAWSURFACE_LCL lcl = pCreateSurface->lplpSList[i];
            LPDDRAWI_DDRAWSURFACE_GBL gpl = pCreateSurface->lplpSList[i]->lpGbl;

            gpl->lPitch = ptmpDdSurfaceGlobal->lPitch;
            gpl->fpVidMem = ptmpDdSurfaceGlobal->fpVidMem;
            gpl->dwBlockSizeX = ptmpDdSurfaceGlobal->dwBlockSizeX;
            gpl->dwBlockSizeY = ptmpDdSurfaceGlobal->dwBlockSizeY;

            if (lcl->dwFlags & DDRAWISURF_HASPIXELFORMAT)
            {
                RtlCopyMemory( &gpl->ddpfSurface, &ptmpDdSurfaceGlobal->ddpfSurface , sizeof(DDPIXELFORMAT));
            }

            if (pCreateSurface->ddRVal != DD_OK)
            {
                gpl->fpVidMem = 0;
                if (lcl->hDDSurface)
                {
                    NtGdiDdDeleteSurfaceObject( (HANDLE)lcl->hDDSurface);
                }
                lcl->hDDSurface = 0;
            }
            else
            {

                lcl->hDDSurface = (ULONG_PTR) puhSurface[i];
            }

            lcl->ddsCaps.dwCaps = ptmpDdSurfaceLocal->ddsCaps.dwCaps;
            if (lcl->lpSurfMore)
            {
                lcl->lpSurfMore->ddsCapsEx.dwCaps2 = ptmpDdSurfaceMore->ddsCapsEx.dwCaps2;
                lcl->lpSurfMore->ddsCapsEx.dwCaps3 = ptmpDdSurfaceMore->ddsCapsEx.dwCaps3;
                lcl->lpSurfMore->ddsCapsEx.dwCaps4 = ptmpDdSurfaceMore->ddsCapsEx.dwCaps4;
            }
            /* FIXME count to next SurfaceCount for 
               ptmpDdSurfaceGlobal = pDdSurfaceGlobal;
               ptmpDdSurfaceLocal = pDdSurfaceLocal;
               ptmpDdSurfaceMore = pDdSurfaceMore;
               we only support one surface create at moment
             */
        }
    }

    /* Check if we have to free all our local allocations */
    if (SurfaceCount > 1)
    {
        /* FIXME: */
    }

    /* Return */
    return Return;
}

DWORD
APIENTRY
DdSetColorKey(LPDDHAL_SETCOLORKEYDATA pSetColorKey)
{
    /* Call win32k */
    return NtGdiDdSetColorKey((HANDLE)pSetColorKey->lpDDSurface->hDDSurface,
                               (PDD_SETCOLORKEYDATA)pSetColorKey);
}

DWORD
APIENTRY
DdGetScanLine(LPDDHAL_GETSCANLINEDATA pGetScanLine)
{
    /* Call win32k */
    return NtGdiDdGetScanLine(GetDdHandle(pGetScanLine->lpDD->hDD),
                               (PDD_GETSCANLINEDATA)pGetScanLine);
}

/* PRIVATE FUNCTIONS *********************************************************/
static ULONG RemberDdQueryDisplaySettingsUniquenessID = 0;

BOOL
WINAPI
bDDCreateSurface(LPDDRAWI_DDRAWSURFACE_LCL pSurface, 
                 BOOL bComplete)
{
    DD_SURFACE_LOCAL SurfaceLocal;
    DD_SURFACE_GLOBAL SurfaceGlobal;
    DD_SURFACE_MORE SurfaceMore;

    /* Zero struct */
    RtlZeroMemory(&SurfaceLocal, sizeof(DD_SURFACE_LOCAL));
    RtlZeroMemory(&SurfaceGlobal, sizeof(DD_SURFACE_GLOBAL));
    RtlZeroMemory(&SurfaceMore, sizeof(DD_SURFACE_MORE));

    /* Set up SurfaceLocal struct */
    SurfaceLocal.ddsCaps.dwCaps = pSurface->ddsCaps.dwCaps;
    SurfaceLocal.dwFlags = pSurface->dwFlags;

    /* Set up SurfaceMore struct */
    RtlMoveMemory(&SurfaceMore.ddsCapsEx,
                  &pSurface->ddckCKDestBlt,
                  sizeof(DDSCAPSEX));
    SurfaceMore.dwSurfaceHandle = (DWORD)pSurface->dbnOverlayNode.object_int->lpVtbl;

    /* Set up SurfaceGlobal struct */
    SurfaceGlobal.fpVidMem = pSurface->lpGbl->fpVidMem;
    SurfaceGlobal.dwLinearSize = pSurface->lpGbl->dwLinearSize;
    SurfaceGlobal.wHeight = pSurface->lpGbl->wHeight;
    SurfaceGlobal.wWidth = pSurface->lpGbl->wWidth;

    /* Check if we have a pixel format */
    if (pSurface->dwFlags & DDSD_PIXELFORMAT)
    {	
        /* Use global one */
        SurfaceGlobal.ddpfSurface = pSurface->lpGbl->lpDD->vmiData.ddpfDisplay;
        SurfaceGlobal.ddpfSurface.dwSize = sizeof(DDPIXELFORMAT);
    }
    else
    {
        /* Use local one */
        SurfaceGlobal.ddpfSurface = pSurface->lpGbl->lpDD->vmiData.ddpfDisplay;
    }

    /* Create the object */
    pSurface->hDDSurface = (DWORD)NtGdiDdCreateSurfaceObject(GetDdHandle(pSurface->lpGbl->lpDD->hDD),
                                                             (HANDLE)pSurface->hDDSurface,
                                                             &SurfaceLocal,
                                                             &SurfaceMore,
                                                             &SurfaceGlobal,
                                                             bComplete);

    /* Return status */
    if (pSurface->hDDSurface) return TRUE;
    return FALSE;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 *
 * GDIEntry 1 
 */
BOOL 
WINAPI 
DdCreateDirectDrawObject(LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal,
                         HDC hdc)
{  
    BOOL Return = FALSE;

    /* Check if the global hDC (hdc == 0) is being used */
    if (!hdc)
  {
        /* We'll only allow this if the global object doesn't exist yet */
        if (!ghDirectDraw)
        {
            /* Create the DC */
            if ((hdc = CreateDC(L"Display", NULL, NULL, NULL)))
            {
                /* Create the DDraw Object */
                ghDirectDraw = NtGdiDdCreateDirectDrawObject(hdc);

                /* Delete our DC */                
                NtGdiDeleteObjectApp(hdc);
            }
        }

        /* If we created the object, or had one ...*/
        if (ghDirectDraw)
        {
            /* Increase count and set success */
            gcDirectDraw++;
            Return = TRUE;
        }

        /* Zero the handle */
        pDirectDrawGlobal->hDD = 0;
    }
    else
    {
        /* Using the per-process object, so create it */
    pDirectDrawGlobal->hDD = (ULONG_PTR)NtGdiDdCreateDirectDrawObject(hdc); 
    
        /* Set the return value */
        Return = pDirectDrawGlobal->hDD ? TRUE : FALSE;
    }

    /* Return to caller */
    return Return;
}

/*
 * @implemented
 *
 * GDIEntry 2
 */
BOOL
WINAPI
DdQueryDirectDrawObject(LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal,
                        LPDDHALINFO pHalInfo,
                        LPDDHAL_DDCALLBACKS pDDCallbacks,
                        LPDDHAL_DDSURFACECALLBACKS pDDSurfaceCallbacks,
                        LPDDHAL_DDPALETTECALLBACKS pDDPaletteCallbacks,
                        LPD3DHAL_CALLBACKS pD3dCallbacks,
                        LPD3DHAL_GLOBALDRIVERDATA pD3dDriverData,
                        LPDDHAL_DDEXEBUFCALLBACKS pD3dBufferCallbacks,
                        LPDDSURFACEDESC pD3dTextureFormats,
                        LPDWORD pdwFourCC,
                        LPVIDMEM pvmList)
    {
    PVIDEOMEMORY VidMemList = NULL;
    DD_HALINFO HalInfo;
    D3DNTHAL_CALLBACKS D3dCallbacks;
    D3DNTHAL_GLOBALDRIVERDATA D3dDriverData;
    DD_D3DBUFCALLBACKS D3dBufferCallbacks;
    DWORD CallbackFlags[3];
    DWORD dwNumHeaps=0, FourCCs=0;
    DWORD Flags;

    /* Clear the structures */
    RtlZeroMemory(&HalInfo, sizeof(DD_HALINFO));
    RtlZeroMemory(&D3dCallbacks, sizeof(D3DNTHAL_CALLBACKS));
    RtlZeroMemory(&D3dDriverData, sizeof(D3DNTHAL_GLOBALDRIVERDATA));
    RtlZeroMemory(&D3dBufferCallbacks, sizeof(DD_D3DBUFCALLBACKS));
    RtlZeroMemory(CallbackFlags, sizeof(DWORD)*3);

    pvmList = NULL;

    /* Do the query */
    if (!NtGdiDdQueryDirectDrawObject(GetDdHandle(pDirectDrawGlobal->hDD),
                                      &HalInfo,
                                      CallbackFlags,
                                      &D3dCallbacks,
                                      &D3dDriverData,
                                      &D3dBufferCallbacks,
                                      pD3dTextureFormats,
                                      &dwNumHeaps,
                                      VidMemList,
                                      &FourCCs,
                                      pdwFourCC))
    {
        /* We failed, free the memory and return */
        return FALSE;
    }

    /* Clear the incoming pointer */
    RtlZeroMemory(pHalInfo, sizeof(DDHALINFO));

    /* Convert all the data */
    pHalInfo->dwSize = sizeof(DDHALINFO);
    pHalInfo->lpDDCallbacks = pDDCallbacks;
    pHalInfo->lpDDSurfaceCallbacks = pDDSurfaceCallbacks;
    pHalInfo->lpDDPaletteCallbacks = pDDPaletteCallbacks;

    /* Check for NT5+ D3D Data */
    if (D3dCallbacks.dwSize && D3dDriverData.dwSize)
    {
        /* Write these down */
        pHalInfo->lpD3DGlobalDriverData = (ULONG_PTR)pD3dDriverData;
        pHalInfo->lpD3DHALCallbacks = (ULONG_PTR)pD3dCallbacks;

        /* Check for Buffer Callbacks */
        if (D3dBufferCallbacks.dwSize)
        {
            /* Write this one too */
            pHalInfo->lpDDExeBufCallbacks = pD3dBufferCallbacks;
        }
    }

    /* Continue converting the rest */
    pHalInfo->vmiData.dwFlags = HalInfo.vmiData.dwFlags;
    pHalInfo->vmiData.dwDisplayWidth = HalInfo.vmiData.dwDisplayWidth;
    pHalInfo->vmiData.dwDisplayHeight = HalInfo.vmiData.dwDisplayHeight;
    pHalInfo->vmiData.lDisplayPitch = HalInfo.vmiData.lDisplayPitch;
    pHalInfo->vmiData.fpPrimary = 0;

    RtlCopyMemory( &pHalInfo->vmiData.ddpfDisplay,
                   &HalInfo.vmiData.ddpfDisplay,
                   sizeof(DDPIXELFORMAT));

    pHalInfo->vmiData.dwOffscreenAlign = HalInfo.vmiData.dwOffscreenAlign;
    pHalInfo->vmiData.dwOverlayAlign = HalInfo.vmiData.dwOverlayAlign;
    pHalInfo->vmiData.dwTextureAlign = HalInfo.vmiData.dwTextureAlign;
    pHalInfo->vmiData.dwZBufferAlign = HalInfo.vmiData.dwZBufferAlign;
    pHalInfo->vmiData.dwAlphaAlign = HalInfo.vmiData.dwAlphaAlign;
    pHalInfo->vmiData.dwNumHeaps = 0;
    pHalInfo->vmiData.pvmList = pvmList;

    RtlCopyMemory( &pHalInfo->ddCaps, &HalInfo.ddCaps,sizeof(DDCORECAPS ));

    pHalInfo->ddCaps.dwNumFourCCCodes = FourCCs;
    pHalInfo->lpdwFourCC = pdwFourCC;
    pHalInfo->ddCaps.dwRops[6] = 0x1000;

    /* FIXME implement DdGetDriverInfo */
    //  pHalInfo->dwFlags = HalInfo.dwFlags | DDHALINFO_GETDRIVERINFOSET;
    //  pHalInfo->GetDriverInfo = DdGetDriverInfo;

    /* Now check if we got any DD callbacks */
    if (pDDCallbacks)
    {
        /* Zero the structure */
        RtlZeroMemory(pDDCallbacks, sizeof(DDHAL_DDCALLBACKS));

        /* Set the flags for this structure */
        Flags = CallbackFlags[0];

        /* Write the header */
        pDDCallbacks->dwSize = sizeof(DDHAL_DDCALLBACKS);
        pDDCallbacks->dwFlags = Flags;

        /* Now write the pointers, if applicable */
        if (Flags & DDHAL_CB32_CREATESURFACE)
        {
            pDDCallbacks->CreateSurface = DdCreateSurface;
        }
        if (Flags & DDHAL_CB32_WAITFORVERTICALBLANK)
        {
            pDDCallbacks->WaitForVerticalBlank = DdWaitForVerticalBlank;
        }
        if (Flags & DDHAL_CB32_CANCREATESURFACE)
        {
            pDDCallbacks->CanCreateSurface = DdCanCreateSurface;
        }
        if (Flags & DDHAL_CB32_GETSCANLINE)
        {
            pDDCallbacks->GetScanLine = DdGetScanLine;
        }
    }

    /* Check for DD Surface Callbacks */
    if (pDDSurfaceCallbacks)
    {
        /* Zero the structures */
        RtlZeroMemory(pDDSurfaceCallbacks, sizeof(DDHAL_DDSURFACECALLBACKS));

        /* Set the flags for this one */
        Flags = CallbackFlags[1];

        /* Write the header, note that some functions are always exposed */
        pDDSurfaceCallbacks->dwSize  = sizeof(DDHAL_DDSURFACECALLBACKS);

        pDDSurfaceCallbacks->dwFlags = Flags;
        /*
        pDDSurfaceCallBacks->dwFlags = (DDHAL_SURFCB32_LOCK |
                                        DDHAL_SURFCB32_UNLOCK |
                                        DDHAL_SURFCB32_SETCOLORKEY |
                                        DDHAL_SURFCB32_DESTROYSURFACE) | Flags;
        */

        /* Write the always-on functions */
        pDDSurfaceCallbacks->Lock = DdLock;
        pDDSurfaceCallbacks->Unlock = DdUnlock;
        pDDSurfaceCallbacks->SetColorKey = DdSetColorKey;
        pDDSurfaceCallbacks->DestroySurface = DdDestroySurface;

        /* Write the optional ones */
        if (Flags & DDHAL_SURFCB32_FLIP)
        {
            pDDSurfaceCallbacks->Flip = DdFlip;
        }
        if (Flags & DDHAL_SURFCB32_BLT)
        {
            pDDSurfaceCallbacks->Blt = DdBlt;
        }
        if (Flags & DDHAL_SURFCB32_GETBLTSTATUS)
        {
            pDDSurfaceCallbacks->GetBltStatus = DdGetBltStatus;
        }
        if (Flags & DDHAL_SURFCB32_GETFLIPSTATUS)
        {
            pDDSurfaceCallbacks->GetFlipStatus = DdGetFlipStatus;
        }
        if (Flags & DDHAL_SURFCB32_UPDATEOVERLAY)
        {
            pDDSurfaceCallbacks->UpdateOverlay = DdUpdateOverlay;
        }
        if (Flags & DDHAL_SURFCB32_SETOVERLAYPOSITION)
        {
            pDDSurfaceCallbacks->SetOverlayPosition = DdSetOverlayPosition;
        }
        if (Flags & DDHAL_SURFCB32_ADDATTACHEDSURFACE)
        {
            pDDSurfaceCallbacks->AddAttachedSurface = DdAddAttachedSurface;
        }
    }

    /* Check for DD Palette Callbacks */
    if (pDDPaletteCallbacks)
    {
        /* Zero the struct */
        RtlZeroMemory(pDDPaletteCallbacks, sizeof(DDHAL_DDPALETTECALLBACKS));

        /* Get the flags for this one */
        Flags = CallbackFlags[2];

        /* Write the header */
        pDDPaletteCallbacks->dwSize  = sizeof(DDHAL_DDPALETTECALLBACKS);
        pDDPaletteCallbacks->dwFlags = Flags;
    }

    /* Check for D3D Callbacks */
    if (pD3dCallbacks)
    {
        /* Zero the struct */
        RtlZeroMemory(pD3dCallbacks, sizeof(D3DHAL_CALLBACKS));

        /* Check if we have one */
        if (D3dCallbacks.dwSize)
        {
            /* Write the header */
            pD3dCallbacks->dwSize  = sizeof(D3DHAL_CALLBACKS);

            /* Now check for each callback */
            if (D3dCallbacks.ContextCreate)
            {
                /* FIXME
                 pD3dCallbacks->ContextCreate = D3dContextCreate; 
                 */
            }
            if (D3dCallbacks.ContextDestroy)
            {
                pD3dCallbacks->ContextDestroy = (LPD3DHAL_CONTEXTDESTROYCB) NtGdiD3dContextDestroy;
            }
            if (D3dCallbacks.ContextDestroyAll)
            {
                /* FIXME 
                pD3dCallbacks->ContextDestroyAll = (LPD3DHAL_CONTEXTDESTROYALLCB) NtGdiD3dContextDestroyAll;
                */
            }
        }
    }

    /* Check for D3D Driver Data */
    if (pD3dDriverData)
    {
        /* Copy the struct */
        RtlMoveMemory(pD3dDriverData,
                      &D3dDriverData,
                      sizeof(D3DHAL_GLOBALDRIVERDATA));

        /* Write the pointer to the texture formats */
        pD3dDriverData->lpTextureFormats = pD3dTextureFormats;
    }

    /* FIXME: Check for D3D Buffer Callbacks */

  return TRUE;
}

/*
 * @implemented
 *
 * GDIEntry 3
 */
BOOL 
WINAPI
DdDeleteDirectDrawObject(LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal)
{
    BOOL Return = FALSE;

    /* If this is the global object */
    if(pDirectDrawGlobal->hDD)
    {
        /* Free it */
        Return = NtGdiDdDeleteDirectDrawObject((HANDLE)pDirectDrawGlobal->hDD);
        if (Return == TRUE)
        {
            pDirectDrawGlobal->hDD = 0;
        }
    }
    else if (ghDirectDraw)
    {
        /* Always success here */
        Return = TRUE;

        /* Make sure this is the last instance */
        if (!(--gcDirectDraw))
        {
            /* Delete the object */
            Return = NtGdiDdDeleteDirectDrawObject(ghDirectDraw);
            if (Return == TRUE)
            {
                ghDirectDraw = 0;
            }
        }
    }

    /* Return */
    return Return;
}

/*
 * @implemented
 *
 * GDIEntry 4
 */
BOOL 
WINAPI 
DdCreateSurfaceObject( LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal,
                       BOOL bPrimarySurface)
{
	return bDDCreateSurface(pSurfaceLocal, TRUE);
    //return bDdCreateSurfaceObject(pSurfaceLocal, TRUE);
}


/*
 * @implemented
 *
 * GDIEntry 5
 */
BOOL 
WINAPI 
DdDeleteSurfaceObject(LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal)
{
    BOOL Return = FALSE;

    /* Make sure there is one */
    if (pSurfaceLocal->hDDSurface)
    {
        /* Delete it */
        Return = NtGdiDdDeleteSurfaceObject((HANDLE)pSurfaceLocal->hDDSurface);
        pSurfaceLocal->hDDSurface = 0;
    }

    return Return;
}

/*
 * @implemented
 *
 * GDIEntry 6
 */
BOOL 
WINAPI 
DdResetVisrgn(LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal, 
              HWND hWnd)
{
    /* Call win32k directly */
    return NtGdiDdResetVisrgn((HANDLE) pSurfaceLocal->hDDSurface, hWnd);
}

/*
 * @implemented
 *
 * GDIEntry 7
 */
HDC
WINAPI
DdGetDC(LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal,
        LPPALETTEENTRY pColorTable)
{
    /* Call win32k directly */
    return NtGdiDdGetDC(pColorTable, (HANDLE) pSurfaceLocal->hDDSurface);
}

/*
 * @implemented
 *
 * GDIEntry 8
 */
BOOL
WINAPI
DdReleaseDC(LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal)
{
    /* Call win32k directly */
    return NtGdiDdReleaseDC((HANDLE) pSurfaceLocal->hDDSurface);
}

/*
 * @unimplemented
 * GDIEntry 9
 */
HBITMAP
STDCALL
DdCreateDIBSection(HDC hdc,
                   CONST BITMAPINFO *pbmi,
                   UINT iUsage,
                   VOID **ppvBits,
                   HANDLE hSectionApp,
                   DWORD dwOffset)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

/*
 * @implemented
 *
 * GDIEntry 10
 */
BOOL 
WINAPI 
DdReenableDirectDrawObject(LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal,
                           BOOL *pbNewMode)
{
    /* Call win32k directly */
    return NtGdiDdReenableDirectDrawObject(GetDdHandle(pDirectDrawGlobal->hDD),
                                           pbNewMode);
} 


/*
 * @implemented
 *
 * GDIEntry 11
 */
BOOL 
STDCALL 
DdAttachSurface( LPDDRAWI_DDRAWSURFACE_LCL pSurfaceFrom,
                 LPDDRAWI_DDRAWSURFACE_LCL pSurfaceTo)
{
    /* Create Surface if it does not exits one */
    if (!pSurfaceFrom->hDDSurface)
    {
        if (!bDDCreateSurface(pSurfaceFrom, FALSE))
        {
            return FALSE;
        }
    }

    /* Create Surface if it does not exits one */
    if (!pSurfaceTo->hDDSurface)
    {
        if (!bDDCreateSurface(pSurfaceTo, FALSE))
        {
            return FALSE;
        }
    }

    /* Call win32k */
    return NtGdiDdAttachSurface((HANDLE)pSurfaceFrom->hDDSurface,
                                (HANDLE)pSurfaceTo->hDDSurface);
}

/*
 * @implemented
 *
 * GDIEntry 12
 */
VOID
STDCALL
DdUnattachSurface(LPDDRAWI_DDRAWSURFACE_LCL pSurface,
                  LPDDRAWI_DDRAWSURFACE_LCL pSurfaceAttached)
{
    /* Call win32k */
    NtGdiDdUnattachSurface((HANDLE)pSurface->hDDSurface,
                           (HANDLE)pSurfaceAttached->hDDSurface);
}

/*
 * @implemented
 *
 * GDIEntry 13
 */
ULONG
STDCALL 
DdQueryDisplaySettingsUniqueness()
{
 return RemberDdQueryDisplaySettingsUniquenessID;
}

/*
 * @implemented
 *
 * GDIEntry 14
 */
HANDLE 
WINAPI 
DdGetDxHandle(LPDDRAWI_DIRECTDRAW_LCL pDDraw,
              LPDDRAWI_DDRAWSURFACE_LCL pSurface,
              BOOL bRelease)
{
    HANDLE hDD = NULL;
    HANDLE hSurface = (HANDLE)pSurface->hDDSurface;

    /* Check if we already have a surface */
    if (!pSurface)
    {
        /* We don't have one, use the DirectDraw Object handle instead */
        hSurface = NULL;
        hDD = GetDdHandle(pDDraw->lpGbl->hDD);
     }

    /* Call the API */
    return (HANDLE)NtGdiDdGetDxHandle(hDD, hSurface, bRelease);
}

/*
 * @implemented
 *
 * GDIEntry 15
 */
BOOL
WINAPI
DdSetGammaRamp(LPDDRAWI_DIRECTDRAW_LCL pDDraw,
               HDC hdc,
               LPVOID lpGammaRamp)
{
    /* Call win32k directly */
    return NtGdiDdSetGammaRamp(GetDdHandle(pDDraw->lpGbl->hDD),
                               hdc,
                               lpGammaRamp);
}





