/*
 * IDirect3DSurface9 implementation
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
#include <assert.h>
#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

static inline IDirect3DSurface9Impl *impl_from_IDirect3DSurface9(IDirect3DSurface9 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DSurface9Impl, IDirect3DSurface9_iface);
}

/* IDirect3DSurface9 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DSurface9Impl_QueryInterface(IDirect3DSurface9 *iface, REFIID riid,
        void **ppobj)
{
    IDirect3DSurface9Impl *This = impl_from_IDirect3DSurface9(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DResource9)
        || IsEqualGUID(riid, &IID_IDirect3DSurface9)) {
        IDirect3DSurface9_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DSurface9Impl_AddRef(IDirect3DSurface9 *iface)
{
    IDirect3DSurface9Impl *This = impl_from_IDirect3DSurface9(iface);

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
            if (This->parentDevice) IDirect3DDevice9Ex_AddRef(This->parentDevice);
            wined3d_mutex_lock();
            wined3d_surface_incref(This->wined3d_surface);
            wined3d_mutex_unlock();
        }

        return ref;
    }

}

static ULONG WINAPI IDirect3DSurface9Impl_Release(IDirect3DSurface9 *iface)
{
    IDirect3DSurface9Impl *This = impl_from_IDirect3DSurface9(iface);

    TRACE("iface %p.\n", iface);

    if (This->forwardReference) {
        /* Forward to the containerParent */
        TRACE("(%p) : Forwarding to %p\n", This, This->forwardReference);
        return IUnknown_Release(This->forwardReference);
    } else {
        /* No container, handle our own refcounting */
        ULONG ref = InterlockedDecrement(&This->ref);

        TRACE("%p decreasing refcount to %u.\n", iface, ref);

        if (ref == 0) {
            IDirect3DDevice9Ex *parentDevice = This->parentDevice;

            wined3d_mutex_lock();
            wined3d_surface_decref(This->wined3d_surface);
            wined3d_mutex_unlock();

            /* Release the device last, as it may cause the device to be destroyed. */
            if (parentDevice) IDirect3DDevice9Ex_Release(parentDevice);
        }

        return ref;
    }
}

