/*
 * IDirect3DTexture9 implementation
 *
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2005 Raphael Junqueira
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

static inline IDirect3DTexture9Impl *impl_from_IDirect3DTexture9(IDirect3DTexture9 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DTexture9Impl, IDirect3DTexture9_iface);
}

static HRESULT WINAPI IDirect3DTexture9Impl_QueryInterface(IDirect3DTexture9 *iface, REFIID riid,
        void **ppobj)
{
    IDirect3DTexture9Impl *This = impl_from_IDirect3DTexture9(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource9)
        || IsEqualGUID(riid, &IID_IDirect3DBaseTexture9)
        || IsEqualGUID(riid, &IID_IDirect3DTexture9)) {
        IDirect3DTexture9_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p) not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DTexture9Impl_AddRef(IDirect3DTexture9 *iface)
{
    IDirect3DTexture9Impl *This = impl_from_IDirect3DTexture9(iface);
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

static ULONG WINAPI IDirect3DTexture9Impl_Release(IDirect3DTexture9 *iface)
{
    IDirect3DTexture9Impl *This = impl_from_IDirect3DTexture9(iface);
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

static HRESULT WINAPI IDirect3DTexture9Impl_GetDevice(IDirect3DTexture9 *iface,
        IDirect3DDevice9 **device)
{
    IDirect3DTexture9Impl *This = impl_from_IDirect3DTexture9(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)This->parentDevice;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DTexture9Impl_SetPrivateData(IDirect3DTexture9 *iface,
        REFGUID refguid, const void *pData, DWORD SizeOfData, DWORD Flags)
{
    IDirect3DTexture9Impl *This = impl_from_IDirect3DTexture9(iface);
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

static HRESULT WINAPI IDirect3DTexture9Impl_GetPrivateData(IDirect3DTexture9 *iface,
        REFGUID refguid, void *pData, DWORD *pSizeOfData)
{
    IDirect3DTexture9Impl *This = impl_from_IDirect3DTexture9(iface);
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

static HRESULT WINAPI IDirect3DTexture9Impl_FreePrivateData(IDirect3DTexture9 *iface,
        REFGUID refguid)
{
    IDirect3DTexture9Impl *This = impl_from_IDirect3DTexture9(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(refguid));

    wined3d_mutex_lock();
    resource = wined3d_texture_get_resource(This->wined3d_texture);
    hr = wined3d_resource_free_private_data(resource, refguid);
    wined3d_mutex_unlock();

    return hr;
}

static DWORD WINAPI IDirect3DTexture9Impl_SetPriority(IDirect3DTexture9 *iface, DWORD PriorityNew)
{
    IDirect3DTexture9Impl *This = impl_from_IDirect3DTexture9(iface);
    DWORD ret;

    TRACE("iface %p, priority %u.\n", iface, PriorityNew);

    wined3d_mutex_lock();
    ret = wined3d_texture_set_priority(This->wined3d_texture, PriorityNew);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI IDirect3DTexture9Impl_GetPriority(IDirect3DTexture9 *iface)
{
    IDirect3DTexture9Impl *This = impl_from_IDirect3DTexture9(iface);
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = wined3d_texture_get_priority(This->wined3d_texture);
    wined3d_mutex_unlock();

    return ret;
}

static void WINAPI IDirect3DTexture9Impl_PreLoad(IDirect3DTexture9 *iface)
{
    IDirect3DTexture9Impl *This = impl_from_IDirect3DTexture9(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_texture_preload(This->wined3d_texture);
    wined3d_mutex_unlock();
}

static D3DRESOURCETYPE WINAPI IDirect3DTexture9Impl_GetType(IDirect3DTexture9 *iface)
{
    TRACE("iface %p.\n", iface);

    return D3DRTYPE_TEXTURE;
}

/* IDirect3DTexture9 IDirect3DBaseTexture9 Interface follow: */
static DWORD WINAPI IDirect3DTexture9Impl_SetLOD(IDirect3DTexture9 *iface, DWORD LODNew)
{
    IDirect3DTexture9Impl *This = impl_from_IDirect3DTexture9(iface);
    DWORD ret;

    TRACE("iface %p, lod %u.\n", iface, LODNew);

    wined3d_mutex_lock();
    ret = wined3d_texture_set_lod(This->wined3d_texture, LODNew);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI IDirect3DTexture9Impl_GetLOD(IDirect3DTexture9 *iface)
{
    IDirect3DTexture9Impl *This = impl_from_IDirect3DTexture9(iface);
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = wined3d_texture_get_lod(This->wined3d_texture);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI IDirect3DTexture9Impl_GetLevelCount(IDirect3DTexture9 *iface)
{
    IDirect3DTexture9Impl *This = impl_from_IDirect3DTexture9(iface);
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = wined3d_texture_get_level_count(This->wined3d_texture);
    wined3d_mutex_unlock();

    return ret;
}

static HRESULT WINAPI IDirect3DTexture9Impl_SetAutoGenFilterType(IDirect3DTexture9 *iface,
        D3DTEXTUREFILTERTYPE FilterType)
{
    IDirect3DTexture9Impl *This = impl_from_IDirect3DTexture9(iface);
    HRESULT hr;

    TRACE("iface %p, filter_type %#x.\n", iface, FilterType);

    wined3d_mutex_lock();
    hr = wined3d_texture_set_autogen_filter_type(This->wined3d_texture, (enum wined3d_texture_filter_type)FilterType);
    wined3d_mutex_unlock();

    return hr;
}

static D3DTEXTUREFILTERTYPE WINAPI IDirect3DTexture9Impl_GetAutoGenFilterType(IDirect3DTexture9 *iface)
{
    IDirect3DTexture9Impl *This = impl_from_IDirect3DTexture9(iface);
    D3DTEXTUREFILTERTYPE ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = (D3DTEXTUREFILTERTYPE)wined3d_texture_get_autogen_filter_type(This->wined3d_texture);
    wined3d_mutex_unlock();

    return ret;
}

static void WINAPI IDirect3DTexture9Impl_GenerateMipSubLevels(IDirect3DTexture9 *iface)
{
    IDirect3DTexture9Impl *This = impl_from_IDirect3DTexture9(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_texture_generate_mipmaps(This->wined3d_texture);
    wined3d_mutex_unlock();
}

static HRESULT WINAPI IDirect3DTexture9Impl_GetLevelDesc(IDirect3DTexture9 *iface,
        UINT level, D3DSURFACE_DESC *desc)
{
    IDirect3DTexture9Impl *texture = impl_from_IDirect3DTexture9(iface);
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
        desc->MultiSampleType = wined3d_desc.multisample_type;
        desc->MultiSampleQuality = wined3d_desc.multisample_quality;
        desc->Width = wined3d_desc.width;
        desc->Height = wined3d_desc.height;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DTexture9Impl_GetSurfaceLevel(IDirect3DTexture9 *iface,
        UINT level, IDirect3DSurface9 **surface)
{
    IDirect3DTexture9Impl *texture = impl_from_IDirect3DTexture9(iface);
    struct wined3d_resource *sub_resource;

    TRACE("iface %p, level %u, surface %p.\n", iface, level, surface);

    wined3d_mutex_lock();
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, level)))
    {
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

    *surface = wined3d_resource_get_parent(sub_resource);
    IDirect3DSurface9_AddRef(*surface);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DTexture9Impl_LockRect(IDirect3DTexture9 *iface,
        UINT level, D3DLOCKED_RECT *locked_rect, const RECT *rect, DWORD flags)
{
    IDirect3DTexture9Impl *texture = impl_from_IDirect3DTexture9(iface);
    struct wined3d_resource *sub_resource;
    HRESULT hr;

    TRACE("iface %p, level %u, locked_rect %p, rect %p, flags %#x.\n",
            iface, level, locked_rect, rect, flags);

    wined3d_mutex_lock();
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, level)))
        hr = D3DERR_INVALIDCALL;
    else
        hr = IDirect3DSurface9_LockRect((IDirect3DSurface9 *)wined3d_resource_get_parent(sub_resource),
                locked_rect, rect, flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DTexture9Impl_UnlockRect(IDirect3DTexture9 *iface, UINT level)
{
    IDirect3DTexture9Impl *texture = impl_from_IDirect3DTexture9(iface);
    struct wined3d_resource *sub_resource;
    HRESULT hr;

    TRACE("iface %p, level %u.\n", iface, level);

    wined3d_mutex_lock();
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, level)))
        hr = D3DERR_INVALIDCALL;
    else
        hr = IDirect3DSurface9_UnlockRect((IDirect3DSurface9 *)wined3d_resource_get_parent(sub_resource));
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DTexture9Impl_AddDirtyRect(IDirect3DTexture9 *iface,
        const RECT *dirty_rect)
{
    IDirect3DTexture9Impl *texture = impl_from_IDirect3DTexture9(iface);
    HRESULT hr;

    TRACE("iface %p, dirty_rect %s.\n",
            iface, wine_dbgstr_rect(dirty_rect));

    wined3d_mutex_lock();
    if (!dirty_rect)
        hr = wined3d_texture_add_dirty_region(texture->wined3d_texture, 0, NULL);
    else
    {
        struct wined3d_box dirty_region;

        dirty_region.left = dirty_rect->left;
        dirty_region.top = dirty_rect->top;
        dirty_region.right = dirty_rect->right;
        dirty_region.bottom = dirty_rect->bottom;
        dirty_region.front = 0;
        dirty_region.back = 1;
        hr = wined3d_texture_add_dirty_region(texture->wined3d_texture, 0, &dirty_region);
    }
    wined3d_mutex_unlock();

    return hr;
}

