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

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

static const DWORD pixel_states_render[] =
{
    WINED3D_RS_ALPHABLENDENABLE,
    WINED3D_RS_ALPHAFUNC,
    WINED3D_RS_ALPHAREF,
    WINED3D_RS_ALPHATESTENABLE,
    WINED3D_RS_ANTIALIASEDLINEENABLE,
    WINED3D_RS_BLENDFACTOR,
    WINED3D_RS_BLENDOP,
    WINED3D_RS_BLENDOPALPHA,
    WINED3D_RS_CCW_STENCILFAIL,
    WINED3D_RS_CCW_STENCILPASS,
    WINED3D_RS_CCW_STENCILZFAIL,
    WINED3D_RS_COLORWRITEENABLE,
    WINED3D_RS_COLORWRITEENABLE1,
    WINED3D_RS_COLORWRITEENABLE2,
    WINED3D_RS_COLORWRITEENABLE3,
    WINED3D_RS_DEPTHBIAS,
    WINED3D_RS_DESTBLEND,
    WINED3D_RS_DESTBLENDALPHA,
    WINED3D_RS_DITHERENABLE,
    WINED3D_RS_FILLMODE,
    WINED3D_RS_FOGDENSITY,
    WINED3D_RS_FOGEND,
    WINED3D_RS_FOGSTART,
    WINED3D_RS_LASTPIXEL,
    WINED3D_RS_SCISSORTESTENABLE,
    WINED3D_RS_SEPARATEALPHABLENDENABLE,
    WINED3D_RS_SHADEMODE,
    WINED3D_RS_SLOPESCALEDEPTHBIAS,
    WINED3D_RS_SRCBLEND,
    WINED3D_RS_SRCBLENDALPHA,
    WINED3D_RS_SRGBWRITEENABLE,
    WINED3D_RS_STENCILENABLE,
    WINED3D_RS_STENCILFAIL,
    WINED3D_RS_STENCILFUNC,
    WINED3D_RS_STENCILMASK,
    WINED3D_RS_STENCILPASS,
    WINED3D_RS_STENCILREF,
    WINED3D_RS_STENCILWRITEMASK,
    WINED3D_RS_STENCILZFAIL,
    WINED3D_RS_TEXTUREFACTOR,
    WINED3D_RS_TWOSIDEDSTENCILMODE,
    WINED3D_RS_WRAP0,
    WINED3D_RS_WRAP1,
    WINED3D_RS_WRAP10,
    WINED3D_RS_WRAP11,
    WINED3D_RS_WRAP12,
    WINED3D_RS_WRAP13,
    WINED3D_RS_WRAP14,
    WINED3D_RS_WRAP15,
    WINED3D_RS_WRAP2,
    WINED3D_RS_WRAP3,
    WINED3D_RS_WRAP4,
    WINED3D_RS_WRAP5,
    WINED3D_RS_WRAP6,
    WINED3D_RS_WRAP7,
    WINED3D_RS_WRAP8,
    WINED3D_RS_WRAP9,
    WINED3D_RS_ZENABLE,
    WINED3D_RS_ZFUNC,
    WINED3D_RS_ZWRITEENABLE,
};

static const DWORD pixel_states_texture[] =
{
    WINED3D_TSS_ALPHA_ARG0,
    WINED3D_TSS_ALPHA_ARG1,
    WINED3D_TSS_ALPHA_ARG2,
    WINED3D_TSS_ALPHA_OP,
    WINED3D_TSS_BUMPENV_LOFFSET,
    WINED3D_TSS_BUMPENV_LSCALE,
    WINED3D_TSS_BUMPENV_MAT00,
    WINED3D_TSS_BUMPENV_MAT01,
    WINED3D_TSS_BUMPENV_MAT10,
    WINED3D_TSS_BUMPENV_MAT11,
    WINED3D_TSS_COLOR_ARG0,
    WINED3D_TSS_COLOR_ARG1,
    WINED3D_TSS_COLOR_ARG2,
    WINED3D_TSS_COLOR_OP,
    WINED3D_TSS_RESULT_ARG,
    WINED3D_TSS_TEXCOORD_INDEX,
    WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS,
};

static const DWORD pixel_states_sampler[] =
{
    WINED3D_SAMP_ADDRESS_U,
    WINED3D_SAMP_ADDRESS_V,
    WINED3D_SAMP_ADDRESS_W,
    WINED3D_SAMP_BORDER_COLOR,
    WINED3D_SAMP_MAG_FILTER,
    WINED3D_SAMP_MIN_FILTER,
    WINED3D_SAMP_MIP_FILTER,
    WINED3D_SAMP_MIPMAP_LOD_BIAS,
    WINED3D_SAMP_MAX_MIP_LEVEL,
    WINED3D_SAMP_MAX_ANISOTROPY,
    WINED3D_SAMP_SRGB_TEXTURE,
    WINED3D_SAMP_ELEMENT_INDEX,
};

static const DWORD vertex_states_render[] =
{
    WINED3D_RS_ADAPTIVETESS_W,
    WINED3D_RS_ADAPTIVETESS_X,
    WINED3D_RS_ADAPTIVETESS_Y,
    WINED3D_RS_ADAPTIVETESS_Z,
    WINED3D_RS_AMBIENT,
    WINED3D_RS_AMBIENTMATERIALSOURCE,
    WINED3D_RS_CLIPPING,
    WINED3D_RS_CLIPPLANEENABLE,
    WINED3D_RS_COLORVERTEX,
    WINED3D_RS_CULLMODE,
    WINED3D_RS_DIFFUSEMATERIALSOURCE,
    WINED3D_RS_EMISSIVEMATERIALSOURCE,
    WINED3D_RS_ENABLEADAPTIVETESSELLATION,
    WINED3D_RS_FOGCOLOR,
    WINED3D_RS_FOGDENSITY,
    WINED3D_RS_FOGENABLE,
    WINED3D_RS_FOGEND,
    WINED3D_RS_FOGSTART,
    WINED3D_RS_FOGTABLEMODE,
    WINED3D_RS_FOGVERTEXMODE,
    WINED3D_RS_INDEXEDVERTEXBLENDENABLE,
    WINED3D_RS_LIGHTING,
    WINED3D_RS_LOCALVIEWER,
    WINED3D_RS_MAXTESSELLATIONLEVEL,
    WINED3D_RS_MINTESSELLATIONLEVEL,
    WINED3D_RS_MULTISAMPLEANTIALIAS,
    WINED3D_RS_MULTISAMPLEMASK,
    WINED3D_RS_NORMALDEGREE,
    WINED3D_RS_NORMALIZENORMALS,
    WINED3D_RS_PATCHEDGESTYLE,
    WINED3D_RS_POINTSCALE_A,
    WINED3D_RS_POINTSCALE_B,
    WINED3D_RS_POINTSCALE_C,
    WINED3D_RS_POINTSCALEENABLE,
    WINED3D_RS_POINTSIZE,
    WINED3D_RS_POINTSIZE_MAX,
    WINED3D_RS_POINTSIZE_MIN,
    WINED3D_RS_POINTSPRITEENABLE,
    WINED3D_RS_POSITIONDEGREE,
    WINED3D_RS_RANGEFOGENABLE,
    WINED3D_RS_SHADEMODE,
    WINED3D_RS_SPECULARENABLE,
    WINED3D_RS_SPECULARMATERIALSOURCE,
    WINED3D_RS_TWEENFACTOR,
    WINED3D_RS_VERTEXBLEND,
};

