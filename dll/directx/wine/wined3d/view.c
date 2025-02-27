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

#include "wined3d_private.h"
#include "wined3d_gl.h"
#include "wined3d_vk.h"

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
        {GL_TEXTURE_3D,        0, GL_TEXTURE_3D},

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
    unsigned int flags = desc->flags & (WINED3D_VIEW_BUFFER_RAW | WINED3D_VIEW_BUFFER_APPEND
            | WINED3D_VIEW_BUFFER_COUNTER | WINED3D_VIEW_TEXTURE_CUBE | WINED3D_VIEW_TEXTURE_ARRAY);
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(view_types); ++i)
    {
        if (view_types[i].texture_target != texture_gl->target || view_types[i].view_flags != flags)
            continue;
        if (gl_info->supported[view_types[i].extension])
            return view_types[i].view_target;

        FIXME("Extension %#x not supported.\n", view_types[i].extension);
    }

    FIXME("Unhandled view flags %#x for texture target %#x.\n", flags, texture_gl->target);
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
        if (!wined3d_bound_range(desc->u.buffer.start_idx, desc->u.buffer.count, buffer_size))
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
                || !wined3d_bound_range(desc->u.texture.level_idx, desc->u.texture.level_count, texture->level_count)
                || !desc->u.texture.layer_count
                || !wined3d_bound_range(desc->u.texture.layer_idx, desc->u.texture.layer_count, depth_or_layer_count))
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

static void create_buffer_texture(struct wined3d_gl_view *view, struct wined3d_context_gl *context_gl,
        struct wined3d_buffer *buffer, const struct wined3d_format_gl *view_format_gl,
        unsigned int offset, unsigned int size)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    const struct wined3d_bo_gl *bo_gl;

    if (!gl_info->supported[ARB_TEXTURE_BUFFER_OBJECT])
    {
        FIXME("OpenGL implementation does not support buffer textures.\n");
        return;
    }

    wined3d_buffer_load_location(buffer, &context_gl->c, WINED3D_LOCATION_BUFFER);
    bo_gl = wined3d_bo_gl(buffer->buffer_object);
    offset += bo_gl->b.buffer_offset;

    if ((offset & (gl_info->limits.texture_buffer_offset_alignment - 1)))
    {
        FIXME("Buffer offset %u is not %u byte aligned.\n",
                offset, gl_info->limits.texture_buffer_offset_alignment);
        return;
    }

    view->target = GL_TEXTURE_BUFFER;
    if (!view->name)
    {
        gl_info->gl_ops.gl.p_glGenTextures(1, &view->name);
    }

    wined3d_context_gl_bind_texture(context_gl, GL_TEXTURE_BUFFER, view->name);
    if (gl_info->supported[ARB_TEXTURE_BUFFER_RANGE])
    {
        GL_EXTCALL(glTexBufferRange(GL_TEXTURE_BUFFER, view_format_gl->internal, bo_gl->id, offset, size));
    }
    else
    {
        if (offset || size != buffer->resource.size)
            FIXME("OpenGL implementation does not support ARB_texture_buffer_range.\n");
        GL_EXTCALL(glTexBuffer(GL_TEXTURE_BUFFER, view_format_gl->internal, bo_gl->id));
    }
    checkGLcall("Create buffer texture");

    context_invalidate_compute_state(&context_gl->c, STATE_COMPUTE_SHADER_RESOURCE_BINDING);
    context_invalidate_state(&context_gl->c, STATE_GRAPHICS_SHADER_RESOURCE_BINDING);
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
    create_buffer_texture(view, wined3d_context_gl(context), buffer, wined3d_format_gl(view_format), offset, size);
}

static void wined3d_view_invalidate_location(struct wined3d_resource *resource,
        const struct wined3d_view_desc *desc, DWORD location)
{
    unsigned int i, sub_resource_idx;
    struct wined3d_texture *texture;

    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        wined3d_buffer_invalidate_location(buffer_from_resource(resource), location);
        return;
    }

    texture = texture_from_resource(resource);
    if (resource->type == WINED3D_RTYPE_TEXTURE_3D)
    {
        wined3d_texture_invalidate_location(texture, desc->u.texture.level_idx, location);
        return;
    }

    sub_resource_idx = desc->u.texture.layer_idx * texture->level_count + desc->u.texture.level_idx;
    for (i = 0; i < desc->u.texture.layer_count; ++i, sub_resource_idx += texture->level_count)
        wined3d_texture_invalidate_location(texture, sub_resource_idx, location);
}

static void wined3d_view_load_location(struct wined3d_resource *resource,
        const struct wined3d_view_desc *desc, struct wined3d_context *context, DWORD location)
{
    unsigned int i, sub_resource_idx;
    struct wined3d_texture *texture;

    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        wined3d_buffer_load_location(buffer_from_resource(resource), context, location);
        return;
    }

    texture = texture_from_resource(resource);
    if (resource->type == WINED3D_RTYPE_TEXTURE_3D)
    {
        wined3d_texture_load_location(texture, desc->u.texture.level_idx, context, location);
        return;
    }

    sub_resource_idx = desc->u.texture.layer_idx * texture->level_count + desc->u.texture.level_idx;
    for (i = 0; i < desc->u.texture.layer_count; ++i, sub_resource_idx += texture->level_count)
        wined3d_texture_load_location(texture, sub_resource_idx, context, location);
}

ULONG CDECL wined3d_rendertarget_view_incref(struct wined3d_rendertarget_view *view)
{
    unsigned int refcount = InterlockedIncrement(&view->refcount);

    TRACE("%p increasing refcount to %u.\n", view, refcount);

    return refcount;
}

void wined3d_rendertarget_view_cleanup(struct wined3d_rendertarget_view *view)
{
    view->parent_ops->wined3d_object_destroyed(view->parent);
}

ULONG CDECL wined3d_rendertarget_view_decref(struct wined3d_rendertarget_view *view)
{
    unsigned int refcount = InterlockedDecrement(&view->refcount);

    TRACE("%p decreasing refcount to %u.\n", view, refcount);

    if (!refcount)
    {
        struct wined3d_resource *resource = view->resource;

        /* Release the resource after destroying the view.
         * See wined3d_shader_resource_view_decref(). */
        wined3d_mutex_lock();
        resource->device->adapter->adapter_ops->adapter_destroy_rendertarget_view(view);
        wined3d_mutex_unlock();
        wined3d_resource_decref(resource);
    }

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

void CDECL wined3d_rendertarget_view_set_parent(struct wined3d_rendertarget_view *view,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    TRACE("view %p, parent %p, parent_ops %p.\n", view, parent, parent_ops);

    view->parent = parent;
    view->parent_ops = parent_ops;
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
    else
    {
        unsigned int level_idx = view->sub_resource_idx % texture->level_count;

        /* The drawable size of an FBO target is the OpenGL texture size,
         * which is the power of two size. */
        *width = wined3d_texture_get_level_width(texture, level_idx);
        *height = wined3d_texture_get_level_height(texture, level_idx);
    }
}

void wined3d_rendertarget_view_prepare_location(struct wined3d_rendertarget_view *view,
        struct wined3d_context *context, uint32_t location)
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
        struct wined3d_context *context, uint32_t location)
{
    wined3d_view_load_location(view->resource, &view->desc, context, location);
}

void wined3d_rendertarget_view_validate_location(struct wined3d_rendertarget_view *view, uint32_t location)
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

void wined3d_rendertarget_view_invalidate_location(struct wined3d_rendertarget_view *view, uint32_t location)
{
    wined3d_view_invalidate_location(view->resource, &view->desc, location);
}

/* Note: This may return 0 if the selected layers do not have a location in common. */
DWORD wined3d_rendertarget_view_get_locations(const struct wined3d_rendertarget_view *view)
{
    struct wined3d_resource *resource = view->resource;
    unsigned int i, sub_resource_idx, layer_count;
    const struct wined3d_texture *texture;
    DWORD ret = ~0u;

    if (resource->type == WINED3D_RTYPE_BUFFER)
        return buffer_from_resource(resource)->locations;

    texture = texture_from_resource(resource);
    sub_resource_idx = view->sub_resource_idx;
    layer_count = resource->type != WINED3D_RTYPE_TEXTURE_3D ? view->layer_count : 1;
    for (i = 0; i < layer_count; ++i, sub_resource_idx += texture->level_count)
        ret &= texture->sub_resources[sub_resource_idx].locations;

    if (!ret)
        WARN("View %p (texture %p) layers do not have a location in common.\n", view, texture);

    return ret;
}

void wined3d_rendertarget_view_get_box(struct wined3d_rendertarget_view *view,
        struct wined3d_box *box)
{
    if (view->resource->type != WINED3D_RTYPE_TEXTURE_3D)
    {
        wined3d_box_set(box, 0, 0, view->width, view->height, 0, 1);
    }
    else
    {
        struct wined3d_texture *texture = texture_from_resource(view->resource);
        wined3d_texture_get_level_box(texture, view->sub_resource_idx, box);
    }
}

