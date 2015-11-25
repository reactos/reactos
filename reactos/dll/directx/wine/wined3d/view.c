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

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

ULONG CDECL wined3d_rendertarget_view_incref(struct wined3d_rendertarget_view *view)
{
    ULONG refcount = InterlockedIncrement(&view->refcount);

    TRACE("%p increasing refcount to %u.\n", view, refcount);

    return refcount;
}

#if defined(STAGING_CSMT)
void wined3d_rendertarget_view_destroy(struct wined3d_rendertarget_view *view)
{
    HeapFree(GetProcessHeap(), 0, view);
}

#endif /* STAGING_CSMT */
ULONG CDECL wined3d_rendertarget_view_decref(struct wined3d_rendertarget_view *view)
{
    ULONG refcount = InterlockedDecrement(&view->refcount);

    TRACE("%p decreasing refcount to %u.\n", view, refcount);

    if (!refcount)
    {
#if defined(STAGING_CSMT)
        struct wined3d_device *device = view->resource->device;

        /* Call wined3d_object_destroyed() before releasing the resource,
         * since releasing the resource may end up destroying the parent. */
        view->parent_ops->wined3d_object_destroyed(view->parent);
        wined3d_resource_decref(view->resource);
        wined3d_cs_emit_view_destroy(device->cs, view);
#else  /* STAGING_CSMT */
        /* Call wined3d_object_destroyed() before releasing the resource,
         * since releasing the resource may end up destroying the parent. */
        view->parent_ops->wined3d_object_destroyed(view->parent);
        wined3d_resource_decref(view->resource);
        HeapFree(GetProcessHeap(), 0, view);
#endif /* STAGING_CSMT */
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
    struct wined3d_resource *sub_resource;

    TRACE("view %p.\n", view);

    if (view->resource->type == WINED3D_RTYPE_BUFFER)
        return wined3d_buffer_get_parent(buffer_from_resource(view->resource));

    if (!(sub_resource = wined3d_texture_get_sub_resource(wined3d_texture_from_resource(view->resource),
            view->sub_resource_idx)))
        return NULL;

    return wined3d_resource_get_parent(sub_resource);
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

static void wined3d_rendertarget_view_init(struct wined3d_rendertarget_view *view,
        const struct wined3d_rendertarget_view_desc *desc, struct wined3d_resource *resource,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_gl_info *gl_info = &resource->device->adapter->gl_info;

    view->refcount = 1;
    view->resource = resource;
    wined3d_resource_incref(resource);
    view->parent = parent;
    view->parent_ops = parent_ops;

    view->format = wined3d_get_format(gl_info, desc->format_id);
    view->format_flags = view->format->flags[resource->gl_type];
    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        view->sub_resource_idx = 0;
        view->buffer_offset = desc->u.buffer.start_idx;
        view->width = desc->u.buffer.count;
        view->height = 1;
        view->depth = 1;
    }
    else
    {
        struct wined3d_texture *texture = wined3d_texture_from_resource(resource);
        struct wined3d_resource *sub_resource;

        view->sub_resource_idx = desc->u.texture.layer_idx * texture->level_count + desc->u.texture.level_idx;
        sub_resource = wined3d_texture_get_sub_resource(texture, view->sub_resource_idx);

        view->buffer_offset = 0;
        view->width = sub_resource->width;
        view->height = sub_resource->height;
        view->depth = desc->u.texture.layer_count;
    }
}

HRESULT CDECL wined3d_rendertarget_view_create(const struct wined3d_rendertarget_view_desc *desc,
        struct wined3d_resource *resource, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_rendertarget_view **view)
{
    struct wined3d_rendertarget_view *object;

    TRACE("desc %p, resource %p, parent %p, parent_ops %p, view %p.\n",
            desc, resource, parent, parent_ops, view);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    wined3d_rendertarget_view_init(object, desc, resource, parent, parent_ops);

    TRACE("Created render target view %p.\n", object);
    *view = object;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_rendertarget_view_create_from_surface(struct wined3d_surface *surface,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_rendertarget_view **view)
{
    struct wined3d_rendertarget_view_desc desc;

    TRACE("surface %p, parent %p, parent_ops %p, view %p.\n", surface, parent, parent_ops, view);

    desc.format_id = surface->resource.format->id;
    desc.u.texture.level_idx = surface->texture_level;
    desc.u.texture.layer_idx = surface->texture_layer;
    desc.u.texture.layer_count = 1;

    return wined3d_rendertarget_view_create(&desc, &surface->container->resource, parent, parent_ops, view);
}

HRESULT CDECL wined3d_rendertarget_view_create_from_sub_resource(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_rendertarget_view **view)
{
    struct wined3d_resource *sub_resource;

    TRACE("texture %p, sub_resource_idx %u, parent %p, parent_ops %p, view %p.\n",
            texture, sub_resource_idx, parent, parent_ops, view);

    if (!(sub_resource = wined3d_texture_get_sub_resource(texture, sub_resource_idx)))
        return WINED3DERR_INVALIDCALL;

    if (sub_resource->type != WINED3D_RTYPE_SURFACE)
    {
        FIXME("Not implemented for %s resources.\n", debug_d3dresourcetype(texture->resource.type));
        return WINED3DERR_INVALIDCALL;
    }

    return wined3d_rendertarget_view_create_from_surface(surface_from_resource(sub_resource),
            parent, parent_ops, view);
}

ULONG CDECL wined3d_shader_resource_view_incref(struct wined3d_shader_resource_view *view)
{
    ULONG refcount = InterlockedIncrement(&view->refcount);

    TRACE("%p increasing refcount to %u.\n", view, refcount);

    return refcount;
}

ULONG CDECL wined3d_shader_resource_view_decref(struct wined3d_shader_resource_view *view)
{
    ULONG refcount = InterlockedDecrement(&view->refcount);

    TRACE("%p decreasing refcount to %u.\n", view, refcount);

    if (!refcount)
    {
        /* Call wined3d_object_destroyed() before releasing the resource,
         * since releasing the resource may end up destroying the parent. */
        view->parent_ops->wined3d_object_destroyed(view->parent);
        wined3d_resource_decref(view->resource);
        HeapFree(GetProcessHeap(), 0, view);
    }

    return refcount;
}

void * CDECL wined3d_shader_resource_view_get_parent(const struct wined3d_shader_resource_view *view)
{
    TRACE("view %p.\n", view);

    return view->parent;
}

HRESULT CDECL wined3d_shader_resource_view_create(struct wined3d_resource *resource, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_shader_resource_view **view)
{
    struct wined3d_shader_resource_view *object;

    TRACE("resource %p, parent %p, parent_ops %p, view %p.\n", resource, parent, parent_ops, view);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->refcount = 1;
    object->resource = resource;
    wined3d_resource_incref(resource);
    object->parent = parent;
    object->parent_ops = parent_ops;

    TRACE("Created shader resource view %p.\n", object);
    *view = object;

    return WINED3D_OK;
}
