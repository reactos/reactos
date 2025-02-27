/*
 * Copyright 2008 Stefan Dösinger for CodeWeavers
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

#include "wine/debug.h"

#define COBJMACROS

#include "winbase.h"
#include "wingdi.h"

#include "ddraw.h"
#include "d3d.h"

#include "ddrawex_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddrawex);

static struct ddrawex_surface *impl_from_IDirectDrawSurface3(IDirectDrawSurface3 *iface)
{
    return CONTAINING_RECORD(iface, struct ddrawex_surface, IDirectDrawSurface3_iface);
}

static struct ddrawex_surface *unsafe_impl_from_IDirectDrawSurface3(IDirectDrawSurface3 *iface);

static struct ddrawex_surface *impl_from_IDirectDrawSurface4(IDirectDrawSurface4 *iface)
{
    return CONTAINING_RECORD(iface, struct ddrawex_surface, IDirectDrawSurface4_iface);
}

static HRESULT WINAPI ddrawex_surface4_QueryInterface(IDirectDrawSurface4 *iface, REFIID riid, void **out)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (!riid)
    {
        *out = NULL;
        return DDERR_INVALIDPARAMS;
    }

    if (IsEqualGUID(riid, &IID_IDirectDrawSurface4)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        *out = &surface->IDirectDrawSurface4_iface;
    }
    else if (IsEqualGUID(riid, &IID_IDirectDrawSurface3)
            || IsEqualGUID(riid, &IID_IDirectDrawSurface2)
            || IsEqualGUID(riid, &IID_IDirectDrawSurface))
    {
        *out = &surface->IDirectDrawSurface3_iface;
    }
    else
    {
        if (IsEqualGUID(riid, &IID_IDirectDrawGammaControl))
            FIXME("Implement IDirectDrawGammaControl in ddrawex.\n");
        else if (IsEqualGUID(riid, &IID_IDirect3DHALDevice)
                || IsEqualGUID(riid, &IID_IDirect3DRGBDevice))
            FIXME("Test IDirect3DDevice in ddrawex.\n");
        else if (IsEqualGUID(&IID_IDirect3DTexture2, riid)
                || IsEqualGUID( &IID_IDirect3DTexture, riid))
            FIXME("Implement IDirect3DTexture in ddrawex.\n");

        *out = NULL;
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT WINAPI ddrawex_surface3_QueryInterface(IDirectDrawSurface3 *iface, REFIID riid, void **out)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    return ddrawex_surface4_QueryInterface(&surface->IDirectDrawSurface4_iface, riid, out);
}

static ULONG WINAPI ddrawex_surface4_AddRef(IDirectDrawSurface4 *iface)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);
    ULONG refcount = InterlockedIncrement(&surface->ref);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI ddrawex_surface3_AddRef(IDirectDrawSurface3 *iface)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p.\n", iface);

    return ddrawex_surface4_AddRef(&surface->IDirectDrawSurface4_iface);
}

static ULONG WINAPI ddrawex_surface4_Release(IDirectDrawSurface4 *iface)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);
    ULONG refcount = InterlockedDecrement(&surface->ref);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        IDirectDrawSurface4_FreePrivateData(surface->parent, &IID_DDrawexPriv);
        IDirectDrawSurface4_Release(surface->parent);
        free(surface);
    }

    return refcount;
}

static ULONG WINAPI ddrawex_surface3_Release(IDirectDrawSurface3 *iface)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p.\n", iface);

    return ddrawex_surface4_Release(&surface->IDirectDrawSurface4_iface);
}

static HRESULT WINAPI ddrawex_surface4_AddAttachedSurface(IDirectDrawSurface4 *iface,
        IDirectDrawSurface4 *attachment)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);
    struct ddrawex_surface *attachment_impl = unsafe_impl_from_IDirectDrawSurface4(attachment);

    TRACE("iface %p, attachment %p.\n", iface, attachment);

    return IDirectDrawSurface4_AddAttachedSurface(surface->parent, attachment_impl->parent);
}

static HRESULT WINAPI ddrawex_surface3_AddAttachedSurface(IDirectDrawSurface3 *iface,
        IDirectDrawSurface3 *attachment)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);
    struct ddrawex_surface *attachment_impl = unsafe_impl_from_IDirectDrawSurface3(attachment);

    TRACE("iface %p, attachment %p.\n", iface, attachment);

    return ddrawex_surface4_AddAttachedSurface(&surface->IDirectDrawSurface4_iface,
            attachment_impl ? &attachment_impl->IDirectDrawSurface4_iface : NULL);
}

static HRESULT WINAPI ddrawex_surface4_AddOverlayDirtyRect(IDirectDrawSurface4 *iface, RECT *rect)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, rect %s.\n", iface, wine_dbgstr_rect(rect));

    return IDirectDrawSurface4_AddOverlayDirtyRect(surface->parent, rect);
}

static HRESULT WINAPI ddrawex_surface3_AddOverlayDirtyRect(IDirectDrawSurface3 *iface, RECT *rect)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, rect %s.\n", iface, wine_dbgstr_rect(rect));

    return ddrawex_surface4_AddOverlayDirtyRect(&surface->IDirectDrawSurface4_iface, rect);
}

static HRESULT WINAPI ddrawex_surface4_Blt(IDirectDrawSurface4 *iface, RECT *dst_rect,
        IDirectDrawSurface4 *src_surface, RECT *src_rect, DWORD flags, DDBLTFX *fx)
{
    struct ddrawex_surface *dst = impl_from_IDirectDrawSurface4(iface);
    struct ddrawex_surface *src = unsafe_impl_from_IDirectDrawSurface4(src_surface);

    TRACE("iface %p, dst_rect %s, src_surface %p, src_rect %s, flags %#lx, fx %p.\n",
            iface, wine_dbgstr_rect(dst_rect), src_surface, wine_dbgstr_rect(src_rect), flags, fx);

    return IDirectDrawSurface4_Blt(dst->parent, dst_rect, src ? src->parent : NULL, src_rect, flags, fx);
}

static HRESULT WINAPI ddrawex_surface3_Blt(IDirectDrawSurface3 *iface, RECT *dst_rect,
        IDirectDrawSurface3 *src_surface, RECT *src_rect, DWORD flags, DDBLTFX *fx)
{
    struct ddrawex_surface *dst = impl_from_IDirectDrawSurface3(iface);
    struct ddrawex_surface *src = unsafe_impl_from_IDirectDrawSurface3(src_surface);

    TRACE("iface %p, dst_rect %s, src_surface %p, src_rect %s, flags %#lx, fx %p.\n",
            iface, wine_dbgstr_rect(dst_rect), src_surface, wine_dbgstr_rect(src_rect), flags, fx);

    return ddrawex_surface4_Blt(&dst->IDirectDrawSurface4_iface, dst_rect,
            src ? &src->IDirectDrawSurface4_iface : NULL, src_rect, flags, fx);
}

static HRESULT WINAPI ddrawex_surface4_BltBatch(IDirectDrawSurface4 *iface,
        DDBLTBATCH *batch, DWORD count, DWORD flags)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, batch %p, count %lu, flags %#lx.\n", iface, batch, count, flags);

    return IDirectDrawSurface4_BltBatch(surface->parent, batch, count, flags);
}

static HRESULT WINAPI ddrawex_surface3_BltBatch(IDirectDrawSurface3 *iface,
        DDBLTBATCH *batch, DWORD count, DWORD flags)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, batch %p, count %lu, flags %#lx.\n", iface, batch, count, flags);

    return ddrawex_surface4_BltBatch(&surface->IDirectDrawSurface4_iface, batch, count, flags);
}

static HRESULT WINAPI ddrawex_surface4_BltFast(IDirectDrawSurface4 *iface, DWORD dst_x,
        DWORD dst_y, IDirectDrawSurface4 *src_surface, RECT *src_rect, DWORD flags)
{
    struct ddrawex_surface *dst = impl_from_IDirectDrawSurface4(iface);
    struct ddrawex_surface *src = unsafe_impl_from_IDirectDrawSurface4(src_surface);

    TRACE("iface %p, dst_x %lu, dst_y %lu, src_surface %p, src_rect %s, flags %#lx.\n",
            iface, dst_x, dst_y, src_surface, wine_dbgstr_rect(src_rect), flags);

    return IDirectDrawSurface4_BltFast(dst->parent, dst_x, dst_y,
            src ? src->parent : NULL, src_rect, flags);
}

static HRESULT WINAPI ddrawex_surface3_BltFast(IDirectDrawSurface3 *iface, DWORD dst_x,
        DWORD dst_y, IDirectDrawSurface3 *src_surface, RECT *src_rect, DWORD flags)
{
    struct ddrawex_surface *dst = impl_from_IDirectDrawSurface3(iface);
    struct ddrawex_surface *src = unsafe_impl_from_IDirectDrawSurface3(src_surface);

    TRACE("iface %p, dst_x %lu, dst_y %lu, src_surface %p, src_rect %s, flags %#lx.\n",
            iface, dst_x, dst_y, src_surface, wine_dbgstr_rect(src_rect), flags);

    return ddrawex_surface4_BltFast(&dst->IDirectDrawSurface4_iface, dst_x, dst_y,
            src ? &src->IDirectDrawSurface4_iface : NULL, src_rect, flags);
}

static HRESULT WINAPI ddrawex_surface4_DeleteAttachedSurface(IDirectDrawSurface4 *iface,
        DWORD flags, IDirectDrawSurface4 *attachment)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);
    struct ddrawex_surface *attachment_impl = unsafe_impl_from_IDirectDrawSurface4(attachment);

    TRACE("iface %p, flags %#lx, attachment %p.\n", iface, flags, attachment);

    return IDirectDrawSurface4_DeleteAttachedSurface(surface->parent,
            flags, attachment_impl ? attachment_impl->parent : NULL);
}

static HRESULT WINAPI ddrawex_surface3_DeleteAttachedSurface(IDirectDrawSurface3 *iface,
        DWORD flags, IDirectDrawSurface3 *attachment)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);
    struct ddrawex_surface *attachment_impl = unsafe_impl_from_IDirectDrawSurface3(attachment);

    TRACE("iface %p, flags %#lx, attachment %p.\n", iface, flags, attachment);

    return ddrawex_surface4_DeleteAttachedSurface(&surface->IDirectDrawSurface4_iface,
            flags, attachment_impl ? &attachment_impl->IDirectDrawSurface4_iface : NULL);
}

struct enumsurfaces_wrap
{
    LPDDENUMSURFACESCALLBACK2 orig_cb;
    void *orig_ctx;
};

static HRESULT WINAPI
enumsurfaces_wrap_cb(IDirectDrawSurface4 *surf, DDSURFACEDESC2 *desc, void *vctx)
{
    struct enumsurfaces_wrap *ctx = vctx;
    IDirectDrawSurface4 *outer = dds_get_outer(surf);

    TRACE("Returning outer surface %p for inner surface %p\n", outer, surf);
    IDirectDrawSurface4_AddRef(outer);
    IDirectDrawSurface4_Release(surf);
    return ctx->orig_cb(outer, desc, vctx);
}

static HRESULT WINAPI ddrawex_surface4_EnumAttachedSurfaces(IDirectDrawSurface4 *iface,
        void *ctx, LPDDENUMSURFACESCALLBACK2 cb)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);
    struct enumsurfaces_wrap cb_ctx;

    TRACE("iface %p, ctx %p, cb %p.\n", iface, ctx, cb);

    cb_ctx.orig_cb = cb;
    cb_ctx.orig_ctx = ctx;
    return IDirectDrawSurface4_EnumAttachedSurfaces(surface->parent, &cb_ctx, enumsurfaces_wrap_cb);
}

struct enumsurfaces_thunk
{
    LPDDENUMSURFACESCALLBACK orig_cb;
    void *orig_ctx;
};

static HRESULT WINAPI enumsurfaces_thunk_cb(IDirectDrawSurface4 *surf, DDSURFACEDESC2 *desc2,
        void *vctx)
{
    struct ddrawex_surface *surface = unsafe_impl_from_IDirectDrawSurface4(surf);
    struct enumsurfaces_thunk *ctx = vctx;
    DDSURFACEDESC desc;

    TRACE("Thunking back to IDirectDrawSurface3\n");
    IDirectDrawSurface3_AddRef(&surface->IDirectDrawSurface3_iface);
    IDirectDrawSurface3_Release(surf);
    DDSD2_to_DDSD(desc2, &desc);
    return ctx->orig_cb((IDirectDrawSurface *)&surface->IDirectDrawSurface3_iface, &desc, ctx->orig_ctx);
}

static HRESULT WINAPI ddrawex_surface3_EnumAttachedSurfaces(IDirectDrawSurface3 *iface,
        void *ctx, LPDDENUMSURFACESCALLBACK cb)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);
    struct enumsurfaces_thunk cb_ctx;

    TRACE("iface %p, ctx %p, cb %p.\n", iface, ctx, cb);

    cb_ctx.orig_cb = cb;
    cb_ctx.orig_ctx = ctx;
    return ddrawex_surface4_EnumAttachedSurfaces(&surface->IDirectDrawSurface4_iface, &cb_ctx, enumsurfaces_thunk_cb);
}

static HRESULT WINAPI ddrawex_surface4_EnumOverlayZOrders(IDirectDrawSurface4 *iface,
        DWORD flags, void *ctx, LPDDENUMSURFACESCALLBACK2 cb)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);
    struct enumsurfaces_wrap cb_ctx;

    TRACE("iface %p, flags %#lx, ctx %p, cb %p.\n", iface, flags, ctx, cb);

    cb_ctx.orig_cb = cb;
    cb_ctx.orig_ctx = ctx;
    return IDirectDrawSurface4_EnumOverlayZOrders(surface->parent, flags, &cb_ctx, enumsurfaces_wrap_cb);
}

static HRESULT WINAPI ddrawex_surface3_EnumOverlayZOrders(IDirectDrawSurface3 *iface,
        DWORD flags, void *ctx, LPDDENUMSURFACESCALLBACK cb)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);
    struct enumsurfaces_thunk cb_ctx;

    TRACE("iface %p, flags %#lx, ctx %p, cb %p.\n", iface, flags, ctx, cb);

    cb_ctx.orig_cb = cb;
    cb_ctx.orig_ctx = ctx;
    return ddrawex_surface4_EnumOverlayZOrders(&surface->IDirectDrawSurface4_iface,
            flags, &cb_ctx, enumsurfaces_thunk_cb);
}

static HRESULT WINAPI ddrawex_surface4_Flip(IDirectDrawSurface4 *iface, IDirectDrawSurface4 *dst, DWORD flags)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);
    struct ddrawex_surface *dst_impl = unsafe_impl_from_IDirectDrawSurface4(dst);

    TRACE("iface %p, dst %p, flags %#lx.\n", iface, dst, flags);

    return IDirectDrawSurface4_Flip(surface->parent, dst_impl ? dst_impl->parent : NULL, flags);
}

static HRESULT WINAPI ddrawex_surface3_Flip(IDirectDrawSurface3 *iface, IDirectDrawSurface3 *dst, DWORD flags)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);
    struct ddrawex_surface *dst_impl = unsafe_impl_from_IDirectDrawSurface3(dst);

    TRACE("iface %p, dst %p, flags %#lx.\n", iface, dst, flags);

    return ddrawex_surface4_Flip(&surface->IDirectDrawSurface4_iface,
            dst_impl ? &dst_impl->IDirectDrawSurface4_iface : NULL, flags);
}

static HRESULT WINAPI ddrawex_surface4_GetAttachedSurface(IDirectDrawSurface4 *iface,
        DDSCAPS2 *caps, IDirectDrawSurface4 **attachment)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);
    IDirectDrawSurface4 *inner = NULL;
    HRESULT hr;

    TRACE("iface %p, caps %p, attachment %p.\n", iface, caps, attachment);

    if (SUCCEEDED(hr = IDirectDrawSurface4_GetAttachedSurface(surface->parent, caps, &inner)))
    {
        *attachment = dds_get_outer(inner);
        IDirectDrawSurface4_AddRef(*attachment);
        IDirectDrawSurface4_Release(inner);
    }
    else
    {
        *attachment = NULL;
    }
    return hr;
}

static HRESULT WINAPI ddrawex_surface3_GetAttachedSurface(IDirectDrawSurface3 *iface,
        DDSCAPS *caps, IDirectDrawSurface3 **attachment)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);
    IDirectDrawSurface4 *surf4;
    DDSCAPS2 caps2;
    HRESULT hr;

    TRACE("iface %p, caps %p, attachment %p.\n", iface, caps, attachment);

    memset(&caps2, 0, sizeof(caps2));
    caps2.dwCaps = caps->dwCaps;
    if (SUCCEEDED(hr = ddrawex_surface4_GetAttachedSurface(&surface->IDirectDrawSurface4_iface, &caps2, &surf4)))
    {
        IDirectDrawSurface4_QueryInterface(surf4, &IID_IDirectDrawSurface3, (void **)attachment);
        IDirectDrawSurface4_Release(surf4);
    }
    else
    {
        *attachment = NULL;
    }
    return hr;
}

static HRESULT WINAPI ddrawex_surface4_GetBltStatus(IDirectDrawSurface4 *iface, DWORD flags)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, flags %#lx.\n", iface, flags);

    return IDirectDrawSurface4_GetBltStatus(surface->parent, flags);
}

static HRESULT WINAPI ddrawex_surface3_GetBltStatus(IDirectDrawSurface3 *iface, DWORD flags)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, flags %#lx.\n", iface, flags);

    return ddrawex_surface4_GetBltStatus(&surface->IDirectDrawSurface4_iface, flags);
}

static HRESULT WINAPI ddrawex_surface4_GetCaps(IDirectDrawSurface4 *iface, DDSCAPS2 *caps)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, caps %p.\n", iface, caps);

    return IDirectDrawSurface4_GetCaps(surface->parent, caps);
}

static HRESULT WINAPI ddrawex_surface3_GetCaps(IDirectDrawSurface3 *iface, DDSCAPS *caps)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);
    DDSCAPS2 caps2;
    HRESULT hr;

    TRACE("iface %p, caps %p.\n", iface, caps);

    memset(&caps2, 0, sizeof(caps2));
    memset(caps, 0, sizeof(*caps));
    hr = IDirectDrawSurface4_GetCaps(&surface->IDirectDrawSurface4_iface, &caps2);
    caps->dwCaps = caps2.dwCaps;
    return hr;
}

static HRESULT WINAPI ddrawex_surface4_GetClipper(IDirectDrawSurface4 *iface, IDirectDrawClipper **clipper)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, clipper %p.\n", iface, clipper);

    return IDirectDrawSurface4_GetClipper(surface->parent, clipper);
}

static HRESULT WINAPI ddrawex_surface3_GetClipper(IDirectDrawSurface3 *iface, IDirectDrawClipper **clipper)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, clipper %p.\n", iface, clipper);

    return ddrawex_surface4_GetClipper(&surface->IDirectDrawSurface4_iface, clipper);
}

static HRESULT WINAPI ddrawex_surface4_GetColorKey(IDirectDrawSurface4 *iface, DWORD flags, DDCOLORKEY *color_key)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, flags %#lx, color_key %p.\n", iface, flags, color_key);

    return IDirectDrawSurface4_GetColorKey(surface->parent, flags, color_key);
}

static HRESULT WINAPI ddrawex_surface3_GetColorKey(IDirectDrawSurface3 *iface, DWORD flags, DDCOLORKEY *color_key)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, flags %#lx, color_key %p.\n", iface, flags, color_key);

    return ddrawex_surface4_GetColorKey(&surface->IDirectDrawSurface4_iface, flags, color_key);
}

static HRESULT WINAPI ddrawex_surface4_GetDC(IDirectDrawSurface4 *iface, HDC *dc)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, dc %p.\n", iface, dc);

    if (surface->permanent_dc)
    {
        TRACE("Returning stored dc %p.\n", surface->hdc);
        *dc = surface->hdc;
        return DD_OK;
    }

    return IDirectDrawSurface4_GetDC(surface->parent, dc);
}

static HRESULT WINAPI ddrawex_surface3_GetDC(IDirectDrawSurface3 *iface, HDC *dc)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, dc %p.\n", iface, dc);

    return ddrawex_surface4_GetDC(&surface->IDirectDrawSurface4_iface, dc);
}

static HRESULT WINAPI ddrawex_surface4_GetFlipStatus(IDirectDrawSurface4 *iface, DWORD flags)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, flags %#lx.\n", iface, flags);

    return IDirectDrawSurface4_GetFlipStatus(surface->parent, flags);
}

static HRESULT WINAPI ddrawex_surface3_GetFlipStatus(IDirectDrawSurface3 *iface, DWORD flags)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, flags %#lx.\n", iface, flags);

    return ddrawex_surface4_GetFlipStatus(&surface->IDirectDrawSurface4_iface, flags);
}

static HRESULT WINAPI ddrawex_surface4_GetOverlayPosition(IDirectDrawSurface4 *iface, LONG *x, LONG *y)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, x %p, y %p.\n", iface, x, y);

    return IDirectDrawSurface4_GetOverlayPosition(surface->parent, x, y);
}

static HRESULT WINAPI ddrawex_surface3_GetOverlayPosition(IDirectDrawSurface3 *iface, LONG *x, LONG *y)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, x %p, y %p.\n", iface, x, y);

    return ddrawex_surface4_GetOverlayPosition(&surface->IDirectDrawSurface4_iface, x, y);
}

static HRESULT WINAPI ddrawex_surface4_GetPalette(IDirectDrawSurface4 *iface, IDirectDrawPalette **palette)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, palette %p.\n", iface, palette);

    return IDirectDrawSurface4_GetPalette(surface->parent, palette);
}

static HRESULT WINAPI ddrawex_surface3_GetPalette(IDirectDrawSurface3 *iface, IDirectDrawPalette **palette)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, palette %p.\n", iface, palette);

    return ddrawex_surface4_GetPalette(&surface->IDirectDrawSurface4_iface, palette);
}

static HRESULT WINAPI ddrawex_surface4_GetPixelFormat(IDirectDrawSurface4 *iface, DDPIXELFORMAT *pixel_format)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, pixel_format %p.\n", iface, pixel_format);

    return IDirectDrawSurface4_GetPixelFormat(surface->parent, pixel_format);
}

static HRESULT WINAPI ddrawex_surface3_GetPixelFormat(IDirectDrawSurface3 *iface, DDPIXELFORMAT *pixel_format)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, pixel_format %p.\n", iface, pixel_format);

    return ddrawex_surface4_GetPixelFormat(&surface->IDirectDrawSurface4_iface, pixel_format);
}

static HRESULT WINAPI ddrawex_surface4_GetSurfaceDesc(IDirectDrawSurface4 *iface, DDSURFACEDESC2 *desc)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);
    HRESULT hr;

    TRACE("iface %p, desc %p.\n", iface, desc);

    if (SUCCEEDED(hr = IDirectDrawSurface4_GetSurfaceDesc(surface->parent, desc))
            && surface->permanent_dc)
    {
        desc->ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
        desc->ddsCaps.dwCaps &= ~DDSCAPS_OWNDC;
    }

    return hr;
}

static HRESULT WINAPI ddrawex_surface3_GetSurfaceDesc(IDirectDrawSurface3 *iface, DDSURFACEDESC *desc)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);
    DDSURFACEDESC2 ddsd2;
    HRESULT hr;

    TRACE("iface %p, desc %p.\n", iface, desc);

    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    hr = ddrawex_surface4_GetSurfaceDesc(&surface->IDirectDrawSurface4_iface, &ddsd2);
    DDSD2_to_DDSD(&ddsd2, desc);

    return hr;
}

static HRESULT WINAPI ddrawex_surface4_Initialize(IDirectDrawSurface4 *iface,
        IDirectDraw *ddraw, DDSURFACEDESC2 *desc)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);
    IDirectDraw4 *outer_DD4;
    IDirectDraw4 *inner_DD4;
    IDirectDraw *inner_DD;
    HRESULT hr;

    TRACE("iface %p, ddraw %p, desc %p.\n", iface, ddraw, desc);

    IDirectDraw_QueryInterface(ddraw, &IID_IDirectDraw4, (void **)&outer_DD4);
    inner_DD4 = dd_get_inner(outer_DD4);
    IDirectDraw4_Release(outer_DD4);
    IDirectDraw4_QueryInterface(inner_DD4, &IID_IDirectDraw4, (void **)&inner_DD);
    hr = IDirectDrawSurface4_Initialize(surface->parent, inner_DD, desc);
    IDirectDraw_Release(inner_DD);
    return hr;
}

static HRESULT WINAPI ddrawex_surface3_Initialize(IDirectDrawSurface3 *iface,
        IDirectDraw *ddraw, DDSURFACEDESC *desc)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);
    DDSURFACEDESC2 ddsd2;

    TRACE("iface %p, ddraw %p, desc %p.\n", iface, ddraw, desc);

    DDSD_to_DDSD2(desc, &ddsd2);
    return ddrawex_surface4_Initialize(&surface->IDirectDrawSurface4_iface, ddraw, &ddsd2);
}

static HRESULT WINAPI ddrawex_surface4_IsLost(IDirectDrawSurface4 *iface)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p.\n", iface);

    return IDirectDrawSurface4_IsLost(surface->parent);
}

static HRESULT WINAPI ddrawex_surface3_IsLost(IDirectDrawSurface3 *iface)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p.\n", iface);

    return ddrawex_surface4_IsLost(&surface->IDirectDrawSurface4_iface);
}

static HRESULT WINAPI ddrawex_surface4_Lock(IDirectDrawSurface4 *iface, RECT *rect,
        DDSURFACEDESC2 *desc, DWORD flags, HANDLE h)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);
    HRESULT hr;

    TRACE("iface %p, rect %s, desc %p, flags %#lx, h %p.\n",
            iface, wine_dbgstr_rect(rect), desc, flags, h);

    if (SUCCEEDED(hr = IDirectDrawSurface4_Lock(surface->parent, rect, desc, flags, h))
            && surface->permanent_dc)
    {
        desc->ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
        desc->ddsCaps.dwCaps &= ~DDSCAPS_OWNDC;
    }

    return hr;
}

static HRESULT WINAPI ddrawex_surface3_Lock(IDirectDrawSurface3 *iface, RECT *rect,
        DDSURFACEDESC *desc, DWORD flags, HANDLE h)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);
    DDSURFACEDESC2 ddsd2;
    HRESULT hr;

    TRACE("iface %p, rect %s, desc %p, flags %#lx, h %p.\n",
            iface, wine_dbgstr_rect(rect), desc, flags, h);

    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    hr = ddrawex_surface4_Lock(&surface->IDirectDrawSurface4_iface, rect, &ddsd2, flags, h);
    DDSD2_to_DDSD(&ddsd2, desc);

    return hr;
}

static HRESULT WINAPI ddrawex_surface4_ReleaseDC(IDirectDrawSurface4 *iface, HDC dc)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, dc %p.\n", iface, dc);

    if (surface->permanent_dc)
    {
        TRACE("Surface has a permanent DC, not doing anything.\n");
        return DD_OK;
    }

    return IDirectDrawSurface4_ReleaseDC(surface->parent, dc);
}

static HRESULT WINAPI ddrawex_surface3_ReleaseDC(IDirectDrawSurface3 *iface, HDC dc)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, dc %p.\n", iface, dc);

    return ddrawex_surface4_ReleaseDC(&surface->IDirectDrawSurface4_iface, dc);
}

static HRESULT WINAPI ddrawex_surface4_Restore(IDirectDrawSurface4 *iface)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p.\n", iface);

    return IDirectDrawSurface4_Restore(surface->parent);
}

static HRESULT WINAPI ddrawex_surface3_Restore(IDirectDrawSurface3 *iface)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p.\n", iface);

    return ddrawex_surface4_Restore(&surface->IDirectDrawSurface4_iface);
}

static HRESULT WINAPI ddrawex_surface4_SetClipper(IDirectDrawSurface4 *iface, IDirectDrawClipper *clipper)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, clipper %p.\n", iface, clipper);

    return IDirectDrawSurface4_SetClipper(surface->parent, clipper);
}

static HRESULT WINAPI ddrawex_surface3_SetClipper(IDirectDrawSurface3 *iface, IDirectDrawClipper *clipper)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, clipper %p.\n", iface, clipper);

    return ddrawex_surface4_SetClipper(&surface->IDirectDrawSurface4_iface, clipper);
}

static HRESULT WINAPI ddrawex_surface4_SetColorKey(IDirectDrawSurface4 *iface,
        DWORD flags, DDCOLORKEY *color_key)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, flags %#lx, color_key %p.\n", iface, flags, color_key);

    return IDirectDrawSurface4_SetColorKey(surface->parent, flags, color_key);
}

static HRESULT WINAPI ddrawex_surface3_SetColorKey(IDirectDrawSurface3 *iface,
        DWORD flags, DDCOLORKEY *color_key)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, flags %#lx, color_key %p.\n", iface, flags, color_key);

    return ddrawex_surface4_SetColorKey(&surface->IDirectDrawSurface4_iface, flags, color_key);
}

static HRESULT WINAPI ddrawex_surface4_SetOverlayPosition(IDirectDrawSurface4 *iface, LONG x, LONG y)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, x %ld, y %ld.\n", iface, x, y);

    return IDirectDrawSurface4_SetOverlayPosition(surface->parent, x, y);
}

static HRESULT WINAPI ddrawex_surface3_SetOverlayPosition(IDirectDrawSurface3 *iface, LONG x, LONG y)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, x %ld, y %ld.\n", iface, x, y);

    return ddrawex_surface4_SetOverlayPosition(&surface->IDirectDrawSurface4_iface, x, y);
}

static HRESULT WINAPI ddrawex_surface4_SetPalette(IDirectDrawSurface4 *iface, IDirectDrawPalette *palette)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, palette %p.\n", iface, palette);

    return IDirectDrawSurface4_SetPalette(surface->parent, palette);
}

static HRESULT WINAPI ddrawex_surface3_SetPalette(IDirectDrawSurface3 *iface, IDirectDrawPalette *palette)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, palette %p.\n", iface, palette);

    return ddrawex_surface4_SetPalette(&surface->IDirectDrawSurface4_iface, palette);
}

static HRESULT WINAPI ddrawex_surface4_Unlock(IDirectDrawSurface4 *iface, RECT *rect)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, rect %s.\n", iface, wine_dbgstr_rect(rect));

    return IDirectDrawSurface4_Unlock(surface->parent, rect);
}

static HRESULT WINAPI ddrawex_surface3_Unlock(IDirectDrawSurface3 *iface, void *data)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, data %p.\n", iface, data);

    return ddrawex_surface4_Unlock(&surface->IDirectDrawSurface4_iface, NULL);
}

static HRESULT WINAPI ddrawex_surface4_UpdateOverlay(IDirectDrawSurface4 *iface, RECT *src_rect,
        IDirectDrawSurface4 *dst_surface, RECT *dst_rect, DWORD flags, DDOVERLAYFX *fx)
{
    struct ddrawex_surface *src_impl = impl_from_IDirectDrawSurface4(iface);
    struct ddrawex_surface *dst_impl = unsafe_impl_from_IDirectDrawSurface4(dst_surface);

    TRACE("iface %p, src_rect %s, dst_surface %p, dst_rect %s, flags %#lx, fx %p.\n",
            iface, wine_dbgstr_rect(src_rect), dst_surface, wine_dbgstr_rect(dst_rect), flags, fx);

    return IDirectDrawSurface4_UpdateOverlay(src_impl->parent, src_rect,
            dst_impl ? dst_impl->parent : NULL, dst_rect, flags, fx);
}

static HRESULT WINAPI ddrawex_surface3_UpdateOverlay(IDirectDrawSurface3 *iface, RECT *src_rect,
        IDirectDrawSurface3 *dst_surface, RECT *dst_rect, DWORD flags, DDOVERLAYFX *fx)
{
    struct ddrawex_surface *src_impl = impl_from_IDirectDrawSurface3(iface);
    struct ddrawex_surface *dst_impl = unsafe_impl_from_IDirectDrawSurface3(dst_surface);

    TRACE("iface %p, src_rect %s, dst_surface %p, dst_rect %s, flags %#lx, fx %p.\n",
            iface, wine_dbgstr_rect(src_rect), dst_surface, wine_dbgstr_rect(dst_rect), flags, fx);

    return ddrawex_surface4_UpdateOverlay(&src_impl->IDirectDrawSurface4_iface, src_rect,
            dst_impl ? &dst_impl->IDirectDrawSurface4_iface : NULL, dst_rect, flags, fx);
}

static HRESULT WINAPI ddrawex_surface4_UpdateOverlayDisplay(IDirectDrawSurface4 *iface, DWORD flags)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, flags %#lx.\n", iface, flags);

    return IDirectDrawSurface4_UpdateOverlayDisplay(surface->parent, flags);
}

static HRESULT WINAPI ddrawex_surface3_UpdateOverlayDisplay(IDirectDrawSurface3 *iface, DWORD flags)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, flags %#lx.\n", iface, flags);

    return ddrawex_surface4_UpdateOverlayDisplay(&surface->IDirectDrawSurface4_iface, flags);
}

static HRESULT WINAPI ddrawex_surface4_UpdateOverlayZOrder(IDirectDrawSurface4 *iface,
        DWORD flags, IDirectDrawSurface4 *reference)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);
    struct ddrawex_surface *reference_impl = unsafe_impl_from_IDirectDrawSurface4(reference);

    TRACE("iface %p, flags %#lx, reference %p.\n", iface, flags, reference);

    return IDirectDrawSurface4_UpdateOverlayZOrder(surface->parent,
            flags, reference_impl ? reference_impl->parent : NULL);
}

static HRESULT WINAPI ddrawex_surface3_UpdateOverlayZOrder(IDirectDrawSurface3 *iface,
        DWORD flags, IDirectDrawSurface3 *reference)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);
    struct ddrawex_surface *reference_impl = unsafe_impl_from_IDirectDrawSurface3(reference);

    TRACE("iface %p, flags %#lx, reference %p.\n", iface, flags, reference);

    return ddrawex_surface4_UpdateOverlayZOrder(&surface->IDirectDrawSurface4_iface,
            flags, reference_impl ? &reference_impl->IDirectDrawSurface4_iface : NULL);
}

static HRESULT WINAPI ddrawex_surface4_GetDDInterface(IDirectDrawSurface4 *iface, void **ddraw)
{
    FIXME("iface %p, ddraw %p stub!\n", iface, ddraw);

    /* This has to be implemented in ddrawex, ddraw's interface can't be used
     * because it is pretty hard to tell which version of the ddraw interface
     * is returned. */
    *ddraw = NULL;

    return E_FAIL;
}

