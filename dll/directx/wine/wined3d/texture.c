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

static BOOL wined3d_texture_use_pbo(const struct wined3d_texture *texture, const struct wined3d_gl_info *gl_info)
{
    return !(texture->resource.access & WINED3D_RESOURCE_ACCESS_CPU)
            && texture->resource.usage & WINED3DUSAGE_DYNAMIC
            && gl_info->supported[ARB_PIXEL_BUFFER_OBJECT]
            && !texture->resource.format->conv_byte_count
            && !(texture->flags & (WINED3D_TEXTURE_PIN_SYSMEM | WINED3D_TEXTURE_COND_NP2_EMULATED));
}

static BOOL wined3d_texture_use_immutable_storage(const struct wined3d_texture *texture,
        const struct wined3d_gl_info *gl_info)
{
    /* We don't expect to create texture views for textures with height-scaled formats.
     * Besides, ARB_texture_storage doesn't allow specifying exact sizes for all levels. */
    return gl_info->supported[ARB_TEXTURE_STORAGE]
            && !(texture->resource.format_flags & WINED3DFMT_FLAG_HEIGHT_SCALE);
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

static BOOL is_power_of_two(UINT x)
{
    return (x != 0) && !(x & (x - 1));
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
        gl_info = context->gl_info;
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
        gl_info = context->gl_info;
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
    static const DWORD sysmem_locations = WINED3D_LOCATION_SYSMEM | WINED3D_LOCATION_USER_MEMORY
            | WINED3D_LOCATION_BUFFER;
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

    if ((location & sysmem_locations) && (current & sysmem_locations))
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
#if !defined(STAGING_CSMT)
        data->buffer_object = sub_resource->buffer_object;
#else  /* STAGING_CSMT */
        data->buffer_object = sub_resource->buffer->name;
#endif /* STAGING_CSMT */
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

static HRESULT wined3d_texture_init(struct wined3d_texture *texture, const struct wined3d_texture_ops *texture_ops,
        UINT layer_count, UINT level_count, const struct wined3d_resource_desc *desc, DWORD flags,
        struct wined3d_device *device, void *parent, const struct wined3d_parent_ops *parent_ops,
        const struct wined3d_resource_ops *resource_ops)
{
    unsigned int i, j, size, offset = 0;
    const struct wined3d_format *format;
    HRESULT hr;

    TRACE("texture %p, texture_ops %p, layer_count %u, level_count %u, resource_type %s, format %s, "
            "multisample_type %#x, multisample_quality %#x, usage %s, access %s, width %u, height %u, depth %u, "
            "flags %#x, device %p, parent %p, parent_ops %p, resource_ops %p.\n",
            texture, texture_ops, layer_count, level_count, debug_d3dresourcetype(desc->resource_type),
            debug_d3dformat(desc->format), desc->multisample_type, desc->multisample_quality,
            debug_d3dusage(desc->usage), wined3d_debug_resource_access(desc->access),
            desc->width, desc->height, desc->depth, flags, device, parent, parent_ops, resource_ops);

    if (!desc->width || !desc->height || !desc->depth)
        return WINED3DERR_INVALIDCALL;

    format = wined3d_get_format(&device->adapter->gl_info, desc->format, desc->usage);

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
            desc->multisample_type, desc->multisample_quality, desc->usage, desc->access,
            desc->width, desc->height, desc->depth, offset, parent, parent_ops, resource_ops)))
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
    if ((flags & WINED3D_TEXTURE_CREATE_MAPPABLE) || desc->format == WINED3DFMT_D16_LOCKABLE)
        texture->resource.access |= WINED3D_RESOURCE_ACCESS_MAP_R | WINED3D_RESOURCE_ACCESS_MAP_W;

    texture->texture_ops = texture_ops;

    texture->layer_count = layer_count;
    texture->level_count = level_count;
    texture->lod = 0;
    texture->flags |= WINED3D_TEXTURE_POW2_MAT_IDENT | WINED3D_TEXTURE_NORMALIZED_COORDS;
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

    return WINED3D_OK;
}

/* Context activation is done by the caller. */
static void wined3d_texture_remove_buffer_object(struct wined3d_texture *texture,
#if !defined(STAGING_CSMT)
        unsigned int sub_resource_idx, const struct wined3d_gl_info *gl_info)
{
    GLuint *buffer_object = &texture->sub_resources[sub_resource_idx].buffer_object;

    GL_EXTCALL(glDeleteBuffers(1, buffer_object));
    checkGLcall("glDeleteBuffers");

    TRACE("Deleted buffer object %u for texture %p, sub-resource %u.\n",
            *buffer_object, texture, sub_resource_idx);

    wined3d_texture_invalidate_location(texture, sub_resource_idx, WINED3D_LOCATION_BUFFER);
    *buffer_object = 0;
#else  /* STAGING_CSMT */
        unsigned int sub_resource_idx, struct wined3d_context *context)
{
    struct wined3d_gl_bo *buffer = texture->sub_resources[sub_resource_idx].buffer;
    GLuint name = buffer->name;

    wined3d_device_release_bo(texture->resource.device, buffer, context);
    texture->sub_resources[sub_resource_idx].buffer = NULL;
    wined3d_texture_invalidate_location(texture, sub_resource_idx, WINED3D_LOCATION_BUFFER);

    TRACE("Deleted buffer object %u for texture %p, sub-resource %u.\n",
            name, texture, sub_resource_idx);
#endif /* STAGING_CSMT */
}

static void wined3d_texture_update_map_binding(struct wined3d_texture *texture)
{
    unsigned int sub_count = texture->level_count * texture->layer_count;
    const struct wined3d_device *device = texture->resource.device;
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
#if !defined(STAGING_CSMT)
            wined3d_texture_remove_buffer_object(texture, i, context->gl_info);
#else  /* STAGING_CSMT */
            wined3d_texture_remove_buffer_object(texture, i, context);
#endif /* STAGING_CSMT */
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

static unsigned int wined3d_texture_get_gl_sample_count(const struct wined3d_texture *texture)
{
    const struct wined3d_format *format = texture->resource.format;

    /* TODO: NVIDIA expose their Coverage Sample Anti-Aliasing (CSAA)
     * feature through type == MULTISAMPLE_XX and quality != 0. This could
     * be mapped to GL_NV_framebuffer_multisample_coverage.
     *
     * AMD have a similar feature called Enhanced Quality Anti-Aliasing
     * (EQAA), but it does not have an equivalent OpenGL extension. */

    /* We advertise as many WINED3D_MULTISAMPLE_NON_MASKABLE quality
     * levels as the count of advertised multisample types for the texture
     * format. */
    if (texture->resource.multisample_type == WINED3D_MULTISAMPLE_NON_MASKABLE)
    {
        unsigned int i, count = 0;

        for (i = 0; i < sizeof(format->multisample_types) * CHAR_BIT; ++i)
        {
            if (format->multisample_types & 1u << i)
            {
                if (texture->resource.multisample_quality == count++)
                    break;
            }
        }
        return i + 1;
    }

    return texture->resource.multisample_type;
}

/* Context activation is done by the caller. */
/* The caller is responsible for binding the correct texture. */
static void wined3d_texture_allocate_gl_mutable_storage(struct wined3d_texture *texture,
        GLenum gl_internal_format, const struct wined3d_format *format,
        const struct wined3d_gl_info *gl_info)
{
    unsigned int level, level_count, layer, layer_count;
    GLsizei width, height;
    GLenum target;

    level_count = texture->level_count;
    layer_count = texture->target == GL_TEXTURE_2D_ARRAY ? 1 : texture->layer_count;

    for (layer = 0; layer < layer_count; ++layer)
    {
        target = wined3d_texture_get_sub_resource_target(texture, layer * level_count);

        for (level = 0; level < level_count; ++level)
        {
            width = wined3d_texture_get_level_pow2_width(texture, level);
            height = wined3d_texture_get_level_pow2_height(texture, level);
            if (texture->resource.format_flags & WINED3DFMT_FLAG_HEIGHT_SCALE)
            {
                height *= format->height_scale.numerator;
                height /= format->height_scale.denominator;
            }

            TRACE("texture %p, layer %u, level %u, target %#x, width %u, height %u.\n",
                    texture, layer, level, target, width, height);

            if (texture->target == GL_TEXTURE_2D_ARRAY)
            {
                GL_EXTCALL(glTexImage3D(target, level, gl_internal_format, width, height,
                        texture->layer_count, 0, format->glFormat, format->glType, NULL));
                checkGLcall("glTexImage3D");
            }
            else
            {
                gl_info->gl_ops.gl.p_glTexImage2D(target, level, gl_internal_format,
                        width, height, 0, format->glFormat, format->glType, NULL);
                checkGLcall("glTexImage2D");
            }
        }
    }
}

/* Context activation is done by the caller. */
/* The caller is responsible for binding the correct texture. */
static void wined3d_texture_allocate_gl_immutable_storage(struct wined3d_texture *texture,
        GLenum gl_internal_format, const struct wined3d_gl_info *gl_info)
{
    unsigned int samples = wined3d_texture_get_gl_sample_count(texture);
    GLsizei height = wined3d_texture_get_level_pow2_height(texture, 0);
    GLsizei width = wined3d_texture_get_level_pow2_width(texture, 0);

    switch (texture->target)
    {
        case GL_TEXTURE_2D_ARRAY:
            GL_EXTCALL(glTexStorage3D(texture->target, texture->level_count,
                    gl_internal_format, width, height, texture->layer_count));
            break;
        case GL_TEXTURE_2D_MULTISAMPLE:
            GL_EXTCALL(glTexStorage2DMultisample(texture->target, samples,
                    gl_internal_format, width, height, GL_FALSE));
            break;
        case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
            GL_EXTCALL(glTexStorage3DMultisample(texture->target, samples,
                    gl_internal_format, width, height, texture->layer_count, GL_FALSE));
            break;
        default:
            GL_EXTCALL(glTexStorage2D(texture->target, texture->level_count,
                    gl_internal_format, width, height));
            break;
    }

    checkGLcall("allocate immutable storage");
}

static void wined3d_texture_unload_gl_texture(struct wined3d_texture *texture)
{
    struct wined3d_device *device = texture->resource.device;
    const struct wined3d_gl_info *gl_info = NULL;
    struct wined3d_context *context = NULL;

    if (texture->texture_rgb.name || texture->texture_srgb.name
            || texture->rb_multisample || texture->rb_resolved)
    {
        context = context_acquire(device, NULL, 0);
        gl_info = context->gl_info;
    }

    if (texture->texture_rgb.name)
        gltexture_delete(device, context->gl_info, &texture->texture_rgb);

    if (texture->texture_srgb.name)
        gltexture_delete(device, context->gl_info, &texture->texture_srgb);

    if (texture->rb_multisample)
    {
        TRACE("Deleting multisample renderbuffer %u.\n", texture->rb_multisample);
        context_gl_resource_released(device, texture->rb_multisample, TRUE);
        gl_info->fbo_ops.glDeleteRenderbuffers(1, &texture->rb_multisample);
        texture->rb_multisample = 0;
    }

    if (texture->rb_resolved)
    {
        TRACE("Deleting resolved renderbuffer %u.\n", texture->rb_resolved);
        context_gl_resource_released(device, texture->rb_resolved, TRUE);
        gl_info->fbo_ops.glDeleteRenderbuffers(1, &texture->rb_resolved);
        texture->rb_resolved = 0;
    }

    if (context) context_release(context);

    wined3d_texture_set_dirty(texture);

    resource_unload(&texture->resource);
}

static void wined3d_texture_sub_resources_destroyed(struct wined3d_texture *texture)
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

static void wined3d_texture_cleanup(struct wined3d_texture *texture)
{
    unsigned int sub_count = texture->level_count * texture->layer_count;
    struct wined3d_device *device = texture->resource.device;
    struct wined3d_context *context = NULL;
#if !defined(STAGING_CSMT)
    const struct wined3d_gl_info *gl_info;
    GLuint buffer_object;
#else  /* STAGING_CSMT */
    struct wined3d_gl_bo *buffer;
#endif /* STAGING_CSMT */
    unsigned int i;

    TRACE("texture %p.\n", texture);

    for (i = 0; i < sub_count; ++i)
    {
#if !defined(STAGING_CSMT)
        if (!(buffer_object = texture->sub_resources[i].buffer_object))
            continue;

        TRACE("Deleting buffer object %u.\n", buffer_object);
#else  /* STAGING_CSMT */
        if (!(buffer = texture->sub_resources[i].buffer))
            continue;

        TRACE("Deleting buffer object %u.\n", buffer->name);
#endif /* STAGING_CSMT */

        /* We may not be able to get a context in wined3d_texture_cleanup() in
         * general, but if a buffer object was previously created we can. */
        if (!context)
#if !defined(STAGING_CSMT)
        {
            context = context_acquire(device, NULL, 0);
            gl_info = context->gl_info;
        }

        GL_EXTCALL(glDeleteBuffers(1, &buffer_object));
#else  /* STAGING_CSMT */
            context = context_acquire(device, NULL, 0);

        wined3d_device_release_bo(device, buffer, context);
        texture->sub_resources[i].buffer = NULL;
#endif /* STAGING_CSMT */
    }
    if (context)
        context_release(context);

    texture->texture_ops->texture_cleanup_sub_resources(texture);
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
    wined3d_texture_unload_gl_texture(texture);
}

void wined3d_texture_set_swapchain(struct wined3d_texture *texture, struct wined3d_swapchain *swapchain)
{
    texture->swapchain = swapchain;
    wined3d_resource_update_draw_binding(&texture->resource);
}

/* Context activation is done by the caller. */
void wined3d_texture_bind(struct wined3d_texture *texture,
        struct wined3d_context *context, BOOL srgb)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    const struct wined3d_format *format = texture->resource.format;
    const struct color_fixup_desc fixup = format->color_fixup;
    struct gl_texture *gl_tex;
    GLenum target;

    TRACE("texture %p, context %p, srgb %#x.\n", texture, context, srgb);

    if (!needs_separate_srgb_gl_texture(context, texture))
        srgb = FALSE;

    /* sRGB mode cache for preload() calls outside drawprim. */
    if (srgb)
        texture->flags |= WINED3D_TEXTURE_IS_SRGB;
    else
        texture->flags &= ~WINED3D_TEXTURE_IS_SRGB;

    gl_tex = wined3d_texture_get_gl_texture(texture, srgb);
    target = texture->target;

    if (gl_tex->name)
    {
        context_bind_texture(context, target, gl_tex->name);
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
    if (context->gl_info->supported[EXT_TEXTURE_SRGB_DECODE])
        gl_tex->sampler_desc.srgb_decode = TRUE;
    else
        gl_tex->sampler_desc.srgb_decode = srgb;
    gl_tex->base_level = 0;
    wined3d_texture_set_dirty(texture);

    context_bind_texture(context, target, gl_tex->name);

    /* For a new texture we have to set the texture levels after binding the
     * texture. Beware that texture rectangles do not support mipmapping, but
     * set the maxmiplevel if we're relying on the partial
     * GL_ARB_texture_non_power_of_two emulation with texture rectangles.
     * (I.e., do not care about cond_np2 here, just look for
     * GL_TEXTURE_RECTANGLE_ARB.) */
    if (target != GL_TEXTURE_RECTANGLE_ARB)
    {
        TRACE("Setting GL_TEXTURE_MAX_LEVEL to %u.\n", texture->level_count - 1);
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, texture->level_count - 1);
        checkGLcall("glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, texture->level_count)");
    }

    if (target == GL_TEXTURE_CUBE_MAP_ARB)
    {
        /* Cubemaps are always set to clamp, regardless of the sampler state. */
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    if (texture->flags & WINED3D_TEXTURE_COND_NP2)
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

    if (!is_identity_fixup(fixup) && can_use_texture_swizzle(gl_info, format))
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
        struct
        {
            GLint x, y, z, w;
        }
        swizzle;

        swizzle.x = swizzle_source[fixup.x_source];
        swizzle.y = swizzle_source[fixup.y_source];
        swizzle.z = swizzle_source[fixup.z_source];
        swizzle.w = swizzle_source[fixup.w_source];
        gl_info->gl_ops.gl.p_glTexParameteriv(target, GL_TEXTURE_SWIZZLE_RGBA, &swizzle.x);
        checkGLcall("glTexParameteriv(GL_TEXTURE_SWIZZLE_RGBA)");
    }
}

