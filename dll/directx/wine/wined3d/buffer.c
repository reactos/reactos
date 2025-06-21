/*
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2007-2011, 2013-2014 Stefan Dösinger for CodeWeavers
 * Copyright 2009-2010 Henri Verbeet for CodeWeavers
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

#define WINED3D_BUFFER_HASDESC      0x01    /* A vertex description has been found. */
#define WINED3D_BUFFER_USE_BO       0x02    /* Use a buffer object for this buffer. */

#define SB_MIN_SIZE (512 * 1024)    /* Minimum size of an allocated streaming buffer. */

struct wined3d_buffer_ops
{
    BOOL (*buffer_prepare_location)(struct wined3d_buffer *buffer,
            struct wined3d_context *context, unsigned int location);
    void (*buffer_unload_location)(struct wined3d_buffer *buffer,
            struct wined3d_context *context, unsigned int location);
};

static void wined3d_buffer_evict_sysmem(struct wined3d_buffer *buffer)
{
    if (buffer->resource.pin_sysmem)
    {
        TRACE("Not evicting system memory for buffer %p.\n", buffer);
        return;
    }

    TRACE("Evicting system memory for buffer %p.\n", buffer);
    wined3d_buffer_invalidate_location(buffer, WINED3D_LOCATION_SYSMEM);
    wined3d_resource_free_sysmem(&buffer->resource);
}

static void buffer_invalidate_bo_range(struct wined3d_buffer *buffer, unsigned int offset, unsigned int size)
{
    if (!offset && (!size || size == buffer->resource.size))
        goto invalidate_all;

    if (offset > buffer->resource.size || size > buffer->resource.size - offset)
    {
        WARN("Invalid range specified, invalidating entire buffer.\n");
        goto invalidate_all;
    }

    if (!wined3d_array_reserve((void **)&buffer->dirty_ranges, &buffer->dirty_ranges_capacity,
            buffer->dirty_range_count + 1, sizeof(*buffer->dirty_ranges)))
    {
        ERR("Failed to allocate dirty ranges array, invalidating entire buffer.\n");
        goto invalidate_all;
    }

    buffer->dirty_ranges[buffer->dirty_range_count].offset = offset;
    buffer->dirty_ranges[buffer->dirty_range_count].size = size;
    ++buffer->dirty_range_count;
    return;

invalidate_all:
    buffer->dirty_range_count = 1;
    buffer->dirty_ranges[0].offset = 0;
    buffer->dirty_ranges[0].size = buffer->resource.size;
}

static inline void buffer_clear_dirty_areas(struct wined3d_buffer *buffer)
{
    buffer->dirty_range_count = 0;
}

static BOOL buffer_is_dirty(const struct wined3d_buffer *buffer)
{
    return !!buffer->dirty_range_count;
}

static BOOL buffer_is_fully_dirty(const struct wined3d_buffer *buffer)
{
    return buffer->dirty_range_count == 1
            && !buffer->dirty_ranges[0].offset && buffer->dirty_ranges[0].size == buffer->resource.size;
}

void wined3d_buffer_validate_location(struct wined3d_buffer *buffer, uint32_t location)
{
    TRACE("buffer %p, location %s.\n", buffer, wined3d_debug_location(location));

    if (location & WINED3D_LOCATION_BUFFER)
        buffer_clear_dirty_areas(buffer);

    buffer->locations |= location;

    TRACE("New locations flags are %s.\n", wined3d_debug_location(buffer->locations));
}

static void wined3d_buffer_invalidate_range(struct wined3d_buffer *buffer, DWORD location,
        unsigned int offset, unsigned int size)
{
    TRACE("buffer %p, location %s, offset %u, size %u.\n",
            buffer, wined3d_debug_location(location), offset, size);

    if ((location & WINED3D_LOCATION_BUFFER) && (buffer->flags & WINED3D_BUFFER_USE_BO))
        buffer_invalidate_bo_range(buffer, offset, size);

    buffer->locations &= ~location;

    TRACE("New locations flags are %s.\n", wined3d_debug_location(buffer->locations));

    if (!buffer->locations)
        ERR("Buffer %p does not have any up to date location.\n", buffer);
}

void wined3d_buffer_invalidate_location(struct wined3d_buffer *buffer, uint32_t location)
{
    wined3d_buffer_invalidate_range(buffer, location, 0, 0);
}

GLenum wined3d_buffer_gl_binding_from_bind_flags(const struct wined3d_gl_info *gl_info, uint32_t bind_flags)
{
    if (!bind_flags)
        return GL_PIXEL_UNPACK_BUFFER;

    /* We must always return GL_ELEMENT_ARRAY_BUFFER here;
     * wined3d_device_gl_create_bo() checks the GL binding to see whether we
     * can suballocate, and we cannot suballocate if this BO might be used for
     * an index buffer. */
    if (bind_flags & WINED3D_BIND_INDEX_BUFFER)
        return GL_ELEMENT_ARRAY_BUFFER;

    if (bind_flags & (WINED3D_BIND_SHADER_RESOURCE | WINED3D_BIND_UNORDERED_ACCESS)
            && gl_info->supported[ARB_TEXTURE_BUFFER_OBJECT])
        return GL_TEXTURE_BUFFER;

    if (bind_flags & WINED3D_BIND_CONSTANT_BUFFER)
        return GL_UNIFORM_BUFFER;

    if (bind_flags & WINED3D_BIND_STREAM_OUTPUT)
        return GL_TRANSFORM_FEEDBACK_BUFFER;

    if (bind_flags & WINED3D_BIND_INDIRECT_BUFFER
            && gl_info->supported[ARB_DRAW_INDIRECT])
        return GL_DRAW_INDIRECT_BUFFER;

    if (bind_flags & ~(WINED3D_BIND_VERTEX_BUFFER | WINED3D_BIND_INDEX_BUFFER))
        FIXME("Unhandled bind flags %#x.\n", bind_flags);

    return GL_ARRAY_BUFFER;
}

/* Context activation is done by the caller. */
static void wined3d_buffer_gl_destroy_buffer_object(struct wined3d_buffer_gl *buffer_gl,
        struct wined3d_context_gl *context_gl)
{
    struct wined3d_resource *resource = &buffer_gl->b.resource;
    struct wined3d_bo_gl *bo_gl;

    if (!buffer_gl->b.buffer_object)
        return;
    bo_gl = wined3d_bo_gl(buffer_gl->b.buffer_object);

    if (context_gl->c.transform_feedback_active && (resource->bind_flags & WINED3D_BIND_STREAM_OUTPUT)
            && wined3d_context_is_graphics_state_dirty(&context_gl->c, STATE_STREAM_OUTPUT))
    {
        /* It's illegal to (un)bind GL_TRANSFORM_FEEDBACK_BUFFER while transform
         * feedback is active. Deleting a buffer implicitly unbinds it, so we
         * need to end transform feedback here if this buffer was bound.
         *
         * This should only be possible if STATE_STREAM_OUTPUT is dirty; if we
         * do a draw call before destroying this buffer then the draw call will
         * already rebind the GL target. */
        WARN("Deleting buffer object for buffer %p, disabling transform feedback.\n", buffer_gl);
        wined3d_context_gl_end_transform_feedback(context_gl);
    }

