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
#include "wined3d_gl.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_perf);
WINE_DECLARE_DEBUG_CHANNEL(d3d_sync);
WINE_DECLARE_DEBUG_CHANNEL(fps);
WINE_DECLARE_DEBUG_CHANNEL(frametime);

static NTSTATUS (WINAPI *pNtAlertThreadByThreadId)(HANDLE tid);
static NTSTATUS (WINAPI *pNtWaitForAlertByThreadId)(void *addr, const LARGE_INTEGER *timeout);

#define WINED3D_INITIAL_CS_SIZE 4096

struct wined3d_deferred_upload
{
    struct wined3d_resource *resource;
    unsigned int sub_resource_idx;
    struct wined3d_bo *bo;
    uint8_t *sysmem, *map_ptr;
    struct wined3d_box box;
    uint32_t upload_flags;
};

struct wined3d_deferred_query_issue
{
    struct wined3d_query *query;
    unsigned int flags;
};

struct wined3d_command_list
{
    LONG refcount;

    struct wined3d_device *device;

    SIZE_T data_size;
    void *data;

    SIZE_T resource_count;
    struct wined3d_resource **resources;

    SIZE_T upload_count;
    struct wined3d_deferred_upload *uploads;

    HANDLE upload_heap;
    LONG *upload_heap_refcount;

    /* List of command lists queued for execution on this command list. We might
     * be the only thing holding a pointer to another command list, so we need
     * to hold a reference here (and in wined3d_deferred_context) as well. */
    SIZE_T command_list_count;
    struct wined3d_command_list **command_lists;

    SIZE_T query_count;
    struct wined3d_deferred_query_issue *queries;
};

static void discard_client_address(struct wined3d_resource *resource)
{
    struct wined3d_client_resource *client = &resource->client;

    client->addr.buffer_object = CLIENT_BO_DISCARDED;
    client->addr.addr = NULL;
}

static void invalidate_client_address(struct wined3d_resource *resource)
{
    struct wined3d_client_resource *client = &resource->client;

    memset(&client->addr, 0, sizeof(client->addr));
}

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
    WINED3D_CS_OP_SET_RENDERTARGET_VIEWS,
    WINED3D_CS_OP_SET_DEPTH_STENCIL_VIEW,
    WINED3D_CS_OP_SET_VERTEX_DECLARATION,
    WINED3D_CS_OP_SET_STREAM_SOURCES,
    WINED3D_CS_OP_SET_STREAM_OUTPUTS,
    WINED3D_CS_OP_SET_INDEX_BUFFER,
    WINED3D_CS_OP_SET_CONSTANT_BUFFERS,
    WINED3D_CS_OP_SET_TEXTURE,
    WINED3D_CS_OP_SET_SHADER_RESOURCE_VIEWS,
    WINED3D_CS_OP_SET_UNORDERED_ACCESS_VIEWS,
    WINED3D_CS_OP_SET_SAMPLERS,
    WINED3D_CS_OP_SET_SHADER,
    WINED3D_CS_OP_SET_BLEND_STATE,
    WINED3D_CS_OP_SET_DEPTH_STENCIL_STATE,
    WINED3D_CS_OP_SET_RASTERIZER_STATE,
    WINED3D_CS_OP_SET_DEPTH_BOUNDS,
    WINED3D_CS_OP_SET_RENDER_STATE,
    WINED3D_CS_OP_SET_TEXTURE_STATE,
    WINED3D_CS_OP_SET_TRANSFORM,
    WINED3D_CS_OP_SET_CLIP_PLANE,
    WINED3D_CS_OP_SET_COLOR_KEY,
    WINED3D_CS_OP_SET_LIGHT,
    WINED3D_CS_OP_SET_LIGHT_ENABLE,
    WINED3D_CS_OP_SET_FEATURE_LEVEL,
    WINED3D_CS_OP_PUSH_CONSTANTS,
    WINED3D_CS_OP_RESET_STATE,
    WINED3D_CS_OP_CALLBACK,
    WINED3D_CS_OP_QUERY_ISSUE,
    WINED3D_CS_OP_PRELOAD_RESOURCE,
    WINED3D_CS_OP_UNLOAD_RESOURCE,
    WINED3D_CS_OP_MAP,
    WINED3D_CS_OP_UNMAP,
    WINED3D_CS_OP_MAP_BO_ADDRESS,
    WINED3D_CS_OP_BLT_SUB_RESOURCE,
    WINED3D_CS_OP_UPDATE_SUB_RESOURCE,
    WINED3D_CS_OP_ADD_DIRTY_TEXTURE_REGION,
    WINED3D_CS_OP_CLEAR_SYSMEM_TEXTURE,
    WINED3D_CS_OP_CLEAR_UNORDERED_ACCESS_VIEW,
    WINED3D_CS_OP_COPY_UAV_COUNTER,
    WINED3D_CS_OP_GENERATE_MIPMAPS,
    WINED3D_CS_OP_EXECUTE_COMMAND_LIST,
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
    unsigned int swap_interval;
    uint32_t flags;
};

struct wined3d_cs_clear
{
    enum wined3d_cs_op opcode;
    uint32_t flags;
    unsigned int rt_count;
    struct wined3d_fb_state fb;
    RECT draw_rect;
    struct wined3d_color color;
    float depth;
    DWORD stencil;
    unsigned int rect_count;
    RECT rects[1];
};

struct wined3d_cs_clear_sysmem_texture
{
    enum wined3d_cs_op opcode;
    struct wined3d_texture *texture;
    unsigned int sub_resource_idx;
    struct wined3d_color color;
    RECT rect;
};

struct wined3d_cs_dispatch
{
    enum wined3d_cs_op opcode;
    struct wined3d_dispatch_parameters parameters;
};

struct wined3d_cs_draw
{
    enum wined3d_cs_op opcode;
    enum wined3d_primitive_type primitive_type;
    unsigned int patch_vertex_count;
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

struct wined3d_cs_set_rendertarget_views
{
    enum wined3d_cs_op opcode;
    unsigned int start_idx;
    unsigned int count;
    struct wined3d_rendertarget_view *views[1];
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

struct wined3d_cs_set_stream_sources
{
    enum wined3d_cs_op opcode;
    unsigned int start_idx;
    unsigned int count;
    struct wined3d_stream_state streams[1];
};

struct wined3d_cs_set_stream_outputs
{
    enum wined3d_cs_op opcode;
    struct wined3d_stream_output outputs[WINED3D_MAX_STREAM_OUTPUT_BUFFERS];
};

struct wined3d_cs_set_index_buffer
{
    enum wined3d_cs_op opcode;
    struct wined3d_buffer *buffer;
    enum wined3d_format_id format_id;
    unsigned int offset;
};

struct wined3d_cs_set_constant_buffers
{
    enum wined3d_cs_op opcode;
    enum wined3d_shader_type type;
    unsigned int start_idx;
    unsigned int count;
    struct wined3d_constant_buffer_state buffers[1];
};

struct wined3d_cs_set_texture
{
    enum wined3d_cs_op opcode;
    enum wined3d_shader_type shader_type;
    unsigned int bind_index;
    struct wined3d_shader_resource_view *view;
};

struct wined3d_cs_set_color_key
{
    enum wined3d_cs_op opcode;
    struct wined3d_texture *texture;
    WORD flags;
    WORD set;
    struct wined3d_color_key color_key;
};

struct wined3d_cs_set_shader_resource_views
{
    enum wined3d_cs_op opcode;
    enum wined3d_shader_type type;
    unsigned int start_idx;
    unsigned int count;
    struct wined3d_shader_resource_view *views[1];
};

struct wined3d_cs_set_unordered_access_views
{
    enum wined3d_cs_op opcode;
    enum wined3d_pipeline pipeline;
    unsigned int start_idx;
    unsigned int count;
    struct
    {
        struct wined3d_unordered_access_view *view;
        unsigned int initial_count;
    } uavs[1];
};

struct wined3d_cs_set_samplers
{
    enum wined3d_cs_op opcode;
    enum wined3d_shader_type type;
    unsigned int start_idx;
    unsigned int count;
    struct wined3d_sampler *samplers[1];
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
    unsigned int sample_mask;
};

struct wined3d_cs_set_depth_stencil_state
{
    enum wined3d_cs_op opcode;
    struct wined3d_depth_stencil_state *state;
    unsigned int stencil_ref;
};

struct wined3d_cs_set_rasterizer_state
{
    enum wined3d_cs_op opcode;
    struct wined3d_rasterizer_state *state;
};

struct wined3d_cs_set_depth_bounds
{
    enum wined3d_cs_op opcode;
    bool enable;
    float min_depth, max_depth;
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

struct wined3d_cs_set_feature_level
{
    enum wined3d_cs_op opcode;
    enum wined3d_feature_level level;
};

struct wined3d_cs_push_constants
{
    enum wined3d_cs_op opcode;
    enum wined3d_push_constants type;
    unsigned int start_idx;
    unsigned int count;
    uint32_t update_mask;
};

struct wined3d_cs_reset_state
{
    enum wined3d_cs_op opcode;
    bool invalidate;
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
    uint32_t flags;
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
    void **map_ptr;
    const struct wined3d_box *box;
    uint32_t flags;
    HRESULT *hr;
};

struct wined3d_cs_unmap
{
    enum wined3d_cs_op opcode;
    struct wined3d_resource *resource;
    unsigned int sub_resource_idx;
    HRESULT *hr;
};

struct wined3d_cs_map_bo_address
{
    enum wined3d_cs_op opcode;
    struct wined3d_bo_address addr;
    size_t size;
    uint32_t flags;
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
    uint32_t flags;
    struct wined3d_blt_fx fx;
    enum wined3d_texture_filter_type filter;
};

struct wined3d_cs_update_sub_resource
{
    enum wined3d_cs_op opcode;
    struct wined3d_resource *resource;
    unsigned int sub_resource_idx;
    struct wined3d_box box;
    struct upload_bo bo;
    unsigned int row_pitch, slice_pitch;
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
    bool fp;
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

struct wined3d_cs_execute_command_list
{
    enum wined3d_cs_op opcode;
    struct wined3d_command_list *list;
};

struct wined3d_cs_stop
{
    enum wined3d_cs_op opcode;
};

static inline void *wined3d_device_context_require_space(struct wined3d_device_context *context,
        size_t size, enum wined3d_cs_queue_id queue_id)
{
    return context->ops->require_space(context, size, queue_id);
}

static inline void wined3d_device_context_submit(struct wined3d_device_context *context,
        enum wined3d_cs_queue_id queue_id)
{
    context->ops->submit(context, queue_id);
}

static inline void wined3d_device_context_finish(struct wined3d_device_context *context,
        enum wined3d_cs_queue_id queue_id)
{
    context->ops->finish(context, queue_id);
}

static inline void wined3d_device_context_reference_resource(struct wined3d_device_context *context,
        struct wined3d_resource *resource)
{
    context->ops->reference_resource(context, resource);
}

static struct wined3d_cs *wined3d_cs_from_context(struct wined3d_device_context *context)
{
    return CONTAINING_RECORD(context, struct wined3d_cs, c);
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
        WINED3D_TO_STR(WINED3D_CS_OP_SET_RENDERTARGET_VIEWS);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_DEPTH_STENCIL_VIEW);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_VERTEX_DECLARATION);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_STREAM_SOURCES);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_STREAM_OUTPUTS);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_INDEX_BUFFER);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_CONSTANT_BUFFERS);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_TEXTURE);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_SHADER_RESOURCE_VIEWS);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_UNORDERED_ACCESS_VIEWS);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_SAMPLERS);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_SHADER);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_BLEND_STATE);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_DEPTH_STENCIL_STATE);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_RASTERIZER_STATE);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_DEPTH_BOUNDS);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_RENDER_STATE);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_TEXTURE_STATE);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_TRANSFORM);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_CLIP_PLANE);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_COLOR_KEY);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_LIGHT);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_LIGHT_ENABLE);
        WINED3D_TO_STR(WINED3D_CS_OP_SET_FEATURE_LEVEL);
        WINED3D_TO_STR(WINED3D_CS_OP_PUSH_CONSTANTS);
        WINED3D_TO_STR(WINED3D_CS_OP_RESET_STATE);
        WINED3D_TO_STR(WINED3D_CS_OP_CALLBACK);
        WINED3D_TO_STR(WINED3D_CS_OP_QUERY_ISSUE);
        WINED3D_TO_STR(WINED3D_CS_OP_PRELOAD_RESOURCE);
        WINED3D_TO_STR(WINED3D_CS_OP_UNLOAD_RESOURCE);
        WINED3D_TO_STR(WINED3D_CS_OP_MAP);
        WINED3D_TO_STR(WINED3D_CS_OP_UNMAP);
        WINED3D_TO_STR(WINED3D_CS_OP_MAP_BO_ADDRESS);
        WINED3D_TO_STR(WINED3D_CS_OP_BLT_SUB_RESOURCE);
        WINED3D_TO_STR(WINED3D_CS_OP_UPDATE_SUB_RESOURCE);
        WINED3D_TO_STR(WINED3D_CS_OP_ADD_DIRTY_TEXTURE_REGION);
        WINED3D_TO_STR(WINED3D_CS_OP_CLEAR_SYSMEM_TEXTURE);
        WINED3D_TO_STR(WINED3D_CS_OP_CLEAR_UNORDERED_ACCESS_VIEW);
        WINED3D_TO_STR(WINED3D_CS_OP_COPY_UAV_COUNTER);
        WINED3D_TO_STR(WINED3D_CS_OP_GENERATE_MIPMAPS);
        WINED3D_TO_STR(WINED3D_CS_OP_EXECUTE_COMMAND_LIST);
        WINED3D_TO_STR(WINED3D_CS_OP_STOP);
#undef WINED3D_TO_STR
    }
    return wine_dbg_sprintf("UNKNOWN_OP(%#x)", op);
}

static struct wined3d_cs_packet *wined3d_next_cs_packet(const uint8_t *data, SIZE_T *offset, SIZE_T mask)
{
    struct wined3d_cs_packet *packet = (struct wined3d_cs_packet *)&data[*offset & mask];

    *offset += offsetof(struct wined3d_cs_packet, data[packet->size]);

    return packet;
}

static void wined3d_cs_exec_nop(struct wined3d_cs *cs, const void *data)
{
}

static void wined3d_cs_exec_present(struct wined3d_cs *cs, const void *data)
{
    static LARGE_INTEGER freq;

    struct wined3d_texture *logo_texture, *cursor_texture, *back_buffer;
    struct wined3d_rendertarget_view *dsv = cs->state.fb.depth_stencil;
    const struct wined3d_cs_present *op = data;
    const struct wined3d_swapchain_desc *desc;
    struct wined3d_swapchain *swapchain;
    LONGLONG elapsed_time;
    LARGE_INTEGER time;

    if (!freq.QuadPart)
        QueryPerformanceFrequency(&freq);

    swapchain = op->swapchain;
    desc = &swapchain->state.desc;
    back_buffer = swapchain->back_buffers[0];
    wined3d_swapchain_set_window(swapchain, op->dst_window_override);

    if ((logo_texture = swapchain->device->logo_texture))
    {
        RECT rect = {0, 0, logo_texture->resource.width, logo_texture->resource.height};

        /* Blit the logo into the upper left corner of the back-buffer. */
        wined3d_device_context_blt(&cs->c, back_buffer, 0, &rect, logo_texture, 0,
                &rect, WINED3D_BLT_SRC_CKEY, NULL, WINED3D_TEXF_POINT);
    }

    if ((cursor_texture = swapchain->device->cursor_texture)
            && swapchain->device->bCursorVisible && !swapchain->device->hardwareCursor)
    {
        RECT dst_rect =
        {
            swapchain->device->xScreenSpace - swapchain->device->xHotSpot,
            swapchain->device->yScreenSpace - swapchain->device->yHotSpot,
            swapchain->device->xScreenSpace + swapchain->device->cursorWidth - swapchain->device->xHotSpot,
            swapchain->device->yScreenSpace + swapchain->device->cursorHeight - swapchain->device->yHotSpot,
        };
        RECT src_rect =
        {
            0, 0, cursor_texture->resource.width, cursor_texture->resource.height
        };
        const RECT clip_rect = {0, 0, back_buffer->resource.width, back_buffer->resource.height};

        TRACE("Rendering the software cursor.\n");

        if (desc->windowed)
            MapWindowPoints(NULL, swapchain->win_handle, (POINT *)&dst_rect, 2);
        if (wined3d_clip_blit(&clip_rect, &dst_rect, &src_rect))
            wined3d_device_context_blt(&cs->c, back_buffer, 0, &dst_rect, cursor_texture, 0,
                    &src_rect, WINED3D_BLT_ALPHA_TEST, NULL, WINED3D_TEXF_POINT);
    }

    swapchain->swapchain_ops->swapchain_present(swapchain, &op->src_rect, &op->dst_rect, op->swap_interval, op->flags);

    /* Discard buffers if the swap effect allows it. */
    back_buffer = swapchain->back_buffers[desc->backbuffer_count - 1];
    if (desc->swap_effect == WINED3D_SWAP_EFFECT_DISCARD || desc->swap_effect == WINED3D_SWAP_EFFECT_FLIP_DISCARD)
        wined3d_texture_validate_location(back_buffer, 0, WINED3D_LOCATION_DISCARDED);

    if (dsv && dsv->resource->type != WINED3D_RTYPE_BUFFER)
    {
        struct wined3d_texture *ds = texture_from_resource(dsv->resource);

        if ((desc->flags & WINED3D_SWAPCHAIN_DISCARD_DEPTHSTENCIL || ds->flags & WINED3D_TEXTURE_DISCARD))
            wined3d_rendertarget_view_validate_location(dsv, WINED3D_LOCATION_DISCARDED);
    }

    if (TRACE_ON(frametime))
    {
        QueryPerformanceCounter(&time);
        if (swapchain->last_present_time.QuadPart)
        {
            elapsed_time = time.QuadPart - swapchain->last_present_time.QuadPart;
            TRACE_(frametime)("Frame duration %u μs.\n", (unsigned int)(elapsed_time * 1000000 / freq.QuadPart));
        }
        swapchain->last_present_time = time;
    }
    if (TRACE_ON(fps))
    {
        DWORD time = GetTickCount();
        ++swapchain->frames;

        /* every 1.5 seconds */
        if (time - swapchain->prev_time > 1500)
        {
            TRACE_(fps)("%p @ approx %.2ffps\n",
                    swapchain, 1000.0 * swapchain->frames / (time - swapchain->prev_time));
            swapchain->prev_time = time;
            swapchain->frames = 0;
        }
    }

    InterlockedDecrement(&cs->pending_presents);
    if (InterlockedCompareExchange(&cs->waiting_for_present, FALSE, TRUE))
        SetEvent(cs->present_event);
}

