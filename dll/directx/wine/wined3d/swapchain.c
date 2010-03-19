/*
 *IDirect3DSwapChain9 implementation
 *
 *Copyright 2002-2003 Jason Edmeades
 *Copyright 2002-2003 Raphael Junqueira
 *Copyright 2005 Oliver Stieber
 *Copyright 2007-2008 Stefan Dösinger for CodeWeavers
 *
 *This library is free software; you can redistribute it and/or
 *modify it under the terms of the GNU Lesser General Public
 *License as published by the Free Software Foundation; either
 *version 2.1 of the License, or (at your option) any later version.
 *
 *This library is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *Lesser General Public License for more details.
 *
 *You should have received a copy of the GNU Lesser General Public
 *License along with this library; if not, write to the Free Software
 *Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"
#include "wined3d_private.h"


/*TODO: some of the additional parameters may be required to
    set the gamma ramp (for some weird reason microsoft have left swap gammaramp in device
    but it operates on a swapchain, it may be a good idea to move it to IWineD3DSwapChain for IWineD3D)*/


WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(fps);

#define GLINFO_LOCATION This->device->adapter->gl_info

/*IWineD3DSwapChain parts follow: */
static void WINAPI IWineD3DSwapChainImpl_Destroy(IWineD3DSwapChain *iface)
{
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    WINED3DDISPLAYMODE mode;
    unsigned int i;

    TRACE("Destroying swapchain %p\n", iface);

    IWineD3DSwapChain_SetGammaRamp(iface, 0, &This->orig_gamma);

    /* Release the swapchain's draw buffers. Make sure This->backBuffer[0] is
     * the last buffer to be destroyed, FindContext() depends on that. */
    if (This->frontBuffer)
    {
        IWineD3DSurface_SetContainer(This->frontBuffer, 0);
        if (IWineD3DSurface_Release(This->frontBuffer))
        {
            WARN("(%p) Something's still holding the front buffer (%p).\n",
                    This, This->frontBuffer);
        }
        This->frontBuffer = NULL;
    }

    if (This->backBuffer)
    {
        UINT i = This->presentParms.BackBufferCount;

        while (i--)
        {
            IWineD3DSurface_SetContainer(This->backBuffer[i], 0);
            if (IWineD3DSurface_Release(This->backBuffer[i]))
                WARN("(%p) Something's still holding back buffer %u (%p).\n",
                        This, i, This->backBuffer[i]);
        }
        HeapFree(GetProcessHeap(), 0, This->backBuffer);
        This->backBuffer = NULL;
    }

    for (i = 0; i < This->num_contexts; ++i)
    {
        context_destroy(This->device, This->context[i]);
    }
    /* Restore the screen resolution if we rendered in fullscreen
     * This will restore the screen resolution to what it was before creating the swapchain. In case of d3d8 and d3d9
     * this will be the original desktop resolution. In case of d3d7 this will be a NOP because ddraw sets the resolution
     * before starting up Direct3D, thus orig_width and orig_height will be equal to the modes in the presentation params
     */
    if(This->presentParms.Windowed == FALSE && This->presentParms.AutoRestoreDisplayMode) {
        mode.Width = This->orig_width;
        mode.Height = This->orig_height;
        mode.RefreshRate = 0;
        mode.Format = This->orig_fmt;
        IWineD3DDevice_SetDisplayMode((IWineD3DDevice *)This->device, 0, &mode);
    }

    HeapFree(GetProcessHeap(), 0, This->context);
    HeapFree(GetProcessHeap(), 0, This);
}

