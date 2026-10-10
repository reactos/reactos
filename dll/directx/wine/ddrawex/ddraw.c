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

#define COBJMACROS

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"

#include "ddraw.h"
#include "d3d.h"

#include "ddrawex_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddrawex);

static struct ddrawex *impl_from_IDirectDraw(IDirectDraw *iface)
{
    return CONTAINING_RECORD(iface, struct ddrawex, IDirectDraw_iface);
}

static struct ddrawex *impl_from_IDirectDraw2(IDirectDraw2 *iface)
{
    return CONTAINING_RECORD(iface, struct ddrawex, IDirectDraw2_iface);
}

static struct ddrawex *impl_from_IDirectDraw3(IDirectDraw3 *iface)
{
    return CONTAINING_RECORD(iface, struct ddrawex, IDirectDraw3_iface);
}

static struct ddrawex *impl_from_IDirectDraw4(IDirectDraw4 *iface)
{
    return CONTAINING_RECORD(iface, struct ddrawex, IDirectDraw4_iface);
}

static HRESULT WINAPI ddrawex4_QueryInterface(IDirectDraw4 *iface, REFIID riid, void **out)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (!riid)
    {
        *out = NULL;
        return DDERR_INVALIDPARAMS;
    }

    if (IsEqualGUID(&IID_IDirectDraw4, riid)
            || IsEqualGUID(&IID_IUnknown, riid))
    {
        *out = &ddrawex->IDirectDraw4_iface;
    }
    else if (IsEqualGUID(&IID_IDirectDraw3, riid))
    {
        *out = &ddrawex->IDirectDraw3_iface;
    }
    else if (IsEqualGUID(&IID_IDirectDraw2, riid))
    {
        *out = &ddrawex->IDirectDraw2_iface;
    }
    else if (IsEqualGUID(&IID_IDirectDraw, riid))
    {
        *out = &ddrawex->IDirectDraw_iface;
    }
    else
    {
        *out = NULL;
        WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*out);
    return S_OK;
}

static HRESULT WINAPI ddrawex3_QueryInterface(IDirectDraw3 *iface, REFIID riid, void **out)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    return ddrawex4_QueryInterface(&ddrawex->IDirectDraw4_iface, riid, out);
}

static HRESULT WINAPI ddrawex2_QueryInterface(IDirectDraw2 *iface, REFIID riid, void **out)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    return ddrawex4_QueryInterface(&ddrawex->IDirectDraw4_iface, riid, out);
}

static HRESULT WINAPI ddrawex1_QueryInterface(IDirectDraw *iface, REFIID riid, void **out)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    return ddrawex4_QueryInterface(&ddrawex->IDirectDraw4_iface, riid, out);
}

static ULONG WINAPI ddrawex4_AddRef(IDirectDraw4 *iface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);
    ULONG refcount = InterlockedIncrement(&ddrawex->ref);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI ddrawex3_AddRef(IDirectDraw3 *iface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);

    TRACE("iface %p.\n", iface);

    return ddrawex4_AddRef(&ddrawex->IDirectDraw4_iface);
}

static ULONG WINAPI ddrawex2_AddRef(IDirectDraw2 *iface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw2(iface);

    TRACE("iface %p.\n", iface);

    return ddrawex4_AddRef(&ddrawex->IDirectDraw4_iface);
}

static ULONG WINAPI ddrawex1_AddRef(IDirectDraw *iface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw(iface);

    TRACE("iface %p.\n", iface);

    return ddrawex4_AddRef(&ddrawex->IDirectDraw4_iface);
}

static ULONG WINAPI ddrawex4_Release(IDirectDraw4 *iface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);
    ULONG refcount = InterlockedDecrement(&ddrawex->ref);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        IDirectDraw4_Release(ddrawex->parent);
        free(ddrawex);
    }

    return refcount;
}

static ULONG WINAPI ddrawex3_Release(IDirectDraw3 *iface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);

    TRACE("iface %p.\n", iface);

    return ddrawex4_Release(&ddrawex->IDirectDraw4_iface);
}

static ULONG WINAPI ddrawex2_Release(IDirectDraw2 *iface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw2(iface);

    TRACE("iface %p.\n", iface);

    return ddrawex4_Release(&ddrawex->IDirectDraw4_iface);
}

static ULONG WINAPI ddrawex1_Release(IDirectDraw *iface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw(iface);

    TRACE("iface %p.\n", iface);

    return ddrawex4_Release(&ddrawex->IDirectDraw4_iface);
}

static HRESULT WINAPI ddrawex4_Compact(IDirectDraw4 *iface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);

    TRACE("iface %p.\n", iface);

    return IDirectDraw4_Compact(ddrawex->parent);
}

static HRESULT WINAPI ddrawex3_Compact(IDirectDraw3 *iface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);

    TRACE("iface %p.\n", iface);

    return ddrawex4_Compact(&ddrawex->IDirectDraw4_iface);
}

static HRESULT WINAPI ddrawex2_Compact(IDirectDraw2 *iface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw2(iface);

    TRACE("iface %p.\n", iface);

    return ddrawex4_Compact(&ddrawex->IDirectDraw4_iface);
}

static HRESULT WINAPI ddrawex1_Compact(IDirectDraw *iface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw(iface);

    TRACE("iface %p.\n", iface);

    return ddrawex4_Compact(&ddrawex->IDirectDraw4_iface);
}

static HRESULT WINAPI ddrawex4_CreateClipper(IDirectDraw4 *iface, DWORD flags,
        IDirectDrawClipper **clipper, IUnknown *outer_unknown)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, flags %#lx, clipper %p, outer_unknown %p.\n",
            iface, flags, clipper, outer_unknown);

    /* This may require a wrapper interface for clippers too which handles this. */
    if (outer_unknown)
        FIXME("Test and implement aggregation for ddrawex clippers.\n");

    return IDirectDraw4_CreateClipper(ddrawex->parent, flags, clipper, outer_unknown);
}

static HRESULT WINAPI ddrawex3_CreateClipper(IDirectDraw3 *iface, DWORD flags,
        IDirectDrawClipper **clipper, IUnknown *outer_unknown)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, flags %#lx, clipper %p, outer_unknown %p.\n",
            iface, flags, clipper, outer_unknown);

    return ddrawex4_CreateClipper(&ddrawex->IDirectDraw4_iface, flags, clipper, outer_unknown);
}

static HRESULT WINAPI ddrawex2_CreateClipper(IDirectDraw2 *iface, DWORD flags,
        IDirectDrawClipper **clipper, IUnknown *outer_unknown)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, flags %#lx, clipper %p, outer_unknown %p.\n",
            iface, flags, clipper, outer_unknown);

    return ddrawex4_CreateClipper(&ddrawex->IDirectDraw4_iface, flags, clipper, outer_unknown);
}

