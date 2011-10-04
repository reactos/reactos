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
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(fps);

/* Do not call while under the GL lock. */
static void swapchain_cleanup(struct wined3d_swapchain *swapchain)
{
    WINED3DDISPLAYMODE mode;
    UINT i;

    TRACE("Destroying swapchain %p.\n", swapchain);

    wined3d_swapchain_set_gamma_ramp(swapchain, 0, &swapchain->orig_gamma);

    /* Release the swapchain's draw buffers. Make sure swapchain->back_buffers[0]
     * is the last buffer to be destroyed, FindContext() depends on that. */
    if (swapchain->front_buffer)
    {
        surface_set_container(swapchain->front_buffer, WINED3D_CONTAINER_NONE, NULL);
        if (wined3d_surface_decref(swapchain->front_buffer))
            WARN("Something's still holding the front buffer (%p).\n", swapchain->front_buffer);
        swapchain->front_buffer = NULL;
    }

    if (swapchain->back_buffers)
    {
        i = swapchain->presentParms.BackBufferCount;

        while (i--)
        {
            surface_set_container(swapchain->back_buffers[i], WINED3D_CONTAINER_NONE, NULL);
            if (wined3d_surface_decref(swapchain->back_buffers[i]))
                WARN("Something's still holding back buffer %u (%p).\n", i, swapchain->back_buffers[i]);
        }
        HeapFree(GetProcessHeap(), 0, swapchain->back_buffers);
        swapchain->back_buffers = NULL;
    }

    for (i = 0; i < swapchain->num_contexts; ++i)
    {
        context_destroy(swapchain->device, swapchain->context[i]);
    }
    HeapFree(GetProcessHeap(), 0, swapchain->context);

    /* Restore the screen resolution if we rendered in fullscreen.
     * This will restore the screen resolution to what it was before creating
     * the swapchain. In case of d3d8 and d3d9 this will be the original
     * desktop resolution. In case of d3d7 this will be a NOP because ddraw
     * sets the resolution before starting up Direct3D, thus orig_width and
     * orig_height will be equal to the modes in the presentation params. */
    if (!swapchain->presentParms.Windowed && swapchain->presentParms.AutoRestoreDisplayMode)
    {
        mode.Width = swapchain->orig_width;
        mode.Height = swapchain->orig_height;
        mode.RefreshRate = 0;
        mode.Format = swapchain->orig_fmt;
        wined3d_device_set_display_mode(swapchain->device, 0, &mode);
    }

    if (swapchain->backup_dc)
    {
        TRACE("Destroying backup wined3d window %p, dc %p.\n", swapchain->backup_wnd, swapchain->backup_dc);

        ReleaseDC(swapchain->backup_wnd, swapchain->backup_dc);
        DestroyWindow(swapchain->backup_wnd);
    }
}

ULONG CDECL wined3d_swapchain_incref(struct wined3d_swapchain *swapchain)
{
    ULONG refcount = InterlockedIncrement(&swapchain->ref);

    TRACE("%p increasing refcount to %u.\n", swapchain, refcount);

    return refcount;
}

/* Do not call while under the GL lock. */
ULONG CDECL wined3d_swapchain_decref(struct wined3d_swapchain *swapchain)
{
    ULONG refcount = InterlockedDecrement(&swapchain->ref);

    TRACE("%p decreasing refcount to %u.\n", swapchain, refcount);

    if (!refcount)
    {
        swapchain_cleanup(swapchain);
        swapchain->parent_ops->wined3d_object_destroyed(swapchain->parent);
        HeapFree(GetProcessHeap(), 0, swapchain);
    }

    return refcount;
}

void * CDECL wined3d_swapchain_get_parent(const struct wined3d_swapchain *swapchain)
{
    TRACE("swapchain %p.\n", swapchain);

    return swapchain->parent;
}

