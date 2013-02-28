/*
 * Context and render target management in wined3d
 *
 * Copyright 2007-2008 Stefan Dösinger for CodeWeavers
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

#include <config.h>
#include <wine/port.h>

//#include <stdio.h>
#ifdef HAVE_FLOAT_H
# include <float.h>
#endif

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);

static DWORD wined3d_context_tls_idx;

/* FBO helper functions */

/* Context activation is done by the caller. */
static void context_bind_fbo(struct wined3d_context *context, GLenum target, GLuint *fbo)
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

/* Context activation is done by the caller. */
static void context_clean_fbo_attachments(const struct wined3d_gl_info *gl_info, GLenum target)
{
    unsigned int i;

    for (i = 0; i < gl_info->limits.buffers; ++i)
    {
        gl_info->fbo_ops.glFramebufferTexture2D(target, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, 0, 0);
        checkGLcall("glFramebufferTexture2D()");
    }
    gl_info->fbo_ops.glFramebufferTexture2D(target, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
    checkGLcall("glFramebufferTexture2D()");

    gl_info->fbo_ops.glFramebufferTexture2D(target, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, 0, 0);
    checkGLcall("glFramebufferTexture2D()");
}

/* Context activation is done by the caller. */
static void context_destroy_fbo(struct wined3d_context *context, GLuint *fbo)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    context_bind_fbo(context, GL_FRAMEBUFFER, fbo);
    context_clean_fbo_attachments(gl_info, GL_FRAMEBUFFER);
    context_bind_fbo(context, GL_FRAMEBUFFER, NULL);

    gl_info->fbo_ops.glDeleteFramebuffers(1, fbo);
    checkGLcall("glDeleteFramebuffers()");
}

static void context_attach_depth_stencil_rb(const struct wined3d_gl_info *gl_info,
        GLenum fbo_target, DWORD format_flags, GLuint rb)
{
    if (format_flags & WINED3DFMT_FLAG_DEPTH)
    {
        gl_info->fbo_ops.glFramebufferRenderbuffer(fbo_target, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb);
        checkGLcall("glFramebufferRenderbuffer()");
    }

    if (format_flags & WINED3DFMT_FLAG_STENCIL)
    {
        gl_info->fbo_ops.glFramebufferRenderbuffer(fbo_target, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rb);
        checkGLcall("glFramebufferRenderbuffer()");
    }
}

/* Context activation is done by the caller. */
static void context_attach_depth_stencil_fbo(struct wined3d_context *context,
        GLenum fbo_target, struct wined3d_surface *depth_stencil, DWORD location)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    TRACE("Attach depth stencil %p\n", depth_stencil);

    if (depth_stencil)
    {
        DWORD format_flags = depth_stencil->resource.format->flags;

        if (depth_stencil->current_renderbuffer)
        {
            context_attach_depth_stencil_rb(gl_info, fbo_target,
                    format_flags, depth_stencil->current_renderbuffer->id);
        }
        else
        {
            switch (location)
            {
                case SFLAG_INTEXTURE:
                case SFLAG_INSRGBTEX:
                    surface_prepare_texture(depth_stencil, context, FALSE);

                    if (format_flags & WINED3DFMT_FLAG_DEPTH)
                    {
                        gl_info->fbo_ops.glFramebufferTexture2D(fbo_target, GL_DEPTH_ATTACHMENT,
                                depth_stencil->texture_target, depth_stencil->texture_name,
                                depth_stencil->texture_level);
                        checkGLcall("glFramebufferTexture2D()");
                    }

                    if (format_flags & WINED3DFMT_FLAG_STENCIL)
                    {
                        gl_info->fbo_ops.glFramebufferTexture2D(fbo_target, GL_STENCIL_ATTACHMENT,
                                depth_stencil->texture_target, depth_stencil->texture_name,
                                depth_stencil->texture_level);
                        checkGLcall("glFramebufferTexture2D()");
                    }
                    break;

                case SFLAG_INRB_MULTISAMPLE:
                    surface_prepare_rb(depth_stencil, gl_info, TRUE);
                    context_attach_depth_stencil_rb(gl_info, fbo_target,
                            format_flags, depth_stencil->rb_multisample);
                    break;

                case SFLAG_INRB_RESOLVED:
                    surface_prepare_rb(depth_stencil, gl_info, FALSE);
                    context_attach_depth_stencil_rb(gl_info, fbo_target,
                            format_flags, depth_stencil->rb_resolved);
                    break;

                default:
                    ERR("Unsupported location %s (%#x).\n", debug_surflocation(location), location);
                    break;
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

/* Context activation is done by the caller. */
static void context_attach_surface_fbo(struct wined3d_context *context,
        GLenum fbo_target, DWORD idx, struct wined3d_surface *surface, DWORD location)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    TRACE("Attach surface %p to %u\n", surface, idx);

    if (surface && surface->resource.format->id != WINED3DFMT_NULL)
    {
        BOOL srgb;

        switch (location)
        {
            case SFLAG_INTEXTURE:
            case SFLAG_INSRGBTEX:
                srgb = location == SFLAG_INSRGBTEX;
                surface_prepare_texture(surface, context, srgb);
                gl_info->fbo_ops.glFramebufferTexture2D(fbo_target, GL_COLOR_ATTACHMENT0 + idx,
                        surface->texture_target, surface_get_texture_name(surface, gl_info, srgb),
                        surface->texture_level);
                checkGLcall("glFramebufferTexture2D()");
                break;

            case SFLAG_INRB_MULTISAMPLE:
                surface_prepare_rb(surface, gl_info, TRUE);
                gl_info->fbo_ops.glFramebufferRenderbuffer(fbo_target, GL_COLOR_ATTACHMENT0 + idx,
                        GL_RENDERBUFFER, surface->rb_multisample);
                checkGLcall("glFramebufferRenderbuffer()");
                break;

            case SFLAG_INRB_RESOLVED:
                surface_prepare_rb(surface, gl_info, FALSE);
                gl_info->fbo_ops.glFramebufferRenderbuffer(fbo_target, GL_COLOR_ATTACHMENT0 + idx,
                        GL_RENDERBUFFER, surface->rb_resolved);
                checkGLcall("glFramebufferRenderbuffer()");
                break;

            default:
                ERR("Unsupported location %s (%#x).\n", debug_surflocation(location), location);
                break;
        }
    }
    else
    {
        gl_info->fbo_ops.glFramebufferTexture2D(fbo_target, GL_COLOR_ATTACHMENT0 + idx, GL_TEXTURE_2D, 0, 0);
        checkGLcall("glFramebufferTexture2D()");
    }
}

/* Context activation is done by the caller. */
void context_check_fbo_status(const struct wined3d_context *context, GLenum target)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    GLenum status;

    if (!FIXME_ON(d3d)) return;

    status = gl_info->fbo_ops.glCheckFramebufferStatus(target);
    if (status == GL_FRAMEBUFFER_COMPLETE)
    {
        TRACE("FBO complete\n");
    }
    else
    {
        const struct wined3d_surface *attachment;
        unsigned int i;

        FIXME("FBO status %s (%#x)\n", debug_fbostatus(status), status);

        if (!context->current_fbo)
        {
            ERR("FBO 0 is incomplete, driver bug?\n");
            return;
        }

        FIXME("\tLocation %s (%#x).\n", debug_surflocation(context->current_fbo->location),
                context->current_fbo->location);

        /* Dump the FBO attachments */
        for (i = 0; i < gl_info->limits.buffers; ++i)
        {
            attachment = context->current_fbo->render_targets[i];
            if (attachment)
            {
                FIXME("\tColor attachment %d: (%p) %s %ux%u %u samples.\n",
                        i, attachment, debug_d3dformat(attachment->resource.format->id),
                        attachment->pow2Width, attachment->pow2Height, attachment->resource.multisample_type);
            }
        }
        attachment = context->current_fbo->depth_stencil;
        if (attachment)
        {
            FIXME("\tDepth attachment: (%p) %s %ux%u %u samples.\n",
                    attachment, debug_d3dformat(attachment->resource.format->id),
                    attachment->pow2Width, attachment->pow2Height, attachment->resource.multisample_type);
        }
    }
}

static inline DWORD context_generate_rt_mask(GLenum buffer)
{
    /* Should take care of all the GL_FRONT/GL_BACK/GL_AUXi/GL_NONE... cases */
    return buffer ? (1 << 31) | buffer : 0;
}

static inline DWORD context_generate_rt_mask_from_surface(const struct wined3d_surface *target)
{
    return (1 << 31) | surface_get_gl_buffer(target);
}

static struct fbo_entry *context_create_fbo_entry(const struct wined3d_context *context,
        struct wined3d_surface **render_targets, struct wined3d_surface *depth_stencil, DWORD location)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct fbo_entry *entry;

    entry = HeapAlloc(GetProcessHeap(), 0, sizeof(*entry));
    entry->render_targets = HeapAlloc(GetProcessHeap(), 0, gl_info->limits.buffers * sizeof(*entry->render_targets));
    memcpy(entry->render_targets, render_targets, gl_info->limits.buffers * sizeof(*entry->render_targets));
    entry->depth_stencil = depth_stencil;
    entry->location = location;
    entry->rt_mask = context_generate_rt_mask(GL_COLOR_ATTACHMENT0);
    entry->attached = FALSE;
    entry->id = 0;

    return entry;
}

