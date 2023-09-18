/*
 * Copyright 2009, 2011 Henri Verbeet for CodeWeavers
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
 *
 */

#include "config.h"
#include "wine/port.h"

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

#define WINED3D_VIEW_CUBE_ARRAY (WINED3D_VIEW_TEXTURE_CUBE | WINED3D_VIEW_TEXTURE_ARRAY)

static BOOL is_stencil_view_format(const struct wined3d_format *format)
{
    return format->id == WINED3DFMT_X24_TYPELESS_G8_UINT
            || format->id == WINED3DFMT_X32_TYPELESS_G8X24_UINT;
}

static GLenum get_texture_view_target(const struct wined3d_gl_info *gl_info,
        const struct wined3d_view_desc *desc, const struct wined3d_texture_gl *texture_gl)
{
    static const struct
    {
        GLenum texture_target;
        unsigned int view_flags;
        GLenum view_target;
        enum wined3d_gl_extension extension;
    }
    view_types[] =
    {
        {GL_TEXTURE_CUBE_MAP,  0, GL_TEXTURE_CUBE_MAP},
        {GL_TEXTURE_RECTANGLE, 0, GL_TEXTURE_RECTANGLE},
        {GL_TEXTURE_3D,        0, GL_TEXTURE_3D},

        {GL_TEXTURE_1D,       0,                          GL_TEXTURE_1D},
        {GL_TEXTURE_1D,       WINED3D_VIEW_TEXTURE_ARRAY, GL_TEXTURE_1D_ARRAY},
        {GL_TEXTURE_1D_ARRAY, 0,                          GL_TEXTURE_1D},
        {GL_TEXTURE_1D_ARRAY, WINED3D_VIEW_TEXTURE_ARRAY, GL_TEXTURE_1D_ARRAY},

        {GL_TEXTURE_2D,       0,                          GL_TEXTURE_2D},
        {GL_TEXTURE_2D,       WINED3D_VIEW_TEXTURE_ARRAY, GL_TEXTURE_2D_ARRAY},
        {GL_TEXTURE_2D_ARRAY, 0,                          GL_TEXTURE_2D},
        {GL_TEXTURE_2D_ARRAY, WINED3D_VIEW_TEXTURE_ARRAY, GL_TEXTURE_2D_ARRAY},
        {GL_TEXTURE_2D_ARRAY, WINED3D_VIEW_TEXTURE_CUBE,  GL_TEXTURE_CUBE_MAP},
        {GL_TEXTURE_2D_ARRAY, WINED3D_VIEW_CUBE_ARRAY,    GL_TEXTURE_CUBE_MAP_ARRAY, ARB_TEXTURE_CUBE_MAP_ARRAY},

        {GL_TEXTURE_2D_MULTISAMPLE,       0,                          GL_TEXTURE_2D_MULTISAMPLE},
        {GL_TEXTURE_2D_MULTISAMPLE,       WINED3D_VIEW_TEXTURE_ARRAY, GL_TEXTURE_2D_MULTISAMPLE_ARRAY},
        {GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 0,                          GL_TEXTURE_2D_MULTISAMPLE},
        {GL_TEXTURE_2D_MULTISAMPLE_ARRAY, WINED3D_VIEW_TEXTURE_ARRAY, GL_TEXTURE_2D_MULTISAMPLE_ARRAY},

        {GL_TEXTURE_1D,       0,                          GL_TEXTURE_1D},
        {GL_TEXTURE_1D,       WINED3D_VIEW_TEXTURE_ARRAY, GL_TEXTURE_1D_ARRAY},
        {GL_TEXTURE_1D_ARRAY, 0,                          GL_TEXTURE_1D},
        {GL_TEXTURE_1D_ARRAY, WINED3D_VIEW_TEXTURE_ARRAY, GL_TEXTURE_1D_ARRAY},
    };
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(view_types); ++i)
    {
        if (view_types[i].texture_target != texture_gl->target || view_types[i].view_flags != desc->flags)
            continue;
        if (gl_info->supported[view_types[i].extension])
            return view_types[i].view_target;

        FIXME("Extension %#x not supported.\n", view_types[i].extension);
    }

    FIXME("Unhandled view flags %#x for texture target %#x.\n", desc->flags, texture_gl->target);
    return texture_gl->target;
}

static const struct wined3d_format *validate_resource_view(const struct wined3d_view_desc *desc,
        struct wined3d_resource *resource, BOOL mip_slice, BOOL allow_srgb_toggle)
{
    const struct wined3d_adapter *adapter = resource->device->adapter;
    const struct wined3d_format *format;

    format = wined3d_get_format(adapter, desc->format_id, resource->bind_flags);
    if (resource->type == WINED3D_RTYPE_BUFFER && (desc->flags & WINED3D_VIEW_BUFFER_RAW))
    {
        if (format->id != WINED3DFMT_R32_TYPELESS)
        {
            WARN("Invalid format %s for raw buffer view.\n", debug_d3dformat(format->id));
            return NULL;
        }

        format = wined3d_get_format(adapter, WINED3DFMT_R32_UINT, resource->bind_flags);
    }

    if (wined3d_format_is_typeless(format))
    {
        WARN("Trying to create view for typeless format %s.\n", debug_d3dformat(format->id));
        return NULL;
    }

    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        struct wined3d_buffer *buffer = buffer_from_resource(resource);
        unsigned int buffer_size, element_size;

        if (buffer->structure_byte_stride)
        {
            if (desc->format_id != WINED3DFMT_UNKNOWN)
            {
                WARN("Invalid format %s for structured buffer view.\n", debug_d3dformat(desc->format_id));
                return NULL;
            }

            format = wined3d_get_format(adapter, WINED3DFMT_R32_UINT, resource->bind_flags);
            element_size = buffer->structure_byte_stride;
        }
        else
        {
            element_size = format->byte_count;
        }

        if (!element_size)
            return NULL;

