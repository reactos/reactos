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
#define WINED3D_ALL_LAYERS (~0u)

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
        GLenum fbo_target, DWORD flags, GLuint rb)
{
    if (flags & WINED3D_FBO_ENTRY_FLAG_DEPTH)
    {
        gl_info->fbo_ops.glFramebufferRenderbuffer(fbo_target, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rb);
        checkGLcall("glFramebufferRenderbuffer()");
    }

    if (flags & WINED3D_FBO_ENTRY_FLAG_STENCIL)
    {
        gl_info->fbo_ops.glFramebufferRenderbuffer(fbo_target, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rb);
        checkGLcall("glFramebufferRenderbuffer()");
    }
}

static void context_attach_gl_texture_fbo(struct wined3d_context *context,
        GLenum fbo_target, GLenum attachment, const struct wined3d_fbo_resource *resource)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    if (!resource)
    {
        gl_info->fbo_ops.glFramebufferTexture2D(fbo_target, attachment, GL_TEXTURE_2D, 0, 0);
    }
    else if (resource->layer == WINED3D_ALL_LAYERS)
    {
        if (!gl_info->fbo_ops.glFramebufferTexture)
        {
            FIXME("OpenGL implementation doesn't support glFramebufferTexture().\n");
            return;
        }

        gl_info->fbo_ops.glFramebufferTexture(fbo_target, attachment,
                resource->object, resource->level);
    }
    else if (resource->target == GL_TEXTURE_1D_ARRAY || resource->target == GL_TEXTURE_2D_ARRAY ||
            resource->target == GL_TEXTURE_3D)
    {
        if (!gl_info->fbo_ops.glFramebufferTextureLayer)
        {
            FIXME("OpenGL implementation doesn't support glFramebufferTextureLayer().\n");
            return;
        }

        gl_info->fbo_ops.glFramebufferTextureLayer(fbo_target, attachment,
                resource->object, resource->level, resource->layer);
    }
    else if (resource->target == GL_TEXTURE_1D)
    {
        gl_info->fbo_ops.glFramebufferTexture1D(fbo_target, attachment,
                resource->target, resource->object, resource->level);
        checkGLcall("glFramebufferTexture1D()");
    }
    else
    {
        gl_info->fbo_ops.glFramebufferTexture2D(fbo_target, attachment,
                resource->target, resource->object, resource->level);
    }
    checkGLcall("attach texture to fbo");
}

/* Context activation is done by the caller. */
static void context_attach_depth_stencil_fbo(struct wined3d_context *context,
        GLenum fbo_target, const struct wined3d_fbo_resource *resource, BOOL rb_namespace,
        DWORD flags)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    if (resource->object)
    {
        TRACE("Attach depth stencil %u.\n", resource->object);

        if (rb_namespace)
        {
            context_attach_depth_stencil_rb(gl_info, fbo_target,
                    flags, resource->object);
        }
        else
        {
            if (flags & WINED3D_FBO_ENTRY_FLAG_DEPTH)
                context_attach_gl_texture_fbo(context, fbo_target, GL_DEPTH_ATTACHMENT, resource);

            if (flags & WINED3D_FBO_ENTRY_FLAG_STENCIL)
                context_attach_gl_texture_fbo(context, fbo_target, GL_STENCIL_ATTACHMENT, resource);
        }

        if (!(flags & WINED3D_FBO_ENTRY_FLAG_DEPTH))
            context_attach_gl_texture_fbo(context, fbo_target, GL_DEPTH_ATTACHMENT, NULL);

        if (!(flags & WINED3D_FBO_ENTRY_FLAG_STENCIL))
            context_attach_gl_texture_fbo(context, fbo_target, GL_STENCIL_ATTACHMENT, NULL);
    }
    else
    {
        TRACE("Attach depth stencil 0.\n");

        context_attach_gl_texture_fbo(context, fbo_target, GL_DEPTH_ATTACHMENT, NULL);
        context_attach_gl_texture_fbo(context, fbo_target, GL_STENCIL_ATTACHMENT, NULL);
    }
}

/* Context activation is done by the caller. */
static void context_attach_surface_fbo(struct wined3d_context *context,
        GLenum fbo_target, DWORD idx, const struct wined3d_fbo_resource *resource, BOOL rb_namespace)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    TRACE("Attach GL object %u to %u.\n", resource->object, idx);

    if (resource->object)
    {

        if (rb_namespace)
        {
            gl_info->fbo_ops.glFramebufferRenderbuffer(fbo_target, GL_COLOR_ATTACHMENT0 + idx,
                    GL_RENDERBUFFER, resource->object);
            checkGLcall("glFramebufferRenderbuffer()");
        }
        else
        {
            context_attach_gl_texture_fbo(context, fbo_target, GL_COLOR_ATTACHMENT0 + idx, resource);
        }
    }
    else
    {
        context_attach_gl_texture_fbo(context, fbo_target, GL_COLOR_ATTACHMENT0 + idx, NULL);
    }
}

static void context_dump_fbo_attachment(const struct wined3d_gl_info *gl_info, GLenum target,
        GLenum attachment)
{
    static const struct
    {
        GLenum target;
        GLenum binding;
        const char *str;
        enum wined3d_gl_extension extension;
    }
    texture_type[] =
    {
        {GL_TEXTURE_2D,            GL_TEXTURE_BINDING_2D,            "2d",        WINED3D_GL_EXT_NONE},
        {GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_BINDING_RECTANGLE_ARB, "rectangle", ARB_TEXTURE_RECTANGLE},
        {GL_TEXTURE_2D_ARRAY,      GL_TEXTURE_BINDING_2D_ARRAY,      "2d-array",  EXT_TEXTURE_ARRAY},
    };

    GLint type, name, samples, width, height, old_texture, level, face, fmt, tex_target;

    gl_info->fbo_ops.glGetFramebufferAttachmentParameteriv(target, attachment,
            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &name);
    gl_info->fbo_ops.glGetFramebufferAttachmentParameteriv(target, attachment,
            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &type);

    if (type == GL_RENDERBUFFER)
    {
        gl_info->fbo_ops.glBindRenderbuffer(GL_RENDERBUFFER, name);
        gl_info->fbo_ops.glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &width);
        gl_info->fbo_ops.glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &height);
        if (gl_info->limits.samples > 1)
            gl_info->fbo_ops.glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_SAMPLES, &samples);
        else
            samples = 1;
        gl_info->fbo_ops.glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_INTERNAL_FORMAT, &fmt);
        FIXME("    %s: renderbuffer %d, %dx%d, %d samples, format %#x.\n",
                debug_fboattachment(attachment), name, width, height, samples, fmt);
    }
    else if (type == GL_TEXTURE)
    {
        const char *tex_type_str;

        gl_info->fbo_ops.glGetFramebufferAttachmentParameteriv(target, attachment,
                GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL, &level);
        gl_info->fbo_ops.glGetFramebufferAttachmentParameteriv(target, attachment,
                GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE, &face);

        if (face)
        {
            gl_info->gl_ops.gl.p_glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &old_texture);

            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_CUBE_MAP, name);
            gl_info->gl_ops.gl.p_glGetTexLevelParameteriv(face, level, GL_TEXTURE_INTERNAL_FORMAT, &fmt);
            gl_info->gl_ops.gl.p_glGetTexLevelParameteriv(face, level, GL_TEXTURE_WIDTH, &width);
            gl_info->gl_ops.gl.p_glGetTexLevelParameteriv(face, level, GL_TEXTURE_HEIGHT, &height);

            tex_target = GL_TEXTURE_CUBE_MAP;
            tex_type_str = "cube";
        }
        else
        {
            unsigned int i;

            tex_type_str = NULL;
            for (i = 0; i < sizeof(texture_type) / sizeof(*texture_type); ++i)
            {
                if (!gl_info->supported[texture_type[i].extension])
                    continue;

                gl_info->gl_ops.gl.p_glGetIntegerv(texture_type[i].binding, &old_texture);
                while (gl_info->gl_ops.gl.p_glGetError());

                gl_info->gl_ops.gl.p_glBindTexture(texture_type[i].target, name);
                if (!gl_info->gl_ops.gl.p_glGetError())
                {
                    tex_target = texture_type[i].target;
                    tex_type_str = texture_type[i].str;
                    break;
                }
                gl_info->gl_ops.gl.p_glBindTexture(texture_type[i].target, old_texture);
            }
            if (!tex_type_str)
            {
                FIXME("Cannot find type of texture %d.\n", name);
                return;
            }

            gl_info->gl_ops.gl.p_glGetTexLevelParameteriv(tex_target, level, GL_TEXTURE_INTERNAL_FORMAT, &fmt);
            gl_info->gl_ops.gl.p_glGetTexLevelParameteriv(tex_target, level, GL_TEXTURE_WIDTH, &width);
            gl_info->gl_ops.gl.p_glGetTexLevelParameteriv(tex_target, level, GL_TEXTURE_HEIGHT, &height);
        }

        FIXME("    %s: %s texture %d, %dx%d, format %#x.\n", debug_fboattachment(attachment),
                tex_type_str, name, width, height, fmt);

        gl_info->gl_ops.gl.p_glBindTexture(tex_target, old_texture);
        checkGLcall("Guess texture type");
    }
    else if (type == GL_NONE)
    {
        FIXME("    %s: NONE.\n", debug_fboattachment(attachment));
    }
    else
    {
        ERR("    %s: Unknown attachment %#x.\n", debug_fboattachment(attachment), type);
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
        unsigned int i;

        FIXME("FBO status %s (%#x)\n", debug_fbostatus(status), status);

        if (!context->current_fbo)
        {
            ERR("FBO 0 is incomplete, driver bug?\n");
            return;
        }

        context_dump_fbo_attachment(gl_info, target, GL_DEPTH_ATTACHMENT);
        context_dump_fbo_attachment(gl_info, target, GL_STENCIL_ATTACHMENT);

        for (i = 0; i < gl_info->limits.buffers; ++i)
            context_dump_fbo_attachment(gl_info, target, GL_COLOR_ATTACHMENT0 + i);
        checkGLcall("Dump FBO attachments");
    }
}

static inline DWORD context_generate_rt_mask(GLenum buffer)
{
    /* Should take care of all the GL_FRONT/GL_BACK/GL_AUXi/GL_NONE... cases */
    return buffer ? (1u << 31) | buffer : 0;
}

static inline DWORD context_generate_rt_mask_from_resource(struct wined3d_resource *resource)
{
    if (resource->type != WINED3D_RTYPE_TEXTURE_2D)
    {
        FIXME("Not implemented for %s resources.\n", debug_d3dresourcetype(resource->type));
        return 0;
    }

    return (1u << 31) | wined3d_texture_get_gl_buffer(texture_from_resource(resource));
}

