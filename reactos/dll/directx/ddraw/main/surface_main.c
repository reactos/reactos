/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/main/surface.c
 * PURPOSE:              IDirectDrawSurface7 Implementation 
 * PROGRAMMER:           Magnus Olsen, Maarten Bosma
 *
 */

#include "rosdraw.h"


/* FIXME adding hal and hel stub 
    DestroySurface; 
    SetClipList;
    AddAttachedSurface;  
    GetFlipStatus;
    SetOverlayPosition;  
    SetPalette;
*/


HRESULT WINAPI Main_DDrawSurface_Initialize (LPDIRECTDRAWSURFACE7 iface, LPDIRECTDRAW pDD, LPDDSURFACEDESC2 pDDSD2)
{    
	return DDERR_ALREADYINITIALIZED;
}

ULONG WINAPI Main_DDrawSurface_AddRef(LPDIRECTDRAWSURFACE7 iface)
{
    IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;

    DX_WINDBG_trace();
    	
    return InterlockedIncrement((PLONG)&This->Owner->mDDrawGlobal.dsList->dwIntRefCnt);
}

ULONG WINAPI Main_DDrawSurface_Release(LPDIRECTDRAWSURFACE7 iface)
{
    IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;
        
    ULONG ref = InterlockedDecrement((PLONG)&This->Owner->mDDrawGlobal.dsList->dwIntRefCnt);
    
    if (ref == 0)
		HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/**** Stubs ****/

HRESULT WINAPI
Main_DDrawSurface_QueryInterface(LPDIRECTDRAWSURFACE7 iface, REFIID riid,
				      LPVOID* ppObj)
{
    DX_WINDBG_trace();

	return E_NOINTERFACE;
}

HRESULT WINAPI Main_DDrawSurface_Blt(LPDIRECTDRAWSURFACE7 iface, LPRECT rdst,
			  LPDIRECTDRAWSURFACE7 src, LPRECT rsrc, DWORD dwFlags, LPDDBLTFX lpbltfx)
{
     DX_WINDBG_trace();

	 IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;
	

	if (This->Owner->mCallbacks.HALDDSurface.dwFlags & DDHAL_SURFCB32_BLT) 
	{
		return Hal_DDrawSurface_Blt( iface,  rdst, src,  rsrc,  dwFlags,  lpbltfx);
	}

	return Hel_DDrawSurface_Blt( iface,  rdst, src,  rsrc,  dwFlags,  lpbltfx);
}


HRESULT WINAPI Main_DDrawSurface_Lock (LPDIRECTDRAWSURFACE7 iface, LPRECT prect,
				LPDDSURFACEDESC2 pDDSD, DWORD flags, HANDLE event)
{   
    DX_WINDBG_trace();

	IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;

    if (event != NULL)
    {
        return DDERR_INVALIDPARAMS; 
    }

	if (This->Owner->mCallbacks.HALDDSurface.dwFlags & DDHAL_SURFCB32_LOCK) 
	{
		return Hal_DDrawSurface_Lock( iface, prect, pDDSD,  flags,  event);
	}

	return Hel_DDrawSurface_Lock( iface, prect, pDDSD,  flags,  event);
}

HRESULT WINAPI Main_DDrawSurface_Unlock (LPDIRECTDRAWSURFACE7 iface, LPRECT pRect)
{
    DX_WINDBG_trace();

	IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;

	if (This->Owner->mCallbacks.HALDDSurface.dwFlags & DDHAL_SURFCB32_LOCK) 
	{
		return Hal_DDrawSurface_Unlock( iface,  pRect);
	}

	return Hel_DDrawSurface_Unlock( iface,  pRect);
}

HRESULT WINAPI
Main_DDrawSurface_AddAttachedSurface(LPDIRECTDRAWSURFACE7 iface,
					  LPDIRECTDRAWSURFACE7 pAttach)
{
   DX_WINDBG_trace();

   IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;        
   
   IDirectDrawSurfaceImpl* That = NULL;
   if (pAttach==NULL)
   {
     return DDERR_INVALIDOBJECT;     
   }
   That = (IDirectDrawSurfaceImpl*)pAttach;
   
   //FIXME Have I put This and That in right order ?? DdAttachSurface(from, to) 
   return DdAttachSurface( That->Owner->mpPrimaryLocals[0],This->Owner->mpPrimaryLocals[0]);
}

/* MSDN: "not currently implemented." */
HRESULT WINAPI
Main_DDrawSurface_AddOverlayDirtyRect(LPDIRECTDRAWSURFACE7 iface,
					   LPRECT pRect)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_BltFast(LPDIRECTDRAWSURFACE7 iface, DWORD dstx,
			      DWORD dsty, LPDIRECTDRAWSURFACE7 src,
			      LPRECT rsrc, DWORD trans)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_Restore(LPDIRECTDRAWSURFACE7 iface)
{
    DX_WINDBG_trace();

    DX_STUB;
}

/* MSDN: "not currently implemented." */
HRESULT WINAPI
Main_DDrawSurface_BltBatch(LPDIRECTDRAWSURFACE7 iface,
				LPDDBLTBATCH pBatch, DWORD dwCount,
				DWORD dwFlags)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_ChangeUniquenessValue(LPDIRECTDRAWSURFACE7 iface)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_DeleteAttachedSurface(LPDIRECTDRAWSURFACE7 iface,
					     DWORD dwFlags,
					     LPDIRECTDRAWSURFACE7 pAttach)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE7 iface,
					    LPVOID context,
					    LPDDENUMSURFACESCALLBACK7 cb)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_EnumOverlayZOrders(LPDIRECTDRAWSURFACE7 iface,
					  DWORD dwFlags, LPVOID context,
					  LPDDENUMSURFACESCALLBACK7 cb)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_Flip(LPDIRECTDRAWSURFACE7 iface,
			    LPDIRECTDRAWSURFACE7 override, DWORD dwFlags)
{
    DX_WINDBG_trace();
    
    IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;

	if (This->Owner->mCallbacks.HALDDSurface.dwFlags & DDHAL_SURFCB32_FLIP) 
	{
		return Hal_DDrawSurface_Flip(iface, override,  dwFlags);
	}

	return Hel_DDrawSurface_Flip(iface, override,  dwFlags);
}