static const IDirect3DTexture9Vtbl Direct3DTexture9_Vtbl =
{
    /* IUnknown */
    IDirect3DTexture9Impl_QueryInterface,
    IDirect3DTexture9Impl_AddRef,
    IDirect3DTexture9Impl_Release,
     /* IDirect3DResource9 */
    IDirect3DTexture9Impl_GetDevice,
    IDirect3DTexture9Impl_SetPrivateData,
    IDirect3DTexture9Impl_GetPrivateData,
    IDirect3DTexture9Impl_FreePrivateData,
    IDirect3DTexture9Impl_SetPriority,
    IDirect3DTexture9Impl_GetPriority,
    IDirect3DTexture9Impl_PreLoad,
    IDirect3DTexture9Impl_GetType,
    /* IDirect3dBaseTexture9 */
    IDirect3DTexture9Impl_SetLOD,
    IDirect3DTexture9Impl_GetLOD,
    IDirect3DTexture9Impl_GetLevelCount,
    IDirect3DTexture9Impl_SetAutoGenFilterType,
    IDirect3DTexture9Impl_GetAutoGenFilterType,
    IDirect3DTexture9Impl_GenerateMipSubLevels,
    /* IDirect3DTexture9 */
    IDirect3DTexture9Impl_GetLevelDesc,
    IDirect3DTexture9Impl_GetSurfaceLevel,
    IDirect3DTexture9Impl_LockRect,
    IDirect3DTexture9Impl_UnlockRect,
    IDirect3DTexture9Impl_AddDirtyRect
};

static void STDMETHODCALLTYPE d3d9_texture_wined3d_object_destroyed(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d9_texture_wined3d_parent_ops =
{
    d3d9_texture_wined3d_object_destroyed,
};

HRESULT texture_init(IDirect3DTexture9Impl *texture, IDirect3DDevice9Impl *device,
        UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool)
{
    HRESULT hr;

    texture->IDirect3DTexture9_iface.lpVtbl = &Direct3DTexture9_Vtbl;
    texture->ref = 1;

    wined3d_mutex_lock();
    hr = wined3d_texture_create_2d(device->wined3d_device, width, height, levels,
            usage & WINED3DUSAGE_MASK, wined3dformat_from_d3dformat(format), pool,
            texture, &d3d9_texture_wined3d_parent_ops, &texture->wined3d_texture);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d texture, hr %#x.\n", hr);
        return hr;
    }

    texture->parentDevice = &device->IDirect3DDevice9Ex_iface;
    IDirect3DDevice9Ex_AddRef(texture->parentDevice);

    return D3D_OK;
}
