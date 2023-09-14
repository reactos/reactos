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

#include "config.h"
#include "wine/port.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

#define WINED3D_INITIAL_CS_SIZE 4096

enum wined3d_cs_op
{
    WINED3D_CS_OP_NOP,
    WINED3D_CS_OP_PRESENT,
    WINED3D_CS_OP_CLEAR,
    WINED3D_CS_OP_DISPATCH,
    WINED3D_CS_OP_DRAW,
    WINED3D_CS_OP_FLUSH,
    WINED3D_CS_OP_SET_PREDICATION,
    WINED3D_CS_OP_SET_VIEWPORTS,
    WINED3D_CS_OP_SET_SCISSOR_RECTS,
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
    WINED3D_CS_OP_SET_UNORDERED_ACCESS_VIEW,
    WINED3D_CS_OP_SET_SAMPLER,
    WINED3D_CS_OP_SET_SHADER,
    WINED3D_CS_OP_SET_BLEND_STATE,
    WINED3D_CS_OP_SET_RASTERIZER_STATE,
    WINED3D_CS_OP_SET_RENDER_STATE,
    WINED3D_CS_OP_SET_TEXTURE_STATE,
    WINED3D_CS_OP_SET_SAMPLER_STATE,
    WINED3D_CS_OP_SET_TRANSFORM,
    WINED3D_CS_OP_SET_CLIP_PLANE,
    WINED3D_CS_OP_SET_COLOR_KEY,
    WINED3D_CS_OP_SET_MATERIAL,
    WINED3D_CS_OP_SET_LIGHT,
    WINED3D_CS_OP_SET_LIGHT_ENABLE,
    WINED3D_CS_OP_PUSH_CONSTANTS,
    WINED3D_CS_OP_RESET_STATE,
    WINED3D_CS_OP_CALLBACK,
    WINED3D_CS_OP_QUERY_ISSUE,
    WINED3D_CS_OP_PRELOAD_RESOURCE,
    WINED3D_CS_OP_UNLOAD_RESOURCE,
    WINED3D_CS_OP_MAP,
    WINED3D_CS_OP_UNMAP,
    WINED3D_CS_OP_BLT_SUB_RESOURCE,
    WINED3D_CS_OP_UPDATE_SUB_RESOURCE,
    WINED3D_CS_OP_ADD_DIRTY_TEXTURE_REGION,
    WINED3D_CS_OP_CLEAR_UNORDERED_ACCESS_VIEW,
    WINED3D_CS_OP_COPY_UAV_COUNTER,
    WINED3D_CS_OP_GENERATE_MIPMAPS,
    WINED3D_CS_OP_STOP,
};

struct wined3d_cs_packet
{
    size_t size;
    BYTE data[1];
};

struct wined3d_cs_nop
{
    enum wined3d_cs_op opcode;
};

struct wined3d_cs_present
{
    enum wined3d_cs_op opcode;
    HWND dst_window_override;
    struct wined3d_swapchain *swapchain;
    RECT src_rect;
    RECT dst_rect;
    DWORD swap_interval;
    DWORD flags;
};

struct wined3d_cs_clear
{
    enum wined3d_cs_op opcode;
    DWORD flags;
    unsigned int rt_count;
    struct wined3d_fb_state *fb;
    RECT draw_rect;
    struct wined3d_color color;
    float depth;
    DWORD stencil;
    unsigned int rect_count;
    RECT rects[1];
};

struct wined3d_cs_dispatch
{
    enum wined3d_cs_op opcode;
    struct wined3d_dispatch_parameters parameters;
};

struct wined3d_cs_draw
{
    enum wined3d_cs_op opcode;
    GLenum primitive_type;
    GLint patch_vertex_count;
    struct wined3d_draw_parameters parameters;
};

struct wined3d_cs_flush
{
    enum wined3d_cs_op opcode;
};

struct wined3d_cs_set_predication
{
    enum wined3d_cs_op opcode;
    struct wined3d_query *predicate;
    BOOL value;
};

struct wined3d_cs_set_viewports
{
    enum wined3d_cs_op opcode;
    unsigned int viewport_count;
    struct wined3d_viewport viewports[1];
};

struct wined3d_cs_set_scissor_rects
{
    enum wined3d_cs_op opcode;
    unsigned int rect_count;
    RECT rects[1];
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
    unsigned int offset;
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

struct wined3d_cs_set_color_key
{
    enum wined3d_cs_op opcode;
    struct wined3d_texture *texture;
    WORD flags;
    WORD set;
    struct wined3d_color_key color_key;
};

struct wined3d_cs_set_shader_resource_view
{
    enum wined3d_cs_op opcode;
    enum wined3d_shader_type type;
    UINT view_idx;
    struct wined3d_shader_resource_view *view;
};

struct wined3d_cs_set_unordered_access_view
{
    enum wined3d_cs_op opcode;
    enum wined3d_pipeline pipeline;
    unsigned int view_idx;
    struct wined3d_unordered_access_view *view;
    unsigned int initial_count;
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

struct wined3d_cs_set_blend_state
{
    enum wined3d_cs_op opcode;
    struct wined3d_blend_state *state;
    struct wined3d_color factor;
};

struct wined3d_cs_set_rasterizer_state
{
    enum wined3d_cs_op opcode;
    struct wined3d_rasterizer_state *state;
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
    struct wined3d_matrix matrix;
};

struct wined3d_cs_set_clip_plane
{
    enum wined3d_cs_op opcode;
    UINT plane_idx;
    struct wined3d_vec4 plane;
};

struct wined3d_cs_set_material
{
    enum wined3d_cs_op opcode;
    struct wined3d_material material;
};

struct wined3d_cs_set_light
{
    enum wined3d_cs_op opcode;
    struct wined3d_light_info light;
};

struct wined3d_cs_set_light_enable
{
    enum wined3d_cs_op opcode;
    unsigned int idx;
    BOOL enable;
};

struct wined3d_cs_push_constants
{
    enum wined3d_cs_op opcode;
    enum wined3d_push_constants type;
    unsigned int start_idx;
    unsigned int count;
    BYTE constants[1];
};

struct wined3d_cs_reset_state
{
    enum wined3d_cs_op opcode;
};

struct wined3d_cs_callback
{
    enum wined3d_cs_op opcode;
    void (*callback)(void *object);
    void *object;
};

struct wined3d_cs_query_issue
{
    enum wined3d_cs_op opcode;
    struct wined3d_query *query;
    DWORD flags;
};

struct wined3d_cs_preload_resource
{
    enum wined3d_cs_op opcode;
    struct wined3d_resource *resource;
};

struct wined3d_cs_unload_resource
{
    enum wined3d_cs_op opcode;
    struct wined3d_resource *resource;
};

struct wined3d_cs_map
{
    enum wined3d_cs_op opcode;
    struct wined3d_resource *resource;
    unsigned int sub_resource_idx;
    struct wined3d_map_desc *map_desc;
    const struct wined3d_box *box;
    DWORD flags;
    HRESULT *hr;
};

struct wined3d_cs_unmap
{
    enum wined3d_cs_op opcode;
    struct wined3d_resource *resource;
    unsigned int sub_resource_idx;
    HRESULT *hr;
};

struct wined3d_cs_blt_sub_resource
{
    enum wined3d_cs_op opcode;
    struct wined3d_resource *dst_resource;
    unsigned int dst_sub_resource_idx;
    struct wined3d_box dst_box;
    struct wined3d_resource *src_resource;
    unsigned int src_sub_resource_idx;
    struct wined3d_box src_box;
    DWORD flags;
    struct wined3d_blt_fx fx;
    enum wined3d_texture_filter_type filter;
};

struct wined3d_cs_update_sub_resource
{
    enum wined3d_cs_op opcode;
    struct wined3d_resource *resource;
    unsigned int sub_resource_idx;
    struct wined3d_box box;
    struct wined3d_sub_resource_data data;
#if defined(STAGING_CSMT)
    BYTE copy_data[1];
#endif /* STAGING_CSMT */
};

struct wined3d_cs_add_dirty_texture_region
{
    enum wined3d_cs_op opcode;
    struct wined3d_texture *texture;
    unsigned int layer;
};

struct wined3d_cs_clear_unordered_access_view
{
    enum wined3d_cs_op opcode;
    struct wined3d_unordered_access_view *view;
    struct wined3d_uvec4 clear_value;
};

struct wined3d_cs_copy_uav_counter
{
    enum wined3d_cs_op opcode;
    struct wined3d_buffer *buffer;
    unsigned int offset;
    struct wined3d_unordered_access_view *view;
};

struct wined3d_cs_generate_mipmaps
{
    enum wined3d_cs_op opcode;
    struct wined3d_shader_resource_view *view;
};

struct wined3d_cs_stop
{
    enum wined3d_cs_op opcode;
};

static inline void *wined3d_cs_require_space(struct wined3d_cs *cs,
        size_t size, enum wined3d_cs_queue_id queue_id)
{
    return cs->ops->require_space(cs, size, queue_id);
}

static inline void wined3d_cs_submit(struct wined3d_cs *cs, enum wined3d_cs_queue_id queue_id)
{
    cs->ops->submit(cs, queue_id);
}

static const char *debug_cs_op(enum wined3d_cs_op op)
{
    switch (op)
    {
#define WINED3D_TO_STR(type) case type: return #type
        WINED3D_TO_STR(WINED3D_CS_OP_NOP);
        WINED3D_TO_STR(WINED3D_CS_OP_PRESENT);
        WINED3D_TO_STR(WINED3D_CS_OP_CLEAR);
        WINED3D_TO_STR(WINED3D_CS_OP_DISPATCH);
        WINED3D_TO_STR(WINED3D_CS_OP_DRAW);
        WINED3D_TO_STR(WINED3D_CS_OP_FLUSH);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_PREDICATION);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_VIEWPORTS);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_SCISSOR_RECTS);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_RENDERTARGET_VIEW);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_DEPTH_STENCIL_VIEW);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_VERTEX_DECLARATION);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_STREAM_SOURCE);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_STREAM_SOURCE_FREQ);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_STREAM_OUTPUT);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_INDEX_BUFFER);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_CONSTANT_BUFFER);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_TEXTURE);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_SHADER_RESOURCE_VIEW);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_UNORDERED_ACCESS_VIEW);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_SAMPLER);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_SHADER);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_BLEND_STATE);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_RASTERIZER_STATE);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_RENDER_STATE);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_TEXTURE_STATE);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_SAMPLER_STATE);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_TRANSFORM);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_CLIP_PLANE);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_COLOR_KEY);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_MATERIAL);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_LIGHT);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_LIGHT_ENABLE);
        WINED3D_TO_STR(WINED3D_CS_OP_PUSH_CONSTANTS);
        WINED3D_TO_STR(WINED3D_CS_OP_RESET_STATE);
        WINED3D_TO_STR(WINED3D_CS_OP_CALLBACK);
        WINED3D_TO_STR(WINED3D_CS_OP_QUERY_ISSUE);
        WINED3D_TO_STR(WINED3D_CS_OP_PRELOAD_RESOURCE);
        WINED3D_TO_STR(WINED3D_CS_OP_UNLOAD_RESOURCE);
        WINED3D_TO_STR(WINED3D_CS_OP_MAP);
        WINED3D_TO_STR(WINED3D_CS_OP_UNMAP);
        WINED3D_TO_STR(WINED3D_CS_OP_BLT_SUB_RESOURCE);
        WINED3D_TO_STR(WINED3D_CS_OP_UPDATE_SUB_RESOURCE);
        WINED3D_TO_STR(WINED3D_CS_OP_ADD_DIRTY_TEXTURE_REGION);
        WINED3D_TO_STR(WINED3D_CS_OP_CLEAR_UNORDERED_ACCESS_VIEW);
        WINED3D_TO_STR(WINED3D_CS_OP_COPY_UAV_COUNTER);
        WINED3D_TO_STR(WINED3D_CS_OP_GENERATE_MIPMAPS);
        WINED3D_TO_STR(WINED3D_CS_OP_STOP);
