/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/hal/surface.c
 * PURPOSE:              DirectDraw HAL Implementation 
 * PROGRAMMER:           Magnus Olsen, Maarten Bosma
 *
 */

#include "rosdraw.h"


HRESULT Hal_DirectDraw_CreateSurface (LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD, LPDIRECTDRAWSURFACE7 *ppSurf, IUnknown *pUnkOuter)
{
	UINT i;
	 IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

	  /* create primare surface now */
  
   memset(&This->mddsdPrimary,   0, sizeof(DDSURFACEDESC));
   This->mddsdPrimary.dwSize      = sizeof(DDSURFACEDESC);
   This->mddsdPrimary.dwFlags     = DDSD_CAPS;
   This->mddsdPrimary.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_VIDEOMEMORY | DDSCAPS_VISIBLE;

   DDHAL_CANCREATESURFACEDATA   mDdCanCreateSurface;
   mDdCanCreateSurface.lpDD = &This->mDDrawGlobal;
   mDdCanCreateSurface.CanCreateSurface = This->mCallbacks.HALDD.CanCreateSurface;
   mDdCanCreateSurface.bIsDifferentPixelFormat = FALSE; //isDifferentPixelFormat;
   mDdCanCreateSurface.lpDDSurfaceDesc = &This->mddsdPrimary; // pDDSD;
   
   
   if (This->mHALInfo.lpDDCallbacks->CanCreateSurface(&mDdCanCreateSurface)== DDHAL_DRIVER_NOTHANDLED) 
   {
    // derr(L"DirectDrawImpl[%08x]::__createPrimary Cannot create primary [%08x]", this, rv);
    return DDERR_NOTINITIALIZED;
   }

   if (mDdCanCreateSurface.ddRVal != DD_OK)
   {
     return DDERR_NOTINITIALIZED;
   }

  memset(&This->mPrimaryGlobal, 0, sizeof(DDRAWI_DDRAWSURFACE_GBL));
  This->mPrimaryGlobal.dwGlobalFlags = DDRAWISURFGBL_ISGDISURFACE;
  This->mPrimaryGlobal.lpDD       = &This->mDDrawGlobal;
  This->mPrimaryGlobal.lpDDHandle = &This->mDDrawGlobal;
  This->mPrimaryGlobal.wWidth  = (WORD)This->mpModeInfos[0].dwWidth;
  This->mPrimaryGlobal.wHeight = (WORD)This->mpModeInfos[0].dwHeight;
  This->mPrimaryGlobal.lPitch  = This->mpModeInfos[0].lPitch;

  memset(&This->mPrimaryMore,   0, sizeof(DDRAWI_DDRAWSURFACE_MORE));
  This->mPrimaryMore.dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);

  memset(&This->mPrimaryLocal,  0, sizeof(DDRAWI_DDRAWSURFACE_LCL));
  This->mPrimaryLocal.lpGbl = &This->mPrimaryGlobal;
  This->mPrimaryLocal.lpSurfMore = &This->mPrimaryMore;
  This->mPrimaryLocal.dwProcessId = GetCurrentProcessId();
  This->mPrimaryLocal.dwFlags = DDRAWISURF_PARTOFPRIMARYCHAIN|DDRAWISURF_HASOVERLAYDATA;
  This->mPrimaryLocal.ddsCaps.dwCaps = This->mddsdPrimary.ddsCaps.dwCaps;

  This->mpPrimaryLocals[0] = &This->mPrimaryLocal;

  DDHAL_CREATESURFACEDATA      mDdCreateSurface;
  mDdCreateSurface.lpDD = &This->mDDrawGlobal;
  mDdCreateSurface.CreateSurface = This->mCallbacks.HALDD.CreateSurface;  
  mDdCreateSurface.lpDDSurfaceDesc = &This->mddsdPrimary;//pDDSD;
  mDdCreateSurface.lplpSList = This->mpPrimaryLocals; //cSurfaces;
  mDdCreateSurface.dwSCnt = This->mDDrawGlobal.dsList->dwIntRefCnt ;  //ppSurfaces;

  if (This->mHALInfo.lpDDCallbacks->CreateSurface(&mDdCreateSurface) == DDHAL_DRIVER_NOTHANDLED)
  {
	return DDERR_NOTINITIALIZED;
  }
  

  if (mDdCreateSurface.ddRVal != DD_OK) 
  {   
    return mDdCreateSurface.ddRVal;
  }

  // -- Setup Clipper ---------------------------------------------------------
  memset(&This->mPrimaryClipperGlobal, 0, sizeof(DDRAWI_DDRAWCLIPPER_GBL));
  This->mPrimaryClipperGlobal.dwFlags = DDRAWICLIP_ISINITIALIZED;
  This->mPrimaryClipperGlobal.dwProcessId = GetCurrentProcessId();
  //mPrimaryClipperGlobal.hWnd = (ULONG_PTR)hwnd; 
  This->mPrimaryClipperGlobal.hWnd = (ULONG_PTR)GetDesktopWindow();
  This->mPrimaryClipperGlobal.lpDD = &This->mDDrawGlobal;
  This->mPrimaryClipperGlobal.lpStaticClipList = NULL;

  memset(&This->mPrimaryClipperLocal, 0, sizeof(DDRAWI_DDRAWCLIPPER_LCL));
  This->mPrimaryClipperLocal.lpGbl = &This->mPrimaryClipperGlobal;

  //memset(&mPrimaryClipperInterface, 0, sizeof(DDRAWI_DDRAWCLIPPER_INT));
  //mPrimaryClipperInterface.lpLcl = &mPrimaryClipperLocal;
  //mPrimaryClipperInterface.dwIntRefCnt = 1;
  //mPrimaryClipperInterface.lpLink = null;
  //mPrimaryClipperInterface.lpVtbl = null;

  This->mPrimaryLocal.lpDDClipper = &This->mPrimaryClipperLocal;
  //mPrimaryMore.lpDDIClipper = &mPrimaryClipperInterface;

  //mDdBlt.lpDDDestSurface = mpPrimaryLocals[0];
  
  
  /* create primare surface is down now */

  
  /*
   *
   *
   *
   */

  /* create overlay surface now */
  
  memset(&This->mddsdOverlay, 0, sizeof(DDSURFACEDESC));
  This->mddsdOverlay.dwSize = sizeof(DDSURFACEDESC);
  This->mddsdOverlay.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_BACKBUFFERCOUNT | DDSD_WIDTH | DDSD_HEIGHT;

  This->mddsdOverlay.ddsCaps.dwCaps = DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM | DDSCAPS_COMPLEX | DDSCAPS_FLIP;

  This->mddsdOverlay.dwWidth = 100;  //pels;
  This->mddsdOverlay.dwHeight = 100; // lines;
  This->mddsdOverlay.dwBackBufferCount = 1; //cBuffers;

  This->mddsdOverlay.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
  This->mddsdOverlay.ddpfPixelFormat.dwFlags = DDPF_RGB; 
  This->mddsdOverlay.ddpfPixelFormat.dwRGBBitCount = 32;
  

   //DDHAL_CANCREATESURFACEDATA   mDdCanCreateSurface;
   mDdCanCreateSurface.lpDD = &This->mDDrawGlobal;
   mDdCanCreateSurface.CanCreateSurface = This->mCallbacks.HALDD.CanCreateSurface;
   mDdCanCreateSurface.bIsDifferentPixelFormat = TRUE; //isDifferentPixelFormat;
   mDdCanCreateSurface.lpDDSurfaceDesc = &This->mddsdOverlay; // pDDSD;
   
   
   if (This->mHALInfo.lpDDCallbacks->CanCreateSurface(&mDdCanCreateSurface)== DDHAL_DRIVER_NOTHANDLED) 
   {
     return DDERR_NOTINITIALIZED;
   }

   if (mDdCanCreateSurface.ddRVal != DD_OK)
   {
     return DDERR_NOTINITIALIZED;
   }

 
  memset(&This->mOverlayGlobal, 0, sizeof(DDRAWI_DDRAWSURFACE_GBL));
  This->mOverlayGlobal.dwGlobalFlags = 0;
  This->mOverlayGlobal.lpDD       = &This->mDDrawGlobal;
  This->mOverlayGlobal.lpDDHandle = &This->mDDrawGlobal;
  This->mOverlayGlobal.wWidth  = (WORD)This->mddsdOverlay.dwWidth;
  This->mOverlayGlobal.wHeight = (WORD)This->mddsdOverlay.dwHeight;
  This->mOverlayGlobal.lPitch  = -1;
  This->mOverlayGlobal.ddpfSurface = This->mddsdOverlay.ddpfPixelFormat;

  // setup front- and backbuffer surfaces
  UINT cSurfaces = This->mddsdOverlay.dwBackBufferCount + 1;
  for (i = 0; i < cSurfaces; i++)
  {
     memset(&This->mOverlayMore[i], 0, sizeof(DDRAWI_DDRAWSURFACE_MORE));
     This->mOverlayMore[i].dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);

     memset(&This->mOverlayLocal[i],  0, sizeof(DDRAWI_DDRAWSURFACE_LCL));
     This->mOverlayLocal[i].lpGbl = &This->mOverlayGlobal;
     This->mOverlayLocal[i].lpSurfMore = &This->mOverlayMore[i];
     This->mOverlayLocal[i].dwProcessId = GetCurrentProcessId();
     This->mOverlayLocal[i].dwFlags = (i == 0) ?
      (DDRAWISURF_IMPLICITROOT|DDRAWISURF_FRONTBUFFER):
      (DDRAWISURF_IMPLICITCREATE|DDRAWISURF_BACKBUFFER);

     This->mOverlayLocal[i].dwFlags |= 
      DDRAWISURF_ATTACHED|DDRAWISURF_ATTACHED_FROM|
      DDRAWISURF_HASPIXELFORMAT|
      DDRAWISURF_HASOVERLAYDATA;

    This->mOverlayLocal[i].ddsCaps.dwCaps = This->mddsdOverlay.ddsCaps.dwCaps;
    This->mpOverlayLocals[i] = &This->mOverlayLocal[i];
  }

  for ( i = 0; i < cSurfaces; i++)
  {
    UINT j = (i + 1) % cSurfaces;

	
    /*if (!mHALInfo.lpDDSurfaceCallbacks->AddAttachedSurface(mpOverlayLocals[i], mpOverlayLocals[j])) 
	{
     // derr(L"DirectDrawImpl[%08x]::__setupDevice DdAttachSurface(%d, %d) failed", this, i, j);
      return DD_FALSE;
    }*/

	if (!DdAttachSurface(This->mpOverlayLocals[i], This->mpOverlayLocals[j])) 
	{
     // derr(L"DirectDrawImpl[%08x]::__setupDevice DdAttachSurface(%d, %d) failed", this, i, j);
      return DD_FALSE;
    }
	
  }


  // DDHAL_CREATESURFACEDATA      mDdCreateSurface;
  mDdCreateSurface.lpDD = &This->mDDrawGlobal;
  mDdCreateSurface.CreateSurface = This->mCallbacks.HALDD.CreateSurface;  
  mDdCreateSurface.lpDDSurfaceDesc = &This->mddsdOverlay;//pDDSD;
  mDdCreateSurface.lplpSList = This->mpOverlayLocals; //cSurfaces;
  mDdCreateSurface.dwSCnt = 1 ;  //ppSurfaces;

 if (This->mHALInfo.lpDDCallbacks->CreateSurface(&mDdCreateSurface) == DDHAL_DRIVER_NOTHANDLED)
  {
	return DDERR_NOTINITIALIZED;
  }
  

  if (mDdCreateSurface.ddRVal != DD_OK) 
  {   
    return mDdCreateSurface.ddRVal;
  }

  //mSrcRect.w = mddsdOverlay.dwWidth;
  //mSrcRect.h = mddsdOverlay.dwHeight;
  //__alignBounds();


  DDHAL_UPDATEOVERLAYDATA      mDdUpdateOverlay;
  mDdUpdateOverlay.lpDD = &This->mDDrawGlobal;
  mDdUpdateOverlay.UpdateOverlay = This->mCallbacks.HALDDSurface.UpdateOverlay;
  mDdUpdateOverlay.lpDDDestSurface = This->mpPrimaryLocals[0];
  mDdUpdateOverlay.lpDDSrcSurface = This->mpOverlayLocals[0];//pDDSurface;
  mDdUpdateOverlay.dwFlags = DDOVER_SHOW;

 /* if (flags & DDOVER_DDFX)
    mDdUpdateOverlay.overlayFX = *pFx;
  copyRect(&mDdUpdateOverlay.rDest, pdst);
  copyRect(&mDdUpdateOverlay.rSrc, psrc);
*/
  
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
	return DDERR_NOTINITIALIZED;
  }
  

  if (mDdUpdateOverlay.ddRVal != DD_OK) 
  {   
    return mDdUpdateOverlay.ddRVal;
  }

  return DD_OK;
}