        buffer_size = buffer->resource.size / element_size;
        if (desc->u.buffer.start_idx >= buffer_size
                || desc->u.buffer.count > buffer_size - desc->u.buffer.start_idx)
            return NULL;
    }
    else
    {
        struct wined3d_texture *texture = texture_from_resource(resource);
        unsigned int depth_or_layer_count;

        if (resource->format->id != format->id && !wined3d_format_is_typeless(resource->format)
                && (!allow_srgb_toggle || !wined3d_formats_are_srgb_variants(resource->format->id, format->id)))
        {
            WARN("Trying to create incompatible view for non typeless format %s.\n",
                    debug_d3dformat(format->id));
            return NULL;
        }

        if (mip_slice && resource->type == WINED3D_RTYPE_TEXTURE_3D)
            depth_or_layer_count = wined3d_texture_get_level_depth(texture, desc->u.texture.level_idx);
        else
            depth_or_layer_count = texture->layer_count;

        if (!desc->u.texture.level_count
                || (mip_slice && desc->u.texture.level_count != 1)
                || desc->u.texture.level_idx >= texture->level_count
                || desc->u.texture.level_count > texture->level_count - desc->u.texture.level_idx
                || !desc->u.texture.layer_count
                || desc->u.texture.layer_idx >= depth_or_layer_count
                || desc->u.texture.layer_count > depth_or_layer_count - desc->u.texture.layer_idx)
            return NULL;
    }

    return format;
}

static void create_texture_view(struct wined3d_gl_view *view, GLenum view_target,
        const struct wined3d_view_desc *desc, struct wined3d_texture_gl *texture_gl,
        const struct wined3d_format *view_format)
{
    const struct wined3d_format_gl *view_format_gl;
    unsigned int level_idx, layer_idx, layer_count;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context_gl *context_gl;
    struct wined3d_context *context;
    GLuint texture_name;

    view_format_gl = wined3d_format_gl(view_format);
    view->target = view_target;

    context = context_acquire(texture_gl->t.resource.device, NULL, 0);
    context_gl = wined3d_context_gl(context);
    gl_info = context_gl->gl_info;

    if (!gl_info->supported[ARB_TEXTURE_VIEW])
    {
        context_release(context);
        FIXME("OpenGL implementation does not support texture views.\n");
        return;
    }

    wined3d_texture_gl_prepare_texture(texture_gl, context_gl, FALSE);
    texture_name = wined3d_texture_gl_get_texture_name(texture_gl, context, FALSE);

    level_idx = desc->u.texture.level_idx;
    layer_idx = desc->u.texture.layer_idx;
    layer_count = desc->u.texture.layer_count;
    if (view_target == GL_TEXTURE_3D)
    {
        if (layer_idx || layer_count != wined3d_texture_get_level_depth(&texture_gl->t, level_idx))
            FIXME("Depth slice (%u-%u) not supported.\n", layer_idx, layer_count);
        layer_idx = 0;
        layer_count = 1;
    }

    gl_info->gl_ops.gl.p_glGenTextures(1, &view->name);
    GL_EXTCALL(glTextureView(view->name, view->target, texture_name, view_format_gl->internal,
            level_idx, desc->u.texture.level_count, layer_idx, layer_count));
    checkGLcall("create texture view");

    if (is_stencil_view_format(view_format))
    {
        static const GLint swizzle[] = {GL_ZERO, GL_RED, GL_ZERO, GL_ZERO};

        if (!gl_info->supported[ARB_STENCIL_TEXTURING])
        {
            context_release(context);
            FIXME("OpenGL implementation does not support stencil texturing.\n");
            return;
        }

        wined3d_context_gl_bind_texture(context_gl, view->target, view->name);
        gl_info->gl_ops.gl.p_glTexParameteriv(view->target, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
        gl_info->gl_ops.gl.p_glTexParameteri(view->target, GL_DEPTH_STENCIL_TEXTURE_MODE, GL_STENCIL_INDEX);
        checkGLcall("initialize stencil view");

        context_invalidate_compute_state(context, STATE_COMPUTE_SHADER_RESOURCE_BINDING);
        context_invalidate_state(context, STATE_GRAPHICS_SHADER_RESOURCE_BINDING);
    }
    else if (!is_identity_fixup(view_format->color_fixup) && can_use_texture_swizzle(context->d3d_info, view_format))
    {
        GLint swizzle[4];

        wined3d_context_gl_bind_texture(context_gl, view->target, view->name);
        wined3d_gl_texture_swizzle_from_color_fixup(swizzle, view_format->color_fixup);
        gl_info->gl_ops.gl.p_glTexParameteriv(view->target, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
        checkGLcall("set format swizzle");

        context_invalidate_compute_state(context, STATE_COMPUTE_SHADER_RESOURCE_BINDING);
        context_invalidate_state(context, STATE_GRAPHICS_SHADER_RESOURCE_BINDING);
    }

    context_release(context);
}

static void create_buffer_texture(struct wined3d_gl_view *view, struct wined3d_context *context,
        struct wined3d_buffer *buffer, const struct wined3d_format *view_format,
        unsigned int offset, unsigned int size)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    const struct wined3d_format_gl *view_format_gl;

    if (!gl_info->supported[ARB_TEXTURE_BUFFER_OBJECT])
    {
        FIXME("OpenGL implementation does not support buffer textures.\n");
        return;
    }

    if ((offset & (gl_info->limits.texture_buffer_offset_alignment - 1)))
    {
        FIXME("Buffer offset %u is not %u byte aligned.\n",
                offset, gl_info->limits.texture_buffer_offset_alignment);
        return;
    }

    view_format_gl = wined3d_format_gl(view_format);
    wined3d_buffer_load_location(buffer, context, WINED3D_LOCATION_BUFFER);

    view->target = GL_TEXTURE_BUFFER;
    gl_info->gl_ops.gl.p_glGenTextures(1, &view->name);

    wined3d_context_gl_bind_texture(context_gl, GL_TEXTURE_BUFFER, view->name);
    if (gl_info->supported[ARB_TEXTURE_BUFFER_RANGE])
    {
        GL_EXTCALL(glTexBufferRange(GL_TEXTURE_BUFFER, view_format_gl->internal,
                buffer->buffer_object, offset, size));
    }
    else
    {
        if (offset || size != buffer->resource.size)
            FIXME("OpenGL implementation does not support ARB_texture_buffer_range.\n");
        GL_EXTCALL(glTexBuffer(GL_TEXTURE_BUFFER, view_format_gl->internal, buffer->buffer_object));
    }
    checkGLcall("Create buffer texture");

    context_invalidate_compute_state(context, STATE_COMPUTE_SHADER_RESOURCE_BINDING);
    context_invalidate_state(context, STATE_GRAPHICS_SHADER_RESOURCE_BINDING);
}

static void get_buffer_view_range(const struct wined3d_buffer *buffer,
        const struct wined3d_view_desc *desc, const struct wined3d_format *view_format,
        unsigned int *offset, unsigned int *size)
{
    if (desc->format_id == WINED3DFMT_UNKNOWN)
    {
        *offset = desc->u.buffer.start_idx * buffer->structure_byte_stride;
        *size = desc->u.buffer.count * buffer->structure_byte_stride;
    }
    else
    {
        *offset = desc->u.buffer.start_idx * view_format->byte_count;
        *size = desc->u.buffer.count * view_format->byte_count;
    }
}

static void create_buffer_view(struct wined3d_gl_view *view, struct wined3d_context *context,
        const struct wined3d_view_desc *desc, struct wined3d_buffer *buffer,
        const struct wined3d_format *view_format)
{
    unsigned int offset, size;

    get_buffer_view_range(buffer, desc, view_format, &offset, &size);
    create_buffer_texture(view, context, buffer, view_format, offset, size);
}

static void wined3d_view_invalidate_location(struct wined3d_resource *resource,
        const struct wined3d_view_desc *desc, DWORD location)
{
    unsigned int i, sub_resource_idx, layer_count;
    struct wined3d_texture *texture;

    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        wined3d_buffer_invalidate_location(buffer_from_resource(resource), location);
        return;
    }

    texture = texture_from_resource(resource);

    sub_resource_idx = desc->u.texture.layer_idx * texture->level_count + desc->u.texture.level_idx;
    layer_count = resource->type != WINED3D_RTYPE_TEXTURE_3D ? desc->u.texture.layer_count : 1;
    for (i = 0; i < layer_count; ++i, sub_resource_idx += texture->level_count)
        wined3d_texture_invalidate_location(texture, sub_resource_idx, location);
}