    if (buffer_gl->b.bo_user.valid)
    {
        buffer_gl->b.bo_user.valid = false;
        list_remove(&buffer_gl->b.bo_user.entry);
    }
    if (!--bo_gl->b.refcount)
    {
        wined3d_context_gl_destroy_bo(context_gl, bo_gl);
        free(bo_gl);
    }
    buffer_gl->b.buffer_object = NULL;
}

/* Context activation is done by the caller. */
static BOOL wined3d_buffer_gl_create_buffer_object(struct wined3d_buffer_gl *buffer_gl,
        struct wined3d_context_gl *context_gl)
{
    struct wined3d_device_gl *device_gl = wined3d_device_gl(buffer_gl->b.resource.device);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    GLenum usage = GL_STATIC_DRAW;
    GLbitfield gl_storage_flags;
    struct wined3d_bo_gl *bo;
    bool coherent = true;
    GLsizeiptr size;
    GLenum binding;

    TRACE("Creating an OpenGL buffer object for wined3d buffer %p with usage %s.\n",
            buffer_gl, debug_d3dusage(buffer_gl->b.resource.usage));

    if (!(bo = malloc(sizeof(*bo))))
        return FALSE;

    size = buffer_gl->b.resource.size;
    binding = wined3d_buffer_gl_binding_from_bind_flags(gl_info, buffer_gl->b.resource.bind_flags);
    if (buffer_gl->b.resource.usage & WINED3DUSAGE_DYNAMIC)
    {
        usage = GL_STREAM_DRAW_ARB;
        coherent = false;
    }
    gl_storage_flags = wined3d_resource_gl_storage_flags(&buffer_gl->b.resource);
    if (!wined3d_device_gl_create_bo(device_gl, context_gl, size, binding, usage, coherent, gl_storage_flags, bo))
    {
        ERR("Failed to create OpenGL buffer object.\n");
        buffer_gl->b.flags &= ~WINED3D_BUFFER_USE_BO;
        buffer_clear_dirty_areas(&buffer_gl->b);
        free(bo);
        return FALSE;
    }

    buffer_gl->b.buffer_object = &bo->b;
    buffer_invalidate_bo_range(&buffer_gl->b, 0, 0);

    return TRUE;
}

ULONG CDECL wined3d_buffer_incref(struct wined3d_buffer *buffer)
{
    unsigned int refcount = InterlockedIncrement(&buffer->resource.ref);

    TRACE("%p increasing refcount to %u.\n", buffer, refcount);

    return refcount;
}

BOOL wined3d_buffer_prepare_location(struct wined3d_buffer *buffer,
        struct wined3d_context *context, unsigned int location)
{
    return buffer->buffer_ops->buffer_prepare_location(buffer, context, location);
}

static void wined3d_buffer_unload_location(struct wined3d_buffer *buffer,
        struct wined3d_context *context, unsigned int location)
{
    buffer->buffer_ops->buffer_unload_location(buffer, context, location);
}

BOOL wined3d_buffer_load_location(struct wined3d_buffer *buffer,
        struct wined3d_context *context, uint32_t location)
{
    struct wined3d_bo_address src, dst;
    struct wined3d_range range;

    TRACE("buffer %p, context %p, location %s.\n",
            buffer, context, wined3d_debug_location(location));

    if (buffer->locations & location)
    {
        TRACE("Location (%#x) is already up to date.\n", location);
        return TRUE;
    }

    if (!buffer->locations)
    {
        ERR("Buffer %p does not have any up to date location.\n", buffer);
        wined3d_buffer_validate_location(buffer, WINED3D_LOCATION_DISCARDED);
        return wined3d_buffer_load_location(buffer, context, location);
    }

    TRACE("Current buffer location %s.\n", wined3d_debug_location(buffer->locations));

    if (!wined3d_buffer_prepare_location(buffer, context, location))
        return FALSE;

    if (buffer->locations & WINED3D_LOCATION_DISCARDED)
    {
        TRACE("Buffer previously discarded, nothing to do.\n");
        wined3d_buffer_validate_location(buffer, location);
        wined3d_buffer_invalidate_location(buffer, WINED3D_LOCATION_DISCARDED);
        return TRUE;
    }

    switch (location)
    {
        case WINED3D_LOCATION_SYSMEM:
            if (buffer->locations & WINED3D_LOCATION_CLEARED)
            {
                memset(buffer->resource.heap_memory, 0, buffer->resource.size);
            }
            else
            {
                dst.buffer_object = NULL;
                dst.addr = buffer->resource.heap_memory;
                src.buffer_object = buffer->buffer_object;
                src.addr = NULL;
                range.offset = 0;
                range.size = buffer->resource.size;
                wined3d_context_copy_bo_address(context, &dst, &src, 1, &range, WINED3D_MAP_WRITE);
            }
            break;

        case WINED3D_LOCATION_BUFFER:
        {
            uint32_t map_flags = WINED3D_MAP_WRITE;

            if (buffer->locations & WINED3D_LOCATION_CLEARED)
            {
                /* FIXME: Clear the buffer on the GPU if possible. */
                if (!wined3d_buffer_prepare_location(buffer, context, WINED3D_LOCATION_SYSMEM))
                    return FALSE;
                memset(buffer->resource.heap_memory, 0, buffer->resource.size);
            }

            dst.buffer_object = buffer->buffer_object;
            dst.addr = NULL;
            src.buffer_object = NULL;
            src.addr = buffer->resource.heap_memory;

            if (buffer_is_fully_dirty(buffer))
                map_flags |= WINED3D_MAP_DISCARD;

            wined3d_context_copy_bo_address(context, &dst, &src,
                    buffer->dirty_range_count, buffer->dirty_ranges, map_flags);
            break;
        }

        default:
            ERR("Invalid location %s.\n", wined3d_debug_location(location));
            return FALSE;
    }

    wined3d_buffer_validate_location(buffer, location);
    if (buffer->resource.heap_memory && location == WINED3D_LOCATION_BUFFER
            && !(buffer->resource.usage & WINED3DUSAGE_DYNAMIC))
        wined3d_buffer_evict_sysmem(buffer);

    return TRUE;
}

/* Context activation is done by the caller. */
void *wined3d_buffer_load_sysmem(struct wined3d_buffer *buffer, struct wined3d_context *context)
{
    if (wined3d_buffer_load_location(buffer, context, WINED3D_LOCATION_SYSMEM))
        buffer->resource.pin_sysmem = 1;
    return buffer->resource.heap_memory;
}

DWORD wined3d_buffer_get_memory(struct wined3d_buffer *buffer, struct wined3d_context *context,
        struct wined3d_bo_address *data)
{
    unsigned int locations = buffer->locations;

