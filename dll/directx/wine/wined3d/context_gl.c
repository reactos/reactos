/*
 * Context and render target management in wined3d
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2002-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2006, 2008 Henri Verbeet
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
#include "wined3d_gl.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
WINE_DECLARE_DEBUG_CHANNEL(d3d_perf);
WINE_DECLARE_DEBUG_CHANNEL(d3d_sync);

#define WINED3D_MAX_FBO_ENTRIES 64
#define WINED3D_ALL_LAYERS (~0u)

static DWORD wined3d_context_tls_idx;

/* Note that except for WINED3DPT_POINTLIST and WINED3DPT_LINELIST these
 * actually have the same values in GL and D3D. */
static GLenum gl_primitive_type_from_d3d(enum wined3d_primitive_type primitive_type)
{
    switch (primitive_type)
    {
        case WINED3D_PT_POINTLIST:
            return GL_POINTS;

        case WINED3D_PT_LINELIST:
            return GL_LINES;

        case WINED3D_PT_LINESTRIP:
            return GL_LINE_STRIP;

        case WINED3D_PT_TRIANGLELIST:
            return GL_TRIANGLES;

        case WINED3D_PT_TRIANGLESTRIP:
            return GL_TRIANGLE_STRIP;

        case WINED3D_PT_TRIANGLEFAN:
            return GL_TRIANGLE_FAN;

        case WINED3D_PT_LINELIST_ADJ:
            return GL_LINES_ADJACENCY_ARB;

        case WINED3D_PT_LINESTRIP_ADJ:
            return GL_LINE_STRIP_ADJACENCY_ARB;

        case WINED3D_PT_TRIANGLELIST_ADJ:
            return GL_TRIANGLES_ADJACENCY_ARB;

        case WINED3D_PT_TRIANGLESTRIP_ADJ:
            return GL_TRIANGLE_STRIP_ADJACENCY_ARB;

        case WINED3D_PT_PATCH:
            return GL_PATCHES;

        default:
            FIXME("Unhandled primitive type %s.\n", debug_d3dprimitivetype(primitive_type));
        case WINED3D_PT_UNDEFINED:
            return ~0u;
    }
}

/* FBO helper functions */

/* Context activation is done by the caller. */
static void wined3d_context_gl_bind_fbo(struct wined3d_context_gl *context_gl, GLenum target, GLuint fbo)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    TRACE("context_gl %p, target %#x, fbo %u.\n", context_gl, target, fbo);

    switch (target)
    {
        case GL_READ_FRAMEBUFFER:
            if (context_gl->fbo_read_binding == fbo)
                return;
            context_gl->fbo_read_binding = fbo;
            break;

        case GL_DRAW_FRAMEBUFFER:
            if (context_gl->fbo_draw_binding == fbo)
                return;
            context_gl->fbo_draw_binding = fbo;
            break;

        case GL_FRAMEBUFFER:
            if (context_gl->fbo_read_binding == fbo
                    && context_gl->fbo_draw_binding == fbo)
                return;
            context_gl->fbo_read_binding = fbo;
            context_gl->fbo_draw_binding = fbo;
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
static void wined3d_context_gl_destroy_fbo(struct wined3d_context_gl *context_gl, GLuint fbo)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    wined3d_context_gl_bind_fbo(context_gl, GL_FRAMEBUFFER, fbo);
    context_clean_fbo_attachments(gl_info, GL_FRAMEBUFFER);
    wined3d_context_gl_bind_fbo(context_gl, GL_FRAMEBUFFER, 0);

    gl_info->fbo_ops.glDeleteFramebuffers(1, &fbo);
    checkGLcall("glDeleteFramebuffers()");
}

static void context_attach_depth_stencil_rb(const struct wined3d_gl_info *gl_info,
        GLenum fbo_target, uint32_t flags, GLuint rb)
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

static void wined3d_context_gl_attach_gl_texture_fbo(struct wined3d_context_gl *context_gl,
        GLenum fbo_target, GLenum attachment, const struct wined3d_fbo_resource *resource)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

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
    else if (resource->target == GL_TEXTURE_1D_ARRAY || resource->target == GL_TEXTURE_2D_ARRAY
            || resource->target == GL_TEXTURE_3D)
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
    }
    else
    {
        gl_info->fbo_ops.glFramebufferTexture2D(fbo_target, attachment,
                resource->target, resource->object, resource->level);
    }
    checkGLcall("attach texture to fbo");
}

/* Context activation is done by the caller. */
static void wined3d_context_gl_attach_depth_stencil_fbo(struct wined3d_context_gl *context_gl,
        GLenum fbo_target, const struct wined3d_fbo_resource *resource, BOOL rb_namespace,
        uint32_t flags)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

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
                wined3d_context_gl_attach_gl_texture_fbo(context_gl, fbo_target, GL_DEPTH_ATTACHMENT, resource);

            if (flags & WINED3D_FBO_ENTRY_FLAG_STENCIL)
                wined3d_context_gl_attach_gl_texture_fbo(context_gl, fbo_target, GL_STENCIL_ATTACHMENT, resource);
        }

        if (!(flags & WINED3D_FBO_ENTRY_FLAG_DEPTH))
            wined3d_context_gl_attach_gl_texture_fbo(context_gl, fbo_target, GL_DEPTH_ATTACHMENT, NULL);

        if (!(flags & WINED3D_FBO_ENTRY_FLAG_STENCIL))
            wined3d_context_gl_attach_gl_texture_fbo(context_gl, fbo_target, GL_STENCIL_ATTACHMENT, NULL);
    }
    else
    {
        TRACE("Attach depth stencil 0.\n");

        wined3d_context_gl_attach_gl_texture_fbo(context_gl, fbo_target, GL_DEPTH_ATTACHMENT, NULL);
        wined3d_context_gl_attach_gl_texture_fbo(context_gl, fbo_target, GL_STENCIL_ATTACHMENT, NULL);
    }
}

/* Context activation is done by the caller. */
static void wined3d_context_gl_attach_surface_fbo(struct wined3d_context_gl *context_gl,
        GLenum fbo_target, unsigned int idx, const struct wined3d_fbo_resource *resource, BOOL rb_namespace)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

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
            wined3d_context_gl_attach_gl_texture_fbo(context_gl, fbo_target, GL_COLOR_ATTACHMENT0 + idx, resource);
        }
    }
    else
    {
        wined3d_context_gl_attach_gl_texture_fbo(context_gl, fbo_target, GL_COLOR_ATTACHMENT0 + idx, NULL);
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
        {GL_TEXTURE_1D,                   GL_TEXTURE_BINDING_1D,                   "1d",          WINED3D_GL_EXT_NONE},
        {GL_TEXTURE_1D_ARRAY,             GL_TEXTURE_BINDING_1D_ARRAY,             "1d-array",    EXT_TEXTURE_ARRAY},
        {GL_TEXTURE_2D,                   GL_TEXTURE_BINDING_2D,                   "2d",          WINED3D_GL_EXT_NONE},
        {GL_TEXTURE_2D_ARRAY,             GL_TEXTURE_BINDING_2D_ARRAY,             "2d-array" ,   EXT_TEXTURE_ARRAY},
        {GL_TEXTURE_CUBE_MAP,             GL_TEXTURE_BINDING_CUBE_MAP,             "cube",        ARB_TEXTURE_CUBE_MAP},
        {GL_TEXTURE_2D_MULTISAMPLE,       GL_TEXTURE_BINDING_2D_MULTISAMPLE,       "2d-ms",       ARB_TEXTURE_MULTISAMPLE},
        {GL_TEXTURE_2D_MULTISAMPLE_ARRAY, GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY, "2d-array-ms", ARB_TEXTURE_MULTISAMPLE},
    };

    GLint type, name, samples, width, height, old_texture, level, face, fmt, tex_target;
    const char *tex_type_str = NULL;
    unsigned int i;

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
        gl_info->fbo_ops.glGetFramebufferAttachmentParameteriv(target, attachment,
                GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL, &level);
        gl_info->fbo_ops.glGetFramebufferAttachmentParameteriv(target, attachment,
                GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE, &face);

        if (gl_info->gl_ops.ext.p_glGetTextureParameteriv)
        {
            GL_EXTCALL(glGetTextureParameteriv(name, GL_TEXTURE_TARGET, &tex_target));

            for (i = 0; i < ARRAY_SIZE(texture_type); ++i)
            {
                if (texture_type[i].target == tex_target)
                {
                    tex_type_str = texture_type[i].str;
                    break;
                }
            }
            if (i == ARRAY_SIZE(texture_type))
                tex_type_str = wine_dbg_sprintf("%#x", tex_target);
        }
        else if (face)
        {
            gl_info->gl_ops.gl.p_glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &old_texture);
            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_CUBE_MAP, name);

            tex_target = GL_TEXTURE_CUBE_MAP;
            tex_type_str = "cube";
        }
        else
        {
            for (i = 0; i < ARRAY_SIZE(texture_type); ++i)
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
        }

        if (gl_info->gl_ops.ext.p_glGetTextureParameteriv)
        {
            GL_EXTCALL(glGetTextureLevelParameteriv(name, level, GL_TEXTURE_INTERNAL_FORMAT, &fmt));
            GL_EXTCALL(glGetTextureLevelParameteriv(name, level, GL_TEXTURE_WIDTH, &width));
            GL_EXTCALL(glGetTextureLevelParameteriv(name, level, GL_TEXTURE_HEIGHT, &height));
            GL_EXTCALL(glGetTextureLevelParameteriv(name, level, GL_TEXTURE_SAMPLES, &samples));
        }
        else
        {
            gl_info->gl_ops.gl.p_glGetTexLevelParameteriv(tex_target, level, GL_TEXTURE_INTERNAL_FORMAT, &fmt);
            gl_info->gl_ops.gl.p_glGetTexLevelParameteriv(tex_target, level, GL_TEXTURE_WIDTH, &width);
            gl_info->gl_ops.gl.p_glGetTexLevelParameteriv(tex_target, level, GL_TEXTURE_HEIGHT, &height);
            if (gl_info->supported[ARB_TEXTURE_MULTISAMPLE])
                gl_info->gl_ops.gl.p_glGetTexLevelParameteriv(tex_target, level, GL_TEXTURE_SAMPLES, &samples);
            else
                samples = 1;

            gl_info->gl_ops.gl.p_glBindTexture(tex_target, old_texture);
        }

        FIXME("    %s: %s texture %d, %dx%d, %d samples, format %#x.\n",
                debug_fboattachment(attachment), tex_type_str, name, width, height, samples, fmt);
    }
    else if (type == GL_NONE)
    {
        FIXME("    %s: NONE.\n", debug_fboattachment(attachment));
    }
    else
    {
        ERR("    %s: Unknown attachment %#x.\n", debug_fboattachment(attachment), type);
    }

    checkGLcall("dump FBO attachment");
}

/* Context activation is done by the caller. */
void wined3d_context_gl_check_fbo_status(const struct wined3d_context_gl *context_gl, GLenum target)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    GLenum status;

    if (!FIXME_ON(d3d))
        return;

    status = gl_info->fbo_ops.glCheckFramebufferStatus(target);
    if (status == GL_FRAMEBUFFER_COMPLETE)
    {
        TRACE("FBO complete.\n");
    }
    else
    {
        unsigned int i;

        FIXME("FBO status %s (%#x).\n", debug_fbostatus(status), status);

        if (!context_gl->current_fbo)
        {
            ERR("FBO 0 is incomplete, driver bug?\n");
            return;
        }

        context_dump_fbo_attachment(gl_info, target, GL_DEPTH_ATTACHMENT);
        context_dump_fbo_attachment(gl_info, target, GL_STENCIL_ATTACHMENT);

        for (i = 0; i < gl_info->limits.buffers; ++i)
            context_dump_fbo_attachment(gl_info, target, GL_COLOR_ATTACHMENT0 + i);
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

/* Context activation is done by the caller. */
static void wined3d_context_gl_set_draw_buffer(struct wined3d_context_gl *context_gl, GLenum buffer)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct fbo_entry *current_fbo = context_gl->current_fbo;
    uint32_t new_mask = context_generate_rt_mask(buffer);
    uint32_t *current_mask;

    current_mask = current_fbo ? &current_fbo->rt_mask : &context_gl->draw_buffers_mask;
    if (new_mask == *current_mask)
        return;

    gl_info->gl_ops.gl.p_glDrawBuffer(buffer);
    checkGLcall("glDrawBuffer()");

    *current_mask = new_mask;
}

static inline void wined3d_context_gl_set_fbo_key_for_render_target(const struct wined3d_context_gl *context_gl,
        struct wined3d_fbo_entry_key *key, unsigned int idx, const struct wined3d_rendertarget_info *render_target,
        DWORD location)
{
    unsigned int sub_resource_idx = render_target->sub_resource_idx;
    struct wined3d_resource *resource = render_target->resource;
    struct wined3d_texture_gl *texture_gl;

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
        key->objects[idx].layer = (render_target->layer_count == 1 ? 0 : WINED3D_ALL_LAYERS);
        return;
    }

    texture_gl = wined3d_texture_gl(wined3d_texture_from_resource(resource));
    if (texture_gl->current_renderbuffer)
    {
        key->objects[idx].object = texture_gl->current_renderbuffer->id;
        key->objects[idx].target = 0;
        key->objects[idx].level = key->objects[idx].layer = 0;
        key->rb_namespace |= 1 << idx;
        return;
    }

    key->objects[idx].target = wined3d_texture_gl_get_sub_resource_target(texture_gl, sub_resource_idx);
    key->objects[idx].level = sub_resource_idx % texture_gl->t.level_count;
    key->objects[idx].layer = sub_resource_idx / texture_gl->t.level_count;

    if (render_target->layer_count != 1)
        key->objects[idx].layer = WINED3D_ALL_LAYERS;

    switch (location)
    {
        case WINED3D_LOCATION_TEXTURE_RGB:
            key->objects[idx].object = wined3d_texture_gl_get_texture_name(texture_gl, &context_gl->c, FALSE);
            break;

        case WINED3D_LOCATION_TEXTURE_SRGB:
            key->objects[idx].object = wined3d_texture_gl_get_texture_name(texture_gl, &context_gl->c, TRUE);
            break;

        case WINED3D_LOCATION_RB_MULTISAMPLE:
            key->objects[idx].object = texture_gl->rb_multisample;
            key->objects[idx].target = 0;
            key->objects[idx].level = key->objects[idx].layer = 0;
            key->rb_namespace |= 1 << idx;
            break;

        case WINED3D_LOCATION_RB_RESOLVED:
            key->objects[idx].object = texture_gl->rb_resolved;
            key->objects[idx].target = 0;
            key->objects[idx].level = key->objects[idx].layer = 0;
            key->rb_namespace |= 1 << idx;
            break;
    }
}

static void wined3d_context_gl_generate_fbo_key(const struct wined3d_context_gl *context_gl,
        struct wined3d_fbo_entry_key *key, const struct wined3d_rendertarget_info *render_targets,
        const struct wined3d_rendertarget_info *depth_stencil, DWORD color_location, DWORD ds_location)
{
    unsigned int buffers = context_gl->gl_info->limits.buffers;
    unsigned int i;

    key->rb_namespace = 0;
    wined3d_context_gl_set_fbo_key_for_render_target(context_gl, key, 0, depth_stencil, ds_location);

    for (i = 0; i < buffers; ++i)
        wined3d_context_gl_set_fbo_key_for_render_target(context_gl, key, i + 1, &render_targets[i], color_location);

    memset(&key->objects[buffers + 1], 0, (ARRAY_SIZE(key->objects) - buffers - 1) * sizeof(*key->objects));
}

static struct fbo_entry *wined3d_context_gl_create_fbo_entry(const struct wined3d_context_gl *context_gl,
        const struct wined3d_rendertarget_info *render_targets, const struct wined3d_rendertarget_info *depth_stencil,
        DWORD color_location, DWORD ds_location)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct fbo_entry *entry;

    entry = malloc(sizeof(*entry));
    wined3d_context_gl_generate_fbo_key(context_gl, &entry->key,
            render_targets, depth_stencil, color_location, ds_location);
    entry->flags = 0;
    if (depth_stencil->resource)
    {
        if (depth_stencil->resource->format->depth_size)
            entry->flags |= WINED3D_FBO_ENTRY_FLAG_DEPTH;
        if (depth_stencil->resource->format->stencil_size)
            entry->flags |= WINED3D_FBO_ENTRY_FLAG_STENCIL;
    }
    entry->rt_mask = context_generate_rt_mask(GL_COLOR_ATTACHMENT0);
    gl_info->fbo_ops.glGenFramebuffers(1, &entry->id);
    checkGLcall("glGenFramebuffers()");
    TRACE("Created FBO %u.\n", entry->id);

    return entry;
}

/* Context activation is done by the caller. */
static void wined3d_context_gl_reuse_fbo_entry(struct wined3d_context_gl *context_gl, GLenum target,
        const struct wined3d_rendertarget_info *render_targets, const struct wined3d_rendertarget_info *depth_stencil,
        DWORD color_location, DWORD ds_location, struct fbo_entry *entry)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    wined3d_context_gl_bind_fbo(context_gl, target, entry->id);
    context_clean_fbo_attachments(gl_info, target);

    wined3d_context_gl_generate_fbo_key(context_gl, &entry->key,
            render_targets, depth_stencil, color_location, ds_location);
    entry->flags = 0;
    if (depth_stencil->resource)
    {
        if (depth_stencil->resource->format->depth_size)
            entry->flags |= WINED3D_FBO_ENTRY_FLAG_DEPTH;
        if (depth_stencil->resource->format->stencil_size)
            entry->flags |= WINED3D_FBO_ENTRY_FLAG_STENCIL;
    }
}

/* Context activation is done by the caller. */
static void wined3d_context_gl_destroy_fbo_entry(struct wined3d_context_gl *context_gl, struct fbo_entry *entry)
{
    if (entry->id)
    {
        TRACE("Destroy FBO %u.\n", entry->id);
        wined3d_context_gl_destroy_fbo(context_gl, entry->id);
    }
    --context_gl->fbo_entry_count;
    list_remove(&entry->entry);
    free(entry);
}

/* Context activation is done by the caller. */
static struct fbo_entry *wined3d_context_gl_find_fbo_entry(struct wined3d_context_gl *context_gl, GLenum target,
        const struct wined3d_rendertarget_info *render_targets, const struct wined3d_rendertarget_info *depth_stencil,
        DWORD color_location, DWORD ds_location)
{
    static const struct wined3d_rendertarget_info ds_null = {{0}};
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_texture *rt_texture, *ds_texture;
    struct wined3d_fbo_entry_key fbo_key;
    unsigned int i, ds_level, rt_level;
    struct fbo_entry *entry;

    if (depth_stencil->resource && depth_stencil->resource->type != WINED3D_RTYPE_BUFFER
            && render_targets[0].resource && render_targets[0].resource->type != WINED3D_RTYPE_BUFFER
            && render_targets[0].resource->format->id != WINED3DFMT_NULL)
    {
        rt_texture = wined3d_texture_from_resource(render_targets[0].resource);
        rt_level = render_targets[0].sub_resource_idx % rt_texture->level_count;
        ds_texture = wined3d_texture_from_resource(depth_stencil->resource);
        ds_level = depth_stencil->sub_resource_idx % ds_texture->level_count;

        if (wined3d_texture_get_level_width(ds_texture, ds_level)
                < wined3d_texture_get_level_width(rt_texture, rt_level)
                || wined3d_texture_get_level_height(ds_texture, ds_level)
                < wined3d_texture_get_level_height(rt_texture, rt_level))
        {
            WARN("Depth stencil is smaller than the primary color buffer, disabling.\n");
            depth_stencil = &ds_null;
        }
        else if (ds_texture->resource.multisample_type != rt_texture->resource.multisample_type
                || (ds_texture->resource.multisample_type
                && ds_texture->resource.multisample_quality != rt_texture->resource.multisample_quality))
        {
            WARN("Color multisample type %u and quality %u, depth stencil has %u and %u, disabling ds buffer.\n",
                    rt_texture->resource.multisample_type, rt_texture->resource.multisample_quality,
                    ds_texture->resource.multisample_type, ds_texture->resource.multisample_quality);
            depth_stencil = &ds_null;
        }
        else if (depth_stencil->resource->type == WINED3D_RTYPE_TEXTURE_2D)
        {
            wined3d_texture_gl_set_compatible_renderbuffer(wined3d_texture_gl(ds_texture),
                    context_gl, ds_level, &render_targets[0]);
        }
    }

    wined3d_context_gl_generate_fbo_key(context_gl, &fbo_key,
            render_targets, depth_stencil, color_location, ds_location);

    if (TRACE_ON(d3d))
    {
        struct wined3d_resource *resource;
        unsigned int width, height;
        const char *resource_type;

        TRACE("Dumping FBO attachments:\n");
        for (i = 0; i < gl_info->limits.buffers; ++i)
        {
            if ((resource = render_targets[i].resource))
            {
                if (resource->type == WINED3D_RTYPE_BUFFER)
                {
                    width = resource->size;
                    height = 1;
                    resource_type = "buffer";
                }
                else
                {
                    rt_texture = wined3d_texture_from_resource(resource);
                    rt_level = render_targets[i].sub_resource_idx % rt_texture->level_count;
                    width = wined3d_texture_get_level_width(rt_texture, rt_level);
                    height = wined3d_texture_get_level_height(rt_texture, rt_level);
                    resource_type = "texture";
                }

                TRACE("    Color attachment %u: %p, %u format %s, %s %u, %ux%u, %u samples.\n",
                        i, resource, render_targets[i].sub_resource_idx, debug_d3dformat(resource->format->id),
                        fbo_key.rb_namespace & (1 << (i + 1)) ? "renderbuffer" : resource_type,
                        fbo_key.objects[i + 1].object, width, height, resource->multisample_type);
            }
        }
        if ((resource = depth_stencil->resource))
        {
            if (resource->type == WINED3D_RTYPE_BUFFER)
            {
                width = resource->size;
                height = 1;
                resource_type = "buffer";
            }
            else
            {
                ds_texture = wined3d_texture_from_resource(resource);
                ds_level = depth_stencil->sub_resource_idx % ds_texture->level_count;
                width = wined3d_texture_get_level_width(ds_texture, ds_level);
                height = wined3d_texture_get_level_height(ds_texture, ds_level);
                resource_type = "texture";
            }

            TRACE("    Depth attachment: %p, %u format %s, %s %u, %ux%u, %u samples.\n",
                    resource, depth_stencil->sub_resource_idx, debug_d3dformat(resource->format->id),
                    fbo_key.rb_namespace & (1 << 0) ? "renderbuffer" : resource_type,
                    fbo_key.objects[0].object, width, height, resource->multisample_type);
        }
    }

    LIST_FOR_EACH_ENTRY(entry, &context_gl->fbo_list, struct fbo_entry, entry)
    {
        if (memcmp(&fbo_key, &entry->key, sizeof(fbo_key)))
            continue;

        list_remove(&entry->entry);
        list_add_head(&context_gl->fbo_list, &entry->entry);
        return entry;
    }

    if (context_gl->fbo_entry_count < WINED3D_MAX_FBO_ENTRIES)
    {
        entry = wined3d_context_gl_create_fbo_entry(context_gl,
                render_targets, depth_stencil, color_location, ds_location);
        list_add_head(&context_gl->fbo_list, &entry->entry);
        ++context_gl->fbo_entry_count;
    }
    else
    {
        entry = LIST_ENTRY(list_tail(&context_gl->fbo_list), struct fbo_entry, entry);
        wined3d_context_gl_reuse_fbo_entry(context_gl, target, render_targets,
                depth_stencil, color_location, ds_location, entry);
        list_remove(&entry->entry);
        list_add_head(&context_gl->fbo_list, &entry->entry);
    }

    return entry;
}

