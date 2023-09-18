/*
 * Compatibility functions for older GL implementations
 *
 * Copyright 2008 Stefan Dösinger for CodeWeavers
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
#include "wine/port.h"

#include <stdio.h>
#ifdef HAVE_FLOAT_H
# include <float.h>
#endif

#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(gl_compat);
WINE_DECLARE_DEBUG_CHANNEL(d3d_perf);

/* Start GL_ARB_multitexture emulation */
static void WINE_GLAPI wine_glMultiTexCoord1fARB(GLenum target, GLfloat s)
{
    if (target != GL_TEXTURE0)
    {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported.\n");
        return;
    }
    wined3d_context_gl_get_current()->gl_info->gl_ops.gl.p_glTexCoord1f(s);
}

static void WINE_GLAPI wine_glMultiTexCoord1fvARB(GLenum target, const GLfloat *v)
{
    if (target != GL_TEXTURE0)
    {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported.\n");
        return;
    }
    wined3d_context_gl_get_current()->gl_info->gl_ops.gl.p_glTexCoord1fv(v);
}

static void WINE_GLAPI wine_glMultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t)
{
    if (target != GL_TEXTURE0)
    {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported.\n");
        return;
    }
    wined3d_context_gl_get_current()->gl_info->gl_ops.gl.p_glTexCoord2f(s, t);
}

static void WINE_GLAPI wine_glMultiTexCoord2fvARB(GLenum target, const GLfloat *v)
{
    if (target != GL_TEXTURE0)
    {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported.\n");
        return;
    }
    wined3d_context_gl_get_current()->gl_info->gl_ops.gl.p_glTexCoord2fv(v);
}

static void WINE_GLAPI wine_glMultiTexCoord3fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r)
{
    if (target != GL_TEXTURE0)
    {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported.\n");
        return;
    }
    wined3d_context_gl_get_current()->gl_info->gl_ops.gl.p_glTexCoord3f(s, t, r);
}

static void WINE_GLAPI wine_glMultiTexCoord3fvARB(GLenum target, const GLfloat *v)
{
    if (target != GL_TEXTURE0)
    {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported.\n");
        return;
    }
    wined3d_context_gl_get_current()->gl_info->gl_ops.gl.p_glTexCoord3fv(v);
}

static void WINE_GLAPI wine_glMultiTexCoord4fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    if (target != GL_TEXTURE0)
    {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported.\n");
        return;
    }
    wined3d_context_gl_get_current()->gl_info->gl_ops.gl.p_glTexCoord4f(s, t, r, q);
}

static void WINE_GLAPI wine_glMultiTexCoord4fvARB(GLenum target, const GLfloat *v)
{
    if (target != GL_TEXTURE0)
    {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported.\n");
        return;
    }
    wined3d_context_gl_get_current()->gl_info->gl_ops.gl.p_glTexCoord4fv(v);
}

static void WINE_GLAPI wine_glMultiTexCoord2svARB(GLenum target, const GLshort *v)
{
    if (target != GL_TEXTURE0)
    {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported.\n");
        return;
    }
    wined3d_context_gl_get_current()->gl_info->gl_ops.gl.p_glTexCoord2sv(v);
}

static void WINE_GLAPI wine_glMultiTexCoord4svARB(GLenum target, const GLshort *v)
{
    if (target != GL_TEXTURE0)
    {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported.\n");
        return;
    }
    wined3d_context_gl_get_current()->gl_info->gl_ops.gl.p_glTexCoord4sv(v);
}

static void WINE_GLAPI wine_glActiveTexture(GLenum texture)
{
    if(texture != GL_TEXTURE0) {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported\n");
        return;
    }
}

static void WINE_GLAPI wine_glClientActiveTextureARB(GLenum texture) {
    if(texture != GL_TEXTURE0) {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported\n");
        return;
    }
}

static void (WINE_GLAPI *old_multitex_glGetIntegerv) (GLenum pname, GLint* params) = NULL;
static void WINE_GLAPI wine_glGetIntegerv(GLenum pname, GLint* params) {
    switch(pname) {
        case GL_ACTIVE_TEXTURE:         *params = 0;    break;
        case GL_MAX_TEXTURE_UNITS_ARB:  *params = 1;    break;
        default: old_multitex_glGetIntegerv(pname, params);
    }
}

