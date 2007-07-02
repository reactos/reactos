/*
 * Context and render target management in wined3d
 *
 * Copyright 2007 Stefan Dösinger for CodeWeavers
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
#include <stdio.h>
#ifdef HAVE_FLOAT_H
# include <float.h>
#endif
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

#define GLINFO_LOCATION ((IWineD3DImpl *)(This->wineD3D))->gl_info

/*****************************************************************************
 * Context_MarkStateDirty
 *
 * Marks a state in a context dirty. Only one context, opposed to
 * IWineD3DDeviceImpl_MarkStateDirty, which marks the state dirty in all
 * contexts
 *
 * Params:
 *  context: Context to mark the state dirty in
 *  state: State to mark dirty
 *
 *****************************************************************************/
static void Context_MarkStateDirty(WineD3DContext *context, DWORD state) {
    DWORD rep = StateTable[state].representative;
    DWORD idx;
    BYTE shift;

    if(!rep || isStateDirty(context, rep)) return;

    context->dirtyArray[context->numDirtyEntries++] = rep;
    idx = rep >> 5;
    shift = rep & 0x1f;
    context->isStateDirty[idx] |= (1 << shift);
}

/*****************************************************************************
 * AddContextToArray
 *
 * Adds a context to the context array. Helper function for CreateContext
 *
 * This method is not called in performance-critical code paths, only when a
 * new render target or swapchain is created. Thus performance is not an issue
 * here.
 *
 * Params:
 *  This: Device to add the context for
 *  display: X display this context uses
 *  glCtx: glX context to add
 *  drawable: drawable used with this context.
 *
 *****************************************************************************/
#ifndef WINE_NATIVEWIN32
static WineD3DContext *AddContextToArray(IWineD3DDeviceImpl *This, Display *display, GLXContext glCtx, Drawable drawable) {
#else
static WineD3DContext *AddContextToArray(IWineD3DDeviceImpl *This, HGLRC glCtx, HDC hdc) {
#endif
    WineD3DContext **oldArray = This->contexts;
    DWORD state;

    This->contexts = HeapAlloc(GetProcessHeap(), 0, sizeof(*This->contexts) * (This->numContexts + 1));
    if(This->contexts == NULL) {
        ERR("Unable to grow the context array\n");
        This->contexts = oldArray;
        return NULL;
    }
    if(oldArray) {
        memcpy(This->contexts, oldArray, sizeof(*This->contexts) * This->numContexts);
    }

    This->contexts[This->numContexts] = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WineD3DContext));
    if(This->contexts[This->numContexts] == NULL) {
        ERR("Unable to allocate a new context\n");
        HeapFree(GetProcessHeap(), 0, This->contexts);
        This->contexts = oldArray;
        return NULL;
    }

#ifndef WINE_NATIVEWIN32
    This->contexts[This->numContexts]->display = display;
    This->contexts[This->numContexts]->glCtx = glCtx;
    This->contexts[This->numContexts]->drawable = drawable;
#else
    This->contexts[This->numContexts]->glCtx = glCtx;
    This->contexts[This->numContexts]->hdc = hdc;
#endif
    HeapFree(GetProcessHeap(), 0, oldArray);

    /* Mark all states dirty to force a proper initialization of the states on the first use of the context
     */
    for(state = 0; state <= STATE_HIGHEST; state++) {
        Context_MarkStateDirty(This->contexts[This->numContexts], state);
    }

    This->numContexts++;
    TRACE("Created context %p\n", This->contexts[This->numContexts - 1]);
    return This->contexts[This->numContexts - 1];
}

#ifndef WINE_NATIVEWIN32
/* Returns an array of compatible FBconfig(s).
 * The array must be freed with XFree. Requires ENTER_GL()
 */
