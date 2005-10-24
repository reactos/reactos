/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/main/palette.c
 * PURPOSE:              IDirectDrawPalette Implementation 
 * PROGRAMMER:           Maarten Bosma
 *
 */

#include "rosdraw.h"


HRESULT WINAPI
Main_DirectDrawPalette_GetEntries(LPDIRECTDRAWPALETTE iface, DWORD dwFlags,
				  DWORD dwStart, DWORD dwCount,
				  LPPALETTEENTRY palent)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DirectDrawPalette_SetEntries(LPDIRECTDRAWPALETTE iface, DWORD dwFlags,
				  DWORD dwStart, DWORD dwCount,
				  LPPALETTEENTRY palent)
{
    DX_STUB;
}

ULONG WINAPI
Main_DirectDrawPalette_Release(LPDIRECTDRAWPALETTE iface)
{
    DX_STUB;
}

ULONG WINAPI Main_DirectDrawPalette_AddRef(LPDIRECTDRAWPALETTE iface) 
{
    DX_STUB;
}

HRESULT WINAPI
Main_DirectDrawPalette_Initialize(LPDIRECTDRAWPALETTE iface,
				  LPDIRECTDRAW ddraw, DWORD dwFlags,
				  LPPALETTEENTRY palent)
{
    DX_STUB;
}

HRESULT WINAPI
Main_DirectDrawPalette_GetCaps(LPDIRECTDRAWPALETTE iface, LPDWORD lpdwCaps)
{
   DX_STUB;
}

HRESULT WINAPI
Main_DirectDrawPalette_QueryInterface(LPDIRECTDRAWPALETTE iface,
				      REFIID refiid, LPVOID *obj)
{
    DX_STUB;
}

IDirectDrawPaletteVtbl DirectDrawPalette_Vtable =
{
    Main_DirectDrawPalette_QueryInterface,
    Main_DirectDrawPalette_AddRef,
    Main_DirectDrawPalette_Release,
    Main_DirectDrawPalette_GetCaps,
    Main_DirectDrawPalette_GetEntries,
    Main_DirectDrawPalette_Initialize,
    Main_DirectDrawPalette_SetEntries
};
