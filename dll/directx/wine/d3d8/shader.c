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

#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

static void STDMETHODCALLTYPE d3d8_vertexshader_wined3d_object_destroyed(void *parent)
{
    struct d3d8_vertex_shader *shader = parent;
    d3d8_vertex_declaration_destroy(shader->vertex_declaration);
    free(shader);
}

void d3d8_vertex_shader_destroy(struct d3d8_vertex_shader *shader)
{
    TRACE("shader %p.\n", shader);

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

static const struct wined3d_parent_ops d3d8_vertexshader_wined3d_parent_ops =
{
    d3d8_vertexshader_wined3d_object_destroyed,
};

static HRESULT d3d8_vertexshader_create_vertexdeclaration(struct d3d8_device *device,
        const DWORD *declaration, DWORD shader_handle, struct d3d8_vertex_declaration **decl_ptr)
{
    struct d3d8_vertex_declaration *object;
    HRESULT hr;

    TRACE("device %p, declaration %p, shader_handle %#lx, decl_ptr %p.\n",
            device, declaration, shader_handle, decl_ptr);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    hr = d3d8_vertex_declaration_init(object, device, declaration, shader_handle);
    if (FAILED(hr))
    {
        WARN("Failed to initialize vertex declaration, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created vertex declaration %p.\n", object);
    *decl_ptr = object;

    return D3D_OK;
}

HRESULT d3d8_vertex_shader_init(struct d3d8_vertex_shader *shader, struct d3d8_device *device,
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
                WARN("Attempt to use a non-FLOAT3 normal with the fixed-function function\n");
                return D3DERR_INVALIDCALL;
            }
        }
        token += parse_token(token);
    }

    hr = d3d8_vertexshader_create_vertexdeclaration(device, declaration, shader_handle, &shader->vertex_declaration);
    if (FAILED(hr))
    {
        WARN("Failed to create vertex declaration, hr %#lx.\n", hr);
        return hr;
    }

    if (byte_code)
    {
        struct wined3d_shader_desc desc;

        if (usage)
            FIXME("Usage %#lx not implemented.\n", usage);

        desc.byte_code = byte_code;
        desc.byte_code_size = ~(size_t)0;

        wined3d_mutex_lock();
        hr = wined3d_shader_create_vs(device->wined3d_device, &desc, shader,
                &d3d8_vertexshader_wined3d_parent_ops, &shader->wined3d_shader);
        wined3d_mutex_unlock();
        if (FAILED(hr))
        {
            WARN("Failed to create wined3d vertex shader, hr %#lx.\n", hr);
            d3d8_vertex_declaration_destroy(shader->vertex_declaration);
            return hr;
        }

        load_local_constants(declaration, shader->wined3d_shader);
    }

    return D3D_OK;
}

static void STDMETHODCALLTYPE d3d8_pixelshader_wined3d_object_destroyed(void *parent)
{
    free(parent);
}

void d3d8_pixel_shader_destroy(struct d3d8_pixel_shader *shader)
{
    TRACE("shader %p.\n", shader);

    wined3d_mutex_lock();
    wined3d_shader_decref(shader->wined3d_shader);
    wined3d_mutex_unlock();
}

static const struct wined3d_parent_ops d3d8_pixelshader_wined3d_parent_ops =
{
    d3d8_pixelshader_wined3d_object_destroyed,
};

HRESULT d3d8_pixel_shader_init(struct d3d8_pixel_shader *shader, struct d3d8_device *device,
        const DWORD *byte_code, DWORD shader_handle)
{
    struct wined3d_shader_desc desc;
    HRESULT hr;

    shader->handle = shader_handle;

    desc.byte_code = byte_code;
    desc.byte_code_size = ~(size_t)0;

    wined3d_mutex_lock();
    hr = wined3d_shader_create_ps(device->wined3d_device, &desc, shader,
            &d3d8_pixelshader_wined3d_parent_ops, &shader->wined3d_shader);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d pixel shader, hr %#lx.\n", hr);
        return hr;
    }

    return D3D_OK;
}
