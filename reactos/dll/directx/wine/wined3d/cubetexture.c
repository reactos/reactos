/*
 * IWineD3DCubeTexture implementation
 *
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
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

WINE_DEFAULT_DEBUG_CHANNEL(d3d_texture);

static void cubetexture_internal_preload(IWineD3DBaseTexture *iface, enum WINED3DSRGB srgb)
{
    /* Override the IWineD3DResource Preload method. */
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;
    UINT sub_count = This->baseTexture.level_count * This->baseTexture.layer_count;
    IWineD3DDeviceImpl *device = This->resource.device;
    struct wined3d_context *context = NULL;
    BOOL srgb_mode;
    BOOL *dirty;
    UINT i;

    switch (srgb)
    {
        case SRGB_RGB:
            srgb_mode = FALSE;
            break;

        case SRGB_BOTH:
            cubetexture_internal_preload(iface, SRGB_RGB);
            /* Fallthrough */

        case SRGB_SRGB:
            srgb_mode = TRUE;
            break;

        default:
            srgb_mode = This->baseTexture.is_srgb;
            break;
    }
    dirty = srgb_mode ? &This->baseTexture.texture_srgb.dirty : &This->baseTexture.texture_rgb.dirty;

    TRACE("(%p) : About to load texture: dirtified(%u).\n", This, *dirty);

    /* We only have to activate a context for gl when we're not drawing.
     * In most cases PreLoad will be called during draw and a context was
     * activated at the beginning of drawPrimitive. */
    if (!device->isInDraw)
    {
        /* No danger of recursive calls, context_acquire() sets isInDraw to true
         * when loading offscreen render targets into their texture. */
        context = context_acquire(device, NULL);
    }

    if (This->resource.format_desc->format == WINED3DFMT_P8_UINT
            || This->resource.format_desc->format == WINED3DFMT_P8_UINT_A8_UNORM)
    {
        for (i = 0; i < sub_count; ++i)
        {
            IWineD3DSurfaceImpl *surface = (IWineD3DSurfaceImpl *)This->baseTexture.sub_resources[i];

            if (palette9_changed(surface))
            {
                TRACE("Reloading surface %p because the d3d8/9 palette was changed.\n", surface);
                /* TODO: This is not necessarily needed with hw palettized texture support. */
                IWineD3DSurface_LoadLocation((IWineD3DSurface *)surface, SFLAG_INSYSMEM, NULL);
                /* Make sure the texture is reloaded because of the palette change,
                 * this kills performance though :( */
                IWineD3DSurface_ModifyLocation((IWineD3DSurface *)surface, SFLAG_INTEXTURE, FALSE);
            }
        }
    }

    /* If the texture is marked dirty or the srgb sampler setting has changed
     * since the last load then reload the surfaces. */
    if (*dirty)
    {
        for (i = 0; i < sub_count; ++i)
        {
            IWineD3DSurface_LoadTexture((IWineD3DSurface *)This->baseTexture.sub_resources[i], srgb_mode);
        }
    }
    else
    {
        TRACE("(%p) Texture not dirty, nothing to do.\n" , iface);
    }

    /* No longer dirty. */
    *dirty = FALSE;

    if (context) context_release(context);
}

static void cubetexture_cleanup(IWineD3DCubeTextureImpl *This)
{
    UINT sub_count = This->baseTexture.level_count * This->baseTexture.layer_count;
    UINT i;

    TRACE("(%p) : Cleaning up.\n", This);

    for (i = 0; i < sub_count; ++i)
    {
        IWineD3DSurfaceImpl *surface = (IWineD3DSurfaceImpl *)This->baseTexture.sub_resources[i];

        if (surface)
        {
            /* Clean out the texture name we gave to the surface so that the
             * surface doesn't try and release it. */
            surface_set_texture_name(surface, 0, TRUE);
            surface_set_texture_name(surface, 0, FALSE);
            surface_set_texture_target(surface, 0);
            IWineD3DSurface_SetContainer((IWineD3DSurface *)surface, NULL);
            IWineD3DSurface_Release((IWineD3DSurface *)surface);
        }
    }
    basetexture_cleanup((IWineD3DBaseTexture *)This);
}

/* *******************************************
   IWineD3DCubeTexture IUnknown parts follow
   ******************************************* */

static HRESULT WINAPI IWineD3DCubeTextureImpl_QueryInterface(IWineD3DCubeTexture *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DResource)
        || IsEqualGUID(riid, &IID_IWineD3DBaseTexture)
        || IsEqualGUID(riid, &IID_IWineD3DCubeTexture)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IWineD3DCubeTextureImpl_AddRef(IWineD3DCubeTexture *iface) {
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;
    TRACE("(%p) : AddRef increasing from %d\n", This, This->resource.ref);
    return InterlockedIncrement(&This->resource.ref);
}

