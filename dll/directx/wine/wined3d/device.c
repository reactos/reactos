/*
 * Copyright 2002 Lionel Ulmer
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006-2008 Stefan Dösinger for CodeWeavers
 * Copyright 2006-2008 Henri Verbeet
 * Copyright 2007 Andrew Riedi
 * Copyright 2009-2011 Henri Verbeet for CodeWeavers
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

#include <stdio.h>
#ifdef HAVE_FLOAT_H
# include <float.h>
#endif

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

/* Define the default light parameters as specified by MSDN. */
const struct wined3d_light WINED3D_default_light =
{
    WINED3D_LIGHT_DIRECTIONAL,  /* Type */
    { 1.0f, 1.0f, 1.0f, 0.0f }, /* Diffuse r,g,b,a */
    { 0.0f, 0.0f, 0.0f, 0.0f }, /* Specular r,g,b,a */
    { 0.0f, 0.0f, 0.0f, 0.0f }, /* Ambient r,g,b,a, */
    { 0.0f, 0.0f, 0.0f },       /* Position x,y,z */
    { 0.0f, 0.0f, 1.0f },       /* Direction x,y,z */
    0.0f,                       /* Range */
    0.0f,                       /* Falloff */
    0.0f, 0.0f, 0.0f,           /* Attenuation 0,1,2 */
    0.0f,                       /* Theta */
    0.0f                        /* Phi */
};

/**********************************************************
 * Global variable / Constants follow
 **********************************************************/
const struct wined3d_matrix identity =
{{{
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f,
}}};  /* When needed for comparisons */

/* Note that except for WINED3DPT_POINTLIST and WINED3DPT_LINELIST these
 * actually have the same values in GL and D3D. */
static GLenum gl_primitive_type_from_d3d(enum wined3d_primitive_type primitive_type)
{
    switch(primitive_type)
    {
        case WINED3D_PT_POINTLIST:
            return GL_POINTS;

        case WINED3D_PT_LINELIST:
            return GL_LINES;

        case WINED3D_PT_LINESTRIP:
            return GL_LINE_STRIP;

        case WINED3D_PT_TRIANGLELIST:
            return GL_TRIANGLES;

        case WINED3D_PT_TRIANGLESTRIP:
            return GL_TRIANGLE_STRIP;

        case WINED3D_PT_TRIANGLEFAN:
            return GL_TRIANGLE_FAN;

        case WINED3D_PT_LINELIST_ADJ:
            return GL_LINES_ADJACENCY_ARB;

        case WINED3D_PT_LINESTRIP_ADJ:
            return GL_LINE_STRIP_ADJACENCY_ARB;

        case WINED3D_PT_TRIANGLELIST_ADJ:
            return GL_TRIANGLES_ADJACENCY_ARB;

        case WINED3D_PT_TRIANGLESTRIP_ADJ:
            return GL_TRIANGLE_STRIP_ADJACENCY_ARB;

        default:
            FIXME("Unhandled primitive type %s\n", debug_d3dprimitivetype(primitive_type));
            return GL_NONE;
    }
}

static enum wined3d_primitive_type d3d_primitive_type_from_gl(GLenum primitive_type)
{
    switch(primitive_type)
    {
        case GL_POINTS:
            return WINED3D_PT_POINTLIST;

        case GL_LINES:
            return WINED3D_PT_LINELIST;

        case GL_LINE_STRIP:
            return WINED3D_PT_LINESTRIP;

        case GL_TRIANGLES:
            return WINED3D_PT_TRIANGLELIST;

        case GL_TRIANGLE_STRIP:
            return WINED3D_PT_TRIANGLESTRIP;

        case GL_TRIANGLE_FAN:
            return WINED3D_PT_TRIANGLEFAN;

        case GL_LINES_ADJACENCY_ARB:
            return WINED3D_PT_LINELIST_ADJ;

        case GL_LINE_STRIP_ADJACENCY_ARB:
            return WINED3D_PT_LINESTRIP_ADJ;

        case GL_TRIANGLES_ADJACENCY_ARB:
            return WINED3D_PT_TRIANGLELIST_ADJ;

        case GL_TRIANGLE_STRIP_ADJACENCY_ARB:
            return WINED3D_PT_TRIANGLESTRIP_ADJ;

        default:
            FIXME("Unhandled primitive type %s\n", debug_d3dprimitivetype(primitive_type));
            return WINED3D_PT_UNDEFINED;
    }
}

static BOOL fixed_get_input(BYTE usage, BYTE usage_idx, unsigned int *regnum)
{
    if ((usage == WINED3D_DECL_USAGE_POSITION || usage == WINED3D_DECL_USAGE_POSITIONT) && !usage_idx)
        *regnum = WINED3D_FFP_POSITION;
    else if (usage == WINED3D_DECL_USAGE_BLEND_WEIGHT && !usage_idx)
        *regnum = WINED3D_FFP_BLENDWEIGHT;
    else if (usage == WINED3D_DECL_USAGE_BLEND_INDICES && !usage_idx)
        *regnum = WINED3D_FFP_BLENDINDICES;
    else if (usage == WINED3D_DECL_USAGE_NORMAL && !usage_idx)
        *regnum = WINED3D_FFP_NORMAL;
    else if (usage == WINED3D_DECL_USAGE_PSIZE && !usage_idx)
        *regnum = WINED3D_FFP_PSIZE;
    else if (usage == WINED3D_DECL_USAGE_COLOR && !usage_idx)
        *regnum = WINED3D_FFP_DIFFUSE;
    else if (usage == WINED3D_DECL_USAGE_COLOR && usage_idx == 1)
        *regnum = WINED3D_FFP_SPECULAR;
    else if (usage == WINED3D_DECL_USAGE_TEXCOORD && usage_idx < WINED3DDP_MAXTEXCOORD)
        *regnum = WINED3D_FFP_TEXCOORD0 + usage_idx;
    else
    {
        FIXME("Unsupported input stream [usage=%s, usage_idx=%u]\n", debug_d3ddeclusage(usage), usage_idx);
        *regnum = ~0U;
        return FALSE;
    }

    return TRUE;
}

/* Context activation is done by the caller. */
void device_stream_info_from_declaration(struct wined3d_device *device, struct wined3d_stream_info *stream_info)
{
    const struct wined3d_state *state = &device->stateBlock->state;
    /* We need to deal with frequency data! */
    struct wined3d_vertex_declaration *declaration = state->vertex_declaration;
    BOOL use_vshader;
    unsigned int i;

    stream_info->use_map = 0;
    stream_info->swizzle_map = 0;

    /* Check for transformed vertices, disable vertex shader if present. */
    stream_info->position_transformed = declaration->position_transformed;
    use_vshader = state->vertex_shader && !declaration->position_transformed;

    /* Translate the declaration into strided data. */
    for (i = 0; i < declaration->element_count; ++i)
    {
        const struct wined3d_vertex_declaration_element *element = &declaration->elements[i];
        const struct wined3d_stream_state *stream = &state->streams[element->input_slot];
        struct wined3d_buffer *buffer = stream->buffer;
        struct wined3d_bo_address data;
        BOOL stride_used;
        unsigned int idx;
        DWORD stride;

        TRACE("%p Element %p (%u of %u)\n", declaration->elements,
                element, i + 1, declaration->element_count);

        if (!buffer) continue;

        data.buffer_object = 0;
        data.addr = NULL;

        stride = stream->stride;
        if (state->user_stream)
        {
            TRACE("Stream %u is UP, %p\n", element->input_slot, buffer);
            data.buffer_object = 0;
            data.addr = (BYTE *)buffer;
        }
        else
        {
            TRACE("Stream %u isn't UP, %p\n", element->input_slot, buffer);
            buffer_get_memory(buffer, &device->adapter->gl_info, &data);

            /* Can't use vbo's if the base vertex index is negative. OpenGL doesn't accept negative offsets
             * (or rather offsets bigger than the vbo, because the pointer is unsigned), so use system memory
             * sources. In most sane cases the pointer - offset will still be > 0, otherwise it will wrap
             * around to some big value. Hope that with the indices, the driver wraps it back internally. If
             * not, drawStridedSlow is needed, including a vertex buffer path. */
            if (state->load_base_vertex_index < 0)
            {
                WARN("load_base_vertex_index is < 0 (%d), not using VBOs.\n",
                        state->load_base_vertex_index);
                data.buffer_object = 0;
                data.addr = buffer_get_sysmem(buffer, &device->adapter->gl_info);
                if ((UINT_PTR)data.addr < -state->load_base_vertex_index * stride)
                {
                    FIXME("System memory vertex data load offset is negative!\n");
                }
            }
        }
        data.addr += element->offset;

        TRACE("offset %u input_slot %u usage_idx %d\n", element->offset, element->input_slot, element->usage_idx);

        if (use_vshader)
        {
            if (element->output_slot == ~0U)
            {
                /* TODO: Assuming vertexdeclarations are usually used with the
                 * same or a similar shader, it might be worth it to store the
                 * last used output slot and try that one first. */
                stride_used = vshader_get_input(state->vertex_shader,
                        element->usage, element->usage_idx, &idx);
            }
            else
            {
                idx = element->output_slot;
                stride_used = TRUE;
            }
        }
        else
        {
            if (!element->ffp_valid)
            {
                WARN("Skipping unsupported fixed function element of format %s and usage %s\n",
                        debug_d3dformat(element->format->id), debug_d3ddeclusage(element->usage));
                stride_used = FALSE;
            }
            else
            {
                stride_used = fixed_get_input(element->usage, element->usage_idx, &idx);
            }
        }

        if (stride_used)
        {
            TRACE("Load %s array %u [usage %s, usage_idx %u, "
                    "input_slot %u, offset %u, stride %u, format %s, buffer_object %u]\n",
                    use_vshader ? "shader": "fixed function", idx,
                    debug_d3ddeclusage(element->usage), element->usage_idx, element->input_slot,
                    element->offset, stride, debug_d3dformat(element->format->id), data.buffer_object);

            data.addr += stream->offset;

            stream_info->elements[idx].format = element->format;
            stream_info->elements[idx].data = data;
            stream_info->elements[idx].stride = stride;
            stream_info->elements[idx].stream_idx = element->input_slot;

            if (!device->adapter->gl_info.supported[ARB_VERTEX_ARRAY_BGRA]
                    && element->format->id == WINED3DFMT_B8G8R8A8_UNORM)
            {
                stream_info->swizzle_map |= 1 << idx;
            }
            stream_info->use_map |= 1 << idx;
        }
    }

    device->num_buffer_queries = 0;
    if (!state->user_stream)
    {
        WORD map = stream_info->use_map;
        stream_info->all_vbo = 1;

        /* PreLoad all the vertex buffers. */
        for (i = 0; map; map >>= 1, ++i)
        {
            struct wined3d_stream_info_element *element;
            struct wined3d_buffer *buffer;

            if (!(map & 1)) continue;

            element = &stream_info->elements[i];
            buffer = state->streams[element->stream_idx].buffer;
            wined3d_buffer_preload(buffer);

            /* If the preload dropped the buffer object, update the stream info. */
            if (buffer->buffer_object != element->data.buffer_object)
            {
                element->data.buffer_object = 0;
                element->data.addr = buffer_get_sysmem(buffer, &device->adapter->gl_info)
                        + (ptrdiff_t)element->data.addr;
            }

            if (!buffer->buffer_object)
                stream_info->all_vbo = 0;

            if (buffer->query)
                device->buffer_queries[device->num_buffer_queries++] = buffer->query;
        }
    }
    else
    {
        stream_info->all_vbo = 0;
    }
}

static void stream_info_element_from_strided(const struct wined3d_gl_info *gl_info,
        const struct wined3d_strided_element *strided, struct wined3d_stream_info_element *e)
{
    e->data.addr = strided->data;
    e->data.buffer_object = 0;
    e->format = wined3d_get_format(gl_info, strided->format);
    e->stride = strided->stride;
    e->stream_idx = 0;
}

static void device_stream_info_from_strided(const struct wined3d_gl_info *gl_info,
        const struct wined3d_strided_data *strided, struct wined3d_stream_info *stream_info)
{
    unsigned int i;

    memset(stream_info, 0, sizeof(*stream_info));

    if (strided->position.data)
        stream_info_element_from_strided(gl_info, &strided->position, &stream_info->elements[WINED3D_FFP_POSITION]);
    if (strided->normal.data)
        stream_info_element_from_strided(gl_info, &strided->normal, &stream_info->elements[WINED3D_FFP_NORMAL]);
    if (strided->diffuse.data)
        stream_info_element_from_strided(gl_info, &strided->diffuse, &stream_info->elements[WINED3D_FFP_DIFFUSE]);
    if (strided->specular.data)
        stream_info_element_from_strided(gl_info, &strided->specular, &stream_info->elements[WINED3D_FFP_SPECULAR]);

    for (i = 0; i < WINED3DDP_MAXTEXCOORD; ++i)
    {
        if (strided->tex_coords[i].data)
            stream_info_element_from_strided(gl_info, &strided->tex_coords[i],
                    &stream_info->elements[WINED3D_FFP_TEXCOORD0 + i]);
    }

    stream_info->position_transformed = strided->position_transformed;

    for (i = 0; i < sizeof(stream_info->elements) / sizeof(*stream_info->elements); ++i)
    {
        if (!stream_info->elements[i].format) continue;

        if (!gl_info->supported[ARB_VERTEX_ARRAY_BGRA]
                && stream_info->elements[i].format->id == WINED3DFMT_B8G8R8A8_UNORM)
        {
            stream_info->swizzle_map |= 1 << i;
        }
        stream_info->use_map |= 1 << i;
    }
}

static void device_trace_strided_stream_info(const struct wined3d_stream_info *stream_info)
{
    TRACE("Strided Data:\n");
    TRACE_STRIDED(stream_info, WINED3D_FFP_POSITION);
    TRACE_STRIDED(stream_info, WINED3D_FFP_BLENDWEIGHT);
    TRACE_STRIDED(stream_info, WINED3D_FFP_BLENDINDICES);
    TRACE_STRIDED(stream_info, WINED3D_FFP_NORMAL);
    TRACE_STRIDED(stream_info, WINED3D_FFP_PSIZE);
    TRACE_STRIDED(stream_info, WINED3D_FFP_DIFFUSE);
    TRACE_STRIDED(stream_info, WINED3D_FFP_SPECULAR);
    TRACE_STRIDED(stream_info, WINED3D_FFP_TEXCOORD0);
    TRACE_STRIDED(stream_info, WINED3D_FFP_TEXCOORD1);
    TRACE_STRIDED(stream_info, WINED3D_FFP_TEXCOORD2);
    TRACE_STRIDED(stream_info, WINED3D_FFP_TEXCOORD3);
    TRACE_STRIDED(stream_info, WINED3D_FFP_TEXCOORD4);
    TRACE_STRIDED(stream_info, WINED3D_FFP_TEXCOORD5);
    TRACE_STRIDED(stream_info, WINED3D_FFP_TEXCOORD6);
    TRACE_STRIDED(stream_info, WINED3D_FFP_TEXCOORD7);
}

/* Context activation is done by the caller. */
void device_update_stream_info(struct wined3d_device *device, const struct wined3d_gl_info *gl_info)
{
    struct wined3d_stream_info *stream_info = &device->strided_streams;
    const struct wined3d_state *state = &device->stateBlock->state;
    DWORD prev_all_vbo = stream_info->all_vbo;

    if (device->up_strided)
    {
        /* Note: this is a ddraw fixed-function code path. */
        TRACE("=============================== Strided Input ================================\n");
        device_stream_info_from_strided(gl_info, device->up_strided, stream_info);
        if (TRACE_ON(d3d)) device_trace_strided_stream_info(stream_info);
    }
    else
    {
        TRACE("============================= Vertex Declaration =============================\n");
        device_stream_info_from_declaration(device, stream_info);
    }

    if (state->vertex_shader && !stream_info->position_transformed)
    {
        if (state->vertex_declaration->half_float_conv_needed && !stream_info->all_vbo)
        {
            TRACE("Using drawStridedSlow with vertex shaders for FLOAT16 conversion.\n");
            device->useDrawStridedSlow = TRUE;
        }
        else
        {
            device->useDrawStridedSlow = FALSE;
        }
    }
    else
    {
        WORD slow_mask = (1 << WINED3D_FFP_PSIZE);
        slow_mask |= -!gl_info->supported[ARB_VERTEX_ARRAY_BGRA]
                & ((1 << WINED3D_FFP_DIFFUSE) | (1 << WINED3D_FFP_SPECULAR));

        if ((stream_info->position_transformed || (stream_info->use_map & slow_mask)) && !stream_info->all_vbo)
        {
            device->useDrawStridedSlow = TRUE;
        }
        else
        {
            device->useDrawStridedSlow = FALSE;
        }
    }

    if (prev_all_vbo != stream_info->all_vbo)
        device_invalidate_state(device, STATE_INDEXBUFFER);
}

static void device_preload_texture(const struct wined3d_state *state, unsigned int idx)
{
    struct wined3d_texture *texture;
    enum WINED3DSRGB srgb;

    if (!(texture = state->textures[idx])) return;
    srgb = state->sampler_states[idx][WINED3D_SAMP_SRGB_TEXTURE] ? SRGB_SRGB : SRGB_RGB;
    texture->texture_ops->texture_preload(texture, srgb);
}

void device_preload_textures(const struct wined3d_device *device)
{
    const struct wined3d_state *state = &device->stateBlock->state;
    unsigned int i;

    if (use_vs(state))
    {
        for (i = 0; i < MAX_VERTEX_SAMPLERS; ++i)
        {
            if (state->vertex_shader->reg_maps.sampler_type[i])
                device_preload_texture(state, MAX_FRAGMENT_SAMPLERS + i);
        }
    }

    if (use_ps(state))
    {
        for (i = 0; i < MAX_FRAGMENT_SAMPLERS; ++i)
        {
            if (state->pixel_shader->reg_maps.sampler_type[i])
                device_preload_texture(state, i);
        }
    }
    else
    {
        WORD ffu_map = device->fixed_function_usage_map;

        for (i = 0; ffu_map; ffu_map >>= 1, ++i)
        {
            if (ffu_map & 1)
                device_preload_texture(state, i);
        }
    }
}

BOOL device_context_add(struct wined3d_device *device, struct wined3d_context *context)
{
    struct wined3d_context **new_array;

    TRACE("Adding context %p.\n", context);

    if (!device->contexts) new_array = HeapAlloc(GetProcessHeap(), 0, sizeof(*new_array));
    else new_array = HeapReAlloc(GetProcessHeap(), 0, device->contexts,
            sizeof(*new_array) * (device->context_count + 1));

    if (!new_array)
    {
        ERR("Failed to grow the context array.\n");
        return FALSE;
    }

    new_array[device->context_count++] = context;
    device->contexts = new_array;
    return TRUE;
}

void device_context_remove(struct wined3d_device *device, struct wined3d_context *context)
{
    struct wined3d_context **new_array;
    BOOL found = FALSE;
    UINT i;

    TRACE("Removing context %p.\n", context);

    for (i = 0; i < device->context_count; ++i)
    {
        if (device->contexts[i] == context)
        {
            found = TRUE;
            break;
        }
    }

    if (!found)
    {
        ERR("Context %p doesn't exist in context array.\n", context);
        return;
    }

    if (!--device->context_count)
    {
        HeapFree(GetProcessHeap(), 0, device->contexts);
        device->contexts = NULL;
        return;
    }

    memmove(&device->contexts[i], &device->contexts[i + 1], (device->context_count - i) * sizeof(*device->contexts));
    new_array = HeapReAlloc(GetProcessHeap(), 0, device->contexts, device->context_count * sizeof(*device->contexts));
    if (!new_array)
    {
        ERR("Failed to shrink context array. Oh well.\n");
        return;
    }

    device->contexts = new_array;
}

/* Do not call while under the GL lock. */
void device_switch_onscreen_ds(struct wined3d_device *device,
        struct wined3d_context *context, struct wined3d_surface *depth_stencil)
{
    if (device->onscreen_depth_stencil)
    {
        surface_load_ds_location(device->onscreen_depth_stencil, context, SFLAG_INTEXTURE);

        surface_modify_ds_location(device->onscreen_depth_stencil, SFLAG_INTEXTURE,
                device->onscreen_depth_stencil->ds_current_size.cx,
                device->onscreen_depth_stencil->ds_current_size.cy);
        wined3d_surface_decref(device->onscreen_depth_stencil);
    }
    device->onscreen_depth_stencil = depth_stencil;
    wined3d_surface_incref(device->onscreen_depth_stencil);
}

static BOOL is_full_clear(const struct wined3d_surface *target, const RECT *draw_rect, const RECT *clear_rect)
{
    /* partial draw rect */
    if (draw_rect->left || draw_rect->top
            || draw_rect->right < target->resource.width
            || draw_rect->bottom < target->resource.height)
        return FALSE;

    /* partial clear rect */
    if (clear_rect && (clear_rect->left > 0 || clear_rect->top > 0
            || clear_rect->right < target->resource.width
            || clear_rect->bottom < target->resource.height))
        return FALSE;

    return TRUE;
}

static void prepare_ds_clear(struct wined3d_surface *ds, struct wined3d_context *context,
        DWORD location, const RECT *draw_rect, UINT rect_count, const RECT *clear_rect, RECT *out_rect)
{
    RECT current_rect, r;

    if (ds->flags & location)
        SetRect(&current_rect, 0, 0,
                ds->ds_current_size.cx,
                ds->ds_current_size.cy);
    else
        SetRectEmpty(&current_rect);

    IntersectRect(&r, draw_rect, &current_rect);
    if (EqualRect(&r, draw_rect))
    {
        /* current_rect ⊇ draw_rect, modify only. */
        SetRect(out_rect, 0, 0, ds->ds_current_size.cx, ds->ds_current_size.cy);
        return;
    }

    if (EqualRect(&r, &current_rect))
    {
        /* draw_rect ⊇ current_rect, test if we're doing a full clear. */

        if (!clear_rect)
        {
            /* Full clear, modify only. */
            *out_rect = *draw_rect;
            return;
        }

        IntersectRect(&r, draw_rect, clear_rect);
        if (EqualRect(&r, draw_rect))
        {
            /* clear_rect ⊇ draw_rect, modify only. */
            *out_rect = *draw_rect;
            return;
        }
    }

    /* Full load. */
    surface_load_ds_location(ds, context, location);
    SetRect(out_rect, 0, 0, ds->ds_current_size.cx, ds->ds_current_size.cy);
}

