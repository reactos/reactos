/*
 * IWineD3DBaseTexture Implementation
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2002-2004 Raphael Junqueira
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

HRESULT basetexture_init(IWineD3DBaseTextureImpl *texture, UINT layer_count, UINT level_count,
        WINED3DRESOURCETYPE resource_type, IWineD3DDeviceImpl *device, UINT size, DWORD usage,
        const struct wined3d_format_desc *format_desc, WINED3DPOOL pool, IUnknown *parent,
        const struct wined3d_parent_ops *parent_ops)
{
    HRESULT hr;

    hr = resource_init((IWineD3DResource *)texture, resource_type, device,
            size, usage, format_desc, pool, parent, parent_ops);
    if (FAILED(hr))
    {
        WARN("Failed to initialize resource, returning %#x\n", hr);
        return hr;
    }

    texture->baseTexture.sub_resources = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            level_count * layer_count * sizeof(*texture->baseTexture.sub_resources));
    if (!texture->baseTexture.sub_resources)
    {
        ERR("Failed to allocate sub-resource array.\n");
        resource_cleanup((IWineD3DResource *)texture);
        return E_OUTOFMEMORY;
    }

    texture->baseTexture.layer_count = layer_count;
    texture->baseTexture.level_count = level_count;
    texture->baseTexture.filterType = (usage & WINED3DUSAGE_AUTOGENMIPMAP) ? WINED3DTEXF_LINEAR : WINED3DTEXF_NONE;
    texture->baseTexture.LOD = 0;
    texture->baseTexture.texture_rgb.dirty = TRUE;
    texture->baseTexture.texture_srgb.dirty = TRUE;
    texture->baseTexture.is_srgb = FALSE;
    texture->baseTexture.pow2Matrix_identity = TRUE;

    if (texture->resource.format_desc->Flags & WINED3DFMT_FLAG_FILTERING)
    {
        texture->baseTexture.minMipLookup = minMipLookup;
        texture->baseTexture.magLookup = magLookup;
    }
    else
    {
        texture->baseTexture.minMipLookup = minMipLookup_noFilter;
        texture->baseTexture.magLookup = magLookup_noFilter;
    }

    return WINED3D_OK;
}

void basetexture_cleanup(IWineD3DBaseTexture *iface)
{
    basetexture_unload(iface);
    HeapFree(GetProcessHeap(), 0, ((IWineD3DBaseTextureImpl *)iface)->baseTexture.sub_resources);
    resource_cleanup((IWineD3DResource *)iface);
}

IWineD3DResourceImpl *basetexture_get_sub_resource(IWineD3DBaseTextureImpl *texture, UINT layer, UINT level)
{
    if (layer >= texture->baseTexture.layer_count)
    {
        WARN("layer %u >= layer_count %u.\n", layer, texture->baseTexture.layer_count);
        return NULL;
    }

    if (level >= texture->baseTexture.level_count)
    {
        WARN("level %u >= level_count %u.\n", level, texture->baseTexture.level_count);
        return NULL;
    }

    return texture->baseTexture.sub_resources[layer * texture->baseTexture.level_count + level];
}

/* A GL context is provided by the caller */
static void gltexture_delete(struct gl_texture *tex)
{
    ENTER_GL();
    glDeleteTextures(1, &tex->name);
    LEAVE_GL();
    tex->name = 0;
}

void basetexture_unload(IWineD3DBaseTexture *iface)
{
    IWineD3DTextureImpl *This = (IWineD3DTextureImpl *)iface;
    IWineD3DDeviceImpl *device = This->resource.device;
    struct wined3d_context *context = NULL;

    if (This->baseTexture.texture_rgb.name || This->baseTexture.texture_srgb.name)
    {
        context = context_acquire(device, NULL);
    }

    if(This->baseTexture.texture_rgb.name) {
        gltexture_delete(&This->baseTexture.texture_rgb);
    }
    if(This->baseTexture.texture_srgb.name) {
        gltexture_delete(&This->baseTexture.texture_srgb);
    }

    if (context) context_release(context);

    This->baseTexture.texture_rgb.dirty = TRUE;
    This->baseTexture.texture_srgb.dirty = TRUE;
}

