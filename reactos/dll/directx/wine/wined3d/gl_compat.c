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
#include <stdio.h>
#ifdef HAVE_FLOAT_H
# include <float.h>
#endif
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(gl_compat);

/* Start GL_ARB_multitexture emulation */
static void WINE_GLAPI wine_glMultiTexCoord1fARB(GLenum target, GLfloat s) {
    if(target != GL_TEXTURE0) {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported\n");
        return;
    }
    glTexCoord1f(s);
}

static void WINE_GLAPI wine_glMultiTexCoord1fvARB(GLenum target, const GLfloat *v) {
    if(target != GL_TEXTURE0) {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported\n");
        return;
    }
    glTexCoord1fv(v);
}

static void WINE_GLAPI wine_glMultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t) {
    if(target != GL_TEXTURE0) {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported\n");
        return;
    }
    glTexCoord2f(s, t);
}

static void WINE_GLAPI wine_glMultiTexCoord2fvARB(GLenum target, const GLfloat *v) {
    if(target != GL_TEXTURE0) {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported\n");
        return;
    }
    glTexCoord2fv(v);
}

static void WINE_GLAPI wine_glMultiTexCoord3fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r) {
    if(target != GL_TEXTURE0) {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported\n");
        return;
    }
    glTexCoord3f(s, t, r);
}

static void WINE_GLAPI wine_glMultiTexCoord3fvARB(GLenum target, const GLfloat *v) {
    if(target != GL_TEXTURE0) {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported\n");
        return;
    }
    glTexCoord3fv(v);
}

static void WINE_GLAPI wine_glMultiTexCoord4fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q) {
    if(target != GL_TEXTURE0) {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported\n");
        return;
    }
    glTexCoord4f(s, t, r, q);
}

static void WINE_GLAPI wine_glMultiTexCoord4fvARB(GLenum target, const GLfloat *v) {
    if(target != GL_TEXTURE0) {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported\n");
        return;
    }
    glTexCoord4fv(v);
}

static void WINE_GLAPI wine_glMultiTexCoord2svARB(GLenum target, const GLshort *v) {
    if(target != GL_TEXTURE0) {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported\n");
        return;
    }
    glTexCoord2sv(v);
}

static void WINE_GLAPI wine_glMultiTexCoord4svARB(GLenum target, const GLshort *v) {
    if(target != GL_TEXTURE0) {
        ERR("Texture unit > 0 used, but GL_ARB_multitexture is not supported\n");
        return;
    }
    glTexCoord4sv(v);
}

static void WINE_GLAPI wine_glActiveTextureARB(GLenum texture) {
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
    if(pname == GL_ACTIVE_TEXTURE) *params = 0.0;
    else old_multitex_glGetFloatv(pname, params);
}

static void (WINE_GLAPI *old_multitex_glGetDoublev) (GLenum pname, GLdouble* params) = NULL;
static void WINE_GLAPI wine_glGetDoublev(GLenum pname, GLdouble* params) {
    if(pname == GL_ACTIVE_TEXTURE) *params = 0.0;
    else old_multitex_glGetDoublev(pname, params);
}

/* Start GL_EXT_fogcoord emulation */
static void (WINE_GLAPI *old_fogcoord_glEnable) (GLenum cap) = NULL;
static void WINE_GLAPI wine_glEnable(GLenum cap) {
    if(cap == GL_FOG) {
        WineD3DContext *ctx = getActiveContext();
        ctx->fog_enabled = 1;
        if(ctx->gl_fog_source != GL_FRAGMENT_DEPTH_EXT) return;
    }
    old_fogcoord_glEnable(cap);
}

static void (WINE_GLAPI *old_fogcoord_glDisable) (GLenum cap) = NULL;
static void WINE_GLAPI wine_glDisable(GLenum cap) {
    if(cap == GL_FOG) {
        WineD3DContext *ctx = getActiveContext();
        ctx->fog_enabled = 0;
        if(ctx->gl_fog_source != GL_FRAGMENT_DEPTH_EXT) return;
    }
    old_fogcoord_glDisable(cap);
}

