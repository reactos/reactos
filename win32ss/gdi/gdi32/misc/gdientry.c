/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS GDI32
 * PURPOSE:          GDI DirectX interface
 * FILE:             win32ss/gdi/gdi32/misc/gdientry.c
 * PROGRAMERS:       Alex Ionescu (alex@relsoft.net)
 *                   Magnus Olsen (magnus@greatlord.com)
 */

/* INCLUDES ******************************************************************/

#include <precomp.h>

#include <stdio.h>
#include <d3dhal.h>

/* DATA **********************************************************************/

HANDLE ghDirectDraw;
ULONG gcDirectDraw;

#define GetDdHandle(Handle) ((HANDLE)Handle ? (HANDLE)Handle : ghDirectDraw)



/* CALLBACKS *****************************************************************/

/*
 * @implemented
 *
 * DdAddAttachedSurface
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

/*
 * @implemented
 *
 * DdBlt
 */
DWORD
WINAPI
DdBlt(LPDDHAL_BLTDATA Blt)
{
    HANDLE Surface = 0;

    /* Use the right surface */
    if (Blt->lpDDSrcSurface)
    {
        Surface = (HANDLE)Blt->lpDDSrcSurface->hDDSurface;
    }

    /* Call win32k */
    return NtGdiDdBlt((HANDLE)Blt->lpDDDestSurface->hDDSurface, Surface, (PDD_BLTDATA)Blt);
}

/*
 * @implemented
 *
 * DdDestroySurface
 */
DWORD
WINAPI
DdDestroySurface(LPDDHAL_DESTROYSURFACEDATA pDestroySurface)
{
    DWORD Return = DDHAL_DRIVER_NOTHANDLED;
    BOOL RealDestroy;

    if (pDestroySurface->lpDDSurface->hDDSurface)
    {
        /* Check if we shoudl really destroy it */
        RealDestroy = !(pDestroySurface->lpDDSurface->dwFlags & DDRAWISURF_DRIVERMANAGED) ||
                      !(pDestroySurface->lpDDSurface->dwFlags & DDRAWISURF_INVALID);

        /* Call win32k */
        Return = NtGdiDdDestroySurface((HANDLE)pDestroySurface->lpDDSurface->hDDSurface, RealDestroy);
    }

    return Return;
}

/*
 * @implemented
 *
 * DdFlip
 */
DWORD
WINAPI
DdFlip(LPDDHAL_FLIPDATA Flip)
{
    /* Note :
    * See http://msdn2.microsoft.com/en-us/library/ms794213.aspx (DEAD_LINK) and
    * http://msdn2.microsoft.com/en-us/library/ms792675.aspx (DEAD_LINK)
    */

    HANDLE hSurfaceCurrentLeft = NULL;
    HANDLE hSurfaceTargetLeft = NULL;

    /* Auto flip off or on */
    if (Flip->dwFlags & DDFLIP_STEREO )
    {
        if ( (Flip->lpSurfTargLeft) &&
                (Flip->lpSurfCurrLeft))
        {
            /* Auto flip on */
            hSurfaceTargetLeft = (HANDLE) Flip->lpSurfTargLeft->hDDSurface;
            hSurfaceCurrentLeft = (HANDLE) Flip->lpSurfCurrLeft->hDDSurface;
        }
    }

    /* Call win32k */
    return NtGdiDdFlip( (HANDLE) Flip->lpSurfCurr->hDDSurface,
                        (HANDLE) Flip->lpSurfTarg->hDDSurface,
                        hSurfaceCurrentLeft,
                        hSurfaceTargetLeft,
                        (PDD_FLIPDATA) Flip);
}

/*
 * @implemented
 *
 * DdLock
 */
DWORD
WINAPI
DdLock(LPDDHAL_LOCKDATA Lock)
{

    /* Call win32k */
    return NtGdiDdLock((HANDLE)Lock->lpDDSurface->hDDSurface,
                       (PDD_LOCKDATA)Lock,
                       (HANDLE)Lock->lpDDSurface->hDC);
}

/*
 * @implemented
 *
 * DdUnlock
 */
DWORD
WINAPI
DdUnlock(LPDDHAL_UNLOCKDATA Unlock)
{
    /* Call win32k */
    return NtGdiDdUnlock((HANDLE)Unlock->lpDDSurface->hDDSurface,
                         (PDD_UNLOCKDATA)Unlock);
}

/*
 * @implemented
 *
 * DdGetBltStatus
 */
DWORD
WINAPI
DdGetBltStatus(LPDDHAL_GETBLTSTATUSDATA GetBltStatus)
{
    /* Call win32k */
    return NtGdiDdGetBltStatus((HANDLE)GetBltStatus->lpDDSurface->hDDSurface,
                               (PDD_GETBLTSTATUSDATA)GetBltStatus);
}

/*
 * @implemented
 *
 * DdGetBltStatus
 */
DWORD
WINAPI
DdGetFlipStatus(LPDDHAL_GETFLIPSTATUSDATA GetFlipStatus)
{
    /* Call win32k */
    return NtGdiDdGetFlipStatus((HANDLE)GetFlipStatus->lpDDSurface->hDDSurface,
                                (PDD_GETFLIPSTATUSDATA)GetFlipStatus);
}

/*
 * @implemented
 *
 * DdUpdateOverlay
 */
DWORD
WINAPI
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

/*
 * @implemented
 *
 * DdSetOverlayPosition
 */
DWORD
WINAPI
DdSetOverlayPosition(LPDDHAL_SETOVERLAYPOSITIONDATA SetOverlayPosition)
{
    /* Call win32k */
    return NtGdiDdSetOverlayPosition( (HANDLE)SetOverlayPosition->lpDDSrcSurface->hDDSurface,
                                      (HANDLE)SetOverlayPosition->lpDDDestSurface->hDDSurface,
                                      (PDD_SETOVERLAYPOSITIONDATA) SetOverlayPosition);
}

/*
 * @implemented
 *
 * DdWaitForVerticalBlank
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

/*
 * @implemented
 *
 * DdCanCreateSurface
 */
DWORD
WINAPI
DdCanCreateSurface(LPDDHAL_CANCREATESURFACEDATA CanCreateSurface)
{
    /*
     * Note : This functions are basic same, in win32k
     * NtGdiDdCanCreateD3DBuffer and  NtGdiDdCanCreateSurface are mergs
     * toghter in win32k at end and retrurn same data, it is still sepreated
     * at user mode but in kmode it is not.
     */

    /* Call win32k */
    return NtGdiDdCanCreateSurface(GetDdHandle(CanCreateSurface->lpDD->hDD),
                                   (PDD_CANCREATESURFACEDATA)CanCreateSurface);
}

/*
 * @implemented
 *
 * DdCreateSurface
 */
DWORD
WINAPI
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

    /* TODO: Optimize speed. Most games/dx apps/programs do not want one surface, they want at least two.
     * So we need increase the stack to contain two surfaces instead of one. This will increase
     * the speed of the apps when allocating buffers. How to increase the surface stack space:
     * we need to create a struct for DD_SURFACE_LOCAL DdSurfaceLocal, DD_SURFACE_MORE DdSurfaceMore
     * DD_SURFACE_GLOBAL DdSurfaceGlobal, HANDLE hPrevSurface, hSurface like
     * struct { DD_SURFACE_LOCAL DdSurfaceLocal1, DD_SURFACE_LOCAL DdSurfaceLocal2 }
     * in a way that it may contain two surfaces, maybe even four. We need to watch what is most common before
     * we create the size. Activate this IF when you start doing the optimze and please also
     * take reports from users which value they got here.
     */
