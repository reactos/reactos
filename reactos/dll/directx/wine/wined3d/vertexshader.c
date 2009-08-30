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

#define GLINFO_LOCATION ((IWineD3DDeviceImpl *)This->baseShader.device)->adapter->gl_info

static void vshader_set_limits(IWineD3DVertexShaderImpl *This)
{
    DWORD shader_version = WINED3D_SHADER_VERSION(This->baseShader.reg_maps.shader_version.major,
            This->baseShader.reg_maps.shader_version.minor);

    This->baseShader.limits.texcoord = 0;
    This->baseShader.limits.attributes = 16;
    This->baseShader.limits.packed_input = 0;

    switch (shader_version)
    {
        case WINED3D_SHADER_VERSION(1,0):
        case WINED3D_SHADER_VERSION(1,1):
            This->baseShader.limits.temporary = 12;
            This->baseShader.limits.constant_bool = 0;
            This->baseShader.limits.constant_int = 0;
            This->baseShader.limits.address = 1;
            This->baseShader.limits.packed_output = 0;
            This->baseShader.limits.sampler = 0;
            This->baseShader.limits.label = 0;
            /* TODO: vs_1_1 has a minimum of 96 constants. What happens if a vs_1_1 shader is used
             * on a vs_3_0 capable card that has 256 constants? */
            This->baseShader.limits.constant_float = min(256, GL_LIMITS(vshader_constantsF));
            break;

        case WINED3D_SHADER_VERSION(2,0):
        case WINED3D_SHADER_VERSION(2,1):
            This->baseShader.limits.temporary = 12;
            This->baseShader.limits.constant_bool = 16;
            This->baseShader.limits.constant_int = 16;
            This->baseShader.limits.address = 1;
            This->baseShader.limits.packed_output = 0;
            This->baseShader.limits.sampler = 0;
            This->baseShader.limits.label = 16;
            This->baseShader.limits.constant_float = min(256, GL_LIMITS(vshader_constantsF));
            break;

        case WINED3D_SHADER_VERSION(3,0):
            This->baseShader.limits.temporary = 32;
            This->baseShader.limits.constant_bool = 32;
            This->baseShader.limits.constant_int = 32;
            This->baseShader.limits.address = 1;
            This->baseShader.limits.packed_output = 12;
            This->baseShader.limits.sampler = 4;
            This->baseShader.limits.label = 16; /* FIXME: 2048 */
            /* DX10 cards on Windows advertise a d3d9 constant limit of 256 even though they are capable
             * of supporting much more(GL drivers advertise 1024). d3d9.dll and d3d8.dll clamp the
             * wined3d-advertised maximum. Clamp the constant limit for <= 3.0 shaders to 256.s
             * use constant buffers */
            This->baseShader.limits.constant_float = min(256, GL_LIMITS(vshader_constantsF));
            break;

        default:
            This->baseShader.limits.temporary = 12;
            This->baseShader.limits.constant_bool = 16;
            This->baseShader.limits.constant_int = 16;
            This->baseShader.limits.address = 1;
            This->baseShader.limits.packed_output = 0;
            This->baseShader.limits.sampler = 0;
            This->baseShader.limits.label = 16;
            This->baseShader.limits.constant_float = min(256, GL_LIMITS(vshader_constantsF));
            FIXME("Unrecognized vertex shader version %u.%u\n",
                    This->baseShader.reg_maps.shader_version.major,
                    This->baseShader.reg_maps.shader_version.minor);
    }
}

/* This is an internal function,
 * used to create fake semantics for shaders
 * that don't have them - d3d8 shaders where the declaration
 * stores the register for each input
 */
static void vshader_set_input(
    IWineD3DVertexShaderImpl* This,
    unsigned int regnum,
    BYTE usage, BYTE usage_idx) {

    This->semantics_in[regnum].usage = usage;
    This->semantics_in[regnum].usage_idx = usage_idx;
    This->semantics_in[regnum].reg.reg.type = WINED3DSPR_INPUT;
    This->semantics_in[regnum].reg.reg.idx = regnum;
    This->semantics_in[regnum].reg.write_mask = WINED3DSP_WRITEMASK_ALL;
    This->semantics_in[regnum].reg.modifiers = 0;
    This->semantics_in[regnum].reg.shift = 0;
    This->semantics_in[regnum].reg.reg.rel_addr = NULL;
}

static BOOL match_usage(BYTE usage1, BYTE usage_idx1, BYTE usage2, BYTE usage_idx2) {
    if (usage_idx1 != usage_idx2) return FALSE;
    if (usage1 == usage2) return TRUE;
    if (usage1 == WINED3DDECLUSAGE_POSITION && usage2 == WINED3DDECLUSAGE_POSITIONT) return TRUE;
    if (usage2 == WINED3DDECLUSAGE_POSITION && usage1 == WINED3DDECLUSAGE_POSITIONT) return TRUE;

    return FALSE;
}

