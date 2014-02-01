/*
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2005 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
 * Copyright 2007-2009, 2013 Stefan Dösinger for CodeWeavers
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

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d_texture);

static HRESULT wined3d_texture_init(struct wined3d_texture *texture, const struct wined3d_texture_ops *texture_ops,
        UINT layer_count, UINT level_count, const struct wined3d_resource_desc *desc, struct wined3d_device *device,
        void *parent, const struct wined3d_parent_ops *parent_ops, const struct wined3d_resource_ops *resource_ops)
{
    const struct wined3d_format *format = wined3d_get_format(&device->adapter->gl_info, desc->format);
    HRESULT hr;

    TRACE("texture %p, texture_ops %p, layer_count %u, level_count %u, resource_type %s, format %s, "
            "multisample_type %#x, multisample_quality %#x, usage %s, pool %s, width %u, height %u, depth %u, "
            "device %p, parent %p, parent_ops %p, resource_ops %p.\n",
            texture, texture_ops, layer_count, level_count, debug_d3dresourcetype(desc->resource_type),
            debug_d3dformat(desc->format), desc->multisample_type, desc->multisample_quality,
            debug_d3dusage(desc->usage), debug_d3dpool(desc->pool), desc->width, desc->height, desc->depth,
            device, parent, parent_ops, resource_ops);

    if ((format->flags & (WINED3DFMT_FLAG_BLOCKS | WINED3DFMT_FLAG_BLOCKS_NO_VERIFY)) == WINED3DFMT_FLAG_BLOCKS)
    {
        UINT width_mask = format->block_width - 1;
        UINT height_mask = format->block_height - 1;
        if (desc->width & width_mask || desc->height & height_mask)
            return WINED3DERR_INVALIDCALL;
    }

    if (FAILED(hr = resource_init(&texture->resource, device, desc->resource_type, format,
            desc->multisample_type, desc->multisample_quality, desc->usage, desc->pool,
            desc->width, desc->height, desc->depth, 0, parent, parent_ops, resource_ops)))
    {
        WARN("Failed to initialize resource, returning %#x\n", hr);
        return hr;
    }

    texture->texture_ops = texture_ops;
    texture->sub_resources = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            level_count * layer_count * sizeof(*texture->sub_resources));
    if (!texture->sub_resources)
    {
        ERR("Failed to allocate sub-resource array.\n");
        resource_cleanup(&texture->resource);
        return E_OUTOFMEMORY;
    }

    texture->layer_count = layer_count;
    texture->level_count = level_count;
    texture->filter_type = (desc->usage & WINED3DUSAGE_AUTOGENMIPMAP) ? WINED3D_TEXF_LINEAR : WINED3D_TEXF_NONE;
    texture->lod = 0;
    texture->flags = WINED3D_TEXTURE_POW2_MAT_IDENT;

    if (texture->resource.format->flags & WINED3DFMT_FLAG_FILTERING)
    {
        texture->min_mip_lookup = minMipLookup;
        texture->mag_lookup = magLookup;
    }
    else
    {
        texture->min_mip_lookup = minMipLookup_noFilter;
        texture->mag_lookup = magLookup_noFilter;
    }

    return WINED3D_OK;
}

/* A GL context is provided by the caller */
static void gltexture_delete(const struct wined3d_gl_info *gl_info, struct gl_texture *tex)
{
    gl_info->gl_ops.gl.p_glDeleteTextures(1, &tex->name);
    tex->name = 0;
}

static void wined3d_texture_unload_gl_texture(struct wined3d_texture *texture)
{
    struct wined3d_device *device = texture->resource.device;
    struct wined3d_context *context = NULL;

    if (texture->texture_rgb.name || texture->texture_srgb.name)
    {
        context = context_acquire(device, NULL);
    }

    if (texture->texture_rgb.name)
        gltexture_delete(context->gl_info, &texture->texture_rgb);

    if (texture->texture_srgb.name)
        gltexture_delete(context->gl_info, &texture->texture_srgb);

    if (context) context_release(context);

    wined3d_texture_set_dirty(texture);

    resource_unload(&texture->resource);
}

static void wined3d_texture_cleanup(struct wined3d_texture *texture)
{
    UINT sub_count = texture->level_count * texture->layer_count;
    UINT i;

    TRACE("texture %p.\n", texture);

    for (i = 0; i < sub_count; ++i)
    {
        struct wined3d_resource *sub_resource = texture->sub_resources[i];

        if (sub_resource)
            texture->texture_ops->texture_sub_resource_cleanup(sub_resource);
    }

    wined3d_texture_unload_gl_texture(texture);
    HeapFree(GetProcessHeap(), 0, texture->sub_resources);
    resource_cleanup(&texture->resource);
}

void wined3d_texture_set_dirty(struct wined3d_texture *texture)
{
    texture->flags &= ~(WINED3D_TEXTURE_RGB_VALID | WINED3D_TEXTURE_SRGB_VALID);
}