#if 1
    {
        char buffer[1024];
        \
        sprintf ( buffer, "Function %s : Optimze max to %d Surface ? (%s:%d)\n", __FUNCTION__, (int)SurfaceCount,__FILE__,__LINE__ );
        OutputDebugStringA(buffer);
    }
#endif

    /* Check how many surfaces there are */
    if (SurfaceCount != 1)
    {
        /* We got more than one surface, so we need to allocate memory for them */
        pDdSurfaceLocal = (PDD_SURFACE_LOCAL) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (sizeof(DD_SURFACE_LOCAL) * SurfaceCount ));
        pDdSurfaceMore = (PDD_SURFACE_MORE) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (sizeof(DD_SURFACE_MORE) * SurfaceCount ));
        pDdSurfaceGlobal = (PDD_SURFACE_GLOBAL)  HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (sizeof(DD_SURFACE_GLOBAL) * SurfaceCount ));
        phSurface = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (sizeof(HANDLE) * SurfaceCount ));
        puhSurface = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (sizeof(HANDLE) * SurfaceCount ));

        /* Check if we successfully allocated all memory we need */
        if ((pDdSurfaceLocal == NULL) || (pDdSurfaceMore == NULL) || (pDdSurfaceGlobal == NULL) || (phSurface == NULL) || (puhSurface == NULL))
        {
            pCreateSurface->ddRVal = DDERR_OUTOFMEMORY;

            if ( pDdSurfaceLocal != NULL )
            {
                HeapFree(GetProcessHeap(), 0, pDdSurfaceLocal);
            }

            if ( pDdSurfaceMore != NULL )
            {
                HeapFree(GetProcessHeap(), 0, pDdSurfaceMore);
            }

            if ( pDdSurfaceGlobal != NULL )
            {
                HeapFree(GetProcessHeap(), 0, pDdSurfaceGlobal);
            }

            if ( phSurface != NULL )
            {
                HeapFree(GetProcessHeap(), 0, phSurface);
            }

            if ( puhSurface != NULL )
            {
                HeapFree(GetProcessHeap(), 0, puhSurface);
            }

            return DDHAL_DRIVER_HANDLED;
        }
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

            /* Note if lcl->lpSurfMore is NULL zero out
             * ptmpDdSurfaceMore->ddsCapsEx.dwCaps2,
             * dwCaps3, dwCaps4, ptmpDdSurfaceMore->dwSurfaceHandle
             */
            if (lcl->lpSurfMore)
            {
                ptmpDdSurfaceMore->ddsCapsEx.dwCaps2 = lcl->lpSurfMore->ddsCapsEx.dwCaps2;
                ptmpDdSurfaceMore->ddsCapsEx.dwCaps3 = lcl->lpSurfMore->ddsCapsEx.dwCaps3;
                ptmpDdSurfaceMore->ddsCapsEx.dwCaps4 = lcl->lpSurfMore->ddsCapsEx.dwCaps4;
                ptmpDdSurfaceMore->dwSurfaceHandle = lcl->lpSurfMore->dwSurfaceHandle;
            }


            /* count to next SurfaceCount */
            ptmpDdSurfaceGlobal = (PDD_SURFACE_GLOBAL) (((PBYTE) ((ULONG_PTR) ptmpDdSurfaceGlobal)) + sizeof(DD_SURFACE_GLOBAL));
            ptmpDdSurfaceLocal = (PDD_SURFACE_LOCAL) (((PBYTE) ((ULONG_PTR) ptmpDdSurfaceLocal)) + sizeof(DD_SURFACE_LOCAL));
            ptmpDdSurfaceMore = (PDD_SURFACE_MORE) (((PBYTE) ((ULONG_PTR) ptmpDdSurfaceMore)) + sizeof(DD_SURFACE_MORE));
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

        for (i=0; i<SurfaceCount; i++)
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

            /* count to next SurfaceCount */
            ptmpDdSurfaceGlobal = (PDD_SURFACE_GLOBAL) (((PBYTE) ((ULONG_PTR) ptmpDdSurfaceGlobal)) + sizeof(DD_SURFACE_GLOBAL));
            ptmpDdSurfaceLocal = (PDD_SURFACE_LOCAL) (((PBYTE) ((ULONG_PTR) ptmpDdSurfaceLocal)) + sizeof(DD_SURFACE_LOCAL));
            ptmpDdSurfaceMore = (PDD_SURFACE_MORE) (((PBYTE) ((ULONG_PTR) ptmpDdSurfaceMore)) + sizeof(DD_SURFACE_MORE));
        }
    }

    /* Check if we have to free all our local allocations */
    if (SurfaceCount > 1)
    {
        if ( pDdSurfaceLocal != NULL )
        {
            HeapFree(GetProcessHeap(), 0, pDdSurfaceLocal);
        }

        if ( pDdSurfaceMore != NULL )
        {
            HeapFree(GetProcessHeap(), 0, pDdSurfaceMore);
        }

        if ( pDdSurfaceGlobal != NULL )
        {
            HeapFree(GetProcessHeap(), 0, pDdSurfaceGlobal);
        }

        if ( phSurface != NULL )
        {
            HeapFree(GetProcessHeap(), 0, phSurface);
        }

        if ( puhSurface != NULL )
        {
            HeapFree(GetProcessHeap(), 0, puhSurface);
        }
    }

    /* Return */
    return Return;
}

/*
 * @implemented
 *
 * DdSetColorKey
 */
DWORD
WINAPI
DdSetColorKey(LPDDHAL_SETCOLORKEYDATA pSetColorKey)
{
    /* Call win32k */
    return NtGdiDdSetColorKey((HANDLE)pSetColorKey->lpDDSurface->hDDSurface,
                              (PDD_SETCOLORKEYDATA)pSetColorKey);
}

/*
 * @implemented
 *
 * DdGetScanLine
 */
DWORD
WINAPI
DdGetScanLine(LPDDHAL_GETSCANLINEDATA pGetScanLine)
{
    /* Call win32k */
    return NtGdiDdGetScanLine(GetDdHandle(pGetScanLine->lpDD->hDD),
                              (PDD_GETSCANLINEDATA)pGetScanLine);
}


/*
 * @implemented
 *
 * DvpCreateVideoPort
 */
BOOL
WINAPI
DvpCreateVideoPort(LPDDHAL_CREATEVPORTDATA pDvdCreatePort)
{
    pDvdCreatePort->lpVideoPort->hDDVideoPort =
        NtGdiDvpCreateVideoPort(GetDdHandle(pDvdCreatePort->lpDD->lpGbl->hDD),
                                (PDD_CREATEVPORTDATA) pDvdCreatePort);

    return TRUE;
}

/*
 * @implemented
 *
 * DvpCreateVideoPort
 */
DWORD
WINAPI
DvpDestroyVideoPort(LPDDHAL_DESTROYVPORTDATA pDvdDestoryPort)
{
    return NtGdiDvpDestroyVideoPort(pDvdDestoryPort->lpVideoPort->hDDVideoPort, (PDD_DESTROYVPORTDATA)pDvdDestoryPort);
}

/*
 * @implemented
 *
 * DvpCreateVideoPort
 */
