/*
 * Copyright 2002 Lionel Ulmer
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006-2008 Stefan Dösinger for CodeWeavers
 * Copyright 2006-2008 Henri Verbeet
 * Copyright 2007 Andrew Riedi
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

#include "config.h"
#include "wine/port.h"

#include <stdio.h>
#ifdef HAVE_FLOAT_H
# include <float.h>
#endif

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

/* Define the default light parameters as specified by MSDN. */
const struct wined3d_light WINED3D_default_light =
{
    WINED3D_LIGHT_DIRECTIONAL,  /* Type */
    { 1.0f, 1.0f, 1.0f, 0.0f }, /* Diffuse r,g,b,a */
    { 0.0f, 0.0f, 0.0f, 0.0f }, /* Specular r,g,b,a */
    { 0.0f, 0.0f, 0.0f, 0.0f }, /* Ambient r,g,b,a, */
    { 0.0f, 0.0f, 0.0f },       /* Position x,y,z */
    { 0.0f, 0.0f, 1.0f },       /* Direction x,y,z */
    0.0f,                       /* Range */
    0.0f,                       /* Falloff */
    0.0f, 0.0f, 0.0f,           /* Attenuation 0,1,2 */
    0.0f,                       /* Theta */
    0.0f                        /* Phi */
};

/* Note that except for WINED3DPT_POINTLIST and WINED3DPT_LINELIST these
 * actually have the same values in GL and D3D. */
GLenum gl_primitive_type_from_d3d(enum wined3d_primitive_type primitive_type)
{
    switch (primitive_type)
    {
        case WINED3D_PT_POINTLIST:
            return GL_POINTS;

        case WINED3D_PT_LINELIST:
            return GL_LINES;

        case WINED3D_PT_LINESTRIP:
            return GL_LINE_STRIP;

        case WINED3D_PT_TRIANGLELIST:
            return GL_TRIANGLES;

        case WINED3D_PT_TRIANGLESTRIP:
            return GL_TRIANGLE_STRIP;

        case WINED3D_PT_TRIANGLEFAN:
            return GL_TRIANGLE_FAN;

        case WINED3D_PT_LINELIST_ADJ:
            return GL_LINES_ADJACENCY_ARB;

        case WINED3D_PT_LINESTRIP_ADJ:
            return GL_LINE_STRIP_ADJACENCY_ARB;

        case WINED3D_PT_TRIANGLELIST_ADJ:
            return GL_TRIANGLES_ADJACENCY_ARB;

        case WINED3D_PT_TRIANGLESTRIP_ADJ:
            return GL_TRIANGLE_STRIP_ADJACENCY_ARB;

        case WINED3D_PT_PATCH:
            return GL_PATCHES;

        default:
            FIXME("Unhandled primitive type %s.\n", debug_d3dprimitivetype(primitive_type));
        case WINED3D_PT_UNDEFINED:
            return ~0u;
    }
}

static enum wined3d_primitive_type d3d_primitive_type_from_gl(GLenum primitive_type)
{
    switch (primitive_type)
    {
        case GL_POINTS:
            return WINED3D_PT_POINTLIST;

        case GL_LINES:
            return WINED3D_PT_LINELIST;

        case GL_LINE_STRIP:
            return WINED3D_PT_LINESTRIP;

        case GL_TRIANGLES:
            return WINED3D_PT_TRIANGLELIST;

        case GL_TRIANGLE_STRIP:
            return WINED3D_PT_TRIANGLESTRIP;

        case GL_TRIANGLE_FAN:
            return WINED3D_PT_TRIANGLEFAN;

        case GL_LINES_ADJACENCY_ARB:
            return WINED3D_PT_LINELIST_ADJ;

        case GL_LINE_STRIP_ADJACENCY_ARB:
            return WINED3D_PT_LINESTRIP_ADJ;

        case GL_TRIANGLES_ADJACENCY_ARB:
            return WINED3D_PT_TRIANGLELIST_ADJ;

        case GL_TRIANGLE_STRIP_ADJACENCY_ARB:
            return WINED3D_PT_TRIANGLESTRIP_ADJ;

        case GL_PATCHES:
            return WINED3D_PT_PATCH;

        default:
            FIXME("Unhandled primitive type %s.\n", debug_d3dprimitivetype(primitive_type));
        case ~0u:
            return WINED3D_PT_UNDEFINED;
    }
}

BOOL device_context_add(struct wined3d_device *device, struct wined3d_context *context)
{
    struct wined3d_context **new_array;

    TRACE("Adding context %p.\n", context);

    if (!(new_array = heap_realloc(device->contexts, sizeof(*new_array) * (device->context_count + 1))))
    {
        ERR("Failed to grow the context array.\n");
        return FALSE;
    }

    new_array[device->context_count++] = context;
    device->contexts = new_array;
    return TRUE;
}

void device_context_remove(struct wined3d_device *device, struct wined3d_context *context)
{
    struct wined3d_context **new_array;
    BOOL found = FALSE;
    UINT i;

    TRACE("Removing context %p.\n", context);

    for (i = 0; i < device->context_count; ++i)
    {
        if (device->contexts[i] == context)
        {
            found = TRUE;
            break;
        }
    }

    if (!found)
    {
        ERR("Context %p doesn't exist in context array.\n", context);
        return;
    }

    if (!--device->context_count)
    {
        heap_free(device->contexts);
        device->contexts = NULL;
        return;
    }

    memmove(&device->contexts[i], &device->contexts[i + 1], (device->context_count - i) * sizeof(*device->contexts));
    if (!(new_array = heap_realloc(device->contexts, device->context_count * sizeof(*device->contexts))))
    {
        ERR("Failed to shrink context array. Oh well.\n");
        return;
    }

    device->contexts = new_array;
}

static BOOL is_full_clear(const struct wined3d_texture *texture, unsigned int sub_resource_idx,
        const RECT *draw_rect, const RECT *clear_rect)
{
    unsigned int width, height, level;

    level = sub_resource_idx % texture->level_count;
    width = wined3d_texture_get_level_width(texture, level);
    height = wined3d_texture_get_level_height(texture, level);

    /* partial draw rect */
    if (draw_rect->left || draw_rect->top || draw_rect->right < width || draw_rect->bottom < height)
        return FALSE;

    /* partial clear rect */
    if (clear_rect && (clear_rect->left > 0 || clear_rect->top > 0
            || clear_rect->right < width || clear_rect->bottom < height))
        return FALSE;

    return TRUE;
}

void device_clear_render_targets(struct wined3d_device *device, UINT rt_count, const struct wined3d_fb_state *fb,
        UINT rect_count, const RECT *clear_rect, const RECT *draw_rect, DWORD flags, const struct wined3d_color *color,
        float depth, DWORD stencil)
{
    struct wined3d_rendertarget_view *rtv = rt_count ? fb->render_targets[0] : NULL;
    struct wined3d_surface *target = rtv ? wined3d_rendertarget_view_get_surface(rtv) : NULL;
    struct wined3d_rendertarget_view *dsv = fb->depth_stencil;
    struct wined3d_surface *depth_stencil = dsv ? wined3d_rendertarget_view_get_surface(dsv) : NULL;
    const struct wined3d_state *state = &device->cs->state;
    const struct wined3d_gl_info *gl_info;
    UINT drawable_width, drawable_height;
    struct wined3d_color corrected_color;
    struct wined3d_context *context;
    GLbitfield clear_mask = 0;
    BOOL render_offscreen;
    unsigned int i;

    if (target)
        context = context_acquire(device, target->container, rtv->sub_resource_idx);
    else
        context = context_acquire(device, NULL, 0);
    if (!context->valid)
    {
        context_release(context);
        WARN("Invalid context, skipping clear.\n");
        return;
    }
    gl_info = context->gl_info;

    /* When we're clearing parts of the drawable, make sure that the target surface is well up to date in the
     * drawable. After the clear we'll mark the drawable up to date, so we have to make sure that this is true
     * for the cleared parts, and the untouched parts.
     *
     * If we're clearing the whole target there is no need to copy it into the drawable, it will be overwritten
     * anyway. If we're not clearing the color buffer we don't have to copy either since we're not going to set
     * the drawable up to date. We have to check all settings that limit the clear area though. Do not bother
     * checking all this if the dest surface is in the drawable anyway. */
    for (i = 0; i < rt_count; ++i)
    {
        struct wined3d_rendertarget_view *rtv = fb->render_targets[i];

        if (rtv && rtv->format->id != WINED3DFMT_NULL)
        {
            struct wined3d_texture *rt = wined3d_texture_from_resource(rtv->resource);

            if (flags & WINED3DCLEAR_TARGET && !is_full_clear(rt, rtv->sub_resource_idx,
                    draw_rect, rect_count ? clear_rect : NULL))
                wined3d_texture_load_location(rt, rtv->sub_resource_idx, context, rtv->resource->draw_binding);
            else
                wined3d_texture_prepare_location(rt, rtv->sub_resource_idx, context, rtv->resource->draw_binding);
        }
    }

    if (target)
    {
        render_offscreen = context->render_offscreen;
        wined3d_rendertarget_view_get_drawable_size(rtv, context, &drawable_width, &drawable_height);
    }
    else
    {
        unsigned int ds_level = dsv->sub_resource_idx % depth_stencil->container->level_count;

        render_offscreen = TRUE;
        drawable_width = wined3d_texture_get_level_pow2_width(depth_stencil->container, ds_level);
        drawable_height = wined3d_texture_get_level_pow2_height(depth_stencil->container, ds_level);
    }

    if (depth_stencil)
    {
        DWORD ds_location = render_offscreen ? dsv->resource->draw_binding : WINED3D_LOCATION_DRAWABLE;
        struct wined3d_texture *ds = wined3d_texture_from_resource(dsv->resource);

        if (flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL)
                && !is_full_clear(ds, dsv->sub_resource_idx, draw_rect, rect_count ? clear_rect : NULL))
            wined3d_texture_load_location(ds, dsv->sub_resource_idx, context, ds_location);
        else
            wined3d_texture_prepare_location(ds, dsv->sub_resource_idx, context, ds_location);

        if (flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL))
        {
            wined3d_texture_validate_location(ds, dsv->sub_resource_idx, ds_location);
            wined3d_texture_invalidate_location(ds, dsv->sub_resource_idx, ~ds_location);
        }
    }

    if (!context_apply_clear_state(context, state, rt_count, fb))
    {
        context_release(context);
        WARN("Failed to apply clear state, skipping clear.\n");
        return;
    }

    /* Only set the values up once, as they are not changing. */
    if (flags & WINED3DCLEAR_STENCIL)
    {
        if (gl_info->supported[EXT_STENCIL_TWO_SIDE])
        {
            gl_info->gl_ops.gl.p_glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
            context_invalidate_state(context, STATE_RENDER(WINED3D_RS_TWOSIDEDSTENCILMODE));
        }
        gl_info->gl_ops.gl.p_glStencilMask(~0U);
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_STENCILWRITEMASK));
        gl_info->gl_ops.gl.p_glClearStencil(stencil);
        checkGLcall("glClearStencil");
        clear_mask = clear_mask | GL_STENCIL_BUFFER_BIT;
    }

    if (flags & WINED3DCLEAR_ZBUFFER)
    {
        gl_info->gl_ops.gl.p_glDepthMask(GL_TRUE);
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_ZWRITEENABLE));
        gl_info->gl_ops.gl.p_glClearDepth(depth);
        checkGLcall("glClearDepth");
        clear_mask = clear_mask | GL_DEPTH_BUFFER_BIT;
    }

    if (flags & WINED3DCLEAR_TARGET)
    {
        for (i = 0; i < rt_count; ++i)
        {
            struct wined3d_rendertarget_view *rtv = fb->render_targets[i];
            struct wined3d_texture *texture;

            if (!rtv)
                continue;

            if (rtv->resource->type == WINED3D_RTYPE_BUFFER)
            {
                FIXME("Not supported on buffer resources.\n");
                continue;
            }

            texture = texture_from_resource(rtv->resource);
            wined3d_texture_validate_location(texture, rtv->sub_resource_idx, rtv->resource->draw_binding);
            wined3d_texture_invalidate_location(texture, rtv->sub_resource_idx, ~rtv->resource->draw_binding);
        }

        if (!gl_info->supported[ARB_FRAMEBUFFER_SRGB] && needs_srgb_write(context, state, fb))
        {
            if (rt_count > 1)
                WARN("Clearing multiple sRGB render targets with no GL_ARB_framebuffer_sRGB "
                        "support, this might cause graphical issues.\n");

            corrected_color.r = color->r < wined3d_srgb_const1[0]
                    ? color->r * wined3d_srgb_const0[3]
                    : pow(color->r, wined3d_srgb_const0[0]) * wined3d_srgb_const0[1]
                    - wined3d_srgb_const0[2];
            corrected_color.r = min(max(corrected_color.r, 0.0f), 1.0f);
            corrected_color.g = color->g < wined3d_srgb_const1[0]
                    ? color->g * wined3d_srgb_const0[3]
                    : pow(color->g, wined3d_srgb_const0[0]) * wined3d_srgb_const0[1]
                    - wined3d_srgb_const0[2];
            corrected_color.g = min(max(corrected_color.g, 0.0f), 1.0f);
            corrected_color.b = color->b < wined3d_srgb_const1[0]
                    ? color->b * wined3d_srgb_const0[3]
                    : pow(color->b, wined3d_srgb_const0[0]) * wined3d_srgb_const0[1]
                    - wined3d_srgb_const0[2];
            corrected_color.b = min(max(corrected_color.b, 0.0f), 1.0f);
            color = &corrected_color;
        }

        gl_info->gl_ops.gl.p_glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        for (i = 0; i < MAX_RENDER_TARGETS; ++i)
            context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITE(i)));
        gl_info->gl_ops.gl.p_glClearColor(color->r, color->g, color->b, color->a);
        checkGLcall("glClearColor");
        clear_mask = clear_mask | GL_COLOR_BUFFER_BIT;
    }

    if (!rect_count)
    {
        if (render_offscreen)
        {
            gl_info->gl_ops.gl.p_glScissor(draw_rect->left, draw_rect->top,
                    draw_rect->right - draw_rect->left, draw_rect->bottom - draw_rect->top);
        }
        else
        {
            gl_info->gl_ops.gl.p_glScissor(draw_rect->left, drawable_height - draw_rect->bottom,
                        draw_rect->right - draw_rect->left, draw_rect->bottom - draw_rect->top);
        }
        checkGLcall("glScissor");
        gl_info->gl_ops.gl.p_glClear(clear_mask);
        checkGLcall("glClear");
    }
    else
    {
        RECT current_rect;

        /* Now process each rect in turn. */
        for (i = 0; i < rect_count; ++i)
        {
            /* Note that GL uses lower left, width/height. */
            IntersectRect(&current_rect, draw_rect, &clear_rect[i]);

            TRACE("clear_rect[%u] %s, current_rect %s.\n", i,
                    wine_dbgstr_rect(&clear_rect[i]),
                    wine_dbgstr_rect(&current_rect));

            /* Tests show that rectangles where x1 > x2 or y1 > y2 are ignored silently.
             * The rectangle is not cleared, no error is returned, but further rectangles are
             * still cleared if they are valid. */
            if (current_rect.left > current_rect.right || current_rect.top > current_rect.bottom)
            {
                TRACE("Rectangle with negative dimensions, ignoring.\n");
                continue;
            }

            if (render_offscreen)
            {
                gl_info->gl_ops.gl.p_glScissor(current_rect.left, current_rect.top,
                        current_rect.right - current_rect.left, current_rect.bottom - current_rect.top);
            }
            else
            {
                gl_info->gl_ops.gl.p_glScissor(current_rect.left, drawable_height - current_rect.bottom,
                          current_rect.right - current_rect.left, current_rect.bottom - current_rect.top);
            }
            checkGLcall("glScissor");

            gl_info->gl_ops.gl.p_glClear(clear_mask);
            checkGLcall("glClear");
        }
    }

    if (wined3d_settings.strict_draw_ordering || (flags & WINED3DCLEAR_TARGET
            && target->container->swapchain && target->container->swapchain->front_buffer == target->container))
        gl_info->gl_ops.gl.p_glFlush(); /* Flush to ensure ordering across contexts. */

    context_release(context);
}

ULONG CDECL wined3d_device_incref(struct wined3d_device *device)
{
    ULONG refcount = InterlockedIncrement(&device->ref);

    TRACE("%p increasing refcount to %u.\n", device, refcount);

    return refcount;
}

static void device_leftover_sampler(struct wine_rb_entry *entry, void *context)
{
    struct wined3d_sampler *sampler = WINE_RB_ENTRY_VALUE(entry, struct wined3d_sampler, entry);

    ERR("Leftover sampler %p.\n", sampler);
}

ULONG CDECL wined3d_device_decref(struct wined3d_device *device)
{
    ULONG refcount = InterlockedDecrement(&device->ref);

    TRACE("%p decreasing refcount to %u.\n", device, refcount);

    if (!refcount)
    {
        UINT i;

        wined3d_cs_destroy(device->cs);

        if (device->recording && wined3d_stateblock_decref(device->recording))
            ERR("Something's still holding the recording stateblock.\n");
        device->recording = NULL;

        state_cleanup(&device->state);

        for (i = 0; i < ARRAY_SIZE(device->multistate_funcs); ++i)
        {
            heap_free(device->multistate_funcs[i]);
            device->multistate_funcs[i] = NULL;
        }

        if (!list_empty(&device->resources))
        {
            struct wined3d_resource *resource;

            ERR("Device released with resources still bound.\n");

            LIST_FOR_EACH_ENTRY(resource, &device->resources, struct wined3d_resource, resource_list_entry)
            {
                ERR("Leftover resource %p with type %s (%#x).\n",
                        resource, debug_d3dresourcetype(resource->type), resource->type);
            }
        }

        if (device->contexts)
            ERR("Context array not freed!\n");
        if (device->hardwareCursor)
            DestroyCursor(device->hardwareCursor);
        device->hardwareCursor = 0;

        wine_rb_destroy(&device->samplers, device_leftover_sampler, NULL);

        wined3d_decref(device->wined3d);
        device->wined3d = NULL;
        heap_free(device);
        TRACE("Freed device %p.\n", device);
    }

    return refcount;
}

UINT CDECL wined3d_device_get_swapchain_count(const struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    return device->swapchain_count;
}

struct wined3d_swapchain * CDECL wined3d_device_get_swapchain(const struct wined3d_device *device, UINT swapchain_idx)
{
    TRACE("device %p, swapchain_idx %u.\n", device, swapchain_idx);

    if (swapchain_idx >= device->swapchain_count)
    {
        WARN("swapchain_idx %u >= swapchain_count %u.\n",
                swapchain_idx, device->swapchain_count);
        return NULL;
    }

    return device->swapchains[swapchain_idx];
}

static void device_load_logo(struct wined3d_device *device, const char *filename)
{
    struct wined3d_color_key color_key;
    struct wined3d_resource_desc desc;
    HBITMAP hbm;
    BITMAP bm;
    HRESULT hr;
    HDC dcb = NULL, dcs = NULL;

    if (!(hbm = LoadImageA(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION)))
    {
        ERR_(winediag)("Failed to load logo %s.\n", wine_dbgstr_a(filename));
        return;
    }
    GetObjectA(hbm, sizeof(BITMAP), &bm);

    if (!(dcb = CreateCompatibleDC(NULL)))
        goto out;
    SelectObject(dcb, hbm);

    desc.resource_type = WINED3D_RTYPE_TEXTURE_2D;
    desc.format = WINED3DFMT_B5G6R5_UNORM;
    desc.multisample_type = WINED3D_MULTISAMPLE_NONE;
    desc.multisample_quality = 0;
    desc.usage = WINED3DUSAGE_DYNAMIC;
    desc.access = WINED3D_RESOURCE_ACCESS_GPU;
    desc.width = bm.bmWidth;
    desc.height = bm.bmHeight;
    desc.depth = 1;
    desc.size = 0;
    if (FAILED(hr = wined3d_texture_create(device, &desc, 1, 1,
            WINED3D_TEXTURE_CREATE_MAPPABLE | WINED3D_TEXTURE_CREATE_GET_DC,
            NULL, NULL, &wined3d_null_parent_ops, &device->logo_texture)))
    {
        ERR("Wine logo requested, but failed to create texture, hr %#x.\n", hr);
        goto out;
    }

    if (FAILED(hr = wined3d_texture_get_dc(device->logo_texture, 0, &dcs)))
    {
        wined3d_texture_decref(device->logo_texture);
        device->logo_texture = NULL;
        goto out;
    }
    BitBlt(dcs, 0, 0, bm.bmWidth, bm.bmHeight, dcb, 0, 0, SRCCOPY);
    wined3d_texture_release_dc(device->logo_texture, 0, dcs);

    color_key.color_space_low_value = 0;
    color_key.color_space_high_value = 0;
    wined3d_texture_set_color_key(device->logo_texture, WINED3D_CKEY_SRC_BLT, &color_key);

out:
    if (dcb) DeleteDC(dcb);
    if (hbm) DeleteObject(hbm);
}

/* Context activation is done by the caller. */
static void create_dummy_textures(struct wined3d_device *device, struct wined3d_context *context)
{
    const struct wined3d_d3d_info *d3d_info = &device->adapter->d3d_info;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_dummy_textures *textures = &device->dummy_textures;
    unsigned int i;
    DWORD color;

    if (d3d_info->wined3d_creation_flags & WINED3D_LEGACY_UNBOUND_RESOURCE_COLOR)
        color = 0x000000ff;
    else
        color = 0x00000000;

    /* Under DirectX you can sample even if no texture is bound, whereas
     * OpenGL will only allow that when a valid texture is bound.
     * We emulate this by creating dummy textures and binding them
     * to each texture stage when the currently set D3D texture is NULL. */
    context_active_texture(context, gl_info, 0);

    gl_info->gl_ops.gl.p_glGenTextures(1, &textures->tex_1d);
    TRACE("Dummy 1D texture given name %u.\n", textures->tex_1d);
    gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_1D, textures->tex_1d);
    gl_info->gl_ops.gl.p_glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA8, 1, 0,
            GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color);

    gl_info->gl_ops.gl.p_glGenTextures(1, &textures->tex_2d);
    TRACE("Dummy 2D texture given name %u.\n", textures->tex_2d);
    gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, textures->tex_2d);
    gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0,
            GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color);

    if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
    {
        gl_info->gl_ops.gl.p_glGenTextures(1, &textures->tex_rect);
        TRACE("Dummy rectangle texture given name %u.\n", textures->tex_rect);
        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_RECTANGLE_ARB, textures->tex_rect);
        gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA8, 1, 1, 0,
                GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color);
    }

    if (gl_info->supported[EXT_TEXTURE3D])
    {
        gl_info->gl_ops.gl.p_glGenTextures(1, &textures->tex_3d);
        TRACE("Dummy 3D texture given name %u.\n", textures->tex_3d);
        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_3D, textures->tex_3d);
        GL_EXTCALL(glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, 1, 1, 1, 0,
                    GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color));
    }

    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
    {
        gl_info->gl_ops.gl.p_glGenTextures(1, &textures->tex_cube);
        TRACE("Dummy cube texture given name %u.\n", textures->tex_cube);
        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_CUBE_MAP, textures->tex_cube);
        for (i = GL_TEXTURE_CUBE_MAP_POSITIVE_X; i <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; ++i)
        {
            gl_info->gl_ops.gl.p_glTexImage2D(i, 0, GL_RGBA8, 1, 1, 0,
                    GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color);
        }
    }

    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP_ARRAY])
    {
        DWORD cube_array_data[6];

        gl_info->gl_ops.gl.p_glGenTextures(1, &textures->tex_cube_array);
        TRACE("Dummy cube array texture given name %u.\n", textures->tex_cube_array);
        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, textures->tex_cube_array);
        for (i = 0; i < ARRAY_SIZE(cube_array_data); ++i)
            cube_array_data[i] = color;
        GL_EXTCALL(glTexImage3D(GL_TEXTURE_CUBE_MAP_ARRAY, 0, GL_RGBA8, 1, 1, 6, 0,
                    GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, cube_array_data));
    }

    if (gl_info->supported[EXT_TEXTURE_ARRAY])
    {

        gl_info->gl_ops.gl.p_glGenTextures(1, &textures->tex_1d_array);
        TRACE("Dummy 1D array texture given name %u.\n", textures->tex_1d_array);
        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_1D_ARRAY, textures->tex_1d_array);
        gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_1D_ARRAY, 0, GL_RGBA8, 1, 1, 0,
                GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color);

        gl_info->gl_ops.gl.p_glGenTextures(1, &textures->tex_2d_array);
        TRACE("Dummy 2D array texture given name %u.\n", textures->tex_2d_array);
        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D_ARRAY, textures->tex_2d_array);
        GL_EXTCALL(glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8, 1, 1, 1, 0,
                    GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color));
    }

    if (gl_info->supported[ARB_TEXTURE_BUFFER_OBJECT])
    {
        GLuint buffer;

        GL_EXTCALL(glGenBuffers(1, &buffer));
        GL_EXTCALL(glBindBuffer(GL_TEXTURE_BUFFER, buffer));
        GL_EXTCALL(glBufferData(GL_TEXTURE_BUFFER, sizeof(color), &color, GL_STATIC_DRAW));
        GL_EXTCALL(glBindBuffer(GL_TEXTURE_BUFFER, 0));

        gl_info->gl_ops.gl.p_glGenTextures(1, &textures->tex_buffer);
        TRACE("Dummy buffer texture given name %u.\n", textures->tex_buffer);
        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_BUFFER, textures->tex_buffer);
        GL_EXTCALL(glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA8, buffer));
        GL_EXTCALL(glDeleteBuffers(1, &buffer));
    }

    if (gl_info->supported[ARB_TEXTURE_MULTISAMPLE])
    {
        gl_info->gl_ops.gl.p_glGenTextures(1, &textures->tex_2d_ms);
        TRACE("Dummy multisample texture given name %u.\n", textures->tex_2d_ms);
        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textures->tex_2d_ms);
        GL_EXTCALL(glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 1, GL_RGBA8, 1, 1, GL_TRUE));

        gl_info->gl_ops.gl.p_glGenTextures(1, &textures->tex_2d_ms_array);
        TRACE("Dummy multisample array texture given name %u.\n", textures->tex_2d_ms_array);
        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, textures->tex_2d_ms_array);
        GL_EXTCALL(glTexImage3DMultisample(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, 1, GL_RGBA8, 1, 1, 1, GL_TRUE));

        if (gl_info->supported[ARB_CLEAR_TEXTURE])
        {
            GL_EXTCALL(glClearTexImage(textures->tex_2d_ms, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color));
            GL_EXTCALL(glClearTexImage(textures->tex_2d_ms_array, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color));
        }
        else
        {
            WARN("ARB_clear_texture is currently required to clear dummy multisample textures.\n");
        }
    }

    checkGLcall("create dummy textures");

    context_bind_dummy_textures(device, context);
}