/* Context activation is done by the caller. */
void wined3d_texture_bind(struct wined3d_texture *texture,
        struct wined3d_context *context, BOOL srgb)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct gl_texture *gl_tex;
    GLenum target;

    TRACE("texture %p, context %p, srgb %#x.\n", texture, context, srgb);

    if (gl_info->supported[EXT_TEXTURE_SRGB_DECODE])
        srgb = FALSE;

    /* sRGB mode cache for preload() calls outside drawprim. */
    if (srgb)
        texture->flags |= WINED3D_TEXTURE_IS_SRGB;
    else
        texture->flags &= ~WINED3D_TEXTURE_IS_SRGB;

    gl_tex = wined3d_texture_get_gl_texture(texture, srgb);
    target = texture->target;

    if (gl_tex->name)
    {
        context_bind_texture(context, target, gl_tex->name);
        return;
    }

    gl_info->gl_ops.gl.p_glGenTextures(1, &gl_tex->name);
    checkGLcall("glGenTextures");
    TRACE("Generated texture %d.\n", gl_tex->name);

    if (!gl_tex->name)
    {
        ERR("Failed to generate a texture name.\n");
        return;
    }

    if (texture->resource.pool == WINED3D_POOL_DEFAULT)
    {
        /* Tell OpenGL to try and keep this texture in video ram (well mostly). */
        GLclampf tmp = 0.9f;
        gl_info->gl_ops.gl.p_glPrioritizeTextures(1, &gl_tex->name, &tmp);
    }

    /* Initialise the state of the texture object to the OpenGL defaults, not
     * the wined3d defaults. */
    gl_tex->states[WINED3DTEXSTA_ADDRESSU] = WINED3D_TADDRESS_WRAP;
    gl_tex->states[WINED3DTEXSTA_ADDRESSV] = WINED3D_TADDRESS_WRAP;
    gl_tex->states[WINED3DTEXSTA_ADDRESSW] = WINED3D_TADDRESS_WRAP;
    gl_tex->states[WINED3DTEXSTA_BORDERCOLOR] = 0;
    gl_tex->states[WINED3DTEXSTA_MAGFILTER] = WINED3D_TEXF_LINEAR;
    gl_tex->states[WINED3DTEXSTA_MINFILTER] = WINED3D_TEXF_POINT; /* GL_NEAREST_MIPMAP_LINEAR */
    gl_tex->states[WINED3DTEXSTA_MIPFILTER] = WINED3D_TEXF_LINEAR; /* GL_NEAREST_MIPMAP_LINEAR */
    gl_tex->states[WINED3DTEXSTA_MAXMIPLEVEL] = 0;
    gl_tex->states[WINED3DTEXSTA_MAXANISOTROPY] = 1;
    if (context->gl_info->supported[EXT_TEXTURE_SRGB_DECODE])
        gl_tex->states[WINED3DTEXSTA_SRGBTEXTURE] = TRUE;
    else
        gl_tex->states[WINED3DTEXSTA_SRGBTEXTURE] = srgb;
    gl_tex->states[WINED3DTEXSTA_SHADOW] = FALSE;
    wined3d_texture_set_dirty(texture);

    context_bind_texture(context, target, gl_tex->name);

    if (texture->resource.usage & WINED3DUSAGE_AUTOGENMIPMAP)
    {
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
        checkGLcall("glTexParameteri(target, GL_GENERATE_MIPMAP_SGIS, GL_TRUE)");
    }

    /* For a new texture we have to set the texture levels after binding the
     * texture. Beware that texture rectangles do not support mipmapping, but
     * set the maxmiplevel if we're relying on the partial
     * GL_ARB_texture_non_power_of_two emulation with texture rectangles.
     * (I.e., do not care about cond_np2 here, just look for
     * GL_TEXTURE_RECTANGLE_ARB.) */
    if (target != GL_TEXTURE_RECTANGLE_ARB)
    {
        TRACE("Setting GL_TEXTURE_MAX_LEVEL to %u.\n", texture->level_count - 1);
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, texture->level_count - 1);
        checkGLcall("glTexParameteri(target, GL_TEXTURE_MAX_LEVEL, texture->level_count)");
    }

    if (target == GL_TEXTURE_CUBE_MAP_ARB)
    {
        /* Cubemaps are always set to clamp, regardless of the sampler state. */
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    if (texture->flags & WINED3D_TEXTURE_COND_NP2)
    {
        /* Conditinal non power of two textures use a different clamping
         * default. If we're using the GL_WINE_normalized_texrect partial
         * driver emulation, we're dealing with a GL_TEXTURE_2D texture which
         * has the address mode set to repeat - something that prevents us
         * from hitting the accelerated codepath. Thus manually set the GL
         * state. The same applies to filtering. Even if the texture has only
         * one mip level, the default LINEAR_MIPMAP_LINEAR filter causes a SW
         * fallback on macos. */
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        checkGLcall("glTexParameteri");
        gl_tex->states[WINED3DTEXSTA_ADDRESSU] = WINED3D_TADDRESS_CLAMP;
        gl_tex->states[WINED3DTEXSTA_ADDRESSV] = WINED3D_TADDRESS_CLAMP;
        gl_tex->states[WINED3DTEXSTA_MAGFILTER] = WINED3D_TEXF_POINT;
        gl_tex->states[WINED3DTEXSTA_MINFILTER] = WINED3D_TEXF_POINT;
        gl_tex->states[WINED3DTEXSTA_MIPFILTER] = WINED3D_TEXF_NONE;
    }
}

/* Context activation is done by the caller. */
void wined3d_texture_bind_and_dirtify(struct wined3d_texture *texture,
        struct wined3d_context *context, BOOL srgb)
{
    DWORD active_sampler;

    /* We don't need a specific texture unit, but after binding the texture
     * the current unit is dirty. Read the unit back instead of switching to
     * 0, this avoids messing around with the state manager's GL states. The
     * current texture unit should always be a valid one.
     *
     * To be more specific, this is tricky because we can implicitly be
     * called from sampler() in state.c. This means we can't touch anything
     * other than whatever happens to be the currently active texture, or we
     * would risk marking already applied sampler states dirty again. */
    active_sampler = context->rev_tex_unit_map[context->active_texture];
    if (active_sampler != WINED3D_UNMAPPED_STAGE)
        context_invalidate_state(context, STATE_SAMPLER(active_sampler));

    wined3d_texture_bind(texture, context, srgb);
}

/* Context activation is done by the caller. */
static void apply_wrap(const struct wined3d_gl_info *gl_info, GLenum target,
        enum wined3d_texture_address d3d_wrap, GLenum param, BOOL cond_np2)
{
    GLint gl_wrap;

