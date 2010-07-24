/*
 * IDirect3DVolume8 implementation
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

/* IDirect3DVolume8 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DVolume8Impl_QueryInterface(LPDIRECT3DVOLUME8 iface, REFIID riid, LPVOID *ppobj) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DVolume8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DVolume8Impl_AddRef(LPDIRECT3DVOLUME8 iface) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;

    TRACE("iface %p.\n", iface);

    if (This->forwardReference) {
        /* Forward to the containerParent */
        TRACE("(%p) : Forwarding to %p\n", This, This->forwardReference);
        return IUnknown_AddRef(This->forwardReference);
    } else {
        /* No container, handle our own refcounting */
        ULONG ref = InterlockedIncrement(&This->ref);

        TRACE("%p increasing refcount to %u.\n", iface, ref);

        if (ref == 1)
        {
            wined3d_mutex_lock();
            IWineD3DVolume_AddRef(This->wineD3DVolume);
            wined3d_mutex_unlock();
        }

        return ref;
    }
}

static ULONG WINAPI IDirect3DVolume8Impl_Release(LPDIRECT3DVOLUME8 iface) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;

    TRACE("iface %p.\n", iface);

    if (This->forwardReference) {
        /* Forward to the containerParent */
        TRACE("(%p) : Forwarding to %p\n", This, This->forwardReference);
        return IUnknown_Release(This->forwardReference);
    }
    else {
        /* No container, handle our own refcounting */
        ULONG ref = InterlockedDecrement(&This->ref);

        TRACE("%p decreasing refcount to %u.\n", iface, ref);

        if (ref == 0) {
            wined3d_mutex_lock();
            IWineD3DVolume_Release(This->wineD3DVolume);
            wined3d_mutex_unlock();
        }

        return ref;
    }
}

/* IDirect3DVolume8 Interface follow: */
static HRESULT WINAPI IDirect3DVolume8Impl_GetDevice(IDirect3DVolume8 *iface, IDirect3DDevice8 **device)
{
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    IDirect3DResource8 *resource;
    HRESULT hr;

    TRACE("iface %p, device %p.\n", iface, device);

    hr = IUnknown_QueryInterface(This->forwardReference, &IID_IDirect3DResource8, (void **)&resource);
    if (SUCCEEDED(hr))
    {
        hr = IDirect3DResource8_GetDevice(resource, device);
        IDirect3DResource8_Release(resource);

        TRACE("Returning device %p.\n", *device);
    }

    return hr;
}

static HRESULT WINAPI IDirect3DVolume8Impl_SetPrivateData(LPDIRECT3DVOLUME8 iface, REFGUID refguid, CONST void *pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %u, flags %#x.\n",
            iface, debugstr_guid(refguid), pData, SizeOfData, Flags);

    wined3d_mutex_lock();
    hr = IWineD3DVolume_SetPrivateData(This->wineD3DVolume, refguid, pData, SizeOfData, Flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolume8Impl_GetPrivateData(LPDIRECT3DVOLUME8 iface, REFGUID  refguid, void *pData, DWORD* pSizeOfData) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %p.\n",
            iface, debugstr_guid(refguid), pData, pSizeOfData);

    wined3d_mutex_lock();
    hr = IWineD3DVolume_GetPrivateData(This->wineD3DVolume, refguid, pData, pSizeOfData);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolume8Impl_FreePrivateData(LPDIRECT3DVOLUME8 iface, REFGUID refguid) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(refguid));

    wined3d_mutex_lock();
    hr = IWineD3DVolume_FreePrivateData(This->wineD3DVolume, refguid);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolume8Impl_GetContainer(LPDIRECT3DVOLUME8 iface, REFIID riid, void **ppContainer) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    HRESULT res;

    TRACE("iface %p, riid %s, container %p.\n",
            iface, debugstr_guid(riid), ppContainer);

    if (!This->container) return E_NOINTERFACE;

    if (!ppContainer) {
        ERR("Called without a valid ppContainer.\n");
    }

    res = IUnknown_QueryInterface(This->container, riid, ppContainer);

    TRACE("Returning ppContainer %p, *ppContainer %p\n", ppContainer, *ppContainer);

    return res;
}

static HRESULT WINAPI IDirect3DVolume8Impl_GetDesc(LPDIRECT3DVOLUME8 iface, D3DVOLUME_DESC *pDesc) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    HRESULT hr;
    WINED3DVOLUME_DESC     wined3ddesc;

    TRACE("iface %p, desc %p.\n", iface, pDesc);

    wined3d_mutex_lock();
    hr = IWineD3DVolume_GetDesc(This->wineD3DVolume, &wined3ddesc);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr))
    {
        pDesc->Format = d3dformat_from_wined3dformat(wined3ddesc.Format);
        pDesc->Type = wined3ddesc.Type;
        pDesc->Usage = wined3ddesc.Usage;
        pDesc->Pool = wined3ddesc.Pool;
        pDesc->Size = wined3ddesc.Size;
        pDesc->Width = wined3ddesc.Width;
        pDesc->Height = wined3ddesc.Height;
        pDesc->Depth = wined3ddesc.Depth;
    }

    return hr;
}

static HRESULT WINAPI IDirect3DVolume8Impl_LockBox(LPDIRECT3DVOLUME8 iface, D3DLOCKED_BOX *pLockedVolume, CONST D3DBOX *pBox, DWORD Flags) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, locked_box %p, box %p, flags %#x.\n",
            iface, pLockedVolume, pBox, Flags);

    wined3d_mutex_lock();
    hr = IWineD3DVolume_LockBox(This->wineD3DVolume, (WINED3DLOCKED_BOX *) pLockedVolume, (CONST WINED3DBOX *) pBox, Flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolume8Impl_UnlockBox(LPDIRECT3DVOLUME8 iface) {
    IDirect3DVolume8Impl *This = (IDirect3DVolume8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = IWineD3DVolume_UnlockBox(This->wineD3DVolume);
    wined3d_mutex_unlock();

    return hr;
}

static const IDirect3DVolume8Vtbl Direct3DVolume8_Vtbl =
{
    /* IUnknown */
    IDirect3DVolume8Impl_QueryInterface,
    IDirect3DVolume8Impl_AddRef,
    IDirect3DVolume8Impl_Release,
    /* IDirect3DVolume8 */
    IDirect3DVolume8Impl_GetDevice,
    IDirect3DVolume8Impl_SetPrivateData,
    IDirect3DVolume8Impl_GetPrivateData,
    IDirect3DVolume8Impl_FreePrivateData,
    IDirect3DVolume8Impl_GetContainer,
    IDirect3DVolume8Impl_GetDesc,
    IDirect3DVolume8Impl_LockBox,
    IDirect3DVolume8Impl_UnlockBox
};

static void STDMETHODCALLTYPE volume_wined3d_object_destroyed(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d8_volume_wined3d_parent_ops =
{
    volume_wined3d_object_destroyed,
};

HRESULT volume_init(IDirect3DVolume8Impl *volume, IDirect3DDevice8Impl *device, UINT width, UINT height,
        UINT depth, DWORD usage, WINED3DFORMAT format, WINED3DPOOL pool)
{
    HRESULT hr;

    volume->lpVtbl = &Direct3DVolume8_Vtbl;
    volume->ref = 1;

    hr = IWineD3DDevice_CreateVolume(device->WineD3DDevice, width, height, depth, usage,
            format, pool, &volume->wineD3DVolume, (IUnknown *)volume, &d3d8_volume_wined3d_parent_ops);
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d volume, hr %#x.\n", hr);
        return hr;
    }

    return D3D_OK;
}