static inline void context_set_fbo_key_for_render_target(const struct wined3d_context *context,
        struct wined3d_fbo_entry_key *key, unsigned int idx, struct wined3d_rendertarget_info *render_target,
        DWORD location)
{
    unsigned int sub_resource_idx = render_target->sub_resource_idx;
    struct wined3d_resource *resource = render_target->resource;
    struct wined3d_texture *texture;

    if (!resource || resource->format->id == WINED3DFMT_NULL || resource->type == WINED3D_RTYPE_BUFFER)
    {
        if (resource && resource->type == WINED3D_RTYPE_BUFFER)
            FIXME("Not implemented for %s resources.\n", debug_d3dresourcetype(resource->type));
        key->objects[idx].object = 0;
        key->objects[idx].target = 0;
        key->objects[idx].level = key->objects[idx].layer = 0;
        return;
    }

    if (render_target->gl_view.name)
    {
        key->objects[idx].object = render_target->gl_view.name;
        key->objects[idx].target = render_target->gl_view.target;
        key->objects[idx].level = 0;
        key->objects[idx].layer = WINED3D_ALL_LAYERS;
        return;
    }

    texture = wined3d_texture_from_resource(resource);
    if (resource->type == WINED3D_RTYPE_TEXTURE_2D)
    {
        struct wined3d_surface *surface = texture->sub_resources[sub_resource_idx].u.surface;

        if (surface->current_renderbuffer)
        {
            key->objects[idx].object = surface->current_renderbuffer->id;
            key->objects[idx].target = 0;
            key->objects[idx].level = key->objects[idx].layer = 0;
            key->rb_namespace |= 1 << idx;
            return;
        }

        key->objects[idx].target = surface->texture_target;
        key->objects[idx].level = surface->texture_level;
        key->objects[idx].layer = surface->texture_layer;
    }
    else
    {
        key->objects[idx].target = texture->target;
        key->objects[idx].level = sub_resource_idx % texture->level_count;
        key->objects[idx].layer = sub_resource_idx / texture->level_count;
    }
    if (render_target->layer_count != 1)
        key->objects[idx].layer = WINED3D_ALL_LAYERS;

    switch (location)
    {
        case WINED3D_LOCATION_TEXTURE_RGB:
            key->objects[idx].object = wined3d_texture_get_texture_name(texture, context, FALSE);
            break;

        case WINED3D_LOCATION_TEXTURE_SRGB:
            key->objects[idx].object = wined3d_texture_get_texture_name(texture, context, TRUE);
            break;

        case WINED3D_LOCATION_RB_MULTISAMPLE:
            key->objects[idx].object = texture->rb_multisample;
            key->objects[idx].target = 0;
            key->objects[idx].level = key->objects[idx].layer = 0;
            key->rb_namespace |= 1 << idx;
            break;

        case WINED3D_LOCATION_RB_RESOLVED:
            key->objects[idx].object = texture->rb_resolved;
            key->objects[idx].target = 0;
            key->objects[idx].level = key->objects[idx].layer = 0;
            key->rb_namespace |= 1 << idx;
            break;
    }
}

static void context_generate_fbo_key(const struct wined3d_context *context,
        struct wined3d_fbo_entry_key *key, struct wined3d_rendertarget_info *render_targets,
        struct wined3d_surface *depth_stencil_surface, DWORD color_location,
        DWORD ds_location)
{
    struct wined3d_rendertarget_info depth_stencil = {{0}};
    unsigned int i;

    key->rb_namespace = 0;
    if (depth_stencil_surface)
    {
        depth_stencil.resource = &depth_stencil_surface->container->resource;
        depth_stencil.sub_resource_idx = surface_get_sub_resource_idx(depth_stencil_surface);
        depth_stencil.layer_count = 1;
    }
    context_set_fbo_key_for_render_target(context, key, 0, &depth_stencil, ds_location);

    for (i = 0; i < context->gl_info->limits.buffers; ++i)
        context_set_fbo_key_for_render_target(context, key, i + 1, &render_targets[i], color_location);
}

static struct fbo_entry *context_create_fbo_entry(const struct wined3d_context *context,
        struct wined3d_rendertarget_info *render_targets, struct wined3d_surface *depth_stencil,
        DWORD color_location, DWORD ds_location)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    unsigned int object_count = gl_info->limits.buffers + 1;
    struct fbo_entry *entry;

    entry = HeapAlloc(GetProcessHeap(), 0,
            FIELD_OFFSET(struct fbo_entry, key.objects[object_count]));
    memset(&entry->key, 0, FIELD_OFFSET(struct wined3d_fbo_entry_key, objects[object_count]));
    context_generate_fbo_key(context, &entry->key, render_targets, depth_stencil, color_location, ds_location);
    entry->flags = 0;
    if (depth_stencil)
    {
        if (depth_stencil->container->resource.format_flags & WINED3DFMT_FLAG_DEPTH)
            entry->flags |= WINED3D_FBO_ENTRY_FLAG_DEPTH;
        if (depth_stencil->container->resource.format_flags & WINED3DFMT_FLAG_STENCIL)
            entry->flags |= WINED3D_FBO_ENTRY_FLAG_STENCIL;
    }
    entry->rt_mask = context_generate_rt_mask(GL_COLOR_ATTACHMENT0);
    gl_info->fbo_ops.glGenFramebuffers(1, &entry->id);
    checkGLcall("glGenFramebuffers()");
    TRACE("Created FBO %u.\n", entry->id);

    return entry;
}

/* Context activation is done by the caller. */
static void context_reuse_fbo_entry(struct wined3d_context *context, GLenum target,
        struct wined3d_rendertarget_info *render_targets, struct wined3d_surface *depth_stencil,
        DWORD color_location, DWORD ds_location, struct fbo_entry *entry)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    context_bind_fbo(context, target, entry->id);
    context_clean_fbo_attachments(gl_info, target);

    context_generate_fbo_key(context, &entry->key, render_targets, depth_stencil, color_location, ds_location);
    entry->flags = 0;
    if (depth_stencil)
    {
        if (depth_stencil->container->resource.format_flags & WINED3DFMT_FLAG_DEPTH)
            entry->flags |= WINED3D_FBO_ENTRY_FLAG_DEPTH;
        if (depth_stencil->container->resource.format_flags & WINED3DFMT_FLAG_STENCIL)
            entry->flags |= WINED3D_FBO_ENTRY_FLAG_STENCIL;
    }
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
    HeapFree(GetProcessHeap(), 0, entry);
}

/* Context activation is done by the caller. */
static struct fbo_entry *context_find_fbo_entry(struct wined3d_context *context, GLenum target,
        struct wined3d_rendertarget_info *render_targets, struct wined3d_surface *depth_stencil,
        DWORD color_location, DWORD ds_location)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    unsigned int object_count = gl_info->limits.buffers + 1;
    struct wined3d_texture *rt_texture, *ds_texture;
    struct fbo_entry *entry;
    unsigned int i, level;

    if (depth_stencil && render_targets[0].resource && render_targets[0].resource->type != WINED3D_RTYPE_BUFFER)
    {
        rt_texture = wined3d_texture_from_resource(render_targets[0].resource);
        level = render_targets[0].sub_resource_idx % rt_texture->level_count;
        ds_texture = depth_stencil->container;

        if (wined3d_texture_get_level_width(ds_texture, depth_stencil->texture_level)
                < wined3d_texture_get_level_width(rt_texture, level)
                || wined3d_texture_get_level_height(ds_texture, depth_stencil->texture_level)
                < wined3d_texture_get_level_height(rt_texture, level))
        {
            WARN("Depth stencil is smaller than the primary color buffer, disabling.\n");
            depth_stencil = NULL;
        }
        else if (ds_texture->resource.multisample_type != rt_texture->resource.multisample_type
                || ds_texture->resource.multisample_quality != rt_texture->resource.multisample_quality)
        {
            WARN("Color multisample type %u and quality %u, depth stencil has %u and %u, disabling ds buffer.\n",
                    rt_texture->resource.multisample_type, rt_texture->resource.multisample_quality,
                    ds_texture->resource.multisample_type, ds_texture->resource.multisample_quality);
            depth_stencil = NULL;
        }
        else
            surface_set_compatible_renderbuffer(depth_stencil, &render_targets[0]);
    }

    context_generate_fbo_key(context, context->fbo_key, render_targets, depth_stencil, color_location,
            ds_location);

    if (TRACE_ON(d3d))
    {
        TRACE("Dumping FBO attachments:\n");
        for (i = 0; i < gl_info->limits.buffers; ++i)
        {
            struct wined3d_resource *resource;
            if ((resource = render_targets[i].resource))
            {
                unsigned int width, height;
                const char *resource_type;

                if (resource->type == WINED3D_RTYPE_BUFFER)
                {
                    width = resource->size;
                    height = 1;
                    resource_type = "buffer";
                }
                else
                {
                    rt_texture = wined3d_texture_from_resource(resource);
                    level = render_targets[i].sub_resource_idx % rt_texture->level_count;
                    width = wined3d_texture_get_level_pow2_width(rt_texture, level);
                    height = wined3d_texture_get_level_pow2_height(rt_texture, level);
                    resource_type = "texture";
                }

                TRACE("    Color attachment %u: %p, %u format %s, %s %u, %ux%u, %u samples.\n",
                        i, resource, render_targets[i].sub_resource_idx, debug_d3dformat(resource->format->id),
                        context->fbo_key->rb_namespace & (1 << (i + 1)) ? "renderbuffer" : resource_type,
                        context->fbo_key->objects[i + 1].object, width, height, resource->multisample_type);
            }
        }
        if (depth_stencil)
        {
            ds_texture = depth_stencil->container;
            TRACE("    Depth attachment: %p format %s, %s %u, %ux%u, %u samples.\n",
                    depth_stencil, debug_d3dformat(ds_texture->resource.format->id),
                    context->fbo_key->rb_namespace & (1 << 0) ? "renderbuffer" : "texture",
                    context->fbo_key->objects[0].object,
                    wined3d_texture_get_level_pow2_width(ds_texture, depth_stencil->texture_level),
                    wined3d_texture_get_level_pow2_height(ds_texture, depth_stencil->texture_level),
                    ds_texture->resource.multisample_type);
        }
    }

    LIST_FOR_EACH_ENTRY(entry, &context->fbo_list, struct fbo_entry, entry)
    {
        if (memcmp(context->fbo_key, &entry->key, FIELD_OFFSET(struct wined3d_fbo_entry_key, objects[object_count])))
            continue;

        list_remove(&entry->entry);
        list_add_head(&context->fbo_list, &entry->entry);
        return entry;
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

    if (entry->flags & WINED3D_FBO_ENTRY_FLAG_ATTACHED)
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
        context_attach_surface_fbo(context, target, i, &entry->key.objects[i + 1],
                entry->key.rb_namespace & (1 << (i + 1)));
    }

    context_attach_depth_stencil_fbo(context, target, &entry->key.objects[0],
            entry->key.rb_namespace & 0x1, entry->flags);

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

    entry->flags |= WINED3D_FBO_ENTRY_FLAG_ATTACHED;
}

/* Context activation is done by the caller. */
static void context_apply_fbo_state(struct wined3d_context *context, GLenum target,
        struct wined3d_rendertarget_info *render_targets, struct wined3d_surface *depth_stencil,
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
    memset(context->blit_targets, 0, context->gl_info->limits.buffers * sizeof(*context->blit_targets));
    if (render_target)
    {
        context->blit_targets[0].resource = &render_target->container->resource;
        context->blit_targets[0].sub_resource_idx = surface_get_sub_resource_idx(render_target);
        context->blit_targets[0].layer_count = 1;
    }
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

    if (!wined3d_array_reserve((void **)&context->free_occlusion_queries,
            &context->free_occlusion_query_size, context->free_occlusion_query_count + 1,
            sizeof(*context->free_occlusion_queries)))
    {
        ERR("Failed to grow free list, leaking query %u in context %p.\n", query->id, context);
        return;
    }

    context->free_occlusion_queries[context->free_occlusion_query_count++] = query->id;
}

/* Context activation is done by the caller. */
void context_alloc_fence(struct wined3d_context *context, struct wined3d_fence *fence)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    if (context->free_fence_count)
    {
        fence->object = context->free_fences[--context->free_fence_count];
    }
    else
    {
        if (gl_info->supported[ARB_SYNC])
        {
            /* Using ARB_sync, not much to do here. */
            fence->object.sync = NULL;
            TRACE("Allocated sync object in context %p.\n", context);
        }
        else if (gl_info->supported[APPLE_FENCE])
        {
            GL_EXTCALL(glGenFencesAPPLE(1, &fence->object.id));
            checkGLcall("glGenFencesAPPLE");

            TRACE("Allocated fence %u in context %p.\n", fence->object.id, context);
        }
        else if(gl_info->supported[NV_FENCE])
        {
            GL_EXTCALL(glGenFencesNV(1, &fence->object.id));
            checkGLcall("glGenFencesNV");

            TRACE("Allocated fence %u in context %p.\n", fence->object.id, context);
        }
        else
        {
            WARN("Fences not supported, not allocating fence.\n");
            fence->object.id = 0;
        }
    }

    fence->context = context;
    list_add_head(&context->fences, &fence->entry);
}

