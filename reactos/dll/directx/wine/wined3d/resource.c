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

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_perf);

static DWORD resource_access_from_pool(enum wined3d_pool pool)
{
    switch (pool)
    {
        case WINED3D_POOL_DEFAULT:
            return WINED3D_RESOURCE_ACCESS_GPU;

        case WINED3D_POOL_MANAGED:
            return WINED3D_RESOURCE_ACCESS_GPU | WINED3D_RESOURCE_ACCESS_CPU;

        case WINED3D_POOL_SCRATCH:
        case WINED3D_POOL_SYSTEM_MEM:
            return WINED3D_RESOURCE_ACCESS_CPU;

        default:
            FIXME("Unhandled pool %#x.\n", pool);
            return 0;
    }
}

static void resource_check_usage(DWORD usage)
{
    static DWORD handled = WINED3DUSAGE_RENDERTARGET
            | WINED3DUSAGE_DEPTHSTENCIL
            | WINED3DUSAGE_WRITEONLY
            | WINED3DUSAGE_DYNAMIC
            | WINED3DUSAGE_AUTOGENMIPMAP
            | WINED3DUSAGE_STATICDECL
            | WINED3DUSAGE_OVERLAY
            | WINED3DUSAGE_PRIVATE
            | WINED3DUSAGE_LEGACY_CUBEMAP
            | WINED3DUSAGE_TEXTURE;

    /* WINED3DUSAGE_WRITEONLY is supposed to result in write-combined mappings
     * being returned. OpenGL doesn't give us explicit control over that, but
     * the hints and access flags we set for typical access patterns on
     * dynamic resources should in theory have the same effect on the OpenGL
     * driver. */

    if (usage & ~handled)
    {
        FIXME("Unhandled usage flags %#x.\n", usage & ~handled);
        handled |= usage;
    }
    if ((usage & (WINED3DUSAGE_DYNAMIC | WINED3DUSAGE_WRITEONLY)) == WINED3DUSAGE_DYNAMIC)
        WARN_(d3d_perf)("WINED3DUSAGE_DYNAMIC used without WINED3DUSAGE_WRITEONLY.\n");
}

