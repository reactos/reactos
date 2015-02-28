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

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

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
    switch(primitive_type)
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

        default:
            FIXME("Unhandled primitive type %s\n", debug_d3dprimitivetype(primitive_type));
        case WINED3D_PT_UNDEFINED:
            return ~0u;
    }
}

static enum wined3d_primitive_type d3d_primitive_type_from_gl(GLenum primitive_type)
{
    switch(primitive_type)
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

        default:
            FIXME("Unhandled primitive type %s\n", debug_d3dprimitivetype(primitive_type));
        case ~0u:
            return WINED3D_PT_UNDEFINED;
    }
}

BOOL device_context_add(struct wined3d_device *device, struct wined3d_context *context)
{
    struct wined3d_context **new_array;

    TRACE("Adding context %p.\n", context);

    if (!device->contexts) new_array = HeapAlloc(GetProcessHeap(), 0, sizeof(*new_array));
    else new_array = HeapReAlloc(GetProcessHeap(), 0, device->contexts,
            sizeof(*new_array) * (device->context_count + 1));

    if (!new_array)
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
        HeapFree(GetProcessHeap(), 0, device->contexts);
        device->contexts = NULL;
        return;
    }

    memmove(&device->contexts[i], &device->contexts[i + 1], (device->context_count - i) * sizeof(*device->contexts));
    new_array = HeapReAlloc(GetProcessHeap(), 0, device->contexts, device->context_count * sizeof(*device->contexts));
    if (!new_array)
    {
        ERR("Failed to shrink context array. Oh well.\n");
        return;
    }

    device->contexts = new_array;
}

void device_switch_onscreen_ds(struct wined3d_device *device,
        struct wined3d_context *context, struct wined3d_surface *depth_stencil)
{
    if (device->onscreen_depth_stencil)
    {
        surface_load_ds_location(device->onscreen_depth_stencil, context, WINED3D_LOCATION_TEXTURE_RGB);

        surface_modify_ds_location(device->onscreen_depth_stencil, WINED3D_LOCATION_TEXTURE_RGB,
                device->onscreen_depth_stencil->ds_current_size.cx,
                device->onscreen_depth_stencil->ds_current_size.cy);
        wined3d_surface_decref(device->onscreen_depth_stencil);
    }
    device->onscreen_depth_stencil = depth_stencil;
    wined3d_surface_incref(device->onscreen_depth_stencil);
}

static BOOL is_full_clear(const struct wined3d_surface *target, const RECT *draw_rect, const RECT *clear_rect)
{
    /* partial draw rect */
    if (draw_rect->left || draw_rect->top
            || draw_rect->right < target->resource.width
            || draw_rect->bottom < target->resource.height)
        return FALSE;

    /* partial clear rect */
    if (clear_rect && (clear_rect->left > 0 || clear_rect->top > 0
            || clear_rect->right < target->resource.width
            || clear_rect->bottom < target->resource.height))
        return FALSE;

    return TRUE;
}

static void prepare_ds_clear(struct wined3d_surface *ds, struct wined3d_context *context,
        DWORD location, const RECT *draw_rect, UINT rect_count, const RECT *clear_rect, RECT *out_rect)
{
    RECT current_rect, r;

    if (ds->locations & WINED3D_LOCATION_DISCARDED)
    {
        /* Depth buffer was discarded, make it entirely current in its new location since
         * there is no other place where we would get data anyway. */
        SetRect(out_rect, 0, 0, ds->resource.width, ds->resource.height);
        return;
    }

    if (ds->locations & location)
        SetRect(&current_rect, 0, 0,
                ds->ds_current_size.cx,
                ds->ds_current_size.cy);
    else
        SetRectEmpty(&current_rect);

    IntersectRect(&r, draw_rect, &current_rect);
    if (EqualRect(&r, draw_rect))
    {
        /* current_rect ⊇ draw_rect, modify only. */
        SetRect(out_rect, 0, 0, ds->ds_current_size.cx, ds->ds_current_size.cy);
        return;
    }

    if (EqualRect(&r, &current_rect))
    {
        /* draw_rect ⊇ current_rect, test if we're doing a full clear. */

        if (!clear_rect)
        {
            /* Full clear, modify only. */
            *out_rect = *draw_rect;
            return;
        }

        IntersectRect(&r, draw_rect, clear_rect);
        if (EqualRect(&r, draw_rect))
        {
            /* clear_rect ⊇ draw_rect, modify only. */
            *out_rect = *draw_rect;
            return;
        }
    }

    /* Full load. */
    surface_load_ds_location(ds, context, location);
    SetRect(out_rect, 0, 0, ds->ds_current_size.cx, ds->ds_current_size.cy);
}

void device_clear_render_targets(struct wined3d_device *device, UINT rt_count, const struct wined3d_fb_state *fb,
        UINT rect_count, const RECT *rects, const RECT *draw_rect, DWORD flags, const struct wined3d_color *color,
        float depth, DWORD stencil)
{
    struct wined3d_surface *target = rt_count ? wined3d_rendertarget_view_get_surface(fb->render_targets[0]) : NULL;
    struct wined3d_surface *depth_stencil = fb->depth_stencil
            ? wined3d_rendertarget_view_get_surface(fb->depth_stencil) : NULL;
    const RECT *clear_rect = (rect_count > 0 && rects) ? (const RECT *)rects : NULL;
    const struct wined3d_gl_info *gl_info;
    UINT drawable_width, drawable_height;
    struct wined3d_context *context;
    GLbitfield clear_mask = 0;
    BOOL render_offscreen;
    unsigned int i;
    RECT ds_rect;

    /* When we're clearing parts of the drawable, make sure that the target surface is well up to date in the
     * drawable. After the clear we'll mark the drawable up to date, so we have to make sure that this is true
     * for the cleared parts, and the untouched parts.
     *
     * If we're clearing the whole target there is no need to copy it into the drawable, it will be overwritten
     * anyway. If we're not clearing the color buffer we don't have to copy either since we're not going to set
     * the drawable up to date. We have to check all settings that limit the clear area though. Do not bother
     * checking all this if the dest surface is in the drawable anyway. */
    if (flags & WINED3DCLEAR_TARGET && !is_full_clear(target, draw_rect, clear_rect))
    {
        for (i = 0; i < rt_count; ++i)
        {
            struct wined3d_surface *rt = wined3d_rendertarget_view_get_surface(fb->render_targets[i]);
            if (rt)
                surface_load_location(rt, rt->container->resource.draw_binding);
        }
    }

    context = context_acquire(device, target);
    if (!context->valid)
    {
        context_release(context);
        WARN("Invalid context, skipping clear.\n");
        return;
    }
    gl_info = context->gl_info;

    if (target)
    {
        render_offscreen = context->render_offscreen;
        surface_get_drawable_size(target, context, &drawable_width, &drawable_height);
    }
    else
    {
        render_offscreen = TRUE;
        drawable_width = depth_stencil->pow2Width;
        drawable_height = depth_stencil->pow2Height;
    }

    if (flags & WINED3DCLEAR_ZBUFFER)
    {
        DWORD location = render_offscreen ? fb->depth_stencil->resource->draw_binding : WINED3D_LOCATION_DRAWABLE;

        if (!render_offscreen && depth_stencil != device->onscreen_depth_stencil)
            device_switch_onscreen_ds(device, context, depth_stencil);
        prepare_ds_clear(depth_stencil, context, location,
                draw_rect, rect_count, clear_rect, &ds_rect);
    }

    if (!context_apply_clear_state(context, device, rt_count, fb))
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
        DWORD location = render_offscreen ? fb->depth_stencil->resource->draw_binding : WINED3D_LOCATION_DRAWABLE;

        surface_modify_ds_location(depth_stencil, location, ds_rect.right, ds_rect.bottom);

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
            struct wined3d_surface *rt = wined3d_rendertarget_view_get_surface(fb->render_targets[i]);

            if (rt)
            {
                surface_validate_location(rt, rt->container->resource.draw_binding);
                surface_invalidate_location(rt, ~rt->container->resource.draw_binding);
            }
        }

        gl_info->gl_ops.gl.p_glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITEENABLE));
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITEENABLE1));
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITEENABLE2));
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITEENABLE3));
        gl_info->gl_ops.gl.p_glClearColor(color->r, color->g, color->b, color->a);
        checkGLcall("glClearColor");
        clear_mask = clear_mask | GL_COLOR_BUFFER_BIT;
    }

    if (!clear_rect)
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
            FIXME("Something's still holding the recording stateblock.\n");
        device->recording = NULL;

        state_cleanup(&device->state);

        for (i = 0; i < sizeof(device->multistate_funcs) / sizeof(device->multistate_funcs[0]); ++i)
        {
            HeapFree(GetProcessHeap(), 0, device->multistate_funcs[i]);
            device->multistate_funcs[i] = NULL;
        }

        if (!list_empty(&device->resources))
        {
            struct wined3d_resource *resource;

            FIXME("Device released with resources still bound, acceptable but unexpected.\n");

            LIST_FOR_EACH_ENTRY(resource, &device->resources, struct wined3d_resource, resource_list_entry)
            {
                FIXME("Leftover resource %p with type %s (%#x).\n",
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
        HeapFree(GetProcessHeap(), 0, device);
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
    struct wined3d_surface *surface;
    HBITMAP hbm;
    BITMAP bm;
    HRESULT hr;
    HDC dcb = NULL, dcs = NULL;

    hbm = LoadImageA(NULL, filename, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
    if(hbm)
    {
        GetObjectA(hbm, sizeof(BITMAP), &bm);
        dcb = CreateCompatibleDC(NULL);
        if(!dcb) goto out;
        SelectObject(dcb, hbm);
    }
    else
    {
        /* Create a 32x32 white surface to indicate that wined3d is used, but the specified image
         * couldn't be loaded
         */
        memset(&bm, 0, sizeof(bm));
        bm.bmWidth = 32;
        bm.bmHeight = 32;
    }

    desc.resource_type = WINED3D_RTYPE_TEXTURE;
    desc.format = WINED3DFMT_B5G6R5_UNORM;
    desc.multisample_type = WINED3D_MULTISAMPLE_NONE;
    desc.multisample_quality = 0;
    desc.usage = WINED3DUSAGE_DYNAMIC;
    desc.pool = WINED3D_POOL_DEFAULT;
    desc.width = bm.bmWidth;
    desc.height = bm.bmHeight;
    desc.depth = 1;
    desc.size = 0;
    if (FAILED(hr = wined3d_texture_create(device, &desc, 1, WINED3D_SURFACE_MAPPABLE,
            NULL, NULL, &wined3d_null_parent_ops, &device->logo_texture)))
    {
        ERR("Wine logo requested, but failed to create texture, hr %#x.\n", hr);
        goto out;
    }
    surface = surface_from_resource(wined3d_texture_get_sub_resource(device->logo_texture, 0));

    if (dcb)
    {
        if (FAILED(hr = wined3d_surface_getdc(surface, &dcs)))
            goto out;
        BitBlt(dcs, 0, 0, bm.bmWidth, bm.bmHeight, dcb, 0, 0, SRCCOPY);
        wined3d_surface_releasedc(surface, dcs);

        color_key.color_space_low_value = 0;
        color_key.color_space_high_value = 0;
        wined3d_texture_set_color_key(device->logo_texture, WINED3D_CKEY_SRC_BLT, &color_key);
    }
    else
    {
        const RECT rect = {0, 0, surface->resource.width, surface->resource.height};
        const struct wined3d_color c = {1.0f, 1.0f, 1.0f, 1.0f};

        /* Fill the surface with a white color to show that wined3d is there */
        surface_color_fill(surface, &rect, &c);
    }

out:
    if (dcb) DeleteDC(dcb);
    if (hbm) DeleteObject(hbm);
}

/* Context activation is done by the caller. */
static void create_dummy_textures(struct wined3d_device *device, struct wined3d_context *context)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    unsigned int i, j, count;
    /* Under DirectX you can sample even if no texture is bound, whereas
     * OpenGL will only allow that when a valid texture is bound.
     * We emulate this by creating dummy textures and binding them
     * to each texture stage when the currently set D3D texture is NULL. */

    count = min(MAX_COMBINED_SAMPLERS, gl_info->limits.combined_samplers);
    for (i = 0; i < count; ++i)
    {
        DWORD color = 0x000000ff;

        /* Make appropriate texture active */
        context_active_texture(context, gl_info, i);

        gl_info->gl_ops.gl.p_glGenTextures(1, &device->dummy_texture_2d[i]);
        checkGLcall("glGenTextures");
        TRACE("Dummy 2D texture %u given name %u.\n", i, device->dummy_texture_2d[i]);

        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, device->dummy_texture_2d[i]);
        checkGLcall("glBindTexture");

        gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0,
                GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color);
        checkGLcall("glTexImage2D");

        if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
        {
            gl_info->gl_ops.gl.p_glGenTextures(1, &device->dummy_texture_rect[i]);
            checkGLcall("glGenTextures");
            TRACE("Dummy rectangle texture %u given name %u.\n", i, device->dummy_texture_rect[i]);

            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_RECTANGLE_ARB, device->dummy_texture_rect[i]);
            checkGLcall("glBindTexture");

            gl_info->gl_ops.gl.p_glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA8, 1, 1, 0,
                    GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color);
            checkGLcall("glTexImage2D");
        }

        if (gl_info->supported[EXT_TEXTURE3D])
        {
            gl_info->gl_ops.gl.p_glGenTextures(1, &device->dummy_texture_3d[i]);
            checkGLcall("glGenTextures");
            TRACE("Dummy 3D texture %u given name %u.\n", i, device->dummy_texture_3d[i]);

            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_3D, device->dummy_texture_3d[i]);
            checkGLcall("glBindTexture");

            GL_EXTCALL(glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, 1, 1, 1, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color));
            checkGLcall("glTexImage3D");
        }

        if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
        {
            gl_info->gl_ops.gl.p_glGenTextures(1, &device->dummy_texture_cube[i]);
            checkGLcall("glGenTextures");
            TRACE("Dummy cube texture %u given name %u.\n", i, device->dummy_texture_cube[i]);

            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_CUBE_MAP, device->dummy_texture_cube[i]);
            checkGLcall("glBindTexture");

            for (j = GL_TEXTURE_CUBE_MAP_POSITIVE_X; j <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z; ++j)
            {
                gl_info->gl_ops.gl.p_glTexImage2D(j, 0, GL_RGBA8, 1, 1, 0,
                        GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, &color);
                checkGLcall("glTexImage2D");
            }
        }
    }
}

