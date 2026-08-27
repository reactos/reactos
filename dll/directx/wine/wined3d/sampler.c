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

#include "wined3d_private.h"
#include "wined3d_gl.h"
#include "wined3d_vk.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

ULONG CDECL wined3d_sampler_incref(struct wined3d_sampler *sampler)
{
    unsigned int refcount = InterlockedIncrement(&sampler->refcount);

    TRACE("%p increasing refcount to %u.\n", sampler, refcount);

    return refcount;
}

ULONG CDECL wined3d_sampler_decref(struct wined3d_sampler *sampler)
{
    unsigned int refcount = wined3d_atomic_decrement_mutex_lock(&sampler->refcount);

    TRACE("%p decreasing refcount to %u.\n", sampler, refcount);

    if (!refcount)
    {
        sampler->parent_ops->wined3d_object_destroyed(sampler->parent);
        sampler->device->adapter->adapter_ops->adapter_destroy_sampler(sampler);
        wined3d_mutex_unlock();
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

    TRACE("    Address modes: U %#x, V %#x, W %#x.\n", desc->address_u, desc->address_v, desc->address_w);
    TRACE("    Border colour: {%.8e, %.8e, %.8e, %.8e}.\n",
            desc->border_color[0], desc->border_color[1], desc->border_color[2], desc->border_color[3]);
    TRACE("    Filters: mag %#x, min %#x, mip %#x.\n", desc->mag_filter, desc->min_filter, desc->mip_filter);
    TRACE("    LOD bias: %.8e.\n", desc->lod_bias);
    TRACE("    Minimum LOD: %.8e.\n", desc->min_lod);
    TRACE("    Maximum LOD: %.8e.\n", desc->max_lod);
    TRACE("    Base mip level: %u.\n", desc->mip_base_level);
    TRACE("    Maximum anisotropy: %u.\n", desc->max_anisotropy);
    TRACE("    Comparison: %d.\n", desc->compare);
    TRACE("    Comparison func: %#x.\n", desc->comparison_func);
    TRACE("    SRGB decode: %d.\n", desc->srgb_decode);

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

    TRACE("sampler_gl %p.\n", sampler_gl);

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

    if (wined3d_adapter_gl(device->adapter)->gl_info.supported[ARB_SAMPLER_OBJECTS])
        wined3d_cs_init_object(device->cs, wined3d_sampler_gl_cs_init, sampler_gl);
}

static VkFilter vk_filter_from_wined3d(enum wined3d_texture_filter_type f)
{
    switch (f)
    {
        default:
            ERR("Invalid filter type %#x.\n", f);
        case WINED3D_TEXF_POINT:
            return VK_FILTER_NEAREST;
        case WINED3D_TEXF_LINEAR:
            return VK_FILTER_LINEAR;
    }
}

static VkSamplerMipmapMode vk_mipmap_mode_from_wined3d(enum wined3d_texture_filter_type f)
{
    switch (f)
    {
        default:
            ERR("Invalid filter type %#x.\n", f);
        case WINED3D_TEXF_NONE:
        case WINED3D_TEXF_POINT:
            return VK_SAMPLER_MIPMAP_MODE_NEAREST;
        case WINED3D_TEXF_LINEAR:
            return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
}

static VkSamplerAddressMode vk_address_mode_from_wined3d(enum wined3d_texture_address a)
{
    switch (a)
    {
        default:
            ERR("Invalid address mode %#x.\n", a);
        case WINED3D_TADDRESS_WRAP:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case WINED3D_TADDRESS_MIRROR:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case WINED3D_TADDRESS_CLAMP:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case WINED3D_TADDRESS_BORDER:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        case WINED3D_TADDRESS_MIRROR_ONCE:
            return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    }
}

static VkBorderColor vk_border_colour_from_wined3d(const struct wined3d_color *colour)
{
    unsigned int i;

    static const struct
    {
        struct wined3d_color wined3d_colour;
        VkBorderColor vk_colour;
    }
    colours[] =
    {
        {{0.0f, 0.0f, 0.0f, 0.0f}, VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK},
        {{0.0f, 0.0f, 0.0f, 1.0f}, VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK},
        {{1.0f, 1.0f, 1.0f, 1.0f}, VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE},
    };

    for (i = 0; i < ARRAY_SIZE(colours); ++i)
    {
        if (!memcmp(&colours[i], colour, sizeof(*colour)))
            return colours[i].vk_colour;
    }

    FIXME("Unhandled border colour {%.8e, %.8e, %.8e, %.8e}.\n", colour->r, colour->g, colour->b, colour->a);

    return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
}

static void wined3d_sampler_vk_cs_init(void *object)
{
    struct wined3d_sampler_vk *sampler_vk = object;
    const struct wined3d_sampler_desc *desc;
    const struct wined3d_d3d_info *d3d_info;
    struct VkSamplerCreateInfo sampler_desc;
    const struct wined3d_vk_info *vk_info;
    struct wined3d_context_vk *context_vk;
    struct wined3d_device_vk *device_vk;
    VkSampler vk_sampler;
    VkResult vr;

    TRACE("sampler_vk %p.\n", sampler_vk);

    context_vk = wined3d_context_vk(context_acquire(sampler_vk->s.device, NULL, 0));
    device_vk = wined3d_device_vk(context_vk->c.device);
    d3d_info = context_vk->c.d3d_info;
    vk_info = context_vk->vk_info;

    desc = &sampler_vk->s.desc;
    sampler_desc.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_desc.pNext = NULL;
    sampler_desc.flags = 0;
    sampler_desc.magFilter = vk_filter_from_wined3d(desc->mag_filter);
    sampler_desc.minFilter = vk_filter_from_wined3d(desc->min_filter);
    sampler_desc.mipmapMode = vk_mipmap_mode_from_wined3d(desc->mip_filter);
    sampler_desc.addressModeU = vk_address_mode_from_wined3d(desc->address_u);
    sampler_desc.addressModeV = vk_address_mode_from_wined3d(desc->address_v);
    sampler_desc.addressModeW = vk_address_mode_from_wined3d(desc->address_w);
    sampler_desc.mipLodBias = desc->lod_bias;
    sampler_desc.anisotropyEnable = desc->max_anisotropy != 1;
    sampler_desc.maxAnisotropy = desc->max_anisotropy;
    sampler_desc.compareEnable = !!desc->compare;
    sampler_desc.compareOp = vk_compare_op_from_wined3d(desc->comparison_func);
    sampler_desc.minLod = desc->min_lod;
    sampler_desc.maxLod = desc->max_lod;
    sampler_desc.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    sampler_desc.unnormalizedCoordinates = VK_FALSE;

    if (desc->address_u == WINED3D_TADDRESS_BORDER || desc->address_v == WINED3D_TADDRESS_BORDER
            || desc->address_w == WINED3D_TADDRESS_BORDER)
        sampler_desc.borderColor = vk_border_colour_from_wined3d((const struct wined3d_color *)desc->border_color);
    if (desc->mip_base_level)
        FIXME("Unhandled mip_base_level %u.\n", desc->mip_base_level);
    if ((d3d_info->wined3d_creation_flags & WINED3D_SRGB_READ_WRITE_CONTROL) && !desc->srgb_decode)
        FIXME("Unhandled srgb_decode %#x.\n", desc->srgb_decode);

    vr = VK_CALL(vkCreateSampler(device_vk->vk_device, &sampler_desc, NULL, &vk_sampler));
    context_release(&context_vk->c);
    if (vr < 0)
    {
        ERR("Failed to create Vulkan sampler, vr %s.\n", wined3d_debug_vkresult(vr));
        return;
    }

    TRACE("Created sampler 0x%s.\n", wine_dbgstr_longlong(vk_sampler));

    sampler_vk->vk_image_info.sampler = vk_sampler;
    sampler_vk->vk_image_info.imageView = VK_NULL_HANDLE;
    sampler_vk->vk_image_info.imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

void wined3d_sampler_vk_init(struct wined3d_sampler_vk *sampler_vk, struct wined3d_device *device,
        const struct wined3d_sampler_desc *desc, void *parent, const struct wined3d_parent_ops *parent_ops)
{
    TRACE("sampler_vk %p, device %p, desc %p, parent %p, parent_ops %p.\n",
            sampler_vk, device, desc, parent, parent_ops);

    wined3d_sampler_init(&sampler_vk->s, device, desc, parent, parent_ops);
    wined3d_cs_init_object(device->cs, wined3d_sampler_vk_cs_init, sampler_vk);
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

    gl_tex = wined3d_texture_gl_get_gl_texture(texture_gl, texture_gl->t.flags & WINED3D_TEXTURE_IS_SRGB);
    if (desc->mip_base_level != gl_tex->sampler_desc.mip_base_level)
    {
        gl_info->gl_ops.gl.p_glTexParameteri(texture_gl->target, GL_TEXTURE_BASE_LEVEL, desc->mip_base_level);
        gl_tex->sampler_desc.mip_base_level = desc->mip_base_level;
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
