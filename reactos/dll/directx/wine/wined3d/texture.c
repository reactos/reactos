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
WINE_DECLARE_DEBUG_CHANNEL(winediag);

static HRESULT wined3d_texture_init(struct wined3d_texture *texture, const struct wined3d_texture_ops *texture_ops,
        UINT layer_count, UINT level_count, const struct wined3d_resource_desc *desc, DWORD flags,
        struct wined3d_device *device, void *parent, const struct wined3d_parent_ops *parent_ops,
        const struct wined3d_resource_ops *resource_ops)
{
    const struct wined3d_format *format = wined3d_get_format(&device->adapter->gl_info, desc->format);
    HRESULT hr;

    TRACE("texture %p, texture_ops %p, layer_count %u, level_count %u, resource_type %s, format %s, "
            "multisample_type %#x, multisample_quality %#x, usage %s, pool %s, width %u, height %u, depth %u, "
            "flags %#x, device %p, parent %p, parent_ops %p, resource_ops %p.\n",
            texture, texture_ops, layer_count, level_count, debug_d3dresourcetype(desc->resource_type),
            debug_d3dformat(desc->format), desc->multisample_type, desc->multisample_quality,
            debug_d3dusage(desc->usage), debug_d3dpool(desc->pool), desc->width, desc->height, desc->depth,
            flags, device, parent, parent_ops, resource_ops);

    if (FAILED(hr = resource_init(&texture->resource, device, desc->resource_type, format,
            desc->multisample_type, desc->multisample_quality, desc->usage, desc->pool,
            desc->width, desc->height, desc->depth, 0, parent, parent_ops, resource_ops)))
    {
        static unsigned int once;

        /* DXTn 3D textures are not supported. Do not write the ERR for them. */
        if ((desc->format == WINED3DFMT_DXT1 || desc->format == WINED3DFMT_DXT2 || desc->format == WINED3DFMT_DXT3
                || desc->format == WINED3DFMT_DXT4 || desc->format == WINED3DFMT_DXT5)
                && !(format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_TEXTURE)
                && desc->resource_type != WINED3D_RTYPE_TEXTURE_3D && !once++)
            ERR_(winediag)("The application tried to create a DXTn texture, but the driver does not support them.\n");

        WARN("Failed to initialize resource, returning %#x\n", hr);
        return hr;
    }
    wined3d_resource_update_draw_binding(&texture->resource);

    texture->texture_ops = texture_ops;

    texture->layer_count = layer_count;
    texture->level_count = level_count;
    texture->filter_type = (desc->usage & WINED3DUSAGE_AUTOGENMIPMAP) ? WINED3D_TEXF_LINEAR : WINED3D_TEXF_NONE;
    texture->lod = 0;
    texture->flags = WINED3D_TEXTURE_POW2_MAT_IDENT | WINED3D_TEXTURE_NORMALIZED_COORDS;
    if (flags & WINED3D_TEXTURE_CREATE_PIN_SYSMEM)
        texture->flags |= WINED3D_TEXTURE_PIN_SYSMEM;

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

#if defined(STAGING_CSMT)
void wined3d_texture_cleanup_cs(struct wined3d_texture *texture)
{
    wined3d_texture_unload_gl_texture(texture);
    HeapFree(GetProcessHeap(), 0, texture);
}

static void wined3d_texture_cleanup(struct wined3d_texture *texture)
{
    UINT sub_count = texture->level_count * texture->layer_count;
    UINT i;
    struct wined3d_device *device = texture->resource.device;
#else  /* STAGING_CSMT */
static void wined3d_texture_cleanup(struct wined3d_texture *texture)
{
    UINT sub_count = texture->level_count * texture->layer_count;
    UINT i;
#endif /* STAGING_CSMT */

    TRACE("texture %p.\n", texture);

    for (i = 0; i < sub_count; ++i)
    {
        struct wined3d_resource *sub_resource = texture->sub_resources[i].resource;

        if (sub_resource)
            texture->texture_ops->texture_sub_resource_cleanup(sub_resource);
    }

#if defined(STAGING_CSMT)
    resource_cleanup(&texture->resource);
    wined3d_cs_emit_texture_cleanup(device->cs, texture);
#else  /* STAGING_CSMT */
    wined3d_texture_unload_gl_texture(texture);
    resource_cleanup(&texture->resource);
#endif /* STAGING_CSMT */
}

void wined3d_texture_set_swapchain(struct wined3d_texture *texture, struct wined3d_swapchain *swapchain)
{
    texture->swapchain = swapchain;
    wined3d_resource_update_draw_binding(&texture->resource);
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

    if (!needs_separate_srgb_gl_texture(context))
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

    /* Initialise the state of the texture object to the OpenGL defaults, not
     * the wined3d defaults. */
    gl_tex->sampler_desc.address_u = WINED3D_TADDRESS_WRAP;
    gl_tex->sampler_desc.address_v = WINED3D_TADDRESS_WRAP;
    gl_tex->sampler_desc.address_w = WINED3D_TADDRESS_WRAP;
    memset(gl_tex->sampler_desc.border_color, 0, sizeof(gl_tex->sampler_desc.border_color));
    gl_tex->sampler_desc.mag_filter = WINED3D_TEXF_LINEAR;
    gl_tex->sampler_desc.min_filter = WINED3D_TEXF_POINT; /* GL_NEAREST_MIPMAP_LINEAR */
    gl_tex->sampler_desc.mip_filter = WINED3D_TEXF_LINEAR; /* GL_NEAREST_MIPMAP_LINEAR */
    gl_tex->sampler_desc.lod_bias = 0.0f;
    gl_tex->sampler_desc.min_lod = -1000.0f;
    gl_tex->sampler_desc.max_lod = 1000.0f;
    gl_tex->sampler_desc.max_anisotropy = 1;
    gl_tex->sampler_desc.compare = FALSE;
    gl_tex->sampler_desc.comparison_func = WINED3D_CMP_LESSEQUAL;
    if (context->gl_info->supported[EXT_TEXTURE_SRGB_DECODE])
        gl_tex->sampler_desc.srgb_decode = TRUE;
    else
        gl_tex->sampler_desc.srgb_decode = srgb;
    gl_tex->base_level = 0;
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
        gl_tex->sampler_desc.address_u = WINED3D_TADDRESS_CLAMP;
        gl_tex->sampler_desc.address_v = WINED3D_TADDRESS_CLAMP;
        gl_tex->sampler_desc.mag_filter = WINED3D_TEXF_POINT;
        gl_tex->sampler_desc.min_filter = WINED3D_TEXF_POINT;
        gl_tex->sampler_desc.mip_filter = WINED3D_TEXF_NONE;
    }

    if (gl_info->supported[WINED3D_GL_LEGACY_CONTEXT] && gl_info->supported[ARB_DEPTH_TEXTURE])
    {
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_DEPTH_TEXTURE_MODE_ARB, GL_INTENSITY);
        checkGLcall("glTexParameteri(GL_DEPTH_TEXTURE_MODE_ARB, GL_INTENSITY)");
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
    /* FIXME: Ideally we'd only do this when touching a binding that's used by
     * a shader. */
    context_invalidate_state(context, STATE_SHADER_RESOURCE_BINDING);

    wined3d_texture_bind(texture, context, srgb);
}

/* Context activation is done by the caller (state handler). */
/* This function relies on the correct texture being bound and loaded. */
void wined3d_texture_apply_sampler_desc(struct wined3d_texture *texture,
        const struct wined3d_sampler_desc *sampler_desc, const struct wined3d_context *context)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    GLenum target = texture->target;
    struct gl_texture *gl_tex;
    DWORD state;

    TRACE("texture %p, sampler_desc %p, context %p.\n", texture, sampler_desc, context);

    gl_tex = wined3d_texture_get_gl_texture(texture, texture->flags & WINED3D_TEXTURE_IS_SRGB);

    state = sampler_desc->address_u;
    if (state != gl_tex->sampler_desc.address_u)
    {
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_S,
                gl_info->wrap_lookup[state - WINED3D_TADDRESS_WRAP]);
        gl_tex->sampler_desc.address_u = state;
    }

    state = sampler_desc->address_v;
    if (state != gl_tex->sampler_desc.address_v)
    {
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_T,
                gl_info->wrap_lookup[state - WINED3D_TADDRESS_WRAP]);
        gl_tex->sampler_desc.address_v = state;
    }

    state = sampler_desc->address_w;
    if (state != gl_tex->sampler_desc.address_w)
    {
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_R,
                gl_info->wrap_lookup[state - WINED3D_TADDRESS_WRAP]);
        gl_tex->sampler_desc.address_w = state;
    }

    if (memcmp(gl_tex->sampler_desc.border_color, sampler_desc->border_color,
            sizeof(gl_tex->sampler_desc.border_color)))
    {
        gl_info->gl_ops.gl.p_glTexParameterfv(target, GL_TEXTURE_BORDER_COLOR, &sampler_desc->border_color[0]);
        memcpy(gl_tex->sampler_desc.border_color, sampler_desc->border_color,
                sizeof(gl_tex->sampler_desc.border_color));
    }

    state = sampler_desc->mag_filter;
    if (state != gl_tex->sampler_desc.mag_filter)
    {
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_MAG_FILTER, wined3d_gl_mag_filter(state));
        gl_tex->sampler_desc.mag_filter = state;
    }

    if (sampler_desc->min_filter != gl_tex->sampler_desc.min_filter
            || sampler_desc->mip_filter != gl_tex->sampler_desc.mip_filter)
    {
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_MIN_FILTER,
                wined3d_gl_min_mip_filter(sampler_desc->min_filter, sampler_desc->mip_filter));
        gl_tex->sampler_desc.min_filter = sampler_desc->min_filter;
        gl_tex->sampler_desc.mip_filter = sampler_desc->mip_filter;
    }

    state = sampler_desc->max_anisotropy;
    if (state != gl_tex->sampler_desc.max_anisotropy)
    {
        if (gl_info->supported[EXT_TEXTURE_FILTER_ANISOTROPIC])
            gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, state);
        else
            WARN("Anisotropic filtering not supported.\n");
        gl_tex->sampler_desc.max_anisotropy = state;
    }

    if (!sampler_desc->srgb_decode != !gl_tex->sampler_desc.srgb_decode
            && (context->d3d_info->wined3d_creation_flags & WINED3D_SRGB_READ_WRITE_CONTROL)
            && gl_info->supported[EXT_TEXTURE_SRGB_DECODE])
    {
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_SRGB_DECODE_EXT,
                sampler_desc->srgb_decode ? GL_DECODE_EXT : GL_SKIP_DECODE_EXT);
        gl_tex->sampler_desc.srgb_decode = sampler_desc->srgb_decode;
    }

    if (!sampler_desc->compare != !gl_tex->sampler_desc.compare)
    {
        if (sampler_desc->compare)
            gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE_ARB);
        else
            gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_COMPARE_MODE_ARB, GL_NONE);
        gl_tex->sampler_desc.compare = sampler_desc->compare;
    }

    checkGLcall("Texture parameter application");

    if (gl_info->supported[EXT_TEXTURE_LOD_BIAS])
    {
        gl_info->gl_ops.gl.p_glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT,
                GL_TEXTURE_LOD_BIAS_EXT, sampler_desc->lod_bias);
        checkGLcall("glTexEnvf(GL_TEXTURE_LOD_BIAS_EXT, ...)");
    }
}