static void wined3d_render_target_view_gl_cs_init(void *object)
{
    struct wined3d_rendertarget_view_gl *view_gl = object;
    struct wined3d_resource *resource = view_gl->v.resource;
    const struct wined3d_view_desc *desc = &view_gl->v.desc;

    TRACE("view_gl %p.\n", view_gl);

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
    view->format_attrs = view->format->attrs;
    view->format_caps = view->format->caps[resource->gl_type];
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

VkImageViewType vk_image_view_type_from_wined3d(enum wined3d_resource_type type, uint32_t flags)
{
    switch (type)
    {
        case WINED3D_RTYPE_TEXTURE_1D:
            if (flags & WINED3D_VIEW_TEXTURE_ARRAY)
                return VK_IMAGE_VIEW_TYPE_1D_ARRAY;
            else
                return VK_IMAGE_VIEW_TYPE_1D;

        case WINED3D_RTYPE_TEXTURE_2D:
            if (flags & WINED3D_VIEW_TEXTURE_CUBE)
            {
                if (flags & WINED3D_VIEW_TEXTURE_ARRAY)
                    return VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
                else
                    return VK_IMAGE_VIEW_TYPE_CUBE;
            }
            if (flags & WINED3D_VIEW_TEXTURE_ARRAY)
                return VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            else
                return VK_IMAGE_VIEW_TYPE_2D;

        case WINED3D_RTYPE_TEXTURE_3D:
            return VK_IMAGE_VIEW_TYPE_3D;

        default:
            ERR("Unhandled resource type %s.\n", debug_d3dresourcetype(type));
            return ~0u;
    }
}

static VkBufferView wined3d_view_vk_create_vk_buffer_view(struct wined3d_context_vk *context_vk,
        const struct wined3d_view_desc *desc, struct wined3d_buffer_vk *buffer_vk,
        const struct wined3d_format_vk *view_format_vk)
{
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    VkBufferViewCreateInfo create_info;
    struct wined3d_device_vk *device_vk;
    VkBufferView vk_buffer_view;
    unsigned int offset, size;
    struct wined3d_bo_vk *bo;
    VkResult vr;

    get_buffer_view_range(&buffer_vk->b, desc, &view_format_vk->f, &offset, &size);
    wined3d_buffer_prepare_location(&buffer_vk->b, &context_vk->c, WINED3D_LOCATION_BUFFER);
    bo = wined3d_bo_vk(buffer_vk->b.buffer_object);

    create_info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.buffer = bo->vk_buffer;
    create_info.format = view_format_vk->vk_format;
    create_info.offset = bo->b.buffer_offset + offset;
    create_info.range = size;

    device_vk = wined3d_device_vk(buffer_vk->b.resource.device);
    if ((vr = VK_CALL(vkCreateBufferView(device_vk->vk_device, &create_info, NULL, &vk_buffer_view))) < 0)
    {
        ERR("Failed to create buffer view, vr %s.\n", wined3d_debug_vkresult(vr));
        return VK_NULL_HANDLE;
    }

    return vk_buffer_view;
}

static VkImageView wined3d_view_vk_create_vk_image_view(struct wined3d_context_vk *context_vk,
        const struct wined3d_view_desc *desc, struct wined3d_texture_vk *texture_vk,
        const struct wined3d_format_vk *view_format_vk, struct color_fixup_desc fixup, bool rtv,
        VkImageUsageFlags usage)
{
    const struct wined3d_resource *resource = &texture_vk->t.resource;
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    const struct wined3d_format_vk *format_vk;
    VkImageViewUsageCreateInfoKHR usage_info;
    struct wined3d_device_vk *device_vk;
    VkImageViewCreateInfo create_info;
    VkImageView vk_image_view;
    VkResult vr;

    device_vk = wined3d_device_vk(resource->device);

    if (!wined3d_texture_vk_prepare_texture(texture_vk, context_vk))
    {
        ERR("Failed to prepare texture.\n");
        return VK_NULL_HANDLE;
    }

    /* Depth formats are a little complicated. For example, the typeless
     * format corresponding to depth/stencil view format WINED3DFMT_D32_FLOAT
     * is WINED3DFMT_R32_TYPELESS, and the corresponding shader resource view
     * format would be WINED3DFMT_R32_FLOAT. Vulkan depth/stencil formats are
     * only compatible with themselves, so it's not possible to create e.g. a
     * VK_FORMAT_R32_SFLOAT view on a VK_FORMAT_D32_SFLOAT image. In order to
     * make it work, we create Vulkan images for WINED3DFMT_R32_TYPELESS
     * resources with either a depth format (VK_FORMAT_D32_SFLOAT) or a colour
     * format, depending on whether the bind flags include
     * WINED3D_BIND_DEPTH_STENCIL or not. In order to then create a Vulkan
     * view on the image, we then replace the view format here with the
     * underlying resource format. However, that means it's still not possible
     * to create e.g. a WINED3DFMT_R32_UINT view on a WINED3DFMT_R32_TYPELESS
     * depth/stencil resource. */
    if (resource->bind_flags & WINED3D_BIND_DEPTH_STENCIL)
        format_vk = wined3d_format_vk(resource->format);
    else
        format_vk = view_format_vk;

    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    create_info.image = texture_vk->image.vk_image;
    create_info.viewType = vk_image_view_type_from_wined3d(resource->type, desc->flags);
    if (rtv && create_info.viewType == VK_IMAGE_VIEW_TYPE_3D)
    {
        if (desc->u.texture.layer_count > 1)
            create_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        else
            create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    }
    create_info.format = format_vk->vk_format;
    if (is_stencil_view_format(&view_format_vk->f))
    {
        create_info.components.r = VK_COMPONENT_SWIZZLE_ZERO;
        create_info.components.g = VK_COMPONENT_SWIZZLE_R;
        create_info.components.b = VK_COMPONENT_SWIZZLE_ZERO;
        create_info.components.a = VK_COMPONENT_SWIZZLE_ZERO;
    }
    else if (is_identity_fixup(fixup) || !can_use_texture_swizzle(context_vk->c.d3d_info, &format_vk->f))
    {
        create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    }
    else
    {
        wined3d_vk_swizzle_from_color_fixup(&create_info.components, fixup);
    }
    if ((resource->bind_flags & WINED3D_BIND_DEPTH_STENCIL)
            && (view_format_vk->f.red_size || view_format_vk->f.green_size))
    {
        create_info.subresourceRange.aspectMask = 0;
        if (view_format_vk->f.red_size)
            create_info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
        if (view_format_vk->f.green_size)
            create_info.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else
    {
        create_info.subresourceRange.aspectMask = vk_aspect_mask_from_format(&format_vk->f);
    }
    create_info.subresourceRange.baseMipLevel = desc->u.texture.level_idx;
    create_info.subresourceRange.levelCount = desc->u.texture.level_count;
    if (create_info.viewType == VK_IMAGE_VIEW_TYPE_3D)
    {
        if (desc->u.texture.layer_idx || (desc->u.texture.layer_count != texture_vk->t.resource.depth
                && desc->u.texture.layer_count != ~0u))
            WARN("Partial 3D texture views are not supported.\n");
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount = 1;
    }
    else
    {
        create_info.subresourceRange.baseArrayLayer = desc->u.texture.layer_idx;
        create_info.subresourceRange.layerCount = desc->u.texture.layer_count;
    }
    if (vk_info->supported[WINED3D_VK_KHR_MAINTENANCE2] || vk_info->api_version >= VK_API_VERSION_1_1)
    {
        usage_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO_KHR;
        usage_info.pNext = NULL;
        usage_info.usage = usage;
        create_info.pNext = &usage_info;
    }
    if ((vr = VK_CALL(vkCreateImageView(device_vk->vk_device, &create_info, NULL, &vk_image_view))) < 0)
    {
        ERR("Failed to create Vulkan image view, vr %s.\n", wined3d_debug_vkresult(vr));
        return VK_NULL_HANDLE;
    }

    return vk_image_view;
}

static void wined3d_render_target_view_vk_cs_init(void *object)
{
    struct wined3d_rendertarget_view_vk *view_vk = object;
    struct wined3d_view_desc *desc = &view_vk->v.desc;
    const struct wined3d_format_vk *format_vk;
    struct wined3d_texture_vk *texture_vk;
    struct wined3d_resource *resource;
    struct wined3d_context *context;
    VkImageUsageFlags vk_usage = 0;
    uint32_t default_flags = 0;

    TRACE("view_vk %p.\n", view_vk);

    resource = view_vk->v.resource;
    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        FIXME("Buffer views not implemented.\n");
        return;
    }

    texture_vk = wined3d_texture_vk(texture_from_resource(resource));
    format_vk = wined3d_format_vk(view_vk->v.format);

    if (texture_vk->t.layer_count > 1)
        default_flags |= WINED3D_VIEW_TEXTURE_ARRAY;

    if (resource->format->id == format_vk->f.id && desc->flags == default_flags
            && !desc->u.texture.level_idx && desc->u.texture.level_count == texture_vk->t.level_count
            && !desc->u.texture.layer_idx && desc->u.texture.layer_count == texture_vk->t.layer_count
            && !is_stencil_view_format(&format_vk->f) && resource->type != WINED3D_RTYPE_TEXTURE_3D
            && is_identity_fixup(format_vk->f.color_fixup))
    {
        TRACE("Creating identity render target view.\n");
        return;
    }

    if (texture_vk->t.swapchain && texture_vk->t.swapchain->state.desc.backbuffer_count > 1)
    {
        FIXME("Swapchain views not supported.\n");
        return;
    }

    if (resource->bind_flags & WINED3D_BIND_RENDER_TARGET)
        vk_usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (resource->bind_flags & WINED3D_BIND_DEPTH_STENCIL)
        vk_usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    context = context_acquire(resource->device, NULL, 0);
    view_vk->vk_image_view = wined3d_view_vk_create_vk_image_view(wined3d_context_vk(context),
            desc, texture_vk, format_vk, COLOR_FIXUP_IDENTITY, true, vk_usage);
    context_release(context);

    if (!view_vk->vk_image_view)
        return;

    TRACE("Created image view 0x%s.\n", wine_dbgstr_longlong(view_vk->vk_image_view));
}

HRESULT wined3d_rendertarget_view_vk_init(struct wined3d_rendertarget_view_vk *view_vk,
        const struct wined3d_view_desc *desc, struct wined3d_resource *resource,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    HRESULT hr;

    TRACE("view_vk %p, desc %s, resource %p, parent %p, parent_ops %p.\n",
            view_vk, wined3d_debug_view_desc(desc, resource), resource, parent, parent_ops);

    if (FAILED(hr = wined3d_rendertarget_view_init(&view_vk->v, desc, resource, parent, parent_ops)))
        return hr;

    wined3d_cs_init_object(resource->device->cs, wined3d_render_target_view_vk_cs_init, view_vk);

    return hr;
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
    unsigned int refcount;

    if (view->desc.flags & WINED3D_VIEW_FORWARD_REFERENCE)
        return wined3d_resource_incref(view->resource);

    refcount = InterlockedIncrement(&view->refcount);
    TRACE("%p increasing refcount to %u.\n", view, refcount);

    return refcount;
}

void wined3d_shader_resource_view_cleanup(struct wined3d_shader_resource_view *view)
{
    view->parent_ops->wined3d_object_destroyed(view->parent);
}

void wined3d_shader_resource_view_destroy(struct wined3d_shader_resource_view *view)
{
    wined3d_mutex_lock();
    view->resource->device->adapter->adapter_ops->adapter_destroy_shader_resource_view(view);
    wined3d_mutex_unlock();
}

ULONG CDECL wined3d_shader_resource_view_decref(struct wined3d_shader_resource_view *view)
{
    unsigned int refcount;

    if (view->desc.flags & WINED3D_VIEW_FORWARD_REFERENCE)
        return wined3d_resource_decref(view->resource);

    refcount = InterlockedDecrement(&view->refcount);
    TRACE("%p decreasing refcount to %u.\n", view, refcount);

    if (!refcount)
    {
        struct wined3d_resource *resource = view->resource;

        /* Release the resource after destroying the view:
         * - adapter_destroy_shader_resource_view() needs a reference to the
         *   device, which the resource implicitly provides.
         * - We shouldn't free buffer resources until after we've removed the
         *   view from its bo_user list. */
        wined3d_shader_resource_view_destroy(view);
        wined3d_resource_decref(resource);
    }

    return refcount;
}

void * CDECL wined3d_shader_resource_view_get_parent(const struct wined3d_shader_resource_view *view)
{
    TRACE("view %p.\n", view);

    return view->parent;
}

void wined3d_shader_resource_view_gl_update(struct wined3d_shader_resource_view_gl *srv_gl,
        struct wined3d_context_gl *context_gl)
{
    struct wined3d_buffer *buffer = buffer_from_resource(srv_gl->v.resource);

    assert(!srv_gl->bo_user.valid);

    create_buffer_view(&srv_gl->gl_view, &context_gl->c, &srv_gl->v.desc, buffer, srv_gl->v.format);
    srv_gl->bo_user.valid = true;
    list_add_head(&buffer->buffer_object->users, &srv_gl->bo_user.entry);
}

static void wined3d_shader_resource_view_gl_cs_init(void *object)
{
    struct wined3d_shader_resource_view_gl *view_gl = object;
    struct wined3d_resource *resource = view_gl->v.resource;
    const struct wined3d_format *view_format;
    const struct wined3d_gl_info *gl_info;
    const struct wined3d_view_desc *desc;
    GLenum view_target;

    TRACE("view_gl %p.\n", view_gl);

    view_format = view_gl->v.format;
    gl_info = &wined3d_adapter_gl(resource->device->adapter)->gl_info;
    desc = &view_gl->v.desc;

    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        struct wined3d_buffer *buffer = buffer_from_resource(resource);
        struct wined3d_context *context;

        context = context_acquire(resource->device, NULL, 0);
        create_buffer_view(&view_gl->gl_view, context, desc, buffer, view_format);
        view_gl->bo_user.valid = true;
        list_add_head(&buffer->buffer_object->users, &view_gl->bo_user.entry);
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

    /* If WINED3D_VIEW_FORWARD_REFERENCE, the view shouldn't take a reference
     * to the resource. However, the reference to the view returned by this
     * function should translate to a resource reference, so we increment the
     * resource's reference count anyway. */
    wined3d_resource_incref(view->resource = resource);

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

    list_init(&view_gl->bo_user.entry);
    wined3d_cs_init_object(resource->device->cs, wined3d_shader_resource_view_gl_cs_init, view_gl);

    return hr;
}

void wined3d_shader_resource_view_vk_update_buffer(struct wined3d_shader_resource_view_vk *srv_vk,
        struct wined3d_context_vk *context_vk)
{
    const struct wined3d_format_vk *view_format_vk = wined3d_format_vk(srv_vk->v.format);
    const struct wined3d_view_desc *desc = &srv_vk->v.desc;
    struct wined3d_resource *resource = srv_vk->v.resource;
    struct wined3d_view_vk *view_vk = &srv_vk->view_vk;
    struct wined3d_buffer_vk *buffer_vk;
    VkBufferView vk_buffer_view;

    assert(!view_vk->bo_user.valid);

    buffer_vk = wined3d_buffer_vk(buffer_from_resource(resource));
    wined3d_context_vk_destroy_vk_buffer_view(context_vk, view_vk->u.vk_buffer_view, view_vk->command_buffer_id);
    if ((vk_buffer_view = wined3d_view_vk_create_vk_buffer_view(context_vk, desc, buffer_vk, view_format_vk)))
    {
        view_vk->u.vk_buffer_view = vk_buffer_view;
        view_vk->bo_user.valid = true;
        list_add_head(&buffer_vk->b.buffer_object->users, &view_vk->bo_user.entry);
    }
}

void wined3d_shader_resource_view_vk_update_layout(struct wined3d_shader_resource_view_vk *srv_vk,
        VkImageLayout layout)
{
    srv_vk->view_vk.u.vk_image_info.imageLayout = layout;
}

static void wined3d_shader_resource_view_vk_cs_init(void *object)
{
    struct wined3d_shader_resource_view_vk *srv_vk = object;
    struct wined3d_view_desc *desc = &srv_vk->v.desc;
    struct wined3d_texture_vk *texture_vk;
    const struct wined3d_format *format;
    struct wined3d_buffer_vk *buffer_vk;
    struct wined3d_resource *resource;
    struct wined3d_context *context;
    VkBufferView vk_buffer_view;
    uint32_t default_flags = 0;
    VkImageView vk_image_view;
    struct wined3d_bo_vk *bo;

    TRACE("srv_vk %p.\n", srv_vk);

    resource = srv_vk->v.resource;
    format = srv_vk->v.format;

    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        buffer_vk = wined3d_buffer_vk(buffer_from_resource(resource));

        context = context_acquire(resource->device, NULL, 0);
        vk_buffer_view = wined3d_view_vk_create_vk_buffer_view(wined3d_context_vk(context),
                desc, buffer_vk, wined3d_format_vk(format));
        context_release(context);

        if (!vk_buffer_view)
            return;
        bo = wined3d_bo_vk(buffer_vk->b.buffer_object);

        TRACE("Created buffer view 0x%s.\n", wine_dbgstr_longlong(vk_buffer_view));

        srv_vk->view_vk.u.vk_buffer_view = vk_buffer_view;
        srv_vk->view_vk.bo_user.valid = true;
        list_add_head(&bo->b.users, &srv_vk->view_vk.bo_user.entry);

        return;
    }

    texture_vk = wined3d_texture_vk(texture_from_resource(resource));

    if (texture_vk->t.layer_count > 1)
        default_flags |= WINED3D_VIEW_TEXTURE_ARRAY;

    if (resource->format->id == format->id && desc->flags == default_flags
            && !desc->u.texture.level_idx && desc->u.texture.level_count == texture_vk->t.level_count
            && !desc->u.texture.layer_idx && desc->u.texture.layer_count == texture_vk->t.layer_count
            && !(resource->bind_flags & WINED3D_BIND_DEPTH_STENCIL))
    {
        TRACE("Creating identity shader resource view.\n");
        return;
    }

    if (texture_vk->t.swapchain && texture_vk->t.swapchain->state.desc.backbuffer_count > 1)
        FIXME("Swapchain shader resource views not supported.\n");

    context = context_acquire(resource->device, NULL, 0);
    vk_image_view = wined3d_view_vk_create_vk_image_view(wined3d_context_vk(context),
            desc, texture_vk, wined3d_format_vk(format), format->color_fixup, false, VK_IMAGE_USAGE_SAMPLED_BIT);
    context_release(context);

    if (!vk_image_view)
        return;

    TRACE("Created image view 0x%s.\n", wine_dbgstr_longlong(vk_image_view));

    srv_vk->view_vk.u.vk_image_info.imageView = vk_image_view;
    srv_vk->view_vk.u.vk_image_info.sampler = VK_NULL_HANDLE;
    if (texture_vk->layout == VK_IMAGE_LAYOUT_GENERAL)
        srv_vk->view_vk.u.vk_image_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    else
        srv_vk->view_vk.u.vk_image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

HRESULT wined3d_shader_resource_view_vk_init(struct wined3d_shader_resource_view_vk *view_vk,
        const struct wined3d_view_desc *desc, struct wined3d_resource *resource,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    HRESULT hr;

    TRACE("view_vk %p, desc %s, resource %p, parent %p, parent_ops %p.\n",
            view_vk, wined3d_debug_view_desc(desc, resource), resource, parent, parent_ops);

    if (FAILED(hr = wined3d_shader_resource_view_init(&view_vk->v, desc, resource, parent, parent_ops)))
        return hr;

    list_init(&view_vk->view_vk.bo_user.entry);
    wined3d_cs_init_object(resource->device->cs, wined3d_shader_resource_view_vk_cs_init, view_vk);

    return hr;
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
    wined3d_texture_gl_bind(texture_gl, context_gl, sampler_gl->s.desc.srgb_decode);
    wined3d_sampler_gl_bind(sampler_gl, unit, texture_gl, context_gl);
}

/* Context activation is done by the caller. */
static void shader_resource_view_gl_bind_and_dirtify(struct wined3d_shader_resource_view_gl *view_gl,
        struct wined3d_context_gl *context_gl)
{
    /* FIXME: Ideally we'd only do this when touching a binding that's used by
     * a shader. */
    context_invalidate_compute_state(&context_gl->c, STATE_COMPUTE_SHADER_RESOURCE_BINDING);
    context_invalidate_state(&context_gl->c, STATE_GRAPHICS_SHADER_RESOURCE_BINDING);

    wined3d_context_gl_bind_texture(context_gl, view_gl->gl_view.target, view_gl->gl_view.name);
}

void wined3d_shader_resource_view_gl_generate_mipmap(struct wined3d_shader_resource_view_gl *view_gl,
        struct wined3d_context_gl *context_gl)
{
    unsigned int i, j, layer_count, level_count, base_level, base_layer, sub_resource_idx;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_texture_gl *texture_gl;
    struct gl_texture *gl_tex;
    DWORD location;
    BOOL srgb;

    TRACE("view_gl %p.\n", view_gl);

    layer_count = view_gl->v.desc.u.texture.layer_count;
    level_count = view_gl->v.desc.u.texture.level_count;
    base_level = view_gl->v.desc.u.texture.level_idx;
    base_layer = view_gl->v.desc.u.texture.layer_idx;

    texture_gl = wined3d_texture_gl(texture_from_resource(view_gl->v.resource));
    srgb = !!(texture_gl->t.flags & WINED3D_TEXTURE_IS_SRGB);
    location = srgb ? WINED3D_LOCATION_TEXTURE_SRGB : WINED3D_LOCATION_TEXTURE_RGB;
    for (i = 0; i < layer_count; ++i)
    {
        sub_resource_idx = (base_layer + i) * texture_gl->t.level_count + base_level;
        if (!wined3d_texture_load_location(&texture_gl->t, sub_resource_idx, &context_gl->c, location))
            ERR("Failed to load source layer %u.\n", base_layer + i);
    }

    if (view_gl->gl_view.name)
    {
        shader_resource_view_gl_bind_and_dirtify(view_gl, context_gl);
    }
    else
    {
        wined3d_texture_gl_bind_and_dirtify(texture_gl, context_gl, srgb);
        gl_info->gl_ops.gl.p_glTexParameteri(texture_gl->target, GL_TEXTURE_BASE_LEVEL, base_level);
        gl_info->gl_ops.gl.p_glTexParameteri(texture_gl->target, GL_TEXTURE_MAX_LEVEL, base_level + level_count - 1);
    }

    if (gl_info->supported[ARB_SAMPLER_OBJECTS])
        GL_EXTCALL(glBindSampler(context_gl->active_texture, 0));
    gl_tex = wined3d_texture_gl_get_gl_texture(texture_gl, srgb);
    if (context_gl->c.d3d_info->wined3d_creation_flags & WINED3D_SRGB_READ_WRITE_CONTROL
            && gl_info->supported[EXT_TEXTURE_SRGB_DECODE])
    {
        gl_info->gl_ops.gl.p_glTexParameteri(texture_gl->target,
                GL_TEXTURE_SRGB_DECODE_EXT, GL_SKIP_DECODE_EXT);
        gl_tex->sampler_desc.srgb_decode = FALSE;
    }

    gl_info->fbo_ops.glGenerateMipmap(texture_gl->target);
    checkGLcall("glGenerateMipMap()");

    for (i = 0; i < layer_count; ++i)
    {
        for (j = 1; j < level_count; ++j)
        {
            sub_resource_idx = (base_layer + i) * texture_gl->t.level_count + base_level + j;
            wined3d_texture_validate_location(&texture_gl->t, sub_resource_idx, location);
            wined3d_texture_invalidate_location(&texture_gl->t, sub_resource_idx, ~location);
        }
    }

    if (!view_gl->gl_view.name)
    {
        gl_tex->sampler_desc.mip_base_level = base_level;
        gl_info->gl_ops.gl.p_glTexParameteri(texture_gl->target,
                GL_TEXTURE_MAX_LEVEL, texture_gl->t.level_count - 1);
    }
}

void wined3d_shader_resource_view_vk_generate_mipmap(struct wined3d_shader_resource_view_vk *srv_vk,
        struct wined3d_context_vk *context_vk)
{
    unsigned int i, j, layer_count, level_count, base_level, base_layer, sub_resource_idx;
    const struct wined3d_vk_info *vk_info = context_vk->vk_info;
    VkImageSubresourceRange vk_src_range, vk_dst_range;
    struct wined3d_texture_vk *texture_vk;
    VkCommandBuffer vk_command_buffer;
    VkImageBlit region;

    TRACE("srv_vk %p.\n", srv_vk);

    layer_count = srv_vk->v.desc.u.texture.layer_count;
    level_count = srv_vk->v.desc.u.texture.level_count;
    base_level = srv_vk->v.desc.u.texture.level_idx;
    base_layer = srv_vk->v.desc.u.texture.layer_idx;

    texture_vk = wined3d_texture_vk(texture_from_resource(srv_vk->v.resource));
    for (i = 0; i < layer_count; ++i)
    {
        sub_resource_idx = (base_layer + i) * texture_vk->t.level_count + base_level;
        if (!wined3d_texture_load_location(&texture_vk->t, sub_resource_idx,
                &context_vk->c, WINED3D_LOCATION_TEXTURE_RGB))
            ERR("Failed to load source layer %u.\n", base_layer + i);
    }

    if (context_vk->c.d3d_info->wined3d_creation_flags & WINED3D_SRGB_READ_WRITE_CONTROL)
        FIXME("Unhandled sRGB read/write control.\n");

    if (wined3d_format_vk(srv_vk->v.format)->vk_format != wined3d_format_vk(texture_vk->t.resource.format)->vk_format)
        FIXME("Ignoring view format %s.\n", debug_d3dformat(srv_vk->v.format->id));

    if (wined3d_resource_get_sample_count(&texture_vk->t.resource) > 1)
        FIXME("Unhandled multi-sampled resource.\n");

    if (!(vk_command_buffer = wined3d_context_vk_get_command_buffer(context_vk)))
    {
        ERR("Failed to get command buffer.\n");
        return;
    }

    vk_src_range.aspectMask = vk_aspect_mask_from_format(texture_vk->t.resource.format);
    vk_src_range.baseMipLevel = base_level;
    vk_src_range.levelCount = 1;
    vk_src_range.baseArrayLayer = base_layer;
    vk_src_range.layerCount = layer_count;

    vk_dst_range = vk_src_range;
    ++vk_dst_range.baseMipLevel;

    wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            vk_access_mask_from_bind_flags(texture_vk->t.resource.bind_flags),
            VK_ACCESS_TRANSFER_READ_BIT,
            texture_vk->layout, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            texture_vk->image.vk_image, &vk_src_range);
    wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            vk_access_mask_from_bind_flags(texture_vk->t.resource.bind_flags),
            VK_ACCESS_TRANSFER_WRITE_BIT,
            texture_vk->layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            texture_vk->image.vk_image, &vk_dst_range);

    region.srcSubresource.aspectMask = vk_src_range.aspectMask;
    region.srcSubresource.mipLevel = vk_src_range.baseMipLevel;
    region.srcSubresource.baseArrayLayer = vk_src_range.baseArrayLayer;
    region.srcSubresource.layerCount = vk_src_range.layerCount;
    region.srcOffsets[0].x = 0;
    region.srcOffsets[0].y = 0;
    region.srcOffsets[0].z = 0;

    region.dstSubresource.aspectMask = vk_dst_range.aspectMask;
    region.dstSubresource.mipLevel = vk_dst_range.baseMipLevel;
    region.dstSubresource.baseArrayLayer = vk_dst_range.baseArrayLayer;
    region.dstSubresource.layerCount = vk_dst_range.layerCount;
    region.dstOffsets[0].x = 0;
    region.dstOffsets[0].y = 0;
    region.dstOffsets[0].z = 0;

    for (i = 1; i < level_count; ++i)
    {
        region.srcOffsets[1].x = wined3d_texture_get_level_width(&texture_vk->t, vk_src_range.baseMipLevel);
        region.srcOffsets[1].y = wined3d_texture_get_level_height(&texture_vk->t, vk_src_range.baseMipLevel);
        region.srcOffsets[1].z = wined3d_texture_get_level_depth(&texture_vk->t, vk_src_range.baseMipLevel);

        region.dstOffsets[1].x = wined3d_texture_get_level_width(&texture_vk->t, vk_dst_range.baseMipLevel);
        region.dstOffsets[1].y = wined3d_texture_get_level_height(&texture_vk->t, vk_dst_range.baseMipLevel);
        region.dstOffsets[1].z = wined3d_texture_get_level_depth(&texture_vk->t, vk_dst_range.baseMipLevel);

        VK_CALL(vkCmdBlitImage(vk_command_buffer, texture_vk->image.vk_image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                texture_vk->image.vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR));

        wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VK_ACCESS_TRANSFER_READ_BIT,
                vk_access_mask_from_bind_flags(texture_vk->t.resource.bind_flags),
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texture_vk->layout,
                texture_vk->image.vk_image, &vk_src_range);

        if (i == level_count - 1)
        {
            wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                    VK_ACCESS_TRANSFER_WRITE_BIT,
                    vk_access_mask_from_bind_flags(texture_vk->t.resource.bind_flags),
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texture_vk->layout,
                    texture_vk->image.vk_image, &vk_dst_range);
        }
        else
        {
            region.srcSubresource.mipLevel = ++vk_src_range.baseMipLevel;
            region.dstSubresource.mipLevel = ++vk_dst_range.baseMipLevel;

            wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    texture_vk->image.vk_image, &vk_src_range);
            wined3d_context_vk_image_barrier(context_vk, vk_command_buffer,
                    VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                    vk_access_mask_from_bind_flags(texture_vk->t.resource.bind_flags),
                    VK_ACCESS_TRANSFER_WRITE_BIT,
                    texture_vk->layout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    texture_vk->image.vk_image, &vk_dst_range);
        }
    }

    for (i = 0; i < layer_count; ++i)
    {
        for (j = 1; j < level_count; ++j)
        {
            sub_resource_idx = (base_layer + i) * texture_vk->t.level_count + base_level + j;
            wined3d_texture_validate_location(&texture_vk->t, sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB);
            wined3d_texture_invalidate_location(&texture_vk->t, sub_resource_idx, ~WINED3D_LOCATION_TEXTURE_RGB);
        }
    }

    wined3d_context_vk_reference_texture(context_vk, texture_vk);
}

