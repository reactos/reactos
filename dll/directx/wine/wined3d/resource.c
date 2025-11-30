/*
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2009-2010 Henri Verbeet for CodeWeavers
 * Copyright 2006-2008, 2013 Stefan Dösinger for CodeWeavers
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
#include "wined3d_vk.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_perf);

static void resource_check_usage(uint32_t usage, unsigned int access)
{
    static const uint32_t handled = WINED3DUSAGE_DYNAMIC
            | WINED3DUSAGE_STATICDECL
            | WINED3DUSAGE_OVERLAY
            | WINED3DUSAGE_SCRATCH
            | WINED3DUSAGE_MANAGED
            | WINED3DUSAGE_CS
            | WINED3DUSAGE_LEGACY_CUBEMAP
            | ~WINED3DUSAGE_MASK;

    /* Write-only CPU access is supposed to result in write-combined mappings
     * being returned. OpenGL doesn't give us explicit control over that, but
     * the hints and access flags we set for typical access patterns on
     * dynamic resources should in theory have the same effect on the OpenGL
     * driver. */

    if (usage & ~handled)
        FIXME("Unhandled usage flags %#x.\n", usage & ~handled);
    if (usage & WINED3DUSAGE_DYNAMIC && access & WINED3D_RESOURCE_ACCESS_MAP_R)
        WARN_(d3d_perf)("WINED3DUSAGE_DYNAMIC used with WINED3D_RESOURCE_ACCESS_MAP_R.\n");
}

