/*
 * IDirect3DVertexBuffer9 implementation
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2002-2004 Raphael Junqueira
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

/* IDirect3DVertexBuffer9 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DVertexBuffer9Impl_QueryInterface(LPDIRECT3DVERTEXBUFFER9 iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource9)
        || IsEqualGUID(riid, &IID_IDirect3DVertexBuffer9)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DVertexBuffer9Impl_AddRef(LPDIRECT3DVERTEXBUFFER9 iface) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);

    return ref;
}

static ULONG WINAPI IDirect3DVertexBuffer9Impl_Release(LPDIRECT3DVERTEXBUFFER9 iface) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

    if (ref == 0) {
        EnterCriticalSection(&d3d9_cs);
        IWineD3DVertexBuffer_Release(This->wineD3DVertexBuffer);
        LeaveCriticalSection(&d3d9_cs);
        IUnknown_Release(This->parentDevice);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DVertexBuffer9 IDirect3DResource9 Interface follow: */
static HRESULT WINAPI IDirect3DVertexBuffer9Impl_GetDevice(LPDIRECT3DVERTEXBUFFER9 iface, IDirect3DDevice9** ppDevice) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IDirect3DResource9Impl_GetDevice((LPDIRECT3DRESOURCE9) This, ppDevice);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DVertexBuffer9Impl_SetPrivateData(LPDIRECT3DVERTEXBUFFER9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    TRACE("(%p) Relay\n", This);
    return IWineD3DVertexBuffer_SetPrivateData(This->wineD3DVertexBuffer, refguid, pData, SizeOfData, Flags);
}

static HRESULT WINAPI IDirect3DVertexBuffer9Impl_GetPrivateData(LPDIRECT3DVERTEXBUFFER9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DVertexBuffer_GetPrivateData(This->wineD3DVertexBuffer, refguid, pData, pSizeOfData);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DVertexBuffer9Impl_FreePrivateData(LPDIRECT3DVERTEXBUFFER9 iface, REFGUID refguid) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DVertexBuffer_FreePrivateData(This->wineD3DVertexBuffer, refguid);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static DWORD WINAPI IDirect3DVertexBuffer9Impl_SetPriority(LPDIRECT3DVERTEXBUFFER9 iface, DWORD PriorityNew) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DVertexBuffer_SetPriority(This->wineD3DVertexBuffer, PriorityNew);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static DWORD WINAPI IDirect3DVertexBuffer9Impl_GetPriority(LPDIRECT3DVERTEXBUFFER9 iface) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DVertexBuffer_GetPriority(This->wineD3DVertexBuffer);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static void WINAPI IDirect3DVertexBuffer9Impl_PreLoad(LPDIRECT3DVERTEXBUFFER9 iface) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    IWineD3DVertexBuffer_PreLoad(This->wineD3DVertexBuffer);
    LeaveCriticalSection(&d3d9_cs);
    return ;
}

static D3DRESOURCETYPE WINAPI IDirect3DVertexBuffer9Impl_GetType(LPDIRECT3DVERTEXBUFFER9 iface) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    D3DRESOURCETYPE ret;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    ret = IWineD3DVertexBuffer_GetType(This->wineD3DVertexBuffer);
    LeaveCriticalSection(&d3d9_cs);
    return ret;
}

/* IDirect3DVertexBuffer9 Interface follow: */
static HRESULT WINAPI IDirect3DVertexBuffer9Impl_Lock(LPDIRECT3DVERTEXBUFFER9 iface, UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DVertexBuffer_Lock(This->wineD3DVertexBuffer, OffsetToLock, SizeToLock, (BYTE **)ppbData, Flags);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DVertexBuffer9Impl_Unlock(LPDIRECT3DVERTEXBUFFER9 iface) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DVertexBuffer_Unlock(This->wineD3DVertexBuffer);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DVertexBuffer9Impl_GetDesc(LPDIRECT3DVERTEXBUFFER9 iface, D3DVERTEXBUFFER_DESC* pDesc) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3DVertexBuffer_GetDesc(This->wineD3DVertexBuffer, (WINED3DVERTEXBUFFER_DESC *) pDesc);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static const IDirect3DVertexBuffer9Vtbl Direct3DVertexBuffer9_Vtbl =
{
    /* IUnknown */
    IDirect3DVertexBuffer9Impl_QueryInterface,
    IDirect3DVertexBuffer9Impl_AddRef,
    IDirect3DVertexBuffer9Impl_Release,
    /* IDirect3DResource9 */
    IDirect3DVertexBuffer9Impl_GetDevice,
    IDirect3DVertexBuffer9Impl_SetPrivateData,
    IDirect3DVertexBuffer9Impl_GetPrivateData,
    IDirect3DVertexBuffer9Impl_FreePrivateData,
    IDirect3DVertexBuffer9Impl_SetPriority,
    IDirect3DVertexBuffer9Impl_GetPriority,
    IDirect3DVertexBuffer9Impl_PreLoad,
    IDirect3DVertexBuffer9Impl_GetType,
    /* IDirect3DVertexBuffer9 */
    IDirect3DVertexBuffer9Impl_Lock,
    IDirect3DVertexBuffer9Impl_Unlock,
    IDirect3DVertexBuffer9Impl_GetDesc
};


/* IDirect3DDevice9 IDirect3DVertexBuffer9 Methods follow: */
HRESULT WINAPI IDirect3DDevice9Impl_CreateVertexBuffer(LPDIRECT3DDEVICE9 iface, 
                                UINT Size, DWORD Usage, DWORD FVF, D3DPOOL Pool,
                                IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle) {
    
    IDirect3DVertexBuffer9Impl *object;
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hrc = D3D_OK;

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DVertexBuffer9Impl));
    if (NULL == object) {
        FIXME("Allocation of memory failed, returning D3DERR_OUTOFVIDEOMEMORY\n");
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DVertexBuffer9_Vtbl;
    object->ref = 1;
    EnterCriticalSection(&d3d9_cs);
    hrc = IWineD3DDevice_CreateVertexBuffer(This->WineD3DDevice, Size, Usage & WINED3DUSAGE_MASK, FVF, (WINED3DPOOL) Pool, &(object->wineD3DVertexBuffer), pSharedHandle, (IUnknown *)object);
    LeaveCriticalSection(&d3d9_cs);
    
    if (hrc != D3D_OK) {

        /* free up object */
        WARN("(%p) call to IWineD3DDevice_CreateVertexBuffer failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
    } else {
        IUnknown_AddRef(iface);
        object->parentDevice = iface;
        TRACE("(%p) : Created vertex buffer %p\n", This, object);
        *ppVertexBuffer = (LPDIRECT3DVERTEXBUFFER9) object;
    }
    return hrc;
}
