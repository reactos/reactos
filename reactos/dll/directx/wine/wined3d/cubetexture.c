/*
 * IWineD3DCubeTexture implementation
 *
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
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
#define GLINFO_LOCATION This->resource.wineD3DDevice->adapter->gl_info

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
    if (ref == 0) {
        IWineD3DCubeTexture_Destroy(iface, D3DCB_DefaultDestroySurface);
    }
    return ref;
}

/* ****************************************************
   IWineD3DCubeTexture IWineD3DResource parts follow
   **************************************************** */
static HRESULT WINAPI IWineD3DCubeTextureImpl_GetDevice(IWineD3DCubeTexture *iface, IWineD3DDevice** ppDevice) {
    return resource_get_device((IWineD3DResource *)iface, ppDevice);
}

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

void cubetexture_internal_preload(IWineD3DBaseTexture *iface, enum WINED3DSRGB srgb) {
    /* Override the IWineD3DResource Preload method */
    unsigned int i,j;
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
    BOOL srgb_mode;
    BOOL *dirty;

    switch(srgb) {
        case SRGB_RGB:      srgb_mode = FALSE; break;
        case SRGB_BOTH:     cubetexture_internal_preload(iface, SRGB_RGB);
        case SRGB_SRGB:     srgb_mode = TRUE; break;
        /* DONTKNOW, and shut up the compiler */
        default:            srgb_mode = This->baseTexture.is_srgb; break;
    }
    dirty = srgb_mode ? &This->baseTexture.srgbDirty : &This->baseTexture.dirty;

    TRACE("(%p) : About to load texture: dirtified(%d)\n", This, *dirty);

    /* We only have to activate a context for gl when we're not drawing. In most cases PreLoad will be called during draw
     * and a context was activated at the beginning of drawPrimitive
     */
    if(!device->isInDraw) {
        /* No danger of recursive calls, ActivateContext sets isInDraw to true when loading
         * offscreen render targets into their texture
         */
        ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
    }

    if (This->resource.format_desc->format == WINED3DFMT_P8 || This->resource.format_desc->format == WINED3DFMT_A8P8)
    {
        for (i = 0; i < This->baseTexture.levels; i++) {
            for (j = WINED3DCUBEMAP_FACE_POSITIVE_X; j <= WINED3DCUBEMAP_FACE_NEGATIVE_Z ; j++) {
                if(palette9_changed((IWineD3DSurfaceImpl *)This->surfaces[j][i])) {
                    TRACE("Reloading surface because the d3d8/9 palette was changed\n");
                    /* TODO: This is not necessarily needed with hw palettized texture support */
                    IWineD3DSurface_LoadLocation(This->surfaces[j][i], SFLAG_INSYSMEM, NULL);
                    /* Make sure the texture is reloaded because of the palette change, this kills performance though :( */
                    IWineD3DSurface_ModifyLocation(This->surfaces[j][i], SFLAG_INTEXTURE, FALSE);
                }
            }
        }
    }
    /* If the texture is marked dirty or the srgb sampler setting has changed since the last load then reload the surfaces */
    if (*dirty) {
        for (i = 0; i < This->baseTexture.levels; i++) {
            for (j = WINED3DCUBEMAP_FACE_POSITIVE_X; j <= WINED3DCUBEMAP_FACE_NEGATIVE_Z ; j++) {
                IWineD3DSurface_LoadTexture(This->surfaces[j][i], srgb_mode);
            }
        }
    } else {
        TRACE("(%p) Texture not dirty, nothing to do\n" , iface);
    }

    /* No longer dirty */
    *dirty = FALSE;
    return;
}

static void WINAPI IWineD3DCubeTextureImpl_PreLoad(IWineD3DCubeTexture *iface) {
    cubetexture_internal_preload((IWineD3DBaseTexture *) iface, SRGB_ANY);
}