DWORD basetexture_set_lod(IWineD3DBaseTexture *iface, DWORD LODNew)
{
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    DWORD old = This->baseTexture.LOD;

    /* The d3d9:texture test shows that SetLOD is ignored on non-managed
     * textures. The call always returns 0, and GetLOD always returns 0
     */
    if (This->resource.pool != WINED3DPOOL_MANAGED) {
        TRACE("Ignoring SetLOD on %s texture, returning 0\n", debug_d3dpool(This->resource.pool));
        return 0;
    }

    if (LODNew >= This->baseTexture.level_count)
        LODNew = This->baseTexture.level_count - 1;

    if(This->baseTexture.LOD != LODNew) {
        This->baseTexture.LOD = LODNew;

        This->baseTexture.texture_rgb.states[WINED3DTEXSTA_MAXMIPLEVEL] = ~0U;
        This->baseTexture.texture_srgb.states[WINED3DTEXSTA_MAXMIPLEVEL] = ~0U;
        if(This->baseTexture.bindCount) {
            IWineD3DDeviceImpl_MarkStateDirty(This->resource.device, STATE_SAMPLER(This->baseTexture.sampler));
        }
    }

    TRACE("(%p) : set LOD to %d\n", This, This->baseTexture.LOD);

    return old;
}

DWORD basetexture_get_lod(IWineD3DBaseTexture *iface)
{
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;

    TRACE("(%p) : returning %d\n", This, This->baseTexture.LOD);

    return This->baseTexture.LOD;
}

DWORD basetexture_get_level_count(IWineD3DBaseTexture *iface)
{
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    TRACE("iface %p, returning %u.\n", iface, This->baseTexture.level_count);
    return This->baseTexture.level_count;
}

HRESULT basetexture_set_autogen_filter_type(IWineD3DBaseTexture *iface, WINED3DTEXTUREFILTERTYPE FilterType)
{
  IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
  IWineD3DDeviceImpl *device = This->resource.device;
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
      struct wined3d_context *context = context_acquire(device, NULL);

      ENTER_GL();
      glBindTexture(textureDimensions, This->baseTexture.texture_rgb.name);
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

      context_release(context);
  }
  This->baseTexture.filterType = FilterType;
  TRACE("(%p) :\n", This);
  return WINED3D_OK;
}

WINED3DTEXTUREFILTERTYPE basetexture_get_autogen_filter_type(IWineD3DBaseTexture *iface)
{
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;

    FIXME("(%p) : stub\n", This);

    return This->baseTexture.filterType;
}

void basetexture_generate_mipmaps(IWineD3DBaseTexture *iface)
{
    /* TODO: Implement filters using GL_SGI_generate_mipmaps. */
    FIXME("iface %p stub!\n", iface);
}

BOOL basetexture_set_dirty(IWineD3DBaseTexture *iface, BOOL dirty)
{
    BOOL old;
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    old = This->baseTexture.texture_rgb.dirty || This->baseTexture.texture_srgb.dirty;
    This->baseTexture.texture_rgb.dirty = dirty;
    This->baseTexture.texture_srgb.dirty = dirty;
    return old;
}

BOOL basetexture_get_dirty(IWineD3DBaseTexture *iface)
{
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    return This->baseTexture.texture_rgb.dirty || This->baseTexture.texture_srgb.dirty;
}

