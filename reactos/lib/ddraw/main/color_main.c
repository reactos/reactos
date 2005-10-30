/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/main/color.c
 * PURPOSE:              IDirectDrawColorControl Implementation 
 * PROGRAMMER:           Maarten Bosma
 *
 */

#include "rosdraw.h"

ULONG WINAPI
Main_DirectDrawColorControl_AddRef(LPDIRECTDRAWCOLORCONTROL iface)
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
    ULONG ref = InterlockedIncrement((PLONG)&This->DirectDrawGlobal.dwRefCnt);

   	return ref;
}

ULONG WINAPI
Main_DirectDrawColorControl_Release(LPDIRECTDRAWCOLORCONTROL iface)
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
    ULONG ref = InterlockedDecrement((PLONG)&This->DirectDrawGlobal.dwRefCnt);
    
    if (ref == 0)
		HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

HRESULT WINAPI
Main_DirectDrawColorControl_QueryInterface(LPDIRECTDRAWCOLORCONTROL iface, 
										   REFIID riid, LPVOID* ppvObj) 
{
	return E_NOINTERFACE;
}

HRESULT WINAPI
Main_DirectDrawColorControl_GetColorControls(LPDIRECTDRAWCOLORCONTROL iface, LPDDCOLORCONTROL lpColorControl)
{
   	DX_STUB;
}

HRESULT WINAPI
Main_DirectDrawColorControl_SetColorControls(LPDIRECTDRAWCOLORCONTROL iface, LPDDCOLORCONTROL lpColorControl)
{
   	DX_STUB;
}

IDirectDrawColorControlVtbl DirectDrawColorControl_Vtable =
{
    Main_DirectDrawColorControl_QueryInterface,
    Main_DirectDrawColorControl_AddRef,
    Main_DirectDrawColorControl_Release,
    Main_DirectDrawColorControl_GetColorControls,
    Main_DirectDrawColorControl_SetColorControls
};
