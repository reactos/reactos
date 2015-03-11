/*
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

#include "d3d9_private.h"

static inline struct d3d9_texture *impl_from_IDirect3DTexture9(IDirect3DTexture9 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d9_texture, IDirect3DBaseTexture9_iface);
}

static inline struct d3d9_texture *impl_from_IDirect3DCubeTexture9(IDirect3DCubeTexture9 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d9_texture, IDirect3DBaseTexture9_iface);
}

static inline struct d3d9_texture *impl_from_IDirect3DVolumeTexture9(IDirect3DVolumeTexture9 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d9_texture, IDirect3DBaseTexture9_iface);
}

static HRESULT WINAPI d3d9_texture_2d_QueryInterface(IDirect3DTexture9 *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DTexture9)
            || IsEqualGUID(riid, &IID_IDirect3DBaseTexture9)
            || IsEqualGUID(riid, &IID_IDirect3DResource9)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DTexture9_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d9_texture_2d_AddRef(IDirect3DTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);
    ULONG ref = InterlockedIncrement(&texture->resource.refcount);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    if (ref == 1)
    {
        struct d3d9_surface *surface;

        IDirect3DDevice9Ex_AddRef(texture->parent_device);
        wined3d_mutex_lock();
        LIST_FOR_EACH_ENTRY(surface, &texture->rtv_list, struct d3d9_surface, rtv_entry)
        {
            wined3d_rendertarget_view_incref(surface->wined3d_rtv);
        }
        wined3d_texture_incref(texture->wined3d_texture);
        wined3d_mutex_unlock();
    }

    return ref;
}

static ULONG WINAPI d3d9_texture_2d_Release(IDirect3DTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);
    ULONG ref = InterlockedDecrement(&texture->resource.refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (!ref)
    {
        IDirect3DDevice9Ex *parent_device = texture->parent_device;
        struct d3d9_surface *surface;

        wined3d_mutex_lock();
        LIST_FOR_EACH_ENTRY(surface, &texture->rtv_list, struct d3d9_surface, rtv_entry)
        {
            wined3d_rendertarget_view_decref(surface->wined3d_rtv);
        }
        wined3d_texture_decref(texture->wined3d_texture);
        wined3d_mutex_unlock();

        /* Release the device last, as it may cause the device to be destroyed. */
        IDirect3DDevice9Ex_Release(parent_device);
    }
    return ref;
}

static HRESULT WINAPI d3d9_texture_2d_GetDevice(IDirect3DTexture9 *iface, IDirect3DDevice9 **device)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)texture->parent_device;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI d3d9_texture_2d_SetPrivateData(IDirect3DTexture9 *iface,
        REFGUID guid, const void *data, DWORD data_size, DWORD flags)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);
    TRACE("iface %p, guid %s, data %p, data_size %u, flags %#x.\n",
            iface, debugstr_guid(guid), data, data_size, flags);

    return d3d9_resource_set_private_data(&texture->resource, guid, data, data_size, flags);
}

static HRESULT WINAPI d3d9_texture_2d_GetPrivateData(IDirect3DTexture9 *iface,
        REFGUID guid, void *data, DWORD *data_size)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);
    TRACE("iface %p, guid %s, data %p, data_size %p.\n",
            iface, debugstr_guid(guid), data, data_size);

    return d3d9_resource_get_private_data(&texture->resource, guid, data, data_size);
}

static HRESULT WINAPI d3d9_texture_2d_FreePrivateData(IDirect3DTexture9 *iface, REFGUID guid)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);
    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    return d3d9_resource_free_private_data(&texture->resource, guid);
}

static DWORD WINAPI d3d9_texture_2d_SetPriority(IDirect3DTexture9 *iface, DWORD priority)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);
    struct wined3d_resource *resource;
    DWORD ret;

    TRACE("iface %p, priority %u.\n", iface, priority);

    wined3d_mutex_lock();
    resource = wined3d_texture_get_resource(texture->wined3d_texture);
    ret = wined3d_resource_set_priority(resource, priority);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI d3d9_texture_2d_GetPriority(IDirect3DTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);
    const struct wined3d_resource *resource;
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    resource = wined3d_texture_get_resource(texture->wined3d_texture);
    ret = wined3d_resource_get_priority(resource);
    wined3d_mutex_unlock();

    return ret;
}

static void WINAPI d3d9_texture_2d_PreLoad(IDirect3DTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_texture_preload(texture->wined3d_texture);
    wined3d_mutex_unlock();
}

static D3DRESOURCETYPE WINAPI d3d9_texture_2d_GetType(IDirect3DTexture9 *iface)
{
    TRACE("iface %p.\n", iface);

    return D3DRTYPE_TEXTURE;
}

