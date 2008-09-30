/*
 * IWineD3DTexture implementation
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
   IWineD3DTexture IUnknown parts follow
   ******************************************* */
static HRESULT WINAPI IWineD3DTextureImpl_QueryInterface(IWineD3DTexture *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DResource)
        || IsEqualGUID(riid, &IID_IWineD3DBaseTexture)
        || IsEqualGUID(riid, &IID_IWineD3DTexture)){
        IUnknown_AddRef(iface);
        *ppobj = This;
        return WINED3D_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IWineD3DTextureImpl_AddRef(IWineD3DTexture *iface) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    TRACE("(%p) : AddRef increasing from %d\n", This, This->resource.ref);
    return InterlockedIncrement(&This->resource.ref);
}

static ULONG WINAPI IWineD3DTextureImpl_Release(IWineD3DTexture *iface) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %d\n", This, This->resource.ref);
    ref = InterlockedDecrement(&This->resource.ref);
    if (ref == 0) {
        IWineD3DTexture_Destroy(iface, D3DCB_DefaultDestroySurface);
    }
    return ref;
}


/* ****************************************************
   IWineD3DTexture IWineD3DResource parts follow
   **************************************************** */
static HRESULT WINAPI IWineD3DTextureImpl_GetDevice(IWineD3DTexture *iface, IWineD3DDevice** ppDevice) {
    return IWineD3DResourceImpl_GetDevice((IWineD3DResource *)iface, ppDevice);
}

static HRESULT WINAPI IWineD3DTextureImpl_SetPrivateData(IWineD3DTexture *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    return IWineD3DResourceImpl_SetPrivateData((IWineD3DResource *)iface, refguid, pData, SizeOfData, Flags);
}

static HRESULT WINAPI IWineD3DTextureImpl_GetPrivateData(IWineD3DTexture *iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    return IWineD3DResourceImpl_GetPrivateData((IWineD3DResource *)iface, refguid, pData, pSizeOfData);
}

static HRESULT WINAPI IWineD3DTextureImpl_FreePrivateData(IWineD3DTexture *iface, REFGUID refguid) {
    return IWineD3DResourceImpl_FreePrivateData((IWineD3DResource *)iface, refguid);
}

static DWORD WINAPI IWineD3DTextureImpl_SetPriority(IWineD3DTexture *iface, DWORD PriorityNew) {
    return IWineD3DResourceImpl_SetPriority((IWineD3DResource *)iface, PriorityNew);
}

static DWORD WINAPI IWineD3DTextureImpl_GetPriority(IWineD3DTexture *iface) {
    return IWineD3DResourceImpl_GetPriority((IWineD3DResource *)iface);
}

static void WINAPI IWineD3DTextureImpl_PreLoad(IWineD3DTexture *iface) {

    /* Override the IWineD3DResource PreLoad method */
    unsigned int i;
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
    BOOL srgb_mode = This->baseTexture.is_srgb;
    BOOL srgb_was_toggled = FALSE;

    TRACE("(%p) : About to load texture\n", This);

    if(!device->isInDraw) {
        /* ActivateContext sets isInDraw to TRUE when loading a pbuffer into a texture, thus no danger of
         * recursive calls
         */
        ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
    } else if (GL_SUPPORT(EXT_TEXTURE_SRGB) && This->baseTexture.bindCount > 0) {
        srgb_mode = device->stateBlock->samplerState[This->baseTexture.sampler][WINED3DSAMP_SRGBTEXTURE];
        srgb_was_toggled = This->baseTexture.is_srgb != srgb_mode;
        This->baseTexture.is_srgb = srgb_mode;
    }

    if (This->resource.format == WINED3DFMT_P8 || This->resource.format == WINED3DFMT_A8P8) {
        for (i = 0; i < This->baseTexture.levels; i++) {
            if(palette9_changed((IWineD3DSurfaceImpl *)This->surfaces[i])) {
                TRACE("Reloading surface because the d3d8/9 palette was changed\n");
                /* TODO: This is not necessarily needed with hw palettized texture support */
                IWineD3DSurface_LoadLocation(This->surfaces[i], SFLAG_INSYSMEM, NULL);
                /* Make sure the texture is reloaded because of the palette change, this kills performance though :( */
                IWineD3DSurface_ModifyLocation(This->surfaces[i], SFLAG_INTEXTURE, FALSE);
            }
        }
    }
    /* If the texture is marked dirty or the srgb sampler setting has changed since the last load then reload the surfaces */
    if (This->baseTexture.dirty) {
        for (i = 0; i < This->baseTexture.levels; i++) {
            IWineD3DSurface_LoadTexture(This->surfaces[i], srgb_mode);
        }
    } else if (srgb_was_toggled) {
        if (This->baseTexture.srgb_mode_change_count < 20)
            ++This->baseTexture.srgb_mode_change_count;
        else
            FIXME("Texture (%p) has been reloaded at least 20 times due to WINED3DSAMP_SRGBTEXTURE changes on it\'s sampler\n", This);

        for (i = 0; i < This->baseTexture.levels; i++) {
            IWineD3DSurface_AddDirtyRect(This->surfaces[i], NULL);
            IWineD3DSurface_SetGlTextureDesc(This->surfaces[i], This->baseTexture.textureName, IWineD3DTexture_GetTextureDimensions(iface));
            IWineD3DSurface_LoadTexture(This->surfaces[i], srgb_mode);
        }
    } else {
        TRACE("(%p) Texture not dirty, nothing to do\n" , iface);
    }

    /* No longer dirty */
    This->baseTexture.dirty = FALSE;

    return ;
}

