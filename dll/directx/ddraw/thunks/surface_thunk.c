/* IDirectDrawSurface3 -> IDirectDrawSurface7 thunks
 * Copyright 2000 TransGaming Technologies Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Taken form wine (wine/dlls/ddraw/surface_thunks.c rev 1.2)
 * with some little changes
 *
 */

#include "winedraw.h"

#define CONVERT(pdds) COM_INTERFACE_CAST(IDirectDrawSurfaceImpl,	\
					 IDirectDrawSurface3,		\
					 IDirectDrawSurface7,		\
					 (pdds))

#define CONVERT_REV(pdds) COM_INTERFACE_CAST(IDirectDrawSurfaceImpl,	\
					     IDirectDrawSurface7,	\
					     IDirectDrawSurface3,	\
					     (pdds))

static HRESULT WINAPI
IDirectDrawSurface3Impl_QueryInterface(LPDIRECTDRAWSURFACE3 This, REFIID iid,
				       LPVOID *ppObj)
{
    return IDirectDrawSurface7_QueryInterface(CONVERT(This), iid, ppObj);
}

static ULONG WINAPI
IDirectDrawSurface3Impl_AddRef(LPDIRECTDRAWSURFACE3 This)
{
    return IDirectDrawSurface7_AddRef(CONVERT(This));
}

