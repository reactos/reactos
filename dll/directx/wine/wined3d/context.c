/*
 * Context and render target management in wined3d
 *
 * Copyright 2007-2011, 2013 Stefan Dösinger for CodeWeavers
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
WINE_DECLARE_DEBUG_CHANNEL(d3d_perf);
WINE_DECLARE_DEBUG_CHANNEL(d3d_synchronous);

#define WINED3D_MAX_FBO_ENTRIES 64

static DWORD wined3d_context_tls_idx;

/* FBO helper functions */

/* Context activation is done by the caller. */
static void context_bind_fbo(struct wined3d_context *context, GLenum target, GLuint fbo)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    switch (target)
    {
        case GL_READ_FRAMEBUFFER:
            if (context->fbo_read_binding == fbo) return;
            context->fbo_read_binding = fbo;
            break;

        case GL_DRAW_FRAMEBUFFER:
            if (context->fbo_draw_binding == fbo) return;
            context->fbo_draw_binding = fbo;
            break;

        case GL_FRAMEBUFFER:
            if (context->fbo_read_binding == fbo
                    && context->fbo_draw_binding == fbo) return;
            context->fbo_read_binding = fbo;
            context->fbo_draw_binding = fbo;
            break;

        default:
            FIXME("Unhandled target %#x.\n", target);
            break;
    }

    gl_info->fbo_ops.glBindFramebuffer(target, fbo);
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
static void context_destroy_fbo(struct wined3d_context *context, GLuint fbo)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    context_bind_fbo(context, GL_FRAMEBUFFER, fbo);
    context_clean_fbo_attachments(gl_info, GL_FRAMEBUFFER);
    context_bind_fbo(context, GL_FRAMEBUFFER, 0);

    gl_info->fbo_ops.glDeleteFramebuffers(1, &fbo);
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
        DWORD format_flags = depth_stencil->container->resource.format_flags;

        if (depth_stencil->current_renderbuffer)
        {
            context_attach_depth_stencil_rb(gl_info, fbo_target,
                    format_flags, depth_stencil->current_renderbuffer->id);
        }
        else
        {
            switch (location)
            {
                case WINED3D_LOCATION_TEXTURE_RGB:
                case WINED3D_LOCATION_TEXTURE_SRGB:
                    wined3d_texture_prepare_texture(depth_stencil->container, context, FALSE);

                    if (format_flags & WINED3DFMT_FLAG_DEPTH)
                    {
                        gl_info->fbo_ops.glFramebufferTexture2D(fbo_target, GL_DEPTH_ATTACHMENT,
                                depth_stencil->texture_target, depth_stencil->container->texture_rgb.name,
                                depth_stencil->texture_level);
                        checkGLcall("glFramebufferTexture2D()");
                    }

                    if (format_flags & WINED3DFMT_FLAG_STENCIL)
                    {
                        gl_info->fbo_ops.glFramebufferTexture2D(fbo_target, GL_STENCIL_ATTACHMENT,
                                depth_stencil->texture_target, depth_stencil->container->texture_rgb.name,
                                depth_stencil->texture_level);
                        checkGLcall("glFramebufferTexture2D()");
                    }
                    break;

                case WINED3D_LOCATION_RB_MULTISAMPLE:
                    surface_prepare_rb(depth_stencil, gl_info, TRUE);
                    context_attach_depth_stencil_rb(gl_info, fbo_target,
                            format_flags, depth_stencil->rb_multisample);
                    break;

                case WINED3D_LOCATION_RB_RESOLVED:
                    surface_prepare_rb(depth_stencil, gl_info, FALSE);
                    context_attach_depth_stencil_rb(gl_info, fbo_target,
                            format_flags, depth_stencil->rb_resolved);
                    break;

                default:
                    ERR("Unsupported location %s (%#x).\n", wined3d_debug_location(location), location);
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
            case WINED3D_LOCATION_TEXTURE_RGB:
            case WINED3D_LOCATION_TEXTURE_SRGB:
                srgb = location == WINED3D_LOCATION_TEXTURE_SRGB;
                wined3d_texture_prepare_texture(surface->container, context, srgb);
                gl_info->fbo_ops.glFramebufferTexture2D(fbo_target, GL_COLOR_ATTACHMENT0 + idx,
                        surface->texture_target, surface_get_texture_name(surface, gl_info, srgb),
                        surface->texture_level);
                checkGLcall("glFramebufferTexture2D()");
                break;

            case WINED3D_LOCATION_RB_MULTISAMPLE:
                surface_prepare_rb(surface, gl_info, TRUE);
                gl_info->fbo_ops.glFramebufferRenderbuffer(fbo_target, GL_COLOR_ATTACHMENT0 + idx,
                        GL_RENDERBUFFER, surface->rb_multisample);
                checkGLcall("glFramebufferRenderbuffer()");
                break;

            case WINED3D_LOCATION_RB_RESOLVED:
                surface_prepare_rb(surface, gl_info, FALSE);
                gl_info->fbo_ops.glFramebufferRenderbuffer(fbo_target, GL_COLOR_ATTACHMENT0 + idx,
                        GL_RENDERBUFFER, surface->rb_resolved);
                checkGLcall("glFramebufferRenderbuffer()");
                break;

            default:
                ERR("Unsupported location %s (%#x).\n", wined3d_debug_location(location), location);
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

        FIXME("\tColor Location %s (%#x).\n", wined3d_debug_location(context->current_fbo->color_location),
                context->current_fbo->color_location);
        FIXME("\tDepth Stencil Location %s (%#x).\n", wined3d_debug_location(context->current_fbo->ds_location),
                context->current_fbo->ds_location);

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
    return buffer ? (1u << 31) | buffer : 0;
}

static inline DWORD context_generate_rt_mask_from_surface(const struct wined3d_surface *target)
{
    return (1u << 31) | surface_get_gl_buffer(target);
}

static struct fbo_entry *context_create_fbo_entry(const struct wined3d_context *context,
        struct wined3d_surface **render_targets, struct wined3d_surface *depth_stencil,
        DWORD color_location, DWORD ds_location)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct fbo_entry *entry;

    entry = HeapAlloc(GetProcessHeap(), 0, sizeof(*entry));
    entry->render_targets = HeapAlloc(GetProcessHeap(), 0, gl_info->limits.buffers * sizeof(*entry->render_targets));
    memcpy(entry->render_targets, render_targets, gl_info->limits.buffers * sizeof(*entry->render_targets));
    entry->depth_stencil = depth_stencil;
    entry->color_location = color_location;
    entry->ds_location = ds_location;
    entry->rt_mask = context_generate_rt_mask(GL_COLOR_ATTACHMENT0);
    entry->attached = FALSE;
    gl_info->fbo_ops.glGenFramebuffers(1, &entry->id);
    checkGLcall("glGenFramebuffers()");
    TRACE("Created FBO %u.\n", entry->id);

    return entry;
}

/* Context activation is done by the caller. */
static void context_reuse_fbo_entry(struct wined3d_context *context, GLenum target,
        struct wined3d_surface **render_targets, struct wined3d_surface *depth_stencil,
        DWORD color_location, DWORD ds_location, struct fbo_entry *entry)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    context_bind_fbo(context, target, entry->id);
    context_clean_fbo_attachments(gl_info, target);

    memcpy(entry->render_targets, render_targets, gl_info->limits.buffers * sizeof(*entry->render_targets));
    entry->depth_stencil = depth_stencil;
    entry->color_location = color_location;
    entry->ds_location = ds_location;
    entry->attached = FALSE;
}

/* Context activation is done by the caller. */
static void context_destroy_fbo_entry(struct wined3d_context *context, struct fbo_entry *entry)
{
    if (entry->id)
    {
        TRACE("Destroy FBO %u.\n", entry->id);
        context_destroy_fbo(context, entry->id);
    }
    --context->fbo_entry_count;
    list_remove(&entry->entry);
    HeapFree(GetProcessHeap(), 0, entry->render_targets);
    HeapFree(GetProcessHeap(), 0, entry);
}

