/*
 * Copyright 2005 Oliver Stieber
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

#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

static inline struct d3d8_vertexbuffer *impl_from_IDirect3DVertexBuffer8(IDirect3DVertexBuffer8 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d8_vertexbuffer, IDirect3DVertexBuffer8_iface);
}

static HRESULT WINAPI d3d8_vertexbuffer_QueryInterface(IDirect3DVertexBuffer8 *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IDirect3DVertexBuffer8)
            || IsEqualGUID(riid, &IID_IDirect3DResource8)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DVertexBuffer8_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d8_vertexbuffer_AddRef(IDirect3DVertexBuffer8 *iface)
{
    struct d3d8_vertexbuffer *buffer = impl_from_IDirect3DVertexBuffer8(iface);
    ULONG refcount = InterlockedIncrement(&buffer->resource.refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    if (refcount == 1)
    {
        IDirect3DDevice8_AddRef(buffer->parent_device);
        wined3d_mutex_lock();
        if (buffer->draw_buffer)
            wined3d_buffer_incref(buffer->draw_buffer);
        else
            wined3d_buffer_incref(buffer->wined3d_buffer);
        wined3d_mutex_unlock();
    }

    return refcount;
}

static ULONG WINAPI d3d8_vertexbuffer_Release(IDirect3DVertexBuffer8 *iface)
{
    struct d3d8_vertexbuffer *buffer = impl_from_IDirect3DVertexBuffer8(iface);
    ULONG refcount = InterlockedDecrement(&buffer->resource.refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        struct wined3d_buffer *draw_buffer = buffer->draw_buffer;
        IDirect3DDevice8 *device = buffer->parent_device;

        wined3d_mutex_lock();
        if (draw_buffer)
            wined3d_buffer_decref(draw_buffer);
        else
            wined3d_buffer_decref(buffer->wined3d_buffer);
        wined3d_mutex_unlock();

        /* Release the device last, as it may cause the device to be destroyed. */
        IDirect3DDevice8_Release(device);
    }

    return refcount;
}

