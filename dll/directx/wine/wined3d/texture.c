/*
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
 * Copyright 2007-2009, 2013 Stefan Dösinger for CodeWeavers
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
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_perf);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

#define WINED3D_TEXTURE_DYNAMIC_MAP_THRESHOLD 50

static const uint32_t wined3d_texture_sysmem_locations = WINED3D_LOCATION_SYSMEM
        | WINED3D_LOCATION_USER_MEMORY | WINED3D_LOCATION_BUFFER;
static const struct wined3d_texture_ops texture_gl_ops;

struct wined3d_texture_idx
{
    struct wined3d_texture *texture;
    unsigned int sub_resource_idx;
};

struct wined3d_rect_f
{
    float l;
    float t;
    float r;
    float b;
};

static BOOL wined3d_texture_use_pbo(const struct wined3d_texture *texture, const struct wined3d_gl_info *gl_info)
{
    if (!gl_info->supported[ARB_PIXEL_BUFFER_OBJECT]
            || texture->resource.format->conv_byte_count
            || (texture->flags & (WINED3D_TEXTURE_PIN_SYSMEM | WINED3D_TEXTURE_COND_NP2_EMULATED)))
        return FALSE;

    /* Use a PBO for dynamic textures and read-only staging textures. */
    return (!(texture->resource.access & WINED3D_RESOURCE_ACCESS_CPU)
                && texture->resource.usage & WINED3DUSAGE_DYNAMIC)
            || texture->resource.access == (WINED3D_RESOURCE_ACCESS_CPU | WINED3D_RESOURCE_ACCESS_MAP_R);
}

static BOOL wined3d_texture_use_immutable_storage(const struct wined3d_texture *texture,
        const struct wined3d_gl_info *gl_info)
{
    /* We don't expect to create texture views for textures with height-scaled formats.
     * Besides, ARB_texture_storage doesn't allow specifying exact sizes for all levels. */
    return gl_info->supported[ARB_TEXTURE_STORAGE]
            && !(texture->resource.format_flags & WINED3DFMT_FLAG_HEIGHT_SCALE);
}

/* Front buffer coordinates are always full screen coordinates, but our GL
 * drawable is limited to the window's client area. The sysmem and texture
 * copies do have the full screen size. Note that GL has a bottom-left
 * origin, while D3D has a top-left origin. */
void wined3d_texture_translate_drawable_coords(const struct wined3d_texture *texture, HWND window, RECT *rect)
{
    unsigned int drawable_height;
    POINT offset = {0, 0};
    RECT windowsize;

    if (!texture->swapchain)
        return;

    if (texture == texture->swapchain->front_buffer)
    {
        ScreenToClient(window, &offset);
        OffsetRect(rect, offset.x, offset.y);
    }

    GetClientRect(window, &windowsize);
    drawable_height = windowsize.bottom - windowsize.top;

    rect->top = drawable_height - rect->top;
    rect->bottom = drawable_height - rect->bottom;
}

GLenum wined3d_texture_get_gl_buffer(const struct wined3d_texture *texture)
{
    const struct wined3d_swapchain *swapchain = texture->swapchain;

    TRACE("texture %p.\n", texture);

    if (!swapchain)
    {
        ERR("Texture %p is not part of a swapchain.\n", texture);
        return GL_NONE;
    }

    if (texture == swapchain->front_buffer)
    {
        TRACE("Returning GL_FRONT.\n");
        return GL_FRONT;
    }

    if (texture == swapchain->back_buffers[0])
    {
        TRACE("Returning GL_BACK.\n");
        return GL_BACK;
    }

    FIXME("Higher back buffer, returning GL_BACK.\n");
    return GL_BACK;
}

static DWORD wined3d_resource_access_from_location(DWORD location)
{
    switch (location)
    {
        case WINED3D_LOCATION_DISCARDED:
            return 0;

        case WINED3D_LOCATION_SYSMEM:
        case WINED3D_LOCATION_USER_MEMORY:
            return WINED3D_RESOURCE_ACCESS_CPU;

        case WINED3D_LOCATION_BUFFER:
        case WINED3D_LOCATION_DRAWABLE:
        case WINED3D_LOCATION_TEXTURE_RGB:
        case WINED3D_LOCATION_TEXTURE_SRGB:
        case WINED3D_LOCATION_RB_MULTISAMPLE:
        case WINED3D_LOCATION_RB_RESOLVED:
            return WINED3D_RESOURCE_ACCESS_GPU;

        default:
            FIXME("Unhandled location %#x.\n", location);
            return 0;
    }
}

static inline void cube_coords_float(const RECT *r, UINT w, UINT h, struct wined3d_rect_f *f)
{
    f->l = ((r->left * 2.0f) / w) - 1.0f;
    f->t = ((r->top * 2.0f) / h) - 1.0f;
    f->r = ((r->right * 2.0f) / w) - 1.0f;
    f->b = ((r->bottom * 2.0f) / h) - 1.0f;
}

void texture2d_get_blt_info(const struct wined3d_texture_gl *texture_gl,
        unsigned int sub_resource_idx, const RECT *rect, struct wined3d_blt_info *info)
{
    struct wined3d_vec3 *coords = info->texcoords;
    struct wined3d_rect_f f;
    unsigned int level;
    GLenum target;
    GLsizei w, h;

    level = sub_resource_idx % texture_gl->t.level_count;
    w = wined3d_texture_get_level_pow2_width(&texture_gl->t, level);
    h = wined3d_texture_get_level_pow2_height(&texture_gl->t, level);
    target = wined3d_texture_gl_get_sub_resource_target(texture_gl, sub_resource_idx);

    switch (target)
    {
        default:
            FIXME("Unsupported texture target %#x.\n", target);
            /* Fall back to GL_TEXTURE_2D */
        case GL_TEXTURE_2D:
            info->bind_target = GL_TEXTURE_2D;
            coords[0].x = (float)rect->left / w;
            coords[0].y = (float)rect->top / h;
            coords[0].z = 0.0f;

            coords[1].x = (float)rect->right / w;
            coords[1].y = (float)rect->top / h;
            coords[1].z = 0.0f;

            coords[2].x = (float)rect->left / w;
            coords[2].y = (float)rect->bottom / h;
            coords[2].z = 0.0f;

            coords[3].x = (float)rect->right / w;
            coords[3].y = (float)rect->bottom / h;
            coords[3].z = 0.0f;
            break;

        case GL_TEXTURE_RECTANGLE_ARB:
            info->bind_target = GL_TEXTURE_RECTANGLE_ARB;
            coords[0].x = rect->left;  coords[0].y = rect->top;    coords[0].z = 0.0f;
            coords[1].x = rect->right; coords[1].y = rect->top;    coords[1].z = 0.0f;
            coords[2].x = rect->left;  coords[2].y = rect->bottom; coords[2].z = 0.0f;
            coords[3].x = rect->right; coords[3].y = rect->bottom; coords[3].z = 0.0f;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            cube_coords_float(rect, w, h, &f);

            coords[0].x =  1.0f;   coords[0].y = -f.t;   coords[0].z = -f.l;
            coords[1].x =  1.0f;   coords[1].y = -f.t;   coords[1].z = -f.r;
            coords[2].x =  1.0f;   coords[2].y = -f.b;   coords[2].z = -f.l;
            coords[3].x =  1.0f;   coords[3].y = -f.b;   coords[3].z = -f.r;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            cube_coords_float(rect, w, h, &f);

            coords[0].x = -1.0f;   coords[0].y = -f.t;   coords[0].z = f.l;
            coords[1].x = -1.0f;   coords[1].y = -f.t;   coords[1].z = f.r;
            coords[2].x = -1.0f;   coords[2].y = -f.b;   coords[2].z = f.l;
            coords[3].x = -1.0f;   coords[3].y = -f.b;   coords[3].z = f.r;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            cube_coords_float(rect, w, h, &f);

            coords[0].x = f.l;   coords[0].y =  1.0f;   coords[0].z = f.t;
            coords[1].x = f.r;   coords[1].y =  1.0f;   coords[1].z = f.t;
            coords[2].x = f.l;   coords[2].y =  1.0f;   coords[2].z = f.b;
            coords[3].x = f.r;   coords[3].y =  1.0f;   coords[3].z = f.b;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            cube_coords_float(rect, w, h, &f);

            coords[0].x = f.l;   coords[0].y = -1.0f;   coords[0].z = -f.t;
            coords[1].x = f.r;   coords[1].y = -1.0f;   coords[1].z = -f.t;
            coords[2].x = f.l;   coords[2].y = -1.0f;   coords[2].z = -f.b;
            coords[3].x = f.r;   coords[3].y = -1.0f;   coords[3].z = -f.b;
            break;

        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            cube_coords_float(rect, w, h, &f);

            coords[0].x = f.l;   coords[0].y = -f.t;   coords[0].z =  1.0f;
            coords[1].x = f.r;   coords[1].y = -f.t;   coords[1].z =  1.0f;
            coords[2].x = f.l;   coords[2].y = -f.b;   coords[2].z =  1.0f;
            coords[3].x = f.r;   coords[3].y = -f.b;   coords[3].z =  1.0f;
            break;

        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            info->bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            cube_coords_float(rect, w, h, &f);

            coords[0].x = -f.l;   coords[0].y = -f.t;   coords[0].z = -1.0f;
            coords[1].x = -f.r;   coords[1].y = -f.t;   coords[1].z = -1.0f;
            coords[2].x = -f.l;   coords[2].y = -f.b;   coords[2].z = -1.0f;
            coords[3].x = -f.r;   coords[3].y = -f.b;   coords[3].z = -1.0f;
            break;
    }
}

static void wined3d_texture_evict_sysmem(struct wined3d_texture *texture)
{
    struct wined3d_texture_sub_resource *sub_resource;
    unsigned int i, sub_count;

    if (texture->flags & (WINED3D_TEXTURE_CONVERTED | WINED3D_TEXTURE_PIN_SYSMEM)
            || texture->download_count > WINED3D_TEXTURE_DYNAMIC_MAP_THRESHOLD)
    {
        TRACE("Not evicting system memory for texture %p.\n", texture);
        return;
    }

    TRACE("Evicting system memory for texture %p.\n", texture);

    sub_count = texture->level_count * texture->layer_count;
    for (i = 0; i < sub_count; ++i)
    {
        sub_resource = &texture->sub_resources[i];
        if (sub_resource->locations == WINED3D_LOCATION_SYSMEM)
            ERR("WINED3D_LOCATION_SYSMEM is the only location for sub-resource %u of texture %p.\n",
                    i, texture);
        sub_resource->locations &= ~WINED3D_LOCATION_SYSMEM;
    }
    wined3d_resource_free_sysmem(&texture->resource);
}

void wined3d_texture_validate_location(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, DWORD location)
{
    struct wined3d_texture_sub_resource *sub_resource;
    DWORD previous_locations;

    TRACE("texture %p, sub_resource_idx %u, location %s.\n",
            texture, sub_resource_idx, wined3d_debug_location(location));

    sub_resource = &texture->sub_resources[sub_resource_idx];
    previous_locations = sub_resource->locations;
    sub_resource->locations |= location;
    if (previous_locations == WINED3D_LOCATION_SYSMEM && location != WINED3D_LOCATION_SYSMEM
            && !--texture->sysmem_count)
        wined3d_texture_evict_sysmem(texture);

    TRACE("New locations flags are %s.\n", wined3d_debug_location(sub_resource->locations));
}

static void wined3d_texture_set_dirty(struct wined3d_texture *texture)
{
    texture->flags &= ~(WINED3D_TEXTURE_RGB_VALID | WINED3D_TEXTURE_SRGB_VALID);
}

void wined3d_texture_invalidate_location(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, DWORD location)
{
    struct wined3d_texture_sub_resource *sub_resource;
    DWORD previous_locations;

    TRACE("texture %p, sub_resource_idx %u, location %s.\n",
            texture, sub_resource_idx, wined3d_debug_location(location));

    if (location & (WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB))
        wined3d_texture_set_dirty(texture);

    sub_resource = &texture->sub_resources[sub_resource_idx];
    previous_locations = sub_resource->locations;
    sub_resource->locations &= ~location;
    if (previous_locations != WINED3D_LOCATION_SYSMEM && sub_resource->locations == WINED3D_LOCATION_SYSMEM)
        ++texture->sysmem_count;

    TRACE("New locations flags are %s.\n", wined3d_debug_location(sub_resource->locations));

    if (!sub_resource->locations)
        ERR("Sub-resource %u of texture %p does not have any up to date location.\n",
                sub_resource_idx, texture);
}

static BOOL wined3d_texture_copy_sysmem_location(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_context *context, DWORD location)
{
    unsigned int size = texture->sub_resources[sub_resource_idx].size;
    struct wined3d_device *device = texture->resource.device;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_bo_address dst, src;

    if (!wined3d_texture_prepare_location(texture, sub_resource_idx, context, location))
        return FALSE;

    wined3d_texture_get_memory(texture, sub_resource_idx, &dst, location);
    wined3d_texture_get_memory(texture, sub_resource_idx, &src,
            texture->sub_resources[sub_resource_idx].locations);

    if (dst.buffer_object)
    {
        context = context_acquire(device, NULL, 0);
        gl_info = wined3d_context_gl(context)->gl_info;
        GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, dst.buffer_object));
        GL_EXTCALL(glBufferSubData(GL_PIXEL_UNPACK_BUFFER, 0, size, src.addr));
        GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
        checkGLcall("PBO upload");
        context_release(context);
        return TRUE;
    }

    if (src.buffer_object)
    {
        context = context_acquire(device, NULL, 0);
        gl_info = wined3d_context_gl(context)->gl_info;
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, src.buffer_object));
        GL_EXTCALL(glGetBufferSubData(GL_PIXEL_PACK_BUFFER, 0, size, dst.addr));
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
        checkGLcall("PBO download");
        context_release(context);
        return TRUE;
    }

    memcpy(dst.addr, src.addr, size);
    return TRUE;
}

/* Context activation is done by the caller. Context may be NULL in
 * WINED3D_NO3D mode. */
BOOL wined3d_texture_load_location(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_context *context, DWORD location)
{
    DWORD current = texture->sub_resources[sub_resource_idx].locations;
    BOOL ret;

    TRACE("texture %p, sub_resource_idx %u, context %p, location %s.\n",
            texture, sub_resource_idx, context, wined3d_debug_location(location));

    TRACE("Current resource location %s.\n", wined3d_debug_location(current));

    if (current & location)
    {
        TRACE("Location %s is already up to date.\n", wined3d_debug_location(location));
        return TRUE;
    }

    if (WARN_ON(d3d))
    {
        DWORD required_access = wined3d_resource_access_from_location(location);
        if ((texture->resource.access & required_access) != required_access)
            WARN("Operation requires %#x access, but texture only has %#x.\n",
                    required_access, texture->resource.access);
    }

    if (current & WINED3D_LOCATION_DISCARDED)
    {
        TRACE("Sub-resource previously discarded, nothing to do.\n");
        if (!wined3d_texture_prepare_location(texture, sub_resource_idx, context, location))
            return FALSE;
        wined3d_texture_validate_location(texture, sub_resource_idx, location);
        wined3d_texture_invalidate_location(texture, sub_resource_idx, WINED3D_LOCATION_DISCARDED);
        return TRUE;
    }

    if (!current)
    {
        ERR("Sub-resource %u of texture %p does not have any up to date location.\n",
                sub_resource_idx, texture);
        wined3d_texture_validate_location(texture, sub_resource_idx, WINED3D_LOCATION_DISCARDED);
        return wined3d_texture_load_location(texture, sub_resource_idx, context, location);
    }

    if ((location & wined3d_texture_sysmem_locations) && (current & wined3d_texture_sysmem_locations))
        ret = wined3d_texture_copy_sysmem_location(texture, sub_resource_idx, context, location);
    else
        ret = texture->texture_ops->texture_load_location(texture, sub_resource_idx, context, location);

    if (ret)
        wined3d_texture_validate_location(texture, sub_resource_idx, location);

    return ret;
}

void wined3d_texture_get_memory(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        struct wined3d_bo_address *data, DWORD locations)
{
    struct wined3d_texture_sub_resource *sub_resource;

    TRACE("texture %p, sub_resource_idx %u, data %p, locations %s.\n",
            texture, sub_resource_idx, data, wined3d_debug_location(locations));

    sub_resource = &texture->sub_resources[sub_resource_idx];
    if (locations & WINED3D_LOCATION_BUFFER)
    {
        data->addr = NULL;
        data->buffer_object = sub_resource->buffer_object;
        return;
    }
    if (locations & WINED3D_LOCATION_USER_MEMORY)
    {
        data->addr = texture->user_memory;
        data->buffer_object = 0;
        return;
    }
    if (locations & WINED3D_LOCATION_SYSMEM)
    {
        data->addr = texture->resource.heap_memory;
        data->addr += sub_resource->offset;
        data->buffer_object = 0;
        return;
    }

    ERR("Unexpected locations %s.\n", wined3d_debug_location(locations));
    data->addr = NULL;
    data->buffer_object = 0;
}

/* Context activation is done by the caller. */
static void wined3d_texture_remove_buffer_object(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, const struct wined3d_gl_info *gl_info)
{
    GLuint *buffer_object = &texture->sub_resources[sub_resource_idx].buffer_object;

    GL_EXTCALL(glDeleteBuffers(1, buffer_object));
    checkGLcall("glDeleteBuffers");

    TRACE("Deleted buffer object %u for texture %p, sub-resource %u.\n",
            *buffer_object, texture, sub_resource_idx);

    wined3d_texture_invalidate_location(texture, sub_resource_idx, WINED3D_LOCATION_BUFFER);
    *buffer_object = 0;
}

static void wined3d_texture_update_map_binding(struct wined3d_texture *texture)
{
    unsigned int sub_count = texture->level_count * texture->layer_count;
    struct wined3d_device *device = texture->resource.device;
    DWORD map_binding = texture->update_map_binding;
    struct wined3d_context *context = NULL;
    unsigned int i;

    if (device->d3d_initialized)
        context = context_acquire(device, NULL, 0);

    for (i = 0; i < sub_count; ++i)
    {
        if (texture->sub_resources[i].locations == texture->resource.map_binding
                && !wined3d_texture_load_location(texture, i, context, map_binding))
            ERR("Failed to load location %s.\n", wined3d_debug_location(map_binding));
        if (texture->resource.map_binding == WINED3D_LOCATION_BUFFER)
            wined3d_texture_remove_buffer_object(texture, i, wined3d_context_gl(context)->gl_info);
    }

    if (context)
        context_release(context);

    texture->resource.map_binding = map_binding;
    texture->update_map_binding = 0;
}

void wined3d_texture_set_map_binding(struct wined3d_texture *texture, DWORD map_binding)
{
    texture->update_map_binding = map_binding;
    if (!texture->resource.map_count)
        wined3d_texture_update_map_binding(texture);
}

/* A GL context is provided by the caller */
static void gltexture_delete(struct wined3d_device *device, const struct wined3d_gl_info *gl_info,
        struct gl_texture *tex)
{
    context_gl_resource_released(device, tex->name, FALSE);
    gl_info->gl_ops.gl.p_glDeleteTextures(1, &tex->name);
    tex->name = 0;
}

/* Context activation is done by the caller. */
/* The caller is responsible for binding the correct texture. */
static void wined3d_texture_gl_allocate_mutable_storage(struct wined3d_texture_gl *texture_gl,
        GLenum gl_internal_format, const struct wined3d_format_gl *format,
        const struct wined3d_gl_info *gl_info)
{
    unsigned int level, level_count, layer, layer_count;
    GLsizei width, height, depth;
    GLenum target;

    level_count = texture_gl->t.level_count;
    if (texture_gl->target == GL_TEXTURE_1D_ARRAY || texture_gl->target == GL_TEXTURE_2D_ARRAY)
        layer_count = 1;
    else
        layer_count = texture_gl->t.layer_count;

    for (layer = 0; layer < layer_count; ++layer)
    {
        target = wined3d_texture_gl_get_sub_resource_target(texture_gl, layer * level_count);

        for (level = 0; level < level_count; ++level)
        {
            width = wined3d_texture_get_level_pow2_width(&texture_gl->t, level);
            height = wined3d_texture_get_level_pow2_height(&texture_gl->t, level);
            if (texture_gl->t.resource.format_flags & WINED3DFMT_FLAG_HEIGHT_SCALE)
            {
                height *= format->f.height_scale.numerator;
                height /= format->f.height_scale.denominator;
            }

            TRACE("texture_gl %p, layer %u, level %u, target %#x, width %u, height %u.\n",
                    texture_gl, layer, level, target, width, height);

            if (target == GL_TEXTURE_3D || target == GL_TEXTURE_2D_ARRAY)
            {
                depth = wined3d_texture_get_level_depth(&texture_gl->t, level);
                GL_EXTCALL(glTexImage3D(target, level, gl_internal_format, width, height,
                        target == GL_TEXTURE_2D_ARRAY ? texture_gl->t.layer_count : depth, 0,
                        format->format, format->type, NULL));
                checkGLcall("glTexImage3D");
            }
            else if (target == GL_TEXTURE_1D)
            {
                gl_info->gl_ops.gl.p_glTexImage1D(target, level, gl_internal_format,
                        width, 0, format->format, format->type, NULL);
            }
            else
            {
                gl_info->gl_ops.gl.p_glTexImage2D(target, level, gl_internal_format, width,
                        target == GL_TEXTURE_1D_ARRAY ? texture_gl->t.layer_count : height, 0,
                        format->format, format->type, NULL);
                checkGLcall("glTexImage2D");
            }
        }
    }
}

