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

struct private_data
{
    struct list entry;

    GUID tag;
    DWORD flags; /* DDSPD_* */

    union
    {
        void *data;
        IUnknown *object;
    } ptr;

    DWORD size;
};

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
    static const DWORD handled = WINED3DUSAGE_RENDERTARGET
            | WINED3DUSAGE_DEPTHSTENCIL
            | WINED3DUSAGE_DYNAMIC
            | WINED3DUSAGE_AUTOGENMIPMAP
            | WINED3DUSAGE_STATICDECL
            | WINED3DUSAGE_OVERLAY
            | WINED3DUSAGE_TEXTURE;

    if (usage & ~handled)
        FIXME("Unhandled usage flags %#x.\n", usage & ~handled);
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
    if (pool != WINED3D_POOL_SCRATCH)
    {
        if ((usage & WINED3DUSAGE_RENDERTARGET) && !(format->flags & WINED3DFMT_FLAG_RENDERTARGET))
            return WINED3DERR_INVALIDCALL;
        if ((usage & WINED3DUSAGE_DEPTHSTENCIL) && !(format->flags & (WINED3DFMT_FLAG_DEPTH | WINED3DFMT_FLAG_STENCIL)))
            return WINED3DERR_INVALIDCALL;
        if ((usage & WINED3DUSAGE_TEXTURE) && !(format->flags & WINED3DFMT_FLAG_TEXTURE))
            return WINED3DERR_INVALIDCALL;
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
    list_init(&resource->privateData);

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
    struct private_data *data;
    struct list *e1, *e2;
    HRESULT hr;

    TRACE("Cleaning up resource %p.\n", resource);

    if (resource->pool == WINED3D_POOL_DEFAULT && d3d->flags & WINED3D_VIDMEM_ACCOUNTING)
    {
        TRACE("Decrementing device memory pool by %u.\n", resource->size);
        adapter_adjust_memory(resource->device->adapter, 0 - resource->size);
    }

    LIST_FOR_EACH_SAFE(e1, e2, &resource->privateData)
    {
        data = LIST_ENTRY(e1, struct private_data, entry);
        hr = wined3d_resource_free_private_data(resource, &data->tag);
        if (FAILED(hr))
            ERR("Failed to free private data when destroying resource %p, hr = %#x.\n", resource, hr);
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

static struct private_data *resource_find_private_data(const struct wined3d_resource *resource, REFGUID tag)
{
    struct private_data *data;
    struct list *entry;

    TRACE("Searching for private data %s\n", debugstr_guid(tag));
    LIST_FOR_EACH(entry, &resource->privateData)
    {
        data = LIST_ENTRY(entry, struct private_data, entry);
        if (IsEqualGUID(&data->tag, tag)) {
            TRACE("Found %p\n", data);
            return data;
        }
    }
    TRACE("Not found\n");
    return NULL;
}

HRESULT CDECL wined3d_resource_set_private_data(struct wined3d_resource *resource, REFGUID guid,
        const void *data, DWORD data_size, DWORD flags)
{
    struct private_data *d;

    TRACE("resource %p, riid %s, data %p, data_size %u, flags %#x.\n",
            resource, debugstr_guid(guid), data, data_size, flags);

    wined3d_resource_free_private_data(resource, guid);

    d = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*d));
    if (!d) return E_OUTOFMEMORY;

    d->tag = *guid;
    d->flags = flags;

    if (flags & WINED3DSPD_IUNKNOWN)
    {
        if (data_size != sizeof(IUnknown *))
        {
            WARN("IUnknown data with size %u, returning WINED3DERR_INVALIDCALL.\n", data_size);
            HeapFree(GetProcessHeap(), 0, d);
            return WINED3DERR_INVALIDCALL;
        }
        d->ptr.object = (IUnknown *)data;
        d->size = sizeof(IUnknown *);
        IUnknown_AddRef(d->ptr.object);
    }
    else
    {
        d->ptr.data = HeapAlloc(GetProcessHeap(), 0, data_size);
        if (!d->ptr.data)
        {
            HeapFree(GetProcessHeap(), 0, d);
            return E_OUTOFMEMORY;
        }
        d->size = data_size;
        memcpy(d->ptr.data, data, data_size);
    }
    list_add_tail(&resource->privateData, &d->entry);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_resource_get_private_data(const struct wined3d_resource *resource, REFGUID guid,
        void *data, DWORD *data_size)
{
    const struct private_data *d;

    TRACE("resource %p, guid %s, data %p, data_size %p.\n",
            resource, debugstr_guid(guid), data, data_size);

    d = resource_find_private_data(resource, guid);
    if (!d) return WINED3DERR_NOTFOUND;

    if (*data_size < d->size)
    {
        *data_size = d->size;
        return WINED3DERR_MOREDATA;
    }

    if (d->flags & WINED3DSPD_IUNKNOWN)
    {
        *(IUnknown **)data = d->ptr.object;
        if (resource->device->wined3d->dxVersion != 7)
        {
            /* D3D8 and D3D9 addref the private data, DDraw does not. This
             * can't be handled in ddraw because it doesn't know if the
             * pointer returned is an IUnknown * or just a blob. */
            IUnknown_AddRef(d->ptr.object);
        }
    }
    else
    {
        memcpy(data, d->ptr.data, d->size);
    }

    return WINED3D_OK;
}
HRESULT CDECL wined3d_resource_free_private_data(struct wined3d_resource *resource, REFGUID guid)
{
    struct private_data *data;

    TRACE("resource %p, guid %s.\n", resource, debugstr_guid(guid));

    data = resource_find_private_data(resource, guid);
    if (!data) return WINED3DERR_NOTFOUND;

    if (data->flags & WINED3DSPD_IUNKNOWN)
    {
        if (data->ptr.object)
            IUnknown_Release(data->ptr.object);
    }
    else
    {
        HeapFree(GetProcessHeap(), 0, data->ptr.data);
    }
    list_remove(&data->entry);

    HeapFree(GetProcessHeap(), 0, data);

    return WINED3D_OK;
}

DWORD resource_set_priority(struct wined3d_resource *resource, DWORD priority)
{
    DWORD prev = resource->priority;
    resource->priority = priority;
    TRACE("resource %p, new priority %u, returning old priority %u.\n", resource, priority, prev);
    return prev;
}

DWORD resource_get_priority(const struct wined3d_resource *resource)
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