ULONG CDECL wined3d_texture_incref(struct wined3d_texture *texture)
{
    ULONG refcount;

    TRACE("texture %p, swapchain %p.\n", texture, texture->swapchain);

    if (texture->swapchain)
        return wined3d_swapchain_incref(texture->swapchain);

    refcount = InterlockedIncrement(&texture->resource.ref);
    TRACE("%p increasing refcount to %u.\n", texture, refcount);

    return refcount;
}

ULONG CDECL wined3d_texture_decref(struct wined3d_texture *texture)
{
    ULONG refcount;

    TRACE("texture %p, swapchain %p.\n", texture, texture->swapchain);

    if (texture->swapchain)
        return wined3d_swapchain_decref(texture->swapchain);

    refcount = InterlockedDecrement(&texture->resource.ref);
    TRACE("%p decreasing refcount to %u.\n", texture, refcount);

    if (!refcount)
    {
#if defined(STAGING_CSMT)
        void *parent = texture->resource.parent;
        const struct wined3d_parent_ops *parent_ops = texture->resource.parent_ops;
        wined3d_texture_cleanup(texture);
        parent_ops->wined3d_object_destroyed(parent);
#else  /* STAGING_CSMT */
        wined3d_texture_cleanup(texture);
        texture->resource.parent_ops->wined3d_object_destroyed(texture->resource.parent);
        HeapFree(GetProcessHeap(), 0, texture);
#endif /* STAGING_CSMT */
    }

    return refcount;
}

struct wined3d_resource * CDECL wined3d_texture_get_resource(struct wined3d_texture *texture)
{
    TRACE("texture %p.\n", texture);

    return &texture->resource;
}

static BOOL color_key_equal(const struct wined3d_color_key *c1, struct wined3d_color_key *c2)
{
    return c1->color_space_low_value == c2->color_space_low_value
            && c1->color_space_high_value == c2->color_space_high_value;
}

/* Context activation is done by the caller */
void wined3d_texture_load(struct wined3d_texture *texture,
        struct wined3d_context *context, BOOL srgb)
{
    UINT sub_count = texture->level_count * texture->layer_count;
    const struct wined3d_d3d_info *d3d_info = context->d3d_info;
    DWORD flag;
    UINT i;

    TRACE("texture %p, context %p, srgb %#x.\n", texture, context, srgb);

    if (!needs_separate_srgb_gl_texture(context))
        srgb = FALSE;

    if (srgb)
        flag = WINED3D_TEXTURE_SRGB_VALID;
    else
        flag = WINED3D_TEXTURE_RGB_VALID;

    if (!d3d_info->shader_color_key
            && (!(texture->async.flags & WINED3D_TEXTURE_ASYNC_COLOR_KEY)
            != !(texture->async.color_key_flags & WINED3D_CKEY_SRC_BLT)
            || (texture->async.flags & WINED3D_TEXTURE_ASYNC_COLOR_KEY
            && !color_key_equal(&texture->async.gl_color_key, &texture->async.src_blt_color_key))))
    {
        unsigned int sub_count = texture->level_count * texture->layer_count;
        unsigned int i;

        TRACE("Reloading because of color key value change.\n");
        for (i = 0; i < sub_count; i++)
            texture->texture_ops->texture_sub_resource_add_dirty_region(texture->sub_resources[i].resource, NULL);
        wined3d_texture_set_dirty(texture);

        texture->async.gl_color_key = texture->async.src_blt_color_key;
    }

    if (texture->flags & flag)
    {
        TRACE("Texture %p not dirty, nothing to do.\n", texture);
        return;
    }

    /* Reload the surfaces if the texture is marked dirty. */
    for (i = 0; i < sub_count; ++i)
    {
        texture->texture_ops->texture_sub_resource_load(texture->sub_resources[i].resource, context, srgb);
    }
    texture->flags |= flag;
}

void CDECL wined3d_texture_preload(struct wined3d_texture *texture)
{
#if defined(STAGING_CSMT)
    const struct wined3d_device *device = texture->resource.device;
    wined3d_cs_emit_texture_preload(device->cs, texture);
#else  /* STAGING_CSMT */
    struct wined3d_context *context;
    context = context_acquire(texture->resource.device, NULL);
    wined3d_texture_load(texture, context, texture->flags & WINED3D_TEXTURE_IS_SRGB);
    context_release(context);
#endif /* STAGING_CSMT */
}

void * CDECL wined3d_texture_get_parent(const struct wined3d_texture *texture)
{
    TRACE("texture %p.\n", texture);

    return texture->resource.parent;
}