DWORD
WINAPI
DvpFlipVideoPort(LPDDHAL_FLIPVPORTDATA pDvdPortFlip)
{
    return NtGdiDvpFlipVideoPort(pDvdPortFlip->lpVideoPort->hDDVideoPort,
                                 (HANDLE)pDvdPortFlip->lpSurfCurr->hDDSurface,
                                 (HANDLE)pDvdPortFlip->lpSurfTarg->hDDSurface,
                                 (PDD_FLIPVPORTDATA) pDvdPortFlip);
}

/*
 * @implemented
 *
 * DvpGetVideoPortBandwidth
 */
DWORD
WINAPI
DvpGetVideoPortBandwidth(LPDDHAL_GETVPORTBANDWIDTHDATA pDvdPortBandWidth)
{
    return NtGdiDvpGetVideoPortBandwidth(pDvdPortBandWidth->lpVideoPort->hDDVideoPort, (PDD_GETVPORTBANDWIDTHDATA)pDvdPortBandWidth);
}

/*
 * @implemented
 *
 * DvpColorControl
 */
DWORD
WINAPI
DvpColorControl(LPDDHAL_VPORTCOLORDATA pDvdPortColorControl)
{
    return NtGdiDvpColorControl(pDvdPortColorControl->lpVideoPort->hDDVideoPort, (PDD_VPORTCOLORDATA) pDvdPortColorControl);
}

/*
 * @implemented
 *
 * DvpGetVideoSignalStatus
 */
DWORD
WINAPI
DvpGetVideoSignalStatus(LPDDHAL_GETVPORTSIGNALDATA pDvdPortVideoSignalStatus)
{
    return NtGdiDvpGetVideoSignalStatus(pDvdPortVideoSignalStatus->lpVideoPort->hDDVideoPort, (PDD_GETVPORTSIGNALDATA) pDvdPortVideoSignalStatus);
}

/*
 * @implemented
 *
 * DvpGetVideoPortFlipStatus
 */
DWORD
WINAPI
DvpGetVideoPortFlipStatus(LPDDHAL_GETVPORTFLIPSTATUSDATA pDvdPortVideoPortFlipStatus)
{
    return NtGdiDvpGetVideoPortFlipStatus(GetDdHandle(pDvdPortVideoPortFlipStatus->lpDD->lpGbl->hDD), (PDD_GETVPORTFLIPSTATUSDATA) pDvdPortVideoPortFlipStatus);

}

/*
 * @implemented
 *
 * DvpCanCreateVideoPort
 */
DWORD
WINAPI
DvpCanCreateVideoPort(LPDDHAL_CANCREATEVPORTDATA pDvdCanCreateVideoPort)
{
    return NtGdiDvpCanCreateVideoPort(GetDdHandle(pDvdCanCreateVideoPort->lpDD->lpGbl->hDD), (PDD_CANCREATEVPORTDATA) pDvdCanCreateVideoPort);
}
/*
 * @implemented
 *
 * DvpWaitForVideoPortSync
 */
DWORD
WINAPI
DvpWaitForVideoPortSync(LPDDHAL_WAITFORVPORTSYNCDATA pDvdWaitForVideoPortSync)
{
    return NtGdiDvpWaitForVideoPortSync(pDvdWaitForVideoPortSync->lpVideoPort->hDDVideoPort,  (PDD_WAITFORVPORTSYNCDATA) pDvdWaitForVideoPortSync);
}

/*
 * @implemented
 *
 * DvpUpdateVideoPort
 */
DWORD
WINAPI
DvpUpdateVideoPort(LPDDHAL_UPDATEVPORTDATA pDvdUpdateVideoPort)
{
    /*
     * Windows XP limit to max 10 handles of videoport surface and Vbi
     * ReactOS doing same to keep compatible, if it is more that 10
     * videoport surface or vbi the stack will be curpted in windows xp
     * ReactOS safe guard againts that
     *
     */

    HANDLE phSurfaceVideo[10];
    HANDLE phSurfaceVbi[10];

    if (pDvdUpdateVideoPort->dwFlags != DDRAWI_VPORTSTOP)
    {
        DWORD dwNumAutoflip;
        DWORD dwNumVBIAutoflip;

        /* Take copy of lplpDDSurface for the handle value will be modify in dxg */
        dwNumAutoflip = pDvdUpdateVideoPort->dwNumAutoflip;
        if ((dwNumAutoflip == 0) &&
                (pDvdUpdateVideoPort->lplpDDSurface == 0))
        {
            dwNumAutoflip++;
        }

        if (dwNumAutoflip != 0)
        {
            if (dwNumAutoflip>10)
            {
                dwNumAutoflip = 10;
            }
            memcpy(phSurfaceVideo,pDvdUpdateVideoPort->lplpDDSurface,dwNumAutoflip*sizeof(HANDLE));
        }

        /* Take copy of lplpDDVBISurface for the handle value will be modify in dxg */
        dwNumVBIAutoflip = pDvdUpdateVideoPort->dwNumVBIAutoflip;
        if ( (dwNumVBIAutoflip == 0) &&
                (pDvdUpdateVideoPort->lplpDDVBISurface == 0) )
        {
            dwNumVBIAutoflip++;
        }

        if (dwNumVBIAutoflip != 0)
        {
            if (dwNumVBIAutoflip>10)
            {
                dwNumVBIAutoflip = 10;
            }
            memcpy(phSurfaceVbi,pDvdUpdateVideoPort->lplpDDVBISurface,dwNumVBIAutoflip*sizeof(HANDLE));
        }
    }

    /* Call Win32k */
    return NtGdiDvpUpdateVideoPort(pDvdUpdateVideoPort->lpVideoPort->hDDVideoPort,phSurfaceVideo,phSurfaceVbi, (PDD_UPDATEVPORTDATA)pDvdUpdateVideoPort);
}

/*
 * @implemented
 *
 * DvpWaitForVideoPortSync
 */
DWORD
WINAPI
DvpGetVideoPortField(LPDDHAL_FLIPVPORTDATA pDvdGetVideoPortField)
{
    return NtGdiDvpGetVideoPortField(pDvdGetVideoPortField->lpVideoPort->hDDVideoPort, (PDD_GETVPORTFIELDDATA)pDvdGetVideoPortField);
}

/*
 * @implemented
 *
 * DvpWaitForVideoPortSync
 */
DWORD
WINAPI
DvpGetVideoPortInputFormats(LPDDHAL_GETVPORTINPUTFORMATDATA pDvdGetVideoPortInputFormat)
{
    return NtGdiDvpGetVideoPortInputFormats(pDvdGetVideoPortInputFormat->lpVideoPort->hDDVideoPort, (PDD_GETVPORTINPUTFORMATDATA) pDvdGetVideoPortInputFormat);
}

/*
 * @implemented
 *
 * DvpGetVideoPortLine
 */
DWORD
WINAPI
DvpGetVideoPortLine(LPDDHAL_GETVPORTLINEDATA pDvdGetVideoPortLine)
{
    return NtGdiDvpGetVideoPortLine(pDvdGetVideoPortLine->lpVideoPort->hDDVideoPort, (PDD_GETVPORTLINEDATA)pDvdGetVideoPortLine);
}

/*
 * @implemented
 *
 * DvpGetVideoPortOutputFormats
 */
DWORD
WINAPI
DvpGetVideoPortOutputFormats(LPDDHAL_GETVPORTLINEDATA pDvdGetVideoPortOutputFormat)
{
    return NtGdiDvpGetVideoPortLine(pDvdGetVideoPortOutputFormat->lpVideoPort->hDDVideoPort, (PDD_GETVPORTLINEDATA)pDvdGetVideoPortOutputFormat);
}