#undef WINED3D_TO_STR
        default:
            return wine_dbg_sprintf("UNKNOWN_OP(%#x)", op);
    }
}

static void wined3d_cs_exec_nop(struct wined3d_cs *cs, const void *data)
{
}

static void wined3d_cs_exec_present(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_present *op = data;
    struct wined3d_swapchain *swapchain;
    unsigned int i;

    swapchain = op->swapchain;
    wined3d_swapchain_set_window(swapchain, op->dst_window_override);

    swapchain->swapchain_ops->swapchain_present(swapchain, &op->src_rect, &op->dst_rect, op->swap_interval, op->flags);

    wined3d_resource_release(&swapchain->front_buffer->resource);
    for (i = 0; i < swapchain->state.desc.backbuffer_count; ++i)
    {
        wined3d_resource_release(&swapchain->back_buffers[i]->resource);
    }

    InterlockedDecrement(&cs->pending_presents);
}

void wined3d_cs_emit_present(struct wined3d_cs *cs, struct wined3d_swapchain *swapchain,
        const RECT *src_rect, const RECT *dst_rect, HWND dst_window_override,
        unsigned int swap_interval, DWORD flags)
{
    struct wined3d_cs_present *op;
    unsigned int i;
    LONG pending;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_PRESENT;
    op->dst_window_override = dst_window_override;
    op->swapchain = swapchain;
    op->src_rect = *src_rect;
    op->dst_rect = *dst_rect;
    op->swap_interval = swap_interval;
    op->flags = flags;

    pending = InterlockedIncrement(&cs->pending_presents);

    wined3d_resource_acquire(&swapchain->front_buffer->resource);
    for (i = 0; i < swapchain->state.desc.backbuffer_count; ++i)
    {
        wined3d_resource_acquire(&swapchain->back_buffers[i]->resource);
    }

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);

    /* Limit input latency by limiting the number of presents that we can get
     * ahead of the worker thread. */
    while (pending >= swapchain->max_frame_latency)
    {
        wined3d_pause();
        pending = InterlockedCompareExchange(&cs->pending_presents, 0, 0);
    }
}

static void wined3d_cs_exec_clear(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_clear *op = data;
    struct wined3d_device *device;
    unsigned int i;

    device = cs->device;
    device->blitter->ops->blitter_clear(device->blitter, device, op->rt_count, op->fb,
            op->rect_count, op->rects, &op->draw_rect, op->flags, &op->color, op->depth, op->stencil);

    if (op->flags & WINED3DCLEAR_TARGET)
    {
        for (i = 0; i < op->rt_count; ++i)
        {
            if (op->fb->render_targets[i])
                wined3d_resource_release(op->fb->render_targets[i]->resource);
        }
    }
    if (op->flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL))
        wined3d_resource_release(op->fb->depth_stencil->resource);
}