/* Context activation is done by the caller. */
static struct fbo_entry *context_find_fbo_entry(struct wined3d_context *context, GLenum target,
        struct wined3d_surface **render_targets, struct wined3d_surface *depth_stencil,
        DWORD color_location, DWORD ds_location)
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
                && entry->depth_stencil == depth_stencil && entry->color_location == color_location
                && entry->ds_location == ds_location)
        {
            list_remove(&entry->entry);
            list_add_head(&context->fbo_list, &entry->entry);
            return entry;
        }
    }

    if (context->fbo_entry_count < WINED3D_MAX_FBO_ENTRIES)
    {
        entry = context_create_fbo_entry(context, render_targets, depth_stencil, color_location, ds_location);
        list_add_head(&context->fbo_list, &entry->entry);
        ++context->fbo_entry_count;
    }
    else
    {
        entry = LIST_ENTRY(list_tail(&context->fbo_list), struct fbo_entry, entry);
        context_reuse_fbo_entry(context, target, render_targets, depth_stencil, color_location, ds_location, entry);
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
    GLuint read_binding, draw_binding;
    struct wined3d_surface *depth_stencil = entry->depth_stencil;

    if (entry->attached)
    {
        context_bind_fbo(context, target, entry->id);
        return;
    }

    read_binding = context->fbo_read_binding;
    draw_binding = context->fbo_draw_binding;
    context_bind_fbo(context, GL_FRAMEBUFFER, entry->id);

    /* Apply render targets */
    for (i = 0; i < gl_info->limits.buffers; ++i)
    {
        context_attach_surface_fbo(context, target, i, entry->render_targets[i], entry->color_location);
    }

    if (depth_stencil && entry->render_targets[0]
            && (depth_stencil->resource.multisample_type
            != entry->render_targets[0]->resource.multisample_type
            || depth_stencil->resource.multisample_quality
            != entry->render_targets[0]->resource.multisample_quality))
    {
        WARN("Color multisample type %u and quality %u, depth stencil has %u and %u, disabling ds buffer.\n",
                entry->render_targets[0]->resource.multisample_quality,
                entry->render_targets[0]->resource.multisample_type,
                depth_stencil->resource.multisample_quality, depth_stencil->resource.multisample_type);
        depth_stencil = NULL;
    }

    if (depth_stencil)
        surface_set_compatible_renderbuffer(depth_stencil, entry->render_targets[0]);
    context_attach_depth_stencil_fbo(context, target, depth_stencil, entry->ds_location);

    /* Set valid read and draw buffer bindings to satisfy pedantic pre-ES2_compatibility
     * GL contexts requirements. */
    gl_info->gl_ops.gl.p_glReadBuffer(GL_NONE);
    context_set_draw_buffer(context, GL_NONE);
    if (target != GL_FRAMEBUFFER)
    {
        if (target == GL_READ_FRAMEBUFFER)
            context_bind_fbo(context, GL_DRAW_FRAMEBUFFER, draw_binding);
        else
            context_bind_fbo(context, GL_READ_FRAMEBUFFER, read_binding);
    }

    entry->attached = TRUE;
}

/* Context activation is done by the caller. */
static void context_apply_fbo_state(struct wined3d_context *context, GLenum target,
        struct wined3d_surface **render_targets, struct wined3d_surface *depth_stencil,
        DWORD color_location, DWORD ds_location)
{
    struct fbo_entry *entry, *entry2;

    LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &context->fbo_destroy_list, struct fbo_entry, entry)
    {
        context_destroy_fbo_entry(context, entry);
    }

    if (context->rebind_fbo)
    {
        context_bind_fbo(context, GL_FRAMEBUFFER, 0);
        context->rebind_fbo = FALSE;
    }

    if (color_location == WINED3D_LOCATION_DRAWABLE)
    {
        context->current_fbo = NULL;
        context_bind_fbo(context, target, 0);
    }
    else
    {
        context->current_fbo = context_find_fbo_entry(context, target, render_targets, depth_stencil,
                color_location, ds_location);
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
    context_apply_fbo_state(context, target, context->blit_targets, depth_stencil, location, location);
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
            GL_EXTCALL(glGenQueries(1, &query->id));
            checkGLcall("glGenQueries");

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

/* Context activation is done by the caller. */
void context_alloc_timestamp_query(struct wined3d_context *context, struct wined3d_timestamp_query *query)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    if (context->free_timestamp_query_count)
    {
        query->id = context->free_timestamp_queries[--context->free_timestamp_query_count];
    }
    else
    {
        GL_EXTCALL(glGenQueries(1, &query->id));
        checkGLcall("glGenQueries");

        TRACE("Allocated timestamp query %u in context %p.\n", query->id, context);
    }

    query->context = context;
    list_add_head(&context->timestamp_queries, &query->entry);
}

void context_free_timestamp_query(struct wined3d_timestamp_query *query)
{
    struct wined3d_context *context = query->context;

    list_remove(&query->entry);
    query->context = NULL;

    if (context->free_timestamp_query_count >= context->free_timestamp_query_size - 1)
    {
        UINT new_size = context->free_timestamp_query_size << 1;
        GLuint *new_data = HeapReAlloc(GetProcessHeap(), 0, context->free_timestamp_queries,
                new_size * sizeof(*context->free_timestamp_queries));

        if (!new_data)
        {
            ERR("Failed to grow free list, leaking query %u in context %p.\n", query->id, context);
            return;
        }

        context->free_timestamp_query_size = new_size;
        context->free_timestamp_queries = new_data;
    }

    context->free_timestamp_queries[context->free_timestamp_query_count++] = query->id;
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

        ctx->valid = 1;
    }
    ctx->needs_set = 0;
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

    if (context->hdc)
        wined3d_release_dc(context->win_handle, context->hdc);

    context->win_handle = context->swapchain->win_handle;
    context->needs_set = 1;
    context->valid = 1;

    if (!(context->hdc = GetDC(context->win_handle)))
    {
        ERR("Failed to get a device context for window %p.\n", context->win_handle);
        context->valid = 0;
    }
}