/* Context activation is done by the caller. */
void wined3d_texture_bind_and_dirtify(struct wined3d_texture *texture,
        struct wined3d_context *context, BOOL srgb)
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
    if (context->active_texture < ARRAY_SIZE(context->rev_tex_unit_map))
    {
        DWORD active_sampler = context->rev_tex_unit_map[context->active_texture];
        if (active_sampler != WINED3D_UNMAPPED_STAGE)
            context_invalidate_state(context, STATE_SAMPLER(active_sampler));
    }
    /* FIXME: Ideally we'd only do this when touching a binding that's used by
     * a shader. */
    context_invalidate_compute_state(context, STATE_COMPUTE_SHADER_RESOURCE_BINDING);
    context_invalidate_state(context, STATE_GRAPHICS_SHADER_RESOURCE_BINDING);

    wined3d_texture_bind(texture, context, srgb);
}

/* Context activation is done by the caller (state handler). */
/* This function relies on the correct texture being bound and loaded. */
void wined3d_texture_apply_sampler_desc(struct wined3d_texture *texture,
        const struct wined3d_sampler_desc *sampler_desc, const struct wined3d_context *context)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    GLenum target = texture->target;
    struct gl_texture *gl_tex;
    DWORD state;

    TRACE("texture %p, sampler_desc %p, context %p.\n", texture, sampler_desc, context);

    gl_tex = wined3d_texture_get_gl_texture(texture, texture->flags & WINED3D_TEXTURE_IS_SRGB);

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
            && (context->d3d_info->wined3d_creation_flags & WINED3D_SRGB_READ_WRITE_CONTROL)
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

static void wined3d_texture_cleanup_sync(struct wined3d_texture *texture)
{
    wined3d_texture_sub_resources_destroyed(texture);
    resource_cleanup(&texture->resource);
    wined3d_resource_wait_idle(&texture->resource);
    wined3d_texture_cleanup(texture);
}

static void wined3d_texture_destroy_object(void *object)
{
    wined3d_texture_cleanup(object);
    heap_free(object);
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
         * wined3d_texture_destroy_object() can't access that memory either. */
        if (texture->user_memory)
            wined3d_resource_wait_idle(&texture->resource);
        wined3d_texture_sub_resources_destroyed(texture);
        texture->resource.parent_ops->wined3d_object_destroyed(texture->resource.parent);
        resource_cleanup(&texture->resource);
        wined3d_cs_destroy_object(texture->resource.device->cs, wined3d_texture_destroy_object, texture);
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
    DWORD old = texture->lod;

    TRACE("texture %p, lod %u.\n", texture, lod);

    /* The d3d9:texture test shows that SetLOD is ignored on non-managed
     * textures. The call always returns 0, and GetLOD always returns 0. */
    if (!wined3d_resource_access_is_managed(texture->resource.access))
    {
        TRACE("Ignoring LOD on texture with resource access %s.\n",
                wined3d_debug_resource_access(texture->resource.access));
        return 0;
    }

    if (lod >= texture->level_count)
        lod = texture->level_count - 1;

    if (texture->lod != lod)
    {
        struct wined3d_device *device = texture->resource.device;

        wined3d_resource_wait_idle(&texture->resource);
        texture->lod = lod;

        texture->texture_rgb.base_level = ~0u;
        texture->texture_srgb.base_level = ~0u;
        if (texture->resource.bind_count)
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

static void texture2d_create_dc(void *object)
{
    struct wined3d_surface *surface = object;
    struct wined3d_context *context = NULL;
    const struct wined3d_format *format;
    unsigned int row_pitch, slice_pitch;
    struct wined3d_texture *texture;
    struct wined3d_bo_address data;
    D3DKMT_CREATEDCFROMMEMORY desc;
    unsigned int sub_resource_idx;
    struct wined3d_device *device;
    NTSTATUS status;

    TRACE("surface %p.\n", surface);

    texture = surface->container;
    sub_resource_idx = surface_get_sub_resource_idx(surface);
    device = texture->resource.device;

    format = texture->resource.format;
    if (!format->ddi_format)
    {
        WARN("Cannot create a DC for format %s.\n", debug_d3dformat(format->id));
        return;
    }

    if (device->d3d_initialized)
        context = context_acquire(device, NULL, 0);

    wined3d_texture_load_location(texture, sub_resource_idx, context, texture->resource.map_binding);
    wined3d_texture_invalidate_location(texture, sub_resource_idx, ~texture->resource.map_binding);
    wined3d_texture_get_pitch(texture, surface->texture_level, &row_pitch, &slice_pitch);
    wined3d_texture_get_memory(texture, sub_resource_idx, &data, texture->resource.map_binding);
    desc.pMemory = context_map_bo_address(context, &data,
            texture->sub_resources[sub_resource_idx].size,
            GL_PIXEL_UNPACK_BUFFER, WINED3D_MAP_READ | WINED3D_MAP_WRITE);

    if (context)
        context_release(context);

    desc.Format = format->ddi_format;
    desc.Width = wined3d_texture_get_level_width(texture, surface->texture_level);
    desc.Height = wined3d_texture_get_level_height(texture, surface->texture_level);
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

    surface->dc = desc.hDc;
    surface->bitmap = desc.hBitmap;

    TRACE("Created DC %p, bitmap %p for surface %p.\n", surface->dc, surface->bitmap, surface);
}

static void texture2d_destroy_dc(void *object)
{
    struct wined3d_surface *surface = object;
    D3DKMT_DESTROYDCFROMMEMORY destroy_desc;
    struct wined3d_context *context = NULL;
    struct wined3d_texture *texture;
    struct wined3d_bo_address data;
    unsigned int sub_resource_idx;
    struct wined3d_device *device;
    NTSTATUS status;

    texture = surface->container;
    sub_resource_idx = surface_get_sub_resource_idx(surface);
    device = texture->resource.device;

    if (!surface->dc)
    {
        ERR("Surface %p has no DC.\n", surface);
        return;
    }

    TRACE("dc %p, bitmap %p.\n", surface->dc, surface->bitmap);

    destroy_desc.hDc = surface->dc;
    destroy_desc.hBitmap = surface->bitmap;
    if ((status = D3DKMTDestroyDCFromMemory(&destroy_desc)))
        ERR("Failed to destroy dc, status %#x.\n", status);
    surface->dc = NULL;
    surface->bitmap = NULL;

    if (device->d3d_initialized)
        context = context_acquire(device, NULL, 0);

    wined3d_texture_get_memory(texture, sub_resource_idx, &data, texture->resource.map_binding);
    context_unmap_bo_address(context, &data, GL_PIXEL_UNPACK_BUFFER);

    if (context)
        context_release(context);
}

HRESULT CDECL wined3d_texture_update_desc(struct wined3d_texture *texture, UINT width, UINT height,
        enum wined3d_format_id format_id, enum wined3d_multisample_type multisample_type,
        UINT multisample_quality, void *mem, UINT pitch)
{
    struct wined3d_device *device = texture->resource.device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    const struct wined3d_format *format = wined3d_get_format(gl_info, format_id, texture->resource.usage);
    UINT resource_size = wined3d_format_calculate_size(format, device->surface_alignment, width, height, 1);
    struct wined3d_texture_sub_resource *sub_resource;
    struct wined3d_surface *surface;
    DWORD valid_location = 0;
    BOOL create_dib = FALSE;

    TRACE("texture %p, width %u, height %u, format %s, multisample_type %#x, multisample_quality %u, "
            "mem %p, pitch %u.\n",
            texture, width, height, debug_d3dformat(format_id), multisample_type, multisample_quality, mem, pitch);

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

    if (texture->resource.type == WINED3D_RTYPE_TEXTURE_1D)
    {
        FIXME("Not yet supported for 1D textures.\n");
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
    surface = sub_resource->u.surface;
    if (surface->dc)
    {
        wined3d_cs_destroy_object(device->cs, texture2d_destroy_dc, surface);
        device->cs->ops->finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);
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
    texture->resource.size = texture->slice_pitch;
    sub_resource->size = texture->slice_pitch;
    sub_resource->locations = WINED3D_LOCATION_DISCARDED;

    if (multisample_type && gl_info->supported[ARB_TEXTURE_MULTISAMPLE])
        texture->target = GL_TEXTURE_2D_MULTISAMPLE;
    else
        texture->target = GL_TEXTURE_2D;

    if ((!is_power_of_two(width) || !is_power_of_two(height)) && !gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO]
            && !gl_info->supported[ARB_TEXTURE_RECTANGLE] && !gl_info->supported[WINED3D_GL_NORMALIZED_TEXRECT])
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
        wined3d_texture_prepare_location(texture, 0, NULL, WINED3D_LOCATION_SYSMEM);
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
        wined3d_cs_init_object(device->cs, texture2d_create_dc, surface);
        device->cs->ops->finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);
    }

    return WINED3D_OK;
}

