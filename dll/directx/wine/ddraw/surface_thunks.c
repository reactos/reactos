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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wine/port.h"
#include "wine/debug.h"
#include <stdarg.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ddraw.h"
#include "winerror.h"

#include "ddraw_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw_thunk);
WINE_DECLARE_DEBUG_CHANNEL(ddraw);

static HRESULT WINAPI
IDirectDrawSurface3Impl_QueryInterface(LPDIRECTDRAWSURFACE3 This, REFIID iid,
				       LPVOID *ppObj)
{
    return IDirectDrawSurface7_QueryInterface((IDirectDrawSurface7 *)surface_from_surface3(This), iid, ppObj);
}

static ULONG WINAPI
IDirectDrawSurface3Impl_AddRef(LPDIRECTDRAWSURFACE3 This)
{
    return IDirectDrawSurface7_AddRef((IDirectDrawSurface7 *)surface_from_surface3(This));
}

static ULONG WINAPI
IDirectDrawSurface3Impl_Release(LPDIRECTDRAWSURFACE3 iface)
{
    IDirectDrawSurfaceImpl *This = surface_from_surface3(iface);
    TRACE("(%p)\n", This);
    return IDirectDrawSurface7_Release((IDirectDrawSurface7 *)surface_from_surface3(iface));
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_AddAttachedSurface(LPDIRECTDRAWSURFACE3 iface,
					   LPDIRECTDRAWSURFACE3 pAttach)
{
    IDirectDrawSurfaceImpl *This = surface_from_surface3(iface);
    IDirectDrawSurfaceImpl *Surf = surface_from_surface3(pAttach);
    TRACE("(%p)->(%p)\n", This, Surf);

    /* Tests suggest that
     * -> offscreen plain surfaces can be attached to other offscreen plain surfaces
     * -> offscreen plain surfaces can be attached to primaries
     * -> primaries can be attached to offscreen plain surfaces
     * -> z buffers can be attached to primaries
     *
     */
    if(This->surface_desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_OFFSCREENPLAIN) &&
       Surf->surface_desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_OFFSCREENPLAIN))
    {
        /* Sizes have to match */
        if(Surf->surface_desc.dwWidth != This->surface_desc.dwWidth ||
        Surf->surface_desc.dwHeight != This->surface_desc.dwHeight)
        {
            WARN("Surface sizes do not match\n");
            return DDERR_CANNOTATTACHSURFACE;
        }
        /* OK */
    }
    else if(This->surface_desc.ddsCaps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_3DDEVICE) &&
            Surf->surface_desc.ddsCaps.dwCaps & (DDSCAPS_ZBUFFER))
    {
        /* OK */
    }
    else
    {
        WARN("Invalid attachment combination\n");
        return DDERR_CANNOTATTACHSURFACE;
    }

    return IDirectDrawSurfaceImpl_AddAttachedSurface(This,
                                                     Surf);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_AddOverlayDirtyRect(LPDIRECTDRAWSURFACE3 This,
					    LPRECT pRect)
{
    return IDirectDrawSurface7_AddOverlayDirtyRect((IDirectDrawSurface7 *)surface_from_surface3(This), pRect);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_Blt(LPDIRECTDRAWSURFACE3 This, LPRECT prcDst,
			    LPDIRECTDRAWSURFACE3 pSrcSurf, LPRECT prcSrc,
			    DWORD dwFlags, LPDDBLTFX pFX)
{
    return IDirectDrawSurface7_Blt((IDirectDrawSurface7 *)surface_from_surface3(This), prcDst,
            pSrcSurf ? (IDirectDrawSurface7 *)surface_from_surface3(pSrcSurf) : NULL, prcSrc, dwFlags, pFX);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_BltBatch(LPDIRECTDRAWSURFACE3 This,
				 LPDDBLTBATCH pBatch, DWORD dwCount,
				 DWORD dwFlags)
{
    return IDirectDrawSurface7_BltBatch((IDirectDrawSurface7 *)surface_from_surface3(This), pBatch, dwCount, dwFlags);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_BltFast(LPDIRECTDRAWSURFACE3 This, DWORD x, DWORD y,
				LPDIRECTDRAWSURFACE3 pSrcSurf, LPRECT prcSrc,
				DWORD dwTrans)
{
    return IDirectDrawSurface7_BltFast((IDirectDrawSurface7 *)surface_from_surface3(This), x, y,
            pSrcSurf ? (IDirectDrawSurface7 *)surface_from_surface3(pSrcSurf) : NULL, prcSrc, dwTrans);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_DeleteAttachedSurface(LPDIRECTDRAWSURFACE3 This,
					      DWORD dwFlags,
					      LPDIRECTDRAWSURFACE3 pAttached)
{
    return IDirectDrawSurface7_DeleteAttachedSurface((IDirectDrawSurface7 *)surface_from_surface3(This), dwFlags,
            pAttached ? (IDirectDrawSurface7 *)surface_from_surface3(pAttached) : NULL);
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
    return info->callback(iface ?
            (IDirectDrawSurface *)&((IDirectDrawSurfaceImpl *)iface)->IDirectDrawSurface3_vtbl : NULL,
            (LPDDSURFACEDESC)pDDSD, info->context);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE3 This,
					     LPVOID context,
					     LPDDENUMSURFACESCALLBACK callback)
{
    struct callback_info info;

    info.callback = callback;
    info.context  = context;

    return IDirectDrawSurface7_EnumAttachedSurfaces((IDirectDrawSurface7 *)surface_from_surface3(This),
            &info, EnumCallback);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_EnumOverlayZOrders(LPDIRECTDRAWSURFACE3 This,
					   DWORD dwFlags, LPVOID context,
					   LPDDENUMSURFACESCALLBACK callback)
{
    struct callback_info info;

    info.callback = callback;
    info.context  = context;

    return IDirectDrawSurface7_EnumOverlayZOrders((IDirectDrawSurface7 *)surface_from_surface3(This),
            dwFlags, &info, EnumCallback);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_Flip(LPDIRECTDRAWSURFACE3 This,
			     LPDIRECTDRAWSURFACE3 pOverride, DWORD dwFlags)
{
    return IDirectDrawSurface7_Flip((IDirectDrawSurface7 *)surface_from_surface3(This),
            pOverride ? (IDirectDrawSurface7 *)surface_from_surface3(pOverride) : NULL, dwFlags);
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

    hr = IDirectDrawSurface7_GetAttachedSurface((IDirectDrawSurface7 *)surface_from_surface3(This), &caps, &pAttached7);
    if (FAILED(hr)) *ppAttached = NULL;
    else *ppAttached = pAttached7 ?
            (IDirectDrawSurface3 *)&((IDirectDrawSurfaceImpl *)pAttached7)->IDirectDrawSurface3_vtbl : NULL;
    return hr;
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_GetBltStatus(LPDIRECTDRAWSURFACE3 This, DWORD dwFlags)
{
    return IDirectDrawSurface7_GetBltStatus((IDirectDrawSurface7 *)surface_from_surface3(This), dwFlags);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_GetCaps(LPDIRECTDRAWSURFACE3 This, LPDDSCAPS pCaps)
{
    DDSCAPS2 caps;
    HRESULT hr;

    hr = IDirectDrawSurface7_GetCaps((IDirectDrawSurface7 *)surface_from_surface3(This), &caps);
    if (FAILED(hr)) return hr;

    pCaps->dwCaps = caps.dwCaps;
    return hr;
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_GetClipper(LPDIRECTDRAWSURFACE3 This,
				   LPDIRECTDRAWCLIPPER* ppClipper)
{
    return IDirectDrawSurface7_GetClipper((IDirectDrawSurface7 *)surface_from_surface3(This), ppClipper);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_GetColorKey(LPDIRECTDRAWSURFACE3 This, DWORD dwFlags,
				    LPDDCOLORKEY pCKey)
{
    return IDirectDrawSurface7_GetColorKey((IDirectDrawSurface7 *)surface_from_surface3(This), dwFlags, pCKey);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_GetDC(LPDIRECTDRAWSURFACE3 This, HDC* phDC)
{
    return IDirectDrawSurface7_GetDC((IDirectDrawSurface7 *)surface_from_surface3(This), phDC);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_GetFlipStatus(LPDIRECTDRAWSURFACE3 This, DWORD dwFlags)
{
    return IDirectDrawSurface7_GetFlipStatus((IDirectDrawSurface7 *)surface_from_surface3(This), dwFlags);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_GetOverlayPosition(LPDIRECTDRAWSURFACE3 This, LPLONG pX,
				       LPLONG pY)
{
    return IDirectDrawSurface7_GetOverlayPosition((IDirectDrawSurface7 *)surface_from_surface3(This), pX, pY);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_GetPalette(LPDIRECTDRAWSURFACE3 This,
				   LPDIRECTDRAWPALETTE* ppPalette)
{
    return IDirectDrawSurface7_GetPalette((IDirectDrawSurface7 *)surface_from_surface3(This), ppPalette);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_GetPixelFormat(LPDIRECTDRAWSURFACE3 This,
				       LPDDPIXELFORMAT pPixelFormat)
{
    return IDirectDrawSurface7_GetPixelFormat((IDirectDrawSurface7 *)surface_from_surface3(This), pPixelFormat);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_GetSurfaceDesc(LPDIRECTDRAWSURFACE3 iface,
				       LPDDSURFACEDESC pDDSD)
{
    IDirectDrawSurfaceImpl *This = surface_from_surface3(iface);

    TRACE_(ddraw)("(%p)->(%p)\n",This,pDDSD);

    if(!pDDSD)
        return DDERR_INVALIDPARAMS;

    if (pDDSD->dwSize != sizeof(DDSURFACEDESC))
    {
        WARN("Incorrect struct size %d, returning DDERR_INVALIDPARAMS\n",pDDSD->dwSize);
        return DDERR_INVALIDPARAMS;
    }

    EnterCriticalSection(&ddraw_cs);
    DD_STRUCT_COPY_BYSIZE(pDDSD,(DDSURFACEDESC *) &This->surface_desc);
    TRACE("Returning surface desc:\n");
    if (TRACE_ON(ddraw))
    {
        /* DDRAW_dump_surface_desc handles the smaller size */
        DDRAW_dump_surface_desc((DDSURFACEDESC2 *) pDDSD);
    }

    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_Initialize(LPDIRECTDRAWSURFACE3 This, LPDIRECTDRAW pDD,
				   LPDDSURFACEDESC pDDSD)
{
    return IDirectDrawSurface7_Initialize((IDirectDrawSurface7 *)surface_from_surface3(This),
            pDD, (LPDDSURFACEDESC2)pDDSD);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_IsLost(LPDIRECTDRAWSURFACE3 This)
{
    return IDirectDrawSurface7_IsLost((IDirectDrawSurface7 *)surface_from_surface3(This));
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_Lock(LPDIRECTDRAWSURFACE3 This, LPRECT pRect,
			     LPDDSURFACEDESC pDDSD, DWORD dwFlags, HANDLE h)
{
    return IDirectDrawSurface7_Lock((IDirectDrawSurface7 *)surface_from_surface3(This),
            pRect, (LPDDSURFACEDESC2)pDDSD, dwFlags, h);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_ReleaseDC(LPDIRECTDRAWSURFACE3 This, HDC hDC)
{
    return IDirectDrawSurface7_ReleaseDC((IDirectDrawSurface7 *)surface_from_surface3(This), hDC);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_Restore(LPDIRECTDRAWSURFACE3 This)
{
    return IDirectDrawSurface7_Restore((IDirectDrawSurface7 *)surface_from_surface3(This));
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_SetClipper(LPDIRECTDRAWSURFACE3 This,
				   LPDIRECTDRAWCLIPPER pClipper)
{
    return IDirectDrawSurface7_SetClipper((IDirectDrawSurface7 *)surface_from_surface3(This), pClipper);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_SetColorKey(LPDIRECTDRAWSURFACE3 This, DWORD dwFlags,
				    LPDDCOLORKEY pCKey)
{
    return IDirectDrawSurface7_SetColorKey((IDirectDrawSurface7 *)surface_from_surface3(This), dwFlags, pCKey);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_SetOverlayPosition(LPDIRECTDRAWSURFACE3 This, LONG x,
				       LONG y)
{
    return IDirectDrawSurface7_SetOverlayPosition((IDirectDrawSurface7 *)surface_from_surface3(This), x, y);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_SetPalette(LPDIRECTDRAWSURFACE3 This,
				   LPDIRECTDRAWPALETTE pPalette)
{
    return IDirectDrawSurface7_SetPalette((IDirectDrawSurface7 *)surface_from_surface3(This), pPalette);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_Unlock(LPDIRECTDRAWSURFACE3 This, LPVOID data)
{
    /* data might not be the LPRECT of later versions, so drop it. */
    return IDirectDrawSurface7_Unlock((IDirectDrawSurface7 *)surface_from_surface3(This), NULL);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_UpdateOverlay(LPDIRECTDRAWSURFACE3 This, LPRECT prcSrc,
				      LPDIRECTDRAWSURFACE3 pDstSurf,
				      LPRECT prcDst, DWORD dwFlags,
				      LPDDOVERLAYFX pFX)
{
    return IDirectDrawSurface7_UpdateOverlay((IDirectDrawSurface7 *)surface_from_surface3(This), prcSrc,
            pDstSurf ? (IDirectDrawSurface7 *)surface_from_surface3(pDstSurf) : NULL, prcDst, dwFlags, pFX);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_UpdateOverlayDisplay(LPDIRECTDRAWSURFACE3 This,
					     DWORD dwFlags)
{
    return IDirectDrawSurface7_UpdateOverlayDisplay((IDirectDrawSurface7 *)surface_from_surface3(This), dwFlags);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_UpdateOverlayZOrder(LPDIRECTDRAWSURFACE3 This,
					    DWORD dwFlags,
					    LPDIRECTDRAWSURFACE3 pSurfReference)
{
    return IDirectDrawSurface7_UpdateOverlayZOrder((IDirectDrawSurface7 *)surface_from_surface3(This), dwFlags,
            pSurfReference ? (IDirectDrawSurface7 *)surface_from_surface3(pSurfReference) : NULL);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_GetDDInterface(LPDIRECTDRAWSURFACE3 This, LPVOID* ppDD)
{
    return IDirectDrawSurface7_GetDDInterface((IDirectDrawSurface7 *)surface_from_surface3(This), ppDD);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_PageLock(LPDIRECTDRAWSURFACE3 This, DWORD dwFlags)
{
    return IDirectDrawSurface7_PageLock((IDirectDrawSurface7 *)surface_from_surface3(This), dwFlags);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_PageUnlock(LPDIRECTDRAWSURFACE3 This, DWORD dwFlags)
{
    return IDirectDrawSurface7_PageUnlock((IDirectDrawSurface7 *)surface_from_surface3(This), dwFlags);
}

static HRESULT WINAPI
IDirectDrawSurface3Impl_SetSurfaceDesc(LPDIRECTDRAWSURFACE3 This,
				       LPDDSURFACEDESC pDDSD, DWORD dwFlags)
{
    return IDirectDrawSurface7_SetSurfaceDesc((IDirectDrawSurface7 *)surface_from_surface3(This),
            (LPDDSURFACEDESC2)pDDSD, dwFlags);
}

const IDirectDrawSurface3Vtbl IDirectDrawSurface3_Vtbl =
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
