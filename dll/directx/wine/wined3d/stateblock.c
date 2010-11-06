/*
 * state block implementation
 *
 * Copyright 2002 Raphael Junqueira
 * Copyright 2004 Jason Edmeades
 * Copyright 2005 Oliver Stieber
 * Copyright 2007 Stefan Dösinger for CodeWeavers
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
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

static const DWORD pixel_states_render[] =
{
    WINED3DRS_ALPHABLENDENABLE,
    WINED3DRS_ALPHAFUNC,
    WINED3DRS_ALPHAREF,
    WINED3DRS_ALPHATESTENABLE,
    WINED3DRS_ANTIALIASEDLINEENABLE,
    WINED3DRS_BLENDFACTOR,
    WINED3DRS_BLENDOP,
    WINED3DRS_BLENDOPALPHA,
    WINED3DRS_CCW_STENCILFAIL,
    WINED3DRS_CCW_STENCILPASS,
    WINED3DRS_CCW_STENCILZFAIL,
    WINED3DRS_COLORWRITEENABLE,
    WINED3DRS_COLORWRITEENABLE1,
    WINED3DRS_COLORWRITEENABLE2,
    WINED3DRS_COLORWRITEENABLE3,
    WINED3DRS_DEPTHBIAS,
    WINED3DRS_DESTBLEND,
    WINED3DRS_DESTBLENDALPHA,
    WINED3DRS_DITHERENABLE,
    WINED3DRS_FILLMODE,
    WINED3DRS_FOGDENSITY,
    WINED3DRS_FOGEND,
    WINED3DRS_FOGSTART,
    WINED3DRS_LASTPIXEL,
    WINED3DRS_SCISSORTESTENABLE,
    WINED3DRS_SEPARATEALPHABLENDENABLE,
    WINED3DRS_SHADEMODE,
    WINED3DRS_SLOPESCALEDEPTHBIAS,
    WINED3DRS_SRCBLEND,
    WINED3DRS_SRCBLENDALPHA,
    WINED3DRS_SRGBWRITEENABLE,
    WINED3DRS_STENCILENABLE,
    WINED3DRS_STENCILFAIL,
    WINED3DRS_STENCILFUNC,
    WINED3DRS_STENCILMASK,
    WINED3DRS_STENCILPASS,
    WINED3DRS_STENCILREF,
    WINED3DRS_STENCILWRITEMASK,
    WINED3DRS_STENCILZFAIL,
    WINED3DRS_TEXTUREFACTOR,
    WINED3DRS_TWOSIDEDSTENCILMODE,
    WINED3DRS_WRAP0,
    WINED3DRS_WRAP1,
    WINED3DRS_WRAP10,
    WINED3DRS_WRAP11,
    WINED3DRS_WRAP12,
    WINED3DRS_WRAP13,
    WINED3DRS_WRAP14,
    WINED3DRS_WRAP15,
    WINED3DRS_WRAP2,
    WINED3DRS_WRAP3,
    WINED3DRS_WRAP4,
    WINED3DRS_WRAP5,
    WINED3DRS_WRAP6,
    WINED3DRS_WRAP7,
    WINED3DRS_WRAP8,
    WINED3DRS_WRAP9,
    WINED3DRS_ZENABLE,
    WINED3DRS_ZFUNC,
    WINED3DRS_ZWRITEENABLE,
};

static const DWORD pixel_states_texture[] =
{
    WINED3DTSS_ALPHAARG0,
    WINED3DTSS_ALPHAARG1,
    WINED3DTSS_ALPHAARG2,
    WINED3DTSS_ALPHAOP,
    WINED3DTSS_BUMPENVLOFFSET,
    WINED3DTSS_BUMPENVLSCALE,
    WINED3DTSS_BUMPENVMAT00,
    WINED3DTSS_BUMPENVMAT01,
    WINED3DTSS_BUMPENVMAT10,
    WINED3DTSS_BUMPENVMAT11,
    WINED3DTSS_COLORARG0,
    WINED3DTSS_COLORARG1,
    WINED3DTSS_COLORARG2,
    WINED3DTSS_COLOROP,
    WINED3DTSS_RESULTARG,
    WINED3DTSS_TEXCOORDINDEX,
    WINED3DTSS_TEXTURETRANSFORMFLAGS,
};

static const DWORD pixel_states_sampler[] =
{
    WINED3DSAMP_ADDRESSU,
    WINED3DSAMP_ADDRESSV,
    WINED3DSAMP_ADDRESSW,
    WINED3DSAMP_BORDERCOLOR,
    WINED3DSAMP_MAGFILTER,
    WINED3DSAMP_MINFILTER,
    WINED3DSAMP_MIPFILTER,
    WINED3DSAMP_MIPMAPLODBIAS,
    WINED3DSAMP_MAXMIPLEVEL,
    WINED3DSAMP_MAXANISOTROPY,
    WINED3DSAMP_SRGBTEXTURE,
    WINED3DSAMP_ELEMENTINDEX,
};

static const DWORD vertex_states_render[] =
{
    WINED3DRS_ADAPTIVETESS_W,
    WINED3DRS_ADAPTIVETESS_X,
    WINED3DRS_ADAPTIVETESS_Y,
    WINED3DRS_ADAPTIVETESS_Z,
    WINED3DRS_AMBIENT,
    WINED3DRS_AMBIENTMATERIALSOURCE,
    WINED3DRS_CLIPPING,
    WINED3DRS_CLIPPLANEENABLE,
    WINED3DRS_COLORVERTEX,
    WINED3DRS_CULLMODE,
    WINED3DRS_DIFFUSEMATERIALSOURCE,
    WINED3DRS_EMISSIVEMATERIALSOURCE,
    WINED3DRS_ENABLEADAPTIVETESSELLATION,
    WINED3DRS_FOGCOLOR,
    WINED3DRS_FOGDENSITY,
    WINED3DRS_FOGENABLE,
    WINED3DRS_FOGEND,
    WINED3DRS_FOGSTART,
    WINED3DRS_FOGTABLEMODE,
    WINED3DRS_FOGVERTEXMODE,
    WINED3DRS_INDEXEDVERTEXBLENDENABLE,
    WINED3DRS_LIGHTING,
    WINED3DRS_LOCALVIEWER,
    WINED3DRS_MAXTESSELLATIONLEVEL,
    WINED3DRS_MINTESSELLATIONLEVEL,
    WINED3DRS_MULTISAMPLEANTIALIAS,
    WINED3DRS_MULTISAMPLEMASK,
    WINED3DRS_NORMALDEGREE,
    WINED3DRS_NORMALIZENORMALS,
    WINED3DRS_PATCHEDGESTYLE,
    WINED3DRS_POINTSCALE_A,
    WINED3DRS_POINTSCALE_B,
    WINED3DRS_POINTSCALE_C,
    WINED3DRS_POINTSCALEENABLE,
    WINED3DRS_POINTSIZE,
    WINED3DRS_POINTSIZE_MAX,
    WINED3DRS_POINTSIZE_MIN,
    WINED3DRS_POINTSPRITEENABLE,
    WINED3DRS_POSITIONDEGREE,
    WINED3DRS_RANGEFOGENABLE,
    WINED3DRS_SHADEMODE,
    WINED3DRS_SPECULARENABLE,
    WINED3DRS_SPECULARMATERIALSOURCE,
    WINED3DRS_TWEENFACTOR,
    WINED3DRS_VERTEXBLEND,
};

static const DWORD vertex_states_texture[] =
{
    WINED3DTSS_TEXCOORDINDEX,
    WINED3DTSS_TEXTURETRANSFORMFLAGS,
};

static const DWORD vertex_states_sampler[] =
{
    WINED3DSAMP_DMAPOFFSET,
};

/* Allocates the correct amount of space for pixel and vertex shader constants,
 * along with their set/changed flags on the given stateblock object
 */
