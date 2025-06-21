/*
 * IDirect3DSurface9 implementation
 *
 * Copyright 2002-2005 Jason Edmeades
 *                     Raphael Junqueira
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

#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

static inline struct d3d9_surface *impl_from_IDirect3DSurface9(IDirect3DSurface9 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d9_surface, IDirect3DSurface9_iface);
}

static HRESULT WINAPI d3d9_surface_QueryInterface(IDirect3DSurface9 *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DSurface9)
            || IsEqualGUID(riid, &IID_IDirect3DResource9)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DSurface9_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d9_surface_AddRef(IDirect3DSurface9 *iface)
{
    struct d3d9_surface *surface = impl_from_IDirect3DSurface9(iface);
    ULONG refcount;

    TRACE("iface %p.\n", iface);

    if (surface->texture)
    {
        TRACE("Forwarding to %p.\n", surface->texture);
        return IDirect3DBaseTexture9_AddRef(&surface->texture->IDirect3DBaseTexture9_iface);
    }

    refcount = InterlockedIncrement(&surface->resource.refcount);
    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    if (refcount == 1)
    {
        if (surface->parent_device)
            IDirect3DDevice9Ex_AddRef(surface->parent_device);
        if (surface->wined3d_rtv)
            wined3d_rendertarget_view_incref(surface->wined3d_rtv);
        wined3d_texture_incref(surface->wined3d_texture);
        if (surface->swapchain)
            wined3d_swapchain_incref(surface->swapchain);
    }

    return refcount;
}

static ULONG WINAPI d3d9_surface_Release(IDirect3DSurface9 *iface)
{
    struct d3d9_surface *surface = impl_from_IDirect3DSurface9(iface);
    ULONG refcount;

    TRACE("iface %p.\n", iface);

    if (surface->texture)
    {
        TRACE("Forwarding to %p.\n", surface->texture);
        return IDirect3DBaseTexture9_Release(&surface->texture->IDirect3DBaseTexture9_iface);
    }

    if (!surface->resource.refcount)
    {
        WARN("Surface does not have any references.\n");
        return 0;
    }

    refcount = InterlockedDecrement(&surface->resource.refcount);
    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        IDirect3DDevice9Ex *parent_device = surface->parent_device;

        if (surface->wined3d_rtv)
            wined3d_rendertarget_view_decref(surface->wined3d_rtv);
        if (surface->swapchain)
            wined3d_swapchain_decref(surface->swapchain);
        /* Releasing the texture may free the d3d9 object, so do not access it
         * after releasing the texture. */
        wined3d_texture_decref(surface->wined3d_texture);

        /* Release the device last, as it may cause the device to be destroyed. */
        if (parent_device)
            IDirect3DDevice9Ex_Release(parent_device);
    }

    return refcount;
}

