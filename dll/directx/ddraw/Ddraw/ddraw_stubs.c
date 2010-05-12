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
Main_DirectDraw_CreateClipper(LPDDRAWI_DIRECTDRAW_INT This,
                              DWORD dwFlags,
                              LPDIRECTDRAWCLIPPER *ppClipper,
                              IUnknown *pUnkOuter)
{
	DX_WINDBG_trace();
	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_CreatePalette(LPDDRAWI_DIRECTDRAW_INT This, DWORD dwFlags,
                  LPPALETTEENTRY palent, LPDIRECTDRAWPALETTE* ppPalette, LPUNKNOWN pUnkOuter)
{
	DX_WINDBG_trace();
	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_DuplicateSurface(LPDDRAWI_DIRECTDRAW_INT This, LPDIRECTDRAWSURFACE7 src,
                 LPDIRECTDRAWSURFACE7* dst)
{
	DX_WINDBG_trace();
	DX_STUB;
}

HRESULT WINAPI
Main_DirectDraw_EnumSurfaces(LPDDRAWI_DIRECTDRAW_INT This, DWORD dwFlags,
                 LPDDSURFACEDESC lpDDSD, LPVOID context,
                 LPDDENUMSURFACESCALLBACK callback)
{
	DX_WINDBG_trace();
	DX_STUB;
}

HRESULT WINAPI
Main_DirectDraw_EnumSurfaces4(LPDDRAWI_DIRECTDRAW_INT This, DWORD dwFlags,
                 LPDDSURFACEDESC2 lpDDSD2, LPVOID context,
                 LPDDENUMSURFACESCALLBACK2 callback)
{
	DX_WINDBG_trace();
	DX_STUB;
}

HRESULT WINAPI
Main_DirectDraw_EnumSurfaces7(LPDDRAWI_DIRECTDRAW_INT This, DWORD dwFlags,
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
Main_DirectDraw_FlipToGDISurface(LPDDRAWI_DIRECTDRAW_INT This)
{
	DX_WINDBG_trace();
	DX_STUB;
}


HRESULT WINAPI
Main_DirectDraw_GetGDISurface(LPDDRAWI_DIRECTDRAW_INT This,
                                             LPDIRECTDRAWSURFACE7 *lplpGDIDDSSurface)
{
	DX_WINDBG_trace();
	DX_STUB;
}

HRESULT WINAPI
Main_DirectDraw_GetScanLine(LPDDRAWI_DIRECTDRAW_INT This, LPDWORD lpdwScanLine)
{
	DX_WINDBG_trace();
	DX_STUB;
}

HRESULT WINAPI
Main_DirectDraw_GetVerticalBlankStatus(LPDDRAWI_DIRECTDRAW_INT This, LPBOOL lpbIsInVB)
{
	DX_WINDBG_trace();
	DX_STUB;
}

/*
 * Status: Implentation removed due to rewrite
 */
HRESULT WINAPI
Main_DirectDraw_WaitForVerticalBlank(LPDDRAWI_DIRECTDRAW_INT This, DWORD dwFlags,
                                                   HANDLE h)
{
	DX_WINDBG_trace();
	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_GetSurfaceFromDC(LPDDRAWI_DIRECTDRAW_INT This, HDC hdc,
                                                LPDIRECTDRAWSURFACE7 *lpDDS)
{
	DX_WINDBG_trace();
	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_RestoreAllSurfaces(LPDDRAWI_DIRECTDRAW_INT This)
{
	DX_WINDBG_trace();
	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_TestCooperativeLevel(LPDDRAWI_DIRECTDRAW_INT This)
{
	DX_WINDBG_trace();
	DX_STUB;
}


HRESULT WINAPI Main_DirectDraw_StartModeTest(LPDDRAWI_DIRECTDRAW_INT This, LPSIZE pModes,
                  DWORD dwNumModes, DWORD dwFlags)
{
	DX_WINDBG_trace();
	DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_EvaluateMode(LPDDRAWI_DIRECTDRAW_INT This,DWORD a,DWORD* b)
{
	DX_WINDBG_trace();
	DX_STUB;
}