ULONG CDECL wined3d_rendertarget_view_incref(struct wined3d_rendertarget_view *view)
{
    ULONG refcount = InterlockedIncrement(&view->refcount);

    TRACE("%p increasing refcount to %u.\n", view, refcount);

    return refcount;
}

void wined3d_rendertarget_view_cleanup(struct wined3d_rendertarget_view *view)
{
    /* Call wined3d_object_destroyed() before releasing the resource,
     * since releasing the resource may end up destroying the parent. */
    view->parent_ops->wined3d_object_destroyed(view->parent);
    wined3d_resource_decref(view->resource);
}

ULONG CDECL wined3d_rendertarget_view_decref(struct wined3d_rendertarget_view *view)
{
    ULONG refcount = InterlockedDecrement(&view->refcount);

    TRACE("%p decreasing refcount to %u.\n", view, refcount);

    if (!refcount)
        view->resource->device->adapter->adapter_ops->adapter_destroy_rendertarget_view(view);

    return refcount;
}

void * CDECL wined3d_rendertarget_view_get_parent(const struct wined3d_rendertarget_view *view)
{
    TRACE("view %p.\n", view);

    return view->parent;
}

void * CDECL wined3d_rendertarget_view_get_sub_resource_parent(const struct wined3d_rendertarget_view *view)
{
    struct wined3d_texture *texture;

    TRACE("view %p.\n", view);

    if (view->resource->type == WINED3D_RTYPE_BUFFER)
        return wined3d_buffer_get_parent(buffer_from_resource(view->resource));

    texture = texture_from_resource(view->resource);

    return texture->sub_resources[view->sub_resource_idx].parent;
}

void CDECL wined3d_rendertarget_view_set_parent(struct wined3d_rendertarget_view *view, void *parent)
{
    TRACE("view %p, parent %p.\n", view, parent);

    view->parent = parent;
}

struct wined3d_resource * CDECL wined3d_rendertarget_view_get_resource(const struct wined3d_rendertarget_view *view)
{
    TRACE("view %p.\n", view);

    return view->resource;
}

void wined3d_rendertarget_view_get_drawable_size(const struct wined3d_rendertarget_view *view,
        const struct wined3d_context *context, unsigned int *width, unsigned int *height)
{
    const struct wined3d_texture *texture;

    if (view->resource->type != WINED3D_RTYPE_TEXTURE_2D)
    {
        *width = view->width;
        *height = view->height;
        return;
    }

    texture = texture_from_resource(view->resource);
    if (texture->swapchain)
    {
        /* The drawable size of an onscreen drawable is the surface size.
         * (Actually: The window size, but the surface is created in window
         * size.) */
        *width = texture->resource.width;
        *height = texture->resource.height;
    }
    else if (wined3d_settings.offscreen_rendering_mode == ORM_BACKBUFFER)
    {
        const struct wined3d_swapchain_desc *desc = &context->swapchain->state.desc;

        /* The drawable size of a backbuffer / aux buffer offscreen target is
         * the size of the current context's drawable, which is the size of
         * the back buffer of the swapchain the active context belongs to. */
        *width = desc->backbuffer_width;
        *height = desc->backbuffer_height;
    }
    else
    {
        unsigned int level_idx = view->sub_resource_idx % texture->level_count;

        /* The drawable size of an FBO target is the OpenGL texture size,
         * which is the power of two size. */
        *width = wined3d_texture_get_level_pow2_width(texture, level_idx);
        *height = wined3d_texture_get_level_pow2_height(texture, level_idx);
    }
}

