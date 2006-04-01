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
   	return 1;
}

ULONG WINAPI
Main_DirectDrawColorControl_Release(LPDIRECTDRAWCOLORCONTROL iface)
{
    return 0;
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
