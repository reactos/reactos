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


HRESULT WINAPI
Main_DirectDrawGammaControl_QueryInterface(LPDIRECTDRAWGAMMACONTROL iface, REFIID riid,
				      LPVOID *ppObj)
{
   	DX_STUB;
}

ULONG WINAPI
Main_DirectDrawGammaControl_AddRef(LPDIRECTDRAWGAMMACONTROL iface)
{
   	DX_STUB;
}

ULONG WINAPI
Main_DirectDrawGammaControl_Release(LPDIRECTDRAWGAMMACONTROL iface)
{
   	DX_STUB;
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