HRESULT resource_init(struct wined3d_resource *resource, struct wined3d_device *device,
        enum wined3d_resource_type type, const struct wined3d_format *format,
        enum wined3d_multisample_type multisample_type, unsigned int multisample_quality, uint32_t usage,
        unsigned int bind_flags, unsigned int access, unsigned int width, unsigned int height, unsigned int depth,
        unsigned int size, void *parent, const struct wined3d_parent_ops *parent_ops,
        const struct wined3d_resource_ops *resource_ops)
{
    enum wined3d_gl_resource_type base_type = WINED3D_GL_RES_TYPE_COUNT;
    enum wined3d_gl_resource_type gl_type = WINED3D_GL_RES_TYPE_COUNT;
    unsigned int i;

    static const struct
    {
        enum wined3d_resource_type type;
        DWORD cube_usage;
        enum wined3d_gl_resource_type gl_type;
    }
    resource_types[] =
    {
        {WINED3D_RTYPE_BUFFER,      0,                              WINED3D_GL_RES_TYPE_BUFFER},
        {WINED3D_RTYPE_TEXTURE_1D,  0,                              WINED3D_GL_RES_TYPE_TEX_1D},
        {WINED3D_RTYPE_TEXTURE_2D,  0,                              WINED3D_GL_RES_TYPE_TEX_2D},
        {WINED3D_RTYPE_TEXTURE_2D,  0,                              WINED3D_GL_RES_TYPE_RB},
        {WINED3D_RTYPE_TEXTURE_2D,  WINED3DUSAGE_LEGACY_CUBEMAP,    WINED3D_GL_RES_TYPE_TEX_CUBE},
        {WINED3D_RTYPE_TEXTURE_3D,  0,                              WINED3D_GL_RES_TYPE_TEX_3D},
    };

    resource_check_usage(usage, access);

    if (usage & WINED3DUSAGE_SCRATCH && access & WINED3D_RESOURCE_ACCESS_GPU)
    {
        ERR("Trying to create a scratch resource with access flags %s.\n",
                wined3d_debug_resource_access(access));
        return WINED3DERR_INVALIDCALL;
    }

    if (bind_flags & (WINED3D_BIND_RENDER_TARGET | WINED3D_BIND_DEPTH_STENCIL))
    {
        if ((access & (WINED3D_RESOURCE_ACCESS_CPU | WINED3D_RESOURCE_ACCESS_GPU)) != WINED3D_RESOURCE_ACCESS_GPU)
        {
            WARN("Bind flags %s are incompatible with resource access %s.\n",
                    wined3d_debug_bind_flags(bind_flags), wined3d_debug_resource_access(access));
            return WINED3DERR_INVALIDCALL;
        }

        /* Dynamic and managed usages are incompatible with GPU writes. */
        if (usage & (WINED3DUSAGE_DYNAMIC | WINED3DUSAGE_MANAGED))
        {
            WARN("Bind flags %s are incompatible with resource usage %s.\n",
                    wined3d_debug_bind_flags(bind_flags), debug_d3dusage(usage));
            return WINED3DERR_INVALIDCALL;
        }
    }

    if (!size)
        ERR("Attempting to create a zero-sized resource.\n");

    for (i = 0; i < ARRAY_SIZE(resource_types); ++i)
    {
        if (resource_types[i].type != type
                || resource_types[i].cube_usage != (usage & WINED3DUSAGE_LEGACY_CUBEMAP))
            continue;

        gl_type = resource_types[i].gl_type;
        if (base_type == WINED3D_GL_RES_TYPE_COUNT)
            base_type = gl_type;

        if (type == WINED3D_RTYPE_BUFFER)
            break;

        if ((bind_flags & WINED3D_BIND_RENDER_TARGET)
                && !(format->caps[gl_type] & WINED3D_FORMAT_CAP_RENDERTARGET))
        {
            WARN("Format %s cannot be used for render targets.\n", debug_d3dformat(format->id));
            continue;
        }
        if ((bind_flags & WINED3D_BIND_DEPTH_STENCIL)
                && !(format->caps[gl_type] & WINED3D_FORMAT_CAP_DEPTH_STENCIL))
        {
            WARN("Format %s cannot be used for depth/stencil buffers.\n", debug_d3dformat(format->id));
            continue;
        }
        if ((bind_flags & WINED3D_BIND_SHADER_RESOURCE)
                && !(format->caps[gl_type] & WINED3D_FORMAT_CAP_TEXTURE))
        {
            WARN("Format %s cannot be used for texturing.\n", debug_d3dformat(format->id));
            continue;
        }
        break;
    }

    if (base_type != WINED3D_GL_RES_TYPE_COUNT && i == ARRAY_SIZE(resource_types))
    {
        if (usage & WINED3DUSAGE_SCRATCH)
        {
            /* Needed for proper format information. */
            gl_type = base_type;
        }
        else
        {
            WARN("Did not find a suitable GL resource type for resource type %s.\n",
                    debug_d3dresourcetype(type));
            return WINED3DERR_INVALIDCALL;
        }
    }

    if (base_type != WINED3D_GL_RES_TYPE_COUNT
            && (format->attrs & (WINED3D_FORMAT_ATTR_BLOCKS | WINED3D_FORMAT_ATTR_BLOCKS_NO_VERIFY))
            == WINED3D_FORMAT_ATTR_BLOCKS)
    {
        UINT width_mask = format->block_width - 1;
        UINT height_mask = format->block_height - 1;
        if (width & width_mask || height & height_mask)
            return WINED3DERR_INVALIDCALL;
    }

    resource->ref = 1;
    resource->device = device;
    resource->type = type;
    resource->gl_type = gl_type;
    resource->format = format;
    resource->format_attrs = format->attrs;
    if (gl_type < WINED3D_GL_RES_TYPE_COUNT)
        resource->format_caps = format->caps[gl_type];
    resource->multisample_type = multisample_type;
    resource->multisample_quality = multisample_quality;
    resource->usage = usage;
    resource->bind_flags = bind_flags;
    if (resource->format_attrs & WINED3D_FORMAT_ATTR_MAPPABLE)
        access |= WINED3D_RESOURCE_ACCESS_MAP_R | WINED3D_RESOURCE_ACCESS_MAP_W;
    resource->access = access;
    resource->width = width;
    resource->height = height;
    resource->depth = depth;
    resource->size = size;
    resource->priority = 0;
    resource->parent = parent;
    resource->parent_ops = parent_ops;
    resource->resource_ops = resource_ops;
    resource->map_binding = WINED3D_LOCATION_SYSMEM;
    resource->heap_pointer = NULL;
    resource->heap_memory = NULL;

    /* Check that we have enough video ram left */
    if (!(access & WINED3D_RESOURCE_ACCESS_CPU) && usage & WINED3DUSAGE_VIDMEM_ACCOUNTING)
    {
        if (size > wined3d_device_get_available_texture_mem(device))
        {
            ERR("Out of adapter memory.\n");
            return WINED3DERR_OUTOFVIDEOMEMORY;
        }
        adapter_adjust_memory(device->adapter, size);
    }

    if (!(usage & WINED3DUSAGE_CS))
        device_resource_add(device, resource);

    return WINED3D_OK;
}