static void context_destroy_gl_resources(struct wined3d_context *context)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct wined3d_timestamp_query *timestamp_query;
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

    if (restore_ctx == context->glCtx)
        restore_ctx = NULL;
    else if (context->valid)
        context_set_gl_context(context);

    LIST_FOR_EACH_ENTRY(timestamp_query, &context->timestamp_queries, struct wined3d_timestamp_query, entry)
    {
        if (context->valid)
            GL_EXTCALL(glDeleteQueries(1, &timestamp_query->id));
        timestamp_query->context = NULL;
    }

    LIST_FOR_EACH_ENTRY(occlusion_query, &context->occlusion_queries, struct wined3d_occlusion_query, entry)
    {
        if (context->valid && gl_info->supported[ARB_OCCLUSION_QUERY])
            GL_EXTCALL(glDeleteQueries(1, &occlusion_query->id));
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

        if (gl_info->supported[ARB_TIMER_QUERY])
            GL_EXTCALL(glDeleteQueries(context->free_timestamp_query_count, context->free_timestamp_queries));

        if (gl_info->supported[ARB_OCCLUSION_QUERY])
            GL_EXTCALL(glDeleteQueries(context->free_occlusion_query_count, context->free_occlusion_queries));

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

    HeapFree(GetProcessHeap(), 0, context->free_timestamp_queries);
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

    wined3d_release_dc(context->win_handle, context->hdc);

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

/* This is used when a context for render target A is active, but a separate context is
 * needed to access the WGL framebuffer for render target B. Re-acquire a context for rt
 * A to avoid breaking caller code. */
void context_restore(struct wined3d_context *context, struct wined3d_surface *restore)
{
    if (context->current_rt != restore)
    {
        context_release(context);
        context = context_acquire(restore->resource.device, restore);
    }

    context_release(context);
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
            context->needs_set = 1;
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
    context->isStateDirty[idx] |= (1u << shift);
}

/* This function takes care of wined3d pixel format selection. */
static int context_choose_pixel_format(const struct wined3d_device *device, HDC hdc,
        const struct wined3d_format *color_format, const struct wined3d_format *ds_format,
        BOOL auxBuffers, BOOL findCompatible)
{
    int iPixelFormat=0;
    unsigned int current_value;
    unsigned int cfg_count = device->adapter->cfg_count;
    unsigned int i;

    TRACE("device %p, dc %p, color_format %s, ds_format %s, aux_buffers %#x, find_compatible %#x.\n",
            device, hdc, debug_d3dformat(color_format->id), debug_d3dformat(ds_format->id),
            auxBuffers, findCompatible);

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
        if (cfg->redSize < color_format->red_size)
            continue;
        if (cfg->greenSize < color_format->green_size)
            continue;
        if (cfg->blueSize < color_format->blue_size)
            continue;
        if (cfg->alphaSize < color_format->alpha_size)
            continue;
        if (cfg->depthSize < ds_format->depth_size)
            continue;
        if (ds_format->stencil_size && cfg->stencilSize != ds_format->stencil_size)
            continue;
        /* Check multisampling support. */
        if (cfg->numSamples)
            continue;

        value = 1;
        /* We try to locate a format which matches our requirements exactly. In case of
         * depth it is no problem to emulate 16-bit using e.g. 24-bit, so accept that. */
        if (cfg->depthSize == ds_format->depth_size)
            value += 1;
        if (cfg->stencilSize == ds_format->stencil_size)
            value += 2;
        if (cfg->alphaSize == color_format->alpha_size)
            value += 4;
        /* We like to have aux buffers in backbuffer mode */
        if (auxBuffers && cfg->auxBuffers)
            value += 8;
        if (cfg->redSize == color_format->red_size
                && cfg->greenSize == color_format->green_size
                && cfg->blueSize == color_format->blue_size)
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
        pfd.cAlphaBits = color_format->alpha_size;
        pfd.cColorBits = color_format->red_size + color_format->green_size
                + color_format->blue_size + color_format->alpha_size;
        pfd.cDepthBits = ds_format->depth_size;
        pfd.cStencilBits = ds_format->stencil_size;
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
        GL_EXTCALL(glActiveTexture(GL_TEXTURE0 + i));
        checkGLcall("glActiveTexture");

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

static BOOL context_debug_output_enabled(const struct wined3d_gl_info *gl_info)
{
    return gl_info->supported[ARB_DEBUG_OUTPUT]
            && (ERR_ON(d3d) || FIXME_ON(d3d) || WARN_ON(d3d_perf));
}

static void WINE_GLAPI wined3d_debug_callback(GLenum source, GLenum type, GLuint id,
        GLenum severity, GLsizei length, const char *message, void *ctx)
{
    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR_ARB:
            ERR("%p: %s.\n", ctx, debugstr_an(message, length));
            break;

        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR_ARB:
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR_ARB:
        case GL_DEBUG_TYPE_PORTABILITY_ARB:
            FIXME("%p: %s.\n", ctx, debugstr_an(message, length));
            break;

        case GL_DEBUG_TYPE_PERFORMANCE_ARB:
            WARN_(d3d_perf)("%p: %s.\n", ctx, debugstr_an(message, length));
            break;

        default:
            FIXME("ctx %p, type %#x: %s.\n", ctx, type, debugstr_an(message, length));
            break;
    }
}

HGLRC context_create_wgl_attribs(const struct wined3d_gl_info *gl_info, HDC hdc, HGLRC share_ctx)
{
    HGLRC ctx;
    unsigned int ctx_attrib_idx = 0;
    GLint ctx_attribs[7], ctx_flags = 0;

    if (context_debug_output_enabled(gl_info))
        ctx_flags = WGL_CONTEXT_DEBUG_BIT_ARB;
    ctx_attribs[ctx_attrib_idx++] = WGL_CONTEXT_MAJOR_VERSION_ARB;
    ctx_attribs[ctx_attrib_idx++] = gl_info->selected_gl_version >> 16;
    ctx_attribs[ctx_attrib_idx++] = WGL_CONTEXT_MINOR_VERSION_ARB;
    ctx_attribs[ctx_attrib_idx++] = gl_info->selected_gl_version & 0xffff;
    if (gl_info->selected_gl_version >= MAKEDWORD_VERSION(3, 2))
        ctx_flags |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
    if (ctx_flags)
    {
        ctx_attribs[ctx_attrib_idx++] = WGL_CONTEXT_FLAGS_ARB;
        ctx_attribs[ctx_attrib_idx++] = ctx_flags;
    }
    ctx_attribs[ctx_attrib_idx] = 0;

    if (!(ctx = gl_info->p_wglCreateContextAttribsARB(hdc, share_ctx, ctx_attribs)))
    {
        if (ctx_flags & WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB)
        {
            ctx_attribs[ctx_attrib_idx - 1] &= ~WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
            if (!(ctx = gl_info->p_wglCreateContextAttribsARB(hdc, share_ctx, ctx_attribs)))
                WARN("Failed to create a WGL context with wglCreateContextAttribsARB, last error %#x.\n",
                        GetLastError());
        }
    }
    return ctx;
}

struct wined3d_context *context_create(struct wined3d_swapchain *swapchain,
        struct wined3d_surface *target, const struct wined3d_format *ds_format)
{
    struct wined3d_device *device = swapchain->device;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    const struct wined3d_format *color_format;
    struct wined3d_context *ret;
    BOOL auxBuffers = FALSE;
    HGLRC ctx, share_ctx;
    int pixel_format;
    unsigned int s;
    int swap_interval;
    DWORD state;
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

    ret->free_timestamp_query_size = 4;
    ret->free_timestamp_queries = HeapAlloc(GetProcessHeap(), 0,
            ret->free_timestamp_query_size * sizeof(*ret->free_timestamp_queries));
    if (!ret->free_timestamp_queries)
        goto out;
    list_init(&ret->timestamp_queries);

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

    if (!device->shader_backend->shader_allocate_context_data(ret))
    {
        ERR("Failed to allocate shader backend context data.\n");
        goto out;
    }
    if (!device->adapter->fragment_pipe->allocate_context_data(ret))
    {
        ERR("Failed to allocate fragment pipeline context data.\n");
        goto out;
    }

#if defined(STAGING_CSMT)
    ret->current_fb.render_targets = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            sizeof(*ret->current_fb.render_targets) * gl_info->limits.buffers);
    ret->current_fb.rt_size = gl_info->limits.buffers;
    if (!ret->current_fb.render_targets)
        goto out;
    if (device->context_count)
        ret->offscreenBuffer = device->contexts[0]->offscreenBuffer;

#endif /* STAGING_CSMT */
    /* Initialize the texture unit mapping to a 1:1 mapping */
    for (s = 0; s < MAX_COMBINED_SAMPLERS; ++s)
    {
        if (s < gl_info->limits.combined_samplers)
        {
            ret->tex_unit_map[s] = s;
            ret->rev_tex_unit_map[s] = s;
        }
        else
        {
            ret->tex_unit_map[s] = WINED3D_UNMAPPED_STAGE;
            ret->rev_tex_unit_map[s] = WINED3D_UNMAPPED_STAGE;
        }
    }

    if (!(hdc = GetDC(swapchain->win_handle)))
    {
        WARN("Failed to retrieve device context, trying swapchain backup.\n");

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

    /* When "always_offscreen" is enabled, we only use the drawable for
     * presentation blits, and don't do any rendering to it. That means we
     * don't need depth or stencil buffers, and can mostly ignore the render
     * target format. This wouldn't necessarily be quite correct for 10bpc
     * display modes, but we don't currently support those.
     * Using the same format regardless of the color/depth/stencil targets
     * makes it much less likely that different wined3d instances will set
     * conflicting pixel formats. */
    if (wined3d_settings.always_offscreen)
    {
        color_format = wined3d_get_format(gl_info, WINED3DFMT_B8G8R8A8_UNORM);
        ds_format = wined3d_get_format(gl_info, WINED3DFMT_UNKNOWN);
    }

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

    share_ctx = device->context_count ? device->contexts[0]->glCtx : NULL;
    if (gl_info->p_wglCreateContextAttribsARB)
    {
        if (!(ctx = context_create_wgl_attribs(gl_info, hdc, share_ctx)))
            goto out;
    }
    else
    {
        if (!(ctx = wglCreateContext(hdc)))
        {
            ERR("Failed to create a WGL context.\n");
            context_release(ret);
            goto out;
        }

        if (share_ctx && !wglShareLists(share_ctx, ctx))
        {
            ERR("wglShareLists(%p, %p) failed, last error %#x.\n", share_ctx, ctx, GetLastError());
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
    ret->d3d_info = &device->adapter->d3d_info;
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

    ret->render_offscreen = wined3d_resource_is_offscreen(&target->container->resource);
    ret->draw_buffers_mask = context_generate_rt_mask(GL_BACK);
    ret->valid = 1;

    ret->glCtx = ctx;
    ret->win_handle = swapchain->win_handle;
    ret->hdc = hdc;
    ret->pixel_format = pixel_format;
    ret->needs_set = 1;

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

    if (context_debug_output_enabled(gl_info))
    {
        GL_EXTCALL(glDebugMessageCallback(wined3d_debug_callback, ret));
        if (TRACE_ON(d3d_synchronous))
            gl_info->gl_ops.gl.p_glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        GL_EXTCALL(glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_FALSE));
        if (ERR_ON(d3d))
        {
            GL_EXTCALL(glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_ERROR,
                    GL_DONT_CARE, 0, NULL, GL_TRUE));
        }
        if (FIXME_ON(d3d))
        {
            GL_EXTCALL(glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR,
                    GL_DONT_CARE, 0, NULL, GL_TRUE));
            GL_EXTCALL(glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR,
                    GL_DONT_CARE, 0, NULL, GL_TRUE));
            GL_EXTCALL(glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_PORTABILITY,
                    GL_DONT_CARE, 0, NULL, GL_TRUE));
        }
        if (WARN_ON(d3d_perf))
        {
            GL_EXTCALL(glDebugMessageControl(GL_DONT_CARE, GL_DEBUG_TYPE_PERFORMANCE,
                    GL_DONT_CARE, 0, NULL, GL_TRUE));
        }
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

    gl_info->gl_ops.gl.p_glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
    checkGLcall("glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);");

    gl_info->gl_ops.gl.p_glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
    checkGLcall("glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);");

    gl_info->gl_ops.gl.p_glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
    checkGLcall("glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);");

    gl_info->gl_ops.gl.p_glPixelStorei(GL_PACK_ALIGNMENT, device->surface_alignment);
    checkGLcall("glPixelStorei(GL_PACK_ALIGNMENT, device->surface_alignment);");
    gl_info->gl_ops.gl.p_glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    checkGLcall("glPixelStorei(GL_UNPACK_ALIGNMENT, device->surface_alignment);");

    if (!gl_info->supported[WINED3D_GL_LEGACY_CONTEXT])
    {
        GLuint vao;

        GL_EXTCALL(glGenVertexArrays(1, &vao));
        GL_EXTCALL(glBindVertexArray(vao));
        checkGLcall("creating VAO");
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
        static const char dummy_program[] =
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
    device->shader_backend->shader_init_context_state(ret);
    ret->shader_update_mask = (1u << WINED3D_SHADER_TYPE_PIXEL)
            | (1u << WINED3D_SHADER_TYPE_VERTEX)
            | (1u << WINED3D_SHADER_TYPE_GEOMETRY);

    /* If this happens to be the first context for the device, dummy textures
     * are not created yet. In that case, they will be created (and bound) by
     * create_dummy_textures right after this context is initialized. */
    if (device->dummy_texture_2d[0])
        bind_dummy_textures(device, ret);

    TRACE("Created context %p.\n", ret);

    return ret;

out:
    device->shader_backend->shader_free_context_data(ret);
    device->adapter->fragment_pipe->free_context_data(ret);
#if defined(STAGING_CSMT)
    HeapFree(GetProcessHeap(), 0, ret->current_fb.render_targets);
#endif /* STAGING_CSMT */
    HeapFree(GetProcessHeap(), 0, ret->free_event_queries);
    HeapFree(GetProcessHeap(), 0, ret->free_occlusion_queries);
    HeapFree(GetProcessHeap(), 0, ret->free_timestamp_queries);
    HeapFree(GetProcessHeap(), 0, ret->draw_buffers);
    HeapFree(GetProcessHeap(), 0, ret->blit_targets);
    HeapFree(GetProcessHeap(), 0, ret);
    return NULL;
}

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

    device->shader_backend->shader_free_context_data(context);
    device->adapter->fragment_pipe->free_context_data(context);
