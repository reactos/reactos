/*
 * IDirect3DVolumeTexture8 implementation
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

static inline IDirect3DVolumeTexture8Impl *impl_from_IDirect3DVolumeTexture8(IDirect3DVolumeTexture8 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DVolumeTexture8Impl, IDirect3DVolumeTexture8_iface);
}

static HRESULT WINAPI IDirect3DVolumeTexture8Impl_QueryInterface(IDirect3DVolumeTexture8 *iface,
        REFIID riid, void **ppobj)
{
    IDirect3DVolumeTexture8Impl *This = impl_from_IDirect3DVolumeTexture8(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
    || IsEqualGUID(riid, &IID_IDirect3DResource8)
    || IsEqualGUID(riid, &IID_IDirect3DBaseTexture8)
    || IsEqualGUID(riid, &IID_IDirect3DVolumeTexture8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DVolumeTexture8Impl_AddRef(IDirect3DVolumeTexture8 *iface)
{
    IDirect3DVolumeTexture8Impl *This = impl_from_IDirect3DVolumeTexture8(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    if (ref == 1)
    {
        IDirect3DDevice8_AddRef(This->parentDevice);
        wined3d_mutex_lock();
        wined3d_texture_incref(This->wined3d_texture);
        wined3d_mutex_unlock();
    }

    return ref;
}

static ULONG WINAPI IDirect3DVolumeTexture8Impl_Release(IDirect3DVolumeTexture8 *iface)
{
    IDirect3DVolumeTexture8Impl *This = impl_from_IDirect3DVolumeTexture8(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (ref == 0) {
        IDirect3DDevice8 *parentDevice = This->parentDevice;

        wined3d_mutex_lock();
        wined3d_texture_decref(This->wined3d_texture);
        wined3d_mutex_unlock();

        /* Release the device last, as it may cause the device to be destroyed. */
        IDirect3DDevice8_Release(parentDevice);
    }
    return ref;
}