static HRESULT WINAPI ddrawex1_CreateClipper(IDirectDraw *iface, DWORD flags,
        IDirectDrawClipper **clipper, IUnknown *outer_unknown)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw(iface);

    TRACE("iface %p, flags %#lx, clipper %p, outer_unknown %p.\n",
            iface, flags, clipper, outer_unknown);

    return ddrawex4_CreateClipper(&ddrawex->IDirectDraw4_iface, flags, clipper, outer_unknown);
}

static HRESULT WINAPI ddrawex4_CreatePalette(IDirectDraw4 *iface, DWORD flags,
        PALETTEENTRY *entries, IDirectDrawPalette **palette, IUnknown *outer_unknown)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, flags %#lx, entries %p, palette %p, outer_unknown %p.\n",
            iface, flags, entries, palette, outer_unknown);

    /* This may require a wrapper interface for palettes too which handles this. */
    if (outer_unknown)
        FIXME("Test and implement aggregation for ddrawex palettes.\n");

    return IDirectDraw4_CreatePalette(ddrawex->parent, flags, entries, palette, outer_unknown);
}

static HRESULT WINAPI ddrawex3_CreatePalette(IDirectDraw3 *iface, DWORD flags,
        PALETTEENTRY *entries, IDirectDrawPalette **palette, IUnknown *outer_unknown)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, flags %#lx, entries %p, palette %p, outer_unknown %p.\n",
            iface, flags, entries, palette, outer_unknown);

    return ddrawex4_CreatePalette(&ddrawex->IDirectDraw4_iface, flags, entries, palette, outer_unknown);
}

static HRESULT WINAPI ddrawex2_CreatePalette(IDirectDraw2 *iface, DWORD flags,
        PALETTEENTRY *entries, IDirectDrawPalette **palette, IUnknown *outer_unknown)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, flags %#lx, entries %p, palette %p, outer_unknown %p.\n",
            iface, flags, entries, palette, outer_unknown);

    return ddrawex4_CreatePalette(&ddrawex->IDirectDraw4_iface, flags, entries, palette, outer_unknown);
}

static HRESULT WINAPI ddrawex1_CreatePalette(IDirectDraw *iface, DWORD flags,
        PALETTEENTRY *entries, IDirectDrawPalette **palette, IUnknown *outer_unknown)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw(iface);

    TRACE("iface %p, flags %#lx, entries %p, palette %p, outer_unknown %p.\n",
            iface, flags, entries, palette, outer_unknown);

    return ddrawex4_CreatePalette(&ddrawex->IDirectDraw4_iface, flags, entries, palette, outer_unknown);
}

static HRESULT WINAPI ddrawex4_CreateSurface(IDirectDraw4 *iface, DDSURFACEDESC2 *desc,
        IDirectDrawSurface4 **surface, IUnknown *outer_unknown)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);
    HRESULT hr;
    const DWORD perm_dc_flags = DDSCAPS_VIDEOMEMORY | DDSCAPS_SYSTEMMEMORY;
    BOOL permanent_dc;
    IDirectDrawSurface4 *inner_surface;

    TRACE("iface %p, desc %p, surface %p, outer_unknown %p.\n",
            iface, desc, surface, outer_unknown);

    /* Handle this in this dll. Don't forward the outer_unknown to ddraw.dll. */
    if (outer_unknown)
        FIXME("Implement aggregation for ddrawex surfaces.\n");

    /* Plain ddraw.dll refuses to create a surface that has both VIDMEM and
     * SYSMEM flags set. In ddrawex this succeeds, and the GetDC() call
     * changes the behavior. The DC is permanently valid, and the surface can
     * be locked between GetDC() and ReleaseDC() calls. GetDC() can be called
     * more than once too. */
    if ((desc->ddsCaps.dwCaps & perm_dc_flags) == perm_dc_flags)
    {
        permanent_dc = TRUE;
        desc->ddsCaps.dwCaps &= ~DDSCAPS_VIDEOMEMORY;
        desc->ddsCaps.dwCaps |= DDSCAPS_OWNDC;
    }
    else
    {
        permanent_dc = FALSE;
    }

    hr = IDirectDraw4_CreateSurface(ddrawex->parent, desc, &inner_surface, outer_unknown);
    if (FAILED(hr))
    {
        *surface = NULL;
        return hr;
    }

    *surface = dds_get_outer(inner_surface);
    /* The wrapper created by dds_get_outer holds a reference to its inner surface. */
    IDirectDrawSurface4_Release(inner_surface);

    if (permanent_dc)
        prepare_permanent_dc(*surface);

    return hr;
}

void DDSD_to_DDSD2(const DDSURFACEDESC *in, DDSURFACEDESC2 *out)
{
    memset(out, 0, sizeof(*out));
    out->dwSize = sizeof(*out);
    out->dwFlags = in->dwFlags;
    if(in->dwFlags & DDSD_WIDTH) out->dwWidth = in->dwWidth;
    if(in->dwFlags & DDSD_HEIGHT) out->dwHeight = in->dwHeight;
    if(in->dwFlags & DDSD_PIXELFORMAT) out->ddpfPixelFormat = in->ddpfPixelFormat;
    if(in->dwFlags & DDSD_CAPS) out->ddsCaps.dwCaps = in->ddsCaps.dwCaps;
    if(in->dwFlags & DDSD_PITCH) out->lPitch = in->lPitch;
    if(in->dwFlags & DDSD_BACKBUFFERCOUNT) out->dwBackBufferCount = in->dwBackBufferCount;
    if(in->dwFlags & DDSD_ZBUFFERBITDEPTH) out->dwMipMapCount = in->dwZBufferBitDepth; /* same union */
    if(in->dwFlags & DDSD_ALPHABITDEPTH) out->dwAlphaBitDepth = in->dwAlphaBitDepth;
    /* DDraw(native, and wine) does not set the DDSD_LPSURFACE, so always copy */
    out->lpSurface = in->lpSurface;
    if(in->dwFlags & DDSD_CKDESTOVERLAY) out->ddckCKDestOverlay = in->ddckCKDestOverlay;
    if(in->dwFlags & DDSD_CKDESTBLT) out->ddckCKDestBlt = in->ddckCKDestBlt;
    if(in->dwFlags & DDSD_CKSRCOVERLAY) out->ddckCKSrcOverlay = in->ddckCKSrcOverlay;
    if(in->dwFlags & DDSD_CKSRCBLT) out->ddckCKSrcBlt = in->ddckCKSrcBlt;
    if(in->dwFlags & DDSD_MIPMAPCOUNT) out->dwMipMapCount = in->dwMipMapCount;
    if(in->dwFlags & DDSD_REFRESHRATE) out->dwRefreshRate = in->dwRefreshRate;
    if(in->dwFlags & DDSD_LINEARSIZE) out->dwLinearSize = in->dwLinearSize;
    /* Does not exist in DDSURFACEDESC:
     * DDSD_TEXTURESTAGE, DDSD_FVF, DDSD_SRCVBHANDLE,
     */
}

