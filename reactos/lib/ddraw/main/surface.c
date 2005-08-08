/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/main/surface.c
 * PURPOSE:              DirectDraw Implementation 
 * PROGRAMMER:           Magnus Olsen, Maarten Bosma
 *
 */

#include "rosdraw.h"


HRESULT WINAPI Main_DDrawSurface_Initialize (LPDIRECTDRAWSURFACE7 iface, LPDIRECTDRAW pDD, LPDDSURFACEDESC2 pDDSD)
{
    IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;

	if (This->owner)
		return DDERR_ALREADYINITIALIZED;

	if(sizeof(DDSURFACEDESC2) != pDDSD->dwSize)
		return DDERR_UNSUPPORTED;

	if(!(pDDSD->dwFlags & DDSD_CAPS))
		return DDERR_INVALIDPARAMS;

	This->owner = (IDirectDrawImpl*)pDD;

	// Surface Global Struct
	DDRAWI_DDRAWSURFACE_GBL Global;
	memset(&Global, 0, sizeof(DDRAWI_DDRAWSURFACE_GBL));
	
	if(pDDSD->ddsCaps.dwCaps == DDSCAPS_PRIMARYSURFACE)
		Global.dwGlobalFlags |= DDRAWISURFGBL_ISGDISURFACE;
	
	Global.lpDD = &This->owner->DirectDrawGlobal;	
	Global.wHeight = This->owner->Width;
	Global.wWidth = This->owner->Height;
	Global.dwLinearSize =  Global.wWidth * This->owner->Bpp/8;
	
	// Surface More Struct
	DDRAWI_DDRAWSURFACE_MORE More;
	memset(&More, 0, sizeof(DDRAWI_DDRAWSURFACE_MORE));
	More.dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);

	// Surface Local Struct
	DDRAWI_DDRAWSURFACE_LCL Local;
	memset(&Local, 0, sizeof(DDRAWI_DDRAWSURFACE_LCL));
	Local.lpGbl = &Global;
	Local.lpSurfMore = &More;
	Local.ddsCaps = *(DDSCAPS*)&pDDSD->ddsCaps;

	if(pDDSD->ddsCaps.dwCaps == DDSCAPS_PRIMARYSURFACE)
		Local.dwFlags |= DDRAWISURF_FRONTBUFFER;

	DDRAWI_DDRAWSURFACE_LCL* pLocal = &Local; // for stupid double pointer below

	// The Parameter Struct
	DDHAL_CREATESURFACEDATA CreateData;
	memset(&CreateData, 0, sizeof(DDHAL_CREATESURFACEDATA));
	CreateData.lpDD = &This->owner->DirectDrawGlobal; 
	CreateData.lpDDSurfaceDesc = (DDSURFACEDESC*)pDDSD;
	CreateData.dwSCnt = 1;
	CreateData.lplpSList = &pLocal;

	if(This->owner->DriverCallbacks.DdMain.CreateSurface (&CreateData) != DDHAL_DRIVER_HANDLED)
		return DDERR_INVALIDPARAMS;

	if(CreateData.ddRVal != DD_OK)
		return CreateData.ddRVal;

	OutputDebugString(L"This does not get hit.");

   	return DD_OK;
}

HRESULT WINAPI Main_DDrawSurface_Blt(LPDIRECTDRAWSURFACE7 iface, LPRECT rdst,
			  LPDIRECTDRAWSURFACE7 src, LPRECT rsrc, DWORD dwFlags, LPDDBLTFX lpbltfx)
{
    return DD_OK;
}

HRESULT WINAPI Main_DDrawSurface_Lock (LPDIRECTDRAWSURFACE7 iface, LPRECT prect,
				LPDDSURFACEDESC2 pDDSD, DWORD flags, HANDLE event)
{
    return DD_OK;
}

HRESULT WINAPI Main_DDrawSurface_Unlock (LPDIRECTDRAWSURFACE7 iface, LPRECT pRect)
{
    return DD_OK;
}


ULONG WINAPI Main_DDrawSurface_AddRef(LPDIRECTDRAWSURFACE7 iface)
{
    IDirectDrawSurfaceImpl* This = (IDirectDrawSurfaceImpl*)iface;
    return InterlockedIncrement(&This->ref);
}

ULONG WINAPI Main_DDrawSurface_Release(LPDIRECTDRAWSURFACE7 iface)
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);
    
    if (ref == 0)
		HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/**** Stubs ****/

HRESULT WINAPI
Main_DDrawSurface_QueryInterface(LPDIRECTDRAWSURFACE7 iface, REFIID riid,
				      LPVOID* ppObj)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_AddAttachedSurface(LPDIRECTDRAWSURFACE7 iface,
					  LPDIRECTDRAWSURFACE7 pAttach)
{
    return DDERR_UNSUPPORTED;
}

/* MSDN: "not currently implemented." */
HRESULT WINAPI
Main_DDrawSurface_AddOverlayDirtyRect(LPDIRECTDRAWSURFACE7 iface,
					   LPRECT pRect)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_BltFast(LPDIRECTDRAWSURFACE7 iface, DWORD dstx,
			      DWORD dsty, LPDIRECTDRAWSURFACE7 src,
			      LPRECT rsrc, DWORD trans)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_Restore(LPDIRECTDRAWSURFACE7 iface)
{
    return DDERR_UNSUPPORTED;
}