    if (d3d_wrap < WINED3D_TADDRESS_WRAP || d3d_wrap > WINED3D_TADDRESS_MIRROR_ONCE)
    {
        FIXME("Unrecognized or unsupported texture address mode %#x.\n", d3d_wrap);
        return;
    }

    /* Cubemaps are always set to clamp, regardless of the sampler state. */
    if (target == GL_TEXTURE_CUBE_MAP_ARB
            || (cond_np2 && d3d_wrap == WINED3D_TADDRESS_WRAP))
        gl_wrap = GL_CLAMP_TO_EDGE;
    else
        gl_wrap = gl_info->wrap_lookup[d3d_wrap - WINED3D_TADDRESS_WRAP];

    TRACE("Setting param %#x to %#x for target %#x.\n", param, gl_wrap, target);
    gl_info->gl_ops.gl.p_glTexParameteri(target, param, gl_wrap);
    checkGLcall("glTexParameteri(target, param, gl_wrap)");
}

/* Context activation is done by the caller (state handler). */
void wined3d_texture_apply_state_changes(struct wined3d_texture *texture,
        const DWORD sampler_states[WINED3D_HIGHEST_SAMPLER_STATE + 1],
        const struct wined3d_gl_info *gl_info)
{
    BOOL cond_np2 = texture->flags & WINED3D_TEXTURE_COND_NP2;
    GLenum target = texture->target;
    struct gl_texture *gl_tex;
    DWORD state;
    DWORD aniso;

    TRACE("texture %p, sampler_states %p.\n", texture, sampler_states);

    gl_tex = wined3d_texture_get_gl_texture(texture, texture->flags & WINED3D_TEXTURE_IS_SRGB);

    /* This function relies on the correct texture being bound and loaded. */

    if (sampler_states[WINED3D_SAMP_ADDRESS_U] != gl_tex->states[WINED3DTEXSTA_ADDRESSU])
    {
        state = sampler_states[WINED3D_SAMP_ADDRESS_U];
        apply_wrap(gl_info, target, state, GL_TEXTURE_WRAP_S, cond_np2);
        gl_tex->states[WINED3DTEXSTA_ADDRESSU] = state;
    }

    if (sampler_states[WINED3D_SAMP_ADDRESS_V] != gl_tex->states[WINED3DTEXSTA_ADDRESSV])
    {
        state = sampler_states[WINED3D_SAMP_ADDRESS_V];
        apply_wrap(gl_info, target, state, GL_TEXTURE_WRAP_T, cond_np2);
        gl_tex->states[WINED3DTEXSTA_ADDRESSV] = state;
    }

    if (sampler_states[WINED3D_SAMP_ADDRESS_W] != gl_tex->states[WINED3DTEXSTA_ADDRESSW])
    {
        state = sampler_states[WINED3D_SAMP_ADDRESS_W];
        apply_wrap(gl_info, target, state, GL_TEXTURE_WRAP_R, cond_np2);
        gl_tex->states[WINED3DTEXSTA_ADDRESSW] = state;
    }

    if (sampler_states[WINED3D_SAMP_BORDER_COLOR] != gl_tex->states[WINED3DTEXSTA_BORDERCOLOR])
    {
        float col[4];

        state = sampler_states[WINED3D_SAMP_BORDER_COLOR];
        D3DCOLORTOGLFLOAT4(state, col);
        TRACE("Setting border color for %#x to %#x.\n", target, state);
        gl_info->gl_ops.gl.p_glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, &col[0]);
        checkGLcall("glTexParameterfv(..., GL_TEXTURE_BORDER_COLOR, ...)");
        gl_tex->states[WINED3DTEXSTA_BORDERCOLOR] = state;
    }

    if (sampler_states[WINED3D_SAMP_MAG_FILTER] != gl_tex->states[WINED3DTEXSTA_MAGFILTER])
    {
        GLint gl_value;

        state = sampler_states[WINED3D_SAMP_MAG_FILTER];
        if (state > WINED3D_TEXF_ANISOTROPIC)
            FIXME("Unrecognized or unsupported MAGFILTER* value %d.\n", state);

        gl_value = wined3d_gl_mag_filter(texture->mag_lookup,
                min(max(state, WINED3D_TEXF_POINT), WINED3D_TEXF_LINEAR));
        TRACE("ValueMAG=%#x setting MAGFILTER to %#x.\n", state, gl_value);
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_MAG_FILTER, gl_value);

        gl_tex->states[WINED3DTEXSTA_MAGFILTER] = state;
    }

    if ((sampler_states[WINED3D_SAMP_MIN_FILTER] != gl_tex->states[WINED3DTEXSTA_MINFILTER]
            || sampler_states[WINED3D_SAMP_MIP_FILTER] != gl_tex->states[WINED3DTEXSTA_MIPFILTER]
            || sampler_states[WINED3D_SAMP_MAX_MIP_LEVEL] != gl_tex->states[WINED3DTEXSTA_MAXMIPLEVEL]))
    {
        GLint gl_value;

        gl_tex->states[WINED3DTEXSTA_MIPFILTER] = sampler_states[WINED3D_SAMP_MIP_FILTER];
        gl_tex->states[WINED3DTEXSTA_MINFILTER] = sampler_states[WINED3D_SAMP_MIN_FILTER];
        gl_tex->states[WINED3DTEXSTA_MAXMIPLEVEL] = sampler_states[WINED3D_SAMP_MAX_MIP_LEVEL];

        if (gl_tex->states[WINED3DTEXSTA_MINFILTER] > WINED3D_TEXF_ANISOTROPIC
            || gl_tex->states[WINED3DTEXSTA_MIPFILTER] > WINED3D_TEXF_ANISOTROPIC)
        {
            FIXME("Unrecognized or unsupported MIN_FILTER value %#x MIP_FILTER value %#x.\n",
                  gl_tex->states[WINED3DTEXSTA_MINFILTER],
                  gl_tex->states[WINED3DTEXSTA_MIPFILTER]);
        }
        gl_value = wined3d_gl_min_mip_filter(texture->min_mip_lookup,
                min(max(sampler_states[WINED3D_SAMP_MIN_FILTER], WINED3D_TEXF_POINT), WINED3D_TEXF_LINEAR),
                min(max(sampler_states[WINED3D_SAMP_MIP_FILTER], WINED3D_TEXF_NONE), WINED3D_TEXF_LINEAR));

        TRACE("ValueMIN=%#x, ValueMIP=%#x, setting MINFILTER to %#x.\n",
              sampler_states[WINED3D_SAMP_MIN_FILTER],
              sampler_states[WINED3D_SAMP_MIP_FILTER], gl_value);
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_MIN_FILTER, gl_value);
        checkGLcall("glTexParameter GL_TEXTURE_MIN_FILTER, ...");

        if (!cond_np2)
        {
            if (gl_tex->states[WINED3DTEXSTA_MIPFILTER] == WINED3D_TEXF_NONE)
                gl_value = texture->lod;
            else if (gl_tex->states[WINED3DTEXSTA_MAXMIPLEVEL] >= texture->level_count)
                gl_value = texture->level_count - 1;
            else if (gl_tex->states[WINED3DTEXSTA_MAXMIPLEVEL] < texture->lod)
                /* texture->lod is already clamped in the setter. */
                gl_value = texture->lod;
            else
                gl_value = gl_tex->states[WINED3DTEXSTA_MAXMIPLEVEL];

            /* Note that WINED3D_SAMP_MAX_MIP_LEVEL specifies the largest mipmap
             * (default 0), while GL_TEXTURE_MAX_LEVEL specifies the smallest
             * mimap used (default 1000). So WINED3D_SAMP_MAX_MIP_LEVEL
             * corresponds to GL_TEXTURE_BASE_LEVEL. */
            gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, gl_value);
        }
    }

    if ((gl_tex->states[WINED3DTEXSTA_MAGFILTER] != WINED3D_TEXF_ANISOTROPIC
            && gl_tex->states[WINED3DTEXSTA_MINFILTER] != WINED3D_TEXF_ANISOTROPIC
            && gl_tex->states[WINED3DTEXSTA_MIPFILTER] != WINED3D_TEXF_ANISOTROPIC)
            || cond_np2)
        aniso = 1;
    else
        aniso = sampler_states[WINED3D_SAMP_MAX_ANISOTROPY];

    if (gl_tex->states[WINED3DTEXSTA_MAXANISOTROPY] != aniso)
    {
        if (gl_info->supported[EXT_TEXTURE_FILTER_ANISOTROPIC])
        {
            gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
            checkGLcall("glTexParameteri(GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso)");
        }
        else
        {
            WARN("Anisotropic filtering not supported.\n");
        }
        gl_tex->states[WINED3DTEXSTA_MAXANISOTROPY] = aniso;
    }

    /* These should always be the same unless EXT_texture_sRGB_decode is supported. */
    if (sampler_states[WINED3D_SAMP_SRGB_TEXTURE] != gl_tex->states[WINED3DTEXSTA_SRGBTEXTURE])
    {
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_SRGB_DECODE_EXT,
                sampler_states[WINED3D_SAMP_SRGB_TEXTURE] ? GL_DECODE_EXT : GL_SKIP_DECODE_EXT);
        checkGLcall("glTexParameteri(GL_TEXTURE_SRGB_DECODE_EXT)");
        gl_tex->states[WINED3DTEXSTA_SRGBTEXTURE] = sampler_states[WINED3D_SAMP_SRGB_TEXTURE];
    }

    if (!(texture->resource.format->flags & WINED3DFMT_FLAG_SHADOW)
            != !gl_tex->states[WINED3DTEXSTA_SHADOW])
    {
        if (texture->resource.format->flags & WINED3DFMT_FLAG_SHADOW)
        {
            gl_info->gl_ops.gl.p_glTexParameteri(target, GL_DEPTH_TEXTURE_MODE_ARB, GL_LUMINANCE);
            gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
            checkGLcall("glTexParameteri(target, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB)");
            gl_tex->states[WINED3DTEXSTA_SHADOW] = TRUE;
        }
        else
        {
            gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
            checkGLcall("glTexParameteri(target, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE)");
            gl_tex->states[WINED3DTEXSTA_SHADOW] = FALSE;
        }
    }
}

