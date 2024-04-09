/*
 * Copyright 2009 Henri Verbeet for CodeWeavers
 * Copyright 2015-2017 Józef Kucia for CodeWeavers
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

/* ID3D11Query methods */

static inline struct d3d_query *impl_from_ID3D11Query(ID3D11Query *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_query, ID3D11Query_iface);
}

static HRESULT STDMETHODCALLTYPE d3d11_query_QueryInterface(ID3D11Query *iface, REFIID riid, void **object)
{
    struct d3d_query *query = impl_from_ID3D11Query(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if ((IsEqualGUID(riid, &IID_ID3D11Predicate) && query->predicate)
            || IsEqualGUID(riid, &IID_ID3D11Query)
            || IsEqualGUID(riid, &IID_ID3D11Asynchronous)
            || IsEqualGUID(riid, &IID_ID3D11DeviceChild)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        ID3D11Query_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    if ((IsEqualGUID(riid, &IID_ID3D10Predicate) && query->predicate)
            || IsEqualGUID(riid, &IID_ID3D10Query)
            || IsEqualGUID(riid, &IID_ID3D10Asynchronous)
            || IsEqualGUID(riid, &IID_ID3D10DeviceChild))
    {
        ID3D10Query_AddRef(&query->ID3D10Query_iface);
        *object = &query->ID3D10Query_iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE d3d11_query_AddRef(ID3D11Query *iface)
{
    struct d3d_query *query = impl_from_ID3D11Query(iface);
    ULONG refcount = InterlockedIncrement(&query->refcount);

    TRACE("%p increasing refcount to %u.\n", query, refcount);

    if (refcount == 1)
    {
        ID3D11Device2_AddRef(query->device);
        wined3d_mutex_lock();
        wined3d_query_incref(query->wined3d_query);
        wined3d_mutex_unlock();
    }

    return refcount;
}

static ULONG STDMETHODCALLTYPE d3d11_query_Release(ID3D11Query *iface)
{
    struct d3d_query *query = impl_from_ID3D11Query(iface);
    ULONG refcount = InterlockedDecrement(&query->refcount);

    TRACE("%p decreasing refcount to %u.\n", query, refcount);

    if (!refcount)
    {
        ID3D11Device2 *device = query->device;

        wined3d_mutex_lock();
        wined3d_query_decref(query->wined3d_query);
        wined3d_mutex_unlock();

        ID3D11Device2_Release(device);
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d11_query_GetDevice(ID3D11Query *iface, ID3D11Device **device)
{
    struct d3d_query *query = impl_from_ID3D11Query(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (ID3D11Device *)query->device;
    ID3D11Device_AddRef(*device);
}

static HRESULT STDMETHODCALLTYPE d3d11_query_GetPrivateData(ID3D11Query *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_query *query = impl_from_ID3D11Query(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&query->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_query_SetPrivateData(ID3D11Query *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_query *query = impl_from_ID3D11Query(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n", iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&query->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d11_query_SetPrivateDataInterface(ID3D11Query *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_query *query = impl_from_ID3D11Query(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&query->private_store, guid, data);
}

static UINT STDMETHODCALLTYPE d3d11_query_GetDataSize(ID3D11Query *iface)
{
    struct d3d_query *query = impl_from_ID3D11Query(iface);
    unsigned int data_size;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    data_size = wined3d_query_get_data_size(query->wined3d_query);
    wined3d_mutex_unlock();

    return data_size;
}

static void STDMETHODCALLTYPE d3d11_query_GetDesc(ID3D11Query *iface, D3D11_QUERY_DESC *desc)
{
    struct d3d_query *query = impl_from_ID3D11Query(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    *desc = query->desc;
}

static const struct ID3D11QueryVtbl d3d11_query_vtbl =
{
    /* IUnknown methods */
    d3d11_query_QueryInterface,
    d3d11_query_AddRef,
    d3d11_query_Release,
    /* ID3D11DeviceChild methods */
    d3d11_query_GetDevice,
    d3d11_query_GetPrivateData,
    d3d11_query_SetPrivateData,
    d3d11_query_SetPrivateDataInterface,
    /* ID3D11Asynchronous methods */
    d3d11_query_GetDataSize,
    /* ID3D11Query methods */
    d3d11_query_GetDesc,
};

static void STDMETHODCALLTYPE d3d_query_wined3d_object_destroyed(void *parent)
{
    struct d3d_query *query = parent;

    wined3d_private_store_cleanup(&query->private_store);
    heap_free(parent);
}

static const struct wined3d_parent_ops d3d_query_wined3d_parent_ops =
{
    d3d_query_wined3d_object_destroyed,
};

struct d3d_query *unsafe_impl_from_ID3D11Query(ID3D11Query *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d11_query_vtbl);
    return CONTAINING_RECORD(iface, struct d3d_query, ID3D11Query_iface);
}

struct d3d_query *unsafe_impl_from_ID3D11Asynchronous(ID3D11Asynchronous *iface)
{
    return unsafe_impl_from_ID3D11Query((ID3D11Query *)iface);
}

/* ID3D10Query methods */

static inline struct d3d_query *impl_from_ID3D10Query(ID3D10Query *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_query, ID3D10Query_iface);
}

/* IUnknown methods */

static HRESULT STDMETHODCALLTYPE d3d10_query_QueryInterface(ID3D10Query *iface, REFIID riid, void **object)
{
    struct d3d_query *query = impl_from_ID3D10Query(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    return d3d11_query_QueryInterface(&query->ID3D11Query_iface, riid, object);
}

static ULONG STDMETHODCALLTYPE d3d10_query_AddRef(ID3D10Query *iface)
{
    struct d3d_query *query = impl_from_ID3D10Query(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_query_AddRef(&query->ID3D11Query_iface);
}

static ULONG STDMETHODCALLTYPE d3d10_query_Release(ID3D10Query *iface)
{
    struct d3d_query *query = impl_from_ID3D10Query(iface);

    TRACE("iface %p.\n", iface);

    return d3d11_query_Release(&query->ID3D11Query_iface);
}

/* ID3D10DeviceChild methods */

static void STDMETHODCALLTYPE d3d10_query_GetDevice(ID3D10Query *iface, ID3D10Device **device)
{
    struct d3d_query *query = impl_from_ID3D10Query(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    ID3D11Device2_QueryInterface(query->device, &IID_ID3D10Device, (void **)device);
}

static HRESULT STDMETHODCALLTYPE d3d10_query_GetPrivateData(ID3D10Query *iface,
        REFGUID guid, UINT *data_size, void *data)
{
    struct d3d_query *query = impl_from_ID3D10Query(iface);

    TRACE("iface %p, guid %s, data_size %p, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_get_private_data(&query->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_query_SetPrivateData(ID3D10Query *iface,
        REFGUID guid, UINT data_size, const void *data)
{
    struct d3d_query *query = impl_from_ID3D10Query(iface);

    TRACE("iface %p, guid %s, data_size %u, data %p.\n",
            iface, debugstr_guid(guid), data_size, data);

    return d3d_set_private_data(&query->private_store, guid, data_size, data);
}

static HRESULT STDMETHODCALLTYPE d3d10_query_SetPrivateDataInterface(ID3D10Query *iface,
        REFGUID guid, const IUnknown *data)
{
    struct d3d_query *query = impl_from_ID3D10Query(iface);

    TRACE("iface %p, guid %s, data %p.\n", iface, debugstr_guid(guid), data);

    return d3d_set_private_data_interface(&query->private_store, guid, data);
}

/* ID3D10Asynchronous methods */

static void STDMETHODCALLTYPE d3d10_query_Begin(ID3D10Query *iface)
{
    struct d3d_query *query = impl_from_ID3D10Query(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    if (FAILED(hr = wined3d_query_issue(query->wined3d_query, WINED3DISSUE_BEGIN)))
        ERR("Failed to issue query, hr %#x.\n", hr);
    wined3d_mutex_unlock();
}

static void STDMETHODCALLTYPE d3d10_query_End(ID3D10Query *iface)
{
    struct d3d_query *query = impl_from_ID3D10Query(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    if (FAILED(hr = wined3d_query_issue(query->wined3d_query, WINED3DISSUE_END)))
        ERR("Failed to issue query, hr %#x.\n", hr);
    wined3d_mutex_unlock();
}

static HRESULT STDMETHODCALLTYPE d3d10_query_GetData(ID3D10Query *iface, void *data, UINT data_size, UINT flags)
{
    struct d3d_query *query = impl_from_ID3D10Query(iface);
    D3D11_QUERY_DATA_PIPELINE_STATISTICS d3d11_data;
    void *d3d10_data_pointer = NULL;
    unsigned int wined3d_flags;
    HRESULT hr;

    TRACE("iface %p, data %p, data_size %u, flags %#x.\n", iface, data, data_size, flags);

    if (!data && data_size)
        return E_INVALIDARG;

    if (query->desc.Query == D3D11_QUERY_PIPELINE_STATISTICS
            && data_size == sizeof(D3D10_QUERY_DATA_PIPELINE_STATISTICS))
    {
        data_size = sizeof(D3D11_QUERY_DATA_PIPELINE_STATISTICS);
        d3d10_data_pointer = data;
        data = &d3d11_data;
    }

    wined3d_flags = wined3d_getdata_flags_from_d3d11_async_getdata_flags(flags);

    wined3d_mutex_lock();
    if (!data_size || wined3d_query_get_data_size(query->wined3d_query) == data_size)
    {
        hr = wined3d_query_get_data(query->wined3d_query, data, data_size, wined3d_flags);
        if (hr == WINED3DERR_INVALIDCALL)
            hr = DXGI_ERROR_INVALID_CALL;
    }
    else
    {
        WARN("Invalid data size %u.\n", data_size);
        hr = E_INVALIDARG;
    }
    wined3d_mutex_unlock();

    if (d3d10_data_pointer && hr == S_OK)
        memcpy(d3d10_data_pointer, data, sizeof(D3D10_QUERY_DATA_PIPELINE_STATISTICS));

    return hr;
}

static UINT STDMETHODCALLTYPE d3d10_query_GetDataSize(ID3D10Query *iface)
{
    struct d3d_query *query = impl_from_ID3D10Query(iface);
    unsigned int data_size;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    data_size = wined3d_query_get_data_size(query->wined3d_query);
    wined3d_mutex_unlock();

    if (query->desc.Query == D3D11_QUERY_PIPELINE_STATISTICS)
        data_size = sizeof(D3D10_QUERY_DATA_PIPELINE_STATISTICS);

    return data_size;
}

/* ID3D10Query methods */

static void STDMETHODCALLTYPE d3d10_query_GetDesc(ID3D10Query *iface, D3D10_QUERY_DESC *desc)
{
    struct d3d_query *query = impl_from_ID3D10Query(iface);

    TRACE("iface %p, desc %p.\n", iface, desc);

    memcpy(desc, &query->desc, sizeof(*desc));
}

static const struct ID3D10QueryVtbl d3d10_query_vtbl =
{
    /* IUnknown methods */
    d3d10_query_QueryInterface,
    d3d10_query_AddRef,
    d3d10_query_Release,
    /* ID3D10DeviceChild methods */
    d3d10_query_GetDevice,
    d3d10_query_GetPrivateData,
    d3d10_query_SetPrivateData,
    d3d10_query_SetPrivateDataInterface,
    /* ID3D10Asynchronous methods */
    d3d10_query_Begin,
    d3d10_query_End,
    d3d10_query_GetData,
    d3d10_query_GetDataSize,
    /* ID3D10Query methods */
    d3d10_query_GetDesc,
};

struct d3d_query *unsafe_impl_from_ID3D10Query(ID3D10Query *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &d3d10_query_vtbl);
    return CONTAINING_RECORD(iface, struct d3d_query, ID3D10Query_iface);
}

static HRESULT d3d_query_init(struct d3d_query *query, struct d3d_device *device,
        const D3D11_QUERY_DESC *desc, BOOL predicate)
{
    HRESULT hr;

    static const enum wined3d_query_type query_type_map[] =
    {
        /* D3D11_QUERY_EVENT                         */ WINED3D_QUERY_TYPE_EVENT,
        /* D3D11_QUERY_OCCLUSION                     */ WINED3D_QUERY_TYPE_OCCLUSION,
        /* D3D11_QUERY_TIMESTAMP                     */ WINED3D_QUERY_TYPE_TIMESTAMP,
        /* D3D11_QUERY_TIMESTAMP_DISJOINT            */ WINED3D_QUERY_TYPE_TIMESTAMP_DISJOINT,
        /* D3D11_QUERY_PIPELINE_STATISTICS           */ WINED3D_QUERY_TYPE_PIPELINE_STATISTICS,
        /* D3D11_QUERY_OCCLUSION_PREDICATE           */ WINED3D_QUERY_TYPE_OCCLUSION,
        /* D3D11_QUERY_SO_STATISTICS                 */ WINED3D_QUERY_TYPE_SO_STATISTICS,
        /* D3D11_QUERY_SO_OVERFLOW_PREDICATE         */ WINED3D_QUERY_TYPE_SO_OVERFLOW,
        /* D3D11_QUERY_SO_STATISTICS_STREAM0         */ WINED3D_QUERY_TYPE_SO_STATISTICS_STREAM0,
        /* D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM0 */ WINED3D_QUERY_TYPE_SO_OVERFLOW_STREAM0,
        /* D3D11_QUERY_SO_STATISTICS_STREAM1         */ WINED3D_QUERY_TYPE_SO_STATISTICS_STREAM1,
        /* D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM1 */ WINED3D_QUERY_TYPE_SO_OVERFLOW_STREAM1,
        /* D3D11_QUERY_SO_STATISTICS_STREAM2         */ WINED3D_QUERY_TYPE_SO_STATISTICS_STREAM2,
        /* D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM2 */ WINED3D_QUERY_TYPE_SO_OVERFLOW_STREAM2,
        /* D3D11_QUERY_SO_STATISTICS_STREAM3         */ WINED3D_QUERY_TYPE_SO_STATISTICS_STREAM3,
        /* D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM3 */ WINED3D_QUERY_TYPE_SO_OVERFLOW_STREAM3,
    };

    if (desc->Query >= ARRAY_SIZE(query_type_map))
    {
        FIXME("Unhandled query type %#x.\n", desc->Query);
        return E_INVALIDARG;
    }

    if (desc->MiscFlags)
        FIXME("Ignoring MiscFlags %#x.\n", desc->MiscFlags);

    query->ID3D11Query_iface.lpVtbl = &d3d11_query_vtbl;
    query->ID3D10Query_iface.lpVtbl = &d3d10_query_vtbl;
    query->refcount = 1;

    query->desc = *desc;

    wined3d_mutex_lock();
    wined3d_private_store_init(&query->private_store);

    if (FAILED(hr = wined3d_query_create(device->wined3d_device, query_type_map[desc->Query],
            query, &d3d_query_wined3d_parent_ops, &query->wined3d_query)))
    {
        WARN("Failed to create wined3d query, hr %#x.\n", hr);
        wined3d_private_store_cleanup(&query->private_store);
        wined3d_mutex_unlock();
        return hr;
    }
    wined3d_mutex_unlock();

    query->predicate = predicate;
    ID3D11Device2_AddRef(query->device = &device->ID3D11Device2_iface);

    return S_OK;
}

HRESULT d3d_query_create(struct d3d_device *device, const D3D11_QUERY_DESC *desc, BOOL predicate,
        struct d3d_query **query)
{
    struct d3d_query *object;
    BOOL is_predicate_type;
    HRESULT hr;

    if (!desc)
        return E_INVALIDARG;

    is_predicate_type = desc->Query == D3D11_QUERY_OCCLUSION_PREDICATE
            || desc->Query == D3D11_QUERY_SO_OVERFLOW_PREDICATE
            || desc->Query == D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM0
            || desc->Query == D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM1
            || desc->Query == D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM2
            || desc->Query == D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM3;

    if (!is_predicate_type && predicate)
    {
        WARN("Query type %u is not a predicate.\n", desc->Query);
        return E_INVALIDARG;
    }

    if (is_predicate_type)
        predicate = TRUE;

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = d3d_query_init(object, device, desc, predicate)))
    {
        WARN("Failed to initialize predicate, hr %#x.\n", hr);
        heap_free(object);
        return hr;
    }

    TRACE("Created query %p.\n", object);
    *query = object;

    return S_OK;
}
