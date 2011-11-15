/*
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
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
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_surface);

/* Context activation is done by the caller. */
static void volume_bind_and_dirtify(struct wined3d_volume *volume, const struct wined3d_gl_info *gl_info)
{
    struct wined3d_texture *container = volume->container;
    DWORD active_sampler;

    /* We don't need a specific texture unit, but after binding the texture the current unit is dirty.
     * Read the unit back instead of switching to 0, this avoids messing around with the state manager's
     * gl states. The current texture unit should always be a valid one.
     *
     * To be more specific, this is tricky because we can implicitly be called
     * from sampler() in state.c. This means we can't touch anything other than
     * whatever happens to be the currently active texture, or we would risk
     * marking already applied sampler states dirty again.
     *
     * TODO: Track the current active texture per GL context instead of using glGet
     */
    if (gl_info->supported[ARB_MULTITEXTURE])
    {
        GLint active_texture;
        ENTER_GL();
        glGetIntegerv(GL_ACTIVE_TEXTURE, &active_texture);
        LEAVE_GL();
        active_sampler = volume->resource.device->rev_tex_unit_map[active_texture - GL_TEXTURE0_ARB];
    } else {
        active_sampler = 0;
    }

    if (active_sampler != WINED3D_UNMAPPED_STAGE)
    {
        IWineD3DDeviceImpl_MarkStateDirty(volume->resource.device, STATE_SAMPLER(active_sampler));
    }

    container->texture_ops->texture_bind(container, gl_info, FALSE);
}

void volume_add_dirty_box(struct wined3d_volume *volume, const WINED3DBOX *dirty_box)
{
    volume->dirty = TRUE;
    if (dirty_box)
    {
        volume->lockedBox.Left = min(volume->lockedBox.Left, dirty_box->Left);
        volume->lockedBox.Top = min(volume->lockedBox.Top, dirty_box->Top);
        volume->lockedBox.Front = min(volume->lockedBox.Front, dirty_box->Front);
        volume->lockedBox.Right = max(volume->lockedBox.Right, dirty_box->Right);
        volume->lockedBox.Bottom = max(volume->lockedBox.Bottom, dirty_box->Bottom);
        volume->lockedBox.Back = max(volume->lockedBox.Back, dirty_box->Back);
    }
    else
    {
        volume->lockedBox.Left = 0;
        volume->lockedBox.Top = 0;
        volume->lockedBox.Front = 0;
        volume->lockedBox.Right = volume->resource.width;
        volume->lockedBox.Bottom = volume->resource.height;
        volume->lockedBox.Back = volume->resource.depth;
    }
}

void volume_set_container(struct wined3d_volume *volume, struct wined3d_texture *container)
{
    TRACE("volume %p, container %p.\n", volume, container);

    volume->container = container;
}

/* Context activation is done by the caller. */
void volume_load(struct wined3d_volume *volume, UINT level, BOOL srgb_mode)
{
    const struct wined3d_gl_info *gl_info = &volume->resource.device->adapter->gl_info;
    const struct wined3d_format *format = volume->resource.format;

    TRACE("volume %p, level %u, srgb %#x, format %s (%#x).\n",
            volume, level, srgb_mode, debug_d3dformat(format->id), format->id);

    volume_bind_and_dirtify(volume, gl_info);

    ENTER_GL();
    GL_EXTCALL(glTexImage3DEXT(GL_TEXTURE_3D, level, format->glInternal,
            volume->resource.width, volume->resource.height, volume->resource.depth,
            0, format->glFormat, format->glType, volume->resource.allocatedMemory));
    checkGLcall("glTexImage3D");
    LEAVE_GL();

    /* When adding code releasing volume->resource.allocatedMemory to save
     * data keep in mind that GL_UNPACK_CLIENT_STORAGE_APPLE is enabled by
     * default if supported(GL_APPLE_client_storage). Thus do not release
     * volume->resource.allocatedMemory if GL_APPLE_client_storage is
     * supported. */
}