HRESULT WINAPI
Main_DDrawSurface_FreePrivateData(LPDIRECTDRAWSURFACE7 iface, REFGUID tag)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetAttachedSurface(LPDIRECTDRAWSURFACE7 iface,
					  LPDDSCAPS2 pCaps,
					  LPDIRECTDRAWSURFACE7* ppSurface)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetBltStatus(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags)
{
     DX_WINDBG_trace();
    
    IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;

	if (This->Owner->mCallbacks.HALDDSurface.dwFlags & DDHAL_SURFCB32_FLIP) 
	{
		return Hal_DDrawSurface_GetBltStatus( iface,  dwFlags);
	}

	return Hel_DDrawSurface_GetBltStatus( iface,  dwFlags);
}

HRESULT WINAPI
Main_DDrawSurface_GetCaps(LPDIRECTDRAWSURFACE7 iface, LPDDSCAPS2 pCaps)
{
    DX_WINDBG_trace();

    IDirectDrawSurfaceImpl* This;

    if (iface == NULL)
    {
       return DDERR_INVALIDOBJECT;  
    }

    if (pCaps == NULL)
    {
       return DDERR_INVALIDPARAMS;  
    }

    This = (IDirectDrawSurfaceImpl*)iface;        
     
    RtlZeroMemory(pCaps,sizeof(DDSCAPS2));
    pCaps->dwCaps = This->Owner->mddsdPrimary.ddsCaps.dwCaps;
    
    return DD_OK;
}