static const DWORD vertex_states_texture[] =
{
    WINED3D_TSS_TEXCOORD_INDEX,
    WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS,
};

static const DWORD vertex_states_sampler[] =
{
    WINED3D_SAMP_DMAP_OFFSET,
};

/* Allocates the correct amount of space for pixel and vertex shader constants,
 * along with their set/changed flags on the given stateblock object
 */
static HRESULT stateblock_allocate_shader_constants(struct wined3d_stateblock *object)
{
    const struct wined3d_d3d_info *d3d_info = &object->device->adapter->d3d_info;

    object->changed.pixelShaderConstantsF = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(BOOL) * d3d_info->limits.ps_uniform_count);
    if (!object->changed.pixelShaderConstantsF) goto fail;

    object->changed.vertexShaderConstantsF = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(BOOL) * d3d_info->limits.vs_uniform_count);
    if (!object->changed.vertexShaderConstantsF) goto fail;

    object->contained_vs_consts_f = HeapAlloc(GetProcessHeap(), 0,
            sizeof(DWORD) * d3d_info->limits.vs_uniform_count);
    if (!object->contained_vs_consts_f) goto fail;

    object->contained_ps_consts_f = HeapAlloc(GetProcessHeap(), 0,
            sizeof(DWORD) * d3d_info->limits.ps_uniform_count);
    if (!object->contained_ps_consts_f) goto fail;

    return WINED3D_OK;

fail:
    ERR("Failed to allocate memory\n");
    HeapFree(GetProcessHeap(), 0, object->changed.pixelShaderConstantsF);
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
static void stateblock_savedstates_set_all(struct wined3d_saved_states *states, DWORD vs_consts, DWORD ps_consts)
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

static void stateblock_savedstates_set_pixel(struct wined3d_saved_states *states, const DWORD num_constants)
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

static void stateblock_savedstates_set_vertex(struct wined3d_saved_states *states, const DWORD num_constants)
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