static HRESULT WINAPI d3d8_vertexbuffer_GetDevice(IDirect3DVertexBuffer8 *iface,
        IDirect3DDevice8 **device)
{
    struct d3d8_vertexbuffer *buffer = impl_from_IDirect3DVertexBuffer8(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = buffer->parent_device;
    IDirect3DDevice8_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI d3d8_vertexbuffer_SetPrivateData(IDirect3DVertexBuffer8 *iface,
        REFGUID guid, const void *data, DWORD data_size, DWORD flags)
{
    struct d3d8_vertexbuffer *buffer = impl_from_IDirect3DVertexBuffer8(iface);
    TRACE("iface %p, guid %s, data %p, data_size %u, flags %#x.\n",
            iface, debugstr_guid(guid), data, data_size, flags);

    return d3d8_resource_set_private_data(&buffer->resource, guid, data, data_size, flags);
}

static HRESULT WINAPI d3d8_vertexbuffer_GetPrivateData(IDirect3DVertexBuffer8 *iface,
        REFGUID guid, void *data, DWORD *data_size)
{
    struct d3d8_vertexbuffer *buffer = impl_from_IDirect3DVertexBuffer8(iface);
    TRACE("iface %p, guid %s, data %p, data_size %p.\n",
            iface, debugstr_guid(guid), data, data_size);

    return d3d8_resource_get_private_data(&buffer->resource, guid, data, data_size);
}

static HRESULT WINAPI d3d8_vertexbuffer_FreePrivateData(IDirect3DVertexBuffer8 *iface, REFGUID guid)
{
    struct d3d8_vertexbuffer *buffer = impl_from_IDirect3DVertexBuffer8(iface);
    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    return d3d8_resource_free_private_data(&buffer->resource, guid);
}

static DWORD WINAPI d3d8_vertexbuffer_SetPriority(IDirect3DVertexBuffer8 *iface, DWORD priority)
{
    struct d3d8_vertexbuffer *buffer = impl_from_IDirect3DVertexBuffer8(iface);
    struct wined3d_resource *resource;
    DWORD previous;

    TRACE("iface %p, priority %u.\n", iface, priority);

    wined3d_mutex_lock();
    resource = wined3d_buffer_get_resource(buffer->wined3d_buffer);
    previous = wined3d_resource_set_priority(resource, priority);
    wined3d_mutex_unlock();

    return previous;
}

static DWORD WINAPI d3d8_vertexbuffer_GetPriority(IDirect3DVertexBuffer8 *iface)
{
    struct d3d8_vertexbuffer *buffer = impl_from_IDirect3DVertexBuffer8(iface);
    const struct wined3d_resource *resource;
    DWORD priority;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    resource = wined3d_buffer_get_resource(buffer->wined3d_buffer);
    priority = wined3d_resource_get_priority(resource);
    wined3d_mutex_unlock();

    return priority;
}

static void WINAPI d3d8_vertexbuffer_PreLoad(IDirect3DVertexBuffer8 *iface)
{
    struct d3d8_vertexbuffer *buffer = impl_from_IDirect3DVertexBuffer8(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_resource_preload(wined3d_buffer_get_resource(buffer->wined3d_buffer));
    wined3d_mutex_unlock();
}

static D3DRESOURCETYPE WINAPI d3d8_vertexbuffer_GetType(IDirect3DVertexBuffer8 *iface)
{
    TRACE("iface %p.\n", iface);

    return D3DRTYPE_VERTEXBUFFER;
}

static HRESULT WINAPI d3d8_vertexbuffer_Lock(IDirect3DVertexBuffer8 *iface, UINT offset, UINT size,
        BYTE **data, DWORD flags)
{
    struct d3d8_vertexbuffer *buffer = impl_from_IDirect3DVertexBuffer8(iface);
    struct wined3d_resource *wined3d_resource;
    struct wined3d_map_desc wined3d_map_desc;
    struct wined3d_box wined3d_box = {0};
    HRESULT hr;

    TRACE("iface %p, offset %u, size %u, data %p, flags %#x.\n",
            iface, offset, size, data, flags);

    wined3d_box.left = offset;
    wined3d_box.right = offset + size;
    wined3d_mutex_lock();
    wined3d_resource = wined3d_buffer_get_resource(buffer->wined3d_buffer);
    hr = wined3d_resource_map(wined3d_resource, 0, &wined3d_map_desc, &wined3d_box,
            wined3dmapflags_from_d3dmapflags(flags, buffer->usage));
    wined3d_mutex_unlock();
    *data = wined3d_map_desc.data;

    return hr;
}

static HRESULT WINAPI d3d8_vertexbuffer_Unlock(IDirect3DVertexBuffer8 *iface)
{
    struct d3d8_vertexbuffer *buffer = impl_from_IDirect3DVertexBuffer8(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_resource_unmap(wined3d_buffer_get_resource(buffer->wined3d_buffer), 0);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_vertexbuffer_GetDesc(IDirect3DVertexBuffer8 *iface,
        D3DVERTEXBUFFER_DESC *desc)
{
    struct d3d8_vertexbuffer *buffer = impl_from_IDirect3DVertexBuffer8(iface);
    struct wined3d_resource_desc wined3d_desc;
    struct wined3d_resource *wined3d_resource;

    TRACE("iface %p, desc %p.\n", iface, desc);

    wined3d_mutex_lock();
    wined3d_resource = wined3d_buffer_get_resource(buffer->wined3d_buffer);
    wined3d_resource_get_desc(wined3d_resource, &wined3d_desc);
    wined3d_mutex_unlock();

    desc->Format = D3DFMT_VERTEXDATA;
    desc->Type = D3DRTYPE_VERTEXBUFFER;
    desc->Usage = buffer->usage;
    desc->Pool = d3dpool_from_wined3daccess(wined3d_desc.access, wined3d_desc.usage);
    desc->Size = wined3d_desc.size;
    desc->FVF = buffer->fvf;

    return D3D_OK;
}

static const IDirect3DVertexBuffer8Vtbl Direct3DVertexBuffer8_Vtbl =
{
    /* IUnknown */
    d3d8_vertexbuffer_QueryInterface,
    d3d8_vertexbuffer_AddRef,
    d3d8_vertexbuffer_Release,
    /* IDirect3DResource8 */
    d3d8_vertexbuffer_GetDevice,
    d3d8_vertexbuffer_SetPrivateData,
    d3d8_vertexbuffer_GetPrivateData,
    d3d8_vertexbuffer_FreePrivateData,
    d3d8_vertexbuffer_SetPriority,
    d3d8_vertexbuffer_GetPriority,
    d3d8_vertexbuffer_PreLoad,
    d3d8_vertexbuffer_GetType,
    /* IDirect3DVertexBuffer8 */
    d3d8_vertexbuffer_Lock,
    d3d8_vertexbuffer_Unlock,
    d3d8_vertexbuffer_GetDesc,
};

static void STDMETHODCALLTYPE d3d8_vertexbuffer_wined3d_object_destroyed(void *parent)
{
    struct d3d8_vertexbuffer *buffer = parent;

    if (buffer->draw_buffer)
        wined3d_buffer_decref(buffer->wined3d_buffer);
    d3d8_resource_cleanup(&buffer->resource);
    heap_free(buffer);
}

static const struct wined3d_parent_ops d3d8_vertexbuffer_wined3d_parent_ops =
{
    d3d8_vertexbuffer_wined3d_object_destroyed,
};

HRESULT vertexbuffer_init(struct d3d8_vertexbuffer *buffer, struct d3d8_device *device,
        UINT size, DWORD usage, DWORD fvf, D3DPOOL pool)
{
    const struct wined3d_parent_ops *parent_ops = &d3d8_null_wined3d_parent_ops;
    struct wined3d_buffer_desc desc;
    HRESULT hr;

    if (pool == D3DPOOL_SCRATCH)
    {
        WARN("Vertex buffer with D3DPOOL_SCRATCH requested.\n");
        return D3DERR_INVALIDCALL;
    }

    /* In d3d8, buffers can't be used as rendertarget or depth/stencil buffer. */
    if (usage & (D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL))
        return D3DERR_INVALIDCALL;

    buffer->IDirect3DVertexBuffer8_iface.lpVtbl = &Direct3DVertexBuffer8_Vtbl;
    d3d8_resource_init(&buffer->resource);
    buffer->fvf = fvf;
    buffer->usage = usage;

    desc.byte_width = size;
    desc.usage = usage & WINED3DUSAGE_MASK;
    desc.bind_flags = 0;
    desc.access = wined3daccess_from_d3dpool(pool, usage) | map_access_from_usage(usage);
    /* Buffers are always readable. */
    if (pool != D3DPOOL_DEFAULT)
        desc.access |= WINED3D_RESOURCE_ACCESS_MAP_R | WINED3D_RESOURCE_ACCESS_MAP_W;
    desc.misc_flags = 0;
    desc.structure_byte_stride = 0;

    if (desc.access & WINED3D_RESOURCE_ACCESS_GPU)
    {
        desc.bind_flags = WINED3D_BIND_VERTEX_BUFFER;
        parent_ops = &d3d8_vertexbuffer_wined3d_parent_ops;
    }

    wined3d_mutex_lock();
    hr = wined3d_buffer_create(device->wined3d_device, &desc, NULL, buffer, parent_ops, &buffer->wined3d_buffer);
    if (SUCCEEDED(hr) && !(desc.access & WINED3D_RESOURCE_ACCESS_GPU))
    {
        desc.bind_flags = WINED3D_BIND_VERTEX_BUFFER;
        desc.access = WINED3D_RESOURCE_ACCESS_GPU;
        if (FAILED(hr = wined3d_buffer_create(device->wined3d_device, &desc, NULL, buffer,
                &d3d8_vertexbuffer_wined3d_parent_ops, &buffer->draw_buffer)))
            wined3d_buffer_decref(buffer->wined3d_buffer);
    }
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d buffer, hr %#x.\n", hr);
        return hr;
    }

    buffer->parent_device = &device->IDirect3DDevice8_iface;
    IDirect3DDevice8_AddRef(buffer->parent_device);

    return D3D_OK;
}

struct d3d8_vertexbuffer *unsafe_impl_from_IDirect3DVertexBuffer8(IDirect3DVertexBuffer8 *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &Direct3DVertexBuffer8_Vtbl);

    return impl_from_IDirect3DVertexBuffer8(iface);
}

static inline struct d3d8_indexbuffer *impl_from_IDirect3DIndexBuffer8(IDirect3DIndexBuffer8 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d8_indexbuffer, IDirect3DIndexBuffer8_iface);
}

static HRESULT WINAPI d3d8_indexbuffer_QueryInterface(IDirect3DIndexBuffer8 *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IDirect3DIndexBuffer8)
            || IsEqualGUID(riid, &IID_IDirect3DResource8)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DIndexBuffer8_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d8_indexbuffer_AddRef(IDirect3DIndexBuffer8 *iface)
{
    struct d3d8_indexbuffer *buffer = impl_from_IDirect3DIndexBuffer8(iface);
    ULONG refcount = InterlockedIncrement(&buffer->resource.refcount);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    if (refcount == 1)
    {
        IDirect3DDevice8_AddRef(buffer->parent_device);
        wined3d_mutex_lock();
        if (buffer->draw_buffer)
            wined3d_buffer_incref(buffer->draw_buffer);
        else
            wined3d_buffer_incref(buffer->wined3d_buffer);
        wined3d_mutex_unlock();
    }

    return refcount;
}

static ULONG WINAPI d3d8_indexbuffer_Release(IDirect3DIndexBuffer8 *iface)
{
    struct d3d8_indexbuffer *buffer = impl_from_IDirect3DIndexBuffer8(iface);
    ULONG refcount = InterlockedDecrement(&buffer->resource.refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        struct wined3d_buffer *draw_buffer = buffer->draw_buffer;
        IDirect3DDevice8 *device = buffer->parent_device;

        wined3d_mutex_lock();
        if (draw_buffer)
            wined3d_buffer_decref(draw_buffer);
        else
            wined3d_buffer_decref(buffer->wined3d_buffer);
        wined3d_mutex_unlock();

        /* Release the device last, as it may cause the device to be destroyed. */
        IDirect3DDevice8_Release(device);
    }

    return refcount;
}

static HRESULT WINAPI d3d8_indexbuffer_GetDevice(IDirect3DIndexBuffer8 *iface,
        IDirect3DDevice8 **device)
{
    struct d3d8_indexbuffer *buffer = impl_from_IDirect3DIndexBuffer8(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = buffer->parent_device;
    IDirect3DDevice8_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI d3d8_indexbuffer_SetPrivateData(IDirect3DIndexBuffer8 *iface,
        REFGUID guid, const void *data, DWORD data_size, DWORD flags)
{
    struct d3d8_indexbuffer *buffer = impl_from_IDirect3DIndexBuffer8(iface);
    TRACE("iface %p, guid %s, data %p, data_size %u, flags %#x.\n",
            iface, debugstr_guid(guid), data, data_size, flags);

    return d3d8_resource_set_private_data(&buffer->resource, guid, data, data_size, flags);
}

static HRESULT WINAPI d3d8_indexbuffer_GetPrivateData(IDirect3DIndexBuffer8 *iface,
        REFGUID guid, void *data, DWORD *data_size)
{
    struct d3d8_indexbuffer *buffer = impl_from_IDirect3DIndexBuffer8(iface);
    TRACE("iface %p, guid %s, data %p, data_size %p.\n",
            iface, debugstr_guid(guid), data, data_size);

    return d3d8_resource_get_private_data(&buffer->resource, guid, data, data_size);
}

static HRESULT WINAPI d3d8_indexbuffer_FreePrivateData(IDirect3DIndexBuffer8 *iface, REFGUID guid)
{
    struct d3d8_indexbuffer *buffer = impl_from_IDirect3DIndexBuffer8(iface);
    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    return d3d8_resource_free_private_data(&buffer->resource, guid);
}

static DWORD WINAPI d3d8_indexbuffer_SetPriority(IDirect3DIndexBuffer8 *iface, DWORD priority)
{
    struct d3d8_indexbuffer *buffer = impl_from_IDirect3DIndexBuffer8(iface);
    struct wined3d_resource *resource;
    DWORD previous;

    TRACE("iface %p, priority %u.\n", iface, priority);

    wined3d_mutex_lock();
    resource = wined3d_buffer_get_resource(buffer->wined3d_buffer);
    previous = wined3d_resource_set_priority(resource, priority);
    wined3d_mutex_unlock();

    return previous;
}

static DWORD WINAPI d3d8_indexbuffer_GetPriority(IDirect3DIndexBuffer8 *iface)
{
    struct d3d8_indexbuffer *buffer = impl_from_IDirect3DIndexBuffer8(iface);
    const struct wined3d_resource *resource;
    DWORD priority;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    resource = wined3d_buffer_get_resource(buffer->wined3d_buffer);
    priority = wined3d_resource_get_priority(resource);
    wined3d_mutex_unlock();

    return priority;
}

static void WINAPI d3d8_indexbuffer_PreLoad(IDirect3DIndexBuffer8 *iface)
{
    struct d3d8_indexbuffer *buffer = impl_from_IDirect3DIndexBuffer8(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_resource_preload(wined3d_buffer_get_resource(buffer->wined3d_buffer));
    wined3d_mutex_unlock();
}

static D3DRESOURCETYPE WINAPI d3d8_indexbuffer_GetType(IDirect3DIndexBuffer8 *iface)
{
    TRACE("iface %p.\n", iface);

    return D3DRTYPE_INDEXBUFFER;
}

static HRESULT WINAPI d3d8_indexbuffer_Lock(IDirect3DIndexBuffer8 *iface, UINT offset, UINT size,
        BYTE **data, DWORD flags)
{
    struct d3d8_indexbuffer *buffer = impl_from_IDirect3DIndexBuffer8(iface);
    struct wined3d_resource *wined3d_resource;
    struct wined3d_map_desc wined3d_map_desc;
    struct wined3d_box wined3d_box = {0};
    HRESULT hr;

    TRACE("iface %p, offset %u, size %u, data %p, flags %#x.\n",
            iface, offset, size, data, flags);

    wined3d_box.left = offset;
    wined3d_box.right = offset + size;
    wined3d_mutex_lock();
    wined3d_resource = wined3d_buffer_get_resource(buffer->wined3d_buffer);
    hr = wined3d_resource_map(wined3d_resource, 0, &wined3d_map_desc, &wined3d_box,
            wined3dmapflags_from_d3dmapflags(flags, buffer->usage));
    wined3d_mutex_unlock();
    *data = wined3d_map_desc.data;

    return hr;
}

static HRESULT WINAPI d3d8_indexbuffer_Unlock(IDirect3DIndexBuffer8 *iface)
{
    struct d3d8_indexbuffer *buffer = impl_from_IDirect3DIndexBuffer8(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_resource_unmap(wined3d_buffer_get_resource(buffer->wined3d_buffer), 0);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d8_indexbuffer_GetDesc(IDirect3DIndexBuffer8 *iface,
        D3DINDEXBUFFER_DESC *desc)
{
    struct d3d8_indexbuffer *buffer = impl_from_IDirect3DIndexBuffer8(iface);
    struct wined3d_resource_desc wined3d_desc;
    struct wined3d_resource *wined3d_resource;

    TRACE("iface %p, desc %p.\n", iface, desc);

    wined3d_mutex_lock();
    wined3d_resource = wined3d_buffer_get_resource(buffer->wined3d_buffer);
    wined3d_resource_get_desc(wined3d_resource, &wined3d_desc);
    wined3d_mutex_unlock();

    desc->Format = d3dformat_from_wined3dformat(buffer->format);
    desc->Type = D3DRTYPE_INDEXBUFFER;
    desc->Usage = buffer->usage;
    desc->Pool = d3dpool_from_wined3daccess(wined3d_desc.access, wined3d_desc.usage);
    desc->Size = wined3d_desc.size;

    return D3D_OK;
}

static const IDirect3DIndexBuffer8Vtbl d3d8_indexbuffer_vtbl =
{
    /* IUnknown */
    d3d8_indexbuffer_QueryInterface,
    d3d8_indexbuffer_AddRef,
    d3d8_indexbuffer_Release,
    /* IDirect3DResource8 */
    d3d8_indexbuffer_GetDevice,
    d3d8_indexbuffer_SetPrivateData,
    d3d8_indexbuffer_GetPrivateData,
    d3d8_indexbuffer_FreePrivateData,
    d3d8_indexbuffer_SetPriority,
    d3d8_indexbuffer_GetPriority,
    d3d8_indexbuffer_PreLoad,
    d3d8_indexbuffer_GetType,
    /* IDirect3DIndexBuffer8 */
    d3d8_indexbuffer_Lock,
    d3d8_indexbuffer_Unlock,
    d3d8_indexbuffer_GetDesc,
};

static void STDMETHODCALLTYPE d3d8_indexbuffer_wined3d_object_destroyed(void *parent)
{
    struct d3d8_indexbuffer *buffer = parent;

    if (buffer->draw_buffer)
        wined3d_buffer_decref(buffer->wined3d_buffer);
    d3d8_resource_cleanup(&buffer->resource);
    heap_free(buffer);
}

static const struct wined3d_parent_ops d3d8_indexbuffer_wined3d_parent_ops =
{
    d3d8_indexbuffer_wined3d_object_destroyed,
};

HRESULT indexbuffer_init(struct d3d8_indexbuffer *buffer, struct d3d8_device *device,
        UINT size, DWORD usage, D3DFORMAT format, D3DPOOL pool)
{
    const struct wined3d_parent_ops *parent_ops = &d3d8_null_wined3d_parent_ops;
    struct wined3d_buffer_desc desc;
    HRESULT hr;

    if (pool == D3DPOOL_SCRATCH)
        return D3DERR_INVALIDCALL;

    /* In d3d8, buffers can't be used as rendertarget or depth/stencil buffer. */
    if (usage & (D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL))
        return D3DERR_INVALIDCALL;

    desc.byte_width = size;
    desc.usage = (usage & WINED3DUSAGE_MASK) | WINED3DUSAGE_STATICDECL;
    desc.bind_flags = 0;
    desc.access = wined3daccess_from_d3dpool(pool, usage) | map_access_from_usage(usage);
    /* Buffers are always readable. */
    if (pool != D3DPOOL_DEFAULT)
        desc.access |= WINED3D_RESOURCE_ACCESS_MAP_R | WINED3D_RESOURCE_ACCESS_MAP_W;
    desc.misc_flags = 0;
    desc.structure_byte_stride = 0;

    if (desc.access & WINED3D_RESOURCE_ACCESS_GPU)
    {
        desc.bind_flags = WINED3D_BIND_INDEX_BUFFER;
        parent_ops = &d3d8_indexbuffer_wined3d_parent_ops;
    }

    buffer->IDirect3DIndexBuffer8_iface.lpVtbl = &d3d8_indexbuffer_vtbl;
    d3d8_resource_init(&buffer->resource);
    buffer->format = wined3dformat_from_d3dformat(format);
    buffer->usage = usage;

    wined3d_mutex_lock();
    hr = wined3d_buffer_create(device->wined3d_device, &desc, NULL, buffer, parent_ops, &buffer->wined3d_buffer);
    if (SUCCEEDED(hr) && !(desc.access & WINED3D_RESOURCE_ACCESS_GPU))
    {
        desc.bind_flags = WINED3D_BIND_INDEX_BUFFER;
        desc.access = WINED3D_RESOURCE_ACCESS_GPU;
        if (FAILED(hr = wined3d_buffer_create(device->wined3d_device, &desc, NULL, buffer,
                &d3d8_indexbuffer_wined3d_parent_ops, &buffer->draw_buffer)))
            wined3d_buffer_decref(buffer->wined3d_buffer);
    }
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d buffer, hr %#x.\n", hr);
        return hr;
    }

    buffer->parent_device = &device->IDirect3DDevice8_iface;
    IDirect3DDevice8_AddRef(buffer->parent_device);

    return D3D_OK;
}

struct d3d8_indexbuffer *unsafe_impl_from_IDirect3DIndexBuffer8(IDirect3DIndexBuffer8 *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d8_indexbuffer_vtbl);

    return impl_from_IDirect3DIndexBuffer8(iface);
}
