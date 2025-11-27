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

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

static ULONG d3d9_texture_incref(struct d3d9_texture *texture)
{
    ULONG ref = InterlockedIncrement(&texture->resource.refcount);

    TRACE("%p increasing refcount to %lu.\n", texture, ref);

    if (ref == 1)
    {
        struct d3d9_surface *surface;

        IDirect3DDevice9Ex_AddRef(&texture->parent_device->IDirect3DDevice9Ex_iface);
        wined3d_mutex_lock();
        LIST_FOR_EACH_ENTRY(surface, &texture->rtv_list, struct d3d9_surface, rtv_entry)
        {
            wined3d_rendertarget_view_incref(surface->wined3d_rtv);
        }
        wined3d_texture_incref(d3d9_texture_get_draw_texture(texture));
        wined3d_mutex_unlock();
    }

    return ref;
}

static ULONG d3d9_texture_decref(struct d3d9_texture *texture)
{
    ULONG ref = InterlockedDecrement(&texture->resource.refcount);

    TRACE("%p decreasing refcount to %lu.\n", texture, ref);

    if (!ref)
    {
        IDirect3DDevice9Ex *parent_device = &texture->parent_device->IDirect3DDevice9Ex_iface;
        struct d3d9_surface *surface;

        wined3d_mutex_lock();
        LIST_FOR_EACH_ENTRY(surface, &texture->rtv_list, struct d3d9_surface, rtv_entry)
            wined3d_rendertarget_view_decref(surface->wined3d_rtv);
        wined3d_texture_decref(d3d9_texture_get_draw_texture(texture));
        wined3d_mutex_unlock();

        /* Release the device last, as it may cause the device to be destroyed. */
        IDirect3DDevice9Ex_Release(parent_device);
    }
    return ref;
}

static void d3d9_texture_preload(struct d3d9_texture *texture)
{
    wined3d_mutex_lock();
    wined3d_resource_preload(wined3d_texture_get_resource(d3d9_texture_get_draw_texture(texture)));
    wined3d_mutex_unlock();
}

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

/* wined3d critical section must be taken by the caller. */
void d3d9_texture_gen_auto_mipmap(struct d3d9_texture *texture)
{
    if (!(texture->flags & D3D9_TEXTURE_MIPMAP_DIRTY))
        return;
    if (texture->draw_texture)
        wined3d_device_update_texture(texture->parent_device->wined3d_device,
                texture->wined3d_texture, texture->draw_texture);
    wined3d_device_context_generate_mipmaps(texture->parent_device->immediate_context,
            wined3d_texture_acquire_identity_srv(d3d9_texture_get_draw_texture(texture)));
    texture->flags &= ~D3D9_TEXTURE_MIPMAP_DIRTY;
}

void d3d9_texture_flag_auto_gen_mipmap(struct d3d9_texture *texture)
{
    if (texture->usage & D3DUSAGE_AUTOGENMIPMAP)
        texture->flags |= D3D9_TEXTURE_MIPMAP_DIRTY;
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

    return d3d9_texture_incref(texture);
}

static ULONG WINAPI d3d9_texture_2d_Release(IDirect3DTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);

    return d3d9_texture_decref(texture);
}

static HRESULT WINAPI d3d9_texture_2d_GetDevice(IDirect3DTexture9 *iface, IDirect3DDevice9 **device)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)&texture->parent_device->IDirect3DDevice9Ex_iface;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI d3d9_texture_2d_SetPrivateData(IDirect3DTexture9 *iface,
        REFGUID guid, const void *data, DWORD data_size, DWORD flags)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);
    TRACE("iface %p, guid %s, data %p, data_size %lu, flags %#lx.\n",
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

    TRACE("iface %p, priority %lu.\n", iface, priority);

    wined3d_mutex_lock();
    resource = wined3d_texture_get_resource(d3d9_texture_get_draw_texture(texture));
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
    resource = wined3d_texture_get_resource(d3d9_texture_get_draw_texture(texture));
    ret = wined3d_resource_get_priority(resource);
    wined3d_mutex_unlock();

    return ret;
}