void wined3d_rendertarget_view_prepare_location(struct wined3d_rendertarget_view *view,
        struct wined3d_context *context, DWORD location)
{
    struct wined3d_resource *resource = view->resource;
    unsigned int i, sub_resource_idx, layer_count;
    struct wined3d_texture *texture;

    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        FIXME("Not implemented for resources %s.\n", debug_d3dresourcetype(resource->type));
        return;
    }

    texture = texture_from_resource(resource);
    sub_resource_idx = view->sub_resource_idx;
    layer_count = resource->type != WINED3D_RTYPE_TEXTURE_3D ? view->layer_count : 1;
    for (i = 0; i < layer_count; ++i, sub_resource_idx += texture->level_count)
        wined3d_texture_prepare_location(texture, sub_resource_idx, context, location);
}

void wined3d_rendertarget_view_load_location(struct wined3d_rendertarget_view *view,
        struct wined3d_context *context, DWORD location)
{
    struct wined3d_resource *resource = view->resource;
    unsigned int i, sub_resource_idx, layer_count;
    struct wined3d_texture *texture;

    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        wined3d_buffer_load_location(buffer_from_resource(resource), context, location);
        return;
    }

    texture = texture_from_resource(resource);
    sub_resource_idx = view->sub_resource_idx;
    layer_count = resource->type != WINED3D_RTYPE_TEXTURE_3D ? view->layer_count : 1;
    for (i = 0; i < layer_count; ++i, sub_resource_idx += texture->level_count)
        wined3d_texture_load_location(texture, sub_resource_idx, context, location);
}

void wined3d_rendertarget_view_validate_location(struct wined3d_rendertarget_view *view, DWORD location)
{
    struct wined3d_resource *resource = view->resource;
    unsigned int i, sub_resource_idx, layer_count;
    struct wined3d_texture *texture;

    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        FIXME("Not implemented for resources %s.\n", debug_d3dresourcetype(resource->type));
        return;
    }

    texture = texture_from_resource(resource);
    sub_resource_idx = view->sub_resource_idx;
    layer_count = resource->type != WINED3D_RTYPE_TEXTURE_3D ? view->layer_count : 1;
    for (i = 0; i < layer_count; ++i, sub_resource_idx += texture->level_count)
        wined3d_texture_validate_location(texture, sub_resource_idx, location);
}

void wined3d_rendertarget_view_invalidate_location(struct wined3d_rendertarget_view *view, DWORD location)
{
    wined3d_view_invalidate_location(view->resource, &view->desc, location);
}

static void wined3d_render_target_view_gl_cs_init(void *object)
{
    struct wined3d_rendertarget_view_gl *view_gl = object;
    struct wined3d_resource *resource = view_gl->v.resource;
    const struct wined3d_view_desc *desc = &view_gl->v.desc;

    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        FIXME("Not implemented for resources %s.\n", debug_d3dresourcetype(resource->type));
    }
    else
    {
        struct wined3d_texture_gl *texture_gl = wined3d_texture_gl(texture_from_resource(resource));
        unsigned int depth_or_layer_count;

        if (resource->type == WINED3D_RTYPE_TEXTURE_3D)
            depth_or_layer_count = wined3d_texture_get_level_depth(&texture_gl->t, desc->u.texture.level_idx);
        else
            depth_or_layer_count = texture_gl->t.layer_count;

        if (resource->format->id != view_gl->v.format->id
                || (view_gl->v.layer_count != 1 && view_gl->v.layer_count != depth_or_layer_count))
        {
            GLenum resource_class, view_class;

            resource_class = wined3d_format_gl(resource->format)->view_class;
            view_class = wined3d_format_gl(view_gl->v.format)->view_class;
            if (resource_class != view_class)
            {
                FIXME("Render target view not supported, resource format %s, view format %s.\n",
                        debug_d3dformat(resource->format->id), debug_d3dformat(view_gl->v.format->id));
                return;
            }
            if (texture_gl->t.swapchain && texture_gl->t.swapchain->state.desc.backbuffer_count > 1)
            {
                FIXME("Swapchain views not supported.\n");
                return;
            }

            create_texture_view(&view_gl->gl_view, texture_gl->target, desc, texture_gl, view_gl->v.format);
        }
    }
}

static HRESULT wined3d_rendertarget_view_init(struct wined3d_rendertarget_view *view,
        const struct wined3d_view_desc *desc, struct wined3d_resource *resource,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    BOOL allow_srgb_toggle = FALSE;

    view->refcount = 1;
    view->parent = parent;
    view->parent_ops = parent_ops;

    if (resource->type != WINED3D_RTYPE_BUFFER)
    {
        struct wined3d_texture *texture = texture_from_resource(resource);

        if (texture->swapchain)
            allow_srgb_toggle = TRUE;
    }
    if (!(view->format = validate_resource_view(desc, resource, TRUE, allow_srgb_toggle)))
        return E_INVALIDARG;
    view->format_flags = view->format->flags[resource->gl_type];
    view->desc = *desc;

    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        view->sub_resource_idx = 0;
        view->layer_count = 1;
        view->width = desc->u.buffer.count;
        view->height = 1;
    }
    else
    {
        struct wined3d_texture *texture = texture_from_resource(resource);

        view->sub_resource_idx = desc->u.texture.level_idx;
        if (resource->type != WINED3D_RTYPE_TEXTURE_3D)
            view->sub_resource_idx += desc->u.texture.layer_idx * texture->level_count;
        view->layer_count = desc->u.texture.layer_count;
        view->width = wined3d_texture_get_level_width(texture, desc->u.texture.level_idx);
        view->height = wined3d_texture_get_level_height(texture, desc->u.texture.level_idx);
    }

    wined3d_resource_incref(view->resource = resource);

    return WINED3D_OK;
}

HRESULT wined3d_rendertarget_view_no3d_init(struct wined3d_rendertarget_view *view_no3d,
        const struct wined3d_view_desc *desc, struct wined3d_resource *resource,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    TRACE("view_no3d %p, desc %s, resource %p, parent %p, parent_ops %p.\n",
            view_no3d, wined3d_debug_view_desc(desc, resource), resource, parent, parent_ops);

    return wined3d_rendertarget_view_init(view_no3d, desc, resource, parent, parent_ops);
}

