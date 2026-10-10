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

#include "d3d11_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d11);

/* ID3D11Buffer methods */

static inline struct d3d_buffer *impl_from_ID3D11Buffer(ID3D11Buffer *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_buffer, ID3D11Buffer_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_buffer_QueryInterface(ID3D11Buffer *iface, REFIID riid, void **out)
{
    struct d3d_buffer *buffer = impl_from_ID3D11Buffer(iface);
    HRESULT hr;

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_ID3D11Buffer)
            || IsEqualGUID(riid, &IID_ID3D11Resource)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D11Buffer_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_ID3D10Buffer)
            || IsEqualGUID(riid, &IID_ID3D10Resource)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild))
    {
        ID3D10Buffer_AddRef(&buffer->ID3D10Buffer_iface);
        *out = &buffer->ID3D10Buffer_iface;
        return S_OK;
    }

    if (FAILED(hr = IUnknown_QueryInterface(buffer->dxgi_resource, riid, out)))
    {
        WARN("%s not implemented, returning %#lx.\n", debugstr_guid(riid), hr);
        *out = NULL;
    }

    return hr;
}

static ULONG STDMETHODCALLTYPE d3d11_buffer_AddRef(ID3D11Buffer *iface)
{
    struct d3d_buffer *buffer = impl_from_ID3D11Buffer(iface);
    ULONG refcount = InterlockedIncrement(&buffer->refcount);

    TRACE("%p increasing refcount to %lu.\n", buffer, refcount);

    if (refcount == 1)
    {
        ID3D11Device2_AddRef(buffer->device);
        wined3d_buffer_incref(buffer->wined3d_buffer);
    }

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_buffer_Release(ID3D11Buffer *iface)
{
    struct d3d_buffer *buffer = impl_from_ID3D11Buffer(iface);
    ULONG refcount = InterlockedDecrement(&buffer->refcount);

    TRACE("%p decreasing refcount to %lu.\n", buffer, refcount);

    if (!refcount)
    {
        ID3D11Device2 *device = buffer->device;

        wined3d_buffer_decref(buffer->wined3d_buffer);
        /* Release the device last, it may cause the wined3d device to be
         * destroyed. */
        ID3D11Device2_Release(device);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_buffer_GetDevice(ID3D11Buffer *iface, ID3D11Device **device)
{
    struct d3d_buffer *buffer = impl_from_ID3D11Buffer(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (ID3D11Device *)buffer->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_buffer_GetPrivateData(ID3D11Buffer *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_buffer *buffer = impl_from_ID3D11Buffer(iface);
    IDXGIResource *dxgi_resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    if (SUCCEEDED(hr = IUnknown_QueryInterface(buffer->dxgi_resource, &IID_IDXGIResource, (void **)&dxgi_resource)))
    {
        hr = IDXGIResource_GetPrivateData(dxgi_resource, guid, data_size, data);
        IDXGIResource_Release(dxgi_resource);
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE d3d11_buffer_SetPrivateData(ID3D11Buffer *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_buffer *buffer = impl_from_ID3D11Buffer(iface);
    IDXGIResource *dxgi_resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    if (SUCCEEDED(hr = IUnknown_QueryInterface(buffer->dxgi_resource, &IID_IDXGIResource, (void **)&dxgi_resource)))
    {
        hr = IDXGIResource_SetPrivateData(dxgi_resource, guid, data_size, data);
        IDXGIResource_Release(dxgi_resource);
    }

    return hr;
}

static HRESULT STDMETHODCALLTYPE d3d11_buffer_SetPrivateDataInterface(ID3D11Buffer *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_buffer *buffer = impl_from_ID3D11Buffer(iface);
    IDXGIResource *dxgi_resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    if (SUCCEEDED(hr = IUnknown_QueryInterface(buffer->dxgi_resource, &IID_IDXGIResource, (void **)&dxgi_resource)))
    {
        hr = IDXGIResource_SetPrivateDataInterface(dxgi_resource, guid, data);
        IDXGIResource_Release(dxgi_resource);
    }

    return hr;
}

static void STDMETHODCALLTYPE d3d11_buffer_GetType(ID3D11Buffer *iface,
        D3D11_RESOURCE_DIMENSION *resource_dimension)
{
    TRACE("iface %p, resource_dimension %p.\n", iface, resource_dimension);

    *resource_dimension = D3D11_RESOURCE_DIMENSION_BUFFER;
}

static void STDMETHODCALLTYPE d3d11_buffer_SetEvictionPriority(ID3D11Buffer *iface, UINT eviction_priority)
{
    FIXME("iface %p, eviction_priority %#x stub!\n", iface, eviction_priority);
}

static UINT STDMETHODCALLTYPE d3d11_buffer_GetEvictionPriority(ID3D11Buffer *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

static void STDMETHODCALLTYPE d3d11_buffer_GetDesc(ID3D11Buffer *iface, D3D11_BUFFER_DESC *desc)
{
    struct d3d_buffer *buffer = impl_from_ID3D11Buffer(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    *desc = buffer->desc;
}

static const struct ID3D11BufferVtbl d3d11_buffer_vtbl =
{
    /* IUnknown methods */
    d3d11_buffer_QueryInterface,
    d3d11_buffer_AddRef,
    d3d11_buffer_Release,
    /* ID3D11DeviceChild methods */
    d3d11_buffer_GetDevice,
    d3d11_buffer_GetPrivateData,
    d3d11_buffer_SetPrivateData,
    d3d11_buffer_SetPrivateDataInterface,
    /* ID3D11Resource methods */
    d3d11_buffer_GetType,
    d3d11_buffer_SetEvictionPriority,
    d3d11_buffer_GetEvictionPriority,
    /* ID3D11Buffer methods */
    d3d11_buffer_GetDesc,
};

struct d3d_buffer *unsafe_impl_from_ID3D11Buffer(ID3D11Buffer *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d11_buffer_vtbl);
    return CONTAINING_RECORD(iface, struct d3d_buffer, ID3D11Buffer_iface);
}

/* ID3D10Buffer methods */

static inline struct d3d_buffer *impl_from_ID3D10Buffer(ID3D10Buffer *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_buffer, ID3D10Buffer_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_buffer_QueryInterface(ID3D10Buffer *iface, REFIID riid, void **out)
{
    struct d3d_buffer *buffer = impl_from_ID3D10Buffer(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    return d3d11_buffer_QueryInterface(&buffer->ID3D11Buffer_iface, riid, out);
}

static ULONG STDMETHODCALLTYPE d3d10_buffer_AddRef(ID3D10Buffer *iface)
{
    struct d3d_buffer *buffer = impl_from_ID3D10Buffer(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_buffer_AddRef(&buffer->ID3D11Buffer_iface);
}

static ULONG STDMETHODCALLTYPE d3d10_buffer_Release(ID3D10Buffer *iface)
{
    struct d3d_buffer *buffer = impl_from_ID3D10Buffer(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_buffer_Release(&buffer->ID3D11Buffer_iface);
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_buffer_GetDevice(ID3D10Buffer *iface, ID3D10Device **device)
{
    struct d3d_buffer *buffer = impl_from_ID3D10Buffer(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device2_QueryInterface(buffer->device, &IID_ID3D10Device, (void **)device);
}

static HRESULT STDMETHODCALLTYPE d3d10_buffer_GetPrivateData(ID3D10Buffer *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_buffer *buffer = impl_from_ID3D10Buffer(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d11_buffer_GetPrivateData(&buffer->ID3D11Buffer_iface, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_buffer_SetPrivateData(ID3D10Buffer *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_buffer *buffer = impl_from_ID3D10Buffer(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d11_buffer_SetPrivateData(&buffer->ID3D11Buffer_iface, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_buffer_SetPrivateDataInterface(ID3D10Buffer *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_buffer *buffer = impl_from_ID3D10Buffer(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d11_buffer_SetPrivateDataInterface(&buffer->ID3D11Buffer_iface, guid, data);
}

/* ID3D10Resource methods */

static void STDMETHODCALLTYPE d3d10_buffer_GetType(ID3D10Buffer *iface, D3D10_RESOURCE_DIMENSION *resource_dimension)
{
    TRACE("iface %p, resource_dimension %p\n", iface, resource_dimension);

    *resource_dimension = D3D10_RESOURCE_DIMENSION_BUFFER;
}

static void STDMETHODCALLTYPE d3d10_buffer_SetEvictionPriority(ID3D10Buffer *iface, UINT eviction_priority)
{
    FIXME("iface %p, eviction_priority %u stub!\n", iface, eviction_priority);
}

static UINT STDMETHODCALLTYPE d3d10_buffer_GetEvictionPriority(ID3D10Buffer *iface)
{
    FIXME("iface %p stub!\n", iface);

    return 0;
}

/* ID3D10Buffer methods */

static HRESULT STDMETHODCALLTYPE d3d10_buffer_Map(ID3D10Buffer *iface, D3D10_MAP map_type, UINT map_flags, void **data)
{
    struct d3d_buffer *buffer = impl_from_ID3D10Buffer(iface);
    struct wined3d_map_desc wined3d_map_desc;
    HRESULT hr;

    TRACE("iface %p, map_type %u, map_flags %#x, data %p.\n", iface, map_type, map_flags, data);

    if (map_flags)
        FIXME("Ignoring map_flags %#x.\n", map_flags);

    hr = wined3d_resource_map(wined3d_buffer_get_resource(buffer->wined3d_buffer), 0,
            &wined3d_map_desc, NULL, wined3d_map_flags_from_d3d10_map_type(map_type));
    *data = wined3d_map_desc.data;

    return hr;
}

static void STDMETHODCALLTYPE d3d10_buffer_Unmap(ID3D10Buffer *iface)
{
    struct d3d_buffer *buffer = impl_from_ID3D10Buffer(iface);

    TRACE("iface %p.\n", iface);

    wined3d_resource_unmap(wined3d_buffer_get_resource(buffer->wined3d_buffer), 0);
}

static void STDMETHODCALLTYPE d3d10_buffer_GetDesc(ID3D10Buffer *iface, D3D10_BUFFER_DESC *desc)
{
    struct d3d_buffer *buffer = impl_from_ID3D10Buffer(iface);
    const D3D11_BUFFER_DESC *d3d11_desc = &buffer->desc;

    TRACE("iface %p, desc %p.\n", iface, desc);

    desc->ByteWidth = d3d11_desc->ByteWidth;
    desc->Usage = d3d10_usage_from_d3d11_usage(d3d11_desc->Usage);
    desc->BindFlags = d3d10_bind_flags_from_d3d11_bind_flags(d3d11_desc->BindFlags);
    desc->CPUAccessFlags = d3d10_cpu_access_flags_from_d3d11_cpu_access_flags(d3d11_desc->CPUAccessFlags);
    desc->MiscFlags = d3d10_resource_misc_flags_from_d3d11_resource_misc_flags(d3d11_desc->MiscFlags);
}

static const struct ID3D10BufferVtbl d3d10_buffer_vtbl =
{
    /* IUnknown methods */
    d3d10_buffer_QueryInterface,
    d3d10_buffer_AddRef,
    d3d10_buffer_Release,
    /* ID3D10DeviceChild methods */
    d3d10_buffer_GetDevice,
    d3d10_buffer_GetPrivateData,
    d3d10_buffer_SetPrivateData,
    d3d10_buffer_SetPrivateDataInterface,
    /* ID3D10Resource methods */
    d3d10_buffer_GetType,
    d3d10_buffer_SetEvictionPriority,
    d3d10_buffer_GetEvictionPriority,
    /* ID3D10Buffer methods */
    d3d10_buffer_Map,
    d3d10_buffer_Unmap,
    d3d10_buffer_GetDesc,
};

struct d3d_buffer *unsafe_impl_from_ID3D10Buffer(ID3D10Buffer *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d10_buffer_vtbl);
    return CONTAINING_RECORD(iface, struct d3d_buffer, ID3D10Buffer_iface);
}

static void STDMETHODCALLTYPE d3d_buffer_wined3d_object_released(void *parent)
{
    struct d3d_buffer *buffer = parent;

    if (buffer->dxgi_resource) IUnknown_Release(buffer->dxgi_resource);
    free(parent);
}

static const struct wined3d_parent_ops d3d_buffer_wined3d_parent_ops =
{
    d3d_buffer_wined3d_object_released,
};

static BOOL validate_buffer_desc(D3D11_BUFFER_DESC *desc, D3D_FEATURE_LEVEL feature_level)
{
    if (!validate_d3d11_resource_access_flags(D3D11_RESOURCE_DIMENSION_BUFFER,
            desc->Usage, desc->BindFlags, desc->CPUAccessFlags, feature_level))
        return FALSE;

    if (desc->MiscFlags & D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS)
    {
        if (desc->MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
        {
            WARN("Raw and structure buffers are mutually exclusive.\n");
            return FALSE;
        }
        if (!(desc->BindFlags & (D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS)))
        {
            WARN("Invalid bind flags %#x for raw buffer.\n", desc->BindFlags);
            return FALSE;
        }
    }

    if (desc->MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED)
    {
        if (!desc->StructureByteStride || desc->StructureByteStride % 4)
        {
            WARN("Invalid structure byte stride %u.\n", desc->StructureByteStride);
            return FALSE;
        }
        if (desc->ByteWidth % desc->StructureByteStride)
        {
            WARN("Byte width %u is not divisible by structure byte stride %u.\n",
                    desc->ByteWidth, desc->StructureByteStride);
            return FALSE;
        }
    }
    else
    {
        desc->StructureByteStride = 0;
    }

    if (desc->MiscFlags & D3D11_RESOURCE_MISC_GENERATE_MIPS)
    {
        WARN("Buffer with the D3D11_RESOURCE_MISC_GENERATE_MIPS flag.\n");
        return FALSE;
    }

    return TRUE;
}

static HRESULT d3d_buffer_init(struct d3d_buffer *buffer, struct d3d_device *device,
        const D3D11_BUFFER_DESC *desc, const D3D11_SUBRESOURCE_DATA *data)
{
    struct wined3d_buffer_desc wined3d_desc;
    HRESULT hr;

    buffer->ID3D11Buffer_iface.lpVtbl = &d3d11_buffer_vtbl;
    buffer->ID3D10Buffer_iface.lpVtbl = &d3d10_buffer_vtbl;
    buffer->refcount = 1;
    buffer->desc = *desc;

    if (!validate_buffer_desc(&buffer->desc, device->state->feature_level))
        return E_INVALIDARG;

    wined3d_desc.byte_width = buffer->desc.ByteWidth;
    wined3d_desc.usage = wined3d_usage_from_d3d11(buffer->desc.Usage);
    wined3d_desc.bind_flags = wined3d_bind_flags_from_d3d11(buffer->desc.BindFlags, buffer->desc.MiscFlags);
    wined3d_desc.access = wined3d_access_from_d3d11(buffer->desc.Usage, buffer->desc.CPUAccessFlags);
    wined3d_desc.misc_flags = buffer->desc.MiscFlags;
    wined3d_desc.structure_byte_stride = buffer->desc.StructureByteStride;

    wined3d_mutex_lock();

    if (FAILED(hr = wined3d_buffer_create(device->wined3d_device, &wined3d_desc,
            (const struct wined3d_sub_resource_data *)data, buffer,
            &d3d_buffer_wined3d_parent_ops, &buffer->wined3d_buffer)))
    {
        WARN("Failed to create wined3d buffer, hr %#lx.\n", hr);
        wined3d_mutex_unlock();
        return hr;
    }

    hr = d3d_device_create_dxgi_resource((IUnknown *)&device->ID3D10Device1_iface,
            wined3d_buffer_get_resource(buffer->wined3d_buffer), (IUnknown *)&buffer->ID3D10Buffer_iface,
            FALSE, &buffer->dxgi_resource);
    if (FAILED(hr))
    {
        ERR("Failed to create DXGI resource, returning %#.lx\n", hr);
        wined3d_buffer_decref(buffer->wined3d_buffer);
        wined3d_mutex_unlock();
        return hr;
    }
    wined3d_mutex_unlock();

    ID3D11Device2_AddRef(buffer->device = &device->ID3D11Device2_iface);

    return S_OK;
}

HRESULT d3d_buffer_create(struct d3d_device *device, const D3D11_BUFFER_DESC *desc,
        const D3D11_SUBRESOURCE_DATA *data, struct d3d_buffer **buffer)
{
    struct d3d_buffer *object;
    HRESULT hr;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d_buffer_init(object, device, desc, data)))
    {
        WARN("Failed to initialise buffer, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created buffer %p.\n", object);
    *buffer = object;

    return S_OK;
}
