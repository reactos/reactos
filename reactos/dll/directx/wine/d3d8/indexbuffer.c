/*
 * IDirect3DIndexBuffer8 implementation
 *
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
#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

/* IDirect3DIndexBuffer8 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DIndexBuffer8Impl_QueryInterface(LPDIRECT3DINDEXBUFFER8 iface, REFIID riid, LPVOID *ppobj) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource8)
        || IsEqualGUID(riid, &IID_IDirect3DIndexBuffer8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DIndexBuffer8Impl_AddRef(LPDIRECT3DINDEXBUFFER8 iface) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);

    return ref;
}

static ULONG WINAPI IDirect3DIndexBuffer8Impl_Release(LPDIRECT3DINDEXBUFFER8 iface) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

    if (ref == 0) {
        EnterCriticalSection(&d3d8_cs);
        IWineD3DIndexBuffer_Release(This->wineD3DIndexBuffer);
        LeaveCriticalSection(&d3d8_cs);
        IUnknown_Release(This->parentDevice);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DIndexBuffer8 IDirect3DResource8 Interface follow: */
static HRESULT WINAPI IDirect3DIndexBuffer8Impl_GetDevice(LPDIRECT3DINDEXBUFFER8 iface, IDirect3DDevice8 **ppDevice) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;
    IWineD3DDevice *wined3d_device;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DIndexBuffer_GetDevice(This->wineD3DIndexBuffer, &wined3d_device);
    if (SUCCEEDED(hr))
    {
        IWineD3DDevice_GetParent(wined3d_device, (IUnknown **)ppDevice);
        IWineD3DDevice_Release(wined3d_device);
    }
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DIndexBuffer8Impl_SetPrivateData(LPDIRECT3DINDEXBUFFER8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DIndexBuffer_SetPrivateData(This->wineD3DIndexBuffer, refguid, pData, SizeOfData, Flags);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DIndexBuffer8Impl_GetPrivateData(LPDIRECT3DINDEXBUFFER8 iface, REFGUID refguid, void *pData, DWORD *pSizeOfData) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DIndexBuffer_GetPrivateData(This->wineD3DIndexBuffer, refguid, pData, pSizeOfData);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DIndexBuffer8Impl_FreePrivateData(LPDIRECT3DINDEXBUFFER8 iface, REFGUID refguid) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DIndexBuffer_FreePrivateData(This->wineD3DIndexBuffer, refguid);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static DWORD WINAPI IDirect3DIndexBuffer8Impl_SetPriority(LPDIRECT3DINDEXBUFFER8 iface, DWORD PriorityNew) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;
    DWORD ret;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    ret = IWineD3DIndexBuffer_SetPriority(This->wineD3DIndexBuffer, PriorityNew);
    LeaveCriticalSection(&d3d8_cs);
    return ret;
}

static DWORD WINAPI IDirect3DIndexBuffer8Impl_GetPriority(LPDIRECT3DINDEXBUFFER8 iface) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;
    DWORD ret;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    ret = IWineD3DIndexBuffer_GetPriority(This->wineD3DIndexBuffer);
    LeaveCriticalSection(&d3d8_cs);
    return ret;
}

static void WINAPI IDirect3DIndexBuffer8Impl_PreLoad(LPDIRECT3DINDEXBUFFER8 iface) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    IWineD3DIndexBuffer_PreLoad(This->wineD3DIndexBuffer);
    LeaveCriticalSection(&d3d8_cs);
}

static D3DRESOURCETYPE WINAPI IDirect3DIndexBuffer8Impl_GetType(LPDIRECT3DINDEXBUFFER8 iface) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;
    D3DRESOURCETYPE type;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    type = IWineD3DIndexBuffer_GetType(This->wineD3DIndexBuffer);
    LeaveCriticalSection(&d3d8_cs);
    return type;
}

/* IDirect3DIndexBuffer8 Interface follow: */
static HRESULT WINAPI IDirect3DIndexBuffer8Impl_Lock(LPDIRECT3DINDEXBUFFER8 iface, UINT OffsetToLock, UINT SizeToLock, BYTE **ppbData, DWORD Flags) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DIndexBuffer_Lock(This->wineD3DIndexBuffer, OffsetToLock, SizeToLock, ppbData, Flags);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DIndexBuffer8Impl_Unlock(LPDIRECT3DINDEXBUFFER8 iface) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DIndexBuffer_Unlock(This->wineD3DIndexBuffer);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DIndexBuffer8Impl_GetDesc(LPDIRECT3DINDEXBUFFER8 iface, D3DINDEXBUFFER_DESC *pDesc) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DIndexBuffer_GetDesc(This->wineD3DIndexBuffer, (WINED3DINDEXBUFFER_DESC *) pDesc);
    LeaveCriticalSection(&d3d8_cs);

    if (SUCCEEDED(hr)) pDesc->Format = d3dformat_from_wined3dformat(pDesc->Format);

    return hr;
}


const IDirect3DIndexBuffer8Vtbl Direct3DIndexBuffer8_Vtbl =
{
    /* IUnknown */
    IDirect3DIndexBuffer8Impl_QueryInterface,
    IDirect3DIndexBuffer8Impl_AddRef,
    IDirect3DIndexBuffer8Impl_Release,
    /* IDirect3DResource8 */
    IDirect3DIndexBuffer8Impl_GetDevice,
    IDirect3DIndexBuffer8Impl_SetPrivateData,
    IDirect3DIndexBuffer8Impl_GetPrivateData,
    IDirect3DIndexBuffer8Impl_FreePrivateData,
    IDirect3DIndexBuffer8Impl_SetPriority,
    IDirect3DIndexBuffer8Impl_GetPriority,
    IDirect3DIndexBuffer8Impl_PreLoad,
    IDirect3DIndexBuffer8Impl_GetType,
    /* IDirect3DIndexBuffer8 */
    IDirect3DIndexBuffer8Impl_Lock,
    IDirect3DIndexBuffer8Impl_Unlock,
    IDirect3DIndexBuffer8Impl_GetDesc
};