    TRACE("buffer %p, context %p, data %p, locations %s.\n",
            buffer, context, data, wined3d_debug_location(locations));

    if (locations & (WINED3D_LOCATION_DISCARDED | WINED3D_LOCATION_CLEARED))
    {
        locations = ((buffer->flags & WINED3D_BUFFER_USE_BO) ? WINED3D_LOCATION_BUFFER : WINED3D_LOCATION_SYSMEM);
        if (!wined3d_buffer_load_location(buffer, context, locations))
        {
            data->buffer_object = 0;
            data->addr = NULL;
            return 0;
        }
    }
    if (locations & WINED3D_LOCATION_BUFFER)
    {
        data->buffer_object = buffer->buffer_object;
        data->addr = NULL;
        return WINED3D_LOCATION_BUFFER;
    }
    if (locations & WINED3D_LOCATION_SYSMEM)
    {
        data->buffer_object = 0;
        data->addr = buffer->resource.heap_memory;
        return WINED3D_LOCATION_SYSMEM;
    }

    ERR("Unexpected locations %s.\n", wined3d_debug_location(locations));
    data->buffer_object = 0;
    data->addr = NULL;
    return 0;
}

static void buffer_resource_unload(struct wined3d_resource *resource)
{
    struct wined3d_buffer *buffer = buffer_from_resource(resource);

    TRACE("buffer %p.\n", buffer);

    if (buffer->buffer_object)
    {
        struct wined3d_context *context;

        context = context_acquire(resource->device, NULL, 0);

        wined3d_buffer_load_location(buffer, context, WINED3D_LOCATION_SYSMEM);
        wined3d_buffer_invalidate_location(buffer, WINED3D_LOCATION_BUFFER);
        wined3d_buffer_unload_location(buffer, context, WINED3D_LOCATION_BUFFER);
        buffer_clear_dirty_areas(buffer);

        context_release(context);

        buffer->flags &= ~WINED3D_BUFFER_HASDESC;
    }

    resource_unload(resource);
}

static void wined3d_buffer_drop_bo(struct wined3d_buffer *buffer)
{
    buffer->flags &= ~WINED3D_BUFFER_USE_BO;
    buffer_resource_unload(&buffer->resource);
}

static void wined3d_buffer_destroy_object(void *object)
{
    struct wined3d_buffer *buffer = object;
    struct wined3d_context *context;

    TRACE("buffer %p.\n", buffer);

    if (buffer->buffer_object)
    {
        context = context_acquire(buffer->resource.device, NULL, 0);
        wined3d_buffer_unload_location(buffer, context, WINED3D_LOCATION_BUFFER);
        context_release(context);
    }
    free(buffer->dirty_ranges);
}

void wined3d_buffer_cleanup(struct wined3d_buffer *buffer)
{
    wined3d_cs_destroy_object(buffer->resource.device->cs, wined3d_buffer_destroy_object, buffer);
    resource_cleanup(&buffer->resource);
}

ULONG CDECL wined3d_buffer_decref(struct wined3d_buffer *buffer)
{
    unsigned int refcount = InterlockedDecrement(&buffer->resource.ref);

    TRACE("%p decreasing refcount to %u.\n", buffer, refcount);

    if (!refcount)
    {
        wined3d_mutex_lock();
        buffer->resource.parent_ops->wined3d_object_destroyed(buffer->resource.parent);
        buffer->resource.device->adapter->adapter_ops->adapter_destroy_buffer(buffer);
        wined3d_mutex_unlock();
    }

    return refcount;
}

void * CDECL wined3d_buffer_get_parent(const struct wined3d_buffer *buffer)
{
    TRACE("buffer %p.\n", buffer);

    return buffer->resource.parent;
}

/* Context activation is done by the caller. */
void wined3d_buffer_load(struct wined3d_buffer *buffer, struct wined3d_context *context,
        const struct wined3d_state *state)
{
    TRACE("buffer %p.\n", buffer);

    if (buffer->resource.map_count && buffer->map_ptr)
    {
        FIXME("Buffer is mapped through buffer object, not loading.\n");
        return;
    }
    else if (buffer->resource.map_count)
    {
        WARN("Loading mapped buffer.\n");
    }

    if (!(buffer->flags & WINED3D_BUFFER_USE_BO))
        return;

    if (!wined3d_buffer_prepare_location(buffer, context, WINED3D_LOCATION_BUFFER))
    {
        ERR("Failed to prepare buffer location.\n");
        return;
    }

    /* Reading the declaration makes only sense if we have valid state information
     * (i.e., if this function is called during draws). */
    if (state)
        buffer->flags |= WINED3D_BUFFER_HASDESC;

    if (!(buffer->flags & WINED3D_BUFFER_HASDESC && buffer_is_dirty(buffer)))
        return;

    if (!wined3d_buffer_load_location(buffer, context, WINED3D_LOCATION_BUFFER))
        ERR("Failed to load buffer location.\n");
}

struct wined3d_resource * CDECL wined3d_buffer_get_resource(struct wined3d_buffer *buffer)
{
    TRACE("buffer %p.\n", buffer);

    return &buffer->resource;
}

static HRESULT buffer_resource_sub_resource_get_desc(struct wined3d_resource *resource,
        unsigned int sub_resource_idx, struct wined3d_sub_resource_desc *desc)
{
    if (sub_resource_idx)
    {
        WARN("Invalid sub_resource_idx %u.\n", sub_resource_idx);
        return E_INVALIDARG;
    }

    desc->format = WINED3DFMT_R8_UNORM;
    desc->multisample_type = WINED3D_MULTISAMPLE_NONE;
    desc->multisample_quality = 0;
    desc->usage = resource->usage;
    desc->bind_flags = resource->bind_flags;
    desc->access = resource->access;
    desc->width = resource->size;
    desc->height = 1;
    desc->depth = 1;
    desc->size = resource->size;
    return S_OK;
}

static void buffer_resource_sub_resource_get_map_pitch(struct wined3d_resource *resource,
        unsigned int sub_resource_idx, unsigned int *row_pitch, unsigned int *slice_pitch)
{
    *row_pitch = *slice_pitch = resource->size;
}