HRESULT Hal_DDrawSurface_Blt(LPDIRECTDRAWSURFACE7 iface, LPRECT rDest,
			  LPDIRECTDRAWSURFACE7 src, LPRECT rSrc, DWORD dwFlags, LPDDBLTFX lpbltfx)
{
	DX_STUB;

	IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;
    IDirectDrawSurfaceImpl* That = (IDirectDrawSurfaceImpl*)src;
 	
	if (!(This->owner->mDDrawGlobal.lpDDCBtmp->HALDDSurface.dwFlags  & DDHAL_SURFCB32_BLT)) 
	{
		return DDERR_NODRIVERSUPPORT;
	}

	DDHAL_BLTDATA BltData;
	BltData.lpDD = &This->owner->mDDrawGlobal;
	BltData.dwFlags = dwFlags;
	BltData.lpDDDestSurface = &This->Local;
    if(rDest) BltData.rDest = *(RECTL*)rDest;
    if(rSrc) BltData.rSrc = *(RECTL*)rSrc;
    if(That) BltData.lpDDSrcSurface = &That->Local;
	if(lpbltfx) BltData.bltFX = *lpbltfx;

	if (This->owner->mDDrawGlobal.lpDDCBtmp->HALDDSurface.Blt(&BltData) != DDHAL_DRIVER_HANDLED)
	{
	   return DDERR_NODRIVERSUPPORT;
	}
	
	return BltData.ddRVal;
}