static HRESULT WINAPI d3d9_surface_GetDevice(IDirect3DSurface9 *iface, IDirect3DDevice9 **device)
{
    struct d3d9_surface *surface = impl_from_IDirect3DSurface9(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    if (surface->texture)
        return IDirect3DBaseTexture9_GetDevice(&surface->texture->IDirect3DBaseTexture9_iface, device);

    *device = (IDirect3DDevice9 *)surface->parent_device;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI d3d9_surface_SetPrivateData(IDirect3DSurface9 *iface, REFGUID guid,
        const void *data, DWORD data_size, DWORD flags)
{
    struct d3d9_surface *surface = impl_from_IDirect3DSurface9(iface);
    TRACE("iface %p, guid %s, data %p, data_size %lu, flags %#lx.\n",
            iface, debugstr_guid(guid), data, data_size, flags);

    return d3d9_resource_set_private_data(&surface->resource, guid, data, data_size, flags);
}

static HRESULT WINAPI d3d9_surface_GetPrivateData(IDirect3DSurface9 *iface, REFGUID guid,
        void *data, DWORD *data_size)
{
    struct d3d9_surface *surface = impl_from_IDirect3DSurface9(iface);
    TRACE("iface %p, guid %s, data %p, data_size %p.\n",
            iface, debugstr_guid(guid), data, data_size);

    return d3d9_resource_get_private_data(&surface->resource, guid, data, data_size);
}

static HRESULT WINAPI d3d9_surface_FreePrivateData(IDirect3DSurface9 *iface, REFGUID guid)
{
    struct d3d9_surface *surface = impl_from_IDirect3DSurface9(iface);
    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    return d3d9_resource_free_private_data(&surface->resource, guid);
}

static DWORD WINAPI d3d9_surface_SetPriority(IDirect3DSurface9 *iface, DWORD priority)
{
    TRACE("iface %p, priority %lu. Ignored on surfaces.\n", iface, priority);
    return 0;
}

static DWORD WINAPI d3d9_surface_GetPriority(IDirect3DSurface9 *iface)
{
    TRACE("iface %p. Ignored on surfaces.\n", iface);
    return 0;
}

static void WINAPI d3d9_surface_PreLoad(IDirect3DSurface9 *iface)
{
    struct d3d9_surface *surface = impl_from_IDirect3DSurface9(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_resource_preload(wined3d_texture_get_resource(surface->wined3d_texture));
    wined3d_mutex_unlock();
}

static D3DRESOURCETYPE WINAPI d3d9_surface_GetType(IDirect3DSurface9 *iface)
{
    TRACE("iface %p.\n", iface);

    return D3DRTYPE_SURFACE;
}

static HRESULT WINAPI d3d9_surface_GetContainer(IDirect3DSurface9 *iface, REFIID riid, void **container)
{
    struct d3d9_surface *surface = impl_from_IDirect3DSurface9(iface);
    HRESULT hr;

    TRACE("iface %p, riid %s, container %p.\n", iface, debugstr_guid(riid), container);

    if (!surface->container)
        return E_NOINTERFACE;

    hr = IUnknown_QueryInterface(surface->container, riid, container);

    TRACE("Returning %p.\n", *container);

    return hr;
}

static HRESULT WINAPI d3d9_surface_GetDesc(IDirect3DSurface9 *iface, D3DSURFACE_DESC *desc)
{
    struct d3d9_surface *surface = impl_from_IDirect3DSurface9(iface);
    struct wined3d_sub_resource_desc wined3d_desc;

    TRACE("iface %p, desc %p.\n", iface, desc);

    wined3d_mutex_lock();
    wined3d_texture_get_sub_resource_desc(surface->wined3d_texture, surface->sub_resource_idx, &wined3d_desc);
    wined3d_mutex_unlock();

    desc->Format = d3dformat_from_wined3dformat(wined3d_desc.format);
    desc->Type = D3DRTYPE_SURFACE;
    desc->Usage = d3dusage_from_wined3dusage(wined3d_desc.usage, wined3d_desc.bind_flags);
    desc->Pool = d3dpool_from_wined3daccess(wined3d_desc.access, wined3d_desc.usage);
    desc->MultiSampleType = d3dmultisample_type_from_wined3d(wined3d_desc.multisample_type);
    desc->MultiSampleQuality = wined3d_desc.multisample_quality;
    desc->Width = wined3d_desc.width;
    desc->Height = wined3d_desc.height;

    return D3D_OK;
}

static HRESULT WINAPI d3d9_surface_LockRect(IDirect3DSurface9 *iface,
        D3DLOCKED_RECT *locked_rect, const RECT *rect, DWORD flags)
{
    struct d3d9_surface *surface = impl_from_IDirect3DSurface9(iface);
    struct wined3d_box box;
    struct wined3d_map_desc map_desc;
    HRESULT hr;

    TRACE("iface %p, locked_rect %p, rect %s, flags %#lx.\n",
            iface, locked_rect, wine_dbgstr_rect(rect), flags);

    if (rect)
        wined3d_box_set(&box, rect->left, rect->top, rect->right, rect->bottom, 0, 1);

    hr = wined3d_resource_map(wined3d_texture_get_resource(surface->wined3d_texture), surface->sub_resource_idx,
            &map_desc, rect ? &box : NULL, wined3dmapflags_from_d3dmapflags(flags, 0));

    if (SUCCEEDED(hr))
    {
        locked_rect->Pitch = map_desc.row_pitch;
        locked_rect->pBits = map_desc.data;
    }

    if (hr == E_INVALIDARG)
        return D3DERR_INVALIDCALL;
    return hr;
}

static HRESULT WINAPI d3d9_surface_UnlockRect(IDirect3DSurface9 *iface)
{
    struct d3d9_surface *surface = impl_from_IDirect3DSurface9(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_resource_unmap(wined3d_texture_get_resource(surface->wined3d_texture), surface->sub_resource_idx);
    if (SUCCEEDED(hr) && surface->texture)
        d3d9_texture_flag_auto_gen_mipmap(surface->texture);
    wined3d_mutex_unlock();

    if (hr == WINEDDERR_NOTLOCKED)
    {
        D3DRESOURCETYPE type;
        if (surface->texture)
            type = IDirect3DBaseTexture9_GetType(&surface->texture->IDirect3DBaseTexture9_iface);
        else
            type = D3DRTYPE_SURFACE;
        hr = type == D3DRTYPE_TEXTURE ? D3D_OK : D3DERR_INVALIDCALL;
    }
    return hr;
}

static HRESULT WINAPI d3d9_surface_GetDC(IDirect3DSurface9 *iface, HDC *dc)
{
    struct d3d9_surface *surface = impl_from_IDirect3DSurface9(iface);
    HRESULT hr;

    TRACE("iface %p, dc %p.\n", iface, dc);

    wined3d_mutex_lock();
    hr = wined3d_texture_get_dc(surface->wined3d_texture, surface->sub_resource_idx, dc);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_surface_ReleaseDC(IDirect3DSurface9 *iface, HDC dc)
{
    struct d3d9_surface *surface = impl_from_IDirect3DSurface9(iface);
    HRESULT hr;

    TRACE("iface %p, dc %p.\n", iface, dc);

    wined3d_mutex_lock();
    hr = wined3d_texture_release_dc(surface->wined3d_texture, surface->sub_resource_idx, dc);
    if (SUCCEEDED(hr) && surface->texture)
        d3d9_texture_flag_auto_gen_mipmap(surface->texture);
    wined3d_mutex_unlock();

    return hr;
}

static const struct IDirect3DSurface9Vtbl d3d9_surface_vtbl =
{
    /* IUnknown */
    d3d9_surface_QueryInterface,
    d3d9_surface_AddRef,
    d3d9_surface_Release,
    /* IDirect3DResource9 */
    d3d9_surface_GetDevice,
    d3d9_surface_SetPrivateData,
    d3d9_surface_GetPrivateData,
    d3d9_surface_FreePrivateData,
    d3d9_surface_SetPriority,
    d3d9_surface_GetPriority,
    d3d9_surface_PreLoad,
    d3d9_surface_GetType,
    /* IDirect3DSurface9 */
    d3d9_surface_GetContainer,
    d3d9_surface_GetDesc,
    d3d9_surface_LockRect,
    d3d9_surface_UnlockRect,
    d3d9_surface_GetDC,
    d3d9_surface_ReleaseDC,
};

static void STDMETHODCALLTYPE surface_wined3d_object_destroyed(void *parent)
{
    struct d3d9_surface *surface = parent;
    d3d9_resource_cleanup(&surface->resource);
    free(surface);
}

static const struct wined3d_parent_ops d3d9_surface_wined3d_parent_ops =
{
    surface_wined3d_object_destroyed,
};

struct d3d9_surface *d3d9_surface_create(struct wined3d_texture *wined3d_texture,
        unsigned int sub_resource_idx, IUnknown *container)
{
    IDirect3DBaseTexture9 *texture;
    struct d3d9_surface *surface;

    if (!(surface = calloc(1, sizeof(*surface))))
        return NULL;

    surface->IDirect3DSurface9_iface.lpVtbl = &d3d9_surface_vtbl;
    d3d9_resource_init(&surface->resource);
    surface->resource.refcount = 0;
    list_init(&surface->rtv_entry);
    surface->container = container;
    surface->wined3d_texture = wined3d_texture;
    surface->sub_resource_idx = sub_resource_idx;
    surface->swapchain = wined3d_texture_get_swapchain(wined3d_texture);

    if (surface->container && SUCCEEDED(IUnknown_QueryInterface(surface->container,
            &IID_IDirect3DBaseTexture9, (void **)&texture)))
    {
        surface->texture = unsafe_impl_from_IDirect3DBaseTexture9(texture);
        IDirect3DBaseTexture9_Release(texture);
    }

    wined3d_texture_set_sub_resource_parent(wined3d_texture, sub_resource_idx,
            surface, &d3d9_surface_wined3d_parent_ops);

    TRACE("Created surface %p.\n", surface);
    return surface;
}

static void STDMETHODCALLTYPE view_wined3d_object_destroyed(void *parent)
{
    struct d3d9_surface *surface = parent;

    /* If the surface reference count drops to zero, we release our reference
     * to the view, but don't clear the pointer yet, in case e.g. a
     * GetRenderTarget() call brings the surface back before the view is
     * actually destroyed. When the view is destroyed, we need to clear the
     * pointer, or a subsequent surface AddRef() would reference it again.
     *
     * This is safe because as long as the view still has a reference to the
     * texture, the surface is also still alive, and we're called before the
     * view releases that reference. */
    surface->wined3d_rtv = NULL;
    list_remove(&surface->rtv_entry);
}

static const struct wined3d_parent_ops d3d9_view_wined3d_parent_ops =
{
    view_wined3d_object_destroyed,
};

struct d3d9_device *d3d9_surface_get_device(const struct d3d9_surface *surface)
{
    IDirect3DDevice9Ex *device;
    device = surface->texture ? &surface->texture->parent_device->IDirect3DDevice9Ex_iface : surface->parent_device;
    return impl_from_IDirect3DDevice9Ex(device);
}

struct wined3d_rendertarget_view *d3d9_surface_acquire_rendertarget_view(struct d3d9_surface *surface)
{
    HRESULT hr;

    /* The surface reference count can be equal to 0 when this function is
     * called. In order to properly manage the render target view reference
     * count, we temporarily increment the surface reference count. */
    d3d9_surface_AddRef(&surface->IDirect3DSurface9_iface);

    if (surface->wined3d_rtv)
        return surface->wined3d_rtv;

    if (FAILED(hr = wined3d_rendertarget_view_create_from_sub_resource(surface->wined3d_texture,
            surface->sub_resource_idx, surface, &d3d9_view_wined3d_parent_ops, &surface->wined3d_rtv)))
    {
        ERR("Failed to create rendertarget view, hr %#lx.\n", hr);
        d3d9_surface_Release(&surface->IDirect3DSurface9_iface);
        return NULL;
    }

    if (surface->texture)
        list_add_head(&surface->texture->rtv_list, &surface->rtv_entry);

    return surface->wined3d_rtv;
}

void d3d9_surface_release_rendertarget_view(struct d3d9_surface *surface,
        struct wined3d_rendertarget_view *rtv)
{
    if (rtv)
        d3d9_surface_Release(&surface->IDirect3DSurface9_iface);
}

struct d3d9_surface *unsafe_impl_from_IDirect3DSurface9(IDirect3DSurface9 *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d9_surface_vtbl);

    return impl_from_IDirect3DSurface9(iface);
}
