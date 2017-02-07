/*
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2002-2004 Raphael Junqueira
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

#include "config.h"
#include <assert.h>
#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

static inline IDirect3DVertexBuffer9Impl *impl_from_IDirect3DVertexBuffer9(IDirect3DVertexBuffer9 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DVertexBuffer9Impl, IDirect3DVertexBuffer9_iface);
}

static HRESULT WINAPI d3d9_vertexbuffer_QueryInterface(IDirect3DVertexBuffer9 *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IDirect3DVertexBuffer9)
            || IsEqualGUID(riid, &IID_IDirect3DResource9)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DVertexBuffer9_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d9_vertexbuffer_AddRef(IDirect3DVertexBuffer9 *iface)
{
    IDirect3DVertexBuffer9Impl *buffer = impl_from_IDirect3DVertexBuffer9(iface);
    ULONG refcount = InterlockedIncrement(&buffer->ref);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    if (refcount == 1)
    {
        IDirect3DDevice9Ex_AddRef(buffer->parentDevice);
        wined3d_mutex_lock();
        wined3d_buffer_incref(buffer->wineD3DVertexBuffer);
        wined3d_mutex_unlock();
    }

    return refcount;
}

static ULONG WINAPI d3d9_vertexbuffer_Release(IDirect3DVertexBuffer9 *iface)
{
    IDirect3DVertexBuffer9Impl *buffer = impl_from_IDirect3DVertexBuffer9(iface);
    ULONG refcount = InterlockedDecrement(&buffer->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        IDirect3DDevice9Ex *device = buffer->parentDevice;

        wined3d_mutex_lock();
        wined3d_buffer_decref(buffer->wineD3DVertexBuffer);
        wined3d_mutex_unlock();

        /* Release the device last, as it may cause the device to be destroyed. */
        IDirect3DDevice9Ex_Release(device);
    }

    return refcount;
}