static void WINAPI d3d9_texture_2d_PreLoad(IDirect3DTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);

    TRACE("iface %p.\n", iface);

    d3d9_texture_preload(texture);
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

    TRACE("iface %p, lod %lu.\n", iface, lod);

    wined3d_mutex_lock();
    ret = wined3d_stateblock_set_texture_lod(texture->parent_device->state,
            d3d9_texture_get_draw_texture(texture), lod);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI d3d9_texture_2d_GetLOD(IDirect3DTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = wined3d_texture_get_lod(d3d9_texture_get_draw_texture(texture));
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI d3d9_texture_2d_GetLevelCount(IDirect3DTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);
    DWORD ret;

    TRACE("iface %p.\n", iface);

    if (texture->usage & D3DUSAGE_AUTOGENMIPMAP)
        return 1;

    wined3d_mutex_lock();
    ret = wined3d_texture_get_level_count(texture->wined3d_texture);
    wined3d_mutex_unlock();

    return ret;
}

static HRESULT WINAPI d3d9_texture_2d_SetAutoGenFilterType(IDirect3DTexture9 *iface, D3DTEXTUREFILTERTYPE filter_type)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);

    TRACE("iface %p, filter_type %#x.\n", iface, filter_type);

    if (filter_type == D3DTEXF_NONE)
    {
        WARN("Invalid filter type D3DTEXF_NONE specified.\n");
        return D3DERR_INVALIDCALL;
    }
    if (!(texture->usage & D3DUSAGE_AUTOGENMIPMAP))
        WARN("Called on a texture without the D3DUSAGE_AUTOGENMIPMAP flag.\n");
    else if (filter_type != D3DTEXF_LINEAR)
        FIXME("Unsupported filter type %u.\n", filter_type);

    texture->autogen_filter_type = filter_type;
    return D3D_OK;
}

static D3DTEXTUREFILTERTYPE WINAPI d3d9_texture_2d_GetAutoGenFilterType(IDirect3DTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);

    TRACE("iface %p.\n", iface);

    if (!(texture->usage & D3DUSAGE_AUTOGENMIPMAP))
        WARN("Called on a texture without the D3DUSAGE_AUTOGENMIPMAP flag.\n");

    return texture->autogen_filter_type;
}

static void WINAPI d3d9_texture_2d_GenerateMipSubLevels(IDirect3DTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    d3d9_texture_gen_auto_mipmap(texture);
    wined3d_mutex_unlock();
}

