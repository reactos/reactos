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



HRESULT WINAPI Main_DDrawSurface_Initialize (LPDIRECTDRAWSURFACE7 iface, LPDIRECTDRAW pDD, LPDDSURFACEDESC2 pDDSD2)
{
	return DDERR_ALREADYINITIALIZED;
}

ULONG WINAPI Main_DDrawSurface_AddRef(LPDIRECTDRAWSURFACE7 iface)
{
    IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;
	
    return InterlockedIncrement((PLONG)&This->owner->mDDrawGlobal.dsList->dwIntRefCnt);
}

ULONG WINAPI Main_DDrawSurface_Release(LPDIRECTDRAWSURFACE7 iface)
{
    IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;
    ULONG ref = InterlockedDecrement((PLONG)&This->owner->mDDrawGlobal.dsList->dwIntRefCnt);
    
    if (ref == 0)
		HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/**** Stubs ****/

HRESULT WINAPI
Main_DDrawSurface_QueryInterface(LPDIRECTDRAWSURFACE7 iface, REFIID riid,
				      LPVOID* ppObj)
{
	return E_NOINTERFACE;
}

HRESULT WINAPI Main_DDrawSurface_Blt(LPDIRECTDRAWSURFACE7 iface, LPRECT rdst,
			  LPDIRECTDRAWSURFACE7 src, LPRECT rsrc, DWORD dwFlags, LPDDBLTFX lpbltfx)
{
	 IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;
	

	if (This->owner->mDDrawGlobal.lpDDCBtmp->HALDD.dwFlags & DDHAL_SURFCB32_BLT) 
	{
		return Hal_DDrawSurface_Blt( iface,  rdst, src,  rsrc,  dwFlags,  lpbltfx);
	}

	return Hel_DDrawSurface_Blt( iface,  rdst, src,  rsrc,  dwFlags,  lpbltfx);
}


HRESULT WINAPI Main_DDrawSurface_Lock (LPDIRECTDRAWSURFACE7 iface, LPRECT prect,
				LPDDSURFACEDESC2 pDDSD, DWORD flags, HANDLE event)
{ /*   
	IDirectDrawSurfaceImpl* That = (IDirectDrawSurfaceImpl*)iface;

	if (This->mDDrawGlobal.lpDDCBtmp->HALDD.dwFlags & DDHAL_CB32_CREATESURFACE) 
	{
		return Hal_DDrawSurface_Lock( iiface, LPRECT prect, pDDSD,  flags,  event);
	}

	return Hel_DDrawSurface_Lock( iiface, LPRECT prect, pDDSD,  flags,  event);*/
    DX_STUB;
}

HRESULT WINAPI Main_DDrawSurface_Unlock (LPDIRECTDRAWSURFACE7 iface, LPRECT pRect)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_AddAttachedSurface(LPDIRECTDRAWSURFACE7 iface,
					  LPDIRECTDRAWSURFACE7 pAttach)
{
   IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;        

   
   IDirectDrawSurfaceImpl* That = NULL;
   if (pAttach==NULL)
   {
     return DDERR_INVALIDOBJECT;     
   }
   That = (IDirectDrawSurfaceImpl*)pAttach;
   
   //FIXME Have I put This and That in right order ?? DdAttachSurface(from, to) 
   return DdAttachSurface( That->Surf->mpPrimaryLocals[0],This->Surf->mpPrimaryLocals[0]);
}

/* MSDN: "not currently implemented." */
HRESULT WINAPI
Main_DDrawSurface_AddOverlayDirtyRect(LPDIRECTDRAWSURFACE7 iface,
					   LPRECT pRect)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_BltFast(LPDIRECTDRAWSURFACE7 iface, DWORD dstx,
			      DWORD dsty, LPDIRECTDRAWSURFACE7 src,
			      LPRECT rsrc, DWORD trans)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_Restore(LPDIRECTDRAWSURFACE7 iface)
{
    DX_STUB;
}

/* MSDN: "not currently implemented." */
HRESULT WINAPI
Main_DDrawSurface_BltBatch(LPDIRECTDRAWSURFACE7 iface,
				LPDDBLTBATCH pBatch, DWORD dwCount,
				DWORD dwFlags)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_ChangeUniquenessValue(LPDIRECTDRAWSURFACE7 iface)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_DeleteAttachedSurface(LPDIRECTDRAWSURFACE7 iface,
					     DWORD dwFlags,
					     LPDIRECTDRAWSURFACE7 pAttach)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE7 iface,
					    LPVOID context,
					    LPDDENUMSURFACESCALLBACK7 cb)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_EnumOverlayZOrders(LPDIRECTDRAWSURFACE7 iface,
					  DWORD dwFlags, LPVOID context,
					  LPDDENUMSURFACESCALLBACK7 cb)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_Flip(LPDIRECTDRAWSURFACE7 iface,
			    LPDIRECTDRAWSURFACE7 override, DWORD dwFlags)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_FreePrivateData(LPDIRECTDRAWSURFACE7 iface, REFGUID tag)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetAttachedSurface(LPDIRECTDRAWSURFACE7 iface,
					  LPDDSCAPS2 pCaps,
					  LPDIRECTDRAWSURFACE7* ppSurface)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetBltStatus(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetCaps(LPDIRECTDRAWSURFACE7 iface, LPDDSCAPS2 pCaps)
{
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
    pCaps->dwCaps = This->Surf->mddsdPrimary.ddsCaps.dwCaps;
    
    return DD_OK;
}

HRESULT WINAPI
Main_DDrawSurface_GetClipper(LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWCLIPPER* ppClipper)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetColorKey(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags,
				   LPDDCOLORKEY pCKey)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetDC(LPDIRECTDRAWSURFACE7 iface, HDC *phDC)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetDDInterface(LPDIRECTDRAWSURFACE7 iface, LPVOID* pDD)
{
    DX_STUB;
}
HRESULT WINAPI
Main_DDrawSurface_GetFlipStatus(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetLOD(LPDIRECTDRAWSURFACE7 iface, LPDWORD pdwMaxLOD)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetOverlayPosition(LPDIRECTDRAWSURFACE7 iface,
					  LPLONG pX, LPLONG pY)
{
    DX_STUB;
}
HRESULT WINAPI
Main_DDrawSurface_GetPalette(LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWPALETTE* ppPalette)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetPixelFormat(LPDIRECTDRAWSURFACE7 iface,
				      LPDDPIXELFORMAT pDDPixelFormat)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetPriority(LPDIRECTDRAWSURFACE7 iface,
				   LPDWORD pdwPriority)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetPrivateData(LPDIRECTDRAWSURFACE7 iface,
				      REFGUID tag, LPVOID pBuffer,
				      LPDWORD pcbBufferSize)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetSurfaceDesc(LPDIRECTDRAWSURFACE7 iface,
				      LPDDSURFACEDESC2 pDDSD)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetUniquenessValue(LPDIRECTDRAWSURFACE7 iface,
					  LPDWORD pValue)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_IsLost(LPDIRECTDRAWSURFACE7 iface)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_PageLock(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_PageUnlock(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_ReleaseDC(LPDIRECTDRAWSURFACE7 iface, HDC hDC)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_SetClipper (LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWCLIPPER pDDClipper)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_SetColorKey (LPDIRECTDRAWSURFACE7 iface,
				   DWORD dwFlags, LPDDCOLORKEY pCKey)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_SetLOD (LPDIRECTDRAWSURFACE7 iface, DWORD dwMaxLOD)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_SetOverlayPosition (LPDIRECTDRAWSURFACE7 iface,
					  LONG X, LONG Y)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_SetPalette (LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWPALETTE pPalette)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_SetPriority (LPDIRECTDRAWSURFACE7 iface,
				   DWORD dwPriority)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_SetPrivateData (LPDIRECTDRAWSURFACE7 iface,
				      REFGUID tag, LPVOID pData,
				      DWORD cbSize, DWORD dwFlags)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_UpdateOverlay (LPDIRECTDRAWSURFACE7 iface,
				     LPRECT pSrcRect,
				     LPDIRECTDRAWSURFACE7 pDstSurface,
				     LPRECT pDstRect, DWORD dwFlags,
				     LPDDOVERLAYFX pFX)
{
    DX_STUB;
}

/* MSDN: "not currently implemented." */
HRESULT WINAPI
Main_DDrawSurface_UpdateOverlayDisplay (LPDIRECTDRAWSURFACE7 iface,
					    DWORD dwFlags)
{
    DX_STUB;
}

HRESULT WINAPI Main_DDrawSurface_UpdateOverlayZOrder (LPDIRECTDRAWSURFACE7 iface,
					   DWORD dwFlags, LPDIRECTDRAWSURFACE7 pDDSRef)
{
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
