/*
 * IDirect3DPixelShader8 implementation
 *
 * Copyright 2002-2003 Jason Edmeades
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
#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

/* IDirect3DPixelShader8 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DPixelShader8Impl_QueryInterface(IDirect3DPixelShader8 *iface, REFIID riid, LPVOID *ppobj) {
    IDirect3DPixelShader8Impl *This = (IDirect3DPixelShader8Impl *)iface;

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DPixelShader8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DPixelShader8Impl_AddRef(IDirect3DPixelShader8 *iface) {
    IDirect3DPixelShader8Impl *This = (IDirect3DPixelShader8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    if (ref == 1)
    {
        wined3d_mutex_lock();
        IWineD3DPixelShader_AddRef(This->wineD3DPixelShader);
        wined3d_mutex_unlock();
    }

    return ref;
}

static ULONG WINAPI IDirect3DPixelShader8Impl_Release(IDirect3DPixelShader8 * iface) {
    IDirect3DPixelShader8Impl *This = (IDirect3DPixelShader8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (ref == 0) {
        wined3d_mutex_lock();
        IWineD3DPixelShader_Release(This->wineD3DPixelShader);
        wined3d_mutex_unlock();
    }
    return ref;
}

static const IDirect3DPixelShader8Vtbl Direct3DPixelShader8_Vtbl =
{
    /* IUnknown */
    IDirect3DPixelShader8Impl_QueryInterface,
    IDirect3DPixelShader8Impl_AddRef,
    IDirect3DPixelShader8Impl_Release,
};

static void STDMETHODCALLTYPE d3d8_pixelshader_wined3d_object_destroyed(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d8_pixelshader_wined3d_parent_ops =
{
    d3d8_pixelshader_wined3d_object_destroyed,
};

HRESULT pixelshader_init(IDirect3DPixelShader8Impl *shader, IDirect3DDevice8Impl *device,
        const DWORD *byte_code, DWORD shader_handle)
{
    HRESULT hr;

    shader->ref = 1;
    shader->lpVtbl = &Direct3DPixelShader8_Vtbl;
    shader->handle = shader_handle;

    wined3d_mutex_lock();
    hr = IWineD3DDevice_CreatePixelShader(device->WineD3DDevice, byte_code,
            NULL, &shader->wineD3DPixelShader, (IUnknown *)shader,
            &d3d8_pixelshader_wined3d_parent_ops);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d pixel shader, hr %#x.\n", hr);
        return hr;
    }

    return D3D_OK;
}