static void WINAPI IWineD3DTextureImpl_UnLoad(IWineD3DTexture *iface) {
    unsigned int i;
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    TRACE("(%p)\n", This);

    /* Unload all the surfaces and reset the texture name. If UnLoad was called on the
     * surface before, this one will be a NOP and vice versa. Unloading an unloaded
     * surface is fine
     */
    for (i = 0; i < This->baseTexture.levels; i++) {
        IWineD3DSurface_UnLoad(This->surfaces[i]);
        IWineD3DSurface_SetGlTextureDesc(This->surfaces[i], 0, IWineD3DTexture_GetTextureDimensions(iface));
    }

    IWineD3DBaseTextureImpl_UnLoad((IWineD3DBaseTexture *) iface);
}

static WINED3DRESOURCETYPE WINAPI IWineD3DTextureImpl_GetType(IWineD3DTexture *iface) {
    return IWineD3DResourceImpl_GetType((IWineD3DResource *)iface);
}

static HRESULT WINAPI IWineD3DTextureImpl_GetParent(IWineD3DTexture *iface, IUnknown **pParent) {
    return IWineD3DResourceImpl_GetParent((IWineD3DResource *)iface, pParent);
}

/* ******************************************************
   IWineD3DTexture IWineD3DBaseTexture parts follow
   ****************************************************** */
static DWORD WINAPI IWineD3DTextureImpl_SetLOD(IWineD3DTexture *iface, DWORD LODNew) {
    return IWineD3DBaseTextureImpl_SetLOD((IWineD3DBaseTexture *)iface, LODNew);
}

static DWORD WINAPI IWineD3DTextureImpl_GetLOD(IWineD3DTexture *iface) {
    return IWineD3DBaseTextureImpl_GetLOD((IWineD3DBaseTexture *)iface);
}

static DWORD WINAPI IWineD3DTextureImpl_GetLevelCount(IWineD3DTexture *iface) {
    return IWineD3DBaseTextureImpl_GetLevelCount((IWineD3DBaseTexture *)iface);
}

static HRESULT WINAPI IWineD3DTextureImpl_SetAutoGenFilterType(IWineD3DTexture *iface, WINED3DTEXTUREFILTERTYPE FilterType) {
  return IWineD3DBaseTextureImpl_SetAutoGenFilterType((IWineD3DBaseTexture *)iface, FilterType);
}

static WINED3DTEXTUREFILTERTYPE WINAPI IWineD3DTextureImpl_GetAutoGenFilterType(IWineD3DTexture *iface) {
  return IWineD3DBaseTextureImpl_GetAutoGenFilterType((IWineD3DBaseTexture *)iface);
}

static void WINAPI IWineD3DTextureImpl_GenerateMipSubLevels(IWineD3DTexture *iface) {
    IWineD3DBaseTextureImpl_GenerateMipSubLevels((IWineD3DBaseTexture *)iface);
}

/* Internal function, No d3d mapping */
static BOOL WINAPI IWineD3DTextureImpl_SetDirty(IWineD3DTexture *iface, BOOL dirty) {
    return IWineD3DBaseTextureImpl_SetDirty((IWineD3DBaseTexture *)iface, dirty);
}

static BOOL WINAPI IWineD3DTextureImpl_GetDirty(IWineD3DTexture *iface) {
    return IWineD3DBaseTextureImpl_GetDirty((IWineD3DBaseTexture *)iface);
}

