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


HRESULT Hal_DirectDraw_CreateSurface (LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD, IDirectDrawSurfaceImpl *ppSurf, IUnknown *pUnkOuter)
{
        //UINT i;
        IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
        IDirectDrawSurfaceImpl* That = ppSurf;        

        DDHAL_CREATESURFACEDATA      mDdCreateSurface;
        DDHAL_CANCREATESURFACEDATA   mDdCanCreateSurface;
	

        mDdCanCreateSurface.lpDD = &This->mDDrawGlobal;
        mDdCanCreateSurface.CanCreateSurface = This->mCallbacks.HALDD.CanCreateSurface;
	
        mDdCreateSurface.lpDD = &This->mDDrawGlobal;
        mDdCreateSurface.CreateSurface = This->mCallbacks.HALDD.CreateSurface;  
          
        if (pDDSD->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
        {   
           
           memcpy(&That->Surf->mddsdPrimary,pDDSD,sizeof(DDSURFACEDESC));
           That->Surf->mddsdPrimary.dwSize      = sizeof(DDSURFACEDESC);          
           mDdCanCreateSurface.bIsDifferentPixelFormat = FALSE; 
           mDdCanCreateSurface.lpDDSurfaceDesc = &That->Surf->mddsdPrimary; 

           if (This->mHALInfo.lpDDCallbacks->CanCreateSurface(&mDdCanCreateSurface)== DDHAL_DRIVER_NOTHANDLED) 
           {         
              return DDERR_NOTINITIALIZED;
           }

           if (mDdCanCreateSurface.ddRVal != DD_OK)
           {
              return DDERR_NOTINITIALIZED;
           }

           memset(&That->Surf->mPrimaryGlobal, 0, sizeof(DDRAWI_DDRAWSURFACE_GBL));
           That->Surf->mPrimaryGlobal.dwGlobalFlags = DDRAWISURFGBL_ISGDISURFACE;
           That->Surf->mPrimaryGlobal.lpDD       = &This->mDDrawGlobal;
           That->Surf->mPrimaryGlobal.lpDDHandle = &This->mDDrawGlobal;
           That->Surf->mPrimaryGlobal.wWidth  = (WORD)This->mpModeInfos[0].dwWidth;
           That->Surf->mPrimaryGlobal.wHeight = (WORD)This->mpModeInfos[0].dwHeight;
           That->Surf->mPrimaryGlobal.lPitch  = This->mpModeInfos[0].lPitch;

           memset(&That->Surf->mPrimaryMore,   0, sizeof(DDRAWI_DDRAWSURFACE_MORE));
           That->Surf->mPrimaryMore.dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);

           memset(&That->Surf->mPrimaryLocal,  0, sizeof(DDRAWI_DDRAWSURFACE_LCL));
           That->Surf->mPrimaryLocal.lpGbl = &That->Surf->mPrimaryGlobal;
           That->Surf->mPrimaryLocal.lpSurfMore = &That->Surf->mPrimaryMore;
           That->Surf->mPrimaryLocal.dwProcessId = GetCurrentProcessId();
	   
           /*
              FIXME Check the flags if we shall create a primaresurface for overlay or something else 
              Examine windows which flags are being set for we assume this is right unsue I think
           */
           //That->Surf->mPrimaryLocal.dwFlags = DDRAWISURF_PARTOFPRIMARYCHAIN|DDRAWISURF_HASOVERLAYDATA;
           That->Surf->mPrimaryLocal.ddsCaps.dwCaps = That->Surf->mddsdPrimary.ddsCaps.dwCaps;
           That->Surf->mpPrimaryLocals[0] = &That->Surf->mPrimaryLocal;

          

           mDdCreateSurface.lpDDSurfaceDesc = &That->Surf->mddsdPrimary;
           mDdCreateSurface.lplpSList = That->Surf->mpPrimaryLocals;
           mDdCreateSurface.dwSCnt = This->mDDrawGlobal.dsList->dwIntRefCnt ; 

            
           if (This->mHALInfo.lpDDCallbacks->CreateSurface(&mDdCreateSurface) == DDHAL_DRIVER_NOTHANDLED)
           {
              return DDERR_NOTINITIALIZED;
           }

           if (mDdCreateSurface.ddRVal != DD_OK) 
           {   
              return mDdCreateSurface.ddRVal;
           }
                     


           return DD_OK;

        }
        else if (pDDSD->ddsCaps.dwCaps & DDSCAPS_OVERLAY)
        {
            DX_STUB_str( "Can not create overlay surface");
           ////memset(&This->mddsdOverlay, 0, sizeof(DDSURFACEDESC));
           //memcpy(&This->mddsdOverlay,pDDSD,sizeof(DDSURFACEDESC));
           //This->mddsdOverlay.dwSize = sizeof(DDSURFACEDESC);
           ////This->mddsdOverlay.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_BACKBUFFERCOUNT | DDSD_WIDTH | DDSD_HEIGHT;

           ////This->mddsdOverlay.ddsCaps.dwCaps = DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM | DDSCAPS_COMPLEX | DDSCAPS_FLIP;

           ////This->mddsdOverlay.dwWidth = 100;  //pels;
           ////This->mddsdOverlay.dwHeight = 100; // lines;
           ////This->mddsdOverlay.dwBackBufferCount = 1; //cBuffers;

           ////This->mddsdOverlay.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
           ////This->mddsdOverlay.ddpfPixelFormat.dwFlags = DDPF_RGB; 
           ////This->mddsdOverlay.ddpfPixelFormat.dwRGBBitCount = 32;
     
           //mDdCanCreateSurface.lpDD = &This->mDDrawGlobal;
           //mDdCanCreateSurface.CanCreateSurface = This->mCallbacks.HALDD.CanCreateSurface;
           //mDdCanCreateSurface.bIsDifferentPixelFormat = TRUE; //isDifferentPixelFormat;
           //mDdCanCreateSurface.lpDDSurfaceDesc = &This->mddsdOverlay; // pDDSD;
   
   
           //if (This->mHALInfo.lpDDCallbacks->CanCreateSurface(&mDdCanCreateSurface)== DDHAL_DRIVER_NOTHANDLED) 
           //{    
           //   return DDERR_NOTINITIALIZED;
           //}

           //if (mDdCanCreateSurface.ddRVal != DD_OK)
           //{
           //   return DDERR_NOTINITIALIZED;
           //}

           //memset(&This->mOverlayGlobal, 0, sizeof(DDRAWI_DDRAWSURFACE_GBL));
           //This->mOverlayGlobal.dwGlobalFlags = 0;
           //This->mOverlayGlobal.lpDD       = &This->mDDrawGlobal;
           //This->mOverlayGlobal.lpDDHandle = &This->mDDrawGlobal;
           //This->mOverlayGlobal.wWidth  = (WORD)This->mddsdOverlay.dwWidth;
           //This->mOverlayGlobal.wHeight = (WORD)This->mddsdOverlay.dwHeight;
           //This->mOverlayGlobal.lPitch  = -1;
           //This->mOverlayGlobal.ddpfSurface = This->mddsdOverlay.ddpfPixelFormat;

           //// setup front- and backbuffer surfaces
           //UINT cSurfaces = This->mddsdOverlay.dwBackBufferCount + 1;
           //for (i = 0; i < cSurfaces; i++)
           //{
           //   memset(&This->mOverlayMore[i], 0, sizeof(DDRAWI_DDRAWSURFACE_MORE));
           //   This->mOverlayMore[i].dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);

           //   memset(&This->mOverlayLocal[i],  0, sizeof(DDRAWI_DDRAWSURFACE_LCL));
           //   This->mOverlayLocal[i].lpGbl = &This->mOverlayGlobal;
           //   This->mOverlayLocal[i].lpSurfMore = &This->mOverlayMore[i];
           //   This->mOverlayLocal[i].dwProcessId = GetCurrentProcessId();
           //   This->mOverlayLocal[i].dwFlags = (i == 0) ?
           //   (DDRAWISURF_IMPLICITROOT|DDRAWISURF_FRONTBUFFER):
           //   (DDRAWISURF_IMPLICITCREATE|DDRAWISURF_BACKBUFFER);

           //   This->mOverlayLocal[i].dwFlags |= DDRAWISURF_ATTACHED|DDRAWISURF_ATTACHED_FROM| DDRAWISURF_HASPIXELFORMAT| DDRAWISURF_HASOVERLAYDATA;

           //   This->mOverlayLocal[i].ddsCaps.dwCaps = This->mddsdOverlay.ddsCaps.dwCaps;
           //   This->mpOverlayLocals[i] = &This->mOverlayLocal[i];
           //}

           //for (i = 0; i < cSurfaces; i++)
           //{
           //   UINT j = (i + 1) % cSurfaces;	
           //   if (!DdAttachSurface(This->mpOverlayLocals[i], This->mpOverlayLocals[j])) 
           //   {
           //      // derr(L"DirectDrawImpl[%08x]::__setupDevice DdAttachSurface(%d, %d) failed", this, i, j);
           //     return DD_FALSE;
           //   }	
           //}
  
           //mDdCreateSurface.lpDD = &This->mDDrawGlobal;
           //mDdCreateSurface.CreateSurface = This->mCallbacks.HALDD.CreateSurface;  
           //mDdCreateSurface.lpDDSurfaceDesc = &This->mddsdOverlay;//pDDSD;
           //mDdCreateSurface.lplpSList = This->mpOverlayLocals; //cSurfaces;
           //mDdCreateSurface.dwSCnt = 1 ;  //ppSurfaces;

           //if (This->mHALInfo.lpDDCallbacks->CreateSurface(&mDdCreateSurface) == DDHAL_DRIVER_NOTHANDLED)
           //{
           //   return DDERR_NOTINITIALIZED;
           //}
  
           //if (mDdCreateSurface.ddRVal != DD_OK) 
           //{   
           //   return mDdCreateSurface.ddRVal;
           //}
  
           //DDHAL_UPDATEOVERLAYDATA      mDdUpdateOverlay;
           //mDdUpdateOverlay.lpDD = &This->mDDrawGlobal;
           //mDdUpdateOverlay.UpdateOverlay = This->mCallbacks.HALDDSurface.UpdateOverlay;
           //mDdUpdateOverlay.lpDDDestSurface = That->Surf->mpPrimaryLocals[0];
           //mDdUpdateOverlay.lpDDSrcSurface = This->mpOverlayLocals[0];//pDDSurface;
           //mDdUpdateOverlay.dwFlags = DDOVER_SHOW;
  
           //mDdUpdateOverlay.rDest.top = 0;
           //mDdUpdateOverlay.rDest.left = 0;
           //mDdUpdateOverlay.rDest.right = 50;
           //mDdUpdateOverlay.rDest.bottom = 50;

           //mDdUpdateOverlay.rSrc.top = 0;
           //mDdUpdateOverlay.rSrc.left = 0;
           //mDdUpdateOverlay.rSrc.right = 50;
           //mDdUpdateOverlay.rSrc.bottom = 50;
	 
           //if (mDdUpdateOverlay.UpdateOverlay(&mDdUpdateOverlay) == DDHAL_DRIVER_NOTHANDLED)
           //{
           //   return DDERR_NOTINITIALIZED;
           //}
  
           //if (mDdUpdateOverlay.ddRVal != DD_OK) 
           //{   
           //   return mDdUpdateOverlay.ddRVal;
           //}

           //return DD_OK;
           return DDERR_INVALIDSURFACETYPE;
 
        }	
        else if (pDDSD->ddsCaps.dwCaps & DDSCAPS_BACKBUFFER)
        {
           DX_STUB_str( "Can not create backbuffer surface");
        }
        else if (pDDSD->ddsCaps.dwCaps & DDSCAPS_TEXTURE)
        {
           DX_STUB_str( "Can not create texture surface");
        }
        else if (pDDSD->ddsCaps.dwCaps & DDSCAPS_ZBUFFER)
        {
           DX_STUB_str( "Can not create zbuffer surface");
        }
        else if (pDDSD->ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN) 
        {
           DX_STUB_str( "Can not create offscreenplain surface");
        }
  
    return DDERR_INVALIDSURFACETYPE;  
}