/* Context activation is done by the caller. */
static void destroy_dummy_textures(struct wined3d_device *device, const struct wined3d_gl_info *gl_info)
{
    unsigned int count = min(MAX_COMBINED_SAMPLERS, gl_info->limits.combined_samplers);

    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
    {
        gl_info->gl_ops.gl.p_glDeleteTextures(count, device->dummy_texture_cube);
        checkGLcall("glDeleteTextures(count, device->dummy_texture_cube)");
    }

    if (gl_info->supported[EXT_TEXTURE3D])
    {
        gl_info->gl_ops.gl.p_glDeleteTextures(count, device->dummy_texture_3d);
        checkGLcall("glDeleteTextures(count, device->dummy_texture_3d)");
    }

    if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
    {
        gl_info->gl_ops.gl.p_glDeleteTextures(count, device->dummy_texture_rect);
        checkGLcall("glDeleteTextures(count, device->dummy_texture_rect)");
    }

    gl_info->gl_ops.gl.p_glDeleteTextures(count, device->dummy_texture_2d);
    checkGLcall("glDeleteTextures(count, device->dummy_texture_2d)");

    memset(device->dummy_texture_cube, 0, count * sizeof(*device->dummy_texture_cube));
    memset(device->dummy_texture_3d, 0, count * sizeof(*device->dummy_texture_3d));
    memset(device->dummy_texture_rect, 0, count * sizeof(*device->dummy_texture_rect));
    memset(device->dummy_texture_2d, 0, count * sizeof(*device->dummy_texture_2d));
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

void CDECL wined3d_device_restore_fullscreen_window(struct wined3d_device *device, HWND window)
{
    BOOL filter_messages;
    LONG style, exstyle;

    if (!device->style && !device->exStyle) return;

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
    SetWindowPos(window, 0, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

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
    BOOL ds_enable = !!swapchain->desc.enable_auto_depth_stencil;
    unsigned int i;

    if (device->fb.render_targets)
    {
        for (i = 0; i < device->adapter->gl_info.limits.buffers; ++i)
        {
            wined3d_device_set_rendertarget_view(device, i, NULL, FALSE);
        }
        if (device->back_buffer_view)
            wined3d_device_set_rendertarget_view(device, 0, device->back_buffer_view, TRUE);
    }

    wined3d_device_set_depth_stencil_view(device, ds_enable ? device->auto_depth_stencil_view : NULL);
    wined3d_device_set_render_state(device, WINED3D_RS_ZENABLE, ds_enable);
}

HRESULT CDECL wined3d_device_init_3d(struct wined3d_device *device,
        struct wined3d_swapchain_desc *swapchain_desc)
{
    static const struct wined3d_color black = {0.0f, 0.0f, 0.0f, 0.0f};
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    struct wined3d_swapchain *swapchain = NULL;
    struct wined3d_context *context;
    DWORD clear_flags = 0;
    HRESULT hr;

    TRACE("device %p, swapchain_desc %p.\n", device, swapchain_desc);

    if (device->d3d_initialized)
        return WINED3DERR_INVALIDCALL;
    if (device->wined3d->flags & WINED3D_NO3D)
        return WINED3DERR_INVALIDCALL;

    device->fb.render_targets = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(*device->fb.render_targets) * gl_info->limits.buffers);

    if (FAILED(hr = device->shader_backend->shader_alloc_private(device,
            device->adapter->vertex_pipe, device->adapter->fragment_pipe)))
    {
        TRACE("Shader private data couldn't be allocated\n");
        goto err_out;
    }
    if (FAILED(hr = device->blitter->alloc_private(device)))
    {
        TRACE("Blitter private data couldn't be allocated\n");
        goto err_out;
    }

    /* Setup the implicit swapchain. This also initializes a context. */
    TRACE("Creating implicit swapchain\n");
    hr = device->device_parent->ops->create_swapchain(device->device_parent,
            swapchain_desc, &swapchain);
    if (FAILED(hr))
    {
        WARN("Failed to create implicit swapchain\n");
        goto err_out;
    }

    if (swapchain_desc->backbuffer_count && FAILED(hr = wined3d_rendertarget_view_create_from_surface(
            surface_from_resource(wined3d_texture_get_sub_resource(swapchain->back_buffers[0], 0)),
            NULL, &wined3d_null_parent_ops, &device->back_buffer_view)))
    {
        ERR("Failed to create rendertarget view, hr %#x.\n", hr);
        goto err_out;
    }

    device->swapchain_count = 1;
    device->swapchains = HeapAlloc(GetProcessHeap(), 0, device->swapchain_count * sizeof(*device->swapchains));
    if (!device->swapchains)
    {
        ERR("Out of memory!\n");
        goto err_out;
    }
    device->swapchains[0] = swapchain;
    device_init_swapchain_state(device, swapchain);

    context = context_acquire(device,
            surface_from_resource(wined3d_texture_get_sub_resource(swapchain->front_buffer, 0)));

    create_dummy_textures(device, context);

    device->contexts[0]->last_was_rhw = 0;

    switch (wined3d_settings.offscreen_rendering_mode)
    {
        case ORM_FBO:
            device->offscreenBuffer = GL_COLOR_ATTACHMENT0;
            break;

        case ORM_BACKBUFFER:
        {
            if (context_get_current()->aux_buffers > 0)
            {
                TRACE("Using auxiliary buffer for offscreen rendering\n");
                device->offscreenBuffer = GL_AUX0;
            }
            else
            {
                TRACE("Using back buffer for offscreen rendering\n");
                device->offscreenBuffer = GL_BACK;
            }
        }
    }

    TRACE("All defaults now set up, leaving 3D init.\n");

    context_release(context);

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
    HeapFree(GetProcessHeap(), 0, device->fb.render_targets);
    HeapFree(GetProcessHeap(), 0, device->swapchains);
    device->swapchain_count = 0;
    if (device->back_buffer_view)
        wined3d_rendertarget_view_decref(device->back_buffer_view);
    if (swapchain)
        wined3d_swapchain_decref(swapchain);
    if (device->blit_priv)
        device->blitter->free_private(device);
    if (device->shader_priv)
        device->shader_backend->shader_free_private(device);

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
    device->swapchains = HeapAlloc(GetProcessHeap(), 0, device->swapchain_count * sizeof(*device->swapchains));
    if (!device->swapchains)
    {
        ERR("Out of memory!\n");
        goto err_out;
    }
    device->swapchains[0] = swapchain;
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
    struct wined3d_resource *resource, *cursor;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    struct wined3d_surface *surface;
    UINT i;

    TRACE("device %p.\n", device);

    if (!device->d3d_initialized)
        return WINED3DERR_INVALIDCALL;

    /* I don't think that the interface guarantees that the device is destroyed from the same thread
     * it was created. Thus make sure a context is active for the glDelete* calls
     */
    context = context_acquire(device, NULL);
    gl_info = context->gl_info;

    if (device->logo_texture)
        wined3d_texture_decref(device->logo_texture);
    if (device->cursor_texture)
        wined3d_texture_decref(device->cursor_texture);

    state_unbind_resources(&device->state);

    /* Unload resources */
    LIST_FOR_EACH_ENTRY_SAFE(resource, cursor, &device->resources, struct wined3d_resource, resource_list_entry)
    {
        TRACE("Unloading resource %p.\n", resource);

        resource->resource_ops->resource_unload(resource);
    }

    wine_rb_clear(&device->samplers, device_free_sampler, NULL);

    /* Destroy the depth blt resources, they will be invalid after the reset. Also free shader
     * private data, it might contain opengl pointers
     */
    if (device->depth_blt_texture)
    {
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &device->depth_blt_texture);
        device->depth_blt_texture = 0;
    }

    /* Destroy the shader backend. Note that this has to happen after all shaders are destroyed. */
    device->blitter->free_private(device);
    device->shader_backend->shader_free_private(device);
    destroy_dummy_textures(device, gl_info);

    /* Release the buffers (with sanity checks)*/
    if (device->onscreen_depth_stencil)
    {
        surface = device->onscreen_depth_stencil;
        device->onscreen_depth_stencil = NULL;
        wined3d_surface_decref(surface);
    }

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

    for (i = 0; i < gl_info->limits.buffers; ++i)
    {
        wined3d_device_set_rendertarget_view(device, i, NULL, FALSE);
    }
    if (device->back_buffer_view)
    {
        wined3d_rendertarget_view_decref(device->back_buffer_view);
        device->back_buffer_view = NULL;
    }

    context_release(context);

    for (i = 0; i < device->swapchain_count; ++i)
    {
        TRACE("Releasing the implicit swapchain %u.\n", i);
        if (wined3d_swapchain_decref(device->swapchains[i]))
            FIXME("Something's still holding the implicit swapchain.\n");
    }

    HeapFree(GetProcessHeap(), 0, device->swapchains);
    device->swapchains = NULL;
    device->swapchain_count = 0;

    HeapFree(GetProcessHeap(), 0, device->fb.render_targets);
    device->fb.render_targets = NULL;

    device->d3d_initialized = FALSE;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_uninit_gdi(struct wined3d_device *device)
{
    unsigned int i;

    for (i = 0; i < device->swapchain_count; ++i)
    {
        TRACE("Releasing the implicit swapchain %u.\n", i);
        if (wined3d_swapchain_decref(device->swapchains[i]))
            FIXME("Something's still holding the implicit swapchain.\n");
    }

    HeapFree(GetProcessHeap(), 0, device->swapchains);
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
    TRACE("device %p.\n", device);

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

    if (idx >= MAX_STREAM_OUT)
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

    if (idx >= MAX_STREAM_OUT)
    {
        WARN("Invalid stream output %u.\n", idx);
        return NULL;
    }

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
        device->recording->changed.streamSource |= 1 << stream_idx;

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
    }

    if (buffer)
        wined3d_buffer_incref(buffer);
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
        device->recording->changed.streamFreq |= 1 << stream_idx;
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
    TRACE("%.8e %.8e %.8e %.8e\n", matrix->u.s._11, matrix->u.s._12, matrix->u.s._13, matrix->u.s._14);
    TRACE("%.8e %.8e %.8e %.8e\n", matrix->u.s._21, matrix->u.s._22, matrix->u.s._23, matrix->u.s._24);
    TRACE("%.8e %.8e %.8e %.8e\n", matrix->u.s._31, matrix->u.s._32, matrix->u.s._33, matrix->u.s._34);
    TRACE("%.8e %.8e %.8e %.8e\n", matrix->u.s._41, matrix->u.s._42, matrix->u.s._43, matrix->u.s._44);

    /* Handle recording of state blocks. */
    if (device->recording)
    {
        TRACE("Recording... not performing anything.\n");
        device->recording->changed.transform[d3dts >> 5] |= 1 << (d3dts & 0x1f);
        device->update_state->transforms[d3dts] = *matrix;
        return;
    }

    /* If the new matrix is the same as the current one,
     * we cut off any further processing. this seems to be a reasonable
     * optimization because as was noticed, some apps (warcraft3 for example)
     * tend towards setting the same matrix repeatedly for some reason.
     *
     * From here on we assume that the new matrix is different, wherever it matters. */
    if (!memcmp(&device->state.transforms[d3dts].u.m[0][0], matrix, sizeof(*matrix)))
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
    struct list *e;
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
        case WINED3D_LIGHT_PARALLELPOINT:
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
            /* Ignores attenuation */
            break;

        default:
        WARN("Light type out of range, returning WINED3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }

    LIST_FOR_EACH(e, &device->update_state->light_map[hash_idx])
    {
        object = LIST_ENTRY(e, struct wined3d_light_info, entry);
        if (object->OriginalIndex == light_idx)
            break;
        object = NULL;
    }

    if (!object)
    {
        TRACE("Adding new light\n");
        object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
        if (!object)
            return E_OUTOFMEMORY;

        list_add_head(&device->update_state->light_map[hash_idx], &object->entry);
        object->glIndex = -1;
        object->OriginalIndex = light_idx;
    }

    /* Initialize the object. */
    TRACE("Light %d setting to type %d, Diffuse(%f,%f,%f,%f), Specular(%f,%f,%f,%f), Ambient(%f,%f,%f,%f)\n",
            light_idx, light->type,
            light->diffuse.r, light->diffuse.g, light->diffuse.b, light->diffuse.a,
            light->specular.r, light->specular.g, light->specular.b, light->specular.a,
            light->ambient.r, light->ambient.g, light->ambient.b, light->ambient.a);
    TRACE("... Pos(%f,%f,%f), Dir(%f,%f,%f)\n", light->position.x, light->position.y, light->position.z,
            light->direction.x, light->direction.y, light->direction.z);
    TRACE("... Range(%f), Falloff(%f), Theta(%f), Phi(%f)\n",
            light->range, light->falloff, light->theta, light->phi);

    /* Update the live definitions if the light is currently assigned a glIndex. */
    if (object->glIndex != -1 && !device->recording)
    {
        if (object->OriginalParms.type != light->type)
            device_invalidate_state(device, STATE_LIGHT_TYPE);
        device_invalidate_state(device, STATE_ACTIVELIGHT(object->glIndex));
    }

    /* Save away the information. */
    object->OriginalParms = *light;

    switch (light->type)
    {
        case WINED3D_LIGHT_POINT:
            /* Position */
            object->lightPosn[0] = light->position.x;
            object->lightPosn[1] = light->position.y;
            object->lightPosn[2] = light->position.z;
            object->lightPosn[3] = 1.0f;
            object->cutoff = 180.0f;
            /* FIXME: Range */
            break;

        case WINED3D_LIGHT_DIRECTIONAL:
            /* Direction */
            object->lightPosn[0] = -light->direction.x;
            object->lightPosn[1] = -light->direction.y;
            object->lightPosn[2] = -light->direction.z;
            object->lightPosn[3] = 0.0f;
            object->exponent = 0.0f;
            object->cutoff = 180.0f;
            break;

        case WINED3D_LIGHT_SPOT:
            /* Position */
            object->lightPosn[0] = light->position.x;
            object->lightPosn[1] = light->position.y;
            object->lightPosn[2] = light->position.z;
            object->lightPosn[3] = 1.0f;

            /* Direction */
            object->lightDirn[0] = light->direction.x;
            object->lightDirn[1] = light->direction.y;
            object->lightDirn[2] = light->direction.z;
            object->lightDirn[3] = 1.0f;

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

        default:
            FIXME("Unrecognized light type %#x.\n", light->type);
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_light(const struct wined3d_device *device,
        UINT light_idx, struct wined3d_light *light)
{
    UINT hash_idx = LIGHTMAP_HASHFUNC(light_idx);
    struct wined3d_light_info *light_info = NULL;
    struct list *e;

    TRACE("device %p, light_idx %u, light %p.\n", device, light_idx, light);

    LIST_FOR_EACH(e, &device->state.light_map[hash_idx])
    {
        light_info = LIST_ENTRY(e, struct wined3d_light_info, entry);
        if (light_info->OriginalIndex == light_idx)
            break;
        light_info = NULL;
    }

    if (!light_info)
    {
        TRACE("Light information requested but light not defined\n");
        return WINED3DERR_INVALIDCALL;
    }

    *light = light_info->OriginalParms;
    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_light_enable(struct wined3d_device *device, UINT light_idx, BOOL enable)
{
    UINT hash_idx = LIGHTMAP_HASHFUNC(light_idx);
    struct wined3d_light_info *light_info = NULL;
    struct list *e;

    TRACE("device %p, light_idx %u, enable %#x.\n", device, light_idx, enable);

    LIST_FOR_EACH(e, &device->update_state->light_map[hash_idx])
    {
        light_info = LIST_ENTRY(e, struct wined3d_light_info, entry);
        if (light_info->OriginalIndex == light_idx)
            break;
        light_info = NULL;
    }
    TRACE("Found light %p.\n", light_info);

    /* Special case - enabling an undefined light creates one with a strict set of parameters. */
    if (!light_info)
    {
        TRACE("Light enabled requested but light not defined, so defining one!\n");
        wined3d_device_set_light(device, light_idx, &WINED3D_default_light);

        /* Search for it again! Should be fairly quick as near head of list. */
        LIST_FOR_EACH(e, &device->update_state->light_map[hash_idx])
        {
            light_info = LIST_ENTRY(e, struct wined3d_light_info, entry);
            if (light_info->OriginalIndex == light_idx)
                break;
            light_info = NULL;
        }
        if (!light_info)
        {
            FIXME("Adding default lights has failed dismally\n");
            return WINED3DERR_INVALIDCALL;
        }
    }

    if (!enable)
    {
        if (light_info->glIndex != -1)
        {
            if (!device->recording)
            {
                device_invalidate_state(device, STATE_LIGHT_TYPE);
                device_invalidate_state(device, STATE_ACTIVELIGHT(light_info->glIndex));
            }

            device->update_state->lights[light_info->glIndex] = NULL;
            light_info->glIndex = -1;
        }
        else
        {
            TRACE("Light already disabled, nothing to do\n");
        }
        light_info->enabled = FALSE;
    }
    else
    {
        light_info->enabled = TRUE;
        if (light_info->glIndex != -1)
        {
            TRACE("Nothing to do as light was enabled\n");
        }
        else
        {
            unsigned int i;
            const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
            /* Find a free GL light. */
            for (i = 0; i < gl_info->limits.lights; ++i)
            {
                if (!device->update_state->lights[i])
                {
                    device->update_state->lights[i] = light_info;
                    light_info->glIndex = i;
                    break;
                }
            }
            if (light_info->glIndex == -1)
            {
                /* Our tests show that Windows returns D3D_OK in this situation, even with
                 * D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE devices. This
                 * is consistent among ddraw, d3d8 and d3d9. GetLightEnable returns TRUE
                 * as well for those lights.
                 *
                 * TODO: Test how this affects rendering. */
                WARN("Too many concurrently active lights\n");
                return WINED3D_OK;
            }

            /* i == light_info->glIndex */
            if (!device->recording)
            {
                device_invalidate_state(device, STATE_LIGHT_TYPE);
                device_invalidate_state(device, STATE_ACTIVELIGHT(i));
            }
        }
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_light_enable(const struct wined3d_device *device, UINT light_idx, BOOL *enable)
{
    UINT hash_idx = LIGHTMAP_HASHFUNC(light_idx);
    struct wined3d_light_info *light_info = NULL;
    struct list *e;

    TRACE("device %p, light_idx %u, enable %p.\n", device, light_idx, enable);

    LIST_FOR_EACH(e, &device->state.light_map[hash_idx])
    {
        light_info = LIST_ENTRY(e, struct wined3d_light_info, entry);
        if (light_info->OriginalIndex == light_idx)
            break;
        light_info = NULL;
    }

    if (!light_info)
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

    /* Validate plane_idx. */
    if (plane_idx >= device->adapter->gl_info.limits.clipplanes)
    {
        TRACE("Application has requested clipplane this device doesn't support.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (device->recording)
        device->recording->changed.clipplane |= 1 << plane_idx;

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

    /* Validate plane_idx. */
    if (plane_idx >= device->adapter->gl_info.limits.clipplanes)
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

    TRACE("diffuse {%.8e, %.8e, %.8e, %.8e}\n",
            material->diffuse.r, material->diffuse.g,
            material->diffuse.b, material->diffuse.a);
    TRACE("ambient {%.8e, %.8e, %.8e, %.8e}\n",
            material->ambient.r, material->ambient.g,
            material->ambient.b, material->ambient.a);
    TRACE("specular {%.8e, %.8e, %.8e, %.8e}\n",
            material->specular.r, material->specular.g,
            material->specular.b, material->specular.a);
    TRACE("emissive {%.8e, %.8e, %.8e, %.8e}\n",
            material->emissive.r, material->emissive.g,
            material->emissive.b, material->emissive.a);
    TRACE("power %.8e.\n", material->power);
}

void CDECL wined3d_device_set_index_buffer(struct wined3d_device *device,
        struct wined3d_buffer *buffer, enum wined3d_format_id format_id)
{
    enum wined3d_format_id prev_format;
    struct wined3d_buffer *prev_buffer;

    TRACE("device %p, buffer %p, format %s.\n",
            device, buffer, debug_d3dformat(format_id));

    prev_buffer = device->update_state->index_buffer;
    prev_format = device->update_state->index_format;

    device->update_state->index_buffer = buffer;
    device->update_state->index_format = format_id;

    if (device->recording)
        device->recording->changed.indices = TRUE;

    if (prev_buffer == buffer && prev_format == format_id)
        return;

    if (buffer)
        wined3d_buffer_incref(buffer);
    if (!device->recording)
        wined3d_cs_emit_set_index_buffer(device->cs, buffer, format_id);
    if (prev_buffer)
        wined3d_buffer_decref(prev_buffer);
}

struct wined3d_buffer * CDECL wined3d_device_get_index_buffer(const struct wined3d_device *device,
        enum wined3d_format_id *format)
{
    TRACE("device %p, format %p.\n", device, format);

    *format = device->state.index_format;
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
    TRACE("x %u, y %u, w %u, h %u, min_z %.8e, max_z %.8e.\n",
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

static void resolve_depth_buffer(struct wined3d_state *state)
{
    struct wined3d_texture *texture = state->textures[0];
    struct wined3d_surface *depth_stencil, *surface;

    if (!texture || texture->resource.type != WINED3D_RTYPE_TEXTURE
            || !(texture->resource.format->flags & WINED3DFMT_FLAG_DEPTH))
        return;
    surface = surface_from_resource(texture->sub_resources[0]);
    if (!(depth_stencil = wined3d_rendertarget_view_get_surface(state->fb->depth_stencil)))
        return;

    wined3d_surface_blt(surface, NULL, depth_stencil, NULL, 0, NULL, WINED3D_TEXF_POINT);
}

void CDECL wined3d_device_set_render_state(struct wined3d_device *device,
        enum wined3d_render_state state, DWORD value)
{
    DWORD old_value = device->state.render_states[state];

    TRACE("device %p, state %s (%#x), value %#x.\n", device, debug_d3drenderstate(state), state, value);

    device->update_state->render_states[state] = value;

    /* Handle recording of state blocks. */
    if (device->recording)
    {
        TRACE("Recording... not performing anything.\n");
        device->recording->changed.renderState[state >> 5] |= 1 << (state & 0x1f);
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
        resolve_depth_buffer(&device->state);
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

    if (sampler_idx >= sizeof(device->state.sampler_states) / sizeof(*device->state.sampler_states))
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
        device->recording->changed.samplerState[sampler_idx] |= 1 << state;
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

    if (sampler_idx >= sizeof(device->state.sampler_states) / sizeof(*device->state.sampler_states))
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

struct wined3d_buffer * CDECL wined3d_device_get_vs_cb(const struct wined3d_device *device, UINT idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    if (idx >= MAX_CONSTANT_BUFFERS)
    {
        WARN("Invalid constant buffer index %u.\n", idx);
        return NULL;
    }

    return device->state.cb[WINED3D_SHADER_TYPE_VERTEX][idx];
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

struct wined3d_shader_resource_view * CDECL wined3d_device_get_vs_resource_view(const struct wined3d_device *device,
        UINT idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    if (idx >= MAX_SHADER_RESOURCE_VIEWS)
    {
        WARN("Invalid view index %u.\n", idx);
        return NULL;
    }

    return device->state.shader_resource_view[WINED3D_SHADER_TYPE_VERTEX][idx];
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

struct wined3d_sampler * CDECL wined3d_device_get_vs_sampler(const struct wined3d_device *device, UINT idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    if (idx >= MAX_SAMPLER_OBJECTS)
    {
        WARN("Invalid sampler index %u.\n", idx);
        return NULL;
    }

    return device->state.sampler[WINED3D_SHADER_TYPE_VERTEX][idx];
}

static void device_invalidate_shader_constants(const struct wined3d_device *device, DWORD mask)
{
    UINT i;

    for (i = 0; i < device->context_count; ++i)
    {
        device->contexts[i]->constant_update_mask |= mask;
    }
}

HRESULT CDECL wined3d_device_set_vs_consts_b(struct wined3d_device *device,
        UINT start_register, const BOOL *constants, UINT bool_count)
{
    UINT count = min(bool_count, MAX_CONST_B - start_register);
    UINT i;

    TRACE("device %p, start_register %u, constants %p, bool_count %u.\n",
            device, start_register, constants, bool_count);

    if (!constants || start_register >= MAX_CONST_B)
        return WINED3DERR_INVALIDCALL;

    memcpy(&device->update_state->vs_consts_b[start_register], constants, count * sizeof(BOOL));
    for (i = 0; i < count; ++i)
        TRACE("Set BOOL constant %u to %s.\n", start_register + i, constants[i] ? "true" : "false");

    if (device->recording)
    {
        for (i = start_register; i < count + start_register; ++i)
            device->recording->changed.vertexShaderConstantsB |= (1 << i);
    }
    else
    {
        device_invalidate_shader_constants(device, WINED3D_SHADER_CONST_VS_B);
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_vs_consts_b(const struct wined3d_device *device,
        UINT start_register, BOOL *constants, UINT bool_count)
{
    UINT count = min(bool_count, MAX_CONST_B - start_register);

    TRACE("device %p, start_register %u, constants %p, bool_count %u.\n",
            device, start_register, constants, bool_count);

    if (!constants || start_register >= MAX_CONST_B)
        return WINED3DERR_INVALIDCALL;

    memcpy(constants, &device->state.vs_consts_b[start_register], count * sizeof(BOOL));

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_vs_consts_i(struct wined3d_device *device,
        UINT start_register, const int *constants, UINT vector4i_count)
{
    UINT count = min(vector4i_count, MAX_CONST_I - start_register);
    UINT i;

    TRACE("device %p, start_register %u, constants %p, vector4i_count %u.\n",
            device, start_register, constants, vector4i_count);

    if (!constants || start_register >= MAX_CONST_I)
        return WINED3DERR_INVALIDCALL;

    memcpy(&device->update_state->vs_consts_i[start_register * 4], constants, count * sizeof(int) * 4);
    for (i = 0; i < count; ++i)
        TRACE("Set INT constant %u to {%d, %d, %d, %d}.\n", start_register + i,
                constants[i * 4], constants[i * 4 + 1],
                constants[i * 4 + 2], constants[i * 4 + 3]);

    if (device->recording)
    {
        for (i = start_register; i < count + start_register; ++i)
            device->recording->changed.vertexShaderConstantsI |= (1 << i);
    }
    else
    {
        device_invalidate_shader_constants(device, WINED3D_SHADER_CONST_VS_I);
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_vs_consts_i(const struct wined3d_device *device,
        UINT start_register, int *constants, UINT vector4i_count)
{
    UINT count = min(vector4i_count, MAX_CONST_I - start_register);

    TRACE("device %p, start_register %u, constants %p, vector4i_count %u.\n",
            device, start_register, constants, vector4i_count);

    if (!constants || start_register >= MAX_CONST_I)
        return WINED3DERR_INVALIDCALL;

    memcpy(constants, &device->state.vs_consts_i[start_register * 4], count * sizeof(int) * 4);
    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_vs_consts_f(struct wined3d_device *device,
        UINT start_register, const float *constants, UINT vector4f_count)
{
    UINT i;
    const struct wined3d_d3d_info *d3d_info = &device->adapter->d3d_info;

    TRACE("device %p, start_register %u, constants %p, vector4f_count %u.\n",
            device, start_register, constants, vector4f_count);

    /* Specifically test start_register > limit to catch MAX_UINT overflows
     * when adding start_register + vector4f_count. */
    if (!constants
            || start_register + vector4f_count > d3d_info->limits.vs_uniform_count
            || start_register > d3d_info->limits.vs_uniform_count)
        return WINED3DERR_INVALIDCALL;

    memcpy(&device->update_state->vs_consts_f[start_register * 4],
            constants, vector4f_count * sizeof(float) * 4);
    if (TRACE_ON(d3d))
    {
        for (i = 0; i < vector4f_count; ++i)
            TRACE("Set FLOAT constant %u to {%.8e, %.8e, %.8e, %.8e}.\n", start_register + i,
                    constants[i * 4], constants[i * 4 + 1],
                    constants[i * 4 + 2], constants[i * 4 + 3]);
    }

    if (device->recording)
        memset(device->recording->changed.vertexShaderConstantsF + start_register, 1,
                sizeof(*device->recording->changed.vertexShaderConstantsF) * vector4f_count);
    else
        device->shader_backend->shader_update_float_vertex_constants(device, start_register, vector4f_count);


    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_vs_consts_f(const struct wined3d_device *device,
        UINT start_register, float *constants, UINT vector4f_count)
{
    const struct wined3d_d3d_info *d3d_info = &device->adapter->d3d_info;
    int count = min(vector4f_count, d3d_info->limits.vs_uniform_count - start_register);

    TRACE("device %p, start_register %u, constants %p, vector4f_count %u.\n",
            device, start_register, constants, vector4f_count);

    if (!constants || count < 0)
        return WINED3DERR_INVALIDCALL;

    memcpy(constants, &device->state.vs_consts_f[start_register * 4], count * sizeof(float) * 4);

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

    if (idx >= MAX_CONSTANT_BUFFERS)
    {
        WARN("Invalid constant buffer index %u.\n", idx);
        return NULL;
    }

    return device->state.cb[WINED3D_SHADER_TYPE_PIXEL][idx];
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

    if (idx >= MAX_SHADER_RESOURCE_VIEWS)
    {
        WARN("Invalid view index %u.\n", idx);
        return NULL;
    }

    return device->state.shader_resource_view[WINED3D_SHADER_TYPE_PIXEL][idx];
}

void CDECL wined3d_device_set_ps_sampler(struct wined3d_device *device, UINT idx, struct wined3d_sampler *sampler)
{
    TRACE("device %p, idx %u, sampler %p.\n", device, idx, sampler);

    wined3d_device_set_sampler(device, WINED3D_SHADER_TYPE_PIXEL, idx, sampler);
}

struct wined3d_sampler * CDECL wined3d_device_get_ps_sampler(const struct wined3d_device *device, UINT idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    if (idx >= MAX_SAMPLER_OBJECTS)
    {
        WARN("Invalid sampler index %u.\n", idx);
        return NULL;
    }

    return device->state.sampler[WINED3D_SHADER_TYPE_PIXEL][idx];
}

HRESULT CDECL wined3d_device_set_ps_consts_b(struct wined3d_device *device,
        UINT start_register, const BOOL *constants, UINT bool_count)
{
    UINT count = min(bool_count, MAX_CONST_B - start_register);
    UINT i;

    TRACE("device %p, start_register %u, constants %p, bool_count %u.\n",
            device, start_register, constants, bool_count);

    if (!constants || start_register >= MAX_CONST_B)
        return WINED3DERR_INVALIDCALL;

    memcpy(&device->update_state->ps_consts_b[start_register], constants, count * sizeof(BOOL));
    for (i = 0; i < count; ++i)
        TRACE("Set BOOL constant %u to %s.\n", start_register + i, constants[i] ? "true" : "false");

    if (device->recording)
    {
        for (i = start_register; i < count + start_register; ++i)
            device->recording->changed.pixelShaderConstantsB |= (1 << i);
    }
    else
    {
        device_invalidate_shader_constants(device, WINED3D_SHADER_CONST_PS_B);
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_ps_consts_b(const struct wined3d_device *device,
        UINT start_register, BOOL *constants, UINT bool_count)
{
    UINT count = min(bool_count, MAX_CONST_B - start_register);

    TRACE("device %p, start_register %u, constants %p, bool_count %u.\n",
            device, start_register, constants, bool_count);

    if (!constants || start_register >= MAX_CONST_B)
        return WINED3DERR_INVALIDCALL;

    memcpy(constants, &device->state.ps_consts_b[start_register], count * sizeof(BOOL));

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_ps_consts_i(struct wined3d_device *device,
        UINT start_register, const int *constants, UINT vector4i_count)
{
    UINT count = min(vector4i_count, MAX_CONST_I - start_register);
    UINT i;

    TRACE("device %p, start_register %u, constants %p, vector4i_count %u.\n",
            device, start_register, constants, vector4i_count);

    if (!constants || start_register >= MAX_CONST_I)
        return WINED3DERR_INVALIDCALL;

    memcpy(&device->update_state->ps_consts_i[start_register * 4], constants, count * sizeof(int) * 4);
    for (i = 0; i < count; ++i)
        TRACE("Set INT constant %u to {%d, %d, %d, %d}.\n", start_register + i,
                constants[i * 4], constants[i * 4 + 1],
                constants[i * 4 + 2], constants[i * 4 + 3]);

    if (device->recording)
    {
        for (i = start_register; i < count + start_register; ++i)
            device->recording->changed.pixelShaderConstantsI |= (1 << i);
    }
    else
    {
        device_invalidate_shader_constants(device, WINED3D_SHADER_CONST_PS_I);
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_ps_consts_i(const struct wined3d_device *device,
        UINT start_register, int *constants, UINT vector4i_count)
{
    UINT count = min(vector4i_count, MAX_CONST_I - start_register);

    TRACE("device %p, start_register %u, constants %p, vector4i_count %u.\n",
            device, start_register, constants, vector4i_count);

    if (!constants || start_register >= MAX_CONST_I)
        return WINED3DERR_INVALIDCALL;

    memcpy(constants, &device->state.ps_consts_i[start_register * 4], count * sizeof(int) * 4);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_set_ps_consts_f(struct wined3d_device *device,
        UINT start_register, const float *constants, UINT vector4f_count)
{
    UINT i;
    const struct wined3d_d3d_info *d3d_info = &device->adapter->d3d_info;

    TRACE("device %p, start_register %u, constants %p, vector4f_count %u.\n",
            device, start_register, constants, vector4f_count);

    /* Specifically test start_register > limit to catch MAX_UINT overflows
     * when adding start_register + vector4f_count. */
    if (!constants
            || start_register + vector4f_count > d3d_info->limits.ps_uniform_count
            || start_register > d3d_info->limits.ps_uniform_count)
        return WINED3DERR_INVALIDCALL;

    memcpy(&device->update_state->ps_consts_f[start_register * 4],
            constants, vector4f_count * sizeof(float) * 4);
    if (TRACE_ON(d3d))
    {
        for (i = 0; i < vector4f_count; ++i)
            TRACE("Set FLOAT constant %u to {%.8e, %.8e, %.8e, %.8e}.\n", start_register + i,
                    constants[i * 4], constants[i * 4 + 1],
                    constants[i * 4 + 2], constants[i * 4 + 3]);
    }

    if (device->recording)
        memset(device->recording->changed.pixelShaderConstantsF + start_register, 1,
                sizeof(*device->recording->changed.pixelShaderConstantsF) * vector4f_count);
    else
        device->shader_backend->shader_update_float_pixel_constants(device, start_register, vector4f_count);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_ps_consts_f(const struct wined3d_device *device,
        UINT start_register, float *constants, UINT vector4f_count)
{
    const struct wined3d_d3d_info *d3d_info = &device->adapter->d3d_info;
    int count = min(vector4f_count, d3d_info->limits.ps_uniform_count - start_register);

    TRACE("device %p, start_register %u, constants %p, vector4f_count %u.\n",
            device, start_register, constants, vector4f_count);

    if (!constants || count < 0)
        return WINED3DERR_INVALIDCALL;

    memcpy(constants, &device->state.ps_consts_f[start_register * 4], count * sizeof(float) * 4);

    return WINED3D_OK;
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

    if (idx >= MAX_CONSTANT_BUFFERS)
    {
        WARN("Invalid constant buffer index %u.\n", idx);
        return NULL;
    }

    return device->state.cb[WINED3D_SHADER_TYPE_GEOMETRY][idx];
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

    if (idx >= MAX_SHADER_RESOURCE_VIEWS)
    {
        WARN("Invalid view index %u.\n", idx);
        return NULL;
    }

    return device->state.shader_resource_view[WINED3D_SHADER_TYPE_GEOMETRY][idx];
}

void CDECL wined3d_device_set_gs_sampler(struct wined3d_device *device, UINT idx, struct wined3d_sampler *sampler)
{
    TRACE("device %p, idx %u, sampler %p.\n", device, idx, sampler);

    wined3d_device_set_sampler(device, WINED3D_SHADER_TYPE_GEOMETRY, idx, sampler);
}

struct wined3d_sampler * CDECL wined3d_device_get_gs_sampler(const struct wined3d_device *device, UINT idx)
{
    TRACE("device %p, idx %u.\n", device, idx);

    if (idx >= MAX_SAMPLER_OBJECTS)
    {
        WARN("Invalid sampler index %u.\n", idx);
        return NULL;
    }

    return device->state.sampler[WINED3D_SHADER_TYPE_GEOMETRY][idx];
}

/* Context activation is done by the caller. */
#define copy_and_next(dest, src, size) memcpy(dest, src, size); dest += (size)
static HRESULT process_vertices_strided(const struct wined3d_device *device, DWORD dwDestIndex, DWORD dwCount,
        const struct wined3d_stream_info *stream_info, struct wined3d_buffer *dest, DWORD flags,
        DWORD DestFVF)
{
    struct wined3d_matrix mat, proj_mat, view_mat, world_mat;
    struct wined3d_viewport vp;
    UINT vertex_size;
    unsigned int i;
    BYTE *dest_ptr;
    BOOL doClip;
    DWORD numTextures;
    HRESULT hr;

    if (stream_info->use_map & (1 << WINED3D_FFP_NORMAL))
    {
        WARN(" lighting state not saved yet... Some strange stuff may happen !\n");
    }

    if (!(stream_info->use_map & (1 << WINED3D_FFP_POSITION)))
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
    if (FAILED(hr = wined3d_buffer_map(dest, dwDestIndex * vertex_size, dwCount * vertex_size, &dest_ptr, 0)))
    {
        WARN("Failed to map buffer, hr %#x.\n", hr);
        return hr;
    }

    wined3d_device_get_transform(device, WINED3D_TS_VIEW, &view_mat);
    wined3d_device_get_transform(device, WINED3D_TS_PROJECTION, &proj_mat);
    wined3d_device_get_transform(device, WINED3D_TS_WORLD_MATRIX(0), &world_mat);

    TRACE("View mat:\n");
    TRACE("%f %f %f %f\n", view_mat.u.s._11, view_mat.u.s._12, view_mat.u.s._13, view_mat.u.s._14);
    TRACE("%f %f %f %f\n", view_mat.u.s._21, view_mat.u.s._22, view_mat.u.s._23, view_mat.u.s._24);
    TRACE("%f %f %f %f\n", view_mat.u.s._31, view_mat.u.s._32, view_mat.u.s._33, view_mat.u.s._34);
    TRACE("%f %f %f %f\n", view_mat.u.s._41, view_mat.u.s._42, view_mat.u.s._43, view_mat.u.s._44);

    TRACE("Proj mat:\n");
    TRACE("%f %f %f %f\n", proj_mat.u.s._11, proj_mat.u.s._12, proj_mat.u.s._13, proj_mat.u.s._14);
    TRACE("%f %f %f %f\n", proj_mat.u.s._21, proj_mat.u.s._22, proj_mat.u.s._23, proj_mat.u.s._24);
    TRACE("%f %f %f %f\n", proj_mat.u.s._31, proj_mat.u.s._32, proj_mat.u.s._33, proj_mat.u.s._34);
    TRACE("%f %f %f %f\n", proj_mat.u.s._41, proj_mat.u.s._42, proj_mat.u.s._43, proj_mat.u.s._44);

    TRACE("World mat:\n");
    TRACE("%f %f %f %f\n", world_mat.u.s._11, world_mat.u.s._12, world_mat.u.s._13, world_mat.u.s._14);
    TRACE("%f %f %f %f\n", world_mat.u.s._21, world_mat.u.s._22, world_mat.u.s._23, world_mat.u.s._24);
    TRACE("%f %f %f %f\n", world_mat.u.s._31, world_mat.u.s._32, world_mat.u.s._33, world_mat.u.s._34);
    TRACE("%f %f %f %f\n", world_mat.u.s._41, world_mat.u.s._42, world_mat.u.s._43, world_mat.u.s._44);

    /* Get the viewport */
    wined3d_device_get_viewport(device, &vp);
    TRACE("viewport  x %u, y %u, width %u, height %u, min_z %.8e, max_z %.8e.\n",
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

            /* Multiplication with world, view and projection matrix */
            x =   (p[0] * mat.u.s._11) + (p[1] * mat.u.s._21) + (p[2] * mat.u.s._31) + (1.0f * mat.u.s._41);
            y =   (p[0] * mat.u.s._12) + (p[1] * mat.u.s._22) + (p[2] * mat.u.s._32) + (1.0f * mat.u.s._42);
            z =   (p[0] * mat.u.s._13) + (p[1] * mat.u.s._23) + (p[2] * mat.u.s._33) + (1.0f * mat.u.s._43);
            rhw = (p[0] * mat.u.s._14) + (p[1] * mat.u.s._24) + (p[2] * mat.u.s._34) + (1.0f * mat.u.s._44);

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
            if (!(stream_info->use_map & (1 << WINED3D_FFP_DIFFUSE)))
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
            if (!(stream_info->use_map & (1 << WINED3D_FFP_SPECULAR)))
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
            if (!(stream_info->use_map & (1 << (WINED3D_FFP_TEXCOORD0 + tex_index))))
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

    wined3d_buffer_unmap(dest);

    return WINED3D_OK;
}
#undef copy_and_next

HRESULT CDECL wined3d_device_process_vertices(struct wined3d_device *device,
        UINT src_start_idx, UINT dst_idx, UINT vertex_count, struct wined3d_buffer *dst_buffer,
        const struct wined3d_vertex_declaration *declaration, DWORD flags, DWORD dst_fvf)
{
    struct wined3d_state *state = &device->state;
    struct wined3d_stream_info stream_info;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
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

    /* Need any context to write to the vbo. */
    context = context_acquire(device, NULL);
    gl_info = context->gl_info;

    vs = state->shader[WINED3D_SHADER_TYPE_VERTEX];
    state->shader[WINED3D_SHADER_TYPE_VERTEX] = NULL;
    context_stream_info_from_declaration(context, state, &stream_info);
    state->shader[WINED3D_SHADER_TYPE_VERTEX] = vs;

    /* We can't convert FROM a VBO, and vertex buffers used to source into
     * process_vertices() are unlikely to ever be used for drawing. Release
     * VBOs in those buffers and fix up the stream_info structure.
     *
     * Also apply the start index. */
    for (i = 0, map = stream_info.use_map; map; map >>= 1, ++i)
    {
        struct wined3d_stream_info_element *e;
        struct wined3d_buffer *buffer;

        if (!(map & 1))
            continue;

        e = &stream_info.elements[i];
        buffer = state->streams[e->stream_idx].buffer;
        e->data.buffer_object = 0;
        e->data.addr += (ULONG_PTR)buffer_get_sysmem(buffer, context);
        if (buffer->buffer_object)
        {
            GL_EXTCALL(glDeleteBuffers(1, &buffer->buffer_object));
            buffer->buffer_object = 0;
        }
        if (e->data.addr)
            e->data.addr += e->stride * src_start_idx;
    }

    hr = process_vertices_strided(device, dst_idx, vertex_count,
            &stream_info, dst_buffer, flags, dst_fvf);

    context_release(context);

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
        device->recording->changed.textureState[stage] |= 1 << state;
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
    if (stage >= sizeof(device->state.textures) / sizeof(*device->state.textures))
    {
        WARN("Ignoring invalid stage %u.\n", stage);
        return WINED3D_OK;
    }

    if (texture && texture->resource.pool == WINED3D_POOL_SCRATCH)
    {
        WARN("Rejecting attempt to set scratch texture.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (device->recording)
        device->recording->changed.textures |= 1 << stage;

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

    if (stage >= sizeof(device->state.textures) / sizeof(*device->state.textures))
    {
        WARN("Ignoring invalid stage %u.\n", stage);
        return NULL; /* Windows accepts overflowing this array ... we do not. */
    }

    return device->state.textures[stage];
}

HRESULT CDECL wined3d_device_get_back_buffer(const struct wined3d_device *device, UINT swapchain_idx,
        UINT backbuffer_idx, enum wined3d_backbuffer_type backbuffer_type, struct wined3d_surface **backbuffer)
{
    struct wined3d_swapchain *swapchain;

    TRACE("device %p, swapchain_idx %u, backbuffer_idx %u, backbuffer_type %#x, backbuffer %p.\n",
            device, swapchain_idx, backbuffer_idx, backbuffer_type, backbuffer);

    if (!(swapchain = wined3d_device_get_swapchain(device, swapchain_idx)))
        return WINED3DERR_INVALIDCALL;

    if (!(*backbuffer = wined3d_swapchain_get_back_buffer(swapchain, backbuffer_idx, backbuffer_type)))
        return WINED3DERR_INVALIDCALL;
    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_device_caps(const struct wined3d_device *device, WINED3DCAPS *caps)
{
    TRACE("device %p, caps %p.\n", device, caps);

    return wined3d_get_device_caps(device->wined3d, device->adapter->ordinal,
            device->create_parms.device_type, caps);
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
    struct wined3d_context *context;

    TRACE("device %p.\n", device);

    if (!device->inScene)
    {
        WARN("Not in scene, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    context = context_acquire(device, NULL);
    /* We only have to do this if we need to read the, swapbuffers performs a flush for us */
    context->gl_info->gl_ops.gl.p_glFlush();
    /* No checkGLcall here to avoid locking the lock just for checking a call that hardly ever
     * fails. */
    context_release(context);

    device->inScene = FALSE;
    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_present(const struct wined3d_device *device, const RECT *src_rect,
        const RECT *dst_rect, HWND dst_window_override, const RGNDATA *dirty_region, DWORD flags)
{
    UINT i;

    TRACE("device %p, src_rect %s, dst_rect %s, dst_window_override %p, dirty_region %p, flags %#x.\n",
            device, wine_dbgstr_rect(src_rect), wine_dbgstr_rect(dst_rect),
            dst_window_override, dirty_region, flags);

    for (i = 0; i < device->swapchain_count; ++i)
    {
        wined3d_swapchain_present(device->swapchains[i], src_rect,
                dst_rect, dst_window_override, dirty_region, flags);
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_clear(struct wined3d_device *device, DWORD rect_count,
        const RECT *rects, DWORD flags, const struct wined3d_color *color, float depth, DWORD stencil)
{
    TRACE("device %p, rect_count %u, rects %p, flags %#x, color {%.8e, %.8e, %.8e, %.8e}, depth %.8e, stencil %u.\n",
            device, rect_count, rects, flags, color->r, color->g, color->b, color->a, depth, stencil);

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

    *value = device->state.predicate_value;
    return device->state.predicate;
}

void CDECL wined3d_device_set_primitive_type(struct wined3d_device *device,
        enum wined3d_primitive_type primitive_type)
{
    GLenum gl_primitive_type, prev;

    TRACE("device %p, primitive_type %s\n", device, debug_d3dprimitivetype(primitive_type));

    gl_primitive_type = gl_primitive_type_from_d3d(primitive_type);
    prev = device->update_state->gl_primitive_type;
    device->update_state->gl_primitive_type = gl_primitive_type;
    if (device->recording)
        device->recording->changed.primitive_type = TRUE;
    else if (gl_primitive_type != prev && (gl_primitive_type == GL_POINTS || prev == GL_POINTS))
        device_invalidate_state(device, STATE_POINT_SIZE_ENABLE);
}

void CDECL wined3d_device_get_primitive_type(const struct wined3d_device *device,
        enum wined3d_primitive_type *primitive_type)
{
    TRACE("device %p, primitive_type %p\n", device, primitive_type);

    *primitive_type = d3d_primitive_type_from_gl(device->state.gl_primitive_type);

    TRACE("Returning %s\n", debug_d3dprimitivetype(*primitive_type));
}

HRESULT CDECL wined3d_device_draw_primitive(struct wined3d_device *device, UINT start_vertex, UINT vertex_count)
{
    TRACE("device %p, start_vertex %u, vertex_count %u.\n", device, start_vertex, vertex_count);

    if (!device->state.vertex_declaration)
    {
        WARN("Called without a valid vertex declaration set.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (device->state.load_base_vertex_index)
    {
        device->state.load_base_vertex_index = 0;
        device_invalidate_state(device, STATE_BASEVERTEXINDEX);
    }

    wined3d_cs_emit_draw(device->cs, start_vertex, vertex_count, 0, 0, FALSE);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_draw_indexed_primitive(struct wined3d_device *device, UINT start_idx, UINT index_count)
{
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;

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

    if (!device->state.vertex_declaration)
    {
        WARN("Called without a valid vertex declaration set.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (!gl_info->supported[ARB_DRAW_ELEMENTS_BASE_VERTEX] &&
        device->state.load_base_vertex_index != device->state.base_vertex_index)
    {
        device->state.load_base_vertex_index = device->state.base_vertex_index;
        device_invalidate_state(device, STATE_BASEVERTEXINDEX);
    }

    wined3d_cs_emit_draw(device->cs, start_idx, index_count, 0, 0, TRUE);

    return WINED3D_OK;
}

void CDECL wined3d_device_draw_indexed_primitive_instanced(struct wined3d_device *device,
        UINT start_idx, UINT index_count, UINT start_instance, UINT instance_count)
{
    TRACE("device %p, start_idx %u, index_count %u.\n", device, start_idx, index_count);

    wined3d_cs_emit_draw(device->cs, start_idx, index_count, start_instance, instance_count, TRUE);
}

/* This is a helper function for UpdateTexture, there is no UpdateVolume method in D3D. */
static HRESULT device_update_volume(struct wined3d_device *device,
        struct wined3d_volume *src_volume, struct wined3d_volume *dst_volume)
{
    struct wined3d_const_bo_address data;
    struct wined3d_map_desc src;
    HRESULT hr;
    struct wined3d_context *context;

    TRACE("device %p, src_volume %p, dst_volume %p.\n",
            device, src_volume, dst_volume);

    if (src_volume->resource.format != dst_volume->resource.format)
    {
        FIXME("Source and destination formats do not match.\n");
        return WINED3DERR_INVALIDCALL;
    }
    if (src_volume->resource.width != dst_volume->resource.width
            || src_volume->resource.height != dst_volume->resource.height
            || src_volume->resource.depth != dst_volume->resource.depth)
    {
        FIXME("Source and destination sizes do not match.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (FAILED(hr = wined3d_volume_map(src_volume, &src, NULL, WINED3D_MAP_READONLY)))
        return hr;

    context = context_acquire(device, NULL);

    /* Only a prepare, since we're uploading the entire volume. */
    wined3d_texture_prepare_texture(dst_volume->container, context, FALSE);
    wined3d_texture_bind(dst_volume->container, context, FALSE);

    data.buffer_object = 0;
    data.addr = src.data;
    wined3d_volume_upload_data(dst_volume, context, &data);
    wined3d_volume_invalidate_location(dst_volume, ~WINED3D_LOCATION_TEXTURE_RGB);

    context_release(context);

    hr = wined3d_volume_unmap(src_volume);

    return hr;
}

HRESULT CDECL wined3d_device_update_texture(struct wined3d_device *device,
        struct wined3d_texture *src_texture, struct wined3d_texture *dst_texture)
{
    enum wined3d_resource_type type;
    unsigned int level_count, i;
    HRESULT hr;
    struct wined3d_context *context;

    TRACE("device %p, src_texture %p, dst_texture %p.\n", device, src_texture, dst_texture);

    /* Verify that the source and destination textures are non-NULL. */
    if (!src_texture || !dst_texture)
    {
        WARN("Source and destination textures must be non-NULL, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (src_texture->resource.pool != WINED3D_POOL_SYSTEM_MEM)
    {
        WARN("Source texture not in WINED3D_POOL_SYSTEM_MEM, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }
    if (dst_texture->resource.pool != WINED3D_POOL_DEFAULT)
    {
        WARN("Destination texture not in WINED3D_POOL_DEFAULT, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* Verify that the source and destination textures are the same type. */
    type = src_texture->resource.type;
    if (dst_texture->resource.type != type)
    {
        WARN("Source and destination have different types, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* Check that both textures have the identical numbers of levels. */
    level_count = wined3d_texture_get_level_count(src_texture);
    if (wined3d_texture_get_level_count(dst_texture) != level_count)
    {
        WARN("Source and destination have different level counts, returning WINED3DERR_INVALIDCALL.\n");
        return WINED3DERR_INVALIDCALL;
    }

    /* Make sure that the destination texture is loaded. */
    context = context_acquire(device, NULL);
    wined3d_texture_load(dst_texture, context, FALSE);
    context_release(context);

    /* Update every surface level of the texture. */
    switch (type)
    {
        case WINED3D_RTYPE_TEXTURE:
        {
            struct wined3d_surface *src_surface;
            struct wined3d_surface *dst_surface;

            for (i = 0; i < level_count; ++i)
            {
                src_surface = surface_from_resource(wined3d_texture_get_sub_resource(src_texture, i));
                dst_surface = surface_from_resource(wined3d_texture_get_sub_resource(dst_texture, i));
                hr = wined3d_device_update_surface(device, src_surface, NULL, dst_surface, NULL);
                if (FAILED(hr))
                {
                    WARN("Failed to update surface, hr %#x.\n", hr);
                    return hr;
                }
            }
            break;
        }

        case WINED3D_RTYPE_CUBE_TEXTURE:
        {
            struct wined3d_surface *src_surface;
            struct wined3d_surface *dst_surface;

            for (i = 0; i < level_count * 6; ++i)
            {
                src_surface = surface_from_resource(wined3d_texture_get_sub_resource(src_texture, i));
                dst_surface = surface_from_resource(wined3d_texture_get_sub_resource(dst_texture, i));
                hr = wined3d_device_update_surface(device, src_surface, NULL, dst_surface, NULL);
                if (FAILED(hr))
                {
                    WARN("Failed to update surface, hr %#x.\n", hr);
                    return hr;
                }
            }
            break;
        }

        case WINED3D_RTYPE_VOLUME_TEXTURE:
        {
            for (i = 0; i < level_count; ++i)
            {
                hr = device_update_volume(device,
                        volume_from_resource(wined3d_texture_get_sub_resource(src_texture, i)),
                        volume_from_resource(wined3d_texture_get_sub_resource(dst_texture, i)));
                if (FAILED(hr))
                {
                    WARN("Failed to update volume, hr %#x.\n", hr);
                    return hr;
                }
            }
            break;
        }

        default:
            FIXME("Unsupported texture type %#x.\n", type);
            return WINED3DERR_INVALIDCALL;
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_get_front_buffer_data(const struct wined3d_device *device,
        UINT swapchain_idx, struct wined3d_surface *dst_surface)
{
    struct wined3d_swapchain *swapchain;

    TRACE("device %p, swapchain_idx %u, dst_surface %p.\n", device, swapchain_idx, dst_surface);

    if (!(swapchain = wined3d_device_get_swapchain(device, swapchain_idx)))
        return WINED3DERR_INVALIDCALL;

    return wined3d_swapchain_get_front_buffer_data(swapchain, dst_surface);
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
        if (!texture || texture->resource.format->flags & WINED3DFMT_FLAG_FILTERING) continue;

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
    static BOOL warned;

    TRACE("device %p, software %#x.\n", device, software);

    if (!warned)
    {
        FIXME("device %p, software %#x stub!\n", device, software);
        warned = TRUE;
    }

    device->softwareVertexProcessing = software;
}

BOOL CDECL wined3d_device_get_software_vertex_processing(const struct wined3d_device *device)
{
    static BOOL warned;

    TRACE("device %p.\n", device);

    if (!warned)
    {
        TRACE("device %p stub!\n", device);
        warned = TRUE;
    }

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

HRESULT CDECL wined3d_device_update_surface(struct wined3d_device *device,
        struct wined3d_surface *src_surface, const RECT *src_rect,
        struct wined3d_surface *dst_surface, const POINT *dst_point)
{
    TRACE("device %p, src_surface %p, src_rect %s, dst_surface %p, dst_point %s.\n",
            device, src_surface, wine_dbgstr_rect(src_rect),
            dst_surface, wine_dbgstr_point(dst_point));

    if (src_surface->resource.pool != WINED3D_POOL_SYSTEM_MEM || dst_surface->resource.pool != WINED3D_POOL_DEFAULT)
    {
        WARN("source %p must be SYSTEMMEM and dest %p must be DEFAULT, returning WINED3DERR_INVALIDCALL\n",
                src_surface, dst_surface);
        return WINED3DERR_INVALIDCALL;
    }

    return surface_upload_from_surface(dst_surface, dst_point, src_surface, src_rect);
}

void CDECL wined3d_device_copy_resource(struct wined3d_device *device,
        struct wined3d_resource *dst_resource, struct wined3d_resource *src_resource)
{
    struct wined3d_surface *dst_surface, *src_surface;
    struct wined3d_texture *dst_texture, *src_texture;
    unsigned int i, count;
    HRESULT hr;

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

    if (src_resource->format->id != dst_resource->format->id)
    {
        WARN("Resource formats (%s / %s) don't match.\n",
                debug_d3dformat(dst_resource->format->id),
                debug_d3dformat(src_resource->format->id));
        return;
    }

    if (dst_resource->type != WINED3D_RTYPE_TEXTURE)
    {
        FIXME("Not implemented for %s resources.\n", debug_d3dresourcetype(dst_resource->type));
        return;
    }

    dst_texture = wined3d_texture_from_resource(dst_resource);
    src_texture = wined3d_texture_from_resource(src_resource);

    if (src_texture->layer_count != dst_texture->layer_count
            || src_texture->level_count != dst_texture->level_count)
    {
        WARN("Subresource layouts (%ux%u / %ux%u) don't match.\n",
                dst_texture->layer_count, dst_texture->level_count,
                src_texture->layer_count, src_texture->level_count);
        return;
    }

    count = dst_texture->layer_count * dst_texture->level_count;
    for (i = 0; i < count; ++i)
    {
        dst_surface = surface_from_resource(wined3d_texture_get_sub_resource(dst_texture, i));
        src_surface = surface_from_resource(wined3d_texture_get_sub_resource(src_texture, i));

        if (FAILED(hr = wined3d_surface_blt(dst_surface, NULL, src_surface, NULL, 0, NULL, WINED3D_TEXF_POINT)))
            ERR("Failed to blit, subresource %u, hr %#x.\n", i, hr);
    }
}

HRESULT CDECL wined3d_device_clear_rendertarget_view(struct wined3d_device *device,
        struct wined3d_rendertarget_view *view, const RECT *rect, const struct wined3d_color *color)
{
    struct wined3d_resource *resource;
    RECT r;

    TRACE("device %p, view %p, rect %s, color {%.8e, %.8e, %.8e, %.8e}.\n",
            device, view, wine_dbgstr_rect(rect), color->r, color->g, color->b, color->a);

    resource = view->resource;
    if (resource->type != WINED3D_RTYPE_TEXTURE && resource->type != WINED3D_RTYPE_CUBE_TEXTURE)
    {
        FIXME("Not implemented for %s resources.\n", debug_d3dresourcetype(resource->type));
        return WINED3DERR_INVALIDCALL;
    }

    if (view->depth > 1)
    {
        FIXME("Layered clears not implemented.\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (!rect)
    {
        SetRect(&r, 0, 0, view->width, view->height);
        rect = &r;
    }

    resource = wined3d_texture_get_sub_resource(wined3d_texture_from_resource(resource), view->sub_resource_idx);

    return surface_color_fill(surface_from_resource(resource), rect, color);
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

        state->scissor_rect.top = 0;
        state->scissor_rect.left = 0;
        state->scissor_rect.right = view->width;
        state->scissor_rect.bottom = view->height;
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
        struct wined3d_surface *cursor_image)
{
    struct wined3d_sub_resource_data data;
    struct wined3d_resource_desc desc;
    struct wined3d_map_desc map_desc;
    struct wined3d_texture *texture;
    HRESULT hr;

    if (FAILED(wined3d_surface_map(cursor_image, &map_desc, NULL, WINED3D_MAP_READONLY)))
    {
        ERR("Failed to map source surface.\n");
        return NULL;
    }

    data.data = map_desc.data;
    data.row_pitch = map_desc.row_pitch;
    data.slice_pitch = map_desc.slice_pitch;

    desc.resource_type = WINED3D_RTYPE_TEXTURE;
    desc.format = WINED3DFMT_B8G8R8A8_UNORM;
    desc.multisample_type = WINED3D_MULTISAMPLE_NONE;
    desc.multisample_quality = 0;
    desc.usage = WINED3DUSAGE_DYNAMIC;
    desc.pool = WINED3D_POOL_DEFAULT;
    desc.width = cursor_image->resource.width;
    desc.height = cursor_image->resource.height;
    desc.depth = 1;
    desc.size = 0;

    hr = wined3d_texture_create(device, &desc, 1, WINED3D_SURFACE_MAPPABLE,
            &data, NULL, &wined3d_null_parent_ops, &texture);
    wined3d_surface_unmap(cursor_image);
    if (FAILED(hr))
    {
        ERR("Failed to create cursor texture.\n");
        return NULL;
    }

    return texture;
}

HRESULT CDECL wined3d_device_set_cursor_properties(struct wined3d_device *device,
        UINT x_hotspot, UINT y_hotspot, struct wined3d_surface *cursor_image)
{
    TRACE("device %p, x_hotspot %u, y_hotspot %u, cursor_image %p.\n",
            device, x_hotspot, y_hotspot, cursor_image);

    if (device->cursor_texture)
    {
        wined3d_texture_decref(device->cursor_texture);
        device->cursor_texture = NULL;
    }

    if (cursor_image)
    {
        struct wined3d_display_mode mode;
        struct wined3d_map_desc map_desc;
        HRESULT hr;

        /* MSDN: Cursor must be A8R8G8B8 */
        if (cursor_image->resource.format->id != WINED3DFMT_B8G8R8A8_UNORM)
        {
            WARN("surface %p has an invalid format.\n", cursor_image);
            return WINED3DERR_INVALIDCALL;
        }

        if (FAILED(hr = wined3d_get_adapter_display_mode(device->wined3d, device->adapter->ordinal, &mode, NULL)))
        {
            ERR("Failed to get display mode, hr %#x.\n", hr);
            return WINED3DERR_INVALIDCALL;
        }

        /* MSDN: Cursor must be smaller than the display mode */
        if (cursor_image->resource.width > mode.width || cursor_image->resource.height > mode.height)
        {
            WARN("Surface %p dimensions are %ux%u, but screen dimensions are %ux%u.\n",
                    cursor_image, cursor_image->resource.width, cursor_image->resource.height,
                    mode.width, mode.height);
            return WINED3DERR_INVALIDCALL;
        }

        /* TODO: MSDN: Cursor sizes must be a power of 2 */

        /* Do not store the surface's pointer because the application may
         * release it after setting the cursor image. Windows doesn't
         * addref the set surface, so we can't do this either without
         * creating circular refcount dependencies. */
        if (!(device->cursor_texture = wined3d_device_create_cursor_texture(device, cursor_image)))
        {
            ERR("Failed to create cursor texture.\n");
            return WINED3DERR_INVALIDCALL;
        }

        device->cursorWidth = cursor_image->resource.width;
        device->cursorHeight = cursor_image->resource.height;

        if (cursor_image->resource.width == 32 && cursor_image->resource.height == 32)
        {
            UINT mask_size = cursor_image->resource.width * cursor_image->resource.height / 8;
            ICONINFO cursorInfo;
            DWORD *maskBits;
            HCURSOR cursor;

            /* 32-bit user32 cursors ignore the alpha channel if it's all
             * zeroes, and use the mask instead. Fill the mask with all ones
             * to ensure we still get a fully transparent cursor. */
            maskBits = HeapAlloc(GetProcessHeap(), 0, mask_size);
            memset(maskBits, 0xff, mask_size);
            wined3d_surface_map(cursor_image, &map_desc, NULL,
                    WINED3D_MAP_NO_DIRTY_UPDATE | WINED3D_MAP_READONLY);
            TRACE("width: %u height: %u.\n", cursor_image->resource.width, cursor_image->resource.height);

            cursorInfo.fIcon = FALSE;
            cursorInfo.xHotspot = x_hotspot;
            cursorInfo.yHotspot = y_hotspot;
            cursorInfo.hbmMask = CreateBitmap(cursor_image->resource.width, cursor_image->resource.height,
                    1, 1, maskBits);
            cursorInfo.hbmColor = CreateBitmap(cursor_image->resource.width, cursor_image->resource.height,
                    1, 32, map_desc.data);
            wined3d_surface_unmap(cursor_image);
            /* Create our cursor and clean up. */
            cursor = CreateIconIndirect(&cursorInfo);
            if (cursorInfo.hbmMask) DeleteObject(cursorInfo.hbmMask);
            if (cursorInfo.hbmColor) DeleteObject(cursorInfo.hbmColor);
            if (device->hardwareCursor) DestroyCursor(device->hardwareCursor);
            device->hardwareCursor = cursor;
            if (device->bCursorVisible) SetCursor( cursor );
            HeapFree(GetProcessHeap(), 0, maskBits);
        }
    }

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

        if (resource->pool == WINED3D_POOL_MANAGED && !resource->map_count)
        {
            TRACE("Evicting %p.\n", resource);
            resource->resource_ops->resource_unload(resource);
        }
    }

    /* Invalidate stream sources, the buffer(s) may have been evicted. */
    device_invalidate_state(device, STATE_STREAMSRC);
}

static void delete_opengl_contexts(struct wined3d_device *device, struct wined3d_swapchain *swapchain)
{
    struct wined3d_resource *resource, *cursor;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    struct wined3d_shader *shader;

    context = context_acquire(device, NULL);
    gl_info = context->gl_info;

    LIST_FOR_EACH_ENTRY_SAFE(resource, cursor, &device->resources, struct wined3d_resource, resource_list_entry)
    {
        TRACE("Unloading resource %p.\n", resource);

        resource->resource_ops->resource_unload(resource);
    }

    LIST_FOR_EACH_ENTRY(shader, &device->shaders, struct wined3d_shader, shader_list_entry)
    {
        device->shader_backend->shader_destroy(shader);
    }

    if (device->depth_blt_texture)
    {
        gl_info->gl_ops.gl.p_glDeleteTextures(1, &device->depth_blt_texture);
        device->depth_blt_texture = 0;
    }

    device->blitter->free_private(device);
    device->shader_backend->shader_free_private(device);
    destroy_dummy_textures(device, gl_info);

    context_release(context);

    while (device->context_count)
    {
        swapchain_destroy_contexts(device->contexts[0]->swapchain);
    }

    HeapFree(GetProcessHeap(), 0, swapchain->context);
    swapchain->context = NULL;
}

static HRESULT create_primary_opengl_context(struct wined3d_device *device, struct wined3d_swapchain *swapchain)
{
    struct wined3d_context *context;
    struct wined3d_surface *target;
    HRESULT hr;

    if (FAILED(hr = device->shader_backend->shader_alloc_private(device,
            device->adapter->vertex_pipe, device->adapter->fragment_pipe)))
    {
        ERR("Failed to allocate shader private data, hr %#x.\n", hr);
        return hr;
    }

    if (FAILED(hr = device->blitter->alloc_private(device)))
    {
        ERR("Failed to allocate blitter private data, hr %#x.\n", hr);
        device->shader_backend->shader_free_private(device);
        return hr;
    }

    /* Recreate the primary swapchain's context */
    swapchain->context = HeapAlloc(GetProcessHeap(), 0, sizeof(*swapchain->context));
    if (!swapchain->context)
    {
        ERR("Failed to allocate memory for swapchain context array.\n");
        device->blitter->free_private(device);
        device->shader_backend->shader_free_private(device);
        return E_OUTOFMEMORY;
    }

    target = swapchain->back_buffers
            ? surface_from_resource(wined3d_texture_get_sub_resource(swapchain->back_buffers[0], 0))
            : surface_from_resource(wined3d_texture_get_sub_resource(swapchain->front_buffer, 0));
    if (!(context = context_create(swapchain, target, swapchain->ds_format)))
    {
        WARN("Failed to create context.\n");
        device->blitter->free_private(device);
        device->shader_backend->shader_free_private(device);
        HeapFree(GetProcessHeap(), 0, swapchain->context);
        return E_FAIL;
    }

    swapchain->context[0] = context;
    swapchain->num_contexts = 1;
    create_dummy_textures(device, context);
    context_release(context);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_device_reset(struct wined3d_device *device,
        const struct wined3d_swapchain_desc *swapchain_desc, const struct wined3d_display_mode *mode,
        wined3d_device_reset_cb callback, BOOL reset_state)
{
    struct wined3d_resource *resource, *cursor;
    struct wined3d_swapchain *swapchain;
    struct wined3d_display_mode m;
    BOOL DisplayModeChanged;
    BOOL update_desc = FALSE;
    UINT backbuffer_width = swapchain_desc->backbuffer_width;
    UINT backbuffer_height = swapchain_desc->backbuffer_height;
    HRESULT hr = WINED3D_OK;
    unsigned int i;

    TRACE("device %p, swapchain_desc %p, mode %p, callback %p.\n", device, swapchain_desc, mode, callback);

    if (!(swapchain = wined3d_device_get_swapchain(device, 0)))
    {
        ERR("Failed to get the first implicit swapchain.\n");
        return WINED3DERR_INVALIDCALL;
    }
    DisplayModeChanged = swapchain->reapply_mode;

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

    if (device->fb.render_targets)
    {
        for (i = 0; i < device->adapter->gl_info.limits.buffers; ++i)
        {
            wined3d_device_set_rendertarget_view(device, i, NULL, FALSE);
        }
    }
    wined3d_device_set_depth_stencil_view(device, NULL);

    if (device->onscreen_depth_stencil)
    {
        wined3d_surface_decref(device->onscreen_depth_stencil);
        device->onscreen_depth_stencil = NULL;
    }

    if (reset_state)
    {
        LIST_FOR_EACH_ENTRY_SAFE(resource, cursor, &device->resources, struct wined3d_resource, resource_list_entry)
        {
            TRACE("Enumerating resource %p.\n", resource);
            if (FAILED(hr = callback(resource)))
                return hr;
        }
    }

    /* Is it necessary to recreate the gl context? Actually every setting can be changed
     * on an existing gl context, so there's no real need for recreation.
     *
     * TODO: Figure out how Reset influences resources in D3DPOOL_DEFAULT, D3DPOOL_SYSTEMMEMORY and D3DPOOL_MANAGED
     *
     * TODO: Figure out what happens to explicit swapchains, or if we have more than one implicit swapchain
     */
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

    /* No special treatment of these parameters. Just store them */
    swapchain->desc.swap_effect = swapchain_desc->swap_effect;
    swapchain->desc.enable_auto_depth_stencil = swapchain_desc->enable_auto_depth_stencil;
    swapchain->desc.auto_depth_stencil_format = swapchain_desc->auto_depth_stencil_format;
    swapchain->desc.flags = swapchain_desc->flags;
    swapchain->desc.refresh_rate = swapchain_desc->refresh_rate;
    swapchain->desc.swap_interval = swapchain_desc->swap_interval;
    swapchain->desc.auto_restore_display_mode = swapchain_desc->auto_restore_display_mode;

    /* What to do about these? */
    if (swapchain_desc->backbuffer_count
            && swapchain_desc->backbuffer_count != swapchain->desc.backbuffer_count)
        FIXME("Cannot change the back buffer count yet.\n");

    if (swapchain_desc->device_window
            && swapchain_desc->device_window != swapchain->desc.device_window)
    {
        TRACE("Changing the device window from %p to %p.\n",
                swapchain->desc.device_window, swapchain_desc->device_window);
        swapchain->desc.device_window = swapchain_desc->device_window;
        swapchain->device_window = swapchain_desc->device_window;
        wined3d_swapchain_set_window(swapchain, NULL);
    }

    if (mode)
    {
        DisplayModeChanged = TRUE;
        m = *mode;
    }
    else if (swapchain_desc->windowed)
    {
        m = swapchain->original_mode;
    }
    else
    {
        m.width = swapchain_desc->backbuffer_width;
        m.height = swapchain_desc->backbuffer_height;
        m.refresh_rate = swapchain_desc->refresh_rate;
        m.format_id = swapchain_desc->backbuffer_format;
        m.scanline_ordering = WINED3D_SCANLINE_ORDERING_UNKNOWN;
    }

    if (!backbuffer_width || !backbuffer_height)
    {
        /* The application is requesting that either the swapchain width or
         * height be set to the corresponding dimension in the window's
         * client rect. */

        RECT client_rect;

        if (!swapchain_desc->windowed)
            return WINED3DERR_INVALIDCALL;

        if (!GetClientRect(swapchain->device_window, &client_rect))
        {
            ERR("Failed to get client rect, last error %#x.\n", GetLastError());
            return WINED3DERR_INVALIDCALL;
        }

        if (!backbuffer_width)
            backbuffer_width = client_rect.right;

        if (!backbuffer_height)
            backbuffer_height = client_rect.bottom;
    }

    if (backbuffer_width != swapchain->desc.backbuffer_width
            || backbuffer_height != swapchain->desc.backbuffer_height)
    {
        if (!swapchain_desc->windowed)
            DisplayModeChanged = TRUE;

        swapchain->desc.backbuffer_width = backbuffer_width;
        swapchain->desc.backbuffer_height = backbuffer_height;
        update_desc = TRUE;
    }

    if (swapchain_desc->backbuffer_format != WINED3DFMT_UNKNOWN
            && swapchain_desc->backbuffer_format != swapchain->desc.backbuffer_format)
    {
        swapchain->desc.backbuffer_format = swapchain_desc->backbuffer_format;
        update_desc = TRUE;
    }

    if (swapchain_desc->multisample_type != swapchain->desc.multisample_type
            || swapchain_desc->multisample_quality != swapchain->desc.multisample_quality)
    {
        swapchain->desc.multisample_type = swapchain_desc->multisample_type;
        swapchain->desc.multisample_quality = swapchain_desc->multisample_quality;
        update_desc = TRUE;
    }

    if (update_desc)
    {
        UINT i;

        if (FAILED(hr = wined3d_texture_update_desc(swapchain->front_buffer, swapchain->desc.backbuffer_width,
                swapchain->desc.backbuffer_height, swapchain->desc.backbuffer_format,
                swapchain->desc.multisample_type, swapchain->desc.multisample_quality, NULL, 0)))
            return hr;

        for (i = 0; i < swapchain->desc.backbuffer_count; ++i)
        {
            if (FAILED(hr = wined3d_texture_update_desc(swapchain->back_buffers[i], swapchain->desc.backbuffer_width,
                    swapchain->desc.backbuffer_height, swapchain->desc.backbuffer_format,
                    swapchain->desc.multisample_type, swapchain->desc.multisample_quality, NULL, 0)))
                return hr;
        }
    }

    if (device->auto_depth_stencil_view)
    {
        wined3d_rendertarget_view_decref(device->auto_depth_stencil_view);
        device->auto_depth_stencil_view = NULL;
    }
    if (swapchain->desc.enable_auto_depth_stencil)
    {
        struct wined3d_resource_desc surface_desc;
        struct wined3d_surface *surface;

        TRACE("Creating the depth stencil buffer\n");

        surface_desc.resource_type = WINED3D_RTYPE_SURFACE;
        surface_desc.format = swapchain->desc.auto_depth_stencil_format;
        surface_desc.multisample_type = swapchain->desc.multisample_type;
        surface_desc.multisample_quality = swapchain->desc.multisample_quality;
        surface_desc.usage = WINED3DUSAGE_DEPTHSTENCIL;
        surface_desc.pool = WINED3D_POOL_DEFAULT;
        surface_desc.width = swapchain->desc.backbuffer_width;
        surface_desc.height = swapchain->desc.backbuffer_height;
        surface_desc.depth = 1;
        surface_desc.size = 0;

        if (FAILED(hr = device->device_parent->ops->create_swapchain_surface(device->device_parent,
                device->device_parent, &surface_desc, &surface)))
        {
            ERR("Failed to create the auto depth/stencil surface, hr %#x.\n", hr);
            return WINED3DERR_INVALIDCALL;
        }

        hr = wined3d_rendertarget_view_create_from_surface(surface,
                NULL, &wined3d_null_parent_ops, &device->auto_depth_stencil_view);
        wined3d_surface_decref(surface);
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
    if (swapchain->desc.backbuffer_count && FAILED(hr = wined3d_rendertarget_view_create_from_surface(
            surface_from_resource(wined3d_texture_get_sub_resource(swapchain->back_buffers[0], 0)),
            NULL, &wined3d_null_parent_ops, &device->back_buffer_view)))
    {
        ERR("Failed to create rendertarget view, hr %#x.\n", hr);
        return hr;
    }

    if (!swapchain_desc->windowed != !swapchain->desc.windowed
            || DisplayModeChanged)
    {
        if (FAILED(hr = wined3d_set_adapter_display_mode(device->wined3d, device->adapter->ordinal, &m)))
        {
            WARN("Failed to set display mode, hr %#x.\n", hr);
            return WINED3DERR_INVALIDCALL;
        }

        if (!swapchain_desc->windowed)
        {
            if (swapchain->desc.windowed)
            {
                HWND focus_window = device->create_parms.focus_window;
                if (!focus_window)
                    focus_window = swapchain_desc->device_window;
                if (FAILED(hr = wined3d_device_acquire_focus_window(device, focus_window)))
                {
                    ERR("Failed to acquire focus window, hr %#x.\n", hr);
                    return hr;
                }

                /* switch from windowed to fs */
                wined3d_device_setup_fullscreen_window(device, swapchain->device_window,
                        swapchain_desc->backbuffer_width,
                        swapchain_desc->backbuffer_height);
            }
            else
            {
                /* Fullscreen -> fullscreen mode change */
                MoveWindow(swapchain->device_window, 0, 0,
                        swapchain_desc->backbuffer_width,
                        swapchain_desc->backbuffer_height,
                        TRUE);
            }
            swapchain->d3d_mode = m;
        }
        else if (!swapchain->desc.windowed)
        {
            /* Fullscreen -> windowed switch */
            wined3d_device_restore_fullscreen_window(device, swapchain->device_window);
            wined3d_device_release_focus_window(device);
        }
        swapchain->desc.windowed = swapchain_desc->windowed;
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
            delete_opengl_contexts(device, swapchain);

        if (FAILED(hr = state_init(&device->state, &device->fb, &device->adapter->gl_info,
                &device->adapter->d3d_info, WINED3D_STATE_INIT_DEFAULT)))
            ERR("Failed to initialize device state, hr %#x.\n", hr);
        device->update_state = &device->state;

        device_init_swapchain_state(device, swapchain);
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

        state->scissor_rect.top = 0;
        state->scissor_rect.left = 0;
        state->scissor_rect.right = view->width;
        state->scissor_rect.bottom = view->height;
        wined3d_cs_emit_set_scissor_rect(device->cs, &state->scissor_rect);
    }

    swapchain_update_render_to_fbo(swapchain);
    swapchain_update_draw_bindings(swapchain);

    if (reset_state && device->d3d_initialized)
        hr = create_primary_opengl_context(device, swapchain);

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

    list_add_head(&device->resources, &resource->resource_list_entry);
}

static void device_resource_remove(struct wined3d_device *device, struct wined3d_resource *resource)
{
    TRACE("device %p, resource %p.\n", device, resource);

    list_remove(&resource->resource_list_entry);
}

void device_resource_released(struct wined3d_device *device, struct wined3d_resource *resource)
{
    enum wined3d_resource_type type = resource->type;
    unsigned int i;

    TRACE("device %p, resource %p, type %s.\n", device, resource, debug_d3dresourcetype(type));

    context_resource_released(device, resource, type);

    switch (type)
    {
        case WINED3D_RTYPE_SURFACE:
            {
                struct wined3d_surface *surface = surface_from_resource(resource);

                if (!device->d3d_initialized) break;

                for (i = 0; i < device->adapter->gl_info.limits.buffers; ++i)
                {
                    if (wined3d_rendertarget_view_get_surface(device->fb.render_targets[i]) == surface)
                    {
                        ERR("Surface %p is still in use as render target %u.\n", surface, i);
                        device->fb.render_targets[i] = NULL;
                    }
                }

                if (wined3d_rendertarget_view_get_surface(device->fb.depth_stencil) == surface)
                {
                    ERR("Surface %p is still in use as depth/stencil buffer.\n", surface);
                    device->fb.depth_stencil = NULL;
                }
            }
            break;

        case WINED3D_RTYPE_TEXTURE:
        case WINED3D_RTYPE_CUBE_TEXTURE:
        case WINED3D_RTYPE_VOLUME_TEXTURE:
            for (i = 0; i < MAX_COMBINED_SAMPLERS; ++i)
            {
                struct wined3d_texture *texture = wined3d_texture_from_resource(resource);

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

struct wined3d_surface * CDECL wined3d_device_get_surface_from_dc(const struct wined3d_device *device, HDC dc)
{
    struct wined3d_resource *resource;

    TRACE("device %p, dc %p.\n", device, dc);

    if (!dc)
        return NULL;

    LIST_FOR_EACH_ENTRY(resource, &device->resources, struct wined3d_resource, resource_list_entry)
    {
        if (resource->type == WINED3D_RTYPE_SURFACE)
        {
            struct wined3d_surface *s = surface_from_resource(resource);

            if (s->hDC == dc)
            {
                TRACE("Found surface %p for dc %p.\n", s, dc);
                return s;
            }
        }
    }

    return NULL;
}

static int wined3d_sampler_compare(const void *key, const struct wine_rb_entry *entry)
{
    const struct wined3d_sampler *sampler = WINE_RB_ENTRY_VALUE(entry, struct wined3d_sampler, entry);

    return memcmp(&sampler->desc, key, sizeof(sampler->desc));
}

static const struct wine_rb_functions wined3d_sampler_rb_functions =
{
    wined3d_rb_alloc,
    wined3d_rb_realloc,
    wined3d_rb_free,
    wined3d_sampler_compare,
};

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

    if (wine_rb_init(&device->samplers, &wined3d_sampler_rb_functions) == -1)
    {
        ERR("Failed to initialize sampler rbtree.\n");
        return E_OUTOFMEMORY;
    }

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

    device->blitter = adapter->blitter;

    if (FAILED(hr = state_init(&device->state, &device->fb, &adapter->gl_info,
            &adapter->d3d_info, WINED3D_STATE_INIT_DEFAULT)))
    {
        ERR("Failed to initialize device state, hr %#x.\n", hr);
        goto err;
    }
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
    for (i = 0; i < sizeof(device->multistate_funcs) / sizeof(device->multistate_funcs[0]); ++i)
    {
        HeapFree(GetProcessHeap(), 0, device->multistate_funcs[i]);
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

    for (i = 0; i < device->context_count; ++i)
    {
        context = device->contexts[i];
        if(isStateDirty(context, rep)) continue;

        context->dirtyArray[context->numDirtyEntries++] = rep;
        idx = rep / (sizeof(*context->isStateDirty) * CHAR_BIT);
        shift = rep & ((sizeof(*context->isStateDirty) * CHAR_BIT) - 1);
        context->isStateDirty[idx] |= (1 << shift);
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