static HRESULT WINAPI ddrawex_surface3_GetDDInterface(IDirectDrawSurface3 *iface, void **ddraw)
{
    FIXME("iface %p, ddraw %p stub!\n", iface, ddraw);

    /* A thunk it pretty pointless for the same reason that relaying to
     * ddraw.dll works badly. */
    *ddraw = NULL;

    return E_FAIL;
}

static HRESULT WINAPI ddrawex_surface4_PageLock(IDirectDrawSurface4 *iface, DWORD flags)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, flags %#lx.\n", iface, flags);

    return IDirectDrawSurface4_PageLock(surface->parent, flags);
}

static HRESULT WINAPI ddrawex_surface3_PageLock(IDirectDrawSurface3 *iface, DWORD flags)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, flags %#lx.\n", iface, flags);

    return ddrawex_surface4_PageLock(&surface->IDirectDrawSurface4_iface, flags);
}

static HRESULT WINAPI ddrawex_surface4_PageUnlock(IDirectDrawSurface4 *iface, DWORD flags)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, flags %#lx.\n", iface, flags);

    return IDirectDrawSurface4_PageUnlock(surface->parent, flags);
}

static HRESULT WINAPI ddrawex_surface3_PageUnlock(IDirectDrawSurface3 *iface, DWORD flags)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);

    TRACE("iface %p, flags %#lx.\n", iface, flags);

    return ddrawex_surface4_PageUnlock(&surface->IDirectDrawSurface4_iface, flags);
}