static void wined3d_resource_destroy_object(void *object)
{
    struct wined3d_resource *resource = object;

    TRACE("resource %p.\n", resource);

    wined3d_resource_free_sysmem(resource);
    context_resource_released(resource->device, resource);
}

void resource_cleanup(struct wined3d_resource *resource)
{
    TRACE("Cleaning up resource %p.\n", resource);

    if (!(resource->access & WINED3D_RESOURCE_ACCESS_CPU) && resource->usage & WINED3DUSAGE_VIDMEM_ACCOUNTING)
    {
        TRACE("Decrementing device memory pool by %u.\n", resource->size);
        adapter_adjust_memory(resource->device->adapter, (INT64)0 - resource->size);
    }

    if (!(resource->usage & WINED3DUSAGE_CS))
        device_resource_released(resource->device, resource);

    wined3d_resource_reference(resource);
    wined3d_cs_destroy_object(resource->device->cs, wined3d_resource_destroy_object, resource);
}

void resource_unload(struct wined3d_resource *resource)
{
    if (resource->map_count)
        ERR("Resource %p is being unloaded while mapped.\n", resource);
}

unsigned int CDECL wined3d_resource_set_priority(struct wined3d_resource *resource, unsigned int priority)
{
    unsigned int prev;

    if (!(resource->usage & WINED3DUSAGE_MANAGED))
    {
        WARN("Called on non-managed resource %p, ignoring.\n", resource);
        return 0;
    }

    prev = resource->priority;
    resource->priority = priority;
    TRACE("resource %p, new priority %u, returning old priority %u.\n", resource, priority, prev);
    return prev;
}

unsigned int CDECL wined3d_resource_get_priority(const struct wined3d_resource *resource)
{
    TRACE("resource %p, returning %u.\n", resource, resource->priority);
    return resource->priority;
}

void * CDECL wined3d_resource_get_parent(const struct wined3d_resource *resource)
{
    return resource->parent;
}

void CDECL wined3d_resource_set_parent(struct wined3d_resource *resource, void *parent, const struct wined3d_parent_ops *parent_ops)
{
    resource->parent = parent;
    resource->parent_ops = parent_ops;
}

void CDECL wined3d_resource_get_desc(const struct wined3d_resource *resource, struct wined3d_resource_desc *desc)
{
    desc->resource_type = resource->type;
    desc->format = resource->format->id;
    desc->multisample_type = resource->multisample_type;
    desc->multisample_quality = resource->multisample_quality;
    desc->usage = resource->usage;
    desc->bind_flags = resource->bind_flags;
    desc->access = resource->access;
    desc->width = resource->width;
    desc->height = resource->height;
    desc->depth = resource->depth;
    desc->size = resource->size;
}

HRESULT CDECL wined3d_resource_map(struct wined3d_resource *resource, unsigned int sub_resource_idx,
        struct wined3d_map_desc *map_desc, const struct wined3d_box *box, uint32_t flags)
{
    TRACE("resource %p, sub_resource_idx %u, map_desc %p, box %s, flags %#x.\n",
            resource, sub_resource_idx, map_desc, debug_box(box), flags);

    return wined3d_device_context_map(&resource->device->cs->c, resource, sub_resource_idx, map_desc, box, flags);
}

HRESULT CDECL wined3d_resource_unmap(struct wined3d_resource *resource, unsigned int sub_resource_idx)
{
    TRACE("resource %p, sub_resource_idx %u.\n", resource, sub_resource_idx);

    return wined3d_device_context_unmap(&resource->device->cs->c, resource, sub_resource_idx);
}

void CDECL wined3d_resource_preload(struct wined3d_resource *resource)
{
    wined3d_cs_emit_preload_resource(resource->device->cs, resource);
}

static BOOL wined3d_resource_allocate_sysmem(struct wined3d_resource *resource)
{
    /* Overallocate and add padding to the allocated pointer, to guard against
     * games (for instance Railroad Tycoon 2) writing before the locked resource
     * memory pointer.
     */
    static const SIZE_T align = RESOURCE_ALIGNMENT;
    void *mem;

    if (!(mem = calloc(1, resource->size + align)))
    {
        ERR("Failed to allocate system memory.\n");
        return FALSE;
    }

    resource->heap_memory = (void *)(((ULONG_PTR)mem + align) & ~(RESOURCE_ALIGNMENT - 1));
    resource->heap_pointer = mem;

    return TRUE;
}