/* Context activation is done by the caller. */
static void destroy_dummy_textures(struct wined3d_device *device, struct wined3d_context *context)
{
    struct wined3d_dummy_textures *dummy_textures = &device->dummy_textures;
    const struct wined3d_gl_info *gl_info = context->gl_info;

    if (gl_info->supported[ARB_TEXTURE_MULTISAMPLE])
    {
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &dummy_textures->tex_2d_ms);
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &dummy_textures->tex_2d_ms_array);
    }

    if (gl_info->supported[ARB_TEXTURE_BUFFER_OBJECT])
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &dummy_textures->tex_buffer);

    if (gl_info->supported[EXT_TEXTURE_ARRAY])
    {
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &dummy_textures->tex_1d_array);
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &dummy_textures->tex_2d_array);
    }

    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP_ARRAY])
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &dummy_textures->tex_cube_array);

    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &dummy_textures->tex_cube);

    if (gl_info->supported[EXT_TEXTURE3D])
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &dummy_textures->tex_3d);

    if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &dummy_textures->tex_rect);

    gl_info->gl_ops.gl.p_glDeleteTextures(1, &dummy_textures->tex_2d);
    gl_info->gl_ops.gl.p_glDeleteTextures(1, &dummy_textures->tex_1d);

    checkGLcall("delete dummy textures");

    memset(dummy_textures, 0, sizeof(*dummy_textures));
}

/* Context activation is done by the caller. */
static void create_default_samplers(struct wined3d_device *device, struct wined3d_context *context)
{
    struct wined3d_sampler_desc desc;
    HRESULT hr;

    desc.address_u = WINED3D_TADDRESS_WRAP;
    desc.address_v = WINED3D_TADDRESS_WRAP;
    desc.address_w = WINED3D_TADDRESS_WRAP;
    memset(desc.border_color, 0, sizeof(desc.border_color));
    desc.mag_filter = WINED3D_TEXF_POINT;
    desc.min_filter = WINED3D_TEXF_POINT;
    desc.mip_filter = WINED3D_TEXF_NONE;
    desc.lod_bias = 0.0f;
    desc.min_lod = -1000.0f;
    desc.max_lod =  1000.0f;
    desc.mip_base_level = 0;
    desc.max_anisotropy = 1;
    desc.compare = FALSE;
    desc.comparison_func = WINED3D_CMP_NEVER;
    desc.srgb_decode = TRUE;

    /* In SM4+ shaders there is a separation between resources and samplers. Some shader
     * instructions allow access to resources without using samplers.
     * In GLSL, resources are always accessed through sampler or image variables. The default
     * sampler object is used to emulate the direct resource access when there is no sampler state
     * to use.
     */
    if (FAILED(hr = wined3d_sampler_create(device, &desc, NULL, &wined3d_null_parent_ops, &device->default_sampler)))
    {
        ERR("Failed to create default sampler, hr %#x.\n", hr);
        device->default_sampler = NULL;
    }

    /* In D3D10+, a NULL sampler maps to the default sampler state. */
    desc.address_u = WINED3D_TADDRESS_CLAMP;
    desc.address_v = WINED3D_TADDRESS_CLAMP;
    desc.address_w = WINED3D_TADDRESS_CLAMP;
    desc.mag_filter = WINED3D_TEXF_LINEAR;
    desc.min_filter = WINED3D_TEXF_LINEAR;
    desc.mip_filter = WINED3D_TEXF_LINEAR;
    if (FAILED(hr = wined3d_sampler_create(device, &desc, NULL, &wined3d_null_parent_ops, &device->null_sampler)))
    {
        ERR("Failed to create null sampler, hr %#x.\n", hr);
        device->null_sampler = NULL;
    }
}

/* Context activation is done by the caller. */
static void destroy_default_samplers(struct wined3d_device *device, struct wined3d_context *context)
{
    wined3d_sampler_decref(device->default_sampler);
    device->default_sampler = NULL;
    wined3d_sampler_decref(device->null_sampler);
    device->null_sampler = NULL;
}

static LONG fullscreen_style(LONG style)
{
    /* Make sure the window is managed, otherwise we won't get keyboard input. */
    style |= WS_POPUP | WS_SYSMENU;
    style &= ~(WS_CAPTION | WS_THICKFRAME);

    return style;
}

static LONG fullscreen_exstyle(LONG exstyle)
{
    /* Filter out window decorations. */
    exstyle &= ~(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE);

    return exstyle;
}

void CDECL wined3d_device_setup_fullscreen_window(struct wined3d_device *device, HWND window, UINT w, UINT h)
{
    BOOL filter_messages;
    LONG style, exstyle;

    TRACE("Setting up window %p for fullscreen mode.\n", window);

    if (device->style || device->exStyle)
    {
        ERR("Changing the window style for window %p, but another style (%08x, %08x) is already stored.\n",
                window, device->style, device->exStyle);
    }

    device->style = GetWindowLongW(window, GWL_STYLE);
    device->exStyle = GetWindowLongW(window, GWL_EXSTYLE);

    style = fullscreen_style(device->style);
    exstyle = fullscreen_exstyle(device->exStyle);

    TRACE("Old style was %08x, %08x, setting to %08x, %08x.\n",
            device->style, device->exStyle, style, exstyle);

    filter_messages = device->filter_messages;
    device->filter_messages = TRUE;

    SetWindowLongW(window, GWL_STYLE, style);
    SetWindowLongW(window, GWL_EXSTYLE, exstyle);
    SetWindowPos(window, HWND_TOPMOST, 0, 0, w, h, SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOACTIVATE);

    device->filter_messages = filter_messages;
}

void CDECL wined3d_device_restore_fullscreen_window(struct wined3d_device *device, HWND window,
        const RECT *window_rect)
{
    unsigned int window_pos_flags = SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE;
    BOOL filter_messages;
    LONG style, exstyle;
    RECT rect = {0};

    if (!device->style && !device->exStyle)
        return;

    style = GetWindowLongW(window, GWL_STYLE);
    exstyle = GetWindowLongW(window, GWL_EXSTYLE);

    /* These flags are set by wined3d_device_setup_fullscreen_window, not the
     * application, and we want to ignore them in the test below, since it's
     * not the application's fault that they changed. Additionally, we want to
     * preserve the current status of these flags (i.e. don't restore them) to
     * more closely emulate the behavior of Direct3D, which leaves these flags
     * alone when returning to windowed mode. */
    device->style ^= (device->style ^ style) & WS_VISIBLE;
    device->exStyle ^= (device->exStyle ^ exstyle) & WS_EX_TOPMOST;

    TRACE("Restoring window style of window %p to %08x, %08x.\n",
            window, device->style, device->exStyle);

    filter_messages = device->filter_messages;
    device->filter_messages = TRUE;

    /* Only restore the style if the application didn't modify it during the
     * fullscreen phase. Some applications change it before calling Reset()
     * when switching between windowed and fullscreen modes (HL2), some
     * depend on the original style (Eve Online). */
    if (style == fullscreen_style(device->style) && exstyle == fullscreen_exstyle(device->exStyle))
    {
        SetWindowLongW(window, GWL_STYLE, device->style);
        SetWindowLongW(window, GWL_EXSTYLE, device->exStyle);
    }

    if (window_rect)
        rect = *window_rect;
    else
        window_pos_flags |= (SWP_NOMOVE | SWP_NOSIZE);
    SetWindowPos(window, 0, rect.left, rect.top,
            rect.right - rect.left, rect.bottom - rect.top, window_pos_flags);

    device->filter_messages = filter_messages;

    /* Delete the old values. */
    device->style = 0;
    device->exStyle = 0;
}

HRESULT CDECL wined3d_device_acquire_focus_window(struct wined3d_device *device, HWND window)
{
    TRACE("device %p, window %p.\n", device, window);

    if (!wined3d_register_window(window, device))
    {
        ERR("Failed to register window %p.\n", window);
        return E_FAIL;
    }

    InterlockedExchangePointer((void **)&device->focus_window, window);
    SetWindowPos(window, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

    return WINED3D_OK;
}

void CDECL wined3d_device_release_focus_window(struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    if (device->focus_window) wined3d_unregister_window(device->focus_window);
    InterlockedExchangePointer((void **)&device->focus_window, NULL);
}

static void device_init_swapchain_state(struct wined3d_device *device, struct wined3d_swapchain *swapchain)
{
    BOOL ds_enable = swapchain->desc.enable_auto_depth_stencil;
    unsigned int i;

    for (i = 0; i < device->adapter->gl_info.limits.buffers; ++i)
    {
        wined3d_device_set_rendertarget_view(device, i, NULL, FALSE);
    }
    if (device->back_buffer_view)
        wined3d_device_set_rendertarget_view(device, 0, device->back_buffer_view, TRUE);

    wined3d_device_set_depth_stencil_view(device, ds_enable ? device->auto_depth_stencil_view : NULL);
}

static void wined3d_device_delete_opengl_contexts_cs(void *object)
{
    struct wined3d_resource *resource, *cursor;
    struct wined3d_device *device = object;
    struct wined3d_context *context;
    struct wined3d_shader *shader;

    LIST_FOR_EACH_ENTRY_SAFE(resource, cursor, &device->resources, struct wined3d_resource, resource_list_entry)
    {
        TRACE("Unloading resource %p.\n", resource);
        wined3d_cs_emit_unload_resource(device->cs, resource);
    }

    LIST_FOR_EACH_ENTRY(shader, &device->shaders, struct wined3d_shader, shader_list_entry)
    {
        device->shader_backend->shader_destroy(shader);
    }

    context = context_acquire(device, NULL, 0);
    device->blitter->ops->blitter_destroy(device->blitter, context);
    device->shader_backend->shader_free_private(device);
    destroy_dummy_textures(device, context);
    destroy_default_samplers(device, context);
    context_release(context);

    while (device->context_count)
    {
        if (device->contexts[0]->swapchain)
            swapchain_destroy_contexts(device->contexts[0]->swapchain);
        else
            context_destroy(device, device->contexts[0]);
    }
}

static void wined3d_device_delete_opengl_contexts(struct wined3d_device *device)
{
    wined3d_cs_destroy_object(device->cs, wined3d_device_delete_opengl_contexts_cs, device);
    device->cs->ops->finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);
}

static void wined3d_device_create_primary_opengl_context_cs(void *object)
{
    struct wined3d_device *device = object;
    struct wined3d_swapchain *swapchain;
    struct wined3d_context *context;
    struct wined3d_texture *target;
    HRESULT hr;

    if (FAILED(hr = device->shader_backend->shader_alloc_private(device,
            device->adapter->vertex_pipe, device->adapter->fragment_pipe)))
    {
        ERR("Failed to allocate shader private data, hr %#x.\n", hr);
        return;
    }

    if (!(device->blitter = wined3d_cpu_blitter_create()))
    {
        ERR("Failed to create CPU blitter.\n");
        device->shader_backend->shader_free_private(device);
        return;
    }
    wined3d_ffp_blitter_create(&device->blitter, &device->adapter->gl_info);
    wined3d_arbfp_blitter_create(&device->blitter, device);
    wined3d_fbo_blitter_create(&device->blitter, &device->adapter->gl_info);
    wined3d_raw_blitter_create(&device->blitter, &device->adapter->gl_info);

    swapchain = device->swapchains[0];
    target = swapchain->back_buffers ? swapchain->back_buffers[0] : swapchain->front_buffer;
    context = context_acquire(device, target, 0);
    create_dummy_textures(device, context);
    create_default_samplers(device, context);
    context_release(context);
}

static HRESULT wined3d_device_create_primary_opengl_context(struct wined3d_device *device)
{
    wined3d_cs_init_object(device->cs, wined3d_device_create_primary_opengl_context_cs, device);
    device->cs->ops->finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);
    if (!device->swapchains[0]->num_contexts)
        return E_FAIL;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_init_3d(struct wined3d_device *device,
        struct wined3d_swapchain_desc *swapchain_desc)
{
    static const struct wined3d_color black = {0.0f, 0.0f, 0.0f, 0.0f};
    struct wined3d_swapchain *swapchain = NULL;
    DWORD clear_flags = 0;
    HRESULT hr;

    TRACE("device %p, swapchain_desc %p.\n", device, swapchain_desc);

    if (device->d3d_initialized)
        return WINED3DERR_INVALIDCALL;
    if (device->wined3d->flags & WINED3D_NO3D)
        return WINED3DERR_INVALIDCALL;

    memset(device->fb.render_targets, 0, sizeof(device->fb.render_targets));

    /* Setup the implicit swapchain. This also initializes a context. */
    TRACE("Creating implicit swapchain.\n");
    if (FAILED(hr = device->device_parent->ops->create_swapchain(device->device_parent,
            swapchain_desc, &swapchain)))
    {
        WARN("Failed to create implicit swapchain.\n");
        goto err_out;
    }

    if (swapchain_desc->backbuffer_count)
    {
        struct wined3d_resource *back_buffer = &swapchain->back_buffers[0]->resource;
        struct wined3d_view_desc view_desc;

        view_desc.format_id = back_buffer->format->id;
        view_desc.flags = 0;
        view_desc.u.texture.level_idx = 0;
        view_desc.u.texture.level_count = 1;
        view_desc.u.texture.layer_idx = 0;
        view_desc.u.texture.layer_count = 1;
        if (FAILED(hr = wined3d_rendertarget_view_create(&view_desc, back_buffer,
                NULL, &wined3d_null_parent_ops, &device->back_buffer_view)))
        {
            ERR("Failed to create rendertarget view, hr %#x.\n", hr);
            goto err_out;
        }
    }

    device->swapchain_count = 1;
    if (!(device->swapchains = heap_calloc(device->swapchain_count, sizeof(*device->swapchains))))
    {
        ERR("Out of memory!\n");
        goto err_out;
    }
    device->swapchains[0] = swapchain;

    if (FAILED(hr = wined3d_device_create_primary_opengl_context(device)))
        goto err_out;
    device_init_swapchain_state(device, swapchain);

    device->contexts[0]->last_was_rhw = 0;

    TRACE("All defaults now set up.\n");

    /* Clear the screen */
    if (swapchain->back_buffers && swapchain->back_buffers[0])
        clear_flags |= WINED3DCLEAR_TARGET;
    if (swapchain_desc->enable_auto_depth_stencil)
        clear_flags |= WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL;
    if (clear_flags)
        wined3d_device_clear(device, 0, NULL, clear_flags, &black, 1.0f, 0);

    device->d3d_initialized = TRUE;

    if (wined3d_settings.logo)
        device_load_logo(device, wined3d_settings.logo);
    return WINED3D_OK;

err_out:
    heap_free(device->swapchains);
    device->swapchain_count = 0;
    if (device->back_buffer_view)
        wined3d_rendertarget_view_decref(device->back_buffer_view);
    if (swapchain)
        wined3d_swapchain_decref(swapchain);

    return hr;
}

HRESULT CDECL wined3d_device_init_gdi(struct wined3d_device *device,
        struct wined3d_swapchain_desc *swapchain_desc)
{
    struct wined3d_swapchain *swapchain = NULL;
    HRESULT hr;

    TRACE("device %p, swapchain_desc %p.\n", device, swapchain_desc);

    /* Setup the implicit swapchain */
    TRACE("Creating implicit swapchain\n");
    hr = device->device_parent->ops->create_swapchain(device->device_parent,
            swapchain_desc, &swapchain);
    if (FAILED(hr))
    {
        WARN("Failed to create implicit swapchain\n");
        goto err_out;
    }

    device->swapchain_count = 1;
    if (!(device->swapchains = heap_calloc(device->swapchain_count, sizeof(*device->swapchains))))
    {
        ERR("Out of memory!\n");
        goto err_out;
    }
    device->swapchains[0] = swapchain;

    if (!(device->blitter = wined3d_cpu_blitter_create()))
    {
        ERR("Failed to create CPU blitter.\n");
        heap_free(device->swapchains);
        device->swapchain_count = 0;
        goto err_out;
    }

    return WINED3D_OK;

err_out:
    wined3d_swapchain_decref(swapchain);
    return hr;
}

static void device_free_sampler(struct wine_rb_entry *entry, void *context)
{
    struct wined3d_sampler *sampler = WINE_RB_ENTRY_VALUE(entry, struct wined3d_sampler, entry);

    wined3d_sampler_decref(sampler);
}

HRESULT CDECL wined3d_device_uninit_3d(struct wined3d_device *device)
{
    unsigned int i;

    TRACE("device %p.\n", device);

    if (!device->d3d_initialized)
        return WINED3DERR_INVALIDCALL;

    device->cs->ops->finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);

    if (device->logo_texture)
        wined3d_texture_decref(device->logo_texture);
    if (device->cursor_texture)
        wined3d_texture_decref(device->cursor_texture);

    state_unbind_resources(&device->state);

    wine_rb_clear(&device->samplers, device_free_sampler, NULL);

#if defined(STAGING_CSMT)
    context_set_current(NULL);
#endif /* STAGING_CSMT */
    wined3d_device_delete_opengl_contexts(device);

    if (device->fb.depth_stencil)
    {
        struct wined3d_rendertarget_view *view = device->fb.depth_stencil;

        TRACE("Releasing depth/stencil view %p.\n", view);

        device->fb.depth_stencil = NULL;
        wined3d_rendertarget_view_decref(view);
    }

    if (device->auto_depth_stencil_view)
    {
        struct wined3d_rendertarget_view *view = device->auto_depth_stencil_view;

        device->auto_depth_stencil_view = NULL;
        if (wined3d_rendertarget_view_decref(view))
            ERR("Something's still holding the auto depth/stencil view (%p).\n", view);
    }

    for (i = 0; i < device->adapter->gl_info.limits.buffers; ++i)
    {
        wined3d_device_set_rendertarget_view(device, i, NULL, FALSE);
    }
    if (device->back_buffer_view)
    {
        wined3d_rendertarget_view_decref(device->back_buffer_view);
        device->back_buffer_view = NULL;
    }

    for (i = 0; i < device->swapchain_count; ++i)
    {
        TRACE("Releasing the implicit swapchain %u.\n", i);
        if (wined3d_swapchain_decref(device->swapchains[i]))
            FIXME("Something's still holding the implicit swapchain.\n");
    }

    heap_free(device->swapchains);
    device->swapchains = NULL;
    device->swapchain_count = 0;

    device->d3d_initialized = FALSE;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_uninit_gdi(struct wined3d_device *device)
{
    unsigned int i;

    device->blitter->ops->blitter_destroy(device->blitter, NULL);

    for (i = 0; i < device->swapchain_count; ++i)
    {
        TRACE("Releasing the implicit swapchain %u.\n", i);
        if (wined3d_swapchain_decref(device->swapchains[i]))
            FIXME("Something's still holding the implicit swapchain.\n");
    }

    heap_free(device->swapchains);
    device->swapchains = NULL;
    device->swapchain_count = 0;
    return WINED3D_OK;
}

/* Enables thread safety in the wined3d device and its resources. Called by DirectDraw
 * from SetCooperativeLevel if DDSCL_MULTITHREADED is specified, and by d3d8/9 from
 * CreateDevice if D3DCREATE_MULTITHREADED is passed.
 *
 * There is no way to deactivate thread safety once it is enabled.
 */
void CDECL wined3d_device_set_multithreaded(struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    /* For now just store the flag (needed in case of ddraw). */
    device->create_parms.flags |= WINED3DCREATE_MULTITHREADED;
}

UINT CDECL wined3d_device_get_available_texture_mem(const struct wined3d_device *device)
{
    /* const struct wined3d_gl_info *gl_info = &device->adapter->gl_info; */

    TRACE("device %p.\n", device);

    /* We can not acquire the context unless there is a swapchain. */
    /*
    if (device->swapchains && gl_info->supported[NVX_GPU_MEMORY_INFO] &&
            !wined3d_settings.emulated_textureram)
    {
        GLint vram_free_kb;
        UINT64 vram_free;

        struct wined3d_context *context = context_acquire(device, NULL, 0);
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &vram_free_kb);
        vram_free = (UINT64)vram_free_kb * 1024;
        context_release(context);

        TRACE("Total 0x%s bytes. emulation 0x%s left, driver 0x%s left.\n",
                wine_dbgstr_longlong(device->adapter->vram_bytes),
                wine_dbgstr_longlong(device->adapter->vram_bytes - device->adapter->vram_bytes_used),
                wine_dbgstr_longlong(vram_free));

        vram_free = min(vram_free, device->adapter->vram_bytes - device->adapter->vram_bytes_used);
        return min(UINT_MAX, vram_free);
    }
    */

    TRACE("Emulating 0x%s bytes. 0x%s used, returning 0x%s left.\n",
            wine_dbgstr_longlong(device->adapter->vram_bytes),
            wine_dbgstr_longlong(device->adapter->vram_bytes_used),
            wine_dbgstr_longlong(device->adapter->vram_bytes - device->adapter->vram_bytes_used));

    return min(UINT_MAX, device->adapter->vram_bytes - device->adapter->vram_bytes_used);
}

void CDECL wined3d_device_set_stream_output(struct wined3d_device *device, UINT idx,
        struct wined3d_buffer *buffer, UINT offset)
{
    struct wined3d_stream_output *stream;
    struct wined3d_buffer *prev_buffer;

    TRACE("device %p, idx %u, buffer %p, offset %u.\n", device, idx, buffer, offset);

    if (idx >= WINED3D_MAX_STREAM_OUTPUT_BUFFERS)
    {
        WARN("Invalid stream output %u.\n", idx);
        return;
    }

    stream = &device->update_state->stream_output[idx];
    prev_buffer = stream->buffer;

    if (buffer)
        wined3d_buffer_incref(buffer);
    stream->buffer = buffer;
    stream->offset = offset;
    if (!device->recording)
        wined3d_cs_emit_set_stream_output(device->cs, idx, buffer, offset);
    if (prev_buffer)
        wined3d_buffer_decref(prev_buffer);
}