HRESULT wined3d_rendertarget_view_gl_init(struct wined3d_rendertarget_view_gl *view_gl,
        const struct wined3d_view_desc *desc, struct wined3d_resource *resource,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    HRESULT hr;

    TRACE("view_gl %p, desc %s, resource %p, parent %p, parent_ops %p.\n",
            view_gl, wined3d_debug_view_desc(desc, resource), resource, parent, parent_ops);

    if (FAILED(hr = wined3d_rendertarget_view_init(&view_gl->v, desc, resource, parent, parent_ops)))
        return hr;

    wined3d_cs_init_object(resource->device->cs, wined3d_render_target_view_gl_cs_init, view_gl);

    return hr;
}

HRESULT wined3d_rendertarget_view_vk_init(struct wined3d_rendertarget_view_vk *view_vk,
        const struct wined3d_view_desc *desc, struct wined3d_resource *resource,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    TRACE("view_vk %p, desc %s, resource %p, parent %p, parent_ops %p.\n",
            view_vk, wined3d_debug_view_desc(desc, resource), resource, parent, parent_ops);

    return wined3d_rendertarget_view_init(&view_vk->v, desc, resource, parent, parent_ops);
}

HRESULT CDECL wined3d_rendertarget_view_create(const struct wined3d_view_desc *desc,
        struct wined3d_resource *resource, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_rendertarget_view **view)
{
    const struct wined3d_adapter_ops *adapter_ops;

    TRACE("desc %s, resource %p, parent %p, parent_ops %p, view %p.\n",
            wined3d_debug_view_desc(desc, resource), resource, parent, parent_ops, view);

    adapter_ops = resource->device->adapter->adapter_ops;
    return adapter_ops->adapter_create_rendertarget_view(desc, resource, parent, parent_ops, view);
}

HRESULT CDECL wined3d_rendertarget_view_create_from_sub_resource(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_rendertarget_view **view)
{
    struct wined3d_view_desc desc;

    TRACE("texture %p, sub_resource_idx %u, parent %p, parent_ops %p, view %p.\n",
            texture, sub_resource_idx, parent, parent_ops, view);

    desc.format_id = texture->resource.format->id;
    desc.flags = 0;
    desc.u.texture.level_idx = sub_resource_idx % texture->level_count;
    desc.u.texture.level_count = 1;
    desc.u.texture.layer_idx = sub_resource_idx / texture->level_count;
    desc.u.texture.layer_count = 1;

    return wined3d_rendertarget_view_create(&desc, &texture->resource, parent, parent_ops, view);
}

ULONG CDECL wined3d_shader_resource_view_incref(struct wined3d_shader_resource_view *view)
{
    ULONG refcount = InterlockedIncrement(&view->refcount);

    TRACE("%p increasing refcount to %u.\n", view, refcount);

    return refcount;
}

void wined3d_shader_resource_view_cleanup(struct wined3d_shader_resource_view *view)
{
    /* Call wined3d_object_destroyed() before releasing the resource,
     * since releasing the resource may end up destroying the parent. */
    view->parent_ops->wined3d_object_destroyed(view->parent);
    wined3d_resource_decref(view->resource);
}

ULONG CDECL wined3d_shader_resource_view_decref(struct wined3d_shader_resource_view *view)
{
    ULONG refcount = InterlockedDecrement(&view->refcount);

    TRACE("%p decreasing refcount to %u.\n", view, refcount);

    if (!refcount)
        view->resource->device->adapter->adapter_ops->adapter_destroy_shader_resource_view(view);

    return refcount;
}

void * CDECL wined3d_shader_resource_view_get_parent(const struct wined3d_shader_resource_view *view)
{
    TRACE("view %p.\n", view);

    return view->parent;
}

static void wined3d_shader_resource_view_gl_cs_init(void *object)
{
    struct wined3d_shader_resource_view_gl *view_gl = object;
    struct wined3d_resource *resource = view_gl->v.resource;
    const struct wined3d_format *view_format;
    const struct wined3d_gl_info *gl_info;
    const struct wined3d_view_desc *desc;
    GLenum view_target;

    view_format = view_gl->v.format;
    gl_info = &resource->device->adapter->gl_info;
    desc = &view_gl->v.desc;

    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        struct wined3d_buffer *buffer = buffer_from_resource(resource);
        struct wined3d_context *context;

        context = context_acquire(resource->device, NULL, 0);
        create_buffer_view(&view_gl->gl_view, context, desc, buffer, view_format);
        context_release(context);
    }
    else
    {
        struct wined3d_texture_gl *texture_gl = wined3d_texture_gl(texture_from_resource(resource));
        GLenum resource_class, view_class;

        resource_class = wined3d_format_gl(resource->format)->view_class;
        view_class = wined3d_format_gl(view_format)->view_class;
        view_target = get_texture_view_target(gl_info, desc, texture_gl);

        if (resource->format->id == view_format->id && texture_gl->target == view_target
                && !desc->u.texture.level_idx && desc->u.texture.level_count == texture_gl->t.level_count
                && !desc->u.texture.layer_idx && desc->u.texture.layer_count == texture_gl->t.layer_count
                && !is_stencil_view_format(view_format))
        {
            TRACE("Creating identity shader resource view.\n");
        }
        else if (texture_gl->t.swapchain && texture_gl->t.swapchain->state.desc.backbuffer_count > 1)
        {
            FIXME("Swapchain shader resource views not supported.\n");
        }
        else if (resource->format->typeless_id == view_format->typeless_id
                && resource_class == view_class)
        {
            create_texture_view(&view_gl->gl_view, view_target, desc, texture_gl, view_format);
        }
        else if (wined3d_format_is_depth_view(resource->format->id, view_format->id))
        {
            create_texture_view(&view_gl->gl_view, view_target, desc, texture_gl, resource->format);
        }
        else
        {
            FIXME("Shader resource view not supported, resource format %s, view format %s.\n",
                    debug_d3dformat(resource->format->id), debug_d3dformat(view_format->id));
        }
    }
#if defined(STAGING_CSMT)

    wined3d_resource_release(resource);
#endif /* STAGING_CSMT */
}