void DDSD2_to_DDSD(const DDSURFACEDESC2 *in, DDSURFACEDESC *out)
{
    memset(out, 0, sizeof(*out));
    out->dwSize = sizeof(*out);
    out->dwFlags = in->dwFlags;
    if(in->dwFlags & DDSD_WIDTH) out->dwWidth = in->dwWidth;
    if(in->dwFlags & DDSD_HEIGHT) out->dwHeight = in->dwHeight;
    if(in->dwFlags & DDSD_PIXELFORMAT) out->ddpfPixelFormat = in->ddpfPixelFormat;
    if(in->dwFlags & DDSD_CAPS) out->ddsCaps.dwCaps = in->ddsCaps.dwCaps;
    if(in->dwFlags & DDSD_PITCH) out->lPitch = in->lPitch;
    if(in->dwFlags & DDSD_BACKBUFFERCOUNT) out->dwBackBufferCount = in->dwBackBufferCount;
    if(in->dwFlags & DDSD_ZBUFFERBITDEPTH) out->dwZBufferBitDepth = in->dwMipMapCount; /* same union */
    if(in->dwFlags & DDSD_ALPHABITDEPTH) out->dwAlphaBitDepth = in->dwAlphaBitDepth;
    /* DDraw(native, and wine) does not set the DDSD_LPSURFACE, so always copy */
    out->lpSurface = in->lpSurface;
    if(in->dwFlags & DDSD_CKDESTOVERLAY) out->ddckCKDestOverlay = in->ddckCKDestOverlay;
    if(in->dwFlags & DDSD_CKDESTBLT) out->ddckCKDestBlt = in->ddckCKDestBlt;
    if(in->dwFlags & DDSD_CKSRCOVERLAY) out->ddckCKSrcOverlay = in->ddckCKSrcOverlay;
    if(in->dwFlags & DDSD_CKSRCBLT) out->ddckCKSrcBlt = in->ddckCKSrcBlt;
    if(in->dwFlags & DDSD_MIPMAPCOUNT) out->dwMipMapCount = in->dwMipMapCount;
    if(in->dwFlags & DDSD_REFRESHRATE) out->dwRefreshRate = in->dwRefreshRate;
    if(in->dwFlags & DDSD_LINEARSIZE) out->dwLinearSize = in->dwLinearSize;
    /* Does not exist in DDSURFACEDESC:
     * DDSD_TEXTURESTAGE, DDSD_FVF, DDSD_SRCVBHANDLE,
     */
    if(in->dwFlags & DDSD_TEXTURESTAGE) WARN("Does not exist in DDSURFACEDESC: DDSD_TEXTURESTAGE\n");
    if(in->dwFlags & DDSD_FVF) WARN("Does not exist in DDSURFACEDESC: DDSD_FVF\n");
    if(in->dwFlags & DDSD_SRCVBHANDLE) WARN("Does not exist in DDSURFACEDESC: DDSD_SRCVBHANDLE\n");
    out->dwFlags &= ~(DDSD_TEXTURESTAGE | DDSD_FVF | DDSD_SRCVBHANDLE);
}

static HRESULT WINAPI ddrawex3_CreateSurface(IDirectDraw3 *iface, DDSURFACEDESC *desc,
        IDirectDrawSurface **surface, IUnknown *outer_unknown)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);
    DDSURFACEDESC2 ddsd2;
    IDirectDrawSurface4 *surf4 = NULL;
    HRESULT hr;

    TRACE("iface %p, desc %p, surface %p, outer_unknown %p.\n",
            iface, desc, surface, outer_unknown);

    DDSD_to_DDSD2(desc, &ddsd2);
    if (FAILED(hr = ddrawex4_CreateSurface(&ddrawex->IDirectDraw4_iface, &ddsd2, &surf4, outer_unknown)))
    {
        *surface = NULL;
        return hr;
    }

    TRACE("Got surface %p\n", surf4);
    IDirectDrawSurface4_QueryInterface(surf4, &IID_IDirectDrawSurface, (void **)surface);
    IDirectDrawSurface4_Release(surf4);
    return hr;
}

static HRESULT WINAPI ddrawex2_CreateSurface(IDirectDraw2 *iface, DDSURFACEDESC *desc,
        IDirectDrawSurface **surface, IUnknown *outer_unknown)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, desc %p, surface %p, outer_unknown %p.\n",
            iface, desc, surface, outer_unknown);

    return ddrawex3_CreateSurface(&ddrawex->IDirectDraw3_iface, desc, surface, outer_unknown);
}

static HRESULT WINAPI ddrawex1_CreateSurface(IDirectDraw *iface, DDSURFACEDESC *desc,
        IDirectDrawSurface **surface, IUnknown *outer_unknown)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw(iface);

    TRACE("iface %p, desc %p, surface %p, outer_unknown %p.\n",
            iface, desc, surface, outer_unknown);

    return ddrawex3_CreateSurface(&ddrawex->IDirectDraw3_iface, desc, surface, outer_unknown);
}

static HRESULT WINAPI ddrawex4_DuplicateSurface(IDirectDraw4 *iface,
        IDirectDrawSurface4 *src, IDirectDrawSurface4 **dst)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);
    struct ddrawex_surface *src_impl = unsafe_impl_from_IDirectDrawSurface4(src);

    TRACE("iface %p, src %p, dst %p.\n", iface, src, dst);
    FIXME("Create a wrapper surface.\n");

    return IDirectDraw4_DuplicateSurface(ddrawex->parent, src_impl ? src_impl->parent : NULL, dst);
}

static HRESULT WINAPI ddrawex3_DuplicateSurface(IDirectDraw3 *iface,
        IDirectDrawSurface *src, IDirectDrawSurface **dst)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);
    IDirectDrawSurface4 *src_4;
    IDirectDrawSurface4 *dst_4;
    HRESULT hr;

    TRACE("iface %p, src %p, dst %p.\n", iface, src, dst);

    IDirectDrawSurface_QueryInterface(src, &IID_IDirectDrawSurface4, (void **)&src_4);
    hr = ddrawex4_DuplicateSurface(&ddrawex->IDirectDraw4_iface, src_4, &dst_4);
    IDirectDrawSurface4_Release(src_4);
    if (FAILED(hr))
    {
        *dst = NULL;
        return hr;
    }

    IDirectDrawSurface4_QueryInterface(dst_4, &IID_IDirectDrawSurface, (void **)dst);
    IDirectDrawSurface4_Release(dst_4);

    return hr;
}

static HRESULT WINAPI ddrawex2_DuplicateSurface(IDirectDraw2 *iface,
        IDirectDrawSurface *src, IDirectDrawSurface **dst)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, src %p, dst %p.\n", iface, src, dst);

    return ddrawex3_DuplicateSurface(&ddrawex->IDirectDraw3_iface, src, dst);
}