/* Context activation is done by the caller. */
static void context_reuse_fbo_entry(struct wined3d_context *context, GLenum target,
        struct wined3d_surface **render_targets, struct wined3d_surface *depth_stencil,
        DWORD location, struct fbo_entry *entry)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    context_bind_fbo(context, target, &entry->id);
    context_clean_fbo_attachments(gl_info, target);

    memcpy(entry->render_targets, render_targets, gl_info->limits.buffers * sizeof(*entry->render_targets));
    entry->depth_stencil = depth_stencil;
    entry->location = location;
    entry->attached = FALSE;
}

/* Context activation is done by the caller. */
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

/* Context activation is done by the caller. */
static struct fbo_entry *context_find_fbo_entry(struct wined3d_context *context, GLenum target,
        struct wined3d_surface **render_targets, struct wined3d_surface *depth_stencil, DWORD location)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct fbo_entry *entry;

    if (depth_stencil && render_targets && render_targets[0])
    {
        if (depth_stencil->resource.width < render_targets[0]->resource.width ||
            depth_stencil->resource.height < render_targets[0]->resource.height)
        {
            WARN("Depth stencil is smaller than the primary color buffer, disabling\n");
            depth_stencil = NULL;
        }
    }

    LIST_FOR_EACH_ENTRY(entry, &context->fbo_list, struct fbo_entry, entry)
    {
        if (!memcmp(entry->render_targets,
                render_targets, gl_info->limits.buffers * sizeof(*entry->render_targets))
                && entry->depth_stencil == depth_stencil && entry->location == location)
        {
            list_remove(&entry->entry);
            list_add_head(&context->fbo_list, &entry->entry);
            return entry;
        }
    }

    if (context->fbo_entry_count < WINED3D_MAX_FBO_ENTRIES)
    {
        entry = context_create_fbo_entry(context, render_targets, depth_stencil, location);
        list_add_head(&context->fbo_list, &entry->entry);
        ++context->fbo_entry_count;
    }
    else
    {
        entry = LIST_ENTRY(list_tail(&context->fbo_list), struct fbo_entry, entry);
        context_reuse_fbo_entry(context, target, render_targets, depth_stencil, location, entry);
        list_remove(&entry->entry);
        list_add_head(&context->fbo_list, &entry->entry);
    }

    return entry;
}

/* Context activation is done by the caller. */
static void context_apply_fbo_entry(struct wined3d_context *context, GLenum target, struct fbo_entry *entry)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    unsigned int i;

    context_bind_fbo(context, target, &entry->id);

    if (entry->attached) return;

    /* Apply render targets */
    for (i = 0; i < gl_info->limits.buffers; ++i)
    {
        context_attach_surface_fbo(context, target, i, entry->render_targets[i], entry->location);
    }

    /* Apply depth targets */
    if (entry->depth_stencil)
        surface_set_compatible_renderbuffer(entry->depth_stencil, entry->render_targets[0]);
    context_attach_depth_stencil_fbo(context, target, entry->depth_stencil, entry->location);

    entry->attached = TRUE;
}

/* Context activation is done by the caller. */
static void context_apply_fbo_state(struct wined3d_context *context, GLenum target,
        struct wined3d_surface **render_targets, struct wined3d_surface *depth_stencil, DWORD location)
{
    struct fbo_entry *entry, *entry2;

    LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &context->fbo_destroy_list, struct fbo_entry, entry)
    {
        context_destroy_fbo_entry(context, entry);
    }

    if (context->rebind_fbo)
    {
        context_bind_fbo(context, GL_FRAMEBUFFER, NULL);
        context->rebind_fbo = FALSE;
    }

    if (location == SFLAG_INDRAWABLE)
    {
        context->current_fbo = NULL;
        context_bind_fbo(context, target, NULL);
    }
    else
    {
        context->current_fbo = context_find_fbo_entry(context, target, render_targets, depth_stencil, location);
        context_apply_fbo_entry(context, target, context->current_fbo);
    }
}

/* Context activation is done by the caller. */
void context_apply_fbo_state_blit(struct wined3d_context *context, GLenum target,
        struct wined3d_surface *render_target, struct wined3d_surface *depth_stencil, DWORD location)
{
    UINT clear_size = (context->gl_info->limits.buffers - 1) * sizeof(*context->blit_targets);

    context->blit_targets[0] = render_target;
    if (clear_size)
        memset(&context->blit_targets[1], 0, clear_size);
    context_apply_fbo_state(context, target, context->blit_targets, depth_stencil, location);
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
        if (gl_info->supported[ARB_OCCLUSION_QUERY])
        {
            GL_EXTCALL(glGenQueriesARB(1, &query->id));
            checkGLcall("glGenQueriesARB");

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
        query->object = context->free_event_queries[--context->free_event_query_count];
    }
    else
    {
        if (gl_info->supported[ARB_SYNC])
        {
            /* Using ARB_sync, not much to do here. */
            query->object.sync = NULL;
            TRACE("Allocated event query %p in context %p.\n", query->object.sync, context);
        }
        else if (gl_info->supported[APPLE_FENCE])
        {
            GL_EXTCALL(glGenFencesAPPLE(1, &query->object.id));
            checkGLcall("glGenFencesAPPLE");

            TRACE("Allocated event query %u in context %p.\n", query->object.id, context);
        }
        else if(gl_info->supported[NV_FENCE])
        {
            GL_EXTCALL(glGenFencesNV(1, &query->object.id));
            checkGLcall("glGenFencesNV");

            TRACE("Allocated event query %u in context %p.\n", query->object.id, context);
        }
        else
        {
            WARN("Event queries not supported, not allocating query id.\n");
            query->object.id = 0;
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
        union wined3d_gl_query_object *new_data = HeapReAlloc(GetProcessHeap(), 0, context->free_event_queries,
                new_size * sizeof(*context->free_event_queries));

        if (!new_data)
        {
            ERR("Failed to grow free list, leaking query %u in context %p.\n", query->object.id, context);
            return;
        }

        context->free_event_query_size = new_size;
        context->free_event_queries = new_data;
    }

    context->free_event_queries[context->free_event_query_count++] = query->object;
}

typedef void (context_fbo_entry_func_t)(struct wined3d_context *context, struct fbo_entry *entry);

static void context_enum_surface_fbo_entries(const struct wined3d_device *device,
        const struct wined3d_surface *surface, context_fbo_entry_func_t *callback)
{
    UINT i;

    for (i = 0; i < device->context_count; ++i)
    {
        struct wined3d_context *context = device->contexts[i];
        const struct wined3d_gl_info *gl_info = context->gl_info;
        struct fbo_entry *entry, *entry2;

        if (context->current_rt == surface) context->current_rt = NULL;

        LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &context->fbo_list, struct fbo_entry, entry)
        {
            UINT j;

            if (entry->depth_stencil == surface)
            {
                callback(context, entry);
                continue;
            }

            for (j = 0; j < gl_info->limits.buffers; ++j)
            {
                if (entry->render_targets[j] == surface)
                {
                    callback(context, entry);
                    break;
                }
            }
        }
    }
}

static void context_queue_fbo_entry_destruction(struct wined3d_context *context, struct fbo_entry *entry)
{
    list_remove(&entry->entry);
    list_add_head(&context->fbo_destroy_list, &entry->entry);
}

void context_resource_released(const struct wined3d_device *device,
        struct wined3d_resource *resource, enum wined3d_resource_type type)
{
    if (!device->d3d_initialized) return;

    switch (type)
    {
        case WINED3D_RTYPE_SURFACE:
            context_enum_surface_fbo_entries(device, surface_from_resource(resource),
                    context_queue_fbo_entry_destruction);
            break;

        default:
            break;
    }
}

static void context_detach_fbo_entry(struct wined3d_context *context, struct fbo_entry *entry)
{
    entry->attached = FALSE;
}

void context_resource_unloaded(const struct wined3d_device *device,
        struct wined3d_resource *resource, enum wined3d_resource_type type)
{
    switch (type)
    {
        case WINED3D_RTYPE_SURFACE:
            context_enum_surface_fbo_entries(device, surface_from_resource(resource),
                    context_detach_fbo_entry);
            break;

        default:
            break;
    }
}

void context_surface_update(struct wined3d_context *context, const struct wined3d_surface *surface)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct fbo_entry *entry = context->current_fbo;
    unsigned int i;

    if (!entry || context->rebind_fbo) return;

    for (i = 0; i < gl_info->limits.buffers; ++i)
    {
        if (surface == entry->render_targets[i])
        {
            TRACE("Updated surface %p is bound as color attachment %u to the current FBO.\n", surface, i);
            context->rebind_fbo = TRUE;
            return;
        }
    }

    if (surface == entry->depth_stencil)
    {
        TRACE("Updated surface %p is bound as depth attachment to the current FBO.\n", surface);
        context->rebind_fbo = TRUE;
    }
}

static BOOL context_set_pixel_format(const struct wined3d_gl_info *gl_info, HDC dc, int format)
{
    int current = GetPixelFormat(dc);

    if (current == format) return TRUE;

    if (!current)
    {
        if (!SetPixelFormat(dc, format, NULL))
        {
            /* This may also happen if the dc belongs to a destroyed window. */
            WARN("Failed to set pixel format %d on device context %p, last error %#x.\n",
                    format, dc, GetLastError());
            return FALSE;
        }
        return TRUE;
    }

    /* By default WGL doesn't allow pixel format adjustments but we need it
     * here. For this reason there's a Wine specific wglSetPixelFormat()
     * which allows us to set the pixel format multiple times. Only use it
     * when really needed. */
    if (gl_info->supported[WGL_WINE_PIXEL_FORMAT_PASSTHROUGH])
    {
        if (!GL_EXTCALL(wglSetPixelFormatWINE(dc, format)))
        {
            ERR("wglSetPixelFormatWINE failed to set pixel format %d on device context %p.\n",
                    format, dc);
            return FALSE;
        }
        return TRUE;
    }

    /* OpenGL doesn't allow pixel format adjustments. Print an error and
     * continue using the old format. There's a big chance that the old
     * format works although with a performance hit and perhaps rendering
     * errors. */
    ERR("Unable to set pixel format %d on device context %p. Already using format %d.\n",
            format, dc, current);
    return TRUE;
}