static HRESULT WINAPI ddrawex_surface4_SetSurfaceDesc(IDirectDrawSurface4 *iface,
        DDSURFACEDESC2 *desc, DWORD flags)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, desc %p, flags %#lx.\n", iface, desc, flags);

    return IDirectDrawSurface4_SetSurfaceDesc(surface->parent, desc, flags);
}

static HRESULT WINAPI ddrawex_surface3_SetSurfaceDesc(IDirectDrawSurface3 *iface,
        DDSURFACEDESC *desc, DWORD flags)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface3(iface);
    DDSURFACEDESC2 ddsd;

    TRACE("iface %p, desc %p, flags %#lx.\n", iface, desc, flags);

    DDSD_to_DDSD2(desc, &ddsd);
    return ddrawex_surface4_SetSurfaceDesc(&surface->IDirectDrawSurface4_iface, &ddsd, flags);
}

static HRESULT WINAPI ddrawex_surface4_SetPrivateData(IDirectDrawSurface4 *iface,
        REFGUID tag, void *data, DWORD data_size, DWORD flags)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, tag %s, data %p, data_size %lu, flags %#lx.\n",
            iface, debugstr_guid(tag), data, data_size, flags);

    /* To completely avoid this we'd have to clone the private data API in
     * ddrawex. */
    if (IsEqualGUID(&IID_DDrawexPriv, tag))
        FIXME("Application uses ddrawex's private GUID.\n");

    return IDirectDrawSurface4_SetPrivateData(surface->parent, tag, data, data_size, flags);
}