/* Context activation is done by the caller. */
/* The caller is responsible for binding the correct texture. */
static void wined3d_texture_gl_allocate_immutable_storage(struct wined3d_texture_gl *texture_gl,
        GLenum gl_internal_format, const struct wined3d_gl_info *gl_info)
{
    unsigned int samples = wined3d_resource_get_sample_count(&texture_gl->t.resource);
    GLsizei height = wined3d_texture_get_level_pow2_height(&texture_gl->t, 0);
    GLsizei width = wined3d_texture_get_level_pow2_width(&texture_gl->t, 0);
    GLboolean standard_pattern = texture_gl->t.resource.multisample_type != WINED3D_MULTISAMPLE_NON_MASKABLE
            && texture_gl->t.resource.multisample_quality == WINED3D_STANDARD_MULTISAMPLE_PATTERN;

    switch (texture_gl->target)
    {
        case GL_TEXTURE_3D:
            GL_EXTCALL(glTexStorage3D(texture_gl->target, texture_gl->t.level_count,
                    gl_internal_format, width, height, wined3d_texture_get_level_depth(&texture_gl->t, 0)));
            break;
        case GL_TEXTURE_2D_ARRAY:
            GL_EXTCALL(glTexStorage3D(texture_gl->target, texture_gl->t.level_count,
                    gl_internal_format, width, height, texture_gl->t.layer_count));
            break;
        case GL_TEXTURE_2D_MULTISAMPLE:
            GL_EXTCALL(glTexStorage2DMultisample(texture_gl->target, samples,
                    gl_internal_format, width, height, standard_pattern));
            break;
        case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
            GL_EXTCALL(glTexStorage3DMultisample(texture_gl->target, samples,
                    gl_internal_format, width, height, texture_gl->t.layer_count, standard_pattern));
            break;
        case GL_TEXTURE_1D_ARRAY:
            GL_EXTCALL(glTexStorage2D(texture_gl->target, texture_gl->t.level_count,
                    gl_internal_format, width, texture_gl->t.layer_count));
            break;
        case GL_TEXTURE_1D:
            GL_EXTCALL(glTexStorage1D(texture_gl->target, texture_gl->t.level_count, gl_internal_format, width));
            break;
        default:
            GL_EXTCALL(glTexStorage2D(texture_gl->target, texture_gl->t.level_count,
                    gl_internal_format, width, height));
            break;
    }

    checkGLcall("allocate immutable storage");
}

void wined3d_texture_gl_unload_texture(struct wined3d_texture_gl *texture_gl)
{
    struct wined3d_device *device = texture_gl->t.resource.device;
    const struct wined3d_gl_info *gl_info = NULL;
    struct wined3d_context *context = NULL;

    if (texture_gl->t.resource.bind_count)
        device_invalidate_state(device, STATE_SAMPLER(texture_gl->t.sampler));

    if (texture_gl->texture_rgb.name || texture_gl->texture_srgb.name
            || texture_gl->rb_multisample || texture_gl->rb_resolved)
    {
        context = context_acquire(device, NULL, 0);
        gl_info = wined3d_context_gl(context)->gl_info;
    }

    if (texture_gl->texture_rgb.name)
        gltexture_delete(device, gl_info, &texture_gl->texture_rgb);

    if (texture_gl->texture_srgb.name)
        gltexture_delete(device, gl_info, &texture_gl->texture_srgb);

    if (texture_gl->rb_multisample)
    {
        TRACE("Deleting multisample renderbuffer %u.\n", texture_gl->rb_multisample);
        context_gl_resource_released(device, texture_gl->rb_multisample, TRUE);
        gl_info->fbo_ops.glDeleteRenderbuffers(1, &texture_gl->rb_multisample);
        texture_gl->rb_multisample = 0;
    }

    if (texture_gl->rb_resolved)
    {
        TRACE("Deleting resolved renderbuffer %u.\n", texture_gl->rb_resolved);
        context_gl_resource_released(device, texture_gl->rb_resolved, TRUE);
        gl_info->fbo_ops.glDeleteRenderbuffers(1, &texture_gl->rb_resolved);
        texture_gl->rb_resolved = 0;
    }

    if (context) context_release(context);

    wined3d_texture_set_dirty(&texture_gl->t);

    resource_unload(&texture_gl->t.resource);
}

void wined3d_texture_sub_resources_destroyed(struct wined3d_texture *texture)
{
    unsigned int sub_count = texture->level_count * texture->layer_count;
    struct wined3d_texture_sub_resource *sub_resource;
    unsigned int i;

    for (i = 0; i < sub_count; ++i)
    {
        sub_resource = &texture->sub_resources[i];
        if (sub_resource->parent)
        {
            TRACE("sub-resource %u.\n", i);
            sub_resource->parent_ops->wined3d_object_destroyed(sub_resource->parent);
            sub_resource->parent = NULL;
        }
    }
}

static void wined3d_texture_create_dc(void *object)
{
    const struct wined3d_texture_idx *idx = object;
    struct wined3d_context *context = NULL;
    unsigned int sub_resource_idx, level;
    const struct wined3d_format *format;
    unsigned int row_pitch, slice_pitch;
    struct wined3d_texture *texture;
    struct wined3d_dc_info *dc_info;
    struct wined3d_bo_address data;
    D3DKMT_CREATEDCFROMMEMORY desc;
    struct wined3d_device *device;
    NTSTATUS status;

    TRACE("texture %p, sub_resource_idx %u.\n", idx->texture, idx->sub_resource_idx);

    texture = idx->texture;
    sub_resource_idx = idx->sub_resource_idx;
    level = sub_resource_idx % texture->level_count;
    device = texture->resource.device;

    format = texture->resource.format;
    if (!format->ddi_format)
    {
        WARN("Cannot create a DC for format %s.\n", debug_d3dformat(format->id));
        return;
    }

    if (!texture->dc_info)
    {
        unsigned int sub_count = texture->level_count * texture->layer_count;

        if (!(texture->dc_info = heap_calloc(sub_count, sizeof(*texture->dc_info))))
        {
            ERR("Failed to allocate DC info.\n");
            return;
        }
    }

    if (!(texture->sub_resources[sub_resource_idx].locations & texture->resource.map_binding))
    {
        context = context_acquire(device, NULL, 0);
        wined3d_texture_load_location(texture, sub_resource_idx, context, texture->resource.map_binding);
    }
    wined3d_texture_invalidate_location(texture, sub_resource_idx, ~texture->resource.map_binding);
    wined3d_texture_get_pitch(texture, level, &row_pitch, &slice_pitch);
    wined3d_texture_get_memory(texture, sub_resource_idx, &data, texture->resource.map_binding);
    if (data.buffer_object)
    {
        if (!context)
            context = context_acquire(device, NULL, 0);
        desc.pMemory = wined3d_context_map_bo_address(context, &data,
                texture->sub_resources[sub_resource_idx].size, 0, WINED3D_MAP_READ | WINED3D_MAP_WRITE);
    }
    else
    {
        desc.pMemory = data.addr;
    }

    if (context)
        context_release(context);

    desc.Format = format->ddi_format;
    desc.Width = wined3d_texture_get_level_width(texture, level);
    desc.Height = wined3d_texture_get_level_height(texture, level);
    desc.Pitch = row_pitch;
    desc.hDeviceDc = CreateCompatibleDC(NULL);
    desc.pColorTable = NULL;

    status = D3DKMTCreateDCFromMemory(&desc);
    DeleteDC(desc.hDeviceDc);
    if (status)
    {
        WARN("Failed to create DC, status %#x.\n", status);
        return;
    }

    dc_info = &texture->dc_info[sub_resource_idx];
    dc_info->dc = desc.hDc;
    dc_info->bitmap = desc.hBitmap;

    TRACE("Created DC %p, bitmap %p for texture %p, %u.\n", dc_info->dc, dc_info->bitmap, texture, sub_resource_idx);
}

static void wined3d_texture_destroy_dc(void *object)
{
    const struct wined3d_texture_idx *idx = object;
    D3DKMT_DESTROYDCFROMMEMORY destroy_desc;
    struct wined3d_context *context;
    struct wined3d_texture *texture;
    struct wined3d_dc_info *dc_info;
    struct wined3d_bo_address data;
    struct wined3d_map_range range;
    unsigned int sub_resource_idx;
    struct wined3d_device *device;
    NTSTATUS status;

    texture = idx->texture;
    sub_resource_idx = idx->sub_resource_idx;
    device = texture->resource.device;
    dc_info = &texture->dc_info[sub_resource_idx];

    if (!dc_info->dc)
    {
        ERR("Sub-resource {%p, %u} has no DC.\n", texture, sub_resource_idx);
        return;
    }

    TRACE("dc %p, bitmap %p.\n", dc_info->dc, dc_info->bitmap);

    destroy_desc.hDc = dc_info->dc;
    destroy_desc.hBitmap = dc_info->bitmap;
    if ((status = D3DKMTDestroyDCFromMemory(&destroy_desc)))
        ERR("Failed to destroy dc, status %#x.\n", status);
    dc_info->dc = NULL;
    dc_info->bitmap = NULL;

    wined3d_texture_get_memory(texture, sub_resource_idx, &data, texture->resource.map_binding);
    if (data.buffer_object)
    {
        context = context_acquire(device, NULL, 0);
        range.offset = 0;
        range.size = texture->sub_resources[sub_resource_idx].size;
        wined3d_context_unmap_bo_address(context, &data, 0, 1, &range);
        context_release(context);
    }
}

void wined3d_texture_set_swapchain(struct wined3d_texture *texture, struct wined3d_swapchain *swapchain)
{
    texture->swapchain = swapchain;
    wined3d_resource_update_draw_binding(&texture->resource);
}

void wined3d_gl_texture_swizzle_from_color_fixup(GLint swizzle[4], struct color_fixup_desc fixup)
{
    static const GLenum swizzle_source[] =
    {
        GL_ZERO,  /* CHANNEL_SOURCE_ZERO */
        GL_ONE,   /* CHANNEL_SOURCE_ONE */
        GL_RED,   /* CHANNEL_SOURCE_X */
        GL_GREEN, /* CHANNEL_SOURCE_Y */
        GL_BLUE,  /* CHANNEL_SOURCE_Z */
        GL_ALPHA, /* CHANNEL_SOURCE_W */
    };

    swizzle[0] = swizzle_source[fixup.x_source];
    swizzle[1] = swizzle_source[fixup.y_source];
    swizzle[2] = swizzle_source[fixup.z_source];
    swizzle[3] = swizzle_source[fixup.w_source];
}

/* Context activation is done by the caller. */
void wined3d_texture_gl_bind(struct wined3d_texture_gl *texture_gl,
        struct wined3d_context_gl *context_gl, BOOL srgb)
{
    const struct wined3d_format *format = texture_gl->t.resource.format;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    const struct color_fixup_desc fixup = format->color_fixup;
    struct gl_texture *gl_tex;
    GLenum target;

    TRACE("texture_gl %p, context_gl %p, srgb %#x.\n", texture_gl, context_gl, srgb);

    if (!needs_separate_srgb_gl_texture(&context_gl->c, &texture_gl->t))
        srgb = FALSE;

    /* sRGB mode cache for preload() calls outside drawprim. */
    if (srgb)
        texture_gl->t.flags |= WINED3D_TEXTURE_IS_SRGB;
    else
        texture_gl->t.flags &= ~WINED3D_TEXTURE_IS_SRGB;

    gl_tex = wined3d_texture_gl_get_gl_texture(texture_gl, srgb);
    target = texture_gl->target;

    if (gl_tex->name)
    {
        wined3d_context_gl_bind_texture(context_gl, target, gl_tex->name);
        return;
    }

    gl_info->gl_ops.gl.p_glGenTextures(1, &gl_tex->name);
    checkGLcall("glGenTextures");
    TRACE("Generated texture %d.\n", gl_tex->name);

    if (!gl_tex->name)
    {
        ERR("Failed to generate a texture name.\n");
        return;
    }

    /* Initialise the state of the texture object to the OpenGL defaults, not
     * the wined3d defaults. */
    gl_tex->sampler_desc.address_u = WINED3D_TADDRESS_WRAP;
    gl_tex->sampler_desc.address_v = WINED3D_TADDRESS_WRAP;
    gl_tex->sampler_desc.address_w = WINED3D_TADDRESS_WRAP;
    memset(gl_tex->sampler_desc.border_color, 0, sizeof(gl_tex->sampler_desc.border_color));
    gl_tex->sampler_desc.mag_filter = WINED3D_TEXF_LINEAR;
    gl_tex->sampler_desc.min_filter = WINED3D_TEXF_POINT; /* GL_NEAREST_MIPMAP_LINEAR */
    gl_tex->sampler_desc.mip_filter = WINED3D_TEXF_LINEAR; /* GL_NEAREST_MIPMAP_LINEAR */
    gl_tex->sampler_desc.lod_bias = 0.0f;
    gl_tex->sampler_desc.min_lod = -1000.0f;
    gl_tex->sampler_desc.max_lod = 1000.0f;
    gl_tex->sampler_desc.max_anisotropy = 1;
    gl_tex->sampler_desc.compare = FALSE;
    gl_tex->sampler_desc.comparison_func = WINED3D_CMP_LESSEQUAL;
    if (gl_info->supported[EXT_TEXTURE_SRGB_DECODE])
        gl_tex->sampler_desc.srgb_decode = TRUE;
    else
        gl_tex->sampler_desc.srgb_decode = srgb;
    gl_tex->base_level = 0;
    wined3d_texture_set_dirty(&texture_gl->t);

    wined3d_context_gl_bind_texture(context_gl, target, gl_tex->name);

    /* For a new texture we have to set the texture levels after binding the
     * texture. Beware that texture rectangles do not support mipmapping, but
     * set the maxmiplevel if we're relying on the partial
     * GL_ARB_texture_non_power_of_two emulation with texture rectangles.
     * (I.e., do not care about cond_np2 here, just look for
     * GL_TEXTURE_RECTANGLE_ARB.) */
    if (target != GL_TEXTURE_RECTANGLE_ARB)
    {
        TRACE("Setting GL_TEXTURE_MAX_LEVEL to %u.\n", texture_gl->t.level_count - 1);
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, texture_gl->t.level_count - 1);
        checkGLcall("glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, texture->level_count)");
    }

    if (target == GL_TEXTURE_CUBE_MAP_ARB)
    {
        /* Cubemaps are always set to clamp, regardless of the sampler state. */
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    if (texture_gl->t.flags & WINED3D_TEXTURE_COND_NP2)
    {
        /* Conditinal non power of two textures use a different clamping
         * default. If we're using the GL_WINE_normalized_texrect partial
         * driver emulation, we're dealing with a GL_TEXTURE_2D texture which
         * has the address mode set to repeat - something that prevents us
         * from hitting the accelerated codepath. Thus manually set the GL
         * state. The same applies to filtering. Even if the texture has only
         * one mip level, the default LINEAR_MIPMAP_LINEAR filter causes a SW
         * fallback on macos. */
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        checkGLcall("glTexParameteri");
        gl_tex->sampler_desc.address_u = WINED3D_TADDRESS_CLAMP;
        gl_tex->sampler_desc.address_v = WINED3D_TADDRESS_CLAMP;
        gl_tex->sampler_desc.mag_filter = WINED3D_TEXF_POINT;
        gl_tex->sampler_desc.min_filter = WINED3D_TEXF_POINT;
        gl_tex->sampler_desc.mip_filter = WINED3D_TEXF_NONE;
    }

    if (gl_info->supported[WINED3D_GL_LEGACY_CONTEXT] && gl_info->supported[ARB_DEPTH_TEXTURE])
    {
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_DEPTH_TEXTURE_MODE_ARB, GL_INTENSITY);
        checkGLcall("glTexParameteri(GL_DEPTH_TEXTURE_MODE_ARB, GL_INTENSITY)");
    }

    if (!is_identity_fixup(fixup) && can_use_texture_swizzle(context_gl->c.d3d_info, format))
    {
        GLint swizzle[4];

        wined3d_gl_texture_swizzle_from_color_fixup(swizzle, fixup);
        gl_info->gl_ops.gl.p_glTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, swizzle);
        checkGLcall("set format swizzle");
    }
}

/* Context activation is done by the caller. */
void wined3d_texture_gl_bind_and_dirtify(struct wined3d_texture_gl *texture_gl,
        struct wined3d_context_gl *context_gl, BOOL srgb)
{
    /* We don't need a specific texture unit, but after binding the texture
     * the current unit is dirty. Read the unit back instead of switching to
     * 0, this avoids messing around with the state manager's GL states. The
     * current texture unit should always be a valid one.
     *
     * To be more specific, this is tricky because we can implicitly be
     * called from sampler() in state.c. This means we can't touch anything
     * other than whatever happens to be the currently active texture, or we
     * would risk marking already applied sampler states dirty again. */
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

    wined3d_texture_gl_bind(texture_gl, context_gl, srgb);
}

/* Context activation is done by the caller (state handler). */
/* This function relies on the correct texture being bound and loaded. */
void wined3d_texture_gl_apply_sampler_desc(struct wined3d_texture_gl *texture_gl,
        const struct wined3d_sampler_desc *sampler_desc, const struct wined3d_context_gl *context_gl)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    GLenum target = texture_gl->target;
    struct gl_texture *gl_tex;
    DWORD state;

    TRACE("texture_gl %p, sampler_desc %p, context_gl %p.\n", texture_gl, sampler_desc, context_gl);

    gl_tex = wined3d_texture_gl_get_gl_texture(texture_gl, texture_gl->t.flags & WINED3D_TEXTURE_IS_SRGB);

    state = sampler_desc->address_u;
    if (state != gl_tex->sampler_desc.address_u)
    {
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_S,
                gl_info->wrap_lookup[state - WINED3D_TADDRESS_WRAP]);
        gl_tex->sampler_desc.address_u = state;
    }

    state = sampler_desc->address_v;
    if (state != gl_tex->sampler_desc.address_v)
    {
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_T,
                gl_info->wrap_lookup[state - WINED3D_TADDRESS_WRAP]);
        gl_tex->sampler_desc.address_v = state;
    }

    state = sampler_desc->address_w;
    if (state != gl_tex->sampler_desc.address_w)
    {
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_R,
                gl_info->wrap_lookup[state - WINED3D_TADDRESS_WRAP]);
        gl_tex->sampler_desc.address_w = state;
    }

    if (memcmp(gl_tex->sampler_desc.border_color, sampler_desc->border_color,
            sizeof(gl_tex->sampler_desc.border_color)))
    {
        gl_info->gl_ops.gl.p_glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, &sampler_desc->border_color[0]);
        memcpy(gl_tex->sampler_desc.border_color, sampler_desc->border_color,
                sizeof(gl_tex->sampler_desc.border_color));
    }

    state = sampler_desc->mag_filter;
    if (state != gl_tex->sampler_desc.mag_filter)
    {
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_MAG_FILTER, wined3d_gl_mag_filter(state));
        gl_tex->sampler_desc.mag_filter = state;
    }

    if (sampler_desc->min_filter != gl_tex->sampler_desc.min_filter
            || sampler_desc->mip_filter != gl_tex->sampler_desc.mip_filter)
    {
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_MIN_FILTER,
                wined3d_gl_min_mip_filter(sampler_desc->min_filter, sampler_desc->mip_filter));
        gl_tex->sampler_desc.min_filter = sampler_desc->min_filter;
        gl_tex->sampler_desc.mip_filter = sampler_desc->mip_filter;
    }

    state = sampler_desc->max_anisotropy;
    if (state != gl_tex->sampler_desc.max_anisotropy)
    {
        if (gl_info->supported[ARB_TEXTURE_FILTER_ANISOTROPIC])
            gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_MAX_ANISOTROPY, state);
        else
            WARN("Anisotropic filtering not supported.\n");
        gl_tex->sampler_desc.max_anisotropy = state;
    }

    if (!sampler_desc->srgb_decode != !gl_tex->sampler_desc.srgb_decode
            && (context_gl->c.d3d_info->wined3d_creation_flags & WINED3D_SRGB_READ_WRITE_CONTROL)
            && gl_info->supported[EXT_TEXTURE_SRGB_DECODE])
    {
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_SRGB_DECODE_EXT,
                sampler_desc->srgb_decode ? GL_DECODE_EXT : GL_SKIP_DECODE_EXT);
        gl_tex->sampler_desc.srgb_decode = sampler_desc->srgb_decode;
    }

    if (!sampler_desc->compare != !gl_tex->sampler_desc.compare)
    {
        if (sampler_desc->compare)
            gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
        else
            gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
        gl_tex->sampler_desc.compare = sampler_desc->compare;
    }

    checkGLcall("Texture parameter application");

    if (gl_info->supported[EXT_TEXTURE_LOD_BIAS])
    {
        gl_info->gl_ops.gl.p_glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT,
                GL_TEXTURE_LOD_BIAS_EXT, sampler_desc->lod_bias);
        checkGLcall("glTexEnvf(GL_TEXTURE_LOD_BIAS_EXT, ...)");
    }
}

