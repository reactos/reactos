/* $Id: palette.c 24690 2006-11-05 21:19:53Z greatlrd $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 ddraw/palette/palette_stubs.c
 * PURPOSE:              IDirectDrawPalette Implementation
 * PROGRAMMER:           Maarten Bosma
 *
 */

#include "rosdraw.h"

HRESULT WINAPI
Main_DirectDrawPalette_Initialize( LPDIRECTDRAWPALETTE iface,
				              LPDIRECTDRAW ddraw,
                              DWORD dwFlags,
				              LPPALETTEENTRY palent)
{
   DX_WINDBG_trace();
   DX_STUB;
}

HRESULT WINAPI
Main_DirectDrawPalette_GetEntries( LPDIRECTDRAWPALETTE iface,
                              DWORD dwFlags,
				              DWORD dwStart, DWORD dwCount,
				              LPPALETTEENTRY palent)
{
   DX_WINDBG_trace();
   DX_STUB;
}

HRESULT WINAPI
Main_DirectDrawPalette_SetEntries( LPDIRECTDRAWPALETTE iface,
                              DWORD dwFlags,
				              DWORD dwStart,
                              DWORD dwCount,
				              LPPALETTEENTRY palent)
{
   DX_WINDBG_trace();
   DX_STUB;
}
HRESULT WINAPI
Main_DirectDrawPalette_GetCaps( LPDIRECTDRAWPALETTE iface,
                           LPDWORD lpdwCaps)
{
   DX_WINDBG_trace();
   DX_STUB;
}
