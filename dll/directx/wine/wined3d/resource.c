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
    const struct wined3d *d3d = device->wined3d;

    resource_check_usage(usage);
    if (pool != WINED3D_POOL_SCRATCH && type != WINED3D_RTYPE_BUFFER)
    {
        if ((usage & WINED3DUSAGE_RENDERTARGET) && !(format->flags & WINED3DFMT_FLAG_RENDERTARGET))
        {
            WARN("Format %s cannot be used for render targets.\n", debug_d3dformat(format->id));
            return WINED3DERR_INVALIDCALL;
        }
        if ((usage & WINED3DUSAGE_DEPTHSTENCIL) && !(format->flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL)))
        {
            WARN("Format %s cannot be used for depth/stencil buffers.\n", debug_d3dformat(format->id));
            return WINED3DERR_INVALIDCALL;
        }
        if ((usage & WINED3DUSAGE_TEXTURE) && !(format->flags & WINED3DFMT_FLAG_TEXTURE))
        {
            WARN("Format %s cannot be used for texturing.\n", debug_d3dformat(format->id));
            return WINED3DERR_INVALIDCALL;
        }
    }

    resource->ref = 1;
    resource->device = device;
    resource->type = type;
    resource->format = format;
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

    /* Check that we have enough video ram left */
    if (pool == WINED3D_POOL_DEFAULT && d3d->flags & WINED3D_VIDMEM_ACCOUNTING)
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

    return WINED3D_OK;
}

void resource_cleanup(struct wined3d_resource *resource)
{
    const struct wined3d *d3d = resource->device->wined3d;

    TRACE("Cleaning up resource %p.\n", resource);

    if (resource->pool == WINED3D_POOL_DEFAULT && d3d->flags & WINED3D_VIDMEM_ACCOUNTING)
    {
        TRACE("Decrementing device memory pool by %u.\n", resource->size);
        adapter_adjust_memory(resource->device->adapter, (INT64)0 - resource->size);
    }

    wined3d_resource_free_sysmem(resource);

    device_resource_released(resource->device, resource);
}

void resource_unload(struct wined3d_resource *resource)
{
    if (resource->map_count)
        ERR("Resource %p is being unloaded while mapped.\n", resource);

    context_resource_unloaded(resource->device,
            resource, resource->type);
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

DWORD wined3d_resource_sanitize_map_flags(const struct wined3d_resource *resource, DWORD flags)
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

    /* Only texture resources can be onscreen. */
    if (resource->type != WINED3D_RTYPE_TEXTURE)
        return TRUE;

    /* Not on a swapchain - must be offscreen */
    if (!(swapchain = wined3d_texture_from_resource(resource)->swapchain))
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
    else
        resource->draw_binding = WINED3D_LOCATION_TEXTURE_RGB;
}