ULONG CDECL wined3d_texture_incref(struct wined3d_texture *texture)
{
    ULONG refcount;

    TRACE("texture %p, swapchain %p.\n", texture, texture->swapchain);

    if (texture->swapchain)
        return wined3d_swapchain_incref(texture->swapchain);

    refcount = InterlockedIncrement(&texture->resource.ref);
    TRACE("%p increasing refcount to %u.\n", texture, refcount);

    return refcount;
}

static void wined3d_texture_destroy_object(void *object)
{
    const struct wined3d_gl_info *gl_info = NULL;
    struct wined3d_texture *texture = object;
    struct wined3d_context *context = NULL;
    struct wined3d_dc_info *dc_info;
    unsigned int sub_count;
    GLuint buffer_object;
    unsigned int i;

    TRACE("texture %p.\n", texture);

    sub_count = texture->level_count * texture->layer_count;
    for (i = 0; i < sub_count; ++i)
    {
        if (!(buffer_object = texture->sub_resources[i].buffer_object))
            continue;

        TRACE("Deleting buffer object %u.\n", buffer_object);

        /* We may not be able to get a context in
         * wined3d_texture_destroy_object() in general, but if a buffer object
         * was previously created we can. */
        if (!context)
        {
            context = context_acquire(texture->resource.device, NULL, 0);
            gl_info = wined3d_context_gl(context)->gl_info;
        }

        GL_EXTCALL(glDeleteBuffers(1, &buffer_object));
    }

    if (context)
        context_release(context);

    if ((dc_info = texture->dc_info))
    {
        for (i = 0; i < sub_count; ++i)
        {
            if (dc_info[i].dc)
            {
                struct wined3d_texture_idx texture_idx = {texture, i};

                wined3d_texture_destroy_dc(&texture_idx);
            }
        }
        heap_free(dc_info);
    }

    if (texture->overlay_info)
    {
        for (i = 0; i < sub_count; ++i)
        {
            struct wined3d_overlay_info *info = &texture->overlay_info[i];
            struct wined3d_overlay_info *overlay, *cur;

            list_remove(&info->entry);
            LIST_FOR_EACH_ENTRY_SAFE(overlay, cur, &info->overlays, struct wined3d_overlay_info, entry)
            {
                list_remove(&overlay->entry);
            }
        }
        heap_free(texture->overlay_info);
    }
}

void wined3d_texture_cleanup(struct wined3d_texture *texture)
{
    resource_cleanup(&texture->resource);
    wined3d_cs_destroy_object(texture->resource.device->cs, wined3d_texture_destroy_object, texture);
}

static void wined3d_texture_cleanup_sync(struct wined3d_texture *texture)
{
    wined3d_texture_sub_resources_destroyed(texture);
    wined3d_texture_cleanup(texture);
    wined3d_resource_wait_idle(&texture->resource);
}

ULONG CDECL wined3d_texture_decref(struct wined3d_texture *texture)
{
    ULONG refcount;

    TRACE("texture %p, swapchain %p.\n", texture, texture->swapchain);

    if (texture->swapchain)
        return wined3d_swapchain_decref(texture->swapchain);

    refcount = InterlockedDecrement(&texture->resource.ref);
    TRACE("%p decreasing refcount to %u.\n", texture, refcount);

    if (!refcount)
    {
        /* Wait for the texture to become idle if it's using user memory,
         * since the application is allowed to free that memory once the
         * texture is destroyed. Note that this implies that
         * the destroy handler can't access that memory either. */
        if (texture->user_memory)
            wined3d_resource_wait_idle(&texture->resource);
        texture->resource.device->adapter->adapter_ops->adapter_destroy_texture(texture);
    }

    return refcount;
}

struct wined3d_resource * CDECL wined3d_texture_get_resource(struct wined3d_texture *texture)
{
    TRACE("texture %p.\n", texture);

    return &texture->resource;
}

static BOOL color_key_equal(const struct wined3d_color_key *c1, struct wined3d_color_key *c2)
{
    return c1->color_space_low_value == c2->color_space_low_value
            && c1->color_space_high_value == c2->color_space_high_value;
}

/* Context activation is done by the caller */
void wined3d_texture_load(struct wined3d_texture *texture,
        struct wined3d_context *context, BOOL srgb)
{
    UINT sub_count = texture->level_count * texture->layer_count;
    const struct wined3d_d3d_info *d3d_info = context->d3d_info;
    DWORD flag;
    UINT i;

    TRACE("texture %p, context %p, srgb %#x.\n", texture, context, srgb);

    if (!needs_separate_srgb_gl_texture(context, texture))
        srgb = FALSE;

    if (srgb)
        flag = WINED3D_TEXTURE_SRGB_VALID;
    else
        flag = WINED3D_TEXTURE_RGB_VALID;

    if (!d3d_info->shader_color_key
            && (!(texture->async.flags & WINED3D_TEXTURE_ASYNC_COLOR_KEY)
            != !(texture->async.color_key_flags & WINED3D_CKEY_SRC_BLT)
            || (texture->async.flags & WINED3D_TEXTURE_ASYNC_COLOR_KEY
            && !color_key_equal(&texture->async.gl_color_key, &texture->async.src_blt_color_key))))
    {
        unsigned int sub_count = texture->level_count * texture->layer_count;
        unsigned int i;

        TRACE("Reloading because of color key value change.\n");
        for (i = 0; i < sub_count; i++)
        {
            if (!wined3d_texture_load_location(texture, i, context, texture->resource.map_binding))
                ERR("Failed to load location %s.\n", wined3d_debug_location(texture->resource.map_binding));
            else
                wined3d_texture_invalidate_location(texture, i, ~texture->resource.map_binding);
        }

        texture->async.gl_color_key = texture->async.src_blt_color_key;
    }

    if (texture->flags & flag)
    {
        TRACE("Texture %p not dirty, nothing to do.\n", texture);
        return;
    }

    /* Reload the surfaces if the texture is marked dirty. */
    for (i = 0; i < sub_count; ++i)
    {
        if (!wined3d_texture_load_location(texture, i, context,
                srgb ? WINED3D_LOCATION_TEXTURE_SRGB : WINED3D_LOCATION_TEXTURE_RGB))
            ERR("Failed to load location (srgb %#x).\n", srgb);
    }
    texture->flags |= flag;
}

void * CDECL wined3d_texture_get_parent(const struct wined3d_texture *texture)
{
    TRACE("texture %p.\n", texture);

    return texture->resource.parent;
}

HRESULT wined3d_texture_check_box_dimensions(const struct wined3d_texture *texture,
        unsigned int level, const struct wined3d_box *box)
{
    const struct wined3d_format *format = texture->resource.format;
    unsigned int width_mask, height_mask, width, height, depth;

    width = wined3d_texture_get_level_width(texture, level);
    height = wined3d_texture_get_level_height(texture, level);
    depth = wined3d_texture_get_level_depth(texture, level);

    if (box->left >= box->right || box->right > width
            || box->top >= box->bottom || box->bottom > height
            || box->front >= box->back || box->back > depth)
    {
        WARN("Box %s is invalid.\n", debug_box(box));
        return WINEDDERR_INVALIDRECT;
    }

    if (texture->resource.format_flags & WINED3DFMT_FLAG_BLOCKS)
    {
        /* This assumes power of two block sizes, but NPOT block sizes would
         * be silly anyway.
         *
         * This also assumes that the format's block depth is 1. */
        width_mask = format->block_width - 1;
        height_mask = format->block_height - 1;

        if ((box->left & width_mask) || (box->top & height_mask)
                || (box->right & width_mask && box->right != width)
                || (box->bottom & height_mask && box->bottom != height))
        {
            WARN("Box %s is misaligned for %ux%u blocks.\n",
                    debug_box(box), format->block_width, format->block_height);
            return WINED3DERR_INVALIDCALL;
        }
    }

    return WINED3D_OK;
}

void CDECL wined3d_texture_get_pitch(const struct wined3d_texture *texture,
        unsigned int level, unsigned int *row_pitch, unsigned int *slice_pitch)
{
    const struct wined3d_resource *resource = &texture->resource;
    unsigned int width = wined3d_texture_get_level_width(texture, level);
    unsigned int height = wined3d_texture_get_level_height(texture, level);

    if (texture->row_pitch)
    {
        *row_pitch = texture->row_pitch;
        *slice_pitch = texture->slice_pitch;
        return;
    }

    wined3d_format_calculate_pitch(resource->format, resource->device->surface_alignment,
            width, height, row_pitch, slice_pitch);
}

DWORD CDECL wined3d_texture_set_lod(struct wined3d_texture *texture, DWORD lod)
{
    struct wined3d_resource *resource;
    DWORD old = texture->lod;

    TRACE("texture %p, lod %u.\n", texture, lod);

    /* The d3d9:texture test shows that SetLOD is ignored on non-managed
     * textures. The call always returns 0, and GetLOD always returns 0. */
    resource = &texture->resource;
    if (!wined3d_resource_access_is_managed(resource->access))
    {
        TRACE("Ignoring LOD on texture with resource access %s.\n",
                wined3d_debug_resource_access(resource->access));
        return 0;
    }

    if (lod >= texture->level_count)
        lod = texture->level_count - 1;

    if (texture->lod != lod)
    {
        struct wined3d_device *device = resource->device;

        wined3d_resource_wait_idle(resource);
        texture->lod = lod;

        wined3d_texture_gl(texture)->texture_rgb.base_level = ~0u;
        wined3d_texture_gl(texture)->texture_srgb.base_level = ~0u;
        if (resource->bind_count)
            wined3d_cs_emit_set_sampler_state(device->cs, texture->sampler, WINED3D_SAMP_MAX_MIP_LEVEL,
                    device->state.sampler_states[texture->sampler][WINED3D_SAMP_MAX_MIP_LEVEL]);
    }

    return old;
}

DWORD CDECL wined3d_texture_get_lod(const struct wined3d_texture *texture)
{
    TRACE("texture %p, returning %u.\n", texture, texture->lod);

    return texture->lod;
}

DWORD CDECL wined3d_texture_get_level_count(const struct wined3d_texture *texture)
{
    TRACE("texture %p, returning %u.\n", texture, texture->level_count);

    return texture->level_count;
}