static HRESULT WINAPI ddrawex1_DuplicateSurface(IDirectDraw *iface,
        IDirectDrawSurface *src, IDirectDrawSurface **dst)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw(iface);

    TRACE("iface %p, src %p, dst %p.\n", iface, src, dst);

    return ddrawex3_DuplicateSurface(&ddrawex->IDirectDraw3_iface, src, dst);
}

static HRESULT WINAPI ddrawex4_EnumDisplayModes(IDirectDraw4 *iface, DWORD flags,
        DDSURFACEDESC2 *desc, void *ctx, LPDDENUMMODESCALLBACK2 cb)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, flags %#lx, desc %p, ctx %p, cb %p.\n", iface, flags, desc, ctx, cb);

    return IDirectDraw4_EnumDisplayModes(ddrawex->parent, flags, desc, ctx, cb);
}

struct enummodes_ctx
{
    LPDDENUMMODESCALLBACK orig_cb;
    void *orig_ctx;
};

static HRESULT WINAPI
enum_modes_cb2(DDSURFACEDESC2 *ddsd2, void *vctx)
{
    struct enummodes_ctx *ctx = vctx;
    DDSURFACEDESC ddsd;

    DDSD2_to_DDSD(ddsd2, &ddsd);
    return ctx->orig_cb(&ddsd, ctx->orig_ctx);
}

static HRESULT WINAPI ddrawex3_EnumDisplayModes(IDirectDraw3 *iface, DWORD flags,
        DDSURFACEDESC *desc, void *ctx, LPDDENUMMODESCALLBACK cb)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);
    struct enummodes_ctx cb_ctx;
    DDSURFACEDESC2 ddsd2;

    TRACE("iface %p, flags %#lx, desc %p, ctx %p, cb %p.\n", iface, flags, desc, ctx, cb);

    DDSD_to_DDSD2(desc, &ddsd2);
    cb_ctx.orig_cb = cb;
    cb_ctx.orig_ctx = ctx;
    return ddrawex4_EnumDisplayModes(&ddrawex->IDirectDraw4_iface, flags, &ddsd2, &cb_ctx, enum_modes_cb2);
}

static HRESULT WINAPI ddrawex2_EnumDisplayModes(IDirectDraw2 *iface, DWORD flags,
        DDSURFACEDESC *desc, void *ctx, LPDDENUMMODESCALLBACK cb)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, flags %#lx, desc %p, ctx %p, cb %p.\n", iface, flags, desc, ctx, cb);

    return ddrawex3_EnumDisplayModes(&ddrawex->IDirectDraw3_iface, flags, desc, ctx, cb);
}

static HRESULT WINAPI ddrawex1_EnumDisplayModes(IDirectDraw *iface, DWORD flags,
        DDSURFACEDESC *desc, void *ctx, LPDDENUMMODESCALLBACK cb)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw(iface);

    TRACE("iface %p, flags %#lx, desc %p, ctx %p, cb %p.\n", iface, flags, desc, ctx, cb);

    return ddrawex3_EnumDisplayModes(&ddrawex->IDirectDraw3_iface, flags, desc, ctx, cb);
}

struct enumsurfaces4_ctx
{
    LPDDENUMSURFACESCALLBACK2 orig_cb;
    void *orig_ctx;
};

static HRESULT WINAPI
enum_surfaces_wrapper(IDirectDrawSurface4 *surf4, DDSURFACEDESC2 *ddsd2, void *vctx)
{
    struct enumsurfaces4_ctx *ctx = vctx;
    IDirectDrawSurface4 *outer = dds_get_outer(surf4);
    IDirectDrawSurface4_AddRef(outer);
    IDirectDrawSurface4_Release(surf4);
    TRACE("Returning wrapper surface %p for enumerated inner surface %p\n", outer, surf4);
    return ctx->orig_cb(outer, ddsd2, ctx->orig_ctx);
}

static HRESULT WINAPI ddrawex4_EnumSurfaces(IDirectDraw4 *iface, DWORD flags,
        DDSURFACEDESC2 *desc, void *ctx, LPDDENUMSURFACESCALLBACK2 cb)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);
    struct enumsurfaces4_ctx cb_ctx;

    TRACE("iface %p, flags %#lx, desc %p, ctx %p, cb %p.\n", iface, flags, desc, ctx, cb);

    cb_ctx.orig_cb = cb;
    cb_ctx.orig_ctx = ctx;
    return IDirectDraw4_EnumSurfaces(ddrawex->parent, flags, desc, &cb_ctx, enum_surfaces_wrapper);
}

struct enumsurfaces_ctx
{
    LPDDENUMSURFACESCALLBACK orig_cb;
    void *orig_ctx;
};

static HRESULT WINAPI
enum_surfaces_cb2(IDirectDrawSurface4 *surf4, DDSURFACEDESC2 *ddsd2, void *vctx)
{
    struct enumsurfaces_ctx *ctx = vctx;
    IDirectDrawSurface *surf1;
    DDSURFACEDESC ddsd;

    /* Keep the reference, it goes to the application */
    IDirectDrawSurface4_QueryInterface(surf4, &IID_IDirectDrawSurface, (void **) &surf1);
    /* Release the reference this function got */
    IDirectDrawSurface4_Release(surf4);

    DDSD2_to_DDSD(ddsd2, &ddsd);
    return ctx->orig_cb(surf1, &ddsd, ctx->orig_ctx);
}

static HRESULT WINAPI ddrawex3_EnumSurfaces(IDirectDraw3 *iface, DWORD flags,
        DDSURFACEDESC *desc, void *ctx, LPDDENUMSURFACESCALLBACK cb)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);
    DDSURFACEDESC2 ddsd2;
    struct enumsurfaces_ctx cb_ctx;

    TRACE("iface %p, flags %#lx, desc %p, ctx %p, cb %p.\n", iface, flags, desc, ctx, cb);

    DDSD_to_DDSD2(desc, &ddsd2);
    cb_ctx.orig_cb = cb;
    cb_ctx.orig_ctx = ctx;
    return ddrawex4_EnumSurfaces(&ddrawex->IDirectDraw4_iface, flags, &ddsd2, &cb_ctx, enum_surfaces_cb2);
}

static HRESULT WINAPI ddrawex2_EnumSurfaces(IDirectDraw2 *iface, DWORD flags,
        DDSURFACEDESC *desc, void *ctx, LPDDENUMSURFACESCALLBACK cb)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, flags %#lx, desc %p, ctx %p, cb %p.\n", iface, flags, desc, ctx, cb);

    return ddrawex3_EnumSurfaces(&ddrawex->IDirectDraw3_iface, flags, desc, ctx, cb);
}

static HRESULT WINAPI ddrawex1_EnumSurfaces(IDirectDraw *iface, DWORD flags,
        DDSURFACEDESC *desc, void *ctx, LPDDENUMSURFACESCALLBACK cb)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw(iface);

    TRACE("iface %p, flags %#lx, desc %p, ctx %p, cb %p.\n", iface, flags, desc, ctx, cb);

    return ddrawex3_EnumSurfaces(&ddrawex->IDirectDraw3_iface, flags, desc, ctx, cb);
}

