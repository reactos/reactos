/*
 * IDirect3DVertexShader8 implementation
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

/* IDirect3DVertexShader8 IUnknown parts follow: */
static HRESULT WINAPI IDirect3DVertexShader8Impl_QueryInterface(IDirect3DVertexShader8 *iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DVertexShader8Impl *This = (IDirect3DVertexShader8Impl *)iface;

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DVertexShader8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DVertexShader8Impl_AddRef(IDirect3DVertexShader8 *iface) {
    IDirect3DVertexShader8Impl *This = (IDirect3DVertexShader8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    if (ref == 1 && This->wineD3DVertexShader)
    {
        wined3d_mutex_lock();
        IWineD3DVertexShader_AddRef(This->wineD3DVertexShader);
        wined3d_mutex_unlock();
    }

    return ref;
}

static void STDMETHODCALLTYPE d3d8_vertexshader_wined3d_object_destroyed(void *parent)
{
    IDirect3DVertexShader8Impl *shader = parent;
    IDirect3DVertexDeclaration8_Release(shader->vertex_declaration);
    HeapFree(GetProcessHeap(), 0, shader);
}

static ULONG WINAPI IDirect3DVertexShader8Impl_Release(IDirect3DVertexShader8 *iface) {
    IDirect3DVertexShader8Impl *This = (IDirect3DVertexShader8Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (ref == 0) {
        if (This->wineD3DVertexShader)
        {
            wined3d_mutex_lock();
            IWineD3DVertexShader_Release(This->wineD3DVertexShader);
            wined3d_mutex_unlock();
        }
        else
        {
            d3d8_vertexshader_wined3d_object_destroyed(This);
        }
    }
    return ref;
}

static const IDirect3DVertexShader8Vtbl Direct3DVertexShader8_Vtbl =
{
    /* IUnknown */
    IDirect3DVertexShader8Impl_QueryInterface,
    IDirect3DVertexShader8Impl_AddRef,
    IDirect3DVertexShader8Impl_Release,
};

static const struct wined3d_parent_ops d3d8_vertexshader_wined3d_parent_ops =
{
    d3d8_vertexshader_wined3d_object_destroyed,
};

static HRESULT vertexshader_create_vertexdeclaration(IDirect3DDevice8Impl *device,
        const DWORD *declaration, DWORD shader_handle, IDirect3DVertexDeclaration8 **decl_ptr)
{
    IDirect3DVertexDeclaration8Impl *object;
    HRESULT hr;

    TRACE("device %p, declaration %p, shader_handle %#x, decl_ptr %p.\n",
            device, declaration, shader_handle, decl_ptr);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object) {
        ERR("Memory allocation failed\n");
        *decl_ptr = NULL;
        return D3DERR_OUTOFVIDEOMEMORY;
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

    /* Test if the vertex declaration is valid */
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
    shader->lpVtbl = &Direct3DVertexShader8_Vtbl;

    hr = vertexshader_create_vertexdeclaration(device, declaration, shader_handle, &shader->vertex_declaration);
    if (FAILED(hr))
    {
        WARN("Failed to create vertex declaration, hr %#x.\n", hr);
        return hr;
    }

    if (byte_code)
    {
        if (usage) FIXME("Usage %#x not implemented.\n", usage);

        wined3d_mutex_lock();
        hr = IWineD3DDevice_CreateVertexShader(device->WineD3DDevice, byte_code,
                NULL /* output signature */, &shader->wineD3DVertexShader,
                (IUnknown *)shader, &d3d8_vertexshader_wined3d_parent_ops);
        wined3d_mutex_unlock();
        if (FAILED(hr))
        {
            WARN("Failed to create wined3d vertex shader, hr %#x.\n", hr);
            IDirect3DVertexDeclaration8_Release(shader->vertex_declaration);
            return hr;
        }

        load_local_constants(declaration, shader->wineD3DVertexShader);
    }

    return D3D_OK;
}
