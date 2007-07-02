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
        IWineD3DIndexBuffer_Release(This->wineD3DIndexBuffer);
        IUnknown_Release(This->parentDevice);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DIndexBuffer8 IDirect3DResource8 Interface follow: */
static HRESULT WINAPI IDirect3DIndexBuffer8Impl_GetDevice(LPDIRECT3DINDEXBUFFER8 iface, IDirect3DDevice8 **ppDevice) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IDirect3DResource8Impl_GetDevice((LPDIRECT3DRESOURCE8) This, ppDevice);
}

static HRESULT WINAPI IDirect3DIndexBuffer8Impl_SetPrivateData(LPDIRECT3DINDEXBUFFER8 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DIndexBuffer_SetPrivateData(This->wineD3DIndexBuffer, refguid, pData, SizeOfData, Flags);
}

static HRESULT WINAPI IDirect3DIndexBuffer8Impl_GetPrivateData(LPDIRECT3DINDEXBUFFER8 iface, REFGUID refguid, void *pData, DWORD *pSizeOfData) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DIndexBuffer_GetPrivateData(This->wineD3DIndexBuffer, refguid, pData, pSizeOfData);
}

static HRESULT WINAPI IDirect3DIndexBuffer8Impl_FreePrivateData(LPDIRECT3DINDEXBUFFER8 iface, REFGUID refguid) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DIndexBuffer_FreePrivateData(This->wineD3DIndexBuffer, refguid);
}

static DWORD WINAPI IDirect3DIndexBuffer8Impl_SetPriority(LPDIRECT3DINDEXBUFFER8 iface, DWORD PriorityNew) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DIndexBuffer_SetPriority(This->wineD3DIndexBuffer, PriorityNew);
}

static DWORD WINAPI IDirect3DIndexBuffer8Impl_GetPriority(LPDIRECT3DINDEXBUFFER8 iface) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DIndexBuffer_GetPriority(This->wineD3DIndexBuffer);
}

static void WINAPI IDirect3DIndexBuffer8Impl_PreLoad(LPDIRECT3DINDEXBUFFER8 iface) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    IWineD3DIndexBuffer_PreLoad(This->wineD3DIndexBuffer);
}

static D3DRESOURCETYPE WINAPI IDirect3DIndexBuffer8Impl_GetType(LPDIRECT3DINDEXBUFFER8 iface) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DIndexBuffer_GetType(This->wineD3DIndexBuffer);
}

/* IDirect3DIndexBuffer8 Interface follow: */
static HRESULT WINAPI IDirect3DIndexBuffer8Impl_Lock(LPDIRECT3DINDEXBUFFER8 iface, UINT OffsetToLock, UINT SizeToLock, BYTE **ppbData, DWORD Flags) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DIndexBuffer_Lock(This->wineD3DIndexBuffer, OffsetToLock, SizeToLock, ppbData, Flags);
}

static HRESULT WINAPI IDirect3DIndexBuffer8Impl_Unlock(LPDIRECT3DINDEXBUFFER8 iface) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DIndexBuffer_Unlock(This->wineD3DIndexBuffer);
}

static HRESULT WINAPI IDirect3DIndexBuffer8Impl_GetDesc(LPDIRECT3DINDEXBUFFER8 iface, D3DINDEXBUFFER_DESC *pDesc) {
    IDirect3DIndexBuffer8Impl *This = (IDirect3DIndexBuffer8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DIndexBuffer_GetDesc(This->wineD3DIndexBuffer, (WINED3DINDEXBUFFER_DESC *) pDesc);
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
