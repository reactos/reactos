/*
 * Context and render target management in wined3d
 *
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
 * Copyright 2009 Henri Verbeet for CodeWeavers
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

#define GLINFO_LOCATION (*gl_info)

static DWORD wined3d_context_tls_idx;

/* FBO helper functions */

/* GL locking is done by the caller */
void context_bind_fbo(struct wined3d_context *context, GLenum target, GLuint *fbo)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    GLuint f;

    if (!fbo)
    {
        f = 0;
    }
    else
    {
        if (!*fbo)
        {
            gl_info->fbo_ops.glGenFramebuffers(1, fbo);
            checkGLcall("glGenFramebuffers()");
            TRACE("Created FBO %u.\n", *fbo);
        }
        f = *fbo;
    }

    switch (target)
    {
        case GL_READ_FRAMEBUFFER:
            if (context->fbo_read_binding == f) return;
            context->fbo_read_binding = f;
            break;

        case GL_DRAW_FRAMEBUFFER:
            if (context->fbo_draw_binding == f) return;
            context->fbo_draw_binding = f;
            break;

        case GL_FRAMEBUFFER:
            if (context->fbo_read_binding == f
                    && context->fbo_draw_binding == f) return;
            context->fbo_read_binding = f;
            context->fbo_draw_binding = f;
            break;

        default:
            FIXME("Unhandled target %#x.\n", target);
            break;
    }

    gl_info->fbo_ops.glBindFramebuffer(target, f);
    checkGLcall("glBindFramebuffer()");
}

/* GL locking is done by the caller */
static void context_clean_fbo_attachments(const struct wined3d_gl_info *gl_info)
{
    unsigned int i;

    for (i = 0; i < GL_LIMITS(buffers); ++i)
    {
        gl_info->fbo_ops.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, 0, 0);
        checkGLcall("glFramebufferTexture2D()");
    }
    gl_info->fbo_ops.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
    checkGLcall("glFramebufferTexture2D()");

    gl_info->fbo_ops.glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
    checkGLcall("glFramebufferTexture2D()");
}

/* GL locking is done by the caller */
static void context_destroy_fbo(struct wined3d_context *context, GLuint *fbo)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    context_bind_fbo(context, GL_FRAMEBUFFER, fbo);
    context_clean_fbo_attachments(gl_info);
    context_bind_fbo(context, GL_FRAMEBUFFER, NULL);

    gl_info->fbo_ops.glDeleteFramebuffers(1, fbo);
    checkGLcall("glDeleteFramebuffers()");
}

