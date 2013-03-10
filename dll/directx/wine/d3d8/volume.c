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

#include <config.h>
#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

static inline struct d3d8_volume *impl_from_IDirect3DVolume8(IDirect3DVolume8 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d8_volume, IDirect3DVolume8_iface);
}

static HRESULT WINAPI d3d8_volume_QueryInterface(IDirect3DVolume8 *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DVolume8)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DVolume8_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d8_volume_AddRef(IDirect3DVolume8 *iface)
{
    struct d3d8_volume *volume = impl_from_IDirect3DVolume8(iface);

    TRACE("iface %p.\n", iface);

    if (volume->forwardReference)
    {
        /* Forward to the containerParent */
        TRACE("Forwarding to %p,\n", volume->forwardReference);
        return IUnknown_AddRef(volume->forwardReference);
    }
    else
    {
        /* No container, handle our own refcounting */
        ULONG ref = InterlockedIncrement(&volume->refcount);

        TRACE("%p increasing refcount to %u.\n", iface, ref);

        if (ref == 1)
        {
            wined3d_mutex_lock();
            wined3d_volume_incref(volume->wined3d_volume);
            wined3d_mutex_unlock();
        }

        return ref;
    }
}

static ULONG WINAPI d3d8_volume_Release(IDirect3DVolume8 *iface)
{
    struct d3d8_volume *volume = impl_from_IDirect3DVolume8(iface);

    TRACE("iface %p.\n", iface);

    if (volume->forwardReference)
    {
        /* Forward to the containerParent */
        TRACE("Forwarding to %p.\n", volume->forwardReference);
        return IUnknown_Release(volume->forwardReference);
    }
    else
    {
        /* No container, handle our own refcounting */
        ULONG ref = InterlockedDecrement(&volume->refcount);

        TRACE("%p decreasing refcount to %u.\n", iface, ref);

        if (!ref)
        {
            wined3d_mutex_lock();
            wined3d_volume_decref(volume->wined3d_volume);
            wined3d_mutex_unlock();
        }

        return ref;
    }
}

static HRESULT WINAPI d3d8_volume_GetDevice(IDirect3DVolume8 *iface, IDirect3DDevice8 **device)
{
    struct d3d8_volume *volume = impl_from_IDirect3DVolume8(iface);
    IDirect3DResource8 *resource;
    HRESULT hr;

    TRACE("iface %p, device %p.\n", iface, device);

    hr = IUnknown_QueryInterface(volume->forwardReference, &IID_IDirect3DResource8, (void **)&resource);
    if (SUCCEEDED(hr))
    {
        hr = IDirect3DResource8_GetDevice(resource, device);
        IDirect3DResource8_Release(resource);

        TRACE("Returning device %p.\n", *device);
    }

    return hr;
}