/*
 * @implemented
 *
 * DvpGetVideoPortConnectInfo
 */
DWORD
WINAPI
DvpGetVideoPortConnectInfo(LPDDHAL_GETVPORTCONNECTDATA pDvdGetVideoPortInfo)
{
    return NtGdiDvpGetVideoPortConnectInfo( GetDdHandle( pDvdGetVideoPortInfo->lpDD->lpGbl->hDD) , (PDD_GETVPORTCONNECTDATA) pDvdGetVideoPortInfo);
}

/*
 * @implemented
 *
 * DdGetAvailDriverMemory
 */
DWORD
WINAPI
DdGetAvailDriverMemory(LPDDHAL_GETAVAILDRIVERMEMORYDATA pDdGetAvailDriverMemory)
{
    return NtGdiDdGetAvailDriverMemory(GetDdHandle( pDdGetAvailDriverMemory->lpDD->hDD), (PDD_GETAVAILDRIVERMEMORYDATA) pDdGetAvailDriverMemory);
}

/*
 * @implemented
 *
 * DdAlphaBlt
 */
DWORD
WINAPI
DdAlphaBlt(LPDDHAL_BLTDATA pDdAlphaBlt)
{
    HANDLE hDDSrcSurface = 0;

    if (pDdAlphaBlt->lpDDSrcSurface != 0)
    {
        hDDSrcSurface = (HANDLE) pDdAlphaBlt->lpDDSrcSurface->hDDSurface;
    }

    return NtGdiDdAlphaBlt((HANDLE)pDdAlphaBlt->lpDDDestSurface->hDDSurface, hDDSrcSurface, (PDD_BLTDATA)&pDdAlphaBlt);
}

/*
 * @implemented
 *
 * DdCreateSurfaceEx
 */
DWORD
WINAPI
DdCreateSurfaceEx(LPDDHAL_CREATESURFACEEXDATA pDdCreateSurfaceEx)
{
    pDdCreateSurfaceEx->ddRVal = NtGdiDdCreateSurfaceEx( GetDdHandle(pDdCreateSurfaceEx->lpDDLcl->lpGbl->hDD),
                                 (HANDLE)pDdCreateSurfaceEx->lpDDSLcl->hDDSurface,
                                 pDdCreateSurfaceEx->lpDDSLcl->lpSurfMore->dwSurfaceHandle);
    return TRUE;
}

/*
 * @implemented
 *
 * DdColorControl
 */
DWORD
WINAPI
DdColorControl(LPDDHAL_COLORCONTROLDATA pDdColorControl)
{
    return NtGdiDdColorControl( (HANDLE) pDdColorControl->lpDDSurface->hDDSurface, (PDD_COLORCONTROLDATA) &pDdColorControl);
}

/*
 * @implemented
 *
 * DdSetExclusiveMode
 */
DWORD
WINAPI
DdSetExclusiveMode(LPDDHAL_SETEXCLUSIVEMODEDATA pDdSetExclusiveMode)
{
    return NtGdiDdSetExclusiveMode( GetDdHandle(pDdSetExclusiveMode->lpDD->hDD), (PDD_SETEXCLUSIVEMODEDATA) &pDdSetExclusiveMode);
}

/*
 * @implemented
 *
 * DdFlipToGDISurface
 */
DWORD
WINAPI
DdFlipToGDISurface(LPDDHAL_FLIPTOGDISURFACEDATA pDdFlipToGDISurface)
{
    return NtGdiDdFlipToGDISurface( GetDdHandle(pDdFlipToGDISurface->lpDD->hDD), (PDD_FLIPTOGDISURFACEDATA) &pDdFlipToGDISurface);
}

