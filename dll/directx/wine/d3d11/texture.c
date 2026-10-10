/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
 * Copyright 2015 Józef Kucia for CodeWeavers
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

#include "d3d11_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d11);

/* ID3D11Texture1D methods */

static inline struct d3d_texture1d *impl_from_ID3D11Texture1D(ID3D11Texture1D *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_texture1d, ID3D11Texture1D_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_texture1d_QueryInterface(ID3D11Texture1D *iface, REFIID iid, void **out)
{
    struct d3d_texture1d *texture = impl_from_ID3D11Texture1D(iface);
    HRESULT hr;

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_ID3D11Texture1D)
            || IsEqualGUID(iid, &IID_ID3D11Resource)
            || IsEqualGUID(iid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(iid, &IID_IUnknown))
    {
        *out = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    if (IsEqualGUID(iid, &IID_ID3D10Texture1D)
            || IsEqualGUID(iid, &IID_ID3D10Resource)
            || IsEqualGUID(iid, &IID_ID3D10DeviceChild))
    {
        *out = &texture->ID3D10Texture1D_iface;
        IUnknown_AddRef((IUnknown *)*out);
        return S_OK;
    }

    TRACE("Forwarding to dxgi resource.\n");

    if (FAILED(hr = IUnknown_QueryInterface(texture->dxgi_resource, iid, out)))
    {
        WARN("%s not implemented, returning %#lx.\n", debugstr_guid(iid), hr);
        *out = NULL;
    }

    return hr;
}

static ULONG STDMETHODCALLTYPE d3d11_texture1d_AddRef(ID3D11Texture1D *iface)
{
    struct d3d_texture1d *texture = impl_from_ID3D11Texture1D(iface);
    ULONG refcount = InterlockedIncrement(&texture->refcount);

    TRACE("%p increasing refcount to %lu.\n", texture, refcount);

    if (refcount == 1)
    {
        ID3D11Device2_AddRef(texture->device);
        wined3d_texture_incref(texture->wined3d_texture);
    }

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_texture1d_Release(ID3D11Texture1D *iface)
{
    struct d3d_texture1d *texture = impl_from_ID3D11Texture1D(iface);
    ULONG refcount = InterlockedDecrement(&texture->refcount);

    TRACE("%p decreasing refcount to %lu.\n", texture, refcount);

    if (!refcount)
    {
        ID3D11Device2 *device = texture->device;
        wined3d_texture_decref(texture->wined3d_texture);
        /* Release the device last, it may cause the wined3d device to be
         * destroyed. */
        ID3D11Device2_Release(device);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_texture1d_GetDevice(ID3D11Texture1D *iface, ID3D11Device **device)
{
    struct d3d_texture1d *texture = impl_from_ID3D11Texture1D(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (ID3D11Device *)texture->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_texture1d_GetPrivateData(ID3D11Texture1D *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_texture1d *texture = impl_from_ID3D11Texture1D(iface);
    IDXGIResource *dxgi_resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    if (SUCCEEDED(hr = IUnknown_QueryInterface(texture->dxgi_resource, &IID_IDXGIResource, (void **)&dxgi_resource)))
    {
        hr = IDXGIResource_GetPrivateData(dxgi_resource, guid, data_size, data);
        IDXGIResource_Release(dxgi_resource);
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE d3d11_texture1d_SetPrivateData(ID3D11Texture1D *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_texture1d *texture = impl_from_ID3D11Texture1D(iface);
    IDXGIResource *dxgi_resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    if (SUCCEEDED(hr = IUnknown_QueryInterface(texture->dxgi_resource, &IID_IDXGIResource, (void **)&dxgi_resource)))
    {
        hr = IDXGIResource_SetPrivateData(dxgi_resource, guid, data_size, data);
        IDXGIResource_Release(dxgi_resource);
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE d3d11_texture1d_SetPrivateDataInterface(ID3D11Texture1D *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_texture1d *texture = impl_from_ID3D11Texture1D(iface);
    IDXGIResource *dxgi_resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    if (SUCCEEDED(hr = IUnknown_QueryInterface(texture->dxgi_resource, &IID_IDXGIResource, (void **)&dxgi_resource)))
    {
        hr = IDXGIResource_SetPrivateDataInterface(dxgi_resource, guid, data);
        IDXGIResource_Release(dxgi_resource);
    }

    return hr;
}

static void STDMETHODCALLTYPE d3d11_texture1d_GetType(ID3D11Texture1D *iface,
        D3D11_RESOURCE_DIMENSION *resource_dimension)
{
    TRACE("iface %p, resource_dimension %p.\n", iface, resource_dimension);

    *resource_dimension = D3D11_RESOURCE_DIMENSION_TEXTURE1D;
}

static void STDMETHODCALLTYPE d3d11_texture1d_SetEvictionPriority(ID3D11Texture1D *iface, UINT eviction_priority)
{
    FIXME("iface %p, eviction_priority %#x stub!\n", iface, eviction_priority);
}

static UINT STDMETHODCALLTYPE d3d11_texture1d_GetEvictionPriority(ID3D11Texture1D *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static void STDMETHODCALLTYPE d3d11_texture1d_GetDesc(ID3D11Texture1D *iface, D3D11_TEXTURE1D_DESC *desc)
{
    struct d3d_texture1d *texture = impl_from_ID3D11Texture1D(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    *desc = texture->desc;
}

static const struct ID3D11Texture1DVtbl d3d11_texture1d_vtbl =
{
    /* IUnknown methods */
    d3d11_texture1d_QueryInterface,
    d3d11_texture1d_AddRef,
    d3d11_texture1d_Release,
    /* ID3D11DeviceChild methods */
    d3d11_texture1d_GetDevice,
    d3d11_texture1d_GetPrivateData,
    d3d11_texture1d_SetPrivateData,
    d3d11_texture1d_SetPrivateDataInterface,
    /* ID3D11Resource methods */
    d3d11_texture1d_GetType,
    d3d11_texture1d_SetEvictionPriority,
    d3d11_texture1d_GetEvictionPriority,
    /* ID3D11Texture1D methods */
    d3d11_texture1d_GetDesc,
};

struct d3d_texture1d *unsafe_impl_from_ID3D11Texture1D(ID3D11Texture1D *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d11_texture1d_vtbl);
    return CONTAINING_RECORD(iface, struct d3d_texture1d, ID3D11Texture1D_iface);
}

static inline struct d3d_texture1d *impl_from_ID3D10Texture1D(ID3D10Texture1D *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_texture1d, ID3D10Texture1D_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_texture1d_QueryInterface(ID3D10Texture1D *iface, REFIID iid, void **out)
{
    struct d3d_texture1d *texture = impl_from_ID3D10Texture1D(iface);

    TRACE("iface %p, iid %s, out %p.\n", iface, debugstr_guid(iid), out);

    return d3d11_texture1d_QueryInterface(&texture->ID3D11Texture1D_iface, iid, out);
}

static ULONG STDMETHODCALLTYPE d3d10_texture1d_AddRef(ID3D10Texture1D *iface)
{
    struct d3d_texture1d *texture = impl_from_ID3D10Texture1D(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_texture1d_AddRef(&texture->ID3D11Texture1D_iface);
}

static void STDMETHODCALLTYPE d3d_texture1d_wined3d_object_released(void *parent)
{
    struct d3d_texture1d *texture = parent;

    if (texture->dxgi_resource)
        IUnknown_Release(texture->dxgi_resource);
    free(texture);
}

static ULONG STDMETHODCALLTYPE d3d10_texture1d_Release(ID3D10Texture1D *iface)
{
    struct d3d_texture1d *texture = impl_from_ID3D10Texture1D(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_texture1d_Release(&texture->ID3D11Texture1D_iface);
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_texture1d_GetDevice(ID3D10Texture1D *iface, ID3D10Device **device)
{
    struct d3d_texture1d *texture = impl_from_ID3D10Texture1D(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device2_QueryInterface(texture->device, &IID_ID3D10Device, (void **)device);
}

static HRESULT STDMETHODCALLTYPE d3d10_texture1d_GetPrivateData(ID3D10Texture1D *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_texture1d *texture = impl_from_ID3D10Texture1D(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d11_texture1d_GetPrivateData(&texture->ID3D11Texture1D_iface, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_texture1d_SetPrivateData(ID3D10Texture1D *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_texture1d *texture = impl_from_ID3D10Texture1D(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d11_texture1d_SetPrivateData(&texture->ID3D11Texture1D_iface, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_texture1d_SetPrivateDataInterface(ID3D10Texture1D *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_texture1d *texture = impl_from_ID3D10Texture1D(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d11_texture1d_SetPrivateDataInterface(&texture->ID3D11Texture1D_iface, guid, data);
}

/* ID3D10Resource methods */

static void STDMETHODCALLTYPE d3d10_texture1d_GetType(ID3D10Texture1D *iface,
        D3D10_RESOURCE_DIMENSION *resource_dimension)
{
    TRACE("iface %p, resource_dimension %p\n", iface, resource_dimension);

    *resource_dimension = D3D10_RESOURCE_DIMENSION_TEXTURE1D;
}

static void STDMETHODCALLTYPE d3d10_texture1d_SetEvictionPriority(ID3D10Texture1D *iface, UINT eviction_priority)
{
    FIXME("iface %p, eviction_priority %u stub!\n", iface, eviction_priority);
}

static UINT STDMETHODCALLTYPE d3d10_texture1d_GetEvictionPriority(ID3D10Texture1D *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

/* ID3D10Texture1D methods */

static HRESULT STDMETHODCALLTYPE d3d10_texture1d_Map(ID3D10Texture1D *iface, UINT sub_resource_idx,
        D3D10_MAP map_type, UINT map_flags, void **data)
{
    struct d3d_texture1d *texture = impl_from_ID3D10Texture1D(iface);
    struct wined3d_map_desc wined3d_map_desc;
    HRESULT hr;

    TRACE("iface %p, sub_resource_idx %u, map_type %u, map_flags %#x, data %p.\n",
            iface, sub_resource_idx, map_type, map_flags, data);

    if (map_flags)
        FIXME("Ignoring map_flags %#x.\n", map_flags);

    if (SUCCEEDED(hr = wined3d_resource_map(wined3d_texture_get_resource(texture->wined3d_texture), sub_resource_idx,
            &wined3d_map_desc, NULL, wined3d_map_flags_from_d3d10_map_type(map_type))))
    {
        *data = wined3d_map_desc.data;
    }

    return hr;
}

static void STDMETHODCALLTYPE d3d10_texture1d_Unmap(ID3D10Texture1D *iface, UINT sub_resource_idx)
{
    struct d3d_texture1d *texture = impl_from_ID3D10Texture1D(iface);

    TRACE("iface %p, sub_resource_idx %u.\n", iface, sub_resource_idx);

    wined3d_resource_unmap(wined3d_texture_get_resource(texture->wined3d_texture), sub_resource_idx);
}

static void STDMETHODCALLTYPE d3d10_texture1d_GetDesc(ID3D10Texture1D *iface, D3D10_TEXTURE1D_DESC *desc)
{
    struct d3d_texture1d *texture = impl_from_ID3D10Texture1D(iface);
    D3D11_TEXTURE1D_DESC d3d11_desc;

    TRACE("iface %p, desc %p.\n", iface, desc);

    d3d11_texture1d_GetDesc(&texture->ID3D11Texture1D_iface, &d3d11_desc);

    desc->Width = d3d11_desc.Width;
    desc->MipLevels = d3d11_desc.MipLevels;
    desc->ArraySize = d3d11_desc.ArraySize;
    desc->Format = d3d11_desc.Format;
    desc->Usage = d3d10_usage_from_d3d11_usage(d3d11_desc.Usage);
    desc->BindFlags = d3d10_bind_flags_from_d3d11_bind_flags(d3d11_desc.BindFlags);
    desc->CPUAccessFlags = d3d10_cpu_access_flags_from_d3d11_cpu_access_flags(d3d11_desc.CPUAccessFlags);
    desc->MiscFlags = d3d10_resource_misc_flags_from_d3d11_resource_misc_flags(d3d11_desc.MiscFlags);
}

static const struct ID3D10Texture1DVtbl d3d10_texture1d_vtbl =
{
    /* IUnknown methods */
    d3d10_texture1d_QueryInterface,
    d3d10_texture1d_AddRef,
    d3d10_texture1d_Release,
    /* ID3D10DeviceChild methods */
    d3d10_texture1d_GetDevice,
    d3d10_texture1d_GetPrivateData,
    d3d10_texture1d_SetPrivateData,
    d3d10_texture1d_SetPrivateDataInterface,
    /* ID3D10Resource methods */
    d3d10_texture1d_GetType,
    d3d10_texture1d_SetEvictionPriority,
    d3d10_texture1d_GetEvictionPriority,
    /* ID3D10Texture1D methods */
    d3d10_texture1d_Map,
    d3d10_texture1d_Unmap,
    d3d10_texture1d_GetDesc,
};

struct d3d_texture1d *unsafe_impl_from_ID3D10Texture1D(ID3D10Texture1D *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d10_texture1d_vtbl);
    return CONTAINING_RECORD(iface, struct d3d_texture1d, ID3D10Texture1D_iface);
}

static const struct wined3d_parent_ops d3d_texture1d_wined3d_parent_ops =
{
    d3d_texture1d_wined3d_object_released,
};

HRESULT d3d_device_create_dxgi_resource(IUnknown *device, struct wined3d_resource *wined3d_resource,
        IUnknown *outer, BOOL needs_surface, IUnknown **dxgi_resource)
{
    IWineDXGIDevice *wine_device;
    HRESULT hr;

    if (FAILED(hr = IUnknown_QueryInterface(device, &IID_IWineDXGIDevice, (void **)&wine_device)))
    {
        ERR("Device should implement IWineDXGIDevice.\n");
        return E_FAIL;
    }

    hr = IWineDXGIDevice_create_resource(wine_device, wined3d_resource, 0, NULL, outer,
            needs_surface, (void **)dxgi_resource);
    IWineDXGIDevice_Release(wine_device);
    if (FAILED(hr))
    {
        ERR("Failed to create DXGI resource, returning %#.lx\n", hr);
        *dxgi_resource = NULL;
    }

    return hr;
}

HRESULT d3d_texture1d_create(struct d3d_device *device, const D3D11_TEXTURE1D_DESC *desc,
        const D3D11_SUBRESOURCE_DATA *data, struct d3d_texture1d **out)
{
    struct wined3d_resource_desc wined3d_desc;
    struct d3d_texture1d *texture;
    unsigned int levels;
    BOOL needs_surface;
    DWORD flags = 0;
    HRESULT hr;

    if (!(texture = calloc(1, sizeof(*texture))))
        return E_OUTOFMEMORY;

    texture->ID3D11Texture1D_iface.lpVtbl = &d3d11_texture1d_vtbl;
    texture->ID3D10Texture1D_iface.lpVtbl = &d3d10_texture1d_vtbl;
    texture->refcount = 1;
    texture->desc = *desc;
    levels = desc->MipLevels ? desc->MipLevels : wined3d_log2i(desc->Width) + 1;
    texture->desc.MipLevels = levels;

    wined3d_desc.resource_type = WINED3D_RTYPE_TEXTURE_1D;
    wined3d_desc.format = wined3dformat_from_dxgi_format(desc->Format);
    wined3d_desc.multisample_type = WINED3D_MULTISAMPLE_NONE;
    wined3d_desc.multisample_quality = 0;
    wined3d_desc.usage = wined3d_usage_from_d3d11(desc->Usage);
    wined3d_desc.bind_flags = wined3d_bind_flags_from_d3d11(desc->BindFlags, desc->MiscFlags);
    wined3d_desc.access = wined3d_access_from_d3d11(desc->Usage,
            desc->Usage == D3D11_USAGE_DEFAULT ? 0 : desc->CPUAccessFlags);
    wined3d_desc.width = desc->Width;
    wined3d_desc.height = 1;
    wined3d_desc.depth = 1;
    wined3d_desc.size = 0;

    if (desc->MiscFlags & D3D11_RESOURCE_MISC_GDI_COMPATIBLE)
        flags |= WINED3D_TEXTURE_CREATE_GET_DC;
    if (desc->MiscFlags & D3D11_RESOURCE_MISC_GENERATE_MIPS)
        flags |= WINED3D_TEXTURE_CREATE_GENERATE_MIPMAPS;

    wined3d_mutex_lock();
    if (FAILED(hr = wined3d_texture_create(device->wined3d_device, &wined3d_desc,
            desc->ArraySize, levels, flags, (struct wined3d_sub_resource_data *)data,
            texture, &d3d_texture1d_wined3d_parent_ops, &texture->wined3d_texture)))
    {
        WARN("Failed to create wined3d texture, hr %#lx.\n", hr);
        free(texture);
        wined3d_mutex_unlock();
        if (hr == WINED3DERR_NOTAVAILABLE || hr == WINED3DERR_INVALIDCALL)
            hr = E_INVALIDARG;
        return hr;
    }

    needs_surface = desc->MipLevels == 1 && desc->ArraySize == 1;
    hr = d3d_device_create_dxgi_resource((IUnknown *)&device->ID3D10Device1_iface,
            wined3d_texture_get_resource(texture->wined3d_texture), (IUnknown *)&texture->ID3D10Texture1D_iface,
            needs_surface, &texture->dxgi_resource);
    if (FAILED(hr))
    {
        ERR("Failed to create DXGI resource, returning %#.lx\n", hr);
        wined3d_texture_decref(texture->wined3d_texture);
        wined3d_mutex_unlock();
        return hr;
    }
    wined3d_mutex_unlock();

    ID3D11Device2_AddRef(texture->device = &device->ID3D11Device2_iface);

    TRACE("Created texture %p.\n", texture);
    *out = texture;

    return S_OK;
}

/* ID3D11Texture2D methods */

static HRESULT STDMETHODCALLTYPE d3d11_texture2d_QueryInterface(ID3D11Texture2D *iface, REFIID riid, void **object)
{
    struct d3d_texture2d *texture = impl_from_ID3D11Texture2D(iface);
    HRESULT hr;

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11Texture2D)
            || IsEqualGUID(riid, &IID_ID3D11Resource)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        *object = &texture->ID3D11Texture2D_iface;
        IUnknown_AddRef((IUnknown *)*object);
        return S_OK;
    }
    else if (IsEqualGUID(riid, &IID_ID3D10Texture2D)
            || IsEqualGUID(riid, &IID_ID3D10Resource)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild))
    {
        *object = &texture->ID3D10Texture2D_iface;
        IUnknown_AddRef((IUnknown *)*object);
        return S_OK;
    }

    TRACE("Forwarding to dxgi resource.\n");

    if (FAILED(hr = IUnknown_QueryInterface(texture->dxgi_resource, riid, object)))
    {
        WARN("%s not implemented, returning %#lx.\n", debugstr_guid(riid), hr);
        *object = NULL;
    }

    return hr;
}

static ULONG STDMETHODCALLTYPE d3d11_texture2d_AddRef(ID3D11Texture2D *iface)
{
    struct d3d_texture2d *texture = impl_from_ID3D11Texture2D(iface);
    ULONG refcount = InterlockedIncrement(&texture->refcount);

    TRACE("%p increasing refcount to %lu.\n", texture, refcount);

    if (refcount == 1)
    {
        ID3D11Device2_AddRef(texture->device);
        wined3d_texture_incref(texture->wined3d_texture);
        if (texture->swapchain)
            wined3d_swapchain_incref(texture->swapchain);
    }

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_texture2d_Release(ID3D11Texture2D *iface)
{
    struct d3d_texture2d *texture = impl_from_ID3D11Texture2D(iface);
    ULONG refcount = InterlockedDecrement(&texture->refcount);

    TRACE("%p decreasing refcount to %lu.\n", texture, refcount);

    if (!refcount)
    {
        ID3D11Device2 *device = texture->device;
        if (texture->swapchain)
            wined3d_swapchain_decref(texture->swapchain);
        /* Releasing the texture may free the d3d11 object, so do not access it
         * after releasing the texture. */
        wined3d_texture_decref(texture->wined3d_texture);
        /* Release the device last, it may cause the wined3d device to be
         * destroyed. */
        ID3D11Device2_Release(device);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_texture2d_GetDevice(ID3D11Texture2D *iface, ID3D11Device **device)
{
    struct d3d_texture2d *texture = impl_from_ID3D11Texture2D(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (ID3D11Device *)texture->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_texture2d_GetPrivateData(ID3D11Texture2D *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_texture2d *texture = impl_from_ID3D11Texture2D(iface);
    IDXGIResource *dxgi_resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    if (SUCCEEDED(hr = IUnknown_QueryInterface(texture->dxgi_resource, &IID_IDXGIResource, (void **)&dxgi_resource)))
    {
        hr = IDXGIResource_GetPrivateData(dxgi_resource, guid, data_size, data);
        IDXGIResource_Release(dxgi_resource);
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE d3d11_texture2d_SetPrivateData(ID3D11Texture2D *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_texture2d *texture = impl_from_ID3D11Texture2D(iface);
    IDXGIResource *dxgi_resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    if (SUCCEEDED(hr = IUnknown_QueryInterface(texture->dxgi_resource, &IID_IDXGIResource, (void **)&dxgi_resource)))
    {
        hr = IDXGIResource_SetPrivateData(dxgi_resource, guid, data_size, data);
        IDXGIResource_Release(dxgi_resource);
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE d3d11_texture2d_SetPrivateDataInterface(ID3D11Texture2D *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_texture2d *texture = impl_from_ID3D11Texture2D(iface);
    IDXGIResource *dxgi_resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    if (SUCCEEDED(hr = IUnknown_QueryInterface(texture->dxgi_resource, &IID_IDXGIResource, (void **)&dxgi_resource)))
    {
        hr = IDXGIResource_SetPrivateDataInterface(dxgi_resource, guid, data);
        IDXGIResource_Release(dxgi_resource);
    }

    return hr;
}

static void STDMETHODCALLTYPE d3d11_texture2d_GetType(ID3D11Texture2D *iface,
        D3D11_RESOURCE_DIMENSION *resource_dimension)
{
    TRACE("iface %p, resource_dimension %p.\n", iface, resource_dimension);

    *resource_dimension = D3D11_RESOURCE_DIMENSION_TEXTURE2D;
}

static void STDMETHODCALLTYPE d3d11_texture2d_SetEvictionPriority(ID3D11Texture2D *iface, UINT eviction_priority)
{
    FIXME("iface %p, eviction_priority %#x stub!\n", iface, eviction_priority);
}

static UINT STDMETHODCALLTYPE d3d11_texture2d_GetEvictionPriority(ID3D11Texture2D *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static void STDMETHODCALLTYPE d3d11_texture2d_GetDesc(ID3D11Texture2D *iface, D3D11_TEXTURE2D_DESC *desc)
{
    struct d3d_texture2d *texture = impl_from_ID3D11Texture2D(iface);
    struct wined3d_resource_desc wined3d_desc;

    TRACE("iface %p, desc %p.\n", iface, desc);

    *desc = texture->desc;

    wined3d_mutex_lock();
    wined3d_resource_get_desc(wined3d_texture_get_resource(texture->wined3d_texture), &wined3d_desc);
    wined3d_mutex_unlock();

    /* FIXME: Resizing swapchain buffers can cause these to change. We'd like
     * to get everything from wined3d, but e.g. bind flags don't exist as such
     * there (yet). */
    desc->Width = wined3d_desc.width;
    desc->Height = wined3d_desc.height;
    desc->Format = dxgi_format_from_wined3dformat(wined3d_desc.format);
    desc->SampleDesc.Count = wined3d_desc.multisample_type == WINED3D_MULTISAMPLE_NONE
        ? 1 : wined3d_desc.multisample_type;
    desc->SampleDesc.Quality = wined3d_desc.multisample_quality;
}

static const struct ID3D11Texture2DVtbl d3d11_texture2d_vtbl =
{
    /* IUnknown methods */
    d3d11_texture2d_QueryInterface,
    d3d11_texture2d_AddRef,
    d3d11_texture2d_Release,
    /* ID3D11DeviceChild methods */
    d3d11_texture2d_GetDevice,
    d3d11_texture2d_GetPrivateData,
    d3d11_texture2d_SetPrivateData,
    d3d11_texture2d_SetPrivateDataInterface,
    /* ID3D11Resource methods */
    d3d11_texture2d_GetType,
    d3d11_texture2d_SetEvictionPriority,
    d3d11_texture2d_GetEvictionPriority,
    /* ID3D11Texture2D methods */
    d3d11_texture2d_GetDesc,
};

struct d3d_texture2d *unsafe_impl_from_ID3D11Texture2D(ID3D11Texture2D *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d11_texture2d_vtbl);
    return CONTAINING_RECORD(iface, struct d3d_texture2d, ID3D11Texture2D_iface);
}

/* IUnknown methods */

static inline struct d3d_texture2d *impl_from_ID3D10Texture2D(ID3D10Texture2D *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_texture2d, ID3D10Texture2D_iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_texture2d_QueryInterface(ID3D10Texture2D *iface, REFIID riid, void **object)
{
    struct d3d_texture2d *texture = impl_from_ID3D10Texture2D(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return d3d11_texture2d_QueryInterface(&texture->ID3D11Texture2D_iface, riid, object);
}

static ULONG STDMETHODCALLTYPE d3d10_texture2d_AddRef(ID3D10Texture2D *iface)
{
    struct d3d_texture2d *texture = impl_from_ID3D10Texture2D(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_texture2d_AddRef(&texture->ID3D11Texture2D_iface);
}

static void STDMETHODCALLTYPE d3d_texture2d_wined3d_object_released(void *parent)
{
    struct d3d_texture2d *texture = parent;

    if (texture->dxgi_resource) IUnknown_Release(texture->dxgi_resource);
    free(texture);
}

static ULONG STDMETHODCALLTYPE d3d10_texture2d_Release(ID3D10Texture2D *iface)
{
    struct d3d_texture2d *texture = impl_from_ID3D10Texture2D(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_texture2d_Release(&texture->ID3D11Texture2D_iface);
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_texture2d_GetDevice(ID3D10Texture2D *iface, ID3D10Device **device)
{
    struct d3d_texture2d *texture = impl_from_ID3D10Texture2D(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device2_QueryInterface(texture->device, &IID_ID3D10Device, (void **)device);
}

static HRESULT STDMETHODCALLTYPE d3d10_texture2d_GetPrivateData(ID3D10Texture2D *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_texture2d *texture = impl_from_ID3D10Texture2D(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d11_texture2d_GetPrivateData(&texture->ID3D11Texture2D_iface, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_texture2d_SetPrivateData(ID3D10Texture2D *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_texture2d *texture = impl_from_ID3D10Texture2D(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d11_texture2d_SetPrivateData(&texture->ID3D11Texture2D_iface, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_texture2d_SetPrivateDataInterface(ID3D10Texture2D *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_texture2d *texture = impl_from_ID3D10Texture2D(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d11_texture2d_SetPrivateDataInterface(&texture->ID3D11Texture2D_iface, guid, data);
}

/* ID3D10Resource methods */

static void STDMETHODCALLTYPE d3d10_texture2d_GetType(ID3D10Texture2D *iface,
        D3D10_RESOURCE_DIMENSION *resource_dimension)
{
    TRACE("iface %p, resource_dimension %p\n", iface, resource_dimension);

    *resource_dimension = D3D10_RESOURCE_DIMENSION_TEXTURE2D;
}

static void STDMETHODCALLTYPE d3d10_texture2d_SetEvictionPriority(ID3D10Texture2D *iface, UINT eviction_priority)
{
    FIXME("iface %p, eviction_priority %u stub!\n", iface, eviction_priority);
}

static UINT STDMETHODCALLTYPE d3d10_texture2d_GetEvictionPriority(ID3D10Texture2D *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

/* ID3D10Texture2D methods */

static HRESULT STDMETHODCALLTYPE d3d10_texture2d_Map(ID3D10Texture2D *iface, UINT sub_resource_idx,
        D3D10_MAP map_type, UINT map_flags, D3D10_MAPPED_TEXTURE2D *mapped_texture)
{
    struct d3d_texture2d *texture = impl_from_ID3D10Texture2D(iface);
    struct wined3d_map_desc wined3d_map_desc;
    HRESULT hr;

    TRACE("iface %p, sub_resource_idx %u, map_type %u, map_flags %#x, mapped_texture %p.\n",
            iface, sub_resource_idx, map_type, map_flags, mapped_texture);

    if (map_flags)
        FIXME("Ignoring map_flags %#x.\n", map_flags);

    if (SUCCEEDED(hr = wined3d_resource_map(wined3d_texture_get_resource(texture->wined3d_texture), sub_resource_idx,
            &wined3d_map_desc, NULL, wined3d_map_flags_from_d3d10_map_type(map_type))))
    {
        mapped_texture->pData = wined3d_map_desc.data;
        mapped_texture->RowPitch = wined3d_map_desc.row_pitch;
    }

    return hr;
}

static void STDMETHODCALLTYPE d3d10_texture2d_Unmap(ID3D10Texture2D *iface, UINT sub_resource_idx)
{
    struct d3d_texture2d *texture = impl_from_ID3D10Texture2D(iface);

    TRACE("iface %p, sub_resource_idx %u.\n", iface, sub_resource_idx);

    wined3d_resource_unmap(wined3d_texture_get_resource(texture->wined3d_texture), sub_resource_idx);
}

static void STDMETHODCALLTYPE d3d10_texture2d_GetDesc(ID3D10Texture2D *iface, D3D10_TEXTURE2D_DESC *desc)
{
    struct d3d_texture2d *texture = impl_from_ID3D10Texture2D(iface);
    D3D11_TEXTURE2D_DESC d3d11_desc;

    TRACE("iface %p, desc %p\n", iface, desc);

    d3d11_texture2d_GetDesc(&texture->ID3D11Texture2D_iface, &d3d11_desc);

    desc->Width = d3d11_desc.Width;
    desc->Height = d3d11_desc.Height;
    desc->MipLevels = d3d11_desc.MipLevels;
    desc->ArraySize = d3d11_desc.ArraySize;
    desc->Format = d3d11_desc.Format;
    desc->SampleDesc = d3d11_desc.SampleDesc;
    desc->Usage = d3d10_usage_from_d3d11_usage(d3d11_desc.Usage);
    desc->BindFlags = d3d10_bind_flags_from_d3d11_bind_flags(d3d11_desc.BindFlags);
    desc->CPUAccessFlags = d3d10_cpu_access_flags_from_d3d11_cpu_access_flags(d3d11_desc.CPUAccessFlags);
    desc->MiscFlags = d3d10_resource_misc_flags_from_d3d11_resource_misc_flags(d3d11_desc.MiscFlags);
}

static const struct ID3D10Texture2DVtbl d3d10_texture2d_vtbl =
{
    /* IUnknown methods */
    d3d10_texture2d_QueryInterface,
    d3d10_texture2d_AddRef,
    d3d10_texture2d_Release,
    /* ID3D10DeviceChild methods */
    d3d10_texture2d_GetDevice,
    d3d10_texture2d_GetPrivateData,
    d3d10_texture2d_SetPrivateData,
    d3d10_texture2d_SetPrivateDataInterface,
    /* ID3D10Resource methods */
    d3d10_texture2d_GetType,
    d3d10_texture2d_SetEvictionPriority,
    d3d10_texture2d_GetEvictionPriority,
    /* ID3D10Texture2D methods */
    d3d10_texture2d_Map,
    d3d10_texture2d_Unmap,
    d3d10_texture2d_GetDesc,
};

struct d3d_texture2d *unsafe_impl_from_ID3D10Texture2D(ID3D10Texture2D *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d10_texture2d_vtbl);
    return CONTAINING_RECORD(iface, struct d3d_texture2d, ID3D10Texture2D_iface);
}

static const struct wined3d_parent_ops d3d_texture2d_wined3d_parent_ops =
{
    d3d_texture2d_wined3d_object_released,
};

static BOOL is_gdi_compatible_texture(const D3D11_TEXTURE2D_DESC *desc)
{
    if (!(desc->Format == DXGI_FORMAT_B8G8R8A8_UNORM
            || desc->Format == DXGI_FORMAT_B8G8R8A8_TYPELESS
            || desc->Format == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB))
        return FALSE;

    if (desc->Usage != D3D11_USAGE_DEFAULT)
        return FALSE;

    return TRUE;
}

static BOOL validate_texture2d_desc(const D3D11_TEXTURE2D_DESC *desc, D3D_FEATURE_LEVEL feature_level)
{
    if (!validate_d3d11_resource_access_flags(D3D11_RESOURCE_DIMENSION_TEXTURE2D,
            desc->Usage, desc->BindFlags, desc->CPUAccessFlags, feature_level))
        return FALSE;

    if (desc->MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE
            && desc->ArraySize < 6)
    {
        WARN("Invalid array size %u for cube texture.\n", desc->ArraySize);
        return FALSE;
    }

    if (desc->MiscFlags & D3D11_RESOURCE_MISC_GDI_COMPATIBLE
            && !is_gdi_compatible_texture(desc))
    {
        WARN("Incompatible description used to create GDI compatible texture.\n");
        return FALSE;
    }

    if (desc->MiscFlags & D3D11_RESOURCE_MISC_GENERATE_MIPS
            && (~desc->BindFlags & (D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)))
    {
        WARN("D3D11_RESOURCE_MISC_GENERATE_MIPS used without D3D11_BIND_RENDER_TARGET and "
                "D3D11_BIND_SHADER_RESOURCE.\n");
        return FALSE;
    }

    return TRUE;
}

HRESULT d3d_texture2d_create(struct d3d_device *device, const D3D11_TEXTURE2D_DESC *desc,
        struct wined3d_texture *wined3d_texture, const D3D11_SUBRESOURCE_DATA *data, struct d3d_texture2d **out)
{
    struct d3d_texture2d *texture;
    BOOL needs_surface;
    DWORD flags = 0;
    HRESULT hr;

    if (!validate_texture2d_desc(desc, device->state->feature_level))
    {
        WARN("Failed to validate texture desc.\n");
        return E_INVALIDARG;
    }

    if (!(texture = calloc(1, sizeof(*texture))))
        return E_OUTOFMEMORY;

    texture->ID3D11Texture2D_iface.lpVtbl = &d3d11_texture2d_vtbl;
    texture->ID3D10Texture2D_iface.lpVtbl = &d3d10_texture2d_vtbl;
    texture->refcount = 1;
    wined3d_mutex_lock();
    texture->desc = *desc;

    if (wined3d_texture)
    {
        wined3d_resource_set_parent(wined3d_texture_get_resource(wined3d_texture),
                texture, &d3d_texture2d_wined3d_parent_ops);
        wined3d_texture_incref(wined3d_texture);
        texture->wined3d_texture = wined3d_texture;

        if ((texture->swapchain = wined3d_texture_get_swapchain(wined3d_texture)))
            wined3d_swapchain_incref(texture->swapchain);
    }
    else
    {
        struct wined3d_resource_desc wined3d_desc;
        unsigned int levels;

        wined3d_desc.resource_type = WINED3D_RTYPE_TEXTURE_2D;
        wined3d_desc.format = wined3dformat_from_dxgi_format(desc->Format);
        wined3d_desc.multisample_type = desc->SampleDesc.Count > 1 ? desc->SampleDesc.Count : WINED3D_MULTISAMPLE_NONE;
        wined3d_desc.multisample_quality = desc->SampleDesc.Quality;
        wined3d_desc.usage = wined3d_usage_from_d3d11(desc->Usage);
        wined3d_desc.bind_flags = wined3d_bind_flags_from_d3d11(desc->BindFlags, desc->MiscFlags);
        wined3d_desc.access = wined3d_access_from_d3d11(desc->Usage,
                desc->Usage == D3D11_USAGE_DEFAULT ? 0 : desc->CPUAccessFlags);
        wined3d_desc.width = desc->Width;
        wined3d_desc.height = desc->Height;
        wined3d_desc.depth = 1;
        wined3d_desc.size = 0;

        levels = desc->MipLevels ? desc->MipLevels : wined3d_log2i(max(desc->Width, desc->Height)) + 1;

        if (desc->MiscFlags & D3D11_RESOURCE_MISC_GDI_COMPATIBLE)
            flags |= WINED3D_TEXTURE_CREATE_GET_DC;
        if (desc->MiscFlags & D3D11_RESOURCE_MISC_GENERATE_MIPS)
            flags |= WINED3D_TEXTURE_CREATE_GENERATE_MIPMAPS;

        if (FAILED(hr = wined3d_texture_create(device->wined3d_device, &wined3d_desc,
                desc->ArraySize, levels, flags, (struct wined3d_sub_resource_data *)data,
                texture, &d3d_texture2d_wined3d_parent_ops, &texture->wined3d_texture)))
        {
            WARN("Failed to create wined3d texture, hr %#lx.\n", hr);
            free(texture);
            wined3d_mutex_unlock();
            if (hr == WINED3DERR_NOTAVAILABLE || hr == WINED3DERR_INVALIDCALL)
                hr = E_INVALIDARG;
            return hr;
        }
        texture->desc.MipLevels = levels;
    }

    needs_surface = desc->MipLevels == 1 && desc->ArraySize == 1;
    hr = d3d_device_create_dxgi_resource((IUnknown *)&device->ID3D10Device1_iface,
            wined3d_texture_get_resource(texture->wined3d_texture), (IUnknown *)&texture->ID3D10Texture2D_iface,
            needs_surface, &texture->dxgi_resource);
    if (FAILED(hr))
    {
        ERR("Failed to create DXGI resource, returning %#.lx\n", hr);
        wined3d_texture_decref(texture->wined3d_texture);
        wined3d_mutex_unlock();
        return hr;
    }
    wined3d_mutex_unlock();

    ID3D11Device2_AddRef(texture->device = &device->ID3D11Device2_iface);

    TRACE("Created texture %p.\n", texture);
    *out = texture;

    return S_OK;
}

/* ID3D11Texture3D methods */

static inline struct d3d_texture3d *impl_from_ID3D11Texture3D(ID3D11Texture3D *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_texture3d, ID3D11Texture3D_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_texture3d_QueryInterface(ID3D11Texture3D *iface, REFIID riid, void **object)
{
    struct d3d_texture3d *texture = impl_from_ID3D11Texture3D(iface);
    HRESULT hr;

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_ID3D11Texture3D)
            || IsEqualGUID(riid, &IID_ID3D11Resource)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }
    else if (IsEqualGUID(riid, &IID_ID3D10Texture3D)
            || IsEqualGUID(riid, &IID_ID3D10Resource)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild))
    {
        IUnknown_AddRef(iface);
        *object = &texture->ID3D10Texture3D_iface;
        return S_OK;
    }

    TRACE("Forwarding to dxgi resource.\n");

    if (FAILED(hr = IUnknown_QueryInterface(texture->dxgi_resource, riid, object)))
    {
        WARN("%s not implemented, returning %#lx.\n", debugstr_guid(riid), hr);
        *object = NULL;
    }

    return hr;
}

static ULONG STDMETHODCALLTYPE d3d11_texture3d_AddRef(ID3D11Texture3D *iface)
{
    struct d3d_texture3d *texture = impl_from_ID3D11Texture3D(iface);
    ULONG refcount = InterlockedIncrement(&texture->refcount);

    TRACE("%p increasing refcount to %lu.\n", texture, refcount);

    if (refcount == 1)
    {
        ID3D11Device2_AddRef(texture->device);
        wined3d_texture_incref(texture->wined3d_texture);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d_texture3d_wined3d_object_released(void *parent)
{
    struct d3d_texture3d *texture = parent;

    if (texture->dxgi_resource)
        IUnknown_Release(texture->dxgi_resource);
    free(parent);
}

static ULONG STDMETHODCALLTYPE d3d11_texture3d_Release(ID3D11Texture3D *iface)
{
    struct d3d_texture3d *texture = impl_from_ID3D11Texture3D(iface);
    ULONG refcount = InterlockedDecrement(&texture->refcount);

    TRACE("%p decreasing refcount to %lu.\n", texture, refcount);

    if (!refcount)
    {
        ID3D11Device2 *device = texture->device;

        wined3d_texture_decref(texture->wined3d_texture);
        /* Release the device last, it may cause the wined3d device to be
         * destroyed. */
        ID3D11Device2_Release(device);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_texture3d_GetDevice(ID3D11Texture3D *iface, ID3D11Device **device)
{
    struct d3d_texture3d *texture = impl_from_ID3D11Texture3D(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (ID3D11Device *)texture->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_texture3d_GetPrivateData(ID3D11Texture3D *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_texture3d *texture = impl_from_ID3D11Texture3D(iface);
    IDXGIResource *dxgi_resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    if (SUCCEEDED(hr = IUnknown_QueryInterface(texture->dxgi_resource, &IID_IDXGIResource, (void **)&dxgi_resource)))
    {
        hr = IDXGIResource_GetPrivateData(dxgi_resource, guid, data_size, data);
        IDXGIResource_Release(dxgi_resource);
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE d3d11_texture3d_SetPrivateData(ID3D11Texture3D *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_texture3d *texture = impl_from_ID3D11Texture3D(iface);
    IDXGIResource *dxgi_resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    if (SUCCEEDED(hr = IUnknown_QueryInterface(texture->dxgi_resource, &IID_IDXGIResource, (void **)&dxgi_resource)))
    {
        hr = IDXGIResource_SetPrivateData(dxgi_resource, guid, data_size, data);
        IDXGIResource_Release(dxgi_resource);
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE d3d11_texture3d_SetPrivateDataInterface(ID3D11Texture3D *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_texture3d *texture = impl_from_ID3D11Texture3D(iface);
    IDXGIResource *dxgi_resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    if (SUCCEEDED(hr = IUnknown_QueryInterface(texture->dxgi_resource, &IID_IDXGIResource, (void **)&dxgi_resource)))
    {
        hr = IDXGIResource_SetPrivateDataInterface(dxgi_resource, guid, data);
        IDXGIResource_Release(dxgi_resource);
    }

    return hr;
}

static void STDMETHODCALLTYPE d3d11_texture3d_GetType(ID3D11Texture3D *iface,
        D3D11_RESOURCE_DIMENSION *resource_dimension)
{
    TRACE("iface %p, resource_dimension %p.\n", iface, resource_dimension);

    *resource_dimension = D3D11_RESOURCE_DIMENSION_TEXTURE3D;
}

static void STDMETHODCALLTYPE d3d11_texture3d_SetEvictionPriority(ID3D11Texture3D *iface, UINT eviction_priority)
{
    FIXME("iface %p, eviction_priority %#x stub!\n", iface, eviction_priority);
}

static UINT STDMETHODCALLTYPE d3d11_texture3d_GetEvictionPriority(ID3D11Texture3D *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static void STDMETHODCALLTYPE d3d11_texture3d_GetDesc(ID3D11Texture3D *iface, D3D11_TEXTURE3D_DESC *desc)
{
    struct d3d_texture3d *texture = impl_from_ID3D11Texture3D(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    *desc = texture->desc;
}

static const struct ID3D11Texture3DVtbl d3d11_texture3d_vtbl =
{
    /* IUnknown methods */
    d3d11_texture3d_QueryInterface,
    d3d11_texture3d_AddRef,
    d3d11_texture3d_Release,
    /* ID3D11DeviceChild methods */
    d3d11_texture3d_GetDevice,
    d3d11_texture3d_GetPrivateData,
    d3d11_texture3d_SetPrivateData,
    d3d11_texture3d_SetPrivateDataInterface,
    /* ID3D11Resource methods */
    d3d11_texture3d_GetType,
    d3d11_texture3d_SetEvictionPriority,
    d3d11_texture3d_GetEvictionPriority,
    /* ID3D11Texture3D methods */
    d3d11_texture3d_GetDesc,
};

/* ID3D10Texture3D methods */

static inline struct d3d_texture3d *impl_from_ID3D10Texture3D(ID3D10Texture3D *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_texture3d, ID3D10Texture3D_iface);
}

static HRESULT STDMETHODCALLTYPE d3d10_texture3d_QueryInterface(ID3D10Texture3D *iface, REFIID riid, void **object)
{
    struct d3d_texture3d *texture = impl_from_ID3D10Texture3D(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return d3d11_texture3d_QueryInterface(&texture->ID3D11Texture3D_iface, riid, object);
}

static ULONG STDMETHODCALLTYPE d3d10_texture3d_AddRef(ID3D10Texture3D *iface)
{
    struct d3d_texture3d *texture = impl_from_ID3D10Texture3D(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_texture3d_AddRef(&texture->ID3D11Texture3D_iface);
}

static ULONG STDMETHODCALLTYPE d3d10_texture3d_Release(ID3D10Texture3D *iface)
{
    struct d3d_texture3d *texture = impl_from_ID3D10Texture3D(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_texture3d_Release(&texture->ID3D11Texture3D_iface);
}

static void STDMETHODCALLTYPE d3d10_texture3d_GetDevice(ID3D10Texture3D *iface, ID3D10Device **device)
{
    struct d3d_texture3d *texture = impl_from_ID3D10Texture3D(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device2_QueryInterface(texture->device, &IID_ID3D10Device, (void **)device);
}

static HRESULT STDMETHODCALLTYPE d3d10_texture3d_GetPrivateData(ID3D10Texture3D *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_texture3d *texture = impl_from_ID3D10Texture3D(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d11_texture3d_GetPrivateData(&texture->ID3D11Texture3D_iface, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_texture3d_SetPrivateData(ID3D10Texture3D *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_texture3d *texture = impl_from_ID3D10Texture3D(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d11_texture3d_SetPrivateData(&texture->ID3D11Texture3D_iface, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_texture3d_SetPrivateDataInterface(ID3D10Texture3D *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_texture3d *texture = impl_from_ID3D10Texture3D(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d11_texture3d_SetPrivateDataInterface(&texture->ID3D11Texture3D_iface, guid, data);
}

static void STDMETHODCALLTYPE d3d10_texture3d_GetType(ID3D10Texture3D *iface,
        D3D10_RESOURCE_DIMENSION *resource_dimension)
{
    TRACE("iface %p, resource_dimension %p.\n", iface, resource_dimension);

    *resource_dimension = D3D10_RESOURCE_DIMENSION_TEXTURE3D;
}

static void STDMETHODCALLTYPE d3d10_texture3d_SetEvictionPriority(ID3D10Texture3D *iface, UINT eviction_priority)
{
    FIXME("iface %p, eviction_priority %u stub!\n", iface, eviction_priority);
}

static UINT STDMETHODCALLTYPE d3d10_texture3d_GetEvictionPriority(ID3D10Texture3D *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static HRESULT STDMETHODCALLTYPE d3d10_texture3d_Map(ID3D10Texture3D *iface, UINT sub_resource_idx,
        D3D10_MAP map_type, UINT map_flags, D3D10_MAPPED_TEXTURE3D *mapped_texture)
{
    struct d3d_texture3d *texture = impl_from_ID3D10Texture3D(iface);
    struct wined3d_map_desc wined3d_map_desc;
    HRESULT hr;

    TRACE("iface %p, sub_resource_idx %u, map_type %u, map_flags %#x, mapped_texture %p.\n",
            iface, sub_resource_idx, map_type, map_flags, mapped_texture);

    if (map_flags)
        FIXME("Ignoring map_flags %#x.\n", map_flags);

    if (SUCCEEDED(hr = wined3d_resource_map(wined3d_texture_get_resource(texture->wined3d_texture), sub_resource_idx,
            &wined3d_map_desc, NULL, wined3d_map_flags_from_d3d10_map_type(map_type))))
    {
        mapped_texture->pData = wined3d_map_desc.data;
        mapped_texture->RowPitch = wined3d_map_desc.row_pitch;
        mapped_texture->DepthPitch = wined3d_map_desc.slice_pitch;
    }

    return hr;
}

static void STDMETHODCALLTYPE d3d10_texture3d_Unmap(ID3D10Texture3D *iface, UINT sub_resource_idx)
{
    struct d3d_texture3d *texture = impl_from_ID3D10Texture3D(iface);

    TRACE("iface %p, sub_resource_idx %u.\n", iface, sub_resource_idx);

    wined3d_resource_unmap(wined3d_texture_get_resource(texture->wined3d_texture), sub_resource_idx);
}

static void STDMETHODCALLTYPE d3d10_texture3d_GetDesc(ID3D10Texture3D *iface, D3D10_TEXTURE3D_DESC *desc)
{
    struct d3d_texture3d *texture = impl_from_ID3D10Texture3D(iface);
    D3D11_TEXTURE3D_DESC d3d11_desc;

    TRACE("iface %p, desc %p.\n", iface, desc);

    d3d11_texture3d_GetDesc(&texture->ID3D11Texture3D_iface, &d3d11_desc);

    desc->Width = d3d11_desc.Width;
    desc->Height = d3d11_desc.Height;
    desc->Depth = d3d11_desc.Depth;
    desc->MipLevels = d3d11_desc.MipLevels;
    desc->Format = d3d11_desc.Format;
    desc->Usage = d3d10_usage_from_d3d11_usage(d3d11_desc.Usage);
    desc->BindFlags = d3d10_bind_flags_from_d3d11_bind_flags(d3d11_desc.BindFlags);
    desc->CPUAccessFlags = d3d10_cpu_access_flags_from_d3d11_cpu_access_flags(d3d11_desc.CPUAccessFlags);
    desc->MiscFlags = d3d10_resource_misc_flags_from_d3d11_resource_misc_flags(d3d11_desc.MiscFlags);
}

static const struct ID3D10Texture3DVtbl d3d10_texture3d_vtbl =
{
    /* IUnknown methods */
    d3d10_texture3d_QueryInterface,
    d3d10_texture3d_AddRef,
    d3d10_texture3d_Release,
    /* ID3D10DeviceChild methods */
    d3d10_texture3d_GetDevice,
    d3d10_texture3d_GetPrivateData,
    d3d10_texture3d_SetPrivateData,
    d3d10_texture3d_SetPrivateDataInterface,
    /* ID3D10Resource methods */
    d3d10_texture3d_GetType,
    d3d10_texture3d_SetEvictionPriority,
    d3d10_texture3d_GetEvictionPriority,
    /* ID3D10Texture3D methods */
    d3d10_texture3d_Map,
    d3d10_texture3d_Unmap,
    d3d10_texture3d_GetDesc,
};

struct d3d_texture3d *unsafe_impl_from_ID3D10Texture3D(ID3D10Texture3D *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d10_texture3d_vtbl);
    return CONTAINING_RECORD(iface, struct d3d_texture3d, ID3D10Texture3D_iface);
}

struct d3d_texture3d *unsafe_impl_from_ID3D11Texture3D(ID3D11Texture3D *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d11_texture3d_vtbl);
    return impl_from_ID3D11Texture3D(iface);
}

static const struct wined3d_parent_ops d3d_texture3d_wined3d_parent_ops =
{
    d3d_texture3d_wined3d_object_released,
};

static HRESULT d3d_texture3d_init(struct d3d_texture3d *texture, struct d3d_device *device,
        const D3D11_TEXTURE3D_DESC *desc, const D3D11_SUBRESOURCE_DATA *data)
{
    struct wined3d_resource_desc wined3d_desc;
    unsigned int levels;
    DWORD flags = 0;
    HRESULT hr;

    if (desc->MiscFlags & D3D11_RESOURCE_MISC_GENERATE_MIPS
            && (~desc->BindFlags & (D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE)))
    {
        WARN("D3D11_RESOURCE_MISC_GENERATE_MIPS used without D3D11_BIND_RENDER_TARGET "
                "and D3D11_BIND_SHADER_RESOURCE.\n");
        return E_INVALIDARG;
    }

    texture->ID3D11Texture3D_iface.lpVtbl = &d3d11_texture3d_vtbl;
    texture->ID3D10Texture3D_iface.lpVtbl = &d3d10_texture3d_vtbl;
    texture->refcount = 1;
    wined3d_mutex_lock();
    texture->desc = *desc;

    wined3d_desc.resource_type = WINED3D_RTYPE_TEXTURE_3D;
    wined3d_desc.format = wined3dformat_from_dxgi_format(desc->Format);
    wined3d_desc.multisample_type = WINED3D_MULTISAMPLE_NONE;
    wined3d_desc.multisample_quality = 0;
    wined3d_desc.usage = wined3d_usage_from_d3d11(desc->Usage);
    wined3d_desc.bind_flags = wined3d_bind_flags_from_d3d11(desc->BindFlags, desc->MiscFlags);
    wined3d_desc.access = wined3d_access_from_d3d11(desc->Usage,
            desc->Usage == D3D11_USAGE_DEFAULT ? 0 : desc->CPUAccessFlags);
    wined3d_desc.width = desc->Width;
    wined3d_desc.height = desc->Height;
    wined3d_desc.depth = desc->Depth;
    wined3d_desc.size = 0;

    levels = desc->MipLevels ? desc->MipLevels : wined3d_log2i(max(max(desc->Width, desc->Height), desc->Depth)) + 1;

    if (desc->MiscFlags & D3D11_RESOURCE_MISC_GENERATE_MIPS)
        flags |= WINED3D_TEXTURE_CREATE_GENERATE_MIPMAPS;

    if (FAILED(hr = wined3d_texture_create(device->wined3d_device, &wined3d_desc,
            1, levels, flags, (struct wined3d_sub_resource_data *)data, texture,
            &d3d_texture3d_wined3d_parent_ops, &texture->wined3d_texture)))
    {
        WARN("Failed to create wined3d texture, hr %#lx.\n", hr);
        wined3d_mutex_unlock();
        if (hr == WINED3DERR_INVALIDCALL)
            hr = E_INVALIDARG;
        return hr;
    }

    hr = d3d_device_create_dxgi_resource((IUnknown *)&device->ID3D10Device1_iface,
            wined3d_texture_get_resource(texture->wined3d_texture), (IUnknown *)&texture->ID3D10Texture3D_iface,
            FALSE, &texture->dxgi_resource);
    if (FAILED(hr))
    {
        ERR("Failed to create DXGI resource, returning %#.lx\n", hr);
        wined3d_texture_decref(texture->wined3d_texture);
        wined3d_mutex_unlock();
        return hr;
    }
    wined3d_mutex_unlock();
    texture->desc.MipLevels = levels;

    ID3D11Device2_AddRef(texture->device = &device->ID3D11Device2_iface);

    return S_OK;
}

HRESULT d3d_texture3d_create(struct d3d_device *device, const D3D11_TEXTURE3D_DESC *desc,
        const D3D11_SUBRESOURCE_DATA *data, struct d3d_texture3d **texture)
{
    struct d3d_texture3d *object;
    HRESULT hr;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d_texture3d_init(object, device, desc, data)))
    {
        WARN("Failed to initialise texture, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created texture %p.\n", object);
    *texture = object;

    return S_OK;
}