HRESULT CDECL wined3d_swapchain_set_window(struct wined3d_swapchain *swapchain, HWND window)
{
    if (!window)
        window = swapchain->device_window;
    if (window == swapchain->win_handle)
        return WINED3D_OK;

    TRACE("Setting swapchain %p window from %p to %p.\n",
            swapchain, swapchain->win_handle, window);
    swapchain->win_handle = window;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_swapchain_present(struct wined3d_swapchain *swapchain,
        const RECT *src_rect, const RECT *dst_rect, HWND dst_window_override,
        const RGNDATA *dirty_region, DWORD flags)
{
    TRACE("swapchain %p, src_rect %s, dst_rect %s, dst_window_override %p, dirty_region %p, flags %#x.\n",
            swapchain, wine_dbgstr_rect(src_rect), wine_dbgstr_rect(dst_rect),
            dst_window_override, dirty_region, flags);

    wined3d_swapchain_set_window(swapchain, dst_window_override);

    return swapchain->swapchain_ops->swapchain_present(swapchain,
            src_rect, dst_rect, dirty_region, flags);
}

HRESULT CDECL wined3d_swapchain_get_front_buffer_data(const struct wined3d_swapchain *swapchain,
        struct wined3d_surface *dst_surface)
{
    POINT offset = {0, 0};

    TRACE("swapchain %p, dst_surface %p.\n", swapchain, dst_surface);

    if (swapchain->presentParms.Windowed)
        MapWindowPoints(swapchain->win_handle, NULL, &offset, 1);

    wined3d_surface_bltfast(dst_surface, offset.x, offset.y, swapchain->front_buffer, NULL, 0);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_swapchain_get_back_buffer(const struct wined3d_swapchain *swapchain,
        UINT back_buffer_idx, WINED3DBACKBUFFER_TYPE type, struct wined3d_surface **back_buffer)
{
    TRACE("swapchain %p, back_buffer_idx %u, type %#x, back_buffer %p.\n",
            swapchain, back_buffer_idx, type, back_buffer);

    /* Return invalid if there is no backbuffer array, otherwise it will
     * crash when ddraw is used (there swapchain->back_buffers is always
     * NULL). We need this because this function is called from
     * stateblock_init_default_state() to get the default scissorrect
     * dimensions. */
    if (!swapchain->back_buffers || back_buffer_idx >= swapchain->presentParms.BackBufferCount)
    {
        WARN("Invalid back buffer index.\n");
        /* Native d3d9 doesn't set NULL here, just as wine's d3d9. But set it
         * here in wined3d to avoid problems in other libs. */
        *back_buffer = NULL;
        return WINED3DERR_INVALIDCALL;
    }

    *back_buffer = swapchain->back_buffers[back_buffer_idx];
    if (*back_buffer)
        wined3d_surface_incref(*back_buffer);

    TRACE("Returning back buffer %p.\n", *back_buffer);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_swapchain_get_raster_status(const struct wined3d_swapchain *swapchain,
        WINED3DRASTER_STATUS *raster_status)
{
    static BOOL warned;
    /* No OpenGL equivalent */
    if (!warned)
    {
        FIXME("swapchain %p, raster_status %p stub!\n", swapchain, raster_status);
        warned = TRUE;
    }

    /* Obtaining the raster status is a widely implemented but optional
     * feature. When this method returns OK StarCraft 2 expects the
     * raster_status->InVBlank value to actually change over time. To prevent
     * StarCraft 2 from running in an infinite loop at startup this method
     * returns INVALIDCALL. */
    return WINED3DERR_INVALIDCALL;
}

HRESULT CDECL wined3d_swapchain_get_display_mode(const struct wined3d_swapchain *swapchain, WINED3DDISPLAYMODE *mode)
{
    HRESULT hr;

    TRACE("swapchain %p, mode %p.\n", swapchain, mode);

    hr = wined3d_get_adapter_display_mode(swapchain->device->wined3d, swapchain->device->adapter->ordinal, mode);

    TRACE("Returning w %u, h %u, refresh rate %u, format %s.\n",
            mode->Width, mode->Height, mode->RefreshRate, debug_d3dformat(mode->Format));

    return hr;
}

struct wined3d_device * CDECL wined3d_swapchain_get_device(const struct wined3d_swapchain *swapchain)
{
    TRACE("swapchain %p.\n", swapchain);

    return swapchain->device;
}

HRESULT CDECL wined3d_swapchain_get_present_parameters(const struct wined3d_swapchain *swapchain,
        WINED3DPRESENT_PARAMETERS *present_parameters)
{
    TRACE("swapchain %p, present_parameters %p.\n", swapchain, present_parameters);

    *present_parameters = swapchain->presentParms;

    return WINED3D_OK;
}

HRESULT CDECL wined3d_swapchain_set_gamma_ramp(const struct wined3d_swapchain *swapchain,
        DWORD flags, const WINED3DGAMMARAMP *ramp)
{
    HDC dc;

    TRACE("swapchain %p, flags %#x, ramp %p.\n", swapchain, flags, ramp);

    if (flags)
        FIXME("Ignoring flags %#x.\n", flags);

    dc = GetDC(swapchain->device_window);
    SetDeviceGammaRamp(dc, (void *)ramp);
    ReleaseDC(swapchain->device_window, dc);

    return WINED3D_OK;
}

HRESULT CDECL wined3d_swapchain_get_gamma_ramp(const struct wined3d_swapchain *swapchain,
        WINED3DGAMMARAMP *ramp)
{
    HDC dc;

    TRACE("swapchain %p, ramp %p.\n", swapchain, ramp);

    dc = GetDC(swapchain->device_window);
    GetDeviceGammaRamp(dc, ramp);
    ReleaseDC(swapchain->device_window, dc);

    return WINED3D_OK;
}

/* A GL context is provided by the caller */
static void swapchain_blit(struct wined3d_swapchain *swapchain,
        struct wined3d_context *context, const RECT *src_rect, const RECT *dst_rect)
{
    struct wined3d_surface *backbuffer = swapchain->back_buffers[0];
    struct wined3d_device *device = swapchain->device;
    UINT src_w = src_rect->right - src_rect->left;
    UINT src_h = src_rect->bottom - src_rect->top;
    GLenum gl_filter;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    RECT win_rect;
    UINT win_h;

    TRACE("swapchain %p, context %p, src_rect %s, dst_rect %s.\n",
            swapchain, context, wine_dbgstr_rect(src_rect), wine_dbgstr_rect(dst_rect));

    if (src_w == dst_rect->right - dst_rect->left && src_h == dst_rect->bottom - dst_rect->top)
        gl_filter = GL_NEAREST;
    else
        gl_filter = GL_LINEAR;

    GetClientRect(swapchain->win_handle, &win_rect);
    win_h = win_rect.bottom - win_rect.top;

    if (gl_info->fbo_ops.glBlitFramebuffer && is_identity_fixup(backbuffer->resource.format->color_fixup))
    {
        ENTER_GL();
        context_apply_fbo_state_blit(context, GL_READ_FRAMEBUFFER, backbuffer, NULL, SFLAG_INTEXTURE);
        glReadBuffer(GL_COLOR_ATTACHMENT0);
        context_check_fbo_status(context, GL_READ_FRAMEBUFFER);

        context_bind_fbo(context, GL_DRAW_FRAMEBUFFER, NULL);
        context_set_draw_buffer(context, GL_BACK);

        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        IWineD3DDeviceImpl_MarkStateDirty(device, STATE_RENDER(WINED3DRS_COLORWRITEENABLE));
        IWineD3DDeviceImpl_MarkStateDirty(device, STATE_RENDER(WINED3DRS_COLORWRITEENABLE1));
        IWineD3DDeviceImpl_MarkStateDirty(device, STATE_RENDER(WINED3DRS_COLORWRITEENABLE2));
        IWineD3DDeviceImpl_MarkStateDirty(device, STATE_RENDER(WINED3DRS_COLORWRITEENABLE3));

        glDisable(GL_SCISSOR_TEST);
        IWineD3DDeviceImpl_MarkStateDirty(device, STATE_RENDER(WINED3DRS_SCISSORTESTENABLE));

        /* Note that the texture is upside down */
        gl_info->fbo_ops.glBlitFramebuffer(src_rect->left, src_rect->top, src_rect->right, src_rect->bottom,
                dst_rect->left, win_h - dst_rect->top, dst_rect->right, win_h - dst_rect->bottom,
                GL_COLOR_BUFFER_BIT, gl_filter);
        checkGLcall("Swapchain present blit(EXT_framebuffer_blit)\n");
        LEAVE_GL();
    }
    else
    {
        struct wined3d_context *context2;
        float tex_left = src_rect->left;
        float tex_top = src_rect->top;
        float tex_right = src_rect->right;
        float tex_bottom = src_rect->bottom;

        context2 = context_acquire(device, swapchain->back_buffers[0]);
        context_apply_blit_state(context2, device);

        if (backbuffer->flags & SFLAG_NORMCOORD)
        {
            tex_left /= src_w;
            tex_right /= src_w;
            tex_top /= src_h;
            tex_bottom /= src_h;
        }

        if (is_complex_fixup(backbuffer->resource.format->color_fixup))
            gl_filter = GL_NEAREST;

        ENTER_GL();
        context_bind_fbo(context2, GL_FRAMEBUFFER, NULL);

        /* Set up the texture. The surface is not in a IWineD3D*Texture container,
         * so there are no d3d texture settings to dirtify
         */
        device->blitter->set_shader(device->blit_priv, context2->gl_info, backbuffer);
        glTexParameteri(backbuffer->texture_target, GL_TEXTURE_MIN_FILTER, gl_filter);
        glTexParameteri(backbuffer->texture_target, GL_TEXTURE_MAG_FILTER, gl_filter);

        context_set_draw_buffer(context, GL_BACK);

        /* Set the viewport to the destination rectandle, disable any projection
         * transformation set up by context_apply_blit_state(), and draw a
         * (-1,-1)-(1,1) quad.
         *
         * Back up viewport and matrix to avoid breaking last_was_blit
         *
         * Note that context_apply_blit_state() set up viewport and ortho to
         * match the surface size - we want the GL drawable(=window) size. */
        glPushAttrib(GL_VIEWPORT_BIT);
        glViewport(dst_rect->left, win_h - dst_rect->bottom, dst_rect->right, win_h - dst_rect->top);
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();

        glBegin(GL_QUADS);
            /* bottom left */
            glTexCoord2f(tex_left, tex_bottom);
            glVertex2i(-1, -1);

            /* top left */
            glTexCoord2f(tex_left, tex_top);
            glVertex2i(-1, 1);

            /* top right */
            glTexCoord2f(tex_right, tex_top);
            glVertex2i(1, 1);

            /* bottom right */
            glTexCoord2f(tex_right, tex_bottom);
            glVertex2i(1, -1);
        glEnd();

        glPopMatrix();
        glPopAttrib();

        device->blitter->unset_shader(context->gl_info);
        checkGLcall("Swapchain present blit(manual)\n");
        LEAVE_GL();

        context_release(context2);
    }
}

static HRESULT swapchain_gl_present(struct wined3d_swapchain *swapchain, const RECT *src_rect_in,
        const RECT *dst_rect_in, const RGNDATA *dirty_region, DWORD flags)
{
    const struct wined3d_fb_state *fb = &swapchain->device->fb;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;
    RECT src_rect, dst_rect;
    BOOL render_to_fbo;

    context = context_acquire(swapchain->device, swapchain->back_buffers[0]);
    if (!context->valid)
    {
        context_release(context);
        WARN("Invalid context, skipping present.\n");
        return WINED3D_OK;
    }

    gl_info = context->gl_info;

    /* Render the cursor onto the back buffer, using our nifty directdraw blitting code :-) */
    if (swapchain->device->bCursorVisible &&
        swapchain->device->cursorTexture &&
        !swapchain->device->hardwareCursor)
    {
        struct wined3d_surface cursor;
        RECT destRect =
        {
            swapchain->device->xScreenSpace - swapchain->device->xHotSpot,
            swapchain->device->yScreenSpace - swapchain->device->yHotSpot,
            swapchain->device->xScreenSpace + swapchain->device->cursorWidth - swapchain->device->xHotSpot,
            swapchain->device->yScreenSpace + swapchain->device->cursorHeight - swapchain->device->yHotSpot,
        };
        TRACE("Rendering the cursor. Creating fake surface at %p\n", &cursor);
        /* Build a fake surface to call the Blitting code. It is not possible to use the interface passed by
         * the application because we are only supposed to copy the information out. Using a fake surface
         * allows to use the Blitting engine and avoid copying the whole texture -> render target blitting code.
         */
        memset(&cursor, 0, sizeof(cursor));
        cursor.resource.ref = 1;
        cursor.resource.device = swapchain->device;
        cursor.resource.pool = WINED3DPOOL_SCRATCH;
        cursor.resource.format = wined3d_get_format(gl_info, WINED3DFMT_B8G8R8A8_UNORM);
        cursor.resource.resourceType = WINED3DRTYPE_SURFACE;
        cursor.texture_name = swapchain->device->cursorTexture;
        cursor.texture_target = GL_TEXTURE_2D;
        cursor.texture_level = 0;
        cursor.resource.width = swapchain->device->cursorWidth;
        cursor.resource.height = swapchain->device->cursorHeight;
        /* The cursor must have pow2 sizes */
        cursor.pow2Width = cursor.resource.width;
        cursor.pow2Height = cursor.resource.height;
        /* The surface is in the texture */
        cursor.flags |= SFLAG_INTEXTURE;
        /* DDBLT_KEYSRC will cause BltOverride to enable the alpha test with GL_NOTEQUAL, 0.0,
         * which is exactly what we want :-)
         */
        if (swapchain->presentParms.Windowed)
            MapWindowPoints(NULL, swapchain->win_handle, (LPPOINT)&destRect, 2);
        wined3d_surface_blt(swapchain->back_buffers[0], &destRect,
                &cursor, NULL, WINEDDBLT_KEYSRC, NULL, WINED3DTEXF_POINT);
    }

    if (swapchain->device->logo_surface)
    {
        /* Blit the logo into the upper left corner of the drawable. */
        wined3d_surface_bltfast(swapchain->back_buffers[0], 0, 0,
                swapchain->device->logo_surface, NULL, WINEDDBLTFAST_SRCCOLORKEY);
    }

    TRACE("Presenting HDC %p.\n", context->hdc);

    render_to_fbo = swapchain->render_to_fbo;

    if (src_rect_in)
    {
        src_rect = *src_rect_in;
        if (!render_to_fbo && (src_rect.left || src_rect.top
                || src_rect.right != swapchain->presentParms.BackBufferWidth
                || src_rect.bottom != swapchain->presentParms.BackBufferHeight))
        {
            render_to_fbo = TRUE;
        }
    }
    else
    {
        src_rect.left = 0;
        src_rect.top = 0;
        src_rect.right = swapchain->presentParms.BackBufferWidth;
        src_rect.bottom = swapchain->presentParms.BackBufferHeight;
    }

    if (dst_rect_in)
        dst_rect = *dst_rect_in;
    else
        GetClientRect(swapchain->win_handle, &dst_rect);

    if (!render_to_fbo && (dst_rect.left || dst_rect.top
            || dst_rect.right != swapchain->presentParms.BackBufferWidth
            || dst_rect.bottom != swapchain->presentParms.BackBufferHeight))
    {
        render_to_fbo = TRUE;
    }

    /* Rendering to a window of different size, presenting partial rectangles,
     * or rendering to a different window needs help from FBO_blit or a textured
     * draw. Render the swapchain to a FBO in the future.
     *
     * Note that FBO_blit from the backbuffer to the frontbuffer cannot solve
     * all these issues - this fails if the window is smaller than the backbuffer.
     */
    if (!swapchain->render_to_fbo && render_to_fbo && wined3d_settings.offscreen_rendering_mode == ORM_FBO)
    {
        surface_load_location(swapchain->back_buffers[0], SFLAG_INTEXTURE, NULL);
        surface_modify_location(swapchain->back_buffers[0], SFLAG_INDRAWABLE, FALSE);
        swapchain->render_to_fbo = TRUE;
    }

    if (swapchain->render_to_fbo)
    {
        /* This codepath should only be hit with the COPY swapeffect. Otherwise a backbuffer-
         * window size mismatch is impossible(fullscreen) and src and dst rectangles are
         * not allowed(they need the COPY swapeffect)
         *
         * The DISCARD swap effect is ok as well since any backbuffer content is allowed after
         * the swap. */
        if (swapchain->presentParms.SwapEffect == WINED3DSWAPEFFECT_FLIP)
            FIXME("Render-to-fbo with WINED3DSWAPEFFECT_FLIP\n");

        swapchain_blit(swapchain, context, &src_rect, &dst_rect);
    }

    if (swapchain->num_contexts > 1)
        wglFinish();
    SwapBuffers(context->hdc); /* TODO: cycle through the swapchain buffers */

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

    /* This is disabled, but the code left in for debug purposes.
     *
     * Since we're allowed to modify the new back buffer on a D3DSWAPEFFECT_DISCARD flip,
     * we can clear it with some ugly color to make bad drawing visible and ease debugging.
     * The Debug runtime does the same on Windows. However, a few games do not redraw the
     * screen properly, like Max Payne 2, which leaves a few pixels undefined.
     *
     * Tests show that the content of the back buffer after a discard flip is indeed not
     * reliable, so no game can depend on the exact content. However, it resembles the
     * old contents in some way, for example by showing fragments at other locations. In
     * general, the color theme is still intact. So Max payne, which draws rather dark scenes
     * gets a dark background image. If we clear it with a bright ugly color, the game's
     * bug shows up much more than it does on Windows, and the players see single pixels
     * with wrong colors.
     * (The Max Payne bug has been confirmed on Windows with the debug runtime) */
    if (FALSE && swapchain->presentParms.SwapEffect == WINED3DSWAPEFFECT_DISCARD)
    {
        TRACE("Clearing the color buffer with cyan color\n");

        wined3d_device_clear(swapchain->device, 0, NULL,
                WINED3DCLEAR_TARGET, 0xff00ffff, 1.0f, 0);
    }

    if (!swapchain->render_to_fbo && ((swapchain->front_buffer->flags & SFLAG_INSYSMEM)
            || (swapchain->back_buffers[0]->flags & SFLAG_INSYSMEM)))
    {
        /* Both memory copies of the surfaces are ok, flip them around too instead of dirtifying
         * Doesn't work with render_to_fbo because we're not flipping
         */
        struct wined3d_surface *front = swapchain->front_buffer;
        struct wined3d_surface *back = swapchain->back_buffers[0];

        if(front->resource.size == back->resource.size) {
            DWORD fbflags;
            flip_surface(front, back);

            /* Tell the front buffer surface that is has been modified. However,
             * the other locations were preserved during that, so keep the flags.
             * This serves to update the emulated overlay, if any. */
            fbflags = front->flags;
            surface_modify_location(front, SFLAG_INDRAWABLE, TRUE);
            front->flags = fbflags;
        }
        else
        {
            surface_modify_location(front, SFLAG_INDRAWABLE, TRUE);
            surface_modify_location(back, SFLAG_INDRAWABLE, TRUE);
        }
    }
    else
    {
        surface_modify_location(swapchain->front_buffer, SFLAG_INDRAWABLE, TRUE);
        /* If the swapeffect is DISCARD, the back buffer is undefined. That means the SYSMEM
         * and INTEXTURE copies can keep their old content if they have any defined content.
         * If the swapeffect is COPY, the content remains the same. If it is FLIP however,
         * the texture / sysmem copy needs to be reloaded from the drawable
         */
        if (swapchain->presentParms.SwapEffect == WINED3DSWAPEFFECT_FLIP)
            surface_modify_location(swapchain->back_buffers[0], SFLAG_INDRAWABLE, TRUE);
    }

    if (fb->depth_stencil)
    {
        if (swapchain->presentParms.Flags & WINED3DPRESENTFLAG_DISCARD_DEPTHSTENCIL
                || fb->depth_stencil->flags & SFLAG_DISCARD)
        {
            surface_modify_ds_location(fb->depth_stencil, SFLAG_DS_DISCARDED,
                    fb->depth_stencil->resource.width,
                    fb->depth_stencil->resource.height);
            if (fb->depth_stencil == swapchain->device->onscreen_depth_stencil)
            {
                wined3d_surface_decref(swapchain->device->onscreen_depth_stencil);
                swapchain->device->onscreen_depth_stencil = NULL;
            }
        }
    }

    context_release(context);

    TRACE("returning\n");
    return WINED3D_OK;
}

static const struct wined3d_swapchain_ops swapchain_gl_ops =
{
    swapchain_gl_present,
};

/* Helper function that blits the front buffer contents to the target window. */
void x11_copy_to_screen(struct wined3d_swapchain *swapchain, const RECT *rect)
{
    struct wined3d_surface *front;
    POINT offset = {0, 0};
    HDC src_dc, dst_dc;
    RECT draw_rect;
    HWND window;

    TRACE("swapchain %p, rect %s.\n", swapchain, wine_dbgstr_rect(rect));

    front = swapchain->front_buffer;
    if (!(front->resource.usage & WINED3DUSAGE_RENDERTARGET))
        return;

    TRACE("Copying surface %p to screen.\n", front);

    src_dc = front->hDC;
    window = swapchain->win_handle;
    dst_dc = GetDCEx(window, 0, DCX_CLIPSIBLINGS | DCX_CACHE);

    /* Front buffer coordinates are screen coordinates. Map them to the
     * destination window if not fullscreened. */
    if (swapchain->presentParms.Windowed)
        ClientToScreen(window, &offset);

    TRACE("offset %s.\n", wine_dbgstr_point(&offset));

#if 0
    /* FIXME: This doesn't work... if users really want to run
     * X in 8bpp, then we need to call directly into display.drv
     * (or Wine's equivalent), and force a private colormap
     * without default entries. */
    if (front->palette)
    {
        SelectPalette(dst_dc, front->palette->hpal, FALSE);
        RealizePalette(dst_dc); /* sends messages => deadlocks */
    }
#endif

    draw_rect.left = 0;
    draw_rect.right = front->resource.width;
    draw_rect.top = 0;
    draw_rect.bottom = front->resource.height;

#if 0
    /* TODO: Support clippers. */
    if (front->clipper)
    {
        RECT xrc;
        HWND hwnd = ((IWineD3DClipperImpl *)front->clipper)->hWnd;
        if (hwnd && GetClientRect(hwnd,&xrc))
        {
            OffsetRect(&xrc, offset.x, offset.y);
            IntersectRect(&draw_rect, &draw_rect, &xrc);
        }
    }
#endif

    if (!rect)
    {
        /* Only use this if the caller did not pass a rectangle, since
         * due to double locking this could be the wrong one... */
        if (front->lockedRect.left != front->lockedRect.right)
            IntersectRect(&draw_rect, &draw_rect, &front->lockedRect);
    }
    else
    {
        IntersectRect(&draw_rect, &draw_rect, rect);
    }

    BitBlt(dst_dc, draw_rect.left - offset.x, draw_rect.top - offset.y,
            draw_rect.right - draw_rect.left, draw_rect.bottom - draw_rect.top,
            src_dc, draw_rect.left, draw_rect.top, SRCCOPY);
    ReleaseDC(window, dst_dc);
}

static HRESULT swapchain_gdi_present(struct wined3d_swapchain *swapchain, const RECT *src_rect_in,
        const RECT *dst_rect_in, const RGNDATA *dirty_region, DWORD flags)
{
    struct wined3d_surface *front, *back;

    if (!swapchain->back_buffers)
    {
        WARN("Swapchain doesn't have a backbuffer, returning WINED3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }
    front = swapchain->front_buffer;
    back = swapchain->back_buffers[0];

    /* Flip the DC. */
    {
        HDC tmp;
        tmp = front->hDC;
        front->hDC = back->hDC;
        back->hDC = tmp;
    }

    /* Flip the DIBsection. */
    {
        HBITMAP tmp;
        tmp = front->dib.DIBsection;
        front->dib.DIBsection = back->dib.DIBsection;
        back->dib.DIBsection = tmp;
    }

    /* Flip the surface data. */
    {
        void *tmp;

        tmp = front->dib.bitmap_data;
        front->dib.bitmap_data = back->dib.bitmap_data;
        back->dib.bitmap_data = tmp;

        tmp = front->resource.allocatedMemory;
        front->resource.allocatedMemory = back->resource.allocatedMemory;
        back->resource.allocatedMemory = tmp;

        if (front->resource.heapMemory)
            ERR("GDI Surface %p has heap memory allocated.\n", front);

        if (back->resource.heapMemory)
            ERR("GDI Surface %p has heap memory allocated.\n", back);
    }

    /* Client_memory should not be different, but just in case. */
    {
        BOOL tmp;
        tmp = front->dib.client_memory;
        front->dib.client_memory = back->dib.client_memory;
        back->dib.client_memory = tmp;
    }

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

    x11_copy_to_screen(swapchain, NULL);

    return WINED3D_OK;
}

static const struct wined3d_swapchain_ops swapchain_gdi_ops =
{
    swapchain_gdi_present,
};

/* Do not call while under the GL lock. */
static HRESULT swapchain_init(struct wined3d_swapchain *swapchain, WINED3DSURFTYPE surface_type,
        struct wined3d_device *device, WINED3DPRESENT_PARAMETERS *present_parameters,
        void *parent, const struct wined3d_parent_ops *parent_ops)
{
    const struct wined3d_adapter *adapter = device->adapter;
    const struct wined3d_format *format;
    BOOL displaymode_set = FALSE;
    WINED3DDISPLAYMODE mode;
    RECT client_rect;
    HWND window;
    HRESULT hr;
    UINT i;

    if (present_parameters->BackBufferCount > WINED3DPRESENT_BACK_BUFFER_MAX)
    {
        FIXME("The application requested %u back buffers, this is not supported.\n",
                present_parameters->BackBufferCount);
        return WINED3DERR_INVALIDCALL;
    }

    if (present_parameters->BackBufferCount > 1)
    {
        FIXME("The application requested more than one back buffer, this is not properly supported.\n"
                "Please configure the application to use double buffering (1 back buffer) if possible.\n");
    }

    switch (surface_type)
    {
        case SURFACE_GDI:
            swapchain->swapchain_ops = &swapchain_gdi_ops;
            break;

        case SURFACE_OPENGL:
            swapchain->swapchain_ops = &swapchain_gl_ops;
            break;

        case SURFACE_UNKNOWN:
            FIXME("Caller tried to create a SURFACE_UNKNOWN swapchain.\n");
            return WINED3DERR_INVALIDCALL;
    }

    window = present_parameters->hDeviceWindow ? present_parameters->hDeviceWindow : device->createParms.hFocusWindow;

    swapchain->device = device;
    swapchain->parent = parent;
    swapchain->parent_ops = parent_ops;
    swapchain->ref = 1;
    swapchain->win_handle = window;
    swapchain->device_window = window;

    wined3d_get_adapter_display_mode(device->wined3d, adapter->ordinal, &mode);
    swapchain->orig_width = mode.Width;
    swapchain->orig_height = mode.Height;
    swapchain->orig_fmt = mode.Format;
    format = wined3d_get_format(&adapter->gl_info, mode.Format);

    GetClientRect(window, &client_rect);
    if (present_parameters->Windowed
            && (!present_parameters->BackBufferWidth || !present_parameters->BackBufferHeight
            || present_parameters->BackBufferFormat == WINED3DFMT_UNKNOWN))
    {

        if (!present_parameters->BackBufferWidth)
        {
            present_parameters->BackBufferWidth = client_rect.right;
            TRACE("Updating width to %u.\n", present_parameters->BackBufferWidth);
        }

        if (!present_parameters->BackBufferHeight)
        {
            present_parameters->BackBufferHeight = client_rect.bottom;
            TRACE("Updating height to %u.\n", present_parameters->BackBufferHeight);
        }

        if (present_parameters->BackBufferFormat == WINED3DFMT_UNKNOWN)
        {
            present_parameters->BackBufferFormat = swapchain->orig_fmt;
            TRACE("Updating format to %s.\n", debug_d3dformat(swapchain->orig_fmt));
        }
    }
    swapchain->presentParms = *present_parameters;

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO
            && present_parameters->BackBufferCount
            && (present_parameters->BackBufferWidth != client_rect.right
            || present_parameters->BackBufferHeight != client_rect.bottom))
    {
        TRACE("Rendering to FBO. Backbuffer %ux%u, window %ux%u.\n",
                present_parameters->BackBufferWidth,
                present_parameters->BackBufferHeight,
                client_rect.right, client_rect.bottom);
        swapchain->render_to_fbo = TRUE;
    }

    TRACE("Creating front buffer.\n");
    hr = device->device_parent->ops->create_rendertarget(device->device_parent, parent,
            swapchain->presentParms.BackBufferWidth, swapchain->presentParms.BackBufferHeight,
            swapchain->presentParms.BackBufferFormat, swapchain->presentParms.MultiSampleType,
            swapchain->presentParms.MultiSampleQuality, TRUE /* Lockable */,
            &swapchain->front_buffer);
    if (FAILED(hr))
    {
        WARN("Failed to create front buffer, hr %#x.\n", hr);
        goto err;
    }

    surface_set_container(swapchain->front_buffer, WINED3D_CONTAINER_SWAPCHAIN, swapchain);
    if (surface_type == SURFACE_OPENGL)
    {
        surface_modify_location(swapchain->front_buffer, SFLAG_INDRAWABLE, TRUE);
    }

    /* MSDN says we're only allowed a single fullscreen swapchain per device,
     * so we should really check to see if there is a fullscreen swapchain
     * already. Does a single head count as full screen? */

    if (!present_parameters->Windowed)
    {
        WINED3DDISPLAYMODE mode;

        /* Change the display settings */
        mode.Width = present_parameters->BackBufferWidth;
        mode.Height = present_parameters->BackBufferHeight;
        mode.Format = present_parameters->BackBufferFormat;
        mode.RefreshRate = present_parameters->FullScreen_RefreshRateInHz;

        hr = wined3d_device_set_display_mode(device, 0, &mode);
        if (FAILED(hr))
        {
            WARN("Failed to set display mode, hr %#x.\n", hr);
            goto err;
        }
        displaymode_set = TRUE;
    }

    if (surface_type == SURFACE_OPENGL)
    {
        static const enum wined3d_format_id formats[] =
        {
            WINED3DFMT_D24_UNORM_S8_UINT,
            WINED3DFMT_D32_UNORM,
            WINED3DFMT_R24_UNORM_X8_TYPELESS,
            WINED3DFMT_D16_UNORM,
            WINED3DFMT_S1_UINT_D15_UNORM
        };

        const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;

        swapchain->context = HeapAlloc(GetProcessHeap(), 0, sizeof(swapchain->context));
        if (!swapchain->context)
        {
            ERR("Failed to create the context array.\n");
            hr = E_OUTOFMEMORY;
            goto err;
        }
        swapchain->num_contexts = 1;

        /* In WGL both color, depth and stencil are features of a pixel format. In case of D3D they are separate.
         * You are able to add a depth + stencil surface at a later stage when you need it.
         * In order to support this properly in WineD3D we need the ability to recreate the opengl context and
         * drawable when this is required. This is very tricky as we need to reapply ALL opengl states for the new
         * context, need torecreate shaders, textures and other resources.
         *
         * The context manager already takes care of the state problem and for the other tasks code from Reset
         * can be used. These changes are way to risky during the 1.0 code freeze which is taking place right now.
         * Likely a lot of other new bugs will be exposed. For that reason request a depth stencil surface all the
         * time. It can cause a slight performance hit but fixes a lot of regressions. A fixme reminds of that this
         * issue needs to be fixed. */
        for (i = 0; i < (sizeof(formats) / sizeof(*formats)); i++)
        {
            swapchain->ds_format = wined3d_get_format(gl_info, formats[i]);
            swapchain->context[0] = context_create(swapchain, swapchain->front_buffer, swapchain->ds_format);
            if (swapchain->context[0]) break;
            TRACE("Depth stencil format %s is not supported, trying next format\n",
                  debug_d3dformat(formats[i]));
        }

        if (!swapchain->context[0])
        {
            WARN("Failed to create context.\n");
            hr = WINED3DERR_NOTAVAILABLE;
            goto err;
        }

        if (!present_parameters->EnableAutoDepthStencil
                || swapchain->presentParms.AutoDepthStencilFormat != swapchain->ds_format->id)
        {
            FIXME("Add OpenGL context recreation support to context_validate_onscreen_formats\n");
        }
        context_release(swapchain->context[0]);
    }

    if (swapchain->presentParms.BackBufferCount > 0)
    {
        swapchain->back_buffers = HeapAlloc(GetProcessHeap(), 0,
                sizeof(*swapchain->back_buffers) * swapchain->presentParms.BackBufferCount);
        if (!swapchain->back_buffers)
        {
            ERR("Failed to allocate backbuffer array memory.\n");
            hr = E_OUTOFMEMORY;
            goto err;
        }

        for (i = 0; i < swapchain->presentParms.BackBufferCount; ++i)
        {
            TRACE("Creating back buffer %u.\n", i);
            hr = device->device_parent->ops->create_rendertarget(device->device_parent, parent,
                    swapchain->presentParms.BackBufferWidth, swapchain->presentParms.BackBufferHeight,
                    swapchain->presentParms.BackBufferFormat, swapchain->presentParms.MultiSampleType,
                    swapchain->presentParms.MultiSampleQuality, TRUE /* Lockable */,
                    &swapchain->back_buffers[i]);
            if (FAILED(hr))
            {
                WARN("Failed to create back buffer %u, hr %#x.\n", i, hr);
                goto err;
            }

            surface_set_container(swapchain->back_buffers[i], WINED3D_CONTAINER_SWAPCHAIN, swapchain);
        }
    }

    /* Swapchains share the depth/stencil buffer, so only create a single depthstencil surface. */
    if (present_parameters->EnableAutoDepthStencil && surface_type == SURFACE_OPENGL)
    {
        TRACE("Creating depth/stencil buffer.\n");
        if (!device->auto_depth_stencil)
        {
            hr = device->device_parent->ops->create_depth_stencil(device->device_parent,
                    swapchain->presentParms.BackBufferWidth, swapchain->presentParms.BackBufferHeight,
                    swapchain->presentParms.AutoDepthStencilFormat, swapchain->presentParms.MultiSampleType,
                    swapchain->presentParms.MultiSampleQuality, FALSE /* FIXME: Discard */,
                    &device->auto_depth_stencil);
            if (FAILED(hr))
            {
                WARN("Failed to create the auto depth stencil, hr %#x.\n", hr);
                goto err;
            }

            surface_set_container(device->auto_depth_stencil, WINED3D_CONTAINER_NONE, NULL);
        }
    }

    wined3d_swapchain_get_gamma_ramp(swapchain, &swapchain->orig_gamma);

    return WINED3D_OK;

err:
    if (displaymode_set)
    {
        DEVMODEW devmode;

        ClipCursor(NULL);

        /* Change the display settings */
        memset(&devmode, 0, sizeof(devmode));
        devmode.dmSize = sizeof(devmode);
        devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
        devmode.dmBitsPerPel = format->byte_count * CHAR_BIT;
        devmode.dmPelsWidth = swapchain->orig_width;
        devmode.dmPelsHeight = swapchain->orig_height;
        ChangeDisplaySettingsExW(adapter->DeviceName, &devmode, NULL, CDS_FULLSCREEN, NULL);
    }

    if (swapchain->back_buffers)
    {
        for (i = 0; i < swapchain->presentParms.BackBufferCount; ++i)
        {
            if (swapchain->back_buffers[i])
                wined3d_surface_decref(swapchain->back_buffers[i]);
        }
        HeapFree(GetProcessHeap(), 0, swapchain->back_buffers);
    }

    if (swapchain->context)
    {
        if (swapchain->context[0])
        {
            context_release(swapchain->context[0]);
            context_destroy(device, swapchain->context[0]);
            swapchain->num_contexts = 0;
        }
        HeapFree(GetProcessHeap(), 0, swapchain->context);
    }

    if (swapchain->front_buffer)
        wined3d_surface_decref(swapchain->front_buffer);

    return hr;
}

/* Do not call while under the GL lock. */
HRESULT CDECL wined3d_swapchain_create(struct wined3d_device *device,
        WINED3DPRESENT_PARAMETERS *present_parameters, WINED3DSURFTYPE surface_type,
        void *parent, const struct wined3d_parent_ops *parent_ops,
        struct wined3d_swapchain **swapchain)
{
    struct wined3d_swapchain *object;
    HRESULT hr;

    TRACE("device %p, present_parameters %p, swapchain %p, parent %p, surface_type %#x.\n",
            device, present_parameters, swapchain, parent, surface_type);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate swapchain memory.\n");
        return E_OUTOFMEMORY;
    }

    hr = swapchain_init(object, surface_type, device, present_parameters, parent, parent_ops);
    if (FAILED(hr))
    {
        WARN("Failed to initialize swapchain, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created swapchain %p.\n", object);
    *swapchain = object;

    return WINED3D_OK;
}

/* Do not call while under the GL lock. */
static struct wined3d_context *swapchain_create_context(struct wined3d_swapchain *swapchain)
{
    struct wined3d_context **newArray;
    struct wined3d_context *ctx;

    TRACE("Creating a new context for swapchain %p, thread %u.\n", swapchain, GetCurrentThreadId());

    if (!(ctx = context_create(swapchain, swapchain->front_buffer, swapchain->ds_format)))
    {
        ERR("Failed to create a new context for the swapchain\n");
        return NULL;
    }
    context_release(ctx);

    newArray = HeapAlloc(GetProcessHeap(), 0, sizeof(*newArray) * (swapchain->num_contexts + 1));
    if(!newArray) {
        ERR("Out of memory when trying to allocate a new context array\n");
        context_destroy(swapchain->device, ctx);
        return NULL;
    }
    memcpy(newArray, swapchain->context, sizeof(*newArray) * swapchain->num_contexts);
    HeapFree(GetProcessHeap(), 0, swapchain->context);
    newArray[swapchain->num_contexts] = ctx;
    swapchain->context = newArray;
    swapchain->num_contexts++;

    TRACE("Returning context %p\n", ctx);
    return ctx;
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

void get_drawable_size_swapchain(struct wined3d_context *context, UINT *width, UINT *height)
{
    /* The drawable size of an onscreen drawable is the surface size.
     * (Actually: The window size, but the surface is created in window size) */
    *width = context->current_rt->resource.width;
    *height = context->current_rt->resource.height;
}
