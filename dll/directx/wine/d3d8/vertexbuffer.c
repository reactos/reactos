/*
 * IDirect3DVertexBuffer8 implementation
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

/* IDirect3DVertexBuffer8 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DVertexBuffer8Impl_QueryInterface(LPDIRECT3DVERTEXBUFFER8 iface, REFIID riid, LPVOID *ppobj) {
    IDirect3DVertexBuffer8Impl *This = (IDirect3DVertexBuffer8Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource8)
        || IsEqualGUID(riid, &IID_IDirect3DVertexBuffer8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);

    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DVertexBuffer8Impl_AddRef(LPDIRECT3DVERTEXBUFFER8 iface) {
    IDirect3DVertexBuffer8Impl *This = (IDirect3DVertexBuffer8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);

    return ref;
}

static ULONG WINAPI IDirect3DVertexBuffer8Impl_Release(LPDIRECT3DVERTEXBUFFER8 iface) {
    IDirect3DVertexBuffer8Impl *This = (IDirect3DVertexBuffer8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

    if (ref == 0) {
        EnterCriticalSection(&d3d8_cs);
        IWineD3DVertexBuffer_Release(This->wineD3DVertexBuffer);
        LeaveCriticalSection(&d3d8_cs);
        IUnknown_Release(This->parentDevice);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/* IDirect3DVertexBuffer8 IDirect3DResource8 Interface follow: */
static HRESULT WINAPI IDirect3DVertexBuffer8Impl_GetDevice(LPDIRECT3DVERTEXBUFFER8 iface, IDirect3DDevice8 **ppDevice) {
    IDirect3DVertexBuffer8Impl *This = (IDirect3DVertexBuffer8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IDirect3DResource8Impl_GetDevice((LPDIRECT3DRESOURCE8) This, ppDevice);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DVertexBuffer8Impl_SetPrivateData(LPDIRECT3DVERTEXBUFFER8 iface, REFGUID refguid, CONST void *pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DVertexBuffer8Impl *This = (IDirect3DVertexBuffer8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DVertexBuffer_SetPrivateData(This->wineD3DVertexBuffer, refguid, pData, SizeOfData, Flags);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DVertexBuffer8Impl_GetPrivateData(LPDIRECT3DVERTEXBUFFER8 iface, REFGUID refguid, void *pData, DWORD *pSizeOfData) {
    IDirect3DVertexBuffer8Impl *This = (IDirect3DVertexBuffer8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DVertexBuffer_GetPrivateData(This->wineD3DVertexBuffer, refguid, pData, pSizeOfData);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DVertexBuffer8Impl_FreePrivateData(LPDIRECT3DVERTEXBUFFER8 iface, REFGUID refguid) {
    IDirect3DVertexBuffer8Impl *This = (IDirect3DVertexBuffer8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DVertexBuffer_FreePrivateData(This->wineD3DVertexBuffer, refguid);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static DWORD WINAPI IDirect3DVertexBuffer8Impl_SetPriority(LPDIRECT3DVERTEXBUFFER8 iface, DWORD PriorityNew) {
    IDirect3DVertexBuffer8Impl *This = (IDirect3DVertexBuffer8Impl *)iface;
    DWORD ret;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    ret = IWineD3DVertexBuffer_SetPriority(This->wineD3DVertexBuffer, PriorityNew);
    LeaveCriticalSection(&d3d8_cs);
    return ret;
}

static DWORD WINAPI IDirect3DVertexBuffer8Impl_GetPriority(LPDIRECT3DVERTEXBUFFER8 iface) {
    IDirect3DVertexBuffer8Impl *This = (IDirect3DVertexBuffer8Impl *)iface;
    DWORD ret;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    ret = IWineD3DVertexBuffer_GetPriority(This->wineD3DVertexBuffer);
    LeaveCriticalSection(&d3d8_cs);
    return ret;
}

static void WINAPI IDirect3DVertexBuffer8Impl_PreLoad(LPDIRECT3DVERTEXBUFFER8 iface) {
    IDirect3DVertexBuffer8Impl *This = (IDirect3DVertexBuffer8Impl *)iface;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    IWineD3DVertexBuffer_PreLoad(This->wineD3DVertexBuffer);
    LeaveCriticalSection(&d3d8_cs);
}

static D3DRESOURCETYPE WINAPI IDirect3DVertexBuffer8Impl_GetType(LPDIRECT3DVERTEXBUFFER8 iface) {
    IDirect3DVertexBuffer8Impl *This = (IDirect3DVertexBuffer8Impl *)iface;
    D3DRESOURCETYPE type;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    type = IWineD3DVertexBuffer_GetType(This->wineD3DVertexBuffer);
    LeaveCriticalSection(&d3d8_cs);
    return type;
}

/* IDirect3DVertexBuffer8 Interface follow: */
static HRESULT WINAPI IDirect3DVertexBuffer8Impl_Lock(LPDIRECT3DVERTEXBUFFER8 iface, UINT OffsetToLock, UINT SizeToLock, BYTE **ppbData, DWORD Flags) {
    IDirect3DVertexBuffer8Impl *This = (IDirect3DVertexBuffer8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DVertexBuffer_Lock(This->wineD3DVertexBuffer, OffsetToLock, SizeToLock, ppbData, Flags);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DVertexBuffer8Impl_Unlock(LPDIRECT3DVERTEXBUFFER8 iface) {
    IDirect3DVertexBuffer8Impl *This = (IDirect3DVertexBuffer8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DVertexBuffer_Unlock(This->wineD3DVertexBuffer);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DVertexBuffer8Impl_GetDesc(LPDIRECT3DVERTEXBUFFER8 iface, D3DVERTEXBUFFER_DESC *pDesc) {
    IDirect3DVertexBuffer8Impl *This = (IDirect3DVertexBuffer8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DVertexBuffer_GetDesc(This->wineD3DVertexBuffer, (WINED3DVERTEXBUFFER_DESC *) pDesc);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

const IDirect3DVertexBuffer8Vtbl Direct3DVertexBuffer8_Vtbl =
{
    /* IUnknown */
    IDirect3DVertexBuffer8Impl_QueryInterface,
    IDirect3DVertexBuffer8Impl_AddRef,
    IDirect3DVertexBuffer8Impl_Release,
    /* IDirect3DResource8 */
    IDirect3DVertexBuffer8Impl_GetDevice,
    IDirect3DVertexBuffer8Impl_SetPrivateData,
    IDirect3DVertexBuffer8Impl_GetPrivateData,
    IDirect3DVertexBuffer8Impl_FreePrivateData,
    IDirect3DVertexBuffer8Impl_SetPriority,
    IDirect3DVertexBuffer8Impl_GetPriority,
    IDirect3DVertexBuffer8Impl_PreLoad,
    IDirect3DVertexBuffer8Impl_GetType,
    /* IDirect3DVertexBuffer8 */
    IDirect3DVertexBuffer8Impl_Lock,
    IDirect3DVertexBuffer8Impl_Unlock,
    IDirect3DVertexBuffer8Impl_GetDesc
};