static BOOL context_set_gl_context(struct wined3d_context *ctx)
{
    struct wined3d_swapchain *swapchain = ctx->swapchain;
    BOOL backup = FALSE;

    if (!context_set_pixel_format(ctx->gl_info, ctx->hdc, ctx->pixel_format))
    {
        WARN("Failed to set pixel format %d on device context %p.\n",
                ctx->pixel_format, ctx->hdc);
        backup = TRUE;
    }

    if (backup || !wglMakeCurrent(ctx->hdc, ctx->glCtx))
    {
        HDC dc;

        WARN("Failed to make GL context %p current on device context %p, last error %#x.\n",
                ctx->glCtx, ctx->hdc, GetLastError());
        ctx->valid = 0;
        WARN("Trying fallback to the backup window.\n");

        /* FIXME: If the context is destroyed it's no longer associated with
         * a swapchain, so we can't use the swapchain to get a backup dc. To
         * make this work windowless contexts would need to be handled by the
         * device. */
        if (ctx->destroyed)
        {
            FIXME("Unable to get backup dc for destroyed context %p.\n", ctx);
            context_set_current(NULL);
            return FALSE;
        }

        if (!(dc = swapchain_get_backup_dc(swapchain)))
        {
            context_set_current(NULL);
            return FALSE;
        }

        if (!context_set_pixel_format(ctx->gl_info, dc, ctx->pixel_format))
        {
            ERR("Failed to set pixel format %d on device context %p.\n",
                    ctx->pixel_format, dc);
            context_set_current(NULL);
            return FALSE;
        }

        if (!wglMakeCurrent(dc, ctx->glCtx))
        {
            ERR("Fallback to backup window (dc %p) failed too, last error %#x.\n",
                    dc, GetLastError());
            context_set_current(NULL);
            return FALSE;
        }
    }
    return TRUE;
}

static void context_restore_gl_context(const struct wined3d_gl_info *gl_info, HDC dc, HGLRC gl_ctx, int pf)
{
    if (!context_set_pixel_format(gl_info, dc, pf))
    {
        ERR("Failed to restore pixel format %d on device context %p.\n", pf, dc);
        context_set_current(NULL);
        return;
    }

    if (!wglMakeCurrent(dc, gl_ctx))
    {
        ERR("Failed to restore GL context %p on device context %p, last error %#x.\n",
                gl_ctx, dc, GetLastError());
        context_set_current(NULL);
    }
}

static void context_update_window(struct wined3d_context *context)
{
    if (context->win_handle == context->swapchain->win_handle)
        return;

    TRACE("Updating context %p window from %p to %p.\n",
            context, context->win_handle, context->swapchain->win_handle);

    if (context->valid)
    {
        /* You'd figure ReleaseDC() would fail if the DC doesn't match the
         * window. However, that's not what actually happens, and there are
         * user32 tests that confirm ReleaseDC() with the wrong window is
         * supposed to succeed. So explicitly check that the DC belongs to
         * the window, since we want to avoid releasing a DC that belongs to
         * some other window if the original window was already destroyed. */
        if (WindowFromDC(context->hdc) != context->win_handle)
        {
            WARN("DC %p does not belong to window %p.\n",
                    context->hdc, context->win_handle);
        }
        else if (!ReleaseDC(context->win_handle, context->hdc))
        {
            ERR("Failed to release device context %p, last error %#x.\n",
                    context->hdc, GetLastError());
        }
    }
    else context->valid = 1;

    context->win_handle = context->swapchain->win_handle;

    if (!(context->hdc = GetDC(context->win_handle)))
    {
        ERR("Failed to get a device context for window %p.\n", context->win_handle);
        goto err;
    }

    if (!context_set_pixel_format(context->gl_info, context->hdc, context->pixel_format))
    {
        ERR("Failed to set pixel format %d on device context %p.\n",
                context->pixel_format, context->hdc);
        goto err;
    }

    context_set_gl_context(context);

    return;

err:
    context->valid = 0;
}

/* Do not call while under the GL lock. */
static void context_destroy_gl_resources(struct wined3d_context *context)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct wined3d_occlusion_query *occlusion_query;
    struct wined3d_event_query *event_query;
    struct fbo_entry *entry, *entry2;
    HGLRC restore_ctx;
    HDC restore_dc;
    unsigned int i;
    int restore_pf;

    restore_ctx = wglGetCurrentContext();
    restore_dc = wglGetCurrentDC();
    restore_pf = GetPixelFormat(restore_dc);

    if (context->valid && restore_ctx != context->glCtx)
        context_set_gl_context(context);
    else
        restore_ctx = NULL;

    LIST_FOR_EACH_ENTRY(occlusion_query, &context->occlusion_queries, struct wined3d_occlusion_query, entry)
    {
        if (context->valid && gl_info->supported[ARB_OCCLUSION_QUERY])
            GL_EXTCALL(glDeleteQueriesARB(1, &occlusion_query->id));
        occlusion_query->context = NULL;
    }

    LIST_FOR_EACH_ENTRY(event_query, &context->event_queries, struct wined3d_event_query, entry)
    {
        if (context->valid)
        {
            if (gl_info->supported[ARB_SYNC])
            {
                if (event_query->object.sync) GL_EXTCALL(glDeleteSync(event_query->object.sync));
            }
            else if (gl_info->supported[APPLE_FENCE]) GL_EXTCALL(glDeleteFencesAPPLE(1, &event_query->object.id));
            else if (gl_info->supported[NV_FENCE]) GL_EXTCALL(glDeleteFencesNV(1, &event_query->object.id));
        }
        event_query->context = NULL;
    }

    LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &context->fbo_destroy_list, struct fbo_entry, entry)
    {
        if (!context->valid) entry->id = 0;
        context_destroy_fbo_entry(context, entry);
    }

    LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &context->fbo_list, struct fbo_entry, entry)
    {
        if (!context->valid) entry->id = 0;
        context_destroy_fbo_entry(context, entry);
    }

    if (context->valid)
    {
        if (context->dummy_arbfp_prog)
        {
            GL_EXTCALL(glDeleteProgramsARB(1, &context->dummy_arbfp_prog));
        }

        if (gl_info->supported[ARB_OCCLUSION_QUERY])
            GL_EXTCALL(glDeleteQueriesARB(context->free_occlusion_query_count, context->free_occlusion_queries));

        if (gl_info->supported[ARB_SYNC])
        {
            for (i = 0; i < context->free_event_query_count; ++i)
            {
                GL_EXTCALL(glDeleteSync(context->free_event_queries[i].sync));
            }
        }
        else if (gl_info->supported[APPLE_FENCE])
        {
            for (i = 0; i < context->free_event_query_count; ++i)
            {
                GL_EXTCALL(glDeleteFencesAPPLE(1, &context->free_event_queries[i].id));
            }
        }
        else if (gl_info->supported[NV_FENCE])
        {
            for (i = 0; i < context->free_event_query_count; ++i)
            {
                GL_EXTCALL(glDeleteFencesNV(1, &context->free_event_queries[i].id));
            }
        }

        checkGLcall("context cleanup");
    }

    HeapFree(GetProcessHeap(), 0, context->free_occlusion_queries);
    HeapFree(GetProcessHeap(), 0, context->free_event_queries);

    if (restore_ctx)
    {
        context_restore_gl_context(gl_info, restore_dc, restore_ctx, restore_pf);
    }
    else if (wglGetCurrentContext() && !wglMakeCurrent(NULL, NULL))
    {
        ERR("Failed to disable GL context.\n");
    }

    ReleaseDC(context->win_handle, context->hdc);

    if (!wglDeleteContext(context->glCtx))
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

/* Do not call while under the GL lock. */
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
            HeapFree(GetProcessHeap(), 0, (void *)old->gl_info);
            HeapFree(GetProcessHeap(), 0, old);
        }
        else
        {
            old->current = 0;
        }
    }

    if (ctx)
    {
        if (!ctx->valid)
        {
            ERR("Trying to make invalid context %p current\n", ctx);
            return FALSE;
        }

        TRACE("Switching to D3D context %p, GL context %p, device context %p.\n", ctx, ctx->glCtx, ctx->hdc);
        if (!context_set_gl_context(ctx))
            return FALSE;
        ctx->current = 1;
    }
    else if(wglGetCurrentContext())
    {
        TRACE("Clearing current D3D context.\n");
        if (!wglMakeCurrent(NULL, NULL))
        {
            DWORD err = GetLastError();
            ERR("Failed to clear current GL context, last error %#x.\n", err);
            TlsSetValue(wined3d_context_tls_idx, NULL);
            return FALSE;
        }
    }

    return TlsSetValue(wined3d_context_tls_idx, ctx);
}