static HRESULT WINAPI ddrawex4_FlipToGDISurface(IDirectDraw4 *iface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);

    TRACE("iface %p.\n", iface);

    return IDirectDraw4_FlipToGDISurface(ddrawex->parent);
}

static HRESULT WINAPI ddrawex3_FlipToGDISurface(IDirectDraw3 *iface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);

    TRACE("iface %p.\n", iface);

    return ddrawex4_FlipToGDISurface(&ddrawex->IDirectDraw4_iface);
}

static HRESULT WINAPI ddrawex2_FlipToGDISurface(IDirectDraw2 *iface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw2(iface);

    TRACE("iface %p.\n", iface);

    return ddrawex4_FlipToGDISurface(&ddrawex->IDirectDraw4_iface);
}

static HRESULT WINAPI ddrawex1_FlipToGDISurface(IDirectDraw *iface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw(iface);

    TRACE("iface %p.\n", iface);

    return ddrawex4_FlipToGDISurface(&ddrawex->IDirectDraw4_iface);
}

static HRESULT WINAPI ddrawex4_GetCaps(IDirectDraw4 *iface, DDCAPS *driver_caps, DDCAPS *hel_caps)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, driver_caps %p, hel_caps %p.\n", iface, driver_caps, hel_caps);

    return IDirectDraw4_GetCaps(ddrawex->parent, driver_caps, hel_caps);
}

static HRESULT WINAPI ddrawex3_GetCaps(IDirectDraw3 *iface, DDCAPS *driver_caps, DDCAPS *hel_caps)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, driver_caps %p, hel_caps %p.\n", iface, driver_caps, hel_caps);

    return ddrawex4_GetCaps(&ddrawex->IDirectDraw4_iface, driver_caps, hel_caps);
}

static HRESULT WINAPI ddrawex2_GetCaps(IDirectDraw2 *iface, DDCAPS *driver_caps, DDCAPS *hel_caps)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, driver_caps %p, hel_caps %p.\n", iface, driver_caps, hel_caps);

    return ddrawex4_GetCaps(&ddrawex->IDirectDraw4_iface, driver_caps, hel_caps);
}

static HRESULT WINAPI ddrawex1_GetCaps(IDirectDraw *iface, DDCAPS *driver_caps, DDCAPS *hel_caps)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw(iface);

    TRACE("iface %p, driver_caps %p, hel_caps %p.\n", iface, driver_caps, hel_caps);

    return ddrawex4_GetCaps(&ddrawex->IDirectDraw4_iface, driver_caps, hel_caps);
}

static HRESULT WINAPI ddrawex4_GetDisplayMode(IDirectDraw4 *iface, DDSURFACEDESC2 *desc)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    return IDirectDraw4_GetDisplayMode(ddrawex->parent, desc);
}

static HRESULT WINAPI ddrawex3_GetDisplayMode(IDirectDraw3 *iface, DDSURFACEDESC *desc)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);
    DDSURFACEDESC2 ddsd2;
    HRESULT hr;

    TRACE("iface %p, desc %p.\n", iface, desc);

    hr = ddrawex4_GetDisplayMode(&ddrawex->IDirectDraw4_iface, &ddsd2);
    DDSD2_to_DDSD(&ddsd2, desc);

    return hr;
}

static HRESULT WINAPI ddrawex2_GetDisplayMode(IDirectDraw2 *iface, DDSURFACEDESC *desc)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    return ddrawex3_GetDisplayMode(&ddrawex->IDirectDraw3_iface, desc);
}

static HRESULT WINAPI ddrawex1_GetDisplayMode(IDirectDraw *iface, DDSURFACEDESC *desc)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    return ddrawex3_GetDisplayMode(&ddrawex->IDirectDraw3_iface, desc);
}

static HRESULT WINAPI ddrawex4_GetFourCCCodes(IDirectDraw4 *iface, DWORD *code_count, DWORD *codes)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, code_count %p, codes %p.\n", iface, code_count, codes);

    return IDirectDraw4_GetFourCCCodes(ddrawex->parent, code_count, codes);
}

static HRESULT WINAPI ddrawex3_GetFourCCCodes(IDirectDraw3 *iface, DWORD *code_count, DWORD *codes)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, code_count %p, codes %p.\n", iface, code_count, codes);

    return ddrawex4_GetFourCCCodes(&ddrawex->IDirectDraw4_iface, code_count, codes);
}

static HRESULT WINAPI ddrawex2_GetFourCCCodes(IDirectDraw2 *iface, DWORD *code_count, DWORD *codes)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, code_count %p, codes %p.\n", iface, code_count, codes);

    return ddrawex4_GetFourCCCodes(&ddrawex->IDirectDraw4_iface, code_count, codes);
}

static HRESULT WINAPI ddrawex1_GetFourCCCodes(IDirectDraw *iface, DWORD *code_count, DWORD *codes)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw(iface);

    TRACE("iface %p, code_count %p, codes %p.\n", iface, code_count, codes);

    return ddrawex4_GetFourCCCodes(&ddrawex->IDirectDraw4_iface, code_count, codes);
}

static HRESULT WINAPI ddrawex4_GetGDISurface(IDirectDraw4 *iface, IDirectDrawSurface4 **gdi_surface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);
    IDirectDrawSurface4 *inner = NULL;
    HRESULT hr;

    TRACE("iface %p, gdi_surface %p.\n", iface, gdi_surface);

    if (FAILED(hr = IDirectDraw4_GetGDISurface(ddrawex->parent, &inner)))
    {
        *gdi_surface = NULL;
        return hr;
    }

    *gdi_surface = dds_get_outer(inner);
    IDirectDrawSurface4_AddRef(*gdi_surface);
    IDirectDrawSurface4_Release(inner);
    return hr;
}

static HRESULT WINAPI ddrawex3_GetGDISurface(IDirectDraw3 *iface, IDirectDrawSurface **gdi_surface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);
    IDirectDrawSurface4 *surf4;
    HRESULT hr;

    TRACE("iface %p, gdi_surface %p.\n", iface, gdi_surface);

    if (FAILED(hr = ddrawex4_GetGDISurface(&ddrawex->IDirectDraw4_iface, &surf4)))
    {
        *gdi_surface = NULL;
        return hr;
    }

    IDirectDrawSurface4_QueryInterface(surf4, &IID_IDirectDrawSurface, (void **)gdi_surface);
    IDirectDrawSurface4_Release(surf4);
    return hr;
}

static HRESULT WINAPI ddrawex2_GetGDISurface(IDirectDraw2 *iface, IDirectDrawSurface **gdi_surface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, gdi_surface %p.\n", iface, gdi_surface);

    return ddrawex3_GetGDISurface(&ddrawex->IDirectDraw3_iface, gdi_surface);
}