/* Context activation is done by the caller. */
static void wined3d_texture_prepare_buffer_object(struct wined3d_texture *texture,
#if !defined(STAGING_CSMT)
        unsigned int sub_resource_idx, const struct wined3d_gl_info *gl_info)
#else  /* STAGING_CSMT */
        unsigned int sub_resource_idx, struct wined3d_context *context)
#endif /* STAGING_CSMT */
{
    struct wined3d_texture_sub_resource *sub_resource;

    sub_resource = &texture->sub_resources[sub_resource_idx];
#if !defined(STAGING_CSMT)
    if (sub_resource->buffer_object)
        return;

    GL_EXTCALL(glGenBuffers(1, &sub_resource->buffer_object));
    GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, sub_resource->buffer_object));
    GL_EXTCALL(glBufferData(GL_PIXEL_UNPACK_BUFFER, sub_resource->size, NULL, GL_STREAM_DRAW));
    GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
    checkGLcall("Create buffer object");

    TRACE("Created buffer object %u for texture %p, sub-resource %u.\n",
            sub_resource->buffer_object, texture, sub_resource_idx);
#else  /* STAGING_CSMT */
    if (sub_resource->buffer)
        return;

    sub_resource->buffer = wined3d_device_get_bo(texture->resource.device,
            sub_resource->size, GL_STREAM_DRAW, GL_PIXEL_UNPACK_BUFFER, context);

    TRACE("Created buffer object %u for texture %p, sub-resource %u.\n",
            sub_resource->buffer->name, texture, sub_resource_idx);
#endif /* STAGING_CSMT */
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

void wined3d_texture_prepare_texture(struct wined3d_texture *texture, struct wined3d_context *context, BOOL srgb)
{
    DWORD alloc_flag = srgb ? WINED3D_TEXTURE_SRGB_ALLOCATED : WINED3D_TEXTURE_RGB_ALLOCATED;
    const struct wined3d_d3d_info *d3d_info = context->d3d_info;

    if (!d3d_info->shader_color_key
            && !(texture->async.flags & WINED3D_TEXTURE_ASYNC_COLOR_KEY)
            != !(texture->async.color_key_flags & WINED3D_CKEY_SRC_BLT))
    {
        wined3d_texture_force_reload(texture);

        if (texture->async.color_key_flags & WINED3D_CKEY_SRC_BLT)
            texture->async.flags |= WINED3D_TEXTURE_ASYNC_COLOR_KEY;
    }

    if (texture->flags & alloc_flag)
        return;

    texture->texture_ops->texture_prepare_texture(texture, context, srgb);
    texture->flags |= alloc_flag;
}

static void wined3d_texture_prepare_rb(struct wined3d_texture *texture,
        const struct wined3d_gl_info *gl_info, BOOL multisample)
{
    const struct wined3d_format *format = texture->resource.format;

    if (multisample)
    {
        DWORD samples;

        if (texture->rb_multisample)
            return;

        samples = wined3d_texture_get_gl_sample_count(texture);

        gl_info->fbo_ops.glGenRenderbuffers(1, &texture->rb_multisample);
        gl_info->fbo_ops.glBindRenderbuffer(GL_RENDERBUFFER, texture->rb_multisample);
        gl_info->fbo_ops.glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples,
                format->glInternal, texture->resource.width, texture->resource.height);
        checkGLcall("glRenderbufferStorageMultisample()");
        TRACE("Created multisample rb %u.\n", texture->rb_multisample);
    }
    else
    {
        if (texture->rb_resolved)
            return;

        gl_info->fbo_ops.glGenRenderbuffers(1, &texture->rb_resolved);
        gl_info->fbo_ops.glBindRenderbuffer(GL_RENDERBUFFER, texture->rb_resolved);
        gl_info->fbo_ops.glRenderbufferStorage(GL_RENDERBUFFER, format->glInternal,
                texture->resource.width, texture->resource.height);
        checkGLcall("glRenderbufferStorage()");
        TRACE("Created resolved rb %u.\n", texture->rb_resolved);
    }
}

/* Context activation is done by the caller. Context may be NULL in
 * WINED3D_NO3D mode. */
BOOL wined3d_texture_prepare_location(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        struct wined3d_context *context, DWORD location)
{
    switch (location)
    {
        case WINED3D_LOCATION_SYSMEM:
            if (texture->resource.heap_memory)
                return TRUE;

            if (!wined3d_resource_allocate_sysmem(&texture->resource))
            {
                ERR("Failed to allocate system memory.\n");
                return FALSE;
            }
            return TRUE;

        case WINED3D_LOCATION_USER_MEMORY:
            if (!texture->user_memory)
                ERR("Map binding is set to WINED3D_LOCATION_USER_MEMORY but surface->user_memory is NULL.\n");
            return TRUE;

        case WINED3D_LOCATION_BUFFER:
#if !defined(STAGING_CSMT)
            wined3d_texture_prepare_buffer_object(texture, sub_resource_idx, context->gl_info);
#else  /* STAGING_CSMT */
            wined3d_texture_prepare_buffer_object(texture, sub_resource_idx, context);
#endif /* STAGING_CSMT */
            return TRUE;

        case WINED3D_LOCATION_TEXTURE_RGB:
            wined3d_texture_prepare_texture(texture, context, FALSE);
            return TRUE;

        case WINED3D_LOCATION_TEXTURE_SRGB:
            wined3d_texture_prepare_texture(texture, context, TRUE);
            return TRUE;

        case WINED3D_LOCATION_DRAWABLE:
            if (!texture->swapchain && wined3d_settings.offscreen_rendering_mode != ORM_BACKBUFFER)
                ERR("Texture %p does not have a drawable.\n", texture);
            return TRUE;

        case WINED3D_LOCATION_RB_MULTISAMPLE:
            wined3d_texture_prepare_rb(texture, context->gl_info, TRUE);
            return TRUE;

        case WINED3D_LOCATION_RB_RESOLVED:
            wined3d_texture_prepare_rb(texture, context->gl_info, FALSE);
            return TRUE;

        default:
            ERR("Invalid location %s.\n", wined3d_debug_location(location));
            return FALSE;
    }
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

void wined3d_texture_upload_data(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        const struct wined3d_context *context, const struct wined3d_box *box,
        const struct wined3d_const_bo_address *data, unsigned int row_pitch, unsigned int slice_pitch)
{
    texture->texture_ops->texture_upload_data(texture, sub_resource_idx,
            context, box, data, row_pitch, slice_pitch);
}


/* This call just uploads data, the caller is responsible for binding the
 * correct texture. */
/* Context activation is done by the caller. */
static void texture1d_upload_data(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        const struct wined3d_context *context, const struct wined3d_box *box, const struct wined3d_const_bo_address *data,
        unsigned int row_pitch, unsigned int slice_pitch)
{
    struct wined3d_surface *surface = texture->sub_resources[sub_resource_idx].u.surface;
    const struct wined3d_format *format = texture->resource.format;
    unsigned int level = sub_resource_idx % texture->level_count;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    const void *mem = data->addr;
    void *converted_mem = NULL;
    unsigned int width, x, update_w;
    GLenum target;

    TRACE("texture %p, sub_resource_idx %u, context %p, box %p, data {%#x:%p}, row_pitch %#x, slice_pitch %#x.\n",
            texture, sub_resource_idx, context, box, data->buffer_object, data->addr, row_pitch, slice_pitch);

    width = wined3d_texture_get_level_width(texture, level);

    if (!box)
    {
        x = 0;
        update_w = width;
    }
    else
    {
        x = box->left;
        update_w = box->right - box->left;
    }

    if (format->upload)
    {
        unsigned int dst_row_pitch;

        if (data->buffer_object)
            ERR("Loading a converted texture from a PBO.\n");
        if (texture->resource.format_flags & WINED3DFMT_FLAG_BLOCKS)
            ERR("Converting a block-based format.\n");

        dst_row_pitch = update_w * format->conv_byte_count;

        converted_mem = HeapAlloc(GetProcessHeap(), 0, dst_row_pitch);
        format->upload(data->addr, converted_mem, row_pitch, slice_pitch, dst_row_pitch, dst_row_pitch, update_w, 1, 1);
        mem = converted_mem;
    }

    if (data->buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, data->buffer_object));
        checkGLcall("glBindBuffer");
    }

    target = wined3d_texture_get_sub_resource_target(texture, sub_resource_idx);
    if (target == GL_TEXTURE_1D_ARRAY)
    {
        gl_info->gl_ops.gl.p_glPixelStorei(GL_UNPACK_ROW_LENGTH, row_pitch / format->byte_count);

        gl_info->gl_ops.gl.p_glTexSubImage2D(target, level, x, surface->texture_layer, update_w, 1, format->glFormat, format->glType, mem);
        checkGLcall("glTexSubImage2D");

        gl_info->gl_ops.gl.p_glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }
    else
    {
        gl_info->gl_ops.gl.p_glTexSubImage1D(target, level, x, update_w, format->glFormat, format->glType, mem);
        checkGLcall("glTexSubImage1D");
    }

    if (data->buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
        checkGLcall("glBindBuffer");
    }

    HeapFree(GetProcessHeap(), 0, converted_mem);
}