void stateblock_init_contained_states(struct wined3d_stateblock *stateblock)
{
    const struct wined3d_d3d_info *d3d_info = &stateblock->device->adapter->d3d_info;
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

    for (i = 0; i < d3d_info->limits.vs_uniform_count; ++i)
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

    for (i = 0; i < d3d_info->limits.ps_uniform_count; ++i)
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

static void stateblock_init_lights(struct wined3d_stateblock *stateblock, struct list *light_map)
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

ULONG CDECL wined3d_stateblock_incref(struct wined3d_stateblock *stateblock)
{
    ULONG refcount = InterlockedIncrement(&stateblock->ref);

    TRACE("%p increasing refcount to %u.\n", stateblock, refcount);

    return refcount;
}

void state_unbind_resources(struct wined3d_state *state)
{
    struct wined3d_shader_resource_view *srv;
    struct wined3d_vertex_declaration *decl;
    struct wined3d_sampler *sampler;
    struct wined3d_texture *texture;
    struct wined3d_buffer *buffer;
    struct wined3d_shader *shader;
    unsigned int i, j;

    if ((decl = state->vertex_declaration))
    {
        state->vertex_declaration = NULL;
        wined3d_vertex_declaration_decref(decl);
    }

    for (i = 0; i < MAX_COMBINED_SAMPLERS; ++i)
    {
        if ((texture = state->textures[i]))
        {
            state->textures[i] = NULL;
            wined3d_texture_decref(texture);
        }
    }

    for (i = 0; i < MAX_STREAM_OUT; ++i)
    {
        if ((buffer = state->stream_output[i].buffer))
        {
            state->stream_output[i].buffer = NULL;
            wined3d_buffer_decref(buffer);
        }
    }

    for (i = 0; i < MAX_STREAMS; ++i)
    {
        if ((buffer = state->streams[i].buffer))
        {
            state->streams[i].buffer = NULL;
            wined3d_buffer_decref(buffer);
        }
    }

    if ((buffer = state->index_buffer))
    {
        state->index_buffer = NULL;
        wined3d_buffer_decref(buffer);
    }

    for (i = 0; i < WINED3D_SHADER_TYPE_COUNT; ++i)
    {
        if ((shader = state->shader[i]))
        {
            state->shader[i] = NULL;
            wined3d_shader_decref(shader);
        }

        for (j = 0; j < MAX_CONSTANT_BUFFERS; ++j)
        {
            if ((buffer = state->cb[i][j]))
            {
                state->cb[i][j] = NULL;
                wined3d_buffer_decref(buffer);
            }
        }

        for (j = 0; j < MAX_SAMPLER_OBJECTS; ++j)
        {
            if ((sampler = state->sampler[i][j]))
            {
                state->sampler[i][j] = NULL;
                wined3d_sampler_decref(sampler);
            }
        }

        for (j = 0; j < MAX_SHADER_RESOURCE_VIEWS; ++j)
        {
            if ((srv = state->shader_resource_view[i][j]))
            {
                state->shader_resource_view[i][j] = NULL;
                wined3d_shader_resource_view_decref(srv);
            }
        }
    }
}

void state_cleanup(struct wined3d_state *state)
{
    unsigned int counter;

    if (!(state->flags & WINED3D_STATE_NO_REF))
        state_unbind_resources(state);

    for (counter = 0; counter < MAX_ACTIVE_LIGHTS; ++counter)
    {
        state->lights[counter] = NULL;
    }

    for (counter = 0; counter < LIGHTMAP_SIZE; ++counter)
    {
        struct list *e1, *e2;
        LIST_FOR_EACH_SAFE(e1, e2, &state->light_map[counter])
        {
            struct wined3d_light_info *light = LIST_ENTRY(e1, struct wined3d_light_info, entry);
            list_remove(&light->entry);
            HeapFree(GetProcessHeap(), 0, light);
        }
    }

    HeapFree(GetProcessHeap(), 0, state->vs_consts_f);
    HeapFree(GetProcessHeap(), 0, state->ps_consts_f);
}

ULONG CDECL wined3d_stateblock_decref(struct wined3d_stateblock *stateblock)
{
    ULONG refcount = InterlockedDecrement(&stateblock->ref);

    TRACE("%p decreasing refcount to %u\n", stateblock, refcount);

    if (!refcount)
    {
        state_cleanup(&stateblock->state);

        HeapFree(GetProcessHeap(), 0, stateblock->changed.vertexShaderConstantsF);
        HeapFree(GetProcessHeap(), 0, stateblock->changed.pixelShaderConstantsF);
        HeapFree(GetProcessHeap(), 0, stateblock->contained_vs_consts_f);
        HeapFree(GetProcessHeap(), 0, stateblock->contained_ps_consts_f);
        HeapFree(GetProcessHeap(), 0, stateblock);
    }

    return refcount;
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

void CDECL wined3d_stateblock_capture(struct wined3d_stateblock *stateblock)
{
    const struct wined3d_state *src_state = &stateblock->device->state;
    unsigned int i;
    DWORD map;

    TRACE("stateblock %p.\n", stateblock);

    TRACE("Capturing state %p.\n", src_state);

    if (stateblock->changed.vertexShader && stateblock->state.shader[WINED3D_SHADER_TYPE_VERTEX]
            != src_state->shader[WINED3D_SHADER_TYPE_VERTEX])
    {
        TRACE("Updating vertex shader from %p to %p\n",
                stateblock->state.shader[WINED3D_SHADER_TYPE_VERTEX],
                src_state->shader[WINED3D_SHADER_TYPE_VERTEX]);

        if (src_state->shader[WINED3D_SHADER_TYPE_VERTEX])
            wined3d_shader_incref(src_state->shader[WINED3D_SHADER_TYPE_VERTEX]);
        if (stateblock->state.shader[WINED3D_SHADER_TYPE_VERTEX])
            wined3d_shader_decref(stateblock->state.shader[WINED3D_SHADER_TYPE_VERTEX]);
        stateblock->state.shader[WINED3D_SHADER_TYPE_VERTEX] = src_state->shader[WINED3D_SHADER_TYPE_VERTEX];
    }

    /* Vertex shader float constants. */
    for (i = 0; i < stateblock->num_contained_vs_consts_f; ++i)
    {
        unsigned int idx = stateblock->contained_vs_consts_f[i];

        TRACE("Setting vs_consts_f[%u] to {%.8e, %.8e, %.8e, %.8e}.\n", idx,
                src_state->vs_consts_f[idx * 4 + 0],
                src_state->vs_consts_f[idx * 4 + 1],
                src_state->vs_consts_f[idx * 4 + 2],
                src_state->vs_consts_f[idx * 4 + 3]);

        stateblock->state.vs_consts_f[idx * 4 + 0] = src_state->vs_consts_f[idx * 4 + 0];
        stateblock->state.vs_consts_f[idx * 4 + 1] = src_state->vs_consts_f[idx * 4 + 1];
        stateblock->state.vs_consts_f[idx * 4 + 2] = src_state->vs_consts_f[idx * 4 + 2];
        stateblock->state.vs_consts_f[idx * 4 + 3] = src_state->vs_consts_f[idx * 4 + 3];
    }

    /* Vertex shader integer constants. */
    for (i = 0; i < stateblock->num_contained_vs_consts_i; ++i)
    {
        unsigned int idx = stateblock->contained_vs_consts_i[i];

        TRACE("Setting vs_consts[%u] to {%d, %d, %d, %d}.\n", idx,
                src_state->vs_consts_i[idx * 4 + 0],
                src_state->vs_consts_i[idx * 4 + 1],
                src_state->vs_consts_i[idx * 4 + 2],
                src_state->vs_consts_i[idx * 4 + 3]);

        stateblock->state.vs_consts_i[idx * 4 + 0] = src_state->vs_consts_i[idx * 4 + 0];
        stateblock->state.vs_consts_i[idx * 4 + 1] = src_state->vs_consts_i[idx * 4 + 1];
        stateblock->state.vs_consts_i[idx * 4 + 2] = src_state->vs_consts_i[idx * 4 + 2];
        stateblock->state.vs_consts_i[idx * 4 + 3] = src_state->vs_consts_i[idx * 4 + 3];
    }

    /* Vertex shader boolean constants. */
    for (i = 0; i < stateblock->num_contained_vs_consts_b; ++i)
    {
        unsigned int idx = stateblock->contained_vs_consts_b[i];

        TRACE("Setting vs_consts_b[%u] to %s.\n",
                idx, src_state->vs_consts_b[idx] ? "TRUE" : "FALSE");

        stateblock->state.vs_consts_b[idx] = src_state->vs_consts_b[idx];
    }

    /* Pixel shader float constants. */
    for (i = 0; i < stateblock->num_contained_ps_consts_f; ++i)
    {
        unsigned int idx = stateblock->contained_ps_consts_f[i];

        TRACE("Setting ps_consts_f[%u] to {%.8e, %.8e, %.8e, %.8e}.\n", idx,
                src_state->ps_consts_f[idx * 4 + 0],
                src_state->ps_consts_f[idx * 4 + 1],
                src_state->ps_consts_f[idx * 4 + 2],
                src_state->ps_consts_f[idx * 4 + 3]);

        stateblock->state.ps_consts_f[idx * 4 + 0] = src_state->ps_consts_f[idx * 4 + 0];
        stateblock->state.ps_consts_f[idx * 4 + 1] = src_state->ps_consts_f[idx * 4 + 1];
        stateblock->state.ps_consts_f[idx * 4 + 2] = src_state->ps_consts_f[idx * 4 + 2];
        stateblock->state.ps_consts_f[idx * 4 + 3] = src_state->ps_consts_f[idx * 4 + 3];
    }

    /* Pixel shader integer constants. */
    for (i = 0; i < stateblock->num_contained_ps_consts_i; ++i)
    {
        unsigned int idx = stateblock->contained_ps_consts_i[i];
        TRACE("Setting ps_consts_i[%u] to {%d, %d, %d, %d}.\n", idx,
                src_state->ps_consts_i[idx * 4 + 0],
                src_state->ps_consts_i[idx * 4 + 1],
                src_state->ps_consts_i[idx * 4 + 2],
                src_state->ps_consts_i[idx * 4 + 3]);

        stateblock->state.ps_consts_i[idx * 4 + 0] = src_state->ps_consts_i[idx * 4 + 0];
        stateblock->state.ps_consts_i[idx * 4 + 1] = src_state->ps_consts_i[idx * 4 + 1];
        stateblock->state.ps_consts_i[idx * 4 + 2] = src_state->ps_consts_i[idx * 4 + 2];
        stateblock->state.ps_consts_i[idx * 4 + 3] = src_state->ps_consts_i[idx * 4 + 3];
    }

    /* Pixel shader boolean constants. */
    for (i = 0; i < stateblock->num_contained_ps_consts_b; ++i)
    {
        unsigned int idx = stateblock->contained_ps_consts_b[i];
        TRACE("Setting ps_consts_b[%u] to %s.\n",
                idx, src_state->ps_consts_b[idx] ? "TRUE" : "FALSE");

        stateblock->state.ps_consts_b[idx] = src_state->ps_consts_b[idx];
    }

    /* Others + Render & Texture */
    for (i = 0; i < stateblock->num_contained_transform_states; ++i)
    {
        enum wined3d_transform_state transform = stateblock->contained_transform_states[i];

        TRACE("Updating transform %#x.\n", transform);

        stateblock->state.transforms[transform] = src_state->transforms[transform];
    }

    if (stateblock->changed.primitive_type)
        stateblock->state.gl_primitive_type = src_state->gl_primitive_type;

    if (stateblock->changed.indices
            && ((stateblock->state.index_buffer != src_state->index_buffer)
                || (stateblock->state.base_vertex_index != src_state->base_vertex_index)
                || (stateblock->state.index_format != src_state->index_format)))
    {
        TRACE("Updating index buffer to %p, base vertex index to %d.\n",
                src_state->index_buffer, src_state->base_vertex_index);

        if (src_state->index_buffer)
            wined3d_buffer_incref(src_state->index_buffer);
        if (stateblock->state.index_buffer)
            wined3d_buffer_decref(stateblock->state.index_buffer);
        stateblock->state.index_buffer = src_state->index_buffer;
        stateblock->state.base_vertex_index = src_state->base_vertex_index;
        stateblock->state.index_format = src_state->index_format;
    }

    if (stateblock->changed.vertexDecl && stateblock->state.vertex_declaration != src_state->vertex_declaration)
    {
        TRACE("Updating vertex declaration from %p to %p.\n",
                stateblock->state.vertex_declaration, src_state->vertex_declaration);

        if (src_state->vertex_declaration)
                wined3d_vertex_declaration_incref(src_state->vertex_declaration);
        if (stateblock->state.vertex_declaration)
                wined3d_vertex_declaration_decref(stateblock->state.vertex_declaration);
        stateblock->state.vertex_declaration = src_state->vertex_declaration;
    }

    if (stateblock->changed.material
            && memcmp(&src_state->material, &stateblock->state.material, sizeof(stateblock->state.material)))
    {
        TRACE("Updating material.\n");

        stateblock->state.material = src_state->material;
    }

    if (stateblock->changed.viewport
            && memcmp(&src_state->viewport, &stateblock->state.viewport, sizeof(stateblock->state.viewport)))
    {
        TRACE("Updating viewport.\n");

        stateblock->state.viewport = src_state->viewport;
    }

    if (stateblock->changed.scissorRect && memcmp(&src_state->scissor_rect,
            &stateblock->state.scissor_rect, sizeof(stateblock->state.scissor_rect)))
    {
        TRACE("Updating scissor rect.\n");

        stateblock->state.scissor_rect = src_state->scissor_rect;
    }

    map = stateblock->changed.streamSource;
    for (i = 0; map; map >>= 1, ++i)
    {
        if (!(map & 1)) continue;

        if (stateblock->state.streams[i].stride != src_state->streams[i].stride
                || stateblock->state.streams[i].buffer != src_state->streams[i].buffer)
        {
            TRACE("Updating stream source %u to %p, stride to %u.\n",
                    i, src_state->streams[i].buffer,
                    src_state->streams[i].stride);

            stateblock->state.streams[i].stride = src_state->streams[i].stride;
            if (src_state->streams[i].buffer)
                    wined3d_buffer_incref(src_state->streams[i].buffer);
            if (stateblock->state.streams[i].buffer)
                    wined3d_buffer_decref(stateblock->state.streams[i].buffer);
            stateblock->state.streams[i].buffer = src_state->streams[i].buffer;
        }
    }

    map = stateblock->changed.streamFreq;
    for (i = 0; map; map >>= 1, ++i)
    {
        if (!(map & 1)) continue;

        if (stateblock->state.streams[i].frequency != src_state->streams[i].frequency
                || stateblock->state.streams[i].flags != src_state->streams[i].flags)
        {
            TRACE("Updating stream frequency %u to %u flags to %#x.\n",
                    i, src_state->streams[i].frequency, src_state->streams[i].flags);

            stateblock->state.streams[i].frequency = src_state->streams[i].frequency;
            stateblock->state.streams[i].flags = src_state->streams[i].flags;
        }
    }

    map = stateblock->changed.clipplane;
    for (i = 0; map; map >>= 1, ++i)
    {
        if (!(map & 1)) continue;

        if (memcmp(&stateblock->state.clip_planes[i], &src_state->clip_planes[i], sizeof(src_state->clip_planes[i])))
        {
            TRACE("Updating clipplane %u.\n", i);
            stateblock->state.clip_planes[i] = src_state->clip_planes[i];
        }
    }

    /* Render */
    for (i = 0; i < stateblock->num_contained_render_states; ++i)
    {
        enum wined3d_render_state rs = stateblock->contained_render_states[i];

        TRACE("Updating render state %#x to %u.\n", rs, src_state->render_states[rs]);

        stateblock->state.render_states[rs] = src_state->render_states[rs];
    }

    /* Texture states */
    for (i = 0; i < stateblock->num_contained_tss_states; ++i)
    {
        DWORD stage = stateblock->contained_tss_states[i].stage;
        DWORD state = stateblock->contained_tss_states[i].state;

        TRACE("Updating texturestage state %u, %u to %#x (was %#x).\n", stage, state,
                src_state->texture_states[stage][state], stateblock->state.texture_states[stage][state]);

        stateblock->state.texture_states[stage][state] = src_state->texture_states[stage][state];
    }

    /* Samplers */
    map = stateblock->changed.textures;
    for (i = 0; map; map >>= 1, ++i)
    {
        if (!(map & 1)) continue;

        TRACE("Updating texture %u to %p (was %p).\n",
                i, src_state->textures[i], stateblock->state.textures[i]);

        if (src_state->textures[i])
            wined3d_texture_incref(src_state->textures[i]);
        if (stateblock->state.textures[i])
            wined3d_texture_decref(stateblock->state.textures[i]);
        stateblock->state.textures[i] = src_state->textures[i];
    }

    for (i = 0; i < stateblock->num_contained_sampler_states; ++i)
    {
        DWORD stage = stateblock->contained_sampler_states[i].stage;
        DWORD state = stateblock->contained_sampler_states[i].state;

        TRACE("Updating sampler state %u, %u to %#x (was %#x).\n", stage, state,
                src_state->sampler_states[stage][state], stateblock->state.sampler_states[stage][state]);

        stateblock->state.sampler_states[stage][state] = src_state->sampler_states[stage][state];
    }

    if (stateblock->changed.pixelShader && stateblock->state.shader[WINED3D_SHADER_TYPE_PIXEL]
            != src_state->shader[WINED3D_SHADER_TYPE_PIXEL])
    {
        if (src_state->shader[WINED3D_SHADER_TYPE_PIXEL])
            wined3d_shader_incref(src_state->shader[WINED3D_SHADER_TYPE_PIXEL]);
        if (stateblock->state.shader[WINED3D_SHADER_TYPE_PIXEL])
            wined3d_shader_decref(stateblock->state.shader[WINED3D_SHADER_TYPE_PIXEL]);
        stateblock->state.shader[WINED3D_SHADER_TYPE_PIXEL] = src_state->shader[WINED3D_SHADER_TYPE_PIXEL];
    }

    wined3d_state_record_lights(&stateblock->state, src_state);

    TRACE("Capture done.\n");
}

static void apply_lights(struct wined3d_device *device, const struct wined3d_state *state)
{
    UINT i;

    for (i = 0; i < LIGHTMAP_SIZE; ++i)
    {
        struct list *e;

        LIST_FOR_EACH(e, &state->light_map[i])
        {
            const struct wined3d_light_info *light = LIST_ENTRY(e, struct wined3d_light_info, entry);

            wined3d_device_set_light(device, light->OriginalIndex, &light->OriginalParms);
            wined3d_device_set_light_enable(device, light->OriginalIndex, light->glIndex != -1);
        }
    }
}

void CDECL wined3d_stateblock_apply(const struct wined3d_stateblock *stateblock)
{
    struct wined3d_device *device = stateblock->device;
    unsigned int i;
    DWORD map;

    TRACE("Applying stateblock %p to device %p.\n", stateblock, device);

    if (stateblock->changed.vertexShader)
        wined3d_device_set_vertex_shader(device, stateblock->state.shader[WINED3D_SHADER_TYPE_VERTEX]);

    /* Vertex Shader Constants. */
    for (i = 0; i < stateblock->num_contained_vs_consts_f; ++i)
    {
        wined3d_device_set_vs_consts_f(device, stateblock->contained_vs_consts_f[i],
                stateblock->state.vs_consts_f + stateblock->contained_vs_consts_f[i] * 4, 1);
    }
    for (i = 0; i < stateblock->num_contained_vs_consts_i; ++i)
    {
        wined3d_device_set_vs_consts_i(device, stateblock->contained_vs_consts_i[i],
                stateblock->state.vs_consts_i + stateblock->contained_vs_consts_i[i] * 4, 1);
    }
    for (i = 0; i < stateblock->num_contained_vs_consts_b; ++i)
    {
        wined3d_device_set_vs_consts_b(device, stateblock->contained_vs_consts_b[i],
                stateblock->state.vs_consts_b + stateblock->contained_vs_consts_b[i], 1);
    }

    apply_lights(device, &stateblock->state);

    if (stateblock->changed.pixelShader)
        wined3d_device_set_pixel_shader(device, stateblock->state.shader[WINED3D_SHADER_TYPE_PIXEL]);

    /* Pixel Shader Constants. */
    for (i = 0; i < stateblock->num_contained_ps_consts_f; ++i)
    {
        wined3d_device_set_ps_consts_f(device, stateblock->contained_ps_consts_f[i],
                stateblock->state.ps_consts_f + stateblock->contained_ps_consts_f[i] * 4, 1);
    }
    for (i = 0; i < stateblock->num_contained_ps_consts_i; ++i)
    {
        wined3d_device_set_ps_consts_i(device, stateblock->contained_ps_consts_i[i],
                stateblock->state.ps_consts_i + stateblock->contained_ps_consts_i[i] * 4, 1);
    }
    for (i = 0; i < stateblock->num_contained_ps_consts_b; ++i)
    {
        wined3d_device_set_ps_consts_b(device, stateblock->contained_ps_consts_b[i],
                stateblock->state.ps_consts_b + stateblock->contained_ps_consts_b[i], 1);
    }

    /* Render states. */
    for (i = 0; i < stateblock->num_contained_render_states; ++i)
    {
        wined3d_device_set_render_state(device, stateblock->contained_render_states[i],
                stateblock->state.render_states[stateblock->contained_render_states[i]]);
    }

    /* Texture states. */
    for (i = 0; i < stateblock->num_contained_tss_states; ++i)
    {
        DWORD stage = stateblock->contained_tss_states[i].stage;
        DWORD state = stateblock->contained_tss_states[i].state;

        wined3d_device_set_texture_stage_state(device, stage, state, stateblock->state.texture_states[stage][state]);
    }

    /* Sampler states. */
    for (i = 0; i < stateblock->num_contained_sampler_states; ++i)
    {
        DWORD stage = stateblock->contained_sampler_states[i].stage;
        DWORD state = stateblock->contained_sampler_states[i].state;
        DWORD value = stateblock->state.sampler_states[stage][state];

        if (stage >= MAX_FRAGMENT_SAMPLERS) stage += WINED3DVERTEXTEXTURESAMPLER0 - MAX_FRAGMENT_SAMPLERS;
        wined3d_device_set_sampler_state(device, stage, state, value);
    }

    /* Transform states. */
    for (i = 0; i < stateblock->num_contained_transform_states; ++i)
    {
        wined3d_device_set_transform(device, stateblock->contained_transform_states[i],
                &stateblock->state.transforms[stateblock->contained_transform_states[i]]);
    }

    if (stateblock->changed.primitive_type)
    {
        GLenum gl_primitive_type, prev;

        if (device->recording)
            device->recording->changed.primitive_type = TRUE;
        gl_primitive_type = stateblock->state.gl_primitive_type;
        prev = device->update_state->gl_primitive_type;
        device->update_state->gl_primitive_type = gl_primitive_type;
        if (gl_primitive_type != prev && (gl_primitive_type == GL_POINTS || prev == GL_POINTS))
            device_invalidate_state(device, STATE_POINT_SIZE_ENABLE);
    }

    if (stateblock->changed.indices)
    {
        wined3d_device_set_index_buffer(device, stateblock->state.index_buffer, stateblock->state.index_format);
        wined3d_device_set_base_vertex_index(device, stateblock->state.base_vertex_index);
    }

    if (stateblock->changed.vertexDecl && stateblock->state.vertex_declaration)
        wined3d_device_set_vertex_declaration(device, stateblock->state.vertex_declaration);

    if (stateblock->changed.material)
        wined3d_device_set_material(device, &stateblock->state.material);

    if (stateblock->changed.viewport)
        wined3d_device_set_viewport(device, &stateblock->state.viewport);

    if (stateblock->changed.scissorRect)
        wined3d_device_set_scissor_rect(device, &stateblock->state.scissor_rect);

    map = stateblock->changed.streamSource;
    for (i = 0; map; map >>= 1, ++i)
    {
        if (map & 1)
            wined3d_device_set_stream_source(device, i,
                    stateblock->state.streams[i].buffer,
                    0, stateblock->state.streams[i].stride);
    }

    map = stateblock->changed.streamFreq;
    for (i = 0; map; map >>= 1, ++i)
    {
        if (map & 1)
            wined3d_device_set_stream_source_freq(device, i,
                    stateblock->state.streams[i].frequency | stateblock->state.streams[i].flags);
    }

    map = stateblock->changed.textures;
    for (i = 0; map; map >>= 1, ++i)
    {
        DWORD stage;

        if (!(map & 1)) continue;

        stage = i < MAX_FRAGMENT_SAMPLERS ? i : WINED3DVERTEXTEXTURESAMPLER0 + i - MAX_FRAGMENT_SAMPLERS;
        wined3d_device_set_texture(device, stage, stateblock->state.textures[i]);
    }

    map = stateblock->changed.clipplane;
    for (i = 0; map; map >>= 1, ++i)
    {
        if (!(map & 1)) continue;

        wined3d_device_set_clip_plane(device, i, &stateblock->state.clip_planes[i]);
    }

    TRACE("Applied stateblock %p.\n", stateblock);
}

static void state_init_default(struct wined3d_state *state, const struct wined3d_gl_info *gl_info)
{
    union
    {
        struct wined3d_line_pattern lp;
        DWORD d;
    } lp;
    union {
        float f;
        DWORD d;
    } tmpfloat;
    unsigned int i;
    static const struct wined3d_matrix identity =
    {{{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    }}};

    TRACE("state %p, gl_info %p.\n", state, gl_info);

    state->gl_primitive_type = ~0u;

    /* Set some of the defaults for lights, transforms etc */
    state->transforms[WINED3D_TS_PROJECTION] = identity;
    state->transforms[WINED3D_TS_VIEW] = identity;
    for (i = 0; i < 256; ++i)
    {
        state->transforms[WINED3D_TS_WORLD_MATRIX(i)] = identity;
    }

    TRACE("Render states\n");
    /* Render states: */
    state->render_states[WINED3D_RS_ZENABLE] = WINED3D_ZB_TRUE;
    state->render_states[WINED3D_RS_FILLMODE] = WINED3D_FILL_SOLID;
    state->render_states[WINED3D_RS_SHADEMODE] = WINED3D_SHADE_GOURAUD;
    lp.lp.repeat_factor = 0;
    lp.lp.line_pattern = 0;
    state->render_states[WINED3D_RS_LINEPATTERN] = lp.d;
    state->render_states[WINED3D_RS_ZWRITEENABLE] = TRUE;
    state->render_states[WINED3D_RS_ALPHATESTENABLE] = FALSE;
    state->render_states[WINED3D_RS_LASTPIXEL] = TRUE;
    state->render_states[WINED3D_RS_SRCBLEND] = WINED3D_BLEND_ONE;
    state->render_states[WINED3D_RS_DESTBLEND] = WINED3D_BLEND_ZERO;
    state->render_states[WINED3D_RS_CULLMODE] = WINED3D_CULL_CCW;
    state->render_states[WINED3D_RS_ZFUNC] = WINED3D_CMP_LESSEQUAL;
    state->render_states[WINED3D_RS_ALPHAFUNC] = WINED3D_CMP_ALWAYS;
    state->render_states[WINED3D_RS_ALPHAREF] = 0;
    state->render_states[WINED3D_RS_DITHERENABLE] = FALSE;
    state->render_states[WINED3D_RS_ALPHABLENDENABLE] = FALSE;
    state->render_states[WINED3D_RS_FOGENABLE] = FALSE;
    state->render_states[WINED3D_RS_SPECULARENABLE] = FALSE;
    state->render_states[WINED3D_RS_ZVISIBLE] = 0;
    state->render_states[WINED3D_RS_FOGCOLOR] = 0;
    state->render_states[WINED3D_RS_FOGTABLEMODE] = WINED3D_FOG_NONE;
    tmpfloat.f = 0.0f;
    state->render_states[WINED3D_RS_FOGSTART] = tmpfloat.d;
    tmpfloat.f = 1.0f;
    state->render_states[WINED3D_RS_FOGEND] = tmpfloat.d;
    tmpfloat.f = 1.0f;
    state->render_states[WINED3D_RS_FOGDENSITY] = tmpfloat.d;
    state->render_states[WINED3D_RS_EDGEANTIALIAS] = FALSE;
    state->render_states[WINED3D_RS_RANGEFOGENABLE] = FALSE;
    state->render_states[WINED3D_RS_STENCILENABLE] = FALSE;
    state->render_states[WINED3D_RS_STENCILFAIL] = WINED3D_STENCIL_OP_KEEP;
    state->render_states[WINED3D_RS_STENCILZFAIL] = WINED3D_STENCIL_OP_KEEP;
    state->render_states[WINED3D_RS_STENCILPASS] = WINED3D_STENCIL_OP_KEEP;
    state->render_states[WINED3D_RS_STENCILREF] = 0;
    state->render_states[WINED3D_RS_STENCILMASK] = 0xffffffff;
    state->render_states[WINED3D_RS_STENCILFUNC] = WINED3D_CMP_ALWAYS;
    state->render_states[WINED3D_RS_STENCILWRITEMASK] = 0xffffffff;
    state->render_states[WINED3D_RS_TEXTUREFACTOR] = 0xffffffff;
    state->render_states[WINED3D_RS_WRAP0] = 0;
    state->render_states[WINED3D_RS_WRAP1] = 0;
    state->render_states[WINED3D_RS_WRAP2] = 0;
    state->render_states[WINED3D_RS_WRAP3] = 0;
    state->render_states[WINED3D_RS_WRAP4] = 0;
    state->render_states[WINED3D_RS_WRAP5] = 0;
    state->render_states[WINED3D_RS_WRAP6] = 0;
    state->render_states[WINED3D_RS_WRAP7] = 0;
    state->render_states[WINED3D_RS_CLIPPING] = TRUE;
    state->render_states[WINED3D_RS_LIGHTING] = TRUE;
    state->render_states[WINED3D_RS_AMBIENT] = 0;
    state->render_states[WINED3D_RS_FOGVERTEXMODE] = WINED3D_FOG_NONE;
    state->render_states[WINED3D_RS_COLORVERTEX] = TRUE;
    state->render_states[WINED3D_RS_LOCALVIEWER] = TRUE;
    state->render_states[WINED3D_RS_NORMALIZENORMALS] = FALSE;
    state->render_states[WINED3D_RS_DIFFUSEMATERIALSOURCE] = WINED3D_MCS_COLOR1;
    state->render_states[WINED3D_RS_SPECULARMATERIALSOURCE] = WINED3D_MCS_COLOR2;
    state->render_states[WINED3D_RS_AMBIENTMATERIALSOURCE] = WINED3D_MCS_MATERIAL;
    state->render_states[WINED3D_RS_EMISSIVEMATERIALSOURCE] = WINED3D_MCS_MATERIAL;
    state->render_states[WINED3D_RS_VERTEXBLEND] = WINED3D_VBF_DISABLE;
    state->render_states[WINED3D_RS_CLIPPLANEENABLE] = 0;
    state->render_states[WINED3D_RS_SOFTWAREVERTEXPROCESSING] = FALSE;
    tmpfloat.f = 1.0f;
    state->render_states[WINED3D_RS_POINTSIZE] = tmpfloat.d;
    tmpfloat.f = 1.0f;
    state->render_states[WINED3D_RS_POINTSIZE_MIN] = tmpfloat.d;
    state->render_states[WINED3D_RS_POINTSPRITEENABLE] = FALSE;
    state->render_states[WINED3D_RS_POINTSCALEENABLE] = FALSE;
    tmpfloat.f = 1.0f;
    state->render_states[WINED3D_RS_POINTSCALE_A] = tmpfloat.d;
    tmpfloat.f = 0.0f;
    state->render_states[WINED3D_RS_POINTSCALE_B] = tmpfloat.d;
    tmpfloat.f = 0.0f;
    state->render_states[WINED3D_RS_POINTSCALE_C] = tmpfloat.d;
    state->render_states[WINED3D_RS_MULTISAMPLEANTIALIAS] = TRUE;
    state->render_states[WINED3D_RS_MULTISAMPLEMASK] = 0xffffffff;
    state->render_states[WINED3D_RS_PATCHEDGESTYLE] = WINED3D_PATCH_EDGE_DISCRETE;
    tmpfloat.f = 1.0f;
    state->render_states[WINED3D_RS_PATCHSEGMENTS] = tmpfloat.d;
    state->render_states[WINED3D_RS_DEBUGMONITORTOKEN] = 0xbaadcafe;
    tmpfloat.f = gl_info->limits.pointsize_max;
    state->render_states[WINED3D_RS_POINTSIZE_MAX] = tmpfloat.d;
    state->render_states[WINED3D_RS_INDEXEDVERTEXBLENDENABLE] = FALSE;
    state->render_states[WINED3D_RS_COLORWRITEENABLE] = 0x0000000f;
    tmpfloat.f = 0.0f;
    state->render_states[WINED3D_RS_TWEENFACTOR] = tmpfloat.d;
    state->render_states[WINED3D_RS_BLENDOP] = WINED3D_BLEND_OP_ADD;
    state->render_states[WINED3D_RS_POSITIONDEGREE] = WINED3D_DEGREE_CUBIC;
    state->render_states[WINED3D_RS_NORMALDEGREE] = WINED3D_DEGREE_LINEAR;
    /* states new in d3d9 */
    state->render_states[WINED3D_RS_SCISSORTESTENABLE] = FALSE;
    state->render_states[WINED3D_RS_SLOPESCALEDEPTHBIAS] = 0;
    tmpfloat.f = 1.0f;
    state->render_states[WINED3D_RS_MINTESSELLATIONLEVEL] = tmpfloat.d;
    state->render_states[WINED3D_RS_MAXTESSELLATIONLEVEL] = tmpfloat.d;
    state->render_states[WINED3D_RS_ANTIALIASEDLINEENABLE] = FALSE;
    tmpfloat.f = 0.0f;
    state->render_states[WINED3D_RS_ADAPTIVETESS_X] = tmpfloat.d;
    state->render_states[WINED3D_RS_ADAPTIVETESS_Y] = tmpfloat.d;
    tmpfloat.f = 1.0f;
    state->render_states[WINED3D_RS_ADAPTIVETESS_Z] = tmpfloat.d;
    tmpfloat.f = 0.0f;
    state->render_states[WINED3D_RS_ADAPTIVETESS_W] = tmpfloat.d;
    state->render_states[WINED3D_RS_ENABLEADAPTIVETESSELLATION] = FALSE;
    state->render_states[WINED3D_RS_TWOSIDEDSTENCILMODE] = FALSE;
    state->render_states[WINED3D_RS_CCW_STENCILFAIL] = WINED3D_STENCIL_OP_KEEP;
    state->render_states[WINED3D_RS_CCW_STENCILZFAIL] = WINED3D_STENCIL_OP_KEEP;
    state->render_states[WINED3D_RS_CCW_STENCILPASS] = WINED3D_STENCIL_OP_KEEP;
    state->render_states[WINED3D_RS_CCW_STENCILFUNC] = WINED3D_CMP_ALWAYS;
    state->render_states[WINED3D_RS_COLORWRITEENABLE1] = 0x0000000f;
    state->render_states[WINED3D_RS_COLORWRITEENABLE2] = 0x0000000f;
    state->render_states[WINED3D_RS_COLORWRITEENABLE3] = 0x0000000f;
    state->render_states[WINED3D_RS_BLENDFACTOR] = 0xffffffff;
    state->render_states[WINED3D_RS_SRGBWRITEENABLE] = 0;
    state->render_states[WINED3D_RS_DEPTHBIAS] = 0;
    state->render_states[WINED3D_RS_WRAP8] = 0;
    state->render_states[WINED3D_RS_WRAP9] = 0;
    state->render_states[WINED3D_RS_WRAP10] = 0;
    state->render_states[WINED3D_RS_WRAP11] = 0;
    state->render_states[WINED3D_RS_WRAP12] = 0;
    state->render_states[WINED3D_RS_WRAP13] = 0;
    state->render_states[WINED3D_RS_WRAP14] = 0;
    state->render_states[WINED3D_RS_WRAP15] = 0;
    state->render_states[WINED3D_RS_SEPARATEALPHABLENDENABLE] = FALSE;
    state->render_states[WINED3D_RS_SRCBLENDALPHA] = WINED3D_BLEND_ONE;
    state->render_states[WINED3D_RS_DESTBLENDALPHA] = WINED3D_BLEND_ZERO;
    state->render_states[WINED3D_RS_BLENDOPALPHA] = WINED3D_BLEND_OP_ADD;

    /* Texture Stage States - Put directly into state block, we will call function below */
    for (i = 0; i < MAX_TEXTURES; ++i)
    {
        TRACE("Setting up default texture states for texture Stage %u.\n", i);
        state->transforms[WINED3D_TS_TEXTURE0 + i] = identity;
        state->texture_states[i][WINED3D_TSS_COLOR_OP] = i ? WINED3D_TOP_DISABLE : WINED3D_TOP_MODULATE;
        state->texture_states[i][WINED3D_TSS_COLOR_ARG1] = WINED3DTA_TEXTURE;
        state->texture_states[i][WINED3D_TSS_COLOR_ARG2] = WINED3DTA_CURRENT;
        state->texture_states[i][WINED3D_TSS_ALPHA_OP] = i ? WINED3D_TOP_DISABLE : WINED3D_TOP_SELECT_ARG1;
        state->texture_states[i][WINED3D_TSS_ALPHA_ARG1] = WINED3DTA_TEXTURE;
        state->texture_states[i][WINED3D_TSS_ALPHA_ARG2] = WINED3DTA_CURRENT;
        state->texture_states[i][WINED3D_TSS_BUMPENV_MAT00] = 0;
        state->texture_states[i][WINED3D_TSS_BUMPENV_MAT01] = 0;
        state->texture_states[i][WINED3D_TSS_BUMPENV_MAT10] = 0;
        state->texture_states[i][WINED3D_TSS_BUMPENV_MAT11] = 0;
        state->texture_states[i][WINED3D_TSS_TEXCOORD_INDEX] = i;
        state->texture_states[i][WINED3D_TSS_BUMPENV_LSCALE] = 0;
        state->texture_states[i][WINED3D_TSS_BUMPENV_LOFFSET] = 0;
        state->texture_states[i][WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS] = WINED3D_TTFF_DISABLE;
        state->texture_states[i][WINED3D_TSS_COLOR_ARG0] = WINED3DTA_CURRENT;
        state->texture_states[i][WINED3D_TSS_ALPHA_ARG0] = WINED3DTA_CURRENT;
        state->texture_states[i][WINED3D_TSS_RESULT_ARG] = WINED3DTA_CURRENT;
    }

    for (i = 0 ; i <  MAX_COMBINED_SAMPLERS; ++i)
    {
        TRACE("Setting up default samplers states for sampler %u.\n", i);
        state->sampler_states[i][WINED3D_SAMP_ADDRESS_U] = WINED3D_TADDRESS_WRAP;
        state->sampler_states[i][WINED3D_SAMP_ADDRESS_V] = WINED3D_TADDRESS_WRAP;
        state->sampler_states[i][WINED3D_SAMP_ADDRESS_W] = WINED3D_TADDRESS_WRAP;
        state->sampler_states[i][WINED3D_SAMP_BORDER_COLOR] = 0;
        state->sampler_states[i][WINED3D_SAMP_MAG_FILTER] = WINED3D_TEXF_POINT;
        state->sampler_states[i][WINED3D_SAMP_MIN_FILTER] = WINED3D_TEXF_POINT;
        state->sampler_states[i][WINED3D_SAMP_MIP_FILTER] = WINED3D_TEXF_NONE;
        state->sampler_states[i][WINED3D_SAMP_MIPMAP_LOD_BIAS] = 0;
        state->sampler_states[i][WINED3D_SAMP_MAX_MIP_LEVEL] = 0;
        state->sampler_states[i][WINED3D_SAMP_MAX_ANISOTROPY] = 1;
        state->sampler_states[i][WINED3D_SAMP_SRGB_TEXTURE] = 0;
        /* TODO: Indicates which element of a multielement texture to use. */
        state->sampler_states[i][WINED3D_SAMP_ELEMENT_INDEX] = 0;
        /* TODO: Vertex offset in the presampled displacement map. */
        state->sampler_states[i][WINED3D_SAMP_DMAP_OFFSET] = 0;
    }
}

HRESULT state_init(struct wined3d_state *state, struct wined3d_fb_state *fb,
        const struct wined3d_gl_info *gl_info, const struct wined3d_d3d_info *d3d_info,
        DWORD flags)
{
    unsigned int i;

    state->flags = flags;
    state->fb = fb;

    for (i = 0; i < LIGHTMAP_SIZE; i++)
    {
        list_init(&state->light_map[i]);
    }

    if (!(state->vs_consts_f = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            4 * sizeof(float) * d3d_info->limits.vs_uniform_count)))
        return E_OUTOFMEMORY;

    if (!(state->ps_consts_f = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            4 * sizeof(float) * d3d_info->limits.ps_uniform_count)))
    {
        HeapFree(GetProcessHeap(), 0, state->vs_consts_f);
        return E_OUTOFMEMORY;
    }

    if (flags & WINED3D_STATE_INIT_DEFAULT)
        state_init_default(state, gl_info);

    return WINED3D_OK;
}