static DWORD WINAPI d3d9_texture_2d_SetLOD(IDirect3DTexture9 *iface, DWORD lod)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);
    DWORD ret;

    TRACE("iface %p, lod %u.\n", iface, lod);

    wined3d_mutex_lock();
    ret = wined3d_texture_set_lod(texture->wined3d_texture, lod);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI d3d9_texture_2d_GetLOD(IDirect3DTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = wined3d_texture_get_lod(texture->wined3d_texture);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI d3d9_texture_2d_GetLevelCount(IDirect3DTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = wined3d_texture_get_level_count(texture->wined3d_texture);
    wined3d_mutex_unlock();

    return ret;
}

static HRESULT WINAPI d3d9_texture_2d_SetAutoGenFilterType(IDirect3DTexture9 *iface, D3DTEXTUREFILTERTYPE filter_type)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);
    HRESULT hr;

    TRACE("iface %p, filter_type %#x.\n", iface, filter_type);

    wined3d_mutex_lock();
    hr = wined3d_texture_set_autogen_filter_type(texture->wined3d_texture,
            (enum wined3d_texture_filter_type)filter_type);
    wined3d_mutex_unlock();

    return hr;
}

static D3DTEXTUREFILTERTYPE WINAPI d3d9_texture_2d_GetAutoGenFilterType(IDirect3DTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);
    D3DTEXTUREFILTERTYPE ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = (D3DTEXTUREFILTERTYPE)wined3d_texture_get_autogen_filter_type(texture->wined3d_texture);
    wined3d_mutex_unlock();

    return ret;
}

static void WINAPI d3d9_texture_2d_GenerateMipSubLevels(IDirect3DTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_texture_generate_mipmaps(texture->wined3d_texture);
    wined3d_mutex_unlock();
}

static HRESULT WINAPI d3d9_texture_2d_GetLevelDesc(IDirect3DTexture9 *iface, UINT level, D3DSURFACE_DESC *desc)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);
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

static HRESULT WINAPI d3d9_texture_2d_GetSurfaceLevel(IDirect3DTexture9 *iface,
        UINT level, IDirect3DSurface9 **surface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);
    struct wined3d_resource *sub_resource;
    struct d3d9_surface *surface_impl;

    TRACE("iface %p, level %u, surface %p.\n", iface, level, surface);

    wined3d_mutex_lock();
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, level)))
    {
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

    surface_impl = wined3d_resource_get_parent(sub_resource);
    *surface = &surface_impl->IDirect3DSurface9_iface;
    IDirect3DSurface9_AddRef(*surface);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_texture_2d_LockRect(IDirect3DTexture9 *iface,
        UINT level, D3DLOCKED_RECT *locked_rect, const RECT *rect, DWORD flags)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);
    struct wined3d_resource *sub_resource;
    struct d3d9_surface *surface_impl;
    HRESULT hr;

    TRACE("iface %p, level %u, locked_rect %p, rect %p, flags %#x.\n",
            iface, level, locked_rect, rect, flags);

    wined3d_mutex_lock();
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, level)))
        hr = D3DERR_INVALIDCALL;
    else
    {
        surface_impl = wined3d_resource_get_parent(sub_resource);
        hr = IDirect3DSurface9_LockRect(&surface_impl->IDirect3DSurface9_iface, locked_rect, rect, flags);
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_texture_2d_UnlockRect(IDirect3DTexture9 *iface, UINT level)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);
    struct wined3d_resource *sub_resource;
    struct d3d9_surface *surface_impl;
    HRESULT hr;

    TRACE("iface %p, level %u.\n", iface, level);

    wined3d_mutex_lock();
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, level)))
        hr = D3DERR_INVALIDCALL;
    else
    {
        surface_impl = wined3d_resource_get_parent(sub_resource);
        hr = IDirect3DSurface9_UnlockRect(&surface_impl->IDirect3DSurface9_iface);
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_texture_2d_AddDirtyRect(IDirect3DTexture9 *iface, const RECT *dirty_rect)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);
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

static const IDirect3DTexture9Vtbl d3d9_texture_2d_vtbl =
{
    /* IUnknown */
    d3d9_texture_2d_QueryInterface,
    d3d9_texture_2d_AddRef,
    d3d9_texture_2d_Release,
     /* IDirect3DResource9 */
    d3d9_texture_2d_GetDevice,
    d3d9_texture_2d_SetPrivateData,
    d3d9_texture_2d_GetPrivateData,
    d3d9_texture_2d_FreePrivateData,
    d3d9_texture_2d_SetPriority,
    d3d9_texture_2d_GetPriority,
    d3d9_texture_2d_PreLoad,
    d3d9_texture_2d_GetType,
    /* IDirect3dBaseTexture9 */
    d3d9_texture_2d_SetLOD,
    d3d9_texture_2d_GetLOD,
    d3d9_texture_2d_GetLevelCount,
    d3d9_texture_2d_SetAutoGenFilterType,
    d3d9_texture_2d_GetAutoGenFilterType,
    d3d9_texture_2d_GenerateMipSubLevels,
    /* IDirect3DTexture9 */
    d3d9_texture_2d_GetLevelDesc,
    d3d9_texture_2d_GetSurfaceLevel,
    d3d9_texture_2d_LockRect,
    d3d9_texture_2d_UnlockRect,
    d3d9_texture_2d_AddDirtyRect,
};

