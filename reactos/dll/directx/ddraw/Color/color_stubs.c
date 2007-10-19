/* $Id: color.c 24690 2006-11-05 21:19:53Z greatlrd $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 ddraw/color/color.c
 * PURPOSE:              IDirectDrawColorControl Implementation
 * PROGRAMMER:           Maarten Bosma
 *
 */

#include "rosdraw.h"

ULONG WINAPI
DirectDrawColorControl_AddRef( LPDIRECTDRAWCOLORCONTROL iface)
{
  DX_WINDBG_trace();

   DX_STUB;
}

ULONG WINAPI
DirectDrawColorControl_Release( LPDIRECTDRAWCOLORCONTROL iface)
{
    LPDDRAWI_DDCOLORCONTROL_INT This = (LPDDRAWI_DDCOLORCONTROL_INT)iface;

    DX_WINDBG_trace();
    /* FIXME
       This is not right exiame how it should be done
     */
    DX_STUB_str("FIXME This is not right exiame how it should be done\n");
    return This->dwIntRefCnt;
}

HRESULT WINAPI
DirectDrawColorControl_QueryInterface( LPDIRECTDRAWCOLORCONTROL iface,
                                       REFIID riid,
                                       LPVOID* ppvObj)
{
   DX_WINDBG_trace();
   DX_STUB;
}

HRESULT WINAPI
DirectDrawColorControl_GetColorControls( LPDIRECTDRAWCOLORCONTROL iface,
                                         LPDDCOLORCONTROL lpColorControl)
{
   DX_WINDBG_trace();
   DX_STUB;
}

HRESULT WINAPI
DirectDrawColorControl_SetColorControls( LPDIRECTDRAWCOLORCONTROL iface,
                                         LPDDCOLORCONTROL lpColorControl)
{
   DX_WINDBG_trace();
   DX_STUB;
}

IDirectDrawColorControlVtbl DirectDrawColorControl_Vtable =
{
    DirectDrawColorControl_QueryInterface,
    DirectDrawColorControl_AddRef,
    DirectDrawColorControl_Release,
    DirectDrawColorControl_GetColorControls,
    DirectDrawColorControl_SetColorControls
};