void context_free_fence(struct wined3d_fence *fence)
{
    struct wined3d_context *context = fence->context;

    list_remove(&fence->entry);
    fence->context = NULL;

    if (!wined3d_array_reserve((void **)&context->free_fences,
            &context->free_fence_size, context->free_fence_count + 1,
            sizeof(*context->free_fences)))
    {
        ERR("Failed to grow free list, leaking fence %u in context %p.\n", fence->object.id, context);
        return;
    }

    context->free_fences[context->free_fence_count++] = fence->object;
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

    if (!wined3d_array_reserve((void **)&context->free_timestamp_queries,
            &context->free_timestamp_query_size, context->free_timestamp_query_count + 1,
            sizeof(*context->free_timestamp_queries)))
    {
        ERR("Failed to grow free list, leaking query %u in context %p.\n", query->id, context);
        return;
    }

    context->free_timestamp_queries[context->free_timestamp_query_count++] = query->id;
}

void context_alloc_so_statistics_query(struct wined3d_context *context,
        struct wined3d_so_statistics_query *query)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    if (context->free_so_statistics_query_count)
    {
        query->u = context->free_so_statistics_queries[--context->free_so_statistics_query_count];
    }
    else
    {
        GL_EXTCALL(glGenQueries(ARRAY_SIZE(query->u.id), query->u.id));
        checkGLcall("glGenQueries");

        TRACE("Allocated SO statistics queries %u, %u in context %p.\n",
                query->u.id[0], query->u.id[1], context);
    }

    query->context = context;
    list_add_head(&context->so_statistics_queries, &query->entry);
}

void context_free_so_statistics_query(struct wined3d_so_statistics_query *query)
{
    struct wined3d_context *context = query->context;

    list_remove(&query->entry);
    query->context = NULL;

    if (!wined3d_array_reserve((void **)&context->free_so_statistics_queries,
            &context->free_so_statistics_query_size, context->free_so_statistics_query_count + 1,
            sizeof(*context->free_so_statistics_queries)))
    {
        ERR("Failed to grow free list, leaking GL queries %u, %u in context %p.\n",
                query->u.id[0], query->u.id[1], context);
        return;
    }

    context->free_so_statistics_queries[context->free_so_statistics_query_count++] = query->u;
}

void context_alloc_pipeline_statistics_query(struct wined3d_context *context,
        struct wined3d_pipeline_statistics_query *query)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    if (context->free_pipeline_statistics_query_count)
    {
        query->u = context->free_pipeline_statistics_queries[--context->free_pipeline_statistics_query_count];
    }
    else
    {
        GL_EXTCALL(glGenQueries(ARRAY_SIZE(query->u.id), query->u.id));
        checkGLcall("glGenQueries");
    }

    query->context = context;
    list_add_head(&context->pipeline_statistics_queries, &query->entry);
}

void context_free_pipeline_statistics_query(struct wined3d_pipeline_statistics_query *query)
{
    struct wined3d_context *context = query->context;

    list_remove(&query->entry);
    query->context = NULL;

    if (!wined3d_array_reserve((void **)&context->free_pipeline_statistics_queries,
            &context->free_pipeline_statistics_query_size, context->free_pipeline_statistics_query_count + 1,
            sizeof(*context->free_pipeline_statistics_queries)))
    {
        ERR("Failed to grow free list, leaking GL queries in context %p.\n", context);
        return;
    }

    context->free_pipeline_statistics_queries[context->free_pipeline_statistics_query_count++] = query->u;
}

typedef void (context_fbo_entry_func_t)(struct wined3d_context *context, struct fbo_entry *entry);

static void context_enum_fbo_entries(const struct wined3d_device *device,
        GLuint name, BOOL rb_namespace, context_fbo_entry_func_t *callback)
{
    UINT i;

    for (i = 0; i < device->context_count; ++i)
    {
        struct wined3d_context *context = device->contexts[i];
        const struct wined3d_gl_info *gl_info = context->gl_info;
        struct fbo_entry *entry, *entry2;

        LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &context->fbo_list, struct fbo_entry, entry)
        {
            UINT j;

            for (j = 0; j < gl_info->limits.buffers + 1; ++j)
            {
                if (entry->key.objects[j].object == name
                        && !(entry->key.rb_namespace & (1 << j)) == !rb_namespace)
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
    struct wined3d_texture *texture;
    UINT i;

    if (!device->d3d_initialized)
        return;

    switch (type)
    {
        case WINED3D_RTYPE_TEXTURE_2D:
        case WINED3D_RTYPE_TEXTURE_3D:
            texture = texture_from_resource(resource);

            for (i = 0; i < device->context_count; ++i)
            {
                struct wined3d_context *context = device->contexts[i];
                if (context->current_rt.texture == texture)
                {
                    context->current_rt.texture = NULL;
                    context->current_rt.sub_resource_idx = 0;
                }
            }
            break;

        default:
            break;
    }
}

void context_gl_resource_released(struct wined3d_device *device,
        GLuint name, BOOL rb_namespace)
{
    context_enum_fbo_entries(device, name, rb_namespace, context_queue_fbo_entry_destruction);
}

void context_surface_update(struct wined3d_context *context, const struct wined3d_surface *surface)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct fbo_entry *entry = context->current_fbo;
    unsigned int i;

    if (!entry || context->rebind_fbo) return;

    for (i = 0; i < gl_info->limits.buffers + 1; ++i)
    {
        if (surface->container->texture_rgb.name == entry->key.objects[i].object
                || surface->container->texture_srgb.name == entry->key.objects[i].object)
        {
            TRACE("Updated surface %p is bound as attachment %u to the current FBO.\n", surface, i);
            context->rebind_fbo = TRUE;
            return;
        }
    }
}

static BOOL context_restore_pixel_format(struct wined3d_context *ctx)
{
    const struct wined3d_gl_info *gl_info = ctx->gl_info;
    BOOL ret = FALSE;

    if (ctx->restore_pf && IsWindow(ctx->restore_pf_win))
    {
        if (ctx->gl_info->supported[WGL_WINE_PIXEL_FORMAT_PASSTHROUGH])
        {
            HDC dc = GetDCEx(ctx->restore_pf_win, 0, DCX_USESTYLE | DCX_CACHE);
            if (dc)
            {
                if (!(ret = GL_EXTCALL(wglSetPixelFormatWINE(dc, ctx->restore_pf))))
                {
                    ERR("wglSetPixelFormatWINE failed to restore pixel format %d on window %p.\n",
                            ctx->restore_pf, ctx->restore_pf_win);
                }
                ReleaseDC(ctx->restore_pf_win, dc);
            }
        }
        else
        {
            ERR("can't restore pixel format %d on window %p\n", ctx->restore_pf, ctx->restore_pf_win);
        }
    }

    ctx->restore_pf = 0;
    ctx->restore_pf_win = NULL;
    return ret;
}

static BOOL context_set_pixel_format(struct wined3d_context *context)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    BOOL private = context->hdc_is_private;
    int format = context->pixel_format;
    HDC dc = context->hdc;
    int current;

    if (private && context->hdc_has_format)
        return TRUE;

    if (!private && WindowFromDC(dc) != context->win_handle)
        return FALSE;

    current = gl_info->gl_ops.wgl.p_wglGetPixelFormat(dc);
    if (current == format) goto success;

    if (!current)
    {
        if (!SetPixelFormat(dc, format, NULL))
        {
            /* This may also happen if the dc belongs to a destroyed window. */
            WARN("Failed to set pixel format %d on device context %p, last error %#x.\n",
                    format, dc, GetLastError());
            return FALSE;
        }

        context->restore_pf = 0;
        context->restore_pf_win = private ? NULL : WindowFromDC(dc);
        goto success;
    }

    /* By default WGL doesn't allow pixel format adjustments but we need it
     * here. For this reason there's a Wine specific wglSetPixelFormat()
     * which allows us to set the pixel format multiple times. Only use it
     * when really needed. */
    if (gl_info->supported[WGL_WINE_PIXEL_FORMAT_PASSTHROUGH])
    {
        HWND win;

        if (!GL_EXTCALL(wglSetPixelFormatWINE(dc, format)))
        {
            ERR("wglSetPixelFormatWINE failed to set pixel format %d on device context %p.\n",
                    format, dc);
            return FALSE;
        }

        win = private ? NULL : WindowFromDC(dc);
        if (win != context->restore_pf_win)
        {
            context_restore_pixel_format(context);

            context->restore_pf = private ? 0 : current;
            context->restore_pf_win = win;
        }

        goto success;
    }

    /* OpenGL doesn't allow pixel format adjustments. Print an error and
     * continue using the old format. There's a big chance that the old
     * format works although with a performance hit and perhaps rendering
     * errors. */
    ERR("Unable to set pixel format %d on device context %p. Already using format %d.\n",
            format, dc, current);
    return TRUE;

success:
    if (private)
        context->hdc_has_format = TRUE;
    return TRUE;
}

static BOOL context_set_gl_context(struct wined3d_context *ctx)
{
    struct wined3d_swapchain *swapchain = ctx->swapchain;
    BOOL backup = FALSE;

    if (!context_set_pixel_format(ctx))
    {
        WARN("Failed to set pixel format %d on device context %p.\n",
                ctx->pixel_format, ctx->hdc);
        backup = TRUE;
    }

    if (backup || !wglMakeCurrent(ctx->hdc, ctx->glCtx))
    {
        WARN("Failed to make GL context %p current on device context %p, last error %#x.\n",
                ctx->glCtx, ctx->hdc, GetLastError());
        ctx->valid = 0;
        WARN("Trying fallback to the backup window.\n");

        /* FIXME: If the context is destroyed it's no longer associated with
         * a swapchain, so we can't use the swapchain to get a backup dc. To
         * make this work windowless contexts would need to be handled by the
         * device. */
        if (ctx->destroyed || !swapchain)
        {
            FIXME("Unable to get backup dc for destroyed context %p.\n", ctx);
            context_set_current(NULL);
            return FALSE;
        }

        if (!(ctx->hdc = swapchain_get_backup_dc(swapchain)))
        {
            context_set_current(NULL);
            return FALSE;
        }

        ctx->hdc_is_private = TRUE;
        ctx->hdc_has_format = FALSE;

        if (!context_set_pixel_format(ctx))
        {
            ERR("Failed to set pixel format %d on device context %p.\n",
                    ctx->pixel_format, ctx->hdc);
            context_set_current(NULL);
            return FALSE;
        }

        if (!wglMakeCurrent(ctx->hdc, ctx->glCtx))
        {
            ERR("Fallback to backup window (dc %p) failed too, last error %#x.\n",
                    ctx->hdc, GetLastError());
            context_set_current(NULL);
            return FALSE;
        }

        ctx->valid = 1;
    }
    ctx->needs_set = 0;
    return TRUE;
}

static void context_restore_gl_context(const struct wined3d_gl_info *gl_info, HDC dc, HGLRC gl_ctx)
{
    if (!wglMakeCurrent(dc, gl_ctx))
    {
        ERR("Failed to restore GL context %p on device context %p, last error %#x.\n",
                gl_ctx, dc, GetLastError());
        context_set_current(NULL);
    }
}

static void context_update_window(struct wined3d_context *context)
{
    if (!context->swapchain)
        return;

    if (context->win_handle == context->swapchain->win_handle)
        return;

    TRACE("Updating context %p window from %p to %p.\n",
            context, context->win_handle, context->swapchain->win_handle);

    if (context->hdc)
        wined3d_release_dc(context->win_handle, context->hdc);

    context->win_handle = context->swapchain->win_handle;
    context->hdc_is_private = FALSE;
    context->hdc_has_format = FALSE;
    context->needs_set = 1;
    context->valid = 1;

    if (!(context->hdc = GetDCEx(context->win_handle, 0, DCX_USESTYLE | DCX_CACHE)))
    {
        ERR("Failed to get a device context for window %p.\n", context->win_handle);
        context->valid = 0;
    }
}