BOOL wined3d_resource_prepare_sysmem(struct wined3d_resource *resource)
{
    if (resource->heap_memory)
        return TRUE;

    return wined3d_resource_allocate_sysmem(resource);
}

void wined3d_resource_free_sysmem(struct wined3d_resource *resource)
{
    if (!resource->heap_memory)
        return;
    resource->heap_memory = NULL;

    free(resource->heap_pointer);
    resource->heap_pointer = NULL;
}

GLbitfield wined3d_resource_gl_storage_flags(const struct wined3d_resource *resource)
{
    uint32_t access = resource->access;
    GLbitfield flags = 0;

    if (resource->usage & WINED3DUSAGE_DYNAMIC)
        flags |= GL_CLIENT_STORAGE_BIT;
    if (!(access & WINED3D_RESOURCE_ACCESS_CPU))
    {
        if (access & WINED3D_RESOURCE_ACCESS_MAP_W)
            flags |= GL_MAP_WRITE_BIT;
        if (access & WINED3D_RESOURCE_ACCESS_MAP_R)
            flags |= GL_MAP_READ_BIT;
    }

    return flags;
}

GLbitfield wined3d_resource_gl_map_flags(const struct wined3d_bo_gl *bo, DWORD d3d_flags)
{
    GLbitfield ret = 0;

    if (d3d_flags & WINED3D_MAP_WRITE)
    {
        ret |= GL_MAP_WRITE_BIT;
        if (!bo->b.coherent)
            ret |= GL_MAP_FLUSH_EXPLICIT_BIT;
    }
    if (d3d_flags & WINED3D_MAP_READ)
        ret |= GL_MAP_READ_BIT;
    else
        ret |= GL_MAP_UNSYNCHRONIZED_BIT;

    return ret;
}

GLenum wined3d_resource_gl_legacy_map_flags(DWORD d3d_flags)
{
    switch (d3d_flags & (WINED3D_MAP_READ | WINED3D_MAP_WRITE))
    {
        case WINED3D_MAP_READ:
            return GL_READ_ONLY_ARB;

        case WINED3D_MAP_WRITE:
            return GL_WRITE_ONLY_ARB;

        default:
            return GL_READ_WRITE_ARB;
    }
}

BOOL wined3d_resource_is_offscreen(struct wined3d_resource *resource)
{
    struct wined3d_swapchain *swapchain;

    /* Only 2D texture resources can be onscreen. */
    if (resource->type != WINED3D_RTYPE_TEXTURE_2D)
        return TRUE;

    /* Not on a swapchain - must be offscreen */
    if (!(swapchain = texture_from_resource(resource)->swapchain))
        return TRUE;

    /* The front buffer is always onscreen */
    if (resource == &swapchain->front_buffer->resource)
        return FALSE;

    return TRUE;
}

void wined3d_resource_update_draw_binding(struct wined3d_resource *resource)
{
    const struct wined3d_d3d_info *d3d_info = &resource->device->adapter->d3d_info;

    if (!wined3d_resource_is_offscreen(resource))
    {
        resource->draw_binding = WINED3D_LOCATION_DRAWABLE;
    }
    else if (resource->multisample_type)
    {
        resource->draw_binding = d3d_info->multisample_draw_location;
    }
    else if (resource->gl_type == WINED3D_GL_RES_TYPE_RB)
    {
        resource->draw_binding = WINED3D_LOCATION_RB_RESOLVED;
    }
    else
    {
        resource->draw_binding = WINED3D_LOCATION_TEXTURE_RGB;
    }
}

const struct wined3d_format *wined3d_resource_get_decompress_format(const struct wined3d_resource *resource)
{
    const struct wined3d_adapter *adapter = resource->device->adapter;
    if (resource->format_caps & (WINED3D_FORMAT_CAP_SRGB_READ | WINED3D_FORMAT_CAP_SRGB_WRITE)
            && !(adapter->d3d_info.wined3d_creation_flags & WINED3D_SRGB_READ_WRITE_CONTROL))
        return wined3d_get_format(adapter, WINED3DFMT_B8G8R8A8_UNORM_SRGB, resource->bind_flags);
    return wined3d_get_format(adapter, WINED3DFMT_B8G8R8A8_UNORM, resource->bind_flags);
}