static GLXFBConfig* pbuffer_find_fbconfigs(
    IWineD3DDeviceImpl* This,
    IWineD3DSurfaceImpl* RenderSurface,
    Display *display) {

    GLXFBConfig* cfgs = NULL;
    int nCfgs = 0;
    int attribs[256];
    int nAttribs = 0;

    IWineD3DSurface *StencilSurface = This->stencilBufferTarget;
    WINED3DFORMAT BackBufferFormat = RenderSurface->resource.format;
    WINED3DFORMAT StencilBufferFormat = (NULL != StencilSurface) ? ((IWineD3DSurfaceImpl *) StencilSurface)->resource.format : 0;

    /* TODO:
     *  if StencilSurface == NULL && zBufferTarget != NULL then switch the zbuffer off,
     *  it StencilSurface != NULL && zBufferTarget == NULL switch it on
     */

#define PUSH1(att)        attribs[nAttribs++] = (att);
#define PUSH2(att,value)  attribs[nAttribs++] = (att); attribs[nAttribs++] = (value);

    /* PUSH2(GLX_BIND_TO_TEXTURE_RGBA_ATI, True); examples of this are few and far between (but I've got a nice working one!)*/

    PUSH2(GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT);
    PUSH2(GLX_X_RENDERABLE,  TRUE);
    PUSH2(GLX_DOUBLEBUFFER,  TRUE);
    TRACE("calling makeglcfg\n");
    D3DFmtMakeGlCfg(BackBufferFormat, StencilBufferFormat, attribs, &nAttribs, FALSE /* alternate */);
    PUSH1(None);
    TRACE("calling chooseFGConfig\n");
    cfgs = glXChooseFBConfig(display,
                             DefaultScreen(display),
                             attribs, &nCfgs);
    if (cfgs == NULL) {
        /* OK we didn't find the exact config, so use any reasonable match */
        /* TODO: fill in the 'requested' and 'current' depths, and make sure that's
           why we failed. */
        static BOOL show_message = TRUE;
        if (show_message) {
            ERR("Failed to find exact match, finding alternative but you may "
                "suffer performance issues, try changing xfree's depth to match the requested depth\n");
            show_message = FALSE;
        }
        nAttribs = 0;
        PUSH2(GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT | GLX_WINDOW_BIT);
        /* PUSH2(GLX_X_RENDERABLE,  TRUE); */
        PUSH2(GLX_RENDER_TYPE,   GLX_RGBA_BIT);
        PUSH2(GLX_DOUBLEBUFFER, FALSE);
        TRACE("calling makeglcfg\n");
        D3DFmtMakeGlCfg(BackBufferFormat, StencilBufferFormat, attribs, &nAttribs, TRUE /* alternate */);
        PUSH1(None);
        cfgs = glXChooseFBConfig(display,
                                 DefaultScreen(display),
                                 attribs, &nCfgs);
    }

    if (cfgs == NULL) {
        ERR("Could not get a valid FBConfig for (%u,%s)/(%u,%s)\n",
            BackBufferFormat, debug_d3dformat(BackBufferFormat),
            StencilBufferFormat, debug_d3dformat(StencilBufferFormat));
    } else {
#ifdef EXTRA_TRACES
        int i;
        for (i = 0; i < nCfgs; ++i) {
            TRACE("for (%u,%s)/(%u,%s) found config[%d]@%p\n", BackBufferFormat,
            debug_d3dformat(BackBufferFormat), StencilBufferFormat,
            debug_d3dformat(StencilBufferFormat), i, cfgs[i]);
        }
#endif
    }
#undef PUSH1
#undef PUSH2

   return cfgs;
}
#endif

/*****************************************************************************
 * CreateContext
 *
 * Creates a new context for a window, or a pbuffer context.
 *
 * * Params:
 *  This: Device to activate the context for
 *  target: Surface this context will render to
 *  display: X11 connection
 *  win: Target window. NULL for a pbuffer
 *
 *****************************************************************************/