struct wined3d_buffer * CDECL wined3d_device_get_stream_output(struct wined3d_device *device,
        UINT idx, UINT *offset)
{
    TRACE("device %p, idx %u, offset %p.\n", device, idx, offset);

    if (idx >= WINED3D_MAX_STREAM_OUTPUT_BUFFERS)
    {
        WARN("Invalid stream output %u.\n", idx);
        return NULL;
    }

    if (offset)
        *offset = device->state.stream_output[idx].offset;
    return device->state.stream_output[idx].buffer;
}

HRESULT CDECL wined3d_device_set_stream_source(struct wined3d_device *device, UINT stream_idx,
        struct wined3d_buffer *buffer, UINT offset, UINT stride)
{
    struct wined3d_stream_state *stream;
    struct wined3d_buffer *prev_buffer;

    TRACE("device %p, stream_idx %u, buffer %p, offset %u, stride %u.\n",
            device, stream_idx, buffer, offset, stride);

    if (stream_idx >= MAX_STREAMS)
    {
        WARN("Stream index %u out of range.\n", stream_idx);
        return WINED3DERR_INVALIDCALL;
    }
    else if (offset & 0x3)
    {
        WARN("Offset %u is not 4 byte aligned.\n", offset);
        return WINED3DERR_INVALIDCALL;
    }

    stream = &device->update_state->streams[stream_idx];
    prev_buffer = stream->buffer;

    if (device->recording)
        device->recording->changed.streamSource |= 1u << stream_idx;

    if (prev_buffer == buffer
            && stream->stride == stride
            && stream->offset == offset)
    {
       TRACE("Application is setting the old values over, nothing to do.\n");
       return WINED3D_OK;
    }

    stream->buffer = buffer;
    if (buffer)
    {
        stream->stride = stride;
        stream->offset = offset;
        wined3d_buffer_incref(buffer);
    }