static HRESULT WINAPI d3d9_texture_cube_QueryInterface(IDirect3DCubeTexture9 *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DCubeTexture9)
            || IsEqualGUID(riid, &IID_IDirect3DBaseTexture9)
            || IsEqualGUID(riid, &IID_IDirect3DResource9)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DCubeTexture9_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d9_texture_cube_AddRef(IDirect3DCubeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);
    ULONG ref = InterlockedIncrement(&texture->resource.refcount);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    if (ref == 1)
    {
        struct d3d9_surface *surface;

        IDirect3DDevice9Ex_AddRef(texture->parent_device);
        wined3d_mutex_lock();
        LIST_FOR_EACH_ENTRY(surface, &texture->rtv_list, struct d3d9_surface, rtv_entry)
        {
            wined3d_rendertarget_view_decref(surface->wined3d_rtv);
        }
        wined3d_texture_incref(texture->wined3d_texture);
        wined3d_mutex_unlock();
    }

    return ref;
}

static ULONG WINAPI d3d9_texture_cube_Release(IDirect3DCubeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);
    ULONG ref = InterlockedDecrement(&texture->resource.refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (!ref)
    {
        IDirect3DDevice9Ex *parent_device = texture->parent_device;
        struct d3d9_surface *surface;

        TRACE("Releasing child %p.\n", texture->wined3d_texture);

        wined3d_mutex_lock();
        LIST_FOR_EACH_ENTRY(surface, &texture->rtv_list, struct d3d9_surface, rtv_entry)
        {
            wined3d_rendertarget_view_decref(surface->wined3d_rtv);
        }
        wined3d_texture_decref(texture->wined3d_texture);
        wined3d_mutex_unlock();

        /* Release the device last, as it may cause the device to be destroyed. */
        IDirect3DDevice9Ex_Release(parent_device);
    }
    return ref;
}

static HRESULT WINAPI d3d9_texture_cube_GetDevice(IDirect3DCubeTexture9 *iface, IDirect3DDevice9 **device)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)texture->parent_device;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI d3d9_texture_cube_SetPrivateData(IDirect3DCubeTexture9 *iface,
        REFGUID guid, const void *data, DWORD data_size, DWORD flags)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);
    TRACE("iface %p, guid %s, data %p, data_size %u, flags %#x.\n",
            iface, debugstr_guid(guid), data, data_size, flags);

    return d3d9_resource_set_private_data(&texture->resource, guid, data, data_size, flags);
}

static HRESULT WINAPI d3d9_texture_cube_GetPrivateData(IDirect3DCubeTexture9 *iface,
        REFGUID guid, void *data, DWORD *data_size)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);
    TRACE("iface %p, guid %s, data %p, data_size %p.\n",
            iface, debugstr_guid(guid), data, data_size);

    return d3d9_resource_get_private_data(&texture->resource, guid, data, data_size);
}

static HRESULT WINAPI d3d9_texture_cube_FreePrivateData(IDirect3DCubeTexture9 *iface, REFGUID guid)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);
    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    return d3d9_resource_free_private_data(&texture->resource, guid);
}

static DWORD WINAPI d3d9_texture_cube_SetPriority(IDirect3DCubeTexture9 *iface, DWORD priority)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);
    struct wined3d_resource *resource;
    DWORD ret;

    TRACE("iface %p, priority %u.\n", iface, priority);

    wined3d_mutex_lock();
    resource = wined3d_texture_get_resource(texture->wined3d_texture);
    ret = wined3d_resource_set_priority(resource, priority);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI d3d9_texture_cube_GetPriority(IDirect3DCubeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);
    const struct wined3d_resource *resource;
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    resource = wined3d_texture_get_resource(texture->wined3d_texture);
    ret = wined3d_resource_get_priority(resource);
    wined3d_mutex_unlock();

    return ret;
}

static void WINAPI d3d9_texture_cube_PreLoad(IDirect3DCubeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_texture_preload(texture->wined3d_texture);
    wined3d_mutex_unlock();
}