static HRESULT stateblock_allocate_shader_constants(IWineD3DStateBlockImpl *object)
{
    IWineD3DDeviceImpl *device = object->device;

    /* Allocate space for floating point constants */
    object->state.ps_consts_f = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(float) * device->d3d_pshader_constantF * 4);
    if (!object->state.ps_consts_f) goto fail;

    object->changed.pixelShaderConstantsF = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(BOOL) * device->d3d_pshader_constantF);
    if (!object->changed.pixelShaderConstantsF) goto fail;

    object->state.vs_consts_f = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(float) * device->d3d_vshader_constantF * 4);
    if (!object->state.vs_consts_f) goto fail;

    object->changed.vertexShaderConstantsF = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(BOOL) * device->d3d_vshader_constantF);
    if (!object->changed.vertexShaderConstantsF) goto fail;

    object->contained_vs_consts_f = HeapAlloc(GetProcessHeap(), 0,
            sizeof(DWORD) * device->d3d_vshader_constantF);
    if (!object->contained_vs_consts_f) goto fail;

    object->contained_ps_consts_f = HeapAlloc(GetProcessHeap(), 0,
            sizeof(DWORD) * device->d3d_pshader_constantF);
    if (!object->contained_ps_consts_f) goto fail;

    return WINED3D_OK;

fail:
    ERR("Failed to allocate memory\n");
    HeapFree(GetProcessHeap(), 0, object->state.ps_consts_f);
    HeapFree(GetProcessHeap(), 0, object->changed.pixelShaderConstantsF);
    HeapFree(GetProcessHeap(), 0, object->state.vs_consts_f);
    HeapFree(GetProcessHeap(), 0, object->changed.vertexShaderConstantsF);
    HeapFree(GetProcessHeap(), 0, object->contained_vs_consts_f);
    HeapFree(GetProcessHeap(), 0, object->contained_ps_consts_f);
    return E_OUTOFMEMORY;
}

static inline void stateblock_set_bits(DWORD *map, UINT map_size)
{
    DWORD mask = (1 << (map_size & 0x1f)) - 1;
    memset(map, 0xff, (map_size >> 5) * sizeof(*map));
    if (mask) map[map_size >> 5] = mask;
}

/* Set all members of a stateblock savedstate to the given value */
static void stateblock_savedstates_set_all(SAVEDSTATES *states, DWORD vs_consts, DWORD ps_consts)
{
    unsigned int i;

    /* Single values */
    states->primitive_type = 1;
    states->indices = 1;
    states->material = 1;
    states->viewport = 1;
    states->vertexDecl = 1;
    states->pixelShader = 1;
    states->vertexShader = 1;
    states->scissorRect = 1;

    /* Fixed size arrays */
    states->streamSource = 0xffff;
    states->streamFreq = 0xffff;
    states->textures = 0xfffff;
    stateblock_set_bits(states->transform, HIGHEST_TRANSFORMSTATE + 1);
    stateblock_set_bits(states->renderState, WINEHIGHEST_RENDER_STATE + 1);
    for (i = 0; i < MAX_TEXTURES; ++i) states->textureState[i] = 0x3ffff;
    for (i = 0; i < MAX_COMBINED_SAMPLERS; ++i) states->samplerState[i] = 0x3ffe;
    states->clipplane = 0xffffffff;
    states->pixelShaderConstantsB = 0xffff;
    states->pixelShaderConstantsI = 0xffff;
    states->vertexShaderConstantsB = 0xffff;
    states->vertexShaderConstantsI = 0xffff;

    /* Dynamically sized arrays */
    memset(states->pixelShaderConstantsF, TRUE, sizeof(BOOL) * ps_consts);
    memset(states->vertexShaderConstantsF, TRUE, sizeof(BOOL) * vs_consts);
}

static void stateblock_savedstates_set_pixel(SAVEDSTATES *states, const DWORD num_constants)
{
    DWORD texture_mask = 0;
    WORD sampler_mask = 0;
    unsigned int i;

    states->pixelShader = 1;

    for (i = 0; i < sizeof(pixel_states_render) / sizeof(*pixel_states_render); ++i)
    {
        DWORD rs = pixel_states_render[i];
        states->renderState[rs >> 5] |= 1 << (rs & 0x1f);
    }

    for (i = 0; i < sizeof(pixel_states_texture) / sizeof(*pixel_states_texture); ++i)
        texture_mask |= 1 << pixel_states_texture[i];
    for (i = 0; i < MAX_TEXTURES; ++i) states->textureState[i] = texture_mask;
    for (i = 0; i < sizeof(pixel_states_sampler) / sizeof(*pixel_states_sampler); ++i)
        sampler_mask |= 1 << pixel_states_sampler[i];
    for (i = 0; i < MAX_COMBINED_SAMPLERS; ++i) states->samplerState[i] = sampler_mask;
    states->pixelShaderConstantsB = 0xffff;
    states->pixelShaderConstantsI = 0xffff;

    memset(states->pixelShaderConstantsF, TRUE, sizeof(BOOL) * num_constants);
}

static void stateblock_savedstates_set_vertex(SAVEDSTATES *states, const DWORD num_constants)
{
    DWORD texture_mask = 0;
    WORD sampler_mask = 0;
    unsigned int i;

    states->vertexDecl = 1;
    states->vertexShader = 1;

    for (i = 0; i < sizeof(vertex_states_render) / sizeof(*vertex_states_render); ++i)
    {
        DWORD rs = vertex_states_render[i];
        states->renderState[rs >> 5] |= 1 << (rs & 0x1f);
    }

    for (i = 0; i < sizeof(vertex_states_texture) / sizeof(*vertex_states_texture); ++i)
        texture_mask |= 1 << vertex_states_texture[i];
    for (i = 0; i < MAX_TEXTURES; ++i) states->textureState[i] = texture_mask;
    for (i = 0; i < sizeof(vertex_states_sampler) / sizeof(*vertex_states_sampler); ++i)
        sampler_mask |= 1 << vertex_states_sampler[i];
    for (i = 0; i < MAX_COMBINED_SAMPLERS; ++i) states->samplerState[i] = sampler_mask;
    states->vertexShaderConstantsB = 0xffff;
    states->vertexShaderConstantsI = 0xffff;

    memset(states->vertexShaderConstantsF, TRUE, sizeof(BOOL) * num_constants);
}

