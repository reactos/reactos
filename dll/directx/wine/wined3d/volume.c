/*
 * IWineD3DVolume implementation
 *
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
 * Copyright 2009 Henri Verbeet for CodeWeavers
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
static void volume_bind_and_dirtify(IWineD3DVolume *iface) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    const struct wined3d_gl_info *gl_info = &This->resource.device->adapter->gl_info;
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
        active_sampler = This->resource.device->rev_tex_unit_map[active_texture - GL_TEXTURE0_ARB];
    } else {
        active_sampler = 0;
    }

    if (active_sampler != WINED3D_UNMAPPED_STAGE)
    {
        IWineD3DDeviceImpl_MarkStateDirty(This->resource.device, STATE_SAMPLER(active_sampler));
    }

    IWineD3DVolumeTexture_BindTexture((IWineD3DVolumeTexture *)This->container, FALSE);
}

void volume_add_dirty_box(IWineD3DVolume *iface, const WINED3DBOX *dirty_box)
{
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;

    This->dirty = TRUE;
    if (dirty_box)
    {
        This->lockedBox.Left = min(This->lockedBox.Left, dirty_box->Left);
        This->lockedBox.Top = min(This->lockedBox.Top, dirty_box->Top);
        This->lockedBox.Front = min(This->lockedBox.Front, dirty_box->Front);
        This->lockedBox.Right = max(This->lockedBox.Right, dirty_box->Right);
        This->lockedBox.Bottom = max(This->lockedBox.Bottom, dirty_box->Bottom);
        This->lockedBox.Back = max(This->lockedBox.Back, dirty_box->Back);
    }
    else
    {
        This->lockedBox.Left = 0;
        This->lockedBox.Top = 0;
        This->lockedBox.Front = 0;
        This->lockedBox.Right = This->currentDesc.Width;
        This->lockedBox.Bottom = This->currentDesc.Height;
        This->lockedBox.Back = This->currentDesc.Depth;
    }
}

void volume_set_container(IWineD3DVolumeImpl *volume, struct IWineD3DVolumeTextureImpl *container)
{
    TRACE("volume %p, container %p.\n", volume, container);

    volume->container = container;
}

/* *******************************************
   IWineD3DVolume IUnknown parts follow
   ******************************************* */
static HRESULT WINAPI IWineD3DVolumeImpl_QueryInterface(IWineD3DVolume *iface, REFIID riid, void **object)
{
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IWineD3DVolume)
            || IsEqualGUID(riid, &IID_IWineD3DResource)
            || IsEqualGUID(riid, &IID_IWineD3DBase)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IUnknown_AddRef(iface);
        *object = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *object = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IWineD3DVolumeImpl_AddRef(IWineD3DVolume *iface) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    TRACE("(%p) : AddRef increasing from %d\n", This, This->resource.ref);
    return InterlockedIncrement(&This->resource.ref);
}

