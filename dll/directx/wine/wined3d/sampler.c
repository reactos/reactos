/*
 * Copyright 2012, 2015 Henri Verbeet for CodeWeavers
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
 *
 */

#include "config.h"
#include "wine/port.h"

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

ULONG CDECL wined3d_sampler_incref(struct wined3d_sampler *sampler)
{
    ULONG refcount = InterlockedIncrement(&sampler->refcount);

    TRACE("%p increasing refcount to %u.\n", sampler, refcount);

    return refcount;
}

ULONG CDECL wined3d_sampler_decref(struct wined3d_sampler *sampler)
{
    ULONG refcount = InterlockedDecrement(&sampler->refcount);

    TRACE("%p decreasing refcount to %u.\n", sampler, refcount);

    if (!refcount)
    {
        sampler->parent_ops->wined3d_object_destroyed(sampler->parent);
        sampler->device->adapter->adapter_ops->adapter_destroy_sampler(sampler);
    }

    return refcount;
}

void * CDECL wined3d_sampler_get_parent(const struct wined3d_sampler *sampler)
{
    TRACE("sampler %p.\n", sampler);

    return sampler->parent;
}

static void wined3d_sampler_init(struct wined3d_sampler *sampler, struct wined3d_device *device,
        const struct wined3d_sampler_desc *desc, void *parent, const struct wined3d_parent_ops *parent_ops)
{
    TRACE("sampler %p, device %p, desc %p, parent %p, parent_ops %p.\n",
            sampler, device, desc, parent, parent_ops);

    sampler->refcount = 1;
    sampler->device = device;
    sampler->parent = parent;
    sampler->parent_ops = parent_ops;
    sampler->desc = *desc;
}

static void wined3d_sampler_gl_cs_init(void *object)
{
    struct wined3d_sampler_gl *sampler_gl = object;
    const struct wined3d_sampler_desc *desc;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    GLuint name;

    context = context_acquire(sampler_gl->s.device, NULL, 0);
    gl_info = wined3d_context_gl(context)->gl_info;

    desc = &sampler_gl->s.desc;
    GL_EXTCALL(glGenSamplers(1, &name));
    GL_EXTCALL(glSamplerParameteri(name, GL_TEXTURE_WRAP_S,
            gl_info->wrap_lookup[desc->address_u - WINED3D_TADDRESS_WRAP]));
    GL_EXTCALL(glSamplerParameteri(name, GL_TEXTURE_WRAP_T,
            gl_info->wrap_lookup[desc->address_v - WINED3D_TADDRESS_WRAP]));
    GL_EXTCALL(glSamplerParameteri(name, GL_TEXTURE_WRAP_R,
            gl_info->wrap_lookup[desc->address_w - WINED3D_TADDRESS_WRAP]));
    GL_EXTCALL(glSamplerParameterfv(name, GL_TEXTURE_BORDER_COLOR, &desc->border_color[0]));
    GL_EXTCALL(glSamplerParameteri(name, GL_TEXTURE_MAG_FILTER,
            wined3d_gl_mag_filter(desc->mag_filter)));
    GL_EXTCALL(glSamplerParameteri(name, GL_TEXTURE_MIN_FILTER,
            wined3d_gl_min_mip_filter(desc->min_filter, desc->mip_filter)));
    GL_EXTCALL(glSamplerParameterf(name, GL_TEXTURE_LOD_BIAS, desc->lod_bias));
    GL_EXTCALL(glSamplerParameterf(name, GL_TEXTURE_MIN_LOD, desc->min_lod));
    GL_EXTCALL(glSamplerParameterf(name, GL_TEXTURE_MAX_LOD, desc->max_lod));
    if (gl_info->supported[ARB_TEXTURE_FILTER_ANISOTROPIC])
        GL_EXTCALL(glSamplerParameteri(name, GL_TEXTURE_MAX_ANISOTROPY, desc->max_anisotropy));
    if (desc->compare)
        GL_EXTCALL(glSamplerParameteri(name, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE));
    GL_EXTCALL(glSamplerParameteri(name, GL_TEXTURE_COMPARE_FUNC,
            wined3d_gl_compare_func(desc->comparison_func)));
    if ((context->d3d_info->wined3d_creation_flags & WINED3D_SRGB_READ_WRITE_CONTROL)
            && gl_info->supported[EXT_TEXTURE_SRGB_DECODE] && !desc->srgb_decode)
        GL_EXTCALL(glSamplerParameteri(name, GL_TEXTURE_SRGB_DECODE_EXT, GL_SKIP_DECODE_EXT));
    checkGLcall("sampler creation");

    TRACE("Created sampler %u.\n", name);
    sampler_gl->name = name;

    context_release(context);
}