BOOL vshader_get_input(
    IWineD3DVertexShader* iface,
    BYTE usage_req, BYTE usage_idx_req,
    unsigned int* regnum) {

    IWineD3DVertexShaderImpl* This = (IWineD3DVertexShaderImpl*) iface;
    int i;

    for (i = 0; i < MAX_ATTRIBS; i++) {
        if (!This->baseShader.reg_maps.attributes[i]) continue;

        if (match_usage(This->semantics_in[i].usage,
                This->semantics_in[i].usage_idx, usage_req, usage_idx_req))
        {
            *regnum = i;
            return TRUE;
        }
    }
    return FALSE;
}

/* *******************************************
   IWineD3DVertexShader IUnknown parts follow
   ******************************************* */
static HRESULT  WINAPI IWineD3DVertexShaderImpl_QueryInterface(IWineD3DVertexShader *iface, REFIID riid, LPVOID *ppobj) {
    TRACE("iface %p, riid %s, ppobj %p\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IWineD3DVertexShader)
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

static ULONG  WINAPI IWineD3DVertexShaderImpl_AddRef(IWineD3DVertexShader *iface) {
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    ULONG refcount = InterlockedIncrement(&This->baseShader.ref);

    TRACE("%p increasing refcount to %u\n", This, refcount);

    return refcount;
}

static ULONG WINAPI IWineD3DVertexShaderImpl_Release(IWineD3DVertexShader *iface) {
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
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
   IWineD3DVertexShader IWineD3DVertexShader parts follow
   ******************************************* */

static HRESULT WINAPI IWineD3DVertexShaderImpl_GetParent(IWineD3DVertexShader *iface, IUnknown** parent){
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    
    *parent = This->parent;
    IUnknown_AddRef(*parent);
    TRACE("(%p) : returning %p\n", This, *parent);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DVertexShaderImpl_GetDevice(IWineD3DVertexShader* iface, IWineD3DDevice **pDevice){
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)iface;
    IWineD3DDevice_AddRef(This->baseShader.device);
    *pDevice = This->baseShader.device;
    TRACE("(%p) returning %p\n", This, *pDevice);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DVertexShaderImpl_GetFunction(IWineD3DVertexShader* impl, VOID* pData, UINT* pSizeOfData) {
    IWineD3DVertexShaderImpl *This = (IWineD3DVertexShaderImpl *)impl;
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

/* Note that for vertex shaders CompileShader isn't called until the
 * shader is first used. The reason for this is that we need the vertex
 * declaration the shader will be used with in order to determine if
 * the data in a register is of type D3DCOLOR, and needs swizzling. */
static HRESULT WINAPI IWineD3DVertexShaderImpl_SetFunction(IWineD3DVertexShader *iface,
        const DWORD *pFunction, const struct wined3d_shader_signature *output_signature)
{
    IWineD3DVertexShaderImpl *This =(IWineD3DVertexShaderImpl *)iface;
    IWineD3DDeviceImpl *deviceImpl = (IWineD3DDeviceImpl *) This->baseShader.device;
    const struct wined3d_shader_frontend *fe;
    HRESULT hr;
    shader_reg_maps *reg_maps = &This->baseShader.reg_maps;

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

    /* Second pass: figure out registers used, semantics, etc.. */
    This->min_rel_offset = GL_LIMITS(vshader_constantsF);
    This->max_rel_offset = 0;
    hr = shader_get_registers_used((IWineD3DBaseShader*) This, fe,
            reg_maps, This->semantics_in, This->semantics_out, pFunction,
            GL_LIMITS(vshader_constantsF));
    if (hr != WINED3D_OK) return hr;

    vshader_set_limits(This);

    if(deviceImpl->vs_selected_mode == SHADER_ARB &&
       (GLINFO_LOCATION).arb_vs_offset_limit      &&
       This->min_rel_offset <= This->max_rel_offset) {

        if(This->max_rel_offset - This->min_rel_offset > 127) {
            FIXME("The difference between the minimum and maximum relative offset is > 127\n");
            FIXME("Which this OpenGL implementation does not support. Try using GLSL\n");
            FIXME("Min: %d, Max: %d\n", This->min_rel_offset, This->max_rel_offset);
        } else if(This->max_rel_offset - This->min_rel_offset > 63) {
            This->rel_offset = This->min_rel_offset + 63;
        } else if(This->max_rel_offset > 63) {
            This->rel_offset = This->min_rel_offset;
        } else {
            This->rel_offset = 0;
        }
    }
    This->baseShader.load_local_constsF = This->baseShader.reg_maps.usesrelconstF && !list_empty(&This->baseShader.constantsF);

    /* copy the function ... because it will certainly be released by application */
    This->baseShader.function = HeapAlloc(GetProcessHeap(), 0, This->baseShader.functionLength);
    if (!This->baseShader.function) return E_OUTOFMEMORY;
    memcpy(This->baseShader.function, pFunction, This->baseShader.functionLength);

    return WINED3D_OK;
}

/* Preload semantics for d3d8 shaders */
static void WINAPI IWineD3DVertexShaderImpl_FakeSemantics(IWineD3DVertexShader *iface, IWineD3DVertexDeclaration *vertex_declaration) {
    IWineD3DVertexShaderImpl *This =(IWineD3DVertexShaderImpl *)iface;
    IWineD3DVertexDeclarationImpl* vdecl = (IWineD3DVertexDeclarationImpl*)vertex_declaration;

    unsigned int i;
    for (i = 0; i < vdecl->element_count; ++i)
    {
        const struct wined3d_vertex_declaration_element *e = &vdecl->elements[i];
        vshader_set_input(This, e->output_slot, e->usage, e->usage_idx);
    }
}

/* Set local constants for d3d8 shaders */
static HRESULT WINAPI IWIneD3DVertexShaderImpl_SetLocalConstantsF(IWineD3DVertexShader *iface,
        UINT start_idx, const float *src_data, UINT count) {
    IWineD3DVertexShaderImpl *This =(IWineD3DVertexShaderImpl *)iface;
    UINT i, end_idx;

    TRACE("(%p) : start_idx %u, src_data %p, count %u\n", This, start_idx, src_data, count);

    end_idx = start_idx + count;
    if (end_idx > GL_LIMITS(vshader_constantsF)) {
        WARN("end_idx %u > float constants limit %u\n", end_idx, GL_LIMITS(vshader_constantsF));
        end_idx = GL_LIMITS(vshader_constantsF);
    }

    for (i = start_idx; i < end_idx; ++i) {
        local_constant* lconst = HeapAlloc(GetProcessHeap(), 0, sizeof(local_constant));
        if (!lconst) return E_OUTOFMEMORY;

        lconst->idx = i;
        memcpy(lconst->value, src_data + (i - start_idx) * 4 /* 4 components */, 4 * sizeof(float));
        list_add_head(&This->baseShader.constantsF, &lconst->entry);
    }

    return WINED3D_OK;
}

static GLuint vertexshader_compile(IWineD3DVertexShaderImpl *This, const struct vs_compile_args *args) {
    IWineD3DDeviceImpl *deviceImpl = (IWineD3DDeviceImpl *) This->baseShader.device;
    SHADER_BUFFER buffer;
    GLuint ret;

    /* Generate the HW shader */
    TRACE("(%p) : Generating hardware program\n", This);
    shader_buffer_init(&buffer);
    This->cur_args = args;
    ret = deviceImpl->shader_backend->shader_generate_vshader((IWineD3DVertexShader *)This, &buffer, args);
    This->cur_args = NULL;
    shader_buffer_free(&buffer);

    return ret;
}

const IWineD3DVertexShaderVtbl IWineD3DVertexShader_Vtbl =
{
    /*** IUnknown methods ***/
    IWineD3DVertexShaderImpl_QueryInterface,
    IWineD3DVertexShaderImpl_AddRef,
    IWineD3DVertexShaderImpl_Release,
    /*** IWineD3DBase methods ***/
    IWineD3DVertexShaderImpl_GetParent,
    /*** IWineD3DBaseShader methods ***/
    IWineD3DVertexShaderImpl_SetFunction,
    /*** IWineD3DVertexShader methods ***/
    IWineD3DVertexShaderImpl_GetDevice,
    IWineD3DVertexShaderImpl_GetFunction,
    IWineD3DVertexShaderImpl_FakeSemantics,
    IWIneD3DVertexShaderImpl_SetLocalConstantsF
};

void find_vs_compile_args(IWineD3DVertexShaderImpl *shader, IWineD3DStateBlockImpl *stateblock, struct vs_compile_args *args) {
    args->fog_src = stateblock->renderState[WINED3DRS_FOGTABLEMODE] == WINED3DFOG_NONE ? VS_FOG_COORD : VS_FOG_Z;
    args->swizzle_map = ((IWineD3DDeviceImpl *)shader->baseShader.device)->strided_streams.swizzle_map;
}

static inline BOOL vs_args_equal(const struct vs_compile_args *stored, const struct vs_compile_args *new,
                                 const DWORD use_map) {
    if((stored->swizzle_map & use_map) != new->swizzle_map) return FALSE;
    return stored->fog_src == new->fog_src;
}

GLuint find_gl_vshader(IWineD3DVertexShaderImpl *shader, const struct vs_compile_args *args)
{
    UINT i;
    DWORD new_size = shader->shader_array_size;
    struct vs_compiled_shader *new_array;
    DWORD use_map = ((IWineD3DDeviceImpl *)shader->baseShader.device)->strided_streams.use_map;

    /* Usually we have very few GL shaders for each d3d shader(just 1 or maybe 2),
     * so a linear search is more performant than a hashmap or a binary search
     * (cache coherency etc)
     */
    for(i = 0; i < shader->num_gl_shaders; i++) {
        if(vs_args_equal(&shader->gl_shaders[i].args, args, use_map)) {
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
    shader->gl_shaders[shader->num_gl_shaders].prgId = vertexshader_compile(shader, args);
    return shader->gl_shaders[shader->num_gl_shaders++].prgId;
}