void context_release(struct wined3d_context *context)
{
    TRACE("Releasing context %p, level %u.\n", context, context->level);

    if (WARN_ON(d3d))
    {
        if (!context->level)
            WARN("Context %p is not active.\n", context);
        else if (context != context_get_current())
            WARN("Context %p is not the current context.\n", context);
    }

    if (!--context->level && context->restore_ctx)
    {
        TRACE("Restoring GL context %p on device context %p.\n", context->restore_ctx, context->restore_dc);
        context_restore_gl_context(context->gl_info, context->restore_dc, context->restore_ctx, context->restore_pf);
        context->restore_ctx = NULL;
        context->restore_dc = NULL;
    }
}

static void context_enter(struct wined3d_context *context)
{
    TRACE("Entering context %p, level %u.\n", context, context->level + 1);

    if (!context->level++)
    {
        const struct wined3d_context *current_context = context_get_current();
        HGLRC current_gl = wglGetCurrentContext();

        if (current_gl && (!current_context || current_context->glCtx != current_gl))
        {
            TRACE("Another GL context (%p on device context %p) is already current.\n",
                    current_gl, wglGetCurrentDC());
            context->restore_ctx = current_gl;
            context->restore_dc = wglGetCurrentDC();
            context->restore_pf = GetPixelFormat(context->restore_dc);
        }
    }
}

void context_invalidate_state(struct wined3d_context *context, DWORD state)
{
    DWORD rep = context->state_table[state].representative;
    DWORD idx;
    BYTE shift;

    if (isStateDirty(context, rep)) return;

    context->dirtyArray[context->numDirtyEntries++] = rep;
    idx = rep / (sizeof(*context->isStateDirty) * CHAR_BIT);
    shift = rep & ((sizeof(*context->isStateDirty) * CHAR_BIT) - 1);
    context->isStateDirty[idx] |= (1 << shift);
}

/* This function takes care of wined3d pixel format selection. */
static int context_choose_pixel_format(const struct wined3d_device *device, HDC hdc,
        const struct wined3d_format *color_format, const struct wined3d_format *ds_format,
        BOOL auxBuffers, BOOL findCompatible)
{
    int iPixelFormat=0;
    BYTE redBits, greenBits, blueBits, alphaBits, colorBits;
    BYTE depthBits=0, stencilBits=0;
    unsigned int current_value;
    unsigned int cfg_count = device->adapter->cfg_count;
    unsigned int i;

    TRACE("device %p, dc %p, color_format %s, ds_format %s, aux_buffers %#x, find_compatible %#x.\n",
            device, hdc, debug_d3dformat(color_format->id), debug_d3dformat(ds_format->id),
            auxBuffers, findCompatible);

    if (!getColorBits(color_format, &redBits, &greenBits, &blueBits, &alphaBits, &colorBits))
    {
        ERR("Unable to get color bits for format %s (%#x)!\n",
                debug_d3dformat(color_format->id), color_format->id);
        return 0;
    }

    getDepthStencilBits(ds_format, &depthBits, &stencilBits);

    current_value = 0;
    for (i = 0; i < cfg_count; ++i)
    {
        const struct wined3d_pixel_format *cfg = &device->adapter->cfgs[i];
        unsigned int value;

        /* For now only accept RGBA formats. Perhaps some day we will
         * allow floating point formats for pbuffers. */
        if (cfg->iPixelType != WGL_TYPE_RGBA_ARB)
            continue;
        /* In window mode we need a window drawable format and double buffering. */
        if (!(cfg->windowDrawable && cfg->doubleBuffer))
            continue;
        if (cfg->redSize < redBits)
            continue;
        if (cfg->greenSize < greenBits)
            continue;
        if (cfg->blueSize < blueBits)
            continue;
        if (cfg->alphaSize < alphaBits)
            continue;
        if (cfg->depthSize < depthBits)
            continue;
        if (stencilBits && cfg->stencilSize != stencilBits)
            continue;
        /* Check multisampling support. */
        if (cfg->numSamples)
            continue;

        value = 1;
        /* We try to locate a format which matches our requirements exactly. In case of
         * depth it is no problem to emulate 16-bit using e.g. 24-bit, so accept that. */
        if (cfg->depthSize == depthBits)
            value += 1;
        if (cfg->stencilSize == stencilBits)
            value += 2;
        if (cfg->alphaSize == alphaBits)
            value += 4;
        /* We like to have aux buffers in backbuffer mode */
        if (auxBuffers && cfg->auxBuffers)
            value += 8;
        if (cfg->redSize == redBits
                && cfg->greenSize == greenBits
                && cfg->blueSize == blueBits)
            value += 16;

        if (value > current_value)
        {
            iPixelFormat = cfg->iPixelFormat;
            current_value = value;
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
            iPixelFormat, debug_d3dformat(color_format->id), debug_d3dformat(ds_format->id));
    return iPixelFormat;
}

/* Context activation is done by the caller. */
static void bind_dummy_textures(const struct wined3d_device *device, const struct wined3d_context *context)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    unsigned int i, count = min(MAX_COMBINED_SAMPLERS, gl_info->limits.combined_samplers);

    for (i = 0; i < count; ++i)
    {
        GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0_ARB + i));
        checkGLcall("glActiveTextureARB");

        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, device->dummy_texture_2d[i]);
        checkGLcall("glBindTexture");

        if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
        {
            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_RECTANGLE_ARB, device->dummy_texture_rect[i]);
            checkGLcall("glBindTexture");
        }

        if (gl_info->supported[EXT_TEXTURE3D])
        {
            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_3D, device->dummy_texture_3d[i]);
            checkGLcall("glBindTexture");
        }

        if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
        {
            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_CUBE_MAP, device->dummy_texture_cube[i]);
            checkGLcall("glBindTexture");
        }
    }
}

/* Do not call while under the GL lock. */
struct wined3d_context *context_create(struct wined3d_swapchain *swapchain,
        struct wined3d_surface *target, const struct wined3d_format *ds_format)
{
    struct wined3d_device *device = swapchain->device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    const struct wined3d_format *color_format;
    struct wined3d_context *ret;
    BOOL auxBuffers = FALSE;
    int pixel_format;
    unsigned int s;
    int swap_interval;
    DWORD state;
    HGLRC ctx;
    HDC hdc;