static void context_destroy_gl_resources(struct wined3d_context *context)
{
    struct wined3d_pipeline_statistics_query *pipeline_statistics_query;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct wined3d_so_statistics_query *so_statistics_query;
    struct wined3d_timestamp_query *timestamp_query;
    struct wined3d_occlusion_query *occlusion_query;
    struct fbo_entry *entry, *entry2;
    struct wined3d_fence *fence;
    HGLRC restore_ctx;
    HDC restore_dc;
    unsigned int i;

    restore_ctx = wglGetCurrentContext();
    restore_dc = wglGetCurrentDC();

    if (restore_ctx == context->glCtx)
        restore_ctx = NULL;
    else if (context->valid)
        context_set_gl_context(context);

    LIST_FOR_EACH_ENTRY(so_statistics_query, &context->so_statistics_queries,
            struct wined3d_so_statistics_query, entry)
    {
        if (context->valid)
            GL_EXTCALL(glDeleteQueries(ARRAY_SIZE(so_statistics_query->u.id), so_statistics_query->u.id));
        so_statistics_query->context = NULL;
    }

    LIST_FOR_EACH_ENTRY(pipeline_statistics_query, &context->pipeline_statistics_queries,
            struct wined3d_pipeline_statistics_query, entry)
    {
        if (context->valid)
            GL_EXTCALL(glDeleteQueries(ARRAY_SIZE(pipeline_statistics_query->u.id), pipeline_statistics_query->u.id));
        pipeline_statistics_query->context = NULL;
    }

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

    LIST_FOR_EACH_ENTRY(fence, &context->fences, struct wined3d_fence, entry)
    {
        if (context->valid)
        {
            if (gl_info->supported[ARB_SYNC])
            {
                if (fence->object.sync)
                    GL_EXTCALL(glDeleteSync(fence->object.sync));
            }
            else if (gl_info->supported[APPLE_FENCE])
            {
                GL_EXTCALL(glDeleteFencesAPPLE(1, &fence->object.id));
            }
            else if (gl_info->supported[NV_FENCE])
            {
                GL_EXTCALL(glDeleteFencesNV(1, &fence->object.id));
            }
        }
        fence->context = NULL;
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

        if (gl_info->supported[WINED3D_GL_PRIMITIVE_QUERY])
        {
            for (i = 0; i < context->free_so_statistics_query_count; ++i)
            {
                union wined3d_gl_so_statistics_query *q = &context->free_so_statistics_queries[i];
                GL_EXTCALL(glDeleteQueries(ARRAY_SIZE(q->id), q->id));
            }
        }

        if (gl_info->supported[ARB_PIPELINE_STATISTICS_QUERY])
        {
            for (i = 0; i < context->free_pipeline_statistics_query_count; ++i)
            {
                union wined3d_gl_pipeline_statistics_query *q = &context->free_pipeline_statistics_queries[i];
                GL_EXTCALL(glDeleteQueries(ARRAY_SIZE(q->id), q->id));
            }
        }

        if (gl_info->supported[ARB_TIMER_QUERY])
            GL_EXTCALL(glDeleteQueries(context->free_timestamp_query_count, context->free_timestamp_queries));

        if (gl_info->supported[ARB_OCCLUSION_QUERY])
            GL_EXTCALL(glDeleteQueries(context->free_occlusion_query_count, context->free_occlusion_queries));

        if (gl_info->supported[ARB_SYNC])
        {
            for (i = 0; i < context->free_fence_count; ++i)
            {
                GL_EXTCALL(glDeleteSync(context->free_fences[i].sync));
            }
        }
        else if (gl_info->supported[APPLE_FENCE])
        {
            for (i = 0; i < context->free_fence_count; ++i)
            {
                GL_EXTCALL(glDeleteFencesAPPLE(1, &context->free_fences[i].id));
            }
        }
        else if (gl_info->supported[NV_FENCE])
        {
            for (i = 0; i < context->free_fence_count; ++i)
            {
                GL_EXTCALL(glDeleteFencesNV(1, &context->free_fences[i].id));
            }
        }

        checkGLcall("context cleanup");
    }

    HeapFree(GetProcessHeap(), 0, context->free_so_statistics_queries);
    HeapFree(GetProcessHeap(), 0, context->free_pipeline_statistics_queries);
    HeapFree(GetProcessHeap(), 0, context->free_timestamp_queries);
    HeapFree(GetProcessHeap(), 0, context->free_occlusion_queries);
    HeapFree(GetProcessHeap(), 0, context->free_fences);

    context_restore_pixel_format(context);
    if (restore_ctx)
    {
        context_restore_gl_context(gl_info, restore_dc, restore_ctx);
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
            if (wglGetCurrentContext())
            {
                const struct wined3d_gl_info *gl_info = old->gl_info;
                TRACE("Flushing context %p before switching to %p.\n", old, ctx);
                gl_info->gl_ops.gl.p_glFlush();
            }
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
    else if (wglGetCurrentContext())
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

    if (!--context->level)
    {
        if (context_restore_pixel_format(context))
            context->needs_set = 1;
        if (context->restore_ctx)
        {
            TRACE("Restoring GL context %p on device context %p.\n", context->restore_ctx, context->restore_dc);
            context_restore_gl_context(context->gl_info, context->restore_dc, context->restore_ctx);
            context->restore_ctx = NULL;
            context->restore_dc = NULL;
        }

        if (context->destroy_delayed)
        {
            TRACE("Destroying context %p.\n", context);
            context_destroy(context->device, context);
        }
    }
}

/* This is used when a context for render target A is active, but a separate context is
 * needed to access the WGL framebuffer for render target B. Re-acquire a context for rt
 * A to avoid breaking caller code. */
void context_restore(struct wined3d_context *context, struct wined3d_surface *restore)
{
    if (context->current_rt.texture != restore->container
            || context->current_rt.sub_resource_idx != surface_get_sub_resource_idx(restore))
    {
        context_release(context);
        context = context_acquire(restore->container->resource.device,
                restore->container, surface_get_sub_resource_idx(restore));
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
            context->needs_set = 1;
        }
        else if (!context->needs_set && !(context->hdc_is_private && context->hdc_has_format)
                    && context->pixel_format != context->gl_info->gl_ops.wgl.p_wglGetPixelFormat(context->hdc))
            context->needs_set = 1;
    }
}

void context_invalidate_compute_state(struct wined3d_context *context, DWORD state_id)
{
    DWORD representative = context->state_table[state_id].representative - STATE_COMPUTE_OFFSET;
    unsigned int index, shift;

    index = representative / (sizeof(*context->dirty_compute_states) * CHAR_BIT);
    shift = representative & (sizeof(*context->dirty_compute_states) * CHAR_BIT - 1);
    context->dirty_compute_states[index] |= (1u << shift);
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
        BOOL auxBuffers)
{
    unsigned int cfg_count = device->adapter->cfg_count;
    unsigned int current_value;
    PIXELFORMATDESCRIPTOR pfd;
    int iPixelFormat = 0;
    unsigned int i;

    TRACE("device %p, dc %p, color_format %s, ds_format %s, aux_buffers %#x.\n",
            device, hdc, debug_d3dformat(color_format->id), debug_d3dformat(ds_format->id),
            auxBuffers);

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

    if (!iPixelFormat)
    {
        ERR("Trying to locate a compatible pixel format because an exact match failed.\n");

        memset(&pfd, 0, sizeof(pfd));
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

        if (!(iPixelFormat = ChoosePixelFormat(hdc, &pfd)))
        {
            /* Something is very wrong as ChoosePixelFormat() barely fails. */
            ERR("Can't find a suitable pixel format.\n");
            return 0;
        }
    }

    TRACE("Found iPixelFormat=%d for ColorFormat=%s, DepthStencilFormat=%s.\n",
            iPixelFormat, debug_d3dformat(color_format->id), debug_d3dformat(ds_format->id));
    return iPixelFormat;
}

/* Context activation is done by the caller. */
void context_bind_dummy_textures(const struct wined3d_device *device, const struct wined3d_context *context)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    unsigned int i;

    for (i = 0; i < gl_info->limits.combined_samplers; ++i)
    {
        GL_EXTCALL(glActiveTexture(GL_TEXTURE0 + i));
        checkGLcall("glActiveTexture");

        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_1D, device->dummy_textures.tex_1d);
        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, device->dummy_textures.tex_2d);

        if (gl_info->supported[ARB_TEXTURE_RECTANGLE])
            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_RECTANGLE_ARB, device->dummy_textures.tex_rect);

        if (gl_info->supported[EXT_TEXTURE3D])
            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_3D, device->dummy_textures.tex_3d);

        if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_CUBE_MAP, device->dummy_textures.tex_cube);

        if (gl_info->supported[ARB_TEXTURE_CUBE_MAP_ARRAY])
            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, device->dummy_textures.tex_cube_array);

        if (gl_info->supported[EXT_TEXTURE_ARRAY])
        {
            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_1D_ARRAY, device->dummy_textures.tex_1d_array);
            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D_ARRAY, device->dummy_textures.tex_2d_array);
        }

        if (gl_info->supported[ARB_TEXTURE_BUFFER_OBJECT])
            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_BUFFER, device->dummy_textures.tex_buffer);

        checkGLcall("Bind dummy textures");
    }
}

void wined3d_check_gl_call(const struct wined3d_gl_info *gl_info,
        const char *file, unsigned int line, const char *name)
{
    GLint err;

    if (gl_info->supported[ARB_DEBUG_OUTPUT] || (err = gl_info->gl_ops.gl.p_glGetError()) == GL_NO_ERROR)
    {
        TRACE("%s call ok %s / %u.\n", name, file, line);
        return;
    }

    do
    {
        ERR(">>>>>>> %s (%#x) from %s @ %s / %u.\n",
                debug_glerror(err), err, name, file,line);
        err = gl_info->gl_ops.gl.p_glGetError();
    } while (err != GL_NO_ERROR);
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
        struct wined3d_texture *target, const struct wined3d_format *ds_format)
{
    struct wined3d_device *device = swapchain->device;
    const struct wined3d_d3d_info *d3d_info = &device->adapter->d3d_info;
    const struct wined3d_gl_info *gl_info = &device->adapter->gl_info;
    const struct wined3d_format *color_format;
    struct wined3d_context *ret;
    BOOL auxBuffers = FALSE;
    HGLRC ctx, share_ctx;
    DWORD target_usage;
    unsigned int i;
    DWORD state;

    TRACE("swapchain %p, target %p, window %p.\n", swapchain, target, swapchain->win_handle);

    wined3d_from_cs(device->cs);