static HRESULT buffer_resource_sub_resource_map(struct wined3d_resource *resource, unsigned int sub_resource_idx,
        void **map_ptr, const struct wined3d_box *box, uint32_t flags)
{
    struct wined3d_buffer *buffer = buffer_from_resource(resource);
    unsigned int offset, size, dirty_offset, dirty_size;
    struct wined3d_device *device = resource->device;
    struct wined3d_context *context;
    struct wined3d_bo_address addr;
    uint8_t *base;
    LONG count;

    TRACE("resource %p, sub_resource_idx %u, map_ptr %p, box %s, flags %#x.\n",
            resource, sub_resource_idx, map_ptr, debug_box(box), flags);

    dirty_offset = offset = box->left;
    dirty_size = size = box->right - box->left;

    count = ++resource->map_count;

    /* DISCARD invalidates the entire buffer, regardless of the specified
     * offset and size. Some applications also depend on the entire buffer
     * being uploaded in that case. Two such applications are Port Royale
     * and Darkstar One. */
    if (flags & WINED3D_MAP_DISCARD)
    {
        dirty_offset = 0;
        dirty_size = 0;
    }

    if (((flags & WINED3D_MAP_WRITE) && !(flags & (WINED3D_MAP_NOOVERWRITE | WINED3D_MAP_DISCARD)))
            || (!(flags & WINED3D_MAP_WRITE) && (buffer->locations & WINED3D_LOCATION_SYSMEM))
            || buffer->resource.pin_sysmem
            || !(buffer->flags & WINED3D_BUFFER_USE_BO))
    {
        if (!(buffer->locations & WINED3D_LOCATION_SYSMEM))
        {
            context = context_acquire(device, NULL, 0);
            wined3d_buffer_load_location(buffer, context, WINED3D_LOCATION_SYSMEM);
            context_release(context);
        }

        if (flags & WINED3D_MAP_WRITE)
            wined3d_buffer_invalidate_range(buffer, ~WINED3D_LOCATION_SYSMEM, dirty_offset, dirty_size);
    }
    else
    {
        context = context_acquire(device, NULL, 0);

        if (flags & WINED3D_MAP_DISCARD)
        {
            if (!wined3d_buffer_prepare_location(buffer, context, WINED3D_LOCATION_BUFFER))
            {
                context_release(context);
                return E_OUTOFMEMORY;
            }
            wined3d_buffer_validate_location(buffer, WINED3D_LOCATION_BUFFER);
        }
        else
        {
            wined3d_buffer_load_location(buffer, context, WINED3D_LOCATION_BUFFER);
        }

        if (flags & WINED3D_MAP_WRITE)
        {
            wined3d_buffer_acquire_bo_for_write(buffer, context);
            wined3d_buffer_invalidate_location(buffer, ~WINED3D_LOCATION_BUFFER);
            buffer_invalidate_bo_range(buffer, dirty_offset, dirty_size);
        }

        if ((flags & WINED3D_MAP_DISCARD) && resource->heap_memory)
            wined3d_buffer_evict_sysmem(buffer);

        if (count == 1)
        {
            addr.buffer_object = buffer->buffer_object;
            addr.addr = 0;
            buffer->map_ptr = wined3d_context_map_bo_address(context, &addr, resource->size, flags);

            /* We are accessing buffer->resource.client from the CS thread,
             * but it's safe because the client thread will wait for the
             * map to return, thus completely serializing this call with
             * other client code. */
            if (context->d3d_info->persistent_map)
                buffer->resource.client.addr = addr;

            if (((DWORD_PTR)buffer->map_ptr) & (RESOURCE_ALIGNMENT - 1))
            {
                WARN("Pointer %p is not %u byte aligned.\n", buffer->map_ptr, RESOURCE_ALIGNMENT);

                wined3d_context_unmap_bo_address(context, &addr, 0, NULL);
                buffer->map_ptr = NULL;

                if (resource->usage & WINED3DUSAGE_DYNAMIC)
                {
                    /* The extra copy is more expensive than not using VBOs
                     * at all on the NVIDIA Linux driver, which is the
                     * only driver that returns unaligned pointers. */
                    TRACE("Dynamic buffer, dropping VBO.\n");
                    wined3d_buffer_drop_bo(buffer);
                }
                else
                {
                    TRACE("Falling back to doublebuffered operation.\n");
                    wined3d_buffer_load_location(buffer, context, WINED3D_LOCATION_SYSMEM);
                    buffer->resource.pin_sysmem = 1;
                }
                TRACE("New pointer is %p.\n", resource->heap_memory);
            }
        }

        context_release(context);
    }

    base = buffer->map_ptr ? buffer->map_ptr : resource->heap_memory;
    *map_ptr = base + offset;

    TRACE("Returning memory at %p (base %p, offset %u).\n", *map_ptr, base, offset);

    return WINED3D_OK;
}

static HRESULT buffer_resource_sub_resource_unmap(struct wined3d_resource *resource, unsigned int sub_resource_idx)
{
    struct wined3d_buffer *buffer = buffer_from_resource(resource);
    struct wined3d_device *device = resource->device;
    struct wined3d_context *context;
    struct wined3d_bo_address addr;

    TRACE("resource %p, sub_resource_idx %u.\n", resource, sub_resource_idx);

    if (sub_resource_idx)
    {
        WARN("Invalid sub_resource_idx %u.\n", sub_resource_idx);
        return E_INVALIDARG;
    }

    if (!resource->map_count)
    {
        WARN("Unmap called without a previous map call.\n");
        return WINED3D_OK;
    }

    if (--resource->map_count)
    {
        /* Delay loading the buffer until everything is unmapped. */
        TRACE("Ignoring unmap.\n");
        return WINED3D_OK;
    }

    if (!buffer->map_ptr)
        return WINED3D_OK;

    context = context_acquire(device, NULL, 0);

    addr.buffer_object = buffer->buffer_object;
    addr.addr = 0;
    wined3d_context_unmap_bo_address(context, &addr, buffer->dirty_range_count, buffer->dirty_ranges);

    context_release(context);

    buffer_clear_dirty_areas(buffer);
    buffer->map_ptr = NULL;

    return WINED3D_OK;
}

static void wined3d_buffer_set_bo(struct wined3d_buffer *buffer, struct wined3d_context *context, struct wined3d_bo *bo)
{
    struct wined3d_bo *prev_bo = buffer->buffer_object;

    TRACE("buffer %p, context %p, bo %p.\n", buffer, context, bo);

    if (prev_bo)
    {
        struct wined3d_bo_user *bo_user;

        /* The previous BO might have users in other buffers which were valid,
         * and should in theory remain valid. The problem is that it's not easy
         * to tell which users belong to this buffer and which don't. We could
         * add a field, but for now it's easier and probably fine to just
         * invalidate every user. */
        LIST_FOR_EACH_ENTRY(bo_user, &prev_bo->users, struct wined3d_bo_user, entry)
            bo_user->valid = false;
        list_init(&prev_bo->users);

        if (!--prev_bo->refcount)
        {
            wined3d_context_destroy_bo(context, prev_bo);
            free(prev_bo);
        }
    }

    buffer->buffer_object = bo;
}

