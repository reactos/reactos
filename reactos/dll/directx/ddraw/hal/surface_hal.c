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

        mDdBlt.lpDD = &This->owner->mDDrawGlobal;
        mDdBlt.Blt = This->owner->mCallbacks.HALDDSurface.Blt; 
        mDdBlt.lpDDDestSurface = This->Surf->mpPrimaryLocals[0];

        mDdBlt.dwFlags = dwFlags;
             
    
        // FIXME dectect if it clipped or not 
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
   Lock.lpDDSurface = &This->Surf->mPrimaryLocal;
   Lock.lpDD = &This->owner->mDDrawGlobal;   
   Lock.lpSurfData = NULL;
     
   if (!DdResetVisrgn(&This->Surf->mPrimaryLocal, NULL)) 
   {
      OutputDebugStringA("Here DdResetVisrgn lock");
      return DDERR_UNSUPPORTED;
   }
   
   if (Lock.Lock(&Lock)!= DDHAL_DRIVER_HANDLED)
   {
      OutputDebugStringA("Here DDHAL_DRIVER_HANDLED lock");
      return DDERR_UNSUPPORTED;
   }
   
   if (Lock.ddRVal!= DD_OK)
   {      
      OutputDebugStringA("Here ddRVal lock");
      return Lock.ddRVal;
   }

   // FIXME ??? is this right ?? 
   RtlZeroMemory(pDDSD,sizeof(DDSURFACEDESC2));
   memcpy(pDDSD,&This->Surf->mddsdPrimary,sizeof(DDSURFACEDESC));
   pDDSD->dwSize = sizeof(DDSURFACEDESC2);
       
   pDDSD->lpSurface = (LPVOID)  Lock.lpSurfData;
      
   return DD_OK;   
}
HRESULT Hal_DDrawSurface_Unlock(LPDIRECTDRAWSURFACE7 iface, LPRECT pRect)
{     
   IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;
      
   DDHAL_UNLOCKDATA unLock;   
   unLock.ddRVal = DDERR_NOTPALETTIZED;
   unLock.lpDD = &This->owner->mDDrawGlobal;   
   unLock.lpDDSurface =  &This->Surf->mPrimaryLocal;
   unLock.Unlock = This->owner->mCallbacks.HALDDSurface.Unlock;



   if (!DdResetVisrgn( unLock.lpDDSurface, NULL)) 
   {   
    return DDERR_UNSUPPORTED;
   }

   if (unLock.Unlock(&unLock)!= DDHAL_DRIVER_HANDLED)
   {
      return DDERR_UNSUPPORTED;
   }

   if (unLock.ddRVal!= DD_OK)
   {     
      return unLock.ddRVal;
   } 
   
    return DD_OK;
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