/* Context activation is done by the caller. */
static void texture1d_download_data(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        const struct wined3d_context *context, const struct wined3d_bo_address *data)
{
    struct wined3d_surface *surface = texture->sub_resources[sub_resource_idx].u.surface;
    const struct wined3d_format *format = texture->resource.format;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct wined3d_texture_sub_resource *sub_resource;
    BYTE *temporary_mem = NULL;
    void *mem;
    GLenum target;

    sub_resource = &texture->sub_resources[sub_resource_idx];

    if (format->conv_byte_count)
    {
        FIXME("Attempting to download a converted 1d texture, format %s.\n",
                debug_d3dformat(format->id));
        return;
    }

    target = wined3d_texture_get_sub_resource_target(texture, sub_resource_idx);
    if (target == GL_TEXTURE_1D_ARRAY)
    {
         WARN_(d3d_perf)("Downloading all miplevel layers to get the surface data for a single sub-resource.\n");

        if (!(temporary_mem = heap_calloc(texture->layer_count, sub_resource->size)))
        {
            ERR("Out of memory.\n");
            return;
        }

        mem = temporary_mem;
    }
    else if (data->buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, data->buffer_object));
        checkGLcall("glBindBuffer");
        mem = data->addr;
    }
    else
        mem = data->addr;

    gl_info->gl_ops.gl.p_glGetTexImage(target, sub_resource_idx,
            format->glFormat, format->glType, mem);
    checkGLcall("glGetTexImage");

    if (temporary_mem)
    {
        void *src_data = temporary_mem + surface->texture_layer * sub_resource->size;
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

    HeapFree(GetProcessHeap(), 0, temporary_mem);
}

/* Context activation is done by the caller. */
static void texture1d_srgb_transfer(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        struct wined3d_context *context, BOOL dest_is_srgb)
{
    struct wined3d_texture_sub_resource *sub_resource = &texture->sub_resources[sub_resource_idx];
    unsigned int row_pitch, slice_pitch;
    struct wined3d_bo_address data;

    WARN_(d3d_perf)("Performing slow rgb/srgb 1d texture transfer.\n");
    data.buffer_object = 0;
    if (!(data.addr = HeapAlloc(GetProcessHeap(), 0, sub_resource->size)))
        return;

    wined3d_texture_get_pitch(texture, sub_resource_idx, &row_pitch, &slice_pitch);
    wined3d_texture_bind_and_dirtify(texture, context, !dest_is_srgb);
    texture1d_download_data(texture, sub_resource_idx, context, &data);
    wined3d_texture_bind_and_dirtify(texture, context, dest_is_srgb);
    texture1d_upload_data(texture, sub_resource_idx, context, NULL,
            wined3d_const_bo_address(&data), row_pitch, slice_pitch);

    HeapFree(GetProcessHeap(), 0, data.addr);
}

/* Context activation is done by the caller. */
static BOOL texture1d_load_location(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        struct wined3d_context *context, DWORD location)
{
    struct wined3d_texture_sub_resource *sub_resource = &texture->sub_resources[sub_resource_idx];
    DWORD required_access = wined3d_resource_access_from_location(location);
    unsigned int row_pitch, slice_pitch;

    TRACE("texture %p, sub_resource_idx %u, context %p, location %s.\n",
            texture, sub_resource_idx, context, wined3d_debug_location(location));

    TRACE("Current resource location %s.\n", wined3d_debug_location(sub_resource->locations));

    if ((sub_resource->locations & location) == location)
    {
        TRACE("Location(s) already up to date.\n");
        return TRUE;
    }

    if ((texture->resource.access & required_access) != required_access)
    {
        ERR("Operation requires %#x access, but 1d texture only has %#x.\n",
                required_access, texture->resource.access);
        return FALSE;
    }

    if (!wined3d_texture_prepare_location(texture, sub_resource_idx, context, location))
        return FALSE;

    if (sub_resource->locations & WINED3D_LOCATION_DISCARDED)
    {
        TRACE("1d texture previously discarded, nothing to do.\n");
        wined3d_texture_validate_location(texture, sub_resource_idx, location);
        wined3d_texture_invalidate_location(texture, sub_resource_idx, WINED3D_LOCATION_DISCARDED);
        goto done;
    }

    switch (location)
    {
        case WINED3D_LOCATION_TEXTURE_RGB:
        case WINED3D_LOCATION_TEXTURE_SRGB:
            if (sub_resource->locations & WINED3D_LOCATION_SYSMEM)
            {
                struct wined3d_const_bo_address data = {0, texture->resource.heap_memory};
                data.addr += sub_resource->offset;
                wined3d_texture_bind_and_dirtify(texture, context, location == WINED3D_LOCATION_TEXTURE_SRGB);
                wined3d_texture_get_pitch(texture, sub_resource_idx, &row_pitch, &slice_pitch);
                texture1d_upload_data(texture, sub_resource_idx, context, NULL, &data, row_pitch, slice_pitch);
            }
            else if (sub_resource->locations & WINED3D_LOCATION_BUFFER)
            {
#if !defined(STAGING_CSMT)
                struct wined3d_const_bo_address data = {sub_resource->buffer_object, NULL};
#else  /* STAGING_CSMT */
                struct wined3d_const_bo_address data = {sub_resource->buffer->name, NULL};
#endif /* STAGING_CSMT */
                wined3d_texture_bind_and_dirtify(texture, context, location == WINED3D_LOCATION_TEXTURE_SRGB);
                wined3d_texture_get_pitch(texture, sub_resource_idx, &row_pitch, &slice_pitch);
                texture1d_upload_data(texture, sub_resource_idx, context, NULL, &data, row_pitch, slice_pitch);
            }
            else if (sub_resource->locations & WINED3D_LOCATION_TEXTURE_RGB)
            {
                texture1d_srgb_transfer(texture, sub_resource_idx, context, TRUE);
            }
            else if (sub_resource->locations & WINED3D_LOCATION_TEXTURE_SRGB)
            {
                texture1d_srgb_transfer(texture, sub_resource_idx, context, FALSE);
            }
            else
            {
                FIXME("Implement 1d texture loading from %s.\n", wined3d_debug_location(sub_resource->locations));
                return FALSE;
            }
            break;

        case WINED3D_LOCATION_SYSMEM:
            if (sub_resource->locations & (WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB))
            {
                struct wined3d_bo_address data = {0, texture->resource.heap_memory};

                data.addr += sub_resource->offset;
                if (sub_resource->locations & WINED3D_LOCATION_TEXTURE_RGB)
                    wined3d_texture_bind_and_dirtify(texture, context, FALSE);
                else
                    wined3d_texture_bind_and_dirtify(texture, context, TRUE);

                texture1d_download_data(texture, sub_resource_idx, context, &data);
                ++texture->download_count;
            }
            else
            {
                FIXME("Implement WINED3D_LOCATION_SYSMEM loading from %s.\n",
                        wined3d_debug_location(sub_resource->locations));
                return FALSE;
            }
            break;

        case WINED3D_LOCATION_BUFFER:
            if (sub_resource->locations & (WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB))
            {
#if !defined(STAGING_CSMT)
                struct wined3d_bo_address data = {sub_resource->buffer_object, NULL};
#else  /* STAGING_CSMT */
                struct wined3d_bo_address data = {sub_resource->buffer->name, NULL};
#endif /* STAGING_CSMT */

                if (sub_resource->locations & WINED3D_LOCATION_TEXTURE_RGB)
                    wined3d_texture_bind_and_dirtify(texture, context, FALSE);
                else
                    wined3d_texture_bind_and_dirtify(texture, context, TRUE);

                texture1d_download_data(texture, sub_resource_idx, context, &data);
            }
            else
            {
                FIXME("Implement WINED3D_LOCATION_BUFFER loading from %s.\n",
                        wined3d_debug_location(sub_resource->locations));
                return FALSE;
            }
            break;

        default:
            FIXME("Implement %s loading from %s.\n", wined3d_debug_location(location),
                    wined3d_debug_location(sub_resource->locations));
            return FALSE;
    }

done:
    wined3d_texture_validate_location(texture, sub_resource_idx, location);

    return TRUE;
}

static void texture1d_prepare_texture(struct wined3d_texture *texture, struct wined3d_context *context, BOOL srgb)
{
    const struct wined3d_format *format = texture->resource.format;
    unsigned int sub_count = texture->level_count * texture->layer_count;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    unsigned int width;
    GLenum internal;

    wined3d_texture_bind_and_dirtify(texture, context, srgb);

    if (srgb)
        internal = format->glGammaInternal;
    else if (texture->resource.usage & WINED3DUSAGE_RENDERTARGET
            && wined3d_resource_is_offscreen(&texture->resource))
        internal = format->rtInternal;
    else
        internal = format->glInternal;

    if (wined3d_texture_use_immutable_storage(texture, gl_info))
    {
        width = wined3d_texture_get_level_width(texture, 0);

        if (texture->target == GL_TEXTURE_1D_ARRAY)
        {
            GL_EXTCALL(glTexStorage2D(texture->target, texture->level_count, internal, width, texture->layer_count));
            checkGLcall("glTexStorage2D");
        }
        else
        {
            GL_EXTCALL(glTexStorage1D(texture->target, texture->level_count, internal, width));
            checkGLcall("glTexStorage1D");
        }
    }
    else
    {
        unsigned int i;

        for (i = 0; i < sub_count; ++i)
        {
            GLenum target;
            struct wined3d_surface *surface = texture->sub_resources[i].u.surface;
            width = wined3d_texture_get_level_width(texture, surface->texture_level);
            target = wined3d_texture_get_sub_resource_target(texture, i);

            if (texture->target == GL_TEXTURE_1D_ARRAY)
            {
                gl_info->gl_ops.gl.p_glTexImage2D(target, surface->texture_level,
                        internal, width, texture->layer_count, 0, format->glFormat, format->glType, NULL);
                checkGLcall("glTexImage2D");
            }
            else
            {
                gl_info->gl_ops.gl.p_glTexImage1D(target, surface->texture_level,
                        internal, width, 0, format->glFormat, format->glType, NULL);
                checkGLcall("glTexImage1D");
            }
        }
    }
}