static HRESULT WINAPI ddrawex1_GetGDISurface(IDirectDraw *iface, IDirectDrawSurface **gdi_surface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw(iface);

    TRACE("iface %p, gdi_surface %p.\n", iface, gdi_surface);

    return ddrawex3_GetGDISurface(&ddrawex->IDirectDraw3_iface, gdi_surface);
}

static HRESULT WINAPI ddrawex4_GetMonitorFrequency(IDirectDraw4 *iface, DWORD *frequency)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, frequency %p.\n", iface, frequency);

    return IDirectDraw4_GetMonitorFrequency(ddrawex->parent, frequency);
}

static HRESULT WINAPI ddrawex3_GetMonitorFrequency(IDirectDraw3 *iface, DWORD *frequency)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, frequency %p.\n", iface, frequency);

    return ddrawex4_GetMonitorFrequency(&ddrawex->IDirectDraw4_iface, frequency);
}

static HRESULT WINAPI ddrawex2_GetMonitorFrequency(IDirectDraw2 *iface, DWORD *frequency)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, frequency %p.\n", iface, frequency);

    return ddrawex4_GetMonitorFrequency(&ddrawex->IDirectDraw4_iface, frequency);
}

static HRESULT WINAPI ddrawex1_GetMonitorFrequency(IDirectDraw *iface, DWORD *frequency)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw(iface);

    TRACE("iface %p, frequency %p.\n", iface, frequency);

    return ddrawex4_GetMonitorFrequency(&ddrawex->IDirectDraw4_iface, frequency);
}

static HRESULT WINAPI ddrawex4_GetScanLine(IDirectDraw4 *iface, DWORD *line)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, line %p.\n", iface, line);

    return IDirectDraw4_GetScanLine(ddrawex->parent, line);
}

static HRESULT WINAPI ddrawex3_GetScanLine(IDirectDraw3 *iface, DWORD *line)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, line %p.\n", iface, line);

    return ddrawex4_GetScanLine(&ddrawex->IDirectDraw4_iface, line);
}

static HRESULT WINAPI ddrawex2_GetScanLine(IDirectDraw2 *iface, DWORD *line)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, line %p.\n", iface, line);

    return ddrawex4_GetScanLine(&ddrawex->IDirectDraw4_iface, line);
}

static HRESULT WINAPI ddrawex1_GetScanLine(IDirectDraw *iface, DWORD *line)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw(iface);

    TRACE("iface %p, line %p.\n", iface, line);

    return ddrawex4_GetScanLine(&ddrawex->IDirectDraw4_iface, line);
}

static HRESULT WINAPI ddrawex4_GetVerticalBlankStatus(IDirectDraw4 *iface, BOOL *status)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, status %p.\n", iface, status);

    return IDirectDraw4_GetVerticalBlankStatus(ddrawex->parent, status);
}

static HRESULT WINAPI ddrawex3_GetVerticalBlankStatus(IDirectDraw3 *iface, BOOL *status)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, status %p.\n", iface, status);

    return ddrawex4_GetVerticalBlankStatus(&ddrawex->IDirectDraw4_iface, status);
}

static HRESULT WINAPI ddrawex2_GetVerticalBlankStatus(IDirectDraw2 *iface, BOOL *status)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, status %p.\n", iface, status);

    return ddrawex4_GetVerticalBlankStatus(&ddrawex->IDirectDraw4_iface, status);
}

static HRESULT WINAPI ddrawex1_GetVerticalBlankStatus(IDirectDraw *iface, BOOL *status)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw(iface);

    TRACE("iface %p, status %p.\n", iface, status);

    return ddrawex4_GetVerticalBlankStatus(&ddrawex->IDirectDraw4_iface, status);
}

static HRESULT WINAPI ddrawex4_Initialize(IDirectDraw4 *iface, GUID *guid)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    return IDirectDraw4_Initialize(ddrawex->parent, guid);
}

static HRESULT WINAPI ddrawex3_Initialize(IDirectDraw3 *iface, GUID *guid)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    return ddrawex4_Initialize(&ddrawex->IDirectDraw4_iface, guid);
}

static HRESULT WINAPI ddrawex2_Initialize(IDirectDraw2 *iface, GUID *guid)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    return ddrawex4_Initialize(&ddrawex->IDirectDraw4_iface, guid);
}

static HRESULT WINAPI ddrawex1_Initialize(IDirectDraw *iface, GUID *guid)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw(iface);

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    return ddrawex4_Initialize(&ddrawex->IDirectDraw4_iface, guid);
}

static HRESULT WINAPI ddrawex4_RestoreDisplayMode(IDirectDraw4 *iface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);

    TRACE("iface %p.\n", iface);

    return IDirectDraw4_RestoreDisplayMode(ddrawex->parent);
}

static HRESULT WINAPI ddrawex3_RestoreDisplayMode(IDirectDraw3 *iface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);

    TRACE("iface %p.\n", iface);

    return ddrawex4_RestoreDisplayMode(&ddrawex->IDirectDraw4_iface);
}

static HRESULT WINAPI ddrawex2_RestoreDisplayMode(IDirectDraw2 *iface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw2(iface);

    TRACE("iface %p.\n", iface);

    return ddrawex4_RestoreDisplayMode(&ddrawex->IDirectDraw4_iface);
}

static HRESULT WINAPI ddrawex1_RestoreDisplayMode(IDirectDraw *iface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw(iface);

    TRACE("iface %p.\n", iface);

    return ddrawex4_RestoreDisplayMode(&ddrawex->IDirectDraw4_iface);
}

static HRESULT WINAPI ddrawex4_SetCooperativeLevel(IDirectDraw4 *iface, HWND window, DWORD flags)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, window %p, flags %#lx.\n", iface, window, flags);

    return IDirectDraw4_SetCooperativeLevel(ddrawex->parent, window, flags);
}

static HRESULT WINAPI ddrawex3_SetCooperativeLevel(IDirectDraw3 *iface, HWND window, DWORD flags)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, window %p, flags %#lx.\n", iface, window, flags);

    return ddrawex4_SetCooperativeLevel(&ddrawex->IDirectDraw4_iface, window, flags);
}

static HRESULT WINAPI ddrawex2_SetCooperativeLevel(IDirectDraw2 *iface, HWND window, DWORD flags)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, window %p, flags %#lx.\n", iface, window, flags);

    return ddrawex4_SetCooperativeLevel(&ddrawex->IDirectDraw4_iface, window, flags);
}

static HRESULT WINAPI ddrawex1_SetCooperativeLevel(IDirectDraw *iface, HWND window, DWORD flags)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw(iface);

    TRACE("iface %p, window %p, flags %#lx.\n", iface, window, flags);

    return ddrawex4_SetCooperativeLevel(&ddrawex->IDirectDraw4_iface, window, flags);
}