/* GL locking is done by the caller */
static void context_apply_attachment_filter_states(IWineD3DSurface *surface, BOOL force_preload)
{
    const IWineD3DSurfaceImpl *surface_impl = (IWineD3DSurfaceImpl *)surface;
    IWineD3DDeviceImpl *device = surface_impl->resource.wineD3DDevice;
    IWineD3DBaseTextureImpl *texture_impl;
    BOOL update_minfilter = FALSE;
    BOOL update_magfilter = FALSE;

    /* Update base texture states array */
    if (SUCCEEDED(IWineD3DSurface_GetContainer(surface, &IID_IWineD3DBaseTexture, (void **)&texture_impl)))
    {
        if (texture_impl->baseTexture.texture_rgb.states[WINED3DTEXSTA_MINFILTER] != WINED3DTEXF_POINT
            || texture_impl->baseTexture.texture_rgb.states[WINED3DTEXSTA_MIPFILTER] != WINED3DTEXF_NONE)
        {
            texture_impl->baseTexture.texture_rgb.states[WINED3DTEXSTA_MINFILTER] = WINED3DTEXF_POINT;
            texture_impl->baseTexture.texture_rgb.states[WINED3DTEXSTA_MIPFILTER] = WINED3DTEXF_NONE;
            update_minfilter = TRUE;
        }

        if (texture_impl->baseTexture.texture_rgb.states[WINED3DTEXSTA_MAGFILTER] != WINED3DTEXF_POINT)
        {
            texture_impl->baseTexture.texture_rgb.states[WINED3DTEXSTA_MAGFILTER] = WINED3DTEXF_POINT;
            update_magfilter = TRUE;
        }

        if (texture_impl->baseTexture.bindCount)
        {
            WARN("Render targets should not be bound to a sampler\n");
            IWineD3DDeviceImpl_MarkStateDirty(device, STATE_SAMPLER(texture_impl->baseTexture.sampler));
        }

        IWineD3DBaseTexture_Release((IWineD3DBaseTexture *)texture_impl);
    }

    if (update_minfilter || update_magfilter || force_preload)
    {
        GLenum target, bind_target;
        GLint old_binding;

        target = surface_impl->texture_target;
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

        glBindTexture(bind_target, surface_impl->texture_name);
        if (update_minfilter) glTexParameteri(bind_target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        if (update_magfilter) glTexParameteri(bind_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glBindTexture(bind_target, old_binding);
    }

    checkGLcall("apply_attachment_filter_states()");
}

/* GL locking is done by the caller */
void context_attach_depth_stencil_fbo(struct wined3d_context *context,
        GLenum fbo_target, IWineD3DSurface *depth_stencil, BOOL use_render_buffer)
{
    IWineD3DSurfaceImpl *depth_stencil_impl = (IWineD3DSurfaceImpl *)depth_stencil;
    const struct wined3d_gl_info *gl_info = context->gl_info;

    TRACE("Attach depth stencil %p\n", depth_stencil);

    if (depth_stencil)
    {
        DWORD format_flags = depth_stencil_impl->resource.format_desc->Flags;

        if (use_render_buffer && depth_stencil_impl->current_renderbuffer)
        {
            if (format_flags & WINED3DFMT_FLAG_DEPTH)
            {
                gl_info->fbo_ops.glFramebufferRenderbuffer(fbo_target, GL_DEPTH_ATTACHMENT,
                        GL_RENDERBUFFER, depth_stencil_impl->current_renderbuffer->id);
                checkGLcall("glFramebufferRenderbuffer()");
            }

            if (format_flags & WINED3DFMT_FLAG_STENCIL)
            {
                gl_info->fbo_ops.glFramebufferRenderbuffer(fbo_target, GL_STENCIL_ATTACHMENT,
                        GL_RENDERBUFFER, depth_stencil_impl->current_renderbuffer->id);
                checkGLcall("glFramebufferRenderbuffer()");
            }
        }
        else
        {
            context_apply_attachment_filter_states(depth_stencil, TRUE);

            if (format_flags & WINED3DFMT_FLAG_DEPTH)
            {
                gl_info->fbo_ops.glFramebufferTexture2D(fbo_target, GL_DEPTH_ATTACHMENT,
                        depth_stencil_impl->texture_target, depth_stencil_impl->texture_name,
                        depth_stencil_impl->texture_level);
                checkGLcall("glFramebufferTexture2D()");
            }

            if (format_flags & WINED3DFMT_FLAG_STENCIL)
            {
                gl_info->fbo_ops.glFramebufferTexture2D(fbo_target, GL_STENCIL_ATTACHMENT,
                        depth_stencil_impl->texture_target, depth_stencil_impl->texture_name,
                        depth_stencil_impl->texture_level);
                checkGLcall("glFramebufferTexture2D()");
            }
        }

        if (!(format_flags & WINED3DFMT_FLAG_DEPTH))
        {
            gl_info->fbo_ops.glFramebufferTexture2D(fbo_target, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
            checkGLcall("glFramebufferTexture2D()");
        }

        if (!(format_flags & WINED3DFMT_FLAG_STENCIL))
        {
            gl_info->fbo_ops.glFramebufferTexture2D(fbo_target, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
            checkGLcall("glFramebufferTexture2D()");
        }
    }
    else
    {
        gl_info->fbo_ops.glFramebufferTexture2D(fbo_target, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
        checkGLcall("glFramebufferTexture2D()");

        gl_info->fbo_ops.glFramebufferTexture2D(fbo_target, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
        checkGLcall("glFramebufferTexture2D()");
    }
}

/* GL locking is done by the caller */
void context_attach_surface_fbo(const struct wined3d_context *context,
        GLenum fbo_target, DWORD idx, IWineD3DSurface *surface)
{
    const IWineD3DSurfaceImpl *surface_impl = (IWineD3DSurfaceImpl *)surface;
    const struct wined3d_gl_info *gl_info = context->gl_info;

    TRACE("Attach surface %p to %u\n", surface, idx);

    if (surface)
    {
        context_apply_attachment_filter_states(surface, TRUE);

        gl_info->fbo_ops.glFramebufferTexture2D(fbo_target, GL_COLOR_ATTACHMENT0 + idx, surface_impl->texture_target,
                surface_impl->texture_name, surface_impl->texture_level);
        checkGLcall("glFramebufferTexture2D()");
    }
    else
    {
        gl_info->fbo_ops.glFramebufferTexture2D(fbo_target, GL_COLOR_ATTACHMENT0 + idx, GL_TEXTURE_2D, 0, 0);
        checkGLcall("glFramebufferTexture2D()");
    }
}

/* GL locking is done by the caller */
static void context_check_fbo_status(struct wined3d_context *context)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    GLenum status;

    status = gl_info->fbo_ops.glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status == GL_FRAMEBUFFER_COMPLETE)
    {
        TRACE("FBO complete\n");
    } else {
        IWineD3DSurfaceImpl *attachment;
        unsigned int i;
        FIXME("FBO status %s (%#x)\n", debug_fbostatus(status), status);

        /* Dump the FBO attachments */
        for (i = 0; i < GL_LIMITS(buffers); ++i)
        {
            attachment = (IWineD3DSurfaceImpl *)context->current_fbo->render_targets[i];
            if (attachment)
            {
                FIXME("\tColor attachment %d: (%p) %s %ux%u\n",
                        i, attachment, debug_d3dformat(attachment->resource.format_desc->format),
                        attachment->pow2Width, attachment->pow2Height);
            }
        }
        attachment = (IWineD3DSurfaceImpl *)context->current_fbo->depth_stencil;
        if (attachment)
        {
            FIXME("\tDepth attachment: (%p) %s %ux%u\n",
                    attachment, debug_d3dformat(attachment->resource.format_desc->format),
                    attachment->pow2Width, attachment->pow2Height);
        }
    }
}

static struct fbo_entry *context_create_fbo_entry(struct wined3d_context *context)
{
    IWineD3DDeviceImpl *device = ((IWineD3DSurfaceImpl *)context->surface)->resource.wineD3DDevice;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct fbo_entry *entry;

    entry = HeapAlloc(GetProcessHeap(), 0, sizeof(*entry));
    entry->render_targets = HeapAlloc(GetProcessHeap(), 0, GL_LIMITS(buffers) * sizeof(*entry->render_targets));
    memcpy(entry->render_targets, device->render_targets, GL_LIMITS(buffers) * sizeof(*entry->render_targets));
    entry->depth_stencil = device->stencilBufferTarget;
    entry->attached = FALSE;
    entry->id = 0;

    return entry;
}

/* GL locking is done by the caller */
static void context_reuse_fbo_entry(struct wined3d_context *context, struct fbo_entry *entry)
{
    IWineD3DDeviceImpl *device = ((IWineD3DSurfaceImpl *)context->surface)->resource.wineD3DDevice;
    const struct wined3d_gl_info *gl_info = context->gl_info;

    context_bind_fbo(context, GL_FRAMEBUFFER, &entry->id);
    context_clean_fbo_attachments(gl_info);

    memcpy(entry->render_targets, device->render_targets, GL_LIMITS(buffers) * sizeof(*entry->render_targets));
    entry->depth_stencil = device->stencilBufferTarget;
    entry->attached = FALSE;
}

/* GL locking is done by the caller */
static void context_destroy_fbo_entry(struct wined3d_context *context, struct fbo_entry *entry)
{
    if (entry->id)
    {
        TRACE("Destroy FBO %d\n", entry->id);
        context_destroy_fbo(context, &entry->id);
    }
    --context->fbo_entry_count;
    list_remove(&entry->entry);
    HeapFree(GetProcessHeap(), 0, entry->render_targets);
    HeapFree(GetProcessHeap(), 0, entry);
}


/* GL locking is done by the caller */
static struct fbo_entry *context_find_fbo_entry(struct wined3d_context *context)
{
    IWineD3DDeviceImpl *device = ((IWineD3DSurfaceImpl *)context->surface)->resource.wineD3DDevice;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct fbo_entry *entry;

    LIST_FOR_EACH_ENTRY(entry, &context->fbo_list, struct fbo_entry, entry)
    {
        if (!memcmp(entry->render_targets, device->render_targets, GL_LIMITS(buffers) * sizeof(*entry->render_targets))
                && entry->depth_stencil == device->stencilBufferTarget)
        {
            list_remove(&entry->entry);
            list_add_head(&context->fbo_list, &entry->entry);
            return entry;
        }
    }

    if (context->fbo_entry_count < WINED3D_MAX_FBO_ENTRIES)
    {
        entry = context_create_fbo_entry(context);
        list_add_head(&context->fbo_list, &entry->entry);
        ++context->fbo_entry_count;
    }
    else
    {
        entry = LIST_ENTRY(list_tail(&context->fbo_list), struct fbo_entry, entry);
        context_reuse_fbo_entry(context, entry);
        list_remove(&entry->entry);
        list_add_head(&context->fbo_list, &entry->entry);
    }

    return entry;
}

/* GL locking is done by the caller */
static void context_apply_fbo_entry(struct wined3d_context *context, struct fbo_entry *entry)
{
    IWineD3DDeviceImpl *device = ((IWineD3DSurfaceImpl *)context->surface)->resource.wineD3DDevice;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    unsigned int i;

    context_bind_fbo(context, GL_FRAMEBUFFER, &entry->id);

    if (!entry->attached)
    {
        /* Apply render targets */
        for (i = 0; i < GL_LIMITS(buffers); ++i)
        {
            IWineD3DSurface *render_target = device->render_targets[i];
            context_attach_surface_fbo(context, GL_FRAMEBUFFER, i, render_target);
        }

        /* Apply depth targets */
        if (device->stencilBufferTarget)
        {
            unsigned int w = ((IWineD3DSurfaceImpl *)device->render_targets[0])->pow2Width;
            unsigned int h = ((IWineD3DSurfaceImpl *)device->render_targets[0])->pow2Height;

            surface_set_compatible_renderbuffer(device->stencilBufferTarget, w, h);
        }
        context_attach_depth_stencil_fbo(context, GL_FRAMEBUFFER, device->stencilBufferTarget, TRUE);

        entry->attached = TRUE;
    } else {
        for (i = 0; i < GL_LIMITS(buffers); ++i)
        {
            if (device->render_targets[i])
                context_apply_attachment_filter_states(device->render_targets[i], FALSE);
        }
        if (device->stencilBufferTarget)
            context_apply_attachment_filter_states(device->stencilBufferTarget, FALSE);
    }

    for (i = 0; i < GL_LIMITS(buffers); ++i)
    {
        if (device->render_targets[i])
            device->draw_buffers[i] = GL_COLOR_ATTACHMENT0 + i;
        else
            device->draw_buffers[i] = GL_NONE;
    }
}

/* GL locking is done by the caller */
static void context_apply_fbo_state(struct wined3d_context *context)
{
    if (context->render_offscreen)
    {
        context->current_fbo = context_find_fbo_entry(context);
        context_apply_fbo_entry(context, context->current_fbo);
    } else {
        context->current_fbo = NULL;
        context_bind_fbo(context, GL_FRAMEBUFFER, NULL);
    }

    context_check_fbo_status(context);
}

/* Context activation is done by the caller. */
void context_alloc_occlusion_query(struct wined3d_context *context, struct wined3d_occlusion_query *query)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    if (context->free_occlusion_query_count)
    {
        query->id = context->free_occlusion_queries[--context->free_occlusion_query_count];
    }
    else
    {
        if (GL_SUPPORT(ARB_OCCLUSION_QUERY))
        {
            ENTER_GL();
            GL_EXTCALL(glGenQueriesARB(1, &query->id));
            checkGLcall("glGenQueriesARB");
            LEAVE_GL();

            TRACE("Allocated occlusion query %u in context %p.\n", query->id, context);
        }
        else
        {
            WARN("Occlusion queries not supported, not allocating query id.\n");
            query->id = 0;
        }
    }

    query->context = context;
    list_add_head(&context->occlusion_queries, &query->entry);
}

void context_free_occlusion_query(struct wined3d_occlusion_query *query)
{
    struct wined3d_context *context = query->context;

    list_remove(&query->entry);
    query->context = NULL;

    if (context->free_occlusion_query_count >= context->free_occlusion_query_size - 1)
    {
        UINT new_size = context->free_occlusion_query_size << 1;
        GLuint *new_data = HeapReAlloc(GetProcessHeap(), 0, context->free_occlusion_queries,
                new_size * sizeof(*context->free_occlusion_queries));

        if (!new_data)
        {
            ERR("Failed to grow free list, leaking query %u in context %p.\n", query->id, context);
            return;
        }

        context->free_occlusion_query_size = new_size;
        context->free_occlusion_queries = new_data;
    }

    context->free_occlusion_queries[context->free_occlusion_query_count++] = query->id;
}

/* Context activation is done by the caller. */
void context_alloc_event_query(struct wined3d_context *context, struct wined3d_event_query *query)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    if (context->free_event_query_count)
    {
        query->id = context->free_event_queries[--context->free_event_query_count];
    }
    else
    {
        if (GL_SUPPORT(APPLE_FENCE))
        {
            ENTER_GL();
            GL_EXTCALL(glGenFencesAPPLE(1, &query->id));
            checkGLcall("glGenFencesAPPLE");
            LEAVE_GL();

            TRACE("Allocated event query %u in context %p.\n", query->id, context);
        }
        else if(GL_SUPPORT(NV_FENCE))
        {
            ENTER_GL();
            GL_EXTCALL(glGenFencesNV(1, &query->id));
            checkGLcall("glGenFencesNV");
            LEAVE_GL();

            TRACE("Allocated event query %u in context %p.\n", query->id, context);
        }
        else
        {
            WARN("Event queries not supported, not allocating query id.\n");
            query->id = 0;
        }
    }

    query->context = context;
    list_add_head(&context->event_queries, &query->entry);
}

void context_free_event_query(struct wined3d_event_query *query)
{
    struct wined3d_context *context = query->context;

    list_remove(&query->entry);
    query->context = NULL;

    if (context->free_event_query_count >= context->free_event_query_size - 1)
    {
        UINT new_size = context->free_event_query_size << 1;
        GLuint *new_data = HeapReAlloc(GetProcessHeap(), 0, context->free_event_queries,
                new_size * sizeof(*context->free_event_queries));

        if (!new_data)
        {
            ERR("Failed to grow free list, leaking query %u in context %p.\n", query->id, context);
            return;
        }

        context->free_event_query_size = new_size;
        context->free_event_queries = new_data;
    }

    context->free_event_queries[context->free_event_query_count++] = query->id;
}

void context_resource_released(IWineD3DDevice *iface, IWineD3DResource *resource, WINED3DRESOURCETYPE type)
{
    IWineD3DDeviceImpl *This = (IWineD3DDeviceImpl *)iface;
    UINT i;

    if (!This->d3d_initialized) return;

    switch(type)
    {
        case WINED3DRTYPE_SURFACE:
        {
            ActivateContext(This, NULL, CTXUSAGE_RESOURCELOAD);

            for (i = 0; i < This->numContexts; ++i)
            {
                struct wined3d_context *context = This->contexts[i];
                const struct wined3d_gl_info *gl_info = context->gl_info;
                struct fbo_entry *entry, *entry2;

                if (context->current_rt == (IWineD3DSurface *)resource) context->current_rt = NULL;

                ENTER_GL();

                LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &context->fbo_list, struct fbo_entry, entry)
                {
                    BOOL destroyed = FALSE;
                    UINT j;

                    for (j = 0; !destroyed && j < GL_LIMITS(buffers); ++j)
                    {
                        if (entry->render_targets[j] == (IWineD3DSurface *)resource)
                        {
                            context_destroy_fbo_entry(context, entry);
                            destroyed = TRUE;
                        }
                    }

                    if (!destroyed && entry->depth_stencil == (IWineD3DSurface *)resource)
                        context_destroy_fbo_entry(context, entry);
                }

                LEAVE_GL();
            }

            break;
        }

        default:
            break;
    }
}

static void context_destroy_gl_resources(struct wined3d_context *context)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct wined3d_occlusion_query *occlusion_query;
    struct wined3d_event_query *event_query;
    struct fbo_entry *entry, *entry2;
    BOOL has_glctx;

    has_glctx = pwglMakeCurrent(context->hdc, context->glCtx);
    if (!has_glctx) WARN("Failed to activate context. Window already destroyed?\n");

    ENTER_GL();

    LIST_FOR_EACH_ENTRY(occlusion_query, &context->occlusion_queries, struct wined3d_occlusion_query, entry)
    {
        if (has_glctx && GL_SUPPORT(ARB_OCCLUSION_QUERY)) GL_EXTCALL(glDeleteQueriesARB(1, &occlusion_query->id));
        occlusion_query->context = NULL;
    }

    LIST_FOR_EACH_ENTRY(event_query, &context->event_queries, struct wined3d_event_query, entry)
    {
        if (has_glctx)
        {
            if (GL_SUPPORT(APPLE_FENCE)) GL_EXTCALL(glDeleteFencesAPPLE(1, &event_query->id));
            else if (GL_SUPPORT(NV_FENCE)) GL_EXTCALL(glDeleteFencesNV(1, &event_query->id));
        }
        event_query->context = NULL;
    }

    LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &context->fbo_list, struct fbo_entry, entry) {
        if (!has_glctx) entry->id = 0;
        context_destroy_fbo_entry(context, entry);
    }
    if (has_glctx)
    {
        if (context->src_fbo)
        {
            TRACE("Destroy src FBO %d\n", context->src_fbo);
            context_destroy_fbo(context, &context->src_fbo);
        }
        if (context->dst_fbo)
        {
            TRACE("Destroy dst FBO %d\n", context->dst_fbo);
            context_destroy_fbo(context, &context->dst_fbo);
        }
        if (context->dummy_arbfp_prog)
        {
            GL_EXTCALL(glDeleteProgramsARB(1, &context->dummy_arbfp_prog));
        }

        if (GL_SUPPORT(ARB_OCCLUSION_QUERY))
            GL_EXTCALL(glDeleteQueriesARB(context->free_occlusion_query_count, context->free_occlusion_queries));

        if (GL_SUPPORT(APPLE_FENCE))
            GL_EXTCALL(glDeleteFencesAPPLE(context->free_event_query_count, context->free_event_queries));
        else if (GL_SUPPORT(NV_FENCE))
            GL_EXTCALL(glDeleteFencesNV(context->free_event_query_count, context->free_event_queries));

        checkGLcall("context cleanup");
    }

    LEAVE_GL();

    HeapFree(GetProcessHeap(), 0, context->free_occlusion_queries);
    HeapFree(GetProcessHeap(), 0, context->free_event_queries);

    if (!pwglMakeCurrent(NULL, NULL))
    {
        ERR("Failed to disable GL context.\n");
    }

    if (context->isPBuffer)
    {
        GL_EXTCALL(wglReleasePbufferDCARB(context->pbuffer, context->hdc));
        GL_EXTCALL(wglDestroyPbufferARB(context->pbuffer));
    }
    else
    {
        ReleaseDC(context->win_handle, context->hdc);
    }

    if (!pwglDeleteContext(context->glCtx))
    {
        DWORD err = GetLastError();
        ERR("wglDeleteContext(%p) failed, last error %#x.\n", context->glCtx, err);
    }
}