static HRESULT WINAPI d3d9_vertexbuffer_GetDevice(IDirect3DVertexBuffer9 *iface,
        IDirect3DDevice9 **device)
{
    IDirect3DVertexBuffer9Impl *buffer = impl_from_IDirect3DVertexBuffer9(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)buffer->parentDevice;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI d3d9_vertexbuffer_SetPrivateData(IDirect3DVertexBuffer9 *iface,
        REFGUID guid, const void *data, DWORD data_size, DWORD flags)
{
    IDirect3DVertexBuffer9Impl *buffer = impl_from_IDirect3DVertexBuffer9(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %u, flags %#x.\n",
            iface, debugstr_guid(guid), data, data_size, flags);

    wined3d_mutex_lock();
    resource = wined3d_buffer_get_resource(buffer->wineD3DVertexBuffer);
    hr = wined3d_resource_set_private_data(resource, guid, data, data_size, flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_vertexbuffer_GetPrivateData(IDirect3DVertexBuffer9 *iface,
        REFGUID guid, void *data, DWORD *data_size)
{
    IDirect3DVertexBuffer9Impl *buffer = impl_from_IDirect3DVertexBuffer9(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %p.\n",
            iface, debugstr_guid(guid), data, data_size);

    wined3d_mutex_lock();
    resource = wined3d_buffer_get_resource(buffer->wineD3DVertexBuffer);
    hr = wined3d_resource_get_private_data(resource, guid, data, data_size);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_vertexbuffer_FreePrivateData(IDirect3DVertexBuffer9 *iface, REFGUID guid)
{
    IDirect3DVertexBuffer9Impl *buffer = impl_from_IDirect3DVertexBuffer9(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    wined3d_mutex_lock();
    resource = wined3d_buffer_get_resource(buffer->wineD3DVertexBuffer);
    hr = wined3d_resource_free_private_data(resource, guid);
    wined3d_mutex_unlock();

    return hr;
}

static DWORD WINAPI d3d9_vertexbuffer_SetPriority(IDirect3DVertexBuffer9 *iface, DWORD priority)
{
    IDirect3DVertexBuffer9Impl *buffer = impl_from_IDirect3DVertexBuffer9(iface);
    DWORD previous;

    TRACE("iface %p, priority %u.\n", iface, priority);

    wined3d_mutex_lock();
    previous = wined3d_buffer_set_priority(buffer->wineD3DVertexBuffer, priority);
    wined3d_mutex_unlock();

    return previous;
}

static DWORD WINAPI d3d9_vertexbuffer_GetPriority(IDirect3DVertexBuffer9 *iface)
{
    IDirect3DVertexBuffer9Impl *buffer = impl_from_IDirect3DVertexBuffer9(iface);
    DWORD priority;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    priority = wined3d_buffer_get_priority(buffer->wineD3DVertexBuffer);
    wined3d_mutex_unlock();

    return priority;
}

static void WINAPI d3d9_vertexbuffer_PreLoad(IDirect3DVertexBuffer9 *iface)
{
    IDirect3DVertexBuffer9Impl *buffer = impl_from_IDirect3DVertexBuffer9(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_buffer_preload(buffer->wineD3DVertexBuffer);
    wined3d_mutex_unlock();
}

static D3DRESOURCETYPE WINAPI d3d9_vertexbuffer_GetType(IDirect3DVertexBuffer9 *iface)
{
    TRACE("iface %p.\n", iface);

    return D3DRTYPE_VERTEXBUFFER;
}

static HRESULT WINAPI d3d9_vertexbuffer_Lock(IDirect3DVertexBuffer9 *iface, UINT offset, UINT size,
        void **data, DWORD flags)
{
    IDirect3DVertexBuffer9Impl *buffer = impl_from_IDirect3DVertexBuffer9(iface);
    HRESULT hr;

    TRACE("iface %p, offset %u, size %u, data %p, flags %#x.\n",
            iface, offset, size, data, flags);

    wined3d_mutex_lock();
    hr = wined3d_buffer_map(buffer->wineD3DVertexBuffer, offset, size, (BYTE **)data, flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_vertexbuffer_Unlock(IDirect3DVertexBuffer9 *iface)
{
    IDirect3DVertexBuffer9Impl *buffer = impl_from_IDirect3DVertexBuffer9(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_buffer_unmap(buffer->wineD3DVertexBuffer);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_vertexbuffer_GetDesc(IDirect3DVertexBuffer9 *iface,
        D3DVERTEXBUFFER_DESC *desc)
{
    IDirect3DVertexBuffer9Impl *buffer = impl_from_IDirect3DVertexBuffer9(iface);
    struct wined3d_resource_desc wined3d_desc;
    struct wined3d_resource *wined3d_resource;

    TRACE("iface %p, desc %p.\n", iface, desc);

    wined3d_mutex_lock();
    wined3d_resource = wined3d_buffer_get_resource(buffer->wineD3DVertexBuffer);
    wined3d_resource_get_desc(wined3d_resource, &wined3d_desc);
    wined3d_mutex_unlock();

    desc->Format = D3DFMT_VERTEXDATA;
    desc->Usage = wined3d_desc.usage & WINED3DUSAGE_MASK;
    desc->Pool = wined3d_desc.pool;
    desc->Size = wined3d_desc.size;
    desc->Type = D3DRTYPE_VERTEXBUFFER;
    desc->FVF = buffer->fvf;

    return D3D_OK;
}

static const IDirect3DVertexBuffer9Vtbl d3d9_vertexbuffer_vtbl =
{
    /* IUnknown */
    d3d9_vertexbuffer_QueryInterface,
    d3d9_vertexbuffer_AddRef,
    d3d9_vertexbuffer_Release,
    /* IDirect3DResource9 */
    d3d9_vertexbuffer_GetDevice,
    d3d9_vertexbuffer_SetPrivateData,
    d3d9_vertexbuffer_GetPrivateData,
    d3d9_vertexbuffer_FreePrivateData,
    d3d9_vertexbuffer_SetPriority,
    d3d9_vertexbuffer_GetPriority,
    d3d9_vertexbuffer_PreLoad,
    d3d9_vertexbuffer_GetType,
    /* IDirect3DVertexBuffer9 */
    d3d9_vertexbuffer_Lock,
    d3d9_vertexbuffer_Unlock,
    d3d9_vertexbuffer_GetDesc,
};

static void STDMETHODCALLTYPE d3d9_vertexbuffer_wined3d_object_destroyed(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d9_vertexbuffer_wined3d_parent_ops =
{
    d3d9_vertexbuffer_wined3d_object_destroyed,
};

HRESULT vertexbuffer_init(IDirect3DVertexBuffer9Impl *buffer, IDirect3DDevice9Impl *device,
        UINT size, UINT usage, DWORD fvf, D3DPOOL pool)
{
    HRESULT hr;

    buffer->IDirect3DVertexBuffer9_iface.lpVtbl = &d3d9_vertexbuffer_vtbl;
    buffer->ref = 1;
    buffer->fvf = fvf;

    wined3d_mutex_lock();
    hr = wined3d_buffer_create_vb(device->wined3d_device, size, usage & WINED3DUSAGE_MASK,
            (enum wined3d_pool)pool, buffer, &d3d9_vertexbuffer_wined3d_parent_ops, &buffer->wineD3DVertexBuffer);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d buffer, hr %#x.\n", hr);
        return hr;
    }

    buffer->parentDevice = &device->IDirect3DDevice9Ex_iface;
    IDirect3DDevice9Ex_AddRef(buffer->parentDevice);

    return D3D_OK;
}

IDirect3DVertexBuffer9Impl *unsafe_impl_from_IDirect3DVertexBuffer9(IDirect3DVertexBuffer9 *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d9_vertexbuffer_vtbl);

    return impl_from_IDirect3DVertexBuffer9(iface);
}

static inline IDirect3DIndexBuffer9Impl *impl_from_IDirect3DIndexBuffer9(IDirect3DIndexBuffer9 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DIndexBuffer9Impl, IDirect3DIndexBuffer9_iface);
}

static HRESULT WINAPI d3d9_indexbuffer_QueryInterface(IDirect3DIndexBuffer9 *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IDirect3DIndexBuffer9)
            || IsEqualGUID(riid, &IID_IDirect3DResource9)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DIndexBuffer9_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d9_indexbuffer_AddRef(IDirect3DIndexBuffer9 *iface)
{
    IDirect3DIndexBuffer9Impl *buffer = impl_from_IDirect3DIndexBuffer9(iface);
    ULONG refcount = InterlockedIncrement(&buffer->ref);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    if (refcount == 1)
    {
        IDirect3DDevice9Ex_AddRef(buffer->parentDevice);
        wined3d_mutex_lock();
        wined3d_buffer_incref(buffer->wineD3DIndexBuffer);
        wined3d_mutex_unlock();
    }

    return refcount;
}

static ULONG WINAPI d3d9_indexbuffer_Release(IDirect3DIndexBuffer9 *iface)
{
    IDirect3DIndexBuffer9Impl *buffer = impl_from_IDirect3DIndexBuffer9(iface);
    ULONG refcount = InterlockedDecrement(&buffer->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        IDirect3DDevice9Ex *device = buffer->parentDevice;

        wined3d_mutex_lock();
        wined3d_buffer_decref(buffer->wineD3DIndexBuffer);
        wined3d_mutex_unlock();

        /* Release the device last, as it may cause the device to be destroyed. */
        IDirect3DDevice9Ex_Release(device);
    }

    return refcount;
}

static HRESULT WINAPI d3d9_indexbuffer_GetDevice(IDirect3DIndexBuffer9 *iface,
        IDirect3DDevice9 **device)
{
    IDirect3DIndexBuffer9Impl *buffer = impl_from_IDirect3DIndexBuffer9(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)buffer->parentDevice;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI d3d9_indexbuffer_SetPrivateData(IDirect3DIndexBuffer9 *iface,
        REFGUID guid, const void *data, DWORD data_size, DWORD flags)
{
    IDirect3DIndexBuffer9Impl *buffer = impl_from_IDirect3DIndexBuffer9(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %u, flags %#x.\n",
            iface, debugstr_guid(guid), data, data_size, flags);

    wined3d_mutex_lock();
    resource = wined3d_buffer_get_resource(buffer->wineD3DIndexBuffer);
    hr = wined3d_resource_set_private_data(resource, guid, data, data_size, flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_indexbuffer_GetPrivateData(IDirect3DIndexBuffer9 *iface,
        REFGUID guid, void *data, DWORD *data_size)
{
    IDirect3DIndexBuffer9Impl *buffer = impl_from_IDirect3DIndexBuffer9(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %p.\n",
            iface, debugstr_guid(guid), data, data_size);

    wined3d_mutex_lock();
    resource = wined3d_buffer_get_resource(buffer->wineD3DIndexBuffer);
    hr = wined3d_resource_get_private_data(resource, guid, data, data_size);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_indexbuffer_FreePrivateData(IDirect3DIndexBuffer9 *iface, REFGUID guid)
{
    IDirect3DIndexBuffer9Impl *buffer = impl_from_IDirect3DIndexBuffer9(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    wined3d_mutex_lock();
    resource = wined3d_buffer_get_resource(buffer->wineD3DIndexBuffer);
    hr = wined3d_resource_free_private_data(resource, guid);
    wined3d_mutex_unlock();

    return hr;
}

static DWORD WINAPI d3d9_indexbuffer_SetPriority(IDirect3DIndexBuffer9 *iface, DWORD priority)
{
    IDirect3DIndexBuffer9Impl *buffer = impl_from_IDirect3DIndexBuffer9(iface);
    DWORD previous;

    TRACE("iface %p, priority %u.\n", iface, priority);

    wined3d_mutex_lock();
    previous = wined3d_buffer_set_priority(buffer->wineD3DIndexBuffer, priority);
    wined3d_mutex_unlock();

    return previous;
}

static DWORD WINAPI d3d9_indexbuffer_GetPriority(IDirect3DIndexBuffer9 *iface)
{
    IDirect3DIndexBuffer9Impl *buffer = impl_from_IDirect3DIndexBuffer9(iface);
    DWORD priority;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    priority = wined3d_buffer_get_priority(buffer->wineD3DIndexBuffer);
    wined3d_mutex_unlock();

    return priority;
}

static void WINAPI d3d9_indexbuffer_PreLoad(IDirect3DIndexBuffer9 *iface)
{
    IDirect3DIndexBuffer9Impl *buffer = impl_from_IDirect3DIndexBuffer9(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_buffer_preload(buffer->wineD3DIndexBuffer);
    wined3d_mutex_unlock();
}

static D3DRESOURCETYPE WINAPI d3d9_indexbuffer_GetType(IDirect3DIndexBuffer9 *iface)
{
    TRACE("iface %p.\n", iface);

    return D3DRTYPE_INDEXBUFFER;
}

static HRESULT WINAPI d3d9_indexbuffer_Lock(IDirect3DIndexBuffer9 *iface,
        UINT offset, UINT size, void **data, DWORD flags)
{
    IDirect3DIndexBuffer9Impl *buffer = impl_from_IDirect3DIndexBuffer9(iface);
    HRESULT hr;

    TRACE("iface %p, offset %u, size %u, data %p, flags %#x.\n",
            iface, offset, size, data, flags);

    wined3d_mutex_lock();
    hr = wined3d_buffer_map(buffer->wineD3DIndexBuffer, offset, size, (BYTE **)data, flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_indexbuffer_Unlock(IDirect3DIndexBuffer9 *iface)
{
    IDirect3DIndexBuffer9Impl *buffer = impl_from_IDirect3DIndexBuffer9(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_buffer_unmap(buffer->wineD3DIndexBuffer);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_indexbuffer_GetDesc(IDirect3DIndexBuffer9 *iface,
        D3DINDEXBUFFER_DESC *desc)
{
    IDirect3DIndexBuffer9Impl *buffer = impl_from_IDirect3DIndexBuffer9(iface);
    struct wined3d_resource_desc wined3d_desc;
    struct wined3d_resource *wined3d_resource;

    TRACE("iface %p, desc %p.\n", iface, desc);

    wined3d_mutex_lock();
    wined3d_resource = wined3d_buffer_get_resource(buffer->wineD3DIndexBuffer);
    wined3d_resource_get_desc(wined3d_resource, &wined3d_desc);
    wined3d_mutex_unlock();

    desc->Format = d3dformat_from_wined3dformat(buffer->format);
    desc->Usage = wined3d_desc.usage & WINED3DUSAGE_MASK;
    desc->Pool = wined3d_desc.pool;
    desc->Size = wined3d_desc.size;
    desc->Type = D3DRTYPE_INDEXBUFFER;

    return D3D_OK;
}

static const IDirect3DIndexBuffer9Vtbl d3d9_indexbuffer_vtbl =
{
    /* IUnknown */
    d3d9_indexbuffer_QueryInterface,
    d3d9_indexbuffer_AddRef,
    d3d9_indexbuffer_Release,
    /* IDirect3DResource9 */
    d3d9_indexbuffer_GetDevice,
    d3d9_indexbuffer_SetPrivateData,
    d3d9_indexbuffer_GetPrivateData,
    d3d9_indexbuffer_FreePrivateData,
    d3d9_indexbuffer_SetPriority,
    d3d9_indexbuffer_GetPriority,
    d3d9_indexbuffer_PreLoad,
    d3d9_indexbuffer_GetType,
    /* IDirect3DIndexBuffer9 */
    d3d9_indexbuffer_Lock,
    d3d9_indexbuffer_Unlock,
    d3d9_indexbuffer_GetDesc,
};

static void STDMETHODCALLTYPE d3d9_indexbuffer_wined3d_object_destroyed(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d9_indexbuffer_wined3d_parent_ops =
{
    d3d9_indexbuffer_wined3d_object_destroyed,
};

HRESULT indexbuffer_init(IDirect3DIndexBuffer9Impl *buffer, IDirect3DDevice9Impl *device,
        UINT size, DWORD usage, D3DFORMAT format, D3DPOOL pool)
{
    HRESULT hr;

    buffer->IDirect3DIndexBuffer9_iface.lpVtbl = &d3d9_indexbuffer_vtbl;
    buffer->ref = 1;
    buffer->format = wined3dformat_from_d3dformat(format);

    wined3d_mutex_lock();
    hr = wined3d_buffer_create_ib(device->wined3d_device, size, usage & WINED3DUSAGE_MASK,
            (enum wined3d_pool)pool, buffer, &d3d9_indexbuffer_wined3d_parent_ops, &buffer->wineD3DIndexBuffer);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d buffer, hr %#x.\n", hr);
        return hr;
    }

    buffer->parentDevice = &device->IDirect3DDevice9Ex_iface;
    IDirect3DDevice9Ex_AddRef(buffer->parentDevice);

    return D3D_OK;
}

IDirect3DIndexBuffer9Impl *unsafe_impl_from_IDirect3DIndexBuffer9(IDirect3DIndexBuffer9 *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d9_indexbuffer_vtbl);

    return impl_from_IDirect3DIndexBuffer9(iface);
}
