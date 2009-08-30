/*
 * IDirect3DIndexBuffer9 implementation
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

/* IDirect3DIndexBuffer9 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DIndexBuffer9Impl_QueryInterface(LPDIRECT3DINDEXBUFFER9 iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DIndexBuffer9Impl *This = (IDirect3DIndexBuffer9Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource9)
        || IsEqualGUID(riid, &IID_IDirect3DIndexBuffer9)) {
        IDirect3DIndexBuffer9_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DIndexBuffer9Impl_AddRef(LPDIRECT3DINDEXBUFFER9 iface) {
    IDirect3DIndexBuffer9Impl *This = (IDirect3DIndexBuffer9Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);

    return ref;
}

static ULONG WINAPI IDirect3DIndexBuffer9Impl_Release(LPDIRECT3DINDEXBUFFER9 iface) {
    IDirect3DIndexBuffer9Impl *This = (IDirect3DIndexBuffer9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

    if (ref == 0) {
        EnterCriticalSection(&d3d9_cs);
        IWineD3DBuffer_Release(This->wineD3DIndexBuffer);
        LeaveCriticalSection(&d3d9_cs);
        IDirect3DDevice9Ex_Release(This->parentDevice);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DIndexBuffer9 IDirect3DResource9 Interface follow: */
static HRESULT WINAPI IDirect3DIndexBuffer9Impl_GetDevice(LPDIRECT3DINDEXBUFFER9 iface, IDirect3DDevice9** ppDevice) {
    IDirect3DIndexBuffer9Impl *This = (IDirect3DIndexBuffer9Impl *)iface;
    IWineD3DDevice *wined3d_device;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DBuffer_GetDevice(This->wineD3DIndexBuffer, &wined3d_device);
    if (SUCCEEDED(hr))
    {
        IWineD3DDevice_GetParent(wined3d_device, (IUnknown **)ppDevice);
        IWineD3DDevice_Release(wined3d_device);
    }
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DIndexBuffer9Impl_SetPrivateData(LPDIRECT3DINDEXBUFFER9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DIndexBuffer9Impl *This = (IDirect3DIndexBuffer9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DBuffer_SetPrivateData(This->wineD3DIndexBuffer, refguid, pData, SizeOfData, Flags);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DIndexBuffer9Impl_GetPrivateData(LPDIRECT3DINDEXBUFFER9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    IDirect3DIndexBuffer9Impl *This = (IDirect3DIndexBuffer9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DBuffer_GetPrivateData(This->wineD3DIndexBuffer, refguid, pData, pSizeOfData);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DIndexBuffer9Impl_FreePrivateData(LPDIRECT3DINDEXBUFFER9 iface, REFGUID refguid) {
    IDirect3DIndexBuffer9Impl *This = (IDirect3DIndexBuffer9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DBuffer_FreePrivateData(This->wineD3DIndexBuffer, refguid);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static DWORD WINAPI IDirect3DIndexBuffer9Impl_SetPriority(LPDIRECT3DINDEXBUFFER9 iface, DWORD PriorityNew) {
    IDirect3DIndexBuffer9Impl *This = (IDirect3DIndexBuffer9Impl *)iface;
    DWORD ret;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    ret = IWineD3DBuffer_SetPriority(This->wineD3DIndexBuffer, PriorityNew);
    LeaveCriticalSection(&d3d9_cs);
    return ret;
}

static DWORD WINAPI IDirect3DIndexBuffer9Impl_GetPriority(LPDIRECT3DINDEXBUFFER9 iface) {
    IDirect3DIndexBuffer9Impl *This = (IDirect3DIndexBuffer9Impl *)iface;
    DWORD ret;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    ret = IWineD3DBuffer_GetPriority(This->wineD3DIndexBuffer);
    LeaveCriticalSection(&d3d9_cs);
    return ret;
}

static void WINAPI IDirect3DIndexBuffer9Impl_PreLoad(LPDIRECT3DINDEXBUFFER9 iface) {
    IDirect3DIndexBuffer9Impl *This = (IDirect3DIndexBuffer9Impl *)iface;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    IWineD3DBuffer_PreLoad(This->wineD3DIndexBuffer);
    LeaveCriticalSection(&d3d9_cs);
}

static D3DRESOURCETYPE WINAPI IDirect3DIndexBuffer9Impl_GetType(LPDIRECT3DINDEXBUFFER9 iface) {
    IDirect3DIndexBuffer9Impl *This = (IDirect3DIndexBuffer9Impl *)iface;
    TRACE("(%p)\n", This);

    return D3DRTYPE_INDEXBUFFER;
}

/* IDirect3DIndexBuffer9 Interface follow: */
static HRESULT WINAPI IDirect3DIndexBuffer9Impl_Lock(LPDIRECT3DINDEXBUFFER9 iface, UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags) {
    IDirect3DIndexBuffer9Impl *This = (IDirect3DIndexBuffer9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DBuffer_Map(This->wineD3DIndexBuffer, OffsetToLock, SizeToLock, (BYTE **)ppbData, Flags);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DIndexBuffer9Impl_Unlock(LPDIRECT3DINDEXBUFFER9 iface) {
    IDirect3DIndexBuffer9Impl *This = (IDirect3DIndexBuffer9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DBuffer_Unmap(This->wineD3DIndexBuffer);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT  WINAPI        IDirect3DIndexBuffer9Impl_GetDesc(LPDIRECT3DINDEXBUFFER9 iface, D3DINDEXBUFFER_DESC *pDesc) {
    IDirect3DIndexBuffer9Impl *This = (IDirect3DIndexBuffer9Impl *)iface;
    HRESULT hr;
    WINED3DBUFFER_DESC desc;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DBuffer_GetDesc(This->wineD3DIndexBuffer, &desc);
    LeaveCriticalSection(&d3d9_cs);

    if (SUCCEEDED(hr)) {
        pDesc->Format = d3dformat_from_wined3dformat(This->format);
        pDesc->Usage = desc.Usage;
        pDesc->Pool = desc.Pool;
        pDesc->Size = desc.Size;
        pDesc->Type = D3DRTYPE_INDEXBUFFER;
    }

    return hr;
}


static const IDirect3DIndexBuffer9Vtbl Direct3DIndexBuffer9_Vtbl =
{
    /* IUnknown */
    IDirect3DIndexBuffer9Impl_QueryInterface,
    IDirect3DIndexBuffer9Impl_AddRef,
    IDirect3DIndexBuffer9Impl_Release,
    /* IDirect3DResource9 */
    IDirect3DIndexBuffer9Impl_GetDevice,
    IDirect3DIndexBuffer9Impl_SetPrivateData,
    IDirect3DIndexBuffer9Impl_GetPrivateData,
    IDirect3DIndexBuffer9Impl_FreePrivateData,
    IDirect3DIndexBuffer9Impl_SetPriority,
    IDirect3DIndexBuffer9Impl_GetPriority,
    IDirect3DIndexBuffer9Impl_PreLoad,
    IDirect3DIndexBuffer9Impl_GetType,
    /* IDirect3DIndexBuffer9 */
    IDirect3DIndexBuffer9Impl_Lock,
    IDirect3DIndexBuffer9Impl_Unlock,
    IDirect3DIndexBuffer9Impl_GetDesc
};


/* IDirect3DDevice9 IDirect3DIndexBuffer9 Methods follow: */
HRESULT WINAPI IDirect3DDevice9Impl_CreateIndexBuffer(LPDIRECT3DDEVICE9EX iface,
                              UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
                              IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* pSharedHandle) {
    
    IDirect3DIndexBuffer9Impl *object;
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hrc = D3D_OK;
    
    TRACE("(%p) Relay\n", This);
    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (NULL == object) {
        FIXME("Allocation of memory failed, returning D3DERR_OUTOFVIDEOMEMORY\n");
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DIndexBuffer9_Vtbl;
    object->ref = 1;
    object->format = wined3dformat_from_d3dformat(Format);
    TRACE("Calling wined3d create index buffer\n");
    EnterCriticalSection(&d3d9_cs);
    hrc = IWineD3DDevice_CreateIndexBuffer(This->WineD3DDevice, Length, Usage & WINED3DUSAGE_MASK,
            (WINED3DPOOL)Pool, &object->wineD3DIndexBuffer, (IUnknown *)object);
    LeaveCriticalSection(&d3d9_cs);
    if (hrc != D3D_OK) {

        /* free up object */
        FIXME("(%p) call to IWineD3DDevice_CreateIndexBuffer failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
    } else {
        IDirect3DDevice9Ex_AddRef(iface);
        object->parentDevice = iface;
        *ppIndexBuffer = (LPDIRECT3DINDEXBUFFER9) object;
        TRACE("(%p) : Created index buffer %p\n", This, object);
    }
    return hrc;
}
