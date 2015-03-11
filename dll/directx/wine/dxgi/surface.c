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

#include "dxgi_private.h"

/* Inner IUnknown methods */

static inline struct dxgi_surface *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct dxgi_surface, IUnknown_iface);
}

static HRESULT STDMETHODCALLTYPE dxgi_surface_inner_QueryInterface(IUnknown *iface, REFIID riid, void **out)
{
    struct dxgi_surface *surface = impl_from_IUnknown(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDXGISurface)
            || IsEqualGUID(riid, &IID_IDXGIDeviceSubObject)
            || IsEqualGUID(riid, &IID_IDXGIObject)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDXGISurface_AddRef(&surface->IDXGISurface_iface);
        *out = &surface->IDXGISurface_iface;
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
        HeapFree(GetProcessHeap(), 0, surface);
    }

    return refcount;
}

static inline struct dxgi_surface *impl_from_IDXGISurface(IDXGISurface *iface)
{
    return CONTAINING_RECORD(iface, struct dxgi_surface, IDXGISurface_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE dxgi_surface_QueryInterface(IDXGISurface *iface, REFIID riid,
        void **object)
{
    struct dxgi_surface *This = impl_from_IDXGISurface(iface);
    TRACE("Forwarding to outer IUnknown\n");
    return IUnknown_QueryInterface(This->outer_unknown, riid, object);
}

static ULONG STDMETHODCALLTYPE dxgi_surface_AddRef(IDXGISurface *iface)
{
    struct dxgi_surface *This = impl_from_IDXGISurface(iface);
    TRACE("Forwarding to outer IUnknown\n");
    return IUnknown_AddRef(This->outer_unknown);
}

static ULONG STDMETHODCALLTYPE dxgi_surface_Release(IDXGISurface *iface)
{
    struct dxgi_surface *This = impl_from_IDXGISurface(iface);
    TRACE("Forwarding to outer IUnknown\n");
    return IUnknown_Release(This->outer_unknown);
}

/* IDXGIObject methods */

static HRESULT STDMETHODCALLTYPE dxgi_surface_SetPrivateData(IDXGISurface *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct dxgi_surface *surface = impl_from_IDXGISurface(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_set_private_data(&surface->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_surface_SetPrivateDataInterface(IDXGISurface *iface,
        REFGUID guid, const IUnknown *object)
{
    struct dxgi_surface *surface = impl_from_IDXGISurface(iface);

    TRACE("iface %p, guid %s, object %p.\n", iface, debugstr_guid(guid), object);

    return dxgi_set_private_data_interface(&surface->private_store, guid, object);
}

static HRESULT STDMETHODCALLTYPE dxgi_surface_GetPrivateData(IDXGISurface *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct dxgi_surface *surface = impl_from_IDXGISurface(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return dxgi_get_private_data(&surface->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE dxgi_surface_GetParent(IDXGISurface *iface, REFIID riid, void **parent)
{
    struct dxgi_surface *This = impl_from_IDXGISurface(iface);

    TRACE("iface %p, riid %s, parent %p.\n", iface, debugstr_guid(riid), parent);

    return IDXGIDevice_QueryInterface(This->device, riid, parent);
}

/* IDXGIDeviceSubObject methods */

static HRESULT STDMETHODCALLTYPE dxgi_surface_GetDevice(IDXGISurface *iface, REFIID riid, void **device)
{
    struct dxgi_surface *This = impl_from_IDXGISurface(iface);

    TRACE("iface %p, riid %s, device %p.\n", iface, debugstr_guid(riid), device);

    return IDXGIDevice_QueryInterface(This->device, riid, device);
}

/* IDXGISurface methods */
static HRESULT STDMETHODCALLTYPE dxgi_surface_GetDesc(IDXGISurface *iface, DXGI_SURFACE_DESC *desc)
{
    struct dxgi_surface *surface = impl_from_IDXGISurface(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    *desc = surface->desc;

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE dxgi_surface_Map(IDXGISurface *iface, DXGI_MAPPED_RECT *mapped_rect, UINT flags)
{
    FIXME("iface %p, mapped_rect %p, flags %#x stub!\n", iface, mapped_rect, flags);

    return E_NOTIMPL;
}

static HRESULT STDMETHODCALLTYPE dxgi_surface_Unmap(IDXGISurface *iface)
{
    FIXME("iface %p stub!\n", iface);

    return E_NOTIMPL;
}

static const struct IDXGISurfaceVtbl dxgi_surface_vtbl =
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
};

static const struct IUnknownVtbl dxgi_surface_inner_unknown_vtbl =
{
    /* IUnknown methods */
    dxgi_surface_inner_QueryInterface,
    dxgi_surface_inner_AddRef,
    dxgi_surface_inner_Release,
};

HRESULT dxgi_surface_init(struct dxgi_surface *surface, IDXGIDevice *device,
        IUnknown *outer, const DXGI_SURFACE_DESC *desc)
{
    surface->IDXGISurface_iface.lpVtbl = &dxgi_surface_vtbl;
    surface->IUnknown_iface.lpVtbl = &dxgi_surface_inner_unknown_vtbl;
    surface->refcount = 1;
    wined3d_private_store_init(&surface->private_store);
    surface->outer_unknown = outer ? outer : &surface->IUnknown_iface;
    surface->device = device;
    surface->desc = *desc;

    return S_OK;
}
