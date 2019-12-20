/*
 * Copyright 2002-2003 Jason Edmeades
 * Copyright 2002-2003 Raphael Junqueira
 * Copyright 2005 Oliver Stieber
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
 * Copyright 2011 Henri Verbeet for CodeWeavers
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
#include "wined3d_private.h"

#ifdef __REACTOS__
#include <reactos/undocuser.h>
#endif

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(fps);

static void wined3d_swapchain_destroy_object(void *object)
{
    swapchain_destroy_contexts(object);
}

static void swapchain_cleanup(struct wined3d_swapchain *swapchain)
{
    HRESULT hr;
    UINT i;

    TRACE("Destroying swapchain %p.\n", swapchain);

    wined3d_swapchain_set_gamma_ramp(swapchain, 0, &swapchain->orig_gamma);

    /* Release the swapchain's draw buffers. Make sure swapchain->back_buffers[0]
     * is the last buffer to be destroyed, FindContext() depends on that. */
    if (swapchain->front_buffer)
    {
        wined3d_texture_set_swapchain(swapchain->front_buffer, NULL);
        if (wined3d_texture_decref(swapchain->front_buffer))
            WARN("Something's still holding the front buffer (%p).\n", swapchain->front_buffer);
        swapchain->front_buffer = NULL;
    }

    if (swapchain->back_buffers)
    {
        i = swapchain->desc.backbuffer_count;

        while (i--)
        {
            wined3d_texture_set_swapchain(swapchain->back_buffers[i], NULL);
            if (wined3d_texture_decref(swapchain->back_buffers[i]))
                WARN("Something's still holding back buffer %u (%p).\n", i, swapchain->back_buffers[i]);
        }
        heap_free(swapchain->back_buffers);
        swapchain->back_buffers = NULL;
    }

    wined3d_cs_destroy_object(swapchain->device->cs, wined3d_swapchain_destroy_object, swapchain);
    swapchain->device->cs->ops->finish(swapchain->device->cs, WINED3D_CS_QUEUE_DEFAULT);

    /* Restore the screen resolution if we rendered in fullscreen.
     * This will restore the screen resolution to what it was before creating
     * the swapchain. In case of d3d8 and d3d9 this will be the original
     * desktop resolution. In case of d3d7 this will be a NOP because ddraw
     * sets the resolution before starting up Direct3D, thus orig_width and
     * orig_height will be equal to the modes in the presentation params. */
    if (!swapchain->desc.windowed && swapchain->desc.auto_restore_display_mode)
    {
        if (FAILED(hr = wined3d_set_adapter_display_mode(swapchain->device->wined3d,
                swapchain->device->adapter->ordinal, &swapchain->original_mode)))
            ERR("Failed to restore display mode, hr %#x.\n", hr);

        if (swapchain->desc.flags & WINED3D_SWAPCHAIN_RESTORE_WINDOW_RECT)
        {
            wined3d_device_restore_fullscreen_window(swapchain->device, swapchain->device_window,
                    &swapchain->original_window_rect);
            wined3d_device_release_focus_window(swapchain->device);
        }
    }

    if (swapchain->backup_dc)
    {
        TRACE("Destroying backup wined3d window %p, dc %p.\n", swapchain->backup_wnd, swapchain->backup_dc);

        wined3d_release_dc(swapchain->backup_wnd, swapchain->backup_dc);
        DestroyWindow(swapchain->backup_wnd);
    }
}

ULONG CDECL wined3d_swapchain_incref(struct wined3d_swapchain *swapchain)
{
    ULONG refcount = InterlockedIncrement(&swapchain->ref);

    TRACE("%p increasing refcount to %u.\n", swapchain, refcount);

    return refcount;
}

ULONG CDECL wined3d_swapchain_decref(struct wined3d_swapchain *swapchain)
{
    ULONG refcount = InterlockedDecrement(&swapchain->ref);

    TRACE("%p decreasing refcount to %u.\n", swapchain, refcount);

    if (!refcount)
    {
        struct wined3d_device *device = swapchain->device;

        device->cs->ops->finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);

        swapchain_cleanup(swapchain);
        swapchain->parent_ops->wined3d_object_destroyed(swapchain->parent);
        heap_free(swapchain);
    }

    return refcount;
}

void * CDECL wined3d_swapchain_get_parent(const struct wined3d_swapchain *swapchain)
{
    TRACE("swapchain %p.\n", swapchain);

    return swapchain->parent;
}

void CDECL wined3d_swapchain_set_window(struct wined3d_swapchain *swapchain, HWND window)
{
    if (!window)
        window = swapchain->device_window;
    if (window == swapchain->win_handle)
        return;

    TRACE("Setting swapchain %p window from %p to %p.\n",
            swapchain, swapchain->win_handle, window);
    swapchain->win_handle = window;
}