static void texture1d_cleanup_sub_resources(struct wined3d_texture *texture)
{
}

static const struct wined3d_texture_ops texture1d_ops =
{
    texture1d_upload_data,
    texture1d_load_location,
    texture1d_prepare_texture,
    texture1d_cleanup_sub_resources,
};

static void texture2d_upload_data(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        const struct wined3d_context *context, const struct wined3d_box *box,
        const struct wined3d_const_bo_address *data, unsigned int row_pitch, unsigned int slice_pitch)
{
    unsigned int texture_level;
    POINT dst_point;
    RECT src_rect;

    src_rect.left = 0;
    src_rect.top = 0;
    if (box)
    {
        dst_point.x = box->left;
        dst_point.y = box->top;
        src_rect.right = box->right - box->left;
        src_rect.bottom = box->bottom - box->top;
    }
    else
    {
        dst_point.x = dst_point.y = 0;
        texture_level = sub_resource_idx % texture->level_count;
        src_rect.right = wined3d_texture_get_level_width(texture, texture_level);
        src_rect.bottom = wined3d_texture_get_level_height(texture, texture_level);
    }

    wined3d_surface_upload_data(texture->sub_resources[sub_resource_idx].u.surface, context->gl_info,
            texture->resource.format, &src_rect, row_pitch, &dst_point, FALSE, data);
}

static BOOL texture2d_load_location(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        struct wined3d_context *context, DWORD location)
{
    return surface_load_location(texture->sub_resources[sub_resource_idx].u.surface, context, location);
}

/* Context activation is done by the caller. */
static void texture2d_prepare_texture(struct wined3d_texture *texture, struct wined3d_context *context, BOOL srgb)
{
    const struct wined3d_format *format = texture->resource.format;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    const struct wined3d_color_key_conversion *conversion;
    GLenum internal;

    TRACE("texture %p, context %p, format %s.\n", texture, context, debug_d3dformat(format->id));

    if (format->conv_byte_count)
    {
        texture->flags |= WINED3D_TEXTURE_CONVERTED;
    }
    else if ((conversion = wined3d_format_get_color_key_conversion(texture, TRUE)))
    {
        texture->flags |= WINED3D_TEXTURE_CONVERTED;
        format = wined3d_get_format(gl_info, conversion->dst_format, texture->resource.usage);
        TRACE("Using format %s for color key conversion.\n", debug_d3dformat(format->id));
    }

    wined3d_texture_bind_and_dirtify(texture, context, srgb);

    if (srgb)
        internal = format->glGammaInternal;
    else if (texture->resource.usage & WINED3DUSAGE_RENDERTARGET
            && wined3d_resource_is_offscreen(&texture->resource))
        internal = format->rtInternal;
    else
        internal = format->glInternal;

    if (!internal)
        FIXME("No GL internal format for format %s.\n", debug_d3dformat(format->id));

    TRACE("internal %#x, format %#x, type %#x.\n", internal, format->glFormat, format->glType);

    if (wined3d_texture_use_immutable_storage(texture, gl_info))
        wined3d_texture_allocate_gl_immutable_storage(texture, internal, gl_info);
    else
        wined3d_texture_allocate_gl_mutable_storage(texture, internal, format, gl_info);
}

static void texture2d_cleanup_sub_resources(struct wined3d_texture *texture)
{
    unsigned int sub_count = texture->level_count * texture->layer_count;
    struct wined3d_device *device = texture->resource.device;
    struct wined3d_texture_sub_resource *sub_resource;
    struct wined3d_renderbuffer_entry *entry, *entry2;
    const struct wined3d_gl_info *gl_info = NULL;
    struct wined3d_context *context = NULL;
    struct wined3d_surface *surface;
    unsigned int i;

    for (i = 0; i < sub_count; ++i)
    {
        sub_resource = &texture->sub_resources[i];
        if (!(surface = sub_resource->u.surface))
            continue;

        TRACE("surface %p.\n", surface);

        if (!context && !list_empty(&surface->renderbuffers))
        {
            context = context_acquire(device, NULL, 0);
            gl_info = context->gl_info;
        }

        LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &surface->renderbuffers, struct wined3d_renderbuffer_entry, entry)
        {
            TRACE("Deleting renderbuffer %u.\n", entry->id);
            context_gl_resource_released(device, entry->id, TRUE);
            gl_info->fbo_ops.glDeleteRenderbuffers(1, &entry->id);
            heap_free(entry);
        }

        if (surface->dc)
            texture2d_destroy_dc(surface);
    }
    if (context)
        context_release(context);
    heap_free(texture->sub_resources[0].u.surface);
}

static const struct wined3d_texture_ops texture2d_ops =
{
    texture2d_upload_data,
    texture2d_load_location,
    texture2d_prepare_texture,
    texture2d_cleanup_sub_resources,
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

static void wined3d_texture_unload(struct wined3d_resource *resource)
{
    struct wined3d_texture *texture = texture_from_resource(resource);
    UINT sub_count = texture->level_count * texture->layer_count;
    struct wined3d_device *device = resource->device;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    UINT i;

    TRACE("texture %p.\n", texture);

    context = context_acquire(device, NULL, 0);
    gl_info = context->gl_info;

    for (i = 0; i < sub_count; ++i)
    {
        struct wined3d_texture_sub_resource *sub_resource = &texture->sub_resources[i];

        if (resource->access & WINED3D_RESOURCE_ACCESS_CPU
                && wined3d_texture_load_location(texture, i, context, resource->map_binding))
        {
            wined3d_texture_invalidate_location(texture, i, ~resource->map_binding);
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
            wined3d_texture_validate_location(texture, i, WINED3D_LOCATION_DISCARDED);
            wined3d_texture_invalidate_location(texture, i, ~WINED3D_LOCATION_DISCARDED);
        }

#if !defined(STAGING_CSMT)
        if (sub_resource->buffer_object)
            wined3d_texture_remove_buffer_object(texture, i, context->gl_info);
#else  /* STAGING_CSMT */
        if (sub_resource->buffer)
            wined3d_texture_remove_buffer_object(texture, i, context);
#endif /* STAGING_CSMT */

        if (resource->type == WINED3D_RTYPE_TEXTURE_2D)
        {
            struct wined3d_surface *surface = sub_resource->u.surface;
            struct wined3d_renderbuffer_entry *entry, *entry2;

            LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &surface->renderbuffers, struct wined3d_renderbuffer_entry, entry)
            {
                context_gl_resource_released(device, entry->id, TRUE);
                gl_info->fbo_ops.glDeleteRenderbuffers(1, &entry->id);
                list_remove(&entry->entry);
                heap_free(entry);
            }
            list_init(&surface->renderbuffers);
            surface->current_renderbuffer = NULL;
        }
    }

    context_release(context);

    wined3d_texture_force_reload(texture);
    wined3d_texture_unload_gl_texture(texture);
}

static HRESULT texture_resource_sub_resource_map(struct wined3d_resource *resource, unsigned int sub_resource_idx,
        struct wined3d_map_desc *map_desc, const struct wined3d_box *box, DWORD flags)
{
    const struct wined3d_format *format = resource->format;
    struct wined3d_texture_sub_resource *sub_resource;
    struct wined3d_device *device = resource->device;
    unsigned int fmt_flags = resource->format_flags;
    struct wined3d_context *context = NULL;
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

    if (device->d3d_initialized)
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
    base_memory = context_map_bo_address(context, &data, sub_resource->size, GL_PIXEL_UNPACK_BUFFER, flags);
    TRACE("Base memory pointer %p.\n", base_memory);

    if (context)
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
    struct wined3d_context *context = NULL;
    struct wined3d_texture *texture;
    struct wined3d_bo_address data;

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

    if (device->d3d_initialized)
        context = context_acquire(device, NULL, 0);

    wined3d_texture_get_memory(texture, sub_resource_idx, &data, texture->resource.map_binding);
    context_unmap_bo_address(context, &data, GL_PIXEL_UNPACK_BUFFER);

    if (context)
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
    wined3d_texture_unload,
    texture_resource_sub_resource_map,
    texture_resource_sub_resource_map_info,
    texture_resource_sub_resource_unmap,
};