/* TODO */
DWORD
WINAPI
DdGetDriverInfo(LPDDHAL_GETDRIVERINFODATA pData)
{
    DDHAL_GETDRIVERINFODATA pDrvInfoData;
    DWORD retValue = DDHAL_DRIVER_NOTHANDLED;
    HANDLE hDD;

    /* FIXME add SEH around this functions */

    RtlZeroMemory(&pDrvInfoData, sizeof (DDHAL_GETDRIVERINFODATA));
    RtlCopyMemory(&pDrvInfoData.guidInfo, &pData->guidInfo, sizeof(GUID));

    hDD = GetDdHandle(pData->dwContext);

    pDrvInfoData.dwSize = sizeof (DDHAL_GETDRIVERINFODATA);
    pDrvInfoData.ddRVal = DDERR_GENERIC;
    pDrvInfoData.dwContext = (ULONG_PTR)hDD;


    /* Videoport Callbacks check and setup for DirectX/ ReactX */
    if (IsEqualGUID(&pData->guidInfo, &GUID_VideoPortCallbacks))
    {
        DDHAL_DDVIDEOPORTCALLBACKS  pDvdPort;
        DDHAL_DDVIDEOPORTCALLBACKS* pUserDvdPort = (DDHAL_DDVIDEOPORTCALLBACKS *)pData->lpvData;

        /* Clear internal out buffer and set it up*/
        RtlZeroMemory(&pDvdPort, DDVIDEOPORTCALLBACKSSIZE);
        pDvdPort.dwSize = DDVIDEOPORTCALLBACKSSIZE;

        /* set up internal buffer */
        pDrvInfoData.lpvData = (PVOID)&pDvdPort;
        pDrvInfoData.dwExpectedSize = DDVIDEOPORTCALLBACKSSIZE ;

        /* Call win32k */
        retValue = NtGdiDdGetDriverInfo(hDD, (PDD_GETDRIVERINFODATA)&pDrvInfoData);

        /* Setup user out buffer and convert kmode callbacks to user mode */
        pUserDvdPort->dwSize = DDVIDEOPORTCALLBACKSSIZE;
        pUserDvdPort->dwFlags = pDrvInfoData.dwFlags =  0;

        pUserDvdPort->dwFlags = (pDrvInfoData.dwFlags & ~(DDHAL_VPORT32_CREATEVIDEOPORT | DDHAL_VPORT32_FLIP |
                                 DDHAL_VPORT32_DESTROY | DDHAL_VPORT32_UPDATE | DDHAL_VPORT32_WAITFORSYNC)) |
                                (DDHAL_VPORT32_CREATEVIDEOPORT | DDHAL_VPORT32_FLIP |
                                 DDHAL_VPORT32_DESTROY | DDHAL_VPORT32_UPDATE);

        pData->dwActualSize = DDVIDEOPORTCALLBACKSSIZE;
        pUserDvdPort->CreateVideoPort = (LPDDHALVPORTCB_CREATEVIDEOPORT) DvpCreateVideoPort;
        pUserDvdPort->FlipVideoPort = (LPDDHALVPORTCB_FLIP) DvpFlipVideoPort;
        pUserDvdPort->DestroyVideoPort = (LPDDHALVPORTCB_DESTROYVPORT) DvpDestroyVideoPort;
        pUserDvdPort->UpdateVideoPort = (LPDDHALVPORTCB_UPDATE) DvpUpdateVideoPort;

        if (pDvdPort.CanCreateVideoPort)
        {
            pUserDvdPort->CanCreateVideoPort = (LPDDHALVPORTCB_CANCREATEVIDEOPORT) DvpCanCreateVideoPort;
        }

        if (pDvdPort.GetVideoPortBandwidth)
        {
            pUserDvdPort->GetVideoPortBandwidth = (LPDDHALVPORTCB_GETBANDWIDTH) DvpGetVideoPortBandwidth;
        }

        if (pDvdPort.GetVideoPortInputFormats)
        {
            pUserDvdPort->GetVideoPortInputFormats = (LPDDHALVPORTCB_GETINPUTFORMATS) DvpGetVideoPortInputFormats;
        }

        if (pDvdPort.GetVideoPortOutputFormats)
        {
            pUserDvdPort->GetVideoPortOutputFormats = (LPDDHALVPORTCB_GETOUTPUTFORMATS) DvpGetVideoPortOutputFormats;
        }

        if (pDvdPort.GetVideoPortField)
        {
            pUserDvdPort->GetVideoPortField = (LPDDHALVPORTCB_GETFIELD) DvpGetVideoPortField;
        }

        if (pDvdPort.GetVideoPortLine)
        {
            pUserDvdPort->GetVideoPortLine = (LPDDHALVPORTCB_GETLINE) DvpGetVideoPortLine;
        }

        if (pDvdPort.GetVideoPortConnectInfo)
        {
            pUserDvdPort->GetVideoPortConnectInfo = (LPDDHALVPORTCB_GETVPORTCONNECT) DvpGetVideoPortConnectInfo;
        }

        if (pDvdPort.GetVideoPortFlipStatus)
        {
            pUserDvdPort->GetVideoPortFlipStatus = (LPDDHALVPORTCB_GETFLIPSTATUS) DvpGetVideoPortFlipStatus;
        }

        if (pDvdPort.WaitForVideoPortSync)
        {
            pUserDvdPort->WaitForVideoPortSync = (LPDDHALVPORTCB_WAITFORSYNC) DvpWaitForVideoPortSync;
        }

        if (pDvdPort.GetVideoSignalStatus)
        {
            pUserDvdPort->GetVideoSignalStatus = (LPDDHALVPORTCB_GETSIGNALSTATUS) DvpGetVideoSignalStatus;
        }

        if (pDvdPort.ColorControl)
        {
            pUserDvdPort->ColorControl = (LPDDHALVPORTCB_COLORCONTROL) DvpColorControl;
        }

        /* Windows XP never repot back the true return value,
         *  it only report back if we have a driver or not
         *  ReactOS keep this behoir to be compatible with
         *  Windows XP
         */
        pData->ddRVal = retValue;
    }

    /* Color Control Callbacks check and setup for DirectX/ ReactX */
    if (IsEqualGUID(&pData->guidInfo, &GUID_ColorControlCallbacks))
    {
        DDHAL_DDCOLORCONTROLCALLBACKS  pColorControl;
        DDHAL_DDCOLORCONTROLCALLBACKS* pUserColorControl = (DDHAL_DDCOLORCONTROLCALLBACKS *)pData->lpvData;

        /* Clear internal out buffer and set it up*/
        RtlZeroMemory(&pColorControl, DDCOLORCONTROLCALLBACKSSIZE);
        pColorControl.dwSize = DDCOLORCONTROLCALLBACKSSIZE;

        /* set up internal buffer */
        pDrvInfoData.lpvData = (PVOID)&pColorControl;
        pDrvInfoData.dwExpectedSize = DDCOLORCONTROLCALLBACKSSIZE ;

        /* Call win32k */
        retValue = NtGdiDdGetDriverInfo(hDD, (PDD_GETDRIVERINFODATA)&pDrvInfoData);

        pData->dwActualSize = DDCOLORCONTROLCALLBACKSSIZE;
        pData->dwFlags = pDrvInfoData.dwFlags;

        pUserColorControl->dwSize = DDCOLORCONTROLCALLBACKSSIZE;
        pUserColorControl->dwFlags = pColorControl.dwFlags;

        if (pColorControl.ColorControl != NULL)
        {
            pUserColorControl->ColorControl = (LPDDHALCOLORCB_COLORCONTROL) DdColorControl;
        }

        /* Windows XP never repot back the true return value,
         *  it only report back if we have a driver or not
         *  ReactOS keep this behoir to be compatible with
         *  Windows XP
         */
        pData->ddRVal = retValue;
    }

    /* Misc Callbacks check and setup for DirectX/ ReactX */
    else if (IsEqualGUID(&pData->guidInfo, &GUID_MiscellaneousCallbacks))
    {
        DDHAL_DDMISCELLANEOUSCALLBACKS  pMisc;
        DDHAL_DDMISCELLANEOUSCALLBACKS* pUserMisc = (DDHAL_DDMISCELLANEOUSCALLBACKS *)pData->lpvData;

        /* Clear internal out buffer and set it up*/
        RtlZeroMemory(&pMisc, DDMISCELLANEOUSCALLBACKSSIZE);
        pMisc.dwSize = DDMISCELLANEOUSCALLBACKSSIZE;

        /* set up internal buffer */
        pDrvInfoData.lpvData = (PVOID)&pMisc;
        pDrvInfoData.dwExpectedSize = DDMISCELLANEOUSCALLBACKSSIZE ;

        /* Call win32k */
        retValue = NtGdiDdGetDriverInfo(hDD, (PDD_GETDRIVERINFODATA)&pDrvInfoData);

        pData->dwActualSize = DDMISCELLANEOUSCALLBACKSSIZE;

        /* Only one callbacks are supported */
        pUserMisc->dwFlags = pMisc.dwFlags & DDHAL_MISCCB32_GETAVAILDRIVERMEMORY;
        pUserMisc->GetAvailDriverMemory = (LPDDHAL_GETAVAILDRIVERMEMORY) DdGetAvailDriverMemory;

        /* This callbacks are only for win9x and theirfor it is not longer use in NT or ReactOS
         * pUserMisc->UpdateNonLocalHeap;
         * pUserMisc->GetHeapAlignment;
         * pUserMisc->GetSysmemBltStatus; */

        /* Windows XP never repot back the true return value,
         *  it only report back if we have a driver or not
         *  ReactOS keep this behoir to be compatible with
         *  Windows XP
         */
        pData->ddRVal = retValue;
    }

    /* Misc 2 Callbacks check and setup for DirectX/ ReactX */
    else if (IsEqualGUID(&pData->guidInfo, &GUID_Miscellaneous2Callbacks))
    {
        DDHAL_DDMISCELLANEOUS2CALLBACKS  pMisc;
        DDHAL_DDMISCELLANEOUS2CALLBACKS* pUserMisc = (DDHAL_DDMISCELLANEOUS2CALLBACKS *)pData->lpvData;

        /* Clear internal out buffer and set it up*/
        RtlZeroMemory(&pMisc, DDMISCELLANEOUS2CALLBACKSSIZE);
        pMisc.dwSize = DDMISCELLANEOUS2CALLBACKSSIZE;

        /* set up internal buffer */
        pDrvInfoData.lpvData = (PVOID)&pMisc;
        pDrvInfoData.dwExpectedSize = DDMISCELLANEOUS2CALLBACKSSIZE ;

        /* Call win32k */
        retValue = NtGdiDdGetDriverInfo(hDD, (PDD_GETDRIVERINFODATA)&pDrvInfoData);

        pData->dwActualSize = DDMISCELLANEOUS2CALLBACKSSIZE;

        pUserMisc->dwFlags = pMisc.dwFlags;

        /* This functions are not documneted in MSDN for this struct, here is directx/reactx alpha blend */
        if ( pMisc.Reserved )
        {
            pUserMisc->Reserved = (LPVOID) DdAlphaBlt;
        }

        if ( pMisc.CreateSurfaceEx )
        {
            pUserMisc->CreateSurfaceEx = (LPDDHAL_CREATESURFACEEX) DdCreateSurfaceEx;
        }

        if ( pMisc.GetDriverState )
        {
            pUserMisc->GetDriverState = (LPDDHAL_GETDRIVERSTATE) NtGdiDdGetDriverState;
        }

        /* NOTE : pUserMisc->DestroyDDLocal is outdated and are not beign tuch */

        /* Windows XP never repot back the true return value,
         *  it only report back if we have a driver or not
         *  ReactOS keep this behoir to be compatible with
         *  Windows XP
         */
        pData->ddRVal = retValue;
    }

    /* NT Callbacks check and setup for DirectX/ ReactX */
    else if (IsEqualGUID(&pData->guidInfo, &GUID_NTCallbacks))
    {
        /* MS does not have DHAL_* version of this callbacks
         * so we are force using PDD_* callbacks here
         */
        DD_NTCALLBACKS  pNtKernel;
        PDD_NTCALLBACKS pUserNtKernel = (PDD_NTCALLBACKS)pData->lpvData;

        /* Clear internal out buffer and set it up*/
        RtlZeroMemory(&pNtKernel, sizeof(DD_NTCALLBACKS));
        pNtKernel.dwSize = sizeof(DD_NTCALLBACKS);

        /* set up internal buffer */
        pDrvInfoData.lpvData = (PVOID)&pNtKernel;
        pDrvInfoData.dwExpectedSize = sizeof(DD_NTCALLBACKS) ;

        /* Call win32k */
        retValue = NtGdiDdGetDriverInfo(hDD, (PDD_GETDRIVERINFODATA)&pDrvInfoData);

        pData->dwActualSize = sizeof(DD_NTCALLBACKS);

        pUserNtKernel->dwSize = sizeof(DD_NTCALLBACKS);
        pUserNtKernel->dwFlags = pNtKernel.dwFlags;
        pUserNtKernel->FreeDriverMemory = 0;

        if (pNtKernel.SetExclusiveMode)
        {
            pUserNtKernel->SetExclusiveMode = (PDD_SETEXCLUSIVEMODE) DdSetExclusiveMode;
        }

        if (pNtKernel.FlipToGDISurface)
        {
            pUserNtKernel->FlipToGDISurface = (PDD_FLIPTOGDISURFACE) DdFlipToGDISurface;
        }

        /* Windows XP never repot back the true return value,
         *  it only report back if we have a driver or not
         *  ReactOS keep this behoir to be compatible with
         *  Windows XP
         */
        pData->ddRVal = retValue;
    }

    /* D3D Callbacks version 2 check and setup for DirectX/ ReactX */
    else if (IsEqualGUID(&pData->guidInfo, &GUID_D3DCallbacks2))
    {
        // FIXME GUID_D3DCallbacks2
    }

    /* D3D Callbacks version 3 check and setup for DirectX/ ReactX */
    else if (IsEqualGUID(&pData->guidInfo, &GUID_D3DCallbacks3))
    {
        // FIXME GUID_D3DCallbacks3
    }

    /* D3DParseUnknownCommand Callbacks check and setup for DirectX/ ReactX */
    else if (IsEqualGUID(&pData->guidInfo, &GUID_D3DParseUnknownCommandCallback))
    {
        // FIXME GUID_D3DParseUnknownCommandCallback
    }

    /* MotionComp Callbacks check and setup for DirectX/ ReactX */
    else if (IsEqualGUID(&pData->guidInfo, &GUID_MotionCompCallbacks))
    {
        // FIXME GUID_MotionCompCallbacks
    }

    /* FIXME VPE2 Callbacks check and setup for DirectX/ ReactX */
    //else if (IsEqualGUID(&pData->guidInfo, &GUID_VPE2Callbacks))
    //{
    // FIXME GUID_VPE2Callbacks
    //}
    else
    {
        /* set up internal buffer */
        pDrvInfoData.dwExpectedSize = pData->dwExpectedSize;
        pDrvInfoData.lpvData = pData->lpvData;

        /* We do not cover all callbacks for user mode, they are only cover by kmode */
        retValue = NtGdiDdGetDriverInfo(hDD, (PDD_GETDRIVERINFODATA)&pDrvInfoData);

        /* Setup return data */
        pData->dwActualSize = pDrvInfoData.dwActualSize;
        pData->lpvData = pDrvInfoData.lpvData;
        /* Windows XP never repot back the true return value,
         *  it only report back if we have a driver or not
         *  ReactOS keep this behoir to be compatible with
         *  Windows XP
         */
        pData->ddRVal = retValue;
    }

    return retValue;
}