/* A GL context is provided by the caller */
static void swapchain_blit(IWineD3DSwapChainImpl *This, struct wined3d_context *context,
        const RECT *src_rect, const RECT *dst_rect)
{
    IWineD3DDeviceImpl *device = This->device;
    IWineD3DSurfaceImpl *backbuffer = ((IWineD3DSurfaceImpl *) This->backBuffer[0]);
    UINT src_w = src_rect->right - src_rect->left;
    UINT src_h = src_rect->bottom - src_rect->top;
    GLenum gl_filter;
    const struct wined3d_gl_info *gl_info = context->gl_info;

    TRACE("swapchain %p, context %p, src_rect %s, dst_rect %s.\n",
            This, context, wine_dbgstr_rect(src_rect), wine_dbgstr_rect(dst_rect));

    if (src_w == dst_rect->right - dst_rect->left && src_h == dst_rect->bottom - dst_rect->top)
        gl_filter = GL_NEAREST;
    else
        gl_filter = GL_LINEAR;

    if (gl_info->fbo_ops.glBlitFramebuffer)
    {
        ENTER_GL();
        context_bind_fbo(context, GL_READ_FRAMEBUFFER, &context->src_fbo);
        context_attach_surface_fbo(context, GL_READ_FRAMEBUFFER, 0, This->backBuffer[0]);
        context_attach_depth_stencil_fbo(context, GL_READ_FRAMEBUFFER, NULL, FALSE);

        context_bind_fbo(context, GL_DRAW_FRAMEBUFFER, NULL);
        context_set_draw_buffer(context, GL_BACK);

        glDisable(GL_SCISSOR_TEST);
        IWineD3DDeviceImpl_MarkStateDirty(This->device, STATE_RENDER(WINED3DRS_SCISSORTESTENABLE));

        /* Note that the texture is upside down */
        gl_info->fbo_ops.glBlitFramebuffer(src_rect->left, src_rect->top, src_rect->right, src_rect->bottom,
                                           dst_rect->left, dst_rect->bottom, dst_rect->right, dst_rect->top,
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

        context2 = context_acquire(This->device, This->backBuffer[0], CTXUSAGE_BLIT);

        if(backbuffer->Flags & SFLAG_NORMCOORD)
        {
            tex_left /= src_w;
            tex_right /= src_w;
            tex_top /= src_h;
            tex_bottom /= src_h;
        }

        ENTER_GL();
        context_bind_fbo(context2, GL_DRAW_FRAMEBUFFER, NULL);

        /* Set up the texture. The surface is not in a IWineD3D*Texture container,
         * so there are no d3d texture settings to dirtify
         */
        device->blitter->set_shader((IWineD3DDevice *) device, backbuffer->resource.format_desc,
                                    backbuffer->texture_target, backbuffer->pow2Width,
                                    backbuffer->pow2Height);
        glTexParameteri(backbuffer->texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(backbuffer->texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        context_set_draw_buffer(context, GL_BACK);

        /* Set the viewport to the destination rectandle, disable any projection
         * transformation set up by CTXUSAGE_BLIT, and draw a (-1,-1)-(1,1) quad.
         *
         * Back up viewport and matrix to avoid breaking last_was_blit
         *
         * Note that CTXUSAGE_BLIT set up viewport and ortho to match the surface
         * size - we want the GL drawable(=window) size.
         */
        glPushAttrib(GL_VIEWPORT_BIT);
        glViewport(dst_rect->left, dst_rect->top, dst_rect->right, dst_rect->bottom);
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

        device->blitter->unset_shader((IWineD3DDevice *) device);
        checkGLcall("Swapchain present blit(manual)\n");
        LEAVE_GL();

        context_release(context2);
    }
}

static HRESULT WINAPI IWineD3DSwapChainImpl_Present(IWineD3DSwapChain *iface, CONST RECT *pSourceRect, CONST RECT *pDestRect, HWND hDestWindowOverride, CONST RGNDATA *pDirtyRegion, DWORD dwFlags) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    struct wined3d_context *context;
    RECT src_rect, dst_rect;
    BOOL render_to_fbo;
    unsigned int sync;
    int retval;

    IWineD3DSwapChain_SetDestWindowOverride(iface, hDestWindowOverride);

    context = context_acquire(This->device, This->backBuffer[0], CTXUSAGE_RESOURCELOAD);

    /* Render the cursor onto the back buffer, using our nifty directdraw blitting code :-) */
    if (This->device->bCursorVisible && This->device->cursorTexture)
    {
        IWineD3DSurfaceImpl cursor;
        RECT destRect =
        {
            This->device->xScreenSpace - This->device->xHotSpot,
            This->device->yScreenSpace - This->device->yHotSpot,
            This->device->xScreenSpace + This->device->cursorWidth - This->device->xHotSpot,
            This->device->yScreenSpace + This->device->cursorHeight - This->device->yHotSpot,
        };
        TRACE("Rendering the cursor. Creating fake surface at %p\n", &cursor);
        /* Build a fake surface to call the Blitting code. It is not possible to use the interface passed by
         * the application because we are only supposed to copy the information out. Using a fake surface
         * allows to use the Blitting engine and avoid copying the whole texture -> render target blitting code.
         */
        memset(&cursor, 0, sizeof(cursor));
        cursor.lpVtbl = &IWineD3DSurface_Vtbl;
        cursor.resource.ref = 1;
        cursor.resource.device = This->device;
        cursor.resource.pool = WINED3DPOOL_SCRATCH;
        cursor.resource.format_desc = getFormatDescEntry(WINED3DFMT_B8G8R8A8_UNORM, context->gl_info);
        cursor.resource.resourceType = WINED3DRTYPE_SURFACE;
        cursor.texture_name = This->device->cursorTexture;
        cursor.texture_target = GL_TEXTURE_2D;
        cursor.texture_level = 0;
        cursor.currentDesc.Width = This->device->cursorWidth;
        cursor.currentDesc.Height = This->device->cursorHeight;
        cursor.glRect.left = 0;
        cursor.glRect.top = 0;
        cursor.glRect.right = cursor.currentDesc.Width;
        cursor.glRect.bottom = cursor.currentDesc.Height;
        /* The cursor must have pow2 sizes */
        cursor.pow2Width = cursor.currentDesc.Width;
        cursor.pow2Height = cursor.currentDesc.Height;
        /* The surface is in the texture */
        cursor.Flags |= SFLAG_INTEXTURE;
        /* DDBLT_KEYSRC will cause BltOverride to enable the alpha test with GL_NOTEQUAL, 0.0,
         * which is exactly what we want :-)
         */
        if (This->presentParms.Windowed) {
            MapWindowPoints(NULL, This->win_handle, (LPPOINT)&destRect, 2);
        }
        IWineD3DSurface_Blt(This->backBuffer[0], &destRect, (IWineD3DSurface *)&cursor,
                NULL, WINEDDBLT_KEYSRC, NULL, WINED3DTEXF_POINT);
    }

    if (This->device->logo_surface)
    {
        /* Blit the logo into the upper left corner of the drawable. */
        IWineD3DSurface_BltFast(This->backBuffer[0], 0, 0, This->device->logo_surface, NULL, WINEDDBLTFAST_SRCCOLORKEY);
    }

    TRACE("Presenting HDC %p.\n", context->hdc);

    render_to_fbo = This->render_to_fbo;

    if (pSourceRect)
    {
        src_rect = *pSourceRect;
        if (!render_to_fbo && (src_rect.left || src_rect.top
                || src_rect.right != This->presentParms.BackBufferWidth
                || src_rect.bottom != This->presentParms.BackBufferHeight))
        {
            render_to_fbo = TRUE;
        }
    }
    else
    {
        src_rect.left = 0;
        src_rect.top = 0;
        src_rect.right = This->presentParms.BackBufferWidth;
        src_rect.bottom = This->presentParms.BackBufferHeight;
    }

    if (pDestRect) dst_rect = *pDestRect;
    else GetClientRect(This->win_handle, &dst_rect);

    if (!render_to_fbo && (dst_rect.left || dst_rect.top
            || dst_rect.right != This->presentParms.BackBufferWidth
            || dst_rect.bottom != This->presentParms.BackBufferHeight))
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
    if (!This->render_to_fbo && render_to_fbo && wined3d_settings.offscreen_rendering_mode == ORM_FBO)
    {
        IWineD3DSurface_LoadLocation(This->backBuffer[0], SFLAG_INTEXTURE, NULL);
        IWineD3DSurface_ModifyLocation(This->backBuffer[0], SFLAG_INDRAWABLE, FALSE);
        This->render_to_fbo = TRUE;

        /* Force the context manager to update the render target configuration next draw. */
        context->current_rt = NULL;
    }

    if(This->render_to_fbo)
    {
        /* This codepath should only be hit with the COPY swapeffect. Otherwise a backbuffer-
         * window size mismatch is impossible(fullscreen) and src and dst rectangles are
         * not allowed(they need the COPY swapeffect)
         *
         * The DISCARD swap effect is ok as well since any backbuffer content is allowed after
         * the swap
         */
        if(This->presentParms.SwapEffect == WINED3DSWAPEFFECT_FLIP )
        {
            FIXME("Render-to-fbo with WINED3DSWAPEFFECT_FLIP\n");
        }

        swapchain_blit(This, context, &src_rect, &dst_rect);
    }

    if (This->num_contexts > 1) wglFinish();
    SwapBuffers(context->hdc); /* TODO: cycle through the swapchain buffers */

    TRACE("SwapBuffers called, Starting new frame\n");
    /* FPS support */
    if (TRACE_ON(fps))
    {
        DWORD time = GetTickCount();
        This->frames++;
        /* every 1.5 seconds */
        if (time - This->prev_time > 1500) {
            TRACE_(fps)("%p @ approx %.2ffps\n", This, 1000.0*This->frames/(time - This->prev_time));
            This->prev_time = time;
            This->frames = 0;
        }
    }

#if defined(FRAME_DEBUGGING)
{
    if (GetFileAttributesA("C:\\D3DTRACE") != INVALID_FILE_ATTRIBUTES) {
        if (!isOn) {
            isOn = TRUE;
            FIXME("Enabling D3D Trace\n");
            __WINE_SET_DEBUGGING(__WINE_DBCL_TRACE, __wine_dbch_d3d, 1);
#if defined(SHOW_FRAME_MAKEUP)
            FIXME("Singe Frame snapshots Starting\n");
            isDumpingFrames = TRUE;
            ENTER_GL();
            glClear(GL_COLOR_BUFFER_BIT);
            LEAVE_GL();
#endif

#if defined(SINGLE_FRAME_DEBUGGING)
        } else {
#if defined(SHOW_FRAME_MAKEUP)
            FIXME("Singe Frame snapshots Finishing\n");
            isDumpingFrames = FALSE;
#endif
            FIXME("Singe Frame trace complete\n");
            DeleteFileA("C:\\D3DTRACE");
            __WINE_SET_DEBUGGING(__WINE_DBCL_TRACE, __wine_dbch_d3d, 0);
#endif
        }
    } else {
        if (isOn) {
            isOn = FALSE;
#if defined(SHOW_FRAME_MAKEUP)
            FIXME("Single Frame snapshots Finishing\n");
            isDumpingFrames = FALSE;
#endif
            FIXME("Disabling D3D Trace\n");
            __WINE_SET_DEBUGGING(__WINE_DBCL_TRACE, __wine_dbch_d3d, 0);
        }
    }
}
#endif

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
     * (The Max Payne bug has been confirmed on Windows with the debug runtime)
     */
    if (FALSE && This->presentParms.SwapEffect == WINED3DSWAPEFFECT_DISCARD) {
        TRACE("Clearing the color buffer with cyan color\n");

        IWineD3DDevice_Clear((IWineD3DDevice *)This->device, 0, NULL,
                WINED3DCLEAR_TARGET, 0xff00ffff, 1.0f, 0);
    }

    if(!This->render_to_fbo &&
       ( ((IWineD3DSurfaceImpl *) This->frontBuffer)->Flags   & SFLAG_INSYSMEM ||
         ((IWineD3DSurfaceImpl *) This->backBuffer[0])->Flags & SFLAG_INSYSMEM ) ) {
        /* Both memory copies of the surfaces are ok, flip them around too instead of dirtifying
         * Doesn't work with render_to_fbo because we're not flipping
         */
        IWineD3DSurfaceImpl *front = (IWineD3DSurfaceImpl *) This->frontBuffer;
        IWineD3DSurfaceImpl *back = (IWineD3DSurfaceImpl *) This->backBuffer[0];

        if(front->resource.size == back->resource.size) {
            DWORD fbflags;
            flip_surface(front, back);

            /* Tell the front buffer surface that is has been modified. However,
             * the other locations were preserved during that, so keep the flags.
             * This serves to update the emulated overlay, if any
             */
            fbflags = front->Flags;
            IWineD3DSurface_ModifyLocation(This->frontBuffer, SFLAG_INDRAWABLE, TRUE);
            front->Flags = fbflags;
        } else {
            IWineD3DSurface_ModifyLocation((IWineD3DSurface *) front, SFLAG_INDRAWABLE, TRUE);
            IWineD3DSurface_ModifyLocation((IWineD3DSurface *) back, SFLAG_INDRAWABLE, TRUE);
        }
    } else {
        IWineD3DSurface_ModifyLocation(This->frontBuffer, SFLAG_INDRAWABLE, TRUE);
        /* If the swapeffect is DISCARD, the back buffer is undefined. That means the SYSMEM
         * and INTEXTURE copies can keep their old content if they have any defined content.
         * If the swapeffect is COPY, the content remains the same. If it is FLIP however,
         * the texture / sysmem copy needs to be reloaded from the drawable
         */
        if(This->presentParms.SwapEffect == WINED3DSWAPEFFECT_FLIP) {
            IWineD3DSurface_ModifyLocation(This->backBuffer[0], SFLAG_INDRAWABLE, TRUE);
        }
    }

    if (This->device->stencilBufferTarget)
    {
        if (This->presentParms.Flags & WINED3DPRESENTFLAG_DISCARD_DEPTHSTENCIL
                || ((IWineD3DSurfaceImpl *)This->device->stencilBufferTarget)->Flags & SFLAG_DISCARD)
        {
            surface_modify_ds_location(This->device->stencilBufferTarget, SFLAG_DS_DISCARDED);
        }
    }

    if (This->presentParms.PresentationInterval != WINED3DPRESENT_INTERVAL_IMMEDIATE
            && context->gl_info->supported[SGI_VIDEO_SYNC])
    {
        retval = GL_EXTCALL(glXGetVideoSyncSGI(&sync));
        if(retval != 0) {
            ERR("glXGetVideoSyncSGI failed(retval = %d\n", retval);
        }

        switch(This->presentParms.PresentationInterval) {
            case WINED3DPRESENT_INTERVAL_DEFAULT:
            case WINED3DPRESENT_INTERVAL_ONE:
                if(sync <= This->vSyncCounter) {
                    retval = GL_EXTCALL(glXWaitVideoSyncSGI(1, 0, &This->vSyncCounter));
                } else {
                    This->vSyncCounter = sync;
                }
                break;
            case WINED3DPRESENT_INTERVAL_TWO:
                if(sync <= This->vSyncCounter + 1) {
                    retval = GL_EXTCALL(glXWaitVideoSyncSGI(2, This->vSyncCounter & 0x1, &This->vSyncCounter));
                } else {
                    This->vSyncCounter = sync;
                }
                break;
            case WINED3DPRESENT_INTERVAL_THREE:
                if(sync <= This->vSyncCounter + 2) {
                    retval = GL_EXTCALL(glXWaitVideoSyncSGI(3, This->vSyncCounter % 0x3, &This->vSyncCounter));
                } else {
                    This->vSyncCounter = sync;
                }
                break;
            case WINED3DPRESENT_INTERVAL_FOUR:
                if(sync <= This->vSyncCounter + 3) {
                    retval = GL_EXTCALL(glXWaitVideoSyncSGI(4, This->vSyncCounter & 0x3, &This->vSyncCounter));
                } else {
                    This->vSyncCounter = sync;
                }
                break;
            default:
                FIXME("Unknown presentation interval %08x\n", This->presentParms.PresentationInterval);
        }
    }

    context_release(context);

    TRACE("returning\n");
    return WINED3D_OK;
}

static HRESULT WINAPI IWineD3DSwapChainImpl_SetDestWindowOverride(IWineD3DSwapChain *iface, HWND window) {
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *)iface;
    WINED3DLOCKED_RECT r;
    BYTE *mem;

    if (!window || window == This->win_handle) return WINED3D_OK;

    TRACE("Performing dest override of swapchain %p from window %p to %p\n", This, This->win_handle, window);
    if (This->context[0] == This->device->contexts[0])
    {
        /* The primary context 'owns' all the opengl resources. Destroying and recreating that context requires downloading
         * all opengl resources, deleting the gl resources, destroying all other contexts, then recreating all other contexts
         * and reload the resources
         */
        delete_opengl_contexts((IWineD3DDevice *)This->device, iface);
        This->win_handle = window;
        create_primary_opengl_context((IWineD3DDevice *)This->device, iface);
    }
    else
    {
        This->win_handle = window;

        /* The old back buffer has to be copied over to the new back buffer. A lockrect - switchcontext - unlockrect
         * would suffice in theory, but it is rather nasty and may cause troubles with future changes of the locking code
         * So lock read only, copy the surface out, then lock with the discard flag and write back
         */
        IWineD3DSurface_LockRect(This->backBuffer[0], &r, NULL, WINED3DLOCK_READONLY);
        mem = HeapAlloc(GetProcessHeap(), 0, r.Pitch * ((IWineD3DSurfaceImpl *) This->backBuffer[0])->currentDesc.Height);
        memcpy(mem, r.pBits, r.Pitch * ((IWineD3DSurfaceImpl *) This->backBuffer[0])->currentDesc.Height);
        IWineD3DSurface_UnlockRect(This->backBuffer[0]);

        context_destroy(This->device, This->context[0]);
        This->context[0] = context_create(This->device, (IWineD3DSurfaceImpl *)This->frontBuffer,
                This->win_handle, FALSE /* pbuffer */, &This->presentParms);
        context_release(This->context[0]);

        IWineD3DSurface_LockRect(This->backBuffer[0], &r, NULL, WINED3DLOCK_DISCARD);
        memcpy(r.pBits, mem, r.Pitch * ((IWineD3DSurfaceImpl *) This->backBuffer[0])->currentDesc.Height);
        HeapFree(GetProcessHeap(), 0, mem);
        IWineD3DSurface_UnlockRect(This->backBuffer[0]);
    }
    return WINED3D_OK;
}

static const IWineD3DSwapChainVtbl IWineD3DSwapChain_Vtbl =
{
    /* IUnknown */
    IWineD3DBaseSwapChainImpl_QueryInterface,
    IWineD3DBaseSwapChainImpl_AddRef,
    IWineD3DBaseSwapChainImpl_Release,
    /* IWineD3DSwapChain */
    IWineD3DBaseSwapChainImpl_GetParent,
    IWineD3DSwapChainImpl_Destroy,
    IWineD3DBaseSwapChainImpl_GetDevice,
    IWineD3DSwapChainImpl_Present,
    IWineD3DSwapChainImpl_SetDestWindowOverride,
    IWineD3DBaseSwapChainImpl_GetFrontBufferData,
    IWineD3DBaseSwapChainImpl_GetBackBuffer,
    IWineD3DBaseSwapChainImpl_GetRasterStatus,
    IWineD3DBaseSwapChainImpl_GetDisplayMode,
    IWineD3DBaseSwapChainImpl_GetPresentParameters,
    IWineD3DBaseSwapChainImpl_SetGammaRamp,
    IWineD3DBaseSwapChainImpl_GetGammaRamp
};

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

void swapchain_setup_fullscreen_window(IWineD3DSwapChainImpl *swapchain, UINT w, UINT h)
{
    IWineD3DDeviceImpl *device = swapchain->device;
    HWND window = swapchain->win_handle;
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
    SetWindowPos(window, HWND_TOP, 0, 0, w, h, SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOACTIVATE);

    device->filter_messages = filter_messages;
}

void swapchain_restore_fullscreen_window(IWineD3DSwapChainImpl *swapchain)
{
    IWineD3DDeviceImpl *device = swapchain->device;
    HWND window = swapchain->win_handle;
    BOOL filter_messages;
    LONG style, exstyle;

    if (!device->style && !device->exStyle) return;

    TRACE("Restoring window style of window %p to %08x, %08x.\n",
            window, device->style, device->exStyle);

    style = GetWindowLongW(window, GWL_STYLE);
    exstyle = GetWindowLongW(window, GWL_EXSTYLE);

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
    SetWindowPos(window, 0, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);

    device->filter_messages = filter_messages;

    /* Delete the old values. */
    device->style = 0;
    device->exStyle = 0;
}


HRESULT swapchain_init(IWineD3DSwapChainImpl *swapchain, WINED3DSURFTYPE surface_type,
        IWineD3DDeviceImpl *device, WINED3DPRESENT_PARAMETERS *present_parameters, IUnknown *parent)
{
    const struct wined3d_adapter *adapter = device->adapter;
    const struct GlPixelFormatDesc *format_desc;
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
            swapchain->lpVtbl = &IWineGDISwapChain_Vtbl;
            break;

        case SURFACE_OPENGL:
            swapchain->lpVtbl = &IWineD3DSwapChain_Vtbl;
            break;

        case SURFACE_UNKNOWN:
            FIXME("Caller tried to create a SURFACE_UNKNOWN swapchain.\n");
            return WINED3DERR_INVALIDCALL;
    }

    window = present_parameters->hDeviceWindow ? present_parameters->hDeviceWindow : device->createParms.hFocusWindow;

    swapchain->device = device;
    swapchain->parent = parent;
    swapchain->ref = 1;
    swapchain->win_handle = window;

    if (!present_parameters->Windowed && window)
    {
        swapchain_setup_fullscreen_window(swapchain, present_parameters->BackBufferWidth,
                present_parameters->BackBufferHeight);
    }

    IWineD3D_GetAdapterDisplayMode(device->wined3d, adapter->ordinal, &mode);
    swapchain->orig_width = mode.Width;
    swapchain->orig_height = mode.Height;
    swapchain->orig_fmt = mode.Format;
    format_desc = getFormatDescEntry(mode.Format, &adapter->gl_info);

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
    hr = IWineD3DDeviceParent_CreateRenderTarget(device->device_parent, parent,
            swapchain->presentParms.BackBufferWidth, swapchain->presentParms.BackBufferHeight,
            swapchain->presentParms.BackBufferFormat, swapchain->presentParms.MultiSampleType,
            swapchain->presentParms.MultiSampleQuality, TRUE /* Lockable */, &swapchain->frontBuffer);
    if (FAILED(hr))
    {
        WARN("Failed to create front buffer, hr %#x.\n", hr);
        goto err;
    }

    IWineD3DSurface_SetContainer(swapchain->frontBuffer, (IWineD3DBase *)swapchain);
    ((IWineD3DSurfaceImpl *)swapchain->frontBuffer)->Flags |= SFLAG_SWAPCHAIN;
    if (surface_type == SURFACE_OPENGL)
    {
        IWineD3DSurface_ModifyLocation(swapchain->frontBuffer, SFLAG_INDRAWABLE, TRUE);
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

        hr = IWineD3DDevice_SetDisplayMode((IWineD3DDevice *)device, 0, &mode);
        if (FAILED(hr))
        {
            WARN("Failed to set display mode, hr %#x.\n", hr);
            goto err;
        }
        displaymode_set = TRUE;
    }

    swapchain->context = HeapAlloc(GetProcessHeap(), 0, sizeof(swapchain->context));
    if (!swapchain->context)
    {
        ERR("Failed to create the context array.\n");
        hr = E_OUTOFMEMORY;
        goto err;
    }
    swapchain->num_contexts = 1;

    if (surface_type == SURFACE_OPENGL)
    {
        swapchain->context[0] = context_create(device, (IWineD3DSurfaceImpl *)swapchain->frontBuffer,
                window, FALSE /* pbuffer */, present_parameters);
        if (!swapchain->context[0])
        {
            WARN("Failed to create context.\n");
            hr = WINED3DERR_NOTAVAILABLE;
            goto err;
        }
        context_release(swapchain->context[0]);
    }
    else
    {
        swapchain->context[0] = NULL;
    }

    if (swapchain->presentParms.BackBufferCount > 0)
    {
        swapchain->backBuffer = HeapAlloc(GetProcessHeap(), 0,
                sizeof(*swapchain->backBuffer) * swapchain->presentParms.BackBufferCount);
        if (!swapchain->backBuffer)
        {
            ERR("Failed to allocate backbuffer array memory.\n");
            hr = E_OUTOFMEMORY;
            goto err;
        }

        for (i = 0; i < swapchain->presentParms.BackBufferCount; ++i)
        {
            TRACE("Creating back buffer %u.\n", i);
            hr = IWineD3DDeviceParent_CreateRenderTarget(device->device_parent, parent,
                    swapchain->presentParms.BackBufferWidth, swapchain->presentParms.BackBufferHeight,
                    swapchain->presentParms.BackBufferFormat, swapchain->presentParms.MultiSampleType,
                    swapchain->presentParms.MultiSampleQuality, TRUE /* Lockable */, &swapchain->backBuffer[i]);
            if (FAILED(hr))
            {
                WARN("Failed to create back buffer %u, hr %#x.\n", i, hr);
                goto err;
            }

            IWineD3DSurface_SetContainer(swapchain->backBuffer[i], (IWineD3DBase *)swapchain);
            ((IWineD3DSurfaceImpl *)swapchain->backBuffer[i])->Flags |= SFLAG_SWAPCHAIN;
        }
    }

    /* Swapchains share the depth/stencil buffer, so only create a single depthstencil surface. */
    if (present_parameters->EnableAutoDepthStencil && surface_type == SURFACE_OPENGL)
    {
        TRACE("Creating depth/stencil buffer.\n");
        if (!device->auto_depth_stencil_buffer)
        {
            hr = IWineD3DDeviceParent_CreateDepthStencilSurface(device->device_parent, parent,
                    swapchain->presentParms.BackBufferWidth, swapchain->presentParms.BackBufferHeight,
                    swapchain->presentParms.AutoDepthStencilFormat, swapchain->presentParms.MultiSampleType,
                    swapchain->presentParms.MultiSampleQuality, FALSE /* FIXME: Discard */,
                    &device->auto_depth_stencil_buffer);
            if (FAILED(hr))
            {
                WARN("Failed to create the auto depth stencil, hr %#x.\n", hr);
                goto err;
            }

            IWineD3DSurface_SetContainer(device->auto_depth_stencil_buffer, NULL);
        }
    }

    IWineD3DSwapChain_GetGammaRamp((IWineD3DSwapChain *)swapchain, &swapchain->orig_gamma);

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
        devmode.dmBitsPerPel = format_desc->byte_count * 8;
        devmode.dmPelsWidth = swapchain->orig_width;
        devmode.dmPelsHeight = swapchain->orig_height;
        ChangeDisplaySettingsExW(adapter->DeviceName, &devmode, NULL, CDS_FULLSCREEN, NULL);
    }

    if (swapchain->backBuffer)
    {
        for (i = 0; i < swapchain->presentParms.BackBufferCount; ++i)
        {
            if (swapchain->backBuffer[i]) IWineD3DSurface_Release(swapchain->backBuffer[i]);
        }
        HeapFree(GetProcessHeap(), 0, swapchain->backBuffer);
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

    if (swapchain->frontBuffer) IWineD3DSurface_Release(swapchain->frontBuffer);

    return hr;
}