ULONG CDECL wined3d_texture_incref(struct wined3d_texture *texture)
{
    ULONG refcount = InterlockedIncrement(&texture->resource.ref);

    TRACE("%p increasing refcount to %u.\n", texture, refcount);

    return refcount;
}

ULONG CDECL wined3d_texture_decref(struct wined3d_texture *texture)
{
    ULONG refcount = InterlockedDecrement(&texture->resource.ref);

    TRACE("%p decreasing refcount to %u.\n", texture, refcount);

    if (!refcount)
    {
        wined3d_texture_cleanup(texture);
        texture->resource.parent_ops->wined3d_object_destroyed(texture->resource.parent);
        HeapFree(GetProcessHeap(), 0, texture);
    }

    return refcount;
}

struct wined3d_resource * CDECL wined3d_texture_get_resource(struct wined3d_texture *texture)
{
    TRACE("texture %p.\n", texture);

    return &texture->resource;
}

DWORD CDECL wined3d_texture_set_priority(struct wined3d_texture *texture, DWORD priority)
{
    return resource_set_priority(&texture->resource, priority);
}

DWORD CDECL wined3d_texture_get_priority(const struct wined3d_texture *texture)
{
    return resource_get_priority(&texture->resource);
}

/* Context activation is done by the caller */
void wined3d_texture_load(struct wined3d_texture *texture,
        struct wined3d_context *context, BOOL srgb)
{
    UINT sub_count = texture->level_count * texture->layer_count;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    DWORD flag;
    UINT i;

