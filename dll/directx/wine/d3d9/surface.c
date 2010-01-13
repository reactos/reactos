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
#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

/* IDirect3DSurface9 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DSurface9Impl_QueryInterface(LPDIRECT3DSURFACE9 iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;

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

static ULONG WINAPI IDirect3DSurface9Impl_AddRef(LPDIRECT3DSURFACE9 iface) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;

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
            IWineD3DSurface_AddRef(This->wineD3DSurface);
            wined3d_mutex_unlock();
        }

        return ref;
    }

}

static ULONG WINAPI IDirect3DSurface9Impl_Release(LPDIRECT3DSURFACE9 iface) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;

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
            IWineD3DSurface_Release(This->wineD3DSurface);
            wined3d_mutex_unlock();

            /* Release the device last, as it may cause the device to be destroyed. */
            if (parentDevice) IDirect3DDevice9Ex_Release(parentDevice);
        }

        return ref;
    }
}

/* IDirect3DSurface9 IDirect3DResource9 Interface follow: */
static HRESULT WINAPI IDirect3DSurface9Impl_GetDevice(IDirect3DSurface9 *iface, IDirect3DDevice9 **device)
{
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;

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

static HRESULT WINAPI IDirect3DSurface9Impl_SetPrivateData(LPDIRECT3DSURFACE9 iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %u, flags %#x.\n",
            iface, debugstr_guid(refguid), pData, SizeOfData, Flags);

    wined3d_mutex_lock();
    hr = IWineD3DSurface_SetPrivateData(This->wineD3DSurface, refguid, pData, SizeOfData, Flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DSurface9Impl_GetPrivateData(LPDIRECT3DSURFACE9 iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, guid %s, data %p, data_size %p.\n",
            iface, debugstr_guid(refguid), pData, pSizeOfData);

    wined3d_mutex_lock();
    hr = IWineD3DSurface_GetPrivateData(This->wineD3DSurface, refguid, pData, pSizeOfData);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DSurface9Impl_FreePrivateData(LPDIRECT3DSURFACE9 iface, REFGUID refguid) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, guid %s.\n", iface, debugstr_guid(refguid));

    wined3d_mutex_lock();
    hr = IWineD3DSurface_FreePrivateData(This->wineD3DSurface, refguid);
    wined3d_mutex_unlock();

    return hr;
}

static DWORD WINAPI IDirect3DSurface9Impl_SetPriority(LPDIRECT3DSURFACE9 iface, DWORD PriorityNew) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, priority %u.\n", iface, PriorityNew);

    wined3d_mutex_lock();
    hr = IWineD3DSurface_SetPriority(This->wineD3DSurface, PriorityNew);
    wined3d_mutex_unlock();

    return hr;
}

static DWORD WINAPI IDirect3DSurface9Impl_GetPriority(LPDIRECT3DSURFACE9 iface) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = IWineD3DSurface_GetPriority(This->wineD3DSurface);
    wined3d_mutex_unlock();

    return hr;
}

static void WINAPI IDirect3DSurface9Impl_PreLoad(LPDIRECT3DSURFACE9 iface) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    IWineD3DSurface_PreLoad(This->wineD3DSurface);
    wined3d_mutex_unlock();
}

static D3DRESOURCETYPE WINAPI IDirect3DSurface9Impl_GetType(LPDIRECT3DSURFACE9 iface) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    D3DRESOURCETYPE ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = IWineD3DSurface_GetType(This->wineD3DSurface);
    wined3d_mutex_unlock();

    return ret;
}

/* IDirect3DSurface9 Interface follow: */
static HRESULT WINAPI IDirect3DSurface9Impl_GetContainer(LPDIRECT3DSURFACE9 iface, REFIID riid, void** ppContainer) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    HRESULT res;

    TRACE("iface %p, riid %s, container %p.\n", iface, debugstr_guid(riid), ppContainer);

    if (!This->container) return E_NOINTERFACE;

    if (!ppContainer) {
        ERR("Called without a valid ppContainer\n");
    }

    res = IUnknown_QueryInterface(This->container, riid, ppContainer);

    TRACE("Returning ppContainer %p, *ppContainer %p\n", ppContainer, *ppContainer);

    return res;
}