void wined3d_cs_emit_clear(struct wined3d_cs *cs, DWORD rect_count, const RECT *rects,
        DWORD flags, const struct wined3d_color *color, float depth, DWORD stencil)
{
    const struct wined3d_state *state = &cs->device->state;
    const struct wined3d_viewport *vp = &state->viewports[0];
    struct wined3d_rendertarget_view *view;
    struct wined3d_cs_clear *op;
    unsigned int rt_count, i;

    rt_count = flags & WINED3DCLEAR_TARGET ? cs->device->adapter->d3d_info.limits.max_rt_count : 0;

    op = wined3d_cs_require_space(cs, FIELD_OFFSET(struct wined3d_cs_clear, rects[rect_count]),
            WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_CLEAR;
    op->flags = flags & (WINED3DCLEAR_TARGET | WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL);
    op->rt_count = rt_count;
    op->fb = &cs->fb;
    SetRect(&op->draw_rect, vp->x, vp->y, vp->x + vp->width, vp->y + vp->height);
    if (state->render_states[WINED3D_RS_SCISSORTESTENABLE])
        IntersectRect(&op->draw_rect, &op->draw_rect, &state->scissor_rects[0]);
    op->color = *color;
    op->depth = depth;
    op->stencil = stencil;
    op->rect_count = rect_count;
    memcpy(op->rects, rects, sizeof(*rects) * rect_count);

    for (i = 0; i < rt_count; ++i)
    {
        if ((view = state->fb->render_targets[i]))
            wined3d_resource_acquire(view->resource);
    }
    if (flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL))
    {
        view = state->fb->depth_stencil;
        wined3d_resource_acquire(view->resource);
    }

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

void wined3d_cs_emit_clear_rendertarget_view(struct wined3d_cs *cs, struct wined3d_rendertarget_view *view,
        const RECT *rect, DWORD flags, const struct wined3d_color *color, float depth, DWORD stencil)
{
    struct wined3d_cs_clear *op;
    size_t size;

    size = FIELD_OFFSET(struct wined3d_cs_clear, rects[1]) + sizeof(struct wined3d_fb_state);
    op = wined3d_cs_require_space(cs, size, WINED3D_CS_QUEUE_DEFAULT);
    op->fb = (void *)&op->rects[1];

    op->opcode = WINED3D_CS_OP_CLEAR;
    op->flags = flags & (WINED3DCLEAR_TARGET | WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL);
    if (flags & WINED3DCLEAR_TARGET)
    {
        op->rt_count = 1;
        op->fb->render_targets[0] = view;
        op->fb->depth_stencil = NULL;
        op->color = *color;
    }
    else
    {
        op->rt_count = 0;
        op->fb->render_targets[0] = NULL;
        op->fb->depth_stencil = view;
        op->depth = depth;
        op->stencil = stencil;
    }
    SetRect(&op->draw_rect, 0, 0, view->width, view->height);
    op->rect_count = 1;
    op->rects[0] = *rect;

    wined3d_resource_acquire(view->resource);

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
    if (flags & WINED3DCLEAR_SYNCHRONOUS)
        wined3d_cs_finish(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void acquire_shader_resources(const struct wined3d_state *state, unsigned int shader_mask)
{
    struct wined3d_shader_sampler_map_entry *entry;
    struct wined3d_shader_resource_view *view;
    struct wined3d_shader *shader;
    unsigned int i, j;

    for (i = 0; i < WINED3D_SHADER_TYPE_COUNT; ++i)
    {
        if (!(shader_mask & (1u << i)))
            continue;

        if (!(shader = state->shader[i]))
            continue;

        for (j = 0; j < WINED3D_MAX_CBS; ++j)
        {
            if (state->cb[i][j])
                wined3d_resource_acquire(&state->cb[i][j]->resource);
        }

        for (j = 0; j < shader->reg_maps.sampler_map.count; ++j)
        {
            entry = &shader->reg_maps.sampler_map.entries[j];

            if (!(view = state->shader_resource_view[i][entry->resource_idx]))
                continue;

            wined3d_resource_acquire(view->resource);
        }
    }
}

static void release_shader_resources(const struct wined3d_state *state, unsigned int shader_mask)
{
    struct wined3d_shader_sampler_map_entry *entry;
    struct wined3d_shader_resource_view *view;
    struct wined3d_shader *shader;
    unsigned int i, j;

    for (i = 0; i < WINED3D_SHADER_TYPE_COUNT; ++i)
    {
        if (!(shader_mask & (1u << i)))
            continue;

        if (!(shader = state->shader[i]))
            continue;

        for (j = 0; j < WINED3D_MAX_CBS; ++j)
        {
            if (state->cb[i][j])
                wined3d_resource_release(&state->cb[i][j]->resource);
        }

        for (j = 0; j < shader->reg_maps.sampler_map.count; ++j)
        {
            entry = &shader->reg_maps.sampler_map.entries[j];

            if (!(view = state->shader_resource_view[i][entry->resource_idx]))
                continue;

            wined3d_resource_release(view->resource);
        }
    }
}

static void acquire_unordered_access_resources(const struct wined3d_shader *shader,
        struct wined3d_unordered_access_view * const *views)
{
    unsigned int i;

    if (!shader)
        return;

    for (i = 0; i < MAX_UNORDERED_ACCESS_VIEWS; ++i)
    {
        if (!shader->reg_maps.uav_resource_info[i].type)
            continue;

        if (!views[i])
            continue;

        wined3d_resource_acquire(views[i]->resource);
    }
}

static void release_unordered_access_resources(const struct wined3d_shader *shader,
        struct wined3d_unordered_access_view * const *views)
{
    unsigned int i;

    if (!shader)
        return;

    for (i = 0; i < MAX_UNORDERED_ACCESS_VIEWS; ++i)
    {
        if (!shader->reg_maps.uav_resource_info[i].type)
            continue;

        if (!views[i])
            continue;

        wined3d_resource_release(views[i]->resource);
    }
}

static void wined3d_cs_exec_dispatch(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_dispatch *op = data;
    struct wined3d_state *state = &cs->state;

    dispatch_compute(cs->device, state, &op->parameters);

    if (op->parameters.indirect)
        wined3d_resource_release(&op->parameters.u.indirect.buffer->resource);

    release_shader_resources(state, 1u << WINED3D_SHADER_TYPE_COMPUTE);
    release_unordered_access_resources(state->shader[WINED3D_SHADER_TYPE_COMPUTE],
            state->unordered_access_view[WINED3D_PIPELINE_COMPUTE]);
}

static void acquire_compute_pipeline_resources(const struct wined3d_state *state)
{
    acquire_shader_resources(state, 1u << WINED3D_SHADER_TYPE_COMPUTE);
    acquire_unordered_access_resources(state->shader[WINED3D_SHADER_TYPE_COMPUTE],
            state->unordered_access_view[WINED3D_PIPELINE_COMPUTE]);
}

void wined3d_cs_emit_dispatch(struct wined3d_cs *cs,
        unsigned int group_count_x, unsigned int group_count_y, unsigned int group_count_z)
{
    const struct wined3d_state *state = &cs->device->state;
    struct wined3d_cs_dispatch *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_DISPATCH;
    op->parameters.indirect = FALSE;
    op->parameters.u.direct.group_count_x = group_count_x;
    op->parameters.u.direct.group_count_y = group_count_y;
    op->parameters.u.direct.group_count_z = group_count_z;

    acquire_compute_pipeline_resources(state);

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

void wined3d_cs_emit_dispatch_indirect(struct wined3d_cs *cs,
        struct wined3d_buffer *buffer, unsigned int offset)
{
    const struct wined3d_state *state = &cs->device->state;
    struct wined3d_cs_dispatch *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_DISPATCH;
    op->parameters.indirect = TRUE;
    op->parameters.u.indirect.buffer = buffer;
    op->parameters.u.indirect.offset = offset;

    acquire_compute_pipeline_resources(state);
    wined3d_resource_acquire(&buffer->resource);

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_draw(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_d3d_info *d3d_info = &cs->device->adapter->d3d_info;
    const struct wined3d_shader *geometry_shader;
    struct wined3d_device *device = cs->device;
    int base_vertex_idx, load_base_vertex_idx;
    struct wined3d_state *state = &cs->state;
    const struct wined3d_cs_draw *op = data;
    unsigned int i;

    base_vertex_idx = 0;
    if (!op->parameters.indirect)
    {
        const struct wined3d_direct_draw_parameters *direct = &op->parameters.u.direct;

        if (op->parameters.indexed && d3d_info->draw_base_vertex_offset)
            base_vertex_idx = direct->base_vertex_idx;
        else if (!op->parameters.indexed)
            base_vertex_idx = direct->start_idx;
    }

    /* ARB_draw_indirect always supports a base vertex offset. */
    if (!op->parameters.indirect && !d3d_info->draw_base_vertex_offset)
        load_base_vertex_idx = op->parameters.u.direct.base_vertex_idx;
    else
        load_base_vertex_idx = 0;

    if (state->base_vertex_index != base_vertex_idx)
    {
        state->base_vertex_index = base_vertex_idx;
        for (i = 0; i < device->context_count; ++i)
            device->contexts[i]->constant_update_mask |= WINED3D_SHADER_CONST_BASE_VERTEX_ID;
    }

    if (state->load_base_vertex_index != load_base_vertex_idx)
    {
        state->load_base_vertex_index = load_base_vertex_idx;
        device_invalidate_state(cs->device, STATE_BASEVERTEXINDEX);
    }

    if (state->gl_primitive_type != op->primitive_type)
    {
        if ((geometry_shader = state->shader[WINED3D_SHADER_TYPE_GEOMETRY]) && !geometry_shader->function)
            device_invalidate_state(cs->device, STATE_SHADER(WINED3D_SHADER_TYPE_GEOMETRY));
        if (state->gl_primitive_type == GL_POINTS || op->primitive_type == GL_POINTS)
            device_invalidate_state(cs->device, STATE_POINT_ENABLE);
        state->gl_primitive_type = op->primitive_type;
    }
    state->gl_patch_vertices = op->patch_vertex_count;

    draw_primitive(cs->device, state, &op->parameters);

    if (op->parameters.indirect)
    {
        struct wined3d_buffer *buffer = op->parameters.u.indirect.buffer;
        wined3d_resource_release(&buffer->resource);
    }

    if (op->parameters.indexed)
        wined3d_resource_release(&state->index_buffer->resource);
    for (i = 0; i < ARRAY_SIZE(state->streams); ++i)
    {
        if (state->streams[i].buffer)
            wined3d_resource_release(&state->streams[i].buffer->resource);
    }
    for (i = 0; i < ARRAY_SIZE(state->stream_output); ++i)
    {
        if (state->stream_output[i].buffer)
            wined3d_resource_release(&state->stream_output[i].buffer->resource);
    }
    for (i = 0; i < ARRAY_SIZE(state->textures); ++i)
    {
        if (state->textures[i])
            wined3d_resource_release(&state->textures[i]->resource);
    }
    for (i = 0; i < d3d_info->limits.max_rt_count; ++i)
    {
        if (state->fb->render_targets[i])
            wined3d_resource_release(state->fb->render_targets[i]->resource);
    }
    if (state->fb->depth_stencil)
        wined3d_resource_release(state->fb->depth_stencil->resource);
    release_shader_resources(state, ~(1u << WINED3D_SHADER_TYPE_COMPUTE));
    release_unordered_access_resources(state->shader[WINED3D_SHADER_TYPE_PIXEL],
            state->unordered_access_view[WINED3D_PIPELINE_GRAPHICS]);
}

static void acquire_graphics_pipeline_resources(const struct wined3d_state *state,
        BOOL indexed, const struct wined3d_d3d_info *d3d_info)
{
    unsigned int i;

    if (indexed)
        wined3d_resource_acquire(&state->index_buffer->resource);
    for (i = 0; i < ARRAY_SIZE(state->streams); ++i)
    {
        if (state->streams[i].buffer)
            wined3d_resource_acquire(&state->streams[i].buffer->resource);
    }
    for (i = 0; i < ARRAY_SIZE(state->stream_output); ++i)
    {
        if (state->stream_output[i].buffer)
            wined3d_resource_acquire(&state->stream_output[i].buffer->resource);
    }
    for (i = 0; i < ARRAY_SIZE(state->textures); ++i)
    {
        if (state->textures[i])
            wined3d_resource_acquire(&state->textures[i]->resource);
    }
    for (i = 0; i < d3d_info->limits.max_rt_count; ++i)
    {
        if (state->fb->render_targets[i])
            wined3d_resource_acquire(state->fb->render_targets[i]->resource);
    }
    if (state->fb->depth_stencil)
        wined3d_resource_acquire(state->fb->depth_stencil->resource);
    acquire_shader_resources(state, ~(1u << WINED3D_SHADER_TYPE_COMPUTE));
    acquire_unordered_access_resources(state->shader[WINED3D_SHADER_TYPE_PIXEL],
            state->unordered_access_view[WINED3D_PIPELINE_GRAPHICS]);
}

void wined3d_cs_emit_draw(struct wined3d_cs *cs, GLenum primitive_type, unsigned int patch_vertex_count,
        int base_vertex_idx, unsigned int start_idx, unsigned int index_count,
        unsigned int start_instance, unsigned int instance_count, BOOL indexed)
{
    const struct wined3d_d3d_info *d3d_info = &cs->device->adapter->d3d_info;
    const struct wined3d_state *state = &cs->device->state;
    struct wined3d_cs_draw *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_DRAW;
    op->primitive_type = primitive_type;
    op->patch_vertex_count = patch_vertex_count;
    op->parameters.indirect = FALSE;
    op->parameters.u.direct.base_vertex_idx = base_vertex_idx;
    op->parameters.u.direct.start_idx = start_idx;
    op->parameters.u.direct.index_count = index_count;
    op->parameters.u.direct.start_instance = start_instance;
    op->parameters.u.direct.instance_count = instance_count;
    op->parameters.indexed = indexed;

    acquire_graphics_pipeline_resources(state, indexed, d3d_info);

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

void wined3d_cs_emit_draw_indirect(struct wined3d_cs *cs, GLenum primitive_type, unsigned int patch_vertex_count,
        struct wined3d_buffer *buffer, unsigned int offset, BOOL indexed)
{
    const struct wined3d_d3d_info *d3d_info = &cs->device->adapter->d3d_info;
    const struct wined3d_state *state = &cs->device->state;
    struct wined3d_cs_draw *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_DRAW;
    op->primitive_type = primitive_type;
    op->patch_vertex_count = patch_vertex_count;
    op->parameters.indirect = TRUE;
    op->parameters.u.indirect.buffer = buffer;
    op->parameters.u.indirect.offset = offset;
    op->parameters.indexed = indexed;

    acquire_graphics_pipeline_resources(state, indexed, d3d_info);
    wined3d_resource_acquire(&buffer->resource);

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_flush(struct wined3d_cs *cs, const void *data)
{
    struct wined3d_context *context;

    context = context_acquire(cs->device, NULL, 0);
    cs->device->adapter->adapter_ops->adapter_flush_context(context);
    context_release(context);
}

void wined3d_cs_emit_flush(struct wined3d_cs *cs)
{
    struct wined3d_cs_flush *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_FLUSH;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
    cs->queries_flushed = TRUE;
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

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_PREDICATION;
    op->predicate = predicate;
    op->value = value;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_viewports(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_viewports *op = data;

    if (op->viewport_count)
        memcpy(cs->state.viewports, op->viewports, op->viewport_count * sizeof(*op->viewports));
    else
        memset(cs->state.viewports, 0, sizeof(*cs->state.viewports));
    cs->state.viewport_count = op->viewport_count;
    device_invalidate_state(cs->device, STATE_VIEWPORT);
}

void wined3d_cs_emit_set_viewports(struct wined3d_cs *cs, unsigned int viewport_count,
        const struct wined3d_viewport *viewports)
{
    struct wined3d_cs_set_viewports *op;

    op = wined3d_cs_require_space(cs, FIELD_OFFSET(struct wined3d_cs_set_viewports, viewports[viewport_count]),
            WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_VIEWPORTS;
    memcpy(op->viewports, viewports, viewport_count * sizeof(*viewports));
    op->viewport_count = viewport_count;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_scissor_rects(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_scissor_rects *op = data;

    if (op->rect_count)
        memcpy(cs->state.scissor_rects, op->rects, op->rect_count * sizeof(*op->rects));
    else
        SetRectEmpty(cs->state.scissor_rects);
    cs->state.scissor_rect_count = op->rect_count;
    device_invalidate_state(cs->device, STATE_SCISSORRECT);
}

void wined3d_cs_emit_set_scissor_rects(struct wined3d_cs *cs, unsigned int rect_count, const RECT *rects)
{
    struct wined3d_cs_set_scissor_rects *op;

    op = wined3d_cs_require_space(cs, FIELD_OFFSET(struct wined3d_cs_set_scissor_rects, rects[rect_count]),
            WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_SCISSOR_RECTS;
    memcpy(op->rects, rects, rect_count * sizeof(*rects));
    op->rect_count = rect_count;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_rendertarget_view(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_rendertarget_view *op = data;
    BOOL prev_alpha_swizzle, curr_alpha_swizzle;
    struct wined3d_rendertarget_view *prev;

    prev = cs->state.fb->render_targets[op->view_idx];
    cs->fb.render_targets[op->view_idx] = op->view;
    device_invalidate_state(cs->device, STATE_FRAMEBUFFER);

    prev_alpha_swizzle = prev && prev->format->id == WINED3DFMT_A8_UNORM;
    curr_alpha_swizzle = op->view && op->view->format->id == WINED3DFMT_A8_UNORM;
    if (prev_alpha_swizzle != curr_alpha_swizzle)
        device_invalidate_state(cs->device, STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL));
}

void wined3d_cs_emit_set_rendertarget_view(struct wined3d_cs *cs, unsigned int view_idx,
        struct wined3d_rendertarget_view *view)
{
    struct wined3d_cs_set_rendertarget_view *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_RENDERTARGET_VIEW;
    op->view_idx = view_idx;
    op->view = view;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_depth_stencil_view(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_depth_stencil_view *op = data;
    struct wined3d_device *device = cs->device;
    struct wined3d_rendertarget_view *prev;

    if ((prev = cs->state.fb->depth_stencil) && prev->resource->type != WINED3D_RTYPE_BUFFER)
    {
        struct wined3d_texture *prev_texture = texture_from_resource(prev->resource);

        if (device->swapchains[0]->state.desc.flags & WINED3D_SWAPCHAIN_DISCARD_DEPTHSTENCIL
                || prev_texture->flags & WINED3D_TEXTURE_DISCARD)
            wined3d_texture_validate_location(prev_texture,
                    prev->sub_resource_idx, WINED3D_LOCATION_DISCARDED);
    }

    cs->fb.depth_stencil = op->view;

    if (!prev != !op->view)
    {
        /* Swapping NULL / non NULL depth stencil affects the depth and tests */
        device_invalidate_state(device, STATE_RENDER(WINED3D_RS_ZENABLE));
        device_invalidate_state(device, STATE_RENDER(WINED3D_RS_STENCILENABLE));
        device_invalidate_state(device, STATE_RENDER(WINED3D_RS_STENCILWRITEMASK));
        device_invalidate_state(device, STATE_RENDER(WINED3D_RS_DEPTHBIAS));
        device_invalidate_state(device, STATE_RENDER(WINED3D_RS_DEPTHBIASCLAMP));
    }
    else if (prev)
    {
        if (prev->format->depth_bias_scale != op->view->format->depth_bias_scale)
            device_invalidate_state(device, STATE_RENDER(WINED3D_RS_DEPTHBIAS));
        if (prev->format->stencil_size != op->view->format->stencil_size)
            device_invalidate_state(device, STATE_RENDER(WINED3D_RS_STENCILREF));
    }

    device_invalidate_state(device, STATE_FRAMEBUFFER);
}

void wined3d_cs_emit_set_depth_stencil_view(struct wined3d_cs *cs, struct wined3d_rendertarget_view *view)
{
    struct wined3d_cs_set_depth_stencil_view *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_DEPTH_STENCIL_VIEW;
    op->view = view;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
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

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_VERTEX_DECLARATION;
    op->declaration = declaration;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
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

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_STREAM_SOURCE;
    op->stream_idx = stream_idx;
    op->buffer = buffer;
    op->offset = offset;
    op->stride = stride;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
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

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_STREAM_SOURCE_FREQ;
    op->stream_idx = stream_idx;
    op->frequency = frequency;
    op->flags = flags;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
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

    device_invalidate_state(cs->device, STATE_STREAM_OUTPUT);
}

void wined3d_cs_emit_set_stream_output(struct wined3d_cs *cs, UINT stream_idx,
        struct wined3d_buffer *buffer, UINT offset)
{
    struct wined3d_cs_set_stream_output *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_STREAM_OUTPUT;
    op->stream_idx = stream_idx;
    op->buffer = buffer;
    op->offset = offset;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_index_buffer(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_index_buffer *op = data;
    struct wined3d_buffer *prev;

    prev = cs->state.index_buffer;
    cs->state.index_buffer = op->buffer;
    cs->state.index_format = op->format_id;
    cs->state.index_offset = op->offset;

    if (op->buffer)
        InterlockedIncrement(&op->buffer->resource.bind_count);
    if (prev)
        InterlockedDecrement(&prev->resource.bind_count);

    device_invalidate_state(cs->device, STATE_INDEXBUFFER);
}

void wined3d_cs_emit_set_index_buffer(struct wined3d_cs *cs, struct wined3d_buffer *buffer,
        enum wined3d_format_id format_id, unsigned int offset)
{
    struct wined3d_cs_set_index_buffer *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_INDEX_BUFFER;
    op->buffer = buffer;
    op->format_id = format_id;
    op->offset = offset;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
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

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_CONSTANT_BUFFER;
    op->type = type;
    op->cb_idx = cb_idx;
    op->buffer = buffer;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_texture(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_d3d_info *d3d_info = &cs->device->adapter->d3d_info;
    const struct wined3d_cs_set_texture *op = data;
    struct wined3d_texture *prev;
    BOOL old_use_color_key = FALSE, new_use_color_key = FALSE;

    prev = cs->state.textures[op->stage];
    cs->state.textures[op->stage] = op->texture;

    if (op->texture)
    {
        const struct wined3d_format *new_format = op->texture->resource.format;
        const struct wined3d_format *old_format = prev ? prev->resource.format : NULL;
        unsigned int old_fmt_flags = prev ? prev->resource.format_flags : 0;
        unsigned int new_fmt_flags = op->texture->resource.format_flags;

        if (InterlockedIncrement(&op->texture->resource.bind_count) == 1)
            op->texture->sampler = op->stage;

        if (!prev || wined3d_texture_gl(op->texture)->target != wined3d_texture_gl(prev)->target
                || (!is_same_fixup(new_format->color_fixup, old_format->color_fixup)
                && !(can_use_texture_swizzle(d3d_info, new_format) && can_use_texture_swizzle(d3d_info, old_format)))
                || (new_fmt_flags & WINED3DFMT_FLAG_SHADOW) != (old_fmt_flags & WINED3DFMT_FLAG_SHADOW))
            device_invalidate_state(cs->device, STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL));

        if (!prev && op->stage < d3d_info->limits.ffp_blend_stages)
        {
            /* The source arguments for color and alpha ops have different
             * meanings when a NULL texture is bound, so the COLOR_OP and
             * ALPHA_OP have to be dirtified. */
            device_invalidate_state(cs->device, STATE_TEXTURESTAGE(op->stage, WINED3D_TSS_COLOR_OP));
            device_invalidate_state(cs->device, STATE_TEXTURESTAGE(op->stage, WINED3D_TSS_ALPHA_OP));
        }

        if (!op->stage && op->texture->async.color_key_flags & WINED3D_CKEY_SRC_BLT)
            new_use_color_key = TRUE;
    }

    if (prev)
    {
        if (InterlockedDecrement(&prev->resource.bind_count) && prev->sampler == op->stage)
        {
            unsigned int i;

            /* Search for other stages the texture is bound to. Shouldn't
             * happen if applications bind textures to a single stage only. */
            TRACE("Searching for other stages the texture is bound to.\n");
            for (i = 0; i < WINED3D_MAX_COMBINED_SAMPLERS; ++i)
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

        if (!op->stage && prev->async.color_key_flags & WINED3D_CKEY_SRC_BLT)
            old_use_color_key = TRUE;
    }

    device_invalidate_state(cs->device, STATE_SAMPLER(op->stage));

    if (new_use_color_key != old_use_color_key)
        device_invalidate_state(cs->device, STATE_RENDER(WINED3D_RS_COLORKEYENABLE));

    if (new_use_color_key)
        device_invalidate_state(cs->device, STATE_COLOR_KEY);
}

void wined3d_cs_emit_set_texture(struct wined3d_cs *cs, UINT stage, struct wined3d_texture *texture)
{
    struct wined3d_cs_set_texture *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_TEXTURE;
    op->stage = stage;
    op->texture = texture;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_shader_resource_view(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_shader_resource_view *op = data;
    struct wined3d_shader_resource_view *prev;

    prev = cs->state.shader_resource_view[op->type][op->view_idx];
    cs->state.shader_resource_view[op->type][op->view_idx] = op->view;

    if (op->view)
        InterlockedIncrement(&op->view->resource->bind_count);
    if (prev)
        InterlockedDecrement(&prev->resource->bind_count);

    if (op->type != WINED3D_SHADER_TYPE_COMPUTE)
        device_invalidate_state(cs->device, STATE_GRAPHICS_SHADER_RESOURCE_BINDING);
    else
        device_invalidate_state(cs->device, STATE_COMPUTE_SHADER_RESOURCE_BINDING);
}

void wined3d_cs_emit_set_shader_resource_view(struct wined3d_cs *cs, enum wined3d_shader_type type,
        UINT view_idx, struct wined3d_shader_resource_view *view)
{
    struct wined3d_cs_set_shader_resource_view *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_SHADER_RESOURCE_VIEW;
    op->type = type;
    op->view_idx = view_idx;
    op->view = view;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_unordered_access_view(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_unordered_access_view *op = data;
    struct wined3d_unordered_access_view *prev;

    prev = cs->state.unordered_access_view[op->pipeline][op->view_idx];
    cs->state.unordered_access_view[op->pipeline][op->view_idx] = op->view;

    if (op->view)
        InterlockedIncrement(&op->view->resource->bind_count);
    if (prev)
        InterlockedDecrement(&prev->resource->bind_count);

    if (op->view && op->initial_count != ~0u)
        wined3d_unordered_access_view_set_counter(op->view, op->initial_count);

    device_invalidate_state(cs->device, STATE_UNORDERED_ACCESS_VIEW_BINDING(op->pipeline));
}

void wined3d_cs_emit_set_unordered_access_view(struct wined3d_cs *cs, enum wined3d_pipeline pipeline,
        unsigned int view_idx, struct wined3d_unordered_access_view *view, unsigned int initial_count)
{
    struct wined3d_cs_set_unordered_access_view *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_UNORDERED_ACCESS_VIEW;
    op->pipeline = pipeline;
    op->view_idx = view_idx;
    op->view = view;
    op->initial_count = initial_count;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_sampler(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_sampler *op = data;

    cs->state.sampler[op->type][op->sampler_idx] = op->sampler;
    if (op->type != WINED3D_SHADER_TYPE_COMPUTE)
        device_invalidate_state(cs->device, STATE_GRAPHICS_SHADER_RESOURCE_BINDING);
    else
        device_invalidate_state(cs->device, STATE_COMPUTE_SHADER_RESOURCE_BINDING);
}

void wined3d_cs_emit_set_sampler(struct wined3d_cs *cs, enum wined3d_shader_type type,
        UINT sampler_idx, struct wined3d_sampler *sampler)
{
    struct wined3d_cs_set_sampler *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_SAMPLER;
    op->type = type;
    op->sampler_idx = sampler_idx;
    op->sampler = sampler;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_shader(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_shader *op = data;

    cs->state.shader[op->type] = op->shader;
    device_invalidate_state(cs->device, STATE_SHADER(op->type));
    if (op->type != WINED3D_SHADER_TYPE_COMPUTE)
        device_invalidate_state(cs->device, STATE_GRAPHICS_SHADER_RESOURCE_BINDING);
    else
        device_invalidate_state(cs->device, STATE_COMPUTE_SHADER_RESOURCE_BINDING);
}

void wined3d_cs_emit_set_shader(struct wined3d_cs *cs, enum wined3d_shader_type type, struct wined3d_shader *shader)
{
    struct wined3d_cs_set_shader *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_SHADER;
    op->type = type;
    op->shader = shader;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_blend_state(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_blend_state *op = data;
    struct wined3d_state *state = &cs->state;

    if (state->blend_state != op->state)
    {
        state->blend_state = op->state;
        device_invalidate_state(cs->device, STATE_BLEND);
    }
    state->blend_factor = op->factor;
    device_invalidate_state(cs->device, STATE_BLEND_FACTOR);
}

void wined3d_cs_emit_set_blend_state(struct wined3d_cs *cs, struct wined3d_blend_state *state,
        const struct wined3d_color *blend_factor)
{
    struct wined3d_cs_set_blend_state *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_BLEND_STATE;
    op->state = state;
    op->factor = *blend_factor;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_rasterizer_state(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_rasterizer_state *op = data;

    cs->state.rasterizer_state = op->state;
    device_invalidate_state(cs->device, STATE_RASTERIZER);
}

void wined3d_cs_emit_set_rasterizer_state(struct wined3d_cs *cs,
        struct wined3d_rasterizer_state *rasterizer_state)
{
    struct wined3d_cs_set_rasterizer_state *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_RASTERIZER_STATE;
    op->state = rasterizer_state;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
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

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_RENDER_STATE;
    op->state = state;
    op->value = value;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
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

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_TEXTURE_STATE;
    op->stage = stage;
    op->state = state;
    op->value = value;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
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

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_SAMPLER_STATE;
    op->sampler_idx = sampler_idx;
    op->state = state;
    op->value = value;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_transform(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_transform *op = data;

    cs->state.transforms[op->state] = op->matrix;
    if (op->state < WINED3D_TS_WORLD_MATRIX(max(cs->device->adapter->d3d_info.limits.ffp_vertex_blend_matrices,
            cs->device->adapter->d3d_info.limits.ffp_max_vertex_blend_matrix_index + 1)))
        device_invalidate_state(cs->device, STATE_TRANSFORM(op->state));
}

void wined3d_cs_emit_set_transform(struct wined3d_cs *cs, enum wined3d_transform_state state,
        const struct wined3d_matrix *matrix)
{
    struct wined3d_cs_set_transform *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_TRANSFORM;
    op->state = state;
    op->matrix = *matrix;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_clip_plane(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_clip_plane *op = data;

    cs->state.clip_planes[op->plane_idx] = op->plane;
    device_invalidate_state(cs->device, STATE_CLIPPLANE(op->plane_idx));
}

void wined3d_cs_emit_set_clip_plane(struct wined3d_cs *cs, UINT plane_idx, const struct wined3d_vec4 *plane)
{
    struct wined3d_cs_set_clip_plane *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_CLIP_PLANE;
    op->plane_idx = plane_idx;
    op->plane = *plane;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_color_key(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_color_key *op = data;
    struct wined3d_texture *texture = op->texture;

    if (op->set)
    {
        switch (op->flags)
        {
            case WINED3D_CKEY_DST_BLT:
                texture->async.dst_blt_color_key = op->color_key;
                texture->async.color_key_flags |= WINED3D_CKEY_DST_BLT;
                break;

            case WINED3D_CKEY_DST_OVERLAY:
                texture->async.dst_overlay_color_key = op->color_key;
                texture->async.color_key_flags |= WINED3D_CKEY_DST_OVERLAY;
                break;

            case WINED3D_CKEY_SRC_BLT:
                if (texture == cs->state.textures[0])
                {
                    device_invalidate_state(cs->device, STATE_COLOR_KEY);
                    if (!(texture->async.color_key_flags & WINED3D_CKEY_SRC_BLT))
                        device_invalidate_state(cs->device, STATE_RENDER(WINED3D_RS_COLORKEYENABLE));
                }

                texture->async.src_blt_color_key = op->color_key;
                texture->async.color_key_flags |= WINED3D_CKEY_SRC_BLT;
                break;

            case WINED3D_CKEY_SRC_OVERLAY:
                texture->async.src_overlay_color_key = op->color_key;
                texture->async.color_key_flags |= WINED3D_CKEY_SRC_OVERLAY;
                break;
        }
    }
    else
    {
        switch (op->flags)
        {
            case WINED3D_CKEY_DST_BLT:
                texture->async.color_key_flags &= ~WINED3D_CKEY_DST_BLT;
                break;

            case WINED3D_CKEY_DST_OVERLAY:
                texture->async.color_key_flags &= ~WINED3D_CKEY_DST_OVERLAY;
                break;

            case WINED3D_CKEY_SRC_BLT:
                if (texture == cs->state.textures[0] && texture->async.color_key_flags & WINED3D_CKEY_SRC_BLT)
                    device_invalidate_state(cs->device, STATE_RENDER(WINED3D_RS_COLORKEYENABLE));

                texture->async.color_key_flags &= ~WINED3D_CKEY_SRC_BLT;
                break;

            case WINED3D_CKEY_SRC_OVERLAY:
                texture->async.color_key_flags &= ~WINED3D_CKEY_SRC_OVERLAY;
                break;
        }
    }
}

void wined3d_cs_emit_set_color_key(struct wined3d_cs *cs, struct wined3d_texture *texture,
        WORD flags, const struct wined3d_color_key *color_key)
{
    struct wined3d_cs_set_color_key *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_COLOR_KEY;
    op->texture = texture;
    op->flags = flags;
    if (color_key)
    {
        op->color_key = *color_key;
        op->set = 1;
    }
    else
        op->set = 0;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_material(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_material *op = data;

    cs->state.material = op->material;
    device_invalidate_state(cs->device, STATE_MATERIAL);
}

void wined3d_cs_emit_set_material(struct wined3d_cs *cs, const struct wined3d_material *material)
{
    struct wined3d_cs_set_material *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_MATERIAL;
    op->material = *material;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_light(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_light *op = data;
    struct wined3d_light_info *light_info;
    unsigned int light_idx, hash_idx;

    light_idx = op->light.OriginalIndex;

    if (!(light_info = wined3d_light_state_get_light(&cs->state.light_state, light_idx)))
    {
        TRACE("Adding new light.\n");
        if (!(light_info = heap_alloc_zero(sizeof(*light_info))))
        {
            ERR("Failed to allocate light info.\n");
            return;
        }

        hash_idx = LIGHTMAP_HASHFUNC(light_idx);
        list_add_head(&cs->state.light_state.light_map[hash_idx], &light_info->entry);
        light_info->glIndex = -1;
        light_info->OriginalIndex = light_idx;
    }

    if (light_info->glIndex != -1)
    {
        if (light_info->OriginalParms.type != op->light.OriginalParms.type)
            device_invalidate_state(cs->device, STATE_LIGHT_TYPE);
        device_invalidate_state(cs->device, STATE_ACTIVELIGHT(light_info->glIndex));
    }

    light_info->OriginalParms = op->light.OriginalParms;
    light_info->position = op->light.position;
    light_info->direction = op->light.direction;
    light_info->exponent = op->light.exponent;
    light_info->cutoff = op->light.cutoff;
}

void wined3d_cs_emit_set_light(struct wined3d_cs *cs, const struct wined3d_light_info *light)
{
    struct wined3d_cs_set_light *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_LIGHT;
    op->light = *light;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_light_enable(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_light_enable *op = data;
    struct wined3d_device *device = cs->device;
    struct wined3d_light_info *light_info;
    int prev_idx;

    if (!(light_info = wined3d_light_state_get_light(&cs->state.light_state, op->idx)))
    {
        ERR("Light doesn't exist.\n");
        return;
    }

    prev_idx = light_info->glIndex;
    wined3d_light_state_enable_light(&cs->state.light_state, &device->adapter->d3d_info, light_info, op->enable);
    if (light_info->glIndex != prev_idx)
    {
        device_invalidate_state(device, STATE_LIGHT_TYPE);
        device_invalidate_state(device, STATE_ACTIVELIGHT(op->enable ? light_info->glIndex : prev_idx));
    }
}

void wined3d_cs_emit_set_light_enable(struct wined3d_cs *cs, unsigned int idx, BOOL enable)
{
    struct wined3d_cs_set_light_enable *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_LIGHT_ENABLE;
    op->idx = idx;
    op->enable = enable;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static const struct
{
    size_t offset;
    size_t size;
    DWORD mask;
}
wined3d_cs_push_constant_info[] =
{
    /* WINED3D_PUSH_CONSTANTS_VS_F */
    {FIELD_OFFSET(struct wined3d_state, vs_consts_f), sizeof(struct wined3d_vec4),  WINED3D_SHADER_CONST_VS_F},
    /* WINED3D_PUSH_CONSTANTS_PS_F */
    {FIELD_OFFSET(struct wined3d_state, ps_consts_f), sizeof(struct wined3d_vec4),  WINED3D_SHADER_CONST_PS_F},
    /* WINED3D_PUSH_CONSTANTS_VS_I */
    {FIELD_OFFSET(struct wined3d_state, vs_consts_i), sizeof(struct wined3d_ivec4), WINED3D_SHADER_CONST_VS_I},
    /* WINED3D_PUSH_CONSTANTS_PS_I */
    {FIELD_OFFSET(struct wined3d_state, ps_consts_i), sizeof(struct wined3d_ivec4), WINED3D_SHADER_CONST_PS_I},
    /* WINED3D_PUSH_CONSTANTS_VS_B */
    {FIELD_OFFSET(struct wined3d_state, vs_consts_b), sizeof(BOOL),                 WINED3D_SHADER_CONST_VS_B},
    /* WINED3D_PUSH_CONSTANTS_PS_B */
    {FIELD_OFFSET(struct wined3d_state, ps_consts_b), sizeof(BOOL),                 WINED3D_SHADER_CONST_PS_B},
};

static void wined3d_cs_st_push_constants(struct wined3d_cs *cs, enum wined3d_push_constants p,
        unsigned int start_idx, unsigned int count, const void *constants)
{
    struct wined3d_device *device = cs->device;
    unsigned int context_count;
    unsigned int i;
    size_t offset;

    if (p == WINED3D_PUSH_CONSTANTS_VS_F)
        device->shader_backend->shader_update_float_vertex_constants(device, start_idx, count);
    else if (p == WINED3D_PUSH_CONSTANTS_PS_F)
        device->shader_backend->shader_update_float_pixel_constants(device, start_idx, count);

    offset = wined3d_cs_push_constant_info[p].offset + start_idx * wined3d_cs_push_constant_info[p].size;
    memcpy((BYTE *)&cs->state + offset, constants, count * wined3d_cs_push_constant_info[p].size);
    for (i = 0, context_count = device->context_count; i < context_count; ++i)
    {
        device->contexts[i]->constant_update_mask |= wined3d_cs_push_constant_info[p].mask;
    }
}

static void wined3d_cs_exec_push_constants(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_push_constants *op = data;

    wined3d_cs_st_push_constants(cs, op->type, op->start_idx, op->count, op->constants);
}

static void wined3d_cs_mt_push_constants(struct wined3d_cs *cs, enum wined3d_push_constants p,
        unsigned int start_idx, unsigned int count, const void *constants)
{
    struct wined3d_cs_push_constants *op;
    size_t size;

    size = count * wined3d_cs_push_constant_info[p].size;
    op = wined3d_cs_require_space(cs, FIELD_OFFSET(struct wined3d_cs_push_constants, constants[size]),
            WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_PUSH_CONSTANTS;
    op->type = p;
    op->start_idx = start_idx;
    op->count = count;
    memcpy(op->constants, constants, size);

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_reset_state(struct wined3d_cs *cs, const void *data)
{
    struct wined3d_adapter *adapter = cs->device->adapter;

    state_cleanup(&cs->state);
    memset(&cs->state, 0, sizeof(cs->state));
    state_init(&cs->state, &cs->fb, &adapter->d3d_info, WINED3D_STATE_NO_REF | WINED3D_STATE_INIT_DEFAULT);
}

void wined3d_cs_emit_reset_state(struct wined3d_cs *cs)
{
    struct wined3d_cs_reset_state *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_RESET_STATE;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_callback(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_callback *op = data;

    op->callback(op->object);
}

static void wined3d_cs_emit_callback(struct wined3d_cs *cs, void (*callback)(void *object), void *object)
{
    struct wined3d_cs_callback *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_CALLBACK;
    op->callback = callback;
    op->object = object;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

void wined3d_cs_destroy_object(struct wined3d_cs *cs, void (*callback)(void *object), void *object)
{
    wined3d_cs_emit_callback(cs, callback, object);
}

void wined3d_cs_init_object(struct wined3d_cs *cs, void (*callback)(void *object), void *object)
{
    wined3d_cs_emit_callback(cs, callback, object);
}

static void wined3d_cs_exec_query_issue(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_query_issue *op = data;
    struct wined3d_query *query = op->query;
    BOOL poll;

    poll = query->query_ops->query_issue(query, op->flags);

    if (!cs->thread)
        return;

    if (poll && list_empty(&query->poll_list_entry))
    {
        if (query->buffer_object)
            InterlockedIncrement(&query->counter_retrieved);
        else
            list_add_tail(&cs->query_poll_list, &query->poll_list_entry);
        return;
    }

    /* This can happen if occlusion queries are restarted. This discards the
     * old result, since polling it could result in a GL error. */
    if ((op->flags & WINED3DISSUE_BEGIN) && !poll && !list_empty(&query->poll_list_entry))
    {
        list_remove(&query->poll_list_entry);
        list_init(&query->poll_list_entry);
        InterlockedIncrement(&query->counter_retrieved);
        return;
    }

    /* This can happen when an occlusion query is ended without being started,
     * in which case we don't want to poll, but still have to counter-balance
     * the increment of the main counter.
     *
     * This can also happen if an event query is re-issued before the first
     * fence was reached. In this case the query is already in the list and
     * the poll function will check the new fence. We have to counter-balance
     * the discarded increment. */
    if (op->flags & WINED3DISSUE_END)
        InterlockedIncrement(&query->counter_retrieved);
}

void wined3d_cs_emit_query_issue(struct wined3d_cs *cs, struct wined3d_query *query, DWORD flags)
{
    struct wined3d_cs_query_issue *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_QUERY_ISSUE;
    op->query = query;
    op->flags = flags;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
    cs->queries_flushed = FALSE;
}

static void wined3d_cs_exec_preload_resource(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_preload_resource *op = data;
    struct wined3d_resource *resource = op->resource;

    resource->resource_ops->resource_preload(resource);
    wined3d_resource_release(resource);
}

void wined3d_cs_emit_preload_resource(struct wined3d_cs *cs, struct wined3d_resource *resource)
{
    struct wined3d_cs_preload_resource *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_PRELOAD_RESOURCE;
    op->resource = resource;

    wined3d_resource_acquire(resource);

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_unload_resource(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_unload_resource *op = data;
    struct wined3d_resource *resource = op->resource;

    resource->resource_ops->resource_unload(resource);
    wined3d_resource_release(resource);
}

void wined3d_cs_emit_unload_resource(struct wined3d_cs *cs, struct wined3d_resource *resource)
{
    struct wined3d_cs_unload_resource *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_UNLOAD_RESOURCE;
    op->resource = resource;

    wined3d_resource_acquire(resource);

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_map(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_map *op = data;
    struct wined3d_resource *resource = op->resource;

    *op->hr = resource->resource_ops->resource_sub_resource_map(resource,
            op->sub_resource_idx, op->map_desc, op->box, op->flags);
}

HRESULT wined3d_cs_map(struct wined3d_cs *cs, struct wined3d_resource *resource, unsigned int sub_resource_idx,
        struct wined3d_map_desc *map_desc, const struct wined3d_box *box, unsigned int flags)
{
    struct wined3d_cs_map *op;
    HRESULT hr;

    /* Mapping resources from the worker thread isn't an issue by itself, but
     * increasing the map count would be visible to applications. */
    wined3d_not_from_cs(cs);

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_MAP);
    op->opcode = WINED3D_CS_OP_MAP;
    op->resource = resource;
    op->sub_resource_idx = sub_resource_idx;
    op->map_desc = map_desc;
    op->box = box;
    op->flags = flags;
    op->hr = &hr;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_MAP);
    wined3d_cs_finish(cs, WINED3D_CS_QUEUE_MAP);

    return hr;
}

static void wined3d_cs_exec_unmap(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_unmap *op = data;
    struct wined3d_resource *resource = op->resource;

    *op->hr = resource->resource_ops->resource_sub_resource_unmap(resource, op->sub_resource_idx);
}

HRESULT wined3d_cs_unmap(struct wined3d_cs *cs, struct wined3d_resource *resource, unsigned int sub_resource_idx)
{
    struct wined3d_cs_unmap *op;
    HRESULT hr;

    wined3d_not_from_cs(cs);

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_MAP);
    op->opcode = WINED3D_CS_OP_UNMAP;
    op->resource = resource;
    op->sub_resource_idx = sub_resource_idx;
    op->hr = &hr;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_MAP);
    wined3d_cs_finish(cs, WINED3D_CS_QUEUE_MAP);

    return hr;
}

static void wined3d_cs_exec_blt_sub_resource(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_blt_sub_resource *op = data;

    if (op->dst_resource->type == WINED3D_RTYPE_BUFFER)
    {
        wined3d_buffer_copy(buffer_from_resource(op->dst_resource), op->dst_box.left,
                buffer_from_resource(op->src_resource), op->src_box.left,
                op->src_box.right - op->src_box.left);
    }
    else if (op->dst_resource->type == WINED3D_RTYPE_TEXTURE_3D)
    {
        struct wined3d_texture *src_texture, *dst_texture;
        unsigned int level, update_w, update_h, update_d;
        unsigned int row_pitch, slice_pitch;
        struct wined3d_context *context;
        struct wined3d_bo_address addr;

        if (op->flags & ~WINED3D_BLT_RAW)
        {
            FIXME("Flags %#x not implemented for %s resources.\n",
                    op->flags, debug_d3dresourcetype(op->dst_resource->type));
            goto error;
        }

        if (!(op->flags & WINED3D_BLT_RAW) && op->src_resource->format != op->dst_resource->format)
        {
            FIXME("Format conversion not implemented for %s resources.\n",
                    debug_d3dresourcetype(op->dst_resource->type));
            goto error;
        }

        update_w = op->dst_box.right - op->dst_box.left;
        update_h = op->dst_box.bottom - op->dst_box.top;
        update_d = op->dst_box.back - op->dst_box.front;
        if (op->src_box.right - op->src_box.left != update_w
                || op->src_box.bottom - op->src_box.top != update_h
                || op->src_box.back - op->src_box.front != update_d)
        {
            FIXME("Stretching not implemented for %s resources.\n",
                    debug_d3dresourcetype(op->dst_resource->type));
            goto error;
        }

        dst_texture = texture_from_resource(op->dst_resource);
        src_texture = texture_from_resource(op->src_resource);

        context = context_acquire(cs->device, NULL, 0);

        if (!wined3d_texture_load_location(src_texture, op->src_sub_resource_idx,
                context, src_texture->resource.map_binding))
        {
            ERR("Failed to load source sub-resource into %s.\n",
                    wined3d_debug_location(src_texture->resource.map_binding));
            context_release(context);
            goto error;
        }

        level = op->dst_sub_resource_idx % dst_texture->level_count;
        if (update_w == wined3d_texture_get_level_width(dst_texture, level)
                && update_h == wined3d_texture_get_level_height(dst_texture, level)
                && update_d == wined3d_texture_get_level_depth(dst_texture, level))
        {
            wined3d_texture_prepare_location(dst_texture, op->dst_sub_resource_idx,
                    context, WINED3D_LOCATION_TEXTURE_RGB);
        }
        else if (!wined3d_texture_load_location(dst_texture, op->dst_sub_resource_idx,
                context, WINED3D_LOCATION_TEXTURE_RGB))
        {
            ERR("Failed to load destination sub-resource.\n");
            context_release(context);
            goto error;
        }

        wined3d_texture_get_memory(src_texture, op->src_sub_resource_idx, &addr, src_texture->resource.map_binding);
        wined3d_texture_get_pitch(src_texture, op->src_sub_resource_idx % src_texture->level_count,
                &row_pitch, &slice_pitch);

        dst_texture->texture_ops->texture_upload_data(context, wined3d_const_bo_address(&addr),
                dst_texture->resource.format, &op->src_box, row_pitch, slice_pitch, dst_texture,
                op->dst_sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB,
                op->dst_box.left, op->dst_box.top, op->dst_box.front);
        wined3d_texture_validate_location(dst_texture, op->dst_sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB);
        wined3d_texture_invalidate_location(dst_texture, op->dst_sub_resource_idx, ~WINED3D_LOCATION_TEXTURE_RGB);

        context_release(context);
    }
    else
    {
        if (FAILED(texture2d_blt(texture_from_resource(op->dst_resource), op->dst_sub_resource_idx,
                &op->dst_box, texture_from_resource(op->src_resource), op->src_sub_resource_idx,
                &op->src_box, op->flags, &op->fx, op->filter)))
            FIXME("Blit failed.\n");
    }

error:
    if (op->src_resource)
        wined3d_resource_release(op->src_resource);
    wined3d_resource_release(op->dst_resource);
}

void wined3d_cs_emit_blt_sub_resource(struct wined3d_cs *cs, struct wined3d_resource *dst_resource,
        unsigned int dst_sub_resource_idx, const struct wined3d_box *dst_box, struct wined3d_resource *src_resource,
        unsigned int src_sub_resource_idx, const struct wined3d_box *src_box, DWORD flags,
        const struct wined3d_blt_fx *fx, enum wined3d_texture_filter_type filter)
{
    struct wined3d_cs_blt_sub_resource *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_BLT_SUB_RESOURCE;
    op->dst_resource = dst_resource;
    op->dst_sub_resource_idx = dst_sub_resource_idx;
    op->dst_box = *dst_box;
    op->src_resource = src_resource;
    op->src_sub_resource_idx = src_sub_resource_idx;
    op->src_box = *src_box;
    op->flags = flags;
    if (fx)
        op->fx = *fx;
    else
        memset(&op->fx, 0, sizeof(op->fx));
    op->filter = filter;

    wined3d_resource_acquire(dst_resource);
    if (src_resource)
        wined3d_resource_acquire(src_resource);

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
    if (flags & WINED3D_BLT_SYNCHRONOUS)
        wined3d_cs_finish(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_update_sub_resource(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_update_sub_resource *op = data;
    struct wined3d_resource *resource = op->resource;
    const struct wined3d_box *box = &op->box;
    unsigned int width, height, depth, level;
    struct wined3d_const_bo_address addr;
    struct wined3d_context *context;
    struct wined3d_texture *texture;
    struct wined3d_box src_box;

    context = context_acquire(cs->device, NULL, 0);

    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        struct wined3d_buffer *buffer = buffer_from_resource(resource);

        if (!wined3d_buffer_load_location(buffer, context, WINED3D_LOCATION_BUFFER))
        {
            ERR("Failed to load buffer location.\n");
            goto done;
        }

        wined3d_buffer_upload_data(buffer, context, box, op->data.data);
        wined3d_buffer_invalidate_location(buffer, ~WINED3D_LOCATION_BUFFER);
        goto done;
    }

    texture = wined3d_texture_from_resource(resource);

    level = op->sub_resource_idx % texture->level_count;
    width = wined3d_texture_get_level_width(texture, level);
    height = wined3d_texture_get_level_height(texture, level);
    depth = wined3d_texture_get_level_depth(texture, level);

    addr.buffer_object = 0;
    addr.addr = op->data.data;

    /* Only load the sub-resource for partial updates. */
    if (!box->left && !box->top && !box->front
            && box->right == width && box->bottom == height && box->back == depth)
        wined3d_texture_prepare_location(texture, op->sub_resource_idx, context, WINED3D_LOCATION_TEXTURE_RGB);
    else
        wined3d_texture_load_location(texture, op->sub_resource_idx, context, WINED3D_LOCATION_TEXTURE_RGB);

    wined3d_box_set(&src_box, 0, 0, box->right - box->left, box->bottom - box->top, 0, box->back - box->front);
    texture->texture_ops->texture_upload_data(context, &addr, texture->resource.format, &src_box,
            op->data.row_pitch, op->data.slice_pitch, texture, op->sub_resource_idx,
            WINED3D_LOCATION_TEXTURE_RGB, box->left, box->top, box->front);

    wined3d_texture_validate_location(texture, op->sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB);
    wined3d_texture_invalidate_location(texture, op->sub_resource_idx, ~WINED3D_LOCATION_TEXTURE_RGB);

done:
    context_release(context);

    wined3d_resource_release(resource);
}

void wined3d_cs_emit_update_sub_resource(struct wined3d_cs *cs, struct wined3d_resource *resource,
        unsigned int sub_resource_idx, const struct wined3d_box *box, const void *data, unsigned int row_pitch,
        unsigned int slice_pitch)
{
    struct wined3d_cs_update_sub_resource *op;
#if defined(STAGING_CSMT)
    size_t data_size, size;

    if (resource->type != WINED3D_RTYPE_BUFFER && resource->format_flags & WINED3DFMT_FLAG_BLOCKS)
        goto no_async;

    data_size = 0;
    switch (resource->type)
    {
        case WINED3D_RTYPE_TEXTURE_3D:
            data_size += (box->back - box->front - 1) * slice_pitch;
            /* fall-through */
        case WINED3D_RTYPE_TEXTURE_2D:
            data_size += (box->bottom - box->top - 1) * row_pitch;
            /* fall-through */
        case WINED3D_RTYPE_TEXTURE_1D:
            data_size += (box->right - box->left) * resource->format->byte_count;
            break;
        case WINED3D_RTYPE_BUFFER:
            data_size = box->right - box->left;
            break;
        case WINED3D_RTYPE_NONE:
            return;
    }

    size = FIELD_OFFSET(struct wined3d_cs_update_sub_resource, copy_data[data_size]);
    if (!cs->ops->check_space(cs, size, WINED3D_CS_QUEUE_DEFAULT))
        goto no_async;

    op = cs->ops->require_space(cs, size, WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_UPDATE_SUB_RESOURCE;
    op->resource = resource;
    op->sub_resource_idx = sub_resource_idx;
    op->box = *box;
    op->data.row_pitch = row_pitch;
    op->data.slice_pitch = slice_pitch;
    op->data.data = op->copy_data;
    memcpy(op->copy_data, data, data_size);

    wined3d_resource_acquire(resource);

    cs->ops->submit(cs, WINED3D_CS_QUEUE_DEFAULT);
    return;

no_async:
    wined3d_resource_wait_idle(resource);
#endif /* STAGING_CSMT */

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_MAP);
    op->opcode = WINED3D_CS_OP_UPDATE_SUB_RESOURCE;
    op->resource = resource;
    op->sub_resource_idx = sub_resource_idx;
    op->box = *box;
    op->data.row_pitch = row_pitch;
    op->data.slice_pitch = slice_pitch;
    op->data.data = data;

    wined3d_resource_acquire(resource);

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_MAP);

    /* The data pointer may go away, so we need to wait until it is read.
     * Copying the data may be faster if it's small. */
    wined3d_cs_finish(cs, WINED3D_CS_QUEUE_MAP);
}

static void wined3d_cs_exec_add_dirty_texture_region(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_add_dirty_texture_region *op = data;
    struct wined3d_texture *texture = op->texture;
    unsigned int sub_resource_idx, i;
    struct wined3d_context *context;

    context = context_acquire(cs->device, NULL, 0);
    sub_resource_idx = op->layer * texture->level_count;
    for (i = 0; i < texture->level_count; ++i, ++sub_resource_idx)
    {
        if (wined3d_texture_load_location(texture, sub_resource_idx, context, texture->resource.map_binding))
            wined3d_texture_invalidate_location(texture, sub_resource_idx, ~texture->resource.map_binding);
        else
            ERR("Failed to load location %s.\n", wined3d_debug_location(texture->resource.map_binding));
    }
    context_release(context);

    wined3d_resource_release(&texture->resource);
}

void wined3d_cs_emit_add_dirty_texture_region(struct wined3d_cs *cs,
        struct wined3d_texture *texture, unsigned int layer)
{
    struct wined3d_cs_add_dirty_texture_region *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_ADD_DIRTY_TEXTURE_REGION;
    op->texture = texture;
    op->layer = layer;

    wined3d_resource_acquire(&texture->resource);

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_clear_unordered_access_view(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_clear_unordered_access_view *op = data;
    struct wined3d_unordered_access_view *view = op->view;
    struct wined3d_context *context;

    context = context_acquire(cs->device, NULL, 0);
    cs->device->adapter->adapter_ops->adapter_clear_uav(context, view, &op->clear_value);
    context_release(context);

    wined3d_resource_release(view->resource);
}

void wined3d_cs_emit_clear_unordered_access_view_uint(struct wined3d_cs *cs,
        struct wined3d_unordered_access_view *view, const struct wined3d_uvec4 *clear_value)
{
    struct wined3d_cs_clear_unordered_access_view *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_CLEAR_UNORDERED_ACCESS_VIEW;
    op->view = view;
    op->clear_value = *clear_value;

    wined3d_resource_acquire(view->resource);

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_copy_uav_counter(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_copy_uav_counter *op = data;
    struct wined3d_unordered_access_view *view = op->view;
    struct wined3d_context *context;

    context = context_acquire(cs->device, NULL, 0);
    wined3d_unordered_access_view_copy_counter(view, op->buffer, op->offset, context);
    context_release(context);

    wined3d_resource_release(&op->buffer->resource);
    wined3d_resource_release(view->resource);
}

void wined3d_cs_emit_copy_uav_counter(struct wined3d_cs *cs, struct wined3d_buffer *dst_buffer,
        unsigned int offset, struct wined3d_unordered_access_view *uav)
{
    struct wined3d_cs_copy_uav_counter *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_COPY_UAV_COUNTER;
    op->buffer = dst_buffer;
    op->offset = offset;
    op->view = uav;

    wined3d_resource_acquire(&dst_buffer->resource);
    wined3d_resource_acquire(uav->resource);

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_generate_mipmaps(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_generate_mipmaps *op = data;
    struct wined3d_shader_resource_view *view = op->view;

    shader_resource_view_generate_mipmaps(view);
    wined3d_resource_release(view->resource);
}

void wined3d_cs_emit_generate_mipmaps(struct wined3d_cs *cs, struct wined3d_shader_resource_view *view)
{
    struct wined3d_cs_generate_mipmaps *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_GENERATE_MIPMAPS;
    op->view = view;

    wined3d_resource_acquire(view->resource);

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_emit_stop(struct wined3d_cs *cs)
{
    struct wined3d_cs_stop *op;

    op = wined3d_cs_require_space(cs, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_STOP;

    wined3d_cs_submit(cs, WINED3D_CS_QUEUE_DEFAULT);
    wined3d_cs_finish(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void (* const wined3d_cs_op_handlers[])(struct wined3d_cs *cs, const void *data) =
{
    /* WINED3D_CS_OP_NOP                         */ wined3d_cs_exec_nop,
    /* WINED3D_CS_OP_PRESENT                     */ wined3d_cs_exec_present,
    /* WINED3D_CS_OP_CLEAR                       */ wined3d_cs_exec_clear,
    /* WINED3D_CS_OP_DISPATCH                    */ wined3d_cs_exec_dispatch,
    /* WINED3D_CS_OP_DRAW                        */ wined3d_cs_exec_draw,
    /* WINED3D_CS_OP_FLUSH                       */ wined3d_cs_exec_flush,
    /* WINED3D_CS_OP_SET_PREDICATION             */ wined3d_cs_exec_set_predication,
    /* WINED3D_CS_OP_SET_VIEWPORTS               */ wined3d_cs_exec_set_viewports,
    /* WINED3D_CS_OP_SET_SCISSOR_RECTS           */ wined3d_cs_exec_set_scissor_rects,
    /* WINED3D_CS_OP_SET_RENDERTARGET_VIEW       */ wined3d_cs_exec_set_rendertarget_view,
    /* WINED3D_CS_OP_SET_DEPTH_STENCIL_VIEW      */ wined3d_cs_exec_set_depth_stencil_view,
    /* WINED3D_CS_OP_SET_VERTEX_DECLARATION      */ wined3d_cs_exec_set_vertex_declaration,
    /* WINED3D_CS_OP_SET_STREAM_SOURCE           */ wined3d_cs_exec_set_stream_source,
    /* WINED3D_CS_OP_SET_STREAM_SOURCE_FREQ      */ wined3d_cs_exec_set_stream_source_freq,
    /* WINED3D_CS_OP_SET_STREAM_OUTPUT           */ wined3d_cs_exec_set_stream_output,
    /* WINED3D_CS_OP_SET_INDEX_BUFFER            */ wined3d_cs_exec_set_index_buffer,
    /* WINED3D_CS_OP_SET_CONSTANT_BUFFER         */ wined3d_cs_exec_set_constant_buffer,
    /* WINED3D_CS_OP_SET_TEXTURE                 */ wined3d_cs_exec_set_texture,
    /* WINED3D_CS_OP_SET_SHADER_RESOURCE_VIEW    */ wined3d_cs_exec_set_shader_resource_view,
    /* WINED3D_CS_OP_SET_UNORDERED_ACCESS_VIEW   */ wined3d_cs_exec_set_unordered_access_view,
    /* WINED3D_CS_OP_SET_SAMPLER                 */ wined3d_cs_exec_set_sampler,
    /* WINED3D_CS_OP_SET_SHADER                  */ wined3d_cs_exec_set_shader,
    /* WINED3D_CS_OP_SET_BLEND_STATE             */ wined3d_cs_exec_set_blend_state,
    /* WINED3D_CS_OP_SET_RASTERIZER_STATE        */ wined3d_cs_exec_set_rasterizer_state,
    /* WINED3D_CS_OP_SET_RENDER_STATE            */ wined3d_cs_exec_set_render_state,
    /* WINED3D_CS_OP_SET_TEXTURE_STATE           */ wined3d_cs_exec_set_texture_state,
    /* WINED3D_CS_OP_SET_SAMPLER_STATE           */ wined3d_cs_exec_set_sampler_state,
    /* WINED3D_CS_OP_SET_TRANSFORM               */ wined3d_cs_exec_set_transform,
    /* WINED3D_CS_OP_SET_CLIP_PLANE              */ wined3d_cs_exec_set_clip_plane,
    /* WINED3D_CS_OP_SET_COLOR_KEY               */ wined3d_cs_exec_set_color_key,
    /* WINED3D_CS_OP_SET_MATERIAL                */ wined3d_cs_exec_set_material,
    /* WINED3D_CS_OP_SET_LIGHT                   */ wined3d_cs_exec_set_light,
    /* WINED3D_CS_OP_SET_LIGHT_ENABLE            */ wined3d_cs_exec_set_light_enable,
    /* WINED3D_CS_OP_PUSH_CONSTANTS              */ wined3d_cs_exec_push_constants,
    /* WINED3D_CS_OP_RESET_STATE                 */ wined3d_cs_exec_reset_state,
    /* WINED3D_CS_OP_CALLBACK                    */ wined3d_cs_exec_callback,
    /* WINED3D_CS_OP_QUERY_ISSUE                 */ wined3d_cs_exec_query_issue,
    /* WINED3D_CS_OP_PRELOAD_RESOURCE            */ wined3d_cs_exec_preload_resource,
    /* WINED3D_CS_OP_UNLOAD_RESOURCE             */ wined3d_cs_exec_unload_resource,
    /* WINED3D_CS_OP_MAP                         */ wined3d_cs_exec_map,
    /* WINED3D_CS_OP_UNMAP                       */ wined3d_cs_exec_unmap,
    /* WINED3D_CS_OP_BLT_SUB_RESOURCE            */ wined3d_cs_exec_blt_sub_resource,
    /* WINED3D_CS_OP_UPDATE_SUB_RESOURCE         */ wined3d_cs_exec_update_sub_resource,
    /* WINED3D_CS_OP_ADD_DIRTY_TEXTURE_REGION    */ wined3d_cs_exec_add_dirty_texture_region,
    /* WINED3D_CS_OP_CLEAR_UNORDERED_ACCESS_VIEW */ wined3d_cs_exec_clear_unordered_access_view,
    /* WINED3D_CS_OP_COPY_UAV_COUNTER            */ wined3d_cs_exec_copy_uav_counter,
    /* WINED3D_CS_OP_GENERATE_MIPMAPS            */ wined3d_cs_exec_generate_mipmaps,
};

#if defined(STAGING_CSMT)
static BOOL wined3d_cs_st_check_space(struct wined3d_cs *cs, size_t size, enum wined3d_cs_queue_id queue_id)
{
    return TRUE;
}

#endif /* STAGING_CSMT */
static void *wined3d_cs_st_require_space(struct wined3d_cs *cs, size_t size, enum wined3d_cs_queue_id queue_id)
{
    if (size > (cs->data_size - cs->end))
    {
        size_t new_size;
        void *new_data;

        new_size = max(size, cs->data_size * 2);
        if (!cs->end)
            new_data = heap_realloc(cs->data, new_size);
        else
            new_data = heap_alloc(new_size);
        if (!new_data)
            return NULL;

        cs->data_size = new_size;
        cs->start = cs->end = 0;
        cs->data = new_data;
    }

    cs->end += size;

    return (BYTE *)cs->data + cs->start;
}

static void wined3d_cs_st_submit(struct wined3d_cs *cs, enum wined3d_cs_queue_id queue_id)
{
    enum wined3d_cs_op opcode;
    size_t start;
    BYTE *data;

    data = cs->data;
    start = cs->start;
    cs->start = cs->end;

    opcode = *(const enum wined3d_cs_op *)&data[start];
    if (opcode >= WINED3D_CS_OP_STOP)
        ERR("Invalid opcode %#x.\n", opcode);
    else
        wined3d_cs_op_handlers[opcode](cs, &data[start]);

    if (cs->data == data)
        cs->start = cs->end = start;
    else if (!start)
        heap_free(data);
}

static void wined3d_cs_st_finish(struct wined3d_cs *cs, enum wined3d_cs_queue_id queue_id)
{
}

static const struct wined3d_cs_ops wined3d_cs_st_ops =
{
#if defined(STAGING_CSMT)
    wined3d_cs_st_check_space,
#endif /* STAGING_CSMT */
    wined3d_cs_st_require_space,
    wined3d_cs_st_submit,
    wined3d_cs_st_finish,
    wined3d_cs_st_push_constants,
};

static BOOL wined3d_cs_queue_is_empty(const struct wined3d_cs *cs, const struct wined3d_cs_queue *queue)
{
    wined3d_from_cs(cs);
    return *(volatile LONG *)&queue->head == queue->tail;
}

static void wined3d_cs_queue_submit(struct wined3d_cs_queue *queue, struct wined3d_cs *cs)
{
    struct wined3d_cs_packet *packet;
    size_t packet_size;

    packet = (struct wined3d_cs_packet *)&queue->data[queue->head];
    packet_size = FIELD_OFFSET(struct wined3d_cs_packet, data[packet->size]);
    InterlockedExchange(&queue->head, (queue->head + packet_size) & (WINED3D_CS_QUEUE_SIZE - 1));

    if (InterlockedCompareExchange(&cs->waiting_for_event, FALSE, TRUE))
        SetEvent(cs->event);
}

static void wined3d_cs_mt_submit(struct wined3d_cs *cs, enum wined3d_cs_queue_id queue_id)
{
    if (cs->thread_id == GetCurrentThreadId())
    {
        wined3d_cs_st_submit(cs, queue_id);
        return;
    }

    wined3d_cs_queue_submit(&cs->queue[queue_id], cs);
}

#if defined(STAGING_CSMT)
static BOOL wined3d_cs_queue_check_space(struct wined3d_cs_queue *queue, size_t size)
{
    size_t queue_size = ARRAY_SIZE(queue->data);
    size_t header_size, packet_size, remaining;

    header_size = FIELD_OFFSET(struct wined3d_cs_packet, data[0]);
    size = (size + header_size - 1) & ~(header_size - 1);
    packet_size = FIELD_OFFSET(struct wined3d_cs_packet, data[size]);

    remaining = queue_size - queue->head;
    return (remaining >= packet_size);
}

#endif /* STAGING_CSMT */
static void *wined3d_cs_queue_require_space(struct wined3d_cs_queue *queue, size_t size, struct wined3d_cs *cs)
{
    size_t queue_size = ARRAY_SIZE(queue->data);
    size_t header_size, packet_size, remaining;
    struct wined3d_cs_packet *packet;

    header_size = FIELD_OFFSET(struct wined3d_cs_packet, data[0]);
    size = (size + header_size - 1) & ~(header_size - 1);
    packet_size = FIELD_OFFSET(struct wined3d_cs_packet, data[size]);
    if (packet_size >= WINED3D_CS_QUEUE_SIZE)
    {
        ERR("Packet size %lu >= queue size %u.\n",
                (unsigned long)packet_size, WINED3D_CS_QUEUE_SIZE);
        return NULL;
    }

    remaining = queue_size - queue->head;
    if (remaining < packet_size)
    {
        size_t nop_size = remaining - header_size;
        struct wined3d_cs_nop *nop;

        TRACE("Inserting a nop for %lu + %lu bytes.\n",
                (unsigned long)header_size, (unsigned long)nop_size);

        nop = wined3d_cs_queue_require_space(queue, nop_size, cs);
        if (nop_size)
            nop->opcode = WINED3D_CS_OP_NOP;

        wined3d_cs_queue_submit(queue, cs);
        assert(!queue->head);
    }

    for (;;)
    {
        LONG tail = *(volatile LONG *)&queue->tail;
        LONG head = queue->head;
        LONG new_pos;

        /* Empty. */
        if (head == tail)
            break;
        new_pos = (head + packet_size) & (WINED3D_CS_QUEUE_SIZE - 1);
        /* Head ahead of tail. We checked the remaining size above, so we only
         * need to make sure we don't make head equal to tail. */
        if (head > tail && (new_pos != tail))
            break;
        /* Tail ahead of head. Make sure the new head is before the tail as
         * well. Note that new_pos is 0 when it's at the end of the queue. */
        if (new_pos < tail && new_pos)
            break;

        TRACE("Waiting for free space. Head %u, tail %u, packet size %lu.\n",
                head, tail, (unsigned long)packet_size);
    }

    packet = (struct wined3d_cs_packet *)&queue->data[queue->head];
    packet->size = size;
    return packet->data;
}

#if defined(STAGING_CSMT)
static BOOL wined3d_cs_mt_check_space(struct wined3d_cs *cs, size_t size, enum wined3d_cs_queue_id queue_id)
{
    if (cs->thread_id == GetCurrentThreadId())
        return wined3d_cs_st_check_space(cs, size, queue_id);

    return wined3d_cs_queue_check_space(&cs->queue[queue_id], size);
}

#endif /* STAGING_CSMT */
static void *wined3d_cs_mt_require_space(struct wined3d_cs *cs, size_t size, enum wined3d_cs_queue_id queue_id)
{
    if (cs->thread_id == GetCurrentThreadId())
        return wined3d_cs_st_require_space(cs, size, queue_id);

    return wined3d_cs_queue_require_space(&cs->queue[queue_id], size, cs);
}

static void wined3d_cs_mt_finish(struct wined3d_cs *cs, enum wined3d_cs_queue_id queue_id)
{
    if (cs->thread_id == GetCurrentThreadId())
    {
        wined3d_cs_st_finish(cs, queue_id);
        return;
    }

    while (cs->queue[queue_id].head != *(volatile LONG *)&cs->queue[queue_id].tail)
        wined3d_pause();
}

static const struct wined3d_cs_ops wined3d_cs_mt_ops =
{
#if defined(STAGING_CSMT)
    wined3d_cs_mt_check_space,
#endif /* STAGING_CSMT */
    wined3d_cs_mt_require_space,
    wined3d_cs_mt_submit,
    wined3d_cs_mt_finish,
    wined3d_cs_mt_push_constants,
};

static void poll_queries(struct wined3d_cs *cs)
{
    struct wined3d_query *query, *cursor;

    LIST_FOR_EACH_ENTRY_SAFE(query, cursor, &cs->query_poll_list, struct wined3d_query, poll_list_entry)
    {
        if (!query->query_ops->query_poll(query, 0))
            continue;

        list_remove(&query->poll_list_entry);
        list_init(&query->poll_list_entry);
        InterlockedIncrement(&query->counter_retrieved);
    }
}

static void wined3d_cs_wait_event(struct wined3d_cs *cs)
{
    InterlockedExchange(&cs->waiting_for_event, TRUE);

    /* The main thread might have enqueued a command and blocked on it after
     * the CS thread decided to enter wined3d_cs_wait_event(), but before
     * "waiting_for_event" was set.
     *
     * Likewise, we can race with the main thread when resetting
     * "waiting_for_event", in which case we would need to call
     * WaitForSingleObject() because the main thread called SetEvent(). */
    if (!(wined3d_cs_queue_is_empty(cs, &cs->queue[WINED3D_CS_QUEUE_DEFAULT])
            && wined3d_cs_queue_is_empty(cs, &cs->queue[WINED3D_CS_QUEUE_MAP]))
            && InterlockedCompareExchange(&cs->waiting_for_event, FALSE, TRUE))
        return;

    WaitForSingleObject(cs->event, INFINITE);
}

static DWORD WINAPI wined3d_cs_run(void *ctx)
{
    struct wined3d_cs_packet *packet;
    struct wined3d_cs_queue *queue;
    unsigned int spin_count = 0;
    struct wined3d_cs *cs = ctx;
    enum wined3d_cs_op opcode;
    HMODULE wined3d_module;
    unsigned int poll = 0;
    LONG tail;

    TRACE("Started.\n");

    /* Copy the module handle to a local variable to avoid racing with the
     * thread freeing "cs" before the FreeLibraryAndExitThread() call. */
    wined3d_module = cs->wined3d_module;

    list_init(&cs->query_poll_list);
    cs->thread_id = GetCurrentThreadId();
    for (;;)
    {
        if (++poll == WINED3D_CS_QUERY_POLL_INTERVAL)
        {
            poll_queries(cs);
            poll = 0;
        }

        queue = &cs->queue[WINED3D_CS_QUEUE_MAP];
        if (wined3d_cs_queue_is_empty(cs, queue))
        {
            queue = &cs->queue[WINED3D_CS_QUEUE_DEFAULT];
            if (wined3d_cs_queue_is_empty(cs, queue))
            {
                if (++spin_count >= WINED3D_CS_SPIN_COUNT && list_empty(&cs->query_poll_list))
                    wined3d_cs_wait_event(cs);
                continue;
            }
        }
        spin_count = 0;

        tail = queue->tail;
        packet = (struct wined3d_cs_packet *)&queue->data[tail];
        if (packet->size)
        {
            opcode = *(const enum wined3d_cs_op *)packet->data;

            TRACE("Executing %s.\n", debug_cs_op(opcode));
            if (opcode >= WINED3D_CS_OP_STOP)
            {
                if (opcode > WINED3D_CS_OP_STOP)
                    ERR("Invalid opcode %#x.\n", opcode);
                break;
            }

            wined3d_cs_op_handlers[opcode](cs, packet->data);
            TRACE("%s executed.\n", debug_cs_op(opcode));
        }

        tail += FIELD_OFFSET(struct wined3d_cs_packet, data[packet->size]);
        tail &= (WINED3D_CS_QUEUE_SIZE - 1);
        InterlockedExchange(&queue->tail, tail);
    }

    cs->queue[WINED3D_CS_QUEUE_MAP].tail = cs->queue[WINED3D_CS_QUEUE_MAP].head;
    cs->queue[WINED3D_CS_QUEUE_DEFAULT].tail = cs->queue[WINED3D_CS_QUEUE_DEFAULT].head;
    TRACE("Stopped.\n");
    FreeLibraryAndExitThread(wined3d_module, 0);
}

struct wined3d_cs *wined3d_cs_create(struct wined3d_device *device)
{
    const struct wined3d_d3d_info *d3d_info = &device->adapter->d3d_info;
    struct wined3d_cs *cs;

    if (!(cs = heap_alloc_zero(sizeof(*cs))))
        return NULL;

    cs->ops = &wined3d_cs_st_ops;
    cs->device = device;

    state_init(&cs->state, &cs->fb, d3d_info, WINED3D_STATE_NO_REF | WINED3D_STATE_INIT_DEFAULT);

    cs->data_size = WINED3D_INITIAL_CS_SIZE;
    if (!(cs->data = heap_alloc(cs->data_size)))
        goto fail;

    if (wined3d_settings.cs_multithreaded
            && !RtlIsCriticalSectionLockedByThread(NtCurrentTeb()->Peb->LoaderLock))
    {
        cs->ops = &wined3d_cs_mt_ops;

        if (!(cs->event = CreateEventW(NULL, FALSE, FALSE, NULL)))
        {
            ERR("Failed to create command stream event.\n");
            heap_free(cs->data);
            goto fail;
        }

        if (!(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                (const WCHAR *)wined3d_cs_run, &cs->wined3d_module)))
        {
            ERR("Failed to get wined3d module handle.\n");
            CloseHandle(cs->event);
            heap_free(cs->data);
            goto fail;
        }

        if (!(cs->thread = CreateThread(NULL, 0, wined3d_cs_run, cs, 0, NULL)))
        {
            ERR("Failed to create wined3d command stream thread.\n");
            FreeLibrary(cs->wined3d_module);
            CloseHandle(cs->event);
            heap_free(cs->data);
            goto fail;
        }
    }

    return cs;

fail:
    state_cleanup(&cs->state);
    heap_free(cs);
    return NULL;
}

void wined3d_cs_destroy(struct wined3d_cs *cs)
{
    if (cs->thread)
    {
        wined3d_cs_emit_stop(cs);
        CloseHandle(cs->thread);
        if (!CloseHandle(cs->event))
            ERR("Closing event failed.\n");
    }

    state_cleanup(&cs->state);
    heap_free(cs->data);
    heap_free(cs);
}