    TRACE("texture %p, srgb %#x.\n", texture, srgb);

    if (gl_info->supported[EXT_TEXTURE_SRGB_DECODE])
        srgb = FALSE;

    if (srgb)
        flag = WINED3D_TEXTURE_SRGB_VALID;
    else
        flag = WINED3D_TEXTURE_RGB_VALID;

    if (texture->flags & flag)
    {
        TRACE("Texture %p not dirty, nothing to do.\n", texture);
        return;
    }

    /* Reload the surfaces if the texture is marked dirty. */
    for (i = 0; i < sub_count; ++i)
    {
        texture->texture_ops->texture_sub_resource_load(texture->sub_resources[i], context, srgb);
    }
    texture->flags |= flag;
}

void CDECL wined3d_texture_preload(struct wined3d_texture *texture)
{
    struct wined3d_context *context;
    context = context_acquire(texture->resource.device, NULL);
    wined3d_texture_load(texture, context, texture->flags & WINED3D_TEXTURE_IS_SRGB);
    context_release(context);
}

void * CDECL wined3d_texture_get_parent(const struct wined3d_texture *texture)
{
    TRACE("texture %p.\n", texture);

    return texture->resource.parent;
}

DWORD CDECL wined3d_texture_set_lod(struct wined3d_texture *texture, DWORD lod)
{
    DWORD old = texture->lod;

    TRACE("texture %p, lod %u.\n", texture, lod);

    /* The d3d9:texture test shows that SetLOD is ignored on non-managed
     * textures. The call always returns 0, and GetLOD always returns 0. */
    if (texture->resource.pool != WINED3D_POOL_MANAGED)
    {
        TRACE("Ignoring SetLOD on %s texture, returning 0.\n", debug_d3dpool(texture->resource.pool));
        return 0;
    }

    if (lod >= texture->level_count)
        lod = texture->level_count - 1;

    if (texture->lod != lod)
    {
        texture->lod = lod;

        texture->texture_rgb.states[WINED3DTEXSTA_MAXMIPLEVEL] = ~0U;
        texture->texture_srgb.states[WINED3DTEXSTA_MAXMIPLEVEL] = ~0U;
        if (texture->resource.bind_count)
            device_invalidate_state(texture->resource.device, STATE_SAMPLER(texture->sampler));
    }

    return old;
}

DWORD CDECL wined3d_texture_get_lod(const struct wined3d_texture *texture)
{
    TRACE("texture %p, returning %u.\n", texture, texture->lod);

    return texture->lod;
}

DWORD CDECL wined3d_texture_get_level_count(const struct wined3d_texture *texture)
{
    TRACE("texture %p, returning %u.\n", texture, texture->level_count);

    return texture->level_count;
}

HRESULT CDECL wined3d_texture_set_autogen_filter_type(struct wined3d_texture *texture,
        enum wined3d_texture_filter_type filter_type)
{
    FIXME("texture %p, filter_type %s stub!\n", texture, debug_d3dtexturefiltertype(filter_type));

    if (!(texture->resource.usage & WINED3DUSAGE_AUTOGENMIPMAP))
    {
        WARN("Texture doesn't have AUTOGENMIPMAP usage.\n");
        return WINED3DERR_INVALIDCALL;
    }

    texture->filter_type = filter_type;

    return WINED3D_OK;
}

enum wined3d_texture_filter_type CDECL wined3d_texture_get_autogen_filter_type(const struct wined3d_texture *texture)
{
    TRACE("texture %p.\n", texture);

    return texture->filter_type;
}

HRESULT CDECL wined3d_texture_set_color_key(struct wined3d_texture *texture,
        DWORD flags, const struct wined3d_color_key *color_key)
{
    TRACE("texture %p, flags %#x, color_key %p.\n", texture, flags, color_key);

    if (flags & WINEDDCKEY_COLORSPACE)
    {
        FIXME("Unhandled flags %#x.\n", flags);
        return WINED3DERR_INVALIDCALL;
    }

    if (color_key)
    {
        switch (flags & ~WINEDDCKEY_COLORSPACE)
        {
            case WINEDDCKEY_DESTBLT:
                texture->dst_blt_color_key = *color_key;
                texture->color_key_flags |= WINEDDSD_CKDESTBLT;
                break;

            case WINEDDCKEY_DESTOVERLAY:
                texture->dst_overlay_color_key = *color_key;
                texture->color_key_flags |= WINEDDSD_CKDESTOVERLAY;
                break;

            case WINEDDCKEY_SRCOVERLAY:
                texture->src_overlay_color_key = *color_key;
                texture->color_key_flags |= WINEDDSD_CKSRCOVERLAY;
                break;

            case WINEDDCKEY_SRCBLT:
                texture->src_blt_color_key = *color_key;
                texture->color_key_flags |= WINEDDSD_CKSRCBLT;
                break;
        }
    }
    else
    {
        switch (flags & ~WINEDDCKEY_COLORSPACE)
        {
            case WINEDDCKEY_DESTBLT:
                texture->color_key_flags &= ~WINEDDSD_CKDESTBLT;
                break;

            case WINEDDCKEY_DESTOVERLAY:
                texture->color_key_flags &= ~WINEDDSD_CKDESTOVERLAY;
                break;

            case WINEDDCKEY_SRCOVERLAY:
                texture->color_key_flags &= ~WINEDDSD_CKSRCOVERLAY;
                break;

            case WINEDDCKEY_SRCBLT:
                texture->color_key_flags &= ~WINEDDSD_CKSRCBLT;
                break;
        }
    }

    return WINED3D_OK;
}

void CDECL wined3d_texture_generate_mipmaps(struct wined3d_texture *texture)
{
    /* TODO: Implement filters using GL_SGI_generate_mipmaps. */
    FIXME("texture %p stub!\n", texture);
}