/* Do not call while under the GL lock. */
void device_clear_render_targets(struct wined3d_device *device, UINT rt_count, const struct wined3d_fb_state *fb,
        UINT rect_count, const RECT *rects, const RECT *draw_rect, DWORD flags, const struct wined3d_color *color,
        float depth, DWORD stencil)
{
    const RECT *clear_rect = (rect_count > 0 && rects) ? (const RECT *)rects : NULL;
    struct wined3d_surface *target = rt_count ? fb->render_targets[0] : NULL;
    const struct wined3d_gl_info *gl_info;
    UINT drawable_width, drawable_height;
    struct wined3d_context *context;
    GLbitfield clear_mask = 0;
    BOOL render_offscreen;
    unsigned int i;
    RECT ds_rect;

    /* When we're clearing parts of the drawable, make sure that the target surface is well up to date in the
     * drawable. After the clear we'll mark the drawable up to date, so we have to make sure that this is true
     * for the cleared parts, and the untouched parts.
     *
     * If we're clearing the whole target there is no need to copy it into the drawable, it will be overwritten
     * anyway. If we're not clearing the color buffer we don't have to copy either since we're not going to set
     * the drawable up to date. We have to check all settings that limit the clear area though. Do not bother
     * checking all this if the dest surface is in the drawable anyway. */
    if (flags & WINED3DCLEAR_TARGET && !is_full_clear(target, draw_rect, clear_rect))
    {
        for (i = 0; i < rt_count; ++i)
        {
            struct wined3d_surface *rt = fb->render_targets[i];
            if (rt)
                surface_load_location(rt, rt->draw_binding, NULL);
        }
    }

    context = context_acquire(device, target);
    if (!context->valid)
    {
        context_release(context);
        WARN("Invalid context, skipping clear.\n");
        return;
    }
    gl_info = context->gl_info;

    if (target)
    {
        render_offscreen = context->render_offscreen;
        target->get_drawable_size(context, &drawable_width, &drawable_height);
    }
    else
    {
        render_offscreen = TRUE;
        drawable_width = fb->depth_stencil->pow2Width;
        drawable_height = fb->depth_stencil->pow2Height;
    }

    if (flags & WINED3DCLEAR_ZBUFFER)
    {
        DWORD location = render_offscreen ? fb->depth_stencil->draw_binding : SFLAG_INDRAWABLE;

        if (!render_offscreen && fb->depth_stencil != device->onscreen_depth_stencil)
            device_switch_onscreen_ds(device, context, fb->depth_stencil);
        prepare_ds_clear(fb->depth_stencil, context, location,
                draw_rect, rect_count, clear_rect, &ds_rect);
    }

    if (!context_apply_clear_state(context, device, rt_count, fb))
    {
        context_release(context);
        WARN("Failed to apply clear state, skipping clear.\n");
        return;
    }

    ENTER_GL();

    /* Only set the values up once, as they are not changing. */
    if (flags & WINED3DCLEAR_STENCIL)
    {
        if (gl_info->supported[EXT_STENCIL_TWO_SIDE])
        {
            gl_info->gl_ops.gl.p_glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
            context_invalidate_state(context, STATE_RENDER(WINED3D_RS_TWOSIDEDSTENCILMODE));
        }
        gl_info->gl_ops.gl.p_glStencilMask(~0U);
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_STENCILWRITEMASK));
        gl_info->gl_ops.gl.p_glClearStencil(stencil);
        checkGLcall("glClearStencil");
        clear_mask = clear_mask | GL_STENCIL_BUFFER_BIT;
    }

    if (flags & WINED3DCLEAR_ZBUFFER)
    {
        DWORD location = render_offscreen ? fb->depth_stencil->draw_binding : SFLAG_INDRAWABLE;

        surface_modify_ds_location(fb->depth_stencil, location, ds_rect.right, ds_rect.bottom);

        gl_info->gl_ops.gl.p_glDepthMask(GL_TRUE);
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_ZWRITEENABLE));
        gl_info->gl_ops.gl.p_glClearDepth(depth);
        checkGLcall("glClearDepth");
        clear_mask = clear_mask | GL_DEPTH_BUFFER_BIT;
    }

    if (flags & WINED3DCLEAR_TARGET)
    {
        for (i = 0; i < rt_count; ++i)
        {
            struct wined3d_surface *rt = fb->render_targets[i];

            if (rt)
                surface_modify_location(rt, rt->draw_binding, TRUE);
        }

        gl_info->gl_ops.gl.p_glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITEENABLE));
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITEENABLE1));
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITEENABLE2));
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITEENABLE3));
        gl_info->gl_ops.gl.p_glClearColor(color->r, color->g, color->b, color->a);
        checkGLcall("glClearColor");
        clear_mask = clear_mask | GL_COLOR_BUFFER_BIT;
    }

    if (!clear_rect)
    {
        if (render_offscreen)
        {
            gl_info->gl_ops.gl.p_glScissor(draw_rect->left, draw_rect->top,
                    draw_rect->right - draw_rect->left, draw_rect->bottom - draw_rect->top);
        }
        else
        {
            gl_info->gl_ops.gl.p_glScissor(draw_rect->left, drawable_height - draw_rect->bottom,
                        draw_rect->right - draw_rect->left, draw_rect->bottom - draw_rect->top);
        }
        checkGLcall("glScissor");
        gl_info->gl_ops.gl.p_glClear(clear_mask);
        checkGLcall("glClear");
    }
    else
    {
        RECT current_rect;

        /* Now process each rect in turn. */
        for (i = 0; i < rect_count; ++i)
        {
            /* Note that GL uses lower left, width/height. */
            IntersectRect(&current_rect, draw_rect, &clear_rect[i]);

            TRACE("clear_rect[%u] %s, current_rect %s.\n", i,
                    wine_dbgstr_rect(&clear_rect[i]),
                    wine_dbgstr_rect(&current_rect));

            /* Tests show that rectangles where x1 > x2 or y1 > y2 are ignored silently.
             * The rectangle is not cleared, no error is returned, but further rectangles are
             * still cleared if they are valid. */
            if (current_rect.left > current_rect.right || current_rect.top > current_rect.bottom)
            {
                TRACE("Rectangle with negative dimensions, ignoring.\n");
                continue;
            }

            if (render_offscreen)
            {
                gl_info->gl_ops.gl.p_glScissor(current_rect.left, current_rect.top,
                        current_rect.right - current_rect.left, current_rect.bottom - current_rect.top);
            }
            else
            {
                gl_info->gl_ops.gl.p_glScissor(current_rect.left, drawable_height - current_rect.bottom,
                          current_rect.right - current_rect.left, current_rect.bottom - current_rect.top);
            }
            checkGLcall("glScissor");

            gl_info->gl_ops.gl.p_glClear(clear_mask);
            checkGLcall("glClear");
        }
    }

    LEAVE_GL();

    if (wined3d_settings.strict_draw_ordering || (flags & WINED3DCLEAR_TARGET
            && target->container.type == WINED3D_CONTAINER_SWAPCHAIN
            && target->container.u.swapchain->front_buffer == target))
        gl_info->gl_ops.gl.p_glFlush(); /* Flush to ensure ordering across contexts. */

    context_release(context);
}

ULONG CDECL wined3d_device_incref(struct wined3d_device *device)
{
    ULONG refcount = InterlockedIncrement(&device->ref);

    TRACE("%p increasing refcount to %u.\n", device, refcount);

    return refcount;
}

ULONG CDECL wined3d_device_decref(struct wined3d_device *device)
{
    ULONG refcount = InterlockedDecrement(&device->ref);

    TRACE("%p decreasing refcount to %u.\n", device, refcount);

    if (!refcount)
    {
        struct wined3d_stateblock *stateblock;
        UINT i;

        if (wined3d_stateblock_decref(device->updateStateBlock)
                && device->updateStateBlock != device->stateBlock)
            FIXME("Something's still holding the update stateblock.\n");
        device->updateStateBlock = NULL;

        stateblock = device->stateBlock;
        device->stateBlock = NULL;
        if (wined3d_stateblock_decref(stateblock))
            FIXME("Something's still holding the stateblock.\n");

        for (i = 0; i < sizeof(device->multistate_funcs) / sizeof(device->multistate_funcs[0]); ++i)
        {
            HeapFree(GetProcessHeap(), 0, device->multistate_funcs[i]);
            device->multistate_funcs[i] = NULL;
        }

        if (!list_empty(&device->resources))
        {
            struct wined3d_resource *resource;

            FIXME("Device released with resources still bound, acceptable but unexpected.\n");

            LIST_FOR_EACH_ENTRY(resource, &device->resources, struct wined3d_resource, resource_list_entry)
            {
                FIXME("Leftover resource %p with type %s (%#x).\n",
                        resource, debug_d3dresourcetype(resource->type), resource->type);
            }
        }

        if (device->contexts)
            ERR("Context array not freed!\n");
        if (device->hardwareCursor)
            DestroyCursor(device->hardwareCursor);
        device->hardwareCursor = 0;

        wined3d_decref(device->wined3d);
        device->wined3d = NULL;
        HeapFree(GetProcessHeap(), 0, device);
        TRACE("Freed device %p.\n", device);
    }

    return refcount;
}

UINT CDECL wined3d_device_get_swapchain_count(const struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    return device->swapchain_count;
}

HRESULT CDECL wined3d_device_get_swapchain(const struct wined3d_device *device,
        UINT swapchain_idx, struct wined3d_swapchain **swapchain)
{
    TRACE("device %p, swapchain_idx %u, swapchain %p.\n",
            device, swapchain_idx, swapchain);

    if (swapchain_idx >= device->swapchain_count)
    {
        WARN("swapchain_idx %u >= swapchain_count %u.\n",
                swapchain_idx, device->swapchain_count);
        *swapchain = NULL;

        return WINED3DERR_INVALIDCALL;
    }

    *swapchain = device->swapchains[swapchain_idx];
    wined3d_swapchain_incref(*swapchain);
    TRACE("Returning %p.\n", *swapchain);

    return WINED3D_OK;
}

static void device_load_logo(struct wined3d_device *device, const char *filename)
{
    struct wined3d_color_key color_key;
    HBITMAP hbm;
    BITMAP bm;
    HRESULT hr;
    HDC dcb = NULL, dcs = NULL;

    hbm = LoadImageA(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if(hbm)
    {
        GetObjectA(hbm, sizeof(BITMAP), &bm);
        dcb = CreateCompatibleDC(NULL);
        if(!dcb) goto out;
        SelectObject(dcb, hbm);
    }
    else
    {
        /* Create a 32x32 white surface to indicate that wined3d is used, but the specified image
         * couldn't be loaded
         */
        memset(&bm, 0, sizeof(bm));
        bm.bmWidth = 32;
        bm.bmHeight = 32;
    }

    hr = wined3d_surface_create(device, bm.bmWidth, bm.bmHeight, WINED3DFMT_B5G6R5_UNORM, 0, 0,
            WINED3D_POOL_SYSTEM_MEM, WINED3D_MULTISAMPLE_NONE, 0, WINED3D_SURFACE_TYPE_OPENGL, WINED3D_SURFACE_MAPPABLE,
            NULL, &wined3d_null_parent_ops, &device->logo_surface);
    if (FAILED(hr))
    {
        ERR("Wine logo requested, but failed to create surface, hr %#x.\n", hr);
        goto out;
    }

    if (dcb)
    {
        if (FAILED(hr = wined3d_surface_getdc(device->logo_surface, &dcs)))
            goto out;
        BitBlt(dcs, 0, 0, bm.bmWidth, bm.bmHeight, dcb, 0, 0, SRCCOPY);
        wined3d_surface_releasedc(device->logo_surface, dcs);

        color_key.color_space_low_value = 0;
        color_key.color_space_high_value = 0;
        wined3d_surface_set_color_key(device->logo_surface, WINEDDCKEY_SRCBLT, &color_key);
    }
    else
    {
        const struct wined3d_color c = {1.0f, 1.0f, 1.0f, 1.0f};
        /* Fill the surface with a white color to show that wined3d is there */
        wined3d_device_color_fill(device, device->logo_surface, NULL, &c);
    }

out:
    if (dcb) DeleteDC(dcb);
    if (hbm) DeleteObject(hbm);
}

/* Context activation is done by the caller. */
static void create_dummy_textures(struct wined3d_device *device, struct wined3d_context *context)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    unsigned int i, j, count;
    /* Under DirectX you can sample even if no texture is bound, whereas
     * OpenGL will only allow that when a valid texture is bound.
     * We emulate this by creating dummy textures and binding them
     * to each texture stage when the currently set D3D texture is NULL. */
    ENTER_GL();

    if (gl_info->supported[APPLE_CLIENT_STORAGE])
    {
        /* The dummy texture does not have client storage backing */
        gl_info->gl_ops.gl.p_glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
        checkGLcall("glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE)");
    }

    count = min(MAX_COMBINED_SAMPLERS, gl_info->limits.combined_samplers);
    for (i = 0; i < count; ++i)
    {
        DWORD color = 0x000000ff;

        /* Make appropriate texture active */
        context_active_texture(context, gl_info, i);

        gl_info->gl_ops.gl.p_glGenTextures(1, &device->dummy_texture_2d[i]);
        checkGLcall("glGenTextures");
        TRACE("Dummy 2D texture %u given name %u.\n", i, device->dummy_texture_2d[i]);

        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, device->dummy_texture_2d[i]);
        checkGLcall("glBindTexture");

        gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0,
                GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color);
        checkGLcall("glTexImage2D");

        if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
        {
            gl_info->gl_ops.gl.p_glGenTextures(1, &device->dummy_texture_rect[i]);
            checkGLcall("glGenTextures");
            TRACE("Dummy rectangle texture %u given name %u.\n", i, device->dummy_texture_rect[i]);

            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_RECTANGLE_ARB, device->dummy_texture_rect[i]);
            checkGLcall("glBindTexture");

            gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA8, 1, 1, 0,
                    GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color);
            checkGLcall("glTexImage2D");
        }

        if (gl_info->supported[EXT_TEXTURE3D])
        {
            gl_info->gl_ops.gl.p_glGenTextures(1, &device->dummy_texture_3d[i]);
            checkGLcall("glGenTextures");
            TRACE("Dummy 3D texture %u given name %u.\n", i, device->dummy_texture_3d[i]);

            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_3D, device->dummy_texture_3d[i]);
            checkGLcall("glBindTexture");

            GL_EXTCALL(glTexImage3DEXT(GL_TEXTURE_3D, 0, GL_RGBA8, 1, 1, 1, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color));
            checkGLcall("glTexImage3D");
        }

        if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
        {
            gl_info->gl_ops.gl.p_glGenTextures(1, &device->dummy_texture_cube[i]);
            checkGLcall("glGenTextures");
            TRACE("Dummy cube texture %u given name %u.\n", i, device->dummy_texture_cube[i]);

            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_CUBE_MAP, device->dummy_texture_cube[i]);
            checkGLcall("glBindTexture");

            for (j = GL_TEXTURE_CUBE_MAP_POSITIVE_X; j <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; ++j)
            {
                gl_info->gl_ops.gl.p_glTexImage2D(j, 0, GL_RGBA8, 1, 1, 0,
                        GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color);
                checkGLcall("glTexImage2D");
            }
        }
    }

    if (gl_info->supported[APPLE_CLIENT_STORAGE])
    {
        /* Re-enable because if supported it is enabled by default */
        gl_info->gl_ops.gl.p_glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
        checkGLcall("glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE)");
    }

    LEAVE_GL();
}

/* Context activation is done by the caller. */
static void destroy_dummy_textures(struct wined3d_device *device, const struct wined3d_gl_info *gl_info)
{
    unsigned int count = min(MAX_COMBINED_SAMPLERS, gl_info->limits.combined_samplers);

    ENTER_GL();
    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
    {
        gl_info->gl_ops.gl.p_glDeleteTextures(count, device->dummy_texture_cube);
        checkGLcall("glDeleteTextures(count, device->dummy_texture_cube)");
    }

    if (gl_info->supported[EXT_TEXTURE3D])
    {
        gl_info->gl_ops.gl.p_glDeleteTextures(count, device->dummy_texture_3d);
        checkGLcall("glDeleteTextures(count, device->dummy_texture_3d)");
    }

    if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
    {
        gl_info->gl_ops.gl.p_glDeleteTextures(count, device->dummy_texture_rect);
        checkGLcall("glDeleteTextures(count, device->dummy_texture_rect)");
    }

    gl_info->gl_ops.gl.p_glDeleteTextures(count, device->dummy_texture_2d);
    checkGLcall("glDeleteTextures(count, device->dummy_texture_2d)");
    LEAVE_GL();

    memset(device->dummy_texture_cube, 0, gl_info->limits.textures * sizeof(*device->dummy_texture_cube));
    memset(device->dummy_texture_3d, 0, gl_info->limits.textures * sizeof(*device->dummy_texture_3d));
    memset(device->dummy_texture_rect, 0, gl_info->limits.textures * sizeof(*device->dummy_texture_rect));
    memset(device->dummy_texture_2d, 0, gl_info->limits.textures * sizeof(*device->dummy_texture_2d));
}

static LONG fullscreen_style(LONG style)
{
    /* Make sure the window is managed, otherwise we won't get keyboard input. */
    style |= WS_POPUP | WS_SYSMENU;
    style &= ~(WS_CAPTION | WS_THICKFRAME);

    return style;
}

static LONG fullscreen_exstyle(LONG exstyle)
{
    /* Filter out window decorations. */
    exstyle &= ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE);

    return exstyle;
}

void CDECL wined3d_device_setup_fullscreen_window(struct wined3d_device *device, HWND window, UINT w, UINT h)
{
    BOOL filter_messages;
    LONG style, exstyle;

    TRACE("Setting up window %p for fullscreen mode.\n", window);

    if (device->style || device->exStyle)
    {
        ERR("Changing the window style for window %p, but another style (%08x, %08x) is already stored.\n",
                window, device->style, device->exStyle);
    }

    device->style = GetWindowLongW(window, GWL_STYLE);
    device->exStyle = GetWindowLongW(window, GWL_EXSTYLE);

    style = fullscreen_style(device->style);
    exstyle = fullscreen_exstyle(device->exStyle);

    TRACE("Old style was %08x, %08x, setting to %08x, %08x.\n",
            device->style, device->exStyle, style, exstyle);

    filter_messages = device->filter_messages;
    device->filter_messages = TRUE;

    SetWindowLongW(window, GWL_STYLE, style);
    SetWindowLongW(window, GWL_EXSTYLE, exstyle);
    SetWindowPos(window, HWND_TOP, 0, 0, w, h, SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOACTIVATE);

    device->filter_messages = filter_messages;
}

void CDECL wined3d_device_restore_fullscreen_window(struct wined3d_device *device, HWND window)
{
    BOOL filter_messages;
    LONG style, exstyle;

    if (!device->style && !device->exStyle) return;

    TRACE("Restoring window style of window %p to %08x, %08x.\n",
            window, device->style, device->exStyle);

    style = GetWindowLongW(window, GWL_STYLE);
    exstyle = GetWindowLongW(window, GWL_EXSTYLE);

    filter_messages = device->filter_messages;
    device->filter_messages = TRUE;

    /* Only restore the style if the application didn't modify it during the
     * fullscreen phase. Some applications change it before calling Reset()
     * when switching between windowed and fullscreen modes (HL2), some
     * depend on the original style (Eve Online). */
    if (style == fullscreen_style(device->style) && exstyle == fullscreen_exstyle(device->exStyle))
    {
        SetWindowLongW(window, GWL_STYLE, device->style);
        SetWindowLongW(window, GWL_EXSTYLE, device->exStyle);
    }
    SetWindowPos(window, 0, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    device->filter_messages = filter_messages;

    /* Delete the old values. */
    device->style = 0;
    device->exStyle = 0;
}

HRESULT CDECL wined3d_device_acquire_focus_window(struct wined3d_device *device, HWND window)
{
    TRACE("device %p, window %p.\n", device, window);

    if (!wined3d_register_window(window, device))
    {
        ERR("Failed to register window %p.\n", window);
        return E_FAIL;
    }

    InterlockedExchangePointer((void **)&device->focus_window, window);
    SetWindowPos(window, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

    return WINED3D_OK;
}

void CDECL wined3d_device_release_focus_window(struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    if (device->focus_window) wined3d_unregister_window(device->focus_window);
    InterlockedExchangePointer((void **)&device->focus_window, NULL);
}

HRESULT CDECL wined3d_device_init_3d(struct wined3d_device *device,
        struct wined3d_swapchain_desc *swapchain_desc)
{
    static const struct wined3d_color black = {0.0f, 0.0f, 0.0f, 0.0f};
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_swapchain *swapchain = NULL;
    struct wined3d_context *context;
    HRESULT hr;
    DWORD state;
    unsigned int i;

    TRACE("device %p, swapchain_desc %p.\n", device, swapchain_desc);

    if (device->d3d_initialized)
        return WINED3DERR_INVALIDCALL;
    if (!device->adapter->opengl)
        return WINED3DERR_INVALIDCALL;

    device->valid_rt_mask = 0;
    for (i = 0; i < gl_info->limits.buffers; ++i)
        device->valid_rt_mask |= (1 << i);
    device->fb.render_targets = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(*device->fb.render_targets) * gl_info->limits.buffers);

    /* Initialize the texture unit mapping to a 1:1 mapping */
    for (state = 0; state < MAX_COMBINED_SAMPLERS; ++state)
    {
        if (state < gl_info->limits.fragment_samplers)
        {
            device->texUnitMap[state] = state;
            device->rev_tex_unit_map[state] = state;
        }
        else
        {
            device->texUnitMap[state] = WINED3D_UNMAPPED_STAGE;
            device->rev_tex_unit_map[state] = WINED3D_UNMAPPED_STAGE;
        }
    }

    /* Setup the implicit swapchain. This also initializes a context. */
    TRACE("Creating implicit swapchain\n");
    hr = device->device_parent->ops->create_swapchain(device->device_parent,
            swapchain_desc, &swapchain);
    if (FAILED(hr))
    {
        WARN("Failed to create implicit swapchain\n");
        goto err_out;
    }

    device->swapchain_count = 1;
    device->swapchains = HeapAlloc(GetProcessHeap(), 0, device->swapchain_count * sizeof(*device->swapchains));
    if (!device->swapchains)
    {
        ERR("Out of memory!\n");
        goto err_out;
    }
    device->swapchains[0] = swapchain;

    if (swapchain->back_buffers && swapchain->back_buffers[0])
    {
        TRACE("Setting rendertarget to %p.\n", swapchain->back_buffers);
        device->fb.render_targets[0] = swapchain->back_buffers[0];
    }
    else
    {
        TRACE("Setting rendertarget to %p.\n", swapchain->front_buffer);
        device->fb.render_targets[0] = swapchain->front_buffer;
    }
    wined3d_surface_incref(device->fb.render_targets[0]);

    /* Depth Stencil support */
    device->fb.depth_stencil = device->auto_depth_stencil;
    if (device->fb.depth_stencil)
        wined3d_surface_incref(device->fb.depth_stencil);

    hr = device->shader_backend->shader_alloc_private(device);
    if (FAILED(hr))
    {
        TRACE("Shader private data couldn't be allocated\n");
        goto err_out;
    }
    hr = device->frag_pipe->alloc_private(device);
    if (FAILED(hr))
    {
        TRACE("Fragment pipeline private data couldn't be allocated\n");
        goto err_out;
    }
    hr = device->blitter->alloc_private(device);
    if (FAILED(hr))
    {
        TRACE("Blitter private data couldn't be allocated\n");
        goto err_out;
    }

    /* Set up some starting GL setup */

    /* Setup all the devices defaults */
    stateblock_init_default_state(device->stateBlock);

    context = context_acquire(device, swapchain->front_buffer);

    create_dummy_textures(device, context);

    ENTER_GL();

    /* Initialize the current view state */
    device->view_ident = 1;
    device->contexts[0]->last_was_rhw = 0;

    switch (wined3d_settings.offscreen_rendering_mode)
    {
        case ORM_FBO:
            device->offscreenBuffer = GL_COLOR_ATTACHMENT0;
            break;

        case ORM_BACKBUFFER:
        {
            if (context_get_current()->aux_buffers > 0)
            {
                TRACE("Using auxiliary buffer for offscreen rendering\n");
                device->offscreenBuffer = GL_AUX0;
            }
            else
            {
                TRACE("Using back buffer for offscreen rendering\n");
                device->offscreenBuffer = GL_BACK;
            }
        }
    }

    TRACE("All defaults now set up, leaving 3D init.\n");
    LEAVE_GL();

    context_release(context);

    /* Clear the screen */
    wined3d_device_clear(device, 0, NULL, WINED3DCLEAR_TARGET
            | (swapchain_desc->enable_auto_depth_stencil ? WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL : 0),
            &black, 1.0f, 0);

    device->d3d_initialized = TRUE;

    if (wined3d_settings.logo)
        device_load_logo(device, wined3d_settings.logo);
    return WINED3D_OK;

err_out:
    HeapFree(GetProcessHeap(), 0, device->fb.render_targets);
    HeapFree(GetProcessHeap(), 0, device->swapchains);
    device->swapchain_count = 0;
    if (swapchain)
        wined3d_swapchain_decref(swapchain);
    if (device->blit_priv)
        device->blitter->free_private(device);
    if (device->fragment_priv)
        device->frag_pipe->free_private(device);
    if (device->shader_priv)
        device->shader_backend->shader_free_private(device);

    return hr;
}

HRESULT CDECL wined3d_device_init_gdi(struct wined3d_device *device,
        struct wined3d_swapchain_desc *swapchain_desc)
{
    struct wined3d_swapchain *swapchain = NULL;
    HRESULT hr;

    TRACE("device %p, swapchain_desc %p.\n", device, swapchain_desc);

    /* Setup the implicit swapchain */
    TRACE("Creating implicit swapchain\n");
    hr = device->device_parent->ops->create_swapchain(device->device_parent,
            swapchain_desc, &swapchain);
    if (FAILED(hr))
    {
        WARN("Failed to create implicit swapchain\n");
        goto err_out;
    }

    device->swapchain_count = 1;
    device->swapchains = HeapAlloc(GetProcessHeap(), 0, device->swapchain_count * sizeof(*device->swapchains));
    if (!device->swapchains)
    {
        ERR("Out of memory!\n");
        goto err_out;
    }
    device->swapchains[0] = swapchain;
    return WINED3D_OK;

err_out:
    wined3d_swapchain_decref(swapchain);
    return hr;
}

HRESULT CDECL wined3d_device_uninit_3d(struct wined3d_device *device)
{
    struct wined3d_resource *resource, *cursor;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    struct wined3d_surface *surface;
    UINT i;

    TRACE("device %p.\n", device);

    if (!device->d3d_initialized)
        return WINED3DERR_INVALIDCALL;

    /* Force making the context current again, to verify it is still valid
     * (workaround for broken drivers) */
    context_set_current(NULL);
    /* I don't think that the interface guarantees that the device is destroyed from the same thread
     * it was created. Thus make sure a context is active for the glDelete* calls
     */
    context = context_acquire(device, NULL);
    gl_info = context->gl_info;

    if (device->logo_surface)
        wined3d_surface_decref(device->logo_surface);

    stateblock_unbind_resources(device->stateBlock);

    /* Unload resources */
    LIST_FOR_EACH_ENTRY_SAFE(resource, cursor, &device->resources, struct wined3d_resource, resource_list_entry)
    {
        TRACE("Unloading resource %p.\n", resource);

        resource->resource_ops->resource_unload(resource);
    }

    TRACE("Deleting high order patches\n");
    for (i = 0; i < PATCHMAP_SIZE; ++i)
    {
        struct wined3d_rect_patch *patch;
        struct list *e1, *e2;

        LIST_FOR_EACH_SAFE(e1, e2, &device->patches[i])
        {
            patch = LIST_ENTRY(e1, struct wined3d_rect_patch, entry);
            wined3d_device_delete_patch(device, patch->Handle);
        }
    }

    /* Delete the mouse cursor texture */
    if (device->cursorTexture)
    {
        ENTER_GL();
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &device->cursorTexture);
        LEAVE_GL();
        device->cursorTexture = 0;
    }

    /* Destroy the depth blt resources, they will be invalid after the reset. Also free shader
     * private data, it might contain opengl pointers
     */
    if (device->depth_blt_texture)
    {
        ENTER_GL();
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &device->depth_blt_texture);
        LEAVE_GL();
        device->depth_blt_texture = 0;
    }

    /* Destroy the shader backend. Note that this has to happen after all shaders are destroyed. */
    device->blitter->free_private(device);
    device->frag_pipe->free_private(device);
    device->shader_backend->shader_free_private(device);

    /* Release the buffers (with sanity checks)*/
    if (device->onscreen_depth_stencil)
    {
        surface = device->onscreen_depth_stencil;
        device->onscreen_depth_stencil = NULL;
        wined3d_surface_decref(surface);
    }

    if (device->fb.depth_stencil)
    {
        surface = device->fb.depth_stencil;

        TRACE("Releasing depth/stencil buffer %p.\n", surface);

        device->fb.depth_stencil = NULL;
        wined3d_surface_decref(surface);
    }

    if (device->auto_depth_stencil)
    {
        surface = device->auto_depth_stencil;
        device->auto_depth_stencil = NULL;
        if (wined3d_surface_decref(surface))
            FIXME("Something's still holding the auto depth stencil buffer (%p).\n", surface);
    }

    for (i = 1; i < gl_info->limits.buffers; ++i)
    {
        wined3d_device_set_render_target(device, i, NULL, FALSE);
    }

    surface = device->fb.render_targets[0];
    TRACE("Setting rendertarget 0 to NULL\n");
    device->fb.render_targets[0] = NULL;
    TRACE("Releasing the render target at %p\n", surface);
    wined3d_surface_decref(surface);

    context_release(context);

    for (i = 0; i < device->swapchain_count; ++i)
    {
        TRACE("Releasing the implicit swapchain %u.\n", i);
        if (wined3d_swapchain_decref(device->swapchains[i]))
            FIXME("Something's still holding the implicit swapchain.\n");
    }

    HeapFree(GetProcessHeap(), 0, device->swapchains);
    device->swapchains = NULL;
    device->swapchain_count = 0;

    HeapFree(GetProcessHeap(), 0, device->fb.render_targets);
    device->fb.render_targets = NULL;

    device->d3d_initialized = FALSE;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_uninit_gdi(struct wined3d_device *device)
{
    unsigned int i;

    for (i = 0; i < device->swapchain_count; ++i)
    {
        TRACE("Releasing the implicit swapchain %u.\n", i);
        if (wined3d_swapchain_decref(device->swapchains[i]))
            FIXME("Something's still holding the implicit swapchain.\n");
    }

    HeapFree(GetProcessHeap(), 0, device->swapchains);
    device->swapchains = NULL;
    device->swapchain_count = 0;
    return WINED3D_OK;
}