/* Do not call while under the GL lock. */
static ULONG WINAPI IWineD3DVolumeImpl_Release(IWineD3DVolume *iface) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %d\n", This, This->resource.ref);
    ref = InterlockedDecrement(&This->resource.ref);

    if (!ref)
    {
        resource_cleanup((IWineD3DResource *)iface);
        This->resource.parent_ops->wined3d_object_destroyed(This->resource.parent);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* ****************************************************
   IWineD3DVolume IWineD3DResource parts follow
   **************************************************** */
static void * WINAPI IWineD3DVolumeImpl_GetParent(IWineD3DVolume *iface)
{
    TRACE("iface %p.\n", iface);

    return ((IWineD3DVolumeImpl *)iface)->resource.parent;
}

static HRESULT WINAPI IWineD3DVolumeImpl_SetPrivateData(IWineD3DVolume *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    return resource_set_private_data((IWineD3DResource *)iface, refguid, pData, SizeOfData, Flags);
}

static HRESULT WINAPI IWineD3DVolumeImpl_GetPrivateData(IWineD3DVolume *iface, REFGUID  refguid, void* pData, DWORD* pSizeOfData) {
    return resource_get_private_data((IWineD3DResource *)iface, refguid, pData, pSizeOfData);
}

static HRESULT WINAPI IWineD3DVolumeImpl_FreePrivateData(IWineD3DVolume *iface, REFGUID refguid) {
    return resource_free_private_data((IWineD3DResource *)iface, refguid);
}

static DWORD WINAPI IWineD3DVolumeImpl_SetPriority(IWineD3DVolume *iface, DWORD PriorityNew) {
    return resource_set_priority((IWineD3DResource *)iface, PriorityNew);
}

static DWORD WINAPI IWineD3DVolumeImpl_GetPriority(IWineD3DVolume *iface) {
    return resource_get_priority((IWineD3DResource *)iface);
}

/* Do not call while under the GL lock. */
static void WINAPI IWineD3DVolumeImpl_PreLoad(IWineD3DVolume *iface) {
    FIXME("iface %p stub!\n", iface);
}

/* Do not call while under the GL lock. */
static void WINAPI IWineD3DVolumeImpl_UnLoad(IWineD3DVolume *iface)
{
    TRACE("iface %p.\n", iface);

    /* The whole content is shadowed on This->resource.allocatedMemory, and
     * the texture name is managed by the VolumeTexture container. */

    resource_unload((IWineD3DResourceImpl *)iface);
}

static WINED3DRESOURCETYPE WINAPI IWineD3DVolumeImpl_GetType(IWineD3DVolume *iface) {
    return resource_get_type((IWineD3DResource *)iface);
}

static void WINAPI IWineD3DVolumeImpl_GetDesc(IWineD3DVolume *iface, WINED3DVOLUME_DESC *desc)
{
    IWineD3DVolumeImpl *volume = (IWineD3DVolumeImpl *)iface;

    TRACE("iface %p, desc %p.\n", iface, desc);

    desc->Format = volume->resource.format->id;
    desc->Type = volume->resource.resourceType;
    desc->Usage = volume->resource.usage;
    desc->Pool = volume->resource.pool;
    desc->Size = volume->resource.size; /* dx8 only */
    desc->Width = volume->currentDesc.Width;
    desc->Height = volume->currentDesc.Height;
    desc->Depth = volume->currentDesc.Depth;
}

static HRESULT WINAPI IWineD3DVolumeImpl_LockBox(IWineD3DVolume *iface, WINED3DLOCKED_BOX* pLockedVolume, CONST WINED3DBOX* pBox, DWORD Flags) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    FIXME("(%p) : pBox=%p stub\n", This, pBox);

    if(!This->resource.allocatedMemory) {
        This->resource.allocatedMemory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, This->resource.size);
    }

    /* fixme: should we really lock as such? */
    TRACE("(%p) : box=%p, output pbox=%p, allMem=%p\n", This, pBox, pLockedVolume, This->resource.allocatedMemory);

    pLockedVolume->RowPitch = This->resource.format->byte_count * This->currentDesc.Width; /* Bytes / row   */
    pLockedVolume->SlicePitch = This->resource.format->byte_count
            * This->currentDesc.Width * This->currentDesc.Height;                               /* Bytes / slice */
    if (!pBox) {
        TRACE("No box supplied - all is ok\n");
        pLockedVolume->pBits = This->resource.allocatedMemory;
        This->lockedBox.Left   = 0;
        This->lockedBox.Top    = 0;
        This->lockedBox.Front  = 0;
        This->lockedBox.Right  = This->currentDesc.Width;
        This->lockedBox.Bottom = This->currentDesc.Height;
        This->lockedBox.Back   = This->currentDesc.Depth;
    } else {
        TRACE("Lock Box (%p) = l %d, t %d, r %d, b %d, fr %d, ba %d\n", pBox, pBox->Left, pBox->Top, pBox->Right, pBox->Bottom, pBox->Front, pBox->Back);
        pLockedVolume->pBits = This->resource.allocatedMemory
                + (pLockedVolume->SlicePitch * pBox->Front)     /* FIXME: is front < back or vica versa? */
                + (pLockedVolume->RowPitch * pBox->Top)
                + (pBox->Left * This->resource.format->byte_count);
        This->lockedBox.Left   = pBox->Left;
        This->lockedBox.Top    = pBox->Top;
        This->lockedBox.Front  = pBox->Front;
        This->lockedBox.Right  = pBox->Right;
        This->lockedBox.Bottom = pBox->Bottom;
        This->lockedBox.Back   = pBox->Back;
    }

    if (Flags & (WINED3DLOCK_NO_DIRTY_UPDATE | WINED3DLOCK_READONLY)) {
      /* Don't dirtify */
    }
    else
    {
        volume_add_dirty_box(iface, &This->lockedBox);
        This->container->baseTexture.texture_rgb.dirty = TRUE;
        This->container->baseTexture.texture_srgb.dirty = TRUE;
    }

    This->locked = TRUE;
    TRACE("returning memory@%p rpitch(%d) spitch(%d)\n", pLockedVolume->pBits, pLockedVolume->RowPitch, pLockedVolume->SlicePitch);
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DVolumeImpl_UnlockBox(IWineD3DVolume *iface) {
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    if (!This->locked)
    {
        WARN("Trying to unlock unlocked volume %p.\n", iface);
        return WINED3DERR_INVALIDCALL;
    }
    TRACE("(%p) : unlocking volume\n", This);
    This->locked = FALSE;
    memset(&This->lockedBox, 0, sizeof(RECT));
    return WINED3D_OK;
}