HRESULT CDECL wined3d_swapchain_present(struct wined3d_swapchain *swapchain,
        const RECT *src_rect, const RECT *dst_rect, HWND dst_window_override,
        DWORD swap_interval, DWORD flags)
{
    static DWORD notified_flags = 0;
    RECT s, d;

    TRACE("swapchain %p, src_rect %s, dst_rect %s, dst_window_override %p, flags %#x.\n",
            swapchain, wine_dbgstr_rect(src_rect), wine_dbgstr_rect(dst_rect),
            dst_window_override, flags);

    if (flags & ~notified_flags)
    {
        FIXME("Ignoring flags %#x.\n", flags & ~notified_flags);
        notified_flags |= flags;
    }

    if (!swapchain->back_buffers)
    {
        WARN("Swapchain doesn't have a backbuffer, returning WINED3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }

    if (!src_rect)
    {
        SetRect(&s, 0, 0, swapchain->desc.backbuffer_width,
                swapchain->desc.backbuffer_height);
        src_rect = &s;
    }

    if (!dst_rect)
    {
        GetClientRect(swapchain->win_handle, &d);
        dst_rect = &d;
    }

    wined3d_cs_emit_present(swapchain->device->cs, swapchain, src_rect,
            dst_rect, dst_window_override, swap_interval, flags);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_swapchain_get_front_buffer_data(const struct wined3d_swapchain *swapchain,
        struct wined3d_texture *dst_texture, unsigned int sub_resource_idx)
{
    RECT src_rect, dst_rect;

    TRACE("swapchain %p, dst_texture %p, sub_resource_idx %u.\n", swapchain, dst_texture, sub_resource_idx);

    SetRect(&src_rect, 0, 0, swapchain->front_buffer->resource.width, swapchain->front_buffer->resource.height);
    dst_rect = src_rect;

    if (swapchain->desc.windowed)
    {
        MapWindowPoints(swapchain->win_handle, NULL, (POINT *)&dst_rect, 2);
        FIXME("Using destination rect %s in windowed mode, this is likely wrong.\n",
                wine_dbgstr_rect(&dst_rect));
    }

    return wined3d_texture_blt(dst_texture, sub_resource_idx, &dst_rect,
            swapchain->front_buffer, 0, &src_rect, 0, NULL, WINED3D_TEXF_POINT);
}

struct wined3d_texture * CDECL wined3d_swapchain_get_back_buffer(const struct wined3d_swapchain *swapchain,
        UINT back_buffer_idx)
{
    TRACE("swapchain %p, back_buffer_idx %u.\n",
            swapchain, back_buffer_idx);

    /* Return invalid if there is no backbuffer array, otherwise it will
     * crash when ddraw is used (there swapchain->back_buffers is always
     * NULL). We need this because this function is called from
     * stateblock_init_default_state() to get the default scissorrect
     * dimensions. */
    if (!swapchain->back_buffers || back_buffer_idx >= swapchain->desc.backbuffer_count)
    {
        WARN("Invalid back buffer index.\n");
        /* Native d3d9 doesn't set NULL here, just as wine's d3d9. But set it
         * here in wined3d to avoid problems in other libs. */
        return NULL;
    }

    TRACE("Returning back buffer %p.\n", swapchain->back_buffers[back_buffer_idx]);

    return swapchain->back_buffers[back_buffer_idx];
}

HRESULT CDECL wined3d_swapchain_get_raster_status(const struct wined3d_swapchain *swapchain,
        struct wined3d_raster_status *raster_status)
{
    TRACE("swapchain %p, raster_status %p.\n", swapchain, raster_status);

    return wined3d_get_adapter_raster_status(swapchain->device->wined3d,
            swapchain->device->adapter->ordinal, raster_status);
}

HRESULT CDECL wined3d_swapchain_get_display_mode(const struct wined3d_swapchain *swapchain,
        struct wined3d_display_mode *mode, enum wined3d_display_rotation *rotation)
{
    HRESULT hr;

    TRACE("swapchain %p, mode %p, rotation %p.\n", swapchain, mode, rotation);

    hr = wined3d_get_adapter_display_mode(swapchain->device->wined3d,
            swapchain->device->adapter->ordinal, mode, rotation);

    TRACE("Returning w %u, h %u, refresh rate %u, format %s.\n",
            mode->width, mode->height, mode->refresh_rate, debug_d3dformat(mode->format_id));

    return hr;
}

struct wined3d_device * CDECL wined3d_swapchain_get_device(const struct wined3d_swapchain *swapchain)
{
    TRACE("swapchain %p.\n", swapchain);

    return swapchain->device;
}

void CDECL wined3d_swapchain_get_desc(const struct wined3d_swapchain *swapchain,
        struct wined3d_swapchain_desc *desc)
{
    TRACE("swapchain %p, desc %p.\n", swapchain, desc);

    *desc = swapchain->desc;
}

HRESULT CDECL wined3d_swapchain_set_gamma_ramp(const struct wined3d_swapchain *swapchain,
        DWORD flags, const struct wined3d_gamma_ramp *ramp)
{
    HDC dc;

    TRACE("swapchain %p, flags %#x, ramp %p.\n", swapchain, flags, ramp);

    if (flags)
        FIXME("Ignoring flags %#x.\n", flags);

    dc = GetDCEx(swapchain->device_window, 0, DCX_USESTYLE | DCX_CACHE);
    SetDeviceGammaRamp(dc, (void *)ramp);
    ReleaseDC(swapchain->device_window, dc);

    return WINED3D_OK;
}

void CDECL wined3d_swapchain_set_palette(struct wined3d_swapchain *swapchain, struct wined3d_palette *palette)
{
    TRACE("swapchain %p, palette %p.\n", swapchain, palette);
    swapchain->palette = palette;
}

HRESULT CDECL wined3d_swapchain_get_gamma_ramp(const struct wined3d_swapchain *swapchain,
        struct wined3d_gamma_ramp *ramp)
{
    HDC dc;

    TRACE("swapchain %p, ramp %p.\n", swapchain, ramp);

    dc = GetDCEx(swapchain->device_window, 0, DCX_USESTYLE | DCX_CACHE);
    GetDeviceGammaRamp(dc, ramp);
    ReleaseDC(swapchain->device_window, dc);

    return WINED3D_OK;
}

/* A GL context is provided by the caller */
static void swapchain_blit(const struct wined3d_swapchain *swapchain,
        struct wined3d_context *context, const RECT *src_rect, const RECT *dst_rect)
{
    struct wined3d_texture *texture = swapchain->back_buffers[0];
    struct wined3d_surface *back_buffer = texture->sub_resources[0].u.surface;
    struct wined3d_device *device = swapchain->device;
    enum wined3d_texture_filter_type filter;
    DWORD location;

    TRACE("swapchain %p, context %p, src_rect %s, dst_rect %s.\n",
            swapchain, context, wine_dbgstr_rect(src_rect), wine_dbgstr_rect(dst_rect));

    if ((src_rect->right - src_rect->left == dst_rect->right - dst_rect->left
            && src_rect->bottom - src_rect->top == dst_rect->bottom - dst_rect->top)
            || is_complex_fixup(texture->resource.format->color_fixup))
        filter = WINED3D_TEXF_NONE;
    else
        filter = WINED3D_TEXF_LINEAR;

    location = WINED3D_LOCATION_TEXTURE_RGB;
    if (texture->resource.multisample_type)
        location = WINED3D_LOCATION_RB_RESOLVED;

    wined3d_texture_validate_location(texture, 0, WINED3D_LOCATION_DRAWABLE);
    device->blitter->ops->blitter_blit(device->blitter, WINED3D_BLIT_OP_COLOR_BLIT, context, back_buffer,
            location, src_rect, back_buffer, WINED3D_LOCATION_DRAWABLE, dst_rect, NULL, filter);
    wined3d_texture_invalidate_location(texture, 0, WINED3D_LOCATION_DRAWABLE);
}

/* Context activation is done by the caller. */
static void wined3d_swapchain_rotate(struct wined3d_swapchain *swapchain, struct wined3d_context *context)
{
    struct wined3d_texture_sub_resource *sub_resource;
    struct wined3d_texture *texture, *texture_prev;
    struct gl_texture tex0;
    GLuint rb0;
    DWORD locations0;
    unsigned int i;
    static const DWORD supported_locations = WINED3D_LOCATION_TEXTURE_RGB | WINED3D_LOCATION_RB_MULTISAMPLE;

    if (swapchain->desc.backbuffer_count < 2 || !swapchain->render_to_fbo)
        return;

    texture_prev = swapchain->back_buffers[0];

    /* Back buffer 0 is already in the draw binding. */
    tex0 = texture_prev->texture_rgb;
    rb0 = texture_prev->rb_multisample;
    locations0 = texture_prev->sub_resources[0].locations;

    for (i = 1; i < swapchain->desc.backbuffer_count; ++i)
    {
        texture = swapchain->back_buffers[i];
        sub_resource = &texture->sub_resources[0];

        if (!(sub_resource->locations & supported_locations))
            wined3d_texture_load_location(texture, 0, context, texture->resource.draw_binding);

        texture_prev->texture_rgb = texture->texture_rgb;
        texture_prev->rb_multisample = texture->rb_multisample;

        wined3d_texture_validate_location(texture_prev, 0, sub_resource->locations & supported_locations);
        wined3d_texture_invalidate_location(texture_prev, 0, ~(sub_resource->locations & supported_locations));

        texture_prev = texture;
    }

    texture_prev->texture_rgb = tex0;
    texture_prev->rb_multisample = rb0;

    wined3d_texture_validate_location(texture_prev, 0, locations0 & supported_locations);
    wined3d_texture_invalidate_location(texture_prev, 0, ~(locations0 & supported_locations));

    device_invalidate_state(swapchain->device, STATE_FRAMEBUFFER);
}

static void swapchain_gl_present(struct wined3d_swapchain *swapchain,
        const RECT *src_rect, const RECT *dst_rect, DWORD flags)
{
    struct wined3d_texture *back_buffer = swapchain->back_buffers[0];
    const struct wined3d_fb_state *fb = &swapchain->device->cs->fb;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_texture *logo_texture;
    struct wined3d_context *context;
    BOOL render_to_fbo;

    context = context_acquire(swapchain->device, back_buffer, 0);
    if (!context->valid)
    {
        context_release(context);
        WARN("Invalid context, skipping present.\n");
        return;
    }

    gl_info = context->gl_info;

    if ((logo_texture = swapchain->device->logo_texture))
    {
        RECT rect = {0, 0, logo_texture->resource.width, logo_texture->resource.height};

        /* Blit the logo into the upper left corner of the drawable. */
        wined3d_texture_blt(back_buffer, 0, &rect, logo_texture, 0, &rect,
                WINED3D_BLT_SRC_CKEY, NULL, WINED3D_TEXF_POINT);
    }

    if (swapchain->device->bCursorVisible && swapchain->device->cursor_texture
            && !swapchain->device->hardwareCursor)
    {
        RECT dst_rect =
        {
            swapchain->device->xScreenSpace - swapchain->device->xHotSpot,
            swapchain->device->yScreenSpace - swapchain->device->yHotSpot,
            swapchain->device->xScreenSpace + swapchain->device->cursorWidth - swapchain->device->xHotSpot,
            swapchain->device->yScreenSpace + swapchain->device->cursorHeight - swapchain->device->yHotSpot,
        };
        RECT src_rect =
        {
            0, 0,
            swapchain->device->cursor_texture->resource.width,
            swapchain->device->cursor_texture->resource.height
        };
        const RECT clip_rect = {0, 0, back_buffer->resource.width, back_buffer->resource.height};

        TRACE("Rendering the software cursor.\n");

        if (swapchain->desc.windowed)
            MapWindowPoints(NULL, swapchain->win_handle, (POINT *)&dst_rect, 2);
        if (wined3d_clip_blit(&clip_rect, &dst_rect, &src_rect))
            wined3d_texture_blt(back_buffer, 0, &dst_rect,
                    swapchain->device->cursor_texture, 0, &src_rect,
                    WINED3D_BLT_ALPHA_TEST, NULL, WINED3D_TEXF_POINT);
    }

    TRACE("Presenting HDC %p.\n", context->hdc);

    if (!(render_to_fbo = swapchain->render_to_fbo)
            && (src_rect->left || src_rect->top
            || src_rect->right != swapchain->desc.backbuffer_width
            || src_rect->bottom != swapchain->desc.backbuffer_height
            || dst_rect->left || dst_rect->top
            || dst_rect->right != swapchain->desc.backbuffer_width
            || dst_rect->bottom != swapchain->desc.backbuffer_height))
        render_to_fbo = TRUE;

    /* Rendering to a window of different size, presenting partial rectangles,
     * or rendering to a different window needs help from FBO_blit or a textured
     * draw. Render the swapchain to a FBO in the future.
     *
     * Note that FBO_blit from the backbuffer to the frontbuffer cannot solve
     * all these issues - this fails if the window is smaller than the backbuffer.
     */
    if (!swapchain->render_to_fbo && render_to_fbo && wined3d_settings.offscreen_rendering_mode == ORM_FBO)
    {
        wined3d_texture_load_location(back_buffer, 0, context, WINED3D_LOCATION_TEXTURE_RGB);
        wined3d_texture_invalidate_location(back_buffer, 0, WINED3D_LOCATION_DRAWABLE);
        swapchain->render_to_fbo = TRUE;
        swapchain_update_draw_bindings(swapchain);
    }
    else
    {
        wined3d_texture_load_location(back_buffer, 0, context, back_buffer->resource.draw_binding);
    }

    if (swapchain->render_to_fbo)
        swapchain_blit(swapchain, context, src_rect, dst_rect);

#if !defined(STAGING_CSMT)
    if (swapchain->num_contexts > 1)
#else  /* STAGING_CSMT */
    if (swapchain->num_contexts > 1 && !wined3d_settings.cs_multithreaded)
#endif /* STAGING_CSMT */
        gl_info->gl_ops.gl.p_glFinish();

    /* call wglSwapBuffers through the gl table to avoid confusing the Steam overlay */
    gl_info->gl_ops.wgl.p_wglSwapBuffers(context->hdc);

    wined3d_swapchain_rotate(swapchain, context);

    TRACE("SwapBuffers called, Starting new frame\n");
    /* FPS support */
    if (TRACE_ON(fps))
    {
        DWORD time = GetTickCount();
        ++swapchain->frames;

        /* every 1.5 seconds */
        if (time - swapchain->prev_time > 1500)
        {
            TRACE_(fps)("%p @ approx %.2ffps\n",
                    swapchain, 1000.0 * swapchain->frames / (time - swapchain->prev_time));
            swapchain->prev_time = time;
            swapchain->frames = 0;
        }
    }

    wined3d_texture_validate_location(swapchain->front_buffer, 0, WINED3D_LOCATION_DRAWABLE);
    wined3d_texture_invalidate_location(swapchain->front_buffer, 0, ~WINED3D_LOCATION_DRAWABLE);
    /* If the swapeffect is DISCARD, the back buffer is undefined. That means the SYSMEM
     * and INTEXTURE copies can keep their old content if they have any defined content.
     * If the swapeffect is COPY, the content remains the same.
     *
     * The FLIP swap effect is not implemented yet. We could mark WINED3D_LOCATION_DRAWABLE
     * up to date and hope WGL flipped front and back buffers and read this data into
     * the FBO. Don't bother about this for now. */
    if (swapchain->desc.swap_effect == WINED3D_SWAP_EFFECT_DISCARD
            || swapchain->desc.swap_effect == WINED3D_SWAP_EFFECT_FLIP_DISCARD)
        wined3d_texture_validate_location(swapchain->back_buffers[swapchain->desc.backbuffer_count - 1],
                0, WINED3D_LOCATION_DISCARDED);

    if (fb->depth_stencil)
    {
        struct wined3d_surface *ds = wined3d_rendertarget_view_get_surface(fb->depth_stencil);

        if (ds && (swapchain->desc.flags & WINED3D_SWAPCHAIN_DISCARD_DEPTHSTENCIL
                || ds->container->flags & WINED3D_TEXTURE_DISCARD))
            wined3d_texture_validate_location(ds->container,
                    fb->depth_stencil->sub_resource_idx, WINED3D_LOCATION_DISCARDED);
    }

    context_release(context);
}

static void swapchain_gl_frontbuffer_updated(struct wined3d_swapchain *swapchain)
{
    struct wined3d_texture *front_buffer = swapchain->front_buffer;
    struct wined3d_context *context;

    context = context_acquire(swapchain->device, front_buffer, 0);
    wined3d_texture_load_location(front_buffer, 0, context, front_buffer->resource.draw_binding);
    context_release(context);
    SetRectEmpty(&swapchain->front_buffer_update);
}

static const struct wined3d_swapchain_ops swapchain_gl_ops =
{
    swapchain_gl_present,
    swapchain_gl_frontbuffer_updated,
};

static void swapchain_gdi_frontbuffer_updated(struct wined3d_swapchain *swapchain)
{
    struct wined3d_surface *front;
    POINT offset = {0, 0};
    HDC src_dc, dst_dc;
    RECT draw_rect;
    HWND window;

    TRACE("swapchain %p.\n", swapchain);

    front = swapchain->front_buffer->sub_resources[0].u.surface;
    if (swapchain->palette)
        wined3d_palette_apply_to_dc(swapchain->palette, front->dc);

    if (front->container->resource.map_count)
        ERR("Trying to blit a mapped surface.\n");

    TRACE("Copying surface %p to screen.\n", front);

    src_dc = front->dc;
    window = swapchain->win_handle;
    dst_dc = GetDCEx(window, 0, DCX_CLIPSIBLINGS | DCX_CACHE);

    /* Front buffer coordinates are screen coordinates. Map them to the
     * destination window if not fullscreened. */
    if (swapchain->desc.windowed)
        ClientToScreen(window, &offset);

    TRACE("offset %s.\n", wine_dbgstr_point(&offset));

    SetRect(&draw_rect, 0, 0, swapchain->front_buffer->resource.width,
            swapchain->front_buffer->resource.height);
    IntersectRect(&draw_rect, &draw_rect, &swapchain->front_buffer_update);

    BitBlt(dst_dc, draw_rect.left - offset.x, draw_rect.top - offset.y,
            draw_rect.right - draw_rect.left, draw_rect.bottom - draw_rect.top,
            src_dc, draw_rect.left, draw_rect.top, SRCCOPY);
    ReleaseDC(window, dst_dc);

    SetRectEmpty(&swapchain->front_buffer_update);
}

static void swapchain_gdi_present(struct wined3d_swapchain *swapchain,
        const RECT *src_rect, const RECT *dst_rect, DWORD flags)
{
    struct wined3d_surface *front, *back;
    HBITMAP bitmap;
    void *data;
    HDC dc;

    front = swapchain->front_buffer->sub_resources[0].u.surface;
    back = swapchain->back_buffers[0]->sub_resources[0].u.surface;

    /* Flip the surface data. */
    dc = front->dc;
    bitmap = front->bitmap;
    data = front->container->resource.heap_memory;

    front->dc = back->dc;
    front->bitmap = back->bitmap;
    front->container->resource.heap_memory = back->container->resource.heap_memory;

    back->dc = dc;
    back->bitmap = bitmap;
    back->container->resource.heap_memory = data;

    /* FPS support */
    if (TRACE_ON(fps))
    {
        static LONG prev_time, frames;
        DWORD time = GetTickCount();

        ++frames;

        /* every 1.5 seconds */
        if (time - prev_time > 1500)
        {
            TRACE_(fps)("@ approx %.2ffps\n", 1000.0 * frames / (time - prev_time));
            prev_time = time;
            frames = 0;
        }
    }

    SetRect(&swapchain->front_buffer_update, 0, 0,
            swapchain->front_buffer->resource.width,
            swapchain->front_buffer->resource.height);
    swapchain_gdi_frontbuffer_updated(swapchain);
}

static const struct wined3d_swapchain_ops swapchain_gdi_ops =
{
    swapchain_gdi_present,
    swapchain_gdi_frontbuffer_updated,
};

static void swapchain_update_render_to_fbo(struct wined3d_swapchain *swapchain)
{
    if (wined3d_settings.offscreen_rendering_mode != ORM_FBO)
        return;

    if (!swapchain->desc.backbuffer_count)
    {
        TRACE("Single buffered rendering.\n");
        swapchain->render_to_fbo = FALSE;
        return;
    }

    TRACE("Rendering to FBO.\n");
    swapchain->render_to_fbo = TRUE;
}

static void wined3d_swapchain_apply_sample_count_override(const struct wined3d_swapchain *swapchain,
        enum wined3d_format_id format_id, enum wined3d_multisample_type *type, DWORD *quality)
{
    const struct wined3d_gl_info *gl_info;
    const struct wined3d_format *format;
    enum wined3d_multisample_type t;

    if (wined3d_settings.sample_count == ~0u)
        return;

    gl_info = &swapchain->device->adapter->gl_info;
    if (!(format = wined3d_get_format(gl_info, format_id, WINED3DUSAGE_RENDERTARGET)))
        return;

    if ((t = min(wined3d_settings.sample_count, gl_info->limits.samples)))
        while (!(format->multisample_types & 1u << (t - 1)))
            ++t;
    TRACE("Using sample count %u.\n", t);
    *type = t;
    *quality = 0;
}

static void wined3d_swapchain_update_swap_interval_cs(void *object)
{
    struct wined3d_swapchain *swapchain = object;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    int swap_interval;

    context = context_acquire(swapchain->device, swapchain->front_buffer, 0);
    gl_info = context->gl_info;

    switch (swapchain->desc.swap_interval)
    {
        case WINED3DPRESENT_INTERVAL_IMMEDIATE:
            swap_interval = 0;
            break;
        case WINED3DPRESENT_INTERVAL_DEFAULT:
        case WINED3DPRESENT_INTERVAL_ONE:
            swap_interval = 1;
            break;
        case WINED3DPRESENT_INTERVAL_TWO:
            swap_interval = 2;
            break;
        case WINED3DPRESENT_INTERVAL_THREE:
            swap_interval = 3;
            break;
        case WINED3DPRESENT_INTERVAL_FOUR:
            swap_interval = 4;
            break;
        default:
            FIXME("Unhandled present interval %#x.\n", swapchain->desc.swap_interval);
            swap_interval = 1;
    }

    if (gl_info->supported[WGL_EXT_SWAP_CONTROL])
    {
        if (!GL_EXTCALL(wglSwapIntervalEXT(swap_interval)))
            ERR("wglSwapIntervalEXT failed to set swap interval %d for context %p, last error %#x\n",
                swap_interval, context, GetLastError());
    }

    context_release(context);
}

static void wined3d_swapchain_cs_init(void *object)
{
    struct wined3d_swapchain *swapchain = object;
    const struct wined3d_gl_info *gl_info;
    unsigned int i;

    static const enum wined3d_format_id formats[] =
    {
        WINED3DFMT_D24_UNORM_S8_UINT,
        WINED3DFMT_D32_UNORM,
        WINED3DFMT_R24_UNORM_X8_TYPELESS,
        WINED3DFMT_D16_UNORM,
        WINED3DFMT_S1_UINT_D15_UNORM,
    };

    gl_info = &swapchain->device->adapter->gl_info;

    /* Without ORM_FBO, switching the depth/stencil format is hard. Always
     * request a depth/stencil buffer in the likely case it's needed later. */
    for (i = 0; i < ARRAY_SIZE(formats); ++i)
    {
        swapchain->ds_format = wined3d_get_format(gl_info, formats[i], WINED3DUSAGE_DEPTHSTENCIL);
        if ((swapchain->context[0] = context_create(swapchain, swapchain->front_buffer, swapchain->ds_format)))
            break;
        TRACE("Depth stencil format %s is not supported, trying next format.\n", debug_d3dformat(formats[i]));
    }

    if (!swapchain->context[0])
    {
        WARN("Failed to create context.\n");
        return;
    }
    swapchain->num_contexts = 1;

    if (wined3d_settings.offscreen_rendering_mode != ORM_FBO
            && (!swapchain->desc.enable_auto_depth_stencil
            || swapchain->desc.auto_depth_stencil_format != swapchain->ds_format->id))
        FIXME("Add OpenGL context recreation support.\n");

    context_release(swapchain->context[0]);

    wined3d_swapchain_update_swap_interval_cs(swapchain);
}

static HRESULT swapchain_init(struct wined3d_swapchain *swapchain, struct wined3d_device *device,
        struct wined3d_swapchain_desc *desc, void *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_adapter *adapter = device->adapter;
    struct wined3d_resource_desc texture_desc;
    BOOL displaymode_set = FALSE;
    DWORD texture_flags = 0;
    RECT client_rect;
    HWND window;
    HRESULT hr;
    UINT i;

    if (desc->backbuffer_count > 1)
    {
        FIXME("The application requested more than one back buffer, this is not properly supported.\n"
                "Please configure the application to use double buffering (1 back buffer) if possible.\n");
    }

    if (desc->swap_effect != WINED3D_SWAP_EFFECT_DISCARD
            && desc->swap_effect != WINED3D_SWAP_EFFECT_SEQUENTIAL
            && desc->swap_effect != WINED3D_SWAP_EFFECT_COPY)
        FIXME("Unimplemented swap effect %#x.\n", desc->swap_effect);

    if (device->wined3d->flags & WINED3D_NO3D)
        swapchain->swapchain_ops = &swapchain_gdi_ops;
    else
        swapchain->swapchain_ops = &swapchain_gl_ops;

    window = desc->device_window ? desc->device_window : device->create_parms.focus_window;

    swapchain->device = device;
    swapchain->parent = parent;
    swapchain->parent_ops = parent_ops;
    swapchain->ref = 1;
    swapchain->win_handle = window;
    swapchain->device_window = window;

    if (FAILED(hr = wined3d_get_adapter_display_mode(device->wined3d,
            adapter->ordinal, &swapchain->original_mode, NULL)))
    {
        ERR("Failed to get current display mode, hr %#x.\n", hr);
        goto err;
    }
    GetWindowRect(window, &swapchain->original_window_rect);

    GetClientRect(window, &client_rect);
    if (desc->windowed)
    {
        if (!desc->backbuffer_width)
        {
            desc->backbuffer_width = client_rect.right;
            TRACE("Updating width to %u.\n", desc->backbuffer_width);
        }

        if (!desc->backbuffer_height)
        {
            desc->backbuffer_height = client_rect.bottom;
            TRACE("Updating height to %u.\n", desc->backbuffer_height);
        }

        if (desc->backbuffer_format == WINED3DFMT_UNKNOWN)
        {
            desc->backbuffer_format = swapchain->original_mode.format_id;
            TRACE("Updating format to %s.\n", debug_d3dformat(swapchain->original_mode.format_id));
        }
    }
    swapchain->desc = *desc;
    wined3d_swapchain_apply_sample_count_override(swapchain, swapchain->desc.backbuffer_format,
            &swapchain->desc.multisample_type, &swapchain->desc.multisample_quality);
    swapchain_update_render_to_fbo(swapchain);

    TRACE("Creating front buffer.\n");

    texture_desc.resource_type = WINED3D_RTYPE_TEXTURE_2D;
    texture_desc.format = swapchain->desc.backbuffer_format;
    texture_desc.multisample_type = swapchain->desc.multisample_type;
    texture_desc.multisample_quality = swapchain->desc.multisample_quality;
    texture_desc.usage = 0;
    texture_desc.access = WINED3D_RESOURCE_ACCESS_GPU;
    texture_desc.width = swapchain->desc.backbuffer_width;
    texture_desc.height = swapchain->desc.backbuffer_height;
    texture_desc.depth = 1;
    texture_desc.size = 0;

    if (swapchain->desc.flags & WINED3D_SWAPCHAIN_GDI_COMPATIBLE)
        texture_flags |= WINED3D_TEXTURE_CREATE_GET_DC;

    if (FAILED(hr = device->device_parent->ops->create_swapchain_texture(device->device_parent,
            parent, &texture_desc, texture_flags, &swapchain->front_buffer)))
    {
        WARN("Failed to create front buffer, hr %#x.\n", hr);
        goto err;
    }

    wined3d_texture_set_swapchain(swapchain->front_buffer, swapchain);
    if (!(device->wined3d->flags & WINED3D_NO3D))
    {
        wined3d_texture_validate_location(swapchain->front_buffer, 0, WINED3D_LOCATION_DRAWABLE);
        wined3d_texture_invalidate_location(swapchain->front_buffer, 0, ~WINED3D_LOCATION_DRAWABLE);
    }

    /* MSDN says we're only allowed a single fullscreen swapchain per device,
     * so we should really check to see if there is a fullscreen swapchain
     * already. Does a single head count as full screen? */
    if (!desc->windowed)
    {
        if (desc->flags & WINED3D_SWAPCHAIN_ALLOW_MODE_SWITCH)
        {
            /* Change the display settings */
            swapchain->d3d_mode.width = desc->backbuffer_width;
            swapchain->d3d_mode.height = desc->backbuffer_height;
            swapchain->d3d_mode.format_id = desc->backbuffer_format;
            swapchain->d3d_mode.refresh_rate = desc->refresh_rate;
            swapchain->d3d_mode.scanline_ordering = WINED3D_SCANLINE_ORDERING_UNKNOWN;

            if (FAILED(hr = wined3d_set_adapter_display_mode(device->wined3d,
                    adapter->ordinal, &swapchain->d3d_mode)))
            {
                WARN("Failed to set display mode, hr %#x.\n", hr);
                goto err;
            }
            displaymode_set = TRUE;
        }
        else
        {
            swapchain->d3d_mode = swapchain->original_mode;
        }
    }

    if (!(device->wined3d->flags & WINED3D_NO3D))
    {
        if (!(swapchain->context = heap_alloc(sizeof(*swapchain->context))))
        {
            ERR("Failed to create the context array.\n");
            hr = E_OUTOFMEMORY;
            goto err;
        }

        wined3d_cs_init_object(device->cs, wined3d_swapchain_cs_init, swapchain);
        device->cs->ops->finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);

        if (!swapchain->context[0])
        {
            hr = WINED3DERR_NOTAVAILABLE;
            goto err;
        }
    }

    if (swapchain->desc.backbuffer_count > 0)
    {
        if (!(swapchain->back_buffers = heap_calloc(swapchain->desc.backbuffer_count,
                sizeof(*swapchain->back_buffers))))
        {
            ERR("Failed to allocate backbuffer array memory.\n");
            hr = E_OUTOFMEMORY;
            goto err;
        }

        texture_desc.usage = swapchain->desc.backbuffer_usage;
        for (i = 0; i < swapchain->desc.backbuffer_count; ++i)
        {
            TRACE("Creating back buffer %u.\n", i);
            if (FAILED(hr = device->device_parent->ops->create_swapchain_texture(device->device_parent,
                    parent, &texture_desc, texture_flags, &swapchain->back_buffers[i])))
            {
                WARN("Failed to create back buffer %u, hr %#x.\n", i, hr);
                swapchain->desc.backbuffer_count = i;
                goto err;
            }
            wined3d_texture_set_swapchain(swapchain->back_buffers[i], swapchain);
        }
    }

    /* Swapchains share the depth/stencil buffer, so only create a single depthstencil surface. */
    if (desc->enable_auto_depth_stencil && !(device->wined3d->flags & WINED3D_NO3D))
    {
        TRACE("Creating depth/stencil buffer.\n");
        if (!device->auto_depth_stencil_view)
        {
            struct wined3d_view_desc desc;
            struct wined3d_texture *ds;

            texture_desc.format = swapchain->desc.auto_depth_stencil_format;
            texture_desc.usage = WINED3DUSAGE_DEPTHSTENCIL;

            if (FAILED(hr = device->device_parent->ops->create_swapchain_texture(device->device_parent,
                    device->device_parent, &texture_desc, texture_flags, &ds)))
            {
                WARN("Failed to create the auto depth/stencil surface, hr %#x.\n", hr);
                goto err;
            }

            desc.format_id = ds->resource.format->id;
            desc.flags = 0;
            desc.u.texture.level_idx = 0;
            desc.u.texture.level_count = 1;
            desc.u.texture.layer_idx = 0;
            desc.u.texture.layer_count = 1;
            hr = wined3d_rendertarget_view_create(&desc, &ds->resource, NULL, &wined3d_null_parent_ops,
                    &device->auto_depth_stencil_view);
            wined3d_texture_decref(ds);
            if (FAILED(hr))
            {
                ERR("Failed to create rendertarget view, hr %#x.\n", hr);
                goto err;
            }
        }
    }

    wined3d_swapchain_get_gamma_ramp(swapchain, &swapchain->orig_gamma);

    return WINED3D_OK;

err:
    if (displaymode_set)
    {
        if (FAILED(wined3d_set_adapter_display_mode(device->wined3d,
                adapter->ordinal, &swapchain->original_mode)))
            ERR("Failed to restore display mode.\n");
        ClipCursor(NULL);
    }

    if (swapchain->back_buffers)
    {
        for (i = 0; i < swapchain->desc.backbuffer_count; ++i)
        {
            if (swapchain->back_buffers[i])
            {
                wined3d_texture_set_swapchain(swapchain->back_buffers[i], NULL);
                wined3d_texture_decref(swapchain->back_buffers[i]);
            }
        }
        heap_free(swapchain->back_buffers);
    }

    wined3d_cs_destroy_object(swapchain->device->cs, wined3d_swapchain_destroy_object, swapchain);
    swapchain->device->cs->ops->finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);

    if (swapchain->front_buffer)
    {
        wined3d_texture_set_swapchain(swapchain->front_buffer, NULL);
        wined3d_texture_decref(swapchain->front_buffer);
    }

    return hr;
}

HRESULT CDECL wined3d_swapchain_create(struct wined3d_device *device, struct wined3d_swapchain_desc *desc,
        void *parent, const struct wined3d_parent_ops *parent_ops, struct wined3d_swapchain **swapchain)
{
    struct wined3d_swapchain *object;
    HRESULT hr;

    TRACE("device %p, desc %p, parent %p, parent_ops %p, swapchain %p.\n",
            device, desc, parent, parent_ops, swapchain);

    if (!(object = heap_alloc_zero(sizeof(*object))))
        return E_OUTOFMEMORY;

    hr = swapchain_init(object, device, desc, parent, parent_ops);
    if (FAILED(hr))
    {
        WARN("Failed to initialize swapchain, hr %#x.\n", hr);
        heap_free(object);
        return hr;
    }

    TRACE("Created swapchain %p.\n", object);
    *swapchain = object;

    return WINED3D_OK;
}

static struct wined3d_context *swapchain_create_context(struct wined3d_swapchain *swapchain)
{
    struct wined3d_context **ctx_array;
    struct wined3d_context *ctx;

    TRACE("Creating a new context for swapchain %p, thread %u.\n", swapchain, GetCurrentThreadId());

    if (!(ctx = context_create(swapchain, swapchain->front_buffer, swapchain->ds_format)))
    {
        ERR("Failed to create a new context for the swapchain\n");
        return NULL;
    }
    context_release(ctx);

    if (!(ctx_array = heap_calloc(swapchain->num_contexts + 1, sizeof(*ctx_array))))
    {
        ERR("Out of memory when trying to allocate a new context array\n");
        context_destroy(swapchain->device, ctx);
        return NULL;
    }
    memcpy(ctx_array, swapchain->context, sizeof(*ctx_array) * swapchain->num_contexts);
    heap_free(swapchain->context);
    ctx_array[swapchain->num_contexts] = ctx;
    swapchain->context = ctx_array;
    swapchain->num_contexts++;

    TRACE("Returning context %p\n", ctx);
    return ctx;
}

void swapchain_destroy_contexts(struct wined3d_swapchain *swapchain)
{
    unsigned int i;

    for (i = 0; i < swapchain->num_contexts; ++i)
    {
        context_destroy(swapchain->device, swapchain->context[i]);
    }
    heap_free(swapchain->context);
    swapchain->num_contexts = 0;
    swapchain->context = NULL;
}

struct wined3d_context *swapchain_get_context(struct wined3d_swapchain *swapchain)
{
    DWORD tid = GetCurrentThreadId();
    unsigned int i;

    for (i = 0; i < swapchain->num_contexts; ++i)
    {
        if (swapchain->context[i]->tid == tid)
            return swapchain->context[i];
    }

    /* Create a new context for the thread */
    return swapchain_create_context(swapchain);
}

HDC swapchain_get_backup_dc(struct wined3d_swapchain *swapchain)
{
    if (!swapchain->backup_dc)
    {
        TRACE("Creating the backup window for swapchain %p.\n", swapchain);

        if (!(swapchain->backup_wnd = CreateWindowA(WINED3D_OPENGL_WINDOW_CLASS_NAME, "WineD3D fake window",
                WS_OVERLAPPEDWINDOW, 10, 10, 10, 10, NULL, NULL, NULL, NULL)))
        {
            ERR("Failed to create a window.\n");
            return NULL;
        }

        if (!(swapchain->backup_dc = GetDC(swapchain->backup_wnd)))
        {
            ERR("Failed to get a DC.\n");
            DestroyWindow(swapchain->backup_wnd);
            swapchain->backup_wnd = NULL;
            return NULL;
        }
    }

    return swapchain->backup_dc;
}

void swapchain_update_draw_bindings(struct wined3d_swapchain *swapchain)
{
    UINT i;

    wined3d_resource_update_draw_binding(&swapchain->front_buffer->resource);

    for (i = 0; i < swapchain->desc.backbuffer_count; ++i)
    {
        wined3d_resource_update_draw_binding(&swapchain->back_buffers[i]->resource);
    }
}

void swapchain_update_swap_interval(struct wined3d_swapchain *swapchain)
{
    wined3d_cs_init_object(swapchain->device->cs, wined3d_swapchain_update_swap_interval_cs, swapchain);
}

void wined3d_swapchain_activate(struct wined3d_swapchain *swapchain, BOOL activate)
{
    struct wined3d_device *device = swapchain->device;
    BOOL filter_messages = device->filter_messages;

    /* This code is not protected by the wined3d mutex, so it may run while
     * wined3d_device_reset is active. Testing on Windows shows that changing
     * focus during resets and resetting during focus change events causes
     * the application to crash with an invalid memory access. */

    device->filter_messages = !(device->wined3d->flags & WINED3D_FOCUS_MESSAGES);

    if (activate)
    {
        if (!(device->create_parms.flags & WINED3DCREATE_NOWINDOWCHANGES))
        {
            /* The d3d versions do not agree on the exact messages here. D3d8 restores
             * the window but leaves the size untouched, d3d9 sets the size on an
             * invisible window, generates messages but doesn't change the window
             * properties. The implementation follows d3d9.
             *
             * Guild Wars 1 wants a WINDOWPOSCHANGED message on the device window to
             * resume drawing after a focus loss. */
            SetWindowPos(swapchain->device_window, NULL, 0, 0,
                    swapchain->desc.backbuffer_width, swapchain->desc.backbuffer_height,
                    SWP_NOACTIVATE | SWP_NOZORDER);
        }

        if (device->wined3d->flags & WINED3D_RESTORE_MODE_ON_ACTIVATE)
        {
            if (FAILED(wined3d_set_adapter_display_mode(device->wined3d,
                    device->adapter->ordinal, &swapchain->d3d_mode)))
                ERR("Failed to set display mode.\n");
        }
    }
    else
    {
        if (FAILED(wined3d_set_adapter_display_mode(device->wined3d,
                device->adapter->ordinal, NULL)))
            ERR("Failed to set display mode.\n");

        swapchain->reapply_mode = TRUE;

        if (!(device->create_parms.flags & WINED3DCREATE_NOWINDOWCHANGES)
                && IsWindowVisible(swapchain->device_window))
            ShowWindow(swapchain->device_window, SW_MINIMIZE);
    }

    device->filter_messages = filter_messages;
}

HRESULT CDECL wined3d_swapchain_resize_buffers(struct wined3d_swapchain *swapchain, unsigned int buffer_count,
        unsigned int width, unsigned int height, enum wined3d_format_id format_id,
        enum wined3d_multisample_type multisample_type, unsigned int multisample_quality)
{
    struct wined3d_device *device = swapchain->device;
    BOOL update_desc = FALSE;

    TRACE("swapchain %p, buffer_count %u, width %u, height %u, format %s, "
            "multisample_type %#x, multisample_quality %#x.\n",
            swapchain, buffer_count, width, height, debug_d3dformat(format_id),
            multisample_type, multisample_quality);

    wined3d_swapchain_apply_sample_count_override(swapchain, format_id, &multisample_type, &multisample_quality);

    if (buffer_count && buffer_count != swapchain->desc.backbuffer_count)
        FIXME("Cannot change the back buffer count yet.\n");

    device->cs->ops->finish(device->cs, WINED3D_CS_QUEUE_DEFAULT);

    if (!width || !height)
    {
        /* The application is requesting that either the swapchain width or
         * height be set to the corresponding dimension in the window's
         * client rect. */

        RECT client_rect;

        if (!swapchain->desc.windowed)
            return WINED3DERR_INVALIDCALL;

        if (!GetClientRect(swapchain->device_window, &client_rect))
        {
            ERR("Failed to get client rect, last error %#x.\n", GetLastError());
            return WINED3DERR_INVALIDCALL;
        }

        if (!width)
            width = client_rect.right;

        if (!height)
            height = client_rect.bottom;
    }

    if (width != swapchain->desc.backbuffer_width
            || height != swapchain->desc.backbuffer_height)
    {
        swapchain->desc.backbuffer_width = width;
        swapchain->desc.backbuffer_height = height;
        update_desc = TRUE;
    }

    if (format_id == WINED3DFMT_UNKNOWN)
    {
        if (!swapchain->desc.windowed)
            return WINED3DERR_INVALIDCALL;
        format_id = swapchain->original_mode.format_id;
    }

    if (format_id != swapchain->desc.backbuffer_format)
    {
        swapchain->desc.backbuffer_format = format_id;
        update_desc = TRUE;
    }

    if (multisample_type != swapchain->desc.multisample_type
            || multisample_quality != swapchain->desc.multisample_quality)
    {
        swapchain->desc.multisample_type = multisample_type;
        swapchain->desc.multisample_quality = multisample_quality;
        update_desc = TRUE;
    }

    if (update_desc)
    {
        HRESULT hr;
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

    swapchain_update_render_to_fbo(swapchain);
    swapchain_update_draw_bindings(swapchain);

    return WINED3D_OK;
}

static HRESULT wined3d_swapchain_set_display_mode(struct wined3d_swapchain *swapchain,
        struct wined3d_display_mode *mode)
{
    struct wined3d_device *device = swapchain->device;
    HRESULT hr;

    if (swapchain->desc.flags & WINED3D_SWAPCHAIN_USE_CLOSEST_MATCHING_MODE)
    {
        if (FAILED(hr = wined3d_find_closest_matching_adapter_mode(device->wined3d,
                device->adapter->ordinal, mode)))
        {
            WARN("Failed to find closest matching mode, hr %#x.\n", hr);
        }
    }

    if (FAILED(hr = wined3d_set_adapter_display_mode(device->wined3d,
            device->adapter->ordinal, mode)))
    {
        WARN("Failed to set display mode, hr %#x.\n", hr);
        return WINED3DERR_INVALIDCALL;
    }

    return WINED3D_OK;
}

HRESULT CDECL wined3d_swapchain_resize_target(struct wined3d_swapchain *swapchain,
        const struct wined3d_display_mode *mode)
{
    struct wined3d_device *device = swapchain->device;
    struct wined3d_display_mode actual_mode;
    RECT original_window_rect, window_rect;
    HRESULT hr;

    TRACE("swapchain %p, mode %p.\n", swapchain, mode);

    if (swapchain->desc.windowed)
    {
        SetRect(&window_rect, 0, 0, mode->width, mode->height);
        AdjustWindowRectEx(&window_rect,
                GetWindowLongW(swapchain->device_window, GWL_STYLE), FALSE,
                GetWindowLongW(swapchain->device_window, GWL_EXSTYLE));
        SetRect(&window_rect, 0, 0,
                window_rect.right - window_rect.left, window_rect.bottom - window_rect.top);
        GetWindowRect(swapchain->device_window, &original_window_rect);
        OffsetRect(&window_rect, original_window_rect.left, original_window_rect.top);
    }
    else if (swapchain->desc.flags & WINED3D_SWAPCHAIN_ALLOW_MODE_SWITCH)
    {
        actual_mode = *mode;
        if (FAILED(hr = wined3d_swapchain_set_display_mode(swapchain, &actual_mode)))
            return hr;
        SetRect(&window_rect, 0, 0, actual_mode.width, actual_mode.height);
    }
    else
    {
        if (FAILED(hr = wined3d_get_adapter_display_mode(device->wined3d, device->adapter->ordinal,
                &actual_mode, NULL)))
        {
            ERR("Failed to get display mode, hr %#x.\n", hr);
            return WINED3DERR_INVALIDCALL;
        }

        SetRect(&window_rect, 0, 0, actual_mode.width, actual_mode.height);
    }

    MoveWindow(swapchain->device_window, window_rect.left, window_rect.top,
            window_rect.right - window_rect.left,
            window_rect.bottom - window_rect.top, TRUE);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_swapchain_set_fullscreen(struct wined3d_swapchain *swapchain,
        const struct wined3d_swapchain_desc *swapchain_desc, const struct wined3d_display_mode *mode)
{
    struct wined3d_device *device = swapchain->device;
    struct wined3d_display_mode actual_mode;
    HRESULT hr;

    TRACE("swapchain %p, desc %p, mode %p.\n", swapchain, swapchain_desc, mode);

    if (swapchain->desc.flags & WINED3D_SWAPCHAIN_ALLOW_MODE_SWITCH)
    {
        if (mode)
        {
            actual_mode = *mode;
        }
        else
        {
            if (!swapchain_desc->windowed)
            {
                actual_mode.width = swapchain_desc->backbuffer_width;
                actual_mode.height = swapchain_desc->backbuffer_height;
                actual_mode.refresh_rate = swapchain_desc->refresh_rate;
                actual_mode.format_id = swapchain_desc->backbuffer_format;
                actual_mode.scanline_ordering = WINED3D_SCANLINE_ORDERING_UNKNOWN;
            }
            else
            {
                actual_mode = swapchain->original_mode;
            }
        }

        if (FAILED(hr = wined3d_swapchain_set_display_mode(swapchain, &actual_mode)))
            return hr;
    }
    else
    {
        if (mode)
            WARN("WINED3D_SWAPCHAIN_ALLOW_MODE_SWITCH is not set, ignoring mode.\n");

        if (FAILED(hr = wined3d_get_adapter_display_mode(device->wined3d, device->adapter->ordinal,
                &actual_mode, NULL)))
        {
            ERR("Failed to get display mode, hr %#x.\n", hr);
            return WINED3DERR_INVALIDCALL;
        }
    }

    if (!swapchain_desc->windowed)
    {
        unsigned int width = actual_mode.width;
        unsigned int height = actual_mode.height;

        if (swapchain->desc.windowed)
        {
            /* Switch from windowed to fullscreen */
            HWND focus_window = device->create_parms.focus_window;
            if (!focus_window)
                focus_window = swapchain->device_window;
            if (FAILED(hr = wined3d_device_acquire_focus_window(device, focus_window)))
            {
                ERR("Failed to acquire focus window, hr %#x.\n", hr);
                return hr;
            }

            wined3d_device_setup_fullscreen_window(device, swapchain->device_window, width, height);
        }
        else
        {
            /* Fullscreen -> fullscreen mode change */
            BOOL filter_messages = device->filter_messages;
            device->filter_messages = TRUE;

            MoveWindow(swapchain->device_window, 0, 0, width, height, TRUE);
            ShowWindow(swapchain->device_window, SW_SHOW);

            device->filter_messages = filter_messages;
        }
        swapchain->d3d_mode = actual_mode;
    }
    else if (!swapchain->desc.windowed)
    {
        /* Fullscreen -> windowed switch */
        RECT *window_rect = NULL;
        if (swapchain->desc.flags & WINED3D_SWAPCHAIN_RESTORE_WINDOW_RECT)
            window_rect = &swapchain->original_window_rect;
        wined3d_device_restore_fullscreen_window(device, swapchain->device_window, window_rect);
        wined3d_device_release_focus_window(device);
    }

    swapchain->desc.windowed = swapchain_desc->windowed;

    return WINED3D_OK;
}