/* IDirect3DSurface9 IDirect3DResource9 Interface follow: */
static HRESULT WINAPI IDirect3DSurface9Impl_GetDevice(IDirect3DSurface9 *iface,
        IDirect3DDevice9 **device)
{
    IDirect3DSurface9Impl *This = impl_from_IDirect3DSurface9(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    if (This->forwardReference)
    {
        IDirect3DResource9 *resource;
        HRESULT hr;

        hr = IUnknown_QueryInterface(This->forwardReference, &IID_IDirect3DResource9, (void **)&resource);
        if (SUCCEEDED(hr))
        {
            hr = IDirect3DResource9_GetDevice(resource, device);
            IDirect3DResource9_Release(resource);

            TRACE("Returning device %p.\n", *device);
        }

        return hr;
    }

    *device = (IDirect3DDevice9 *)This->parentDevice;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DSurface9Impl_SetPrivateData(IDirect3DSurface9 *iface, REFGUID guid,
        const void *data, DWORD data_size, DWORD flags)
{
    IDirect3DSurface9Impl *surface = impl_from_IDirect3DSurface9(iface);
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

static HRESULT WINAPI IDirect3DSurface9Impl_GetPrivateData(IDirect3DSurface9 *iface, REFGUID guid,
        void *data, DWORD *data_size)
{
    IDirect3DSurface9Impl *surface = impl_from_IDirect3DSurface9(iface);
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

static HRESULT WINAPI IDirect3DSurface9Impl_FreePrivateData(IDirect3DSurface9 *iface, REFGUID guid)
{
    IDirect3DSurface9Impl *surface = impl_from_IDirect3DSurface9(iface);
    struct wined3d_resource *resource;
    HRESULT hr;

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(guid));

    wined3d_mutex_lock();
    resource = wined3d_surface_get_resource(surface->wined3d_surface);
    hr = wined3d_resource_free_private_data(resource, guid);
    wined3d_mutex_unlock();

    return hr;
}

static DWORD WINAPI IDirect3DSurface9Impl_SetPriority(IDirect3DSurface9 *iface, DWORD PriorityNew)
{
    IDirect3DSurface9Impl *This = impl_from_IDirect3DSurface9(iface);
    HRESULT hr;

    TRACE("iface %p, priority %u.\n", iface, PriorityNew);

    wined3d_mutex_lock();
    hr = wined3d_surface_set_priority(This->wined3d_surface, PriorityNew);
    wined3d_mutex_unlock();

    return hr;
}

static DWORD WINAPI IDirect3DSurface9Impl_GetPriority(IDirect3DSurface9 *iface)
{
    IDirect3DSurface9Impl *This = impl_from_IDirect3DSurface9(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_surface_get_priority(This->wined3d_surface);
    wined3d_mutex_unlock();

    return hr;
}

static void WINAPI IDirect3DSurface9Impl_PreLoad(IDirect3DSurface9 *iface)
{
    IDirect3DSurface9Impl *This = impl_from_IDirect3DSurface9(iface);

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    wined3d_surface_preload(This->wined3d_surface);
    wined3d_mutex_unlock();
}

static D3DRESOURCETYPE WINAPI IDirect3DSurface9Impl_GetType(IDirect3DSurface9 *iface)
{
    TRACE("iface %p.\n", iface);

    return D3DRTYPE_SURFACE;
}

/* IDirect3DSurface9 Interface follow: */
static HRESULT WINAPI IDirect3DSurface9Impl_GetContainer(IDirect3DSurface9 *iface, REFIID riid,
        void **ppContainer)
{
    IDirect3DSurface9Impl *This = impl_from_IDirect3DSurface9(iface);
    HRESULT res;

    TRACE("iface %p, riid %s, container %p.\n", iface, debugstr_guid(riid), ppContainer);

    if (!This->container) return E_NOINTERFACE;

    res = IUnknown_QueryInterface(This->container, riid, ppContainer);

    TRACE("Returning ppContainer %p, *ppContainer %p\n", ppContainer, *ppContainer);

    return res;
}

static HRESULT WINAPI IDirect3DSurface9Impl_GetDesc(IDirect3DSurface9 *iface, D3DSURFACE_DESC *desc)
{
    IDirect3DSurface9Impl *This = impl_from_IDirect3DSurface9(iface);
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
    desc->MultiSampleType = wined3d_desc.multisample_type;
    desc->MultiSampleQuality = wined3d_desc.multisample_quality;
    desc->Width = wined3d_desc.width;
    desc->Height = wined3d_desc.height;

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DSurface9Impl_LockRect(IDirect3DSurface9 *iface,
        D3DLOCKED_RECT *pLockedRect, const RECT *pRect, DWORD Flags)
{
    IDirect3DSurface9Impl *This = impl_from_IDirect3DSurface9(iface);
    HRESULT hr;

    TRACE("iface %p, locked_rect %p, rect %p, flags %#x.\n", iface, pLockedRect, pRect, Flags);

    wined3d_mutex_lock();
    hr = wined3d_surface_map(This->wined3d_surface, (struct wined3d_mapped_rect *)pLockedRect, pRect, Flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DSurface9Impl_UnlockRect(IDirect3DSurface9 *iface)
{
    IDirect3DSurface9Impl *This = impl_from_IDirect3DSurface9(iface);
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

static HRESULT WINAPI IDirect3DSurface9Impl_GetDC(IDirect3DSurface9 *iface, HDC* phdc)
{
    IDirect3DSurface9Impl *This = impl_from_IDirect3DSurface9(iface);
    HRESULT hr;

    TRACE("iface %p, hdc %p.\n", iface, phdc);

    if(!This->getdc_supported)
    {
        WARN("Surface does not support GetDC, returning D3DERR_INVALIDCALL\n");
        /* Don't touch the DC */
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = wined3d_surface_getdc(This->wined3d_surface, phdc);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DSurface9Impl_ReleaseDC(IDirect3DSurface9 *iface, HDC hdc)
{
    IDirect3DSurface9Impl *This = impl_from_IDirect3DSurface9(iface);
    HRESULT hr;

    TRACE("iface %p, hdc %p.\n", iface, hdc);

    wined3d_mutex_lock();
    hr = wined3d_surface_releasedc(This->wined3d_surface, hdc);
    wined3d_mutex_unlock();

    switch (hr)
    {
        case WINEDDERR_NODC:    return D3DERR_INVALIDCALL;
        default:                return hr;
    }
}

static const IDirect3DSurface9Vtbl Direct3DSurface9_Vtbl =
{
    /* IUnknown */
    IDirect3DSurface9Impl_QueryInterface,
    IDirect3DSurface9Impl_AddRef,
    IDirect3DSurface9Impl_Release,
    /* IDirect3DResource9 */
    IDirect3DSurface9Impl_GetDevice,
    IDirect3DSurface9Impl_SetPrivateData,
    IDirect3DSurface9Impl_GetPrivateData,
    IDirect3DSurface9Impl_FreePrivateData,
    IDirect3DSurface9Impl_SetPriority,
    IDirect3DSurface9Impl_GetPriority,
    IDirect3DSurface9Impl_PreLoad,
    IDirect3DSurface9Impl_GetType,
    /* IDirect3DSurface9 */
    IDirect3DSurface9Impl_GetContainer,
    IDirect3DSurface9Impl_GetDesc,
    IDirect3DSurface9Impl_LockRect,
    IDirect3DSurface9Impl_UnlockRect,
    IDirect3DSurface9Impl_GetDC,
    IDirect3DSurface9Impl_ReleaseDC
};

static void STDMETHODCALLTYPE surface_wined3d_object_destroyed(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d9_surface_wined3d_parent_ops =
{
    surface_wined3d_object_destroyed,
};

HRESULT surface_init(IDirect3DSurface9Impl *surface, IDirect3DDevice9Impl *device,
        UINT width, UINT height, D3DFORMAT format, BOOL lockable, BOOL discard, UINT level,
        DWORD usage, D3DPOOL pool, D3DMULTISAMPLE_TYPE multisample_type, DWORD multisample_quality)
{
    DWORD flags = 0;
    HRESULT hr;

    surface->IDirect3DSurface9_iface.lpVtbl = &Direct3DSurface9_Vtbl;
    surface->ref = 1;

    switch (format)
    {
        case D3DFMT_A8R8G8B8:
        case D3DFMT_X8R8G8B8:
        case D3DFMT_R5G6B5:
        case D3DFMT_X1R5G5B5:
        case D3DFMT_A1R5G5B5:
        case D3DFMT_R8G8B8:
            surface->getdc_supported = TRUE;
            break;

        default:
            surface->getdc_supported = FALSE;
            break;
    }

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
            SURFACE_OPENGL, flags, surface, &d3d9_surface_wined3d_parent_ops, &surface->wined3d_surface);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d surface, hr %#x.\n", hr);
        return hr;
    }

    surface->parentDevice = &device->IDirect3DDevice9Ex_iface;
    IDirect3DDevice9Ex_AddRef(surface->parentDevice);

    return D3D_OK;
}

IDirect3DSurface9Impl *unsafe_impl_from_IDirect3DSurface9(IDirect3DSurface9 *iface)
{
    if (!iface)
        return NULL;
    assert(iface->lpVtbl == &Direct3DSurface9_Vtbl);

    return impl_from_IDirect3DSurface9(iface);
}