    ret = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*ret));
    if (!ret)
        return NULL;

    if (!(ret->blit_targets = wined3d_calloc(gl_info->limits.buffers, sizeof(*ret->blit_targets))))
        goto out;

    if (!(ret->draw_buffers = wined3d_calloc(gl_info->limits.buffers, sizeof(*ret->draw_buffers))))
        goto out;

    ret->fbo_key = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            FIELD_OFFSET(struct wined3d_fbo_entry_key, objects[gl_info->limits.buffers + 1]));
    if (!ret->fbo_key)
        goto out;

    ret->free_timestamp_query_size = 4;
    if (!(ret->free_timestamp_queries = wined3d_calloc(ret->free_timestamp_query_size,
            sizeof(*ret->free_timestamp_queries))))
        goto out;
    list_init(&ret->timestamp_queries);

    ret->free_occlusion_query_size = 4;
    if (!(ret->free_occlusion_queries = wined3d_calloc(ret->free_occlusion_query_size,
            sizeof(*ret->free_occlusion_queries))))
        goto out;
    list_init(&ret->occlusion_queries);

    ret->free_fence_size = 4;
    if (!(ret->free_fences = wined3d_calloc(ret->free_fence_size, sizeof(*ret->free_fences))))
        goto out;
    list_init(&ret->fences);

    list_init(&ret->so_statistics_queries);

    list_init(&ret->pipeline_statistics_queries);

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

    for (i = 0; i < ARRAY_SIZE(ret->tex_unit_map); ++i)
        ret->tex_unit_map[i] = WINED3D_UNMAPPED_STAGE;
    for (i = 0; i < ARRAY_SIZE(ret->rev_tex_unit_map); ++i)
        ret->rev_tex_unit_map[i] = WINED3D_UNMAPPED_STAGE;
    if (gl_info->limits.graphics_samplers >= MAX_COMBINED_SAMPLERS)
    {
        /* Initialize the texture unit mapping to a 1:1 mapping. */
        unsigned int base, count;

        wined3d_gl_limits_get_texture_unit_range(&gl_info->limits, WINED3D_SHADER_TYPE_PIXEL, &base, &count);
        if (base + MAX_FRAGMENT_SAMPLERS > ARRAY_SIZE(ret->rev_tex_unit_map))
        {
            ERR("Unexpected texture unit base index %u.\n", base);
            goto out;
        }
        for (i = 0; i < min(count, MAX_FRAGMENT_SAMPLERS); ++i)
        {
            ret->tex_unit_map[i] = base + i;
            ret->rev_tex_unit_map[base + i] = i;
        }

        wined3d_gl_limits_get_texture_unit_range(&gl_info->limits, WINED3D_SHADER_TYPE_VERTEX, &base, &count);
        if (base + MAX_VERTEX_SAMPLERS > ARRAY_SIZE(ret->rev_tex_unit_map))
        {
            ERR("Unexpected texture unit base index %u.\n", base);
            goto out;
        }
        for (i = 0; i < min(count, MAX_VERTEX_SAMPLERS); ++i)
        {
            ret->tex_unit_map[MAX_FRAGMENT_SAMPLERS + i] = base + i;
            ret->rev_tex_unit_map[base + i] = MAX_FRAGMENT_SAMPLERS + i;
        }
    }

    if (!(ret->texture_type = wined3d_calloc(gl_info->limits.combined_samplers,
            sizeof(*ret->texture_type))))
        goto out;

    if (!(ret->hdc = GetDCEx(swapchain->win_handle, 0, DCX_USESTYLE | DCX_CACHE)))
    {
        WARN("Failed to retrieve device context, trying swapchain backup.\n");

        if ((ret->hdc = swapchain_get_backup_dc(swapchain)))
            ret->hdc_is_private = TRUE;
        else
        {
            ERR("Failed to retrieve a device context.\n");
            goto out;
        }
    }

    color_format = target->resource.format;
    target_usage = target->resource.usage;

    /* In case of ORM_BACKBUFFER, make sure to request an alpha component for
     * X4R4G4B4/X8R8G8B8 as we might need it for the backbuffer. */
    if (wined3d_settings.offscreen_rendering_mode == ORM_BACKBUFFER)
    {
        auxBuffers = TRUE;

        if (color_format->id == WINED3DFMT_B4G4R4X4_UNORM)
            color_format = wined3d_get_format(gl_info, WINED3DFMT_B4G4R4A4_UNORM, target_usage);
        else if (color_format->id == WINED3DFMT_B8G8R8X8_UNORM)
            color_format = wined3d_get_format(gl_info, WINED3DFMT_B8G8R8A8_UNORM, target_usage);
    }

    /* DirectDraw supports 8bit paletted render targets and these are used by
     * old games like StarCraft and C&C. Most modern hardware doesn't support
     * 8bit natively so we perform some form of 8bit -> 32bit conversion. The
     * conversion (ab)uses the alpha component for storing the palette index.
     * For this reason we require a format with 8bit alpha, so request
     * A8R8G8B8. */
    if (color_format->id == WINED3DFMT_P8_UINT)
        color_format = wined3d_get_format(gl_info, WINED3DFMT_B8G8R8A8_UNORM, target_usage);

    /* When using FBOs for off-screen rendering, we only use the drawable for
     * presentation blits, and don't do any rendering to it. That means we
     * don't need depth or stencil buffers, and can mostly ignore the render
     * target format. This wouldn't necessarily be quite correct for 10bpc
     * display modes, but we don't currently support those.
     * Using the same format regardless of the color/depth/stencil targets
     * makes it much less likely that different wined3d instances will set
     * conflicting pixel formats. */
    if (wined3d_settings.offscreen_rendering_mode != ORM_BACKBUFFER)
    {
        color_format = wined3d_get_format(gl_info, WINED3DFMT_B8G8R8A8_UNORM, target_usage);
        ds_format = wined3d_get_format(gl_info, WINED3DFMT_UNKNOWN, WINED3DUSAGE_DEPTHSTENCIL);
    }

    /* Try to find a pixel format which matches our requirements. */
    if (!(ret->pixel_format = context_choose_pixel_format(device, ret->hdc, color_format, ds_format, auxBuffers)))
        goto out;

    ret->gl_info = gl_info;
    ret->win_handle = swapchain->win_handle;

    context_enter(ret);

    if (!context_set_pixel_format(ret))
    {
        ERR("Failed to set pixel format %d on device context %p.\n", ret->pixel_format, ret->hdc);
        context_release(ret);
        goto out;
    }

    share_ctx = device->context_count ? device->contexts[0]->glCtx : NULL;
    if (gl_info->p_wglCreateContextAttribsARB)
    {
        if (!(ctx = context_create_wgl_attribs(gl_info, ret->hdc, share_ctx)))
            goto out;
    }
    else
    {
        if (!(ctx = wglCreateContext(ret->hdc)))
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

    ret->d3d_info = d3d_info;
    ret->state_table = device->StateTable;

    /* Mark all states dirty to force a proper initialization of the states on
     * the first use of the context. Compute states do not need initialization. */
    for (state = 0; state <= STATE_HIGHEST; ++state)
    {
        if (ret->state_table[state].representative && !STATE_IS_COMPUTE(state))
            context_invalidate_state(ret, state);
    }

    ret->device = device;
    ret->swapchain = swapchain;
    ret->current_rt.texture = target;
    ret->current_rt.sub_resource_idx = 0;
    ret->tid = GetCurrentThreadId();

    ret->render_offscreen = wined3d_resource_is_offscreen(&target->resource);
    ret->draw_buffers_mask = context_generate_rt_mask(GL_BACK);
    ret->valid = 1;

    ret->glCtx = ctx;
    ret->hdc_has_format = TRUE;
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

    if (gl_info->supported[WINED3D_GL_LEGACY_CONTEXT])
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_AUX_BUFFERS, &ret->aux_buffers);

    TRACE("Setting up the screen\n");

    if (gl_info->supported[WINED3D_GL_LEGACY_CONTEXT])
    {
        gl_info->gl_ops.gl.p_glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);
        checkGLcall("glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);");

        gl_info->gl_ops.gl.p_glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
        checkGLcall("glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);");

        gl_info->gl_ops.gl.p_glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
        checkGLcall("glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);");
    }
    else
    {
        GLuint vao;

        GL_EXTCALL(glGenVertexArrays(1, &vao));
        GL_EXTCALL(glBindVertexArray(vao));
        checkGLcall("creating VAO");
    }

    gl_info->gl_ops.gl.p_glPixelStorei(GL_PACK_ALIGNMENT, device->surface_alignment);
    checkGLcall("glPixelStorei(GL_PACK_ALIGNMENT, device->surface_alignment);");
    gl_info->gl_ops.gl.p_glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    checkGLcall("glPixelStorei(GL_UNPACK_ALIGNMENT, 1);");

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
        for (i = 1; i < gl_info->limits.textures; ++i)
        {
            context_active_texture(ret, gl_info, i);
            gl_info->gl_ops.gl.p_glTexEnvi(GL_TEXTURE_SHADER_NV,
                    GL_PREVIOUS_TEXTURE_INPUT_NV, GL_TEXTURE0_ARB + i - 1);
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
        for (i = 0; i < gl_info->limits.textures; ++i)
        {
            context_active_texture(ret, gl_info, i);
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
    if (!(d3d_info->wined3d_creation_flags & WINED3D_NO_PRIMITIVE_RESTART))
    {
        if (gl_info->supported[ARB_ES3_COMPATIBILITY])
        {
            gl_info->gl_ops.gl.p_glEnable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
            checkGLcall("enable GL_PRIMITIVE_RESTART_FIXED_INDEX");
        }
        else
        {
            FIXME("OpenGL implementation does not support GL_PRIMITIVE_RESTART_FIXED_INDEX.\n");
        }
    }
    if (!(d3d_info->wined3d_creation_flags & WINED3D_LEGACY_CUBEMAP_FILTERING)
            && gl_info->supported[ARB_SEAMLESS_CUBE_MAP])
    {
        gl_info->gl_ops.gl.p_glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        checkGLcall("enable seamless cube map filtering");
    }
    if (gl_info->supported[ARB_CLIP_CONTROL])
        GL_EXTCALL(glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, GL_LOWER_LEFT));
    device->shader_backend->shader_init_context_state(ret);
    ret->shader_update_mask = (1u << WINED3D_SHADER_TYPE_PIXEL)
            | (1u << WINED3D_SHADER_TYPE_VERTEX)
            | (1u << WINED3D_SHADER_TYPE_GEOMETRY)
            | (1u << WINED3D_SHADER_TYPE_HULL)
            | (1u << WINED3D_SHADER_TYPE_DOMAIN)
            | (1u << WINED3D_SHADER_TYPE_COMPUTE);

    /* If this happens to be the first context for the device, dummy textures
     * are not created yet. In that case, they will be created (and bound) by
     * create_dummy_textures right after this context is initialized. */
    if (device->dummy_textures.tex_2d)
        context_bind_dummy_textures(device, ret);

    TRACE("Created context %p.\n", ret);

    return ret;

out:
    if (ret->hdc)
        wined3d_release_dc(swapchain->win_handle, ret->hdc);
    device->shader_backend->shader_free_context_data(ret);
    device->adapter->fragment_pipe->free_context_data(ret);
    HeapFree(GetProcessHeap(), 0, ret->texture_type);
    HeapFree(GetProcessHeap(), 0, ret->free_fences);
    HeapFree(GetProcessHeap(), 0, ret->free_occlusion_queries);
    HeapFree(GetProcessHeap(), 0, ret->free_timestamp_queries);
    HeapFree(GetProcessHeap(), 0, ret->fbo_key);
    HeapFree(GetProcessHeap(), 0, ret->draw_buffers);
    HeapFree(GetProcessHeap(), 0, ret->blit_targets);
    HeapFree(GetProcessHeap(), 0, ret);
    return NULL;
}

void context_destroy(struct wined3d_device *device, struct wined3d_context *context)
{
    BOOL destroy;

    TRACE("Destroying ctx %p\n", context);

    wined3d_from_cs(device->cs);

    /* We delay destroying a context when it is active. The context_release()
     * function invokes context_destroy() again while leaving the last level. */
    if (context->level)
    {
        TRACE("Delaying destruction of context %p.\n", context);
        context->destroy_delayed = 1;
        /* FIXME: Get rid of a pointer to swapchain from wined3d_context. */
        context->swapchain = NULL;
        return;
    }

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
    HeapFree(GetProcessHeap(), 0, context->texture_type);
    HeapFree(GetProcessHeap(), 0, context->fbo_key);
    HeapFree(GetProcessHeap(), 0, context->draw_buffers);
    HeapFree(GetProcessHeap(), 0, context->blit_targets);
    device_context_remove(device, context);
    if (destroy) HeapFree(GetProcessHeap(), 0, context);
}

const DWORD *context_get_tex_unit_mapping(const struct wined3d_context *context,
        const struct wined3d_shader_version *shader_version, unsigned int *base, unsigned int *count)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    if (!shader_version)
    {
        *base = 0;
        *count = MAX_TEXTURES;
        return context->tex_unit_map;
    }

    if (shader_version->major >= 4)
    {
        wined3d_gl_limits_get_texture_unit_range(&gl_info->limits, shader_version->type, base, count);
        return NULL;
    }

    switch (shader_version->type)
    {
        case WINED3D_SHADER_TYPE_PIXEL:
            *base = 0;
            *count = MAX_FRAGMENT_SAMPLERS;
            break;
        case WINED3D_SHADER_TYPE_VERTEX:
            *base = MAX_FRAGMENT_SAMPLERS;
            *count = MAX_VERTEX_SAMPLERS;
            break;
        default:
            ERR("Unhandled shader type %#x.\n", shader_version->type);
            *base = 0;
            *count = 0;
    }

    return context->tex_unit_map;
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
    const struct wined3d_texture *rt = context->current_rt.texture;
    unsigned int level;

    if (rt->swapchain)
    {
        RECT window_size;

        GetClientRect(context->win_handle, &window_size);
        size->cx = window_size.right - window_size.left;
        size->cy = window_size.bottom - window_size.top;

        return;
    }

    level = context->current_rt.sub_resource_idx % rt->level_count;
    size->cx = wined3d_texture_get_level_width(rt, level);
    size->cy = wined3d_texture_get_level_height(rt, level);
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
    for (i = 0; i < MAX_RENDER_TARGETS; ++i)
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_COLORWRITE(i)));
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

    /* FIXME: Make draw_textured_quad() able to work with a upper left origin. */
    if (gl_info->supported[ARB_CLIP_CONTROL])
        GL_EXTCALL(glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE));

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

