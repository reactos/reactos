/*
 * IWineD3DVolumeTexture implementation
 *
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
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
static HRESULT WINAPI IWineD3DVolumeTextureImpl_QueryInterface(IWineD3DVolumeTexture *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DResource)
        || IsEqualGUID(riid, &IID_IWineD3DBaseTexture)
        || IsEqualGUID(riid, &IID_IWineD3DVolumeTexture)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IWineD3DVolumeTextureImpl_AddRef(IWineD3DVolumeTexture *iface) {
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;
    TRACE("(%p) : AddRef increasing from %d\n", This, This->resource.ref);
    return InterlockedIncrement(&This->resource.ref);
}

static ULONG WINAPI IWineD3DVolumeTextureImpl_Release(IWineD3DVolumeTexture *iface) {
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;
    ULONG ref;
    TRACE("(%p) : Releasing from %d\n", This, This->resource.ref);
    ref = InterlockedDecrement(&This->resource.ref);
    if (ref == 0) {
        IWineD3DVolumeTexture_Destroy(iface, D3DCB_DefaultDestroyVolume);
    }
    return ref;
}

/* ****************************************************
   IWineD3DVolumeTexture IWineD3DResource parts follow
   **************************************************** */
static HRESULT WINAPI IWineD3DVolumeTextureImpl_GetDevice(IWineD3DVolumeTexture *iface, IWineD3DDevice** ppDevice) {
    return resource_get_device((IWineD3DResource *)iface, ppDevice);
}

static HRESULT WINAPI IWineD3DVolumeTextureImpl_SetPrivateData(IWineD3DVolumeTexture *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    return resource_set_private_data((IWineD3DResource *)iface, refguid, pData, SizeOfData, Flags);
}

static HRESULT WINAPI IWineD3DVolumeTextureImpl_GetPrivateData(IWineD3DVolumeTexture *iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    return resource_get_private_data((IWineD3DResource *)iface, refguid, pData, pSizeOfData);
}

static HRESULT WINAPI IWineD3DVolumeTextureImpl_FreePrivateData(IWineD3DVolumeTexture *iface, REFGUID refguid) {
    return resource_free_private_data((IWineD3DResource *)iface, refguid);
}

static DWORD WINAPI IWineD3DVolumeTextureImpl_SetPriority(IWineD3DVolumeTexture *iface, DWORD PriorityNew) {
    return resource_set_priority((IWineD3DResource *)iface, PriorityNew);
}

static DWORD WINAPI IWineD3DVolumeTextureImpl_GetPriority(IWineD3DVolumeTexture *iface) {
    return resource_get_priority((IWineD3DResource *)iface);
}

static void WINAPI IWineD3DVolumeTextureImpl_PreLoad(IWineD3DVolumeTexture *iface) {
    /* Overrider the IWineD3DResource Preload method */
    int i;
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
    BOOL srgb_mode = This->baseTexture.is_srgb;
    BOOL srgb_was_toggled = FALSE;

    TRACE("(%p) : About to load texture\n", This);

    if(!device->isInDraw) {
        ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
    } else if (GL_SUPPORT(EXT_TEXTURE_SRGB) && This->baseTexture.bindCount > 0) {
        srgb_mode = device->stateBlock->samplerState[This->baseTexture.sampler][WINED3DSAMP_SRGBTEXTURE];
        srgb_was_toggled = This->baseTexture.is_srgb != srgb_mode;
        This->baseTexture.is_srgb = srgb_mode;
    }

    /* If the texture is marked dirty or the srgb sampler setting has changed since the last load then reload the surfaces */
    if (This->baseTexture.dirty) {
        for (i = 0; i < This->baseTexture.levels; i++)
            IWineD3DVolume_LoadTexture(This->volumes[i], i, srgb_mode);
    } else if (srgb_was_toggled) {
        if (This->baseTexture.srgb_mode_change_count < 20)
            ++This->baseTexture.srgb_mode_change_count;
        else
            FIXME("Volumetexture (%p) has been reloaded at least 20 times due to WINED3DSAMP_SRGBTEXTURE changes on it\'s sampler\n", This);

        for (i = 0; i < This->baseTexture.levels; i++) {
            volume_add_dirty_box(This->volumes[i], NULL);
            IWineD3DVolume_LoadTexture(This->volumes[i], i, srgb_mode);
        }
    } else {
        TRACE("(%p) Texture not dirty, nothing to do\n" , iface);
    }

    /* No longer dirty */
    This->baseTexture.dirty = FALSE;

    return ;
}

static void WINAPI IWineD3DVolumeTextureImpl_UnLoad(IWineD3DVolumeTexture *iface) {
    unsigned int i;
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;
    TRACE("(%p)\n", This);

    /* Unload all the surfaces and reset the texture name. If UnLoad was called on the
     * surface before, this one will be a NOP and vice versa. Unloading an unloaded
     * surface is fine
     */
    for (i = 0; i < This->baseTexture.levels; i++) {
        IWineD3DVolume_UnLoad(This->volumes[i]);
    }

    basetexture_unload((IWineD3DBaseTexture *)iface);
}

