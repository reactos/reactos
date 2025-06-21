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

#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

static inline struct d3d9_vertexshader *impl_from_IDirect3DVertexShader9(IDirect3DVertexShader9 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d9_vertexshader, IDirect3DVertexShader9_iface);
}

static HRESULT WINAPI d3d9_vertexshader_QueryInterface(IDirect3DVertexShader9 *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DVertexShader9)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DVertexShader9_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d9_vertexshader_AddRef(IDirect3DVertexShader9 *iface)
{
    struct d3d9_vertexshader *shader = impl_from_IDirect3DVertexShader9(iface);
    ULONG refcount = InterlockedIncrement(&shader->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    if (refcount == 1)
    {
        IDirect3DDevice9Ex_AddRef(shader->parent_device);
        wined3d_shader_incref(shader->wined3d_shader);
    }

    return refcount;
}

static ULONG WINAPI d3d9_vertexshader_Release(IDirect3DVertexShader9 *iface)
{
    struct d3d9_vertexshader *shader = impl_from_IDirect3DVertexShader9(iface);
    ULONG refcount = InterlockedDecrement(&shader->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        IDirect3DDevice9Ex *device = shader->parent_device;
        wined3d_shader_decref(shader->wined3d_shader);
        /* Release the device last, as it may cause the device to be destroyed. */
        IDirect3DDevice9Ex_Release(device);
    }

    return refcount;
}

static HRESULT WINAPI d3d9_vertexshader_GetDevice(IDirect3DVertexShader9 *iface, IDirect3DDevice9 **device)
{
    struct d3d9_vertexshader *shader = impl_from_IDirect3DVertexShader9(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)shader->parent_device;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI d3d9_vertexshader_GetFunction(IDirect3DVertexShader9 *iface, void *data, UINT *data_size)
{
    struct d3d9_vertexshader *shader = impl_from_IDirect3DVertexShader9(iface);
    HRESULT hr;

    TRACE("iface %p, data %p, data_size %p.\n", iface, data, data_size);

    wined3d_mutex_lock();
    hr = wined3d_shader_get_byte_code(shader->wined3d_shader, data, data_size);
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
    free(parent);
}

static const struct wined3d_parent_ops d3d9_vertexshader_wined3d_parent_ops =
{
    d3d9_vertexshader_wined3d_object_destroyed,
};

HRESULT vertexshader_init(struct d3d9_vertexshader *shader, struct d3d9_device *device, const DWORD *byte_code)
{
    struct wined3d_shader_desc desc;
    HRESULT hr;

    shader->refcount = 1;
    shader->IDirect3DVertexShader9_iface.lpVtbl = &d3d9_vertexshader_vtbl;

    desc.byte_code = byte_code;
    desc.byte_code_size = ~(size_t)0;

    wined3d_mutex_lock();
    hr = wined3d_shader_create_vs(device->wined3d_device, &desc, shader,
            &d3d9_vertexshader_wined3d_parent_ops, &shader->wined3d_shader);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d vertex shader, hr %#lx.\n", hr);
        return hr;
    }

    shader->parent_device = &device->IDirect3DDevice9Ex_iface;
    IDirect3DDevice9Ex_AddRef(shader->parent_device);

    return D3D_OK;
}

struct d3d9_vertexshader *unsafe_impl_from_IDirect3DVertexShader9(IDirect3DVertexShader9 *iface)
{
    if (!iface)
        return NULL;
    if (iface->lpVtbl != &d3d9_vertexshader_vtbl)
        WARN("Vertex shader %p with the wrong vtbl %p\n", iface, iface->lpVtbl);

    return impl_from_IDirect3DVertexShader9(iface);
}

static inline struct d3d9_pixelshader *impl_from_IDirect3DPixelShader9(IDirect3DPixelShader9 *iface)
{
    return CONTAINING_RECORD(iface, struct d3d9_pixelshader, IDirect3DPixelShader9_iface);
}

static HRESULT WINAPI d3d9_pixelshader_QueryInterface(IDirect3DPixelShader9 *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirect3DPixelShader9)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirect3DPixelShader9_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d9_pixelshader_AddRef(IDirect3DPixelShader9 *iface)
{
    struct d3d9_pixelshader *shader = impl_from_IDirect3DPixelShader9(iface);
    ULONG refcount = InterlockedIncrement(&shader->refcount);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    if (refcount == 1)
    {
        IDirect3DDevice9Ex_AddRef(shader->parent_device);
        wined3d_shader_incref(shader->wined3d_shader);
    }

    return refcount;
}

static ULONG WINAPI d3d9_pixelshader_Release(IDirect3DPixelShader9 *iface)
{
    struct d3d9_pixelshader *shader = impl_from_IDirect3DPixelShader9(iface);
    ULONG refcount = InterlockedDecrement(&shader->refcount);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
    {
        IDirect3DDevice9Ex *device = shader->parent_device;
        wined3d_shader_decref(shader->wined3d_shader);
        /* Release the device last, as it may cause the device to be destroyed. */
        IDirect3DDevice9Ex_Release(device);
    }

    return refcount;
}

static HRESULT WINAPI d3d9_pixelshader_GetDevice(IDirect3DPixelShader9 *iface, IDirect3DDevice9 **device)
{
    struct d3d9_pixelshader *shader = impl_from_IDirect3DPixelShader9(iface);

    TRACE("iface %p, device %p.\n", iface, device);

    *device = (IDirect3DDevice9 *)shader->parent_device;
    IDirect3DDevice9_AddRef(*device);

    TRACE("Returning device %p.\n", *device);

    return D3D_OK;
}

static HRESULT WINAPI d3d9_pixelshader_GetFunction(IDirect3DPixelShader9 *iface, void *data, UINT *data_size)
{
    struct d3d9_pixelshader *shader = impl_from_IDirect3DPixelShader9(iface);
    HRESULT hr;

    TRACE("iface %p, data %p, data_size %p.\n", iface, data, data_size);

    wined3d_mutex_lock();
    hr = wined3d_shader_get_byte_code(shader->wined3d_shader, data, data_size);
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
    free(parent);
}

static const struct wined3d_parent_ops d3d9_pixelshader_wined3d_parent_ops =
{
    d3d9_pixelshader_wined3d_object_destroyed,
};

HRESULT pixelshader_init(struct d3d9_pixelshader *shader, struct d3d9_device *device, const DWORD *byte_code)
{
    struct wined3d_shader_desc desc;
    HRESULT hr;

    shader->refcount = 1;
    shader->IDirect3DPixelShader9_iface.lpVtbl = &d3d9_pixelshader_vtbl;

    desc.byte_code = byte_code;
    desc.byte_code_size = ~(size_t)0;

    wined3d_mutex_lock();
    hr = wined3d_shader_create_ps(device->wined3d_device, &desc, shader,
            &d3d9_pixelshader_wined3d_parent_ops, &shader->wined3d_shader);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to created wined3d pixel shader, hr %#lx.\n", hr);
        return hr;
    }

    shader->parent_device = &device->IDirect3DDevice9Ex_iface;
    IDirect3DDevice9Ex_AddRef(shader->parent_device);

    return D3D_OK;
}

struct d3d9_pixelshader *unsafe_impl_from_IDirect3DPixelShader9(IDirect3DPixelShader9 *iface)
{
    if (!iface)
        return NULL;
    if (iface->lpVtbl != &d3d9_pixelshader_vtbl)
        WARN("Pixel shader %p with the wrong vtbl %p\n", iface, iface->lpVtbl);

    return impl_from_IDirect3DPixelShader9(iface);
}