struct wined3d_context *swapchain_create_context_for_thread(IWineD3DSwapChain *iface)
{
    IWineD3DSwapChainImpl *This = (IWineD3DSwapChainImpl *) iface;
    struct wined3d_context **newArray;
    struct wined3d_context *ctx;

    TRACE("Creating a new context for swapchain %p, thread %d\n", This, GetCurrentThreadId());

    ctx = context_create(This->device, (IWineD3DSurfaceImpl *)This->frontBuffer,
            This->context[0]->win_handle, FALSE /* pbuffer */, &This->presentParms);
    if (!ctx)
    {
        ERR("Failed to create a new context for the swapchain\n");
        return NULL;
    }
    context_release(ctx);

    newArray = HeapAlloc(GetProcessHeap(), 0, sizeof(*newArray) * This->num_contexts + 1);
    if(!newArray) {
        ERR("Out of memory when trying to allocate a new context array\n");
        context_destroy(This->device, ctx);
        return NULL;
    }
    memcpy(newArray, This->context, sizeof(*newArray) * This->num_contexts);
    HeapFree(GetProcessHeap(), 0, This->context);
    newArray[This->num_contexts] = ctx;
    This->context = newArray;
    This->num_contexts++;

    TRACE("Returning context %p\n", ctx);
    return ctx;
}

void get_drawable_size_swapchain(struct wined3d_context *context, UINT *width, UINT *height)
{
    IWineD3DSurfaceImpl *surface = (IWineD3DSurfaceImpl *)context->current_rt;
    /* The drawable size of an onscreen drawable is the surface size.
     * (Actually: The window size, but the surface is created in window size) */
    *width = surface->currentDesc.Width;
    *height = surface->currentDesc.Height;
}