void context_bind_bo(struct wined3d_context *context, GLenum binding, GLuint name)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;

    if (binding == GL_ELEMENT_ARRAY_BUFFER)
        context_invalidate_state(context, STATE_INDEXBUFFER);

    GL_EXTCALL(glBindBuffer(binding, name));
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
        const struct wined3d_device *device = context->device;

        switch (old_texture_type)
        {
            case GL_NONE:
                /* nothing to do */
                break;
            case GL_TEXTURE_1D:
                gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_1D, device->dummy_textures.tex_1d);
                checkGLcall("glBindTexture");
                break;
            case GL_TEXTURE_1D_ARRAY:
                gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_1D_ARRAY, device->dummy_textures.tex_1d_array);
                checkGLcall("glBindTexture");
                break;
            case GL_TEXTURE_2D:
                gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, device->dummy_textures.tex_2d);
                checkGLcall("glBindTexture");
                break;
            case GL_TEXTURE_2D_ARRAY:
                gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D_ARRAY, device->dummy_textures.tex_2d_array);
                checkGLcall("glBindTexture");
                break;
            case GL_TEXTURE_RECTANGLE_ARB:
                gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_RECTANGLE_ARB, device->dummy_textures.tex_rect);
                checkGLcall("glBindTexture");
                break;
            case GL_TEXTURE_CUBE_MAP:
                gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_CUBE_MAP, device->dummy_textures.tex_cube);
                checkGLcall("glBindTexture");
                break;
            case GL_TEXTURE_CUBE_MAP_ARRAY:
                gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, device->dummy_textures.tex_cube_array);
                checkGLcall("glBindTexture");
                break;
            case GL_TEXTURE_3D:
                gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_3D, device->dummy_textures.tex_3d);
                checkGLcall("glBindTexture");
                break;
            case GL_TEXTURE_BUFFER:
                gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_BUFFER, device->dummy_textures.tex_buffer);
                checkGLcall("glBindTexture");
                break;
            default:
                ERR("Unexpected texture target %#x.\n", old_texture_type);
        }

        context->texture_type[unit] = target;
    }
}

void *context_map_bo_address(struct wined3d_context *context,
        const struct wined3d_bo_address *data, size_t size, GLenum binding, DWORD flags)
{
    const struct wined3d_gl_info *gl_info;
    BYTE *memory;

    if (!data->buffer_object)
        return data->addr;

    gl_info = context->gl_info;
    context_bind_bo(context, binding, data->buffer_object);

    if (gl_info->supported[ARB_MAP_BUFFER_RANGE])
    {
        GLbitfield map_flags = wined3d_resource_gl_map_flags(flags) & ~GL_MAP_FLUSH_EXPLICIT_BIT;
        memory = GL_EXTCALL(glMapBufferRange(binding, (INT_PTR)data->addr, size, map_flags));
    }
    else
    {
        memory = GL_EXTCALL(glMapBuffer(binding, wined3d_resource_gl_legacy_map_flags(flags)));
        memory += (INT_PTR)data->addr;
    }

    context_bind_bo(context, binding, 0);
    checkGLcall("Map buffer object");

    return memory;
}

void context_unmap_bo_address(struct wined3d_context *context,
        const struct wined3d_bo_address *data, GLenum binding)
{
    const struct wined3d_gl_info *gl_info;

    if (!data->buffer_object)
        return;

    gl_info = context->gl_info;
    context_bind_bo(context, binding, data->buffer_object);
    GL_EXTCALL(glUnmapBuffer(binding));
    context_bind_bo(context, binding, 0);
    checkGLcall("Unmap buffer object");
}

void context_copy_bo_address(struct wined3d_context *context,
        const struct wined3d_bo_address *dst, GLenum dst_binding,
        const struct wined3d_bo_address *src, GLenum src_binding, size_t size)
{
    const struct wined3d_gl_info *gl_info;
    BYTE *dst_ptr, *src_ptr;

    gl_info = context->gl_info;

    if (dst->buffer_object && src->buffer_object)
    {
        if (gl_info->supported[ARB_COPY_BUFFER])
        {
            GL_EXTCALL(glBindBuffer(GL_COPY_READ_BUFFER, src->buffer_object));
            GL_EXTCALL(glBindBuffer(GL_COPY_WRITE_BUFFER, dst->buffer_object));
            GL_EXTCALL(glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
                    (GLintptr)src->addr, (GLintptr)dst->addr, size));
            checkGLcall("direct buffer copy");
        }
        else
        {
            src_ptr = context_map_bo_address(context, src, size, src_binding, WINED3D_MAP_READONLY);
            dst_ptr = context_map_bo_address(context, dst, size, dst_binding, 0);

            memcpy(dst_ptr, src_ptr, size);

            context_unmap_bo_address(context, dst, dst_binding);
            context_unmap_bo_address(context, src, src_binding);
        }
    }
    else if (!dst->buffer_object && src->buffer_object)
    {
        context_bind_bo(context, src_binding, src->buffer_object);
        GL_EXTCALL(glGetBufferSubData(src_binding, (GLintptr)src->addr, size, dst->addr));
        checkGLcall("buffer download");
    }
    else if (dst->buffer_object && !src->buffer_object)
    {
        context_bind_bo(context, dst_binding, dst->buffer_object);
        GL_EXTCALL(glBufferSubData(dst_binding, (GLintptr)dst->addr, size, src->addr));
        checkGLcall("buffer upload");
    }
    else
    {
        memcpy(dst->addr, src->addr, size);
    }
}

static void context_set_render_offscreen(struct wined3d_context *context, BOOL offscreen)
{
    if (context->render_offscreen == offscreen)
        return;

    context_invalidate_state(context, STATE_VIEWPORT);
    context_invalidate_state(context, STATE_SCISSORRECT);
    if (!context->gl_info->supported[ARB_CLIP_CONTROL])
    {
        context_invalidate_state(context, STATE_FRONTFACE);
        context_invalidate_state(context, STATE_POINTSPRITECOORDORIGIN);
        context_invalidate_state(context, STATE_TRANSFORM(WINED3D_TS_PROJECTION));
    }
    context_invalidate_state(context, STATE_SHADER(WINED3D_SHADER_TYPE_DOMAIN));
    if (context->gl_info->supported[ARB_FRAGMENT_COORD_CONVENTIONS])
        context_invalidate_state(context, STATE_SHADER(WINED3D_SHADER_TYPE_PIXEL));
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
    struct wined3d_swapchain *swapchain = context->current_rt.texture->swapchain;

    if (context->render_offscreen || !depth_stencil) return;
    if (match_depth_stencil_format(swapchain->ds_format, depth_stencil->format)) return;

    /* TODO: If the requested format would satisfy the needs of the existing one(reverse match),
     * or no onscreen depth buffer was created, the OpenGL drawable could be changed to the new
     * format. */
    WARN("Depth stencil format is not supported by WGL, rendering the backbuffer in an FBO\n");

    /* The currently active context is the necessary context to access the swapchain's onscreen buffers */
    if (!(wined3d_texture_load_location(context->current_rt.texture, context->current_rt.sub_resource_idx,
            context, WINED3D_LOCATION_TEXTURE_RGB)))
        ERR("Failed to load location.\n");
    swapchain->render_to_fbo = TRUE;
    swapchain_update_draw_bindings(swapchain);
    context_set_render_offscreen(context, TRUE);
}

GLenum context_get_offscreen_gl_buffer(const struct wined3d_context *context)
{
    switch (wined3d_settings.offscreen_rendering_mode)
    {
        case ORM_FBO:
            return GL_COLOR_ATTACHMENT0;

        case ORM_BACKBUFFER:
            return context->aux_buffers > 0 ? GL_AUX0 : GL_BACK;

        default:
            FIXME("Unhandled offscreen rendering mode %#x.\n", wined3d_settings.offscreen_rendering_mode);
            return GL_BACK;
    }
}

static DWORD context_generate_rt_mask_no_fbo(const struct wined3d_context *context, struct wined3d_texture *rt)
{
    if (!rt || rt->resource.format->id == WINED3DFMT_NULL)
        return 0;
    else if (rt->swapchain)
        return context_generate_rt_mask_from_resource(&rt->resource);
    else
        return context_generate_rt_mask(context_get_offscreen_gl_buffer(context));
}

/* Context activation is done by the caller. */
void context_apply_blit_state(struct wined3d_context *context, const struct wined3d_device *device)
{
    struct wined3d_texture *rt = context->current_rt.texture;
    struct wined3d_surface *surface;
    DWORD rt_mask, *cur_mask;

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
    {
        context_validate_onscreen_formats(context, NULL);

        if (context->render_offscreen)
        {
            wined3d_texture_load(rt, context, FALSE);

            surface = rt->sub_resources[context->current_rt.sub_resource_idx].u.surface;
            context_apply_fbo_state_blit(context, GL_FRAMEBUFFER, surface, NULL, rt->resource.draw_binding);
            if (rt->resource.format->id != WINED3DFMT_NULL)
                rt_mask = 1;
            else
                rt_mask = 0;
        }
        else
        {
            context->current_fbo = NULL;
            context_bind_fbo(context, GL_FRAMEBUFFER, 0);
            rt_mask = context_generate_rt_mask_from_resource(&rt->resource);
        }
    }
    else
    {
        rt_mask = context_generate_rt_mask_no_fbo(context, rt);
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
BOOL context_apply_clear_state(struct wined3d_context *context, const struct wined3d_state *state,
        UINT rt_count, const struct wined3d_fb_state *fb)
{
    struct wined3d_rendertarget_view **rts = fb->render_targets;
    struct wined3d_rendertarget_view *dsv = fb->depth_stencil;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    DWORD rt_mask = 0, *cur_mask;
    unsigned int i;

    if (isStateDirty(context, STATE_FRAMEBUFFER) || fb != state->fb
            || rt_count != gl_info->limits.buffers)
    {
        if (!context_validate_rt_config(rt_count, rts, dsv))
            return FALSE;

        if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
        {
            context_validate_onscreen_formats(context, dsv);

            if (!rt_count || wined3d_resource_is_offscreen(rts[0]->resource))
            {
                memset(context->blit_targets, 0, gl_info->limits.buffers * sizeof(*context->blit_targets));
                for (i = 0; i < rt_count; ++i)
                {
                    if (rts[i])
                    {
                        context->blit_targets[i].gl_view = rts[i]->gl_view;
                        context->blit_targets[i].resource = rts[i]->resource;
                        context->blit_targets[i].sub_resource_idx = rts[i]->sub_resource_idx;
                        context->blit_targets[i].layer_count = rts[i]->layer_count;
                    }
                    if (rts[i] && rts[i]->format->id != WINED3DFMT_NULL)
                        rt_mask |= (1u << i);
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
                rt_mask = context_generate_rt_mask_from_resource(rts[0]->resource);
            }

            /* If the framebuffer is not the device's fb the device's fb has to be reapplied
             * next draw. Otherwise we could mark the framebuffer state clean here, once the
             * state management allows this */
            context_invalidate_state(context, STATE_FRAMEBUFFER);
        }
        else
        {
            rt_mask = context_generate_rt_mask_no_fbo(context,
                    rt_count ? wined3d_rendertarget_view_get_surface(rts[0])->container : NULL);
        }
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
        rt_mask = context_generate_rt_mask_no_fbo(context,
                rt_count ? wined3d_rendertarget_view_get_surface(rts[0])->container : NULL);
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

    context->last_was_blit = FALSE;

    /* Blending and clearing should be orthogonal, but tests on the nvidia
     * driver show that disabling blending when clearing improves the clearing
     * performance incredibly. */
    gl_info->gl_ops.gl.p_glDisable(GL_BLEND);
    gl_info->gl_ops.gl.p_glEnable(GL_SCISSOR_TEST);
    if (rt_count && gl_info->supported[ARB_FRAMEBUFFER_SRGB])
    {
        if (needs_srgb_write(context, state, fb))
            gl_info->gl_ops.gl.p_glEnable(GL_FRAMEBUFFER_SRGB);
        else
            gl_info->gl_ops.gl.p_glDisable(GL_FRAMEBUFFER_SRGB);
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_SRGBWRITEENABLE));
    }
    checkGLcall("setting up state for clear");

    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_ALPHABLENDENABLE));
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_SCISSORTESTENABLE));
    context_invalidate_state(context, STATE_SCISSORRECT);

    return TRUE;
}