static HRESULT WINAPI IDirect3DSurface9Impl_GetDesc(LPDIRECT3DSURFACE9 iface, D3DSURFACE_DESC* pDesc) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    WINED3DSURFACE_DESC wined3ddesc;
    HRESULT hr;

    TRACE("iface %p, desc %p.\n", iface, pDesc);

    wined3d_mutex_lock();
    hr = IWineD3DSurface_GetDesc(This->wineD3DSurface, &wined3ddesc);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr))
    {
        pDesc->Format = d3dformat_from_wined3dformat(wined3ddesc.format);
        pDesc->Type = wined3ddesc.resource_type;
        pDesc->Usage = wined3ddesc.usage;
        pDesc->Pool = wined3ddesc.pool;
        pDesc->MultiSampleType = wined3ddesc.multisample_type;
        pDesc->MultiSampleQuality = wined3ddesc.multisample_quality;
        pDesc->Width = wined3ddesc.width;
        pDesc->Height = wined3ddesc.height;
    }

    return hr;
}

static HRESULT WINAPI IDirect3DSurface9Impl_LockRect(LPDIRECT3DSURFACE9 iface, D3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, locked_rect %p, rect %p, flags %#x.\n", iface, pLockedRect, pRect, Flags);

    wined3d_mutex_lock();
    hr = IWineD3DSurface_LockRect(This->wineD3DSurface, (WINED3DLOCKED_RECT *) pLockedRect, pRect, Flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DSurface9Impl_UnlockRect(LPDIRECT3DSURFACE9 iface) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = IWineD3DSurface_UnlockRect(This->wineD3DSurface);
    wined3d_mutex_unlock();

    switch(hr)
    {
        case WINEDDERR_NOTLOCKED:       return D3DERR_INVALIDCALL;
        default:                        return hr;
    }
}

static HRESULT WINAPI IDirect3DSurface9Impl_GetDC(LPDIRECT3DSURFACE9 iface, HDC* phdc) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, hdc %p.\n", iface, phdc);

    if(!This->getdc_supported)
    {
        WARN("Surface does not support GetDC, returning D3DERR_INVALIDCALL\n");
        /* Don't touch the DC */
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = IWineD3DSurface_GetDC(This->wineD3DSurface, phdc);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DSurface9Impl_ReleaseDC(LPDIRECT3DSURFACE9 iface, HDC hdc) {
    IDirect3DSurface9Impl *This = (IDirect3DSurface9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, hdc %p.\n", iface, hdc);

    wined3d_mutex_lock();
    hr = IWineD3DSurface_ReleaseDC(This->wineD3DSurface, hdc);
    wined3d_mutex_unlock();

    switch(hr) {
        case WINEDDERR_NODC:    return WINED3DERR_INVALIDCALL;
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
    HRESULT hr;

    surface->lpVtbl = &Direct3DSurface9_Vtbl;
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

    wined3d_mutex_lock();
    hr = IWineD3DDevice_CreateSurface(device->WineD3DDevice, width, height, wined3dformat_from_d3dformat(format),
            lockable, discard, level, &surface->wineD3DSurface, usage & WINED3DUSAGE_MASK, (WINED3DPOOL)pool,
            multisample_type, multisample_quality, SURFACE_OPENGL, (IUnknown *)surface,
            &d3d9_surface_wined3d_parent_ops);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d surface, hr %#x.\n", hr);
        return hr;
    }

    surface->parentDevice = (IDirect3DDevice9Ex *)device;
    IDirect3DDevice9Ex_AddRef(surface->parentDevice);

    return D3D_OK;
}