/* Context activation is done by the caller. */
static void wined3d_context_gl_apply_fbo_entry(struct wined3d_context_gl *context_gl,
        GLenum target, struct fbo_entry *entry)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    GLuint read_binding, draw_binding;
    unsigned int i;

    if (entry->flags & WINED3D_FBO_ENTRY_FLAG_ATTACHED)
    {
        wined3d_context_gl_bind_fbo(context_gl, target, entry->id);
        return;
    }

    read_binding = context_gl->fbo_read_binding;
    draw_binding = context_gl->fbo_draw_binding;
    wined3d_context_gl_bind_fbo(context_gl, GL_FRAMEBUFFER, entry->id);

    if (gl_info->supported[ARB_FRAMEBUFFER_NO_ATTACHMENTS])
    {
        GL_EXTCALL(glFramebufferParameteri(GL_FRAMEBUFFER,
                GL_FRAMEBUFFER_DEFAULT_WIDTH, gl_info->limits.framebuffer_width));
        GL_EXTCALL(glFramebufferParameteri(GL_FRAMEBUFFER,
                GL_FRAMEBUFFER_DEFAULT_HEIGHT, gl_info->limits.framebuffer_height));
        GL_EXTCALL(glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_LAYERS, 1));
        GL_EXTCALL(glFramebufferParameteri(GL_FRAMEBUFFER, GL_FRAMEBUFFER_DEFAULT_SAMPLES, 1));
    }

    /* Apply render targets */
    for (i = 0; i < gl_info->limits.buffers; ++i)
    {
        wined3d_context_gl_attach_surface_fbo(context_gl, target, i,
                &entry->key.objects[i + 1], entry->key.rb_namespace & (1 << (i + 1)));
    }

    wined3d_context_gl_attach_depth_stencil_fbo(context_gl, target,
            &entry->key.objects[0], entry->key.rb_namespace & 0x1, entry->flags);

    /* Set valid read and draw buffer bindings to satisfy pedantic pre-ES2_compatibility
     * GL contexts requirements. */
    gl_info->gl_ops.gl.p_glReadBuffer(GL_NONE);
    wined3d_context_gl_set_draw_buffer(context_gl, GL_NONE);
    if (target != GL_FRAMEBUFFER)
    {
        if (target == GL_READ_FRAMEBUFFER)
            wined3d_context_gl_bind_fbo(context_gl, GL_DRAW_FRAMEBUFFER, draw_binding);
        else
            wined3d_context_gl_bind_fbo(context_gl, GL_READ_FRAMEBUFFER, read_binding);
    }

    entry->flags |= WINED3D_FBO_ENTRY_FLAG_ATTACHED;
}

/* Context activation is done by the caller. */
static void wined3d_context_gl_apply_fbo_state(struct wined3d_context_gl *context_gl, GLenum target,
        const struct wined3d_rendertarget_info *render_targets,
        const struct wined3d_rendertarget_info *depth_stencil, DWORD color_location, DWORD ds_location)
{
    struct fbo_entry *entry, *entry2;

    LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &context_gl->fbo_destroy_list, struct fbo_entry, entry)
    {
        wined3d_context_gl_destroy_fbo_entry(context_gl, entry);
    }

    if (context_gl->rebind_fbo)
    {
        wined3d_context_gl_bind_fbo(context_gl, GL_FRAMEBUFFER, 0);
        context_gl->rebind_fbo = FALSE;
    }

    if (color_location == WINED3D_LOCATION_DRAWABLE)
    {
        context_gl->current_fbo = NULL;
        wined3d_context_gl_bind_fbo(context_gl, target, 0);
    }
    else
    {
        context_gl->current_fbo = wined3d_context_gl_find_fbo_entry(context_gl,
                target, render_targets, depth_stencil, color_location, ds_location);
        wined3d_context_gl_apply_fbo_entry(context_gl, target, context_gl->current_fbo);
    }
}

/* Context activation is done by the caller. */
void wined3d_context_gl_apply_fbo_state_explicit(struct wined3d_context_gl *context_gl, GLenum target,
        struct wined3d_resource *rt, unsigned int rt_sub_resource_idx,
        struct wined3d_resource *ds, unsigned int ds_sub_resource_idx, uint32_t location)
{
    struct wined3d_rendertarget_info ds_info = {{0}};

    memset(context_gl->blit_targets, 0, sizeof(context_gl->blit_targets));
    if (rt)
    {
        context_gl->blit_targets[0].resource = rt;
        context_gl->blit_targets[0].sub_resource_idx = rt_sub_resource_idx;
        context_gl->blit_targets[0].layer_count = 1;
    }

    if (ds)
    {
        ds_info.resource = ds;
        ds_info.sub_resource_idx = ds_sub_resource_idx;
        ds_info.layer_count = 1;
    }

    wined3d_context_gl_apply_fbo_state(context_gl, target, context_gl->blit_targets, &ds_info, location, location);
}

/* Context activation is done by the caller. */
void wined3d_context_gl_alloc_occlusion_query(struct wined3d_context_gl *context_gl,
        struct wined3d_occlusion_query *query)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    if (context_gl->free_occlusion_query_count)
    {
        query->id = context_gl->free_occlusion_queries[--context_gl->free_occlusion_query_count];
    }
    else
    {
        if (gl_info->supported[ARB_OCCLUSION_QUERY])
        {
            GL_EXTCALL(glGenQueries(1, &query->id));
            checkGLcall("glGenQueries");

            TRACE("Allocated occlusion query %u in context %p.\n", query->id, context_gl);
        }
        else
        {
            WARN("Occlusion queries not supported, not allocating query id.\n");
            query->id = 0;
        }
    }

    query->context_gl = context_gl;
    list_add_head(&context_gl->occlusion_queries, &query->entry);
}

void wined3d_context_gl_free_occlusion_query(struct wined3d_occlusion_query *query)
{
    struct wined3d_context_gl *context_gl = query->context_gl;

    list_remove(&query->entry);
    query->context_gl = NULL;

    if (!wined3d_array_reserve((void **)&context_gl->free_occlusion_queries,
            &context_gl->free_occlusion_query_size, context_gl->free_occlusion_query_count + 1,
            sizeof(*context_gl->free_occlusion_queries)))
    {
        ERR("Failed to grow free list, leaking query %u in context %p.\n", query->id, context_gl);
        return;
    }

    context_gl->free_occlusion_queries[context_gl->free_occlusion_query_count++] = query->id;
}

/* Context activation is done by the caller. */
void wined3d_context_gl_alloc_fence(struct wined3d_context_gl *context_gl, struct wined3d_fence *fence)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    if (context_gl->free_fence_count)
    {
        fence->object = context_gl->free_fences[--context_gl->free_fence_count];
    }
    else
    {
        if (gl_info->supported[ARB_SYNC])
        {
            /* Using ARB_sync, not much to do here. */
            fence->object.sync = NULL;
            TRACE("Allocated sync object in context %p.\n", context_gl);
        }
        else if (gl_info->supported[APPLE_FENCE])
        {
            GL_EXTCALL(glGenFencesAPPLE(1, &fence->object.id));
            checkGLcall("glGenFencesAPPLE");

            TRACE("Allocated fence %u in context %p.\n", fence->object.id, context_gl);
        }
        else if(gl_info->supported[NV_FENCE])
        {
            GL_EXTCALL(glGenFencesNV(1, &fence->object.id));
            checkGLcall("glGenFencesNV");

            TRACE("Allocated fence %u in context %p.\n", fence->object.id, context_gl);
        }
        else
        {
            WARN("Fences not supported, not allocating fence.\n");
            fence->object.id = 0;
        }
    }

    fence->context_gl = context_gl;
    list_add_head(&context_gl->fences, &fence->entry);
}

void wined3d_context_gl_free_fence(struct wined3d_fence *fence)
{
    struct wined3d_context_gl *context_gl = fence->context_gl;

    list_remove(&fence->entry);
    fence->context_gl = NULL;

    if (!wined3d_array_reserve((void **)&context_gl->free_fences,
            &context_gl->free_fence_size, context_gl->free_fence_count + 1,
            sizeof(*context_gl->free_fences)))
    {
        ERR("Failed to grow free list, leaking fence %u in context %p.\n", fence->object.id, context_gl);
        return;
    }

    context_gl->free_fences[context_gl->free_fence_count++] = fence->object;
}

/* Context activation is done by the caller. */
void wined3d_context_gl_alloc_timestamp_query(struct wined3d_context_gl *context_gl,
        struct wined3d_timestamp_query *query)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    if (context_gl->free_timestamp_query_count)
    {
        query->id = context_gl->free_timestamp_queries[--context_gl->free_timestamp_query_count];
    }
    else
    {
        GL_EXTCALL(glGenQueries(1, &query->id));
        checkGLcall("glGenQueries");

        TRACE("Allocated timestamp query %u in context %p.\n", query->id, context_gl);
    }

    query->context_gl = context_gl;
    list_add_head(&context_gl->timestamp_queries, &query->entry);
}

void wined3d_context_gl_free_timestamp_query(struct wined3d_timestamp_query *query)
{
    struct wined3d_context_gl *context_gl = query->context_gl;

    list_remove(&query->entry);
    query->context_gl = NULL;

    if (!wined3d_array_reserve((void **)&context_gl->free_timestamp_queries,
            &context_gl->free_timestamp_query_size, context_gl->free_timestamp_query_count + 1,
            sizeof(*context_gl->free_timestamp_queries)))
    {
        ERR("Failed to grow free list, leaking query %u in context %p.\n", query->id, context_gl);
        return;
    }

    context_gl->free_timestamp_queries[context_gl->free_timestamp_query_count++] = query->id;
}

void wined3d_context_gl_alloc_so_statistics_query(struct wined3d_context_gl *context_gl,
        struct wined3d_so_statistics_query *query)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    if (context_gl->free_so_statistics_query_count)
    {
        query->u = context_gl->free_so_statistics_queries[--context_gl->free_so_statistics_query_count];
    }
    else
    {
        GL_EXTCALL(glGenQueries(ARRAY_SIZE(query->u.id), query->u.id));
        checkGLcall("glGenQueries");

        TRACE("Allocated SO statistics queries %u, %u in context %p.\n",
                query->u.id[0], query->u.id[1], context_gl);
    }

    query->context_gl = context_gl;
    list_add_head(&context_gl->so_statistics_queries, &query->entry);
}

void wined3d_context_gl_free_so_statistics_query(struct wined3d_so_statistics_query *query)
{
    struct wined3d_context_gl *context_gl = query->context_gl;

    list_remove(&query->entry);
    query->context_gl = NULL;

    if (!wined3d_array_reserve((void **)&context_gl->free_so_statistics_queries,
            &context_gl->free_so_statistics_query_size, context_gl->free_so_statistics_query_count + 1,
            sizeof(*context_gl->free_so_statistics_queries)))
    {
        ERR("Failed to grow free list, leaking GL queries %u, %u in context %p.\n",
                query->u.id[0], query->u.id[1], context_gl);
        return;
    }

    context_gl->free_so_statistics_queries[context_gl->free_so_statistics_query_count++] = query->u;
}

void wined3d_context_gl_alloc_pipeline_statistics_query(struct wined3d_context_gl *context_gl,
        struct wined3d_pipeline_statistics_query *query)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    if (context_gl->free_pipeline_statistics_query_count)
    {
        query->u = context_gl->free_pipeline_statistics_queries[--context_gl->free_pipeline_statistics_query_count];
    }
    else
    {
        GL_EXTCALL(glGenQueries(ARRAY_SIZE(query->u.id), query->u.id));
        checkGLcall("glGenQueries");
    }

    query->context_gl = context_gl;
    list_add_head(&context_gl->pipeline_statistics_queries, &query->entry);
}

void wined3d_context_gl_free_pipeline_statistics_query(struct wined3d_pipeline_statistics_query *query)
{
    struct wined3d_context_gl *context_gl = query->context_gl;

    list_remove(&query->entry);
    query->context_gl = NULL;

    if (!wined3d_array_reserve((void **)&context_gl->free_pipeline_statistics_queries,
            &context_gl->free_pipeline_statistics_query_size, context_gl->free_pipeline_statistics_query_count + 1,
            sizeof(*context_gl->free_pipeline_statistics_queries)))
    {
        ERR("Failed to grow free list, leaking GL queries in context %p.\n", context_gl);
        return;
    }

    context_gl->free_pipeline_statistics_queries[context_gl->free_pipeline_statistics_query_count++] = query->u;
}

typedef void (context_fbo_entry_func_t)(struct wined3d_context_gl *context_gl, struct fbo_entry *entry);

static void wined3d_context_gl_enum_fbo_entries(const struct wined3d_device *device,
        GLuint name, BOOL rb_namespace, context_fbo_entry_func_t *callback)
{
    unsigned int i, j;

    for (i = 0; i < device->context_count; ++i)
    {
        struct wined3d_context_gl *context_gl = wined3d_context_gl(device->contexts[i]);
        const struct wined3d_gl_info *gl_info = context_gl->gl_info;
        struct fbo_entry *entry, *entry2;

        LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &context_gl->fbo_list, struct fbo_entry, entry)
        {
            for (j = 0; j < gl_info->limits.buffers + 1; ++j)
            {
                if (entry->key.objects[j].object == name
                        && !(entry->key.rb_namespace & (1 << j)) == !rb_namespace)
                {
                    callback(context_gl, entry);
                    break;
                }
            }
        }
    }
}

static void wined3d_context_gl_queue_fbo_entry_destruction(struct wined3d_context_gl *context_gl,
        struct fbo_entry *entry)
{
    list_remove(&entry->entry);
    list_add_head(&context_gl->fbo_destroy_list, &entry->entry);
}

void context_gl_resource_released(struct wined3d_device *device, GLuint name, BOOL rb_namespace)
{
    wined3d_context_gl_enum_fbo_entries(device, name, rb_namespace,
            wined3d_context_gl_queue_fbo_entry_destruction);
}

void wined3d_context_gl_texture_update(struct wined3d_context_gl *context_gl,
        const struct wined3d_texture_gl *texture_gl)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct fbo_entry *entry = context_gl->current_fbo;
    unsigned int i;

    if (!entry || context_gl->rebind_fbo)
        return;

    for (i = 0; i < gl_info->limits.buffers + 1; ++i)
    {
        if (texture_gl->texture_rgb.name == entry->key.objects[i].object
                || texture_gl->texture_srgb.name == entry->key.objects[i].object)
        {
            TRACE("Updated texture %p is bound as attachment %u to the current FBO.\n", texture_gl, i);
            context_gl->rebind_fbo = TRUE;
            return;
        }
    }
}

static BOOL wined3d_context_gl_restore_pixel_format(struct wined3d_context_gl *context_gl)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    BOOL ret = FALSE;

    if (context_gl->restore_pf && IsWindow(context_gl->restore_pf_win))
    {
        if (gl_info->supported[WGL_WINE_PIXEL_FORMAT_PASSTHROUGH])
        {
            HDC dc = GetDCEx(context_gl->restore_pf_win, 0, DCX_USESTYLE | DCX_CACHE);
            if (dc)
            {
                if (!(ret = GL_EXTCALL(wglSetPixelFormatWINE(dc, context_gl->restore_pf))))
                {
                    ERR("Failed to restore pixel format %d on window %p.\n",
                            context_gl->restore_pf, context_gl->restore_pf_win);
                }
                ReleaseDC(context_gl->restore_pf_win, dc);
            }
        }
        else
        {
            ERR("Unable to restore pixel format %d on window %p.\n",
                    context_gl->restore_pf, context_gl->restore_pf_win);
        }
    }

    context_gl->restore_pf = 0;
    context_gl->restore_pf_win = NULL;
    return ret;
}

static BOOL wined3d_context_gl_set_pixel_format(struct wined3d_context_gl *context_gl)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    BOOL private = context_gl->dc_is_private;
    int format = context_gl->pixel_format;
    HDC dc = context_gl->dc;
    int current;
    HWND win;

    if (private && context_gl->dc_has_format)
        return TRUE;

    if (!private && WindowFromDC(dc) != context_gl->window)
        return FALSE;

    current = gl_info->gl_ops.wgl.p_wglGetPixelFormat(dc);
    if ((current == format) || (!current && context_gl->internal_format_set))
        goto success;

    /* By default WGL doesn't allow pixel format adjustments but we need it
     * here. For this reason there's a Wine specific wglSetPixelFormat()
     * which allows us to set the pixel format multiple times. Use it when we
     * can, because even though no pixel format may currently be set, the
     * application may try to set one later. */
    if (gl_info->supported[WGL_WINE_PIXEL_FORMAT_PASSTHROUGH])
    {
        if (!GL_EXTCALL(wglSetPixelFormatWINE(dc, format)))
        {
            ERR("wglSetPixelFormatWINE failed to set pixel format %d on device context %p.\n",
                    format, dc);
            return FALSE;
        }
        context_gl->internal_format_set = 1;
    }
    else if (current)
    {
        /* OpenGL doesn't allow pixel format adjustments. Print an error and
         * continue using the old format. There's a big chance that the old
         * format works although with a performance hit and perhaps rendering
         * errors. */
        ERR("Unable to set pixel format %d on device context %p. Already using format %d.\n",
                format, dc, current);
        return TRUE;
    }
    else if (!SetPixelFormat(dc, format, NULL))
    {
        /* This may also happen if the dc belongs to a destroyed window. */
        WARN("Failed to set pixel format %d on device context %p, last error %#lx.\n",
                format, dc, GetLastError());
        return FALSE;
    }

    win = private ? NULL : WindowFromDC(dc);
    if (win != context_gl->restore_pf_win)
        wined3d_context_gl_restore_pixel_format(context_gl);
    context_gl->restore_pf = private ? 0 : current;
    context_gl->restore_pf_win = win;

success:
    if (private)
        context_gl->dc_has_format = TRUE;
    return TRUE;
}

static BOOL wined3d_context_gl_set_gl_context(struct wined3d_context_gl *context_gl)
{
    struct wined3d_device_gl *device_gl = wined3d_device_gl(context_gl->c.device);
    BOOL backup = FALSE;

    TRACE("context_gl %p.\n", context_gl);

    if (!wined3d_context_gl_set_pixel_format(context_gl))
    {
        WARN("Failed to set pixel format %d on device context %p.\n",
                context_gl->pixel_format, context_gl->dc);
        backup = TRUE;
    }

    if (backup || !wglMakeCurrent(context_gl->dc, context_gl->gl_ctx))
    {
        WARN("Failed to make GL context %p current on device context %p, last error %#lx.\n",
                context_gl->gl_ctx, context_gl->dc, GetLastError());
        context_gl->valid = 0;
        WARN("Trying fallback to the backup window.\n");

        if (context_gl->c.destroyed)
        {
            FIXME("Unable to get backup dc for destroyed context %p.\n", context_gl);
            wined3d_context_gl_set_current(NULL);
            return FALSE;
        }

        wined3d_release_dc(context_gl->window, context_gl->dc);
        if (!(context_gl->dc = wined3d_device_gl_get_backup_dc(device_gl)))
        {
            wined3d_context_gl_set_current(NULL);
            return FALSE;
        }

        TRACE("Using backup DC %p.\n", context_gl->dc);
        context_gl->dc_is_private = TRUE;
        context_gl->dc_has_format = FALSE;

        if (!wined3d_context_gl_set_pixel_format(context_gl))
        {
            ERR("Failed to set pixel format %d on device context %p.\n",
                    context_gl->pixel_format, context_gl->dc);
            wined3d_context_gl_set_current(NULL);
            return FALSE;
        }

        if (!wglMakeCurrent(context_gl->dc, context_gl->gl_ctx))
        {
            ERR("Fallback to backup window (dc %p) failed too, last error %#lx.\n",
                    context_gl->dc, GetLastError());
            wined3d_context_gl_set_current(NULL);
            return FALSE;
        }

        context_gl->valid = 1;
    }
    context_gl->needs_set = 0;

    return TRUE;
}

static void context_restore_gl_context(const struct wined3d_gl_info *gl_info, HDC dc, HGLRC gl_ctx)
{
    if (!wglMakeCurrent(dc, gl_ctx))
    {
        ERR("Failed to restore GL context %p on device context %p, last error %#lx.\n",
                gl_ctx, dc, GetLastError());
        wined3d_context_gl_set_current(NULL);
    }
}

static void wined3d_context_gl_update_window(struct wined3d_context_gl *context_gl)
{
    if (!context_gl->c.swapchain)
        return;

    if (context_gl->window == context_gl->c.swapchain->win_handle)
        return;

    TRACE("Updating context %p window from %p to %p.\n",
            context_gl, context_gl->window, context_gl->c.swapchain->win_handle);

    if (context_gl->dc)
        wined3d_release_dc(context_gl->window, context_gl->dc);

    context_gl->window = context_gl->c.swapchain->win_handle;
    context_gl->dc_is_private = FALSE;
    context_gl->dc_has_format = FALSE;
    context_gl->needs_set = 1;
    context_gl->valid = 1;
    context_gl->internal_format_set = 0;

    if (!(context_gl->dc = GetDCEx(context_gl->window, 0, DCX_USESTYLE | DCX_CACHE)))
    {
        ERR("Failed to get a device context for window %p.\n", context_gl->window);
        context_gl->valid = 0;
    }
}