static HRESULT WINAPI d3d8_volume_SetPrivateData(IDirect3DVolume8 *iface, REFGUID guid,
        const void *data, DWORD data_size, DWORD flags)
{
    struct d3d8_volume *volume = impl_from_IDirect3DVolume8(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %u, flags %#x.\n",
            iface, debugstr_guid(guid), data, data_size, flags);

    wined3d_mutex_lock();
    resource = wined3d_volume_get_resource(volume->wined3d_volume);
    hr = wined3d_resource_set_private_data(resource, guid, data, data_size, flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_volume_GetPrivateData(IDirect3DVolume8 *iface, REFGUID  guid,
        void *data, DWORD *data_size)
{
    struct d3d8_volume *volume = impl_from_IDirect3DVolume8(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %p.\n",
            iface, debugstr_guid(guid), data, data_size);

    wined3d_mutex_lock();
    resource = wined3d_volume_get_resource(volume->wined3d_volume);
    hr = wined3d_resource_get_private_data(resource, guid, data, data_size);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_volume_FreePrivateData(IDirect3DVolume8 *iface, REFGUID guid)
{
    struct d3d8_volume *volume = impl_from_IDirect3DVolume8(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    wined3d_mutex_lock();
    resource = wined3d_volume_get_resource(volume->wined3d_volume);
    hr = wined3d_resource_free_private_data(resource, guid);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d8_volume_GetContainer(IDirect3DVolume8 *iface, REFIID riid, void **container)
{
    struct d3d8_volume *volume = impl_from_IDirect3DVolume8(iface);
    HRESULT res;

    TRACE("iface %p, riid %s, container %p.\n",
            iface, debugstr_guid(riid), container);

    if (!volume->container)
        return E_NOINTERFACE;

    res = IUnknown_QueryInterface(volume->container, riid, container);

    TRACE("Returning %p.\n", *container);

    return res;
}

static HRESULT WINAPI d3d8_volume_GetDesc(IDirect3DVolume8 *iface, D3DVOLUME_DESC *desc)
{
    struct d3d8_volume *volume = impl_from_IDirect3DVolume8(iface);
    struct wined3d_resource_desc wined3d_desc;
    struct wined3d_resource *wined3d_resource;

    TRACE("iface %p, desc %p.\n", iface, desc);

    wined3d_mutex_lock();
    wined3d_resource = wined3d_volume_get_resource(volume->wined3d_volume);
    wined3d_resource_get_desc(wined3d_resource, &wined3d_desc);
    wined3d_mutex_unlock();

    desc->Format = d3dformat_from_wined3dformat(wined3d_desc.format);
    desc->Type = wined3d_desc.resource_type;
    desc->Usage = wined3d_desc.usage & WINED3DUSAGE_MASK;
    desc->Pool = wined3d_desc.pool;
    desc->Size = wined3d_desc.size;
    desc->Width = wined3d_desc.width;
    desc->Height = wined3d_desc.height;
    desc->Depth = wined3d_desc.depth;

    return D3D_OK;
}

static HRESULT WINAPI d3d8_volume_LockBox(IDirect3DVolume8 *iface,
        D3DLOCKED_BOX *locked_box, const D3DBOX *box, DWORD flags)
{
    struct d3d8_volume *volume = impl_from_IDirect3DVolume8(iface);
    struct wined3d_map_desc map_desc;
    HRESULT hr;

    TRACE("iface %p, locked_box %p, box %p, flags %#x.\n",
            iface, locked_box, box, flags);

    wined3d_mutex_lock();
    hr = wined3d_volume_map(volume->wined3d_volume, &map_desc, (const struct wined3d_box *)box, flags);
    wined3d_mutex_unlock();

    locked_box->RowPitch = map_desc.row_pitch;
    locked_box->SlicePitch = map_desc.slice_pitch;
    locked_box->pBits = map_desc.data;

    return hr;
}

static HRESULT WINAPI d3d8_volume_UnlockBox(IDirect3DVolume8 *iface)
{
    struct d3d8_volume *volume = impl_from_IDirect3DVolume8(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_volume_unmap(volume->wined3d_volume);
    wined3d_mutex_unlock();

    return hr;
}

static const IDirect3DVolume8Vtbl d3d8_volume_vtbl =
{
    /* IUnknown */
    d3d8_volume_QueryInterface,
    d3d8_volume_AddRef,
    d3d8_volume_Release,
    /* IDirect3DVolume8 */
    d3d8_volume_GetDevice,
    d3d8_volume_SetPrivateData,
    d3d8_volume_GetPrivateData,
    d3d8_volume_FreePrivateData,
    d3d8_volume_GetContainer,
    d3d8_volume_GetDesc,
    d3d8_volume_LockBox,
    d3d8_volume_UnlockBox,
};

static void STDMETHODCALLTYPE volume_wined3d_object_destroyed(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d8_volume_wined3d_parent_ops =
{
    volume_wined3d_object_destroyed,
};

HRESULT volume_init(struct d3d8_volume *volume, struct d3d8_device *device, UINT width, UINT height,
        UINT depth, DWORD usage, enum wined3d_format_id format, enum wined3d_pool pool)
{
    HRESULT hr;

    volume->IDirect3DVolume8_iface.lpVtbl = &d3d8_volume_vtbl;
    volume->refcount = 1;

    hr = wined3d_volume_create(device->wined3d_device, width, height, depth, usage,
            format, pool, volume, &d3d8_volume_wined3d_parent_ops, &volume->wined3d_volume);
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d volume, hr %#x.\n", hr);
        return hr;
    }

    return D3D_OK;
}