void wined3d_buffer_acquire_bo_for_write(struct wined3d_buffer *buffer, struct wined3d_context *context)
{
    const struct wined3d_range range = {.size = buffer->resource.size};
    struct wined3d_bo_address dst, src;
    struct wined3d_bo *bo;

    if (!(bo = buffer->buffer_object))
        return;

    /* If we are the only owner of this BO, there is nothing to do. */
    if (bo->refcount == 1)
        return;

    TRACE("Performing copy-on-write for BO %p.\n", bo);

    /* Grab a reference to the current BO. It's okay if this overflows, because
     * the following unload will release it. */
    ++bo->refcount;

    /* Unload and re-prepare to get a new buffer. This is a bit cheap and not
     * perfectly idiomatic—we should really just factor out an adapter-agnostic
     * function to create a BO and then use wined3d_buffer_set_bo()—but it'll
     * do nonetheless. */
    wined3d_buffer_unload_location(buffer, context, WINED3D_LOCATION_BUFFER);
    wined3d_buffer_prepare_location(buffer, context, WINED3D_LOCATION_BUFFER);

    /* And finally, perform the actual copy. */
    assert(buffer->buffer_object != bo);
    dst.buffer_object = buffer->buffer_object;
    dst.addr = NULL;
    src.buffer_object = bo;
    src.addr = NULL;
    wined3d_context_copy_bo_address(context, &dst, &src, 1, &range, WINED3D_MAP_WRITE | WINED3D_MAP_DISCARD);
}

void wined3d_buffer_copy_bo_address(struct wined3d_buffer *dst_buffer, struct wined3d_context *context,
        unsigned int dst_offset, const struct wined3d_const_bo_address *src_addr, unsigned int size)
{
    uint32_t map_flags = WINED3D_MAP_WRITE;
    struct wined3d_bo_address dst_addr;
    struct wined3d_range range;
    DWORD dst_location;

    if (!dst_offset && size == dst_buffer->resource.size)
        map_flags |= WINED3D_MAP_DISCARD;

    if (map_flags & WINED3D_MAP_DISCARD)
        wined3d_buffer_acquire_bo_for_write(dst_buffer, context);

    dst_location = wined3d_buffer_get_memory(dst_buffer, context, &dst_addr);
    dst_addr.addr += dst_offset;

    range.offset = 0;
    range.size = size;
    wined3d_context_copy_bo_address(context, &dst_addr, (const struct wined3d_bo_address *)src_addr, 1, &range, map_flags);
    wined3d_buffer_invalidate_range(dst_buffer, ~dst_location, dst_offset, size);
}

void wined3d_buffer_copy(struct wined3d_buffer *dst_buffer, unsigned int dst_offset,
        struct wined3d_buffer *src_buffer, unsigned int src_offset, unsigned int size)
{
    struct wined3d_context *context;
    struct wined3d_bo_address src;

    TRACE("dst_buffer %p, dst_offset %u, src_buffer %p, src_offset %u, size %u.\n",
            dst_buffer, dst_offset, src_buffer, src_offset, size);

    context = context_acquire(dst_buffer->resource.device, NULL, 0);

    wined3d_buffer_get_memory(src_buffer, context, &src);
    src.addr += src_offset;

    wined3d_buffer_copy_bo_address(dst_buffer, context, dst_offset, wined3d_const_bo_address(&src), size);

    context_release(context);
}

void wined3d_buffer_update_sub_resource(struct wined3d_buffer *buffer, struct wined3d_context *context,
        const struct upload_bo *upload_bo, unsigned int offset, unsigned int size)
{
    struct wined3d_bo *bo = upload_bo->addr.buffer_object;
    uint32_t flags = upload_bo->flags;

    /* Try to take this buffer for COW. Don't take it if we've saturated the
     * refcount. */
    if (!offset && size == buffer->resource.size
            && bo && bo->refcount < UINT8_MAX && !(upload_bo->flags & UPLOAD_BO_RENAME_ON_UNMAP))
    {
        flags |= UPLOAD_BO_RENAME_ON_UNMAP;
        ++bo->refcount;
    }

    if (flags & UPLOAD_BO_RENAME_ON_UNMAP)
    {
        /* Don't increment the refcount. UPLOAD_BO_RENAME_ON_UNMAP transfers an
         * existing reference.
         *
         * FIXME: We could degenerate RENAME to a copy + free and rely on the
         * COW logic to detect this case.
         */
        wined3d_buffer_set_bo(buffer, context, upload_bo->addr.buffer_object);
        wined3d_buffer_validate_location(buffer, WINED3D_LOCATION_BUFFER);
        wined3d_buffer_invalidate_location(buffer, ~WINED3D_LOCATION_BUFFER);
    }

    if (upload_bo->addr.buffer_object && upload_bo->addr.buffer_object == buffer->buffer_object)
    {
        struct wined3d_range range;

        /* We need to flush changes, which is implicitly done by
         * wined3d_context_unmap_bo_address() even if we aren't actually going
         * to unmap.
         *
         * We would also like to free up virtual address space used by this BO
         * if it's at a premium—note that this BO was allocated for an
         * accelerated map. Hence we unmap the BO instead of merely flushing it;
         * if we don't care about unmapping BOs then
         * wined3d_context_unmap_bo_address() will flush and return.
         */
        range.offset = offset;
        range.size = size;
        if (upload_bo->addr.buffer_object->map_ptr)
            wined3d_context_unmap_bo_address(context, (const struct wined3d_bo_address *)&upload_bo->addr, 1, &range);
    }
    else
    {
        wined3d_buffer_copy_bo_address(buffer, context, offset, &upload_bo->addr, size);
    }
}

static void wined3d_buffer_init_data(struct wined3d_buffer *buffer,
        struct wined3d_device *device, const struct wined3d_sub_resource_data *data)
{
    struct wined3d_resource *resource = &buffer->resource;
    struct wined3d_box box;

    if (buffer->flags & WINED3D_BUFFER_USE_BO)
    {
        wined3d_box_set(&box, 0, 0, resource->size, 1, 0, 1);
        wined3d_device_context_emit_update_sub_resource(&device->cs->c, resource,
                0, &box, data->data, data->row_pitch, data->slice_pitch);
    }
    else
    {
        memcpy(buffer->resource.heap_memory, data->data, resource->size);
        wined3d_buffer_validate_location(buffer, WINED3D_LOCATION_SYSMEM);
        wined3d_buffer_invalidate_location(buffer, ~WINED3D_LOCATION_SYSMEM);
    }
}

static ULONG buffer_resource_incref(struct wined3d_resource *resource)
{
    return wined3d_buffer_incref(buffer_from_resource(resource));
}

static ULONG buffer_resource_decref(struct wined3d_resource *resource)
{
    return wined3d_buffer_decref(buffer_from_resource(resource));
}

static void buffer_resource_preload(struct wined3d_resource *resource)
{
    struct wined3d_context *context;

    context = context_acquire(resource->device, NULL, 0);
    wined3d_buffer_load(buffer_from_resource(resource), context, NULL);
    context_release(context);
}

static const struct wined3d_resource_ops buffer_resource_ops =
{
    buffer_resource_incref,
    buffer_resource_decref,
    buffer_resource_preload,
    buffer_resource_unload,
    buffer_resource_sub_resource_get_desc,
    buffer_resource_sub_resource_get_map_pitch,
    buffer_resource_sub_resource_map,
    buffer_resource_sub_resource_unmap,
};

