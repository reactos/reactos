/*
 * shaders implementation
 *
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006 Ivan Gyurdiev
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
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

#define GLINFO_LOCATION ((IWineD3DDeviceImpl *) This->baseShader.device)->adapter->gl_info

static HRESULT  WINAPI IWineD3DPixelShaderImpl_QueryInterface(IWineD3DPixelShader *iface, REFIID riid, LPVOID *ppobj) {
    TRACE("iface %p, riid %s, ppobj %p\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IWineD3DPixelShader)
            || IsEqualGUID(riid, &IID_IWineD3DBaseShader)
            || IsEqualGUID(riid, &IID_IWineD3DBase)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *ppobj = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE\n", debugstr_guid(riid));

    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG  WINAPI IWineD3DPixelShaderImpl_AddRef(IWineD3DPixelShader *iface) {
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    ULONG refcount = InterlockedIncrement(&This->baseShader.ref);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    return refcount;
}

static ULONG  WINAPI IWineD3DPixelShaderImpl_Release(IWineD3DPixelShader *iface) {
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    ULONG refcount = InterlockedDecrement(&This->baseShader.ref);

    TRACE("%p decreasing refcount to %u\n", This, refcount);

    if (!refcount)
    {
        shader_cleanup((IWineD3DBaseShader *)iface);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refcount;
}

/* *******************************************
   IWineD3DPixelShader IWineD3DPixelShader parts follow
   ******************************************* */

static HRESULT  WINAPI IWineD3DPixelShaderImpl_GetParent(IWineD3DPixelShader *iface, IUnknown** parent){
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;

    *parent = This->parent;
    IUnknown_AddRef(*parent);
    TRACE("(%p) : returning %p\n", This, *parent);
    return WINED3D_OK;
}

static HRESULT  WINAPI IWineD3DPixelShaderImpl_GetDevice(IWineD3DPixelShader* iface, IWineD3DDevice **pDevice){
    IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)iface;
    IWineD3DDevice_AddRef(This->baseShader.device);
    *pDevice = This->baseShader.device;
    TRACE("(%p) returning %p\n", This, *pDevice);
    return WINED3D_OK;
}


static HRESULT  WINAPI IWineD3DPixelShaderImpl_GetFunction(IWineD3DPixelShader* impl, VOID* pData, UINT* pSizeOfData) {
  IWineD3DPixelShaderImpl *This = (IWineD3DPixelShaderImpl *)impl;
  TRACE("(%p) : pData(%p), pSizeOfData(%p)\n", This, pData, pSizeOfData);

  if (NULL == pData) {
    *pSizeOfData = This->baseShader.functionLength;
    return WINED3D_OK;
  }
  if (*pSizeOfData < This->baseShader.functionLength) {
    /* MSDN claims (for d3d8 at least) that if *pSizeOfData is smaller
     * than the required size we should write the required size and
     * return D3DERR_MOREDATA. That's not actually true. */
    return WINED3DERR_INVALIDCALL;
  }

  TRACE("(%p) : GetFunction copying to %p\n", This, pData);
  memcpy(pData, This->baseShader.function, This->baseShader.functionLength);

  return WINED3D_OK;
}