/* IDirect3DVolumeTexture8 IDirect3DResource8 Interface follow: */
static HRESULT WINAPI IDirect3DVolumeTexture8Impl_GetDevice(IDirect3DVolumeTexture8 *iface,
        IDirect3DDevice8 **device)
{
    IDirect3DVolumeTexture8Impl *This = impl_from_IDirect3DVolumeTexture8(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = This->parentDevice;
    IDirect3DDevice8_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DVolumeTexture8Impl_SetPrivateData(IDirect3DVolumeTexture8 *iface,
        REFGUID refguid, const void *pData, DWORD SizeOfData, DWORD Flags)
{
    IDirect3DVolumeTexture8Impl *This = impl_from_IDirect3DVolumeTexture8(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %u, flags %#x.\n",
            iface, debugstr_guid(refguid), pData, SizeOfData, Flags);

    wined3d_mutex_lock();
    resource = wined3d_texture_get_resource(This->wined3d_texture);
    hr = wined3d_resource_set_private_data(resource, refguid, pData, SizeOfData, Flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolumeTexture8Impl_GetPrivateData(IDirect3DVolumeTexture8 *iface,
        REFGUID refguid, void *pData, DWORD *pSizeOfData)
{
    IDirect3DVolumeTexture8Impl *This = impl_from_IDirect3DVolumeTexture8(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %p.\n",
            iface, debugstr_guid(refguid), pData, pSizeOfData);

    wined3d_mutex_lock();
    resource = wined3d_texture_get_resource(This->wined3d_texture);
    hr = wined3d_resource_get_private_data(resource, refguid, pData, pSizeOfData);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolumeTexture8Impl_FreePrivateData(IDirect3DVolumeTexture8 *iface,
        REFGUID refguid)
{
    IDirect3DVolumeTexture8Impl *This = impl_from_IDirect3DVolumeTexture8(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(refguid));

    wined3d_mutex_lock();
    resource = wined3d_texture_get_resource(This->wined3d_texture);
    hr = wined3d_resource_free_private_data(resource, refguid);
    wined3d_mutex_unlock();

    return hr;
}

static DWORD WINAPI IDirect3DVolumeTexture8Impl_SetPriority(IDirect3DVolumeTexture8 *iface,
        DWORD PriorityNew)
{
    IDirect3DVolumeTexture8Impl *This = impl_from_IDirect3DVolumeTexture8(iface);
    DWORD ret;

    TRACE("iface %p, priority %u.\n", iface, PriorityNew);

    wined3d_mutex_lock();
    ret = wined3d_texture_set_priority(This->wined3d_texture, PriorityNew);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI IDirect3DVolumeTexture8Impl_GetPriority(IDirect3DVolumeTexture8 *iface)
{
    IDirect3DVolumeTexture8Impl *This = impl_from_IDirect3DVolumeTexture8(iface);
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = wined3d_texture_get_priority(This->wined3d_texture);
    wined3d_mutex_unlock();

    return ret;
}

static void WINAPI IDirect3DVolumeTexture8Impl_PreLoad(IDirect3DVolumeTexture8 *iface)
{
    IDirect3DVolumeTexture8Impl *This = impl_from_IDirect3DVolumeTexture8(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_texture_preload(This->wined3d_texture);
    wined3d_mutex_unlock();
}

static D3DRESOURCETYPE WINAPI IDirect3DVolumeTexture8Impl_GetType(IDirect3DVolumeTexture8 *iface)
{
    TRACE("iface %p.\n", iface);

    return D3DRTYPE_VOLUMETEXTURE;
}

/* IDirect3DVolumeTexture8 IDirect3DBaseTexture8 Interface follow: */
static DWORD WINAPI IDirect3DVolumeTexture8Impl_SetLOD(IDirect3DVolumeTexture8 *iface, DWORD LODNew)
{
    IDirect3DVolumeTexture8Impl *This = impl_from_IDirect3DVolumeTexture8(iface);
    DWORD ret;

    TRACE("iface %p, lod %u.\n", iface, LODNew);

    wined3d_mutex_lock();
    ret = wined3d_texture_set_lod(This->wined3d_texture, LODNew);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI IDirect3DVolumeTexture8Impl_GetLOD(IDirect3DVolumeTexture8 *iface)
{
    IDirect3DVolumeTexture8Impl *This = impl_from_IDirect3DVolumeTexture8(iface);
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = wined3d_texture_get_lod(This->wined3d_texture);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI IDirect3DVolumeTexture8Impl_GetLevelCount(IDirect3DVolumeTexture8 *iface)
{
    IDirect3DVolumeTexture8Impl *This = impl_from_IDirect3DVolumeTexture8(iface);
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = wined3d_texture_get_level_count(This->wined3d_texture);
    wined3d_mutex_unlock();

    return ret;
}

static HRESULT WINAPI IDirect3DVolumeTexture8Impl_GetLevelDesc(IDirect3DVolumeTexture8 *iface,
        UINT level, D3DVOLUME_DESC *desc)
{
    IDirect3DVolumeTexture8Impl *texture = impl_from_IDirect3DVolumeTexture8(iface);
    struct wined3d_resource *sub_resource;
    HRESULT hr = D3D_OK;

    TRACE("iface %p, level %u, desc %p.\n", iface, level, desc);

    wined3d_mutex_lock();
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, level)))
        hr = D3DERR_INVALIDCALL;
    else
    {
        struct wined3d_resource_desc wined3d_desc;

        wined3d_resource_get_desc(sub_resource, &wined3d_desc);
        desc->Format = d3dformat_from_wined3dformat(wined3d_desc.format);
        desc->Type = wined3d_desc.resource_type;
        desc->Usage = wined3d_desc.usage & WINED3DUSAGE_MASK;
        desc->Pool = wined3d_desc.pool;
        desc->Size = wined3d_desc.size;
        desc->Width = wined3d_desc.width;
        desc->Height = wined3d_desc.height;
        desc->Depth = wined3d_desc.depth;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolumeTexture8Impl_GetVolumeLevel(IDirect3DVolumeTexture8 *iface,
        UINT level, IDirect3DVolume8 **volume)
{
    IDirect3DVolumeTexture8Impl *texture = impl_from_IDirect3DVolumeTexture8(iface);
    struct wined3d_resource *sub_resource;

    TRACE("iface %p, level %u, volume %p.\n", iface, level, volume);

    wined3d_mutex_lock();
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, level)))
    {
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

    *volume = wined3d_resource_get_parent(sub_resource);
    IDirect3DVolume8_AddRef(*volume);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DVolumeTexture8Impl_LockBox(IDirect3DVolumeTexture8 *iface,
        UINT level, D3DLOCKED_BOX *locked_box, const D3DBOX *box, DWORD flags)
{
    IDirect3DVolumeTexture8Impl *texture = impl_from_IDirect3DVolumeTexture8(iface);
    struct wined3d_resource *sub_resource;
    HRESULT hr;

    TRACE("iface %p, level %u, locked_box %p, box %p, flags %#x.\n",
            iface, level, locked_box, box, flags);

    wined3d_mutex_lock();
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, level)))
        hr = D3DERR_INVALIDCALL;
    else
        hr = IDirect3DVolume8_LockBox((IDirect3DVolume8 *)wined3d_resource_get_parent(sub_resource),
                locked_box, box, flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolumeTexture8Impl_UnlockBox(IDirect3DVolumeTexture8 *iface,
        UINT level)
{
    IDirect3DVolumeTexture8Impl *texture = impl_from_IDirect3DVolumeTexture8(iface);
    struct wined3d_resource *sub_resource;
    HRESULT hr;

    TRACE("iface %p, level %u.\n", iface, level);

    wined3d_mutex_lock();
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, level)))
        hr = D3DERR_INVALIDCALL;
    else
        hr = IDirect3DVolume8_UnlockBox((IDirect3DVolume8 *)wined3d_resource_get_parent(sub_resource));
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolumeTexture8Impl_AddDirtyBox(IDirect3DVolumeTexture8 *iface,
        const D3DBOX *dirty_box)
{
    IDirect3DVolumeTexture8Impl *texture = impl_from_IDirect3DVolumeTexture8(iface);
    HRESULT hr;

    TRACE("iface %p, dirty_box %p.\n", iface, dirty_box);

    wined3d_mutex_lock();
    hr = wined3d_texture_add_dirty_region(texture->wined3d_texture, 0, (const struct wined3d_box *)dirty_box);
    wined3d_mutex_unlock();

    return hr;
}

static const IDirect3DVolumeTexture8Vtbl Direct3DVolumeTexture8_Vtbl =
{
    /* IUnknown */
    IDirect3DVolumeTexture8Impl_QueryInterface,
    IDirect3DVolumeTexture8Impl_AddRef,
    IDirect3DVolumeTexture8Impl_Release,
    /* IDirect3DResource8 */
    IDirect3DVolumeTexture8Impl_GetDevice,
    IDirect3DVolumeTexture8Impl_SetPrivateData,
    IDirect3DVolumeTexture8Impl_GetPrivateData,
    IDirect3DVolumeTexture8Impl_FreePrivateData,
    IDirect3DVolumeTexture8Impl_SetPriority,
    IDirect3DVolumeTexture8Impl_GetPriority,
    IDirect3DVolumeTexture8Impl_PreLoad,
    IDirect3DVolumeTexture8Impl_GetType,
    /* IDirect3DBaseTexture8 */
    IDirect3DVolumeTexture8Impl_SetLOD,
    IDirect3DVolumeTexture8Impl_GetLOD,
    IDirect3DVolumeTexture8Impl_GetLevelCount,
    /* IDirect3DVolumeTexture8 */
    IDirect3DVolumeTexture8Impl_GetLevelDesc,
    IDirect3DVolumeTexture8Impl_GetVolumeLevel,
    IDirect3DVolumeTexture8Impl_LockBox,
    IDirect3DVolumeTexture8Impl_UnlockBox,
    IDirect3DVolumeTexture8Impl_AddDirtyBox
};

static void STDMETHODCALLTYPE volumetexture_wined3d_object_destroyed(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d8_volumetexture_wined3d_parent_ops =
{
    volumetexture_wined3d_object_destroyed,
};

HRESULT volumetexture_init(IDirect3DVolumeTexture8Impl *texture, IDirect3DDevice8Impl *device,
        UINT width, UINT height, UINT depth, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool)
{
    HRESULT hr;

    texture->IDirect3DVolumeTexture8_iface.lpVtbl = &Direct3DVolumeTexture8_Vtbl;
    texture->ref = 1;

    wined3d_mutex_lock();
    hr = wined3d_texture_create_3d(device->wined3d_device, width, height, depth, levels,
            usage & WINED3DUSAGE_MASK, wined3dformat_from_d3dformat(format), pool, texture,
            &d3d8_volumetexture_wined3d_parent_ops, &texture->wined3d_texture);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d volume texture, hr %#x.\n", hr);
        return hr;
    }

    texture->parentDevice = &device->IDirect3DDevice8_iface;
    IDirect3DDevice8_AddRef(texture->parentDevice);

    return D3D_OK;
}