static HRESULT WINAPI d3d9_texture_2d_GetLevelDesc(IDirect3DTexture9 *iface, UINT level, D3DSURFACE_DESC *desc)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);
    struct wined3d_sub_resource_desc wined3d_desc;
    HRESULT hr;

    TRACE("iface %p, level %u, desc %p.\n", iface, level, desc);

    if (texture->usage & D3DUSAGE_AUTOGENMIPMAP && level)
    {
        WARN("D3DUSAGE_AUTOGENMIPMAP textures have only one accessible level.\n");
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    if (SUCCEEDED(hr = wined3d_texture_get_sub_resource_desc(texture->wined3d_texture, level, &wined3d_desc)))
    {
        desc->Format = d3dformat_from_wined3dformat(wined3d_desc.format);
        desc->Type = D3DRTYPE_SURFACE;
        desc->Usage = texture->usage;
        desc->Pool = d3dpool_from_wined3daccess(wined3d_desc.access, wined3d_desc.usage);
        desc->MultiSampleType = d3dmultisample_type_from_wined3d(wined3d_desc.multisample_type);
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
    struct d3d9_surface *surface_impl;

    TRACE("iface %p, level %u, surface %p.\n", iface, level, surface);

    if (texture->usage & D3DUSAGE_AUTOGENMIPMAP && level)
    {
        WARN("D3DUSAGE_AUTOGENMIPMAP textures have only one accessible level.\n");
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    if (!(surface_impl = wined3d_texture_get_sub_resource_parent(texture->wined3d_texture, level)))
    {
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

    *surface = &surface_impl->IDirect3DSurface9_iface;
    IDirect3DSurface9_AddRef(*surface);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_texture_2d_LockRect(IDirect3DTexture9 *iface,
        UINT level, D3DLOCKED_RECT *locked_rect, const RECT *rect, DWORD flags)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);
    struct d3d9_surface *surface_impl;
    HRESULT hr;

    TRACE("iface %p, level %u, locked_rect %p, rect %p, flags %#lx.\n",
            iface, level, locked_rect, rect, flags);

    if (texture->usage & D3DUSAGE_AUTOGENMIPMAP && level)
    {
        WARN("D3DUSAGE_AUTOGENMIPMAP textures have only one accessible level.\n");
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    if (!(surface_impl = wined3d_texture_get_sub_resource_parent(texture->wined3d_texture, level)))
        hr = D3DERR_INVALIDCALL;
    else
        hr = IDirect3DSurface9_LockRect(&surface_impl->IDirect3DSurface9_iface, locked_rect, rect, flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_texture_2d_UnlockRect(IDirect3DTexture9 *iface, UINT level)
{
    struct d3d9_texture *texture = impl_from_IDirect3DTexture9(iface);
    struct d3d9_surface *surface_impl;
    HRESULT hr;

    TRACE("iface %p, level %u.\n", iface, level);

    if (texture->usage & D3DUSAGE_AUTOGENMIPMAP && level)
    {
        WARN("D3DUSAGE_AUTOGENMIPMAP textures have only one accessible level.\n");
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    if (!(surface_impl = wined3d_texture_get_sub_resource_parent(texture->wined3d_texture, level)))
        hr = D3DERR_INVALIDCALL;
    else
        hr = IDirect3DSurface9_UnlockRect(&surface_impl->IDirect3DSurface9_iface);
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

        wined3d_box_set(&dirty_region, dirty_rect->left, dirty_rect->top, dirty_rect->right, dirty_rect->bottom, 0, 1);
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

    return d3d9_texture_incref(texture);
}

static ULONG WINAPI d3d9_texture_cube_Release(IDirect3DCubeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);

    return d3d9_texture_decref(texture);
}

static HRESULT WINAPI d3d9_texture_cube_GetDevice(IDirect3DCubeTexture9 *iface, IDirect3DDevice9 **device)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)&texture->parent_device->IDirect3DDevice9Ex_iface;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI d3d9_texture_cube_SetPrivateData(IDirect3DCubeTexture9 *iface,
        REFGUID guid, const void *data, DWORD data_size, DWORD flags)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);
    TRACE("iface %p, guid %s, data %p, data_size %lu, flags %#lx.\n",
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

    TRACE("iface %p, priority %lu.\n", iface, priority);

    wined3d_mutex_lock();
    resource = wined3d_texture_get_resource(d3d9_texture_get_draw_texture(texture));
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
    resource = wined3d_texture_get_resource(d3d9_texture_get_draw_texture(texture));
    ret = wined3d_resource_get_priority(resource);
    wined3d_mutex_unlock();

    return ret;
}

static void WINAPI d3d9_texture_cube_PreLoad(IDirect3DCubeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);

    TRACE("iface %p.\n", iface);

    d3d9_texture_preload(texture);
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

    TRACE("iface %p, lod %lu.\n", iface, lod);

    wined3d_mutex_lock();
    ret = wined3d_stateblock_set_texture_lod(texture->parent_device->state,
            d3d9_texture_get_draw_texture(texture), lod);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI d3d9_texture_cube_GetLOD(IDirect3DCubeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = wined3d_texture_get_lod(d3d9_texture_get_draw_texture(texture));
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI d3d9_texture_cube_GetLevelCount(IDirect3DCubeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);
    DWORD ret;

    TRACE("iface %p.\n", iface);

    if (texture->usage & D3DUSAGE_AUTOGENMIPMAP)
        return 1;

    wined3d_mutex_lock();
    ret = wined3d_texture_get_level_count(texture->wined3d_texture);
    wined3d_mutex_unlock();

    return ret;
}

static HRESULT WINAPI d3d9_texture_cube_SetAutoGenFilterType(IDirect3DCubeTexture9 *iface,
        D3DTEXTUREFILTERTYPE filter_type)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);

    TRACE("iface %p, filter_type %#x.\n", iface, filter_type);

    if (filter_type == D3DTEXF_NONE)
    {
        WARN("Invalid filter type D3DTEXF_NONE specified.\n");
        return D3DERR_INVALIDCALL;
    }
    if (!(texture->usage & D3DUSAGE_AUTOGENMIPMAP))
        WARN("Called on a texture without the D3DUSAGE_AUTOGENMIPMAP flag.\n");
    else if (filter_type != D3DTEXF_LINEAR)
        FIXME("Unsupported filter type %u.\n", filter_type);

    texture->autogen_filter_type = filter_type;
    return D3D_OK;
}

static D3DTEXTUREFILTERTYPE WINAPI d3d9_texture_cube_GetAutoGenFilterType(IDirect3DCubeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);

    TRACE("iface %p.\n", iface);

    if (!(texture->usage & D3DUSAGE_AUTOGENMIPMAP))
        WARN("Called on a texture without the D3DUSAGE_AUTOGENMIPMAP flag.\n");

    return texture->autogen_filter_type;
}

