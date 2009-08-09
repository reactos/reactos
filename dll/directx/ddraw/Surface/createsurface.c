/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 ddraw/surface/createsurface.c
 * PURPOSE:              IDirectDrawSurface Creation
 * PROGRAMMER:           Magnus Olsen
 *
 */
#include "rosdraw.h"

/*
* All parameters must have been checked if they are valid before they are passed to Internal_CreateSurface.
* If not please fix the code in the functions which call Internal_CreateSurface.
* ppSurf,pDDSD,pDDraw are being validated in Internal_CreateSurface.
 */

HRESULT
Internal_CreateSurface( LPDDRAWI_DIRECTDRAW_INT pDDraw, LPDDSURFACEDESC2 pDDSD,
                        LPDDRAWI_DDRAWSURFACE_INT *ppSurf, IUnknown *pUnkOuter)
{
    DDHAL_CANCREATESURFACEDATA mDdCanCreateSurface = { 0 };
    DDHAL_CREATESURFACEDATA mDdCreateSurface = { 0 };

    LPDDRAWI_DDRAWSURFACE_INT ThisSurfInt;
    LPDDRAWI_DDRAWSURFACE_LCL ThisSurfLcl;
    LPDDRAWI_DDRAWSURFACE_GBL ThisSurfGbl;
    LPDDRAWI_DDRAWSURFACE_MORE ThisSurfMore;

    LPDDRAWI_DDRAWSURFACE_INT * slist_int = NULL;
    LPDDRAWI_DDRAWSURFACE_LCL * slist_lcl = NULL;
    LPDDRAWI_DDRAWSURFACE_GBL * slist_gbl = NULL;
    LPDDRAWI_DDRAWSURFACE_MORE * slist_more = NULL;
    DWORD num_of_surf=1;
    DWORD count;
    HRESULT ret;

    if(pDDraw->lpLcl->dwLocalFlags != DDRAWILCL_SETCOOPCALLED)
    {
        return DDERR_NOCOOPERATIVELEVELSET;
    }

    if(pUnkOuter)
    {
        return CLASS_E_NOAGGREGATION;
    }

    if(!pDDSD->dwFlags & DDSD_CAPS)
    {
        return DDERR_INVALIDPARAMS;
    }
    if (pDDraw->lpLcl->dwProcessId != GetCurrentProcessId() )
    {
        return DDERR_INVALIDOBJECT;
    }

    if  ( ((pDDSD->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) == DDSCAPS_SYSTEMMEMORY) &&
          ((pDDSD->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) == DDSCAPS_VIDEOMEMORY) )
    {
        return DDERR_INVALIDCAPS;
    }

    if(!(pDDSD->dwFlags & DDSD_HEIGHT) && !(pDDSD->dwFlags & DDSD_HEIGHT)
        && !(pDDSD->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
    {
        return DDERR_INVALIDPARAMS;
    }

    else if(pDDSD->dwFlags & DDSD_HEIGHT && pDDSD->dwFlags & DDSD_HEIGHT
        && pDDSD->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {
        return DDERR_INVALIDPARAMS;
    }

    /*
     * program does not need set the DDSD_LPSURFACE,
     * if they forget set it, the ddraw will autoamtic
     * set it for system memory.
     */
    if ( ((pDDSD->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) == DDSCAPS_SYSTEMMEMORY) &&
         ((pDDSD->dwFlags & DDSD_LPSURFACE) != DDSD_LPSURFACE) )
    {
        pDDSD->dwFlags = pDDSD->dwFlags | DDSD_LPSURFACE;
    }

    /* FIXME count how many surfaces we need */

    DxHeapMemAlloc(slist_int, num_of_surf * sizeof( LPDDRAWI_DDRAWSURFACE_INT ) );
    if( slist_int == NULL)
    {
        ret = DDERR_OUTOFMEMORY;
        goto cleanup;
    }

    DxHeapMemAlloc(slist_lcl, num_of_surf * sizeof( LPDDRAWI_DDRAWSURFACE_LCL ) );
    if( slist_lcl == NULL )
    {
        ret = DDERR_OUTOFMEMORY;
        goto cleanup;
    }

    /* keep pointers to all gbl surfs to be able to free them on error */
    DxHeapMemAlloc(slist_gbl, num_of_surf * sizeof( LPDDRAWI_DDRAWSURFACE_GBL ) );
    if( slist_gbl == NULL )
    {
        DxHeapMemFree(slist_int);
        return DDERR_OUTOFMEMORY;
    }

    /* keep pointers to all more surfs to be able to free them on error */
    DxHeapMemAlloc(slist_more, num_of_surf * sizeof( LPDDRAWI_DDRAWSURFACE_MORE ) );
    if( slist_more == NULL )
    {
        DxHeapMemFree(slist_int);
        return DDERR_OUTOFMEMORY;
    }

    for( count=0; count < num_of_surf; count++ )
    {
        /* Allocate the surface interface and needed members */
        DxHeapMemAlloc(ThisSurfInt,  sizeof( DDRAWI_DDRAWSURFACE_INT ) );
        if( ThisSurfInt == NULL )
        {
            ret = DDERR_OUTOFMEMORY;
            goto cleanup;
        }

        DxHeapMemAlloc(ThisSurfLcl,  sizeof( DDRAWI_DDRAWSURFACE_LCL ) );
        if( ThisSurfLcl == NULL )
        {
            ret = DDERR_OUTOFMEMORY;
            goto cleanup;
        }

        DxHeapMemAlloc(ThisSurfGbl,  sizeof( DDRAWI_DDRAWSURFACE_GBL ) );
        if( ThisSurfGbl == NULL )
        {
            ret = DDERR_OUTOFMEMORY;
            goto cleanup;
        }

        DxHeapMemAlloc(ThisSurfMore, sizeof( DDRAWI_DDRAWSURFACE_MORE ) );
        if( ThisSurfMore == NULL )
        {
            ret = DDERR_OUTOFMEMORY;
            goto cleanup;
        }

        /* setup lists, really needed are slist_lcl, slist_int
          other slists should be released on return */

        slist_int[count] = ThisSurfInt;
        slist_lcl[count] = ThisSurfLcl;
        slist_gbl[count] = ThisSurfGbl;
        slist_more[count] = ThisSurfMore;

        /* Start now fill in the member as they shall look like before call to createsurface */

        ThisSurfInt->lpLcl = ThisSurfLcl;
        ThisSurfLcl->lpGbl = ThisSurfGbl;

        ThisSurfLcl->ddsCaps.dwCaps = pDDSD->ddsCaps.dwCaps;

        ThisSurfGbl->lpDD = pDDraw->lpLcl->lpGbl;
        ThisSurfGbl->lpDDHandle = pDDraw->lpLcl->lpGbl;

        /* FIXME ? */
        ThisSurfGbl->dwGlobalFlags = DDRAWISURFGBL_ISGDISURFACE;

        if (pDDSD->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
        {
            ThisSurfGbl->wWidth  = pDDraw->lpLcl->lpGbl->vmiData.dwDisplayWidth;
            ThisSurfGbl->wHeight = pDDraw->lpLcl->lpGbl->vmiData.dwDisplayHeight;
            ThisSurfGbl->lPitch  = pDDraw->lpLcl->lpGbl->vmiData.lDisplayPitch;
            ThisSurfGbl->dwLinearSize = pDDraw->lpLcl->lpGbl->vmiData.lDisplayPitch;


            ThisSurfMore->dmiDDrawReserved7.wWidth = pDDraw->lpLcl->lpGbl->vmiData.dwDisplayWidth;
            ThisSurfMore->dmiDDrawReserved7.wHeight = pDDraw->lpLcl->lpGbl->vmiData.dwDisplayHeight;
            ThisSurfMore->dmiDDrawReserved7.wBPP    = pDDraw->lpLcl->lpGbl->dwMonitorFrequency;

            /* FIXME  ThisSurfaceMore->dmiDDrawReserved7.wMonitorsAttachedToDesktop */
            ThisSurfMore->dmiDDrawReserved7.wMonitorsAttachedToDesktop = 1;
            pDDraw->lpLcl->lpPrimary = ThisSurfInt;
            Main_DirectDraw_AddRef(pDDraw);
        }
        else
        {
            ThisSurfGbl->wWidth  = (WORD)pDDSD->dwWidth;
            ThisSurfGbl->wHeight = (WORD)pDDSD->dwHeight;
            ThisSurfGbl->lPitch  = pDDSD->lPitch;
            ThisSurfGbl->dwLinearSize = pDDSD->lPitch;
        }

        if(pDDraw->lpVtbl == &DirectDraw7_Vtable)
        {
            ThisSurfInt->lpVtbl = &DirectDrawSurface7_Vtable;
        }
        else if(pDDraw->lpVtbl == &DirectDraw4_Vtable)
        {
            ThisSurfInt->lpVtbl = &DirectDrawSurface4_Vtable;
        }
        else if(pDDraw->lpVtbl == &DirectDraw2_Vtable)
        {
            ThisSurfInt->lpVtbl = &DirectDrawSurface2_Vtable;
        }
        else if(pDDraw->lpVtbl == &DirectDraw_Vtable)
        {
            ThisSurfInt->lpVtbl = &DirectDrawSurface_Vtable;
        }
        else
        {
            ret =  DDERR_NOTINITIALIZED;
            goto cleanup;
        }

        ThisSurfLcl->lpSurfMore = ThisSurfMore;
        ThisSurfMore->dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);
        ThisSurfMore->lpDD_int = pDDraw;
        ThisSurfMore->lpDD_lcl = pDDraw->lpLcl;
        ThisSurfMore->slist = slist_lcl;

        ThisSurfLcl->dwProcessId = GetCurrentProcessId();

        /* FIXME the lpLnk */

        Main_DDrawSurface_AddRef(ThisSurfInt);
    }

    pDDraw->lpLcl->lpGbl->dsList = (LPDDRAWI_DDRAWSURFACE_INT) slist_int;

    /* Fixme call on DdCanCreate then on DdCreateSurface createsurface data here */

    /* FIXME bIsDifferentPixelFormat being set to true or false with automatic detcitons */
    mDdCanCreateSurface.bIsDifferentPixelFormat = FALSE;

    mDdCanCreateSurface.lpDD = pDDraw->lpLcl->lpGbl;
    mDdCanCreateSurface.CanCreateSurface = pDDraw->lpLcl->lpDDCB->HALDD.CanCreateSurface;
    mDdCanCreateSurface.lpDDSurfaceDesc = (LPDDSURFACEDESC) pDDSD;
    mDdCanCreateSurface.ddRVal = DDERR_GENERIC;

    if (mDdCanCreateSurface.CanCreateSurface(&mDdCanCreateSurface) == DDHAL_DRIVER_NOTHANDLED)
    {
        DX_STUB_str("mDdCanCreateSurface failed with DDHAL_DRIVER_NOTHANDLED.");
        ret = DDERR_NOTINITIALIZED;
        goto cleanup;
    }

    if (mDdCanCreateSurface.ddRVal != DD_OK)
    {
        DX_STUB_str("mDdCanCreateSurface failed.");
        ret = mDdCanCreateSurface.ddRVal;
        goto cleanup;
    }

    mDdCreateSurface.lpDD = pDDraw->lpLcl->lpGbl;
    mDdCreateSurface.CreateSurface = pDDraw->lpLcl->lpGbl->lpDDCBtmp->HALDD.CreateSurface;
    mDdCreateSurface.ddRVal = DDERR_GENERIC;
    mDdCreateSurface.dwSCnt = num_of_surf;
    mDdCreateSurface.lpDDSurfaceDesc = (LPDDSURFACEDESC) pDDSD;
    mDdCreateSurface.lplpSList = slist_lcl;

    if (mDdCreateSurface.CreateSurface(&mDdCreateSurface) == DDHAL_DRIVER_NOTHANDLED)
    {
        DX_STUB_str("mDdCreateSurface failed with DDHAL_DRIVER_NOTHANDLED.");
        ret = DDERR_NOTINITIALIZED;
        goto cleanup;
    }

    if (mDdCreateSurface.ddRVal != DD_OK)
    {
        DX_STUB_str("mDdCreateSurface failed.");
        ret = mDdCreateSurface.ddRVal;
        goto cleanup;
    }

    /* free unneeded slists */
    if (slist_more != NULL)
        DxHeapMemFree(slist_more);
    if (slist_gbl != NULL)
        DxHeapMemFree(slist_gbl);

    *ppSurf = (LPDDRAWI_DDRAWSURFACE_INT) &slist_int[0]->lpVtbl;

    return DD_OK;

cleanup:
    for(count = 0; count < num_of_surf; count++)
    {
        if (slist_more[count] != NULL)
            DxHeapMemFree(slist_more[count]);
        if (slist_gbl[count] != NULL)
            DxHeapMemFree(slist_gbl[count]);
        if (slist_lcl[count] != NULL)
            DxHeapMemFree(slist_lcl[count]);
        if (slist_int[count] != NULL)
            DxHeapMemFree(slist_int[count]);
    }
    if (slist_more != NULL)
        DxHeapMemFree(slist_more);
    if (slist_gbl != NULL)
        DxHeapMemFree(slist_gbl);
    if (slist_lcl != NULL)
        DxHeapMemFree(slist_lcl);
    if (slist_int != NULL)
        DxHeapMemFree(slist_int);

    return ret;
}



void CopyDDSurfDescToDDSurfDesc2(LPDDSURFACEDESC2 dst_pDesc, LPDDSURFACEDESC src_pDesc)
{
    RtlZeroMemory(dst_pDesc,sizeof(DDSURFACEDESC2));
    RtlCopyMemory(dst_pDesc,src_pDesc,sizeof(DDSURFACEDESC));
    dst_pDesc->dwSize =  sizeof(DDSURFACEDESC2);
}





