/*
 * IDirect3DVolume9 implementation
 *
 * Copyright 2002-2005 Jason Edmeades
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

static inline IDirect3DVolume9Impl *impl_from_IDirect3DVolume9(IDirect3DVolume9 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DVolume9Impl, IDirect3DVolume9_iface);
}

static HRESULT WINAPI IDirect3DVolume9Impl_QueryInterface(IDirect3DVolume9 *iface, REFIID riid,
        void **ppobj)
{
    IDirect3DVolume9Impl *This = impl_from_IDirect3DVolume9(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DVolume9)) {
        IDirect3DVolume9_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DVolume9Impl_AddRef(IDirect3DVolume9 *iface)
{
    IDirect3DVolume9Impl *This = impl_from_IDirect3DVolume9(iface);

    TRACE("iface %p.\n", iface);

    if (This->forwardReference) {
        /* Forward refcounting */
        TRACE("(%p) : Forwarding to %p\n", This, This->forwardReference);
        return IUnknown_AddRef(This->forwardReference);
    } else {
        /* No container, handle our own refcounting */
        ULONG ref = InterlockedIncrement(&This->ref);

        TRACE("%p increasing refcount to %u.\n", iface, ref);

        if (ref == 1)
        {
            wined3d_mutex_lock();
            wined3d_volume_incref(This->wined3d_volume);
            wined3d_mutex_unlock();
        }

        return ref;
    }
}

static ULONG WINAPI IDirect3DVolume9Impl_Release(IDirect3DVolume9 *iface)
{
    IDirect3DVolume9Impl *This = impl_from_IDirect3DVolume9(iface);

    TRACE("iface %p.\n", iface);

    if (This->forwardReference) {
        /* Forward refcounting */
        TRACE("(%p) : Forwarding to %p\n", This, This->forwardReference);
        return IUnknown_Release(This->forwardReference);
    } else {
        /* No container, handle our own refcounting */
        ULONG ref = InterlockedDecrement(&This->ref);

        TRACE("%p decreasing refcount to %u.\n", iface, ref);

        if (ref == 0) {
            wined3d_mutex_lock();
            wined3d_volume_decref(This->wined3d_volume);
            wined3d_mutex_unlock();
        }

        return ref;
    }
}

static HRESULT WINAPI IDirect3DVolume9Impl_GetDevice(IDirect3DVolume9 *iface,
        IDirect3DDevice9 **device)
{
    IDirect3DVolume9Impl *This = impl_from_IDirect3DVolume9(iface);
    IDirect3DResource9 *resource;
    HRESULT hr;

    TRACE("iface %p, device %p.\n", iface, device);

    hr = IUnknown_QueryInterface(This->forwardReference, &IID_IDirect3DResource9, (void **)&resource);
    if (SUCCEEDED(hr))
    {
        hr = IDirect3DResource9_GetDevice(resource, device);
        IDirect3DResource9_Release(resource);

        TRACE("Returning device %p.\n", *device);
    }

    return hr;
}