static HRESULT WINAPI ddrawex4_SetDisplayMode(IDirectDraw4 *iface, DWORD width,
        DWORD height, DWORD bpp, DWORD refresh_rate, DWORD flags)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, width %lu, height %lu, bpp %lu, refresh_rate %lu, flags %#lx.\n",
            iface, width, height, bpp, refresh_rate, flags);

    return IDirectDraw4_SetDisplayMode(ddrawex->parent, width, height, bpp, refresh_rate, flags);
}

static HRESULT WINAPI ddrawex3_SetDisplayMode(IDirectDraw3 *iface, DWORD width,
        DWORD height, DWORD bpp, DWORD refresh_rate, DWORD flags)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, width %lu, height %lu, bpp %lu, refresh_rate %lu, flags %#lx.\n",
            iface, width, height, bpp, refresh_rate, flags);

    return ddrawex4_SetDisplayMode(&ddrawex->IDirectDraw4_iface, width, height, bpp, refresh_rate, flags);
}

static HRESULT WINAPI ddrawex2_SetDisplayMode(IDirectDraw2 *iface, DWORD width,
        DWORD height, DWORD bpp, DWORD refresh_rate, DWORD flags)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, width %lu, height %lu, bpp %lu, refresh_rate %lu, flags %#lx.\n",
            iface, width, height, bpp, refresh_rate, flags);

    return ddrawex4_SetDisplayMode(&ddrawex->IDirectDraw4_iface, width, height, bpp, refresh_rate, flags);
}

static HRESULT WINAPI ddrawex1_SetDisplayMode(IDirectDraw *iface, DWORD width,
        DWORD height, DWORD bpp)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw(iface);

    TRACE("iface %p, width %lu, height %lu, bpp %lu.\n", iface, width, height, bpp);

    return ddrawex4_SetDisplayMode(&ddrawex->IDirectDraw4_iface, width, height, bpp, 0, 0);
}

static HRESULT WINAPI ddrawex4_WaitForVerticalBlank(IDirectDraw4 *iface, DWORD flags, HANDLE event)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, flags %#lx, event %p.\n", iface, flags, event);

    return IDirectDraw4_WaitForVerticalBlank(ddrawex->parent, flags, event);
}

static HRESULT WINAPI ddrawex3_WaitForVerticalBlank(IDirectDraw3 *iface, DWORD flags, HANDLE event)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);

    TRACE("iface %p, flags %#lx, event %p.\n", iface, flags, event);

    return ddrawex4_WaitForVerticalBlank(&ddrawex->IDirectDraw4_iface, flags, event);
}

static HRESULT WINAPI ddrawex2_WaitForVerticalBlank(IDirectDraw2 *iface, DWORD flags, HANDLE event)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw2(iface);

    TRACE("iface %p, flags %#lx, event %p.\n", iface, flags, event);

    return ddrawex4_WaitForVerticalBlank(&ddrawex->IDirectDraw4_iface, flags, event);
}

static HRESULT WINAPI ddrawex1_WaitForVerticalBlank(IDirectDraw *iface, DWORD flags, HANDLE event)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw(iface);

    TRACE("iface %p, flags %#lx, event %p.\n", iface, flags, event);

    return ddrawex4_WaitForVerticalBlank(&ddrawex->IDirectDraw4_iface, flags, event);
}

static HRESULT WINAPI ddrawex4_GetAvailableVidMem(IDirectDraw4 *iface,
        DDSCAPS2 *caps, DWORD *total, DWORD *free)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, caps %p, total %p, free %p.\n", iface, caps, total, free);

    return IDirectDraw4_GetAvailableVidMem(ddrawex->parent, caps, total, free);
}

static HRESULT WINAPI ddrawex3_GetAvailableVidMem(IDirectDraw3 *iface,
        DDSCAPS *caps, DWORD *total, DWORD *free)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);
    DDSCAPS2 caps2;

    TRACE("iface %p, caps %p, total %p, free %p.\n", iface, caps, total, free);

    memset(&caps2, 0, sizeof(caps2));
    caps2.dwCaps = caps->dwCaps;
    return ddrawex4_GetAvailableVidMem(&ddrawex->IDirectDraw4_iface, &caps2, total, free);
}

static HRESULT WINAPI ddrawex2_GetAvailableVidMem(IDirectDraw2 *iface,
        DDSCAPS *caps, DWORD *total, DWORD *free)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw2(iface);
    DDSCAPS2 caps2;

    TRACE("iface %p, caps %p, total %p, free %p.\n", iface, caps, total, free);

    memset(&caps2, 0, sizeof(caps2));
    caps2.dwCaps = caps->dwCaps;
    return ddrawex4_GetAvailableVidMem(&ddrawex->IDirectDraw4_iface, &caps2, total, free);
}

static HRESULT WINAPI ddrawex4_GetSurfaceFromDC(IDirectDraw4 *iface,
        HDC dc, IDirectDrawSurface4 **surface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, dc %p, surface %p.\n", iface, dc, surface);

    return IDirectDraw4_GetSurfaceFromDC(ddrawex->parent, dc, surface);
}

static HRESULT WINAPI ddrawex3_GetSurfaceFromDC(IDirectDraw3 *iface,
        HDC dc, IDirectDrawSurface **surface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw3(iface);
    IDirectDrawSurface4 *surf4, *outer;
    IDirectDrawSurface *inner;
    HRESULT hr;

    TRACE("iface %p, dc %p, surface %p.\n", iface, dc, surface);

    if (!surface)
        return E_POINTER;

    if (FAILED(hr = IDirectDraw4_GetSurfaceFromDC(ddrawex->parent, dc, (IDirectDrawSurface4 **)&inner)))
    {
        *surface = NULL;
        return hr;
    }

    hr = IDirectDrawSurface_QueryInterface(inner, &IID_IDirectDrawSurface4, (void **)&surf4);
    IDirectDrawSurface_Release(inner);
    if (FAILED(hr))
    {
        *surface = NULL;
        return hr;
    }

    outer = dds_get_outer(surf4);
    hr = IDirectDrawSurface4_QueryInterface(outer, &IID_IDirectDrawSurface, (void **)surface);
    IDirectDrawSurface4_Release(surf4);
    return hr;
}

static HRESULT WINAPI ddrawex4_RestoreAllSurfaces(IDirectDraw4 *iface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);

    TRACE("iface %p.\n", iface);

    return IDirectDraw4_RestoreAllSurfaces(ddrawex->parent);
}

static HRESULT WINAPI ddrawex4_TestCooperativeLevel(IDirectDraw4 *iface)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);

    TRACE("iface %p.\n", iface);

    return IDirectDraw4_TestCooperativeLevel(ddrawex->parent);
}

static HRESULT WINAPI ddrawex4_GetDeviceIdentifier(IDirectDraw4 *iface,
        DDDEVICEIDENTIFIER *identifier, DWORD flags)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(iface);

    TRACE("iface %p, identifier %p, flags %#lx.\n", iface, identifier, flags);

    return IDirectDraw4_GetDeviceIdentifier(ddrawex->parent, identifier, flags);
}