/* Context activation is done by the caller. */
HRESULT basetexture_bind(IWineD3DBaseTexture *iface, BOOL srgb, BOOL *set_surface_desc)
{
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    HRESULT hr = WINED3D_OK;
    UINT textureDimensions;
    BOOL isNewTexture = FALSE;
    struct gl_texture *gl_tex;
    TRACE("(%p) : About to bind texture\n", This);

    This->baseTexture.is_srgb = srgb; /* SRGB mode cache for PreLoad calls outside drawprim */
    if(srgb) {
        gl_tex = &This->baseTexture.texture_srgb;
    } else {
        gl_tex = &This->baseTexture.texture_rgb;
    }

    textureDimensions = IWineD3DBaseTexture_GetTextureDimensions(iface);
    ENTER_GL();
    /* Generate a texture name if we don't already have one */
    if (gl_tex->name == 0) {
        *set_surface_desc = TRUE;
        glGenTextures(1, &gl_tex->name);
        checkGLcall("glGenTextures");
        TRACE("Generated texture %d\n", gl_tex->name);
        if (This->resource.pool == WINED3DPOOL_DEFAULT) {
            /* Tell opengl to try and keep this texture in video ram (well mostly) */
            GLclampf tmp;
            tmp = 0.9f;
            glPrioritizeTextures(1, &gl_tex->name, &tmp);

        }
        /* Initialise the state of the texture object
        to the openGL defaults, not the directx defaults */
        gl_tex->states[WINED3DTEXSTA_ADDRESSU]      = WINED3DTADDRESS_WRAP;
        gl_tex->states[WINED3DTEXSTA_ADDRESSV]      = WINED3DTADDRESS_WRAP;
        gl_tex->states[WINED3DTEXSTA_ADDRESSW]      = WINED3DTADDRESS_WRAP;
        gl_tex->states[WINED3DTEXSTA_BORDERCOLOR]   = 0;
        gl_tex->states[WINED3DTEXSTA_MAGFILTER]     = WINED3DTEXF_LINEAR;
        gl_tex->states[WINED3DTEXSTA_MINFILTER]     = WINED3DTEXF_POINT; /* GL_NEAREST_MIPMAP_LINEAR */
        gl_tex->states[WINED3DTEXSTA_MIPFILTER]     = WINED3DTEXF_LINEAR; /* GL_NEAREST_MIPMAP_LINEAR */
        gl_tex->states[WINED3DTEXSTA_MAXMIPLEVEL]   = 0;
        gl_tex->states[WINED3DTEXSTA_MAXANISOTROPY] = 1;
        gl_tex->states[WINED3DTEXSTA_SRGBTEXTURE]   = 0;
        gl_tex->states[WINED3DTEXSTA_SHADOW]        = FALSE;
        IWineD3DBaseTexture_SetDirty(iface, TRUE);
        isNewTexture = TRUE;

        if(This->resource.usage & WINED3DUSAGE_AUTOGENMIPMAP) {
            /* This means double binding the texture at creation, but keeps the code simpler all
             * in all, and the run-time path free from additional checks
             */
            glBindTexture(textureDimensions, gl_tex->name);
            checkGLcall("glBindTexture");
            glTexParameteri(textureDimensions, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
            checkGLcall("glTexParameteri(textureDimensions, GL_GENERATE_MIPMAP_SGIS, GL_TRUE)");
        }
    } else {
        *set_surface_desc = FALSE;
    }

    /* Bind the texture */
    if (gl_tex->name != 0) {
        glBindTexture(textureDimensions, gl_tex->name);
        checkGLcall("glBindTexture");
        if (isNewTexture) {
            /* For a new texture we have to set the textures levels after binding the texture.
             * In theory this is all we should ever have to do, but because ATI's drivers are broken, we
             * also need to set the texture dimensions before the texture is set
             * Beware that texture rectangles do not support mipmapping, but set the maxmiplevel if we're
             * relying on the partial GL_ARB_texture_non_power_of_two emulation with texture rectangles
             * (ie, do not care for cond_np2 here, just look for GL_TEXTURE_RECTANGLE_ARB)
             */
            if (textureDimensions != GL_TEXTURE_RECTANGLE_ARB)
            {
                TRACE("Setting GL_TEXTURE_MAX_LEVEL to %u.\n", This->baseTexture.level_count - 1);
                glTexParameteri(textureDimensions, GL_TEXTURE_MAX_LEVEL, This->baseTexture.level_count - 1);
                checkGLcall("glTexParameteri(textureDimensions, GL_TEXTURE_MAX_LEVEL, This->baseTexture.level_count)");
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

/* GL locking is done by the caller */
static void apply_wrap(const struct wined3d_gl_info *gl_info, GLenum target,
        WINED3DTEXTUREADDRESS d3d_wrap, GLenum param, BOOL cond_np2)
{
    GLint gl_wrap;

    if (d3d_wrap < WINED3DTADDRESS_WRAP || d3d_wrap > WINED3DTADDRESS_MIRRORONCE)
    {
        FIXME("Unrecognized or unsupported WINED3DTEXTUREADDRESS %#x.\n", d3d_wrap);
        return;
    }

    if (target == GL_TEXTURE_CUBE_MAP_ARB
            || (cond_np2 && d3d_wrap == WINED3DTADDRESS_WRAP))
    {
        /* Cubemaps are always set to clamp, regardless of the sampler state. */
        gl_wrap = GL_CLAMP_TO_EDGE;
    }
    else
    {
        gl_wrap = gl_info->wrap_lookup[d3d_wrap - WINED3DTADDRESS_WRAP];
    }

    TRACE("Setting param %#x to %#x for target %#x.\n", param, gl_wrap, target);
    glTexParameteri(target, param, gl_wrap);
    checkGLcall("glTexParameteri(target, param, gl_wrap)");
}

/* GL locking is done by the caller (state handler) */
void basetexture_apply_state_changes(IWineD3DBaseTexture *iface,
        const DWORD textureStates[WINED3D_HIGHEST_TEXTURE_STATE + 1],
        const DWORD samplerStates[WINED3D_HIGHEST_SAMPLER_STATE + 1],
        const struct wined3d_gl_info *gl_info)
{
    IWineD3DBaseTextureImpl *This = (IWineD3DBaseTextureImpl *)iface;
    DWORD state;
    GLint textureDimensions = IWineD3DBaseTexture_GetTextureDimensions(iface);
    BOOL cond_np2 = IWineD3DBaseTexture_IsCondNP2(iface);
    DWORD aniso;
    struct gl_texture *gl_tex;

    TRACE("iface %p, textureStates %p, samplerStates %p\n", iface, textureStates, samplerStates);

    if(This->baseTexture.is_srgb) {
        gl_tex = &This->baseTexture.texture_srgb;
    } else {
        gl_tex = &This->baseTexture.texture_rgb;
    }

    /* This function relies on the correct texture being bound and loaded. */

    if(samplerStates[WINED3DSAMP_ADDRESSU]      != gl_tex->states[WINED3DTEXSTA_ADDRESSU]) {
        state = samplerStates[WINED3DSAMP_ADDRESSU];
        apply_wrap(gl_info, textureDimensions, state, GL_TEXTURE_WRAP_S, cond_np2);
        gl_tex->states[WINED3DTEXSTA_ADDRESSU] = state;
    }

    if(samplerStates[WINED3DSAMP_ADDRESSV]      != gl_tex->states[WINED3DTEXSTA_ADDRESSV]) {
        state = samplerStates[WINED3DSAMP_ADDRESSV];
        apply_wrap(gl_info, textureDimensions, state, GL_TEXTURE_WRAP_T, cond_np2);
        gl_tex->states[WINED3DTEXSTA_ADDRESSV] = state;
    }

    if(samplerStates[WINED3DSAMP_ADDRESSW]      != gl_tex->states[WINED3DTEXSTA_ADDRESSW]) {
        state = samplerStates[WINED3DSAMP_ADDRESSW];
        apply_wrap(gl_info, textureDimensions, state, GL_TEXTURE_WRAP_R, cond_np2);
        gl_tex->states[WINED3DTEXSTA_ADDRESSW] = state;
    }

    if(samplerStates[WINED3DSAMP_BORDERCOLOR]   != gl_tex->states[WINED3DTEXSTA_BORDERCOLOR]) {
        float col[4];

        state = samplerStates[WINED3DSAMP_BORDERCOLOR];
        D3DCOLORTOGLFLOAT4(state, col);
        TRACE("Setting border color for %u to %x\n", textureDimensions, state);
        glTexParameterfv(textureDimensions, GL_TEXTURE_BORDER_COLOR, &col[0]);
        checkGLcall("glTexParameteri(..., GL_TEXTURE_BORDER_COLOR, ...)");
        gl_tex->states[WINED3DTEXSTA_BORDERCOLOR] = state;
    }

    if(samplerStates[WINED3DSAMP_MAGFILTER]     != gl_tex->states[WINED3DTEXSTA_MAGFILTER]) {
        GLint glValue;
        state = samplerStates[WINED3DSAMP_MAGFILTER];
        if (state > WINED3DTEXF_ANISOTROPIC) {
            FIXME("Unrecognized or unsupported MAGFILTER* value %d\n", state);
        }

        glValue = wined3d_gl_mag_filter(This->baseTexture.magLookup,
                min(max(state, WINED3DTEXF_POINT), WINED3DTEXF_LINEAR));
        TRACE("ValueMAG=%d setting MAGFILTER to %x\n", state, glValue);
        glTexParameteri(textureDimensions, GL_TEXTURE_MAG_FILTER, glValue);

        gl_tex->states[WINED3DTEXSTA_MAGFILTER] = state;
    }

    if((samplerStates[WINED3DSAMP_MINFILTER]     != gl_tex->states[WINED3DTEXSTA_MINFILTER] ||
        samplerStates[WINED3DSAMP_MIPFILTER]     != gl_tex->states[WINED3DTEXSTA_MIPFILTER] ||
        samplerStates[WINED3DSAMP_MAXMIPLEVEL]   != gl_tex->states[WINED3DTEXSTA_MAXMIPLEVEL])) {
        GLint glValue;

        gl_tex->states[WINED3DTEXSTA_MIPFILTER] = samplerStates[WINED3DSAMP_MIPFILTER];
        gl_tex->states[WINED3DTEXSTA_MINFILTER] = samplerStates[WINED3DSAMP_MINFILTER];
        gl_tex->states[WINED3DTEXSTA_MAXMIPLEVEL] = samplerStates[WINED3DSAMP_MAXMIPLEVEL];

        if (gl_tex->states[WINED3DTEXSTA_MINFILTER] > WINED3DTEXF_ANISOTROPIC
            || gl_tex->states[WINED3DTEXSTA_MIPFILTER] > WINED3DTEXF_ANISOTROPIC)
        {

            FIXME("Unrecognized or unsupported D3DSAMP_MINFILTER value %d D3DSAMP_MIPFILTER value %d\n",
                  gl_tex->states[WINED3DTEXSTA_MINFILTER],
                  gl_tex->states[WINED3DTEXSTA_MIPFILTER]);
        }
        glValue = wined3d_gl_min_mip_filter(This->baseTexture.minMipLookup,
                min(max(samplerStates[WINED3DSAMP_MINFILTER], WINED3DTEXF_POINT), WINED3DTEXF_LINEAR),
                min(max(samplerStates[WINED3DSAMP_MIPFILTER], WINED3DTEXF_NONE), WINED3DTEXF_LINEAR));

        TRACE("ValueMIN=%d, ValueMIP=%d, setting MINFILTER to %x\n",
              samplerStates[WINED3DSAMP_MINFILTER],
              samplerStates[WINED3DSAMP_MIPFILTER], glValue);
        glTexParameteri(textureDimensions, GL_TEXTURE_MIN_FILTER, glValue);
        checkGLcall("glTexParameter GL_TEXTURE_MIN_FILTER, ...");

        if (!cond_np2)
        {
            if (gl_tex->states[WINED3DTEXSTA_MIPFILTER] == WINED3DTEXF_NONE)
                glValue = This->baseTexture.LOD;
            else if (gl_tex->states[WINED3DTEXSTA_MAXMIPLEVEL] >= This->baseTexture.level_count)
                glValue = This->baseTexture.level_count - 1;
            else if (gl_tex->states[WINED3DTEXSTA_MAXMIPLEVEL] < This->baseTexture.LOD)
                /* baseTexture.LOD is already clamped in the setter */
                glValue = This->baseTexture.LOD;
            else
                glValue = gl_tex->states[WINED3DTEXSTA_MAXMIPLEVEL];

            /* Note that D3DSAMP_MAXMIPLEVEL specifies the biggest mipmap(default 0), while
             * GL_TEXTURE_MAX_LEVEL specifies the smallest mimap used(default 1000).
             * So D3DSAMP_MAXMIPLEVEL is the same as GL_TEXTURE_BASE_LEVEL.
             */
            glTexParameteri(textureDimensions, GL_TEXTURE_BASE_LEVEL, glValue);
        }
    }

    if ((gl_tex->states[WINED3DTEXSTA_MAGFILTER] != WINED3DTEXF_ANISOTROPIC
         && gl_tex->states[WINED3DTEXSTA_MINFILTER] != WINED3DTEXF_ANISOTROPIC
         && gl_tex->states[WINED3DTEXSTA_MIPFILTER] != WINED3DTEXF_ANISOTROPIC)
            || cond_np2)
    {
        aniso = 1;
    }
    else
    {
        aniso = samplerStates[WINED3DSAMP_MAXANISOTROPY];
    }

    if (gl_tex->states[WINED3DTEXSTA_MAXANISOTROPY] != aniso)
    {
        if (gl_info->supported[EXT_TEXTURE_FILTER_ANISOTROPIC])
        {
            glTexParameteri(textureDimensions, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
            checkGLcall("glTexParameteri(GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso)");
        }
        else
        {
            WARN("Anisotropic filtering not supported.\n");
        }
        gl_tex->states[WINED3DTEXSTA_MAXANISOTROPY] = aniso;
    }

    if (!(This->resource.format_desc->Flags & WINED3DFMT_FLAG_SHADOW)
            != !gl_tex->states[WINED3DTEXSTA_SHADOW])
    {
        if (This->resource.format_desc->Flags & WINED3DFMT_FLAG_SHADOW)
        {
            glTexParameteri(textureDimensions, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
            glTexParameteri(textureDimensions, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
            checkGLcall("glTexParameteri(textureDimensions, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB)");
            gl_tex->states[WINED3DTEXSTA_SHADOW] = TRUE;
        }
        else
        {
            glTexParameteri(textureDimensions, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
            checkGLcall("glTexParameteri(textureDimensions, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE)");
            gl_tex->states[WINED3DTEXSTA_SHADOW] = FALSE;
        }
    }
}