ULONG CDECL wined3d_unordered_access_view_incref(struct wined3d_unordered_access_view *view)
{
    unsigned int refcount = InterlockedIncrement(&view->refcount);

    TRACE("%p increasing refcount to %u.\n", view, refcount);

    return refcount;
}

void wined3d_unordered_access_view_cleanup(struct wined3d_unordered_access_view *view)
{
    view->parent_ops->wined3d_object_destroyed(view->parent);
}

ULONG CDECL wined3d_unordered_access_view_decref(struct wined3d_unordered_access_view *view)
{
    unsigned int refcount = InterlockedDecrement(&view->refcount);

    TRACE("%p decreasing refcount to %u.\n", view, refcount);

    if (!refcount)
    {
        struct wined3d_resource *resource = view->resource;

        /* Release the resource after destroying the view.
         * See wined3d_shader_resource_view_decref(). */
        wined3d_mutex_lock();
        resource->device->adapter->adapter_ops->adapter_destroy_unordered_access_view(view);
        wined3d_mutex_unlock();
        wined3d_resource_decref(resource);
    }

    return refcount;
}

void * CDECL wined3d_unordered_access_view_get_parent(const struct wined3d_unordered_access_view *view)
{
    TRACE("view %p.\n", view);

    return view->parent;
}

