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
                        LPDIRECTDRAWSURFACE7 *ppSurf, IUnknown *pUnkOuter)
{
    DDSURFACEDESC2 desc;

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


    /* Fixme adding vaidlate of income param */

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
        ThisSurfaceGbl->lpDD = pDDraw->lpLcl->lpGbl;
        

        /* FIXME set right version */
        ThisSurfInt->lpVtbl = &DirectDrawSurface7_Vtable;

        ThisSurfLcl->lpSurfMore = ThisSurfaceMore;
        ThisSurfaceMore->dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);
        ThisSurfaceMore->lpDD_int = pDDraw;
        ThisSurfaceMore->lpDD_lcl = pDDraw->lpLcl;
        ThisSurfaceMore->slist = slist_lcl;

        ThisSurfLcl->dwProcessId = GetCurrentProcessId();

        /* FIXME the lpLnk */
        /* FIXME the ref counter */
    }


    /* Fixme call on DdCanCreate then on DdCreateSurface createsurface data here */


    *ppSurf = &slist_int[0]->lpVtbl;
    return DD_FALSE;
}

void CopyDDSurfDescToDDSurfDesc2(LPDDSURFACEDESC2 dst_pDesc, LPDDSURFACEDESC src_pDesc)
{
    RtlZeroMemory(dst_pDesc,sizeof(DDSURFACEDESC2));
    RtlCopyMemory(dst_pDesc,src_pDesc,sizeof(DDSURFACEDESC));
    dst_pDesc->dwSize =  sizeof(DDSURFACEDESC2);
}

HRESULT 
CreatePrimarySurface(LPDDRAWI_DIRECTDRAW_INT This, 
              LPDDRAWI_DDRAWSURFACE_INT That,
              LPDDRAWI_DDRAWSURFACE_LCL lpLcl,
              LPDDSURFACEDESC2 pDDSD)
{
    DDHAL_CANCREATESURFACEDATA mDdCanCreateSurface;
    DDHAL_CREATESURFACEDATA mDdCreateSurface;


    DxHeapMemAlloc( That->lpLcl->lpSurfMore, sizeof(DDRAWI_DDRAWSURFACE_MORE));
    if (That->lpLcl->lpSurfMore == NULL)
    {
        DxHeapMemFree(That);
        return DDERR_OUTOFMEMORY;
    }

   // That->lpLcl->lpSurfMore->slist = lpLcl;

    That->lpVtbl = &DirectDrawSurface7_Vtable;
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

      That->lpLcl->lpSurfMore->slist = mDdCreateSurface.lplpSList ;

      That->lpLink = This->lpLcl->lpGbl->dsList;
      This->lpLcl->lpGbl->dsList = That;

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
