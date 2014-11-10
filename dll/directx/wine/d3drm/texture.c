/*
 * Implementation of IDirect3DRMTextureX interfaces
 *
 * Copyright 2012 Christian Costa
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

#include "d3drm_private.h"

struct d3drm_texture
{
    IDirect3DRMTexture2 IDirect3DRMTexture2_iface;
    IDirect3DRMTexture3 IDirect3DRMTexture3_iface;
    LONG ref;
    DWORD app_data;
};

static inline struct d3drm_texture *impl_from_IDirect3DRMTexture2(IDirect3DRMTexture2 *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_texture, IDirect3DRMTexture2_iface);
}

static inline struct d3drm_texture *impl_from_IDirect3DRMTexture3(IDirect3DRMTexture3 *iface)
{
    return CONTAINING_RECORD(iface, struct d3drm_texture, IDirect3DRMTexture3_iface);
}

static HRESULT WINAPI d3drm_texture2_QueryInterface(IDirect3DRMTexture2 *iface, REFIID riid, void **out)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DRMTexture2)
            || IsEqualGUID(riid, &IID_IDirect3DRMTexture)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        *out = &texture->IDirect3DRMTexture2_iface;
    }
    else if (IsEqualGUID(riid, &IID_IDirect3DRMTexture3))
    {
        *out = &texture->IDirect3DRMTexture3_iface;
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

static ULONG WINAPI d3drm_texture2_AddRef(IDirect3DRMTexture2 *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);
    ULONG refcount = InterlockedIncrement(&texture->ref);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3drm_texture2_Release(IDirect3DRMTexture2 *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);
    ULONG refcount = InterlockedDecrement(&texture->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
        HeapFree(GetProcessHeap(), 0, texture);

    return refcount;
}

static HRESULT WINAPI d3drm_texture2_Clone(IDirect3DRMTexture2 *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, outer %p, iid %s, out %p stub!\n", iface, outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture2_AddDestroyCallback(IDirect3DRMTexture2 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture2_DeleteDestroyCallback(IDirect3DRMTexture2 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture2_SetAppData(IDirect3DRMTexture2 *iface, DWORD data)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p, data %#x.\n", iface, data);

    return IDirect3DRMTexture3_SetAppData(&texture->IDirect3DRMTexture3_iface, data);
}

static DWORD WINAPI d3drm_texture2_GetAppData(IDirect3DRMTexture2 *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DRMTexture3_GetAppData(&texture->IDirect3DRMTexture3_iface);
}

static HRESULT WINAPI d3drm_texture2_SetName(IDirect3DRMTexture2 *iface, const char *name)
{
    FIXME("iface %p, name %s stub!\n", iface, debugstr_a(name));

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture2_GetName(IDirect3DRMTexture2 *iface, DWORD *size, char *name)
{
    FIXME("iface %p, size %p, name %p stub!\n", iface, size, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture2_GetClassName(IDirect3DRMTexture2 *iface, DWORD *size, char *name)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture2(iface);

    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    return IDirect3DRMTexture3_GetClassName(&texture->IDirect3DRMTexture3_iface, size, name);
}

static HRESULT WINAPI d3drm_texture2_InitFromFile(IDirect3DRMTexture2 *iface, const char *filename)
{
    FIXME("iface %p, filename %s stub!\n", iface, debugstr_a(filename));

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture2_InitFromSurface(IDirect3DRMTexture2 *iface,
        IDirectDrawSurface *surface)
{
    FIXME("iface %p, surface %p stub!\n", iface, surface);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture2_InitFromResource(IDirect3DRMTexture2 *iface, HRSRC resource)
{
    FIXME("iface %p, resource %p stub!\n", iface, resource);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture2_Changed(IDirect3DRMTexture2 *iface, BOOL pixels, BOOL palette)
{
    FIXME("iface %p, pixels %#x, palette %#x stub!\n", iface, pixels, palette);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture2_SetColors(IDirect3DRMTexture2 *iface, DWORD max_colors)
{
    FIXME("iface %p, max_colors %u stub!\n", iface, max_colors);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture2_SetShades(IDirect3DRMTexture2 *iface, DWORD max_shades)
{
    FIXME("iface %p, max_shades %u stub!\n", iface, max_shades);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture2_SetDecalSize(IDirect3DRMTexture2 *iface, D3DVALUE width, D3DVALUE height)
{
    FIXME("iface %p, width %.8e, height %.8e stub!\n", iface, width, height);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture2_SetDecalOrigin(IDirect3DRMTexture2 *iface, LONG x, LONG y)
{
    FIXME("iface %p, x %d, y %d stub!\n", iface, x, y);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture2_SetDecalScale(IDirect3DRMTexture2 *iface, DWORD scale)
{
    FIXME("iface %p, scale %u stub!\n", iface, scale);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture2_SetDecalTransparency(IDirect3DRMTexture2 *iface, BOOL transparency)
{
    FIXME("iface %p, transparency %#x stub!\n", iface, transparency);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture2_SetDecalTransparentColor(IDirect3DRMTexture2 *iface, D3DCOLOR color)
{
    FIXME("iface %p, color 0x%08x stub!\n", iface, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture2_GetDecalSize(IDirect3DRMTexture2 *iface, D3DVALUE *width, D3DVALUE *height)
{
    FIXME("iface %p, width %p, height %p stub!\n", iface, width, height);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture2_GetDecalOrigin(IDirect3DRMTexture2 *iface, LONG *x, LONG *y)
{
    FIXME("iface %p, x %p, y %p stub!\n", iface, x, y);

    return E_NOTIMPL;
}

static D3DRMIMAGE * WINAPI d3drm_texture2_GetImage(IDirect3DRMTexture2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return NULL;
}

static DWORD WINAPI d3drm_texture2_GetShades(IDirect3DRMTexture2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static DWORD WINAPI d3drm_texture2_GetColors(IDirect3DRMTexture2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static DWORD WINAPI d3drm_texture2_GetDecalScale(IDirect3DRMTexture2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static BOOL WINAPI d3drm_texture2_GetDecalTransparency(IDirect3DRMTexture2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static D3DCOLOR WINAPI d3drm_texture2_GetDecalTransparentColor(IDirect3DRMTexture2 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT WINAPI d3drm_texture2_InitFromImage(IDirect3DRMTexture2 *iface, D3DRMIMAGE *image)
{
    FIXME("iface %p, image %p stub!\n", iface, image);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture2_InitFromResource2(IDirect3DRMTexture2 *iface,
        HMODULE module, const char *name, const char *type)
{
    FIXME("iface %p, module %p, name %s, type %s stub!\n",
            iface, module, debugstr_a(name), debugstr_a(type));

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture2_GenerateMIPMap(IDirect3DRMTexture2 *iface, DWORD flags)
{
    FIXME("iface %p, flags %#x stub!\n", iface, flags);

    return E_NOTIMPL;
}

static const struct IDirect3DRMTexture2Vtbl d3drm_texture2_vtbl =
{
    d3drm_texture2_QueryInterface,
    d3drm_texture2_AddRef,
    d3drm_texture2_Release,
    d3drm_texture2_Clone,
    d3drm_texture2_AddDestroyCallback,
    d3drm_texture2_DeleteDestroyCallback,
    d3drm_texture2_SetAppData,
    d3drm_texture2_GetAppData,
    d3drm_texture2_SetName,
    d3drm_texture2_GetName,
    d3drm_texture2_GetClassName,
    d3drm_texture2_InitFromFile,
    d3drm_texture2_InitFromSurface,
    d3drm_texture2_InitFromResource,
    d3drm_texture2_Changed,
    d3drm_texture2_SetColors,
    d3drm_texture2_SetShades,
    d3drm_texture2_SetDecalSize,
    d3drm_texture2_SetDecalOrigin,
    d3drm_texture2_SetDecalScale,
    d3drm_texture2_SetDecalTransparency,
    d3drm_texture2_SetDecalTransparentColor,
    d3drm_texture2_GetDecalSize,
    d3drm_texture2_GetDecalOrigin,
    d3drm_texture2_GetImage,
    d3drm_texture2_GetShades,
    d3drm_texture2_GetColors,
    d3drm_texture2_GetDecalScale,
    d3drm_texture2_GetDecalTransparency,
    d3drm_texture2_GetDecalTransparentColor,
    d3drm_texture2_InitFromImage,
    d3drm_texture2_InitFromResource2,
    d3drm_texture2_GenerateMIPMap,
};

static HRESULT WINAPI d3drm_texture3_QueryInterface(IDirect3DRMTexture3 *iface, REFIID riid, void **out)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DRMTexture2)
            || IsEqualGUID(riid, &IID_IDirect3DRMTexture)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        *out = &texture->IDirect3DRMTexture2_iface;
    }
    else if (IsEqualGUID(riid, &IID_IDirect3DRMTexture3))
    {
        *out = &texture->IDirect3DRMTexture3_iface;
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

static ULONG WINAPI d3drm_texture3_AddRef(IDirect3DRMTexture3 *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);
    ULONG refcount = InterlockedIncrement(&texture->ref);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3drm_texture3_Release(IDirect3DRMTexture3 *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);
    ULONG refcount = InterlockedDecrement(&texture->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
        HeapFree(GetProcessHeap(), 0, texture);

    return refcount;
}

static HRESULT WINAPI d3drm_texture3_Clone(IDirect3DRMTexture3 *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    FIXME("iface %p, outer %p, iid %s, out %p stub!\n", iface, outer, debugstr_guid(iid), out);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_AddDestroyCallback(IDirect3DRMTexture3 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_DeleteDestroyCallback(IDirect3DRMTexture3 *iface,
        D3DRMOBJECTCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_SetAppData(IDirect3DRMTexture3 *iface, DWORD data)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);

    TRACE("iface %p, data %#x.\n", iface, data);

    texture->app_data = data;

    return D3DRM_OK;
}

static DWORD WINAPI d3drm_texture3_GetAppData(IDirect3DRMTexture3 *iface)
{
    struct d3drm_texture *texture = impl_from_IDirect3DRMTexture3(iface);

    TRACE("iface %p.\n", iface);

    return texture->app_data;
}

static HRESULT WINAPI d3drm_texture3_SetName(IDirect3DRMTexture3 *iface, const char *name)
{
    FIXME("iface %p, name %s stub!\n", iface, debugstr_a(name));

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_GetName(IDirect3DRMTexture3 *iface, DWORD *size, char *name)
{
    FIXME("iface %p, size %p, name %p stub!\n", iface, size, name);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_GetClassName(IDirect3DRMTexture3 *iface, DWORD *size, char *name)
{
    TRACE("iface %p, size %p, name %p.\n", iface, size, name);

    if (!size || *size < strlen("Texture") || !name)
        return E_INVALIDARG;

    strcpy(name, "Texture");
    *size = sizeof("Texture");

    return D3DRM_OK;
}

static HRESULT WINAPI d3drm_texture3_InitFromFile(IDirect3DRMTexture3 *iface, const char *filename)
{
    FIXME("iface %p, filename %s stub!\n", iface, debugstr_a(filename));

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_InitFromSurface(IDirect3DRMTexture3 *iface,
        IDirectDrawSurface *surface)
{
    FIXME("iface %p, surface %p stub!\n", iface, surface);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_InitFromResource(IDirect3DRMTexture3 *iface, HRSRC resource)
{
    FIXME("iface %p, resource %p stub!\n", iface, resource);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_Changed(IDirect3DRMTexture3 *iface,
        DWORD flags, DWORD rect_count, RECT *rects)
{
    FIXME("iface %p, flags %#x, rect_count %u, rects %p stub!\n", iface, flags, rect_count, rects);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_SetColors(IDirect3DRMTexture3 *iface, DWORD max_colors)
{
    FIXME("iface %p, max_colors %u stub!\n", iface, max_colors);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_SetShades(IDirect3DRMTexture3 *iface, DWORD max_shades)
{
    FIXME("iface %p, max_shades %u stub!\n", iface, max_shades);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_SetDecalSize(IDirect3DRMTexture3 *iface, D3DVALUE width, D3DVALUE height)
{
    FIXME("iface %p, width %.8e, height %.8e stub!\n", iface, width, height);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_SetDecalOrigin(IDirect3DRMTexture3 *iface, LONG x, LONG y)
{
    FIXME("iface %p, x %d, y %d stub!\n", iface, x, y);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_SetDecalScale(IDirect3DRMTexture3 *iface, DWORD scale)
{
    FIXME("iface %p, scale %u stub!\n", iface, scale);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_SetDecalTransparency(IDirect3DRMTexture3 *iface, BOOL transparency)
{
    FIXME("iface %p, transparency %#x stub!\n", iface, transparency);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_SetDecalTransparentColor(IDirect3DRMTexture3 *iface, D3DCOLOR color)
{
    FIXME("iface %p, color 0x%08x stub!\n", iface, color);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_GetDecalSize(IDirect3DRMTexture3 *iface, D3DVALUE *width, D3DVALUE *height)
{
    FIXME("iface %p, width %p, height %p stub!\n", iface, width, height);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_GetDecalOrigin(IDirect3DRMTexture3 *iface, LONG *x, LONG *y)
{
    FIXME("iface %p, x %p, y %p stub!\n", iface, x, y);

    return E_NOTIMPL;
}

static D3DRMIMAGE * WINAPI d3drm_texture3_GetImage(IDirect3DRMTexture3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return NULL;
}

static DWORD WINAPI d3drm_texture3_GetShades(IDirect3DRMTexture3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static DWORD WINAPI d3drm_texture3_GetColors(IDirect3DRMTexture3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static DWORD WINAPI d3drm_texture3_GetDecalScale(IDirect3DRMTexture3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static BOOL WINAPI d3drm_texture3_GetDecalTransparency(IDirect3DRMTexture3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return FALSE;
}

static D3DCOLOR WINAPI d3drm_texture3_GetDecalTransparentColor(IDirect3DRMTexture3 *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT WINAPI d3drm_texture3_InitFromImage(IDirect3DRMTexture3 *iface, D3DRMIMAGE *image)
{
    FIXME("iface %p, image %p stub!\n", iface, image);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_InitFromResource2(IDirect3DRMTexture3 *iface,
        HMODULE module, const char *name, const char *type)
{
    FIXME("iface %p, module %p, name %s, type %s stub!\n",
            iface, module, debugstr_a(name), debugstr_a(type));

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_GenerateMIPMap(IDirect3DRMTexture3 *iface, DWORD flags)
{
    FIXME("iface %p, flags %#x stub!\n", iface, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_GetSurface(IDirect3DRMTexture3 *iface,
        DWORD flags, IDirectDrawSurface **surface)
{
    FIXME("iface %p, flags %#x, surface %p stub!\n", iface, flags, surface);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_SetCacheOptions(IDirect3DRMTexture3 *iface, LONG importance, DWORD flags)
{
    FIXME("iface %p, importance %d, flags %#x stub!\n", iface, importance, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_GetCacheOptions(IDirect3DRMTexture3 *iface,
        LONG *importance, DWORD *flags)
{
    FIXME("iface %p, importance %p, flags %p stub!\n", iface, importance, flags);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_SetDownsampleCallback(IDirect3DRMTexture3 *iface,
        D3DRMDOWNSAMPLECALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static HRESULT WINAPI d3drm_texture3_SetValidationCallback(IDirect3DRMTexture3 *iface,
        D3DRMVALIDATIONCALLBACK cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_NOTIMPL;
}

static const struct IDirect3DRMTexture3Vtbl d3drm_texture3_vtbl =
{
    d3drm_texture3_QueryInterface,
    d3drm_texture3_AddRef,
    d3drm_texture3_Release,
    d3drm_texture3_Clone,
    d3drm_texture3_AddDestroyCallback,
    d3drm_texture3_DeleteDestroyCallback,
    d3drm_texture3_SetAppData,
    d3drm_texture3_GetAppData,
    d3drm_texture3_SetName,
    d3drm_texture3_GetName,
    d3drm_texture3_GetClassName,
    d3drm_texture3_InitFromFile,
    d3drm_texture3_InitFromSurface,
    d3drm_texture3_InitFromResource,
    d3drm_texture3_Changed,
    d3drm_texture3_SetColors,
    d3drm_texture3_SetShades,
    d3drm_texture3_SetDecalSize,
    d3drm_texture3_SetDecalOrigin,
    d3drm_texture3_SetDecalScale,
    d3drm_texture3_SetDecalTransparency,
    d3drm_texture3_SetDecalTransparentColor,
    d3drm_texture3_GetDecalSize,
    d3drm_texture3_GetDecalOrigin,
    d3drm_texture3_GetImage,
    d3drm_texture3_GetShades,
    d3drm_texture3_GetColors,
    d3drm_texture3_GetDecalScale,
    d3drm_texture3_GetDecalTransparency,
    d3drm_texture3_GetDecalTransparentColor,
    d3drm_texture3_InitFromImage,
    d3drm_texture3_InitFromResource2,
    d3drm_texture3_GenerateMIPMap,
    d3drm_texture3_GetSurface,
    d3drm_texture3_SetCacheOptions,
    d3drm_texture3_GetCacheOptions,
    d3drm_texture3_SetDownsampleCallback,
    d3drm_texture3_SetValidationCallback,
};

HRESULT Direct3DRMTexture_create(REFIID riid, IUnknown **out)
{
    struct d3drm_texture *object;

    TRACE("riid %s, out %p.\n", debugstr_guid(riid), out);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDirect3DRMTexture2_iface.lpVtbl = &d3drm_texture2_vtbl;
    object->IDirect3DRMTexture3_iface.lpVtbl = &d3drm_texture3_vtbl;
    object->ref = 1;

    if (IsEqualGUID(riid, &IID_IDirect3DRMTexture3))
        *out = (IUnknown *)&object->IDirect3DRMTexture3_iface;
    else
        *out = (IUnknown *)&object->IDirect3DRMTexture2_iface;

    return S_OK;
}