/* Enables thread safety in the wined3d device and its resources. Called by DirectDraw
 * from SetCooperativeLevel if DDSCL_MULTITHREADED is specified, and by d3d8/9 from
 * CreateDevice if D3DCREATE_MULTITHREADED is passed.
 *
 * There is no way to deactivate thread safety once it is enabled.
 */
void CDECL wined3d_device_set_multithreaded(struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    /* For now just store the flag (needed in case of ddraw). */
    device->create_parms.flags |= WINED3DCREATE_MULTITHREADED;
}

HRESULT CDECL wined3d_device_get_wined3d(const struct wined3d_device *device, struct wined3d **wined3d)
{
    TRACE("device %p, wined3d %p.\n", device, wined3d);

    *wined3d = device->wined3d;
    wined3d_incref(*wined3d);

    TRACE("Returning %p.\n", *wined3d);

    return WINED3D_OK;
}

UINT CDECL wined3d_device_get_available_texture_mem(const struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    TRACE("Emulating %d MB, returning %d MB left.\n",
            device->adapter->TextureRam / (1024 * 1024),
            (device->adapter->TextureRam - device->adapter->UsedTextureRam) / (1024 * 1024));

    return device->adapter->TextureRam - device->adapter->UsedTextureRam;
}

HRESULT CDECL wined3d_device_set_stream_source(struct wined3d_device *device, UINT stream_idx,
        struct wined3d_buffer *buffer, UINT offset, UINT stride)
{
    struct wined3d_stream_state *stream;
    struct wined3d_buffer *prev_buffer;

    TRACE("device %p, stream_idx %u, buffer %p, offset %u, stride %u.\n",
            device, stream_idx, buffer, offset, stride);

    if (stream_idx >= MAX_STREAMS)
    {
        WARN("Stream index %u out of range.\n", stream_idx);
        return WINED3DERR_INVALIDCALL;
    }
    else if (offset & 0x3)
    {
        WARN("Offset %u is not 4 byte aligned.\n", offset);
        return WINED3DERR_INVALIDCALL;
    }

    stream = &device->updateStateBlock->state.streams[stream_idx];
    prev_buffer = stream->buffer;

    device->updateStateBlock->changed.streamSource |= 1 << stream_idx;

    if (prev_buffer == buffer
            && stream->stride == stride
            && stream->offset == offset)
    {
       TRACE("Application is setting the old values over, nothing to do.\n");
       return WINED3D_OK;
    }

    stream->buffer = buffer;
    if (buffer)
    {
        stream->stride = stride;
        stream->offset = offset;
    }

    /* Handle recording of state blocks. */
    if (device->isRecordingState)
    {
        TRACE("Recording... not performing anything.\n");
        if (buffer)
            wined3d_buffer_incref(buffer);
        if (prev_buffer)
            wined3d_buffer_decref(prev_buffer);
        return WINED3D_OK;
    }

    if (buffer)
    {
        InterlockedIncrement(&buffer->resource.bind_count);
        wined3d_buffer_incref(buffer);
    }
    if (prev_buffer)
    {
        InterlockedDecrement(&prev_buffer->resource.bind_count);
        wined3d_buffer_decref(prev_buffer);
    }