/* Internal use functions follow : */

/* Context activation is done by the caller. */
static HRESULT WINAPI IWineD3DVolumeImpl_LoadTexture(IWineD3DVolume *iface, int gl_level, BOOL srgb_mode)
{
    IWineD3DVolumeImpl *This = (IWineD3DVolumeImpl *)iface;
    const struct wined3d_gl_info *gl_info = &This->resource.device->adapter->gl_info;
    const struct wined3d_format *format = This->resource.format;

    TRACE("iface %p, level %u, srgb %#x, format %s (%#x).\n",
            iface, gl_level, srgb_mode, debug_d3dformat(format->id), format->id);

    volume_bind_and_dirtify(iface);

    TRACE("Calling glTexImage3D %x level=%d, intfmt=%x, w=%d, h=%d,d=%d, 0=%d, glFmt=%x, glType=%x, Mem=%p\n",
            GL_TEXTURE_3D, gl_level, format->glInternal, This->currentDesc.Width, This->currentDesc.Height,
            This->currentDesc.Depth, 0, format->glFormat, format->glType, This->resource.allocatedMemory);

    ENTER_GL();
    GL_EXTCALL(glTexImage3DEXT(GL_TEXTURE_3D, gl_level, format->glInternal,
            This->currentDesc.Width, This->currentDesc.Height, This->currentDesc.Depth,
            0, format->glFormat, format->glType, This->resource.allocatedMemory));
    checkGLcall("glTexImage3D");
    LEAVE_GL();

    /* When adding code releasing This->resource.allocatedMemory to save data keep in mind that
     * GL_UNPACK_CLIENT_STORAGE_APPLE is enabled by default if supported(GL_APPLE_client_storage).
     * Thus do not release This->resource.allocatedMemory if GL_APPLE_client_storage is supported.
     */
    return WINED3D_OK;

}

static const IWineD3DVolumeVtbl IWineD3DVolume_Vtbl =
{
    /* IUnknown */
    IWineD3DVolumeImpl_QueryInterface,
    IWineD3DVolumeImpl_AddRef,
    IWineD3DVolumeImpl_Release,
    /* IWineD3DResource */
    IWineD3DVolumeImpl_GetParent,
    IWineD3DVolumeImpl_SetPrivateData,
    IWineD3DVolumeImpl_GetPrivateData,
    IWineD3DVolumeImpl_FreePrivateData,
    IWineD3DVolumeImpl_SetPriority,
    IWineD3DVolumeImpl_GetPriority,
    IWineD3DVolumeImpl_PreLoad,
    IWineD3DVolumeImpl_UnLoad,
    IWineD3DVolumeImpl_GetType,
    /* IWineD3DVolume */
    IWineD3DVolumeImpl_GetDesc,
    IWineD3DVolumeImpl_LockBox,
    IWineD3DVolumeImpl_UnlockBox,
    /* Internal interface */
    IWineD3DVolumeImpl_LoadTexture,
};

HRESULT volume_init(IWineD3DVolumeImpl *volume, IWineD3DDeviceImpl *device, UINT width,
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

    volume->lpVtbl = &IWineD3DVolume_Vtbl;

    hr = resource_init((IWineD3DResource *)volume, WINED3DRTYPE_VOLUME, device,
            width * height * depth * format->byte_count, usage, format, pool, parent, parent_ops);
    if (FAILED(hr))
    {
        WARN("Failed to initialize resource, returning %#x.\n", hr);
        return hr;
    }

    volume->currentDesc.Width = width;
    volume->currentDesc.Height = height;
    volume->currentDesc.Depth = depth;
    volume->lockable = TRUE;
    volume->locked = FALSE;
    memset(&volume->lockedBox, 0, sizeof(volume->lockedBox));
    volume->dirty = TRUE;

    volume_add_dirty_box((IWineD3DVolume *)volume, NULL);

    return WINED3D_OK;
}