struct wined3d_resource * CDECL wined3d_texture_get_sub_resource(struct wined3d_texture *texture,
        UINT sub_resource_idx)
{
    UINT sub_count = texture->level_count * texture->layer_count;

    TRACE("texture %p, sub_resource_idx %u.\n", texture, sub_resource_idx);

    if (sub_resource_idx >= sub_count)
    {
        WARN("sub_resource_idx %u >= sub_count %u.\n", sub_resource_idx, sub_count);
        return NULL;
    }

    return texture->sub_resources[sub_resource_idx];
}

HRESULT CDECL wined3d_texture_add_dirty_region(struct wined3d_texture *texture,
        UINT layer, const struct wined3d_box *dirty_region)
{
    struct wined3d_resource *sub_resource;

    TRACE("texture %p, layer %u, dirty_region %p.\n", texture, layer, dirty_region);

    if (!(sub_resource = wined3d_texture_get_sub_resource(texture, layer * texture->level_count)))
    {
        WARN("Failed to get sub-resource.\n");
        return WINED3DERR_INVALIDCALL;
    }

    texture->texture_ops->texture_sub_resource_add_dirty_region(sub_resource, dirty_region);

    return WINED3D_OK;
}

static void texture2d_sub_resource_load(struct wined3d_resource *sub_resource,
        struct wined3d_context *context, BOOL srgb)
{
    surface_load(surface_from_resource(sub_resource), srgb);
}

static void texture2d_sub_resource_add_dirty_region(struct wined3d_resource *sub_resource,
        const struct wined3d_box *dirty_region)
{
    struct wined3d_surface *surface = surface_from_resource(sub_resource);

    surface_prepare_map_memory(surface);
    surface_load_location(surface, surface->map_binding);
    surface_invalidate_location(surface, ~surface->map_binding);
}

static void texture2d_sub_resource_cleanup(struct wined3d_resource *sub_resource)
{
    struct wined3d_surface *surface = surface_from_resource(sub_resource);

    surface_set_texture_target(surface, 0, 0);
    surface_set_container(surface, NULL);
    wined3d_surface_decref(surface);
}

static const struct wined3d_texture_ops texture2d_ops =
{
    texture2d_sub_resource_load,
    texture2d_sub_resource_add_dirty_region,
    texture2d_sub_resource_cleanup,
};

static void wined3d_texture_unload(struct wined3d_resource *resource)
{
    struct wined3d_texture *texture = wined3d_texture_from_resource(resource);
    UINT sub_count = texture->level_count * texture->layer_count;
    UINT i;

    TRACE("texture %p.\n", texture);

    for (i = 0; i < sub_count; ++i)
    {
        struct wined3d_resource *sub_resource = texture->sub_resources[i];

        sub_resource->resource_ops->resource_unload(sub_resource);
    }

    wined3d_texture_unload_gl_texture(texture);
}

static const struct wined3d_resource_ops texture_resource_ops =
{
    wined3d_texture_unload,
};

static HRESULT cubetexture_init(struct wined3d_texture *texture, const struct wined3d_resource_desc *desc,
        UINT levels, DWORD surface_flags, struct wined3d_device *device, void *parent,
        const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_resource_desc surface_desc;
    unsigned int i, j;
    HRESULT hr;

    /* TODO: It should only be possible to create textures for formats
     * that are reported as supported. */
    if (WINED3DFMT_UNKNOWN >= desc->format)
    {
        WARN("(%p) : Texture cannot be created with a format of WINED3DFMT_UNKNOWN.\n", texture);
        return WINED3DERR_INVALIDCALL;
    }

    if (!gl_info->supported[ARB_TEXTURE_CUBE_MAP] && desc->pool != WINED3D_POOL_SCRATCH)
    {
        WARN("(%p) : Tried to create not supported cube texture.\n", texture);
        return WINED3DERR_INVALIDCALL;
    }

    /* Calculate levels for mip mapping */
    if (desc->usage & WINED3DUSAGE_AUTOGENMIPMAP)
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
        levels = wined3d_log2i(desc->width) + 1;
        TRACE("Calculated levels = %u.\n", levels);
    }

    if (!gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO])
    {
        UINT pow2_edge_length = 1;
        while (pow2_edge_length < desc->width)
            pow2_edge_length <<= 1;

        if (desc->width != pow2_edge_length)
        {
            if (desc->pool == WINED3D_POOL_SCRATCH)
            {
                /* SCRATCH textures cannot be used for texturing */
                WARN("Creating a scratch NPOT cube texture despite lack of HW support.\n");
            }
            else
            {
                WARN("Attempted to create a NPOT cube texture (edge length %u) without GL support.\n", desc->width);
                return WINED3DERR_INVALIDCALL;
            }
        }
    }

    if (FAILED(hr = wined3d_texture_init(texture, &texture2d_ops, 6, levels,
            desc, device, parent, parent_ops, &texture_resource_ops)))
    {
        WARN("Failed to initialize texture, returning %#x\n", hr);
        return hr;
    }

    texture->pow2_matrix[0] = 1.0f;
    texture->pow2_matrix[5] = 1.0f;
    texture->pow2_matrix[10] = 1.0f;
    texture->pow2_matrix[15] = 1.0f;
    texture->target = GL_TEXTURE_CUBE_MAP_ARB;

    /* Generate all the surfaces. */
    surface_desc = *desc;
    surface_desc.resource_type = WINED3D_RTYPE_SURFACE;
    for (i = 0; i < texture->level_count; ++i)
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
            UINT idx = j * texture->level_count + i;
            struct wined3d_surface *surface;

            if (FAILED(hr = wined3d_surface_create(texture, &surface_desc, surface_flags, &surface)))
            {
                WARN("Failed to create surface, hr %#x.\n", hr);
                wined3d_texture_cleanup(texture);
                return hr;
            }

            surface_set_texture_target(surface, cube_targets[j], i);
            texture->sub_resources[idx] = &surface->resource;
            TRACE("Created surface level %u @ %p.\n", i, surface);
        }
        surface_desc.width = max(1, surface_desc.width >> 1);
        surface_desc.height = surface_desc.width;
    }

    return WINED3D_OK;
}