unsigned int wined3d_resource_get_sample_count(const struct wined3d_resource *resource)
{
    const struct wined3d_format *format = resource->format;

    /* TODO: NVIDIA expose their Coverage Sample Anti-Aliasing (CSAA)
     * feature through type == MULTISAMPLE_XX and quality != 0. This could
     * be mapped to GL_NV_framebuffer_multisample_coverage.
     *
     * AMD have a similar feature called Enhanced Quality Anti-Aliasing
     * (EQAA), but it does not have an equivalent OpenGL extension. */

    /* We advertise as many WINED3D_MULTISAMPLE_NON_MASKABLE quality
     * levels as the count of advertised multisample types for the texture
     * format. */
    if (resource->multisample_type == WINED3D_MULTISAMPLE_NON_MASKABLE)
    {
        unsigned int i, count = 0;

        for (i = 0; i < sizeof(format->multisample_types) * CHAR_BIT; ++i)
        {
            if (format->multisample_types & 1u << i)
            {
                if (resource->multisample_quality == count++)
                    break;
            }
        }
        return i + 1;
    }

    return resource->multisample_type;
}

HRESULT wined3d_resource_check_box_dimensions(struct wined3d_resource *resource,
        unsigned int sub_resource_idx, const struct wined3d_box *box)
{
    const struct wined3d_format *format = resource->format;
    struct wined3d_sub_resource_desc desc;
    unsigned int width_mask, height_mask;

    wined3d_resource_get_sub_resource_desc(resource, sub_resource_idx, &desc);

    if (box->left >= box->right || box->right > desc.width
            || box->top >= box->bottom || box->bottom > desc.height
            || box->front >= box->back || box->back > desc.depth)
    {
        WARN("Box %s is invalid.\n", debug_box(box));
        return WINEDDERR_INVALIDRECT;
    }

    if (resource->format_attrs & WINED3D_FORMAT_ATTR_BLOCKS)
    {
        /* This assumes power of two block sizes, but NPOT block sizes would
         * be silly anyway.
         *
         * This also assumes that the format's block depth is 1. */
        width_mask = format->block_width - 1;
        height_mask = format->block_height - 1;

        if ((box->left & width_mask) || (box->top & height_mask)
                || (box->right & width_mask && box->right != desc.width)
                || (box->bottom & height_mask && box->bottom != desc.height))
        {
            WARN("Box %s is misaligned for %ux%u blocks.\n", debug_box(box), format->block_width, format->block_height);
            return WINED3DERR_INVALIDCALL;
        }
    }

    return WINED3D_OK;
}

VkAccessFlags vk_access_mask_from_bind_flags(uint32_t bind_flags)
{
    VkAccessFlags flags = 0;

    if (bind_flags & WINED3D_BIND_VERTEX_BUFFER)
        flags |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
    if (bind_flags & WINED3D_BIND_INDEX_BUFFER)
        flags |= VK_ACCESS_INDEX_READ_BIT;
    if (bind_flags & WINED3D_BIND_CONSTANT_BUFFER)
        flags |= VK_ACCESS_UNIFORM_READ_BIT;
    if (bind_flags & WINED3D_BIND_SHADER_RESOURCE)
        flags |= VK_ACCESS_SHADER_READ_BIT;
    if (bind_flags & WINED3D_BIND_UNORDERED_ACCESS)
        flags |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    if (bind_flags & WINED3D_BIND_INDIRECT_BUFFER)
        flags |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
    if (bind_flags & WINED3D_BIND_RENDER_TARGET)
        flags |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    if (bind_flags & WINED3D_BIND_DEPTH_STENCIL)
        flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    if (bind_flags & WINED3D_BIND_STREAM_OUTPUT)
        flags |= VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT;

    return flags;
}

