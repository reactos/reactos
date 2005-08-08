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
    This->DriverCallbacks.DdMain.dwSize = sizeof(DDHAL_DDCALLBACKS);
	This->DriverCallbacks.DdSurface.dwSize = sizeof(DDHAL_DDSURFACECALLBACKS);
	This->DriverCallbacks.DdPalette.dwSize = sizeof(DDHAL_DDPALETTECALLBACKS);
	This->DriverCallbacks.D3dMain.dwSize = sizeof(D3DHAL_CALLBACKS);
	This->DriverCallbacks.D3dDriverData.dwSize = sizeof(D3DHAL_GLOBALDRIVERDATA);
	This->DriverCallbacks.D3dBufferCallbacks.dwSize = sizeof(DDHAL_DDEXEBUFCALLBACKS);
 
	if(!DdQueryDirectDrawObject (
		&This->DirectDrawGlobal, 
		&This->HalInfo, 
		&This->DriverCallbacks.DdMain,
		&This->DriverCallbacks.DdSurface,
		&This->DriverCallbacks.DdPalette,
		&This->DriverCallbacks.D3dMain,
		&This->DriverCallbacks.D3dDriverData,
		&This->DriverCallbacks.D3dBufferCallbacks, 
		NULL, 
		NULL, 
		NULL ))
		return DDERR_INVALIDPARAMS;

	// ToDo: Second DdQueryDirectDrawObject without the three NULLs	

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
}




