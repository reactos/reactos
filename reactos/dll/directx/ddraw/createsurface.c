
#include "rosdraw.h"

HRESULT 
CreateBackBufferSurface(LPDDRAWI_DIRECTDRAW_INT This, 
              LPDDRAWI_DDRAWSURFACE_INT That,
              LPDDSURFACEDESC2 pDDSD)
{

    
    That = (LPDDRAWI_DDRAWSURFACE_INT)DxHeapMemAlloc(sizeof(DDRAWI_DDRAWSURFACE_INT));
    if (That == NULL) 
    {
        return E_OUTOFMEMORY;
    }

    That->lpLcl = (LPDDRAWI_DDRAWSURFACE_LCL)DxHeapMemAlloc(sizeof(DDRAWI_DDRAWSURFACE_LCL));   
    if (That->lpLcl == NULL) 
    {
        /* shall we free it if it fail ?? */
        DxHeapMemFree(That);
        return E_OUTOFMEMORY;
    }

    That->lpLcl->lpSurfMore =  DxHeapMemAlloc(sizeof(DDRAWI_DDRAWSURFACE_MORE));
    if (That->lpLcl->lpSurfMore == NULL)
    {
        /* shall we free it if it fail ?? */
        DxHeapMemFree(That->lpLcl);
        DxHeapMemFree(That);
        return DDERR_OUTOFMEMORY;
    }

    That->lpLcl->lpSurfMore->slist = DxHeapMemAlloc(sizeof(LPDDRAWI_DDRAWSURFACE_LCL)<<1);
    if (That->lpLcl->lpSurfMore->slist == NULL)
    {
        /* shall we free it if it fail ?? */
        DxHeapMemFree(That->lpLcl->lpSurfMore);
        DxHeapMemFree(That->lpLcl);
        DxHeapMemFree(That);
        return DDERR_OUTOFMEMORY;
    }

    That->lpLcl->lpGbl = DxHeapMemAlloc(sizeof(DDRAWI_DDRAWSURFACE_GBL));
    if (That->lpLcl->lpGbl == NULL)
    {
        /* shall we free it if it fail ?? */
        DxHeapMemFree(That->lpLcl->lpSurfMore->slist);
        DxHeapMemFree(That->lpLcl->lpSurfMore);
        DxHeapMemFree(That->lpLcl);
        DxHeapMemFree(That);
        return DDERR_OUTOFMEMORY;
    }

    memcpy(That->lpLcl->lpGbl, &ddSurfGbl,sizeof(DDRAWI_DDRAWSURFACE_GBL);

    That->lpVtbl = &DirectDrawSurface7_Vtable;
    That->lpLcl->lpGbl->lpDD = &ddgbl;
    That->lpLcl->lpSurfMore->dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);
    That->lpLcl->lpSurfMore->lpDD_int = This;
    That->lpLcl->lpSurfMore->lpDD_lcl = This->lpLcl;
    That->lpLcl->lpSurfMore->slist[0] = That->lpLcl;
    That->lpLcl->dwProcessId = GetCurrentProcessId();
    mDdCreateSurface.lplpSList = That->lpLcl->lpSurfMore->slist;



    That->lpVtbl = &DirectDrawSurface7_Vtable;
    That->lpLcl->lpGbl = &ddSurfGbl;
    That->lpLcl->lpGbl->lpDD = &ddgbl;
    That->lpLcl->lpSurfMore->dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);
    That->lpLcl->lpSurfMore->lpDD_int = This;
    That->lpLcl->lpSurfMore->lpDD_lcl = This->lpLcl;
    That->lpLcl->lpSurfMore->slist[0] = That->lpLcl;
    That->lpLcl->dwProcessId = GetCurrentProcessId();

    if  (pDDSD->dwFlags & DDSD_PIXELFORMAT)
    {
        That->lpLcl->dwFlags |= DDRAWISURF_HASPIXELFORMAT;
    }


}