static void wined3d_context_gl_cleanup(struct wined3d_context_gl *context_gl)
{
    struct wined3d_pipeline_statistics_query *pipeline_statistics_query;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_so_statistics_query *so_statistics_query;
    struct wined3d_timestamp_query *timestamp_query;
    struct wined3d_occlusion_query *occlusion_query;
    struct wined3d_context_gl *current;
    struct fbo_entry *entry, *entry2;
    struct wined3d_fence *fence;
    HGLRC restore_ctx;
    HDC restore_dc;
    unsigned int i;

    TRACE("context_gl %p.\n", context_gl);

    restore_ctx = wglGetCurrentContext();
    restore_dc = wglGetCurrentDC();

    if (context_gl->valid && context_gl->gl_ctx != restore_ctx)
    {
        /* Attempting to restore a GL context corresponding to a wined3d
         * context is not particularly useful. Worse, when we're called from
         * wined3d_context_gl_destroy(), we subsequently clear the "current
         * D3D context" TLS value, which would cause
         * wined3d_context_gl_enter() to consider the GL context a non-D3D
         * context. */
        if ((current = wined3d_context_gl_get_current()) && current->gl_ctx == restore_ctx)
            restore_ctx = NULL;
        wined3d_context_gl_set_gl_context(context_gl);
    }
    else
    {
        restore_ctx = NULL;
    }

    if (context_gl->valid)
    {
        /* If we're here because we're switching away from a previously
         * destroyed context, acquiring a context in order to submit a fence
         * is problematic. In particular, we'd end up back here again in the
         * process of switching to the newly acquired context.
         *
         * If fences aren't supported there should be nothing to wait for
         * anyway, so just do nothing in that case. */
        if (context_gl->c.destroyed)
        {
            gl_info->gl_ops.gl.p_glFinish();
        }
        else if (context_gl->c.d3d_info->fences)
        {
            wined3d_context_gl_submit_command_fence(context_gl);
            wined3d_context_gl_wait_command_fence(context_gl,
                    wined3d_device_gl(context_gl->c.device)->current_fence_id - 1);
        }

        if (context_gl->dummy_arbfp_prog)
            GL_EXTCALL(glDeleteProgramsARB(1, &context_gl->dummy_arbfp_prog));

        if (context_gl->blit_vbo)
            GL_EXTCALL(glDeleteBuffers(1, &context_gl->blit_vbo));

        for (i = 0; i < context_gl->free_pipeline_statistics_query_count; ++i)
        {
            union wined3d_gl_pipeline_statistics_query *q = &context_gl->free_pipeline_statistics_queries[i];
            GL_EXTCALL(glDeleteQueries(ARRAY_SIZE(q->id), q->id));
        }

        for (i = 0; i < context_gl->free_so_statistics_query_count; ++i)
        {
            union wined3d_gl_so_statistics_query *q = &context_gl->free_so_statistics_queries[i];
            GL_EXTCALL(glDeleteQueries(ARRAY_SIZE(q->id), q->id));
        }

        if (context_gl->free_timestamp_query_count)
            GL_EXTCALL(glDeleteQueries(context_gl->free_timestamp_query_count, context_gl->free_timestamp_queries));

        if (gl_info->supported[ARB_SYNC])
        {
            for (i = 0; i < context_gl->free_fence_count; ++i)
            {
                GL_EXTCALL(glDeleteSync(context_gl->free_fences[i].sync));
            }
        }
        else if (gl_info->supported[APPLE_FENCE])
        {
            for (i = 0; i < context_gl->free_fence_count; ++i)
            {
                GL_EXTCALL(glDeleteFencesAPPLE(1, &context_gl->free_fences[i].id));
            }
        }
        else if (gl_info->supported[NV_FENCE])
        {
            for (i = 0; i < context_gl->free_fence_count; ++i)
            {
                GL_EXTCALL(glDeleteFencesNV(1, &context_gl->free_fences[i].id));
            }
        }

        if (context_gl->free_occlusion_query_count)
            GL_EXTCALL(glDeleteQueries(context_gl->free_occlusion_query_count, context_gl->free_occlusion_queries));

        checkGLcall("context cleanup");
    }
    free(context_gl->submitted.fences);
    free(context_gl->free_pipeline_statistics_queries);
    free(context_gl->free_so_statistics_queries);
    free(context_gl->free_timestamp_queries);
    free(context_gl->free_fences);
    free(context_gl->free_occlusion_queries);

    LIST_FOR_EACH_ENTRY(pipeline_statistics_query, &context_gl->pipeline_statistics_queries,
            struct wined3d_pipeline_statistics_query, entry)
    {
        if (context_gl->valid)
            GL_EXTCALL(glDeleteQueries(ARRAY_SIZE(pipeline_statistics_query->u.id), pipeline_statistics_query->u.id));
        pipeline_statistics_query->context_gl = NULL;
    }

    LIST_FOR_EACH_ENTRY(so_statistics_query, &context_gl->so_statistics_queries,
            struct wined3d_so_statistics_query, entry)
    {
        if (context_gl->valid)
            GL_EXTCALL(glDeleteQueries(ARRAY_SIZE(so_statistics_query->u.id), so_statistics_query->u.id));
        so_statistics_query->context_gl = NULL;
    }

    LIST_FOR_EACH_ENTRY(timestamp_query, &context_gl->timestamp_queries, struct wined3d_timestamp_query, entry)
    {
        if (context_gl->valid)
            GL_EXTCALL(glDeleteQueries(1, &timestamp_query->id));
        timestamp_query->context_gl = NULL;
    }

    LIST_FOR_EACH_ENTRY(fence, &context_gl->fences, struct wined3d_fence, entry)
    {
        if (context_gl->valid)
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
        fence->context_gl = NULL;
    }

    LIST_FOR_EACH_ENTRY(occlusion_query, &context_gl->occlusion_queries, struct wined3d_occlusion_query, entry)
    {
        if (context_gl->valid)
            GL_EXTCALL(glDeleteQueries(1, &occlusion_query->id));
        occlusion_query->context_gl = NULL;
    }

    LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &context_gl->fbo_destroy_list, struct fbo_entry, entry)
    {
        if (!context_gl->valid)
            entry->id = 0;
        wined3d_context_gl_destroy_fbo_entry(context_gl, entry);
    }

    LIST_FOR_EACH_ENTRY_SAFE(entry, entry2, &context_gl->fbo_list, struct fbo_entry, entry)
    {
        if (!context_gl->valid)
            entry->id = 0;
        wined3d_context_gl_destroy_fbo_entry(context_gl, entry);
    }

    free(context_gl->texture_type);

    wined3d_context_gl_restore_pixel_format(context_gl);
    if (restore_ctx)
        context_restore_gl_context(gl_info, restore_dc, restore_ctx);
    else if (wglGetCurrentContext() && !wglMakeCurrent(NULL, NULL))
        ERR("Failed to disable GL context.\n");

    wined3d_release_dc(context_gl->window, context_gl->dc);

    if (!wglDeleteContext(context_gl->gl_ctx))
    {
        unsigned int err = GetLastError();
        ERR("Failed to delete GL context %p, last error %#x.\n", context_gl->gl_ctx, err);
    }

    wined3d_context_cleanup(&context_gl->c);
}

DWORD context_get_tls_idx(void)
{
    return wined3d_context_tls_idx;
}

void context_set_tls_idx(DWORD idx)
{
    wined3d_context_tls_idx = idx;
}

struct wined3d_context_gl *wined3d_context_gl_get_current(void)
{
    return TlsGetValue(wined3d_context_tls_idx);
}

BOOL wined3d_context_gl_set_current(struct wined3d_context_gl *context_gl)
{
    struct wined3d_context_gl *old = wined3d_context_gl_get_current();

    if (old == context_gl)
    {
        TRACE("Already using D3D context %p.\n", context_gl);
        return TRUE;
    }

    if (old)
    {
        if (old->c.destroyed)
        {
            TRACE("Switching away from destroyed context %p.\n", old);
            wined3d_context_gl_cleanup(old);
            free((void *)old->gl_info);
            free(old);
        }
        else
        {
            if (wglGetCurrentContext())
            {
                const struct wined3d_gl_info *gl_info = old->gl_info;
                TRACE("Flushing context %p before switching to %p.\n", old, context_gl);
                gl_info->gl_ops.gl.p_glFlush();
            }
            old->c.current = 0;
        }
    }

    if (context_gl)
    {
        if (!context_gl->valid)
        {
            ERR("Trying to make invalid context %p current.\n", context_gl);
            return FALSE;
        }

        TRACE("Switching to D3D context %p, GL context %p, device context %p.\n",
                context_gl, context_gl->gl_ctx, context_gl->dc);
        if (!wined3d_context_gl_set_gl_context(context_gl))
            return FALSE;
        context_gl->c.current = 1;
    }
    else if (wglGetCurrentContext())
    {
        TRACE("Clearing current D3D context.\n");
        if (!wglMakeCurrent(NULL, NULL))
        {
            unsigned int err = GetLastError();
            ERR("Failed to clear current GL context, last error %#x.\n", err);
            TlsSetValue(wined3d_context_tls_idx, NULL);
            return FALSE;
        }
    }

    return TlsSetValue(wined3d_context_tls_idx, context_gl);
}

void wined3d_context_gl_release(struct wined3d_context_gl *context_gl)
{
    TRACE("Releasing context %p, level %u.\n", context_gl, context_gl->level);

    if (WARN_ON(d3d))
    {
        if (!context_gl->level)
            WARN("Context %p is not active.\n", context_gl);
        else if (context_gl != wined3d_context_gl_get_current())
            WARN("Context %p is not the current context.\n", context_gl);
    }

    if (!--context_gl->level)
    {
        if (wined3d_context_gl_restore_pixel_format(context_gl))
            context_gl->needs_set = 1;
        if (context_gl->restore_ctx)
        {
            TRACE("Restoring GL context %p on device context %p.\n", context_gl->restore_ctx, context_gl->restore_dc);
            context_restore_gl_context(context_gl->gl_info, context_gl->restore_dc, context_gl->restore_ctx);
            context_gl->restore_ctx = NULL;
            context_gl->restore_dc = NULL;
        }

        if (context_gl->c.destroy_delayed)
        {
            TRACE("Destroying context %p.\n", context_gl);
            wined3d_context_gl_destroy(context_gl);
        }
    }
}

static void wined3d_context_gl_enter(struct wined3d_context_gl *context_gl)
{
    TRACE("Entering context %p, level %u.\n", context_gl, context_gl->level + 1);

    if (!context_gl->level++)
    {
        const struct wined3d_context_gl *current_context = wined3d_context_gl_get_current();
        HGLRC current_gl = wglGetCurrentContext();

        if (current_gl && (!current_context || current_context->gl_ctx != current_gl))
        {
            TRACE("Another GL context (%p on device context %p) is already current.\n",
                    current_gl, wglGetCurrentDC());
            context_gl->restore_ctx = current_gl;
            context_gl->restore_dc = wglGetCurrentDC();
            context_gl->needs_set = 1;
        }
        else if (!context_gl->needs_set && !(context_gl->dc_is_private && context_gl->dc_has_format))
        {
            int current = context_gl->gl_info->gl_ops.wgl.p_wglGetPixelFormat(context_gl->dc);

            if ((current && current != context_gl->pixel_format) || (!current && !context_gl->internal_format_set))
                context_gl->needs_set = 1;
        }
    }
}

/* This function takes care of wined3d pixel format selection. */
static int context_choose_pixel_format(const struct wined3d_device *device, HDC hdc,
        const struct wined3d_format *color_format, const struct wined3d_format *ds_format,
        bool aux_buffers, bool swap_effect_copy)
{
    unsigned int cfg_count = wined3d_adapter_gl(device->adapter)->pixel_format_count;
    unsigned int current_value;
    PIXELFORMATDESCRIPTOR pfd;
    int iPixelFormat = 0;
    unsigned int i;

    TRACE("device %p, dc %p, color_format %s, ds_format %s, aux_buffers %#x, swap_effect_copy %#x.\n",
            device, hdc, debug_d3dformat(color_format->id), debug_d3dformat(ds_format->id),
            aux_buffers, swap_effect_copy);

    current_value = 0;
    for (i = 0; i < cfg_count; ++i)
    {
        const struct wined3d_pixel_format *cfg = &wined3d_adapter_gl(device->adapter)->pixel_formats[i];
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
        if (swap_effect_copy && cfg->swap_method == WGL_SWAP_COPY_ARB)
            value += 1;
        /* We try to locate a format which matches our requirements exactly. In case of
         * depth it is no problem to emulate 16-bit using e.g. 24-bit, so accept that. */
        if (cfg->depthSize == ds_format->depth_size)
            value += 2;
        if (cfg->stencilSize == ds_format->stencil_size)
            value += 4;
        if (cfg->alphaSize == color_format->alpha_size)
            value += 8;
        /* We like to have aux buffers in backbuffer mode */
        if (aux_buffers && cfg->auxBuffers)
            value += 16;
        if (cfg->redSize == color_format->red_size
                && cfg->greenSize == color_format->green_size
                && cfg->blueSize == color_format->blue_size)
            value += 32;

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
void wined3d_context_gl_bind_dummy_textures(const struct wined3d_context_gl *context_gl)
{
    const struct wined3d_dummy_textures *textures = &wined3d_device_gl(context_gl->c.device)->dummy_textures;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    unsigned int i;

    for (i = 0; i < gl_info->limits.combined_samplers; ++i)
    {
        GL_EXTCALL(glActiveTexture(GL_TEXTURE0 + i));

        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_1D, textures->tex_1d);
        gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, textures->tex_2d);

        if (gl_info->supported[EXT_TEXTURE3D])
            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_3D, textures->tex_3d);

        if (gl_info->supported[ARB_TEXTURE_CUBE_MAP])
            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_CUBE_MAP, textures->tex_cube);

        if (gl_info->supported[ARB_TEXTURE_CUBE_MAP_ARRAY])
            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, textures->tex_cube_array);

        if (gl_info->supported[EXT_TEXTURE_ARRAY])
        {
            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_1D_ARRAY, textures->tex_1d_array);
            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D_ARRAY, textures->tex_2d_array);
        }

        if (gl_info->supported[ARB_TEXTURE_BUFFER_OBJECT])
            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_BUFFER, textures->tex_buffer);

        if (gl_info->supported[ARB_TEXTURE_MULTISAMPLE])
        {
            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textures->tex_2d_ms);
            gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, textures->tex_2d_ms_array);
        }
    }

    checkGLcall("bind dummy textures");
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
    if (ctx_flags)
    {
        ctx_attribs[ctx_attrib_idx++] = WGL_CONTEXT_FLAGS_ARB;
        ctx_attribs[ctx_attrib_idx++] = ctx_flags;
    }
    ctx_attribs[ctx_attrib_idx] = 0;

    if (!(ctx = gl_info->p_wglCreateContextAttribsARB(hdc, share_ctx, ctx_attribs)))
    {
        if (gl_info->selected_gl_version >= MAKEDWORD_VERSION(3, 2))
        {
            if (ctx_flags)
            {
                ctx_flags |= WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
                ctx_attribs[ctx_attrib_idx - 1] = ctx_flags;
            }
            else
            {
                ctx_flags = WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
                ctx_attribs[ctx_attrib_idx++] = WGL_CONTEXT_FLAGS_ARB;
                ctx_attribs[ctx_attrib_idx++] = ctx_flags;
                ctx_attribs[ctx_attrib_idx] = 0;
            }
            if (!(ctx = gl_info->p_wglCreateContextAttribsARB(hdc, share_ctx, ctx_attribs)))
                WARN("Failed to create a WGL context with wglCreateContextAttribsARB, last error %#lx.\n",
                        GetLastError());
        }
    }
    return ctx;
}

static BOOL wined3d_context_gl_create_wgl_ctx(struct wined3d_context_gl *context_gl,
        struct wined3d_swapchain_gl *swapchain_gl, BOOL *new_drawable)
{
    enum wined3d_swap_effect swap_effect = swapchain_gl->s.state.desc.swap_effect;
    const struct wined3d_format *colour_format, *ds_format;
    struct wined3d_context *context = &context_gl->c;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_resource *target;
    struct wined3d_adapter *adapter;
    unsigned int target_bind_flags;
    struct wined3d_device *device;
    bool swap_effect_copy;
    HGLRC ctx, share_ctx;

    device = context->device;
    adapter = device->adapter;
    gl_info = &wined3d_adapter_gl(adapter)->gl_info;

    target = &context->current_rt.texture->resource;
    target_bind_flags = target->bind_flags;

    swap_effect_copy = swap_effect == WINED3D_SWAP_EFFECT_COPY || swap_effect == WINED3D_SWAP_EFFECT_COPY_VSYNC;

    *new_drawable = !GetPixelFormat(context_gl->dc);

    /* When using FBOs for off-screen rendering, we only use the drawable for
     * presentation blits, and don't do any rendering to it. That means we
     * don't need depth or stencil buffers, and can mostly ignore the render
     * target format. This wouldn't necessarily be quite correct for 10bpc
     * display modes, but we don't currently support those.
     * Using the same format regardless of the colour/depth/stencil targets
     * makes it much less likely that different wined3d instances will set
     * conflicting pixel formats. */
    colour_format = wined3d_get_format(adapter, WINED3DFMT_B8G8R8A8_UNORM, target_bind_flags);
    ds_format = wined3d_get_format(adapter, WINED3DFMT_UNKNOWN, WINED3D_BIND_DEPTH_STENCIL);
    context_gl->pixel_format = context_choose_pixel_format(device,
            context_gl->dc, colour_format, ds_format, false, swap_effect_copy);

    if (!context_gl->pixel_format)
    {
        ERR("Failed to choose pixel format.\n");
        return FALSE;
    }

    wined3d_context_gl_enter(context_gl);

    if (!wined3d_context_gl_set_pixel_format(context_gl))
    {
        context_release(context);

        if (context_gl->dc_is_private)
        {
            ERR("Failed to set pixel format %d on device context %p.\n", context_gl->pixel_format, context_gl->dc);

            return FALSE;
        }

        WARN("Failed to set pixel format %d on device context %p, trying backup DC.\n",
                context_gl->pixel_format, context_gl->dc);

        wined3d_release_dc(context_gl->window, context_gl->dc);
        if (!(context_gl->dc = wined3d_device_gl_get_backup_dc(wined3d_device_gl(device))))
        {
            ERR("Failed to retrieve the backup device context.\n");
            return FALSE;
        }
        context_gl->dc_is_private = TRUE;

        return wined3d_context_gl_create_wgl_ctx(context_gl, swapchain_gl, new_drawable);
    }

    share_ctx = device->context_count ? wined3d_context_gl(device->contexts[0])->gl_ctx : NULL;
    if (gl_info->p_wglCreateContextAttribsARB)
    {
        if (!(ctx = context_create_wgl_attribs(gl_info, context_gl->dc, share_ctx)))
        {
            ERR("Failed to create a WGL context.\n");
            context_release(context);
            return FALSE;
        }
    }
    else
    {
        if (!(ctx = wglCreateContext(context_gl->dc)))
        {
            ERR("Failed to create a WGL context.\n");
            context_release(context);
            return FALSE;
        }

        if (share_ctx && !wglShareLists(share_ctx, ctx))
        {
            ERR("wglShareLists(%p, %p) failed, last error %#lx.\n", share_ctx, ctx, GetLastError());
            context_release(context);
            if (!wglDeleteContext(ctx))
                ERR("wglDeleteContext(%p) failed, last error %#lx.\n", ctx, GetLastError());
            return FALSE;
        }
    }

    context_gl->dc_has_format = TRUE;
    context_gl->needs_set = 1;
    context_gl->valid = 1;
    context_gl->gl_ctx = ctx;

    return TRUE;
}

