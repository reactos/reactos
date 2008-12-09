/*
 * IWineD3DBaseTexture Implementation
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2002-2004 Raphael Junqueira
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

void basetexture_cleanup(IWineD3DBaseTexture *iface)
{
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;

    TRACE("(%p) : textureName(%d)\n", This, This->baseTexture.textureName);
    if (This->baseTexture.textureName != 0) {
        ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
        ENTER_GL();
        TRACE("(%p) : Deleting texture %d\n", This, This->baseTexture.textureName);
        glDeleteTextures(1, &This->baseTexture.textureName);
        LEAVE_GL();
    }

    resource_cleanup((IWineD3DResource *)iface);
}

void basetexture_unload(IWineD3DBaseTexture *iface)
{
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;

    if(This->baseTexture.textureName) {
        ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
        ENTER_GL();
        glDeleteTextures(1, &This->baseTexture.textureName);
        This->baseTexture.textureName = 0;
        LEAVE_GL();
    }
    This->baseTexture.dirty = TRUE;
}

/* There is no OpenGL equivalent of setLOD, getLOD. All they do anyway is prioritize texture loading
 * so just pretend that they work unless something really needs a failure. */
DWORD basetexture_set_lod(IWineD3DBaseTexture *iface, DWORD LODNew)
{
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

DWORD basetexture_get_lod(IWineD3DBaseTexture *iface)
{
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;

    if (This->resource.pool != WINED3DPOOL_MANAGED) {
        return  WINED3DERR_INVALIDCALL;
    }

    TRACE("(%p) : returning %d\n", This, This->baseTexture.LOD);

    return This->baseTexture.LOD;
}

DWORD basetexture_get_level_count(IWineD3DBaseTexture *iface)
{
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    TRACE("(%p) : returning %d\n", This, This->baseTexture.levels);
    return This->baseTexture.levels;
}

HRESULT basetexture_set_autogen_filter_type(IWineD3DBaseTexture *iface, WINED3DTEXTUREFILTERTYPE FilterType)
{
  IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
  IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
  UINT textureDimensions = IWineD3DBaseTexture_GetTextureDimensions(iface);

  if (!(This->resource.usage & WINED3DUSAGE_AUTOGENMIPMAP)) {
      TRACE("(%p) : returning invalid call\n", This);
      return WINED3DERR_INVALIDCALL;
  }
  if(This->baseTexture.filterType != FilterType) {
      /* What about multithreading? Do we want all the context overhead just to set this value?
       * Or should we delay the applying until the texture is used for drawing? For now, apply
       * immediately.
       */
      ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
      ENTER_GL();
      glBindTexture(textureDimensions, This->baseTexture.textureName);
      checkGLcall("glBindTexture");
      switch(FilterType) {
          case WINED3DTEXF_NONE:
          case WINED3DTEXF_POINT:
              glTexParameteri(textureDimensions, GL_GENERATE_MIPMAP_HINT_SGIS, GL_FASTEST);
              checkGLcall("glTexParameteri(textureDimensions, GL_GENERATE_MIPMAP_HINT_SGIS, GL_FASTEST)");

              break;
          case WINED3DTEXF_LINEAR:
              glTexParameteri(textureDimensions, GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);
              checkGLcall("glTexParameteri(textureDimensions, GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST)");

              break;
          default:
              WARN("Unexpected filter type %d, setting to GL_NICEST\n", FilterType);
              glTexParameteri(textureDimensions, GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);
              checkGLcall("glTexParameteri(textureDimensions, GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST)");
      }
      LEAVE_GL();
  }
  This->baseTexture.filterType = FilterType;
  TRACE("(%p) :\n", This);
  return WINED3D_OK;
}

WINED3DTEXTUREFILTERTYPE basetexture_get_autogen_filter_type(IWineD3DBaseTexture *iface)
{
  IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
  FIXME("(%p) : stub\n", This);
  if (!(This->resource.usage & WINED3DUSAGE_AUTOGENMIPMAP)) {
     return WINED3DTEXF_NONE;
  }
  return This->baseTexture.filterType;
}

void basetexture_generate_mipmaps(IWineD3DBaseTexture *iface)
{
  IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
  /* TODO: implement filters using GL_SGI_generate_mipmaps http://oss.sgi.com/projects/ogl-sample/registry/SGIS/generate_mipmap.txt */
  FIXME("(%p) : stub\n", This);
  return ;
}

BOOL basetexture_set_dirty(IWineD3DBaseTexture *iface, BOOL dirty)
{
    BOOL old;
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    old = This->baseTexture.dirty;
    This->baseTexture.dirty = dirty;
    return old;
}

BOOL basetexture_get_dirty(IWineD3DBaseTexture *iface)
{
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    return This->baseTexture.dirty;
}

HRESULT basetexture_bind(IWineD3DBaseTexture *iface)
{
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

        if(This->resource.usage & WINED3DUSAGE_AUTOGENMIPMAP) {
            /* This means double binding the texture at creation, but keeps the code simpler all
             * in all, and the run-time path free from additional checks
             */
            glBindTexture(textureDimensions, This->baseTexture.textureName);
            checkGLcall("glBindTexture");
            glTexParameteri(textureDimensions, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
            checkGLcall("glTexParameteri(textureDimensions, GL_GENERATE_MIPMAP_SGIS, GL_TRUE)");
        }
    }

    /* Bind the texture */
    if (This->baseTexture.textureName != 0) {
        glBindTexture(textureDimensions, This->baseTexture.textureName);
        checkGLcall("glBindTexture");
        if (isNewTexture) {
            /* For a new texture we have to set the textures levels after binding the texture.
             * In theory this is all we should ever have to do, but because ATI's drivers are broken, we
             * also need to set the texture dimensions before the texture is set
             * Beware that texture rectangles do not support mipmapping, but set the maxmiplevel if we're
             * relying on the partial GL_ARB_texture_non_power_of_two emulation with texture rectangles
             * (ie, do not care for cond_np2 here, just look for GL_TEXTURE_RECTANGLE_ARB)
             */
            if(textureDimensions != GL_TEXTURE_RECTANGLE_ARB) {
                TRACE("Setting GL_TEXTURE_MAX_LEVEL to %d\n", This->baseTexture.levels - 1);
                glTexParameteri(textureDimensions, GL_TEXTURE_MAX_LEVEL, This->baseTexture.levels - 1);
                checkGLcall("glTexParameteri(textureDimensions, GL_TEXTURE_MAX_LEVEL, This->baseTexture.levels)");
            }
            if(textureDimensions==GL_TEXTURE_CUBE_MAP_ARB) {
                /* Cubemaps are always set to clamp, regardless of the sampler state. */
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

static inline void apply_wrap(const GLint textureDimensions, const DWORD state, const GLint type,
                              BOOL cond_np2) {
    GLint wrapParm;

    if (state < minLookup[WINELOOKUP_WARPPARAM] || state > maxLookup[WINELOOKUP_WARPPARAM]) {
        FIXME("Unrecognized or unsupported WINED3DTADDRESS_U value %d\n", state);
    } else {
        if(textureDimensions==GL_TEXTURE_CUBE_MAP_ARB) {
            /* Cubemaps are always set to clamp, regardless of the sampler state. */
            wrapParm = GL_CLAMP_TO_EDGE;
        } else if(cond_np2) {
            if(state == WINED3DTADDRESS_WRAP) {
                wrapParm = GL_CLAMP_TO_EDGE;
            } else {
                wrapParm = stateLookup[WINELOOKUP_WARPPARAM][state - minLookup[WINELOOKUP_WARPPARAM]];
            }
        } else {
            wrapParm = stateLookup[WINELOOKUP_WARPPARAM][state - minLookup[WINELOOKUP_WARPPARAM]];
        }
        TRACE("Setting WRAP_S to %d for %x\n", wrapParm, textureDimensions);
        glTexParameteri(textureDimensions, type, wrapParm);
        checkGLcall("glTexParameteri(..., type, wrapParm)");
    }
}

void basetexture_apply_state_changes(IWineD3DBaseTexture *iface,
        const DWORD textureStates[WINED3D_HIGHEST_TEXTURE_STATE + 1],
        const DWORD samplerStates[WINED3D_HIGHEST_SAMPLER_STATE + 1])
{
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    DWORD state;
    GLint textureDimensions = IWineD3DBaseTexture_GetTextureDimensions(iface);
    BOOL cond_np2 = IWineD3DBaseTexture_IsCondNP2(iface);

    /* ApplyStateChanges relies on the correct texture being bound and loaded. */

    if(samplerStates[WINED3DSAMP_ADDRESSU]      != This->baseTexture.states[WINED3DTEXSTA_ADDRESSU]) {
        state = samplerStates[WINED3DSAMP_ADDRESSU];
        apply_wrap(textureDimensions, state, GL_TEXTURE_WRAP_S, cond_np2);
        This->baseTexture.states[WINED3DTEXSTA_ADDRESSU] = state;
    }

    if(samplerStates[WINED3DSAMP_ADDRESSV]      != This->baseTexture.states[WINED3DTEXSTA_ADDRESSV]) {
        state = samplerStates[WINED3DSAMP_ADDRESSV];
        apply_wrap(textureDimensions, state, GL_TEXTURE_WRAP_T, cond_np2);
        This->baseTexture.states[WINED3DTEXSTA_ADDRESSV] = state;
    }

    if(samplerStates[WINED3DSAMP_ADDRESSW]      != This->baseTexture.states[WINED3DTEXSTA_ADDRESSW]) {
        state = samplerStates[WINED3DSAMP_ADDRESSW];
        apply_wrap(textureDimensions, state, GL_TEXTURE_WRAP_R, cond_np2);
        This->baseTexture.states[WINED3DTEXSTA_ADDRESSW] = state;
    }

    if(samplerStates[WINED3DSAMP_BORDERCOLOR]   != This->baseTexture.states[WINED3DTEXSTA_BORDERCOLOR]) {
        float col[4];

        state = samplerStates[WINED3DSAMP_BORDERCOLOR];
        D3DCOLORTOGLFLOAT4(state, col);
        TRACE("Setting border color for %u to %x\n", textureDimensions, state);
        glTexParameterfv(textureDimensions, GL_TEXTURE_BORDER_COLOR, &col[0]);
        checkGLcall("glTexParameteri(..., GL_TEXTURE_BORDER_COLOR, ...)");
        This->baseTexture.states[WINED3DTEXSTA_BORDERCOLOR] = state;
    }

    if(samplerStates[WINED3DSAMP_MAGFILTER]     != This->baseTexture.states[WINED3DTEXSTA_MAGFILTER]) {
        GLint glValue;
        state = samplerStates[WINED3DSAMP_MAGFILTER];
        if (state > WINED3DTEXF_ANISOTROPIC) {
            FIXME("Unrecognized or unsupported MAGFILTER* value %d\n", state);
        } else {
            glValue = This->baseTexture.magLookup[state - WINED3DTEXF_NONE];
            TRACE("ValueMAG=%d setting MAGFILTER to %x\n", state, glValue);
            glTexParameteri(textureDimensions, GL_TEXTURE_MAG_FILTER, glValue);
            /* We need to reset the Anisotropic filtering state when we change the mag filter to WINED3DTEXF_ANISOTROPIC (this seems a bit weird, check the documentation to see how it should be switched off. */
            if (GL_SUPPORT(EXT_TEXTURE_FILTER_ANISOTROPIC) && WINED3DTEXF_ANISOTROPIC == state &&
                !cond_np2) {
                glTexParameteri(textureDimensions, GL_TEXTURE_MAX_ANISOTROPY_EXT, samplerStates[WINED3DSAMP_MAXANISOTROPY]);
            }
            This->baseTexture.states[WINED3DTEXSTA_MAGFILTER] = state;
        }
    }

    if((samplerStates[WINED3DSAMP_MINFILTER]     != This->baseTexture.states[WINED3DTEXSTA_MINFILTER] ||
        samplerStates[WINED3DSAMP_MIPFILTER]     != This->baseTexture.states[WINED3DTEXSTA_MIPFILTER] ||
        samplerStates[WINED3DSAMP_MAXMIPLEVEL]   != This->baseTexture.states[WINED3DTEXSTA_MAXMIPLEVEL])) {
        GLint glValue;

        This->baseTexture.states[WINED3DTEXSTA_MIPFILTER] = samplerStates[WINED3DSAMP_MIPFILTER];
        This->baseTexture.states[WINED3DTEXSTA_MINFILTER] = samplerStates[WINED3DSAMP_MINFILTER];
        This->baseTexture.states[WINED3DTEXSTA_MAXMIPLEVEL] = samplerStates[WINED3DSAMP_MAXMIPLEVEL];

        if (This->baseTexture.states[WINED3DTEXSTA_MINFILTER] > WINED3DTEXF_ANISOTROPIC ||
            This->baseTexture.states[WINED3DTEXSTA_MIPFILTER] > WINED3DTEXF_LINEAR)
        {

            FIXME("Unrecognized or unsupported D3DSAMP_MINFILTER value %d D3DSAMP_MIPFILTER value %d\n",
                  This->baseTexture.states[WINED3DTEXSTA_MINFILTER],
                  This->baseTexture.states[WINED3DTEXSTA_MIPFILTER]);
        }
        glValue = This->baseTexture.minMipLookup
                [min(max(samplerStates[WINED3DSAMP_MINFILTER],WINED3DTEXF_NONE), WINED3DTEXF_ANISOTROPIC)]
                .mip[min(max(samplerStates[WINED3DSAMP_MIPFILTER],WINED3DTEXF_NONE), WINED3DTEXF_LINEAR)];

        TRACE("ValueMIN=%d, ValueMIP=%d, setting MINFILTER to %x\n",
              samplerStates[WINED3DSAMP_MINFILTER],
              samplerStates[WINED3DSAMP_MIPFILTER], glValue);
        glTexParameteri(textureDimensions, GL_TEXTURE_MIN_FILTER, glValue);
        checkGLcall("glTexParameter GL_TEXTURE_MIN_FILTER, ...");

        if(!cond_np2) {
            if(This->baseTexture.states[WINED3DTEXSTA_MIPFILTER] == WINED3DTEXF_NONE) {
                glValue = 0;
            } else if(This->baseTexture.states[WINED3DTEXSTA_MAXMIPLEVEL] >= This->baseTexture.levels) {
                glValue = This->baseTexture.levels - 1;
            } else {
                glValue = This->baseTexture.states[WINED3DTEXSTA_MAXMIPLEVEL];
            }
            glTexParameteri(textureDimensions, GL_TEXTURE_BASE_LEVEL, glValue);
        }
    }

    if(samplerStates[WINED3DSAMP_MAXANISOTROPY] != This->baseTexture.states[WINED3DTEXSTA_MAXANISOTROPY]) {
        if (GL_SUPPORT(EXT_TEXTURE_FILTER_ANISOTROPIC) && !cond_np2) {
            glTexParameteri(textureDimensions, GL_TEXTURE_MAX_ANISOTROPY_EXT, samplerStates[WINED3DSAMP_MAXANISOTROPY]);
            checkGLcall("glTexParameteri GL_TEXTURE_MAX_ANISOTROPY_EXT ...");
        } else {
            WARN("Unsupported in local OpenGL implementation: glTexParameteri GL_TEXTURE_MAX_ANISOTROPY_EXT\n");
        }
        This->baseTexture.states[WINED3DTEXSTA_MAXANISOTROPY] = samplerStates[WINED3DSAMP_MAXANISOTROPY];
    }
}