VkPipelineStageFlags vk_pipeline_stage_mask_from_bind_flags(uint32_t bind_flags)
{
    VkPipelineStageFlags flags = 0;

    if (bind_flags & (WINED3D_BIND_VERTEX_BUFFER | WINED3D_BIND_INDEX_BUFFER))
        flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
    if (bind_flags & (WINED3D_BIND_CONSTANT_BUFFER | WINED3D_BIND_SHADER_RESOURCE | WINED3D_BIND_UNORDERED_ACCESS))
        flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT
                | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT
                | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    if (bind_flags & WINED3D_BIND_INDIRECT_BUFFER)
        flags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
    if (bind_flags & WINED3D_BIND_RENDER_TARGET)
        flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    if (bind_flags & WINED3D_BIND_DEPTH_STENCIL)
        flags |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    if (bind_flags & WINED3D_BIND_STREAM_OUTPUT)
        flags |= VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT;

    return flags;
}

void *resource_offset_map_pointer(struct wined3d_resource *resource, unsigned int sub_resource_idx,
        uint8_t *base_memory, const struct wined3d_box *box)
{
    const struct wined3d_format *format = resource->format;
    unsigned int row_pitch, slice_pitch;

    wined3d_resource_get_sub_resource_map_pitch(resource, sub_resource_idx, &row_pitch, &slice_pitch);

    if ((resource->format_attrs & (WINED3D_FORMAT_ATTR_BLOCKS | WINED3D_FORMAT_ATTR_BROKEN_PITCH))
            == WINED3D_FORMAT_ATTR_BLOCKS)
    {
        /* Compressed textures are block based, so calculate the offset of
         * the block that contains the top-left pixel of the mapped box. */
        return base_memory
                + (box->front * slice_pitch)
                + ((box->top / format->block_height) * row_pitch)
                + ((box->left / format->block_width) * format->block_byte_count);
    }
    else
    {
        return base_memory
                + (box->front * slice_pitch)
                + (box->top * row_pitch)
                + (box->left * format->byte_count);
    }
}

void wined3d_resource_memory_colour_fill(struct wined3d_resource *resource,
        const struct wined3d_map_desc *map, const struct wined3d_color *colour,
        const struct wined3d_box *box, bool full_subresource)
{
    const struct wined3d_format *format = resource->format;
    unsigned int w, h, d, x, y, z, bpp;
    uint8_t *dst, *dst2;
    uint32_t c[4];

    /* Fast and simple path for setting everything to zero. The C library's memset is
     * more sophisticated than our code below. Also this works for block formats, which
     * we still need to zero-initialize for newly created resources. */
    if (full_subresource && !colour->r && !colour->g && !colour->b && !colour->a)
    {
        memset(map->data, 0, map->slice_pitch * box->back);
        return;
    }

    w = box->right - box->left;
    h = box->bottom - box->top;
    d = box->back - box->front;

    dst = (uint8_t *)map->data
            + (box->front * map->slice_pitch)
            + ((box->top / format->block_height) * map->row_pitch)
            + ((box->left / format->block_width) * format->block_byte_count);

    wined3d_format_convert_from_float(format, colour, c);
    bpp = format->byte_count;

    switch (bpp)
    {
        case 1:
            for (x = 0; x < w; ++x)
            {
                dst[x] = c[0];
            }
            break;

        case 2:
            for (x = 0; x < w; ++x)
            {
                ((uint16_t *)dst)[x] = c[0];
            }
            break;

        case 3:
        {
            dst2 = dst;
            for (x = 0; x < w; ++x, dst2 += 3)
            {
                dst2[0] = (c[0]      ) & 0xff;
                dst2[1] = (c[0] >>  8) & 0xff;
                dst2[2] = (c[0] >> 16) & 0xff;
            }
            break;
        }
        case 4:
            for (x = 0; x < w; ++x)
            {
                ((uint32_t *)dst)[x] = c[0];
            }
            break;

        case 8:
        case 12:
        case 16:
            for (x = 0; x < w; ++x)
                memcpy(((uint8_t *)map->data) + x * bpp, c, bpp);
            break;

        default:
            FIXME("Not implemented for bpp %u.\n", bpp);
            return;
    }

    dst2 = dst;
    for (y = 1; y < h; ++y)
    {
        dst2 += map->row_pitch;
        memcpy(dst2, dst, w * bpp);
    }

    dst2 = dst;
    for (z = 1; z < d; ++z)
    {
        dst2 += map->slice_pitch;
        memcpy(dst2, dst, w * h * bpp);
    }

}