void wined3d_unordered_access_view_invalidate_location(struct wined3d_unordered_access_view *view,
        uint32_t location)
{
    wined3d_view_invalidate_location(view->resource, &view->desc, location);
}

void wined3d_unordered_access_view_gl_clear(struct wined3d_unordered_access_view_gl *view_gl,
        const struct wined3d_uvec4 *clear_value, struct wined3d_context_gl *context_gl, bool fp)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    const struct wined3d_format_gl *format_gl;
    struct wined3d_resource *resource;
    struct wined3d_buffer *buffer;
    struct wined3d_bo_gl *bo_gl;
    unsigned int offset, size;

    resource = view_gl->v.resource;
    if (resource->type != WINED3D_RTYPE_BUFFER)
    {
        unsigned int layer_count, level_count, base_level, base_layer;
        unsigned int sub_resource_idx, width, height, depth, i, j;
        struct wined3d_texture_gl *texture_gl;
        const void *data = clear_value;
        GLenum gl_format, gl_type;
        uint32_t packed;

        if (!gl_info->supported[ARB_CLEAR_TEXTURE])
        {
            FIXME("OpenGL implementation does not support ARB_clear_texture.\n");
            return;
        }

        format_gl = wined3d_format_gl(resource->format);
        texture_gl = wined3d_texture_gl(texture_from_resource(resource));
        level_count = view_gl->v.desc.u.texture.level_count;
        base_level = view_gl->v.desc.u.texture.level_idx;

        if (resource->type == WINED3D_RTYPE_TEXTURE_3D)
        {
            layer_count = 1;
            base_layer = 0;
        }
        else
        {
            layer_count = view_gl->v.desc.u.texture.layer_count;
            base_layer = view_gl->v.desc.u.texture.layer_idx;
        }

        if (format_gl->f.byte_count <= 4 && !fp)
        {
            gl_format = format_gl->format;
            gl_type = format_gl->type;
            packed = wined3d_format_pack(&format_gl->f, clear_value);
            data = &packed;
        }
        else if (resource->format_attrs & WINED3D_FORMAT_ATTR_INTEGER)
        {
            gl_format = GL_RGBA_INTEGER;
            gl_type = GL_UNSIGNED_INT;
        }
        else
        {
            gl_format = GL_RGBA;
            gl_type = GL_FLOAT;
        }

        for (i = 0; i < layer_count; ++i)
        {
            for (j = 0; j < level_count; ++j)
            {
                sub_resource_idx = (base_layer + i) * texture_gl->t.level_count + base_level + j;
                wined3d_texture_prepare_location(&texture_gl->t, sub_resource_idx,
                        &context_gl->c, WINED3D_LOCATION_TEXTURE_RGB);

                width = wined3d_texture_get_level_width(&texture_gl->t, base_level + j);
                height = wined3d_texture_get_level_height(&texture_gl->t, base_level + j);
                depth = wined3d_texture_get_level_depth(&texture_gl->t, base_level + j);

                switch (texture_gl->target)
                {
                    case GL_TEXTURE_1D_ARRAY:
                        GL_EXTCALL(glClearTexSubImage(texture_gl->texture_rgb.name, base_level + j,
                                0, base_layer + i, 0, width, 1, 1, gl_format, gl_type, data));
                        break;

                    case GL_TEXTURE_2D_ARRAY:
                    case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
                    case GL_TEXTURE_CUBE_MAP:
                    case GL_TEXTURE_CUBE_MAP_ARRAY:
                        GL_EXTCALL(glClearTexSubImage(texture_gl->texture_rgb.name, base_level + j,
                                0, 0, base_layer + i, width, height, 1, gl_format, gl_type, data));
                        break;

                    default:
                        GL_EXTCALL(glClearTexSubImage(texture_gl->texture_rgb.name, base_level + j,
                                0, 0, 0, width, height, depth, gl_format, gl_type, data));
                        break;
                }

                wined3d_texture_validate_location(&texture_gl->t, sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB);
                wined3d_texture_invalidate_location(&texture_gl->t, sub_resource_idx, ~WINED3D_LOCATION_TEXTURE_RGB);
            }
        }

        return;
    }

    if (!gl_info->supported[ARB_CLEAR_BUFFER_OBJECT])
    {
        FIXME("OpenGL implementation does not support ARB_clear_buffer_object.\n");
        return;
    }

    format_gl = wined3d_format_gl(view_gl->v.format);
    if (format_gl->f.id != WINED3DFMT_R32_UINT && format_gl->f.id != WINED3DFMT_R32_SINT
            && format_gl->f.id != WINED3DFMT_R32G32B32A32_UINT
            && format_gl->f.id != WINED3DFMT_R32G32B32A32_SINT)
    {
        FIXME("Not implemented for format %s.\n", debug_d3dformat(format_gl->f.id));
        return;
    }

    if (fp)
    {
        FIXME("Floating-point buffer clears not implemented.\n");
        return;
    }

    buffer = buffer_from_resource(resource);
    get_buffer_view_range(buffer, &view_gl->v.desc, &format_gl->f, &offset, &size);

    if (!offset && size == buffer->resource.size)
    {
        wined3d_buffer_prepare_location(buffer, &context_gl->c, WINED3D_LOCATION_BUFFER);
    }
    else
    {
        wined3d_buffer_acquire_bo_for_write(buffer, &context_gl->c);
        wined3d_buffer_load_location(buffer, &context_gl->c, WINED3D_LOCATION_BUFFER);
    }
    wined3d_unordered_access_view_invalidate_location(&view_gl->v, ~WINED3D_LOCATION_BUFFER);

    bo_gl = wined3d_bo_gl(buffer->buffer_object);
    wined3d_context_gl_bind_bo(context_gl, bo_gl->binding, bo_gl->id);
    GL_EXTCALL(glClearBufferSubData(bo_gl->binding, format_gl->internal,
            bo_gl->b.buffer_offset + offset, size, format_gl->format, format_gl->type, clear_value));
    wined3d_context_gl_reference_bo(context_gl, bo_gl);
    checkGLcall("clear unordered access view");
}