#if defined(STAGING_CSMT)
    HeapFree(GetProcessHeap(), 0, context->current_fb.render_targets);
#endif /* STAGING_CSMT */
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

    if (rt->container->swapchain && rt->container->swapchain->front_buffer == rt->container)
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
        sampler = context->rev_tex_unit_map[i];
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
    if (gl_info->supported[ARB_SAMPLER_OBJECTS])
        GL_EXTCALL(glBindSampler(0, 0));
    context_active_texture(context, gl_info, 0);

    sampler = context->rev_tex_unit_map[0];

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
    device->shader_backend->shader_disable(device->shader_priv, context);

    context->blit_w = rt_size.cx;
    context->blit_h = rt_size.cy;
    context_invalidate_state(context, STATE_VIEWPORT);
    context_invalidate_state(context, STATE_TRANSFORM(WINED3D_TS_PROJECTION));
}

static inline BOOL is_rt_mask_onscreen(DWORD rt_mask)
{
    return rt_mask & (1u << 31);
}

static inline GLenum draw_buffer_from_rt_mask(DWORD rt_mask)
{
    return rt_mask & ~(1u << 31);
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
                GL_EXTCALL(glDrawBuffers(i, context->draw_buffers));
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
    DWORD *current_mask = context->current_fbo ? &context->current_fbo->rt_mask : &context->draw_buffers_mask;
    DWORD new_mask = context_generate_rt_mask(buffer);

    if (new_mask == *current_mask)
        return;

    gl_info->gl_ops.gl.p_glDrawBuffer(buffer);
    checkGLcall("glDrawBuffer()");

    *current_mask = new_mask;
}

/* Context activation is done by the caller. */
void context_active_texture(struct wined3d_context *context, const struct wined3d_gl_info *gl_info, unsigned int unit)
{
    GL_EXTCALL(glActiveTexture(GL_TEXTURE0 + unit));
    checkGLcall("glActiveTexture");
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
    if (existing == required)
        return TRUE;
    if ((existing->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_FLOAT)
            != (required->flags[WINED3D_GL_RES_TYPE_TEX_2D] & WINED3DFMT_FLAG_FLOAT))
        return FALSE;
    if (existing->depth_size < required->depth_size)
        return FALSE;
    /* If stencil bits are used the exact amount is required - otherwise
     * wrapping won't work correctly. */
    if (required->stencil_size && required->stencil_size != existing->stencil_size)
        return FALSE;
    return TRUE;
}

/* Context activation is done by the caller. */
static void context_validate_onscreen_formats(struct wined3d_context *context,
        const struct wined3d_rendertarget_view *depth_stencil)
{
    /* Onscreen surfaces are always in a swapchain */
    struct wined3d_swapchain *swapchain = context->current_rt->container->swapchain;

    if (context->render_offscreen || !depth_stencil) return;
    if (match_depth_stencil_format(swapchain->ds_format, depth_stencil->format)) return;

    /* TODO: If the requested format would satisfy the needs of the existing one(reverse match),
     * or no onscreen depth buffer was created, the OpenGL drawable could be changed to the new
     * format. */
    WARN("Depth stencil format is not supported by WGL, rendering the backbuffer in an FBO\n");

    /* The currently active context is the necessary context to access the swapchain's onscreen buffers */
#if defined(STAGING_CSMT)
    wined3d_resource_load_location(&context->current_rt->resource, context, WINED3D_LOCATION_TEXTURE_RGB);
    swapchain->render_to_fbo = TRUE;
    swapchain_update_draw_bindings(swapchain);
    context_set_render_offscreen(context, TRUE);
}

static DWORD context_generate_rt_mask_no_fbo(const struct wined3d_context *context, const struct wined3d_surface *rt)
{
    if (!rt || rt->resource.format->id == WINED3DFMT_NULL)
        return 0;
    else if (rt->container->swapchain)
        return context_generate_rt_mask_from_surface(rt);
    else
        return context_generate_rt_mask(context->offscreenBuffer);
#else  /* STAGING_CSMT */
    surface_load_location(context->current_rt, context, WINED3D_LOCATION_TEXTURE_RGB);
    swapchain->render_to_fbo = TRUE;
    swapchain_update_draw_bindings(swapchain);
    context_set_render_offscreen(context, TRUE);
}