static HRESULT wined3d_shader_resource_view_init(struct wined3d_shader_resource_view *view,
        const struct wined3d_view_desc *desc, struct wined3d_resource *resource,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    view->refcount = 1;
    view->parent = parent;
    view->parent_ops = parent_ops;

    if (!(resource->bind_flags & WINED3D_BIND_SHADER_RESOURCE))
        return E_INVALIDARG;
    if (!(view->format = validate_resource_view(desc, resource, FALSE, FALSE)))
        return E_INVALIDARG;
    view->desc = *desc;

    wined3d_resource_incref(view->resource = resource);

#if defined(STAGING_CSMT)
    wined3d_resource_acquire(resource);
#endif /* STAGING_CSMT */

    return WINED3D_OK;
}

HRESULT wined3d_shader_resource_view_gl_init(struct wined3d_shader_resource_view_gl *view_gl,
        const struct wined3d_view_desc *desc, struct wined3d_resource *resource,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    HRESULT hr;

    TRACE("view_gl %p, desc %s, resource %p, parent %p, parent_ops %p.\n",
            view_gl, wined3d_debug_view_desc(desc, resource), resource, parent, parent_ops);

    if (FAILED(hr = wined3d_shader_resource_view_init(&view_gl->v, desc, resource, parent, parent_ops)))
        return hr;

    wined3d_cs_init_object(resource->device->cs, wined3d_shader_resource_view_gl_cs_init, view_gl);

    return hr;
}

HRESULT wined3d_shader_resource_view_vk_init(struct wined3d_shader_resource_view_vk *view_vk,
        const struct wined3d_view_desc *desc, struct wined3d_resource *resource,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    TRACE("view_vk %p, desc %s, resource %p, parent %p, parent_ops %p.\n",
            view_vk, wined3d_debug_view_desc(desc, resource), resource, parent, parent_ops);

    return wined3d_shader_resource_view_init(&view_vk->v, desc, resource, parent, parent_ops);
}

HRESULT CDECL wined3d_shader_resource_view_create(const struct wined3d_view_desc *desc,
        struct wined3d_resource *resource, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_shader_resource_view **view)
{
    const struct wined3d_adapter_ops *adapter_ops;

    TRACE("desc %s, resource %p, parent %p, parent_ops %p, view %p.\n",
            wined3d_debug_view_desc(desc, resource), resource, parent, parent_ops, view);

    adapter_ops = resource->device->adapter->adapter_ops;
    return adapter_ops->adapter_create_shader_resource_view(desc, resource, parent, parent_ops, view);
}

void wined3d_shader_resource_view_gl_bind(struct wined3d_shader_resource_view_gl *view_gl,
        unsigned int unit, struct wined3d_sampler_gl *sampler_gl, struct wined3d_context_gl *context_gl)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_texture_gl *texture_gl;

    wined3d_context_gl_active_texture(context_gl, gl_info, unit);

    if (view_gl->gl_view.name)
    {
        wined3d_context_gl_bind_texture(context_gl, view_gl->gl_view.target, view_gl->gl_view.name);
        wined3d_sampler_gl_bind(sampler_gl, unit, NULL, context_gl);
        return;
    }

    if (view_gl->v.resource->type == WINED3D_RTYPE_BUFFER)
    {
        FIXME("Buffer shader resources not supported.\n");
        return;
    }

    texture_gl = wined3d_texture_gl(wined3d_texture_from_resource(view_gl->v.resource));
    wined3d_texture_gl_bind(texture_gl, context_gl, FALSE);
    wined3d_sampler_gl_bind(sampler_gl, unit, texture_gl, context_gl);
}

/* Context activation is done by the caller. */
static void shader_resource_view_gl_bind_and_dirtify(struct wined3d_shader_resource_view_gl *view_gl,
        struct wined3d_context_gl *context_gl)
{
    if (context_gl->active_texture < ARRAY_SIZE(context_gl->rev_tex_unit_map))
    {
        unsigned int active_sampler = context_gl->rev_tex_unit_map[context_gl->active_texture];
        if (active_sampler != WINED3D_UNMAPPED_STAGE)
            context_invalidate_state(&context_gl->c, STATE_SAMPLER(active_sampler));
    }
    /* FIXME: Ideally we'd only do this when touching a binding that's used by
     * a shader. */
    context_invalidate_compute_state(&context_gl->c, STATE_COMPUTE_SHADER_RESOURCE_BINDING);
    context_invalidate_state(&context_gl->c, STATE_GRAPHICS_SHADER_RESOURCE_BINDING);

    wined3d_context_gl_bind_texture(context_gl, view_gl->gl_view.target, view_gl->gl_view.name);
}

