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

/* IDirect3DQuery9 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DQuery9Impl_QueryInterface(LPDIRECT3DQUERY9 iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DQuery9Impl *This = (IDirect3DQuery9Impl *)iface;

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

static ULONG WINAPI IDirect3DQuery9Impl_AddRef(LPDIRECT3DQUERY9 iface) {
    IDirect3DQuery9Impl *This = (IDirect3DQuery9Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    return ref;
}

static ULONG WINAPI IDirect3DQuery9Impl_Release(LPDIRECT3DQUERY9 iface) {
    IDirect3DQuery9Impl *This = (IDirect3DQuery9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (ref == 0) {
        wined3d_mutex_lock();
        IWineD3DQuery_Release(This->wineD3DQuery);
        wined3d_mutex_unlock();

        IDirect3DDevice9Ex_Release(This->parentDevice);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DQuery9 Interface follow: */
static HRESULT WINAPI IDirect3DQuery9Impl_GetDevice(IDirect3DQuery9 *iface, IDirect3DDevice9 **device)
{
    IDirect3DQuery9Impl *This = (IDirect3DQuery9Impl *)iface;

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)This->parentDevice;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static D3DQUERYTYPE WINAPI IDirect3DQuery9Impl_GetType(LPDIRECT3DQUERY9 iface) {
    IDirect3DQuery9Impl *This = (IDirect3DQuery9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = IWineD3DQuery_GetType(This->wineD3DQuery);
    wined3d_mutex_unlock();

    return hr;
}

static DWORD WINAPI IDirect3DQuery9Impl_GetDataSize(LPDIRECT3DQUERY9 iface) {
    IDirect3DQuery9Impl *This = (IDirect3DQuery9Impl *)iface;
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = IWineD3DQuery_GetDataSize(This->wineD3DQuery);
    wined3d_mutex_unlock();

    return ret;
}

static HRESULT WINAPI IDirect3DQuery9Impl_Issue(LPDIRECT3DQUERY9 iface, DWORD dwIssueFlags) {
    IDirect3DQuery9Impl *This = (IDirect3DQuery9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, flags %#x.\n", iface, dwIssueFlags);

    wined3d_mutex_lock();
    hr = IWineD3DQuery_Issue(This->wineD3DQuery, dwIssueFlags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DQuery9Impl_GetData(LPDIRECT3DQUERY9 iface, void* pData, DWORD dwSize, DWORD dwGetDataFlags) {
    IDirect3DQuery9Impl *This = (IDirect3DQuery9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, data %p, size %u, flags %#x.\n",
            iface, pData, dwSize, dwGetDataFlags);

    wined3d_mutex_lock();
    hr = IWineD3DQuery_GetData(This->wineD3DQuery, pData, dwSize, dwGetDataFlags);
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


/* IDirect3DDevice9 IDirect3DQuery9 Methods follow: */
HRESULT WINAPI IDirect3DDevice9Impl_CreateQuery(LPDIRECT3DDEVICE9EX iface, D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DQuery9Impl *object = NULL;
    HRESULT hr = D3D_OK;

    TRACE("iface %p, type %#x, query %p.\n", iface, Type, ppQuery);

    if (!ppQuery)
    {
        wined3d_mutex_lock();
        hr = IWineD3DDevice_CreateQuery(This->WineD3DDevice, Type, NULL, NULL);
        wined3d_mutex_unlock();

        return hr;
    }

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DQuery9Impl));
    if (NULL == object) {
        ERR("Allocation of memory failed, returning D3DERR_OUTOFVIDEOMEMORY\n");
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DQuery9_Vtbl;
    object->ref = 1;

    wined3d_mutex_lock();
    hr = IWineD3DDevice_CreateQuery(This->WineD3DDevice, Type, &object->wineD3DQuery, (IUnknown *)object);
    wined3d_mutex_unlock();

    if (FAILED(hr)) {

        /* free up object */
        WARN("(%p) call to IWineD3DDevice_CreateQuery failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
    } else {
        IDirect3DDevice9Ex_AddRef(iface);
        object->parentDevice = iface;
        *ppQuery = (LPDIRECT3DQUERY9) object;
        TRACE("(%p) : Created query %p\n", This , object);
    }
    TRACE("(%p) : returning %x\n", This, hr);
    return hr;
}