#ifndef WINE_NATIVEWIN32
WineD3DContext *CreateContext(IWineD3DDeviceImpl *This, IWineD3DSurfaceImpl *target, Display *display, Window win) {
    Drawable drawable = win, oldDrawable;
    XVisualInfo *visinfo = NULL;
    GLXFBConfig *cfgs = NULL;
    GLXContext ctx = NULL, oldCtx;
    WineD3DContext *ret = NULL;

    TRACE("(%p): Creating a %s context for render target %p\n", This, win ? "onscreen" : "offscreen", target);

    if(!win) {
        int attribs[256];
        int nAttribs = 0;

        TRACE("Creating a pBuffer drawable for the new context\n");

        cfgs = pbuffer_find_fbconfigs(This, target, display);
        if(!cfgs) {
            ERR("Cannot find a frame buffer configuration for the pbuffer\n");
            goto out;
        }

        attribs[nAttribs++] = GLX_PBUFFER_WIDTH;
        attribs[nAttribs++] = target->currentDesc.Width;
        attribs[nAttribs++] = GLX_PBUFFER_HEIGHT;
        attribs[nAttribs++] = target->currentDesc.Height;
        attribs[nAttribs++] = None;

        visinfo = glXGetVisualFromFBConfig(display, cfgs[0]);
        if(!visinfo) {
            ERR("Cannot find a visual for the pbuffer\n");
            goto out;
        }

        drawable = glXCreatePbuffer(display, cfgs[0], attribs);

        if(!drawable) {
            ERR("Cannot create a pbuffer\n");
            goto out;
        }
        XFree(cfgs);
        cfgs = NULL;
    } else {
        /* Create an onscreen target */
        XVisualInfo             template;
        int                     num;

        template.visualid = (VisualID)GetPropA(GetDesktopWindow(), "__wine_x11_visual_id");
        /* TODO: change this to find a similar visual, but one with a stencil/zbuffer buffer that matches the request
        (or the best possible if none is requested) */
        TRACE("Found x visual ID  : %ld\n", template.visualid);
        visinfo   = XGetVisualInfo(display, VisualIDMask, &template, &num);

        if (NULL == visinfo) {
            ERR("cannot really get XVisual\n");
            goto out;
        } else {
            int n, value;
            /* Write out some debug info about the visual/s */
            TRACE("Using x visual ID  : %ld\n", template.visualid);
            TRACE("        visual info: %p\n", visinfo);
            TRACE("        num items  : %d\n", num);
            for (n = 0;n < num; n++) {
                TRACE("=====item=====: %d\n", n + 1);
                TRACE("   visualid      : %ld\n", visinfo[n].visualid);
                TRACE("   screen        : %d\n",  visinfo[n].screen);
                TRACE("   depth         : %u\n",  visinfo[n].depth);
                TRACE("   class         : %d\n",  visinfo[n].class);
                TRACE("   red_mask      : %ld\n", visinfo[n].red_mask);
                TRACE("   green_mask    : %ld\n", visinfo[n].green_mask);
                TRACE("   blue_mask     : %ld\n", visinfo[n].blue_mask);
                TRACE("   colormap_size : %d\n",  visinfo[n].colormap_size);
                TRACE("   bits_per_rgb  : %d\n",  visinfo[n].bits_per_rgb);
                /* log some extra glx info */
                glXGetConfig(display, visinfo, GLX_AUX_BUFFERS, &value);
                TRACE("   gl_aux_buffers  : %d\n",  value);
                glXGetConfig(display, visinfo, GLX_BUFFER_SIZE ,&value);
                TRACE("   gl_buffer_size  : %d\n",  value);
                glXGetConfig(display, visinfo, GLX_RED_SIZE, &value);
                TRACE("   gl_red_size  : %d\n",  value);
                glXGetConfig(display, visinfo, GLX_GREEN_SIZE, &value);
                TRACE("   gl_green_size  : %d\n",  value);
                glXGetConfig(display, visinfo, GLX_BLUE_SIZE, &value);
                TRACE("   gl_blue_size  : %d\n",  value);
                glXGetConfig(display, visinfo, GLX_ALPHA_SIZE, &value);
                TRACE("   gl_alpha_size  : %d\n",  value);
                glXGetConfig(display, visinfo, GLX_DEPTH_SIZE ,&value);
                TRACE("   gl_depth_size  : %d\n",  value);
                glXGetConfig(display, visinfo, GLX_STENCIL_SIZE, &value);
                TRACE("   gl_stencil_size : %d\n",  value);
            }
            /* Now choose a similar visual ID*/
        }
    }

    ctx = glXCreateContext(display, visinfo,
                           This->numContexts ? This->contexts[0]->glCtx : NULL,
                           GL_TRUE);
    if(!ctx) {
        ERR("Failed to create a glX context\n");
        if(drawable != win) glXDestroyPbuffer(display, drawable);
        goto out;
    }
    ret = AddContextToArray(This, display, ctx, drawable);
    if(!ret) {
        ERR("Failed to add the newly created context to the context list\n");
        glXDestroyContext(display, ctx);
        if(drawable != win) glXDestroyPbuffer(display, drawable);
        goto out;
    }
    ret->surface = (IWineD3DSurface *) target;
    ret->isPBuffer = win == 0;

    TRACE("Successfully created new context %p\n", ret);

    /* Set up the context defaults */
    oldCtx  = glXGetCurrentContext();
    oldDrawable = glXGetCurrentDrawable();
    if(glXMakeCurrent(display, drawable, ctx) == FALSE) {
        ERR("Cannot activate context to set up defaults\n");
        goto out;
    }

    TRACE("Setting up the screen\n");
    /* Clear the screen */
    glClearColor(1.0, 0.0, 0.0, 0.0);
    checkGLcall("glClearColor");
    glClearIndex(0);
    glClearDepth(1);
    glClearStencil(0xffff);

    checkGLcall("glClear");

    glColor3f(1.0, 1.0, 1.0);
    checkGLcall("glColor3f");

    glEnable(GL_LIGHTING);
    checkGLcall("glEnable");

    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    checkGLcall("glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);");

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    checkGLcall("glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);");

    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
    checkGLcall("glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);");

    glPixelStorei(GL_PACK_ALIGNMENT, SURFACE_ALIGNMENT);
    checkGLcall("glPixelStorei(GL_PACK_ALIGNMENT, SURFACE_ALIGNMENT);");
    glPixelStorei(GL_UNPACK_ALIGNMENT, SURFACE_ALIGNMENT);
    checkGLcall("glPixelStorei(GL_UNPACK_ALIGNMENT, SURFACE_ALIGNMENT);");

    if(GL_SUPPORT(APPLE_CLIENT_STORAGE)) {
        /* Most textures will use client storage if supported. Exceptions are non-native power of 2 textures
         * and textures in DIB sections(due to the memory protection).
         */
        glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
        checkGLcall("glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE)");
    }

    if(oldDrawable && oldCtx) {
        glXMakeCurrent(display, oldDrawable, oldCtx);
    }

out:
    if(visinfo) XFree(visinfo);
    if(cfgs) XFree(cfgs);
    return ret;
}

