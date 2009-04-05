/*
 * Context and render target management in wined3d
 *
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
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

/* The last used device.
 *
 * If the application creates multiple devices and switches between them, ActivateContext has to
 * change the opengl context. This flag allows to keep track which device is active
 */
static IWineD3DDeviceImpl *last_device;

/* FBO helper functions */

void context_bind_fbo(IWineD3DDevice *iface, GLenum target, GLuint *fbo)
{
    const IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;

    if (!*fbo)
    {
        GL_EXTCALL(glGenFramebuffersEXT(1, fbo));
        checkGLcall("glGenFramebuffersEXT()");
        TRACE("Created FBO %d\n", *fbo);
    }

    GL_EXTCALL(glBindFramebufferEXT(target, *fbo));
    checkGLcall("glBindFramebuffer()");
}

static void context_destroy_fbo(IWineD3DDeviceImpl *This, const GLuint *fbo)
{
    unsigned int i;

    GL_EXTCALL(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, *fbo));
    checkGLcall("glBindFramebuffer()");
    for (i = 0; i < GL_LIMITS(buffers); ++i)
    {
        GL_EXTCALL(glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + i, GL_TEXTURE_2D, 0, 0));
        checkGLcall("glFramebufferTexture2D()");
    }
    GL_EXTCALL(glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, 0, 0));
    checkGLcall("glFramebufferTexture2D()");
    GL_EXTCALL(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0));
    checkGLcall("glBindFramebuffer()");
    GL_EXTCALL(glDeleteFramebuffersEXT(1, fbo));
    checkGLcall("glDeleteFramebuffers()");
}

static void context_apply_attachment_filter_states(IWineD3DDevice *iface, IWineD3DSurface *surface, BOOL force_preload)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    const IWineD3DSurfaceImpl *surface_impl = (IWineD3DSurfaceImpl *)surface;
    IWineD3DBaseTextureImpl *texture_impl;
    BOOL update_minfilter = FALSE;
    BOOL update_magfilter = FALSE;

    /* Update base texture states array */
    if (SUCCEEDED(IWineD3DSurface_GetContainer(surface, &IID_IWineD3DBaseTexture, (void **)&texture_impl)))
    {
        if (texture_impl->baseTexture.states[WINED3DTEXSTA_MINFILTER] != WINED3DTEXF_POINT
            || texture_impl->baseTexture.states[WINED3DTEXSTA_MIPFILTER] != WINED3DTEXF_NONE)
        {
            texture_impl->baseTexture.states[WINED3DTEXSTA_MINFILTER] = WINED3DTEXF_POINT;
            texture_impl->baseTexture.states[WINED3DTEXSTA_MIPFILTER] = WINED3DTEXF_NONE;
            update_minfilter = TRUE;
        }

        if (texture_impl->baseTexture.states[WINED3DTEXSTA_MAGFILTER] != WINED3DTEXF_POINT)
        {
            texture_impl->baseTexture.states[WINED3DTEXSTA_MAGFILTER] = WINED3DTEXF_POINT;
            update_magfilter = TRUE;
        }

        if (texture_impl->baseTexture.bindCount)
        {
            WARN("Render targets should not be bound to a sampler\n");
            IWineD3DDeviceImpl_MarkStateDirty(This, STATE_SAMPLER(texture_impl->baseTexture.sampler));
        }

        IWineD3DBaseTexture_Release((IWineD3DBaseTexture *)texture_impl);
    }

    if (update_minfilter || update_magfilter || force_preload)
    {
        GLenum target, bind_target;
        GLint old_binding;

        target = surface_impl->glDescription.target;
        if (target == GL_TEXTURE_2D)
        {
            bind_target = GL_TEXTURE_2D;
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &old_binding);
        } else if (target == GL_TEXTURE_RECTANGLE_ARB) {
            bind_target = GL_TEXTURE_RECTANGLE_ARB;
            glGetIntegerv(GL_TEXTURE_BINDING_RECTANGLE_ARB, &old_binding);
        } else {
            bind_target = GL_TEXTURE_CUBE_MAP_ARB;
            glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP_ARB, &old_binding);
        }

        surface_internal_preload(surface, SRGB_RGB);

        glBindTexture(bind_target, surface_impl->glDescription.textureName);
        if (update_minfilter) glTexParameteri(bind_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        if (update_magfilter) glTexParameteri(bind_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(bind_target, old_binding);
    }

    checkGLcall("apply_attachment_filter_states()");
}

/* TODO: Handle stencil attachments */
void context_attach_depth_stencil_fbo(IWineD3DDeviceImpl *This, GLenum fbo_target, IWineD3DSurface *depth_stencil, BOOL use_render_buffer)
{
    IWineD3DSurfaceImpl *depth_stencil_impl = (IWineD3DSurfaceImpl *)depth_stencil;

    TRACE("Attach depth stencil %p\n", depth_stencil);

    if (depth_stencil)
    {
        if (use_render_buffer && depth_stencil_impl->current_renderbuffer)
        {
            GL_EXTCALL(glFramebufferRenderbufferEXT(fbo_target, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, depth_stencil_impl->current_renderbuffer->id));
            checkGLcall("glFramebufferRenderbufferEXT()");
        } else {
            context_apply_attachment_filter_states((IWineD3DDevice *)This, depth_stencil, TRUE);

            GL_EXTCALL(glFramebufferTexture2DEXT(fbo_target, GL_DEPTH_ATTACHMENT_EXT, depth_stencil_impl->glDescription.target,
                        depth_stencil_impl->glDescription.textureName, depth_stencil_impl->glDescription.level));
            checkGLcall("glFramebufferTexture2DEXT()");
        }
    } else {
        GL_EXTCALL(glFramebufferTexture2DEXT(fbo_target, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, 0, 0));
        checkGLcall("glFramebufferTexture2DEXT()");
    }
}

void context_attach_surface_fbo(IWineD3DDeviceImpl *This, GLenum fbo_target, DWORD idx, IWineD3DSurface *surface)
{
    const IWineD3DSurfaceImpl *surface_impl = (IWineD3DSurfaceImpl *)surface;

    TRACE("Attach surface %p to %u\n", surface, idx);

    if (surface)
    {
        context_apply_attachment_filter_states((IWineD3DDevice *)This, surface, TRUE);

        GL_EXTCALL(glFramebufferTexture2DEXT(fbo_target, GL_COLOR_ATTACHMENT0_EXT + idx, surface_impl->glDescription.target,
                surface_impl->glDescription.textureName, surface_impl->glDescription.level));
        checkGLcall("glFramebufferTexture2DEXT()");
    } else {
        GL_EXTCALL(glFramebufferTexture2DEXT(fbo_target, GL_COLOR_ATTACHMENT0_EXT + idx, GL_TEXTURE_2D, 0, 0));
        checkGLcall("glFramebufferTexture2DEXT()");
    }
}

