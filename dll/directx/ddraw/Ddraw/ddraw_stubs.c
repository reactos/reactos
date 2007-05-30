/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 ddraw/ddraw/ddraw_stubs.c
 * PURPOSE:              IDirectDraw7 Implementation
 * PROGRAMMER:           Magnus Olsen, Maarten Bosma
 *
 */

#include "rosdraw.h"

/*
 * Status: Implentation removed due to rewrite
 */
HRESULT
WINAPI
Main_DirectDraw_CreateClipper(LPDIRECTDRAW7 iface,
                              DWORD dwFlags,
                              LPDIRECTDRAWCLIPPER *ppClipper,
                              IUnknown *pUnkOuter)
{
	DX_WINDBG_trace();
	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_CreatePalette(LPDIRECTDRAW7 iface, DWORD dwFlags,
                  LPPALETTEENTRY palent, LPDIRECTDRAWPALETTE* ppPalette, LPUNKNOWN pUnkOuter)
{
	DX_WINDBG_trace();
	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_DuplicateSurface(LPDIRECTDRAW7 iface, LPDIRECTDRAWSURFACE7 src,
                 LPDIRECTDRAWSURFACE7* dst)
{
	DX_WINDBG_trace();
	DX_STUB;
}

HRESULT WINAPI
Main_DirectDraw_EnumSurfaces(LPDIRECTDRAW7 iface, DWORD dwFlags,
                 LPDDSURFACEDESC2 lpDDSD2, LPVOID context,
                 LPDDENUMSURFACESCALLBACK7 callback)
{
	DX_WINDBG_trace();
	DX_STUB;
}

/*
 * Status: Implentation removed due to rewrite
 */
HRESULT WINAPI
Main_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7 iface)
{
	DX_WINDBG_trace();
	DX_STUB;
}

/*
 * Status: Implentation removed due to rewrite
 */
HRESULT WINAPI
Main_DirectDraw_GetCaps(LPDIRECTDRAW7 iface, LPDDCAPS pDriverCaps,
            LPDDCAPS pHELCaps)
{
	DX_WINDBG_trace();
	DX_STUB;
}

HRESULT WINAPI
Main_DirectDraw_GetGDISurface(LPDIRECTDRAW7 iface,
                                             LPDIRECTDRAWSURFACE7 *lplpGDIDDSSurface)
{
	DX_WINDBG_trace();
	DX_STUB;
}

HRESULT WINAPI
Main_DirectDraw_GetScanLine(LPDIRECTDRAW7 iface, LPDWORD lpdwScanLine)
{
	DX_WINDBG_trace();
	DX_STUB;
}

HRESULT WINAPI
Main_DirectDraw_GetVerticalBlankStatus(LPDIRECTDRAW7 iface, LPBOOL lpbIsInVB)
{
	DX_WINDBG_trace();
	DX_STUB;
}

/*
 * Status: Implentation removed due to rewrite
 */
HRESULT WINAPI
Main_DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW7 iface, DWORD dwFlags,
                                                   HANDLE h)
{
	DX_WINDBG_trace();
	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_GetSurfaceFromDC(LPDIRECTDRAW7 iface, HDC hdc,
                                                LPDIRECTDRAWSURFACE7 *lpDDS)
{
	DX_WINDBG_trace();
	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_RestoreAllSurfaces(LPDIRECTDRAW7 iface)
{
	DX_WINDBG_trace();
	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_TestCooperativeLevel(LPDIRECTDRAW7 iface)
{
	DX_WINDBG_trace();
	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_GetDeviceIdentifier(LPDIRECTDRAW7 iface,
                   LPDDDEVICEIDENTIFIER2 pDDDI, DWORD dwFlags)
{
	DX_WINDBG_trace();
	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_StartModeTest(LPDIRECTDRAW7 iface, LPSIZE pModes,
                  DWORD dwNumModes, DWORD dwFlags)
{
	DX_WINDBG_trace();
	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_EvaluateMode(LPDIRECTDRAW7 iface,DWORD a,DWORD* b)
{
	DX_WINDBG_trace();
	DX_STUB;
}