static void pshader_set_limits(IWineD3DPixelShaderImpl *This)
{
    DWORD shader_version = WINED3D_SHADER_VERSION(This->baseShader.reg_maps.shader_version.major,
            This->baseShader.reg_maps.shader_version.minor);

    This->baseShader.limits.attributes = 0;
    This->baseShader.limits.address = 0;
    This->baseShader.limits.packed_output = 0;

    switch (shader_version)
    {
        case WINED3D_SHADER_VERSION(1,0):
        case WINED3D_SHADER_VERSION(1,1):
        case WINED3D_SHADER_VERSION(1,2):
        case WINED3D_SHADER_VERSION(1,3):
            This->baseShader.limits.temporary = 2;
            This->baseShader.limits.constant_float = 8;
            This->baseShader.limits.constant_int = 0;
            This->baseShader.limits.constant_bool = 0;
            This->baseShader.limits.texcoord = 4;
            This->baseShader.limits.sampler = 4;
            This->baseShader.limits.packed_input = 0;
            This->baseShader.limits.label = 0;
            break;

        case WINED3D_SHADER_VERSION(1,4):
            This->baseShader.limits.temporary = 6;
            This->baseShader.limits.constant_float = 8;
            This->baseShader.limits.constant_int = 0;
            This->baseShader.limits.constant_bool = 0;
            This->baseShader.limits.texcoord = 6;
            This->baseShader.limits.sampler = 6;
            This->baseShader.limits.packed_input = 0;
            This->baseShader.limits.label = 0;
            break;

        /* FIXME: temporaries must match D3DPSHADERCAPS2_0.NumTemps */
        case WINED3D_SHADER_VERSION(2,0):
            This->baseShader.limits.temporary = 32;
            This->baseShader.limits.constant_float = 32;
            This->baseShader.limits.constant_int = 16;
            This->baseShader.limits.constant_bool = 16;
            This->baseShader.limits.texcoord = 8;
            This->baseShader.limits.sampler = 16;
            This->baseShader.limits.packed_input = 0;
            break;

        case WINED3D_SHADER_VERSION(2,1):
            This->baseShader.limits.temporary = 32;
            This->baseShader.limits.constant_float = 32;
            This->baseShader.limits.constant_int = 16;
            This->baseShader.limits.constant_bool = 16;
            This->baseShader.limits.texcoord = 8;
            This->baseShader.limits.sampler = 16;
            This->baseShader.limits.packed_input = 0;
            This->baseShader.limits.label = 16;
            break;

        case WINED3D_SHADER_VERSION(3,0):
            This->baseShader.limits.temporary = 32;
            This->baseShader.limits.constant_float = 224;
            This->baseShader.limits.constant_int = 16;
            This->baseShader.limits.constant_bool = 16;
            This->baseShader.limits.texcoord = 0;
            This->baseShader.limits.sampler = 16;
            This->baseShader.limits.packed_input = 12;
            This->baseShader.limits.label = 16; /* FIXME: 2048 */
            break;

        default:
            This->baseShader.limits.temporary = 32;
            This->baseShader.limits.constant_float = 32;
            This->baseShader.limits.constant_int = 16;
            This->baseShader.limits.constant_bool = 16;
            This->baseShader.limits.texcoord = 8;
            This->baseShader.limits.sampler = 16;
            This->baseShader.limits.packed_input = 0;
            This->baseShader.limits.label = 0;
            FIXME("Unrecognized pixel shader version %u.%u\n",
                    This->baseShader.reg_maps.shader_version.major,
                    This->baseShader.reg_maps.shader_version.minor);
    }
}