static HRESULT WINAPI ddrawex_surface4_GetPrivateData(IDirectDrawSurface4 *iface,
        REFGUID tag, void *data, DWORD *data_size)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, tag %s, data %p, data_size %p.\n",
            iface, debugstr_guid(tag), data, data_size);

    /* To completely avoid this we'd have to clone the private data API in
     * ddrawex. */
    if (IsEqualGUID(&IID_DDrawexPriv, tag))
        FIXME("Application uses ddrawex's private GUID.\n");

    return IDirectDrawSurface4_GetPrivateData(surface->parent, tag, data, data_size);
}

static HRESULT WINAPI ddrawex_surface4_FreePrivateData(IDirectDrawSurface4 *iface, REFGUID tag)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, tag %s.\n", iface, debugstr_guid(tag));

    /* To completely avoid this we'd have to clone the private data API in
     * ddrawex. */
    if (IsEqualGUID(&IID_DDrawexPriv, tag))
        FIXME("Application uses ddrawex's private GUID.\n");

    return IDirectDrawSurface4_FreePrivateData(surface->parent, tag);
}

static HRESULT WINAPI ddrawex_surface4_GetUniquenessValue(IDirectDrawSurface4 *iface, DWORD *value)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p, value %p.\n", iface, value);

    return IDirectDrawSurface4_GetUniquenessValue(surface->parent, value);
}