void stateblock_init_contained_states(IWineD3DStateBlockImpl *stateblock)
{
    IWineD3DDeviceImpl *device = stateblock->device;
    unsigned int i, j;

    for (i = 0; i <= WINEHIGHEST_RENDER_STATE >> 5; ++i)
    {
        DWORD map = stateblock->changed.renderState[i];
        for (j = 0; map; map >>= 1, ++j)
        {
            if (!(map & 1)) continue;

            stateblock->contained_render_states[stateblock->num_contained_render_states] = (i << 5) | j;
            ++stateblock->num_contained_render_states;
        }
    }

    for (i = 0; i <= HIGHEST_TRANSFORMSTATE >> 5; ++i)
    {
        DWORD map = stateblock->changed.transform[i];
        for (j = 0; map; map >>= 1, ++j)
        {
            if (!(map & 1)) continue;

            stateblock->contained_transform_states[stateblock->num_contained_transform_states] = (i << 5) | j;
            ++stateblock->num_contained_transform_states;
        }
    }

    for (i = 0; i < device->d3d_vshader_constantF; ++i)
    {
        if (stateblock->changed.vertexShaderConstantsF[i])
        {
            stateblock->contained_vs_consts_f[stateblock->num_contained_vs_consts_f] = i;
            ++stateblock->num_contained_vs_consts_f;
        }
    }

    for (i = 0; i < MAX_CONST_I; ++i)
    {
        if (stateblock->changed.vertexShaderConstantsI & (1 << i))
        {
            stateblock->contained_vs_consts_i[stateblock->num_contained_vs_consts_i] = i;
            ++stateblock->num_contained_vs_consts_i;
        }
    }

    for (i = 0; i < MAX_CONST_B; ++i)
    {
        if (stateblock->changed.vertexShaderConstantsB & (1 << i))
        {
            stateblock->contained_vs_consts_b[stateblock->num_contained_vs_consts_b] = i;
            ++stateblock->num_contained_vs_consts_b;
        }
    }

    for (i = 0; i < device->d3d_pshader_constantF; ++i)
    {
        if (stateblock->changed.pixelShaderConstantsF[i])
        {
            stateblock->contained_ps_consts_f[stateblock->num_contained_ps_consts_f] = i;
            ++stateblock->num_contained_ps_consts_f;
        }
    }

    for (i = 0; i < MAX_CONST_I; ++i)
    {
        if (stateblock->changed.pixelShaderConstantsI & (1 << i))
        {
            stateblock->contained_ps_consts_i[stateblock->num_contained_ps_consts_i] = i;
            ++stateblock->num_contained_ps_consts_i;
        }
    }

    for (i = 0; i < MAX_CONST_B; ++i)
    {
        if (stateblock->changed.pixelShaderConstantsB & (1 << i))
        {
            stateblock->contained_ps_consts_b[stateblock->num_contained_ps_consts_b] = i;
            ++stateblock->num_contained_ps_consts_b;
        }
    }

    for (i = 0; i < MAX_TEXTURES; ++i)
    {
        DWORD map = stateblock->changed.textureState[i];

        for(j = 0; map; map >>= 1, ++j)
        {
            if (!(map & 1)) continue;

            stateblock->contained_tss_states[stateblock->num_contained_tss_states].stage = i;
            stateblock->contained_tss_states[stateblock->num_contained_tss_states].state = j;
            ++stateblock->num_contained_tss_states;
        }
    }

    for (i = 0; i < MAX_COMBINED_SAMPLERS; ++i)
    {
        DWORD map = stateblock->changed.samplerState[i];

        for (j = 0; map; map >>= 1, ++j)
        {
            if (!(map & 1)) continue;

            stateblock->contained_sampler_states[stateblock->num_contained_sampler_states].stage = i;
            stateblock->contained_sampler_states[stateblock->num_contained_sampler_states].state = j;
            ++stateblock->num_contained_sampler_states;
        }
    }
}

static void stateblock_init_lights(IWineD3DStateBlockImpl *stateblock, struct list *light_map)
{
    unsigned int i;

    for (i = 0; i < LIGHTMAP_SIZE; ++i)
    {
        const struct wined3d_light_info *src_light;

        LIST_FOR_EACH_ENTRY(src_light, &light_map[i], struct wined3d_light_info, entry)
        {
            struct wined3d_light_info *dst_light = HeapAlloc(GetProcessHeap(), 0, sizeof(*dst_light));

            *dst_light = *src_light;
            list_add_tail(&stateblock->state.light_map[i], &dst_light->entry);
        }
    }
}

/**********************************************************
 * IWineD3DStateBlockImpl IUnknown parts follows
 **********************************************************/