/*
 * @implemented
 *
 * D3dContextCreate
 */
BOOL
WINAPI
D3dContextCreate(LPD3DHAL_CONTEXTCREATEDATA pdcci)
{
    HANDLE hSurfZ = NULL;

    if (pdcci->lpDDSZLcl)
    {
        hSurfZ = (HANDLE)pdcci->lpDDSZLcl->hDDSurface;
    }

    return  NtGdiD3dContextCreate(GetDdHandle(pdcci->lpDDLcl->hDD),
                                  (HANDLE)pdcci->lpDDSLcl->hDDSurface,
                                  hSurfZ,
                                  (D3DNTHAL_CONTEXTCREATEI *)pdcci);
}

/*
 * @implemented
 *
 * DdCanCreateD3DBuffer
 */
DWORD
WINAPI
DdCanCreateD3DBuffer(LPDDHAL_CANCREATESURFACEDATA CanCreateD3DBuffer)
{
    /*
     * Note : This functions are basic same, in win32k
     * NtGdiDdCanCreateD3DBuffer and  NtGdiDdCanCreateSurface are mergs
     * toghter in win32k at end and retrurn same data, it is still sepreated
     * at user mode but in kmode it is not.
     */

    /* Call win32k */
    return NtGdiDdCanCreateD3DBuffer(GetDdHandle(CanCreateD3DBuffer->lpDD->hDD),
                                     (PDD_CANCREATESURFACEDATA)CanCreateD3DBuffer);
}


/*
 * @implemented
 *
 * DdCreateD3DBuffer
 */
