/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 ddraw/clipper/clipper_stubs.c
 * PURPOSE:              IDirectDrawClipper Implementation
 * PROGRAMMER:           Maarten Bosma
 *
 */

#include "rosdraw.h"

HRESULT WINAPI
DirectDrawClipper_QueryInterface (LPDIRECTDRAWCLIPPER iface,
                                REFIID id,  LPVOID *obj)
{
    if(!iface)
        return DDERR_INVALIDOBJECT;

    return E_NOINTERFACE;
}

HRESULT WINAPI
DirectDrawClipper_Initialize (LPDIRECTDRAWCLIPPER iface,
                               LPDIRECTDRAW lpDD,  DWORD dwFlags)
{
    LPDDRAWI_DDRAWCLIPPER_INT This = (LPDDRAWI_DDRAWCLIPPER_INT)iface;
    LPDDRAWI_DIRECTDRAW_INT DDraw = (LPDDRAWI_DIRECTDRAW_INT)lpDD;

    DX_WINDBG_trace();

    if(!This)
        return DDERR_INVALIDOBJECT;

    if(This->dwIntRefCnt)
        return DDERR_ALREADYINITIALIZED;

    This->lpVtbl = (LPVOID)&DirectDrawClipper_Vtable;
    This->dwIntRefCnt = 1;

    DxHeapMemAlloc(This->lpLcl, sizeof(LPDDRAWI_DDRAWCLIPPER_LCL));
    //This->lpLcl->lpClipMore
    This->lpLcl->lpDD_int = DDraw;
    This->lpLcl->lpGbl->dwRefCnt = 1;
    This->lpLcl->pAddrefedThisOwner = (IUnknown*)DDraw;

    // FIXME: Implement Linking and share global object
    DxHeapMemAlloc(This->lpLcl->lpGbl, sizeof(LPDDRAWI_DDRAWCLIPPER_GBL));
    This->lpLcl->lpGbl->dwProcessId = GetCurrentProcessId();

    return DD_OK;
}

ULONG WINAPI
DirectDrawClipper_Release(LPDIRECTDRAWCLIPPER iface)
{
    LPDDRAWI_DDRAWCLIPPER_INT This = (LPDDRAWI_DDRAWCLIPPER_INT)iface;
    if(!This)
        return DDERR_INVALIDOBJECT;

    DX_WINDBG_trace();

    if (iface!=NULL)
    {
        This->lpLcl->dwLocalRefCnt--;
        This->dwIntRefCnt--;

        if (This->lpLcl->lpGbl)
            This->lpLcl->lpGbl->dwRefCnt--;
    }

    return This->dwIntRefCnt;
}

ULONG WINAPI
DirectDrawClipper_AddRef (LPDIRECTDRAWCLIPPER iface)
{
    LPDDRAWI_DDRAWCLIPPER_INT This = (LPDDRAWI_DDRAWCLIPPER_INT)iface;
    if(!This)
        return DDERR_INVALIDOBJECT;

    DX_WINDBG_trace();

    if (iface!=NULL)
    {
        This->dwIntRefCnt++;
        This->lpLcl->dwLocalRefCnt++;
        if (This->lpLcl->lpGbl)
            This->lpLcl->lpGbl->dwRefCnt++;
    }
    return This->dwIntRefCnt;
}

HRESULT WINAPI
DirectDrawClipper_SetHwnd( LPDIRECTDRAWCLIPPER iface,
                           DWORD dwFlags,
                           HWND hWnd)
{
    LPDDRAWI_DDRAWCLIPPER_INT This = (LPDDRAWI_DDRAWCLIPPER_INT)iface;

    if(!This)
        return DDERR_INVALIDOBJECT;

    if(!IsWindow(hWnd))
        return DDERR_INVALIDPARAMS;

    // TODO: check flags

    This->lpLcl->lpGbl->hWnd = (ULONG_PTR)hWnd;
    This->lpLcl->lpGbl->dwFlags = dwFlags;

    return DD_OK;
}

HRESULT WINAPI
DirectDrawClipper_GetHWnd( LPDIRECTDRAWCLIPPER iface,
                           HWND* hWndPtr)
{
    LPDDRAWI_DDRAWCLIPPER_INT This = (LPDDRAWI_DDRAWCLIPPER_INT)iface;

    if(!This)
        return DDERR_INVALIDOBJECT;

    if(!hWndPtr)
        return DDERR_INVALIDPARAMS;

    *hWndPtr = (HWND)This->lpLcl->lpGbl->hWnd;

    return DD_OK;
}

IDirectDrawClipperVtbl DirectDrawClipper_Vtable =
{
    DirectDrawClipper_QueryInterface,
    DirectDrawClipper_AddRef,
    DirectDrawClipper_Release,
    DirectDrawClipper_GetClipList,
    DirectDrawClipper_GetHWnd,
    DirectDrawClipper_Initialize,
    DirectDrawClipper_IsClipListChanged,
    DirectDrawClipper_SetClipList,
    DirectDrawClipper_SetHwnd
};