static ULONG WINAPI IWineD3DCubeTextureImpl_Release(IWineD3DCubeTexture *iface) {
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %d\n", This, This->resource.ref);
    ref = InterlockedDecrement(&This->resource.ref);
    if (!ref)
    {
        cubetexture_cleanup(This);
        This->resource.parent_ops->wined3d_object_destroyed(This->resource.parent);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* ****************************************************
   IWineD3DCubeTexture IWineD3DResource parts follow
   **************************************************** */
static HRESULT WINAPI IWineD3DCubeTextureImpl_SetPrivateData(IWineD3DCubeTexture *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    return resource_set_private_data((IWineD3DResource *)iface, refguid, pData, SizeOfData, Flags);
}

static HRESULT WINAPI IWineD3DCubeTextureImpl_GetPrivateData(IWineD3DCubeTexture *iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    return resource_get_private_data((IWineD3DResource *)iface, refguid, pData, pSizeOfData);
}

static HRESULT WINAPI IWineD3DCubeTextureImpl_FreePrivateData(IWineD3DCubeTexture *iface, REFGUID refguid) {
    return resource_free_private_data((IWineD3DResource *)iface, refguid);
}

static DWORD WINAPI IWineD3DCubeTextureImpl_SetPriority(IWineD3DCubeTexture *iface, DWORD PriorityNew) {
    return resource_set_priority((IWineD3DResource *)iface, PriorityNew);
}

static DWORD WINAPI IWineD3DCubeTextureImpl_GetPriority(IWineD3DCubeTexture *iface) {
    return resource_get_priority((IWineD3DResource *)iface);
}

static void WINAPI IWineD3DCubeTextureImpl_PreLoad(IWineD3DCubeTexture *iface) {
    cubetexture_internal_preload((IWineD3DBaseTexture *) iface, SRGB_ANY);
}

static void WINAPI IWineD3DCubeTextureImpl_UnLoad(IWineD3DCubeTexture *iface)
{
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;
    UINT sub_count = This->baseTexture.level_count * This->baseTexture.layer_count;
    UINT i;

    TRACE("iface %p.\n", iface);

    /* Unload all the surfaces and reset the texture name. If UnLoad was called on the
     * surface before, this one will be a NOP and vice versa. Unloading an unloaded
     * surface is fine. */

    for (i = 0; i < sub_count; ++i)
    {
        IWineD3DSurfaceImpl *surface = (IWineD3DSurfaceImpl *)This->baseTexture.sub_resources[i];

        IWineD3DSurface_UnLoad((IWineD3DSurface *)surface);
        surface_set_texture_name(surface, 0, TRUE);
        surface_set_texture_name(surface, 0, FALSE);
    }

    basetexture_unload((IWineD3DBaseTexture *)iface);
}

static WINED3DRESOURCETYPE WINAPI IWineD3DCubeTextureImpl_GetType(IWineD3DCubeTexture *iface) {
    return resource_get_type((IWineD3DResource *)iface);
}

static HRESULT WINAPI IWineD3DCubeTextureImpl_GetParent(IWineD3DCubeTexture *iface, IUnknown **pParent) {
    return resource_get_parent((IWineD3DResource *)iface, pParent);
}

/* ******************************************************
   IWineD3DCubeTexture IWineD3DBaseTexture parts follow
   ****************************************************** */
static DWORD WINAPI IWineD3DCubeTextureImpl_SetLOD(IWineD3DCubeTexture *iface, DWORD LODNew) {
    return basetexture_set_lod((IWineD3DBaseTexture *)iface, LODNew);
}

static DWORD WINAPI IWineD3DCubeTextureImpl_GetLOD(IWineD3DCubeTexture *iface) {
    return basetexture_get_lod((IWineD3DBaseTexture *)iface);
}

static DWORD WINAPI IWineD3DCubeTextureImpl_GetLevelCount(IWineD3DCubeTexture *iface) {
    return basetexture_get_level_count((IWineD3DBaseTexture *)iface);
}

static HRESULT WINAPI IWineD3DCubeTextureImpl_SetAutoGenFilterType(IWineD3DCubeTexture *iface, WINED3DTEXTUREFILTERTYPE FilterType) {
  return basetexture_set_autogen_filter_type((IWineD3DBaseTexture *)iface, FilterType);
}

static WINED3DTEXTUREFILTERTYPE WINAPI IWineD3DCubeTextureImpl_GetAutoGenFilterType(IWineD3DCubeTexture *iface) {
  return basetexture_get_autogen_filter_type((IWineD3DBaseTexture *)iface);
}

static void WINAPI IWineD3DCubeTextureImpl_GenerateMipSubLevels(IWineD3DCubeTexture *iface) {
    basetexture_generate_mipmaps((IWineD3DBaseTexture *)iface);
}

/* Internal function, No d3d mapping */
static BOOL WINAPI IWineD3DCubeTextureImpl_SetDirty(IWineD3DCubeTexture *iface, BOOL dirty) {
    return basetexture_set_dirty((IWineD3DBaseTexture *)iface, dirty);
}

/* Internal function, No d3d mapping */
static BOOL WINAPI IWineD3DCubeTextureImpl_GetDirty(IWineD3DCubeTexture *iface) {
    return basetexture_get_dirty((IWineD3DBaseTexture *)iface);
}

/* Context activation is done by the caller. */
static HRESULT WINAPI IWineD3DCubeTextureImpl_BindTexture(IWineD3DCubeTexture *iface, BOOL srgb) {
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;
    BOOL set_gl_texture_desc;
    HRESULT hr;

    TRACE("(%p) : relay to BaseTexture\n", This);

    hr = basetexture_bind((IWineD3DBaseTexture *)iface, srgb, &set_gl_texture_desc);
    if (set_gl_texture_desc && SUCCEEDED(hr))
    {
        UINT sub_count = This->baseTexture.level_count * This->baseTexture.layer_count;
        UINT i;

        for (i = 0; i < sub_count; ++i)
        {
            IWineD3DSurfaceImpl *surface = (IWineD3DSurfaceImpl *)This->baseTexture.sub_resources[i];

            if (This->baseTexture.is_srgb)
                surface_set_texture_name(surface, This->baseTexture.texture_srgb.name, TRUE);
            else
                surface_set_texture_name(surface, This->baseTexture.texture_rgb.name, FALSE);
        }
    }

    return hr;
}

static UINT WINAPI IWineD3DCubeTextureImpl_GetTextureDimensions(IWineD3DCubeTexture *iface)
{
    TRACE("iface %p.\n", iface);

    return GL_TEXTURE_CUBE_MAP_ARB;
}

static BOOL WINAPI IWineD3DCubeTextureImpl_IsCondNP2(IWineD3DCubeTexture *iface)
{
    TRACE("iface %p.\n", iface);

    return FALSE;
}

/* *******************************************
   IWineD3DCubeTexture IWineD3DCubeTexture parts follow
   ******************************************* */
static HRESULT WINAPI IWineD3DCubeTextureImpl_GetLevelDesc(IWineD3DCubeTexture *iface,
        UINT level, WINED3DSURFACE_DESC *desc)
{
    IWineD3DBaseTextureImpl *texture = (IWineD3DBaseTextureImpl *)iface;
    IWineD3DSurface *surface;

    TRACE("iface %p, level %u, desc %p.\n", iface, level, desc);

    if (!(surface = (IWineD3DSurface *)basetexture_get_sub_resource(texture, 0, level)))
    {
        WARN("Failed to get sub-resource.\n");
        return WINED3DERR_INVALIDCALL;
    }

    return IWineD3DSurface_GetDesc(surface, desc);
}

static HRESULT WINAPI IWineD3DCubeTextureImpl_GetCubeMapSurface(IWineD3DCubeTexture *iface,
        WINED3DCUBEMAP_FACES face, UINT level, IWineD3DSurface **surface)
{
    IWineD3DBaseTextureImpl *texture = (IWineD3DBaseTextureImpl *)iface;
    IWineD3DSurface *s;

    TRACE("iface %p, face %u, level %u, surface %p.\n",
            iface, face, level, surface);

    if (!(s = (IWineD3DSurface *)basetexture_get_sub_resource(texture, face, level)))
    {
        WARN("Failed to get sub-resource.\n");
        return WINED3DERR_INVALIDCALL;
    }

    IWineD3DSurface_AddRef(s);
    *surface = s;

    TRACE("Returning surface %p.\n", *surface);

    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DCubeTextureImpl_LockRect(IWineD3DCubeTexture *iface,
        WINED3DCUBEMAP_FACES face, UINT level, WINED3DLOCKED_RECT *locked_rect, const RECT *rect, DWORD flags)
{
    IWineD3DBaseTextureImpl *texture = (IWineD3DBaseTextureImpl *)iface;
    IWineD3DSurface *surface;

    TRACE("iface %p, face %u, level %u, locked_rect %p, rect %s, flags %#x.\n",
            iface, face, level, locked_rect, wine_dbgstr_rect(rect), flags);

    if (!(surface = (IWineD3DSurface *)basetexture_get_sub_resource(texture, face, level)))
    {
        WARN("Failed to get sub-resource.\n");
        return WINED3DERR_INVALIDCALL;
    }

    return IWineD3DSurface_LockRect(surface, locked_rect, rect, flags);
}

static HRESULT WINAPI IWineD3DCubeTextureImpl_UnlockRect(IWineD3DCubeTexture *iface,
        WINED3DCUBEMAP_FACES face, UINT level)
{
    IWineD3DBaseTextureImpl *texture = (IWineD3DBaseTextureImpl *)iface;
    IWineD3DSurface *surface;

    TRACE("iface %p, face %u, level %u.\n",
            iface, face, level);

    if (!(surface = (IWineD3DSurface *)basetexture_get_sub_resource(texture, face, level)))
    {
        WARN("Failed to get sub-resource.\n");
        return WINED3DERR_INVALIDCALL;
    }

    return IWineD3DSurface_UnlockRect(surface);
}

static HRESULT WINAPI IWineD3DCubeTextureImpl_AddDirtyRect(IWineD3DCubeTexture *iface,
        WINED3DCUBEMAP_FACES face, const RECT *dirty_rect)
{
    IWineD3DBaseTextureImpl *texture = (IWineD3DBaseTextureImpl *)iface;
    IWineD3DSurfaceImpl *surface;

    TRACE("iface %p, face %u, dirty_rect %s.\n",
            iface, face, wine_dbgstr_rect(dirty_rect));

    if (!(surface = (IWineD3DSurfaceImpl *)basetexture_get_sub_resource(texture, face, 0)))
    {
        WARN("Failed to get sub-resource.\n");
        return WINED3DERR_INVALIDCALL;
    }

    texture->baseTexture.texture_rgb.dirty = TRUE;
    texture->baseTexture.texture_srgb.dirty = TRUE;
    surface_add_dirty_rect(surface, dirty_rect);

    return WINED3D_OK;
}

static const IWineD3DCubeTextureVtbl IWineD3DCubeTexture_Vtbl =
{
    /* IUnknown */
    IWineD3DCubeTextureImpl_QueryInterface,
    IWineD3DCubeTextureImpl_AddRef,
    IWineD3DCubeTextureImpl_Release,
    /* IWineD3DResource */
    IWineD3DCubeTextureImpl_GetParent,
    IWineD3DCubeTextureImpl_SetPrivateData,
    IWineD3DCubeTextureImpl_GetPrivateData,
    IWineD3DCubeTextureImpl_FreePrivateData,
    IWineD3DCubeTextureImpl_SetPriority,
    IWineD3DCubeTextureImpl_GetPriority,
    IWineD3DCubeTextureImpl_PreLoad,
    IWineD3DCubeTextureImpl_UnLoad,
    IWineD3DCubeTextureImpl_GetType,
    /* IWineD3DBaseTexture */
    IWineD3DCubeTextureImpl_SetLOD,
    IWineD3DCubeTextureImpl_GetLOD,
    IWineD3DCubeTextureImpl_GetLevelCount,
    IWineD3DCubeTextureImpl_SetAutoGenFilterType,
    IWineD3DCubeTextureImpl_GetAutoGenFilterType,
    IWineD3DCubeTextureImpl_GenerateMipSubLevels,
    IWineD3DCubeTextureImpl_SetDirty,
    IWineD3DCubeTextureImpl_GetDirty,
    IWineD3DCubeTextureImpl_BindTexture,
    IWineD3DCubeTextureImpl_GetTextureDimensions,
    IWineD3DCubeTextureImpl_IsCondNP2,
    /* IWineD3DCubeTexture */
    IWineD3DCubeTextureImpl_GetLevelDesc,
    IWineD3DCubeTextureImpl_GetCubeMapSurface,
    IWineD3DCubeTextureImpl_LockRect,
    IWineD3DCubeTextureImpl_UnlockRect,
    IWineD3DCubeTextureImpl_AddDirtyRect
};

HRESULT cubetexture_init(IWineD3DCubeTextureImpl *texture, UINT edge_length, UINT levels,
        IWineD3DDeviceImpl *device, DWORD usage, WINED3DFORMAT format, WINED3DPOOL pool,
        IUnknown *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    const struct wined3d_format_desc *format_desc = getFormatDescEntry(format, gl_info);
    UINT pow2_edge_length;
    unsigned int i, j;
    UINT tmp_w;
    HRESULT hr;

    /* TODO: It should only be possible to create textures for formats
     * that are reported as supported. */
    if (WINED3DFMT_UNKNOWN >= format)
    {
        WARN("(%p) : Texture cannot be created with a format of WINED3DFMT_UNKNOWN.\n", texture);
        return WINED3DERR_INVALIDCALL;
    }

    if (!gl_info->supported[ARB_TEXTURE_CUBE_MAP] && pool != WINED3DPOOL_SCRATCH)
    {
        WARN("(%p) : Tried to create not supported cube texture.\n", texture);
        return WINED3DERR_INVALIDCALL;
    }

    /* Calculate levels for mip mapping */
    if (usage & WINED3DUSAGE_AUTOGENMIPMAP)
    {
        if (!gl_info->supported[SGIS_GENERATE_MIPMAP])
        {
            WARN("No mipmap generation support, returning D3DERR_INVALIDCALL.\n");
            return WINED3DERR_INVALIDCALL;
        }

        if (levels > 1)
        {
            WARN("D3DUSAGE_AUTOGENMIPMAP is set, and level count > 1, returning D3DERR_INVALIDCALL.\n");
            return WINED3DERR_INVALIDCALL;
        }

        levels = 1;
    }
    else if (!levels)
    {
        levels = wined3d_log2i(edge_length) + 1;
        TRACE("Calculated levels = %u.\n", levels);
    }

    texture->lpVtbl = &IWineD3DCubeTexture_Vtbl;

    hr = basetexture_init((IWineD3DBaseTextureImpl *)texture, 6, levels,
            WINED3DRTYPE_CUBETEXTURE, device, 0, usage, format_desc, pool, parent, parent_ops);
    if (FAILED(hr))
    {
        WARN("Failed to initialize basetexture, returning %#x\n", hr);
        return hr;
    }

    /* Find the nearest pow2 match. */
    pow2_edge_length = 1;
    while (pow2_edge_length < edge_length) pow2_edge_length <<= 1;

    if (gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO] || (edge_length == pow2_edge_length))
    {
        /* Precalculated scaling for 'faked' non power of two texture coords. */
        texture->baseTexture.pow2Matrix[0] = 1.0f;
        texture->baseTexture.pow2Matrix[5] = 1.0f;
        texture->baseTexture.pow2Matrix[10] = 1.0f;
        texture->baseTexture.pow2Matrix[15] = 1.0f;
    }
    else
    {
        /* Precalculated scaling for 'faked' non power of two texture coords. */
        texture->baseTexture.pow2Matrix[0] = ((float)edge_length) / ((float)pow2_edge_length);
        texture->baseTexture.pow2Matrix[5] = ((float)edge_length) / ((float)pow2_edge_length);
        texture->baseTexture.pow2Matrix[10] = ((float)edge_length) / ((float)pow2_edge_length);
        texture->baseTexture.pow2Matrix[15] = 1.0f;
        texture->baseTexture.pow2Matrix_identity = FALSE;
    }

    /* Generate all the surfaces. */
    tmp_w = edge_length;
    for (i = 0; i < texture->baseTexture.level_count; ++i)
    {
        /* Create the 6 faces. */
        for (j = 0; j < 6; ++j)
        {
            static const GLenum cube_targets[6] =
            {
                GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB,
                GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB,
                GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB,
                GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB,
                GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB,
                GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB,
            };
            UINT idx = j * texture->baseTexture.level_count + i;
            IWineD3DSurface *surface;

            hr = IWineD3DDeviceParent_CreateSurface(device->device_parent, parent, tmp_w, tmp_w,
                    format, usage, pool, i /* Level */, j, &surface);
            if (FAILED(hr))
            {
                FIXME("(%p) Failed to create surface, hr %#x.\n", texture, hr);
                cubetexture_cleanup(texture);
                return hr;
            }

            IWineD3DSurface_SetContainer(surface, (IWineD3DBase *)texture);
            surface_set_texture_target((IWineD3DSurfaceImpl *)surface, cube_targets[j]);
            texture->baseTexture.sub_resources[idx] = (IWineD3DResourceImpl *)surface;
            TRACE("Created surface level %u @ %p.\n", i, surface);
        }
        tmp_w = max(1, tmp_w >> 1);
    }
    texture->baseTexture.internal_preload = cubetexture_internal_preload;

    return WINED3D_OK;
}