/* Do not call while under the GL lock. */
static void volume_unload(struct wined3d_resource *resource)
{
    TRACE("texture %p.\n", resource);

    /* The whole content is shadowed on This->resource.allocatedMemory, and
     * the texture name is managed by the VolumeTexture container. */

    resource_unload(resource);
}

ULONG CDECL wined3d_volume_incref(struct wined3d_volume *volume)
{
    ULONG refcount;

    if (volume->container)
    {
        TRACE("Forwarding to container %p.\n", volume->container);
        return wined3d_texture_incref(volume->container);
    }

    refcount = InterlockedIncrement(&volume->resource.ref);

    TRACE("%p increasing refcount to %u.\n", volume, refcount);

    return refcount;
}

/* Do not call while under the GL lock. */
ULONG CDECL wined3d_volume_decref(struct wined3d_volume *volume)
{
    ULONG refcount;

    if (volume->container)
    {
        TRACE("Forwarding to container %p.\n", volume->container);
        return wined3d_texture_decref(volume->container);
    }

    refcount = InterlockedDecrement(&volume->resource.ref);

    TRACE("%p decreasing refcount to %u.\n", volume, refcount);

    if (!refcount)
    {
        resource_cleanup(&volume->resource);
        volume->resource.parent_ops->wined3d_object_destroyed(volume->resource.parent);
        HeapFree(GetProcessHeap(), 0, volume);
    }

    return refcount;
}

void * CDECL wined3d_volume_get_parent(const struct wined3d_volume *volume)
{
    TRACE("volume %p.\n", volume);

    return volume->resource.parent;
}

HRESULT CDECL wined3d_volume_set_private_data(struct wined3d_volume *volume,
        REFGUID guid, const void *data, DWORD data_size, DWORD flags)
{
    return resource_set_private_data(&volume->resource, guid, data, data_size, flags);
}

HRESULT CDECL wined3d_volume_get_private_data(const struct wined3d_volume *volume,
        REFGUID guid, void *data, DWORD *data_size)
{
    return resource_get_private_data(&volume->resource, guid, data, data_size);
}

HRESULT CDECL wined3d_volume_free_private_data(struct wined3d_volume *volume, REFGUID guid)
{
    return resource_free_private_data(&volume->resource, guid);
}

DWORD CDECL wined3d_volume_set_priority(struct wined3d_volume *volume, DWORD priority)
{
    return resource_set_priority(&volume->resource, priority);
}

DWORD CDECL wined3d_volume_get_priority(const struct wined3d_volume *volume)
{
    return resource_get_priority(&volume->resource);
}

/* Do not call while under the GL lock. */
void CDECL wined3d_volume_preload(struct wined3d_volume *volume)
{
    FIXME("volume %p stub!\n", volume);
}

struct wined3d_resource * CDECL wined3d_volume_get_resource(struct wined3d_volume *volume)
{
    TRACE("volume %p.\n", volume);

    return &volume->resource;
}