#else /*WINE_NATIVEWIN32*/

static BOOL SelectPixelFormat(HDC hdc, DWORD bits) {
    int index = 1;
    int maxindex;

    if (bits == 24)
        bits = 32;
    do {
        PIXELFORMATDESCRIPTOR pfd;

        maxindex = wglDescribePixelFormat(hdc, index, sizeof(pfd), &pfd);
        /* skip non-GL formats */
        if (~pfd.dwFlags
            & (PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL
            /* double buffering and GDI are mutually exclusive */
                | PFD_DOUBLEBUFFER))
            continue;
        /* skip software GL implementation */
        if (pfd.dwFlags & PFD_GENERIC_FORMAT)
            continue;
        /* we're not going to provide a palette, or do we?
        if (pfd.dwFlags & PFD_NEED_SYSTEM_PALETTE)
            continue; */
        /* skip paletized modes
        if (pfd.iPixelType != PFD_TYPE_RGBA)
            continue; */
        /* do we need good depth buffer and stencil support?
        if (pfd.cDepthBits < 15 || pfd.cStencilBits < 8)
            continue; */
        /* match bit depth */
        if (pfd.cColorBits != bits)
            continue;
        /* pixel format can only be set once per window */
        if (wglSetPixelFormat(hdc, index, &pfd))
            return TRUE;
    } while (index++ < maxindex);
    return FALSE;
}

WineD3DContext *CreateContext(IWineD3DDeviceImpl *This, IWineD3DSurfaceImpl *target, HWND hwnd) {
    HDC hdc, oldHdc;
    HGLRC ctx = NULL, oldCtx;
    WineD3DContext *ret = NULL;

    TRACE("(%p): Creating a %s context for render target %p\n", This, hwnd ? "onscreen" : "offscreen", target);

    if(!hwnd) {
        ERR("Cannot find a frame buffer configuration for the pbuffer\n");
        goto out;
    } else {
        hdc = GetDC(hwnd);
    }

    if (!SelectPixelFormat(hdc, GetDeviceCaps(hdc, BITSPIXEL))
        || !(ctx = wglCreateContext(hdc))) {
        ERR("Failed to create a wgl context\n");
        goto out;
    }
    ret = AddContextToArray(This, ctx, hdc);
    if(!ret) {
        ERR("Failed to add the newly created context to the context list\n");
        wglDeleteContext(ctx);
        goto out;
    }
    ret->surface = (IWineD3DSurface *) target;
    ret->isPBuffer = hwnd == 0;

    TRACE("Successfully created new context %p\n", ret);

    /* Set up the context defaults */
    oldCtx = wglGetCurrentContext();
    oldHdc = wglGetCurrentDC();
    if(!wglMakeCurrent(hdc, ctx)) {
        ERR("Cannot activate context to set up defaults\n");
        goto out;
    }

    TRACE("Setting up the screen\n");
    /* Clear the screen */
    glClearColor(1.0, 0.0, 0.0, 0.0);
    checkGLcall("glClearColor");
    glClearIndex(0);
    glClearDepth(1);
    glClearStencil(0xffff);

    checkGLcall("glClear");

    glColor3f(1.0, 1.0, 1.0);
    checkGLcall("glColor3f");

    glEnable(GL_LIGHTING);
    checkGLcall("glEnable");

    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    checkGLcall("glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);");

    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    checkGLcall("glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);");

    glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
    checkGLcall("glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);");

    glPixelStorei(GL_PACK_ALIGNMENT, SURFACE_ALIGNMENT);
    checkGLcall("glPixelStorei(GL_PACK_ALIGNMENT, SURFACE_ALIGNMENT);");
    glPixelStorei(GL_UNPACK_ALIGNMENT, SURFACE_ALIGNMENT);
    checkGLcall("glPixelStorei(GL_UNPACK_ALIGNMENT, SURFACE_ALIGNMENT);");

    if(GL_SUPPORT(APPLE_CLIENT_STORAGE)) {
        /* Most textures will use client storage if supported. Exceptions are non-native power of 2 textures
         * and textures in DIB sections(due to the memory protection).
         */
        glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
        checkGLcall("glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE)");
    }

    if(oldHdc && oldCtx) {
        wglMakeCurrent(oldHdc, oldCtx);
    }

out:
    return ret;
}