HRESULT wined3d_context_gl_init(struct wined3d_context_gl *context_gl, struct wined3d_swapchain_gl *swapchain_gl)
{
    struct wined3d_context *context = &context_gl->c;
    const struct wined3d_d3d_info *d3d_info;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_device *device;
    BOOL new_drawable;
    unsigned int i;

    TRACE("context_gl %p, swapchain %p.\n", context_gl, swapchain_gl);

    wined3d_context_init(&context_gl->c, &swapchain_gl->s);

    device = context->device;
    gl_info = &wined3d_adapter_gl(device->adapter)->gl_info;
    context_gl->gl_info = gl_info;
    d3d_info = context->d3d_info;

    context_gl->tid = GetCurrentThreadId();
    context_gl->window = context->swapchain->win_handle;
    if (context_gl->window == GetDesktopWindow())
    {
        TRACE("Swapchain is created on the desktop window, trying backup device context.\n");
        context_gl->dc = NULL;
    }
    else if (!(context_gl->dc = GetDCEx(context_gl->window, 0, DCX_USESTYLE | DCX_CACHE)))
        WARN("Failed to retrieve device context, trying swapchain backup.\n");

    if (!context_gl->dc)
    {
        if (!(context_gl->dc = wined3d_device_gl_get_backup_dc(wined3d_device_gl(device))))
        {
            ERR("Failed to retrieve a device context.\n");
            return E_FAIL;
        }
        context_gl->dc_is_private = TRUE;
    }

    list_init(&context_gl->fbo_list);
    list_init(&context_gl->fbo_destroy_list);

    list_init(&context_gl->occlusion_queries);
    list_init(&context_gl->fences);
    list_init(&context_gl->timestamp_queries);
    list_init(&context_gl->so_statistics_queries);
    list_init(&context_gl->pipeline_statistics_queries);

    for (i = 0; i < ARRAY_SIZE(context_gl->tex_unit_map); ++i)
        context_gl->tex_unit_map[i] = WINED3D_UNMAPPED_STAGE;
    for (i = 0; i < ARRAY_SIZE(context_gl->rev_tex_unit_map); ++i)
        context_gl->rev_tex_unit_map[i] = WINED3D_UNMAPPED_STAGE;
    if (gl_info->limits.graphics_samplers >= WINED3D_MAX_COMBINED_SAMPLERS)
    {
        /* Initialize the texture unit mapping to a 1:1 mapping. */
        unsigned int base, count;

        wined3d_gl_limits_get_texture_unit_range(&gl_info->limits, WINED3D_SHADER_TYPE_PIXEL, &base, &count);
        if (base + WINED3D_MAX_FRAGMENT_SAMPLERS > ARRAY_SIZE(context_gl->rev_tex_unit_map))
        {
            ERR("Unexpected texture unit base index %u.\n", base);
            goto fail;
        }
        for (i = 0; i < min(count, WINED3D_MAX_FRAGMENT_SAMPLERS); ++i)
        {
            context_gl->tex_unit_map[i] = base + i;
            context_gl->rev_tex_unit_map[base + i] = i;
        }

        wined3d_gl_limits_get_texture_unit_range(&gl_info->limits, WINED3D_SHADER_TYPE_VERTEX, &base, &count);
        if (base + WINED3D_MAX_VERTEX_SAMPLERS > ARRAY_SIZE(context_gl->rev_tex_unit_map))
        {
            ERR("Unexpected texture unit base index %u.\n", base);
            goto fail;
        }
        for (i = 0; i < min(count, WINED3D_MAX_VERTEX_SAMPLERS); ++i)
        {
            context_gl->tex_unit_map[WINED3D_MAX_FRAGMENT_SAMPLERS + i] = base + i;
            context_gl->rev_tex_unit_map[base + i] = WINED3D_MAX_FRAGMENT_SAMPLERS + i;
        }
    }

    if (!(context_gl->texture_type = calloc(gl_info->limits.combined_samplers,
            sizeof(*context_gl->texture_type))))
        goto fail;

    if (!wined3d_context_gl_create_wgl_ctx(context_gl, swapchain_gl, &new_drawable))
        goto fail;

    /* Set up the context defaults. */

    context_gl->draw_buffers_mask = context_generate_rt_mask(GL_BACK);

    if (!wined3d_context_gl_set_current(context_gl))
    {
        ERR("Cannot activate context to set up defaults.\n");
        context_release(context);
        if (!wglDeleteContext(context_gl->gl_ctx))
            ERR("wglDeleteContext(%p) failed, last error %#lx.\n", context_gl->gl_ctx, GetLastError());
        goto fail;
    }

    if (context_debug_output_enabled(gl_info))
    {
        GL_EXTCALL(glDebugMessageCallback(wined3d_debug_callback, context));
        if (TRACE_ON(d3d_sync))
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
        gl_info->gl_ops.gl.p_glGetIntegerv(GL_AUX_BUFFERS, &context_gl->aux_buffers);

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

    if (gl_info->supported[NV_TEXTURE_SHADER2])
    {
        /* Set up the previous texture input for all shader units. This applies to bump mapping, and in d3d
         * the previous texture where to source the offset from is always unit - 1.
         */
        for (i = 1; i < gl_info->limits.ffp_textures; ++i)
        {
            wined3d_context_gl_active_texture(context_gl, gl_info, i);
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
        GL_EXTCALL(glGenProgramsARB(1, &context_gl->dummy_arbfp_prog));
        GL_EXTCALL(glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, context_gl->dummy_arbfp_prog));
        GL_EXTCALL(glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB,
                GL_PROGRAM_FORMAT_ASCII_ARB, strlen(dummy_program), dummy_program));
    }

    if (gl_info->supported[ARB_POINT_SPRITE])
    {
        /* The actual effect of D3DRS_POINTSPRITEENABLE is more accurately
         * congruent to GL_COORD_REPLACE_ARB. GL_POINT_SPRITE_ARB, when
         * disabled, affects point rasterization (which we do not want),
         * implicitly disables GL_COORD_REPLACE_ARB, and causes gl_PointCoord
         * to return an undefined value.
         *
         * We implement D3DRS_POINTSPRITEENABLE by altering the shader to read
         * from gl_PointCoord or from the actual texcoord, so we do not need
         * GL_COORD_REPLACE_ARB, nor do we need to dynamically toggle
         * GL_POINT_SPRITE_ARB. */

        gl_info->gl_ops.gl.p_glEnable(GL_POINT_SPRITE_ARB);

        for (i = 0; i < gl_info->limits.ffp_textures; ++i)
        {
            wined3d_context_gl_active_texture(context_gl, gl_info, i);
            gl_info->gl_ops.gl.p_glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
            checkGLcall("glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE)");
        }
    }

    if (!gl_info->supported[ARB_CLIP_CONTROL] && gl_info->supported[WINED3D_GL_VERSION_2_0])
    {
        GL_EXTCALL(glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, GL_LOWER_LEFT));
        checkGLcall("glPointParameteri(GL_POINT_SPRITE_COORD_ORIGIN, GL_LOWER_LEFT)");
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

    /* If this happens to be the first context for the device, dummy textures
     * are not created yet. In that case, they will be created (and bound) by
     * create_dummy_textures right after this context is initialized. */
    if (wined3d_device_gl(device)->dummy_textures.tex_2d)
        wined3d_context_gl_bind_dummy_textures(context_gl);

    /* Initialise all rectangles to avoid resetting unused ones later. */
    gl_info->gl_ops.gl.p_glScissor(0, 0, 0, 0);
    checkGLcall("glScissor");

    if (new_drawable)
    {
        gl_info->gl_ops.gl.p_glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        gl_info->gl_ops.gl.p_glClearDepth(1.0f);
        gl_info->gl_ops.gl.p_glClearStencil(0);
        gl_info->gl_ops.gl.p_glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        checkGLcall("glClear");
    }
    return WINED3D_OK;

fail:
    free(context_gl->texture_type);
    wined3d_release_dc(context_gl->window, context_gl->dc);
    return E_FAIL;
}

void wined3d_context_gl_destroy(struct wined3d_context_gl *context_gl)
{
    struct wined3d_device *device = context_gl->c.device;

    TRACE("Destroying context %p.\n", context_gl);

    wined3d_from_cs(device->cs);

    /* We delay destroying a context when it is active. The context_release()
     * function invokes wined3d_context_gl_destroy() again while leaving the
     * last level. */
    if (context_gl->level)
    {
        TRACE("Delaying destruction of context %p.\n", context_gl);
        context_gl->c.destroy_delayed = 1;
        /* FIXME: Get rid of a pointer to swapchain from wined3d_context. */
        context_gl->c.swapchain = NULL;
        context_gl->c.device = NULL;
        return;
    }

    device_context_remove(device, &context_gl->c);

    if (context_gl->c.current && context_gl->tid != GetCurrentThreadId())
    {
        struct wined3d_gl_info *gl_info;

        /* Make a copy of gl_info for wined3d_context_gl_cleanup() use, the
         * one in wined3d_adapter may go away in the meantime. */
        gl_info = malloc(sizeof(*gl_info));
        *gl_info = *context_gl->gl_info;
        context_gl->gl_info = gl_info;
        context_gl->c.destroyed = 1;

        return;
    }

    wined3d_context_gl_cleanup(context_gl);
    TlsSetValue(context_get_tls_idx(), NULL);
    free(context_gl);
}

const unsigned int *wined3d_context_gl_get_tex_unit_mapping(const struct wined3d_context_gl *context_gl,
        const struct wined3d_shader_version *shader_version, unsigned int *base, unsigned int *count)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    if (!shader_version)
    {
        *base = 0;
        *count = WINED3D_MAX_FFP_TEXTURES;
        return context_gl->tex_unit_map;
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
            *count = WINED3D_MAX_FRAGMENT_SAMPLERS;
            break;
        case WINED3D_SHADER_TYPE_VERTEX:
            *base = WINED3D_MAX_FRAGMENT_SAMPLERS;
            *count = WINED3D_MAX_VERTEX_SAMPLERS;
            break;
        default:
            ERR("Unhandled shader type %#x.\n", shader_version->type);
            *base = 0;
            *count = 0;
    }

    return context_gl->tex_unit_map;
}

static void wined3d_context_gl_get_rt_size(const struct wined3d_context_gl *context_gl, SIZE *size)
{
    const struct wined3d_texture *rt = context_gl->c.current_rt.texture;
    unsigned int level;

    if (rt->swapchain)
    {
        RECT window_size;

        GetClientRect(context_gl->window, &window_size);
        size->cx = window_size.right - window_size.left;
        size->cy = window_size.bottom - window_size.top;

        return;
    }

    level = context_gl->c.current_rt.sub_resource_idx % rt->level_count;
    size->cx = wined3d_texture_get_level_width(rt, level);
    size->cy = wined3d_texture_get_level_height(rt, level);
}

void wined3d_context_gl_enable_clip_distances(struct wined3d_context_gl *context_gl, uint32_t enable_mask)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    unsigned int clip_distance_count, i;
    uint32_t disable_mask, current_mask;

    clip_distance_count = gl_info->limits.user_clip_distances;
    disable_mask = ~enable_mask;
    enable_mask &= wined3d_mask_from_size(clip_distance_count);
    disable_mask &= wined3d_mask_from_size(clip_distance_count);
    current_mask = context_gl->c.clip_distance_mask;
    context_gl->c.clip_distance_mask = enable_mask;

    enable_mask &= ~current_mask;
    while (enable_mask)
    {
        i = wined3d_bit_scan(&enable_mask);
        gl_info->gl_ops.gl.p_glEnable(GL_CLIP_DISTANCE0 + i);
    }
    disable_mask &= current_mask;
    while (disable_mask)
    {
        i = wined3d_bit_scan(&disable_mask);
        gl_info->gl_ops.gl.p_glDisable(GL_CLIP_DISTANCE0 + i);
    }
    checkGLcall("toggle clip distances");
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
static void wined3d_context_gl_apply_draw_buffers(struct wined3d_context_gl *context_gl, uint32_t rt_mask)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    GLenum draw_buffers[WINED3D_MAX_RENDER_TARGETS];

    if (!rt_mask)
    {
        gl_info->gl_ops.gl.p_glDrawBuffer(GL_NONE);
    }
    else if (is_rt_mask_onscreen(rt_mask))
    {
        gl_info->gl_ops.gl.p_glDrawBuffer(draw_buffer_from_rt_mask(rt_mask));
    }
    else
    {
        unsigned int i = 0;

        while (rt_mask)
        {
            if (rt_mask & 1)
                draw_buffers[i] = GL_COLOR_ATTACHMENT0 + i;
            else
                draw_buffers[i] = GL_NONE;

            rt_mask >>= 1;
            ++i;
        }

        if (gl_info->supported[ARB_DRAW_BUFFERS])
        {
            GL_EXTCALL(glDrawBuffers(i, draw_buffers));
        }
        else
        {
            gl_info->gl_ops.gl.p_glDrawBuffer(draw_buffers[0]);
        }
    }

    checkGLcall("apply draw buffers");
}

/* Context activation is done by the caller. */
void wined3d_context_gl_active_texture(struct wined3d_context_gl *context_gl,
        const struct wined3d_gl_info *gl_info, unsigned int unit)
{
    GL_EXTCALL(glActiveTexture(GL_TEXTURE0 + unit));
    checkGLcall("glActiveTexture");
    context_gl->active_texture = unit;
}

void wined3d_context_gl_bind_bo(struct wined3d_context_gl *context_gl, GLenum binding, GLuint name)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    if (binding == GL_ELEMENT_ARRAY_BUFFER)
        context_invalidate_state(&context_gl->c, STATE_INDEXBUFFER);

    GL_EXTCALL(glBindBuffer(binding, name));
}

void wined3d_context_gl_bind_texture(struct wined3d_context_gl *context_gl, GLenum target, GLuint name)
{
    const struct wined3d_dummy_textures *textures = &wined3d_device_gl(context_gl->c.device)->dummy_textures;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    GLenum old_texture_type;
    unsigned int unit;

    if (name)
        gl_info->gl_ops.gl.p_glBindTexture(target, name);
    else
        target = GL_NONE;

    unit = context_gl->active_texture;
    old_texture_type = context_gl->texture_type[unit];
    if (old_texture_type != target)
    {
        switch (old_texture_type)
        {
            case GL_NONE:
                /* nothing to do */
                break;
            case GL_TEXTURE_1D:
                gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_1D, textures->tex_1d);
                break;
            case GL_TEXTURE_1D_ARRAY:
                gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_1D_ARRAY, textures->tex_1d_array);
                break;
            case GL_TEXTURE_2D:
                gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D, textures->tex_2d);
                break;
            case GL_TEXTURE_2D_ARRAY:
                gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D_ARRAY, textures->tex_2d_array);
                break;
            case GL_TEXTURE_CUBE_MAP:
                gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_CUBE_MAP, textures->tex_cube);
                break;
            case GL_TEXTURE_CUBE_MAP_ARRAY:
                gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_CUBE_MAP_ARRAY, textures->tex_cube_array);
                break;
            case GL_TEXTURE_3D:
                gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_3D, textures->tex_3d);
                break;
            case GL_TEXTURE_BUFFER:
                gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_BUFFER, textures->tex_buffer);
                break;
            case GL_TEXTURE_2D_MULTISAMPLE:
                gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, textures->tex_2d_ms);
                break;
            case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
                gl_info->gl_ops.gl.p_glBindTexture(GL_TEXTURE_2D_MULTISAMPLE_ARRAY, textures->tex_2d_ms_array);
                break;
            default:
                ERR("Unexpected texture target %#x.\n", old_texture_type);
        }

        context_gl->texture_type[unit] = target;
    }

    checkGLcall("bind texture");
}

static void wined3d_context_gl_poll_fences(struct wined3d_context_gl *context_gl)
{
    struct wined3d_device_gl *device_gl = wined3d_device_gl(context_gl->c.device);
    struct wined3d_command_fence_gl *f;
    SIZE_T i;

    for (i = 0; i < context_gl->submitted.fence_count; ++i)
    {
        f = &context_gl->submitted.fences[i];

        if (f->id > device_gl->completed_fence_id)
        {
            if (wined3d_fence_test(f->fence, &device_gl->d, 0) != WINED3D_FENCE_OK)
                continue;
            device_gl->completed_fence_id = f->id;
        }

        wined3d_fence_destroy(f->fence);
        if (i != context_gl->submitted.fence_count - 1)
            *f = context_gl->submitted.fences[context_gl->submitted.fence_count - 1];
        --context_gl->submitted.fence_count;
    }
}

static void wined3d_device_gl_free_memory(struct wined3d_device_gl *device_gl, struct wined3d_allocator_block *block)
{
    assert(block->chunk->allocator == &device_gl->allocator);
    wined3d_device_gl_allocator_lock(device_gl);
    wined3d_allocator_block_free(block);
    wined3d_device_gl_allocator_unlock(device_gl);
}

static void wined3d_context_gl_cleanup_resources(struct wined3d_context_gl *context_gl)
{
    struct wined3d_device_gl *device_gl = wined3d_device_gl(context_gl->c.device);
    struct wined3d_retired_block_gl *r, *blocks;
    SIZE_T count, i = 0;
    uint64_t id;

    wined3d_context_gl_poll_fences(context_gl);
    id = device_gl->completed_fence_id;

    blocks = device_gl->retired_blocks;
    count = device_gl->retired_block_count;
    while (i < count)
    {
        r = &blocks[i];

        if (r->fence_id > id)
        {
            ++i;
            continue;
        }

        wined3d_device_gl_free_memory(device_gl, r->block);
        if (i != --count)
            *r = blocks[count];
    }
    device_gl->retired_block_count = count;
}

void wined3d_context_gl_wait_command_fence(struct wined3d_context_gl *context_gl, uint64_t id)
{
    struct wined3d_device_gl *device_gl = wined3d_device_gl(context_gl->c.device);
    enum wined3d_fence_result ret;
    SIZE_T i;

    if (id <= device_gl->completed_fence_id
            || id > device_gl->current_fence_id) /* In case the fence ID wrapped. */
        return;

    for (i = 0; i < context_gl->submitted.fence_count; ++i)
    {
        if (context_gl->submitted.fences[i].id != id)
            continue;

        if ((ret = wined3d_fence_wait(context_gl->submitted.fences[i].fence, &device_gl->d)) != WINED3D_FENCE_OK)
            ERR("Failed to wait for command fence with id 0x%s, ret %#x.\n", wine_dbgstr_longlong(id), ret);
        wined3d_context_gl_cleanup_resources(context_gl);
        return;
    }

    ERR("Failed to find fence for command fence with id 0x%s.\n", wine_dbgstr_longlong(id));
}

void wined3d_context_gl_submit_command_fence(struct wined3d_context_gl *context_gl)
{
    struct wined3d_device_gl *device_gl = wined3d_device_gl(context_gl->c.device);
    struct wined3d_command_fence_gl *f;
    HRESULT hr;

    if (!wined3d_array_reserve((void **)&context_gl->submitted.fences, &context_gl->submitted.fences_size,
            context_gl->submitted.fence_count + 1, sizeof(*context_gl->submitted.fences)))
        ERR("Failed to grow submitted command buffer array.\n");

    f = &context_gl->submitted.fences[context_gl->submitted.fence_count++];
    f->id = device_gl->current_fence_id;
    if (FAILED(hr = wined3d_fence_create(&device_gl->d, &f->fence)))
        ERR("Failed to create fence, hr %#lx.\n", hr);
    wined3d_fence_issue(f->fence, &device_gl->d);

    /* We don't expect this to ever happen, but handle it anyway. */
    if (!++device_gl->current_fence_id)
    {
        wined3d_context_gl_wait_command_fence(context_gl, device_gl->current_fence_id - 1);
        device_gl->completed_fence_id = 0;
        device_gl->current_fence_id = 1;
    }
    device_gl->retired_bo_size = 0;
    wined3d_context_gl_cleanup_resources(context_gl);
}

static void wined3d_context_gl_destroy_allocator_block(struct wined3d_context_gl *context_gl,
        struct wined3d_allocator_block *block, uint64_t fence_id)
{
    struct wined3d_device_gl *device_gl = wined3d_device_gl(context_gl->c.device);
    struct wined3d_retired_block_gl *r;

    if (device_gl->completed_fence_id >= fence_id)
    {
        wined3d_device_gl_free_memory(device_gl, block);
        TRACE("Freed block %p.\n", block);
        return;
    }

    if (!wined3d_array_reserve((void **)&device_gl->retired_blocks,
            &device_gl->retired_blocks_size, device_gl->retired_block_count + 1,
            sizeof(*device_gl->retired_blocks)))
    {
        ERR("Leaking block %p.\n", block);
        return;
    }

    r = &device_gl->retired_blocks[device_gl->retired_block_count++];
    r->block = block;
    r->fence_id = fence_id;
}

/* We always have buffer storage here. */
GLuint wined3d_context_gl_allocate_vram_chunk_buffer(struct wined3d_context_gl *context_gl,
        unsigned int pool, size_t size)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    GLbitfield flags;
    GLuint id = 0;

    TRACE("context_gl %p, pool %u, size %Iu.\n", context_gl, pool, size);

    GL_EXTCALL(glGenBuffers(1, &id));
    if (!id)
    {
        checkGLcall("buffer object creation");
        return 0;
    }
    wined3d_context_gl_bind_bo(context_gl, GL_PIXEL_UNPACK_BUFFER, id);

    flags = wined3d_device_gl_get_memory_type_flags(pool) | GL_DYNAMIC_STORAGE_BIT;
    if (flags & (GL_MAP_READ_BIT | GL_MAP_WRITE_BIT))
        flags |= GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    GL_EXTCALL(glBufferStorage(GL_PIXEL_UNPACK_BUFFER, size, NULL, flags));

    checkGLcall("buffer object creation");

    TRACE("Created buffer object %u.\n", id);

    return id;
}

static void *wined3d_allocator_chunk_gl_map(struct wined3d_allocator_chunk_gl *chunk_gl,
        struct wined3d_context_gl *context_gl)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    void *map_ptr;

    TRACE("chunk %p, gl_buffer %u, map_ptr %p.\n", chunk_gl, chunk_gl->gl_buffer, chunk_gl->c.map_ptr);

    wined3d_allocator_chunk_gl_lock(chunk_gl);

    if (!chunk_gl->c.map_ptr)
    {
        unsigned int flags = wined3d_device_gl_get_memory_type_flags(chunk_gl->memory_type) & ~GL_CLIENT_STORAGE_BIT;

        flags |= GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
        if (!(flags & GL_MAP_READ_BIT))
            flags |= GL_MAP_UNSYNCHRONIZED_BIT;
        if (flags & GL_MAP_WRITE_BIT)
            flags |= GL_MAP_FLUSH_EXPLICIT_BIT;
        wined3d_context_gl_bind_bo(context_gl, GL_PIXEL_UNPACK_BUFFER, chunk_gl->gl_buffer);
        chunk_gl->c.map_ptr = GL_EXTCALL(glMapBufferRange(GL_PIXEL_UNPACK_BUFFER,
                0, WINED3D_ALLOCATOR_CHUNK_SIZE, flags));
        if (!chunk_gl->c.map_ptr)
        {
            wined3d_allocator_chunk_gl_unlock(chunk_gl);
            ERR("Failed to map chunk memory.\n");
            return NULL;
        }

        adapter_adjust_mapped_memory(context_gl->c.device->adapter, WINED3D_ALLOCATOR_CHUNK_SIZE);
    }

    ++chunk_gl->c.map_count;
    map_ptr = chunk_gl->c.map_ptr;

    wined3d_allocator_chunk_gl_unlock(chunk_gl);

    return map_ptr;
}

static void wined3d_allocator_chunk_gl_unmap(struct wined3d_allocator_chunk_gl *chunk_gl,
        struct wined3d_context_gl *context_gl)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    TRACE("chunk_gl %p, context_gl %p.\n", chunk_gl, context_gl);

    wined3d_allocator_chunk_gl_lock(chunk_gl);

    if (!--chunk_gl->c.map_count)
    {
        wined3d_context_gl_bind_bo(context_gl, GL_PIXEL_UNPACK_BUFFER, chunk_gl->gl_buffer);
        GL_EXTCALL(glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER));
        chunk_gl->c.map_ptr = NULL;

        adapter_adjust_mapped_memory(context_gl->c.device->adapter, -WINED3D_ALLOCATOR_CHUNK_SIZE);
    }

    wined3d_allocator_chunk_gl_unlock(chunk_gl);
}

