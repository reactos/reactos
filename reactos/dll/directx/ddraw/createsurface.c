
#include "rosdraw.h"


HRESULT 
CreatePrimarySurface(LPDDRAWI_DIRECTDRAW_INT This, 
              LPDDRAWI_DDRAWSURFACE_INT That,
              LPDDSURFACEDESC2 pDDSD)
{
    DDHAL_CANCREATESURFACEDATA mDdCanCreateSurface;
    DDHAL_CREATESURFACEDATA mDdCreateSurface;

    That = (LPDDRAWI_DDRAWSURFACE_INT)DxHeapMemAlloc(sizeof(DDRAWI_DDRAWSURFACE_INT));
    if (That == NULL) 
    {
        return DDERR_OUTOFMEMORY;
    }

    That->lpLcl = (LPDDRAWI_DDRAWSURFACE_LCL)DxHeapMemAlloc(sizeof(DDRAWI_DDRAWSURFACE_LCL));   
    if (That->lpLcl == NULL) 
    {
        DxHeapMemFree(That);
        return DDERR_OUTOFMEMORY;
    }

    That->lpLcl->lpSurfMore =  DxHeapMemAlloc(sizeof(DDRAWI_DDRAWSURFACE_MORE));
    if (That->lpLcl->lpSurfMore == NULL)
    {
        DxHeapMemFree(That->lpLcl);
        DxHeapMemFree(That);
        return DDERR_OUTOFMEMORY;
    }

    That->lpLcl->lpSurfMore->slist = DxHeapMemAlloc(sizeof(LPDDRAWI_DDRAWSURFACE_LCL)<<1);
    if (That->lpLcl->lpSurfMore->slist == NULL)
    {
        DxHeapMemFree(That->lpLcl->lpSurfMore);
        DxHeapMemFree(That->lpLcl);
        DxHeapMemFree(That);
        return DDERR_OUTOFMEMORY;
    }

    That->lpVtbl = &DirectDrawSurface7_Vtable;
    That->lpLcl->lpGbl = &ddSurfGbl;
    That->lpLcl->lpSurfMore->dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);
    That->lpLcl->lpSurfMore->lpDD_int = This;
    That->lpLcl->lpSurfMore->lpDD_lcl = This->lpLcl;
    That->lpLcl->lpSurfMore->slist[0] = That->lpLcl;
    That->lpLcl->dwProcessId = GetCurrentProcessId();

    mDdCanCreateSurface.lpDD = This->lpLcl->lpGbl;
    if  (pDDSD->dwFlags & DDSD_PIXELFORMAT)
    {
        That->lpLcl->dwFlags |= DDRAWISURF_HASPIXELFORMAT;
        mDdCanCreateSurface.bIsDifferentPixelFormat = TRUE; //isDifferentPixelFormat;
    }
    else
    {
        mDdCanCreateSurface.bIsDifferentPixelFormat = FALSE; //isDifferentPixelFormat;
    }
    mDdCanCreateSurface.lpDDSurfaceDesc = (LPDDSURFACEDESC) pDDSD;
    mDdCanCreateSurface.CanCreateSurface = This->lpLcl->lpDDCB->cbDDCallbacks.CanCreateSurface;
    mDdCanCreateSurface.ddRVal = DDERR_GENERIC;

    mDdCreateSurface.lpDD = This->lpLcl->lpGbl;
    mDdCreateSurface.CreateSurface = This->lpLcl->lpDDCB->cbDDCallbacks.CreateSurface;
    mDdCreateSurface.ddRVal =  DDERR_GENERIC;
    mDdCreateSurface.dwSCnt = That->dwIntRefCnt + 1; // is this correct 
    mDdCreateSurface.lpDDSurfaceDesc = (LPDDSURFACEDESC) pDDSD;

    mDdCreateSurface.lplpSList = That->lpLcl->lpSurfMore->slist;

           That->lpLcl->ddsCaps.dwCaps = pDDSD->ddsCaps.dwCaps;

       This->lpLcl->lpPrimary = That;
       if (mDdCanCreateSurface.CanCreateSurface(&mDdCanCreateSurface)== DDHAL_DRIVER_NOTHANDLED) 
       {   
           return DDERR_NOTINITIALIZED;
       }

       if (mDdCanCreateSurface.ddRVal != DD_OK)
       {
           return DDERR_NOTINITIALIZED;
       }

       mDdCreateSurface.lplpSList = That->lpLcl->lpSurfMore->slist;

       if (mDdCreateSurface.CreateSurface(&mDdCreateSurface) == DDHAL_DRIVER_NOTHANDLED)
       {
           return DDERR_NOTINITIALIZED;
       }

       if (mDdCreateSurface.ddRVal != DD_OK) 
       {   
           return mDdCreateSurface.ddRVal;
       }

       return DD_OK;
}