static D3DRESOURCETYPE WINAPI d3d9_texture_cube_GetType(IDirect3DCubeTexture9 *iface)
{
    TRACE("iface %p.\n", iface);

    return D3DRTYPE_CUBETEXTURE;
}

static DWORD WINAPI d3d9_texture_cube_SetLOD(IDirect3DCubeTexture9 *iface, DWORD lod)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);
    DWORD ret;

    TRACE("iface %p, lod %u.\n", iface, lod);

    wined3d_mutex_lock();
    ret = wined3d_texture_set_lod(texture->wined3d_texture, lod);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI d3d9_texture_cube_GetLOD(IDirect3DCubeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = wined3d_texture_get_lod(texture->wined3d_texture);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI d3d9_texture_cube_GetLevelCount(IDirect3DCubeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = wined3d_texture_get_level_count(texture->wined3d_texture);
    wined3d_mutex_unlock();

    return ret;
}

static HRESULT WINAPI d3d9_texture_cube_SetAutoGenFilterType(IDirect3DCubeTexture9 *iface,
        D3DTEXTUREFILTERTYPE filter_type)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);
    HRESULT hr;

    TRACE("iface %p, filter_type %#x.\n", iface, filter_type);

    wined3d_mutex_lock();
    hr = wined3d_texture_set_autogen_filter_type(texture->wined3d_texture,
            (enum wined3d_texture_filter_type)filter_type);
    wined3d_mutex_unlock();

    return hr;
}

static D3DTEXTUREFILTERTYPE WINAPI d3d9_texture_cube_GetAutoGenFilterType(IDirect3DCubeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);
    D3DTEXTUREFILTERTYPE ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = (D3DTEXTUREFILTERTYPE)wined3d_texture_get_autogen_filter_type(texture->wined3d_texture);
    wined3d_mutex_unlock();

    return ret;
}

static void WINAPI d3d9_texture_cube_GenerateMipSubLevels(IDirect3DCubeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_texture_generate_mipmaps(texture->wined3d_texture);
    wined3d_mutex_unlock();
}

static HRESULT WINAPI d3d9_texture_cube_GetLevelDesc(IDirect3DCubeTexture9 *iface, UINT level, D3DSURFACE_DESC *desc)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);
    struct wined3d_resource *sub_resource;
    HRESULT hr = D3D_OK;
    DWORD level_count;

    TRACE("iface %p, level %u, desc %p.\n", iface, level, desc);

    wined3d_mutex_lock();
    level_count = wined3d_texture_get_level_count(texture->wined3d_texture);
    if (level >= level_count)
    {
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

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

static HRESULT WINAPI d3d9_texture_cube_GetCubeMapSurface(IDirect3DCubeTexture9 *iface,
        D3DCUBEMAP_FACES face, UINT level, IDirect3DSurface9 **surface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);
    struct wined3d_resource *sub_resource;
    struct d3d9_surface *surface_impl;
    UINT sub_resource_idx;
    DWORD level_count;

    TRACE("iface %p, face %#x, level %u, surface %p.\n", iface, face, level, surface);

    wined3d_mutex_lock();
    level_count = wined3d_texture_get_level_count(texture->wined3d_texture);
    if (level >= level_count)
    {
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

    sub_resource_idx = level_count * face + level;
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, sub_resource_idx)))
    {
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

    surface_impl = wined3d_resource_get_parent(sub_resource);
    *surface = &surface_impl->IDirect3DSurface9_iface;
    IDirect3DSurface9_AddRef(*surface);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_texture_cube_LockRect(IDirect3DCubeTexture9 *iface,
        D3DCUBEMAP_FACES face, UINT level, D3DLOCKED_RECT *locked_rect, const RECT *rect,
        DWORD flags)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);
    struct wined3d_resource *sub_resource;
    struct d3d9_surface *surface_impl;
    UINT sub_resource_idx;
    HRESULT hr;

    TRACE("iface %p, face %#x, level %u, locked_rect %p, rect %p, flags %#x.\n",
            iface, face, level, locked_rect, rect, flags);

    wined3d_mutex_lock();
    sub_resource_idx = wined3d_texture_get_level_count(texture->wined3d_texture) * face + level;
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, sub_resource_idx)))
        hr = D3DERR_INVALIDCALL;
    else
    {
        surface_impl = wined3d_resource_get_parent(sub_resource);
        hr = IDirect3DSurface9_LockRect(&surface_impl->IDirect3DSurface9_iface, locked_rect, rect, flags);
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_texture_cube_UnlockRect(IDirect3DCubeTexture9 *iface,
        D3DCUBEMAP_FACES face, UINT level)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);
    struct wined3d_resource *sub_resource;
    struct d3d9_surface *surface_impl;
    UINT sub_resource_idx;
    HRESULT hr;

    TRACE("iface %p, face %#x, level %u.\n", iface, face, level);

    wined3d_mutex_lock();
    sub_resource_idx = wined3d_texture_get_level_count(texture->wined3d_texture) * face + level;
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, sub_resource_idx)))
        hr = D3DERR_INVALIDCALL;
    else
    {
        surface_impl = wined3d_resource_get_parent(sub_resource);
        hr = IDirect3DSurface9_UnlockRect(&surface_impl->IDirect3DSurface9_iface);
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI d3d9_texture_cube_AddDirtyRect(IDirect3DCubeTexture9 *iface,
        D3DCUBEMAP_FACES face, const RECT *dirty_rect)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);
    HRESULT hr;

    TRACE("iface %p, face %#x, dirty_rect %s.\n",
            iface, face, wine_dbgstr_rect(dirty_rect));

    wined3d_mutex_lock();
    if (!dirty_rect)
        hr = wined3d_texture_add_dirty_region(texture->wined3d_texture, face, NULL);
    else
    {
        struct wined3d_box dirty_region;

        dirty_region.left = dirty_rect->left;
        dirty_region.top = dirty_rect->top;
        dirty_region.right = dirty_rect->right;
        dirty_region.bottom = dirty_rect->bottom;
        dirty_region.front = 0;
        dirty_region.back = 1;
        hr = wined3d_texture_add_dirty_region(texture->wined3d_texture, face, &dirty_region);
    }
    wined3d_mutex_unlock();

    return hr;
}