    device_invalidate_state(device, STATE_STREAMSRC);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_stream_source(const struct wined3d_device *device,
        UINT stream_idx, struct wined3d_buffer **buffer, UINT *offset, UINT *stride)
{
    struct wined3d_stream_state *stream;

    TRACE("device %p, stream_idx %u, buffer %p, offset %p, stride %p.\n",
            device, stream_idx, buffer, offset, stride);

    if (stream_idx >= MAX_STREAMS)
    {
        WARN("Stream index %u out of range.\n", stream_idx);
        return WINED3DERR_INVALIDCALL;
    }

    stream = &device->stateBlock->state.streams[stream_idx];
    *buffer = stream->buffer;
    if (*buffer)
        wined3d_buffer_incref(*buffer);
    if (offset)
        *offset = stream->offset;
    *stride = stream->stride;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_stream_source_freq(struct wined3d_device *device, UINT stream_idx, UINT divider)
{
    struct wined3d_stream_state *stream;
    UINT old_flags, old_freq;

    TRACE("device %p, stream_idx %u, divider %#x.\n", device, stream_idx, divider);

    /* Verify input. At least in d3d9 this is invalid. */
    if ((divider & WINED3DSTREAMSOURCE_INSTANCEDATA) && (divider & WINED3DSTREAMSOURCE_INDEXEDDATA))
    {
        WARN("INSTANCEDATA and INDEXEDDATA were set, returning D3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }
    if ((divider & WINED3DSTREAMSOURCE_INSTANCEDATA) && !stream_idx)
    {
        WARN("INSTANCEDATA used on stream 0, returning D3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }
    if (!divider)
    {
        WARN("Divider is 0, returning D3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    stream = &device->updateStateBlock->state.streams[stream_idx];
    old_flags = stream->flags;
    old_freq = stream->frequency;

    stream->flags = divider & (WINED3DSTREAMSOURCE_INSTANCEDATA | WINED3DSTREAMSOURCE_INDEXEDDATA);
    stream->frequency = divider & 0x7fffff;

    device->updateStateBlock->changed.streamFreq |= 1 << stream_idx;

    if (stream->frequency != old_freq || stream->flags != old_flags)
        device_invalidate_state(device, STATE_STREAMSRC);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_stream_source_freq(const struct wined3d_device *device,
        UINT stream_idx, UINT *divider)
{
    struct wined3d_stream_state *stream;

    TRACE("device %p, stream_idx %u, divider %p.\n", device, stream_idx, divider);

    stream = &device->updateStateBlock->state.streams[stream_idx];
    *divider = stream->flags | stream->frequency;

    TRACE("Returning %#x.\n", *divider);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_transform(struct wined3d_device *device,
        enum wined3d_transform_state d3dts, const struct wined3d_matrix *matrix)
{
    TRACE("device %p, state %s, matrix %p.\n",
            device, debug_d3dtstype(d3dts), matrix);
    TRACE("%.8e %.8e %.8e %.8e\n", matrix->u.s._11, matrix->u.s._12, matrix->u.s._13, matrix->u.s._14);
    TRACE("%.8e %.8e %.8e %.8e\n", matrix->u.s._21, matrix->u.s._22, matrix->u.s._23, matrix->u.s._24);
    TRACE("%.8e %.8e %.8e %.8e\n", matrix->u.s._31, matrix->u.s._32, matrix->u.s._33, matrix->u.s._34);
    TRACE("%.8e %.8e %.8e %.8e\n", matrix->u.s._41, matrix->u.s._42, matrix->u.s._43, matrix->u.s._44);

    /* Handle recording of state blocks. */
    if (device->isRecordingState)
    {
        TRACE("Recording... not performing anything.\n");
        device->updateStateBlock->changed.transform[d3dts >> 5] |= 1 << (d3dts & 0x1f);
        device->updateStateBlock->state.transforms[d3dts] = *matrix;
        return WINED3D_OK;
    }

    /* If the new matrix is the same as the current one,
     * we cut off any further processing. this seems to be a reasonable
     * optimization because as was noticed, some apps (warcraft3 for example)
     * tend towards setting the same matrix repeatedly for some reason.
     *
     * From here on we assume that the new matrix is different, wherever it matters. */
    if (!memcmp(&device->stateBlock->state.transforms[d3dts].u.m[0][0], matrix, sizeof(*matrix)))
    {
        TRACE("The application is setting the same matrix over again.\n");
        return WINED3D_OK;
    }

    device->stateBlock->state.transforms[d3dts] = *matrix;
    if (d3dts == WINED3D_TS_VIEW)
        device->view_ident = !memcmp(matrix, &identity, sizeof(identity));

    if (d3dts < WINED3D_TS_WORLD_MATRIX(device->adapter->gl_info.limits.blends))
        device_invalidate_state(device, STATE_TRANSFORM(d3dts));

    return WINED3D_OK;

}

HRESULT CDECL wined3d_device_get_transform(const struct wined3d_device *device,
        enum wined3d_transform_state state, struct wined3d_matrix *matrix)
{
    TRACE("device %p, state %s, matrix %p.\n", device, debug_d3dtstype(state), matrix);

    *matrix = device->stateBlock->state.transforms[state];

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_multiply_transform(struct wined3d_device *device,
        enum wined3d_transform_state state, const struct wined3d_matrix *matrix)
{
    const struct wined3d_matrix *mat = NULL;
    struct wined3d_matrix temp;

    TRACE("device %p, state %s, matrix %p.\n", device, debug_d3dtstype(state), matrix);

    /* Note: Using 'updateStateBlock' rather than 'stateblock' in the code
     * below means it will be recorded in a state block change, but it
     * works regardless where it is recorded.
     * If this is found to be wrong, change to StateBlock. */
    if (state > HIGHEST_TRANSFORMSTATE)
    {
        WARN("Unhandled transform state %#x.\n", state);
        return WINED3D_OK;
    }

    mat = &device->updateStateBlock->state.transforms[state];
    multiply_matrix(&temp, mat, matrix);

    /* Apply change via set transform - will reapply to eg. lights this way. */
    return wined3d_device_set_transform(device, state, &temp);
}

/* Note lights are real special cases. Although the device caps state only
 * e.g. 8 are supported, you can reference any indexes you want as long as
 * that number max are enabled at any one point in time. Therefore since the
 * indices can be anything, we need a hashmap of them. However, this causes
 * stateblock problems. When capturing the state block, I duplicate the
 * hashmap, but when recording, just build a chain pretty much of commands to
 * be replayed. */
HRESULT CDECL wined3d_device_set_light(struct wined3d_device *device,
        UINT light_idx, const struct wined3d_light *light)
{
    UINT hash_idx = LIGHTMAP_HASHFUNC(light_idx);
    struct wined3d_light_info *object = NULL;
    struct list *e;
    float rho;

    TRACE("device %p, light_idx %u, light %p.\n", device, light_idx, light);

    /* Check the parameter range. Need for speed most wanted sets junk lights
     * which confuse the GL driver. */
    if (!light)
        return WINED3DERR_INVALIDCALL;

    switch (light->type)
    {
        case WINED3D_LIGHT_POINT:
        case WINED3D_LIGHT_SPOT:
        case WINED3D_LIGHT_PARALLELPOINT:
        case WINED3D_LIGHT_GLSPOT:
            /* Incorrect attenuation values can cause the gl driver to crash.
             * Happens with Need for speed most wanted. */
            if (light->attenuation0 < 0.0f || light->attenuation1 < 0.0f || light->attenuation2 < 0.0f)
            {
                WARN("Attenuation is negative, returning WINED3DERR_INVALIDCALL.\n");
                return WINED3DERR_INVALIDCALL;
            }
            break;

        case WINED3D_LIGHT_DIRECTIONAL:
            /* Ignores attenuation */
            break;

        default:
        WARN("Light type out of range, returning WINED3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }

    LIST_FOR_EACH(e, &device->updateStateBlock->state.light_map[hash_idx])
    {
        object = LIST_ENTRY(e, struct wined3d_light_info, entry);
        if (object->OriginalIndex == light_idx)
            break;
        object = NULL;
    }

    if (!object)
    {
        TRACE("Adding new light\n");
        object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
        if (!object)
        {
            ERR("Out of memory error when allocating a light\n");
            return E_OUTOFMEMORY;
        }
        list_add_head(&device->updateStateBlock->state.light_map[hash_idx], &object->entry);
        object->glIndex = -1;
        object->OriginalIndex = light_idx;
    }

    /* Initialize the object. */
    TRACE("Light %d setting to type %d, Diffuse(%f,%f,%f,%f), Specular(%f,%f,%f,%f), Ambient(%f,%f,%f,%f)\n",
            light_idx, light->type,
            light->diffuse.r, light->diffuse.g, light->diffuse.b, light->diffuse.a,
            light->specular.r, light->specular.g, light->specular.b, light->specular.a,
            light->ambient.r, light->ambient.g, light->ambient.b, light->ambient.a);
    TRACE("... Pos(%f,%f,%f), Dir(%f,%f,%f)\n", light->position.x, light->position.y, light->position.z,
            light->direction.x, light->direction.y, light->direction.z);
    TRACE("... Range(%f), Falloff(%f), Theta(%f), Phi(%f)\n",
            light->range, light->falloff, light->theta, light->phi);

    /* Save away the information. */
    object->OriginalParms = *light;

    switch (light->type)
    {
        case WINED3D_LIGHT_POINT:
            /* Position */
            object->lightPosn[0] = light->position.x;
            object->lightPosn[1] = light->position.y;
            object->lightPosn[2] = light->position.z;
            object->lightPosn[3] = 1.0f;
            object->cutoff = 180.0f;
            /* FIXME: Range */
            break;

        case WINED3D_LIGHT_DIRECTIONAL:
            /* Direction */
            object->lightPosn[0] = -light->direction.x;
            object->lightPosn[1] = -light->direction.y;
            object->lightPosn[2] = -light->direction.z;
            object->lightPosn[3] = 0.0f;
            object->exponent = 0.0f;
            object->cutoff = 180.0f;
            break;

        case WINED3D_LIGHT_SPOT:
            /* Position */
            object->lightPosn[0] = light->position.x;
            object->lightPosn[1] = light->position.y;
            object->lightPosn[2] = light->position.z;
            object->lightPosn[3] = 1.0f;

            /* Direction */
            object->lightDirn[0] = light->direction.x;
            object->lightDirn[1] = light->direction.y;
            object->lightDirn[2] = light->direction.z;
            object->lightDirn[3] = 1.0f;

            /* opengl-ish and d3d-ish spot lights use too different models
             * for the light "intensity" as a function of the angle towards
             * the main light direction, so we only can approximate very
             * roughly. However, spot lights are rather rarely used in games
             * (if ever used at all). Furthermore if still used, probably
             * nobody pays attention to such details. */
            if (!light->falloff)
            {
                /* Falloff = 0 is easy, because d3d's and opengl's spot light
                 * equations have the falloff resp. exponent parameter as an
                 * exponent, so the spot light lighting will always be 1.0 for
                 * both of them, and we don't have to care for the rest of the
                 * rather complex calculation. */
                object->exponent = 0.0f;
            }
            else
            {
                rho = light->theta + (light->phi - light->theta) / (2 * light->falloff);
                if (rho < 0.0001f)
                    rho = 0.0001f;
                object->exponent = -0.3f / logf(cosf(rho / 2));
            }

            if (object->exponent > 128.0f)
                object->exponent = 128.0f;

            object->cutoff = (float)(light->phi * 90 / M_PI);
            /* FIXME: Range */
            break;

        default:
            FIXME("Unrecognized light type %#x.\n", light->type);
    }

    /* Update the live definitions if the light is currently assigned a glIndex. */
    if (object->glIndex != -1 && !device->isRecordingState)
        device_invalidate_state(device, STATE_ACTIVELIGHT(object->glIndex));

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_light(const struct wined3d_device *device,
        UINT light_idx, struct wined3d_light *light)
{
    UINT hash_idx = LIGHTMAP_HASHFUNC(light_idx);
    struct wined3d_light_info *light_info = NULL;
    struct list *e;

    TRACE("device %p, light_idx %u, light %p.\n", device, light_idx, light);

    LIST_FOR_EACH(e, &device->stateBlock->state.light_map[hash_idx])
    {
        light_info = LIST_ENTRY(e, struct wined3d_light_info, entry);
        if (light_info->OriginalIndex == light_idx)
            break;
        light_info = NULL;
    }

    if (!light_info)
    {
        TRACE("Light information requested but light not defined\n");
        return WINED3DERR_INVALIDCALL;
    }

    *light = light_info->OriginalParms;
    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_light_enable(struct wined3d_device *device, UINT light_idx, BOOL enable)
{
    UINT hash_idx = LIGHTMAP_HASHFUNC(light_idx);
    struct wined3d_light_info *light_info = NULL;
    struct list *e;

    TRACE("device %p, light_idx %u, enable %#x.\n", device, light_idx, enable);

    LIST_FOR_EACH(e, &device->updateStateBlock->state.light_map[hash_idx])
    {
        light_info = LIST_ENTRY(e, struct wined3d_light_info, entry);
        if (light_info->OriginalIndex == light_idx)
            break;
        light_info = NULL;
    }
    TRACE("Found light %p.\n", light_info);

    /* Special case - enabling an undefined light creates one with a strict set of parameters. */
    if (!light_info)
    {
        TRACE("Light enabled requested but light not defined, so defining one!\n");
        wined3d_device_set_light(device, light_idx, &WINED3D_default_light);

        /* Search for it again! Should be fairly quick as near head of list. */
        LIST_FOR_EACH(e, &device->updateStateBlock->state.light_map[hash_idx])
        {
            light_info = LIST_ENTRY(e, struct wined3d_light_info, entry);
            if (light_info->OriginalIndex == light_idx)
                break;
            light_info = NULL;
        }
        if (!light_info)
        {
            FIXME("Adding default lights has failed dismally\n");
            return WINED3DERR_INVALIDCALL;
        }
    }

    if (!enable)
    {
        if (light_info->glIndex != -1)
        {
            if (!device->isRecordingState)
                device_invalidate_state(device, STATE_ACTIVELIGHT(light_info->glIndex));

            device->updateStateBlock->state.lights[light_info->glIndex] = NULL;
            light_info->glIndex = -1;
        }
        else
        {
            TRACE("Light already disabled, nothing to do\n");
        }
        light_info->enabled = FALSE;
    }
    else
    {
        light_info->enabled = TRUE;
        if (light_info->glIndex != -1)
        {
            TRACE("Nothing to do as light was enabled\n");
        }
        else
        {
            unsigned int i;
            const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
            /* Find a free GL light. */
            for (i = 0; i < gl_info->limits.lights; ++i)
            {
                if (!device->updateStateBlock->state.lights[i])
                {
                    device->updateStateBlock->state.lights[i] = light_info;
                    light_info->glIndex = i;
                    break;
                }
            }
            if (light_info->glIndex == -1)
            {
                /* Our tests show that Windows returns D3D_OK in this situation, even with
                 * D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE devices. This
                 * is consistent among ddraw, d3d8 and d3d9. GetLightEnable returns TRUE
                 * as well for those lights.
                 *
                 * TODO: Test how this affects rendering. */
                WARN("Too many concurrently active lights\n");
                return WINED3D_OK;
            }

            /* i == light_info->glIndex */
            if (!device->isRecordingState)
                device_invalidate_state(device, STATE_ACTIVELIGHT(i));
        }
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_light_enable(const struct wined3d_device *device, UINT light_idx, BOOL *enable)
{
    UINT hash_idx = LIGHTMAP_HASHFUNC(light_idx);
    struct wined3d_light_info *light_info = NULL;
    struct list *e;

    TRACE("device %p, light_idx %u, enable %p.\n", device, light_idx, enable);

    LIST_FOR_EACH(e, &device->stateBlock->state.light_map[hash_idx])
    {
        light_info = LIST_ENTRY(e, struct wined3d_light_info, entry);
        if (light_info->OriginalIndex == light_idx)
            break;
        light_info = NULL;
    }

    if (!light_info)
    {
        TRACE("Light enabled state requested but light not defined.\n");
        return WINED3DERR_INVALIDCALL;
    }
    /* true is 128 according to SetLightEnable */
    *enable = light_info->enabled ? 128 : 0;
    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_clip_plane(struct wined3d_device *device,
        UINT plane_idx, const struct wined3d_vec4 *plane)
{
    TRACE("device %p, plane_idx %u, plane %p.\n", device, plane_idx, plane);

    /* Validate plane_idx. */
    if (plane_idx >= device->adapter->gl_info.limits.clipplanes)
    {
        TRACE("Application has requested clipplane this device doesn't support.\n");
        return WINED3DERR_INVALIDCALL;
    }

    device->updateStateBlock->changed.clipplane |= 1 << plane_idx;

    if (!memcmp(&device->updateStateBlock->state.clip_planes[plane_idx], plane, sizeof(*plane)))
    {
        TRACE("Application is setting old values over, nothing to do.\n");
        return WINED3D_OK;
    }

    device->updateStateBlock->state.clip_planes[plane_idx] = *plane;

    /* Handle recording of state blocks. */
    if (device->isRecordingState)
    {
        TRACE("Recording... not performing anything.\n");
        return WINED3D_OK;
    }

    device_invalidate_state(device, STATE_CLIPPLANE(plane_idx));

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_clip_plane(const struct wined3d_device *device,
        UINT plane_idx, struct wined3d_vec4 *plane)
{
    TRACE("device %p, plane_idx %u, plane %p.\n", device, plane_idx, plane);

    /* Validate plane_idx. */
    if (plane_idx >= device->adapter->gl_info.limits.clipplanes)
    {
        TRACE("Application has requested clipplane this device doesn't support.\n");
        return WINED3DERR_INVALIDCALL;
    }

    *plane = device->stateBlock->state.clip_planes[plane_idx];

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_clip_status(struct wined3d_device *device,
        const struct wined3d_clip_status *clip_status)
{
    FIXME("device %p, clip_status %p stub!\n", device, clip_status);

    if (!clip_status)
        return WINED3DERR_INVALIDCALL;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_clip_status(const struct wined3d_device *device,
        struct wined3d_clip_status *clip_status)
{
    FIXME("device %p, clip_status %p stub!\n", device, clip_status);

    if (!clip_status)
        return WINED3DERR_INVALIDCALL;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_material(struct wined3d_device *device, const struct wined3d_material *material)
{
    TRACE("device %p, material %p.\n", device, material);

    device->updateStateBlock->changed.material = TRUE;
    device->updateStateBlock->state.material = *material;

    /* Handle recording of state blocks */
    if (device->isRecordingState)
    {
        TRACE("Recording... not performing anything.\n");
        return WINED3D_OK;
    }

    device_invalidate_state(device, STATE_MATERIAL);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_material(const struct wined3d_device *device, struct wined3d_material *material)
{
    TRACE("device %p, material %p.\n", device, material);

    *material = device->updateStateBlock->state.material;

    TRACE("diffuse {%.8e, %.8e, %.8e, %.8e}\n",
            material->diffuse.r, material->diffuse.g,
            material->diffuse.b, material->diffuse.a);
    TRACE("ambient {%.8e, %.8e, %.8e, %.8e}\n",
            material->ambient.r, material->ambient.g,
            material->ambient.b, material->ambient.a);
    TRACE("specular {%.8e, %.8e, %.8e, %.8e}\n",
            material->specular.r, material->specular.g,
            material->specular.b, material->specular.a);
    TRACE("emissive {%.8e, %.8e, %.8e, %.8e}\n",
            material->emissive.r, material->emissive.g,
            material->emissive.b, material->emissive.a);
    TRACE("power %.8e.\n", material->power);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_index_buffer(struct wined3d_device *device,
        struct wined3d_buffer *buffer, enum wined3d_format_id format_id)
{
    struct wined3d_buffer *prev_buffer;

    TRACE("device %p, buffer %p, format %s.\n",
            device, buffer, debug_d3dformat(format_id));

    prev_buffer = device->updateStateBlock->state.index_buffer;

    device->updateStateBlock->changed.indices = TRUE;
    device->updateStateBlock->state.index_buffer = buffer;
    device->updateStateBlock->state.index_format = format_id;

    /* Handle recording of state blocks. */
    if (device->isRecordingState)
    {
        TRACE("Recording... not performing anything.\n");
        if (buffer)
            wined3d_buffer_incref(buffer);
        if (prev_buffer)
            wined3d_buffer_decref(prev_buffer);
        return WINED3D_OK;
    }

    if (prev_buffer != buffer)
    {
        device_invalidate_state(device, STATE_INDEXBUFFER);
        if (buffer)
        {
            InterlockedIncrement(&buffer->resource.bind_count);
            wined3d_buffer_incref(buffer);
        }
        if (prev_buffer)
        {
            InterlockedDecrement(&prev_buffer->resource.bind_count);
            wined3d_buffer_decref(prev_buffer);
        }
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_index_buffer(const struct wined3d_device *device, struct wined3d_buffer **buffer)
{
    TRACE("device %p, buffer %p.\n", device, buffer);

    *buffer = device->stateBlock->state.index_buffer;

    if (*buffer)
        wined3d_buffer_incref(*buffer);

    TRACE("Returning %p.\n", *buffer);

    return WINED3D_OK;
}

/* Method to offer d3d9 a simple way to set the base vertex index without messing with the index buffer */
HRESULT CDECL wined3d_device_set_base_vertex_index(struct wined3d_device *device, INT base_index)
{
    TRACE("device %p, base_index %d.\n", device, base_index);

    if (device->updateStateBlock->state.base_vertex_index == base_index)
    {
        TRACE("Application is setting the old value over, nothing to do\n");
        return WINED3D_OK;
    }

    device->updateStateBlock->state.base_vertex_index = base_index;

    if (device->isRecordingState)
    {
        TRACE("Recording... not performing anything\n");
        return WINED3D_OK;
    }
    return WINED3D_OK;
}

INT CDECL wined3d_device_get_base_vertex_index(const struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    return device->stateBlock->state.base_vertex_index;
}

HRESULT CDECL wined3d_device_set_viewport(struct wined3d_device *device, const struct wined3d_viewport *viewport)
{
    TRACE("device %p, viewport %p.\n", device, viewport);
    TRACE("x %u, y %u, w %u, h %u, min_z %.8e, max_z %.8e.\n",
          viewport->x, viewport->y, viewport->width, viewport->height, viewport->min_z, viewport->max_z);

    device->updateStateBlock->changed.viewport = TRUE;
    device->updateStateBlock->state.viewport = *viewport;

    /* Handle recording of state blocks */
    if (device->isRecordingState)
    {
        TRACE("Recording... not performing anything\n");
        return WINED3D_OK;
    }

    device_invalidate_state(device, STATE_VIEWPORT);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_viewport(const struct wined3d_device *device, struct wined3d_viewport *viewport)
{
    TRACE("device %p, viewport %p.\n", device, viewport);

    *viewport = device->stateBlock->state.viewport;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_render_state(struct wined3d_device *device,
        enum wined3d_render_state state, DWORD value)
{
    DWORD old_value = device->stateBlock->state.render_states[state];

    TRACE("device %p, state %s (%#x), value %#x.\n", device, debug_d3drenderstate(state), state, value);

    device->updateStateBlock->changed.renderState[state >> 5] |= 1 << (state & 0x1f);
    device->updateStateBlock->state.render_states[state] = value;

    /* Handle recording of state blocks. */
    if (device->isRecordingState)
    {
        TRACE("Recording... not performing anything.\n");
        return WINED3D_OK;
    }

    /* Compared here and not before the assignment to allow proper stateblock recording. */
    if (value == old_value)
        TRACE("Application is setting the old value over, nothing to do.\n");
    else
        device_invalidate_state(device, STATE_RENDER(state));

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_render_state(const struct wined3d_device *device,
        enum wined3d_render_state state, DWORD *value)
{
    TRACE("device %p, state %s (%#x), value %p.\n", device, debug_d3drenderstate(state), state, value);

    *value = device->stateBlock->state.render_states[state];

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_sampler_state(struct wined3d_device *device,
        UINT sampler_idx, enum wined3d_sampler_state state, DWORD value)
{
    DWORD old_value;

    TRACE("device %p, sampler_idx %u, state %s, value %#x.\n",
            device, sampler_idx, debug_d3dsamplerstate(state), value);

    if (sampler_idx >= WINED3DVERTEXTEXTURESAMPLER0 && sampler_idx <= WINED3DVERTEXTEXTURESAMPLER3)
        sampler_idx -= (WINED3DVERTEXTEXTURESAMPLER0 - MAX_FRAGMENT_SAMPLERS);

    if (sampler_idx >= sizeof(device->stateBlock->state.sampler_states)
            / sizeof(*device->stateBlock->state.sampler_states))
    {
        WARN("Invalid sampler %u.\n", sampler_idx);
        return WINED3D_OK; /* Windows accepts overflowing this array ... we do not. */
    }

    old_value = device->stateBlock->state.sampler_states[sampler_idx][state];
    device->updateStateBlock->state.sampler_states[sampler_idx][state] = value;
    device->updateStateBlock->changed.samplerState[sampler_idx] |= 1 << state;

    /* Handle recording of state blocks. */
    if (device->isRecordingState)
    {
        TRACE("Recording... not performing anything.\n");
        return WINED3D_OK;
    }

    if (old_value == value)
    {
        TRACE("Application is setting the old value over, nothing to do.\n");
        return WINED3D_OK;
    }

    device_invalidate_state(device, STATE_SAMPLER(sampler_idx));

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_sampler_state(const struct wined3d_device *device,
        UINT sampler_idx, enum wined3d_sampler_state state, DWORD *value)
{
    TRACE("device %p, sampler_idx %u, state %s, value %p.\n",
            device, sampler_idx, debug_d3dsamplerstate(state), value);

    if (sampler_idx >= WINED3DVERTEXTEXTURESAMPLER0 && sampler_idx <= WINED3DVERTEXTEXTURESAMPLER3)
        sampler_idx -= (WINED3DVERTEXTEXTURESAMPLER0 - MAX_FRAGMENT_SAMPLERS);

    if (sampler_idx >= sizeof(device->stateBlock->state.sampler_states)
            / sizeof(*device->stateBlock->state.sampler_states))
    {
        WARN("Invalid sampler %u.\n", sampler_idx);
        return WINED3D_OK; /* Windows accepts overflowing this array ... we do not. */
    }

    *value = device->stateBlock->state.sampler_states[sampler_idx][state];
    TRACE("Returning %#x.\n", *value);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_scissor_rect(struct wined3d_device *device, const RECT *rect)
{
    TRACE("device %p, rect %s.\n", device, wine_dbgstr_rect(rect));

    device->updateStateBlock->changed.scissorRect = TRUE;
    if (EqualRect(&device->updateStateBlock->state.scissor_rect, rect))
    {
        TRACE("App is setting the old scissor rectangle over, nothing to do.\n");
        return WINED3D_OK;
    }
    CopyRect(&device->updateStateBlock->state.scissor_rect, rect);

    if (device->isRecordingState)
    {
        TRACE("Recording... not performing anything.\n");
        return WINED3D_OK;
    }

    device_invalidate_state(device, STATE_SCISSORRECT);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_scissor_rect(const struct wined3d_device *device, RECT *rect)
{
    TRACE("device %p, rect %p.\n", device, rect);

    *rect = device->updateStateBlock->state.scissor_rect;
    TRACE("Returning rect %s.\n", wine_dbgstr_rect(rect));

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_vertex_declaration(struct wined3d_device *device,
        struct wined3d_vertex_declaration *declaration)
{
    struct wined3d_vertex_declaration *prev = device->updateStateBlock->state.vertex_declaration;

    TRACE("device %p, declaration %p.\n", device, declaration);

    if (declaration)
        wined3d_vertex_declaration_incref(declaration);
    if (prev)
        wined3d_vertex_declaration_decref(prev);

    device->updateStateBlock->state.vertex_declaration = declaration;
    device->updateStateBlock->changed.vertexDecl = TRUE;

    if (device->isRecordingState)
    {
        TRACE("Recording... not performing anything.\n");
        return WINED3D_OK;
    }
    else if (declaration == prev)
    {
        /* Checked after the assignment to allow proper stateblock recording. */
        TRACE("Application is setting the old declaration over, nothing to do.\n");
        return WINED3D_OK;
    }

    device_invalidate_state(device, STATE_VDECL);
    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_vertex_declaration(const struct wined3d_device *device,
        struct wined3d_vertex_declaration **declaration)
{
    TRACE("device %p, declaration %p.\n", device, declaration);

    *declaration = device->stateBlock->state.vertex_declaration;
    if (*declaration)
        wined3d_vertex_declaration_incref(*declaration);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_vertex_shader(struct wined3d_device *device, struct wined3d_shader *shader)
{
    struct wined3d_shader *prev = device->updateStateBlock->state.vertex_shader;

    TRACE("device %p, shader %p.\n", device, shader);

    device->updateStateBlock->state.vertex_shader = shader;
    device->updateStateBlock->changed.vertexShader = TRUE;

    if (device->isRecordingState)
    {
        if (shader)
            wined3d_shader_incref(shader);
        if (prev)
            wined3d_shader_decref(prev);
        TRACE("Recording... not performing anything.\n");
        return WINED3D_OK;
    }

    if (shader == prev)
    {
        TRACE("Application is setting the old shader over, nothing to do.\n");
        return WINED3D_OK;
    }

    if (shader)
        wined3d_shader_incref(shader);
    if (prev)
        wined3d_shader_decref(prev);

    device_invalidate_state(device, STATE_VSHADER);

    return WINED3D_OK;
}

struct wined3d_shader * CDECL wined3d_device_get_vertex_shader(const struct wined3d_device *device)
{
    struct wined3d_shader *shader;

    TRACE("device %p.\n", device);

    shader = device->stateBlock->state.vertex_shader;
    if (shader)
        wined3d_shader_incref(shader);

    TRACE("Returning %p.\n", shader);
    return shader;
}

HRESULT CDECL wined3d_device_set_vs_consts_b(struct wined3d_device *device,
        UINT start_register, const BOOL *constants, UINT bool_count)
{
    UINT count = min(bool_count, MAX_CONST_B - start_register);
    UINT i;

    TRACE("device %p, start_register %u, constants %p, bool_count %u.\n",
            device, start_register, constants, bool_count);

    if (!constants || start_register >= MAX_CONST_B)
        return WINED3DERR_INVALIDCALL;

    memcpy(&device->updateStateBlock->state.vs_consts_b[start_register], constants, count * sizeof(BOOL));
    for (i = 0; i < count; ++i)
        TRACE("Set BOOL constant %u to %s.\n", start_register + i, constants[i] ? "true" : "false");

    for (i = start_register; i < count + start_register; ++i)
        device->updateStateBlock->changed.vertexShaderConstantsB |= (1 << i);

    if (!device->isRecordingState)
        device_invalidate_state(device, STATE_VERTEXSHADERCONSTANT);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_vs_consts_b(const struct wined3d_device *device,
        UINT start_register, BOOL *constants, UINT bool_count)
{
    UINT count = min(bool_count, MAX_CONST_B - start_register);

    TRACE("device %p, start_register %u, constants %p, bool_count %u.\n",
            device, start_register, constants, bool_count);

    if (!constants || start_register >= MAX_CONST_B)
        return WINED3DERR_INVALIDCALL;

    memcpy(constants, &device->stateBlock->state.vs_consts_b[start_register], count * sizeof(BOOL));

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_vs_consts_i(struct wined3d_device *device,
        UINT start_register, const int *constants, UINT vector4i_count)
{
    UINT count = min(vector4i_count, MAX_CONST_I - start_register);
    UINT i;

    TRACE("device %p, start_register %u, constants %p, vector4i_count %u.\n",
            device, start_register, constants, vector4i_count);

    if (!constants || start_register >= MAX_CONST_I)
        return WINED3DERR_INVALIDCALL;

    memcpy(&device->updateStateBlock->state.vs_consts_i[start_register * 4], constants, count * sizeof(int) * 4);
    for (i = 0; i < count; ++i)
        TRACE("Set INT constant %u to {%d, %d, %d, %d}.\n", start_register + i,
                constants[i * 4], constants[i * 4 + 1],
                constants[i * 4 + 2], constants[i * 4 + 3]);

    for (i = start_register; i < count + start_register; ++i)
        device->updateStateBlock->changed.vertexShaderConstantsI |= (1 << i);

    if (!device->isRecordingState)
        device_invalidate_state(device, STATE_VERTEXSHADERCONSTANT);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_vs_consts_i(const struct wined3d_device *device,
        UINT start_register, int *constants, UINT vector4i_count)
{
    UINT count = min(vector4i_count, MAX_CONST_I - start_register);

    TRACE("device %p, start_register %u, constants %p, vector4i_count %u.\n",
            device, start_register, constants, vector4i_count);

    if (!constants || start_register >= MAX_CONST_I)
        return WINED3DERR_INVALIDCALL;

    memcpy(constants, &device->stateBlock->state.vs_consts_i[start_register * 4], count * sizeof(int) * 4);
    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_vs_consts_f(struct wined3d_device *device,
        UINT start_register, const float *constants, UINT vector4f_count)
{
    UINT i;

    TRACE("device %p, start_register %u, constants %p, vector4f_count %u.\n",
            device, start_register, constants, vector4f_count);

    /* Specifically test start_register > limit to catch MAX_UINT overflows
     * when adding start_register + vector4f_count. */
    if (!constants
            || start_register + vector4f_count > device->d3d_vshader_constantF
            || start_register > device->d3d_vshader_constantF)
        return WINED3DERR_INVALIDCALL;

    memcpy(&device->updateStateBlock->state.vs_consts_f[start_register * 4],
            constants, vector4f_count * sizeof(float) * 4);
    if (TRACE_ON(d3d))
    {
        for (i = 0; i < vector4f_count; ++i)
            TRACE("Set FLOAT constant %u to {%.8e, %.8e, %.8e, %.8e}.\n", start_register + i,
                    constants[i * 4], constants[i * 4 + 1],
                    constants[i * 4 + 2], constants[i * 4 + 3]);
    }

    if (!device->isRecordingState)
    {
        device->shader_backend->shader_update_float_vertex_constants(device, start_register, vector4f_count);
        device_invalidate_state(device, STATE_VERTEXSHADERCONSTANT);
    }

    memset(device->updateStateBlock->changed.vertexShaderConstantsF + start_register, 1,
            sizeof(*device->updateStateBlock->changed.vertexShaderConstantsF) * vector4f_count);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_vs_consts_f(const struct wined3d_device *device,
        UINT start_register, float *constants, UINT vector4f_count)
{
    int count = min(vector4f_count, device->d3d_vshader_constantF - start_register);

    TRACE("device %p, start_register %u, constants %p, vector4f_count %u.\n",
            device, start_register, constants, vector4f_count);

    if (!constants || count < 0)
        return WINED3DERR_INVALIDCALL;

    memcpy(constants, &device->stateBlock->state.vs_consts_f[start_register * 4], count * sizeof(float) * 4);

    return WINED3D_OK;
}

static void device_invalidate_texture_stage(const struct wined3d_device *device, DWORD stage)
{
    DWORD i;

    for (i = 0; i <= WINED3D_HIGHEST_TEXTURE_STATE; ++i)
    {
        device_invalidate_state(device, STATE_TEXTURESTAGE(stage, i));
    }
}

static void device_map_stage(struct wined3d_device *device, DWORD stage, DWORD unit)
{
    DWORD i = device->rev_tex_unit_map[unit];
    DWORD j = device->texUnitMap[stage];

    device->texUnitMap[stage] = unit;
    if (i != WINED3D_UNMAPPED_STAGE && i != stage)
        device->texUnitMap[i] = WINED3D_UNMAPPED_STAGE;

    device->rev_tex_unit_map[unit] = stage;
    if (j != WINED3D_UNMAPPED_STAGE && j != unit)
        device->rev_tex_unit_map[j] = WINED3D_UNMAPPED_STAGE;
}

static void device_update_fixed_function_usage_map(struct wined3d_device *device)
{
    UINT i;

    device->fixed_function_usage_map = 0;
    for (i = 0; i < MAX_TEXTURES; ++i)
    {
        const struct wined3d_state *state = &device->stateBlock->state;
        enum wined3d_texture_op color_op = state->texture_states[i][WINED3D_TSS_COLOR_OP];
        enum wined3d_texture_op alpha_op = state->texture_states[i][WINED3D_TSS_ALPHA_OP];
        DWORD color_arg1 = state->texture_states[i][WINED3D_TSS_COLOR_ARG1] & WINED3DTA_SELECTMASK;
        DWORD color_arg2 = state->texture_states[i][WINED3D_TSS_COLOR_ARG2] & WINED3DTA_SELECTMASK;
        DWORD color_arg3 = state->texture_states[i][WINED3D_TSS_COLOR_ARG0] & WINED3DTA_SELECTMASK;
        DWORD alpha_arg1 = state->texture_states[i][WINED3D_TSS_ALPHA_ARG1] & WINED3DTA_SELECTMASK;
        DWORD alpha_arg2 = state->texture_states[i][WINED3D_TSS_ALPHA_ARG2] & WINED3DTA_SELECTMASK;
        DWORD alpha_arg3 = state->texture_states[i][WINED3D_TSS_ALPHA_ARG0] & WINED3DTA_SELECTMASK;

        /* Not used, and disable higher stages. */
        if (color_op == WINED3D_TOP_DISABLE)
            break;

        if (((color_arg1 == WINED3DTA_TEXTURE) && color_op != WINED3D_TOP_SELECT_ARG2)
                || ((color_arg2 == WINED3DTA_TEXTURE) && color_op != WINED3D_TOP_SELECT_ARG1)
                || ((color_arg3 == WINED3DTA_TEXTURE)
                    && (color_op == WINED3D_TOP_MULTIPLY_ADD || color_op == WINED3D_TOP_LERP))
                || ((alpha_arg1 == WINED3DTA_TEXTURE) && alpha_op != WINED3D_TOP_SELECT_ARG2)
                || ((alpha_arg2 == WINED3DTA_TEXTURE) && alpha_op != WINED3D_TOP_SELECT_ARG1)
                || ((alpha_arg3 == WINED3DTA_TEXTURE)
                    && (alpha_op == WINED3D_TOP_MULTIPLY_ADD || alpha_op == WINED3D_TOP_LERP)))
            device->fixed_function_usage_map |= (1 << i);

        if ((color_op == WINED3D_TOP_BUMPENVMAP || color_op == WINED3D_TOP_BUMPENVMAP_LUMINANCE)
                && i < MAX_TEXTURES - 1)
            device->fixed_function_usage_map |= (1 << (i + 1));
    }
}

static void device_map_fixed_function_samplers(struct wined3d_device *device, const struct wined3d_gl_info *gl_info)
{
    unsigned int i, tex;
    WORD ffu_map;

    device_update_fixed_function_usage_map(device);
    ffu_map = device->fixed_function_usage_map;

    if (device->max_ffp_textures == gl_info->limits.texture_stages
            || device->stateBlock->state.lowest_disabled_stage <= device->max_ffp_textures)
    {
        for (i = 0; ffu_map; ffu_map >>= 1, ++i)
        {
            if (!(ffu_map & 1)) continue;

            if (device->texUnitMap[i] != i)
            {
                device_map_stage(device, i, i);
                device_invalidate_state(device, STATE_SAMPLER(i));
                device_invalidate_texture_stage(device, i);
            }
        }
        return;
    }

    /* Now work out the mapping */
    tex = 0;
    for (i = 0; ffu_map; ffu_map >>= 1, ++i)
    {
        if (!(ffu_map & 1)) continue;

        if (device->texUnitMap[i] != tex)
        {
            device_map_stage(device, i, tex);
            device_invalidate_state(device, STATE_SAMPLER(i));
            device_invalidate_texture_stage(device, i);
        }

        ++tex;
    }
}

static void device_map_psamplers(struct wined3d_device *device, const struct wined3d_gl_info *gl_info)
{
    const enum wined3d_sampler_texture_type *sampler_type =
            device->stateBlock->state.pixel_shader->reg_maps.sampler_type;
    unsigned int i;

    for (i = 0; i < MAX_FRAGMENT_SAMPLERS; ++i)
    {
        if (sampler_type[i] && device->texUnitMap[i] != i)
        {
            device_map_stage(device, i, i);
            device_invalidate_state(device, STATE_SAMPLER(i));
            if (i < gl_info->limits.texture_stages)
                device_invalidate_texture_stage(device, i);
        }
    }
}

static BOOL device_unit_free_for_vs(const struct wined3d_device *device,
        const enum wined3d_sampler_texture_type *pshader_sampler_tokens,
        const enum wined3d_sampler_texture_type *vshader_sampler_tokens, DWORD unit)
{
    DWORD current_mapping = device->rev_tex_unit_map[unit];

    /* Not currently used */
    if (current_mapping == WINED3D_UNMAPPED_STAGE) return TRUE;

    if (current_mapping < MAX_FRAGMENT_SAMPLERS) {
        /* Used by a fragment sampler */

        if (!pshader_sampler_tokens) {
            /* No pixel shader, check fixed function */
            return current_mapping >= MAX_TEXTURES || !(device->fixed_function_usage_map & (1 << current_mapping));
        }

        /* Pixel shader, check the shader's sampler map */
        return !pshader_sampler_tokens[current_mapping];
    }

    /* Used by a vertex sampler */
    return !vshader_sampler_tokens[current_mapping - MAX_FRAGMENT_SAMPLERS];
}

static void device_map_vsamplers(struct wined3d_device *device, BOOL ps, const struct wined3d_gl_info *gl_info)
{
    const enum wined3d_sampler_texture_type *vshader_sampler_type =
            device->stateBlock->state.vertex_shader->reg_maps.sampler_type;
    const enum wined3d_sampler_texture_type *pshader_sampler_type = NULL;
    int start = min(MAX_COMBINED_SAMPLERS, gl_info->limits.combined_samplers) - 1;
    int i;

    if (ps)
    {
        /* Note that we only care if a sampler is sampled or not, not the sampler's specific type.
         * Otherwise we'd need to call shader_update_samplers() here for 1.x pixelshaders. */
        pshader_sampler_type = device->stateBlock->state.pixel_shader->reg_maps.sampler_type;
    }

    for (i = 0; i < MAX_VERTEX_SAMPLERS; ++i) {
        DWORD vsampler_idx = i + MAX_FRAGMENT_SAMPLERS;
        if (vshader_sampler_type[i])
        {
            if (device->texUnitMap[vsampler_idx] != WINED3D_UNMAPPED_STAGE)
            {
                /* Already mapped somewhere */
                continue;
            }

            while (start >= 0)
            {
                if (device_unit_free_for_vs(device, pshader_sampler_type, vshader_sampler_type, start))
                {
                    device_map_stage(device, vsampler_idx, start);
                    device_invalidate_state(device, STATE_SAMPLER(vsampler_idx));

                    --start;
                    break;
                }

                --start;
            }
        }
    }
}

void device_update_tex_unit_map(struct wined3d_device *device)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    const struct wined3d_state *state = &device->stateBlock->state;
    BOOL vs = use_vs(state);
    BOOL ps = use_ps(state);
    /*
     * Rules are:
     * -> Pixel shaders need a 1:1 map. In theory the shader input could be mapped too, but
     * that would be really messy and require shader recompilation
     * -> When the mapping of a stage is changed, sampler and ALL texture stage states have
     * to be reset. Because of that try to work with a 1:1 mapping as much as possible
     */
    if (ps)
        device_map_psamplers(device, gl_info);
    else
        device_map_fixed_function_samplers(device, gl_info);

    if (vs)
        device_map_vsamplers(device, ps, gl_info);
}

HRESULT CDECL wined3d_device_set_pixel_shader(struct wined3d_device *device, struct wined3d_shader *shader)
{
    struct wined3d_shader *prev = device->updateStateBlock->state.pixel_shader;

    TRACE("device %p, shader %p.\n", device, shader);

    device->updateStateBlock->state.pixel_shader = shader;
    device->updateStateBlock->changed.pixelShader = TRUE;

    if (device->isRecordingState)
    {
        if (shader)
            wined3d_shader_incref(shader);
        if (prev)
            wined3d_shader_decref(prev);
        TRACE("Recording... not performing anything.\n");
        return WINED3D_OK;
    }

    if (shader == prev)
    {
        TRACE("Application is setting the old shader over, nothing to do.\n");
        return WINED3D_OK;
    }

    if (shader)
        wined3d_shader_incref(shader);
    if (prev)
        wined3d_shader_decref(prev);

    device_invalidate_state(device, STATE_PIXELSHADER);

    return WINED3D_OK;
}

struct wined3d_shader * CDECL wined3d_device_get_pixel_shader(const struct wined3d_device *device)
{
    struct wined3d_shader *shader;

    TRACE("device %p.\n", device);

    shader = device->stateBlock->state.pixel_shader;
    if (shader)
        wined3d_shader_incref(shader);

    TRACE("Returning %p.\n", shader);
    return shader;
}

HRESULT CDECL wined3d_device_set_ps_consts_b(struct wined3d_device *device,
        UINT start_register, const BOOL *constants, UINT bool_count)
{
    UINT count = min(bool_count, MAX_CONST_B - start_register);
    UINT i;

    TRACE("device %p, start_register %u, constants %p, bool_count %u.\n",
            device, start_register, constants, bool_count);

    if (!constants || start_register >= MAX_CONST_B)
        return WINED3DERR_INVALIDCALL;

    memcpy(&device->updateStateBlock->state.ps_consts_b[start_register], constants, count * sizeof(BOOL));
    for (i = 0; i < count; ++i)
        TRACE("Set BOOL constant %u to %s.\n", start_register + i, constants[i] ? "true" : "false");

    for (i = start_register; i < count + start_register; ++i)
        device->updateStateBlock->changed.pixelShaderConstantsB |= (1 << i);

    if (!device->isRecordingState)
        device_invalidate_state(device, STATE_PIXELSHADERCONSTANT);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_ps_consts_b(const struct wined3d_device *device,
        UINT start_register, BOOL *constants, UINT bool_count)
{
    UINT count = min(bool_count, MAX_CONST_B - start_register);

    TRACE("device %p, start_register %u, constants %p, bool_count %u.\n",
            device, start_register, constants, bool_count);

    if (!constants || start_register >= MAX_CONST_B)
        return WINED3DERR_INVALIDCALL;

    memcpy(constants, &device->stateBlock->state.ps_consts_b[start_register], count * sizeof(BOOL));

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_ps_consts_i(struct wined3d_device *device,
        UINT start_register, const int *constants, UINT vector4i_count)
{
    UINT count = min(vector4i_count, MAX_CONST_I - start_register);
    UINT i;

    TRACE("device %p, start_register %u, constants %p, vector4i_count %u.\n",
            device, start_register, constants, vector4i_count);

    if (!constants || start_register >= MAX_CONST_I)
        return WINED3DERR_INVALIDCALL;

    memcpy(&device->updateStateBlock->state.ps_consts_i[start_register * 4], constants, count * sizeof(int) * 4);
    for (i = 0; i < count; ++i)
        TRACE("Set INT constant %u to {%d, %d, %d, %d}.\n", start_register + i,
                constants[i * 4], constants[i * 4 + 1],
                constants[i * 4 + 2], constants[i * 4 + 3]);

    for (i = start_register; i < count + start_register; ++i)
        device->updateStateBlock->changed.pixelShaderConstantsI |= (1 << i);

    if (!device->isRecordingState)
        device_invalidate_state(device, STATE_PIXELSHADERCONSTANT);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_ps_consts_i(const struct wined3d_device *device,
        UINT start_register, int *constants, UINT vector4i_count)
{
    UINT count = min(vector4i_count, MAX_CONST_I - start_register);

    TRACE("device %p, start_register %u, constants %p, vector4i_count %u.\n",
            device, start_register, constants, vector4i_count);

    if (!constants || start_register >= MAX_CONST_I)
        return WINED3DERR_INVALIDCALL;

    memcpy(constants, &device->stateBlock->state.ps_consts_i[start_register * 4], count * sizeof(int) * 4);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_ps_consts_f(struct wined3d_device *device,
        UINT start_register, const float *constants, UINT vector4f_count)
{
    UINT i;

    TRACE("device %p, start_register %u, constants %p, vector4f_count %u.\n",
            device, start_register, constants, vector4f_count);

    /* Specifically test start_register > limit to catch MAX_UINT overflows
     * when adding start_register + vector4f_count. */
    if (!constants
            || start_register + vector4f_count > device->d3d_pshader_constantF
            || start_register > device->d3d_pshader_constantF)
        return WINED3DERR_INVALIDCALL;

    memcpy(&device->updateStateBlock->state.ps_consts_f[start_register * 4],
            constants, vector4f_count * sizeof(float) * 4);
    if (TRACE_ON(d3d))
    {
        for (i = 0; i < vector4f_count; ++i)
            TRACE("Set FLOAT constant %u to {%.8e, %.8e, %.8e, %.8e}.\n", start_register + i,
                    constants[i * 4], constants[i * 4 + 1],
                    constants[i * 4 + 2], constants[i * 4 + 3]);
    }

    if (!device->isRecordingState)
    {
        device->shader_backend->shader_update_float_pixel_constants(device, start_register, vector4f_count);
        device_invalidate_state(device, STATE_PIXELSHADERCONSTANT);
    }

    memset(device->updateStateBlock->changed.pixelShaderConstantsF + start_register, 1,
            sizeof(*device->updateStateBlock->changed.pixelShaderConstantsF) * vector4f_count);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_ps_consts_f(const struct wined3d_device *device,
        UINT start_register, float *constants, UINT vector4f_count)
{
    int count = min(vector4f_count, device->d3d_pshader_constantF - start_register);

    TRACE("device %p, start_register %u, constants %p, vector4f_count %u.\n",
            device, start_register, constants, vector4f_count);

    if (!constants || count < 0)
        return WINED3DERR_INVALIDCALL;

    memcpy(constants, &device->stateBlock->state.ps_consts_f[start_register * 4], count * sizeof(float) * 4);

    return WINED3D_OK;
}

/* Context activation is done by the caller. */
/* Do not call while under the GL lock. */
#define copy_and_next(dest, src, size) memcpy(dest, src, size); dest += (size)
static HRESULT process_vertices_strided(const struct wined3d_device *device, DWORD dwDestIndex, DWORD dwCount,
        const struct wined3d_stream_info *stream_info, struct wined3d_buffer *dest, DWORD flags,
        DWORD DestFVF)
{
    struct wined3d_matrix mat, proj_mat, view_mat, world_mat;
    struct wined3d_viewport vp;
    UINT vertex_size;
    unsigned int i;
    BYTE *dest_ptr;
    BOOL doClip;
    DWORD numTextures;
    HRESULT hr;

    if (stream_info->use_map & (1 << WINED3D_FFP_NORMAL))
    {
        WARN(" lighting state not saved yet... Some strange stuff may happen !\n");
    }

    if (!(stream_info->use_map & (1 << WINED3D_FFP_POSITION)))
    {
        ERR("Source has no position mask\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (device->stateBlock->state.render_states[WINED3D_RS_CLIPPING])
    {
        static BOOL warned = FALSE;
        /*
         * The clipping code is not quite correct. Some things need
         * to be checked against IDirect3DDevice3 (!), d3d8 and d3d9,
         * so disable clipping for now.
         * (The graphics in Half-Life are broken, and my processvertices
         *  test crashes with IDirect3DDevice3)
        doClip = TRUE;
         */
        doClip = FALSE;
        if(!warned) {
           warned = TRUE;
           FIXME("Clipping is broken and disabled for now\n");
        }
    }
    else
        doClip = FALSE;

    vertex_size = get_flexible_vertex_size(DestFVF);
    if (FAILED(hr = wined3d_buffer_map(dest, dwDestIndex * vertex_size, dwCount * vertex_size, &dest_ptr, 0)))
    {
        WARN("Failed to map buffer, hr %#x.\n", hr);
        return hr;
    }

    wined3d_device_get_transform(device, WINED3D_TS_VIEW, &view_mat);
    wined3d_device_get_transform(device, WINED3D_TS_PROJECTION, &proj_mat);
    wined3d_device_get_transform(device, WINED3D_TS_WORLD_MATRIX(0), &world_mat);

    TRACE("View mat:\n");
    TRACE("%f %f %f %f\n", view_mat.u.s._11, view_mat.u.s._12, view_mat.u.s._13, view_mat.u.s._14);
    TRACE("%f %f %f %f\n", view_mat.u.s._21, view_mat.u.s._22, view_mat.u.s._23, view_mat.u.s._24);
    TRACE("%f %f %f %f\n", view_mat.u.s._31, view_mat.u.s._32, view_mat.u.s._33, view_mat.u.s._34);
    TRACE("%f %f %f %f\n", view_mat.u.s._41, view_mat.u.s._42, view_mat.u.s._43, view_mat.u.s._44);

    TRACE("Proj mat:\n");
    TRACE("%f %f %f %f\n", proj_mat.u.s._11, proj_mat.u.s._12, proj_mat.u.s._13, proj_mat.u.s._14);
    TRACE("%f %f %f %f\n", proj_mat.u.s._21, proj_mat.u.s._22, proj_mat.u.s._23, proj_mat.u.s._24);
    TRACE("%f %f %f %f\n", proj_mat.u.s._31, proj_mat.u.s._32, proj_mat.u.s._33, proj_mat.u.s._34);
    TRACE("%f %f %f %f\n", proj_mat.u.s._41, proj_mat.u.s._42, proj_mat.u.s._43, proj_mat.u.s._44);

    TRACE("World mat:\n");
    TRACE("%f %f %f %f\n", world_mat.u.s._11, world_mat.u.s._12, world_mat.u.s._13, world_mat.u.s._14);
    TRACE("%f %f %f %f\n", world_mat.u.s._21, world_mat.u.s._22, world_mat.u.s._23, world_mat.u.s._24);
    TRACE("%f %f %f %f\n", world_mat.u.s._31, world_mat.u.s._32, world_mat.u.s._33, world_mat.u.s._34);
    TRACE("%f %f %f %f\n", world_mat.u.s._41, world_mat.u.s._42, world_mat.u.s._43, world_mat.u.s._44);

    /* Get the viewport */
    wined3d_device_get_viewport(device, &vp);
    TRACE("viewport  x %u, y %u, width %u, height %u, min_z %.8e, max_z %.8e.\n",
          vp.x, vp.y, vp.width, vp.height, vp.min_z, vp.max_z);

    multiply_matrix(&mat,&view_mat,&world_mat);
    multiply_matrix(&mat,&proj_mat,&mat);

    numTextures = (DestFVF & WINED3DFVF_TEXCOUNT_MASK) >> WINED3DFVF_TEXCOUNT_SHIFT;

    for (i = 0; i < dwCount; i+= 1) {
        unsigned int tex_index;

        if ( ((DestFVF & WINED3DFVF_POSITION_MASK) == WINED3DFVF_XYZ ) ||
             ((DestFVF & WINED3DFVF_POSITION_MASK) == WINED3DFVF_XYZRHW ) ) {
            /* The position first */
            const struct wined3d_stream_info_element *element = &stream_info->elements[WINED3D_FFP_POSITION];
            const float *p = (const float *)(element->data.addr + i * element->stride);
            float x, y, z, rhw;
            TRACE("In: ( %06.2f %06.2f %06.2f )\n", p[0], p[1], p[2]);

            /* Multiplication with world, view and projection matrix */
            x =   (p[0] * mat.u.s._11) + (p[1] * mat.u.s._21) + (p[2] * mat.u.s._31) + (1.0f * mat.u.s._41);
            y =   (p[0] * mat.u.s._12) + (p[1] * mat.u.s._22) + (p[2] * mat.u.s._32) + (1.0f * mat.u.s._42);
            z =   (p[0] * mat.u.s._13) + (p[1] * mat.u.s._23) + (p[2] * mat.u.s._33) + (1.0f * mat.u.s._43);
            rhw = (p[0] * mat.u.s._14) + (p[1] * mat.u.s._24) + (p[2] * mat.u.s._34) + (1.0f * mat.u.s._44);

            TRACE("x=%f y=%f z=%f rhw=%f\n", x, y, z, rhw);

            /* WARNING: The following things are taken from d3d7 and were not yet checked
             * against d3d8 or d3d9!
             */

            /* Clipping conditions: From msdn
             *
             * A vertex is clipped if it does not match the following requirements
             * -rhw < x <= rhw
             * -rhw < y <= rhw
             *    0 < z <= rhw
             *    0 < rhw ( Not in d3d7, but tested in d3d7)
             *
             * If clipping is on is determined by the D3DVOP_CLIP flag in D3D7, and
             * by the D3DRS_CLIPPING in D3D9(according to the msdn, not checked)
             *
             */

            if( !doClip ||
                ( (-rhw -eps < x) && (-rhw -eps < y) && ( -eps < z) &&
                  (x <= rhw + eps) && (y <= rhw + eps ) && (z <= rhw + eps) &&
                  ( rhw > eps ) ) ) {

                /* "Normal" viewport transformation (not clipped)
                 * 1) The values are divided by rhw
                 * 2) The y axis is negative, so multiply it with -1
                 * 3) Screen coordinates go from -(Width/2) to +(Width/2) and
                 *    -(Height/2) to +(Height/2). The z range is MinZ to MaxZ
                 * 4) Multiply x with Width/2 and add Width/2
                 * 5) The same for the height
                 * 6) Add the viewpoint X and Y to the 2D coordinates and
                 *    The minimum Z value to z
                 * 7) rhw = 1 / rhw Reciprocal of Homogeneous W....
                 *
                 * Well, basically it's simply a linear transformation into viewport
                 * coordinates
                 */

                x /= rhw;
                y /= rhw;
                z /= rhw;

                y *= -1;

                x *= vp.width / 2;
                y *= vp.height / 2;
                z *= vp.max_z - vp.min_z;

                x += vp.width / 2 + vp.x;
                y += vp.height / 2 + vp.y;
                z += vp.min_z;

                rhw = 1 / rhw;
            } else {
                /* That vertex got clipped
                 * Contrary to OpenGL it is not dropped completely, it just
                 * undergoes a different calculation.
                 */
                TRACE("Vertex got clipped\n");
                x += rhw;
                y += rhw;

                x  /= 2;
                y  /= 2;

                /* Msdn mentions that Direct3D9 keeps a list of clipped vertices
                 * outside of the main vertex buffer memory. That needs some more
                 * investigation...
                 */
            }

            TRACE("Writing (%f %f %f) %f\n", x, y, z, rhw);


            ( (float *) dest_ptr)[0] = x;
            ( (float *) dest_ptr)[1] = y;
            ( (float *) dest_ptr)[2] = z;
            ( (float *) dest_ptr)[3] = rhw; /* SIC, see ddraw test! */

            dest_ptr += 3 * sizeof(float);

            if ((DestFVF & WINED3DFVF_POSITION_MASK) == WINED3DFVF_XYZRHW)
                dest_ptr += sizeof(float);
        }

        if (DestFVF & WINED3DFVF_PSIZE)
            dest_ptr += sizeof(DWORD);

        if (DestFVF & WINED3DFVF_NORMAL)
        {
            const struct wined3d_stream_info_element *element = &stream_info->elements[WINED3D_FFP_NORMAL];
            const float *normal = (const float *)(element->data.addr + i * element->stride);
            /* AFAIK this should go into the lighting information */
            FIXME("Didn't expect the destination to have a normal\n");
            copy_and_next(dest_ptr, normal, 3 * sizeof(float));
        }

        if (DestFVF & WINED3DFVF_DIFFUSE)
        {
            const struct wined3d_stream_info_element *element = &stream_info->elements[WINED3D_FFP_DIFFUSE];
            const DWORD *color_d = (const DWORD *)(element->data.addr + i * element->stride);
            if (!(stream_info->use_map & (1 << WINED3D_FFP_DIFFUSE)))
            {
                static BOOL warned = FALSE;

                if(!warned) {
                    ERR("No diffuse color in source, but destination has one\n");
                    warned = TRUE;
                }

                *( (DWORD *) dest_ptr) = 0xffffffff;
                dest_ptr += sizeof(DWORD);
            }
            else
            {
                copy_and_next(dest_ptr, color_d, sizeof(DWORD));
            }
        }

        if (DestFVF & WINED3DFVF_SPECULAR)
        {
            /* What's the color value in the feedback buffer? */
            const struct wined3d_stream_info_element *element = &stream_info->elements[WINED3D_FFP_SPECULAR];
            const DWORD *color_s = (const DWORD *)(element->data.addr + i * element->stride);
            if (!(stream_info->use_map & (1 << WINED3D_FFP_SPECULAR)))
            {
                static BOOL warned = FALSE;

                if(!warned) {
                    ERR("No specular color in source, but destination has one\n");
                    warned = TRUE;
                }

                *(DWORD *)dest_ptr = 0xff000000;
                dest_ptr += sizeof(DWORD);
            }
            else
            {
                copy_and_next(dest_ptr, color_s, sizeof(DWORD));
            }
        }

        for (tex_index = 0; tex_index < numTextures; ++tex_index)
        {
            const struct wined3d_stream_info_element *element = &stream_info->elements[WINED3D_FFP_TEXCOORD0 + tex_index];
            const float *tex_coord = (const float *)(element->data.addr + i * element->stride);
            if (!(stream_info->use_map & (1 << (WINED3D_FFP_TEXCOORD0 + tex_index))))
            {
                ERR("No source texture, but destination requests one\n");
                dest_ptr += GET_TEXCOORD_SIZE_FROM_FVF(DestFVF, tex_index) * sizeof(float);
            }
            else
            {
                copy_and_next(dest_ptr, tex_coord, GET_TEXCOORD_SIZE_FROM_FVF(DestFVF, tex_index) * sizeof(float));
            }
        }
    }

    wined3d_buffer_unmap(dest);

    return WINED3D_OK;
}
#undef copy_and_next

/* Do not call while under the GL lock. */
HRESULT CDECL wined3d_device_process_vertices(struct wined3d_device *device,
        UINT src_start_idx, UINT dst_idx, UINT vertex_count, struct wined3d_buffer *dst_buffer,
        const struct wined3d_vertex_declaration *declaration, DWORD flags, DWORD dst_fvf)
{
    struct wined3d_state *state = &device->stateBlock->state;
    struct wined3d_stream_info stream_info;
    const struct wined3d_gl_info *gl_info;
    BOOL streamWasUP = state->user_stream;
    struct wined3d_context *context;
    struct wined3d_shader *vs;
    unsigned int i;
    HRESULT hr;

    TRACE("device %p, src_start_idx %u, dst_idx %u, vertex_count %u, "
            "dst_buffer %p, declaration %p, flags %#x, dst_fvf %#x.\n",
            device, src_start_idx, dst_idx, vertex_count,
            dst_buffer, declaration, flags, dst_fvf);

    if (declaration)
        FIXME("Output vertex declaration not implemented yet.\n");

    /* Need any context to write to the vbo. */
    context = context_acquire(device, NULL);
    gl_info = context->gl_info;

    /* ProcessVertices reads from vertex buffers, which have to be assigned.
     * DrawPrimitive and DrawPrimitiveUP control the streamIsUP flag, thus
     * restore it afterwards. */
    vs = state->vertex_shader;
    state->vertex_shader = NULL;
    state->user_stream = FALSE;
    device_stream_info_from_declaration(device, &stream_info);
    state->user_stream = streamWasUP;
    state->vertex_shader = vs;

    /* We can't convert FROM a VBO, and vertex buffers used to source into
     * process_vertices() are unlikely to ever be used for drawing. Release
     * VBOs in those buffers and fix up the stream_info structure.
     *
     * Also apply the start index. */
    for (i = 0; i < (sizeof(stream_info.elements) / sizeof(*stream_info.elements)); ++i)
    {
        struct wined3d_stream_info_element *e;

        if (!(stream_info.use_map & (1 << i)))
            continue;

        e = &stream_info.elements[i];
        if (e->data.buffer_object)
        {
            struct wined3d_buffer *vb = state->streams[e->stream_idx].buffer;
            e->data.buffer_object = 0;
            e->data.addr = (BYTE *)((ULONG_PTR)e->data.addr + (ULONG_PTR)buffer_get_sysmem(vb, gl_info));
            ENTER_GL();
            GL_EXTCALL(glDeleteBuffersARB(1, &vb->buffer_object));
            vb->buffer_object = 0;
            LEAVE_GL();
        }
        if (e->data.addr)
            e->data.addr += e->stride * src_start_idx;
    }

    hr = process_vertices_strided(device, dst_idx, vertex_count,
            &stream_info, dst_buffer, flags, dst_fvf);

    context_release(context);

    return hr;
}

HRESULT CDECL wined3d_device_set_texture_stage_state(struct wined3d_device *device,
        UINT stage, enum wined3d_texture_stage_state state, DWORD value)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    DWORD old_value;

    TRACE("device %p, stage %u, state %s, value %#x.\n",
            device, stage, debug_d3dtexturestate(state), value);

    if (state > WINED3D_HIGHEST_TEXTURE_STATE)
    {
        WARN("Invalid state %#x passed.\n", state);
        return WINED3D_OK;
    }

    if (stage >= gl_info->limits.texture_stages)
    {
        WARN("Attempting to set stage %u which is higher than the max stage %u, ignoring.\n",
                stage, gl_info->limits.texture_stages - 1);
        return WINED3D_OK;
    }

    old_value = device->updateStateBlock->state.texture_states[stage][state];
    device->updateStateBlock->changed.textureState[stage] |= 1 << state;
    device->updateStateBlock->state.texture_states[stage][state] = value;

    if (device->isRecordingState)
    {
        TRACE("Recording... not performing anything.\n");
        return WINED3D_OK;
    }

    /* Checked after the assignments to allow proper stateblock recording. */
    if (old_value == value)
    {
        TRACE("Application is setting the old value over, nothing to do.\n");
        return WINED3D_OK;
    }

    if (stage > device->stateBlock->state.lowest_disabled_stage
            && device->StateTable[STATE_TEXTURESTAGE(0, state)].representative
            == STATE_TEXTURESTAGE(0, WINED3D_TSS_COLOR_OP))
    {
        /* Colorop change above lowest disabled stage? That won't change
         * anything in the GL setup. Changes in other states are important on
         * disabled stages too. */
        return WINED3D_OK;
    }

    if (state == WINED3D_TSS_COLOR_OP)
    {
        unsigned int i;

        if (value == WINED3D_TOP_DISABLE && old_value != WINED3D_TOP_DISABLE)
        {
            /* Previously enabled stage disabled now. Make sure to dirtify
             * all enabled stages above stage, they have to be disabled.
             *
             * The current stage is dirtified below. */
            for (i = stage + 1; i < device->stateBlock->state.lowest_disabled_stage; ++i)
            {
                TRACE("Additionally dirtifying stage %u.\n", i);
                device_invalidate_state(device, STATE_TEXTURESTAGE(i, WINED3D_TSS_COLOR_OP));
            }
            device->stateBlock->state.lowest_disabled_stage = stage;
            TRACE("New lowest disabled: %u.\n", stage);
        }
        else if (value != WINED3D_TOP_DISABLE && old_value == WINED3D_TOP_DISABLE)
        {
            /* Previously disabled stage enabled. Stages above it may need
             * enabling. Stage must be lowest_disabled_stage here, if it's
             * bigger success is returned above, and stages below the lowest
             * disabled stage can't be enabled (because they are enabled
             * already).
             *
             * Again stage stage doesn't need to be dirtified here, it is
             * handled below. */
            for (i = stage + 1; i < gl_info->limits.texture_stages; ++i)
            {
                if (device->updateStateBlock->state.texture_states[i][WINED3D_TSS_COLOR_OP] == WINED3D_TOP_DISABLE)
                    break;
                TRACE("Additionally dirtifying stage %u due to enable.\n", i);
                device_invalidate_state(device, STATE_TEXTURESTAGE(i, WINED3D_TSS_COLOR_OP));
            }
            device->stateBlock->state.lowest_disabled_stage = i;
            TRACE("New lowest disabled: %u.\n", i);
        }
    }

    device_invalidate_state(device, STATE_TEXTURESTAGE(stage, state));

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_texture_stage_state(const struct wined3d_device *device,
        UINT stage, enum wined3d_texture_stage_state state, DWORD *value)
{
    TRACE("device %p, stage %u, state %s, value %p.\n",
            device, stage, debug_d3dtexturestate(state), value);

    if (state > WINED3D_HIGHEST_TEXTURE_STATE)
    {
        WARN("Invalid state %#x passed.\n", state);
        return WINED3D_OK;
    }

    *value = device->updateStateBlock->state.texture_states[stage][state];
    TRACE("Returning %#x.\n", *value);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_texture(struct wined3d_device *device,
        UINT stage, struct wined3d_texture *texture)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_texture *prev;

    TRACE("device %p, stage %u, texture %p.\n", device, stage, texture);

    if (stage >= WINED3DVERTEXTEXTURESAMPLER0 && stage <= WINED3DVERTEXTEXTURESAMPLER3)
        stage -= (WINED3DVERTEXTEXTURESAMPLER0 - MAX_FRAGMENT_SAMPLERS);

    /* Windows accepts overflowing this array... we do not. */
    if (stage >= sizeof(device->stateBlock->state.textures) / sizeof(*device->stateBlock->state.textures))
    {
        WARN("Ignoring invalid stage %u.\n", stage);
        return WINED3D_OK;
    }

    if (texture && texture->resource.pool == WINED3D_POOL_SCRATCH)
    {
        WARN("Rejecting attempt to set scratch texture.\n");
        return WINED3DERR_INVALIDCALL;
    }

    device->updateStateBlock->changed.textures |= 1 << stage;

    prev = device->updateStateBlock->state.textures[stage];
    TRACE("Previous texture %p.\n", prev);

    if (texture == prev)
    {
        TRACE("App is setting the same texture again, nothing to do.\n");
        return WINED3D_OK;
    }

    TRACE("Setting new texture to %p.\n", texture);
    device->updateStateBlock->state.textures[stage] = texture;

    if (device->isRecordingState)
    {
        TRACE("Recording... not performing anything\n");

        if (texture) wined3d_texture_incref(texture);
        if (prev) wined3d_texture_decref(prev);

        return WINED3D_OK;
    }

    if (texture)
    {
        LONG bind_count = InterlockedIncrement(&texture->resource.bind_count);

        wined3d_texture_incref(texture);

        if (!prev || texture->target != prev->target)
            device_invalidate_state(device, STATE_PIXELSHADER);

        if (!prev && stage < gl_info->limits.texture_stages)
        {
            /* The source arguments for color and alpha ops have different
             * meanings when a NULL texture is bound, so the COLOR_OP and
             * ALPHA_OP have to be dirtified. */
            device_invalidate_state(device, STATE_TEXTURESTAGE(stage, WINED3D_TSS_COLOR_OP));
            device_invalidate_state(device, STATE_TEXTURESTAGE(stage, WINED3D_TSS_ALPHA_OP));
        }

        if (bind_count == 1)
            texture->sampler = stage;
    }

    if (prev)
    {
        LONG bind_count = InterlockedDecrement(&prev->resource.bind_count);

        wined3d_texture_decref(prev);

        if (!texture && stage < gl_info->limits.texture_stages)
        {
            device_invalidate_state(device, STATE_TEXTURESTAGE(stage, WINED3D_TSS_COLOR_OP));
            device_invalidate_state(device, STATE_TEXTURESTAGE(stage, WINED3D_TSS_ALPHA_OP));
        }

        if (bind_count && prev->sampler == stage)
        {
            unsigned int i;

            /* Search for other stages the texture is bound to. Shouldn't
             * happen if applications bind textures to a single stage only. */
            TRACE("Searching for other stages the texture is bound to.\n");
            for (i = 0; i < MAX_COMBINED_SAMPLERS; ++i)
            {
                if (device->updateStateBlock->state.textures[i] == prev)
                {
                    TRACE("Texture is also bound to stage %u.\n", i);
                    prev->sampler = i;
                    break;
                }
            }
        }
    }

    device_invalidate_state(device, STATE_SAMPLER(stage));

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_texture(const struct wined3d_device *device,
        UINT stage, struct wined3d_texture **texture)
{
    TRACE("device %p, stage %u, texture %p.\n", device, stage, texture);

    if (stage >= WINED3DVERTEXTEXTURESAMPLER0 && stage <= WINED3DVERTEXTEXTURESAMPLER3)
        stage -= (WINED3DVERTEXTEXTURESAMPLER0 - MAX_FRAGMENT_SAMPLERS);

    if (stage >= sizeof(device->stateBlock->state.textures) / sizeof(*device->stateBlock->state.textures))
    {
        WARN("Ignoring invalid stage %u.\n", stage);
        return WINED3D_OK; /* Windows accepts overflowing this array ... we do not. */
    }

    *texture = device->stateBlock->state.textures[stage];
    if (*texture)
        wined3d_texture_incref(*texture);

    TRACE("Returning %p.\n", *texture);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_back_buffer(const struct wined3d_device *device, UINT swapchain_idx,
        UINT backbuffer_idx, enum wined3d_backbuffer_type backbuffer_type, struct wined3d_surface **backbuffer)
{
    struct wined3d_swapchain *swapchain;
    HRESULT hr;

    TRACE("device %p, swapchain_idx %u, backbuffer_idx %u, backbuffer_type %#x, backbuffer %p.\n",
            device, swapchain_idx, backbuffer_idx, backbuffer_type, backbuffer);

    hr = wined3d_device_get_swapchain(device, swapchain_idx, &swapchain);
    if (FAILED(hr))
    {
        WARN("Failed to get swapchain %u, hr %#x.\n", swapchain_idx, hr);
        return hr;
    }

    hr = wined3d_swapchain_get_back_buffer(swapchain, backbuffer_idx, backbuffer_type, backbuffer);
    wined3d_swapchain_decref(swapchain);
    if (FAILED(hr))
    {
        WARN("Failed to get backbuffer %u, hr %#x.\n", backbuffer_idx, hr);
        return hr;
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_device_caps(const struct wined3d_device *device, WINED3DCAPS *caps)
{
    TRACE("device %p, caps %p.\n", device, caps);

    return wined3d_get_device_caps(device->wined3d, device->adapter->ordinal,
            device->create_parms.device_type, caps);
}

HRESULT CDECL wined3d_device_get_display_mode(const struct wined3d_device *device, UINT swapchain_idx,
        struct wined3d_display_mode *mode, enum wined3d_display_rotation *rotation)
{
    struct wined3d_swapchain *swapchain;
    HRESULT hr;

    TRACE("device %p, swapchain_idx %u, mode %p, rotation %p.\n",
            device, swapchain_idx, mode, rotation);

    if (SUCCEEDED(hr = wined3d_device_get_swapchain(device, swapchain_idx, &swapchain)))
    {
        hr = wined3d_swapchain_get_display_mode(swapchain, mode, rotation);
        wined3d_swapchain_decref(swapchain);
    }

    return hr;
}

HRESULT CDECL wined3d_device_begin_stateblock(struct wined3d_device *device)
{
    struct wined3d_stateblock *stateblock;
    HRESULT hr;

    TRACE("device %p.\n", device);

    if (device->isRecordingState)
        return WINED3DERR_INVALIDCALL;

    hr = wined3d_stateblock_create(device, WINED3D_SBT_RECORDED, &stateblock);
    if (FAILED(hr))
        return hr;

    wined3d_stateblock_decref(device->updateStateBlock);
    device->updateStateBlock = stateblock;
    device->isRecordingState = TRUE;

    TRACE("Recording stateblock %p.\n", stateblock);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_end_stateblock(struct wined3d_device *device,
        struct wined3d_stateblock **stateblock)
{
    struct wined3d_stateblock *object = device->updateStateBlock;

    TRACE("device %p, stateblock %p.\n", device, stateblock);

    if (!device->isRecordingState)
    {
        WARN("Not recording.\n");
        *stateblock = NULL;
        return WINED3DERR_INVALIDCALL;
    }

    stateblock_init_contained_states(object);

    *stateblock = object;
    device->isRecordingState = FALSE;
    device->updateStateBlock = device->stateBlock;
    wined3d_stateblock_incref(device->updateStateBlock);

    TRACE("Returning stateblock %p.\n", *stateblock);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_begin_scene(struct wined3d_device *device)
{
    /* At the moment we have no need for any functionality at the beginning
     * of a scene. */
    TRACE("device %p.\n", device);

    if (device->inScene)
    {
        WARN("Already in scene, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }
    device->inScene = TRUE;
    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_end_scene(struct wined3d_device *device)
{
    struct wined3d_context *context;

    TRACE("device %p.\n", device);

    if (!device->inScene)
    {
        WARN("Not in scene, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    context = context_acquire(device, NULL);
    /* We only have to do this if we need to read the, swapbuffers performs a flush for us */
    context->gl_info->gl_ops.gl.p_glFlush();
    /* No checkGLcall here to avoid locking the lock just for checking a call that hardly ever
     * fails. */
    context_release(context);

    device->inScene = FALSE;
    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_present(const struct wined3d_device *device, const RECT *src_rect,
        const RECT *dst_rect, HWND dst_window_override, const RGNDATA *dirty_region, DWORD flags)
{
    UINT i;

    TRACE("device %p, src_rect %s, dst_rect %s, dst_window_override %p, dirty_region %p, flags %#x.\n",
            device, wine_dbgstr_rect(src_rect), wine_dbgstr_rect(dst_rect),
            dst_window_override, dirty_region, flags);

    for (i = 0; i < device->swapchain_count; ++i)
    {
        wined3d_swapchain_present(device->swapchains[i], src_rect,
                dst_rect, dst_window_override, dirty_region, flags);
    }

    return WINED3D_OK;
}

/* Do not call while under the GL lock. */
HRESULT CDECL wined3d_device_clear(struct wined3d_device *device, DWORD rect_count,
        const RECT *rects, DWORD flags, const struct wined3d_color *color, float depth, DWORD stencil)
{
    RECT draw_rect;

    TRACE("device %p, rect_count %u, rects %p, flags %#x, color {%.8e, %.8e, %.8e, %.8e}, depth %.8e, stencil %u.\n",
            device, rect_count, rects, flags, color->r, color->g, color->b, color->a, depth, stencil);

    if (flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL))
    {
        struct wined3d_surface *ds = device->fb.depth_stencil;
        if (!ds)
        {
            WARN("Clearing depth and/or stencil without a depth stencil buffer attached, returning WINED3DERR_INVALIDCALL\n");
            /* TODO: What about depth stencil buffers without stencil bits? */
            return WINED3DERR_INVALIDCALL;
        }
        else if (flags & WINED3DCLEAR_TARGET)
        {
            if (ds->resource.width < device->fb.render_targets[0]->resource.width
                    || ds->resource.height < device->fb.render_targets[0]->resource.height)
            {
                WARN("Silently ignoring depth and target clear with mismatching sizes\n");
                return WINED3D_OK;
            }
        }
    }

    wined3d_get_draw_rect(&device->stateBlock->state, &draw_rect);
    device_clear_render_targets(device, device->adapter->gl_info.limits.buffers,
            &device->fb, rect_count, rects, &draw_rect, flags, color, depth, stencil);

    return WINED3D_OK;
}

void CDECL wined3d_device_set_primitive_type(struct wined3d_device *device,
        enum wined3d_primitive_type primitive_type)
{
    TRACE("device %p, primitive_type %s\n", device, debug_d3dprimitivetype(primitive_type));

    device->updateStateBlock->changed.primitive_type = TRUE;
    device->updateStateBlock->state.gl_primitive_type = gl_primitive_type_from_d3d(primitive_type);
}

void CDECL wined3d_device_get_primitive_type(const struct wined3d_device *device,
        enum wined3d_primitive_type *primitive_type)
{
    TRACE("device %p, primitive_type %p\n", device, primitive_type);

    *primitive_type = d3d_primitive_type_from_gl(device->stateBlock->state.gl_primitive_type);

    TRACE("Returning %s\n", debug_d3dprimitivetype(*primitive_type));
}

HRESULT CDECL wined3d_device_draw_primitive(struct wined3d_device *device, UINT start_vertex, UINT vertex_count)
{
    TRACE("device %p, start_vertex %u, vertex_count %u.\n", device, start_vertex, vertex_count);

    if (!device->stateBlock->state.vertex_declaration)
    {
        WARN("Called without a valid vertex declaration set.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* The index buffer is not needed here, but restore it, otherwise it is hell to keep track of */
    if (device->stateBlock->state.user_stream)
    {
        device_invalidate_state(device, STATE_INDEXBUFFER);
        device->stateBlock->state.user_stream = FALSE;
    }

    if (device->stateBlock->state.load_base_vertex_index)
    {
        device->stateBlock->state.load_base_vertex_index = 0;
        device_invalidate_state(device, STATE_BASEVERTEXINDEX);
    }

    /* Account for the loading offset due to index buffers. Instead of
     * reloading all sources correct it with the startvertex parameter. */
    drawPrimitive(device, vertex_count, start_vertex, FALSE, NULL);
    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_draw_indexed_primitive(struct wined3d_device *device, UINT start_idx, UINT index_count)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;

    TRACE("device %p, start_idx %u, index_count %u.\n", device, start_idx, index_count);

    if (!device->stateBlock->state.index_buffer)
    {
        /* D3D9 returns D3DERR_INVALIDCALL when DrawIndexedPrimitive is called
         * without an index buffer set. (The first time at least...)
         * D3D8 simply dies, but I doubt it can do much harm to return
         * D3DERR_INVALIDCALL there as well. */
        WARN("Called without a valid index buffer set, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (!device->stateBlock->state.vertex_declaration)
    {
        WARN("Called without a valid vertex declaration set.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (device->stateBlock->state.user_stream)
    {
        device_invalidate_state(device, STATE_INDEXBUFFER);
        device->stateBlock->state.user_stream = FALSE;
    }

    if (!gl_info->supported[ARB_DRAW_ELEMENTS_BASE_VERTEX] &&
        device->stateBlock->state.load_base_vertex_index != device->stateBlock->state.base_vertex_index)
    {
        device->stateBlock->state.load_base_vertex_index = device->stateBlock->state.base_vertex_index;
        device_invalidate_state(device, STATE_BASEVERTEXINDEX);
    }

    drawPrimitive(device, index_count, start_idx, TRUE, NULL);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_draw_primitive_up(struct wined3d_device *device, UINT vertex_count,
        const void *stream_data, UINT stream_stride)
{
    struct wined3d_stream_state *stream;
    struct wined3d_buffer *vb;

    TRACE("device %p, vertex count %u, stream_data %p, stream_stride %u.\n",
            device, vertex_count, stream_data, stream_stride);

    if (!device->stateBlock->state.vertex_declaration)
    {
        WARN("Called without a valid vertex declaration set.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* Note in the following, it's not this type, but that's the purpose of streamIsUP */
    stream = &device->stateBlock->state.streams[0];
    vb = stream->buffer;
    stream->buffer = (struct wined3d_buffer *)stream_data;
    if (vb)
        wined3d_buffer_decref(vb);
    stream->offset = 0;
    stream->stride = stream_stride;
    device->stateBlock->state.user_stream = TRUE;
    if (device->stateBlock->state.load_base_vertex_index)
    {
        device->stateBlock->state.load_base_vertex_index = 0;
        device_invalidate_state(device, STATE_BASEVERTEXINDEX);
    }

    /* TODO: Only mark dirty if drawing from a different UP address */
    device_invalidate_state(device, STATE_STREAMSRC);

    drawPrimitive(device, vertex_count, 0, FALSE, NULL);

    /* MSDN specifies stream zero settings must be set to NULL */
    stream->buffer = NULL;
    stream->stride = 0;

    /* stream zero settings set to null at end, as per the msdn. No need to
     * mark dirty here, the app has to set the new stream sources or use UP
     * drawing again. */
    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_draw_indexed_primitive_up(struct wined3d_device *device,
        UINT index_count, const void *index_data, enum wined3d_format_id index_data_format_id,
        const void *stream_data, UINT stream_stride)
{
    struct wined3d_stream_state *stream;
    struct wined3d_buffer *vb, *ib;

    TRACE("device %p, index_count %u, index_data %p, index_data_format %s, stream_data %p, stream_stride %u.\n",
            device, index_count, index_data, debug_d3dformat(index_data_format_id), stream_data, stream_stride);

    if (!device->stateBlock->state.vertex_declaration)
    {
        WARN("(%p) : Called without a valid vertex declaration set\n", device);
        return WINED3DERR_INVALIDCALL;
    }

    stream = &device->stateBlock->state.streams[0];
    vb = stream->buffer;
    stream->buffer = (struct wined3d_buffer *)stream_data;
    if (vb)
        wined3d_buffer_decref(vb);
    stream->offset = 0;
    stream->stride = stream_stride;
    device->stateBlock->state.user_stream = TRUE;
    device->stateBlock->state.index_format = index_data_format_id;

    /* Set to 0 as per msdn. Do it now due to the stream source loading during drawPrimitive */
    device->stateBlock->state.base_vertex_index = 0;
    if (device->stateBlock->state.load_base_vertex_index)
    {
        device->stateBlock->state.load_base_vertex_index = 0;
        device_invalidate_state(device, STATE_BASEVERTEXINDEX);
    }
    /* Invalidate the state until we have nicer tracking of the stream source pointers */
    device_invalidate_state(device, STATE_STREAMSRC);
    device_invalidate_state(device, STATE_INDEXBUFFER);

    drawPrimitive(device, index_count, 0, TRUE, index_data);

    /* MSDN specifies stream zero settings and index buffer must be set to NULL */
    stream->buffer = NULL;
    stream->stride = 0;
    ib = device->stateBlock->state.index_buffer;
    if (ib)
    {
        wined3d_buffer_decref(ib);
        device->stateBlock->state.index_buffer = NULL;
    }
    /* No need to mark the stream source state dirty here. Either the app calls UP drawing again, or it has to call
     * SetStreamSource to specify a vertex buffer
     */

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_draw_primitive_strided(struct wined3d_device *device,
        UINT vertex_count, const struct wined3d_strided_data *strided_data)
{
    /* Mark the state dirty until we have nicer tracking. It's fine to change
     * baseVertexIndex because that call is only called by ddraw which does
     * not need that value. */
    device_invalidate_state(device, STATE_VDECL);
    device_invalidate_state(device, STATE_STREAMSRC);
    device_invalidate_state(device, STATE_INDEXBUFFER);

    device->stateBlock->state.base_vertex_index = 0;
    device->up_strided = strided_data;
    drawPrimitive(device, vertex_count, 0, FALSE, NULL);
    device->up_strided = NULL;

    /* Invalidate the states again to make sure the values from the stateblock
     * are properly applied in the next regular draw. Note that the application-
     * provided strided data has ovwritten pretty much the entire vertex and
     * and index stream related states */
    device_invalidate_state(device, STATE_VDECL);
    device_invalidate_state(device, STATE_STREAMSRC);
    device_invalidate_state(device, STATE_INDEXBUFFER);
    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_draw_indexed_primitive_strided(struct wined3d_device *device,
        UINT index_count, const struct wined3d_strided_data *strided_data,
        UINT vertex_count, const void *index_data, enum wined3d_format_id index_data_format_id)
{
    enum wined3d_format_id prev_idx_format;

    /* Mark the state dirty until we have nicer tracking
     * its fine to change baseVertexIndex because that call is only called by ddraw which does not need
     * that value.
     */
    device_invalidate_state(device, STATE_VDECL);
    device_invalidate_state(device, STATE_STREAMSRC);
    device_invalidate_state(device, STATE_INDEXBUFFER);

    prev_idx_format = device->stateBlock->state.index_format;
    device->stateBlock->state.index_format = index_data_format_id;
    device->stateBlock->state.user_stream = TRUE;
    device->stateBlock->state.base_vertex_index = 0;
    device->up_strided = strided_data;
    drawPrimitive(device, index_count, 0, TRUE, index_data);
    device->up_strided = NULL;
    device->stateBlock->state.index_format = prev_idx_format;

    device_invalidate_state(device, STATE_VDECL);
    device_invalidate_state(device, STATE_STREAMSRC);
    device_invalidate_state(device, STATE_INDEXBUFFER);
    return WINED3D_OK;
}

/* This is a helper function for UpdateTexture, there is no UpdateVolume method in D3D. */
static HRESULT device_update_volume(struct wined3d_device *device,
        struct wined3d_volume *src_volume, struct wined3d_volume *dst_volume)
{
    struct wined3d_map_desc src;
    struct wined3d_map_desc dst;
    HRESULT hr;

    TRACE("device %p, src_volume %p, dst_volume %p.\n",
            device, src_volume, dst_volume);

    /* TODO: Implement direct loading into the gl volume instead of using
     * memcpy and dirtification to improve loading performance. */
    if (FAILED(hr = wined3d_volume_map(src_volume, &src, NULL, WINED3D_MAP_READONLY)))
        return hr;
    if (FAILED(hr = wined3d_volume_map(dst_volume, &dst, NULL, WINED3D_MAP_DISCARD)))
    {
        wined3d_volume_unmap(src_volume);
        return hr;
    }

    memcpy(dst.data, src.data, dst_volume->resource.size);

    hr = wined3d_volume_unmap(dst_volume);
    if (FAILED(hr))
        wined3d_volume_unmap(src_volume);
    else
        hr = wined3d_volume_unmap(src_volume);

    return hr;
}

HRESULT CDECL wined3d_device_update_texture(struct wined3d_device *device,
        struct wined3d_texture *src_texture, struct wined3d_texture *dst_texture)
{
    enum wined3d_resource_type type;
    unsigned int level_count, i;
    HRESULT hr;

    TRACE("device %p, src_texture %p, dst_texture %p.\n", device, src_texture, dst_texture);

    /* Verify that the source and destination textures are non-NULL. */
    if (!src_texture || !dst_texture)
    {
        WARN("Source and destination textures must be non-NULL, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (src_texture == dst_texture)
    {
        WARN("Source and destination are the same object, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* Verify that the source and destination textures are the same type. */
    type = src_texture->resource.type;
    if (dst_texture->resource.type != type)
    {
        WARN("Source and destination have different types, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* Check that both textures have the identical numbers of levels. */
    level_count = wined3d_texture_get_level_count(src_texture);
    if (wined3d_texture_get_level_count(dst_texture) != level_count)
    {
        WARN("Source and destination have different level counts, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* Make sure that the destination texture is loaded. */
    dst_texture->texture_ops->texture_preload(dst_texture, SRGB_RGB);

    /* Update every surface level of the texture. */
    switch (type)
    {
        case WINED3D_RTYPE_TEXTURE:
        {
            struct wined3d_surface *src_surface;
            struct wined3d_surface *dst_surface;

            for (i = 0; i < level_count; ++i)
            {
                src_surface = surface_from_resource(wined3d_texture_get_sub_resource(src_texture, i));
                dst_surface = surface_from_resource(wined3d_texture_get_sub_resource(dst_texture, i));
                hr = wined3d_device_update_surface(device, src_surface, NULL, dst_surface, NULL);
                if (FAILED(hr))
                {
                    WARN("Failed to update surface, hr %#x.\n", hr);
                    return hr;
                }
            }
            break;
        }

        case WINED3D_RTYPE_CUBE_TEXTURE:
        {
            struct wined3d_surface *src_surface;
            struct wined3d_surface *dst_surface;

            for (i = 0; i < level_count * 6; ++i)
            {
                src_surface = surface_from_resource(wined3d_texture_get_sub_resource(src_texture, i));
                dst_surface = surface_from_resource(wined3d_texture_get_sub_resource(dst_texture, i));
                hr = wined3d_device_update_surface(device, src_surface, NULL, dst_surface, NULL);
                if (FAILED(hr))
                {
                    WARN("Failed to update surface, hr %#x.\n", hr);
                    return hr;
                }
            }
            break;
        }

        case WINED3D_RTYPE_VOLUME_TEXTURE:
        {
            for (i = 0; i < level_count; ++i)
            {
                hr = device_update_volume(device,
                        volume_from_resource(wined3d_texture_get_sub_resource(src_texture, i)),
                        volume_from_resource(wined3d_texture_get_sub_resource(dst_texture, i)));
                if (FAILED(hr))
                {
                    WARN("Failed to update volume, hr %#x.\n", hr);
                    return hr;
                }
            }
            break;
        }

        default:
            FIXME("Unsupported texture type %#x.\n", type);
            return WINED3DERR_INVALIDCALL;
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_front_buffer_data(const struct wined3d_device *device,
        UINT swapchain_idx, struct wined3d_surface *dst_surface)
{
    struct wined3d_swapchain *swapchain;
    HRESULT hr;

    TRACE("device %p, swapchain_idx %u, dst_surface %p.\n", device, swapchain_idx, dst_surface);

    hr = wined3d_device_get_swapchain(device, swapchain_idx, &swapchain);
    if (FAILED(hr)) return hr;

    hr = wined3d_swapchain_get_front_buffer_data(swapchain, dst_surface);
    wined3d_swapchain_decref(swapchain);

    return hr;
}

HRESULT CDECL wined3d_device_validate_device(const struct wined3d_device *device, DWORD *num_passes)
{
    const struct wined3d_state *state = &device->stateBlock->state;
    struct wined3d_texture *texture;
    DWORD i;

    TRACE("device %p, num_passes %p.\n", device, num_passes);

    for (i = 0; i < MAX_COMBINED_SAMPLERS; ++i)
    {
        if (state->sampler_states[i][WINED3D_SAMP_MIN_FILTER] == WINED3D_TEXF_NONE)
        {
            WARN("Sampler state %u has minfilter D3DTEXF_NONE, returning D3DERR_UNSUPPORTEDTEXTUREFILTER\n", i);
            return WINED3DERR_UNSUPPORTEDTEXTUREFILTER;
        }
        if (state->sampler_states[i][WINED3D_SAMP_MAG_FILTER] == WINED3D_TEXF_NONE)
        {
            WARN("Sampler state %u has magfilter D3DTEXF_NONE, returning D3DERR_UNSUPPORTEDTEXTUREFILTER\n", i);
            return WINED3DERR_UNSUPPORTEDTEXTUREFILTER;
        }

        texture = state->textures[i];
        if (!texture || texture->resource.format->flags & WINED3DFMT_FLAG_FILTERING) continue;

        if (state->sampler_states[i][WINED3D_SAMP_MAG_FILTER] != WINED3D_TEXF_POINT)
        {
            WARN("Non-filterable texture and mag filter enabled on samper %u, returning E_FAIL\n", i);
            return E_FAIL;
        }
        if (state->sampler_states[i][WINED3D_SAMP_MIN_FILTER] != WINED3D_TEXF_POINT)
        {
            WARN("Non-filterable texture and min filter enabled on samper %u, returning E_FAIL\n", i);
            return E_FAIL;
        }
        if (state->sampler_states[i][WINED3D_SAMP_MIP_FILTER] != WINED3D_TEXF_NONE
                && state->sampler_states[i][WINED3D_SAMP_MIP_FILTER] != WINED3D_TEXF_POINT)
        {
            WARN("Non-filterable texture and mip filter enabled on samper %u, returning E_FAIL\n", i);
            return E_FAIL;
        }
    }

    if (state->render_states[WINED3D_RS_ZENABLE] || state->render_states[WINED3D_RS_ZWRITEENABLE]
            || state->render_states[WINED3D_RS_STENCILENABLE])
    {
        struct wined3d_surface *ds = device->fb.depth_stencil;
        struct wined3d_surface *target = device->fb.render_targets[0];

        if(ds && target
                && (ds->resource.width < target->resource.width || ds->resource.height < target->resource.height))
        {
            WARN("Depth stencil is smaller than the color buffer, returning D3DERR_CONFLICTINGRENDERSTATE\n");
            return WINED3DERR_CONFLICTINGRENDERSTATE;
        }
    }

    /* return a sensible default */
    *num_passes = 1;

    TRACE("returning D3D_OK\n");
    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_software_vertex_processing(struct wined3d_device *device, BOOL software)
{
    static BOOL warned;

    TRACE("device %p, software %#x.\n", device, software);

    if (!warned)
    {
        FIXME("device %p, software %#x stub!\n", device, software);
        warned = TRUE;
    }

    device->softwareVertexProcessing = software;

    return WINED3D_OK;
}

BOOL CDECL wined3d_device_get_software_vertex_processing(const struct wined3d_device *device)
{
    static BOOL warned;

    TRACE("device %p.\n", device);

    if (!warned)
    {
        TRACE("device %p stub!\n", device);
        warned = TRUE;
    }

    return device->softwareVertexProcessing;
}

HRESULT CDECL wined3d_device_get_raster_status(const struct wined3d_device *device,
        UINT swapchain_idx, struct wined3d_raster_status *raster_status)
{
    struct wined3d_swapchain *swapchain;
    HRESULT hr;

    TRACE("device %p, swapchain_idx %u, raster_status %p.\n",
            device, swapchain_idx, raster_status);

    hr = wined3d_device_get_swapchain(device, swapchain_idx, &swapchain);
    if (FAILED(hr))
    {
        WARN("Failed to get swapchain %u, hr %#x.\n", swapchain_idx, hr);
        return hr;
    }

    hr = wined3d_swapchain_get_raster_status(swapchain, raster_status);
    wined3d_swapchain_decref(swapchain);
    if (FAILED(hr))
    {
        WARN("Failed to get raster status, hr %#x.\n", hr);
        return hr;
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_npatch_mode(struct wined3d_device *device, float segments)
{
    static BOOL warned;

    TRACE("device %p, segments %.8e.\n", device, segments);

    if (segments != 0.0f)
    {
        if (!warned)
        {
            FIXME("device %p, segments %.8e stub!\n", device, segments);
            warned = TRUE;
        }
    }

    return WINED3D_OK;
}

float CDECL wined3d_device_get_npatch_mode(const struct wined3d_device *device)
{
    static BOOL warned;

    TRACE("device %p.\n", device);

    if (!warned)
    {
        FIXME("device %p stub!\n", device);
        warned = TRUE;
    }

    return 0.0f;
}

HRESULT CDECL wined3d_device_update_surface(struct wined3d_device *device,
        struct wined3d_surface *src_surface, const RECT *src_rect,
        struct wined3d_surface *dst_surface, const POINT *dst_point)
{
    TRACE("device %p, src_surface %p, src_rect %s, dst_surface %p, dst_point %s.\n",
            device, src_surface, wine_dbgstr_rect(src_rect),
            dst_surface, wine_dbgstr_point(dst_point));

    if (src_surface->resource.pool != WINED3D_POOL_SYSTEM_MEM || dst_surface->resource.pool != WINED3D_POOL_DEFAULT)
    {
        WARN("source %p must be SYSTEMMEM and dest %p must be DEFAULT, returning WINED3DERR_INVALIDCALL\n",
                src_surface, dst_surface);
        return WINED3DERR_INVALIDCALL;
    }

    return surface_upload_from_surface(dst_surface, dst_point, src_surface, src_rect);
}

HRESULT CDECL wined3d_device_draw_rect_patch(struct wined3d_device *device, UINT handle,
        const float *num_segs, const struct wined3d_rect_patch_info *rect_patch_info)
{
    struct wined3d_rect_patch *patch;
    GLenum old_primitive_type;
    unsigned int i;
    struct list *e;
    BOOL found;

    TRACE("device %p, handle %#x, num_segs %p, rect_patch_info %p.\n",
            device, handle, num_segs, rect_patch_info);

    if (!(handle || rect_patch_info))
    {
        /* TODO: Write a test for the return value, thus the FIXME */
        FIXME("Both handle and rect_patch_info are NULL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (handle)
    {
        i = PATCHMAP_HASHFUNC(handle);
        found = FALSE;
        LIST_FOR_EACH(e, &device->patches[i])
        {
            patch = LIST_ENTRY(e, struct wined3d_rect_patch, entry);
            if (patch->Handle == handle)
            {
                found = TRUE;
                break;
            }
        }

        if (!found)
        {
            TRACE("Patch does not exist. Creating a new one\n");
            patch = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*patch));
            patch->Handle = handle;
            list_add_head(&device->patches[i], &patch->entry);
        } else {
            TRACE("Found existing patch %p\n", patch);
        }
    }
    else
    {
        /* Since opengl does not load tesselated vertex attributes into numbered vertex
         * attributes we have to tesselate, read back, and draw. This needs a patch
         * management structure instance. Create one.
         *
         * A possible improvement is to check if a vertex shader is used, and if not directly
         * draw the patch.
         */
        FIXME("Drawing an uncached patch. This is slow\n");
        patch = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*patch));
    }

    if (num_segs[0] != patch->numSegs[0] || num_segs[1] != patch->numSegs[1]
            || num_segs[2] != patch->numSegs[2] || num_segs[3] != patch->numSegs[3]
            || (rect_patch_info && memcmp(rect_patch_info, &patch->rect_patch_info, sizeof(*rect_patch_info))))
    {
        HRESULT hr;
        TRACE("Tesselation density or patch info changed, retesselating\n");

        if (rect_patch_info)
            patch->rect_patch_info = *rect_patch_info;

        patch->numSegs[0] = num_segs[0];
        patch->numSegs[1] = num_segs[1];
        patch->numSegs[2] = num_segs[2];
        patch->numSegs[3] = num_segs[3];

        hr = tesselate_rectpatch(device, patch);
        if (FAILED(hr))
        {
            WARN("Patch tesselation failed.\n");

            /* Do not release the handle to store the params of the patch */
            if (!handle)
                HeapFree(GetProcessHeap(), 0, patch);

            return hr;
        }
    }

    old_primitive_type = device->stateBlock->state.gl_primitive_type;
    device->stateBlock->state.gl_primitive_type = GL_TRIANGLES;
    wined3d_device_draw_primitive_strided(device, patch->numSegs[0] * patch->numSegs[1] * 2 * 3, &patch->strided);
    device->stateBlock->state.gl_primitive_type = old_primitive_type;

    /* Destroy uncached patches */
    if (!handle)
    {
        HeapFree(GetProcessHeap(), 0, patch->mem);
        HeapFree(GetProcessHeap(), 0, patch);
    }
    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_draw_tri_patch(struct wined3d_device *device, UINT handle,
        const float *segment_count, const struct wined3d_tri_patch_info *patch_info)
{
    FIXME("device %p, handle %#x, segment_count %p, patch_info %p stub!\n",
            device, handle, segment_count, patch_info);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_delete_patch(struct wined3d_device *device, UINT handle)
{
    struct wined3d_rect_patch *patch;
    struct list *e;
    int i;

    TRACE("device %p, handle %#x.\n", device, handle);

    i = PATCHMAP_HASHFUNC(handle);
    LIST_FOR_EACH(e, &device->patches[i])
    {
        patch = LIST_ENTRY(e, struct wined3d_rect_patch, entry);
        if (patch->Handle == handle)
        {
            TRACE("Deleting patch %p\n", patch);
            list_remove(&patch->entry);
            HeapFree(GetProcessHeap(), 0, patch->mem);
            HeapFree(GetProcessHeap(), 0, patch);
            return WINED3D_OK;
        }
    }

    /* TODO: Write a test for the return value */
    FIXME("Attempt to destroy nonexistent patch\n");
    return WINED3DERR_INVALIDCALL;
}

/* Do not call while under the GL lock. */
HRESULT CDECL wined3d_device_color_fill(struct wined3d_device *device,
        struct wined3d_surface *surface, const RECT *rect, const struct wined3d_color *color)
{
    RECT r;

    TRACE("device %p, surface %p, rect %s, color {%.8e, %.8e, %.8e, %.8e}.\n",
            device, surface, wine_dbgstr_rect(rect),
            color->r, color->g, color->b, color->a);

    if (surface->resource.pool != WINED3D_POOL_DEFAULT && surface->resource.pool != WINED3D_POOL_SYSTEM_MEM)
    {
        WARN("Color-fill not allowed on %s surfaces.\n", debug_d3dpool(surface->resource.pool));
        return WINED3DERR_INVALIDCALL;
    }

    if (!rect)
    {
        SetRect(&r, 0, 0, surface->resource.width, surface->resource.height);
        rect = &r;
    }

    return surface_color_fill(surface, rect, color);
}

/* Do not call while under the GL lock. */
void CDECL wined3d_device_clear_rendertarget_view(struct wined3d_device *device,
        struct wined3d_rendertarget_view *rendertarget_view, const struct wined3d_color *color)
{
    struct wined3d_resource *resource;
    HRESULT hr;
    RECT rect;

    resource = rendertarget_view->resource;
    if (resource->type != WINED3D_RTYPE_SURFACE)
    {
        FIXME("Only supported on surface resources\n");
        return;
    }

    SetRect(&rect, 0, 0, resource->width, resource->height);
    hr = surface_color_fill(surface_from_resource(resource), &rect, color);
    if (FAILED(hr)) ERR("Color fill failed, hr %#x.\n", hr);
}

HRESULT CDECL wined3d_device_get_render_target(const struct wined3d_device *device,
        UINT render_target_idx, struct wined3d_surface **render_target)
{
    TRACE("device %p, render_target_idx %u, render_target %p.\n",
            device, render_target_idx, render_target);

    if (render_target_idx >= device->adapter->gl_info.limits.buffers)
    {
        WARN("Only %u render targets are supported.\n", device->adapter->gl_info.limits.buffers);
        return WINED3DERR_INVALIDCALL;
    }

    *render_target = device->fb.render_targets[render_target_idx];
    TRACE("Returning render target %p.\n", *render_target);

    if (!*render_target)
        return WINED3DERR_NOTFOUND;

    wined3d_surface_incref(*render_target);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_depth_stencil(const struct wined3d_device *device,
        struct wined3d_surface **depth_stencil)
{
    TRACE("device %p, depth_stencil %p.\n", device, depth_stencil);

    *depth_stencil = device->fb.depth_stencil;
    TRACE("Returning depth/stencil surface %p.\n", *depth_stencil);

    if (!*depth_stencil)
        return WINED3DERR_NOTFOUND;

    wined3d_surface_incref(*depth_stencil);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_render_target(struct wined3d_device *device,
        UINT render_target_idx, struct wined3d_surface *render_target, BOOL set_viewport)
{
    struct wined3d_surface *prev;

    TRACE("device %p, render_target_idx %u, render_target %p, set_viewport %#x.\n",
            device, render_target_idx, render_target, set_viewport);

    if (render_target_idx >= device->adapter->gl_info.limits.buffers)
    {
        WARN("Only %u render targets are supported.\n", device->adapter->gl_info.limits.buffers);
        return WINED3DERR_INVALIDCALL;
    }

    prev = device->fb.render_targets[render_target_idx];
    if (render_target == prev)
    {
        TRACE("Trying to do a NOP SetRenderTarget operation.\n");
        return WINED3D_OK;
    }

    /* Render target 0 can't be set to NULL. */
    if (!render_target && !render_target_idx)
    {
        WARN("Trying to set render target 0 to NULL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (render_target && !(render_target->resource.usage & WINED3DUSAGE_RENDERTARGET))
    {
        FIXME("Surface %p doesn't have render target usage.\n", render_target);
        return WINED3DERR_INVALIDCALL;
    }

    if (render_target)
        wined3d_surface_incref(render_target);
    device->fb.render_targets[render_target_idx] = render_target;
    /* Release after the assignment, to prevent device_resource_released()
     * from seeing the surface as still in use. */
    if (prev)
        wined3d_surface_decref(prev);

    /* Render target 0 is special. */
    if (!render_target_idx && set_viewport)
    {
        /* Set the viewport and scissor rectangles, if requested. Tests show
         * that stateblock recording is ignored, the change goes directly
         * into the primary stateblock. */
        device->stateBlock->state.viewport.height = device->fb.render_targets[0]->resource.height;
        device->stateBlock->state.viewport.width  = device->fb.render_targets[0]->resource.width;
        device->stateBlock->state.viewport.x      = 0;
        device->stateBlock->state.viewport.y      = 0;
        device->stateBlock->state.viewport.max_z  = 1.0f;
        device->stateBlock->state.viewport.min_z  = 0.0f;
        device_invalidate_state(device, STATE_VIEWPORT);

        device->stateBlock->state.scissor_rect.top = 0;
        device->stateBlock->state.scissor_rect.left = 0;
        device->stateBlock->state.scissor_rect.right = device->stateBlock->state.viewport.width;
        device->stateBlock->state.scissor_rect.bottom = device->stateBlock->state.viewport.height;
        device_invalidate_state(device, STATE_SCISSORRECT);
    }

    device_invalidate_state(device, STATE_FRAMEBUFFER);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_depth_stencil(struct wined3d_device *device, struct wined3d_surface *depth_stencil)
{
    struct wined3d_surface *prev = device->fb.depth_stencil;

    TRACE("device %p, depth_stencil %p, old depth_stencil %p.\n",
            device, depth_stencil, prev);

    if (prev == depth_stencil)
    {
        TRACE("Trying to do a NOP SetRenderTarget operation.\n");
        return WINED3D_OK;
    }

    if (prev)
    {
        if (device->swapchains[0]->desc.flags & WINED3DPRESENTFLAG_DISCARD_DEPTHSTENCIL
                || prev->flags & SFLAG_DISCARD)
        {
            surface_modify_ds_location(prev, SFLAG_DISCARDED,
                    prev->resource.width, prev->resource.height);
            if (prev == device->onscreen_depth_stencil)
            {
                wined3d_surface_decref(device->onscreen_depth_stencil);
                device->onscreen_depth_stencil = NULL;
            }
        }
    }

    device->fb.depth_stencil = depth_stencil;
    if (depth_stencil)
        wined3d_surface_incref(depth_stencil);

    if (!prev != !depth_stencil)
    {
        /* Swapping NULL / non NULL depth stencil affects the depth and tests */
        device_invalidate_state(device, STATE_RENDER(WINED3D_RS_ZENABLE));
        device_invalidate_state(device, STATE_RENDER(WINED3D_RS_STENCILENABLE));
        device_invalidate_state(device, STATE_RENDER(WINED3D_RS_STENCILWRITEMASK));
        device_invalidate_state(device, STATE_RENDER(WINED3D_RS_DEPTHBIAS));
    }
    else if (prev && prev->resource.format->depth_size != depth_stencil->resource.format->depth_size)
    {
        device_invalidate_state(device, STATE_RENDER(WINED3D_RS_DEPTHBIAS));
    }
    if (prev)
        wined3d_surface_decref(prev);

    device_invalidate_state(device, STATE_FRAMEBUFFER);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_cursor_properties(struct wined3d_device *device,
        UINT x_hotspot, UINT y_hotspot, struct wined3d_surface *cursor_image)
{
    TRACE("device %p, x_hotspot %u, y_hotspot %u, cursor_image %p.\n",
            device, x_hotspot, y_hotspot, cursor_image);

    /* some basic validation checks */
    if (device->cursorTexture)
    {
        struct wined3d_context *context = context_acquire(device, NULL);
        ENTER_GL();
        context->gl_info->gl_ops.gl.p_glDeleteTextures(1, &device->cursorTexture);
        LEAVE_GL();
        context_release(context);
        device->cursorTexture = 0;
    }

    if (cursor_image)
    {
        struct wined3d_display_mode mode;
        struct wined3d_map_desc map_desc;
        HRESULT hr;

        /* MSDN: Cursor must be A8R8G8B8 */
        if (cursor_image->resource.format->id != WINED3DFMT_B8G8R8A8_UNORM)
        {
            WARN("surface %p has an invalid format.\n", cursor_image);
            return WINED3DERR_INVALIDCALL;
        }

        if (FAILED(hr = wined3d_get_adapter_display_mode(device->wined3d, device->adapter->ordinal, &mode, NULL)))
        {
            ERR("Failed to get display mode, hr %#x.\n", hr);
            return WINED3DERR_INVALIDCALL;
        }

        /* MSDN: Cursor must be smaller than the display mode */
        if (cursor_image->resource.width > mode.width || cursor_image->resource.height > mode.height)
        {
            WARN("Surface %p dimensions are %ux%u, but screen dimensions are %ux%u.\n",
                    cursor_image, cursor_image->resource.width, cursor_image->resource.height,
                    mode.width, mode.height);
            return WINED3DERR_INVALIDCALL;
        }

        /* TODO: MSDN: Cursor sizes must be a power of 2 */

        /* Do not store the surface's pointer because the application may
         * release it after setting the cursor image. Windows doesn't
         * addref the set surface, so we can't do this either without
         * creating circular refcount dependencies. Copy out the gl texture
         * instead. */
        device->cursorWidth = cursor_image->resource.width;
        device->cursorHeight = cursor_image->resource.height;
        if (SUCCEEDED(wined3d_surface_map(cursor_image, &map_desc, NULL, WINED3D_MAP_READONLY)))
        {
            const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
            const struct wined3d_format *format = wined3d_get_format(gl_info, WINED3DFMT_B8G8R8A8_UNORM);
            struct wined3d_context *context;
            char *mem, *bits = map_desc.data;
            GLint intfmt = format->glInternal;
            GLint gl_format = format->glFormat;
            GLint type = format->glType;
            INT height = device->cursorHeight;
            INT width = device->cursorWidth;
            INT bpp = format->byte_count;
            INT i;

            /* Reformat the texture memory (pitch and width can be
             * different) */
            mem = HeapAlloc(GetProcessHeap(), 0, width * height * bpp);
            for (i = 0; i < height; ++i)
                memcpy(&mem[width * bpp * i], &bits[map_desc.row_pitch * i], width * bpp);
            wined3d_surface_unmap(cursor_image);

            context = context_acquire(device, NULL);

            ENTER_GL();

            if (gl_info->supported[APPLE_CLIENT_STORAGE])
            {
                gl_info->gl_ops.gl.p_glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE);
                checkGLcall("glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_FALSE)");
            }

            invalidate_active_texture(device, context);
            /* Create a new cursor texture */
            gl_info->gl_ops.gl.p_glGenTextures(1, &device->cursorTexture);
            checkGLcall("glGenTextures");
            context_bind_texture(context, GL_TEXTURE_2D, device->cursorTexture);
            /* Copy the bitmap memory into the cursor texture */
            gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_2D, 0, intfmt, width, height, 0, gl_format, type, mem);
            checkGLcall("glTexImage2D");
            HeapFree(GetProcessHeap(), 0, mem);

            if (gl_info->supported[APPLE_CLIENT_STORAGE])
            {
                gl_info->gl_ops.gl.p_glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
                checkGLcall("glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE)");
            }

            LEAVE_GL();

            context_release(context);
        }
        else
        {
            FIXME("A cursor texture was not returned.\n");
            device->cursorTexture = 0;
        }

        if (cursor_image->resource.width == 32 && cursor_image->resource.height == 32)
        {
            UINT mask_size = cursor_image->resource.width * cursor_image->resource.height / 8;
            ICONINFO cursorInfo;
            DWORD *maskBits;
            HCURSOR cursor;

            /* 32-bit user32 cursors ignore the alpha channel if it's all
             * zeroes, and use the mask instead. Fill the mask with all ones
             * to ensure we still get a fully transparent cursor. */
            maskBits = HeapAlloc(GetProcessHeap(), 0, mask_size);
            memset(maskBits, 0xff, mask_size);
            wined3d_surface_map(cursor_image, &map_desc, NULL,
                    WINED3D_MAP_NO_DIRTY_UPDATE | WINED3D_MAP_READONLY);
            TRACE("width: %u height: %u.\n", cursor_image->resource.width, cursor_image->resource.height);

            cursorInfo.fIcon = FALSE;
            cursorInfo.xHotspot = x_hotspot;
            cursorInfo.yHotspot = y_hotspot;
            cursorInfo.hbmMask = CreateBitmap(cursor_image->resource.width, cursor_image->resource.height,
                    1, 1, maskBits);
            cursorInfo.hbmColor = CreateBitmap(cursor_image->resource.width, cursor_image->resource.height,
                    1, 32, map_desc.data);
            wined3d_surface_unmap(cursor_image);
            /* Create our cursor and clean up. */
            cursor = CreateIconIndirect(&cursorInfo);
            if (cursorInfo.hbmMask) DeleteObject(cursorInfo.hbmMask);
            if (cursorInfo.hbmColor) DeleteObject(cursorInfo.hbmColor);
            if (device->hardwareCursor) DestroyCursor(device->hardwareCursor);
            device->hardwareCursor = cursor;
            if (device->bCursorVisible) SetCursor( cursor );
            HeapFree(GetProcessHeap(), 0, maskBits);
        }
    }

    device->xHotSpot = x_hotspot;
    device->yHotSpot = y_hotspot;
    return WINED3D_OK;
}

void CDECL wined3d_device_set_cursor_position(struct wined3d_device *device,
        int x_screen_space, int y_screen_space, DWORD flags)
{
    TRACE("device %p, x %d, y %d, flags %#x.\n",
            device, x_screen_space, y_screen_space, flags);

    device->xScreenSpace = x_screen_space;
    device->yScreenSpace = y_screen_space;

    if (device->hardwareCursor)
    {
        POINT pt;

        GetCursorPos( &pt );
        if (x_screen_space == pt.x && y_screen_space == pt.y)
            return;
        SetCursorPos( x_screen_space, y_screen_space );

        /* Switch to the software cursor if position diverges from the hardware one. */
        GetCursorPos( &pt );
        if (x_screen_space != pt.x || y_screen_space != pt.y)
        {
            if (device->bCursorVisible) SetCursor( NULL );
            DestroyCursor( device->hardwareCursor );
            device->hardwareCursor = 0;
        }
    }
}

BOOL CDECL wined3d_device_show_cursor(struct wined3d_device *device, BOOL show)
{
    BOOL oldVisible = device->bCursorVisible;

    TRACE("device %p, show %#x.\n", device, show);

    /*
     * When ShowCursor is first called it should make the cursor appear at the OS's last
     * known cursor position.
     */
    if (show && !oldVisible)
    {
        POINT pt;
        GetCursorPos(&pt);
        device->xScreenSpace = pt.x;
        device->yScreenSpace = pt.y;
    }

    if (device->hardwareCursor)
    {
        device->bCursorVisible = show;
        if (show)
            SetCursor(device->hardwareCursor);
        else
            SetCursor(NULL);
    }
    else
    {
        if (device->cursorTexture)
            device->bCursorVisible = show;
    }

    return oldVisible;
}

void CDECL wined3d_device_evict_managed_resources(struct wined3d_device *device)
{
    struct wined3d_resource *resource, *cursor;

    TRACE("device %p.\n", device);

    LIST_FOR_EACH_ENTRY_SAFE(resource, cursor, &device->resources, struct wined3d_resource, resource_list_entry)
    {
        TRACE("Checking resource %p for eviction.\n", resource);

        if (resource->pool == WINED3D_POOL_MANAGED && !resource->map_count)
        {
            TRACE("Evicting %p.\n", resource);
            resource->resource_ops->resource_unload(resource);
        }
    }

    /* Invalidate stream sources, the buffer(s) may have been evicted. */
    device_invalidate_state(device, STATE_STREAMSRC);
}

/* Do not call while under the GL lock. */
static void delete_opengl_contexts(struct wined3d_device *device, struct wined3d_swapchain *swapchain)
{
    struct wined3d_resource *resource, *cursor;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    struct wined3d_shader *shader;

    context = context_acquire(device, NULL);
    gl_info = context->gl_info;

    LIST_FOR_EACH_ENTRY_SAFE(resource, cursor, &device->resources, struct wined3d_resource, resource_list_entry)
    {
        TRACE("Unloading resource %p.\n", resource);

        resource->resource_ops->resource_unload(resource);
    }

    LIST_FOR_EACH_ENTRY(shader, &device->shaders, struct wined3d_shader, shader_list_entry)
    {
        device->shader_backend->shader_destroy(shader);
    }

    ENTER_GL();
    if (device->depth_blt_texture)
    {
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &device->depth_blt_texture);
        device->depth_blt_texture = 0;
    }
    if (device->cursorTexture)
    {
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &device->cursorTexture);
        device->cursorTexture = 0;
    }
    LEAVE_GL();

    device->blitter->free_private(device);
    device->frag_pipe->free_private(device);
    device->shader_backend->shader_free_private(device);
    destroy_dummy_textures(device, gl_info);

    context_release(context);

    while (device->context_count)
    {
        swapchain_destroy_contexts(device->contexts[0]->swapchain);
    }

    HeapFree(GetProcessHeap(), 0, swapchain->context);
    swapchain->context = NULL;
}

/* Do not call while under the GL lock. */
static HRESULT create_primary_opengl_context(struct wined3d_device *device, struct wined3d_swapchain *swapchain)
{
    struct wined3d_context *context;
    struct wined3d_surface *target;
    HRESULT hr;

    /* Recreate the primary swapchain's context */
    swapchain->context = HeapAlloc(GetProcessHeap(), 0, sizeof(*swapchain->context));
    if (!swapchain->context)
    {
        ERR("Failed to allocate memory for swapchain context array.\n");
        return E_OUTOFMEMORY;
    }

    target = swapchain->back_buffers ? swapchain->back_buffers[0] : swapchain->front_buffer;
    if (!(context = context_create(swapchain, target, swapchain->ds_format)))
    {
        WARN("Failed to create context.\n");
        HeapFree(GetProcessHeap(), 0, swapchain->context);
        return E_FAIL;
    }

    swapchain->context[0] = context;
    swapchain->num_contexts = 1;
    create_dummy_textures(device, context);
    context_release(context);

    hr = device->shader_backend->shader_alloc_private(device);
    if (FAILED(hr))
    {
        ERR("Failed to allocate shader private data, hr %#x.\n", hr);
        goto err;
    }

    hr = device->frag_pipe->alloc_private(device);
    if (FAILED(hr))
    {
        ERR("Failed to allocate fragment pipe private data, hr %#x.\n", hr);
        device->shader_backend->shader_free_private(device);
        goto err;
    }

    hr = device->blitter->alloc_private(device);
    if (FAILED(hr))
    {
        ERR("Failed to allocate blitter private data, hr %#x.\n", hr);
        device->frag_pipe->free_private(device);
        device->shader_backend->shader_free_private(device);
        goto err;
    }

    return WINED3D_OK;

err:
    context_acquire(device, NULL);
    destroy_dummy_textures(device, context->gl_info);
    context_release(context);
    context_destroy(device, context);
    HeapFree(GetProcessHeap(), 0, swapchain->context);
    swapchain->num_contexts = 0;
    return hr;
}

/* Do not call while under the GL lock. */
HRESULT CDECL wined3d_device_reset(struct wined3d_device *device,
        const struct wined3d_swapchain_desc *swapchain_desc, const struct wined3d_display_mode *mode,
        wined3d_device_reset_cb callback)
{
    struct wined3d_resource *resource, *cursor;
    struct wined3d_swapchain *swapchain;
    struct wined3d_display_mode m;
    BOOL DisplayModeChanged = FALSE;
    BOOL update_desc = FALSE;
    unsigned int i;
    HRESULT hr;

    TRACE("device %p, swapchain_desc %p, mode %p, callback %p.\n", device, swapchain_desc, mode, callback);

    if (FAILED(hr = wined3d_device_get_swapchain(device, 0, &swapchain)))
    {
        ERR("Failed to get the first implicit swapchain.\n");
        return hr;
    }

    stateblock_unbind_resources(device->stateBlock);
    if (swapchain->back_buffers && swapchain->back_buffers[0])
        wined3d_device_set_render_target(device, 0, swapchain->back_buffers[0], FALSE);
    else
        wined3d_device_set_render_target(device, 0, swapchain->front_buffer, FALSE);
    for (i = 1; i < device->adapter->gl_info.limits.buffers; ++i)
    {
        wined3d_device_set_render_target(device, i, NULL, FALSE);
    }
    wined3d_device_set_depth_stencil(device, NULL);

    if (device->onscreen_depth_stencil)
    {
        wined3d_surface_decref(device->onscreen_depth_stencil);
        device->onscreen_depth_stencil = NULL;
    }

    LIST_FOR_EACH_ENTRY_SAFE(resource, cursor, &device->resources, struct wined3d_resource, resource_list_entry)
    {
        TRACE("Enumerating resource %p.\n", resource);
        if (FAILED(hr = callback(resource)))
        {
            wined3d_swapchain_decref(swapchain);
            return hr;
        }
    }

    /* Is it necessary to recreate the gl context? Actually every setting can be changed
     * on an existing gl context, so there's no real need for recreation.
     *
     * TODO: Figure out how Reset influences resources in D3DPOOL_DEFAULT, D3DPOOL_SYSTEMMEMORY and D3DPOOL_MANAGED
     *
     * TODO: Figure out what happens to explicit swapchains, or if we have more than one implicit swapchain
     */
    TRACE("New params:\n");
    TRACE("backbuffer_width %u\n", swapchain_desc->backbuffer_width);
    TRACE("backbuffer_height %u\n", swapchain_desc->backbuffer_height);
    TRACE("backbuffer_format %s\n", debug_d3dformat(swapchain_desc->backbuffer_format));
    TRACE("backbuffer_count %u\n", swapchain_desc->backbuffer_count);
    TRACE("multisample_type %#x\n", swapchain_desc->multisample_type);
    TRACE("multisample_quality %u\n", swapchain_desc->multisample_quality);
    TRACE("swap_effect %#x\n", swapchain_desc->swap_effect);
    TRACE("device_window %p\n", swapchain_desc->device_window);
    TRACE("windowed %#x\n", swapchain_desc->windowed);
    TRACE("enable_auto_depth_stencil %#x\n", swapchain_desc->enable_auto_depth_stencil);
    if (swapchain_desc->enable_auto_depth_stencil)
        TRACE("auto_depth_stencil_format %s\n", debug_d3dformat(swapchain_desc->auto_depth_stencil_format));
    TRACE("flags %#x\n", swapchain_desc->flags);
    TRACE("refresh_rate %u\n", swapchain_desc->refresh_rate);
    TRACE("swap_interval %u\n", swapchain_desc->swap_interval);
    TRACE("auto_restore_display_mode %#x\n", swapchain_desc->auto_restore_display_mode);

    /* No special treatment of these parameters. Just store them */
    swapchain->desc.swap_effect = swapchain_desc->swap_effect;
    swapchain->desc.flags = swapchain_desc->flags;
    swapchain->desc.swap_interval = swapchain_desc->swap_interval;
    swapchain->desc.refresh_rate = swapchain_desc->refresh_rate;

    /* What to do about these? */
    if (swapchain_desc->backbuffer_count
            && swapchain_desc->backbuffer_count != swapchain->desc.backbuffer_count)
        FIXME("Cannot change the back buffer count yet.\n");

    if (swapchain_desc->device_window
            && swapchain_desc->device_window != swapchain->desc.device_window)
    {
        TRACE("Changing the device window from %p to %p.\n",
                swapchain->desc.device_window, swapchain_desc->device_window);
        swapchain->desc.device_window = swapchain_desc->device_window;
        swapchain->device_window = swapchain_desc->device_window;
        wined3d_swapchain_set_window(swapchain, NULL);
    }

    if (swapchain_desc->enable_auto_depth_stencil && !device->auto_depth_stencil)
    {
        HRESULT hr;

        TRACE("Creating the depth stencil buffer\n");

        if (FAILED(hr = device->device_parent->ops->create_swapchain_surface(device->device_parent,
                device->device_parent, swapchain_desc->backbuffer_width, swapchain_desc->backbuffer_height,
                swapchain_desc->auto_depth_stencil_format, WINED3DUSAGE_DEPTHSTENCIL,
                swapchain_desc->multisample_type, swapchain_desc->multisample_quality,
                &device->auto_depth_stencil)))
        {
            ERR("Failed to create the depth stencil buffer, hr %#x.\n", hr);
            wined3d_swapchain_decref(swapchain);
            return WINED3DERR_INVALIDCALL;
        }
    }

    /* Reset the depth stencil */
    if (swapchain_desc->enable_auto_depth_stencil)
        wined3d_device_set_depth_stencil(device, device->auto_depth_stencil);

    if (mode)
    {
        DisplayModeChanged = TRUE;
        m = *mode;
    }
    else if (swapchain_desc->windowed)
    {
        m.width = swapchain->orig_width;
        m.height = swapchain->orig_height;
        m.refresh_rate = 0;
        m.format_id = swapchain->desc.backbuffer_format;
        m.scanline_ordering = WINED3D_SCANLINE_ORDERING_UNKNOWN;
    }
    else
    {
        m.width = swapchain_desc->backbuffer_width;
        m.height = swapchain_desc->backbuffer_height;
        m.refresh_rate = swapchain_desc->refresh_rate;
        m.format_id = swapchain_desc->backbuffer_format;
        m.scanline_ordering = WINED3D_SCANLINE_ORDERING_UNKNOWN;
    }

    /* Should Width == 800 && Height == 0 set 800x600? */
    if (swapchain_desc->backbuffer_width && swapchain_desc->backbuffer_height
            && (swapchain_desc->backbuffer_width != swapchain->desc.backbuffer_width
            || swapchain_desc->backbuffer_height != swapchain->desc.backbuffer_height))
    {
        if (!swapchain_desc->windowed)
            DisplayModeChanged = TRUE;

        swapchain->desc.backbuffer_width = swapchain_desc->backbuffer_width;
        swapchain->desc.backbuffer_height = swapchain_desc->backbuffer_height;
        update_desc = TRUE;
    }

    if (swapchain_desc->backbuffer_format != WINED3DFMT_UNKNOWN
            && swapchain_desc->backbuffer_format != swapchain->desc.backbuffer_format)
    {
        swapchain->desc.backbuffer_format = swapchain_desc->backbuffer_format;
        update_desc = TRUE;
    }

    if (swapchain_desc->multisample_type != swapchain->desc.multisample_type
            || swapchain_desc->multisample_quality != swapchain->desc.multisample_quality)
    {
        swapchain->desc.multisample_type = swapchain_desc->multisample_type;
        swapchain->desc.multisample_quality = swapchain_desc->multisample_quality;
        update_desc = TRUE;
    }

    if (update_desc)
    {
        UINT i;

        hr = wined3d_surface_update_desc(swapchain->front_buffer, swapchain->desc.backbuffer_width,
                swapchain->desc.backbuffer_height, swapchain->desc.backbuffer_format,
                swapchain->desc.multisample_type, swapchain->desc.multisample_quality);
        if (FAILED(hr))
        {
            wined3d_swapchain_decref(swapchain);
            return hr;
        }

        for (i = 0; i < swapchain->desc.backbuffer_count; ++i)
        {
            hr = wined3d_surface_update_desc(swapchain->back_buffers[i], swapchain->desc.backbuffer_width,
                    swapchain->desc.backbuffer_height, swapchain->desc.backbuffer_format,
                    swapchain->desc.multisample_type, swapchain->desc.multisample_quality);
            if (FAILED(hr))
            {
                wined3d_swapchain_decref(swapchain);
                return hr;
            }
        }
        if (device->auto_depth_stencil)
        {
            hr = wined3d_surface_update_desc(device->auto_depth_stencil, swapchain->desc.backbuffer_width,
                    swapchain->desc.backbuffer_height, device->auto_depth_stencil->resource.format->id,
                    swapchain->desc.multisample_type, swapchain->desc.multisample_quality);
            if (FAILED(hr))
            {
                wined3d_swapchain_decref(swapchain);
                return hr;
            }
        }
    }

    if (!swapchain_desc->windowed != !swapchain->desc.windowed
            || DisplayModeChanged)
    {
        if (FAILED(hr = wined3d_set_adapter_display_mode(device->wined3d, device->adapter->ordinal, &m)))
        {
            WARN("Failed to set display mode, hr %#x.\n", hr);
            wined3d_swapchain_decref(swapchain);
            return WINED3DERR_INVALIDCALL;
        }

        if (!swapchain_desc->windowed)
        {
            if (swapchain->desc.windowed)
            {
                HWND focus_window = device->create_parms.focus_window;
                if (!focus_window)
                    focus_window = swapchain_desc->device_window;
                if (FAILED(hr = wined3d_device_acquire_focus_window(device, focus_window)))
                {
                    ERR("Failed to acquire focus window, hr %#x.\n", hr);
                    wined3d_swapchain_decref(swapchain);
                    return hr;
                }

                /* switch from windowed to fs */
                wined3d_device_setup_fullscreen_window(device, swapchain->device_window,
                        swapchain_desc->backbuffer_width,
                        swapchain_desc->backbuffer_height);
            }
            else
            {
                /* Fullscreen -> fullscreen mode change */
                MoveWindow(swapchain->device_window, 0, 0,
                        swapchain_desc->backbuffer_width,
                        swapchain_desc->backbuffer_height,
                        TRUE);
            }
        }
        else if (!swapchain->desc.windowed)
        {
            /* Fullscreen -> windowed switch */
            wined3d_device_restore_fullscreen_window(device, swapchain->device_window);
            wined3d_device_release_focus_window(device);
        }
        swapchain->desc.windowed = swapchain_desc->windowed;
    }
    else if (!swapchain_desc->windowed)
    {
        DWORD style = device->style;
        DWORD exStyle = device->exStyle;
        /* If we're in fullscreen, and the mode wasn't changed, we have to get the window back into
         * the right position. Some applications(Battlefield 2, Guild Wars) move it and then call
         * Reset to clear up their mess. Guild Wars also loses the device during that.
         */
        device->style = 0;
        device->exStyle = 0;
        wined3d_device_setup_fullscreen_window(device, swapchain->device_window,
                swapchain_desc->backbuffer_width,
                swapchain_desc->backbuffer_height);
        device->style = style;
        device->exStyle = exStyle;
    }

    TRACE("Resetting stateblock.\n");
    wined3d_stateblock_decref(device->updateStateBlock);
    wined3d_stateblock_decref(device->stateBlock);

    if (device->d3d_initialized)
        delete_opengl_contexts(device, swapchain);

    /* Note: No parent needed for initial internal stateblock */
    hr = wined3d_stateblock_create(device, WINED3D_SBT_INIT, &device->stateBlock);
    if (FAILED(hr))
        ERR("Resetting the stateblock failed with error %#x.\n", hr);
    else
        TRACE("Created stateblock %p.\n", device->stateBlock);
    device->updateStateBlock = device->stateBlock;
    wined3d_stateblock_incref(device->updateStateBlock);

    stateblock_init_default_state(device->stateBlock);

    swapchain_update_render_to_fbo(swapchain);
    swapchain_update_draw_bindings(swapchain);

    if (device->d3d_initialized)
        hr = create_primary_opengl_context(device, swapchain);
    wined3d_swapchain_decref(swapchain);

    /* All done. There is no need to reload resources or shaders, this will happen automatically on the
     * first use
     */
    return hr;
}

HRESULT CDECL wined3d_device_set_dialog_box_mode(struct wined3d_device *device, BOOL enable_dialogs)
{
    TRACE("device %p, enable_dialogs %#x.\n", device, enable_dialogs);

    if (!enable_dialogs) FIXME("Dialogs cannot be disabled yet.\n");

    return WINED3D_OK;
}


HRESULT CDECL wined3d_device_get_creation_parameters(const struct wined3d_device *device,
        struct wined3d_device_creation_parameters *parameters)
{
    TRACE("device %p, parameters %p.\n", device, parameters);

    *parameters = device->create_parms;
    return WINED3D_OK;
}

void CDECL wined3d_device_set_gamma_ramp(const struct wined3d_device *device,
        UINT swapchain_idx, DWORD flags, const struct wined3d_gamma_ramp *ramp)
{
    struct wined3d_swapchain *swapchain;

    TRACE("device %p, swapchain_idx %u, flags %#x, ramp %p.\n",
            device, swapchain_idx, flags, ramp);

    if (SUCCEEDED(wined3d_device_get_swapchain(device, swapchain_idx, &swapchain)))
    {
        wined3d_swapchain_set_gamma_ramp(swapchain, flags, ramp);
        wined3d_swapchain_decref(swapchain);
    }
}

void CDECL wined3d_device_get_gamma_ramp(const struct wined3d_device *device,
        UINT swapchain_idx, struct wined3d_gamma_ramp *ramp)
{
    struct wined3d_swapchain *swapchain;

    TRACE("device %p, swapchain_idx %u, ramp %p.\n",
            device, swapchain_idx, ramp);

    if (SUCCEEDED(wined3d_device_get_swapchain(device, swapchain_idx, &swapchain)))
    {
        wined3d_swapchain_get_gamma_ramp(swapchain, ramp);
        wined3d_swapchain_decref(swapchain);
    }
}

void device_resource_add(struct wined3d_device *device, struct wined3d_resource *resource)
{
    TRACE("device %p, resource %p.\n", device, resource);

    list_add_head(&device->resources, &resource->resource_list_entry);
}

static void device_resource_remove(struct wined3d_device *device, struct wined3d_resource *resource)
{
    TRACE("device %p, resource %p.\n", device, resource);

    list_remove(&resource->resource_list_entry);
}

void device_resource_released(struct wined3d_device *device, struct wined3d_resource *resource)
{
    enum wined3d_resource_type type = resource->type;
    unsigned int i;

    TRACE("device %p, resource %p, type %s.\n", device, resource, debug_d3dresourcetype(type));

    context_resource_released(device, resource, type);

    switch (type)
    {
        case WINED3D_RTYPE_SURFACE:
            {
                struct wined3d_surface *surface = surface_from_resource(resource);

                if (!device->d3d_initialized) break;

                for (i = 0; i < device->adapter->gl_info.limits.buffers; ++i)
                {
                    if (device->fb.render_targets[i] == surface)
                    {
                        ERR("Surface %p is still in use as render target %u.\n", surface, i);
                        device->fb.render_targets[i] = NULL;
                    }
                }

                if (device->fb.depth_stencil == surface)
                {
                    ERR("Surface %p is still in use as depth/stencil buffer.\n", surface);
                    device->fb.depth_stencil = NULL;
                }
            }
            break;

        case WINED3D_RTYPE_TEXTURE:
        case WINED3D_RTYPE_CUBE_TEXTURE:
        case WINED3D_RTYPE_VOLUME_TEXTURE:
            for (i = 0; i < MAX_COMBINED_SAMPLERS; ++i)
            {
                struct wined3d_texture *texture = wined3d_texture_from_resource(resource);

                if (device->stateBlock && device->stateBlock->state.textures[i] == texture)
                {
                    ERR("Texture %p is still in use by stateblock %p, stage %u.\n",
                            texture, device->stateBlock, i);
                    device->stateBlock->state.textures[i] = NULL;
                }

                if (device->updateStateBlock != device->stateBlock
                        && device->updateStateBlock->state.textures[i] == texture)
                {
                    ERR("Texture %p is still in use by stateblock %p, stage %u.\n",
                            texture, device->updateStateBlock, i);
                    device->updateStateBlock->state.textures[i] = NULL;
                }
            }
            break;

        case WINED3D_RTYPE_BUFFER:
            {
                struct wined3d_buffer *buffer = buffer_from_resource(resource);

                for (i = 0; i < MAX_STREAMS; ++i)
                {
                    if (device->stateBlock && device->stateBlock->state.streams[i].buffer == buffer)
                    {
                        ERR("Buffer %p is still in use by stateblock %p, stream %u.\n",
                                buffer, device->stateBlock, i);
                        device->stateBlock->state.streams[i].buffer = NULL;
                    }

                    if (device->updateStateBlock != device->stateBlock
                            && device->updateStateBlock->state.streams[i].buffer == buffer)
                    {
                        ERR("Buffer %p is still in use by stateblock %p, stream %u.\n",
                                buffer, device->updateStateBlock, i);
                        device->updateStateBlock->state.streams[i].buffer = NULL;
                    }

                }

                if (device->stateBlock && device->stateBlock->state.index_buffer == buffer)
                {
                    ERR("Buffer %p is still in use by stateblock %p as index buffer.\n",
                            buffer, device->stateBlock);
                    device->stateBlock->state.index_buffer =  NULL;
                }

                if (device->updateStateBlock != device->stateBlock
                        && device->updateStateBlock->state.index_buffer == buffer)
                {
                    ERR("Buffer %p is still in use by stateblock %p as index buffer.\n",
                            buffer, device->updateStateBlock);
                    device->updateStateBlock->state.index_buffer =  NULL;
                }
            }
            break;

        default:
            break;
    }

    /* Remove the resource from the resourceStore */
    device_resource_remove(device, resource);

    TRACE("Resource released.\n");
}

HRESULT CDECL wined3d_device_get_surface_from_dc(const struct wined3d_device *device,
        HDC dc, struct wined3d_surface **surface)
{
    struct wined3d_resource *resource;

    TRACE("device %p, dc %p, surface %p.\n", device, dc, surface);

    if (!dc)
        return WINED3DERR_INVALIDCALL;

    LIST_FOR_EACH_ENTRY(resource, &device->resources, struct wined3d_resource, resource_list_entry)
    {
        if (resource->type == WINED3D_RTYPE_SURFACE)
        {
            struct wined3d_surface *s = surface_from_resource(resource);

            if (s->hDC == dc)
            {
                TRACE("Found surface %p for dc %p.\n", s, dc);
                *surface = s;
                return WINED3D_OK;
            }
        }
    }

    return WINED3DERR_INVALIDCALL;
}

HRESULT device_init(struct wined3d_device *device, struct wined3d *wined3d,
        UINT adapter_idx, enum wined3d_device_type device_type, HWND focus_window, DWORD flags,
        BYTE surface_alignment, struct wined3d_device_parent *device_parent)
{
    struct wined3d_adapter *adapter = &wined3d->adapters[adapter_idx];
    const struct fragment_pipeline *fragment_pipeline;
    struct shader_caps shader_caps;
    struct fragment_caps ffp_caps;
    unsigned int i;
    HRESULT hr;

    device->ref = 1;
    device->wined3d = wined3d;
    wined3d_incref(device->wined3d);
    device->adapter = wined3d->adapter_count ? adapter : NULL;
    device->device_parent = device_parent;
    list_init(&device->resources);
    list_init(&device->shaders);
    device->surface_alignment = surface_alignment;

    /* Save the creation parameters. */
    device->create_parms.adapter_idx = adapter_idx;
    device->create_parms.device_type = device_type;
    device->create_parms.focus_window = focus_window;
    device->create_parms.flags = flags;

    for (i = 0; i < PATCHMAP_SIZE; ++i) list_init(&device->patches[i]);

    select_shader_mode(&adapter->gl_info, &device->ps_selected_mode, &device->vs_selected_mode);
    device->shader_backend = adapter->shader_backend;

    if (device->shader_backend)
    {
        device->shader_backend->shader_get_caps(&adapter->gl_info, &shader_caps);
        device->vshader_version = shader_caps.VertexShaderVersion;
        device->pshader_version = shader_caps.PixelShaderVersion;
        device->d3d_vshader_constantF = shader_caps.MaxVertexShaderConst;
        device->d3d_pshader_constantF = shader_caps.MaxPixelShaderConst;
        device->vs_clipping = shader_caps.VSClipping;
    }
    fragment_pipeline = adapter->fragment_pipe;
    device->frag_pipe = fragment_pipeline;
    if (fragment_pipeline)
    {
        fragment_pipeline->get_caps(&adapter->gl_info, &ffp_caps);
        device->max_ffp_textures = ffp_caps.MaxSimultaneousTextures;

        hr = compile_state_table(device->StateTable, device->multistate_funcs, &adapter->gl_info,
                                 ffp_vertexstate_template, fragment_pipeline, misc_state_template);
        if (FAILED(hr))
        {
            ERR("Failed to compile state table, hr %#x.\n", hr);
            wined3d_decref(device->wined3d);
            return hr;
        }
    }
    device->blitter = adapter->blitter;

    hr = wined3d_stateblock_create(device, WINED3D_SBT_INIT, &device->stateBlock);
    if (FAILED(hr))
    {
        WARN("Failed to create stateblock.\n");
        for (i = 0; i < sizeof(device->multistate_funcs) / sizeof(device->multistate_funcs[0]); ++i)
        {
            HeapFree(GetProcessHeap(), 0, device->multistate_funcs[i]);
        }
        wined3d_decref(device->wined3d);
        return hr;
    }

    TRACE("Created stateblock %p.\n", device->stateBlock);
    device->updateStateBlock = device->stateBlock;
    wined3d_stateblock_incref(device->updateStateBlock);

    return WINED3D_OK;
}


void device_invalidate_state(const struct wined3d_device *device, DWORD state)
{
    DWORD rep = device->StateTable[state].representative;
    struct wined3d_context *context;
    DWORD idx;
    BYTE shift;
    UINT i;

    for (i = 0; i < device->context_count; ++i)
    {
        context = device->contexts[i];
        if(isStateDirty(context, rep)) continue;

        context->dirtyArray[context->numDirtyEntries++] = rep;
        idx = rep / (sizeof(*context->isStateDirty) * CHAR_BIT);
        shift = rep & ((sizeof(*context->isStateDirty) * CHAR_BIT) - 1);
        context->isStateDirty[idx] |= (1 << shift);
    }
}

void get_drawable_size_fbo(const struct wined3d_context *context, UINT *width, UINT *height)
{
    /* The drawable size of a fbo target is the opengl texture size, which is the power of two size. */
    *width = context->current_rt->pow2Width;
    *height = context->current_rt->pow2Height;
}

void get_drawable_size_backbuffer(const struct wined3d_context *context, UINT *width, UINT *height)
{
    const struct wined3d_swapchain *swapchain = context->swapchain;
    /* The drawable size of a backbuffer / aux buffer offscreen target is the size of the
     * current context's drawable, which is the size of the back buffer of the swapchain
     * the active context belongs to. */
    *width = swapchain->desc.backbuffer_width;
    *height = swapchain->desc.backbuffer_height;
}

LRESULT device_process_message(struct wined3d_device *device, HWND window, BOOL unicode,
        UINT message, WPARAM wparam, LPARAM lparam, WNDPROC proc)
{
    if (device->filter_messages)
    {
        TRACE("Filtering message: window %p, message %#x, wparam %#lx, lparam %#lx.\n",
                window, message, wparam, lparam);
        if (unicode)
            return DefWindowProcW(window, message, wparam, lparam);
        else
            return DefWindowProcA(window, message, wparam, lparam);
    }

    if (message == WM_DESTROY)
    {
        TRACE("unregister window %p.\n", window);
        wined3d_unregister_window(window);

        if (InterlockedCompareExchangePointer((void **)&device->focus_window, NULL, window) != window)
            ERR("Window %p is not the focus window for device %p.\n", window, device);
    }
    else if (message == WM_DISPLAYCHANGE)
    {
        device->device_parent->ops->mode_changed(device->device_parent);
    }

    if (unicode)
        return CallWindowProcW(proc, window, message, wparam, lparam);
    else
        return CallWindowProcA(proc, window, message, wparam, lparam);
}