HRESULT CDECL wined3d_texture_set_color_key(struct wined3d_texture *texture,
        DWORD flags, const struct wined3d_color_key *color_key)
{
    struct wined3d_device *device = texture->resource.device;
    static const DWORD all_flags = WINED3D_CKEY_DST_BLT | WINED3D_CKEY_DST_OVERLAY
            | WINED3D_CKEY_SRC_BLT | WINED3D_CKEY_SRC_OVERLAY;

    TRACE("texture %p, flags %#x, color_key %p.\n", texture, flags, color_key);

    if (flags & ~all_flags)
    {
        WARN("Invalid flags passed, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    wined3d_cs_emit_set_color_key(device->cs, texture, flags, color_key);

    return WINED3D_OK;
}

/* In D3D the depth stencil dimensions have to be greater than or equal to the
 * render target dimensions. With FBOs, the dimensions have to be an exact match. */
/* TODO: We should synchronize the renderbuffer's content with the texture's content. */
/* Context activation is done by the caller. */
void wined3d_texture_gl_set_compatible_renderbuffer(struct wined3d_texture_gl *texture_gl,
        struct wined3d_context_gl *context_gl, unsigned int level, const struct wined3d_rendertarget_info *rt)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_renderbuffer_entry *entry;
    unsigned int src_width, src_height;
    unsigned int width, height;
    GLuint renderbuffer = 0;

    if (gl_info->supported[ARB_FRAMEBUFFER_OBJECT])
        return;

    if (rt && rt->resource->format->id != WINED3DFMT_NULL)
    {
        struct wined3d_texture *rt_texture;
        unsigned int rt_level;

        if (rt->resource->type == WINED3D_RTYPE_BUFFER)
        {
            FIXME("Unsupported resource type %s.\n", debug_d3dresourcetype(rt->resource->type));
            return;
        }
        rt_texture = wined3d_texture_from_resource(rt->resource);
        rt_level = rt->sub_resource_idx % rt_texture->level_count;

        width = wined3d_texture_get_level_pow2_width(rt_texture, rt_level);
        height = wined3d_texture_get_level_pow2_height(rt_texture, rt_level);
    }
    else
    {
        width = wined3d_texture_get_level_pow2_width(&texture_gl->t, level);
        height = wined3d_texture_get_level_pow2_height(&texture_gl->t, level);
    }

    src_width = wined3d_texture_get_level_pow2_width(&texture_gl->t, level);
    src_height = wined3d_texture_get_level_pow2_height(&texture_gl->t, level);

    /* A depth stencil smaller than the render target is not valid */
    if (width > src_width || height > src_height)
        return;

    /* Remove any renderbuffer set if the sizes match */
    if (width == src_width && height == src_height)
    {
        texture_gl->current_renderbuffer = NULL;
        return;
    }

    /* Look if we've already got a renderbuffer of the correct dimensions */
    LIST_FOR_EACH_ENTRY(entry, &texture_gl->renderbuffers, struct wined3d_renderbuffer_entry, entry)
    {
        if (entry->width == width && entry->height == height)
        {
            renderbuffer = entry->id;
            texture_gl->current_renderbuffer = entry;
            break;
        }
    }

    if (!renderbuffer)
    {
        const struct wined3d_format_gl *format_gl;

        format_gl = wined3d_format_gl(texture_gl->t.resource.format);
        gl_info->fbo_ops.glGenRenderbuffers(1, &renderbuffer);
        gl_info->fbo_ops.glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
        gl_info->fbo_ops.glRenderbufferStorage(GL_RENDERBUFFER, format_gl->internal, width, height);

        entry = heap_alloc(sizeof(*entry));
        entry->width = width;
        entry->height = height;
        entry->id = renderbuffer;
        list_add_head(&texture_gl->renderbuffers, &entry->entry);

        texture_gl->current_renderbuffer = entry;
    }

    checkGLcall("set compatible renderbuffer");
}

HRESULT CDECL wined3d_texture_update_desc(struct wined3d_texture *texture, UINT width, UINT height,
        enum wined3d_format_id format_id, enum wined3d_multisample_type multisample_type,
        UINT multisample_quality, void *mem, UINT pitch)
{
    struct wined3d_texture_sub_resource *sub_resource;
    const struct wined3d_d3d_info *d3d_info;
    const struct wined3d_gl_info *gl_info;
    const struct wined3d_format *format;
    const struct wined3d *d3d;
    struct wined3d_device *device;
    unsigned int resource_size;
    DWORD valid_location = 0;
    BOOL create_dib = FALSE;

    TRACE("texture %p, width %u, height %u, format %s, multisample_type %#x, multisample_quality %u, "
            "mem %p, pitch %u.\n",
            texture, width, height, debug_d3dformat(format_id), multisample_type, multisample_quality, mem, pitch);

    device = texture->resource.device;
    d3d = device->wined3d;
    gl_info = &device->adapter->gl_info;
    d3d_info = &device->adapter->d3d_info;
    format = wined3d_get_format(device->adapter, format_id, texture->resource.bind_flags);
    resource_size = wined3d_format_calculate_size(format, device->surface_alignment, width, height, 1);

    if (!resource_size)
        return WINED3DERR_INVALIDCALL;

    if (texture->level_count * texture->layer_count > 1)
    {
        WARN("Texture has multiple sub-resources, not supported.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (texture->resource.type != WINED3D_RTYPE_TEXTURE_2D)
    {
        WARN("Not supported on %s.\n", debug_d3dresourcetype(texture->resource.type));
        return WINED3DERR_INVALIDCALL;
    }

    if (texture->resource.map_count)
    {
        WARN("Texture is mapped.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* We have no way of supporting a pitch that is not a multiple of the pixel
     * byte width short of uploading the texture row-by-row.
     * Fortunately that's not an issue since D3D9Ex doesn't allow a custom pitch
     * for user-memory textures (it always expects packed data) while DirectDraw
     * requires a 4-byte aligned pitch and doesn't support texture formats
     * larger than 4 bytes per pixel nor any format using 3 bytes per pixel.
     * This check is here to verify that the assumption holds. */
    if (pitch % texture->resource.format->byte_count)
    {
        WARN("Pitch unsupported, not a multiple of the texture format byte width.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (device->d3d_initialized)
        wined3d_cs_emit_unload_resource(device->cs, &texture->resource);
    wined3d_resource_wait_idle(&texture->resource);

    sub_resource = &texture->sub_resources[0];
    if (texture->dc_info && texture->dc_info[0].dc)
    {
        struct wined3d_texture_idx texture_idx = {texture, 0};

        wined3d_cs_destroy_object(device->cs, wined3d_texture_destroy_dc, &texture_idx);
        wined3d_cs_finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);
        create_dib = TRUE;
    }

    wined3d_resource_free_sysmem(&texture->resource);

    if ((texture->row_pitch = pitch))
        texture->slice_pitch = height * pitch;
    else
        /* User memory surfaces don't have the regular surface alignment. */
        wined3d_format_calculate_pitch(format, 1, width, height,
                &texture->row_pitch, &texture->slice_pitch);

    texture->resource.format = format;
    texture->resource.multisample_type = multisample_type;
    texture->resource.multisample_quality = multisample_quality;
    texture->resource.width = width;
    texture->resource.height = height;
    if (!(texture->resource.access & WINED3D_RESOURCE_ACCESS_CPU) && d3d->flags & WINED3D_VIDMEM_ACCOUNTING)
        adapter_adjust_memory(device->adapter,  (INT64)texture->slice_pitch - texture->resource.size);
    texture->resource.size = texture->slice_pitch;
    sub_resource->size = texture->slice_pitch;
    sub_resource->locations = WINED3D_LOCATION_DISCARDED;

    if (texture->texture_ops == &texture_gl_ops)
    {
        if (multisample_type && gl_info->supported[ARB_TEXTURE_MULTISAMPLE])
        {
            wined3d_texture_gl(texture)->target = GL_TEXTURE_2D_MULTISAMPLE;
            texture->flags &= ~WINED3D_TEXTURE_DOWNLOADABLE;
        }
        else
        {
            wined3d_texture_gl(texture)->target = GL_TEXTURE_2D;
            texture->flags |= WINED3D_TEXTURE_DOWNLOADABLE;
        }
    }

    if (((width & (width - 1)) || (height & (height - 1))) && !d3d_info->texture_npot
            && !d3d_info->texture_npot_conditional)
    {
        texture->flags |= WINED3D_TEXTURE_COND_NP2_EMULATED;
        texture->pow2_width = texture->pow2_height = 1;
        while (texture->pow2_width < width)
            texture->pow2_width <<= 1;
        while (texture->pow2_height < height)
            texture->pow2_height <<= 1;
    }
    else
    {
        texture->flags &= ~WINED3D_TEXTURE_COND_NP2_EMULATED;
        texture->pow2_width = width;
        texture->pow2_height = height;
    }

    if ((texture->user_memory = mem))
    {
        texture->resource.map_binding = WINED3D_LOCATION_USER_MEMORY;
        valid_location = WINED3D_LOCATION_USER_MEMORY;
    }
    else
    {
        if (!wined3d_resource_prepare_sysmem(&texture->resource))
            ERR("Failed to allocate resource memory.\n");
        valid_location = WINED3D_LOCATION_SYSMEM;
    }

    /* The format might be changed to a format that needs conversion.
     * If the surface didn't use PBOs previously but could now, don't
     * change it - whatever made us not use PBOs might come back, e.g.
     * color keys. */
    if (texture->resource.map_binding == WINED3D_LOCATION_BUFFER && !wined3d_texture_use_pbo(texture, gl_info))
        texture->resource.map_binding = WINED3D_LOCATION_SYSMEM;

    wined3d_texture_validate_location(texture, 0, valid_location);
    wined3d_texture_invalidate_location(texture, 0, ~valid_location);

    if (create_dib)
    {
        struct wined3d_texture_idx texture_idx = {texture, 0};

        wined3d_cs_init_object(device->cs, wined3d_texture_create_dc, &texture_idx);
        wined3d_cs_finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);
    }

    return WINED3D_OK;
}

/* Context activation is done by the caller. */
static void wined3d_texture_prepare_buffer_object(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, const struct wined3d_gl_info *gl_info)
{
    struct wined3d_texture_sub_resource *sub_resource;

    sub_resource = &texture->sub_resources[sub_resource_idx];
    if (sub_resource->buffer_object)
        return;

    GL_EXTCALL(glGenBuffers(1, &sub_resource->buffer_object));
    GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, sub_resource->buffer_object));
    GL_EXTCALL(glBufferData(GL_PIXEL_UNPACK_BUFFER, sub_resource->size, NULL, GL_STREAM_DRAW));
    GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
    checkGLcall("Create buffer object");

    TRACE("Created buffer object %u for texture %p, sub-resource %u.\n",
            sub_resource->buffer_object, texture, sub_resource_idx);
}

static void wined3d_texture_force_reload(struct wined3d_texture *texture)
{
    unsigned int sub_count = texture->level_count * texture->layer_count;
    unsigned int i;

    texture->flags &= ~(WINED3D_TEXTURE_RGB_ALLOCATED | WINED3D_TEXTURE_SRGB_ALLOCATED
            | WINED3D_TEXTURE_CONVERTED);
    texture->async.flags &= ~WINED3D_TEXTURE_ASYNC_COLOR_KEY;
    for (i = 0; i < sub_count; ++i)
    {
        wined3d_texture_invalidate_location(texture, i,
                WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB);
    }
}

/* Context activation is done by the caller. */
void wined3d_texture_gl_prepare_texture(struct wined3d_texture_gl *texture_gl,
        struct wined3d_context_gl *context_gl, BOOL srgb)
{
    DWORD alloc_flag = srgb ? WINED3D_TEXTURE_SRGB_ALLOCATED : WINED3D_TEXTURE_RGB_ALLOCATED;
    const struct wined3d_d3d_info *d3d_info = context_gl->c.d3d_info;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_resource *resource = &texture_gl->t.resource;
    const struct wined3d_device *device = resource->device;
    const struct wined3d_format *format = resource->format;
    const struct wined3d_color_key_conversion *conversion;
    const struct wined3d_format_gl *format_gl;
    GLenum internal;

    TRACE("texture_gl %p, context_gl %p, format %s.\n", texture_gl, context_gl, debug_d3dformat(format->id));

    if (!d3d_info->shader_color_key
            && !(texture_gl->t.async.flags & WINED3D_TEXTURE_ASYNC_COLOR_KEY)
            != !(texture_gl->t.async.color_key_flags & WINED3D_CKEY_SRC_BLT))
    {
        wined3d_texture_force_reload(&texture_gl->t);

        if (texture_gl->t.async.color_key_flags & WINED3D_CKEY_SRC_BLT)
            texture_gl->t.async.flags |= WINED3D_TEXTURE_ASYNC_COLOR_KEY;
    }

    if (texture_gl->t.flags & alloc_flag)
        return;

    if (resource->format_flags & WINED3DFMT_FLAG_DECOMPRESS)
    {
        TRACE("WINED3DFMT_FLAG_DECOMPRESS set.\n");
        texture_gl->t.flags |= WINED3D_TEXTURE_CONVERTED;
        format = wined3d_resource_get_decompress_format(resource);
    }
    else if (format->conv_byte_count)
    {
        texture_gl->t.flags |= WINED3D_TEXTURE_CONVERTED;
    }
    else if ((conversion = wined3d_format_get_color_key_conversion(&texture_gl->t, TRUE)))
    {
        texture_gl->t.flags |= WINED3D_TEXTURE_CONVERTED;
        format = wined3d_get_format(device->adapter, conversion->dst_format, resource->bind_flags);
        TRACE("Using format %s for color key conversion.\n", debug_d3dformat(format->id));
    }
    format_gl = wined3d_format_gl(format);

    wined3d_texture_gl_bind_and_dirtify(texture_gl, context_gl, srgb);

    if (srgb)
        internal = format_gl->srgb_internal;
    else if (resource->bind_flags & WINED3D_BIND_RENDER_TARGET && wined3d_resource_is_offscreen(resource))
        internal = format_gl->rt_internal;
    else
        internal = format_gl->internal;

    if (!internal)
        FIXME("No GL internal format for format %s.\n", debug_d3dformat(format->id));

    TRACE("internal %#x, format %#x, type %#x.\n", internal, format_gl->format, format_gl->type);

    if (wined3d_texture_use_immutable_storage(&texture_gl->t, gl_info))
        wined3d_texture_gl_allocate_immutable_storage(texture_gl, internal, gl_info);
    else
        wined3d_texture_gl_allocate_mutable_storage(texture_gl, internal, format_gl, gl_info);
    texture_gl->t.flags |= alloc_flag;
}

static void wined3d_texture_gl_prepare_rb(struct wined3d_texture_gl *texture_gl,
        const struct wined3d_gl_info *gl_info, BOOL multisample)
{
    const struct wined3d_format_gl *format_gl;

    format_gl = wined3d_format_gl(texture_gl->t.resource.format);
    if (multisample)
    {
        DWORD samples;

        if (texture_gl->rb_multisample)
            return;

        samples = wined3d_resource_get_sample_count(&texture_gl->t.resource);

        gl_info->fbo_ops.glGenRenderbuffers(1, &texture_gl->rb_multisample);
        gl_info->fbo_ops.glBindRenderbuffer(GL_RENDERBUFFER, texture_gl->rb_multisample);
        gl_info->fbo_ops.glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples,
                format_gl->internal, texture_gl->t.resource.width, texture_gl->t.resource.height);
        checkGLcall("glRenderbufferStorageMultisample()");
        TRACE("Created multisample rb %u.\n", texture_gl->rb_multisample);
    }
    else
    {
        if (texture_gl->rb_resolved)
            return;

        gl_info->fbo_ops.glGenRenderbuffers(1, &texture_gl->rb_resolved);
        gl_info->fbo_ops.glBindRenderbuffer(GL_RENDERBUFFER, texture_gl->rb_resolved);
        gl_info->fbo_ops.glRenderbufferStorage(GL_RENDERBUFFER, format_gl->internal,
                texture_gl->t.resource.width, texture_gl->t.resource.height);
        checkGLcall("glRenderbufferStorage()");
        TRACE("Created resolved rb %u.\n", texture_gl->rb_resolved);
    }
}

BOOL wined3d_texture_prepare_location(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_context *context, unsigned int location)
{
    return texture->texture_ops->texture_prepare_location(texture, sub_resource_idx, context, location);
}

static struct wined3d_texture_sub_resource *wined3d_texture_get_sub_resource(struct wined3d_texture *texture,
        unsigned int sub_resource_idx)
{
    UINT sub_count = texture->level_count * texture->layer_count;

    TRACE("texture %p, sub_resource_idx %u.\n", texture, sub_resource_idx);

    if (sub_resource_idx >= sub_count)
    {
        WARN("sub_resource_idx %u >= sub_count %u.\n", sub_resource_idx, sub_count);
        return NULL;
    }

    return &texture->sub_resources[sub_resource_idx];
}

HRESULT CDECL wined3d_texture_add_dirty_region(struct wined3d_texture *texture,
        UINT layer, const struct wined3d_box *dirty_region)
{
    TRACE("texture %p, layer %u, dirty_region %s.\n", texture, layer, debug_box(dirty_region));

    if (layer >= texture->layer_count)
    {
        WARN("Invalid layer %u specified.\n", layer);
        return WINED3DERR_INVALIDCALL;
    }

    if (dirty_region)
        WARN("Ignoring dirty_region %s.\n", debug_box(dirty_region));

    wined3d_cs_emit_add_dirty_texture_region(texture->resource.device->cs, texture, layer);

    return WINED3D_OK;
}

static void wined3d_texture_gl_upload_bo(const struct wined3d_format *src_format, GLenum target,
        unsigned int level, unsigned int src_row_pitch, unsigned int dst_x, unsigned int dst_y,
        unsigned int dst_z, unsigned int update_w, unsigned int update_h, unsigned int update_d,
        const BYTE *addr, BOOL srgb, struct wined3d_texture *dst_texture,
        const struct wined3d_gl_info *gl_info)
{
    const struct wined3d_format_gl *format_gl = wined3d_format_gl(src_format);

    if (src_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_COMPRESSED)
    {
        unsigned int dst_row_pitch, dst_slice_pitch;
        GLenum internal;

        if (srgb)
            internal = format_gl->srgb_internal;
        else if (dst_texture->resource.bind_flags & WINED3D_BIND_RENDER_TARGET
                && wined3d_resource_is_offscreen(&dst_texture->resource))
            internal = format_gl->rt_internal;
        else
            internal = format_gl->internal;

        wined3d_format_calculate_pitch(src_format, 1, update_w, update_h, &dst_row_pitch, &dst_slice_pitch);

        TRACE("Uploading compressed data, target %#x, level %u, x %u, y %u, z %u, "
                "w %u, h %u, d %u, format %#x, image_size %#x, addr %p.\n",
                target, level, dst_x, dst_y, dst_z, update_w, update_h,
                update_d, internal, dst_slice_pitch, addr);

        if (target == GL_TEXTURE_1D)
        {
            GL_EXTCALL(glCompressedTexSubImage1D(target, level, dst_x,
                    update_w, internal, dst_row_pitch, addr));
        }
        else if (dst_row_pitch == src_row_pitch)
        {
            if (target == GL_TEXTURE_2D_ARRAY || target == GL_TEXTURE_3D)
            {
                GL_EXTCALL(glCompressedTexSubImage3D(target, level, dst_x, dst_y, dst_z,
                        update_w, update_h, update_d, internal, dst_slice_pitch * update_d, addr));
            }
            else
            {
                GL_EXTCALL(glCompressedTexSubImage2D(target, level, dst_x, dst_y,
                        update_w, update_h, internal, dst_slice_pitch, addr));
            }
        }
        else
        {
            unsigned int row_count = (update_h + src_format->block_height - 1) / src_format->block_height;
            unsigned int row, y, z;

            /* glCompressedTexSubImage2D() ignores pixel store state, so we
             * can't use the unpack row length like for glTexSubImage2D. */
            for (z = dst_z; z < dst_z + update_d; ++z)
            {
                for (row = 0, y = dst_y; row < row_count; ++row)
                {
                    if (target == GL_TEXTURE_2D_ARRAY || target == GL_TEXTURE_3D)
                    {
                        GL_EXTCALL(glCompressedTexSubImage3D(target, level, dst_x, y, z,
                                update_w, src_format->block_height, 1, internal, dst_row_pitch, addr));
                    }
                    else
                    {
                        GL_EXTCALL(glCompressedTexSubImage2D(target, level, dst_x, y,
                                update_w, src_format->block_height, internal, dst_row_pitch, addr));
                    }

                    y += src_format->block_height;
                    addr += src_row_pitch;
                }
            }
        }
        checkGLcall("Upload compressed texture data");
    }
    else
    {
        unsigned int y, y_count;

        TRACE("Uploading data, target %#x, level %u, x %u, y %u, z %u, "
                "w %u, h %u, d %u, format %#x, type %#x, addr %p.\n",
                target, level, dst_x, dst_y, dst_z, update_w, update_h,
                update_d, format_gl->format, format_gl->type, addr);

        if (src_row_pitch)
        {
            gl_info->gl_ops.gl.p_glPixelStorei(GL_UNPACK_ROW_LENGTH, src_row_pitch / src_format->byte_count);
            y_count = 1;
        }
        else
        {
            y_count = update_h;
            update_h = 1;
        }

        for (y = 0; y < y_count; ++y)
        {
            if (target == GL_TEXTURE_2D_ARRAY || target == GL_TEXTURE_3D)
            {
                GL_EXTCALL(glTexSubImage3D(target, level, dst_x, dst_y + y, dst_z,
                        update_w, update_h, update_d, format_gl->format, format_gl->type, addr));
            }
            else if (target == GL_TEXTURE_1D)
            {
                gl_info->gl_ops.gl.p_glTexSubImage1D(target, level, dst_x,
                        update_w, format_gl->format, format_gl->type, addr);
            }
            else
            {
                gl_info->gl_ops.gl.p_glTexSubImage2D(target, level, dst_x, dst_y + y,
                        update_w, update_h, format_gl->format, format_gl->type, addr);
            }
        }
        gl_info->gl_ops.gl.p_glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        checkGLcall("Upload texture data");
    }
}

static void wined3d_texture_gl_upload_data(struct wined3d_context *context,
        const struct wined3d_const_bo_address *src_bo_addr, const struct wined3d_format *src_format,
        const struct wined3d_box *src_box, unsigned int src_row_pitch, unsigned int src_slice_pitch,
        struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx, unsigned int dst_location,
        unsigned int dst_x, unsigned int dst_y, unsigned int dst_z)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    unsigned int update_w = src_box->right - src_box->left;
    unsigned int update_h = src_box->bottom - src_box->top;
    unsigned int update_d = src_box->back - src_box->front;
    struct wined3d_bo_address bo;
    unsigned int level;
    BOOL decompress;
    GLenum target;

    BOOL srgb = FALSE;

    TRACE("context %p, src_bo_addr %s, src_format %s, src_box %s, src_row_pitch %u, src_slice_pitch %u, "
            "dst_texture %p, dst_sub_resource_idx %u, dst_location %s, dst_x %u, dst_y %u, dst_z %u.\n",
            context, debug_const_bo_address(src_bo_addr), debug_d3dformat(src_format->id), debug_box(src_box),
            src_row_pitch, src_slice_pitch, dst_texture, dst_sub_resource_idx,
            wined3d_debug_location(dst_location), dst_x, dst_y, dst_z);

    if (dst_location == WINED3D_LOCATION_TEXTURE_SRGB)
    {
        srgb = TRUE;
    }
    else if (dst_location != WINED3D_LOCATION_TEXTURE_RGB)
    {
        FIXME("Unhandled location %s.\n", wined3d_debug_location(dst_location));
        return;
    }

    wined3d_texture_gl_bind_and_dirtify(wined3d_texture_gl(dst_texture), wined3d_context_gl(context), srgb);

    if (dst_texture->sub_resources[dst_sub_resource_idx].map_count)
    {
        WARN("Uploading a texture that is currently mapped, setting WINED3D_TEXTURE_PIN_SYSMEM.\n");
        dst_texture->flags |= WINED3D_TEXTURE_PIN_SYSMEM;
    }

    if (src_format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_HEIGHT_SCALE)
    {
        update_h *= src_format->height_scale.numerator;
        update_h /= src_format->height_scale.denominator;
    }

    target = wined3d_texture_gl_get_sub_resource_target(wined3d_texture_gl(dst_texture), dst_sub_resource_idx);
    level = dst_sub_resource_idx % dst_texture->level_count;

    switch (target)
    {
        case GL_TEXTURE_1D_ARRAY:
            dst_y = dst_sub_resource_idx / dst_texture->level_count;
            update_h = 1;
            break;
        case GL_TEXTURE_2D_ARRAY:
            dst_z = dst_sub_resource_idx / dst_texture->level_count;
            update_d = 1;
            break;
        case GL_TEXTURE_2D_MULTISAMPLE:
        case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
            FIXME("Not supported for multisample textures.\n");
            return;
    }

    bo.buffer_object = src_bo_addr->buffer_object;
    bo.addr = (BYTE *)src_bo_addr->addr + src_box->front * src_slice_pitch;
    if (dst_texture->resource.format_flags & WINED3DFMT_FLAG_BLOCKS)
    {
        bo.addr += (src_box->top / src_format->block_height) * src_row_pitch;
        bo.addr += (src_box->left / src_format->block_width) * src_format->block_byte_count;
    }
    else
    {
        bo.addr += src_box->top * src_row_pitch;
        bo.addr += src_box->left * src_format->byte_count;
    }

    decompress = dst_texture->resource.format_flags & WINED3DFMT_FLAG_DECOMPRESS;
    if (src_format->upload || decompress)
    {
        const struct wined3d_format *compressed_format = src_format;
        unsigned int dst_row_pitch, dst_slice_pitch;
        struct wined3d_format_gl f;
        void *converted_mem;
        unsigned int z;
        BYTE *src_mem;

        if (decompress)
        {
            src_format = wined3d_resource_get_decompress_format(&dst_texture->resource);
        }
        else
        {
            if (dst_texture->resource.format_flags & WINED3DFMT_FLAG_BLOCKS)
                ERR("Converting a block-based format.\n");

            f = *wined3d_format_gl(src_format);
            f.f.byte_count = src_format->conv_byte_count;
            src_format = &f.f;
        }

        wined3d_format_calculate_pitch(src_format, 1, update_w, update_h, &dst_row_pitch, &dst_slice_pitch);

        if (!(converted_mem = heap_alloc(dst_slice_pitch)))
        {
            ERR("Failed to allocate upload buffer.\n");
            return;
        }

        src_mem = wined3d_context_gl_map_bo_address(context_gl, &bo,
                src_slice_pitch * update_d, GL_PIXEL_UNPACK_BUFFER, WINED3D_MAP_READ);

        for (z = 0; z < update_d; ++z, src_mem += src_slice_pitch)
        {
            if (decompress)
                compressed_format->decompress(src_mem, converted_mem, src_row_pitch, src_slice_pitch,
                        dst_row_pitch, dst_slice_pitch, update_w, update_h, 1);
            else
                src_format->upload(src_mem, converted_mem, src_row_pitch, src_slice_pitch,
                        dst_row_pitch, dst_slice_pitch, update_w, update_h, 1);

            wined3d_texture_gl_upload_bo(src_format, target, level, dst_row_pitch, dst_x, dst_y,
                    dst_z + z, update_w, update_h, 1, converted_mem, srgb, dst_texture, gl_info);
        }

        wined3d_context_gl_unmap_bo_address(context_gl, &bo, GL_PIXEL_UNPACK_BUFFER, 0, NULL);
        heap_free(converted_mem);
    }
    else
    {
        if (bo.buffer_object)
        {
            GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, bo.buffer_object));
            checkGLcall("glBindBuffer");
        }

        wined3d_texture_gl_upload_bo(src_format, target, level, src_row_pitch, dst_x, dst_y,
                dst_z, update_w, update_h, update_d, bo.addr, srgb, dst_texture, gl_info);

        if (bo.buffer_object)
        {
            GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
            checkGLcall("glBindBuffer");
        }
    }

    if (gl_info->quirks & WINED3D_QUIRK_FBO_TEX_UPDATE)
    {
        struct wined3d_device *device = dst_texture->resource.device;
        unsigned int i;

        for (i = 0; i < device->context_count; ++i)
        {
            wined3d_context_gl_texture_update(wined3d_context_gl(device->contexts[i]), wined3d_texture_gl(dst_texture));
        }
    }
}

