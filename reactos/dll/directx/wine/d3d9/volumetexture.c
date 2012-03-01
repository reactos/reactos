/*
 * IDirect3DVolumeTexture9 implementation
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

static inline IDirect3DVolumeTexture9Impl *impl_from_IDirect3DVolumeTexture9(IDirect3DVolumeTexture9 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DVolumeTexture9Impl, IDirect3DVolumeTexture9_iface);
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_QueryInterface(IDirect3DVolumeTexture9 *iface,
        REFIID riid, void **ppobj)
{
    IDirect3DVolumeTexture9Impl *This = impl_from_IDirect3DVolumeTexture9(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
    || IsEqualGUID(riid, &IID_IDirect3DResource9)
    || IsEqualGUID(riid, &IID_IDirect3DBaseTexture9)
    || IsEqualGUID(riid, &IID_IDirect3DVolumeTexture9)) {
        IDirect3DVolumeTexture9_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DVolumeTexture9Impl_AddRef(IDirect3DVolumeTexture9 *iface)
{
    IDirect3DVolumeTexture9Impl *This = impl_from_IDirect3DVolumeTexture9(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    if (ref == 1)
    {
        IDirect3DDevice9Ex_AddRef(This->parentDevice);
        wined3d_mutex_lock();
        wined3d_texture_incref(This->wined3d_texture);
        wined3d_mutex_unlock();
    }

    return ref;
}

static ULONG WINAPI IDirect3DVolumeTexture9Impl_Release(IDirect3DVolumeTexture9 *iface)
{
    IDirect3DVolumeTexture9Impl *This = impl_from_IDirect3DVolumeTexture9(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (ref == 0) {
        IDirect3DDevice9Ex *parentDevice = This->parentDevice;

        wined3d_mutex_lock();
        wined3d_texture_decref(This->wined3d_texture);
        wined3d_mutex_unlock();

        /* Release the device last, as it may cause the device to be destroyed. */
        IDirect3DDevice9Ex_Release(parentDevice);
    }
    return ref;
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_GetDevice(IDirect3DVolumeTexture9 *iface,
        IDirect3DDevice9 **device)
{
    IDirect3DVolumeTexture9Impl *This = impl_from_IDirect3DVolumeTexture9(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)This->parentDevice;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_SetPrivateData(IDirect3DVolumeTexture9 *iface,
        REFGUID refguid, const void *pData, DWORD SizeOfData, DWORD Flags)
{
    IDirect3DVolumeTexture9Impl *This = impl_from_IDirect3DVolumeTexture9(iface);
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

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_GetPrivateData(IDirect3DVolumeTexture9 *iface,
        REFGUID refguid, void *pData, DWORD *pSizeOfData)
{
    IDirect3DVolumeTexture9Impl *This = impl_from_IDirect3DVolumeTexture9(iface);
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

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_FreePrivateData(IDirect3DVolumeTexture9 *iface,
        REFGUID refguid)
{
    IDirect3DVolumeTexture9Impl *This = impl_from_IDirect3DVolumeTexture9(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(refguid));

    wined3d_mutex_lock();
    resource = wined3d_texture_get_resource(This->wined3d_texture);
    hr = wined3d_resource_free_private_data(resource, refguid);
    wined3d_mutex_unlock();

    return hr;
}

static DWORD WINAPI IDirect3DVolumeTexture9Impl_SetPriority(IDirect3DVolumeTexture9 *iface,
        DWORD PriorityNew)
{
    IDirect3DVolumeTexture9Impl *This = impl_from_IDirect3DVolumeTexture9(iface);
    DWORD priority;

    TRACE("iface %p, priority %u.\n", iface, PriorityNew);

    wined3d_mutex_lock();
    priority = wined3d_texture_set_priority(This->wined3d_texture, PriorityNew);
    wined3d_mutex_unlock();

    return priority;
}

static DWORD WINAPI IDirect3DVolumeTexture9Impl_GetPriority(IDirect3DVolumeTexture9 *iface)
{
    IDirect3DVolumeTexture9Impl *This = impl_from_IDirect3DVolumeTexture9(iface);
    DWORD priority;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    priority = wined3d_texture_get_priority(This->wined3d_texture);
    wined3d_mutex_unlock();

    return priority;
}

static void WINAPI IDirect3DVolumeTexture9Impl_PreLoad(IDirect3DVolumeTexture9 *iface)
{
    IDirect3DVolumeTexture9Impl *This = impl_from_IDirect3DVolumeTexture9(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_texture_preload(This->wined3d_texture);
    wined3d_mutex_unlock();
}

static D3DRESOURCETYPE WINAPI IDirect3DVolumeTexture9Impl_GetType(IDirect3DVolumeTexture9 *iface)
{
    TRACE("iface %p.\n", iface);

    return D3DRTYPE_VOLUMETEXTURE;
}

static DWORD WINAPI IDirect3DVolumeTexture9Impl_SetLOD(IDirect3DVolumeTexture9 *iface, DWORD LODNew)
{
    IDirect3DVolumeTexture9Impl *This = impl_from_IDirect3DVolumeTexture9(iface);
    DWORD lod;

    TRACE("iface %p, lod %u.\n", iface, LODNew);

    wined3d_mutex_lock();
    lod = wined3d_texture_set_lod(This->wined3d_texture, LODNew);
    wined3d_mutex_unlock();

    return lod;
}

static DWORD WINAPI IDirect3DVolumeTexture9Impl_GetLOD(IDirect3DVolumeTexture9 *iface)
{
    IDirect3DVolumeTexture9Impl *This = impl_from_IDirect3DVolumeTexture9(iface);
    DWORD lod;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    lod = wined3d_texture_get_lod(This->wined3d_texture);
    wined3d_mutex_unlock();

    return lod;
}

static DWORD WINAPI IDirect3DVolumeTexture9Impl_GetLevelCount(IDirect3DVolumeTexture9 *iface)
{
    IDirect3DVolumeTexture9Impl *This = impl_from_IDirect3DVolumeTexture9(iface);
    DWORD level_count;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    level_count = wined3d_texture_get_level_count(This->wined3d_texture);
    wined3d_mutex_unlock();

    return level_count;
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_SetAutoGenFilterType(IDirect3DVolumeTexture9 *iface,
        D3DTEXTUREFILTERTYPE FilterType)
{
    IDirect3DVolumeTexture9Impl *This = impl_from_IDirect3DVolumeTexture9(iface);
    HRESULT hr;

    TRACE("iface %p, filter_type %#x.\n", iface, FilterType);

    wined3d_mutex_lock();
    hr = wined3d_texture_set_autogen_filter_type(This->wined3d_texture, (enum wined3d_texture_filter_type)FilterType);
    wined3d_mutex_unlock();

    return hr;
}

static D3DTEXTUREFILTERTYPE WINAPI IDirect3DVolumeTexture9Impl_GetAutoGenFilterType(IDirect3DVolumeTexture9 *iface)
{
    IDirect3DVolumeTexture9Impl *This = impl_from_IDirect3DVolumeTexture9(iface);
    D3DTEXTUREFILTERTYPE filter_type;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    filter_type = (D3DTEXTUREFILTERTYPE)wined3d_texture_get_autogen_filter_type(This->wined3d_texture);
    wined3d_mutex_unlock();

    return filter_type;
}

static void WINAPI IDirect3DVolumeTexture9Impl_GenerateMipSubLevels(IDirect3DVolumeTexture9 *iface)
{
    IDirect3DVolumeTexture9Impl *This = impl_from_IDirect3DVolumeTexture9(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_texture_generate_mipmaps(This->wined3d_texture);
    wined3d_mutex_unlock();
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_GetLevelDesc(IDirect3DVolumeTexture9 *iface,
        UINT level, D3DVOLUME_DESC *desc)
{
    IDirect3DVolumeTexture9Impl *texture = impl_from_IDirect3DVolumeTexture9(iface);
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
        desc->Width = wined3d_desc.width;
        desc->Height = wined3d_desc.height;
        desc->Depth = wined3d_desc.depth;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_GetVolumeLevel(IDirect3DVolumeTexture9 *iface,
        UINT level, IDirect3DVolume9 **volume)
{
    IDirect3DVolumeTexture9Impl *texture = impl_from_IDirect3DVolumeTexture9(iface);
    struct wined3d_resource *sub_resource;

    TRACE("iface %p, level %u, volume %p.\n", iface, level, volume);

    wined3d_mutex_lock();
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, level)))
    {
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

    *volume = wined3d_resource_get_parent(sub_resource);
    IDirect3DVolume9_AddRef(*volume);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_LockBox(IDirect3DVolumeTexture9 *iface,
        UINT level, D3DLOCKED_BOX *locked_box, const D3DBOX *box, DWORD flags)
{
    IDirect3DVolumeTexture9Impl *texture = impl_from_IDirect3DVolumeTexture9(iface);
    struct wined3d_resource *sub_resource;
    HRESULT hr;

    TRACE("iface %p, level %u, locked_box %p, box %p, flags %#x.\n",
            iface, level, locked_box, box, flags);

    wined3d_mutex_lock();
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, level)))
        hr = D3DERR_INVALIDCALL;
    else
        hr = IDirect3DVolume9_LockBox((IDirect3DVolume9 *)wined3d_resource_get_parent(sub_resource),
                locked_box, box, flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_UnlockBox(IDirect3DVolumeTexture9 *iface,
        UINT level)
{
    IDirect3DVolumeTexture9Impl *texture = impl_from_IDirect3DVolumeTexture9(iface);
    struct wined3d_resource *sub_resource;
    HRESULT hr;

    TRACE("iface %p, level %u.\n", iface, level);

    wined3d_mutex_lock();
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, level)))
        hr = D3DERR_INVALIDCALL;
    else
        hr = IDirect3DVolume9_UnlockBox((IDirect3DVolume9 *)wined3d_resource_get_parent(sub_resource));
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DVolumeTexture9Impl_AddDirtyBox(IDirect3DVolumeTexture9 *iface,
        const D3DBOX *dirty_box)
{
    IDirect3DVolumeTexture9Impl *texture = impl_from_IDirect3DVolumeTexture9(iface);
    HRESULT hr;

    TRACE("iface %p, dirty_box %p.\n", iface, dirty_box);

    wined3d_mutex_lock();
    hr = wined3d_texture_add_dirty_region(texture->wined3d_texture, 0, (const struct wined3d_box *)dirty_box);
    wined3d_mutex_unlock();

    return hr;
}


static const IDirect3DVolumeTexture9Vtbl Direct3DVolumeTexture9_Vtbl =
{
    /* IUnknown */
    IDirect3DVolumeTexture9Impl_QueryInterface,
    IDirect3DVolumeTexture9Impl_AddRef,
    IDirect3DVolumeTexture9Impl_Release,
    /* IDirect3DResource9 */
    IDirect3DVolumeTexture9Impl_GetDevice,
    IDirect3DVolumeTexture9Impl_SetPrivateData,
    IDirect3DVolumeTexture9Impl_GetPrivateData,
    IDirect3DVolumeTexture9Impl_FreePrivateData,
    IDirect3DVolumeTexture9Impl_SetPriority,
    IDirect3DVolumeTexture9Impl_GetPriority,
    IDirect3DVolumeTexture9Impl_PreLoad,
    IDirect3DVolumeTexture9Impl_GetType,
    /* IDirect3DBaseTexture9 */
    IDirect3DVolumeTexture9Impl_SetLOD,
    IDirect3DVolumeTexture9Impl_GetLOD,
    IDirect3DVolumeTexture9Impl_GetLevelCount,
    IDirect3DVolumeTexture9Impl_SetAutoGenFilterType,
    IDirect3DVolumeTexture9Impl_GetAutoGenFilterType,
    IDirect3DVolumeTexture9Impl_GenerateMipSubLevels,
    /* IDirect3DVolumeTexture9 */
    IDirect3DVolumeTexture9Impl_GetLevelDesc,
    IDirect3DVolumeTexture9Impl_GetVolumeLevel,
    IDirect3DVolumeTexture9Impl_LockBox,
    IDirect3DVolumeTexture9Impl_UnlockBox,
    IDirect3DVolumeTexture9Impl_AddDirtyBox
};

static void STDMETHODCALLTYPE volumetexture_wined3d_object_destroyed(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d9_volumetexture_wined3d_parent_ops =
{
    volumetexture_wined3d_object_destroyed,
};

HRESULT volumetexture_init(IDirect3DVolumeTexture9Impl *texture, IDirect3DDevice9Impl *device,
        UINT width, UINT height, UINT depth, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool)
{
    HRESULT hr;

    texture->IDirect3DVolumeTexture9_iface.lpVtbl = &Direct3DVolumeTexture9_Vtbl;
    texture->ref = 1;

    wined3d_mutex_lock();
    hr = wined3d_texture_create_3d(device->wined3d_device, width, height, depth, levels,
            usage & WINED3DUSAGE_MASK, wined3dformat_from_d3dformat(format), pool, texture,
            &d3d9_volumetexture_wined3d_parent_ops, &texture->wined3d_texture);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d volume texture, hr %#x.\n", hr);
        return hr;
    }

    texture->parentDevice = &device->IDirect3DDevice9Ex_iface;
    IDirect3DDevice9Ex_AddRef(texture->parentDevice);

    return D3D_OK;
}