HRESULT resource_init(struct wined3d_resource *resource, struct wined3d_device *device,
        enum wined3d_resource_type type, const struct wined3d_format *format,
        enum wined3d_multisample_type multisample_type, UINT multisample_quality,
        DWORD usage, enum wined3d_pool pool, UINT width, UINT height, UINT depth, UINT size,
        void *parent, const struct wined3d_parent_ops *parent_ops,
        const struct wined3d_resource_ops *resource_ops)
{
    enum wined3d_gl_resource_type base_type = WINED3D_GL_RES_TYPE_COUNT;
    enum wined3d_gl_resource_type gl_type = WINED3D_GL_RES_TYPE_COUNT;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    BOOL tex_2d_ok = FALSE;
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
        {WINED3D_RTYPE_TEXTURE_2D,  0,                              WINED3D_GL_RES_TYPE_TEX_RECT},
        {WINED3D_RTYPE_TEXTURE_2D,  0,                              WINED3D_GL_RES_TYPE_RB},
        {WINED3D_RTYPE_TEXTURE_2D,  WINED3DUSAGE_LEGACY_CUBEMAP,    WINED3D_GL_RES_TYPE_TEX_CUBE},
        {WINED3D_RTYPE_TEXTURE_3D,  0,                              WINED3D_GL_RES_TYPE_TEX_3D},
    };

    resource_check_usage(usage);

    for (i = 0; i < ARRAY_SIZE(resource_types); ++i)
    {
        if (resource_types[i].type != type
                || resource_types[i].cube_usage != (usage & WINED3DUSAGE_LEGACY_CUBEMAP))
            continue;

        gl_type = resource_types[i].gl_type;
        if (base_type == WINED3D_GL_RES_TYPE_COUNT)
            base_type = gl_type;

        if ((usage & WINED3DUSAGE_RENDERTARGET) && !(format->flags[gl_type] & WINED3DFMT_FLAG_RENDERTARGET))
        {
            WARN("Format %s cannot be used for render targets.\n", debug_d3dformat(format->id));
            continue;
        }
        if ((usage & WINED3DUSAGE_DEPTHSTENCIL)
                && !(format->flags[gl_type] & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL)))
        {
            WARN("Format %s cannot be used for depth/stencil buffers.\n", debug_d3dformat(format->id));
            continue;
        }
        if (wined3d_settings.offscreen_rendering_mode == ORM_FBO
                && usage & (WINED3DUSAGE_RENDERTARGET | WINED3DUSAGE_DEPTHSTENCIL)
                && !(format->flags[gl_type] & WINED3DFMT_FLAG_FBO_ATTACHABLE))
        {
            WARN("Render target or depth stencil is not FBO attachable.\n");
            continue;
        }
        if ((usage & WINED3DUSAGE_TEXTURE) && !(format->flags[gl_type] & WINED3DFMT_FLAG_TEXTURE))
        {
            WARN("Format %s cannot be used for texturing.\n", debug_d3dformat(format->id));
            continue;
        }
        if (((width & (width - 1)) || (height & (height - 1)))
                && !gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO]
                && !gl_info->supported[WINED3D_GL_NORMALIZED_TEXRECT]
                && gl_type == WINED3D_GL_RES_TYPE_TEX_2D)
        {
            TRACE("Skipping 2D texture type to try texture rectangle.\n");
            tex_2d_ok = TRUE;
            continue;
        }
        break;
    }

    if (base_type != WINED3D_GL_RES_TYPE_COUNT && i == ARRAY_SIZE(resource_types))
    {
        if (tex_2d_ok)
        {
            /* Non power of 2 texture and rectangle textures or renderbuffers do not work.
             * Use 2D textures, the texture code will pad to a power of 2 size. */
            gl_type = WINED3D_GL_RES_TYPE_TEX_2D;
        }
        else if (pool == WINED3D_POOL_SCRATCH)
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
            && (format->flags[base_type] & (WINED3DFMT_FLAG_BLOCKS | WINED3DFMT_FLAG_BLOCKS_NO_VERIFY))
            == WINED3DFMT_FLAG_BLOCKS)
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
    if (gl_type < WINED3D_GL_RES_TYPE_COUNT)
        resource->format_flags = format->flags[gl_type];
    resource->multisample_type = multisample_type;
    resource->multisample_quality = multisample_quality;
    resource->usage = usage;
    resource->pool = pool;
    resource->access_flags = resource_access_from_pool(pool);
    if (usage & WINED3DUSAGE_DYNAMIC)
        resource->access_flags |= WINED3D_RESOURCE_ACCESS_CPU;
    resource->width = width;
    resource->height = height;
    resource->depth = depth;
    resource->size = size;
    resource->priority = 0;
    resource->parent = parent;
    resource->parent_ops = parent_ops;
    resource->resource_ops = resource_ops;
    resource->map_binding = WINED3D_LOCATION_SYSMEM;

    if (size)
    {
        if (!wined3d_resource_allocate_sysmem(resource))
        {
            ERR("Failed to allocate system memory.\n");
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        resource->heap_memory = NULL;
    }

    if (!(usage & WINED3DUSAGE_PRIVATE))
    {
        /* Check that we have enough video ram left */
        if (pool == WINED3D_POOL_DEFAULT && device->wined3d->flags & WINED3D_VIDMEM_ACCOUNTING)
        {
            if (size > wined3d_device_get_available_texture_mem(device))
            {
                ERR("Out of adapter memory\n");
                wined3d_resource_free_sysmem(resource);
                return WINED3DERR_OUTOFVIDEOMEMORY;
            }
            adapter_adjust_memory(device->adapter, size);
        }

        device_resource_add(device, resource);
    }

    return WINED3D_OK;
}

static void wined3d_resource_destroy_object(void *object)
{
    struct wined3d_resource *resource = object;

    wined3d_resource_free_sysmem(resource);
    context_resource_released(resource->device, resource, resource->type);
    wined3d_resource_release(resource);
}