static WINED3DRESOURCETYPE WINAPI IWineD3DVolumeTextureImpl_GetType(IWineD3DVolumeTexture *iface) {
    return resource_get_type((IWineD3DResource *)iface);
}

static HRESULT WINAPI IWineD3DVolumeTextureImpl_GetParent(IWineD3DVolumeTexture *iface, IUnknown **pParent) {
    return resource_get_parent((IWineD3DResource *)iface, pParent);
}

/* ******************************************************
   IWineD3DVolumeTexture IWineD3DBaseTexture parts follow
   ****************************************************** */
static DWORD WINAPI IWineD3DVolumeTextureImpl_SetLOD(IWineD3DVolumeTexture *iface, DWORD LODNew) {
    return basetexture_set_lod((IWineD3DBaseTexture *)iface, LODNew);
}

static DWORD WINAPI IWineD3DVolumeTextureImpl_GetLOD(IWineD3DVolumeTexture *iface) {
    return basetexture_get_lod((IWineD3DBaseTexture *)iface);
}

static DWORD WINAPI IWineD3DVolumeTextureImpl_GetLevelCount(IWineD3DVolumeTexture *iface) {
    return basetexture_get_level_count((IWineD3DBaseTexture *)iface);
}

static HRESULT WINAPI IWineD3DVolumeTextureImpl_SetAutoGenFilterType(IWineD3DVolumeTexture *iface, WINED3DTEXTUREFILTERTYPE FilterType) {
  return basetexture_set_autogen_filter_type((IWineD3DBaseTexture *)iface, FilterType);
}

static WINED3DTEXTUREFILTERTYPE WINAPI IWineD3DVolumeTextureImpl_GetAutoGenFilterType(IWineD3DVolumeTexture *iface) {
  return basetexture_get_autogen_filter_type((IWineD3DBaseTexture *)iface);
}

static void WINAPI IWineD3DVolumeTextureImpl_GenerateMipSubLevels(IWineD3DVolumeTexture *iface) {
    basetexture_generate_mipmaps((IWineD3DBaseTexture *)iface);
}

/* Internal function, No d3d mapping */
static BOOL WINAPI IWineD3DVolumeTextureImpl_SetDirty(IWineD3DVolumeTexture *iface, BOOL dirty) {
    return basetexture_set_dirty((IWineD3DBaseTexture *)iface, dirty);
}

static BOOL WINAPI IWineD3DVolumeTextureImpl_GetDirty(IWineD3DVolumeTexture *iface) {
    return basetexture_get_dirty((IWineD3DBaseTexture *)iface);
}

static HRESULT WINAPI IWineD3DVolumeTextureImpl_BindTexture(IWineD3DVolumeTexture *iface) {
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;
    TRACE("(%p) : relay to BaseTexture\n", This);
    return basetexture_bind((IWineD3DBaseTexture *)iface);
}

static UINT WINAPI IWineD3DVolumeTextureImpl_GetTextureDimensions(IWineD3DVolumeTexture *iface) {
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;
    TRACE("(%p)\n", This);
    return GL_TEXTURE_3D;
}

static BOOL WINAPI IWineD3DVolumeTextureImpl_IsCondNP2(IWineD3DVolumeTexture *iface) {
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;
    TRACE("(%p)\n", This);

    return FALSE;
}

static void WINAPI IWineD3DVolumeTextureImpl_ApplyStateChanges(IWineD3DVolumeTexture *iface,
                                                        const DWORD textureStates[WINED3D_HIGHEST_TEXTURE_STATE + 1],
                                                        const DWORD samplerStates[WINED3D_HIGHEST_SAMPLER_STATE + 1]) {
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;
    TRACE("(%p) : nothing to do, passing to base texture\n", This);
    basetexture_apply_state_changes((IWineD3DBaseTexture *)iface, textureStates, samplerStates);
}


/* *******************************************
   IWineD3DVolumeTexture IWineD3DVolumeTexture parts follow
   ******************************************* */
static void WINAPI IWineD3DVolumeTextureImpl_Destroy(IWineD3DVolumeTexture *iface, D3DCB_DESTROYVOLUMEFN D3DCB_DestroyVolume) {
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;
    int i;
    TRACE("(%p) : Cleaning up\n",This);
    for (i = 0; i < This->baseTexture.levels; i++) {
        if (This->volumes[i] != NULL) {
            /* Cleanup the container */
            IWineD3DVolume_SetContainer(This->volumes[i], 0);
            D3DCB_DestroyVolume(This->volumes[i]);
        }
    }
    basetexture_cleanup((IWineD3DBaseTexture *)iface);
    HeapFree(GetProcessHeap(), 0, This);
}