static void wined3d_texture_gl_download_data_slow_path(struct wined3d_texture_gl *texture_gl,
        unsigned int sub_resource_idx, struct wined3d_context_gl *context_gl, const struct wined3d_bo_address *data)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_texture_sub_resource *sub_resource;
    unsigned int dst_row_pitch, dst_slice_pitch;
    unsigned int src_row_pitch, src_slice_pitch;
    const struct wined3d_format_gl *format_gl;
    BYTE *temporary_mem = NULL;
    unsigned int level;
    GLenum target;
    void *mem;

    format_gl = wined3d_format_gl(texture_gl->t.resource.format);

    /* Only support read back of converted P8 textures. */
    if (texture_gl->t.flags & WINED3D_TEXTURE_CONVERTED && format_gl->f.id != WINED3DFMT_P8_UINT
            && !format_gl->f.download)
    {
        ERR("Trying to read back converted texture %p, %u with format %s.\n",
                texture_gl, sub_resource_idx, debug_d3dformat(format_gl->f.id));
        return;
    }

    sub_resource = &texture_gl->t.sub_resources[sub_resource_idx];
    target = wined3d_texture_gl_get_sub_resource_target(texture_gl, sub_resource_idx);
    level = sub_resource_idx % texture_gl->t.level_count;

    if (target == GL_TEXTURE_1D_ARRAY || target == GL_TEXTURE_2D_ARRAY)
    {
        if (format_gl->f.download)
        {
            FIXME("Reading back converted array texture %p is not supported.\n", texture_gl);
            return;
        }

        /* NP2 emulation is not allowed on array textures. */
        if (texture_gl->t.flags & WINED3D_TEXTURE_COND_NP2_EMULATED)
            ERR("Array texture %p uses NP2 emulation.\n", texture_gl);

        WARN_(d3d_perf)("Downloading all miplevel layers to get the data for a single sub-resource.\n");

        if (!(temporary_mem = heap_calloc(texture_gl->t.layer_count, sub_resource->size)))
        {
            ERR("Out of memory.\n");
            return;
        }
    }

    if (texture_gl->t.flags & WINED3D_TEXTURE_COND_NP2_EMULATED)
    {
        if (format_gl->f.download)
        {
            FIXME("Reading back converted texture %p with NP2 emulation is not supported.\n", texture_gl);
            return;
        }

        wined3d_texture_get_pitch(&texture_gl->t, level, &dst_row_pitch, &dst_slice_pitch);
        wined3d_format_calculate_pitch(&format_gl->f, texture_gl->t.resource.device->surface_alignment,
                wined3d_texture_get_level_pow2_width(&texture_gl->t, level),
                wined3d_texture_get_level_pow2_height(&texture_gl->t, level),
                &src_row_pitch, &src_slice_pitch);
        if (!(temporary_mem = heap_alloc(src_slice_pitch)))
        {
            ERR("Out of memory.\n");
            return;
        }

        if (data->buffer_object)
            ERR("NP2 emulated texture uses PBO unexpectedly.\n");
        if (texture_gl->t.resource.format_flags & WINED3DFMT_FLAG_COMPRESSED)
            ERR("Unexpected compressed format for NP2 emulated texture.\n");
    }

    if (format_gl->f.download)
    {
        struct wined3d_format f;

        if (data->buffer_object)
            ERR("Converted texture %p uses PBO unexpectedly.\n", texture_gl);

        WARN_(d3d_perf)("Downloading converted texture %p, %u with format %s.\n",
                texture_gl, sub_resource_idx, debug_d3dformat(format_gl->f.id));

        f = format_gl->f;
        f.byte_count = format_gl->f.conv_byte_count;
        wined3d_texture_get_pitch(&texture_gl->t, level, &dst_row_pitch, &dst_slice_pitch);
        wined3d_format_calculate_pitch(&f, texture_gl->t.resource.device->surface_alignment,
                wined3d_texture_get_level_width(&texture_gl->t, level),
                wined3d_texture_get_level_height(&texture_gl->t, level),
                &src_row_pitch, &src_slice_pitch);

        if (!(temporary_mem = heap_alloc(src_slice_pitch)))
        {
            ERR("Failed to allocate memory.\n");
            return;
        }
    }

    if (temporary_mem)
    {
        mem = temporary_mem;
    }
    else if (data->buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, data->buffer_object));
        checkGLcall("glBindBuffer");
        mem = data->addr;
    }
    else
    {
        mem = data->addr;
    }

    if (texture_gl->t.resource.format_flags & WINED3DFMT_FLAG_COMPRESSED)
    {
        TRACE("Downloading compressed texture %p, %u, level %u, format %#x, type %#x, data %p.\n",
                texture_gl, sub_resource_idx, level, format_gl->format, format_gl->type, mem);

        GL_EXTCALL(glGetCompressedTexImage(target, level, mem));
        checkGLcall("glGetCompressedTexImage");
    }
    else
    {
        TRACE("Downloading texture %p, %u, level %u, format %#x, type %#x, data %p.\n",
                texture_gl, sub_resource_idx, level, format_gl->format, format_gl->type, mem);

        gl_info->gl_ops.gl.p_glGetTexImage(target, level, format_gl->format, format_gl->type, mem);
        checkGLcall("glGetTexImage");
    }

    if (format_gl->f.download)
    {
        format_gl->f.download(mem, data->addr, src_row_pitch, src_slice_pitch, dst_row_pitch, dst_slice_pitch,
                wined3d_texture_get_level_width(&texture_gl->t, level),
                wined3d_texture_get_level_height(&texture_gl->t, level), 1);
    }
    else if (texture_gl->t.flags & WINED3D_TEXTURE_COND_NP2_EMULATED)
    {
        const BYTE *src_data;
        unsigned int h, y;
        BYTE *dst_data;
        /* Some games (e.g. Warhammer 40,000) don't properly handle texture
         * pitches, preventing us from using the texture pitch to box NPOT
         * textures. Instead, we repack the texture's CPU copy so that its
         * pitch equals bpp * width instead of bpp * pow2width.
         *
         * Instead of boxing the texture:
         *
         * │<── texture width ──>│ pow2 width ──>│
         * ├─────────────────────┼───────────────┼─
         * │111111111111111111111│               │ʌ
         * │222222222222222222222│               ││
         * │333333333333333333333│    padding    │texture height
         * │444444444444444444444│               ││
         * │555555555555555555555│               │v
         * ├─────────────────────┘               ├─
         * │                                     │pow2 height
         * │       padding            padding    ││
         * │                                     │v
         * └─────────────────────────────────────┴─
         *
         * we're repacking the data to the expected texture width
         *
         * │<── texture width ──>│ pow2 width ──>│
         * ├─────────────────────┴───────────────┼─
         * │1111111111111111111112222222222222222│ʌ
         * │2222233333333333333333333344444444444││
         * │4444444444555555555555555555555      │texture height
         * │                                     ││
         * │        padding       padding        │v
         * │                                     ├─
         * │                                     │pow2 height
         * │        padding       padding        ││
         * │                                     │v
         * └─────────────────────────────────────┴─
         *
         * == is the same as
         *
         * │<── texture width ──>│
         * ├─────────────────────┼─
         * │111111111111111111111│ʌ
         * │222222222222222222222││
         * │333333333333333333333│texture height
         * │444444444444444444444││
         * │555555555555555555555│v
         * └─────────────────────┴─
         *
         * This also means that any references to surface memory should work
         * with the data as if it were a standard texture with a NPOT width
         * instead of a texture boxed up to be a power-of-two texture. */
        src_data = mem;
        dst_data = data->addr;
        TRACE("Repacking the surface data from pitch %u to pitch %u.\n", src_row_pitch, dst_row_pitch);
        h = wined3d_texture_get_level_height(&texture_gl->t, level);
        for (y = 0; y < h; ++y)
        {
            memcpy(dst_data, src_data, dst_row_pitch);
            src_data += src_row_pitch;
            dst_data += dst_row_pitch;
        }
    }
    else if (temporary_mem)
    {
        unsigned int layer = sub_resource_idx / texture_gl->t.level_count;
        void *src_data = temporary_mem + layer * sub_resource->size;
        if (data->buffer_object)
        {
            GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, data->buffer_object));
            checkGLcall("glBindBuffer");
            GL_EXTCALL(glBufferSubData(GL_PIXEL_PACK_BUFFER, 0, sub_resource->size, src_data));
            checkGLcall("glBufferSubData");
        }
        else
        {
            memcpy(data->addr, src_data, sub_resource->size);
        }
    }

    if (data->buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
        checkGLcall("glBindBuffer");
    }

    heap_free(temporary_mem);
}

static void wined3d_texture_gl_download_data(struct wined3d_context *context,
        struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx, unsigned int src_location,
        const struct wined3d_box *src_box, const struct wined3d_bo_address *dst_bo_addr,
        const struct wined3d_format *dst_format, unsigned int dst_x, unsigned int dst_y, unsigned int dst_z,
        unsigned int dst_row_pitch, unsigned int dst_slice_pitch)
{
    struct wined3d_texture_gl *src_texture_gl = wined3d_texture_gl(src_texture);
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    unsigned int src_level, src_width, src_height, src_depth;
    unsigned int src_row_pitch, src_slice_pitch;
    const struct wined3d_format_gl *format_gl;
    BOOL srgb = FALSE;
    GLenum target;
    struct wined3d_texture_sub_resource *sub_resource;

    TRACE("context %p, src_texture %p, src_sub_resource_idx %u, src_location %s, src_box %s, dst_bo_addr %s, "
            "dst_format %s, dst_x %u, dst_y %u, dst_z %u, dst_row_pitch %u, dst_slice_pitch %u.\n",
            context, src_texture, src_sub_resource_idx, wined3d_debug_location(src_location),
            debug_box(src_box), debug_bo_address(dst_bo_addr), debug_d3dformat(dst_format->id),
            dst_x, dst_y, dst_z, dst_row_pitch, dst_slice_pitch);

    if (src_location == WINED3D_LOCATION_TEXTURE_SRGB)
    {
        srgb = TRUE;
    }
    else if (src_location != WINED3D_LOCATION_TEXTURE_RGB)
    {
        FIXME("Unhandled location %s.\n", wined3d_debug_location(src_location));
        return;
    }

    src_level = src_sub_resource_idx % src_texture->level_count;
    src_width = wined3d_texture_get_level_width(src_texture, src_level);
    src_height = wined3d_texture_get_level_height(src_texture, src_level);
    src_depth = wined3d_texture_get_level_depth(src_texture, src_level);
    if (src_box->left || src_box->top || src_box->right != src_width || src_box->bottom != src_height
            || src_box->front || src_box->back != src_depth)
    {
        FIXME("Unhandled source box %s.\n", debug_box(src_box));
        return;
    }

    if (dst_x || dst_y || dst_z)
    {
        FIXME("Unhandled destination (%u, %u, %u).\n", dst_x, dst_y, dst_z);
        return;
    }

    if (dst_format->id != src_texture->resource.format->id)
    {
        FIXME("Unhandled format conversion (%s -> %s).\n",
                debug_d3dformat(src_texture->resource.format->id),
                debug_d3dformat(dst_format->id));
        return;
    }

    wined3d_texture_get_pitch(src_texture, src_level, &src_row_pitch, &src_slice_pitch);
    if (src_row_pitch != dst_row_pitch || src_slice_pitch != dst_slice_pitch)
    {
        FIXME("Unhandled destination pitches %u/%u (source pitches %u/%u).\n",
                dst_row_pitch, dst_slice_pitch, src_row_pitch, src_slice_pitch);
        return;
    }

    wined3d_texture_gl_bind_and_dirtify(src_texture_gl, context_gl, srgb);

    format_gl = wined3d_format_gl(src_texture->resource.format);
    target = wined3d_texture_gl_get_sub_resource_target(src_texture_gl, src_sub_resource_idx);
    sub_resource = &src_texture->sub_resources[src_sub_resource_idx];

    if ((src_texture->resource.type == WINED3D_RTYPE_TEXTURE_2D
            && (target == GL_TEXTURE_2D_ARRAY || format_gl->f.conv_byte_count
            || src_texture->flags & (WINED3D_TEXTURE_CONVERTED | WINED3D_TEXTURE_COND_NP2_EMULATED)))
            || target == GL_TEXTURE_1D_ARRAY)
    {
        wined3d_texture_gl_download_data_slow_path(src_texture_gl, src_sub_resource_idx, context_gl, dst_bo_addr);
        return;
    }

    if (format_gl->f.conv_byte_count)
    {
        FIXME("Attempting to download a converted texture, type %s format %s.\n",
                debug_d3dresourcetype(src_texture->resource.type),
                debug_d3dformat(format_gl->f.id));
        return;
    }

    if (dst_bo_addr->buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, dst_bo_addr->buffer_object));
        checkGLcall("glBindBuffer");
    }

    if (src_texture->resource.format_flags & WINED3DFMT_FLAG_COMPRESSED)
    {
        TRACE("Downloading compressed texture %p, %u, level %u, format %#x, type %#x, data %p.\n",
                src_texture, src_sub_resource_idx, src_level, format_gl->format, format_gl->type, dst_bo_addr->addr);

        GL_EXTCALL(glGetCompressedTexImage(target, src_level, dst_bo_addr->addr));
        checkGLcall("glGetCompressedTexImage");
    }
    else if (dst_bo_addr->buffer_object && src_texture->resource.bind_flags & WINED3D_BIND_RENDER_TARGET)
    {
        /* PBO texture download is not accelerated on Mesa. Use glReadPixels if possible. */
        TRACE("Downloading (glReadPixels) texture %p, %u, level %u, format %#x, type %#x, data %p.\n",
                src_texture, src_sub_resource_idx, src_level, format_gl->format, format_gl->type, dst_bo_addr->addr);

        wined3d_context_gl_apply_fbo_state_blit(context_gl, GL_READ_FRAMEBUFFER, &src_texture->resource, src_sub_resource_idx, NULL,
                0, sub_resource->locations & (WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB));
        wined3d_context_gl_check_fbo_status(context_gl, GL_READ_FRAMEBUFFER);
        context_invalidate_state(context, STATE_FRAMEBUFFER);
        gl_info->gl_ops.gl.p_glReadBuffer(GL_COLOR_ATTACHMENT0);
        checkGLcall("glReadBuffer()");

        gl_info->gl_ops.gl.p_glReadPixels(0, 0, wined3d_texture_get_level_width(src_texture, src_level),
                wined3d_texture_get_level_height(src_texture, src_level), format_gl->format, format_gl->type, dst_bo_addr->addr);
        checkGLcall("glReadPixels");
    }
    else
    {
        TRACE("Downloading texture %p, %u, level %u, format %#x, type %#x, data %p.\n",
                src_texture, src_sub_resource_idx, src_level, format_gl->format, format_gl->type, dst_bo_addr->addr);

        gl_info->gl_ops.gl.p_glGetTexImage(target, src_level, format_gl->format, format_gl->type, dst_bo_addr->addr);
        checkGLcall("glGetTexImage");
    }

    if (dst_bo_addr->buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
        checkGLcall("glBindBuffer");
    }
}

/* Context activation is done by the caller. */
static BOOL wined3d_texture_gl_load_sysmem(struct wined3d_texture_gl *texture_gl,
        unsigned int sub_resource_idx, struct wined3d_context_gl *context_gl, DWORD dst_location)
{
    struct wined3d_texture_sub_resource *sub_resource;

    sub_resource = &texture_gl->t.sub_resources[sub_resource_idx];

    /* We cannot download data from multisample textures directly. */
    if (wined3d_texture_gl_is_multisample_location(texture_gl, WINED3D_LOCATION_TEXTURE_RGB))
    {
        wined3d_texture_load_location(&texture_gl->t, sub_resource_idx, &context_gl->c, WINED3D_LOCATION_RB_RESOLVED);
        texture2d_read_from_framebuffer(&texture_gl->t, sub_resource_idx, &context_gl->c,
                WINED3D_LOCATION_RB_RESOLVED, dst_location);
        return TRUE;
    }

    if (sub_resource->locations & (WINED3D_LOCATION_RB_MULTISAMPLE | WINED3D_LOCATION_RB_RESOLVED))
        wined3d_texture_load_location(&texture_gl->t, sub_resource_idx, &context_gl->c, WINED3D_LOCATION_TEXTURE_RGB);

    /* Download the sub-resource to system memory. */
    if (sub_resource->locations & (WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB))
    {
        unsigned int row_pitch, slice_pitch, level;
        struct wined3d_bo_address data;
        struct wined3d_box src_box;
        unsigned int src_location;

        level = sub_resource_idx % texture_gl->t.level_count;
        wined3d_texture_get_memory(&texture_gl->t, sub_resource_idx, &data, dst_location);
        src_location = sub_resource->locations & WINED3D_LOCATION_TEXTURE_RGB
                ? WINED3D_LOCATION_TEXTURE_RGB : WINED3D_LOCATION_TEXTURE_SRGB;
        wined3d_texture_get_level_box(&texture_gl->t, level, &src_box);
        wined3d_texture_get_pitch(&texture_gl->t, level, &row_pitch, &slice_pitch);
        wined3d_texture_gl_download_data(&context_gl->c, &texture_gl->t, sub_resource_idx, src_location,
                &src_box, &data, texture_gl->t.resource.format, 0, 0, 0, row_pitch, slice_pitch);

        ++texture_gl->t.download_count;
        return TRUE;
    }

    if (!(texture_gl->t.resource.bind_flags & WINED3D_BIND_DEPTH_STENCIL)
            && (sub_resource->locations & WINED3D_LOCATION_DRAWABLE))
    {
        texture2d_read_from_framebuffer(&texture_gl->t, sub_resource_idx, &context_gl->c,
                texture_gl->t.resource.draw_binding, dst_location);
        return TRUE;
    }

    FIXME("Can't load texture %p, %u with location flags %s into sysmem.\n",
            texture_gl, sub_resource_idx, wined3d_debug_location(sub_resource->locations));

    return FALSE;
}

static BOOL wined3d_texture_load_drawable(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_context *context)
{
    struct wined3d_device *device;
    unsigned int level;
    RECT r;

    if (texture->resource.bind_flags & WINED3D_BIND_DEPTH_STENCIL)
    {
        DWORD current = texture->sub_resources[sub_resource_idx].locations;
        FIXME("Unimplemented copy from %s for depth/stencil buffers.\n",
                wined3d_debug_location(current));
        return FALSE;
    }

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO
            && wined3d_resource_is_offscreen(&texture->resource))
    {
        ERR("Trying to load offscreen texture into WINED3D_LOCATION_DRAWABLE.\n");
        return FALSE;
    }

    device = texture->resource.device;
    level = sub_resource_idx % texture->level_count;
    SetRect(&r, 0, 0, wined3d_texture_get_level_width(texture, level),
            wined3d_texture_get_level_height(texture, level));
    wined3d_texture_load_location(texture, sub_resource_idx, context, WINED3D_LOCATION_TEXTURE_RGB);
    device->blitter->ops->blitter_blit(device->blitter, WINED3D_BLIT_OP_COLOR_BLIT, context,
            texture, sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB, &r,
            texture, sub_resource_idx, WINED3D_LOCATION_DRAWABLE, &r,
            NULL, WINED3D_TEXF_POINT);

    return TRUE;
}

static BOOL wined3d_texture_load_renderbuffer(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_context *context, DWORD dst_location)
{
    unsigned int level = sub_resource_idx % texture->level_count;
    const RECT rect = {0, 0,
            wined3d_texture_get_level_width(texture, level),
            wined3d_texture_get_level_height(texture, level)};
    struct wined3d_texture_sub_resource *sub_resource;
    DWORD src_location, locations;

    sub_resource = &texture->sub_resources[sub_resource_idx];
    locations = sub_resource->locations;
    if (texture->resource.bind_flags & WINED3D_BIND_DEPTH_STENCIL)
    {
        FIXME("Unimplemented copy from %s for depth/stencil buffers.\n",
                wined3d_debug_location(locations));
        return FALSE;
    }

    if (locations & WINED3D_LOCATION_RB_MULTISAMPLE)
        src_location = WINED3D_LOCATION_RB_MULTISAMPLE;
    else if (locations & WINED3D_LOCATION_RB_RESOLVED)
        src_location = WINED3D_LOCATION_RB_RESOLVED;
    else if (locations & WINED3D_LOCATION_TEXTURE_SRGB)
        src_location = WINED3D_LOCATION_TEXTURE_SRGB;
    else if (locations & WINED3D_LOCATION_TEXTURE_RGB)
        src_location = WINED3D_LOCATION_TEXTURE_RGB;
    else if (locations & WINED3D_LOCATION_DRAWABLE)
        src_location = WINED3D_LOCATION_DRAWABLE;
    else /* texture2d_blt_fbo() will load the source location if necessary. */
        src_location = WINED3D_LOCATION_TEXTURE_RGB;

    texture2d_blt_fbo(texture->resource.device, context, WINED3D_TEXF_POINT, texture,
            sub_resource_idx, src_location, &rect, texture, sub_resource_idx, dst_location, &rect);

    return TRUE;
}

