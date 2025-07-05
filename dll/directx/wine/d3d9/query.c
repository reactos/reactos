/*
 * IDirect3DQuery9 implementation
 *
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2002-2003 Jason Edmeades
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

#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

static D3DQUERYTYPE d3dquerytype_from_wined3d(enum wined3d_query_type type)
{
    return (D3DQUERYTYPE)type;
}

static enum wined3d_query_type wined3d_query_type_from_d3d(D3DQUERYTYPE type)
{
    return (enum wined3d_query_type)type;
}

static inline struct d3d9_query *impl_from_IDirect3DQuery9(IDirect3DQuery9 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d9_query, IDirect3DQuery9_iface);
}

static HRESULT WINAPI d3d9_query_QueryInterface(IDirect3DQuery9 *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DQuery9)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DQuery9_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d9_query_AddRef(IDirect3DQuery9 *iface)
{
    struct d3d9_query *query = impl_from_IDirect3DQuery9(iface);
    ULONG refcount = InterlockedIncrement(&query->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI d3d9_query_Release(IDirect3DQuery9 *iface)
{
    struct d3d9_query *query = impl_from_IDirect3DQuery9(iface);
    ULONG refcount = InterlockedDecrement(&query->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        wined3d_query_decref(query->wined3d_query);
        IDirect3DDevice9Ex_Release(query->parent_device);
        free(query);
    }
    return refcount;
}

static HRESULT WINAPI d3d9_query_GetDevice(IDirect3DQuery9 *iface, IDirect3DDevice9 **device)
{
    struct d3d9_query *query = impl_from_IDirect3DQuery9(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)query->parent_device;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static D3DQUERYTYPE WINAPI d3d9_query_GetType(IDirect3DQuery9 *iface)
{
    struct d3d9_query *query = impl_from_IDirect3DQuery9(iface);
    D3DQUERYTYPE type;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    type = d3dquerytype_from_wined3d(wined3d_query_get_type(query->wined3d_query));
    wined3d_mutex_unlock();

    return type;
}

static DWORD WINAPI d3d9_query_GetDataSize(IDirect3DQuery9 *iface)
{
    struct d3d9_query *query = impl_from_IDirect3DQuery9(iface);

    TRACE("iface %p.\n", iface);

    return query->data_size;
}

static HRESULT WINAPI d3d9_query_Issue(IDirect3DQuery9 *iface, DWORD flags)
{
    struct d3d9_query *query = impl_from_IDirect3DQuery9(iface);
    TRACE("iface %p, flags %#lx.\n", iface, flags);
    return wined3d_query_issue(query->wined3d_query, flags);
}

static HRESULT WINAPI d3d9_query_GetData(IDirect3DQuery9 *iface, void *data, DWORD size, DWORD flags)
{
    struct d3d9_query *query = impl_from_IDirect3DQuery9(iface);
    enum wined3d_query_type type;
    HRESULT hr;

    TRACE("iface %p, data %p, size %lu, flags %#lx.\n", iface, data, size, flags);

    wined3d_mutex_lock();
    type = wined3d_query_get_type(query->wined3d_query);
    if (type == WINED3D_QUERY_TYPE_TIMESTAMP_DISJOINT && data)
    {
        struct wined3d_query_data_timestamp_disjoint data_disjoint;

        if (size > sizeof(data_disjoint.disjoint))
            size = sizeof(data_disjoint.disjoint);

        hr = wined3d_query_get_data(query->wined3d_query, &data_disjoint, sizeof(data_disjoint), flags);
        if (SUCCEEDED(hr))
            memcpy(data, &data_disjoint.disjoint, size);
    }
    else
    {
        hr = wined3d_query_get_data(query->wined3d_query, data, size, flags);
    }
    wined3d_mutex_unlock();

    if (hr == D3DERR_INVALIDCALL)
    {
        if (data)
        {
            memset(data, 0, size);
            memset(data, 0xdd, min(size, query->data_size));
        }
        return S_OK;
    }
    return hr;
}


static const struct IDirect3DQuery9Vtbl d3d9_query_vtbl =
{
    d3d9_query_QueryInterface,
    d3d9_query_AddRef,
    d3d9_query_Release,
    d3d9_query_GetDevice,
    d3d9_query_GetType,
    d3d9_query_GetDataSize,
    d3d9_query_Issue,
    d3d9_query_GetData,
};

HRESULT query_init(struct d3d9_query *query, struct d3d9_device *device, D3DQUERYTYPE type)
{
    HRESULT hr;

    if (type > D3DQUERYTYPE_MEMORYPRESSURE)
    {
        if (type == 0x16)
            FIXME("Undocumented query %#x created.\n", type);
        else
            WARN("Invalid query type %#x.\n", type);

        return D3DERR_NOTAVAILABLE;
    }

    query->IDirect3DQuery9_iface.lpVtbl = &d3d9_query_vtbl;
    query->refcount = 1;

    wined3d_mutex_lock();
    if (FAILED(hr = wined3d_query_create(device->wined3d_device, wined3d_query_type_from_d3d(type),
            query, &d3d9_null_wined3d_parent_ops, &query->wined3d_query)))
    {
        wined3d_mutex_unlock();
        WARN("Failed to create wined3d query, hr %#lx.\n", hr);
        return hr;
    }

    if (type == D3DQUERYTYPE_OCCLUSION)
        query->data_size = sizeof(DWORD);
    else if (type == D3DQUERYTYPE_TIMESTAMPDISJOINT)
        query->data_size = sizeof(BOOL);
    else
        query->data_size = wined3d_query_get_data_size(query->wined3d_query);
    wined3d_mutex_unlock();

    query->parent_device = &device->IDirect3DDevice9Ex_iface;
    IDirect3DDevice9Ex_AddRef(query->parent_device);

    return D3D_OK;
}
