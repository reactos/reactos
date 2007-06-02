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

/*
 * all param have been checked if they are vaild before they are call to 
 * Internal_CreateSurface, if not please fix the code in the functions 
 * call to Internal_CreateSurface, ppSurf,pDDSD,pDDraw  are being vaildate in 
 * Internal_CreateSurface
 */

HRESULT 
Internal_CreateSurface( LPDDRAWI_DIRECTDRAW_INT pDDraw, LPDDSURFACEDESC2 pDDSD,
                        LPDIRECTDRAWSURFACE7 *ppSurf, IUnknown *pUnkOuter)
{
    DDSURFACEDESC2 desc;

    /* Test se if the pointers are vaild */
    if ((IsBadReadPtr(pDDraw,sizeof(LPDDRAWI_DIRECTDRAW_INT))) ||
       (IsBadReadPtr(pDDSD,sizeof(LPDDSURFACEDESC2))) ||
       (IsBadWritePtr(ppSurf,sizeof(LPDIRECTDRAWSURFACE7))) ||
       (IsBadReadPtr(ppSurf,sizeof(LPDIRECTDRAWSURFACE7))))
    {
        return DDERR_INVALIDPARAMS;
    }

    /* make local backup of DDSURFACEDESC2 */
    RtlCopyMemory(&desc, pDDSD, sizeof(DDSURFACEDESC2));

    /* 
     * pDDSD->dwCaps can not contain both DDSCAPS_SYSTEMMEMORY and DDSCAPS_VIDEOMEMORY
     * if both are define ddraw.dll will return error code 0x88760064
     */
    if ( (desc.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) && 
         (desc.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
    {
        return DDERR_INVALIDCAPS;
    }

    /* check if pDDSD.dwFlags DDSD_LPSURFACE is set or not */
    if (desc.dwFlags & DDSD_LPSURFACE)
    {
        /*
         *  DDSD_LPSURFACE flag is set now we start vaildate see if
         *  pDDSD->lpSurface are a pointer or not 
         */
        if (IsBadReadPtr(desc.lpSurface,sizeof(LPVOID)) ||
            (desc.dwFlags - (desc.dwFlags & DDSD_ALL)))
        {
            return DDERR_INVALIDPARAMS;
        }

        /* only support dx7 interface for DDSD_LPSURFACE older interface do not support it */
        if (pDDraw->lpVtbl != &DirectDraw7_Vtable)
        {
            return DDERR_INVALIDPARAMS;
        }

        /* vaildate the caps */
        if (desc.ddsCaps.dwCaps & ( DDSCAPS_COMPLEX | DDSCAPS_RESERVED2 | DDSCAPS_RESERVED3 |
                                       DDSCAPS_VIDEOPORT | DDSCAPS_PRIMARYSURFACE | DDSCAPS_OVERLAY ))
        {
            return DDERR_INVALIDCAPS;
        }

        /* more code will follow */
    }
    else
    {
       /*
        *  DDSD_LPSURFACE flag are not set
        *  remove all wrong flags are being set
        */

        desc.dwFlags = desc.dwFlags & DDSD_ALL;
    }

     /* more code will follow */



    return DDERR_GENERIC;
}

void CopyDDSurfDescToDDSurfDesc2(LPDDSURFACEDESC2 dst_pDesc, LPDDSURFACEDESC src_pDesc)
{
    RtlZeroMemory(dst_pDesc,sizeof(DDSURFACEDESC2));
    RtlCopyMemory(dst_pDesc,src_pDesc,sizeof(DDSURFACEDESC));
    dst_pDesc->dwSize =  sizeof(DDSURFACEDESC2);
}

HRESULT 
CreatePrimarySurface(LPDDRAWI_DIRECTDRAW_INT This, 
              LPDDRAWI_DDRAWSURFACE_INT *That,
              LPDDRAWI_DDRAWSURFACE_LCL *lpLcl,
              LPDDSURFACEDESC2 pDDSD)
{
    DDHAL_CANCREATESURFACEDATA mDdCanCreateSurface;
    DDHAL_CREATESURFACEDATA mDdCreateSurface;


    DxHeapMemAlloc( That[0]->lpLcl->lpSurfMore, sizeof(DDRAWI_DDRAWSURFACE_MORE));
    if (That[0]->lpLcl->lpSurfMore == NULL)
    {
        DxHeapMemFree(That);
        return DDERR_OUTOFMEMORY;
    }

    That[0]->lpLcl->lpSurfMore->slist = lpLcl;

    That[0]->lpVtbl = &DirectDrawSurface7_Vtable;
    That[0]->lpLcl->lpSurfMore->dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);
    That[0]->lpLcl->lpSurfMore->lpDD_int = This;
    That[0]->lpLcl->lpSurfMore->lpDD_lcl = This->lpLcl;
    That[0]->lpLcl->lpSurfMore->slist[0] = That[0]->lpLcl;
    That[0]->lpLcl->dwProcessId = GetCurrentProcessId();

    mDdCanCreateSurface.lpDD = This->lpLcl->lpGbl;
    if  (pDDSD->dwFlags & DDSD_PIXELFORMAT)
    {
        That[0]->lpLcl->dwFlags |= DDRAWISURF_HASPIXELFORMAT;
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
    mDdCreateSurface.dwSCnt = That[0]->dwIntRefCnt + 1; // is this correct 
    mDdCreateSurface.lpDDSurfaceDesc = (LPDDSURFACEDESC) pDDSD;

    mDdCreateSurface.lplpSList = That[0]->lpLcl->lpSurfMore->slist;

           That[0]->lpLcl->ddsCaps.dwCaps = pDDSD->ddsCaps.dwCaps;

       This->lpLcl->lpPrimary = That[0];
       if (mDdCanCreateSurface.CanCreateSurface(&mDdCanCreateSurface)== DDHAL_DRIVER_NOTHANDLED) 
       {   
           return DDERR_NOTINITIALIZED;
       }

       if (mDdCanCreateSurface.ddRVal != DD_OK)
       {
           return DDERR_NOTINITIALIZED;
       }

       mDdCreateSurface.lplpSList = That[0]->lpLcl->lpSurfMore->slist;

       if (mDdCreateSurface.CreateSurface(&mDdCreateSurface) == DDHAL_DRIVER_NOTHANDLED)
       {
           return DDERR_NOTINITIALIZED;
       }

       if (mDdCreateSurface.ddRVal != DD_OK) 
       {   
           return mDdCreateSurface.ddRVal;
       }

      That[0]->lpLcl->lpSurfMore->slist = mDdCreateSurface.lplpSList ;

      That[0]->lpLink = This->lpLcl->lpGbl->dsList;
      This->lpLcl->lpGbl->dsList = That[0];

       return DD_OK;
}

HRESULT 
CreateBackBufferSurface(LPDDRAWI_DIRECTDRAW_INT This, 
              LPDDRAWI_DDRAWSURFACE_INT *That,
              LPDDRAWI_DDRAWSURFACE_LCL *lpLcl,
              LPDDSURFACEDESC2 pDDSD)
{
    DDHAL_CANCREATESURFACEDATA mDdCanCreateSurface;
    DDHAL_CREATESURFACEDATA mDdCreateSurface;
    DWORD t;


    /* we are building the backbuffersurface pointer list 
     * and create the backbuffer surface and set it up 
     */

    That[0]->lpLcl->dwBackBufferCount = pDDSD->dwBackBufferCount;

    for (t=0;t<pDDSD->dwBackBufferCount+1;t++)
    {
        

    DxHeapMemAlloc(That[t]->lpLcl->lpSurfMore, sizeof(DDRAWI_DDRAWSURFACE_MORE));
    if (That[t]->lpLcl->lpSurfMore == NULL)
    {
        DxHeapMemFree(That);
        return DDERR_OUTOFMEMORY;
    }

    That[t]->lpLcl->lpSurfMore->slist = lpLcl;

    That[t]->lpVtbl = &DirectDrawSurface7_Vtable; 
    That[t]->lpLcl->lpSurfMore->dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);
    That[t]->lpLcl->lpSurfMore->lpDD_int = This;
    That[t]->lpLcl->lpSurfMore->lpDD_lcl = This->lpLcl;
    That[t]->lpLcl->lpSurfMore->slist[0] = That[t]->lpLcl;
    That[t]->lpLcl->dwProcessId = GetCurrentProcessId();

    mDdCanCreateSurface.lpDD = This->lpLcl->lpGbl;
    if  (pDDSD->dwFlags & DDSD_PIXELFORMAT)
    {
        That[t]->lpLcl->dwFlags |= DDRAWISURF_HASPIXELFORMAT;
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
    mDdCreateSurface.dwSCnt = That[t]->dwIntRefCnt + 1; // is this correct 
    mDdCreateSurface.lpDDSurfaceDesc = (LPDDSURFACEDESC) pDDSD;

    mDdCreateSurface.lplpSList = That[t]->lpLcl->lpSurfMore->slist;

           That[t]->lpLcl->ddsCaps.dwCaps = pDDSD->ddsCaps.dwCaps;

       This->lpLcl->lpPrimary = That[0];
       if (mDdCanCreateSurface.CanCreateSurface(&mDdCanCreateSurface)== DDHAL_DRIVER_NOTHANDLED) 
       {   
           return DDERR_NOTINITIALIZED;
       }

       if (mDdCanCreateSurface.ddRVal != DD_OK)
       {
           return DDERR_NOTINITIALIZED;
       }

       mDdCreateSurface.lplpSList = That[t]->lpLcl->lpSurfMore->slist;

       if (mDdCreateSurface.CreateSurface(&mDdCreateSurface) == DDHAL_DRIVER_NOTHANDLED)
       {
           return DDERR_NOTINITIALIZED;
       }

       if (mDdCreateSurface.ddRVal != DD_OK) 
       {   
           return mDdCreateSurface.ddRVal;
       }

      That[t]->lpLcl->lpSurfMore->slist = mDdCreateSurface.lplpSList ;

        /* Build the linking buffer */
        That[t]->lpLink = This->lpLcl->lpGbl->dsList;
        This->lpLcl->lpGbl->dsList = That[t];
DX_STUB_str( "ok");
    }
   return DD_OK;
}



HRESULT 
CreateOverlaySurface(LPDDRAWI_DIRECTDRAW_INT This, 
              LPDDRAWI_DDRAWSURFACE_INT *That,
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