static const IDirectDraw4Vtbl ddrawex4_vtbl =
{
    ddrawex4_QueryInterface,
    ddrawex4_AddRef,
    ddrawex4_Release,
    ddrawex4_Compact,
    ddrawex4_CreateClipper,
    ddrawex4_CreatePalette,
    ddrawex4_CreateSurface,
    ddrawex4_DuplicateSurface,
    ddrawex4_EnumDisplayModes,
    ddrawex4_EnumSurfaces,
    ddrawex4_FlipToGDISurface,
    ddrawex4_GetCaps,
    ddrawex4_GetDisplayMode,
    ddrawex4_GetFourCCCodes,
    ddrawex4_GetGDISurface,
    ddrawex4_GetMonitorFrequency,
    ddrawex4_GetScanLine,
    ddrawex4_GetVerticalBlankStatus,
    ddrawex4_Initialize,
    ddrawex4_RestoreDisplayMode,
    ddrawex4_SetCooperativeLevel,
    ddrawex4_SetDisplayMode,
    ddrawex4_WaitForVerticalBlank,
    ddrawex4_GetAvailableVidMem,
    ddrawex4_GetSurfaceFromDC,
    ddrawex4_RestoreAllSurfaces,
    ddrawex4_TestCooperativeLevel,
    ddrawex4_GetDeviceIdentifier,
};

static const IDirectDraw3Vtbl ddrawex3_vtbl =
{
    ddrawex3_QueryInterface,
    ddrawex3_AddRef,
    ddrawex3_Release,
    ddrawex3_Compact,
    ddrawex3_CreateClipper,
    ddrawex3_CreatePalette,
    ddrawex3_CreateSurface,
    ddrawex3_DuplicateSurface,
    ddrawex3_EnumDisplayModes,
    ddrawex3_EnumSurfaces,
    ddrawex3_FlipToGDISurface,
    ddrawex3_GetCaps,
    ddrawex3_GetDisplayMode,
    ddrawex3_GetFourCCCodes,
    ddrawex3_GetGDISurface,
    ddrawex3_GetMonitorFrequency,
    ddrawex3_GetScanLine,
    ddrawex3_GetVerticalBlankStatus,
    ddrawex3_Initialize,
    ddrawex3_RestoreDisplayMode,
    ddrawex3_SetCooperativeLevel,
    ddrawex3_SetDisplayMode,
    ddrawex3_WaitForVerticalBlank,
    ddrawex3_GetAvailableVidMem,
    ddrawex3_GetSurfaceFromDC,
};

static const IDirectDraw2Vtbl ddrawex2_vtbl =
{
    ddrawex2_QueryInterface,
    ddrawex2_AddRef,
    ddrawex2_Release,
    ddrawex2_Compact,
    ddrawex2_CreateClipper,
    ddrawex2_CreatePalette,
    ddrawex2_CreateSurface,
    ddrawex2_DuplicateSurface,
    ddrawex2_EnumDisplayModes,
    ddrawex2_EnumSurfaces,
    ddrawex2_FlipToGDISurface,
    ddrawex2_GetCaps,
    ddrawex2_GetDisplayMode,
    ddrawex2_GetFourCCCodes,
    ddrawex2_GetGDISurface,
    ddrawex2_GetMonitorFrequency,
    ddrawex2_GetScanLine,
    ddrawex2_GetVerticalBlankStatus,
    ddrawex2_Initialize,
    ddrawex2_RestoreDisplayMode,
    ddrawex2_SetCooperativeLevel,
    ddrawex2_SetDisplayMode,
    ddrawex2_WaitForVerticalBlank,
    ddrawex2_GetAvailableVidMem,
};

static const IDirectDrawVtbl ddrawex1_vtbl =
{
    ddrawex1_QueryInterface,
    ddrawex1_AddRef,
    ddrawex1_Release,
    ddrawex1_Compact,
    ddrawex1_CreateClipper,
    ddrawex1_CreatePalette,
    ddrawex1_CreateSurface,
    ddrawex1_DuplicateSurface,
    ddrawex1_EnumDisplayModes,
    ddrawex1_EnumSurfaces,
    ddrawex1_FlipToGDISurface,
    ddrawex1_GetCaps,
    ddrawex1_GetDisplayMode,
    ddrawex1_GetFourCCCodes,
    ddrawex1_GetGDISurface,
    ddrawex1_GetMonitorFrequency,
    ddrawex1_GetScanLine,
    ddrawex1_GetVerticalBlankStatus,
    ddrawex1_Initialize,
    ddrawex1_RestoreDisplayMode,
    ddrawex1_SetCooperativeLevel,
    ddrawex1_SetDisplayMode,
    ddrawex1_WaitForVerticalBlank,
};

HRESULT WINAPI ddrawex_factory_CreateDirectDraw(IDirectDrawFactory *iface, GUID *guid, HWND window,
        DWORD coop_level, DWORD reserved, IUnknown *outer_unknown, IDirectDraw **ddraw)
{
    IDirectDraw *parent = NULL;
    struct ddrawex *object;
    HRESULT hr;

    TRACE("iface %p, guid %s, window %p, coop_level %#lx, reserved %#lx, outer_unknown %p, ddraw %p.\n",
            iface, debugstr_guid(guid), window, coop_level, reserved, outer_unknown, ddraw);

    if (outer_unknown)
        FIXME("Implement aggregation in ddrawex's IDirectDraw interface.\n");

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->ref = 1;
    object->IDirectDraw_iface.lpVtbl = &ddrawex1_vtbl;
    object->IDirectDraw2_iface.lpVtbl = &ddrawex2_vtbl;
    object->IDirectDraw3_iface.lpVtbl = &ddrawex3_vtbl;
    object->IDirectDraw4_iface.lpVtbl = &ddrawex4_vtbl;

    if (FAILED(hr = DirectDrawCreate(guid, &parent, NULL)))
        goto err;
    if (FAILED(hr = IDirectDraw_QueryInterface(parent, &IID_IDirectDraw4, (void **)&object->parent)))
        goto err;
    if (FAILED(hr = IDirectDraw_SetCooperativeLevel(&object->IDirectDraw_iface, window, coop_level)))
        goto err;

    *ddraw = &object->IDirectDraw_iface;
    IDirectDraw_Release(parent);
    return DD_OK;

err:
    if (object && object->parent)
        IDirectDraw4_Release(object->parent);
    if (parent)
        IDirectDraw_Release(parent);
    free(object);
    *ddraw = NULL;
    return hr;
}

IDirectDraw4 *dd_get_inner(IDirectDraw4 *outer)
{
    struct ddrawex *ddrawex = impl_from_IDirectDraw4(outer);

    if (outer->lpVtbl != &ddrawex4_vtbl)
        return NULL;
    return ddrawex->parent;
}