static HRESULT WINAPI ddrawex_surface4_ChangeUniquenessValue(IDirectDrawSurface4 *iface)
{
    struct ddrawex_surface *surface = impl_from_IDirectDrawSurface4(iface);

    TRACE("iface %p.\n", iface);

    return IDirectDrawSurface4_ChangeUniquenessValue(surface->parent);
}

static const IDirectDrawSurface3Vtbl ddrawex_surface3_vtbl =
{
    ddrawex_surface3_QueryInterface,
    ddrawex_surface3_AddRef,
    ddrawex_surface3_Release,
    ddrawex_surface3_AddAttachedSurface,
    ddrawex_surface3_AddOverlayDirtyRect,
    ddrawex_surface3_Blt,
    ddrawex_surface3_BltBatch,
    ddrawex_surface3_BltFast,
    ddrawex_surface3_DeleteAttachedSurface,
    ddrawex_surface3_EnumAttachedSurfaces,
    ddrawex_surface3_EnumOverlayZOrders,
    ddrawex_surface3_Flip,
    ddrawex_surface3_GetAttachedSurface,
    ddrawex_surface3_GetBltStatus,
    ddrawex_surface3_GetCaps,
    ddrawex_surface3_GetClipper,
    ddrawex_surface3_GetColorKey,
    ddrawex_surface3_GetDC,
    ddrawex_surface3_GetFlipStatus,
    ddrawex_surface3_GetOverlayPosition,
    ddrawex_surface3_GetPalette,
    ddrawex_surface3_GetPixelFormat,
    ddrawex_surface3_GetSurfaceDesc,
    ddrawex_surface3_Initialize,
    ddrawex_surface3_IsLost,
    ddrawex_surface3_Lock,
    ddrawex_surface3_ReleaseDC,
    ddrawex_surface3_Restore,
    ddrawex_surface3_SetClipper,
    ddrawex_surface3_SetColorKey,
    ddrawex_surface3_SetOverlayPosition,
    ddrawex_surface3_SetPalette,
    ddrawex_surface3_Unlock,
    ddrawex_surface3_UpdateOverlay,
    ddrawex_surface3_UpdateOverlayDisplay,
    ddrawex_surface3_UpdateOverlayZOrder,
    ddrawex_surface3_GetDDInterface,
    ddrawex_surface3_PageLock,
    ddrawex_surface3_PageUnlock,
    ddrawex_surface3_SetSurfaceDesc,
};