void wined3d_unordered_access_view_set_counter(struct wined3d_unordered_access_view *view,
        unsigned int value)
{
    struct wined3d_bo_address dst, src;
    struct wined3d_context *context;
    struct wined3d_range range;

    if (!view->counter_bo)
        return;

    context = context_acquire(view->resource->device, NULL, 0);

    src.buffer_object = 0;
    src.addr = (void *)&value;

    dst.buffer_object = view->counter_bo;
    dst.addr = NULL;

    range.offset = 0;
    range.size = sizeof(value);
    wined3d_context_copy_bo_address(context, &dst, &src, 1, &range, WINED3D_MAP_WRITE | WINED3D_MAP_DISCARD);

    context_release(context);
}

void wined3d_unordered_access_view_copy_counter(struct wined3d_unordered_access_view *view,
        struct wined3d_buffer *buffer, unsigned int offset, struct wined3d_context *context)
{
    struct wined3d_const_bo_address src;

    if (!view->counter_bo)
        return;

    src.buffer_object = view->counter_bo;
    src.addr = NULL;

    wined3d_buffer_copy_bo_address(buffer, context, offset, &src, sizeof(uint32_t));
}

void wined3d_unordered_access_view_gl_update(struct wined3d_unordered_access_view_gl *uav_gl,
        struct wined3d_context_gl *context_gl)
{
    struct wined3d_buffer *buffer = buffer_from_resource(uav_gl->v.resource);

    assert(!uav_gl->bo_user.valid);
    create_buffer_view(&uav_gl->gl_view, &context_gl->c, &uav_gl->v.desc, buffer, uav_gl->v.format);
    uav_gl->bo_user.valid = true;
    list_add_head(&buffer->buffer_object->users, &uav_gl->bo_user.entry);
}

static void wined3d_unordered_access_view_gl_cs_init(void *object)
{
    struct wined3d_unordered_access_view_gl *view_gl = object;
    struct wined3d_resource *resource = view_gl->v.resource;
    struct wined3d_view_desc *desc = &view_gl->v.desc;
    const struct wined3d_gl_info *gl_info;

    TRACE("view_gl %p.\n", view_gl);

    gl_info = &wined3d_adapter_gl(resource->device->adapter)->gl_info;

    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        struct wined3d_device_gl *device_gl = wined3d_device_gl(resource->device);
        struct wined3d_buffer *buffer = buffer_from_resource(resource);
        struct wined3d_context_gl *context_gl;

        context_gl = wined3d_context_gl(context_acquire(&device_gl->d, NULL, 0));
        create_buffer_view(&view_gl->gl_view, &context_gl->c, desc, buffer, view_gl->v.format);
        view_gl->bo_user.valid = true;
        list_add_head(&buffer->buffer_object->users, &view_gl->bo_user.entry);
        if (desc->flags & (WINED3D_VIEW_BUFFER_COUNTER | WINED3D_VIEW_BUFFER_APPEND))
        {
            struct wined3d_bo_gl *bo = &view_gl->counter_bo;

            view_gl->v.counter_bo = &bo->b;
            wined3d_device_gl_create_bo(device_gl, context_gl, sizeof(uint32_t), GL_ATOMIC_COUNTER_BUFFER,
                    GL_STATIC_DRAW, true, GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_CLIENT_STORAGE_BIT, bo);
            wined3d_unordered_access_view_set_counter(&view_gl->v, 0);
        }
        context_release(&context_gl->c);
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

    list_init(&view_gl->bo_user.entry);
    wined3d_cs_init_object(resource->device->cs, wined3d_unordered_access_view_gl_cs_init, view_gl);

    return hr;
}