static BOOL wined3d_texture_gl_load_texture(struct wined3d_texture_gl *texture_gl,
        unsigned int sub_resource_idx, struct wined3d_context_gl *context_gl, BOOL srgb)
{
    unsigned int width, height, level, src_row_pitch, src_slice_pitch, dst_row_pitch, dst_slice_pitch;
    struct wined3d_device *device = texture_gl->t.resource.device;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    const struct wined3d_color_key_conversion *conversion;
    struct wined3d_texture_sub_resource *sub_resource;
    const struct wined3d_format *format;
    struct wined3d_bo_address data;
    BYTE *src_mem, *dst_mem = NULL;
    struct wined3d_box src_box;
    DWORD dst_location;
    BOOL depth;

    depth = texture_gl->t.resource.bind_flags & WINED3D_BIND_DEPTH_STENCIL;
    sub_resource = &texture_gl->t.sub_resources[sub_resource_idx];

    if (!depth && wined3d_settings.offscreen_rendering_mode != ORM_FBO
            && wined3d_resource_is_offscreen(&texture_gl->t.resource)
            && (sub_resource->locations & WINED3D_LOCATION_DRAWABLE))
    {
        texture2d_load_fb_texture(texture_gl, sub_resource_idx, srgb, &context_gl->c);

        return TRUE;
    }

    level = sub_resource_idx % texture_gl->t.level_count;
    wined3d_texture_get_level_box(&texture_gl->t, level, &src_box);

    if (!depth && sub_resource->locations & (WINED3D_LOCATION_TEXTURE_SRGB | WINED3D_LOCATION_TEXTURE_RGB)
            && (texture_gl->t.resource.format_flags & WINED3DFMT_FLAG_FBO_ATTACHABLE_SRGB)
            && fbo_blitter_supported(WINED3D_BLIT_OP_COLOR_BLIT, gl_info,
                    &texture_gl->t.resource, WINED3D_LOCATION_TEXTURE_RGB,
                    &texture_gl->t.resource, WINED3D_LOCATION_TEXTURE_SRGB))
    {
        RECT src_rect;

        SetRect(&src_rect, src_box.left, src_box.top, src_box.right, src_box.bottom);
        if (srgb)
            texture2d_blt_fbo(device, &context_gl->c, WINED3D_TEXF_POINT,
                    &texture_gl->t, sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB, &src_rect,
                    &texture_gl->t, sub_resource_idx, WINED3D_LOCATION_TEXTURE_SRGB, &src_rect);
        else
            texture2d_blt_fbo(device, &context_gl->c, WINED3D_TEXF_POINT,
                    &texture_gl->t, sub_resource_idx, WINED3D_LOCATION_TEXTURE_SRGB, &src_rect,
                    &texture_gl->t, sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB, &src_rect);

        return TRUE;
    }

    if (!depth && sub_resource->locations & (WINED3D_LOCATION_RB_MULTISAMPLE | WINED3D_LOCATION_RB_RESOLVED)
            && (!srgb || (texture_gl->t.resource.format_flags & WINED3DFMT_FLAG_FBO_ATTACHABLE_SRGB)))
    {
        DWORD src_location = sub_resource->locations & WINED3D_LOCATION_RB_RESOLVED ?
                WINED3D_LOCATION_RB_RESOLVED : WINED3D_LOCATION_RB_MULTISAMPLE;
        RECT src_rect;

        SetRect(&src_rect, src_box.left, src_box.top, src_box.right, src_box.bottom);
        dst_location = srgb ? WINED3D_LOCATION_TEXTURE_SRGB : WINED3D_LOCATION_TEXTURE_RGB;
        if (fbo_blitter_supported(WINED3D_BLIT_OP_COLOR_BLIT, gl_info,
                &texture_gl->t.resource, src_location, &texture_gl->t.resource, dst_location))
            texture2d_blt_fbo(device, &context_gl->c, WINED3D_TEXF_POINT, &texture_gl->t, sub_resource_idx,
                    src_location, &src_rect, &texture_gl->t, sub_resource_idx, dst_location, &src_rect);

        return TRUE;
    }

    /* Upload from system memory */

    if (srgb)
    {
        dst_location = WINED3D_LOCATION_TEXTURE_SRGB;
        if ((sub_resource->locations & (WINED3D_LOCATION_TEXTURE_RGB | texture_gl->t.resource.map_binding))
                == WINED3D_LOCATION_TEXTURE_RGB)
        {
            FIXME_(d3d_perf)("Downloading RGB texture %p, %u to reload it as sRGB.\n", texture_gl, sub_resource_idx);
            wined3d_texture_load_location(&texture_gl->t, sub_resource_idx,
                    &context_gl->c, texture_gl->t.resource.map_binding);
        }
    }
    else
    {
        dst_location = WINED3D_LOCATION_TEXTURE_RGB;
        if ((sub_resource->locations & (WINED3D_LOCATION_TEXTURE_SRGB | texture_gl->t.resource.map_binding))
                == WINED3D_LOCATION_TEXTURE_SRGB)
        {
            FIXME_(d3d_perf)("Downloading sRGB texture %p, %u to reload it as RGB.\n", texture_gl, sub_resource_idx);
            wined3d_texture_load_location(&texture_gl->t, sub_resource_idx,
                    &context_gl->c, texture_gl->t.resource.map_binding);
        }
    }

    if (!(sub_resource->locations & wined3d_texture_sysmem_locations))
    {
        WARN("Trying to load a texture from sysmem, but no simple location is valid.\n");
        /* Lets hope we get it from somewhere... */
        wined3d_texture_load_location(&texture_gl->t, sub_resource_idx, &context_gl->c, WINED3D_LOCATION_SYSMEM);
    }

    wined3d_texture_get_pitch(&texture_gl->t, level, &src_row_pitch, &src_slice_pitch);

    format = texture_gl->t.resource.format;
    if ((conversion = wined3d_format_get_color_key_conversion(&texture_gl->t, TRUE)))
        format = wined3d_get_format(device->adapter, conversion->dst_format, texture_gl->t.resource.bind_flags);

    /* Don't use PBOs for converted surfaces. During PBO conversion we look at
     * WINED3D_TEXTURE_CONVERTED but it isn't set (yet) in all cases it is
     * getting called. */
    if (conversion && sub_resource->buffer_object)
    {
        TRACE("Removing the pbo attached to texture %p, %u.\n", texture_gl, sub_resource_idx);

        wined3d_texture_load_location(&texture_gl->t, sub_resource_idx, &context_gl->c, WINED3D_LOCATION_SYSMEM);
        wined3d_texture_set_map_binding(&texture_gl->t, WINED3D_LOCATION_SYSMEM);
    }

    wined3d_texture_get_memory(&texture_gl->t, sub_resource_idx, &data, sub_resource->locations);
    if (conversion)
    {
        width = src_box.right - src_box.left;
        height = src_box.bottom - src_box.top;
        wined3d_format_calculate_pitch(format, device->surface_alignment,
                width, height, &dst_row_pitch, &dst_slice_pitch);

        src_mem = wined3d_context_gl_map_bo_address(context_gl, &data,
                src_slice_pitch, GL_PIXEL_UNPACK_BUFFER, WINED3D_MAP_READ);
        if (!(dst_mem = heap_alloc(dst_slice_pitch)))
        {
            ERR("Out of memory (%u).\n", dst_slice_pitch);
            return FALSE;
        }
        conversion->convert(src_mem, src_row_pitch, dst_mem, dst_row_pitch,
                width, height, &texture_gl->t.async.gl_color_key);
        src_row_pitch = dst_row_pitch;
        src_slice_pitch = dst_slice_pitch;
        wined3d_context_gl_unmap_bo_address(context_gl, &data, GL_PIXEL_UNPACK_BUFFER, 0, NULL);

        data.buffer_object = 0;
        data.addr = dst_mem;
    }

    wined3d_texture_gl_upload_data(&context_gl->c, wined3d_const_bo_address(&data), format, &src_box,
            src_row_pitch, src_slice_pitch, &texture_gl->t, sub_resource_idx, dst_location, 0, 0, 0);

    heap_free(dst_mem);

    return TRUE;
}

static BOOL wined3d_texture_gl_prepare_location(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_context *context, unsigned int location)
{
    struct wined3d_texture_gl *texture_gl = wined3d_texture_gl(texture);
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);

    switch (location)
    {
        case WINED3D_LOCATION_SYSMEM:
            return wined3d_resource_prepare_sysmem(&texture->resource);

        case WINED3D_LOCATION_USER_MEMORY:
            if (!texture->user_memory)
                ERR("Preparing WINED3D_LOCATION_USER_MEMORY, but texture->user_memory is NULL.\n");
            return TRUE;

        case WINED3D_LOCATION_BUFFER:
            wined3d_texture_prepare_buffer_object(texture, sub_resource_idx, context_gl->gl_info);
            return TRUE;

        case WINED3D_LOCATION_TEXTURE_RGB:
            wined3d_texture_gl_prepare_texture(texture_gl, context_gl, FALSE);
            return TRUE;

        case WINED3D_LOCATION_TEXTURE_SRGB:
            wined3d_texture_gl_prepare_texture(texture_gl, context_gl, TRUE);
            return TRUE;

        case WINED3D_LOCATION_DRAWABLE:
            if (!texture->swapchain && wined3d_settings.offscreen_rendering_mode != ORM_BACKBUFFER)
                ERR("Texture %p does not have a drawable.\n", texture);
            return TRUE;

        case WINED3D_LOCATION_RB_MULTISAMPLE:
            wined3d_texture_gl_prepare_rb(texture_gl, context_gl->gl_info, TRUE);
            return TRUE;

        case WINED3D_LOCATION_RB_RESOLVED:
            wined3d_texture_gl_prepare_rb(texture_gl, context_gl->gl_info, FALSE);
            return TRUE;

        default:
            ERR("Invalid location %s.\n", wined3d_debug_location(location));
            return FALSE;
    }
}

/* Context activation is done by the caller. */
static BOOL wined3d_texture_gl_load_location(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_context *context, DWORD location)
{
    struct wined3d_texture_gl *texture_gl = wined3d_texture_gl(texture);
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);

    TRACE("texture %p, sub_resource_idx %u, context %p, location %s.\n",
            texture, sub_resource_idx, context, wined3d_debug_location(location));

    if (!wined3d_texture_gl_prepare_location(texture, sub_resource_idx, context, location))
        return FALSE;

    switch (location)
    {
        case WINED3D_LOCATION_USER_MEMORY:
        case WINED3D_LOCATION_SYSMEM:
        case WINED3D_LOCATION_BUFFER:
            return wined3d_texture_gl_load_sysmem(texture_gl, sub_resource_idx, context_gl, location);

        case WINED3D_LOCATION_DRAWABLE:
            return wined3d_texture_load_drawable(texture, sub_resource_idx, context);

        case WINED3D_LOCATION_RB_RESOLVED:
        case WINED3D_LOCATION_RB_MULTISAMPLE:
            return wined3d_texture_load_renderbuffer(texture, sub_resource_idx, context, location);

        case WINED3D_LOCATION_TEXTURE_RGB:
        case WINED3D_LOCATION_TEXTURE_SRGB:
            return wined3d_texture_gl_load_texture(texture_gl, sub_resource_idx,
                    context_gl, location == WINED3D_LOCATION_TEXTURE_SRGB);

        default:
            FIXME("Unhandled %s load from %s.\n", wined3d_debug_location(location),
                    wined3d_debug_location(texture->sub_resources[sub_resource_idx].locations));
            return FALSE;
    }
}

static const struct wined3d_texture_ops texture_gl_ops =
{
    wined3d_texture_gl_prepare_location,
    wined3d_texture_gl_load_location,
    wined3d_texture_gl_upload_data,
    wined3d_texture_gl_download_data,
};

struct wined3d_texture * __cdecl wined3d_texture_from_resource(struct wined3d_resource *resource)
{
    return texture_from_resource(resource);
}

static ULONG texture_resource_incref(struct wined3d_resource *resource)
{
    return wined3d_texture_incref(texture_from_resource(resource));
}

static ULONG texture_resource_decref(struct wined3d_resource *resource)
{
    return wined3d_texture_decref(texture_from_resource(resource));
}

static void texture_resource_preload(struct wined3d_resource *resource)
{
    struct wined3d_texture *texture = texture_from_resource(resource);
    struct wined3d_context *context;

    context = context_acquire(resource->device, NULL, 0);
    wined3d_texture_load(texture, context, texture->flags & WINED3D_TEXTURE_IS_SRGB);
    context_release(context);
}

static void wined3d_texture_gl_unload(struct wined3d_resource *resource)
{
    struct wined3d_texture_gl *texture_gl = wined3d_texture_gl(texture_from_resource(resource));
    UINT sub_count = texture_gl->t.level_count * texture_gl->t.layer_count;
    struct wined3d_renderbuffer_entry *entry, *entry2;
    struct wined3d_device *device = resource->device;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    UINT i;

    TRACE("texture_gl %p.\n", texture_gl);

    context = context_acquire(device, NULL, 0);
    gl_info = wined3d_context_gl(context)->gl_info;

    for (i = 0; i < sub_count; ++i)
    {
        struct wined3d_texture_sub_resource *sub_resource = &texture_gl->t.sub_resources[i];

        if (resource->access & WINED3D_RESOURCE_ACCESS_CPU
                && wined3d_texture_load_location(&texture_gl->t, i, context, resource->map_binding))
        {
            wined3d_texture_invalidate_location(&texture_gl->t, i, ~resource->map_binding);
        }
        else
        {
            /* We should only get here on device reset/teardown for implicit
             * resources. */
            if (resource->access & WINED3D_RESOURCE_ACCESS_CPU
                    || resource->type != WINED3D_RTYPE_TEXTURE_2D)
                ERR("Discarding %s %p sub-resource %u with resource access %s.\n",
                        debug_d3dresourcetype(resource->type), resource, i,
                        wined3d_debug_resource_access(resource->access));
            wined3d_texture_validate_location(&texture_gl->t, i, WINED3D_LOCATION_DISCARDED);
            wined3d_texture_invalidate_location(&texture_gl->t, i, ~WINED3D_LOCATION_DISCARDED);
        }

        if (sub_resource->buffer_object)
            wined3d_texture_remove_buffer_object(&texture_gl->t, i, gl_info);
    }

    LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &texture_gl->renderbuffers, struct wined3d_renderbuffer_entry, entry)
    {
        context_gl_resource_released(device, entry->id, TRUE);
        gl_info->fbo_ops.glDeleteRenderbuffers(1, &entry->id);
        list_remove(&entry->entry);
        heap_free(entry);
    }
    list_init(&texture_gl->renderbuffers);
    texture_gl->current_renderbuffer = NULL;

    context_release(context);

    wined3d_texture_force_reload(&texture_gl->t);
    wined3d_texture_gl_unload_texture(texture_gl);
}

static HRESULT texture_resource_sub_resource_map(struct wined3d_resource *resource, unsigned int sub_resource_idx,
        struct wined3d_map_desc *map_desc, const struct wined3d_box *box, DWORD flags)
{
    const struct wined3d_format *format = resource->format;
    struct wined3d_texture_sub_resource *sub_resource;
    struct wined3d_device *device = resource->device;
    unsigned int fmt_flags = resource->format_flags;
    struct wined3d_context *context;
    struct wined3d_texture *texture;
    struct wined3d_bo_address data;
    unsigned int texture_level;
    BYTE *base_memory;
    BOOL ret;

    TRACE("resource %p, sub_resource_idx %u, map_desc %p, box %s, flags %#x.\n",
            resource, sub_resource_idx, map_desc, debug_box(box), flags);

    texture = texture_from_resource(resource);
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture, sub_resource_idx)))
        return E_INVALIDARG;

    texture_level = sub_resource_idx % texture->level_count;
    if (box && FAILED(wined3d_texture_check_box_dimensions(texture, texture_level, box)))
    {
        WARN("Map box is invalid.\n");
        if (((fmt_flags & WINED3DFMT_FLAG_BLOCKS) && !(resource->access & WINED3D_RESOURCE_ACCESS_CPU))
                || resource->type != WINED3D_RTYPE_TEXTURE_2D)
            return WINED3DERR_INVALIDCALL;
    }

    if (texture->flags & WINED3D_TEXTURE_DC_IN_USE)
    {
        WARN("DC is in use.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (sub_resource->map_count)
    {
        WARN("Sub-resource is already mapped.\n");
        return WINED3DERR_INVALIDCALL;
    }

    context = context_acquire(device, NULL, 0);

    if (flags & WINED3D_MAP_DISCARD)
    {
        TRACE("WINED3D_MAP_DISCARD flag passed, marking %s as up to date.\n",
                wined3d_debug_location(resource->map_binding));
        if ((ret = wined3d_texture_prepare_location(texture, sub_resource_idx, context, resource->map_binding)))
            wined3d_texture_validate_location(texture, sub_resource_idx, resource->map_binding);
    }
    else
    {
        if (resource->usage & WINED3DUSAGE_DYNAMIC)
            WARN_(d3d_perf)("Mapping a dynamic texture without WINED3D_MAP_DISCARD.\n");
        ret = wined3d_texture_load_location(texture, sub_resource_idx, context, resource->map_binding);
    }

    if (!ret)
    {
        ERR("Failed to prepare location.\n");
        context_release(context);
        return E_OUTOFMEMORY;
    }

    if (flags & WINED3D_MAP_WRITE
            && (!(flags & WINED3D_MAP_NO_DIRTY_UPDATE) || (resource->usage & WINED3DUSAGE_DYNAMIC)))
        wined3d_texture_invalidate_location(texture, sub_resource_idx, ~resource->map_binding);

    wined3d_texture_get_memory(texture, sub_resource_idx, &data, resource->map_binding);
    base_memory = wined3d_context_map_bo_address(context, &data, sub_resource->size, 0, flags);
    sub_resource->map_flags = flags;
    TRACE("Base memory pointer %p.\n", base_memory);

    context_release(context);

    if (fmt_flags & WINED3DFMT_FLAG_BROKEN_PITCH)
    {
        map_desc->row_pitch = wined3d_texture_get_level_width(texture, texture_level) * format->byte_count;
        map_desc->slice_pitch = wined3d_texture_get_level_height(texture, texture_level) * map_desc->row_pitch;
    }
    else
    {
        wined3d_texture_get_pitch(texture, texture_level, &map_desc->row_pitch, &map_desc->slice_pitch);
    }

    if (!box)
    {
        map_desc->data = base_memory;
    }
    else
    {
        if ((fmt_flags & (WINED3DFMT_FLAG_BLOCKS | WINED3DFMT_FLAG_BROKEN_PITCH)) == WINED3DFMT_FLAG_BLOCKS)
        {
            /* Compressed textures are block based, so calculate the offset of
             * the block that contains the top-left pixel of the mapped box. */
            map_desc->data = base_memory
                    + (box->front * map_desc->slice_pitch)
                    + ((box->top / format->block_height) * map_desc->row_pitch)
                    + ((box->left / format->block_width) * format->block_byte_count);
        }
        else
        {
            map_desc->data = base_memory
                    + (box->front * map_desc->slice_pitch)
                    + (box->top * map_desc->row_pitch)
                    + (box->left * format->byte_count);
        }
    }

    if (texture->swapchain && texture->swapchain->front_buffer == texture)
    {
        RECT *r = &texture->swapchain->front_buffer_update;

        if (!box)
            SetRect(r, 0, 0, resource->width, resource->height);
        else
            SetRect(r, box->left, box->top, box->right, box->bottom);
        TRACE("Mapped front buffer %s.\n", wine_dbgstr_rect(r));
    }

    ++resource->map_count;
    ++sub_resource->map_count;

    TRACE("Returning memory %p, row pitch %u, slice pitch %u.\n",
            map_desc->data, map_desc->row_pitch, map_desc->slice_pitch);

    return WINED3D_OK;
}

static HRESULT texture_resource_sub_resource_map_info(struct wined3d_resource *resource, unsigned int sub_resource_idx,
        struct wined3d_map_info *info, DWORD flags)
{
    const struct wined3d_format *format = resource->format;
    struct wined3d_texture_sub_resource *sub_resource;
    unsigned int fmt_flags = resource->format_flags;
    struct wined3d_texture *texture;
    unsigned int texture_level;

    texture = texture_from_resource(resource);
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture, sub_resource_idx)))
        return E_INVALIDARG;

    texture_level = sub_resource_idx % texture->level_count;

    if (fmt_flags & WINED3DFMT_FLAG_BROKEN_PITCH)
    {
        info->row_pitch = wined3d_texture_get_level_width(texture, texture_level) * format->byte_count;
        info->slice_pitch = wined3d_texture_get_level_height(texture, texture_level) * info->row_pitch;
    }
    else
    {
        wined3d_texture_get_pitch(texture, texture_level, &info->row_pitch, &info->slice_pitch);
    }

    info->size = info->slice_pitch * wined3d_texture_get_level_depth(texture, texture_level);

    return WINED3D_OK;
}

static HRESULT texture_resource_sub_resource_unmap(struct wined3d_resource *resource, unsigned int sub_resource_idx)
{
    struct wined3d_texture_sub_resource *sub_resource;
    struct wined3d_device *device = resource->device;
    struct wined3d_context *context;
    struct wined3d_texture *texture;
    struct wined3d_bo_address data;
    struct wined3d_map_range range;

    TRACE("resource %p, sub_resource_idx %u.\n", resource, sub_resource_idx);

    texture = texture_from_resource(resource);
    if (!(sub_resource = wined3d_texture_get_sub_resource(texture, sub_resource_idx)))
        return E_INVALIDARG;

    if (!sub_resource->map_count)
    {
        WARN("Trying to unmap unmapped sub-resource.\n");
        if (texture->flags & WINED3D_TEXTURE_DC_IN_USE)
            return WINED3D_OK;
        return WINEDDERR_NOTLOCKED;
    }

    context = context_acquire(device, NULL, 0);

    wined3d_texture_get_memory(texture, sub_resource_idx, &data, texture->resource.map_binding);
    range.offset = 0;
    range.size = sub_resource->size;
    wined3d_context_unmap_bo_address(context, &data, 0, !!(sub_resource->map_flags & WINED3D_MAP_WRITE), &range);

    context_release(context);

    if (texture->swapchain && texture->swapchain->front_buffer == texture)
    {
        if (!(sub_resource->locations & (WINED3D_LOCATION_DRAWABLE | WINED3D_LOCATION_TEXTURE_RGB)))
            texture->swapchain->swapchain_ops->swapchain_frontbuffer_updated(texture->swapchain);
    }

    --sub_resource->map_count;
    if (!--resource->map_count && texture->update_map_binding)
        wined3d_texture_update_map_binding(texture);

    return WINED3D_OK;
}

static const struct wined3d_resource_ops texture_resource_ops =
{
    texture_resource_incref,
    texture_resource_decref,
    texture_resource_preload,
    wined3d_texture_gl_unload,
    texture_resource_sub_resource_map,
    texture_resource_sub_resource_map_info,
    texture_resource_sub_resource_unmap,
};