static void *wined3d_bo_gl_map(struct wined3d_bo_gl *bo, struct wined3d_context_gl *context_gl, uint32_t flags)
{
    struct wined3d_device_gl *device_gl = wined3d_device_gl(context_gl->c.device);
    const struct wined3d_gl_info *gl_info;
    struct wined3d_bo_user *bo_user;
    struct wined3d_bo_gl tmp;

    if (flags & WINED3D_MAP_NOOVERWRITE)
        goto map;

    if ((flags & WINED3D_MAP_DISCARD) && bo->command_fence_id > device_gl->completed_fence_id)
    {
        if (wined3d_device_gl_create_bo(device_gl, context_gl, bo->size,
                bo->binding, bo->usage, bo->b.coherent, bo->flags, &tmp))
        {
            LIST_FOR_EACH_ENTRY(bo_user, &bo->b.users, struct wined3d_bo_user, entry)
                bo_user->valid = false;
            list_init(&bo->b.users);

            wined3d_context_gl_destroy_bo(context_gl, bo);
            *bo = tmp;
            list_init(&bo->b.users);

            goto map;
        }

        ERR("Failed to create new buffer object.\n");
    }

    if (context_gl->c.d3d_info->fences)
    {
        if (bo->command_fence_id == device_gl->current_fence_id)
            wined3d_context_gl_submit_command_fence(context_gl);
        wined3d_context_gl_wait_command_fence(context_gl, bo->command_fence_id);
    }

map:
    if (bo->b.map_ptr)
        return bo->b.map_ptr;

    if (bo->memory)
    {
        struct wined3d_allocator_chunk_gl *chunk_gl = wined3d_allocator_chunk_gl(bo->memory->chunk);

        if (!(bo->b.map_ptr = wined3d_allocator_chunk_gl_map(chunk_gl, context_gl)))
            ERR("Failed to map chunk.\n");
        return bo->b.map_ptr;
    }

    gl_info = context_gl->gl_info;
    wined3d_context_gl_bind_bo(context_gl, bo->binding, bo->id);

    if (gl_info->supported[ARB_BUFFER_STORAGE])
    {
        GLbitfield gl_flags;

        /* When mapping the bo persistently, we need to use the access flags
         * used to create the bo, instead of the access flags passed to the
         * map call. Otherwise, if for example the initial map call that
         * caused the bo to be persistently mapped was a read-only map,
         * subsequent write access to the bo would be undefined.
         *
         * Note that we use GL_MAP_PERSISTENT_BIT for non-persistent maps here
         * as well, in order to allow draws to succeed while referenced buffer
         * resources are mapped. On the other hand, we don't want to use the
         * access flags used to create the bo for non-persistent maps, because
         * that may imply dropping GL_MAP_UNSYNCHRONIZED_BIT. */
        gl_flags = bo->flags & ~GL_CLIENT_STORAGE_BIT;
        if (!(gl_flags & GL_MAP_READ_BIT))
            gl_flags |= GL_MAP_UNSYNCHRONIZED_BIT;
        if (gl_flags & GL_MAP_WRITE_BIT)
            gl_flags |= GL_MAP_FLUSH_EXPLICIT_BIT;
        gl_flags |= GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;

        bo->b.map_ptr = GL_EXTCALL(glMapBufferRange(bo->binding, 0, bo->size, gl_flags));
    }
    else if (gl_info->supported[ARB_MAP_BUFFER_RANGE])
    {
        bo->b.map_ptr = GL_EXTCALL(glMapBufferRange(bo->binding, 0, bo->size, wined3d_resource_gl_map_flags(bo, flags)));
    }
    else
    {
        bo->b.map_ptr = GL_EXTCALL(glMapBuffer(bo->binding, wined3d_resource_gl_legacy_map_flags(flags)));
    }

    if (bo->b.map_ptr)
        adapter_adjust_mapped_memory(device_gl->d.adapter, bo->size);

    wined3d_context_gl_bind_bo(context_gl, bo->binding, 0);
    checkGLcall("Map buffer object");

    return bo->b.map_ptr;
}

static void wined3d_bo_gl_unmap(struct wined3d_bo_gl *bo, struct wined3d_context_gl *context_gl)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    if (context_gl->c.d3d_info->persistent_map
            && context_gl->c.device->adapter->mapped_size <= MAX_PERSISTENT_MAPPED_BYTES)
    {
        TRACE("Not unmapping BO %p.\n", bo);
        return;
    }

    wined3d_device_bo_map_lock(context_gl->c.device);
    /* The mapping is still in use by the client (viz. for an accelerated
     * NOOVERWRITE map). The client will trigger another unmap request when the
     * d3d application requests to unmap the BO. */
    if (bo->b.client_map_count)
    {
        wined3d_device_bo_map_unlock(context_gl->c.device);
        assert(context_gl->c.d3d_info->persistent_map);
        TRACE("BO %p is still in use by a client thread; not unmapping.\n", bo);
        return;
    }
    bo->b.map_ptr = NULL;
    wined3d_device_bo_map_unlock(context_gl->c.device);

    if (bo->memory)
    {
        struct wined3d_allocator_chunk_gl *chunk_gl = wined3d_allocator_chunk_gl(bo->memory->chunk);

        wined3d_allocator_chunk_gl_unmap(chunk_gl, context_gl);
        return;
    }

    wined3d_context_gl_bind_bo(context_gl, bo->binding, bo->id);
    GL_EXTCALL(glUnmapBuffer(bo->binding));
    wined3d_context_gl_bind_bo(context_gl, bo->binding, 0);
    checkGLcall("Unmap buffer object");

    adapter_adjust_mapped_memory(context_gl->c.device->adapter, -bo->size);
}

void *wined3d_context_gl_map_bo_address(struct wined3d_context_gl *context_gl,
        const struct wined3d_bo_address *data, size_t size, uint32_t flags)
{
    struct wined3d_bo *bo;
    void *map_ptr;

    if (!(bo = data->buffer_object))
        return data->addr;

    if (!(map_ptr = wined3d_bo_gl_map(wined3d_bo_gl(bo), context_gl, flags)))
    {
        ERR("Failed to map bo.\n");
        return NULL;
    }

    return (uint8_t *)map_ptr + bo->buffer_offset + (uintptr_t)data->addr;
}

static void flush_bo_ranges(struct wined3d_context_gl *context_gl, const struct wined3d_const_bo_address *data,
        unsigned int range_count, const struct wined3d_range *ranges)
{
    const struct wined3d_gl_info *gl_info;
    struct wined3d_bo_gl *bo;
    unsigned int i;

    if (!data->buffer_object || data->buffer_object->coherent)
        return;
    bo = wined3d_bo_gl(data->buffer_object);

    gl_info = context_gl->gl_info;
    wined3d_context_gl_bind_bo(context_gl, bo->binding, bo->id);

    if (gl_info->supported[ARB_MAP_BUFFER_RANGE])
    {
        /* The offset passed to glFlushMappedBufferRange() is relative to the
         * mapped range, but we map the whole buffer anyway. */
        for (i = 0; i < range_count; ++i)
        {
            GL_EXTCALL(glFlushMappedBufferRange(bo->binding,
                    bo->b.buffer_offset + (uintptr_t)data->addr + ranges[i].offset, ranges[i].size));
        }
    }
    else if (gl_info->supported[APPLE_FLUSH_BUFFER_RANGE])
    {
        for (i = 0; i < range_count; ++i)
        {
            GL_EXTCALL(glFlushMappedBufferRangeAPPLE(bo->binding,
                    bo->b.buffer_offset + (uintptr_t)data->addr + ranges[i].offset, ranges[i].size));
            checkGLcall("glFlushMappedBufferRangeAPPLE");
        }
    }

    wined3d_context_gl_bind_bo(context_gl, bo->binding, 0);
    checkGLcall("Flush buffer object");
}

void wined3d_context_gl_unmap_bo_address(struct wined3d_context_gl *context_gl,
        const struct wined3d_bo_address *data, unsigned int range_count, const struct wined3d_range *ranges)
{
    struct wined3d_bo_gl *bo;

    if (!data->buffer_object)
        return;
    bo = wined3d_bo_gl(data->buffer_object);

    assert(bo->b.map_ptr);

    flush_bo_ranges(context_gl, wined3d_const_bo_address(data), range_count, ranges);
    wined3d_bo_gl_unmap(bo, context_gl);
}

void wined3d_context_gl_flush_bo_address(struct wined3d_context_gl *context_gl,
        const struct wined3d_const_bo_address *data, size_t size)
{
    struct wined3d_range range;

    TRACE("context_gl %p, data %s, size %Iu.\n", context_gl, debug_const_bo_address(data), size);

    range.offset = (uintptr_t)data->addr;
    range.size = size;

    flush_bo_ranges(context_gl, data, 1, &range);
}

void wined3d_context_gl_copy_bo_address(struct wined3d_context_gl *context_gl,
        const struct wined3d_bo_address *dst, const struct wined3d_bo_address *src,
        unsigned int range_count, const struct wined3d_range *ranges, uint32_t map_flags)
{
    const struct wined3d_gl_info *gl_info;
    struct wined3d_bo_gl *src_bo, *dst_bo;
    BYTE *dst_ptr, *src_ptr;
    unsigned int i;

    gl_info = context_gl->gl_info;
    src_bo = src->buffer_object ? wined3d_bo_gl(src->buffer_object) : NULL;
    dst_bo = dst->buffer_object ? wined3d_bo_gl(dst->buffer_object) : NULL;

    if (dst_bo && src_bo)
    {
        if (gl_info->supported[ARB_COPY_BUFFER])
        {
            GL_EXTCALL(glBindBuffer(GL_COPY_READ_BUFFER, src_bo->id));
            GL_EXTCALL(glBindBuffer(GL_COPY_WRITE_BUFFER, dst_bo->id));

            for (i = 0; i < range_count; ++i)
                GL_EXTCALL(glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
                        src_bo->b.buffer_offset + (GLintptr)src->addr + ranges[i].offset,
                        dst_bo->b.buffer_offset + (GLintptr)dst->addr + ranges[i].offset, ranges[i].size));
            checkGLcall("direct buffer copy");

            wined3d_context_gl_reference_bo(context_gl, src_bo);
            wined3d_context_gl_reference_bo(context_gl, dst_bo);
        }
        else
        {
            src_ptr = wined3d_context_gl_map_bo_address(context_gl, src,
                    src_bo->size - (uintptr_t)src->addr, WINED3D_MAP_READ);
            dst_ptr = wined3d_context_gl_map_bo_address(context_gl, dst,
                    dst_bo->size - (uintptr_t)dst->addr, map_flags);

            for (i = 0; i < range_count; ++i)
                memcpy(dst_ptr + ranges[i].offset, src_ptr + ranges[i].offset, ranges[i].size);

            wined3d_context_gl_unmap_bo_address(context_gl, dst, range_count, ranges);
            wined3d_context_gl_unmap_bo_address(context_gl, src, 0, NULL);
        }
    }
    else if (!dst_bo && src_bo)
    {
        wined3d_context_gl_bind_bo(context_gl, src_bo->binding, src_bo->id);
        for (i = 0; i < range_count; ++i)
            GL_EXTCALL(glGetBufferSubData(src_bo->binding,
                    src_bo->b.buffer_offset + (GLintptr)src->addr + ranges[i].offset,
                    ranges[i].size, dst->addr + ranges[i].offset));
        checkGLcall("buffer download");

        wined3d_context_gl_reference_bo(context_gl, src_bo);
    }
    else if (dst_bo && !src_bo)
    {
        if ((map_flags & WINED3D_MAP_DISCARD) && (dst_bo->flags & GL_MAP_WRITE_BIT))
        {
            dst_ptr = wined3d_context_gl_map_bo_address(context_gl, dst, dst_bo->size, map_flags);
            memcpy(dst_ptr, src->addr, dst_bo->size);
            wined3d_context_gl_unmap_bo_address(context_gl, dst, range_count, ranges);
        }
        else
        {
            wined3d_context_gl_bind_bo(context_gl, dst_bo->binding, dst_bo->id);
            for (i = 0; i < range_count; ++i)
                GL_EXTCALL(glBufferSubData(dst_bo->binding,
                        dst_bo->b.buffer_offset + (GLintptr)dst->addr + ranges[i].offset,
                        ranges[i].size, src->addr + ranges[i].offset));
            checkGLcall("buffer upload");

            wined3d_context_gl_reference_bo(context_gl, dst_bo);
        }
    }
    else
    {
        for (i = 0; i < range_count; ++i)
            memcpy(dst->addr + ranges[i].offset, src->addr + ranges[i].offset, ranges[i].size);
    }
}

void wined3d_context_gl_destroy_bo(struct wined3d_context_gl *context_gl, struct wined3d_bo_gl *bo)
{
    struct wined3d_device_gl *device_gl = wined3d_device_gl(context_gl->c.device);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    TRACE("context_gl %p, bo %p.\n", context_gl, bo);

    assert(list_empty(&bo->b.users));

    if (bo->memory)
    {
        unsigned int order = bo->memory->order;

        if (bo->b.map_ptr)
            wined3d_allocator_chunk_gl_unmap(wined3d_allocator_chunk_gl(bo->memory->chunk), context_gl);
        wined3d_context_gl_destroy_allocator_block(context_gl, bo->memory, bo->command_fence_id);

        if (bo->command_fence_id == device_gl->current_fence_id)
        {
            device_gl->retired_bo_size += WINED3D_ALLOCATOR_CHUNK_SIZE >> order;
            if (device_gl->retired_bo_size > WINED3D_RETIRED_BO_SIZE_THRESHOLD)
                wined3d_context_gl_submit_command_fence(context_gl);
        }

        bo->id = 0;
        return;
    }

    if (bo->b.map_ptr)
    {
        wined3d_context_gl_bind_bo(context_gl, bo->binding, bo->id);
        GL_EXTCALL(glUnmapBuffer(bo->binding));
        adapter_adjust_mapped_memory(context_gl->c.device->adapter, -bo->size);
    }

    TRACE("Destroying GL buffer %u.\n", bo->id);

    GL_EXTCALL(glDeleteBuffers(1, &bo->id));
    checkGLcall("buffer object destruction");
    bo->id = 0;
}

GLenum wined3d_context_gl_get_offscreen_gl_buffer(const struct wined3d_context_gl *context_gl)
{
    return GL_COLOR_ATTACHMENT0;
}

static uint32_t wined3d_context_gl_generate_rt_mask_no_fbo(const struct wined3d_context_gl *context_gl,
        struct wined3d_resource *rt)
{
    if (!rt || rt->format->id == WINED3DFMT_NULL)
        return 0;
    else if (rt->type != WINED3D_RTYPE_BUFFER && texture_from_resource(rt)->swapchain)
        return context_generate_rt_mask_from_resource(rt);
    else
        return context_generate_rt_mask(wined3d_context_gl_get_offscreen_gl_buffer(context_gl));
}

/* Context activation is done by the caller. */
void wined3d_context_gl_apply_blit_state(struct wined3d_context_gl *context_gl, const struct wined3d_device *device)
{
    struct wined3d_context *context = &context_gl->c;
    const struct wined3d_gl_info *gl_info;
    SIZE rt_size;

    TRACE("Setting up context %p for blitting.\n", context);

    gl_info = context_gl->gl_info;

    wined3d_context_gl_get_rt_size(context_gl, &rt_size);

    if (context->last_was_blit)
    {
        if (context_gl->blit_size.cx != rt_size.cx || context_gl->blit_size.cy != rt_size.cy)
        {
            gl_info->gl_ops.gl.p_glViewport(0, 0, rt_size.cx, rt_size.cy);
            context->viewport_count = WINED3D_MAX_VIEWPORTS;
            context_gl->blit_size = rt_size;
            /* No need to dirtify here, the states are still dirtified because
             * they weren't applied since the last context_apply_blit_state()
             * call. */
        }
        checkGLcall("blit state application");
        TRACE("Context is already set up for blitting, nothing to do.\n");
        return;
    }
    context->last_was_blit = TRUE;

    if (gl_info->supported[ARB_SAMPLER_OBJECTS])
        GL_EXTCALL(glBindSampler(0, 0));
    wined3d_context_gl_active_texture(context_gl, gl_info, 0);

    context_invalidate_compute_state(context, STATE_COMPUTE_SHADER_RESOURCE_BINDING);
    context_invalidate_state(context, STATE_GRAPHICS_SHADER_RESOURCE_BINDING);

    if (gl_info->supported[WINED3D_GL_LEGACY_CONTEXT])
    {
        gl_info->gl_ops.gl.p_glDisable(GL_ALPHA_TEST);
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_ALPHATESTENABLE));
    }
    gl_info->gl_ops.gl.p_glDisable(GL_BLEND);
    gl_info->gl_ops.gl.p_glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    context_invalidate_state(context, STATE_BLEND);
    gl_info->gl_ops.gl.p_glDisable(GL_CULL_FACE);
    gl_info->gl_ops.gl.p_glDisable(GL_SCISSOR_TEST);
    context_invalidate_state(context, STATE_RASTERIZER);
    gl_info->gl_ops.gl.p_glDisable(GL_DEPTH_TEST);
    gl_info->gl_ops.gl.p_glDisable(GL_STENCIL_TEST);
    context_invalidate_state(context, STATE_DEPTH_STENCIL);
    if (gl_info->supported[ARB_FRAMEBUFFER_SRGB])
    {
        gl_info->gl_ops.gl.p_glDisable(GL_FRAMEBUFFER_SRGB);
        context_invalidate_state(context, STATE_RENDER(WINED3D_RS_SRGBWRITEENABLE));
    }

    context->last_was_rhw = TRUE;
    context_invalidate_state(context, STATE_VDECL); /* because of last_was_rhw = TRUE */

    wined3d_context_gl_enable_clip_distances(context_gl, 0);
    context_invalidate_state(context, STATE_RENDER(WINED3D_RS_CLIPPING));

    /* FIXME: Make draw_textured_quad() able to work with a upper left origin. */
    if (gl_info->supported[ARB_CLIP_CONTROL])
        GL_EXTCALL(glClipControl(GL_LOWER_LEFT, GL_NEGATIVE_ONE_TO_ONE));
    gl_info->gl_ops.gl.p_glViewport(0, 0, rt_size.cx, rt_size.cy);
    context->viewport_count = WINED3D_MAX_VIEWPORTS;
    context_invalidate_state(context, STATE_VIEWPORT);

    device->shader_backend->shader_disable(device->shader_priv, context);

    context_gl->blit_size = rt_size;

    checkGLcall("blit state application");
}

static BOOL have_framebuffer_attachment(unsigned int rt_count, struct wined3d_rendertarget_view * const *rts,
        const struct wined3d_rendertarget_view *ds)
{
    unsigned int i;

    if (ds)
        return TRUE;

    for (i = 0; i < rt_count; ++i)
    {
        if (rts[i] && rts[i]->format->id != WINED3DFMT_NULL)
            return TRUE;
    }

    return FALSE;
}

/* Context activation is done by the caller. */
BOOL wined3d_context_gl_apply_clear_state(struct wined3d_context_gl *context_gl,
        const struct wined3d_state *state, unsigned int rt_count, const struct wined3d_fb_state *fb)
{
    struct wined3d_rendertarget_view * const *rts = fb->render_targets;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_rendertarget_view *dsv = fb->depth_stencil;
    uint32_t rt_mask = 0, *cur_mask;
    unsigned int i;

    if (isStateDirty(&context_gl->c, STATE_FRAMEBUFFER) || fb != &state->fb
            || rt_count != gl_info->limits.buffers)
    {
        struct wined3d_rendertarget_info ds_info = {{0}};

        if (!have_framebuffer_attachment(rt_count, rts, dsv))
        {
            WARN("Invalid render target config, need at least one attachment.\n");
            return FALSE;
        }

        if (!rt_count || wined3d_resource_is_offscreen(rts[0]->resource))
        {
            memset(context_gl->blit_targets, 0, sizeof(context_gl->blit_targets));
            for (i = 0; i < rt_count; ++i)
            {
                if (rts[i])
                {
                    struct wined3d_rendertarget_view_gl *rtv_gl = wined3d_rendertarget_view_gl(rts[i]);
                    context_gl->blit_targets[i].gl_view = rtv_gl->gl_view;
                    context_gl->blit_targets[i].resource = rtv_gl->v.resource;
                    context_gl->blit_targets[i].sub_resource_idx = rtv_gl->v.sub_resource_idx;
                    context_gl->blit_targets[i].layer_count = rtv_gl->v.layer_count;
                }
                if (rts[i] && rts[i]->format->id != WINED3DFMT_NULL)
                    rt_mask |= (1u << i);
            }

            if (dsv)
            {
                struct wined3d_rendertarget_view_gl *dsv_gl = wined3d_rendertarget_view_gl(dsv);
                ds_info.gl_view = dsv_gl->gl_view;
                ds_info.resource = dsv_gl->v.resource;
                ds_info.sub_resource_idx = dsv_gl->v.sub_resource_idx;
                ds_info.layer_count = dsv_gl->v.layer_count;
            }

            wined3d_context_gl_apply_fbo_state(context_gl, GL_FRAMEBUFFER, context_gl->blit_targets, &ds_info,
                    rt_count ? rts[0]->resource->draw_binding : 0, dsv ? dsv->resource->draw_binding : 0);
        }
        else
        {
            wined3d_context_gl_apply_fbo_state(context_gl, GL_FRAMEBUFFER, NULL, &ds_info,
                    WINED3D_LOCATION_DRAWABLE, WINED3D_LOCATION_DRAWABLE);
            rt_mask = context_generate_rt_mask_from_resource(rts[0]->resource);
        }

        /* If the framebuffer is not the device's fb the device's fb has to be reapplied
         * next draw. Otherwise we could mark the framebuffer state clean here, once the
         * state management allows this */
        context_invalidate_state(&context_gl->c, STATE_FRAMEBUFFER);
    }
    else if (!rt_count || wined3d_resource_is_offscreen(rts[0]->resource))
    {
        for (i = 0; i < rt_count; ++i)
        {
            if (rts[i] && rts[i]->format->id != WINED3DFMT_NULL)
                rt_mask |= (1u << i);
        }
    }
    else
    {
        rt_mask = wined3d_context_gl_generate_rt_mask_no_fbo(context_gl, rt_count ? rts[0]->resource : NULL);
    }

    cur_mask = context_gl->current_fbo ? &context_gl->current_fbo->rt_mask : &context_gl->draw_buffers_mask;

    if (rt_mask != *cur_mask)
    {
        wined3d_context_gl_apply_draw_buffers(context_gl, rt_mask);
        *cur_mask = rt_mask;
        context_invalidate_state(&context_gl->c, STATE_FRAMEBUFFER);
    }

    wined3d_context_gl_check_fbo_status(context_gl, GL_FRAMEBUFFER);

    context_gl->c.last_was_blit = FALSE;
    context_gl->c.last_was_ffp_blit = FALSE;

    /* Blending and clearing should be orthogonal, but tests on the nvidia
     * driver show that disabling blending when clearing improves the clearing
     * performance incredibly. */
    gl_info->gl_ops.gl.p_glDisable(GL_BLEND);
    gl_info->gl_ops.gl.p_glEnable(GL_SCISSOR_TEST);
    if (rt_count && gl_info->supported[ARB_FRAMEBUFFER_SRGB])
    {
        if (needs_srgb_write(context_gl->c.d3d_info, state, fb))
            gl_info->gl_ops.gl.p_glEnable(GL_FRAMEBUFFER_SRGB);
        else
            gl_info->gl_ops.gl.p_glDisable(GL_FRAMEBUFFER_SRGB);
        context_invalidate_state(&context_gl->c, STATE_RENDER(WINED3D_RS_SRGBWRITEENABLE));
    }
    checkGLcall("setting up state for clear");

    context_invalidate_state(&context_gl->c, STATE_BLEND);
    context_invalidate_state(&context_gl->c, STATE_RASTERIZER);
    context_invalidate_state(&context_gl->c, STATE_SCISSORRECT);

    return TRUE;
}

static uint32_t find_draw_buffers_mask(const struct wined3d_context_gl *context_gl, const struct wined3d_state *state)
{
    struct wined3d_rendertarget_view * const *rts = state->fb.render_targets;
    struct wined3d_shader *ps = state->shader[WINED3D_SHADER_TYPE_PIXEL];
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    unsigned int rt_mask, mask;
    unsigned int i;

    rt_mask = ps ? ps->reg_maps.rt_mask : 1;
    rt_mask &= wined3d_mask_from_size(gl_info->limits.buffers);
    if (state->blend_state && state->blend_state->dual_source)
        rt_mask = 1;

    mask = rt_mask;
    while (mask)
    {
        i = wined3d_bit_scan(&mask);
        if (!rts[i] || rts[i]->format->id == WINED3DFMT_NULL)
            rt_mask &= ~(1u << i);
    }

    return rt_mask;
}