static HRESULT WINAPI IWineD3DTextureImpl_BindTexture(IWineD3DTexture *iface) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    BOOL set_gl_texture_desc = This->baseTexture.textureName == 0;
    HRESULT hr;

    TRACE("(%p) : relay to BaseTexture\n", This);

    hr = IWineD3DBaseTextureImpl_BindTexture((IWineD3DBaseTexture *)iface);
    if (set_gl_texture_desc && SUCCEEDED(hr)) {
        UINT i;
        for (i = 0; i < This->baseTexture.levels; ++i) {
            IWineD3DSurface_SetGlTextureDesc(This->surfaces[i], This->baseTexture.textureName, IWineD3DTexture_GetTextureDimensions(iface));
        }
        /* Conditinal non power of two textures use a different clamping default. If we're using the GL_WINE_normalized_texrect
         * partial driver emulation, we're dealing with a GL_TEXTURE_2D texture which has the address mode set to repeat - something
         * that prevents us from hitting the accelerated codepath. Thus manually set the GL state. The same applies to filtering.
         * Even if the texture has only one mip level, the default LINEAR_MIPMAP_LINEAR filter causes a SW fallback on macos.
         */
        if(IWineD3DBaseTexture_IsCondNP2(iface)) {
            ENTER_GL();
            glTexParameteri(IWineD3DTexture_GetTextureDimensions(iface), GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            checkGLcall("glTexParameteri(dimension, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)");
            glTexParameteri(IWineD3DTexture_GetTextureDimensions(iface), GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            checkGLcall("glTexParameteri(dimension, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)");
            glTexParameteri(IWineD3DTexture_GetTextureDimensions(iface), GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            checkGLcall("glTexParameteri(dimension, GL_TEXTURE_MIN_FILTER, GL_NEAREST)");
            glTexParameteri(IWineD3DTexture_GetTextureDimensions(iface), GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            checkGLcall("glTexParameteri(dimension, GL_TEXTURE_MAG_FILTER, GL_NEAREST)");
            LEAVE_GL();
            This->baseTexture.states[WINED3DTEXSTA_ADDRESSU]      = WINED3DTADDRESS_CLAMP;
            This->baseTexture.states[WINED3DTEXSTA_ADDRESSV]      = WINED3DTADDRESS_CLAMP;
            This->baseTexture.states[WINED3DTEXSTA_MAGFILTER]     = WINED3DTEXF_POINT;
            This->baseTexture.states[WINED3DTEXSTA_MINFILTER]     = WINED3DTEXF_POINT;
            This->baseTexture.states[WINED3DTEXSTA_MIPFILTER]     = WINED3DTEXF_NONE;
        }
    }

    return hr;
}

static UINT WINAPI IWineD3DTextureImpl_GetTextureDimensions(IWineD3DTexture *iface) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    TRACE("(%p)\n", This);

    return This->target;
}

static BOOL WINAPI IWineD3DTextureImpl_IsCondNP2(IWineD3DTexture *iface) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    TRACE("(%p)\n", This);

    return This->cond_np2;
}

static void WINAPI IWineD3DTextureImpl_ApplyStateChanges(IWineD3DTexture *iface,
                                                   const DWORD textureStates[WINED3D_HIGHEST_TEXTURE_STATE + 1],
                                                   const DWORD samplerStates[WINED3D_HIGHEST_SAMPLER_STATE + 1]) {
    TRACE("(%p) : relay to BaseTexture\n", iface);
    IWineD3DBaseTextureImpl_ApplyStateChanges((IWineD3DBaseTexture *)iface, textureStates, samplerStates);
}

/* *******************************************
   IWineD3DTexture IWineD3DTexture parts follow
   ******************************************* */
static void WINAPI IWineD3DTextureImpl_Destroy(IWineD3DTexture *iface, D3DCB_DESTROYSURFACEFN D3DCB_DestroySurface) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    int i;

    TRACE("(%p) : Cleaning up\n",This);
    for (i = 0; i < This->baseTexture.levels; i++) {
        if (This->surfaces[i] != NULL) {
            /* Clean out the texture name we gave to the surface so that the surface doesn't try and release it */
            IWineD3DSurface_SetGlTextureDesc(This->surfaces[i], 0, 0);
            IWineD3DSurface_SetContainer(This->surfaces[i], 0);
            D3DCB_DestroySurface(This->surfaces[i]);
        }
    }
    TRACE("(%p) : cleaning up base texture\n", This);
    IWineD3DBaseTextureImpl_CleanUp((IWineD3DBaseTexture *)iface);
    /* free the object */
    HeapFree(GetProcessHeap(), 0, This);
}