static void context_check_fbo_status(IWineD3DDevice *iface)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    GLenum status;

    status = GL_EXTCALL(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT));
    if (status == GL_FRAMEBUFFER_COMPLETE_EXT)
    {
        TRACE("FBO complete\n");
    } else {
        IWineD3DSurfaceImpl *attachment;
        unsigned int i;
        FIXME("FBO status %s (%#x)\n", debug_fbostatus(status), status);

        /* Dump the FBO attachments */
        for (i = 0; i < GL_LIMITS(buffers); ++i)
        {
            attachment = (IWineD3DSurfaceImpl *)This->activeContext->current_fbo->render_targets[i];
            if (attachment)
            {
                FIXME("\tColor attachment %d: (%p) %s %ux%u\n",
                        i, attachment, debug_d3dformat(attachment->resource.format_desc->format),
                        attachment->pow2Width, attachment->pow2Height);
            }
        }
        attachment = (IWineD3DSurfaceImpl *)This->activeContext->current_fbo->depth_stencil;
        if (attachment)
        {
            FIXME("\tDepth attachment: (%p) %s %ux%u\n",
                    attachment, debug_d3dformat(attachment->resource.format_desc->format),
                    attachment->pow2Width, attachment->pow2Height);
        }
    }
}

static struct fbo_entry *context_create_fbo_entry(IWineD3DDevice *iface)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    struct fbo_entry *entry;

    entry = HeapAlloc(GetProcessHeap(), 0, sizeof(*entry));
    entry->render_targets = HeapAlloc(GetProcessHeap(), 0, GL_LIMITS(buffers) * sizeof(*entry->render_targets));
    memcpy(entry->render_targets, This->render_targets, GL_LIMITS(buffers) * sizeof(*entry->render_targets));
    entry->depth_stencil = This->stencilBufferTarget;
    entry->attached = FALSE;
    entry->id = 0;

    return entry;
}

static void context_destroy_fbo_entry(IWineD3DDeviceImpl *This, struct fbo_entry *entry)
{
    if (entry->id)
    {
        TRACE("Destroy FBO %d\n", entry->id);
        context_destroy_fbo(This, &entry->id);
    }
    list_remove(&entry->entry);
    HeapFree(GetProcessHeap(), 0, entry->render_targets);
    HeapFree(GetProcessHeap(), 0, entry);
}


static struct fbo_entry *context_find_fbo_entry(IWineD3DDevice *iface, WineD3DContext *context)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    struct fbo_entry *entry;

    LIST_FOR_EACH_ENTRY(entry, &context->fbo_list, struct fbo_entry, entry)
    {
        if (!memcmp(entry->render_targets, This->render_targets, GL_LIMITS(buffers) * sizeof(*entry->render_targets))
                && entry->depth_stencil == This->stencilBufferTarget)
        {
            return entry;
        }
    }

    entry = context_create_fbo_entry(iface);
    list_add_head(&context->fbo_list, &entry->entry);
    return entry;
}

static void context_apply_fbo_entry(IWineD3DDevice *iface, struct fbo_entry *entry)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    unsigned int i;

    context_bind_fbo(iface, GL_FRAMEBUFFER_EXT, &entry->id);

    if (!entry->attached)
    {
        /* Apply render targets */
        for (i = 0; i < GL_LIMITS(buffers); ++i)
        {
            IWineD3DSurface *render_target = This->render_targets[i];
            context_attach_surface_fbo(This, GL_FRAMEBUFFER_EXT, i, render_target);
        }

        /* Apply depth targets */
        if (This->stencilBufferTarget) {
            unsigned int w = ((IWineD3DSurfaceImpl *)This->render_targets[0])->pow2Width;
            unsigned int h = ((IWineD3DSurfaceImpl *)This->render_targets[0])->pow2Height;

            surface_set_compatible_renderbuffer(This->stencilBufferTarget, w, h);
        }
        context_attach_depth_stencil_fbo(This, GL_FRAMEBUFFER_EXT, This->stencilBufferTarget, TRUE);

        entry->attached = TRUE;
    } else {
        for (i = 0; i < GL_LIMITS(buffers); ++i)
        {
            if (This->render_targets[i])
                context_apply_attachment_filter_states(iface, This->render_targets[i], FALSE);
        }
        if (This->stencilBufferTarget)
            context_apply_attachment_filter_states(iface, This->stencilBufferTarget, FALSE);
    }

    for (i = 0; i < GL_LIMITS(buffers); ++i)
    {
        if (This->render_targets[i])
            This->draw_buffers[i] = GL_COLOR_ATTACHMENT0_EXT + i;
        else
            This->draw_buffers[i] = GL_NONE;
    }
}

static void context_apply_fbo_state(IWineD3DDevice *iface)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    WineD3DContext *context = This->activeContext;

    if (This->render_offscreen)
    {
        context->current_fbo = context_find_fbo_entry(iface, context);
        context_apply_fbo_entry(iface, context->current_fbo);
    } else {
        context->current_fbo = NULL;
        GL_EXTCALL(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0));
    }

    context_check_fbo_status(iface);
}

void context_resource_released(IWineD3DDevice *iface, IWineD3DResource *resource, WINED3DRESOURCETYPE type)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    UINT i;

    switch(type)
    {
        case WINED3DRTYPE_SURFACE:
        {
            for (i = 0; i < This->numContexts; ++i)
            {
                struct fbo_entry *entry, *entry2;

                LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &This->contexts[i]->fbo_list, struct fbo_entry, entry)
                {
                    BOOL destroyed = FALSE;
                    UINT j;

                    for (j = 0; !destroyed && j < GL_LIMITS(buffers); ++j)
                    {
                        if (entry->render_targets[j] == (IWineD3DSurface *)resource)
                        {
                            context_destroy_fbo_entry(This, entry);
                            destroyed = TRUE;
                        }
                    }

                    if (!destroyed && entry->depth_stencil == (IWineD3DSurface *)resource)
                        context_destroy_fbo_entry(This, entry);
                }
            }

            break;
        }

        default:
            break;
    }
}

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
 *  StateTable: Pointer to the state table in use(for state grouping)
 *
 *****************************************************************************/
static void Context_MarkStateDirty(WineD3DContext *context, DWORD state, const struct StateEntry *StateTable) {
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
        Context_MarkStateDirty(This->contexts[This->numContexts], state, This->StateTable);
    }

    This->numContexts++;
    TRACE("Created context %p\n", This->contexts[This->numContexts - 1]);
    return This->contexts[This->numContexts - 1];
}

