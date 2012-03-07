/*
 * IDirect3DSurface8 implementation
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
#include <assert.h>
#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

static inline IDirect3DSurface8Impl *impl_from_IDirect3DSurface8(IDirect3DSurface8 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DSurface8Impl, IDirect3DSurface8_iface);
}

/* IDirect3DSurface8 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DSurface8Impl_QueryInterface(IDirect3DSurface8 *iface, REFIID riid,
        void **ppobj)
{
    IDirect3DSurface8Impl *This = impl_from_IDirect3DSurface8(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource8)
        || IsEqualGUID(riid, &IID_IDirect3DSurface8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DSurface8Impl_AddRef(IDirect3DSurface8 *iface)
{
    IDirect3DSurface8Impl *This = impl_from_IDirect3DSurface8(iface);

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
            if (This->parentDevice) IUnknown_AddRef(This->parentDevice);
            wined3d_mutex_lock();
            wined3d_surface_incref(This->wined3d_surface);
            wined3d_mutex_unlock();
        }

        return ref;
    }
}

static ULONG WINAPI IDirect3DSurface8Impl_Release(IDirect3DSurface8 *iface)
{
    IDirect3DSurface8Impl *This = impl_from_IDirect3DSurface8(iface);

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
            IDirect3DDevice8 *parentDevice = This->parentDevice;

            /* Implicit surfaces are destroyed with the device, not if refcount reaches 0. */
            wined3d_mutex_lock();
            wined3d_surface_decref(This->wined3d_surface);
            wined3d_mutex_unlock();

            if (parentDevice) IDirect3DDevice8_Release(parentDevice);
        }

        return ref;
    }
}

