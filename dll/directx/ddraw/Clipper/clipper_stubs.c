/* $Id: clipper.c 24690 2006-11-05 21:19:53Z greatlrd $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 ddraw/clipper/clipper_stubs.c
 * PURPOSE:              IDirectDrawClipper Implementation 
 * PROGRAMMER:           Maarten Bosma
 *
 */

#include "rosdraw.h"


ULONG WINAPI 
DirectDrawClipper_Release(LPDIRECTDRAWCLIPPER iface) 
{
  DX_WINDBG_trace();

   DX_STUB;
}

ULONG WINAPI 
DirectDrawClipper_AddRef (LPDIRECTDRAWCLIPPER iface)
{
  DX_WINDBG_trace();

   DX_STUB;  
}

HRESULT WINAPI 
DirectDrawClipper_Initialize( LPDIRECTDRAWCLIPPER iface, 
                              LPDIRECTDRAW lpDD, 
                              DWORD dwFlags) 
{
   /* FIXME not implment */
   DX_WINDBG_trace();
   DX_STUB_DD_OK;
}

HRESULT WINAPI 
DirectDrawClipper_SetHwnd( LPDIRECTDRAWCLIPPER iface, 
                           DWORD dwFlags, 
                           HWND hWnd) 
{
   /* FIXME not implment */
   DX_WINDBG_trace();
   DX_STUB_DD_OK;
}

HRESULT WINAPI 
DirectDrawClipper_GetClipList( LPDIRECTDRAWCLIPPER iface, 
                               LPRECT lpRect, 
                               LPRGNDATA lpClipList,
                               LPDWORD lpdwSize)
{
   DX_WINDBG_trace();
   DX_STUB;    
}

HRESULT WINAPI 
DirectDrawClipper_SetClipList( LPDIRECTDRAWCLIPPER iface,
                               LPRGNDATA lprgn,
                               DWORD dwFlag) 
{
   DX_WINDBG_trace();
   DX_STUB;    
}

HRESULT WINAPI 
DirectDrawClipper_QueryInterface( LPDIRECTDRAWCLIPPER iface, 
                                  REFIID riid, 
                                  LPVOID* ppvObj) 
{
   DX_WINDBG_trace();
   DX_STUB;    
}

HRESULT WINAPI 
DirectDrawClipper_GetHWnd( LPDIRECTDRAWCLIPPER iface, 
                           HWND* hWndPtr) 
{
   DX_WINDBG_trace();
   DX_STUB;    
}

HRESULT WINAPI 
DirectDrawClipper_IsClipListChanged( LPDIRECTDRAWCLIPPER iface, 
                                     BOOL* lpbChanged) 
{
   DX_WINDBG_trace();
   DX_STUB;    
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