HRESULT Hal_DDrawSurface_Blt(LPDIRECTDRAWSURFACE7 iface, LPRECT rDest,
			  LPDIRECTDRAWSURFACE7 src, LPRECT rSrc, DWORD dwFlags, LPDDBLTFX lpbltfx)
{
        DDHAL_BLTDATA mDdBlt;
        IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;        

        IDirectDrawSurfaceImpl* That = NULL;
        if (src!=NULL)
        {
            That = (IDirectDrawSurfaceImpl*)src;
        }
 	
        if (This==NULL)
        {
            return DD_FALSE;
        }

        if (!(This->owner->mDDrawGlobal.lpDDCBtmp->HALDDSurface.dwFlags  & DDHAL_SURFCB32_BLT)) 
        {
              return DDERR_NODRIVERSUPPORT;
        }

        mDdBlt.lpDDDestSurface = This->Surf->mpPrimaryLocals[0];

        if (!DdResetVisrgn(This->Surf->mpPrimaryLocals[0], NULL)) 
        {      
              return DDERR_NOGDI;
        }

        memset(&mDdBlt, 0, sizeof(DDHAL_BLTDATA));
        memset(&mDdBlt.bltFX, 0, sizeof(DDBLTFX));

        if (lpbltfx!=NULL)
        {
              memcpy(&mDdBlt.bltFX, lpbltfx, sizeof(DDBLTFX));
        }

        if (rDest!=NULL)
        {
              memcpy(& mDdBlt.rDest, rDest, sizeof(DDBLTFX));
        }

        if (rSrc!=NULL)
        {
              memcpy(& mDdBlt.rDest, rSrc, sizeof(DDBLTFX));
        }
           
        if (src != NULL)
        {
              mDdBlt.lpDDSrcSurface = That->Surf->mpPrimaryLocals[0];
        }

        //mDdBlt.lpDDSrcSurface = NULL; //src->
   
        mDdBlt.lpDD = &This->owner->mDDrawGlobal;
        mDdBlt.Blt = This->owner->mCallbacks.HALDDSurface.Blt; 
        mDdBlt.lpDDDestSurface = This->Surf->mpPrimaryLocals[0];

        mDdBlt.dwFlags = dwFlags;
      
       // This->Surf->mpPrimaryLocals[0]->hDC = This->owner->mDDrawGlobal.lpExclusiveOwner->hDC; 
    
        // FIXME dectect if it clipped or not 
        DX_STUB_str( "Can not create offscreenplain surface");
        mDdBlt.IsClipped = FALSE;    
   
        if (mDdBlt.Blt(&mDdBlt) != DDHAL_DRIVER_HANDLED)
        {
              return DDHAL_DRIVER_HANDLED;
        }


        if (mDdBlt.ddRVal!=DD_OK) 
        {
              return mDdBlt.ddRVal;
        }

        return DD_OK;
}

HRESULT Hal_DDrawSurface_Lock(LPDIRECTDRAWSURFACE7 iface, LPRECT prect, LPDDSURFACEDESC2 
                              pDDSD, DWORD flags, HANDLE event)
{
    DX_STUB;
}