    TRACE("swapchain %p, target %p, window %p.\n", swapchain, target, swapchain->win_handle);

    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ret));
    if (!ret)
        return NULL;

    ret->blit_targets = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            gl_info->limits.buffers * sizeof(*ret->blit_targets));
    if (!ret->blit_targets)
        goto out;

    ret->draw_buffers = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            gl_info->limits.buffers * sizeof(*ret->draw_buffers));
    if (!ret->draw_buffers)
        goto out;

    ret->free_occlusion_query_size = 4;
    ret->free_occlusion_queries = HeapAlloc(GetProcessHeap(), 0,
            ret->free_occlusion_query_size * sizeof(*ret->free_occlusion_queries));
    if (!ret->free_occlusion_queries)
        goto out;

    list_init(&ret->occlusion_queries);

    ret->free_event_query_size = 4;
    ret->free_event_queries = HeapAlloc(GetProcessHeap(), 0,
            ret->free_event_query_size * sizeof(*ret->free_event_queries));
    if (!ret->free_event_queries)
        goto out;

    list_init(&ret->event_queries);
    list_init(&ret->fbo_list);
    list_init(&ret->fbo_destroy_list);

    if (!(hdc = GetDC(swapchain->win_handle)))
    {
        WARN("Failed to retireve device context, trying swapchain backup.\n");

        if (!(hdc = swapchain_get_backup_dc(swapchain)))
        {
            ERR("Failed to retrieve a device context.\n");
            goto out;
        }
    }

    color_format = target->resource.format;

    /* In case of ORM_BACKBUFFER, make sure to request an alpha component for
     * X4R4G4B4/X8R8G8B8 as we might need it for the backbuffer. */
    if (wined3d_settings.offscreen_rendering_mode == ORM_BACKBUFFER)
    {
        auxBuffers = TRUE;

        if (color_format->id == WINED3DFMT_B4G4R4X4_UNORM)
            color_format = wined3d_get_format(gl_info, WINED3DFMT_B4G4R4A4_UNORM);
        else if (color_format->id == WINED3DFMT_B8G8R8X8_UNORM)
            color_format = wined3d_get_format(gl_info, WINED3DFMT_B8G8R8A8_UNORM);
    }

    /* DirectDraw supports 8bit paletted render targets and these are used by
     * old games like StarCraft and C&C. Most modern hardware doesn't support
     * 8bit natively so we perform some form of 8bit -> 32bit conversion. The
     * conversion (ab)uses the alpha component for storing the palette index.
     * For this reason we require a format with 8bit alpha, so request
     * A8R8G8B8. */
    if (color_format->id == WINED3DFMT_P8_UINT)
        color_format = wined3d_get_format(gl_info, WINED3DFMT_B8G8R8A8_UNORM);

    /* Try to find a pixel format which matches our requirements. */
    pixel_format = context_choose_pixel_format(device, hdc, color_format, ds_format, auxBuffers, FALSE);

    /* Try to locate a compatible format if we weren't able to find anything. */
    if (!pixel_format)
    {
        TRACE("Trying to locate a compatible pixel format because an exact match failed.\n");
        pixel_format = context_choose_pixel_format(device, hdc, color_format, ds_format, auxBuffers, TRUE);
    }

    /* If we still don't have a pixel format, something is very wrong as ChoosePixelFormat barely fails */
    if (!pixel_format)
    {
        ERR("Can't find a suitable pixel format.\n");
        goto out;
    }

    context_enter(ret);

    if (!context_set_pixel_format(gl_info, hdc, pixel_format))
    {
        ERR("Failed to set pixel format %d on device context %p.\n", pixel_format, hdc);
        context_release(ret);
        goto out;
    }

    if (!(ctx = wglCreateContext(hdc)))
    {
        ERR("Failed to create a WGL context.\n");
        context_release(ret);
        goto out;
    }

    if (device->context_count)
    {
        if (!wglShareLists(device->contexts[0]->glCtx, ctx))
        {
            ERR("wglShareLists(%p, %p) failed, last error %#x.\n",
                    device->contexts[0]->glCtx, ctx, GetLastError());
            context_release(ret);
            if (!wglDeleteContext(ctx))
                ERR("wglDeleteContext(%p) failed, last error %#x.\n", ctx, GetLastError());
            goto out;
        }
    }

    if (!device_context_add(device, ret))
    {
        ERR("Failed to add the newly created context to the context list\n");
        context_release(ret);
        if (!wglDeleteContext(ctx))
            ERR("wglDeleteContext(%p) failed, last error %#x.\n", ctx, GetLastError());
        goto out;
    }

    ret->gl_info = gl_info;
    ret->state_table = device->StateTable;

    /* Mark all states dirty to force a proper initialization of the states
     * on the first use of the context. */
    for (state = 0; state <= STATE_HIGHEST; ++state)
    {
        if (ret->state_table[state].representative)
            context_invalidate_state(ret, state);
    }

    ret->swapchain = swapchain;
    ret->current_rt = target;
    ret->tid = GetCurrentThreadId();

    ret->render_offscreen = surface_is_offscreen(target);
    ret->draw_buffers_mask = context_generate_rt_mask(GL_BACK);
    ret->valid = 1;

    ret->glCtx = ctx;
    ret->win_handle = swapchain->win_handle;
    ret->hdc = hdc;
    ret->pixel_format = pixel_format;

    /* Set up the context defaults */
    if (!context_set_current(ret))
    {
        ERR("Cannot activate context to set up defaults.\n");
        device_context_remove(device, ret);
        context_release(ret);
        if (!wglDeleteContext(ctx))
            ERR("wglDeleteContext(%p) failed, last error %#x.\n", ctx, GetLastError());
        goto out;
    }

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
            FIXME("Unknown swap interval %#x.\n", swapchain->desc.swap_interval);
            swap_interval = 1;
    }

    if (gl_info->supported[WGL_EXT_SWAP_CONTROL])
    {
        if (!GL_EXTCALL(wglSwapIntervalEXT(swap_interval)))
            ERR("wglSwapIntervalEXT failed to set swap interval %d for context %p, last error %#x\n",
                swap_interval, ret, GetLastError());
    }

    gl_info->gl_ops.gl.p_glGetIntegerv(GL_AUX_BUFFERS, &ret->aux_buffers);

    TRACE("Setting up the screen\n");
    /* Clear the screen */
    gl_info->gl_ops.gl.p_glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
    checkGLcall("glClearColor");
    gl_info->gl_ops.gl.p_glClearIndex(0);
    gl_info->gl_ops.gl.p_glClearDepth(1);
    gl_info->gl_ops.gl.p_glClearStencil(0xffff);

    checkGLcall("glClear");

    gl_info->gl_ops.gl.p_glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    checkGLcall("glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);");

    gl_info->gl_ops.gl.p_glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    checkGLcall("glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);");

    gl_info->gl_ops.gl.p_glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
    checkGLcall("glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);");

    gl_info->gl_ops.gl.p_glPixelStorei(GL_PACK_ALIGNMENT, device->surface_alignment);
    checkGLcall("glPixelStorei(GL_PACK_ALIGNMENT, device->surface_alignment);");
    gl_info->gl_ops.gl.p_glPixelStorei(GL_UNPACK_ALIGNMENT, device->surface_alignment);
    checkGLcall("glPixelStorei(GL_UNPACK_ALIGNMENT, device->surface_alignment);");

    if (gl_info->supported[APPLE_CLIENT_STORAGE])
    {
        /* Most textures will use client storage if supported. Exceptions are
         * non-native power of 2 textures and textures in DIB sections. */
        gl_info->gl_ops.gl.p_glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE);
        checkGLcall("glPixelStorei(GL_UNPACK_CLIENT_STORAGE_APPLE, GL_TRUE)");
    }
    if (gl_info->supported[ARB_VERTEX_BLEND])
    {
        /* Direct3D always uses n-1 weights for n world matrices and uses
         * 1 - sum for the last one this is equal to GL_WEIGHT_SUM_UNITY_ARB.
         * Enabling it doesn't do anything unless GL_VERTEX_BLEND_ARB isn't
         * enabled as well. */
        gl_info->gl_ops.gl.p_glEnable(GL_WEIGHT_SUM_UNITY_ARB);
        checkGLcall("glEnable(GL_WEIGHT_SUM_UNITY_ARB)");
    }
    if (gl_info->supported[NV_TEXTURE_SHADER2])
    {
        /* Set up the previous texture input for all shader units. This applies to bump mapping, and in d3d
         * the previous texture where to source the offset from is always unit - 1.
         */
        for (s = 1; s < gl_info->limits.textures; ++s)
        {
            context_active_texture(ret, gl_info, s);
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_SHADER_NV,
                    GL_PREVIOUS_TEXTURE_INPUT_NV, GL_TEXTURE0_ARB + s - 1);
            checkGLcall("glTexEnvi(GL_TEXTURE_SHADER_NV, GL_PREVIOUS_TEXTURE_INPUT_NV, ...");
        }
    }
    if (gl_info->supported[ARB_FRAGMENT_PROGRAM])
    {
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

    if (gl_info->supported[ARB_POINT_SPRITE])
    {
        for (s = 0; s < gl_info->limits.textures; ++s)
        {
            context_active_texture(ret, gl_info, s);
            gl_info->gl_ops.gl.p_glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
            checkGLcall("glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE)");
        }
    }

    if (gl_info->supported[ARB_PROVOKING_VERTEX])
    {
        GL_EXTCALL(glProvokingVertex(GL_FIRST_VERTEX_CONVENTION));
    }
    else if (gl_info->supported[EXT_PROVOKING_VERTEX])
    {
        GL_EXTCALL(glProvokingVertexEXT(GL_FIRST_VERTEX_CONVENTION_EXT));
    }
    ret->select_shader = 1;

    /* If this happens to be the first context for the device, dummy textures
     * are not created yet. In that case, they will be created (and bound) by
     * create_dummy_textures right after this context is initialized. */
    if (device->dummy_texture_2d[0])
        bind_dummy_textures(device, ret);

    TRACE("Created context %p.\n", ret);

    return ret;

out:
    HeapFree(GetProcessHeap(), 0, ret->free_event_queries);
    HeapFree(GetProcessHeap(), 0, ret->free_occlusion_queries);
    HeapFree(GetProcessHeap(), 0, ret->draw_buffers);
    HeapFree(GetProcessHeap(), 0, ret->blit_targets);
    HeapFree(GetProcessHeap(), 0, ret);
    return NULL;
}

/* Do not call while under the GL lock. */
void context_destroy(struct wined3d_device *device, struct wined3d_context *context)
{
    BOOL destroy;

    TRACE("Destroying ctx %p\n", context);

    if (context->tid == GetCurrentThreadId() || !context->current)
    {
        context_destroy_gl_resources(context);
        TlsSetValue(wined3d_context_tls_idx, NULL);
        destroy = TRUE;
    }
    else
    {
        /* Make a copy of gl_info for context_destroy_gl_resources use, the one
           in wined3d_adapter may go away in the meantime */
        struct wined3d_gl_info *gl_info = HeapAlloc(GetProcessHeap(), 0, sizeof(*gl_info));
        *gl_info = *context->gl_info;
        context->gl_info = gl_info;
        context->destroyed = 1;
        destroy = FALSE;
    }

    HeapFree(GetProcessHeap(), 0, context->draw_buffers);
    HeapFree(GetProcessHeap(), 0, context->blit_targets);
    device_context_remove(device, context);
    if (destroy) HeapFree(GetProcessHeap(), 0, context);
}

/* Context activation is done by the caller. */
static void set_blit_dimension(const struct wined3d_gl_info *gl_info, UINT width, UINT height)
{
    const GLdouble projection[] =
    {
        2.0 / width,          0.0,  0.0, 0.0,
                0.0, 2.0 / height,  0.0, 0.0,
                0.0,          0.0,  2.0, 0.0,
               -1.0,         -1.0, -1.0, 1.0,
    };

    gl_info->gl_ops.gl.p_glMatrixMode(GL_PROJECTION);
    checkGLcall("glMatrixMode(GL_PROJECTION)");
    gl_info->gl_ops.gl.p_glLoadMatrixd(projection);
    checkGLcall("glLoadMatrixd");
    gl_info->gl_ops.gl.p_glViewport(0, 0, width, height);
    checkGLcall("glViewport");
}

