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
#include "wine/port.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

static const DWORD pixel_states_render[] =
{
    WINED3D_RS_ALPHABLENDENABLE,
    WINED3D_RS_ALPHAFUNC,
    WINED3D_RS_ALPHAREF,
    WINED3D_RS_ALPHATESTENABLE,
    WINED3D_RS_ANTIALIASEDLINEENABLE,
    WINED3D_RS_BLENDOP,
    WINED3D_RS_BLENDOPALPHA,
    WINED3D_RS_BACK_STENCILFAIL,
    WINED3D_RS_BACK_STENCILPASS,
    WINED3D_RS_BACK_STENCILZFAIL,
    WINED3D_RS_COLORWRITEENABLE,
    WINED3D_RS_COLORWRITEENABLE1,
    WINED3D_RS_COLORWRITEENABLE2,
    WINED3D_RS_COLORWRITEENABLE3,
    WINED3D_RS_COLORWRITEENABLE4,
    WINED3D_RS_COLORWRITEENABLE5,
    WINED3D_RS_COLORWRITEENABLE6,
    WINED3D_RS_COLORWRITEENABLE7,
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

static inline void stateblock_set_bits(DWORD *map, UINT map_size)
{
    DWORD mask = (1u << (map_size & 0x1f)) - 1;
    memset(map, 0xff, (map_size >> 5) * sizeof(*map));
    if (mask) map[map_size >> 5] = mask;
}

/* Set all members of a stateblock savedstate to the given value */
static void stateblock_savedstates_set_all(struct wined3d_saved_states *states, DWORD vs_consts, DWORD ps_consts)
{
    unsigned int i;

    /* Single values */
    states->indices = 1;
    states->material = 1;
    states->viewport = 1;
    states->vertexDecl = 1;
    states->pixelShader = 1;
    states->vertexShader = 1;
    states->scissorRect = 1;
    states->blend_state = 1;

    /* Fixed size arrays */
    states->streamSource = 0xffff;
    states->streamFreq = 0xffff;
    states->textures = 0xfffff;
    stateblock_set_bits(states->transform, WINED3D_HIGHEST_TRANSFORM_STATE + 1);
    stateblock_set_bits(states->renderState, WINEHIGHEST_RENDER_STATE + 1);
    for (i = 0; i < WINED3D_MAX_TEXTURES; ++i) states->textureState[i] = 0x3ffff;
    for (i = 0; i < WINED3D_MAX_COMBINED_SAMPLERS; ++i) states->samplerState[i] = 0x3ffe;
    states->clipplane = (1u << WINED3D_MAX_CLIP_DISTANCES) - 1;
    states->pixelShaderConstantsB = 0xffff;
    states->pixelShaderConstantsI = 0xffff;
    states->vertexShaderConstantsB = 0xffff;
    states->vertexShaderConstantsI = 0xffff;

    /* Dynamically sized arrays */
    memset(states->ps_consts_f, TRUE, sizeof(BOOL) * ps_consts);
    memset(states->vs_consts_f, TRUE, sizeof(BOOL) * vs_consts);
}

static void stateblock_savedstates_set_pixel(struct wined3d_saved_states *states, const DWORD num_constants)
{
    DWORD texture_mask = 0;
    WORD sampler_mask = 0;
    unsigned int i;

    states->pixelShader = 1;
    states->blend_state = 1;

    for (i = 0; i < ARRAY_SIZE(pixel_states_render); ++i)
    {
        DWORD rs = pixel_states_render[i];
        states->renderState[rs >> 5] |= 1u << (rs & 0x1f);
    }

    for (i = 0; i < ARRAY_SIZE(pixel_states_texture); ++i)
        texture_mask |= 1u << pixel_states_texture[i];
    for (i = 0; i < WINED3D_MAX_TEXTURES; ++i) states->textureState[i] = texture_mask;
    for (i = 0; i < ARRAY_SIZE(pixel_states_sampler); ++i)
        sampler_mask |= 1u << pixel_states_sampler[i];
    for (i = 0; i < WINED3D_MAX_COMBINED_SAMPLERS; ++i) states->samplerState[i] = sampler_mask;
    states->pixelShaderConstantsB = 0xffff;
    states->pixelShaderConstantsI = 0xffff;

    memset(states->ps_consts_f, TRUE, sizeof(BOOL) * num_constants);
}

static void stateblock_savedstates_set_vertex(struct wined3d_saved_states *states, const DWORD num_constants)
{
    DWORD texture_mask = 0;
    WORD sampler_mask = 0;
    unsigned int i;

    states->vertexDecl = 1;
    states->vertexShader = 1;

    for (i = 0; i < ARRAY_SIZE(vertex_states_render); ++i)
    {
        DWORD rs = vertex_states_render[i];
        states->renderState[rs >> 5] |= 1u << (rs & 0x1f);
    }

    for (i = 0; i < ARRAY_SIZE(vertex_states_texture); ++i)
        texture_mask |= 1u << vertex_states_texture[i];
    for (i = 0; i < WINED3D_MAX_TEXTURES; ++i) states->textureState[i] = texture_mask;
    for (i = 0; i < ARRAY_SIZE(vertex_states_sampler); ++i)
        sampler_mask |= 1u << vertex_states_sampler[i];
    for (i = 0; i < WINED3D_MAX_COMBINED_SAMPLERS; ++i) states->samplerState[i] = sampler_mask;
    states->vertexShaderConstantsB = 0xffff;
    states->vertexShaderConstantsI = 0xffff;

    memset(states->vs_consts_f, TRUE, sizeof(BOOL) * num_constants);
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

    for (i = 0; i <= WINED3D_HIGHEST_TRANSFORM_STATE >> 5; ++i)
    {
        DWORD map = stateblock->changed.transform[i];
        for (j = 0; map; map >>= 1, ++j)
        {
            if (!(map & 1)) continue;

            stateblock->contained_transform_states[stateblock->num_contained_transform_states] = (i << 5) | j;
            ++stateblock->num_contained_transform_states;
        }
    }

    for (i = 0; i < d3d_info->limits.vs_uniform_count_swvp; ++i)
    {
        if (stateblock->changed.vs_consts_f[i])
        {
            stateblock->contained_vs_consts_f[stateblock->num_contained_vs_consts_f] = i;
            ++stateblock->num_contained_vs_consts_f;
        }
    }

    for (i = 0; i < WINED3D_MAX_CONSTS_I; ++i)
    {
        if (stateblock->changed.vertexShaderConstantsI & (1u << i))
        {
            stateblock->contained_vs_consts_i[stateblock->num_contained_vs_consts_i] = i;
            ++stateblock->num_contained_vs_consts_i;
        }
    }

    for (i = 0; i < WINED3D_MAX_CONSTS_B; ++i)
    {
        if (stateblock->changed.vertexShaderConstantsB & (1u << i))
        {
            stateblock->contained_vs_consts_b[stateblock->num_contained_vs_consts_b] = i;
            ++stateblock->num_contained_vs_consts_b;
        }
    }

    for (i = 0; i < d3d_info->limits.ps_uniform_count; ++i)
    {
        if (stateblock->changed.ps_consts_f[i])
        {
            stateblock->contained_ps_consts_f[stateblock->num_contained_ps_consts_f] = i;
            ++stateblock->num_contained_ps_consts_f;
        }
    }

    for (i = 0; i < WINED3D_MAX_CONSTS_I; ++i)
    {
        if (stateblock->changed.pixelShaderConstantsI & (1u << i))
        {
            stateblock->contained_ps_consts_i[stateblock->num_contained_ps_consts_i] = i;
            ++stateblock->num_contained_ps_consts_i;
        }
    }

    for (i = 0; i < WINED3D_MAX_CONSTS_B; ++i)
    {
        if (stateblock->changed.pixelShaderConstantsB & (1u << i))
        {
            stateblock->contained_ps_consts_b[stateblock->num_contained_ps_consts_b] = i;
            ++stateblock->num_contained_ps_consts_b;
        }
    }

    for (i = 0; i < WINED3D_MAX_TEXTURES; ++i)
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

    for (i = 0; i < WINED3D_MAX_COMBINED_SAMPLERS; ++i)
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

static void stateblock_init_lights(struct list *dst_map, struct list *src_map)
{
    unsigned int i;

    for (i = 0; i < LIGHTMAP_SIZE; ++i)
    {
        const struct wined3d_light_info *src_light;

        LIST_FOR_EACH_ENTRY(src_light, &src_map[i], struct wined3d_light_info, entry)
        {
            struct wined3d_light_info *dst_light = heap_alloc(sizeof(*dst_light));

            *dst_light = *src_light;
            list_add_tail(&dst_map[i], &dst_light->entry);
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
    struct wined3d_unordered_access_view *uav;
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

    for (i = 0; i < WINED3D_MAX_COMBINED_SAMPLERS; ++i)
    {
        if ((texture = state->textures[i]))
        {
            state->textures[i] = NULL;
            wined3d_texture_decref(texture);
        }
    }

    for (i = 0; i < WINED3D_MAX_STREAM_OUTPUT_BUFFERS; ++i)
    {
        if ((buffer = state->stream_output[i].buffer))
        {
            state->stream_output[i].buffer = NULL;
            wined3d_buffer_decref(buffer);
        }
    }

    for (i = 0; i < WINED3D_MAX_STREAMS; ++i)
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

    for (i = 0; i < WINED3D_PIPELINE_COUNT; ++i)
    {
        for (j = 0; j < MAX_UNORDERED_ACCESS_VIEWS; ++j)
        {
            if ((uav = state->unordered_access_view[i][j]))
            {
                state->unordered_access_view[i][j] = NULL;
                wined3d_unordered_access_view_decref(uav);
            }
        }
    }
}

void wined3d_stateblock_state_cleanup(struct wined3d_stateblock_state *state)
{
    struct wined3d_light_info *light, *cursor;
    struct wined3d_vertex_declaration *decl;
    struct wined3d_texture *texture;
    struct wined3d_buffer *buffer;
    struct wined3d_shader *shader;
    unsigned int i;

    if ((decl = state->vertex_declaration))
    {
        state->vertex_declaration = NULL;
        wined3d_vertex_declaration_decref(decl);
    }

    for (i = 0; i < WINED3D_MAX_STREAMS; ++i)
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

    if ((shader = state->vs))
    {
        state->vs = NULL;
        wined3d_shader_decref(shader);
    }

    if ((shader = state->ps))
    {
        state->ps = NULL;
        wined3d_shader_decref(shader);
    }

    for (i = 0; i < WINED3D_MAX_COMBINED_SAMPLERS; ++i)
    {
        if ((texture = state->textures[i]))
        {
            state->textures[i] = NULL;
            wined3d_texture_decref(texture);
        }
    }

    for (i = 0; i < LIGHTMAP_SIZE; ++i)
    {
        LIST_FOR_EACH_ENTRY_SAFE(light, cursor, &state->light_state.light_map[i], struct wined3d_light_info, entry)
        {
            list_remove(&light->entry);
            heap_free(light);
        }
    }
}

void state_cleanup(struct wined3d_state *state)
{
    unsigned int counter;

    if (!(state->flags & WINED3D_STATE_NO_REF))
        state_unbind_resources(state);

    for (counter = 0; counter < WINED3D_MAX_ACTIVE_LIGHTS; ++counter)
    {
        state->light_state.lights[counter] = NULL;
    }

    for (counter = 0; counter < LIGHTMAP_SIZE; ++counter)
    {
        struct list *e1, *e2;
        LIST_FOR_EACH_SAFE(e1, e2, &state->light_state.light_map[counter])
        {
            struct wined3d_light_info *light = LIST_ENTRY(e1, struct wined3d_light_info, entry);
            list_remove(&light->entry);
            heap_free(light);
        }
    }
}

ULONG CDECL wined3d_stateblock_decref(struct wined3d_stateblock *stateblock)
{
    ULONG refcount = InterlockedDecrement(&stateblock->ref);

    TRACE("%p decreasing refcount to %u\n", stateblock, refcount);

    if (!refcount)
    {
        wined3d_stateblock_state_cleanup(&stateblock->stateblock_state);
        heap_free(stateblock);
    }

    return refcount;
}

struct wined3d_light_info *wined3d_light_state_get_light(const struct wined3d_light_state *state, unsigned int idx)
{
    struct wined3d_light_info *light_info;
    unsigned int hash_idx;

    hash_idx = LIGHTMAP_HASHFUNC(idx);
    LIST_FOR_EACH_ENTRY(light_info, &state->light_map[hash_idx], struct wined3d_light_info, entry)
    {
        if (light_info->OriginalIndex == idx)
            return light_info;
    }

    return NULL;
}

HRESULT wined3d_light_state_set_light(struct wined3d_light_state *state, DWORD light_idx,
        const struct wined3d_light *params, struct wined3d_light_info **light_info)
{
    struct wined3d_light_info *object;
    unsigned int hash_idx;

    if (!(object = wined3d_light_state_get_light(state, light_idx)))
    {
        TRACE("Adding new light.\n");
        if (!(object = heap_alloc_zero(sizeof(*object))))
        {
            ERR("Failed to allocate light info.\n");
            return E_OUTOFMEMORY;
        }

        hash_idx = LIGHTMAP_HASHFUNC(light_idx);
        list_add_head(&state->light_map[hash_idx], &object->entry);
        object->glIndex = -1;
        object->OriginalIndex = light_idx;
    }

    object->OriginalParms = *params;

    *light_info = object;
    return WINED3D_OK;
}

void wined3d_light_state_enable_light(struct wined3d_light_state *state, const struct wined3d_d3d_info *d3d_info,
        struct wined3d_light_info *light_info, BOOL enable)
{
    unsigned int light_count, i;

    if (!(light_info->enabled = enable))
    {
        if (light_info->glIndex == -1)
        {
            TRACE("Light already disabled, nothing to do.\n");
            return;
        }

        state->lights[light_info->glIndex] = NULL;
        light_info->glIndex = -1;
        return;
    }

    if (light_info->glIndex != -1)
    {
        TRACE("Light already enabled, nothing to do.\n");
        return;
    }

    /* Find a free light. */
    light_count = d3d_info->limits.active_light_count;
    for (i = 0; i < light_count; ++i)
    {
        if (state->lights[i])
            continue;

        state->lights[i] = light_info;
        light_info->glIndex = i;
        return;
    }

    /* Our tests show that Windows returns D3D_OK in this situation, even with
     * D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE devices.
     * This is consistent among ddraw, d3d8 and d3d9. GetLightEnable returns
     * TRUE * as well for those lights.
     *
     * TODO: Test how this affects rendering. */
    WARN("Too many concurrently active lights.\n");
}

static void wined3d_state_record_lights(struct wined3d_light_state *dst_state,
        const struct wined3d_light_state *src_state)
{
    const struct wined3d_light_info *src;
    struct wined3d_light_info *dst;
    UINT i;

    /* Lights... For a recorded state block, we just had a chain of actions
     * to perform, so we need to walk that chain and update any actions which
     * differ. */
    for (i = 0; i < LIGHTMAP_SIZE; ++i)
    {
        LIST_FOR_EACH_ENTRY(dst, &dst_state->light_map[i], struct wined3d_light_info, entry)
        {
            if ((src = wined3d_light_state_get_light(src_state, dst->OriginalIndex)))
            {
                dst->OriginalParms = src->OriginalParms;

                if (src->glIndex == -1 && dst->glIndex != -1)
                {
                    /* Light disabled. */
                    dst_state->lights[dst->glIndex] = NULL;
                }
                else if (src->glIndex != -1 && dst->glIndex == -1)
                {
                    /* Light enabled. */
                    dst_state->lights[src->glIndex] = dst;
                }
                dst->glIndex = src->glIndex;
            }
            else
            {
                /* This can happen if the light was originally created as a
                 * default light for SetLightEnable() while recording. */
                WARN("Light %u in dst_state %p does not exist in src_state %p.\n",
                        dst->OriginalIndex, dst_state, src_state);

                dst->OriginalParms = WINED3D_default_light;
                if (dst->glIndex != -1)
                {
                    dst_state->lights[dst->glIndex] = NULL;
                    dst->glIndex = -1;
                }
            }
        }
    }
}

void CDECL wined3d_stateblock_capture(struct wined3d_stateblock *stateblock)
{
    const struct wined3d_stateblock_state *state = &stateblock->device->stateblock_state;
    unsigned int i;
    DWORD map;

    TRACE("stateblock %p.\n", stateblock);

    if (stateblock->changed.vertexShader && stateblock->stateblock_state.vs != state->vs)
    {
        TRACE("Updating vertex shader from %p to %p.\n", stateblock->stateblock_state.vs, state->vs);

        if (state->vs)
            wined3d_shader_incref(state->vs);
        if (stateblock->stateblock_state.vs)
            wined3d_shader_decref(stateblock->stateblock_state.vs);
        stateblock->stateblock_state.vs = state->vs;
    }

    /* Vertex shader float constants. */
    for (i = 0; i < stateblock->num_contained_vs_consts_f; ++i)
    {
        unsigned int idx = stateblock->contained_vs_consts_f[i];

        TRACE("Setting vs_consts_f[%u] to %s.\n", idx, debug_vec4(&state->vs_consts_f[idx]));

        stateblock->stateblock_state.vs_consts_f[idx] = state->vs_consts_f[idx];
    }

    /* Vertex shader integer constants. */
    for (i = 0; i < stateblock->num_contained_vs_consts_i; ++i)
    {
        unsigned int idx = stateblock->contained_vs_consts_i[i];

        TRACE("Setting vs_consts_i[%u] to %s.\n", idx, debug_ivec4(&state->vs_consts_i[idx]));

        stateblock->stateblock_state.vs_consts_i[idx] = state->vs_consts_i[idx];
    }

    /* Vertex shader boolean constants. */
    for (i = 0; i < stateblock->num_contained_vs_consts_b; ++i)
    {
        unsigned int idx = stateblock->contained_vs_consts_b[i];

        TRACE("Setting vs_consts_b[%u] to %s.\n",
                idx, state->vs_consts_b[idx] ? "TRUE" : "FALSE");

        stateblock->stateblock_state.vs_consts_b[idx] = state->vs_consts_b[idx];
    }

    /* Pixel shader float constants. */
    for (i = 0; i < stateblock->num_contained_ps_consts_f; ++i)
    {
        unsigned int idx = stateblock->contained_ps_consts_f[i];

        TRACE("Setting ps_consts_f[%u] to %s.\n", idx, debug_vec4(&state->ps_consts_f[idx]));

        stateblock->stateblock_state.ps_consts_f[idx] = state->ps_consts_f[idx];
    }

    /* Pixel shader integer constants. */
    for (i = 0; i < stateblock->num_contained_ps_consts_i; ++i)
    {
        unsigned int idx = stateblock->contained_ps_consts_i[i];

        TRACE("Setting ps_consts_i[%u] to %s.\n", idx, debug_ivec4(&state->ps_consts_i[idx]));

        stateblock->stateblock_state.ps_consts_i[idx] = state->ps_consts_i[idx];
    }

    /* Pixel shader boolean constants. */
    for (i = 0; i < stateblock->num_contained_ps_consts_b; ++i)
    {
        unsigned int idx = stateblock->contained_ps_consts_b[i];

        TRACE("Setting ps_consts_b[%u] to %s.\n",
                idx, state->ps_consts_b[idx] ? "TRUE" : "FALSE");

        stateblock->stateblock_state.ps_consts_b[idx] = state->ps_consts_b[idx];
    }

    /* Others + Render & Texture */
    for (i = 0; i < stateblock->num_contained_transform_states; ++i)
    {
        enum wined3d_transform_state transform = stateblock->contained_transform_states[i];

        TRACE("Updating transform %#x.\n", transform);

        stateblock->stateblock_state.transforms[transform] = state->transforms[transform];
    }

    if (stateblock->changed.indices
            && ((stateblock->stateblock_state.index_buffer != state->index_buffer)
                || (stateblock->stateblock_state.base_vertex_index != state->base_vertex_index)
                || (stateblock->stateblock_state.index_format != state->index_format)))
    {
        TRACE("Updating index buffer to %p, base vertex index to %d.\n",
                state->index_buffer, state->base_vertex_index);

        if (state->index_buffer)
            wined3d_buffer_incref(state->index_buffer);
        if (stateblock->stateblock_state.index_buffer)
            wined3d_buffer_decref(stateblock->stateblock_state.index_buffer);
        stateblock->stateblock_state.index_buffer = state->index_buffer;
        stateblock->stateblock_state.base_vertex_index = state->base_vertex_index;
        stateblock->stateblock_state.index_format = state->index_format;
    }

    if (stateblock->changed.vertexDecl && stateblock->stateblock_state.vertex_declaration != state->vertex_declaration)
    {
        TRACE("Updating vertex declaration from %p to %p.\n",
                stateblock->stateblock_state.vertex_declaration, state->vertex_declaration);

        if (state->vertex_declaration)
                wined3d_vertex_declaration_incref(state->vertex_declaration);
        if (stateblock->stateblock_state.vertex_declaration)
                wined3d_vertex_declaration_decref(stateblock->stateblock_state.vertex_declaration);
        stateblock->stateblock_state.vertex_declaration = state->vertex_declaration;
    }

    if (stateblock->changed.material
            && memcmp(&state->material, &stateblock->stateblock_state.material,
            sizeof(stateblock->stateblock_state.material)))
    {
        TRACE("Updating material.\n");

        stateblock->stateblock_state.material = state->material;
    }

    if (stateblock->changed.viewport
            && memcmp(&state->viewport, &stateblock->stateblock_state.viewport, sizeof(state->viewport)))
    {
        TRACE("Updating viewport.\n");

        stateblock->stateblock_state.viewport = state->viewport;
    }

    if (stateblock->changed.scissorRect
            && memcmp(&state->scissor_rect, &stateblock->stateblock_state.scissor_rect, sizeof(state->scissor_rect)))
    {
        TRACE("Updating scissor rect.\n");

        stateblock->stateblock_state.scissor_rect = state->scissor_rect;
    }

    if (stateblock->changed.blend_state
            && memcmp(&state->blend_factor, &stateblock->stateblock_state.blend_factor,
                    sizeof(stateblock->stateblock_state.blend_factor)))
    {
        TRACE("Updating blend factor.\n");

        stateblock->stateblock_state.blend_factor = state->blend_factor;
    }

    map = stateblock->changed.streamSource;
    for (i = 0; map; map >>= 1, ++i)
    {
        if (!(map & 1)) continue;

        if (stateblock->stateblock_state.streams[i].stride != state->streams[i].stride
                || stateblock->stateblock_state.streams[i].offset != state->streams[i].offset
                || stateblock->stateblock_state.streams[i].buffer != state->streams[i].buffer)
        {
            TRACE("stateblock %p, stream source %u, buffer %p, stride %u, offset %u.\n",
                    stateblock, i, state->streams[i].buffer, state->streams[i].stride,
                    state->streams[i].offset);

            stateblock->stateblock_state.streams[i].stride = state->streams[i].stride;
            if (stateblock->changed.store_stream_offset)
                stateblock->stateblock_state.streams[i].offset = state->streams[i].offset;

            if (state->streams[i].buffer)
                    wined3d_buffer_incref(state->streams[i].buffer);
            if (stateblock->stateblock_state.streams[i].buffer)
                    wined3d_buffer_decref(stateblock->stateblock_state.streams[i].buffer);
            stateblock->stateblock_state.streams[i].buffer = state->streams[i].buffer;
        }
    }

    map = stateblock->changed.streamFreq;
    for (i = 0; map; map >>= 1, ++i)
    {
        if (!(map & 1)) continue;

        if (stateblock->stateblock_state.streams[i].frequency != state->streams[i].frequency
                || stateblock->stateblock_state.streams[i].flags != state->streams[i].flags)
        {
            TRACE("Updating stream frequency %u to %u flags to %#x.\n",
                    i, state->streams[i].frequency, state->streams[i].flags);

            stateblock->stateblock_state.streams[i].frequency = state->streams[i].frequency;
            stateblock->stateblock_state.streams[i].flags = state->streams[i].flags;
        }
    }

    map = stateblock->changed.clipplane;
    for (i = 0; map; map >>= 1, ++i)
    {
        if (!(map & 1)) continue;

        if (memcmp(&stateblock->stateblock_state.clip_planes[i], &state->clip_planes[i], sizeof(state->clip_planes[i])))
        {
            TRACE("Updating clipplane %u.\n", i);
            stateblock->stateblock_state.clip_planes[i] = state->clip_planes[i];
        }
    }

    /* Render */
    for (i = 0; i < stateblock->num_contained_render_states; ++i)
    {
        enum wined3d_render_state rs = stateblock->contained_render_states[i];

        TRACE("Updating render state %#x to %u.\n", rs, state->rs[rs]);

        stateblock->stateblock_state.rs[rs] = state->rs[rs];
    }

    /* Texture states */
    for (i = 0; i < stateblock->num_contained_tss_states; ++i)
    {
        DWORD stage = stateblock->contained_tss_states[i].stage;
        DWORD texture_state = stateblock->contained_tss_states[i].state;

        TRACE("Updating texturestage state %u, %u to %#x (was %#x).\n", stage, texture_state,
                state->texture_states[stage][texture_state],
                stateblock->stateblock_state.texture_states[stage][texture_state]);

        stateblock->stateblock_state.texture_states[stage][texture_state] = state->texture_states[stage][texture_state];
    }

    /* Samplers */
    map = stateblock->changed.textures;
    for (i = 0; map; map >>= 1, ++i)
    {
        if (!(map & 1)) continue;

        TRACE("Updating texture %u to %p (was %p).\n",
                i, state->textures[i], stateblock->stateblock_state.textures[i]);

        if (state->textures[i])
            wined3d_texture_incref(state->textures[i]);
        if (stateblock->stateblock_state.textures[i])
            wined3d_texture_decref(stateblock->stateblock_state.textures[i]);
        stateblock->stateblock_state.textures[i] = state->textures[i];
    }

    for (i = 0; i < stateblock->num_contained_sampler_states; ++i)
    {
        DWORD stage = stateblock->contained_sampler_states[i].stage;
        DWORD sampler_state = stateblock->contained_sampler_states[i].state;

        TRACE("Updating sampler state %u, %u to %#x (was %#x).\n", stage, sampler_state,
                state->sampler_states[stage][sampler_state],
                stateblock->stateblock_state.sampler_states[stage][sampler_state]);

        stateblock->stateblock_state.sampler_states[stage][sampler_state] = state->sampler_states[stage][sampler_state];
    }

    if (stateblock->changed.pixelShader && stateblock->stateblock_state.ps != state->ps)
    {
        if (state->ps)
            wined3d_shader_incref(state->ps);
        if (stateblock->stateblock_state.ps)
            wined3d_shader_decref(stateblock->stateblock_state.ps);
        stateblock->stateblock_state.ps = state->ps;
    }

    wined3d_state_record_lights(&stateblock->stateblock_state.light_state, &state->light_state);

    TRACE("Capture done.\n");
}

void CDECL wined3d_stateblock_apply(const struct wined3d_stateblock *stateblock)
{
    struct wined3d_stateblock_state *state = &stateblock->device->stateblock_state;
    struct wined3d_device *device = stateblock->device;
    unsigned int i;
    DWORD map;

    TRACE("Applying stateblock %p to device %p.\n", stateblock, device);

    if (stateblock->changed.vertexShader)
    {
        if (stateblock->stateblock_state.vs)
            wined3d_shader_incref(stateblock->stateblock_state.vs);
        if (state->vs)
            wined3d_shader_decref(state->vs);
        state->vs = stateblock->stateblock_state.vs;
        wined3d_device_set_vertex_shader(device, stateblock->stateblock_state.vs);
    }

    /* Vertex Shader Constants. */
    for (i = 0; i < stateblock->num_contained_vs_consts_f; ++i)
    {
        unsigned int idx = stateblock->contained_vs_consts_f[i];

        state->vs_consts_f[idx] = stateblock->stateblock_state.vs_consts_f[idx];
        wined3d_device_set_vs_consts_f(device, idx, 1, &stateblock->stateblock_state.vs_consts_f[idx]);
    }
    for (i = 0; i < stateblock->num_contained_vs_consts_i; ++i)
    {
        unsigned int idx = stateblock->contained_vs_consts_i[i];

        state->vs_consts_i[idx] = stateblock->stateblock_state.vs_consts_i[idx];
        wined3d_device_set_vs_consts_i(device, idx, 1, &stateblock->stateblock_state.vs_consts_i[idx]);
    }
    for (i = 0; i < stateblock->num_contained_vs_consts_b; ++i)
    {
        unsigned int idx = stateblock->contained_vs_consts_b[i];

        state->vs_consts_b[idx] = stateblock->stateblock_state.vs_consts_b[idx];
        wined3d_device_set_vs_consts_b(device, idx, 1, &stateblock->stateblock_state.vs_consts_b[idx]);
    }

    for (i = 0; i < ARRAY_SIZE(stateblock->stateblock_state.light_state.light_map); ++i)
    {
        const struct wined3d_light_info *light;
        struct wined3d_light_info *new_light;

        LIST_FOR_EACH_ENTRY(light, &stateblock->stateblock_state.light_state.light_map[i], struct wined3d_light_info, entry)
        {
            if (SUCCEEDED(wined3d_light_state_set_light(&state->light_state, light->OriginalIndex,
                    &light->OriginalParms, &new_light)))
            {
                wined3d_light_state_enable_light(&state->light_state, &device->adapter->d3d_info, new_light, light->glIndex != -1);
            }
            wined3d_device_set_light(device, light->OriginalIndex, &light->OriginalParms);
            wined3d_device_set_light_enable(device, light->OriginalIndex, light->glIndex != -1);
        }
    }

    if (stateblock->changed.pixelShader)
    {
        if (stateblock->stateblock_state.ps)
            wined3d_shader_incref(stateblock->stateblock_state.ps);
        if (state->ps)
            wined3d_shader_decref(state->ps);
        state->ps = stateblock->stateblock_state.ps;
        wined3d_device_set_pixel_shader(device, stateblock->stateblock_state.ps);
    }

    /* Pixel Shader Constants. */
    for (i = 0; i < stateblock->num_contained_ps_consts_f; ++i)
    {
        unsigned int idx = stateblock->contained_ps_consts_f[i];

        state->ps_consts_f[idx] = stateblock->stateblock_state.ps_consts_f[idx];
        wined3d_device_set_ps_consts_f(device, idx, 1, &stateblock->stateblock_state.ps_consts_f[idx]);
    }
    for (i = 0; i < stateblock->num_contained_ps_consts_i; ++i)
    {
        unsigned int idx = stateblock->contained_ps_consts_i[i];

        state->ps_consts_i[idx] = stateblock->stateblock_state.ps_consts_i[idx];
        wined3d_device_set_ps_consts_i(device, idx, 1, &stateblock->stateblock_state.ps_consts_i[idx]);
    }
    for (i = 0; i < stateblock->num_contained_ps_consts_b; ++i)
    {
        unsigned int idx = stateblock->contained_ps_consts_b[i];

        state->ps_consts_b[idx] = stateblock->stateblock_state.ps_consts_b[idx];
        wined3d_device_set_ps_consts_b(device, idx, 1, &stateblock->stateblock_state.ps_consts_b[idx]);
    }

    /* Render states. */
    for (i = 0; i < stateblock->num_contained_render_states; ++i)
    {
        enum wined3d_render_state rs = stateblock->contained_render_states[i];

        state->rs[rs] = stateblock->stateblock_state.rs[rs];
        wined3d_device_set_render_state(device, rs, stateblock->stateblock_state.rs[rs]);
    }

    /* Texture states. */
    for (i = 0; i < stateblock->num_contained_tss_states; ++i)
    {
        DWORD stage = stateblock->contained_tss_states[i].stage;
        DWORD texture_state = stateblock->contained_tss_states[i].state;

        state->texture_states[stage][texture_state] = stateblock->stateblock_state.texture_states[stage][texture_state];
        wined3d_device_set_texture_stage_state(device, stage, texture_state,
                stateblock->stateblock_state.texture_states[stage][texture_state]);
    }

    /* Sampler states. */
    for (i = 0; i < stateblock->num_contained_sampler_states; ++i)
    {
        DWORD stage = stateblock->contained_sampler_states[i].stage;
        DWORD sampler_state = stateblock->contained_sampler_states[i].state;
        DWORD value = stateblock->stateblock_state.sampler_states[stage][sampler_state];

        state->sampler_states[stage][sampler_state] = value;
        if (stage >= WINED3D_MAX_FRAGMENT_SAMPLERS) stage += WINED3DVERTEXTEXTURESAMPLER0 - WINED3D_MAX_FRAGMENT_SAMPLERS;
        wined3d_device_set_sampler_state(device, stage, sampler_state, value);
    }

    /* Transform states. */
    for (i = 0; i < stateblock->num_contained_transform_states; ++i)
    {
        enum wined3d_transform_state transform = stateblock->contained_transform_states[i];

        state->transforms[transform] = stateblock->stateblock_state.transforms[transform];
        wined3d_device_set_transform(device, transform, &stateblock->stateblock_state.transforms[transform]);
    }

    if (stateblock->changed.indices)
    {
        if (stateblock->stateblock_state.index_buffer)
            wined3d_buffer_incref(stateblock->stateblock_state.index_buffer);
        if (state->index_buffer)
            wined3d_buffer_decref(state->index_buffer);
        state->index_buffer = stateblock->stateblock_state.index_buffer;
        state->index_format = stateblock->stateblock_state.index_format;
        state->base_vertex_index = stateblock->stateblock_state.base_vertex_index;

        wined3d_device_set_index_buffer(device, stateblock->stateblock_state.index_buffer,
                stateblock->stateblock_state.index_format, 0);
        wined3d_device_set_base_vertex_index(device, stateblock->stateblock_state.base_vertex_index);
    }

    if (stateblock->changed.vertexDecl && stateblock->stateblock_state.vertex_declaration)
    {
        if (stateblock->stateblock_state.vertex_declaration)
            wined3d_vertex_declaration_incref(stateblock->stateblock_state.vertex_declaration);
        if (state->vertex_declaration)
            wined3d_vertex_declaration_decref(state->vertex_declaration);
        state->vertex_declaration = stateblock->stateblock_state.vertex_declaration;
        wined3d_device_set_vertex_declaration(device, stateblock->stateblock_state.vertex_declaration);
    }

    if (stateblock->changed.material)
    {
        state->material = stateblock->stateblock_state.material;
        wined3d_device_set_material(device, &stateblock->stateblock_state.material);
    }

    if (stateblock->changed.viewport)
    {
        state->viewport = stateblock->stateblock_state.viewport;

        wined3d_device_set_viewports(device, 1, &stateblock->stateblock_state.viewport);
    }

    if (stateblock->changed.scissorRect)
    {
        state->scissor_rect = stateblock->stateblock_state.scissor_rect;

        wined3d_device_set_scissor_rects(device, 1, &stateblock->stateblock_state.scissor_rect);
    }

    if (stateblock->changed.blend_state)
    {
        state->blend_factor = stateblock->stateblock_state.blend_factor;
        wined3d_device_set_blend_state(device, NULL, &stateblock->stateblock_state.blend_factor);
    }

    map = stateblock->changed.streamSource;
    for (i = 0; map; map >>= 1, ++i)
    {
        unsigned int offset, stride;

        if (!(map & 1)) continue;

        if (stateblock->stateblock_state.streams[i].buffer)
                wined3d_buffer_incref(stateblock->stateblock_state.streams[i].buffer);
        if (state->streams[i].buffer)
                wined3d_buffer_decref(state->streams[i].buffer);
        state->streams[i].buffer = stateblock->stateblock_state.streams[i].buffer;

        offset = stateblock->stateblock_state.streams[i].offset;
        stride = stateblock->stateblock_state.streams[i].stride;

        state->streams[i].stride = stride;
        state->streams[i].offset = offset;
        wined3d_device_set_stream_source(device, i,
                stateblock->stateblock_state.streams[i].buffer, offset, stride);
    }

    map = stateblock->changed.streamFreq;
    for (i = 0; map; map >>= 1, ++i)
    {
        if (!(map & 1)) continue;

        state->streams[i].frequency = stateblock->stateblock_state.streams[i].frequency;
        state->streams[i].flags = stateblock->stateblock_state.streams[i].flags;

        wined3d_device_set_stream_source_freq(device, i,
                stateblock->stateblock_state.streams[i].frequency | stateblock->stateblock_state.streams[i].flags);
    }

    map = stateblock->changed.textures;
    for (i = 0; map; map >>= 1, ++i)
    {
        DWORD stage;

        if (!(map & 1)) continue;

        stage = i < WINED3D_MAX_FRAGMENT_SAMPLERS ? i : WINED3DVERTEXTEXTURESAMPLER0 + i - WINED3D_MAX_FRAGMENT_SAMPLERS;
        if (stateblock->stateblock_state.textures[i])
            wined3d_texture_incref(stateblock->stateblock_state.textures[i]);
        if (state->textures[i])
            wined3d_texture_decref(state->textures[i]);
        state->textures[i] = stateblock->stateblock_state.textures[i];
        wined3d_device_set_texture(device, stage, stateblock->stateblock_state.textures[i]);
    }

    map = stateblock->changed.clipplane;
    for (i = 0; map; map >>= 1, ++i)
    {
        if (!(map & 1)) continue;

        state->clip_planes[i] = stateblock->stateblock_state.clip_planes[i];
        wined3d_device_set_clip_plane(device, i, &stateblock->stateblock_state.clip_planes[i]);
    }

    TRACE("Applied stateblock %p.\n", stateblock);
}

void CDECL wined3d_stateblock_set_vertex_shader(struct wined3d_stateblock *stateblock, struct wined3d_shader *shader)
{
    TRACE("stateblock %p, shader %p.\n", stateblock, shader);

    if (shader)
        wined3d_shader_incref(shader);
    if (stateblock->stateblock_state.vs)
        wined3d_shader_decref(stateblock->stateblock_state.vs);
    stateblock->stateblock_state.vs = shader;
    stateblock->changed.vertexShader = TRUE;
}

HRESULT CDECL wined3d_stateblock_set_vs_consts_f(struct wined3d_stateblock *stateblock,
        unsigned int start_idx, unsigned int count, const struct wined3d_vec4 *constants)
{
    const struct wined3d_d3d_info *d3d_info = &stateblock->device->adapter->d3d_info;
    unsigned int constants_count;

    TRACE("stateblock %p, start_idx %u, count %u, constants %p.\n",
            stateblock, start_idx, count, constants);

    constants_count = stateblock->device->create_parms.flags
            & (WINED3DCREATE_SOFTWARE_VERTEXPROCESSING | WINED3DCREATE_MIXED_VERTEXPROCESSING)
            ? d3d_info->limits.vs_uniform_count_swvp : d3d_info->limits.vs_uniform_count;

    if (!constants || start_idx >= constants_count || count > constants_count - start_idx)
        return WINED3DERR_INVALIDCALL;

    memcpy(&stateblock->stateblock_state.vs_consts_f[start_idx], constants, count * sizeof(*constants));
    memset(&stateblock->changed.vs_consts_f[start_idx], 1, count * sizeof(*stateblock->changed.vs_consts_f));
    return WINED3D_OK;
}

HRESULT CDECL wined3d_stateblock_set_vs_consts_i(struct wined3d_stateblock *stateblock,
        unsigned int start_idx, unsigned int count, const struct wined3d_ivec4 *constants)
{
    unsigned int i;

    TRACE("stateblock %p, start_idx %u, count %u, constants %p.\n",
            stateblock, start_idx, count, constants);

    if (!constants || start_idx >= WINED3D_MAX_CONSTS_I)
        return WINED3DERR_INVALIDCALL;

    if (count > WINED3D_MAX_CONSTS_I - start_idx)
        count = WINED3D_MAX_CONSTS_I - start_idx;

    memcpy(&stateblock->stateblock_state.vs_consts_i[start_idx], constants, count * sizeof(*constants));
    for (i = start_idx; i < count + start_idx; ++i)
        stateblock->changed.vertexShaderConstantsI |= (1u << i);
    return WINED3D_OK;
}

HRESULT CDECL wined3d_stateblock_set_vs_consts_b(struct wined3d_stateblock *stateblock,
        unsigned int start_idx, unsigned int count, const BOOL *constants)
{
    unsigned int i;

    TRACE("stateblock %p, start_idx %u, count %u, constants %p.\n",
            stateblock, start_idx, count, constants);

    if (!constants || start_idx >= WINED3D_MAX_CONSTS_B)
        return WINED3DERR_INVALIDCALL;

    if (count > WINED3D_MAX_CONSTS_B - start_idx)
        count = WINED3D_MAX_CONSTS_B - start_idx;

    memcpy(&stateblock->stateblock_state.vs_consts_b[start_idx], constants, count * sizeof(*constants));
    for (i = start_idx; i < count + start_idx; ++i)
        stateblock->changed.vertexShaderConstantsB |= (1u << i);
    return WINED3D_OK;
}

void CDECL wined3d_stateblock_set_pixel_shader(struct wined3d_stateblock *stateblock, struct wined3d_shader *shader)
{
    TRACE("stateblock %p, shader %p.\n", stateblock, shader);

    if (shader)
        wined3d_shader_incref(shader);
    if (stateblock->stateblock_state.ps)
        wined3d_shader_decref(stateblock->stateblock_state.ps);
    stateblock->stateblock_state.ps = shader;
    stateblock->changed.pixelShader = TRUE;
}

HRESULT CDECL wined3d_stateblock_set_ps_consts_f(struct wined3d_stateblock *stateblock,
        unsigned int start_idx, unsigned int count, const struct wined3d_vec4 *constants)
{
    TRACE("stateblock %p, start_idx %u, count %u, constants %p.\n",
            stateblock, start_idx, count, constants);

    if (!constants || start_idx >= WINED3D_MAX_PS_CONSTS_F || count > WINED3D_MAX_PS_CONSTS_F - start_idx)
        return WINED3DERR_INVALIDCALL;

    memcpy(&stateblock->stateblock_state.ps_consts_f[start_idx], constants, count * sizeof(*constants));
    memset(&stateblock->changed.ps_consts_f[start_idx], 1, count * sizeof(*stateblock->changed.ps_consts_f));
    return WINED3D_OK;
}

HRESULT CDECL wined3d_stateblock_set_ps_consts_i(struct wined3d_stateblock *stateblock,
        unsigned int start_idx, unsigned int count, const struct wined3d_ivec4 *constants)
{
    unsigned int i;

    TRACE("stateblock %p, start_idx %u, count %u, constants %p.\n",
            stateblock, start_idx, count, constants);

    if (!constants || start_idx >= WINED3D_MAX_CONSTS_I)
        return WINED3DERR_INVALIDCALL;

    if (count > WINED3D_MAX_CONSTS_I - start_idx)
        count = WINED3D_MAX_CONSTS_I - start_idx;

    memcpy(&stateblock->stateblock_state.ps_consts_i[start_idx], constants, count * sizeof(*constants));
    for (i = start_idx; i < count + start_idx; ++i)
        stateblock->changed.pixelShaderConstantsI |= (1u << i);
    return WINED3D_OK;
}

HRESULT CDECL wined3d_stateblock_set_ps_consts_b(struct wined3d_stateblock *stateblock,
        unsigned int start_idx, unsigned int count, const BOOL *constants)
{
    unsigned int i;

    TRACE("stateblock %p, start_idx %u, count %u, constants %p.\n",
            stateblock, start_idx, count, constants);

    if (!constants || start_idx >= WINED3D_MAX_CONSTS_B)
        return WINED3DERR_INVALIDCALL;

    if (count > WINED3D_MAX_CONSTS_B - start_idx)
        count = WINED3D_MAX_CONSTS_B - start_idx;

    memcpy(&stateblock->stateblock_state.ps_consts_b[start_idx], constants, count * sizeof(*constants));
    for (i = start_idx; i < count + start_idx; ++i)
        stateblock->changed.pixelShaderConstantsB |= (1u << i);
    return WINED3D_OK;
}

void CDECL wined3d_stateblock_set_vertex_declaration(struct wined3d_stateblock *stateblock,
        struct wined3d_vertex_declaration *declaration)
{
    TRACE("stateblock %p, declaration %p.\n", stateblock, declaration);

    if (declaration)
        wined3d_vertex_declaration_incref(declaration);
    if (stateblock->stateblock_state.vertex_declaration)
        wined3d_vertex_declaration_decref(stateblock->stateblock_state.vertex_declaration);
    stateblock->stateblock_state.vertex_declaration = declaration;
    stateblock->changed.vertexDecl = TRUE;
}

void CDECL wined3d_stateblock_set_render_state(struct wined3d_stateblock *stateblock,
        enum wined3d_render_state state, DWORD value)
{
    TRACE("stateblock %p, state %s (%#x), value %#x.\n", stateblock, debug_d3drenderstate(state), state, value);

    if (state > WINEHIGHEST_RENDER_STATE)
    {
        WARN("Unhandled render state %#x.\n", state);
        return;
    }

    stateblock->stateblock_state.rs[state] = value;
    stateblock->changed.renderState[state >> 5] |= 1u << (state & 0x1f);
}

void CDECL wined3d_stateblock_set_blend_factor(struct wined3d_stateblock *stateblock,
        const struct wined3d_color *blend_factor)
{
    TRACE("stateblock %p, blend_factor %p.\n", stateblock, blend_factor);

    stateblock->stateblock_state.blend_factor = *blend_factor;
    stateblock->changed.blend_state = TRUE;
}

static void init_default_render_states(DWORD rs[WINEHIGHEST_RENDER_STATE + 1], const struct wined3d_d3d_info *d3d_info)
{
    unsigned int i;
    union
    {
        struct wined3d_line_pattern lp;
        DWORD d;
    } lp;
    union
    {
        float f;
        DWORD d;
    } tmpfloat;

    rs[WINED3D_RS_ZENABLE] = WINED3D_ZB_TRUE;
    rs[WINED3D_RS_FILLMODE] = WINED3D_FILL_SOLID;
    rs[WINED3D_RS_SHADEMODE] = WINED3D_SHADE_GOURAUD;
    lp.lp.repeat_factor = 0;
    lp.lp.line_pattern = 0;
    rs[WINED3D_RS_LINEPATTERN] = lp.d;
    rs[WINED3D_RS_ZWRITEENABLE] = TRUE;
    rs[WINED3D_RS_ALPHATESTENABLE] = FALSE;
    rs[WINED3D_RS_LASTPIXEL] = TRUE;
    rs[WINED3D_RS_SRCBLEND] = WINED3D_BLEND_ONE;
    rs[WINED3D_RS_DESTBLEND] = WINED3D_BLEND_ZERO;
    rs[WINED3D_RS_CULLMODE] = WINED3D_CULL_BACK;
    rs[WINED3D_RS_ZFUNC] = WINED3D_CMP_LESSEQUAL;
    rs[WINED3D_RS_ALPHAFUNC] = WINED3D_CMP_ALWAYS;
    rs[WINED3D_RS_ALPHAREF] = 0;
    rs[WINED3D_RS_DITHERENABLE] = FALSE;
    rs[WINED3D_RS_ALPHABLENDENABLE] = FALSE;
    rs[WINED3D_RS_FOGENABLE] = FALSE;
    rs[WINED3D_RS_SPECULARENABLE] = FALSE;
    rs[WINED3D_RS_ZVISIBLE] = 0;
    rs[WINED3D_RS_FOGCOLOR] = 0;
    rs[WINED3D_RS_FOGTABLEMODE] = WINED3D_FOG_NONE;
    tmpfloat.f = 0.0f;
    rs[WINED3D_RS_FOGSTART] = tmpfloat.d;
    tmpfloat.f = 1.0f;
    rs[WINED3D_RS_FOGEND] = tmpfloat.d;
    tmpfloat.f = 1.0f;
    rs[WINED3D_RS_FOGDENSITY] = tmpfloat.d;
    rs[WINED3D_RS_EDGEANTIALIAS] = FALSE;
    rs[WINED3D_RS_RANGEFOGENABLE] = FALSE;
    rs[WINED3D_RS_STENCILENABLE] = FALSE;
    rs[WINED3D_RS_STENCILFAIL] = WINED3D_STENCIL_OP_KEEP;
    rs[WINED3D_RS_STENCILZFAIL] = WINED3D_STENCIL_OP_KEEP;
    rs[WINED3D_RS_STENCILPASS] = WINED3D_STENCIL_OP_KEEP;
    rs[WINED3D_RS_STENCILREF] = 0;
    rs[WINED3D_RS_STENCILMASK] = 0xffffffff;
    rs[WINED3D_RS_STENCILFUNC] = WINED3D_CMP_ALWAYS;
    rs[WINED3D_RS_STENCILWRITEMASK] = 0xffffffff;
    rs[WINED3D_RS_TEXTUREFACTOR] = 0xffffffff;
    rs[WINED3D_RS_WRAP0] = 0;
    rs[WINED3D_RS_WRAP1] = 0;
    rs[WINED3D_RS_WRAP2] = 0;
    rs[WINED3D_RS_WRAP3] = 0;
    rs[WINED3D_RS_WRAP4] = 0;
    rs[WINED3D_RS_WRAP5] = 0;
    rs[WINED3D_RS_WRAP6] = 0;
    rs[WINED3D_RS_WRAP7] = 0;
    rs[WINED3D_RS_CLIPPING] = TRUE;
    rs[WINED3D_RS_LIGHTING] = TRUE;
    rs[WINED3D_RS_AMBIENT] = 0;
    rs[WINED3D_RS_FOGVERTEXMODE] = WINED3D_FOG_NONE;
    rs[WINED3D_RS_COLORVERTEX] = TRUE;
    rs[WINED3D_RS_LOCALVIEWER] = TRUE;
    rs[WINED3D_RS_NORMALIZENORMALS] = FALSE;
    rs[WINED3D_RS_DIFFUSEMATERIALSOURCE] = WINED3D_MCS_COLOR1;
    rs[WINED3D_RS_SPECULARMATERIALSOURCE] = WINED3D_MCS_COLOR2;
    rs[WINED3D_RS_AMBIENTMATERIALSOURCE] = WINED3D_MCS_MATERIAL;
    rs[WINED3D_RS_EMISSIVEMATERIALSOURCE] = WINED3D_MCS_MATERIAL;
    rs[WINED3D_RS_VERTEXBLEND] = WINED3D_VBF_DISABLE;
    rs[WINED3D_RS_CLIPPLANEENABLE] = 0;
    rs[WINED3D_RS_SOFTWAREVERTEXPROCESSING] = FALSE;
    tmpfloat.f = 1.0f;
    rs[WINED3D_RS_POINTSIZE] = tmpfloat.d;
    tmpfloat.f = 1.0f;
    rs[WINED3D_RS_POINTSIZE_MIN] = tmpfloat.d;
    rs[WINED3D_RS_POINTSPRITEENABLE] = FALSE;
    rs[WINED3D_RS_POINTSCALEENABLE] = FALSE;
    tmpfloat.f = 1.0f;
    rs[WINED3D_RS_POINTSCALE_A] = tmpfloat.d;
    tmpfloat.f = 0.0f;
    rs[WINED3D_RS_POINTSCALE_B] = tmpfloat.d;
    tmpfloat.f = 0.0f;
    rs[WINED3D_RS_POINTSCALE_C] = tmpfloat.d;
    rs[WINED3D_RS_MULTISAMPLEANTIALIAS] = TRUE;
    rs[WINED3D_RS_MULTISAMPLEMASK] = 0xffffffff;
    rs[WINED3D_RS_PATCHEDGESTYLE] = WINED3D_PATCH_EDGE_DISCRETE;
    tmpfloat.f = 1.0f;
    rs[WINED3D_RS_PATCHSEGMENTS] = tmpfloat.d;
    rs[WINED3D_RS_DEBUGMONITORTOKEN] = 0xbaadcafe;
    tmpfloat.f = d3d_info->limits.pointsize_max;
    rs[WINED3D_RS_POINTSIZE_MAX] = tmpfloat.d;
    rs[WINED3D_RS_INDEXEDVERTEXBLENDENABLE] = FALSE;
    tmpfloat.f = 0.0f;
    rs[WINED3D_RS_TWEENFACTOR] = tmpfloat.d;
    rs[WINED3D_RS_BLENDOP] = WINED3D_BLEND_OP_ADD;
    rs[WINED3D_RS_POSITIONDEGREE] = WINED3D_DEGREE_CUBIC;
    rs[WINED3D_RS_NORMALDEGREE] = WINED3D_DEGREE_LINEAR;
    /* states new in d3d9 */
    rs[WINED3D_RS_SCISSORTESTENABLE] = FALSE;
    rs[WINED3D_RS_SLOPESCALEDEPTHBIAS] = 0;
    tmpfloat.f = 1.0f;
    rs[WINED3D_RS_MINTESSELLATIONLEVEL] = tmpfloat.d;
    rs[WINED3D_RS_MAXTESSELLATIONLEVEL] = tmpfloat.d;
    rs[WINED3D_RS_ANTIALIASEDLINEENABLE] = FALSE;
    tmpfloat.f = 0.0f;
    rs[WINED3D_RS_ADAPTIVETESS_X] = tmpfloat.d;
    rs[WINED3D_RS_ADAPTIVETESS_Y] = tmpfloat.d;
    tmpfloat.f = 1.0f;
    rs[WINED3D_RS_ADAPTIVETESS_Z] = tmpfloat.d;
    tmpfloat.f = 0.0f;
    rs[WINED3D_RS_ADAPTIVETESS_W] = tmpfloat.d;
    rs[WINED3D_RS_ENABLEADAPTIVETESSELLATION] = FALSE;
    rs[WINED3D_RS_TWOSIDEDSTENCILMODE] = FALSE;
    rs[WINED3D_RS_BACK_STENCILFAIL] = WINED3D_STENCIL_OP_KEEP;
    rs[WINED3D_RS_BACK_STENCILZFAIL] = WINED3D_STENCIL_OP_KEEP;
    rs[WINED3D_RS_BACK_STENCILPASS] = WINED3D_STENCIL_OP_KEEP;
    rs[WINED3D_RS_BACK_STENCILFUNC] = WINED3D_CMP_ALWAYS;
    rs[WINED3D_RS_SRGBWRITEENABLE] = 0;
    rs[WINED3D_RS_DEPTHBIAS] = 0;
    rs[WINED3D_RS_WRAP8] = 0;
    rs[WINED3D_RS_WRAP9] = 0;
    rs[WINED3D_RS_WRAP10] = 0;
    rs[WINED3D_RS_WRAP11] = 0;
    rs[WINED3D_RS_WRAP12] = 0;
    rs[WINED3D_RS_WRAP13] = 0;
    rs[WINED3D_RS_WRAP14] = 0;
    rs[WINED3D_RS_WRAP15] = 0;
    rs[WINED3D_RS_SEPARATEALPHABLENDENABLE] = FALSE;
    rs[WINED3D_RS_SRCBLENDALPHA] = WINED3D_BLEND_ONE;
    rs[WINED3D_RS_DESTBLENDALPHA] = WINED3D_BLEND_ZERO;
    rs[WINED3D_RS_BLENDOPALPHA] = WINED3D_BLEND_OP_ADD;
    for (i = 0; i < MAX_RENDER_TARGETS; ++i)
        rs[WINED3D_RS_COLORWRITE(i)] = 0x0000000f;
}

static void init_default_texture_state(unsigned int i, DWORD stage[WINED3D_HIGHEST_TEXTURE_STATE + 1])
{
    stage[WINED3D_TSS_COLOR_OP] = i ? WINED3D_TOP_DISABLE : WINED3D_TOP_MODULATE;
    stage[WINED3D_TSS_COLOR_ARG1] = WINED3DTA_TEXTURE;
    stage[WINED3D_TSS_COLOR_ARG2] = WINED3DTA_CURRENT;
    stage[WINED3D_TSS_ALPHA_OP] = i ? WINED3D_TOP_DISABLE : WINED3D_TOP_SELECT_ARG1;
    stage[WINED3D_TSS_ALPHA_ARG1] = WINED3DTA_TEXTURE;
    stage[WINED3D_TSS_ALPHA_ARG2] = WINED3DTA_CURRENT;
    stage[WINED3D_TSS_BUMPENV_MAT00] = 0;
    stage[WINED3D_TSS_BUMPENV_MAT01] = 0;
    stage[WINED3D_TSS_BUMPENV_MAT10] = 0;
    stage[WINED3D_TSS_BUMPENV_MAT11] = 0;
    stage[WINED3D_TSS_TEXCOORD_INDEX] = i;
    stage[WINED3D_TSS_BUMPENV_LSCALE] = 0;
    stage[WINED3D_TSS_BUMPENV_LOFFSET] = 0;
    stage[WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS] = WINED3D_TTFF_DISABLE;
    stage[WINED3D_TSS_COLOR_ARG0] = WINED3DTA_CURRENT;
    stage[WINED3D_TSS_ALPHA_ARG0] = WINED3DTA_CURRENT;
    stage[WINED3D_TSS_RESULT_ARG] = WINED3DTA_CURRENT;
}

static void init_default_sampler_states(DWORD states[WINED3D_MAX_COMBINED_SAMPLERS][WINED3D_HIGHEST_SAMPLER_STATE + 1])
{
    unsigned int i;

    for (i = 0 ; i < WINED3D_MAX_COMBINED_SAMPLERS; ++i)
    {
        TRACE("Setting up default samplers states for sampler %u.\n", i);
        states[i][WINED3D_SAMP_ADDRESS_U] = WINED3D_TADDRESS_WRAP;
        states[i][WINED3D_SAMP_ADDRESS_V] = WINED3D_TADDRESS_WRAP;
        states[i][WINED3D_SAMP_ADDRESS_W] = WINED3D_TADDRESS_WRAP;
        states[i][WINED3D_SAMP_BORDER_COLOR] = 0;
        states[i][WINED3D_SAMP_MAG_FILTER] = WINED3D_TEXF_POINT;
        states[i][WINED3D_SAMP_MIN_FILTER] = WINED3D_TEXF_POINT;
        states[i][WINED3D_SAMP_MIP_FILTER] = WINED3D_TEXF_NONE;
        states[i][WINED3D_SAMP_MIPMAP_LOD_BIAS] = 0;
        states[i][WINED3D_SAMP_MAX_MIP_LEVEL] = 0;
        states[i][WINED3D_SAMP_MAX_ANISOTROPY] = 1;
        states[i][WINED3D_SAMP_SRGB_TEXTURE] = 0;
        /* TODO: Indicates which element of a multielement texture to use. */
        states[i][WINED3D_SAMP_ELEMENT_INDEX] = 0;
        /* TODO: Vertex offset in the presampled displacement map. */
        states[i][WINED3D_SAMP_DMAP_OFFSET] = 0;
    }
}

static void state_init_default(struct wined3d_state *state, const struct wined3d_d3d_info *d3d_info)
{
    unsigned int i;
    struct wined3d_matrix identity;

    TRACE("state %p, d3d_info %p.\n", state, d3d_info);

    get_identity_matrix(&identity);
    state->gl_primitive_type = ~0u;
    state->gl_patch_vertices = 0;

    /* Set some of the defaults for lights, transforms etc */
    state->transforms[WINED3D_TS_PROJECTION] = identity;
    state->transforms[WINED3D_TS_VIEW] = identity;
    for (i = 0; i < 256; ++i)
    {
        state->transforms[WINED3D_TS_WORLD_MATRIX(i)] = identity;
    }

    init_default_render_states(state->render_states, d3d_info);

    /* Texture Stage States - Put directly into state block, we will call function below */
    for (i = 0; i < WINED3D_MAX_TEXTURES; ++i)
    {
        TRACE("Setting up default texture states for texture Stage %u.\n", i);
        state->transforms[WINED3D_TS_TEXTURE0 + i] = identity;
        init_default_texture_state(i, state->texture_states[i]);
    }

    init_default_sampler_states(state->sampler_states);

    state->blend_factor.r = 1.0f;
    state->blend_factor.g = 1.0f;
    state->blend_factor.b = 1.0f;
    state->blend_factor.a = 1.0f;

    for (i = 0; i < WINED3D_MAX_STREAMS; ++i)
        state->streams[i].frequency = 1;
}

void state_init(struct wined3d_state *state, struct wined3d_fb_state *fb,
        const struct wined3d_d3d_info *d3d_info, DWORD flags)
{
    unsigned int i;

    state->flags = flags;
    state->fb = fb;

    for (i = 0; i < LIGHTMAP_SIZE; i++)
    {
        list_init(&state->light_state.light_map[i]);
    }

    if (flags & WINED3D_STATE_INIT_DEFAULT)
        state_init_default(state, d3d_info);
}

static void stateblock_state_init_default(struct wined3d_stateblock_state *state,
        const struct wined3d_d3d_info *d3d_info)
{
    struct wined3d_matrix identity;
    unsigned int i;

    get_identity_matrix(&identity);

    state->transforms[WINED3D_TS_PROJECTION] = identity;
    state->transforms[WINED3D_TS_VIEW] = identity;
    for (i = 0; i < 256; ++i)
    {
        state->transforms[WINED3D_TS_WORLD_MATRIX(i)] = identity;
    }

    init_default_render_states(state->rs, d3d_info);

    for (i = 0; i < WINED3D_MAX_TEXTURES; ++i)
    {
        state->transforms[WINED3D_TS_TEXTURE0 + i] = identity;
        init_default_texture_state(i, state->texture_states[i]);
    }

    init_default_sampler_states(state->sampler_states);

    state->blend_factor.r = 1.0f;
    state->blend_factor.g = 1.0f;
    state->blend_factor.b = 1.0f;
    state->blend_factor.a = 1.0f;

    for (i = 0; i < WINED3D_MAX_STREAMS; ++i)
        state->streams[i].frequency = 1;
}

void wined3d_stateblock_state_init(struct wined3d_stateblock_state *state,
        const struct wined3d_device *device, DWORD flags)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(state->light_state.light_map); i++)
    {
        list_init(&state->light_state.light_map[i]);
    }

    if (flags & WINED3D_STATE_INIT_DEFAULT)
        stateblock_state_init_default(state, &device->adapter->d3d_info);

}

static HRESULT stateblock_init(struct wined3d_stateblock *stateblock,
        struct wined3d_device *device, enum wined3d_stateblock_type type)
{
    const struct wined3d_d3d_info *d3d_info = &device->adapter->d3d_info;

    stateblock->ref = 1;
    stateblock->device = device;
    wined3d_stateblock_state_init(&stateblock->stateblock_state, device,
            type == WINED3D_SBT_PRIMARY ? WINED3D_STATE_INIT_DEFAULT : 0);

    stateblock->changed.store_stream_offset = 1;

    if (type == WINED3D_SBT_RECORDED || type == WINED3D_SBT_PRIMARY)
        return WINED3D_OK;

    TRACE("Updating changed flags appropriate for type %#x.\n", type);

    switch (type)
    {
        case WINED3D_SBT_ALL:
            stateblock_init_lights(stateblock->stateblock_state.light_state.light_map,
                    device->stateblock_state.light_state.light_map);
            stateblock_savedstates_set_all(&stateblock->changed,
                    d3d_info->limits.vs_uniform_count_swvp, d3d_info->limits.ps_uniform_count);
            break;

        case WINED3D_SBT_PIXEL_STATE:
            stateblock_savedstates_set_pixel(&stateblock->changed,
                    d3d_info->limits.ps_uniform_count);
            break;

        case WINED3D_SBT_VERTEX_STATE:
            stateblock_init_lights(stateblock->stateblock_state.light_state.light_map,
                    device->stateblock_state.light_state.light_map);
            stateblock_savedstates_set_vertex(&stateblock->changed,
                    d3d_info->limits.vs_uniform_count_swvp);
            break;

        default:
            FIXME("Unrecognized state block type %#x.\n", type);
            break;
    }

    stateblock_init_contained_states(stateblock);
    wined3d_stateblock_capture(stateblock);

    /* According to the tests, stream offset is not updated in the captured state if
     * the state was captured on state block creation. This is not the case for
     * state blocks initialized with BeginStateBlock / EndStateBlock, multiple
     * captures get stream offsets updated. */
    stateblock->changed.store_stream_offset = 0;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_stateblock_create(struct wined3d_device *device,
        enum wined3d_stateblock_type type, struct wined3d_stateblock **stateblock)
{
    struct wined3d_stateblock *object;
    HRESULT hr;

    TRACE("device %p, type %#x, stateblock %p.\n",
            device, type, stateblock);

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    hr = stateblock_init(object, device, type);
    if (FAILED(hr))
    {
        WARN("Failed to initialize stateblock, hr %#x.\n", hr);
        heap_free(object);
        return hr;
    }

    TRACE("Created stateblock %p.\n", object);
    *stateblock = object;

    return WINED3D_OK;
}