static DWORD find_draw_buffers_mask(const struct wined3d_context *context, const struct wined3d_state *state)
{
    struct wined3d_rendertarget_view **rts = state->fb->render_targets;
    struct wined3d_shader *ps = state->shader[WINED3D_SHADER_TYPE_PIXEL];
    DWORD rt_mask, rt_mask_bits;
    unsigned int i;

    if (wined3d_settings.offscreen_rendering_mode != ORM_FBO)
        return context_generate_rt_mask_no_fbo(context, wined3d_rendertarget_view_get_surface(rts[0])->container);
    else if (!context->render_offscreen)
        return context_generate_rt_mask_from_resource(rts[0]->resource);

    /* If we attach more buffers than supported in dual blend mode, the NVIDIA
     * driver generates the following error:
     *      GL_INVALID_OPERATION error generated. State(s) are invalid: blend.
     * DX11 does not treat this configuration as invalid, so disable the unused ones.
     */
    rt_mask = ps ? ps->reg_maps.rt_mask : 1;
    if (wined3d_dualblend_enabled(state, context->gl_info))
        rt_mask &= context->d3d_info->valid_dual_rt_mask;
    else
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
    DWORD rt_mask = find_draw_buffers_mask(context, state);
    const struct wined3d_fb_state *fb = state->fb;
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

            memset(context->blit_targets, 0, context->gl_info->limits.buffers * sizeof (*context->blit_targets));
            for (i = 0; i < context->gl_info->limits.buffers; ++i)
            {
                if (fb->render_targets[i])
                {
                    context->blit_targets[i].gl_view = fb->render_targets[i]->gl_view;
                    context->blit_targets[i].resource = fb->render_targets[i]->resource;
                    context->blit_targets[i].sub_resource_idx = fb->render_targets[i]->sub_resource_idx;
                    context->blit_targets[i].layer_count = fb->render_targets[i]->layer_count;
                }
            }
            context_apply_fbo_state(context, GL_FRAMEBUFFER, context->blit_targets,
                    wined3d_rendertarget_view_get_surface(fb->depth_stencil),
                    fb->render_targets[0] ? fb->render_targets[0]->resource->draw_binding : 0,
                    fb->depth_stencil ? fb->depth_stencil->resource->draw_binding : 0);
        }
    }

    cur_mask = context->current_fbo ? &context->current_fbo->rt_mask : &context->draw_buffers_mask;
    if (rt_mask != *cur_mask)
    {
        context_apply_draw_buffers(context, rt_mask);
        *cur_mask = rt_mask;
    }
    context->constant_update_mask |= WINED3D_SHADER_CONST_PS_Y_CORR;
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
    unsigned int i, tex;
    WORD ffu_map;

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
    const struct wined3d_shader_resource_info *resource_info =
            state->shader[WINED3D_SHADER_TYPE_PIXEL]->reg_maps.resource_info;
    unsigned int i;

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
    int start = min(MAX_COMBINED_SAMPLERS, gl_info->limits.graphics_samplers) - 1;
    int i;

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
    const struct wined3d_gl_info *gl_info = context->gl_info;
    BOOL vs = use_vs(state);
    BOOL ps = use_ps(state);

    if (!ps)
        context_update_fixed_function_usage_map(context, state);

    /* Try to go for a 1:1 mapping of the samplers when possible. Pixel shaders
     * need a 1:1 map at the moment.
     * When the mapping of a stage is changed, sampler and ALL texture stage
     * states have to be reset. */

    if (gl_info->limits.graphics_samplers >= MAX_COMBINED_SAMPLERS)
        return;

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
    DWORD rt_mask, *cur_mask;

    if (isStateDirty(context, STATE_FRAMEBUFFER)) return;

    cur_mask = context->current_fbo ? &context->current_fbo->rt_mask : &context->draw_buffers_mask;
    rt_mask = find_draw_buffers_mask(context, state);
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
        WARN("Unsupported input stream [usage=%s, usage_idx=%u].\n", debug_d3ddeclusage(usage), usage_idx);
        *regnum = ~0u;
        return FALSE;
    }

    return TRUE;
}

/* Context activation is done by the caller. */
void wined3d_stream_info_from_declaration(struct wined3d_stream_info *stream_info,
        const struct wined3d_state *state, const struct wined3d_gl_info *gl_info,
        const struct wined3d_d3d_info *d3d_info)
{
    /* We need to deal with frequency data! */
    struct wined3d_vertex_declaration *declaration = state->vertex_declaration;
    BOOL generic_attributes = d3d_info->ffp_generic_attributes;
    BOOL use_vshader = use_vs(state);
    unsigned int i;

    stream_info->use_map = 0;
    stream_info->swizzle_map = 0;
    stream_info->position_transformed = 0;

    if (!declaration)
        return;

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

            if (!gl_info->supported[ARB_VERTEX_ARRAY_BGRA]
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
    struct wined3d_stream_info *stream_info = &context->stream_info;
    const struct wined3d_d3d_info *d3d_info = context->d3d_info;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    DWORD prev_all_vbo = stream_info->all_vbo;
    unsigned int i;
    WORD map;

    wined3d_stream_info_from_declaration(stream_info, state, gl_info, d3d_info);

    stream_info->all_vbo = 1;
    context->buffer_fence_count = 0;
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
         * draw_primitive_immediate_mode() is needed, including a vertex buffer
         * path. */
        if (state->load_base_vertex_index < 0)
        {
            WARN_(d3d_perf)("load_base_vertex_index is < 0 (%d), not using VBOs.\n",
                    state->load_base_vertex_index);
            element->data.buffer_object = 0;
            element->data.addr += (ULONG_PTR)wined3d_buffer_load_sysmem(buffer, context);
            if ((UINT_PTR)element->data.addr < -state->load_base_vertex_index * element->stride)
                FIXME("System memory vertex data load offset is negative!\n");
        }
        else
        {
            wined3d_buffer_load(buffer, context, state);
            wined3d_buffer_get_memory(buffer, &data, buffer->locations);
            element->data.buffer_object = data.buffer_object;
            element->data.addr += (ULONG_PTR)data.addr;
        }

        if (!element->data.buffer_object)
            stream_info->all_vbo = 0;

        if (buffer->fence)
            context->buffer_fences[context->buffer_fence_count++] = buffer->fence;

        TRACE("Load array %u {%#x:%p}.\n", i, element->data.buffer_object, element->data.addr);
    }

    if (prev_all_vbo != stream_info->all_vbo)
        context_invalidate_state(context, STATE_INDEXBUFFER);

    context->use_immediate_mode_draw = FALSE;

    if (stream_info->all_vbo)
        return;

    if (use_vs(state))
    {
        if (state->vertex_declaration->half_float_conv_needed)
        {
            TRACE("Using immediate mode draw with vertex shaders for FLOAT16 conversion.\n");
            context->use_immediate_mode_draw = TRUE;
        }
    }
    else
    {
        WORD slow_mask = -!d3d_info->ffp_generic_attributes & (1u << WINED3D_FFP_PSIZE);
        slow_mask |= -(!gl_info->supported[ARB_VERTEX_ARRAY_BGRA] && !d3d_info->ffp_generic_attributes)
                & ((1u << WINED3D_FFP_DIFFUSE) | (1u << WINED3D_FFP_SPECULAR) | (1u << WINED3D_FFP_BLENDWEIGHT));

        if ((stream_info->position_transformed && !d3d_info->xyzrhw)
                || (stream_info->use_map & slow_mask))
            context->use_immediate_mode_draw = TRUE;
    }
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

static void context_load_shader_resources(struct wined3d_context *context, const struct wined3d_state *state,
        unsigned int shader_mask)
{
    struct wined3d_shader_sampler_map_entry *entry;
    struct wined3d_shader_resource_view *view;
    struct wined3d_shader *shader;
    unsigned int i, j;

    for (i = 0; i < WINED3D_SHADER_TYPE_COUNT; ++i)
    {
        if (!(shader_mask & (1u << i)))
            continue;

        if (!(shader = state->shader[i]))
            continue;

        for (j = 0; j < WINED3D_MAX_CBS; ++j)
        {
            if (state->cb[i][j])
                wined3d_buffer_load(state->cb[i][j], context, state);
        }

        for (j = 0; j < shader->reg_maps.sampler_map.count; ++j)
        {
            entry = &shader->reg_maps.sampler_map.entries[j];

            if (!(view = state->shader_resource_view[i][entry->resource_idx]))
                continue;

            if (view->resource->type == WINED3D_RTYPE_BUFFER)
                wined3d_buffer_load(buffer_from_resource(view->resource), context, state);
            else
                wined3d_texture_load(texture_from_resource(view->resource), context, FALSE);
        }
    }
}

static void context_bind_shader_resources(struct wined3d_context *context,
        const struct wined3d_state *state, enum wined3d_shader_type shader_type)
{
    unsigned int bind_idx, shader_sampler_count, base, count, i;
    const struct wined3d_device *device = context->device;
    struct wined3d_shader_sampler_map_entry *entry;
    struct wined3d_shader_resource_view *view;
    const struct wined3d_shader *shader;
    struct wined3d_sampler *sampler;
    const DWORD *tex_unit_map;

    if (!(shader = state->shader[shader_type]))
        return;

    tex_unit_map = context_get_tex_unit_mapping(context,
            &shader->reg_maps.shader_version, &base, &count);

    shader_sampler_count = shader->reg_maps.sampler_map.count;
    if (shader_sampler_count > count)
        FIXME("Shader %p needs %u samplers, but only %u are supported.\n",
                shader, shader_sampler_count, count);
    count = min(shader_sampler_count, count);

    for (i = 0; i < count; ++i)
    {
        entry = &shader->reg_maps.sampler_map.entries[i];
        bind_idx = base + entry->bind_idx;
        if (tex_unit_map)
            bind_idx = tex_unit_map[bind_idx];

        if (!(view = state->shader_resource_view[shader_type][entry->resource_idx]))
        {
            WARN("No resource view bound at index %u, %u.\n", shader_type, entry->resource_idx);
            continue;
        }

        if (entry->sampler_idx == WINED3D_SAMPLER_DEFAULT)
            sampler = device->default_sampler;
        else if (!(sampler = state->sampler[shader_type][entry->sampler_idx]))
            sampler = device->null_sampler;
        wined3d_shader_resource_view_bind(view, bind_idx, sampler, context);
    }
}

static void context_load_unordered_access_resources(struct wined3d_context *context,
        const struct wined3d_shader *shader, struct wined3d_unordered_access_view * const *views)
{
    struct wined3d_unordered_access_view *view;
    struct wined3d_texture *texture;
    struct wined3d_buffer *buffer;
    unsigned int i;

    context->uses_uavs = 0;

    if (!shader)
        return;

    for (i = 0; i < MAX_UNORDERED_ACCESS_VIEWS; ++i)
    {
        if (!(view = views[i]))
            continue;

        if (view->resource->type == WINED3D_RTYPE_BUFFER)
        {
            buffer = buffer_from_resource(view->resource);
            wined3d_buffer_load_location(buffer, context, WINED3D_LOCATION_BUFFER);
            wined3d_unordered_access_view_invalidate_location(view, ~WINED3D_LOCATION_BUFFER);
        }
        else
        {
            texture = texture_from_resource(view->resource);
            wined3d_texture_load(texture, context, FALSE);
            wined3d_unordered_access_view_invalidate_location(view, ~WINED3D_LOCATION_TEXTURE_RGB);
        }

        context->uses_uavs = 1;
    }
}

static void context_bind_unordered_access_views(struct wined3d_context *context,
        const struct wined3d_shader *shader, struct wined3d_unordered_access_view * const *views)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    struct wined3d_unordered_access_view *view;
    GLuint texture_name;
    unsigned int i;
    GLint level;

    if (!shader)
        return;

    for (i = 0; i < MAX_UNORDERED_ACCESS_VIEWS; ++i)
    {
        if (!(view = views[i]))
        {
            if (shader->reg_maps.uav_resource_info[i].type)
                WARN("No unordered access view bound at index %u.\n", i);
            GL_EXTCALL(glBindImageTexture(i, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R8));
            continue;
        }

        if (view->gl_view.name)
        {
            texture_name = view->gl_view.name;
            level = 0;
        }
        else if (view->resource->type != WINED3D_RTYPE_BUFFER)
        {
            struct wined3d_texture *texture = texture_from_resource(view->resource);
            texture_name = wined3d_texture_get_texture_name(texture, context, FALSE);
            level = view->desc.u.texture.level_idx;
        }
        else
        {
            FIXME("Unsupported buffer unordered access view.\n");
            GL_EXTCALL(glBindImageTexture(i, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R8));
            continue;
        }

        GL_EXTCALL(glBindImageTexture(i, texture_name, level, GL_TRUE, 0, GL_READ_WRITE,
                view->format->glInternal));

        if (view->counter_bo)
            GL_EXTCALL(glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, i, view->counter_bo));
    }
    checkGLcall("Bind unordered access views");
}