void wined3d_sampler_gl_init(struct wined3d_sampler_gl *sampler_gl, struct wined3d_device *device,
        const struct wined3d_sampler_desc *desc, void *parent, const struct wined3d_parent_ops *parent_ops)
{
    TRACE("sampler_gl %p, device %p, desc %p, parent %p, parent_ops %p.\n",
            sampler_gl, device, desc, parent, parent_ops);

    wined3d_sampler_init(&sampler_gl->s, device, desc, parent, parent_ops);

    if (device->adapter->gl_info.supported[ARB_SAMPLER_OBJECTS])
        wined3d_cs_init_object(device->cs, wined3d_sampler_gl_cs_init, sampler_gl);
}

void wined3d_sampler_vk_init(struct wined3d_sampler *sampler_vk, struct wined3d_device *device,
        const struct wined3d_sampler_desc *desc, void *parent, const struct wined3d_parent_ops *parent_ops)
{
    TRACE("sampler_vk %p, device %p, desc %p, parent %p, parent_ops %p.\n",
            sampler_vk, device, desc, parent, parent_ops);

    wined3d_sampler_init(sampler_vk, device, desc, parent, parent_ops);
}

HRESULT CDECL wined3d_sampler_create(struct wined3d_device *device, const struct wined3d_sampler_desc *desc,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_sampler **sampler)
{
    TRACE("device %p, desc %p, parent %p, parent_ops %p, sampler %p.\n",
            device, desc, parent, parent_ops, sampler);

    if (desc->address_u < WINED3D_TADDRESS_WRAP || desc->address_u > WINED3D_TADDRESS_MIRROR_ONCE
            || desc->address_v < WINED3D_TADDRESS_WRAP || desc->address_v > WINED3D_TADDRESS_MIRROR_ONCE
            || desc->address_w < WINED3D_TADDRESS_WRAP || desc->address_w > WINED3D_TADDRESS_MIRROR_ONCE)
        return WINED3DERR_INVALIDCALL;

    if (desc->mag_filter < WINED3D_TEXF_POINT || desc->mag_filter > WINED3D_TEXF_LINEAR
            || desc->min_filter < WINED3D_TEXF_POINT || desc->min_filter > WINED3D_TEXF_LINEAR
            || desc->mip_filter > WINED3D_TEXF_LINEAR)
        return WINED3DERR_INVALIDCALL;

    return device->adapter->adapter_ops->adapter_create_sampler(device, desc, parent, parent_ops, sampler);
}

static void texture_gl_apply_base_level(struct wined3d_texture_gl *texture_gl,
        const struct wined3d_sampler_desc *desc, const struct wined3d_gl_info *gl_info)
{
    struct gl_texture *gl_tex;
    unsigned int base_level;

    if (texture_gl->t.flags & WINED3D_TEXTURE_COND_NP2)
        base_level = 0;
    else if (desc->mip_filter == WINED3D_TEXF_NONE)
        base_level = texture_gl->t.lod;
    else
        base_level = min(max(desc->mip_base_level, texture_gl->t.lod), texture_gl->t.level_count - 1);

    gl_tex = wined3d_texture_gl_get_gl_texture(texture_gl, texture_gl->t.flags & WINED3D_TEXTURE_IS_SRGB);
    if (base_level != gl_tex->base_level)
    {
        /* Note that WINED3D_SAMP_MAX_MIP_LEVEL specifies the largest mipmap
         * (default 0), while GL_TEXTURE_MAX_LEVEL specifies the smallest
         * mipmap used (default 1000). So WINED3D_SAMP_MAX_MIP_LEVEL
         * corresponds to GL_TEXTURE_BASE_LEVEL. */
        gl_info->gl_ops.gl.p_glTexParameteri(texture_gl->target, GL_TEXTURE_BASE_LEVEL, base_level);
        gl_tex->base_level = base_level;
    }
}

/* This function relies on the correct texture being bound and loaded. */
void wined3d_sampler_gl_bind(struct wined3d_sampler_gl *sampler_gl, unsigned int unit,
        struct wined3d_texture_gl *texture_gl, const struct wined3d_context_gl *context_gl)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    if (gl_info->supported[ARB_SAMPLER_OBJECTS])
    {
        GL_EXTCALL(glBindSampler(unit, sampler_gl->name));
        checkGLcall("bind sampler");
    }
    else if (texture_gl)
    {
        wined3d_texture_gl_apply_sampler_desc(texture_gl, &sampler_gl->s.desc, context_gl);
    }
    else
    {
        ERR("Could not apply sampler state.\n");
    }

    if (texture_gl)
        texture_gl_apply_base_level(texture_gl, &sampler_gl->s.desc, gl_info);
}
