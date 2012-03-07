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
#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

static inline IDirect3DVertexShader8Impl *impl_from_IDirect3DVertexShader8(IDirect3DVertexShader8 *iface)
{
  return CONTAINING_RECORD(iface, IDirect3DVertexShader8Impl, IDirect3DVertexShader8_iface);
}

static HRESULT WINAPI d3d8_vertexshader_QueryInterface(IDirect3DVertexShader8 *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IDirect3DVertexShader8)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d8_vertexshader_AddRef(IDirect3DVertexShader8 *iface)
{
    IDirect3DVertexShader8Impl *shader = impl_from_IDirect3DVertexShader8(iface);
    ULONG refcount = InterlockedIncrement(&shader->ref);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    if (refcount == 1 && shader->wined3d_shader)
    {
        wined3d_mutex_lock();
        wined3d_shader_incref(shader->wined3d_shader);
        wined3d_mutex_unlock();
    }

    return refcount;
}

static void STDMETHODCALLTYPE d3d8_vertexshader_wined3d_object_destroyed(void *parent)
{
    IDirect3DVertexShader8Impl *shader = parent;
    IDirect3DVertexDeclaration8_Release(shader->vertex_declaration);
    HeapFree(GetProcessHeap(), 0, shader);
}

static ULONG WINAPI d3d8_vertexshader_Release(IDirect3DVertexShader8 *iface)
{
    IDirect3DVertexShader8Impl *shader = impl_from_IDirect3DVertexShader8(iface);
    ULONG refcount = InterlockedDecrement(&shader->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        if (shader->wined3d_shader)
        {
            wined3d_mutex_lock();
            wined3d_shader_decref(shader->wined3d_shader);
            wined3d_mutex_unlock();
        }
        else
        {
            d3d8_vertexshader_wined3d_object_destroyed(shader);
        }
    }

    return refcount;
}

static const IDirect3DVertexShader8Vtbl d3d8_vertexshader_vtbl =
{
    /* IUnknown */
    d3d8_vertexshader_QueryInterface,
    d3d8_vertexshader_AddRef,
    d3d8_vertexshader_Release,
};

static const struct wined3d_parent_ops d3d8_vertexshader_wined3d_parent_ops =
{
    d3d8_vertexshader_wined3d_object_destroyed,
};

static HRESULT d3d8_vertexshader_create_vertexdeclaration(IDirect3DDevice8Impl *device,
        const DWORD *declaration, DWORD shader_handle, IDirect3DVertexDeclaration8 **decl_ptr)
{
    IDirect3DVertexDeclaration8Impl *object;
    HRESULT hr;

    TRACE("device %p, declaration %p, shader_handle %#x, decl_ptr %p.\n",
            device, declaration, shader_handle, decl_ptr);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Memory allocation failed.\n");
        return E_OUTOFMEMORY;
    }

    hr = vertexdeclaration_init(object, device, declaration, shader_handle);
    if (FAILED(hr))
    {
        WARN("Failed to initialize vertex declaration, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created vertex declaration %p.\n", object);
    *decl_ptr = (IDirect3DVertexDeclaration8 *)object;

    return D3D_OK;
}

HRESULT vertexshader_init(IDirect3DVertexShader8Impl *shader, IDirect3DDevice8Impl *device,
        const DWORD *declaration, const DWORD *byte_code, DWORD shader_handle, DWORD usage)
{
    const DWORD *token = declaration;
    HRESULT hr;

    /* Test if the vertex declaration is valid. */
    while (D3DVSD_END() != *token)
    {
        D3DVSD_TOKENTYPE token_type = ((*token & D3DVSD_TOKENTYPEMASK) >> D3DVSD_TOKENTYPESHIFT);

        if (token_type == D3DVSD_TOKEN_STREAMDATA && !(token_type & 0x10000000))
        {
            DWORD type = ((*token & D3DVSD_DATATYPEMASK) >> D3DVSD_DATATYPESHIFT);
            DWORD reg  = ((*token & D3DVSD_VERTEXREGMASK) >> D3DVSD_VERTEXREGSHIFT);

            if (reg == D3DVSDE_NORMAL && type != D3DVSDT_FLOAT3 && !byte_code)
            {
                WARN("Attempt to use a non-FLOAT3 normal with the fixed function function\n");
                return D3DERR_INVALIDCALL;
            }
        }
        token += parse_token(token);
    }

    shader->ref = 1;
    shader->IDirect3DVertexShader8_iface.lpVtbl = &d3d8_vertexshader_vtbl;

    hr = d3d8_vertexshader_create_vertexdeclaration(device, declaration, shader_handle, &shader->vertex_declaration);
    if (FAILED(hr))
    {
        WARN("Failed to create vertex declaration, hr %#x.\n", hr);
        return hr;
    }

    if (byte_code)
    {
        if (usage) FIXME("Usage %#x not implemented.\n", usage);

        wined3d_mutex_lock();
        hr = wined3d_shader_create_vs(device->wined3d_device, byte_code, NULL /* output signature */,
                shader, &d3d8_vertexshader_wined3d_parent_ops, &shader->wined3d_shader, 1);
        wined3d_mutex_unlock();
        if (FAILED(hr))
        {
            WARN("Failed to create wined3d vertex shader, hr %#x.\n", hr);
            IDirect3DVertexDeclaration8_Release(shader->vertex_declaration);
            return hr;
        }

        load_local_constants(declaration, shader->wined3d_shader);
    }

    return D3D_OK;
}

static inline IDirect3DPixelShader8Impl *impl_from_IDirect3DPixelShader8(IDirect3DPixelShader8 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DPixelShader8Impl, IDirect3DPixelShader8_iface);
}

static HRESULT WINAPI d3d8_pixelshader_QueryInterface(IDirect3DPixelShader8 *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IDirect3DPixelShader8)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI d3d8_pixelshader_AddRef(IDirect3DPixelShader8 *iface)
{
    IDirect3DPixelShader8Impl *shader = impl_from_IDirect3DPixelShader8(iface);
    ULONG refcount = InterlockedIncrement(&shader->ref);

    TRACE("%p increasing refcount to %u.\n", iface, refcount);

    if (refcount == 1)
    {
        wined3d_mutex_lock();
        wined3d_shader_incref(shader->wined3d_shader);
        wined3d_mutex_unlock();
    }

    return refcount;
}

static ULONG WINAPI d3d8_pixelshader_Release(IDirect3DPixelShader8 *iface)
{
    IDirect3DPixelShader8Impl *shader = impl_from_IDirect3DPixelShader8(iface);
    ULONG refcount = InterlockedDecrement(&shader->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, refcount);

    if (!refcount)
    {
        wined3d_mutex_lock();
        wined3d_shader_decref(shader->wined3d_shader);
        wined3d_mutex_unlock();
    }

    return refcount;
}

static const IDirect3DPixelShader8Vtbl d3d8_pixelshader_vtbl =
{
    /* IUnknown */
    d3d8_pixelshader_QueryInterface,
    d3d8_pixelshader_AddRef,
    d3d8_pixelshader_Release,
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
    shader->IDirect3DPixelShader8_iface.lpVtbl = &d3d8_pixelshader_vtbl;
    shader->handle = shader_handle;

    wined3d_mutex_lock();
    hr = wined3d_shader_create_ps(device->wined3d_device, byte_code, NULL, shader,
            &d3d8_pixelshader_wined3d_parent_ops, &shader->wined3d_shader, 1);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d pixel shader, hr %#x.\n", hr);
        return hr;
    }

    return D3D_OK;
}
