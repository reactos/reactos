/*
 * IDirect3DResource9 implementation
 *
 * Copyright 2002-2004 Jason Edmeades
 *                     Raphael Junqueira
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

/* IDirect3DResource9 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DResource9Impl_QueryInterface(LPDIRECT3DRESOURCE9 iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DResource9Impl *This = (IDirect3DResource9Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource9)) {
        IDirect3DResource9_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DResource9Impl_AddRef(LPDIRECT3DRESOURCE9 iface) {
    IDirect3DResource9Impl *This = (IDirect3DResource9Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);

    return ref;
}

static ULONG WINAPI IDirect3DResource9Impl_Release(LPDIRECT3DRESOURCE9 iface) {
    IDirect3DResource9Impl *This = (IDirect3DResource9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

    if (ref == 0) {
        IWineD3DResource_Release(This->wineD3DResource);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DResource9 Interface follow: */
HRESULT WINAPI IDirect3DResource9Impl_GetDevice(LPDIRECT3DRESOURCE9 iface, IDirect3DDevice9** ppDevice) {
    IDirect3DResource9Impl *This = (IDirect3DResource9Impl *)iface;
    IWineD3DDevice *myDevice = NULL;

    TRACE("(%p) Relay\n", This);

    IWineD3DResource_GetDevice(This->wineD3DResource, &myDevice);
    IWineD3DDevice_GetParent(myDevice, (IUnknown **)ppDevice);
    IWineD3DDevice_Release(myDevice);
    return D3D_OK;
}

static HRESULT WINAPI IDirect3DResource9Impl_SetPrivateData(LPDIRECT3DRESOURCE9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DResource9Impl *This = (IDirect3DResource9Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DResource_SetPrivateData(This->wineD3DResource, refguid, pData, SizeOfData, Flags);
}

static HRESULT WINAPI IDirect3DResource9Impl_GetPrivateData(LPDIRECT3DRESOURCE9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    IDirect3DResource9Impl *This = (IDirect3DResource9Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DResource_GetPrivateData(This->wineD3DResource, refguid, pData, pSizeOfData);
}

static HRESULT WINAPI IDirect3DResource9Impl_FreePrivateData(LPDIRECT3DRESOURCE9 iface, REFGUID refguid) {
    IDirect3DResource9Impl *This = (IDirect3DResource9Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DResource_FreePrivateData(This->wineD3DResource, refguid);
}

static DWORD  WINAPI IDirect3DResource9Impl_SetPriority(LPDIRECT3DRESOURCE9 iface, DWORD PriorityNew) {
    IDirect3DResource9Impl *This = (IDirect3DResource9Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DResource_SetPriority(This->wineD3DResource, PriorityNew);
}

static DWORD WINAPI IDirect3DResource9Impl_GetPriority(LPDIRECT3DRESOURCE9 iface) {
    IDirect3DResource9Impl *This = (IDirect3DResource9Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DResource_GetPriority(This->wineD3DResource);
}

static void WINAPI IDirect3DResource9Impl_PreLoad(LPDIRECT3DRESOURCE9 iface) {
    IDirect3DResource9Impl *This = (IDirect3DResource9Impl *)iface;
    TRACE("(%p) Relay\n", This);
    IWineD3DResource_PreLoad(This->wineD3DResource);
    return;
}

static D3DRESOURCETYPE WINAPI IDirect3DResource9Impl_GetType(LPDIRECT3DRESOURCE9 iface) {
    IDirect3DResource9Impl *This = (IDirect3DResource9Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DResource_GetType(This->wineD3DResource);
}


const IDirect3DResource9Vtbl Direct3DResource9_Vtbl =
{
    IDirect3DResource9Impl_QueryInterface,
    IDirect3DResource9Impl_AddRef,
    IDirect3DResource9Impl_Release,
    IDirect3DResource9Impl_GetDevice,
    IDirect3DResource9Impl_SetPrivateData,
    IDirect3DResource9Impl_GetPrivateData,
    IDirect3DResource9Impl_FreePrivateData,
    IDirect3DResource9Impl_SetPriority,
    IDirect3DResource9Impl_GetPriority,
    IDirect3DResource9Impl_PreLoad,
    IDirect3DResource9Impl_GetType
};