DWORD
WINAPI
DdCreateD3DBuffer(LPDDHAL_CREATESURFACEDATA pCreateSurface)
{
    HANDLE puhSurface = 0;
    DDRAWI_DDRAWSURFACE_GBL *pSurfGBL;
    DDRAWI_DDRAWSURFACE_LCL *pSurfLcl;
    DD_SURFACE_GLOBAL puSurfaceGlobalData;
    DD_SURFACE_MORE puSurfaceMoreData;
    DD_SURFACE_LOCAL puSurfaceLocalData;
    DWORD retValue;

    /* Zero all local memory pointer */
    RtlZeroMemory(&puSurfaceGlobalData, sizeof(DD_SURFACE_GLOBAL) );
    RtlZeroMemory(&puSurfaceMoreData, sizeof(DD_SURFACE_MORE) ) ;
    RtlZeroMemory(&puSurfaceLocalData, sizeof(DD_SURFACE_LOCAL) );

    pCreateSurface->dwSCnt = 1;
    pSurfLcl = pCreateSurface->lplpSList[0];
    pSurfGBL = pSurfLcl->lpGbl;

    /* Convert DDRAWI_DDRAWSURFACE_GBL to DD_SURFACE_GLOBAL */
    puSurfaceGlobalData.wWidth = pSurfGBL->wWidth;
    puSurfaceGlobalData.wHeight = pSurfGBL->wHeight;
    puSurfaceGlobalData.dwLinearSize = pSurfGBL->dwLinearSize;
    puSurfaceGlobalData.fpVidMem = pSurfGBL->fpVidMem;
    puSurfaceGlobalData.dwBlockSizeX = pSurfGBL->dwBlockSizeX;
    puSurfaceGlobalData.dwBlockSizeY = pSurfGBL->dwBlockSizeY;

    /* Convert DDRAWI_DDRAWSURFACE_MORE to DD_SURFACE_MORE */
    puSurfaceMoreData.dwSurfaceHandle = pSurfLcl->lpSurfMore->dwSurfaceHandle;
    puSurfaceMoreData.ddsCapsEx.dwCaps2 = pSurfLcl->lpSurfMore->ddsCapsEx.dwCaps2;
    puSurfaceMoreData.ddsCapsEx.dwCaps3 = pSurfLcl->lpSurfMore->ddsCapsEx.dwCaps3;
    puSurfaceMoreData.ddsCapsEx.dwCaps4 = pSurfLcl->lpSurfMore->ddsCapsEx.dwCaps4;

    /* Convert DDRAWI_DDRAWSURFACE_LCL to DD_SURFACE_LOCAL */
    puSurfaceLocalData.dwFlags = pSurfLcl->dwFlags;
    puSurfaceLocalData.ddsCaps.dwCaps = pSurfLcl->ddsCaps.dwCaps;

    /* Call win32k */
    retValue = NtGdiDdCreateD3DBuffer( GetDdHandle(pCreateSurface->lpDD->hDD),
                                       (HANDLE*)&pSurfLcl->hDDSurface,
                                       pCreateSurface->lpDDSurfaceDesc,
                                       &puSurfaceGlobalData,
                                       &puSurfaceLocalData,
                                       &puSurfaceMoreData,
                                       (DD_CREATESURFACEDATA *) pCreateSurface,
                                       &puhSurface);

    /* Setup surface handle if we got one back  */
    if ( puhSurface != NULL )
    {
        pCreateSurface->lplpSList[0]->hDDSurface = (ULONG_PTR)puhSurface;
    }

    /* Convert DD_SURFACE_GLOBAL to DDRAWI_DDRAWSURFACE_GBL */
    pSurfGBL->dwLinearSize = puSurfaceGlobalData.dwLinearSize;
    pSurfGBL->fpVidMem = puSurfaceGlobalData.fpVidMem;
    pSurfGBL->dwBlockSizeX = puSurfaceGlobalData.dwBlockSizeX;
    pSurfGBL->dwBlockSizeY = puSurfaceGlobalData.dwBlockSizeY;

    return retValue;
}

/*
 * @implemented
 *
 * DdDestroyD3DBuffer
 */
DWORD
WINAPI
DdDestroyD3DBuffer(LPDDHAL_DESTROYSURFACEDATA pDestroySurface)
{
    DWORD retValue = 0;
    if ( pDestroySurface->lpDDSurface->hDDSurface)
    {
        /* Call win32k */
        retValue = NtGdiDdDestroyD3DBuffer((HANDLE)pDestroySurface->lpDDSurface->hDDSurface);
    }

    return retValue;
}

/*
 * @implemented
 *
 * DdLockD3D
 */
DWORD
WINAPI
DdLockD3D(LPDDHAL_LOCKDATA Lock)
{

    /* Call win32k */
    return NtGdiDdLockD3D((HANDLE)Lock->lpDDSurface->hDDSurface, (PDD_LOCKDATA)Lock);
}

/*
 * @implemented
 *
 * DdUnlockD3D
 */
DWORD
WINAPI
DdUnlockD3D(LPDDHAL_UNLOCKDATA Unlock)
{
    /* Call win32k */
    return NtGdiDdUnlock((HANDLE)Unlock->lpDDSurface->hDDSurface,
                         (PDD_UNLOCKDATA)Unlock);
}