static void context_get_rt_size(const struct wined3d_context *context, SIZE *size)
{
    const struct wined3d_surface *rt = context->current_rt;

    if (rt->container.type == WINED3D_CONTAINER_SWAPCHAIN
            && rt->container.u.swapchain->front_buffer == rt)
    {
        RECT window_size;

        GetClientRect(context->win_handle, &window_size);
        size->cx = window_size.right - window_size.left;
        size->cy = window_size.bottom - window_size.top;

        return;
    }

    size->cx = rt->resource.width;
    size->cy = rt->resource.height;
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
 *
 *****************************************************************************/
/* Context activation is done by the caller. */
static void SetupForBlit(const struct wined3d_device *device, struct wined3d_context *context)
{
    int i;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    DWORD sampler;
    SIZE rt_size;

    TRACE("Setting up context %p for blitting\n", context);

    context_get_rt_size(context, &rt_size);

    if (context->last_was_blit)
    {
        if (context->blit_w != rt_size.cx || context->blit_h != rt_size.cy)
        {
            set_blit_dimension(gl_info, rt_size.cx, rt_size.cy);
            context->blit_w = rt_size.cx;
            context->blit_h = rt_size.cy;
            /* No need to dirtify here, the states are still dirtified because
             * they weren't applied since the last SetupForBlit() call. */
        }
        TRACE("Context is already set up for blitting, nothing to do\n");
        return;
    }
    context->last_was_blit = TRUE;

    /* Disable all textures. The caller can then bind a texture it wants to blit
     * from
     *
     * The blitting code uses (for now) the fixed function pipeline, so make sure to reset all fixed
     * function texture unit. No need to care for higher samplers
     */
    for (i = gl_info->limits.textures - 1; i > 0 ; --i)
    {
        sampler = device->rev_tex_unit_map[i];
        context_active_texture(context, gl_info, i);

        if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
        {
            gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_CUBE_MAP_ARB);
            checkGLcall("glDisable GL_TEXTURE_CUBE_MAP_ARB");
        }
        gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_3D);
        checkGLcall("glDisable GL_TEXTURE_3D");
        if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
        {
            gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_RECTANGLE_ARB);
            checkGLcall("glDisable GL_TEXTURE_RECTANGLE_ARB");
        }
        gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_2D);
        checkGLcall("glDisable GL_TEXTURE_2D");

        gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
        checkGLcall("glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);");

        if (sampler != WINED3D_UNMAPPED_STAGE)
        {
            if (sampler < MAX_TEXTURES)
                context_invalidate_state(context, STATE_TEXTURESTAGE(sampler, WINED3D_TSS_COLOR_OP));
            context_invalidate_state(context, STATE_SAMPLER(sampler));
        }
    }
    context_active_texture(context, gl_info, 0);

    sampler = device->rev_tex_unit_map[0];

    if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
    {
        gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_CUBE_MAP_ARB);
        checkGLcall("glDisable GL_TEXTURE_CUBE_MAP_ARB");
    }
    gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_3D);
    checkGLcall("glDisable GL_TEXTURE_3D");
    if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
    {
        gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_RECTANGLE_ARB);
        checkGLcall("glDisable GL_TEXTURE_RECTANGLE_ARB");
    }
    gl_info->gl_ops.gl.p_glDisable(GL_TEXTURE_2D);
    checkGLcall("glDisable GL_TEXTURE_2D");

    gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    gl_info->gl_ops.gl.p_glMatrixMode(GL_TEXTURE);
    checkGLcall("glMatrixMode(GL_TEXTURE)");
    gl_info->gl_ops.gl.p_glLoadIdentity();
    checkGLcall("glLoadIdentity()");

    if (gl_info->supported[EXT_TEXTURE_LOD_BIAS])
    {
        gl_info->gl_ops.gl.p_glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT,
                GL_TEXTURE_LOD_BIAS_EXT, 0.0f);
        checkGLcall("glTexEnvf GL_TEXTURE_LOD_BIAS_EXT ...");
    }

    if (sampler != WINED3D_UNMAPPED_STAGE)
    {
        if (sampler < MAX_TEXTURES)
        {
            context_invalidate_state(context, STATE_TRANSFORM(WINED3D_TS_TEXTURE0 + sampler));
            context_invalidate_state(context, STATE_TEXTURESTAGE(sampler, WINED3D_TSS_COLOR_OP));
        }
        context_invalidate_state(context, STATE_SAMPLER(sampler));
    }

    /* Other misc states */
    gl_info->gl_ops.gl.p_glDisable(GL_ALPHA_TEST);
    checkGLcall("glDisable(GL_ALPHA_TEST)");
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_ALPHATESTENABLE));
    gl_info->gl_ops.gl.p_glDisable(GL_LIGHTING);
    checkGLcall("glDisable GL_LIGHTING");
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_LIGHTING));
    gl_info->gl_ops.gl.p_glDisable(GL_DEPTH_TEST);
    checkGLcall("glDisable GL_DEPTH_TEST");
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_ZENABLE));
    glDisableWINE(GL_FOG);
    checkGLcall("glDisable GL_FOG");
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_FOGENABLE));
    gl_info->gl_ops.gl.p_glDisable(GL_BLEND);
    checkGLcall("glDisable GL_BLEND");
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_ALPHABLENDENABLE));
    gl_info->gl_ops.gl.p_glDisable(GL_CULL_FACE);
    checkGLcall("glDisable GL_CULL_FACE");
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_CULLMODE));
    gl_info->gl_ops.gl.p_glDisable(GL_STENCIL_TEST);
    checkGLcall("glDisable GL_STENCIL_TEST");
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_STENCILENABLE));
    gl_info->gl_ops.gl.p_glDisable(GL_SCISSOR_TEST);
    checkGLcall("glDisable GL_SCISSOR_TEST");
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_SCISSORTESTENABLE));
    if (gl_info->supported[ARB_POINT_SPRITE])
    {
        gl_info->gl_ops.gl.p_glDisable(GL_POINT_SPRITE_ARB);
        checkGLcall("glDisable GL_POINT_SPRITE_ARB");
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_POINTSPRITEENABLE));
    }
    gl_info->gl_ops.gl.p_glColorMask(GL_TRUE, GL_TRUE,GL_TRUE,GL_TRUE);
    checkGLcall("glColorMask");
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITEENABLE));
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITEENABLE1));
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITEENABLE2));
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITEENABLE3));
    if (gl_info->supported[EXT_SECONDARY_COLOR])
    {
        gl_info->gl_ops.gl.p_glDisable(GL_COLOR_SUM_EXT);
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_SPECULARENABLE));
        checkGLcall("glDisable(GL_COLOR_SUM_EXT)");
    }

    /* Setup transforms */
    gl_info->gl_ops.gl.p_glMatrixMode(GL_MODELVIEW);
    checkGLcall("glMatrixMode(GL_MODELVIEW)");
    gl_info->gl_ops.gl.p_glLoadIdentity();
    checkGLcall("glLoadIdentity()");
    context_invalidate_state(context, STATE_TRANSFORM(WINED3D_TS_WORLD_MATRIX(0)));

    context->last_was_rhw = TRUE;
    context_invalidate_state(context, STATE_VDECL); /* because of last_was_rhw = TRUE */

    gl_info->gl_ops.gl.p_glDisable(GL_CLIP_PLANE0); checkGLcall("glDisable(clip plane 0)");
    gl_info->gl_ops.gl.p_glDisable(GL_CLIP_PLANE1); checkGLcall("glDisable(clip plane 1)");
    gl_info->gl_ops.gl.p_glDisable(GL_CLIP_PLANE2); checkGLcall("glDisable(clip plane 2)");
    gl_info->gl_ops.gl.p_glDisable(GL_CLIP_PLANE3); checkGLcall("glDisable(clip plane 3)");
    gl_info->gl_ops.gl.p_glDisable(GL_CLIP_PLANE4); checkGLcall("glDisable(clip plane 4)");
    gl_info->gl_ops.gl.p_glDisable(GL_CLIP_PLANE5); checkGLcall("glDisable(clip plane 5)");
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_CLIPPING));

    set_blit_dimension(gl_info, rt_size.cx, rt_size.cy);

    /* Disable shaders */
    device->shader_backend->shader_select(context, WINED3D_SHADER_MODE_NONE, WINED3D_SHADER_MODE_NONE);
    context->select_shader = 1;
    context->load_constants = 1;

    context->blit_w = rt_size.cx;
    context->blit_h = rt_size.cy;
    context_invalidate_state(context, STATE_VIEWPORT);
    context_invalidate_state(context, STATE_TRANSFORM(WINED3D_TS_PROJECTION));
}

static inline BOOL is_rt_mask_onscreen(DWORD rt_mask)
{
    return rt_mask & (1 << 31);
}

static inline GLenum draw_buffer_from_rt_mask(DWORD rt_mask)
{
    return rt_mask & ~(1 << 31);
}

/* Context activation is done by the caller. */
static void context_apply_draw_buffers(struct wined3d_context *context, DWORD rt_mask)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    if (!rt_mask)
    {
        gl_info->gl_ops.gl.p_glDrawBuffer(GL_NONE);
        checkGLcall("glDrawBuffer()");
    }
    else if (is_rt_mask_onscreen(rt_mask))
    {
        gl_info->gl_ops.gl.p_glDrawBuffer(draw_buffer_from_rt_mask(rt_mask));
        checkGLcall("glDrawBuffer()");
    }
    else
    {
        if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
        {
            unsigned int i = 0;

            while (rt_mask)
            {
                if (rt_mask & 1)
                    context->draw_buffers[i] = GL_COLOR_ATTACHMENT0 + i;
                else
                    context->draw_buffers[i] = GL_NONE;

                rt_mask >>= 1;
                ++i;
            }

            if (gl_info->supported[ARB_DRAW_BUFFERS])
            {
                GL_EXTCALL(glDrawBuffersARB(i, context->draw_buffers));
                checkGLcall("glDrawBuffers()");
            }
            else
            {
                gl_info->gl_ops.gl.p_glDrawBuffer(context->draw_buffers[0]);
                checkGLcall("glDrawBuffer()");
            }
        }
        else
        {
            ERR("Unexpected draw buffers mask with backbuffer ORM.\n");
        }
    }
}