void shader_resource_view_generate_mipmaps(struct wined3d_shader_resource_view *view)
{
    struct wined3d_shader_resource_view_gl *view_gl = wined3d_shader_resource_view_gl(view);
    unsigned int i, j, layer_count, level_count, base_level, max_level;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_texture_gl *texture_gl;
    struct wined3d_context_gl *context_gl;
    struct wined3d_context *context;
    struct gl_texture *gl_tex;
    DWORD location;
    BOOL srgb;

    TRACE("view %p.\n", view);

    context = context_acquire(view_gl->v.resource->device, NULL, 0);
    context_gl = wined3d_context_gl(context);
    gl_info = context_gl->gl_info;
    layer_count = view_gl->v.desc.u.texture.layer_count;
    level_count = view_gl->v.desc.u.texture.level_count;
    base_level = view_gl->v.desc.u.texture.level_idx;
    max_level = base_level + level_count - 1;

    texture_gl = wined3d_texture_gl(texture_from_resource(view_gl->v.resource));
    srgb = !!(texture_gl->t.flags & WINED3D_TEXTURE_IS_SRGB);
    location = srgb ? WINED3D_LOCATION_TEXTURE_SRGB : WINED3D_LOCATION_TEXTURE_RGB;
    for (i = 0; i < layer_count; ++i)
        wined3d_texture_load_location(&texture_gl->t, i * level_count + base_level, context, location);

    if (view_gl->gl_view.name)
    {
        shader_resource_view_gl_bind_and_dirtify(view_gl, context_gl);
    }
    else
    {
        wined3d_texture_gl_bind_and_dirtify(texture_gl, context_gl, srgb);
        gl_info->gl_ops.gl.p_glTexParameteri(texture_gl->target, GL_TEXTURE_BASE_LEVEL, base_level);
        gl_info->gl_ops.gl.p_glTexParameteri(texture_gl->target, GL_TEXTURE_MAX_LEVEL, max_level);
    }

    if (gl_info->supported[ARB_SAMPLER_OBJECTS])
        GL_EXTCALL(glBindSampler(context_gl->active_texture, 0));
    gl_tex = wined3d_texture_gl_get_gl_texture(texture_gl, srgb);
    if (context->d3d_info->wined3d_creation_flags & WINED3D_SRGB_READ_WRITE_CONTROL)
    {
        gl_info->gl_ops.gl.p_glTexParameteri(texture_gl->target,
                GL_TEXTURE_SRGB_DECODE_EXT, GL_SKIP_DECODE_EXT);
        gl_tex->sampler_desc.srgb_decode = FALSE;
    }

    gl_info->fbo_ops.glGenerateMipmap(texture_gl->target);
    checkGLcall("glGenerateMipMap()");

    for (i = 0; i < layer_count; ++i)
    {
        for (j = base_level + 1; j <= max_level; ++j)
        {
            wined3d_texture_validate_location(&texture_gl->t, i * level_count + j, location);
            wined3d_texture_invalidate_location(&texture_gl->t, i * level_count + j, ~location);
        }
    }

    if (!view_gl->gl_view.name)
    {
        gl_tex->base_level = base_level;
        gl_info->gl_ops.gl.p_glTexParameteri(texture_gl->target,
                GL_TEXTURE_MAX_LEVEL, texture_gl->t.level_count - 1);
    }

    context_release(context);
}

void CDECL wined3d_shader_resource_view_generate_mipmaps(struct wined3d_shader_resource_view *view)
{
    struct wined3d_texture *texture;

    TRACE("view %p.\n", view);

    if (view->resource->type == WINED3D_RTYPE_BUFFER)
    {
        WARN("Called on buffer resource %p.\n", view->resource);
        return;
    }

    texture = texture_from_resource(view->resource);
    if (!(texture->flags & WINED3D_TEXTURE_GENERATE_MIPMAPS))
    {
        WARN("Texture without the WINED3D_TEXTURE_GENERATE_MIPMAPS flag, ignoring.\n");
        return;
    }

    wined3d_cs_emit_generate_mipmaps(view->resource->device->cs, view);
}

ULONG CDECL wined3d_unordered_access_view_incref(struct wined3d_unordered_access_view *view)
{
    ULONG refcount = InterlockedIncrement(&view->refcount);

    TRACE("%p increasing refcount to %u.\n", view, refcount);

    return refcount;
}

void wined3d_unordered_access_view_cleanup(struct wined3d_unordered_access_view *view)
{
    /* Call wined3d_object_destroyed() before releasing the resource,
     * since releasing the resource may end up destroying the parent. */
    view->parent_ops->wined3d_object_destroyed(view->parent);
    wined3d_resource_decref(view->resource);
}

ULONG CDECL wined3d_unordered_access_view_decref(struct wined3d_unordered_access_view *view)
{
    ULONG refcount = InterlockedDecrement(&view->refcount);

    TRACE("%p decreasing refcount to %u.\n", view, refcount);

    if (!refcount)
        view->resource->device->adapter->adapter_ops->adapter_destroy_unordered_access_view(view);

    return refcount;
}

void * CDECL wined3d_unordered_access_view_get_parent(const struct wined3d_unordered_access_view *view)
{
    TRACE("view %p.\n", view);

    return view->parent;
}

void wined3d_unordered_access_view_invalidate_location(struct wined3d_unordered_access_view *view,
        DWORD location)
{
    wined3d_view_invalidate_location(view->resource, &view->desc, location);
}

void wined3d_unordered_access_view_gl_clear_uint(struct wined3d_unordered_access_view_gl *view_gl,
        const struct wined3d_uvec4 *clear_value, struct wined3d_context_gl *context_gl)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    const struct wined3d_format_gl *format;
    struct wined3d_buffer_gl *buffer_gl;
    struct wined3d_resource *resource;
    unsigned int offset, size;

    resource = view_gl->v.resource;
    if (resource->type != WINED3D_RTYPE_BUFFER)
    {
        FIXME("Not implemented for %s resources.\n", debug_d3dresourcetype(resource->type));
        return;
    }

    if (!gl_info->supported[ARB_CLEAR_BUFFER_OBJECT])
    {
        FIXME("OpenGL implementation does not support ARB_clear_buffer_object.\n");
        return;
    }

    format = wined3d_format_gl(view_gl->v.format);
    if (format->f.id != WINED3DFMT_R32_UINT && format->f.id != WINED3DFMT_R32_SINT
            && format->f.id != WINED3DFMT_R32G32B32A32_UINT
            && format->f.id != WINED3DFMT_R32G32B32A32_SINT)
    {
        FIXME("Not implemented for format %s.\n", debug_d3dformat(format->f.id));
        return;
    }

    buffer_gl = wined3d_buffer_gl(buffer_from_resource(resource));
    wined3d_buffer_load_location(&buffer_gl->b, &context_gl->c, WINED3D_LOCATION_BUFFER);
    wined3d_unordered_access_view_invalidate_location(&view_gl->v, ~WINED3D_LOCATION_BUFFER);

    get_buffer_view_range(&buffer_gl->b, &view_gl->v.desc, &format->f, &offset, &size);
    wined3d_context_gl_bind_bo(context_gl, buffer_gl->buffer_type_hint, buffer_gl->b.buffer_object);
    GL_EXTCALL(glClearBufferSubData(buffer_gl->buffer_type_hint, format->internal,
            offset, size, format->format, format->type, clear_value));
    checkGLcall("clear unordered access view");
}

