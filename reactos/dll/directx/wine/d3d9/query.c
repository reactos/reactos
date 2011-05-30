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

#include "config.h"
#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

static inline IDirect3DQuery9Impl *impl_from_IDirect3DQuery9(IDirect3DQuery9 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DQuery9Impl, IDirect3DQuery9_iface);
}

static HRESULT WINAPI IDirect3DQuery9Impl_QueryInterface(IDirect3DQuery9 *iface, REFIID riid,
        void **ppobj)
{
    IDirect3DQuery9Impl *This = impl_from_IDirect3DQuery9(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DQuery9)) {
        IDirect3DQuery9_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DQuery9Impl_AddRef(IDirect3DQuery9 *iface)
{
    IDirect3DQuery9Impl *This = impl_from_IDirect3DQuery9(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    return ref;
}

static ULONG WINAPI IDirect3DQuery9Impl_Release(IDirect3DQuery9 *iface)
{
    IDirect3DQuery9Impl *This = impl_from_IDirect3DQuery9(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (ref == 0) {
        wined3d_mutex_lock();
        wined3d_query_decref(This->wineD3DQuery);
        wined3d_mutex_unlock();

        IDirect3DDevice9Ex_Release(This->parentDevice);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static HRESULT WINAPI IDirect3DQuery9Impl_GetDevice(IDirect3DQuery9 *iface,
        IDirect3DDevice9 **device)
{
    IDirect3DQuery9Impl *This = impl_from_IDirect3DQuery9(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)This->parentDevice;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static D3DQUERYTYPE WINAPI IDirect3DQuery9Impl_GetType(IDirect3DQuery9 *iface)
{
    IDirect3DQuery9Impl *This = impl_from_IDirect3DQuery9(iface);
    D3DQUERYTYPE type;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    type = wined3d_query_get_type(This->wineD3DQuery);
    wined3d_mutex_unlock();

    return type;
}

static DWORD WINAPI IDirect3DQuery9Impl_GetDataSize(IDirect3DQuery9 *iface)
{
    IDirect3DQuery9Impl *This = impl_from_IDirect3DQuery9(iface);
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = wined3d_query_get_data_size(This->wineD3DQuery);
    wined3d_mutex_unlock();

    return ret;
}

static HRESULT WINAPI IDirect3DQuery9Impl_Issue(IDirect3DQuery9 *iface, DWORD dwIssueFlags)
{
    IDirect3DQuery9Impl *This = impl_from_IDirect3DQuery9(iface);
    HRESULT hr;

    TRACE("iface %p, flags %#x.\n", iface, dwIssueFlags);

    wined3d_mutex_lock();
    hr = wined3d_query_issue(This->wineD3DQuery, dwIssueFlags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DQuery9Impl_GetData(IDirect3DQuery9 *iface, void *pData,
        DWORD dwSize, DWORD dwGetDataFlags)
{
    IDirect3DQuery9Impl *This = impl_from_IDirect3DQuery9(iface);
    HRESULT hr;

    TRACE("iface %p, data %p, size %u, flags %#x.\n",
            iface, pData, dwSize, dwGetDataFlags);

    wined3d_mutex_lock();
    hr = wined3d_query_get_data(This->wineD3DQuery, pData, dwSize, dwGetDataFlags);
    wined3d_mutex_unlock();

    return hr;
}


static const IDirect3DQuery9Vtbl Direct3DQuery9_Vtbl =
{
    IDirect3DQuery9Impl_QueryInterface,
    IDirect3DQuery9Impl_AddRef,
    IDirect3DQuery9Impl_Release,
    IDirect3DQuery9Impl_GetDevice,
    IDirect3DQuery9Impl_GetType,
    IDirect3DQuery9Impl_GetDataSize,
    IDirect3DQuery9Impl_Issue,
    IDirect3DQuery9Impl_GetData
};

HRESULT query_init(IDirect3DQuery9Impl *query, IDirect3DDevice9Impl *device, D3DQUERYTYPE type)
{
    HRESULT hr;

    query->IDirect3DQuery9_iface.lpVtbl = &Direct3DQuery9_Vtbl;
    query->ref = 1;

    wined3d_mutex_lock();
    hr = wined3d_query_create(device->wined3d_device, type, &query->wineD3DQuery);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d query, hr %#x.\n", hr);
        return hr;
    }

    query->parentDevice = &device->IDirect3DDevice9Ex_iface;
    IDirect3DDevice9Ex_AddRef(query->parentDevice);

    return D3D_OK;
}