static void WINAPI IWineD3DCubeTextureImpl_UnLoad(IWineD3DCubeTexture *iface) {
    unsigned int i, j;
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;
    TRACE("(%p)\n", This);

    /* Unload all the surfaces and reset the texture name. If UnLoad was called on the
     * surface before, this one will be a NOP and vice versa. Unloading an unloaded
     * surface is fine
     */
    for (i = 0; i < This->baseTexture.levels; i++) {
        for (j = WINED3DCUBEMAP_FACE_POSITIVE_X; j <= WINED3DCUBEMAP_FACE_NEGATIVE_Z ; j++) {
            IWineD3DSurface_UnLoad(This->surfaces[j][i]);
            surface_set_texture_name(This->surfaces[j][i], 0, TRUE);
            surface_set_texture_name(This->surfaces[j][i], 0, FALSE);
        }
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

static HRESULT WINAPI IWineD3DCubeTextureImpl_BindTexture(IWineD3DCubeTexture *iface, BOOL srgb) {
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;
    BOOL set_gl_texture_desc;
    HRESULT hr;

    TRACE("(%p) : relay to BaseTexture\n", This);

    hr = basetexture_bind((IWineD3DBaseTexture *)iface, srgb, &set_gl_texture_desc);
    if (set_gl_texture_desc && SUCCEEDED(hr)) {
        UINT i, j;
        for (i = 0; i < This->baseTexture.levels; ++i) {
            for (j = WINED3DCUBEMAP_FACE_POSITIVE_X; j <= WINED3DCUBEMAP_FACE_NEGATIVE_Z; ++j) {
                if(This->baseTexture.is_srgb) {
                    surface_set_texture_name(This->surfaces[j][i], This->baseTexture.srgbTextureName, TRUE);
                } else {
                    surface_set_texture_name(This->surfaces[j][i], This->baseTexture.textureName, FALSE);
                }
            }
        }
    }

    return hr;
}

static UINT WINAPI IWineD3DCubeTextureImpl_GetTextureDimensions(IWineD3DCubeTexture *iface){
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;
    TRACE("(%p)\n", This);

    return GL_TEXTURE_CUBE_MAP_ARB;
}

static BOOL WINAPI IWineD3DCubeTextureImpl_IsCondNP2(IWineD3DCubeTexture *iface) {
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;
    TRACE("(%p)\n", This);

    return FALSE;
}

static void WINAPI IWineD3DCubeTextureImpl_ApplyStateChanges(IWineD3DCubeTexture *iface, 
                                                        const DWORD textureStates[WINED3D_HIGHEST_TEXTURE_STATE + 1], 
                                                        const DWORD samplerStates[WINED3D_HIGHEST_SAMPLER_STATE + 1]) {
    TRACE("(%p) : relay to BaseTexture\n", iface);
    basetexture_apply_state_changes((IWineD3DBaseTexture *)iface, textureStates, samplerStates);
}


/* *******************************************
   IWineD3DCubeTexture IWineD3DCubeTexture parts follow
   ******************************************* */
static void WINAPI IWineD3DCubeTextureImpl_Destroy(IWineD3DCubeTexture *iface, D3DCB_DESTROYSURFACEFN D3DCB_DestroySurface) {
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;
    unsigned int i,j;
    TRACE("(%p) : Cleaning up\n",This);
    for (i = 0; i < This->baseTexture.levels; i++) {
        for (j = 0; j < 6; j++) {
            if (This->surfaces[j][i] != NULL) {
                IWineD3DSurface *surface = This->surfaces[j][i];
                /* Clean out the texture name we gave to the surface so that the surface doesn't try and release it */
                surface_set_texture_name(surface, 0, TRUE);
                surface_set_texture_name(surface, 0, FALSE);
                surface_set_texture_target(surface, 0);
                /* Cleanup the container */
                IWineD3DSurface_SetContainer(This->surfaces[j][i], 0);
                D3DCB_DestroySurface(This->surfaces[j][i]);
            }
        }
    }
    basetexture_cleanup((IWineD3DBaseTexture *)iface);
    /* finally delete the object */
    HeapFree(GetProcessHeap(), 0, This);
}

static HRESULT WINAPI IWineD3DCubeTextureImpl_GetLevelDesc(IWineD3DCubeTexture *iface, UINT Level, WINED3DSURFACE_DESC* pDesc) {
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;

    if (Level < This->baseTexture.levels) {
        TRACE("(%p) level (%d)\n", This, Level);
        return IWineD3DSurface_GetDesc(This->surfaces[0][Level], pDesc);
    }
    WARN("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->baseTexture.levels);
    return WINED3DERR_INVALIDCALL;
}

static HRESULT WINAPI IWineD3DCubeTextureImpl_GetCubeMapSurface(IWineD3DCubeTexture *iface, WINED3DCUBEMAP_FACES FaceType, UINT Level, IWineD3DSurface** ppCubeMapSurface) {
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;
    HRESULT hr = WINED3DERR_INVALIDCALL;

    if (Level < This->baseTexture.levels && FaceType <= WINED3DCUBEMAP_FACE_NEGATIVE_Z) {
        *ppCubeMapSurface = This->surfaces[FaceType][Level];
        IWineD3DSurface_AddRef(*ppCubeMapSurface);

        hr = WINED3D_OK;
    }
    if (WINED3D_OK == hr) {
        TRACE("(%p) -> faceType(%d) level(%d) returning surface@%p\n", This, FaceType, Level, This->surfaces[FaceType][Level]);
    } else {
        WARN("(%p) level(%d) overflow Levels(%d) Or FaceType(%d)\n", This, Level, This->baseTexture.levels, FaceType);
    }

    return hr;
}

static HRESULT WINAPI IWineD3DCubeTextureImpl_LockRect(IWineD3DCubeTexture *iface, WINED3DCUBEMAP_FACES FaceType, UINT Level, WINED3DLOCKED_RECT* pLockedRect, CONST RECT* pRect, DWORD Flags) {
    HRESULT hr = WINED3DERR_INVALIDCALL;
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;

    if (Level < This->baseTexture.levels && FaceType <= WINED3DCUBEMAP_FACE_NEGATIVE_Z) {
        hr = IWineD3DSurface_LockRect(This->surfaces[FaceType][Level], pLockedRect, pRect, Flags);
    }

    if (WINED3D_OK == hr) {
        TRACE("(%p) -> faceType(%d) level(%d) returning memory@%p success(%u)\n", This, FaceType, Level, pLockedRect->pBits, hr);
    } else {
        WARN("(%p) level(%d) overflow Levels(%d)  Or FaceType(%d)\n", This, Level, This->baseTexture.levels, FaceType);
    }

    return hr;
}

static HRESULT WINAPI IWineD3DCubeTextureImpl_UnlockRect(IWineD3DCubeTexture *iface, WINED3DCUBEMAP_FACES FaceType, UINT Level) {
    HRESULT hr = WINED3DERR_INVALIDCALL;
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;

    if (Level < This->baseTexture.levels && FaceType <= WINED3DCUBEMAP_FACE_NEGATIVE_Z) {
        hr = IWineD3DSurface_UnlockRect(This->surfaces[FaceType][Level]);
    }

    if (WINED3D_OK == hr) {
        TRACE("(%p) -> faceType(%d) level(%d) success(%u)\n", This, FaceType, Level, hr);
    } else {
        WARN("(%p) level(%d) overflow Levels(%d) Or FaceType(%d)\n", This, Level, This->baseTexture.levels, FaceType);
    }
    return hr;
}

static HRESULT  WINAPI IWineD3DCubeTextureImpl_AddDirtyRect(IWineD3DCubeTexture *iface, WINED3DCUBEMAP_FACES FaceType, CONST RECT* pDirtyRect) {
    HRESULT hr = WINED3DERR_INVALIDCALL;
    IWineD3DCubeTextureImpl *This = (IWineD3DCubeTextureImpl *)iface;
    This->baseTexture.dirty = TRUE;
    This->baseTexture.srgbDirty = TRUE;
    TRACE("(%p) : dirtyfication of faceType(%d) Level (0)\n", This, FaceType);
    if (FaceType <= WINED3DCUBEMAP_FACE_NEGATIVE_Z) {
        surface_add_dirty_rect(This->surfaces[FaceType][0], pDirtyRect);
        hr = WINED3D_OK;
    } else {
        WARN("(%p) overflow FaceType(%d)\n", This, FaceType);
    }
    return hr;
}


const IWineD3DCubeTextureVtbl IWineD3DCubeTexture_Vtbl =
{
    /* IUnknown */
    IWineD3DCubeTextureImpl_QueryInterface,
    IWineD3DCubeTextureImpl_AddRef,
    IWineD3DCubeTextureImpl_Release,
    /* IWineD3DResource */
    IWineD3DCubeTextureImpl_GetParent,
    IWineD3DCubeTextureImpl_GetDevice,
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
    IWineD3DCubeTextureImpl_ApplyStateChanges,
    /* IWineD3DCubeTexture */
    IWineD3DCubeTextureImpl_Destroy,
    IWineD3DCubeTextureImpl_GetLevelDesc,
    IWineD3DCubeTextureImpl_GetCubeMapSurface,
    IWineD3DCubeTextureImpl_LockRect,
    IWineD3DCubeTextureImpl_UnlockRect,
    IWineD3DCubeTextureImpl_AddDirtyRect
};