static void (WINE_GLAPI *old_multitex_glGetFloatv) (GLenum pname, GLfloat* params) = NULL;
static void WINE_GLAPI wine_glGetFloatv(GLenum pname, GLfloat* params) {
    if (pname == GL_ACTIVE_TEXTURE) *params = 0.0f;
    else old_multitex_glGetFloatv(pname, params);
}

static void (WINE_GLAPI *old_multitex_glGetDoublev) (GLenum pname, GLdouble* params) = NULL;
static void WINE_GLAPI wine_glGetDoublev(GLenum pname, GLdouble* params) {
    if(pname == GL_ACTIVE_TEXTURE) *params = 0.0;
    else old_multitex_glGetDoublev(pname, params);
}

/* Start GL_EXT_fogcoord emulation */
static void (WINE_GLAPI *old_fogcoord_glEnable)(GLenum cap);
static void WINE_GLAPI wine_glEnable(GLenum cap)
{
    if (cap == GL_FOG)
    {
        struct wined3d_context_gl *ctx = wined3d_context_gl_get_current();

        ctx->fog_enabled = 1;
        if (ctx->gl_fog_source != GL_FRAGMENT_DEPTH_EXT)
            return;
    }
    old_fogcoord_glEnable(cap);
}

static void (WINE_GLAPI *old_fogcoord_glDisable)(GLenum cap);
static void WINE_GLAPI wine_glDisable(GLenum cap)
{
    if (cap == GL_FOG)
    {
        struct wined3d_context_gl *ctx = wined3d_context_gl_get_current();

        ctx->fog_enabled = 0;
        if (ctx->gl_fog_source != GL_FRAGMENT_DEPTH_EXT)
            return;
    }
    old_fogcoord_glDisable(cap);
}

static void (WINE_GLAPI *old_fogcoord_glFogi)(GLenum pname, GLint param);
static void WINE_GLAPI wine_glFogi(GLenum pname, GLint param)
{
    struct wined3d_context_gl *ctx = wined3d_context_gl_get_current();

    if (pname == GL_FOG_COORDINATE_SOURCE_EXT)
    {
        ctx->gl_fog_source = param;
        if (param == GL_FRAGMENT_DEPTH_EXT)
        {
            if (ctx->fog_enabled)
                old_fogcoord_glEnable(GL_FOG);
        }
        else
        {
            WARN_(d3d_perf)("Fog coordinates activated, but not supported. Using slow emulation.\n");
            old_fogcoord_glDisable(GL_FOG);
        }
    }
    else
    {
        if (pname == GL_FOG_START)
            ctx->fog_start = (float)param;
        else if (pname == GL_FOG_END)
            ctx->fog_end = (float)param;
        old_fogcoord_glFogi(pname, param);
    }
}

static void (WINE_GLAPI *old_fogcoord_glFogiv)(GLenum pname, const GLint *param);
static void WINE_GLAPI wine_glFogiv(GLenum pname, const GLint *param)
{
    struct wined3d_context_gl *ctx = wined3d_context_gl_get_current();

    if (pname == GL_FOG_COORDINATE_SOURCE_EXT)
    {
        ctx->gl_fog_source = *param;
        if (*param == GL_FRAGMENT_DEPTH_EXT)
        {
            if (ctx->fog_enabled)
                old_fogcoord_glEnable(GL_FOG);
        }
        else
        {
            WARN_(d3d_perf)("Fog coordinates activated, but not supported. Using slow emulation.\n");
            old_fogcoord_glDisable(GL_FOG);
        }
    }
    else
    {
        if (pname == GL_FOG_START)
            ctx->fog_start = (float)*param;
        else if (pname == GL_FOG_END)
            ctx->fog_end = (float)*param;
        old_fogcoord_glFogiv(pname, param);
    }
}

static void (WINE_GLAPI *old_fogcoord_glFogf)(GLenum pname, GLfloat param);
static void WINE_GLAPI wine_glFogf(GLenum pname, GLfloat param)
{
    struct wined3d_context_gl *ctx = wined3d_context_gl_get_current();

    if (pname == GL_FOG_COORDINATE_SOURCE_EXT)
    {
        ctx->gl_fog_source = (GLint)param;
        if (param == GL_FRAGMENT_DEPTH_EXT)
        {
            if (ctx->fog_enabled)
                old_fogcoord_glEnable(GL_FOG);
        }
        else
        {
            WARN_(d3d_perf)("Fog coordinates activated, but not supported. Using slow emulation.\n");
            old_fogcoord_glDisable(GL_FOG);
        }
    }
    else
    {
        if (pname == GL_FOG_START)
            ctx->fog_start = param;
        else if (pname == GL_FOG_END)
            ctx->fog_end = param;
        old_fogcoord_glFogf(pname, param);
    }
}