static const IDirect3DCubeTexture9Vtbl d3d9_texture_cube_vtbl =
{
    /* IUnknown */
    d3d9_texture_cube_QueryInterface,
    d3d9_texture_cube_AddRef,
    d3d9_texture_cube_Release,
    /* IDirect3DResource9 */
    d3d9_texture_cube_GetDevice,
    d3d9_texture_cube_SetPrivateData,
    d3d9_texture_cube_GetPrivateData,
    d3d9_texture_cube_FreePrivateData,
    d3d9_texture_cube_SetPriority,
    d3d9_texture_cube_GetPriority,
    d3d9_texture_cube_PreLoad,
    d3d9_texture_cube_GetType,
    /* IDirect3DBaseTexture9 */
    d3d9_texture_cube_SetLOD,
    d3d9_texture_cube_GetLOD,
    d3d9_texture_cube_GetLevelCount,
    d3d9_texture_cube_SetAutoGenFilterType,
    d3d9_texture_cube_GetAutoGenFilterType,
    d3d9_texture_cube_GenerateMipSubLevels,
    /* IDirect3DCubeTexture9 */
    d3d9_texture_cube_GetLevelDesc,
    d3d9_texture_cube_GetCubeMapSurface,
    d3d9_texture_cube_LockRect,
    d3d9_texture_cube_UnlockRect,
    d3d9_texture_cube_AddDirtyRect,
};

static HRESULT WINAPI d3d9_texture_3d_QueryInterface(IDirect3DVolumeTexture9 *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DVolumeTexture9)
            || IsEqualGUID(riid, &IID_IDirect3DBaseTexture9)
            || IsEqualGUID(riid, &IID_IDirect3DResource9)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DVolumeTexture9_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d9_texture_3d_AddRef(IDirect3DVolumeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);
    ULONG ref = InterlockedIncrement(&texture->resource.refcount);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    if (ref == 1)
    {
        IDirect3DDevice9Ex_AddRef(texture->parent_device);
        wined3d_mutex_lock();
        wined3d_texture_incref(texture->wined3d_texture);
        wined3d_mutex_unlock();
    }

    return ref;
}

static ULONG WINAPI d3d9_texture_3d_Release(IDirect3DVolumeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);
    ULONG ref = InterlockedDecrement(&texture->resource.refcount);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (!ref)
    {
        IDirect3DDevice9Ex *parent_device = texture->parent_device;

        wined3d_mutex_lock();
        wined3d_texture_decref(texture->wined3d_texture);
        wined3d_mutex_unlock();

        /* Release the device last, as it may cause the device to be destroyed. */
        IDirect3DDevice9Ex_Release(parent_device);
    }
    return ref;
}

static HRESULT WINAPI d3d9_texture_3d_GetDevice(IDirect3DVolumeTexture9 *iface, IDirect3DDevice9 **device)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)texture->parent_device;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI d3d9_texture_3d_SetPrivateData(IDirect3DVolumeTexture9 *iface,
        REFGUID guid, const void *data, DWORD data_size, DWORD flags)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);
    TRACE("iface %p, guid %s, data %p, data_size %u, flags %#x.\n",
            iface, debugstr_guid(guid), data, data_size, flags);

    return d3d9_resource_set_private_data(&texture->resource, guid, data, data_size, flags);
}