static HRESULT wined3d_buffer_init(struct wined3d_buffer *buffer, struct wined3d_device *device,
        const struct wined3d_buffer_desc *desc, const struct wined3d_sub_resource_data *data,
        void *parent, const struct wined3d_parent_ops *parent_ops, const struct wined3d_buffer_ops *buffer_ops)
{
    const struct wined3d_format *format = wined3d_get_format(device->adapter, WINED3DFMT_R8_UNORM, desc->bind_flags);
    struct wined3d_resource *resource = &buffer->resource;
    unsigned int access;
    HRESULT hr;

    TRACE("buffer %p, device %p, desc byte_width %u, usage %s, bind_flags %s, "
            "access %s, data %p, parent %p, parent_ops %p.\n",
            buffer, device, desc->byte_width, debug_d3dusage(desc->usage), wined3d_debug_bind_flags(desc->bind_flags),
            wined3d_debug_resource_access(desc->access), data, parent, parent_ops);

    if (!desc->byte_width)
    {
        WARN("Size 0 requested, returning E_INVALIDARG.\n");
        return E_INVALIDARG;
    }

    if (desc->bind_flags & WINED3D_BIND_CONSTANT_BUFFER && desc->byte_width & (WINED3D_CONSTANT_BUFFER_ALIGNMENT - 1))
    {
        WARN("Size %#x is not suitably aligned for constant buffers.\n", desc->byte_width);
        return E_INVALIDARG;
    }

    if (data && !data->data)
    {
        WARN("Invalid sub-resource data specified.\n");
        return E_INVALIDARG;
    }

    access = desc->access;
    if (desc->bind_flags & WINED3D_BIND_CONSTANT_BUFFER && wined3d_settings.cb_access_map_w)
        access |= WINED3D_RESOURCE_ACCESS_MAP_W;

    if (FAILED(hr = resource_init(resource, device, WINED3D_RTYPE_BUFFER, format,
            WINED3D_MULTISAMPLE_NONE, 0, desc->usage, desc->bind_flags, access,
            desc->byte_width, 1, 1, desc->byte_width, parent, parent_ops, &buffer_resource_ops)))
    {
        WARN("Failed to initialize resource, hr %#lx.\n", hr);
        return hr;
    }
    buffer->buffer_ops = buffer_ops;
    buffer->structure_byte_stride = desc->structure_byte_stride;
    buffer->locations = WINED3D_LOCATION_CLEARED;

    TRACE("buffer %p, size %#x, usage %#x, memory @ %p.\n",
            buffer, buffer->resource.size, buffer->resource.usage, buffer->resource.heap_memory);

    if (device->create_parms.flags & WINED3DCREATE_SOFTWARE_VERTEXPROCESSING
            || (desc->usage & WINED3DUSAGE_MANAGED))
    {
        /* SWvp and managed buffers always return the same pointer in buffer
         * maps and retain data in DISCARD maps. Keep a system memory copy of
         * the buffer to provide the same behavior to the application. */
        TRACE("Pinning system memory.\n");
        buffer->resource.pin_sysmem = 1;
        buffer->locations = WINED3D_LOCATION_SYSMEM;
    }

    if (buffer->locations & WINED3D_LOCATION_SYSMEM || !(buffer->flags & WINED3D_BUFFER_USE_BO))
    {
        if (!wined3d_resource_prepare_sysmem(&buffer->resource))
            return E_OUTOFMEMORY;
    }

    if ((buffer->flags & WINED3D_BUFFER_USE_BO) && !wined3d_array_reserve((void **)&buffer->dirty_ranges,
            &buffer->dirty_ranges_capacity, 1, sizeof(*buffer->dirty_ranges)))
    {
        ERR("Out of memory.\n");
        buffer_resource_unload(resource);
        resource_cleanup(resource);
        wined3d_resource_wait_idle(resource);
        return E_OUTOFMEMORY;
    }

    if (buffer->locations & WINED3D_LOCATION_DISCARDED)
        buffer->resource.client.addr.buffer_object = CLIENT_BO_DISCARDED;

    if (data)
        wined3d_buffer_init_data(buffer, device, data);

    return WINED3D_OK;
}

static BOOL wined3d_buffer_no3d_prepare_location(struct wined3d_buffer *buffer,
        struct wined3d_context *context, unsigned int location)
{
    if (location == WINED3D_LOCATION_SYSMEM)
        return wined3d_resource_prepare_sysmem(&buffer->resource);

    FIXME("Unhandled location %s.\n", wined3d_debug_location(location));

    return FALSE;
}

static void wined3d_buffer_no3d_unload_location(struct wined3d_buffer *buffer,
        struct wined3d_context *context, unsigned int location)
{
    TRACE("buffer %p, context %p, location %s.\n", buffer, context, wined3d_debug_location(location));
}

static const struct wined3d_buffer_ops wined3d_buffer_no3d_ops =
{
    wined3d_buffer_no3d_prepare_location,
    wined3d_buffer_no3d_unload_location,
};

HRESULT wined3d_buffer_no3d_init(struct wined3d_buffer *buffer_no3d, struct wined3d_device *device,
        const struct wined3d_buffer_desc *desc, const struct wined3d_sub_resource_data *data,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    TRACE("buffer_no3d %p, device %p, desc %p, data %p, parent %p, parent_ops %p.\n",
            buffer_no3d, device, desc, data, parent, parent_ops);

    return wined3d_buffer_init(buffer_no3d, device, desc, data, parent, parent_ops, &wined3d_buffer_no3d_ops);
}

static BOOL wined3d_buffer_gl_prepare_location(struct wined3d_buffer *buffer,
        struct wined3d_context *context, unsigned int location)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    struct wined3d_buffer_gl *buffer_gl = wined3d_buffer_gl(buffer);

    switch (location)
    {
        case WINED3D_LOCATION_SYSMEM:
            return wined3d_resource_prepare_sysmem(&buffer->resource);

        case WINED3D_LOCATION_BUFFER:
            if (buffer->buffer_object)
                return TRUE;

            if (!(buffer->flags & WINED3D_BUFFER_USE_BO))
            {
                WARN("Trying to create BO for buffer %p with no WINED3D_BUFFER_USE_BO.\n", buffer);
                return FALSE;
            }
            return wined3d_buffer_gl_create_buffer_object(buffer_gl, context_gl);

        default:
            ERR("Invalid location %s.\n", wined3d_debug_location(location));
            return FALSE;
    }
}

static void wined3d_buffer_gl_unload_location(struct wined3d_buffer *buffer,
        struct wined3d_context *context, unsigned int location)
{
    TRACE("buffer %p, context %p, location %s.\n", buffer, context, wined3d_debug_location(location));

    switch (location)
    {
        case WINED3D_LOCATION_BUFFER:
            wined3d_buffer_gl_destroy_buffer_object(wined3d_buffer_gl(buffer), wined3d_context_gl(context));
            break;

        default:
            ERR("Unhandled location %s.\n", wined3d_debug_location(location));
            break;
    }
}

static const struct wined3d_buffer_ops wined3d_buffer_gl_ops =
{
    wined3d_buffer_gl_prepare_location,
    wined3d_buffer_gl_unload_location,
};