#endif /* WINE_NATIVEWIN32 */

/*****************************************************************************
 * RemoveContextFromArray
 *
 * Removes a context from the context manager. The opengl context is not
 * destroyed or unset. context is not a valid pointer after that call.
 *
 * Similar to the former call this isn't a performance critical function. A
 * helper function for DestroyContext.
 *
 * Params:
 *  This: Device to activate the context for
 *  context: Context to remove
 *
 *****************************************************************************/
static void RemoveContextFromArray(IWineD3DDeviceImpl *This, WineD3DContext *context) {
    UINT t, s;
    WineD3DContext **oldArray = This->contexts;

    TRACE("Removing ctx %p\n", context);

    This->numContexts--;

    if(This->numContexts) {
        This->contexts = HeapAlloc(GetProcessHeap(), 0, sizeof(*This->contexts) * This->numContexts);
        if(!This->contexts) {
            ERR("Cannot allocate a new context array, PANIC!!!\n");
        }
        t = 0;
        for(s = 0; s <= This->numContexts; s++) {
            if(oldArray[s] == context) continue;
            This->contexts[t] = oldArray[s];
            t++;
        }
    } else {
        This->contexts = NULL;
    }

    HeapFree(GetProcessHeap(), 0, context);
    HeapFree(GetProcessHeap(), 0, oldArray);
}

/*****************************************************************************
 * DestroyContext
 *
 * Destroys a wineD3DContext
 *
 * Params:
 *  This: Device to activate the context for
 *  context: Context to destroy
 *
 *****************************************************************************/
void DestroyContext(IWineD3DDeviceImpl *This, WineD3DContext *context) {

    /* check that we are the current context first */
    TRACE("Destroying ctx %p\n", context);
#ifndef WINE_NATIVEWIN32
    if(glXGetCurrentContext() == context->glCtx){
        glXMakeCurrent(context->display, None, NULL);
    }

    glXDestroyContext(context->display, context->glCtx);
    if(context->isPBuffer) {
        glXDestroyPbuffer(context->display, context->drawable);
    }
#else
    if(wglGetCurrentContext() == context->glCtx) {
        wglMakeCurrent(0, 0);
    }
    wglDeleteContext(context->glCtx);
    ReleaseDC(WindowFromDC(context->hdc), context->hdc);
    if(context->isPBuffer) {
    }
#endif
    RemoveContextFromArray(This, context);
}

/*****************************************************************************
 * SetupForBlit
 *
 * Sets up a context for DirectDraw blitting.
 * All texture units are disabled, except unit 0
 * Texture unit 0 is activted where GL_TEXTURE_2D is activated
 * fog, lighting, blending, alpha test, z test, scissor test, culling diabled
 * color writing enabled for all channels
 * register combiners disabled, shaders disabled
 * world matris is set to identity, texture matrix 0 too
 * projection matrix is setup for drawing screen coordinates
 *
 * Params:
 *  This: Device to activate the context for
 *  context: Context to setup
 *  width: render target width
 *  height: render target height
 *
 *****************************************************************************/