static ULONG WINAPI
IDirectDrawSurface3Impl_Release(LPDIRECTDRAWSURFACE3 This)
{
    return IDirectDrawSurface7_Release(CONVERT(This));
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_AddAttachedSurface(LPDIRECTDRAWSURFACE3 This,
					   LPDIRECTDRAWSURFACE3 pAttach)
{
    return IDirectDrawSurface7_AddAttachedSurface(CONVERT(This),
						  CONVERT(pAttach));
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_AddOverlayDirtyRect(LPDIRECTDRAWSURFACE3 This,
					    LPRECT pRect)
{
    return IDirectDrawSurface7_AddOverlayDirtyRect(CONVERT(This), pRect);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_Blt(LPDIRECTDRAWSURFACE3 This, LPRECT prcDst,
			    LPDIRECTDRAWSURFACE3 pSrcSurf, LPRECT prcSrc,
			    DWORD dwFlags, LPDDBLTFX pFX)
{
    return IDirectDrawSurface7_Blt(CONVERT(This), prcDst, CONVERT(pSrcSurf),
				   prcSrc, dwFlags, pFX);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_BltBatch(LPDIRECTDRAWSURFACE3 This,
				 LPDDBLTBATCH pBatch, DWORD dwCount,
				 DWORD dwFlags)
{
    return IDirectDrawSurface7_BltBatch(CONVERT(This), pBatch, dwCount,
					dwFlags);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_BltFast(LPDIRECTDRAWSURFACE3 This, DWORD x, DWORD y,
				LPDIRECTDRAWSURFACE3 pSrcSurf, LPRECT prcSrc,
				DWORD dwTrans)
{
    return IDirectDrawSurface7_BltFast(CONVERT(This), x, y, CONVERT(pSrcSurf),
				       prcSrc, dwTrans);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_DeleteAttachedSurface(LPDIRECTDRAWSURFACE3 This,
					      DWORD dwFlags,
					      LPDIRECTDRAWSURFACE3 pAttached)
{
    return IDirectDrawSurface7_DeleteAttachedSurface(CONVERT(This), dwFlags,
						     CONVERT(pAttached));
}

struct callback_info
{
    LPDDENUMSURFACESCALLBACK callback;
    LPVOID context;
};

static HRESULT CALLBACK
EnumCallback(LPDIRECTDRAWSURFACE7 iface, LPDDSURFACEDESC2 pDDSD,
	     LPVOID context)
{
    const struct callback_info* info = context;

#if 0
    /* This is an outgoing conversion so we have to do it. */
    DDSURFACEDESC ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    DDRAW_Convert_DDSURFACEDESC_2_To_1(pDDSD, &ddsd);
#endif

    /* the LPDDSURFACEDESC2 -> LPDDSURFACEDESC coercion is safe, since
     * the data format is compatible with older enum procs */
    return info->callback((LPDIRECTDRAWSURFACE)CONVERT_REV(iface), (LPDDSURFACEDESC)pDDSD,
			  info->context);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE3 This,
					     LPVOID context,
					     LPDDENUMSURFACESCALLBACK callback)
{
    struct callback_info info;

    info.callback = callback;
    info.context  = context;

    return IDirectDrawSurface7_EnumAttachedSurfaces(CONVERT(This), &info,
						    EnumCallback);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_EnumOverlayZOrders(LPDIRECTDRAWSURFACE3 This,
					   DWORD dwFlags, LPVOID context,
					   LPDDENUMSURFACESCALLBACK callback)
{
    struct callback_info info;

    info.callback = callback;
    info.context  = context;

    return IDirectDrawSurface7_EnumOverlayZOrders(CONVERT(This), dwFlags,
						  &info, EnumCallback);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_Flip(LPDIRECTDRAWSURFACE3 This,
			     LPDIRECTDRAWSURFACE3 pOverride, DWORD dwFlags)
{
    return IDirectDrawSurface7_Flip(CONVERT(This), CONVERT(pOverride),
				    dwFlags);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_GetAttachedSurface(LPDIRECTDRAWSURFACE3 This,
					   LPDDSCAPS pCaps,
					   LPDIRECTDRAWSURFACE3* ppAttached)
{
    DDSCAPS2 caps;
    LPDIRECTDRAWSURFACE7 pAttached7;
    HRESULT hr;

    caps.dwCaps  = pCaps->dwCaps;
    caps.dwCaps2 = 0;
    caps.dwCaps3 = 0;
    caps.dwCaps4 = 0;

    hr = IDirectDrawSurface7_GetAttachedSurface(CONVERT(This), &caps,
						&pAttached7);
    if (FAILED(hr)) return hr;

    *ppAttached = CONVERT_REV(pAttached7);
    return hr;
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_GetBltStatus(LPDIRECTDRAWSURFACE3 This, DWORD dwFlags)
{
    return IDirectDrawSurface7_GetBltStatus(CONVERT(This), dwFlags);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_GetCaps(LPDIRECTDRAWSURFACE3 This, LPDDSCAPS pCaps)
{
    DDSCAPS2 caps;
    HRESULT hr;

    hr = IDirectDrawSurface7_GetCaps(CONVERT(This), &caps);
    if (FAILED(hr)) return hr;

    pCaps->dwCaps = caps.dwCaps;
    return hr;
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_GetClipper(LPDIRECTDRAWSURFACE3 This,
				   LPDIRECTDRAWCLIPPER* ppClipper)
{
    return IDirectDrawSurface7_GetClipper(CONVERT(This), ppClipper);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_GetColorKey(LPDIRECTDRAWSURFACE3 This, DWORD dwFlags,
				    LPDDCOLORKEY pCKey)
{
    return IDirectDrawSurface7_GetColorKey(CONVERT(This), dwFlags, pCKey);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_GetDC(LPDIRECTDRAWSURFACE3 This, HDC* phDC)
{
    return IDirectDrawSurface7_GetDC(CONVERT(This), phDC);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_GetFlipStatus(LPDIRECTDRAWSURFACE3 This, DWORD dwFlags)
{
    return IDirectDrawSurface7_GetFlipStatus(CONVERT(This), dwFlags);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_GetOverlayPosition(LPDIRECTDRAWSURFACE3 This, LPLONG pX,
				       LPLONG pY)
{
    return IDirectDrawSurface7_GetOverlayPosition(CONVERT(This), pX, pY);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_GetPalette(LPDIRECTDRAWSURFACE3 This,
				   LPDIRECTDRAWPALETTE* ppPalette)
{
    return IDirectDrawSurface7_GetPalette(CONVERT(This), ppPalette);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_GetPixelFormat(LPDIRECTDRAWSURFACE3 This,
				       LPDDPIXELFORMAT pPixelFormat)
{
    return IDirectDrawSurface7_GetPixelFormat(CONVERT(This), pPixelFormat);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_GetSurfaceDesc(LPDIRECTDRAWSURFACE3 This,
				       LPDDSURFACEDESC pDDSD)
{
    return IDirectDrawSurface7_GetSurfaceDesc(CONVERT(This),
					      (LPDDSURFACEDESC2)pDDSD);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_Initialize(LPDIRECTDRAWSURFACE3 This, LPDIRECTDRAW pDD,
				   LPDDSURFACEDESC pDDSD)
{
    return IDirectDrawSurface7_Initialize(CONVERT(This), pDD,
					  (LPDDSURFACEDESC2)pDDSD);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_IsLost(LPDIRECTDRAWSURFACE3 This)
{
    return IDirectDrawSurface7_IsLost(CONVERT(This));
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_Lock(LPDIRECTDRAWSURFACE3 This, LPRECT pRect,
			     LPDDSURFACEDESC pDDSD, DWORD dwFlags, HANDLE h)
{
    return IDirectDrawSurface7_Lock(CONVERT(This), pRect,
				    (LPDDSURFACEDESC2)pDDSD, dwFlags, h);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_ReleaseDC(LPDIRECTDRAWSURFACE3 This, HDC hDC)
{
    return IDirectDrawSurface7_ReleaseDC(CONVERT(This), hDC);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_Restore(LPDIRECTDRAWSURFACE3 This)
{
    return IDirectDrawSurface7_Restore(CONVERT(This));
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_SetClipper(LPDIRECTDRAWSURFACE3 This,
				   LPDIRECTDRAWCLIPPER pClipper)
{
    return IDirectDrawSurface7_SetClipper(CONVERT(This), pClipper);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_SetColorKey(LPDIRECTDRAWSURFACE3 This, DWORD dwFlags,
				    LPDDCOLORKEY pCKey)
{
    return IDirectDrawSurface7_SetColorKey(CONVERT(This), dwFlags, pCKey);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_SetOverlayPosition(LPDIRECTDRAWSURFACE3 This, LONG x,
				       LONG y)
{
    return IDirectDrawSurface7_SetOverlayPosition(CONVERT(This), x, y);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_SetPalette(LPDIRECTDRAWSURFACE3 This,
				   LPDIRECTDRAWPALETTE pPalette)
{
    return IDirectDrawSurface7_SetPalette(CONVERT(This), pPalette);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_Unlock(LPDIRECTDRAWSURFACE3 This, LPVOID data)
{
    /* data might not be the LPRECT of later versions, so drop it. */
    return IDirectDrawSurface7_Unlock(CONVERT(This), NULL);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_UpdateOverlay(LPDIRECTDRAWSURFACE3 This, LPRECT prcSrc,
				      LPDIRECTDRAWSURFACE3 pDstSurf,
				      LPRECT prcDst, DWORD dwFlags,
				      LPDDOVERLAYFX pFX)
{
    return IDirectDrawSurface7_UpdateOverlay(CONVERT(This), prcSrc,
					     CONVERT(pDstSurf), prcDst,
					     dwFlags, pFX);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_UpdateOverlayDisplay(LPDIRECTDRAWSURFACE3 This,
					     DWORD dwFlags)
{
    return IDirectDrawSurface7_UpdateOverlayDisplay(CONVERT(This), dwFlags);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_UpdateOverlayZOrder(LPDIRECTDRAWSURFACE3 This,
					    DWORD dwFlags,
					    LPDIRECTDRAWSURFACE3 pSurfReference)
{
    return IDirectDrawSurface7_UpdateOverlayZOrder(CONVERT(This), dwFlags,
						   CONVERT(pSurfReference));
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_GetDDInterface(LPDIRECTDRAWSURFACE3 This, LPVOID* ppDD)
{
    return IDirectDrawSurface7_GetDDInterface(CONVERT(This), ppDD);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_PageLock(LPDIRECTDRAWSURFACE3 This, DWORD dwFlags)
{
    return IDirectDrawSurface7_PageLock(CONVERT(This), dwFlags);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_PageUnlock(LPDIRECTDRAWSURFACE3 This, DWORD dwFlags)
{
    return IDirectDrawSurface7_PageUnlock(CONVERT(This), dwFlags);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_SetSurfaceDesc(LPDIRECTDRAWSURFACE3 This,
				       LPDDSURFACEDESC pDDSD, DWORD dwFlags)
{
    return IDirectDrawSurface7_SetSurfaceDesc(CONVERT(This),
					      (LPDDSURFACEDESC2)pDDSD,
					      dwFlags);
}

IDirectDrawSurface3Vtbl DDRAW_IDDS3_Thunk_VTable =
{
    IDirectDrawSurface3Impl_QueryInterface,
    IDirectDrawSurface3Impl_AddRef,
    IDirectDrawSurface3Impl_Release,
    IDirectDrawSurface3Impl_AddAttachedSurface,
    IDirectDrawSurface3Impl_AddOverlayDirtyRect,
    IDirectDrawSurface3Impl_Blt,
    IDirectDrawSurface3Impl_BltBatch,
    IDirectDrawSurface3Impl_BltFast,
    IDirectDrawSurface3Impl_DeleteAttachedSurface,
    IDirectDrawSurface3Impl_EnumAttachedSurfaces,
    IDirectDrawSurface3Impl_EnumOverlayZOrders,
    IDirectDrawSurface3Impl_Flip,
    IDirectDrawSurface3Impl_GetAttachedSurface,
    IDirectDrawSurface3Impl_GetBltStatus,
    IDirectDrawSurface3Impl_GetCaps,
    IDirectDrawSurface3Impl_GetClipper,
    IDirectDrawSurface3Impl_GetColorKey,
    IDirectDrawSurface3Impl_GetDC,
    IDirectDrawSurface3Impl_GetFlipStatus,
    IDirectDrawSurface3Impl_GetOverlayPosition,
    IDirectDrawSurface3Impl_GetPalette,
    IDirectDrawSurface3Impl_GetPixelFormat,
    IDirectDrawSurface3Impl_GetSurfaceDesc,
    IDirectDrawSurface3Impl_Initialize,
    IDirectDrawSurface3Impl_IsLost,
    IDirectDrawSurface3Impl_Lock,
    IDirectDrawSurface3Impl_ReleaseDC,
    IDirectDrawSurface3Impl_Restore,
    IDirectDrawSurface3Impl_SetClipper,
    IDirectDrawSurface3Impl_SetColorKey,
    IDirectDrawSurface3Impl_SetOverlayPosition,
    IDirectDrawSurface3Impl_SetPalette,
    IDirectDrawSurface3Impl_Unlock,
    IDirectDrawSurface3Impl_UpdateOverlay,
    IDirectDrawSurface3Impl_UpdateOverlayDisplay,
    IDirectDrawSurface3Impl_UpdateOverlayZOrder,
    IDirectDrawSurface3Impl_GetDDInterface,
    IDirectDrawSurface3Impl_PageLock,
    IDirectDrawSurface3Impl_PageUnlock,
    IDirectDrawSurface3Impl_SetSurfaceDesc
};