static HRESULT WINAPI IWineD3DPixelShaderImpl_SetFunction(IWineD3DPixelShader *iface,
        const DWORD *pFunction, const struct wined3d_shader_signature *output_signature)
{
    IWineD3DPixelShaderImpl *This =(IWineD3DPixelShaderImpl *)iface;
    unsigned int i, highest_reg_used = 0, num_regs_used = 0;
    shader_reg_maps *reg_maps = &This->baseShader.reg_maps;
    const struct wined3d_shader_frontend *fe;
    HRESULT hr;

    TRACE("(%p) : pFunction %p\n", iface, pFunction);

    fe = shader_select_frontend(*pFunction);
    if (!fe)
    {
        FIXME("Unable to find frontend for shader.\n");
        return WINED3DERR_INVALIDCALL;
    }
    This->baseShader.frontend = fe;
    This->baseShader.frontend_data = fe->shader_init(pFunction, output_signature);
    if (!This->baseShader.frontend_data)
    {
        FIXME("Failed to initialize frontend.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* First pass: trace shader */
    if (TRACE_ON(d3d_shader)) shader_trace_init(fe, This->baseShader.frontend_data, pFunction);

    /* Initialize immediate constant lists */
    list_init(&This->baseShader.constantsF);
    list_init(&This->baseShader.constantsB);
    list_init(&This->baseShader.constantsI);

    /* Second pass: figure out which registers are used, what the semantics are, etc.. */
    hr = shader_get_registers_used((IWineD3DBaseShader *)This, fe, reg_maps, This->semantics_in, NULL, pFunction,
                                    GL_LIMITS(pshader_constantsF));
    if (FAILED(hr)) return hr;

    pshader_set_limits(This);

    for (i = 0; i < MAX_REG_INPUT; ++i)
    {
        if (This->input_reg_used[i])
        {
            ++num_regs_used;
            highest_reg_used = i;
        }
    }

    /* Don't do any register mapping magic if it is not needed, or if we can't
     * achieve anything anyway */
    if (highest_reg_used < (GL_LIMITS(glsl_varyings) / 4)
            || num_regs_used > (GL_LIMITS(glsl_varyings) / 4))
    {
        if (num_regs_used > (GL_LIMITS(glsl_varyings) / 4))
        {
            /* This happens with relative addressing. The input mapper function
             * warns about this if the higher registers are declared too, so
             * don't write a FIXME here */
            WARN("More varying registers used than supported\n");
        }

        for (i = 0; i < MAX_REG_INPUT; ++i)
        {
            This->input_reg_map[i] = i;
        }

        This->declared_in_count = highest_reg_used + 1;
    }
    else
    {
        This->declared_in_count = 0;
        for (i = 0; i < MAX_REG_INPUT; ++i)
        {
            if (This->input_reg_used[i]) This->input_reg_map[i] = This->declared_in_count++;
            else This->input_reg_map[i] = ~0U;
        }
    }

    This->baseShader.load_local_constsF = FALSE;

    TRACE("(%p) : Copying the function\n", This);

    This->baseShader.function = HeapAlloc(GetProcessHeap(), 0, This->baseShader.functionLength);
    if (!This->baseShader.function) return E_OUTOFMEMORY;
    memcpy(This->baseShader.function, pFunction, This->baseShader.functionLength);

    return WINED3D_OK;
}

static void pixelshader_update_samplers(struct shader_reg_maps *reg_maps, IWineD3DBaseTexture * const *textures)
{
    WINED3DSAMPLER_TEXTURE_TYPE *sampler_type = reg_maps->sampler_type;
    unsigned int i;

    if (reg_maps->shader_version.major != 1) return;

    for (i = 0; i < max(MAX_FRAGMENT_SAMPLERS, MAX_VERTEX_SAMPLERS); ++i)
    {
        /* We don't sample from this sampler */
        if (!sampler_type[i]) continue;

        if (!textures[i])
        {
            ERR("No texture bound to sampler %u, using 2D\n", i);
            sampler_type[i] = WINED3DSTT_2D;
            continue;
        }

        switch (IWineD3DBaseTexture_GetTextureDimensions(textures[i]))
        {
            case GL_TEXTURE_RECTANGLE_ARB:
            case GL_TEXTURE_2D:
                /* We have to select between texture rectangles and 2D textures later because 2.0 and
                 * 3.0 shaders only have WINED3DSTT_2D as well */
                sampler_type[i] = WINED3DSTT_2D;
                break;

            case GL_TEXTURE_3D:
                sampler_type[i] = WINED3DSTT_VOLUME;
                break;

            case GL_TEXTURE_CUBE_MAP_ARB:
                sampler_type[i] = WINED3DSTT_CUBE;
                break;

            default:
                FIXME("Unrecognized texture type %#x, using 2D\n",
                        IWineD3DBaseTexture_GetTextureDimensions(textures[i]));
                sampler_type[i] = WINED3DSTT_2D;
        }
    }
}

static GLuint pixelshader_compile(IWineD3DPixelShaderImpl *This, const struct ps_compile_args *args)
{
    CONST DWORD *function = This->baseShader.function;
    GLuint retval;
    SHADER_BUFFER buffer;
    IWineD3DDeviceImpl *device = (IWineD3DDeviceImpl *) This->baseShader.device;

    TRACE("(%p) : function %p\n", This, function);

    pixelshader_update_samplers(&This->baseShader.reg_maps,
            ((IWineD3DDeviceImpl *)This->baseShader.device)->stateBlock->textures);

    /* Generate the HW shader */
    TRACE("(%p) : Generating hardware program\n", This);
    This->cur_args = args;
    shader_buffer_init(&buffer);
    retval = device->shader_backend->shader_generate_pshader((IWineD3DPixelShader *)This, &buffer, args);
    shader_buffer_free(&buffer);
    This->cur_args = NULL;

    return retval;
}

const IWineD3DPixelShaderVtbl IWineD3DPixelShader_Vtbl =
{
    /*** IUnknown methods ***/
    IWineD3DPixelShaderImpl_QueryInterface,
    IWineD3DPixelShaderImpl_AddRef,
    IWineD3DPixelShaderImpl_Release,
    /*** IWineD3DBase methods ***/
    IWineD3DPixelShaderImpl_GetParent,
    /*** IWineD3DBaseShader methods ***/
    IWineD3DPixelShaderImpl_SetFunction,
    /*** IWineD3DPixelShader methods ***/
    IWineD3DPixelShaderImpl_GetDevice,
    IWineD3DPixelShaderImpl_GetFunction
};

void find_ps_compile_args(IWineD3DPixelShaderImpl *shader, IWineD3DStateBlockImpl *stateblock, struct ps_compile_args *args) {
    UINT i;
    IWineD3DBaseTextureImpl *tex;

    memset(args, 0, sizeof(*args)); /* FIXME: Make sure all bits are set */
    args->srgb_correction = stateblock->renderState[WINED3DRS_SRGBWRITEENABLE] ? 1 : 0;
    args->np2_fixup = 0;

    for(i = 0; i < MAX_FRAGMENT_SAMPLERS; i++) {
        if (!shader->baseShader.reg_maps.sampler_type[i]) continue;
        tex = (IWineD3DBaseTextureImpl *) stateblock->textures[i];
        if(!tex) {
            args->color_fixup[i] = COLOR_FIXUP_IDENTITY;
            continue;
        }
        args->color_fixup[i] = tex->resource.format_desc->color_fixup;

        /* Flag samplers that need NP2 texcoord fixup. */
        if(!tex->baseTexture.pow2Matrix_identity) {
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
        } else {
            args->vp_mode = fixedfunction;
        }
        args->fog = FOG_OFF;
    } else {
        args->vp_mode = vertexshader;
        if(stateblock->renderState[WINED3DRS_FOGENABLE]) {
            switch(stateblock->renderState[WINED3DRS_FOGTABLEMODE]) {
                case WINED3DFOG_NONE:
                    if (((IWineD3DDeviceImpl *)shader->baseShader.device)->strided_streams.position_transformed
                            || use_vs(stateblock))
                    {
                        args->fog = FOG_LINEAR;
                        break;
                    }
                    switch(stateblock->renderState[WINED3DRS_FOGVERTEXMODE]) {
                        case WINED3DFOG_NONE: /* Drop through */
                        case WINED3DFOG_LINEAR: args->fog = FOG_LINEAR; break;
                        case WINED3DFOG_EXP:    args->fog = FOG_EXP;    break;
                        case WINED3DFOG_EXP2:   args->fog = FOG_EXP2;   break;
                    }
                    break;

                case WINED3DFOG_LINEAR: args->fog = FOG_LINEAR; break;
                case WINED3DFOG_EXP:    args->fog = FOG_EXP;    break;
                case WINED3DFOG_EXP2:   args->fog = FOG_EXP2;   break;
            }
        } else {
            args->fog = FOG_OFF;
        }
    }
}

GLuint find_gl_pshader(IWineD3DPixelShaderImpl *shader, const struct ps_compile_args *args)
{
    UINT i;
    DWORD new_size;
    struct ps_compiled_shader *new_array;

    /* Usually we have very few GL shaders for each d3d shader(just 1 or maybe 2),
     * so a linear search is more performant than a hashmap or a binary search
     * (cache coherency etc)
     */
    for(i = 0; i < shader->num_gl_shaders; i++) {
        if(memcmp(&shader->gl_shaders[i].args, args, sizeof(*args)) == 0) {
            return shader->gl_shaders[i].prgId;
        }
    }

    TRACE("No matching GL shader found, compiling a new shader\n");
    if(shader->shader_array_size == shader->num_gl_shaders) {
        if (shader->num_gl_shaders)
        {
            new_size = shader->shader_array_size + max(1, shader->shader_array_size / 2);
            new_array = HeapReAlloc(GetProcessHeap(), 0, shader->gl_shaders,
                                    new_size * sizeof(*shader->gl_shaders));
        } else {
            new_array = HeapAlloc(GetProcessHeap(), 0, sizeof(*shader->gl_shaders));
            new_size = 1;
        }

        if(!new_array) {
            ERR("Out of memory\n");
            return 0;
        }
        shader->gl_shaders = new_array;
        shader->shader_array_size = new_size;
    }

    shader->gl_shaders[shader->num_gl_shaders].args = *args;
    shader->gl_shaders[shader->num_gl_shaders].prgId = pixelshader_compile(shader, args);
    return shader->gl_shaders[shader->num_gl_shaders++].prgId;
}