static HRESULT WINAPI IWineD3DVolumeTextureImpl_GetLevelDesc(IWineD3DVolumeTexture *iface, UINT Level,WINED3DVOLUME_DESC *pDesc) {
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;
    if (Level < This->baseTexture.levels) {
        TRACE("(%p) Level (%d)\n", This, Level);
        return IWineD3DVolume_GetDesc(This->volumes[Level], pDesc);
    } else {
        WARN("(%p) Level (%d)\n", This, Level);
    }
    return WINED3D_OK;
}
static HRESULT WINAPI IWineD3DVolumeTextureImpl_GetVolumeLevel(IWineD3DVolumeTexture *iface, UINT Level, IWineD3DVolume** ppVolumeLevel) {
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;
    if (Level < This->baseTexture.levels) {
      *ppVolumeLevel = This->volumes[Level];
      IWineD3DVolume_AddRef(*ppVolumeLevel);
      TRACE("(%p) -> level(%d) returning volume@%p\n", This, Level, *ppVolumeLevel);
    } else {
      WARN("(%p) Level(%d) overflow Levels(%d)\n", This, Level, This->baseTexture.levels);
      return WINED3DERR_INVALIDCALL;
    }
    return WINED3D_OK;

}
static HRESULT WINAPI IWineD3DVolumeTextureImpl_LockBox(IWineD3DVolumeTexture *iface, UINT Level, WINED3DLOCKED_BOX* pLockedVolume, CONST WINED3DBOX* pBox, DWORD Flags) {
    HRESULT hr;
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;

    if (Level < This->baseTexture.levels) {
      hr = IWineD3DVolume_LockBox(This->volumes[Level], pLockedVolume, pBox, Flags);
      TRACE("(%p) Level (%d) success(%u)\n", This, Level, hr);

    } else {
      FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->baseTexture.levels);
      return WINED3DERR_INVALIDCALL;
    }
    return hr;
}

static HRESULT WINAPI IWineD3DVolumeTextureImpl_UnlockBox(IWineD3DVolumeTexture *iface, UINT Level) {
    HRESULT hr;
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;

    if (Level < This->baseTexture.levels) {
      hr = IWineD3DVolume_UnlockBox(This->volumes[Level]);
      TRACE("(%p) -> level(%d) success(%u)\n", This, Level, hr);

    } else {
      FIXME("(%p) level(%d) overflow Levels(%d)\n", This, Level, This->baseTexture.levels);
      return WINED3DERR_INVALIDCALL;
    }
    return hr;
}

static HRESULT WINAPI IWineD3DVolumeTextureImpl_AddDirtyBox(IWineD3DVolumeTexture *iface, CONST WINED3DBOX* pDirtyBox) {
    IWineD3DVolumeTextureImpl *This = (IWineD3DVolumeTextureImpl *)iface;
    This->baseTexture.dirty = TRUE;
    TRACE("(%p) : dirtyfication of volume Level (0)\n", This);
    volume_add_dirty_box(This->volumes[0], pDirtyBox);

    return WINED3D_OK;
}

const IWineD3DVolumeTextureVtbl IWineD3DVolumeTexture_Vtbl =
{
    /* IUnknown */
    IWineD3DVolumeTextureImpl_QueryInterface,
    IWineD3DVolumeTextureImpl_AddRef,
    IWineD3DVolumeTextureImpl_Release,
    /* resource */
    IWineD3DVolumeTextureImpl_GetParent,
    IWineD3DVolumeTextureImpl_GetDevice,
    IWineD3DVolumeTextureImpl_SetPrivateData,
    IWineD3DVolumeTextureImpl_GetPrivateData,
    IWineD3DVolumeTextureImpl_FreePrivateData,
    IWineD3DVolumeTextureImpl_SetPriority,
    IWineD3DVolumeTextureImpl_GetPriority,
    IWineD3DVolumeTextureImpl_PreLoad,
    IWineD3DVolumeTextureImpl_UnLoad,
    IWineD3DVolumeTextureImpl_GetType,
    /* BaseTexture */
    IWineD3DVolumeTextureImpl_SetLOD,
    IWineD3DVolumeTextureImpl_GetLOD,
    IWineD3DVolumeTextureImpl_GetLevelCount,
    IWineD3DVolumeTextureImpl_SetAutoGenFilterType,
    IWineD3DVolumeTextureImpl_GetAutoGenFilterType,
    IWineD3DVolumeTextureImpl_GenerateMipSubLevels,
    IWineD3DVolumeTextureImpl_SetDirty,
    IWineD3DVolumeTextureImpl_GetDirty,
    /* not in d3d */
    IWineD3DVolumeTextureImpl_BindTexture,
    IWineD3DVolumeTextureImpl_GetTextureDimensions,
    IWineD3DVolumeTextureImpl_IsCondNP2,
    IWineD3DVolumeTextureImpl_ApplyStateChanges,
    /* volume texture */
    IWineD3DVolumeTextureImpl_Destroy,
    IWineD3DVolumeTextureImpl_GetLevelDesc,
    IWineD3DVolumeTextureImpl_GetVolumeLevel,
    IWineD3DVolumeTextureImpl_LockBox,
    IWineD3DVolumeTextureImpl_UnlockBox,
    IWineD3DVolumeTextureImpl_AddDirtyBox
};