static const IDirectDrawSurface4Vtbl ddrawex_surface4_vtbl =
{
    ddrawex_surface4_QueryInterface,
    ddrawex_surface4_AddRef,
    ddrawex_surface4_Release,
    ddrawex_surface4_AddAttachedSurface,
    ddrawex_surface4_AddOverlayDirtyRect,
    ddrawex_surface4_Blt,
    ddrawex_surface4_BltBatch,
    ddrawex_surface4_BltFast,
    ddrawex_surface4_DeleteAttachedSurface,
    ddrawex_surface4_EnumAttachedSurfaces,
    ddrawex_surface4_EnumOverlayZOrders,
    ddrawex_surface4_Flip,
    ddrawex_surface4_GetAttachedSurface,
    ddrawex_surface4_GetBltStatus,
    ddrawex_surface4_GetCaps,
    ddrawex_surface4_GetClipper,
    ddrawex_surface4_GetColorKey,
    ddrawex_surface4_GetDC,
    ddrawex_surface4_GetFlipStatus,
    ddrawex_surface4_GetOverlayPosition,
    ddrawex_surface4_GetPalette,
    ddrawex_surface4_GetPixelFormat,
    ddrawex_surface4_GetSurfaceDesc,
    ddrawex_surface4_Initialize,
    ddrawex_surface4_IsLost,
    ddrawex_surface4_Lock,
    ddrawex_surface4_ReleaseDC,
    ddrawex_surface4_Restore,
    ddrawex_surface4_SetClipper,
    ddrawex_surface4_SetColorKey,
    ddrawex_surface4_SetOverlayPosition,
    ddrawex_surface4_SetPalette,
    ddrawex_surface4_Unlock,
    ddrawex_surface4_UpdateOverlay,
    ddrawex_surface4_UpdateOverlayDisplay,
    ddrawex_surface4_UpdateOverlayZOrder,
    ddrawex_surface4_GetDDInterface,
    ddrawex_surface4_PageLock,
    ddrawex_surface4_PageUnlock,
    ddrawex_surface4_SetSurfaceDesc,
    ddrawex_surface4_SetPrivateData,
    ddrawex_surface4_GetPrivateData,
    ddrawex_surface4_FreePrivateData,
    ddrawex_surface4_GetUniquenessValue,
    ddrawex_surface4_ChangeUniquenessValue,
};