static HRESULT WINAPI d3d9_texture_3d_GetPrivateData(IDirect3DVolumeTexture9 *iface,
        REFGUID guid, void *data, DWORD *data_size)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);
    TRACE("iface %p, guid %s, data %p, data_size %p.\n",
            iface, debugstr_guid(guid), data, data_size);

    return d3d9_resource_get_private_data(&texture->resource, guid, data, data_size);
}

static HRESULT WINAPI d3d9_texture_3d_FreePrivateData(IDirect3DVolumeTexture9 *iface, REFGUID guid)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);
    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    return d3d9_resource_free_private_data(&texture->resource, guid);
}

static DWORD WINAPI d3d9_texture_3d_SetPriority(IDirect3DVolumeTexture9 *iface, DWORD priority)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);
    struct wined3d_resource *resource;
    DWORD ret;

    TRACE("iface %p, priority %u.\n", iface, priority);

    wined3d_mutex_lock();
    resource = wined3d_texture_get_resource(texture->wined3d_texture);
    ret = wined3d_resource_set_priority(resource, priority);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI d3d9_texture_3d_GetPriority(IDirect3DVolumeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);
    const struct wined3d_resource *resource;
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    resource = wined3d_texture_get_resource(texture->wined3d_texture);
    ret = wined3d_resource_get_priority(resource);
    wined3d_mutex_unlock();

    return ret;
}

static void WINAPI d3d9_texture_3d_PreLoad(IDirect3DVolumeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_texture_preload(texture->wined3d_texture);
    wined3d_mutex_unlock();
}

static D3DRESOURCETYPE WINAPI d3d9_texture_3d_GetType(IDirect3DVolumeTexture9 *iface)
{
    TRACE("iface %p.\n", iface);

    return D3DRTYPE_VOLUMETEXTURE;
}

static DWORD WINAPI d3d9_texture_3d_SetLOD(IDirect3DVolumeTexture9 *iface, DWORD lod)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);
    DWORD ret;

    TRACE("iface %p, lod %u.\n", iface, lod);

    wined3d_mutex_lock();
    ret = wined3d_texture_set_lod(texture->wined3d_texture, lod);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI d3d9_texture_3d_GetLOD(IDirect3DVolumeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = wined3d_texture_get_lod(texture->wined3d_texture);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI d3d9_texture_3d_GetLevelCount(IDirect3DVolumeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = wined3d_texture_get_level_count(texture->wined3d_texture);
    wined3d_mutex_unlock();

    return ret;
}

static HRESULT WINAPI d3d9_texture_3d_SetAutoGenFilterType(IDirect3DVolumeTexture9 *iface,
        D3DTEXTUREFILTERTYPE filter_type)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);
    HRESULT hr;

    TRACE("iface %p, filter_type %#x.\n", iface, filter_type);

    wined3d_mutex_lock();
    hr = wined3d_texture_set_autogen_filter_type(texture->wined3d_texture,
            (enum wined3d_texture_filter_type)filter_type);
    wined3d_mutex_unlock();

    return hr;
}

static D3DTEXTUREFILTERTYPE WINAPI d3d9_texture_3d_GetAutoGenFilterType(IDirect3DVolumeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);
    D3DTEXTUREFILTERTYPE filter_type;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    filter_type = (D3DTEXTUREFILTERTYPE)wined3d_texture_get_autogen_filter_type(texture->wined3d_texture);
    wined3d_mutex_unlock();

    return filter_type;
}

static void WINAPI d3d9_texture_3d_GenerateMipSubLevels(IDirect3DVolumeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_texture_generate_mipmaps(texture->wined3d_texture);
    wined3d_mutex_unlock();
}

static HRESULT WINAPI d3d9_texture_3d_GetLevelDesc(IDirect3DVolumeTexture9 *iface, UINT level, D3DVOLUME_DESC *desc)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);
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

