/*
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Ivan Gyurdiev
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
 * Copyright 2009 Henri Verbeet for CodeWeavers
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

#include <math.h>
#include <stdio.h>

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_shader);

static void shader_get_parent(IWineD3DBaseShaderImpl *shader, IUnknown **parent)
{
    *parent = shader->baseShader.parent;
    IUnknown_AddRef(*parent);
    TRACE("shader %p, returning %p.\n", shader, *parent);
}

static HRESULT shader_get_function(IWineD3DBaseShaderImpl *shader, void *data, UINT *data_size)
{
    if (!data)
    {
        *data_size = shader->baseShader.functionLength;
        return WINED3D_OK;
    }

    if (*data_size < shader->baseShader.functionLength)
    {
        /* MSDN claims (for d3d8 at least) that if *pSizeOfData is smaller
         * than the required size we should write the required size and
         * return D3DERR_MOREDATA. That's not actually true. */
        return WINED3DERR_INVALIDCALL;
    }

    memcpy(data, shader->baseShader.function, shader->baseShader.functionLength);

    return WINED3D_OK;
}

static HRESULT shader_set_function(IWineD3DBaseShaderImpl *shader, const DWORD *byte_code,
        const struct wined3d_shader_signature *output_signature, DWORD float_const_count)
{
    struct shader_reg_maps *reg_maps = &shader->baseShader.reg_maps;
    const struct wined3d_shader_frontend *fe;
    HRESULT hr;

    TRACE("shader %p, byte_code %p, output_signature %p, float_const_count %u.\n",
            shader, byte_code, output_signature, float_const_count);

    fe = shader_select_frontend(*byte_code);
    if (!fe)
    {
        FIXME("Unable to find frontend for shader.\n");
        return WINED3DERR_INVALIDCALL;
    }
    shader->baseShader.frontend = fe;
    shader->baseShader.frontend_data = fe->shader_init(byte_code, output_signature);
    if (!shader->baseShader.frontend_data)
    {
        FIXME("Failed to initialize frontend.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* First pass: trace shader. */
    if (TRACE_ON(d3d_shader)) shader_trace_init(fe, shader->baseShader.frontend_data, byte_code);

    /* Initialize immediate constant lists. */
    list_init(&shader->baseShader.constantsF);
    list_init(&shader->baseShader.constantsB);
    list_init(&shader->baseShader.constantsI);

    /* Second pass: figure out which registers are used, what the semantics are, etc. */
    hr = shader_get_registers_used((IWineD3DBaseShader *)shader, fe,
            reg_maps, shader->baseShader.input_signature, shader->baseShader.output_signature,
            byte_code, float_const_count);
    if (FAILED(hr)) return hr;

    shader->baseShader.function = HeapAlloc(GetProcessHeap(), 0, shader->baseShader.functionLength);
    if (!shader->baseShader.function) return E_OUTOFMEMORY;
    memcpy(shader->baseShader.function, byte_code, shader->baseShader.functionLength);

    return WINED3D_OK;
}

static HRESULT STDMETHODCALLTYPE vertexshader_QueryInterface(IWineD3DVertexShader *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IWineD3DVertexShader)
            || IsEqualGUID(riid, &IID_IWineD3DBaseShader)
            || IsEqualGUID(riid, &IID_IWineD3DBase)
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

static ULONG STDMETHODCALLTYPE vertexshader_AddRef(IWineD3DVertexShader *iface)
{
    IWineD3DVertexShaderImpl *shader = (IWineD3DVertexShaderImpl *)iface;
    ULONG refcount = InterlockedIncrement(&shader->baseShader.ref);

    TRACE("%p increasing refcount to %u.\n", shader, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE vertexshader_Release(IWineD3DVertexShader *iface)
{
    IWineD3DVertexShaderImpl *shader = (IWineD3DVertexShaderImpl *)iface;
    ULONG refcount = InterlockedDecrement(&shader->baseShader.ref);

    TRACE("%p decreasing refcount to %u.\n", shader, refcount);

    if (!refcount)
    {
        shader_cleanup((IWineD3DBaseShader *)iface);
        shader->baseShader.parent_ops->wined3d_object_destroyed(shader->baseShader.parent);
        HeapFree(GetProcessHeap(), 0, shader);
    }

    return refcount;
}

static HRESULT STDMETHODCALLTYPE vertexshader_GetParent(IWineD3DVertexShader *iface, IUnknown **parent)
{
    TRACE("iface %p, parent %p.\n", iface, parent);

    shader_get_parent((IWineD3DBaseShaderImpl *)iface, parent);

    return WINED3D_OK;
}

static HRESULT STDMETHODCALLTYPE vertexshader_GetFunction(IWineD3DVertexShader *iface, void *data, UINT *data_size)
{
    TRACE("iface %p, data %p, data_size %p.\n", iface, data, data_size);

    return shader_get_function((IWineD3DBaseShaderImpl *)iface, data, data_size);
}

/* Set local constants for d3d8 shaders. */
static HRESULT STDMETHODCALLTYPE vertexshader_SetLocalConstantsF(IWineD3DVertexShader *iface,
        UINT start_idx, const float *src_data, UINT count)
{
    IWineD3DVertexShaderImpl *shader =(IWineD3DVertexShaderImpl *)iface;
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *)shader->baseShader.device;
    UINT i, end_idx;

    TRACE("iface %p, start_idx %u, src_data %p, count %u.\n", iface, start_idx, src_data, count);

    end_idx = start_idx + count;
    if (end_idx > device->d3d_vshader_constantF)
    {
        WARN("end_idx %u > float constants limit %u.\n", end_idx, device->d3d_vshader_constantF);
        end_idx = device->d3d_vshader_constantF;
    }

    for (i = start_idx; i < end_idx; ++i)
    {
        local_constant* lconst = HeapAlloc(GetProcessHeap(), 0, sizeof(local_constant));
        if (!lconst) return E_OUTOFMEMORY;

        lconst->idx = i;
        memcpy(lconst->value, src_data + (i - start_idx) * 4 /* 4 components */, 4 * sizeof(float));
        list_add_head(&shader->baseShader.constantsF, &lconst->entry);
    }

    return WINED3D_OK;
}

static const IWineD3DVertexShaderVtbl IWineD3DVertexShader_Vtbl =
{
    /* IUnknown methods */
    vertexshader_QueryInterface,
    vertexshader_AddRef,
    vertexshader_Release,
    /* IWineD3DBase methods */
    vertexshader_GetParent,
    /* IWineD3DBaseShader methods */
    vertexshader_GetFunction,
    /* IWineD3DVertexShader methods */
    vertexshader_SetLocalConstantsF,
};

void find_vs_compile_args(IWineD3DVertexShaderImpl *shader,
        IWineD3DStateBlockImpl *stateblock, struct vs_compile_args *args)
{
    args->fog_src = stateblock->renderState[WINED3DRS_FOGTABLEMODE] == WINED3DFOG_NONE ? VS_FOG_COORD : VS_FOG_Z;
    args->clip_enabled = stateblock->renderState[WINED3DRS_CLIPPING]
            && stateblock->renderState[WINED3DRS_CLIPPLANEENABLE];
    args->swizzle_map = ((IWineD3DDeviceImpl *)shader->baseShader.device)->strided_streams.swizzle_map;
}

static BOOL match_usage(BYTE usage1, BYTE usage_idx1, BYTE usage2, BYTE usage_idx2)
{
    if (usage_idx1 != usage_idx2) return FALSE;
    if (usage1 == usage2) return TRUE;
    if (usage1 == WINED3DDECLUSAGE_POSITION && usage2 == WINED3DDECLUSAGE_POSITIONT) return TRUE;
    if (usage2 == WINED3DDECLUSAGE_POSITION && usage1 == WINED3DDECLUSAGE_POSITIONT) return TRUE;

    return FALSE;
}

BOOL vshader_get_input(IWineD3DVertexShader *iface, BYTE usage_req, BYTE usage_idx_req, unsigned int *regnum)
{
    IWineD3DVertexShaderImpl *shader = (IWineD3DVertexShaderImpl *)iface;
    WORD map = shader->baseShader.reg_maps.input_registers;
    unsigned int i;

    for (i = 0; map; map >>= 1, ++i)
    {
        if (!(map & 1)) continue;

        if (match_usage(shader->attributes[i].usage,
                shader->attributes[i].usage_idx, usage_req, usage_idx_req))
        {
            *regnum = i;
            return TRUE;
        }
    }
    return FALSE;
}

static void vertexshader_set_limits(IWineD3DVertexShaderImpl *shader)
{
    DWORD shader_version = WINED3D_SHADER_VERSION(shader->baseShader.reg_maps.shader_version.major,
            shader->baseShader.reg_maps.shader_version.minor);
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *)shader->baseShader.device;

    shader->baseShader.limits.texcoord = 0;
    shader->baseShader.limits.attributes = 16;
    shader->baseShader.limits.packed_input = 0;

    switch (shader_version)
    {
        case WINED3D_SHADER_VERSION(1, 0):
        case WINED3D_SHADER_VERSION(1, 1):
            shader->baseShader.limits.temporary = 12;
            shader->baseShader.limits.constant_bool = 0;
            shader->baseShader.limits.constant_int = 0;
            shader->baseShader.limits.address = 1;
            shader->baseShader.limits.packed_output = 0;
            shader->baseShader.limits.sampler = 0;
            shader->baseShader.limits.label = 0;
            /* TODO: vs_1_1 has a minimum of 96 constants. What happens when
             * a vs_1_1 shader is used on a vs_3_0 capable card that has 256
             * constants? */
            shader->baseShader.limits.constant_float = min(256, device->d3d_vshader_constantF);
            break;

        case WINED3D_SHADER_VERSION(2, 0):
        case WINED3D_SHADER_VERSION(2, 1):
            shader->baseShader.limits.temporary = 12;
            shader->baseShader.limits.constant_bool = 16;
            shader->baseShader.limits.constant_int = 16;
            shader->baseShader.limits.address = 1;
            shader->baseShader.limits.packed_output = 0;
            shader->baseShader.limits.sampler = 0;
            shader->baseShader.limits.label = 16;
            shader->baseShader.limits.constant_float = min(256, device->d3d_vshader_constantF);
            break;

        case WINED3D_SHADER_VERSION(4, 0):
            FIXME("Using 3.0 limits for 4.0 shader.\n");
            /* Fall through. */

        case WINED3D_SHADER_VERSION(3, 0):
            shader->baseShader.limits.temporary = 32;
            shader->baseShader.limits.constant_bool = 32;
            shader->baseShader.limits.constant_int = 32;
            shader->baseShader.limits.address = 1;
            shader->baseShader.limits.packed_output = 12;
            shader->baseShader.limits.sampler = 4;
            shader->baseShader.limits.label = 16; /* FIXME: 2048 */
            /* DX10 cards on Windows advertise a d3d9 constant limit of 256
             * even though they are capable of supporting much more (GL
             * drivers advertise 1024). d3d9.dll and d3d8.dll clamp the
             * wined3d-advertised maximum. Clamp the constant limit for <= 3.0
             * shaders to 256. */
            shader->baseShader.limits.constant_float = min(256, device->d3d_vshader_constantF);
            break;

        default:
            shader->baseShader.limits.temporary = 12;
            shader->baseShader.limits.constant_bool = 16;
            shader->baseShader.limits.constant_int = 16;
            shader->baseShader.limits.address = 1;
            shader->baseShader.limits.packed_output = 0;
            shader->baseShader.limits.sampler = 0;
            shader->baseShader.limits.label = 16;
            shader->baseShader.limits.constant_float = min(256, device->d3d_vshader_constantF);
            FIXME("Unrecognized vertex shader version \"%u.%u\".\n",
                    shader->baseShader.reg_maps.shader_version.major,
                    shader->baseShader.reg_maps.shader_version.minor);
    }
}

HRESULT vertexshader_init(IWineD3DVertexShaderImpl *shader, IWineD3DDeviceImpl *device,
        const DWORD *byte_code, const struct wined3d_shader_signature *output_signature,
        IUnknown *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct shader_reg_maps *reg_maps = &shader->baseShader.reg_maps;
    unsigned int i;
    HRESULT hr;
    WORD map;

    if (!byte_code) return WINED3DERR_INVALIDCALL;

    shader->lpVtbl = &IWineD3DVertexShader_Vtbl;
    shader_init(&shader->baseShader, device, parent, parent_ops);

    hr = shader_set_function((IWineD3DBaseShaderImpl *)shader, byte_code,
            output_signature, device->d3d_vshader_constantF);
    if (FAILED(hr))
    {
        WARN("Failed to set function, hr %#x.\n", hr);
        shader_cleanup((IWineD3DBaseShader *)shader);
        return hr;
    }

    map = shader->baseShader.reg_maps.input_registers;
    for (i = 0; map; map >>= 1, ++i)
    {
        if (!(map & 1) || !shader->baseShader.input_signature[i].semantic_name) continue;

        shader->attributes[i].usage =
                shader_usage_from_semantic_name(shader->baseShader.input_signature[i].semantic_name);
        shader->attributes[i].usage_idx = shader->baseShader.input_signature[i].semantic_idx;
    }

    if (output_signature)
    {
        for (i = 0; i < output_signature->element_count; ++i)
        {
            struct wined3d_shader_signature_element *e = &output_signature->elements[i];
            reg_maps->output_registers |= 1 << e->register_idx;
            shader->baseShader.output_signature[e->register_idx] = *e;
        }
    }

    vertexshader_set_limits(shader);

    if (device->vs_selected_mode == SHADER_ARB
            && (gl_info->quirks & WINED3D_QUIRK_ARB_VS_OFFSET_LIMIT)
            && shader->min_rel_offset <= shader->max_rel_offset)
    {
        if (shader->max_rel_offset - shader->min_rel_offset > 127)
        {
            FIXME("The difference between the minimum and maximum relative offset is > 127.\n");
            FIXME("Which this OpenGL implementation does not support. Try using GLSL.\n");
            FIXME("Min: %d, Max: %d.\n", shader->min_rel_offset, shader->max_rel_offset);
        }
        else if (shader->max_rel_offset - shader->min_rel_offset > 63)
        {
            shader->rel_offset = shader->min_rel_offset + 63;
        }
        else if (shader->max_rel_offset > 63)
        {
            shader->rel_offset = shader->min_rel_offset;
        }
        else
        {
            shader->rel_offset = 0;
        }
    }

    shader->baseShader.load_local_constsF = shader->baseShader.reg_maps.usesrelconstF
            && !list_empty(&shader->baseShader.constantsF);

    return WINED3D_OK;
}

static HRESULT STDMETHODCALLTYPE geometryshader_QueryInterface(IWineD3DGeometryShader *iface,
        REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IWineD3DGeometryShader)
            || IsEqualGUID(riid, &IID_IWineD3DBaseShader)
            || IsEqualGUID(riid, &IID_IWineD3DBase)
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

static ULONG STDMETHODCALLTYPE geometryshader_AddRef(IWineD3DGeometryShader *iface)
{
    struct wined3d_geometryshader *shader = (struct wined3d_geometryshader *)iface;
    ULONG refcount = InterlockedIncrement(&shader->base_shader.ref);

    TRACE("%p increasing refcount to %u.\n", shader, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE geometryshader_Release(IWineD3DGeometryShader *iface)
{
    struct wined3d_geometryshader *shader = (struct wined3d_geometryshader *)iface;
    ULONG refcount = InterlockedDecrement(&shader->base_shader.ref);

    TRACE("%p decreasing refcount to %u.\n", shader, refcount);

    if (!refcount)
    {
        shader_cleanup((IWineD3DBaseShader *)iface);
        shader->base_shader.parent_ops->wined3d_object_destroyed(shader->base_shader.parent);
        HeapFree(GetProcessHeap(), 0, shader);
    }

    return refcount;
}

static HRESULT STDMETHODCALLTYPE geometryshader_GetParent(IWineD3DGeometryShader *iface, IUnknown **parent)
{
    TRACE("iface %p, parent %p.\n", iface, parent);

    shader_get_parent((IWineD3DBaseShaderImpl *)iface, parent);

    return WINED3D_OK;
}

static HRESULT STDMETHODCALLTYPE geometryshader_GetFunction(IWineD3DGeometryShader *iface, void *data, UINT *data_size)
{
    TRACE("iface %p, data %p, data_size %p.\n", iface, data, data_size);

    return shader_get_function((IWineD3DBaseShaderImpl *)iface, data, data_size);
}

static const IWineD3DGeometryShaderVtbl wined3d_geometryshader_vtbl =
{
    /* IUnknown methods */
    geometryshader_QueryInterface,
    geometryshader_AddRef,
    geometryshader_Release,
    /* IWineD3DBase methods */
    geometryshader_GetParent,
    /* IWineD3DBaseShader methods */
    geometryshader_GetFunction,
};

HRESULT geometryshader_init(struct wined3d_geometryshader *shader, IWineD3DDeviceImpl *device,
        const DWORD *byte_code, const struct wined3d_shader_signature *output_signature,
        IUnknown *parent, const struct wined3d_parent_ops *parent_ops)
{
    HRESULT hr;

    shader->vtbl = &wined3d_geometryshader_vtbl;
    shader_init(&shader->base_shader, device, parent, parent_ops);

    hr = shader_set_function((IWineD3DBaseShaderImpl *)shader, byte_code, output_signature, 0);
    if (FAILED(hr))
    {
        WARN("Failed to set function, hr %#x.\n", hr);
        shader_cleanup((IWineD3DBaseShader *)shader);
        return hr;
    }

    shader->base_shader.load_local_constsF = FALSE;

    return WINED3D_OK;
}

static HRESULT STDMETHODCALLTYPE pixelshader_QueryInterface(IWineD3DPixelShader *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IWineD3DPixelShader)
            || IsEqualGUID(riid, &IID_IWineD3DBaseShader)
            || IsEqualGUID(riid, &IID_IWineD3DBase)
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

static ULONG STDMETHODCALLTYPE pixelshader_AddRef(IWineD3DPixelShader *iface)
{
    IWineD3DPixelShaderImpl *shader = (IWineD3DPixelShaderImpl *)iface;
    ULONG refcount = InterlockedIncrement(&shader->baseShader.ref);

    TRACE("%p increasing refcount to %u.\n", shader, refcount);

    return refcount;
}

static ULONG STDMETHODCALLTYPE pixelshader_Release(IWineD3DPixelShader *iface)
{
    IWineD3DPixelShaderImpl *shader = (IWineD3DPixelShaderImpl *)iface;
    ULONG refcount = InterlockedDecrement(&shader->baseShader.ref);

    TRACE("%p decreasing refcount to %u.\n", shader, refcount);

    if (!refcount)
    {
        shader_cleanup((IWineD3DBaseShader *)iface);
        shader->baseShader.parent_ops->wined3d_object_destroyed(shader->baseShader.parent);
        HeapFree(GetProcessHeap(), 0, shader);
    }

    return refcount;
}

static HRESULT STDMETHODCALLTYPE pixelshader_GetParent(IWineD3DPixelShader *iface, IUnknown **parent)
{
    TRACE("iface %p, parent %p.\n", iface, parent);

    shader_get_parent((IWineD3DBaseShaderImpl *)iface, parent);

    return WINED3D_OK;
}

static HRESULT STDMETHODCALLTYPE pixelshader_GetFunction(IWineD3DPixelShader *iface, void *data, UINT *data_size)
{
    TRACE("iface %p, data %p, data_size %p.\n", iface, data, data_size);

    return shader_get_function((IWineD3DBaseShaderImpl *)iface, data, data_size);
}

static const IWineD3DPixelShaderVtbl IWineD3DPixelShader_Vtbl =
{
    /* IUnknown methods */
    pixelshader_QueryInterface,
    pixelshader_AddRef,
    pixelshader_Release,
    /* IWineD3DBase methods */
    pixelshader_GetParent,
    /* IWineD3DBaseShader methods */
    pixelshader_GetFunction
};

void find_ps_compile_args(IWineD3DPixelShaderImpl *shader,
        IWineD3DStateBlockImpl *stateblock, struct ps_compile_args *args)
{
    IWineD3DBaseTextureImpl *texture;
    UINT i;

    memset(args, 0, sizeof(*args)); /* FIXME: Make sure all bits are set. */
    args->srgb_correction = stateblock->renderState[WINED3DRS_SRGBWRITEENABLE] ? 1 : 0;
    args->np2_fixup = 0;

    for (i = 0; i < MAX_FRAGMENT_SAMPLERS; ++i)
    {
        if (!shader->baseShader.reg_maps.sampler_type[i]) continue;
        texture = (IWineD3DBaseTextureImpl *)stateblock->textures[i];
        if (!texture)
        {
            args->color_fixup[i] = COLOR_FIXUP_IDENTITY;
            continue;
        }
        args->color_fixup[i] = texture->resource.format_desc->color_fixup;

        /* Flag samplers that need NP2 texcoord fixup. */
        if (!texture->baseTexture.pow2Matrix_identity)
        {
            args->np2_fixup |= (1 << i);
        }
    }
    if (shader->baseShader.reg_maps.shader_version.major >= 3)
    {
        if (((IWineD3DDeviceImpl *)shader->baseShader.device)->strided_streams.position_transformed)
        {
            args->vp_mode = pretransformed;
        }
        else if (use_vs(stateblock))
        {
            args->vp_mode = vertexshader;
        }
        else
        {
            args->vp_mode = fixedfunction;
        }
        args->fog = FOG_OFF;
    }
    else
    {
        args->vp_mode = vertexshader;
        if (stateblock->renderState[WINED3DRS_FOGENABLE])
        {
            switch (stateblock->renderState[WINED3DRS_FOGTABLEMODE])
            {
                case WINED3DFOG_NONE:
                    if (((IWineD3DDeviceImpl *)shader->baseShader.device)->strided_streams.position_transformed
                            || use_vs(stateblock))
                    {
                        args->fog = FOG_LINEAR;
                        break;
                    }

                    switch (stateblock->renderState[WINED3DRS_FOGVERTEXMODE])
                    {
                        case WINED3DFOG_NONE: /* Fall through. */
                        case WINED3DFOG_LINEAR: args->fog = FOG_LINEAR; break;
                        case WINED3DFOG_EXP:    args->fog = FOG_EXP;    break;
                        case WINED3DFOG_EXP2:   args->fog = FOG_EXP2;   break;
                    }
                    break;

                case WINED3DFOG_LINEAR: args->fog = FOG_LINEAR; break;
                case WINED3DFOG_EXP:    args->fog = FOG_EXP;    break;
                case WINED3DFOG_EXP2:   args->fog = FOG_EXP2;   break;
            }
        }
        else
        {
            args->fog = FOG_OFF;
        }
    }
}

static void pixelshader_set_limits(IWineD3DPixelShaderImpl *shader)
{
    DWORD shader_version = WINED3D_SHADER_VERSION(shader->baseShader.reg_maps.shader_version.major,
            shader->baseShader.reg_maps.shader_version.minor);

    shader->baseShader.limits.attributes = 0;
    shader->baseShader.limits.address = 0;
    shader->baseShader.limits.packed_output = 0;

    switch (shader_version)
    {
        case WINED3D_SHADER_VERSION(1, 0):
        case WINED3D_SHADER_VERSION(1, 1):
        case WINED3D_SHADER_VERSION(1, 2):
        case WINED3D_SHADER_VERSION(1, 3):
            shader->baseShader.limits.temporary = 2;
            shader->baseShader.limits.constant_float = 8;
            shader->baseShader.limits.constant_int = 0;
            shader->baseShader.limits.constant_bool = 0;
            shader->baseShader.limits.texcoord = 4;
            shader->baseShader.limits.sampler = 4;
            shader->baseShader.limits.packed_input = 0;
            shader->baseShader.limits.label = 0;
            break;

        case WINED3D_SHADER_VERSION(1, 4):
            shader->baseShader.limits.temporary = 6;
            shader->baseShader.limits.constant_float = 8;
            shader->baseShader.limits.constant_int = 0;
            shader->baseShader.limits.constant_bool = 0;
            shader->baseShader.limits.texcoord = 6;
            shader->baseShader.limits.sampler = 6;
            shader->baseShader.limits.packed_input = 0;
            shader->baseShader.limits.label = 0;
            break;

        /* FIXME: Temporaries must match D3DPSHADERCAPS2_0.NumTemps. */
        case WINED3D_SHADER_VERSION(2, 0):
            shader->baseShader.limits.temporary = 32;
            shader->baseShader.limits.constant_float = 32;
            shader->baseShader.limits.constant_int = 16;
            shader->baseShader.limits.constant_bool = 16;
            shader->baseShader.limits.texcoord = 8;
            shader->baseShader.limits.sampler = 16;
            shader->baseShader.limits.packed_input = 0;
            break;

        case WINED3D_SHADER_VERSION(2, 1):
            shader->baseShader.limits.temporary = 32;
            shader->baseShader.limits.constant_float = 32;
            shader->baseShader.limits.constant_int = 16;
            shader->baseShader.limits.constant_bool = 16;
            shader->baseShader.limits.texcoord = 8;
            shader->baseShader.limits.sampler = 16;
            shader->baseShader.limits.packed_input = 0;
            shader->baseShader.limits.label = 16;
            break;

        case WINED3D_SHADER_VERSION(4, 0):
            FIXME("Using 3.0 limits for 4.0 shader.\n");
            /* Fall through. */

        case WINED3D_SHADER_VERSION(3, 0):
            shader->baseShader.limits.temporary = 32;
            shader->baseShader.limits.constant_float = 224;
            shader->baseShader.limits.constant_int = 16;
            shader->baseShader.limits.constant_bool = 16;
            shader->baseShader.limits.texcoord = 0;
            shader->baseShader.limits.sampler = 16;
            shader->baseShader.limits.packed_input = 12;
            shader->baseShader.limits.label = 16; /* FIXME: 2048 */
            break;

        default:
            shader->baseShader.limits.temporary = 32;
            shader->baseShader.limits.constant_float = 32;
            shader->baseShader.limits.constant_int = 16;
            shader->baseShader.limits.constant_bool = 16;
            shader->baseShader.limits.texcoord = 8;
            shader->baseShader.limits.sampler = 16;
            shader->baseShader.limits.packed_input = 0;
            shader->baseShader.limits.label = 0;
            FIXME("Unrecognized pixel shader version %u.%u\n",
                    shader->baseShader.reg_maps.shader_version.major,
                    shader->baseShader.reg_maps.shader_version.minor);
    }
}

HRESULT pixelshader_init(IWineD3DPixelShaderImpl *shader, IWineD3DDeviceImpl *device,
        const DWORD *byte_code, const struct wined3d_shader_signature *output_signature,
        IUnknown *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    unsigned int i, highest_reg_used = 0, num_regs_used = 0;
    HRESULT hr;

    if (!byte_code) return WINED3DERR_INVALIDCALL;

    shader->lpVtbl = &IWineD3DPixelShader_Vtbl;
    shader_init(&shader->baseShader, device, parent, parent_ops);

    hr = shader_set_function((IWineD3DBaseShaderImpl *)shader, byte_code,
            output_signature, device->d3d_pshader_constantF);
    if (FAILED(hr))
    {
        WARN("Failed to set function, hr %#x.\n", hr);
        shader_cleanup((IWineD3DBaseShader *)shader);
        return hr;
    }

    pixelshader_set_limits(shader);

    for (i = 0; i < MAX_REG_INPUT; ++i)
    {
        if (shader->input_reg_used[i])
        {
            ++num_regs_used;
            highest_reg_used = i;
        }
    }

    /* Don't do any register mapping magic if it is not needed, or if we can't
     * achieve anything anyway */
    if (highest_reg_used < (gl_info->limits.glsl_varyings / 4)
            || num_regs_used > (gl_info->limits.glsl_varyings / 4))
    {
        if (num_regs_used > (gl_info->limits.glsl_varyings / 4))
        {
            /* This happens with relative addressing. The input mapper function
             * warns about this if the higher registers are declared too, so
             * don't write a FIXME here */
            WARN("More varying registers used than supported\n");
        }

        for (i = 0; i < MAX_REG_INPUT; ++i)
        {
            shader->input_reg_map[i] = i;
        }

        shader->declared_in_count = highest_reg_used + 1;
    }
    else
    {
        shader->declared_in_count = 0;
        for (i = 0; i < MAX_REG_INPUT; ++i)
        {
            if (shader->input_reg_used[i]) shader->input_reg_map[i] = shader->declared_in_count++;
            else shader->input_reg_map[i] = ~0U;
        }
    }

    shader->baseShader.load_local_constsF = FALSE;

    return WINED3D_OK;
}

void pixelshader_update_samplers(struct shader_reg_maps *reg_maps, IWineD3DBaseTexture * const *textures)
{
    WINED3DSAMPLER_TEXTURE_TYPE *sampler_type = reg_maps->sampler_type;
    unsigned int i;

    if (reg_maps->shader_version.major != 1) return;

    for (i = 0; i < max(MAX_FRAGMENT_SAMPLERS, MAX_VERTEX_SAMPLERS); ++i)
    {
        /* We don't sample from this sampler. */
        if (!sampler_type[i]) continue;

        if (!textures[i])
        {
            WARN("No texture bound to sampler %u, using 2D.\n", i);
            sampler_type[i] = WINED3DSTT_2D;
            continue;
        }

        switch (IWineD3DBaseTexture_GetTextureDimensions(textures[i]))
        {
            case GL_TEXTURE_RECTANGLE_ARB:
            case GL_TEXTURE_2D:
                /* We have to select between texture rectangles and 2D
                 * textures later because 2.0 and 3.0 shaders only have
                 * WINED3DSTT_2D as well. */
                sampler_type[i] = WINED3DSTT_2D;
                break;

            case GL_TEXTURE_3D:
                sampler_type[i] = WINED3DSTT_VOLUME;
                break;

            case GL_TEXTURE_CUBE_MAP_ARB:
                sampler_type[i] = WINED3DSTT_CUBE;
                break;

            default:
                FIXME("Unrecognized texture type %#x, using 2D.\n",
                        IWineD3DBaseTexture_GetTextureDimensions(textures[i]));
                sampler_type[i] = WINED3DSTT_2D;
        }
    }
}