static HRESULT texture1d_init(struct wined3d_texture *texture, const struct wined3d_resource_desc *desc,
        UINT layer_count, UINT level_count, struct wined3d_device *device, void *parent,
        const struct wined3d_parent_ops *parent_ops)
{
    struct wined3d_device_parent *device_parent = device->device_parent;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_surface *surfaces;
    unsigned int i, j;
    HRESULT hr;

    if (layer_count > 1 && !gl_info->supported[EXT_TEXTURE_ARRAY])
    {
        WARN("OpenGL implementation does not support array textures.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* TODO: It should only be possible to create textures for formats
     * that are reported as supported. */
    if (WINED3DFMT_UNKNOWN >= desc->format)
    {
        WARN("(%p) : Texture cannot be created with a format of WINED3DFMT_UNKNOWN.\n", texture);
        return WINED3DERR_INVALIDCALL;
    }

    if (desc->usage & WINED3DUSAGE_LEGACY_CUBEMAP)
    {
        WARN("1d textures can not be used for cube mapping, returning D3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if ((desc->usage & WINED3DUSAGE_DYNAMIC && wined3d_resource_access_is_managed(desc->access))
            || (desc->usage & WINED3DUSAGE_SCRATCH))
    {
        WARN("Attempted to create a DYNAMIC texture in pool %s.\n", wined3d_debug_resource_access(desc->access));
        return WINED3DERR_INVALIDCALL;
    }

    if (!gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO] && !is_power_of_two(desc->width))
    {
        if (desc->usage & WINED3DUSAGE_SCRATCH)
        {
            WARN("Creating a scratch NPOT 1d texture despite lack of HW support.\n");
        }
        else
        {
            WARN("Attempted to create a NPOT 1d texture (%u, %u, %u) without GL support.\n",
                    desc->width, desc->height, desc->depth);
            return WINED3DERR_INVALIDCALL;
        }
    }

    if (desc->usage & WINED3DUSAGE_QUERY_GENMIPMAP)
    {
        if (level_count != 1)
        {
            WARN("WINED3DUSAGE_QUERY_GENMIPMAP is set, and level count != 1, returning WINED3DERR_INVALIDCALL.\n");
            return WINED3DERR_INVALIDCALL;
        }
    }

    if (FAILED(hr = wined3d_texture_init(texture, &texture1d_ops, layer_count, level_count, desc,
            0, device, parent, parent_ops, &texture_resource_ops)))
    {
        WARN("Failed to initialize texture, returning %#x.\n", hr);
        return hr;
    }

    texture->pow2_matrix[0] = 1.0f;
    texture->pow2_matrix[5] = 1.0f;
    texture->pow2_matrix[10] = 1.0f;
    texture->pow2_matrix[15] = 1.0f;
    texture->target = (layer_count > 1) ? GL_TEXTURE_1D_ARRAY : GL_TEXTURE_1D;

    if (wined3d_texture_use_pbo(texture, gl_info))
    {
        wined3d_resource_free_sysmem(&texture->resource);
        texture->resource.map_binding = WINED3D_LOCATION_BUFFER;
    }

    if (level_count > ~(SIZE_T)0 / layer_count
            || !(surfaces = heap_calloc(level_count * layer_count, sizeof(*surfaces))))
    {
        wined3d_texture_cleanup_sync(texture);
        return E_OUTOFMEMORY;
    }

    /* Generate all the surfaces. */
    for (i = 0; i < texture->level_count; ++i)
    {
        for (j = 0; j < texture->layer_count; ++j)
        {
            struct wined3d_texture_sub_resource *sub_resource;
            unsigned int idx = j * texture->level_count + i;
            struct wined3d_surface *surface;

            surface = &surfaces[idx];
            surface->container = texture;
            surface->texture_level = i;
            surface->texture_layer = j;
            list_init(&surface->renderbuffers);

            sub_resource = &texture->sub_resources[idx];
            sub_resource->locations = WINED3D_LOCATION_DISCARDED;
            sub_resource->u.surface = surface;

            if (FAILED(hr = device_parent->ops->surface_created(device_parent,
                    texture, idx, &sub_resource->parent, &sub_resource->parent_ops)))
            {
                WARN("Failed to create texture1d parent, hr %#x.\n", hr);
                sub_resource->parent = NULL;
                wined3d_texture_cleanup_sync(texture);
                return hr;
            }

            TRACE("parent %p, parent_ops %p.\n", parent, parent_ops);

            TRACE("Created 1d texture surface level %u, layer %u @ %p.\n", i, j, surface);
        }
    }

    return WINED3D_OK;
}

static HRESULT texture_init(struct wined3d_texture *texture, const struct wined3d_resource_desc *desc,
        unsigned int layer_count, unsigned int level_count, DWORD flags, struct wined3d_device *device,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    struct wined3d_device_parent *device_parent = device->device_parent;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_surface *surfaces;
    UINT pow2_width, pow2_height;
    unsigned int i, j, sub_count;
    HRESULT hr;

    if (!(desc->usage & WINED3DUSAGE_LEGACY_CUBEMAP) && layer_count > 1
            && !gl_info->supported[EXT_TEXTURE_ARRAY])
    {
        WARN("OpenGL implementation does not support array textures.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* TODO: It should only be possible to create textures for formats
     * that are reported as supported. */
    if (WINED3DFMT_UNKNOWN >= desc->format)
    {
        WARN("(%p) : Texture cannot be created with a format of WINED3DFMT_UNKNOWN.\n", texture);
        return WINED3DERR_INVALIDCALL;
    }

    if (desc->usage & WINED3DUSAGE_DYNAMIC && wined3d_resource_access_is_managed(desc->access))
        FIXME("Trying to create a managed texture with dynamic usage.\n");
    if (!(desc->usage & (WINED3DUSAGE_DYNAMIC | WINED3DUSAGE_RENDERTARGET | WINED3DUSAGE_DEPTHSTENCIL))
            && (flags & WINED3D_TEXTURE_CREATE_MAPPABLE))
        WARN("Creating a mappable texture that doesn't specify dynamic usage.\n");
    if (desc->usage & WINED3DUSAGE_RENDERTARGET && desc->access & WINED3D_RESOURCE_ACCESS_CPU)
        FIXME("Trying to create a CPU accessible render target.\n");

    pow2_width = desc->width;
    pow2_height = desc->height;
    if (((desc->width & (desc->width - 1)) || (desc->height & (desc->height - 1)))
            && !gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO])
    {
        /* level_count == 0 returns an error as well. */
        if (level_count != 1 || layer_count != 1)
        {
            if (!(desc->usage & WINED3DUSAGE_SCRATCH))
            {
                WARN("Attempted to create a mipmapped/cube/array NPOT texture without unconditional NPOT support.\n");
                return WINED3DERR_INVALIDCALL;
            }

            WARN("Creating a scratch mipmapped/cube/array NPOT texture despite lack of HW support.\n");
        }
        texture->flags |= WINED3D_TEXTURE_COND_NP2;

        if (!gl_info->supported[ARB_TEXTURE_RECTANGLE] && !gl_info->supported[WINED3D_GL_NORMALIZED_TEXRECT])
        {
            const struct wined3d_format *format = wined3d_get_format(gl_info, desc->format, desc->usage);

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

    if ((pow2_width > gl_info->limits.texture_size || pow2_height > gl_info->limits.texture_size)
            && (desc->usage & WINED3DUSAGE_TEXTURE))
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

    if (FAILED(hr = wined3d_texture_init(texture, &texture2d_ops, layer_count, level_count, desc,
            flags, device, parent, parent_ops, &texture_resource_ops)))
    {
        WARN("Failed to initialize texture, returning %#x.\n", hr);
        return hr;
    }

    /* Precalculated scaling for 'faked' non power of two texture coords. */
    if (texture->resource.gl_type == WINED3D_GL_RES_TYPE_TEX_RECT)
    {
        texture->pow2_matrix[0] = (float)desc->width;
        texture->pow2_matrix[5] = (float)desc->height;
        texture->flags &= ~(WINED3D_TEXTURE_POW2_MAT_IDENT | WINED3D_TEXTURE_NORMALIZED_COORDS);
        texture->target = GL_TEXTURE_RECTANGLE_ARB;
    }
    else
    {
        if (texture->flags & WINED3D_TEXTURE_COND_NP2_EMULATED)
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
        if (desc->usage & WINED3DUSAGE_LEGACY_CUBEMAP)
        {
            texture->target = GL_TEXTURE_CUBE_MAP_ARB;
        }
        else if (desc->multisample_type && gl_info->supported[ARB_TEXTURE_MULTISAMPLE])
        {
            if (layer_count > 1)
                texture->target = GL_TEXTURE_2D_MULTISAMPLE_ARRAY;
            else
                texture->target = GL_TEXTURE_2D_MULTISAMPLE;
        }
        else
        {
            if (layer_count > 1)
                texture->target = GL_TEXTURE_2D_ARRAY;
            else
                texture->target = GL_TEXTURE_2D;
        }
    }
    texture->pow2_matrix[10] = 1.0f;
    texture->pow2_matrix[15] = 1.0f;
    TRACE("x scale %.8e, y scale %.8e.\n", texture->pow2_matrix[0], texture->pow2_matrix[5]);

    if (wined3d_texture_use_pbo(texture, gl_info))
        texture->resource.map_binding = WINED3D_LOCATION_BUFFER;

    sub_count = level_count * layer_count;
    if (sub_count / layer_count != level_count
            || !(surfaces = heap_calloc(sub_count, sizeof(*surfaces))))
    {
        wined3d_texture_cleanup_sync(texture);
        return E_OUTOFMEMORY;
    }

    if (desc->usage & WINED3DUSAGE_OVERLAY)
    {
        if (!(texture->overlay_info = heap_calloc(sub_count, sizeof(*texture->overlay_info))))
        {
            heap_free(surfaces);
            wined3d_texture_cleanup_sync(texture);
            return E_OUTOFMEMORY;
        }

        for (i = 0; i < sub_count; ++i)
        {
            list_init(&texture->overlay_info[i].entry);
            list_init(&texture->overlay_info[i].overlays);
        }
    }

    /* Generate all the surfaces. */
    for (i = 0; i < texture->level_count; ++i)
    {
        for (j = 0; j < texture->layer_count; ++j)
        {
            struct wined3d_texture_sub_resource *sub_resource;
            unsigned int idx = j * texture->level_count + i;
            struct wined3d_surface *surface;

            surface = &surfaces[idx];
            surface->container = texture;
            surface->texture_level = i;
            surface->texture_layer = j;
            list_init(&surface->renderbuffers);

            sub_resource = &texture->sub_resources[idx];
            sub_resource->locations = WINED3D_LOCATION_DISCARDED;
            sub_resource->u.surface = surface;
            if (!(texture->resource.usage & WINED3DUSAGE_DEPTHSTENCIL))
            {
                wined3d_texture_validate_location(texture, idx, WINED3D_LOCATION_SYSMEM);
                wined3d_texture_invalidate_location(texture, idx, ~WINED3D_LOCATION_SYSMEM);
            }

            if (FAILED(hr = device_parent->ops->surface_created(device_parent,
                    texture, idx, &sub_resource->parent, &sub_resource->parent_ops)))
            {
                WARN("Failed to create surface parent, hr %#x.\n", hr);
                sub_resource->parent = NULL;
                wined3d_texture_cleanup_sync(texture);
                return hr;
            }

            TRACE("parent %p, parent_ops %p.\n", sub_resource->parent, sub_resource->parent_ops);

            TRACE("Created surface level %u, layer %u @ %p.\n", i, j, surface);

            if ((desc->usage & WINED3DUSAGE_OWNDC) || (device->wined3d->flags & WINED3D_NO3D))
            {
                wined3d_cs_init_object(device->cs, texture2d_create_dc, surface);
                device->cs->ops->finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);
                if (!surface->dc)
                {
                    wined3d_texture_cleanup_sync(texture);
                    return WINED3DERR_INVALIDCALL;
                }
            }
        }
    }

    return WINED3D_OK;
}

/* This call just uploads data, the caller is responsible for binding the
 * correct texture. */
/* Context activation is done by the caller. */
static void texture3d_upload_data(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        const struct wined3d_context *context, const struct wined3d_box *box,
        const struct wined3d_const_bo_address *data, unsigned int row_pitch, unsigned int slice_pitch)
{
    const struct wined3d_format *format = texture->resource.format;
    unsigned int level = sub_resource_idx % texture->level_count;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    unsigned int x, y, z, update_w, update_h, update_d;
    unsigned int dst_row_pitch, dst_slice_pitch;
    unsigned int width, height, depth;
    const void *mem = data->addr;
    void *converted_mem = NULL;

    TRACE("texture %p, sub_resource_idx %u, context %p, box %s, data {%#x:%p}, row_pitch %#x, slice_pitch %#x.\n",
            texture, sub_resource_idx, context, debug_box(box),
            data->buffer_object, data->addr, row_pitch, slice_pitch);

    width = wined3d_texture_get_level_width(texture, level);
    height = wined3d_texture_get_level_height(texture, level);
    depth = wined3d_texture_get_level_depth(texture, level);

    if (!box)
    {
        x = y = z = 0;
        update_w = width;
        update_h = height;
        update_d = depth;
    }
    else
    {
        x = box->left;
        y = box->top;
        z = box->front;
        update_w = box->right - box->left;
        update_h = box->bottom - box->top;
        update_d = box->back - box->front;
    }

    if (format->conv_byte_count)
    {
        if (data->buffer_object)
            ERR("Loading a converted texture from a PBO.\n");
        if (texture->resource.format_flags & WINED3DFMT_FLAG_BLOCKS)
            ERR("Converting a block-based format.\n");

        dst_row_pitch = update_w * format->conv_byte_count;
        dst_slice_pitch = dst_row_pitch * update_h;

        converted_mem = heap_calloc(update_d, dst_slice_pitch);
        format->upload(data->addr, converted_mem, row_pitch, slice_pitch,
                dst_row_pitch, dst_slice_pitch, update_w, update_h, update_d);
        mem = converted_mem;
    }
    else
    {
        wined3d_texture_get_pitch(texture, sub_resource_idx, &dst_row_pitch, &dst_slice_pitch);
        if (row_pitch != dst_row_pitch || slice_pitch != dst_slice_pitch)
            FIXME("Ignoring row/slice pitch (%u/%u).\n", row_pitch, slice_pitch);
    }

    if (data->buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, data->buffer_object));
        checkGLcall("glBindBuffer");
    }

    GL_EXTCALL(glTexSubImage3D(GL_TEXTURE_3D, level, x, y, z,
            update_w, update_h, update_d, format->glFormat, format->glType, mem));
    checkGLcall("glTexSubImage3D");

    if (data->buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0));
        checkGLcall("glBindBuffer");
    }

    heap_free(converted_mem);
}

