/*
 * IDirect3DStateBlock9 implementation
 *
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2002-2003 Jason Edmeades
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

static inline IDirect3DStateBlock9Impl *impl_from_IDirect3DStateBlock9(IDirect3DStateBlock9 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DStateBlock9Impl, IDirect3DStateBlock9_iface);
}

static HRESULT WINAPI IDirect3DStateBlock9Impl_QueryInterface(IDirect3DStateBlock9 *iface,
        REFIID riid, void **ppobj)
{
    IDirect3DStateBlock9Impl *This = impl_from_IDirect3DStateBlock9(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DStateBlock9)) {
        IDirect3DStateBlock9_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DStateBlock9Impl_AddRef(IDirect3DStateBlock9 *iface)
{
    IDirect3DStateBlock9Impl *This = impl_from_IDirect3DStateBlock9(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    return ref;
}

static ULONG WINAPI IDirect3DStateBlock9Impl_Release(IDirect3DStateBlock9 *iface)
{
    IDirect3DStateBlock9Impl *This = impl_from_IDirect3DStateBlock9(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (ref == 0) {
        wined3d_mutex_lock();
        wined3d_stateblock_decref(This->wined3d_stateblock);
        wined3d_mutex_unlock();

        IDirect3DDevice9Ex_Release(This->parentDevice);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DStateBlock9 Interface follow: */
static HRESULT WINAPI IDirect3DStateBlock9Impl_GetDevice(IDirect3DStateBlock9 *iface,
        IDirect3DDevice9 **device)
{
    IDirect3DStateBlock9Impl *This = impl_from_IDirect3DStateBlock9(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)This->parentDevice;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DStateBlock9Impl_Capture(IDirect3DStateBlock9 *iface)
{
    IDirect3DStateBlock9Impl *This = impl_from_IDirect3DStateBlock9(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_capture(This->wined3d_stateblock);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DStateBlock9Impl_Apply(IDirect3DStateBlock9 *iface)
{
    IDirect3DStateBlock9Impl *This = impl_from_IDirect3DStateBlock9(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_apply(This->wined3d_stateblock);
    wined3d_mutex_unlock();

    return hr;
}


static const IDirect3DStateBlock9Vtbl Direct3DStateBlock9_Vtbl =
{
    /* IUnknown */
    IDirect3DStateBlock9Impl_QueryInterface,
    IDirect3DStateBlock9Impl_AddRef,
    IDirect3DStateBlock9Impl_Release,
    /* IDirect3DStateBlock9 */
    IDirect3DStateBlock9Impl_GetDevice,
    IDirect3DStateBlock9Impl_Capture,
    IDirect3DStateBlock9Impl_Apply
};

HRESULT stateblock_init(IDirect3DStateBlock9Impl *stateblock, IDirect3DDevice9Impl *device,
        D3DSTATEBLOCKTYPE type, struct wined3d_stateblock *wined3d_stateblock)
{
    HRESULT hr;

    stateblock->IDirect3DStateBlock9_iface.lpVtbl = &Direct3DStateBlock9_Vtbl;
    stateblock->ref = 1;

    if (wined3d_stateblock)
    {
        stateblock->wined3d_stateblock = wined3d_stateblock;
    }
    else
    {
        wined3d_mutex_lock();
        hr = wined3d_stateblock_create(device->wined3d_device,
                (enum wined3d_stateblock_type)type, &stateblock->wined3d_stateblock);
        wined3d_mutex_unlock();
        if (FAILED(hr))
        {
            WARN("Failed to create wined3d stateblock, hr %#x.\n", hr);
            return hr;
        }
    }

    stateblock->parentDevice = &device->IDirect3DDevice9Ex_iface;
    IDirect3DDevice9Ex_AddRef(stateblock->parentDevice);

    return D3D_OK;
}
