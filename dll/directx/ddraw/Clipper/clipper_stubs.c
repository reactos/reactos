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
DirectDrawClipper_IsClipListChanged( LPDIRECTDRAWCLIPPER iface,
                                     BOOL* lpbChanged)
{
   DX_WINDBG_trace();
   DX_STUB;
}