static void (WINE_GLAPI *old_fogcoord_glFogi) (GLenum pname, GLint param) = NULL;
static void WINE_GLAPI wine_glFogi(GLenum pname, GLint param) {
    if(pname == GL_FOG_COORDINATE_SOURCE_EXT) {
        WineD3DContext *ctx = getActiveContext();
        ctx->gl_fog_source = param;
        if(param == GL_FRAGMENT_DEPTH_EXT) {
            if(ctx->fog_enabled) old_fogcoord_glEnable(GL_FOG);
        } else {
            WARN("Fog coords activated, but not supported. Using slow emulation\n");
            old_fogcoord_glDisable(GL_FOG);
        }
    } else {
        if(pname == GL_FOG_START) {
            getActiveContext()->fogstart = param;
        } else if(pname == GL_FOG_END) {
            getActiveContext()->fogend = param;
        }
        old_fogcoord_glFogi(pname, param);
    }
}

static void (WINE_GLAPI *old_fogcoord_glFogiv) (GLenum pname, const GLint *param) = NULL;
static void WINE_GLAPI wine_glFogiv(GLenum pname, const GLint *param) {
    if(pname == GL_FOG_COORDINATE_SOURCE_EXT) {
        WineD3DContext *ctx = getActiveContext();
        ctx->gl_fog_source = *param;
        if(*param == GL_FRAGMENT_DEPTH_EXT) {
            if(ctx->fog_enabled) old_fogcoord_glEnable(GL_FOG);
        } else {
            WARN("Fog coords activated, but not supported. Using slow emulation\n");
            old_fogcoord_glDisable(GL_FOG);
        }
    } else {
        if(pname == GL_FOG_START) {
            getActiveContext()->fogstart = *param;
        } else if(pname == GL_FOG_END) {
            getActiveContext()->fogend = *param;
        }
        old_fogcoord_glFogiv(pname, param);
    }
}

static void (WINE_GLAPI *old_fogcoord_glFogf) (GLenum pname, GLfloat param) = NULL;
static void WINE_GLAPI wine_glFogf(GLenum pname, GLfloat param) {
    if(pname == GL_FOG_COORDINATE_SOURCE_EXT) {
        WineD3DContext *ctx = getActiveContext();
        ctx->gl_fog_source = (GLint) param;
        if(param == GL_FRAGMENT_DEPTH_EXT) {
            if(ctx->fog_enabled) old_fogcoord_glEnable(GL_FOG);
        } else {
            WARN("Fog coords activated, but not supported. Using slow emulation\n");
            old_fogcoord_glDisable(GL_FOG);
        }
    } else {
        if(pname == GL_FOG_START) {
            getActiveContext()->fogstart = param;
        } else if(pname == GL_FOG_END) {
            getActiveContext()->fogend = param;
        }
        old_fogcoord_glFogf(pname, param);
    }
}

static void (WINE_GLAPI *old_fogcoord_glFogfv) (GLenum pname, const GLfloat *param) = NULL;
static void WINE_GLAPI wine_glFogfv(GLenum pname, const GLfloat *param) {
    if(pname == GL_FOG_COORDINATE_SOURCE_EXT) {
        WineD3DContext *ctx = getActiveContext();
        ctx->gl_fog_source = (GLint) *param;
        if(*param == GL_FRAGMENT_DEPTH_EXT) {
            if(ctx->fog_enabled) old_fogcoord_glEnable(GL_FOG);
        } else {
            WARN("Fog coords activated, but not supported. Using slow emulation\n");
            old_fogcoord_glDisable(GL_FOG);
        }
    } else {
        if(pname == GL_FOG_COLOR) {
            WineD3DContext *ctx = getActiveContext();
            ctx->fogcolor[0] = param[0];
            ctx->fogcolor[1] = param[1];
            ctx->fogcolor[2] = param[2];
            ctx->fogcolor[3] = param[3];
        } else if(pname == GL_FOG_START) {
            getActiveContext()->fogstart = *param;
        } else if(pname == GL_FOG_END) {
            getActiveContext()->fogend = *param;
        }
        old_fogcoord_glFogfv(pname, param);
    }
}

static void (WINE_GLAPI *old_fogcoord_glVertex4f) (GLfloat x, GLfloat y, GLfloat z, GLfloat w) = NULL;
static void (WINE_GLAPI *old_fogcoord_glVertex4fv) (const GLfloat *pos) = NULL;
static void (WINE_GLAPI *old_fogcoord_glVertex3f) (GLfloat x, GLfloat y, GLfloat z) = NULL;
static void (WINE_GLAPI *old_fogcoord_glVertex3fv) (const GLfloat *pos) = NULL;
static void (WINE_GLAPI *old_fogcoord_glColor4f) (GLfloat r, GLfloat g, GLfloat b, GLfloat a) = NULL;
static void (WINE_GLAPI *old_fogcoord_glColor4fv) (const GLfloat *color) = NULL;
static void (WINE_GLAPI *old_fogcoord_glColor3f) (GLfloat r, GLfloat g, GLfloat b) = NULL;
static void (WINE_GLAPI *old_fogcoord_glColor3fv) (const GLfloat *color) = NULL;
static void (WINE_GLAPI *old_fogcoord_glColor4ub) (GLubyte r, GLubyte g, GLubyte b, GLubyte a) = NULL;
static void (WINE_GLAPI *old_fogcoord_glFogCoordfEXT) (GLfloat f) = NULL;
static void (WINE_GLAPI *old_fogcoord_glFogCoorddEXT) (GLdouble f) = NULL;
static void (WINE_GLAPI *old_fogcoord_glFogCoordfvEXT) (const GLfloat *f) = NULL;
static void (WINE_GLAPI *old_fogcoord_glFogCoorddvEXT) (const GLdouble *f) = NULL;