/* Context activation is done by the caller. */
static void texture3d_download_data(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        const struct wined3d_context *context, const struct wined3d_bo_address *data)
{
    const struct wined3d_format *format = texture->resource.format;
    const struct wined3d_gl_info *gl_info = context->gl_info;

    if (format->conv_byte_count)
    {
        FIXME("Attempting to download a converted volume, format %s.\n",
                debug_d3dformat(format->id));
        return;
    }

    if (data->buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, data->buffer_object));
        checkGLcall("glBindBuffer");
    }

    gl_info->gl_ops.gl.p_glGetTexImage(GL_TEXTURE_3D, sub_resource_idx,
            format->glFormat, format->glType, data->addr);
    checkGLcall("glGetTexImage");

    if (data->buffer_object)
    {
        GL_EXTCALL(glBindBuffer(GL_PIXEL_PACK_BUFFER, 0));
        checkGLcall("glBindBuffer");
    }

}

/* Context activation is done by the caller. */
static void texture3d_srgb_transfer(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        struct wined3d_context *context, BOOL dest_is_srgb)
{
    struct wined3d_texture_sub_resource *sub_resource = &texture->sub_resources[sub_resource_idx];
    unsigned int row_pitch, slice_pitch;
    struct wined3d_bo_address data;

    /* Optimisations are possible, but the effort should be put into either
     * implementing EXT_SRGB_DECODE in the driver or finding out why we
     * picked the wrong copy for the original upload and fixing that.
     *
     * Also keep in mind that we want to avoid using resource.heap_memory
     * for DEFAULT pool surfaces. */
    WARN_(d3d_perf)("Performing slow rgb/srgb volume transfer.\n");
    data.buffer_object = 0;
    if (!(data.addr = heap_alloc(sub_resource->size)))
        return;

    wined3d_texture_get_pitch(texture, sub_resource_idx, &row_pitch, &slice_pitch);
    wined3d_texture_bind_and_dirtify(texture, context, !dest_is_srgb);
    texture3d_download_data(texture, sub_resource_idx, context, &data);
    wined3d_texture_bind_and_dirtify(texture, context, dest_is_srgb);
    texture3d_upload_data(texture, sub_resource_idx, context,
            NULL, wined3d_const_bo_address(&data), row_pitch, slice_pitch);

    heap_free(data.addr);
}

/* Context activation is done by the caller. */
static BOOL texture3d_load_location(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        struct wined3d_context *context, DWORD location)
{
    struct wined3d_texture_sub_resource *sub_resource = &texture->sub_resources[sub_resource_idx];
    unsigned int row_pitch, slice_pitch;

    if (!wined3d_texture_prepare_location(texture, sub_resource_idx, context, location))
        return FALSE;

    switch (location)
    {
        case WINED3D_LOCATION_TEXTURE_RGB:
        case WINED3D_LOCATION_TEXTURE_SRGB:
            if (sub_resource->locations & WINED3D_LOCATION_SYSMEM)
            {
                struct wined3d_const_bo_address data = {0, texture->resource.heap_memory};
                data.addr += sub_resource->offset;
                wined3d_texture_bind_and_dirtify(texture, context,
                        location == WINED3D_LOCATION_TEXTURE_SRGB);
                wined3d_texture_get_pitch(texture, sub_resource_idx, &row_pitch, &slice_pitch);
                texture3d_upload_data(texture, sub_resource_idx, context, NULL, &data, row_pitch, slice_pitch);
            }
            else if (sub_resource->locations & WINED3D_LOCATION_BUFFER)
            {
#if !defined(STAGING_CSMT)
                struct wined3d_const_bo_address data = {sub_resource->buffer_object, NULL};
#else  /* STAGING_CSMT */
                struct wined3d_const_bo_address data = {sub_resource->buffer->name, NULL};
#endif /* STAGING_CSMT */
                wined3d_texture_bind_and_dirtify(texture, context,
                        location == WINED3D_LOCATION_TEXTURE_SRGB);
                wined3d_texture_get_pitch(texture, sub_resource_idx, &row_pitch, &slice_pitch);
                texture3d_upload_data(texture, sub_resource_idx, context, NULL, &data, row_pitch, slice_pitch);
            }
            else if (sub_resource->locations & WINED3D_LOCATION_TEXTURE_RGB)
            {
                texture3d_srgb_transfer(texture, sub_resource_idx, context, TRUE);
            }
            else if (sub_resource->locations & WINED3D_LOCATION_TEXTURE_SRGB)
            {
                texture3d_srgb_transfer(texture, sub_resource_idx, context, FALSE);
            }
            else
            {
                FIXME("Implement texture loading from %s.\n", wined3d_debug_location(sub_resource->locations));
                return FALSE;
            }
            break;

        case WINED3D_LOCATION_SYSMEM:
            if (sub_resource->locations & (WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB))
            {
                struct wined3d_bo_address data = {0, texture->resource.heap_memory};

                data.addr += sub_resource->offset;
                if (sub_resource->locations & WINED3D_LOCATION_TEXTURE_RGB)
                    wined3d_texture_bind_and_dirtify(texture, context, FALSE);
                else
                    wined3d_texture_bind_and_dirtify(texture, context, TRUE);

                texture3d_download_data(texture, sub_resource_idx, context, &data);
                ++texture->download_count;
            }
            else
            {
                FIXME("Implement WINED3D_LOCATION_SYSMEM loading from %s.\n",
                        wined3d_debug_location(sub_resource->locations));
                return FALSE;
            }
            break;

        case WINED3D_LOCATION_BUFFER:
            if (sub_resource->locations & (WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB))
            {
#if !defined(STAGING_CSMT)
                struct wined3d_bo_address data = {sub_resource->buffer_object, NULL};
#else  /* STAGING_CSMT */
                struct wined3d_bo_address data = {sub_resource->buffer->name, NULL};
#endif /* STAGING_CSMT */

                if (sub_resource->locations & WINED3D_LOCATION_TEXTURE_RGB)
                    wined3d_texture_bind_and_dirtify(texture, context, FALSE);
                else
                    wined3d_texture_bind_and_dirtify(texture, context, TRUE);

                texture3d_download_data(texture, sub_resource_idx, context, &data);
            }
            else
            {
                FIXME("Implement WINED3D_LOCATION_BUFFER loading from %s.\n",
                        wined3d_debug_location(sub_resource->locations));
                return FALSE;
            }
            break;

        default:
            FIXME("Implement %s loading from %s.\n", wined3d_debug_location(location),
                    wined3d_debug_location(sub_resource->locations));
            return FALSE;
    }

    return TRUE;
}

static void texture3d_prepare_texture(struct wined3d_texture *texture, struct wined3d_context *context, BOOL srgb)
{
    const struct wined3d_format *format = texture->resource.format;
    GLenum internal = srgb ? format->glGammaInternal : format->glInternal;
    unsigned int sub_count = texture->level_count * texture->layer_count;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    unsigned int i;

    wined3d_texture_bind_and_dirtify(texture, context, srgb);

    if (wined3d_texture_use_immutable_storage(texture, gl_info))
    {
        GL_EXTCALL(glTexStorage3D(GL_TEXTURE_3D, texture->level_count, internal,
                wined3d_texture_get_level_width(texture, 0),
                wined3d_texture_get_level_height(texture, 0),
                wined3d_texture_get_level_depth(texture, 0)));
        checkGLcall("glTexStorage3D");
    }
    else
    {
        for (i = 0; i < sub_count; ++i)
        {
            GL_EXTCALL(glTexImage3D(GL_TEXTURE_3D, i, internal,
                    wined3d_texture_get_level_width(texture, i),
                    wined3d_texture_get_level_height(texture, i),
                    wined3d_texture_get_level_depth(texture, i),
                    0, format->glFormat, format->glType, NULL));
            checkGLcall("glTexImage3D");
        }
    }
}

static void texture3d_cleanup_sub_resources(struct wined3d_texture *texture)
{
}

static const struct wined3d_texture_ops texture3d_ops =
{
    texture3d_upload_data,
    texture3d_load_location,
    texture3d_prepare_texture,
    texture3d_cleanup_sub_resources,
};

static HRESULT volumetexture_init(struct wined3d_texture *texture, const struct wined3d_resource_desc *desc,
        UINT layer_count, UINT level_count, DWORD flags, struct wined3d_device *device, void *parent,
        const struct wined3d_parent_ops *parent_ops)
{
    struct wined3d_device_parent *device_parent = device->device_parent;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    unsigned int i;
    HRESULT hr;

    if (layer_count != 1)
    {
        ERR("Invalid layer count for volume texture.\n");
        return E_INVALIDARG;
    }

    /* TODO: It should only be possible to create textures for formats
     * that are reported as supported. */
    if (WINED3DFMT_UNKNOWN >= desc->format)
    {
        WARN("(%p) : Texture cannot be created with a format of WINED3DFMT_UNKNOWN.\n", texture);
        return WINED3DERR_INVALIDCALL;
    }

    if (!gl_info->supported[EXT_TEXTURE3D])
    {
        WARN("(%p) : Texture cannot be created - no volume texture support.\n", texture);
        return WINED3DERR_INVALIDCALL;
    }

    if (desc->usage & WINED3DUSAGE_DYNAMIC && (wined3d_resource_access_is_managed(desc->access)
            || desc->usage & WINED3DUSAGE_SCRATCH))
    {
        WARN("Attempted to create a DYNAMIC texture with access %s.\n",
                wined3d_debug_resource_access(desc->access));
        return WINED3DERR_INVALIDCALL;
    }

    if (!gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO])
    {
        if (!is_power_of_two(desc->width) || !is_power_of_two(desc->height) || !is_power_of_two(desc->depth))
        {
            if (desc->usage & WINED3DUSAGE_SCRATCH)
            {
                WARN("Creating a scratch NPOT volume texture despite lack of HW support.\n");
            }
            else
            {
                WARN("Attempted to create a NPOT volume texture (%u, %u, %u) without GL support.\n",
                        desc->width, desc->height, desc->depth);
                return WINED3DERR_INVALIDCALL;
            }
        }
    }

    if (FAILED(hr = wined3d_texture_init(texture, &texture3d_ops, 1, level_count, desc,
            flags, device, parent, parent_ops, &texture_resource_ops)))
    {
        WARN("Failed to initialize texture, returning %#x.\n", hr);
        return hr;
    }

    texture->pow2_matrix[0] = 1.0f;
    texture->pow2_matrix[5] = 1.0f;
    texture->pow2_matrix[10] = 1.0f;
    texture->pow2_matrix[15] = 1.0f;
    texture->target = GL_TEXTURE_3D;

    if (wined3d_texture_use_pbo(texture, gl_info))
    {
        wined3d_resource_free_sysmem(&texture->resource);
        texture->resource.map_binding = WINED3D_LOCATION_BUFFER;
    }

    /* Generate all the sub resources. */
    for (i = 0; i < texture->level_count; ++i)
    {
        struct wined3d_texture_sub_resource *sub_resource;

        sub_resource = &texture->sub_resources[i];
        sub_resource->locations = WINED3D_LOCATION_DISCARDED;

        if (FAILED(hr = device_parent->ops->volume_created(device_parent,
                texture, i, &sub_resource->parent, &sub_resource->parent_ops)))
        {
            WARN("Failed to create volume parent, hr %#x.\n", hr);
            sub_resource->parent = NULL;
            wined3d_texture_cleanup_sync(texture);
            return hr;
        }

        TRACE("parent %p, parent_ops %p.\n", parent, parent_ops);

        TRACE("Created volume level %u.\n", i);
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
#if !defined(STAGING_CSMT)
        WARN("Sub-resource is busy, returning WINEDDERR_SURFACEBUSY.\n");
        return WINEDDERR_SURFACEBUSY;
#else  /* STAGING_CSMT */
        struct wined3d_device *device = dst_texture->resource.device;
        device->cs->ops->finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);
        if (dst_texture->sub_resources[dst_sub_resource_idx].map_count
                || (src_texture && src_texture->sub_resources[src_sub_resource_idx].map_count))
        {
            WARN("Sub-resource is busy, returning WINEDDERR_SURFACEBUSY.\n");
            return WINEDDERR_SURFACEBUSY;
        }