DWORD context_get_tls_idx(void)
{
    return wined3d_context_tls_idx;
}

void context_set_tls_idx(DWORD idx)
{
    wined3d_context_tls_idx = idx;
}

struct wined3d_context *context_get_current(void)
{
    return TlsGetValue(wined3d_context_tls_idx);
}

BOOL context_set_current(struct wined3d_context *ctx)
{
    struct wined3d_context *old = context_get_current();

    if (old == ctx)
    {
        TRACE("Already using D3D context %p.\n", ctx);
        return TRUE;
    }

    if (old)
    {
        if (old->destroyed)
        {
            TRACE("Switching away from destroyed context %p.\n", old);
            context_destroy_gl_resources(old);
            HeapFree(GetProcessHeap(), 0, old);
        }
        else
        {
            old->current = 0;
        }
    }

    if (ctx)
    {
        TRACE("Switching to D3D context %p, GL context %p, device context %p.\n", ctx, ctx->glCtx, ctx->hdc);
        if (!pwglMakeCurrent(ctx->hdc, ctx->glCtx))
        {
            DWORD err = GetLastError();
            ERR("Failed to make GL context %p current on device context %p, last error %#x.\n",
                    ctx->glCtx, ctx->hdc, err);
            TlsSetValue(wined3d_context_tls_idx, NULL);
            return FALSE;
        }
        ctx->current = 1;
    }
    else if(pwglGetCurrentContext())
    {
        TRACE("Clearing current D3D context.\n");
        if (!pwglMakeCurrent(NULL, NULL))
        {
            DWORD err = GetLastError();
            ERR("Failed to clear current GL context, last error %#x.\n", err);
            TlsSetValue(wined3d_context_tls_idx, NULL);
            return FALSE;
        }
    }

    return TlsSetValue(wined3d_context_tls_idx, ctx);
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
static void Context_MarkStateDirty(struct wined3d_context *context, DWORD state, const struct StateEntry *StateTable)
{
    DWORD rep = StateTable[state].representative;
    DWORD idx;
    BYTE shift;

    if (isStateDirty(context, rep)) return;

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
static struct wined3d_context *AddContextToArray(IWineD3DDeviceImpl *This,
        HWND win_handle, HDC hdc, HGLRC glCtx, HPBUFFERARB pbuffer)
{
    struct wined3d_context **oldArray = This->contexts;
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

    This->contexts[This->numContexts] = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(**This->contexts));
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
        if (This->StateTable[state].representative)
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
    if (ds_format_desc->format != WINED3DFMT_S8_UINT_D24_UNORM)
    {
        FIXME("Add OpenGL context recreation support to SetDepthStencilSurface\n");
        ds_format_desc = getFormatDescEntry(WINED3DFMT_S8_UINT_D24_UNORM, &This->adapter->gl_info);
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
struct wined3d_context *CreateContext(IWineD3DDeviceImpl *This, IWineD3DSurfaceImpl *target,
        HWND win_handle, BOOL create_pbuffer, const WINED3DPRESENT_PARAMETERS *pPresentParms)
{
    const struct wined3d_gl_info *gl_info = &This->adapter->gl_info;
    struct wined3d_context *ret = NULL;
    HPBUFFERARB pbuffer = NULL;
    unsigned int s;
    HGLRC ctx;
    HDC hdc;

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

            if (color_format_desc->format == WINED3DFMT_B4G4R4X4_UNORM)
                color_format_desc = getFormatDescEntry(WINED3DFMT_B4G4R4A4_UNORM, &This->adapter->gl_info);
            else if (color_format_desc->format == WINED3DFMT_B8G8R8X8_UNORM)
                color_format_desc = getFormatDescEntry(WINED3DFMT_B8G8R8A8_UNORM, &This->adapter->gl_info);
        }

        /* DirectDraw supports 8bit paletted render targets and these are used by old games like Starcraft and C&C.
         * Most modern hardware doesn't support 8bit natively so we perform some form of 8bit -> 32bit conversion.
         * The conversion (ab)uses the alpha component for storing the palette index. For this reason we require
         * a format with 8bit alpha, so request A8R8G8B8. */
        if (color_format_desc->format == WINED3DFMT_P8_UINT)
            color_format_desc = getFormatDescEntry(WINED3DFMT_B8G8R8A8_UNORM, &This->adapter->gl_info);

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
            return NULL;
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
                    return NULL;
                }
            } else if(oldPixelFormat) {
                /* OpenGL doesn't allow pixel format adjustments. Print an error and continue using the old format.
                 * There's a big chance that the old format works although with a performance hit and perhaps rendering errors. */
                ERR("HDC=%p is already set to iPixelFormat=%d and OpenGL doesn't allow changes!\n", hdc, oldPixelFormat);
            } else {
                ERR("SetPixelFormat failed on HDC=%p for iPixelFormat=%d\n", hdc, iPixelFormat);
                return NULL;
            }
        }
    }

    ctx = pwglCreateContext(hdc);
    if (This->numContexts)
    {
        if (!pwglShareLists(This->contexts[0]->glCtx, ctx))
        {
            DWORD err = GetLastError();
            ERR("wglShareLists(%p, %p) failed, last error %#x.\n",
                    This->contexts[0]->glCtx, ctx, err);
        }
    }

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
        if (!pwglDeleteContext(ctx))
        {
            DWORD err = GetLastError();
            ERR("wglDeleteContext(%p) failed, last error %#x.\n", ctx, err);
        }
        if(create_pbuffer) {
            GL_EXTCALL(wglReleasePbufferDCARB(pbuffer, hdc));
            GL_EXTCALL(wglDestroyPbufferARB(pbuffer));
        }
        goto out;
    }
    ret->gl_info = &This->adapter->gl_info;
    ret->surface = (IWineD3DSurface *) target;
    ret->current_rt = (IWineD3DSurface *)target;
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

    ret->free_occlusion_query_size = 4;
    ret->free_occlusion_queries = HeapAlloc(GetProcessHeap(), 0,
            ret->free_occlusion_query_size * sizeof(*ret->free_occlusion_queries));
    if (!ret->free_occlusion_queries) goto out;

    list_init(&ret->occlusion_queries);

    ret->free_event_query_size = 4;
    ret->free_event_queries = HeapAlloc(GetProcessHeap(), 0,
            ret->free_event_query_size * sizeof(*ret->free_event_queries));
    if (!ret->free_event_queries) goto out;

    list_init(&ret->event_queries);

    TRACE("Successfully created new context %p\n", ret);

    list_init(&ret->fbo_list);

    /* Set up the context defaults */
    if (!context_set_current(ret))
    {
        ERR("Cannot activate context to set up defaults\n");
        goto out;
    }

    ENTER_GL();

    glGetIntegerv(GL_AUX_BUFFERS, &ret->aux_buffers);

    TRACE("Setting up the screen\n");
    /* Clear the screen */
    glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
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
            checkGLcall("glTexEnvi(GL_TEXTURE_SHADER_NV, GL_PREVIOUS_TEXTURE_INPUT_NV, ...");
        }
    }
    if(GL_SUPPORT(ARB_FRAGMENT_PROGRAM)) {
        /* MacOS(radeon X1600 at least, but most likely others too) refuses to draw if GLSL and ARBFP are
         * enabled, but the currently bound arbfp program is 0. Enabling ARBFP with prog 0 is invalid, but
         * GLSL should bypass this. This causes problems in programs that never use the fixed function pipeline,
         * because the ARBFP extension is enabled by the ARBFP pipeline at context creation, but no program
         * is ever assigned.
         *
         * So make sure a program is assigned to each context. The first real ARBFP use will set a different
         * program and the dummy program is destroyed when the context is destroyed.
         */
        const char *dummy_program =
                "!!ARBfp1.0\n"
                "MOV result.color, fragment.color.primary;\n"
                "END\n";
        GL_EXTCALL(glGenProgramsARB(1, &ret->dummy_arbfp_prog));
        GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, ret->dummy_arbfp_prog));
        GL_EXTCALL(glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, strlen(dummy_program), dummy_program));
    }

    for(s = 0; s < GL_LIMITS(point_sprite_units); s++) {
        GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + s));
        glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
        checkGLcall("glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE)");
    }

    if (GL_SUPPORT(ARB_PROVOKING_VERTEX))
    {
        GL_EXTCALL(glProvokingVertex(GL_FIRST_VERTEX_CONVENTION));
    }
    else if (GL_SUPPORT(EXT_PROVOKING_VERTEX))
    {
        GL_EXTCALL(glProvokingVertexEXT(GL_FIRST_VERTEX_CONVENTION_EXT));
    }

    LEAVE_GL();

    This->frag_pipe->enable_extension((IWineD3DDevice *) This, TRUE);

    return ret;