/* This function takes care of WineD3D pixel format selection. */
static int WineD3D_ChoosePixelFormat(IWineD3DDeviceImpl *This, HDC hdc,
        const struct GlPixelFormatDesc *color_format_desc, const struct GlPixelFormatDesc *ds_format_desc,
        BOOL auxBuffers, int numSamples, BOOL pbuffer, BOOL findCompatible)
{
    int iPixelFormat=0;
    unsigned int matchtry;
    short redBits, greenBits, blueBits, alphaBits, colorBits;
    short depthBits=0, stencilBits=0;

    struct match_type {
        BOOL require_aux;
        BOOL exact_alpha;
        BOOL exact_color;
    } matches[] = {
        /* First, try without alpha match buffers. MacOS supports aux buffers only
         * on A8R8G8B8, and we prefer better offscreen rendering over an alpha match.
         * Then try without aux buffers - this is the most common cause for not
         * finding a pixel format. Also some drivers(the open source ones)
         * only offer 32 bit ARB pixel formats. First try without an exact alpha
         * match, then try without an exact alpha and color match.
         */
        { TRUE,  TRUE,  TRUE  },
        { TRUE,  FALSE, TRUE  },
        { FALSE, TRUE,  TRUE  },
        { FALSE, FALSE, TRUE  },
        { TRUE,  FALSE, FALSE },
        { FALSE, FALSE, FALSE },
    };

    int i = 0;
    int nCfgs = This->adapter->nCfgs;

    TRACE("ColorFormat=%s, DepthStencilFormat=%s, auxBuffers=%d, numSamples=%d, pbuffer=%d, findCompatible=%d\n",
          debug_d3dformat(color_format_desc->format), debug_d3dformat(ds_format_desc->format),
          auxBuffers, numSamples, pbuffer, findCompatible);

    if (!getColorBits(color_format_desc, &redBits, &greenBits, &blueBits, &alphaBits, &colorBits))
    {
        ERR("Unable to get color bits for format %s (%#x)!\n",
                debug_d3dformat(color_format_desc->format), color_format_desc->format);
        return 0;
    }

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
    if (ds_format_desc->format != WINED3DFMT_D24S8)
    {
        FIXME("Add OpenGL context recreation support to SetDepthStencilSurface\n");
        ds_format_desc = getFormatDescEntry(WINED3DFMT_D24S8, &This->adapter->gl_info);
    }

    getDepthStencilBits(ds_format_desc, &depthBits, &stencilBits);

    for(matchtry = 0; matchtry < (sizeof(matches) / sizeof(matches[0])) && !iPixelFormat; matchtry++) {
        for(i=0; i<nCfgs; i++) {
            BOOL exactDepthMatch = TRUE;
            WineD3D_PixelFormat *cfg = &This->adapter->cfgs[i];

            /* For now only accept RGBA formats. Perhaps some day we will
             * allow floating point formats for pbuffers. */
            if(cfg->iPixelType != WGL_TYPE_RGBA_ARB)
                continue;

            /* In window mode (!pbuffer) we need a window drawable format and double buffering. */
            if(!pbuffer && !(cfg->windowDrawable && cfg->doubleBuffer))
                continue;

            /* We like to have aux buffers in backbuffer mode */
            if(auxBuffers && !cfg->auxBuffers && matches[matchtry].require_aux)
                continue;

            /* In pbuffer-mode we need a pbuffer-capable format but we don't want double buffering */
            if(pbuffer && (!cfg->pbufferDrawable || cfg->doubleBuffer))
                continue;

            if(matches[matchtry].exact_color) {
                if(cfg->redSize != redBits)
                    continue;
                if(cfg->greenSize != greenBits)
                    continue;
                if(cfg->blueSize != blueBits)
                    continue;
            } else {
                if(cfg->redSize < redBits)
                    continue;
                if(cfg->greenSize < greenBits)
                    continue;
                if(cfg->blueSize < blueBits)
                    continue;
            }
            if(matches[matchtry].exact_alpha) {
                if(cfg->alphaSize != alphaBits)
                    continue;
            } else {
                if(cfg->alphaSize < alphaBits)
                    continue;
            }

            /* We try to locate a format which matches our requirements exactly. In case of
             * depth it is no problem to emulate 16-bit using e.g. 24-bit, so accept that. */
            if(cfg->depthSize < depthBits)
                continue;
            else if(cfg->depthSize > depthBits)
                exactDepthMatch = FALSE;

            /* In all cases make sure the number of stencil bits matches our requirements
             * even when we don't need stencil because it could affect performance EXCEPT
             * on cards which don't offer depth formats without stencil like the i915 drivers
             * on Linux. */
            if(stencilBits != cfg->stencilSize && !(This->adapter->brokenStencil && stencilBits <= cfg->stencilSize))
                continue;

            /* Check multisampling support */
            if(cfg->numSamples != numSamples)
                continue;

            /* When we have passed all the checks then we have found a format which matches our
             * requirements. Note that we only check for a limit number of capabilities right now,
             * so there can easily be a dozen of pixel formats which appear to be the 'same' but
             * can still differ in things like multisampling, stereo, SRGB and other flags.
             */

            /* Exit the loop as we have found a format :) */
            if(exactDepthMatch) {
                iPixelFormat = cfg->iPixelFormat;
                break;
            } else if(!iPixelFormat) {
                /* In the end we might end up with a format which doesn't exactly match our depth
                 * requirements. Accept the first format we found because formats with higher iPixelFormat
                 * values tend to have more extended capabilities (e.g. multisampling) which we don't need. */
                iPixelFormat = cfg->iPixelFormat;
            }
        }
    }

    /* When findCompatible is set and no suitable format was found, let ChoosePixelFormat choose a pixel format in order not to crash. */
    if(!iPixelFormat && !findCompatible) {
        ERR("Can't find a suitable iPixelFormat\n");
        return FALSE;
    } else if(!iPixelFormat) {
        PIXELFORMATDESCRIPTOR pfd;

        TRACE("Falling back to ChoosePixelFormat as we weren't able to find an exactly matching pixel format\n");
        /* PixelFormat selection */
        ZeroMemory(&pfd, sizeof(pfd));
        pfd.nSize      = sizeof(pfd);
        pfd.nVersion   = 1;
        pfd.dwFlags    = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW;/*PFD_GENERIC_ACCELERATED*/
        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cAlphaBits = alphaBits;
        pfd.cColorBits = colorBits;
        pfd.cDepthBits = depthBits;
        pfd.cStencilBits = stencilBits;
        pfd.iLayerType = PFD_MAIN_PLANE;

        iPixelFormat = ChoosePixelFormat(hdc, &pfd);
        if(!iPixelFormat) {
            /* If this happens something is very wrong as ChoosePixelFormat barely fails */
            ERR("Can't find a suitable iPixelFormat\n");
            return FALSE;
        }
    }

    TRACE("Found iPixelFormat=%d for ColorFormat=%s, DepthStencilFormat=%s\n",
            iPixelFormat, debug_d3dformat(color_format_desc->format), debug_d3dformat(ds_format_desc->format));
    return iPixelFormat;
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
    unsigned int s;

    TRACE("(%p): Creating a %s context for render target %p\n", This, create_pbuffer ? "offscreen" : "onscreen", target);

    if(create_pbuffer) {
        HDC hdc_parent = GetDC(win_handle);
        int iPixelFormat = 0;

        IWineD3DSurface *StencilSurface = This->stencilBufferTarget;
        const struct GlPixelFormatDesc *ds_format_desc = StencilSurface
                ? ((IWineD3DSurfaceImpl *)StencilSurface)->resource.format_desc
                : getFormatDescEntry(WINED3DFMT_UNKNOWN, &This->adapter->gl_info);

        /* Try to find a pixel format with pbuffer support. */
        iPixelFormat = WineD3D_ChoosePixelFormat(This, hdc_parent, target->resource.format_desc,
                ds_format_desc, FALSE /* auxBuffers */, 0 /* numSamples */, TRUE /* PBUFFER */,
                FALSE /* findCompatible */);
        if(!iPixelFormat) {
            TRACE("Trying to locate a compatible pixel format because an exact match failed.\n");

            /* For some reason we weren't able to find a format, try to find something instead of crashing.
             * A reason for failure could have been wglChoosePixelFormatARB strictness. */
            iPixelFormat = WineD3D_ChoosePixelFormat(This, hdc_parent, target->resource.format_desc,
                    ds_format_desc, FALSE /* auxBuffer */, 0 /* numSamples */, TRUE /* PBUFFER */,
                    TRUE /* findCompatible */);
        }

        /* This shouldn't happen as ChoosePixelFormat always returns something */
        if(!iPixelFormat) {
            ERR("Unable to locate a pixel format for a pbuffer\n");
            ReleaseDC(win_handle, hdc_parent);
            goto out;
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
        int res;
        const struct GlPixelFormatDesc *color_format_desc = target->resource.format_desc;
        const struct GlPixelFormatDesc *ds_format_desc = getFormatDescEntry(WINED3DFMT_UNKNOWN,
                &This->adapter->gl_info);
        BOOL auxBuffers = FALSE;
        int numSamples = 0;

        hdc = GetDC(win_handle);
        if(hdc == NULL) {
            ERR("Cannot retrieve a device context!\n");
            goto out;
        }

        /* In case of ORM_BACKBUFFER, make sure to request an alpha component for X4R4G4B4/X8R8G8B8 as we might need it for the backbuffer. */
        if(wined3d_settings.offscreen_rendering_mode == ORM_BACKBUFFER) {
            auxBuffers = TRUE;

            if (color_format_desc->format == WINED3DFMT_X4R4G4B4)
                color_format_desc = getFormatDescEntry(WINED3DFMT_A4R4G4B4, &This->adapter->gl_info);
            else if (color_format_desc->format == WINED3DFMT_X8R8G8B8)
                color_format_desc = getFormatDescEntry(WINED3DFMT_A8R8G8B8, &This->adapter->gl_info);
        }

        /* DirectDraw supports 8bit paletted render targets and these are used by old games like Starcraft and C&C.
         * Most modern hardware doesn't support 8bit natively so we perform some form of 8bit -> 32bit conversion.
         * The conversion (ab)uses the alpha component for storing the palette index. For this reason we require
         * a format with 8bit alpha, so request A8R8G8B8. */
        if (color_format_desc->format == WINED3DFMT_P8)
            color_format_desc = getFormatDescEntry(WINED3DFMT_A8R8G8B8, &This->adapter->gl_info);

        /* Retrieve the depth stencil format from the present parameters.
         * The choice of the proper format can give a nice performance boost
         * in case of GPU limited programs. */
        if(pPresentParms->EnableAutoDepthStencil) {
            TRACE("pPresentParms->EnableAutoDepthStencil=enabled; using AutoDepthStencilFormat=%s\n", debug_d3dformat(pPresentParms->AutoDepthStencilFormat));
            ds_format_desc = getFormatDescEntry(pPresentParms->AutoDepthStencilFormat, &This->adapter->gl_info);
        }

        /* D3D only allows multisampling when SwapEffect is set to WINED3DSWAPEFFECT_DISCARD */
        if(pPresentParms->MultiSampleType && (pPresentParms->SwapEffect == WINED3DSWAPEFFECT_DISCARD)) {
            if(!GL_SUPPORT(ARB_MULTISAMPLE))
                ERR("The program is requesting multisampling without support!\n");
            else {
                ERR("Requesting MultiSampleType=%d\n", pPresentParms->MultiSampleType);
                numSamples = pPresentParms->MultiSampleType;
            }
        }

        /* Try to find a pixel format which matches our requirements */
        iPixelFormat = WineD3D_ChoosePixelFormat(This, hdc, color_format_desc, ds_format_desc,
                auxBuffers, numSamples, FALSE /* PBUFFER */, FALSE /* findCompatible */);

        /* Try to locate a compatible format if we weren't able to find anything */
        if(!iPixelFormat) {
            TRACE("Trying to locate a compatible pixel format because an exact match failed.\n");
            iPixelFormat = WineD3D_ChoosePixelFormat(This, hdc, color_format_desc, ds_format_desc,
                    auxBuffers, 0 /* numSamples */, FALSE /* PBUFFER */, TRUE /* findCompatible */ );
        }

        /* If we still don't have a pixel format, something is very wrong as ChoosePixelFormat barely fails */
        if(!iPixelFormat) {
            ERR("Can't find a suitable iPixelFormat\n");
            return FALSE;
        }

        DescribePixelFormat(hdc, iPixelFormat, sizeof(pfd), &pfd);
        res = SetPixelFormat(hdc, iPixelFormat, NULL);
        if(!res) {
            int oldPixelFormat = GetPixelFormat(hdc);

            /* By default WGL doesn't allow pixel format adjustments but we need it here.
             * For this reason there is a WINE-specific wglSetPixelFormat which allows you to
             * set the pixel format multiple times. Only use it when it is really needed. */

            if(oldPixelFormat == iPixelFormat) {
                /* We don't have to do anything as the formats are the same :) */
            } else if(oldPixelFormat && GL_SUPPORT(WGL_WINE_PIXEL_FORMAT_PASSTHROUGH)) {
                res = GL_EXTCALL(wglSetPixelFormatWINE(hdc, iPixelFormat, NULL));

                if(!res) {
                    ERR("wglSetPixelFormatWINE failed on HDC=%p for iPixelFormat=%d\n", hdc, iPixelFormat);
                    return FALSE;
                }
            } else if(oldPixelFormat) {
                /* OpenGL doesn't allow pixel format adjustments. Print an error and continue using the old format.
                 * There's a big chance that the old format works although with a performance hit and perhaps rendering errors. */
                ERR("HDC=%p is already set to iPixelFormat=%d and OpenGL doesn't allow changes!\n", hdc, oldPixelFormat);
            } else {
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
    if(This->shader_backend->shader_dirtifyable_constants((IWineD3DDevice *) This)) {
        /* Create the dirty constants array and initialize them to dirty */
        ret->vshader_const_dirty = HeapAlloc(GetProcessHeap(), 0,
                sizeof(*ret->vshader_const_dirty) * GL_LIMITS(vshader_constantsF));
        ret->pshader_const_dirty = HeapAlloc(GetProcessHeap(), 0,
                sizeof(*ret->pshader_const_dirty) * GL_LIMITS(pshader_constantsF));
        memset(ret->vshader_const_dirty, 1,
               sizeof(*ret->vshader_const_dirty) * GL_LIMITS(vshader_constantsF));
        memset(ret->pshader_const_dirty, 1,
                sizeof(*ret->pshader_const_dirty) * GL_LIMITS(pshader_constantsF));
    }

    TRACE("Successfully created new context %p\n", ret);

    list_init(&ret->fbo_list);

    /* Set up the context defaults */
    oldCtx  = pwglGetCurrentContext();
    oldDrawable = pwglGetCurrentDC();
    if(oldCtx && oldDrawable) {
        /* See comment in ActivateContext context switching */
        This->frag_pipe->enable_extension((IWineD3DDevice *) This, FALSE);
    }
    if(pwglMakeCurrent(hdc, ctx) == FALSE) {
        ERR("Cannot activate context to set up defaults\n");
        goto out;
    }

    ENTER_GL();

    glGetIntegerv(GL_AUX_BUFFERS, &ret->aux_buffers);

    TRACE("Setting up the screen\n");
    /* Clear the screen */
    glClearColor(1.0, 0.0, 0.0, 0.0);
    checkGLcall("glClearColor");
    glClearIndex(0);
    glClearDepth(1);
    glClearStencil(0xffff);

    checkGLcall("glClear");

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
    LEAVE_GL();

    /* Never keep GL_FRAGMENT_SHADER_ATI enabled on a context that we switch away from,
     * but enable it for the first context we create, and reenable it on the old context
     */
    if(oldDrawable && oldCtx) {
        pwglMakeCurrent(oldDrawable, oldCtx);
    } else {
        last_device = This;
    }
    This->frag_pipe->enable_extension((IWineD3DDevice *) This, TRUE);

    return ret;

out:
    return NULL;
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
    WineD3DContext **new_array;
    BOOL found = FALSE;
    UINT i;

    TRACE("Removing ctx %p\n", context);

    for (i = 0; i < This->numContexts; ++i)
    {
        if (This->contexts[i] == context)
        {
            HeapFree(GetProcessHeap(), 0, context);
            found = TRUE;
            break;
        }
    }

    if (!found)
    {
        ERR("Context %p doesn't exist in context array\n", context);
        return;
    }

    while (i < This->numContexts - 1)
    {
        This->contexts[i] = This->contexts[i + 1];
        ++i;
    }

    --This->numContexts;
    if (!This->numContexts)
    {
        HeapFree(GetProcessHeap(), 0, This->contexts);
        This->contexts = NULL;
        return;
    }

    new_array = HeapReAlloc(GetProcessHeap(), 0, This->contexts, This->numContexts * sizeof(*This->contexts));
    if (!new_array)
    {
        ERR("Failed to shrink context array. Oh well.\n");
        return;
    }

    This->contexts = new_array;
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
    struct fbo_entry *entry, *entry2;

    TRACE("Destroying ctx %p\n", context);

    /* The correct GL context needs to be active to cleanup the GL resources below */
    if(pwglGetCurrentContext() != context->glCtx){
        pwglMakeCurrent(context->hdc, context->glCtx);
        last_device = NULL;
    }

    ENTER_GL();

    LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &context->fbo_list, struct fbo_entry, entry) {
        context_destroy_fbo_entry(This, entry);
    }
    if (context->src_fbo) {
        TRACE("Destroy src FBO %d\n", context->src_fbo);
        context_destroy_fbo(This, &context->src_fbo);
    }
    if (context->dst_fbo) {
        TRACE("Destroy dst FBO %d\n", context->dst_fbo);
        context_destroy_fbo(This, &context->dst_fbo);
    }

    LEAVE_GL();

    if (This->activeContext == context)
    {
        This->activeContext = NULL;
        TRACE("Destroying the active context.\n");
    }

    /* Cleanup the GL context */
    pwglMakeCurrent(NULL, NULL);
    if(context->isPBuffer) {
        GL_EXTCALL(wglReleasePbufferDCARB(context->pbuffer, context->hdc));
        GL_EXTCALL(wglDestroyPbufferARB(context->pbuffer));
    } else ReleaseDC(context->win_handle, context->hdc);
    pwglDeleteContext(context->glCtx);

    HeapFree(GetProcessHeap(), 0, context->vshader_const_dirty);
    HeapFree(GetProcessHeap(), 0, context->pshader_const_dirty);
    RemoveContextFromArray(This, context);
}

static inline void set_blit_dimension(UINT width, UINT height) {
    glMatrixMode(GL_PROJECTION);
    checkGLcall("glMatrixMode(GL_PROJECTION)");
    glLoadIdentity();
    checkGLcall("glLoadIdentity()");
    glOrtho(0, width, height, 0, 0.0, -1.0);
    checkGLcall("glOrtho");
    glViewport(0, 0, width, height);
    checkGLcall("glViewport");
}

/*****************************************************************************
 * SetupForBlit
 *
 * Sets up a context for DirectDraw blitting.
 * All texture units are disabled, texture unit 0 is set as current unit
 * fog, lighting, blending, alpha test, z test, scissor test, culling disabled
 * color writing enabled for all channels
 * register combiners disabled, shaders disabled
 * world matrix is set to identity, texture matrix 0 too
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
    int i, sampler;
    const struct StateEntry *StateTable = This->StateTable;

    TRACE("Setting up context %p for blitting\n", context);
    if(context->last_was_blit) {
        if(context->blit_w != width || context->blit_h != height) {
            set_blit_dimension(width, height);
            context->blit_w = width; context->blit_h = height;
            /* No need to dirtify here, the states are still dirtified because they weren't
             * applied since the last SetupForBlit call. Otherwise last_was_blit would not
             * be set
             */
        }
        TRACE("Context is already set up for blitting, nothing to do\n");
        return;
    }
    context->last_was_blit = TRUE;

    /* TODO: Use a display list */

    /* Disable shaders */
    This->shader_backend->shader_select((IWineD3DDevice *)This, FALSE, FALSE);
    Context_MarkStateDirty(context, STATE_VSHADER, StateTable);
    Context_MarkStateDirty(context, STATE_PIXELSHADER, StateTable);

    /* Call ENTER_GL() once for all gl calls below. In theory we should not call
     * helper functions in between gl calls. This function is full of Context_MarkStateDirty
     * which can safely be called from here, we only lock once instead locking/unlocking
     * after each GL call.
     */
    ENTER_GL();

    /* Disable all textures. The caller can then bind a texture it wants to blit
     * from
     *
     * The blitting code uses (for now) the fixed function pipeline, so make sure to reset all fixed
     * function texture unit. No need to care for higher samplers
     */
    for(i = GL_LIMITS(textures) - 1; i > 0 ; i--) {
        sampler = This->rev_tex_unit_map[i];
        GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + i));
        checkGLcall("glActiveTextureARB");

        if(GL_SUPPORT(ARB_TEXTURE_CUBE_MAP)) {
            glDisable(GL_TEXTURE_CUBE_MAP_ARB);
            checkGLcall("glDisable GL_TEXTURE_CUBE_MAP_ARB");
        }
        glDisable(GL_TEXTURE_3D);
        checkGLcall("glDisable GL_TEXTURE_3D");
        if(GL_SUPPORT(ARB_TEXTURE_RECTANGLE)) {
            glDisable(GL_TEXTURE_RECTANGLE_ARB);
            checkGLcall("glDisable GL_TEXTURE_RECTANGLE_ARB");
        }
        glDisable(GL_TEXTURE_2D);
        checkGLcall("glDisable GL_TEXTURE_2D");

        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        checkGLcall("glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);");

        if (sampler != -1) {
            if (sampler < MAX_TEXTURES) {
                Context_MarkStateDirty(context, STATE_TEXTURESTAGE(sampler, WINED3DTSS_COLOROP), StateTable);
            }
            Context_MarkStateDirty(context, STATE_SAMPLER(sampler), StateTable);
        }
    }
    GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB));
    checkGLcall("glActiveTextureARB");

    sampler = This->rev_tex_unit_map[0];

    if(GL_SUPPORT(ARB_TEXTURE_CUBE_MAP)) {
        glDisable(GL_TEXTURE_CUBE_MAP_ARB);
        checkGLcall("glDisable GL_TEXTURE_CUBE_MAP_ARB");
    }
    glDisable(GL_TEXTURE_3D);
    checkGLcall("glDisable GL_TEXTURE_3D");
    if(GL_SUPPORT(ARB_TEXTURE_RECTANGLE)) {
        glDisable(GL_TEXTURE_RECTANGLE_ARB);
        checkGLcall("glDisable GL_TEXTURE_RECTANGLE_ARB");
    }
    glDisable(GL_TEXTURE_2D);
    checkGLcall("glDisable GL_TEXTURE_2D");

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    glMatrixMode(GL_TEXTURE);
    checkGLcall("glMatrixMode(GL_TEXTURE)");
    glLoadIdentity();
    checkGLcall("glLoadIdentity()");

    if (GL_SUPPORT(EXT_TEXTURE_LOD_BIAS)) {
        glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT,
                  GL_TEXTURE_LOD_BIAS_EXT,
                  0.0);
        checkGLcall("glTexEnvi GL_TEXTURE_LOD_BIAS_EXT ...");
    }

    if (sampler != -1) {
        if (sampler < MAX_TEXTURES) {
            Context_MarkStateDirty(context, STATE_TRANSFORM(WINED3DTS_TEXTURE0 + sampler), StateTable);
            Context_MarkStateDirty(context, STATE_TEXTURESTAGE(sampler, WINED3DTSS_COLOROP), StateTable);
        }
        Context_MarkStateDirty(context, STATE_SAMPLER(sampler), StateTable);
    }

    /* Other misc states */
    glDisable(GL_ALPHA_TEST);
    checkGLcall("glDisable(GL_ALPHA_TEST)");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_ALPHATESTENABLE), StateTable);
    glDisable(GL_LIGHTING);
    checkGLcall("glDisable GL_LIGHTING");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_LIGHTING), StateTable);
    glDisable(GL_DEPTH_TEST);
    checkGLcall("glDisable GL_DEPTH_TEST");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_ZENABLE), StateTable);
    glDisableWINE(GL_FOG);
    checkGLcall("glDisable GL_FOG");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_FOGENABLE), StateTable);
    glDisable(GL_BLEND);
    checkGLcall("glDisable GL_BLEND");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_ALPHABLENDENABLE), StateTable);
    glDisable(GL_CULL_FACE);
    checkGLcall("glDisable GL_CULL_FACE");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_CULLMODE), StateTable);
    glDisable(GL_STENCIL_TEST);
    checkGLcall("glDisable GL_STENCIL_TEST");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_STENCILENABLE), StateTable);
    glDisable(GL_SCISSOR_TEST);
    checkGLcall("glDisable GL_SCISSOR_TEST");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_SCISSORTESTENABLE), StateTable);
    if(GL_SUPPORT(ARB_POINT_SPRITE)) {
        glDisable(GL_POINT_SPRITE_ARB);
        checkGLcall("glDisable GL_POINT_SPRITE_ARB");
        Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_POINTSPRITEENABLE), StateTable);
    }
    glColorMask(GL_TRUE, GL_TRUE,GL_TRUE,GL_TRUE);
    checkGLcall("glColorMask");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_CLIPPING), StateTable);
    if (GL_SUPPORT(EXT_SECONDARY_COLOR)) {
        glDisable(GL_COLOR_SUM_EXT);
        Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_SPECULARENABLE), StateTable);
        checkGLcall("glDisable(GL_COLOR_SUM_EXT)");
    }

    /* Setup transforms */
    glMatrixMode(GL_MODELVIEW);
    checkGLcall("glMatrixMode(GL_MODELVIEW)");
    glLoadIdentity();
    checkGLcall("glLoadIdentity()");
    Context_MarkStateDirty(context, STATE_TRANSFORM(WINED3DTS_WORLDMATRIX(0)), StateTable);

    context->last_was_rhw = TRUE;
    Context_MarkStateDirty(context, STATE_VDECL, StateTable); /* because of last_was_rhw = TRUE */

    glDisable(GL_CLIP_PLANE0); checkGLcall("glDisable(clip plane 0)");
    glDisable(GL_CLIP_PLANE1); checkGLcall("glDisable(clip plane 1)");
    glDisable(GL_CLIP_PLANE2); checkGLcall("glDisable(clip plane 2)");
    glDisable(GL_CLIP_PLANE3); checkGLcall("glDisable(clip plane 3)");
    glDisable(GL_CLIP_PLANE4); checkGLcall("glDisable(clip plane 4)");
    glDisable(GL_CLIP_PLANE5); checkGLcall("glDisable(clip plane 5)");
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_CLIPPING), StateTable);
    LEAVE_GL();

    set_blit_dimension(width, height);
    context->blit_w = width; context->blit_h = height;
    Context_MarkStateDirty(context, STATE_VIEWPORT, StateTable);
    Context_MarkStateDirty(context, STATE_TRANSFORM(WINED3DTS_PROJECTION), StateTable);


    This->frag_pipe->enable_extension((IWineD3DDevice *) This, FALSE);
}