static HRESULT texture_init(struct wined3d_texture *texture, const struct wined3d_resource_desc *desc,
        UINT levels, DWORD surface_flags, struct wined3d_device *device, void *parent,
        const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_resource_desc surface_desc;
    UINT pow2_width, pow2_height;
    unsigned int i;
    HRESULT hr;

    /* TODO: It should only be possible to create textures for formats
     * that are reported as supported. */
    if (WINED3DFMT_UNKNOWN >= desc->format)
    {
        WARN("(%p) : Texture cannot be created with a format of WINED3DFMT_UNKNOWN.\n", texture);
        return WINED3DERR_INVALIDCALL;
    }

    /* Non-power2 support. */
    if (gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO])
    {
        pow2_width = desc->width;
        pow2_height = desc->height;
    }
    else
    {
        /* Find the nearest pow2 match. */
        pow2_width = pow2_height = 1;
        while (pow2_width < desc->width)
            pow2_width <<= 1;
        while (pow2_height < desc->height)
            pow2_height <<= 1;

        if (pow2_width != desc->width || pow2_height != desc->height)
        {
            /* levels == 0 returns an error as well */
            if (levels != 1)
            {
                if (desc->pool == WINED3D_POOL_SCRATCH)
                {
                    WARN("Creating a scratch mipmapped NPOT texture despite lack of HW support.\n");
                }
                else
                {
                    WARN("Attempted to create a mipmapped NPOT texture without unconditional NPOT support.\n");
                    return WINED3DERR_INVALIDCALL;
                }
            }
        }
    }

    /* Calculate levels for mip mapping. */
    if (desc->usage & WINED3DUSAGE_AUTOGENMIPMAP)
    {
        if (!gl_info->supported[SGIS_GENERATE_MIPMAP])
        {
            WARN("No mipmap generation support, returning WINED3DERR_INVALIDCALL.\n");
            return WINED3DERR_INVALIDCALL;
        }

        if (levels > 1)
        {
            WARN("D3DUSAGE_AUTOGENMIPMAP is set, and level count > 1, returning WINED3DERR_INVALIDCALL.\n");
            return WINED3DERR_INVALIDCALL;
        }

        levels = 1;
    }
    else if (!levels)
    {
        levels = wined3d_log2i(max(desc->width, desc->height)) + 1;
        TRACE("Calculated levels = %u.\n", levels);
    }

    if (FAILED(hr = wined3d_texture_init(texture, &texture2d_ops, 1, levels,
            desc, device, parent, parent_ops, &texture_resource_ops)))
    {
        WARN("Failed to initialize texture, returning %#x.\n", hr);
        return hr;
    }

    /* Precalculated scaling for 'faked' non power of two texture coords. */
    if (gl_info->supported[WINED3D_GL_NORMALIZED_TEXRECT]
            && (desc->width != pow2_width || desc->height != pow2_height))
    {
        texture->pow2_matrix[0] = 1.0f;
        texture->pow2_matrix[5] = 1.0f;
        texture->pow2_matrix[10] = 1.0f;
        texture->pow2_matrix[15] = 1.0f;
        texture->target = GL_TEXTURE_2D;
        texture->flags |= WINED3D_TEXTURE_COND_NP2;
        texture->min_mip_lookup = minMipLookup_noFilter;
    }
    else if (gl_info->supported[ARB_TEXTURE_RECTANGLE]
            && (desc->width != pow2_width || desc->height != pow2_height))
    {
        texture->pow2_matrix[0] = (float)desc->width;
        texture->pow2_matrix[5] = (float)desc->height;
        texture->pow2_matrix[10] = 1.0f;
        texture->pow2_matrix[15] = 1.0f;
        texture->target = GL_TEXTURE_RECTANGLE_ARB;
        texture->flags |= WINED3D_TEXTURE_COND_NP2;
        texture->flags &= ~WINED3D_TEXTURE_POW2_MAT_IDENT;

        if (texture->resource.format->flags & WINED3DFMT_FLAG_FILTERING)
            texture->min_mip_lookup = minMipLookup_noMip;
        else
            texture->min_mip_lookup = minMipLookup_noFilter;
    }
    else
    {
        if ((desc->width != pow2_width) || (desc->height != pow2_height))
        {
            texture->pow2_matrix[0] = (((float)desc->width) / ((float)pow2_width));
            texture->pow2_matrix[5] = (((float)desc->height) / ((float)pow2_height));
            texture->flags &= ~WINED3D_TEXTURE_POW2_MAT_IDENT;
        }
        else
        {
            texture->pow2_matrix[0] = 1.0f;
            texture->pow2_matrix[5] = 1.0f;
        }

        texture->pow2_matrix[10] = 1.0f;
        texture->pow2_matrix[15] = 1.0f;
        texture->target = GL_TEXTURE_2D;
    }
    TRACE("xf(%f) yf(%f)\n", texture->pow2_matrix[0], texture->pow2_matrix[5]);

    /* Generate all the surfaces. */
    surface_desc = *desc;
    surface_desc.resource_type = WINED3D_RTYPE_SURFACE;
    for (i = 0; i < texture->level_count; ++i)
    {
        struct wined3d_surface *surface;

        if (FAILED(hr = wined3d_surface_create(texture, &surface_desc, surface_flags, &surface)))
        {
            WARN("Failed to create surface, hr %#x.\n", hr);
            wined3d_texture_cleanup(texture);
            return hr;
        }

        surface_set_texture_target(surface, texture->target, i);
        texture->sub_resources[i] = &surface->resource;
        TRACE("Created surface level %u @ %p.\n", i, surface);
        /* Calculate the next mipmap level. */
        surface_desc.width = max(1, surface_desc.width >> 1);
        surface_desc.height = max(1, surface_desc.height >> 1);
    }

    return WINED3D_OK;
}