#endif /* STAGING_CSMT */
    }

    if ((src_format_flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL))
            != (dst_format_flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL)))
    {
        WARN("Rejecting depth/stencil blit between incompatible formats.\n");
        return WINED3DERR_INVALIDCALL;
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
    if (!overlay->dst)
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
    struct wined3d_texture_sub_resource *sub_resource, *dst_sub_resource;
    struct wined3d_surface *surface, *dst_surface;
    struct wined3d_overlay_info *overlay;

    TRACE("texture %p, sub_resource_idx %u, src_rect %s, dst_texture %p, "
            "dst_sub_resource_idx %u, dst_rect %s, flags %#x.\n",
            texture, sub_resource_idx, wine_dbgstr_rect(src_rect), dst_texture,
            dst_sub_resource_idx, wine_dbgstr_rect(dst_rect), flags);

    if (!(texture->resource.usage & WINED3DUSAGE_OVERLAY) || texture->resource.type != WINED3D_RTYPE_TEXTURE_2D
            || !(sub_resource = wined3d_texture_get_sub_resource(texture, sub_resource_idx)))
    {
        WARN("Invalid sub-resource specified.\n");
        return WINEDDERR_NOTAOVERLAYSURFACE;
    }

    if (!dst_texture || dst_texture->resource.type != WINED3D_RTYPE_TEXTURE_2D
            || !(dst_sub_resource = wined3d_texture_get_sub_resource(dst_texture, dst_sub_resource_idx)))
    {
        WARN("Invalid destination sub-resource specified.\n");
        return WINED3DERR_INVALIDCALL;
    }

    overlay = &texture->overlay_info[sub_resource_idx];

    surface = sub_resource->u.surface;
    if (src_rect)
        overlay->src_rect = *src_rect;
    else
        SetRect(&overlay->src_rect, 0, 0,
                wined3d_texture_get_level_width(texture, surface->texture_level),
                wined3d_texture_get_level_height(texture, surface->texture_level));

    dst_surface = dst_sub_resource->u.surface;
    if (dst_rect)
        overlay->dst_rect = *dst_rect;
    else
        SetRect(&overlay->dst_rect, 0, 0,
                wined3d_texture_get_level_width(dst_texture, dst_surface->texture_level),
                wined3d_texture_get_level_height(dst_texture, dst_surface->texture_level));

    if (overlay->dst && (overlay->dst != dst_surface || flags & WINEDDOVER_HIDE))
    {
        overlay->dst = NULL;
        list_remove(&overlay->entry);
    }

    if (flags & WINEDDOVER_SHOW)
    {
        if (overlay->dst != dst_surface)
        {
            overlay->dst = dst_surface;
            list_add_tail(&texture->overlay_info[dst_sub_resource_idx].overlays, &overlay->entry);
        }
    }
    else if (flags & WINEDDOVER_HIDE)
    {
        /* Tests show that the rectangles are erased on hide. */
        SetRectEmpty(&overlay->src_rect);
        SetRectEmpty(&overlay->dst_rect);
        overlay->dst = NULL;
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
    desc->access = resource->access;

    level_idx = sub_resource_idx % texture->level_count;
    desc->width = wined3d_texture_get_level_width(texture, level_idx);
    desc->height = wined3d_texture_get_level_height(texture, level_idx);
    desc->depth = wined3d_texture_get_level_depth(texture, level_idx);
    desc->size = texture->sub_resources[sub_resource_idx].size;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_texture_create(struct wined3d_device *device, const struct wined3d_resource_desc *desc,
        UINT layer_count, UINT level_count, DWORD flags, const struct wined3d_sub_resource_data *data,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_texture **texture)
{
    struct wined3d_texture *object;
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
        const struct wined3d_format *format = wined3d_get_format(&device->adapter->gl_info,
                desc->format, desc->usage);

        if (desc->multisample_type == WINED3D_MULTISAMPLE_NON_MASKABLE
                && desc->multisample_quality >= wined3d_popcount(format->multisample_types))
        {
            WARN("Unsupported quality level %u requested for WINED3D_MULTISAMPLE_NON_MASKABLE.\n",
                    desc->multisample_quality);
            return WINED3DERR_NOTAVAILABLE;
        }
        if (desc->multisample_type != WINED3D_MULTISAMPLE_NON_MASKABLE
                && (!(format->multisample_types & 1u << (desc->multisample_type - 1))
                || desc->multisample_quality))
        {
            WARN("Unsupported multisample type %u quality %u requested.\n", desc->multisample_type,
                    desc->multisample_quality);
            return WINED3DERR_NOTAVAILABLE;
        }
    }

    if (!(object = heap_alloc_zero(FIELD_OFFSET(struct wined3d_texture,
            sub_resources[level_count * layer_count]))))
        return E_OUTOFMEMORY;

    switch (desc->resource_type)
    {
        case WINED3D_RTYPE_TEXTURE_1D:
            hr = texture1d_init(object, desc, layer_count, level_count, device, parent, parent_ops);
            break;

        case WINED3D_RTYPE_TEXTURE_2D:
            hr = texture_init(object, desc, layer_count, level_count, flags, device, parent, parent_ops);
            break;

        case WINED3D_RTYPE_TEXTURE_3D:
            hr = volumetexture_init(object, desc, layer_count, level_count, flags, device, parent, parent_ops);
            break;

        default:
            ERR("Invalid resource type %s.\n", debug_d3dresourcetype(desc->resource_type));
            hr = WINED3DERR_INVALIDCALL;
            break;
    }

    if (FAILED(hr))
    {
        WARN("Failed to initialize texture, returning %#x.\n", hr);
        heap_free(object);
        return hr;
    }

    /* FIXME: We'd like to avoid ever allocating system memory for the texture
     * in this case. */
    if (data)
    {
        unsigned int sub_count = level_count * layer_count;
        unsigned int i;

        for (i = 0; i < sub_count; ++i)
        {
            if (!data[i].data)
            {
                WARN("Invalid sub-resource data specified for sub-resource %u.\n", i);
                wined3d_texture_cleanup_sync(object);
                heap_free(object);
                return E_INVALIDARG;
            }
        }

        for (i = 0; i < sub_count; ++i)
        {
            wined3d_device_update_sub_resource(device, &object->resource,
                    i, NULL, data[i].data, data[i].row_pitch, data[i].slice_pitch);
        }
    }

    TRACE("Created texture %p.\n", object);
    *texture = object;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_texture_get_dc(struct wined3d_texture *texture, unsigned int sub_resource_idx, HDC *dc)
{
    struct wined3d_device *device = texture->resource.device;
    struct wined3d_texture_sub_resource *sub_resource;
    struct wined3d_surface *surface;

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

    surface = sub_resource->u.surface;

    if (texture->resource.map_count && !(texture->flags & WINED3D_TEXTURE_GET_DC_LENIENT))
        return WINED3DERR_INVALIDCALL;

    if (!surface->dc)
    {
        wined3d_cs_init_object(device->cs, texture2d_create_dc, surface);
        device->cs->ops->finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);
    }
    if (!surface->dc)
        return WINED3DERR_INVALIDCALL;

    if (!(texture->flags & WINED3D_TEXTURE_GET_DC_LENIENT))
        texture->flags |= WINED3D_TEXTURE_DC_IN_USE;
    ++texture->resource.map_count;
    ++sub_resource->map_count;

    *dc = surface->dc;
    TRACE("Returning dc %p.\n", *dc);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_texture_release_dc(struct wined3d_texture *texture, unsigned int sub_resource_idx, HDC dc)
{
    struct wined3d_device *device = texture->resource.device;
    struct wined3d_texture_sub_resource *sub_resource;
    struct wined3d_surface *surface;

    TRACE("texture %p, sub_resource_idx %u, dc %p.\n", texture, sub_resource_idx, dc);

    if (!(sub_resource = wined3d_texture_get_sub_resource(texture, sub_resource_idx)))
        return WINED3DERR_INVALIDCALL;

    if (texture->resource.type != WINED3D_RTYPE_TEXTURE_2D)
    {
        WARN("Not supported on %s resources.\n", debug_d3dresourcetype(texture->resource.type));
        return WINED3DERR_INVALIDCALL;
    }

    surface = sub_resource->u.surface;

    if (!(texture->flags & (WINED3D_TEXTURE_GET_DC_LENIENT | WINED3D_TEXTURE_DC_IN_USE)))
        return WINED3DERR_INVALIDCALL;

    if (surface->dc != dc)
    {
        WARN("Application tries to release invalid DC %p, surface DC is %p.\n", dc, surface->dc);
        return WINED3DERR_INVALIDCALL;
    }

    if (!(texture->resource.usage & WINED3DUSAGE_OWNDC) && !(device->wined3d->flags & WINED3D_NO3D))
    {
        wined3d_cs_destroy_object(device->cs, texture2d_destroy_dc, surface);
        device->cs->ops->finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);
    }

    --sub_resource->map_count;
    if (!--texture->resource.map_count && texture->update_map_binding)
        wined3d_texture_update_map_binding(texture);
    if (!(texture->flags & WINED3D_TEXTURE_GET_DC_LENIENT))
        texture->flags &= ~WINED3D_TEXTURE_DC_IN_USE;

    return WINED3D_OK;
}
