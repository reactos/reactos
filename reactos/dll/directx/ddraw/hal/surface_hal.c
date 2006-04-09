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
       // UINT i;
        IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
        IDirectDrawSurfaceImpl* That = ppSurf;        

        DDHAL_CREATESURFACEDATA      mDdCreateSurface;
        DDHAL_CANCREATESURFACEDATA   mDdCanCreateSurface;
	

        mDdCanCreateSurface.lpDD = &This->mDDrawGlobal;
        
        if (This->mDDrawGlobal.lpDDCBtmp->HALDD.dwFlags & DDHAL_CB32_CANCREATESURFACE) 
        {
           mDdCanCreateSurface.CanCreateSurface = This->mCallbacks.HALDD.CanCreateSurface;  
        }
        else
        {
           mDdCanCreateSurface.CanCreateSurface = This->mCallbacks.HELDD.CanCreateSurface;
        }
                	
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
                     
            
           /* FIXME fill in this if they are avali
              DDSD_BACKBUFFERCOUNT 
              DDSD_CKDESTBLT 
              DDSD_CKDESTOVERLAY 
              DDSD_CKSRCBLT 
              DDSD_CKSRCOVERLAY 
              DDSD_LINEARSIZE 
              DDSD_LPSURFACE 
              DDSD_MIPMAPCOUNT 
              DDSD_ZBUFFERBITDEPTH
           */

           That->Surf->mddsdPrimary.dwFlags = DDSD_CAPS + DDSD_PIXELFORMAT;
           RtlCopyMemory(&That->Surf->mddsdPrimary.ddpfPixelFormat,&This->mDDrawGlobal.vmiData.ddpfDisplay,sizeof(DDPIXELFORMAT));
           RtlCopyMemory(&That->Surf->mddsdPrimary.ddsCaps,&This->mDDrawGlobal.ddCaps,sizeof(DDCORECAPS));
    
           //RtlCopyMemory(&pDDSD->ddckCKDestOverlay,&This->mDDrawGlobal.ddckCKDestOverlay,sizeof(DDCOLORKEY));
           //RtlCopyMemory(&pDDSD->ddckCKSrcOverlay,&This->mDDrawGlobal.ddckCKSrcOverlay,sizeof(DDCOLORKEY));

           if (This->mDDrawGlobal.vmiData.dwDisplayHeight != 0)
           {
              That->Surf->mddsdPrimary.dwFlags += DDSD_HEIGHT ;
              That->Surf->mddsdPrimary.dwHeight  = This->mDDrawGlobal.vmiData.dwDisplayHeight;
           }

           if (This->mDDrawGlobal.vmiData.dwDisplayWidth != 0)
           {
              That->Surf->mddsdPrimary.dwFlags += DDSD_WIDTH ;
              That->Surf->mddsdPrimary.dwWidth = This->mDDrawGlobal.vmiData.dwDisplayWidth; 
           }

           if (This->mDDrawGlobal.vmiData.lDisplayPitch != 0)
           {
              That->Surf->mddsdPrimary.dwFlags += DDSD_PITCH ;           
              That->Surf->mddsdPrimary.lPitch  = This->mDDrawGlobal.vmiData.lDisplayPitch;
           }

           if ( This->mDDrawGlobal.dwMonitorFrequency != 0)
           {
              That->Surf->mddsdPrimary.dwFlags += DDSD_REFRESHRATE ;           
              That->Surf->mddsdPrimary.dwRefreshRate = This->mDDrawGlobal.dwMonitorFrequency;
           }
          
           if (This->mDDrawGlobal.vmiData.ddpfDisplay.dwAlphaBitDepth != 0)
           {
             That->Surf->mddsdPrimary.dwFlags += DDSD_ALPHABITDEPTH ;
             That->Surf->mddsdPrimary.dwAlphaBitDepth = This->mDDrawGlobal.vmiData.ddpfDisplay.dwAlphaBitDepth;
           }

           return DD_OK;

        }
        else if (pDDSD->ddsCaps.dwCaps & DDSCAPS_OVERLAY)
        {
           // DX_STUB_str( "Can not create overlay surface");
           // memset(&That->Surf->mddsdOverlay, 0, sizeof(DDSURFACEDESC));
           // memcpy(&That->Surf->mddsdOverlay,pDDSD,sizeof(DDSURFACEDESC));
           // That->Surf->mddsdOverlay.dwSize = sizeof(DDSURFACEDESC);
           //That->Surf->mddsdOverlay.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_BACKBUFFERCOUNT | DDSD_WIDTH | DDSD_HEIGHT;

           //That->Surf->mddsdOverlay.ddsCaps.dwCaps = DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM | DDSCAPS_COMPLEX | DDSCAPS_FLIP;

           //That->Surf->mddsdOverlay.dwWidth = 100;  //pels;
           //That->Surf->mddsdOverlay.dwHeight = 100; // lines;
           //That->Surf->mddsdOverlay.dwBackBufferCount = 1; //cBuffers;

           //That->Surf->mddsdOverlay.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
           //That->Surf->mddsdOverlay.ddpfPixelFormat.dwFlags = DDPF_RGB; 
           //That->Surf->mddsdOverlay.ddpfPixelFormat.dwRGBBitCount = 32;
     
           //mDdCanCreateSurface.lpDD = &This->mDDrawGlobal;
           //mDdCanCreateSurface.CanCreateSurface = This->mCallbacks.HALDD.CanCreateSurface;
           //mDdCanCreateSurface.bIsDifferentPixelFormat = TRUE; //isDifferentPixelFormat;
           //mDdCanCreateSurface.lpDDSurfaceDesc = &That->Surf->mddsdOverlay; // pDDSD;
   
   
           //if (This->mHALInfo.lpDDCallbacks->CanCreateSurface(&mDdCanCreateSurface)== DDHAL_DRIVER_NOTHANDLED) 
           //{    
           //   return DDERR_NOTINITIALIZED;
           //}

           //if (mDdCanCreateSurface.ddRVal != DD_OK)
           //{
           //   return DDERR_NOTINITIALIZED;
           //}

           //memset(&That->Surf->mOverlayGlobal, 0, sizeof(DDRAWI_DDRAWSURFACE_GBL));
           //That->Surf->mOverlayGlobal.dwGlobalFlags = 0;
           //That->Surf->mOverlayGlobal.lpDD       = &This->mDDrawGlobal;
           //That->Surf->mOverlayGlobal.lpDDHandle = &This->mDDrawGlobal;
           //That->Surf->mOverlayGlobal.wWidth  = (WORD)That->Surf->mddsdOverlay.dwWidth;
           //That->Surf->mOverlayGlobal.wHeight = (WORD)That->Surf->mddsdOverlay.dwHeight;
           //That->Surf->mOverlayGlobal.lPitch  = -1;
           //That->Surf->mOverlayGlobal.ddpfSurface = That->Surf->mddsdOverlay.ddpfPixelFormat;

           //// setup front- and backbuffer surfaces
           //UINT cSurfaces = That->Surf->mddsdOverlay.dwBackBufferCount + 1;
           //for (i = 0; i < cSurfaces; i++)
           //{
           //   memset(&That->Surf->mOverlayMore[i], 0, sizeof(DDRAWI_DDRAWSURFACE_MORE));
           //   That->Surf->mOverlayMore[i].dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);

           //   memset(&That->Surf->mOverlayLocal[i],  0, sizeof(DDRAWI_DDRAWSURFACE_LCL));
           //   That->Surf->mOverlayLocal[i].lpGbl = &That->Surf->mOverlayGlobal;
           //   That->Surf->mOverlayLocal[i].lpSurfMore = &That->Surf->mOverlayMore[i];
           //   That->Surf->mOverlayLocal[i].dwProcessId = GetCurrentProcessId();
           //   That->Surf->mOverlayLocal[i].dwFlags = (i == 0) ?
           //   (DDRAWISURF_IMPLICITROOT|DDRAWISURF_FRONTBUFFER):
           //   (DDRAWISURF_IMPLICITCREATE|DDRAWISURF_BACKBUFFER);

           //   That->Surf->mOverlayLocal[i].dwFlags |= DDRAWISURF_ATTACHED|DDRAWISURF_ATTACHED_FROM| DDRAWISURF_HASPIXELFORMAT| DDRAWISURF_HASOVERLAYDATA;

           //   That->Surf->mOverlayLocal[i].ddsCaps.dwCaps = That->Surf->mddsdOverlay.ddsCaps.dwCaps;
           //   That->Surf->mpOverlayLocals[i] = &That->Surf->mOverlayLocal[i];
           //}

           //for (i = 0; i < cSurfaces; i++)
           //{
           //   UINT j = (i + 1) % cSurfaces;	
           //   if (!DdAttachSurface(That->Surf->mpOverlayLocals[i], That->Surf->mpOverlayLocals[j])) 
           //   {
           //      // derr(L"DirectDrawImpl[%08x]::__setupDevice DdAttachSurface(%d, %d) failed", this, i, j);
           //     return DD_FALSE;
           //   }	
           //}
  
           //mDdCreateSurface.lpDD = &This->mDDrawGlobal;
           //mDdCreateSurface.CreateSurface = This->mCallbacks.HALDD.CreateSurface;  
           //mDdCreateSurface.lpDDSurfaceDesc = &That->Surf->mddsdOverlay;//pDDSD;
           //mDdCreateSurface.lplpSList = That->Surf->mpOverlayLocals; //cSurfaces;
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
           //mDdUpdateOverlay.lpDDSrcSurface = That->Surf->mpOverlayLocals[0];//pDDSurface;
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

           return DD_OK;
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

   IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;
   
  
   DDHAL_LOCKDATA Lock;
   
   if (prect!=NULL)
   {
      Lock.bHasRect = TRUE;
      memcpy(&Lock.rArea,prect,sizeof(RECTL));
   }
   else
   {
      Lock.bHasRect = FALSE;
   }

   Lock.ddRVal = DDERR_NOTPALETTIZED;
   Lock.Lock = This->owner->mCallbacks.HALDDSurface.Lock;
   Lock.dwFlags = flags;
   Lock.lpDDSurface = This->Surf->mpPrimaryLocals[0];
   Lock.lpDD = &This->owner->mDDrawGlobal;   
   Lock.lpSurfData = NULL;

   // FIXME some how lock goes wrong; 
   return DD_FALSE;
   if (This->owner->mCallbacks.HALDDSurface.Lock(&Lock)!= DDHAL_DRIVER_HANDLED)
   {
      return Lock.ddRVal;
   }

   RtlZeroMemory(pDDSD,sizeof(DDSURFACEDESC2));
   memcpy(pDDSD,&This->Surf->mddsdPrimary,sizeof(DDSURFACEDESC));
   pDDSD->dwSize = sizeof(DDSURFACEDESC2);

   pDDSD->lpSurface = Lock.lpSurfData;


    // FIXME some things is wrong it does not show the data on screen ??
   
   return DD_OK;   
}
HRESULT Hal_DDrawSurface_Unlock(LPDIRECTDRAWSURFACE7 iface, LPRECT pRect)
{
    DX_STUB;
}

HRESULT Hal_DDrawSurface_Flip(LPDIRECTDRAWSURFACE7 iface, LPDIRECTDRAWSURFACE7 override, DWORD dwFlags)
{
    DX_STUB;
}

HRESULT Hal_DDrawSurface_SetColorKey (LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags, LPDDCOLORKEY pCKey)
{
    DX_STUB;
}

HRESULT Hal_DDrawSurface_GetBltStatus(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags)
{
    DX_STUB;
}

HRESULT Hal_DDrawSurface_UpdateOverlayDisplay (LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags)
{
    DX_STUB;
}