void wined3d_unordered_access_view_set_counter(struct wined3d_unordered_access_view *view,
        unsigned int value)
{
    struct wined3d_unordered_access_view_gl *view_gl = wined3d_unordered_access_view_gl(view);
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;

    if (!view_gl->counter_bo)
        return;

    context = context_acquire(view_gl->v.resource->device, NULL, 0);
    gl_info = wined3d_context_gl(context)->gl_info;
    GL_EXTCALL(glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, view_gl->counter_bo));
    GL_EXTCALL(glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(value), &value));
    checkGLcall("set atomic counter");
    context_release(context);
}

void wined3d_unordered_access_view_copy_counter(struct wined3d_unordered_access_view *view,
        struct wined3d_buffer *buffer, unsigned int offset, struct wined3d_context *context)
{
    struct wined3d_unordered_access_view_gl *view_gl = wined3d_unordered_access_view_gl(view);
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    struct wined3d_bo_address dst, src;
    DWORD dst_location;

    if (!view_gl->counter_bo)
        return;

    dst_location = wined3d_buffer_get_memory(buffer, &dst, buffer->locations);
    dst.addr += offset;

    src.buffer_object = view_gl->counter_bo;
    src.addr = NULL;

    wined3d_context_gl_copy_bo_address(context_gl, &dst, wined3d_buffer_gl(buffer)->buffer_type_hint,
            &src, GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint));

    wined3d_buffer_invalidate_location(buffer, ~dst_location);
}

static void wined3d_unordered_access_view_gl_cs_init(void *object)
{
    struct wined3d_unordered_access_view_gl *view_gl = object;
    struct wined3d_resource *resource = view_gl->v.resource;
    struct wined3d_view_desc *desc = &view_gl->v.desc;
    const struct wined3d_gl_info *gl_info;

    gl_info = &resource->device->adapter->gl_info;

    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        struct wined3d_buffer *buffer = buffer_from_resource(resource);
        struct wined3d_context *context;

        context = context_acquire(resource->device, NULL, 0);
        gl_info = wined3d_context_gl(context)->gl_info;
        create_buffer_view(&view_gl->gl_view, context, desc, buffer, view_gl->v.format);
        if (desc->flags & (WINED3D_VIEW_BUFFER_COUNTER | WINED3D_VIEW_BUFFER_APPEND))
        {
            static const GLuint initial_value = 0;
            GL_EXTCALL(glGenBuffers(1, &view_gl->counter_bo));
            GL_EXTCALL(glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, view_gl->counter_bo));
            GL_EXTCALL(glBufferData(GL_ATOMIC_COUNTER_BUFFER,
                    sizeof(initial_value), &initial_value, GL_STATIC_DRAW));
            checkGLcall("create atomic counter buffer");
        }
        context_release(context);
    }
    else
    {
        struct wined3d_texture_gl *texture_gl = wined3d_texture_gl(texture_from_resource(resource));
        unsigned int depth_or_layer_count;

        if (resource->type == WINED3D_RTYPE_TEXTURE_3D)
            depth_or_layer_count = wined3d_texture_get_level_depth(&texture_gl->t, desc->u.texture.level_idx);
        else
            depth_or_layer_count = texture_gl->t.layer_count;

        if (desc->u.texture.layer_idx || desc->u.texture.layer_count != depth_or_layer_count)
        {
            create_texture_view(&view_gl->gl_view, get_texture_view_target(gl_info, desc, texture_gl),
                    desc, texture_gl, view_gl->v.format);
        }
    }
#if defined(STAGING_CSMT)

    wined3d_resource_release(resource);
#endif /* STAGING_CSMT */
}

static HRESULT wined3d_unordered_access_view_init(struct wined3d_unordered_access_view *view,
        const struct wined3d_view_desc *desc, struct wined3d_resource *resource,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    view->refcount = 1;
    view->parent = parent;
    view->parent_ops = parent_ops;

    if (!(resource->bind_flags & WINED3D_BIND_UNORDERED_ACCESS))
        return E_INVALIDARG;
    if (!(view->format = validate_resource_view(desc, resource, TRUE, FALSE)))
        return E_INVALIDARG;
    view->desc = *desc;

    wined3d_resource_incref(view->resource = resource);

#if defined(STAGING_CSMT)
    wined3d_resource_acquire(resource);
#endif /* STAGING_CSMT */

    return WINED3D_OK;
}

HRESULT wined3d_unordered_access_view_gl_init(struct wined3d_unordered_access_view_gl *view_gl,
        const struct wined3d_view_desc *desc, struct wined3d_resource *resource,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    HRESULT hr;

    TRACE("view_gl %p, desc %s, resource %p, parent %p, parent_ops %p.\n",
            view_gl, wined3d_debug_view_desc(desc, resource), resource, parent, parent_ops);

    if (FAILED(hr = wined3d_unordered_access_view_init(&view_gl->v, desc, resource, parent, parent_ops)))
        return hr;

    wined3d_cs_init_object(resource->device->cs, wined3d_unordered_access_view_gl_cs_init, view_gl);

    return hr;
}

HRESULT wined3d_unordered_access_view_vk_init(struct wined3d_unordered_access_view_vk *view_vk,
        const struct wined3d_view_desc *desc, struct wined3d_resource *resource,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    TRACE("view_vk %p, desc %s, resource %p, parent %p, parent_ops %p.\n",
            view_vk, wined3d_debug_view_desc(desc, resource), resource, parent, parent_ops);

    return wined3d_unordered_access_view_init(&view_vk->v, desc, resource, parent, parent_ops);
}

HRESULT CDECL wined3d_unordered_access_view_create(const struct wined3d_view_desc *desc,
        struct wined3d_resource *resource, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_unordered_access_view **view)
{
    const struct wined3d_adapter_ops *adapter_ops;

    TRACE("desc %s, resource %p, parent %p, parent_ops %p, view %p.\n",
            wined3d_debug_view_desc(desc, resource), resource, parent, parent_ops, view);

    adapter_ops = resource->device->adapter->adapter_ops;
    return adapter_ops->adapter_create_unordered_access_view(desc, resource, parent, parent_ops, view);
}
