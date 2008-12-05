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
    TRACE("(%p) Relay\n", This);

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

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IDirect3DQuery9Impl_Release(LPDIRECT3DQUERY9 iface) {
    IDirect3DQuery9Impl *This = (IDirect3DQuery9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

    if (ref == 0) {
        EnterCriticalSection(&d3d9_cs);
        IWineD3DQuery_Release(This->wineD3DQuery);
        LeaveCriticalSection(&d3d9_cs);
        IDirect3DDevice9Ex_Release(This->parentDevice);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DQuery9 Interface follow: */
static HRESULT WINAPI IDirect3DQuery9Impl_GetDevice(LPDIRECT3DQUERY9 iface, IDirect3DDevice9** ppDevice) {
    IDirect3DQuery9Impl *This = (IDirect3DQuery9Impl *)iface;
    IWineD3DDevice* pDevice;
    HRESULT hr;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DQuery_GetDevice(This->wineD3DQuery, &pDevice);
    if(hr != D3D_OK){
        *ppDevice = NULL;
    }else{
        hr = IWineD3DDevice_GetParent(pDevice, (IUnknown **)ppDevice);
        IWineD3DDevice_Release(pDevice);
    }
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static D3DQUERYTYPE WINAPI IDirect3DQuery9Impl_GetType(LPDIRECT3DQUERY9 iface) {
    IDirect3DQuery9Impl *This = (IDirect3DQuery9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DQuery_GetType(This->wineD3DQuery);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static DWORD WINAPI IDirect3DQuery9Impl_GetDataSize(LPDIRECT3DQUERY9 iface) {
    IDirect3DQuery9Impl *This = (IDirect3DQuery9Impl *)iface;
    DWORD ret;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    ret = IWineD3DQuery_GetDataSize(This->wineD3DQuery);
    LeaveCriticalSection(&d3d9_cs);
    return ret;
}

static HRESULT WINAPI IDirect3DQuery9Impl_Issue(LPDIRECT3DQUERY9 iface, DWORD dwIssueFlags) {
    IDirect3DQuery9Impl *This = (IDirect3DQuery9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DQuery_Issue(This->wineD3DQuery, dwIssueFlags);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DQuery9Impl_GetData(LPDIRECT3DQUERY9 iface, void* pData, DWORD dwSize, DWORD dwGetDataFlags) {
    IDirect3DQuery9Impl *This = (IDirect3DQuery9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DQuery_GetData(This->wineD3DQuery, pData, dwSize, dwGetDataFlags);
    LeaveCriticalSection(&d3d9_cs);
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

    TRACE("(%p) Relay\n", This);

    if (!ppQuery)
    {
        EnterCriticalSection(&d3d9_cs);
        hr = IWineD3DDevice_CreateQuery(This->WineD3DDevice, Type, NULL, NULL);
        LeaveCriticalSection(&d3d9_cs);
        return hr;
    }

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DQuery9Impl));
    if (NULL == object) {
        FIXME("Allocation of memory failed, returning D3DERR_OUTOFVIDEOMEMORY\n");
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DQuery9_Vtbl;
    object->ref = 1;
    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DDevice_CreateQuery(This->WineD3DDevice, Type, &object->wineD3DQuery, (IUnknown *)object);
    LeaveCriticalSection(&d3d9_cs);

    if (FAILED(hr)) {

        /* free up object */
        FIXME("(%p) call to IWineD3DDevice_CreateQuery failed\n", This);
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