HRESULT wined3d_buffer_gl_init(struct wined3d_buffer_gl *buffer_gl, struct wined3d_device *device,
        const struct wined3d_buffer_desc *desc, const struct wined3d_sub_resource_data *data,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_gl_info *gl_info = &wined3d_adapter_gl(device->adapter)->gl_info;

    TRACE("buffer_gl %p, device %p, desc %p, data %p, parent %p, parent_ops %p.\n",
            buffer_gl, device, desc, data, parent, parent_ops);

    /* Observations show that draw_primitive_immediate_mode() is faster on
     * dynamic vertex buffers than converting + draw_primitive_arrays().
     * (Half-Life 2 and others.) */
    if (!(desc->access & WINED3D_RESOURCE_ACCESS_GPU))
        TRACE("Not creating a BO because the buffer is not GPU accessible.\n");
    else if (!gl_info->supported[ARB_VERTEX_BUFFER_OBJECT])
        TRACE("Not creating a BO because GL_ARB_vertex_buffer is not supported.\n");
    else if (!(gl_info->supported[APPLE_FLUSH_BUFFER_RANGE] || gl_info->supported[ARB_MAP_BUFFER_RANGE])
            && (desc->usage & WINED3DUSAGE_DYNAMIC))
        TRACE("Not creating a BO because the buffer has dynamic usage and no GL support.\n");
    else
        buffer_gl->b.flags |= WINED3D_BUFFER_USE_BO;

    return wined3d_buffer_init(&buffer_gl->b, device, desc, data, parent, parent_ops, &wined3d_buffer_gl_ops);
}

VkBufferUsageFlags vk_buffer_usage_from_bind_flags(uint32_t bind_flags)
{
    VkBufferUsageFlags usage;

    usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    if (bind_flags & WINED3D_BIND_VERTEX_BUFFER)
        usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (bind_flags & WINED3D_BIND_INDEX_BUFFER)
        usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (bind_flags & WINED3D_BIND_CONSTANT_BUFFER)
        usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    if (bind_flags & WINED3D_BIND_SHADER_RESOURCE)
        usage |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
    if (bind_flags & WINED3D_BIND_STREAM_OUTPUT)
        usage |= VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT;
    if (bind_flags & WINED3D_BIND_UNORDERED_ACCESS)
        usage |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
    if (bind_flags & WINED3D_BIND_INDIRECT_BUFFER)
        usage |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    if (bind_flags & (WINED3D_BIND_RENDER_TARGET | WINED3D_BIND_DEPTH_STENCIL))
        FIXME("Ignoring some bind flags %#x.\n", bind_flags);
    return usage;
}

VkMemoryPropertyFlags vk_memory_type_from_access_flags(uint32_t access, uint32_t usage)
{
    VkMemoryPropertyFlags memory_type = 0;

    if (access & WINED3D_RESOURCE_ACCESS_MAP_R)
        memory_type |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    else if (access & WINED3D_RESOURCE_ACCESS_MAP_W)
        memory_type |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    else if (!(usage & WINED3DUSAGE_DYNAMIC))
        memory_type |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    return memory_type;
}

static BOOL wined3d_buffer_vk_create_buffer_object(struct wined3d_buffer_vk *buffer_vk,
        struct wined3d_context_vk *context_vk)
{
    struct wined3d_resource *resource = &buffer_vk->b.resource;
    struct wined3d_bo_vk *bo_vk;

    if (!(bo_vk = malloc(sizeof(*bo_vk))))
        return FALSE;

    if (!(wined3d_context_vk_create_bo(context_vk, resource->size,
            vk_buffer_usage_from_bind_flags(resource->bind_flags),
            vk_memory_type_from_access_flags(resource->access, resource->usage), bo_vk)))
    {
        WARN("Failed to create Vulkan buffer.\n");
        free(bo_vk);
        return FALSE;
    }

    buffer_vk->b.buffer_object = &bo_vk->b;
    buffer_invalidate_bo_range(&buffer_vk->b, 0, 0);

    return TRUE;
}

const VkDescriptorBufferInfo *wined3d_buffer_vk_get_buffer_info(struct wined3d_buffer_vk *buffer_vk)
{
    struct wined3d_bo_vk *bo = wined3d_bo_vk(buffer_vk->b.buffer_object);

    if (buffer_vk->b.bo_user.valid)
        return &buffer_vk->buffer_info;

    buffer_vk->buffer_info.buffer = bo->vk_buffer;
    buffer_vk->buffer_info.offset = bo->b.buffer_offset;
    buffer_vk->buffer_info.range = buffer_vk->b.resource.size;
    wined3d_buffer_validate_user(&buffer_vk->b);

    return &buffer_vk->buffer_info;
}

static BOOL wined3d_buffer_vk_prepare_location(struct wined3d_buffer *buffer,
        struct wined3d_context *context, unsigned int location)
{
    switch (location)
    {
        case WINED3D_LOCATION_SYSMEM:
            return wined3d_resource_prepare_sysmem(&buffer->resource);

        case WINED3D_LOCATION_BUFFER:
            if (buffer->buffer_object)
                return TRUE;

            return wined3d_buffer_vk_create_buffer_object(wined3d_buffer_vk(buffer), wined3d_context_vk(context));

        default:
            FIXME("Unhandled location %s.\n", wined3d_debug_location(location));
            return FALSE;
    }
}

static void wined3d_buffer_vk_unload_location(struct wined3d_buffer *buffer,
        struct wined3d_context *context, unsigned int location)
{
    struct wined3d_context_vk *context_vk = wined3d_context_vk(context);
    struct wined3d_bo_vk *bo_vk = wined3d_bo_vk(buffer->buffer_object);

    TRACE("buffer %p, context %p, location %s.\n", buffer, context, wined3d_debug_location(location));

    switch (location)
    {
        case WINED3D_LOCATION_BUFFER:
            if (buffer->bo_user.valid)
            {
                buffer->bo_user.valid = false;
                list_remove(&buffer->bo_user.entry);
            }
            if (!--bo_vk->b.refcount)
            {
                wined3d_context_vk_destroy_bo(context_vk, bo_vk);
                free(bo_vk);
            }
            buffer->buffer_object = NULL;
            break;

        default:
            ERR("Unhandled location %s.\n", wined3d_debug_location(location));
            break;
    }
}

static const struct wined3d_buffer_ops wined3d_buffer_vk_ops =
{
    wined3d_buffer_vk_prepare_location,
    wined3d_buffer_vk_unload_location,
};