static struct ddrawex_surface *unsafe_impl_from_IDirectDrawSurface3(IDirectDrawSurface3 *iface)
{
    if (!iface || iface->lpVtbl != &ddrawex_surface3_vtbl)
        return NULL;
    return impl_from_IDirectDrawSurface3(iface);
}

struct ddrawex_surface *unsafe_impl_from_IDirectDrawSurface4(IDirectDrawSurface4 *iface)
{
    if (!iface || iface->lpVtbl != &ddrawex_surface4_vtbl)
        return NULL;
    return impl_from_IDirectDrawSurface4(iface);
}

/* dds_get_outer
 *
 * Given a surface from ddraw.dll it retrieves the pointer to the ddrawex.dll wrapper around it
 *
 * Parameters:
 *  inner: ddraw.dll surface to retrieve the outer surface from
 *
 * Returns:
 *  The surface wrapper. If there is none yet, a new one is created
 */
IDirectDrawSurface4 *dds_get_outer(IDirectDrawSurface4 *inner)
{
    IDirectDrawSurface4 *outer = NULL;
    DWORD size = sizeof(outer);
    HRESULT hr;
    if(!inner) return NULL;

    hr = IDirectDrawSurface4_GetPrivateData(inner,
                                            &IID_DDrawexPriv,
                                            &outer,
                                            &size);
    if(FAILED(hr) || outer == NULL)
    {
        struct ddrawex_surface *impl;

        TRACE("Creating new ddrawex surface wrapper for surface %p\n", inner);
        impl = calloc(1, sizeof(*impl));
        impl->ref = 1;
        impl->IDirectDrawSurface3_iface.lpVtbl = &ddrawex_surface3_vtbl;
        impl->IDirectDrawSurface4_iface.lpVtbl = &ddrawex_surface4_vtbl;
        IDirectDrawSurface4_AddRef(inner);
        impl->parent = inner;

        outer = &impl->IDirectDrawSurface4_iface;

        hr = IDirectDrawSurface4_SetPrivateData(inner,
                                                &IID_DDrawexPriv,
                                                &outer,
                                                sizeof(outer),
                                                0 /* Flags */);
        if(FAILED(hr))
        {
            ERR("IDirectDrawSurface4_SetPrivateData failed\n");
        }
    }

    return outer;
}

HRESULT prepare_permanent_dc(IDirectDrawSurface4 *iface)
{
    struct ddrawex_surface *surface = unsafe_impl_from_IDirectDrawSurface4(iface);
    HRESULT hr;

    surface->permanent_dc = TRUE;
    if (FAILED(hr = IDirectDrawSurface4_GetDC(surface->parent, &surface->hdc)))
        return hr;
    return IDirectDrawSurface4_ReleaseDC(surface->parent, surface->hdc);
}
