/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 ddraw/surface/createsurface.c
 * PURPOSE:              IDirectDrawSurface7 Creation
 * PROGRAMMER:           Magnus Olsen
 *
 */
#include "rosdraw.h"

/* PSEH for SEH Support */
#include <pseh/pseh.h>


/*
 * all param have been checked if they are vaild before they are call to
 * Internal_CreateSurface, if not please fix the code in the functions
 * call to Internal_CreateSurface, ppSurf,pDDSD,pDDraw  are being vaildate in
 * Internal_CreateSurface
 */

HRESULT
Internal_CreateSurface( LPDDRAWI_DIRECTDRAW_INT pDDraw, LPDDSURFACEDESC2 pDDSD,
                        LPDDRAWI_DDRAWSURFACE_INT *ppSurf, IUnknown *pUnkOuter)
{
    DDHAL_CANCREATESURFACEDATA mDdCanCreateSurface = { 0 };
    DDHAL_CREATESURFACEDATA mDdCreateSurface = { 0 };

    LPDDRAWI_DDRAWSURFACE_INT ThisSurfInt;
    LPDDRAWI_DDRAWSURFACE_LCL ThisSurfLcl;
    LPDDRAWI_DDRAWSURFACE_GBL ThisSurfaceGbl;
    LPDDRAWI_DDRAWSURFACE_MORE  ThisSurfaceMore;

    LPDDRAWI_DDRAWSURFACE_INT * slist_int;
    LPDDRAWI_DDRAWSURFACE_LCL * slist_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL * slist_gbl;
    LPDDRAWI_DDRAWSURFACE_MORE * slist_more;
    DWORD num_of_surf=1;
    DWORD count;

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

    /* FIXME count our how many surface we need */

    DxHeapMemAlloc(slist_int, num_of_surf * sizeof( LPDDRAWI_DDRAWSURFACE_INT ) );
    if( slist_int == NULL)
    {
        return DDERR_OUTOFMEMORY;
    }

    DxHeapMemAlloc(slist_lcl, num_of_surf * sizeof( LPDDRAWI_DDRAWSURFACE_LCL ) );
    if( slist_lcl == NULL )
    {
        DxHeapMemFree(slist_int);
        return DDERR_OUTOFMEMORY;
    }

    /* for more easy to free the memory if something goes wrong */
    DxHeapMemAlloc(slist_gbl, num_of_surf * sizeof( LPDDRAWI_DDRAWSURFACE_GBL ) );
    if( slist_lcl == NULL )
    {
        DxHeapMemFree(slist_int);
        return DDERR_OUTOFMEMORY;
    }

    /* for more easy to free the memory if something goes wrong */
    DxHeapMemAlloc(slist_more, num_of_surf * sizeof( LPDDRAWI_DDRAWSURFACE_MORE ) );
    if( slist_lcl == NULL )
    {
        DxHeapMemFree(slist_int);
        return DDERR_OUTOFMEMORY;
    }

    for( count=0; count < num_of_surf; count++ )
    {
        /* Alloc the surface interface and need members */
        DxHeapMemAlloc(ThisSurfInt,  sizeof( DDRAWI_DDRAWSURFACE_INT ) );
        if( ThisSurfInt == NULL )
        {
            /* Fixme free the memory */
            return DDERR_OUTOFMEMORY;
        }

        DxHeapMemAlloc(ThisSurfLcl,  sizeof( DDRAWI_DDRAWSURFACE_LCL ) );
        if( ThisSurfLcl == NULL )
        {
            /* Fixme free the memory */
            return DDERR_OUTOFMEMORY;
        }

        DxHeapMemAlloc(ThisSurfaceGbl,  sizeof( DDRAWI_DDRAWSURFACE_GBL ) );
        if( ThisSurfaceGbl == NULL )
        {
            /* Fixme free the memory */
            return DDERR_OUTOFMEMORY;
        }

        DxHeapMemAlloc(ThisSurfaceMore, sizeof( DDRAWI_DDRAWSURFACE_MORE ) );
        if( ThisSurfaceMore == NULL )
        {
            /* Fixme free the memory */
            return DDERR_OUTOFMEMORY;
        }

        /* setup a list only one we really need is  slist_lcl
          rest of slist shall be release before a return */

        slist_int[count] = ThisSurfInt;
        slist_lcl[count] = ThisSurfLcl;
        slist_gbl[count] = ThisSurfaceGbl;
        slist_more[count] = ThisSurfaceMore;

        /* Start now fill in the member as they shall look like before call to createsurface */

        ThisSurfInt->lpLcl = ThisSurfLcl;
        ThisSurfLcl->lpGbl = ThisSurfaceGbl;

        ThisSurfLcl->ddsCaps.dwCaps = pDDSD->ddsCaps.dwCaps;

        ThisSurfaceGbl->lpDD = pDDraw->lpLcl->lpGbl;
        ThisSurfaceGbl->lpDDHandle = pDDraw->lpLcl->lpGbl;

        /* FIXME ? */
        ThisSurfaceGbl->dwGlobalFlags = DDRAWISURFGBL_ISGDISURFACE;

        if (pDDSD->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
        {
            ThisSurfaceGbl->wWidth  = pDDraw->lpLcl->lpGbl->vmiData.dwDisplayWidth;
            ThisSurfaceGbl->wHeight = pDDraw->lpLcl->lpGbl->vmiData.dwDisplayHeight;
            ThisSurfaceGbl->lPitch  = pDDraw->lpLcl->lpGbl->vmiData.lDisplayPitch;
            ThisSurfaceGbl->dwLinearSize = pDDraw->lpLcl->lpGbl->vmiData.lDisplayPitch;


            ThisSurfaceMore->dmiDDrawReserved7.wWidth = pDDraw->lpLcl->lpGbl->vmiData.dwDisplayWidth;
            ThisSurfaceMore->dmiDDrawReserved7.wHeight = pDDraw->lpLcl->lpGbl->vmiData.dwDisplayHeight;
            ThisSurfaceMore->dmiDDrawReserved7.wBPP    = pDDraw->lpLcl->lpGbl->dwMonitorFrequency;

            /* FIXME  ThisSurfaceMore->dmiDDrawReserved7.wMonitorsAttachedToDesktop */
            ThisSurfaceMore->dmiDDrawReserved7.wMonitorsAttachedToDesktop = 1;
            pDDraw->lpLcl->lpPrimary = ThisSurfInt;
            Main_DirectDraw_AddRef(pDDraw);
        }
        else
        {
            ThisSurfaceGbl->wWidth  = (WORD)pDDSD->dwWidth;
            ThisSurfaceGbl->wHeight = (WORD)pDDSD->dwHeight;
            ThisSurfaceGbl->lPitch  = pDDSD->lPitch;
            ThisSurfaceGbl->dwLinearSize = pDDSD->lPitch;
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
            return DDERR_NOTINITIALIZED;
        }

        ThisSurfLcl->lpSurfMore = ThisSurfaceMore;
        ThisSurfaceMore->dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);
        ThisSurfaceMore->lpDD_int = pDDraw;
        ThisSurfaceMore->lpDD_lcl = pDDraw->lpLcl;
        ThisSurfaceMore->slist = slist_lcl;

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

    if (mDdCanCreateSurface.CanCreateSurface(&mDdCanCreateSurface)== DDHAL_DRIVER_NOTHANDLED)
    {
        DX_STUB_str("mDdCanCreateSurface DDHAL_DRIVER_NOTHANDLED fail");
        return DDERR_NOTINITIALIZED;
    }

    if (mDdCanCreateSurface.ddRVal != DD_OK)
    {
        DX_STUB_str("mDdCanCreateSurface fail");
        return DDERR_NOTINITIALIZED;
    }

    mDdCreateSurface.lpDD = pDDraw->lpLcl->lpGbl;
    mDdCreateSurface.CreateSurface = pDDraw->lpLcl->lpGbl->lpDDCBtmp->HALDD.CreateSurface;
    mDdCreateSurface.ddRVal =  DDERR_GENERIC;
    mDdCreateSurface.dwSCnt = num_of_surf;
    mDdCreateSurface.lpDDSurfaceDesc = (LPDDSURFACEDESC) pDDSD;
    mDdCreateSurface.lplpSList = slist_lcl;

    if (mDdCreateSurface.CreateSurface(&mDdCreateSurface) == DDHAL_DRIVER_NOTHANDLED)
    {
        return DDERR_NOTINITIALIZED;
    }

    if (mDdCreateSurface.ddRVal != DD_OK)
    {
        return mDdCreateSurface.ddRVal;
    }

    *ppSurf = (LPDDRAWI_DDRAWSURFACE_INT) &slist_int[0]->lpVtbl;
    return DD_OK;
}



void CopyDDSurfDescToDDSurfDesc2(LPDDSURFACEDESC2 dst_pDesc, LPDDSURFACEDESC src_pDesc)
{
    RtlZeroMemory(dst_pDesc,sizeof(DDSURFACEDESC2));
    RtlCopyMemory(dst_pDesc,src_pDesc,sizeof(DDSURFACEDESC));
    dst_pDesc->dwSize =  sizeof(DDSURFACEDESC2);
}