static inline void SetupForBlit(IWineD3DDeviceImpl *This, WineD3DContext *context, UINT width, UINT height) {
    int i;

    TRACE("Setting up context %p for blitting\n", context);
    if(context->last_was_blit) {
        TRACE("Context is already set up for blitting, nothing to do\n");
        return;
    }
    context->last_was_blit = TRUE;

    /* TODO: Use a display list */

    /* Disable shaders */
    This->shader_backend->shader_cleanup((IWineD3DDevice *) This);
    Context_MarkStateDirty(context, STATE_VSHADER);
    Context_MarkStateDirty(context, STATE_PIXELSHADER);

    /* Disable all textures. The caller can then bind a texture it wants to blit
     * from
     */
    if(GL_SUPPORT(NV_REGISTER_COMBINERS)) {
        glDisable(GL_REGISTER_COMBINERS_NV);
        checkGLcall("glDisable(GL_REGISTER_COMBINERS_NV)");
    }
    if (GL_SUPPORT(ARB_MULTITEXTURE)) {
        /* The blitting code uses (for now) the fixed function pipeline, so make sure to reset all fixed
         * function texture unit. No need to care for higher samplers
         */
        for(i = GL_LIMITS(textures) - 1; i > 0 ; i--) {
            GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + i));
            checkGLcall("glActiveTextureARB");

            if(GL_SUPPORT(ARB_TEXTURE_CUBE_MAP)) {
                glDisable(GL_TEXTURE_CUBE_MAP_ARB);
                checkGLcall("glDisable GL_TEXTURE_CUBE_MAP_ARB");
            }
            glDisable(GL_TEXTURE_3D);
            checkGLcall("glDisable GL_TEXTURE_3D");
            glDisable(GL_TEXTURE_2D);
            checkGLcall("glDisable GL_TEXTURE_2D");

            glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
            checkGLcall("glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);");

            Context_MarkStateDirty(context, STATE_TEXTURESTAGE(i, WINED3DTSS_COLOROP));
            Context_MarkStateDirty(context, STATE_SAMPLER(i));
        }
        GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB));
        checkGLcall("glActiveTextureARB");
    }
    if(GL_SUPPORT(ARB_TEXTURE_CUBE_MAP)) {
        glDisable(GL_TEXTURE_CUBE_MAP_ARB);
        checkGLcall("glDisable GL_TEXTURE_CUBE_MAP_ARB");
    }
    glDisable(GL_TEXTURE_3D);
    checkGLcall("glDisable GL_TEXTURE_3D");
    glEnable(GL_TEXTURE_2D);
    checkGLcall("glEnable GL_TEXTURE_2D");

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    glMatrixMode(GL_TEXTURE);
    checkGLcall("glMatrixMode(GL_TEXTURE)");
    glLoadIdentity();
    checkGLcall("glLoadIdentity()");
    Context_MarkStateDirty(context, STATE_TRANSFORM(WINED3DTS_TEXTURE0));

    if (GL_SUPPORT(EXT_TEXTURE_LOD_BIAS)) {
        glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT,
                  GL_TEXTURE_LOD_BIAS_EXT,
                  0.0);
        checkGLcall("glTexEnvi GL_TEXTURE_LOD_BIAS_EXT ...");
    }
    Context_MarkStateDirty(context, STATE_SAMPLER(0));
    Context_MarkStateDirty(context, STATE_TEXTURESTAGE(0, WINED3DTSS_COLOROP));

    /* Other misc states */
    glDisable(GL_ALPHA_TEST);
    checkGLcall("glDisable(GL_ALPHA_TEST)");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_ALPHATESTENABLE));
    glDisable(GL_LIGHTING);
    checkGLcall("glDisable GL_LIGHTING");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_LIGHTING));
    glDisable(GL_DEPTH_TEST);
    checkGLcall("glDisable GL_DEPTH_TEST");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_ZENABLE));
    glDisable(GL_FOG);
    checkGLcall("glDisable GL_FOG");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_FOGENABLE));
    glDisable(GL_BLEND);
    checkGLcall("glDisable GL_BLEND");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_ALPHABLENDENABLE));
    glDisable(GL_CULL_FACE);
    checkGLcall("glDisable GL_CULL_FACE");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_CULLMODE));
    glDisable(GL_STENCIL_TEST);
    checkGLcall("glDisable GL_STENCIL_TEST");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_STENCILENABLE));
    if(GL_SUPPORT(ARB_POINT_SPRITE)) {
        glDisable(GL_POINT_SPRITE_ARB);
        checkGLcall("glDisable GL_POINT_SPRITE_ARB");
        Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_POINTSPRITEENABLE));
    }
    glColorMask(GL_TRUE, GL_TRUE,GL_TRUE,GL_TRUE);
    checkGLcall("glColorMask");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_CLIPPING));

    /* Setup transforms */
    glMatrixMode(GL_MODELVIEW);
    checkGLcall("glMatrixMode(GL_MODELVIEW)");
    glLoadIdentity();
    checkGLcall("glLoadIdentity()");
    Context_MarkStateDirty(context, STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(0)));

    glMatrixMode(GL_PROJECTION);
    checkGLcall("glMatrixMode(GL_PROJECTION)");
    glLoadIdentity();
    checkGLcall("glLoadIdentity()");
    glOrtho(0, width, height, 0, 0.0, -1.0);
    checkGLcall("glOrtho");
    Context_MarkStateDirty(context, STATE_TRANSFORM(WINED3DTS_PROJECTION));

    context->last_was_rhw = TRUE;
    Context_MarkStateDirty(context, STATE_VDECL); /* because of last_was_rhw = TRUE */

    glDisable(GL_CLIP_PLANE0); checkGLcall("glDisable(clip plane 0)");
    glDisable(GL_CLIP_PLANE1); checkGLcall("glDisable(clip plane 1)");
    glDisable(GL_CLIP_PLANE2); checkGLcall("glDisable(clip plane 2)");
    glDisable(GL_CLIP_PLANE3); checkGLcall("glDisable(clip plane 3)");
    glDisable(GL_CLIP_PLANE4); checkGLcall("glDisable(clip plane 4)");
    glDisable(GL_CLIP_PLANE5); checkGLcall("glDisable(clip plane 5)");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_CLIPPING));

    glViewport(0, 0, width, height);
    checkGLcall("glViewport");
    Context_MarkStateDirty(context, STATE_VIEWPORT);
}

