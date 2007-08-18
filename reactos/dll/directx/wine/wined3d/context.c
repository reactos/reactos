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

#define GLINFO_LOCATION This->adapter->gl_info

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
 *  hdc: device context
 *  glCtx: WGL context to add
 *  pbuffer: optional pbuffer used with this context
 *
 *****************************************************************************/
static WineD3DContext *AddContextToArray(IWineD3DDeviceImpl *This, HWND win_handle, HDC hdc, HGLRC glCtx, HPBUFFERARB pbuffer) {
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

    This->contexts[This->numContexts]->hdc = hdc;
    This->contexts[This->numContexts]->glCtx = glCtx;
    This->contexts[This->numContexts]->pbuffer = pbuffer;
    This->contexts[This->numContexts]->win_handle = win_handle;
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

/*****************************************************************************
 * CreateContext
 *
 * Creates a new context for a window, or a pbuffer context.
 *
 * * Params:
 *  This: Device to activate the context for
 *  target: Surface this context will render to
 *  win_handle: handle to the window which we are drawing to
 *  create_pbuffer: tells whether to create a pbuffer or not
 *  pPresentParameters: contains the pixelformats to use for onscreen rendering
 *
 *****************************************************************************/
WineD3DContext *CreateContext(IWineD3DDeviceImpl *This, IWineD3DSurfaceImpl *target, HWND win_handle, BOOL create_pbuffer, const WINED3DPRESENT_PARAMETERS *pPresentParms) {
    HDC oldDrawable, hdc;
    HPBUFFERARB pbuffer = NULL;
    HGLRC ctx = NULL, oldCtx;
    WineD3DContext *ret = NULL;
    int s;

    TRACE("(%p): Creating a %s context for render target %p\n", This, create_pbuffer ? "offscreen" : "onscreen", target);

    if(create_pbuffer) {
        HDC hdc_parent = GetDC(win_handle);
        int iPixelFormat = 0;
        short red, green, blue, alphaBits, colorBits;
        short depthBits, stencilBits;

        IWineD3DSurface *StencilSurface = This->stencilBufferTarget;
        WINED3DFORMAT StencilBufferFormat = (NULL != StencilSurface) ? ((IWineD3DSurfaceImpl *) StencilSurface)->resource.format : 0;

        int attribs[256];
        int nAttribs = 0;
        unsigned int nFormats;

#define PUSH1(att)        attribs[nAttribs++] = (att);
#define PUSH2(att,value)  attribs[nAttribs++] = (att); attribs[nAttribs++] = (value);

        /* Retrieve the specifications for the pixelformat from the backbuffer / stencilbuffer */
        getColorBits(target->resource.format, &red, &green, &blue, &alphaBits, &colorBits);
        getDepthStencilBits(StencilBufferFormat, &depthBits, &stencilBits);
        PUSH2(WGL_DRAW_TO_PBUFFER_ARB, 1); /* We need pbuffer support; doublebuffering isn't needed */
        PUSH2(WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB); /* Make sure we don't get a float or color index format */
        PUSH2(WGL_COLOR_BITS_ARB, colorBits);
        PUSH2(WGL_ALPHA_BITS_ARB, alphaBits);
        PUSH2(WGL_DEPTH_BITS_ARB, depthBits);
        PUSH2(WGL_STENCIL_BITS_ARB, stencilBits);
        PUSH1(0); /* end the list */

#undef PUSH1
#undef PUSH2

        /* Try to find a pixelformat that matches exactly. If that fails let ChoosePixelFormat try to find a close match */
        if(!GL_EXTCALL(wglChoosePixelFormatARB(hdc_parent, (const int*)&attribs, NULL, 1, &iPixelFormat, &nFormats)))
        {
            PIXELFORMATDESCRIPTOR pfd;

            TRACE("Falling back to ChoosePixelFormat as wglChoosePixelFormatARB failed\n");

            ZeroMemory(&pfd, sizeof(pfd));
            pfd.nSize      = sizeof(pfd);
            pfd.nVersion   = 1;
            pfd.dwFlags    = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER_DONTCARE | PFD_DRAW_TO_WINDOW;
            pfd.iPixelType = PFD_TYPE_RGBA;
            pfd.cColorBits = colorBits;
            pfd.cDepthBits = depthBits;
            pfd.cStencilBits = stencilBits;
            pfd.iLayerType = PFD_MAIN_PLANE;

            iPixelFormat = ChoosePixelFormat(hdc_parent, &pfd);
            if(!iPixelFormat) {
                /* If this happens something is very wrong as ChoosePixelFormat barely fails */
                ERR("Can't find a suitable iPixelFormat for the pbuffer\n");
            }
        }

        TRACE("Creating a pBuffer drawable for the new context\n");
        pbuffer = GL_EXTCALL(wglCreatePbufferARB(hdc_parent, iPixelFormat, target->currentDesc.Width, target->currentDesc.Height, 0));
        if(!pbuffer) {
            ERR("Cannot create a pbuffer\n");
            ReleaseDC(win_handle, hdc_parent);
            goto out;
        }

        /* In WGL a pbuffer is 'wrapped' inside a HDC to 'fool' wglMakeCurrent */
        hdc = GL_EXTCALL(wglGetPbufferDCARB(pbuffer));
        if(!hdc) {
            ERR("Cannot get a HDC for pbuffer (%p)\n", pbuffer);
            GL_EXTCALL(wglDestroyPbufferARB(pbuffer));
            ReleaseDC(win_handle, hdc_parent);
            goto out;
        }
        ReleaseDC(win_handle, hdc_parent);
    } else {
        PIXELFORMATDESCRIPTOR pfd;
        int iPixelFormat;
        short red, green, blue, alpha;
        short colorBits;
        short depthBits, stencilBits;
        int res;

        hdc = GetDC(win_handle);
        if(hdc == NULL) {
            ERR("Cannot retrieve a device context!\n");
            goto out;
        }

        /* PixelFormat selection */
        /* TODO: fill cColorBits/cDepthBits with target->resource.format */
        ZeroMemory(&pfd, sizeof(pfd));
        pfd.nSize      = sizeof(pfd);
        pfd.nVersion   = 1;
        pfd.dwFlags    = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;/*PFD_GENERIC_ACCELERATED*/
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 0;
        pfd.cStencilBits = 0;
        pfd.iLayerType = PFD_MAIN_PLANE;

        /* Try to match the colorBits of the d3d format */
        if(getColorBits(target->resource.format, &red, &green, &blue, &alpha, &colorBits))
            pfd.cColorBits = colorBits;

        /* Retrieve the depth stencil format from the present parameters.
         * The choice of the proper format can give a nice performance boost
         * in case of GPU limited programs. */
        if(pPresentParms->EnableAutoDepthStencil) {
            TRACE("pPresentParms->EnableAutoDepthStencil=enabled; using AutoDepthStencilFormat=%s\n", debug_d3dformat(pPresentParms->AutoDepthStencilFormat));
            if(getDepthStencilBits(pPresentParms->AutoDepthStencilFormat, &depthBits, &stencilBits)) {
                pfd.cDepthBits = depthBits;
                pfd.cStencilBits = stencilBits;
            }
        }

        iPixelFormat = ChoosePixelFormat(hdc, &pfd);
        if(!iPixelFormat) {
            /* If this happens something is very wrong as ChoosePixelFormat barely fails */
            ERR("Can't find a suitable iPixelFormat\n");
        }

        DescribePixelFormat(hdc, iPixelFormat, sizeof(pfd), &pfd);
        res = SetPixelFormat(hdc, iPixelFormat, NULL);
        if(!res) {
            int oldPixelFormat = GetPixelFormat(hdc);

            if(oldPixelFormat) {
                /* OpenGL doesn't allow pixel format adjustments. Print an error and continue using the old format.
                 * There's a big chance that the old format works although with a performance hit and perhaps rendering errors. */
                ERR("HDC=%p is already set to iPixelFormat=%d and OpenGL doesn't allow changes!\n", hdc, oldPixelFormat);
            }
            else {
                ERR("SetPixelFormat failed on HDC=%p for iPixelFormat=%d\n", hdc, iPixelFormat);
                return FALSE;
            }
        }
    }

    ctx = pwglCreateContext(hdc);
    if(This->numContexts) pwglShareLists(This->contexts[0]->glCtx, ctx);

    if(!ctx) {
        ERR("Failed to create a WGL context\n");
        if(create_pbuffer) {
            GL_EXTCALL(wglReleasePbufferDCARB(pbuffer, hdc));
            GL_EXTCALL(wglDestroyPbufferARB(pbuffer));
        }
        goto out;
    }
    ret = AddContextToArray(This, win_handle, hdc, ctx, pbuffer);
    if(!ret) {
        ERR("Failed to add the newly created context to the context list\n");
        pwglDeleteContext(ctx);
        if(create_pbuffer) {
            GL_EXTCALL(wglReleasePbufferDCARB(pbuffer, hdc));
            GL_EXTCALL(wglDestroyPbufferARB(pbuffer));
        }
        goto out;
    }
    ret->surface = (IWineD3DSurface *) target;
    ret->isPBuffer = create_pbuffer;
    ret->tid = GetCurrentThreadId();

    TRACE("Successfully created new context %p\n", ret);

    /* Set up the context defaults */
    oldCtx  = pwglGetCurrentContext();
    oldDrawable = pwglGetCurrentDC();
    if(pwglMakeCurrent(hdc, ctx) == FALSE) {
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

    glPixelStorei(GL_PACK_ALIGNMENT, This->surface_alignment);
    checkGLcall("glPixelStorei(GL_PACK_ALIGNMENT, This->surface_alignment);");
    glPixelStorei(GL_UNPACK_ALIGNMENT, This->surface_alignment);
    checkGLcall("glPixelStorei(GL_UNPACK_ALIGNMENT, This->surface_alignment);");

    if(GL_SUPPORT(APPLE_CLIENT_STORAGE)) {
        /* Most textures will use client storage if supported. Exceptions are non-native power of 2 textures
         * and textures in DIB sections(due to the memory protection).
         */
        glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
        checkGLcall("glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE)");
    }
    if(GL_SUPPORT(ARB_VERTEX_BLEND)) {
        /* Direct3D always uses n-1 weights for n world matrices and uses 1 - sum for the last one
         * this is equal to GL_WEIGHT_SUM_UNITY_ARB. Enabling it doesn't do anything unless
         * GL_VERTEX_BLEND_ARB isn't enabled too
         */
        glEnable(GL_WEIGHT_SUM_UNITY_ARB);
        checkGLcall("glEnable(GL_WEIGHT_SUM_UNITY_ARB)");
    }
    if(GL_SUPPORT(NV_TEXTURE_SHADER2)) {
        glEnable(GL_TEXTURE_SHADER_NV);
        checkGLcall("glEnable(GL_TEXTURE_SHADER_NV)");

        /* Set up the previous texture input for all shader units. This applies to bump mapping, and in d3d
         * the previous texture where to source the offset from is always unit - 1.
         */
        for(s = 1; s < GL_LIMITS(textures); s++) {
            GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + s));
            glTexEnvi(GL_TEXTURE_SHADER_NV, GL_PREVIOUS_TEXTURE_INPUT_NV, GL_TEXTURE0_ARB + s - 1);
            checkGLcall("glTexEnvi(GL_TEXTURE_SHADER_NV, GL_PREVIOUS_TEXTURE_INPUT_NV, ...\n");
        }
    }
    if(GL_SUPPORT(ARB_POINT_SPRITE)) {
        for(s = 0; s < GL_LIMITS(textures); s++) {
            GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + s));
            glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
            checkGLcall("glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE)\n");
        }
    }

    if(oldDrawable && oldCtx) {
        pwglMakeCurrent(oldDrawable, oldCtx);
    }

