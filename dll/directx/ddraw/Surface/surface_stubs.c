/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 ddraw/surface/surface_stubs.c
 * PURPOSE:              IDirectDrawSurface7 Implementation
 * PROGRAMMER:           Magnus Olsen, Maarten Bosma
 *
 */

#include "rosdraw.h"




/* MSDN: "not currently implemented." */
HRESULT WINAPI
Main_DDrawSurface_AddOverlayDirtyRect(LPDDRAWI_DDRAWSURFACE_INT iface,
					   LPRECT pRect)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_BltFast(LPDDRAWI_DDRAWSURFACE_INT iface, DWORD dstx,
			      DWORD dsty, LPDDRAWI_DDRAWSURFACE_INT src,
			      LPRECT rsrc, DWORD trans)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_Restore(LPDDRAWI_DDRAWSURFACE_INT iface)
{
    DX_WINDBG_trace();

    DX_STUB;
}

/* MSDN: "not currently implemented." */
HRESULT WINAPI
Main_DDrawSurface_BltBatch(LPDDRAWI_DDRAWSURFACE_INT iface,
				LPDDBLTBATCH pBatch, DWORD dwCount,
				DWORD dwFlags)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_ChangeUniquenessValue(LPDDRAWI_DDRAWSURFACE_INT iface)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_DeleteAttachedSurface(LPDDRAWI_DDRAWSURFACE_INT iface,
					     DWORD dwFlags,
					     LPDDRAWI_DDRAWSURFACE_INT pAttach)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_EnumAttachedSurfaces(LPDDRAWI_DDRAWSURFACE_INT iface,
					    LPVOID context,
					    LPDDENUMSURFACESCALLBACK7 cb)
{
	DX_WINDBG_trace();

	DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_EnumOverlayZOrders(LPDDRAWI_DDRAWSURFACE_INT iface,
					  DWORD dwFlags, LPVOID context,
					  LPDDENUMSURFACESCALLBACK7 cb)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_Flip(LPDDRAWI_DDRAWSURFACE_INT iface,
			    LPDDRAWI_DDRAWSURFACE_INT lpDDSurfaceTargetOverride, DWORD dwFlags)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_FreePrivateData(LPDDRAWI_DDRAWSURFACE_INT iface, REFGUID tag)
{
    DX_WINDBG_trace();

    DX_STUB;
}
HRESULT WINAPI
Main_DDrawSurface_GetColorKey(LPDDRAWI_DDRAWSURFACE_INT iface, DWORD dwFlags,
				   LPDDCOLORKEY pCKey)
{
    //LPDDRAWI_DDRAWSURFACE_INT This = (LPDDRAWI_DDRAWSURFACE_INT)iface;

	DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetDDInterface(LPDDRAWI_DDRAWSURFACE_INT iface, LPVOID* pDD)
{
    DX_WINDBG_trace();

    DX_STUB;
}
HRESULT WINAPI
Main_DDrawSurface_GetFlipStatus(LPDDRAWI_DDRAWSURFACE_INT iface, DWORD dwFlags)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetLOD(LPDDRAWI_DDRAWSURFACE_INT iface, LPDWORD pdwMaxLOD)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetOverlayPosition(LPDDRAWI_DDRAWSURFACE_INT iface,
					  LPLONG pX, LPLONG pY)
{
    DX_WINDBG_trace();

    DX_STUB;
}
HRESULT WINAPI
Main_DDrawSurface_GetPalette(LPDDRAWI_DDRAWSURFACE_INT iface,
				  LPDIRECTDRAWPALETTE* ppPalette)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetPriority(LPDDRAWI_DDRAWSURFACE_INT iface,
				   LPDWORD pdwPriority)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetPrivateData(LPDDRAWI_DDRAWSURFACE_INT iface,
				      REFGUID tag, LPVOID pBuffer,
				      LPDWORD pcbBufferSize)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_GetUniquenessValue(LPDDRAWI_DDRAWSURFACE_INT iface,
					  LPDWORD pValue)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_IsLost(LPDDRAWI_DDRAWSURFACE_INT iface)
{
    DX_WINDBG_trace();

    //DX_STUB;
    DX_STUB_str("not implement return not lost\n");
    return DD_OK;
}

HRESULT WINAPI
Main_DDrawSurface_PageLock(LPDDRAWI_DDRAWSURFACE_INT iface, DWORD dwFlags)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_PageUnlock(LPDDRAWI_DDRAWSURFACE_INT iface, DWORD dwFlags)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_SetPalette (LPDDRAWI_DDRAWSURFACE_INT iface,
				  LPDIRECTDRAWPALETTE pPalette)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_SetPriority (LPDDRAWI_DDRAWSURFACE_INT iface,
				   DWORD dwPriority)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_SetPrivateData (LPDDRAWI_DDRAWSURFACE_INT iface,
				      REFGUID tag, LPVOID pData,
				      DWORD cbSize, DWORD dwFlags)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_UpdateOverlay (LPDDRAWI_DDRAWSURFACE_INT iface,
				     LPRECT pSrcRect,
				     LPDDRAWI_DDRAWSURFACE_INT pDstSurface,
				     LPRECT pDstRect, DWORD dwFlags,
				     LPDDOVERLAYFX pFX)
{
    DX_WINDBG_trace();

    DX_STUB;
}


/* MSDN: "not currently implemented." */
HRESULT WINAPI
Main_DDrawSurface_UpdateOverlayDisplay (LPDDRAWI_DDRAWSURFACE_INT This,
					    DWORD dwFlags)
{

	DX_WINDBG_trace();

	if (!This->lpLcl->lpGbl->lpDD->lpDDCBtmp->cbDDSurfaceCallbacks.dwFlags & DDHAL_SURFCB32_UPDATEOVERLAY)
	{
		return DDERR_GENERIC;
	}

	DX_STUB;
}


HRESULT WINAPI Main_DDrawSurface_UpdateOverlayZOrder (LPDDRAWI_DDRAWSURFACE_INT iface,
					   DWORD dwFlags, LPDDRAWI_DDRAWSURFACE_INT pDDSRef)
{
    DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_SetSurfaceDesc(LPDDRAWI_DDRAWSURFACE_INT iface, DDSURFACEDESC2 *DDSD, DWORD Flags)
{
	DX_WINDBG_trace();

    DX_STUB;
}

HRESULT WINAPI
Main_DDrawSurface_SetLOD(LPDDRAWI_DDRAWSURFACE_INT iface, DWORD MaxLOD)
{
	DX_WINDBG_trace();

    DX_STUB;
}