/* MSDN: "not currently implemented." */
HRESULT WINAPI
Main_DDrawSurface_BltBatch(LPDIRECTDRAWSURFACE7 iface,
				LPDDBLTBATCH pBatch, DWORD dwCount,
				DWORD dwFlags)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_ChangeUniquenessValue(LPDIRECTDRAWSURFACE7 iface)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_DeleteAttachedSurface(LPDIRECTDRAWSURFACE7 iface,
					     DWORD dwFlags,
					     LPDIRECTDRAWSURFACE7 pAttach)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE7 iface,
					    LPVOID context,
					    LPDDENUMSURFACESCALLBACK7 cb)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_EnumOverlayZOrders(LPDIRECTDRAWSURFACE7 iface,
					  DWORD dwFlags, LPVOID context,
					  LPDDENUMSURFACESCALLBACK7 cb)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_Flip(LPDIRECTDRAWSURFACE7 iface,
			    LPDIRECTDRAWSURFACE7 override, DWORD dwFlags)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_FreePrivateData(LPDIRECTDRAWSURFACE7 iface, REFGUID tag)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_GetAttachedSurface(LPDIRECTDRAWSURFACE7 iface,
					  LPDDSCAPS2 pCaps,
					  LPDIRECTDRAWSURFACE7* ppSurface)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_GetBltStatus(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_GetCaps(LPDIRECTDRAWSURFACE7 iface, LPDDSCAPS2 pCaps)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_GetClipper(LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWCLIPPER* ppClipper)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_GetColorKey(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags,
				   LPDDCOLORKEY pCKey)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_GetDC(LPDIRECTDRAWSURFACE7 iface, HDC *phDC)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_GetDDInterface(LPDIRECTDRAWSURFACE7 iface, LPVOID* pDD)
{
    return DDERR_UNSUPPORTED;
}
HRESULT WINAPI
Main_DDrawSurface_GetFlipStatus(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_GetLOD(LPDIRECTDRAWSURFACE7 iface, LPDWORD pdwMaxLOD)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_GetOverlayPosition(LPDIRECTDRAWSURFACE7 iface,
					  LPLONG pX, LPLONG pY)
{
    return DDERR_UNSUPPORTED;
}
HRESULT WINAPI
Main_DDrawSurface_GetPalette(LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWPALETTE* ppPalette)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_GetPixelFormat(LPDIRECTDRAWSURFACE7 iface,
				      LPDDPIXELFORMAT pDDPixelFormat)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_GetPriority(LPDIRECTDRAWSURFACE7 iface,
				   LPDWORD pdwPriority)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_GetPrivateData(LPDIRECTDRAWSURFACE7 iface,
				      REFGUID tag, LPVOID pBuffer,
				      LPDWORD pcbBufferSize)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_GetSurfaceDesc(LPDIRECTDRAWSURFACE7 iface,
				      LPDDSURFACEDESC2 pDDSD)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_GetUniquenessValue(LPDIRECTDRAWSURFACE7 iface,
					  LPDWORD pValue)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_IsLost(LPDIRECTDRAWSURFACE7 iface)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_PageLock(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_PageUnlock(LPDIRECTDRAWSURFACE7 iface, DWORD dwFlags)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_ReleaseDC(LPDIRECTDRAWSURFACE7 iface, HDC hDC)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_SetClipper (LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWCLIPPER pDDClipper)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_SetColorKey (LPDIRECTDRAWSURFACE7 iface,
				   DWORD dwFlags, LPDDCOLORKEY pCKey)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_SetLOD (LPDIRECTDRAWSURFACE7 iface, DWORD dwMaxLOD)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_SetOverlayPosition (LPDIRECTDRAWSURFACE7 iface,
					  LONG X, LONG Y)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_SetPalette (LPDIRECTDRAWSURFACE7 iface,
				  LPDIRECTDRAWPALETTE pPalette)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_SetPriority (LPDIRECTDRAWSURFACE7 iface,
				   DWORD dwPriority)
{
    return DDERR_UNSUPPORTED;
}

/* Be careful when locking this: it is risky to call the object's AddRef
 * or Release holding a lock. */
HRESULT WINAPI
Main_DDrawSurface_SetPrivateData (LPDIRECTDRAWSURFACE7 iface,
				      REFGUID tag, LPVOID pData,
				      DWORD cbSize, DWORD dwFlags)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI
Main_DDrawSurface_UpdateOverlay (LPDIRECTDRAWSURFACE7 iface,
				     LPRECT pSrcRect,
				     LPDIRECTDRAWSURFACE7 pDstSurface,
				     LPRECT pDstRect, DWORD dwFlags,
				     LPDDOVERLAYFX pFX)
{
    return DDERR_UNSUPPORTED;
}

/* MSDN: "not currently implemented." */
HRESULT WINAPI
Main_DDrawSurface_UpdateOverlayDisplay (LPDIRECTDRAWSURFACE7 iface,
					    DWORD dwFlags)
{
    return DDERR_UNSUPPORTED;
}

HRESULT WINAPI Main_DDrawSurface_UpdateOverlayZOrder (LPDIRECTDRAWSURFACE7 iface,
					   DWORD dwFlags, LPDIRECTDRAWSURFACE7 pDDSRef)
{
    return DDERR_NOTAOVERLAYSURFACE;
}

IDirectDrawSurface7Vtbl DDrawSurface_VTable =
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