static HRESULT WINAPI d3d9_texture_3d_GetVolumeLevel(IDirect3DVolumeTexture9 *iface,
        UINT level, IDirect3DVolume9 **volume)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);
    struct wined3d_resource *sub_resource;
    struct d3d9_volume *volume_impl;

    TRACE("iface %p, level %u, volume %p.\n", iface, level, volume);

    wined3d_mutex_lock();
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, level)))
    {
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

    volume_impl = wined3d_resource_get_parent(sub_resource);
    *volume = &volume_impl->IDirect3DVolume9_iface;
    IDirect3DVolume9_AddRef(*volume);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_texture_3d_LockBox(IDirect3DVolumeTexture9 *iface,
        UINT level, D3DLOCKED_BOX *locked_box, const D3DBOX *box, DWORD flags)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);
    struct wined3d_resource *sub_resource;
    struct d3d9_volume *volume_impl;
    HRESULT hr;

    TRACE("iface %p, level %u, locked_box %p, box %p, flags %#x.\n",
            iface, level, locked_box, box, flags);

    wined3d_mutex_lock();
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, level)))
        hr = D3DERR_INVALIDCALL;
    else
    {
        volume_impl = wined3d_resource_get_parent(sub_resource);
        hr = IDirect3DVolume9_LockBox(&volume_impl->IDirect3DVolume9_iface, locked_box, box, flags);
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_texture_3d_UnlockBox(IDirect3DVolumeTexture9 *iface, UINT level)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);
    struct wined3d_resource *sub_resource;
    struct d3d9_volume *volume_impl;
    HRESULT hr;

    TRACE("iface %p, level %u.\n", iface, level);

    wined3d_mutex_lock();
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture->wined3d_texture, level)))
        hr = D3DERR_INVALIDCALL;
    else
    {
        volume_impl = wined3d_resource_get_parent(sub_resource);
        hr = IDirect3DVolume9_UnlockBox(&volume_impl->IDirect3DVolume9_iface);
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_texture_3d_AddDirtyBox(IDirect3DVolumeTexture9 *iface, const D3DBOX *dirty_box)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);
    HRESULT hr;

    TRACE("iface %p, dirty_box %p.\n", iface, dirty_box);

    wined3d_mutex_lock();
    hr = wined3d_texture_add_dirty_region(texture->wined3d_texture, 0, (const struct wined3d_box *)dirty_box);
    wined3d_mutex_unlock();

    return hr;
}


static const IDirect3DVolumeTexture9Vtbl d3d9_texture_3d_vtbl =
{
    /* IUnknown */
    d3d9_texture_3d_QueryInterface,
    d3d9_texture_3d_AddRef,
    d3d9_texture_3d_Release,
    /* IDirect3DResource9 */
    d3d9_texture_3d_GetDevice,
    d3d9_texture_3d_SetPrivateData,
    d3d9_texture_3d_GetPrivateData,
    d3d9_texture_3d_FreePrivateData,
    d3d9_texture_3d_SetPriority,
    d3d9_texture_3d_GetPriority,
    d3d9_texture_3d_PreLoad,
    d3d9_texture_3d_GetType,
    /* IDirect3DBaseTexture9 */
    d3d9_texture_3d_SetLOD,
    d3d9_texture_3d_GetLOD,
    d3d9_texture_3d_GetLevelCount,
    d3d9_texture_3d_SetAutoGenFilterType,
    d3d9_texture_3d_GetAutoGenFilterType,
    d3d9_texture_3d_GenerateMipSubLevels,
    /* IDirect3DVolumeTexture9 */
    d3d9_texture_3d_GetLevelDesc,
    d3d9_texture_3d_GetVolumeLevel,
    d3d9_texture_3d_LockBox,
    d3d9_texture_3d_UnlockBox,
    d3d9_texture_3d_AddDirtyBox,
};

struct d3d9_texture *unsafe_impl_from_IDirect3DBaseTexture9(IDirect3DBaseTexture9 *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == (const IDirect3DBaseTexture9Vtbl *)&d3d9_texture_2d_vtbl
            || iface->lpVtbl == (const IDirect3DBaseTexture9Vtbl *)&d3d9_texture_cube_vtbl
            || iface->lpVtbl == (const IDirect3DBaseTexture9Vtbl *)&d3d9_texture_3d_vtbl);
    return CONTAINING_RECORD(iface, struct d3d9_texture, IDirect3DBaseTexture9_iface);
}

static void STDMETHODCALLTYPE d3d9_texture_wined3d_object_destroyed(void *parent)
{
    struct d3d9_texture *texture = parent;
    d3d9_resource_cleanup(&texture->resource);
    HeapFree(GetProcessHeap(), 0, texture);
}

static const struct wined3d_parent_ops d3d9_texture_wined3d_parent_ops =
{
    d3d9_texture_wined3d_object_destroyed,
};