/*****************************************************************************
 * ActivateContext
 *
 * Finds a rendering context and drawable matching the device and render
 * target for the current thread, activates them and puts them into the
 * requested state.
 *
 * Params:
 *  This: Device to activate the context for
 *  target: Requested render target
 *  usage: Prepares the context for blitting, drawing or other actions
 *
 *****************************************************************************/
void ActivateContext(IWineD3DDeviceImpl *This, IWineD3DSurface *target, ContextUsage usage) {
    DWORD tid = This->createParms.BehaviorFlags & WINED3DCREATE_MULTITHREADED ? GetCurrentThreadId() : 0;
    int                           i;
    DWORD                         dirtyState, idx;
    BYTE                          shift;
    WineD3DContext                *context = This->activeContext;
    BOOL                          oldRenderOffscreen = This->render_offscreen;

    TRACE("(%p): Selecting context for render target %p, thread %d\n", This, target, tid);

    if(This->lastActiveRenderTarget != target) {
        IWineD3DSwapChain *swapchain = NULL;
        HRESULT hr;
        BOOL readTexture = wined3d_settings.offscreen_rendering_mode != ORM_FBO && This->render_offscreen;

        hr = IWineD3DSurface_GetContainer(target, &IID_IWineD3DSwapChain, (void **) &swapchain);
        if(hr == WINED3D_OK && swapchain) {
            TRACE("Rendering onscreen\n");
            context = ((IWineD3DSwapChainImpl *) swapchain)->context[0];
            This->render_offscreen = FALSE;
            /* The context != This->activeContext will catch a NOP context change. This can occur
             * if we are switching back to swapchain rendering in case of FBO or Back Buffer offscreen
             * rendering. No context change is needed in that case
             */

            if (wined3d_settings.offscreen_rendering_mode == ORM_BACKBUFFER) {
                if(((IWineD3DSwapChainImpl *) swapchain)->backBuffer) {
                    glDrawBuffer(GL_BACK);
                    checkGLcall("glDrawBuffer(GL_BACK)");
                } else {
                    glDrawBuffer(GL_FRONT);
                    checkGLcall("glDrawBuffer(GL_FRONT)");
                }
            }
            IWineD3DSwapChain_Release(swapchain);

            if(oldRenderOffscreen) {
                Context_MarkStateDirty(context, WINED3DRS_CULLMODE);
                Context_MarkStateDirty(context, WINED3DTS_PROJECTION);
                Context_MarkStateDirty(context, STATE_VDECL);
                Context_MarkStateDirty(context, STATE_VIEWPORT);
            }
        } else {
            TRACE("Rendering offscreen\n");
            This->render_offscreen = TRUE;

            switch(wined3d_settings.offscreen_rendering_mode) {
                case ORM_FBO:
                    /* FBOs do not need a different context. Stay with whatever context is active at the moment */
                    if(This->activeContext) {
                        context = This->activeContext;
                    } else {
                        /* This may happen if the app jumps streight into offscreen rendering
                         * Start using the context of the primary swapchain
                         */
                        context = ((IWineD3DSwapChainImpl *) This->swapchains[0])->context[0];
                    }
                    break;

                case ORM_PBUFFER:
                {
                    IWineD3DSurfaceImpl *targetimpl = (IWineD3DSurfaceImpl *) target;
                    if(This->pbufferContext == NULL ||
                       This->pbufferWidth < targetimpl->currentDesc.Width ||
                       This->pbufferHeight < targetimpl->currentDesc.Height) {
                        if(This->pbufferContext) {
                            DestroyContext(This, This->pbufferContext);
                        }

                        /* The display is irrelevant here, the window is 0. But CreateContext needs a valid X connection.
                         * Create the context on the same server as the primary swapchain. The primary swapchain is exists at this point.
                         */
                        This->pbufferContext = CreateContext(This, targetimpl,
#ifndef WINE_NATIVEWIN32
                                                             ((IWineD3DSwapChainImpl *) This->swapchains[0])->context[0]->display,
                                                             0 /* Window */);
#else
                                                             0 /* HWND */);
#endif
                        This->pbufferWidth = targetimpl->currentDesc.Width;
                        This->pbufferHeight = targetimpl->currentDesc.Height;
                    }

                    if(This->pbufferContext) {
                        context = This->pbufferContext;
                        break;
                    } else {
                        ERR("Failed to create a buffer context and drawable, falling back to back buffer offscreen rendering\n");
                        wined3d_settings.offscreen_rendering_mode = ORM_BACKBUFFER;
                    }
                }

                case ORM_BACKBUFFER:
                    /* Stay with the currently active context for back buffer rendering */
                    if(This->activeContext) {
                        context = This->activeContext;
                    } else {
                        /* This may happen if the app jumps streight into offscreen rendering
                         * Start using the context of the primary swapchain
                         */
                        context = ((IWineD3DSwapChainImpl *) This->swapchains[0])->context[0];
                    }
                    glDrawBuffer(This->offscreenBuffer);
                    checkGLcall("glDrawBuffer(This->offscreenBuffer)");
                    break;
            }

            if (wined3d_settings.offscreen_rendering_mode != ORM_FBO) {
                /* Make sure we have a OpenGL texture name so the PreLoad() used to read the buffer
                 * back when we are done won't mark us dirty.
                 */
                IWineD3DSurface_PreLoad(target);
            }

            if(!oldRenderOffscreen) {
                Context_MarkStateDirty(context, WINED3DRS_CULLMODE);
                Context_MarkStateDirty(context, WINED3DTS_PROJECTION);
                Context_MarkStateDirty(context, STATE_VDECL);
                Context_MarkStateDirty(context, STATE_VIEWPORT);
            }
        }
        if (readTexture) {
            BOOL oldInDraw = This->isInDraw;

            /* PreLoad requires a context to load the texture, thus it will call ActivateContext.
             * Set the isInDraw to true to signal PreLoad that it has a context. Will be tricky
             * when using offscreen rendering with multithreading
             */
            This->isInDraw = TRUE;

            /* Do that before switching the context:
             * Read the back buffer of the old drawable into the destination texture
             */
            IWineD3DSurface_PreLoad(This->lastActiveRenderTarget);

            /* Assume that the drawable will be modified by some other things now */
            ((IWineD3DSurfaceImpl *) This->lastActiveRenderTarget)->Flags &= ~SFLAG_INDRAWABLE;

            This->isInDraw = oldInDraw;
        }
        This->lastActiveRenderTarget = target;
        if(oldRenderOffscreen != This->render_offscreen && This->depth_copy_state != WINED3D_DCS_NO_COPY) {
            This->depth_copy_state = WINED3D_DCS_COPY;
        }
    } else {
        /* Stick to the old context */
        context = This->activeContext;
    }

    /* Activate the opengl context */
    if(context != This->activeContext) {
#ifndef WINE_NATIVEWIN32
        Bool ret;
        TRACE("Switching gl ctx to %p, drawable=%ld, ctx=%p\n", context, context->drawable, context->glCtx);
        ret = glXMakeCurrent(context->display, context->drawable, context->glCtx);
        if(ret == FALSE) {
#else
        TRACE("Switching gl ctx to %p, hdc=%p, ctx=%p\n", context, context->hdc, context->glCtx);
        if(!wglMakeCurrent(context->hdc, context->glCtx)) {
#endif
            ERR("Failed to activate the new context\n");
        }
        This->activeContext = context;
    }

    switch(usage) {
        case CTXUSAGE_RESOURCELOAD:
            /* This does not require any special states to be set up */
            break;

        case CTXUSAGE_DRAWPRIM:
            /* This needs all dirty states applied */
            for(i=0; i < context->numDirtyEntries; i++) {
                dirtyState = context->dirtyArray[i];
                idx = dirtyState >> 5;
                shift = dirtyState & 0x1f;
                context->isStateDirty[idx] &= ~(1 << shift);
                StateTable[dirtyState].apply(dirtyState, This->stateBlock, context);
            }
            context->numDirtyEntries = 0; /* This makes the whole list clean */
            context->last_was_blit = FALSE;
            break;

        case CTXUSAGE_BLIT:
            SetupForBlit(This, context,
                         ((IWineD3DSurfaceImpl *)target)->currentDesc.Width,
                         ((IWineD3DSurfaceImpl *)target)->currentDesc.Height);
            break;

        default:
            FIXME("Unexpected context usage requested\n");
    }
}