static void (WINE_GLAPI *old_fogcoord_glFogfv)(GLenum pname, const GLfloat *param);
static void WINE_GLAPI wine_glFogfv(GLenum pname, const GLfloat *param)
{
    struct wined3d_context_gl *ctx = wined3d_context_gl_get_current();

    if (pname == GL_FOG_COORDINATE_SOURCE_EXT)
    {
        ctx->gl_fog_source = (GLint)*param;
        if (*param == GL_FRAGMENT_DEPTH_EXT)
        {
            if (ctx->fog_enabled)
                old_fogcoord_glEnable(GL_FOG);
        }
        else
        {
            WARN_(d3d_perf)("Fog coordinates activated, but not supported. Using slow emulation.\n");
            old_fogcoord_glDisable(GL_FOG);
        }
    }
    else
    {
        if (pname == GL_FOG_COLOR)
        {
            ctx->fog_colour[0] = param[0];
            ctx->fog_colour[1] = param[1];
            ctx->fog_colour[2] = param[2];
            ctx->fog_colour[3] = param[3];
        }
        else if (pname == GL_FOG_START)
        {
            ctx->fog_start = *param;
        }
        else if (pname == GL_FOG_END)
        {
            ctx->fog_end = *param;
        }
        old_fogcoord_glFogfv(pname, param);
    }
}

static void (WINE_GLAPI *old_fogcoord_glVertex4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
static void (WINE_GLAPI *old_fogcoord_glColor4f)(GLfloat r, GLfloat g, GLfloat b, GLfloat a);

static void WINE_GLAPI wine_glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    const struct wined3d_context_gl *ctx_gl = wined3d_context_gl_get_current();

    /* This can be called from draw_test_quad() and at that point there is no
     * wined3d_context current. */
    if (!ctx_gl)
    {
        old_fogcoord_glVertex4f(x, y, z, w);
        return;
    }

    if (ctx_gl->gl_fog_source == GL_FOG_COORDINATE_EXT && ctx_gl->fog_enabled)
    {
        GLfloat c[4] = {ctx_gl->colour[0], ctx_gl->colour[1], ctx_gl->colour[2], ctx_gl->colour[3]};
        GLfloat i;

        i = (ctx_gl->fog_end - ctx_gl->fog_coord_value) / (ctx_gl->fog_end - ctx_gl->fog_start);
        c[0] = i * c[0] + (1.0f - i) * ctx_gl->fog_colour[0];
        c[1] = i * c[1] + (1.0f - i) * ctx_gl->fog_colour[1];
        c[2] = i * c[2] + (1.0f - i) * ctx_gl->fog_colour[2];

        old_fogcoord_glColor4f(c[0], c[1], c[2], c[3]);
        old_fogcoord_glVertex4f(x, y, z, w);
    }
    else
    {
        old_fogcoord_glVertex4f(x, y, z, w);
    }
}

static void WINE_GLAPI wine_glVertex4fv(const GLfloat *pos) {
    wine_glVertex4f(pos[0], pos[1], pos[2], pos[3]);
}

static void WINE_GLAPI wine_glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
    wine_glVertex4f(x, y, z, 1.0f);
}

static void WINE_GLAPI wine_glVertex3fv(const GLfloat *pos) {
    wine_glVertex4f(pos[0], pos[1], pos[2], 1.0f);
}

static void WINE_GLAPI wine_glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
    struct wined3d_context_gl *ctx_gl = wined3d_context_gl_get_current();

    /* This can be called from draw_test_quad() and at that point there is no
     * wined3d_context current. */
    if (!ctx_gl)
    {
        old_fogcoord_glColor4f(r, g, b, a);
        return;
    }

    ctx_gl->colour[0] = r;
    ctx_gl->colour[1] = g;
    ctx_gl->colour[2] = b;
    ctx_gl->colour[3] = a;
    old_fogcoord_glColor4f(r, g, b, a);
}

static void WINE_GLAPI wine_glColor4fv(const GLfloat *c) {
    wine_glColor4f(c[0], c[1], c[2], c[3]);
}