static HRESULT WINAPI IWineD3DTextureImpl_GetLevelDesc(IWineD3DTexture *iface, UINT Level, WINED3DSURFACE_DESC* pDesc) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;

    if (Level < This->baseTexture.levels) {
        TRACE("(%p) Level (%d)\n", This, Level);
        return IWineD3DSurface_GetDesc(This->surfaces[Level], pDesc);
    }
    FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->baseTexture.levels);
    return WINED3DERR_INVALIDCALL;
}

static HRESULT WINAPI IWineD3DTextureImpl_GetSurfaceLevel(IWineD3DTexture *iface, UINT Level, IWineD3DSurface** ppSurfaceLevel) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    HRESULT hr = WINED3DERR_INVALIDCALL;

    if (Level < This->baseTexture.levels) {
        *ppSurfaceLevel = This->surfaces[Level];
        IWineD3DSurface_AddRef(This->surfaces[Level]);
        hr = WINED3D_OK;
        TRACE("(%p) : returning %p for level %d\n", This, *ppSurfaceLevel, Level);
    }
    if (WINED3D_OK != hr) {
        WARN("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->baseTexture.levels);
        *ppSurfaceLevel = NULL; /* Just to be on the safe side.. */
    }
    return hr;
}

static HRESULT WINAPI IWineD3DTextureImpl_LockRect(IWineD3DTexture *iface, UINT Level, WINED3DLOCKED_RECT *pLockedRect,
                                            CONST RECT *pRect, DWORD Flags) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    HRESULT hr = WINED3DERR_INVALIDCALL;

    if (Level < This->baseTexture.levels) {
        hr = IWineD3DSurface_LockRect(This->surfaces[Level], pLockedRect, pRect, Flags);
    }
    if (WINED3D_OK == hr) {
        TRACE("(%p) Level (%d) success\n", This, Level);
    } else {
        WARN("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->baseTexture.levels);
    }

    return hr;
}

static HRESULT WINAPI IWineD3DTextureImpl_UnlockRect(IWineD3DTexture *iface, UINT Level) {
   IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    HRESULT hr = WINED3DERR_INVALIDCALL;

    if (Level < This->baseTexture.levels) {
        hr = IWineD3DSurface_UnlockRect(This->surfaces[Level]);
    }
    if ( WINED3D_OK == hr) {
        TRACE("(%p) Level (%d) success\n", This, Level);
    } else {
        WARN("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->baseTexture.levels);
    }
    return hr;
}

static HRESULT WINAPI IWineD3DTextureImpl_AddDirtyRect(IWineD3DTexture *iface, CONST RECT* pDirtyRect) {
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    This->baseTexture.dirty = TRUE;
    TRACE("(%p) : dirtyfication of surface Level (0)\n", This);
    return IWineD3DSurface_AddDirtyRect(This->surfaces[0], pDirtyRect);
}

const IWineD3DTextureVtbl IWineD3DTexture_Vtbl =
{
    /* IUnknown */
    IWineD3DTextureImpl_QueryInterface,
    IWineD3DTextureImpl_AddRef,
    IWineD3DTextureImpl_Release,
    /* IWineD3DResource */
    IWineD3DTextureImpl_GetParent,
    IWineD3DTextureImpl_GetDevice,
    IWineD3DTextureImpl_SetPrivateData,
    IWineD3DTextureImpl_GetPrivateData,
    IWineD3DTextureImpl_FreePrivateData,
    IWineD3DTextureImpl_SetPriority,
    IWineD3DTextureImpl_GetPriority,
    IWineD3DTextureImpl_PreLoad,
    IWineD3DTextureImpl_UnLoad,
    IWineD3DTextureImpl_GetType,
    /* IWineD3DBaseTexture */
    IWineD3DTextureImpl_SetLOD,
    IWineD3DTextureImpl_GetLOD,
    IWineD3DTextureImpl_GetLevelCount,
    IWineD3DTextureImpl_SetAutoGenFilterType,
    IWineD3DTextureImpl_GetAutoGenFilterType,
    IWineD3DTextureImpl_GenerateMipSubLevels,
    IWineD3DTextureImpl_SetDirty,
    IWineD3DTextureImpl_GetDirty,
    IWineD3DTextureImpl_BindTexture,
    IWineD3DTextureImpl_GetTextureDimensions,
    IWineD3DTextureImpl_IsCondNP2,
    IWineD3DTextureImpl_ApplyStateChanges,
    /* IWineD3DTexture */
    IWineD3DTextureImpl_Destroy,
    IWineD3DTextureImpl_GetLevelDesc,
    IWineD3DTextureImpl_GetSurfaceLevel,
    IWineD3DTextureImpl_LockRect,
    IWineD3DTextureImpl_UnlockRect,
    IWineD3DTextureImpl_AddDirtyRect
};