void context_gl_apply_texture_draw_state(struct wined3d_context_gl *context_gl,
        struct wined3d_texture *texture, unsigned int sub_resource_idx, unsigned int location)
{
    const struct wined3d_format *format = texture->resource.format;
    struct wined3d_context *context = &context_gl->c;
    uint32_t rt_mask, *cur_mask;

    TRACE("context_gl %p, texture %p, sub_resource_idx %u, location %s.\n",
            context_gl, texture, sub_resource_idx, wined3d_debug_location(location));

    if (format->depth_size || format->stencil_size)
        wined3d_context_gl_apply_fbo_state_explicit(context_gl, GL_DRAW_FRAMEBUFFER, NULL,
                0, &texture->resource, sub_resource_idx, location);
    else
        wined3d_context_gl_apply_fbo_state_explicit(context_gl, GL_DRAW_FRAMEBUFFER, &texture->resource,
                sub_resource_idx, NULL, 0, location);

    if (location != WINED3D_LOCATION_DRAWABLE)
    {
        if (texture->resource.format->id == WINED3DFMT_NULL || format->depth_size || format->stencil_size)
            rt_mask = 0;
        else
            rt_mask = 1;
    }
    else
    {
        rt_mask = context_generate_rt_mask_from_resource(&texture->resource);
    }

    cur_mask = context_gl->current_fbo ? &context_gl->current_fbo->rt_mask : &context_gl->draw_buffers_mask;
    if (rt_mask != *cur_mask)
    {
        wined3d_context_gl_apply_draw_buffers(context_gl, rt_mask);
        *cur_mask = rt_mask;
    }

    wined3d_context_gl_check_fbo_status(context_gl, GL_DRAW_FRAMEBUFFER);

    context_invalidate_state(context, STATE_FRAMEBUFFER);
}

/* Context activation is done by the caller. */
void context_state_fb(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    uint32_t rt_mask = find_draw_buffers_mask(context_gl, state);
    const struct wined3d_rendertarget_view_gl *view_gl;
    struct wined3d_rendertarget_info ds_info = {{0}};
    const struct wined3d_fb_state *fb = &state->fb;
    DWORD color_location = 0;
    uint32_t *cur_mask;

    memset(context_gl->blit_targets, 0, sizeof(context_gl->blit_targets));
    for (unsigned int i = 0; i < context_gl->gl_info->limits.buffers; ++i)
    {
        if (!fb->render_targets[i])
            continue;

        view_gl = wined3d_rendertarget_view_gl(fb->render_targets[i]);
        context_gl->blit_targets[i].gl_view = view_gl->gl_view;
        context_gl->blit_targets[i].resource = view_gl->v.resource;
        context_gl->blit_targets[i].sub_resource_idx = view_gl->v.sub_resource_idx;
        context_gl->blit_targets[i].layer_count = view_gl->v.layer_count;

        if (!color_location)
            color_location = view_gl->v.resource->draw_binding;
    }

    if (fb->depth_stencil)
    {
        view_gl = wined3d_rendertarget_view_gl(fb->depth_stencil);
        ds_info.gl_view = view_gl->gl_view;
        ds_info.resource = view_gl->v.resource;
        ds_info.sub_resource_idx = view_gl->v.sub_resource_idx;
        ds_info.layer_count = view_gl->v.layer_count;
    }

    wined3d_context_gl_apply_fbo_state(context_gl, GL_FRAMEBUFFER, context_gl->blit_targets, &ds_info,
            color_location, fb->depth_stencil ? fb->depth_stencil->resource->draw_binding : 0);

    cur_mask = context_gl->current_fbo ? &context_gl->current_fbo->rt_mask : &context_gl->draw_buffers_mask;
    if (rt_mask != *cur_mask)
    {
        wined3d_context_gl_apply_draw_buffers(context_gl, rt_mask);
        *cur_mask = rt_mask;
    }
}

static void wined3d_context_gl_map_stage(struct wined3d_context_gl *context_gl, unsigned int stage, unsigned int unit)
{
    unsigned int i = context_gl->rev_tex_unit_map[unit];
    unsigned int j = context_gl->tex_unit_map[stage];

    TRACE("Mapping stage %u to unit %u.\n", stage, unit);
    context_gl->tex_unit_map[stage] = unit;
    if (i != WINED3D_UNMAPPED_STAGE && i != stage)
        context_gl->tex_unit_map[i] = WINED3D_UNMAPPED_STAGE;

    context_gl->rev_tex_unit_map[unit] = stage;
    if (j != WINED3D_UNMAPPED_STAGE && j != unit)
        context_gl->rev_tex_unit_map[j] = WINED3D_UNMAPPED_STAGE;
}

static void context_invalidate_texture_stage(struct wined3d_context *context, DWORD stage)
{
    DWORD i;

    for (i = 0; i <= WINED3D_HIGHEST_TEXTURE_STATE; ++i)
        context_invalidate_state(context, STATE_TEXTURESTAGE(stage, i));
}

static bool use_ffp_ps(const struct wined3d_state *state)
{
    struct wined3d_shader *vs = state->shader[WINED3D_SHADER_TYPE_VERTEX];

    if (!use_vs(state))
        return true;
    if (vs && vs->reg_maps.shader_version.major >= 4)
        return false;
    return true;
}

static void context_update_fixed_function_usage_map(struct wined3d_context *context,
        const struct wined3d_state *state)
{
    UINT i = 0, start, end;

    context->fixed_function_usage_map = 0;

    if (use_ffp_ps(state))
    {
        for (i = 0; i < WINED3D_MAX_FFP_TEXTURES; ++i)
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
                    && i < WINED3D_MAX_FFP_TEXTURES - 1)
                context->fixed_function_usage_map |= (1u << (i + 1));
        }
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

static void wined3d_context_gl_map_fixed_function_samplers(struct wined3d_context_gl *context_gl,
        const struct wined3d_state *state)
{
    const struct wined3d_d3d_info *d3d_info = context_gl->c.d3d_info;
    unsigned int i, tex;
    uint32_t ffu_map;

    ffu_map = context_gl->c.fixed_function_usage_map;

    if (d3d_info->ffp_fragment_caps.max_textures == d3d_info->ffp_fragment_caps.max_blend_stages
            || context_gl->c.lowest_disabled_stage <= d3d_info->ffp_fragment_caps.max_textures)
    {
        while (ffu_map)
        {
            i = wined3d_bit_scan(&ffu_map);
            if (context_gl->tex_unit_map[i] != i)
            {
                wined3d_context_gl_map_stage(context_gl, i, i);
                context_invalidate_state(&context_gl->c, STATE_GRAPHICS_SHADER_RESOURCE_BINDING);
                context_invalidate_texture_stage(&context_gl->c, i);
            }
        }
        return;
    }

    /* Now work out the mapping */
    tex = 0;
    while (ffu_map)
    {
        i = wined3d_bit_scan(&ffu_map);
        if (context_gl->tex_unit_map[i] != tex)
        {
            wined3d_context_gl_map_stage(context_gl, i, tex);
            context_invalidate_state(&context_gl->c, STATE_GRAPHICS_SHADER_RESOURCE_BINDING);
            context_invalidate_texture_stage(&context_gl->c, i);
        }

        ++tex;
    }
}

static void wined3d_context_gl_map_psamplers(struct wined3d_context_gl *context_gl, const struct wined3d_state *state)
{
    const struct wined3d_d3d_info *d3d_info = context_gl->c.d3d_info;
    const struct wined3d_shader_resource_info *resource_info =
            state->shader[WINED3D_SHADER_TYPE_PIXEL]->reg_maps.resource_info;
    unsigned int i;

    for (i = 0; i < WINED3D_MAX_FRAGMENT_SAMPLERS; ++i)
    {
        if (resource_info[i].type && context_gl->tex_unit_map[i] != i)
        {
            wined3d_context_gl_map_stage(context_gl, i, i);
            context_invalidate_state(&context_gl->c, STATE_GRAPHICS_SHADER_RESOURCE_BINDING);
            if (i < d3d_info->ffp_fragment_caps.max_blend_stages)
                context_invalidate_texture_stage(&context_gl->c, i);
        }
    }
}

static BOOL wined3d_context_gl_unit_free_for_vs(const struct wined3d_context_gl *context_gl,
        const struct wined3d_shader_resource_info *ps_resource_info, unsigned int unit)
{
    unsigned int current_mapping = context_gl->rev_tex_unit_map[unit];

    /* Not currently used */
    if (current_mapping == WINED3D_UNMAPPED_STAGE)
        return TRUE;

    if (current_mapping < WINED3D_MAX_FRAGMENT_SAMPLERS)
    {
        /* Used by a fragment sampler */

        if (!ps_resource_info)
        {
            /* No pixel shader, check fixed function */
            return current_mapping >= WINED3D_MAX_FFP_TEXTURES
                    || !(context_gl->c.fixed_function_usage_map & (1u << current_mapping));
        }

        /* Pixel shader, check the shader's sampler map */
        return !ps_resource_info[current_mapping].type;
    }

    return TRUE;
}

static void wined3d_context_gl_map_vsamplers(struct wined3d_context_gl *context_gl,
        BOOL ps, const struct wined3d_state *state)
{
    const struct wined3d_shader_resource_info *vs_resource_info =
            state->shader[WINED3D_SHADER_TYPE_VERTEX]->reg_maps.resource_info;
    const struct wined3d_shader_resource_info *ps_resource_info = NULL;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    int start = min(WINED3D_MAX_COMBINED_SAMPLERS, gl_info->limits.graphics_samplers) - 1;
    int i;

    /* Note that we only care if a resource is used or not, not the
     * resource's specific type. Otherwise we'd need to call
     * shader_update_samplers() here for 1.x pixelshaders. */
    if (ps)
        ps_resource_info = state->shader[WINED3D_SHADER_TYPE_PIXEL]->reg_maps.resource_info;

    for (i = 0; i < WINED3D_MAX_VERTEX_SAMPLERS; ++i)
    {
        DWORD vsampler_idx = i + WINED3D_MAX_FRAGMENT_SAMPLERS;
        if (vs_resource_info[i].type)
        {
            while (start >= 0)
            {
                if (wined3d_context_gl_unit_free_for_vs(context_gl, ps_resource_info, start))
                {
                    if (context_gl->tex_unit_map[vsampler_idx] != start)
                    {
                        wined3d_context_gl_map_stage(context_gl, vsampler_idx, start);
                        context_invalidate_state(&context_gl->c, STATE_GRAPHICS_SHADER_RESOURCE_BINDING);
                    }

                    --start;
                    break;
                }

                --start;
            }
            if (context_gl->tex_unit_map[vsampler_idx] == WINED3D_UNMAPPED_STAGE)
                WARN("Couldn't find a free texture unit for vertex sampler %u.\n", i);
        }
    }
}

static void wined3d_context_gl_update_tex_unit_map(struct wined3d_context_gl *context_gl,
        const struct wined3d_state *state)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    BOOL vs = use_vs(state);
    BOOL ps = use_ps(state);

    if (!ps)
        context_update_fixed_function_usage_map(&context_gl->c, state);

    /* Try to go for a 1:1 mapping of the samplers when possible. Pixel shaders
     * need a 1:1 map at the moment.
     * When the mapping of a stage is changed, sampler and ALL texture stage
     * states have to be reset. */

    if (gl_info->limits.graphics_samplers >= WINED3D_MAX_COMBINED_SAMPLERS)
        return;

    if (ps)
        wined3d_context_gl_map_psamplers(context_gl, state);
    else
        wined3d_context_gl_map_fixed_function_samplers(context_gl, state);

    if (vs)
        wined3d_context_gl_map_vsamplers(context_gl, ps, state);
}

/* Context activation is done by the caller. */
void context_state_drawbuf(struct wined3d_context *context, const struct wined3d_state *state, DWORD state_id)
{
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    uint32_t rt_mask, *cur_mask;

    if (isStateDirty(context, STATE_FRAMEBUFFER)) return;

    cur_mask = context_gl->current_fbo ? &context_gl->current_fbo->rt_mask : &context_gl->draw_buffers_mask;
    rt_mask = find_draw_buffers_mask(context_gl, state);
    if (rt_mask != *cur_mask)
    {
        wined3d_context_gl_apply_draw_buffers(context_gl, rt_mask);
        *cur_mask = rt_mask;
    }
}

static void wined3d_context_gl_bind_shader_resources(struct wined3d_context_gl *context_gl,
        const struct wined3d_state *state, enum wined3d_shader_type shader_type)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    unsigned int bind_idx, shader_sampler_count, base, count, i;
    const struct wined3d_device *device = context_gl->c.device;
    struct wined3d_shader_sampler_map_entry *entry;
    struct wined3d_shader_resource_view *view;
    const struct wined3d_shader *shader;
    const unsigned int *tex_unit_map;
    struct wined3d_sampler *sampler;

    if (!(shader = state->shader[shader_type]))
    {
        if (shader_type == WINED3D_SHADER_TYPE_PIXEL)
        {
            uint32_t ffu_map = context_gl->c.fixed_function_usage_map;

            while (ffu_map)
            {
                i = wined3d_bit_scan(&ffu_map);
                bind_idx = context_gl->tex_unit_map[i];

                view = state->shader_resource_view[WINED3D_SHADER_TYPE_PIXEL][i];
                sampler = state->sampler[WINED3D_SHADER_TYPE_PIXEL][i];

                if (view)
                {
                    wined3d_shader_resource_view_gl_bind(wined3d_shader_resource_view_gl(view),
                            bind_idx, wined3d_sampler_gl(sampler), context_gl);
                }
                else
                {
                    WARN("No resource view bound at index %u.\n", i);
                    wined3d_context_gl_active_texture(context_gl, gl_info, bind_idx);
                    wined3d_context_gl_bind_texture(context_gl, GL_NONE, 0);
                    if (gl_info->supported[ARB_SAMPLER_OBJECTS])
                        GL_EXTCALL(glBindSampler(bind_idx, 0));
                }
            }
        }

        return;
    }

    tex_unit_map = wined3d_context_gl_get_tex_unit_mapping(context_gl,
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

        if ((view = state->shader_resource_view[shader_type][entry->resource_idx]))
        {
            if (entry->sampler_idx == WINED3D_SAMPLER_DEFAULT)
                sampler = device->default_sampler;
            else if (!(sampler = state->sampler[shader_type][entry->sampler_idx]))
                sampler = device->null_sampler;
            wined3d_shader_resource_view_gl_bind(wined3d_shader_resource_view_gl(view),
                    bind_idx, wined3d_sampler_gl(sampler), context_gl);
        }
        else
        {
            WARN("No resource view bound at index %u, %u.\n", shader_type, entry->resource_idx);
            wined3d_context_gl_active_texture(context_gl, gl_info, bind_idx);
            wined3d_context_gl_bind_texture(context_gl, GL_NONE, 0);
            if (gl_info->supported[ARB_SAMPLER_OBJECTS])
                GL_EXTCALL(glBindSampler(bind_idx, 0));
        }
    }
}

static void wined3d_context_gl_bind_unordered_access_views(struct wined3d_context_gl *context_gl,
        const struct wined3d_shader *shader, struct wined3d_unordered_access_view * const *views)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_unordered_access_view_gl *view_gl;
    const struct wined3d_format_gl *format_gl;
    GLuint texture_name;
    unsigned int i;
    GLint level;

    if (!shader)
        return;

    for (i = 0; i < MAX_UNORDERED_ACCESS_VIEWS; ++i)
    {
        if (!views[i])
        {
            if (shader->reg_maps.uav_resource_info[i].type)
                WARN("No unordered access view bound at index %u.\n", i);
            GL_EXTCALL(glBindImageTexture(i, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R8));
            continue;
        }

        view_gl = wined3d_unordered_access_view_gl(views[i]);
        if (view_gl->gl_view.name)
        {
            texture_name = view_gl->gl_view.name;
            level = 0;
        }
        else if (view_gl->v.resource->type != WINED3D_RTYPE_BUFFER)
        {
            struct wined3d_texture_gl *texture_gl = wined3d_texture_gl(texture_from_resource(view_gl->v.resource));
            texture_name = wined3d_texture_gl_get_texture_name(texture_gl, &context_gl->c, FALSE);
            level = view_gl->v.desc.u.texture.level_idx;
        }
        else
        {
            FIXME("Unsupported buffer unordered access view.\n");
            GL_EXTCALL(glBindImageTexture(i, 0, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R8));
            continue;
        }

        format_gl = wined3d_format_gl(view_gl->v.format);
        GL_EXTCALL(glBindImageTexture(i, texture_name, level, GL_TRUE, 0, GL_READ_WRITE,
                format_gl->internal));

        if (view_gl->counter_bo.id)
            GL_EXTCALL(glBindBufferRange(GL_ATOMIC_COUNTER_BUFFER, i, view_gl->counter_bo.id,
                    view_gl->counter_bo.b.buffer_offset, view_gl->counter_bo.size));
    }
    checkGLcall("Bind unordered access views");
}

static bool is_resource_rtv_bound(const struct wined3d_state *state, const struct wined3d_resource *resource)
{
    unsigned int i;

    if (state->fb.depth_stencil && state->fb.depth_stencil->resource == resource)
        return true;

    if (!resource->rtv_bind_count_device)
        return false;

    for (i = 0; i < ARRAY_SIZE(state->fb.render_targets); ++i)
    {
        if (state->fb.render_targets[i] && state->fb.render_targets[i]->resource == resource)
            return true;
    }

    return false;
}

static void context_gl_load_shader_resources(struct wined3d_context_gl *context_gl,
        const struct wined3d_state *state, unsigned int shader_mask)
{
    struct wined3d_shader_sampler_map_entry *entry;
    struct wined3d_shader_resource_view_gl *srv_gl;
    struct wined3d_shader_resource_view *view;
    struct wined3d_sampler *sampler;
    struct wined3d_shader *shader;
    struct wined3d_buffer *buffer;
    unsigned int i, j;

    for (i = 0; i < WINED3D_SHADER_TYPE_COUNT; ++i)
    {
        if (!(shader_mask & (1u << i)))
            continue;

        if (!(shader = state->shader[i]))
        {
            if (i == WINED3D_SHADER_TYPE_PIXEL)
            {
                uint32_t ffu_map = context_gl->c.fixed_function_usage_map;

                while (ffu_map)
                {
                    i = wined3d_bit_scan(&ffu_map);

                    view = state->shader_resource_view[WINED3D_SHADER_TYPE_PIXEL][i];
                    sampler = state->sampler[WINED3D_SHADER_TYPE_PIXEL][i];

                    if (view)
                        wined3d_texture_load(texture_from_resource(view->resource),
                                &context_gl->c, sampler->desc.srgb_decode);
                }
            }

            continue;
        }

        for (j = 0; j < WINED3D_MAX_CBS; ++j)
        {
            if (!state->cb[i][j].buffer)
                continue;

            buffer = state->cb[i][j].buffer;
            wined3d_buffer_load(buffer, &context_gl->c, state);
            wined3d_context_gl_reference_buffer(context_gl, buffer);
            if (!buffer->bo_user.valid)
                device_invalidate_state(context_gl->c.device, STATE_CONSTANT_BUFFER(i));
        }

        for (j = 0; j < shader->reg_maps.sampler_map.count; ++j)
        {
            entry = &shader->reg_maps.sampler_map.entries[j];

            if (!(view = state->shader_resource_view[i][entry->resource_idx]))
                continue;

            if (is_resource_rtv_bound(state, view->resource))
                context_gl->c.uses_fbo_attached_resources = 1;

            if (view->resource->type == WINED3D_RTYPE_BUFFER)
            {
                buffer = buffer_from_resource(view->resource);
                wined3d_buffer_load(buffer, &context_gl->c, state);
                wined3d_context_gl_reference_buffer(context_gl, buffer);

                srv_gl = wined3d_shader_resource_view_gl(view);
                if (!srv_gl->bo_user.valid)
                    wined3d_shader_resource_view_gl_update(srv_gl, context_gl);
            }
            else
            {
                BOOL srgb = TRUE;

                if (entry->sampler_idx != WINED3D_SAMPLER_DEFAULT
                        && (sampler = state->sampler[i][entry->sampler_idx]))
                    srgb = sampler->desc.srgb_decode;

                wined3d_texture_load(texture_from_resource(view->resource), &context_gl->c, srgb);
            }
        }
    }
}

static void context_gl_load_unordered_access_resources(struct wined3d_context_gl *context_gl,
        const struct wined3d_shader *shader, struct wined3d_unordered_access_view * const *views)
{
    struct wined3d_unordered_access_view_gl *uav_gl;
    struct wined3d_unordered_access_view *view;
    struct wined3d_texture *texture;
    struct wined3d_buffer *buffer;
    unsigned int i;

    context_gl->c.uses_uavs = 0;

    if (!shader)
        return;

    for (i = 0; i < MAX_UNORDERED_ACCESS_VIEWS; ++i)
    {
        if (!(view = views[i]))
            continue;

        if (view->resource->type == WINED3D_RTYPE_BUFFER)
        {
            buffer = buffer_from_resource(view->resource);
            wined3d_buffer_acquire_bo_for_write(buffer, &context_gl->c);
            wined3d_buffer_load_location(buffer, &context_gl->c, WINED3D_LOCATION_BUFFER);
            wined3d_unordered_access_view_invalidate_location(view, ~WINED3D_LOCATION_BUFFER);
            wined3d_context_gl_reference_buffer(context_gl, buffer);

            uav_gl = wined3d_unordered_access_view_gl(view);
            if (!uav_gl->bo_user.valid)
                wined3d_unordered_access_view_gl_update(uav_gl, context_gl);
        }
        else
        {
            texture = texture_from_resource(view->resource);
            wined3d_texture_load(texture, &context_gl->c, FALSE);
            wined3d_unordered_access_view_invalidate_location(view, ~WINED3D_LOCATION_TEXTURE_RGB);
        }

        context_gl->c.uses_uavs = 1;
    }
}

static void context_gl_load_stream_output_buffers(struct wined3d_context_gl *context_gl,
        const struct wined3d_state *state)
{
    struct wined3d_buffer *buffer;
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE(state->stream_output); ++i)
    {
        if (!(buffer = state->stream_output[i].buffer))
            continue;

        wined3d_buffer_acquire_bo_for_write(buffer, &context_gl->c);

        wined3d_buffer_load(buffer, &context_gl->c, state);
        wined3d_buffer_invalidate_location(buffer, ~WINED3D_LOCATION_BUFFER);
        wined3d_context_gl_reference_buffer(context_gl, buffer);
        if (!buffer->bo_user.valid)
            device_invalidate_state(context_gl->c.device, STATE_STREAM_OUTPUT);
    }
}