/* Context activation is done by the caller. */
void context_set_draw_buffer(struct wined3d_context *context, GLenum buffer)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    gl_info->gl_ops.gl.p_glDrawBuffer(buffer);
    checkGLcall("glDrawBuffer()");
    if (context->current_fbo)
        context->current_fbo->rt_mask = context_generate_rt_mask(buffer);
    else
        context->draw_buffers_mask = context_generate_rt_mask(buffer);
}

/* Context activation is done by the caller. */
void context_active_texture(struct wined3d_context *context, const struct wined3d_gl_info *gl_info, unsigned int unit)
{
    GL_EXTCALL(glActiveTextureARB(GL_TEXTURE0 + unit));
    checkGLcall("glActiveTextureARB");
    context->active_texture = unit;
}

void context_bind_texture(struct wined3d_context *context, GLenum target, GLuint name)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    DWORD unit = context->active_texture;
    DWORD old_texture_type = context->texture_type[unit];

    if (name)
    {
        gl_info->gl_ops.gl.p_glBindTexture(target, name);
        checkGLcall("glBindTexture");
    }
    else
    {
        target = GL_NONE;
    }

    if (old_texture_type != target)
    {
        const struct wined3d_device *device = context->swapchain->device;

        switch (old_texture_type)
        {
            case GL_NONE:
                /* nothing to do */
                break;
            case GL_TEXTURE_2D:
                gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, device->dummy_texture_2d[unit]);
                checkGLcall("glBindTexture");
                break;
            case GL_TEXTURE_RECTANGLE_ARB:
                gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_RECTANGLE_ARB, device->dummy_texture_rect[unit]);
                checkGLcall("glBindTexture");
                break;
            case GL_TEXTURE_CUBE_MAP:
                gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_CUBE_MAP, device->dummy_texture_cube[unit]);
                checkGLcall("glBindTexture");
                break;
            case GL_TEXTURE_3D:
                gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_3D, device->dummy_texture_3d[unit]);
                checkGLcall("glBindTexture");
                break;
            default:
                ERR("Unexpected texture target %#x\n", old_texture_type);
        }

        context->texture_type[unit] = target;
    }
}

static void context_set_render_offscreen(struct wined3d_context *context, BOOL offscreen)
{
    if (context->render_offscreen == offscreen) return;

    context_invalidate_state(context, STATE_POINTSPRITECOORDORIGIN);
    context_invalidate_state(context, STATE_TRANSFORM(WINED3D_TS_PROJECTION));
    context_invalidate_state(context, STATE_VIEWPORT);
    context_invalidate_state(context, STATE_SCISSORRECT);
    context_invalidate_state(context, STATE_FRONTFACE);
    context->render_offscreen = offscreen;
}

static BOOL match_depth_stencil_format(const struct wined3d_format *existing,
        const struct wined3d_format *required)
{
    BYTE existing_depth, existing_stencil, required_depth, required_stencil;

    if (existing == required) return TRUE;
    if ((existing->flags & WINED3DFMT_FLAG_FLOAT) != (required->flags & WINED3DFMT_FLAG_FLOAT)) return FALSE;

    getDepthStencilBits(existing, &existing_depth, &existing_stencil);
    getDepthStencilBits(required, &required_depth, &required_stencil);

    if(existing_depth < required_depth) return FALSE;
    /* If stencil bits are used the exact amount is required - otherwise wrapping
     * won't work correctly */
    if(required_stencil && required_stencil != existing_stencil) return FALSE;
    return TRUE;
}

/* The caller provides a context */
static void context_validate_onscreen_formats(struct wined3d_context *context,
        const struct wined3d_surface *depth_stencil)
{
    /* Onscreen surfaces are always in a swapchain */
    struct wined3d_swapchain *swapchain = context->current_rt->container.u.swapchain;

    if (context->render_offscreen || !depth_stencil) return;
    if (match_depth_stencil_format(swapchain->ds_format, depth_stencil->resource.format)) return;

    /* TODO: If the requested format would satisfy the needs of the existing one(reverse match),
     * or no onscreen depth buffer was created, the OpenGL drawable could be changed to the new
     * format. */
    WARN("Depth stencil format is not supported by WGL, rendering the backbuffer in an FBO\n");

    /* The currently active context is the necessary context to access the swapchain's onscreen buffers */
    surface_load_location(context->current_rt, SFLAG_INTEXTURE, NULL);
    swapchain->render_to_fbo = TRUE;
    swapchain_update_draw_bindings(swapchain);
    context_set_render_offscreen(context, TRUE);
}

static DWORD context_generate_rt_mask_no_fbo(const struct wined3d_device *device, const struct wined3d_surface *rt)
{
    if (!rt || rt->resource.format->id == WINED3DFMT_NULL)
        return 0;
    else if (rt->container.type == WINED3D_CONTAINER_SWAPCHAIN)
        return context_generate_rt_mask_from_surface(rt);
    else
        return context_generate_rt_mask(device->offscreenBuffer);
}

/* Context activation is done by the caller. */
void context_apply_blit_state(struct wined3d_context *context, const struct wined3d_device *device)
{
    struct wined3d_surface *rt = context->current_rt;
    DWORD rt_mask, *cur_mask;

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
    {
        context_validate_onscreen_formats(context, NULL);

        if (context->render_offscreen)
        {
            surface_internal_preload(rt, SRGB_RGB);

            context_apply_fbo_state_blit(context, GL_FRAMEBUFFER, rt, NULL, rt->draw_binding);
            if (rt->resource.format->id != WINED3DFMT_NULL)
                rt_mask = 1;
            else
                rt_mask = 0;
        }
        else
        {
            context_bind_fbo(context, GL_FRAMEBUFFER, NULL);
            rt_mask = context_generate_rt_mask_from_surface(rt);
        }
    }
    else
    {
        rt_mask = context_generate_rt_mask_no_fbo(device, rt);
    }

    cur_mask = context->current_fbo ? &context->current_fbo->rt_mask : &context->draw_buffers_mask;

    if (rt_mask != *cur_mask)
    {
        context_apply_draw_buffers(context, rt_mask);
        *cur_mask = rt_mask;
    }

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
    {
        context_check_fbo_status(context, GL_FRAMEBUFFER);
    }

    SetupForBlit(device, context);
    context_invalidate_state(context, STATE_FRAMEBUFFER);
}

static BOOL context_validate_rt_config(UINT rt_count,
        struct wined3d_surface * const *rts, const struct wined3d_surface *ds)
{
    unsigned int i;

    if (ds) return TRUE;

    for (i = 0; i < rt_count; ++i)
    {
        if (rts[i] && rts[i]->resource.format->id != WINED3DFMT_NULL)
            return TRUE;
    }

    WARN("Invalid render target config, need at least one attachment.\n");
    return FALSE;
}

/* Context activation is done by the caller. */
BOOL context_apply_clear_state(struct wined3d_context *context, const struct wined3d_device *device,
        UINT rt_count, const struct wined3d_fb_state *fb)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    DWORD rt_mask = 0, *cur_mask;
    UINT i;
    struct wined3d_surface **rts = fb->render_targets;

    if (isStateDirty(context, STATE_FRAMEBUFFER) || fb != &device->fb
            || rt_count != context->gl_info->limits.buffers)
    {
        if (!context_validate_rt_config(rt_count, rts, fb->depth_stencil))
            return FALSE;

        if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
        {
            context_validate_onscreen_formats(context, fb->depth_stencil);

            if (!rt_count || surface_is_offscreen(rts[0]))
            {
                for (i = 0; i < rt_count; ++i)
                {
                    context->blit_targets[i] = rts[i];
                    if (rts[i] && rts[i]->resource.format->id != WINED3DFMT_NULL)
                        rt_mask |= (1 << i);
                }
                while (i < context->gl_info->limits.buffers)
                {
                    context->blit_targets[i] = NULL;
                    ++i;
                }
                context_apply_fbo_state(context, GL_FRAMEBUFFER, context->blit_targets, fb->depth_stencil,
                        rt_count ? rts[0]->draw_binding : SFLAG_INTEXTURE);
                gl_info->gl_ops.gl.p_glReadBuffer(GL_NONE);
                checkGLcall("glReadBuffer");
            }
            else
            {
                context_apply_fbo_state(context, GL_FRAMEBUFFER, NULL, NULL, SFLAG_INDRAWABLE);
                rt_mask = context_generate_rt_mask_from_surface(rts[0]);
            }

            /* If the framebuffer is not the device's fb the device's fb has to be reapplied
             * next draw. Otherwise we could mark the framebuffer state clean here, once the
             * state management allows this */
            context_invalidate_state(context, STATE_FRAMEBUFFER);
        }
        else
        {
            rt_mask = context_generate_rt_mask_no_fbo(device, rt_count ? rts[0] : NULL);
        }
    }
    else if (wined3d_settings.offscreen_rendering_mode == ORM_FBO
            && (!rt_count || surface_is_offscreen(rts[0])))
    {
        for (i = 0; i < rt_count; ++i)
        {
            if (rts[i] && rts[i]->resource.format->id != WINED3DFMT_NULL) rt_mask |= (1 << i);
        }
    }
    else
    {
        rt_mask = context_generate_rt_mask_no_fbo(device, rt_count ? rts[0] : NULL);
    }

    cur_mask = context->current_fbo ? &context->current_fbo->rt_mask : &context->draw_buffers_mask;

    if (rt_mask != *cur_mask)
    {
        context_apply_draw_buffers(context, rt_mask);
        *cur_mask = rt_mask;
        context_invalidate_state(context, STATE_FRAMEBUFFER);
    }

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
    {
        context_check_fbo_status(context, GL_FRAMEBUFFER);
    }

    if (context->last_was_blit)
        context->last_was_blit = FALSE;

    /* Blending and clearing should be orthogonal, but tests on the nvidia
     * driver show that disabling blending when clearing improves the clearing
     * performance incredibly. */
    gl_info->gl_ops.gl.p_glDisable(GL_BLEND);
    gl_info->gl_ops.gl.p_glEnable(GL_SCISSOR_TEST);
    checkGLcall("glEnable GL_SCISSOR_TEST");

    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_ALPHABLENDENABLE));
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_SCISSORTESTENABLE));
    context_invalidate_state(context, STATE_SCISSORRECT);

    return TRUE;
}

