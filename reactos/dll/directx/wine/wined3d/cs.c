/*
 * Copyright 2013 Henri Verbeet for CodeWeavers
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

#define WINED3D_INITIAL_CS_SIZE 4096

enum wined3d_cs_op
{
    WINED3D_CS_OP_PRESENT,
    WINED3D_CS_OP_CLEAR,
    WINED3D_CS_OP_DRAW,
    WINED3D_CS_OP_SET_PREDICATION,
    WINED3D_CS_OP_SET_VIEWPORT,
    WINED3D_CS_OP_SET_SCISSOR_RECT,
    WINED3D_CS_OP_SET_RENDERTARGET_VIEW,
    WINED3D_CS_OP_SET_DEPTH_STENCIL_VIEW,
    WINED3D_CS_OP_SET_VERTEX_DECLARATION,
    WINED3D_CS_OP_SET_STREAM_SOURCE,
    WINED3D_CS_OP_SET_STREAM_SOURCE_FREQ,
    WINED3D_CS_OP_SET_STREAM_OUTPUT,
    WINED3D_CS_OP_SET_INDEX_BUFFER,
    WINED3D_CS_OP_SET_CONSTANT_BUFFER,
    WINED3D_CS_OP_SET_TEXTURE,
    WINED3D_CS_OP_SET_SHADER_RESOURCE_VIEW,
    WINED3D_CS_OP_SET_SAMPLER,
    WINED3D_CS_OP_SET_SHADER,
    WINED3D_CS_OP_SET_RENDER_STATE,
    WINED3D_CS_OP_SET_TEXTURE_STATE,
    WINED3D_CS_OP_SET_SAMPLER_STATE,
    WINED3D_CS_OP_SET_TRANSFORM,
    WINED3D_CS_OP_SET_CLIP_PLANE,
    WINED3D_CS_OP_SET_MATERIAL,
    WINED3D_CS_OP_RESET_STATE,
};

struct wined3d_cs_present
{
    enum wined3d_cs_op opcode;
    HWND dst_window_override;
    struct wined3d_swapchain *swapchain;
    const RECT *src_rect;
    const RECT *dst_rect;
    const RGNDATA *dirty_region;
    DWORD flags;
};

struct wined3d_cs_clear
{
    enum wined3d_cs_op opcode;
    DWORD rect_count;
    const RECT *rects;
    DWORD flags;
    const struct wined3d_color *color;
    float depth;
    DWORD stencil;
};

struct wined3d_cs_draw
{
    enum wined3d_cs_op opcode;
    UINT start_idx;
    UINT index_count;
    UINT start_instance;
    UINT instance_count;
    BOOL indexed;
};

struct wined3d_cs_set_predication
{
    enum wined3d_cs_op opcode;
    struct wined3d_query *predicate;
    BOOL value;
};

struct wined3d_cs_set_viewport
{
    enum wined3d_cs_op opcode;
    const struct wined3d_viewport *viewport;
};

struct wined3d_cs_set_scissor_rect
{
    enum wined3d_cs_op opcode;
    const RECT *rect;
};

struct wined3d_cs_set_rendertarget_view
{
    enum wined3d_cs_op opcode;
    unsigned int view_idx;
    struct wined3d_rendertarget_view *view;
};

struct wined3d_cs_set_depth_stencil_view
{
    enum wined3d_cs_op opcode;
    struct wined3d_rendertarget_view *view;
};

struct wined3d_cs_set_vertex_declaration
{
    enum wined3d_cs_op opcode;
    struct wined3d_vertex_declaration *declaration;
};

struct wined3d_cs_set_stream_source
{
    enum wined3d_cs_op opcode;
    UINT stream_idx;
    struct wined3d_buffer *buffer;
    UINT offset;
    UINT stride;
};

struct wined3d_cs_set_stream_source_freq
{
    enum wined3d_cs_op opcode;
    UINT stream_idx;
    UINT frequency;
    UINT flags;
};

struct wined3d_cs_set_stream_output
{
    enum wined3d_cs_op opcode;
    UINT stream_idx;
    struct wined3d_buffer *buffer;
    UINT offset;
};

struct wined3d_cs_set_index_buffer
{
    enum wined3d_cs_op opcode;
    struct wined3d_buffer *buffer;
    enum wined3d_format_id format_id;
};

struct wined3d_cs_set_constant_buffer
{
    enum wined3d_cs_op opcode;
    enum wined3d_shader_type type;
    UINT cb_idx;
    struct wined3d_buffer *buffer;
};

struct wined3d_cs_set_texture
{
    enum wined3d_cs_op opcode;
    UINT stage;
    struct wined3d_texture *texture;
};

struct wined3d_cs_set_shader_resource_view
{
    enum wined3d_cs_op opcode;
    enum wined3d_shader_type type;
    UINT view_idx;
    struct wined3d_shader_resource_view *view;
};

struct wined3d_cs_set_sampler
{
    enum wined3d_cs_op opcode;
    enum wined3d_shader_type type;
    UINT sampler_idx;
    struct wined3d_sampler *sampler;
};

struct wined3d_cs_set_shader
{
    enum wined3d_cs_op opcode;
    enum wined3d_shader_type type;
    struct wined3d_shader *shader;
};

struct wined3d_cs_set_render_state
{
    enum wined3d_cs_op opcode;
    enum wined3d_render_state state;
    DWORD value;
};

struct wined3d_cs_set_texture_state
{
    enum wined3d_cs_op opcode;
    UINT stage;
    enum wined3d_texture_stage_state state;
    DWORD value;
};

struct wined3d_cs_set_sampler_state
{
    enum wined3d_cs_op opcode;
    UINT sampler_idx;
    enum wined3d_sampler_state state;
    DWORD value;
};

struct wined3d_cs_set_transform
{
    enum wined3d_cs_op opcode;
    enum wined3d_transform_state state;
    const struct wined3d_matrix *matrix;
};

struct wined3d_cs_set_clip_plane
{
    enum wined3d_cs_op opcode;
    UINT plane_idx;
    const struct wined3d_vec4 *plane;
};

struct wined3d_cs_set_material
{
    enum wined3d_cs_op opcode;
    const struct wined3d_material *material;
};

struct wined3d_cs_reset_state
{
    enum wined3d_cs_op opcode;
};

static void wined3d_cs_exec_present(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_present *op = data;
    struct wined3d_swapchain *swapchain;

    swapchain = op->swapchain;
    wined3d_swapchain_set_window(swapchain, op->dst_window_override);

    swapchain->swapchain_ops->swapchain_present(swapchain,
            op->src_rect, op->dst_rect, op->dirty_region, op->flags);
}

void wined3d_cs_emit_present(struct wined3d_cs *cs, struct wined3d_swapchain *swapchain,
        const RECT *src_rect, const RECT *dst_rect, HWND dst_window_override,
        const RGNDATA *dirty_region, DWORD flags)
{
    struct wined3d_cs_present *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_PRESENT;
    op->dst_window_override = dst_window_override;
    op->swapchain = swapchain;
    op->src_rect = src_rect;
    op->dst_rect = dst_rect;
    op->dirty_region = dirty_region;
    op->flags = flags;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_clear(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_clear *op = data;
    struct wined3d_device *device;
    RECT draw_rect;

    device = cs->device;
    wined3d_get_draw_rect(&device->state, &draw_rect);
    device_clear_render_targets(device, device->adapter->gl_info.limits.buffers,
            &device->fb, op->rect_count, op->rects, &draw_rect, op->flags,
            op->color, op->depth, op->stencil);
}

void wined3d_cs_emit_clear(struct wined3d_cs *cs, DWORD rect_count, const RECT *rects,
        DWORD flags, const struct wined3d_color *color, float depth, DWORD stencil)
{
    struct wined3d_cs_clear *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_CLEAR;
    op->rect_count = rect_count;
    op->rects = rects;
    op->flags = flags;
    op->color = color;
    op->depth = depth;
    op->stencil = stencil;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_draw(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_draw *op = data;

    draw_primitive(cs->device, op->start_idx, op->index_count,
            op->start_instance, op->instance_count, op->indexed);
}

void wined3d_cs_emit_draw(struct wined3d_cs *cs, UINT start_idx, UINT index_count,
        UINT start_instance, UINT instance_count, BOOL indexed)
{
    struct wined3d_cs_draw *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_DRAW;
    op->start_idx = start_idx;
    op->index_count = index_count;
    op->start_instance = start_instance;
    op->instance_count = instance_count;
    op->indexed = indexed;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_predication(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_predication *op = data;

    cs->state.predicate = op->predicate;
    cs->state.predicate_value = op->value;
}

void wined3d_cs_emit_set_predication(struct wined3d_cs *cs, struct wined3d_query *predicate, BOOL value)
{
    struct wined3d_cs_set_predication *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_PREDICATION;
    op->predicate = predicate;
    op->value = value;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_viewport(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_viewport *op = data;

    cs->state.viewport = *op->viewport;
    device_invalidate_state(cs->device, STATE_VIEWPORT);
}

void wined3d_cs_emit_set_viewport(struct wined3d_cs *cs, const struct wined3d_viewport *viewport)
{
    struct wined3d_cs_set_viewport *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_VIEWPORT;
    op->viewport = viewport;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_scissor_rect(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_scissor_rect *op = data;

    cs->state.scissor_rect = *op->rect;
    device_invalidate_state(cs->device, STATE_SCISSORRECT);
}

void wined3d_cs_emit_set_scissor_rect(struct wined3d_cs *cs, const RECT *rect)
{
    struct wined3d_cs_set_scissor_rect *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_SCISSOR_RECT;
    op->rect = rect;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_rendertarget_view(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_rendertarget_view *op = data;

    cs->state.fb->render_targets[op->view_idx] = op->view;
    device_invalidate_state(cs->device, STATE_FRAMEBUFFER);
}

void wined3d_cs_emit_set_rendertarget_view(struct wined3d_cs *cs, unsigned int view_idx,
        struct wined3d_rendertarget_view *view)
{
    struct wined3d_cs_set_rendertarget_view *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_RENDERTARGET_VIEW;
    op->view_idx = view_idx;
    op->view = view;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_depth_stencil_view(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_depth_stencil_view *op = data;
    struct wined3d_device *device = cs->device;
    struct wined3d_rendertarget_view *prev;

    if ((prev = cs->state.fb->depth_stencil))
    {
        struct wined3d_surface *prev_surface = wined3d_rendertarget_view_get_surface(prev);

        if (prev_surface && (device->swapchains[0]->desc.flags & WINED3DPRESENTFLAG_DISCARD_DEPTHSTENCIL
                || prev_surface->flags & SFLAG_DISCARD))
        {
            surface_modify_ds_location(prev_surface, WINED3D_LOCATION_DISCARDED, prev->width, prev->height);
            if (prev_surface == device->onscreen_depth_stencil)
            {
                wined3d_surface_decref(device->onscreen_depth_stencil);
                device->onscreen_depth_stencil = NULL;
            }
        }
    }

    cs->fb.depth_stencil = op->view;

    if (!prev != !op->view)
    {
        /* Swapping NULL / non NULL depth stencil affects the depth and tests */
        device_invalidate_state(device, STATE_RENDER(WINED3D_RS_ZENABLE));
        device_invalidate_state(device, STATE_RENDER(WINED3D_RS_STENCILENABLE));
        device_invalidate_state(device, STATE_RENDER(WINED3D_RS_STENCILWRITEMASK));
        device_invalidate_state(device, STATE_RENDER(WINED3D_RS_DEPTHBIAS));
    }
    else if (prev && prev->format->depth_size != op->view->format->depth_size)
    {
        device_invalidate_state(device, STATE_RENDER(WINED3D_RS_DEPTHBIAS));
    }

    device_invalidate_state(device, STATE_FRAMEBUFFER);
}