/* Context activation is done by the caller. */
static BOOL context_apply_draw_state(struct wined3d_context *context,
        const struct wined3d_device *device, const struct wined3d_state *state, BOOL indexed)
{
    const struct wined3d_state_entry *state_table = context->state_table;
    struct wined3d_context_gl *context_gl = wined3d_context_gl(context);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    const struct wined3d_fb_state *fb = &state->fb;
    unsigned int i, base;
    uint32_t map;

    context->uses_fbo_attached_resources = 0;

    if (!have_framebuffer_attachment(gl_info->limits.buffers, fb->render_targets, fb->depth_stencil))
    {
        if (!gl_info->supported[ARB_FRAMEBUFFER_NO_ATTACHMENTS])
        {
            FIXME("OpenGL implementation does not support framebuffers with no attachments.\n");
            return FALSE;
        }
    }

    /* Preload resources before FBO setup. Texture preload in particular may
     * result in changes to the current FBO, due to using e.g. FBO blits for
     * updating a resource location. */
    wined3d_context_gl_update_tex_unit_map(context_gl, state);
    context_gl_load_shader_resources(context_gl, state, ~(1u << WINED3D_SHADER_TYPE_COMPUTE));
    context_gl_load_unordered_access_resources(context_gl, state->shader[WINED3D_SHADER_TYPE_PIXEL],
            state->unordered_access_view[WINED3D_PIPELINE_GRAPHICS]);
    context_gl_load_stream_output_buffers(context_gl, state);
    /* TODO: Right now the dependency on the vertex shader is necessary
     * since wined3d_stream_info_from_declaration() depends on the reg_maps of
     * the current VS but maybe it's possible to relax the coupling in some
     * situations at least. */
    if (isStateDirty(context, STATE_VDECL) || isStateDirty(context, STATE_STREAMSRC)
            || isStateDirty(context, STATE_SHADER(WINED3D_SHADER_TYPE_VERTEX)))
    {
        context_update_stream_info(context, state);
    }

    map = context->stream_info.use_map;
    while (map)
    {
        const struct wined3d_stream_info_element *e;
        struct wined3d_buffer *buffer;

        e = &context->stream_info.elements[wined3d_bit_scan(&map)];
        buffer = state->streams[e->stream_idx].buffer;

        if (!buffer->bo_user.valid)
            device_invalidate_state(device, STATE_STREAMSRC);
        else
            wined3d_buffer_load(buffer, context, state);
    }
    /* Loading the buffers above may have invalidated the stream info. */
    if (wined3d_context_is_graphics_state_dirty(context, STATE_STREAMSRC))
        context_update_stream_info(context, state);

    map = context->stream_info.use_map;
    while (map)
    {
        const struct wined3d_stream_info_element *e;
        struct wined3d_buffer *buffer;

        e = &context->stream_info.elements[wined3d_bit_scan(&map)];
        buffer = state->streams[e->stream_idx].buffer;

        wined3d_context_gl_reference_buffer(context_gl, buffer);
    }

    if (indexed && state->index_buffer)
    {
        struct wined3d_buffer *buffer = state->index_buffer;

        if (context->stream_info.all_vbo)
        {
            wined3d_buffer_load(buffer, context, state);
            if (!buffer->bo_user.valid)
                device_invalidate_state(device, STATE_INDEXBUFFER);
            wined3d_context_gl_reference_buffer(context_gl, buffer);
        }
        else
        {
            wined3d_buffer_load_sysmem(buffer, context);
        }
    }

    for (i = 0, base = 0; i < ARRAY_SIZE(context->dirty_graphics_states); ++i)
    {
        uint32_t dirty_mask = context->dirty_graphics_states[i];

        while (dirty_mask)
        {
            unsigned int state_id = base + wined3d_bit_scan(&dirty_mask);

            state_table[state_id].apply(context, state, state_id);
        }
        base += sizeof(dirty_mask) * CHAR_BIT;
    }

    memset(context->dirty_graphics_states, 0, sizeof(context->dirty_graphics_states));

    if (context->update_shader_resource_bindings)
    {
        for (i = 0; i < WINED3D_SHADER_TYPE_GRAPHICS_COUNT; ++i)
            wined3d_context_gl_bind_shader_resources(context_gl, state, i);
        context->update_shader_resource_bindings = 0;
        if (gl_info->limits.combined_samplers == gl_info->limits.graphics_samplers)
            context->update_compute_shader_resource_bindings = 1;
    }

    if (context->update_unordered_access_view_bindings)
    {
        wined3d_context_gl_bind_unordered_access_views(context_gl,
                state->shader[WINED3D_SHADER_TYPE_PIXEL],
                state->unordered_access_view[WINED3D_PIPELINE_GRAPHICS]);
        context->update_unordered_access_view_bindings = 0;
        context->update_compute_unordered_access_view_bindings = 1;
    }

    wined3d_context_gl_check_fbo_status(context_gl, GL_FRAMEBUFFER);

    device->shader_backend->shader_apply_draw_state(device->shader_priv, context, state);
    context->shader_update_mask &= 1u << WINED3D_SHADER_TYPE_COMPUTE;
    context->constant_update_mask = 0;

    context->last_was_blit = FALSE;
    context->last_was_ffp_blit = FALSE;

    return TRUE;
}

static void wined3d_context_gl_apply_compute_state(struct wined3d_context_gl *context_gl,
        const struct wined3d_device *device, const struct wined3d_state *state)
{
    const struct wined3d_state_entry *state_table = context_gl->c.state_table;
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    unsigned int state_id, i;

    context_gl_load_shader_resources(context_gl, state, 1u << WINED3D_SHADER_TYPE_COMPUTE);
    context_gl_load_unordered_access_resources(context_gl, state->shader[WINED3D_SHADER_TYPE_COMPUTE],
            state->unordered_access_view[WINED3D_PIPELINE_COMPUTE]);

    for (i = 0, state_id = STATE_COMPUTE_OFFSET; i < ARRAY_SIZE(context_gl->c.dirty_compute_states); ++i)
    {
        unsigned int dirty_mask = context_gl->c.dirty_compute_states[i];

        while (dirty_mask)
        {
            unsigned int current_state_id = state_id + wined3d_bit_scan(&dirty_mask);
            state_table[current_state_id].apply(&context_gl->c, state, current_state_id);
        }
        state_id += sizeof(*context_gl->c.dirty_compute_states) * CHAR_BIT;
    }
    memset(context_gl->c.dirty_compute_states, 0, sizeof(*context_gl->c.dirty_compute_states));

    device->shader_backend->shader_apply_compute_state(device->shader_priv, &context_gl->c, state);

    if (context_gl->c.update_compute_shader_resource_bindings)
    {
        wined3d_context_gl_bind_shader_resources(context_gl, state, WINED3D_SHADER_TYPE_COMPUTE);
        context_gl->c.update_compute_shader_resource_bindings = 0;
        if (gl_info->limits.combined_samplers == gl_info->limits.graphics_samplers)
            context_gl->c.update_shader_resource_bindings = 1;
    }

    if (context_gl->c.update_compute_unordered_access_view_bindings)
    {
        wined3d_context_gl_bind_unordered_access_views(context_gl,
                state->shader[WINED3D_SHADER_TYPE_COMPUTE],
                state->unordered_access_view[WINED3D_PIPELINE_COMPUTE]);
        context_gl->c.update_compute_unordered_access_view_bindings = 0;
        context_gl->c.update_unordered_access_view_bindings = 1;
    }

    /* Updates to currently bound render targets aren't necessarily coherent
     * between the graphics and compute pipelines. Unbind any currently bound
     * FBO here to ensure preceding updates to its attachments by the graphics
     * pipeline are visible to the compute pipeline.
     *
     * Without this, the bloom effect in Nier:Automata is too bright on the
     * Mesa radeonsi driver, and presumably on other Mesa based drivers. */
    wined3d_context_gl_bind_fbo(context_gl, GL_FRAMEBUFFER, 0);
    context_invalidate_state(&context_gl->c, STATE_FRAMEBUFFER);

    context_gl->c.last_was_blit = FALSE;
    context_gl->c.last_was_ffp_blit = FALSE;
    context_gl->c.shader_update_mask &= ~(1u << WINED3D_SHADER_TYPE_COMPUTE);
}

void wined3d_context_gl_end_transform_feedback(struct wined3d_context_gl *context_gl)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    if (context_gl->c.transform_feedback_active)
    {
        GL_EXTCALL(glEndTransformFeedback());
        checkGLcall("glEndTransformFeedback");
        context_gl->c.transform_feedback_active = 0;
        context_gl->c.transform_feedback_paused = 0;
    }
}

static void wined3d_context_gl_pause_transform_feedback(struct wined3d_context_gl *context_gl, BOOL force)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    if (!context_gl->c.transform_feedback_active || context_gl->c.transform_feedback_paused)
        return;

    if (gl_info->supported[ARB_TRANSFORM_FEEDBACK2])
    {
        GL_EXTCALL(glPauseTransformFeedback());
        checkGLcall("glPauseTransformFeedback");
        context_gl->c.transform_feedback_paused = 1;
        return;
    }

    WARN("Cannot pause transform feedback operations.\n");

    if (force)
        wined3d_context_gl_end_transform_feedback(context_gl);
}

static void wined3d_context_gl_setup_target(struct wined3d_context_gl *context_gl,
        struct wined3d_texture *texture, unsigned int sub_resource_idx)
{
    if (context_gl->c.current_rt.texture == texture
            && context_gl->c.current_rt.sub_resource_idx == sub_resource_idx)
        return;

    /* To compensate the lack of format switching with some offscreen rendering methods and on onscreen buffers
     * the alpha blend state changes with different render target formats. */
    if (!context_gl->c.current_rt.texture)
    {
        context_invalidate_state(&context_gl->c, STATE_BLEND);
    }
    else
    {
        const struct wined3d_format *old = context_gl->c.current_rt.texture->resource.format;
        const struct wined3d_format *new = texture->resource.format;

        if (old->id != new->id)
        {
            /* Disable blending when the alpha mask has changed and when a format doesn't support blending. */
            if ((old->alpha_size && !new->alpha_size) || (!old->alpha_size && new->alpha_size)
                    || !(texture->resource.format_caps & WINED3D_FORMAT_CAP_POSTPIXELSHADER_BLENDING))
                context_invalidate_state(&context_gl->c, STATE_BLEND);
        }
    }

    context_gl->c.current_rt.texture = texture;
    context_gl->c.current_rt.sub_resource_idx = sub_resource_idx;
}

static void wined3d_context_gl_activate(struct wined3d_context_gl *context_gl,
        struct wined3d_texture *texture, unsigned int sub_resource_idx)
{
    wined3d_context_gl_enter(context_gl);
    if (texture && texture->swapchain && texture->swapchain != context_gl->c.swapchain)
    {
        TRACE("Switching context_gl %p from swapchain %p to swapchain %p.\n",
                context_gl, context_gl->c.swapchain, texture->swapchain);
        context_gl->c.swapchain = texture->swapchain;
    }
    wined3d_context_gl_update_window(context_gl);
    wined3d_context_gl_setup_target(context_gl, texture, sub_resource_idx);
    if (!context_gl->valid)
        return;

    if (context_gl != wined3d_context_gl_get_current())
    {
        if (!wined3d_context_gl_set_current(context_gl))
            ERR("Failed to activate the new context.\n");
    }
    else if (context_gl->needs_set)
    {
        wined3d_context_gl_set_gl_context(context_gl);
    }
}

struct wined3d_context *wined3d_context_gl_acquire(const struct wined3d_device *device,
        struct wined3d_texture *texture, unsigned int sub_resource_idx)
{
    struct wined3d_context_gl *current_context = wined3d_context_gl_get_current();
    struct wined3d_context_gl *context_gl;

    TRACE("device %p, texture %p, sub_resource_idx %u.\n", device, texture, sub_resource_idx);

    if (current_context && current_context->c.destroyed)
        current_context = NULL;

    if (!texture)
    {
        if (current_context
                && current_context->c.device == device)
        {
            texture = current_context->c.current_rt.texture;
            sub_resource_idx = current_context->c.current_rt.sub_resource_idx;
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

    if (current_context && current_context->c.current_rt.texture == texture)
    {
        context_gl = current_context;
    }
    else if (!wined3d_resource_is_offscreen(&texture->resource))
    {
        TRACE("Rendering onscreen.\n");

        if (!(context_gl = wined3d_swapchain_gl_get_context(wined3d_swapchain_gl(texture->swapchain))))
            return NULL;
    }
    else
    {
        TRACE("Rendering offscreen.\n");

        /* Stay with the current context if possible. Otherwise use the
         * context for the primary swapchain. */
        if (current_context && current_context->c.device == device)
            context_gl = current_context;
        else if (!(context_gl = wined3d_swapchain_gl_get_context(wined3d_swapchain_gl(device->swapchains[0]))))
            return NULL;
    }

    wined3d_context_gl_activate(context_gl, texture, sub_resource_idx);

    return &context_gl->c;
}

struct wined3d_context_gl *wined3d_context_gl_reacquire(struct wined3d_context_gl *context_gl)
{
    struct wined3d_context *acquired_context;
    struct wined3d_device *device;

    if (!context_gl || context_gl->tid != GetCurrentThreadId())
        return NULL;

    device = context_gl->c.device;
    wined3d_from_cs(device->cs);

    if (context_gl->c.current_rt.texture)
    {
        wined3d_context_gl_activate(context_gl, context_gl->c.current_rt.texture,
                context_gl->c.current_rt.sub_resource_idx);
        return context_gl;
    }

    acquired_context = context_acquire(device, NULL, 0);
    if (acquired_context != &context_gl->c)
        ERR("Acquired context %p instead of %p.\n", acquired_context, &context_gl->c);
    return wined3d_context_gl(acquired_context);
}

void dispatch_compute(struct wined3d_device *device, const struct wined3d_state *state,
        const struct wined3d_dispatch_parameters *parameters)
{
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context_gl *context_gl;

    context_gl = wined3d_context_gl(context_acquire(device, NULL, 0));
    if (!context_gl->valid)
    {
        context_release(&context_gl->c);
        WARN("Invalid context, skipping dispatch.\n");
        return;
    }
    gl_info = context_gl->gl_info;

    if (!gl_info->supported[ARB_COMPUTE_SHADER])
    {
        context_release(&context_gl->c);
        FIXME("OpenGL implementation does not support compute shaders.\n");
        return;
    }

    if (parameters->indirect)
        wined3d_buffer_load(parameters->u.indirect.buffer, &context_gl->c, state);

    wined3d_context_gl_apply_compute_state(context_gl, device, state);

    if (parameters->indirect)
    {
        const struct wined3d_indirect_dispatch_parameters *indirect = &parameters->u.indirect;
        struct wined3d_bo_gl *bo_gl = wined3d_bo_gl(indirect->buffer->buffer_object);

        GL_EXTCALL(glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, bo_gl->id));
        GL_EXTCALL(glDispatchComputeIndirect(bo_gl->b.buffer_offset + (GLintptr)indirect->offset));
        GL_EXTCALL(glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, 0));
        wined3d_context_gl_reference_bo(context_gl, bo_gl);
    }
    else
    {
        const struct wined3d_direct_dispatch_parameters *direct = &parameters->u.direct;
        GL_EXTCALL(glDispatchCompute(direct->group_count_x, direct->group_count_y, direct->group_count_z));
    }
    checkGLcall("dispatch compute");

    GL_EXTCALL(glMemoryBarrier(GL_ALL_BARRIER_BITS));
    checkGLcall("glMemoryBarrier");

    context_release(&context_gl->c);
}

