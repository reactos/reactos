/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/main/color.c
 * PURPOSE:              DirectDraw Implementation 
 * PROGRAMMER:           Maarten Bosma
 *
 */

#include "rosdraw.h"

HRESULT WINAPI
Main_DirectDrawColorControl_QueryInterface(LPDIRECTDRAWCOLORCONTROL iface, 
										   REFIID riid, LPVOID* ppvObj) 
{
   	DX_STUB;
}

ULONG WINAPI
Main_DirectDrawColorControl_AddRef(LPDIRECTDRAWCOLORCONTROL iface)
{
   	DX_STUB;
}

ULONG WINAPI
Main_DirectDrawColorControl_Release(LPDIRECTDRAWCOLORCONTROL iface)
{
   	DX_STUB;
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