    if (!device->recording)
        wined3d_cs_emit_set_stream_source(device->cs, stream_idx, buffer, offset, stride);
    if (prev_buffer)
        wined3d_buffer_decref(prev_buffer);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_stream_source(const struct wined3d_device *device,
        UINT stream_idx, struct wined3d_buffer **buffer, UINT *offset, UINT *stride)
{
    const struct wined3d_stream_state *stream;

    TRACE("device %p, stream_idx %u, buffer %p, offset %p, stride %p.\n",
            device, stream_idx, buffer, offset, stride);

    if (stream_idx >= MAX_STREAMS)
    {
        WARN("Stream index %u out of range.\n", stream_idx);
        return WINED3DERR_INVALIDCALL;
    }

    stream = &device->state.streams[stream_idx];
    *buffer = stream->buffer;
    if (offset)
        *offset = stream->offset;
    *stride = stream->stride;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_stream_source_freq(struct wined3d_device *device, UINT stream_idx, UINT divider)
{
    struct wined3d_stream_state *stream;
    UINT old_flags, old_freq;

    TRACE("device %p, stream_idx %u, divider %#x.\n", device, stream_idx, divider);

    /* Verify input. At least in d3d9 this is invalid. */
    if ((divider & WINED3DSTREAMSOURCE_INSTANCEDATA) && (divider & WINED3DSTREAMSOURCE_INDEXEDDATA))
    {
        WARN("INSTANCEDATA and INDEXEDDATA were set, returning D3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }
    if ((divider & WINED3DSTREAMSOURCE_INSTANCEDATA) && !stream_idx)
    {
        WARN("INSTANCEDATA used on stream 0, returning D3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }
    if (!divider)
    {
        WARN("Divider is 0, returning D3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    stream = &device->update_state->streams[stream_idx];
    old_flags = stream->flags;
    old_freq = stream->frequency;

    stream->flags = divider & (WINED3DSTREAMSOURCE_INSTANCEDATA | WINED3DSTREAMSOURCE_INDEXEDDATA);
    stream->frequency = divider & 0x7fffff;

    if (device->recording)
        device->recording->changed.streamFreq |= 1u << stream_idx;
    else if (stream->frequency != old_freq || stream->flags != old_flags)
        wined3d_cs_emit_set_stream_source_freq(device->cs, stream_idx, stream->frequency, stream->flags);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_stream_source_freq(const struct wined3d_device *device,
        UINT stream_idx, UINT *divider)
{
    const struct wined3d_stream_state *stream;

    TRACE("device %p, stream_idx %u, divider %p.\n", device, stream_idx, divider);

    stream = &device->state.streams[stream_idx];
    *divider = stream->flags | stream->frequency;

    TRACE("Returning %#x.\n", *divider);

    return WINED3D_OK;
}

void CDECL wined3d_device_set_transform(struct wined3d_device *device,
        enum wined3d_transform_state d3dts, const struct wined3d_matrix *matrix)
{
    TRACE("device %p, state %s, matrix %p.\n",
            device, debug_d3dtstype(d3dts), matrix);
    TRACE("%.8e %.8e %.8e %.8e\n", matrix->_11, matrix->_12, matrix->_13, matrix->_14);
    TRACE("%.8e %.8e %.8e %.8e\n", matrix->_21, matrix->_22, matrix->_23, matrix->_24);
    TRACE("%.8e %.8e %.8e %.8e\n", matrix->_31, matrix->_32, matrix->_33, matrix->_34);
    TRACE("%.8e %.8e %.8e %.8e\n", matrix->_41, matrix->_42, matrix->_43, matrix->_44);

    /* Handle recording of state blocks. */
    if (device->recording)
    {
        TRACE("Recording... not performing anything.\n");
        device->recording->changed.transform[d3dts >> 5] |= 1u << (d3dts & 0x1f);
        device->update_state->transforms[d3dts] = *matrix;
        return;
    }

    /* If the new matrix is the same as the current one,
     * we cut off any further processing. this seems to be a reasonable
     * optimization because as was noticed, some apps (warcraft3 for example)
     * tend towards setting the same matrix repeatedly for some reason.
     *
     * From here on we assume that the new matrix is different, wherever it matters. */
    if (!memcmp(&device->state.transforms[d3dts], matrix, sizeof(*matrix)))
    {
        TRACE("The application is setting the same matrix over again.\n");
        return;
    }

    device->state.transforms[d3dts] = *matrix;
    wined3d_cs_emit_set_transform(device->cs, d3dts, matrix);
}

void CDECL wined3d_device_get_transform(const struct wined3d_device *device,
        enum wined3d_transform_state state, struct wined3d_matrix *matrix)
{
    TRACE("device %p, state %s, matrix %p.\n", device, debug_d3dtstype(state), matrix);

    *matrix = device->state.transforms[state];
}

void CDECL wined3d_device_multiply_transform(struct wined3d_device *device,
        enum wined3d_transform_state state, const struct wined3d_matrix *matrix)
{
    const struct wined3d_matrix *mat;
    struct wined3d_matrix temp;

    TRACE("device %p, state %s, matrix %p.\n", device, debug_d3dtstype(state), matrix);

    /* Note: Using 'updateStateBlock' rather than 'stateblock' in the code
     * below means it will be recorded in a state block change, but it
     * works regardless where it is recorded.
     * If this is found to be wrong, change to StateBlock. */
    if (state > HIGHEST_TRANSFORMSTATE)
    {
        WARN("Unhandled transform state %#x.\n", state);
        return;
    }

    mat = &device->update_state->transforms[state];
    multiply_matrix(&temp, mat, matrix);

    /* Apply change via set transform - will reapply to eg. lights this way. */
    wined3d_device_set_transform(device, state, &temp);
}

/* Note lights are real special cases. Although the device caps state only
 * e.g. 8 are supported, you can reference any indexes you want as long as
 * that number max are enabled at any one point in time. Therefore since the
 * indices can be anything, we need a hashmap of them. However, this causes
 * stateblock problems. When capturing the state block, I duplicate the
 * hashmap, but when recording, just build a chain pretty much of commands to
 * be replayed. */
HRESULT CDECL wined3d_device_set_light(struct wined3d_device *device,
        UINT light_idx, const struct wined3d_light *light)
{
    UINT hash_idx = LIGHTMAP_HASHFUNC(light_idx);
    struct wined3d_light_info *object = NULL;
    float rho;

    TRACE("device %p, light_idx %u, light %p.\n", device, light_idx, light);

    /* Check the parameter range. Need for speed most wanted sets junk lights
     * which confuse the GL driver. */
    if (!light)
        return WINED3DERR_INVALIDCALL;

    switch (light->type)
    {
        case WINED3D_LIGHT_POINT:
        case WINED3D_LIGHT_SPOT:
        case WINED3D_LIGHT_GLSPOT:
            /* Incorrect attenuation values can cause the gl driver to crash.
             * Happens with Need for speed most wanted. */
            if (light->attenuation0 < 0.0f || light->attenuation1 < 0.0f || light->attenuation2 < 0.0f)
            {
                WARN("Attenuation is negative, returning WINED3DERR_INVALIDCALL.\n");
                return WINED3DERR_INVALIDCALL;
            }
            break;

        case WINED3D_LIGHT_DIRECTIONAL:
        case WINED3D_LIGHT_PARALLELPOINT:
            /* Ignores attenuation */
            break;

        default:
        WARN("Light type out of range, returning WINED3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (!(object = wined3d_state_get_light(device->update_state, light_idx)))
    {
        TRACE("Adding new light\n");
        if (!(object = heap_alloc_zero(sizeof(*object))))
            return E_OUTOFMEMORY;

        list_add_head(&device->update_state->light_map[hash_idx], &object->entry);
        object->glIndex = -1;
        object->OriginalIndex = light_idx;
    }

    /* Initialize the object. */
    TRACE("Light %u setting to type %#x, diffuse %s, specular %s, ambient %s, "
            "position {%.8e, %.8e, %.8e}, direction {%.8e, %.8e, %.8e}, "
            "range %.8e, falloff %.8e, theta %.8e, phi %.8e.\n",
            light_idx, light->type, debug_color(&light->diffuse),
            debug_color(&light->specular), debug_color(&light->ambient),
            light->position.x, light->position.y, light->position.z,
            light->direction.x, light->direction.y, light->direction.z,
            light->range, light->falloff, light->theta, light->phi);

    /* Save away the information. */
    object->OriginalParms = *light;

    switch (light->type)
    {
        case WINED3D_LIGHT_POINT:
            /* Position */
            object->position.x = light->position.x;
            object->position.y = light->position.y;
            object->position.z = light->position.z;
            object->position.w = 1.0f;
            object->cutoff = 180.0f;
            /* FIXME: Range */
            break;

        case WINED3D_LIGHT_DIRECTIONAL:
            /* Direction */
            object->direction.x = -light->direction.x;
            object->direction.y = -light->direction.y;
            object->direction.z = -light->direction.z;
            object->direction.w = 0.0f;
            object->exponent = 0.0f;
            object->cutoff = 180.0f;
            break;

        case WINED3D_LIGHT_SPOT:
            /* Position */
            object->position.x = light->position.x;
            object->position.y = light->position.y;
            object->position.z = light->position.z;
            object->position.w = 1.0f;

            /* Direction */
            object->direction.x = light->direction.x;
            object->direction.y = light->direction.y;
            object->direction.z = light->direction.z;
            object->direction.w = 0.0f;

            /* opengl-ish and d3d-ish spot lights use too different models
             * for the light "intensity" as a function of the angle towards
             * the main light direction, so we only can approximate very
             * roughly. However, spot lights are rather rarely used in games
             * (if ever used at all). Furthermore if still used, probably
             * nobody pays attention to such details. */
            if (!light->falloff)
            {
                /* Falloff = 0 is easy, because d3d's and opengl's spot light
                 * equations have the falloff resp. exponent parameter as an
                 * exponent, so the spot light lighting will always be 1.0 for
                 * both of them, and we don't have to care for the rest of the
                 * rather complex calculation. */
                object->exponent = 0.0f;
            }
            else
            {
                rho = light->theta + (light->phi - light->theta) / (2 * light->falloff);
                if (rho < 0.0001f)
                    rho = 0.0001f;
                object->exponent = -0.3f / logf(cosf(rho / 2));
            }

            if (object->exponent > 128.0f)
                object->exponent = 128.0f;

            object->cutoff = (float)(light->phi * 90 / M_PI);
            /* FIXME: Range */
            break;

        case WINED3D_LIGHT_PARALLELPOINT:
            object->position.x = light->position.x;
            object->position.y = light->position.y;
            object->position.z = light->position.z;
            object->position.w = 1.0f;
            break;

        default:
            FIXME("Unrecognized light type %#x.\n", light->type);
    }

    if (!device->recording)
        wined3d_cs_emit_set_light(device->cs, object);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_light(const struct wined3d_device *device,
        UINT light_idx, struct wined3d_light *light)
{
    struct wined3d_light_info *light_info;

    TRACE("device %p, light_idx %u, light %p.\n", device, light_idx, light);

    if (!(light_info = wined3d_state_get_light(&device->state, light_idx)))
    {
        TRACE("Light information requested but light not defined\n");
        return WINED3DERR_INVALIDCALL;
    }

    *light = light_info->OriginalParms;
    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_light_enable(struct wined3d_device *device, UINT light_idx, BOOL enable)
{
    struct wined3d_light_info *light_info;

    TRACE("device %p, light_idx %u, enable %#x.\n", device, light_idx, enable);

    /* Special case - enabling an undefined light creates one with a strict set of parameters. */
    if (!(light_info = wined3d_state_get_light(device->update_state, light_idx)))
    {
        TRACE("Light enabled requested but light not defined, so defining one!\n");
        wined3d_device_set_light(device, light_idx, &WINED3D_default_light);

        if (!(light_info = wined3d_state_get_light(device->update_state, light_idx)))
        {
            FIXME("Adding default lights has failed dismally\n");
            return WINED3DERR_INVALIDCALL;
        }
    }

    wined3d_state_enable_light(device->update_state, &device->adapter->d3d_info, light_info, enable);
    if (!device->recording)
        wined3d_cs_emit_set_light_enable(device->cs, light_idx, enable);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_light_enable(const struct wined3d_device *device, UINT light_idx, BOOL *enable)
{
    struct wined3d_light_info *light_info;

    TRACE("device %p, light_idx %u, enable %p.\n", device, light_idx, enable);

    if (!(light_info = wined3d_state_get_light(&device->state, light_idx)))
    {
        TRACE("Light enabled state requested but light not defined.\n");
        return WINED3DERR_INVALIDCALL;
    }
    /* true is 128 according to SetLightEnable */
    *enable = light_info->enabled ? 128 : 0;
    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_clip_plane(struct wined3d_device *device,
        UINT plane_idx, const struct wined3d_vec4 *plane)
{
    TRACE("device %p, plane_idx %u, plane %p.\n", device, plane_idx, plane);

    if (plane_idx >= device->adapter->gl_info.limits.user_clip_distances)
    {
        TRACE("Application has requested clipplane this device doesn't support.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (device->recording)
        device->recording->changed.clipplane |= 1u << plane_idx;

    if (!memcmp(&device->update_state->clip_planes[plane_idx], plane, sizeof(*plane)))
    {
        TRACE("Application is setting old values over, nothing to do.\n");
        return WINED3D_OK;
    }

    device->update_state->clip_planes[plane_idx] = *plane;

    if (!device->recording)
        wined3d_cs_emit_set_clip_plane(device->cs, plane_idx, plane);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_clip_plane(const struct wined3d_device *device,
        UINT plane_idx, struct wined3d_vec4 *plane)
{
    TRACE("device %p, plane_idx %u, plane %p.\n", device, plane_idx, plane);

    if (plane_idx >= device->adapter->gl_info.limits.user_clip_distances)
    {
        TRACE("Application has requested clipplane this device doesn't support.\n");
        return WINED3DERR_INVALIDCALL;
    }

    *plane = device->state.clip_planes[plane_idx];

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_clip_status(struct wined3d_device *device,
        const struct wined3d_clip_status *clip_status)
{
    FIXME("device %p, clip_status %p stub!\n", device, clip_status);

    if (!clip_status)
        return WINED3DERR_INVALIDCALL;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_clip_status(const struct wined3d_device *device,
        struct wined3d_clip_status *clip_status)
{
    FIXME("device %p, clip_status %p stub!\n", device, clip_status);

    if (!clip_status)
        return WINED3DERR_INVALIDCALL;

    return WINED3D_OK;
}

void CDECL wined3d_device_set_material(struct wined3d_device *device, const struct wined3d_material *material)
{
    TRACE("device %p, material %p.\n", device, material);

    device->update_state->material = *material;

    if (device->recording)
        device->recording->changed.material = TRUE;
    else
        wined3d_cs_emit_set_material(device->cs, material);
}

void CDECL wined3d_device_get_material(const struct wined3d_device *device, struct wined3d_material *material)
{
    TRACE("device %p, material %p.\n", device, material);

    *material = device->state.material;

    TRACE("diffuse %s\n", debug_color(&material->diffuse));
    TRACE("ambient %s\n", debug_color(&material->ambient));
    TRACE("specular %s\n", debug_color(&material->specular));
    TRACE("emissive %s\n", debug_color(&material->emissive));
    TRACE("power %.8e.\n", material->power);
}

void CDECL wined3d_device_set_index_buffer(struct wined3d_device *device,
        struct wined3d_buffer *buffer, enum wined3d_format_id format_id, unsigned int offset)
{
    enum wined3d_format_id prev_format;
    struct wined3d_buffer *prev_buffer;
    unsigned int prev_offset;

    TRACE("device %p, buffer %p, format %s, offset %u.\n",
            device, buffer, debug_d3dformat(format_id), offset);

    prev_buffer = device->update_state->index_buffer;
    prev_format = device->update_state->index_format;
    prev_offset = device->update_state->index_offset;

    device->update_state->index_buffer = buffer;
    device->update_state->index_format = format_id;
    device->update_state->index_offset = offset;

    if (device->recording)
        device->recording->changed.indices = TRUE;

    if (prev_buffer == buffer && prev_format == format_id && prev_offset == offset)
        return;

    if (buffer)
        wined3d_buffer_incref(buffer);
    if (!device->recording)
        wined3d_cs_emit_set_index_buffer(device->cs, buffer, format_id, offset);
    if (prev_buffer)
        wined3d_buffer_decref(prev_buffer);
}

struct wined3d_buffer * CDECL wined3d_device_get_index_buffer(const struct wined3d_device *device,
        enum wined3d_format_id *format, unsigned int *offset)
{
    TRACE("device %p, format %p, offset %p.\n", device, format, offset);

    *format = device->state.index_format;
    if (offset)
        *offset = device->state.index_offset;
    return device->state.index_buffer;
}

void CDECL wined3d_device_set_base_vertex_index(struct wined3d_device *device, INT base_index)
{
    TRACE("device %p, base_index %d.\n", device, base_index);

    device->update_state->base_vertex_index = base_index;
}

INT CDECL wined3d_device_get_base_vertex_index(const struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    return device->state.base_vertex_index;
}

void CDECL wined3d_device_set_viewport(struct wined3d_device *device, const struct wined3d_viewport *viewport)
{
    TRACE("device %p, viewport %p.\n", device, viewport);
    TRACE("x %.8e, y %.8e, w %.8e, h %.8e, min_z %.8e, max_z %.8e.\n",
          viewport->x, viewport->y, viewport->width, viewport->height, viewport->min_z, viewport->max_z);

    device->update_state->viewport = *viewport;

    /* Handle recording of state blocks */
    if (device->recording)
    {
        TRACE("Recording... not performing anything\n");
        device->recording->changed.viewport = TRUE;
        return;
    }

    wined3d_cs_emit_set_viewport(device->cs, viewport);
}

void CDECL wined3d_device_get_viewport(const struct wined3d_device *device, struct wined3d_viewport *viewport)
{
    TRACE("device %p, viewport %p.\n", device, viewport);

    *viewport = device->state.viewport;
}

static void resolve_depth_buffer(struct wined3d_device *device)
{
    const struct wined3d_state *state = &device->state;
    struct wined3d_rendertarget_view *src_view;
    struct wined3d_resource *dst_resource;
    struct wined3d_texture *dst_texture;

    if (!(dst_texture = state->textures[0]))
        return;
    dst_resource = &dst_texture->resource;
    if (!(dst_resource->format_flags & WINED3DFMT_FLAG_DEPTH))
        return;
    if (!(src_view = state->fb->depth_stencil))
        return;

    wined3d_device_resolve_sub_resource(device, dst_resource, 0,
            src_view->resource, src_view->sub_resource_idx, dst_resource->format->id);
}

void CDECL wined3d_device_set_blend_state(struct wined3d_device *device, struct wined3d_blend_state *blend_state)
{
    struct wined3d_blend_state *prev;

    TRACE("device %p, blend_state %p.\n", device, blend_state);

    prev = device->update_state->blend_state;
    if (prev == blend_state)
        return;

    if (blend_state)
        wined3d_blend_state_incref(blend_state);
    device->update_state->blend_state = blend_state;
    wined3d_cs_emit_set_blend_state(device->cs, blend_state);
    if (prev)
        wined3d_blend_state_decref(prev);
}

struct wined3d_blend_state * CDECL wined3d_device_get_blend_state(const struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    return device->state.blend_state;
}

void CDECL wined3d_device_set_rasterizer_state(struct wined3d_device *device,
        struct wined3d_rasterizer_state *rasterizer_state)
{
    struct wined3d_rasterizer_state *prev;

    TRACE("device %p, rasterizer_state %p.\n", device, rasterizer_state);

    prev = device->update_state->rasterizer_state;
    if (prev == rasterizer_state)
        return;

    if (rasterizer_state)
        wined3d_rasterizer_state_incref(rasterizer_state);
    device->update_state->rasterizer_state = rasterizer_state;
    wined3d_cs_emit_set_rasterizer_state(device->cs, rasterizer_state);
    if (prev)
        wined3d_rasterizer_state_decref(prev);
}

struct wined3d_rasterizer_state * CDECL wined3d_device_get_rasterizer_state(struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    return device->state.rasterizer_state;
}

void CDECL wined3d_device_set_render_state(struct wined3d_device *device,
        enum wined3d_render_state state, DWORD value)
{
    DWORD old_value;

    TRACE("device %p, state %s (%#x), value %#x.\n", device, debug_d3drenderstate(state), state, value);

    if (state > WINEHIGHEST_RENDER_STATE)
    {
        WARN("Unhandled render state %#x.\n", state);
        return;
    }

    old_value = device->state.render_states[state];
    device->update_state->render_states[state] = value;

    /* Handle recording of state blocks. */
    if (device->recording)
    {
        TRACE("Recording... not performing anything.\n");
        device->recording->changed.renderState[state >> 5] |= 1u << (state & 0x1f);
        return;
    }

    /* Compared here and not before the assignment to allow proper stateblock recording. */
    if (value == old_value)
        TRACE("Application is setting the old value over, nothing to do.\n");
    else
        wined3d_cs_emit_set_render_state(device->cs, state, value);

    if (state == WINED3D_RS_POINTSIZE && value == WINED3D_RESZ_CODE)
    {
        TRACE("RESZ multisampled depth buffer resolve triggered.\n");
        resolve_depth_buffer(device);
    }
}

DWORD CDECL wined3d_device_get_render_state(const struct wined3d_device *device, enum wined3d_render_state state)
{
    TRACE("device %p, state %s (%#x).\n", device, debug_d3drenderstate(state), state);

    return device->state.render_states[state];
}

void CDECL wined3d_device_set_sampler_state(struct wined3d_device *device,
        UINT sampler_idx, enum wined3d_sampler_state state, DWORD value)
{
    DWORD old_value;

    TRACE("device %p, sampler_idx %u, state %s, value %#x.\n",
            device, sampler_idx, debug_d3dsamplerstate(state), value);

    if (sampler_idx >= WINED3DVERTEXTEXTURESAMPLER0 && sampler_idx <= WINED3DVERTEXTEXTURESAMPLER3)
        sampler_idx -= (WINED3DVERTEXTEXTURESAMPLER0 - MAX_FRAGMENT_SAMPLERS);

    if (sampler_idx >= ARRAY_SIZE(device->state.sampler_states))
    {
        WARN("Invalid sampler %u.\n", sampler_idx);
        return; /* Windows accepts overflowing this array ... we do not. */
    }

    old_value = device->state.sampler_states[sampler_idx][state];
    device->update_state->sampler_states[sampler_idx][state] = value;

    /* Handle recording of state blocks. */
    if (device->recording)
    {
        TRACE("Recording... not performing anything.\n");
        device->recording->changed.samplerState[sampler_idx] |= 1u << state;
        return;
    }

    if (old_value == value)
    {
        TRACE("Application is setting the old value over, nothing to do.\n");
        return;
    }

    wined3d_cs_emit_set_sampler_state(device->cs, sampler_idx, state, value);
}

DWORD CDECL wined3d_device_get_sampler_state(const struct wined3d_device *device,
        UINT sampler_idx, enum wined3d_sampler_state state)
{
    TRACE("device %p, sampler_idx %u, state %s.\n",
            device, sampler_idx, debug_d3dsamplerstate(state));

    if (sampler_idx >= WINED3DVERTEXTEXTURESAMPLER0 && sampler_idx <= WINED3DVERTEXTEXTURESAMPLER3)
        sampler_idx -= (WINED3DVERTEXTEXTURESAMPLER0 - MAX_FRAGMENT_SAMPLERS);

    if (sampler_idx >= ARRAY_SIZE(device->state.sampler_states))
    {
        WARN("Invalid sampler %u.\n", sampler_idx);
        return 0; /* Windows accepts overflowing this array ... we do not. */
    }

    return device->state.sampler_states[sampler_idx][state];
}

void CDECL wined3d_device_set_scissor_rect(struct wined3d_device *device, const RECT *rect)
{
    TRACE("device %p, rect %s.\n", device, wine_dbgstr_rect(rect));

    if (device->recording)
        device->recording->changed.scissorRect = TRUE;

    if (EqualRect(&device->update_state->scissor_rect, rect))
    {
        TRACE("App is setting the old scissor rectangle over, nothing to do.\n");
        return;
    }
    CopyRect(&device->update_state->scissor_rect, rect);

    if (device->recording)
    {
        TRACE("Recording... not performing anything.\n");
        return;
    }

    wined3d_cs_emit_set_scissor_rect(device->cs, rect);
}

void CDECL wined3d_device_get_scissor_rect(const struct wined3d_device *device, RECT *rect)
{
    TRACE("device %p, rect %p.\n", device, rect);

    *rect = device->state.scissor_rect;
    TRACE("Returning rect %s.\n", wine_dbgstr_rect(rect));
}

void CDECL wined3d_device_set_vertex_declaration(struct wined3d_device *device,
        struct wined3d_vertex_declaration *declaration)
{
    struct wined3d_vertex_declaration *prev = device->update_state->vertex_declaration;

    TRACE("device %p, declaration %p.\n", device, declaration);

    if (device->recording)
        device->recording->changed.vertexDecl = TRUE;

    if (declaration == prev)
        return;

    if (declaration)
        wined3d_vertex_declaration_incref(declaration);
    device->update_state->vertex_declaration = declaration;
    if (!device->recording)
        wined3d_cs_emit_set_vertex_declaration(device->cs, declaration);
    if (prev)
        wined3d_vertex_declaration_decref(prev);
}

struct wined3d_vertex_declaration * CDECL wined3d_device_get_vertex_declaration(const struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    return device->state.vertex_declaration;
}

void CDECL wined3d_device_set_vertex_shader(struct wined3d_device *device, struct wined3d_shader *shader)
{
    struct wined3d_shader *prev = device->update_state->shader[WINED3D_SHADER_TYPE_VERTEX];

    TRACE("device %p, shader %p.\n", device, shader);

    if (device->recording)
        device->recording->changed.vertexShader = TRUE;

    if (shader == prev)
        return;

    if (shader)
        wined3d_shader_incref(shader);
    device->update_state->shader[WINED3D_SHADER_TYPE_VERTEX] = shader;
    if (!device->recording)
        wined3d_cs_emit_set_shader(device->cs, WINED3D_SHADER_TYPE_VERTEX, shader);
    if (prev)
        wined3d_shader_decref(prev);
}

struct wined3d_shader * CDECL wined3d_device_get_vertex_shader(const struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    return device->state.shader[WINED3D_SHADER_TYPE_VERTEX];
}

static void wined3d_device_set_constant_buffer(struct wined3d_device *device,
        enum wined3d_shader_type type, UINT idx, struct wined3d_buffer *buffer)
{
    struct wined3d_buffer *prev;

    if (idx >= MAX_CONSTANT_BUFFERS)
    {
        WARN("Invalid constant buffer index %u.\n", idx);
        return;
    }

    prev = device->update_state->cb[type][idx];
    if (buffer == prev)
        return;

    if (buffer)
        wined3d_buffer_incref(buffer);
    device->update_state->cb[type][idx] = buffer;
    if (!device->recording)
        wined3d_cs_emit_set_constant_buffer(device->cs, type, idx, buffer);
    if (prev)
        wined3d_buffer_decref(prev);
}

void CDECL wined3d_device_set_vs_cb(struct wined3d_device *device, UINT idx, struct wined3d_buffer *buffer)
{
    TRACE("device %p, idx %u, buffer %p.\n", device, idx, buffer);

    wined3d_device_set_constant_buffer(device, WINED3D_SHADER_TYPE_VERTEX, idx, buffer);
}

static struct wined3d_buffer *wined3d_device_get_constant_buffer(const struct wined3d_device *device,
        enum wined3d_shader_type shader_type, unsigned int idx)
{
    if (idx >= MAX_CONSTANT_BUFFERS)
    {
        WARN("Invalid constant buffer index %u.\n", idx);
        return NULL;
    }

    return device->state.cb[shader_type][idx];
}

struct wined3d_buffer * CDECL wined3d_device_get_vs_cb(const struct wined3d_device *device, UINT idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    return wined3d_device_get_constant_buffer(device, WINED3D_SHADER_TYPE_VERTEX, idx);
}

static void wined3d_device_set_shader_resource_view(struct wined3d_device *device,
        enum wined3d_shader_type type, UINT idx, struct wined3d_shader_resource_view *view)
{
    struct wined3d_shader_resource_view *prev;

    if (idx >= MAX_SHADER_RESOURCE_VIEWS)
    {
        WARN("Invalid view index %u.\n", idx);
        return;
    }

    prev = device->update_state->shader_resource_view[type][idx];
    if (view == prev)
        return;

    if (view)
        wined3d_shader_resource_view_incref(view);
    device->update_state->shader_resource_view[type][idx] = view;
    if (!device->recording)
        wined3d_cs_emit_set_shader_resource_view(device->cs, type, idx, view);
    if (prev)
        wined3d_shader_resource_view_decref(prev);
}

void CDECL wined3d_device_set_vs_resource_view(struct wined3d_device *device,
        UINT idx, struct wined3d_shader_resource_view *view)
{
    TRACE("device %p, idx %u, view %p.\n", device, idx, view);

    wined3d_device_set_shader_resource_view(device, WINED3D_SHADER_TYPE_VERTEX, idx, view);
}

static struct wined3d_shader_resource_view *wined3d_device_get_shader_resource_view(
        const struct wined3d_device *device, enum wined3d_shader_type shader_type, unsigned int idx)
{
    if (idx >= MAX_SHADER_RESOURCE_VIEWS)
    {
        WARN("Invalid view index %u.\n", idx);
        return NULL;
    }

    return device->state.shader_resource_view[shader_type][idx];
}

struct wined3d_shader_resource_view * CDECL wined3d_device_get_vs_resource_view(const struct wined3d_device *device,
        UINT idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    return wined3d_device_get_shader_resource_view(device, WINED3D_SHADER_TYPE_VERTEX, idx);
}

static void wined3d_device_set_sampler(struct wined3d_device *device,
        enum wined3d_shader_type type, UINT idx, struct wined3d_sampler *sampler)
{
    struct wined3d_sampler *prev;

    if (idx >= MAX_SAMPLER_OBJECTS)
    {
        WARN("Invalid sampler index %u.\n", idx);
        return;
    }

    prev = device->update_state->sampler[type][idx];
    if (sampler == prev)
        return;

    if (sampler)
        wined3d_sampler_incref(sampler);
    device->update_state->sampler[type][idx] = sampler;
    if (!device->recording)
        wined3d_cs_emit_set_sampler(device->cs, type, idx, sampler);
    if (prev)
        wined3d_sampler_decref(prev);
}

void CDECL wined3d_device_set_vs_sampler(struct wined3d_device *device, UINT idx, struct wined3d_sampler *sampler)
{
    TRACE("device %p, idx %u, sampler %p.\n", device, idx, sampler);

    wined3d_device_set_sampler(device, WINED3D_SHADER_TYPE_VERTEX, idx, sampler);
}

static struct wined3d_sampler *wined3d_device_get_sampler(const struct wined3d_device *device,
        enum wined3d_shader_type shader_type, unsigned int idx)
{
    if (idx >= MAX_SAMPLER_OBJECTS)
    {
        WARN("Invalid sampler index %u.\n", idx);
        return NULL;
    }

    return device->state.sampler[shader_type][idx];
}

struct wined3d_sampler * CDECL wined3d_device_get_vs_sampler(const struct wined3d_device *device, UINT idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    return wined3d_device_get_sampler(device, WINED3D_SHADER_TYPE_VERTEX, idx);
}

HRESULT CDECL wined3d_device_set_vs_consts_b(struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, const BOOL *constants)
{
    unsigned int i;

    TRACE("device %p, start_idx %u, count %u, constants %p.\n",
            device, start_idx, count, constants);

    if (!constants || start_idx >= WINED3D_MAX_CONSTS_B)
        return WINED3DERR_INVALIDCALL;

    if (count > WINED3D_MAX_CONSTS_B - start_idx)
        count = WINED3D_MAX_CONSTS_B - start_idx;
    memcpy(&device->update_state->vs_consts_b[start_idx], constants, count * sizeof(*constants));
    if (TRACE_ON(d3d))
    {
        for (i = 0; i < count; ++i)
            TRACE("Set BOOL constant %u to %#x.\n", start_idx + i, constants[i]);
    }

    if (device->recording)
    {
        for (i = start_idx; i < count + start_idx; ++i)
            device->recording->changed.vertexShaderConstantsB |= (1u << i);
    }
    else
    {
        wined3d_cs_push_constants(device->cs, WINED3D_PUSH_CONSTANTS_VS_B, start_idx, count, constants);
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_vs_consts_b(const struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, BOOL *constants)
{
    TRACE("device %p, start_idx %u, count %u, constants %p.\n",
            device, start_idx, count, constants);

    if (!constants || start_idx >= WINED3D_MAX_CONSTS_B)
        return WINED3DERR_INVALIDCALL;

    if (count > WINED3D_MAX_CONSTS_B - start_idx)
        count = WINED3D_MAX_CONSTS_B - start_idx;
    memcpy(constants, &device->state.vs_consts_b[start_idx], count * sizeof(*constants));

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_vs_consts_i(struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, const struct wined3d_ivec4 *constants)
{
    unsigned int i;

    TRACE("device %p, start_idx %u, count %u, constants %p.\n",
            device, start_idx, count, constants);

    if (!constants || start_idx >= WINED3D_MAX_CONSTS_I)
        return WINED3DERR_INVALIDCALL;

    if (count > WINED3D_MAX_CONSTS_I - start_idx)
        count = WINED3D_MAX_CONSTS_I - start_idx;
    memcpy(&device->update_state->vs_consts_i[start_idx], constants, count * sizeof(*constants));
    if (TRACE_ON(d3d))
    {
        for (i = 0; i < count; ++i)
            TRACE("Set ivec4 constant %u to %s.\n", start_idx + i, debug_ivec4(&constants[i]));
    }

    if (device->recording)
    {
        for (i = start_idx; i < count + start_idx; ++i)
            device->recording->changed.vertexShaderConstantsI |= (1u << i);
    }
    else
    {
        wined3d_cs_push_constants(device->cs, WINED3D_PUSH_CONSTANTS_VS_I, start_idx, count, constants);
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_vs_consts_i(const struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, struct wined3d_ivec4 *constants)
{
    TRACE("device %p, start_idx %u, count %u, constants %p.\n",
            device, start_idx, count, constants);

    if (!constants || start_idx >= WINED3D_MAX_CONSTS_I)
        return WINED3DERR_INVALIDCALL;

    if (count > WINED3D_MAX_CONSTS_I - start_idx)
        count = WINED3D_MAX_CONSTS_I - start_idx;
    memcpy(constants, &device->state.vs_consts_i[start_idx], count * sizeof(*constants));
    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_vs_consts_f(struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, const struct wined3d_vec4 *constants)
{
    const struct wined3d_d3d_info *d3d_info = &device->adapter->d3d_info;
    unsigned int i;

    TRACE("device %p, start_idx %u, count %u, constants %p.\n",
            device, start_idx, count, constants);

    if (!constants || start_idx >= d3d_info->limits.vs_uniform_count
            || count > d3d_info->limits.vs_uniform_count - start_idx)
        return WINED3DERR_INVALIDCALL;

    memcpy(&device->update_state->vs_consts_f[start_idx], constants, count * sizeof(*constants));
    if (TRACE_ON(d3d))
    {
        for (i = 0; i < count; ++i)
            TRACE("Set vec4 constant %u to %s.\n", start_idx + i, debug_vec4(&constants[i]));
    }

    if (device->recording)
        memset(&device->recording->changed.vs_consts_f[start_idx], 1,
                count * sizeof(*device->recording->changed.vs_consts_f));
    else
        wined3d_cs_push_constants(device->cs, WINED3D_PUSH_CONSTANTS_VS_F, start_idx, count, constants);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_vs_consts_f(const struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, struct wined3d_vec4 *constants)
{
    const struct wined3d_d3d_info *d3d_info = &device->adapter->d3d_info;

    TRACE("device %p, start_idx %u, count %u, constants %p.\n",
            device, start_idx, count, constants);

    if (!constants || start_idx >= d3d_info->limits.vs_uniform_count
            || count > d3d_info->limits.vs_uniform_count - start_idx)
        return WINED3DERR_INVALIDCALL;

    memcpy(constants, &device->state.vs_consts_f[start_idx], count * sizeof(*constants));

    return WINED3D_OK;
}

void CDECL wined3d_device_set_pixel_shader(struct wined3d_device *device, struct wined3d_shader *shader)
{
    struct wined3d_shader *prev = device->update_state->shader[WINED3D_SHADER_TYPE_PIXEL];

    TRACE("device %p, shader %p.\n", device, shader);

    if (device->recording)
        device->recording->changed.pixelShader = TRUE;

    if (shader == prev)
        return;

    if (shader)
        wined3d_shader_incref(shader);
    device->update_state->shader[WINED3D_SHADER_TYPE_PIXEL] = shader;
    if (!device->recording)
        wined3d_cs_emit_set_shader(device->cs, WINED3D_SHADER_TYPE_PIXEL, shader);
    if (prev)
        wined3d_shader_decref(prev);
}

struct wined3d_shader * CDECL wined3d_device_get_pixel_shader(const struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    return device->state.shader[WINED3D_SHADER_TYPE_PIXEL];
}

void CDECL wined3d_device_set_ps_cb(struct wined3d_device *device, UINT idx, struct wined3d_buffer *buffer)
{
    TRACE("device %p, idx %u, buffer %p.\n", device, idx, buffer);

    wined3d_device_set_constant_buffer(device, WINED3D_SHADER_TYPE_PIXEL, idx, buffer);
}

struct wined3d_buffer * CDECL wined3d_device_get_ps_cb(const struct wined3d_device *device, UINT idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    return wined3d_device_get_constant_buffer(device, WINED3D_SHADER_TYPE_PIXEL, idx);
}

void CDECL wined3d_device_set_ps_resource_view(struct wined3d_device *device,
        UINT idx, struct wined3d_shader_resource_view *view)
{
    TRACE("device %p, idx %u, view %p.\n", device, idx, view);

    wined3d_device_set_shader_resource_view(device, WINED3D_SHADER_TYPE_PIXEL, idx, view);
}

struct wined3d_shader_resource_view * CDECL wined3d_device_get_ps_resource_view(const struct wined3d_device *device,
        UINT idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    return wined3d_device_get_shader_resource_view(device, WINED3D_SHADER_TYPE_PIXEL, idx);
}

void CDECL wined3d_device_set_ps_sampler(struct wined3d_device *device, UINT idx, struct wined3d_sampler *sampler)
{
    TRACE("device %p, idx %u, sampler %p.\n", device, idx, sampler);

    wined3d_device_set_sampler(device, WINED3D_SHADER_TYPE_PIXEL, idx, sampler);
}

struct wined3d_sampler * CDECL wined3d_device_get_ps_sampler(const struct wined3d_device *device, UINT idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    return wined3d_device_get_sampler(device, WINED3D_SHADER_TYPE_PIXEL, idx);
}

HRESULT CDECL wined3d_device_set_ps_consts_b(struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, const BOOL *constants)
{
    unsigned int i;

    TRACE("device %p, start_idx %u, count %u, constants %p.\n",
            device, start_idx, count, constants);

    if (!constants || start_idx >= WINED3D_MAX_CONSTS_B)
        return WINED3DERR_INVALIDCALL;

    if (count > WINED3D_MAX_CONSTS_B - start_idx)
        count = WINED3D_MAX_CONSTS_B - start_idx;
    memcpy(&device->update_state->ps_consts_b[start_idx], constants, count * sizeof(*constants));
    if (TRACE_ON(d3d))
    {
        for (i = 0; i < count; ++i)
            TRACE("Set BOOL constant %u to %#x.\n", start_idx + i, constants[i]);
    }

    if (device->recording)
    {
        for (i = start_idx; i < count + start_idx; ++i)
            device->recording->changed.pixelShaderConstantsB |= (1u << i);
    }
    else
    {
        wined3d_cs_push_constants(device->cs, WINED3D_PUSH_CONSTANTS_PS_B, start_idx, count, constants);
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_ps_consts_b(const struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, BOOL *constants)
{
    TRACE("device %p, start_idx %u, count %u,constants %p.\n",
            device, start_idx, count, constants);

    if (!constants || start_idx >= WINED3D_MAX_CONSTS_B)
        return WINED3DERR_INVALIDCALL;

    if (count > WINED3D_MAX_CONSTS_B - start_idx)
        count = WINED3D_MAX_CONSTS_B - start_idx;
    memcpy(constants, &device->state.ps_consts_b[start_idx], count * sizeof(*constants));

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_ps_consts_i(struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, const struct wined3d_ivec4 *constants)
{
    unsigned int i;

    TRACE("device %p, start_idx %u, count %u, constants %p.\n",
            device, start_idx, count, constants);

    if (!constants || start_idx >= WINED3D_MAX_CONSTS_I)
        return WINED3DERR_INVALIDCALL;

    if (count > WINED3D_MAX_CONSTS_I - start_idx)
        count = WINED3D_MAX_CONSTS_I - start_idx;
    memcpy(&device->update_state->ps_consts_i[start_idx], constants, count * sizeof(*constants));
    if (TRACE_ON(d3d))
    {
        for (i = 0; i < count; ++i)
            TRACE("Set ivec4 constant %u to %s.\n", start_idx + i, debug_ivec4(&constants[i]));
    }

    if (device->recording)
    {
        for (i = start_idx; i < count + start_idx; ++i)
            device->recording->changed.pixelShaderConstantsI |= (1u << i);
    }
    else
    {
        wined3d_cs_push_constants(device->cs, WINED3D_PUSH_CONSTANTS_PS_I, start_idx, count, constants);
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_ps_consts_i(const struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, struct wined3d_ivec4 *constants)
{
    TRACE("device %p, start_idx %u, count %u, constants %p.\n",
            device, start_idx, count, constants);

    if (!constants || start_idx >= WINED3D_MAX_CONSTS_I)
        return WINED3DERR_INVALIDCALL;

    if (count > WINED3D_MAX_CONSTS_I - start_idx)
        count = WINED3D_MAX_CONSTS_I - start_idx;
    memcpy(constants, &device->state.ps_consts_i[start_idx], count * sizeof(*constants));

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_ps_consts_f(struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, const struct wined3d_vec4 *constants)
{
    const struct wined3d_d3d_info *d3d_info = &device->adapter->d3d_info;
    unsigned int i;

    TRACE("device %p, start_idx %u, count %u, constants %p.\n",
            device, start_idx, count, constants);

    if (!constants || start_idx >= d3d_info->limits.ps_uniform_count
            || count > d3d_info->limits.ps_uniform_count - start_idx)
        return WINED3DERR_INVALIDCALL;

    memcpy(&device->update_state->ps_consts_f[start_idx], constants, count * sizeof(*constants));
    if (TRACE_ON(d3d))
    {
        for (i = 0; i < count; ++i)
            TRACE("Set vec4 constant %u to %s.\n", start_idx + i, debug_vec4(&constants[i]));
    }

    if (device->recording)
        memset(&device->recording->changed.ps_consts_f[start_idx], 1,
                count * sizeof(*device->recording->changed.ps_consts_f));
    else
        wined3d_cs_push_constants(device->cs, WINED3D_PUSH_CONSTANTS_PS_F, start_idx, count, constants);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_ps_consts_f(const struct wined3d_device *device,
        unsigned int start_idx, unsigned int count, struct wined3d_vec4 *constants)
{
    const struct wined3d_d3d_info *d3d_info = &device->adapter->d3d_info;

    TRACE("device %p, start_idx %u, count %u, constants %p.\n",
            device, start_idx, count, constants);

    if (!constants || start_idx >= d3d_info->limits.ps_uniform_count
            || count > d3d_info->limits.ps_uniform_count - start_idx)
        return WINED3DERR_INVALIDCALL;

    memcpy(constants, &device->state.ps_consts_f[start_idx], count * sizeof(*constants));

    return WINED3D_OK;
}

void CDECL wined3d_device_set_hull_shader(struct wined3d_device *device, struct wined3d_shader *shader)
{
    struct wined3d_shader *prev;

    TRACE("device %p, shader %p.\n", device, shader);

    prev = device->update_state->shader[WINED3D_SHADER_TYPE_HULL];
    if (shader == prev)
        return;
    if (shader)
        wined3d_shader_incref(shader);
    device->update_state->shader[WINED3D_SHADER_TYPE_HULL] = shader;
    wined3d_cs_emit_set_shader(device->cs, WINED3D_SHADER_TYPE_HULL, shader);
    if (prev)
        wined3d_shader_decref(prev);
}

struct wined3d_shader * CDECL wined3d_device_get_hull_shader(const struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    return device->state.shader[WINED3D_SHADER_TYPE_HULL];
}

void CDECL wined3d_device_set_hs_cb(struct wined3d_device *device, unsigned int idx, struct wined3d_buffer *buffer)
{
    TRACE("device %p, idx %u, buffer %p.\n", device, idx, buffer);

    wined3d_device_set_constant_buffer(device, WINED3D_SHADER_TYPE_HULL, idx, buffer);
}

struct wined3d_buffer * CDECL wined3d_device_get_hs_cb(const struct wined3d_device *device, unsigned int idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    return wined3d_device_get_constant_buffer(device, WINED3D_SHADER_TYPE_HULL, idx);
}

void CDECL wined3d_device_set_hs_resource_view(struct wined3d_device *device,
        unsigned int idx, struct wined3d_shader_resource_view *view)
{
    TRACE("device %p, idx %u, view %p.\n", device, idx, view);

    wined3d_device_set_shader_resource_view(device, WINED3D_SHADER_TYPE_HULL, idx, view);
}

struct wined3d_shader_resource_view * CDECL wined3d_device_get_hs_resource_view(const struct wined3d_device *device,
        unsigned int idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    return wined3d_device_get_shader_resource_view(device, WINED3D_SHADER_TYPE_HULL, idx);
}

void CDECL wined3d_device_set_hs_sampler(struct wined3d_device *device,
        unsigned int idx, struct wined3d_sampler *sampler)
{
    TRACE("device %p, idx %u, sampler %p.\n", device, idx, sampler);

    wined3d_device_set_sampler(device, WINED3D_SHADER_TYPE_HULL, idx, sampler);
}

struct wined3d_sampler * CDECL wined3d_device_get_hs_sampler(const struct wined3d_device *device, unsigned int idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    return wined3d_device_get_sampler(device, WINED3D_SHADER_TYPE_HULL, idx);
}

void CDECL wined3d_device_set_domain_shader(struct wined3d_device *device, struct wined3d_shader *shader)
{
    struct wined3d_shader *prev;

    TRACE("device %p, shader %p.\n", device, shader);

    prev = device->update_state->shader[WINED3D_SHADER_TYPE_DOMAIN];
    if (shader == prev)
        return;
    if (shader)
        wined3d_shader_incref(shader);
    device->update_state->shader[WINED3D_SHADER_TYPE_DOMAIN] = shader;
    wined3d_cs_emit_set_shader(device->cs, WINED3D_SHADER_TYPE_DOMAIN, shader);
    if (prev)
        wined3d_shader_decref(prev);
}

struct wined3d_shader * CDECL wined3d_device_get_domain_shader(const struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    return device->state.shader[WINED3D_SHADER_TYPE_DOMAIN];
}

void CDECL wined3d_device_set_ds_cb(struct wined3d_device *device, unsigned int idx, struct wined3d_buffer *buffer)
{
    TRACE("device %p, idx %u, buffer %p.\n", device, idx, buffer);

    wined3d_device_set_constant_buffer(device, WINED3D_SHADER_TYPE_DOMAIN, idx, buffer);
}

struct wined3d_buffer * CDECL wined3d_device_get_ds_cb(const struct wined3d_device *device, unsigned int idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    return wined3d_device_get_constant_buffer(device, WINED3D_SHADER_TYPE_DOMAIN, idx);
}

void CDECL wined3d_device_set_ds_resource_view(struct wined3d_device *device,
        unsigned int idx, struct wined3d_shader_resource_view *view)
{
    TRACE("device %p, idx %u, view %p.\n", device, idx, view);

    wined3d_device_set_shader_resource_view(device, WINED3D_SHADER_TYPE_DOMAIN, idx, view);
}

struct wined3d_shader_resource_view * CDECL wined3d_device_get_ds_resource_view(const struct wined3d_device *device,
        unsigned int idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    return wined3d_device_get_shader_resource_view(device, WINED3D_SHADER_TYPE_DOMAIN, idx);
}

void CDECL wined3d_device_set_ds_sampler(struct wined3d_device *device,
        unsigned int idx, struct wined3d_sampler *sampler)
{
    TRACE("device %p, idx %u, sampler %p.\n", device, idx, sampler);

    wined3d_device_set_sampler(device, WINED3D_SHADER_TYPE_DOMAIN, idx, sampler);
}

struct wined3d_sampler * CDECL wined3d_device_get_ds_sampler(const struct wined3d_device *device, unsigned int idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    return wined3d_device_get_sampler(device, WINED3D_SHADER_TYPE_DOMAIN, idx);
}

void CDECL wined3d_device_set_geometry_shader(struct wined3d_device *device, struct wined3d_shader *shader)
{
    struct wined3d_shader *prev = device->update_state->shader[WINED3D_SHADER_TYPE_GEOMETRY];

    TRACE("device %p, shader %p.\n", device, shader);

    if (device->recording || shader == prev)
        return;
    if (shader)
        wined3d_shader_incref(shader);
    device->update_state->shader[WINED3D_SHADER_TYPE_GEOMETRY] = shader;
    wined3d_cs_emit_set_shader(device->cs, WINED3D_SHADER_TYPE_GEOMETRY, shader);
    if (prev)
        wined3d_shader_decref(prev);
}

struct wined3d_shader * CDECL wined3d_device_get_geometry_shader(const struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    return device->state.shader[WINED3D_SHADER_TYPE_GEOMETRY];
}

void CDECL wined3d_device_set_gs_cb(struct wined3d_device *device, UINT idx, struct wined3d_buffer *buffer)
{
    TRACE("device %p, idx %u, buffer %p.\n", device, idx, buffer);

    wined3d_device_set_constant_buffer(device, WINED3D_SHADER_TYPE_GEOMETRY, idx, buffer);
}

struct wined3d_buffer * CDECL wined3d_device_get_gs_cb(const struct wined3d_device *device, UINT idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    return wined3d_device_get_constant_buffer(device, WINED3D_SHADER_TYPE_GEOMETRY, idx);
}

void CDECL wined3d_device_set_gs_resource_view(struct wined3d_device *device,
        UINT idx, struct wined3d_shader_resource_view *view)
{
    TRACE("device %p, idx %u, view %p.\n", device, idx, view);

    wined3d_device_set_shader_resource_view(device, WINED3D_SHADER_TYPE_GEOMETRY, idx, view);
}

struct wined3d_shader_resource_view * CDECL wined3d_device_get_gs_resource_view(const struct wined3d_device *device,
        UINT idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    return wined3d_device_get_shader_resource_view(device, WINED3D_SHADER_TYPE_GEOMETRY, idx);
}

void CDECL wined3d_device_set_gs_sampler(struct wined3d_device *device, UINT idx, struct wined3d_sampler *sampler)
{
    TRACE("device %p, idx %u, sampler %p.\n", device, idx, sampler);

    wined3d_device_set_sampler(device, WINED3D_SHADER_TYPE_GEOMETRY, idx, sampler);
}

struct wined3d_sampler * CDECL wined3d_device_get_gs_sampler(const struct wined3d_device *device, UINT idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    return wined3d_device_get_sampler(device, WINED3D_SHADER_TYPE_GEOMETRY, idx);
}

void CDECL wined3d_device_set_compute_shader(struct wined3d_device *device, struct wined3d_shader *shader)
{
    struct wined3d_shader *prev;

    TRACE("device %p, shader %p.\n", device, shader);

    prev = device->update_state->shader[WINED3D_SHADER_TYPE_COMPUTE];
    if (device->recording || shader == prev)
        return;
    if (shader)
        wined3d_shader_incref(shader);
    device->update_state->shader[WINED3D_SHADER_TYPE_COMPUTE] = shader;
    wined3d_cs_emit_set_shader(device->cs, WINED3D_SHADER_TYPE_COMPUTE, shader);
    if (prev)
        wined3d_shader_decref(prev);
}

struct wined3d_shader * CDECL wined3d_device_get_compute_shader(const struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    return device->state.shader[WINED3D_SHADER_TYPE_COMPUTE];
}

void CDECL wined3d_device_set_cs_cb(struct wined3d_device *device, unsigned int idx, struct wined3d_buffer *buffer)
{
    TRACE("device %p, idx %u, buffer %p.\n", device, idx, buffer);

    wined3d_device_set_constant_buffer(device, WINED3D_SHADER_TYPE_COMPUTE, idx, buffer);
}

struct wined3d_buffer * CDECL wined3d_device_get_cs_cb(const struct wined3d_device *device, unsigned int idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    return wined3d_device_get_constant_buffer(device, WINED3D_SHADER_TYPE_COMPUTE, idx);
}

void CDECL wined3d_device_set_cs_resource_view(struct wined3d_device *device,
        unsigned int idx, struct wined3d_shader_resource_view *view)
{
    TRACE("device %p, idx %u, view %p.\n", device, idx, view);

    wined3d_device_set_shader_resource_view(device, WINED3D_SHADER_TYPE_COMPUTE, idx, view);
}

struct wined3d_shader_resource_view * CDECL wined3d_device_get_cs_resource_view(const struct wined3d_device *device,
        unsigned int idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    return wined3d_device_get_shader_resource_view(device, WINED3D_SHADER_TYPE_COMPUTE, idx);
}

void CDECL wined3d_device_set_cs_sampler(struct wined3d_device *device,
        unsigned int idx, struct wined3d_sampler *sampler)
{
    TRACE("device %p, idx %u, sampler %p.\n", device, idx, sampler);

    wined3d_device_set_sampler(device, WINED3D_SHADER_TYPE_COMPUTE, idx, sampler);
}

struct wined3d_sampler * CDECL wined3d_device_get_cs_sampler(const struct wined3d_device *device, unsigned int idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    return wined3d_device_get_sampler(device, WINED3D_SHADER_TYPE_COMPUTE, idx);
}

static void wined3d_device_set_pipeline_unordered_access_view(struct wined3d_device *device,
        enum wined3d_pipeline pipeline, unsigned int idx, struct wined3d_unordered_access_view *uav,
        unsigned int initial_count)
{
    struct wined3d_unordered_access_view *prev;

    if (idx >= MAX_UNORDERED_ACCESS_VIEWS)
    {
        WARN("Invalid UAV index %u.\n", idx);
        return;
    }

    prev = device->update_state->unordered_access_view[pipeline][idx];
    if (uav == prev && initial_count == ~0u)
        return;

    if (uav)
        wined3d_unordered_access_view_incref(uav);
    device->update_state->unordered_access_view[pipeline][idx] = uav;
    if (!device->recording)
        wined3d_cs_emit_set_unordered_access_view(device->cs, pipeline, idx, uav, initial_count);
    if (prev)
        wined3d_unordered_access_view_decref(prev);
}

static struct wined3d_unordered_access_view *wined3d_device_get_pipeline_unordered_access_view(
        const struct wined3d_device *device, enum wined3d_pipeline pipeline, unsigned int idx)
{
    if (idx >= MAX_UNORDERED_ACCESS_VIEWS)
    {
        WARN("Invalid UAV index %u.\n", idx);
        return NULL;
    }

    return device->state.unordered_access_view[pipeline][idx];
}

void CDECL wined3d_device_set_cs_uav(struct wined3d_device *device, unsigned int idx,
        struct wined3d_unordered_access_view *uav, unsigned int initial_count)
{
    TRACE("device %p, idx %u, uav %p, initial_count %#x.\n", device, idx, uav, initial_count);

    wined3d_device_set_pipeline_unordered_access_view(device, WINED3D_PIPELINE_COMPUTE, idx, uav, initial_count);
}

struct wined3d_unordered_access_view * CDECL wined3d_device_get_cs_uav(const struct wined3d_device *device,
        unsigned int idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    return wined3d_device_get_pipeline_unordered_access_view(device, WINED3D_PIPELINE_COMPUTE, idx);
}

void CDECL wined3d_device_set_unordered_access_view(struct wined3d_device *device,
        unsigned int idx, struct wined3d_unordered_access_view *uav, unsigned int initial_count)
{
    TRACE("device %p, idx %u, uav %p, initial_count %#x.\n", device, idx, uav, initial_count);

    wined3d_device_set_pipeline_unordered_access_view(device, WINED3D_PIPELINE_GRAPHICS, idx, uav, initial_count);
}

struct wined3d_unordered_access_view * CDECL wined3d_device_get_unordered_access_view(
        const struct wined3d_device *device, unsigned int idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    return wined3d_device_get_pipeline_unordered_access_view(device, WINED3D_PIPELINE_GRAPHICS, idx);
}

/* Context activation is done by the caller. */
#define copy_and_next(dest, src, size) memcpy(dest, src, size); dest += (size)
static HRESULT process_vertices_strided(const struct wined3d_device *device, DWORD dwDestIndex, DWORD dwCount,
        const struct wined3d_stream_info *stream_info, struct wined3d_buffer *dest, DWORD flags,
        DWORD DestFVF)
{
    struct wined3d_matrix mat, proj_mat, view_mat, world_mat;
    struct wined3d_map_desc map_desc;
    struct wined3d_box box = {0};
    struct wined3d_viewport vp;
    UINT vertex_size;
    unsigned int i;
    BYTE *dest_ptr;
    BOOL doClip;
    DWORD numTextures;
    HRESULT hr;

    if (stream_info->use_map & (1u << WINED3D_FFP_NORMAL))
    {
        WARN(" lighting state not saved yet... Some strange stuff may happen !\n");
    }

    if (!(stream_info->use_map & (1u << WINED3D_FFP_POSITION)))
    {
        ERR("Source has no position mask\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (device->state.render_states[WINED3D_RS_CLIPPING])
    {
        static BOOL warned = FALSE;
        /*
         * The clipping code is not quite correct. Some things need
         * to be checked against IDirect3DDevice3 (!), d3d8 and d3d9,
         * so disable clipping for now.
         * (The graphics in Half-Life are broken, and my processvertices
         *  test crashes with IDirect3DDevice3)
        doClip = TRUE;
         */
        doClip = FALSE;
        if(!warned) {
           warned = TRUE;
           FIXME("Clipping is broken and disabled for now\n");
        }
    }
    else
        doClip = FALSE;

    vertex_size = get_flexible_vertex_size(DestFVF);
    box.left = dwDestIndex * vertex_size;
    box.right = box.left + dwCount * vertex_size;
    if (FAILED(hr = wined3d_resource_map(&dest->resource, 0, &map_desc, &box, WINED3D_MAP_WRITE)))
    {
        WARN("Failed to map buffer, hr %#x.\n", hr);
        return hr;
    }
    dest_ptr = map_desc.data;

    wined3d_device_get_transform(device, WINED3D_TS_VIEW, &view_mat);
    wined3d_device_get_transform(device, WINED3D_TS_PROJECTION, &proj_mat);
    wined3d_device_get_transform(device, WINED3D_TS_WORLD_MATRIX(0), &world_mat);

    TRACE("View mat:\n");
    TRACE("%.8e %.8e %.8e %.8e\n", view_mat._11, view_mat._12, view_mat._13, view_mat._14);
    TRACE("%.8e %.8e %.8e %.8e\n", view_mat._21, view_mat._22, view_mat._23, view_mat._24);
    TRACE("%.8e %.8e %.8e %.8e\n", view_mat._31, view_mat._32, view_mat._33, view_mat._34);
    TRACE("%.8e %.8e %.8e %.8e\n", view_mat._41, view_mat._42, view_mat._43, view_mat._44);

    TRACE("Proj mat:\n");
    TRACE("%.8e %.8e %.8e %.8e\n", proj_mat._11, proj_mat._12, proj_mat._13, proj_mat._14);
    TRACE("%.8e %.8e %.8e %.8e\n", proj_mat._21, proj_mat._22, proj_mat._23, proj_mat._24);
    TRACE("%.8e %.8e %.8e %.8e\n", proj_mat._31, proj_mat._32, proj_mat._33, proj_mat._34);
    TRACE("%.8e %.8e %.8e %.8e\n", proj_mat._41, proj_mat._42, proj_mat._43, proj_mat._44);

    TRACE("World mat:\n");
    TRACE("%.8e %.8e %.8e %.8e\n", world_mat._11, world_mat._12, world_mat._13, world_mat._14);
    TRACE("%.8e %.8e %.8e %.8e\n", world_mat._21, world_mat._22, world_mat._23, world_mat._24);
    TRACE("%.8e %.8e %.8e %.8e\n", world_mat._31, world_mat._32, world_mat._33, world_mat._34);
    TRACE("%.8e %.8e %.8e %.8e\n", world_mat._41, world_mat._42, world_mat._43, world_mat._44);

    /* Get the viewport */
    wined3d_device_get_viewport(device, &vp);
    TRACE("viewport  x %.8e, y %.8e, width %.8e, height %.8e, min_z %.8e, max_z %.8e.\n",
          vp.x, vp.y, vp.width, vp.height, vp.min_z, vp.max_z);

    multiply_matrix(&mat,&view_mat,&world_mat);
    multiply_matrix(&mat,&proj_mat,&mat);

    numTextures = (DestFVF & WINED3DFVF_TEXCOUNT_MASK) >> WINED3DFVF_TEXCOUNT_SHIFT;

    for (i = 0; i < dwCount; i+= 1) {
        unsigned int tex_index;

        if ( ((DestFVF & WINED3DFVF_POSITION_MASK) == WINED3DFVF_XYZ ) ||
             ((DestFVF & WINED3DFVF_POSITION_MASK) == WINED3DFVF_XYZRHW ) ) {
            /* The position first */
            const struct wined3d_stream_info_element *element = &stream_info->elements[WINED3D_FFP_POSITION];
            const float *p = (const float *)(element->data.addr + i * element->stride);
            float x, y, z, rhw;
            TRACE("In: ( %06.2f %06.2f %06.2f )\n", p[0], p[1], p[2]);

            /* Multiplication with world, view and projection matrix. */
            x   = (p[0] * mat._11) + (p[1] * mat._21) + (p[2] * mat._31) + mat._41;
            y   = (p[0] * mat._12) + (p[1] * mat._22) + (p[2] * mat._32) + mat._42;
            z   = (p[0] * mat._13) + (p[1] * mat._23) + (p[2] * mat._33) + mat._43;
            rhw = (p[0] * mat._14) + (p[1] * mat._24) + (p[2] * mat._34) + mat._44;

            TRACE("x=%f y=%f z=%f rhw=%f\n", x, y, z, rhw);

            /* WARNING: The following things are taken from d3d7 and were not yet checked
             * against d3d8 or d3d9!
             */

            /* Clipping conditions: From msdn
             *
             * A vertex is clipped if it does not match the following requirements
             * -rhw < x <= rhw
             * -rhw < y <= rhw
             *    0 < z <= rhw
             *    0 < rhw ( Not in d3d7, but tested in d3d7)
             *
             * If clipping is on is determined by the D3DVOP_CLIP flag in D3D7, and
             * by the D3DRS_CLIPPING in D3D9(according to the msdn, not checked)
             *
             */

            if( !doClip ||
                ( (-rhw -eps < x) && (-rhw -eps < y) && ( -eps < z) &&
                  (x <= rhw + eps) && (y <= rhw + eps ) && (z <= rhw + eps) &&
                  ( rhw > eps ) ) ) {

                /* "Normal" viewport transformation (not clipped)
                 * 1) The values are divided by rhw
                 * 2) The y axis is negative, so multiply it with -1
                 * 3) Screen coordinates go from -(Width/2) to +(Width/2) and
                 *    -(Height/2) to +(Height/2). The z range is MinZ to MaxZ
                 * 4) Multiply x with Width/2 and add Width/2
                 * 5) The same for the height
                 * 6) Add the viewpoint X and Y to the 2D coordinates and
                 *    The minimum Z value to z
                 * 7) rhw = 1 / rhw Reciprocal of Homogeneous W....
                 *
                 * Well, basically it's simply a linear transformation into viewport
                 * coordinates
                 */

                x /= rhw;
                y /= rhw;
                z /= rhw;

                y *= -1;

                x *= vp.width / 2;
                y *= vp.height / 2;
                z *= vp.max_z - vp.min_z;

                x += vp.width / 2 + vp.x;
                y += vp.height / 2 + vp.y;
                z += vp.min_z;

                rhw = 1 / rhw;
            } else {
                /* That vertex got clipped
                 * Contrary to OpenGL it is not dropped completely, it just
                 * undergoes a different calculation.
                 */
                TRACE("Vertex got clipped\n");
                x += rhw;
                y += rhw;

                x  /= 2;
                y  /= 2;

                /* Msdn mentions that Direct3D9 keeps a list of clipped vertices
                 * outside of the main vertex buffer memory. That needs some more
                 * investigation...
                 */
            }

            TRACE("Writing (%f %f %f) %f\n", x, y, z, rhw);


            ( (float *) dest_ptr)[0] = x;
            ( (float *) dest_ptr)[1] = y;
            ( (float *) dest_ptr)[2] = z;
            ( (float *) dest_ptr)[3] = rhw; /* SIC, see ddraw test! */

            dest_ptr += 3 * sizeof(float);

            if ((DestFVF & WINED3DFVF_POSITION_MASK) == WINED3DFVF_XYZRHW)
                dest_ptr += sizeof(float);
        }

        if (DestFVF & WINED3DFVF_PSIZE)
            dest_ptr += sizeof(DWORD);

        if (DestFVF & WINED3DFVF_NORMAL)
        {
            const struct wined3d_stream_info_element *element = &stream_info->elements[WINED3D_FFP_NORMAL];
            const float *normal = (const float *)(element->data.addr + i * element->stride);
            /* AFAIK this should go into the lighting information */
            FIXME("Didn't expect the destination to have a normal\n");
            copy_and_next(dest_ptr, normal, 3 * sizeof(float));
        }

        if (DestFVF & WINED3DFVF_DIFFUSE)
        {
            const struct wined3d_stream_info_element *element = &stream_info->elements[WINED3D_FFP_DIFFUSE];
            const DWORD *color_d = (const DWORD *)(element->data.addr + i * element->stride);
            if (!(stream_info->use_map & (1u << WINED3D_FFP_DIFFUSE)))
            {
                static BOOL warned = FALSE;

                if(!warned) {
                    ERR("No diffuse color in source, but destination has one\n");
                    warned = TRUE;
                }

                *( (DWORD *) dest_ptr) = 0xffffffff;
                dest_ptr += sizeof(DWORD);
            }
            else
            {
                copy_and_next(dest_ptr, color_d, sizeof(DWORD));
            }
        }

        if (DestFVF & WINED3DFVF_SPECULAR)
        {
            /* What's the color value in the feedback buffer? */
            const struct wined3d_stream_info_element *element = &stream_info->elements[WINED3D_FFP_SPECULAR];
            const DWORD *color_s = (const DWORD *)(element->data.addr + i * element->stride);
            if (!(stream_info->use_map & (1u << WINED3D_FFP_SPECULAR)))
            {
                static BOOL warned = FALSE;

                if(!warned) {
                    ERR("No specular color in source, but destination has one\n");
                    warned = TRUE;
                }

                *(DWORD *)dest_ptr = 0xff000000;
                dest_ptr += sizeof(DWORD);
            }
            else
            {
                copy_and_next(dest_ptr, color_s, sizeof(DWORD));
            }
        }

        for (tex_index = 0; tex_index < numTextures; ++tex_index)
        {
            const struct wined3d_stream_info_element *element = &stream_info->elements[WINED3D_FFP_TEXCOORD0 + tex_index];
            const float *tex_coord = (const float *)(element->data.addr + i * element->stride);
            if (!(stream_info->use_map & (1u << (WINED3D_FFP_TEXCOORD0 + tex_index))))
            {
                ERR("No source texture, but destination requests one\n");
                dest_ptr += GET_TEXCOORD_SIZE_FROM_FVF(DestFVF, tex_index) * sizeof(float);
            }
            else
            {
                copy_and_next(dest_ptr, tex_coord, GET_TEXCOORD_SIZE_FROM_FVF(DestFVF, tex_index) * sizeof(float));
            }
        }
    }

    wined3d_resource_unmap(&dest->resource, 0);

    return WINED3D_OK;
}
#undef copy_and_next

HRESULT CDECL wined3d_device_process_vertices(struct wined3d_device *device,
        UINT src_start_idx, UINT dst_idx, UINT vertex_count, struct wined3d_buffer *dst_buffer,
        const struct wined3d_vertex_declaration *declaration, DWORD flags, DWORD dst_fvf)
{
    struct wined3d_state *state = &device->state;
    struct wined3d_stream_info stream_info;
    struct wined3d_resource *resource;
    struct wined3d_box box = {0};
    struct wined3d_shader *vs;
    unsigned int i;
    HRESULT hr;
    WORD map;

    TRACE("device %p, src_start_idx %u, dst_idx %u, vertex_count %u, "
            "dst_buffer %p, declaration %p, flags %#x, dst_fvf %#x.\n",
            device, src_start_idx, dst_idx, vertex_count,
            dst_buffer, declaration, flags, dst_fvf);

    if (declaration)
        FIXME("Output vertex declaration not implemented yet.\n");

    vs = state->shader[WINED3D_SHADER_TYPE_VERTEX];
    state->shader[WINED3D_SHADER_TYPE_VERTEX] = NULL;
    wined3d_stream_info_from_declaration(&stream_info, state, &device->adapter->gl_info, &device->adapter->d3d_info);
    state->shader[WINED3D_SHADER_TYPE_VERTEX] = vs;

    /* We can't convert FROM a VBO, and vertex buffers used to source into
     * process_vertices() are unlikely to ever be used for drawing. Release
     * VBOs in those buffers and fix up the stream_info structure.
     *
     * Also apply the start index. */
    for (i = 0, map = stream_info.use_map; map; map >>= 1, ++i)
    {
        struct wined3d_stream_info_element *e;
        struct wined3d_map_desc map_desc;

        if (!(map & 1))
            continue;

        e = &stream_info.elements[i];
        resource = &state->streams[e->stream_idx].buffer->resource;
        box.left = src_start_idx * e->stride;
        box.right = box.left + vertex_count * e->stride;
        if (FAILED(wined3d_resource_map(resource, 0, &map_desc, &box, WINED3D_MAP_READ)))
            ERR("Failed to map resource.\n");
        e->data.buffer_object = 0;
        e->data.addr += (ULONG_PTR)map_desc.data;
    }

    hr = process_vertices_strided(device, dst_idx, vertex_count,
            &stream_info, dst_buffer, flags, dst_fvf);

    for (i = 0, map = stream_info.use_map; map; map >>= 1, ++i)
    {
        if (!(map & 1))
            continue;

        resource = &state->streams[stream_info.elements[i].stream_idx].buffer->resource;
        if (FAILED(wined3d_resource_unmap(resource, 0)))
            ERR("Failed to unmap resource.\n");
    }

    return hr;
}

void CDECL wined3d_device_set_texture_stage_state(struct wined3d_device *device,
        UINT stage, enum wined3d_texture_stage_state state, DWORD value)
{
    const struct wined3d_d3d_info *d3d_info = &device->adapter->d3d_info;
    DWORD old_value;

    TRACE("device %p, stage %u, state %s, value %#x.\n",
            device, stage, debug_d3dtexturestate(state), value);

    if (state > WINED3D_HIGHEST_TEXTURE_STATE)
    {
        WARN("Invalid state %#x passed.\n", state);
        return;
    }

    if (stage >= d3d_info->limits.ffp_blend_stages)
    {
        WARN("Attempting to set stage %u which is higher than the max stage %u, ignoring.\n",
                stage, d3d_info->limits.ffp_blend_stages - 1);
        return;
    }

    old_value = device->update_state->texture_states[stage][state];
    device->update_state->texture_states[stage][state] = value;

    if (device->recording)
    {
        TRACE("Recording... not performing anything.\n");
        device->recording->changed.textureState[stage] |= 1u << state;
        return;
    }

    /* Checked after the assignments to allow proper stateblock recording. */
    if (old_value == value)
    {
        TRACE("Application is setting the old value over, nothing to do.\n");
        return;
    }

    wined3d_cs_emit_set_texture_state(device->cs, stage, state, value);
}

DWORD CDECL wined3d_device_get_texture_stage_state(const struct wined3d_device *device,
        UINT stage, enum wined3d_texture_stage_state state)
{
    TRACE("device %p, stage %u, state %s.\n",
            device, stage, debug_d3dtexturestate(state));

    if (state > WINED3D_HIGHEST_TEXTURE_STATE)
    {
        WARN("Invalid state %#x passed.\n", state);
        return 0;
    }

    return device->state.texture_states[stage][state];
}

HRESULT CDECL wined3d_device_set_texture(struct wined3d_device *device,
        UINT stage, struct wined3d_texture *texture)
{
    struct wined3d_texture *prev;

    TRACE("device %p, stage %u, texture %p.\n", device, stage, texture);

    if (stage >= WINED3DVERTEXTEXTURESAMPLER0 && stage <= WINED3DVERTEXTEXTURESAMPLER3)
        stage -= (WINED3DVERTEXTEXTURESAMPLER0 - MAX_FRAGMENT_SAMPLERS);

    /* Windows accepts overflowing this array... we do not. */
    if (stage >= ARRAY_SIZE(device->state.textures))
    {
        WARN("Ignoring invalid stage %u.\n", stage);
        return WINED3D_OK;
    }

    if (texture && texture->resource.usage & WINED3DUSAGE_SCRATCH)
    {
        WARN("Rejecting attempt to set scratch texture.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (device->recording)
        device->recording->changed.textures |= 1u << stage;

    prev = device->update_state->textures[stage];
    TRACE("Previous texture %p.\n", prev);

    if (texture == prev)
    {
        TRACE("App is setting the same texture again, nothing to do.\n");
        return WINED3D_OK;
    }

    TRACE("Setting new texture to %p.\n", texture);
    device->update_state->textures[stage] = texture;

    if (texture)
        wined3d_texture_incref(texture);
    if (!device->recording)
        wined3d_cs_emit_set_texture(device->cs, stage, texture);
    if (prev)
        wined3d_texture_decref(prev);

    return WINED3D_OK;
}

struct wined3d_texture * CDECL wined3d_device_get_texture(const struct wined3d_device *device, UINT stage)
{
    TRACE("device %p, stage %u.\n", device, stage);

    if (stage >= WINED3DVERTEXTEXTURESAMPLER0 && stage <= WINED3DVERTEXTEXTURESAMPLER3)
        stage -= (WINED3DVERTEXTEXTURESAMPLER0 - MAX_FRAGMENT_SAMPLERS);

    if (stage >= ARRAY_SIZE(device->state.textures))
    {
        WARN("Ignoring invalid stage %u.\n", stage);
        return NULL; /* Windows accepts overflowing this array ... we do not. */
    }

    return device->state.textures[stage];
}

HRESULT CDECL wined3d_device_get_device_caps(const struct wined3d_device *device, WINED3DCAPS *caps)
{
    HRESULT hr;

    TRACE("device %p, caps %p.\n", device, caps);

    hr = wined3d_get_device_caps(device->wined3d, device->adapter->ordinal,
            device->create_parms.device_type, caps);

    if (SUCCEEDED(hr) && use_software_vertex_processing(device))
        caps->MaxVertexBlendMatrixIndex = 255;

    return hr;
}

HRESULT CDECL wined3d_device_get_display_mode(const struct wined3d_device *device, UINT swapchain_idx,
        struct wined3d_display_mode *mode, enum wined3d_display_rotation *rotation)
{
    struct wined3d_swapchain *swapchain;

    TRACE("device %p, swapchain_idx %u, mode %p, rotation %p.\n",
            device, swapchain_idx, mode, rotation);

    if (!(swapchain = wined3d_device_get_swapchain(device, swapchain_idx)))
        return WINED3DERR_INVALIDCALL;

    return wined3d_swapchain_get_display_mode(swapchain, mode, rotation);
}

HRESULT CDECL wined3d_device_begin_stateblock(struct wined3d_device *device)
{
    struct wined3d_stateblock *stateblock;
    HRESULT hr;

    TRACE("device %p.\n", device);

    if (device->recording)
        return WINED3DERR_INVALIDCALL;

    hr = wined3d_stateblock_create(device, WINED3D_SBT_RECORDED, &stateblock);
    if (FAILED(hr))
        return hr;

    device->recording = stateblock;
    device->update_state = &stateblock->state;

    TRACE("Recording stateblock %p.\n", stateblock);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_end_stateblock(struct wined3d_device *device,
        struct wined3d_stateblock **stateblock)
{
    struct wined3d_stateblock *object = device->recording;

    TRACE("device %p, stateblock %p.\n", device, stateblock);

    if (!device->recording)
    {
        WARN("Not recording.\n");
        *stateblock = NULL;
        return WINED3DERR_INVALIDCALL;
    }

    stateblock_init_contained_states(object);

    *stateblock = object;
    device->recording = NULL;
    device->update_state = &device->state;

    TRACE("Returning stateblock %p.\n", *stateblock);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_begin_scene(struct wined3d_device *device)
{
    /* At the moment we have no need for any functionality at the beginning
     * of a scene. */
    TRACE("device %p.\n", device);

    if (device->inScene)
    {
        WARN("Already in scene, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }
    device->inScene = TRUE;
    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_end_scene(struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    if (!device->inScene)
    {
        WARN("Not in scene, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    wined3d_cs_emit_flush(device->cs);

    device->inScene = FALSE;
    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_clear(struct wined3d_device *device, DWORD rect_count,
        const RECT *rects, DWORD flags, const struct wined3d_color *color, float depth, DWORD stencil)
{
    TRACE("device %p, rect_count %u, rects %p, flags %#x, color %s, depth %.8e, stencil %u.\n",
            device, rect_count, rects, flags, debug_color(color), depth, stencil);

    if (!rect_count && rects)
    {
        WARN("Rects is %p, but rect_count is 0, ignoring clear\n", rects);
        return WINED3D_OK;
    }

    if (flags & (WINED3DCLEAR_ZBUFFER | WINED3DCLEAR_STENCIL))
    {
        struct wined3d_rendertarget_view *ds = device->fb.depth_stencil;
        if (!ds)
        {
            WARN("Clearing depth and/or stencil without a depth stencil buffer attached, returning WINED3DERR_INVALIDCALL\n");
            /* TODO: What about depth stencil buffers without stencil bits? */
            return WINED3DERR_INVALIDCALL;
        }
        else if (flags & WINED3DCLEAR_TARGET)
        {
            if (ds->width < device->fb.render_targets[0]->width
                    || ds->height < device->fb.render_targets[0]->height)
            {
                WARN("Silently ignoring depth and target clear with mismatching sizes\n");
                return WINED3D_OK;
            }
        }
    }

    wined3d_cs_emit_clear(device->cs, rect_count, rects, flags, color, depth, stencil);

    return WINED3D_OK;
}

void CDECL wined3d_device_set_predication(struct wined3d_device *device,
        struct wined3d_query *predicate, BOOL value)
{
    struct wined3d_query *prev;

    TRACE("device %p, predicate %p, value %#x.\n", device, predicate, value);

    prev = device->update_state->predicate;
    if (predicate)
    {
        FIXME("Predicated rendering not implemented.\n");
        wined3d_query_incref(predicate);
    }
    device->update_state->predicate = predicate;
    device->update_state->predicate_value = value;
    if (!device->recording)
        wined3d_cs_emit_set_predication(device->cs, predicate, value);
    if (prev)
        wined3d_query_decref(prev);
}

struct wined3d_query * CDECL wined3d_device_get_predication(struct wined3d_device *device, BOOL *value)
{
    TRACE("device %p, value %p.\n", device, value);

    if (value)
        *value = device->state.predicate_value;
    return device->state.predicate;
}

void CDECL wined3d_device_dispatch_compute(struct wined3d_device *device,
        unsigned int group_count_x, unsigned int group_count_y, unsigned int group_count_z)
{
    TRACE("device %p, group_count_x %u, group_count_y %u, group_count_z %u.\n",
            device, group_count_x, group_count_y, group_count_z);

    wined3d_cs_emit_dispatch(device->cs, group_count_x, group_count_y, group_count_z);
}

void CDECL wined3d_device_dispatch_compute_indirect(struct wined3d_device *device,
        struct wined3d_buffer *buffer, unsigned int offset)
{
    TRACE("device %p, buffer %p, offset %u.\n", device, buffer, offset);

    wined3d_cs_emit_dispatch_indirect(device->cs, buffer, offset);
}

void CDECL wined3d_device_set_primitive_type(struct wined3d_device *device,
        enum wined3d_primitive_type primitive_type, unsigned int patch_vertex_count)
{
    TRACE("device %p, primitive_type %s, patch_vertex_count %u.\n",
            device, debug_d3dprimitivetype(primitive_type), patch_vertex_count);

    device->state.gl_primitive_type = gl_primitive_type_from_d3d(primitive_type);
    device->state.gl_patch_vertices = patch_vertex_count;
}

void CDECL wined3d_device_get_primitive_type(const struct wined3d_device *device,
        enum wined3d_primitive_type *primitive_type, unsigned int *patch_vertex_count)
{
    TRACE("device %p, primitive_type %p, patch_vertex_count %p.\n",
            device, primitive_type, patch_vertex_count);

    *primitive_type = d3d_primitive_type_from_gl(device->state.gl_primitive_type);
    if (patch_vertex_count)
        *patch_vertex_count = device->state.gl_patch_vertices;

    TRACE("Returning %s.\n", debug_d3dprimitivetype(*primitive_type));
}

HRESULT CDECL wined3d_device_draw_primitive(struct wined3d_device *device, UINT start_vertex, UINT vertex_count)
{
    TRACE("device %p, start_vertex %u, vertex_count %u.\n", device, start_vertex, vertex_count);

    wined3d_cs_emit_draw(device->cs, device->state.gl_primitive_type, device->state.gl_patch_vertices,
            0, start_vertex, vertex_count, 0, 0, FALSE);

    return WINED3D_OK;
}

void CDECL wined3d_device_draw_primitive_instanced(struct wined3d_device *device,
        UINT start_vertex, UINT vertex_count, UINT start_instance, UINT instance_count)
{
    TRACE("device %p, start_vertex %u, vertex_count %u, start_instance %u, instance_count %u.\n",
            device, start_vertex, vertex_count, start_instance, instance_count);

    wined3d_cs_emit_draw(device->cs, device->state.gl_primitive_type, device->state.gl_patch_vertices,
            0, start_vertex, vertex_count, start_instance, instance_count, FALSE);
}

void CDECL wined3d_device_draw_primitive_instanced_indirect(struct wined3d_device *device,
        struct wined3d_buffer *buffer, unsigned int offset)
{
    TRACE("device %p, buffer %p, offset %u.\n", device, buffer, offset);

    wined3d_cs_emit_draw_indirect(device->cs, device->state.gl_primitive_type, device->state.gl_patch_vertices,
            buffer, offset, FALSE);
}

HRESULT CDECL wined3d_device_draw_indexed_primitive(struct wined3d_device *device, UINT start_idx, UINT index_count)
{
    TRACE("device %p, start_idx %u, index_count %u.\n", device, start_idx, index_count);

    if (!device->state.index_buffer)
    {
        /* D3D9 returns D3DERR_INVALIDCALL when DrawIndexedPrimitive is called
         * without an index buffer set. (The first time at least...)
         * D3D8 simply dies, but I doubt it can do much harm to return
         * D3DERR_INVALIDCALL there as well. */
        WARN("Called without a valid index buffer set, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    wined3d_cs_emit_draw(device->cs, device->state.gl_primitive_type, device->state.gl_patch_vertices,
            device->state.base_vertex_index, start_idx, index_count, 0, 0, TRUE);

    return WINED3D_OK;
}

void CDECL wined3d_device_draw_indexed_primitive_instanced(struct wined3d_device *device,
        UINT start_idx, UINT index_count, UINT start_instance, UINT instance_count)
{
    TRACE("device %p, start_idx %u, index_count %u, start_instance %u, instance_count %u.\n",
            device, start_idx, index_count, start_instance, instance_count);

    wined3d_cs_emit_draw(device->cs, device->state.gl_primitive_type, device->state.gl_patch_vertices,
            device->state.base_vertex_index, start_idx, index_count, start_instance, instance_count, TRUE);
}

void CDECL wined3d_device_draw_indexed_primitive_instanced_indirect(struct wined3d_device *device,
        struct wined3d_buffer *buffer, unsigned int offset)
{
    TRACE("device %p, buffer %p, offset %u.\n", device, buffer, offset);

    wined3d_cs_emit_draw_indirect(device->cs, device->state.gl_primitive_type, device->state.gl_patch_vertices,
            buffer, offset, TRUE);
}

HRESULT CDECL wined3d_device_update_texture(struct wined3d_device *device,
        struct wined3d_texture *src_texture, struct wined3d_texture *dst_texture)
{
    unsigned int src_size, dst_size, src_skip_levels = 0;
    unsigned int src_level_count, dst_level_count;
    unsigned int layer_count, level_count, i, j;
    unsigned int width, height, depth;
    enum wined3d_resource_type type;
    struct wined3d_box box;

    TRACE("device %p, src_texture %p, dst_texture %p.\n", device, src_texture, dst_texture);

    /* Verify that the source and destination textures are non-NULL. */
    if (!src_texture || !dst_texture)
    {
        WARN("Source and destination textures must be non-NULL, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (src_texture->resource.access & WINED3D_RESOURCE_ACCESS_GPU
            || src_texture->resource.usage & WINED3DUSAGE_SCRATCH)
    {
        WARN("Source resource is GPU accessible or a scratch resource.\n");
        return WINED3DERR_INVALIDCALL;
    }
    if (dst_texture->resource.access & WINED3D_RESOURCE_ACCESS_CPU)
    {
        WARN("Destination resource is CPU accessible.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* Verify that the source and destination textures are the same type. */
    type = src_texture->resource.type;
    if (dst_texture->resource.type != type)
    {
        WARN("Source and destination have different types, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    layer_count = src_texture->layer_count;
    if (layer_count != dst_texture->layer_count)
    {
        WARN("Source and destination have different layer counts.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (src_texture->resource.format != dst_texture->resource.format)
    {
        WARN("Source and destination formats do not match.\n");
        return WINED3DERR_INVALIDCALL;
    }

    src_level_count = src_texture->level_count;
    dst_level_count = dst_texture->level_count;
    level_count = min(src_level_count, dst_level_count);

    src_size = max(src_texture->resource.width, src_texture->resource.height);
    dst_size = max(dst_texture->resource.width, dst_texture->resource.height);
    if (type == WINED3D_RTYPE_TEXTURE_3D)
    {
        src_size = max(src_size, src_texture->resource.depth);
        dst_size = max(dst_size, dst_texture->resource.depth);
    }
    while (src_size > dst_size)
    {
        src_size >>= 1;
        ++src_skip_levels;
    }

    if (wined3d_texture_get_level_width(src_texture, src_skip_levels) != dst_texture->resource.width
            || wined3d_texture_get_level_height(src_texture, src_skip_levels) != dst_texture->resource.height
            || wined3d_texture_get_level_depth(src_texture, src_skip_levels) != dst_texture->resource.depth)
    {
        WARN("Source and destination dimensions do not match.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* Update every surface level of the texture. */
    for (i = 0; i < level_count; ++i)
    {
        width = wined3d_texture_get_level_width(dst_texture, i);
        height = wined3d_texture_get_level_height(dst_texture, i);
        depth = wined3d_texture_get_level_depth(dst_texture, i);
        wined3d_box_set(&box, 0, 0, width, height, 0, depth);

        for (j = 0; j < layer_count; ++j)
        {
            wined3d_cs_emit_blt_sub_resource(device->cs,
                    &dst_texture->resource, j * dst_level_count + i, &box,
                    &src_texture->resource, j * src_level_count + i + src_skip_levels, &box,
                    0, NULL, WINED3D_TEXF_POINT);
        }
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_validate_device(const struct wined3d_device *device, DWORD *num_passes)
{
    const struct wined3d_state *state = &device->state;
    struct wined3d_texture *texture;
    DWORD i;

    TRACE("device %p, num_passes %p.\n", device, num_passes);

    for (i = 0; i < MAX_COMBINED_SAMPLERS; ++i)
    {
        if (state->sampler_states[i][WINED3D_SAMP_MIN_FILTER] == WINED3D_TEXF_NONE)
        {
            WARN("Sampler state %u has minfilter D3DTEXF_NONE, returning D3DERR_UNSUPPORTEDTEXTUREFILTER\n", i);
            return WINED3DERR_UNSUPPORTEDTEXTUREFILTER;
        }
        if (state->sampler_states[i][WINED3D_SAMP_MAG_FILTER] == WINED3D_TEXF_NONE)
        {
            WARN("Sampler state %u has magfilter D3DTEXF_NONE, returning D3DERR_UNSUPPORTEDTEXTUREFILTER\n", i);
            return WINED3DERR_UNSUPPORTEDTEXTUREFILTER;
        }

        texture = state->textures[i];
        if (!texture || texture->resource.format_flags & WINED3DFMT_FLAG_FILTERING) continue;

        if (state->sampler_states[i][WINED3D_SAMP_MAG_FILTER] != WINED3D_TEXF_POINT)
        {
            WARN("Non-filterable texture and mag filter enabled on sampler %u, returning E_FAIL\n", i);
            return E_FAIL;
        }
        if (state->sampler_states[i][WINED3D_SAMP_MIN_FILTER] != WINED3D_TEXF_POINT)
        {
            WARN("Non-filterable texture and min filter enabled on sampler %u, returning E_FAIL\n", i);
            return E_FAIL;
        }
        if (state->sampler_states[i][WINED3D_SAMP_MIP_FILTER] != WINED3D_TEXF_NONE
                && state->sampler_states[i][WINED3D_SAMP_MIP_FILTER] != WINED3D_TEXF_POINT)
        {
            WARN("Non-filterable texture and mip filter enabled on sampler %u, returning E_FAIL\n", i);
            return E_FAIL;
        }
    }

    if (state->render_states[WINED3D_RS_ZENABLE] || state->render_states[WINED3D_RS_ZWRITEENABLE]
            || state->render_states[WINED3D_RS_STENCILENABLE])
    {
        struct wined3d_rendertarget_view *rt = device->fb.render_targets[0];
        struct wined3d_rendertarget_view *ds = device->fb.depth_stencil;

        if (ds && rt && (ds->width < rt->width || ds->height < rt->height))
        {
            WARN("Depth stencil is smaller than the color buffer, returning D3DERR_CONFLICTINGRENDERSTATE\n");
            return WINED3DERR_CONFLICTINGRENDERSTATE;
        }
    }

    /* return a sensible default */
    *num_passes = 1;

    TRACE("returning D3D_OK\n");
    return WINED3D_OK;
}

void CDECL wined3d_device_set_software_vertex_processing(struct wined3d_device *device, BOOL software)
{
    TRACE("device %p, software %#x.\n", device, software);

    device->softwareVertexProcessing = software;
}

BOOL CDECL wined3d_device_get_software_vertex_processing(const struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    return device->softwareVertexProcessing;
}

HRESULT CDECL wined3d_device_get_raster_status(const struct wined3d_device *device,
        UINT swapchain_idx, struct wined3d_raster_status *raster_status)
{
    struct wined3d_swapchain *swapchain;

    TRACE("device %p, swapchain_idx %u, raster_status %p.\n",
            device, swapchain_idx, raster_status);

    if (!(swapchain = wined3d_device_get_swapchain(device, swapchain_idx)))
        return WINED3DERR_INVALIDCALL;

    return wined3d_swapchain_get_raster_status(swapchain, raster_status);
}

HRESULT CDECL wined3d_device_set_npatch_mode(struct wined3d_device *device, float segments)
{
    static BOOL warned;

    TRACE("device %p, segments %.8e.\n", device, segments);

    if (segments != 0.0f)
    {
        if (!warned)
        {
            FIXME("device %p, segments %.8e stub!\n", device, segments);
            warned = TRUE;
        }
    }

    return WINED3D_OK;
}

float CDECL wined3d_device_get_npatch_mode(const struct wined3d_device *device)
{
    static BOOL warned;

    TRACE("device %p.\n", device);

    if (!warned)
    {
        FIXME("device %p stub!\n", device);
        warned = TRUE;
    }

    return 0.0f;
}

void CDECL wined3d_device_copy_uav_counter(struct wined3d_device *device,
        struct wined3d_buffer *dst_buffer, unsigned int offset, struct wined3d_unordered_access_view *uav)
{
    TRACE("device %p, dst_buffer %p, offset %u, uav %p.\n",
            device, dst_buffer, offset, uav);

    if (offset + sizeof(GLuint) > dst_buffer->resource.size)
    {
        WARN("Offset %u too large.\n", offset);
        return;
    }

    wined3d_cs_emit_copy_uav_counter(device->cs, dst_buffer, offset, uav);
}

void CDECL wined3d_device_copy_resource(struct wined3d_device *device,
        struct wined3d_resource *dst_resource, struct wined3d_resource *src_resource)
{
    struct wined3d_texture *dst_texture, *src_texture;
    struct wined3d_box box;
    unsigned int i, j;

    TRACE("device %p, dst_resource %p, src_resource %p.\n", device, dst_resource, src_resource);

    if (src_resource == dst_resource)
    {
        WARN("Source and destination are the same resource.\n");
        return;
    }

    if (src_resource->type != dst_resource->type)
    {
        WARN("Resource types (%s / %s) don't match.\n",
                debug_d3dresourcetype(dst_resource->type),
                debug_d3dresourcetype(src_resource->type));
        return;
    }

    if (src_resource->width != dst_resource->width
            || src_resource->height != dst_resource->height
            || src_resource->depth != dst_resource->depth)
    {
        WARN("Resource dimensions (%ux%ux%u / %ux%ux%u) don't match.\n",
                dst_resource->width, dst_resource->height, dst_resource->depth,
                src_resource->width, src_resource->height, src_resource->depth);
        return;
    }

    if (src_resource->format->typeless_id != dst_resource->format->typeless_id
            || (!src_resource->format->typeless_id && src_resource->format->id != dst_resource->format->id))
    {
        WARN("Resource formats %s and %s are incompatible.\n",
                debug_d3dformat(dst_resource->format->id),
                debug_d3dformat(src_resource->format->id));
        return;
    }

    if (dst_resource->type == WINED3D_RTYPE_BUFFER)
    {
        wined3d_box_set(&box, 0, 0, src_resource->size, 1, 0, 1);
        wined3d_cs_emit_blt_sub_resource(device->cs, dst_resource, 0, &box,
                src_resource, 0, &box, WINED3D_BLT_RAW, NULL, WINED3D_TEXF_POINT);
        return;
    }

    dst_texture = texture_from_resource(dst_resource);
    src_texture = texture_from_resource(src_resource);

    if (src_texture->layer_count != dst_texture->layer_count
            || src_texture->level_count != dst_texture->level_count)
    {
        WARN("Subresource layouts (%ux%u / %ux%u) don't match.\n",
                dst_texture->layer_count, dst_texture->level_count,
                src_texture->layer_count, src_texture->level_count);
        return;
    }

    for (i = 0; i < dst_texture->level_count; ++i)
    {
        wined3d_box_set(&box, 0, 0,
                wined3d_texture_get_level_width(dst_texture, i),
                wined3d_texture_get_level_height(dst_texture, i),
                0, wined3d_texture_get_level_depth(dst_texture, i));
        for (j = 0; j < dst_texture->layer_count; ++j)
        {
            unsigned int idx = j * dst_texture->level_count + i;

            wined3d_cs_emit_blt_sub_resource(device->cs, dst_resource, idx, &box,
                    src_resource, idx, &box, WINED3D_BLT_RAW, NULL, WINED3D_TEXF_POINT);
        }
    }
}

HRESULT CDECL wined3d_device_copy_sub_resource_region(struct wined3d_device *device,
        struct wined3d_resource *dst_resource, unsigned int dst_sub_resource_idx, unsigned int dst_x,
        unsigned int dst_y, unsigned int dst_z, struct wined3d_resource *src_resource,
        unsigned int src_sub_resource_idx, const struct wined3d_box *src_box)
{
    struct wined3d_box dst_box, b;

    TRACE("device %p, dst_resource %p, dst_sub_resource_idx %u, dst_x %u, dst_y %u, dst_z %u, "
            "src_resource %p, src_sub_resource_idx %u, src_box %s.\n",
            device, dst_resource, dst_sub_resource_idx, dst_x, dst_y, dst_z,
            src_resource, src_sub_resource_idx, debug_box(src_box));

    if (src_resource == dst_resource && src_sub_resource_idx == dst_sub_resource_idx)
    {
        WARN("Source and destination are the same sub-resource.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (src_resource->type != dst_resource->type)
    {
        WARN("Resource types (%s / %s) don't match.\n",
                debug_d3dresourcetype(dst_resource->type),
                debug_d3dresourcetype(src_resource->type));
        return WINED3DERR_INVALIDCALL;
    }

    if (src_resource->format->typeless_id != dst_resource->format->typeless_id
            || (!src_resource->format->typeless_id && src_resource->format->id != dst_resource->format->id))
    {
        WARN("Resource formats %s and %s are incompatible.\n",
                debug_d3dformat(dst_resource->format->id),
                debug_d3dformat(src_resource->format->id));
        return WINED3DERR_INVALIDCALL;
    }

    if (dst_resource->type == WINED3D_RTYPE_BUFFER)
    {
        if (dst_sub_resource_idx)
        {
            WARN("Invalid dst_sub_resource_idx %u.\n", dst_sub_resource_idx);
            return WINED3DERR_INVALIDCALL;
        }

        if (src_sub_resource_idx)
        {
            WARN("Invalid src_sub_resource_idx %u.\n", src_sub_resource_idx);
            return WINED3DERR_INVALIDCALL;
        }

        if (!src_box)
        {
            unsigned int dst_w;

            dst_w = dst_resource->size - dst_x;
            wined3d_box_set(&b, 0, 0, min(src_resource->size, dst_w), 1, 0, 1);
            src_box = &b;
        }
        else if ((src_box->left >= src_box->right
                || src_box->top >= src_box->bottom
                || src_box->front >= src_box->back))
        {
            WARN("Invalid box %s specified.\n", debug_box(src_box));
            return WINED3DERR_INVALIDCALL;
        }

        if (src_box->right > src_resource->size || dst_x >= dst_resource->size
                || src_box->right - src_box->left > dst_resource->size - dst_x)
        {
            WARN("Invalid range specified, dst_offset %u, src_offset %u, size %u.\n",
                    dst_x, src_box->left, src_box->right - src_box->left);
            return WINED3DERR_INVALIDCALL;
        }

        wined3d_box_set(&dst_box, dst_x, 0, dst_x + (src_box->right - src_box->left), 1, 0, 1);
    }
    else if (dst_resource->type == WINED3D_RTYPE_TEXTURE_2D)
    {
        struct wined3d_texture *dst_texture = texture_from_resource(dst_resource);
        struct wined3d_texture *src_texture = texture_from_resource(src_resource);
        unsigned int src_level = src_sub_resource_idx % src_texture->level_count;

        if (dst_sub_resource_idx >= dst_texture->level_count * dst_texture->layer_count)
        {
            WARN("Invalid destination sub-resource %u.\n", dst_sub_resource_idx);
            return WINED3DERR_INVALIDCALL;
        }

        if (src_sub_resource_idx >= src_texture->level_count * src_texture->layer_count)
        {
            WARN("Invalid source sub-resource %u.\n", src_sub_resource_idx);
            return WINED3DERR_INVALIDCALL;
        }

#if !defined(STAGING_CSMT)
        if (dst_texture->sub_resources[dst_sub_resource_idx].map_count)
        {
            WARN("Destination sub-resource %u is mapped.\n", dst_sub_resource_idx);
            return WINED3DERR_INVALIDCALL;
        }

        if (src_texture->sub_resources[src_sub_resource_idx].map_count)
        {
            WARN("Source sub-resource %u is mapped.\n", src_sub_resource_idx);
            return WINED3DERR_INVALIDCALL;
#else  /* STAGING_CSMT */
        if (dst_texture->sub_resources[dst_sub_resource_idx].map_count ||
            src_texture->sub_resources[src_sub_resource_idx].map_count)
        {
            struct wined3d_device *device = dst_texture->resource.device;
            device->cs->ops->finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);
            if (dst_texture->sub_resources[dst_sub_resource_idx].map_count ||
                src_texture->sub_resources[src_sub_resource_idx].map_count)
            {
                WARN("Destination or source sub-resource is mapped.\n");
                return WINEDDERR_SURFACEBUSY;
            }
#endif /* STAGING_CSMT */
        }

        if (!src_box)
        {
            unsigned int src_w, src_h, dst_w, dst_h, dst_level;

            src_w = wined3d_texture_get_level_width(src_texture, src_level);
            src_h = wined3d_texture_get_level_height(src_texture, src_level);

            dst_level = dst_sub_resource_idx % dst_texture->level_count;
            dst_w = wined3d_texture_get_level_width(dst_texture, dst_level) - dst_x;
            dst_h = wined3d_texture_get_level_height(dst_texture, dst_level) - dst_y;

            wined3d_box_set(&b, 0, 0, min(src_w, dst_w), min(src_h, dst_h), 0, 1);
            src_box = &b;
        }
        else if (FAILED(wined3d_texture_check_box_dimensions(src_texture, src_level, src_box)))
        {
            WARN("Invalid source box %s.\n", debug_box(src_box));
            return WINED3DERR_INVALIDCALL;
        }

        wined3d_box_set(&dst_box, dst_x, dst_y, dst_x + (src_box->right - src_box->left),
                dst_y + (src_box->bottom - src_box->top), 0, 1);
        if (FAILED(wined3d_texture_check_box_dimensions(dst_texture,
                dst_sub_resource_idx % dst_texture->level_count, &dst_box)))
        {
            WARN("Invalid destination box %s.\n", debug_box(&dst_box));
            return WINED3DERR_INVALIDCALL;
        }
    }
    else
    {
        FIXME("Not implemented for %s resources.\n", debug_d3dresourcetype(dst_resource->type));
        return WINED3DERR_INVALIDCALL;
    }

    wined3d_cs_emit_blt_sub_resource(device->cs, dst_resource, dst_sub_resource_idx, &dst_box,
            src_resource, src_sub_resource_idx, src_box, WINED3D_BLT_RAW, NULL, WINED3D_TEXF_POINT);

    return WINED3D_OK;
}

void CDECL wined3d_device_update_sub_resource(struct wined3d_device *device, struct wined3d_resource *resource,
        unsigned int sub_resource_idx, const struct wined3d_box *box, const void *data, unsigned int row_pitch,
        unsigned int depth_pitch)
{
    unsigned int width, height, depth;
    struct wined3d_box b;

    TRACE("device %p, resource %p, sub_resource_idx %u, box %s, data %p, row_pitch %u, depth_pitch %u.\n",
            device, resource, sub_resource_idx, debug_box(box), data, row_pitch, depth_pitch);

    if (resource->type == WINED3D_RTYPE_BUFFER)
    {
        if (sub_resource_idx > 0)
        {
            WARN("Invalid sub_resource_idx %u.\n", sub_resource_idx);
            return;
        }

        width = resource->size;
        height = 1;
        depth = 1;
    }
    else if (resource->type == WINED3D_RTYPE_TEXTURE_1D ||
            resource->type == WINED3D_RTYPE_TEXTURE_2D || resource->type == WINED3D_RTYPE_TEXTURE_3D)
    {
        struct wined3d_texture *texture = texture_from_resource(resource);
        unsigned int level;

        if (sub_resource_idx >= texture->level_count * texture->layer_count)
        {
            WARN("Invalid sub_resource_idx %u.\n", sub_resource_idx);
            return;
        }

        level = sub_resource_idx % texture->level_count;
        width = wined3d_texture_get_level_width(texture, level);
        height = wined3d_texture_get_level_height(texture, level);
        depth = wined3d_texture_get_level_depth(texture, level);
    }
    else
    {
        FIXME("Not implemented for %s resources.\n", debug_d3dresourcetype(resource->type));
        return;
    }

    if (!box)
    {
        wined3d_box_set(&b, 0, 0, width, height, 0, depth);
        box = &b;
    }
    else if (box->left >= box->right || box->right > width
            || box->top >= box->bottom || box->bottom > height
            || box->front >= box->back || box->back > depth)
    {
        WARN("Invalid box %s specified.\n", debug_box(box));
        return;
    }

#if !defined(STAGING_CSMT)
    wined3d_resource_wait_idle(resource);

#endif /* STAGING_CSMT */
    wined3d_cs_emit_update_sub_resource(device->cs, resource, sub_resource_idx, box, data, row_pitch, depth_pitch);
}

void CDECL wined3d_device_resolve_sub_resource(struct wined3d_device *device,
        struct wined3d_resource *dst_resource, unsigned int dst_sub_resource_idx,
        struct wined3d_resource *src_resource, unsigned int src_sub_resource_idx,
        enum wined3d_format_id format_id)
{
    struct wined3d_texture *dst_texture, *src_texture;
    unsigned int dst_level, src_level;
    RECT dst_rect, src_rect;

    TRACE("device %p, dst_resource %p, dst_sub_resource_idx %u, "
            "src_resource %p, src_sub_resource_idx %u, format %s.\n",
            device, dst_resource, dst_sub_resource_idx,
            src_resource, src_sub_resource_idx, debug_d3dformat(format_id));

    if (wined3d_format_is_typeless(dst_resource->format)
            || wined3d_format_is_typeless(src_resource->format))
    {
        FIXME("Unhandled multisample resolve, dst_format %s, src_format %s, format %s.\n",
                debug_d3dformat(dst_resource->format->id), debug_d3dformat(src_resource->format->id),
                debug_d3dformat(format_id));
        return;
    }
    if (dst_resource->type != WINED3D_RTYPE_TEXTURE_2D)
    {
        WARN("Invalid destination resource type %s.\n", debug_d3dresourcetype(dst_resource->type));
        return;
    }
    if (src_resource->type != WINED3D_RTYPE_TEXTURE_2D)
    {
        WARN("Invalid source resource type %s.\n", debug_d3dresourcetype(src_resource->type));
        return;
    }

    dst_texture = texture_from_resource(dst_resource);
    src_texture = texture_from_resource(src_resource);

    dst_level = dst_sub_resource_idx % dst_texture->level_count;
    SetRect(&dst_rect, 0, 0, wined3d_texture_get_level_width(dst_texture, dst_level),
            wined3d_texture_get_level_height(dst_texture, dst_level));
    src_level = src_sub_resource_idx % src_texture->level_count;
    SetRect(&src_rect, 0, 0, wined3d_texture_get_level_width(src_texture, src_level),
            wined3d_texture_get_level_height(src_texture, src_level));
    wined3d_texture_blt(dst_texture, dst_sub_resource_idx, &dst_rect,
            src_texture, src_sub_resource_idx, &src_rect, 0, NULL, WINED3D_TEXF_POINT);
}

HRESULT CDECL wined3d_device_clear_rendertarget_view(struct wined3d_device *device,
        struct wined3d_rendertarget_view *view, const RECT *rect, DWORD flags,
        const struct wined3d_color *color, float depth, DWORD stencil)
{
    struct wined3d_resource *resource;
    RECT r;

    TRACE("device %p, view %p, rect %s, flags %#x, color %s, depth %.8e, stencil %u.\n",
            device, view, wine_dbgstr_rect(rect), flags, debug_color(color), depth, stencil);

    if (!flags)
        return WINED3D_OK;

    resource = view->resource;
    if (resource->type != WINED3D_RTYPE_TEXTURE_2D)
    {
        FIXME("Not implemented for %s resources.\n", debug_d3dresourcetype(resource->type));
        return WINED3DERR_INVALIDCALL;
    }

    if (view->layer_count > 1)
    {
        FIXME("Layered clears not implemented.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (!rect)
    {
        SetRect(&r, 0, 0, view->width, view->height);
        rect = &r;
    }
    else
    {
        struct wined3d_box b = {rect->left, rect->top, rect->right, rect->bottom, 0, 1};
        struct wined3d_texture *texture = texture_from_resource(view->resource);
        HRESULT hr;

        if (FAILED(hr = wined3d_texture_check_box_dimensions(texture,
                view->sub_resource_idx % texture->level_count, &b)))
            return hr;
    }

    wined3d_cs_emit_clear_rendertarget_view(device->cs, view, rect, flags, color, depth, stencil);

    return WINED3D_OK;
}

void CDECL wined3d_device_clear_unordered_access_view_uint(struct wined3d_device *device,
        struct wined3d_unordered_access_view *view, const struct wined3d_uvec4 *clear_value)
{
    TRACE("device %p, view %p, clear_value %s.\n", device, view, debug_uvec4(clear_value));

    wined3d_cs_emit_clear_unordered_access_view_uint(device->cs, view, clear_value);
}

struct wined3d_rendertarget_view * CDECL wined3d_device_get_rendertarget_view(const struct wined3d_device *device,
        unsigned int view_idx)
{
    TRACE("device %p, view_idx %u.\n", device, view_idx);

    if (view_idx >= device->adapter->gl_info.limits.buffers)
    {
        WARN("Only %u render targets are supported.\n", device->adapter->gl_info.limits.buffers);
        return NULL;
    }

    return device->fb.render_targets[view_idx];
}

struct wined3d_rendertarget_view * CDECL wined3d_device_get_depth_stencil_view(const struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    return device->fb.depth_stencil;
}

HRESULT CDECL wined3d_device_set_rendertarget_view(struct wined3d_device *device,
        unsigned int view_idx, struct wined3d_rendertarget_view *view, BOOL set_viewport)
{
    struct wined3d_rendertarget_view *prev;

    TRACE("device %p, view_idx %u, view %p, set_viewport %#x.\n",
            device, view_idx, view, set_viewport);

    if (view_idx >= device->adapter->gl_info.limits.buffers)
    {
        WARN("Only %u render targets are supported.\n", device->adapter->gl_info.limits.buffers);
        return WINED3DERR_INVALIDCALL;
    }

    if (view && !(view->resource->usage & WINED3DUSAGE_RENDERTARGET))
    {
        WARN("View resource %p doesn't have render target usage.\n", view->resource);
        return WINED3DERR_INVALIDCALL;
    }

    /* Set the viewport and scissor rectangles, if requested. Tests show that
     * stateblock recording is ignored, the change goes directly into the
     * primary stateblock. */
    if (!view_idx && set_viewport)
    {
        struct wined3d_state *state = &device->state;

        state->viewport.x = 0;
        state->viewport.y = 0;
        state->viewport.width = view->width;
        state->viewport.height = view->height;
        state->viewport.min_z = 0.0f;
        state->viewport.max_z = 1.0f;
        wined3d_cs_emit_set_viewport(device->cs, &state->viewport);

        SetRect(&state->scissor_rect, 0, 0, view->width, view->height);
        wined3d_cs_emit_set_scissor_rect(device->cs, &state->scissor_rect);
    }


    prev = device->fb.render_targets[view_idx];
    if (view == prev)
        return WINED3D_OK;

    if (view)
        wined3d_rendertarget_view_incref(view);
    device->fb.render_targets[view_idx] = view;
    wined3d_cs_emit_set_rendertarget_view(device->cs, view_idx, view);
    /* Release after the assignment, to prevent device_resource_released()
     * from seeing the surface as still in use. */
    if (prev)
        wined3d_rendertarget_view_decref(prev);

    return WINED3D_OK;
}

void CDECL wined3d_device_set_depth_stencil_view(struct wined3d_device *device, struct wined3d_rendertarget_view *view)
{
    struct wined3d_rendertarget_view *prev;

    TRACE("device %p, view %p.\n", device, view);

    prev = device->fb.depth_stencil;
    if (prev == view)
    {
        TRACE("Trying to do a NOP SetRenderTarget operation.\n");
        return;
    }

    if ((device->fb.depth_stencil = view))
        wined3d_rendertarget_view_incref(view);
    wined3d_cs_emit_set_depth_stencil_view(device->cs, view);
    if (prev)
        wined3d_rendertarget_view_decref(prev);
}

static struct wined3d_texture *wined3d_device_create_cursor_texture(struct wined3d_device *device,
        struct wined3d_texture *cursor_image, unsigned int sub_resource_idx)
{
    unsigned int texture_level = sub_resource_idx % cursor_image->level_count;
    struct wined3d_sub_resource_data data;
    struct wined3d_resource_desc desc;
    struct wined3d_map_desc map_desc;
    struct wined3d_texture *texture;
    HRESULT hr;

    if (FAILED(wined3d_resource_map(&cursor_image->resource, sub_resource_idx, &map_desc, NULL, WINED3D_MAP_READ)))
    {
        ERR("Failed to map source texture.\n");
        return NULL;
    }

    data.data = map_desc.data;
    data.row_pitch = map_desc.row_pitch;
    data.slice_pitch = map_desc.slice_pitch;

    desc.resource_type = WINED3D_RTYPE_TEXTURE_2D;
    desc.format = WINED3DFMT_B8G8R8A8_UNORM;
    desc.multisample_type = WINED3D_MULTISAMPLE_NONE;
    desc.multisample_quality = 0;
    desc.usage = WINED3DUSAGE_DYNAMIC;
    desc.access = WINED3D_RESOURCE_ACCESS_GPU;
    desc.width = wined3d_texture_get_level_width(cursor_image, texture_level);
    desc.height = wined3d_texture_get_level_height(cursor_image, texture_level);
    desc.depth = 1;
    desc.size = 0;

    hr = wined3d_texture_create(device, &desc, 1, 1, WINED3D_TEXTURE_CREATE_MAPPABLE,
            &data, NULL, &wined3d_null_parent_ops, &texture);
    wined3d_resource_unmap(&cursor_image->resource, sub_resource_idx);
    if (FAILED(hr))
    {
        ERR("Failed to create cursor texture.\n");
        return NULL;
    }

    return texture;
}

HRESULT CDECL wined3d_device_set_cursor_properties(struct wined3d_device *device,
        UINT x_hotspot, UINT y_hotspot, struct wined3d_texture *texture, unsigned int sub_resource_idx)
{
    unsigned int texture_level = sub_resource_idx % texture->level_count;
    unsigned int cursor_width, cursor_height;
    struct wined3d_display_mode mode;
    struct wined3d_map_desc map_desc;
    HRESULT hr;

    TRACE("device %p, x_hotspot %u, y_hotspot %u, texture %p, sub_resource_idx %u.\n",
            device, x_hotspot, y_hotspot, texture, sub_resource_idx);

    if (sub_resource_idx >= texture->level_count * texture->layer_count
            || texture->resource.type != WINED3D_RTYPE_TEXTURE_2D)
        return WINED3DERR_INVALIDCALL;

    if (device->cursor_texture)
    {
        wined3d_texture_decref(device->cursor_texture);
        device->cursor_texture = NULL;
    }

    if (texture->resource.format->id != WINED3DFMT_B8G8R8A8_UNORM)
    {
        WARN("Texture %p has invalid format %s.\n",
                texture, debug_d3dformat(texture->resource.format->id));
        return WINED3DERR_INVALIDCALL;
    }

    if (FAILED(hr = wined3d_get_adapter_display_mode(device->wined3d, device->adapter->ordinal, &mode, NULL)))
    {
        ERR("Failed to get display mode, hr %#x.\n", hr);
        return WINED3DERR_INVALIDCALL;
    }

    cursor_width = wined3d_texture_get_level_width(texture, texture_level);
    cursor_height = wined3d_texture_get_level_height(texture, texture_level);
    if (cursor_width > mode.width || cursor_height > mode.height)
    {
        WARN("Texture %p, sub-resource %u dimensions are %ux%u, but screen dimensions are %ux%u.\n",
                texture, sub_resource_idx, cursor_width, cursor_height, mode.width, mode.height);
        return WINED3DERR_INVALIDCALL;
    }

    /* TODO: MSDN: Cursor sizes must be a power of 2 */

    /* Do not store the surface's pointer because the application may
     * release it after setting the cursor image. Windows doesn't
     * addref the set surface, so we can't do this either without
     * creating circular refcount dependencies. */
    if (!(device->cursor_texture = wined3d_device_create_cursor_texture(device, texture, sub_resource_idx)))
    {
        ERR("Failed to create cursor texture.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (cursor_width == 32 && cursor_height == 32)
    {
        UINT mask_size = cursor_width * cursor_height / 8;
        ICONINFO cursor_info;
        DWORD *mask_bits;
        HCURSOR cursor;

        /* 32-bit user32 cursors ignore the alpha channel if it's all
         * zeroes, and use the mask instead. Fill the mask with all ones
         * to ensure we still get a fully transparent cursor. */
        if (!(mask_bits = heap_alloc(mask_size)))
            return E_OUTOFMEMORY;
        memset(mask_bits, 0xff, mask_size);

        wined3d_resource_map(&texture->resource, sub_resource_idx, &map_desc, NULL,
                WINED3D_MAP_NO_DIRTY_UPDATE | WINED3D_MAP_READ);
        cursor_info.fIcon = FALSE;
        cursor_info.xHotspot = x_hotspot;
        cursor_info.yHotspot = y_hotspot;
        cursor_info.hbmMask = CreateBitmap(cursor_width, cursor_height, 1, 1, mask_bits);
        cursor_info.hbmColor = CreateBitmap(cursor_width, cursor_height, 1, 32, map_desc.data);
        wined3d_resource_unmap(&texture->resource, sub_resource_idx);

        /* Create our cursor and clean up. */
        cursor = CreateIconIndirect(&cursor_info);
        if (cursor_info.hbmMask)
            DeleteObject(cursor_info.hbmMask);
        if (cursor_info.hbmColor)
            DeleteObject(cursor_info.hbmColor);
        if (device->hardwareCursor)
            DestroyCursor(device->hardwareCursor);
        device->hardwareCursor = cursor;
        if (device->bCursorVisible)
            SetCursor(cursor);

        heap_free(mask_bits);
    }

    TRACE("New cursor dimensions are %ux%u.\n", cursor_width, cursor_height);
    device->cursorWidth = cursor_width;
    device->cursorHeight = cursor_height;
    device->xHotSpot = x_hotspot;
    device->yHotSpot = y_hotspot;

    return WINED3D_OK;
}

void CDECL wined3d_device_set_cursor_position(struct wined3d_device *device,
        int x_screen_space, int y_screen_space, DWORD flags)
{
    TRACE("device %p, x %d, y %d, flags %#x.\n",
            device, x_screen_space, y_screen_space, flags);

    device->xScreenSpace = x_screen_space;
    device->yScreenSpace = y_screen_space;

    if (device->hardwareCursor)
    {
        POINT pt;

        GetCursorPos( &pt );
        if (x_screen_space == pt.x && y_screen_space == pt.y)
            return;
        SetCursorPos( x_screen_space, y_screen_space );

        /* Switch to the software cursor if position diverges from the hardware one. */
        GetCursorPos( &pt );
        if (x_screen_space != pt.x || y_screen_space != pt.y)
        {
            if (device->bCursorVisible) SetCursor( NULL );
            DestroyCursor( device->hardwareCursor );
            device->hardwareCursor = 0;
        }
    }
}

BOOL CDECL wined3d_device_show_cursor(struct wined3d_device *device, BOOL show)
{
    BOOL oldVisible = device->bCursorVisible;

    TRACE("device %p, show %#x.\n", device, show);

    /*
     * When ShowCursor is first called it should make the cursor appear at the OS's last
     * known cursor position.
     */
    if (show && !oldVisible)
    {
        POINT pt;
        GetCursorPos(&pt);
        device->xScreenSpace = pt.x;
        device->yScreenSpace = pt.y;
    }

    if (device->hardwareCursor)
    {
        device->bCursorVisible = show;
        if (show)
            SetCursor(device->hardwareCursor);
        else
            SetCursor(NULL);
    }
    else if (device->cursor_texture)
    {
        device->bCursorVisible = show;
    }

    return oldVisible;
}

void CDECL wined3d_device_evict_managed_resources(struct wined3d_device *device)
{
    struct wined3d_resource *resource, *cursor;

    TRACE("device %p.\n", device);

    LIST_FOR_EACH_ENTRY_SAFE(resource, cursor, &device->resources, struct wined3d_resource, resource_list_entry)
    {
        TRACE("Checking resource %p for eviction.\n", resource);

        if (wined3d_resource_access_is_managed(resource->access) && !resource->map_count)
        {
            TRACE("Evicting %p.\n", resource);
            wined3d_cs_emit_unload_resource(device->cs, resource);
        }
    }
}

HRESULT CDECL wined3d_device_reset(struct wined3d_device *device,
        const struct wined3d_swapchain_desc *swapchain_desc, const struct wined3d_display_mode *mode,
        wined3d_device_reset_cb callback, BOOL reset_state)
{
    struct wined3d_resource *resource, *cursor;
    struct wined3d_swapchain *swapchain;
    struct wined3d_view_desc view_desc;
    BOOL backbuffer_resized;
    HRESULT hr = WINED3D_OK;
    unsigned int i;

    TRACE("device %p, swapchain_desc %p, mode %p, callback %p, reset_state %#x.\n",
            device, swapchain_desc, mode, callback, reset_state);

    device->cs->ops->finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);

    if (!(swapchain = wined3d_device_get_swapchain(device, 0)))
    {
        ERR("Failed to get the first implicit swapchain.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (reset_state)
    {
        if (device->logo_texture)
        {
            wined3d_texture_decref(device->logo_texture);
            device->logo_texture = NULL;
        }
        if (device->cursor_texture)
        {
            wined3d_texture_decref(device->cursor_texture);
            device->cursor_texture = NULL;
        }
        state_unbind_resources(&device->state);
    }

    for (i = 0; i < device->adapter->gl_info.limits.buffers; ++i)
    {
        wined3d_device_set_rendertarget_view(device, i, NULL, FALSE);
    }
    wined3d_device_set_depth_stencil_view(device, NULL);

    if (reset_state)
    {
        LIST_FOR_EACH_ENTRY_SAFE(resource, cursor, &device->resources, struct wined3d_resource, resource_list_entry)
        {
            TRACE("Enumerating resource %p.\n", resource);
            if (FAILED(hr = callback(resource)))
                return hr;
        }
    }

    TRACE("New params:\n");
    TRACE("backbuffer_width %u\n", swapchain_desc->backbuffer_width);
    TRACE("backbuffer_height %u\n", swapchain_desc->backbuffer_height);
    TRACE("backbuffer_format %s\n", debug_d3dformat(swapchain_desc->backbuffer_format));
    TRACE("backbuffer_count %u\n", swapchain_desc->backbuffer_count);
    TRACE("multisample_type %#x\n", swapchain_desc->multisample_type);
    TRACE("multisample_quality %u\n", swapchain_desc->multisample_quality);
    TRACE("swap_effect %#x\n", swapchain_desc->swap_effect);
    TRACE("device_window %p\n", swapchain_desc->device_window);
    TRACE("windowed %#x\n", swapchain_desc->windowed);
    TRACE("enable_auto_depth_stencil %#x\n", swapchain_desc->enable_auto_depth_stencil);
    if (swapchain_desc->enable_auto_depth_stencil)
        TRACE("auto_depth_stencil_format %s\n", debug_d3dformat(swapchain_desc->auto_depth_stencil_format));
    TRACE("flags %#x\n", swapchain_desc->flags);
    TRACE("refresh_rate %u\n", swapchain_desc->refresh_rate);
    TRACE("swap_interval %u\n", swapchain_desc->swap_interval);
    TRACE("auto_restore_display_mode %#x\n", swapchain_desc->auto_restore_display_mode);

    if (swapchain_desc->backbuffer_usage != WINED3DUSAGE_RENDERTARGET)
        FIXME("Got unexpected backbuffer usage %#x.\n", swapchain_desc->backbuffer_usage);

    if (swapchain_desc->swap_effect != WINED3D_SWAP_EFFECT_DISCARD
            && swapchain_desc->swap_effect != WINED3D_SWAP_EFFECT_SEQUENTIAL
            && swapchain_desc->swap_effect != WINED3D_SWAP_EFFECT_COPY)
        FIXME("Unimplemented swap effect %#x.\n", swapchain_desc->swap_effect);

    /* No special treatment of these parameters. Just store them */
    swapchain->desc.swap_effect = swapchain_desc->swap_effect;
    swapchain->desc.enable_auto_depth_stencil = swapchain_desc->enable_auto_depth_stencil;
    swapchain->desc.auto_depth_stencil_format = swapchain_desc->auto_depth_stencil_format;
    swapchain->desc.flags = swapchain_desc->flags;
    swapchain->desc.refresh_rate = swapchain_desc->refresh_rate;
    swapchain->desc.swap_interval = swapchain_desc->swap_interval;
    swapchain->desc.auto_restore_display_mode = swapchain_desc->auto_restore_display_mode;

    if (swapchain_desc->device_window
            && swapchain_desc->device_window != swapchain->desc.device_window)
    {
        TRACE("Changing the device window from %p to %p.\n",
                swapchain->desc.device_window, swapchain_desc->device_window);
        swapchain->desc.device_window = swapchain_desc->device_window;
        swapchain->device_window = swapchain_desc->device_window;
        wined3d_swapchain_set_window(swapchain, NULL);
    }

    backbuffer_resized = swapchain_desc->backbuffer_width != swapchain->desc.backbuffer_width
            || swapchain_desc->backbuffer_height != swapchain->desc.backbuffer_height;

    if (!swapchain_desc->windowed != !swapchain->desc.windowed
            || swapchain->reapply_mode || mode
            || (!swapchain_desc->windowed && backbuffer_resized))
    {
        if (FAILED(hr = wined3d_swapchain_set_fullscreen(swapchain, swapchain_desc, mode)))
            return hr;
    }
    else if (!swapchain_desc->windowed)
    {
        DWORD style = device->style;
        DWORD exStyle = device->exStyle;
        /* If we're in fullscreen, and the mode wasn't changed, we have to get the window back into
         * the right position. Some applications(Battlefield 2, Guild Wars) move it and then call
         * Reset to clear up their mess. Guild Wars also loses the device during that.
         */
        device->style = 0;
        device->exStyle = 0;
        wined3d_device_setup_fullscreen_window(device, swapchain->device_window,
                swapchain_desc->backbuffer_width,
                swapchain_desc->backbuffer_height);
        device->style = style;
        device->exStyle = exStyle;
    }

    if (FAILED(hr = wined3d_swapchain_resize_buffers(swapchain, swapchain_desc->backbuffer_count,
            swapchain_desc->backbuffer_width, swapchain_desc->backbuffer_height, swapchain_desc->backbuffer_format,
            swapchain_desc->multisample_type, swapchain_desc->multisample_quality)))
        return hr;

    if (device->auto_depth_stencil_view)
    {
        wined3d_rendertarget_view_decref(device->auto_depth_stencil_view);
        device->auto_depth_stencil_view = NULL;
    }
    if (swapchain->desc.enable_auto_depth_stencil)
    {
        struct wined3d_resource_desc texture_desc;
        struct wined3d_texture *texture;
        DWORD flags = 0;

        TRACE("Creating the depth stencil buffer.\n");

        texture_desc.resource_type = WINED3D_RTYPE_TEXTURE_2D;
        texture_desc.format = swapchain->desc.auto_depth_stencil_format;
        texture_desc.multisample_type = swapchain->desc.multisample_type;
        texture_desc.multisample_quality = swapchain->desc.multisample_quality;
        texture_desc.usage = WINED3DUSAGE_DEPTHSTENCIL;
        texture_desc.access = WINED3D_RESOURCE_ACCESS_GPU;
        texture_desc.width = swapchain->desc.backbuffer_width;
        texture_desc.height = swapchain->desc.backbuffer_height;
        texture_desc.depth = 1;
        texture_desc.size = 0;

        if (swapchain_desc->flags & WINED3D_SWAPCHAIN_GDI_COMPATIBLE)
            flags |= WINED3D_TEXTURE_CREATE_GET_DC;

        if (FAILED(hr = device->device_parent->ops->create_swapchain_texture(device->device_parent,
                device->device_parent, &texture_desc, flags, &texture)))
        {
            ERR("Failed to create the auto depth/stencil surface, hr %#x.\n", hr);
            return WINED3DERR_INVALIDCALL;
        }

        view_desc.format_id = texture->resource.format->id;
        view_desc.flags = 0;
        view_desc.u.texture.level_idx = 0;
        view_desc.u.texture.level_count = 1;
        view_desc.u.texture.layer_idx = 0;
        view_desc.u.texture.layer_count = 1;
        hr = wined3d_rendertarget_view_create(&view_desc, &texture->resource,
                NULL, &wined3d_null_parent_ops, &device->auto_depth_stencil_view);
        wined3d_texture_decref(texture);
        if (FAILED(hr))
        {
            ERR("Failed to create rendertarget view, hr %#x.\n", hr);
            return hr;
        }

        wined3d_device_set_depth_stencil_view(device, device->auto_depth_stencil_view);
    }

    if (device->back_buffer_view)
    {
        wined3d_rendertarget_view_decref(device->back_buffer_view);
        device->back_buffer_view = NULL;
    }
    if (swapchain->desc.backbuffer_count)
    {
        struct wined3d_resource *back_buffer = &swapchain->back_buffers[0]->resource;

        view_desc.format_id = back_buffer->format->id;
        view_desc.flags = 0;
        view_desc.u.texture.level_idx = 0;
        view_desc.u.texture.level_count = 1;
        view_desc.u.texture.layer_idx = 0;
        view_desc.u.texture.layer_count = 1;
        if (FAILED(hr = wined3d_rendertarget_view_create(&view_desc, back_buffer,
                NULL, &wined3d_null_parent_ops, &device->back_buffer_view)))
        {
            ERR("Failed to create rendertarget view, hr %#x.\n", hr);
            return hr;
        }
    }

    wine_rb_clear(&device->samplers, device_free_sampler, NULL);

    if (reset_state)
    {
        TRACE("Resetting stateblock.\n");
        if (device->recording)
        {
            wined3d_stateblock_decref(device->recording);
            device->recording = NULL;
        }
        wined3d_cs_emit_reset_state(device->cs);
        state_cleanup(&device->state);

        if (device->d3d_initialized)
            wined3d_device_delete_opengl_contexts(device);

        memset(&device->state, 0, sizeof(device->state));
        state_init(&device->state, &device->fb, &device->adapter->gl_info,
                &device->adapter->d3d_info, WINED3D_STATE_INIT_DEFAULT);
        device->update_state = &device->state;

        device_init_swapchain_state(device, swapchain);
        if (wined3d_settings.logo)
            device_load_logo(device, wined3d_settings.logo);
    }
    else if (device->back_buffer_view)
    {
        struct wined3d_rendertarget_view *view = device->back_buffer_view;
        struct wined3d_state *state = &device->state;

        wined3d_device_set_rendertarget_view(device, 0, view, FALSE);

        /* Note the min_z / max_z is not reset. */
        state->viewport.x = 0;
        state->viewport.y = 0;
        state->viewport.width = view->width;
        state->viewport.height = view->height;
        wined3d_cs_emit_set_viewport(device->cs, &state->viewport);

        SetRect(&state->scissor_rect, 0, 0, view->width, view->height);
        wined3d_cs_emit_set_scissor_rect(device->cs, &state->scissor_rect);
    }

    if (device->d3d_initialized)
    {
        if (reset_state)
            hr = wined3d_device_create_primary_opengl_context(device);
        swapchain_update_swap_interval(swapchain);
    }

    /* All done. There is no need to reload resources or shaders, this will happen automatically on the
     * first use
     */
    return hr;
}

HRESULT CDECL wined3d_device_set_dialog_box_mode(struct wined3d_device *device, BOOL enable_dialogs)
{
    TRACE("device %p, enable_dialogs %#x.\n", device, enable_dialogs);

    if (!enable_dialogs) FIXME("Dialogs cannot be disabled yet.\n");

    return WINED3D_OK;
}


void CDECL wined3d_device_get_creation_parameters(const struct wined3d_device *device,
        struct wined3d_device_creation_parameters *parameters)
{
    TRACE("device %p, parameters %p.\n", device, parameters);

    *parameters = device->create_parms;
}

struct wined3d * CDECL wined3d_device_get_wined3d(const struct wined3d_device *device)
{
    TRACE("device %p.\n", device);

    return device->wined3d;
}

void CDECL wined3d_device_set_gamma_ramp(const struct wined3d_device *device,
        UINT swapchain_idx, DWORD flags, const struct wined3d_gamma_ramp *ramp)
{
    struct wined3d_swapchain *swapchain;

    TRACE("device %p, swapchain_idx %u, flags %#x, ramp %p.\n",
            device, swapchain_idx, flags, ramp);

    if ((swapchain = wined3d_device_get_swapchain(device, swapchain_idx)))
        wined3d_swapchain_set_gamma_ramp(swapchain, flags, ramp);
}

void CDECL wined3d_device_get_gamma_ramp(const struct wined3d_device *device,
        UINT swapchain_idx, struct wined3d_gamma_ramp *ramp)
{
    struct wined3d_swapchain *swapchain;

    TRACE("device %p, swapchain_idx %u, ramp %p.\n",
            device, swapchain_idx, ramp);

    if ((swapchain = wined3d_device_get_swapchain(device, swapchain_idx)))
        wined3d_swapchain_get_gamma_ramp(swapchain, ramp);
}

void device_resource_add(struct wined3d_device *device, struct wined3d_resource *resource)
{
    TRACE("device %p, resource %p.\n", device, resource);

    wined3d_not_from_cs(device->cs);

    list_add_head(&device->resources, &resource->resource_list_entry);
}

static void device_resource_remove(struct wined3d_device *device, struct wined3d_resource *resource)
{
    TRACE("device %p, resource %p.\n", device, resource);

    wined3d_not_from_cs(device->cs);

    list_remove(&resource->resource_list_entry);
}

void device_resource_released(struct wined3d_device *device, struct wined3d_resource *resource)
{
    enum wined3d_resource_type type = resource->type;
    struct wined3d_rendertarget_view *rtv;
    unsigned int i;

    TRACE("device %p, resource %p, type %s.\n", device, resource, debug_d3dresourcetype(type));

    if (device->d3d_initialized)
    {
        for (i = 0; i < device->adapter->gl_info.limits.buffers; ++i)
        {
            if ((rtv = device->fb.render_targets[i]) && rtv->resource == resource)
                ERR("Resource %p is still in use as render target %u.\n", resource, i);
        }

        if ((rtv = device->fb.depth_stencil) && rtv->resource == resource)
            ERR("Resource %p is still in use as depth/stencil buffer.\n", resource);
    }

    switch (type)
    {
        case WINED3D_RTYPE_TEXTURE_1D:
        case WINED3D_RTYPE_TEXTURE_2D:
        case WINED3D_RTYPE_TEXTURE_3D:
            for (i = 0; i < MAX_COMBINED_SAMPLERS; ++i)
            {
                struct wined3d_texture *texture = texture_from_resource(resource);

                if (device->state.textures[i] == texture)
                {
                    ERR("Texture %p is still in use, stage %u.\n", texture, i);
                    device->state.textures[i] = NULL;
                }

                if (device->recording && device->update_state->textures[i] == texture)
                {
                    ERR("Texture %p is still in use by recording stateblock %p, stage %u.\n",
                            texture, device->recording, i);
                    device->update_state->textures[i] = NULL;
                }
            }
            break;

        case WINED3D_RTYPE_BUFFER:
            {
                struct wined3d_buffer *buffer = buffer_from_resource(resource);

                for (i = 0; i < MAX_STREAMS; ++i)
                {
                    if (device->state.streams[i].buffer == buffer)
                    {
                        ERR("Buffer %p is still in use, stream %u.\n", buffer, i);
                        device->state.streams[i].buffer = NULL;
                    }

                    if (device->recording && device->update_state->streams[i].buffer == buffer)
                    {
                        ERR("Buffer %p is still in use by stateblock %p, stream %u.\n",
                                buffer, device->recording, i);
                        device->update_state->streams[i].buffer = NULL;
                    }
                }

                if (device->state.index_buffer == buffer)
                {
                    ERR("Buffer %p is still in use as index buffer.\n", buffer);
                    device->state.index_buffer =  NULL;
                }

                if (device->recording && device->update_state->index_buffer == buffer)
                {
                    ERR("Buffer %p is still in use by stateblock %p as index buffer.\n",
                            buffer, device->recording);
                    device->update_state->index_buffer =  NULL;
                }
            }
            break;

        default:
            break;
    }

    /* Remove the resource from the resourceStore */
    device_resource_remove(device, resource);

    TRACE("Resource released.\n");
}

static int wined3d_sampler_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct wined3d_sampler *sampler = WINE_RB_ENTRY_VALUE(entry, struct wined3d_sampler, entry);

    return memcmp(&sampler->desc, key, sizeof(sampler->desc));
}

HRESULT device_init(struct wined3d_device *device, struct wined3d *wined3d,
        UINT adapter_idx, enum wined3d_device_type device_type, HWND focus_window, DWORD flags,
        BYTE surface_alignment, struct wined3d_device_parent *device_parent)
{
    struct wined3d_adapter *adapter = &wined3d->adapters[adapter_idx];
    const struct fragment_pipeline *fragment_pipeline;
    const struct wined3d_vertex_pipe_ops *vertex_pipeline;
    unsigned int i;
    HRESULT hr;

    device->ref = 1;
    device->wined3d = wined3d;
    wined3d_incref(device->wined3d);
    device->adapter = wined3d->adapter_count ? adapter : NULL;
    device->device_parent = device_parent;
    list_init(&device->resources);
    list_init(&device->shaders);
    device->surface_alignment = surface_alignment;

    /* Save the creation parameters. */
    device->create_parms.adapter_idx = adapter_idx;
    device->create_parms.device_type = device_type;
    device->create_parms.focus_window = focus_window;
    device->create_parms.flags = flags;

    device->shader_backend = adapter->shader_backend;

    vertex_pipeline = adapter->vertex_pipe;

    fragment_pipeline = adapter->fragment_pipe;

    wine_rb_init(&device->samplers, wined3d_sampler_compare);

    if (vertex_pipeline->vp_states && fragment_pipeline->states
            && FAILED(hr = compile_state_table(device->StateTable, device->multistate_funcs,
            &adapter->gl_info, &adapter->d3d_info, vertex_pipeline,
            fragment_pipeline, misc_state_template)))
    {
        ERR("Failed to compile state table, hr %#x.\n", hr);
        wine_rb_destroy(&device->samplers, NULL, NULL);
        wined3d_decref(device->wined3d);
        return hr;
    }

    state_init(&device->state, &device->fb, &adapter->gl_info,
            &adapter->d3d_info, WINED3D_STATE_INIT_DEFAULT);
    device->update_state = &device->state;

    if (!(device->cs = wined3d_cs_create(device)))
    {
        WARN("Failed to create command stream.\n");
        state_cleanup(&device->state);
        hr = E_FAIL;
        goto err;
    }

    return WINED3D_OK;

err:
    for (i = 0; i < ARRAY_SIZE(device->multistate_funcs); ++i)
    {
        heap_free(device->multistate_funcs[i]);
    }
    wine_rb_destroy(&device->samplers, NULL, NULL);
    wined3d_decref(device->wined3d);
    return hr;
}

void device_invalidate_state(const struct wined3d_device *device, DWORD state)
{
    DWORD rep = device->StateTable[state].representative;
    struct wined3d_context *context;
    DWORD idx;
    BYTE shift;
    UINT i;

    wined3d_from_cs(device->cs);

    if (STATE_IS_COMPUTE(state))
    {
        for (i = 0; i < device->context_count; ++i)
            context_invalidate_compute_state(device->contexts[i], state);
        return;
    }

    for (i = 0; i < device->context_count; ++i)
    {
        context = device->contexts[i];
        if(isStateDirty(context, rep)) continue;

        context->dirtyArray[context->numDirtyEntries++] = rep;
        idx = rep / (sizeof(*context->isStateDirty) * CHAR_BIT);
        shift = rep & ((sizeof(*context->isStateDirty) * CHAR_BIT) - 1);
        context->isStateDirty[idx] |= (1u << shift);
    }
}

LRESULT device_process_message(struct wined3d_device *device, HWND window, BOOL unicode,
        UINT message, WPARAM wparam, LPARAM lparam, WNDPROC proc)
{
    if (device->filter_messages && message != WM_DISPLAYCHANGE)
    {
        TRACE("Filtering message: window %p, message %#x, wparam %#lx, lparam %#lx.\n",
                window, message, wparam, lparam);
        if (unicode)
            return DefWindowProcW(window, message, wparam, lparam);
        else
            return DefWindowProcA(window, message, wparam, lparam);
    }

    if (message == WM_DESTROY)
    {
        TRACE("unregister window %p.\n", window);
        wined3d_unregister_window(window);

        if (InterlockedCompareExchangePointer((void **)&device->focus_window, NULL, window) != window)
            ERR("Window %p is not the focus window for device %p.\n", window, device);
    }
    else if (message == WM_DISPLAYCHANGE)
    {
        device->device_parent->ops->mode_changed(device->device_parent);
    }
    else if (message == WM_ACTIVATEAPP)
    {
        UINT i;

        for (i = 0; i < device->swapchain_count; i++)
            wined3d_swapchain_activate(device->swapchains[i], wparam);

        device->device_parent->ops->activate(device->device_parent, wparam);
    }
    else if (message == WM_SYSCOMMAND)
    {
        if (wparam == SC_RESTORE && device->wined3d->flags & WINED3D_HANDLE_RESTORE)
        {
            if (unicode)
                DefWindowProcW(window, message, wparam, lparam);
            else
                DefWindowProcA(window, message, wparam, lparam);
        }
    }

    if (unicode)
        return CallWindowProcW(proc, window, message, wparam, lparam);
    else
        return CallWindowProcA(proc, window, message, wparam, lparam);
}
#if defined(STAGING_CSMT)

/* Context activation is done by the caller */
struct wined3d_gl_bo *wined3d_device_get_bo(struct wined3d_device *device, UINT size, GLenum gl_usage,
        GLenum type_hint, struct wined3d_context *context)
{
    struct wined3d_gl_bo *ret;
    const struct wined3d_gl_info *gl_info;

    TRACE("device %p, size %u, gl_usage %u, type_hint %u\n", device, size, gl_usage,
            type_hint);

    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ret));
    if(!ret)
        return NULL;
    ret->type_hint = type_hint;
    ret->size = size;
    ret->usage = gl_usage;

    gl_info = context->gl_info;

    GL_EXTCALL(glGenBuffers(1, &ret->name));
    if (type_hint == GL_ELEMENT_ARRAY_BUFFER)
        context_invalidate_state(context, STATE_INDEXBUFFER);
    GL_EXTCALL(glBindBuffer(type_hint, ret->name));
    GL_EXTCALL(glBufferData(type_hint, size, NULL, gl_usage));
    GL_EXTCALL(glBindBuffer(type_hint, 0));
    checkGLcall("Create buffer object");

    TRACE("Successfully created and set up buffer %u\n", ret->name);
    return ret;
}

/* Context activation is done by the caller */
static void wined3d_device_destroy_bo(struct wined3d_device *device, const struct wined3d_context *context,
        struct wined3d_gl_bo *bo)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    TRACE("device %p, bo %p, GL bo %u\n", device, bo, bo->name);

    GL_EXTCALL(glDeleteBuffers(1, &bo->name));
    checkGLcall("glDeleteBuffers");

    HeapFree(GetProcessHeap(), 0, bo);
}

/* Context activation is done by the caller */
void wined3d_device_release_bo(struct wined3d_device *device, struct wined3d_gl_bo *bo,
        const struct wined3d_context *context)
{
    TRACE("device %p, bo %p, GL bo %u\n", device, bo, bo->name);

    wined3d_device_destroy_bo(device, context, bo);
}
#endif /* STAGING_CSMT */