static void WINAPI d3d9_texture_cube_GenerateMipSubLevels(IDirect3DCubeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    d3d9_texture_gen_auto_mipmap(texture);
    wined3d_mutex_unlock();
}

static HRESULT WINAPI d3d9_texture_cube_GetLevelDesc(IDirect3DCubeTexture9 *iface, UINT level, D3DSURFACE_DESC *desc)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);
    struct wined3d_sub_resource_desc wined3d_desc;
    DWORD level_count;
    HRESULT hr;

    TRACE("iface %p, level %u, desc %p.\n", iface, level, desc);

    if (texture->usage & D3DUSAGE_AUTOGENMIPMAP && level)
    {
        WARN("D3DUSAGE_AUTOGENMIPMAP textures have only one accessible level.\n");
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    level_count = wined3d_texture_get_level_count(texture->wined3d_texture);
    if (level >= level_count)
    {
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

    if (SUCCEEDED(hr = wined3d_texture_get_sub_resource_desc(texture->wined3d_texture, level, &wined3d_desc)))
    {
        desc->Format = d3dformat_from_wined3dformat(wined3d_desc.format);
        desc->Type = D3DRTYPE_SURFACE;
        desc->Usage = texture->usage;
        desc->Pool = d3dpool_from_wined3daccess(wined3d_desc.access, wined3d_desc.usage);
        desc->MultiSampleType = d3dmultisample_type_from_wined3d(wined3d_desc.multisample_type);
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
    struct d3d9_surface *surface_impl;
    UINT sub_resource_idx;
    DWORD level_count;

    TRACE("iface %p, face %#x, level %u, surface %p.\n", iface, face, level, surface);

    if (texture->usage & D3DUSAGE_AUTOGENMIPMAP && level)
    {
        WARN("D3DUSAGE_AUTOGENMIPMAP textures have only one accessible level.\n");
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    level_count = wined3d_texture_get_level_count(texture->wined3d_texture);
    if (level >= level_count)
    {
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

    sub_resource_idx = level_count * face + level;
    if (!(surface_impl = wined3d_texture_get_sub_resource_parent(texture->wined3d_texture, sub_resource_idx)))
    {
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

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
    struct d3d9_surface *surface_impl;
    UINT sub_resource_idx;
    HRESULT hr;

    TRACE("iface %p, face %#x, level %u, locked_rect %p, rect %p, flags %#lx.\n",
            iface, face, level, locked_rect, rect, flags);

    if (texture->usage & D3DUSAGE_AUTOGENMIPMAP && level)
    {
        WARN("D3DUSAGE_AUTOGENMIPMAP textures have only one accessible level.\n");
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    sub_resource_idx = wined3d_texture_get_level_count(texture->wined3d_texture) * face + level;
    if (!(surface_impl = wined3d_texture_get_sub_resource_parent(texture->wined3d_texture, sub_resource_idx)))
        hr = D3DERR_INVALIDCALL;
    else
        hr = IDirect3DSurface9_LockRect(&surface_impl->IDirect3DSurface9_iface, locked_rect, rect, flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_texture_cube_UnlockRect(IDirect3DCubeTexture9 *iface,
        D3DCUBEMAP_FACES face, UINT level)
{
    struct d3d9_texture *texture = impl_from_IDirect3DCubeTexture9(iface);
    struct d3d9_surface *surface_impl;
    UINT sub_resource_idx;
    HRESULT hr;

    TRACE("iface %p, face %#x, level %u.\n", iface, face, level);

    if (texture->usage & D3DUSAGE_AUTOGENMIPMAP && level)
    {
        WARN("D3DUSAGE_AUTOGENMIPMAP textures have only one accessible level.\n");
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    sub_resource_idx = wined3d_texture_get_level_count(texture->wined3d_texture) * face + level;
    if (!(surface_impl = wined3d_texture_get_sub_resource_parent(texture->wined3d_texture, sub_resource_idx)))
        hr = D3DERR_INVALIDCALL;
    else
        hr = IDirect3DSurface9_UnlockRect(&surface_impl->IDirect3DSurface9_iface);
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

        wined3d_box_set(&dirty_region, dirty_rect->left, dirty_rect->top, dirty_rect->right, dirty_rect->bottom, 0, 1);
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

    return d3d9_texture_incref(texture);
}

static ULONG WINAPI d3d9_texture_3d_Release(IDirect3DVolumeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);

    return d3d9_texture_decref(texture);
}

static HRESULT WINAPI d3d9_texture_3d_GetDevice(IDirect3DVolumeTexture9 *iface, IDirect3DDevice9 **device)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)&texture->parent_device->IDirect3DDevice9Ex_iface;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI d3d9_texture_3d_SetPrivateData(IDirect3DVolumeTexture9 *iface,
        REFGUID guid, const void *data, DWORD data_size, DWORD flags)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);
    TRACE("iface %p, guid %s, data %p, data_size %lu, flags %#lx.\n",
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

    TRACE("iface %p, priority %lu.\n", iface, priority);

    wined3d_mutex_lock();
    resource = wined3d_texture_get_resource(d3d9_texture_get_draw_texture(texture));
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
    resource = wined3d_texture_get_resource(d3d9_texture_get_draw_texture(texture));
    ret = wined3d_resource_get_priority(resource);
    wined3d_mutex_unlock();

    return ret;
}

static void WINAPI d3d9_texture_3d_PreLoad(IDirect3DVolumeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);

    TRACE("iface %p.\n", iface);

    d3d9_texture_preload(texture);
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

    TRACE("iface %p, lod %lu.\n", iface, lod);

    wined3d_mutex_lock();
    ret = wined3d_stateblock_set_texture_lod(texture->parent_device->state,
            d3d9_texture_get_draw_texture(texture), lod);
    wined3d_mutex_unlock();

    return ret;
}

static DWORD WINAPI d3d9_texture_3d_GetLOD(IDirect3DVolumeTexture9 *iface)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);
    DWORD ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = wined3d_texture_get_lod(d3d9_texture_get_draw_texture(texture));
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
    TRACE("iface %p, filter_type %#x.\n", iface, filter_type);

    return D3DERR_INVALIDCALL;
}

static D3DTEXTUREFILTERTYPE WINAPI d3d9_texture_3d_GetAutoGenFilterType(IDirect3DVolumeTexture9 *iface)
{
    TRACE("iface %p.\n", iface);

    return D3DTEXF_NONE;
}

static void WINAPI d3d9_texture_3d_GenerateMipSubLevels(IDirect3DVolumeTexture9 *iface)
{
    TRACE("iface %p.\n", iface);
}

static HRESULT WINAPI d3d9_texture_3d_GetLevelDesc(IDirect3DVolumeTexture9 *iface, UINT level, D3DVOLUME_DESC *desc)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);
    struct wined3d_sub_resource_desc wined3d_desc;
    HRESULT hr;

    TRACE("iface %p, level %u, desc %p.\n", iface, level, desc);

    wined3d_mutex_lock();
    if (SUCCEEDED(hr = wined3d_texture_get_sub_resource_desc(texture->wined3d_texture, level, &wined3d_desc)))
    {
        desc->Format = d3dformat_from_wined3dformat(wined3d_desc.format);
        desc->Type = D3DRTYPE_VOLUME;
        desc->Usage = texture->usage;
        desc->Pool = d3dpool_from_wined3daccess(wined3d_desc.access, wined3d_desc.usage);
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
    struct d3d9_volume *volume_impl;

    TRACE("iface %p, level %u, volume %p.\n", iface, level, volume);

    wined3d_mutex_lock();
    if (!(volume_impl = wined3d_texture_get_sub_resource_parent(texture->wined3d_texture, level)))
    {
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

    *volume = &volume_impl->IDirect3DVolume9_iface;
    IDirect3DVolume9_AddRef(*volume);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d9_texture_3d_LockBox(IDirect3DVolumeTexture9 *iface,
        UINT level, D3DLOCKED_BOX *locked_box, const D3DBOX *box, DWORD flags)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);
    struct d3d9_volume *volume_impl;
    HRESULT hr;

    TRACE("iface %p, level %u, locked_box %p, box %p, flags %#lx.\n",
            iface, level, locked_box, box, flags);

    wined3d_mutex_lock();
    if (!(volume_impl = wined3d_texture_get_sub_resource_parent(texture->wined3d_texture, level)))
        hr = D3DERR_INVALIDCALL;
    else
        hr = IDirect3DVolume9_LockBox(&volume_impl->IDirect3DVolume9_iface, locked_box, box, flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d9_texture_3d_UnlockBox(IDirect3DVolumeTexture9 *iface, UINT level)
{
    struct d3d9_texture *texture = impl_from_IDirect3DVolumeTexture9(iface);
    struct d3d9_volume *volume_impl;
    HRESULT hr;

    TRACE("iface %p, level %u.\n", iface, level);

    wined3d_mutex_lock();
    if (!(volume_impl = wined3d_texture_get_sub_resource_parent(texture->wined3d_texture, level)))
        hr = D3DERR_INVALIDCALL;
    else
        hr = IDirect3DVolume9_UnlockBox(&volume_impl->IDirect3DVolume9_iface);
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

    if (iface->lpVtbl != (const IDirect3DBaseTexture9Vtbl *)&d3d9_texture_2d_vtbl
            && iface->lpVtbl != (const IDirect3DBaseTexture9Vtbl *)&d3d9_texture_cube_vtbl
            && iface->lpVtbl != (const IDirect3DBaseTexture9Vtbl *)&d3d9_texture_3d_vtbl)
    {
        WARN("%p is not a valid IDirect3DBaseTexture9 interface.\n", iface);
        return NULL;
    }

    return CONTAINING_RECORD(iface, struct d3d9_texture, IDirect3DBaseTexture9_iface);
}

static void STDMETHODCALLTYPE d3d9_texture_wined3d_object_destroyed(void *parent)
{
    struct d3d9_texture *texture = parent;

    /* If the texture was managed, release the sysmem copy now. */
    if (texture->draw_texture)
        wined3d_texture_decref(texture->wined3d_texture);
    d3d9_resource_cleanup(&texture->resource);
    free(texture);
}

static const struct wined3d_parent_ops d3d9_texture_wined3d_parent_ops =
{
    d3d9_texture_wined3d_object_destroyed,
};

static HRESULT d3d9_texture_init(struct d3d9_texture *texture, struct d3d9_device *device,
        const struct wined3d_resource_desc *desc, D3DPOOL pool, DWORD usage,
        unsigned int layer_count, unsigned int level_count)
{
    struct wined3d_adapter *wined3d_adapter;
    unsigned int output_idx;
    unsigned int flags = 0;
    HRESULT hr;

    if (pool == D3DPOOL_MANAGED && device->d3d_parent->extended)
    {
        WARN("Managed resources are not supported by d3d9ex devices.\n");
        return D3DERR_INVALIDCALL;
    }

    d3d9_resource_init(&texture->resource);
    list_init(&texture->rtv_list);
    texture->usage = usage;

    if (desc->resource_type == WINED3D_RTYPE_TEXTURE_2D && is_gdi_compat_wined3dformat(desc->format))
        flags |= WINED3D_TEXTURE_CREATE_GET_DC;
    if (pool == D3DPOOL_SYSTEMMEM || pool == D3DPOOL_MANAGED)
        flags |= WINED3D_TEXTURE_CREATE_RECORD_DIRTY_REGIONS;

    if (usage & D3DUSAGE_WRITEONLY)
    {
        WARN("Texture can't be created with the D3DUSAGE_WRITEONLY flags, returning D3DERR_INVALIDCALL.\n");
        return D3DERR_INVALIDCALL;
    }
    if (usage & D3DUSAGE_AUTOGENMIPMAP)
    {
        if (desc->resource_type == WINED3D_RTYPE_TEXTURE_3D)
        {
            WARN("Texture can't be created with the D3DUSAGE_WRITEONLY flags, returning D3DERR_INVALIDCALL.\n");
            return D3DERR_INVALIDCALL;
        }

        if (pool == D3DPOOL_SYSTEMMEM)
        {
            WARN("D3DUSAGE_AUTOGENMIPMAP texture can't be in D3DPOOL_SYSTEMMEM, returning D3DERR_INVALIDCALL.\n");
            return D3DERR_INVALIDCALL;
        }

        if (level_count && level_count != 1)
        {
            WARN("D3DUSAGE_AUTOGENMIPMAP texture with %u level_count, returning D3DERR_INVALIDCALL.\n", level_count);
            return D3DERR_INVALIDCALL;
        }
        wined3d_mutex_lock();
        output_idx = device->adapter_ordinal;
        wined3d_adapter = wined3d_output_get_adapter(device->d3d_parent->wined3d_outputs[output_idx]);
        hr = wined3d_check_device_format(device->d3d_parent->wined3d, wined3d_adapter,
                WINED3D_DEVICE_TYPE_HAL, WINED3DFMT_B8G8R8A8_UNORM, WINED3DUSAGE_QUERY_GENMIPMAP,
                WINED3D_BIND_SHADER_RESOURCE, WINED3D_RTYPE_TEXTURE_2D, desc->format);
        wined3d_mutex_unlock();
        if (hr == D3D_OK)
        {
            flags |= WINED3D_TEXTURE_CREATE_GENERATE_MIPMAPS;
            level_count = 0;
        }
        else
        {
            WARN("Mipmap autogeneration is not supported on wined3d format %#x, "
                    "creating a texture with a single level.\n", desc->format);
            level_count = 1;
        }
        texture->autogen_filter_type = D3DTEXF_LINEAR;
    }
    else
    {
        texture->autogen_filter_type = D3DTEXF_NONE;
    }
    if (!level_count)
        level_count = wined3d_log2i(max(max(desc->width, desc->height), desc->depth)) + 1;

    wined3d_mutex_lock();
    if (pool == D3DPOOL_MANAGED)
    {
        struct wined3d_resource_desc managed_desc = *desc;

        managed_desc.access = WINED3D_RESOURCE_ACCESS_CPU | WINED3D_RESOURCE_ACCESS_MAP_R | WINED3D_RESOURCE_ACCESS_MAP_W;
        managed_desc.bind_flags = 0;
        if (FAILED(hr = wined3d_texture_create(device->wined3d_device, &managed_desc, layer_count, level_count, flags,
                NULL, texture, &d3d9_null_wined3d_parent_ops, &texture->wined3d_texture)))
        {
            wined3d_mutex_unlock();
            WARN("Failed to create sysmem texture, hr %#lx.\n", hr);
            return hr;
        }

        managed_desc.access = WINED3D_RESOURCE_ACCESS_GPU;
        managed_desc.bind_flags = desc->bind_flags;
        flags &= WINED3D_TEXTURE_CREATE_GENERATE_MIPMAPS;
        if (FAILED(hr = wined3d_texture_create(device->wined3d_device, &managed_desc, layer_count, level_count, flags,
                NULL, texture, &d3d9_texture_wined3d_parent_ops, &texture->draw_texture)))
        {
            wined3d_texture_decref(texture->wined3d_texture);
            wined3d_mutex_unlock();
            WARN("Failed to create draw texture, hr %#lx.\n", hr);
            return hr;
        }
    }
    else
    {
        if (FAILED(hr = wined3d_texture_create(device->wined3d_device, desc, layer_count, level_count, flags,
                NULL, texture, &d3d9_texture_wined3d_parent_ops, &texture->wined3d_texture)))
        {
            wined3d_mutex_unlock();
            WARN("Failed to create wined3d texture, hr %#lx.\n", hr);
            return hr;
        }
    }
    wined3d_mutex_unlock();

    texture->parent_device = device;
    IDirect3DDevice9Ex_AddRef(&texture->parent_device->IDirect3DDevice9Ex_iface);

    return D3D_OK;
}

HRESULT d3d9_texture_2d_init(struct d3d9_texture *texture, struct d3d9_device *device, unsigned int width,
        unsigned int height, unsigned int level_count, DWORD usage, D3DFORMAT format, D3DPOOL pool)
{
    struct wined3d_resource_desc desc;

    desc.resource_type = WINED3D_RTYPE_TEXTURE_2D;
    desc.format = wined3dformat_from_d3dformat(format);
    desc.multisample_type = WINED3D_MULTISAMPLE_NONE;
    desc.multisample_quality = 0;
    desc.usage = wined3d_usage_from_d3d(pool, usage);
    desc.bind_flags = wined3d_bind_flags_from_d3d9_usage(usage) | WINED3D_BIND_SHADER_RESOURCE;
    desc.access = wined3daccess_from_d3dpool(pool, usage);
    desc.width = width;
    desc.height = height;
    desc.depth = 1;
    desc.size = 0;

    if (!device->d3d_parent->extended)
        desc.usage |= WINED3DUSAGE_VIDMEM_ACCOUNTING;

    texture->IDirect3DBaseTexture9_iface.lpVtbl = (const IDirect3DBaseTexture9Vtbl *)&d3d9_texture_2d_vtbl;
    return d3d9_texture_init(texture, device, &desc, pool, usage, 1, level_count);
}

HRESULT d3d9_texture_cube_init(struct d3d9_texture *texture, struct d3d9_device *device,
        unsigned int edge_length, unsigned int level_count, DWORD usage, D3DFORMAT format, D3DPOOL pool)
{
    struct wined3d_resource_desc desc;

    desc.resource_type = WINED3D_RTYPE_TEXTURE_2D;
    desc.format = wined3dformat_from_d3dformat(format);
    desc.multisample_type = WINED3D_MULTISAMPLE_NONE;
    desc.multisample_quality = 0;
    desc.usage = wined3d_usage_from_d3d(pool, usage) | WINED3DUSAGE_LEGACY_CUBEMAP;
    desc.bind_flags = wined3d_bind_flags_from_d3d9_usage(usage) | WINED3D_BIND_SHADER_RESOURCE;
    desc.access = wined3daccess_from_d3dpool(pool, usage);
    desc.width = edge_length;
    desc.height = edge_length;
    desc.depth = 1;
    desc.size = 0;

    if (!device->d3d_parent->extended)
        desc.usage |= WINED3DUSAGE_VIDMEM_ACCOUNTING;

    texture->IDirect3DBaseTexture9_iface.lpVtbl = (const IDirect3DBaseTexture9Vtbl *)&d3d9_texture_cube_vtbl;
    return d3d9_texture_init(texture, device, &desc, pool, usage, 6, level_count);
}

HRESULT d3d9_texture_3d_init(struct d3d9_texture *texture, struct d3d9_device *device, unsigned int width,
        unsigned int height, unsigned int depth, unsigned int level_count, DWORD usage, D3DFORMAT format, D3DPOOL pool)
{
    struct wined3d_resource_desc desc;

    /* In d3d9, 3D textures can't be used as rendertarget or depth/stencil buffer. */
    if (usage & (D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL))
        return D3DERR_INVALIDCALL;

    desc.resource_type = WINED3D_RTYPE_TEXTURE_3D;
    desc.format = wined3dformat_from_d3dformat(format);
    desc.multisample_type = WINED3D_MULTISAMPLE_NONE;
    desc.multisample_quality = 0;
    desc.usage = wined3d_usage_from_d3d(pool, usage);
    desc.bind_flags = wined3d_bind_flags_from_d3d9_usage(usage) | WINED3D_BIND_SHADER_RESOURCE;
    desc.access = wined3daccess_from_d3dpool(pool, usage);
    desc.width = width;
    desc.height = height;
    desc.depth = depth;
    desc.size = 0;

    if (!device->d3d_parent->extended)
        desc.usage |= WINED3DUSAGE_VIDMEM_ACCOUNTING;

    texture->IDirect3DBaseTexture9_iface.lpVtbl = (const IDirect3DBaseTexture9Vtbl *)&d3d9_texture_3d_vtbl;
    return d3d9_texture_init(texture, device, &desc, pool, usage, 1, level_count);
}