static HRESULT wined3d_texture_init(struct wined3d_texture *texture, const struct wined3d_resource_desc *desc,
        unsigned int layer_count, unsigned int level_count, DWORD flags, struct wined3d_device *device,
        void *parent, const struct wined3d_parent_ops *parent_ops, void *sub_resources,
        const struct wined3d_texture_ops *texture_ops)
{
    const struct wined3d_d3d_info *d3d_info = &device->adapter->d3d_info;
    struct wined3d_device_parent *device_parent = device->device_parent;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    unsigned int sub_count, i, j, size, offset = 0;
    unsigned int pow2_width, pow2_height;
    const struct wined3d_format *format;
    HRESULT hr;

    TRACE("texture %p, resource_type %s, format %s, multisample_type %#x, multisample_quality %#x, "
            "usage %s, access %s, width %u, height %u, depth %u, layer_count %u, level_count %u, "
            "flags %#x, device %p, parent %p, parent_ops %p, sub_resources %p, texture_ops %p.\n",
            texture, debug_d3dresourcetype(desc->resource_type), debug_d3dformat(desc->format),
            desc->multisample_type, desc->multisample_quality, debug_d3dusage(desc->usage),
            wined3d_debug_resource_access(desc->access), desc->width, desc->height, desc->depth,
            layer_count, level_count, flags, device, parent, parent_ops, sub_resources, texture_ops);

    if (!desc->width || !desc->height || !desc->depth)
        return WINED3DERR_INVALIDCALL;

    if (desc->resource_type == WINED3D_RTYPE_TEXTURE_3D && layer_count != 1)
    {
        ERR("Invalid layer count for volume texture.\n");
        return E_INVALIDARG;
    }

    texture->sub_resources = sub_resources;

    /* TODO: It should only be possible to create textures for formats
     * that are reported as supported. */
    if (WINED3DFMT_UNKNOWN >= desc->format)
    {
        WARN("Texture cannot be created with a format of WINED3DFMT_UNKNOWN.\n");
        return WINED3DERR_INVALIDCALL;
    }
    format = wined3d_get_format(device->adapter, desc->format, desc->bind_flags);

    if (desc->usage & WINED3DUSAGE_DYNAMIC && (wined3d_resource_access_is_managed(desc->access)
            || desc->usage & WINED3DUSAGE_SCRATCH))
    {
        WARN("Attempted to create a dynamic texture with access %s and usage %s.\n",
                wined3d_debug_resource_access(desc->access), debug_d3dusage(desc->usage));
        return WINED3DERR_INVALIDCALL;
    }

    pow2_width = desc->width;
    pow2_height = desc->height;
    if (((desc->width & (desc->width - 1)) || (desc->height & (desc->height - 1)) || (desc->depth & (desc->depth - 1)))
            && !d3d_info->texture_npot)
    {
        /* level_count == 0 returns an error as well. */
        if (level_count != 1 || layer_count != 1 || desc->resource_type == WINED3D_RTYPE_TEXTURE_3D)
        {
            if (!(desc->usage & WINED3DUSAGE_SCRATCH))
            {
                WARN("Attempted to create a mipmapped/cube/array/volume NPOT "
                        "texture without unconditional NPOT support.\n");
                return WINED3DERR_INVALIDCALL;
            }

            WARN("Creating a scratch mipmapped/cube/array NPOT texture despite lack of HW support.\n");
        }
        texture->flags |= WINED3D_TEXTURE_COND_NP2;

        if (desc->resource_type != WINED3D_RTYPE_TEXTURE_3D && !d3d_info->texture_npot_conditional)
        {
            /* TODO: Add support for non-power-of-two compressed textures. */
            if (format->flags[WINED3D_GL_RES_TYPE_TEX_2D]
                    & (WINED3DFMT_FLAG_COMPRESSED | WINED3DFMT_FLAG_HEIGHT_SCALE))
            {
                FIXME("Compressed or height scaled non-power-of-two (%ux%u) textures are not supported.\n",
                        desc->width, desc->height);
                return WINED3DERR_NOTAVAILABLE;
            }

            /* Find the nearest pow2 match. */
            pow2_width = pow2_height = 1;
            while (pow2_width < desc->width)
                pow2_width <<= 1;
            while (pow2_height < desc->height)
                pow2_height <<= 1;
            texture->flags |= WINED3D_TEXTURE_COND_NP2_EMULATED;
        }
    }
    texture->pow2_width = pow2_width;
    texture->pow2_height = pow2_height;

    if ((pow2_width > d3d_info->limits.texture_size || pow2_height > d3d_info->limits.texture_size)
            && (desc->bind_flags & WINED3D_BIND_SHADER_RESOURCE))
    {
        /* One of four options:
         * 1: Do the same as we do with NPOT and scale the texture. (Any
         *    texture ops would require the texture to be scaled which is
         *    potentially slow.)
         * 2: Set the texture to the maximum size (bad idea).
         * 3: WARN and return WINED3DERR_NOTAVAILABLE.
         * 4: Create the surface, but allow it to be used only for DirectDraw
         *    Blts. Some apps (e.g. Swat 3) create textures with a height of
         *    16 and a width > 3000 and blt 16x16 letter areas from them to
         *    the render target. */
        if (desc->access & WINED3D_RESOURCE_ACCESS_GPU)
        {
            WARN("Dimensions (%ux%u) exceed the maximum texture size.\n", pow2_width, pow2_height);
            return WINED3DERR_NOTAVAILABLE;
        }

        /* We should never use this surface in combination with OpenGL. */
        TRACE("Creating an oversized (%ux%u) surface.\n", pow2_width, pow2_height);
    }

    for (i = 0; i < layer_count; ++i)
    {
        for (j = 0; j < level_count; ++j)
        {
            unsigned int idx = i * level_count + j;

            size = wined3d_format_calculate_size(format, device->surface_alignment,
                    max(1, desc->width >> j), max(1, desc->height >> j), max(1, desc->depth >> j));
            texture->sub_resources[idx].offset = offset;
            texture->sub_resources[idx].size = size;
            offset += size;
        }
        offset = (offset + (RESOURCE_ALIGNMENT - 1)) & ~(RESOURCE_ALIGNMENT - 1);
    }

    if (!offset)
        return WINED3DERR_INVALIDCALL;

    if (FAILED(hr = resource_init(&texture->resource, device, desc->resource_type, format,
            desc->multisample_type, desc->multisample_quality, desc->usage, desc->bind_flags, desc->access,
            desc->width, desc->height, desc->depth, offset, parent, parent_ops, &texture_resource_ops)))
    {
        static unsigned int once;

        /* DXTn 3D textures are not supported. Do not write the ERR for them. */
        if ((desc->format == WINED3DFMT_DXT1 || desc->format == WINED3DFMT_DXT2 || desc->format == WINED3DFMT_DXT3
                || desc->format == WINED3DFMT_DXT4 || desc->format == WINED3DFMT_DXT5)
                && !(format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_TEXTURE)
                && desc->resource_type != WINED3D_RTYPE_TEXTURE_3D && !once++)
            ERR_(winediag)("The application tried to create a DXTn texture, but the driver does not support them.\n");

        WARN("Failed to initialize resource, returning %#x\n", hr);
        return hr;
    }
    wined3d_resource_update_draw_binding(&texture->resource);

    texture->texture_ops = texture_ops;

    texture->layer_count = layer_count;
    texture->level_count = level_count;
    texture->lod = 0;
    texture->flags |= WINED3D_TEXTURE_POW2_MAT_IDENT | WINED3D_TEXTURE_NORMALIZED_COORDS
            | WINED3D_TEXTURE_DOWNLOADABLE;
    if (flags & WINED3D_TEXTURE_CREATE_GET_DC_LENIENT)
        texture->flags |= WINED3D_TEXTURE_PIN_SYSMEM | WINED3D_TEXTURE_GET_DC_LENIENT;
    if (flags & (WINED3D_TEXTURE_CREATE_GET_DC | WINED3D_TEXTURE_CREATE_GET_DC_LENIENT))
        texture->flags |= WINED3D_TEXTURE_GET_DC;
    if (flags & WINED3D_TEXTURE_CREATE_DISCARD)
        texture->flags |= WINED3D_TEXTURE_DISCARD;
    if (flags & WINED3D_TEXTURE_CREATE_GENERATE_MIPMAPS)
    {
        if (!(texture->resource.format_flags & WINED3DFMT_FLAG_GEN_MIPMAP))
            WARN("Format doesn't support mipmaps generation, "
                    "ignoring WINED3D_TEXTURE_CREATE_GENERATE_MIPMAPS flag.\n");
        else
            texture->flags |= WINED3D_TEXTURE_GENERATE_MIPMAPS;
    }

    /* Precalculated scaling for 'faked' non power of two texture coords. */
    if (texture->resource.gl_type == WINED3D_GL_RES_TYPE_TEX_RECT)
    {
        texture->pow2_matrix[0] = (float)desc->width;
        texture->pow2_matrix[5] = (float)desc->height;
        texture->flags &= ~(WINED3D_TEXTURE_POW2_MAT_IDENT | WINED3D_TEXTURE_NORMALIZED_COORDS);
    }
    else if (texture->flags & WINED3D_TEXTURE_COND_NP2_EMULATED)
    {
        texture->pow2_matrix[0] = (((float)desc->width) / ((float)pow2_width));
        texture->pow2_matrix[5] = (((float)desc->height) / ((float)pow2_height));
        texture->flags &= ~WINED3D_TEXTURE_POW2_MAT_IDENT;
    }
    else
    {
        texture->pow2_matrix[0] = 1.0f;
        texture->pow2_matrix[5] = 1.0f;
    }
    texture->pow2_matrix[10] = 1.0f;
    texture->pow2_matrix[15] = 1.0f;
    TRACE("x scale %.8e, y scale %.8e.\n", texture->pow2_matrix[0], texture->pow2_matrix[5]);

    if (wined3d_texture_use_pbo(texture, gl_info))
        texture->resource.map_binding = WINED3D_LOCATION_BUFFER;

    if (desc->resource_type != WINED3D_RTYPE_TEXTURE_3D
            || !wined3d_texture_use_pbo(texture, gl_info))
    {
        if (!wined3d_resource_prepare_sysmem(&texture->resource))
        {
            wined3d_texture_cleanup_sync(texture);
            return E_OUTOFMEMORY;
        }
    }

    sub_count = level_count * layer_count;
    if (sub_count / layer_count != level_count)
    {
        wined3d_texture_cleanup_sync(texture);
        return E_OUTOFMEMORY;
    }

    if (desc->usage & WINED3DUSAGE_OVERLAY)
    {
        if (!(texture->overlay_info = heap_calloc(sub_count, sizeof(*texture->overlay_info))))
        {
            wined3d_texture_cleanup_sync(texture);
            return E_OUTOFMEMORY;
        }

        for (i = 0; i < sub_count; ++i)
        {
            list_init(&texture->overlay_info[i].entry);
            list_init(&texture->overlay_info[i].overlays);
        }
    }

    /* Generate all sub-resources. */
    for (i = 0; i < sub_count; ++i)
    {
        struct wined3d_texture_sub_resource *sub_resource;

        sub_resource = &texture->sub_resources[i];
        sub_resource->locations = WINED3D_LOCATION_DISCARDED;
        if (desc->resource_type != WINED3D_RTYPE_TEXTURE_3D)
        {
            wined3d_texture_validate_location(texture, i, WINED3D_LOCATION_SYSMEM);
            wined3d_texture_invalidate_location(texture, i, ~WINED3D_LOCATION_SYSMEM);
        }

        if (FAILED(hr = device_parent->ops->texture_sub_resource_created(device_parent,
                desc->resource_type, texture, i, &sub_resource->parent, &sub_resource->parent_ops)))
        {
            WARN("Failed to create sub-resource parent, hr %#x.\n", hr);
            sub_resource->parent = NULL;
            wined3d_texture_cleanup_sync(texture);
            return hr;
        }

        TRACE("parent %p, parent_ops %p.\n", sub_resource->parent, sub_resource->parent_ops);

        TRACE("Created sub-resource %u (level %u, layer %u).\n",
                i, i % texture->level_count, i / texture->level_count);

        if (desc->usage & WINED3DUSAGE_OWNDC)
        {
            struct wined3d_texture_idx texture_idx = {texture, i};

            wined3d_cs_init_object(device->cs, wined3d_texture_create_dc, &texture_idx);
            wined3d_cs_finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);
            if (!texture->dc_info || !texture->dc_info[i].dc)
            {
                wined3d_texture_cleanup_sync(texture);
                return WINED3DERR_INVALIDCALL;
            }
        }
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_texture_blt(struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx,
        const RECT *dst_rect, struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx,
        const RECT *src_rect, DWORD flags, const struct wined3d_blt_fx *fx, enum wined3d_texture_filter_type filter)
{
    struct wined3d_box src_box = {src_rect->left, src_rect->top, src_rect->right, src_rect->bottom, 0, 1};
    struct wined3d_box dst_box = {dst_rect->left, dst_rect->top, dst_rect->right, dst_rect->bottom, 0, 1};
    unsigned int dst_format_flags, src_format_flags = 0;
    HRESULT hr;

    TRACE("dst_texture %p, dst_sub_resource_idx %u, dst_rect %s, src_texture %p, "
            "src_sub_resource_idx %u, src_rect %s, flags %#x, fx %p, filter %s.\n",
            dst_texture, dst_sub_resource_idx, wine_dbgstr_rect(dst_rect), src_texture,
            src_sub_resource_idx, wine_dbgstr_rect(src_rect), flags, fx, debug_d3dtexturefiltertype(filter));

    if (dst_sub_resource_idx >= dst_texture->level_count * dst_texture->layer_count
            || dst_texture->resource.type != WINED3D_RTYPE_TEXTURE_2D)
        return WINED3DERR_INVALIDCALL;

    if (src_sub_resource_idx >= src_texture->level_count * src_texture->layer_count
            || src_texture->resource.type != WINED3D_RTYPE_TEXTURE_2D)
        return WINED3DERR_INVALIDCALL;

    dst_format_flags = dst_texture->resource.format_flags;
    if (FAILED(hr = wined3d_texture_check_box_dimensions(dst_texture,
            dst_sub_resource_idx % dst_texture->level_count, &dst_box)))
        return hr;

    src_format_flags = src_texture->resource.format_flags;
    if (FAILED(hr = wined3d_texture_check_box_dimensions(src_texture,
            src_sub_resource_idx % src_texture->level_count, &src_box)))
        return hr;

    if (dst_texture->sub_resources[dst_sub_resource_idx].map_count
            || src_texture->sub_resources[src_sub_resource_idx].map_count)
    {
        WARN("Sub-resource is busy, returning WINEDDERR_SURFACEBUSY.\n");
        return WINEDDERR_SURFACEBUSY;
    }

    if ((src_format_flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL))
            != (dst_format_flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL)))
    {
        WARN("Rejecting depth/stencil blit between incompatible formats.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (dst_texture->resource.device != src_texture->resource.device)
    {
        FIXME("Rejecting cross-device blit.\n");
        return E_NOTIMPL;
    }

    wined3d_cs_emit_blt_sub_resource(dst_texture->resource.device->cs, &dst_texture->resource, dst_sub_resource_idx,
            &dst_box, &src_texture->resource, src_sub_resource_idx, &src_box, flags, fx, filter);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_texture_get_overlay_position(const struct wined3d_texture *texture,
        unsigned int sub_resource_idx, LONG *x, LONG *y)
{
    struct wined3d_overlay_info *overlay;

    TRACE("texture %p, sub_resource_idx %u, x %p, y %p.\n", texture, sub_resource_idx, x, y);

    if (!(texture->resource.usage & WINED3DUSAGE_OVERLAY)
            || sub_resource_idx >= texture->level_count * texture->layer_count)
    {
        WARN("Invalid sub-resource specified.\n");
        return WINEDDERR_NOTAOVERLAYSURFACE;
    }

    overlay = &texture->overlay_info[sub_resource_idx];
    if (!overlay->dst_texture)
    {
        TRACE("Overlay not visible.\n");
        *x = 0;
        *y = 0;
        return WINEDDERR_OVERLAYNOTVISIBLE;
    }

    *x = overlay->dst_rect.left;
    *y = overlay->dst_rect.top;

    TRACE("Returning position %d, %d.\n", *x, *y);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_texture_set_overlay_position(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, LONG x, LONG y)
{
    struct wined3d_overlay_info *overlay;
    LONG w, h;

    TRACE("texture %p, sub_resource_idx %u, x %d, y %d.\n", texture, sub_resource_idx, x, y);

    if (!(texture->resource.usage & WINED3DUSAGE_OVERLAY)
            || sub_resource_idx >= texture->level_count * texture->layer_count)
    {
        WARN("Invalid sub-resource specified.\n");
        return WINEDDERR_NOTAOVERLAYSURFACE;
    }

    overlay = &texture->overlay_info[sub_resource_idx];
    w = overlay->dst_rect.right - overlay->dst_rect.left;
    h = overlay->dst_rect.bottom - overlay->dst_rect.top;
    SetRect(&overlay->dst_rect, x, y, x + w, y + h);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_texture_update_overlay(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        const RECT *src_rect, struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx,
        const RECT *dst_rect, DWORD flags)
{
    struct wined3d_overlay_info *overlay;
    unsigned int level, dst_level;

    TRACE("texture %p, sub_resource_idx %u, src_rect %s, dst_texture %p, "
            "dst_sub_resource_idx %u, dst_rect %s, flags %#x.\n",
            texture, sub_resource_idx, wine_dbgstr_rect(src_rect), dst_texture,
            dst_sub_resource_idx, wine_dbgstr_rect(dst_rect), flags);

    if (!(texture->resource.usage & WINED3DUSAGE_OVERLAY) || texture->resource.type != WINED3D_RTYPE_TEXTURE_2D
            || sub_resource_idx >= texture->level_count * texture->layer_count)
    {
        WARN("Invalid sub-resource specified.\n");
        return WINEDDERR_NOTAOVERLAYSURFACE;
    }

    if (!dst_texture || dst_texture->resource.type != WINED3D_RTYPE_TEXTURE_2D
            || dst_sub_resource_idx >= dst_texture->level_count * dst_texture->layer_count)
    {
        WARN("Invalid destination sub-resource specified.\n");
        return WINED3DERR_INVALIDCALL;
    }

    overlay = &texture->overlay_info[sub_resource_idx];

    level = sub_resource_idx % texture->level_count;
    if (src_rect)
        overlay->src_rect = *src_rect;
    else
        SetRect(&overlay->src_rect, 0, 0,
                wined3d_texture_get_level_width(texture, level),
                wined3d_texture_get_level_height(texture, level));

    dst_level = dst_sub_resource_idx % dst_texture->level_count;
    if (dst_rect)
        overlay->dst_rect = *dst_rect;
    else
        SetRect(&overlay->dst_rect, 0, 0,
                wined3d_texture_get_level_width(dst_texture, dst_level),
                wined3d_texture_get_level_height(dst_texture, dst_level));

    if (overlay->dst_texture && (overlay->dst_texture != dst_texture
            || overlay->dst_sub_resource_idx != dst_sub_resource_idx || flags & WINEDDOVER_HIDE))
    {
        overlay->dst_texture = NULL;
        list_remove(&overlay->entry);
    }

    if (flags & WINEDDOVER_SHOW)
    {
        if (overlay->dst_texture != dst_texture || overlay->dst_sub_resource_idx != dst_sub_resource_idx)
        {
            overlay->dst_texture = dst_texture;
            overlay->dst_sub_resource_idx = dst_sub_resource_idx;
            list_add_tail(&texture->overlay_info[dst_sub_resource_idx].overlays, &overlay->entry);
        }
    }
    else if (flags & WINEDDOVER_HIDE)
    {
        /* Tests show that the rectangles are erased on hide. */
        SetRectEmpty(&overlay->src_rect);
        SetRectEmpty(&overlay->dst_rect);
        overlay->dst_texture = NULL;
    }

    return WINED3D_OK;
}

void * CDECL wined3d_texture_get_sub_resource_parent(struct wined3d_texture *texture, unsigned int sub_resource_idx)
{
    unsigned int sub_count = texture->level_count * texture->layer_count;

    TRACE("texture %p, sub_resource_idx %u.\n", texture, sub_resource_idx);

    if (sub_resource_idx >= sub_count)
    {
        WARN("sub_resource_idx %u >= sub_count %u.\n", sub_resource_idx, sub_count);
        return NULL;
    }

    return texture->sub_resources[sub_resource_idx].parent;
}

void CDECL wined3d_texture_set_sub_resource_parent(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, void *parent)
{
    unsigned int sub_count = texture->level_count * texture->layer_count;

    TRACE("texture %p, sub_resource_idx %u, parent %p.\n", texture, sub_resource_idx, parent);

    if (sub_resource_idx >= sub_count)
    {
        WARN("sub_resource_idx %u >= sub_count %u.\n", sub_resource_idx, sub_count);
        return;
    }

    texture->sub_resources[sub_resource_idx].parent = parent;
}

HRESULT CDECL wined3d_texture_get_sub_resource_desc(const struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_sub_resource_desc *desc)
{
    unsigned int sub_count = texture->level_count * texture->layer_count;
    const struct wined3d_resource *resource;
    unsigned int level_idx;

    TRACE("texture %p, sub_resource_idx %u, desc %p.\n", texture, sub_resource_idx, desc);

    if (sub_resource_idx >= sub_count)
    {
        WARN("sub_resource_idx %u >= sub_count %u.\n", sub_resource_idx, sub_count);
        return WINED3DERR_INVALIDCALL;
    }

    resource = &texture->resource;
    desc->format = resource->format->id;
    desc->multisample_type = resource->multisample_type;
    desc->multisample_quality = resource->multisample_quality;
    desc->usage = resource->usage;
    desc->bind_flags = resource->bind_flags;
    desc->access = resource->access;

    level_idx = sub_resource_idx % texture->level_count;
    desc->width = wined3d_texture_get_level_width(texture, level_idx);
    desc->height = wined3d_texture_get_level_height(texture, level_idx);
    desc->depth = wined3d_texture_get_level_depth(texture, level_idx);
    desc->size = texture->sub_resources[sub_resource_idx].size;

    return WINED3D_OK;
}

HRESULT wined3d_texture_gl_init(struct wined3d_texture_gl *texture_gl, struct wined3d_device *device,
        const struct wined3d_resource_desc *desc, unsigned int layer_count, unsigned int level_count,
        uint32_t flags, void *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    HRESULT hr;

    TRACE("texture_gl %p, device %p, desc %p, layer_count %u, "
            "level_count %u, flags %#x, parent %p, parent_ops %p.\n",
            texture_gl, device, desc, layer_count,
            level_count, flags, parent, parent_ops);

    if (!(desc->usage & WINED3DUSAGE_LEGACY_CUBEMAP) && layer_count > 1
            && !gl_info->supported[EXT_TEXTURE_ARRAY])
    {
        WARN("OpenGL implementation does not support array textures.\n");
        return WINED3DERR_INVALIDCALL;
    }

    switch (desc->resource_type)
    {
        case WINED3D_RTYPE_TEXTURE_1D:
            if (layer_count > 1)
                texture_gl->target = GL_TEXTURE_1D_ARRAY;
            else
                texture_gl->target = GL_TEXTURE_1D;
            break;

        case WINED3D_RTYPE_TEXTURE_2D:
            if (desc->usage & WINED3DUSAGE_LEGACY_CUBEMAP)
            {
                texture_gl->target = GL_TEXTURE_CUBE_MAP_ARB;
            }
            else if (desc->multisample_type && gl_info->supported[ARB_TEXTURE_MULTISAMPLE])
            {
                if (layer_count > 1)
                    texture_gl->target = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
                else
                    texture_gl->target = GL_TEXTURE_2D_MULTISAMPLE;
            }
            else
            {
                if (layer_count > 1)
                    texture_gl->target = GL_TEXTURE_2D_ARRAY;
                else
                    texture_gl->target = GL_TEXTURE_2D;
            }
            break;

        case WINED3D_RTYPE_TEXTURE_3D:
            if (!gl_info->supported[EXT_TEXTURE3D])
            {
                WARN("OpenGL implementation does not support 3D textures.\n");
                return WINED3DERR_INVALIDCALL;
            }
            texture_gl->target = GL_TEXTURE_3D;
            break;

        default:
            ERR("Invalid resource type %s.\n", debug_d3dresourcetype(desc->resource_type));
            return WINED3DERR_INVALIDCALL;
    }

    list_init(&texture_gl->renderbuffers);

    if (FAILED(hr = wined3d_texture_init(&texture_gl->t, desc, layer_count, level_count,
            flags, device, parent, parent_ops, &texture_gl[1], &texture_gl_ops)))
        return hr;

    if (texture_gl->t.resource.gl_type == WINED3D_GL_RES_TYPE_TEX_RECT)
        texture_gl->target = GL_TEXTURE_RECTANGLE_ARB;

    if (texture_gl->target == GL_TEXTURE_2D_MULTISAMPLE_ARRAY || texture_gl->target == GL_TEXTURE_2D_MULTISAMPLE)
        texture_gl->t.flags &= ~WINED3D_TEXTURE_DOWNLOADABLE;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_texture_create(struct wined3d_device *device, const struct wined3d_resource_desc *desc,
        UINT layer_count, UINT level_count, DWORD flags, const struct wined3d_sub_resource_data *data,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_texture **texture)
{
    unsigned int sub_count = level_count * layer_count;
    unsigned int i;
    HRESULT hr;

    TRACE("device %p, desc %p, layer_count %u, level_count %u, flags %#x, data %p, "
            "parent %p, parent_ops %p, texture %p.\n",
            device, desc, layer_count, level_count, flags, data, parent, parent_ops, texture);

    if (!layer_count)
    {
        WARN("Invalid layer count.\n");
        return E_INVALIDARG;
    }
    if ((desc->usage & WINED3DUSAGE_LEGACY_CUBEMAP) && layer_count != 6)
    {
        ERR("Invalid layer count %u for legacy cubemap.\n", layer_count);
        layer_count = 6;
    }

    if (!level_count)
    {
        WARN("Invalid level count.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (desc->multisample_type != WINED3D_MULTISAMPLE_NONE)
    {
        const struct wined3d_format *format = wined3d_get_format(device->adapter, desc->format, desc->bind_flags);

        if (desc->multisample_type == WINED3D_MULTISAMPLE_NON_MASKABLE
                && desc->multisample_quality >= wined3d_popcount(format->multisample_types))
        {
            WARN("Unsupported quality level %u requested for WINED3D_MULTISAMPLE_NON_MASKABLE.\n",
                    desc->multisample_quality);
            return WINED3DERR_NOTAVAILABLE;
        }
        if (desc->multisample_type != WINED3D_MULTISAMPLE_NON_MASKABLE
                && (!(format->multisample_types & 1u << (desc->multisample_type - 1))
                || (desc->multisample_quality && desc->multisample_quality != WINED3D_STANDARD_MULTISAMPLE_PATTERN)))
        {
            WARN("Unsupported multisample type %u quality %u requested.\n", desc->multisample_type,
                    desc->multisample_quality);
            return WINED3DERR_NOTAVAILABLE;
        }
    }

    if (data)
    {
        for (i = 0; i < sub_count; ++i)
        {
            if (data[i].data)
                continue;

            WARN("Invalid sub-resource data specified for sub-resource %u.\n", i);
            return E_INVALIDARG;
        }
    }

    if (FAILED(hr = device->adapter->adapter_ops->adapter_create_texture(device, desc,
            layer_count, level_count, flags, parent, parent_ops, texture)))
        return hr;

    /* FIXME: We'd like to avoid ever allocating system memory for the texture
     * in this case. */
    if (data)
    {
        struct wined3d_box box;

        for (i = 0; i < sub_count; ++i)
        {
            wined3d_texture_get_level_box(*texture, i % (*texture)->level_count, &box);
            wined3d_cs_emit_update_sub_resource(device->cs, &(*texture)->resource,
                    i, &box, data[i].data, data[i].row_pitch, data[i].slice_pitch);
        }
    }

    TRACE("Created texture %p.\n", *texture);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_texture_get_dc(struct wined3d_texture *texture, unsigned int sub_resource_idx, HDC *dc)
{
    struct wined3d_device *device = texture->resource.device;
    struct wined3d_texture_sub_resource *sub_resource;
    struct wined3d_dc_info *dc_info;

    TRACE("texture %p, sub_resource_idx %u, dc %p.\n", texture, sub_resource_idx, dc);

    if (!(texture->flags & WINED3D_TEXTURE_GET_DC))
    {
        WARN("Texture does not support GetDC\n");
        /* Don't touch the DC */
        return WINED3DERR_INVALIDCALL;
    }

    if (!(sub_resource = wined3d_texture_get_sub_resource(texture, sub_resource_idx)))
        return WINED3DERR_INVALIDCALL;

    if (texture->resource.type != WINED3D_RTYPE_TEXTURE_2D)
    {
        WARN("Not supported on %s resources.\n", debug_d3dresourcetype(texture->resource.type));
        return WINED3DERR_INVALIDCALL;
    }

    if (texture->resource.map_count && !(texture->flags & WINED3D_TEXTURE_GET_DC_LENIENT))
        return WINED3DERR_INVALIDCALL;

    if (!(dc_info = texture->dc_info) || !dc_info[sub_resource_idx].dc)
    {
        struct wined3d_texture_idx texture_idx = {texture, sub_resource_idx};

        wined3d_cs_init_object(device->cs, wined3d_texture_create_dc, &texture_idx);
        wined3d_cs_finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);
        if (!(dc_info = texture->dc_info) || !dc_info[sub_resource_idx].dc)
            return WINED3DERR_INVALIDCALL;
    }

    if (!(texture->flags & WINED3D_TEXTURE_GET_DC_LENIENT))
        texture->flags |= WINED3D_TEXTURE_DC_IN_USE;
    ++texture->resource.map_count;
    ++sub_resource->map_count;

    *dc = dc_info[sub_resource_idx].dc;
    TRACE("Returning dc %p.\n", *dc);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_texture_release_dc(struct wined3d_texture *texture, unsigned int sub_resource_idx, HDC dc)
{
    struct wined3d_device *device = texture->resource.device;
    struct wined3d_texture_sub_resource *sub_resource;
    struct wined3d_dc_info *dc_info;

    TRACE("texture %p, sub_resource_idx %u, dc %p.\n", texture, sub_resource_idx, dc);

    if (!(sub_resource = wined3d_texture_get_sub_resource(texture, sub_resource_idx)))
        return WINED3DERR_INVALIDCALL;

    if (texture->resource.type != WINED3D_RTYPE_TEXTURE_2D)
    {
        WARN("Not supported on %s resources.\n", debug_d3dresourcetype(texture->resource.type));
        return WINED3DERR_INVALIDCALL;
    }

    if (!(texture->flags & (WINED3D_TEXTURE_GET_DC_LENIENT | WINED3D_TEXTURE_DC_IN_USE)))
        return WINED3DERR_INVALIDCALL;

    if (!(dc_info = texture->dc_info) || dc_info[sub_resource_idx].dc != dc)
    {
        WARN("Application tries to release invalid DC %p, sub-resource DC is %p.\n",
                dc, dc_info ? dc_info[sub_resource_idx].dc : NULL);
        return WINED3DERR_INVALIDCALL;
    }

    if (!(texture->resource.usage & WINED3DUSAGE_OWNDC))
    {
        struct wined3d_texture_idx texture_idx = {texture, sub_resource_idx};

        wined3d_cs_destroy_object(device->cs, wined3d_texture_destroy_dc, &texture_idx);
        wined3d_cs_finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);
    }

    --sub_resource->map_count;
    if (!--texture->resource.map_count && texture->update_map_binding)
        wined3d_texture_update_map_binding(texture);
    if (!(texture->flags & WINED3D_TEXTURE_GET_DC_LENIENT))
        texture->flags &= ~WINED3D_TEXTURE_DC_IN_USE;

    return WINED3D_OK;
}

void wined3d_texture_upload_from_texture(struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx,
        unsigned int dst_x, unsigned int dst_y, unsigned int dst_z, struct wined3d_texture *src_texture,
        unsigned int src_sub_resource_idx, const struct wined3d_box *src_box)
{
    unsigned int src_row_pitch, src_slice_pitch;
    unsigned int update_w, update_h, update_d;
    unsigned int src_level, dst_level;
    struct wined3d_context *context;
    struct wined3d_bo_address data;

    TRACE("dst_texture %p, dst_sub_resource_idx %u, dst_x %u, dst_y %u, dst_z %u, "
            "src_texture %p, src_sub_resource_idx %u, src_box %s.\n",
            dst_texture, dst_sub_resource_idx, dst_x, dst_y, dst_z,
            src_texture, src_sub_resource_idx, debug_box(src_box));

    context = context_acquire(dst_texture->resource.device, NULL, 0);

    /* Only load the sub-resource for partial updates. For newly allocated
     * textures the texture wouldn't be the current location, and we'd upload
     * zeroes just to overwrite them again. */
    update_w = src_box->right - src_box->left;
    update_h = src_box->bottom - src_box->top;
    update_d = src_box->back - src_box->front;
    dst_level = dst_sub_resource_idx % dst_texture->level_count;
    if (update_w == wined3d_texture_get_level_width(dst_texture, dst_level)
            && update_h == wined3d_texture_get_level_height(dst_texture, dst_level)
            && update_d == wined3d_texture_get_level_depth(dst_texture, dst_level))
        wined3d_texture_prepare_location(dst_texture, dst_sub_resource_idx, context, WINED3D_LOCATION_TEXTURE_RGB);
    else
        wined3d_texture_load_location(dst_texture, dst_sub_resource_idx, context, WINED3D_LOCATION_TEXTURE_RGB);

    src_level = src_sub_resource_idx % src_texture->level_count;
    wined3d_texture_get_memory(src_texture, src_sub_resource_idx, &data,
            src_texture->sub_resources[src_sub_resource_idx].locations);
    wined3d_texture_get_pitch(src_texture, src_level, &src_row_pitch, &src_slice_pitch);

    dst_texture->texture_ops->texture_upload_data(context, wined3d_const_bo_address(&data),
            src_texture->resource.format, src_box, src_row_pitch, src_slice_pitch, dst_texture,
            dst_sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB, dst_x, dst_y, dst_z);

    context_release(context);

    wined3d_texture_validate_location(dst_texture, dst_sub_resource_idx, WINED3D_LOCATION_TEXTURE_RGB);
    wined3d_texture_invalidate_location(dst_texture, dst_sub_resource_idx, ~WINED3D_LOCATION_TEXTURE_RGB);
}

/* Partial downloads are not supported. */
void wined3d_texture_download_from_texture(struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx,
        struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx)
{
    unsigned int src_level, dst_level, dst_row_pitch, dst_slice_pitch;
    unsigned int dst_location = dst_texture->resource.map_binding;
    struct wined3d_context *context;
    struct wined3d_bo_address data;
    struct wined3d_box src_box;
    unsigned int src_location;

    context = context_acquire(src_texture->resource.device, NULL, 0);

    wined3d_texture_prepare_location(dst_texture, dst_sub_resource_idx, context, dst_location);
    wined3d_texture_get_memory(dst_texture, dst_sub_resource_idx, &data, dst_location);

    if (src_texture->sub_resources[src_sub_resource_idx].locations & WINED3D_LOCATION_TEXTURE_RGB)
        src_location = WINED3D_LOCATION_TEXTURE_RGB;
    else
        src_location = WINED3D_LOCATION_TEXTURE_SRGB;
    src_level = src_sub_resource_idx % src_texture->level_count;
    wined3d_texture_get_level_box(src_texture, src_level, &src_box);

    dst_level = dst_sub_resource_idx % dst_texture->level_count;
    wined3d_texture_get_pitch(dst_texture, dst_level, &dst_row_pitch, &dst_slice_pitch);

    src_texture->texture_ops->texture_download_data(context, src_texture, src_sub_resource_idx, src_location,
            &src_box, &data, dst_texture->resource.format, 0, 0, 0, dst_row_pitch, dst_slice_pitch);

    context_release(context);

    wined3d_texture_validate_location(dst_texture, dst_sub_resource_idx, dst_location);
    wined3d_texture_invalidate_location(dst_texture, dst_sub_resource_idx, ~dst_location);
}

static void wined3d_texture_no3d_upload_data(struct wined3d_context *context,
        const struct wined3d_const_bo_address *src_bo_addr, const struct wined3d_format *src_format,
        const struct wined3d_box *src_box, unsigned int src_row_pitch, unsigned int src_slice_pitch,
        struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx, unsigned int dst_location,
        unsigned int dst_x, unsigned int dst_y, unsigned int dst_z)
{
    FIXME("Not implemented.\n");
}

static void wined3d_texture_no3d_download_data(struct wined3d_context *context,
        struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx, unsigned int src_location,
        const struct wined3d_box *src_box, const struct wined3d_bo_address *dst_bo_addr,
        const struct wined3d_format *dst_format, unsigned int dst_x, unsigned int dst_y, unsigned int dst_z,
        unsigned int dst_row_pitch, unsigned int dst_slice_pitch)
{
    FIXME("Not implemented.\n");
}

static BOOL wined3d_texture_no3d_prepare_location(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_context *context, unsigned int location)
{
    switch (location)
    {
        case WINED3D_LOCATION_SYSMEM:
            return wined3d_resource_prepare_sysmem(&texture->resource);

        case WINED3D_LOCATION_USER_MEMORY:
            if (!texture->user_memory)
                ERR("Preparing WINED3D_LOCATION_USER_MEMORY, but texture->user_memory is NULL.\n");
            return TRUE;

        default:
            FIXME("Unhandled location %s.\n", wined3d_debug_location(location));
            return FALSE;
    }
}

static BOOL wined3d_texture_no3d_load_location(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_context *context, DWORD location)
{
    TRACE("texture %p, sub_resource_idx %u, context %p, location %s.\n",
            texture, sub_resource_idx, context, wined3d_debug_location(location));

    if (location == WINED3D_LOCATION_USER_MEMORY || location == WINED3D_LOCATION_SYSMEM)
        return TRUE;

    ERR("Unhandled location %s.\n", wined3d_debug_location(location));

    return FALSE;
}

static const struct wined3d_texture_ops wined3d_texture_no3d_ops =
{
    wined3d_texture_no3d_prepare_location,
    wined3d_texture_no3d_load_location,
    wined3d_texture_no3d_upload_data,
    wined3d_texture_no3d_download_data,
};

HRESULT wined3d_texture_no3d_init(struct wined3d_texture *texture_no3d, struct wined3d_device *device,
        const struct wined3d_resource_desc *desc, unsigned int layer_count, unsigned int level_count,
        uint32_t flags, void *parent, const struct wined3d_parent_ops *parent_ops)
{
    TRACE("texture_no3d %p, device %p, desc %p, layer_count %u, "
            "level_count %u, flags %#x, parent %p, parent_ops %p.\n",
            texture_no3d, device, desc, layer_count,
            level_count, flags, parent, parent_ops);

    return wined3d_texture_init(texture_no3d, desc, layer_count, level_count,
            flags, device, parent, parent_ops, &texture_no3d[1], &wined3d_texture_no3d_ops);
}

static void wined3d_texture_vk_upload_data(struct wined3d_context *context,
        const struct wined3d_const_bo_address *src_bo_addr, const struct wined3d_format *src_format,
        const struct wined3d_box *src_box, unsigned int src_row_pitch, unsigned int src_slice_pitch,
        struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx, unsigned int dst_location,
        unsigned int dst_x, unsigned int dst_y, unsigned int dst_z)
{
    FIXME("Not implemented.\n");
}

static void wined3d_texture_vk_download_data(struct wined3d_context *context,
        struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx, unsigned int src_location,
        const struct wined3d_box *src_box, const struct wined3d_bo_address *dst_bo_addr,
        const struct wined3d_format *dst_format, unsigned int dst_x, unsigned int dst_y, unsigned int dst_z,
        unsigned int dst_row_pitch, unsigned int dst_slice_pitch)
{
    FIXME("Not implemented.\n");
}

static BOOL wined3d_texture_vk_prepare_location(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_context *context, unsigned int location)
{
    switch (location)
    {
        case WINED3D_LOCATION_SYSMEM:
            return wined3d_resource_prepare_sysmem(&texture->resource);

        case WINED3D_LOCATION_USER_MEMORY:
            if (!texture->user_memory)
                ERR("Preparing WINED3D_LOCATION_USER_MEMORY, but texture->user_memory is NULL.\n");
            return TRUE;

        case WINED3D_LOCATION_TEXTURE_RGB:
            /* The Vulkan image is created during resource creation. */
            return TRUE;

        default:
            FIXME("Unhandled location %s.\n", wined3d_debug_location(location));
            return FALSE;
    }
}

static BOOL wined3d_texture_vk_load_location(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, struct wined3d_context *context, DWORD location)
{
    FIXME("Not implemented.\n");

    return FALSE;
}

static const struct wined3d_texture_ops wined3d_texture_vk_ops =
{
    wined3d_texture_vk_prepare_location,
    wined3d_texture_vk_load_location,
    wined3d_texture_vk_upload_data,
    wined3d_texture_vk_download_data,
};

HRESULT wined3d_texture_vk_init(struct wined3d_texture_vk *texture_vk, struct wined3d_device *device,
        const struct wined3d_resource_desc *desc, unsigned int layer_count, unsigned int level_count,
        uint32_t flags, void *parent, const struct wined3d_parent_ops *parent_ops)
{
    TRACE("texture_vk %p, device %p, desc %p, layer_count %u, "
            "level_count %u, flags %#x, parent %p, parent_ops %p.\n",
            texture_vk, device, desc, layer_count,
            level_count, flags, parent, parent_ops);

    return wined3d_texture_init(&texture_vk->t, desc, layer_count, level_count,
            flags, device, parent, parent_ops, &texture_vk[1], &wined3d_texture_vk_ops);
}