static int compile_hlsl_cs(const struct vkd3d_shader_code *hlsl, struct vkd3d_shader_code *dxbc)
{
    struct vkd3d_shader_hlsl_source_info hlsl_info;
    struct vkd3d_shader_compile_info info;

    static const struct vkd3d_shader_compile_option options[] =
    {
        {VKD3D_SHADER_COMPILE_OPTION_API_VERSION, VKD3D_SHADER_API_VERSION_1_12},
    };

    info.type = VKD3D_SHADER_STRUCTURE_TYPE_COMPILE_INFO;
    info.next = &hlsl_info;
    info.source = *hlsl;
    info.source_type = VKD3D_SHADER_SOURCE_HLSL;
    info.target_type = VKD3D_SHADER_TARGET_DXBC_TPF;
    info.options = options;
    info.option_count = ARRAY_SIZE(options);
    info.log_level = VKD3D_SHADER_LOG_NONE;
    info.source_name = NULL;

    hlsl_info.type = VKD3D_SHADER_STRUCTURE_TYPE_HLSL_SOURCE_INFO;
    hlsl_info.next = NULL;
    hlsl_info.entry_point = "main";
    hlsl_info.secondary_code.code = NULL;
    hlsl_info.secondary_code.size = 0;
    hlsl_info.profile = "cs_5_0";

    return vkd3d_shader_compile(&info, dxbc, NULL);
}

struct wined3d_uav_clear_constants_vk
{
    VkClearColorValue color;
    VkOffset2D offset;
    VkExtent2D extent;
};

static VkPipeline create_uav_pipeline(struct wined3d_context_vk *context_vk,
        struct wined3d_pipeline_layout_vk *layout, const char *resource_filename,
        enum wined3d_shader_resource_type resource_type)
{
    VkComputePipelineCreateInfo pipeline_info;
    struct wined3d_shader_desc shader_desc;
    const struct wined3d_vk_info *vk_info;
    struct vkd3d_shader_code code, dxbc;
    struct wined3d_context *context;
    VkShaderModule shader_module;
    VkDevice vk_device;
    void *resource_ptr;
    VkPipeline result;
    HGLOBAL global;
    HMODULE module;
    HRSRC resource;
    VkResult vr;
    int ret;

    vk_info = context_vk->vk_info;
    context = &context_vk->c;

    if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (const char *)create_uav_pipeline, &module))
    {
        ERR("Failed to get a reference to the current module, last error %ld.\n", GetLastError());
        return VK_NULL_HANDLE;
    }

    if (!(resource = FindResourceA(module, resource_filename, (const char *)RT_RCDATA)))
    {
        ERR("Failed to retrieve resource, last error %ld.\n", GetLastError());
        return VK_NULL_HANDLE;
    }

    if (!(global = LoadResource(module, resource)))
    {
        ERR("Failed to load resource, last error %ld.\n", GetLastError());
        return VK_NULL_HANDLE;
    }

    if (!(resource_ptr = LockResource(global)))
    {
        ERR("Failed to lock resource.\n");
        FreeResource(resource);
        return VK_NULL_HANDLE;
    }

    code.code = resource_ptr;
    code.size = SizeofResource(module, resource);

    if ((ret = compile_hlsl_cs(&code, &dxbc)) < 0)
    {
        ERR("Failed to compile shader, ret %d.\n", ret);
        FreeResource(resource);
        return VK_NULL_HANDLE;
    }

    if (FreeResource(resource))
        ERR("Failed to free resource.\n");

    shader_desc.byte_code = dxbc.code;
    shader_desc.byte_code_size = dxbc.size;

    shader_module = (VkShaderModule)context->device->adapter->shader_backend->shader_compile(context, &shader_desc,
            WINED3D_SHADER_TYPE_COMPUTE);
    if (shader_module == VK_NULL_HANDLE)
    {
        ERR("Failed to create shader.\n");
        vkd3d_shader_free_shader_code(&dxbc);
        return VK_NULL_HANDLE;
    }

    vkd3d_shader_free_shader_code(&dxbc);

    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.pNext = NULL;
    pipeline_info.flags = 0;
    pipeline_info.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pipeline_info.stage.pNext = NULL;
    pipeline_info.stage.flags = 0;
    pipeline_info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pipeline_info.stage.pName = "main";
    pipeline_info.stage.pSpecializationInfo = NULL;
    pipeline_info.stage.module = shader_module;
    pipeline_info.layout = layout->vk_pipeline_layout;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;

    vk_device = wined3d_device_vk(context->device)->vk_device;

    if ((vr = VK_CALL(vkCreateComputePipelines(vk_device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &result))) < 0)
    {
        ERR("Failed to create Vulkan compute pipeline, vr %s.\n", wined3d_debug_vkresult(vr));
        return VK_NULL_HANDLE;
    }

    VK_CALL(vkDestroyShaderModule(vk_device, shader_module, NULL));
    return result;
}

void wined3d_device_vk_uav_clear_state_init(struct wined3d_device_vk *device_vk)
{
    struct wined3d_uav_clear_state_vk *state = &device_vk->uav_clear_state;
    struct wined3d_context_vk *context_vk = &device_vk->context_vk;
    VkDescriptorSetLayoutBinding vk_set_bindings[2];

    vk_set_bindings[0].binding = 0;
    vk_set_bindings[0].descriptorCount = 1;
    vk_set_bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    vk_set_bindings[0].pImmutableSamplers = NULL;
    vk_set_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

    vk_set_bindings[1].binding = 1;
    vk_set_bindings[1].descriptorCount = 1;
    vk_set_bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    vk_set_bindings[1].pImmutableSamplers = NULL;

    vk_set_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    state->image_layout = wined3d_context_vk_get_pipeline_layout(context_vk, vk_set_bindings, 2);

    vk_set_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    state->buffer_layout = wined3d_context_vk_get_pipeline_layout(context_vk, vk_set_bindings, 2);

    state->float_pipelines.buffer = create_uav_pipeline(context_vk, state->buffer_layout,
            "cs_uav_clear_buffer_float_code.hlsl", WINED3D_SHADER_RESOURCE_BUFFER);
    state->uint_pipelines.buffer = create_uav_pipeline(context_vk, state->buffer_layout,
            "cs_uav_clear_buffer_uint_code.hlsl", WINED3D_SHADER_RESOURCE_BUFFER);
    state->float_pipelines.image_1d = create_uav_pipeline(context_vk, state->image_layout,
            "cs_uav_clear_1d_float_code.hlsl", WINED3D_SHADER_RESOURCE_TEXTURE_1D);
    state->uint_pipelines.image_1d = create_uav_pipeline(context_vk, state->image_layout,
            "cs_uav_clear_1d_uint_code.hlsl", WINED3D_SHADER_RESOURCE_TEXTURE_1D);
    state->float_pipelines.image_1d_array = create_uav_pipeline(context_vk, state->image_layout,
            "cs_uav_clear_1d_array_float_code.hlsl", WINED3D_SHADER_RESOURCE_TEXTURE_1DARRAY);
    state->uint_pipelines.image_1d_array = create_uav_pipeline(context_vk, state->image_layout,
            "cs_uav_clear_1d_array_uint_code.hlsl", WINED3D_SHADER_RESOURCE_TEXTURE_1DARRAY);
    state->float_pipelines.image_2d = create_uav_pipeline(context_vk, state->image_layout,
            "cs_uav_clear_2d_float_code.hlsl", WINED3D_SHADER_RESOURCE_TEXTURE_2D);
    state->uint_pipelines.image_2d = create_uav_pipeline(context_vk, state->image_layout,
            "cs_uav_clear_2d_uint_code.hlsl", WINED3D_SHADER_RESOURCE_TEXTURE_2D);
    state->float_pipelines.image_2d_array = create_uav_pipeline(context_vk, state->image_layout,
            "cs_uav_clear_2d_array_float_code.hlsl", WINED3D_SHADER_RESOURCE_TEXTURE_2DARRAY);
    state->uint_pipelines.image_2d_array = create_uav_pipeline(context_vk, state->image_layout,
            "cs_uav_clear_2d_array_uint_code.hlsl", WINED3D_SHADER_RESOURCE_TEXTURE_2DARRAY);
    state->float_pipelines.image_3d = create_uav_pipeline(context_vk, state->image_layout,
            "cs_uav_clear_3d_float_code.hlsl", WINED3D_SHADER_RESOURCE_TEXTURE_3D);
    state->uint_pipelines.image_3d = create_uav_pipeline(context_vk, state->image_layout,
            "cs_uav_clear_3d_uint_code.hlsl", WINED3D_SHADER_RESOURCE_TEXTURE_3D);

    state->buffer_group_size.x = 128;
    state->buffer_group_size.y = 1;
    state->buffer_group_size.z = 1;
    state->image_1d_group_size.x = 64;
    state->image_1d_group_size.y = 1;
    state->image_1d_group_size.z = 1;
    state->image_1d_array_group_size.x = 64;
    state->image_1d_array_group_size.y = 1;
    state->image_1d_array_group_size.z = 1;
    state->image_2d_group_size.x = 8;
    state->image_2d_group_size.y = 8;
    state->image_2d_group_size.z = 1;
    state->image_2d_array_group_size.x = 8;
    state->image_2d_array_group_size.y = 8;
    state->image_2d_array_group_size.z = 1;
    state->image_3d_group_size.x = 8;
    state->image_3d_group_size.y = 8;
    state->image_3d_group_size.z = 1;
}

void wined3d_device_vk_uav_clear_state_cleanup(struct wined3d_device_vk *device_vk)
{
    struct wined3d_uav_clear_state_vk *state = &device_vk->uav_clear_state;
    const struct wined3d_vk_info *vk_info = &device_vk->vk_info;

    VK_CALL(vkDestroyPipeline(device_vk->vk_device, state->float_pipelines.buffer, NULL));
    VK_CALL(vkDestroyPipeline(device_vk->vk_device, state->float_pipelines.image_1d, NULL));
    VK_CALL(vkDestroyPipeline(device_vk->vk_device, state->float_pipelines.image_1d_array, NULL));
    VK_CALL(vkDestroyPipeline(device_vk->vk_device, state->float_pipelines.image_2d, NULL));
    VK_CALL(vkDestroyPipeline(device_vk->vk_device, state->float_pipelines.image_2d_array, NULL));
    VK_CALL(vkDestroyPipeline(device_vk->vk_device, state->float_pipelines.image_3d, NULL));

    VK_CALL(vkDestroyPipeline(device_vk->vk_device, state->uint_pipelines.buffer, NULL));
    VK_CALL(vkDestroyPipeline(device_vk->vk_device, state->uint_pipelines.image_1d, NULL));
    VK_CALL(vkDestroyPipeline(device_vk->vk_device, state->uint_pipelines.image_1d_array, NULL));
    VK_CALL(vkDestroyPipeline(device_vk->vk_device, state->uint_pipelines.image_2d, NULL));
    VK_CALL(vkDestroyPipeline(device_vk->vk_device, state->uint_pipelines.image_2d_array, NULL));
    VK_CALL(vkDestroyPipeline(device_vk->vk_device, state->uint_pipelines.image_3d, NULL));
}

