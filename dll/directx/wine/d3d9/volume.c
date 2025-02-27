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

#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

static inline struct d3d9_volume *impl_from_IDirect3DVolume9(IDirect3DVolume9 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d9_volume, IDirect3DVolume9_iface);
}

static HRESULT WINAPI d3d9_volume_QueryInterface(IDirect3DVolume9 *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DVolume9)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DVolume9_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d9_volume_AddRef(IDirect3DVolume9 *iface)
{
    struct d3d9_volume *volume = impl_from_IDirect3DVolume9(iface);

    TRACE("iface %p.\n", iface);
    TRACE("Forwarding to %p.\n", volume->texture);

    return IDirect3DBaseTexture9_AddRef(&volume->texture->IDirect3DBaseTexture9_iface);
}

static ULONG WINAPI d3d9_volume_Release(IDirect3DVolume9 *iface)
{
    struct d3d9_volume *volume = impl_from_IDirect3DVolume9(iface);

    TRACE("iface %p.\n", iface);
    TRACE("Forwarding to %p.\n", volume->texture);

    return IDirect3DBaseTexture9_Release(&volume->texture->IDirect3DBaseTexture9_iface);
}

static HRESULT WINAPI d3d9_volume_GetDevice(IDirect3DVolume9 *iface, IDirect3DDevice9 **device)
{
    struct d3d9_volume *volume = impl_from_IDirect3DVolume9(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    return IDirect3DBaseTexture9_GetDevice(&volume->texture->IDirect3DBaseTexture9_iface, device);
}

static HRESULT WINAPI d3d9_volume_SetPrivateData(IDirect3DVolume9 *iface, REFGUID guid,
        const void *data, DWORD data_size, DWORD flags)
{
    struct d3d9_volume *volume = impl_from_IDirect3DVolume9(iface);
    TRACE("iface %p, guid %s, data %p, data_size %lu, flags %#lx.\n",
            iface, debugstr_guid(guid), data, data_size, flags);

    return d3d9_resource_set_private_data(&volume->resource, guid, data, data_size, flags);
}

static HRESULT WINAPI d3d9_volume_GetPrivateData(IDirect3DVolume9 *iface, REFGUID guid,
        void *data, DWORD *data_size)
{
    struct d3d9_volume *volume = impl_from_IDirect3DVolume9(iface);
    TRACE("iface %p, guid %s, data %p, data_size %p.\n",
            iface, debugstr_guid(guid), data, data_size);

    return d3d9_resource_get_private_data(&volume->resource, guid, data, data_size);
}

static HRESULT WINAPI d3d9_volume_FreePrivateData(IDirect3DVolume9 *iface, REFGUID guid)
{
    struct d3d9_volume *volume = impl_from_IDirect3DVolume9(iface);
    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    return d3d9_resource_free_private_data(&volume->resource, guid);
}

static HRESULT WINAPI d3d9_volume_GetContainer(IDirect3DVolume9 *iface, REFIID riid, void **container)
{
    struct d3d9_volume *volume = impl_from_IDirect3DVolume9(iface);

    TRACE("iface %p, riid %s, container %p.\n", iface, debugstr_guid(riid), container);

    return IDirect3DBaseTexture9_QueryInterface(&volume->texture->IDirect3DBaseTexture9_iface, riid, container);
}

static HRESULT WINAPI d3d9_volume_GetDesc(IDirect3DVolume9 *iface, D3DVOLUME_DESC *desc)
{
    struct d3d9_volume *volume = impl_from_IDirect3DVolume9(iface);
    struct wined3d_sub_resource_desc wined3d_desc;

    TRACE("iface %p, desc %p.\n", iface, desc);

    wined3d_mutex_lock();
    wined3d_texture_get_sub_resource_desc(volume->wined3d_texture, volume->sub_resource_idx, &wined3d_desc);
    wined3d_mutex_unlock();

    desc->Format = d3dformat_from_wined3dformat(wined3d_desc.format);
    desc->Type = D3DRTYPE_VOLUME;
    desc->Usage = d3dusage_from_wined3dusage(wined3d_desc.usage, wined3d_desc.bind_flags);
    desc->Pool = d3dpool_from_wined3daccess(wined3d_desc.access, wined3d_desc.usage);
    desc->Width = wined3d_desc.width;
    desc->Height = wined3d_desc.height;
    desc->Depth = wined3d_desc.depth;

    return D3D_OK;
}

static HRESULT WINAPI d3d9_volume_LockBox(IDirect3DVolume9 *iface,
        D3DLOCKED_BOX *locked_box, const D3DBOX *box, DWORD flags)
{
    struct d3d9_volume *volume = impl_from_IDirect3DVolume9(iface);
    struct wined3d_map_desc map_desc;
    HRESULT hr;

    TRACE("iface %p, locked_box %p, box %p, flags %#lx.\n",
            iface, locked_box, box, flags);

    if (FAILED(hr = wined3d_resource_map(wined3d_texture_get_resource(volume->wined3d_texture),
            volume->sub_resource_idx, &map_desc, (const struct wined3d_box *)box,
            wined3dmapflags_from_d3dmapflags(flags, 0))))
        map_desc.data = NULL;

    locked_box->RowPitch = map_desc.row_pitch;
    locked_box->SlicePitch = map_desc.slice_pitch;
    locked_box->pBits = map_desc.data;

    if (hr == E_INVALIDARG)
        return D3DERR_INVALIDCALL;
    return hr;
}

static HRESULT WINAPI d3d9_volume_UnlockBox(IDirect3DVolume9 *iface)
{
    struct d3d9_volume *volume = impl_from_IDirect3DVolume9(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    hr = wined3d_resource_unmap(wined3d_texture_get_resource(volume->wined3d_texture), volume->sub_resource_idx);

    if (hr == WINEDDERR_NOTLOCKED)
        return D3DERR_INVALIDCALL;
    return hr;
}

static const struct IDirect3DVolume9Vtbl d3d9_volume_vtbl =
{
    /* IUnknown */
    d3d9_volume_QueryInterface,
    d3d9_volume_AddRef,
    d3d9_volume_Release,
    /* IDirect3DVolume9 */
    d3d9_volume_GetDevice,
    d3d9_volume_SetPrivateData,
    d3d9_volume_GetPrivateData,
    d3d9_volume_FreePrivateData,
    d3d9_volume_GetContainer,
    d3d9_volume_GetDesc,
    d3d9_volume_LockBox,
    d3d9_volume_UnlockBox,
};

static void STDMETHODCALLTYPE volume_wined3d_object_destroyed(void *parent)
{
    struct d3d9_volume *volume = parent;
    d3d9_resource_cleanup(&volume->resource);
    free(volume);
}

static const struct wined3d_parent_ops d3d9_volume_wined3d_parent_ops =
{
    volume_wined3d_object_destroyed,
};

void volume_init(struct d3d9_volume *volume, struct wined3d_texture *wined3d_texture,
        unsigned int sub_resource_idx, const struct wined3d_parent_ops **parent_ops)
{
    volume->IDirect3DVolume9_iface.lpVtbl = &d3d9_volume_vtbl;
    d3d9_resource_init(&volume->resource);
    volume->resource.refcount = 0;
    volume->texture = wined3d_texture_get_parent(wined3d_texture);
    volume->wined3d_texture = wined3d_texture;
    volume->sub_resource_idx = sub_resource_idx;

    *parent_ops = &d3d9_volume_wined3d_parent_ops;
}
