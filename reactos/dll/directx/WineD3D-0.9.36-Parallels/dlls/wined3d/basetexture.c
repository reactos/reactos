/*
 * IWineD3DBaseTexture Implementation
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2002-2004 Raphael Junqueira
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
#define GLINFO_LOCATION ((IWineD3DImpl *)(((IWineD3DDeviceImpl *)This->resource.wineD3DDevice)->wineD3D))->gl_info

static const Wined3dTextureStateMap textureObjectSamplerStates[]  = {
    {WINED3DSAMP_ADDRESSU,      WINED3DSAMP_ADDRESSU},
    {WINED3DSAMP_ADDRESSV,      WINED3DSAMP_ADDRESSV},
    {WINED3DSAMP_ADDRESSW,      WINED3DSAMP_ADDRESSW},
/* NOTE: Sometimes it's a good idea to disable the setting of border colour, e.g. Axis and Allies */
    {WINED3DSAMP_BORDERCOLOR,   WINED3DFUNC_NOTSUPPORTED/* WINED3DSAMP_BORDERCOLOR */},
    {WINED3DSAMP_MAGFILTER,     WINED3DSAMP_MAGFILTER},
    {WINED3DSAMP_MINFILTER,     WINED3DSAMP_MINFILTER},
    {WINED3DSAMP_MIPFILTER,     WINED3DSAMP_MIPFILTER},
/* applies to the texture unit
    WINED3DSAMP_MIPMAPLODBIAS, WINED3DSAMP_MIPMAPLODBIAS,
*/
    {WINED3DSAMP_MAXMIPLEVEL,   WINED3DSAMP_MAXMIPLEVEL},
#if 0
    {WINED3DSAMP_MAXANISOTROPY, GL_SUPPORTED(EXT_TEXTURE_FILTER_ANISOTROPIC) ? WINED3DSAMP_MAXANISOTROPY : WINED3DFUNC_NOTSUPPORTED},
#else
    {WINED3DSAMP_MAXANISOTROPY, WINED3DSAMP_MAXANISOTROPY},
#endif
    {WINED3DSAMP_SRGBTEXTURE,   WINED3DFUNC_UNIMPLEMENTED},
    {WINED3DSAMP_ELEMENTINDEX,  WINED3DFUNC_UNIMPLEMENTED},
    {WINED3DSAMP_DMAPOFFSET,    WINED3DFUNC_UNIMPLEMENTED},
    {-1, 0}
};

static const Wined3dTextureStateMap textureObjectTextureStates[] = {
    {WINED3DTSS_ADDRESSW , WINED3DTSS_ADDRESSW},
    {-1, 0}
};

/* *******************************************
   IWineD3DBaseTexture IUnknown parts follow
   ******************************************* */
HRESULT WINAPI IWineD3DBaseTextureImpl_QueryInterface(IWineD3DBaseTexture *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DResource)
        || IsEqualGUID(riid, &IID_IWineD3DBaseTexture)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DBaseTextureImpl_AddRef(IWineD3DBaseTexture *iface) {
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->resource.ref);

    TRACE("(%p) : AddRef increasing from %d\n", This,ref - 1);
    return ref;
}