out:
    if (ret)
    {
        HeapFree(GetProcessHeap(), 0, ret->free_event_queries);
        HeapFree(GetProcessHeap(), 0, ret->free_occlusion_queries);
        HeapFree(GetProcessHeap(), 0, ret->pshader_const_dirty);
        HeapFree(GetProcessHeap(), 0, ret->vshader_const_dirty);
        HeapFree(GetProcessHeap(), 0, ret);
    }
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
static void RemoveContextFromArray(IWineD3DDeviceImpl *This, struct wined3d_context *context)
{
    struct wined3d_context **new_array;
    BOOL found = FALSE;
    UINT i;

    TRACE("Removing ctx %p\n", context);

    for (i = 0; i < This->numContexts; ++i)
    {
        if (This->contexts[i] == context)
        {
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
void DestroyContext(IWineD3DDeviceImpl *This, struct wined3d_context *context)
{
    BOOL destroy;

    TRACE("Destroying ctx %p\n", context);

    if (context->tid == GetCurrentThreadId() || !context->current)
    {
        context_destroy_gl_resources(context);
        destroy = TRUE;

        if (!context_set_current(NULL))
        {
            ERR("Failed to clear current D3D context.\n");
        }
    }
    else
    {
        context->destroyed = 1;
        destroy = FALSE;
    }

    HeapFree(GetProcessHeap(), 0, context->vshader_const_dirty);
    HeapFree(GetProcessHeap(), 0, context->pshader_const_dirty);
    RemoveContextFromArray(This, context);
    if (destroy) HeapFree(GetProcessHeap(), 0, context);
}

/* GL locking is done by the caller */
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
/* Context activation is done by the caller. */
static inline void SetupForBlit(IWineD3DDeviceImpl *This, struct wined3d_context *context, UINT width, UINT height)
{
    int i;
    const struct StateEntry *StateTable = This->StateTable;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    DWORD sampler;

    TRACE("Setting up context %p for blitting\n", context);
    if(context->last_was_blit) {
        if(context->blit_w != width || context->blit_h != height) {
            ENTER_GL();
            set_blit_dimension(width, height);
            LEAVE_GL();
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
    ENTER_GL();
    This->shader_backend->shader_select(context, FALSE, FALSE);
    LEAVE_GL();

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

        if (sampler != WINED3D_UNMAPPED_STAGE)
        {
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
                  0.0f);
        checkGLcall("glTexEnvi GL_TEXTURE_LOD_BIAS_EXT ...");
    }

    if (sampler != WINED3D_UNMAPPED_STAGE)
    {
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
    Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_COLORWRITEENABLE), StateTable);
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

    set_blit_dimension(width, height);

    LEAVE_GL();

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
static struct wined3d_context *findThreadContextForSwapChain(IWineD3DSwapChain *swapchain, DWORD tid)
{
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
static inline struct wined3d_context *FindContext(IWineD3DDeviceImpl *This, IWineD3DSurface *target, DWORD tid)
{
    IWineD3DSwapChain *swapchain = NULL;
    struct wined3d_context *current_context = context_get_current();
    const struct StateEntry *StateTable = This->StateTable;
    struct wined3d_context *context;
    BOOL old_render_offscreen;

    if (current_context && current_context->destroyed) current_context = NULL;

    if (!target)
    {
        if (current_context
                && current_context->current_rt
                && ((IWineD3DSurfaceImpl *)current_context->surface)->resource.wineD3DDevice == This)
        {
            target = current_context->current_rt;
        }
        else
        {
            IWineD3DSwapChainImpl *swapchain = (IWineD3DSwapChainImpl *)This->swapchains[0];
            if (swapchain->backBuffer) target = swapchain->backBuffer[0];
            else target = swapchain->frontBuffer;
        }
    }

    if (current_context && current_context->current_rt == target)
    {
        return current_context;
    }

    if (SUCCEEDED(IWineD3DSurface_GetContainer(target, &IID_IWineD3DSwapChain, (void **)&swapchain))) {
        TRACE("Rendering onscreen\n");

        context = findThreadContextForSwapChain(swapchain, tid);

        old_render_offscreen = context->render_offscreen;
        context->render_offscreen = FALSE;
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
    }
    else
    {
        TRACE("Rendering offscreen\n");

retry:
        if (wined3d_settings.offscreen_rendering_mode == ORM_PBUFFER)
        {
            IWineD3DSurfaceImpl *targetimpl = (IWineD3DSurfaceImpl *)target;
            if (!This->pbufferContext
                    || This->pbufferWidth < targetimpl->currentDesc.Width
                    || This->pbufferHeight < targetimpl->currentDesc.Height)
            {
                if (This->pbufferContext) DestroyContext(This, This->pbufferContext);

                /* The display is irrelevant here, the window is 0. But
                 * CreateContext needs a valid X connection. Create the context
                 * on the same server as the primary swapchain. The primary
                 * swapchain is exists at this point. */
                This->pbufferContext = CreateContext(This, targetimpl,
                        ((IWineD3DSwapChainImpl *)This->swapchains[0])->context[0]->win_handle,
                        TRUE /* pbuffer */, &((IWineD3DSwapChainImpl *)This->swapchains[0])->presentParms);
                This->pbufferWidth = targetimpl->currentDesc.Width;
                This->pbufferHeight = targetimpl->currentDesc.Height;
            }

            if (This->pbufferContext)
            {
                if (This->pbufferContext->tid && This->pbufferContext->tid != tid)
                {
                    FIXME("The PBuffer context is only supported for one thread for now!\n");
                }
                This->pbufferContext->tid = tid;
                context = This->pbufferContext;
            }
            else
            {
                ERR("Failed to create a buffer context and drawable, falling back to back buffer offscreen rendering.\n");
                wined3d_settings.offscreen_rendering_mode = ORM_BACKBUFFER;
                goto retry;
            }
        }
        else
        {
            /* Stay with the currently active context. */
            if (current_context
                    && ((IWineD3DSurfaceImpl *)current_context->surface)->resource.wineD3DDevice == This)
            {
                context = current_context;
            }
            else
            {
                /* This may happen if the app jumps straight into offscreen rendering
                 * Start using the context of the primary swapchain. tid == 0 is no problem
                 * for findThreadContextForSwapChain.
                 *
                 * Can also happen on thread switches - in that case findThreadContextForSwapChain
                 * is perfect to call. */
                context = findThreadContextForSwapChain(This->swapchains[0], tid);
            }
        }

        old_render_offscreen = context->render_offscreen;
        context->render_offscreen = TRUE;
    }

    if (context->render_offscreen != old_render_offscreen)
    {
        Context_MarkStateDirty(context, STATE_TRANSFORM(WINED3DTS_PROJECTION), StateTable);
        Context_MarkStateDirty(context, STATE_VDECL, StateTable);
        Context_MarkStateDirty(context, STATE_VIEWPORT, StateTable);
        Context_MarkStateDirty(context, STATE_SCISSORRECT, StateTable);
        Context_MarkStateDirty(context, STATE_FRONTFACE, StateTable);
    }

    /* To compensate the lack of format switching with some offscreen rendering methods and on onscreen buffers
     * the alpha blend state changes with different render target formats. */
    if (!context->current_rt)
    {
        Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_ALPHABLENDENABLE), StateTable);
    }
    else
    {
        const struct GlPixelFormatDesc *old = ((IWineD3DSurfaceImpl *)context->current_rt)->resource.format_desc;
        const struct GlPixelFormatDesc *new = ((IWineD3DSurfaceImpl *)target)->resource.format_desc;

        if (old->format != new->format)
        {
            /* Disable blending when the alpha mask has changed and when a format doesn't support blending. */
            if ((old->alpha_mask && !new->alpha_mask) || (!old->alpha_mask && new->alpha_mask)
                    || !(new->Flags & WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING))
            {
                Context_MarkStateDirty(context, STATE_RENDER(WINED3DRS_ALPHABLENDENABLE), StateTable);
            }
        }

        /* When switching away from an offscreen render target, and we're not
         * using FBOs, we have to read the drawable into the texture. This is
         * done via PreLoad (and SFLAG_INDRAWABLE set on the surface). There
         * are some things that need care though. PreLoad needs a GL context,
         * and FindContext is called before the context is activated. It also
         * has to be called with the old rendertarget active, otherwise a
         * wrong drawable is read. */
        if (wined3d_settings.offscreen_rendering_mode != ORM_FBO
                && old_render_offscreen && context->current_rt != target)
        {
            BOOL oldInDraw = This->isInDraw;

            /* surface_internal_preload() requires a context to load the
             * texture, so it will call ActivateContext. Set isInDraw to true
             * to signal surface_internal_preload() that it has a context. */

            /* FIXME: This is just broken. There's no guarantee whatsoever
             * that the currently active context, if any, is appropriate for
             * reading back the render target. We should probably call
             * context_set_current(context) here and then rely on
             * ActivateContext() doing the right thing. */
            This->isInDraw = TRUE;

            /* Read the back buffer of the old drawable into the destination texture. */
            if (((IWineD3DSurfaceImpl *)context->current_rt)->texture_name_srgb)
            {
                surface_internal_preload(context->current_rt, SRGB_BOTH);
            }
            else
            {
                surface_internal_preload(context->current_rt, SRGB_RGB);
            }

            IWineD3DSurface_ModifyLocation(context->current_rt, SFLAG_INDRAWABLE, FALSE);

            This->isInDraw = oldInDraw;
        }
    }

    context->draw_buffer_dirty = TRUE;
    context->current_rt = target;

    return context;
}

/* Context activation is done by the caller. */
static void context_apply_draw_buffer(struct wined3d_context *context, BOOL blit)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    IWineD3DSurface *rt = context->current_rt;
    IWineD3DSwapChain *swapchain;
    IWineD3DDeviceImpl *device;

    device = ((IWineD3DSurfaceImpl *)rt)->resource.wineD3DDevice;
    if (SUCCEEDED(IWineD3DSurface_GetContainer(rt, &IID_IWineD3DSwapChain, (void **)&swapchain)))
    {
        IWineD3DSwapChain_Release((IUnknown *)swapchain);
        ENTER_GL();
        glDrawBuffer(surface_get_gl_buffer(rt, swapchain));
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
                    GL_EXTCALL(glDrawBuffersARB(GL_LIMITS(buffers), device->draw_buffers));
                    checkGLcall("glDrawBuffers()");
                }
                else
                {
                    glDrawBuffer(device->draw_buffers[0]);
                    checkGLcall("glDrawBuffer()");
                }
            } else {
                glDrawBuffer(GL_COLOR_ATTACHMENT0);
                checkGLcall("glDrawBuffer()");
            }
        }
        else
        {
            glDrawBuffer(device->offscreenBuffer);
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
struct wined3d_context *ActivateContext(IWineD3DDeviceImpl *This, IWineD3DSurface *target, enum ContextUsage usage)
{
    struct wined3d_context *current_context = context_get_current();
    DWORD                         tid = GetCurrentThreadId();
    DWORD                         i, dirtyState, idx;
    BYTE                          shift;
    const struct StateEntry       *StateTable = This->StateTable;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context *context;

    TRACE("(%p): Selecting context for render target %p, thread %d\n", This, target, tid);

    context = FindContext(This, target, tid);

    gl_info = context->gl_info;

    /* Activate the opengl context */
    if (context != current_context)
    {
        if (!context_set_current(context)) ERR("Failed to activate the new context.\n");
        else This->frag_pipe->enable_extension((IWineD3DDevice *)This, !context->last_was_blit);

        if (context->vshader_const_dirty)
        {
            memset(context->vshader_const_dirty, 1,
                    sizeof(*context->vshader_const_dirty) * GL_LIMITS(vshader_constantsF));
            This->highest_dirty_vs_const = GL_LIMITS(vshader_constantsF);
        }
        if (context->pshader_const_dirty)
        {
            memset(context->pshader_const_dirty, 1,
                   sizeof(*context->pshader_const_dirty) * GL_LIMITS(pshader_constantsF));
            This->highest_dirty_ps_const = GL_LIMITS(pshader_constantsF);
        }
    }

    switch (usage) {
        case CTXUSAGE_CLEAR:
        case CTXUSAGE_DRAWPRIM:
            if (wined3d_settings.offscreen_rendering_mode == ORM_FBO) {
                ENTER_GL();
                context_apply_fbo_state(context);
                LEAVE_GL();
            }
            if (context->draw_buffer_dirty) {
                context_apply_draw_buffer(context, FALSE);
                context->draw_buffer_dirty = FALSE;
            }
            break;

        case CTXUSAGE_BLIT:
            if (wined3d_settings.offscreen_rendering_mode == ORM_FBO) {
                if (context->render_offscreen)
                {
                    FIXME("Activating for CTXUSAGE_BLIT for an offscreen target with ORM_FBO. This should be avoided.\n");
                    ENTER_GL();
                    context_bind_fbo(context, GL_FRAMEBUFFER, &context->dst_fbo);
                    context_attach_surface_fbo(context, GL_FRAMEBUFFER, 0, target);
                    context_attach_depth_stencil_fbo(context, GL_FRAMEBUFFER, NULL, FALSE);
                    LEAVE_GL();
                } else {
                    ENTER_GL();
                    context_bind_fbo(context, GL_FRAMEBUFFER, NULL);
                    LEAVE_GL();
                }
                context->draw_buffer_dirty = TRUE;
            }
            if (context->draw_buffer_dirty) {
                context_apply_draw_buffer(context, TRUE);
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

            ENTER_GL();
            for(i=0; i < context->numDirtyEntries; i++) {
                dirtyState = context->dirtyArray[i];
                idx = dirtyState >> 5;
                shift = dirtyState & 0x1f;
                context->isStateDirty[idx] &= ~(1 << shift);
                StateTable[dirtyState].apply(dirtyState, This->stateBlock, context);
            }
            LEAVE_GL();
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

    return context;
}