void CDECL wined3d_texture_get_pitch(const struct wined3d_texture *texture,
        unsigned int level, unsigned int *row_pitch, unsigned int *slice_pitch)
{
    const struct wined3d_resource *resource = &texture->resource;
#if defined(STAGING_CSMT)
    const struct wined3d_format *format = resource->format;
    unsigned int alignment = resource->device->surface_alignment;
    unsigned int width = max(1, texture->resource.width >> level);
    unsigned int height = max(1, texture->resource.height >> level);

    if (texture->row_pitch)
    {
        *row_pitch = texture->row_pitch;
        *slice_pitch = *row_pitch * height;
        return;
    }

    if (resource->format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_BLOCKS)
    {
        unsigned int row_block_count = (width + format->block_width - 1) / format->block_width;
        unsigned int slice_block_count = (height + format->block_height - 1) / format->block_height;
        *row_pitch = row_block_count * format->block_byte_count;
        *row_pitch = (*row_pitch + alignment - 1) & ~(alignment - 1);
        *slice_pitch = *row_pitch * slice_block_count;
    }
    else
    {
        *row_pitch = format->byte_count * width;  /* Bytes / row */
        *row_pitch = (*row_pitch + alignment - 1) & ~(alignment - 1);
        *slice_pitch = *row_pitch * height;
    }

    if (format->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_HEIGHT_SCALE)
    {
        /* The D3D format requirements make sure that the resulting format is an integer again */
        *slice_pitch *= format->height_scale.numerator;
        *slice_pitch /= format->height_scale.denominator;
    }
#else  /* STAGING_CSMT */
    unsigned int width = max(1, texture->resource.width >> level);
    unsigned int height = max(1, texture->resource.height >> level);

    if (texture->row_pitch)
    {
        *row_pitch = texture->row_pitch;
        *slice_pitch = texture->slice_pitch;
        return;
    }

    wined3d_format_calculate_pitch(resource->format, resource->device->surface_alignment,
            width, height, row_pitch, slice_pitch);
#endif /* STAGING_CSMT */
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
#if defined(STAGING_CSMT)
        if (wined3d_settings.cs_multithreaded)
        {
            struct wined3d_device *device = texture->resource.device;
            FIXME("Waiting for cs.\n");
            device->cs->ops->finish(device->cs);
        }

#endif /* STAGING_CSMT */
        texture->lod = lod;

        texture->texture_rgb.base_level = ~0u;
        texture->texture_srgb.base_level = ~0u;
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
    struct wined3d_device *device = texture->resource.device;
    static const DWORD all_flags = WINED3D_CKEY_DST_BLT | WINED3D_CKEY_DST_OVERLAY
            | WINED3D_CKEY_SRC_BLT | WINED3D_CKEY_SRC_OVERLAY;

    TRACE("texture %p, flags %#x, color_key %p.\n", texture, flags, color_key);

    if (flags & ~all_flags)
    {
        WARN("Invalid flags passed, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    wined3d_cs_emit_set_color_key(device->cs, texture, flags, color_key);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_texture_update_desc(struct wined3d_texture *texture, UINT width, UINT height,
        enum wined3d_format_id format_id, enum wined3d_multisample_type multisample_type,
        UINT multisample_quality, void *mem, UINT pitch)
{
    struct wined3d_device *device = texture->resource.device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    const struct wined3d_format *format = wined3d_get_format(gl_info, format_id);
    UINT resource_size = wined3d_format_calculate_size(format, device->surface_alignment, width, height, 1);
    struct wined3d_surface *surface;

    TRACE("texture %p, width %u, height %u, format %s, multisample_type %#x, multisample_quality %u, "
            "mem %p, pitch %u.\n",
            texture, width, height, debug_d3dformat(format_id), multisample_type, multisample_type, mem, pitch);

    if (!resource_size)
        return WINED3DERR_INVALIDCALL;

    if (texture->level_count * texture->layer_count > 1)
    {
        WARN("Texture has multiple sub-resources, not supported.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (texture->resource.type == WINED3D_RTYPE_TEXTURE_3D)
    {
        WARN("Not supported on 3D textures.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* We have no way of supporting a pitch that is not a multiple of the pixel
     * byte width short of uploading the texture row-by-row.
     * Fortunately that's not an issue since D3D9Ex doesn't allow a custom pitch
     * for user-memory textures (it always expects packed data) while DirectDraw
     * requires a 4-byte aligned pitch and doesn't support texture formats
     * larger than 4 bytes per pixel nor any format using 3 bytes per pixel.
     * This check is here to verify that the assumption holds. */
    if (pitch % texture->resource.format->byte_count)
    {
        WARN("Pitch unsupported, not a multiple of the texture format byte width.\n");
        return WINED3DERR_INVALIDCALL;
    }

    surface = surface_from_resource(texture->sub_resources[0].resource);
    if (surface->resource.map_count || (surface->flags & SFLAG_DCINUSE))
    {
        WARN("Surface is mapped or the DC is in use.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (device->d3d_initialized)
#if defined(STAGING_CSMT)
    {
        wined3d_cs_emit_evict_resource(device->cs, &surface->resource);
        device->cs->ops->finish(device->cs);
    }
#else  /* STAGING_CSMT */
        texture->resource.resource_ops->resource_unload(&texture->resource);
#endif /* STAGING_CSMT */

    texture->resource.format = format;
    texture->resource.multisample_type = multisample_type;
    texture->resource.multisample_quality = multisample_quality;
    texture->resource.width = width;
    texture->resource.height = height;

#if defined(STAGING_CSMT)
    texture->row_pitch = pitch;

    texture->flags &= ~WINED3D_TEXTURE_COND_NP2_EMULATED;
    if (((width & (width - 1)) || (height & (height - 1))) && !gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO]
            && !gl_info->supported[ARB_TEXTURE_RECTANGLE] && !gl_info->supported[WINED3D_GL_NORMALIZED_TEXRECT])
        texture->flags |= WINED3D_TEXTURE_COND_NP2_EMULATED;

    return wined3d_surface_update_desc(surface, gl_info, mem, pitch);
#else  /* STAGING_CSMT */
    texture->user_memory = mem;
    if ((texture->row_pitch = pitch))
        texture->slice_pitch = height * pitch;
    else
        /* User memory surfaces don't have the regular surface alignment. */
        wined3d_format_calculate_pitch(format, 1, width, height,
                &texture->row_pitch, &texture->slice_pitch);

    texture->flags &= ~WINED3D_TEXTURE_COND_NP2_EMULATED;
    if (((width & (width - 1)) || (height & (height - 1))) && !gl_info->supported[ARB_TEXTURE_NON_POWER_OF_TWO]
            && !gl_info->supported[ARB_TEXTURE_RECTANGLE] && !gl_info->supported[WINED3D_GL_NORMALIZED_TEXRECT])
        texture->flags |= WINED3D_TEXTURE_COND_NP2_EMULATED;

    return wined3d_surface_update_desc(surface, gl_info);
#endif /* STAGING_CSMT */
}

void wined3d_texture_prepare_texture(struct wined3d_texture *texture, struct wined3d_context *context, BOOL srgb)
{
    DWORD alloc_flag = srgb ? WINED3D_TEXTURE_SRGB_ALLOCATED : WINED3D_TEXTURE_RGB_ALLOCATED;
    const struct wined3d_d3d_info *d3d_info = context->d3d_info;

    if (!d3d_info->shader_color_key
            && !(texture->async.flags & WINED3D_TEXTURE_ASYNC_COLOR_KEY)
            != !(texture->async.color_key_flags & WINED3D_CKEY_SRC_BLT))
    {
        wined3d_texture_force_reload(texture);

        if (texture->async.color_key_flags & WINED3D_CKEY_SRC_BLT)
            texture->async.flags |= WINED3D_TEXTURE_ASYNC_COLOR_KEY;
    }

    if (texture->flags & alloc_flag)
        return;

    texture->texture_ops->texture_prepare_texture(texture, context, srgb);
    texture->flags |= alloc_flag;
}

void wined3d_texture_force_reload(struct wined3d_texture *texture)
{
    unsigned int sub_count = texture->level_count * texture->layer_count;
    unsigned int i;

    texture->flags &= ~(WINED3D_TEXTURE_RGB_ALLOCATED | WINED3D_TEXTURE_SRGB_ALLOCATED
            | WINED3D_TEXTURE_CONVERTED);
    texture->async.flags &= ~WINED3D_TEXTURE_ASYNC_COLOR_KEY;
    for (i = 0; i < sub_count; ++i)
    {
        texture->texture_ops->texture_sub_resource_invalidate_location(texture->sub_resources[i].resource,
                WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_TEXTURE_SRGB);
    }
}

void CDECL wined3d_texture_generate_mipmaps(struct wined3d_texture *texture)
{
    /* TODO: Implement filters using GL_SGI_generate_mipmaps. */
    FIXME("texture %p stub!\n", texture);
}

struct wined3d_resource * CDECL wined3d_texture_get_sub_resource(const struct wined3d_texture *texture,
        UINT sub_resource_idx)
{
    UINT sub_count = texture->level_count * texture->layer_count;

    TRACE("texture %p, sub_resource_idx %u.\n", texture, sub_resource_idx);

    if (sub_resource_idx >= sub_count)
    {
        WARN("sub_resource_idx %u >= sub_count %u.\n", sub_resource_idx, sub_count);
        return NULL;
    }

    return texture->sub_resources[sub_resource_idx].resource;
}

HRESULT CDECL wined3d_texture_add_dirty_region(struct wined3d_texture *texture,
        UINT layer, const struct wined3d_box *dirty_region)
{
    struct wined3d_resource *sub_resource;

    TRACE("texture %p, layer %u, dirty_region %s.\n", texture, layer, debug_box(dirty_region));

    if (!(sub_resource = wined3d_texture_get_sub_resource(texture, layer * texture->level_count)))
    {
        WARN("Failed to get sub-resource.\n");
        return WINED3DERR_INVALIDCALL;
    }

    texture->texture_ops->texture_sub_resource_add_dirty_region(sub_resource, dirty_region);

    return WINED3D_OK;
}

static HRESULT wined3d_texture_upload_data(struct wined3d_texture *texture,
        const struct wined3d_sub_resource_data *data)
{
    unsigned int sub_count = texture->level_count * texture->layer_count;
    struct wined3d_context *context;
    unsigned int i;

    for (i = 0; i < sub_count; ++i)
    {
        if (!data[i].data)
        {
            WARN("Invalid sub-resource data specified for sub-resource %u.\n", i);
            return E_INVALIDARG;
        }
    }

    context = context_acquire(texture->resource.device, NULL);

    wined3d_texture_prepare_texture(texture, context, FALSE);
    wined3d_texture_bind_and_dirtify(texture, context, FALSE);

    for (i = 0; i < sub_count; ++i)
    {
        struct wined3d_resource *sub_resource = texture->sub_resources[i].resource;

        texture->texture_ops->texture_sub_resource_upload_data(sub_resource, context, &data[i]);
        texture->texture_ops->texture_sub_resource_validate_location(sub_resource, WINED3D_LOCATION_TEXTURE_RGB);
        texture->texture_ops->texture_sub_resource_invalidate_location(sub_resource, ~WINED3D_LOCATION_TEXTURE_RGB);
    }

    context_release(context);

    return WINED3D_OK;
}

static void texture2d_sub_resource_load(struct wined3d_resource *sub_resource,
        struct wined3d_context *context, BOOL srgb)
{
    surface_load(surface_from_resource(sub_resource), context, srgb);
}

static void texture2d_sub_resource_add_dirty_region(struct wined3d_resource *sub_resource,
        const struct wined3d_box *dirty_region)
{
    struct wined3d_surface *surface = surface_from_resource(sub_resource);
    struct wined3d_context *context;

#if defined(STAGING_CSMT)
    context = context_acquire(surface->resource.device, NULL);
    wined3d_resource_prepare_map_memory(&surface->resource, context);
    wined3d_resource_load_location(&surface->resource, context, surface->resource.map_binding);
    context_release(context);
    wined3d_resource_invalidate_location(&surface->resource, ~surface->resource.map_binding);
#else  /* STAGING_CSMT */
    surface_prepare_map_memory(surface);
    context = context_acquire(surface->resource.device, NULL);
    surface_load_location(surface, context, surface->resource.map_binding);
    context_release(context);
    surface_invalidate_location(surface, ~surface->resource.map_binding);
#endif /* STAGING_CSMT */
}

static void texture2d_sub_resource_cleanup(struct wined3d_resource *sub_resource)
{
    struct wined3d_surface *surface = surface_from_resource(sub_resource);

    wined3d_surface_destroy(surface);
}

static void texture2d_sub_resource_invalidate_location(struct wined3d_resource *sub_resource, DWORD location)
{
#if defined(STAGING_CSMT)
    wined3d_resource_invalidate_location(sub_resource, location);
}

static void texture2d_sub_resource_validate_location(struct wined3d_resource *sub_resource, DWORD location)
{
    wined3d_resource_validate_location(sub_resource, location);
#else  /* STAGING_CSMT */
    struct wined3d_surface *surface = surface_from_resource(sub_resource);

    surface_invalidate_location(surface, location);
}

static void texture2d_sub_resource_validate_location(struct wined3d_resource *sub_resource, DWORD location)
{
    struct wined3d_surface *surface = surface_from_resource(sub_resource);

    surface_validate_location(surface, location);
#endif /* STAGING_CSMT */
}

static void texture2d_sub_resource_upload_data(struct wined3d_resource *sub_resource,
        const struct wined3d_context *context, const struct wined3d_sub_resource_data *data)
{
    struct wined3d_surface *surface = surface_from_resource(sub_resource);
    static const POINT dst_point = {0, 0};
    struct wined3d_const_bo_address addr;
    RECT src_rect;

    src_rect.left = 0;
    src_rect.top = 0;
    src_rect.right = surface->resource.width;
    src_rect.bottom = surface->resource.height;

    addr.buffer_object = 0;
    addr.addr = data->data;

    wined3d_surface_upload_data(surface, context->gl_info, surface->container->resource.format,
            &src_rect, data->row_pitch, &dst_point, FALSE, &addr);
}

/* Context activation is done by the caller. */
static void texture2d_prepare_texture(struct wined3d_texture *texture, struct wined3d_context *context, BOOL srgb)
{
    UINT sub_count = texture->level_count * texture->layer_count;
    const struct wined3d_format *format = texture->resource.format;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    const struct wined3d_color_key_conversion *conversion;
    GLenum internal;
    UINT i;

    TRACE("texture %p, context %p, format %s.\n", texture, context, debug_d3dformat(format->id));

    if (format->convert)
    {
        texture->flags |= WINED3D_TEXTURE_CONVERTED;
    }
    else if ((conversion = wined3d_format_get_color_key_conversion(texture, TRUE)))
    {
        texture->flags |= WINED3D_TEXTURE_CONVERTED;
        format = wined3d_get_format(gl_info, conversion->dst_format);
        TRACE("Using format %s for color key conversion.\n", debug_d3dformat(format->id));
    }

    wined3d_texture_bind_and_dirtify(texture, context, srgb);

    if (srgb)
        internal = format->glGammaInternal;
    else if (texture->resource.usage & WINED3DUSAGE_RENDERTARGET
            && wined3d_resource_is_offscreen(&texture->resource))
        internal = format->rtInternal;
    else
        internal = format->glInternal;

    if (!internal)
        FIXME("No GL internal format for format %s.\n", debug_d3dformat(format->id));

    TRACE("internal %#x, format %#x, type %#x.\n", internal, format->glFormat, format->glType);

    for (i = 0; i < sub_count; ++i)
    {
        struct wined3d_surface *surface = surface_from_resource(texture->sub_resources[i].resource);
        GLsizei height = surface->pow2Height;
        GLsizei width = surface->pow2Width;

        if (texture->resource.format_flags & WINED3DFMT_FLAG_HEIGHT_SCALE)
        {
            height *= format->height_scale.numerator;
            height /= format->height_scale.denominator;
        }

        TRACE("surface %p, target %#x, level %d, width %d, height %d.\n",
                surface, surface->texture_target, surface->texture_level, width, height);

        gl_info->gl_ops.gl.p_glTexImage2D(surface->texture_target, surface->texture_level,
                internal, width, height, 0, format->glFormat, format->glType, NULL);
        checkGLcall("glTexImage2D");
    }
}

static const struct wined3d_texture_ops texture2d_ops =
{
    texture2d_sub_resource_load,
    texture2d_sub_resource_add_dirty_region,
    texture2d_sub_resource_cleanup,
    texture2d_sub_resource_invalidate_location,
    texture2d_sub_resource_validate_location,
    texture2d_sub_resource_upload_data,
    texture2d_prepare_texture,
};

static ULONG texture_resource_incref(struct wined3d_resource *resource)
{
    return wined3d_texture_incref(wined3d_texture_from_resource(resource));
}

static ULONG texture_resource_decref(struct wined3d_resource *resource)
{
    return wined3d_texture_decref(wined3d_texture_from_resource(resource));
}

static void wined3d_texture_unload(struct wined3d_resource *resource)
{
    struct wined3d_texture *texture = wined3d_texture_from_resource(resource);
    UINT sub_count = texture->level_count * texture->layer_count;
    UINT i;

    TRACE("texture %p.\n", texture);

    for (i = 0; i < sub_count; ++i)
    {
        struct wined3d_resource *sub_resource = texture->sub_resources[i].resource;

        sub_resource->resource_ops->resource_unload(sub_resource);
    }

    wined3d_texture_force_reload(texture);
    wined3d_texture_unload_gl_texture(texture);
}

static HRESULT texture2d_resource_sub_resource_map(struct wined3d_resource *resource, unsigned int sub_resource_idx,
        struct wined3d_map_desc *map_desc, const struct wined3d_box *box, DWORD flags)
{
    struct wined3d_resource *sub_resource;

    if (!(sub_resource = wined3d_texture_get_sub_resource(wined3d_texture_from_resource(resource), sub_resource_idx)))
        return E_INVALIDARG;

    return wined3d_surface_map(surface_from_resource(sub_resource), map_desc, box, flags);
}

static HRESULT texture2d_resource_sub_resource_unmap(struct wined3d_resource *resource, unsigned int sub_resource_idx)
{
    struct wined3d_resource *sub_resource;

    if (!(sub_resource = wined3d_texture_get_sub_resource(wined3d_texture_from_resource(resource), sub_resource_idx)))
        return E_INVALIDARG;

    return wined3d_surface_unmap(surface_from_resource(sub_resource));
}

#if defined(STAGING_CSMT)
static void wined3d_texture_load_location_invalidated(struct wined3d_resource *resource, DWORD location)
{
    ERR("Should not be called on textures.\n");
}

/* Context activation is done by the caller. */
static void wined3d_texture_load_location(struct wined3d_resource *resource,
        struct wined3d_context *context, DWORD location)
{
    ERR("Should not be called on textures.\n");
}

#endif /* STAGING_CSMT */
static const struct wined3d_resource_ops texture2d_resource_ops =
{
    texture_resource_incref,
    texture_resource_decref,
    wined3d_texture_unload,
    texture2d_resource_sub_resource_map,
    texture2d_resource_sub_resource_unmap,
#if defined(STAGING_CSMT)
    wined3d_texture_load_location_invalidated,
    wined3d_texture_load_location,
#endif /* STAGING_CSMT */
};

static HRESULT texture_init(struct wined3d_texture *texture, const struct wined3d_resource_desc *desc,
        unsigned int layer_count, unsigned int level_count, DWORD flags, struct wined3d_device *device,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_resource_desc surface_desc;
    UINT pow2_width, pow2_height;
    unsigned int i, j;
    HRESULT hr;

    /* TODO: It should only be possible to create textures for formats
     * that are reported as supported. */
    if (WINED3DFMT_UNKNOWN >= desc->format)
    {
        WARN("(%p) : Texture cannot be created with a format of WINED3DFMT_UNKNOWN.\n", texture);
#if defined(STAGING_CSMT)
        HeapFree(GetProcessHeap(), 0, texture);
#endif /* STAGING_CSMT */
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
            /* level_count == 0 returns an error as well */
            if (level_count != 1 || desc->usage & WINED3DUSAGE_LEGACY_CUBEMAP)
            {
                if (desc->pool == WINED3D_POOL_SCRATCH)
                {
                    WARN("Creating a scratch mipmapped/cube NPOT texture despite lack of HW support.\n");
                }
                else
                {
                    WARN("Attempted to create a mipmapped/cube NPOT texture without unconditional NPOT support.\n");
#if defined(STAGING_CSMT)
                    HeapFree(GetProcessHeap(), 0, texture);
#endif /* STAGING_CSMT */
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
#if defined(STAGING_CSMT)
            HeapFree(GetProcessHeap(), 0, texture);
            return WINED3DERR_INVALIDCALL;
        }

        if (level_count != 1)
        {
            WARN("WINED3DUSAGE_AUTOGENMIPMAP is set, and level count != 1, returning WINED3DERR_INVALIDCALL.\n");
            HeapFree(GetProcessHeap(), 0, texture);
#else  /* STAGING_CSMT */
            return WINED3DERR_INVALIDCALL;
        }

        if (level_count != 1)
        {
            WARN("WINED3DUSAGE_AUTOGENMIPMAP is set, and level count != 1, returning WINED3DERR_INVALIDCALL.\n");
#endif /* STAGING_CSMT */
            return WINED3DERR_INVALIDCALL;
        }
    }

    if (FAILED(hr = wined3d_texture_init(texture, &texture2d_ops, layer_count, level_count, desc,
            flags, device, parent, parent_ops, &texture2d_resource_ops)))
    {
        WARN("Failed to initialize texture, returning %#x.\n", hr);
#if defined(STAGING_CSMT)
        HeapFree(GetProcessHeap(), 0, texture);
#endif /* STAGING_CSMT */
        return hr;
    }

    /* Precalculated scaling for 'faked' non power of two texture coords. */
    if (texture->resource.gl_type == WINED3D_GL_RES_TYPE_TEX_RECT)
    {
        texture->pow2_matrix[0] = (float)desc->width;
        texture->pow2_matrix[5] = (float)desc->height;
        texture->pow2_matrix[10] = 1.0f;
        texture->pow2_matrix[15] = 1.0f;
        texture->target = GL_TEXTURE_RECTANGLE_ARB;
        texture->flags |= WINED3D_TEXTURE_COND_NP2;
        texture->flags &= ~(WINED3D_TEXTURE_POW2_MAT_IDENT | WINED3D_TEXTURE_NORMALIZED_COORDS);
    }
    else
    {
        if (desc->usage & WINED3DUSAGE_LEGACY_CUBEMAP)
            texture->target = GL_TEXTURE_CUBE_MAP_ARB;
        else
            texture->target = GL_TEXTURE_2D;
        if (desc->width == pow2_width && desc->height == pow2_height)
        {
            texture->pow2_matrix[0] = 1.0f;
            texture->pow2_matrix[5] = 1.0f;
        }
        else if (gl_info->supported[WINED3D_GL_NORMALIZED_TEXRECT])
        {
            texture->pow2_matrix[0] = 1.0f;
            texture->pow2_matrix[5] = 1.0f;
            texture->flags |= WINED3D_TEXTURE_COND_NP2;
        }
        else
        {
            texture->pow2_matrix[0] = (((float)desc->width) / ((float)pow2_width));
            texture->pow2_matrix[5] = (((float)desc->height) / ((float)pow2_height));
            texture->flags &= ~WINED3D_TEXTURE_POW2_MAT_IDENT;
            texture->flags |= WINED3D_TEXTURE_COND_NP2_EMULATED;
        }
        texture->pow2_matrix[10] = 1.0f;
        texture->pow2_matrix[15] = 1.0f;
    }
    TRACE("xf(%f) yf(%f)\n", texture->pow2_matrix[0], texture->pow2_matrix[5]);

    /* Generate all the surfaces. */
    surface_desc = *desc;
    surface_desc.resource_type = WINED3D_RTYPE_SURFACE;
    for (i = 0; i < texture->level_count; ++i)
    {
        for (j = 0; j < texture->layer_count; ++j)
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
            GLenum target = desc->usage & WINED3DUSAGE_LEGACY_CUBEMAP ? cube_targets[j] : texture->target;
            unsigned int idx = j * texture->level_count + i;
            struct wined3d_surface *surface;

            if (FAILED(hr = wined3d_surface_create(texture, &surface_desc,
                    target, i, j, flags, &surface)))
            {
                WARN("Failed to create surface, hr %#x.\n", hr);
                wined3d_texture_cleanup(texture);
                return hr;
            }

            texture->sub_resources[idx].resource = &surface->resource;
            TRACE("Created surface level %u @ %p.\n", i, surface);
        }
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

    wined3d_volume_destroy(volume);
}

static void texture3d_sub_resource_invalidate_location(struct wined3d_resource *sub_resource, DWORD location)
{
#if defined(STAGING_CSMT)
    wined3d_resource_invalidate_location(sub_resource, location);
}

static void texture3d_sub_resource_validate_location(struct wined3d_resource *sub_resource, DWORD location)
{
    wined3d_resource_validate_location(sub_resource, location);
#else  /* STAGING_CSMT */
    struct wined3d_volume *volume = volume_from_resource(sub_resource);

    wined3d_volume_invalidate_location(volume, location);
}

static void texture3d_sub_resource_validate_location(struct wined3d_resource *sub_resource, DWORD location)
{
    struct wined3d_volume *volume = volume_from_resource(sub_resource);

    wined3d_volume_validate_location(volume, location);
#endif /* STAGING_CSMT */
}

static void texture3d_sub_resource_upload_data(struct wined3d_resource *sub_resource,
        const struct wined3d_context *context, const struct wined3d_sub_resource_data *data)
{
    struct wined3d_volume *volume = volume_from_resource(sub_resource);
    struct wined3d_const_bo_address addr;
    unsigned int row_pitch, slice_pitch;

#if defined(STAGING_CSMT)
    wined3d_resource_get_pitch(sub_resource, &row_pitch, &slice_pitch);
#else  /* STAGING_CSMT */
    wined3d_texture_get_pitch(volume->container, volume->texture_level, &row_pitch, &slice_pitch);
#endif /* STAGING_CSMT */
    if (row_pitch != data->row_pitch || slice_pitch != data->slice_pitch)
        FIXME("Ignoring row/slice pitch (%u/%u).\n", data->row_pitch, data->slice_pitch);

    addr.buffer_object = 0;
    addr.addr = data->data;

    wined3d_volume_upload_data(volume, context, &addr);
}

static void texture3d_prepare_texture(struct wined3d_texture *texture, struct wined3d_context *context, BOOL srgb)
{
    unsigned int sub_count = texture->level_count * texture->layer_count;
    const struct wined3d_format *format = texture->resource.format;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    unsigned int i;

    wined3d_texture_bind_and_dirtify(texture, context, srgb);

    for (i = 0; i < sub_count; ++i)
    {
        struct wined3d_volume *volume = volume_from_resource(texture->sub_resources[i].resource);

        GL_EXTCALL(glTexImage3D(GL_TEXTURE_3D, volume->texture_level,
                srgb ? format->glGammaInternal : format->glInternal,
                volume->resource.width, volume->resource.height, volume->resource.depth,
                0, format->glFormat, format->glType, NULL));
        checkGLcall("glTexImage3D");
    }
}

static const struct wined3d_texture_ops texture3d_ops =
{
    texture3d_sub_resource_load,
    texture3d_sub_resource_add_dirty_region,
    texture3d_sub_resource_cleanup,
    texture3d_sub_resource_invalidate_location,
    texture3d_sub_resource_validate_location,
    texture3d_sub_resource_upload_data,
    texture3d_prepare_texture,
};

#if !defined(STAGING_CSMT)
BOOL wined3d_texture_check_block_align(const struct wined3d_texture *texture,
        unsigned int level, const struct wined3d_box *box)
{
    const struct wined3d_format *format = texture->resource.format;
    unsigned int height = max(1, texture->resource.height >> level);
    unsigned int width = max(1, texture->resource.width >> level);
    unsigned int width_mask, height_mask;

    if ((box->left >= box->right)
            || (box->top >= box->bottom)
            || (box->right > width)
            || (box->bottom > height))
        return FALSE;

    /* This assumes power of two block sizes, but NPOT block sizes would be
     * silly anyway.
     *
     * This also assumes that the format's block depth is 1. */
    width_mask = format->block_width - 1;
    height_mask = format->block_height - 1;

    if ((box->left & width_mask) || (box->top & height_mask)
            || (box->right & width_mask && box->right != width)
            || (box->bottom & height_mask && box->bottom != height))
        return FALSE;

    return TRUE;
}

#endif /* STAGING_CSMT */
static HRESULT texture3d_resource_sub_resource_map(struct wined3d_resource *resource, unsigned int sub_resource_idx,
        struct wined3d_map_desc *map_desc, const struct wined3d_box *box, DWORD flags)
{
    struct wined3d_resource *sub_resource;

    if (!(sub_resource = wined3d_texture_get_sub_resource(wined3d_texture_from_resource(resource), sub_resource_idx)))
        return E_INVALIDARG;

    return wined3d_volume_map(volume_from_resource(sub_resource), map_desc, box, flags);
}

static HRESULT texture3d_resource_sub_resource_unmap(struct wined3d_resource *resource, unsigned int sub_resource_idx)
{
    struct wined3d_resource *sub_resource;

    if (!(sub_resource = wined3d_texture_get_sub_resource(wined3d_texture_from_resource(resource), sub_resource_idx)))
        return E_INVALIDARG;

    return wined3d_volume_unmap(volume_from_resource(sub_resource));
}

static const struct wined3d_resource_ops texture3d_resource_ops =
{
    texture_resource_incref,
    texture_resource_decref,
    wined3d_texture_unload,
    texture3d_resource_sub_resource_map,
    texture3d_resource_sub_resource_unmap,
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
#if defined(STAGING_CSMT)
        HeapFree(GetProcessHeap(), 0, texture);
        return WINED3DERR_INVALIDCALL;
    }

    if (!gl_info->supported[EXT_TEXTURE3D])
    {
        WARN("(%p) : Texture cannot be created - no volume texture support.\n", texture);
        HeapFree(GetProcessHeap(), 0, texture);
#else  /* STAGING_CSMT */
        return WINED3DERR_INVALIDCALL;
    }

    if (!gl_info->supported[EXT_TEXTURE3D])
    {
        WARN("(%p) : Texture cannot be created - no volume texture support.\n", texture);
#endif /* STAGING_CSMT */
        return WINED3DERR_INVALIDCALL;
    }

    /* Calculate levels for mip mapping. */
    if (desc->usage & WINED3DUSAGE_AUTOGENMIPMAP)
    {
        if (!gl_info->supported[SGIS_GENERATE_MIPMAP])
        {
            WARN("No mipmap generation support, returning D3DERR_INVALIDCALL.\n");
#if defined(STAGING_CSMT)
            HeapFree(GetProcessHeap(), 0, texture);
            return WINED3DERR_INVALIDCALL;
        }

        if (levels != 1)
        {
            WARN("WINED3DUSAGE_AUTOGENMIPMAP is set, and level count != 1, returning D3DERR_INVALIDCALL.\n");
            HeapFree(GetProcessHeap(), 0, texture);
#else  /* STAGING_CSMT */
            return WINED3DERR_INVALIDCALL;
        }

        if (levels != 1)
        {
            WARN("WINED3DUSAGE_AUTOGENMIPMAP is set, and level count != 1, returning D3DERR_INVALIDCALL.\n");
#endif /* STAGING_CSMT */
            return WINED3DERR_INVALIDCALL;
        }
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
#if defined(STAGING_CSMT)
                HeapFree(GetProcessHeap(), 0, texture);
#endif /* STAGING_CSMT */
                return WINED3DERR_INVALIDCALL;
            }
        }
    }

    if (FAILED(hr = wined3d_texture_init(texture, &texture3d_ops, 1, levels, desc,
            0, device, parent, parent_ops, &texture3d_resource_ops)))
    {
        WARN("Failed to initialize texture, returning %#x.\n", hr);
#if defined(STAGING_CSMT)
        HeapFree(GetProcessHeap(), 0, texture);
#endif /* STAGING_CSMT */
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

        texture->sub_resources[i].resource = &volume->resource;

        /* Calculate the next mipmap level. */
        volume_desc.width = max(1, volume_desc.width >> 1);
        volume_desc.height = max(1, volume_desc.height >> 1);
        volume_desc.depth = max(1, volume_desc.depth >> 1);
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_texture_blt(struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx,
        const RECT *dst_rect, struct wined3d_texture *src_texture, unsigned int src_sub_resource_idx,
        const RECT *src_rect, DWORD flags, const struct wined3d_blt_fx *fx, enum wined3d_texture_filter_type filter)
{
    struct wined3d_resource *dst_resource, *src_resource = NULL;

    TRACE("dst_texture %p, dst_sub_resource_idx %u, dst_rect %s, src_texture %p, "
            "src_sub_resource_idx %u, src_rect %s, flags %#x, fx %p, filter %s.\n",
            dst_texture, dst_sub_resource_idx, wine_dbgstr_rect(dst_rect), src_texture,
            src_sub_resource_idx, wine_dbgstr_rect(src_rect), flags, fx, debug_d3dtexturefiltertype(filter));

    if (!(dst_resource = wined3d_texture_get_sub_resource(dst_texture, dst_sub_resource_idx))
            || dst_resource->type != WINED3D_RTYPE_SURFACE)
        return WINED3DERR_INVALIDCALL;

    if (src_texture)
    {
        if (!(src_resource = wined3d_texture_get_sub_resource(src_texture, src_sub_resource_idx))
                || src_resource->type != WINED3D_RTYPE_SURFACE)
            return WINED3DERR_INVALIDCALL;
    }

    return wined3d_surface_blt(surface_from_resource(dst_resource), dst_rect,
            src_resource ? surface_from_resource(src_resource) : NULL, src_rect, flags, fx, filter);
}

HRESULT CDECL wined3d_texture_get_overlay_position(const struct wined3d_texture *texture,
        unsigned int sub_resource_idx, LONG *x, LONG *y)
{
    struct wined3d_resource *sub_resource;
    struct wined3d_surface *surface;

    TRACE("texture %p, sub_resource_idx %u, x %p, y %p.\n", texture, sub_resource_idx, x, y);

    if (!(texture->resource.usage & WINED3DUSAGE_OVERLAY) || texture->resource.type != WINED3D_RTYPE_TEXTURE_2D
            || !(sub_resource = wined3d_texture_get_sub_resource(texture, sub_resource_idx)))
    {
        WARN("Invalid sub-resource specified.\n");
        return WINEDDERR_NOTAOVERLAYSURFACE;
    }

    surface = surface_from_resource(sub_resource);
    if (!surface->overlay_dest)
    {
        TRACE("Overlay not visible.\n");
        *x = 0;
        *y = 0;
        return WINEDDERR_OVERLAYNOTVISIBLE;
    }

    *x = surface->overlay_destrect.left;
    *y = surface->overlay_destrect.top;

    TRACE("Returning position %d, %d.\n", *x, *y);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_texture_set_overlay_position(struct wined3d_texture *texture,
        unsigned int sub_resource_idx, LONG x, LONG y)
{
    struct wined3d_resource *sub_resource;
    struct wined3d_surface *surface;
    LONG w, h;

    TRACE("texture %p, sub_resource_idx %u, x %d, y %d.\n", texture, sub_resource_idx, x, y);

    if (!(texture->resource.usage & WINED3DUSAGE_OVERLAY) || texture->resource.type != WINED3D_RTYPE_TEXTURE_2D
            || !(sub_resource = wined3d_texture_get_sub_resource(texture, sub_resource_idx)))
    {
        WARN("Invalid sub-resource specified.\n");
        return WINEDDERR_NOTAOVERLAYSURFACE;
    }

    surface = surface_from_resource(sub_resource);
    w = surface->overlay_destrect.right - surface->overlay_destrect.left;
    h = surface->overlay_destrect.bottom - surface->overlay_destrect.top;
    surface->overlay_destrect.left = x;
    surface->overlay_destrect.top = y;
    surface->overlay_destrect.right = x + w;
    surface->overlay_destrect.bottom = y + h;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_texture_update_overlay(struct wined3d_texture *texture, unsigned int sub_resource_idx,
        const RECT *src_rect, struct wined3d_texture *dst_texture, unsigned int dst_sub_resource_idx,
        const RECT *dst_rect, DWORD flags)
{
    struct wined3d_resource *sub_resource, *dst_sub_resource;
    struct wined3d_surface *surface, *dst_surface;

    TRACE("texture %p, sub_resource_idx %u, src_rect %s, dst_texture %p, "
            "dst_sub_resource_idx %u, dst_rect %s, flags %#x.\n",
            texture, sub_resource_idx, wine_dbgstr_rect(src_rect), dst_texture,
            dst_sub_resource_idx, wine_dbgstr_rect(dst_rect), flags);

    if (!(texture->resource.usage & WINED3DUSAGE_OVERLAY) || texture->resource.type != WINED3D_RTYPE_TEXTURE_2D
            || !(sub_resource = wined3d_texture_get_sub_resource(texture, sub_resource_idx)))
    {
        WARN("Invalid sub-resource specified.\n");
        return WINEDDERR_NOTAOVERLAYSURFACE;
    }

    if (!dst_texture || dst_texture->resource.type != WINED3D_RTYPE_TEXTURE_2D
            || !(dst_sub_resource = wined3d_texture_get_sub_resource(dst_texture, dst_sub_resource_idx)))
    {
        WARN("Invalid destination sub-resource specified.\n");
        return WINED3DERR_INVALIDCALL;
    }

    surface = surface_from_resource(sub_resource);
    if (src_rect)
        surface->overlay_srcrect = *src_rect;
    else
        SetRect(&surface->overlay_srcrect, 0, 0, surface->resource.width, surface->resource.height);

    dst_surface = surface_from_resource(dst_sub_resource);
    if (dst_rect)
        surface->overlay_destrect = *dst_rect;
    else
        SetRect(&surface->overlay_destrect, 0, 0, dst_surface->resource.width, dst_surface->resource.height);

    if (surface->overlay_dest && (surface->overlay_dest != dst_surface || flags & WINEDDOVER_HIDE))
    {
        surface->overlay_dest = NULL;
        list_remove(&surface->overlay_entry);
    }

    if (flags & WINEDDOVER_SHOW)
    {
        if (surface->overlay_dest != dst_surface)
        {
            surface->overlay_dest = dst_surface;
            list_add_tail(&dst_surface->overlays, &surface->overlay_entry);
        }
    }
    else if (flags & WINEDDOVER_HIDE)
    {
        /* Tests show that the rectangles are erased on hide. */
        surface->overlay_srcrect.left = 0; surface->overlay_srcrect.top = 0;
        surface->overlay_srcrect.right = 0; surface->overlay_srcrect.bottom = 0;
        surface->overlay_destrect.left = 0; surface->overlay_destrect.top = 0;
        surface->overlay_destrect.right = 0; surface->overlay_destrect.bottom = 0;
        surface->overlay_dest = NULL;
    }

    return WINED3D_OK;
}

void * CDECL wined3d_texture_get_sub_resource_parent(struct wined3d_texture *texture, unsigned int sub_resource_idx)
{
    unsigned int sub_count = texture->level_count * texture->layer_count;

    TRACE("texture %p, sub_resource_idx %u.\n", texture, sub_resource_idx);

    if (sub_resource_idx >= sub_count)
    {
        WARN("sub_resource_idx %u >= sub_count %u.\n", sub_resource_idx, sub_count);
        return NULL;
    }

    return texture->sub_resources[sub_resource_idx].resource->parent;
}

HRESULT CDECL wined3d_texture_create(struct wined3d_device *device, const struct wined3d_resource_desc *desc,
        UINT level_count, DWORD flags, const struct wined3d_sub_resource_data *data, void *parent,
        const struct wined3d_parent_ops *parent_ops, struct wined3d_texture **texture)
{
    unsigned int layer_count = desc->usage & WINED3DUSAGE_LEGACY_CUBEMAP ? 6 : 1;
    struct wined3d_texture *object;
    HRESULT hr;

    TRACE("device %p, desc %p, level_count %u, flags %#x, data %p, parent %p, parent_ops %p, texture %p.\n",
            device, desc, level_count, flags, data, parent, parent_ops, texture);

    if (!level_count)
    {
        WARN("Invalid level count.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (desc->multisample_type != WINED3D_MULTISAMPLE_NONE)
    {
        const struct wined3d_format *format = wined3d_get_format(&device->adapter->gl_info, desc->format);

        if (desc->multisample_type == WINED3D_MULTISAMPLE_NON_MASKABLE
                && desc->multisample_quality >= wined3d_popcount(format->multisample_types))
        {
            WARN("Unsupported quality level %u requested for WINED3D_MULTISAMPLE_NON_MASKABLE.\n",
                    desc->multisample_quality);
            return WINED3DERR_NOTAVAILABLE;
        }
        if (desc->multisample_type != WINED3D_MULTISAMPLE_NON_MASKABLE
                && (!(format->multisample_types & 1u << (desc->multisample_type - 1))
                || desc->multisample_quality))
        {
            WARN("Unsupported multisample type %u quality %u requested.\n", desc->multisample_type,
                    desc->multisample_quality);
            return WINED3DERR_NOTAVAILABLE;
        }
    }

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            FIELD_OFFSET(struct wined3d_texture, sub_resources[level_count * layer_count]))))
        return E_OUTOFMEMORY;

    switch (desc->resource_type)
    {
        case WINED3D_RTYPE_TEXTURE_2D:
            hr = texture_init(object, desc, layer_count, level_count, flags, device, parent, parent_ops);
            break;

        case WINED3D_RTYPE_TEXTURE_3D:
            hr = volumetexture_init(object, desc, level_count, device, parent, parent_ops);
            break;

        default:
            ERR("Invalid resource type %s.\n", debug_d3dresourcetype(desc->resource_type));
            hr = WINED3DERR_INVALIDCALL;
            break;
    }

    if (FAILED(hr))
    {
        WARN("Failed to initialize texture, returning %#x.\n", hr);
#if !defined(STAGING_CSMT)
        HeapFree(GetProcessHeap(), 0, object);
#endif /* STAGING_CSMT */
        return hr;
    }

    /* FIXME: We'd like to avoid ever allocating system memory for the texture
     * in this case. */
    if (data && FAILED(hr = wined3d_texture_upload_data(object, data)))
    {
        wined3d_texture_cleanup(object);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created texture %p.\n", object);
    *texture = object;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_texture_get_dc(struct wined3d_texture *texture, unsigned int sub_resource_idx, HDC *dc)
{
    struct wined3d_device *device = texture->resource.device;
#if defined(STAGING_CSMT)
    struct wined3d_resource *sub_resource;
    struct wined3d_surface *surface;
#else  /* STAGING_CSMT */
    struct wined3d_context *context = NULL;
    struct wined3d_resource *sub_resource;
    struct wined3d_surface *surface;
    HRESULT hr;
#endif /* STAGING_CSMT */

    TRACE("texture %p, sub_resource_idx %u, dc %p.\n", texture, sub_resource_idx, dc);

    if (!(sub_resource = wined3d_texture_get_sub_resource(texture, sub_resource_idx)))
        return WINED3DERR_INVALIDCALL;

    if (sub_resource->type != WINED3D_RTYPE_SURFACE)
    {
        WARN("Not supported on %s resources.\n", debug_d3dresourcetype(texture->resource.type));
        return WINED3DERR_INVALIDCALL;
    }

    surface = surface_from_resource(sub_resource);

#if defined(STAGING_CSMT)
    if (!(surface->container->resource.format_flags & WINED3DFMT_FLAG_GETDC))
    {
        WARN("Cannot use GetDC on a %s surface.\n", debug_d3dformat(surface->resource.format->id));
        return WINED3DERR_INVALIDCALL;
    }

#endif /* STAGING_CSMT */
    /* Give more detailed info for ddraw. */
    if (surface->flags & SFLAG_DCINUSE)
        return WINEDDERR_DCALREADYCREATED;

    /* Can't GetDC if the surface is locked. */
    if (surface->resource.map_count)
        return WINED3DERR_INVALIDCALL;

#if defined(STAGING_CSMT)
    surface->flags |= SFLAG_DCINUSE;
    surface->resource.map_count++;
    wined3d_cs_emit_getdc(device->cs, surface);
    *dc = surface->hDC;
    TRACE("Returning dc %p.\n", *dc);

    return *dc ? WINED3D_OK : WINED3DERR_INVALIDCALL;
}

HRESULT CDECL wined3d_texture_release_dc(struct wined3d_texture *texture, unsigned int sub_resource_idx, HDC dc)
{
    struct wined3d_device *device = texture->resource.device;
#else  /* STAGING_CSMT */
    if (device->d3d_initialized)
        context = context_acquire(device, NULL);

    /* Create a DIB section if there isn't a dc yet. */
    if (!surface->hDC)
    {
        if (FAILED(hr = surface_create_dib_section(surface)))
        {
            if (context)
                context_release(context);
             return WINED3DERR_INVALIDCALL;
        }
        if (!(surface->resource.map_binding == WINED3D_LOCATION_USER_MEMORY
                || surface->container->flags & WINED3D_TEXTURE_PIN_SYSMEM
                || surface->pbo))
            surface->resource.map_binding = WINED3D_LOCATION_DIB;
    }

    surface_load_location(surface, context, WINED3D_LOCATION_DIB);
    surface_invalidate_location(surface, ~WINED3D_LOCATION_DIB);

    if (context)
        context_release(context);

    surface->flags |= SFLAG_DCINUSE;
    surface->resource.map_count++;

    *dc = surface->hDC;
    TRACE("Returning dc %p.\n", *dc);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_texture_release_dc(struct wined3d_texture *texture, unsigned int sub_resource_idx, HDC dc)
{
    struct wined3d_device *device = texture->resource.device;
    struct wined3d_context *context = NULL;
#endif /* STAGING_CSMT */
    struct wined3d_resource *sub_resource;
    struct wined3d_surface *surface;

    TRACE("texture %p, sub_resource_idx %u, dc %p.\n", texture, sub_resource_idx, dc);

    if (!(sub_resource = wined3d_texture_get_sub_resource(texture, sub_resource_idx)))
        return WINED3DERR_INVALIDCALL;

    if (sub_resource->type != WINED3D_RTYPE_SURFACE)
    {
        WARN("Not supported on %s resources.\n", debug_d3dresourcetype(texture->resource.type));
        return WINED3DERR_INVALIDCALL;
    }

    surface = surface_from_resource(sub_resource);

    if (!(surface->flags & SFLAG_DCINUSE))
        return WINEDDERR_NODC;

    if (surface->hDC != dc)
    {
        WARN("Application tries to release invalid DC %p, surface DC is %p.\n",
                dc, surface->hDC);
        return WINEDDERR_NODC;
    }

    surface->resource.map_count--;
    surface->flags &= ~SFLAG_DCINUSE;

#if defined(STAGING_CSMT)
    wined3d_cs_emit_releasedc(device->cs, surface);
#else  /* STAGING_CSMT */
    if (surface->resource.map_binding == WINED3D_LOCATION_USER_MEMORY
            || (surface->container->flags & WINED3D_TEXTURE_PIN_SYSMEM
            && surface->resource.map_binding != WINED3D_LOCATION_DIB))
    {
        /* The game Salammbo modifies the surface contents without mapping the surface between
         * a GetDC/ReleaseDC operation and flipping the surface. If the DIB remains the active
         * copy and is copied to the screen, this update, which draws the mouse pointer, is lost.
         * Do not only copy the DIB to the map location, but also make sure the map location is
         * copied back to the DIB in the next getdc call.
         *
         * The same consideration applies to user memory surfaces. */

        if (device->d3d_initialized)
            context = context_acquire(device, NULL);

        surface_load_location(surface, context, surface->resource.map_binding);
        surface_invalidate_location(surface, WINED3D_LOCATION_DIB);
        if (context)
            context_release(context);
    }
#endif /* STAGING_CSMT */

    return WINED3D_OK;
}