out:
    return ret;
}

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
        for(s = 0; s < This->numContexts; s++) {
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
    if(pwglGetCurrentContext() == context->glCtx){
        pwglMakeCurrent(NULL, NULL);
    }

    if(context->isPBuffer) {
        GL_EXTCALL(wglReleasePbufferDCARB(context->pbuffer, context->hdc));
        GL_EXTCALL(wglDestroyPbufferARB(context->pbuffer));
    } else ReleaseDC(context->win_handle, context->hdc);
    pwglDeleteContext(context->glCtx);

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

    if(GL_SUPPORT(NV_TEXTURE_SHADER2)) {
        glDisable(GL_TEXTURE_SHADER_NV);
        checkGLcall("glDisable(GL_TEXTURE_SHADER_NV)");
    }
}

/*****************************************************************************
 * findThreadContextForSwapChain
 *
 * Searches a swapchain for all contexts and picks one for the thread tid.
 * If none can be found the swapchain is requested to create a new context
 *
 *****************************************************************************/
static WineD3DContext *findThreadContextForSwapChain(IWineD3DSwapChain *swapchain, DWORD tid) {
    int i;

    for(i = 0; i < ((IWineD3DSwapChainImpl *) swapchain)->num_contexts; i++) {
        if(((IWineD3DSwapChainImpl *) swapchain)->context[i]->tid == tid) {
            return ((IWineD3DSwapChainImpl *) swapchain)->context[i];
        }

    }

    /* Create a new context for the thread */
    return IWineD3DSwapChainImpl_CreateContextForThread(swapchain);
}

