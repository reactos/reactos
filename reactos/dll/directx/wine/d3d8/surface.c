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
#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

/* IDirect3DSurface8 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DSurface8Impl_QueryInterface(LPDIRECT3DSURFACE8 iface, REFIID riid, LPVOID *ppobj) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;

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

static ULONG WINAPI IDirect3DSurface8Impl_AddRef(LPDIRECT3DSURFACE8 iface) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;

    TRACE("(%p)\n", This);

    if (This->forwardReference) {
        /* Forward refcounting */
        TRACE("(%p) : Forwarding to %p\n", This, This->forwardReference);
        return IUnknown_AddRef(This->forwardReference);
    } else {
        /* No container, handle our own refcounting */
        ULONG ref = InterlockedIncrement(&This->ref);
        if (ref == 1)
        {
            if (This->parentDevice) IUnknown_AddRef(This->parentDevice);
            wined3d_mutex_lock();
            IUnknown_AddRef(This->wineD3DSurface);
            wined3d_mutex_unlock();
        }
        TRACE("(%p) : AddRef from %d\n", This, ref - 1);
        return ref;
    }
}

static ULONG WINAPI IDirect3DSurface8Impl_Release(LPDIRECT3DSURFACE8 iface) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;

    TRACE("(%p)\n", This);

    if (This->forwardReference) {
        /* Forward refcounting */
        TRACE("(%p) : Forwarding to %p\n", This, This->forwardReference);
        return IUnknown_Release(This->forwardReference);
    } else {
        /* No container, handle our own refcounting */
        ULONG ref = InterlockedDecrement(&This->ref);
        TRACE("(%p) : ReleaseRef to %d\n", This, ref);

        if (ref == 0) {
            if (This->parentDevice) IUnknown_Release(This->parentDevice);
            /* Implicit surfaces are destroyed with the device, not if refcount reaches 0. */
            wined3d_mutex_lock();
            IWineD3DSurface_Release(This->wineD3DSurface);
            wined3d_mutex_unlock();
        }

        return ref;
    }
}

/* IDirect3DSurface8 IDirect3DResource8 Interface follow: */
static HRESULT WINAPI IDirect3DSurface8Impl_GetDevice(LPDIRECT3DSURFACE8 iface, IDirect3DDevice8 **ppDevice) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    IWineD3DDevice *wined3d_device;
    HRESULT hr;
    TRACE("(%p)->(%p)\n", This, ppDevice);

    wined3d_mutex_lock();
    hr = IWineD3DSurface_GetDevice(This->wineD3DSurface, &wined3d_device);
    if (SUCCEEDED(hr))
    {
        IWineD3DDevice_GetParent(wined3d_device, (IUnknown **)ppDevice);
        IWineD3DDevice_Release(wined3d_device);
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DSurface8Impl_SetPrivateData(LPDIRECT3DSURFACE8 iface, REFGUID refguid, CONST void *pData, DWORD SizeOfData, DWORD Flags) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    hr = IWineD3DSurface_SetPrivateData(This->wineD3DSurface, refguid, pData, SizeOfData, Flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DSurface8Impl_GetPrivateData(LPDIRECT3DSURFACE8 iface, REFGUID refguid, void *pData, DWORD *pSizeOfData) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    hr = IWineD3DSurface_GetPrivateData(This->wineD3DSurface, refguid, pData, pSizeOfData);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DSurface8Impl_FreePrivateData(LPDIRECT3DSURFACE8 iface, REFGUID refguid) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    hr = IWineD3DSurface_FreePrivateData(This->wineD3DSurface, refguid);
    wined3d_mutex_unlock();

    return hr;
}

/* IDirect3DSurface8 Interface follow: */
static HRESULT WINAPI IDirect3DSurface8Impl_GetContainer(LPDIRECT3DSURFACE8 iface, REFIID riid, void **ppContainer) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    HRESULT res;

    TRACE("(%p) Relay\n", This);

    if (!This->container) return E_NOINTERFACE;

    res = IUnknown_QueryInterface(This->container, riid, ppContainer);

    TRACE("(%p) : returning %p\n", This, *ppContainer);
    return res;
}

static HRESULT WINAPI IDirect3DSurface8Impl_GetDesc(LPDIRECT3DSURFACE8 iface, D3DSURFACE_DESC *pDesc) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    WINED3DSURFACE_DESC    wined3ddesc;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    hr = IWineD3DSurface_GetDesc(This->wineD3DSurface, &wined3ddesc);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr))
    {
        pDesc->Format = d3dformat_from_wined3dformat(wined3ddesc.format);
        pDesc->Type = wined3ddesc.resource_type;
        pDesc->Usage = wined3ddesc.usage;
        pDesc->Pool = wined3ddesc.pool;
        pDesc->Size = wined3ddesc.size;
        pDesc->MultiSampleType = wined3ddesc.multisample_type;
        pDesc->Width = wined3ddesc.width;
        pDesc->Height = wined3ddesc.height;
    }

    return hr;
}

static HRESULT WINAPI IDirect3DSurface8Impl_LockRect(LPDIRECT3DSURFACE8 iface, D3DLOCKED_RECT *pLockedRect, CONST RECT *pRect, DWORD Flags) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);
    TRACE("(%p) calling IWineD3DSurface_LockRect %p %p %p %d\n", This, This->wineD3DSurface, pLockedRect, pRect, Flags);

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

    hr = IWineD3DSurface_LockRect(This->wineD3DSurface, (WINED3DLOCKED_RECT *) pLockedRect, pRect, Flags);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DSurface8Impl_UnlockRect(LPDIRECT3DSURFACE8 iface) {
    IDirect3DSurface8Impl *This = (IDirect3DSurface8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    wined3d_mutex_lock();
    hr = IWineD3DSurface_UnlockRect(This->wineD3DSurface);
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
    HRESULT hr;

    surface->lpVtbl = &Direct3DSurface8_Vtbl;
    surface->ref = 1;

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
            &d3d8_surface_wined3d_parent_ops);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d surface, hr %#x.\n", hr);
        return hr;
    }

    surface->parentDevice = (IDirect3DDevice8 *)device;
    IUnknown_AddRef(surface->parentDevice);

    return D3D_OK;
}