HRESULT texture_init(struct d3d9_texture *texture, struct d3d9_device *device,
        UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool)
{
    struct wined3d_resource_desc desc;
    DWORD surface_flags = 0;
    HRESULT hr;

    texture->IDirect3DBaseTexture9_iface.lpVtbl = (const IDirect3DBaseTexture9Vtbl *)&d3d9_texture_2d_vtbl;
    d3d9_resource_init(&texture->resource);
    list_init(&texture->rtv_list);

    desc.resource_type = WINED3D_RTYPE_TEXTURE;
    desc.format = wined3dformat_from_d3dformat(format);
    desc.multisample_type = WINED3D_MULTISAMPLE_NONE;
    desc.multisample_quality = 0;
    desc.usage = usage & WINED3DUSAGE_MASK;
    desc.usage |= WINED3DUSAGE_TEXTURE;
    desc.pool = pool;
    desc.width = width;
    desc.height = height;
    desc.depth = 1;
    desc.size = 0;

    if (pool != D3DPOOL_DEFAULT || (usage & D3DUSAGE_DYNAMIC))
        surface_flags |= WINED3D_SURFACE_MAPPABLE;

    if (!levels)
    {
        if (usage & D3DUSAGE_AUTOGENMIPMAP)
            levels = 1;
        else
            levels = wined3d_log2i(max(width, height)) + 1;
    }

    wined3d_mutex_lock();
    hr = wined3d_texture_create(device->wined3d_device, &desc, levels, surface_flags,
            NULL, texture, &d3d9_texture_wined3d_parent_ops, &texture->wined3d_texture);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d texture, hr %#x.\n", hr);
        return hr;
    }

    texture->parent_device = &device->IDirect3DDevice9Ex_iface;
    IDirect3DDevice9Ex_AddRef(texture->parent_device);

    return D3D_OK;
}

HRESULT cubetexture_init(struct d3d9_texture *texture, struct d3d9_device *device,
        UINT edge_length, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool)
{
    struct wined3d_resource_desc desc;
    DWORD surface_flags = 0;
    HRESULT hr;

    texture->IDirect3DBaseTexture9_iface.lpVtbl = (const IDirect3DBaseTexture9Vtbl *)&d3d9_texture_cube_vtbl;
    d3d9_resource_init(&texture->resource);
    list_init(&texture->rtv_list);

    desc.resource_type = WINED3D_RTYPE_CUBE_TEXTURE;
    desc.format = wined3dformat_from_d3dformat(format);
    desc.multisample_type = WINED3D_MULTISAMPLE_NONE;
    desc.multisample_quality = 0;
    desc.usage = usage & WINED3DUSAGE_MASK;
    desc.usage |= WINED3DUSAGE_TEXTURE;
    desc.pool = pool;
    desc.width = edge_length;
    desc.height = edge_length;
    desc.depth = 1;
    desc.size = 0;

    if (pool != D3DPOOL_DEFAULT || (usage & D3DUSAGE_DYNAMIC))
        surface_flags |= WINED3D_SURFACE_MAPPABLE;

    if (!levels)
    {
        if (usage & D3DUSAGE_AUTOGENMIPMAP)
            levels = 1;
        else
            levels = wined3d_log2i(edge_length) + 1;
    }

    wined3d_mutex_lock();
    hr = wined3d_texture_create(device->wined3d_device, &desc, levels, surface_flags,
            NULL, texture, &d3d9_texture_wined3d_parent_ops, &texture->wined3d_texture);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d cube texture, hr %#x.\n", hr);
        return hr;
    }

    texture->parent_device = &device->IDirect3DDevice9Ex_iface;
    IDirect3DDevice9Ex_AddRef(texture->parent_device);

    return D3D_OK;
}

HRESULT volumetexture_init(struct d3d9_texture *texture, struct d3d9_device *device,
        UINT width, UINT height, UINT depth, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool)
{
    struct wined3d_resource_desc desc;
    HRESULT hr;

    texture->IDirect3DBaseTexture9_iface.lpVtbl = (const IDirect3DBaseTexture9Vtbl *)&d3d9_texture_3d_vtbl;
    d3d9_resource_init(&texture->resource);
    list_init(&texture->rtv_list);

    desc.resource_type = WINED3D_RTYPE_VOLUME_TEXTURE;
    desc.format = wined3dformat_from_d3dformat(format);
    desc.multisample_type = WINED3D_MULTISAMPLE_NONE;
    desc.multisample_quality = 0;
    desc.usage = usage & WINED3DUSAGE_MASK;
    desc.usage |= WINED3DUSAGE_TEXTURE;
    desc.pool = pool;
    desc.width = width;
    desc.height = height;
    desc.depth = depth;
    desc.size = 0;

    if (!levels)
    {
        if (usage & D3DUSAGE_AUTOGENMIPMAP)
            levels = 1;
        else
            levels = wined3d_log2i(max(max(width, height), depth)) + 1;
    }

    wined3d_mutex_lock();
    hr = wined3d_texture_create(device->wined3d_device, &desc, levels, 0,
            NULL, texture, &d3d9_texture_wined3d_parent_ops, &texture->wined3d_texture);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d volume texture, hr %#x.\n", hr);
        return hr;
    }

    texture->parent_device = &device->IDirect3DDevice9Ex_iface;
    IDirect3DDevice9Ex_AddRef(texture->parent_device);

    return D3D_OK;
}