static DWORD context_generate_rt_mask_no_fbo(const struct wined3d_device *device, const struct wined3d_surface *rt)
{
    if (!rt || rt->resource.format->id == WINED3DFMT_NULL)
        return 0;
    else if (rt->container->swapchain)
        return context_generate_rt_mask_from_surface(rt);
    else
        return context_generate_rt_mask(device->offscreenBuffer);
#endif /* STAGING_CSMT */
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
            wined3d_texture_load(rt->container, context, FALSE);

            context_apply_fbo_state_blit(context, GL_FRAMEBUFFER, rt, NULL, rt->container->resource.draw_binding);
            if (rt->resource.format->id != WINED3DFMT_NULL)
                rt_mask = 1;
            else
                rt_mask = 0;
        }
        else
        {
            context->current_fbo = NULL;
            context_bind_fbo(context, GL_FRAMEBUFFER, 0);
            rt_mask = context_generate_rt_mask_from_surface(rt);
        }
    }
    else
    {
#if defined(STAGING_CSMT)
        rt_mask = context_generate_rt_mask_no_fbo(context, rt);
#else  /* STAGING_CSMT */
        rt_mask = context_generate_rt_mask_no_fbo(device, rt);
#endif /* STAGING_CSMT */
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

static BOOL context_validate_rt_config(UINT rt_count, struct wined3d_rendertarget_view * const *rts,
        const struct wined3d_rendertarget_view *ds)
{
    unsigned int i;

    if (ds) return TRUE;

    for (i = 0; i < rt_count; ++i)
    {
        if (rts[i] && rts[i]->format->id != WINED3DFMT_NULL)
            return TRUE;
    }

    WARN("Invalid render target config, need at least one attachment.\n");
    return FALSE;
}

/* Context activation is done by the caller. */
BOOL context_apply_clear_state(struct wined3d_context *context, const struct wined3d_device *device,
        UINT rt_count, const struct wined3d_fb_state *fb)
{
    struct wined3d_rendertarget_view **rts = fb->render_targets;
    struct wined3d_rendertarget_view *dsv = fb->depth_stencil;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    DWORD rt_mask = 0, *cur_mask;
    UINT i;

#if defined(STAGING_CSMT)
    if (isStateDirty(context, STATE_FRAMEBUFFER) || !wined3d_fb_equal(fb, &context->current_fb)
#else  /* STAGING_CSMT */
    if (isStateDirty(context, STATE_FRAMEBUFFER) || fb != &device->fb
#endif /* STAGING_CSMT */
            || rt_count != context->gl_info->limits.buffers)
    {
        if (!context_validate_rt_config(rt_count, rts, dsv))
            return FALSE;

        if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
        {
            context_validate_onscreen_formats(context, dsv);

            if (!rt_count || wined3d_resource_is_offscreen(rts[0]->resource))
            {
                for (i = 0; i < rt_count; ++i)
                {
                    context->blit_targets[i] = wined3d_rendertarget_view_get_surface(rts[i]);
                    if (rts[i] && rts[i]->format->id != WINED3DFMT_NULL)
                        rt_mask |= (1u << i);
                }
                while (i < context->gl_info->limits.buffers)
                {
                    context->blit_targets[i] = NULL;
                    ++i;
                }
                context_apply_fbo_state(context, GL_FRAMEBUFFER, context->blit_targets,
                        wined3d_rendertarget_view_get_surface(dsv),
                        rt_count ? rts[0]->resource->draw_binding : 0,
                        dsv ? dsv->resource->draw_binding : 0);
            }
            else
            {
                context_apply_fbo_state(context, GL_FRAMEBUFFER, NULL, NULL,
                        WINED3D_LOCATION_DRAWABLE, WINED3D_LOCATION_DRAWABLE);
                rt_mask = context_generate_rt_mask_from_surface(wined3d_rendertarget_view_get_surface(rts[0]));
            }

            /* If the framebuffer is not the device's fb the device's fb has to be reapplied
             * next draw. Otherwise we could mark the framebuffer state clean here, once the
             * state management allows this */
            context_invalidate_state(context, STATE_FRAMEBUFFER);
        }
        else
        {
#if defined(STAGING_CSMT)
            rt_mask = context_generate_rt_mask_no_fbo(context,
                    rt_count ? wined3d_rendertarget_view_get_surface(rts[0]) : NULL);
        }

        wined3d_fb_copy(&context->current_fb, fb);
#else  /* STAGING_CSMT */
            rt_mask = context_generate_rt_mask_no_fbo(device,
                    rt_count ? wined3d_rendertarget_view_get_surface(rts[0]) : NULL);
        }
#endif /* STAGING_CSMT */
    }
    else if (wined3d_settings.offscreen_rendering_mode == ORM_FBO
            && (!rt_count || wined3d_resource_is_offscreen(rts[0]->resource)))
    {
        for (i = 0; i < rt_count; ++i)
        {
            if (rts[i] && rts[i]->format->id != WINED3DFMT_NULL)
                rt_mask |= (1u << i);
        }
    }
    else
    {
#if defined(STAGING_CSMT)
        rt_mask = context_generate_rt_mask_no_fbo(context,
#else  /* STAGING_CSMT */
        rt_mask = context_generate_rt_mask_no_fbo(device,
#endif /* STAGING_CSMT */
                rt_count ? wined3d_rendertarget_view_get_surface(rts[0]) : NULL);
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

#if defined(STAGING_CSMT)
static DWORD find_draw_buffers_mask(const struct wined3d_context *context, const struct wined3d_state *state)
{
    struct wined3d_rendertarget_view **rts = state->fb.render_targets;
    struct wined3d_shader *ps = state->shader[WINED3D_SHADER_TYPE_PIXEL];
    DWORD rt_mask, rt_mask_bits;
    unsigned int i;

    if (wined3d_settings.offscreen_rendering_mode != ORM_FBO)
        return context_generate_rt_mask_no_fbo(context, wined3d_rendertarget_view_get_surface(rts[0]));
#else  /* STAGING_CSMT */
static DWORD find_draw_buffers_mask(const struct wined3d_context *context, const struct wined3d_device *device)
{
    const struct wined3d_state *state = &device->state;
    struct wined3d_rendertarget_view **rts = state->fb->render_targets;
    struct wined3d_shader *ps = state->shader[WINED3D_SHADER_TYPE_PIXEL];
    DWORD rt_mask, rt_mask_bits;
    unsigned int i;

    if (wined3d_settings.offscreen_rendering_mode != ORM_FBO)
        return context_generate_rt_mask_no_fbo(device, wined3d_rendertarget_view_get_surface(rts[0]));
#endif /* STAGING_CSMT */
    else if (!context->render_offscreen)
        return context_generate_rt_mask_from_surface(wined3d_rendertarget_view_get_surface(rts[0]));

    rt_mask = ps ? ps->reg_maps.rt_mask : 1;
    rt_mask &= context->d3d_info->valid_rt_mask;
    rt_mask_bits = rt_mask;
    i = 0;
    while (rt_mask_bits)
    {
        rt_mask_bits &= ~(1u << i);
        if (!rts[i] || rts[i]->format->id == WINED3DFMT_NULL)
            rt_mask &= ~(1u << i);

        i++;
    }

    return rt_mask;
}

/* Context activation is done by the caller. */
void context_state_fb(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
#if defined(STAGING_CSMT)
    const struct wined3d_fb_state *fb = &state->fb;
    DWORD rt_mask = find_draw_buffers_mask(context, state);
#else  /* STAGING_CSMT */
    const struct wined3d_device *device = context->swapchain->device;
    const struct wined3d_fb_state *fb = state->fb;
    DWORD rt_mask = find_draw_buffers_mask(context, device);
#endif /* STAGING_CSMT */
    DWORD *cur_mask;

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
    {
        if (!context->render_offscreen)
        {
            context_apply_fbo_state(context, GL_FRAMEBUFFER, NULL, NULL,
                    WINED3D_LOCATION_DRAWABLE, WINED3D_LOCATION_DRAWABLE);
        }
        else
        {
            unsigned int i;

            for (i = 0; i < context->gl_info->limits.buffers; ++i)
            {
                context->blit_targets[i] = wined3d_rendertarget_view_get_surface(fb->render_targets[i]);
            }
            context_apply_fbo_state(context, GL_FRAMEBUFFER, context->blit_targets,
                    wined3d_rendertarget_view_get_surface(fb->depth_stencil),
                    fb->render_targets[0]->resource->draw_binding,
                    fb->depth_stencil ? fb->depth_stencil->resource->draw_binding : 0);
        }
    }

    cur_mask = context->current_fbo ? &context->current_fbo->rt_mask : &context->draw_buffers_mask;
    if (rt_mask != *cur_mask)
    {
        context_apply_draw_buffers(context, rt_mask);
        *cur_mask = rt_mask;
    }
#if defined(STAGING_CSMT)

    wined3d_fb_copy(&context->current_fb, &state->fb);
#endif /* STAGING_CSMT */
}

static void context_map_stage(struct wined3d_context *context, DWORD stage, DWORD unit)
{
    DWORD i = context->rev_tex_unit_map[unit];
    DWORD j = context->tex_unit_map[stage];

    TRACE("Mapping stage %u to unit %u.\n", stage, unit);
    context->tex_unit_map[stage] = unit;
    if (i != WINED3D_UNMAPPED_STAGE && i != stage)
        context->tex_unit_map[i] = WINED3D_UNMAPPED_STAGE;

    context->rev_tex_unit_map[unit] = stage;
    if (j != WINED3D_UNMAPPED_STAGE && j != unit)
        context->rev_tex_unit_map[j] = WINED3D_UNMAPPED_STAGE;
}

static void context_invalidate_texture_stage(struct wined3d_context *context, DWORD stage)
{
    DWORD i;

    for (i = 0; i <= WINED3D_HIGHEST_TEXTURE_STATE; ++i)
        context_invalidate_state(context, STATE_TEXTURESTAGE(stage, i));
}

static void context_update_fixed_function_usage_map(struct wined3d_context *context,
        const struct wined3d_state *state)
{
    UINT i, start, end;

    context->fixed_function_usage_map = 0;
    for (i = 0; i < MAX_TEXTURES; ++i)
    {
        enum wined3d_texture_op color_op = state->texture_states[i][WINED3D_TSS_COLOR_OP];
        enum wined3d_texture_op alpha_op = state->texture_states[i][WINED3D_TSS_ALPHA_OP];
        DWORD color_arg1 = state->texture_states[i][WINED3D_TSS_COLOR_ARG1] & WINED3DTA_SELECTMASK;
        DWORD color_arg2 = state->texture_states[i][WINED3D_TSS_COLOR_ARG2] & WINED3DTA_SELECTMASK;
        DWORD color_arg3 = state->texture_states[i][WINED3D_TSS_COLOR_ARG0] & WINED3DTA_SELECTMASK;
        DWORD alpha_arg1 = state->texture_states[i][WINED3D_TSS_ALPHA_ARG1] & WINED3DTA_SELECTMASK;
        DWORD alpha_arg2 = state->texture_states[i][WINED3D_TSS_ALPHA_ARG2] & WINED3DTA_SELECTMASK;
        DWORD alpha_arg3 = state->texture_states[i][WINED3D_TSS_ALPHA_ARG0] & WINED3DTA_SELECTMASK;

        /* Not used, and disable higher stages. */
        if (color_op == WINED3D_TOP_DISABLE)
            break;

        if (((color_arg1 == WINED3DTA_TEXTURE) && color_op != WINED3D_TOP_SELECT_ARG2)
                || ((color_arg2 == WINED3DTA_TEXTURE) && color_op != WINED3D_TOP_SELECT_ARG1)
                || ((color_arg3 == WINED3DTA_TEXTURE)
                    && (color_op == WINED3D_TOP_MULTIPLY_ADD || color_op == WINED3D_TOP_LERP))
                || ((alpha_arg1 == WINED3DTA_TEXTURE) && alpha_op != WINED3D_TOP_SELECT_ARG2)
                || ((alpha_arg2 == WINED3DTA_TEXTURE) && alpha_op != WINED3D_TOP_SELECT_ARG1)
                || ((alpha_arg3 == WINED3DTA_TEXTURE)
                    && (alpha_op == WINED3D_TOP_MULTIPLY_ADD || alpha_op == WINED3D_TOP_LERP)))
            context->fixed_function_usage_map |= (1u << i);

        if ((color_op == WINED3D_TOP_BUMPENVMAP || color_op == WINED3D_TOP_BUMPENVMAP_LUMINANCE)
                && i < MAX_TEXTURES - 1)
            context->fixed_function_usage_map |= (1u << (i + 1));
    }

    if (i < context->lowest_disabled_stage)
    {
        start = i;
        end = context->lowest_disabled_stage;
    }
    else
    {
        start = context->lowest_disabled_stage;
        end = i;
    }

    context->lowest_disabled_stage = i;
    for (i = start + 1; i < end; ++i)
    {
        context_invalidate_state(context, STATE_TEXTURESTAGE(i, WINED3D_TSS_COLOR_OP));
    }
}

static void context_map_fixed_function_samplers(struct wined3d_context *context,
        const struct wined3d_state *state)
{
    const struct wined3d_d3d_info *d3d_info = context->d3d_info;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    unsigned int i, tex;
    WORD ffu_map;

    context_update_fixed_function_usage_map(context, state);

    if (gl_info->limits.combined_samplers >= MAX_COMBINED_SAMPLERS)
        return;

    ffu_map = context->fixed_function_usage_map;

    if (d3d_info->limits.ffp_textures == d3d_info->limits.ffp_blend_stages
            || context->lowest_disabled_stage <= d3d_info->limits.ffp_textures)
    {
        for (i = 0; ffu_map; ffu_map >>= 1, ++i)
        {
            if (!(ffu_map & 1))
                continue;

            if (context->tex_unit_map[i] != i)
            {
                context_map_stage(context, i, i);
                context_invalidate_state(context, STATE_SAMPLER(i));
                context_invalidate_texture_stage(context, i);
            }
        }
        return;
    }

    /* Now work out the mapping */
    tex = 0;
    for (i = 0; ffu_map; ffu_map >>= 1, ++i)
    {
        if (!(ffu_map & 1))
            continue;

        if (context->tex_unit_map[i] != tex)
        {
            context_map_stage(context, i, tex);
            context_invalidate_state(context, STATE_SAMPLER(i));
            context_invalidate_texture_stage(context, i);
        }

        ++tex;
    }
}

static void context_map_psamplers(struct wined3d_context *context, const struct wined3d_state *state)
{
    const struct wined3d_d3d_info *d3d_info = context->d3d_info;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    const struct wined3d_shader_resource_info *resource_info =
            state->shader[WINED3D_SHADER_TYPE_PIXEL]->reg_maps.resource_info;
    unsigned int i;

    if (gl_info->limits.combined_samplers >= MAX_COMBINED_SAMPLERS)
        return;

    for (i = 0; i < MAX_FRAGMENT_SAMPLERS; ++i)
    {
        if (resource_info[i].type && context->tex_unit_map[i] != i)
        {
            context_map_stage(context, i, i);
            context_invalidate_state(context, STATE_SAMPLER(i));
            if (i < d3d_info->limits.ffp_blend_stages)
                context_invalidate_texture_stage(context, i);
        }
    }
}

static BOOL context_unit_free_for_vs(const struct wined3d_context *context,
        const struct wined3d_shader_resource_info *ps_resource_info, DWORD unit)
{
    DWORD current_mapping = context->rev_tex_unit_map[unit];

    /* Not currently used */
    if (current_mapping == WINED3D_UNMAPPED_STAGE)
        return TRUE;

    if (current_mapping < MAX_FRAGMENT_SAMPLERS)
    {
        /* Used by a fragment sampler */

        if (!ps_resource_info)
        {
            /* No pixel shader, check fixed function */
            return current_mapping >= MAX_TEXTURES || !(context->fixed_function_usage_map & (1u << current_mapping));
        }

        /* Pixel shader, check the shader's sampler map */
        return !ps_resource_info[current_mapping].type;
    }

    return TRUE;
}

static void context_map_vsamplers(struct wined3d_context *context, BOOL ps, const struct wined3d_state *state)
{
    const struct wined3d_shader_resource_info *vs_resource_info =
            state->shader[WINED3D_SHADER_TYPE_VERTEX]->reg_maps.resource_info;
    const struct wined3d_shader_resource_info *ps_resource_info = NULL;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    int start = min(MAX_COMBINED_SAMPLERS, gl_info->limits.combined_samplers) - 1;
    int i;

    if (gl_info->limits.combined_samplers >= MAX_COMBINED_SAMPLERS)
        return;

    /* Note that we only care if a resource is used or not, not the
     * resource's specific type. Otherwise we'd need to call
     * shader_update_samplers() here for 1.x pixelshaders. */
    if (ps)
        ps_resource_info = state->shader[WINED3D_SHADER_TYPE_PIXEL]->reg_maps.resource_info;

    for (i = 0; i < MAX_VERTEX_SAMPLERS; ++i)
    {
        DWORD vsampler_idx = i + MAX_FRAGMENT_SAMPLERS;
        if (vs_resource_info[i].type)
        {
            while (start >= 0)
            {
                if (context_unit_free_for_vs(context, ps_resource_info, start))
                {
                    if (context->tex_unit_map[vsampler_idx] != start)
                    {
                        context_map_stage(context, vsampler_idx, start);
                        context_invalidate_state(context, STATE_SAMPLER(vsampler_idx));
                    }

                    --start;
                    break;
                }

                --start;
            }
            if (context->tex_unit_map[vsampler_idx] == WINED3D_UNMAPPED_STAGE)
                WARN("Couldn't find a free texture unit for vertex sampler %u.\n", i);
        }
    }
}

static void context_update_tex_unit_map(struct wined3d_context *context, const struct wined3d_state *state)
{
    BOOL vs = use_vs(state);
    BOOL ps = use_ps(state);

    /* Try to go for a 1:1 mapping of the samplers when possible. Pixel shaders
     * need a 1:1 map at the moment.
     * When the mapping of a stage is changed, sampler and ALL texture stage
     * states have to be reset. */

    if (ps)
        context_map_psamplers(context, state);
    else
        context_map_fixed_function_samplers(context, state);

    if (vs)
        context_map_vsamplers(context, ps, state);
}

/* Context activation is done by the caller. */
void context_state_drawbuf(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
#if defined(STAGING_CSMT)
    DWORD rt_mask, *cur_mask;

    if (isStateDirty(context, STATE_FRAMEBUFFER)) return;

    cur_mask = context->current_fbo ? &context->current_fbo->rt_mask : &context->draw_buffers_mask;
    rt_mask = find_draw_buffers_mask(context, state);
#else  /* STAGING_CSMT */
    const struct wined3d_device *device = context->swapchain->device;
    DWORD rt_mask, *cur_mask;

    if (isStateDirty(context, STATE_FRAMEBUFFER)) return;

    cur_mask = context->current_fbo ? &context->current_fbo->rt_mask : &context->draw_buffers_mask;
    rt_mask = find_draw_buffers_mask(context, device);
#endif /* STAGING_CSMT */
    if (rt_mask != *cur_mask)
    {
        context_apply_draw_buffers(context, rt_mask);
        *cur_mask = rt_mask;
    }
}

static BOOL fixed_get_input(BYTE usage, BYTE usage_idx, unsigned int *regnum)
{
    if ((usage == WINED3D_DECL_USAGE_POSITION || usage == WINED3D_DECL_USAGE_POSITIONT) && !usage_idx)
        *regnum = WINED3D_FFP_POSITION;
    else if (usage == WINED3D_DECL_USAGE_BLEND_WEIGHT && !usage_idx)
        *regnum = WINED3D_FFP_BLENDWEIGHT;
    else if (usage == WINED3D_DECL_USAGE_BLEND_INDICES && !usage_idx)
        *regnum = WINED3D_FFP_BLENDINDICES;
    else if (usage == WINED3D_DECL_USAGE_NORMAL && !usage_idx)
        *regnum = WINED3D_FFP_NORMAL;
    else if (usage == WINED3D_DECL_USAGE_PSIZE && !usage_idx)
        *regnum = WINED3D_FFP_PSIZE;
    else if (usage == WINED3D_DECL_USAGE_COLOR && !usage_idx)
        *regnum = WINED3D_FFP_DIFFUSE;
    else if (usage == WINED3D_DECL_USAGE_COLOR && usage_idx == 1)
        *regnum = WINED3D_FFP_SPECULAR;
    else if (usage == WINED3D_DECL_USAGE_TEXCOORD && usage_idx < WINED3DDP_MAXTEXCOORD)
        *regnum = WINED3D_FFP_TEXCOORD0 + usage_idx;
    else
    {
        FIXME("Unsupported input stream [usage=%s, usage_idx=%u].\n", debug_d3ddeclusage(usage), usage_idx);
        *regnum = ~0U;
        return FALSE;
    }

    return TRUE;
}

/* Context activation is done by the caller. */
void context_stream_info_from_declaration(struct wined3d_context *context,
        const struct wined3d_state *state, struct wined3d_stream_info *stream_info)
{
    /* We need to deal with frequency data! */
    struct wined3d_vertex_declaration *declaration = state->vertex_declaration;
    BOOL use_vshader = use_vs(state);
    BOOL generic_attributes = context->d3d_info->ffp_generic_attributes;
    unsigned int i;

    stream_info->use_map = 0;
    stream_info->swizzle_map = 0;
    stream_info->position_transformed = declaration->position_transformed;

    /* Translate the declaration into strided data. */
    for (i = 0; i < declaration->element_count; ++i)
    {
        const struct wined3d_vertex_declaration_element *element = &declaration->elements[i];
        const struct wined3d_stream_state *stream = &state->streams[element->input_slot];
        BOOL stride_used;
        unsigned int idx;

        TRACE("%p Element %p (%u of %u).\n", declaration->elements,
                element, i + 1, declaration->element_count);

        if (!stream->buffer)
            continue;

        TRACE("offset %u input_slot %u usage_idx %d.\n", element->offset, element->input_slot, element->usage_idx);

        if (use_vshader)
        {
            if (element->output_slot == WINED3D_OUTPUT_SLOT_UNUSED)
            {
                stride_used = FALSE;
            }
            else if (element->output_slot == WINED3D_OUTPUT_SLOT_SEMANTIC)
            {
                /* TODO: Assuming vertexdeclarations are usually used with the
                 * same or a similar shader, it might be worth it to store the
                 * last used output slot and try that one first. */
                stride_used = vshader_get_input(state->shader[WINED3D_SHADER_TYPE_VERTEX],
                        element->usage, element->usage_idx, &idx);
            }
            else
            {
                idx = element->output_slot;
                stride_used = TRUE;
            }
        }
        else
        {
            if (!generic_attributes && !element->ffp_valid)
            {
                WARN("Skipping unsupported fixed function element of format %s and usage %s.\n",
                        debug_d3dformat(element->format->id), debug_d3ddeclusage(element->usage));
                stride_used = FALSE;
            }
            else
            {
                stride_used = fixed_get_input(element->usage, element->usage_idx, &idx);
            }
        }

        if (stride_used)
        {
            TRACE("Load %s array %u [usage %s, usage_idx %u, "
                    "input_slot %u, offset %u, stride %u, format %s, class %s, step_rate %u].\n",
                    use_vshader ? "shader": "fixed function", idx,
                    debug_d3ddeclusage(element->usage), element->usage_idx, element->input_slot,
                    element->offset, stream->stride, debug_d3dformat(element->format->id),
                    debug_d3dinput_classification(element->input_slot_class), element->instance_data_step_rate);

            stream_info->elements[idx].format = element->format;
            stream_info->elements[idx].data.buffer_object = 0;
            stream_info->elements[idx].data.addr = (BYTE *)NULL + stream->offset + element->offset;
            stream_info->elements[idx].stride = stream->stride;
            stream_info->elements[idx].stream_idx = element->input_slot;
            if (stream->flags & WINED3DSTREAMSOURCE_INSTANCEDATA)
            {
                stream_info->elements[idx].divisor = 1;
            }
            else if (element->input_slot_class == WINED3D_INPUT_PER_INSTANCE_DATA)
            {
                stream_info->elements[idx].divisor = element->instance_data_step_rate;
                if (!element->instance_data_step_rate)
                    FIXME("Instance step rate 0 not implemented.\n");
            }
            else
            {
                stream_info->elements[idx].divisor = 0;
            }

            if (!context->gl_info->supported[ARB_VERTEX_ARRAY_BGRA]
                    && element->format->id == WINED3DFMT_B8G8R8A8_UNORM)
            {
                stream_info->swizzle_map |= 1u << idx;
            }
            stream_info->use_map |= 1u << idx;
        }
    }
}

/* Context activation is done by the caller. */
static void context_update_stream_info(struct wined3d_context *context, const struct wined3d_state *state)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    const struct wined3d_d3d_info *d3d_info = context->d3d_info;
    struct wined3d_stream_info *stream_info = &context->stream_info;
    DWORD prev_all_vbo = stream_info->all_vbo;
    unsigned int i;
    WORD map;

    context_stream_info_from_declaration(context, state, stream_info);

    stream_info->all_vbo = 1;
    context->num_buffer_queries = 0;
    for (i = 0, map = stream_info->use_map; map; map >>= 1, ++i)
    {
        struct wined3d_stream_info_element *element;
        struct wined3d_bo_address data;
        struct wined3d_buffer *buffer;

        if (!(map & 1))
            continue;

        element = &stream_info->elements[i];
        buffer = state->streams[element->stream_idx].buffer;

        /* We can't use VBOs if the base vertex index is negative. OpenGL
         * doesn't accept negative offsets (or rather offsets bigger than the
         * VBO, because the pointer is unsigned), so use system memory
         * sources. In most sane cases the pointer - offset will still be > 0,
         * otherwise it will wrap around to some big value. Hope that with the
         * indices the driver wraps it back internally. If not,
         * drawStridedSlow is needed, including a vertex buffer path. */
        if (state->load_base_vertex_index < 0)
        {
            WARN_(d3d_perf)("load_base_vertex_index is < 0 (%d), not using VBOs.\n",
                    state->load_base_vertex_index);
            element->data.buffer_object = 0;
            element->data.addr += (ULONG_PTR)buffer_get_sysmem(buffer, context);
            if ((UINT_PTR)element->data.addr < -state->load_base_vertex_index * element->stride)
                FIXME("System memory vertex data load offset is negative!\n");
        }
        else
        {
            buffer_internal_preload(buffer, context, state);
            buffer_get_memory(buffer, context, &data);
            element->data.buffer_object = data.buffer_object;
            element->data.addr += (ULONG_PTR)data.addr;
        }

        if (!element->data.buffer_object)
            stream_info->all_vbo = 0;

        if (buffer->query)
            context->buffer_queries[context->num_buffer_queries++] = buffer->query;

        TRACE("Load array %u {%#x:%p}.\n", i, element->data.buffer_object, element->data.addr);
    }

    if (use_vs(state))
    {
        if (state->vertex_declaration->half_float_conv_needed && !stream_info->all_vbo)
        {
#if defined(STAGING_CSMT)
            TRACE("Using draw_strided_slow with vertex shaders for FLOAT16 conversion.\n");
#else  /* STAGING_CSMT */
            TRACE("Using drawStridedSlow with vertex shaders for FLOAT16 conversion.\n");
#endif /* STAGING_CSMT */
            context->use_immediate_mode_draw = TRUE;
        }
        else
        {
            context->use_immediate_mode_draw = FALSE;
        }
    }
    else
    {
        WORD slow_mask = -!d3d_info->ffp_generic_attributes & (1u << WINED3D_FFP_PSIZE);
        slow_mask |= -!gl_info->supported[ARB_VERTEX_ARRAY_BGRA]
                & ((1u << WINED3D_FFP_DIFFUSE) | (1u << WINED3D_FFP_SPECULAR));

        if (((stream_info->position_transformed && !d3d_info->xyzrhw)
                || (stream_info->use_map & slow_mask)) && !stream_info->all_vbo)
            context->use_immediate_mode_draw = TRUE;
        else
            context->use_immediate_mode_draw = FALSE;
    }

    if (prev_all_vbo != stream_info->all_vbo)
        context_invalidate_state(context, STATE_INDEXBUFFER);
}

/* Context activation is done by the caller. */
static void context_preload_texture(struct wined3d_context *context,
        const struct wined3d_state *state, unsigned int idx)
{
    struct wined3d_texture *texture;

    if (!(texture = state->textures[idx]))
        return;

    wined3d_texture_load(texture, context, state->sampler_states[idx][WINED3D_SAMP_SRGB_TEXTURE]);
}

/* Context activation is done by the caller. */
static void context_preload_textures(struct wined3d_context *context, const struct wined3d_state *state)
{
    unsigned int i;

    if (use_vs(state))
    {
        for (i = 0; i < MAX_VERTEX_SAMPLERS; ++i)
        {
            if (state->shader[WINED3D_SHADER_TYPE_VERTEX]->reg_maps.resource_info[i].type)
                context_preload_texture(context, state, MAX_FRAGMENT_SAMPLERS + i);
        }
    }

    if (use_ps(state))
    {
        for (i = 0; i < MAX_FRAGMENT_SAMPLERS; ++i)
        {
            if (state->shader[WINED3D_SHADER_TYPE_PIXEL]->reg_maps.resource_info[i].type)
                context_preload_texture(context, state, i);
        }
    }
    else
    {
        WORD ffu_map = context->fixed_function_usage_map;

        for (i = 0; ffu_map; ffu_map >>= 1, ++i)
        {
            if (ffu_map & 1)
                context_preload_texture(context, state, i);
        }
    }
}

static void context_load_shader_resources(struct wined3d_context *context, const struct wined3d_state *state)
{
    struct wined3d_shader_sampler_map_entry *entry;
    struct wined3d_shader_resource_view *view;
    struct wined3d_shader *shader;
    unsigned int i, j;

    for (i = 0; i < WINED3D_SHADER_TYPE_COUNT; ++i)
    {
        if (!(shader = state->shader[i]))
            continue;

        for (j = 0; j < WINED3D_MAX_CBS; ++j)
        {
            if (state->cb[i][j])
                buffer_internal_preload(state->cb[i][j], context, state);
        }

        for (j = 0; j < shader->reg_maps.sampler_map.count; ++j)
        {
            entry = &shader->reg_maps.sampler_map.entries[j];

            if (!(view = state->shader_resource_view[i][entry->resource_idx]))
            {
                WARN("No resource view bound at index %u, %u.\n", i, entry->resource_idx);
                continue;
            }

            if (view->resource->type == WINED3D_RTYPE_BUFFER)
                buffer_internal_preload(buffer_from_resource(view->resource), context, state);
            else
                wined3d_texture_load(wined3d_texture_from_resource(view->resource), context, FALSE);
        }
    }
}

static void context_bind_shader_resources(struct wined3d_context *context, const struct wined3d_state *state)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct wined3d_shader_sampler_map_entry *entry;
    struct wined3d_shader_resource_view *view;
    struct wined3d_sampler *sampler;
    struct wined3d_texture *texture;
    struct wined3d_shader *shader;
    unsigned int i, j, count;

    static const struct
    {
        enum wined3d_shader_type type;
        unsigned int base_idx;
        unsigned int count;
    }
    shader_types[] =
    {
        {WINED3D_SHADER_TYPE_PIXEL,     0,                      MAX_FRAGMENT_SAMPLERS},
        {WINED3D_SHADER_TYPE_VERTEX,    MAX_FRAGMENT_SAMPLERS,  MAX_VERTEX_SAMPLERS},
    };

    for (i = 0; i < ARRAY_SIZE(shader_types); ++i)
    {
        if (!(shader = state->shader[shader_types[i].type]))
            continue;

        count = shader->reg_maps.sampler_map.count;
        if (count > shader_types[i].count)
        {
            FIXME("Shader %p needs %u samplers, but only %u are supported.\n",
                    shader, count, shader_types[i].count);
            count = shader_types[i].count;
        }

        for (j = 0; j < count; ++j)
        {
            entry = &shader->reg_maps.sampler_map.entries[j];

            if (!(view = state->shader_resource_view[shader_types[i].type][entry->resource_idx]))
            {
                WARN("No resource view bound at index %u, %u.\n", shader_types[i].type, entry->resource_idx);
                continue;
            }

            if (view->resource->type == WINED3D_RTYPE_BUFFER)
            {
                FIXME("Buffer shader resources not supported.\n");
                continue;
            }

            if (!(sampler = state->sampler[shader_types[i].type][entry->sampler_idx]))
            {
                WARN("No sampler object bound at index %u, %u.\n", shader_types[i].type, entry->sampler_idx);
                continue;
            }

            texture = wined3d_texture_from_resource(view->resource);
            context_active_texture(context, gl_info, shader_types[i].base_idx + entry->bind_idx);
            wined3d_texture_bind(texture, context, FALSE);

            GL_EXTCALL(glBindSampler(shader_types[i].base_idx + entry->bind_idx, sampler->name));
            checkGLcall("glBindSampler");
        }
    }
}

/* Context activation is done by the caller. */
#if defined(STAGING_CSMT)
BOOL context_apply_draw_state(struct wined3d_context *context, const struct wined3d_device *device,
        const struct wined3d_state *state)
{
    const struct StateEntry *state_table = context->state_table;
    const struct wined3d_fb_state *fb = &state->fb;
#else  /* STAGING_CSMT */
BOOL context_apply_draw_state(struct wined3d_context *context, struct wined3d_device *device)
{
    const struct wined3d_state *state = &device->state;
    const struct StateEntry *state_table = context->state_table;
    const struct wined3d_fb_state *fb = state->fb;
#endif /* STAGING_CSMT */
    unsigned int i;
    WORD map;

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
    context_update_tex_unit_map(context, state);
    context_preload_textures(context, state);
    context_load_shader_resources(context, state);
    /* TODO: Right now the dependency on the vertex shader is necessary
     * since context_stream_info_from_declaration depends on the reg_maps of
     * the current VS but maybe it's possible to relax the coupling in some
     * situations at least. */
    if (isStateDirty(context, STATE_VDECL) || isStateDirty(context, STATE_STREAMSRC)
            || isStateDirty(context, STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX)))
    {
        context_update_stream_info(context, state);
    }
    else
    {
        for (i = 0, map = context->stream_info.use_map; map; map >>= 1, ++i)
        {
            if (map & 1)
#if defined(STAGING_CSMT)
                buffer_internal_preload(state->streams[context->stream_info.elements[i].stream_idx].buffer,
                        context, state);
        }
        /* PreLoad may kick buffers out of vram. */
        if (isStateDirty(context, STATE_STREAMSRC))
            context_update_stream_info(context, state);
#else  /* STAGING_CSMT */
                buffer_mark_used(state->streams[context->stream_info.elements[i].stream_idx].buffer);
        }
#endif /* STAGING_CSMT */
    }
    if (state->index_buffer)
    {
        if (context->stream_info.all_vbo)
            buffer_internal_preload(state->index_buffer, context, state);
        else
            buffer_get_sysmem(state->index_buffer, context);
    }

    for (i = 0; i < context->numDirtyEntries; ++i)
    {
        DWORD rep = context->dirtyArray[i];
        DWORD idx = rep / (sizeof(*context->isStateDirty) * CHAR_BIT);
        BYTE shift = rep & ((sizeof(*context->isStateDirty) * CHAR_BIT) - 1);
        context->isStateDirty[idx] &= ~(1u << shift);
        state_table[rep].apply(context, state, rep);
    }

    if (context->shader_update_mask)
    {
        device->shader_backend->shader_select(device->shader_priv, context, state);
        context->shader_update_mask = 0;
    }

    if (context->constant_update_mask)
    {
        device->shader_backend->shader_load_constants(device->shader_priv, context, state);
        context->constant_update_mask = 0;
    }

    if (context->update_shader_resource_bindings)
    {
        context_bind_shader_resources(context, state);
        context->update_shader_resource_bindings = 0;
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

    render_offscreen = wined3d_resource_is_offscreen(&target->container->resource);
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
                    || !(target->container->resource.format_flags & WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING))
                context_invalidate_state(context, STATE_RENDER(WINED3D_RS_ALPHABLENDENABLE));

            /* Update sRGB writing when switching between formats that do/do not support sRGB writing */
            if ((context->current_rt->container->resource.format_flags & WINED3DFMT_FLAG_SRGB_WRITE)
                    != (target->container->resource.format_flags & WINED3DFMT_FLAG_SRGB_WRITE))
                context_invalidate_state(context, STATE_RENDER(WINED3D_RS_SRGBWRITEENABLE));
        }

        /* When switching away from an offscreen render target, and we're not
         * using FBOs, we have to read the drawable into the texture. This is
         * done via PreLoad (and WINED3D_LOCATION_DRAWABLE set on the surface).
         * There are some things that need care though. PreLoad needs a GL context,
         * and FindContext is called before the context is activated. It also
         * has to be called with the old rendertarget active, otherwise a
         * wrong drawable is read. */
        if (wined3d_settings.offscreen_rendering_mode != ORM_FBO
                && old_render_offscreen && context->current_rt != target)
        {
            struct wined3d_texture *texture = context->current_rt->container;

            /* Read the back buffer of the old drawable into the destination texture. */
            if (texture->texture_srgb.name)
                wined3d_texture_load(texture, context, TRUE);
            wined3d_texture_load(texture, context, FALSE);
#if defined(STAGING_CSMT)
            wined3d_resource_invalidate_location(&context->current_rt->resource, WINED3D_LOCATION_DRAWABLE);
#else  /* STAGING_CSMT */
            surface_invalidate_location(context->current_rt, WINED3D_LOCATION_DRAWABLE);
#endif /* STAGING_CSMT */
        }
    }

    context->current_rt = target;
    context_set_render_offscreen(context, render_offscreen);
}

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
                target = surface_from_resource(wined3d_texture_get_sub_resource(swapchain->back_buffers[0], 0));
            else
                target = surface_from_resource(wined3d_texture_get_sub_resource(swapchain->front_buffer, 0));
        }
    }

    if (current_context && current_context->current_rt == target)
    {
        context = current_context;
    }
    else if (target->container->swapchain)
    {
        TRACE("Rendering onscreen.\n");

        context = swapchain_get_context(target->container->swapchain);
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

    context_enter(context);
    context_update_window(context);
    context_setup_target(context, target);
    if (!context->valid) return context;

    if (context != current_context)
    {
        if (!context_set_current(context))
            ERR("Failed to activate the new context.\n");
    }
    else if (context->needs_set)
    {
        context_set_gl_context(context);
    }

    return context;
}