static void WINE_GLAPI wine_glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w) {
    WineD3DContext *ctx = getActiveContext();
    if(ctx->gl_fog_source == GL_FOG_COORDINATE_EXT && ctx->fog_enabled) {
        GLfloat c[4] = {ctx->color[0], ctx->color[1], ctx->color[2], ctx->color[3]};
        GLfloat i;

        i = (ctx->fogend - ctx->fog_coord_value) / (ctx->fogend - ctx->fogstart);
        c[0] = i * c[0] + (1.0 - i) * ctx->fogcolor[0];
        c[1] = i * c[1] + (1.0 - i) * ctx->fogcolor[1];
        c[2] = i * c[2] + (1.0 - i) * ctx->fogcolor[2];

        old_fogcoord_glColor4f(c[0], c[1], c[2], c[3]);
        old_fogcoord_glVertex4f(x, y, z, w);
    } else {
        old_fogcoord_glVertex4f(x, y, z, w);
    }
}

static void WINE_GLAPI wine_glVertex4fv(const GLfloat *pos) {
    wine_glVertex4f(pos[0], pos[1], pos[2], pos[3]);
}

static void WINE_GLAPI wine_glVertex3f(GLfloat x, GLfloat y, GLfloat z) {
    wine_glVertex4f(x, y, z, 1.0);
}

static void WINE_GLAPI wine_glVertex3fv(const GLfloat *pos) {
    wine_glVertex4f(pos[0], pos[1], pos[2], 1.0);
}

static void wine_glColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
    WineD3DContext *ctx = getActiveContext();
    ctx->color[0] = r;
    ctx->color[1] = g;
    ctx->color[2] = b;
    ctx->color[3] = a;
    old_fogcoord_glColor4f(r, g, b, a);
}

static void wine_glColor4fv(const GLfloat *c) {
    wine_glColor4f(c[0], c[1], c[2], c[3]);
}

static void wine_glColor3f(GLfloat r, GLfloat g, GLfloat b) {
    wine_glColor4f(r, g, b, 1.0);
}

static void wine_glColor3fv(const GLfloat *c) {
    wine_glColor4f(c[0], c[1], c[2], 1.0);
}

static void wine_glColor4ub(GLubyte r, GLubyte g, GLubyte b, GLubyte a) {
    wine_glColor4f(r / 255.0, g / 255.0, b / 255.0, a / 255.0);
}

/* In D3D the fog coord is a UBYTE, so there's no problem with using the single
 * precision function
 */
static void wine_glFogCoordfEXT(GLfloat f) {
    WineD3DContext *ctx = getActiveContext();
    ctx->fog_coord_value = f;
}
static void wine_glFogCoorddEXT(GLdouble f) {
    wine_glFogCoordfEXT(f);
}
static void wine_glFogCoordfvEXT(const GLfloat *f) {
    wine_glFogCoordfEXT(*f);
}
static void wine_glFogCoorddvEXT(const GLdouble *f) {
    wine_glFogCoordfEXT(*f);
}

/* End GL_EXT_fog_coord emulation */