static HRESULT WINAPI IDirect3DVolume9Impl_SetPrivateData(IDirect3DVolume9 *iface, REFGUID refguid,
        const void *pData, DWORD SizeOfData, DWORD Flags)
{
    IDirect3DVolume9Impl *This = impl_from_IDirect3DVolume9(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %u, flags %#x.\n",
            iface, debugstr_guid(refguid), pData, SizeOfData, Flags);

    wined3d_mutex_lock();
    resource = wined3d_volume_get_resource(This->wined3d_volume);
    hr = wined3d_resource_set_private_data(resource, refguid, pData, SizeOfData, Flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolume9Impl_GetPrivateData(IDirect3DVolume9 *iface, REFGUID  refguid,
        void *pData, DWORD *pSizeOfData)
{
    IDirect3DVolume9Impl *This = impl_from_IDirect3DVolume9(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %p.\n",
            iface, debugstr_guid(refguid), pData, pSizeOfData);

    wined3d_mutex_lock();
    resource = wined3d_volume_get_resource(This->wined3d_volume);
    hr = wined3d_resource_get_private_data(resource, refguid, pData, pSizeOfData);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolume9Impl_FreePrivateData(IDirect3DVolume9 *iface, REFGUID refguid)
{
    IDirect3DVolume9Impl *This = impl_from_IDirect3DVolume9(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(refguid));

    wined3d_mutex_lock();
    resource = wined3d_volume_get_resource(This->wined3d_volume);
    hr = wined3d_resource_free_private_data(resource, refguid);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolume9Impl_GetContainer(IDirect3DVolume9 *iface, REFIID riid,
        void **ppContainer)
{
    IDirect3DVolume9Impl *This = impl_from_IDirect3DVolume9(iface);
    HRESULT res;

    TRACE("iface %p, riid %s, container %p.\n", iface, debugstr_guid(riid), ppContainer);

    if (!This->container) return E_NOINTERFACE;

    res = IUnknown_QueryInterface(This->container, riid, ppContainer);

    TRACE("Returning ppContainer %p, *ppContainer %p\n", ppContainer, *ppContainer);

    return res;
}

static HRESULT WINAPI IDirect3DVolume9Impl_GetDesc(IDirect3DVolume9 *iface, D3DVOLUME_DESC *desc)
{
    IDirect3DVolume9Impl *This = impl_from_IDirect3DVolume9(iface);
    struct wined3d_resource_desc wined3d_desc;
    struct wined3d_resource *wined3d_resource;

    TRACE("iface %p, desc %p.\n", iface, desc);

    wined3d_mutex_lock();
    wined3d_resource = wined3d_volume_get_resource(This->wined3d_volume);
    wined3d_resource_get_desc(wined3d_resource, &wined3d_desc);
    wined3d_mutex_unlock();

    desc->Format = d3dformat_from_wined3dformat(wined3d_desc.format);
    desc->Type = wined3d_desc.resource_type;
    desc->Usage = wined3d_desc.usage & WINED3DUSAGE_MASK;
    desc->Pool = wined3d_desc.pool;
    desc->Width = wined3d_desc.width;
    desc->Height = wined3d_desc.height;
    desc->Depth = wined3d_desc.depth;

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DVolume9Impl_LockBox(IDirect3DVolume9 *iface,
        D3DLOCKED_BOX *pLockedVolume, const D3DBOX *pBox, DWORD Flags)
{
    IDirect3DVolume9Impl *This = impl_from_IDirect3DVolume9(iface);
    HRESULT hr;

    TRACE("iface %p, locked_box %p, box %p, flags %#x.\n",
            iface, pLockedVolume, pBox, Flags);

    wined3d_mutex_lock();
    hr = wined3d_volume_map(This->wined3d_volume, (struct wined3d_mapped_box *)pLockedVolume,
            (const struct wined3d_box *)pBox, Flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolume9Impl_UnlockBox(IDirect3DVolume9 *iface)
{
    IDirect3DVolume9Impl *This = impl_from_IDirect3DVolume9(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_volume_unmap(This->wined3d_volume);
    wined3d_mutex_unlock();

    return hr;
}

static const IDirect3DVolume9Vtbl Direct3DVolume9_Vtbl =
{
    /* IUnknown */
    IDirect3DVolume9Impl_QueryInterface,
    IDirect3DVolume9Impl_AddRef,
    IDirect3DVolume9Impl_Release,
    /* IDirect3DVolume9 */
    IDirect3DVolume9Impl_GetDevice,
    IDirect3DVolume9Impl_SetPrivateData,
    IDirect3DVolume9Impl_GetPrivateData,
    IDirect3DVolume9Impl_FreePrivateData,
    IDirect3DVolume9Impl_GetContainer,
    IDirect3DVolume9Impl_GetDesc,
    IDirect3DVolume9Impl_LockBox,
    IDirect3DVolume9Impl_UnlockBox
};

static void STDMETHODCALLTYPE volume_wined3d_object_destroyed(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d9_volume_wined3d_parent_ops =
{
    volume_wined3d_object_destroyed,
};

HRESULT volume_init(IDirect3DVolume9Impl *volume, IDirect3DDevice9Impl *device, UINT width, UINT height,
        UINT depth, DWORD usage, enum wined3d_format_id format, enum wined3d_pool pool)
{
    HRESULT hr;

    volume->IDirect3DVolume9_iface.lpVtbl = &Direct3DVolume9_Vtbl;
    volume->ref = 1;

    hr = wined3d_volume_create(device->wined3d_device, width, height, depth, usage & WINED3DUSAGE_MASK,
            format, pool, volume, &d3d9_volume_wined3d_parent_ops, &volume->wined3d_volume);
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d volume, hr %#x.\n", hr);
        return hr;
    }

    return D3D_OK;
}