void wined3d_cs_emit_set_depth_stencil_view(struct wined3d_cs *cs, struct wined3d_rendertarget_view *view)
{
    struct wined3d_cs_set_depth_stencil_view *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_DEPTH_STENCIL_VIEW;
    op->view = view;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_vertex_declaration(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_vertex_declaration *op = data;

    cs->state.vertex_declaration = op->declaration;
    device_invalidate_state(cs->device, STATE_VDECL);
}

void wined3d_cs_emit_set_vertex_declaration(struct wined3d_cs *cs, struct wined3d_vertex_declaration *declaration)
{
    struct wined3d_cs_set_vertex_declaration *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_VERTEX_DECLARATION;
    op->declaration = declaration;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_stream_source(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_stream_source *op = data;
    struct wined3d_stream_state *stream;
    struct wined3d_buffer *prev;

    stream = &cs->state.streams[op->stream_idx];
    prev = stream->buffer;
    stream->buffer = op->buffer;
    stream->offset = op->offset;
    stream->stride = op->stride;

    if (op->buffer)
        InterlockedIncrement(&op->buffer->resource.bind_count);
    if (prev)
        InterlockedDecrement(&prev->resource.bind_count);

    device_invalidate_state(cs->device, STATE_STREAMSRC);
}

void wined3d_cs_emit_set_stream_source(struct wined3d_cs *cs, UINT stream_idx,
        struct wined3d_buffer *buffer, UINT offset, UINT stride)
{
    struct wined3d_cs_set_stream_source *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_STREAM_SOURCE;
    op->stream_idx = stream_idx;
    op->buffer = buffer;
    op->offset = offset;
    op->stride = stride;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_stream_source_freq(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_stream_source_freq *op = data;
    struct wined3d_stream_state *stream;

    stream = &cs->state.streams[op->stream_idx];
    stream->frequency = op->frequency;
    stream->flags = op->flags;

    device_invalidate_state(cs->device, STATE_STREAMSRC);
}

void wined3d_cs_emit_set_stream_source_freq(struct wined3d_cs *cs, UINT stream_idx, UINT frequency, UINT flags)
{
    struct wined3d_cs_set_stream_source_freq *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_STREAM_SOURCE_FREQ;
    op->stream_idx = stream_idx;
    op->frequency = frequency;
    op->flags = flags;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_stream_output(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_stream_output *op = data;
    struct wined3d_stream_output *stream;
    struct wined3d_buffer *prev;

    stream = &cs->state.stream_output[op->stream_idx];
    prev = stream->buffer;
    stream->buffer = op->buffer;
    stream->offset = op->offset;

    if (op->buffer)
        InterlockedIncrement(&op->buffer->resource.bind_count);
    if (prev)
        InterlockedDecrement(&prev->resource.bind_count);
}

void wined3d_cs_emit_set_stream_output(struct wined3d_cs *cs, UINT stream_idx,
        struct wined3d_buffer *buffer, UINT offset)
{
    struct wined3d_cs_set_stream_output *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_STREAM_OUTPUT;
    op->stream_idx = stream_idx;
    op->buffer = buffer;
    op->offset = offset;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_index_buffer(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_index_buffer *op = data;
    struct wined3d_buffer *prev;

    prev = cs->state.index_buffer;
    cs->state.index_buffer = op->buffer;
    cs->state.index_format = op->format_id;

    if (op->buffer)
        InterlockedIncrement(&op->buffer->resource.bind_count);
    if (prev)
        InterlockedDecrement(&prev->resource.bind_count);

    device_invalidate_state(cs->device, STATE_INDEXBUFFER);
}

void wined3d_cs_emit_set_index_buffer(struct wined3d_cs *cs, struct wined3d_buffer *buffer,
        enum wined3d_format_id format_id)
{
    struct wined3d_cs_set_index_buffer *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_INDEX_BUFFER;
    op->buffer = buffer;
    op->format_id = format_id;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_constant_buffer(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_constant_buffer *op = data;
    struct wined3d_buffer *prev;

    prev = cs->state.cb[op->type][op->cb_idx];
    cs->state.cb[op->type][op->cb_idx] = op->buffer;

    if (op->buffer)
        InterlockedIncrement(&op->buffer->resource.bind_count);
    if (prev)
        InterlockedDecrement(&prev->resource.bind_count);

    device_invalidate_state(cs->device, STATE_CONSTANT_BUFFER(op->type));
}

void wined3d_cs_emit_set_constant_buffer(struct wined3d_cs *cs, enum wined3d_shader_type type,
        UINT cb_idx, struct wined3d_buffer *buffer)
{
    struct wined3d_cs_set_constant_buffer *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_CONSTANT_BUFFER;
    op->type = type;
    op->cb_idx = cb_idx;
    op->buffer = buffer;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_texture(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_d3d_info *d3d_info = &cs->device->adapter->d3d_info;
    const struct wined3d_cs_set_texture *op = data;
    struct wined3d_texture *prev;

    prev = cs->state.textures[op->stage];
    cs->state.textures[op->stage] = op->texture;

    if (op->texture)
    {
        const struct wined3d_format *new_format = op->texture->resource.format;
        const struct wined3d_format *old_format = prev ? prev->resource.format : NULL;

        if (InterlockedIncrement(&op->texture->resource.bind_count) == 1)
            op->texture->sampler = op->stage;

        if (!prev || op->texture->target != prev->target
                || !is_same_fixup(new_format->color_fixup, old_format->color_fixup)
                || (new_format->flags & WINED3DFMT_FLAG_SHADOW) != (old_format->flags & WINED3DFMT_FLAG_SHADOW))
            device_invalidate_state(cs->device, STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL));

        if (!prev && op->stage < d3d_info->limits.ffp_blend_stages)
        {
            /* The source arguments for color and alpha ops have different
             * meanings when a NULL texture is bound, so the COLOR_OP and
             * ALPHA_OP have to be dirtified. */
            device_invalidate_state(cs->device, STATE_TEXTURESTAGE(op->stage, WINED3D_TSS_COLOR_OP));
            device_invalidate_state(cs->device, STATE_TEXTURESTAGE(op->stage, WINED3D_TSS_ALPHA_OP));
        }
    }

    if (prev)
    {
        if (InterlockedDecrement(&prev->resource.bind_count) && prev->sampler == op->stage)
        {
            unsigned int i;

            /* Search for other stages the texture is bound to. Shouldn't
             * happen if applications bind textures to a single stage only. */
            TRACE("Searching for other stages the texture is bound to.\n");
            for (i = 0; i < MAX_COMBINED_SAMPLERS; ++i)
            {
                if (cs->state.textures[i] == prev)
                {
                    TRACE("Texture is also bound to stage %u.\n", i);
                    prev->sampler = i;
                    break;
                }
            }
        }

        if (!op->texture && op->stage < d3d_info->limits.ffp_blend_stages)
        {
            device_invalidate_state(cs->device, STATE_TEXTURESTAGE(op->stage, WINED3D_TSS_COLOR_OP));
            device_invalidate_state(cs->device, STATE_TEXTURESTAGE(op->stage, WINED3D_TSS_ALPHA_OP));
        }
    }

    device_invalidate_state(cs->device, STATE_SAMPLER(op->stage));
}

void wined3d_cs_emit_set_texture(struct wined3d_cs *cs, UINT stage, struct wined3d_texture *texture)
{
    struct wined3d_cs_set_texture *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_TEXTURE;
    op->stage = stage;
    op->texture = texture;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_shader_resource_view(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_shader_resource_view *op = data;

    cs->state.shader_resource_view[op->type][op->view_idx] = op->view;
    device_invalidate_state(cs->device, STATE_SHADER_RESOURCE_BINDING);
}

void wined3d_cs_emit_set_shader_resource_view(struct wined3d_cs *cs, enum wined3d_shader_type type,
        UINT view_idx, struct wined3d_shader_resource_view *view)
{
    struct wined3d_cs_set_shader_resource_view *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_SHADER_RESOURCE_VIEW;
    op->type = type;
    op->view_idx = view_idx;
    op->view = view;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_sampler(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_sampler *op = data;

    cs->state.sampler[op->type][op->sampler_idx] = op->sampler;
    device_invalidate_state(cs->device, STATE_SHADER_RESOURCE_BINDING);
}

void wined3d_cs_emit_set_sampler(struct wined3d_cs *cs, enum wined3d_shader_type type,
        UINT sampler_idx, struct wined3d_sampler *sampler)
{
    struct wined3d_cs_set_sampler *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_SAMPLER;
    op->type = type;
    op->sampler_idx = sampler_idx;
    op->sampler = sampler;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_shader(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_shader *op = data;

    cs->state.shader[op->type] = op->shader;
    device_invalidate_state(cs->device, STATE_SHADER(op->type));
    device_invalidate_state(cs->device, STATE_SHADER_RESOURCE_BINDING);
}

void wined3d_cs_emit_set_shader(struct wined3d_cs *cs, enum wined3d_shader_type type, struct wined3d_shader *shader)
{
    struct wined3d_cs_set_shader *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_SHADER;
    op->type = type;
    op->shader = shader;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_render_state(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_render_state *op = data;

    cs->state.render_states[op->state] = op->value;
    device_invalidate_state(cs->device, STATE_RENDER(op->state));
}

void wined3d_cs_emit_set_render_state(struct wined3d_cs *cs, enum wined3d_render_state state, DWORD value)
{
    struct wined3d_cs_set_render_state *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_RENDER_STATE;
    op->state = state;
    op->value = value;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_texture_state(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_texture_state *op = data;

    cs->state.texture_states[op->stage][op->state] = op->value;
    device_invalidate_state(cs->device, STATE_TEXTURESTAGE(op->stage, op->state));
}

void wined3d_cs_emit_set_texture_state(struct wined3d_cs *cs, UINT stage,
        enum wined3d_texture_stage_state state, DWORD value)
{
    struct wined3d_cs_set_texture_state *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_TEXTURE_STATE;
    op->stage = stage;
    op->state = state;
    op->value = value;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_sampler_state(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_sampler_state *op = data;

    cs->state.sampler_states[op->sampler_idx][op->state] = op->value;
    device_invalidate_state(cs->device, STATE_SAMPLER(op->sampler_idx));
}

void wined3d_cs_emit_set_sampler_state(struct wined3d_cs *cs, UINT sampler_idx,
        enum wined3d_sampler_state state, DWORD value)
{
    struct wined3d_cs_set_sampler_state *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_SAMPLER_STATE;
    op->sampler_idx = sampler_idx;
    op->state = state;
    op->value = value;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_transform(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_transform *op = data;

    cs->state.transforms[op->state] = *op->matrix;
    if (op->state < WINED3D_TS_WORLD_MATRIX(cs->device->adapter->gl_info.limits.blends))
        device_invalidate_state(cs->device, STATE_TRANSFORM(op->state));
}

void wined3d_cs_emit_set_transform(struct wined3d_cs *cs, enum wined3d_transform_state state,
        const struct wined3d_matrix *matrix)
{
    struct wined3d_cs_set_transform *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_TRANSFORM;
    op->state = state;
    op->matrix = matrix;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_clip_plane(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_clip_plane *op = data;

    cs->state.clip_planes[op->plane_idx] = *op->plane;
    device_invalidate_state(cs->device, STATE_CLIPPLANE(op->plane_idx));
}

void wined3d_cs_emit_set_clip_plane(struct wined3d_cs *cs, UINT plane_idx, const struct wined3d_vec4 *plane)
{
    struct wined3d_cs_set_clip_plane *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_CLIP_PLANE;
    op->plane_idx = plane_idx;
    op->plane = plane;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_set_material(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_material *op = data;

    cs->state.material = *op->material;
    device_invalidate_state(cs->device, STATE_MATERIAL);
}

void wined3d_cs_emit_set_material(struct wined3d_cs *cs, const struct wined3d_material *material)
{
    struct wined3d_cs_set_material *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_SET_MATERIAL;
    op->material = material;

    cs->ops->submit(cs);
}

static void wined3d_cs_exec_reset_state(struct wined3d_cs *cs, const void *data)
{
    struct wined3d_adapter *adapter = cs->device->adapter;
    HRESULT hr;

    state_cleanup(&cs->state);
    memset(&cs->state, 0, sizeof(cs->state));
    if (FAILED(hr = state_init(&cs->state, &cs->fb, &adapter->gl_info, &adapter->d3d_info,
            WINED3D_STATE_NO_REF | WINED3D_STATE_INIT_DEFAULT)))
        ERR("Failed to initialize CS state, hr %#x.\n", hr);
}

void wined3d_cs_emit_reset_state(struct wined3d_cs *cs)
{
    struct wined3d_cs_reset_state *op;

    op = cs->ops->require_space(cs, sizeof(*op));
    op->opcode = WINED3D_CS_OP_RESET_STATE;

    cs->ops->submit(cs);
}

static void (* const wined3d_cs_op_handlers[])(struct wined3d_cs *cs, const void *data) =
{
    /* WINED3D_CS_OP_PRESENT                    */ wined3d_cs_exec_present,
    /* WINED3D_CS_OP_CLEAR                      */ wined3d_cs_exec_clear,
    /* WINED3D_CS_OP_DRAW                       */ wined3d_cs_exec_draw,
    /* WINED3D_CS_OP_SET_PREDICATION            */ wined3d_cs_exec_set_predication,
    /* WINED3D_CS_OP_SET_VIEWPORT               */ wined3d_cs_exec_set_viewport,
    /* WINED3D_CS_OP_SET_SCISSOR_RECT           */ wined3d_cs_exec_set_scissor_rect,
    /* WINED3D_CS_OP_SET_RENDERTARGET_VIEW      */ wined3d_cs_exec_set_rendertarget_view,
    /* WINED3D_CS_OP_SET_DEPTH_STENCIL_VIEW     */ wined3d_cs_exec_set_depth_stencil_view,
    /* WINED3D_CS_OP_SET_VERTEX_DECLARATION     */ wined3d_cs_exec_set_vertex_declaration,
    /* WINED3D_CS_OP_SET_STREAM_SOURCE          */ wined3d_cs_exec_set_stream_source,
    /* WINED3D_CS_OP_SET_STREAM_SOURCE_FREQ     */ wined3d_cs_exec_set_stream_source_freq,
    /* WINED3D_CS_OP_SET_STREAM_OUTPUT          */ wined3d_cs_exec_set_stream_output,
    /* WINED3D_CS_OP_SET_INDEX_BUFFER           */ wined3d_cs_exec_set_index_buffer,
    /* WINED3D_CS_OP_SET_CONSTANT_BUFFER        */ wined3d_cs_exec_set_constant_buffer,
    /* WINED3D_CS_OP_SET_TEXTURE                */ wined3d_cs_exec_set_texture,
    /* WINED3D_CS_OP_SET_SHADER_RESOURCE_VIEW   */ wined3d_cs_exec_set_shader_resource_view,
    /* WINED3D_CS_OP_SET_SAMPLER                */ wined3d_cs_exec_set_sampler,
    /* WINED3D_CS_OP_SET_SHADER                 */ wined3d_cs_exec_set_shader,
    /* WINED3D_CS_OP_SET_RENDER_STATE           */ wined3d_cs_exec_set_render_state,
    /* WINED3D_CS_OP_SET_TEXTURE_STATE          */ wined3d_cs_exec_set_texture_state,
    /* WINED3D_CS_OP_SET_SAMPLER_STATE          */ wined3d_cs_exec_set_sampler_state,
    /* WINED3D_CS_OP_SET_TRANSFORM              */ wined3d_cs_exec_set_transform,
    /* WINED3D_CS_OP_SET_CLIP_PLANE             */ wined3d_cs_exec_set_clip_plane,
    /* WINED3D_CS_OP_SET_MATERIAL               */ wined3d_cs_exec_set_material,
    /* WINED3D_CS_OP_RESET_STATE                */ wined3d_cs_exec_reset_state,
};

static void *wined3d_cs_st_require_space(struct wined3d_cs *cs, size_t size)
{
    if (size > cs->data_size)
    {
        void *new_data;

        size = max( size, cs->data_size * 2 );
        if (!(new_data = HeapReAlloc(GetProcessHeap(), 0, cs->data, size)))
            return NULL;

        cs->data_size = size;
        cs->data = new_data;
    }

    return cs->data;
}

static void wined3d_cs_st_submit(struct wined3d_cs *cs)
{
    enum wined3d_cs_op opcode = *(const enum wined3d_cs_op *)cs->data;

    wined3d_cs_op_handlers[opcode](cs, cs->data);
}

static const struct wined3d_cs_ops wined3d_cs_st_ops =
{
    wined3d_cs_st_require_space,
    wined3d_cs_st_submit,
};

struct wined3d_cs *wined3d_cs_create(struct wined3d_device *device)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_cs *cs;

    if (!(cs = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*cs))))
        return NULL;

    if (!(cs->fb.render_targets = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(*cs->fb.render_targets) * gl_info->limits.buffers)))
    {
        HeapFree(GetProcessHeap(), 0, cs);
        return NULL;
    }

    if (FAILED(state_init(&cs->state, &cs->fb, gl_info, &device->adapter->d3d_info,
            WINED3D_STATE_NO_REF | WINED3D_STATE_INIT_DEFAULT)))
    {
        HeapFree(GetProcessHeap(), 0, cs->fb.render_targets);
        HeapFree(GetProcessHeap(), 0, cs);
        return NULL;
    }

    cs->ops = &wined3d_cs_st_ops;
    cs->device = device;

    cs->data_size = WINED3D_INITIAL_CS_SIZE;
    if (!(cs->data = HeapAlloc(GetProcessHeap(), 0, cs->data_size)))
    {
        HeapFree(GetProcessHeap(), 0, cs);
        return NULL;
    }

    return cs;
}

void wined3d_cs_destroy(struct wined3d_cs *cs)
{
    state_cleanup(&cs->state);
    HeapFree(GetProcessHeap(), 0, cs->fb.render_targets);
    HeapFree(GetProcessHeap(), 0, cs->data);
    HeapFree(GetProcessHeap(), 0, cs);
}