HRESULT WINAPI
Main_DDrawSurface_GetClipper(LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWCLIPPER* ppClipper)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetColorKey(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags,
				   LPDDCOLORKEY pCKey)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetDC(LPDIRECTDRAWSURFACE7 iface, HDC *phDC)
{
    DX_WINDBG_trace();

    IDirectDrawSurfaceImpl* This;

    if (iface == NULL)
    {
       return DDERR_INVALIDOBJECT;  
    }

    if (phDC == NULL)
    {
       return DDERR_INVALIDPARAMS;  
    }

    This = (IDirectDrawSurfaceImpl*)iface;        

    /*
      FIXME check if the surface exists or not
      for now we aussme the surface exits and create the hDC for it
    */
     
    if ((HDC)This->Owner->mPrimaryLocal.hDC == NULL)
    {
         This->Owner->mPrimaryLocal.hDC = (ULONG_PTR)GetDC((HWND)This->Owner->mDDrawGlobal.lpExclusiveOwner->hWnd);
        *phDC = (HDC)This->Owner->mPrimaryLocal.hDC;
    }
    else
    {
       *phDC =  (HDC)This->Owner->mpPrimaryLocals[0]->hDC;
    }

    return DD_OK;
}

HRESULT WINAPI
Main_DDrawSurface_GetDDInterface(LPDIRECTDRAWSURFACE7 iface, LPVOID* pDD)
{
    DX_WINDBG_trace();

    DX_STUB;
}
HRESULT WINAPI
Main_DDrawSurface_GetFlipStatus(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetLOD(LPDIRECTDRAWSURFACE7 iface, LPDWORD pdwMaxLOD)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetOverlayPosition(LPDIRECTDRAWSURFACE7 iface,
					  LPLONG pX, LPLONG pY)
{
    DX_WINDBG_trace();

    DX_STUB;
}
HRESULT WINAPI
Main_DDrawSurface_GetPalette(LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWPALETTE* ppPalette)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetPixelFormat(LPDIRECTDRAWSURFACE7 iface,
				      LPDDPIXELFORMAT pDDPixelFormat)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetPriority(LPDIRECTDRAWSURFACE7 iface,
				   LPDWORD pdwPriority)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetPrivateData(LPDIRECTDRAWSURFACE7 iface,
				      REFGUID tag, LPVOID pBuffer,
				      LPDWORD pcbBufferSize)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetSurfaceDesc(LPDIRECTDRAWSURFACE7 iface,
				      LPDDSURFACEDESC2 pDDSD)
{
    DWORD dwSize;
    DX_WINDBG_trace();

    IDirectDrawSurfaceImpl *This = (IDirectDrawSurfaceImpl *)iface;

    dwSize =  pDDSD->dwSize;
  
    if ((dwSize != sizeof(DDSURFACEDESC)) &&
    	(dwSize != sizeof(DDSURFACEDESC2))) 
    {	
	   return DDERR_GENERIC;
    }
    
    RtlZeroMemory(pDDSD,dwSize);
    memcpy(pDDSD, &This->Owner->mddsdPrimary, sizeof(DDSURFACEDESC));
    pDDSD->dwSize = dwSize;
   
    return DD_OK;
}

HRESULT WINAPI
Main_DDrawSurface_GetUniquenessValue(LPDIRECTDRAWSURFACE7 iface,
					  LPDWORD pValue)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_IsLost(LPDIRECTDRAWSURFACE7 iface)
{
    DX_WINDBG_trace();

    //DX_STUB;
    DX_STUB_str("not implement return not lost\n");
    return DD_OK;    
}

HRESULT WINAPI
Main_DDrawSurface_PageLock(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_PageUnlock(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_ReleaseDC(LPDIRECTDRAWSURFACE7 iface, HDC hDC)
{
    DX_WINDBG_trace();

     IDirectDrawSurfaceImpl* This;

    if (iface == NULL)
    {
       return DDERR_INVALIDOBJECT;  
    }

    if (hDC == NULL)
    {
       return DDERR_INVALIDPARAMS;  
    }

    This = (IDirectDrawSurfaceImpl*)iface;        
   
    /* FIXME check if surface exits or not */

    if ((HDC)This->Owner->mPrimaryLocal.hDC == NULL)
    {
        return DDERR_GENERIC;         
    }

    ReleaseDC((HWND)This->Owner->mDDrawGlobal.lpExclusiveOwner->hWnd,hDC);

    return DD_OK;
}

HRESULT WINAPI
Main_DDrawSurface_SetClipper (LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWCLIPPER pDDClipper)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_SetColorKey (LPDIRECTDRAWSURFACE7 iface,
				   DWORD dwFlags, LPDDCOLORKEY pCKey)
{
    DX_WINDBG_trace();

    IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;

	if (This->Owner->mCallbacks.HALDDSurface.dwFlags & DDHAL_SURFCB32_SETCOLORKEY) 
	{
		return Hal_DDrawSurface_SetColorKey (iface, dwFlags, pCKey);
	}

	return Hel_DDrawSurface_SetColorKey (iface, dwFlags, pCKey);
}



HRESULT WINAPI
Main_DDrawSurface_SetOverlayPosition (LPDIRECTDRAWSURFACE7 iface,
					  LONG X, LONG Y)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_SetPalette (LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWPALETTE pPalette)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_SetPriority (LPDIRECTDRAWSURFACE7 iface,
				   DWORD dwPriority)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_SetPrivateData (LPDIRECTDRAWSURFACE7 iface,
				      REFGUID tag, LPVOID pData,
				      DWORD cbSize, DWORD dwFlags)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_UpdateOverlay (LPDIRECTDRAWSURFACE7 iface,
				     LPRECT pSrcRect,
				     LPDIRECTDRAWSURFACE7 pDstSurface,
				     LPRECT pDstRect, DWORD dwFlags,
				     LPDDOVERLAYFX pFX)
{
    DX_WINDBG_trace();

    DX_STUB;
}

/* MSDN: "not currently implemented." */
HRESULT WINAPI
Main_DDrawSurface_UpdateOverlayDisplay (LPDIRECTDRAWSURFACE7 iface,
					    DWORD dwFlags)
{
    DX_WINDBG_trace();

    IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;

	if (This->Owner->mCallbacks.HALDDSurface.dwFlags & DDHAL_SURFCB32_UPDATEOVERLAY) 
	{
		return Hal_DDrawSurface_UpdateOverlayDisplay ( iface,  dwFlags);
	}

	return Hel_DDrawSurface_UpdateOverlayDisplay ( iface,  dwFlags);
}

HRESULT WINAPI Main_DDrawSurface_UpdateOverlayZOrder (LPDIRECTDRAWSURFACE7 iface,
					   DWORD dwFlags, LPDIRECTDRAWSURFACE7 pDDSRef)
{
    DX_WINDBG_trace();

    DX_STUB;
}

IDirectDrawSurface7Vtbl DirectDrawSurface7_Vtable =
{
    Main_DDrawSurface_QueryInterface,
    Main_DDrawSurface_AddRef,
    Main_DDrawSurface_Release,
    Main_DDrawSurface_AddAttachedSurface,
    Main_DDrawSurface_AddOverlayDirtyRect,
    Main_DDrawSurface_Blt,
    Main_DDrawSurface_BltBatch,
    Main_DDrawSurface_BltFast,
    Main_DDrawSurface_DeleteAttachedSurface,
    Main_DDrawSurface_EnumAttachedSurfaces,
    Main_DDrawSurface_EnumOverlayZOrders,
    Main_DDrawSurface_Flip,
    Main_DDrawSurface_GetAttachedSurface,
    Main_DDrawSurface_GetBltStatus,
    Main_DDrawSurface_GetCaps,
    Main_DDrawSurface_GetClipper,
    Main_DDrawSurface_GetColorKey,
    Main_DDrawSurface_GetDC,
    Main_DDrawSurface_GetFlipStatus,
    Main_DDrawSurface_GetOverlayPosition,
    Main_DDrawSurface_GetPalette,
    Main_DDrawSurface_GetPixelFormat,
    Main_DDrawSurface_GetSurfaceDesc,
    Main_DDrawSurface_Initialize,
    Main_DDrawSurface_IsLost,
    Main_DDrawSurface_Lock,
    Main_DDrawSurface_ReleaseDC,
    Main_DDrawSurface_Restore,
    Main_DDrawSurface_SetClipper,
    Main_DDrawSurface_SetColorKey,
    Main_DDrawSurface_SetOverlayPosition,
    Main_DDrawSurface_SetPalette,
    Main_DDrawSurface_Unlock,
    Main_DDrawSurface_UpdateOverlay,
    Main_DDrawSurface_UpdateOverlayDisplay,
    Main_DDrawSurface_UpdateOverlayZOrder
};