/*****************************************************************************
 * findThreadContextForSwapChain
 *
 * Searches a swapchain for all contexts and picks one for the thread tid.
 * If none can be found the swapchain is requested to create a new context
 *
 *****************************************************************************/
static WineD3DContext *findThreadContextForSwapChain(IWineD3DSwapChain *swapchain, DWORD tid) {
    unsigned int i;

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
    BOOL readTexture = wined3d_settings.offscreen_rendering_mode != ORM_FBO && This->render_offscreen;
    WineD3DContext *context = This->activeContext;
    BOOL oldRenderOffscreen = This->render_offscreen;
    const struct GlPixelFormatDesc *old = ((IWineD3DSurfaceImpl *)This->lastActiveRenderTarget)->resource.format_desc;
    const struct GlPixelFormatDesc *new = ((IWineD3DSurfaceImpl *)target)->resource.format_desc;
    const struct StateEntry *StateTable = This->StateTable;

    /* To compensate the lack of format switching with some offscreen rendering methods and on onscreen buffers
     * the alpha blend state changes with different render target formats
     */
    if (old->format != new->format)
    {
        /* Disable blending when the alpha mask has changed and when a format doesn't support blending */
        if ((old->alpha_mask && !new->alpha_mask) || (!old->alpha_mask && new->alpha_mask)
                || !(new->Flags & WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING))
        {
            Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_ALPHABLENDENABLE), StateTable);
        }
    }

    if (SUCCEEDED(IWineD3DSurface_GetContainer(target, &IID_IWineD3DSwapChain, (void **)&swapchain))) {
        TRACE("Rendering onscreen\n");

        context = findThreadContextForSwapChain(swapchain, tid);

        This->render_offscreen = FALSE;
        /* The context != This->activeContext will catch a NOP context change. This can occur
         * if we are switching back to swapchain rendering in case of FBO or Back Buffer offscreen
         * rendering. No context change is needed in that case
         */

        if(wined3d_settings.offscreen_rendering_mode == ORM_PBUFFER) {
            if(This->pbufferContext && tid == This->pbufferContext->tid) {
                This->pbufferContext->tid = 0;
            }
        }
        IWineD3DSwapChain_Release(swapchain);

        if(oldRenderOffscreen) {
            Context_MarkStateDirty(context, WINED3DTS_PROJECTION, StateTable);
            Context_MarkStateDirty(context, STATE_VDECL, StateTable);
            Context_MarkStateDirty(context, STATE_VIEWPORT, StateTable);
            Context_MarkStateDirty(context, STATE_SCISSORRECT, StateTable);
            Context_MarkStateDirty(context, STATE_FRONTFACE, StateTable);
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
                    /* This may happen if the app jumps straight into offscreen rendering
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
                    /* This may happen if the app jumps straight into offscreen rendering
                     * Start using the context of the primary swapchain. tid == 0 is no problem
                     * for findThreadContextForSwapChain.
                     *
                     * Can also happen on thread switches - in that case findThreadContextForSwapChain
                     * is perfect to call.
                     */
                    context = findThreadContextForSwapChain(This->swapchains[0], tid);
                }
                break;
        }

        if(!oldRenderOffscreen) {
            Context_MarkStateDirty(context, WINED3DTS_PROJECTION, StateTable);
            Context_MarkStateDirty(context, STATE_VDECL, StateTable);
            Context_MarkStateDirty(context, STATE_VIEWPORT, StateTable);
            Context_MarkStateDirty(context, STATE_SCISSORRECT, StateTable);
            Context_MarkStateDirty(context, STATE_FRONTFACE, StateTable);
        }
    }

    /* When switching away from an offscreen render target, and we're not using FBOs,
     * we have to read the drawable into the texture. This is done via PreLoad(and
     * SFLAG_INDRAWABLE set on the surface). There are some things that need care though.
     * PreLoad needs a GL context, and FindContext is called before the context is activated.
     * It also has to be called with the old rendertarget active, otherwise a wrong drawable
     * is read. This leads to these possible situations:
     *
     * 0) lastActiveRenderTarget == target && oldTid == newTid:
     *    Nothing to do, we don't even reach this code in this case...
     *
     * 1) lastActiveRenderTarget != target && oldTid == newTid:
     *    The currently active context is OK for readback. Call PreLoad, and it
     *    performs the read
     *
     * 2) lastActiveRenderTarget == target && oldTid != newTid:
     *    Nothing to do - the drawable is unchanged
     *
     * 3) lastActiveRenderTarget != target && oldTid != newTid:
     *    This is tricky. We have to get a context with the old drawable from somewhere
     *    before we can switch to the new context. In this case, PreLoad calls
     *    ActivateContext(lastActiveRenderTarget) from the new(current) thread. This
     *    is case (2) then. The old drawable is activated for the new thread, and the
     *    readback can be done. The recursed ActivateContext does *not* call PreLoad again.
     *    After that, the outer ActivateContext(which calls PreLoad) can activate the new
     *    target for the new thread
     */
    if (readTexture && This->lastActiveRenderTarget != target) {
        BOOL oldInDraw = This->isInDraw;

        /* PreLoad requires a context to load the texture, thus it will call ActivateContext.
         * Set the isInDraw to true to signal PreLoad that it has a context. Will be tricky
         * when using offscreen rendering with multithreading
         */
        This->isInDraw = TRUE;

        /* Do that before switching the context:
         * Read the back buffer of the old drawable into the destination texture
         */
        if(((IWineD3DSurfaceImpl *)This->lastActiveRenderTarget)->glDescription.srgbTextureName) {
            surface_internal_preload(This->lastActiveRenderTarget, SRGB_BOTH);
        } else {
            surface_internal_preload(This->lastActiveRenderTarget, SRGB_RGB);
        }

        /* Assume that the drawable will be modified by some other things now */
        IWineD3DSurface_ModifyLocation(This->lastActiveRenderTarget, SFLAG_INDRAWABLE, FALSE);

        This->isInDraw = oldInDraw;
    }

    return context;
}

