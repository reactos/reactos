/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
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
 *
 */

#include "config.h"
#include "wine/port.h"

#include "dxgi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dxgi);

/* Inner IUnknown methods */

static inline struct dxgi_surface *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct dxgi_surface, IUnknown_iface);
}

static HRESULT STDMETHODCALLTYPE dxgi_surface_inner_QueryInterface(IUnknown *iface, REFIID riid, void **out)
{
    struct dxgi_surface *surface = impl_from_IUnknown(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDXGISurface1)
            || IsEqualGUID(riid, &IID_IDXGISurface)
            || IsEqualGUID(riid, &IID_IDXGIDeviceSubObject)
            || IsEqualGUID(riid, &IID_IDXGIObject)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDXGISurface1_AddRef(&surface->IDXGISurface1_iface);
        *out = &surface->IDXGISurface1_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE dxgi_surface_inner_AddRef(IUnknown *iface)
{
    struct dxgi_surface *surface = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedIncrement(&surface->refcount);

    TRACE("%p increasing refcount to %u.\n", surface, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE dxgi_surface_inner_Release(IUnknown *iface)
{
    struct dxgi_surface *surface = impl_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&surface->refcount);

    TRACE("%p decreasing refcount to %u.\n", surface, refcount);

    if (!refcount)
    {
        wined3d_private_store_cleanup(&surface->private_store);
        heap_free(surface);
    }

    return refcount;
}

static inline struct dxgi_surface *impl_from_IDXGISurface1(IDXGISurface1 *iface)
{
    return CONTAINING_RECORD(iface, struct dxgi_surface, IDXGISurface1_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE dxgi_surface_QueryInterface(IDXGISurface1 *iface, REFIID riid,
        void **object)
{
    struct dxgi_surface *surface = impl_from_IDXGISurface1(iface);
    TRACE("Forwarding to outer IUnknown\n");
    return IUnknown_QueryInterface(surface->outer_unknown, riid, object);
}

static ULONG STDMETHODCALLTYPE dxgi_surface_AddRef(IDXGISurface1 *iface)
{
    struct dxgi_surface *surface = impl_from_IDXGISurface1(iface);
    TRACE("Forwarding to outer IUnknown\n");
    return IUnknown_AddRef(surface->outer_unknown);
}

static ULONG STDMETHODCALLTYPE dxgi_surface_Release(IDXGISurface1 *iface)
{
    struct dxgi_surface *surface = impl_from_IDXGISurface1(iface);
    TRACE("Forwarding to outer IUnknown\n");
    return IUnknown_Release(surface->outer_unknown);
}

/* IDXGIObject methods */

static HRESULT STDMETHODCALLTYPE dxgi_surface_SetPrivateData(IDXGISurface1 *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct dxgi_surface *surface = impl_from_IDXGISurface1(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_set_private_data(&surface->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_surface_SetPrivateDataInterface(IDXGISurface1 *iface,
        REFGUID guid, const IUnknown *object)
{
    struct dxgi_surface *surface = impl_from_IDXGISurface1(iface);

    TRACE("iface %p, guid %s, object %p.\n", iface, debugstr_guid(guid), object);

    return dxgi_set_private_data_interface(&surface->private_store, guid, object);
}

static HRESULT STDMETHODCALLTYPE dxgi_surface_GetPrivateData(IDXGISurface1 *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct dxgi_surface *surface = impl_from_IDXGISurface1(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_get_private_data(&surface->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_surface_GetParent(IDXGISurface1 *iface, REFIID riid, void **parent)
{
    struct dxgi_surface *surface = impl_from_IDXGISurface1(iface);

    TRACE("iface %p, riid %s, parent %p.\n", iface, debugstr_guid(riid), parent);

    return IDXGIDevice_QueryInterface(surface->device, riid, parent);
}

/* IDXGIDeviceSubObject methods */

static HRESULT STDMETHODCALLTYPE dxgi_surface_GetDevice(IDXGISurface1 *iface, REFIID riid, void **device)
{
    struct dxgi_surface *surface = impl_from_IDXGISurface1(iface);

    TRACE("iface %p, riid %s, device %p.\n", iface, debugstr_guid(riid), device);

    return IDXGIDevice_QueryInterface(surface->device, riid, device);
}

/* IDXGISurface methods */
static HRESULT STDMETHODCALLTYPE dxgi_surface_GetDesc(IDXGISurface1 *iface, DXGI_SURFACE_DESC *desc)
{
    struct dxgi_surface *surface = impl_from_IDXGISurface1(iface);
    struct wined3d_resource_desc wined3d_desc;

    TRACE("iface %p, desc %p.\n", iface, desc);

    wined3d_mutex_lock();
    wined3d_resource_get_desc(wined3d_texture_get_resource(surface->wined3d_texture), &wined3d_desc);
    wined3d_mutex_unlock();
    desc->Width = wined3d_desc.width;
    desc->Height = wined3d_desc.height;
    desc->Format = dxgi_format_from_wined3dformat(wined3d_desc.format);
    dxgi_sample_desc_from_wined3d(&desc->SampleDesc, wined3d_desc.multisample_type, wined3d_desc.multisample_quality);

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_surface_Map(IDXGISurface1 *iface, DXGI_MAPPED_RECT *mapped_rect, UINT flags)
{
    struct dxgi_surface *surface = impl_from_IDXGISurface1(iface);
    struct wined3d_map_desc wined3d_map_desc;
    DWORD wined3d_map_flags = 0;
    HRESULT hr;

    TRACE("iface %p, mapped_rect %p, flags %#x.\n", iface, mapped_rect, flags);

    if (flags & DXGI_MAP_READ)
        wined3d_map_flags |= WINED3D_MAP_READ;
    if (flags & DXGI_MAP_WRITE)
        wined3d_map_flags |= WINED3D_MAP_WRITE;
    if (flags & DXGI_MAP_DISCARD)
        wined3d_map_flags |= WINED3D_MAP_DISCARD;

    wined3d_mutex_lock();
    if (SUCCEEDED(hr = wined3d_resource_map(wined3d_texture_get_resource(surface->wined3d_texture), 0,
            &wined3d_map_desc, NULL, wined3d_map_flags)))
    {
        mapped_rect->Pitch = wined3d_map_desc.row_pitch;
        mapped_rect->pBits = wined3d_map_desc.data;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_surface_Unmap(IDXGISurface1 *iface)
{
    struct dxgi_surface *surface = impl_from_IDXGISurface1(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_resource_unmap(wined3d_texture_get_resource(surface->wined3d_texture), 0);
    wined3d_mutex_unlock();

    return S_OK;
}

/* IDXGISurface1 methods */
static HRESULT STDMETHODCALLTYPE dxgi_surface_GetDC(IDXGISurface1 *iface, BOOL discard, HDC *hdc)
{
    struct dxgi_surface *surface = impl_from_IDXGISurface1(iface);
    HRESULT hr;

    FIXME("iface %p, discard %d, hdc %p semi-stub!\n", iface, discard, hdc);

    if (!hdc)
        return E_INVALIDARG;

    wined3d_mutex_lock();
    hr = wined3d_texture_get_dc(surface->wined3d_texture, 0, hdc);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr))
       surface->dc = *hdc;

    return hr;
}

static HRESULT STDMETHODCALLTYPE dxgi_surface_ReleaseDC(IDXGISurface1 *iface, RECT *dirty_rect)
{
    struct dxgi_surface *surface = impl_from_IDXGISurface1(iface);
    HRESULT hr;

    TRACE("iface %p, rect %s\n", iface, wine_dbgstr_rect(dirty_rect));

    if (!IsRectEmpty(dirty_rect))
        FIXME("dirty rectangle is ignored.\n");

    wined3d_mutex_lock();
    hr = wined3d_texture_release_dc(surface->wined3d_texture, 0, surface->dc);
    wined3d_mutex_unlock();

    return hr;
}

static const struct IDXGISurface1Vtbl dxgi_surface_vtbl =
{
    /* IUnknown methods */
    dxgi_surface_QueryInterface,
    dxgi_surface_AddRef,
    dxgi_surface_Release,
    /* IDXGIObject methods */
    dxgi_surface_SetPrivateData,
    dxgi_surface_SetPrivateDataInterface,
    dxgi_surface_GetPrivateData,
    dxgi_surface_GetParent,
    /* IDXGIDeviceSubObject methods */
    dxgi_surface_GetDevice,
    /* IDXGISurface methods */
    dxgi_surface_GetDesc,
    dxgi_surface_Map,
    dxgi_surface_Unmap,
    /* IDXGISurface1 methods */
    dxgi_surface_GetDC,
    dxgi_surface_ReleaseDC,
};

static const struct IUnknownVtbl dxgi_surface_inner_unknown_vtbl =
{
    /* IUnknown methods */
    dxgi_surface_inner_QueryInterface,
    dxgi_surface_inner_AddRef,
    dxgi_surface_inner_Release,
};

HRESULT dxgi_surface_init(struct dxgi_surface *surface, IDXGIDevice *device,
        IUnknown *outer, struct wined3d_texture *wined3d_texture)
{
    surface->IDXGISurface1_iface.lpVtbl = &dxgi_surface_vtbl;
    surface->IUnknown_iface.lpVtbl = &dxgi_surface_inner_unknown_vtbl;
    surface->refcount = 1;
    wined3d_private_store_init(&surface->private_store);
    surface->outer_unknown = outer ? outer : &surface->IUnknown_iface;
    surface->device = device;
    surface->wined3d_texture = wined3d_texture;
    surface->dc = NULL;

    return S_OK;
}