void wined3d_cs_emit_present(struct wined3d_cs *cs, struct wined3d_swapchain *swapchain,
        const RECT *src_rect, const RECT *dst_rect, HWND dst_window_override,
        unsigned int swap_interval, uint32_t flags)
{
    struct wined3d_cs_present *op;
    unsigned int i;
    LONG pending;

    wined3d_not_from_cs(cs);

    op = wined3d_device_context_require_space(&cs->c, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_PRESENT;
    op->dst_window_override = dst_window_override;
    op->swapchain = swapchain;
    op->src_rect = *src_rect;
    op->dst_rect = *dst_rect;
    op->swap_interval = swap_interval;
    op->flags = flags;

    pending = InterlockedIncrement(&cs->pending_presents);

    wined3d_resource_reference(&swapchain->front_buffer->resource);
    for (i = 0; i < swapchain->state.desc.backbuffer_count; ++i)
    {
        wined3d_resource_reference(&swapchain->back_buffers[i]->resource);
    }

    wined3d_device_context_submit(&cs->c, WINED3D_CS_QUEUE_DEFAULT);

    /* Limit input latency by limiting the number of presents that we can get
     * ahead of the worker thread. */
    while (pending >= swapchain->max_frame_latency)
    {
        InterlockedExchange(&cs->waiting_for_present, TRUE);

        pending = InterlockedCompareExchange(&cs->pending_presents, 0, 0);
        if (pending >= swapchain->max_frame_latency || !InterlockedCompareExchange(&cs->waiting_for_present, FALSE, TRUE))
        {
            TRACE_(d3d_perf)("Reached latency limit (%u frames), blocking to wait.\n", swapchain->max_frame_latency);
            wined3d_mutex_unlock();
            WaitForSingleObject(cs->present_event, INFINITE);
            wined3d_mutex_lock();
            TRACE_(d3d_perf)("Woken up from the wait.\n");
        }
    }
}

static void wined3d_cs_exec_clear(struct wined3d_cs *cs, const void *data)
{
    struct wined3d_device *device = cs->c.device;
    const struct wined3d_cs_clear *op = data;

    device->blitter->ops->blitter_clear(device->blitter, device, op->rt_count, &op->fb,
            op->rect_count, op->rects, &op->draw_rect, op->flags, &op->color, op->depth, op->stencil);
}

void wined3d_cs_emit_clear(struct wined3d_cs *cs, DWORD rect_count, const RECT *rects,
        uint32_t flags, const struct wined3d_color *color, float depth, DWORD stencil)
{
    const struct wined3d_state *state = cs->c.state;
    const struct wined3d_viewport *vp = &state->viewports[0];
    struct wined3d_rendertarget_view *view;
    struct wined3d_cs_clear *op;
    unsigned int rt_count, i;

    rt_count = flags & WINED3DCLEAR_TARGET ? cs->c.device->adapter->d3d_info.limits.max_rt_count : 0;

    op = wined3d_device_context_require_space(&cs->c, FIELD_OFFSET(struct wined3d_cs_clear, rects[rect_count]),
            WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_CLEAR;
    op->flags = flags & (WINED3DCLEAR_TARGET | WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL);
    op->rt_count = rt_count;
    op->fb = state->fb;
    SetRect(&op->draw_rect, vp->x, vp->y, vp->x + vp->width, vp->y + vp->height);
    if (state->rasterizer_state && state->rasterizer_state->desc.scissor)
        IntersectRect(&op->draw_rect, &op->draw_rect, &state->scissor_rects[0]);
    op->color = *color;
    op->depth = depth;
    op->stencil = stencil;
    op->rect_count = rect_count;
    memcpy(op->rects, rects, sizeof(*rects) * rect_count);

    for (i = 0; i < rt_count; ++i)
    {
        if ((view = state->fb.render_targets[i]))
            wined3d_resource_reference(view->resource);
    }
    if (flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL))
    {
        view = state->fb.depth_stencil;
        wined3d_resource_reference(view->resource);
    }

    wined3d_device_context_submit(&cs->c, WINED3D_CS_QUEUE_DEFAULT);
}

void wined3d_device_context_emit_clear_rendertarget_view(struct wined3d_device_context *context,
        struct wined3d_rendertarget_view *view, const RECT *rect, unsigned int flags,
        const struct wined3d_color *color, float depth, unsigned int stencil)
{
    struct wined3d_cs_clear *op;
    size_t size;

    size = FIELD_OFFSET(struct wined3d_cs_clear, rects[1]);
    op = wined3d_device_context_require_space(context, size, WINED3D_CS_QUEUE_DEFAULT);

    op->opcode = WINED3D_CS_OP_CLEAR;
    op->flags = flags & (WINED3DCLEAR_TARGET | WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL);
    if (flags & WINED3DCLEAR_TARGET)
    {
        op->rt_count = 1;
        op->fb.render_targets[0] = view;
        op->fb.depth_stencil = NULL;
        op->color = *color;
    }
    else
    {
        op->rt_count = 0;
        op->fb.render_targets[0] = NULL;
        op->fb.depth_stencil = view;
        op->depth = depth;
        op->stencil = stencil;
    }
    SetRect(&op->draw_rect, 0, 0, view->width, view->height);
    op->rect_count = 1;
    op->rects[0] = *rect;

    wined3d_device_context_reference_resource(context, view->resource);

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
    if (flags & WINED3DCLEAR_SYNCHRONOUS)
        wined3d_device_context_finish(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_clear_sysmem_texture(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_clear_sysmem_texture *op = data;
    struct wined3d_box box;

    box.left = op->rect.left;
    box.right = op->rect.right;
    box.top = op->rect.top;
    box.bottom = op->rect.bottom;
    box.front = 0;
    box.back = 1;
    cpu_blitter_clear_texture(op->texture, op->sub_resource_idx, &box, &op->color);
}

HRESULT CDECL wined3d_device_context_clear_sysmem_texture(struct wined3d_device_context *context,
        struct wined3d_texture *texture, unsigned int sub_resource_idx, const RECT *rect,
        unsigned int flags, const struct wined3d_color *color)
{
    struct wined3d_cs_clear_sysmem_texture *op;

    TRACE("context %p, texture %p, sub_resource_idx %u, rect %s, flags %#x, color %s.\n",
            context, texture, sub_resource_idx, wine_dbgstr_rect(rect), flags, debug_color(color));

    if (rect)
    {
        struct wined3d_box b = {rect->left, rect->top, rect->right, rect->bottom, 0, 1};
        HRESULT hr;

        if (FAILED(hr = wined3d_resource_check_box_dimensions(&texture->resource, sub_resource_idx, &b)))
            return hr;
    }

    wined3d_device_context_lock(context);

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_CLEAR_SYSMEM_TEXTURE;
    op->texture = texture;
    op->sub_resource_idx = sub_resource_idx;
    op->color = *color;

    if (rect)
    {
        op->rect = *rect;
    }
    else
    {
        unsigned int level_idx = sub_resource_idx % texture->level_count;

        op->rect.left = 0;
        op->rect.top = 0;
        op->rect.right = wined3d_texture_get_level_width(texture, level_idx);
        op->rect.bottom = wined3d_texture_get_level_height(texture, level_idx);
    }

    wined3d_device_context_reference_resource(context, &texture->resource);

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
    if (flags & WINED3DCLEAR_SYNCHRONOUS)
        wined3d_device_context_finish(context, WINED3D_CS_QUEUE_DEFAULT);

    wined3d_device_context_unlock(context);
    return S_OK;
}

static void reference_shader_resources(struct wined3d_device_context *context, unsigned int shader_mask)
{
    const struct wined3d_state *state = context->state;
    struct wined3d_shader_sampler_map_entry *entry;
    struct wined3d_shader_resource_view *view;
    struct wined3d_shader *shader;
    unsigned int i, j;

    for (i = 0; i < WINED3D_SHADER_TYPE_COUNT; ++i)
    {
        if (!(shader_mask & (1u << i)))
            continue;

        if (!(shader = state->shader[i]))
        {
            if (i == WINED3D_SHADER_TYPE_PIXEL)
            {
                for (j = 0; j < WINED3D_MAX_FFP_TEXTURES; ++j)
                {
                    if ((view = state->shader_resource_view[WINED3D_SHADER_TYPE_PIXEL][j]))
                        wined3d_device_context_reference_resource(context, view->resource);
                }
            }
            continue;
        }

        for (j = 0; j < WINED3D_MAX_CBS; ++j)
        {
            if (state->cb[i][j].buffer)
                wined3d_device_context_reference_resource(context, &state->cb[i][j].buffer->resource);
        }

        for (j = 0; j < shader->reg_maps.sampler_map.count; ++j)
        {
            entry = &shader->reg_maps.sampler_map.entries[j];

            if (!(view = state->shader_resource_view[i][entry->resource_idx]))
                continue;

            wined3d_device_context_reference_resource(context, view->resource);
        }
    }
}

static void reference_unordered_access_resources(struct wined3d_device_context *context,
        const struct wined3d_shader *shader, struct wined3d_unordered_access_view * const *views)
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

        wined3d_device_context_reference_resource(context, views[i]->resource);
    }
}

static void wined3d_cs_exec_dispatch(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_dispatch *op = data;
    struct wined3d_state *state = &cs->state;

    if (!state->shader[WINED3D_SHADER_TYPE_COMPUTE])
        WARN("No compute shader bound, skipping dispatch.\n");
    else
        cs->c.device->adapter->adapter_ops->adapter_dispatch_compute(cs->c.device, state, &op->parameters);
}

static void reference_compute_pipeline_resources(struct wined3d_device_context *context)
{
    const struct wined3d_state *state = context->state;

    reference_shader_resources(context, 1u << WINED3D_SHADER_TYPE_COMPUTE);
    reference_unordered_access_resources(context, state->shader[WINED3D_SHADER_TYPE_COMPUTE],
            state->unordered_access_view[WINED3D_PIPELINE_COMPUTE]);
}

void CDECL wined3d_device_context_dispatch(struct wined3d_device_context *context,
        unsigned int group_count_x, unsigned int group_count_y, unsigned int group_count_z)
{
    struct wined3d_cs_dispatch *op;

    wined3d_device_context_lock(context);
    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_DISPATCH;
    op->parameters.indirect = FALSE;
    op->parameters.u.direct.group_count_x = group_count_x;
    op->parameters.u.direct.group_count_y = group_count_y;
    op->parameters.u.direct.group_count_z = group_count_z;

    reference_compute_pipeline_resources(context);

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
    wined3d_device_context_unlock(context);
}

void CDECL wined3d_device_context_dispatch_indirect(struct wined3d_device_context *context,
        struct wined3d_buffer *buffer, unsigned int offset)
{
    struct wined3d_cs_dispatch *op;

    wined3d_device_context_lock(context);
    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_DISPATCH;
    op->parameters.indirect = TRUE;
    op->parameters.u.indirect.buffer = buffer;
    op->parameters.u.indirect.offset = offset;

    reference_compute_pipeline_resources(context);
    wined3d_device_context_reference_resource(context, &buffer->resource);

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
    wined3d_device_context_unlock(context);
}

static void wined3d_cs_exec_draw(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_d3d_info *d3d_info = &cs->c.device->adapter->d3d_info;
    const struct wined3d_shader *geometry_shader;
    struct wined3d_device *device = cs->c.device;
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
        device_invalidate_state(cs->c.device, STATE_BASEVERTEXINDEX);
    }

    if (state->primitive_type != op->primitive_type)
    {
        if ((geometry_shader = state->shader[WINED3D_SHADER_TYPE_GEOMETRY]) && !geometry_shader->function)
            device_invalidate_state(cs->c.device, STATE_SHADER(WINED3D_SHADER_TYPE_GEOMETRY));
        if (state->primitive_type == WINED3D_PT_POINTLIST || op->primitive_type == WINED3D_PT_POINTLIST)
        {
            device_invalidate_state(cs->c.device, STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX));
            device_invalidate_state(cs->c.device, STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL));
        }
        state->primitive_type = op->primitive_type;
        for (i = 0; i < device->context_count; ++i)
            device->contexts[i]->update_primitive_type = 1;
    }
    if (state->patch_vertex_count != op->patch_vertex_count)
    {
        for (i = 0; i < device->context_count; ++i)
            device->contexts[i]->update_patch_vertex_count = 1;
    }
    state->patch_vertex_count = op->patch_vertex_count;

    cs->c.device->adapter->adapter_ops->adapter_draw_primitive(cs->c.device, state, &op->parameters);
}

static void reference_graphics_pipeline_resources(struct wined3d_device_context *context,
        BOOL indexed, const struct wined3d_d3d_info *d3d_info)
{
    const struct wined3d_state *state = context->state;
    unsigned int i;

    if (indexed)
        wined3d_device_context_reference_resource(context, &state->index_buffer->resource);
    for (i = 0; i < ARRAY_SIZE(state->streams); ++i)
    {
        if (state->streams[i].buffer)
            wined3d_device_context_reference_resource(context, &state->streams[i].buffer->resource);
    }
    for (i = 0; i < ARRAY_SIZE(state->stream_output); ++i)
    {
        if (state->stream_output[i].buffer)
            wined3d_device_context_reference_resource(context, &state->stream_output[i].buffer->resource);
    }
    for (i = 0; i < d3d_info->limits.max_rt_count; ++i)
    {
        if (state->fb.render_targets[i])
            wined3d_device_context_reference_resource(context, state->fb.render_targets[i]->resource);
    }
    for (i = 0; i < WINED3D_PUSH_CONSTANTS_COUNT; ++i)
    {
        if (context->device->push_constants[i])
            wined3d_device_context_reference_resource(context, &context->device->push_constants[i]->resource);
    }
    if (state->fb.depth_stencil)
        wined3d_device_context_reference_resource(context, state->fb.depth_stencil->resource);
    reference_shader_resources(context, ~(1u << WINED3D_SHADER_TYPE_COMPUTE));
    reference_unordered_access_resources(context, state->shader[WINED3D_SHADER_TYPE_PIXEL],
            state->unordered_access_view[WINED3D_PIPELINE_GRAPHICS]);
}