void resource_cleanup(struct wined3d_resource *resource)
{
    const struct wined3d *d3d = resource->device->wined3d;

    TRACE("Cleaning up resource %p.\n", resource);

    if (!(resource->usage & WINED3DUSAGE_PRIVATE))
    {
        if (resource->pool == WINED3D_POOL_DEFAULT && d3d->flags & WINED3D_VIDMEM_ACCOUNTING)
        {
            TRACE("Decrementing device memory pool by %u.\n", resource->size);
            adapter_adjust_memory(resource->device->adapter, (INT64)0 - resource->size);
        }

        device_resource_released(resource->device, resource);
    }
    wined3d_resource_acquire(resource);
    wined3d_cs_destroy_object(resource->device->cs, wined3d_resource_destroy_object, resource);
}

void resource_unload(struct wined3d_resource *resource)
{
    if (resource->map_count)
        ERR("Resource %p is being unloaded while mapped.\n", resource);
}

DWORD CDECL wined3d_resource_set_priority(struct wined3d_resource *resource, DWORD priority)
{
    DWORD prev;

    if (resource->pool != WINED3D_POOL_MANAGED)
    {
        WARN("Called on non-managed resource %p, ignoring.\n", resource);
        return 0;
    }

    prev = resource->priority;
    resource->priority = priority;
    TRACE("resource %p, new priority %u, returning old priority %u.\n", resource, priority, prev);
    return prev;
}

DWORD CDECL wined3d_resource_get_priority(const struct wined3d_resource *resource)
{
    TRACE("resource %p, returning %u.\n", resource, resource->priority);
    return resource->priority;
}

void * CDECL wined3d_resource_get_parent(const struct wined3d_resource *resource)
{
    return resource->parent;
}

void CDECL wined3d_resource_set_parent(struct wined3d_resource *resource, void *parent)
{
    resource->parent = parent;
}

void CDECL wined3d_resource_get_desc(const struct wined3d_resource *resource, struct wined3d_resource_desc *desc)
{
    desc->resource_type = resource->type;
    desc->format = resource->format->id;
    desc->multisample_type = resource->multisample_type;
    desc->multisample_quality = resource->multisample_quality;
    desc->usage = resource->usage;
    desc->pool = resource->pool;
    desc->width = resource->width;
    desc->height = resource->height;
    desc->depth = resource->depth;
    desc->size = resource->size;
}

static DWORD wined3d_resource_sanitise_map_flags(const struct wined3d_resource *resource, DWORD flags)
{
    /* Not all flags make sense together, but Windows never returns an error.
     * Catch the cases that could cause issues. */
    if (flags & WINED3D_MAP_READONLY)
    {
        if (flags & WINED3D_MAP_DISCARD)
        {
            WARN("WINED3D_MAP_READONLY combined with WINED3D_MAP_DISCARD, ignoring flags.\n");
            return 0;
        }
        if (flags & WINED3D_MAP_NOOVERWRITE)
        {
            WARN("WINED3D_MAP_READONLY combined with WINED3D_MAP_NOOVERWRITE, ignoring flags.\n");
            return 0;
        }
    }
    else if ((flags & (WINED3D_MAP_DISCARD | WINED3D_MAP_NOOVERWRITE))
            == (WINED3D_MAP_DISCARD | WINED3D_MAP_NOOVERWRITE))
    {
        WARN("WINED3D_MAP_DISCARD and WINED3D_MAP_NOOVERWRITE used together, ignoring.\n");
        return 0;
    }
    else if (flags & (WINED3D_MAP_DISCARD | WINED3D_MAP_NOOVERWRITE)
            && !(resource->usage & WINED3DUSAGE_DYNAMIC))
    {
        WARN("DISCARD or NOOVERWRITE map on non-dynamic buffer, ignoring.\n");
        return 0;
    }

    return flags;
}

HRESULT CDECL wined3d_resource_map(struct wined3d_resource *resource, unsigned int sub_resource_idx,
        struct wined3d_map_desc *map_desc, const struct wined3d_box *box, DWORD flags)
{
    TRACE("resource %p, sub_resource_idx %u, map_desc %p, box %s, flags %#x.\n",
            resource, sub_resource_idx, map_desc, debug_box(box), flags);

