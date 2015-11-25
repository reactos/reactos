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

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

ULONG CDECL wined3d_sampler_incref(struct wined3d_sampler *sampler)
{
    ULONG refcount = InterlockedIncrement(&sampler->refcount);

    TRACE("%p increasing refcount to %u.\n", sampler, refcount);

    return refcount;
}

#if defined(STAGING_CSMT)
void wined3d_sampler_destroy(struct wined3d_sampler *sampler)
{
    struct wined3d_context *context = context_acquire(sampler->device, NULL);
    const struct wined3d_gl_info *gl_info = context->gl_info;

    GL_EXTCALL(glDeleteSamplers(1, &sampler->name));
    context_release(context);

    HeapFree(GetProcessHeap(), 0, sampler);
}

ULONG CDECL wined3d_sampler_decref(struct wined3d_sampler *sampler)
{
    ULONG refcount = InterlockedDecrement(&sampler->refcount);

    TRACE("%p decreasing refcount to %u.\n", sampler, refcount);

    if (!refcount)
    {
        struct wined3d_device *device = sampler->device;
        wined3d_cs_emit_sampler_destroy(device->cs, sampler);
#else  /* STAGING_CSMT */
ULONG CDECL wined3d_sampler_decref(struct wined3d_sampler *sampler)
{
    ULONG refcount = InterlockedDecrement(&sampler->refcount);
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;

    TRACE("%p decreasing refcount to %u.\n", sampler, refcount);

    if (!refcount)
    {
        context = context_acquire(sampler->device, NULL);
        gl_info = context->gl_info;
        GL_EXTCALL(glDeleteSamplers(1, &sampler->name));
        context_release(context);

        HeapFree(GetProcessHeap(), 0, sampler);
#endif /* STAGING_CSMT */
    }

    return refcount;
}

void * CDECL wined3d_sampler_get_parent(const struct wined3d_sampler *sampler)
{
    TRACE("sampler %p.\n", sampler);

    return sampler->parent;
}

static void wined3d_sampler_init(struct wined3d_sampler *sampler, struct wined3d_device *device,
        const struct wined3d_sampler_desc *desc, void *parent)
{
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;

    sampler->refcount = 1;
    sampler->device = device;
    sampler->parent = parent;
    sampler->desc = *desc;

    context = context_acquire(device, NULL);
    gl_info = context->gl_info;

    GL_EXTCALL(glGenSamplers(1, &sampler->name));
    GL_EXTCALL(glSamplerParameteri(sampler->name, GL_TEXTURE_WRAP_S,
            gl_info->wrap_lookup[desc->address_u - WINED3D_TADDRESS_WRAP]));
    GL_EXTCALL(glSamplerParameteri(sampler->name, GL_TEXTURE_WRAP_T,
            gl_info->wrap_lookup[desc->address_v - WINED3D_TADDRESS_WRAP]));
    GL_EXTCALL(glSamplerParameteri(sampler->name, GL_TEXTURE_WRAP_R,
            gl_info->wrap_lookup[desc->address_w - WINED3D_TADDRESS_WRAP]));
    GL_EXTCALL(glSamplerParameterfv(sampler->name, GL_TEXTURE_BORDER_COLOR, &desc->border_color[0]));
    GL_EXTCALL(glSamplerParameteri(sampler->name, GL_TEXTURE_MAG_FILTER,
            wined3d_gl_mag_filter(desc->mag_filter)));
    GL_EXTCALL(glSamplerParameteri(sampler->name, GL_TEXTURE_MIN_FILTER,
            wined3d_gl_min_mip_filter(desc->min_filter, desc->mip_filter)));
    GL_EXTCALL(glSamplerParameterf(sampler->name, GL_TEXTURE_LOD_BIAS, desc->lod_bias));
    GL_EXTCALL(glSamplerParameterf(sampler->name, GL_TEXTURE_MIN_LOD, desc->min_lod));
    GL_EXTCALL(glSamplerParameterf(sampler->name, GL_TEXTURE_MAX_LOD, desc->max_lod));
    if (gl_info->supported[EXT_TEXTURE_FILTER_ANISOTROPIC])
        GL_EXTCALL(glSamplerParameteri(sampler->name, GL_TEXTURE_MAX_ANISOTROPY_EXT, desc->max_anisotropy));
    if (desc->compare)
        GL_EXTCALL(glSamplerParameteri(sampler->name, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE));
    GL_EXTCALL(glSamplerParameteri(sampler->name, GL_TEXTURE_COMPARE_FUNC,
            wined3d_gl_compare_func(desc->comparison_func)));
    if (gl_info->supported[EXT_TEXTURE_SRGB_DECODE] && !desc->srgb_decode)
        GL_EXTCALL(glSamplerParameteri(sampler->name, GL_TEXTURE_SRGB_DECODE_EXT, GL_SKIP_DECODE_EXT));
    checkGLcall("sampler creation");

    TRACE("Created sampler %u.\n", sampler->name);

    context_release(context);
}

HRESULT CDECL wined3d_sampler_create(struct wined3d_device *device, const struct wined3d_sampler_desc *desc,
        void *parent, struct wined3d_sampler **sampler)
{
    struct wined3d_sampler *object;

    TRACE("device %p, desc %p, parent %p, sampler %p.\n", device, desc, parent, sampler);

    if (!device->adapter->gl_info.supported[ARB_SAMPLER_OBJECTS])
        return WINED3DERR_INVALIDCALL;

    if (desc->address_u < WINED3D_TADDRESS_WRAP || desc->address_u > WINED3D_TADDRESS_MIRROR_ONCE
            || desc->address_v < WINED3D_TADDRESS_WRAP || desc->address_v > WINED3D_TADDRESS_MIRROR_ONCE
            || desc->address_w < WINED3D_TADDRESS_WRAP || desc->address_w > WINED3D_TADDRESS_MIRROR_ONCE)
        return WINED3DERR_INVALIDCALL;

    if (desc->mag_filter < WINED3D_TEXF_POINT || desc->mag_filter > WINED3D_TEXF_LINEAR
            || desc->min_filter < WINED3D_TEXF_POINT || desc->min_filter > WINED3D_TEXF_LINEAR
            || desc->mip_filter > WINED3D_TEXF_LINEAR)
        return WINED3DERR_INVALIDCALL;

    if (!(object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object))))
        return E_OUTOFMEMORY;

    wined3d_sampler_init(object, device, desc, parent);

    TRACE("Created sampler %p.\n", object);
    *sampler = object;

    return WINED3D_OK;
}
