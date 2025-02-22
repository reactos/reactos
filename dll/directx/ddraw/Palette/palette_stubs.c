/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 dll/directx/ddraw/Palette/palette_stubs.c
 * PURPOSE:              IDirectDrawPalette Implementation
 * PROGRAMMER:           Maarten Bosma
 *
 */

#include "rosdraw.h"

ULONG WINAPI
DirectDrawPalette_Release( LPDIRECTDRAWPALETTE iface)
{
  DX_WINDBG_trace();

   DX_STUB;
}

ULONG WINAPI
DirectDrawPalette_AddRef( LPDIRECTDRAWPALETTE iface)
{
  DX_WINDBG_trace();

   DX_STUB;
}

HRESULT WINAPI
DirectDrawPalette_Initialize( LPDIRECTDRAWPALETTE iface,
				              LPDIRECTDRAW ddraw,
                              DWORD dwFlags,
				              LPPALETTEENTRY palent)
{
   DX_WINDBG_trace();
   DX_STUB;
}

HRESULT WINAPI
DirectDrawPalette_GetEntries( LPDIRECTDRAWPALETTE iface,
                              DWORD dwFlags,
				              DWORD dwStart, DWORD dwCount,
				              LPPALETTEENTRY palent)
{
   DX_WINDBG_trace();
   DX_STUB;
}

HRESULT WINAPI
DirectDrawPalette_SetEntries( LPDIRECTDRAWPALETTE iface,
                              DWORD dwFlags,
				              DWORD dwStart,
                              DWORD dwCount,
				              LPPALETTEENTRY palent)
{
   DX_WINDBG_trace();
   DX_STUB;
}
HRESULT WINAPI
DirectDrawPalette_GetCaps( LPDIRECTDRAWPALETTE iface,
                           LPDWORD lpdwCaps)
{
   DX_WINDBG_trace();
   DX_STUB;
}

HRESULT WINAPI
DirectDrawPalette_QueryInterface( LPDIRECTDRAWPALETTE iface,
				                  REFIID refiid,
                                  LPVOID *obj)
{
   DX_WINDBG_trace();
   DX_STUB;
}

IDirectDrawPaletteVtbl DirectDrawPalette_Vtable =
{
    DirectDrawPalette_QueryInterface,
    DirectDrawPalette_AddRef,
    DirectDrawPalette_Release,
    DirectDrawPalette_GetCaps,
    DirectDrawPalette_GetEntries,
    DirectDrawPalette_Initialize,
    DirectDrawPalette_SetEntries
};