#define GLINFO_LOCATION (*gl_info)
void add_gl_compat_wrappers(WineD3D_GL_Info *gl_info) {
    if(!GL_SUPPORT(ARB_MULTITEXTURE)) {
        TRACE("Applying GL_ARB_multitexture emulation hooks\n");
        gl_info->glActiveTextureARB         = wine_glActiveTextureARB;
        gl_info->glClientActiveTextureARB   = wine_glClientActiveTextureARB;
        gl_info->glMultiTexCoord1fARB       = wine_glMultiTexCoord1fARB;
        gl_info->glMultiTexCoord1fvARB      = wine_glMultiTexCoord1fvARB;
        gl_info->glMultiTexCoord2fARB       = wine_glMultiTexCoord2fARB;
        gl_info->glMultiTexCoord2fvARB      = wine_glMultiTexCoord2fvARB;
        gl_info->glMultiTexCoord3fARB       = wine_glMultiTexCoord3fARB;
        gl_info->glMultiTexCoord3fvARB      = wine_glMultiTexCoord3fvARB;
        gl_info->glMultiTexCoord4fARB       = wine_glMultiTexCoord4fARB;
        gl_info->glMultiTexCoord4fvARB      = wine_glMultiTexCoord4fvARB;
        gl_info->glMultiTexCoord2svARB      = wine_glMultiTexCoord2svARB;
        gl_info->glMultiTexCoord4svARB      = wine_glMultiTexCoord4svARB;
        if(old_multitex_glGetIntegerv) {
            FIXME("GL_ARB_multitexture glGetIntegerv hook already applied\n");
        } else {
            old_multitex_glGetIntegerv = glGetIntegerv;
            glGetIntegerv = wine_glGetIntegerv;
        }
        if(old_multitex_glGetFloatv) {
            FIXME("GL_ARB_multitexture glGetGloatv hook already applied\n");
        } else {
            old_multitex_glGetFloatv = glGetFloatv;
            glGetFloatv = wine_glGetFloatv;
        }
        if(old_multitex_glGetDoublev) {
            FIXME("GL_ARB_multitexture glGetDoublev hook already applied\n");
        } else {
            old_multitex_glGetDoublev = glGetDoublev;
            glGetDoublev = wine_glGetDoublev;
        }
        gl_info->supported[ARB_MULTITEXTURE] = TRUE;
    }

    if(!GL_SUPPORT(EXT_FOG_COORD)) {
        /* This emulation isn't perfect. There are a number of potential problems, but they should
         * not matter in practise:
         *
         * Fog vs fragment shader: If we are using GL_ARB_fragment_program with the fog option, the
         * glDisable(GL_FOG) here won't matter. However, if we have GL_ARB_fragment_program, it is pretty
         * unlikely that we don't have GL_EXT_fog_coord. Besides, we probably have GL_ARB_vertex_program
         * too, which would allow fog coord emulation in a fixed function vertex pipeline replacement.
         *
         * Fog vs texture: We apply the fog in the vertex color. An app could set up texturing settings which
         * ignore the vertex color, thus effectively disabing our fog. However, in D3D this type of fog is
         * a per-vertex fog too, so the apps shouldn't do that.
         *
         * Fog vs lighting: The app could in theory use D3DFOG_NONE table and D3DFOG_NONE vertex fog with
         * untransformed vertices. That enables lighting and fog coords at the same time, and the lighting
         * calculations could affect the already blended in fog color. There's nothing we can do against that,
         * but most apps using fog color do their own lighting too and often even use RHW vertices. So live
         * with it.
         */
        TRACE("Applying GL_ARB_fog_coord emulation hooks\n");

        /* This probably means that the implementation doesn't advertise the extension, but implicitly supports
         * it via the GL core version, or someone messed around in the extension table in directx.c. Add version-
         * dependent loading for this extension if we ever hit this situation
         */
        if(GL_SUPPORT(ARB_FRAGMENT_PROGRAM)) {
            FIXME("GL implementation supports GL_ARB_fragment_program but not GL_EXT_fog_coord\n");
            FIXME("The fog coord emulation will most likely fail\n");
        } else if(GL_SUPPORT(ARB_FRAGMENT_SHADER)) {
            FIXME("GL implementation supports GL_ARB_fragment_shader but not GL_EXT_fog_coord\n");
            FIXME("The fog coord emulation will most likely fail\n");
        }

        if(old_fogcoord_glFogi) {
            FIXME("GL_EXT_fogcoord glFogi hook already applied\n");
        } else {
            old_fogcoord_glFogi = glFogi;
            glFogi = wine_glFogi;
        }
        if(old_fogcoord_glFogiv) {
            FIXME("GL_EXT_fogcoord glFogiv hook already applied\n");
        } else {
            old_fogcoord_glFogiv = glFogiv;
            glFogiv = wine_glFogiv;
        }
        if(old_fogcoord_glFogf) {
            FIXME("GL_EXT_fogcoord glFogf hook already applied\n");
        } else {
            old_fogcoord_glFogf = glFogf;
            glFogf = wine_glFogf;
        }
        if(old_fogcoord_glFogfv) {
            FIXME("GL_EXT_fogcoord glFogfv hook already applied\n");
        } else {
            old_fogcoord_glFogfv = glFogfv;
            glFogfv = wine_glFogfv;
        }
        if(old_fogcoord_glEnable) {
            FIXME("GL_EXT_fogcoord glEnable hook already applied\n");
        } else {
            old_fogcoord_glEnable = glEnableWINE;
            glEnableWINE = wine_glEnable;
        }
        if(old_fogcoord_glDisable) {
            FIXME("GL_EXT_fogcoord glDisable hook already applied\n");
        } else {
            old_fogcoord_glDisable = glDisableWINE;
            glDisableWINE = wine_glDisable;
        }

        if(old_fogcoord_glVertex4f) {
            FIXME("GL_EXT_fogcoord glVertex4f hook already applied\n");
        } else {
            old_fogcoord_glVertex4f = glVertex4f;
            glVertex4f = wine_glVertex4f;
        }
        if(old_fogcoord_glVertex4fv) {
            FIXME("GL_EXT_fogcoord glVertex4fv hook already applied\n");
        } else {
            old_fogcoord_glVertex4fv = glVertex4fv;
            glVertex4fv = wine_glVertex4fv;
        }
        if(old_fogcoord_glVertex3f) {
            FIXME("GL_EXT_fogcoord glVertex3f hook already applied\n");
        } else {
            old_fogcoord_glVertex3f = glVertex3f;
            glVertex3f = wine_glVertex3f;
        }
        if(old_fogcoord_glVertex3fv) {
            FIXME("GL_EXT_fogcoord glVertex3fv hook already applied\n");
        } else {
            old_fogcoord_glVertex3fv = glVertex3fv;
            glVertex3fv = wine_glVertex3fv;
        }

        if(old_fogcoord_glColor4f) {
            FIXME("GL_EXT_fogcoord glColor4f hook already applied\n");
        } else {
            old_fogcoord_glColor4f = glColor4f;
            glColor4f = wine_glColor4f;
        }
        if(old_fogcoord_glColor4fv) {
            FIXME("GL_EXT_fogcoord glColor4fv hook already applied\n");
        } else {
            old_fogcoord_glColor4fv = glColor4fv;
            glColor4fv = wine_glColor4fv;
        }
        if(old_fogcoord_glColor3f) {
            FIXME("GL_EXT_fogcoord glColor3f hook already applied\n");
        } else {
            old_fogcoord_glColor3f = glColor3f;
            glColor3f = wine_glColor3f;
        }
        if(old_fogcoord_glColor3fv) {
            FIXME("GL_EXT_fogcoord glColor3fv hook already applied\n");
        } else {
            old_fogcoord_glColor3fv = glColor3fv;
            glColor3fv = wine_glColor3fv;
        }
        if(old_fogcoord_glColor4ub) {
            FIXME("GL_EXT_fogcoord glColor4ub hook already applied\n");
        } else {
            old_fogcoord_glColor4ub = glColor4ub;
            glColor4ub = wine_glColor4ub;
        }

        if(old_fogcoord_glFogCoordfEXT) {
            FIXME("GL_EXT_fogcoord glFogCoordfEXT hook already applied\n");
        } else {
            old_fogcoord_glFogCoordfEXT = gl_info->glFogCoordfEXT;
            gl_info->glFogCoordfEXT = wine_glFogCoordfEXT;
        }
        if(old_fogcoord_glFogCoordfvEXT) {
            FIXME("GL_EXT_fogcoord glFogCoordfvEXT hook already applied\n");
        } else {
            old_fogcoord_glFogCoordfvEXT = gl_info->glFogCoordfvEXT;
            gl_info->glFogCoordfvEXT = wine_glFogCoordfvEXT;
        }
        if(old_fogcoord_glFogCoorddEXT) {
            FIXME("GL_EXT_fogcoord glFogCoorddEXT hook already applied\n");
        } else {
            old_fogcoord_glFogCoorddEXT = gl_info->glFogCoorddEXT;
            gl_info->glFogCoorddEXT = wine_glFogCoorddEXT;
        }
        if(old_fogcoord_glFogCoorddvEXT) {
            FIXME("GL_EXT_fogcoord glFogCoorddvEXT hook already applied\n");
        } else {
            old_fogcoord_glFogCoorddvEXT = gl_info->glFogCoorddvEXT;
            gl_info->glFogCoorddvEXT = wine_glFogCoorddvEXT;
        }
        gl_info->supported[EXT_FOG_COORD] = TRUE;
    }
}
#undef GLINFO_LOCATION
