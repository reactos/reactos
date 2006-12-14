
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



HRESULT 
CreateOverlaySurface(LPDDRAWI_DIRECTDRAW_INT This, 
              LPDDRAWI_DDRAWSURFACE_INT That,
              LPDDSURFACEDESC2 pDDSD)
{

  DDSURFACEDESC mddsdOverlay;
  DDRAWI_DDRAWSURFACE_GBL mOverlayGlobal;
  DDRAWI_DDRAWSURFACE_LCL mOverlayLocal[6];
  DDRAWI_DDRAWSURFACE_LCL *mpOverlayLocals[6];
  DDRAWI_DDRAWSURFACE_MORE mOverlayMore[6];
  DDHAL_CANCREATESURFACEDATA mDdCanCreateSurface;
  DDHAL_CREATESURFACEDATA mDdCreateSurface;
  INT i;
  INT j;
  INT cSurfaces;
  DDHAL_UPDATEOVERLAYDATA      mDdUpdateOverlay;

  /* create overlay surface now */  
  ZeroMemory(&mddsdOverlay, sizeof(DDSURFACEDESC));
  mddsdOverlay.dwSize = sizeof(DDSURFACEDESC);
  mddsdOverlay.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_BACKBUFFERCOUNT | DDSD_WIDTH | DDSD_HEIGHT;

  mddsdOverlay.ddsCaps.dwCaps = DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM | DDSCAPS_COMPLEX | DDSCAPS_FLIP;

  mddsdOverlay.dwWidth = 100;  //pels;
  mddsdOverlay.dwHeight = 100; // lines;
  mddsdOverlay.dwBackBufferCount = 1; //cBuffers;

  mddsdOverlay.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
  mddsdOverlay.ddpfPixelFormat.dwFlags = DDPF_RGB; 
  mddsdOverlay.ddpfPixelFormat.dwRGBBitCount = 32;
  

   //DDHAL_CANCREATESURFACEDATA   mDdCanCreateSurface;
  mDdCanCreateSurface.lpDD = This->lpLcl->lpGbl;
   mDdCanCreateSurface.CanCreateSurface = This->lpLcl->lpDDCB->cbDDCallbacks.CanCreateSurface;
   mDdCanCreateSurface.bIsDifferentPixelFormat = TRUE; //isDifferentPixelFormat;
   mDdCanCreateSurface.lpDDSurfaceDesc = &mddsdOverlay; // pDDSD;
   
   
   if (mDdCanCreateSurface.CanCreateSurface(&mDdCanCreateSurface)== DDHAL_DRIVER_NOTHANDLED) 
   {
    // derr(L"DirectDrawImpl[%08x]::__createPrimary Cannot create primary [%08x]", this, rv);
   // printf("Fail to mDdCanCreateSurface DDHAL_DRIVER_NOTHANDLED \n"); 
       DX_STUB_str("mDdCanCreateSurface DDHAL_DRIVER_NOTHANDLED fail");
    return DDERR_NOTINITIALIZED;
   }

   if (mDdCanCreateSurface.ddRVal != DD_OK)
   {
       DX_STUB_str("mDdCanCreateSurface fail");
// printf("Fail to mDdCanCreateSurface mDdCanCreateSurface.ddRVal = %d:%s\n",(int)mDdCanCreateSurface.ddRVal,DDErrorString(mDdCanCreateSurface.ddRVal)); 
     return DDERR_NOTINITIALIZED;
   }

 
  ZeroMemory(&mOverlayGlobal, sizeof(DDRAWI_DDRAWSURFACE_GBL));
  mOverlayGlobal.dwGlobalFlags = 0;
  mOverlayGlobal.lpDD       = This->lpLcl->lpGbl;
  mOverlayGlobal.lpDDHandle = This->lpLcl->lpGbl;
  mOverlayGlobal.wWidth  = (WORD)mddsdOverlay.dwWidth;
  mOverlayGlobal.wHeight = (WORD)mddsdOverlay.dwHeight;
  mOverlayGlobal.lPitch  = -1;
  mOverlayGlobal.ddpfSurface = mddsdOverlay.ddpfPixelFormat;

  // setup front- and backbuffer surfaces
  cSurfaces = mddsdOverlay.dwBackBufferCount + 1;
  for ( i = 0; i < cSurfaces; i++)
  {
     ZeroMemory(&mOverlayMore[i], sizeof(DDRAWI_DDRAWSURFACE_MORE));
     mOverlayMore[i].dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);

     ZeroMemory(&mOverlayLocal[i], sizeof(DDRAWI_DDRAWSURFACE_LCL));
     mOverlayLocal[i].lpGbl = &mOverlayGlobal;
     mOverlayLocal[i].lpSurfMore = &mOverlayMore[i];
     mOverlayLocal[i].dwProcessId = GetCurrentProcessId();
     mOverlayLocal[i].dwFlags = (i == 0) ?
      (DDRAWISURF_IMPLICITROOT|DDRAWISURF_FRONTBUFFER):
      (DDRAWISURF_IMPLICITCREATE|DDRAWISURF_BACKBUFFER);

     mOverlayLocal[i].dwFlags |= 
      DDRAWISURF_ATTACHED|DDRAWISURF_ATTACHED_FROM|
      DDRAWISURF_HASPIXELFORMAT|
      DDRAWISURF_HASOVERLAYDATA;

     mOverlayLocal[i].ddsCaps.dwCaps = mddsdOverlay.ddsCaps.dwCaps;
     mpOverlayLocals[i] = &mOverlayLocal[i];
  }

  for ( i = 0; i < cSurfaces; i++)
  {
    j = (i + 1) % cSurfaces;

	
    /*if (!mHALInfo.lpDDSurfaceCallbacks->AddAttachedSurface(mpOverlayLocals[i], mpOverlayLocals[j])) 
	{
     // derr(L"DirectDrawImpl[%08x]::__setupDevice DdAttachSurface(%d, %d) failed", this, i, j);
      return DD_FALSE;
    }*/

	if (!DdAttachSurface(mpOverlayLocals[i], mpOverlayLocals[j])) 
	{
     // derr(L"DirectDrawImpl[%08x]::__setupDevice DdAttachSurface(%d, %d) failed", this, i, j);
//printf("Fail to DdAttachSurface (%d:%d)\n", i, j);
        DX_STUB_str("DdAttachSurface fail");
      return DD_FALSE;
    }
	
  }

    // DDHAL_CREATESURFACEDATA      mDdCreateSurface;
  mDdCreateSurface.lpDD = This->lpLcl->lpGbl;
  mDdCreateSurface.CreateSurface = This->lpLcl->lpDDCB->cbDDCallbacks.CreateSurface;  
  mDdCreateSurface.lpDDSurfaceDesc = &mddsdOverlay;//pDDSD;
  mDdCreateSurface.lplpSList = mpOverlayLocals; //cSurfaces;
  mDdCreateSurface.dwSCnt = 1 ;  //ppSurfaces;

  if (mDdCreateSurface.CreateSurface(&mDdCreateSurface) == DDHAL_DRIVER_NOTHANDLED)
  {
	DX_STUB_str("mDdCreateSurface DDHAL_DRIVER_NOTHANDLED fail");
	return DDERR_NOTINITIALIZED;
  }
  

  if (mDdCreateSurface.ddRVal != DD_OK) 
  {   
	 DX_STUB_str("mDdCreateSurface fail");
     return mDdCreateSurface.ddRVal;
  }

  mDdUpdateOverlay.lpDD = This->lpLcl->lpGbl;
  mDdUpdateOverlay.UpdateOverlay = This->lpLcl->lpDDCB->HALDDSurface.UpdateOverlay;
  mDdUpdateOverlay.lpDDDestSurface = This->lpLcl->lpPrimary->lpLcl->lpSurfMore->slist[0];
  mDdUpdateOverlay.lpDDSrcSurface = mpOverlayLocals[0];//pDDSurface;
  mDdUpdateOverlay.dwFlags = DDOVER_SHOW;

  mDdUpdateOverlay.rDest.top = 0;
  mDdUpdateOverlay.rDest.left = 0;
  mDdUpdateOverlay.rDest.right = 50;
  mDdUpdateOverlay.rDest.bottom = 50;

   mDdUpdateOverlay.rSrc.top = 0;
  mDdUpdateOverlay.rSrc.left = 0;
  mDdUpdateOverlay.rSrc.right = 50;
  mDdUpdateOverlay.rSrc.bottom = 50;

 
 

  if ( mDdUpdateOverlay.UpdateOverlay(&mDdUpdateOverlay) == DDHAL_DRIVER_NOTHANDLED)
  {
	DX_STUB_str("UpdateOverlay fail");
	 return DDERR_NOTINITIALIZED;
  }
  

  if (mDdUpdateOverlay.ddRVal != DD_OK) 
  {   
      DX_STUB_str("mDdUpdateOverlay fail");
	  //printf("Fail to mDdUpdateOverlay mDdUpdateOverlay.ddRVal = %d:%s\n",(int)mDdUpdateOverlay.ddRVal,DDErrorString(mDdUpdateOverlay.ddRVal)); 
      return mDdUpdateOverlay.ddRVal;
  }

  DX_STUB_str("OK");
  return DD_OK;
}