static void texture3d_sub_resource_load(struct wined3d_resource *sub_resource,
        struct wined3d_context *context, BOOL srgb)
{
    wined3d_volume_load(volume_from_resource(sub_resource), context, srgb);
}

static void texture3d_sub_resource_add_dirty_region(struct wined3d_resource *sub_resource,
        const struct wined3d_box *dirty_region)
{
    wined3d_texture_set_dirty(volume_from_resource(sub_resource)->container);
}

static void texture3d_sub_resource_cleanup(struct wined3d_resource *sub_resource)
{
    struct wined3d_volume *volume = volume_from_resource(sub_resource);

    /* Cleanup the container. */
    volume_set_container(volume, NULL);
    wined3d_volume_decref(volume);
}

static const struct wined3d_texture_ops texture3d_ops =
{
    texture3d_sub_resource_load,
    texture3d_sub_resource_add_dirty_region,
    texture3d_sub_resource_cleanup,
};

static HRESULT volumetexture_init(struct wined3d_texture *texture, const struct wined3d_resource_desc *desc,
        UINT levels, struct wined3d_device *device, void *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_resource_desc volume_desc;
    unsigned int i;
    HRESULT hr;

    /* TODO: It should only be possible to create textures for formats
     * that are reported as supported. */
    if (WINED3DFMT_UNKNOWN >= desc->format)
    {
        WARN("(%p) : Texture cannot be created with a format of WINED3DFMT_UNKNOWN.\n", texture);
        return WINED3DERR_INVALIDCALL;
    }

    if (!gl_info->supported[EXT_TEXTURE3D])
    {
        WARN("(%p) : Texture cannot be created - no volume texture support.\n", texture);
        return WINED3DERR_INVALIDCALL;
    }

    /* Calculate levels for mip mapping. */
    if (desc->usage & WINED3DUSAGE_AUTOGENMIPMAP)
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
        levels = wined3d_log2i(max(max(desc->width, desc->height), desc->depth)) + 1;
        TRACE("Calculated levels = %u.\n", levels);
    }

    if (!gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO])
    {
        UINT pow2_w, pow2_h, pow2_d;
        pow2_w = 1;
        while (pow2_w < desc->width)
            pow2_w <<= 1;
        pow2_h = 1;
        while (pow2_h < desc->height)
            pow2_h <<= 1;
        pow2_d = 1;
        while (pow2_d < desc->depth)
            pow2_d <<= 1;

        if (pow2_w != desc->width || pow2_h != desc->height || pow2_d != desc->depth)
        {
            if (desc->pool == WINED3D_POOL_SCRATCH)
            {
                WARN("Creating a scratch NPOT volume texture despite lack of HW support.\n");
            }
            else
            {
                WARN("Attempted to create a NPOT volume texture (%u, %u, %u) without GL support.\n",
                        desc->width, desc->height, desc->depth);
                return WINED3DERR_INVALIDCALL;
            }
        }
    }

    if (FAILED(hr = wined3d_texture_init(texture, &texture3d_ops, 1, levels,
            desc, device, parent, parent_ops, &texture_resource_ops)))
    {
        WARN("Failed to initialize texture, returning %#x.\n", hr);
        return hr;
    }

    texture->pow2_matrix[0] = 1.0f;
    texture->pow2_matrix[5] = 1.0f;
    texture->pow2_matrix[10] = 1.0f;
    texture->pow2_matrix[15] = 1.0f;
    texture->target = GL_TEXTURE_3D;

    /* Generate all the surfaces. */
    volume_desc = *desc;
    volume_desc.resource_type = WINED3D_RTYPE_VOLUME;
    for (i = 0; i < texture->level_count; ++i)
    {
        struct wined3d_volume *volume;

        if (FAILED(hr = wined3d_volume_create(texture, &volume_desc, i, &volume)))
        {
            ERR("Creating a volume for the volume texture failed, hr %#x.\n", hr);
            wined3d_texture_cleanup(texture);
            return hr;
        }

        texture->sub_resources[i] = &volume->resource;

        /* Calculate the next mipmap level. */
        volume_desc.width = max(1, volume_desc.width >> 1);
        volume_desc.height = max(1, volume_desc.height >> 1);
        volume_desc.depth = max(1, volume_desc.depth >> 1);
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_texture_create(struct wined3d_device *device, const struct wined3d_resource_desc *desc,
        UINT level_count, DWORD surface_flags, void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_texture **texture)
{
    struct wined3d_texture *object;
    HRESULT hr;

    TRACE("device %p, desc %p, level_count %u, surface_flags %#x, parent %p, parent_ops %p, texture %p.\n",
            device, desc, level_count, surface_flags, parent, parent_ops, texture);

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    switch (desc->resource_type)
    {
        case WINED3D_RTYPE_TEXTURE:
            hr = texture_init(object, desc, level_count, surface_flags, device, parent, parent_ops);
            break;

        case WINED3D_RTYPE_VOLUME_TEXTURE:
            hr = volumetexture_init(object, desc, level_count, device, parent, parent_ops);
            break;

        case WINED3D_RTYPE_CUBE_TEXTURE:
            hr = cubetexture_init(object, desc, level_count, surface_flags, device, parent, parent_ops);
            break;

        default:
            ERR("Invalid resource type %s.\n", debug_d3dresourcetype(desc->resource_type));
            hr = WINED3DERR_INVALIDCALL;
            break;
    }

    if (FAILED(hr))
    {
        WARN("Failed to initialize texture, returning %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created texture %p.\n", object);
    *texture = object;

    return WINED3D_OK;
}