static void WINE_GLAPI wine_glColor3f(GLfloat r, GLfloat g, GLfloat b) {
    wine_glColor4f(r, g, b, 1.0f);
}

static void WINE_GLAPI wine_glColor3fv(const GLfloat *c) {
    wine_glColor4f(c[0], c[1], c[2], 1.0f);
}

static void WINE_GLAPI wine_glColor4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a) {
    wine_glColor4f(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
}

/* In D3D the fog coord is a UBYTE, so there's no problem with using the
 * single precision function. */
static void WINE_GLAPI wine_glFogCoordfEXT(GLfloat f)
{
    struct wined3d_context_gl *ctx = wined3d_context_gl_get_current();

    ctx->fog_coord_value = f;
}
static void WINE_GLAPI wine_glFogCoorddEXT(GLdouble f) {
    wine_glFogCoordfEXT((GLfloat) f);
}
static void WINE_GLAPI wine_glFogCoordfvEXT(const GLfloat *f) {
    wine_glFogCoordfEXT(*f);
}
static void WINE_GLAPI wine_glFogCoorddvEXT(const GLdouble *f) {
    wine_glFogCoordfEXT((GLfloat) *f);
}

/* End GL_EXT_fog_coord emulation */

void install_gl_compat_wrapper(struct wined3d_gl_info *gl_info, enum wined3d_gl_extension ext)
{
    switch (ext)
    {
        case ARB_MULTITEXTURE:
            if (gl_info->supported[ARB_MULTITEXTURE])
                return;
            if (gl_info->gl_ops.ext.p_glActiveTexture == wine_glActiveTexture)
            {
                FIXME("ARB_multitexture emulation hooks already applied.\n");
                return;
            }
            TRACE("Applying GL_ARB_multitexture emulation hooks.\n");
            gl_info->gl_ops.ext.p_glActiveTexture           = wine_glActiveTexture;
            gl_info->gl_ops.ext.p_glClientActiveTextureARB  = wine_glClientActiveTextureARB;
            gl_info->gl_ops.ext.p_glMultiTexCoord1fARB      = wine_glMultiTexCoord1fARB;
            gl_info->gl_ops.ext.p_glMultiTexCoord1fvARB     = wine_glMultiTexCoord1fvARB;
            gl_info->gl_ops.ext.p_glMultiTexCoord2fARB      = wine_glMultiTexCoord2fARB;
            gl_info->gl_ops.ext.p_glMultiTexCoord2fvARB     = wine_glMultiTexCoord2fvARB;
            gl_info->gl_ops.ext.p_glMultiTexCoord3fARB      = wine_glMultiTexCoord3fARB;
            gl_info->gl_ops.ext.p_glMultiTexCoord3fvARB     = wine_glMultiTexCoord3fvARB;
            gl_info->gl_ops.ext.p_glMultiTexCoord4fARB      = wine_glMultiTexCoord4fARB;
            gl_info->gl_ops.ext.p_glMultiTexCoord4fvARB     = wine_glMultiTexCoord4fvARB;
            gl_info->gl_ops.ext.p_glMultiTexCoord2svARB     = wine_glMultiTexCoord2svARB;
            gl_info->gl_ops.ext.p_glMultiTexCoord4svARB     = wine_glMultiTexCoord4svARB;
            old_multitex_glGetIntegerv = gl_info->gl_ops.gl.p_glGetIntegerv;
            gl_info->gl_ops.gl.p_glGetIntegerv = wine_glGetIntegerv;
            old_multitex_glGetFloatv = gl_info->gl_ops.gl.p_glGetFloatv;
            gl_info->gl_ops.gl.p_glGetFloatv = wine_glGetFloatv;
            old_multitex_glGetDoublev = gl_info->gl_ops.gl.p_glGetDoublev;
            gl_info->gl_ops.gl.p_glGetDoublev = wine_glGetDoublev;
            gl_info->supported[ARB_MULTITEXTURE] = TRUE;
            return;

        case EXT_FOG_COORD:
            /* This emulation isn't perfect. There are a number of potential problems, but they should
             * not matter in practise:
             *
             * Fog vs fragment shader: If we are using GL_ARB_fragment_program with the fog option, the
             * glDisable(GL_FOG) here won't matter. However, if we have GL_ARB_fragment_program, it is pretty
             * unlikely that we don't have GL_EXT_fog_coord. Besides, we probably have GL_ARB_vertex_program
             * too, which would allow fog coord emulation in a fixed function vertex pipeline replacement.
             *
             * Fog vs texture: We apply the fog in the vertex color. An app could set up texturing settings which
             * ignore the vertex color, thus effectively disabling our fog. However, in D3D this type of fog is
             * a per-vertex fog too, so the apps shouldn't do that.
             *
             * Fog vs lighting: The app could in theory use D3DFOG_NONE table and D3DFOG_NONE vertex fog with
             * untransformed vertices. That enables lighting and fog coords at the same time, and the lighting
             * calculations could affect the already blended in fog color. There's nothing we can do against that,
             * but most apps using fog color do their own lighting too and often even use RHW vertices. So live
             * with it.
             */
            if (gl_info->supported[EXT_FOG_COORD])
                return;
            if (gl_info->gl_ops.gl.p_glFogi == wine_glFogi)
            {
                FIXME("EXT_fog_coord emulation hooks already applied.\n");
                return;
            }
            TRACE("Applying GL_ARB_fog_coord emulation hooks\n");

            /* This probably means that the implementation doesn't advertise the extension, but implicitly supports
             * it via the GL core version, or someone messed around in the extension table in directx.c. Add version-
             * dependent loading for this extension if we ever hit this situation
             */
            if (gl_info->supported[ARB_FRAGMENT_PROGRAM])
            {
                FIXME("GL implementation supports GL_ARB_fragment_program but not GL_EXT_fog_coord\n");
                FIXME("The fog coord emulation will most likely fail\n");
            }
            else if (gl_info->supported[ARB_FRAGMENT_SHADER])
            {
                FIXME("GL implementation supports GL_ARB_fragment_shader but not GL_EXT_fog_coord\n");
                FIXME("The fog coord emulation will most likely fail\n");
            }

            old_fogcoord_glFogi = gl_info->gl_ops.gl.p_glFogi;
            gl_info->gl_ops.gl.p_glFogi = wine_glFogi;
            old_fogcoord_glFogiv = gl_info->gl_ops.gl.p_glFogiv;
            gl_info->gl_ops.gl.p_glFogiv = wine_glFogiv;
            old_fogcoord_glFogf = gl_info->gl_ops.gl.p_glFogf;
            gl_info->gl_ops.gl.p_glFogf = wine_glFogf;
            old_fogcoord_glFogfv = gl_info->gl_ops.gl.p_glFogfv;
            gl_info->gl_ops.gl.p_glFogfv = wine_glFogfv;
            old_fogcoord_glEnable = gl_info->p_glEnableWINE;
            gl_info->p_glEnableWINE = wine_glEnable;
            old_fogcoord_glDisable = gl_info->p_glDisableWINE;
            gl_info->p_glDisableWINE = wine_glDisable;

            old_fogcoord_glVertex4f = gl_info->gl_ops.gl.p_glVertex4f;
            gl_info->gl_ops.gl.p_glVertex4f = wine_glVertex4f;
            gl_info->gl_ops.gl.p_glVertex4fv = wine_glVertex4fv;
            gl_info->gl_ops.gl.p_glVertex3f = wine_glVertex3f;
            gl_info->gl_ops.gl.p_glVertex3fv = wine_glVertex3fv;

            old_fogcoord_glColor4f = gl_info->gl_ops.gl.p_glColor4f;
            gl_info->gl_ops.gl.p_glColor4f = wine_glColor4f;
            gl_info->gl_ops.gl.p_glColor4fv = wine_glColor4fv;
            gl_info->gl_ops.gl.p_glColor3f = wine_glColor3f;
            gl_info->gl_ops.gl.p_glColor3fv = wine_glColor3fv;
            gl_info->gl_ops.gl.p_glColor4ub = wine_glColor4ub;

            gl_info->gl_ops.ext.p_glFogCoordfEXT = wine_glFogCoordfEXT;
            gl_info->gl_ops.ext.p_glFogCoordfvEXT = wine_glFogCoordfvEXT;
            gl_info->gl_ops.ext.p_glFogCoorddEXT = wine_glFogCoorddEXT;
            gl_info->gl_ops.ext.p_glFogCoorddvEXT = wine_glFogCoorddvEXT;
            gl_info->supported[EXT_FOG_COORD] = TRUE;
            return;

        default:
            FIXME("Extension %u emulation not supported.\n", ext);
    }
}