ULONG WINAPI IWineD3DBaseTextureImpl_Release(IWineD3DBaseTexture *iface) {
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->resource.ref);
    TRACE("(%p) : Releasing from %d\n", This, ref + 1);
    if (ref == 0) {
        IWineD3DBaseTextureImpl_CleanUp(iface);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* class static */
void IWineD3DBaseTextureImpl_CleanUp(IWineD3DBaseTexture *iface) {
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;

    TRACE("(%p) : textureName(%d)\n", This, This->baseTexture.textureName);
    if (This->baseTexture.textureName != 0) {
        ENTER_GL();
        ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
        TRACE("(%p) : Deleting texture %d\n", This, This->baseTexture.textureName);
        glDeleteTextures(1, &This->baseTexture.textureName);
        LEAVE_GL();
    }
    IWineD3DResourceImpl_CleanUp((IWineD3DResource *)iface);
}

/* ****************************************************
   IWineD3DBaseTexture IWineD3DResource parts follow
   **************************************************** */
HRESULT WINAPI IWineD3DBaseTextureImpl_GetDevice(IWineD3DBaseTexture *iface, IWineD3DDevice** ppDevice) {
    return IWineD3DResourceImpl_GetDevice((IWineD3DResource *)iface, ppDevice);
}

HRESULT WINAPI IWineD3DBaseTextureImpl_SetPrivateData(IWineD3DBaseTexture *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    return IWineD3DResourceImpl_SetPrivateData((IWineD3DResource *)iface, refguid, pData, SizeOfData, Flags);
}

HRESULT WINAPI IWineD3DBaseTextureImpl_GetPrivateData(IWineD3DBaseTexture *iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    return IWineD3DResourceImpl_GetPrivateData((IWineD3DResource *)iface, refguid, pData, pSizeOfData);
}

HRESULT WINAPI IWineD3DBaseTextureImpl_FreePrivateData(IWineD3DBaseTexture *iface, REFGUID refguid) {
    return IWineD3DResourceImpl_FreePrivateData((IWineD3DResource *)iface, refguid);
}

DWORD    WINAPI        IWineD3DBaseTextureImpl_SetPriority(IWineD3DBaseTexture *iface, DWORD PriorityNew) {
    return IWineD3DResourceImpl_SetPriority((IWineD3DResource *)iface, PriorityNew);
}

DWORD    WINAPI        IWineD3DBaseTextureImpl_GetPriority(IWineD3DBaseTexture *iface) {
    return IWineD3DResourceImpl_GetPriority((IWineD3DResource *)iface);
}

void     WINAPI        IWineD3DBaseTextureImpl_PreLoad(IWineD3DBaseTexture *iface) {
    /*return*/ IWineD3DResourceImpl_PreLoad((IWineD3DResource *)iface);
}

WINED3DRESOURCETYPE WINAPI IWineD3DBaseTextureImpl_GetType(IWineD3DBaseTexture *iface) {
    return IWineD3DResourceImpl_GetType((IWineD3DResource *)iface);
}

HRESULT WINAPI IWineD3DBaseTextureImpl_GetParent(IWineD3DBaseTexture *iface, IUnknown **pParent) {
    return IWineD3DResourceImpl_GetParent((IWineD3DResource *)iface, pParent);
}

/* ******************************************************
   IWineD3DBaseTexture IWineD3DBaseTexture parts follow
   ****************************************************** */

/* There is no OpenGL equivilent of setLOD, getLOD, all they do it priortise testure loading
 * so just pretend that they work unless something really needs a failure. */
DWORD WINAPI IWineD3DBaseTextureImpl_SetLOD(IWineD3DBaseTexture *iface, DWORD LODNew) {
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;

    if (This->resource.pool != WINED3DPOOL_MANAGED) {
        return  WINED3DERR_INVALIDCALL;
    }

    if(LODNew >= This->baseTexture.levels)
        LODNew = This->baseTexture.levels - 1;
     This->baseTexture.LOD = LODNew;

    TRACE("(%p) : set bogus LOD to %d\n", This, This->baseTexture.LOD);

    return This->baseTexture.LOD;
}

DWORD WINAPI IWineD3DBaseTextureImpl_GetLOD(IWineD3DBaseTexture *iface) {
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;

    if (This->resource.pool != WINED3DPOOL_MANAGED) {
        return  WINED3DERR_INVALIDCALL;
    }

    TRACE("(%p) : returning %d\n", This, This->baseTexture.LOD);

    return This->baseTexture.LOD;
}

DWORD WINAPI IWineD3DBaseTextureImpl_GetLevelCount(IWineD3DBaseTexture *iface) {
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    TRACE("(%p) : returning %d\n", This, This->baseTexture.levels);
    return This->baseTexture.levels;
}

HRESULT WINAPI IWineD3DBaseTextureImpl_SetAutoGenFilterType(IWineD3DBaseTexture *iface, WINED3DTEXTUREFILTERTYPE FilterType) {
  IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;

  if (!(This->baseTexture.usage & WINED3DUSAGE_AUTOGENMIPMAP)) {
      TRACE("(%p) : returning invalid call\n", This);
      return WINED3DERR_INVALIDCALL;
  }
  This->baseTexture.filterType = FilterType;
  TRACE("(%p) :\n", This);
  return WINED3D_OK;
}

WINED3DTEXTUREFILTERTYPE WINAPI IWineD3DBaseTextureImpl_GetAutoGenFilterType(IWineD3DBaseTexture *iface) {
  IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
  FIXME("(%p) : stub\n", This);
  if (!(This->baseTexture.usage & WINED3DUSAGE_AUTOGENMIPMAP)) {
     return WINED3DTEXF_NONE;
  }
  return This->baseTexture.filterType;
}

void WINAPI IWineD3DBaseTextureImpl_GenerateMipSubLevels(IWineD3DBaseTexture *iface) {
  IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
  /* TODO: implement filters using GL_SGI_generate_mipmaps http://oss.sgi.com/projects/ogl-sample/registry/SGIS/generate_mipmap.txt */
  FIXME("(%p) : stub\n", This);
  return ;
}

/* Internal function, No d3d mapping */
BOOL WINAPI IWineD3DBaseTextureImpl_SetDirty(IWineD3DBaseTexture *iface, BOOL dirty) {
    BOOL old;
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    old = This->baseTexture.dirty;
    This->baseTexture.dirty = dirty;
    return old;
}

BOOL WINAPI IWineD3DBaseTextureImpl_GetDirty(IWineD3DBaseTexture *iface) {
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    return This->baseTexture.dirty;
}

HRESULT WINAPI IWineD3DBaseTextureImpl_BindTexture(IWineD3DBaseTexture *iface) {
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    HRESULT hr = WINED3D_OK;
    UINT textureDimensions;
    BOOL isNewTexture = FALSE;
    TRACE("(%p) : About to bind texture\n", This);

    textureDimensions = IWineD3DBaseTexture_GetTextureDimensions(iface);
    ENTER_GL();
    /* Generate a texture name if we don't already have one */
    if (This->baseTexture.textureName == 0) {
        glGenTextures(1, &This->baseTexture.textureName);
        checkGLcall("glGenTextures");
        TRACE("Generated texture %d\n", This->baseTexture.textureName);
        if (This->resource.pool == WINED3DPOOL_DEFAULT) {
            /* Tell opengl to try and keep this texture in video ram (well mostly) */
            GLclampf tmp;
            tmp = 0.9f;
            glPrioritizeTextures(1, &This->baseTexture.textureName, &tmp);

        }
        /* Initialise the state of the texture object
        to the openGL defaults, not the directx defaults */
        This->baseTexture.states[WINED3DTEXSTA_ADDRESSU]      = WINED3DTADDRESS_WRAP;
        This->baseTexture.states[WINED3DTEXSTA_ADDRESSV]      = WINED3DTADDRESS_WRAP;
        This->baseTexture.states[WINED3DTEXSTA_ADDRESSW]      = WINED3DTADDRESS_WRAP;
        This->baseTexture.states[WINED3DTEXSTA_BORDERCOLOR]   = 0;
        This->baseTexture.states[WINED3DTEXSTA_MAGFILTER]     = WINED3DTEXF_LINEAR;
        This->baseTexture.states[WINED3DTEXSTA_MINFILTER]     = WINED3DTEXF_POINT; /* GL_NEAREST_MIPMAP_LINEAR */
        This->baseTexture.states[WINED3DTEXSTA_MIPFILTER]     = WINED3DTEXF_LINEAR; /* GL_NEAREST_MIPMAP_LINEAR */
        This->baseTexture.states[WINED3DTEXSTA_MAXMIPLEVEL]   = 0;
        This->baseTexture.states[WINED3DTEXSTA_MAXANISOTROPY] = 0;
        This->baseTexture.states[WINED3DTEXSTA_SRGBTEXTURE]   = 0;
        This->baseTexture.states[WINED3DTEXSTA_ELEMENTINDEX]  = 0;
        This->baseTexture.states[WINED3DTEXSTA_DMAPOFFSET]    = 0;
        This->baseTexture.states[WINED3DTEXSTA_TSSADDRESSW]   = WINED3DTADDRESS_WRAP;
        IWineD3DBaseTexture_SetDirty(iface, TRUE);
        isNewTexture = TRUE;
    }

    /* Bind the texture */
    if (This->baseTexture.textureName != 0) {
        glBindTexture(textureDimensions, This->baseTexture.textureName);
        checkGLcall("glBindTexture");
        if (isNewTexture) {
            /* For a new texture we have to set the textures levels after binding the texture.
            * In theory this is all we should ever have to do, but because ATI's drivers are broken, we
            * also need to set the texture dimensions before the texture is set */
            TRACE("Setting GL_TEXTURE_MAX_LEVEL to %d\n", This->baseTexture.levels - 1);
            glTexParameteri(textureDimensions, GL_TEXTURE_MAX_LEVEL, This->baseTexture.levels - 1);
            checkGLcall("glTexParameteri(textureDimensions, GL_TEXTURE_MAX_LEVEL, This->baseTexture.levels)");
            if(textureDimensions==GL_TEXTURE_CUBE_MAP_ARB) {
                /* Cubemaps are always set to clamp, regardeless of the sampler state. */
                glTexParameteri(textureDimensions, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(textureDimensions, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(textureDimensions, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            }
        }
		
    } else { /* this only happened if we've run out of openGL textures */
        WARN("This texture doesn't have an openGL texture assigned to it\n");
        hr =  WINED3DERR_INVALIDCALL;
    }

    LEAVE_GL();
    return hr;
}

HRESULT WINAPI IWineD3DBaseTextureImpl_UnBindTexture(IWineD3DBaseTexture *iface) {
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    UINT textureDimensions;

    TRACE("(%p) : About to bind texture\n", This);
    textureDimensions = IWineD3DBaseTexture_GetTextureDimensions(iface);

    ENTER_GL();

    glBindTexture(textureDimensions, 0);
#if 0 /* TODO: context manager support */
     IWineD3DContextManager_PopState(This->contextManager, textureDimensions, ENABLED, NOW /* make sure the state is applied now */);
#else
    glDisable(textureDimensions);
#endif

    LEAVE_GL();
    return WINED3D_OK;
}

UINT WINAPI IWineD3DBaseTextureImpl_GetTextureDimensions(IWineD3DBaseTexture *iface){
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    FIXME("(%p) : This shouldn't be called\n", This);
    return WINED3D_OK;
}

static inline GLenum warpLookupType(WINED3DSAMPLERSTATETYPE Type) {
    switch(Type) {
    case WINED3DSAMP_ADDRESSU:
        return GL_TEXTURE_WRAP_S;
    case WINED3DSAMP_ADDRESSV:
        return GL_TEXTURE_WRAP_T;
    case WINED3DSAMP_ADDRESSW:
        return GL_TEXTURE_WRAP_R;
    default:
        FIXME("Unexpected warp type %d\n", Type);
        return 0;
    }
}

void WINAPI IWineD3DBaseTextureImpl_ApplyStateChanges(IWineD3DBaseTexture *iface,
                                    const DWORD textureStates[WINED3D_HIGHEST_TEXTURE_STATE + 1],
                                    const DWORD samplerStates[WINED3D_HIGHEST_SAMPLER_STATE + 1]) {
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    int i;
    DWORD *state = This->baseTexture.states;
    GLint textureDimensions = IWineD3DBaseTexture_GetTextureDimensions(iface);
    IWineD3DBaseTexture_PreLoad(iface);
    /* run through a couple of loops and apply and states that are different */
    /* this will reduce the number of texture state changes to an absolute minimum
    for multi-parameter states we  pickup the first one that changes, work out the correct values for the other states
    and set all the states that we've just applied to their new values */

    for (i = 0 ;textureObjectSamplerStates[i].state != -1; i++) {
        if (*state != samplerStates[textureObjectSamplerStates[i].state]) {
            /* apply the state */
            TRACE("(%p) : Changing state %u from %d to %d\n", This, i, *state , samplerStates[textureObjectSamplerStates[i].state]);
            switch (textureObjectSamplerStates[i].function) {
            case WINED3DSAMP_ADDRESSU:
            case WINED3DSAMP_ADDRESSV: /* fall through */
            case WINED3DSAMP_ADDRESSW: /* fall through */
                *state = samplerStates[textureObjectSamplerStates[i].state];
                if (*state < minLookup[WINELOOKUP_WARPPARAM] || *state > maxLookup[WINELOOKUP_WARPPARAM]) {
                    FIXME("Unrecognized or unsupported WINED3DTADDRESS_* value %d, state %d\n", *state, textureObjectSamplerStates[i].function);
                } else {
                    GLint wrapParm;
                    if(textureDimensions==GL_TEXTURE_CUBE_MAP_ARB) {
                        /* Cubemaps are always set to clamp, regardeless of the sampler state. */
                        wrapParm = GL_CLAMP_TO_EDGE;
                    } else {
                        wrapParm = stateLookup[WINELOOKUP_WARPPARAM][*state - minLookup[WINELOOKUP_WARPPARAM]];
                    }
                    TRACE("Setting WRAP_R to %d for %x\n", wrapParm, textureDimensions);
                    glTexParameteri(textureDimensions, warpLookupType(textureObjectSamplerStates[i].function), wrapParm);
                    checkGLcall("glTexParameteri(..., GL_TEXTURE_WRAP_R, wrapParm)");
                }
            break;
            case WINED3DSAMP_BORDERCOLOR:
            {
                float col[4];
                *state = samplerStates[textureObjectSamplerStates[i].state];
                D3DCOLORTOGLFLOAT4(*state, col);
                TRACE("Setting border color for %u to %x\n", textureDimensions, *state);
                glTexParameterfv(textureDimensions, GL_TEXTURE_BORDER_COLOR, &col[0]);
                checkGLcall("glTexParameteri(..., GL_TEXTURE_BORDER_COLOR, ...)");
            }
            break;
            case WINED3DSAMP_MAGFILTER:
                {
                    GLint glValue;
                    *state = samplerStates[textureObjectSamplerStates[i].state];
                    if (*state < minLookup[WINELOOKUP_MAGFILTER] || *state > maxLookup[WINELOOKUP_MAGFILTER]) {
                        FIXME("Unrecognized or unsupported MAGFILTER* value %d, state %d\n", *state, textureObjectSamplerStates[i].function);
                    }
                    glValue = stateLookup[WINELOOKUP_MAGFILTER][*state - minLookup[WINELOOKUP_MAGFILTER]];
                    TRACE("ValueMAG=%d setting MAGFILTER to %x\n", *state, glValue);
                    glTexParameteri(textureDimensions, GL_TEXTURE_MAG_FILTER, glValue);
                /* We need to reset the Aniotropic filtering state when we change the mag filter to WINED3DTEXF_ANISOTROPIC (this seems a bit weird, check the documentataion to see how it should be switched off. */
                    if (GL_SUPPORT(EXT_TEXTURE_FILTER_ANISOTROPIC) && WINED3DTEXF_ANISOTROPIC == *state) {
                        glTexParameteri(textureDimensions, GL_TEXTURE_MAX_ANISOTROPY_EXT, samplerStates[WINED3DSAMP_MAXANISOTROPY]);
                    }
                }
            break;

            case WINED3DSAMP_MINFILTER:
                This->baseTexture.states[WINED3DTEXSTA_MIPFILTER] = samplerStates[WINED3DSAMP_MIPFILTER];
            case WINED3DSAMP_MIPFILTER: /* fall through */
                {
                    GLint glValue;
                    *state = samplerStates[textureObjectSamplerStates[i].state];
                    if (This->baseTexture.states[WINED3DTEXSTA_MINFILTER] < WINED3DTEXF_NONE ||
                        This->baseTexture.states[WINED3DTEXSTA_MIPFILTER] < WINED3DTEXF_NONE ||
                        This->baseTexture.states[WINED3DTEXSTA_MINFILTER] > WINED3DTEXF_ANISOTROPIC ||
                        This->baseTexture.states[WINED3DTEXSTA_MIPFILTER] > WINED3DTEXF_LINEAR)
                    {

                        FIXME("Unrecognized or unsupported D3DSAMP_MINFILTER value %d, state %d D3DSAMP_MIPFILTER value %d, state %d\n",
                                This->baseTexture.states[WINED3DTEXSTA_MINFILTER],
                                textureObjectSamplerStates[WINED3DTEXSTA_MINFILTER].function,
                                This->baseTexture.states[WINED3DTEXSTA_MIPFILTER],
                                textureObjectSamplerStates[WINED3DTEXSTA_MIPFILTER].function);
                    }
                    glValue = minMipLookup[min(max(This->baseTexture.states[WINED3DTEXSTA_MINFILTER],WINED3DTEXF_NONE), WINED3DTEXF_ANISOTROPIC)]
                                                [min(max(This->baseTexture.states[WINED3DTEXSTA_MIPFILTER],WINED3DTEXF_NONE), WINED3DTEXF_LINEAR)];

                    TRACE("ValueMIN=%d, ValueMIP=%d, setting MINFILTER to %x\n", 
                            This->baseTexture.states[WINED3DTEXSTA_MINFILTER], 
                            This->baseTexture.states[WINED3DTEXSTA_MIPFILTER], glValue);
                    glTexParameteri(textureDimensions, GL_TEXTURE_MIN_FILTER, glValue);
                    checkGLcall("glTexParameter GL_TEXTURE_MIN_FILTER, ...");
                }
            break;
            case WINED3DSAMP_MAXMIPLEVEL:
                *state = samplerStates[textureObjectSamplerStates[i].state];
                /**
                * Not really the same, but the more apprioprate than nothing
                */
                glTexParameteri(textureDimensions, GL_TEXTURE_BASE_LEVEL, *state);
            break;
            case WINED3DSAMP_MAXANISOTROPY:
                *state = samplerStates[textureObjectSamplerStates[i].state];
                if (GL_SUPPORT(EXT_TEXTURE_FILTER_ANISOTROPIC)) {
                    glTexParameteri(textureDimensions, GL_TEXTURE_MAX_ANISOTROPY_EXT,  *state);
                    checkGLcall("glTexParameteri GL_TEXTURE_MAX_ANISOTROPY_EXT ...");
                } else {
                    WARN("Unsupported in local OpenGL implementation: glTexParameteri GL_TEXTURE_MAX_ANISOTROPY_EXT\n");
                }
            break;
            case WINED3DFUNC_UNIMPLEMENTED: /* unimplemented */
                TRACE("(%p) : stub\n", This);
                *state = samplerStates[textureObjectSamplerStates[i].state];
            break;
            case WINED3DFUNC_NOTSUPPORTED: /* nop */
                TRACE("(%p) : %s function is not supported by this opengl implementation\n", This, "unknown" /* TODO: replace with debug_blah... */);
                *state = samplerStates[textureObjectSamplerStates[i].state];
            break;
            }
        }
        state++;
    }
}


static const IWineD3DBaseTextureVtbl IWineD3DBaseTexture_Vtbl =
{
    /* IUnknown */
    IWineD3DBaseTextureImpl_QueryInterface,
    IWineD3DBaseTextureImpl_AddRef,
    IWineD3DBaseTextureImpl_Release,
    /* IWineD3DResource */
    IWineD3DBaseTextureImpl_GetParent,
    IWineD3DBaseTextureImpl_GetDevice,
    IWineD3DBaseTextureImpl_SetPrivateData,
    IWineD3DBaseTextureImpl_GetPrivateData,
    IWineD3DBaseTextureImpl_FreePrivateData,
    IWineD3DBaseTextureImpl_SetPriority,
    IWineD3DBaseTextureImpl_GetPriority,
    IWineD3DBaseTextureImpl_PreLoad,
    IWineD3DBaseTextureImpl_GetType,
    /*IWineD3DBaseTexture*/
    IWineD3DBaseTextureImpl_SetLOD,
    IWineD3DBaseTextureImpl_GetLOD,
    IWineD3DBaseTextureImpl_GetLevelCount,
    IWineD3DBaseTextureImpl_SetAutoGenFilterType,
    IWineD3DBaseTextureImpl_GetAutoGenFilterType,
    IWineD3DBaseTextureImpl_GenerateMipSubLevels,
    IWineD3DBaseTextureImpl_SetDirty,
    IWineD3DBaseTextureImpl_GetDirty,
    /* internal */
    IWineD3DBaseTextureImpl_BindTexture,
    IWineD3DBaseTextureImpl_UnBindTexture,
    IWineD3DBaseTextureImpl_GetTextureDimensions,
    IWineD3DBaseTextureImpl_ApplyStateChanges

};