HRESULT wined3d_buffer_vk_init(struct wined3d_buffer_vk *buffer_vk, struct wined3d_device *device,
        const struct wined3d_buffer_desc *desc, const struct wined3d_sub_resource_data *data,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_vk_info *vk_info = &wined3d_adapter_vk(device->adapter)->vk_info;

    TRACE("buffer_vk %p, device %p, desc %p, data %p, parent %p, parent_ops %p.\n",
            buffer_vk, device, desc, data, parent, parent_ops);

    if ((desc->bind_flags & WINED3D_BIND_STREAM_OUTPUT)
            && !vk_info->supported[WINED3D_VK_EXT_TRANSFORM_FEEDBACK])
    {
        WARN("The Vulkan implementation does not support transform feedback.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (desc->access & WINED3D_RESOURCE_ACCESS_GPU)
        buffer_vk->b.flags |= WINED3D_BUFFER_USE_BO;

    return wined3d_buffer_init(&buffer_vk->b, device, desc, data, parent, parent_ops, &wined3d_buffer_vk_ops);
}

void wined3d_buffer_vk_barrier(struct wined3d_buffer_vk *buffer_vk,
        struct wined3d_context_vk *context_vk, uint32_t bind_mask)
{
    uint32_t src_bind_mask = 0;

    TRACE("buffer_vk %p, context_vk %p, bind_mask %s.\n",
            buffer_vk, context_vk, wined3d_debug_bind_flags(bind_mask));

    if (bind_mask & ~WINED3D_READ_ONLY_BIND_MASK)
    {
        src_bind_mask = buffer_vk->bind_mask & WINED3D_READ_ONLY_BIND_MASK;
        if (!src_bind_mask)
            src_bind_mask = buffer_vk->bind_mask;

        buffer_vk->bind_mask = bind_mask;
    }
    else if ((buffer_vk->bind_mask & bind_mask) != bind_mask)
    {
        src_bind_mask = buffer_vk->bind_mask & ~WINED3D_READ_ONLY_BIND_MASK;
        buffer_vk->bind_mask |= bind_mask;
    }

    if (src_bind_mask)
    {
        const struct wined3d_bo_vk *bo = wined3d_bo_vk(buffer_vk->b.buffer_object);
        const struct wined3d_vk_info *vk_info = context_vk->vk_info;
        VkBufferMemoryBarrier vk_barrier;

        TRACE("    %s -> %s.\n",
                wined3d_debug_bind_flags(src_bind_mask), wined3d_debug_bind_flags(bind_mask));

        wined3d_context_vk_end_current_render_pass(context_vk);

        vk_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        vk_barrier.pNext = NULL;
        vk_barrier.srcAccessMask = vk_access_mask_from_bind_flags(src_bind_mask);
        vk_barrier.dstAccessMask = vk_access_mask_from_bind_flags(bind_mask);
        vk_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vk_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vk_barrier.buffer = bo->vk_buffer;
        vk_barrier.offset = bo->b.buffer_offset;
        vk_barrier.size = buffer_vk->b.resource.size;
        VK_CALL(vkCmdPipelineBarrier(wined3d_context_vk_get_command_buffer(context_vk),
                vk_pipeline_stage_mask_from_bind_flags(src_bind_mask),
                vk_pipeline_stage_mask_from_bind_flags(bind_mask),
                0, 0, NULL, 1, &vk_barrier, 0, NULL));
    }
}

HRESULT CDECL wined3d_buffer_create(struct wined3d_device *device, const struct wined3d_buffer_desc *desc,
        const struct wined3d_sub_resource_data *data, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_buffer **buffer)
{
    TRACE("device %p, desc %p, data %p, parent %p, parent_ops %p, buffer %p.\n",
            device, desc, data, parent, parent_ops, buffer);

    return device->adapter->adapter_ops->adapter_create_buffer(device, desc, data, parent, parent_ops, buffer);
}

static HRESULT wined3d_streaming_buffer_prepare(struct wined3d_device *device,
        struct wined3d_streaming_buffer *buffer, unsigned int min_size)
{
    struct wined3d_buffer *wined3d_buffer;
    struct wined3d_buffer_desc desc;
    unsigned int old_size = 0;
    unsigned int size;
    HRESULT hr;

    if (buffer->buffer)
    {
        old_size = buffer->buffer->resource.size;
        if (old_size >= min_size)
            return S_OK;
    }

    size = max(SB_MIN_SIZE, max(old_size * 2, min_size));
    TRACE("Growing buffer to %u bytes.\n", size);

    desc.byte_width = size;
    desc.usage = WINED3DUSAGE_DYNAMIC;
    desc.bind_flags = buffer->bind_flags;
    desc.access = WINED3D_RESOURCE_ACCESS_GPU | WINED3D_RESOURCE_ACCESS_MAP_W;
    desc.misc_flags = 0;
    desc.structure_byte_stride = 0;

    if (SUCCEEDED(hr = wined3d_buffer_create(device, &desc, NULL, NULL, &wined3d_null_parent_ops, &wined3d_buffer)))
    {
        if (buffer->buffer)
            wined3d_buffer_decref(buffer->buffer);
        buffer->buffer = wined3d_buffer;
        buffer->pos = 0;
    }
    return hr;
}

HRESULT CDECL wined3d_streaming_buffer_map(struct wined3d_device *device,
        struct wined3d_streaming_buffer *buffer, unsigned int size, unsigned int stride,
        unsigned int *ret_pos, void **ret_data)
{
    unsigned int map_flags = WINED3D_MAP_WRITE;
    struct wined3d_resource *resource;
    struct wined3d_map_desc map_desc;
    unsigned int pos, align;
    struct wined3d_box box;
    HRESULT hr;

    TRACE("device %p, buffer %p, size %u, stride %u, ret_pos %p, ret_data %p.\n",
            device, buffer, size, stride, ret_pos, ret_data);

    if (FAILED(hr = wined3d_streaming_buffer_prepare(device, buffer, size)))
        return hr;
    resource = &buffer->buffer->resource;

    pos = buffer->pos;
    if ((align = pos % stride))
        align = stride - align;
    if (pos + size + align > resource->size)
    {
        pos = 0;
        map_flags |= WINED3D_MAP_DISCARD;
    }
    else
    {
        pos += align;
        map_flags |= WINED3D_MAP_NOOVERWRITE;
    }

    wined3d_box_set(&box, pos, 0, pos + size, 1, 0, 1);
    if (SUCCEEDED(hr = wined3d_resource_map(resource, 0, &map_desc, &box, map_flags)))
    {
        *ret_pos = pos;
        *ret_data = map_desc.data;
        buffer->pos = pos + size;
    }
    return hr;
}

void CDECL wined3d_streaming_buffer_unmap(struct wined3d_streaming_buffer *buffer)
{
    wined3d_resource_unmap(&buffer->buffer->resource, 0);
}

HRESULT CDECL wined3d_streaming_buffer_upload(struct wined3d_device *device, struct wined3d_streaming_buffer *buffer,
        const void *data, unsigned int size, unsigned int stride, unsigned int *ret_pos)
{
    void *dst_data;
    HRESULT hr;

    if (SUCCEEDED(hr = wined3d_streaming_buffer_map(device, buffer, size, stride, ret_pos, &dst_data)))
    {
        memcpy(dst_data, data, size);
        wined3d_streaming_buffer_unmap(buffer);
    }
    return hr;
}
