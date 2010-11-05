/*
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
#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

static HRESULT WINAPI d3d9_vertexshader_QueryInterface(IDirect3DVertexShader9 *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IDirect3DVertexShader9)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DVertexShader9_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d9_vertexshader_AddRef(IDirect3DVertexShader9 *iface)
{
    IDirect3DVertexShader9Impl *shader = (IDirect3DVertexShader9Impl *)iface;
    ULONG refcount = InterlockedIncrement(&shader->ref);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    if (refcount == 1)
    {
        IDirect3DDevice9Ex_AddRef(shader->parentDevice);
        wined3d_mutex_lock();
        IWineD3DVertexShader_AddRef(shader->wineD3DVertexShader);
        wined3d_mutex_unlock();
    }

    return refcount;
}

static ULONG WINAPI d3d9_vertexshader_Release(IDirect3DVertexShader9 *iface)
{
    IDirect3DVertexShader9Impl *shader = (IDirect3DVertexShader9Impl *)iface;
    ULONG refcount = InterlockedDecrement(&shader->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        IDirect3DDevice9Ex *device = shader->parentDevice;

        wined3d_mutex_lock();
        IWineD3DVertexShader_Release(shader->wineD3DVertexShader);
        wined3d_mutex_unlock();

        /* Release the device last, as it may cause the device to be destroyed. */
        IDirect3DDevice9Ex_Release(device);
    }

    return refcount;
}

static HRESULT WINAPI d3d9_vertexshader_GetDevice(IDirect3DVertexShader9 *iface, IDirect3DDevice9 **device)
{
    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)((IDirect3DVertexShader9Impl *)iface)->parentDevice;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI d3d9_vertexshader_GetFunction(IDirect3DVertexShader9 *iface,
        void *data, UINT *data_size)
{
    HRESULT hr;

    TRACE("iface %p, data %p, data_size %p.\n", iface, data, data_size);

    wined3d_mutex_lock();
    hr = IWineD3DVertexShader_GetFunction(((IDirect3DVertexShader9Impl *)iface)->wineD3DVertexShader, data, data_size);
    wined3d_mutex_unlock();

    return hr;
}

static const IDirect3DVertexShader9Vtbl d3d9_vertexshader_vtbl =
{
    /* IUnknown */
    d3d9_vertexshader_QueryInterface,
    d3d9_vertexshader_AddRef,
    d3d9_vertexshader_Release,
    /* IDirect3DVertexShader9 */
    d3d9_vertexshader_GetDevice,
    d3d9_vertexshader_GetFunction,
};

static void STDMETHODCALLTYPE d3d9_vertexshader_wined3d_object_destroyed(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d9_vertexshader_wined3d_parent_ops =
{
    d3d9_vertexshader_wined3d_object_destroyed,
};

HRESULT vertexshader_init(IDirect3DVertexShader9Impl *shader, IDirect3DDevice9Impl *device, const DWORD *byte_code)
{
    HRESULT hr;

    shader->ref = 1;
    shader->lpVtbl = &d3d9_vertexshader_vtbl;

    wined3d_mutex_lock();
    hr = IWineD3DDevice_CreateVertexShader(device->WineD3DDevice, byte_code, NULL,
            shader, &d3d9_vertexshader_wined3d_parent_ops, &shader->wineD3DVertexShader);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d vertex shader, hr %#x.\n", hr);
        return hr;
    }

    shader->parentDevice = (IDirect3DDevice9Ex *)device;
    IDirect3DDevice9Ex_AddRef(shader->parentDevice);

    return D3D_OK;
}

static HRESULT WINAPI d3d9_pixelshader_QueryInterface(IDirect3DPixelShader9 *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IDirect3DPixelShader9)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DPixelShader9_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d9_pixelshader_AddRef(IDirect3DPixelShader9 *iface)
{
    IDirect3DPixelShader9Impl *shader = (IDirect3DPixelShader9Impl *)iface;
    ULONG refcount = InterlockedIncrement(&shader->ref);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    if (refcount == 1)
    {
        IDirect3DDevice9Ex_AddRef(shader->parentDevice);
        wined3d_mutex_lock();
        IWineD3DPixelShader_AddRef(shader->wineD3DPixelShader);
        wined3d_mutex_unlock();
    }

    return refcount;
}

static ULONG WINAPI d3d9_pixelshader_Release(IDirect3DPixelShader9 *iface)
{
    IDirect3DPixelShader9Impl *shader = (IDirect3DPixelShader9Impl *)iface;
    ULONG refcount = InterlockedDecrement(&shader->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        IDirect3DDevice9Ex *device = shader->parentDevice;

        wined3d_mutex_lock();
        IWineD3DPixelShader_Release(shader->wineD3DPixelShader);
        wined3d_mutex_unlock();

        /* Release the device last, as it may cause the device to be destroyed. */
        IDirect3DDevice9Ex_Release(device);
    }

    return refcount;
}

static HRESULT WINAPI d3d9_pixelshader_GetDevice(IDirect3DPixelShader9 *iface, IDirect3DDevice9 **device)
{
    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)((IDirect3DPixelShader9Impl *)iface)->parentDevice;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI d3d9_pixelshader_GetFunction(IDirect3DPixelShader9 *iface, void *data, UINT *data_size)
{
    HRESULT hr;

    TRACE("iface %p, data %p, data_size %p.\n", iface, data, data_size);

    wined3d_mutex_lock();
    hr = IWineD3DPixelShader_GetFunction(((IDirect3DPixelShader9Impl *)iface)->wineD3DPixelShader, data, data_size);
    wined3d_mutex_unlock();

    return hr;
}

static const IDirect3DPixelShader9Vtbl d3d9_pixelshader_vtbl =
{
    /* IUnknown */
    d3d9_pixelshader_QueryInterface,
    d3d9_pixelshader_AddRef,
    d3d9_pixelshader_Release,
    /* IDirect3DPixelShader9 */
    d3d9_pixelshader_GetDevice,
    d3d9_pixelshader_GetFunction,
};

static void STDMETHODCALLTYPE d3d9_pixelshader_wined3d_object_destroyed(void *parent)
{
    HeapFree(GetProcessHeap(), 0, parent);
}

static const struct wined3d_parent_ops d3d9_pixelshader_wined3d_parent_ops =
{
    d3d9_pixelshader_wined3d_object_destroyed,
};

HRESULT pixelshader_init(IDirect3DPixelShader9Impl *shader, IDirect3DDevice9Impl *device, const DWORD *byte_code)
{
    HRESULT hr;

    shader->ref = 1;
    shader->lpVtbl = &d3d9_pixelshader_vtbl;

    wined3d_mutex_lock();
    hr = IWineD3DDevice_CreatePixelShader(device->WineD3DDevice, byte_code, NULL, shader,
            &d3d9_pixelshader_wined3d_parent_ops, &shader->wineD3DPixelShader);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to created wined3d pixel shader, hr %#x.\n", hr);
        return hr;
    }

    shader->parentDevice = (IDirect3DDevice9Ex *)device;
    IDirect3DDevice9Ex_AddRef(shader->parentDevice);

    return D3D_OK;
}