void wined3d_device_context_emit_draw(struct wined3d_device_context *context,
        enum wined3d_primitive_type primitive_type, unsigned int patch_vertex_count, int base_vertex_idx,
        unsigned int start_idx, unsigned int index_count, unsigned int start_instance, unsigned int instance_count,
        bool indexed)
{
    const struct wined3d_d3d_info *d3d_info = &context->device->adapter->d3d_info;
    struct wined3d_cs_draw *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
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

    reference_graphics_pipeline_resources(context, indexed, d3d_info);

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

void CDECL wined3d_device_context_draw_indirect(struct wined3d_device_context *context,
        struct wined3d_buffer *buffer, unsigned int offset, bool indexed)
{
    const struct wined3d_d3d_info *d3d_info = &context->device->adapter->d3d_info;
    const struct wined3d_state *state = context->state;
    struct wined3d_cs_draw *op;

    wined3d_device_context_lock(context);
    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_DRAW;
    op->primitive_type = state->primitive_type;
    op->patch_vertex_count = state->patch_vertex_count;
    op->parameters.indirect = TRUE;
    op->parameters.u.indirect.buffer = buffer;
    op->parameters.u.indirect.offset = offset;
    op->parameters.indexed = indexed;

    reference_graphics_pipeline_resources(context, indexed, d3d_info);
    wined3d_device_context_reference_resource(context, &buffer->resource);

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
    wined3d_device_context_unlock(context);
}

static void wined3d_cs_exec_flush(struct wined3d_cs *cs, const void *data)
{
    struct wined3d_context *context;

    TRACE_(d3d_perf)("Flushing adapter.\n");
    context = context_acquire(cs->c.device, NULL, 0);
    cs->c.device->adapter->adapter_ops->adapter_flush_context(context);
    context_release(context);
}

static void wined3d_cs_flush(struct wined3d_device_context *context)
{
    struct wined3d_cs *cs = wined3d_cs_from_context(context);
    struct wined3d_cs_flush *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_FLUSH;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
    cs->queries_flushed = TRUE;
}

static void wined3d_cs_exec_set_predication(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_predication *op = data;

    cs->state.predicate = op->predicate;
    cs->state.predicate_value = op->value;
}

void wined3d_device_context_emit_set_predication(struct wined3d_device_context *context,
        struct wined3d_query *predicate, BOOL value)
{
    struct wined3d_cs_set_predication *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_PREDICATION;
    op->predicate = predicate;
    op->value = value;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_viewports(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_viewports *op = data;

    if (op->viewport_count)
        memcpy(cs->state.viewports, op->viewports, op->viewport_count * sizeof(*op->viewports));
    else
        memset(cs->state.viewports, 0, sizeof(*cs->state.viewports));
    cs->state.viewport_count = op->viewport_count;
    device_invalidate_state(cs->c.device, STATE_VIEWPORT);
}

void wined3d_device_context_emit_set_viewports(struct wined3d_device_context *context, unsigned int viewport_count,
        const struct wined3d_viewport *viewports)
{
    struct wined3d_cs_set_viewports *op;

    op = wined3d_device_context_require_space(context,
            FIELD_OFFSET(struct wined3d_cs_set_viewports,viewports[viewport_count]), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_VIEWPORTS;
    memcpy(op->viewports, viewports, viewport_count * sizeof(*viewports));
    op->viewport_count = viewport_count;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_scissor_rects(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_scissor_rects *op = data;

    if (op->rect_count)
        memcpy(cs->state.scissor_rects, op->rects, op->rect_count * sizeof(*op->rects));
    else
        SetRectEmpty(cs->state.scissor_rects);
    cs->state.scissor_rect_count = op->rect_count;
    device_invalidate_state(cs->c.device, STATE_SCISSORRECT);
}

void wined3d_device_context_emit_set_scissor_rects(struct wined3d_device_context *context,
        unsigned int rect_count, const RECT *rects)
{
    struct wined3d_cs_set_scissor_rects *op;

    op = wined3d_device_context_require_space(context, FIELD_OFFSET(struct wined3d_cs_set_scissor_rects, rects[rect_count]),
            WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_SCISSOR_RECTS;
    memcpy(op->rects, rects, rect_count * sizeof(*rects));
    op->rect_count = rect_count;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_rendertarget_views(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_rendertarget_views *op = data;
    struct wined3d_device *device = cs->c.device;
    unsigned int i;

    for (i = 0; i < op->count; ++i)
    {
        struct wined3d_rendertarget_view *prev = cs->state.fb.render_targets[op->start_idx + i];
        struct wined3d_rendertarget_view *view = op->views[i];
        bool prev_alpha_swizzle, curr_alpha_swizzle;
        bool prev_srgb_write, curr_srgb_write;

        cs->state.fb.render_targets[op->start_idx + i] = view;

        prev_alpha_swizzle = prev && prev->format->id == WINED3DFMT_A8_UNORM;
        curr_alpha_swizzle = view && view->format->id == WINED3DFMT_A8_UNORM;
        if (prev_alpha_swizzle != curr_alpha_swizzle)
            device_invalidate_state(device, STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL));

        if (!(device->adapter->d3d_info.wined3d_creation_flags & WINED3D_SRGB_READ_WRITE_CONTROL)
                || cs->state.render_states[WINED3D_RS_SRGBWRITEENABLE])
        {
            prev_srgb_write = prev && prev->format_caps & WINED3D_FORMAT_CAP_SRGB_WRITE;
            curr_srgb_write = view && view->format_caps & WINED3D_FORMAT_CAP_SRGB_WRITE;
            if (prev_srgb_write != curr_srgb_write)
                device_invalidate_state(device, STATE_RENDER(WINED3D_RS_SRGBWRITEENABLE));
        }
    }

    device_invalidate_state(device, STATE_FRAMEBUFFER);
}

void wined3d_device_context_emit_set_rendertarget_views(struct wined3d_device_context *context,
        unsigned int start_idx, unsigned int count, struct wined3d_rendertarget_view *const *views)
{
    struct wined3d_cs_set_rendertarget_views *op;

    op = wined3d_device_context_require_space(context,
            offsetof(struct wined3d_cs_set_rendertarget_views, views[count]), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_RENDERTARGET_VIEWS;
    op->start_idx = start_idx;
    op->count = count;
    memcpy(op->views, views, count * sizeof(*views));

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_depth_stencil_view(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_depth_stencil_view *op = data;
    struct wined3d_device *device = cs->c.device;
    struct wined3d_rendertarget_view *prev;

    if ((prev = cs->state.fb.depth_stencil) && prev->resource->type != WINED3D_RTYPE_BUFFER)
    {
        struct wined3d_texture *prev_texture = texture_from_resource(prev->resource);

        if (device->swapchains[0]->state.desc.flags & WINED3D_SWAPCHAIN_DISCARD_DEPTHSTENCIL
                || prev_texture->flags & WINED3D_TEXTURE_DISCARD)
            wined3d_texture_validate_location(prev_texture,
                    prev->sub_resource_idx, WINED3D_LOCATION_DISCARDED);
    }

    cs->state.fb.depth_stencil = op->view;

    if (!prev != !op->view)
    {
        /* Swapping NULL / non NULL depth stencil affects the depth and tests */
        device_invalidate_state(device, STATE_DEPTH_STENCIL);
        device_invalidate_state(device, STATE_STENCIL_REF);
        device_invalidate_state(device, STATE_RASTERIZER);
    }
    else if (prev)
    {
        if (prev->format->depth_bias_scale != op->view->format->depth_bias_scale)
            device_invalidate_state(device, STATE_RASTERIZER);
        if (prev->format->stencil_size != op->view->format->stencil_size)
            device_invalidate_state(device, STATE_STENCIL_REF);
    }

    device_invalidate_state(device, STATE_FRAMEBUFFER);
}

void wined3d_device_context_emit_set_depth_stencil_view(struct wined3d_device_context *context,
        struct wined3d_rendertarget_view *view)
{
    struct wined3d_cs_set_depth_stencil_view *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_DEPTH_STENCIL_VIEW;
    op->view = view;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_vertex_declaration(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_vertex_declaration *op = data;

    cs->state.vertex_declaration = op->declaration;
    device_invalidate_state(cs->c.device, STATE_VDECL);
}

void wined3d_device_context_emit_set_vertex_declaration(struct wined3d_device_context *context,
        struct wined3d_vertex_declaration *declaration)
{
    struct wined3d_cs_set_vertex_declaration *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_VERTEX_DECLARATION;
    op->declaration = declaration;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_stream_sources(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_stream_sources *op = data;
    unsigned int i;

    for (i = 0; i < op->count; ++i)
    {
        struct wined3d_buffer *prev = cs->state.streams[op->start_idx + i].buffer;
        struct wined3d_buffer *buffer = op->streams[i].buffer;

        if (buffer)
            ++buffer->resource.bind_count;
        if (prev)
            --prev->resource.bind_count;
    }

    memcpy(&cs->state.streams[op->start_idx], op->streams, op->count * sizeof(*op->streams));
    device_invalidate_state(cs->c.device, STATE_STREAMSRC);
}

void wined3d_device_context_emit_set_stream_sources(struct wined3d_device_context *context,
        unsigned int start_idx, unsigned int count, const struct wined3d_stream_state *streams)
{
    struct wined3d_cs_set_stream_sources *op;

    op = wined3d_device_context_require_space(context,
            offsetof(struct wined3d_cs_set_stream_sources, streams[count]), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_STREAM_SOURCES;
    op->start_idx = start_idx;
    op->count = count;
    memcpy(op->streams, streams, count * sizeof(*streams));

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_stream_outputs(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_stream_outputs *op = data;
    unsigned int i;

    for (i = 0; i < WINED3D_MAX_STREAM_OUTPUT_BUFFERS; ++i)
    {
        struct wined3d_buffer *prev = cs->state.stream_output[i].buffer;
        struct wined3d_buffer *buffer = op->outputs[i].buffer;

        if (buffer)
            ++buffer->resource.bind_count;
        if (prev)
            --prev->resource.bind_count;
    }

    memcpy(cs->state.stream_output, op->outputs, sizeof(op->outputs));
    device_invalidate_state(cs->c.device, STATE_STREAM_OUTPUT);
}

void wined3d_device_context_emit_set_stream_outputs(struct wined3d_device_context *context,
        const struct wined3d_stream_output outputs[WINED3D_MAX_STREAM_OUTPUT_BUFFERS])
{
    struct wined3d_cs_set_stream_outputs *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_STREAM_OUTPUTS;
    memcpy(op->outputs, outputs, sizeof(op->outputs));

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
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
        ++op->buffer->resource.bind_count;
    if (prev)
        --prev->resource.bind_count;

    device_invalidate_state(cs->c.device, STATE_INDEXBUFFER);
}

void wined3d_device_context_emit_set_index_buffer(struct wined3d_device_context *context, struct wined3d_buffer *buffer,
        enum wined3d_format_id format_id, unsigned int offset)
{
    struct wined3d_cs_set_index_buffer *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_INDEX_BUFFER;
    op->buffer = buffer;
    op->format_id = format_id;
    op->offset = offset;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_constant_buffers(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_constant_buffers *op = data;
    unsigned int i;

    for (i = 0; i < op->count; ++i)
    {
        struct wined3d_buffer *prev = cs->state.cb[op->type][op->start_idx + i].buffer;
        struct wined3d_buffer *buffer = op->buffers[i].buffer;

        cs->state.cb[op->type][op->start_idx + i] = op->buffers[i];

        if (buffer)
            ++buffer->resource.bind_count;
        if (prev)
            --prev->resource.bind_count;
    }

    device_invalidate_state(cs->c.device, STATE_CONSTANT_BUFFER(op->type));
}

void wined3d_device_context_emit_set_constant_buffers(struct wined3d_device_context *context,
        enum wined3d_shader_type type, unsigned int start_idx, unsigned int count,
        const struct wined3d_constant_buffer_state *buffers)
{
    struct wined3d_cs_set_constant_buffers *op;

    op = wined3d_device_context_require_space(context, offsetof(struct wined3d_cs_set_constant_buffers, buffers[count]),
            WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_CONSTANT_BUFFERS;
    op->type = type;
    op->start_idx = start_idx;
    op->count = count;
    memcpy(op->buffers, buffers, count * sizeof(*buffers));

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static bool texture_binding_might_invalidate_ps(struct wined3d_shader_resource_view *view,
        struct wined3d_shader_resource_view *prev, const struct wined3d_d3d_info *d3d_info)
{
    unsigned int old_usage, new_usage, old_caps, new_caps;
    const struct wined3d_format *old_format, *new_format;

    if (!prev)
        return true;

    /* 1.x pixel shaders need to be recompiled based on the resource type. */
    old_usage = prev->resource->usage;
    new_usage = view->resource->usage;
    if (view->resource->type != prev->resource->type
            || ((old_usage & WINED3DUSAGE_LEGACY_CUBEMAP) != (new_usage & WINED3DUSAGE_LEGACY_CUBEMAP)))
        return true;

    old_format = prev->resource->format;
    new_format = view->resource->format;
    old_caps = prev->resource->format_caps;
    new_caps = view->resource->format_caps;
    if ((old_caps & WINED3D_FORMAT_CAP_SHADOW) != (new_caps & WINED3D_FORMAT_CAP_SHADOW))
        return true;

    if (is_same_fixup(old_format->color_fixup, new_format->color_fixup))
        return false;

    if (can_use_texture_swizzle(d3d_info, new_format) && can_use_texture_swizzle(d3d_info, old_format))
        return false;

    return true;
}

static void wined3d_cs_exec_set_texture(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_d3d_info *d3d_info = &cs->c.device->adapter->d3d_info;
    const struct wined3d_cs_set_texture *op = data;
    struct wined3d_shader_resource_view *prev;

    prev = cs->state.shader_resource_view[op->shader_type][op->bind_index];
    cs->state.shader_resource_view[op->shader_type][op->bind_index] = op->view;

    if (op->view)
    {
        ++op->view->resource->bind_count;

        if (op->shader_type == WINED3D_SHADER_TYPE_PIXEL)
        {
            if (texture_binding_might_invalidate_ps(op->view, prev, d3d_info))
                device_invalidate_state(cs->c.device, STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL));
        }
    }

    if (prev)
        --prev->resource->bind_count;

    device_invalidate_state(cs->c.device, STATE_GRAPHICS_SHADER_RESOURCE_BINDING);
}

void wined3d_device_context_emit_set_texture(struct wined3d_device_context *context,
        enum wined3d_shader_type shader_type, unsigned int bind_index, struct wined3d_shader_resource_view *view)
{
    struct wined3d_cs_set_texture *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_TEXTURE;
    op->shader_type = shader_type;
    op->bind_index = bind_index;
    op->view = view;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_shader_resource_views(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_shader_resource_views *op = data;
    unsigned int i;

    for (i = 0; i < op->count; ++i)
    {
        struct wined3d_shader_resource_view *prev = cs->state.shader_resource_view[op->type][op->start_idx + i];
        struct wined3d_shader_resource_view *view = op->views[i];

        cs->state.shader_resource_view[op->type][op->start_idx + i] = view;

        if (view)
            ++view->resource->bind_count;
        if (prev)
            --prev->resource->bind_count;
    }

    if (op->type != WINED3D_SHADER_TYPE_COMPUTE)
        device_invalidate_state(cs->c.device, STATE_GRAPHICS_SHADER_RESOURCE_BINDING);
    else
        device_invalidate_state(cs->c.device, STATE_COMPUTE_SHADER_RESOURCE_BINDING);
}

void wined3d_device_context_emit_set_shader_resource_views(struct wined3d_device_context *context,
        enum wined3d_shader_type type, unsigned int start_idx, unsigned int count,
        struct wined3d_shader_resource_view *const *views)
{
    struct wined3d_cs_set_shader_resource_views *op;

    op = wined3d_device_context_require_space(context,
            offsetof(struct wined3d_cs_set_shader_resource_views, views[count]), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_SHADER_RESOURCE_VIEWS;
    op->type = type;
    op->start_idx = start_idx;
    op->count = count;
    memcpy(op->views, views, count * sizeof(*views));

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_unordered_access_views(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_unordered_access_views *op = data;
    unsigned int i;

    for (i = 0; i < op->count; ++i)
    {
        struct wined3d_unordered_access_view *prev = cs->state.unordered_access_view[op->pipeline][op->start_idx + i];
        struct wined3d_unordered_access_view *view = op->uavs[i].view;
        unsigned int initial_count = op->uavs[i].initial_count;

        cs->state.unordered_access_view[op->pipeline][op->start_idx + i] = view;

        if (view)
            ++view->resource->bind_count;
        if (prev)
            --prev->resource->bind_count;

        if (view && initial_count != ~0u)
            wined3d_unordered_access_view_set_counter(view, initial_count);
    }

    device_invalidate_state(cs->c.device, STATE_UNORDERED_ACCESS_VIEW_BINDING(op->pipeline));
}

void wined3d_device_context_emit_set_unordered_access_views(struct wined3d_device_context *context,
        enum wined3d_pipeline pipeline, unsigned int start_idx, unsigned int count,
        struct wined3d_unordered_access_view *const *views, const unsigned int *initial_counts)
{
    struct wined3d_cs_set_unordered_access_views *op;
    unsigned int i;

    op = wined3d_device_context_require_space(context,
            offsetof(struct wined3d_cs_set_unordered_access_views, uavs[count]), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_UNORDERED_ACCESS_VIEWS;
    op->pipeline = pipeline;
    op->start_idx = start_idx;
    op->count = count;
    for (i = 0; i < count; ++i)
    {
        op->uavs[i].view = views[i];
        op->uavs[i].initial_count = initial_counts ? initial_counts[i] : ~0u;
    }

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_samplers(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_samplers *op = data;
    unsigned int i;

    for (i = 0; i < op->count; ++i)
        cs->state.sampler[op->type][op->start_idx + i] = op->samplers[i];

    if (op->type != WINED3D_SHADER_TYPE_COMPUTE)
        device_invalidate_state(cs->c.device, STATE_GRAPHICS_SHADER_RESOURCE_BINDING);
    else
        device_invalidate_state(cs->c.device, STATE_COMPUTE_SHADER_RESOURCE_BINDING);
}

void wined3d_device_context_emit_set_samplers(struct wined3d_device_context *context, enum wined3d_shader_type type,
        unsigned int start_idx, unsigned int count, struct wined3d_sampler *const *samplers)
{
    struct wined3d_cs_set_samplers *op;

    op = wined3d_device_context_require_space(context, offsetof(struct wined3d_cs_set_samplers, samplers[count]),
            WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_SAMPLERS;
    op->type = type;
    op->start_idx = start_idx;
    op->count = count;
    memcpy(op->samplers, samplers, count * sizeof(*samplers));

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_shader(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_shader *op = data;

    /* CB binding may have been skipped earlier if the shader wasn't set, so make it happen. */
    if (!cs->state.shader[op->type] && op->shader)
        device_invalidate_state(cs->c.device, STATE_CONSTANT_BUFFER(op->type));
    cs->state.shader[op->type] = op->shader;
    device_invalidate_state(cs->c.device, STATE_SHADER(op->type));
    if (op->type != WINED3D_SHADER_TYPE_COMPUTE)
        device_invalidate_state(cs->c.device, STATE_GRAPHICS_SHADER_RESOURCE_BINDING);
    else
        device_invalidate_state(cs->c.device, STATE_COMPUTE_SHADER_RESOURCE_BINDING);
}

void wined3d_device_context_emit_set_shader(struct wined3d_device_context *context,
        enum wined3d_shader_type type, struct wined3d_shader *shader)
{
    struct wined3d_cs_set_shader *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_SHADER;
    op->type = type;
    op->shader = shader;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_blend_state(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_blend_state *op = data;
    struct wined3d_device *device = cs->c.device;
    struct wined3d_state *state = &cs->state;

    if (state->blend_state != op->state)
    {
        state->blend_state = op->state;
        device_invalidate_state(device, STATE_BLEND);
    }
    state->blend_factor = op->factor;
    device_invalidate_state(device, STATE_BLEND_FACTOR);
    state->sample_mask = op->sample_mask;
    device_invalidate_state(device, STATE_SAMPLE_MASK);
    for (unsigned int i = 0; i < device->context_count; ++i)
        device->contexts[i]->update_multisample_state = 1;
}

void wined3d_device_context_emit_set_blend_state(struct wined3d_device_context *context,
        struct wined3d_blend_state *state, const struct wined3d_color *blend_factor, unsigned int sample_mask)
{
    struct wined3d_cs_set_blend_state *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_BLEND_STATE;
    op->state = state;
    op->factor = *blend_factor;
    op->sample_mask = sample_mask;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_depth_stencil_state(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_depth_stencil_state *op = data;
    struct wined3d_state *state = &cs->state;

    if (state->depth_stencil_state != op->state)
    {
        state->depth_stencil_state = op->state;
        device_invalidate_state(cs->c.device, STATE_DEPTH_STENCIL);
    }
    state->stencil_ref = op->stencil_ref;
    device_invalidate_state(cs->c.device, STATE_STENCIL_REF);
}

void wined3d_device_context_emit_set_depth_stencil_state(struct wined3d_device_context *context,
        struct wined3d_depth_stencil_state *state, unsigned int stencil_ref)
{
    struct wined3d_cs_set_depth_stencil_state *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_DEPTH_STENCIL_STATE;
    op->state = state;
    op->stencil_ref = stencil_ref;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_rasterizer_state(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_rasterizer_state *op = data;

    cs->state.rasterizer_state = op->state;
    device_invalidate_state(cs->c.device, STATE_RASTERIZER);
}

void wined3d_device_context_emit_set_rasterizer_state(struct wined3d_device_context *context,
        struct wined3d_rasterizer_state *rasterizer_state)
{
    struct wined3d_cs_set_rasterizer_state *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_RASTERIZER_STATE;
    op->state = rasterizer_state;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_depth_bounds(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_depth_bounds *op = data;

    cs->state.depth_bounds_enable = op->enable;
    cs->state.depth_bounds_min = op->min_depth;
    cs->state.depth_bounds_max = op->max_depth;
    device_invalidate_state(cs->c.device, STATE_DEPTH_BOUNDS);
}

void wined3d_device_context_set_depth_bounds(struct wined3d_device_context *context,
        bool enable, float min_depth, float max_depth)
{
    struct wined3d_cs_set_depth_bounds *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_DEPTH_BOUNDS;
    op->enable = enable;
    op->min_depth = min_depth;
    op->max_depth = max_depth;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_render_state(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_render_state *op = data;

    cs->state.render_states[op->state] = op->value;
    device_invalidate_state(cs->c.device, STATE_RENDER(op->state));
}

void wined3d_device_context_emit_set_render_state(struct wined3d_device_context *context,
        enum wined3d_render_state state, unsigned int value)
{
    struct wined3d_cs_set_render_state *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_RENDER_STATE;
    op->state = state;
    op->value = value;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_texture_state(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_texture_state *op = data;

    cs->state.texture_states[op->stage][op->state] = op->value;
    device_invalidate_state(cs->c.device, STATE_TEXTURESTAGE(op->stage, op->state));
}

void wined3d_device_context_emit_set_texture_state(struct wined3d_device_context *context, unsigned int stage,
        enum wined3d_texture_stage_state state, unsigned int value)
{
    struct wined3d_cs_set_texture_state *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_TEXTURE_STATE;
    op->stage = stage;
    op->state = state;
    op->value = value;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_transform(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_transform *op = data;

    cs->state.transforms[op->state] = op->matrix;
    /* Fog behaviour depends on the projection matrix. */
    if (op->state == WINED3D_TS_PROJECTION
            && cs->state.render_states[WINED3D_RS_FOGENABLE]
            && cs->state.render_states[WINED3D_RS_FOGTABLEMODE] != WINED3D_FOG_NONE)
        device_invalidate_state(cs->c.device, STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX));
}

void wined3d_device_context_emit_set_transform(struct wined3d_device_context *context,
        enum wined3d_transform_state state, const struct wined3d_matrix *matrix)
{
    struct wined3d_cs_set_transform *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_TRANSFORM;
    op->state = state;
    op->matrix = *matrix;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_clip_plane(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_clip_plane *op = data;

    cs->state.clip_planes[op->plane_idx] = op->plane;
    device_invalidate_state(cs->c.device, STATE_CLIPPLANE(op->plane_idx));
}

void wined3d_device_context_emit_set_clip_plane(struct wined3d_device_context *context,
        unsigned int plane_idx, const struct wined3d_vec4 *plane)
{
    struct wined3d_cs_set_clip_plane *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_CLIP_PLANE;
    op->plane_idx = plane_idx;
    op->plane = *plane;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
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
                if (texture == wined3d_state_get_ffp_texture(&cs->state, 0))
                {
                    if (!(texture->async.color_key_flags & WINED3D_CKEY_SRC_BLT))
                        device_invalidate_state(cs->c.device, STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL));
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
                if (texture == wined3d_state_get_ffp_texture(&cs->state, 0)
                        && texture->async.color_key_flags & WINED3D_CKEY_SRC_BLT)
                    device_invalidate_state(cs->c.device, STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL));

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

    op = wined3d_device_context_require_space(&cs->c, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
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

    wined3d_device_context_submit(&cs->c, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_light(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_light *op = data;
    struct wined3d_light_info *light_info;
    unsigned int light_idx;

    light_idx = op->light.OriginalIndex;

    if (!(light_info = wined3d_light_state_get_light(&cs->state.light_state, light_idx)))
    {
        TRACE("Adding new light.\n");
        if (!(light_info = calloc(1, sizeof(*light_info))))
        {
            ERR("Failed to allocate light info.\n");
            return;
        }

        light_info->glIndex = -1;
        light_info->OriginalIndex = light_idx;
        rb_put(&cs->state.light_state.lights_tree, (void *)(ULONG_PTR)light_idx, &light_info->entry);
    }

    light_info->OriginalParms = op->light.OriginalParms;
    light_info->constants = op->light.constants;
}

void wined3d_device_context_emit_set_light(struct wined3d_device_context *context,
        const struct wined3d_light_info *light)
{
    struct wined3d_cs_set_light *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_LIGHT;
    op->light = *light;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_light_enable(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_light_enable *op = data;
    struct wined3d_device *device = cs->c.device;
    struct wined3d_light_info *light_info;

    if (!(light_info = wined3d_light_state_get_light(&cs->state.light_state, op->idx)))
    {
        ERR("Light doesn't exist.\n");
        return;
    }

    wined3d_light_state_enable_light(&cs->state.light_state, &device->adapter->d3d_info, light_info, op->enable);
}

void wined3d_device_context_emit_set_light_enable(struct wined3d_device_context *context, unsigned int idx, BOOL enable)
{
    struct wined3d_cs_set_light_enable *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_LIGHT_ENABLE;
    op->idx = idx;
    op->enable = enable;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_set_feature_level(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_set_feature_level *op = data;

    cs->state.feature_level = op->level;
}

void wined3d_device_context_emit_set_feature_level(struct wined3d_device_context *context,
        enum wined3d_feature_level level)
{
    struct wined3d_cs_set_feature_level *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_SET_FEATURE_LEVEL;
    op->level = level;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static const struct push_constant_info
{
    size_t size;
    unsigned int max_count;
    enum wined3d_shader_type shader_type;
}
wined3d_cs_push_constant_info[] =
{
    [WINED3D_PUSH_CONSTANTS_VS_F]       = {sizeof(struct wined3d_vec4),  WINED3D_MAX_VS_CONSTS_F, WINED3D_SHADER_TYPE_VERTEX},
    [WINED3D_PUSH_CONSTANTS_PS_F]       = {sizeof(struct wined3d_vec4),  WINED3D_MAX_PS_CONSTS_F, WINED3D_SHADER_TYPE_PIXEL},
    [WINED3D_PUSH_CONSTANTS_VS_I]       = {sizeof(struct wined3d_ivec4), WINED3D_MAX_CONSTS_I,    WINED3D_SHADER_TYPE_VERTEX},
    [WINED3D_PUSH_CONSTANTS_PS_I]       = {sizeof(struct wined3d_ivec4), WINED3D_MAX_CONSTS_I,    WINED3D_SHADER_TYPE_PIXEL},
    [WINED3D_PUSH_CONSTANTS_VS_B]       = {sizeof(BOOL),                 WINED3D_MAX_CONSTS_B,    WINED3D_SHADER_TYPE_VERTEX},
    [WINED3D_PUSH_CONSTANTS_PS_B]       = {sizeof(BOOL),                 WINED3D_MAX_CONSTS_B,    WINED3D_SHADER_TYPE_PIXEL},
    [WINED3D_PUSH_CONSTANTS_VS_FFP]     = {1, sizeof(struct wined3d_ffp_vs_constants),            WINED3D_SHADER_TYPE_VERTEX},
    [WINED3D_PUSH_CONSTANTS_PS_FFP]     = {1, sizeof(struct wined3d_ffp_ps_constants),            WINED3D_SHADER_TYPE_PIXEL},
};

static bool prepare_push_constant_buffer(struct wined3d_device_context *context, enum wined3d_push_constants type)
{
    const struct push_constant_info *info = &wined3d_cs_push_constant_info[type];
    struct wined3d_device *device = context->device;
    bool gpu = device->adapter->d3d_info.gpu_push_constants;
    HRESULT hr;

    struct wined3d_buffer_desc desc =
    {
        .byte_width = info->max_count * info->size,
        .bind_flags = 0,
        .access = WINED3D_RESOURCE_ACCESS_CPU | WINED3D_RESOURCE_ACCESS_MAP_R | WINED3D_RESOURCE_ACCESS_MAP_W,
    };

    if (gpu)
    {
        desc.bind_flags = WINED3D_BIND_CONSTANT_BUFFER;
        desc.access = WINED3D_RESOURCE_ACCESS_GPU;
    }

    if (!device->push_constants[type] && FAILED(hr = wined3d_buffer_create(device,
            &desc, NULL, NULL, &wined3d_null_parent_ops, &device->push_constants[type])))
    {
        ERR("Failed to create push constant buffer, hr %#lx.\n", hr);
        return false;
    }

    return true;
}

static void wined3d_cs_exec_push_constants(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_push_constants *op = data;
    struct wined3d_device *device = cs->c.device;
    unsigned int ffp_start_idx, ffp_end_idx;
    unsigned int context_count, i;

    /* The constant buffers were already updated; this op is just to mark the
     * constants as invalid in the device state. */

    ffp_start_idx = op->start_idx / sizeof(struct wined3d_vec4);
    ffp_end_idx = (op->start_idx + op->count + sizeof(struct wined3d_vec4)) / sizeof(struct wined3d_vec4);

    if (op->type == WINED3D_PUSH_CONSTANTS_VS_F)
        device->shader_backend->shader_update_float_vertex_constants(device, op->start_idx, op->count);
    else if (op->type == WINED3D_PUSH_CONSTANTS_PS_F)
        device->shader_backend->shader_update_float_pixel_constants(device, op->start_idx, op->count);
    else if (op->type == WINED3D_PUSH_CONSTANTS_VS_FFP)
        device->shader_backend->shader_update_float_vertex_constants(device, ffp_start_idx, ffp_end_idx - ffp_start_idx);
    else if (op->type == WINED3D_PUSH_CONSTANTS_PS_FFP)
        device->shader_backend->shader_update_float_pixel_constants(device, ffp_start_idx, ffp_end_idx - ffp_start_idx);

    for (i = 0, context_count = device->context_count; i < context_count; ++i)
        device->contexts[i]->constant_update_mask |= op->update_mask;
}

static void wined3d_device_context_emit_push_constants(struct wined3d_device_context *context,
        enum wined3d_push_constants type, uint32_t update_mask, unsigned int start_idx, unsigned int count)
{
    struct wined3d_cs *cs = wined3d_cs_from_context(context);
    struct wined3d_cs_push_constants *op;

    op = wined3d_device_context_require_space(&cs->c, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_PUSH_CONSTANTS;
    op->type = type;
    op->update_mask = update_mask;
    op->start_idx = start_idx;
    op->count = count;

    wined3d_device_context_submit(&cs->c, WINED3D_CS_QUEUE_DEFAULT);
}

void wined3d_device_context_push_constants(struct wined3d_device_context *context,
        enum wined3d_push_constants type, uint32_t update_mask, unsigned int start_idx,
        unsigned int count, const void *constants)
{
    const struct push_constant_info *info = &wined3d_cs_push_constant_info[type];
    unsigned int byte_offset = start_idx * info->size;
    unsigned int byte_size = count * info->size;
    struct wined3d_box box;

    if (!prepare_push_constant_buffer(context, type))
        return;

    wined3d_box_set(&box, byte_offset, 0, byte_offset + byte_size, 1, 0, 1);
    wined3d_device_context_emit_update_sub_resource(context,
            &context->device->push_constants[type]->resource, 0, &box, constants, byte_size, byte_size);
    wined3d_device_context_emit_push_constants(context, type, update_mask, start_idx, count);
}

static void wined3d_cs_exec_reset_state(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_device *device = cs->c.device;
    const struct wined3d_cs_reset_state *op = data;
    const struct wined3d_state_entry *state_table;
    unsigned int state;

    state_cleanup(&cs->state);
    wined3d_state_reset(&cs->state, &device->adapter->d3d_info);
    if (op->invalidate)
    {
        state_table = device->state_table;
        for (state = 0; state <= STATE_HIGHEST; ++state)
        {
            if (state_table[state].representative)
                device_invalidate_state(device, state);
        }
    }
}

void wined3d_device_context_emit_reset_state(struct wined3d_device_context *context, bool invalidate)
{
    struct wined3d_cs_reset_state *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_RESET_STATE;
    op->invalidate = invalidate;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_callback(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_callback *op = data;

    op->callback(op->object);
}

static void wined3d_cs_emit_callback(struct wined3d_cs *cs, void (*callback)(void *object), void *object)
{
    struct wined3d_cs_callback *op;

    op = wined3d_device_context_require_space(&cs->c, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_CALLBACK;
    op->callback = callback;
    op->object = object;

    wined3d_device_context_submit(&cs->c, WINED3D_CS_QUEUE_DEFAULT);
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

    if (!query->poll_in_cs)
    {
        if (op->flags & WINED3DISSUE_END)
            InterlockedIncrement(&query->counter_retrieved);
        return;
    }

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

static void wined3d_cs_issue_query(struct wined3d_device_context *context,
        struct wined3d_query *query, unsigned int flags)
{
    struct wined3d_cs *cs = wined3d_cs_from_context(context);
    struct wined3d_cs_query_issue *op;

    if (flags & WINED3DISSUE_END)
        ++query->counter_main;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_QUERY_ISSUE;
    op->query = query;
    op->flags = flags;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
    cs->queries_flushed = FALSE;

    if (flags & WINED3DISSUE_BEGIN)
        query->state = QUERY_BUILDING;
    else
        query->state = QUERY_SIGNALLED;
}

static void wined3d_cs_reference_command_list(struct wined3d_device_context *context, struct wined3d_command_list *list)
{
    struct wined3d_cs *cs = wined3d_cs_from_context(context);
    SIZE_T i;

    if (list->query_count)
    {
        cs->queries_flushed = FALSE;

        for (i = 0; i < list->query_count; ++i)
        {
            if (list->queries[i].flags & WINED3DISSUE_END)
            {
                list->queries[i].query->counter_main++;
                list->queries[i].query->state = QUERY_SIGNALLED;
            }
            else
            {
                list->queries[i].query->state = QUERY_BUILDING;
            }
        }
    }

    for (i = 0; i < list->resource_count; ++i)
        wined3d_resource_reference(list->resources[i]);

    for (i = 0; i < list->command_list_count; ++i)
        wined3d_cs_reference_command_list(context, list->command_lists[i]);

    for (i = 0; i < list->upload_count; ++i)
        invalidate_client_address(list->uploads[i].resource);
}

static void wined3d_cs_exec_preload_resource(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_preload_resource *op = data;
    struct wined3d_resource *resource = op->resource;

    resource->resource_ops->resource_preload(resource);
}

void wined3d_cs_emit_preload_resource(struct wined3d_cs *cs, struct wined3d_resource *resource)
{
    struct wined3d_cs_preload_resource *op;

    op = wined3d_device_context_require_space(&cs->c, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_PRELOAD_RESOURCE;
    op->resource = resource;

    wined3d_resource_reference(resource);

    wined3d_device_context_submit(&cs->c, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_unload_resource(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_unload_resource *op = data;
    struct wined3d_resource *resource = op->resource;

    resource->resource_ops->resource_unload(resource);
}

void wined3d_cs_emit_unload_resource(struct wined3d_cs *cs, struct wined3d_resource *resource)
{
    struct wined3d_cs_unload_resource *op;

    discard_client_address(resource);

    op = wined3d_device_context_require_space(&cs->c, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_UNLOAD_RESOURCE;
    op->resource = resource;

    wined3d_resource_reference(resource);

    wined3d_device_context_submit(&cs->c, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_device_context_upload_bo(struct wined3d_device_context *context,
        struct wined3d_resource *resource, unsigned int sub_resource_idx, const struct wined3d_box *box,
        const struct upload_bo *bo, unsigned int row_pitch, unsigned int slice_pitch)
{
    struct wined3d_cs_update_sub_resource *op;

    TRACE("context %p, resource %p, sub_resource_idx %u, box %s, bo %s, flags %#x, row_pitch %u, slice_pitch %u.\n",
            context, resource, sub_resource_idx, debug_box(box),
            debug_const_bo_address(&bo->addr), bo->flags, row_pitch, slice_pitch);

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_UPDATE_SUB_RESOURCE;
    op->resource = resource;
    op->sub_resource_idx = sub_resource_idx;
    op->box = *box;
    op->bo = *bo;
    op->row_pitch = row_pitch;
    op->slice_pitch = slice_pitch;

    wined3d_device_context_reference_resource(context, resource);

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_map(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_map *op = data;
    struct wined3d_resource *resource = op->resource;

    *op->hr = resource->resource_ops->resource_sub_resource_map(resource,
            op->sub_resource_idx, op->map_ptr, op->box, op->flags);
}

HRESULT wined3d_device_context_emit_map(struct wined3d_device_context *context,
        struct wined3d_resource *resource, unsigned int sub_resource_idx,
        struct wined3d_map_desc *map_desc, const struct wined3d_box *box, unsigned int flags)
{
    struct wined3d_cs_map *op;
    HRESULT hr;

    /* Mapping resources from the worker thread isn't an issue by itself, but
     * increasing the map count would be visible to applications. */
    wined3d_not_from_cs(context->device->cs);

    if ((flags & (WINED3D_MAP_DISCARD | WINED3D_MAP_NOOVERWRITE))
            && context->ops->map_upload_bo(context, resource, sub_resource_idx, map_desc, box, flags))
    {
        TRACE("Returning map pointer %p, row pitch %u, slice pitch %u.\n",
                map_desc->data, map_desc->row_pitch, map_desc->slice_pitch);
        return WINED3D_OK;
    }

    TRACE_(d3d_perf)("Mapping resource %p (type %u), flags %#x through the CS.\n", resource, resource->type, flags);

    wined3d_resource_wait_idle(resource);

    /* We might end up invalidating the resource on the CS thread. */
    invalidate_client_address(resource);

    if (!(op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_MAP)))
        return E_OUTOFMEMORY;
    op->opcode = WINED3D_CS_OP_MAP;
    op->resource = resource;
    op->sub_resource_idx = sub_resource_idx;
    op->map_ptr = &map_desc->data;
    op->box = box;
    op->flags = flags;
    op->hr = &hr;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_MAP);
    wined3d_device_context_finish(context, WINED3D_CS_QUEUE_MAP);

    if (SUCCEEDED(hr))
        wined3d_resource_get_sub_resource_map_pitch(resource, sub_resource_idx,
                &map_desc->row_pitch, &map_desc->slice_pitch);
    return hr;
}

static void wined3d_cs_exec_unmap(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_unmap *op = data;
    struct wined3d_resource *resource = op->resource;

    *op->hr = resource->resource_ops->resource_sub_resource_unmap(resource, op->sub_resource_idx);
}

HRESULT wined3d_device_context_emit_unmap(struct wined3d_device_context *context,
        struct wined3d_resource *resource, unsigned int sub_resource_idx)
{
    struct wined3d_cs_unmap *op;
    struct wined3d_box box;
    struct upload_bo bo;
    HRESULT hr;

    if (context->ops->unmap_upload_bo(context, resource, sub_resource_idx, &box, &bo))
    {
        unsigned int row_pitch, slice_pitch;

        wined3d_resource_get_sub_resource_map_pitch(resource, sub_resource_idx, &row_pitch, &slice_pitch);
        if (bo.flags & UPLOAD_BO_UPLOAD_ON_UNMAP)
            wined3d_device_context_upload_bo(context, resource, sub_resource_idx, &box, &bo, row_pitch, slice_pitch);
        return WINED3D_OK;
    }

    wined3d_not_from_cs(context->device->cs);

    TRACE_(d3d_perf)("Unmapping resource %p (type %u) through the CS.\n", resource, resource->type);

    if (!(op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_MAP)))
        return E_OUTOFMEMORY;
    op->opcode = WINED3D_CS_OP_UNMAP;
    op->resource = resource;
    op->sub_resource_idx = sub_resource_idx;
    op->hr = &hr;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_MAP);
    wined3d_device_context_finish(context, WINED3D_CS_QUEUE_MAP);

    return hr;
}

static void wined3d_cs_exec_map_bo_address(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_map_bo_address *op = data;
    struct wined3d_context *context;

    context = context_acquire(cs->c.device, NULL, 0);
    wined3d_context_map_bo_address(context, &op->addr, op->size, op->flags);
    context_release(context);
}

void wined3d_cs_map_bo_address(struct wined3d_cs *cs,
        struct wined3d_bo_address *addr, size_t size, unsigned int flags)
{
    struct wined3d_device_context *context = &cs->c;
    struct wined3d_cs_map_bo_address *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_MAP);
    op->opcode = WINED3D_CS_OP_MAP_BO_ADDRESS;
    op->addr = *addr;
    op->size = size;
    op->flags = flags;
    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_MAP);
    wined3d_device_context_finish(context, WINED3D_CS_QUEUE_MAP);
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
        unsigned int location;

        if (op->flags & ~WINED3D_BLT_RAW)
        {
            FIXME("Flags %#x not implemented for %s resources.\n",
                    op->flags, debug_d3dresourcetype(op->dst_resource->type));
            return;
        }

        if (!(op->flags & WINED3D_BLT_RAW) && op->src_resource->format != op->dst_resource->format)
        {
            FIXME("Format conversion not implemented for %s resources.\n",
                    debug_d3dresourcetype(op->dst_resource->type));
            return;
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
            return;
        }

        dst_texture = texture_from_resource(op->dst_resource);
        src_texture = texture_from_resource(op->src_resource);

        context = context_acquire(cs->c.device, NULL, 0);

        location = src_texture->resource.map_binding;
        if (location == WINED3D_LOCATION_SYSMEM
                && wined3d_texture_can_use_pbo(src_texture, &context->device->adapter->d3d_info))
            location = WINED3D_LOCATION_BUFFER;

        if (!wined3d_texture_load_location(src_texture, op->src_sub_resource_idx,
                context, location))
        {
            ERR("Failed to load source sub-resource into %s.\n",
                    wined3d_debug_location(location));
            context_release(context);
            return;
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
            return;
        }

        wined3d_texture_get_bo_address(src_texture, op->src_sub_resource_idx, &addr, location);
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
}

void wined3d_device_context_emit_blt_sub_resource(struct wined3d_device_context *context,
        struct wined3d_resource *dst_resource, unsigned int dst_sub_resource_idx, const struct wined3d_box *dst_box,
        struct wined3d_resource *src_resource, unsigned int src_sub_resource_idx, const struct wined3d_box *src_box,
        unsigned int flags, const struct wined3d_blt_fx *fx, enum wined3d_texture_filter_type filter)
{
    struct wined3d_cs_blt_sub_resource *op;

    /* If we are replacing the whole resource, the CS thread might discard and
     * rename the buffer object, in which case ours is no longer valid. */
    if (dst_resource->type == WINED3D_RTYPE_BUFFER && dst_box->right - dst_box->left == dst_resource->size)
        invalidate_client_address(dst_resource);

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
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

    wined3d_device_context_reference_resource(context, dst_resource);
    if (src_resource)
        wined3d_device_context_reference_resource(context, src_resource);

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
    if (flags & WINED3D_BLT_SYNCHRONOUS)
        wined3d_device_context_finish(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_update_sub_resource(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_update_sub_resource *op = data;
    struct wined3d_resource *resource = op->resource;
    const struct wined3d_box *box = &op->box;
    struct wined3d_context *context;

    context = context_acquire(cs->c.device, NULL, 0);

    if (resource->type == WINED3D_RTYPE_BUFFER)
        wined3d_buffer_update_sub_resource(buffer_from_resource(resource),
                context, &op->bo, box->left, box->right - box->left);
    else
        wined3d_texture_update_sub_resource(texture_from_resource(resource),
                op->sub_resource_idx, context, &op->bo, box, op->row_pitch, op->slice_pitch);

    context_release(context);

    if (op->bo.flags & UPLOAD_BO_FREE_ON_UNMAP)
    {
        if (op->bo.addr.buffer_object)
            FIXME("Free BO address %s.\n", debug_const_bo_address(&op->bo.addr));
        else
            free((void *)op->bo.addr.addr);
    }
}

void wined3d_device_context_emit_update_sub_resource(struct wined3d_device_context *context,
        struct wined3d_resource *resource, unsigned int sub_resource_idx, const struct wined3d_box *box,
        const void *data, unsigned int row_pitch, unsigned int slice_pitch)
{
    struct wined3d_cs_update_sub_resource *op;
    struct wined3d_map_desc map_desc;
    struct wined3d_box dummy_box;
    struct upload_bo bo;

    /* If we are replacing the whole resource, the CS thread might discard and
     * rename the buffer object, in which case ours is no longer valid. */
    if (resource->type == WINED3D_RTYPE_BUFFER && box->right - box->left == resource->size)
        invalidate_client_address(resource);

    if (context->ops->map_upload_bo(context, resource, sub_resource_idx, &map_desc, box, WINED3D_MAP_WRITE))
    {
        wined3d_format_copy_data(resource->format, data, row_pitch, slice_pitch, map_desc.data, map_desc.row_pitch,
                map_desc.slice_pitch, box->right - box->left, box->bottom - box->top, box->back - box->front);
        context->ops->unmap_upload_bo(context, resource, sub_resource_idx, &dummy_box, &bo);
        wined3d_device_context_upload_bo(context, resource, sub_resource_idx,
                box, &bo, map_desc.row_pitch, map_desc.slice_pitch);
        return;
    }

    wined3d_resource_wait_idle(resource);

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_MAP);
    op->opcode = WINED3D_CS_OP_UPDATE_SUB_RESOURCE;
    op->resource = resource;
    op->sub_resource_idx = sub_resource_idx;
    op->box = *box;
    op->bo.addr.buffer_object = 0;
    op->bo.addr.addr = data;
    op->bo.flags = 0;
    op->row_pitch = row_pitch;
    op->slice_pitch = slice_pitch;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_MAP);
    /* The data pointer may go away, so we need to wait until it is read.
     * Copying the data may be faster if it's small. */
    wined3d_device_context_finish(context, WINED3D_CS_QUEUE_MAP);
}

static void wined3d_cs_exec_add_dirty_texture_region(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_add_dirty_texture_region *op = data;
    struct wined3d_texture *texture = op->texture;
    unsigned int sub_resource_idx, i;
    struct wined3d_context *context;

    context = context_acquire(cs->c.device, NULL, 0);
    sub_resource_idx = op->layer * texture->level_count;
    for (i = 0; i < texture->level_count; ++i, ++sub_resource_idx)
    {
        if (wined3d_texture_load_location(texture, sub_resource_idx, context, texture->resource.map_binding))
            wined3d_texture_invalidate_location(texture, sub_resource_idx, ~texture->resource.map_binding);
        else
            ERR("Failed to load location %s.\n", wined3d_debug_location(texture->resource.map_binding));
    }
    context_release(context);
}

void wined3d_cs_emit_add_dirty_texture_region(struct wined3d_cs *cs,
        struct wined3d_texture *texture, unsigned int layer)
{
    struct wined3d_cs_add_dirty_texture_region *op;

    op = wined3d_device_context_require_space(&cs->c, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_ADD_DIRTY_TEXTURE_REGION;
    op->texture = texture;
    op->layer = layer;

    wined3d_resource_reference(&texture->resource);

    wined3d_device_context_submit(&cs->c, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_clear_unordered_access_view(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_clear_unordered_access_view *op = data;
    struct wined3d_unordered_access_view *view = op->view;
    struct wined3d_context *context;

    context = context_acquire(cs->c.device, NULL, 0);
    cs->c.device->adapter->adapter_ops->adapter_clear_uav(context, view, &op->clear_value, op->fp);
    context_release(context);
}

void wined3d_device_context_emit_clear_uav(struct wined3d_device_context *context,
        struct wined3d_unordered_access_view *view, const struct wined3d_uvec4 *clear_value, bool fp)
{
    struct wined3d_cs_clear_unordered_access_view *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_CLEAR_UNORDERED_ACCESS_VIEW;
    op->view = view;
    op->clear_value = *clear_value;
    op->fp = fp;

    wined3d_device_context_reference_resource(context, view->resource);

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_copy_uav_counter(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_copy_uav_counter *op = data;
    struct wined3d_unordered_access_view *view = op->view;
    struct wined3d_context *context;

    context = context_acquire(cs->c.device, NULL, 0);
    wined3d_unordered_access_view_copy_counter(view, op->buffer, op->offset, context);
    context_release(context);
}

void wined3d_device_context_emit_copy_uav_counter(struct wined3d_device_context *context,
        struct wined3d_buffer *dst_buffer, unsigned int offset, struct wined3d_unordered_access_view *uav)
{
    struct wined3d_cs_copy_uav_counter *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_COPY_UAV_COUNTER;
    op->buffer = dst_buffer;
    op->offset = offset;
    op->view = uav;

    wined3d_device_context_reference_resource(context, &dst_buffer->resource);

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_exec_generate_mipmaps(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_generate_mipmaps *op = data;
    struct wined3d_shader_resource_view *view = op->view;
    struct wined3d_context *context;

    context = context_acquire(cs->c.device, NULL, 0);
    cs->c.device->adapter->adapter_ops->adapter_generate_mipmap(context, view);
    context_release(context);
}

void wined3d_device_context_emit_generate_mipmaps(struct wined3d_device_context *context,
        struct wined3d_shader_resource_view *view)
{
    struct wined3d_cs_generate_mipmaps *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_GENERATE_MIPMAPS;
    op->view = view;

    wined3d_device_context_reference_resource(context, view->resource);

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_emit_stop(struct wined3d_cs *cs)
{
    struct wined3d_cs_stop *op;

    op = wined3d_device_context_require_space(&cs->c, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_STOP;

    wined3d_device_context_submit(&cs->c, WINED3D_CS_QUEUE_DEFAULT);
    wined3d_cs_finish(cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_cs_reference_resource(struct wined3d_device_context *context, struct wined3d_resource *resource)
{
    wined3d_resource_reference(resource);
}

static void wined3d_cs_exec_execute_command_list(struct wined3d_cs *cs, const void *data);

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
    /* WINED3D_CS_OP_SET_RENDERTARGET_VIEWS      */ wined3d_cs_exec_set_rendertarget_views,
    /* WINED3D_CS_OP_SET_DEPTH_STENCIL_VIEW      */ wined3d_cs_exec_set_depth_stencil_view,
    /* WINED3D_CS_OP_SET_VERTEX_DECLARATION      */ wined3d_cs_exec_set_vertex_declaration,
    /* WINED3D_CS_OP_SET_STREAM_SOURCES          */ wined3d_cs_exec_set_stream_sources,
    /* WINED3D_CS_OP_SET_STREAM_OUTPUTS          */ wined3d_cs_exec_set_stream_outputs,
    /* WINED3D_CS_OP_SET_INDEX_BUFFER            */ wined3d_cs_exec_set_index_buffer,
    /* WINED3D_CS_OP_SET_CONSTANT_BUFFERS        */ wined3d_cs_exec_set_constant_buffers,
    /* WINED3D_CS_OP_SET_TEXTURE                 */ wined3d_cs_exec_set_texture,
    /* WINED3D_CS_OP_SET_SHADER_RESOURCE_VIEWS   */ wined3d_cs_exec_set_shader_resource_views,
    /* WINED3D_CS_OP_SET_UNORDERED_ACCESS_VIEWS  */ wined3d_cs_exec_set_unordered_access_views,
    /* WINED3D_CS_OP_SET_SAMPLERS                */ wined3d_cs_exec_set_samplers,
    /* WINED3D_CS_OP_SET_SHADER                  */ wined3d_cs_exec_set_shader,
    /* WINED3D_CS_OP_SET_BLEND_STATE             */ wined3d_cs_exec_set_blend_state,
    /* WINED3D_CS_OP_SET_DEPTH_STENCIL_STATE     */ wined3d_cs_exec_set_depth_stencil_state,
    /* WINED3D_CS_OP_SET_RASTERIZER_STATE        */ wined3d_cs_exec_set_rasterizer_state,
    /* WINED3D_CS_OP_SET_DEPTH_BOUNDS            */ wined3d_cs_exec_set_depth_bounds,
    /* WINED3D_CS_OP_SET_RENDER_STATE            */ wined3d_cs_exec_set_render_state,
    /* WINED3D_CS_OP_SET_TEXTURE_STATE           */ wined3d_cs_exec_set_texture_state,
    /* WINED3D_CS_OP_SET_TRANSFORM               */ wined3d_cs_exec_set_transform,
    /* WINED3D_CS_OP_SET_CLIP_PLANE              */ wined3d_cs_exec_set_clip_plane,
    /* WINED3D_CS_OP_SET_COLOR_KEY               */ wined3d_cs_exec_set_color_key,
    /* WINED3D_CS_OP_SET_LIGHT                   */ wined3d_cs_exec_set_light,
    /* WINED3D_CS_OP_SET_LIGHT_ENABLE            */ wined3d_cs_exec_set_light_enable,
    /* WINED3D_CS_OP_SET_FEATURE_LEVEL           */ wined3d_cs_exec_set_feature_level,
    /* WINED3D_CS_OP_PUSH_CONSTANTS              */ wined3d_cs_exec_push_constants,
    /* WINED3D_CS_OP_RESET_STATE                 */ wined3d_cs_exec_reset_state,
    /* WINED3D_CS_OP_CALLBACK                    */ wined3d_cs_exec_callback,
    /* WINED3D_CS_OP_QUERY_ISSUE                 */ wined3d_cs_exec_query_issue,
    /* WINED3D_CS_OP_PRELOAD_RESOURCE            */ wined3d_cs_exec_preload_resource,
    /* WINED3D_CS_OP_UNLOAD_RESOURCE             */ wined3d_cs_exec_unload_resource,
    /* WINED3D_CS_OP_MAP                         */ wined3d_cs_exec_map,
    /* WINED3D_CS_OP_UNMAP                       */ wined3d_cs_exec_unmap,
    /* WINED3D_CS_OP_MAP_BO_ADDRESS              */ wined3d_cs_exec_map_bo_address,
    /* WINED3D_CS_OP_BLT_SUB_RESOURCE            */ wined3d_cs_exec_blt_sub_resource,
    /* WINED3D_CS_OP_UPDATE_SUB_RESOURCE         */ wined3d_cs_exec_update_sub_resource,
    /* WINED3D_CS_OP_ADD_DIRTY_TEXTURE_REGION    */ wined3d_cs_exec_add_dirty_texture_region,
    /* WINED3D_CS_OP_CLEAR_SYSMEM_TEXTURE        */ wined3d_cs_exec_clear_sysmem_texture,
    /* WINED3D_CS_OP_CLEAR_UNORDERED_ACCESS_VIEW */ wined3d_cs_exec_clear_unordered_access_view,
    /* WINED3D_CS_OP_COPY_UAV_COUNTER            */ wined3d_cs_exec_copy_uav_counter,
    /* WINED3D_CS_OP_GENERATE_MIPMAPS            */ wined3d_cs_exec_generate_mipmaps,
    /* WINED3D_CS_OP_EXECUTE_COMMAND_LIST        */ wined3d_cs_exec_execute_command_list,
};

void wined3d_device_context_emit_execute_command_list(struct wined3d_device_context *context,
        struct wined3d_command_list *list, bool restore_state)
{
    struct wined3d_cs_execute_command_list *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_EXECUTE_COMMAND_LIST;
    op->list = list;

    context->ops->reference_command_list(context, list);

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);

    if (restore_state)
        wined3d_device_context_set_state(context, context->state);
    else
        wined3d_device_context_reset_state(context);
}

static void *wined3d_cs_st_require_space(struct wined3d_device_context *context,
        size_t size, enum wined3d_cs_queue_id queue_id)
{
    struct wined3d_cs *cs = wined3d_cs_from_context(context);

    if (size > (cs->data_size - cs->end))
    {
        size_t new_size;
        void *new_data;

        new_size = max(size, cs->data_size * 2);
        if (!cs->end)
            new_data = realloc(cs->data, new_size);
        else
            new_data = malloc(new_size);
        if (!new_data)
            return NULL;

        cs->data_size = new_size;
        cs->start = cs->end = 0;
        cs->data = new_data;
    }

    cs->end += size;

    return (BYTE *)cs->data + cs->start;
}

static void wined3d_cs_st_submit(struct wined3d_device_context *context, enum wined3d_cs_queue_id queue_id)
{
    struct wined3d_cs *cs = wined3d_cs_from_context(context);
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
        free(data);
}

static void wined3d_cs_st_finish(struct wined3d_device_context *context, enum wined3d_cs_queue_id queue_id)
{
}

static void get_map_pitch(const struct wined3d_format *format, const struct wined3d_box *box,
        struct wined3d_map_desc *map_desc, size_t *size)
{
    unsigned int height = box->bottom - box->top;
    unsigned int width = box->right - box->left;
    unsigned int depth = box->back - box->front;

    wined3d_format_calculate_pitch(format, 1, width, height, &map_desc->row_pitch, &map_desc->slice_pitch);

    *size = (depth - 1) * map_desc->slice_pitch
            + ((height - 1) / format->block_height) * map_desc->row_pitch
            + ((width + format->block_width - 1) / format->block_width) * format->block_byte_count;
}

static bool wined3d_cs_map_upload_bo(struct wined3d_device_context *context, struct wined3d_resource *resource,
        unsigned int sub_resource_idx, struct wined3d_map_desc *map_desc, const struct wined3d_box *box, uint32_t flags)
{
    struct wined3d_client_resource *client = &resource->client;
    const struct wined3d_format *format = resource->format;
    size_t size;

    if (flags & (WINED3D_MAP_DISCARD | WINED3D_MAP_NOOVERWRITE))
    {
        struct wined3d_device *device = context->device;
        struct wined3d_bo_address addr;
        struct wined3d_bo *bo;
        uint8_t *map_ptr;

        if (resource->pin_sysmem)
        {
            TRACE("Not allocating an upload buffer because system memory is pinned for this resource.\n");
            return false;
        }

        if ((flags & WINED3D_MAP_NOOVERWRITE) && client->addr.buffer_object == CLIENT_BO_DISCARDED)
            flags = (flags & ~WINED3D_MAP_NOOVERWRITE) | WINED3D_MAP_DISCARD;

        if (flags & WINED3D_MAP_DISCARD)
        {
            if (!device->adapter->adapter_ops->adapter_alloc_bo(device, resource, sub_resource_idx, &addr))
                return false;

            /* Limit NOOVERWRITE maps to buffers for now; there are too many
             * ways that a texture can be invalidated to even count. */
            if (resource->type == WINED3D_RTYPE_BUFFER)
                client->addr = addr;
        }
        else
        {
            addr = client->addr;
        }

        map_ptr = NULL;
        if ((bo = addr.buffer_object))
        {
            wined3d_device_bo_map_lock(device);
            if ((map_ptr = bo->map_ptr))
                ++bo->client_map_count;
            wined3d_device_bo_map_unlock(device);

            if (!map_ptr)
            {
                /* adapter_alloc_bo() should have given us a mapped BO if we are
                 * discarding. */
                assert(flags & WINED3D_MAP_NOOVERWRITE);
                WARN_(d3d_perf)("Not accelerating a NOOVERWRITE map because the BO is not mapped.\n");
                return false;
            }
        }
        map_ptr += (uintptr_t)addr.addr;

        if (!map_ptr)
        {
            assert(flags & WINED3D_MAP_NOOVERWRITE);
            WARN_(d3d_perf)("Not accelerating a NOOVERWRITE map because the sub-resource has no valid address.\n");
            return false;
        }

        wined3d_resource_get_sub_resource_map_pitch(resource, sub_resource_idx,
                &map_desc->row_pitch, &map_desc->slice_pitch);

        client->mapped_upload.addr = *wined3d_const_bo_address(&addr);
        client->mapped_upload.flags = 0;
        if (bo)
        {
            map_ptr += bo->memory_offset;
            /* If we are not mapping all buffers persistently, use
             * UPDATE_SUB_RESOURCE as a means of telling the CS thread to try
             * to unmap the resource, so that we can free VA space. */
            if (!bo->coherent || !wined3d_map_persistent())
                client->mapped_upload.flags |= UPLOAD_BO_UPLOAD_ON_UNMAP;
        }
        map_desc->data = resource_offset_map_pointer(resource, sub_resource_idx, map_ptr, box);

        if (flags & WINED3D_MAP_DISCARD)
            client->mapped_upload.flags |= UPLOAD_BO_UPLOAD_ON_UNMAP | UPLOAD_BO_RENAME_ON_UNMAP;

        client->mapped_box = *box;

        TRACE("Returning bo %s, flags %#x.\n", debug_const_bo_address(&client->mapped_upload.addr),
                client->mapped_upload.flags);
        return true;
    }

    get_map_pitch(format, box, map_desc, &size);

    if (!(map_desc->data = malloc(size)))
    {
        WARN_(d3d_perf)("Failed to allocate a heap memory buffer.\n");
        return false;
    }
    client->mapped_upload.addr.buffer_object = 0;
    client->mapped_upload.addr.addr = map_desc->data;
    client->mapped_upload.flags = UPLOAD_BO_UPLOAD_ON_UNMAP | UPLOAD_BO_FREE_ON_UNMAP;
    client->mapped_box = *box;
    return true;
}

static bool wined3d_bo_address_is_null(struct wined3d_const_bo_address *addr)
{
    return !addr->buffer_object && !addr->addr;
}

static bool wined3d_cs_unmap_upload_bo(struct wined3d_device_context *context, struct wined3d_resource *resource,
        unsigned int sub_resource_idx, struct wined3d_box *box, struct upload_bo *upload_bo)
{
    struct wined3d_client_resource *client = &resource->client;
    struct wined3d_device *device = context->device;
    struct wined3d_bo *bo;

    if (wined3d_bo_address_is_null(&client->mapped_upload.addr))
        return false;

    if ((bo = client->mapped_upload.addr.buffer_object))
    {
        wined3d_device_bo_map_lock(device);
        --bo->client_map_count;
        wined3d_device_bo_map_unlock(device);
    }

    *upload_bo = client->mapped_upload;
    *box = client->mapped_box;
    memset(&client->mapped_upload, 0, sizeof(client->mapped_upload));
    memset(&client->mapped_box, 0, sizeof(client->mapped_box));
    return true;
}

static const struct wined3d_device_context_ops wined3d_cs_st_ops =
{
    wined3d_cs_st_require_space,
    wined3d_cs_st_submit,
    wined3d_cs_st_finish,
    wined3d_cs_map_upload_bo,
    wined3d_cs_unmap_upload_bo,
    wined3d_cs_issue_query,
    wined3d_cs_flush,
    wined3d_cs_reference_resource,
    wined3d_cs_reference_command_list,
};

static BOOL wined3d_cs_queue_is_empty(const struct wined3d_cs *cs, const struct wined3d_cs_queue *queue)
{
    wined3d_from_cs(cs);
    return *(volatile ULONG *)&queue->head == queue->tail;
}

static void wined3d_cs_queue_submit(struct wined3d_cs_queue *queue, struct wined3d_cs *cs)
{
    struct wined3d_cs_packet *packet;
    size_t packet_size;

    packet = (struct wined3d_cs_packet *)&queue->data[queue->head & WINED3D_CS_QUEUE_MASK];
    TRACE("Queuing op %s at %p.\n", debug_cs_op(*(const enum wined3d_cs_op *)packet->data), packet);
    packet_size = FIELD_OFFSET(struct wined3d_cs_packet, data[packet->size]);
    InterlockedExchange((LONG *)&queue->head, queue->head + packet_size);

    if (InterlockedCompareExchange(&cs->waiting_for_event, FALSE, TRUE))
    {
        if (pNtAlertThreadByThreadId)
            pNtAlertThreadByThreadId((HANDLE)(ULONG_PTR)cs->thread_id);
        else
            SetEvent(cs->event);
    }
}

static void wined3d_cs_mt_submit(struct wined3d_device_context *context, enum wined3d_cs_queue_id queue_id)
{
    struct wined3d_cs *cs = wined3d_cs_from_context(context);

    if (cs->thread_id == GetCurrentThreadId())
        return wined3d_cs_st_submit(context, queue_id);

    wined3d_cs_queue_submit(&cs->queue[queue_id], cs);
}

static void *wined3d_cs_queue_require_space(struct wined3d_cs_queue *queue, size_t size, struct wined3d_cs *cs)
{
    size_t queue_size = ARRAY_SIZE(queue->data);
    size_t header_size, packet_size, remaining;
    struct wined3d_cs_packet *packet;
    ULONG head = queue->head & WINED3D_CS_QUEUE_MASK;

    header_size = FIELD_OFFSET(struct wined3d_cs_packet, data[0]);
    packet_size = FIELD_OFFSET(struct wined3d_cs_packet, data[size]);
    packet_size = (packet_size + header_size - 1) & ~(header_size - 1);
    size = packet_size - header_size;
    if (packet_size >= WINED3D_CS_QUEUE_SIZE)
    {
        ERR("Packet size %Iu >= queue size %u.\n", packet_size, WINED3D_CS_QUEUE_SIZE);
        return NULL;
    }

    remaining = queue_size - head;
    if (remaining < packet_size)
    {
        size_t nop_size = remaining - header_size;
        struct wined3d_cs_nop *nop;

        TRACE("Inserting a nop for %Iu + %Iu bytes.\n", header_size, nop_size);

        nop = wined3d_cs_queue_require_space(queue, nop_size, cs);
        if (nop_size)
            nop->opcode = WINED3D_CS_OP_NOP;

        wined3d_cs_queue_submit(queue, cs);
        head = queue->head & WINED3D_CS_QUEUE_MASK;
        assert(!head);
    }

    for (;;)
    {
        ULONG tail = (*(volatile ULONG *)&queue->tail) & WINED3D_CS_QUEUE_MASK;
        ULONG new_pos;

        /* Empty. */
        if (head == tail)
            break;
        new_pos = (head + packet_size) & WINED3D_CS_QUEUE_MASK;
        /* Head ahead of tail. We checked the remaining size above, so we only
         * need to make sure we don't make head equal to tail. */
        if (head > tail && (new_pos != tail))
            break;
        /* Tail ahead of head. Make sure the new head is before the tail as
         * well. Note that new_pos is 0 when it's at the end of the queue. */
        if (new_pos < tail && new_pos)
            break;

        TRACE_(d3d_perf)("Waiting for free space. Head %lu, tail %lu, packet size %Iu.\n",
                head, tail, packet_size);
    }

    packet = (struct wined3d_cs_packet *)&queue->data[head];
    packet->size = size;
    return packet->data;
}

static void *wined3d_cs_mt_require_space(struct wined3d_device_context *context,
        size_t size, enum wined3d_cs_queue_id queue_id)
{
    struct wined3d_cs *cs = wined3d_cs_from_context(context);

    if (cs->thread_id == GetCurrentThreadId())
        return wined3d_cs_st_require_space(context, size, queue_id);

    return wined3d_cs_queue_require_space(&cs->queue[queue_id], size, cs);
}

static void wined3d_cs_mt_finish(struct wined3d_device_context *context, enum wined3d_cs_queue_id queue_id)
{
    struct wined3d_cs *cs = wined3d_cs_from_context(context);
    unsigned int spin_count = 0;

    if (cs->thread_id == GetCurrentThreadId())
        return wined3d_cs_st_finish(context, queue_id);

    TRACE_(d3d_perf)("Waiting for queue %u to be empty.\n", queue_id);
    while (cs->queue[queue_id].head != *(volatile ULONG *)&cs->queue[queue_id].tail)
        wined3d_pause(&spin_count);
    TRACE_(d3d_perf)("Queue is now empty.\n");
}

static const struct wined3d_device_context_ops wined3d_cs_mt_ops =
{
    wined3d_cs_mt_require_space,
    wined3d_cs_mt_submit,
    wined3d_cs_mt_finish,
    wined3d_cs_map_upload_bo,
    wined3d_cs_unmap_upload_bo,
    wined3d_cs_issue_query,
    wined3d_cs_flush,
    wined3d_cs_reference_resource,
    wined3d_cs_reference_command_list,
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
    static const LARGE_INTEGER query_timeout = {.QuadPart = WINED3D_CS_COMMAND_WAIT_WITH_QUERIES_TIMEOUT * -10};
    const LARGE_INTEGER *timeout = NULL;

    if (!list_empty(&cs->query_poll_list))
        timeout = &query_timeout;

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

    if (pNtWaitForAlertByThreadId)
        pNtWaitForAlertByThreadId(NULL, timeout);
    else
        NtWaitForSingleObject(cs->event, FALSE, timeout);
}

static void wined3d_cs_command_lock(const struct wined3d_cs *cs)
{
    if (cs->serialize_commands)
        EnterCriticalSection(&wined3d_command_cs);
}

static void wined3d_cs_command_unlock(const struct wined3d_cs *cs)
{
    if (cs->serialize_commands)
        LeaveCriticalSection(&wined3d_command_cs);
}

static inline bool wined3d_cs_execute_next(struct wined3d_cs *cs, struct wined3d_cs_queue *queue)
{
    struct wined3d_cs_packet *packet;
    enum wined3d_cs_op opcode;
    SIZE_T tail;

    tail = queue->tail;
    packet = wined3d_next_cs_packet(queue->data, &tail, WINED3D_CS_QUEUE_MASK);

    if (packet->size)
    {
        opcode = *(const enum wined3d_cs_op *)packet->data;

        TRACE("Executing %s at %p.\n", debug_cs_op(opcode), packet);
        if (opcode >= WINED3D_CS_OP_STOP)
        {
            if (opcode > WINED3D_CS_OP_STOP)
                ERR("Invalid opcode %#x.\n", opcode);
            return false;
        }

        wined3d_cs_command_lock(cs);
        wined3d_cs_op_handlers[opcode](cs, packet->data);
        wined3d_cs_command_unlock(cs);
        TRACE("%s at %p executed.\n", debug_cs_op(opcode), packet);
    }

    InterlockedExchange((LONG *)&queue->tail, tail);
    return true;
}

static void wined3d_cs_exec_execute_command_list(struct wined3d_cs *cs, const void *data)
{
    const struct wined3d_cs_execute_command_list *op = data;
    SIZE_T start = 0, end = op->list->data_size;
    const BYTE *cs_data = op->list->data;
    struct wined3d_cs_queue *queue;

    TRACE("Executing command list %p.\n", op->list);

    queue = &cs->queue[WINED3D_CS_QUEUE_MAP];
    while (start < end)
    {
        const struct wined3d_cs_packet *packet;
        enum wined3d_cs_op opcode;

        while (!wined3d_cs_queue_is_empty(cs, queue))
            wined3d_cs_execute_next(cs, queue);

        packet = wined3d_next_cs_packet(cs_data, &start, WINED3D_CS_QUEUE_MASK);
        opcode = *(const enum wined3d_cs_op *)packet->data;

        if (opcode >= WINED3D_CS_OP_STOP)
            ERR("Invalid opcode %#x.\n", opcode);
        else
            wined3d_cs_op_handlers[opcode](cs, packet->data);
        TRACE("%s executed.\n", debug_cs_op(opcode));
    }
}

static DWORD WINAPI wined3d_cs_run(void *ctx)
{
    struct wined3d_cs_queue *queue;
    unsigned int spin_count = 0;
    struct wined3d_cs *cs = ctx;
    HMODULE wined3d_module;
    unsigned int poll = 0;
    bool run = true;

    TRACE("Started.\n");
    SetThreadDescription(GetCurrentThread(), L"wined3d_cs");

    /* Copy the module handle to a local variable to avoid racing with the
     * thread freeing "cs" before the FreeLibraryAndExitThread() call. */
    wined3d_module = cs->wined3d_module;

    list_init(&cs->query_poll_list);
    cs->thread_id = GetCurrentThreadId();
    while (run)
    {
        if (++poll == WINED3D_CS_QUERY_POLL_INTERVAL)
        {
            wined3d_cs_command_lock(cs);
            poll_queries(cs);
            wined3d_cs_command_unlock(cs);
            poll = 0;
        }

        queue = &cs->queue[WINED3D_CS_QUEUE_MAP];
        if (wined3d_cs_queue_is_empty(cs, queue))
        {
            queue = &cs->queue[WINED3D_CS_QUEUE_DEFAULT];
            if (wined3d_cs_queue_is_empty(cs, queue))
            {
                YieldProcessor();
                if (++spin_count >= WINED3D_CS_SPIN_COUNT)
                {
                    if (poll)
                        poll = WINED3D_CS_QUERY_POLL_INTERVAL - 1;
                    else
                        wined3d_cs_wait_event(cs);
                }
                continue;
            }
        }
        spin_count = 0;

        run = wined3d_cs_execute_next(cs, queue);
    }

    cs->queue[WINED3D_CS_QUEUE_MAP].tail = cs->queue[WINED3D_CS_QUEUE_MAP].head;
    cs->queue[WINED3D_CS_QUEUE_DEFAULT].tail = cs->queue[WINED3D_CS_QUEUE_DEFAULT].head;
    TRACE("Stopped.\n");
    FreeLibraryAndExitThread(wined3d_module, 0);
}

void CDECL wined3d_device_context_flush_mapped_buffer(struct wined3d_device_context *context,
        struct wined3d_buffer *buffer)
{
    struct wined3d_client_resource *client = &buffer->resource.client;

    /* d3d9 applications can draw from a mapped dynamic buffer.
     * Castlevania 2 depends on this behaviour.
     * This means that we need to upload a pending discard bo, without actually
     * unmapping it. */

    assert(context == &context->device->cs->c);

    if (wined3d_bo_address_is_null(&client->mapped_upload.addr))
        return;

    if (client->mapped_upload.flags & UPLOAD_BO_UPLOAD_ON_UNMAP)
        wined3d_device_context_upload_bo(context, &buffer->resource, 0,
                &client->mapped_box, &client->mapped_upload, buffer->resource.size, buffer->resource.size);

    if (client->mapped_upload.flags & UPLOAD_BO_RENAME_ON_UNMAP)
    {
        client->mapped_upload.flags &= ~UPLOAD_BO_RENAME_ON_UNMAP;
        /* This logic matches wined3d_cs_map_upload_bo(). */
        if (client->mapped_upload.addr.buffer_object->coherent && wined3d_map_persistent())
            client->mapped_upload.flags &= ~UPLOAD_BO_UPLOAD_ON_UNMAP;
    }
}

struct wined3d_cs *wined3d_cs_create(struct wined3d_device *device,
        const enum wined3d_feature_level *levels, unsigned int level_count)
{
    const struct wined3d_d3d_info *d3d_info = &device->adapter->d3d_info;
    struct wined3d_cs *cs;

    if (!(cs = calloc(1, sizeof(*cs))))
        return NULL;

    if (FAILED(wined3d_state_create(device, levels, level_count, &cs->c.state)))
    {
        free(cs);
        return NULL;
    }

    cs->c.ops = &wined3d_cs_st_ops;
    cs->c.device = device;
    cs->serialize_commands = TRACE_ON(d3d_sync) || wined3d_settings.cs_multithreaded & WINED3D_CSMT_SERIALIZE;

    if (cs->serialize_commands)
        ERR_(d3d_sync)("Forcing serialization of all command streams.\n");

    state_init(&cs->state, d3d_info, WINED3D_STATE_NO_REF | WINED3D_STATE_INIT_DEFAULT, cs->c.state->feature_level);

    cs->data_size = WINED3D_INITIAL_CS_SIZE;
    if (!(cs->data = malloc(cs->data_size)))
        goto fail;

    if (wined3d_settings.cs_multithreaded & WINED3D_CSMT_ENABLE)
    {
        if (!d3d_info->fences)
        {
            WARN("Disabling CSMT, adapter doesn't support fences.\n");
            wined3d_settings.cs_multithreaded &= ~WINED3D_CSMT_ENABLE;
        }
    }

    if (wined3d_settings.cs_multithreaded & WINED3D_CSMT_ENABLE
            && !RtlIsCriticalSectionLockedByThread(NtCurrentTeb()->Peb->LoaderLock))
    {
        cs->c.ops = &wined3d_cs_mt_ops;

        if (!pNtAlertThreadByThreadId)
        {
            HANDLE ntdll = GetModuleHandleW(L"ntdll.dll");

            pNtAlertThreadByThreadId = (void *)GetProcAddress(ntdll, "NtAlertThreadByThreadId");
            pNtWaitForAlertByThreadId = (void *)GetProcAddress(ntdll, "NtWaitForAlertByThreadId");
        }

        if (!pNtAlertThreadByThreadId && !(cs->event = CreateEventW(NULL, FALSE, FALSE, NULL)))
        {
            ERR("Failed to create command stream event.\n");
            free(cs->data);
            goto fail;
        }
        if (!(cs->present_event = CreateEventW(NULL, FALSE, FALSE, NULL)))
        {
            ERR("Failed to create command stream present event.\n");
            free(cs->data);
            goto fail;
        }

        if (!(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                (const WCHAR *)wined3d_cs_run, &cs->wined3d_module)))
        {
            ERR("Failed to get wined3d module handle.\n");
            CloseHandle(cs->present_event);
            if (cs->event)
                CloseHandle(cs->event);
            free(cs->data);
            goto fail;
        }

        if (!(cs->thread = CreateThread(NULL, 0, wined3d_cs_run, cs, 0, NULL)))
        {
            ERR("Failed to create wined3d command stream thread.\n");
            FreeLibrary(cs->wined3d_module);
            CloseHandle(cs->present_event);
            if (cs->event)
                CloseHandle(cs->event);
            free(cs->data);
            goto fail;
        }
    }

    TRACE("Created command stream %p.\n", cs);
    return cs;

fail:
    wined3d_state_destroy(cs->c.state);
    state_cleanup(&cs->state);
    free(cs);
    return NULL;
}

void wined3d_cs_destroy(struct wined3d_cs *cs)
{
    if (cs->thread)
    {
        wined3d_cs_emit_stop(cs);
        CloseHandle(cs->thread);
        if (!CloseHandle(cs->present_event))
            ERR("Closing present event failed.\n");
        if (cs->event && !CloseHandle(cs->event))
            ERR("Closing event failed.\n");
    }

    wined3d_state_destroy(cs->c.state);
    state_cleanup(&cs->state);
    free(cs->data);
    free(cs);
}

static void wined3d_cs_packet_decref_objects(const struct wined3d_cs_packet *packet)
{
    enum wined3d_cs_op opcode = *(const enum wined3d_cs_op *)packet->data;
    unsigned int i;

    switch (opcode)
    {
        case WINED3D_CS_OP_SET_SAMPLERS:
        {
            struct wined3d_cs_set_samplers *op = (struct wined3d_cs_set_samplers *)packet->data;

            for (i = 0; i < op->count; ++i)
            {
                if (op->samplers[i])
                    wined3d_sampler_decref(op->samplers[i]);
            }
            break;
        }

        case WINED3D_CS_OP_SET_SHADER:
        {
            struct wined3d_cs_set_shader *op = (struct wined3d_cs_set_shader *)packet->data;

            if (op->shader)
                wined3d_shader_decref(op->shader);
            break;
        }

        case WINED3D_CS_OP_SET_DEPTH_STENCIL_STATE:
        {
            struct wined3d_cs_set_depth_stencil_state *op;

            op = (struct wined3d_cs_set_depth_stencil_state *)packet->data;
            if (op->state)
                wined3d_depth_stencil_state_decref(op->state);
            break;
        }

        case WINED3D_CS_OP_SET_RASTERIZER_STATE:
        {
            struct wined3d_cs_set_rasterizer_state *op;

            op = (struct wined3d_cs_set_rasterizer_state *)packet->data;
            if (op->state)
                wined3d_rasterizer_state_decref(op->state);
            break;
        }

        case WINED3D_CS_OP_SET_BLEND_STATE:
        {
            struct wined3d_cs_set_blend_state *op;

            op = (struct wined3d_cs_set_blend_state *)packet->data;
            if (op->state)
                wined3d_blend_state_decref(op->state);
            break;
        }

        case WINED3D_CS_OP_SET_RENDERTARGET_VIEWS:
        {
            struct wined3d_cs_set_rendertarget_views *op;

            op = (struct wined3d_cs_set_rendertarget_views *)packet->data;
            for (i = 0; i < op->count; ++i)
            {
                if (op->views[i])
                    wined3d_rendertarget_view_decref(op->views[i]);
            }
            break;
        }

        case WINED3D_CS_OP_SET_SHADER_RESOURCE_VIEWS:
        {
            struct wined3d_cs_set_shader_resource_views *op;

            op = (struct wined3d_cs_set_shader_resource_views *)packet->data;
            for (i = 0; i < op->count; ++i)
            {
                if (op->views[i])
                    wined3d_shader_resource_view_decref(op->views[i]);
            }
            break;
        }

        case WINED3D_CS_OP_SET_UNORDERED_ACCESS_VIEWS:
        {
            struct wined3d_cs_set_unordered_access_views *op;

            op = (struct wined3d_cs_set_unordered_access_views *)packet->data;
            for (i = 0; i < op->count; ++i)
            {
                if (op->uavs[i].view)
                    wined3d_unordered_access_view_decref(op->uavs[i].view);
            }
            break;
        }

        case WINED3D_CS_OP_SET_DEPTH_STENCIL_VIEW:
        {
            struct wined3d_cs_set_depth_stencil_view *op;

            op = (struct wined3d_cs_set_depth_stencil_view *)packet->data;
            if (op->view)
                wined3d_rendertarget_view_decref(op->view);
            break;
        }

        case WINED3D_CS_OP_SET_CONSTANT_BUFFERS:
        {
            struct wined3d_cs_set_constant_buffers *op;

            op = (struct wined3d_cs_set_constant_buffers *)packet->data;
            for (i = 0; i < op->count; ++i)
            {
                if (op->buffers[i].buffer)
                    wined3d_buffer_decref(op->buffers[i].buffer);
            }
            break;
        }

        case WINED3D_CS_OP_CLEAR_UNORDERED_ACCESS_VIEW:
        {
            struct wined3d_cs_clear_unordered_access_view *op;

            op = (struct wined3d_cs_clear_unordered_access_view *)packet->data;
            wined3d_unordered_access_view_decref(op->view);
            break;
        }

        case WINED3D_CS_OP_CLEAR:
        {
            struct wined3d_cs_clear *op = (struct wined3d_cs_clear *)packet->data;

            for (i = 0; i < op->rt_count; ++i)
            {
                if (op->fb.render_targets[i])
                    wined3d_rendertarget_view_decref(op->fb.render_targets[i]);
            }
            if (op->fb.depth_stencil)
                wined3d_rendertarget_view_decref(op->fb.depth_stencil);
            break;
        }

        case WINED3D_CS_OP_CLEAR_SYSMEM_TEXTURE:
        {
            struct wined3d_cs_clear_sysmem_texture *op = (struct wined3d_cs_clear_sysmem_texture *)packet->data;

            wined3d_texture_decref(op->texture);
            break;
        }

        case WINED3D_CS_OP_DISPATCH:
        {
            struct wined3d_cs_dispatch *op = (struct wined3d_cs_dispatch *)packet->data;

            if (op->parameters.indirect)
                wined3d_buffer_decref(op->parameters.u.indirect.buffer);
            break;
        }

        case WINED3D_CS_OP_DRAW:
        {
            struct wined3d_cs_draw *op = (struct wined3d_cs_draw *)packet->data;

            if (op->parameters.indirect)
                wined3d_buffer_decref(op->parameters.u.indirect.buffer);
            break;
        }

        case WINED3D_CS_OP_SET_INDEX_BUFFER:
        {
            struct wined3d_cs_set_index_buffer *op;

            op = (struct wined3d_cs_set_index_buffer *)packet->data;
            if (op->buffer)
                wined3d_buffer_decref(op->buffer);
            break;
        }

        case WINED3D_CS_OP_SET_STREAM_OUTPUTS:
        {
            struct wined3d_cs_set_stream_outputs *op;

            op = (struct wined3d_cs_set_stream_outputs *)packet->data;
            for (i = 0; i < ARRAY_SIZE(op->outputs); ++i)
            {
                if (op->outputs[i].buffer)
                    wined3d_buffer_decref(op->outputs[i].buffer);
            }
            break;
        }

        case WINED3D_CS_OP_SET_STREAM_SOURCES:
        {
            struct wined3d_cs_set_stream_sources *op;

            op = (struct wined3d_cs_set_stream_sources *)packet->data;
            for (i = 0; i < op->count; ++i)
            {
                if (op->streams[i].buffer)
                    wined3d_buffer_decref(op->streams[i].buffer);
            }
            break;
        }

        case WINED3D_CS_OP_UPDATE_SUB_RESOURCE:
        {
            struct wined3d_cs_update_sub_resource *op;

            op = (struct wined3d_cs_update_sub_resource *)packet->data;
            wined3d_resource_decref(op->resource);
            break;
        }

        case WINED3D_CS_OP_BLT_SUB_RESOURCE:
        {
            struct wined3d_cs_blt_sub_resource *op;

            op = (struct wined3d_cs_blt_sub_resource *)packet->data;
            if (op->src_resource)
                wined3d_resource_decref(op->src_resource);
            wined3d_resource_decref(op->dst_resource);
            break;
        }

        case WINED3D_CS_OP_COPY_UAV_COUNTER:
        {
            struct wined3d_cs_copy_uav_counter *op;

            op = (struct wined3d_cs_copy_uav_counter *)packet->data;
            wined3d_buffer_decref(op->buffer);
            wined3d_unordered_access_view_decref(op->view);
            break;
        }

        case WINED3D_CS_OP_GENERATE_MIPMAPS:
        {
            struct wined3d_cs_generate_mipmaps *op;

            op = (struct wined3d_cs_generate_mipmaps *)packet->data;
            wined3d_shader_resource_view_decref(op->view);
            break;
        }

        default:
            break;
    }
}

static void wined3d_cs_packet_incref_objects(struct wined3d_cs_packet *packet)
{
    enum wined3d_cs_op opcode = *(const enum wined3d_cs_op *)packet->data;
    unsigned int i;

    switch (opcode)
    {
        case WINED3D_CS_OP_SET_SAMPLERS:
        {
            struct wined3d_cs_set_samplers *op = (struct wined3d_cs_set_samplers *)packet->data;

            for (i = 0; i < op->count; ++i)
            {
                if (op->samplers[i])
                    wined3d_sampler_incref(op->samplers[i]);
            }
            break;
        }

        case WINED3D_CS_OP_SET_SHADER:
        {
            struct wined3d_cs_set_shader *op = (struct wined3d_cs_set_shader *)packet->data;

            if (op->shader)
                wined3d_shader_incref(op->shader);
            break;
        }

        case WINED3D_CS_OP_SET_DEPTH_STENCIL_STATE:
        {
            struct wined3d_cs_set_depth_stencil_state *op;

            op = (struct wined3d_cs_set_depth_stencil_state *)packet->data;
            if (op->state)
                wined3d_depth_stencil_state_incref(op->state);
            break;
        }

        case WINED3D_CS_OP_SET_RASTERIZER_STATE:
        {
            struct wined3d_cs_set_rasterizer_state *op;

            op = (struct wined3d_cs_set_rasterizer_state *)packet->data;
            if (op->state)
                wined3d_rasterizer_state_incref(op->state);
            break;
        }

        case WINED3D_CS_OP_SET_BLEND_STATE:
        {
            struct wined3d_cs_set_blend_state *op;

            op = (struct wined3d_cs_set_blend_state *)packet->data;
            if (op->state)
                wined3d_blend_state_incref(op->state);
            break;
        }

        case WINED3D_CS_OP_SET_RENDERTARGET_VIEWS:
        {
            struct wined3d_cs_set_rendertarget_views *op;

            op = (struct wined3d_cs_set_rendertarget_views *)packet->data;
            for (i = 0; i < op->count; ++i)
            {
                if (op->views[i])
                    wined3d_rendertarget_view_incref(op->views[i]);
            }
            break;
        }

        case WINED3D_CS_OP_SET_SHADER_RESOURCE_VIEWS:
        {
            struct wined3d_cs_set_shader_resource_views *op;

            op = (struct wined3d_cs_set_shader_resource_views *)packet->data;
            for (i = 0; i < op->count; ++i)
            {
                if (op->views[i])
                    wined3d_shader_resource_view_incref(op->views[i]);
            }
            break;
        }

        case WINED3D_CS_OP_SET_UNORDERED_ACCESS_VIEWS:
        {
            struct wined3d_cs_set_unordered_access_views *op;

            op = (struct wined3d_cs_set_unordered_access_views *)packet->data;
            for (i = 0; i < op->count; ++i)
            {
                if (op->uavs[i].view)
                    wined3d_unordered_access_view_incref(op->uavs[i].view);
            }
            break;
        }

        case WINED3D_CS_OP_SET_DEPTH_STENCIL_VIEW:
        {
            struct wined3d_cs_set_depth_stencil_view *op;

            op = (struct wined3d_cs_set_depth_stencil_view *)packet->data;
            if (op->view)
                wined3d_rendertarget_view_incref(op->view);
            break;
        }

        case WINED3D_CS_OP_SET_CONSTANT_BUFFERS:
        {
            struct wined3d_cs_set_constant_buffers *op;

            op = (struct wined3d_cs_set_constant_buffers *)packet->data;
            for (i = 0; i < op->count; ++i)
            {
                if (op->buffers[i].buffer)
                    wined3d_buffer_incref(op->buffers[i].buffer);
            }
            break;
        }

        case WINED3D_CS_OP_CLEAR_UNORDERED_ACCESS_VIEW:
        {
            struct wined3d_cs_clear_unordered_access_view *op;

            op = (struct wined3d_cs_clear_unordered_access_view *)packet->data;
            wined3d_unordered_access_view_incref(op->view);
            break;
        }

        case WINED3D_CS_OP_CLEAR:
        {
            struct wined3d_cs_clear *op = (struct wined3d_cs_clear *)packet->data;

            for (i = 0; i < op->rt_count; ++i)
            {
                if (op->fb.render_targets[i])
                    wined3d_rendertarget_view_incref(op->fb.render_targets[i]);
            }
            if (op->fb.depth_stencil)
                wined3d_rendertarget_view_incref(op->fb.depth_stencil);
            break;
        }

        case WINED3D_CS_OP_CLEAR_SYSMEM_TEXTURE:
        {
            struct wined3d_cs_clear_sysmem_texture *op = (struct wined3d_cs_clear_sysmem_texture *)packet->data;

            wined3d_texture_incref(op->texture);
            break;
        }

        case WINED3D_CS_OP_DISPATCH:
        {
            struct wined3d_cs_dispatch *op = (struct wined3d_cs_dispatch *)packet->data;

            if (op->parameters.indirect)
                wined3d_buffer_incref(op->parameters.u.indirect.buffer);
            break;
        }

        case WINED3D_CS_OP_DRAW:
        {
            struct wined3d_cs_draw *op = (struct wined3d_cs_draw *)packet->data;

            if (op->parameters.indirect)
                wined3d_buffer_incref(op->parameters.u.indirect.buffer);
            break;
        }

        case WINED3D_CS_OP_SET_INDEX_BUFFER:
        {
            struct wined3d_cs_set_index_buffer *op;

            op = (struct wined3d_cs_set_index_buffer *)packet->data;
            if (op->buffer)
                wined3d_buffer_incref(op->buffer);
            break;
        }

        case WINED3D_CS_OP_SET_STREAM_OUTPUTS:
        {
            struct wined3d_cs_set_stream_outputs *op;

            op = (struct wined3d_cs_set_stream_outputs *)packet->data;
            for (i = 0; i < ARRAY_SIZE(op->outputs); ++i)
            {
                if (op->outputs[i].buffer)
                    wined3d_buffer_incref(op->outputs[i].buffer);
            }
            break;
        }

        case WINED3D_CS_OP_SET_STREAM_SOURCES:
        {
            struct wined3d_cs_set_stream_sources *op;

            op  = (struct wined3d_cs_set_stream_sources *)packet->data;
            for (i = 0; i < op->count; ++i)
            {
                if (op->streams[i].buffer)
                    wined3d_buffer_incref(op->streams[i].buffer);
            }
            break;
        }

        case WINED3D_CS_OP_UPDATE_SUB_RESOURCE:
        {
            struct wined3d_cs_update_sub_resource *op;

            op = (struct wined3d_cs_update_sub_resource *)packet->data;
            wined3d_resource_incref(op->resource);
            break;
        }

        case WINED3D_CS_OP_BLT_SUB_RESOURCE:
        {
            struct wined3d_cs_blt_sub_resource *op;

            op = (struct wined3d_cs_blt_sub_resource *)packet->data;
            if (op->src_resource)
                wined3d_resource_incref(op->src_resource);
            wined3d_resource_incref(op->dst_resource);
            break;
        }

        case WINED3D_CS_OP_COPY_UAV_COUNTER:
        {
            struct wined3d_cs_copy_uav_counter *op;

            op = (struct wined3d_cs_copy_uav_counter *)packet->data;
            wined3d_buffer_incref(op->buffer);
            wined3d_unordered_access_view_incref(op->view);
            break;
        }

        case WINED3D_CS_OP_GENERATE_MIPMAPS:
        {
            struct wined3d_cs_generate_mipmaps *op;

            op = (struct wined3d_cs_generate_mipmaps *)packet->data;
            wined3d_shader_resource_view_incref(op->view);
            break;
        }

        default:
            break;
    }
}

struct wined3d_deferred_context
{
    struct wined3d_device_context c;

    SIZE_T data_size, data_capacity;
    void *data;

    SIZE_T resource_count, resources_capacity;
    struct wined3d_resource **resources;

    SIZE_T upload_count, uploads_capacity;
    struct wined3d_deferred_upload *uploads;

    HANDLE upload_heap;
    LONG *upload_heap_refcount;

    /* List of command lists queued for execution on this context. A command
     * list can be the only thing holding a pointer to another command list, so
     * we need to hold a reference here and in wined3d_command_list as well. */
    SIZE_T command_list_count, command_lists_capacity;
    struct wined3d_command_list **command_lists;

    SIZE_T query_count, queries_capacity;
    struct wined3d_deferred_query_issue *queries;
};

static struct wined3d_deferred_context *wined3d_deferred_context_from_context(struct wined3d_device_context *context)
{
    return CONTAINING_RECORD(context, struct wined3d_deferred_context, c);
}

static void *wined3d_deferred_context_require_space(struct wined3d_device_context *context,
        size_t size, enum wined3d_cs_queue_id queue_id)
{
    struct wined3d_deferred_context *deferred = wined3d_deferred_context_from_context(context);
    struct wined3d_cs_packet *packet;
    size_t header_size, packet_size;

    if (queue_id != WINED3D_CS_QUEUE_DEFAULT)
        return NULL;

    header_size = offsetof(struct wined3d_cs_packet, data[0]);
    packet_size = offsetof(struct wined3d_cs_packet, data[size]);
    packet_size = (packet_size + header_size - 1) & ~(header_size - 1);

    if (!wined3d_array_reserve(&deferred->data, &deferred->data_capacity, deferred->data_size + packet_size, 1))
        return NULL;

    packet = (struct wined3d_cs_packet *)((BYTE *)deferred->data + deferred->data_size);
    TRACE("size was %Iu, adding %Iu\n", (size_t)deferred->data_size, packet_size);
    packet->size = packet_size - header_size;
    return &packet->data;
}

static void wined3d_deferred_context_submit(struct wined3d_device_context *context, enum wined3d_cs_queue_id queue_id)
{
    struct wined3d_deferred_context *deferred = wined3d_deferred_context_from_context(context);
    struct wined3d_cs_packet *packet;

    assert(queue_id == WINED3D_CS_QUEUE_DEFAULT);
    packet = wined3d_next_cs_packet(deferred->data, &deferred->data_size, ~(SIZE_T)0);
    wined3d_cs_packet_incref_objects(packet);
}

static void wined3d_deferred_context_finish(struct wined3d_device_context *context, enum wined3d_cs_queue_id queue_id)
{
    /* This should not happen; we cannot meaningfully finish a deferred context. */
    ERR("Ignoring finish() on a deferred context.\n");
}

static struct wined3d_deferred_upload *deferred_context_get_upload(struct wined3d_deferred_context *deferred,
        struct wined3d_resource *resource, unsigned int sub_resource_idx)
{
    SIZE_T i = deferred->upload_count;

    while (i--)
    {
        struct wined3d_deferred_upload *upload = &deferred->uploads[i];

        if (upload->resource == resource && upload->sub_resource_idx == sub_resource_idx)
            return upload;
    }

    return NULL;
}

static bool wined3d_deferred_context_map_upload_bo(struct wined3d_device_context *context,
        struct wined3d_resource *resource, unsigned int sub_resource_idx,
        struct wined3d_map_desc *map_desc, const struct wined3d_box *box, uint32_t flags)
{
    struct wined3d_deferred_context *deferred = wined3d_deferred_context_from_context(context);
    const struct wined3d_format *format = resource->format;
    struct wined3d_device *device = context->device;
    struct wined3d_deferred_upload *upload;
    struct wined3d_bo_address addr;
    uint8_t *sysmem;
    size_t size;

    get_map_pitch(format, box, map_desc, &size);

    if (!(flags & WINED3D_MAP_WRITE))
    {
        WARN("Flags %#x are not valid on a deferred context.\n", flags);
        return false;
    }

    if (flags & ~(WINED3D_MAP_WRITE | WINED3D_MAP_DISCARD | WINED3D_MAP_NOOVERWRITE))
    {
        FIXME("Unhandled flags %#x.\n", flags);
        return false;
    }

    if (flags & WINED3D_MAP_NOOVERWRITE)
    {
        if (!(upload = deferred_context_get_upload(deferred, resource, sub_resource_idx)))
            return false;

        upload->upload_flags = 0;
        map_desc->data = upload->map_ptr;
        return true;
    }

    if (!wined3d_array_reserve((void **)&deferred->uploads, &deferred->uploads_capacity,
            deferred->upload_count + 1, sizeof(*deferred->uploads)))
        return false;

    upload = &deferred->uploads[deferred->upload_count++];

    if ((flags & WINED3D_MAP_DISCARD)
            && device->adapter->adapter_ops->adapter_alloc_bo(device, resource, sub_resource_idx, &addr))
    {
        upload->bo = addr.buffer_object;
        upload->sysmem = NULL;

        TRACE("Allocated BO %s.\n", debug_bo_address(&addr));

        wined3d_device_bo_map_lock(device);
        upload->map_ptr = addr.buffer_object->map_ptr;
        wined3d_device_bo_map_unlock(device);
        upload->map_ptr += addr.buffer_object->memory_offset;
        assert(upload->map_ptr);
    }
    else
    {
        if (!deferred->upload_heap)
        {
            if (!(deferred->upload_heap = HeapCreate(0, 0, 0)))
            {
                ERR("Failed to create upload heap.\n");
                return false;
            }

            if (!(deferred->upload_heap_refcount = malloc(sizeof(*deferred->upload_heap_refcount))))
            {
                HeapDestroy(deferred->upload_heap);
                deferred->upload_heap = 0;
                return false;
            }

            *deferred->upload_heap_refcount = 1;
        }

        if (!(sysmem = HeapAlloc(deferred->upload_heap, 0, size + RESOURCE_ALIGNMENT - 1)))
            return false;

        upload->bo = NULL;
        upload->sysmem = sysmem;
        upload->map_ptr = (void *)align((size_t)upload->sysmem, RESOURCE_ALIGNMENT);
    }

    upload->upload_flags = UPLOAD_BO_UPLOAD_ON_UNMAP;
    upload->resource = resource;
    wined3d_resource_incref(resource);
    upload->sub_resource_idx = sub_resource_idx;
    upload->box = *box;

    map_desc->data = upload->map_ptr;
    return true;
}

static bool wined3d_deferred_context_unmap_upload_bo(struct wined3d_device_context *context,
        struct wined3d_resource *resource, unsigned int sub_resource_idx, struct wined3d_box *box, struct upload_bo *bo)
{
    struct wined3d_deferred_context *deferred = wined3d_deferred_context_from_context(context);
    struct wined3d_deferred_upload *upload;

    if ((upload = deferred_context_get_upload(deferred, resource, sub_resource_idx)))
    {
        *box = upload->box;
        if ((bo->addr.buffer_object = upload->bo))
            bo->addr.addr = NULL;
        else
            bo->addr.addr = upload->map_ptr;
        bo->flags = upload->upload_flags;
        upload->upload_flags = 0;
        return true;
    }

    return false;
}

static void wined3d_deferred_context_issue_query(struct wined3d_device_context *context,
        struct wined3d_query *query, unsigned int flags)
{
    struct wined3d_deferred_context *deferred = wined3d_deferred_context_from_context(context);
    struct wined3d_cs_query_issue *op;

    op = wined3d_device_context_require_space(context, sizeof(*op), WINED3D_CS_QUEUE_DEFAULT);
    op->opcode = WINED3D_CS_OP_QUERY_ISSUE;
    op->query = query;
    op->flags = flags;

    wined3d_device_context_submit(context, WINED3D_CS_QUEUE_DEFAULT);

    if (!wined3d_array_reserve((void **)&deferred->queries, &deferred->queries_capacity,
            deferred->query_count + 1, sizeof(*deferred->queries)))
    {
        ERR("Failed to reserve memory.\n");
        return;
    }

    deferred->queries[deferred->query_count].flags = flags;
    wined3d_query_incref(deferred->queries[deferred->query_count++].query = query);
}

static void wined3d_deferred_context_flush(struct wined3d_device_context *context)
{
    FIXME("context %p, stub!\n", context);
}

static void wined3d_deferred_context_reference_resource(struct wined3d_device_context *context,
        struct wined3d_resource *resource)
{
    struct wined3d_deferred_context *deferred = wined3d_deferred_context_from_context(context);

    if (!wined3d_array_reserve((void **)&deferred->resources, &deferred->resources_capacity,
            deferred->resource_count + 1, sizeof(*deferred->resources)))
        return;

    deferred->resources[deferred->resource_count++] = resource;
    wined3d_resource_incref(resource);
}

static void wined3d_deferred_context_reference_command_list(struct wined3d_device_context *context,
        struct wined3d_command_list *list)
{
    struct wined3d_deferred_context *deferred = wined3d_deferred_context_from_context(context);

    /* Grab a reference to the command list. Note that this implicitly prevents
     * any dependent command lists or resources from being freed as well. */
    if (!wined3d_array_reserve((void **)&deferred->command_lists, &deferred->command_lists_capacity,
            deferred->command_list_count + 1, sizeof(*deferred->command_lists)))
        return;

    wined3d_command_list_incref(deferred->command_lists[deferred->command_list_count++] = list);
}

static const struct wined3d_device_context_ops wined3d_deferred_context_ops =
{
    wined3d_deferred_context_require_space,
    wined3d_deferred_context_submit,
    wined3d_deferred_context_finish,
    wined3d_deferred_context_map_upload_bo,
    wined3d_deferred_context_unmap_upload_bo,
    wined3d_deferred_context_issue_query,
    wined3d_deferred_context_flush,
    wined3d_deferred_context_reference_resource,
    wined3d_deferred_context_reference_command_list,
};

HRESULT CDECL wined3d_deferred_context_create(struct wined3d_device *device, struct wined3d_device_context **context)
{
    struct wined3d_deferred_context *object;
    HRESULT hr;

    TRACE("device %p, context %p.\n", device, context);

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = wined3d_state_create(device, &device->cs->c.state->feature_level, 1, &object->c.state)))
    {
        free(object);
        return hr;
    }

    object->c.ops = &wined3d_deferred_context_ops;
    object->c.device = device;

    /* Make sure the first command list gets the state reset when executed.
     * Resets for subsequent command lists are encoded in wined3d_deferred_context_record_command_list(). */
    wined3d_device_context_emit_reset_state(&object->c, true);

    TRACE("Created deferred context %p.\n", object);
    *context = &object->c;

    return S_OK;
}

void CDECL wined3d_deferred_context_destroy(struct wined3d_device_context *context)
{
    struct wined3d_deferred_context *deferred = wined3d_deferred_context_from_context(context);
    const struct wined3d_cs_packet *packet;
    SIZE_T i, offset = 0;

    TRACE("context %p.\n", context);

    for (i = 0; i < deferred->resource_count; ++i)
        wined3d_resource_decref(deferred->resources[i]);
    free(deferred->resources);

    for (i = 0; i < deferred->upload_count; ++i)
    {
        wined3d_resource_decref(deferred->uploads[i].resource);
        HeapFree(deferred->upload_heap, 0, deferred->uploads[i].resource);
    }

    if (deferred->upload_heap)
    {
        if (!InterlockedDecrement(deferred->upload_heap_refcount))
        {
            HeapDestroy(deferred->upload_heap);
            free(deferred->upload_heap_refcount);
        }
    }

    free(deferred->uploads);

    for (i = 0; i < deferred->command_list_count; ++i)
        wined3d_command_list_decref(deferred->command_lists[i]);
    free(deferred->command_lists);

    for (i = 0; i < deferred->query_count; ++i)
        wined3d_query_decref(deferred->queries[i].query);
    free(deferred->queries);

    while (offset < deferred->data_size)
    {
        packet = wined3d_next_cs_packet(deferred->data, &offset, ~(SIZE_T)0);
        wined3d_cs_packet_decref_objects(packet);
    }

    wined3d_state_destroy(deferred->c.state);
    free(deferred->data);
    free(deferred);
}

HRESULT CDECL wined3d_deferred_context_record_command_list(struct wined3d_device_context *context,
        bool restore, struct wined3d_command_list **list)
{
    struct wined3d_deferred_context *deferred = wined3d_deferred_context_from_context(context);
    struct wined3d_command_list *object;
    void *memory;

    TRACE("context %p, list %p.\n", context, list);

    wined3d_device_context_lock(context);
    memory = malloc(sizeof(*object) + deferred->resource_count * sizeof(*object->resources)
            + deferred->upload_count * sizeof(*object->uploads)
            + deferred->command_list_count * sizeof(*object->command_lists)
            + deferred->query_count * sizeof(*object->queries)
            + deferred->data_size);

    if (!memory)
    {
        wined3d_device_context_unlock(context);
        return E_OUTOFMEMORY;
    }

    object = memory;
    memory = &object[1];
    memset(object, 0, sizeof(*object));
    object->refcount = 1;
    object->device = deferred->c.device;

    object->resources = memory;
    memory = &object->resources[deferred->resource_count];
    object->resource_count = deferred->resource_count;
    memcpy(object->resources, deferred->resources, deferred->resource_count * sizeof(*object->resources));
    /* Transfer our references to the resources to the command list. */

    object->uploads = memory;
    memory = &object->uploads[deferred->upload_count];
    object->upload_count = deferred->upload_count;
    memcpy(object->uploads, deferred->uploads, deferred->upload_count * sizeof(*object->uploads));
    /* Transfer our references to the resources to the command list. */

    object->command_lists = memory;
    memory = &object->command_lists[deferred->command_list_count];
    object->command_list_count = deferred->command_list_count;
    memcpy(object->command_lists, deferred->command_lists,
            deferred->command_list_count * sizeof(*object->command_lists));
    /* Transfer our references to the command lists to the command list. */

    object->queries = memory;
    memory = &object->queries[deferred->query_count];
    object->query_count = deferred->query_count;
    memcpy(object->queries, deferred->queries, deferred->query_count * sizeof(*object->queries));
    /* Transfer our references to the queries to the command list. */

    object->data = memory;
    object->data_size = deferred->data_size;
    memcpy(object->data, deferred->data, deferred->data_size);

    deferred->data_size = 0;
    deferred->resource_count = 0;
    deferred->upload_count = 0;
    deferred->command_list_count = 0;
    deferred->query_count = 0;

    object->upload_heap = deferred->upload_heap;
    if ((object->upload_heap_refcount = deferred->upload_heap_refcount))
        InterlockedIncrement(object->upload_heap_refcount);

    /* This is in fact recorded into a subsequent command list. */
    if (restore)
        wined3d_device_context_set_state(&deferred->c, deferred->c.state);
    else
        wined3d_device_context_reset_state(&deferred->c);

    TRACE("Created command list %p.\n", object);
    *list = object;
    wined3d_device_context_unlock(context);

    return S_OK;
}

static void wined3d_command_list_destroy_object(void *object)
{
    struct wined3d_command_list *list = object;
    struct wined3d_context *context;
    unsigned int i;

    TRACE("list %p.\n", list);

    context = context_acquire(list->device, NULL, 0);

    for (i = 0; i < list->upload_count; ++i)
    {
        struct wined3d_bo *bo;

        if ((bo = list->uploads[i].bo))
        {
            if (!--bo->refcount)
            {
                wined3d_context_destroy_bo(context, bo);
                free(bo);
            }
        }
        else
        {
            HeapFree(list->upload_heap, 0, list->uploads[i].sysmem);
        }
    }

    context_release(context);

    if (list->upload_heap)
    {
        if (!InterlockedDecrement(list->upload_heap_refcount))
        {
            HeapDestroy(list->upload_heap);
            free(list->upload_heap_refcount);
        }
    }

    free(list);
}

ULONG CDECL wined3d_command_list_incref(struct wined3d_command_list *list)
{
    unsigned int refcount = InterlockedIncrement(&list->refcount);

    TRACE("%p increasing refcount to %u.\n", list, refcount);

    return refcount;
}

ULONG CDECL wined3d_command_list_decref(struct wined3d_command_list *list)
{
    unsigned int refcount = InterlockedDecrement(&list->refcount);
    struct wined3d_device *device = list->device;
    const struct wined3d_cs_packet *packet;
    SIZE_T i, offset;

    TRACE("%p decreasing refcount to %u.\n", list, refcount);

    if (!refcount)
    {
        for (i = 0; i < list->command_list_count; ++i)
            wined3d_command_list_decref(list->command_lists[i]);
        for (i = 0; i < list->resource_count; ++i)
            wined3d_resource_decref(list->resources[i]);
        for (i = 0; i < list->upload_count; ++i)
            wined3d_resource_decref(list->uploads[i].resource);
        for (i = 0; i < list->query_count; ++i)
            wined3d_query_decref(list->queries[i].query);

        offset = 0;
        while (offset < list->data_size)
        {
            packet = wined3d_next_cs_packet(list->data, &offset, ~(SIZE_T)0);
            wined3d_cs_packet_decref_objects(packet);
        }

        wined3d_mutex_lock();
        wined3d_cs_destroy_object(device->cs, wined3d_command_list_destroy_object, list);
        wined3d_mutex_unlock();
    }

    return refcount;
}
