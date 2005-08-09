/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/hal/ddraw.c
 * PURPOSE:              DirectDraw HAL Implementation 
 * PROGRAMMER:           Magnus Olsen, Maarten Bosma
 *
 */

#include "rosdraw.h"


HRESULT Hal_DirectDraw_Initialize (LPDIRECTDRAW7 iface)
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
 
	if(!DdCreateDirectDrawObject (&This->DirectDrawGlobal, This->hdc))
		return DDERR_INVALIDPARAMS;
 
	This->HalInfo.dwSize = sizeof(DDHALINFO);
	This->D3dDriverData.dwSize = sizeof(D3DHAL_GLOBALDRIVERDATA);
    This->DriverCallbacks.DdMain.dwSize = sizeof(DDHAL_DDCALLBACKS);
	This->DriverCallbacks.DdSurface.dwSize = sizeof(DDHAL_DDSURFACECALLBACKS);
	This->DriverCallbacks.DdPalette.dwSize = sizeof(DDHAL_DDPALETTECALLBACKS);
	This->DriverCallbacks.D3dMain.dwSize = sizeof(D3DHAL_CALLBACKS);
	This->DriverCallbacks.D3dBufferCallbacks.dwSize = sizeof(DDHAL_DDEXEBUFCALLBACKS);
 
	if(!DdQueryDirectDrawObject (
		&This->DirectDrawGlobal, 
		&This->HalInfo, 
		&This->DriverCallbacks.DdMain,
		&This->DriverCallbacks.DdSurface,
		&This->DriverCallbacks.DdPalette,
		&This->DriverCallbacks.D3dMain,
		&This->D3dDriverData,
		&This->DriverCallbacks.D3dBufferCallbacks, 
		NULL, 
		NULL, 
		NULL ))
		return DDERR_INVALIDPARAMS;

	This->pD3dTextureFormats = HeapAlloc(GetProcessHeap(), 0, sizeof(DDSURFACEDESC) * This->D3dDriverData.dwNumTextureFormats);
	This->pdwFourCC = HeapAlloc(GetProcessHeap(), 0, sizeof(DWORD) * This->HalInfo.ddCaps.dwNumFourCCCodes);
	This->pvmList = HeapAlloc(GetProcessHeap(), 0, sizeof(VIDMEM) * This->HalInfo.vmiData.dwNumHeaps);

	if(!DdQueryDirectDrawObject (
		&This->DirectDrawGlobal, 
		&This->HalInfo, 
		&This->DriverCallbacks.DdMain,
		&This->DriverCallbacks.DdSurface,
		&This->DriverCallbacks.DdPalette,
		&This->DriverCallbacks.D3dMain,
		&This->D3dDriverData,
		&This->DriverCallbacks.D3dBufferCallbacks, 
		This->pD3dTextureFormats, 
		This->pdwFourCC, 
		This->pvmList ))
		return DDERR_INVALIDPARAMS;

	return DD_OK;
}

HRESULT Hal_DirectDraw_SetCooperativeLevel (LPDIRECTDRAW7 iface)
{
   	return DD_OK;
}

VOID Hal_DirectDraw_Release (LPDIRECTDRAW7 iface) 
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

	DdDeleteDirectDrawObject (&This->DirectDrawGlobal);
			
	if(This->pD3dTextureFormats)
		HeapFree(GetProcessHeap(), 0, This->pD3dTextureFormats);
	if(This->pdwFourCC)
		HeapFree(GetProcessHeap(), 0, This->pdwFourCC);
	if(This->pvmList)
		HeapFree(GetProcessHeap(), 0, This->pvmList);
}