static void apply_draw_buffer(IWineD3DDeviceImpl *This, IWineD3DSurface *target, BOOL blit)
{
    IWineD3DSwapChain *swapchain;

    if (SUCCEEDED(IWineD3DSurface_GetContainer(target, &IID_IWineD3DSwapChain, (void **)&swapchain)))
    {
        IWineD3DSwapChain_Release((IUnknown *)swapchain);
        ENTER_GL();
        glDrawBuffer(surface_get_gl_buffer(target, swapchain));
        checkGLcall("glDrawBuffers()");
        LEAVE_GL();
    }
    else
    {
        ENTER_GL();
        if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
        {
            if (!blit)
            {
                if (GL_SUPPORT(ARB_DRAW_BUFFERS))
                {
                    GL_EXTCALL(glDrawBuffersARB(GL_LIMITS(buffers), This->draw_buffers));
                    checkGLcall("glDrawBuffers()");
                }
                else
                {
                    glDrawBuffer(This->draw_buffers[0]);
                    checkGLcall("glDrawBuffer()");
                }
            } else {
                glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
                checkGLcall("glDrawBuffer()");
            }
        }
        else
        {
            glDrawBuffer(This->offscreenBuffer);
            checkGLcall("glDrawBuffer()");
        }
        LEAVE_GL();
    }
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
    DWORD                         i, dirtyState, idx;
    BYTE                          shift;
    WineD3DContext                *context;
    const struct StateEntry       *StateTable = This->StateTable;

    TRACE("(%p): Selecting context for render target %p, thread %d\n", This, target, tid);
    if(This->lastActiveRenderTarget != target || tid != This->lastThread) {
        context = FindContext(This, target, tid);
        context->draw_buffer_dirty = TRUE;
        This->lastActiveRenderTarget = target;
        This->lastThread = tid;
    } else {
        /* Stick to the old context */
        context = This->activeContext;
    }

    /* Activate the opengl context */
    if(last_device != This || context != This->activeContext) {
        BOOL ret;

        /* Prevent an unneeded context switch as those are expensive */
        if(context->glCtx && (context->glCtx == pwglGetCurrentContext())) {
            TRACE("Already using gl context %p\n", context->glCtx);
        }
        else {
            TRACE("Switching gl ctx to %p, hdc=%p ctx=%p\n", context, context->hdc, context->glCtx);

            ret = pwglMakeCurrent(context->hdc, context->glCtx);
            if(ret == FALSE) {
                ERR("Failed to activate the new context\n");
            } else if(!context->last_was_blit) {
                This->frag_pipe->enable_extension((IWineD3DDevice *) This, TRUE);
            } else {
                This->frag_pipe->enable_extension((IWineD3DDevice *) This, FALSE);
            }
        }
        if(This->activeContext->vshader_const_dirty) {
            memset(This->activeContext->vshader_const_dirty, 1,
                   sizeof(*This->activeContext->vshader_const_dirty) * GL_LIMITS(vshader_constantsF));
        }
        if(This->activeContext->pshader_const_dirty) {
            memset(This->activeContext->pshader_const_dirty, 1,
                   sizeof(*This->activeContext->pshader_const_dirty) * GL_LIMITS(pshader_constantsF));
        }
        This->activeContext = context;
        last_device = This;
    }

    switch (usage) {
        case CTXUSAGE_CLEAR:
        case CTXUSAGE_DRAWPRIM:
            if (wined3d_settings.offscreen_rendering_mode == ORM_FBO) {
                context_apply_fbo_state((IWineD3DDevice *)This);
            }
            if (context->draw_buffer_dirty) {
                apply_draw_buffer(This, target, FALSE);
                context->draw_buffer_dirty = FALSE;
            }
            break;

        case CTXUSAGE_BLIT:
            if (wined3d_settings.offscreen_rendering_mode == ORM_FBO) {
                if (This->render_offscreen) {
                    FIXME("Activating for CTXUSAGE_BLIT for an offscreen target with ORM_FBO. This should be avoided.\n");
                    context_bind_fbo((IWineD3DDevice *)This, GL_FRAMEBUFFER_EXT, &context->dst_fbo);
                    context_attach_surface_fbo(This, GL_FRAMEBUFFER_EXT, 0, target);

                    ENTER_GL();
                    GL_EXTCALL(glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, 0));
                    checkGLcall("glFramebufferRenderbufferEXT");
                    LEAVE_GL();
                } else {
                    ENTER_GL();
                    GL_EXTCALL(glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0));
                    checkGLcall("glFramebufferRenderbufferEXT");
                    LEAVE_GL();
                }
                context->draw_buffer_dirty = TRUE;
            }
            if (context->draw_buffer_dirty) {
                apply_draw_buffer(This, target, TRUE);
                if (wined3d_settings.offscreen_rendering_mode != ORM_FBO) {
                    context->draw_buffer_dirty = FALSE;
                }
            }
            break;

        default:
            break;
    }

    switch(usage) {
        case CTXUSAGE_RESOURCELOAD:
            /* This does not require any special states to be set up */
            break;

        case CTXUSAGE_CLEAR:
            if(context->last_was_blit) {
                This->frag_pipe->enable_extension((IWineD3DDevice *) This, TRUE);
            }

            /* Blending and clearing should be orthogonal, but tests on the nvidia driver show that disabling
             * blending when clearing improves the clearing performance incredibly.
             */
            ENTER_GL();
            glDisable(GL_BLEND);
            LEAVE_GL();
            Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_ALPHABLENDENABLE), StateTable);

            ENTER_GL();
            glEnable(GL_SCISSOR_TEST);
            checkGLcall("glEnable GL_SCISSOR_TEST");
            LEAVE_GL();
            context->last_was_blit = FALSE;
            Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_SCISSORTESTENABLE), StateTable);
            Context_MarkStateDirty(context, STATE_SCISSORRECT, StateTable);
            break;

        case CTXUSAGE_DRAWPRIM:
            /* This needs all dirty states applied */
            if(context->last_was_blit) {
                This->frag_pipe->enable_extension((IWineD3DDevice *) This, TRUE);
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
}

WineD3DContext *getActiveContext(void) {
    return last_device->activeContext;
}