static HRESULT stateblock_init(struct wined3d_stateblock *stateblock,
        struct wined3d_device *device, enum wined3d_stateblock_type type)
{
    HRESULT hr;
    const struct wined3d_d3d_info *d3d_info = &device->adapter->d3d_info;

    stateblock->ref = 1;
    stateblock->device = device;

    if (FAILED(hr = state_init(&stateblock->state, NULL, &device->adapter->gl_info, d3d_info, 0)))
        return hr;

    if (FAILED(hr = stateblock_allocate_shader_constants(stateblock)))
    {
        state_cleanup(&stateblock->state);
        return hr;
    }

    if (type == WINED3D_SBT_RECORDED)
        return WINED3D_OK;

    TRACE("Updating changed flags appropriate for type %#x.\n", type);

    switch (type)
    {
        case WINED3D_SBT_ALL:
            stateblock_init_lights(stateblock, device->state.light_map);
            stateblock_savedstates_set_all(&stateblock->changed,
                    d3d_info->limits.vs_uniform_count, d3d_info->limits.ps_uniform_count);
            break;

        case WINED3D_SBT_PIXEL_STATE:
            stateblock_savedstates_set_pixel(&stateblock->changed,
                    d3d_info->limits.ps_uniform_count);
            break;

        case WINED3D_SBT_VERTEX_STATE:
            stateblock_init_lights(stateblock, device->state.light_map);
            stateblock_savedstates_set_vertex(&stateblock->changed,
                    d3d_info->limits.vs_uniform_count);
            break;

        default:
            FIXME("Unrecognized state block type %#x.\n", type);
            break;
    }

    stateblock_init_contained_states(stateblock);
    wined3d_stateblock_capture(stateblock);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_stateblock_create(struct wined3d_device *device,
        enum wined3d_stateblock_type type, struct wined3d_stateblock **stateblock)
{
    struct wined3d_stateblock *object;
    HRESULT hr;

    TRACE("device %p, type %#x, stateblock %p.\n",
            device, type, stateblock);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
        return E_OUTOFMEMORY;

    hr = stateblock_init(object, device, type);
    if (FAILED(hr))
    {
        WARN("Failed to initialize stateblock, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created stateblock %p.\n", object);
    *stateblock = object;

    return WINED3D_OK;
}