static HRESULT  WINAPI IWineD3DStateBlockImpl_QueryInterface(IWineD3DStateBlock *iface,REFIID riid,LPVOID *ppobj)
{
    IWineD3DStateBlockImpl *This = (IWineD3DStateBlockImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IWineD3DStateBlock))
    {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG  WINAPI IWineD3DStateBlockImpl_AddRef(IWineD3DStateBlock *iface) {
    IWineD3DStateBlockImpl *This = (IWineD3DStateBlockImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef increasing from %d\n", This, refCount - 1);
    return refCount;
}

static ULONG  WINAPI IWineD3DStateBlockImpl_Release(IWineD3DStateBlock *iface) {
    IWineD3DStateBlockImpl *This = (IWineD3DStateBlockImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p) : Releasing from %d\n", This, refCount + 1);

    if (!refCount) {
        int counter;

        if (This->state.vertex_declaration)
            IWineD3DVertexDeclaration_Release((IWineD3DVertexDeclaration *)This->state.vertex_declaration);

        for (counter = 0; counter < MAX_COMBINED_SAMPLERS; counter++)
        {
            if (This->state.textures[counter])
                IWineD3DBaseTexture_Release((IWineD3DBaseTexture *)This->state.textures[counter]);
        }

        for (counter = 0; counter < MAX_STREAMS; ++counter)
        {
            struct wined3d_buffer *buffer = This->state.streams[counter].buffer;
            if (buffer)
            {
                if (IWineD3DBuffer_Release((IWineD3DBuffer *)buffer))
                {
                    WARN("Buffer %p still referenced by stateblock, stream %u.\n", buffer, counter);
                }
            }
        }
        if (This->state.index_buffer) IWineD3DBuffer_Release((IWineD3DBuffer *)This->state.index_buffer);
        if (This->state.vertex_shader) IWineD3DVertexShader_Release((IWineD3DVertexShader *)This->state.vertex_shader);
        if (This->state.pixel_shader) IWineD3DPixelShader_Release((IWineD3DPixelShader *)This->state.pixel_shader);

        for(counter = 0; counter < LIGHTMAP_SIZE; counter++) {
            struct list *e1, *e2;
            LIST_FOR_EACH_SAFE(e1, e2, &This->state.light_map[counter])
            {
                struct wined3d_light_info *light = LIST_ENTRY(e1, struct wined3d_light_info, entry);
                list_remove(&light->entry);
                HeapFree(GetProcessHeap(), 0, light);
            }
        }

        HeapFree(GetProcessHeap(), 0, This->state.vs_consts_f);
        HeapFree(GetProcessHeap(), 0, This->changed.vertexShaderConstantsF);
        HeapFree(GetProcessHeap(), 0, This->state.ps_consts_f);
        HeapFree(GetProcessHeap(), 0, This->changed.pixelShaderConstantsF);
        HeapFree(GetProcessHeap(), 0, This->contained_vs_consts_f);
        HeapFree(GetProcessHeap(), 0, This->contained_ps_consts_f);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return refCount;
}

static void wined3d_state_record_lights(struct wined3d_state *dst_state, const struct wined3d_state *src_state)
{
    UINT i;

    /* Lights... For a recorded state block, we just had a chain of actions
     * to perform, so we need to walk that chain and update any actions which
     * differ. */
    for (i = 0; i < LIGHTMAP_SIZE; ++i)
    {
        struct list *e, *f;
        LIST_FOR_EACH(e, &dst_state->light_map[i])
        {
            BOOL updated = FALSE;
            struct wined3d_light_info *src = LIST_ENTRY(e, struct wined3d_light_info, entry), *realLight;

            /* Look up the light in the destination */
            LIST_FOR_EACH(f, &src_state->light_map[i])
            {
                realLight = LIST_ENTRY(f, struct wined3d_light_info, entry);
                if (realLight->OriginalIndex == src->OriginalIndex)
                {
                    src->OriginalParms = realLight->OriginalParms;

                    if (realLight->glIndex == -1 && src->glIndex != -1)
                    {
                        /* Light disabled */
                        dst_state->lights[src->glIndex] = NULL;
                    }
                    else if (realLight->glIndex != -1 && src->glIndex == -1)
                    {
                        /* Light enabled */
                        dst_state->lights[realLight->glIndex] = src;
                    }
                    src->glIndex = realLight->glIndex;
                    updated = TRUE;
                    break;
                }
            }

            if (!updated)
            {
                /* This can happen if the light was originally created as a
                 * default light for SetLightEnable() while recording. */
                WARN("Light %u in dst_state %p does not exist in src_state %p.\n",
                        src->OriginalIndex, dst_state, src_state);

                src->OriginalParms = WINED3D_default_light;
                if (src->glIndex != -1)
                {
                    dst_state->lights[src->glIndex] = NULL;
                    src->glIndex = -1;
                }
            }
        }
    }
}

static HRESULT WINAPI IWineD3DStateBlockImpl_Capture(IWineD3DStateBlock *iface)
{
    IWineD3DStateBlockImpl *This = (IWineD3DStateBlockImpl *)iface;
    const struct wined3d_state *src_state = &This->device->stateBlock->state;
    unsigned int i;
    DWORD map;

    TRACE("iface %p.\n", iface);

    TRACE("Capturing state %p.\n", src_state);

    if (This->changed.vertexShader && This->state.vertex_shader != src_state->vertex_shader)
    {
        TRACE("Updating vertex shader from %p to %p\n",
                This->state.vertex_shader, src_state->vertex_shader);

        if (src_state->vertex_shader)
            IWineD3DVertexShader_AddRef((IWineD3DVertexShader *)src_state->vertex_shader);
        if (This->state.vertex_shader)
            IWineD3DVertexShader_Release((IWineD3DVertexShader *)This->state.vertex_shader);
        This->state.vertex_shader = src_state->vertex_shader;
    }

    /* Vertex Shader Float Constants */
    for (i = 0; i < This->num_contained_vs_consts_f; ++i)
    {
        unsigned int idx = This->contained_vs_consts_f[i];

        TRACE("Setting vs_consts_f[%u] to {%.8e, %.8e, %.8e, %.8e}.\n", idx,
                src_state->vs_consts_f[idx * 4 + 0],
                src_state->vs_consts_f[idx * 4 + 1],
                src_state->vs_consts_f[idx * 4 + 2],
                src_state->vs_consts_f[idx * 4 + 3]);

        This->state.vs_consts_f[idx * 4 + 0] = src_state->vs_consts_f[idx * 4 + 0];
        This->state.vs_consts_f[idx * 4 + 1] = src_state->vs_consts_f[idx * 4 + 1];
        This->state.vs_consts_f[idx * 4 + 2] = src_state->vs_consts_f[idx * 4 + 2];
        This->state.vs_consts_f[idx * 4 + 3] = src_state->vs_consts_f[idx * 4 + 3];
    }

    /* Vertex Shader Integer Constants */
    for (i = 0; i < This->num_contained_vs_consts_i; ++i)
    {
        unsigned int idx = This->contained_vs_consts_i[i];

        TRACE("Setting vs_consts[%u] to {%d, %d, %d, %d}.\n", idx,
                src_state->vs_consts_i[idx * 4 + 0],
                src_state->vs_consts_i[idx * 4 + 1],
                src_state->vs_consts_i[idx * 4 + 2],
                src_state->vs_consts_i[idx * 4 + 3]);

        This->state.vs_consts_i[idx * 4 + 0] = src_state->vs_consts_i[idx * 4 + 0];
        This->state.vs_consts_i[idx * 4 + 1] = src_state->vs_consts_i[idx * 4 + 1];
        This->state.vs_consts_i[idx * 4 + 2] = src_state->vs_consts_i[idx * 4 + 2];
        This->state.vs_consts_i[idx * 4 + 3] = src_state->vs_consts_i[idx * 4 + 3];
    }

    /* Vertex Shader Boolean Constants */
    for (i = 0; i < This->num_contained_vs_consts_b; ++i)
    {
        unsigned int idx = This->contained_vs_consts_b[i];

        TRACE("Setting vs_consts_b[%u] to %s.\n",
                idx, src_state->vs_consts_b[idx] ? "TRUE" : "FALSE");

        This->state.vs_consts_b[idx] = src_state->vs_consts_b[idx];
    }

    /* Pixel Shader Float Constants */
    for (i = 0; i < This->num_contained_ps_consts_f; ++i)
    {
        unsigned int idx = This->contained_ps_consts_f[i];

        TRACE("Setting ps_consts_f[%u] to {%.8e, %.8e, %.8e, %.8e}.\n", idx,
                src_state->ps_consts_f[idx * 4 + 0],
                src_state->ps_consts_f[idx * 4 + 1],
                src_state->ps_consts_f[idx * 4 + 2],
                src_state->ps_consts_f[idx * 4 + 3]);

        This->state.ps_consts_f[idx * 4 + 0] = src_state->ps_consts_f[idx * 4 + 0];
        This->state.ps_consts_f[idx * 4 + 1] = src_state->ps_consts_f[idx * 4 + 1];
        This->state.ps_consts_f[idx * 4 + 2] = src_state->ps_consts_f[idx * 4 + 2];
        This->state.ps_consts_f[idx * 4 + 3] = src_state->ps_consts_f[idx * 4 + 3];
    }

    /* Pixel Shader Integer Constants */
    for (i = 0; i < This->num_contained_ps_consts_i; ++i)
    {
        unsigned int idx = This->contained_ps_consts_i[i];
        TRACE("Setting ps_consts_i[%u] to {%d, %d, %d, %d}.\n", idx,
                src_state->ps_consts_i[idx * 4 + 0],
                src_state->ps_consts_i[idx * 4 + 1],
                src_state->ps_consts_i[idx * 4 + 2],
                src_state->ps_consts_i[idx * 4 + 3]);

        This->state.ps_consts_i[idx * 4 + 0] = src_state->ps_consts_i[idx * 4 + 0];
        This->state.ps_consts_i[idx * 4 + 1] = src_state->ps_consts_i[idx * 4 + 1];
        This->state.ps_consts_i[idx * 4 + 2] = src_state->ps_consts_i[idx * 4 + 2];
        This->state.ps_consts_i[idx * 4 + 3] = src_state->ps_consts_i[idx * 4 + 3];
    }

    /* Pixel Shader Boolean Constants */
    for (i = 0; i < This->num_contained_ps_consts_b; ++i)
    {
        unsigned int idx = This->contained_ps_consts_b[i];
        TRACE("Setting ps_consts_b[%u] to %s.\n",
                idx, src_state->ps_consts_b[idx] ? "TRUE" : "FALSE");

        This->state.ps_consts_b[idx] = src_state->ps_consts_b[idx];
    }

    /* Others + Render & Texture */
    for (i = 0; i < This->num_contained_transform_states; ++i)
    {
        WINED3DTRANSFORMSTATETYPE transform = This->contained_transform_states[i];

        TRACE("Updating transform %#x.\n", transform);

        This->state.transforms[transform] = src_state->transforms[transform];
    }

    if (This->changed.primitive_type)
        This->state.gl_primitive_type = src_state->gl_primitive_type;

    if (This->changed.indices
            && ((This->state.index_buffer != src_state->index_buffer)
                || (This->state.base_vertex_index != src_state->base_vertex_index)
                || (This->state.index_format != src_state->index_format)))
    {
        TRACE("Updating index buffer to %p, base vertex index to %d.\n",
                src_state->index_buffer, src_state->base_vertex_index);

        if (src_state->index_buffer)
            IWineD3DBuffer_AddRef((IWineD3DBuffer *)src_state->index_buffer);
        if (This->state.index_buffer)
            IWineD3DBuffer_Release((IWineD3DBuffer *)This->state.index_buffer);
        This->state.index_buffer = src_state->index_buffer;
        This->state.base_vertex_index = src_state->base_vertex_index;
        This->state.index_format = src_state->index_format;
    }

    if (This->changed.vertexDecl && This->state.vertex_declaration != src_state->vertex_declaration)
    {
        TRACE("Updating vertex declaration from %p to %p.\n",
                This->state.vertex_declaration, src_state->vertex_declaration);

        if (src_state->vertex_declaration)
                IWineD3DVertexDeclaration_AddRef((IWineD3DVertexDeclaration *)src_state->vertex_declaration);
        if (This->state.vertex_declaration)
                IWineD3DVertexDeclaration_Release((IWineD3DVertexDeclaration *)This->state.vertex_declaration);
        This->state.vertex_declaration = src_state->vertex_declaration;
    }

    if (This->changed.material && memcmp(&src_state->material, &This->state.material, sizeof(This->state.material)))
    {
        TRACE("Updating material.\n");

        This->state.material = src_state->material;
    }

    if (This->changed.viewport && memcmp(&src_state->viewport, &This->state.viewport, sizeof(This->state.viewport)))
    {
        TRACE("Updating viewport.\n");

        This->state.viewport = src_state->viewport;
    }

    if (This->changed.scissorRect && memcmp(&src_state->scissor_rect,
            &This->state.scissor_rect, sizeof(This->state.scissor_rect)))
    {
        TRACE("Updating scissor rect.\n");

        This->state.scissor_rect = src_state->scissor_rect;
    }

    map = This->changed.streamSource;
    for (i = 0; map; map >>= 1, ++i)
    {
        if (!(map & 1)) continue;

        if (This->state.streams[i].stride != src_state->streams[i].stride
                || This->state.streams[i].buffer != src_state->streams[i].buffer)
        {
            TRACE("Updating stream source %u to %p, stride to %u.\n",
                    i, src_state->streams[i].buffer,
                    src_state->streams[i].stride);

            This->state.streams[i].stride = src_state->streams[i].stride;
            if (src_state->streams[i].buffer)
                    IWineD3DBuffer_AddRef((IWineD3DBuffer *)src_state->streams[i].buffer);
            if (This->state.streams[i].buffer)
                    IWineD3DBuffer_Release((IWineD3DBuffer *)This->state.streams[i].buffer);
            This->state.streams[i].buffer = src_state->streams[i].buffer;
        }
    }

    map = This->changed.streamFreq;
    for (i = 0; map; map >>= 1, ++i)
    {
        if (!(map & 1)) continue;

        if (This->state.streams[i].frequency != src_state->streams[i].frequency
                || This->state.streams[i].flags != src_state->streams[i].flags)
        {
            TRACE("Updating stream frequency %u to %u flags to %#x.\n",
                    i, src_state->streams[i].frequency, src_state->streams[i].flags);

            This->state.streams[i].frequency = src_state->streams[i].frequency;
            This->state.streams[i].flags = src_state->streams[i].flags;
        }
    }

    map = This->changed.clipplane;
    for (i = 0; map; map >>= 1, ++i)
    {
        if (!(map & 1)) continue;

        if (memcmp(src_state->clip_planes[i], This->state.clip_planes[i], sizeof(*This->state.clip_planes)))
        {
            TRACE("Updating clipplane %u.\n", i);
            memcpy(This->state.clip_planes[i], src_state->clip_planes[i], sizeof(*This->state.clip_planes));
        }
    }

    /* Render */
    for (i = 0; i < This->num_contained_render_states; ++i)
    {
        WINED3DRENDERSTATETYPE rs = This->contained_render_states[i];

        TRACE("Updating render state %#x to %u.\n", rs, src_state->render_states[rs]);

        This->state.render_states[rs] = src_state->render_states[rs];
    }

    /* Texture states */
    for (i = 0; i < This->num_contained_tss_states; ++i)
    {
        DWORD stage = This->contained_tss_states[i].stage;
        DWORD state = This->contained_tss_states[i].state;

        TRACE("Updating texturestage state %u, %u to %#x (was %#x).\n", stage, state,
                src_state->texture_states[stage][state], This->state.texture_states[stage][state]);

        This->state.texture_states[stage][state] = src_state->texture_states[stage][state];
    }

    /* Samplers */
    map = This->changed.textures;
    for (i = 0; map; map >>= 1, ++i)
    {
        if (!(map & 1)) continue;

        TRACE("Updating texture %u to %p (was %p).\n",
                i, src_state->textures[i], This->state.textures[i]);

        if (src_state->textures[i])
            IWineD3DBaseTexture_AddRef((IWineD3DBaseTexture *)src_state->textures[i]);
        if (This->state.textures[i])
            IWineD3DBaseTexture_Release((IWineD3DBaseTexture *)This->state.textures[i]);
        This->state.textures[i] = src_state->textures[i];
    }

    for (i = 0; i < This->num_contained_sampler_states; ++i)
    {
        DWORD stage = This->contained_sampler_states[i].stage;
        DWORD state = This->contained_sampler_states[i].state;

        TRACE("Updating sampler state %u, %u to %#x (was %#x).\n", stage, state,
                src_state->sampler_states[stage][state], This->state.sampler_states[stage][state]);

        This->state.sampler_states[stage][state] = src_state->sampler_states[stage][state];
    }

    if (This->changed.pixelShader && This->state.pixel_shader != src_state->pixel_shader)
    {
        if (src_state->pixel_shader)
            IWineD3DPixelShader_AddRef((IWineD3DPixelShader *)src_state->pixel_shader);
        if (This->state.pixel_shader)
            IWineD3DPixelShader_Release((IWineD3DPixelShader *)This->state.pixel_shader);
        This->state.pixel_shader = src_state->pixel_shader;
    }

    wined3d_state_record_lights(&This->state, src_state);

    TRACE("Captue done.\n");

    return WINED3D_OK;
}

static void apply_lights(IWineD3DDevice *device, const struct wined3d_state *state)
{
    UINT i;

    for (i = 0; i < LIGHTMAP_SIZE; ++i)
    {
        struct list *e;

        LIST_FOR_EACH(e, &state->light_map[i])
        {
            const struct wined3d_light_info *light = LIST_ENTRY(e, struct wined3d_light_info, entry);

            IWineD3DDevice_SetLight(device, light->OriginalIndex, &light->OriginalParms);
            IWineD3DDevice_SetLightEnable(device, light->OriginalIndex, light->glIndex != -1);
        }
    }
}

static HRESULT WINAPI IWineD3DStateBlockImpl_Apply(IWineD3DStateBlock *iface)
{
    IWineD3DStateBlockImpl *This = (IWineD3DStateBlockImpl *)iface;
    IWineD3DDevice *device = (IWineD3DDevice *)This->device;
    unsigned int i;
    DWORD map;

    TRACE("(%p) : Applying state block %p ------------------v\n", This, device);

    TRACE("Blocktype: %d\n", This->blockType);

    if (This->changed.vertexShader)
        IWineD3DDevice_SetVertexShader(device, (IWineD3DVertexShader *)This->state.vertex_shader);

    /* Vertex Shader Constants */
    for (i = 0; i < This->num_contained_vs_consts_f; ++i)
    {
        IWineD3DDevice_SetVertexShaderConstantF(device, This->contained_vs_consts_f[i],
                This->state.vs_consts_f + This->contained_vs_consts_f[i] * 4, 1);
    }
    for (i = 0; i < This->num_contained_vs_consts_i; ++i)
    {
        IWineD3DDevice_SetVertexShaderConstantI(device, This->contained_vs_consts_i[i],
                This->state.vs_consts_i + This->contained_vs_consts_i[i] * 4, 1);
    }
    for (i = 0; i < This->num_contained_vs_consts_b; ++i)
    {
        IWineD3DDevice_SetVertexShaderConstantB(device, This->contained_vs_consts_b[i],
                This->state.vs_consts_b + This->contained_vs_consts_b[i], 1);
    }

    apply_lights(device, &This->state);

    if (This->changed.pixelShader)
        IWineD3DDevice_SetPixelShader(device, (IWineD3DPixelShader *)This->state.pixel_shader);

    /* Pixel Shader Constants */
    for (i = 0; i < This->num_contained_ps_consts_f; ++i)
    {
        IWineD3DDevice_SetPixelShaderConstantF(device, This->contained_ps_consts_f[i],
                This->state.ps_consts_f + This->contained_ps_consts_f[i] * 4, 1);
    }
    for (i = 0; i < This->num_contained_ps_consts_i; ++i)
    {
        IWineD3DDevice_SetPixelShaderConstantI(device, This->contained_ps_consts_i[i],
                This->state.ps_consts_i + This->contained_ps_consts_i[i] * 4, 1);
    }
    for (i = 0; i < This->num_contained_ps_consts_b; ++i)
    {
        IWineD3DDevice_SetPixelShaderConstantB(device, This->contained_ps_consts_b[i],
                This->state.ps_consts_b + This->contained_ps_consts_b[i], 1);
    }

    /* Render */
    for (i = 0; i < This->num_contained_render_states; ++i)
    {
        IWineD3DDevice_SetRenderState(device, This->contained_render_states[i],
                This->state.render_states[This->contained_render_states[i]]);
    }

    /* Texture states */
    for (i = 0; i < This->num_contained_tss_states; ++i)
    {
        DWORD stage = This->contained_tss_states[i].stage;
        DWORD state = This->contained_tss_states[i].state;

        IWineD3DDevice_SetTextureStageState(device, stage, state, This->state.texture_states[stage][state]);
    }

    /* Sampler states */
    for (i = 0; i < This->num_contained_sampler_states; ++i)
    {
        DWORD stage = This->contained_sampler_states[i].stage;
        DWORD state = This->contained_sampler_states[i].state;
        DWORD value = This->state.sampler_states[stage][state];

        if (stage >= MAX_FRAGMENT_SAMPLERS) stage += WINED3DVERTEXTEXTURESAMPLER0 - MAX_FRAGMENT_SAMPLERS;
        IWineD3DDevice_SetSamplerState(device, stage, state, value);
    }

    for (i = 0; i < This->num_contained_transform_states; ++i)
    {
        IWineD3DDevice_SetTransform(device, This->contained_transform_states[i],
                &This->state.transforms[This->contained_transform_states[i]]);
    }

    if (This->changed.primitive_type)
    {
        This->device->updateStateBlock->changed.primitive_type = TRUE;
        This->device->updateStateBlock->state.gl_primitive_type = This->state.gl_primitive_type;
    }

    if (This->changed.indices)
    {
        IWineD3DDevice_SetIndexBuffer(device, (IWineD3DBuffer *)This->state.index_buffer, This->state.index_format);
        IWineD3DDevice_SetBaseVertexIndex(device, This->state.base_vertex_index);
    }

    if (This->changed.vertexDecl && This->state.vertex_declaration)
    {
        IWineD3DDevice_SetVertexDeclaration(device, (IWineD3DVertexDeclaration *)This->state.vertex_declaration);
    }

    if (This->changed.material)
    {
        IWineD3DDevice_SetMaterial(device, &This->state.material);
    }

    if (This->changed.viewport)
    {
        IWineD3DDevice_SetViewport(device, &This->state.viewport);
    }

    if (This->changed.scissorRect)
    {
        IWineD3DDevice_SetScissorRect(device, &This->state.scissor_rect);
    }

    map = This->changed.streamSource;
    for (i = 0; map; map >>= 1, ++i)
    {
        if (map & 1)
            IWineD3DDevice_SetStreamSource(device, i,
                    (IWineD3DBuffer *)This->state.streams[i].buffer,
                    0, This->state.streams[i].stride);
    }

    map = This->changed.streamFreq;
    for (i = 0; map; map >>= 1, ++i)
    {
        if (map & 1)
            IWineD3DDevice_SetStreamSourceFreq(device, i,
                    This->state.streams[i].frequency | This->state.streams[i].flags);
    }

    map = This->changed.textures;
    for (i = 0; map; map >>= 1, ++i)
    {
        DWORD stage;

        if (!(map & 1)) continue;

        stage = i < MAX_FRAGMENT_SAMPLERS ? i : WINED3DVERTEXTEXTURESAMPLER0 + i - MAX_FRAGMENT_SAMPLERS;
        IWineD3DDevice_SetTexture(device, stage, (IWineD3DBaseTexture *)This->state.textures[i]);
    }

    map = This->changed.clipplane;
    for (i = 0; map; map >>= 1, ++i)
    {
        float clip[4];

        if (!(map & 1)) continue;

        clip[0] = This->state.clip_planes[i][0];
        clip[1] = This->state.clip_planes[i][1];
        clip[2] = This->state.clip_planes[i][2];
        clip[3] = This->state.clip_planes[i][3];
        IWineD3DDevice_SetClipPlane(device, i, clip);
    }

    This->device->stateBlock->state.lowest_disabled_stage = MAX_TEXTURES - 1;
    for (i = 0; i < MAX_TEXTURES - 1; ++i)
    {
        if (This->device->stateBlock->state.texture_states[i][WINED3DTSS_COLOROP] == WINED3DTOP_DISABLE)
        {
            This->device->stateBlock->state.lowest_disabled_stage = i;
            break;
        }
    }
    TRACE("(%p) : Applied state block %p ------------------^\n", This, device);

    return WINED3D_OK;
}

void stateblock_init_default_state(IWineD3DStateBlockImpl *stateblock)
{
    IWineD3DDeviceImpl *device = stateblock->device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_state *state = &stateblock->state;
    union {
        WINED3DLINEPATTERN lp;
        DWORD d;
    } lp;
    union {
        float f;
        DWORD d;
    } tmpfloat;
    unsigned int i;
    IWineD3DSwapChain *swapchain;
    IWineD3DSurface *backbuffer;
    HRESULT hr;

    TRACE("stateblock %p.\n", stateblock);

    stateblock->blockType = WINED3DSBT_INIT;

    /* Set some of the defaults for lights, transforms etc */
    memcpy(&state->transforms[WINED3DTS_PROJECTION], identity, sizeof(identity));
    memcpy(&state->transforms[WINED3DTS_VIEW], identity, sizeof(identity));
    for (i = 0; i < 256; ++i)
    {
        memcpy(&state->transforms[WINED3DTS_WORLDMATRIX(i)], identity, sizeof(identity));
    }

    TRACE("Render states\n");
    /* Render states: */
    if (device->auto_depth_stencil)
       state->render_states[WINED3DRS_ZENABLE] = WINED3DZB_TRUE;
    else
       state->render_states[WINED3DRS_ZENABLE] = WINED3DZB_FALSE;
    state->render_states[WINED3DRS_FILLMODE] = WINED3DFILL_SOLID;
    state->render_states[WINED3DRS_SHADEMODE] = WINED3DSHADE_GOURAUD;
    lp.lp.wRepeatFactor = 0;
    lp.lp.wLinePattern = 0;
    state->render_states[WINED3DRS_LINEPATTERN] = lp.d;
    state->render_states[WINED3DRS_ZWRITEENABLE] = TRUE;
    state->render_states[WINED3DRS_ALPHATESTENABLE] = FALSE;
    state->render_states[WINED3DRS_LASTPIXEL] = TRUE;
    state->render_states[WINED3DRS_SRCBLEND] = WINED3DBLEND_ONE;
    state->render_states[WINED3DRS_DESTBLEND] = WINED3DBLEND_ZERO;
    state->render_states[WINED3DRS_CULLMODE] = WINED3DCULL_CCW;
    state->render_states[WINED3DRS_ZFUNC] = WINED3DCMP_LESSEQUAL;
    state->render_states[WINED3DRS_ALPHAFUNC] = WINED3DCMP_ALWAYS;
    state->render_states[WINED3DRS_ALPHAREF] = 0;
    state->render_states[WINED3DRS_DITHERENABLE] = FALSE;
    state->render_states[WINED3DRS_ALPHABLENDENABLE] = FALSE;
    state->render_states[WINED3DRS_FOGENABLE] = FALSE;
    state->render_states[WINED3DRS_SPECULARENABLE] = FALSE;
    state->render_states[WINED3DRS_ZVISIBLE] = 0;
    state->render_states[WINED3DRS_FOGCOLOR] = 0;
    state->render_states[WINED3DRS_FOGTABLEMODE] = WINED3DFOG_NONE;
    tmpfloat.f = 0.0f;
    state->render_states[WINED3DRS_FOGSTART] = tmpfloat.d;
    tmpfloat.f = 1.0f;
    state->render_states[WINED3DRS_FOGEND] = tmpfloat.d;
    tmpfloat.f = 1.0f;
    state->render_states[WINED3DRS_FOGDENSITY] = tmpfloat.d;
    state->render_states[WINED3DRS_EDGEANTIALIAS] = FALSE;
    state->render_states[WINED3DRS_ZBIAS] = 0;
    state->render_states[WINED3DRS_RANGEFOGENABLE] = FALSE;
    state->render_states[WINED3DRS_STENCILENABLE] = FALSE;
    state->render_states[WINED3DRS_STENCILFAIL] = WINED3DSTENCILOP_KEEP;
    state->render_states[WINED3DRS_STENCILZFAIL] = WINED3DSTENCILOP_KEEP;
    state->render_states[WINED3DRS_STENCILPASS] = WINED3DSTENCILOP_KEEP;
    state->render_states[WINED3DRS_STENCILREF] = 0;
    state->render_states[WINED3DRS_STENCILMASK] = 0xffffffff;
    state->render_states[WINED3DRS_STENCILFUNC] = WINED3DCMP_ALWAYS;
    state->render_states[WINED3DRS_STENCILWRITEMASK] = 0xffffffff;
    state->render_states[WINED3DRS_TEXTUREFACTOR] = 0xffffffff;
    state->render_states[WINED3DRS_WRAP0] = 0;
    state->render_states[WINED3DRS_WRAP1] = 0;
    state->render_states[WINED3DRS_WRAP2] = 0;
    state->render_states[WINED3DRS_WRAP3] = 0;
    state->render_states[WINED3DRS_WRAP4] = 0;
    state->render_states[WINED3DRS_WRAP5] = 0;
    state->render_states[WINED3DRS_WRAP6] = 0;
    state->render_states[WINED3DRS_WRAP7] = 0;
    state->render_states[WINED3DRS_CLIPPING] = TRUE;
    state->render_states[WINED3DRS_LIGHTING] = TRUE;
    state->render_states[WINED3DRS_AMBIENT] = 0;
    state->render_states[WINED3DRS_FOGVERTEXMODE] = WINED3DFOG_NONE;
    state->render_states[WINED3DRS_COLORVERTEX] = TRUE;
    state->render_states[WINED3DRS_LOCALVIEWER] = TRUE;
    state->render_states[WINED3DRS_NORMALIZENORMALS] = FALSE;
    state->render_states[WINED3DRS_DIFFUSEMATERIALSOURCE] = WINED3DMCS_COLOR1;
    state->render_states[WINED3DRS_SPECULARMATERIALSOURCE] = WINED3DMCS_COLOR2;
    state->render_states[WINED3DRS_AMBIENTMATERIALSOURCE] = WINED3DMCS_MATERIAL;
    state->render_states[WINED3DRS_EMISSIVEMATERIALSOURCE] = WINED3DMCS_MATERIAL;
    state->render_states[WINED3DRS_VERTEXBLEND] = WINED3DVBF_DISABLE;
    state->render_states[WINED3DRS_CLIPPLANEENABLE] = 0;
    state->render_states[WINED3DRS_SOFTWAREVERTEXPROCESSING] = FALSE;
    tmpfloat.f = 1.0f;
    state->render_states[WINED3DRS_POINTSIZE] = tmpfloat.d;
    tmpfloat.f = 1.0f;
    state->render_states[WINED3DRS_POINTSIZE_MIN] = tmpfloat.d;
    state->render_states[WINED3DRS_POINTSPRITEENABLE] = FALSE;
    state->render_states[WINED3DRS_POINTSCALEENABLE] = FALSE;
    tmpfloat.f = 1.0f;
    state->render_states[WINED3DRS_POINTSCALE_A] = tmpfloat.d;
    tmpfloat.f = 0.0f;
    state->render_states[WINED3DRS_POINTSCALE_B] = tmpfloat.d;
    tmpfloat.f = 0.0f;
    state->render_states[WINED3DRS_POINTSCALE_C] = tmpfloat.d;
    state->render_states[WINED3DRS_MULTISAMPLEANTIALIAS] = TRUE;
    state->render_states[WINED3DRS_MULTISAMPLEMASK] = 0xffffffff;
    state->render_states[WINED3DRS_PATCHEDGESTYLE] = WINED3DPATCHEDGE_DISCRETE;
    tmpfloat.f = 1.0f;
    state->render_states[WINED3DRS_PATCHSEGMENTS] = tmpfloat.d;
    state->render_states[WINED3DRS_DEBUGMONITORTOKEN] = 0xbaadcafe;
    tmpfloat.f = gl_info->limits.pointsize_max;
    state->render_states[WINED3DRS_POINTSIZE_MAX] = tmpfloat.d;
    state->render_states[WINED3DRS_INDEXEDVERTEXBLENDENABLE] = FALSE;
    state->render_states[WINED3DRS_COLORWRITEENABLE] = 0x0000000f;
    tmpfloat.f = 0.0f;
    state->render_states[WINED3DRS_TWEENFACTOR] = tmpfloat.d;
    state->render_states[WINED3DRS_BLENDOP] = WINED3DBLENDOP_ADD;
    state->render_states[WINED3DRS_POSITIONDEGREE] = WINED3DDEGREE_CUBIC;
    state->render_states[WINED3DRS_NORMALDEGREE] = WINED3DDEGREE_LINEAR;
    /* states new in d3d9 */
    state->render_states[WINED3DRS_SCISSORTESTENABLE] = FALSE;
    state->render_states[WINED3DRS_SLOPESCALEDEPTHBIAS] = 0;
    tmpfloat.f = 1.0f;
    state->render_states[WINED3DRS_MINTESSELLATIONLEVEL] = tmpfloat.d;
    state->render_states[WINED3DRS_MAXTESSELLATIONLEVEL] = tmpfloat.d;
    state->render_states[WINED3DRS_ANTIALIASEDLINEENABLE] = FALSE;
    tmpfloat.f = 0.0f;
    state->render_states[WINED3DRS_ADAPTIVETESS_X] = tmpfloat.d;
    state->render_states[WINED3DRS_ADAPTIVETESS_Y] = tmpfloat.d;
    tmpfloat.f = 1.0f;
    state->render_states[WINED3DRS_ADAPTIVETESS_Z] = tmpfloat.d;
    tmpfloat.f = 0.0f;
    state->render_states[WINED3DRS_ADAPTIVETESS_W] = tmpfloat.d;
    state->render_states[WINED3DRS_ENABLEADAPTIVETESSELLATION] = FALSE;
    state->render_states[WINED3DRS_TWOSIDEDSTENCILMODE] = FALSE;
    state->render_states[WINED3DRS_CCW_STENCILFAIL] = WINED3DSTENCILOP_KEEP;
    state->render_states[WINED3DRS_CCW_STENCILZFAIL] = WINED3DSTENCILOP_KEEP;
    state->render_states[WINED3DRS_CCW_STENCILPASS] = WINED3DSTENCILOP_KEEP;
    state->render_states[WINED3DRS_CCW_STENCILFUNC] = WINED3DCMP_ALWAYS;
    state->render_states[WINED3DRS_COLORWRITEENABLE1] = 0x0000000f;
    state->render_states[WINED3DRS_COLORWRITEENABLE2] = 0x0000000f;
    state->render_states[WINED3DRS_COLORWRITEENABLE3] = 0x0000000f;
    state->render_states[WINED3DRS_BLENDFACTOR] = 0xFFFFFFFF;
    state->render_states[WINED3DRS_SRGBWRITEENABLE] = 0;
    state->render_states[WINED3DRS_DEPTHBIAS] = 0;
    state->render_states[WINED3DRS_WRAP8] = 0;
    state->render_states[WINED3DRS_WRAP9] = 0;
    state->render_states[WINED3DRS_WRAP10] = 0;
    state->render_states[WINED3DRS_WRAP11] = 0;
    state->render_states[WINED3DRS_WRAP12] = 0;
    state->render_states[WINED3DRS_WRAP13] = 0;
    state->render_states[WINED3DRS_WRAP14] = 0;
    state->render_states[WINED3DRS_WRAP15] = 0;
    state->render_states[WINED3DRS_SEPARATEALPHABLENDENABLE] = FALSE;
    state->render_states[WINED3DRS_SRCBLENDALPHA] = WINED3DBLEND_ONE;
    state->render_states[WINED3DRS_DESTBLENDALPHA] = WINED3DBLEND_ZERO;
    state->render_states[WINED3DRS_BLENDOPALPHA] = WINED3DBLENDOP_ADD;

    /* clipping status */
    state->clip_status.ClipUnion = 0;
    state->clip_status.ClipIntersection = 0xFFFFFFFF;

    /* Texture Stage States - Put directly into state block, we will call function below */
    for (i = 0; i < MAX_TEXTURES; ++i)
    {
        TRACE("Setting up default texture states for texture Stage %u.\n", i);
        memcpy(&state->transforms[WINED3DTS_TEXTURE0 + i], identity, sizeof(identity));
        state->texture_states[i][WINED3DTSS_COLOROP] = i ? WINED3DTOP_DISABLE : WINED3DTOP_MODULATE;
        state->texture_states[i][WINED3DTSS_COLORARG1] = WINED3DTA_TEXTURE;
        state->texture_states[i][WINED3DTSS_COLORARG2] = WINED3DTA_CURRENT;
        state->texture_states[i][WINED3DTSS_ALPHAOP] = i ? WINED3DTOP_DISABLE : WINED3DTOP_SELECTARG1;
        state->texture_states[i][WINED3DTSS_ALPHAARG1] = WINED3DTA_TEXTURE;
        state->texture_states[i][WINED3DTSS_ALPHAARG2] = WINED3DTA_CURRENT;
        state->texture_states[i][WINED3DTSS_BUMPENVMAT00] = 0;
        state->texture_states[i][WINED3DTSS_BUMPENVMAT01] = 0;
        state->texture_states[i][WINED3DTSS_BUMPENVMAT10] = 0;
        state->texture_states[i][WINED3DTSS_BUMPENVMAT11] = 0;
        state->texture_states[i][WINED3DTSS_TEXCOORDINDEX] = i;
        state->texture_states[i][WINED3DTSS_BUMPENVLSCALE] = 0;
        state->texture_states[i][WINED3DTSS_BUMPENVLOFFSET] = 0;
        state->texture_states[i][WINED3DTSS_TEXTURETRANSFORMFLAGS] = WINED3DTTFF_DISABLE;
        state->texture_states[i][WINED3DTSS_COLORARG0] = WINED3DTA_CURRENT;
        state->texture_states[i][WINED3DTSS_ALPHAARG0] = WINED3DTA_CURRENT;
        state->texture_states[i][WINED3DTSS_RESULTARG] = WINED3DTA_CURRENT;
    }
    state->lowest_disabled_stage = 1;

        /* Sampler states*/
    for (i = 0 ; i <  MAX_COMBINED_SAMPLERS; ++i)
    {
        TRACE("Setting up default samplers states for sampler %u.\n", i);
        state->sampler_states[i][WINED3DSAMP_ADDRESSU] = WINED3DTADDRESS_WRAP;
        state->sampler_states[i][WINED3DSAMP_ADDRESSV] = WINED3DTADDRESS_WRAP;
        state->sampler_states[i][WINED3DSAMP_ADDRESSW] = WINED3DTADDRESS_WRAP;
        state->sampler_states[i][WINED3DSAMP_BORDERCOLOR] = 0;
        state->sampler_states[i][WINED3DSAMP_MAGFILTER] = WINED3DTEXF_POINT;
        state->sampler_states[i][WINED3DSAMP_MINFILTER] = WINED3DTEXF_POINT;
        state->sampler_states[i][WINED3DSAMP_MIPFILTER] = WINED3DTEXF_NONE;
        state->sampler_states[i][WINED3DSAMP_MIPMAPLODBIAS] = 0;
        state->sampler_states[i][WINED3DSAMP_MAXMIPLEVEL] = 0;
        state->sampler_states[i][WINED3DSAMP_MAXANISOTROPY] = 1;
        state->sampler_states[i][WINED3DSAMP_SRGBTEXTURE] = 0;
        /* TODO: Indicates which element of a multielement texture to use. */
        state->sampler_states[i][WINED3DSAMP_ELEMENTINDEX] = 0;
        /* TODO: Vertex offset in the presampled displacement map. */
        state->sampler_states[i][WINED3DSAMP_DMAPOFFSET] = 0;
    }

    for (i = 0; i < gl_info->limits.textures; ++i)
    {
        state->textures[i] = NULL;
    }

    /* check the return values, because the GetBackBuffer call isn't valid for ddraw */
    hr = IWineD3DDevice_GetSwapChain((IWineD3DDevice *)device, 0, &swapchain);
    if (SUCCEEDED(hr) && swapchain)
    {
        hr = IWineD3DSwapChain_GetBackBuffer(swapchain, 0, WINED3DBACKBUFFER_TYPE_MONO, &backbuffer);
        if (SUCCEEDED(hr) && backbuffer)
        {
            WINED3DSURFACE_DESC desc;

            IWineD3DSurface_GetDesc(backbuffer, &desc);
            IWineD3DSurface_Release(backbuffer);

            /* Set the default scissor rect values */
            state->scissor_rect.left = 0;
            state->scissor_rect.right = desc.width;
            state->scissor_rect.top = 0;
            state->scissor_rect.bottom = desc.height;
        }

        /* Set the default viewport */
        state->viewport.X = 0;
        state->viewport.Y = 0;
        state->viewport.Width = ((IWineD3DSwapChainImpl *)swapchain)->presentParms.BackBufferWidth;
        state->viewport.Height = ((IWineD3DSwapChainImpl *)swapchain)->presentParms.BackBufferHeight;
        state->viewport.MinZ = 0.0f;
        state->viewport.MaxZ = 1.0f;

        IWineD3DSwapChain_Release(swapchain);
    }

    TRACE("Done.\n");
}

/**********************************************************
 * IWineD3DStateBlock VTbl follows
 **********************************************************/

static const IWineD3DStateBlockVtbl IWineD3DStateBlock_Vtbl =
{
    /* IUnknown */
    IWineD3DStateBlockImpl_QueryInterface,
    IWineD3DStateBlockImpl_AddRef,
    IWineD3DStateBlockImpl_Release,
    /* IWineD3DStateBlock */
    IWineD3DStateBlockImpl_Capture,
    IWineD3DStateBlockImpl_Apply,
};

HRESULT stateblock_init(IWineD3DStateBlockImpl *stateblock, IWineD3DDeviceImpl *device, WINED3DSTATEBLOCKTYPE type)
{
    unsigned int i;
    HRESULT hr;

    stateblock->lpVtbl = &IWineD3DStateBlock_Vtbl;
    stateblock->ref = 1;
    stateblock->device = device;
    stateblock->blockType = type;

    for (i = 0; i < LIGHTMAP_SIZE; i++)
    {
        list_init(&stateblock->state.light_map[i]);
    }

    hr = stateblock_allocate_shader_constants(stateblock);
    if (FAILED(hr)) return hr;

    /* The WINED3DSBT_INIT stateblock type is used during initialization to
     * produce a placeholder stateblock so other functions called can update a
     * state block. */
    if (type == WINED3DSBT_INIT || type == WINED3DSBT_RECORDED) return WINED3D_OK;

    TRACE("Updating changed flags appropriate for type %#x.\n", type);

    switch (type)
    {
        case WINED3DSBT_ALL:
            stateblock_init_lights(stateblock, device->stateBlock->state.light_map);
            stateblock_savedstates_set_all(&stateblock->changed, device->d3d_vshader_constantF,
                                           device->d3d_pshader_constantF);
            break;

        case WINED3DSBT_PIXELSTATE:
            stateblock_savedstates_set_pixel(&stateblock->changed, device->d3d_pshader_constantF);
            break;

        case WINED3DSBT_VERTEXSTATE:
            stateblock_init_lights(stateblock, device->stateBlock->state.light_map);
            stateblock_savedstates_set_vertex(&stateblock->changed, device->d3d_vshader_constantF);
            break;

        default:
            FIXME("Unrecognized state block type %#x.\n", type);
            break;
    }

    stateblock_init_contained_states(stateblock);
    IWineD3DStateBlockImpl_Capture((IWineD3DStateBlock *)stateblock);

    return WINED3D_OK;
}