static void context_load_stream_output_buffers(struct wined3d_context *context,
        const struct wined3d_state *state)
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(state->stream_output); ++i)
    {
        struct wined3d_buffer *buffer;
        if (!(buffer = state->stream_output[i].buffer))
            continue;

        wined3d_buffer_load(buffer, context, state);
        wined3d_buffer_invalidate_location(buffer, ~WINED3D_LOCATION_BUFFER);
    }
}

/* Context activation is done by the caller. */
BOOL context_apply_draw_state(struct wined3d_context *context,
        const struct wined3d_device *device, const struct wined3d_state *state)
{
    const struct StateEntry *state_table = context->state_table;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    const struct wined3d_fb_state *fb = state->fb;
    unsigned int i;
    WORD map;

    if (!context_validate_rt_config(gl_info->limits.buffers, fb->render_targets, fb->depth_stencil))
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
    context_load_shader_resources(context, state, ~(1u << WINED3D_SHADER_TYPE_COMPUTE));
    context_load_unordered_access_resources(context, state->shader[WINED3D_SHADER_TYPE_PIXEL],
            state->unordered_access_view[WINED3D_PIPELINE_GRAPHICS]);
    context_load_stream_output_buffers(context, state);
    /* TODO: Right now the dependency on the vertex shader is necessary
     * since wined3d_stream_info_from_declaration() depends on the reg_maps of
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
                wined3d_buffer_load(state->streams[context->stream_info.elements[i].stream_idx].buffer,
                        context, state);
        }
        /* Loading the buffers above may have invalidated the stream info. */
        if (isStateDirty(context, STATE_STREAMSRC))
            context_update_stream_info(context, state);
    }
    if (state->index_buffer)
    {
        if (context->stream_info.all_vbo)
            wined3d_buffer_load(state->index_buffer, context, state);
        else
            wined3d_buffer_load_sysmem(state->index_buffer, context);
    }

    for (i = 0; i < context->numDirtyEntries; ++i)
    {
        DWORD rep = context->dirtyArray[i];
        DWORD idx = rep / (sizeof(*context->isStateDirty) * CHAR_BIT);
        BYTE shift = rep & ((sizeof(*context->isStateDirty) * CHAR_BIT) - 1);
        context->isStateDirty[idx] &= ~(1u << shift);
        state_table[rep].apply(context, state, rep);
    }

    if (context->shader_update_mask & ~(1u << WINED3D_SHADER_TYPE_COMPUTE))
    {
        device->shader_backend->shader_select(device->shader_priv, context, state);
        context->shader_update_mask &= 1u << WINED3D_SHADER_TYPE_COMPUTE;
    }

    if (context->constant_update_mask)
    {
        device->shader_backend->shader_load_constants(device->shader_priv, context, state);
        context->constant_update_mask = 0;
    }

    if (context->update_shader_resource_bindings)
    {
        for (i = 0; i < WINED3D_SHADER_TYPE_GRAPHICS_COUNT; ++i)
            context_bind_shader_resources(context, state, i);
        context->update_shader_resource_bindings = 0;
        if (gl_info->limits.combined_samplers == gl_info->limits.graphics_samplers)
            context->update_compute_shader_resource_bindings = 1;
    }

    if (context->update_unordered_access_view_bindings)
    {
        context_bind_unordered_access_views(context,
                state->shader[WINED3D_SHADER_TYPE_PIXEL],
                state->unordered_access_view[WINED3D_PIPELINE_GRAPHICS]);
        context->update_unordered_access_view_bindings = 0;
        context->update_compute_unordered_access_view_bindings = 1;
    }

    if (wined3d_settings.offscreen_rendering_mode == ORM_FBO)
    {
        context_check_fbo_status(context, GL_FRAMEBUFFER);
    }

    context->numDirtyEntries = 0; /* This makes the whole list clean */
    context->last_was_blit = FALSE;

    return TRUE;
}

void context_apply_compute_state(struct wined3d_context *context,
        const struct wined3d_device *device, const struct wined3d_state *state)
{
    const struct StateEntry *state_table = context->state_table;
    const struct wined3d_gl_info *gl_info = context->gl_info;
    unsigned int state_id, i, j;

    context_load_shader_resources(context, state, 1u << WINED3D_SHADER_TYPE_COMPUTE);
    context_load_unordered_access_resources(context, state->shader[WINED3D_SHADER_TYPE_COMPUTE],
            state->unordered_access_view[WINED3D_PIPELINE_COMPUTE]);

    for (i = 0, state_id = STATE_COMPUTE_OFFSET; i < ARRAY_SIZE(context->dirty_compute_states); ++i)
    {
        for (j = 0; j < sizeof(*context->dirty_compute_states) * CHAR_BIT; ++j, ++state_id)
        {
            if (context->dirty_compute_states[i] & (1u << j))
                state_table[state_id].apply(context, state, state_id);
        }
    }
    memset(context->dirty_compute_states, 0, sizeof(*context->dirty_compute_states));

    if (context->shader_update_mask & (1u << WINED3D_SHADER_TYPE_COMPUTE))
    {
        device->shader_backend->shader_select_compute(device->shader_priv, context, state);
        context->shader_update_mask &= ~(1u << WINED3D_SHADER_TYPE_COMPUTE);
    }

    if (context->update_compute_shader_resource_bindings)
    {
        context_bind_shader_resources(context, state, WINED3D_SHADER_TYPE_COMPUTE);
        context->update_compute_shader_resource_bindings = 0;
        if (gl_info->limits.combined_samplers == gl_info->limits.graphics_samplers)
            context->update_shader_resource_bindings = 1;
    }

    if (context->update_compute_unordered_access_view_bindings)
    {
        context_bind_unordered_access_views(context,
                state->shader[WINED3D_SHADER_TYPE_COMPUTE],
                state->unordered_access_view[WINED3D_PIPELINE_COMPUTE]);
        context->update_compute_unordered_access_view_bindings = 0;
        context->update_unordered_access_view_bindings = 1;
    }

    context->last_was_blit = FALSE;
}

void context_end_transform_feedback(struct wined3d_context *context)
{
    const struct wined3d_gl_info *gl_info = context->gl_info;
    if (context->transform_feedback_active)
    {
        GL_EXTCALL(glEndTransformFeedback());
        checkGLcall("glEndTransformFeedback");
        context->transform_feedback_active = 0;
        context->transform_feedback_paused = 0;
    }
}

static void context_setup_target(struct wined3d_context *context,
        struct wined3d_texture *texture, unsigned int sub_resource_idx)
{
    BOOL old_render_offscreen = context->render_offscreen, render_offscreen;

    render_offscreen = wined3d_resource_is_offscreen(&texture->resource);
    if (context->current_rt.texture == texture
            && context->current_rt.sub_resource_idx == sub_resource_idx
            && render_offscreen == old_render_offscreen)
        return;

    /* To compensate the lack of format switching with some offscreen rendering methods and on onscreen buffers
     * the alpha blend state changes with different render target formats. */
    if (!context->current_rt.texture)
    {
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_ALPHABLENDENABLE));
    }
    else
    {
        const struct wined3d_format *old = context->current_rt.texture->resource.format;
        const struct wined3d_format *new = texture->resource.format;

        if (old->id != new->id)
        {
            /* Disable blending when the alpha mask has changed and when a format doesn't support blending. */
            if ((old->alpha_size && !new->alpha_size) || (!old->alpha_size && new->alpha_size)
                    || !(texture->resource.format_flags & WINED3DFMT_FLAG_POSTPIXELSHADER_BLENDING))
                context_invalidate_state(context, STATE_RENDER(WINED3D_RS_ALPHABLENDENABLE));

            /* Update sRGB writing when switching between formats that do/do not support sRGB writing */
            if ((context->current_rt.texture->resource.format_flags & WINED3DFMT_FLAG_SRGB_WRITE)
                    != (texture->resource.format_flags & WINED3DFMT_FLAG_SRGB_WRITE))
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
                && old_render_offscreen && (context->current_rt.texture != texture
                || context->current_rt.sub_resource_idx != sub_resource_idx))
        {
            unsigned int prev_sub_resource_idx = context->current_rt.sub_resource_idx;
            struct wined3d_texture *prev_texture = context->current_rt.texture;

            /* Read the back buffer of the old drawable into the destination texture. */
            if (prev_texture->texture_srgb.name)
                wined3d_texture_load(prev_texture, context, TRUE);
            wined3d_texture_load(prev_texture, context, FALSE);
            wined3d_texture_invalidate_location(prev_texture, prev_sub_resource_idx, WINED3D_LOCATION_DRAWABLE);
        }
    }

    context->current_rt.texture = texture;
    context->current_rt.sub_resource_idx = sub_resource_idx;
    context_set_render_offscreen(context, render_offscreen);
}

struct wined3d_context *context_acquire(const struct wined3d_device *device,
        struct wined3d_texture *texture, unsigned int sub_resource_idx)
{
    struct wined3d_context *current_context = context_get_current();
    struct wined3d_context *context;
    BOOL swapchain_texture;

    TRACE("device %p, texture %p, sub_resource_idx %u.\n", device, texture, sub_resource_idx);

    wined3d_from_cs(device->cs);

    if (current_context && current_context->destroyed)
        current_context = NULL;

    swapchain_texture = texture && texture->swapchain;
    if (!texture)
    {
        if (current_context
                && current_context->current_rt.texture
                && current_context->device == device)
        {
            texture = current_context->current_rt.texture;
            sub_resource_idx = current_context->current_rt.sub_resource_idx;
        }
        else
        {
            struct wined3d_swapchain *swapchain = device->swapchains[0];

            if (swapchain->back_buffers)
                texture = swapchain->back_buffers[0];
            else
                texture = swapchain->front_buffer;
            sub_resource_idx = 0;
        }
    }

    if (current_context && current_context->current_rt.texture == texture)
    {
        context = current_context;
    }
    else if (swapchain_texture)
    {
        TRACE("Rendering onscreen.\n");

        context = swapchain_get_context(texture->swapchain);
    }
    else
    {
        TRACE("Rendering offscreen.\n");

        /* Stay with the current context if possible. Otherwise use the
         * context for the primary swapchain. */
        if (current_context && current_context->device == device)
            context = current_context;
        else
            context = swapchain_get_context(device->swapchains[0]);
    }

    context_enter(context);
    context_update_window(context);
    context_setup_target(context, texture, sub_resource_idx);
    if (!context->valid)
        return context;

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

struct wined3d_context *context_reacquire(const struct wined3d_device *device,
        struct wined3d_context *context)
{
    struct wined3d_context *current_context;

    if (!context || context->tid != GetCurrentThreadId())
        return NULL;

    current_context = context_acquire(device, context->current_rt.texture,
            context->current_rt.sub_resource_idx);
    if (current_context != context)
        ERR("Acquired context %p instead of %p.\n", current_context, context);
    return current_context;
}