    flags = wined3d_resource_sanitise_map_flags(resource, flags);
    wined3d_resource_wait_idle(resource);

    return wined3d_cs_map(resource->device->cs, resource, sub_resource_idx, map_desc, box, flags);
}

HRESULT CDECL wined3d_resource_map_info(struct wined3d_resource *resource, unsigned int sub_resource_idx,
        struct wined3d_map_info *info, DWORD flags)
{
    TRACE("resource %p, sub_resource_idx %u.\n", resource, sub_resource_idx);

    return resource->resource_ops->resource_map_info(resource, sub_resource_idx, info, flags);
}

HRESULT CDECL wined3d_resource_unmap(struct wined3d_resource *resource, unsigned int sub_resource_idx)
{
    TRACE("resource %p, sub_resource_idx %u.\n", resource, sub_resource_idx);

    return wined3d_cs_unmap(resource->device->cs, resource, sub_resource_idx);
}

void CDECL wined3d_resource_preload(struct wined3d_resource *resource)
{
    wined3d_cs_emit_preload_resource(resource->device->cs, resource);
}

BOOL wined3d_resource_allocate_sysmem(struct wined3d_resource *resource)
{
    void **p;
    SIZE_T align = RESOURCE_ALIGNMENT - 1 + sizeof(*p);
    void *mem;

    if (!(mem = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, resource->size + align)))
        return FALSE;

    p = (void **)(((ULONG_PTR)mem + align) & ~(RESOURCE_ALIGNMENT - 1)) - 1;
    *p = mem;

    resource->heap_memory = ++p;

    return TRUE;
}

void wined3d_resource_free_sysmem(struct wined3d_resource *resource)
{
    void **p = resource->heap_memory;

    if (!p)
        return;

    HeapFree(GetProcessHeap(), 0, *(--p));
    resource->heap_memory = NULL;
}

GLbitfield wined3d_resource_gl_map_flags(DWORD d3d_flags)
{
    GLbitfield ret = 0;

    if (!(d3d_flags & WINED3D_MAP_READONLY))
        ret |= GL_MAP_WRITE_BIT | GL_MAP_FLUSH_EXPLICIT_BIT;
    if (!(d3d_flags & (WINED3D_MAP_DISCARD | WINED3D_MAP_NOOVERWRITE)))
        ret |= GL_MAP_READ_BIT;

    if (d3d_flags & WINED3D_MAP_DISCARD)
        ret |= GL_MAP_INVALIDATE_BUFFER_BIT;
    if (d3d_flags & WINED3D_MAP_NOOVERWRITE)
        ret |= GL_MAP_UNSYNCHRONIZED_BIT;

    return ret;
}

GLenum wined3d_resource_gl_legacy_map_flags(DWORD d3d_flags)
{
    if (d3d_flags & WINED3D_MAP_READONLY)
        return GL_READ_ONLY_ARB;
    if (d3d_flags & (WINED3D_MAP_DISCARD | WINED3D_MAP_NOOVERWRITE))
        return GL_WRITE_ONLY_ARB;
    return GL_READ_WRITE_ARB;
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

    /* If the swapchain is rendered to an FBO, the backbuffer is
     * offscreen, otherwise onscreen */
    return swapchain->render_to_fbo;
}

void wined3d_resource_update_draw_binding(struct wined3d_resource *resource)
{
    if (!wined3d_resource_is_offscreen(resource) || wined3d_settings.offscreen_rendering_mode != ORM_FBO)
        resource->draw_binding = WINED3D_LOCATION_DRAWABLE;
    else if (resource->multisample_type)
        resource->draw_binding = WINED3D_LOCATION_RB_MULTISAMPLE;
    else if (resource->gl_type == WINED3D_GL_RES_TYPE_RB)
        resource->draw_binding = WINED3D_LOCATION_RB_RESOLVED;
    else
        resource->draw_binding = WINED3D_LOCATION_TEXTURE_RGB;
}