/* IDirect3DSurface8 IDirect3DResource8 Interface follow: */
static HRESULT WINAPI IDirect3DSurface8Impl_GetDevice(IDirect3DSurface8 *iface,
        IDirect3DDevice8 **device)
{
    IDirect3DSurface8Impl *This = impl_from_IDirect3DSurface8(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    if (This->forwardReference)
    {
        IDirect3DResource8 *resource;
        HRESULT hr;

        hr = IUnknown_QueryInterface(This->forwardReference, &IID_IDirect3DResource8, (void **)&resource);
        if (SUCCEEDED(hr))
        {
            hr = IDirect3DResource8_GetDevice(resource, device);
            IDirect3DResource8_Release(resource);

            TRACE("Returning device %p.\n", *device);
        }

        return hr;
    }

    *device = This->parentDevice;
    IDirect3DDevice8_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DSurface8Impl_SetPrivateData(IDirect3DSurface8 *iface, REFGUID guid,
        const void *data, DWORD data_size, DWORD flags)
{
    IDirect3DSurface8Impl *surface = impl_from_IDirect3DSurface8(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %u, flags %#x.\n",
            iface, debugstr_guid(guid), data, data_size, flags);

    wined3d_mutex_lock();
    resource = wined3d_surface_get_resource(surface->wined3d_surface);
    hr = wined3d_resource_set_private_data(resource, guid, data, data_size, flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DSurface8Impl_GetPrivateData(IDirect3DSurface8 *iface, REFGUID guid,
        void *data, DWORD *data_size)
{
    IDirect3DSurface8Impl *surface = impl_from_IDirect3DSurface8(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %p.\n",
            iface, debugstr_guid(guid), data, data_size);

    wined3d_mutex_lock();
    resource = wined3d_surface_get_resource(surface->wined3d_surface);
    hr = wined3d_resource_get_private_data(resource, guid, data, data_size);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DSurface8Impl_FreePrivateData(IDirect3DSurface8 *iface, REFGUID guid)
{
    IDirect3DSurface8Impl *surface = impl_from_IDirect3DSurface8(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    wined3d_mutex_lock();
    resource = wined3d_surface_get_resource(surface->wined3d_surface);
    hr = wined3d_resource_free_private_data(resource, guid);
    wined3d_mutex_unlock();

    return hr;
}

/* IDirect3DSurface8 Interface follow: */
static HRESULT WINAPI IDirect3DSurface8Impl_GetContainer(IDirect3DSurface8 *iface, REFIID riid,
        void **ppContainer)
{
    IDirect3DSurface8Impl *This = impl_from_IDirect3DSurface8(iface);
    HRESULT res;

    TRACE("iface %p, riid %s, container %p.\n", iface, debugstr_guid(riid), ppContainer);

    if (!This->container) return E_NOINTERFACE;

    res = IUnknown_QueryInterface(This->container, riid, ppContainer);

    TRACE("(%p) : returning %p\n", This, *ppContainer);
    return res;
}

static HRESULT WINAPI IDirect3DSurface8Impl_GetDesc(IDirect3DSurface8 *iface, D3DSURFACE_DESC *desc)
{
    IDirect3DSurface8Impl *This = impl_from_IDirect3DSurface8(iface);
    struct wined3d_resource_desc wined3d_desc;
    struct wined3d_resource *wined3d_resource;

    TRACE("iface %p, desc %p.\n", iface, desc);

    wined3d_mutex_lock();
    wined3d_resource = wined3d_surface_get_resource(This->wined3d_surface);
    wined3d_resource_get_desc(wined3d_resource, &wined3d_desc);
    wined3d_mutex_unlock();

    desc->Format = d3dformat_from_wined3dformat(wined3d_desc.format);
    desc->Type = wined3d_desc.resource_type;
    desc->Usage = wined3d_desc.usage & WINED3DUSAGE_MASK;
    desc->Pool = wined3d_desc.pool;
    desc->Size = wined3d_desc.size;
    desc->MultiSampleType = wined3d_desc.multisample_type;
    desc->Width = wined3d_desc.width;
    desc->Height = wined3d_desc.height;

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DSurface8Impl_LockRect(IDirect3DSurface8 *iface,
        D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags)
{
    IDirect3DSurface8Impl *This = impl_from_IDirect3DSurface8(iface);
    HRESULT hr;

    TRACE("iface %p, locked_rect %p, rect %p, flags %#x.\n", iface, pLockedRect, pRect, Flags);

    wined3d_mutex_lock();
    if (pRect) {
        D3DSURFACE_DESC desc;
        IDirect3DSurface8_GetDesc(iface, &desc);

        if ((pRect->left < 0)
                || (pRect->top < 0)
                || (pRect->left >= pRect->right)
                || (pRect->top >= pRect->bottom)
                || (pRect->right > desc.Width)
                || (pRect->bottom > desc.Height)) {
            WARN("Trying to lock an invalid rectangle, returning D3DERR_INVALIDCALL\n");
            wined3d_mutex_unlock();

            return D3DERR_INVALIDCALL;
        }
    }

    hr = wined3d_surface_map(This->wined3d_surface, (struct wined3d_mapped_rect *)pLockedRect, pRect, Flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DSurface8Impl_UnlockRect(IDirect3DSurface8 *iface)
{
    IDirect3DSurface8Impl *This = impl_from_IDirect3DSurface8(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_surface_unmap(This->wined3d_surface);
    wined3d_mutex_unlock();

    switch(hr)
    {
        case WINEDDERR_NOTLOCKED:       return D3DERR_INVALIDCALL;
        default:                        return hr;
    }
}

static const IDirect3DSurface8Vtbl Direct3DSurface8_Vtbl =
{
    /* IUnknown */
    IDirect3DSurface8Impl_QueryInterface,
    IDirect3DSurface8Impl_AddRef,
    IDirect3DSurface8Impl_Release,
    /* IDirect3DResource8 */
    IDirect3DSurface8Impl_GetDevice,
    IDirect3DSurface8Impl_SetPrivateData,
    IDirect3DSurface8Impl_GetPrivateData,
    IDirect3DSurface8Impl_FreePrivateData,
    /* IDirect3DSurface8 */
    IDirect3DSurface8Impl_GetContainer,
    IDirect3DSurface8Impl_GetDesc,
    IDirect3DSurface8Impl_LockRect,
    IDirect3DSurface8Impl_UnlockRect
};

static void STDMETHODCALLTYPE surface_wined3d_object_destroyed(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d8_surface_wined3d_parent_ops =
{
    surface_wined3d_object_destroyed,
};

HRESULT surface_init(IDirect3DSurface8Impl *surface, IDirect3DDevice8Impl *device,
        UINT width, UINT height, D3DFORMAT format, BOOL lockable, BOOL discard, UINT level,
        DWORD usage, D3DPOOL pool, D3DMULTISAMPLE_TYPE multisample_type, DWORD multisample_quality)
{
    DWORD flags = 0;
    HRESULT hr;

    surface->IDirect3DSurface8_iface.lpVtbl = &Direct3DSurface8_Vtbl;
    surface->ref = 1;

    /* FIXME: Check MAX bounds of MultisampleQuality. */
    if (multisample_quality > 0)
    {
        FIXME("Multisample quality set to %u, substituting 0.\n", multisample_quality);
        multisample_quality = 0;
    }

    if (lockable)
        flags |= WINED3D_SURFACE_MAPPABLE;
    if (discard)
        flags |= WINED3D_SURFACE_DISCARD;

    wined3d_mutex_lock();
    hr = wined3d_surface_create(device->wined3d_device, width, height, wined3dformat_from_d3dformat(format),
            level, usage & WINED3DUSAGE_MASK, (enum wined3d_pool)pool, multisample_type, multisample_quality,
            SURFACE_OPENGL, flags, surface, &d3d8_surface_wined3d_parent_ops, &surface->wined3d_surface);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d surface, hr %#x.\n", hr);
        return hr;
    }

    surface->parentDevice = &device->IDirect3DDevice8_iface;
    IUnknown_AddRef(surface->parentDevice);

    return D3D_OK;
}

IDirect3DSurface8Impl *unsafe_impl_from_IDirect3DSurface8(IDirect3DSurface8 *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &Direct3DSurface8_Vtbl);

    return impl_from_IDirect3DSurface8(iface);
}