void wined3d_unordered_access_view_vk_clear(struct wined3d_unordered_access_view_vk *view_vk,
        const struct wined3d_uvec4 *clear_value, struct wined3d_context_vk *context_vk, bool fp)
{
    struct wined3d_bo_address cb_source_address, cb_destination_address;
    const struct wined3d_format *view_format = view_vk->v.format;
    struct wined3d_view_desc *view_desc = &view_vk->v.desc;
    struct wined3d_uav_clear_constants_vk constants = {0};
    struct wined3d_device *device = context_vk->c.device;
    struct wined3d_shader_thread_group_size group_count;
    enum wined3d_format_id format_id = view_format->id;
    struct wined3d_uav_clear_pipelines_vk *pipelines;
    struct wined3d_texture_vk *texture_vk = NULL;
    struct wined3d_pipeline_layout_vk *layout;
    struct wined3d_uav_clear_state_vk *state;
    const struct wined3d_vk_info *vk_info;
    VkDescriptorImageInfo vk_image_info;
    struct wined3d_device_vk *device_vk;
    VkDescriptorBufferInfo buffer_info;
    struct wined3d_resource *resource;
    VkCommandBuffer vk_command_buffer;
    struct wined3d_bo_vk constants_bo;
    VkWriteDescriptorSet vk_writes[2];
    VkBufferView vk_buffer_view;
    struct wined3d_range range;
    VkMemoryBarrier vk_barrier;
    VkPipeline vk_pipeline;
    bool is_array;

    device_vk = wined3d_device_vk(device);
    state = &device_vk->uav_clear_state;
    pipelines = fp ? &state->float_pipelines : &state->uint_pipelines;

    resource = view_vk->v.resource;
    is_array = view_desc->flags & WINED3D_VIEW_TEXTURE_ARRAY;

    switch (resource->type)
    {
        case WINED3D_RTYPE_BUFFER:
            vk_pipeline = pipelines->buffer;
            group_count = state->buffer_group_size;
            break;
        case WINED3D_RTYPE_TEXTURE_1D:
            if (is_array)
            {
                vk_pipeline = pipelines->image_1d_array;
                group_count = state->image_1d_array_group_size;
            }
            else
            {
                vk_pipeline = pipelines->image_1d;
                group_count = state->image_1d_group_size;
            }
            break;
        case WINED3D_RTYPE_TEXTURE_2D:
            if (is_array)
            {
                vk_pipeline = pipelines->image_2d_array;
                group_count = state->image_2d_array_group_size;
            }
            else
            {
                vk_pipeline = pipelines->image_2d;
                group_count = state->image_2d_group_size;
            }
            break;
        case WINED3D_RTYPE_TEXTURE_3D:
            vk_pipeline = pipelines->image_3d;
            group_count = state->image_3d_group_size;
            break;

        default:
            ERR("Unhandled resource type %s.\n", debug_d3dresourcetype(resource->type));
            return;
    }

    if (vk_pipeline == VK_NULL_HANDLE)
    {
        ERR("Pipeline was not correctly initialized.\n");
        return;
    }

    vk_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vk_writes[0].pNext = NULL;
    vk_writes[0].dstBinding = 1;
    vk_writes[0].dstArrayElement = 0;
    vk_writes[0].descriptorCount = 1;
    vk_writes[0].pImageInfo = NULL;
    vk_writes[0].pTexelBufferView = &vk_buffer_view;
    vk_writes[0].pImageInfo = &vk_image_info;

    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        struct wined3d_buffer *buffer = buffer_from_resource(resource);
        unsigned int offset, size;

        layout = state->buffer_layout;
        vk_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;

        if (buffer->structure_byte_stride)
            constants.extent.width = view_desc->u.buffer.count * buffer->structure_byte_stride / 4;
        else
            constants.extent.width = view_desc->u.buffer.count;
        constants.extent.height = 1;

        get_buffer_view_range(buffer, view_desc, view_format, &offset, &size);
        if (!offset && size == buffer->resource.size)
        {
            wined3d_buffer_prepare_location(buffer, &context_vk->c, WINED3D_LOCATION_BUFFER);
        }
        else
        {
            wined3d_buffer_acquire_bo_for_write(buffer, &context_vk->c);
            wined3d_buffer_load_location(buffer, &context_vk->c, WINED3D_LOCATION_BUFFER);
        }
        wined3d_buffer_validate_location(buffer, WINED3D_LOCATION_BUFFER);
        wined3d_buffer_invalidate_location(buffer, ~WINED3D_LOCATION_BUFFER);
    }
    else
    {
        unsigned int layer_count, level_count, base_level, base_layer, i, j;

        texture_vk = wined3d_texture_vk(wined3d_texture_from_resource(resource));

        layout = state->image_layout;
        vk_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;

        level_count = view_desc->u.texture.level_count;
        base_level = view_desc->u.texture.level_idx;
        if (resource->type == WINED3D_RTYPE_TEXTURE_3D)
        {
            layer_count = 1;
            base_layer = 0;
        }
        else
        {
            layer_count = view_desc->u.texture.layer_count;
            base_layer = view_desc->u.texture.layer_idx;
        }

        constants.extent.width  = wined3d_texture_get_level_width(&texture_vk->t, base_level);
        constants.extent.height = wined3d_texture_get_level_height(&texture_vk->t, base_level);
        group_count.z = (view_desc->u.texture.layer_count + group_count.z - 1) / group_count.z;

        for (i = 0; i < layer_count; ++i)
        {
            for (j = 0; j < level_count; ++j)
            {
                unsigned int sub_resource_idx = (base_layer + i) * texture_vk->t.level_count + base_level + j;

                wined3d_texture_prepare_location(&texture_vk->t, sub_resource_idx,
                        &context_vk->c, WINED3D_LOCATION_TEXTURE_RGB);
                wined3d_texture_validate_location(&texture_vk->t, sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB);
                wined3d_texture_invalidate_location(&texture_vk->t, sub_resource_idx, ~WINED3D_LOCATION_TEXTURE_RGB);
            }
        }
    }

    group_count.x = (constants.extent.width  + group_count.x - 1) / group_count.x;
    group_count.y = (constants.extent.height + group_count.y - 1) / group_count.y;

    constants.color.uint32[0] = clear_value->x;
    constants.color.uint32[1] = clear_value->y;
    constants.color.uint32[2] = clear_value->z;
    constants.color.uint32[3] = clear_value->w;

    if (!fp)
    {
        /* Make sure values are truncated, not saturated to some maximum value. */
        constants.color.uint32[0] &= wined3d_mask_from_size(view_format->red_size);
        constants.color.uint32[1] &= wined3d_mask_from_size(view_format->green_size);
        constants.color.uint32[2] &= wined3d_mask_from_size(view_format->blue_size);
        constants.color.uint32[3] &= wined3d_mask_from_size(view_format->alpha_size);

        if (view_format->id == WINED3DFMT_R11G11B10_FLOAT)
        {
            constants.color.uint32[0] |= constants.color.uint32[1] << 11;
            constants.color.uint32[0] |= constants.color.uint32[2] << 22;
            format_id = WINED3DFMT_R32_UINT;
        }
        else
        {
            format_id = wined3d_get_typed_format_id(context_vk->c.device->adapter, view_format,
                    WINED3D_CHANNEL_TYPE_UINT);
        }
    }

    if (format_id == WINED3DFMT_UNKNOWN)
    {
        ERR("Unsupported format %s.\n", debug_d3dformat(view_format->id));
        return;
    }

    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        if (format_id == view_format->id)
            vk_buffer_view = view_vk->view_vk.u.vk_buffer_view;
        else
        {
            vk_buffer_view = wined3d_view_vk_create_vk_buffer_view(context_vk, view_desc,
                    wined3d_buffer_vk(buffer_from_resource(resource)), wined3d_format_vk(
                        wined3d_get_format(context_vk->c.device->adapter, format_id, WINED3D_BIND_UNORDERED_ACCESS)));
            if (vk_buffer_view == VK_NULL_HANDLE)
                return;
        }
    }
    else
    {
        if (format_id == view_format->id)
        {
            vk_image_info = view_vk->view_vk.u.vk_image_info;
            if (!vk_image_info.imageView)
            {
                const VkDescriptorImageInfo *default_info;
                if (!(default_info = wined3d_texture_vk_get_default_image_info(texture_vk, context_vk)))
                    return;
                vk_image_info = *default_info;
            }
        }
        else
        {
            vk_image_info.sampler = VK_NULL_HANDLE;
            vk_image_info.imageLayout = texture_vk->layout;
            vk_image_info.imageView = wined3d_view_vk_create_vk_image_view(context_vk, view_desc, texture_vk,
                    wined3d_format_vk(wined3d_get_format(context_vk->c.device->adapter, format_id,
                        WINED3D_BIND_UNORDERED_ACCESS)),
                    COLOR_FIXUP_IDENTITY, false, VK_IMAGE_USAGE_STORAGE_BIT);

            if (vk_image_info.imageView == VK_NULL_HANDLE)
                return;
        }
    }

    vk_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vk_writes[1].pNext = NULL;
    vk_writes[1].dstBinding = 0;
    vk_writes[1].dstArrayElement = 0;
    vk_writes[1].descriptorCount = 1;
    vk_writes[1].pImageInfo = NULL;
    vk_writes[1].pTexelBufferView = NULL;
    vk_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vk_writes[1].pBufferInfo = &buffer_info;

    if (!wined3d_context_vk_create_bo(context_vk, sizeof(constants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &constants_bo))
    {
        ERR("Failed to create constants BO.\n");
        goto out;
    }

    cb_source_address.buffer_object = NULL;
    cb_source_address.addr = (BYTE *)&constants;
    cb_destination_address.buffer_object = &constants_bo.b;
    cb_destination_address.addr = 0;

    range.offset = 0;
    range.size = sizeof(constants);
    adapter_vk_copy_bo_address(&context_vk->c, &cb_destination_address,
            &cb_source_address, 1, &range, WINED3D_MAP_WRITE | WINED3D_MAP_DISCARD);

    buffer_info.buffer = constants_bo.vk_buffer;
    buffer_info.range = constants_bo.size;
    buffer_info.offset = constants_bo.b.buffer_offset;

    vk_info = context_vk->vk_info;

    if (!(vk_writes[0].dstSet = wined3d_context_vk_create_vk_descriptor_set(context_vk, layout->vk_set_layout)))
    {
        ERR("Failed to create descriptor set.\n");
        wined3d_context_vk_destroy_bo(context_vk, &constants_bo);
        goto out;
    }

    vk_writes[1].dstSet = vk_writes[0].dstSet;

    VK_CALL(vkUpdateDescriptorSets(device_vk->vk_device, 2, vk_writes, 0, NULL));

    vk_command_buffer = wined3d_context_vk_get_command_buffer(context_vk);
    wined3d_context_vk_end_current_render_pass(context_vk);

    wined3d_context_vk_reference_unordered_access_view(context_vk, view_vk);
    wined3d_context_vk_reference_bo(context_vk, &constants_bo);
    wined3d_context_vk_destroy_bo(context_vk, &constants_bo);

    vk_barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    vk_barrier.pNext = NULL;
    vk_barrier.srcAccessMask = vk_access_mask_from_bind_flags(resource->bind_flags);
    vk_barrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

    VK_CALL(vkCmdPipelineBarrier(vk_command_buffer,
            vk_pipeline_stage_mask_from_bind_flags(resource->bind_flags),
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            0, 1, &vk_barrier, 0, NULL, 0, NULL));
    VK_CALL(vkCmdBindPipeline(vk_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, vk_pipeline));
    VK_CALL(vkCmdBindDescriptorSets(vk_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE,
            layout->vk_pipeline_layout, 0, 1, &vk_writes[0].dstSet, 0, NULL));
    VK_CALL(vkCmdDispatch(vk_command_buffer, group_count.x, group_count.y, group_count.z));

    vk_barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    vk_barrier.dstAccessMask = vk_access_mask_from_bind_flags(resource->bind_flags);

    VK_CALL(vkCmdPipelineBarrier(vk_command_buffer,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            vk_pipeline_stage_mask_from_bind_flags(resource->bind_flags),
            0, 1, &vk_barrier, 0, NULL, 0, NULL));

    context_invalidate_compute_state(&context_vk->c, STATE_COMPUTE_SHADER);

out:
    if (format_id != view_format->id)
    {
        if (resource->type == WINED3D_RTYPE_BUFFER)
            wined3d_context_vk_destroy_vk_buffer_view(context_vk, vk_buffer_view, view_vk->view_vk.command_buffer_id);
        else
            wined3d_context_vk_destroy_vk_image_view(context_vk, vk_image_info.imageView,
                    view_vk->view_vk.command_buffer_id);
    }
}

