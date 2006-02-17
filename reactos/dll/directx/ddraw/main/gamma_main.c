/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/main/gamma.c
 * PURPOSE:              IDirectDrawGamma Implementation 
 * PROGRAMMER:           Maarten Bosma
 *
 */

#include "rosdraw.h"

ULONG WINAPI
Main_DirectDrawGammaControl_AddRef(LPDIRECTDRAWGAMMACONTROL iface)
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
    ULONG ref = InterlockedIncrement((PLONG)&This->DirectDrawGlobal.dwRefCnt);

   	return ref;
}

ULONG WINAPI
Main_DirectDrawGammaControl_Release(LPDIRECTDRAWGAMMACONTROL iface)
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
    ULONG ref = InterlockedDecrement((PLONG)&This->DirectDrawGlobal.dwRefCnt);
    
    if (ref == 0)
		HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

HRESULT WINAPI
Main_DirectDrawGammaControl_QueryInterface(LPDIRECTDRAWGAMMACONTROL iface, REFIID riid,
				      LPVOID *ppObj)
{
	return E_NOINTERFACE;
}

HRESULT WINAPI
Main_DirectDrawGammaControl_GetGammaRamp(LPDIRECTDRAWGAMMACONTROL iface, DWORD dwFlags, LPDDGAMMARAMP lpGammaRamp)
{
   	DX_STUB;
}

HRESULT WINAPI
Main_DirectDrawGammaControl_SetGammaRamp(LPDIRECTDRAWGAMMACONTROL iface, DWORD dwFlags, LPDDGAMMARAMP lpGammaRamp)
{
   	DX_STUB;
}

IDirectDrawGammaControlVtbl DirectDrawGammaControl_Vtable =
{
    Main_DirectDrawGammaControl_QueryInterface,
    Main_DirectDrawGammaControl_AddRef,
    Main_DirectDrawGammaControl_Release,
    Main_DirectDrawGammaControl_GetGammaRamp,
    Main_DirectDrawGammaControl_SetGammaRamp
};
