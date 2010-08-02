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

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource9)
        || IsEqualGUID(riid, &IID_IDirect3DVertexBuffer9)) {
        IDirect3DVertexBuffer9_AddRef(iface);
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

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    if (ref == 1)
    {
        IDirect3DDevice9Ex_AddRef(This->parentDevice);
        wined3d_mutex_lock();
        IWineD3DBuffer_AddRef(This->wineD3DVertexBuffer);
        wined3d_mutex_unlock();
    }

    return ref;
}

static ULONG WINAPI IDirect3DVertexBuffer9Impl_Release(LPDIRECT3DVERTEXBUFFER9 iface) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (ref == 0) {
        IDirect3DDevice9Ex *parentDevice = This->parentDevice;

        wined3d_mutex_lock();
        IWineD3DBuffer_Release(This->wineD3DVertexBuffer);
        wined3d_mutex_unlock();

        /* Release the device last, as it may cause the device to be destroyed. */
        IDirect3DDevice9Ex_Release(parentDevice);
    }
    return ref;
}

/* IDirect3DVertexBuffer9 IDirect3DResource9 Interface follow: */
static HRESULT WINAPI IDirect3DVertexBuffer9Impl_GetDevice(IDirect3DVertexBuffer9 *iface, IDirect3DDevice9 **device)
{
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)This->parentDevice;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DVertexBuffer9Impl_SetPrivateData(LPDIRECT3DVERTEXBUFFER9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %u, flags %#x.\n",
            iface, debugstr_guid(refguid), pData, SizeOfData, Flags);

    wined3d_mutex_lock();
    hr = IWineD3DBuffer_SetPrivateData(This->wineD3DVertexBuffer, refguid, pData, SizeOfData, Flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVertexBuffer9Impl_GetPrivateData(LPDIRECT3DVERTEXBUFFER9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %p.\n",
            iface, debugstr_guid(refguid), pData, pSizeOfData);

    wined3d_mutex_lock();
    hr = IWineD3DBuffer_GetPrivateData(This->wineD3DVertexBuffer, refguid, pData, pSizeOfData);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVertexBuffer9Impl_FreePrivateData(LPDIRECT3DVERTEXBUFFER9 iface, REFGUID refguid) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(refguid));

    wined3d_mutex_lock();
    hr = IWineD3DBuffer_FreePrivateData(This->wineD3DVertexBuffer, refguid);
    wined3d_mutex_unlock();

    return hr;
}

static DWORD WINAPI IDirect3DVertexBuffer9Impl_SetPriority(LPDIRECT3DVERTEXBUFFER9 iface, DWORD PriorityNew) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, priority %u.\n", iface, PriorityNew);

    wined3d_mutex_lock();
    hr = IWineD3DBuffer_SetPriority(This->wineD3DVertexBuffer, PriorityNew);
    wined3d_mutex_unlock();

    return hr;
}

static DWORD WINAPI IDirect3DVertexBuffer9Impl_GetPriority(LPDIRECT3DVERTEXBUFFER9 iface) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = IWineD3DBuffer_GetPriority(This->wineD3DVertexBuffer);
    wined3d_mutex_unlock();

    return hr;
}

static void WINAPI IDirect3DVertexBuffer9Impl_PreLoad(LPDIRECT3DVERTEXBUFFER9 iface) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    IWineD3DBuffer_PreLoad(This->wineD3DVertexBuffer);
    wined3d_mutex_unlock();
}

static D3DRESOURCETYPE WINAPI IDirect3DVertexBuffer9Impl_GetType(IDirect3DVertexBuffer9 *iface)
{
    TRACE("iface %p.\n", iface);

    return D3DRTYPE_VERTEXBUFFER;
}

/* IDirect3DVertexBuffer9 Interface follow: */
static HRESULT WINAPI IDirect3DVertexBuffer9Impl_Lock(LPDIRECT3DVERTEXBUFFER9 iface, UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, offset %u, size %u, data %p, flags %#x.\n",
            iface, OffsetToLock, SizeToLock, ppbData, Flags);

    wined3d_mutex_lock();
    hr = IWineD3DBuffer_Map(This->wineD3DVertexBuffer, OffsetToLock, SizeToLock, (BYTE **)ppbData, Flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVertexBuffer9Impl_Unlock(LPDIRECT3DVERTEXBUFFER9 iface) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = IWineD3DBuffer_Unmap(This->wineD3DVertexBuffer);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVertexBuffer9Impl_GetDesc(LPDIRECT3DVERTEXBUFFER9 iface, D3DVERTEXBUFFER_DESC* pDesc) {
    IDirect3DVertexBuffer9Impl *This = (IDirect3DVertexBuffer9Impl *)iface;
    HRESULT hr;
    WINED3DBUFFER_DESC desc;

    TRACE("iface %p, desc %p.\n", iface, pDesc);

    wined3d_mutex_lock();
    hr = IWineD3DBuffer_GetDesc(This->wineD3DVertexBuffer, &desc);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr)) {
        pDesc->Format = D3DFMT_VERTEXDATA;
        pDesc->Usage = desc.Usage;
        pDesc->Pool = desc.Pool;
        pDesc->Size = desc.Size;
        pDesc->Type = D3DRTYPE_VERTEXBUFFER;
        pDesc->FVF = This->fvf;
    }


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

static void STDMETHODCALLTYPE d3d9_vertexbuffer_wined3d_object_destroyed(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d9_vertexbuffer_wined3d_parent_ops =
{
    d3d9_vertexbuffer_wined3d_object_destroyed,
};

HRESULT vertexbuffer_init(IDirect3DVertexBuffer9Impl *buffer, IDirect3DDevice9Impl *device,
        UINT size, UINT usage, DWORD fvf, D3DPOOL pool)
{
    HRESULT hr;

    buffer->lpVtbl = &Direct3DVertexBuffer9_Vtbl;
    buffer->ref = 1;
    buffer->fvf = fvf;

    wined3d_mutex_lock();
    hr = IWineD3DDevice_CreateVertexBuffer(device->WineD3DDevice, size,
            usage & WINED3DUSAGE_MASK, (WINED3DPOOL)pool, &buffer->wineD3DVertexBuffer,
            (IUnknown *)buffer, &d3d9_vertexbuffer_wined3d_parent_ops);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d buffer, hr %#x.\n", hr);
        return hr;
    }

    buffer->parentDevice = (IDirect3DDevice9Ex *)device;
    IDirect3DDevice9Ex_AddRef(buffer->parentDevice);

    return D3D_OK;
}