void wined3d_unordered_access_view_vk_update(struct wined3d_unordered_access_view_vk *uav_vk,
        struct wined3d_context_vk *context_vk)
{
    const struct wined3d_format_vk *view_format_vk = wined3d_format_vk(uav_vk->v.format);
    const struct wined3d_view_desc *desc = &uav_vk->v.desc;
    struct wined3d_resource *resource = uav_vk->v.resource;
    struct wined3d_view_vk *view_vk = &uav_vk->view_vk;
    struct wined3d_buffer_vk *buffer_vk;
    VkBufferView vk_buffer_view;

    assert(!view_vk->bo_user.valid);

    buffer_vk = wined3d_buffer_vk(buffer_from_resource(resource));
    wined3d_context_vk_destroy_vk_buffer_view(context_vk, view_vk->u.vk_buffer_view, view_vk->command_buffer_id);
    if ((vk_buffer_view = wined3d_view_vk_create_vk_buffer_view(context_vk, desc, buffer_vk, view_format_vk)))
    {
        view_vk->u.vk_buffer_view = vk_buffer_view;
        view_vk->bo_user.valid = true;
        list_add_head(&buffer_vk->b.buffer_object->users, &view_vk->bo_user.entry);
    }
}

static void wined3d_unordered_access_view_vk_cs_init(void *object)
{
    struct wined3d_unordered_access_view_vk *uav_vk = object;
    struct wined3d_view_vk *view_vk = &uav_vk->view_vk;
    struct wined3d_view_desc *desc = &uav_vk->v.desc;
    const struct wined3d_format_vk *format_vk;
    const struct wined3d_vk_info *vk_info;
    struct wined3d_texture_vk *texture_vk;
    struct wined3d_context_vk *context_vk;
    struct wined3d_device_vk *device_vk;
    struct wined3d_buffer_vk *buffer_vk;
    VkBufferViewCreateInfo create_info;
    struct wined3d_resource *resource;
    VkBufferView vk_buffer_view;
    uint32_t default_flags = 0;
    VkImageView vk_image_view;
    VkResult vr;

    TRACE("uav_vk %p.\n", uav_vk);

    resource = uav_vk->v.resource;
    device_vk = wined3d_device_vk(resource->device);
    format_vk = wined3d_format_vk(uav_vk->v.format);

    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        buffer_vk = wined3d_buffer_vk(buffer_from_resource(resource));

        context_vk = wined3d_context_vk(context_acquire(&device_vk->d, NULL, 0));
        vk_info = context_vk->vk_info;

        if ((vk_buffer_view = wined3d_view_vk_create_vk_buffer_view(context_vk, desc, buffer_vk, format_vk)))
        {
            struct wined3d_bo_vk *bo = wined3d_bo_vk(buffer_vk->b.buffer_object);

            TRACE("Created buffer view 0x%s.\n", wine_dbgstr_longlong(vk_buffer_view));

            uav_vk->view_vk.u.vk_buffer_view = vk_buffer_view;
            uav_vk->view_vk.bo_user.valid = true;
            list_add_head(&bo->b.users, &view_vk->bo_user.entry);
        }

        if (desc->flags & (WINED3D_VIEW_BUFFER_COUNTER | WINED3D_VIEW_BUFFER_APPEND))
        {
            if (!wined3d_context_vk_create_bo(context_vk, sizeof(uint32_t), VK_BUFFER_USAGE_TRANSFER_SRC_BIT
                    | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &uav_vk->counter_bo))
            {
                ERR("Failed to create counter bo.\n");
                context_release(&context_vk->c);

                return;
            }

            wined3d_context_vk_end_current_render_pass(context_vk);
            VK_CALL(vkCmdFillBuffer(wined3d_context_vk_get_command_buffer(context_vk),
                    uav_vk->counter_bo.vk_buffer, uav_vk->counter_bo.b.buffer_offset, sizeof(uint32_t), 0));
            wined3d_context_vk_reference_bo(context_vk, &uav_vk->counter_bo);

            create_info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
            create_info.pNext = NULL;
            create_info.flags = 0;
            create_info.buffer = uav_vk->counter_bo.vk_buffer;
            create_info.format = VK_FORMAT_R32_UINT;
            create_info.offset = uav_vk->counter_bo.b.buffer_offset;
            create_info.range = sizeof(uint32_t);
            if ((vr = VK_CALL(vkCreateBufferView(device_vk->vk_device,
                    &create_info, NULL, &uav_vk->vk_counter_view))) < 0)
            {
                ERR("Failed to create counter buffer view, vr %s.\n", wined3d_debug_vkresult(vr));
            }
            else
            {
                TRACE("Created counter buffer view 0x%s.\n", wine_dbgstr_longlong(uav_vk->vk_counter_view));

                uav_vk->v.counter_bo = &uav_vk->counter_bo.b;
            }
        }

        context_release(&context_vk->c);

        return;
    }

    texture_vk = wined3d_texture_vk(texture_from_resource(resource));

    if (texture_vk->t.layer_count > 1)
        default_flags |= WINED3D_VIEW_TEXTURE_ARRAY;

    if (resource->format->id == format_vk->f.id && desc->flags == default_flags
            && !desc->u.texture.level_idx && desc->u.texture.level_count == texture_vk->t.level_count
            && !desc->u.texture.layer_idx && desc->u.texture.layer_count == texture_vk->t.layer_count
            && !(resource->bind_flags & WINED3D_BIND_DEPTH_STENCIL) && resource->type != WINED3D_RTYPE_TEXTURE_3D)
    {
        TRACE("Creating identity unordered access view.\n");
        return;
    }

    if (texture_vk->t.swapchain && texture_vk->t.swapchain->state.desc.backbuffer_count > 1)
        FIXME("Swapchain unordered access views not supported.\n");

    context_vk = wined3d_context_vk(context_acquire(&device_vk->d, NULL, 0));
    vk_image_view = wined3d_view_vk_create_vk_image_view(context_vk, desc,
            texture_vk, format_vk, format_vk->f.color_fixup, false, VK_IMAGE_USAGE_STORAGE_BIT);
    context_release(&context_vk->c);

    if (!vk_image_view)
        return;

    TRACE("Created image view 0x%s.\n", wine_dbgstr_longlong(vk_image_view));

    view_vk->u.vk_image_info.imageView = vk_image_view;
    view_vk->u.vk_image_info.sampler = VK_NULL_HANDLE;
    view_vk->u.vk_image_info.imageLayout = texture_vk->layout;
}

HRESULT wined3d_unordered_access_view_vk_init(struct wined3d_unordered_access_view_vk *view_vk,
        const struct wined3d_view_desc *desc, struct wined3d_resource *resource,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    HRESULT hr;

    TRACE("view_vk %p, desc %s, resource %p, parent %p, parent_ops %p.\n",
            view_vk, wined3d_debug_view_desc(desc, resource), resource, parent, parent_ops);

    if (FAILED(hr = wined3d_unordered_access_view_init(&view_vk->v, desc, resource, parent, parent_ops)))
        return hr;

    list_init(&view_vk->view_vk.bo_user.entry);
    wined3d_cs_init_object(resource->device->cs, wined3d_unordered_access_view_vk_cs_init, view_vk);

    return hr;
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