/*****************************************************************************
 * FindContext
 *
 * Finds a context for the current render target and thread
 *
 * Parameters:
 *  target: Render target to find the context for
 *  tid: Thread to activate the context for
 *
 * Returns: The needed context
 *
 *****************************************************************************/
static inline WineD3DContext *FindContext(IWineD3DDeviceImpl *This, IWineD3DSurface *target, DWORD tid) {
    IWineD3DSwapChain *swapchain = NULL;
    HRESULT hr;
    BOOL readTexture = wined3d_settings.offscreen_rendering_mode != ORM_FBO && This->render_offscreen;
    WineD3DContext *context = This->activeContext;
    BOOL oldRenderOffscreen = This->render_offscreen;

    hr = IWineD3DSurface_GetContainer(target, &IID_IWineD3DSwapChain, (void **) &swapchain);
    if(hr == WINED3D_OK && swapchain) {
        TRACE("Rendering onscreen\n");

        context = findThreadContextForSwapChain(swapchain, tid);

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
        } else if(wined3d_settings.offscreen_rendering_mode == ORM_PBUFFER) {
            if(This->pbufferContext && tid == This->pbufferContext->tid) {
                This->pbufferContext->tid = 0;
            }
        }
        IWineD3DSwapChain_Release(swapchain);

        if(oldRenderOffscreen) {
            Context_MarkStateDirty(context, WINED3DRS_CULLMODE);
            Context_MarkStateDirty(context, WINED3DTS_PROJECTION);
            Context_MarkStateDirty(context, STATE_VDECL);
            Context_MarkStateDirty(context, STATE_VIEWPORT);
            Context_MarkStateDirty(context, STATE_SCISSORRECT);
        }

    } else {
        TRACE("Rendering offscreen\n");
        This->render_offscreen = TRUE;

        switch(wined3d_settings.offscreen_rendering_mode) {
            case ORM_FBO:
                /* FBOs do not need a different context. Stay with whatever context is active at the moment */
                if(This->activeContext && tid == This->lastThread) {
                    context = This->activeContext;
                } else {
                    /* This may happen if the app jumps streight into offscreen rendering
                     * Start using the context of the primary swapchain. tid == 0 is no problem
                     * for findThreadContextForSwapChain.
                     *
                     * Can also happen on thread switches - in that case findThreadContextForSwapChain
                     * is perfect to call.
                     */
                    context = findThreadContextForSwapChain(This->swapchains[0], tid);
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
                            ((IWineD3DSwapChainImpl *) This->swapchains[0])->context[0]->win_handle,
                            TRUE /* pbuffer */, &((IWineD3DSwapChainImpl *)This->swapchains[0])->presentParms);
                    This->pbufferWidth = targetimpl->currentDesc.Width;
                    This->pbufferHeight = targetimpl->currentDesc.Height;
                   }

                   if(This->pbufferContext) {
                       if(This->pbufferContext->tid != 0 && This->pbufferContext->tid != tid) {
                           FIXME("The PBuffr context is only supported for one thread for now!\n");
                       }
                       This->pbufferContext->tid = tid;
                       context = This->pbufferContext;
                       break;
                   } else {
                       ERR("Failed to create a buffer context and drawable, falling back to back buffer offscreen rendering\n");
                       wined3d_settings.offscreen_rendering_mode = ORM_BACKBUFFER;
                   }
            }

            case ORM_BACKBUFFER:
                /* Stay with the currently active context for back buffer rendering */
                if(This->activeContext && tid == This->lastThread) {
                    context = This->activeContext;
                } else {
                    /* This may happen if the app jumps streight into offscreen rendering
                     * Start using the context of the primary swapchain. tid == 0 is no problem
                     * for findThreadContextForSwapChain.
                     *
                     * Can also happen on thread switches - in that case findThreadContextForSwapChain
                     * is perfect to call.
                     */
                    context = findThreadContextForSwapChain(This->swapchains[0], tid);
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
            Context_MarkStateDirty(context, STATE_SCISSORRECT);
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

    if(oldRenderOffscreen != This->render_offscreen && This->depth_copy_state != WINED3D_DCS_NO_COPY) {
        This->depth_copy_state = WINED3D_DCS_COPY;
    }
    return context;
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
    DWORD                         tid = GetCurrentThreadId();
    int                           i;
    DWORD                         dirtyState, idx;
    BYTE                          shift;
    WineD3DContext                *context;

    TRACE("(%p): Selecting context for render target %p, thread %d\n", This, target, tid);

    ENTER_GL();
    if(This->lastActiveRenderTarget != target || tid != This->lastThread) {
        context = FindContext(This, target, tid);
        This->lastActiveRenderTarget = target;
        This->lastThread = tid;
    } else {
        /* Stick to the old context */
        context = This->activeContext;
    }

    /* Activate the opengl context */
    if(context != This->activeContext) {
        BOOL ret;
        TRACE("Switching gl ctx to %p, hdc=%p ctx=%p\n", context, context->hdc, context->glCtx);
        LEAVE_GL();
        ret = pwglMakeCurrent(context->hdc, context->glCtx);
        ENTER_GL();
        if(ret == FALSE) {
            ERR("Failed to activate the new context\n");
        }
        This->activeContext = context;
    }

    switch(usage) {
        case CTXUSAGE_RESOURCELOAD:
            /* This does not require any special states to be set up */
            break;

        case CTXUSAGE_CLEAR:
            if(context->last_was_blit && GL_SUPPORT(NV_TEXTURE_SHADER2)) {
                glEnable(GL_TEXTURE_SHADER_NV);
                checkGLcall("glEnable(GL_TEXTURE_SHADER_NV)");
            }

            glEnable(GL_SCISSOR_TEST);
            checkGLcall("glEnable GL_SCISSOR_TEST");
            context->last_was_blit = FALSE;
            Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_SCISSORTESTENABLE));
            Context_MarkStateDirty(context, STATE_SCISSORRECT);
            break;

        case CTXUSAGE_DRAWPRIM:
            /* This needs all dirty states applied */
            if(context->last_was_blit && GL_SUPPORT(NV_TEXTURE_SHADER2)) {
                glEnable(GL_TEXTURE_SHADER_NV);
                checkGLcall("glEnable(GL_TEXTURE_SHADER_NV)");
            }

            IWineD3DDeviceImpl_FindTexUnitMap(This);

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
    LEAVE_GL();
}