HRESULT 
CreateBackBufferSurface(LPDDRAWI_DIRECTDRAW_INT This, 
              LPDDRAWI_DDRAWSURFACE_INT That,
              LPDDSURFACEDESC2 pDDSD)
{
    DDHAL_CANCREATESURFACEDATA mDdCanCreateSurface;
    DDHAL_CREATESURFACEDATA mDdCreateSurface;
    DWORD t;


    /* we are building the backbuffersurface pointer list 
     * and create the backbuffer surface and set it up 
     */

    for (t=0;t<pDDSD->dwBackBufferCount;t++)
    {
        That = (LPDDRAWI_DDRAWSURFACE_INT)DxHeapMemAlloc(sizeof(DDRAWI_DDRAWSURFACE_INT));
        if (That == NULL) 
        {
            return DDERR_OUTOFMEMORY;
        }

        That->lpLcl = (LPDDRAWI_DDRAWSURFACE_LCL)DxHeapMemAlloc(sizeof(DDRAWI_DDRAWSURFACE_LCL));   
        if (That->lpLcl == NULL) 
        {
            return DDERR_OUTOFMEMORY;
        }

        That->lpLcl->lpSurfMore =  DxHeapMemAlloc(sizeof(DDRAWI_DDRAWSURFACE_MORE));
        if (That->lpLcl->lpSurfMore == NULL)
        {
            return DDERR_OUTOFMEMORY;
        }

        That->lpLcl->lpSurfMore->slist = DxHeapMemAlloc(sizeof(LPDDRAWI_DDRAWSURFACE_LCL)<<1);
        if (That->lpLcl->lpSurfMore->slist == NULL)
        {
            return DDERR_OUTOFMEMORY;
        }

        That->lpLcl->lpGbl = DxHeapMemAlloc(sizeof(DDRAWI_DDRAWSURFACE_GBL));
        if (That->lpLcl->lpGbl == NULL)
        {
            return DDERR_OUTOFMEMORY;
        }

        memcpy(That->lpLcl->lpGbl, &ddSurfGbl,sizeof(DDRAWI_DDRAWSURFACE_GBL));
        That->lpVtbl = &DirectDrawSurface7_Vtable;
        That->lpLcl->lpSurfMore->dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);
        That->lpLcl->lpSurfMore->lpDD_int = This;
        That->lpLcl->lpSurfMore->lpDD_lcl = This->lpLcl;
        That->lpLcl->lpSurfMore->slist[0] = That->lpLcl;
        That->lpLcl->dwProcessId = GetCurrentProcessId();

        That->lpVtbl = &DirectDrawSurface7_Vtable;
        That->lpLcl->lpSurfMore->dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);
        That->lpLcl->lpSurfMore->lpDD_int = This;
        That->lpLcl->lpSurfMore->lpDD_lcl = This->lpLcl;
        That->lpLcl->lpSurfMore->slist[0] = That->lpLcl;
        That->lpLcl->dwProcessId = GetCurrentProcessId();

        if  (pDDSD->dwFlags & DDSD_PIXELFORMAT)
        {
            That->lpLcl->dwFlags |= DDRAWISURF_HASPIXELFORMAT;
        }

        mDdCanCreateSurface.lpDD = This->lpLcl->lpGbl;

        if (pDDSD->dwFlags & DDSD_PIXELFORMAT)
        {
            That->lpLcl->dwFlags |= DDRAWISURF_HASPIXELFORMAT;
            mDdCanCreateSurface.bIsDifferentPixelFormat = TRUE; //isDifferentPixelFormat;
        }
        else
        {
            mDdCanCreateSurface.bIsDifferentPixelFormat = FALSE; //isDifferentPixelFormat;
        }

        That->lpLcl->ddsCaps.dwCaps = pDDSD->ddsCaps.dwCaps;

        mDdCanCreateSurface.lpDDSurfaceDesc = (LPDDSURFACEDESC) pDDSD;
        mDdCanCreateSurface.CanCreateSurface = This->lpLcl->lpDDCB->cbDDCallbacks.CanCreateSurface;
        mDdCanCreateSurface.ddRVal = DDERR_GENERIC;

        mDdCreateSurface.lpDD = This->lpLcl->lpGbl;
        mDdCreateSurface.CreateSurface = This->lpLcl->lpDDCB->cbDDCallbacks.CreateSurface;
        mDdCreateSurface.ddRVal =  DDERR_GENERIC;
        mDdCreateSurface.dwSCnt = That->dwIntRefCnt + 1; // is this correct 
        mDdCreateSurface.lpDDSurfaceDesc = (LPDDSURFACEDESC) pDDSD;

        mDdCreateSurface.lplpSList = That->lpLcl->lpSurfMore->slist;

        if (mDdCanCreateSurface.CanCreateSurface(&mDdCanCreateSurface)== DDHAL_DRIVER_NOTHANDLED) 
        {
            return DDERR_NOTINITIALIZED;
        }

        if (mDdCanCreateSurface.ddRVal != DD_OK)
        {
            return DDERR_NOTINITIALIZED;
        }

        mDdCreateSurface.lplpSList = That->lpLcl->lpSurfMore->slist;

        if (mDdCreateSurface.CreateSurface(&mDdCreateSurface) == DDHAL_DRIVER_NOTHANDLED)
        {
            return DDERR_NOTINITIALIZED;
        }

        if (mDdCreateSurface.ddRVal != DD_OK) 
        {
            return mDdCreateSurface.ddRVal;
        }

        /* Build the linking buffer */
        That->lpLink = This->lpLcl->lpGbl->dsList;
        This->lpLcl->lpGbl->dsList = That;
DX_STUB_str( "ok");
    }
   return DD_OK;
}