/* PRIVATE FUNCTIONS *********************************************************/

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
    SurfaceMore.dwSurfaceHandle = pSurface->lpSurfMore->dwSurfaceHandle;

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
    pSurface->hDDSurface = (DWORD_PTR)NtGdiDdCreateSurfaceObject(GetDdHandle(pSurface->lpGbl->lpDD->hDD),
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
            if ((hdc = CreateDCW(L"Display", NULL, NULL, NULL)))
            {
                /* Create the DDraw Object */
                ghDirectDraw = NtGdiDdCreateDirectDrawObject(hdc);

                /* Delete our DC */
                DeleteDC(hdc);
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
    BOOL retVal = TRUE;

    /* Clear the structures */
    RtlZeroMemory(&HalInfo, sizeof(DD_HALINFO));
    RtlZeroMemory(&D3dCallbacks, sizeof(D3DNTHAL_CALLBACKS));
    RtlZeroMemory(&D3dDriverData, sizeof(D3DNTHAL_GLOBALDRIVERDATA));
    RtlZeroMemory(&D3dBufferCallbacks, sizeof(DD_D3DBUFCALLBACKS));
    RtlZeroMemory(CallbackFlags, sizeof(DWORD)*3);

    /* Note : XP always alloc 24*sizeof(VIDEOMEMORY) of pvmlist so we change it to it */
    if ( (pvmList != NULL) &&
            (pHalInfo->vmiData.dwNumHeaps != 0) )
    {
        VidMemList = (PVIDEOMEMORY) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (sizeof(VIDEOMEMORY) * 24 ) * pHalInfo->vmiData.dwNumHeaps);
    }


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
        retVal = FALSE;
        goto cleanup;
    }

    /* Clear the incoming pointer */
    RtlZeroMemory(pHalInfo, sizeof(DDHALINFO));

    /* Convert all the data */
    pHalInfo->dwSize = sizeof(DDHALINFO);
    pHalInfo->lpDDCallbacks = pDDCallbacks;
    pHalInfo->lpDDSurfaceCallbacks = pDDSurfaceCallbacks;
    pHalInfo->lpDDPaletteCallbacks = pDDPaletteCallbacks;

    /* Check for NT5+ D3D Data */
    if ( (D3dCallbacks.dwSize != 0) &&
            (D3dDriverData.dwSize != 0) )
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

    pHalInfo->vmiData.dwNumHeaps = dwNumHeaps;
    pHalInfo->vmiData.pvmList = pvmList;

    RtlCopyMemory( &pHalInfo->ddCaps,
                   &HalInfo.ddCaps,
                   sizeof(DDCORECAPS ));

    pHalInfo->ddCaps.dwNumFourCCCodes = FourCCs;
    pHalInfo->lpdwFourCC = pdwFourCC;

    /* always force rope 0x1000 for hal it mean only source copy is supported */
    pHalInfo->ddCaps.dwRops[6] = 0x1000;

    /* Set the HAL flags what ReactX got from the driver
     * Windows XP force setting DDHALINFO_GETDRIVERINFOSET if the driver does not set it
     * and ReactX doing same to keep compatible with drivers, but the driver are
     * force support DdGetDriverInfo acoriding MSDN but it seam some driver do not set
     * this flag even it is being supported. that is mean. It is small hack to keep
     * bad driver working, that trust this is always being setting by it self at end
     */
    pHalInfo->dwFlags = (HalInfo.dwFlags & ~DDHALINFO_GETDRIVERINFOSET) | DDHALINFO_GETDRIVERINFOSET;
    pHalInfo->GetDriverInfo = (LPDDHAL_GETDRIVERINFO) DdGetDriverInfo;

    /* Now check if we got any DD callbacks */
    if (pDDCallbacks)
    {
        /* Zero the structure */
        RtlZeroMemory(pDDCallbacks, sizeof(DDHAL_DDCALLBACKS));
        pDDCallbacks->dwSize = sizeof(DDHAL_DDCALLBACKS);

        /* Set the flags for this structure
         * Windows XP force setting DDHAL_CB32_CREATESURFACE if the driver does not set it
         * and ReactX doing same to keep compatible with drivers, but the driver are
         * force support pDDCallbacks acoriding MSDN but it seam some driver do not set
         * this flag even it is being supported. that is mean. It is small hack to keep
         * bad driver working, that trust this is always being setting by it self at end
        */
        Flags = (CallbackFlags[0] & ~DDHAL_CB32_CREATESURFACE) | DDHAL_CB32_CREATESURFACE;
        pDDCallbacks->dwFlags = Flags;

        /* Write the always-on functions */
        pDDCallbacks->CreateSurface = DdCreateSurface;

        /* Now write the pointers, if applicable */
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
        pDDSurfaceCallbacks->dwSize  = sizeof(DDHAL_DDSURFACECALLBACKS);

        /* Set the flags for this structure
         * Windows XP force setting DDHAL_SURFCB32_LOCK, DDHAL_SURFCB32_UNLOCK,
         * DDHAL_SURFCB32_SETCOLORKEY, DDHAL_SURFCB32_DESTROYSURFACE if the driver
         * does not set it and ReactX doing same to keep compatible with drivers,
         * but the driver are force support pDDSurfaceCallbacks acoriding MSDN but it seam
         * some driver do not set this flag even it is being supported. that is mean.
         * It is small hack to keep bad driver working, that trust this is always being
         * setting by it self at end
         */

        Flags = (CallbackFlags[1] & ~(DDHAL_SURFCB32_LOCK | DDHAL_SURFCB32_UNLOCK |
                                      DDHAL_SURFCB32_SETCOLORKEY | DDHAL_SURFCB32_DESTROYSURFACE)) |
                (DDHAL_SURFCB32_LOCK | DDHAL_SURFCB32_UNLOCK |
                 DDHAL_SURFCB32_SETCOLORKEY | DDHAL_SURFCB32_DESTROYSURFACE);

        pDDSurfaceCallbacks->dwFlags = Flags;

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

    /* Check for DD Palette Callbacks, This interface are dead for user mode,
     * only what it can support are being report back.
     */
    if (pDDPaletteCallbacks)
    {
        /* Zero the struct */
        RtlZeroMemory(pDDPaletteCallbacks, sizeof(DDHAL_DDPALETTECALLBACKS));

        /* Write the header */
        pDDPaletteCallbacks->dwSize  = sizeof(DDHAL_DDPALETTECALLBACKS);
        pDDPaletteCallbacks->dwFlags = CallbackFlags[2];
    }

    if (pD3dCallbacks)
    {
        /* Zero the struct */
        RtlZeroMemory(pD3dCallbacks, sizeof(DDHAL_DDEXEBUFCALLBACKS));

        /* Check if we have one */
        if (D3dCallbacks.dwSize)
        {
            /* Write the header */
            pD3dCallbacks->dwSize = sizeof(DDHAL_DDEXEBUFCALLBACKS);

            /* Now check for each callback */
            if (D3dCallbacks.ContextCreate)
            {
                pD3dCallbacks->ContextCreate = (LPD3DHAL_CONTEXTCREATECB) D3dContextCreate;
            }
            if (D3dCallbacks.ContextDestroy)
            {
                pD3dCallbacks->ContextDestroy = (LPD3DHAL_CONTEXTDESTROYCB) NtGdiD3dContextDestroy;
            }
            if (D3dCallbacks.ContextDestroyAll)
            {
                pD3dCallbacks->ContextDestroyAll = (LPD3DHAL_CONTEXTDESTROYALLCB) NtGdiD3dContextDestroyAll;
            }
        }
    }

    /* Check for D3D Driver Data */
    if (pD3dDriverData)
    {
        /* Copy the struct */
        RtlMoveMemory(pD3dDriverData, &D3dDriverData, sizeof(D3DHAL_GLOBALDRIVERDATA));

        /* Write the pointer to the texture formats */
        pD3dDriverData->lpTextureFormats = pD3dTextureFormats;
    }

    /* Check for D3D Buffer Callbacks */
    if (pD3dBufferCallbacks)
    {
        /* Zero the struct */
        RtlZeroMemory(pD3dBufferCallbacks, sizeof(DDHAL_DDEXEBUFCALLBACKS));

        if ( D3dBufferCallbacks.dwSize)
        {
            pD3dBufferCallbacks->dwSize = D3dBufferCallbacks.dwSize;

            pD3dBufferCallbacks->dwFlags = D3dBufferCallbacks.dwFlags;
            if ( D3dBufferCallbacks.CanCreateD3DBuffer)
            {
                pD3dBufferCallbacks->CanCreateExecuteBuffer = (LPDDHALEXEBUFCB_CANCREATEEXEBUF)DdCanCreateD3DBuffer;
            }

            if (D3dBufferCallbacks.CreateD3DBuffer)
            {
                pD3dBufferCallbacks->CreateExecuteBuffer = (LPDDHALEXEBUFCB_CREATEEXEBUF) DdCreateD3DBuffer;
            }

            if ( D3dBufferCallbacks.DestroyD3DBuffer )
            {
                pD3dBufferCallbacks->DestroyExecuteBuffer = (LPDDHALEXEBUFCB_DESTROYEXEBUF) DdDestroyD3DBuffer;
            }

            if ( D3dBufferCallbacks.LockD3DBuffer )
            {
                pD3dBufferCallbacks->LockExecuteBuffer = (LPDDHALEXEBUFCB_LOCKEXEBUF) DdLockD3D;
            }

            if ( D3dBufferCallbacks.UnlockD3DBuffer )
            {
                pD3dBufferCallbacks->UnlockExecuteBuffer = (LPDDHALEXEBUFCB_UNLOCKEXEBUF) DdUnlockD3D;
            }

        }
    }

    /* FIXME VidMemList */

cleanup:
    if (VidMemList)
    {
        HeapFree(GetProcessHeap(), 0, VidMemList);
    }

    return retVal;
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
        if (Return)
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
            if (Return)
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
    return NtGdiDdGetDC((HANDLE)pSurfaceLocal->hDDSurface, pColorTable);
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
WINAPI
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
WINAPI
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
WINAPI
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
WINAPI
DdQueryDisplaySettingsUniqueness(VOID)
{
    return GdiSharedHandleTable->flDeviceUniq;
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
    HANDLE hSurface = NULL;

    /* Check if we already have a surface */
    if (!pSurface)
    {
        /* We don't have one, use the DirectDraw Object handle instead */
        hDD = GetDdHandle(pDDraw->lpGbl->hDD);
    }
    else
    {
        hSurface = (HANDLE)pSurface->hDDSurface;
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