static DWORD find_draw_buffers_mask(const struct wined3d_context *context, const struct wined3d_device *device)
{
    const struct wined3d_state *state = &device->stateBlock->state;
    struct wined3d_surface **rts = state->fb->render_targets;
    struct wined3d_shader *ps = state->pixel_shader;
    DWORD rt_mask, rt_mask_bits;
    unsigned int i;

    if (wined3d_settings.offscreen_rendering_mode != ORM_FBO) return context_generate_rt_mask_no_fbo(device, rts[0]);
    else if (!context->render_offscreen) return context_generate_rt_mask_from_surface(rts[0]);

    rt_mask = ps ? ps->reg_maps.rt_mask : 1;
    rt_mask &= device->valid_rt_mask;
    rt_mask_bits = rt_mask;
    i = 0;
    while (rt_mask_bits)
    {
        rt_mask_bits &= ~(1 << i);
        if (!rts[i] || rts[i]->resource.format->id == WINED3DFMT_NULL)
            rt_mask &= ~(1 << i);

        i++;
    }

    return rt_mask;
}

/* Context activation is done by the caller. */
void context_state_fb(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_device *device = context->swapchain->device;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    const struct wined3d_fb_state *fb = state->fb;
    DWORD rt_mask = find_draw_buffers_mask(context, device);
    DWORD *cur_mask;

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
    {
        if (!context->render_offscreen)
        {
            context_apply_fbo_state(context, GL_FRAMEBUFFER, NULL, NULL, SFLAG_INDRAWABLE);
        }
        else
        {
            context_apply_fbo_state(context, GL_FRAMEBUFFER, fb->render_targets, fb->depth_stencil,
                    fb->render_targets[0]->draw_binding);
            gl_info->gl_ops.gl.p_glReadBuffer(GL_NONE);
            checkGLcall("glReadBuffer");
        }
    }

    cur_mask = context->current_fbo ? &context->current_fbo->rt_mask : &context->draw_buffers_mask;
    if (rt_mask != *cur_mask)
    {
        context_apply_draw_buffers(context, rt_mask);
        *cur_mask = rt_mask;
    }
}

/* Context activation is done by the caller. */
void context_state_drawbuf(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    const struct wined3d_device *device = context->swapchain->device;
    DWORD rt_mask, *cur_mask;

    if (isStateDirty(context, STATE_FRAMEBUFFER)) return;

    cur_mask = context->current_fbo ? &context->current_fbo->rt_mask : &context->draw_buffers_mask;
    rt_mask = find_draw_buffers_mask(context, device);
    if (rt_mask != *cur_mask)
    {
        context_apply_draw_buffers(context, rt_mask);
        *cur_mask = rt_mask;
    }
}

/* Context activation is done by the caller. */
BOOL context_apply_draw_state(struct wined3d_context *context, struct wined3d_device *device)
{
    const struct wined3d_state *state = &device->stateBlock->state;
    const struct StateEntry *state_table = context->state_table;
    const struct wined3d_fb_state *fb = state->fb;
    unsigned int i;

    if (!context_validate_rt_config(context->gl_info->limits.buffers,
            fb->render_targets, fb->depth_stencil))
        return FALSE;

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO && isStateDirty(context, STATE_FRAMEBUFFER))
    {
        context_validate_onscreen_formats(context, fb->depth_stencil);
    }

    /* Preload resources before FBO setup. Texture preload in particular may
     * result in changes to the current FBO, due to using e.g. FBO blits for
     * updating a resource location. */
    device_update_tex_unit_map(device);
    device_preload_textures(device);
    if (isStateDirty(context, STATE_VDECL) || isStateDirty(context, STATE_STREAMSRC))
        device_update_stream_info(device, context->gl_info);
    if (state->index_buffer)
    {
        if (device->strided_streams.all_vbo)
            wined3d_buffer_preload(state->index_buffer);
        else
            buffer_get_sysmem(state->index_buffer, context->gl_info);
    }

    for (i = 0; i < context->numDirtyEntries; ++i)
    {
        DWORD rep = context->dirtyArray[i];
        DWORD idx = rep / (sizeof(*context->isStateDirty) * CHAR_BIT);
        BYTE shift = rep & ((sizeof(*context->isStateDirty) * CHAR_BIT) - 1);
        context->isStateDirty[idx] &= ~(1 << shift);
        state_table[rep].apply(context, state, rep);
    }

    if (context->select_shader)
    {
        device->shader_backend->shader_select(context,
                use_vs(state) ? WINED3D_SHADER_MODE_SHADER : WINED3D_SHADER_MODE_FFP,
                use_ps(state) ? WINED3D_SHADER_MODE_SHADER : WINED3D_SHADER_MODE_FFP);
        context->select_shader = 0;
    }

    if (context->load_constants)
    {
        device->shader_backend->shader_load_constants(context, use_ps(state), use_vs(state));
        context->load_constants = 0;
    }

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
    {
        context_check_fbo_status(context, GL_FRAMEBUFFER);
    }

    context->numDirtyEntries = 0; /* This makes the whole list clean */
    context->last_was_blit = FALSE;

    return TRUE;
}

static void context_setup_target(struct wined3d_context *context, struct wined3d_surface *target)
{
    BOOL old_render_offscreen = context->render_offscreen, render_offscreen;

    render_offscreen = surface_is_offscreen(target);
    if (context->current_rt == target && render_offscreen == old_render_offscreen) return;

    /* To compensate the lack of format switching with some offscreen rendering methods and on onscreen buffers
     * the alpha blend state changes with different render target formats. */
    if (!context->current_rt)
    {
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_ALPHABLENDENABLE));
    }
    else
    {
        const struct wined3d_format *old = context->current_rt->resource.format;
        const struct wined3d_format *new = target->resource.format;

        if (old->id != new->id)
        {
            /* Disable blending when the alpha mask has changed and when a format doesn't support blending. */
            if ((old->alpha_size && !new->alpha_size) || (!old->alpha_size && new->alpha_size)
                    || !(new->flags & WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING))
                context_invalidate_state(context, STATE_RENDER(WINED3D_RS_ALPHABLENDENABLE));

            /* Update sRGB writing when switching between formats that do/do not support sRGB writing */
            if ((old->flags & WINED3DFMT_FLAG_SRGB_WRITE) != (new->flags & WINED3DFMT_FLAG_SRGB_WRITE))
                context_invalidate_state(context, STATE_RENDER(WINED3D_RS_SRGBWRITEENABLE));
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
            /* Read the back buffer of the old drawable into the destination texture. */
            if (context->current_rt->texture_name_srgb)
                surface_internal_preload(context->current_rt, SRGB_SRGB);
            surface_internal_preload(context->current_rt, SRGB_RGB);
            surface_modify_location(context->current_rt, SFLAG_INDRAWABLE, FALSE);
        }
    }

    context->current_rt = target;
    context_set_render_offscreen(context, render_offscreen);
}

/* Do not call while under the GL lock. */
struct wined3d_context *context_acquire(const struct wined3d_device *device, struct wined3d_surface *target)
{
    struct wined3d_context *current_context = context_get_current();
    struct wined3d_context *context;

    TRACE("device %p, target %p.\n", device, target);

    if (current_context && current_context->destroyed)
        current_context = NULL;

    if (!target)
    {
        if (current_context
                && current_context->current_rt
                && current_context->swapchain->device == device)
        {
            target = current_context->current_rt;
        }
        else
        {
            struct wined3d_swapchain *swapchain = device->swapchains[0];
            if (swapchain->back_buffers)
                target = swapchain->back_buffers[0];
            else
                target = swapchain->front_buffer;
        }
    }

    if (current_context && current_context->current_rt == target)
    {
        context = current_context;
    }
    else if (target->container.type == WINED3D_CONTAINER_SWAPCHAIN)
    {
        TRACE("Rendering onscreen.\n");

        context = swapchain_get_context(target->container.u.swapchain);
    }
    else
    {
        TRACE("Rendering offscreen.\n");

        /* Stay with the current context if possible. Otherwise use the
         * context for the primary swapchain. */
        if (current_context && current_context->swapchain->device == device)
            context = current_context;
        else
            context = swapchain_get_context(device->swapchains[0]);
    }

    context_update_window(context);
    context_setup_target(context, target);
    context_enter(context);
    if (!context->valid) return context;

    if (context != current_context)
    {
        if (!context_set_current(context))
            ERR("Failed to activate the new context.\n");
    }
    else if (context->restore_ctx)
    {
        context_set_gl_context(context);
    }

    return context;
}