HRESULT CDECL wined3d_volume_map(struct wined3d_volume *volume,
        WINED3DLOCKED_BOX *locked_box, const WINED3DBOX *box, DWORD flags)
{
    TRACE("volume %p, locked_box %p, box %p, flags %#x.\n",
            volume, locked_box, box, flags);

    if (!volume->resource.allocatedMemory)
        volume->resource.allocatedMemory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, volume->resource.size);

    TRACE("allocatedMemory %p.\n", volume->resource.allocatedMemory);

    locked_box->RowPitch = volume->resource.format->byte_count * volume->resource.width; /* Bytes / row */
    locked_box->SlicePitch = volume->resource.format->byte_count
            * volume->resource.width * volume->resource.height; /* Bytes / slice */
    if (!box)
    {
        TRACE("No box supplied - all is ok\n");
        locked_box->pBits = volume->resource.allocatedMemory;
        volume->lockedBox.Left   = 0;
        volume->lockedBox.Top    = 0;
        volume->lockedBox.Front  = 0;
        volume->lockedBox.Right  = volume->resource.width;
        volume->lockedBox.Bottom = volume->resource.height;
        volume->lockedBox.Back   = volume->resource.depth;
    }
    else
    {
        TRACE("Lock Box (%p) = l %d, t %d, r %d, b %d, fr %d, ba %d\n",
                box, box->Left, box->Top, box->Right, box->Bottom, box->Front, box->Back);
        locked_box->pBits = volume->resource.allocatedMemory
                + (locked_box->SlicePitch * box->Front)     /* FIXME: is front < back or vica versa? */
                + (locked_box->RowPitch * box->Top)
                + (box->Left * volume->resource.format->byte_count);
        volume->lockedBox.Left   = box->Left;
        volume->lockedBox.Top    = box->Top;
        volume->lockedBox.Front  = box->Front;
        volume->lockedBox.Right  = box->Right;
        volume->lockedBox.Bottom = box->Bottom;
        volume->lockedBox.Back   = box->Back;
    }

    if (!(flags & (WINED3DLOCK_NO_DIRTY_UPDATE | WINED3DLOCK_READONLY)))
    {
        volume_add_dirty_box(volume, &volume->lockedBox);
        wined3d_texture_set_dirty(volume->container, TRUE);
    }

    volume->locked = TRUE;

    TRACE("Returning memory %p, row pitch %d, slice pitch %d.\n",
            locked_box->pBits, locked_box->RowPitch, locked_box->SlicePitch);

    return WINED3D_OK;
}

struct wined3d_volume * CDECL wined3d_volume_from_resource(struct wined3d_resource *resource)
{
    return volume_from_resource(resource);
}

HRESULT CDECL wined3d_volume_unmap(struct wined3d_volume *volume)
{
    TRACE("volume %p.\n", volume);

    if (!volume->locked)
    {
        WARN("Trying to unlock unlocked volume %p.\n", volume);
        return WINED3DERR_INVALIDCALL;
    }

    volume->locked = FALSE;
    memset(&volume->lockedBox, 0, sizeof(volume->lockedBox));

    return WINED3D_OK;
}

static const struct wined3d_resource_ops volume_resource_ops =
{
    volume_unload,
};

static HRESULT volume_init(struct wined3d_volume *volume, struct wined3d_device *device, UINT width,
        UINT height, UINT depth, DWORD usage, enum wined3d_format_id format_id, WINED3DPOOL pool,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    const struct wined3d_format *format = wined3d_get_format(gl_info, format_id);
    HRESULT hr;

    if (!gl_info->supported[EXT_TEXTURE3D])
    {
        WARN("Volume cannot be created - no volume texture support.\n");
        return WINED3DERR_INVALIDCALL;
    }

    hr = resource_init(&volume->resource, device, WINED3DRTYPE_VOLUME, format,
            WINED3DMULTISAMPLE_NONE, 0, usage, pool, width, height, depth,
            width * height * depth * format->byte_count, parent, parent_ops,
            &volume_resource_ops);
    if (FAILED(hr))
    {
        WARN("Failed to initialize resource, returning %#x.\n", hr);
        return hr;
    }

    volume->lockable = TRUE;
    volume->locked = FALSE;
    memset(&volume->lockedBox, 0, sizeof(volume->lockedBox));
    volume->dirty = TRUE;

    volume_add_dirty_box(volume, NULL);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_volume_create(struct wined3d_device *device, UINT width, UINT height,
        UINT depth, DWORD usage, enum wined3d_format_id format_id, WINED3DPOOL pool, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_volume **volume)
{
    struct wined3d_volume *object;
    HRESULT hr;

    TRACE("device %p, width %u, height %u, depth %u, usage %#x, format %s, pool %s\n",
            device, width, height, depth, usage, debug_d3dformat(format_id), debug_d3dpool(pool));
    TRACE("parent %p, parent_ops %p, volume %p.\n", parent, parent_ops, volume);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Out of memory\n");
        *volume = NULL;
        return WINED3DERR_OUTOFVIDEOMEMORY;
    }

    hr = volume_init(object, device, width, height, depth, usage, format_id, pool, parent, parent_ops);
    if (FAILED(hr))
    {
        WARN("Failed to initialize volume, returning %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created volume %p.\n", object);
    *volume = object;

    return WINED3D_OK;
}