/* Context activation is done by the caller. */
static void wined3d_context_gl_draw_primitive_arrays(struct wined3d_context_gl *context_gl,
        const struct wined3d_state *state, const void *idx_data, unsigned int idx_size, int base_vertex_idx,
        unsigned int start_idx, unsigned int count, unsigned int start_instance, unsigned int instance_count)
{
    GLenum idx_type = idx_size == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
    const struct wined3d_stream_info *si = &context_gl->c.stream_info;
    GLenum mode = gl_primitive_type_from_d3d(state->primitive_type);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    unsigned int instanced_elements[ARRAY_SIZE(si->elements)];
    const struct wined3d_ffp_attrib_ops *ops;
    unsigned int instanced_element_count = 0;
    const void *indices;
    unsigned int i, j;

    ops = &gl_info->ffp_attrib_ops;

    indices = (const char *)idx_data + idx_size * start_idx;

    if (!instance_count)
    {
        if (!idx_size)
        {
            gl_info->gl_ops.gl.p_glDrawArrays(mode, start_idx, count);
            checkGLcall("glDrawArrays");
            return;
        }

        if (gl_info->supported[ARB_DRAW_ELEMENTS_BASE_VERTEX])
        {
            GL_EXTCALL(glDrawElementsBaseVertex(mode, count, idx_type, indices, base_vertex_idx));
            checkGLcall("glDrawElementsBaseVertex");
            return;
        }

        gl_info->gl_ops.gl.p_glDrawElements(mode, count, idx_type, indices);
        checkGLcall("glDrawElements");
        return;
    }

    if (start_instance && !(gl_info->supported[ARB_BASE_INSTANCE] && gl_info->supported[ARB_INSTANCED_ARRAYS]))
        FIXME("Start instance (%u) not supported.\n", start_instance);

    if (gl_info->supported[ARB_INSTANCED_ARRAYS])
    {
        if (!idx_size)
        {
            if (gl_info->supported[ARB_BASE_INSTANCE])
            {
                GL_EXTCALL(glDrawArraysInstancedBaseInstance(mode, start_idx, count, instance_count, start_instance));
                checkGLcall("glDrawArraysInstancedBaseInstance");
                return;
            }

            GL_EXTCALL(glDrawArraysInstanced(mode, start_idx, count, instance_count));
            checkGLcall("glDrawArraysInstanced");
            return;
        }

        if (gl_info->supported[ARB_BASE_INSTANCE])
        {
            GL_EXTCALL(glDrawElementsInstancedBaseVertexBaseInstance(mode, count, idx_type,
                    indices, instance_count, base_vertex_idx, start_instance));
            checkGLcall("glDrawElementsInstancedBaseVertexBaseInstance");
            return;
        }
        if (gl_info->supported[ARB_DRAW_ELEMENTS_BASE_VERTEX])
        {
            GL_EXTCALL(glDrawElementsInstancedBaseVertex(mode, count, idx_type,
                    indices, instance_count, base_vertex_idx));
            checkGLcall("glDrawElementsInstancedBaseVertex");
            return;
        }

        GL_EXTCALL(glDrawElementsInstanced(mode, count, idx_type, indices, instance_count));
        checkGLcall("glDrawElementsInstanced");
        return;
    }

    /* Instancing emulation by mixing immediate mode and arrays. */

    /* This is a nasty thing. MSDN says no hardware supports this and
     * applications have to use software vertex processing. We don't support
     * this for now.
     *
     * Shouldn't be too hard to support with OpenGL, in theory just call
     * glDrawArrays() instead of drawElements(). But the stream frequency value
     * has a different meaning in that situation. */
    if (!idx_size)
    {
        FIXME("Non-indexed instanced drawing is not supported.\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(si->elements); ++i)
    {
        if (!(si->use_map & (1u << i)))
            continue;

        if (state->streams[si->elements[i].stream_idx].flags & WINED3DSTREAMSOURCE_INSTANCEDATA)
            instanced_elements[instanced_element_count++] = i;
    }

    for (i = 0; i < instance_count; ++i)
    {
        /* Specify the instanced attributes using immediate mode calls. */
        for (j = 0; j < instanced_element_count; ++j)
        {
            const struct wined3d_stream_info_element *element;
            unsigned int element_idx;
            const BYTE *ptr;

            element_idx = instanced_elements[j];
            element = &si->elements[element_idx];
            ptr = element->data.addr + element->stride * i;
            if (element->data.buffer_object)
                ptr += (ULONG_PTR)wined3d_buffer_load_sysmem(state->streams[element->stream_idx].buffer,
                        &context_gl->c);
            ops->generic[element->format->emit_idx](element_idx, ptr);
        }

        if (gl_info->supported[ARB_DRAW_ELEMENTS_BASE_VERTEX])
        {
            GL_EXTCALL(glDrawElementsBaseVertex(mode, count, idx_type, indices, base_vertex_idx));
            checkGLcall("glDrawElementsBaseVertex");
        }
        else
        {
            gl_info->gl_ops.gl.p_glDrawElements(mode, count, idx_type, indices);
            checkGLcall("glDrawElements");
        }
    }
}

static void wined3d_context_gl_draw_indirect(struct wined3d_context_gl *context_gl, const struct wined3d_state *state,
        const struct wined3d_indirect_draw_parameters *parameters, unsigned int idx_size)
{
    struct wined3d_bo_gl *bo_gl = wined3d_bo_gl(parameters->buffer->buffer_object);
    GLenum gl_primitive_type = gl_primitive_type_from_d3d(state->primitive_type);
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    const void *offset;

    if (!gl_info->supported[ARB_DRAW_INDIRECT])
    {
        FIXME("OpenGL implementation does not support indirect draws.\n");
        return;
    }

    GL_EXTCALL(glBindBuffer(GL_DRAW_INDIRECT_BUFFER, bo_gl->id));

    offset = (const uint8_t *)bo_gl->b.buffer_offset + parameters->offset;
    if (idx_size)
    {
        GLenum idx_type = idx_size == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
        if (state->index_offset)
            FIXME("Ignoring index offset %u.\n", state->index_offset);
        GL_EXTCALL(glDrawElementsIndirect(gl_primitive_type, idx_type, offset));
    }
    else
    {
        GL_EXTCALL(glDrawArraysIndirect(gl_primitive_type, offset));
    }

    GL_EXTCALL(glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0));
    wined3d_context_gl_reference_bo(context_gl, bo_gl);

    checkGLcall("draw indirect");
}

static GLenum gl_tfb_primitive_type_from_d3d(enum wined3d_primitive_type primitive_type)
{
    GLenum gl_primitive_type = gl_primitive_type_from_d3d(primitive_type);
    switch (gl_primitive_type)
    {
        case GL_POINTS:
            return GL_POINTS;

        case GL_LINE_STRIP:
        case GL_LINE_STRIP_ADJACENCY:
        case GL_LINES_ADJACENCY:
        case GL_LINES:
            return GL_LINES;

        case GL_TRIANGLE_FAN:
        case GL_TRIANGLE_STRIP:
        case GL_TRIANGLE_STRIP_ADJACENCY:
        case GL_TRIANGLES_ADJACENCY:
        case GL_TRIANGLES:
            return GL_TRIANGLES;

        default:
            return gl_primitive_type;
    }
}

/* Routine common to the draw primitive and draw indexed primitive routines */
void draw_primitive(struct wined3d_device *device, const struct wined3d_state *state,
        const struct wined3d_draw_parameters *parameters)
{
    const struct wined3d_fb_state *fb = &state->fb;
    const struct wined3d_stream_info *stream_info;
    struct wined3d_rendertarget_view *dsv, *rtv;
    const struct wined3d_gl_info *gl_info;
    struct wined3d_context_gl *context_gl;
    struct wined3d_context *context;
    bool rasterizer_discard = false;
    unsigned int i, idx_size = 0;
    const void *idx_data = NULL;

    TRACE("device %p, state %p, parameters %p.\n", device, state, parameters);

    if (!parameters->indirect && !parameters->u.direct.index_count)
        return;

    if (!parameters->indirect)
        TRACE("base_vertex_idx %d, start_idx %u, index_count %u, start_instance %u, instance_count %u.\n",
                parameters->u.direct.base_vertex_idx, parameters->u.direct.start_idx,
                parameters->u.direct.index_count, parameters->u.direct.start_instance,
                parameters->u.direct.instance_count);

    if (!(rtv = fb->render_targets[0]))
        rtv = fb->depth_stencil;

    if (rtv && rtv->resource->type == WINED3D_RTYPE_BUFFER)
    {
        FIXME("Buffer render targets not implemented.\n");
        return;
    }

    if (rtv)
        context = context_acquire(device, wined3d_texture_from_resource(rtv->resource), rtv->sub_resource_idx);
    else
        context = context_acquire(device, NULL, 0);
    context_gl = wined3d_context_gl(context);
    if (!context_gl->valid)
    {
        context_release(context);
        WARN("Invalid context, skipping draw.\n");
        return;
    }
    gl_info = context_gl->gl_info;

    if (!use_transform_feedback(state))
        wined3d_context_gl_pause_transform_feedback(context_gl, TRUE);

    for (i = 0; i < gl_info->limits.buffers; ++i)
    {
        if (!(rtv = fb->render_targets[i]) || rtv->format->id == WINED3DFMT_NULL)
            continue;

        if (wined3d_blend_state_get_writemask(state->blend_state, i))
        {
            wined3d_rendertarget_view_load_location(rtv, context, rtv->resource->draw_binding);
            wined3d_rendertarget_view_invalidate_location(rtv, ~rtv->resource->draw_binding);
        }
        else
        {
            wined3d_rendertarget_view_prepare_location(rtv, context, rtv->resource->draw_binding);
        }
    }

    if ((dsv = fb->depth_stencil))
    {
        /* We don't currently take the Z-compare function into account,
         * but we could skip loading the depthstencil for D3DCMP_NEVER and
         * D3DCMP_ALWAYS as well.
         * Also note that we never copy the stencil data. */
        uint32_t location = dsv->resource->draw_binding;

        if (wined3d_state_uses_depth_buffer(state))
            wined3d_rendertarget_view_load_location(dsv, context, location);
        else
            wined3d_rendertarget_view_prepare_location(dsv, context, location);
    }

    if (parameters->indirect)
        wined3d_buffer_load(parameters->u.indirect.buffer, context, state);

    if (!context_apply_draw_state(context, device, state, parameters->indexed))
    {
        context_release(context);
        WARN("Unable to apply draw state, skipping draw.\n");
        return;
    }

    if (dsv && (!state->depth_stencil_state || state->depth_stencil_state->writes_ds))
    {
        uint32_t location = dsv->resource->draw_binding;

        wined3d_rendertarget_view_validate_location(dsv, location);
        wined3d_rendertarget_view_invalidate_location(dsv, ~location);
    }

    stream_info = &context->stream_info;

    if (parameters->indexed)
    {
        struct wined3d_buffer *index_buffer = state->index_buffer;
        struct wined3d_bo *bo = index_buffer->buffer_object;

        if (!bo || !stream_info->all_vbo)
            idx_data = index_buffer->resource.heap_memory;
        else
            idx_data = (void *)bo->buffer_offset;
        idx_data = (const BYTE *)idx_data + state->index_offset;

        if (state->index_format == WINED3DFMT_R16_UINT)
            idx_size = 2;
        else
            idx_size = 4;
    }

    if (use_transform_feedback(state))
    {
        const struct wined3d_shader *shader = state->shader[WINED3D_SHADER_TYPE_GEOMETRY];

        if (is_rasterization_disabled(shader))
        {
            glEnable(GL_RASTERIZER_DISCARD);
            checkGLcall("enable rasterizer discard");
            rasterizer_discard = true;
        }

        if (context->transform_feedback_paused)
        {
            GL_EXTCALL(glResumeTransformFeedback());
            checkGLcall("glResumeTransformFeedback");
            context->transform_feedback_paused = 0;
        }
        else if (!context->transform_feedback_active)
        {
            enum wined3d_primitive_type primitive_type = shader->u.gs.output_type
                    ? shader->u.gs.output_type : state->primitive_type;
            GLenum mode = gl_tfb_primitive_type_from_d3d(primitive_type);
            GL_EXTCALL(glBeginTransformFeedback(mode));
            checkGLcall("glBeginTransformFeedback");
            context->transform_feedback_active = 1;
        }
    }

    if (state->primitive_type == WINED3D_PT_PATCH)
    {
        GL_EXTCALL(glPatchParameteri(GL_PATCH_VERTICES, state->patch_vertex_count));
        checkGLcall("glPatchParameteri");
    }

    if (context->uses_fbo_attached_resources)
    {
        static unsigned int fixme_once;

        if (gl_info->supported[ARB_TEXTURE_BARRIER])
        {
            GL_EXTCALL(glTextureBarrier());
        }
        else if (gl_info->supported[NV_TEXTURE_BARRIER])
        {
            GL_EXTCALL(glTextureBarrierNV());
        }
        else
        {
            if (!fixme_once++)
                FIXME("Sampling attached render targets is not supported.\n");

            WARN("Sampling attached render targets is not supported, skipping draw.\n");
            context_release(context);
            return;
        }
        checkGLcall("glTextureBarrier");
    }

    if (parameters->indirect)
    {
        wined3d_context_gl_draw_indirect(context_gl, state, &parameters->u.indirect, idx_size);
    }
    else
    {
        wined3d_context_gl_draw_primitive_arrays(context_gl, state, idx_data, idx_size,
                parameters->u.direct.base_vertex_idx, parameters->u.direct.start_idx,
                parameters->u.direct.index_count, parameters->u.direct.start_instance,
                parameters->u.direct.instance_count);
    }

    if (context->uses_uavs)
    {
        GL_EXTCALL(glMemoryBarrier(GL_ALL_BARRIER_BITS));
        checkGLcall("glMemoryBarrier");
    }

    if (rasterizer_discard)
    {
        glDisable(GL_RASTERIZER_DISCARD);
        checkGLcall("disable rasterizer discard");
    }

    context_release(context);

    TRACE("Draw completed.\n");
}

static const void *get_vertex_attrib_pointer(const struct wined3d_stream_info_element *element,
        const struct wined3d_state *state)
{
    const uint8_t *offset = element->data.addr + state->load_base_vertex_index * element->stride;

    if (element->data.buffer_object)
        offset += element->data.buffer_object->buffer_offset;
    return offset;
}

static void wined3d_context_gl_unload_numbered_array(struct wined3d_context_gl *context_gl, unsigned int i)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;

    GL_EXTCALL(glDisableVertexAttribArray(i));
    checkGLcall("glDisableVertexAttribArray");
    if (gl_info->supported[ARB_INSTANCED_ARRAYS])
        GL_EXTCALL(glVertexAttribDivisor(i, 0));

    context_gl->c.numbered_array_mask &= ~(1u << i);
}

static void wined3d_context_gl_unload_numbered_arrays(struct wined3d_context_gl *context_gl)
{
    uint32_t mask = context_gl->c.numbered_array_mask;
    unsigned int i;

    while (mask)
    {
        i = wined3d_bit_scan(&mask);
        wined3d_context_gl_unload_numbered_array(context_gl, i);
    }
}

static void wined3d_context_gl_load_numbered_arrays(struct wined3d_context_gl *context_gl,
        const struct wined3d_stream_info *stream_info, const struct wined3d_state *state)
{
    struct wined3d_context *context = &context_gl->c;
    const struct wined3d_shader *vs = state->shader[WINED3D_SHADER_TYPE_VERTEX];
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    GLuint current_bo, bo;
    unsigned int i;

    /* Default to no instancing. */
    current_bo = gl_info->supported[ARB_VERTEX_BUFFER_OBJECT] ? ~0u : 0;

    if (stream_info->use_map & ~wined3d_mask_from_size(gl_info->limits.vertex_attribs))
    {
        static unsigned int once;

        if (!once++)
            FIXME("More than the supported %u vertex attributes are in use.\n", gl_info->limits.vertex_attribs);
    }

    for (i = 0; i < gl_info->limits.vertex_attribs; ++i)
    {
        const struct wined3d_stream_info_element *element = &stream_info->elements[i];
        const struct wined3d_stream_state *stream;
        const struct wined3d_format_gl *format_gl;

        if (!(stream_info->use_map & (1u << i)))
        {
            if (context->numbered_array_mask & (1u << i))
                wined3d_context_gl_unload_numbered_array(context_gl, i);
            if (!(context_gl->default_attrib_value_set & (1u << i)))
                GL_EXTCALL(glVertexAttrib4f(i, 0.0f, 0.0f, 0.0f, 0.0f));
            context_gl->default_attrib_value_set |= 1u << i;
            continue;
        }

        format_gl = wined3d_format_gl(element->format);
        stream = &state->streams[element->stream_idx];

        if (gl_info->supported[ARB_INSTANCED_ARRAYS])
        {
            unsigned int divisor = 0;

            if (element->instanced)
                divisor = element->divisor ? element->divisor : UINT_MAX;
            GL_EXTCALL(glVertexAttribDivisor(i, divisor));
        }
        else if (element->divisor)
        {
            /* Unload instanced arrays, they will be loaded using immediate
             * mode instead. */
            if (context->numbered_array_mask & (1u << i))
                wined3d_context_gl_unload_numbered_array(context_gl, i);
            context_gl->default_attrib_value_set &= ~(1u << i);
            continue;
        }

        TRACE("Loading array %u %s.\n", i, debug_bo_address(&element->data));

        if (element->stride)
        {
            const void *offset = get_vertex_attrib_pointer(element, state);
            unsigned int format_attrs = format_gl->f.attrs;

            bo = wined3d_bo_gl_id(element->data.buffer_object);
            if (current_bo != bo)
            {
                GL_EXTCALL(glBindBuffer(GL_ARRAY_BUFFER, bo));
                checkGLcall("glBindBuffer");
                current_bo = bo;
            }
            if (bo)
                wined3d_buffer_validate_user(stream->buffer);

            /* Use the VBO to find out if a vertex buffer exists, not the vb
             * pointer. vb can point to a user pointer data blob. In that case
             * current_bo will be 0. If there is a vertex buffer but no vbo we
             * won't be load converted attributes anyway. */
            if (vs && vs->reg_maps.shader_version.major >= 4 && (format_attrs & WINED3D_FORMAT_ATTR_INTEGER))
            {
                GL_EXTCALL(glVertexAttribIPointer(i, format_gl->vtx_format,
                        format_gl->vtx_type, element->stride, offset));
            }
            else
            {
                GL_EXTCALL(glVertexAttribPointer(i, format_gl->vtx_format, format_gl->vtx_type,
                        !!(format_attrs & WINED3D_FORMAT_ATTR_NORMALISED), element->stride, offset));
            }

            if (!(context->numbered_array_mask & (1u << i)))
            {
                GL_EXTCALL(glEnableVertexAttribArray(i));
                context->numbered_array_mask |= (1u << i);
            }
        }
        else
        {
            /* Stride = 0 means always the same values.
             * glVertexAttribPointer() doesn't do that. Instead disable the
             * pointer and set up the attribute statically. But we have to
             * figure out the system memory address. */
            const BYTE *ptr = element->data.addr;
            if (element->data.buffer_object)
                ptr += (ULONG_PTR)wined3d_buffer_load_sysmem(stream->buffer, context);

            if (context->numbered_array_mask & (1u << i))
                wined3d_context_gl_unload_numbered_array(context_gl, i);

            switch (format_gl->f.id)
            {
                case WINED3DFMT_R32_FLOAT:
                    GL_EXTCALL(glVertexAttrib1fv(i, (const GLfloat *)ptr));
                    break;
                case WINED3DFMT_R32G32_FLOAT:
                    GL_EXTCALL(glVertexAttrib2fv(i, (const GLfloat *)ptr));
                    break;
                case WINED3DFMT_R32G32B32_FLOAT:
                    GL_EXTCALL(glVertexAttrib3fv(i, (const GLfloat *)ptr));
                    break;
                case WINED3DFMT_R32G32B32A32_FLOAT:
                    GL_EXTCALL(glVertexAttrib4fv(i, (const GLfloat *)ptr));
                    break;
                case WINED3DFMT_R8G8B8A8_UINT:
                    GL_EXTCALL(glVertexAttrib4ubv(i, ptr));
                    break;
                case WINED3DFMT_B8G8R8A8_UNORM:
                    if (gl_info->supported[ARB_VERTEX_ARRAY_BGRA])
                    {
                        const DWORD *src = (const DWORD *)ptr;
                        DWORD c = *src & 0xff00ff00u;
                        c |= (*src & 0xff0000u) >> 16;
                        c |= (*src & 0xffu) << 16;
                        GL_EXTCALL(glVertexAttrib4Nubv(i, (GLubyte *)&c));
                        break;
                    }
                    /* else fallthrough */
                case WINED3DFMT_R8G8B8A8_UNORM:
                    GL_EXTCALL(glVertexAttrib4Nubv(i, ptr));
                    break;
                case WINED3DFMT_R16G16_SINT:
                    GL_EXTCALL(glVertexAttrib2sv(i, (const GLshort *)ptr));
                    break;
                case WINED3DFMT_R16G16B16A16_SINT:
                    GL_EXTCALL(glVertexAttrib4sv(i, (const GLshort *)ptr));
                    break;
                case WINED3DFMT_R16G16_SNORM:
                {
                    const GLshort s[4] = {((const GLshort *)ptr)[0], ((const GLshort *)ptr)[1], 0, 1};
                    GL_EXTCALL(glVertexAttrib4Nsv(i, s));
                    break;
                }
                case WINED3DFMT_R16G16_UNORM:
                {
                    const GLushort s[4] = {((const GLushort *)ptr)[0], ((const GLushort *)ptr)[1], 0, 1};
                    GL_EXTCALL(glVertexAttrib4Nusv(i, s));
                    break;
                }
                case WINED3DFMT_R16G16B16A16_SNORM:
                    GL_EXTCALL(glVertexAttrib4Nsv(i, (const GLshort *)ptr));
                    break;
                case WINED3DFMT_R16G16B16A16_UNORM:
                    GL_EXTCALL(glVertexAttrib4Nusv(i, (const GLushort *)ptr));
                    break;
                case WINED3DFMT_R10G10B10X2_UINT:
                    FIXME("Unsure about WINED3DDECLTYPE_UDEC3.\n");
                    /*glVertexAttrib3usvARB(i, (const GLushort *)ptr); Does not exist */
                    break;
                case WINED3DFMT_R10G10B10X2_SNORM:
                    FIXME("Unsure about WINED3DDECLTYPE_DEC3N.\n");
                    /*glVertexAttrib3NusvARB(i, (const GLushort *)ptr); Does not exist */
                    break;
                case WINED3DFMT_R16G16_FLOAT:
                    if (gl_info->supported[NV_HALF_FLOAT] && gl_info->supported[NV_VERTEX_PROGRAM])
                    {
                        /* Not supported by GL_ARB_half_float_vertex. */
                        GL_EXTCALL(glVertexAttrib2hvNV(i, (const GLhalfNV *)ptr));
                    }
                    else
                    {
                        float x = float_16_to_32(((const unsigned short *)ptr) + 0);
                        float y = float_16_to_32(((const unsigned short *)ptr) + 1);
                        GL_EXTCALL(glVertexAttrib2f(i, x, y));
                    }
                    break;
                case WINED3DFMT_R16G16B16A16_FLOAT:
                    if (gl_info->supported[NV_HALF_FLOAT] && gl_info->supported[NV_VERTEX_PROGRAM])
                    {
                        /* Not supported by GL_ARB_half_float_vertex. */
                        GL_EXTCALL(glVertexAttrib4hvNV(i, (const GLhalfNV *)ptr));
                    }
                    else
                    {
                        float x = float_16_to_32(((const unsigned short *)ptr) + 0);
                        float y = float_16_to_32(((const unsigned short *)ptr) + 1);
                        float z = float_16_to_32(((const unsigned short *)ptr) + 2);
                        float w = float_16_to_32(((const unsigned short *)ptr) + 3);
                        GL_EXTCALL(glVertexAttrib4f(i, x, y, z, w));
                    }
                    break;
                default:
                    ERR("Unexpected declaration in stride 0 attributes.\n");
                    break;

            }
            context_gl->default_attrib_value_set &= ~(1u << i);
        }
    }
    checkGLcall("Loading numbered arrays");
}

void wined3d_context_gl_update_stream_sources(struct wined3d_context_gl *context_gl,
        const struct wined3d_state *state)
{
    wined3d_context_gl_load_numbered_arrays(context_gl, &context_gl->c.stream_info, state);
}

static void apply_texture_blit_state(const struct wined3d_gl_info *gl_info, struct gl_texture *texture,
        GLenum target, unsigned int level, enum wined3d_texture_filter_type filter)
{
    gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_MAG_FILTER, wined3d_gl_mag_filter(filter));
    gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_MIN_FILTER,
            wined3d_gl_min_mip_filter(filter, WINED3D_TEXF_NONE));
    gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    if (gl_info->supported[EXT_TEXTURE_SRGB_DECODE])
        gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_SRGB_DECODE_EXT, GL_SKIP_DECODE_EXT);
    gl_info->gl_ops.gl.p_glTexParameteri(target, GL_TEXTURE_BASE_LEVEL, level);

    /* We changed the filtering settings on the texture. Make sure they get
     * reset on subsequent draws. */
    texture->sampler_desc.mag_filter = WINED3D_TEXF_POINT;
    texture->sampler_desc.min_filter = WINED3D_TEXF_POINT;
    texture->sampler_desc.mip_filter = WINED3D_TEXF_NONE;
    texture->sampler_desc.address_u = WINED3D_TADDRESS_CLAMP;
    texture->sampler_desc.address_v = WINED3D_TADDRESS_CLAMP;
    texture->sampler_desc.srgb_decode = FALSE;
    texture->sampler_desc.mip_base_level = level;
}

/* Context activation is done by the caller. */
void wined3d_context_gl_draw_shaded_quad(struct wined3d_context_gl *context_gl, struct wined3d_texture_gl *texture_gl,
        unsigned int sub_resource_idx, const RECT *src_rect, const RECT *dst_rect,
        enum wined3d_texture_filter_type filter)
{
    const struct wined3d_gl_info *gl_info = context_gl->gl_info;
    struct wined3d_blt_info info;
    unsigned int level, w, h, i;
    SIZE dst_size;
    struct blit_vertex
    {
        float x, y;
        struct wined3d_vec3 texcoord;
    }
    quad[4];

    texture2d_get_blt_info(texture_gl, sub_resource_idx, src_rect, &info);

    level = sub_resource_idx % texture_gl->t.level_count;
    wined3d_context_gl_bind_texture(context_gl, info.bind_target, texture_gl->texture_rgb.name);
    apply_texture_blit_state(gl_info, &texture_gl->texture_rgb, info.bind_target, level, filter);
    gl_info->gl_ops.gl.p_glTexParameteri(info.bind_target, GL_TEXTURE_MAX_LEVEL, level);

    wined3d_context_gl_pause_transform_feedback(context_gl, FALSE);

    wined3d_context_gl_get_rt_size(context_gl, &dst_size);
    w = dst_size.cx;
    h = dst_size.cy;

    quad[0].x = dst_rect->left * 2.0f / w - 1.0f;
    quad[0].y = dst_rect->top * 2.0f / h - 1.0f;
    quad[0].texcoord = info.texcoords[0];

    quad[1].x = dst_rect->right * 2.0f / w - 1.0f;
    quad[1].y = dst_rect->top * 2.0f / h - 1.0f;
    quad[1].texcoord = info.texcoords[1];

    quad[2].x = dst_rect->left * 2.0f / w - 1.0f;
    quad[2].y = dst_rect->bottom * 2.0f / h - 1.0f;
    quad[2].texcoord = info.texcoords[2];

    quad[3].x = dst_rect->right * 2.0f / w - 1.0f;
    quad[3].y = dst_rect->bottom * 2.0f / h - 1.0f;
    quad[3].texcoord = info.texcoords[3];

    /* Draw a quad. */
    if (gl_info->supported[ARB_VERTEX_BUFFER_OBJECT])
    {
        if (!context_gl->blit_vbo)
            GL_EXTCALL(glGenBuffers(1, &context_gl->blit_vbo));
        GL_EXTCALL(glBindBuffer(GL_ARRAY_BUFFER, context_gl->blit_vbo));

        wined3d_context_gl_unload_numbered_arrays(context_gl);

        GL_EXTCALL(glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STREAM_DRAW));
        GL_EXTCALL(glVertexAttribPointer(0, 2, GL_FLOAT, FALSE, sizeof(*quad), NULL));
        GL_EXTCALL(glVertexAttribPointer(1, 3, GL_FLOAT, FALSE, sizeof(*quad),
                (void *)FIELD_OFFSET(struct blit_vertex, texcoord)));

        GL_EXTCALL(glEnableVertexAttribArray(0));
        GL_EXTCALL(glEnableVertexAttribArray(1));

        gl_info->gl_ops.gl.p_glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        GL_EXTCALL(glBindBuffer(GL_ARRAY_BUFFER, 0));
        GL_EXTCALL(glDisableVertexAttribArray(1));
        GL_EXTCALL(glDisableVertexAttribArray(0));
    }
    else
    {
        gl_info->gl_ops.gl.p_glBegin(GL_TRIANGLE_STRIP);

        for (i = 0; i < ARRAY_SIZE(quad); ++i)
        {
            GL_EXTCALL(glVertexAttrib3fv(1, &quad[i].texcoord.x));
            GL_EXTCALL(glVertexAttrib2fv(0, &quad[i].x));
        }

        gl_info->gl_ops.gl.p_glEnd();
    }
    checkGLcall("draw");

    gl_info->gl_ops.gl.p_glTexParameteri(info.bind_target, GL_TEXTURE_MAX_LEVEL, texture_gl->t.level_count - 1);
    wined3d_context_gl_bind_texture(context_gl, info.bind_target, 0);
}
