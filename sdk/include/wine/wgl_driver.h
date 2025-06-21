/* Automatically generated from http://www.opengl.org/registry files; DO NOT EDIT! */

#ifndef __WINE_WGL_DRIVER_H
#define __WINE_WGL_DRIVER_H

#ifndef WINE_GLAPI
#define WINE_GLAPI
#endif

#define WINE_WGL_DRIVER_VERSION 28

struct wgl_context;
struct wgl_pbuffer;

struct wgl_pixel_format
{
    PIXELFORMATDESCRIPTOR pfd;
    int swap_method;
    int transparent;
    int pixel_type;
    int draw_to_pbuffer;
    int max_pbuffer_pixels;
    int max_pbuffer_width;
    int max_pbuffer_height;
    int transparent_red_value;
    int transparent_red_value_valid;
    int transparent_green_value;
    int transparent_green_value_valid;
    int transparent_blue_value;
    int transparent_blue_value_valid;
    int transparent_alpha_value;
    int transparent_alpha_value_valid;
    int transparent_index_value;
    int transparent_index_value_valid;
    int sample_buffers;
    int samples;
    int bind_to_texture_rgb;
    int bind_to_texture_rgba;
    int bind_to_texture_rectangle_rgb;
    int bind_to_texture_rectangle_rgba;
    int framebuffer_srgb_capable;
    int float_components;
};

struct opengl_funcs
{
    struct
    {
        BOOL       (WINE_GLAPI *p_wglCopyContext)( struct wgl_context * hglrcSrc, struct wgl_context * hglrcDst, UINT mask );
        struct wgl_context * (WINE_GLAPI *p_wglCreateContext)( HDC hDc );
        BOOL       (WINE_GLAPI *p_wglDeleteContext)( struct wgl_context * oldContext );
        int        (WINE_GLAPI *p_wglGetPixelFormat)( HDC hdc );
        PROC       (WINE_GLAPI *p_wglGetProcAddress)( LPCSTR lpszProc );
        BOOL       (WINE_GLAPI *p_wglMakeCurrent)( HDC hDc, struct wgl_context * newContext );
        BOOL       (WINE_GLAPI *p_wglSetPixelFormat)( HDC hdc, int ipfd, const PIXELFORMATDESCRIPTOR *ppfd );
        BOOL       (WINE_GLAPI *p_wglShareLists)( struct wgl_context * hrcSrvShare, struct wgl_context * hrcSrvSource );
        BOOL       (WINE_GLAPI *p_wglSwapBuffers)( HDC hdc );
        void       (WINE_GLAPI *p_get_pixel_formats)( struct wgl_pixel_format *formats, UINT max_formats, UINT *num_formats, UINT *num_onscreen_formats );
    } wgl;

    struct
    {
        void       (WINE_GLAPI *p_glAccum)( GLenum op, GLfloat value );
        void       (WINE_GLAPI *p_glAlphaFunc)( GLenum func, GLfloat ref );
        GLboolean  (WINE_GLAPI *p_glAreTexturesResident)( GLsizei n, const GLuint *textures, GLboolean *residences );
        void       (WINE_GLAPI *p_glArrayElement)( GLint i );
        void       (WINE_GLAPI *p_glBegin)( GLenum mode );
        void       (WINE_GLAPI *p_glBindTexture)( GLenum target, GLuint texture );
        void       (WINE_GLAPI *p_glBitmap)( GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap );
        void       (WINE_GLAPI *p_glBlendFunc)( GLenum sfactor, GLenum dfactor );
        void       (WINE_GLAPI *p_glCallList)( GLuint list );
        void       (WINE_GLAPI *p_glCallLists)( GLsizei n, GLenum type, const void *lists );
        void       (WINE_GLAPI *p_glClear)( GLbitfield mask );
        void       (WINE_GLAPI *p_glClearAccum)( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
        void       (WINE_GLAPI *p_glClearColor)( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
        void       (WINE_GLAPI *p_glClearDepth)( GLdouble depth );
        void       (WINE_GLAPI *p_glClearIndex)( GLfloat c );
        void       (WINE_GLAPI *p_glClearStencil)( GLint s );
        void       (WINE_GLAPI *p_glClipPlane)( GLenum plane, const GLdouble *equation );
        void       (WINE_GLAPI *p_glColor3b)( GLbyte red, GLbyte green, GLbyte blue );
        void       (WINE_GLAPI *p_glColor3bv)( const GLbyte *v );
        void       (WINE_GLAPI *p_glColor3d)( GLdouble red, GLdouble green, GLdouble blue );
        void       (WINE_GLAPI *p_glColor3dv)( const GLdouble *v );
        void       (WINE_GLAPI *p_glColor3f)( GLfloat red, GLfloat green, GLfloat blue );
        void       (WINE_GLAPI *p_glColor3fv)( const GLfloat *v );
        void       (WINE_GLAPI *p_glColor3i)( GLint red, GLint green, GLint blue );
        void       (WINE_GLAPI *p_glColor3iv)( const GLint *v );
        void       (WINE_GLAPI *p_glColor3s)( GLshort red, GLshort green, GLshort blue );
        void       (WINE_GLAPI *p_glColor3sv)( const GLshort *v );
        void       (WINE_GLAPI *p_glColor3ub)( GLubyte red, GLubyte green, GLubyte blue );
        void       (WINE_GLAPI *p_glColor3ubv)( const GLubyte *v );
        void       (WINE_GLAPI *p_glColor3ui)( GLuint red, GLuint green, GLuint blue );
        void       (WINE_GLAPI *p_glColor3uiv)( const GLuint *v );
        void       (WINE_GLAPI *p_glColor3us)( GLushort red, GLushort green, GLushort blue );
        void       (WINE_GLAPI *p_glColor3usv)( const GLushort *v );
        void       (WINE_GLAPI *p_glColor4b)( GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha );
        void       (WINE_GLAPI *p_glColor4bv)( const GLbyte *v );
        void       (WINE_GLAPI *p_glColor4d)( GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha );
        void       (WINE_GLAPI *p_glColor4dv)( const GLdouble *v );
        void       (WINE_GLAPI *p_glColor4f)( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
        void       (WINE_GLAPI *p_glColor4fv)( const GLfloat *v );
        void       (WINE_GLAPI *p_glColor4i)( GLint red, GLint green, GLint blue, GLint alpha );
        void       (WINE_GLAPI *p_glColor4iv)( const GLint *v );
        void       (WINE_GLAPI *p_glColor4s)( GLshort red, GLshort green, GLshort blue, GLshort alpha );
        void       (WINE_GLAPI *p_glColor4sv)( const GLshort *v );
        void       (WINE_GLAPI *p_glColor4ub)( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha );
        void       (WINE_GLAPI *p_glColor4ubv)( const GLubyte *v );
        void       (WINE_GLAPI *p_glColor4ui)( GLuint red, GLuint green, GLuint blue, GLuint alpha );
        void       (WINE_GLAPI *p_glColor4uiv)( const GLuint *v );
        void       (WINE_GLAPI *p_glColor4us)( GLushort red, GLushort green, GLushort blue, GLushort alpha );
        void       (WINE_GLAPI *p_glColor4usv)( const GLushort *v );
        void       (WINE_GLAPI *p_glColorMask)( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha );
        void       (WINE_GLAPI *p_glColorMaterial)( GLenum face, GLenum mode );
        void       (WINE_GLAPI *p_glColorPointer)( GLint size, GLenum type, GLsizei stride, const void *pointer );
        void       (WINE_GLAPI *p_glCopyPixels)( GLint x, GLint y, GLsizei width, GLsizei height, GLenum type );
        void       (WINE_GLAPI *p_glCopyTexImage1D)( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border );
        void       (WINE_GLAPI *p_glCopyTexImage2D)( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border );
        void       (WINE_GLAPI *p_glCopyTexSubImage1D)( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width );
        void       (WINE_GLAPI *p_glCopyTexSubImage2D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glCullFace)( GLenum mode );
        void       (WINE_GLAPI *p_glDeleteLists)( GLuint list, GLsizei range );
        void       (WINE_GLAPI *p_glDeleteTextures)( GLsizei n, const GLuint *textures );
        void       (WINE_GLAPI *p_glDepthFunc)( GLenum func );
        void       (WINE_GLAPI *p_glDepthMask)( GLboolean flag );
        void       (WINE_GLAPI *p_glDepthRange)( GLdouble n, GLdouble f );
        void       (WINE_GLAPI *p_glDisable)( GLenum cap );
        void       (WINE_GLAPI *p_glDisableClientState)( GLenum array );
        void       (WINE_GLAPI *p_glDrawArrays)( GLenum mode, GLint first, GLsizei count );
        void       (WINE_GLAPI *p_glDrawBuffer)( GLenum buf );
        void       (WINE_GLAPI *p_glDrawElements)( GLenum mode, GLsizei count, GLenum type, const void *indices );
        void       (WINE_GLAPI *p_glDrawPixels)( GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glEdgeFlag)( GLboolean flag );
        void       (WINE_GLAPI *p_glEdgeFlagPointer)( GLsizei stride, const void *pointer );
        void       (WINE_GLAPI *p_glEdgeFlagv)( const GLboolean *flag );
        void       (WINE_GLAPI *p_glEnable)( GLenum cap );
        void       (WINE_GLAPI *p_glEnableClientState)( GLenum array );
        void       (WINE_GLAPI *p_glEnd)(void);
        void       (WINE_GLAPI *p_glEndList)(void);
        void       (WINE_GLAPI *p_glEvalCoord1d)( GLdouble u );
        void       (WINE_GLAPI *p_glEvalCoord1dv)( const GLdouble *u );
        void       (WINE_GLAPI *p_glEvalCoord1f)( GLfloat u );
        void       (WINE_GLAPI *p_glEvalCoord1fv)( const GLfloat *u );
        void       (WINE_GLAPI *p_glEvalCoord2d)( GLdouble u, GLdouble v );
        void       (WINE_GLAPI *p_glEvalCoord2dv)( const GLdouble *u );
        void       (WINE_GLAPI *p_glEvalCoord2f)( GLfloat u, GLfloat v );
        void       (WINE_GLAPI *p_glEvalCoord2fv)( const GLfloat *u );
        void       (WINE_GLAPI *p_glEvalMesh1)( GLenum mode, GLint i1, GLint i2 );
        void       (WINE_GLAPI *p_glEvalMesh2)( GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2 );
        void       (WINE_GLAPI *p_glEvalPoint1)( GLint i );
        void       (WINE_GLAPI *p_glEvalPoint2)( GLint i, GLint j );
        void       (WINE_GLAPI *p_glFeedbackBuffer)( GLsizei size, GLenum type, GLfloat *buffer );
        void       (WINE_GLAPI *p_glFinish)(void);
        void       (WINE_GLAPI *p_glFlush)(void);
        void       (WINE_GLAPI *p_glFogf)( GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glFogfv)( GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glFogi)( GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glFogiv)( GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glFrontFace)( GLenum mode );
        void       (WINE_GLAPI *p_glFrustum)( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar );
        GLuint     (WINE_GLAPI *p_glGenLists)( GLsizei range );
        void       (WINE_GLAPI *p_glGenTextures)( GLsizei n, GLuint *textures );
        void       (WINE_GLAPI *p_glGetBooleanv)( GLenum pname, GLboolean *data );
        void       (WINE_GLAPI *p_glGetClipPlane)( GLenum plane, GLdouble *equation );
        void       (WINE_GLAPI *p_glGetDoublev)( GLenum pname, GLdouble *data );
        GLenum     (WINE_GLAPI *p_glGetError)(void);
        void       (WINE_GLAPI *p_glGetFloatv)( GLenum pname, GLfloat *data );
        void       (WINE_GLAPI *p_glGetIntegerv)( GLenum pname, GLint *data );
        void       (WINE_GLAPI *p_glGetLightfv)( GLenum light, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetLightiv)( GLenum light, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetMapdv)( GLenum target, GLenum query, GLdouble *v );
        void       (WINE_GLAPI *p_glGetMapfv)( GLenum target, GLenum query, GLfloat *v );
        void       (WINE_GLAPI *p_glGetMapiv)( GLenum target, GLenum query, GLint *v );
        void       (WINE_GLAPI *p_glGetMaterialfv)( GLenum face, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetMaterialiv)( GLenum face, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetPixelMapfv)( GLenum map, GLfloat *values );
        void       (WINE_GLAPI *p_glGetPixelMapuiv)( GLenum map, GLuint *values );
        void       (WINE_GLAPI *p_glGetPixelMapusv)( GLenum map, GLushort *values );
        void       (WINE_GLAPI *p_glGetPointerv)( GLenum pname, void **params );
        void       (WINE_GLAPI *p_glGetPolygonStipple)( GLubyte *mask );
        const GLubyte * (WINE_GLAPI *p_glGetString)( GLenum name );
        void       (WINE_GLAPI *p_glGetTexEnvfv)( GLenum target, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetTexEnviv)( GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetTexGendv)( GLenum coord, GLenum pname, GLdouble *params );
        void       (WINE_GLAPI *p_glGetTexGenfv)( GLenum coord, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetTexGeniv)( GLenum coord, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetTexImage)( GLenum target, GLint level, GLenum format, GLenum type, void *pixels );
        void       (WINE_GLAPI *p_glGetTexLevelParameterfv)( GLenum target, GLint level, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetTexLevelParameteriv)( GLenum target, GLint level, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetTexParameterfv)( GLenum target, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetTexParameteriv)( GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glHint)( GLenum target, GLenum mode );
        void       (WINE_GLAPI *p_glIndexMask)( GLuint mask );
        void       (WINE_GLAPI *p_glIndexPointer)( GLenum type, GLsizei stride, const void *pointer );
        void       (WINE_GLAPI *p_glIndexd)( GLdouble c );
        void       (WINE_GLAPI *p_glIndexdv)( const GLdouble *c );
        void       (WINE_GLAPI *p_glIndexf)( GLfloat c );
        void       (WINE_GLAPI *p_glIndexfv)( const GLfloat *c );
        void       (WINE_GLAPI *p_glIndexi)( GLint c );
        void       (WINE_GLAPI *p_glIndexiv)( const GLint *c );
        void       (WINE_GLAPI *p_glIndexs)( GLshort c );
        void       (WINE_GLAPI *p_glIndexsv)( const GLshort *c );
        void       (WINE_GLAPI *p_glIndexub)( GLubyte c );
        void       (WINE_GLAPI *p_glIndexubv)( const GLubyte *c );
        void       (WINE_GLAPI *p_glInitNames)(void);
        void       (WINE_GLAPI *p_glInterleavedArrays)( GLenum format, GLsizei stride, const void *pointer );
        GLboolean  (WINE_GLAPI *p_glIsEnabled)( GLenum cap );
        GLboolean  (WINE_GLAPI *p_glIsList)( GLuint list );
        GLboolean  (WINE_GLAPI *p_glIsTexture)( GLuint texture );
        void       (WINE_GLAPI *p_glLightModelf)( GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glLightModelfv)( GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glLightModeli)( GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glLightModeliv)( GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glLightf)( GLenum light, GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glLightfv)( GLenum light, GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glLighti)( GLenum light, GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glLightiv)( GLenum light, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glLineStipple)( GLint factor, GLushort pattern );
        void       (WINE_GLAPI *p_glLineWidth)( GLfloat width );
        void       (WINE_GLAPI *p_glListBase)( GLuint base );
        void       (WINE_GLAPI *p_glLoadIdentity)(void);
        void       (WINE_GLAPI *p_glLoadMatrixd)( const GLdouble *m );
        void       (WINE_GLAPI *p_glLoadMatrixf)( const GLfloat *m );
        void       (WINE_GLAPI *p_glLoadName)( GLuint name );
        void       (WINE_GLAPI *p_glLogicOp)( GLenum opcode );
        void       (WINE_GLAPI *p_glMap1d)( GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points );
        void       (WINE_GLAPI *p_glMap1f)( GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points );
        void       (WINE_GLAPI *p_glMap2d)( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points );
        void       (WINE_GLAPI *p_glMap2f)( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points );
        void       (WINE_GLAPI *p_glMapGrid1d)( GLint un, GLdouble u1, GLdouble u2 );
        void       (WINE_GLAPI *p_glMapGrid1f)( GLint un, GLfloat u1, GLfloat u2 );
        void       (WINE_GLAPI *p_glMapGrid2d)( GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2 );
        void       (WINE_GLAPI *p_glMapGrid2f)( GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2 );
        void       (WINE_GLAPI *p_glMaterialf)( GLenum face, GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glMaterialfv)( GLenum face, GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glMateriali)( GLenum face, GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glMaterialiv)( GLenum face, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glMatrixMode)( GLenum mode );
        void       (WINE_GLAPI *p_glMultMatrixd)( const GLdouble *m );
        void       (WINE_GLAPI *p_glMultMatrixf)( const GLfloat *m );
        void       (WINE_GLAPI *p_glNewList)( GLuint list, GLenum mode );
        void       (WINE_GLAPI *p_glNormal3b)( GLbyte nx, GLbyte ny, GLbyte nz );
        void       (WINE_GLAPI *p_glNormal3bv)( const GLbyte *v );
        void       (WINE_GLAPI *p_glNormal3d)( GLdouble nx, GLdouble ny, GLdouble nz );
        void       (WINE_GLAPI *p_glNormal3dv)( const GLdouble *v );
        void       (WINE_GLAPI *p_glNormal3f)( GLfloat nx, GLfloat ny, GLfloat nz );
        void       (WINE_GLAPI *p_glNormal3fv)( const GLfloat *v );
        void       (WINE_GLAPI *p_glNormal3i)( GLint nx, GLint ny, GLint nz );
        void       (WINE_GLAPI *p_glNormal3iv)( const GLint *v );
        void       (WINE_GLAPI *p_glNormal3s)( GLshort nx, GLshort ny, GLshort nz );
        void       (WINE_GLAPI *p_glNormal3sv)( const GLshort *v );
        void       (WINE_GLAPI *p_glNormalPointer)( GLenum type, GLsizei stride, const void *pointer );
        void       (WINE_GLAPI *p_glOrtho)( GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar );
        void       (WINE_GLAPI *p_glPassThrough)( GLfloat token );
        void       (WINE_GLAPI *p_glPixelMapfv)( GLenum map, GLsizei mapsize, const GLfloat *values );
        void       (WINE_GLAPI *p_glPixelMapuiv)( GLenum map, GLsizei mapsize, const GLuint *values );
        void       (WINE_GLAPI *p_glPixelMapusv)( GLenum map, GLsizei mapsize, const GLushort *values );
        void       (WINE_GLAPI *p_glPixelStoref)( GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glPixelStorei)( GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glPixelTransferf)( GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glPixelTransferi)( GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glPixelZoom)( GLfloat xfactor, GLfloat yfactor );
        void       (WINE_GLAPI *p_glPointSize)( GLfloat size );
        void       (WINE_GLAPI *p_glPolygonMode)( GLenum face, GLenum mode );
        void       (WINE_GLAPI *p_glPolygonOffset)( GLfloat factor, GLfloat units );
        void       (WINE_GLAPI *p_glPolygonStipple)( const GLubyte *mask );
        void       (WINE_GLAPI *p_glPopAttrib)(void);
        void       (WINE_GLAPI *p_glPopClientAttrib)(void);
        void       (WINE_GLAPI *p_glPopMatrix)(void);
        void       (WINE_GLAPI *p_glPopName)(void);
        void       (WINE_GLAPI *p_glPrioritizeTextures)( GLsizei n, const GLuint *textures, const GLfloat *priorities );
        void       (WINE_GLAPI *p_glPushAttrib)( GLbitfield mask );
        void       (WINE_GLAPI *p_glPushClientAttrib)( GLbitfield mask );
        void       (WINE_GLAPI *p_glPushMatrix)(void);
        void       (WINE_GLAPI *p_glPushName)( GLuint name );
        void       (WINE_GLAPI *p_glRasterPos2d)( GLdouble x, GLdouble y );
        void       (WINE_GLAPI *p_glRasterPos2dv)( const GLdouble *v );
        void       (WINE_GLAPI *p_glRasterPos2f)( GLfloat x, GLfloat y );
        void       (WINE_GLAPI *p_glRasterPos2fv)( const GLfloat *v );
        void       (WINE_GLAPI *p_glRasterPos2i)( GLint x, GLint y );
        void       (WINE_GLAPI *p_glRasterPos2iv)( const GLint *v );
        void       (WINE_GLAPI *p_glRasterPos2s)( GLshort x, GLshort y );
        void       (WINE_GLAPI *p_glRasterPos2sv)( const GLshort *v );
        void       (WINE_GLAPI *p_glRasterPos3d)( GLdouble x, GLdouble y, GLdouble z );
        void       (WINE_GLAPI *p_glRasterPos3dv)( const GLdouble *v );
        void       (WINE_GLAPI *p_glRasterPos3f)( GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glRasterPos3fv)( const GLfloat *v );
        void       (WINE_GLAPI *p_glRasterPos3i)( GLint x, GLint y, GLint z );
        void       (WINE_GLAPI *p_glRasterPos3iv)( const GLint *v );
        void       (WINE_GLAPI *p_glRasterPos3s)( GLshort x, GLshort y, GLshort z );
        void       (WINE_GLAPI *p_glRasterPos3sv)( const GLshort *v );
        void       (WINE_GLAPI *p_glRasterPos4d)( GLdouble x, GLdouble y, GLdouble z, GLdouble w );
        void       (WINE_GLAPI *p_glRasterPos4dv)( const GLdouble *v );
        void       (WINE_GLAPI *p_glRasterPos4f)( GLfloat x, GLfloat y, GLfloat z, GLfloat w );
        void       (WINE_GLAPI *p_glRasterPos4fv)( const GLfloat *v );
        void       (WINE_GLAPI *p_glRasterPos4i)( GLint x, GLint y, GLint z, GLint w );
        void       (WINE_GLAPI *p_glRasterPos4iv)( const GLint *v );
        void       (WINE_GLAPI *p_glRasterPos4s)( GLshort x, GLshort y, GLshort z, GLshort w );
        void       (WINE_GLAPI *p_glRasterPos4sv)( const GLshort *v );
        void       (WINE_GLAPI *p_glReadBuffer)( GLenum src );
        void       (WINE_GLAPI *p_glReadPixels)( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void *pixels );
        void       (WINE_GLAPI *p_glRectd)( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 );
        void       (WINE_GLAPI *p_glRectdv)( const GLdouble *v1, const GLdouble *v2 );
        void       (WINE_GLAPI *p_glRectf)( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 );
        void       (WINE_GLAPI *p_glRectfv)( const GLfloat *v1, const GLfloat *v2 );
        void       (WINE_GLAPI *p_glRecti)( GLint x1, GLint y1, GLint x2, GLint y2 );
        void       (WINE_GLAPI *p_glRectiv)( const GLint *v1, const GLint *v2 );
        void       (WINE_GLAPI *p_glRects)( GLshort x1, GLshort y1, GLshort x2, GLshort y2 );
        void       (WINE_GLAPI *p_glRectsv)( const GLshort *v1, const GLshort *v2 );
        GLint      (WINE_GLAPI *p_glRenderMode)( GLenum mode );
        void       (WINE_GLAPI *p_glRotated)( GLdouble angle, GLdouble x, GLdouble y, GLdouble z );
        void       (WINE_GLAPI *p_glRotatef)( GLfloat angle, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glScaled)( GLdouble x, GLdouble y, GLdouble z );
        void       (WINE_GLAPI *p_glScalef)( GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glScissor)( GLint x, GLint y, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glSelectBuffer)( GLsizei size, GLuint *buffer );
        void       (WINE_GLAPI *p_glShadeModel)( GLenum mode );
        void       (WINE_GLAPI *p_glStencilFunc)( GLenum func, GLint ref, GLuint mask );
        void       (WINE_GLAPI *p_glStencilMask)( GLuint mask );
        void       (WINE_GLAPI *p_glStencilOp)( GLenum fail, GLenum zfail, GLenum zpass );
        void       (WINE_GLAPI *p_glTexCoord1d)( GLdouble s );
        void       (WINE_GLAPI *p_glTexCoord1dv)( const GLdouble *v );
        void       (WINE_GLAPI *p_glTexCoord1f)( GLfloat s );
        void       (WINE_GLAPI *p_glTexCoord1fv)( const GLfloat *v );
        void       (WINE_GLAPI *p_glTexCoord1i)( GLint s );
        void       (WINE_GLAPI *p_glTexCoord1iv)( const GLint *v );
        void       (WINE_GLAPI *p_glTexCoord1s)( GLshort s );
        void       (WINE_GLAPI *p_glTexCoord1sv)( const GLshort *v );
        void       (WINE_GLAPI *p_glTexCoord2d)( GLdouble s, GLdouble t );
        void       (WINE_GLAPI *p_glTexCoord2dv)( const GLdouble *v );
        void       (WINE_GLAPI *p_glTexCoord2f)( GLfloat s, GLfloat t );
        void       (WINE_GLAPI *p_glTexCoord2fv)( const GLfloat *v );
        void       (WINE_GLAPI *p_glTexCoord2i)( GLint s, GLint t );
        void       (WINE_GLAPI *p_glTexCoord2iv)( const GLint *v );
        void       (WINE_GLAPI *p_glTexCoord2s)( GLshort s, GLshort t );
        void       (WINE_GLAPI *p_glTexCoord2sv)( const GLshort *v );
        void       (WINE_GLAPI *p_glTexCoord3d)( GLdouble s, GLdouble t, GLdouble r );
        void       (WINE_GLAPI *p_glTexCoord3dv)( const GLdouble *v );
        void       (WINE_GLAPI *p_glTexCoord3f)( GLfloat s, GLfloat t, GLfloat r );
        void       (WINE_GLAPI *p_glTexCoord3fv)( const GLfloat *v );
        void       (WINE_GLAPI *p_glTexCoord3i)( GLint s, GLint t, GLint r );
        void       (WINE_GLAPI *p_glTexCoord3iv)( const GLint *v );
        void       (WINE_GLAPI *p_glTexCoord3s)( GLshort s, GLshort t, GLshort r );
        void       (WINE_GLAPI *p_glTexCoord3sv)( const GLshort *v );
        void       (WINE_GLAPI *p_glTexCoord4d)( GLdouble s, GLdouble t, GLdouble r, GLdouble q );
        void       (WINE_GLAPI *p_glTexCoord4dv)( const GLdouble *v );
        void       (WINE_GLAPI *p_glTexCoord4f)( GLfloat s, GLfloat t, GLfloat r, GLfloat q );
        void       (WINE_GLAPI *p_glTexCoord4fv)( const GLfloat *v );
        void       (WINE_GLAPI *p_glTexCoord4i)( GLint s, GLint t, GLint r, GLint q );
        void       (WINE_GLAPI *p_glTexCoord4iv)( const GLint *v );
        void       (WINE_GLAPI *p_glTexCoord4s)( GLshort s, GLshort t, GLshort r, GLshort q );
        void       (WINE_GLAPI *p_glTexCoord4sv)( const GLshort *v );
        void       (WINE_GLAPI *p_glTexCoordPointer)( GLint size, GLenum type, GLsizei stride, const void *pointer );
        void       (WINE_GLAPI *p_glTexEnvf)( GLenum target, GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glTexEnvfv)( GLenum target, GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glTexEnvi)( GLenum target, GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glTexEnviv)( GLenum target, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glTexGend)( GLenum coord, GLenum pname, GLdouble param );
        void       (WINE_GLAPI *p_glTexGendv)( GLenum coord, GLenum pname, const GLdouble *params );
        void       (WINE_GLAPI *p_glTexGenf)( GLenum coord, GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glTexGenfv)( GLenum coord, GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glTexGeni)( GLenum coord, GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glTexGeniv)( GLenum coord, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glTexImage1D)( GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glTexImage2D)( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glTexParameterf)( GLenum target, GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glTexParameterfv)( GLenum target, GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glTexParameteri)( GLenum target, GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glTexParameteriv)( GLenum target, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glTexSubImage1D)( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glTexSubImage2D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glTranslated)( GLdouble x, GLdouble y, GLdouble z );
        void       (WINE_GLAPI *p_glTranslatef)( GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glVertex2d)( GLdouble x, GLdouble y );
        void       (WINE_GLAPI *p_glVertex2dv)( const GLdouble *v );
        void       (WINE_GLAPI *p_glVertex2f)( GLfloat x, GLfloat y );
        void       (WINE_GLAPI *p_glVertex2fv)( const GLfloat *v );
        void       (WINE_GLAPI *p_glVertex2i)( GLint x, GLint y );
        void       (WINE_GLAPI *p_glVertex2iv)( const GLint *v );
        void       (WINE_GLAPI *p_glVertex2s)( GLshort x, GLshort y );
        void       (WINE_GLAPI *p_glVertex2sv)( const GLshort *v );
        void       (WINE_GLAPI *p_glVertex3d)( GLdouble x, GLdouble y, GLdouble z );
        void       (WINE_GLAPI *p_glVertex3dv)( const GLdouble *v );
        void       (WINE_GLAPI *p_glVertex3f)( GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glVertex3fv)( const GLfloat *v );
        void       (WINE_GLAPI *p_glVertex3i)( GLint x, GLint y, GLint z );
        void       (WINE_GLAPI *p_glVertex3iv)( const GLint *v );
        void       (WINE_GLAPI *p_glVertex3s)( GLshort x, GLshort y, GLshort z );
        void       (WINE_GLAPI *p_glVertex3sv)( const GLshort *v );
        void       (WINE_GLAPI *p_glVertex4d)( GLdouble x, GLdouble y, GLdouble z, GLdouble w );
        void       (WINE_GLAPI *p_glVertex4dv)( const GLdouble *v );
        void       (WINE_GLAPI *p_glVertex4f)( GLfloat x, GLfloat y, GLfloat z, GLfloat w );
        void       (WINE_GLAPI *p_glVertex4fv)( const GLfloat *v );
        void       (WINE_GLAPI *p_glVertex4i)( GLint x, GLint y, GLint z, GLint w );
        void       (WINE_GLAPI *p_glVertex4iv)( const GLint *v );
        void       (WINE_GLAPI *p_glVertex4s)( GLshort x, GLshort y, GLshort z, GLshort w );
        void       (WINE_GLAPI *p_glVertex4sv)( const GLshort *v );
        void       (WINE_GLAPI *p_glVertexPointer)( GLint size, GLenum type, GLsizei stride, const void *pointer );
        void       (WINE_GLAPI *p_glViewport)( GLint x, GLint y, GLsizei width, GLsizei height );
    } gl;

    struct
    {
        void       (WINE_GLAPI *p_glAccumxOES)( GLenum op, GLfixed value );
        GLboolean  (WINE_GLAPI *p_glAcquireKeyedMutexWin32EXT)( GLuint memory, GLuint64 key, GLuint timeout );
        void       (WINE_GLAPI *p_glActiveProgramEXT)( GLuint program );
        void       (WINE_GLAPI *p_glActiveShaderProgram)( GLuint pipeline, GLuint program );
        void       (WINE_GLAPI *p_glActiveStencilFaceEXT)( GLenum face );
        void       (WINE_GLAPI *p_glActiveTexture)( GLenum texture );
        void       (WINE_GLAPI *p_glActiveTextureARB)( GLenum texture );
        void       (WINE_GLAPI *p_glActiveVaryingNV)( GLuint program, const GLchar *name );
        void       (WINE_GLAPI *p_glAlphaFragmentOp1ATI)( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod );
        void       (WINE_GLAPI *p_glAlphaFragmentOp2ATI)( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod );
        void       (WINE_GLAPI *p_glAlphaFragmentOp3ATI)( GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod );
        void       (WINE_GLAPI *p_glAlphaFuncxOES)( GLenum func, GLfixed ref );
        void       (WINE_GLAPI *p_glAlphaToCoverageDitherControlNV)( GLenum mode );
        void       (WINE_GLAPI *p_glApplyFramebufferAttachmentCMAAINTEL)(void);
        void       (WINE_GLAPI *p_glApplyTextureEXT)( GLenum mode );
        GLboolean  (WINE_GLAPI *p_glAreProgramsResidentNV)( GLsizei n, const GLuint *programs, GLboolean *residences );
        GLboolean  (WINE_GLAPI *p_glAreTexturesResidentEXT)( GLsizei n, const GLuint *textures, GLboolean *residences );
        void       (WINE_GLAPI *p_glArrayElementEXT)( GLint i );
        void       (WINE_GLAPI *p_glArrayObjectATI)( GLenum array, GLint size, GLenum type, GLsizei stride, GLuint buffer, GLuint offset );
        GLuint     (WINE_GLAPI *p_glAsyncCopyBufferSubDataNVX)( GLsizei waitSemaphoreCount, const GLuint *waitSemaphoreArray, const GLuint64 *fenceValueArray, GLuint readGpu, GLbitfield writeGpuMask, GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size, GLsizei signalSemaphoreCount, const GLuint *signalSemaphoreArray, const GLuint64 *signalValueArray );
        GLuint     (WINE_GLAPI *p_glAsyncCopyImageSubDataNVX)( GLsizei waitSemaphoreCount, const GLuint *waitSemaphoreArray, const GLuint64 *waitValueArray, GLuint srcGpu, GLbitfield dstGpuMask, GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth, GLsizei signalSemaphoreCount, const GLuint *signalSemaphoreArray, const GLuint64 *signalValueArray );
        void       (WINE_GLAPI *p_glAsyncMarkerSGIX)( GLuint marker );
        void       (WINE_GLAPI *p_glAttachObjectARB)( GLhandleARB containerObj, GLhandleARB obj );
        void       (WINE_GLAPI *p_glAttachShader)( GLuint program, GLuint shader );
        void       (WINE_GLAPI *p_glBeginConditionalRender)( GLuint id, GLenum mode );
        void       (WINE_GLAPI *p_glBeginConditionalRenderNV)( GLuint id, GLenum mode );
        void       (WINE_GLAPI *p_glBeginConditionalRenderNVX)( GLuint id );
        void       (WINE_GLAPI *p_glBeginFragmentShaderATI)(void);
        void       (WINE_GLAPI *p_glBeginOcclusionQueryNV)( GLuint id );
        void       (WINE_GLAPI *p_glBeginPerfMonitorAMD)( GLuint monitor );
        void       (WINE_GLAPI *p_glBeginPerfQueryINTEL)( GLuint queryHandle );
        void       (WINE_GLAPI *p_glBeginQuery)( GLenum target, GLuint id );
        void       (WINE_GLAPI *p_glBeginQueryARB)( GLenum target, GLuint id );
        void       (WINE_GLAPI *p_glBeginQueryIndexed)( GLenum target, GLuint index, GLuint id );
        void       (WINE_GLAPI *p_glBeginTransformFeedback)( GLenum primitiveMode );
        void       (WINE_GLAPI *p_glBeginTransformFeedbackEXT)( GLenum primitiveMode );
        void       (WINE_GLAPI *p_glBeginTransformFeedbackNV)( GLenum primitiveMode );
        void       (WINE_GLAPI *p_glBeginVertexShaderEXT)(void);
        void       (WINE_GLAPI *p_glBeginVideoCaptureNV)( GLuint video_capture_slot );
        void       (WINE_GLAPI *p_glBindAttribLocation)( GLuint program, GLuint index, const GLchar *name );
        void       (WINE_GLAPI *p_glBindAttribLocationARB)( GLhandleARB programObj, GLuint index, const GLcharARB *name );
        void       (WINE_GLAPI *p_glBindBuffer)( GLenum target, GLuint buffer );
        void       (WINE_GLAPI *p_glBindBufferARB)( GLenum target, GLuint buffer );
        void       (WINE_GLAPI *p_glBindBufferBase)( GLenum target, GLuint index, GLuint buffer );
        void       (WINE_GLAPI *p_glBindBufferBaseEXT)( GLenum target, GLuint index, GLuint buffer );
        void       (WINE_GLAPI *p_glBindBufferBaseNV)( GLenum target, GLuint index, GLuint buffer );
        void       (WINE_GLAPI *p_glBindBufferOffsetEXT)( GLenum target, GLuint index, GLuint buffer, GLintptr offset );
        void       (WINE_GLAPI *p_glBindBufferOffsetNV)( GLenum target, GLuint index, GLuint buffer, GLintptr offset );
        void       (WINE_GLAPI *p_glBindBufferRange)( GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size );
        void       (WINE_GLAPI *p_glBindBufferRangeEXT)( GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size );
        void       (WINE_GLAPI *p_glBindBufferRangeNV)( GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size );
        void       (WINE_GLAPI *p_glBindBuffersBase)( GLenum target, GLuint first, GLsizei count, const GLuint *buffers );
        void       (WINE_GLAPI *p_glBindBuffersRange)( GLenum target, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizeiptr *sizes );
        void       (WINE_GLAPI *p_glBindFragDataLocation)( GLuint program, GLuint color, const GLchar *name );
        void       (WINE_GLAPI *p_glBindFragDataLocationEXT)( GLuint program, GLuint color, const GLchar *name );
        void       (WINE_GLAPI *p_glBindFragDataLocationIndexed)( GLuint program, GLuint colorNumber, GLuint index, const GLchar *name );
        void       (WINE_GLAPI *p_glBindFragmentShaderATI)( GLuint id );
        void       (WINE_GLAPI *p_glBindFramebuffer)( GLenum target, GLuint framebuffer );
        void       (WINE_GLAPI *p_glBindFramebufferEXT)( GLenum target, GLuint framebuffer );
        void       (WINE_GLAPI *p_glBindImageTexture)( GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format );
        void       (WINE_GLAPI *p_glBindImageTextureEXT)( GLuint index, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLint format );
        void       (WINE_GLAPI *p_glBindImageTextures)( GLuint first, GLsizei count, const GLuint *textures );
        GLuint     (WINE_GLAPI *p_glBindLightParameterEXT)( GLenum light, GLenum value );
        GLuint     (WINE_GLAPI *p_glBindMaterialParameterEXT)( GLenum face, GLenum value );
        void       (WINE_GLAPI *p_glBindMultiTextureEXT)( GLenum texunit, GLenum target, GLuint texture );
        GLuint     (WINE_GLAPI *p_glBindParameterEXT)( GLenum value );
        void       (WINE_GLAPI *p_glBindProgramARB)( GLenum target, GLuint program );
        void       (WINE_GLAPI *p_glBindProgramNV)( GLenum target, GLuint id );
        void       (WINE_GLAPI *p_glBindProgramPipeline)( GLuint pipeline );
        void       (WINE_GLAPI *p_glBindRenderbuffer)( GLenum target, GLuint renderbuffer );
        void       (WINE_GLAPI *p_glBindRenderbufferEXT)( GLenum target, GLuint renderbuffer );
        void       (WINE_GLAPI *p_glBindSampler)( GLuint unit, GLuint sampler );
        void       (WINE_GLAPI *p_glBindSamplers)( GLuint first, GLsizei count, const GLuint *samplers );
        void       (WINE_GLAPI *p_glBindShadingRateImageNV)( GLuint texture );
        GLuint     (WINE_GLAPI *p_glBindTexGenParameterEXT)( GLenum unit, GLenum coord, GLenum value );
        void       (WINE_GLAPI *p_glBindTextureEXT)( GLenum target, GLuint texture );
        void       (WINE_GLAPI *p_glBindTextureUnit)( GLuint unit, GLuint texture );
        GLuint     (WINE_GLAPI *p_glBindTextureUnitParameterEXT)( GLenum unit, GLenum value );
        void       (WINE_GLAPI *p_glBindTextures)( GLuint first, GLsizei count, const GLuint *textures );
        void       (WINE_GLAPI *p_glBindTransformFeedback)( GLenum target, GLuint id );
        void       (WINE_GLAPI *p_glBindTransformFeedbackNV)( GLenum target, GLuint id );
        void       (WINE_GLAPI *p_glBindVertexArray)( GLuint array );
        void       (WINE_GLAPI *p_glBindVertexArrayAPPLE)( GLuint array );
        void       (WINE_GLAPI *p_glBindVertexBuffer)( GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride );
        void       (WINE_GLAPI *p_glBindVertexBuffers)( GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides );
        void       (WINE_GLAPI *p_glBindVertexShaderEXT)( GLuint id );
        void       (WINE_GLAPI *p_glBindVideoCaptureStreamBufferNV)( GLuint video_capture_slot, GLuint stream, GLenum frame_region, GLintptrARB offset );
        void       (WINE_GLAPI *p_glBindVideoCaptureStreamTextureNV)( GLuint video_capture_slot, GLuint stream, GLenum frame_region, GLenum target, GLuint texture );
        void       (WINE_GLAPI *p_glBinormal3bEXT)( GLbyte bx, GLbyte by, GLbyte bz );
        void       (WINE_GLAPI *p_glBinormal3bvEXT)( const GLbyte *v );
        void       (WINE_GLAPI *p_glBinormal3dEXT)( GLdouble bx, GLdouble by, GLdouble bz );
        void       (WINE_GLAPI *p_glBinormal3dvEXT)( const GLdouble *v );
        void       (WINE_GLAPI *p_glBinormal3fEXT)( GLfloat bx, GLfloat by, GLfloat bz );
        void       (WINE_GLAPI *p_glBinormal3fvEXT)( const GLfloat *v );
        void       (WINE_GLAPI *p_glBinormal3iEXT)( GLint bx, GLint by, GLint bz );
        void       (WINE_GLAPI *p_glBinormal3ivEXT)( const GLint *v );
        void       (WINE_GLAPI *p_glBinormal3sEXT)( GLshort bx, GLshort by, GLshort bz );
        void       (WINE_GLAPI *p_glBinormal3svEXT)( const GLshort *v );
        void       (WINE_GLAPI *p_glBinormalPointerEXT)( GLenum type, GLsizei stride, const void *pointer );
        void       (WINE_GLAPI *p_glBitmapxOES)( GLsizei width, GLsizei height, GLfixed xorig, GLfixed yorig, GLfixed xmove, GLfixed ymove, const GLubyte *bitmap );
        void       (WINE_GLAPI *p_glBlendBarrierKHR)(void);
        void       (WINE_GLAPI *p_glBlendBarrierNV)(void);
        void       (WINE_GLAPI *p_glBlendColor)( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
        void       (WINE_GLAPI *p_glBlendColorEXT)( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );
        void       (WINE_GLAPI *p_glBlendColorxOES)( GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha );
        void       (WINE_GLAPI *p_glBlendEquation)( GLenum mode );
        void       (WINE_GLAPI *p_glBlendEquationEXT)( GLenum mode );
        void       (WINE_GLAPI *p_glBlendEquationIndexedAMD)( GLuint buf, GLenum mode );
        void       (WINE_GLAPI *p_glBlendEquationSeparate)( GLenum modeRGB, GLenum modeAlpha );
        void       (WINE_GLAPI *p_glBlendEquationSeparateEXT)( GLenum modeRGB, GLenum modeAlpha );
        void       (WINE_GLAPI *p_glBlendEquationSeparateIndexedAMD)( GLuint buf, GLenum modeRGB, GLenum modeAlpha );
        void       (WINE_GLAPI *p_glBlendEquationSeparatei)( GLuint buf, GLenum modeRGB, GLenum modeAlpha );
        void       (WINE_GLAPI *p_glBlendEquationSeparateiARB)( GLuint buf, GLenum modeRGB, GLenum modeAlpha );
        void       (WINE_GLAPI *p_glBlendEquationi)( GLuint buf, GLenum mode );
        void       (WINE_GLAPI *p_glBlendEquationiARB)( GLuint buf, GLenum mode );
        void       (WINE_GLAPI *p_glBlendFuncIndexedAMD)( GLuint buf, GLenum src, GLenum dst );
        void       (WINE_GLAPI *p_glBlendFuncSeparate)( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha );
        void       (WINE_GLAPI *p_glBlendFuncSeparateEXT)( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha );
        void       (WINE_GLAPI *p_glBlendFuncSeparateINGR)( GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha );
        void       (WINE_GLAPI *p_glBlendFuncSeparateIndexedAMD)( GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha );
        void       (WINE_GLAPI *p_glBlendFuncSeparatei)( GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha );
        void       (WINE_GLAPI *p_glBlendFuncSeparateiARB)( GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha );
        void       (WINE_GLAPI *p_glBlendFunci)( GLuint buf, GLenum src, GLenum dst );
        void       (WINE_GLAPI *p_glBlendFunciARB)( GLuint buf, GLenum src, GLenum dst );
        void       (WINE_GLAPI *p_glBlendParameteriNV)( GLenum pname, GLint value );
        void       (WINE_GLAPI *p_glBlitFramebuffer)( GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter );
        void       (WINE_GLAPI *p_glBlitFramebufferEXT)( GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter );
        void       (WINE_GLAPI *p_glBlitNamedFramebuffer)( GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter );
        void       (WINE_GLAPI *p_glBufferAddressRangeNV)( GLenum pname, GLuint index, GLuint64EXT address, GLsizeiptr length );
        void       (WINE_GLAPI *p_glBufferAttachMemoryNV)( GLenum target, GLuint memory, GLuint64 offset );
        void       (WINE_GLAPI *p_glBufferData)( GLenum target, GLsizeiptr size, const void *data, GLenum usage );
        void       (WINE_GLAPI *p_glBufferDataARB)( GLenum target, GLsizeiptrARB size, const void *data, GLenum usage );
        void       (WINE_GLAPI *p_glBufferPageCommitmentARB)( GLenum target, GLintptr offset, GLsizeiptr size, GLboolean commit );
        void       (WINE_GLAPI *p_glBufferParameteriAPPLE)( GLenum target, GLenum pname, GLint param );
        GLuint     (WINE_GLAPI *p_glBufferRegionEnabled)(void);
        void       (WINE_GLAPI *p_glBufferStorage)( GLenum target, GLsizeiptr size, const void *data, GLbitfield flags );
        void       (WINE_GLAPI *p_glBufferStorageExternalEXT)( GLenum target, GLintptr offset, GLsizeiptr size, GLeglClientBufferEXT clientBuffer, GLbitfield flags );
        void       (WINE_GLAPI *p_glBufferStorageMemEXT)( GLenum target, GLsizeiptr size, GLuint memory, GLuint64 offset );
        void       (WINE_GLAPI *p_glBufferSubData)( GLenum target, GLintptr offset, GLsizeiptr size, const void *data );
        void       (WINE_GLAPI *p_glBufferSubDataARB)( GLenum target, GLintptrARB offset, GLsizeiptrARB size, const void *data );
        void       (WINE_GLAPI *p_glCallCommandListNV)( GLuint list );
        GLenum     (WINE_GLAPI *p_glCheckFramebufferStatus)( GLenum target );
        GLenum     (WINE_GLAPI *p_glCheckFramebufferStatusEXT)( GLenum target );
        GLenum     (WINE_GLAPI *p_glCheckNamedFramebufferStatus)( GLuint framebuffer, GLenum target );
        GLenum     (WINE_GLAPI *p_glCheckNamedFramebufferStatusEXT)( GLuint framebuffer, GLenum target );
        void       (WINE_GLAPI *p_glClampColor)( GLenum target, GLenum clamp );
        void       (WINE_GLAPI *p_glClampColorARB)( GLenum target, GLenum clamp );
        void       (WINE_GLAPI *p_glClearAccumxOES)( GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha );
        void       (WINE_GLAPI *p_glClearBufferData)( GLenum target, GLenum internalformat, GLenum format, GLenum type, const void *data );
        void       (WINE_GLAPI *p_glClearBufferSubData)( GLenum target, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data );
        void       (WINE_GLAPI *p_glClearBufferfi)( GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil );
        void       (WINE_GLAPI *p_glClearBufferfv)( GLenum buffer, GLint drawbuffer, const GLfloat *value );
        void       (WINE_GLAPI *p_glClearBufferiv)( GLenum buffer, GLint drawbuffer, const GLint *value );
        void       (WINE_GLAPI *p_glClearBufferuiv)( GLenum buffer, GLint drawbuffer, const GLuint *value );
        void       (WINE_GLAPI *p_glClearColorIiEXT)( GLint red, GLint green, GLint blue, GLint alpha );
        void       (WINE_GLAPI *p_glClearColorIuiEXT)( GLuint red, GLuint green, GLuint blue, GLuint alpha );
        void       (WINE_GLAPI *p_glClearColorxOES)( GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha );
        void       (WINE_GLAPI *p_glClearDepthdNV)( GLdouble depth );
        void       (WINE_GLAPI *p_glClearDepthf)( GLfloat d );
        void       (WINE_GLAPI *p_glClearDepthfOES)( GLclampf depth );
        void       (WINE_GLAPI *p_glClearDepthxOES)( GLfixed depth );
        void       (WINE_GLAPI *p_glClearNamedBufferData)( GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data );
        void       (WINE_GLAPI *p_glClearNamedBufferDataEXT)( GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data );
        void       (WINE_GLAPI *p_glClearNamedBufferSubData)( GLuint buffer, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data );
        void       (WINE_GLAPI *p_glClearNamedBufferSubDataEXT)( GLuint buffer, GLenum internalformat, GLsizeiptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data );
        void       (WINE_GLAPI *p_glClearNamedFramebufferfi)( GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil );
        void       (WINE_GLAPI *p_glClearNamedFramebufferfv)( GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat *value );
        void       (WINE_GLAPI *p_glClearNamedFramebufferiv)( GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint *value );
        void       (WINE_GLAPI *p_glClearNamedFramebufferuiv)( GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint *value );
        void       (WINE_GLAPI *p_glClearTexImage)( GLuint texture, GLint level, GLenum format, GLenum type, const void *data );
        void       (WINE_GLAPI *p_glClearTexSubImage)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *data );
        void       (WINE_GLAPI *p_glClientActiveTexture)( GLenum texture );
        void       (WINE_GLAPI *p_glClientActiveTextureARB)( GLenum texture );
        void       (WINE_GLAPI *p_glClientActiveVertexStreamATI)( GLenum stream );
        void       (WINE_GLAPI *p_glClientAttribDefaultEXT)( GLbitfield mask );
        void       (WINE_GLAPI *p_glClientWaitSemaphoreui64NVX)( GLsizei fenceObjectCount, const GLuint *semaphoreArray, const GLuint64 *fenceValueArray );
        GLenum     (WINE_GLAPI *p_glClientWaitSync)( GLsync sync, GLbitfield flags, GLuint64 timeout );
        void       (WINE_GLAPI *p_glClipControl)( GLenum origin, GLenum depth );
        void       (WINE_GLAPI *p_glClipPlanefOES)( GLenum plane, const GLfloat *equation );
        void       (WINE_GLAPI *p_glClipPlanexOES)( GLenum plane, const GLfixed *equation );
        void       (WINE_GLAPI *p_glColor3fVertex3fSUN)( GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glColor3fVertex3fvSUN)( const GLfloat *c, const GLfloat *v );
        void       (WINE_GLAPI *p_glColor3hNV)( GLhalfNV red, GLhalfNV green, GLhalfNV blue );
        void       (WINE_GLAPI *p_glColor3hvNV)( const GLhalfNV *v );
        void       (WINE_GLAPI *p_glColor3xOES)( GLfixed red, GLfixed green, GLfixed blue );
        void       (WINE_GLAPI *p_glColor3xvOES)( const GLfixed *components );
        void       (WINE_GLAPI *p_glColor4fNormal3fVertex3fSUN)( GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glColor4fNormal3fVertex3fvSUN)( const GLfloat *c, const GLfloat *n, const GLfloat *v );
        void       (WINE_GLAPI *p_glColor4hNV)( GLhalfNV red, GLhalfNV green, GLhalfNV blue, GLhalfNV alpha );
        void       (WINE_GLAPI *p_glColor4hvNV)( const GLhalfNV *v );
        void       (WINE_GLAPI *p_glColor4ubVertex2fSUN)( GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y );
        void       (WINE_GLAPI *p_glColor4ubVertex2fvSUN)( const GLubyte *c, const GLfloat *v );
        void       (WINE_GLAPI *p_glColor4ubVertex3fSUN)( GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glColor4ubVertex3fvSUN)( const GLubyte *c, const GLfloat *v );
        void       (WINE_GLAPI *p_glColor4xOES)( GLfixed red, GLfixed green, GLfixed blue, GLfixed alpha );
        void       (WINE_GLAPI *p_glColor4xvOES)( const GLfixed *components );
        void       (WINE_GLAPI *p_glColorFormatNV)( GLint size, GLenum type, GLsizei stride );
        void       (WINE_GLAPI *p_glColorFragmentOp1ATI)( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod );
        void       (WINE_GLAPI *p_glColorFragmentOp2ATI)( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod );
        void       (WINE_GLAPI *p_glColorFragmentOp3ATI)( GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod );
        void       (WINE_GLAPI *p_glColorMaskIndexedEXT)( GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a );
        void       (WINE_GLAPI *p_glColorMaski)( GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a );
        void       (WINE_GLAPI *p_glColorP3ui)( GLenum type, GLuint color );
        void       (WINE_GLAPI *p_glColorP3uiv)( GLenum type, const GLuint *color );
        void       (WINE_GLAPI *p_glColorP4ui)( GLenum type, GLuint color );
        void       (WINE_GLAPI *p_glColorP4uiv)( GLenum type, const GLuint *color );
        void       (WINE_GLAPI *p_glColorPointerEXT)( GLint size, GLenum type, GLsizei stride, GLsizei count, const void *pointer );
        void       (WINE_GLAPI *p_glColorPointerListIBM)( GLint size, GLenum type, GLint stride, const void **pointer, GLint ptrstride );
        void       (WINE_GLAPI *p_glColorPointervINTEL)( GLint size, GLenum type, const void **pointer );
        void       (WINE_GLAPI *p_glColorSubTable)( GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const void *data );
        void       (WINE_GLAPI *p_glColorSubTableEXT)( GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const void *data );
        void       (WINE_GLAPI *p_glColorTable)( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *table );
        void       (WINE_GLAPI *p_glColorTableEXT)( GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const void *table );
        void       (WINE_GLAPI *p_glColorTableParameterfv)( GLenum target, GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glColorTableParameterfvSGI)( GLenum target, GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glColorTableParameteriv)( GLenum target, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glColorTableParameterivSGI)( GLenum target, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glColorTableSGI)( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *table );
        void       (WINE_GLAPI *p_glCombinerInputNV)( GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage );
        void       (WINE_GLAPI *p_glCombinerOutputNV)( GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum );
        void       (WINE_GLAPI *p_glCombinerParameterfNV)( GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glCombinerParameterfvNV)( GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glCombinerParameteriNV)( GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glCombinerParameterivNV)( GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glCombinerStageParameterfvNV)( GLenum stage, GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glCommandListSegmentsNV)( GLuint list, GLuint segments );
        void       (WINE_GLAPI *p_glCompileCommandListNV)( GLuint list );
        void       (WINE_GLAPI *p_glCompileShader)( GLuint shader );
        void       (WINE_GLAPI *p_glCompileShaderARB)( GLhandleARB shaderObj );
        void       (WINE_GLAPI *p_glCompileShaderIncludeARB)( GLuint shader, GLsizei count, const GLchar *const*path, const GLint *length );
        void       (WINE_GLAPI *p_glCompressedMultiTexImage1DEXT)( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *bits );
        void       (WINE_GLAPI *p_glCompressedMultiTexImage2DEXT)( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *bits );
        void       (WINE_GLAPI *p_glCompressedMultiTexImage3DEXT)( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *bits );
        void       (WINE_GLAPI *p_glCompressedMultiTexSubImage1DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *bits );
        void       (WINE_GLAPI *p_glCompressedMultiTexSubImage2DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *bits );
        void       (WINE_GLAPI *p_glCompressedMultiTexSubImage3DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *bits );
        void       (WINE_GLAPI *p_glCompressedTexImage1D)( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data );
        void       (WINE_GLAPI *p_glCompressedTexImage1DARB)( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data );
        void       (WINE_GLAPI *p_glCompressedTexImage2D)( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data );
        void       (WINE_GLAPI *p_glCompressedTexImage2DARB)( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data );
        void       (WINE_GLAPI *p_glCompressedTexImage3D)( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data );
        void       (WINE_GLAPI *p_glCompressedTexImage3DARB)( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data );
        void       (WINE_GLAPI *p_glCompressedTexSubImage1D)( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data );
        void       (WINE_GLAPI *p_glCompressedTexSubImage1DARB)( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data );
        void       (WINE_GLAPI *p_glCompressedTexSubImage2D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data );
        void       (WINE_GLAPI *p_glCompressedTexSubImage2DARB)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data );
        void       (WINE_GLAPI *p_glCompressedTexSubImage3D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data );
        void       (WINE_GLAPI *p_glCompressedTexSubImage3DARB)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data );
        void       (WINE_GLAPI *p_glCompressedTextureImage1DEXT)( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *bits );
        void       (WINE_GLAPI *p_glCompressedTextureImage2DEXT)( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *bits );
        void       (WINE_GLAPI *p_glCompressedTextureImage3DEXT)( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *bits );
        void       (WINE_GLAPI *p_glCompressedTextureSubImage1D)( GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data );
        void       (WINE_GLAPI *p_glCompressedTextureSubImage1DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *bits );
        void       (WINE_GLAPI *p_glCompressedTextureSubImage2D)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data );
        void       (WINE_GLAPI *p_glCompressedTextureSubImage2DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *bits );
        void       (WINE_GLAPI *p_glCompressedTextureSubImage3D)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data );
        void       (WINE_GLAPI *p_glCompressedTextureSubImage3DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *bits );
        void       (WINE_GLAPI *p_glConservativeRasterParameterfNV)( GLenum pname, GLfloat value );
        void       (WINE_GLAPI *p_glConservativeRasterParameteriNV)( GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glConvolutionFilter1D)( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *image );
        void       (WINE_GLAPI *p_glConvolutionFilter1DEXT)( GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const void *image );
        void       (WINE_GLAPI *p_glConvolutionFilter2D)( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *image );
        void       (WINE_GLAPI *p_glConvolutionFilter2DEXT)( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *image );
        void       (WINE_GLAPI *p_glConvolutionParameterf)( GLenum target, GLenum pname, GLfloat params );
        void       (WINE_GLAPI *p_glConvolutionParameterfEXT)( GLenum target, GLenum pname, GLfloat params );
        void       (WINE_GLAPI *p_glConvolutionParameterfv)( GLenum target, GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glConvolutionParameterfvEXT)( GLenum target, GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glConvolutionParameteri)( GLenum target, GLenum pname, GLint params );
        void       (WINE_GLAPI *p_glConvolutionParameteriEXT)( GLenum target, GLenum pname, GLint params );
        void       (WINE_GLAPI *p_glConvolutionParameteriv)( GLenum target, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glConvolutionParameterivEXT)( GLenum target, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glConvolutionParameterxOES)( GLenum target, GLenum pname, GLfixed param );
        void       (WINE_GLAPI *p_glConvolutionParameterxvOES)( GLenum target, GLenum pname, const GLfixed *params );
        void       (WINE_GLAPI *p_glCopyBufferSubData)( GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size );
        void       (WINE_GLAPI *p_glCopyColorSubTable)( GLenum target, GLsizei start, GLint x, GLint y, GLsizei width );
        void       (WINE_GLAPI *p_glCopyColorSubTableEXT)( GLenum target, GLsizei start, GLint x, GLint y, GLsizei width );
        void       (WINE_GLAPI *p_glCopyColorTable)( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width );
        void       (WINE_GLAPI *p_glCopyColorTableSGI)( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width );
        void       (WINE_GLAPI *p_glCopyConvolutionFilter1D)( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width );
        void       (WINE_GLAPI *p_glCopyConvolutionFilter1DEXT)( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width );
        void       (WINE_GLAPI *p_glCopyConvolutionFilter2D)( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glCopyConvolutionFilter2DEXT)( GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glCopyImageSubData)( GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth );
        void       (WINE_GLAPI *p_glCopyImageSubDataNV)( GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei width, GLsizei height, GLsizei depth );
        void       (WINE_GLAPI *p_glCopyMultiTexImage1DEXT)( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border );
        void       (WINE_GLAPI *p_glCopyMultiTexImage2DEXT)( GLenum texunit, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border );
        void       (WINE_GLAPI *p_glCopyMultiTexSubImage1DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width );
        void       (WINE_GLAPI *p_glCopyMultiTexSubImage2DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glCopyMultiTexSubImage3DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glCopyNamedBufferSubData)( GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size );
        void       (WINE_GLAPI *p_glCopyPathNV)( GLuint resultPath, GLuint srcPath );
        void       (WINE_GLAPI *p_glCopyTexImage1DEXT)( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border );
        void       (WINE_GLAPI *p_glCopyTexImage2DEXT)( GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border );
        void       (WINE_GLAPI *p_glCopyTexSubImage1DEXT)( GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width );
        void       (WINE_GLAPI *p_glCopyTexSubImage2DEXT)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glCopyTexSubImage3D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glCopyTexSubImage3DEXT)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glCopyTextureImage1DEXT)( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border );
        void       (WINE_GLAPI *p_glCopyTextureImage2DEXT)( GLuint texture, GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border );
        void       (WINE_GLAPI *p_glCopyTextureSubImage1D)( GLuint texture, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width );
        void       (WINE_GLAPI *p_glCopyTextureSubImage1DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width );
        void       (WINE_GLAPI *p_glCopyTextureSubImage2D)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glCopyTextureSubImage2DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glCopyTextureSubImage3D)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glCopyTextureSubImage3DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glCoverFillPathInstancedNV)( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues );
        void       (WINE_GLAPI *p_glCoverFillPathNV)( GLuint path, GLenum coverMode );
        void       (WINE_GLAPI *p_glCoverStrokePathInstancedNV)( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum coverMode, GLenum transformType, const GLfloat *transformValues );
        void       (WINE_GLAPI *p_glCoverStrokePathNV)( GLuint path, GLenum coverMode );
        void       (WINE_GLAPI *p_glCoverageModulationNV)( GLenum components );
        void       (WINE_GLAPI *p_glCoverageModulationTableNV)( GLsizei n, const GLfloat *v );
        void       (WINE_GLAPI *p_glCreateBuffers)( GLsizei n, GLuint *buffers );
        void       (WINE_GLAPI *p_glCreateCommandListsNV)( GLsizei n, GLuint *lists );
        void       (WINE_GLAPI *p_glCreateFramebuffers)( GLsizei n, GLuint *framebuffers );
        void       (WINE_GLAPI *p_glCreateMemoryObjectsEXT)( GLsizei n, GLuint *memoryObjects );
        void       (WINE_GLAPI *p_glCreatePerfQueryINTEL)( GLuint queryId, GLuint *queryHandle );
        GLuint     (WINE_GLAPI *p_glCreateProgram)(void);
        GLhandleARB (WINE_GLAPI *p_glCreateProgramObjectARB)(void);
        void       (WINE_GLAPI *p_glCreateProgramPipelines)( GLsizei n, GLuint *pipelines );
        GLuint     (WINE_GLAPI *p_glCreateProgressFenceNVX)(void);
        void       (WINE_GLAPI *p_glCreateQueries)( GLenum target, GLsizei n, GLuint *ids );
        void       (WINE_GLAPI *p_glCreateRenderbuffers)( GLsizei n, GLuint *renderbuffers );
        void       (WINE_GLAPI *p_glCreateSamplers)( GLsizei n, GLuint *samplers );
        GLuint     (WINE_GLAPI *p_glCreateShader)( GLenum type );
        GLhandleARB (WINE_GLAPI *p_glCreateShaderObjectARB)( GLenum shaderType );
        GLuint     (WINE_GLAPI *p_glCreateShaderProgramEXT)( GLenum type, const GLchar *string );
        GLuint     (WINE_GLAPI *p_glCreateShaderProgramv)( GLenum type, GLsizei count, const GLchar *const*strings );
        void       (WINE_GLAPI *p_glCreateStatesNV)( GLsizei n, GLuint *states );
        GLsync     (WINE_GLAPI *p_glCreateSyncFromCLeventARB)( struct _cl_context *context, struct _cl_event *event, GLbitfield flags );
        void       (WINE_GLAPI *p_glCreateTextures)( GLenum target, GLsizei n, GLuint *textures );
        void       (WINE_GLAPI *p_glCreateTransformFeedbacks)( GLsizei n, GLuint *ids );
        void       (WINE_GLAPI *p_glCreateVertexArrays)( GLsizei n, GLuint *arrays );
        void       (WINE_GLAPI *p_glCullParameterdvEXT)( GLenum pname, GLdouble *params );
        void       (WINE_GLAPI *p_glCullParameterfvEXT)( GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glCurrentPaletteMatrixARB)( GLint index );
        void       (WINE_GLAPI *p_glDebugMessageCallback)( GLDEBUGPROC callback, const void *userParam );
        void       (WINE_GLAPI *p_glDebugMessageCallbackAMD)( GLDEBUGPROCAMD callback, void *userParam );
        void       (WINE_GLAPI *p_glDebugMessageCallbackARB)( GLDEBUGPROCARB callback, const void *userParam );
        void       (WINE_GLAPI *p_glDebugMessageControl)( GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled );
        void       (WINE_GLAPI *p_glDebugMessageControlARB)( GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled );
        void       (WINE_GLAPI *p_glDebugMessageEnableAMD)( GLenum category, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled );
        void       (WINE_GLAPI *p_glDebugMessageInsert)( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf );
        void       (WINE_GLAPI *p_glDebugMessageInsertAMD)( GLenum category, GLenum severity, GLuint id, GLsizei length, const GLchar *buf );
        void       (WINE_GLAPI *p_glDebugMessageInsertARB)( GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf );
        void       (WINE_GLAPI *p_glDeformSGIX)( GLbitfield mask );
        void       (WINE_GLAPI *p_glDeformationMap3dSGIX)( GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, GLdouble w1, GLdouble w2, GLint wstride, GLint worder, const GLdouble *points );
        void       (WINE_GLAPI *p_glDeformationMap3fSGIX)( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, GLfloat w1, GLfloat w2, GLint wstride, GLint worder, const GLfloat *points );
        void       (WINE_GLAPI *p_glDeleteAsyncMarkersSGIX)( GLuint marker, GLsizei range );
        void       (WINE_GLAPI *p_glDeleteBufferRegion)( GLenum region );
        void       (WINE_GLAPI *p_glDeleteBuffers)( GLsizei n, const GLuint *buffers );
        void       (WINE_GLAPI *p_glDeleteBuffersARB)( GLsizei n, const GLuint *buffers );
        void       (WINE_GLAPI *p_glDeleteCommandListsNV)( GLsizei n, const GLuint *lists );
        void       (WINE_GLAPI *p_glDeleteFencesAPPLE)( GLsizei n, const GLuint *fences );
        void       (WINE_GLAPI *p_glDeleteFencesNV)( GLsizei n, const GLuint *fences );
        void       (WINE_GLAPI *p_glDeleteFragmentShaderATI)( GLuint id );
        void       (WINE_GLAPI *p_glDeleteFramebuffers)( GLsizei n, const GLuint *framebuffers );
        void       (WINE_GLAPI *p_glDeleteFramebuffersEXT)( GLsizei n, const GLuint *framebuffers );
        void       (WINE_GLAPI *p_glDeleteMemoryObjectsEXT)( GLsizei n, const GLuint *memoryObjects );
        void       (WINE_GLAPI *p_glDeleteNamedStringARB)( GLint namelen, const GLchar *name );
        void       (WINE_GLAPI *p_glDeleteNamesAMD)( GLenum identifier, GLuint num, const GLuint *names );
        void       (WINE_GLAPI *p_glDeleteObjectARB)( GLhandleARB obj );
        void       (WINE_GLAPI *p_glDeleteObjectBufferATI)( GLuint buffer );
        void       (WINE_GLAPI *p_glDeleteOcclusionQueriesNV)( GLsizei n, const GLuint *ids );
        void       (WINE_GLAPI *p_glDeletePathsNV)( GLuint path, GLsizei range );
        void       (WINE_GLAPI *p_glDeletePerfMonitorsAMD)( GLsizei n, GLuint *monitors );
        void       (WINE_GLAPI *p_glDeletePerfQueryINTEL)( GLuint queryHandle );
        void       (WINE_GLAPI *p_glDeleteProgram)( GLuint program );
        void       (WINE_GLAPI *p_glDeleteProgramPipelines)( GLsizei n, const GLuint *pipelines );
        void       (WINE_GLAPI *p_glDeleteProgramsARB)( GLsizei n, const GLuint *programs );
        void       (WINE_GLAPI *p_glDeleteProgramsNV)( GLsizei n, const GLuint *programs );
        void       (WINE_GLAPI *p_glDeleteQueries)( GLsizei n, const GLuint *ids );
        void       (WINE_GLAPI *p_glDeleteQueriesARB)( GLsizei n, const GLuint *ids );
        void       (WINE_GLAPI *p_glDeleteQueryResourceTagNV)( GLsizei n, const GLint *tagIds );
        void       (WINE_GLAPI *p_glDeleteRenderbuffers)( GLsizei n, const GLuint *renderbuffers );
        void       (WINE_GLAPI *p_glDeleteRenderbuffersEXT)( GLsizei n, const GLuint *renderbuffers );
        void       (WINE_GLAPI *p_glDeleteSamplers)( GLsizei count, const GLuint *samplers );
        void       (WINE_GLAPI *p_glDeleteSemaphoresEXT)( GLsizei n, const GLuint *semaphores );
        void       (WINE_GLAPI *p_glDeleteShader)( GLuint shader );
        void       (WINE_GLAPI *p_glDeleteStatesNV)( GLsizei n, const GLuint *states );
        void       (WINE_GLAPI *p_glDeleteSync)( GLsync sync );
        void       (WINE_GLAPI *p_glDeleteTexturesEXT)( GLsizei n, const GLuint *textures );
        void       (WINE_GLAPI *p_glDeleteTransformFeedbacks)( GLsizei n, const GLuint *ids );
        void       (WINE_GLAPI *p_glDeleteTransformFeedbacksNV)( GLsizei n, const GLuint *ids );
        void       (WINE_GLAPI *p_glDeleteVertexArrays)( GLsizei n, const GLuint *arrays );
        void       (WINE_GLAPI *p_glDeleteVertexArraysAPPLE)( GLsizei n, const GLuint *arrays );
        void       (WINE_GLAPI *p_glDeleteVertexShaderEXT)( GLuint id );
        void       (WINE_GLAPI *p_glDepthBoundsEXT)( GLclampd zmin, GLclampd zmax );
        void       (WINE_GLAPI *p_glDepthBoundsdNV)( GLdouble zmin, GLdouble zmax );
        void       (WINE_GLAPI *p_glDepthRangeArraydvNV)( GLuint first, GLsizei count, const GLdouble *v );
        void       (WINE_GLAPI *p_glDepthRangeArrayv)( GLuint first, GLsizei count, const GLdouble *v );
        void       (WINE_GLAPI *p_glDepthRangeIndexed)( GLuint index, GLdouble n, GLdouble f );
        void       (WINE_GLAPI *p_glDepthRangeIndexeddNV)( GLuint index, GLdouble n, GLdouble f );
        void       (WINE_GLAPI *p_glDepthRangedNV)( GLdouble zNear, GLdouble zFar );
        void       (WINE_GLAPI *p_glDepthRangef)( GLfloat n, GLfloat f );
        void       (WINE_GLAPI *p_glDepthRangefOES)( GLclampf n, GLclampf f );
        void       (WINE_GLAPI *p_glDepthRangexOES)( GLfixed n, GLfixed f );
        void       (WINE_GLAPI *p_glDetachObjectARB)( GLhandleARB containerObj, GLhandleARB attachedObj );
        void       (WINE_GLAPI *p_glDetachShader)( GLuint program, GLuint shader );
        void       (WINE_GLAPI *p_glDetailTexFuncSGIS)( GLenum target, GLsizei n, const GLfloat *points );
        void       (WINE_GLAPI *p_glDisableClientStateIndexedEXT)( GLenum array, GLuint index );
        void       (WINE_GLAPI *p_glDisableClientStateiEXT)( GLenum array, GLuint index );
        void       (WINE_GLAPI *p_glDisableIndexedEXT)( GLenum target, GLuint index );
        void       (WINE_GLAPI *p_glDisableVariantClientStateEXT)( GLuint id );
        void       (WINE_GLAPI *p_glDisableVertexArrayAttrib)( GLuint vaobj, GLuint index );
        void       (WINE_GLAPI *p_glDisableVertexArrayAttribEXT)( GLuint vaobj, GLuint index );
        void       (WINE_GLAPI *p_glDisableVertexArrayEXT)( GLuint vaobj, GLenum array );
        void       (WINE_GLAPI *p_glDisableVertexAttribAPPLE)( GLuint index, GLenum pname );
        void       (WINE_GLAPI *p_glDisableVertexAttribArray)( GLuint index );
        void       (WINE_GLAPI *p_glDisableVertexAttribArrayARB)( GLuint index );
        void       (WINE_GLAPI *p_glDisablei)( GLenum target, GLuint index );
        void       (WINE_GLAPI *p_glDispatchCompute)( GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z );
        void       (WINE_GLAPI *p_glDispatchComputeGroupSizeARB)( GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z, GLuint group_size_x, GLuint group_size_y, GLuint group_size_z );
        void       (WINE_GLAPI *p_glDispatchComputeIndirect)( GLintptr indirect );
        void       (WINE_GLAPI *p_glDrawArraysEXT)( GLenum mode, GLint first, GLsizei count );
        void       (WINE_GLAPI *p_glDrawArraysIndirect)( GLenum mode, const void *indirect );
        void       (WINE_GLAPI *p_glDrawArraysInstanced)( GLenum mode, GLint first, GLsizei count, GLsizei instancecount );
        void       (WINE_GLAPI *p_glDrawArraysInstancedARB)( GLenum mode, GLint first, GLsizei count, GLsizei primcount );
        void       (WINE_GLAPI *p_glDrawArraysInstancedBaseInstance)( GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance );
        void       (WINE_GLAPI *p_glDrawArraysInstancedEXT)( GLenum mode, GLint start, GLsizei count, GLsizei primcount );
        void       (WINE_GLAPI *p_glDrawBufferRegion)( GLenum region, GLint x, GLint y, GLsizei width, GLsizei height, GLint xDest, GLint yDest );
        void       (WINE_GLAPI *p_glDrawBuffers)( GLsizei n, const GLenum *bufs );
        void       (WINE_GLAPI *p_glDrawBuffersARB)( GLsizei n, const GLenum *bufs );
        void       (WINE_GLAPI *p_glDrawBuffersATI)( GLsizei n, const GLenum *bufs );
        void       (WINE_GLAPI *p_glDrawCommandsAddressNV)( GLenum primitiveMode, const GLuint64 *indirects, const GLsizei *sizes, GLuint count );
        void       (WINE_GLAPI *p_glDrawCommandsNV)( GLenum primitiveMode, GLuint buffer, const GLintptr *indirects, const GLsizei *sizes, GLuint count );
        void       (WINE_GLAPI *p_glDrawCommandsStatesAddressNV)( const GLuint64 *indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count );
        void       (WINE_GLAPI *p_glDrawCommandsStatesNV)( GLuint buffer, const GLintptr *indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count );
        void       (WINE_GLAPI *p_glDrawElementArrayAPPLE)( GLenum mode, GLint first, GLsizei count );
        void       (WINE_GLAPI *p_glDrawElementArrayATI)( GLenum mode, GLsizei count );
        void       (WINE_GLAPI *p_glDrawElementsBaseVertex)( GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex );
        void       (WINE_GLAPI *p_glDrawElementsIndirect)( GLenum mode, GLenum type, const void *indirect );
        void       (WINE_GLAPI *p_glDrawElementsInstanced)( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount );
        void       (WINE_GLAPI *p_glDrawElementsInstancedARB)( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount );
        void       (WINE_GLAPI *p_glDrawElementsInstancedBaseInstance)( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance );
        void       (WINE_GLAPI *p_glDrawElementsInstancedBaseVertex)( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex );
        void       (WINE_GLAPI *p_glDrawElementsInstancedBaseVertexBaseInstance)( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance );
        void       (WINE_GLAPI *p_glDrawElementsInstancedEXT)( GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei primcount );
        void       (WINE_GLAPI *p_glDrawMeshArraysSUN)( GLenum mode, GLint first, GLsizei count, GLsizei width );
        void       (WINE_GLAPI *p_glDrawMeshTasksIndirectNV)( GLintptr indirect );
        void       (WINE_GLAPI *p_glDrawMeshTasksNV)( GLuint first, GLuint count );
        void       (WINE_GLAPI *p_glDrawRangeElementArrayAPPLE)( GLenum mode, GLuint start, GLuint end, GLint first, GLsizei count );
        void       (WINE_GLAPI *p_glDrawRangeElementArrayATI)( GLenum mode, GLuint start, GLuint end, GLsizei count );
        void       (WINE_GLAPI *p_glDrawRangeElements)( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices );
        void       (WINE_GLAPI *p_glDrawRangeElementsBaseVertex)( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex );
        void       (WINE_GLAPI *p_glDrawRangeElementsEXT)( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices );
        void       (WINE_GLAPI *p_glDrawTextureNV)( GLuint texture, GLuint sampler, GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1, GLfloat z, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1 );
        void       (WINE_GLAPI *p_glDrawTransformFeedback)( GLenum mode, GLuint id );
        void       (WINE_GLAPI *p_glDrawTransformFeedbackInstanced)( GLenum mode, GLuint id, GLsizei instancecount );
        void       (WINE_GLAPI *p_glDrawTransformFeedbackNV)( GLenum mode, GLuint id );
        void       (WINE_GLAPI *p_glDrawTransformFeedbackStream)( GLenum mode, GLuint id, GLuint stream );
        void       (WINE_GLAPI *p_glDrawTransformFeedbackStreamInstanced)( GLenum mode, GLuint id, GLuint stream, GLsizei instancecount );
        void       (WINE_GLAPI *p_glDrawVkImageNV)( GLuint64 vkImage, GLuint sampler, GLfloat x0, GLfloat y0, GLfloat x1, GLfloat y1, GLfloat z, GLfloat s0, GLfloat t0, GLfloat s1, GLfloat t1 );
        void       (WINE_GLAPI *p_glEGLImageTargetTexStorageEXT)( GLenum target, GLeglImageOES image, const GLint* attrib_list );
        void       (WINE_GLAPI *p_glEGLImageTargetTextureStorageEXT)( GLuint texture, GLeglImageOES image, const GLint* attrib_list );
        void       (WINE_GLAPI *p_glEdgeFlagFormatNV)( GLsizei stride );
        void       (WINE_GLAPI *p_glEdgeFlagPointerEXT)( GLsizei stride, GLsizei count, const GLboolean *pointer );
        void       (WINE_GLAPI *p_glEdgeFlagPointerListIBM)( GLint stride, const GLboolean **pointer, GLint ptrstride );
        void       (WINE_GLAPI *p_glElementPointerAPPLE)( GLenum type, const void *pointer );
        void       (WINE_GLAPI *p_glElementPointerATI)( GLenum type, const void *pointer );
        void       (WINE_GLAPI *p_glEnableClientStateIndexedEXT)( GLenum array, GLuint index );
        void       (WINE_GLAPI *p_glEnableClientStateiEXT)( GLenum array, GLuint index );
        void       (WINE_GLAPI *p_glEnableIndexedEXT)( GLenum target, GLuint index );
        void       (WINE_GLAPI *p_glEnableVariantClientStateEXT)( GLuint id );
        void       (WINE_GLAPI *p_glEnableVertexArrayAttrib)( GLuint vaobj, GLuint index );
        void       (WINE_GLAPI *p_glEnableVertexArrayAttribEXT)( GLuint vaobj, GLuint index );
        void       (WINE_GLAPI *p_glEnableVertexArrayEXT)( GLuint vaobj, GLenum array );
        void       (WINE_GLAPI *p_glEnableVertexAttribAPPLE)( GLuint index, GLenum pname );
        void       (WINE_GLAPI *p_glEnableVertexAttribArray)( GLuint index );
        void       (WINE_GLAPI *p_glEnableVertexAttribArrayARB)( GLuint index );
        void       (WINE_GLAPI *p_glEnablei)( GLenum target, GLuint index );
        void       (WINE_GLAPI *p_glEndConditionalRender)(void);
        void       (WINE_GLAPI *p_glEndConditionalRenderNV)(void);
        void       (WINE_GLAPI *p_glEndConditionalRenderNVX)(void);
        void       (WINE_GLAPI *p_glEndFragmentShaderATI)(void);
        void       (WINE_GLAPI *p_glEndOcclusionQueryNV)(void);
        void       (WINE_GLAPI *p_glEndPerfMonitorAMD)( GLuint monitor );
        void       (WINE_GLAPI *p_glEndPerfQueryINTEL)( GLuint queryHandle );
        void       (WINE_GLAPI *p_glEndQuery)( GLenum target );
        void       (WINE_GLAPI *p_glEndQueryARB)( GLenum target );
        void       (WINE_GLAPI *p_glEndQueryIndexed)( GLenum target, GLuint index );
        void       (WINE_GLAPI *p_glEndTransformFeedback)(void);
        void       (WINE_GLAPI *p_glEndTransformFeedbackEXT)(void);
        void       (WINE_GLAPI *p_glEndTransformFeedbackNV)(void);
        void       (WINE_GLAPI *p_glEndVertexShaderEXT)(void);
        void       (WINE_GLAPI *p_glEndVideoCaptureNV)( GLuint video_capture_slot );
        void       (WINE_GLAPI *p_glEvalCoord1xOES)( GLfixed u );
        void       (WINE_GLAPI *p_glEvalCoord1xvOES)( const GLfixed *coords );
        void       (WINE_GLAPI *p_glEvalCoord2xOES)( GLfixed u, GLfixed v );
        void       (WINE_GLAPI *p_glEvalCoord2xvOES)( const GLfixed *coords );
        void       (WINE_GLAPI *p_glEvalMapsNV)( GLenum target, GLenum mode );
        void       (WINE_GLAPI *p_glEvaluateDepthValuesARB)(void);
        void       (WINE_GLAPI *p_glExecuteProgramNV)( GLenum target, GLuint id, const GLfloat *params );
        void       (WINE_GLAPI *p_glExtractComponentEXT)( GLuint res, GLuint src, GLuint num );
        void       (WINE_GLAPI *p_glFeedbackBufferxOES)( GLsizei n, GLenum type, const GLfixed *buffer );
        GLsync     (WINE_GLAPI *p_glFenceSync)( GLenum condition, GLbitfield flags );
        void       (WINE_GLAPI *p_glFinalCombinerInputNV)( GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage );
        GLint      (WINE_GLAPI *p_glFinishAsyncSGIX)( GLuint *markerp );
        void       (WINE_GLAPI *p_glFinishFenceAPPLE)( GLuint fence );
        void       (WINE_GLAPI *p_glFinishFenceNV)( GLuint fence );
        void       (WINE_GLAPI *p_glFinishObjectAPPLE)( GLenum object, GLint name );
        void       (WINE_GLAPI *p_glFinishTextureSUNX)(void);
        void       (WINE_GLAPI *p_glFlushMappedBufferRange)( GLenum target, GLintptr offset, GLsizeiptr length );
        void       (WINE_GLAPI *p_glFlushMappedBufferRangeAPPLE)( GLenum target, GLintptr offset, GLsizeiptr size );
        void       (WINE_GLAPI *p_glFlushMappedNamedBufferRange)( GLuint buffer, GLintptr offset, GLsizeiptr length );
        void       (WINE_GLAPI *p_glFlushMappedNamedBufferRangeEXT)( GLuint buffer, GLintptr offset, GLsizeiptr length );
        void       (WINE_GLAPI *p_glFlushPixelDataRangeNV)( GLenum target );
        void       (WINE_GLAPI *p_glFlushRasterSGIX)(void);
        void       (WINE_GLAPI *p_glFlushStaticDataIBM)( GLenum target );
        void       (WINE_GLAPI *p_glFlushVertexArrayRangeAPPLE)( GLsizei length, void *pointer );
        void       (WINE_GLAPI *p_glFlushVertexArrayRangeNV)(void);
        void       (WINE_GLAPI *p_glFogCoordFormatNV)( GLenum type, GLsizei stride );
        void       (WINE_GLAPI *p_glFogCoordPointer)( GLenum type, GLsizei stride, const void *pointer );
        void       (WINE_GLAPI *p_glFogCoordPointerEXT)( GLenum type, GLsizei stride, const void *pointer );
        void       (WINE_GLAPI *p_glFogCoordPointerListIBM)( GLenum type, GLint stride, const void **pointer, GLint ptrstride );
        void       (WINE_GLAPI *p_glFogCoordd)( GLdouble coord );
        void       (WINE_GLAPI *p_glFogCoorddEXT)( GLdouble coord );
        void       (WINE_GLAPI *p_glFogCoorddv)( const GLdouble *coord );
        void       (WINE_GLAPI *p_glFogCoorddvEXT)( const GLdouble *coord );
        void       (WINE_GLAPI *p_glFogCoordf)( GLfloat coord );
        void       (WINE_GLAPI *p_glFogCoordfEXT)( GLfloat coord );
        void       (WINE_GLAPI *p_glFogCoordfv)( const GLfloat *coord );
        void       (WINE_GLAPI *p_glFogCoordfvEXT)( const GLfloat *coord );
        void       (WINE_GLAPI *p_glFogCoordhNV)( GLhalfNV fog );
        void       (WINE_GLAPI *p_glFogCoordhvNV)( const GLhalfNV *fog );
        void       (WINE_GLAPI *p_glFogFuncSGIS)( GLsizei n, const GLfloat *points );
        void       (WINE_GLAPI *p_glFogxOES)( GLenum pname, GLfixed param );
        void       (WINE_GLAPI *p_glFogxvOES)( GLenum pname, const GLfixed *param );
        void       (WINE_GLAPI *p_glFragmentColorMaterialSGIX)( GLenum face, GLenum mode );
        void       (WINE_GLAPI *p_glFragmentCoverageColorNV)( GLuint color );
        void       (WINE_GLAPI *p_glFragmentLightModelfSGIX)( GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glFragmentLightModelfvSGIX)( GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glFragmentLightModeliSGIX)( GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glFragmentLightModelivSGIX)( GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glFragmentLightfSGIX)( GLenum light, GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glFragmentLightfvSGIX)( GLenum light, GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glFragmentLightiSGIX)( GLenum light, GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glFragmentLightivSGIX)( GLenum light, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glFragmentMaterialfSGIX)( GLenum face, GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glFragmentMaterialfvSGIX)( GLenum face, GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glFragmentMaterialiSGIX)( GLenum face, GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glFragmentMaterialivSGIX)( GLenum face, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glFrameTerminatorGREMEDY)(void);
        void       (WINE_GLAPI *p_glFrameZoomSGIX)( GLint factor );
        void       (WINE_GLAPI *p_glFramebufferDrawBufferEXT)( GLuint framebuffer, GLenum mode );
        void       (WINE_GLAPI *p_glFramebufferDrawBuffersEXT)( GLuint framebuffer, GLsizei n, const GLenum *bufs );
        void       (WINE_GLAPI *p_glFramebufferFetchBarrierEXT)(void);
        void       (WINE_GLAPI *p_glFramebufferParameteri)( GLenum target, GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glFramebufferParameteriMESA)( GLenum target, GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glFramebufferReadBufferEXT)( GLuint framebuffer, GLenum mode );
        void       (WINE_GLAPI *p_glFramebufferRenderbuffer)( GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer );
        void       (WINE_GLAPI *p_glFramebufferRenderbufferEXT)( GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer );
        void       (WINE_GLAPI *p_glFramebufferSampleLocationsfvARB)( GLenum target, GLuint start, GLsizei count, const GLfloat *v );
        void       (WINE_GLAPI *p_glFramebufferSampleLocationsfvNV)( GLenum target, GLuint start, GLsizei count, const GLfloat *v );
        void       (WINE_GLAPI *p_glFramebufferSamplePositionsfvAMD)( GLenum target, GLuint numsamples, GLuint pixelindex, const GLfloat *values );
        void       (WINE_GLAPI *p_glFramebufferTexture)( GLenum target, GLenum attachment, GLuint texture, GLint level );
        void       (WINE_GLAPI *p_glFramebufferTexture1D)( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level );
        void       (WINE_GLAPI *p_glFramebufferTexture1DEXT)( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level );
        void       (WINE_GLAPI *p_glFramebufferTexture2D)( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level );
        void       (WINE_GLAPI *p_glFramebufferTexture2DEXT)( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level );
        void       (WINE_GLAPI *p_glFramebufferTexture3D)( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset );
        void       (WINE_GLAPI *p_glFramebufferTexture3DEXT)( GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset );
        void       (WINE_GLAPI *p_glFramebufferTextureARB)( GLenum target, GLenum attachment, GLuint texture, GLint level );
        void       (WINE_GLAPI *p_glFramebufferTextureEXT)( GLenum target, GLenum attachment, GLuint texture, GLint level );
        void       (WINE_GLAPI *p_glFramebufferTextureFaceARB)( GLenum target, GLenum attachment, GLuint texture, GLint level, GLenum face );
        void       (WINE_GLAPI *p_glFramebufferTextureFaceEXT)( GLenum target, GLenum attachment, GLuint texture, GLint level, GLenum face );
        void       (WINE_GLAPI *p_glFramebufferTextureLayer)( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer );
        void       (WINE_GLAPI *p_glFramebufferTextureLayerARB)( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer );
        void       (WINE_GLAPI *p_glFramebufferTextureLayerEXT)( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer );
        void       (WINE_GLAPI *p_glFramebufferTextureMultiviewOVR)( GLenum target, GLenum attachment, GLuint texture, GLint level, GLint baseViewIndex, GLsizei numViews );
        void       (WINE_GLAPI *p_glFreeObjectBufferATI)( GLuint buffer );
        void       (WINE_GLAPI *p_glFrustumfOES)( GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f );
        void       (WINE_GLAPI *p_glFrustumxOES)( GLfixed l, GLfixed r, GLfixed b, GLfixed t, GLfixed n, GLfixed f );
        GLuint     (WINE_GLAPI *p_glGenAsyncMarkersSGIX)( GLsizei range );
        void       (WINE_GLAPI *p_glGenBuffers)( GLsizei n, GLuint *buffers );
        void       (WINE_GLAPI *p_glGenBuffersARB)( GLsizei n, GLuint *buffers );
        void       (WINE_GLAPI *p_glGenFencesAPPLE)( GLsizei n, GLuint *fences );
        void       (WINE_GLAPI *p_glGenFencesNV)( GLsizei n, GLuint *fences );
        GLuint     (WINE_GLAPI *p_glGenFragmentShadersATI)( GLuint range );
        void       (WINE_GLAPI *p_glGenFramebuffers)( GLsizei n, GLuint *framebuffers );
        void       (WINE_GLAPI *p_glGenFramebuffersEXT)( GLsizei n, GLuint *framebuffers );
        void       (WINE_GLAPI *p_glGenNamesAMD)( GLenum identifier, GLuint num, GLuint *names );
        void       (WINE_GLAPI *p_glGenOcclusionQueriesNV)( GLsizei n, GLuint *ids );
        GLuint     (WINE_GLAPI *p_glGenPathsNV)( GLsizei range );
        void       (WINE_GLAPI *p_glGenPerfMonitorsAMD)( GLsizei n, GLuint *monitors );
        void       (WINE_GLAPI *p_glGenProgramPipelines)( GLsizei n, GLuint *pipelines );
        void       (WINE_GLAPI *p_glGenProgramsARB)( GLsizei n, GLuint *programs );
        void       (WINE_GLAPI *p_glGenProgramsNV)( GLsizei n, GLuint *programs );
        void       (WINE_GLAPI *p_glGenQueries)( GLsizei n, GLuint *ids );
        void       (WINE_GLAPI *p_glGenQueriesARB)( GLsizei n, GLuint *ids );
        void       (WINE_GLAPI *p_glGenQueryResourceTagNV)( GLsizei n, GLint *tagIds );
        void       (WINE_GLAPI *p_glGenRenderbuffers)( GLsizei n, GLuint *renderbuffers );
        void       (WINE_GLAPI *p_glGenRenderbuffersEXT)( GLsizei n, GLuint *renderbuffers );
        void       (WINE_GLAPI *p_glGenSamplers)( GLsizei count, GLuint *samplers );
        void       (WINE_GLAPI *p_glGenSemaphoresEXT)( GLsizei n, GLuint *semaphores );
        GLuint     (WINE_GLAPI *p_glGenSymbolsEXT)( GLenum datatype, GLenum storagetype, GLenum range, GLuint components );
        void       (WINE_GLAPI *p_glGenTexturesEXT)( GLsizei n, GLuint *textures );
        void       (WINE_GLAPI *p_glGenTransformFeedbacks)( GLsizei n, GLuint *ids );
        void       (WINE_GLAPI *p_glGenTransformFeedbacksNV)( GLsizei n, GLuint *ids );
        void       (WINE_GLAPI *p_glGenVertexArrays)( GLsizei n, GLuint *arrays );
        void       (WINE_GLAPI *p_glGenVertexArraysAPPLE)( GLsizei n, GLuint *arrays );
        GLuint     (WINE_GLAPI *p_glGenVertexShadersEXT)( GLuint range );
        void       (WINE_GLAPI *p_glGenerateMipmap)( GLenum target );
        void       (WINE_GLAPI *p_glGenerateMipmapEXT)( GLenum target );
        void       (WINE_GLAPI *p_glGenerateMultiTexMipmapEXT)( GLenum texunit, GLenum target );
        void       (WINE_GLAPI *p_glGenerateTextureMipmap)( GLuint texture );
        void       (WINE_GLAPI *p_glGenerateTextureMipmapEXT)( GLuint texture, GLenum target );
        void       (WINE_GLAPI *p_glGetActiveAtomicCounterBufferiv)( GLuint program, GLuint bufferIndex, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetActiveAttrib)( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name );
        void       (WINE_GLAPI *p_glGetActiveAttribARB)( GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name );
        void       (WINE_GLAPI *p_glGetActiveSubroutineName)( GLuint program, GLenum shadertype, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name );
        void       (WINE_GLAPI *p_glGetActiveSubroutineUniformName)( GLuint program, GLenum shadertype, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name );
        void       (WINE_GLAPI *p_glGetActiveSubroutineUniformiv)( GLuint program, GLenum shadertype, GLuint index, GLenum pname, GLint *values );
        void       (WINE_GLAPI *p_glGetActiveUniform)( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name );
        void       (WINE_GLAPI *p_glGetActiveUniformARB)( GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name );
        void       (WINE_GLAPI *p_glGetActiveUniformBlockName)( GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName );
        void       (WINE_GLAPI *p_glGetActiveUniformBlockiv)( GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetActiveUniformName)( GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformName );
        void       (WINE_GLAPI *p_glGetActiveUniformsiv)( GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetActiveVaryingNV)( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name );
        void       (WINE_GLAPI *p_glGetArrayObjectfvATI)( GLenum array, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetArrayObjectivATI)( GLenum array, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetAttachedObjectsARB)( GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj );
        void       (WINE_GLAPI *p_glGetAttachedShaders)( GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders );
        GLint      (WINE_GLAPI *p_glGetAttribLocation)( GLuint program, const GLchar *name );
        GLint      (WINE_GLAPI *p_glGetAttribLocationARB)( GLhandleARB programObj, const GLcharARB *name );
        void       (WINE_GLAPI *p_glGetBooleanIndexedvEXT)( GLenum target, GLuint index, GLboolean *data );
        void       (WINE_GLAPI *p_glGetBooleani_v)( GLenum target, GLuint index, GLboolean *data );
        void       (WINE_GLAPI *p_glGetBufferParameteri64v)( GLenum target, GLenum pname, GLint64 *params );
        void       (WINE_GLAPI *p_glGetBufferParameteriv)( GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetBufferParameterivARB)( GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetBufferParameterui64vNV)( GLenum target, GLenum pname, GLuint64EXT *params );
        void       (WINE_GLAPI *p_glGetBufferPointerv)( GLenum target, GLenum pname, void **params );
        void       (WINE_GLAPI *p_glGetBufferPointervARB)( GLenum target, GLenum pname, void **params );
        void       (WINE_GLAPI *p_glGetBufferSubData)( GLenum target, GLintptr offset, GLsizeiptr size, void *data );
        void       (WINE_GLAPI *p_glGetBufferSubDataARB)( GLenum target, GLintptrARB offset, GLsizeiptrARB size, void *data );
        void       (WINE_GLAPI *p_glGetClipPlanefOES)( GLenum plane, GLfloat *equation );
        void       (WINE_GLAPI *p_glGetClipPlanexOES)( GLenum plane, GLfixed *equation );
        void       (WINE_GLAPI *p_glGetColorTable)( GLenum target, GLenum format, GLenum type, void *table );
        void       (WINE_GLAPI *p_glGetColorTableEXT)( GLenum target, GLenum format, GLenum type, void *data );
        void       (WINE_GLAPI *p_glGetColorTableParameterfv)( GLenum target, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetColorTableParameterfvEXT)( GLenum target, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetColorTableParameterfvSGI)( GLenum target, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetColorTableParameteriv)( GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetColorTableParameterivEXT)( GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetColorTableParameterivSGI)( GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetColorTableSGI)( GLenum target, GLenum format, GLenum type, void *table );
        void       (WINE_GLAPI *p_glGetCombinerInputParameterfvNV)( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetCombinerInputParameterivNV)( GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetCombinerOutputParameterfvNV)( GLenum stage, GLenum portion, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetCombinerOutputParameterivNV)( GLenum stage, GLenum portion, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetCombinerStageParameterfvNV)( GLenum stage, GLenum pname, GLfloat *params );
        GLuint     (WINE_GLAPI *p_glGetCommandHeaderNV)( GLenum tokenID, GLuint size );
        void       (WINE_GLAPI *p_glGetCompressedMultiTexImageEXT)( GLenum texunit, GLenum target, GLint lod, void *img );
        void       (WINE_GLAPI *p_glGetCompressedTexImage)( GLenum target, GLint level, void *img );
        void       (WINE_GLAPI *p_glGetCompressedTexImageARB)( GLenum target, GLint level, void *img );
        void       (WINE_GLAPI *p_glGetCompressedTextureImage)( GLuint texture, GLint level, GLsizei bufSize, void *pixels );
        void       (WINE_GLAPI *p_glGetCompressedTextureImageEXT)( GLuint texture, GLenum target, GLint lod, void *img );
        void       (WINE_GLAPI *p_glGetCompressedTextureSubImage)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei bufSize, void *pixels );
        void       (WINE_GLAPI *p_glGetConvolutionFilter)( GLenum target, GLenum format, GLenum type, void *image );
        void       (WINE_GLAPI *p_glGetConvolutionFilterEXT)( GLenum target, GLenum format, GLenum type, void *image );
        void       (WINE_GLAPI *p_glGetConvolutionParameterfv)( GLenum target, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetConvolutionParameterfvEXT)( GLenum target, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetConvolutionParameteriv)( GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetConvolutionParameterivEXT)( GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetConvolutionParameterxvOES)( GLenum target, GLenum pname, GLfixed *params );
        void       (WINE_GLAPI *p_glGetCoverageModulationTableNV)( GLsizei bufSize, GLfloat *v );
        GLuint     (WINE_GLAPI *p_glGetDebugMessageLog)( GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog );
        GLuint     (WINE_GLAPI *p_glGetDebugMessageLogAMD)( GLuint count, GLsizei bufSize, GLenum *categories, GLuint *severities, GLuint *ids, GLsizei *lengths, GLchar *message );
        GLuint     (WINE_GLAPI *p_glGetDebugMessageLogARB)( GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog );
        void       (WINE_GLAPI *p_glGetDetailTexFuncSGIS)( GLenum target, GLfloat *points );
        void       (WINE_GLAPI *p_glGetDoubleIndexedvEXT)( GLenum target, GLuint index, GLdouble *data );
        void       (WINE_GLAPI *p_glGetDoublei_v)( GLenum target, GLuint index, GLdouble *data );
        void       (WINE_GLAPI *p_glGetDoublei_vEXT)( GLenum pname, GLuint index, GLdouble *params );
        void       (WINE_GLAPI *p_glGetFenceivNV)( GLuint fence, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetFinalCombinerInputParameterfvNV)( GLenum variable, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetFinalCombinerInputParameterivNV)( GLenum variable, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetFirstPerfQueryIdINTEL)( GLuint *queryId );
        void       (WINE_GLAPI *p_glGetFixedvOES)( GLenum pname, GLfixed *params );
        void       (WINE_GLAPI *p_glGetFloatIndexedvEXT)( GLenum target, GLuint index, GLfloat *data );
        void       (WINE_GLAPI *p_glGetFloati_v)( GLenum target, GLuint index, GLfloat *data );
        void       (WINE_GLAPI *p_glGetFloati_vEXT)( GLenum pname, GLuint index, GLfloat *params );
        void       (WINE_GLAPI *p_glGetFogFuncSGIS)( GLfloat *points );
        GLint      (WINE_GLAPI *p_glGetFragDataIndex)( GLuint program, const GLchar *name );
        GLint      (WINE_GLAPI *p_glGetFragDataLocation)( GLuint program, const GLchar *name );
        GLint      (WINE_GLAPI *p_glGetFragDataLocationEXT)( GLuint program, const GLchar *name );
        void       (WINE_GLAPI *p_glGetFragmentLightfvSGIX)( GLenum light, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetFragmentLightivSGIX)( GLenum light, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetFragmentMaterialfvSGIX)( GLenum face, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetFragmentMaterialivSGIX)( GLenum face, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetFramebufferAttachmentParameteriv)( GLenum target, GLenum attachment, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetFramebufferAttachmentParameterivEXT)( GLenum target, GLenum attachment, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetFramebufferParameterfvAMD)( GLenum target, GLenum pname, GLuint numsamples, GLuint pixelindex, GLsizei size, GLfloat *values );
        void       (WINE_GLAPI *p_glGetFramebufferParameteriv)( GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetFramebufferParameterivEXT)( GLuint framebuffer, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetFramebufferParameterivMESA)( GLenum target, GLenum pname, GLint *params );
        GLenum     (WINE_GLAPI *p_glGetGraphicsResetStatus)(void);
        GLenum     (WINE_GLAPI *p_glGetGraphicsResetStatusARB)(void);
        GLhandleARB (WINE_GLAPI *p_glGetHandleARB)( GLenum pname );
        void       (WINE_GLAPI *p_glGetHistogram)( GLenum target, GLboolean reset, GLenum format, GLenum type, void *values );
        void       (WINE_GLAPI *p_glGetHistogramEXT)( GLenum target, GLboolean reset, GLenum format, GLenum type, void *values );
        void       (WINE_GLAPI *p_glGetHistogramParameterfv)( GLenum target, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetHistogramParameterfvEXT)( GLenum target, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetHistogramParameteriv)( GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetHistogramParameterivEXT)( GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetHistogramParameterxvOES)( GLenum target, GLenum pname, GLfixed *params );
        GLuint64   (WINE_GLAPI *p_glGetImageHandleARB)( GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format );
        GLuint64   (WINE_GLAPI *p_glGetImageHandleNV)( GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format );
        void       (WINE_GLAPI *p_glGetImageTransformParameterfvHP)( GLenum target, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetImageTransformParameterivHP)( GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetInfoLogARB)( GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog );
        GLint      (WINE_GLAPI *p_glGetInstrumentsSGIX)(void);
        void       (WINE_GLAPI *p_glGetInteger64i_v)( GLenum target, GLuint index, GLint64 *data );
        void       (WINE_GLAPI *p_glGetInteger64v)( GLenum pname, GLint64 *data );
        void       (WINE_GLAPI *p_glGetIntegerIndexedvEXT)( GLenum target, GLuint index, GLint *data );
        void       (WINE_GLAPI *p_glGetIntegeri_v)( GLenum target, GLuint index, GLint *data );
        void       (WINE_GLAPI *p_glGetIntegerui64i_vNV)( GLenum value, GLuint index, GLuint64EXT *result );
        void       (WINE_GLAPI *p_glGetIntegerui64vNV)( GLenum value, GLuint64EXT *result );
        void       (WINE_GLAPI *p_glGetInternalformatSampleivNV)( GLenum target, GLenum internalformat, GLsizei samples, GLenum pname, GLsizei count, GLint *params );
        void       (WINE_GLAPI *p_glGetInternalformati64v)( GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint64 *params );
        void       (WINE_GLAPI *p_glGetInternalformativ)( GLenum target, GLenum internalformat, GLenum pname, GLsizei count, GLint *params );
        void       (WINE_GLAPI *p_glGetInvariantBooleanvEXT)( GLuint id, GLenum value, GLboolean *data );
        void       (WINE_GLAPI *p_glGetInvariantFloatvEXT)( GLuint id, GLenum value, GLfloat *data );
        void       (WINE_GLAPI *p_glGetInvariantIntegervEXT)( GLuint id, GLenum value, GLint *data );
        void       (WINE_GLAPI *p_glGetLightxOES)( GLenum light, GLenum pname, GLfixed *params );
        void       (WINE_GLAPI *p_glGetListParameterfvSGIX)( GLuint list, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetListParameterivSGIX)( GLuint list, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetLocalConstantBooleanvEXT)( GLuint id, GLenum value, GLboolean *data );
        void       (WINE_GLAPI *p_glGetLocalConstantFloatvEXT)( GLuint id, GLenum value, GLfloat *data );
        void       (WINE_GLAPI *p_glGetLocalConstantIntegervEXT)( GLuint id, GLenum value, GLint *data );
        void       (WINE_GLAPI *p_glGetMapAttribParameterfvNV)( GLenum target, GLuint index, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetMapAttribParameterivNV)( GLenum target, GLuint index, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetMapControlPointsNV)( GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLboolean packed, void *points );
        void       (WINE_GLAPI *p_glGetMapParameterfvNV)( GLenum target, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetMapParameterivNV)( GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetMapxvOES)( GLenum target, GLenum query, GLfixed *v );
        void       (WINE_GLAPI *p_glGetMaterialxOES)( GLenum face, GLenum pname, GLfixed param );
        void       (WINE_GLAPI *p_glGetMemoryObjectDetachedResourcesuivNV)( GLuint memory, GLenum pname, GLint first, GLsizei count, GLuint *params );
        void       (WINE_GLAPI *p_glGetMemoryObjectParameterivEXT)( GLuint memoryObject, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetMinmax)( GLenum target, GLboolean reset, GLenum format, GLenum type, void *values );
        void       (WINE_GLAPI *p_glGetMinmaxEXT)( GLenum target, GLboolean reset, GLenum format, GLenum type, void *values );
        void       (WINE_GLAPI *p_glGetMinmaxParameterfv)( GLenum target, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetMinmaxParameterfvEXT)( GLenum target, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetMinmaxParameteriv)( GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetMinmaxParameterivEXT)( GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetMultiTexEnvfvEXT)( GLenum texunit, GLenum target, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetMultiTexEnvivEXT)( GLenum texunit, GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetMultiTexGendvEXT)( GLenum texunit, GLenum coord, GLenum pname, GLdouble *params );
        void       (WINE_GLAPI *p_glGetMultiTexGenfvEXT)( GLenum texunit, GLenum coord, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetMultiTexGenivEXT)( GLenum texunit, GLenum coord, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetMultiTexImageEXT)( GLenum texunit, GLenum target, GLint level, GLenum format, GLenum type, void *pixels );
        void       (WINE_GLAPI *p_glGetMultiTexLevelParameterfvEXT)( GLenum texunit, GLenum target, GLint level, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetMultiTexLevelParameterivEXT)( GLenum texunit, GLenum target, GLint level, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetMultiTexParameterIivEXT)( GLenum texunit, GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetMultiTexParameterIuivEXT)( GLenum texunit, GLenum target, GLenum pname, GLuint *params );
        void       (WINE_GLAPI *p_glGetMultiTexParameterfvEXT)( GLenum texunit, GLenum target, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetMultiTexParameterivEXT)( GLenum texunit, GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetMultisamplefv)( GLenum pname, GLuint index, GLfloat *val );
        void       (WINE_GLAPI *p_glGetMultisamplefvNV)( GLenum pname, GLuint index, GLfloat *val );
        void       (WINE_GLAPI *p_glGetNamedBufferParameteri64v)( GLuint buffer, GLenum pname, GLint64 *params );
        void       (WINE_GLAPI *p_glGetNamedBufferParameteriv)( GLuint buffer, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetNamedBufferParameterivEXT)( GLuint buffer, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetNamedBufferParameterui64vNV)( GLuint buffer, GLenum pname, GLuint64EXT *params );
        void       (WINE_GLAPI *p_glGetNamedBufferPointerv)( GLuint buffer, GLenum pname, void **params );
        void       (WINE_GLAPI *p_glGetNamedBufferPointervEXT)( GLuint buffer, GLenum pname, void **params );
        void       (WINE_GLAPI *p_glGetNamedBufferSubData)( GLuint buffer, GLintptr offset, GLsizeiptr size, void *data );
        void       (WINE_GLAPI *p_glGetNamedBufferSubDataEXT)( GLuint buffer, GLintptr offset, GLsizeiptr size, void *data );
        void       (WINE_GLAPI *p_glGetNamedFramebufferAttachmentParameteriv)( GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetNamedFramebufferAttachmentParameterivEXT)( GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetNamedFramebufferParameterfvAMD)( GLuint framebuffer, GLenum pname, GLuint numsamples, GLuint pixelindex, GLsizei size, GLfloat *values );
        void       (WINE_GLAPI *p_glGetNamedFramebufferParameteriv)( GLuint framebuffer, GLenum pname, GLint *param );
        void       (WINE_GLAPI *p_glGetNamedFramebufferParameterivEXT)( GLuint framebuffer, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetNamedProgramLocalParameterIivEXT)( GLuint program, GLenum target, GLuint index, GLint *params );
        void       (WINE_GLAPI *p_glGetNamedProgramLocalParameterIuivEXT)( GLuint program, GLenum target, GLuint index, GLuint *params );
        void       (WINE_GLAPI *p_glGetNamedProgramLocalParameterdvEXT)( GLuint program, GLenum target, GLuint index, GLdouble *params );
        void       (WINE_GLAPI *p_glGetNamedProgramLocalParameterfvEXT)( GLuint program, GLenum target, GLuint index, GLfloat *params );
        void       (WINE_GLAPI *p_glGetNamedProgramStringEXT)( GLuint program, GLenum target, GLenum pname, void *string );
        void       (WINE_GLAPI *p_glGetNamedProgramivEXT)( GLuint program, GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetNamedRenderbufferParameteriv)( GLuint renderbuffer, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetNamedRenderbufferParameterivEXT)( GLuint renderbuffer, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetNamedStringARB)( GLint namelen, const GLchar *name, GLsizei bufSize, GLint *stringlen, GLchar *string );
        void       (WINE_GLAPI *p_glGetNamedStringivARB)( GLint namelen, const GLchar *name, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetNextPerfQueryIdINTEL)( GLuint queryId, GLuint *nextQueryId );
        void       (WINE_GLAPI *p_glGetObjectBufferfvATI)( GLuint buffer, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetObjectBufferivATI)( GLuint buffer, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetObjectLabel)( GLenum identifier, GLuint name, GLsizei bufSize, GLsizei *length, GLchar *label );
        void       (WINE_GLAPI *p_glGetObjectLabelEXT)( GLenum type, GLuint object, GLsizei bufSize, GLsizei *length, GLchar *label );
        void       (WINE_GLAPI *p_glGetObjectParameterfvARB)( GLhandleARB obj, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetObjectParameterivAPPLE)( GLenum objectType, GLuint name, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetObjectParameterivARB)( GLhandleARB obj, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetObjectPtrLabel)( const void *ptr, GLsizei bufSize, GLsizei *length, GLchar *label );
        void       (WINE_GLAPI *p_glGetOcclusionQueryivNV)( GLuint id, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetOcclusionQueryuivNV)( GLuint id, GLenum pname, GLuint *params );
        void       (WINE_GLAPI *p_glGetPathColorGenfvNV)( GLenum color, GLenum pname, GLfloat *value );
        void       (WINE_GLAPI *p_glGetPathColorGenivNV)( GLenum color, GLenum pname, GLint *value );
        void       (WINE_GLAPI *p_glGetPathCommandsNV)( GLuint path, GLubyte *commands );
        void       (WINE_GLAPI *p_glGetPathCoordsNV)( GLuint path, GLfloat *coords );
        void       (WINE_GLAPI *p_glGetPathDashArrayNV)( GLuint path, GLfloat *dashArray );
        GLfloat    (WINE_GLAPI *p_glGetPathLengthNV)( GLuint path, GLsizei startSegment, GLsizei numSegments );
        void       (WINE_GLAPI *p_glGetPathMetricRangeNV)( GLbitfield metricQueryMask, GLuint firstPathName, GLsizei numPaths, GLsizei stride, GLfloat *metrics );
        void       (WINE_GLAPI *p_glGetPathMetricsNV)( GLbitfield metricQueryMask, GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLsizei stride, GLfloat *metrics );
        void       (WINE_GLAPI *p_glGetPathParameterfvNV)( GLuint path, GLenum pname, GLfloat *value );
        void       (WINE_GLAPI *p_glGetPathParameterivNV)( GLuint path, GLenum pname, GLint *value );
        void       (WINE_GLAPI *p_glGetPathSpacingNV)( GLenum pathListMode, GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLfloat advanceScale, GLfloat kerningScale, GLenum transformType, GLfloat *returnedSpacing );
        void       (WINE_GLAPI *p_glGetPathTexGenfvNV)( GLenum texCoordSet, GLenum pname, GLfloat *value );
        void       (WINE_GLAPI *p_glGetPathTexGenivNV)( GLenum texCoordSet, GLenum pname, GLint *value );
        void       (WINE_GLAPI *p_glGetPerfCounterInfoINTEL)( GLuint queryId, GLuint counterId, GLuint counterNameLength, GLchar *counterName, GLuint counterDescLength, GLchar *counterDesc, GLuint *counterOffset, GLuint *counterDataSize, GLuint *counterTypeEnum, GLuint *counterDataTypeEnum, GLuint64 *rawCounterMaxValue );
        void       (WINE_GLAPI *p_glGetPerfMonitorCounterDataAMD)( GLuint monitor, GLenum pname, GLsizei dataSize, GLuint *data, GLint *bytesWritten );
        void       (WINE_GLAPI *p_glGetPerfMonitorCounterInfoAMD)( GLuint group, GLuint counter, GLenum pname, void *data );
        void       (WINE_GLAPI *p_glGetPerfMonitorCounterStringAMD)( GLuint group, GLuint counter, GLsizei bufSize, GLsizei *length, GLchar *counterString );
        void       (WINE_GLAPI *p_glGetPerfMonitorCountersAMD)( GLuint group, GLint *numCounters, GLint *maxActiveCounters, GLsizei counterSize, GLuint *counters );
        void       (WINE_GLAPI *p_glGetPerfMonitorGroupStringAMD)( GLuint group, GLsizei bufSize, GLsizei *length, GLchar *groupString );
        void       (WINE_GLAPI *p_glGetPerfMonitorGroupsAMD)( GLint *numGroups, GLsizei groupsSize, GLuint *groups );
        void       (WINE_GLAPI *p_glGetPerfQueryDataINTEL)( GLuint queryHandle, GLuint flags, GLsizei dataSize, void *data, GLuint *bytesWritten );
        void       (WINE_GLAPI *p_glGetPerfQueryIdByNameINTEL)( GLchar *queryName, GLuint *queryId );
        void       (WINE_GLAPI *p_glGetPerfQueryInfoINTEL)( GLuint queryId, GLuint queryNameLength, GLchar *queryName, GLuint *dataSize, GLuint *noCounters, GLuint *noInstances, GLuint *capsMask );
        void       (WINE_GLAPI *p_glGetPixelMapxv)( GLenum map, GLint size, GLfixed *values );
        void       (WINE_GLAPI *p_glGetPixelTexGenParameterfvSGIS)( GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetPixelTexGenParameterivSGIS)( GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetPixelTransformParameterfvEXT)( GLenum target, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetPixelTransformParameterivEXT)( GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetPointerIndexedvEXT)( GLenum target, GLuint index, void **data );
        void       (WINE_GLAPI *p_glGetPointeri_vEXT)( GLenum pname, GLuint index, void **params );
        void       (WINE_GLAPI *p_glGetPointervEXT)( GLenum pname, void **params );
        void       (WINE_GLAPI *p_glGetProgramBinary)( GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary );
        void       (WINE_GLAPI *p_glGetProgramEnvParameterIivNV)( GLenum target, GLuint index, GLint *params );
        void       (WINE_GLAPI *p_glGetProgramEnvParameterIuivNV)( GLenum target, GLuint index, GLuint *params );
        void       (WINE_GLAPI *p_glGetProgramEnvParameterdvARB)( GLenum target, GLuint index, GLdouble *params );
        void       (WINE_GLAPI *p_glGetProgramEnvParameterfvARB)( GLenum target, GLuint index, GLfloat *params );
        void       (WINE_GLAPI *p_glGetProgramInfoLog)( GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog );
        void       (WINE_GLAPI *p_glGetProgramInterfaceiv)( GLuint program, GLenum programInterface, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetProgramLocalParameterIivNV)( GLenum target, GLuint index, GLint *params );
        void       (WINE_GLAPI *p_glGetProgramLocalParameterIuivNV)( GLenum target, GLuint index, GLuint *params );
        void       (WINE_GLAPI *p_glGetProgramLocalParameterdvARB)( GLenum target, GLuint index, GLdouble *params );
        void       (WINE_GLAPI *p_glGetProgramLocalParameterfvARB)( GLenum target, GLuint index, GLfloat *params );
        void       (WINE_GLAPI *p_glGetProgramNamedParameterdvNV)( GLuint id, GLsizei len, const GLubyte *name, GLdouble *params );
        void       (WINE_GLAPI *p_glGetProgramNamedParameterfvNV)( GLuint id, GLsizei len, const GLubyte *name, GLfloat *params );
        void       (WINE_GLAPI *p_glGetProgramParameterdvNV)( GLenum target, GLuint index, GLenum pname, GLdouble *params );
        void       (WINE_GLAPI *p_glGetProgramParameterfvNV)( GLenum target, GLuint index, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetProgramPipelineInfoLog)( GLuint pipeline, GLsizei bufSize, GLsizei *length, GLchar *infoLog );
        void       (WINE_GLAPI *p_glGetProgramPipelineiv)( GLuint pipeline, GLenum pname, GLint *params );
        GLuint     (WINE_GLAPI *p_glGetProgramResourceIndex)( GLuint program, GLenum programInterface, const GLchar *name );
        GLint      (WINE_GLAPI *p_glGetProgramResourceLocation)( GLuint program, GLenum programInterface, const GLchar *name );
        GLint      (WINE_GLAPI *p_glGetProgramResourceLocationIndex)( GLuint program, GLenum programInterface, const GLchar *name );
        void       (WINE_GLAPI *p_glGetProgramResourceName)( GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name );
        void       (WINE_GLAPI *p_glGetProgramResourcefvNV)( GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei count, GLsizei *length, GLfloat *params );
        void       (WINE_GLAPI *p_glGetProgramResourceiv)( GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei count, GLsizei *length, GLint *params );
        void       (WINE_GLAPI *p_glGetProgramStageiv)( GLuint program, GLenum shadertype, GLenum pname, GLint *values );
        void       (WINE_GLAPI *p_glGetProgramStringARB)( GLenum target, GLenum pname, void *string );
        void       (WINE_GLAPI *p_glGetProgramStringNV)( GLuint id, GLenum pname, GLubyte *program );
        void       (WINE_GLAPI *p_glGetProgramSubroutineParameteruivNV)( GLenum target, GLuint index, GLuint *param );
        void       (WINE_GLAPI *p_glGetProgramiv)( GLuint program, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetProgramivARB)( GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetProgramivNV)( GLuint id, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetQueryBufferObjecti64v)( GLuint id, GLuint buffer, GLenum pname, GLintptr offset );
        void       (WINE_GLAPI *p_glGetQueryBufferObjectiv)( GLuint id, GLuint buffer, GLenum pname, GLintptr offset );
        void       (WINE_GLAPI *p_glGetQueryBufferObjectui64v)( GLuint id, GLuint buffer, GLenum pname, GLintptr offset );
        void       (WINE_GLAPI *p_glGetQueryBufferObjectuiv)( GLuint id, GLuint buffer, GLenum pname, GLintptr offset );
        void       (WINE_GLAPI *p_glGetQueryIndexediv)( GLenum target, GLuint index, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetQueryObjecti64v)( GLuint id, GLenum pname, GLint64 *params );
        void       (WINE_GLAPI *p_glGetQueryObjecti64vEXT)( GLuint id, GLenum pname, GLint64 *params );
        void       (WINE_GLAPI *p_glGetQueryObjectiv)( GLuint id, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetQueryObjectivARB)( GLuint id, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetQueryObjectui64v)( GLuint id, GLenum pname, GLuint64 *params );
        void       (WINE_GLAPI *p_glGetQueryObjectui64vEXT)( GLuint id, GLenum pname, GLuint64 *params );
        void       (WINE_GLAPI *p_glGetQueryObjectuiv)( GLuint id, GLenum pname, GLuint *params );
        void       (WINE_GLAPI *p_glGetQueryObjectuivARB)( GLuint id, GLenum pname, GLuint *params );
        void       (WINE_GLAPI *p_glGetQueryiv)( GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetQueryivARB)( GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetRenderbufferParameteriv)( GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetRenderbufferParameterivEXT)( GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetSamplerParameterIiv)( GLuint sampler, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetSamplerParameterIuiv)( GLuint sampler, GLenum pname, GLuint *params );
        void       (WINE_GLAPI *p_glGetSamplerParameterfv)( GLuint sampler, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetSamplerParameteriv)( GLuint sampler, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetSemaphoreParameterui64vEXT)( GLuint semaphore, GLenum pname, GLuint64 *params );
        void       (WINE_GLAPI *p_glGetSeparableFilter)( GLenum target, GLenum format, GLenum type, void *row, void *column, void *span );
        void       (WINE_GLAPI *p_glGetSeparableFilterEXT)( GLenum target, GLenum format, GLenum type, void *row, void *column, void *span );
        void       (WINE_GLAPI *p_glGetShaderInfoLog)( GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog );
        void       (WINE_GLAPI *p_glGetShaderPrecisionFormat)( GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision );
        void       (WINE_GLAPI *p_glGetShaderSource)( GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source );
        void       (WINE_GLAPI *p_glGetShaderSourceARB)( GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source );
        void       (WINE_GLAPI *p_glGetShaderiv)( GLuint shader, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetShadingRateImagePaletteNV)( GLuint viewport, GLuint entry, GLenum *rate );
        void       (WINE_GLAPI *p_glGetShadingRateSampleLocationivNV)( GLenum rate, GLuint samples, GLuint index, GLint *location );
        void       (WINE_GLAPI *p_glGetSharpenTexFuncSGIS)( GLenum target, GLfloat *points );
        GLushort   (WINE_GLAPI *p_glGetStageIndexNV)( GLenum shadertype );
        const GLubyte * (WINE_GLAPI *p_glGetStringi)( GLenum name, GLuint index );
        GLuint     (WINE_GLAPI *p_glGetSubroutineIndex)( GLuint program, GLenum shadertype, const GLchar *name );
        GLint      (WINE_GLAPI *p_glGetSubroutineUniformLocation)( GLuint program, GLenum shadertype, const GLchar *name );
        void       (WINE_GLAPI *p_glGetSynciv)( GLsync sync, GLenum pname, GLsizei count, GLsizei *length, GLint *values );
        void       (WINE_GLAPI *p_glGetTexBumpParameterfvATI)( GLenum pname, GLfloat *param );
        void       (WINE_GLAPI *p_glGetTexBumpParameterivATI)( GLenum pname, GLint *param );
        void       (WINE_GLAPI *p_glGetTexEnvxvOES)( GLenum target, GLenum pname, GLfixed *params );
        void       (WINE_GLAPI *p_glGetTexFilterFuncSGIS)( GLenum target, GLenum filter, GLfloat *weights );
        void       (WINE_GLAPI *p_glGetTexGenxvOES)( GLenum coord, GLenum pname, GLfixed *params );
        void       (WINE_GLAPI *p_glGetTexLevelParameterxvOES)( GLenum target, GLint level, GLenum pname, GLfixed *params );
        void       (WINE_GLAPI *p_glGetTexParameterIiv)( GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetTexParameterIivEXT)( GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetTexParameterIuiv)( GLenum target, GLenum pname, GLuint *params );
        void       (WINE_GLAPI *p_glGetTexParameterIuivEXT)( GLenum target, GLenum pname, GLuint *params );
        void       (WINE_GLAPI *p_glGetTexParameterPointervAPPLE)( GLenum target, GLenum pname, void **params );
        void       (WINE_GLAPI *p_glGetTexParameterxvOES)( GLenum target, GLenum pname, GLfixed *params );
        GLuint64   (WINE_GLAPI *p_glGetTextureHandleARB)( GLuint texture );
        GLuint64   (WINE_GLAPI *p_glGetTextureHandleNV)( GLuint texture );
        void       (WINE_GLAPI *p_glGetTextureImage)( GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels );
        void       (WINE_GLAPI *p_glGetTextureImageEXT)( GLuint texture, GLenum target, GLint level, GLenum format, GLenum type, void *pixels );
        void       (WINE_GLAPI *p_glGetTextureLevelParameterfv)( GLuint texture, GLint level, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetTextureLevelParameterfvEXT)( GLuint texture, GLenum target, GLint level, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetTextureLevelParameteriv)( GLuint texture, GLint level, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetTextureLevelParameterivEXT)( GLuint texture, GLenum target, GLint level, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetTextureParameterIiv)( GLuint texture, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetTextureParameterIivEXT)( GLuint texture, GLenum target, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetTextureParameterIuiv)( GLuint texture, GLenum pname, GLuint *params );
        void       (WINE_GLAPI *p_glGetTextureParameterIuivEXT)( GLuint texture, GLenum target, GLenum pname, GLuint *params );
        void       (WINE_GLAPI *p_glGetTextureParameterfv)( GLuint texture, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetTextureParameterfvEXT)( GLuint texture, GLenum target, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetTextureParameteriv)( GLuint texture, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetTextureParameterivEXT)( GLuint texture, GLenum target, GLenum pname, GLint *params );
        GLuint64   (WINE_GLAPI *p_glGetTextureSamplerHandleARB)( GLuint texture, GLuint sampler );
        GLuint64   (WINE_GLAPI *p_glGetTextureSamplerHandleNV)( GLuint texture, GLuint sampler );
        void       (WINE_GLAPI *p_glGetTextureSubImage)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLsizei bufSize, void *pixels );
        void       (WINE_GLAPI *p_glGetTrackMatrixivNV)( GLenum target, GLuint address, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetTransformFeedbackVarying)( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name );
        void       (WINE_GLAPI *p_glGetTransformFeedbackVaryingEXT)( GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name );
        void       (WINE_GLAPI *p_glGetTransformFeedbackVaryingNV)( GLuint program, GLuint index, GLint *location );
        void       (WINE_GLAPI *p_glGetTransformFeedbacki64_v)( GLuint xfb, GLenum pname, GLuint index, GLint64 *param );
        void       (WINE_GLAPI *p_glGetTransformFeedbacki_v)( GLuint xfb, GLenum pname, GLuint index, GLint *param );
        void       (WINE_GLAPI *p_glGetTransformFeedbackiv)( GLuint xfb, GLenum pname, GLint *param );
        GLuint     (WINE_GLAPI *p_glGetUniformBlockIndex)( GLuint program, const GLchar *uniformBlockName );
        GLint      (WINE_GLAPI *p_glGetUniformBufferSizeEXT)( GLuint program, GLint location );
        void       (WINE_GLAPI *p_glGetUniformIndices)( GLuint program, GLsizei uniformCount, const GLchar *const*uniformNames, GLuint *uniformIndices );
        GLint      (WINE_GLAPI *p_glGetUniformLocation)( GLuint program, const GLchar *name );
        GLint      (WINE_GLAPI *p_glGetUniformLocationARB)( GLhandleARB programObj, const GLcharARB *name );
        GLintptr   (WINE_GLAPI *p_glGetUniformOffsetEXT)( GLuint program, GLint location );
        void       (WINE_GLAPI *p_glGetUniformSubroutineuiv)( GLenum shadertype, GLint location, GLuint *params );
        void       (WINE_GLAPI *p_glGetUniformdv)( GLuint program, GLint location, GLdouble *params );
        void       (WINE_GLAPI *p_glGetUniformfv)( GLuint program, GLint location, GLfloat *params );
        void       (WINE_GLAPI *p_glGetUniformfvARB)( GLhandleARB programObj, GLint location, GLfloat *params );
        void       (WINE_GLAPI *p_glGetUniformi64vARB)( GLuint program, GLint location, GLint64 *params );
        void       (WINE_GLAPI *p_glGetUniformi64vNV)( GLuint program, GLint location, GLint64EXT *params );
        void       (WINE_GLAPI *p_glGetUniformiv)( GLuint program, GLint location, GLint *params );
        void       (WINE_GLAPI *p_glGetUniformivARB)( GLhandleARB programObj, GLint location, GLint *params );
        void       (WINE_GLAPI *p_glGetUniformui64vARB)( GLuint program, GLint location, GLuint64 *params );
        void       (WINE_GLAPI *p_glGetUniformui64vNV)( GLuint program, GLint location, GLuint64EXT *params );
        void       (WINE_GLAPI *p_glGetUniformuiv)( GLuint program, GLint location, GLuint *params );
        void       (WINE_GLAPI *p_glGetUniformuivEXT)( GLuint program, GLint location, GLuint *params );
        void       (WINE_GLAPI *p_glGetUnsignedBytei_vEXT)( GLenum target, GLuint index, GLubyte *data );
        void       (WINE_GLAPI *p_glGetUnsignedBytevEXT)( GLenum pname, GLubyte *data );
        void       (WINE_GLAPI *p_glGetVariantArrayObjectfvATI)( GLuint id, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetVariantArrayObjectivATI)( GLuint id, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetVariantBooleanvEXT)( GLuint id, GLenum value, GLboolean *data );
        void       (WINE_GLAPI *p_glGetVariantFloatvEXT)( GLuint id, GLenum value, GLfloat *data );
        void       (WINE_GLAPI *p_glGetVariantIntegervEXT)( GLuint id, GLenum value, GLint *data );
        void       (WINE_GLAPI *p_glGetVariantPointervEXT)( GLuint id, GLenum value, void **data );
        GLint      (WINE_GLAPI *p_glGetVaryingLocationNV)( GLuint program, const GLchar *name );
        void       (WINE_GLAPI *p_glGetVertexArrayIndexed64iv)( GLuint vaobj, GLuint index, GLenum pname, GLint64 *param );
        void       (WINE_GLAPI *p_glGetVertexArrayIndexediv)( GLuint vaobj, GLuint index, GLenum pname, GLint *param );
        void       (WINE_GLAPI *p_glGetVertexArrayIntegeri_vEXT)( GLuint vaobj, GLuint index, GLenum pname, GLint *param );
        void       (WINE_GLAPI *p_glGetVertexArrayIntegervEXT)( GLuint vaobj, GLenum pname, GLint *param );
        void       (WINE_GLAPI *p_glGetVertexArrayPointeri_vEXT)( GLuint vaobj, GLuint index, GLenum pname, void **param );
        void       (WINE_GLAPI *p_glGetVertexArrayPointervEXT)( GLuint vaobj, GLenum pname, void **param );
        void       (WINE_GLAPI *p_glGetVertexArrayiv)( GLuint vaobj, GLenum pname, GLint *param );
        void       (WINE_GLAPI *p_glGetVertexAttribArrayObjectfvATI)( GLuint index, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetVertexAttribArrayObjectivATI)( GLuint index, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetVertexAttribIiv)( GLuint index, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetVertexAttribIivEXT)( GLuint index, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetVertexAttribIuiv)( GLuint index, GLenum pname, GLuint *params );
        void       (WINE_GLAPI *p_glGetVertexAttribIuivEXT)( GLuint index, GLenum pname, GLuint *params );
        void       (WINE_GLAPI *p_glGetVertexAttribLdv)( GLuint index, GLenum pname, GLdouble *params );
        void       (WINE_GLAPI *p_glGetVertexAttribLdvEXT)( GLuint index, GLenum pname, GLdouble *params );
        void       (WINE_GLAPI *p_glGetVertexAttribLi64vNV)( GLuint index, GLenum pname, GLint64EXT *params );
        void       (WINE_GLAPI *p_glGetVertexAttribLui64vARB)( GLuint index, GLenum pname, GLuint64EXT *params );
        void       (WINE_GLAPI *p_glGetVertexAttribLui64vNV)( GLuint index, GLenum pname, GLuint64EXT *params );
        void       (WINE_GLAPI *p_glGetVertexAttribPointerv)( GLuint index, GLenum pname, void **pointer );
        void       (WINE_GLAPI *p_glGetVertexAttribPointervARB)( GLuint index, GLenum pname, void **pointer );
        void       (WINE_GLAPI *p_glGetVertexAttribPointervNV)( GLuint index, GLenum pname, void **pointer );
        void       (WINE_GLAPI *p_glGetVertexAttribdv)( GLuint index, GLenum pname, GLdouble *params );
        void       (WINE_GLAPI *p_glGetVertexAttribdvARB)( GLuint index, GLenum pname, GLdouble *params );
        void       (WINE_GLAPI *p_glGetVertexAttribdvNV)( GLuint index, GLenum pname, GLdouble *params );
        void       (WINE_GLAPI *p_glGetVertexAttribfv)( GLuint index, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetVertexAttribfvARB)( GLuint index, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetVertexAttribfvNV)( GLuint index, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetVertexAttribiv)( GLuint index, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetVertexAttribivARB)( GLuint index, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetVertexAttribivNV)( GLuint index, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetVideoCaptureStreamdvNV)( GLuint video_capture_slot, GLuint stream, GLenum pname, GLdouble *params );
        void       (WINE_GLAPI *p_glGetVideoCaptureStreamfvNV)( GLuint video_capture_slot, GLuint stream, GLenum pname, GLfloat *params );
        void       (WINE_GLAPI *p_glGetVideoCaptureStreamivNV)( GLuint video_capture_slot, GLuint stream, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetVideoCaptureivNV)( GLuint video_capture_slot, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetVideoi64vNV)( GLuint video_slot, GLenum pname, GLint64EXT *params );
        void       (WINE_GLAPI *p_glGetVideoivNV)( GLuint video_slot, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glGetVideoui64vNV)( GLuint video_slot, GLenum pname, GLuint64EXT *params );
        void       (WINE_GLAPI *p_glGetVideouivNV)( GLuint video_slot, GLenum pname, GLuint *params );
        GLVULKANPROCNV (WINE_GLAPI *p_glGetVkProcAddrNV)( const GLchar *name );
        void       (WINE_GLAPI *p_glGetnColorTable)( GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *table );
        void       (WINE_GLAPI *p_glGetnColorTableARB)( GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *table );
        void       (WINE_GLAPI *p_glGetnCompressedTexImage)( GLenum target, GLint lod, GLsizei bufSize, void *pixels );
        void       (WINE_GLAPI *p_glGetnCompressedTexImageARB)( GLenum target, GLint lod, GLsizei bufSize, void *img );
        void       (WINE_GLAPI *p_glGetnConvolutionFilter)( GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *image );
        void       (WINE_GLAPI *p_glGetnConvolutionFilterARB)( GLenum target, GLenum format, GLenum type, GLsizei bufSize, void *image );
        void       (WINE_GLAPI *p_glGetnHistogram)( GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values );
        void       (WINE_GLAPI *p_glGetnHistogramARB)( GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values );
        void       (WINE_GLAPI *p_glGetnMapdv)( GLenum target, GLenum query, GLsizei bufSize, GLdouble *v );
        void       (WINE_GLAPI *p_glGetnMapdvARB)( GLenum target, GLenum query, GLsizei bufSize, GLdouble *v );
        void       (WINE_GLAPI *p_glGetnMapfv)( GLenum target, GLenum query, GLsizei bufSize, GLfloat *v );
        void       (WINE_GLAPI *p_glGetnMapfvARB)( GLenum target, GLenum query, GLsizei bufSize, GLfloat *v );
        void       (WINE_GLAPI *p_glGetnMapiv)( GLenum target, GLenum query, GLsizei bufSize, GLint *v );
        void       (WINE_GLAPI *p_glGetnMapivARB)( GLenum target, GLenum query, GLsizei bufSize, GLint *v );
        void       (WINE_GLAPI *p_glGetnMinmax)( GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values );
        void       (WINE_GLAPI *p_glGetnMinmaxARB)( GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, void *values );
        void       (WINE_GLAPI *p_glGetnPixelMapfv)( GLenum map, GLsizei bufSize, GLfloat *values );
        void       (WINE_GLAPI *p_glGetnPixelMapfvARB)( GLenum map, GLsizei bufSize, GLfloat *values );
        void       (WINE_GLAPI *p_glGetnPixelMapuiv)( GLenum map, GLsizei bufSize, GLuint *values );
        void       (WINE_GLAPI *p_glGetnPixelMapuivARB)( GLenum map, GLsizei bufSize, GLuint *values );
        void       (WINE_GLAPI *p_glGetnPixelMapusv)( GLenum map, GLsizei bufSize, GLushort *values );
        void       (WINE_GLAPI *p_glGetnPixelMapusvARB)( GLenum map, GLsizei bufSize, GLushort *values );
        void       (WINE_GLAPI *p_glGetnPolygonStipple)( GLsizei bufSize, GLubyte *pattern );
        void       (WINE_GLAPI *p_glGetnPolygonStippleARB)( GLsizei bufSize, GLubyte *pattern );
        void       (WINE_GLAPI *p_glGetnSeparableFilter)( GLenum target, GLenum format, GLenum type, GLsizei rowBufSize, void *row, GLsizei columnBufSize, void *column, void *span );
        void       (WINE_GLAPI *p_glGetnSeparableFilterARB)( GLenum target, GLenum format, GLenum type, GLsizei rowBufSize, void *row, GLsizei columnBufSize, void *column, void *span );
        void       (WINE_GLAPI *p_glGetnTexImage)( GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels );
        void       (WINE_GLAPI *p_glGetnTexImageARB)( GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *img );
        void       (WINE_GLAPI *p_glGetnUniformdv)( GLuint program, GLint location, GLsizei bufSize, GLdouble *params );
        void       (WINE_GLAPI *p_glGetnUniformdvARB)( GLuint program, GLint location, GLsizei bufSize, GLdouble *params );
        void       (WINE_GLAPI *p_glGetnUniformfv)( GLuint program, GLint location, GLsizei bufSize, GLfloat *params );
        void       (WINE_GLAPI *p_glGetnUniformfvARB)( GLuint program, GLint location, GLsizei bufSize, GLfloat *params );
        void       (WINE_GLAPI *p_glGetnUniformi64vARB)( GLuint program, GLint location, GLsizei bufSize, GLint64 *params );
        void       (WINE_GLAPI *p_glGetnUniformiv)( GLuint program, GLint location, GLsizei bufSize, GLint *params );
        void       (WINE_GLAPI *p_glGetnUniformivARB)( GLuint program, GLint location, GLsizei bufSize, GLint *params );
        void       (WINE_GLAPI *p_glGetnUniformui64vARB)( GLuint program, GLint location, GLsizei bufSize, GLuint64 *params );
        void       (WINE_GLAPI *p_glGetnUniformuiv)( GLuint program, GLint location, GLsizei bufSize, GLuint *params );
        void       (WINE_GLAPI *p_glGetnUniformuivARB)( GLuint program, GLint location, GLsizei bufSize, GLuint *params );
        void       (WINE_GLAPI *p_glGlobalAlphaFactorbSUN)( GLbyte factor );
        void       (WINE_GLAPI *p_glGlobalAlphaFactordSUN)( GLdouble factor );
        void       (WINE_GLAPI *p_glGlobalAlphaFactorfSUN)( GLfloat factor );
        void       (WINE_GLAPI *p_glGlobalAlphaFactoriSUN)( GLint factor );
        void       (WINE_GLAPI *p_glGlobalAlphaFactorsSUN)( GLshort factor );
        void       (WINE_GLAPI *p_glGlobalAlphaFactorubSUN)( GLubyte factor );
        void       (WINE_GLAPI *p_glGlobalAlphaFactoruiSUN)( GLuint factor );
        void       (WINE_GLAPI *p_glGlobalAlphaFactorusSUN)( GLushort factor );
        void       (WINE_GLAPI *p_glHintPGI)( GLenum target, GLint mode );
        void       (WINE_GLAPI *p_glHistogram)( GLenum target, GLsizei width, GLenum internalformat, GLboolean sink );
        void       (WINE_GLAPI *p_glHistogramEXT)( GLenum target, GLsizei width, GLenum internalformat, GLboolean sink );
        void       (WINE_GLAPI *p_glIglooInterfaceSGIX)( GLenum pname, const void *params );
        void       (WINE_GLAPI *p_glImageTransformParameterfHP)( GLenum target, GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glImageTransformParameterfvHP)( GLenum target, GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glImageTransformParameteriHP)( GLenum target, GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glImageTransformParameterivHP)( GLenum target, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glImportMemoryFdEXT)( GLuint memory, GLuint64 size, GLenum handleType, GLint fd );
        void       (WINE_GLAPI *p_glImportMemoryWin32HandleEXT)( GLuint memory, GLuint64 size, GLenum handleType, void *handle );
        void       (WINE_GLAPI *p_glImportMemoryWin32NameEXT)( GLuint memory, GLuint64 size, GLenum handleType, const void *name );
        void       (WINE_GLAPI *p_glImportSemaphoreFdEXT)( GLuint semaphore, GLenum handleType, GLint fd );
        void       (WINE_GLAPI *p_glImportSemaphoreWin32HandleEXT)( GLuint semaphore, GLenum handleType, void *handle );
        void       (WINE_GLAPI *p_glImportSemaphoreWin32NameEXT)( GLuint semaphore, GLenum handleType, const void *name );
        GLsync     (WINE_GLAPI *p_glImportSyncEXT)( GLenum external_sync_type, GLintptr external_sync, GLbitfield flags );
        void       (WINE_GLAPI *p_glIndexFormatNV)( GLenum type, GLsizei stride );
        void       (WINE_GLAPI *p_glIndexFuncEXT)( GLenum func, GLclampf ref );
        void       (WINE_GLAPI *p_glIndexMaterialEXT)( GLenum face, GLenum mode );
        void       (WINE_GLAPI *p_glIndexPointerEXT)( GLenum type, GLsizei stride, GLsizei count, const void *pointer );
        void       (WINE_GLAPI *p_glIndexPointerListIBM)( GLenum type, GLint stride, const void **pointer, GLint ptrstride );
        void       (WINE_GLAPI *p_glIndexxOES)( GLfixed component );
        void       (WINE_GLAPI *p_glIndexxvOES)( const GLfixed *component );
        void       (WINE_GLAPI *p_glInsertComponentEXT)( GLuint res, GLuint src, GLuint num );
        void       (WINE_GLAPI *p_glInsertEventMarkerEXT)( GLsizei length, const GLchar *marker );
        void       (WINE_GLAPI *p_glInstrumentsBufferSGIX)( GLsizei size, GLint *buffer );
        void       (WINE_GLAPI *p_glInterpolatePathsNV)( GLuint resultPath, GLuint pathA, GLuint pathB, GLfloat weight );
        void       (WINE_GLAPI *p_glInvalidateBufferData)( GLuint buffer );
        void       (WINE_GLAPI *p_glInvalidateBufferSubData)( GLuint buffer, GLintptr offset, GLsizeiptr length );
        void       (WINE_GLAPI *p_glInvalidateFramebuffer)( GLenum target, GLsizei numAttachments, const GLenum *attachments );
        void       (WINE_GLAPI *p_glInvalidateNamedFramebufferData)( GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments );
        void       (WINE_GLAPI *p_glInvalidateNamedFramebufferSubData)( GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glInvalidateSubFramebuffer)( GLenum target, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glInvalidateTexImage)( GLuint texture, GLint level );
        void       (WINE_GLAPI *p_glInvalidateTexSubImage)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth );
        GLboolean  (WINE_GLAPI *p_glIsAsyncMarkerSGIX)( GLuint marker );
        GLboolean  (WINE_GLAPI *p_glIsBuffer)( GLuint buffer );
        GLboolean  (WINE_GLAPI *p_glIsBufferARB)( GLuint buffer );
        GLboolean  (WINE_GLAPI *p_glIsBufferResidentNV)( GLenum target );
        GLboolean  (WINE_GLAPI *p_glIsCommandListNV)( GLuint list );
        GLboolean  (WINE_GLAPI *p_glIsEnabledIndexedEXT)( GLenum target, GLuint index );
        GLboolean  (WINE_GLAPI *p_glIsEnabledi)( GLenum target, GLuint index );
        GLboolean  (WINE_GLAPI *p_glIsFenceAPPLE)( GLuint fence );
        GLboolean  (WINE_GLAPI *p_glIsFenceNV)( GLuint fence );
        GLboolean  (WINE_GLAPI *p_glIsFramebuffer)( GLuint framebuffer );
        GLboolean  (WINE_GLAPI *p_glIsFramebufferEXT)( GLuint framebuffer );
        GLboolean  (WINE_GLAPI *p_glIsImageHandleResidentARB)( GLuint64 handle );
        GLboolean  (WINE_GLAPI *p_glIsImageHandleResidentNV)( GLuint64 handle );
        GLboolean  (WINE_GLAPI *p_glIsMemoryObjectEXT)( GLuint memoryObject );
        GLboolean  (WINE_GLAPI *p_glIsNameAMD)( GLenum identifier, GLuint name );
        GLboolean  (WINE_GLAPI *p_glIsNamedBufferResidentNV)( GLuint buffer );
        GLboolean  (WINE_GLAPI *p_glIsNamedStringARB)( GLint namelen, const GLchar *name );
        GLboolean  (WINE_GLAPI *p_glIsObjectBufferATI)( GLuint buffer );
        GLboolean  (WINE_GLAPI *p_glIsOcclusionQueryNV)( GLuint id );
        GLboolean  (WINE_GLAPI *p_glIsPathNV)( GLuint path );
        GLboolean  (WINE_GLAPI *p_glIsPointInFillPathNV)( GLuint path, GLuint mask, GLfloat x, GLfloat y );
        GLboolean  (WINE_GLAPI *p_glIsPointInStrokePathNV)( GLuint path, GLfloat x, GLfloat y );
        GLboolean  (WINE_GLAPI *p_glIsProgram)( GLuint program );
        GLboolean  (WINE_GLAPI *p_glIsProgramARB)( GLuint program );
        GLboolean  (WINE_GLAPI *p_glIsProgramNV)( GLuint id );
        GLboolean  (WINE_GLAPI *p_glIsProgramPipeline)( GLuint pipeline );
        GLboolean  (WINE_GLAPI *p_glIsQuery)( GLuint id );
        GLboolean  (WINE_GLAPI *p_glIsQueryARB)( GLuint id );
        GLboolean  (WINE_GLAPI *p_glIsRenderbuffer)( GLuint renderbuffer );
        GLboolean  (WINE_GLAPI *p_glIsRenderbufferEXT)( GLuint renderbuffer );
        GLboolean  (WINE_GLAPI *p_glIsSampler)( GLuint sampler );
        GLboolean  (WINE_GLAPI *p_glIsSemaphoreEXT)( GLuint semaphore );
        GLboolean  (WINE_GLAPI *p_glIsShader)( GLuint shader );
        GLboolean  (WINE_GLAPI *p_glIsStateNV)( GLuint state );
        GLboolean  (WINE_GLAPI *p_glIsSync)( GLsync sync );
        GLboolean  (WINE_GLAPI *p_glIsTextureEXT)( GLuint texture );
        GLboolean  (WINE_GLAPI *p_glIsTextureHandleResidentARB)( GLuint64 handle );
        GLboolean  (WINE_GLAPI *p_glIsTextureHandleResidentNV)( GLuint64 handle );
        GLboolean  (WINE_GLAPI *p_glIsTransformFeedback)( GLuint id );
        GLboolean  (WINE_GLAPI *p_glIsTransformFeedbackNV)( GLuint id );
        GLboolean  (WINE_GLAPI *p_glIsVariantEnabledEXT)( GLuint id, GLenum cap );
        GLboolean  (WINE_GLAPI *p_glIsVertexArray)( GLuint array );
        GLboolean  (WINE_GLAPI *p_glIsVertexArrayAPPLE)( GLuint array );
        GLboolean  (WINE_GLAPI *p_glIsVertexAttribEnabledAPPLE)( GLuint index, GLenum pname );
        void       (WINE_GLAPI *p_glLGPUCopyImageSubDataNVX)( GLuint sourceGpu, GLbitfield destinationGpuMask, GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srxY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei width, GLsizei height, GLsizei depth );
        void       (WINE_GLAPI *p_glLGPUInterlockNVX)(void);
        void       (WINE_GLAPI *p_glLGPUNamedBufferSubDataNVX)( GLbitfield gpuMask, GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data );
        void       (WINE_GLAPI *p_glLabelObjectEXT)( GLenum type, GLuint object, GLsizei length, const GLchar *label );
        void       (WINE_GLAPI *p_glLightEnviSGIX)( GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glLightModelxOES)( GLenum pname, GLfixed param );
        void       (WINE_GLAPI *p_glLightModelxvOES)( GLenum pname, const GLfixed *param );
        void       (WINE_GLAPI *p_glLightxOES)( GLenum light, GLenum pname, GLfixed param );
        void       (WINE_GLAPI *p_glLightxvOES)( GLenum light, GLenum pname, const GLfixed *params );
        void       (WINE_GLAPI *p_glLineWidthxOES)( GLfixed width );
        void       (WINE_GLAPI *p_glLinkProgram)( GLuint program );
        void       (WINE_GLAPI *p_glLinkProgramARB)( GLhandleARB programObj );
        void       (WINE_GLAPI *p_glListDrawCommandsStatesClientNV)( GLuint list, GLuint segment, const void **indirects, const GLsizei *sizes, const GLuint *states, const GLuint *fbos, GLuint count );
        void       (WINE_GLAPI *p_glListParameterfSGIX)( GLuint list, GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glListParameterfvSGIX)( GLuint list, GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glListParameteriSGIX)( GLuint list, GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glListParameterivSGIX)( GLuint list, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glLoadIdentityDeformationMapSGIX)( GLbitfield mask );
        void       (WINE_GLAPI *p_glLoadMatrixxOES)( const GLfixed *m );
        void       (WINE_GLAPI *p_glLoadProgramNV)( GLenum target, GLuint id, GLsizei len, const GLubyte *program );
        void       (WINE_GLAPI *p_glLoadTransposeMatrixd)( const GLdouble *m );
        void       (WINE_GLAPI *p_glLoadTransposeMatrixdARB)( const GLdouble *m );
        void       (WINE_GLAPI *p_glLoadTransposeMatrixf)( const GLfloat *m );
        void       (WINE_GLAPI *p_glLoadTransposeMatrixfARB)( const GLfloat *m );
        void       (WINE_GLAPI *p_glLoadTransposeMatrixxOES)( const GLfixed *m );
        void       (WINE_GLAPI *p_glLockArraysEXT)( GLint first, GLsizei count );
        void       (WINE_GLAPI *p_glMTexCoord2fSGIS)( GLenum target, GLfloat s, GLfloat t );
        void       (WINE_GLAPI *p_glMTexCoord2fvSGIS)( GLenum target, GLfloat * v );
        void       (WINE_GLAPI *p_glMakeBufferNonResidentNV)( GLenum target );
        void       (WINE_GLAPI *p_glMakeBufferResidentNV)( GLenum target, GLenum access );
        void       (WINE_GLAPI *p_glMakeImageHandleNonResidentARB)( GLuint64 handle );
        void       (WINE_GLAPI *p_glMakeImageHandleNonResidentNV)( GLuint64 handle );
        void       (WINE_GLAPI *p_glMakeImageHandleResidentARB)( GLuint64 handle, GLenum access );
        void       (WINE_GLAPI *p_glMakeImageHandleResidentNV)( GLuint64 handle, GLenum access );
        void       (WINE_GLAPI *p_glMakeNamedBufferNonResidentNV)( GLuint buffer );
        void       (WINE_GLAPI *p_glMakeNamedBufferResidentNV)( GLuint buffer, GLenum access );
        void       (WINE_GLAPI *p_glMakeTextureHandleNonResidentARB)( GLuint64 handle );
        void       (WINE_GLAPI *p_glMakeTextureHandleNonResidentNV)( GLuint64 handle );
        void       (WINE_GLAPI *p_glMakeTextureHandleResidentARB)( GLuint64 handle );
        void       (WINE_GLAPI *p_glMakeTextureHandleResidentNV)( GLuint64 handle );
        void       (WINE_GLAPI *p_glMap1xOES)( GLenum target, GLfixed u1, GLfixed u2, GLint stride, GLint order, GLfixed points );
        void       (WINE_GLAPI *p_glMap2xOES)( GLenum target, GLfixed u1, GLfixed u2, GLint ustride, GLint uorder, GLfixed v1, GLfixed v2, GLint vstride, GLint vorder, GLfixed points );
        void *     (WINE_GLAPI *p_glMapBuffer)( GLenum target, GLenum access );
        void *     (WINE_GLAPI *p_glMapBufferARB)( GLenum target, GLenum access );
        void *     (WINE_GLAPI *p_glMapBufferRange)( GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access );
        void       (WINE_GLAPI *p_glMapControlPointsNV)( GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLint uorder, GLint vorder, GLboolean packed, const void *points );
        void       (WINE_GLAPI *p_glMapGrid1xOES)( GLint n, GLfixed u1, GLfixed u2 );
        void       (WINE_GLAPI *p_glMapGrid2xOES)( GLint n, GLfixed u1, GLfixed u2, GLfixed v1, GLfixed v2 );
        void *     (WINE_GLAPI *p_glMapNamedBuffer)( GLuint buffer, GLenum access );
        void *     (WINE_GLAPI *p_glMapNamedBufferEXT)( GLuint buffer, GLenum access );
        void *     (WINE_GLAPI *p_glMapNamedBufferRange)( GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access );
        void *     (WINE_GLAPI *p_glMapNamedBufferRangeEXT)( GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access );
        void *     (WINE_GLAPI *p_glMapObjectBufferATI)( GLuint buffer );
        void       (WINE_GLAPI *p_glMapParameterfvNV)( GLenum target, GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glMapParameterivNV)( GLenum target, GLenum pname, const GLint *params );
        void *     (WINE_GLAPI *p_glMapTexture2DINTEL)( GLuint texture, GLint level, GLbitfield access, GLint *stride, GLenum *layout );
        void       (WINE_GLAPI *p_glMapVertexAttrib1dAPPLE)( GLuint index, GLuint size, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points );
        void       (WINE_GLAPI *p_glMapVertexAttrib1fAPPLE)( GLuint index, GLuint size, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points );
        void       (WINE_GLAPI *p_glMapVertexAttrib2dAPPLE)( GLuint index, GLuint size, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points );
        void       (WINE_GLAPI *p_glMapVertexAttrib2fAPPLE)( GLuint index, GLuint size, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points );
        void       (WINE_GLAPI *p_glMaterialxOES)( GLenum face, GLenum pname, GLfixed param );
        void       (WINE_GLAPI *p_glMaterialxvOES)( GLenum face, GLenum pname, const GLfixed *param );
        void       (WINE_GLAPI *p_glMatrixFrustumEXT)( GLenum mode, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar );
        void       (WINE_GLAPI *p_glMatrixIndexPointerARB)( GLint size, GLenum type, GLsizei stride, const void *pointer );
        void       (WINE_GLAPI *p_glMatrixIndexubvARB)( GLint size, const GLubyte *indices );
        void       (WINE_GLAPI *p_glMatrixIndexuivARB)( GLint size, const GLuint *indices );
        void       (WINE_GLAPI *p_glMatrixIndexusvARB)( GLint size, const GLushort *indices );
        void       (WINE_GLAPI *p_glMatrixLoad3x2fNV)( GLenum matrixMode, const GLfloat *m );
        void       (WINE_GLAPI *p_glMatrixLoad3x3fNV)( GLenum matrixMode, const GLfloat *m );
        void       (WINE_GLAPI *p_glMatrixLoadIdentityEXT)( GLenum mode );
        void       (WINE_GLAPI *p_glMatrixLoadTranspose3x3fNV)( GLenum matrixMode, const GLfloat *m );
        void       (WINE_GLAPI *p_glMatrixLoadTransposedEXT)( GLenum mode, const GLdouble *m );
        void       (WINE_GLAPI *p_glMatrixLoadTransposefEXT)( GLenum mode, const GLfloat *m );
        void       (WINE_GLAPI *p_glMatrixLoaddEXT)( GLenum mode, const GLdouble *m );
        void       (WINE_GLAPI *p_glMatrixLoadfEXT)( GLenum mode, const GLfloat *m );
        void       (WINE_GLAPI *p_glMatrixMult3x2fNV)( GLenum matrixMode, const GLfloat *m );
        void       (WINE_GLAPI *p_glMatrixMult3x3fNV)( GLenum matrixMode, const GLfloat *m );
        void       (WINE_GLAPI *p_glMatrixMultTranspose3x3fNV)( GLenum matrixMode, const GLfloat *m );
        void       (WINE_GLAPI *p_glMatrixMultTransposedEXT)( GLenum mode, const GLdouble *m );
        void       (WINE_GLAPI *p_glMatrixMultTransposefEXT)( GLenum mode, const GLfloat *m );
        void       (WINE_GLAPI *p_glMatrixMultdEXT)( GLenum mode, const GLdouble *m );
        void       (WINE_GLAPI *p_glMatrixMultfEXT)( GLenum mode, const GLfloat *m );
        void       (WINE_GLAPI *p_glMatrixOrthoEXT)( GLenum mode, GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar );
        void       (WINE_GLAPI *p_glMatrixPopEXT)( GLenum mode );
        void       (WINE_GLAPI *p_glMatrixPushEXT)( GLenum mode );
        void       (WINE_GLAPI *p_glMatrixRotatedEXT)( GLenum mode, GLdouble angle, GLdouble x, GLdouble y, GLdouble z );
        void       (WINE_GLAPI *p_glMatrixRotatefEXT)( GLenum mode, GLfloat angle, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glMatrixScaledEXT)( GLenum mode, GLdouble x, GLdouble y, GLdouble z );
        void       (WINE_GLAPI *p_glMatrixScalefEXT)( GLenum mode, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glMatrixTranslatedEXT)( GLenum mode, GLdouble x, GLdouble y, GLdouble z );
        void       (WINE_GLAPI *p_glMatrixTranslatefEXT)( GLenum mode, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glMaxShaderCompilerThreadsARB)( GLuint count );
        void       (WINE_GLAPI *p_glMaxShaderCompilerThreadsKHR)( GLuint count );
        void       (WINE_GLAPI *p_glMemoryBarrier)( GLbitfield barriers );
        void       (WINE_GLAPI *p_glMemoryBarrierByRegion)( GLbitfield barriers );
        void       (WINE_GLAPI *p_glMemoryBarrierEXT)( GLbitfield barriers );
        void       (WINE_GLAPI *p_glMemoryObjectParameterivEXT)( GLuint memoryObject, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glMinSampleShading)( GLfloat value );
        void       (WINE_GLAPI *p_glMinSampleShadingARB)( GLfloat value );
        void       (WINE_GLAPI *p_glMinmax)( GLenum target, GLenum internalformat, GLboolean sink );
        void       (WINE_GLAPI *p_glMinmaxEXT)( GLenum target, GLenum internalformat, GLboolean sink );
        void       (WINE_GLAPI *p_glMultMatrixxOES)( const GLfixed *m );
        void       (WINE_GLAPI *p_glMultTransposeMatrixd)( const GLdouble *m );
        void       (WINE_GLAPI *p_glMultTransposeMatrixdARB)( const GLdouble *m );
        void       (WINE_GLAPI *p_glMultTransposeMatrixf)( const GLfloat *m );
        void       (WINE_GLAPI *p_glMultTransposeMatrixfARB)( const GLfloat *m );
        void       (WINE_GLAPI *p_glMultTransposeMatrixxOES)( const GLfixed *m );
        void       (WINE_GLAPI *p_glMultiDrawArrays)( GLenum mode, const GLint *first, const GLsizei *count, GLsizei drawcount );
        void       (WINE_GLAPI *p_glMultiDrawArraysEXT)( GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount );
        void       (WINE_GLAPI *p_glMultiDrawArraysIndirect)( GLenum mode, const void *indirect, GLsizei drawcount, GLsizei stride );
        void       (WINE_GLAPI *p_glMultiDrawArraysIndirectAMD)( GLenum mode, const void *indirect, GLsizei primcount, GLsizei stride );
        void       (WINE_GLAPI *p_glMultiDrawArraysIndirectBindlessCountNV)( GLenum mode, const void *indirect, GLsizei drawCount, GLsizei maxDrawCount, GLsizei stride, GLint vertexBufferCount );
        void       (WINE_GLAPI *p_glMultiDrawArraysIndirectBindlessNV)( GLenum mode, const void *indirect, GLsizei drawCount, GLsizei stride, GLint vertexBufferCount );
        void       (WINE_GLAPI *p_glMultiDrawArraysIndirectCount)( GLenum mode, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride );
        void       (WINE_GLAPI *p_glMultiDrawArraysIndirectCountARB)( GLenum mode, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride );
        void       (WINE_GLAPI *p_glMultiDrawElementArrayAPPLE)( GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount );
        void       (WINE_GLAPI *p_glMultiDrawElements)( GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount );
        void       (WINE_GLAPI *p_glMultiDrawElementsBaseVertex)( GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount, const GLint *basevertex );
        void       (WINE_GLAPI *p_glMultiDrawElementsEXT)( GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei primcount );
        void       (WINE_GLAPI *p_glMultiDrawElementsIndirect)( GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride );
        void       (WINE_GLAPI *p_glMultiDrawElementsIndirectAMD)( GLenum mode, GLenum type, const void *indirect, GLsizei primcount, GLsizei stride );
        void       (WINE_GLAPI *p_glMultiDrawElementsIndirectBindlessCountNV)( GLenum mode, GLenum type, const void *indirect, GLsizei drawCount, GLsizei maxDrawCount, GLsizei stride, GLint vertexBufferCount );
        void       (WINE_GLAPI *p_glMultiDrawElementsIndirectBindlessNV)( GLenum mode, GLenum type, const void *indirect, GLsizei drawCount, GLsizei stride, GLint vertexBufferCount );
        void       (WINE_GLAPI *p_glMultiDrawElementsIndirectCount)( GLenum mode, GLenum type, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride );
        void       (WINE_GLAPI *p_glMultiDrawElementsIndirectCountARB)( GLenum mode, GLenum type, const void *indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride );
        void       (WINE_GLAPI *p_glMultiDrawMeshTasksIndirectCountNV)( GLintptr indirect, GLintptr drawcount, GLsizei maxdrawcount, GLsizei stride );
        void       (WINE_GLAPI *p_glMultiDrawMeshTasksIndirectNV)( GLintptr indirect, GLsizei drawcount, GLsizei stride );
        void       (WINE_GLAPI *p_glMultiDrawRangeElementArrayAPPLE)( GLenum mode, GLuint start, GLuint end, const GLint *first, const GLsizei *count, GLsizei primcount );
        void       (WINE_GLAPI *p_glMultiModeDrawArraysIBM)( const GLenum *mode, const GLint *first, const GLsizei *count, GLsizei primcount, GLint modestride );
        void       (WINE_GLAPI *p_glMultiModeDrawElementsIBM)( const GLenum *mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei primcount, GLint modestride );
        void       (WINE_GLAPI *p_glMultiTexBufferEXT)( GLenum texunit, GLenum target, GLenum internalformat, GLuint buffer );
        void       (WINE_GLAPI *p_glMultiTexCoord1bOES)( GLenum texture, GLbyte s );
        void       (WINE_GLAPI *p_glMultiTexCoord1bvOES)( GLenum texture, const GLbyte *coords );
        void       (WINE_GLAPI *p_glMultiTexCoord1d)( GLenum target, GLdouble s );
        void       (WINE_GLAPI *p_glMultiTexCoord1dARB)( GLenum target, GLdouble s );
        void       (WINE_GLAPI *p_glMultiTexCoord1dSGIS)( GLenum target, GLdouble s );
        void       (WINE_GLAPI *p_glMultiTexCoord1dv)( GLenum target, const GLdouble *v );
        void       (WINE_GLAPI *p_glMultiTexCoord1dvARB)( GLenum target, const GLdouble *v );
        void       (WINE_GLAPI *p_glMultiTexCoord1dvSGIS)( GLenum target, GLdouble * v );
        void       (WINE_GLAPI *p_glMultiTexCoord1f)( GLenum target, GLfloat s );
        void       (WINE_GLAPI *p_glMultiTexCoord1fARB)( GLenum target, GLfloat s );
        void       (WINE_GLAPI *p_glMultiTexCoord1fSGIS)( GLenum target, GLfloat s );
        void       (WINE_GLAPI *p_glMultiTexCoord1fv)( GLenum target, const GLfloat *v );
        void       (WINE_GLAPI *p_glMultiTexCoord1fvARB)( GLenum target, const GLfloat *v );
        void       (WINE_GLAPI *p_glMultiTexCoord1fvSGIS)( GLenum target, const GLfloat * v );
        void       (WINE_GLAPI *p_glMultiTexCoord1hNV)( GLenum target, GLhalfNV s );
        void       (WINE_GLAPI *p_glMultiTexCoord1hvNV)( GLenum target, const GLhalfNV *v );
        void       (WINE_GLAPI *p_glMultiTexCoord1i)( GLenum target, GLint s );
        void       (WINE_GLAPI *p_glMultiTexCoord1iARB)( GLenum target, GLint s );
        void       (WINE_GLAPI *p_glMultiTexCoord1iSGIS)( GLenum target, GLint s );
        void       (WINE_GLAPI *p_glMultiTexCoord1iv)( GLenum target, const GLint *v );
        void       (WINE_GLAPI *p_glMultiTexCoord1ivARB)( GLenum target, const GLint *v );
        void       (WINE_GLAPI *p_glMultiTexCoord1ivSGIS)( GLenum target, GLint * v );
        void       (WINE_GLAPI *p_glMultiTexCoord1s)( GLenum target, GLshort s );
        void       (WINE_GLAPI *p_glMultiTexCoord1sARB)( GLenum target, GLshort s );
        void       (WINE_GLAPI *p_glMultiTexCoord1sSGIS)( GLenum target, GLshort s );
        void       (WINE_GLAPI *p_glMultiTexCoord1sv)( GLenum target, const GLshort *v );
        void       (WINE_GLAPI *p_glMultiTexCoord1svARB)( GLenum target, const GLshort *v );
        void       (WINE_GLAPI *p_glMultiTexCoord1svSGIS)( GLenum target, GLshort * v );
        void       (WINE_GLAPI *p_glMultiTexCoord1xOES)( GLenum texture, GLfixed s );
        void       (WINE_GLAPI *p_glMultiTexCoord1xvOES)( GLenum texture, const GLfixed *coords );
        void       (WINE_GLAPI *p_glMultiTexCoord2bOES)( GLenum texture, GLbyte s, GLbyte t );
        void       (WINE_GLAPI *p_glMultiTexCoord2bvOES)( GLenum texture, const GLbyte *coords );
        void       (WINE_GLAPI *p_glMultiTexCoord2d)( GLenum target, GLdouble s, GLdouble t );
        void       (WINE_GLAPI *p_glMultiTexCoord2dARB)( GLenum target, GLdouble s, GLdouble t );
        void       (WINE_GLAPI *p_glMultiTexCoord2dSGIS)( GLenum target, GLdouble s, GLdouble t );
        void       (WINE_GLAPI *p_glMultiTexCoord2dv)( GLenum target, const GLdouble *v );
        void       (WINE_GLAPI *p_glMultiTexCoord2dvARB)( GLenum target, const GLdouble *v );
        void       (WINE_GLAPI *p_glMultiTexCoord2dvSGIS)( GLenum target, GLdouble * v );
        void       (WINE_GLAPI *p_glMultiTexCoord2f)( GLenum target, GLfloat s, GLfloat t );
        void       (WINE_GLAPI *p_glMultiTexCoord2fARB)( GLenum target, GLfloat s, GLfloat t );
        void       (WINE_GLAPI *p_glMultiTexCoord2fSGIS)( GLenum target, GLfloat s, GLfloat t );
        void       (WINE_GLAPI *p_glMultiTexCoord2fv)( GLenum target, const GLfloat *v );
        void       (WINE_GLAPI *p_glMultiTexCoord2fvARB)( GLenum target, const GLfloat *v );
        void       (WINE_GLAPI *p_glMultiTexCoord2fvSGIS)( GLenum target, GLfloat * v );
        void       (WINE_GLAPI *p_glMultiTexCoord2hNV)( GLenum target, GLhalfNV s, GLhalfNV t );
        void       (WINE_GLAPI *p_glMultiTexCoord2hvNV)( GLenum target, const GLhalfNV *v );
        void       (WINE_GLAPI *p_glMultiTexCoord2i)( GLenum target, GLint s, GLint t );
        void       (WINE_GLAPI *p_glMultiTexCoord2iARB)( GLenum target, GLint s, GLint t );
        void       (WINE_GLAPI *p_glMultiTexCoord2iSGIS)( GLenum target, GLint s, GLint t );
        void       (WINE_GLAPI *p_glMultiTexCoord2iv)( GLenum target, const GLint *v );
        void       (WINE_GLAPI *p_glMultiTexCoord2ivARB)( GLenum target, const GLint *v );
        void       (WINE_GLAPI *p_glMultiTexCoord2ivSGIS)( GLenum target, GLint * v );
        void       (WINE_GLAPI *p_glMultiTexCoord2s)( GLenum target, GLshort s, GLshort t );
        void       (WINE_GLAPI *p_glMultiTexCoord2sARB)( GLenum target, GLshort s, GLshort t );
        void       (WINE_GLAPI *p_glMultiTexCoord2sSGIS)( GLenum target, GLshort s, GLshort t );
        void       (WINE_GLAPI *p_glMultiTexCoord2sv)( GLenum target, const GLshort *v );
        void       (WINE_GLAPI *p_glMultiTexCoord2svARB)( GLenum target, const GLshort *v );
        void       (WINE_GLAPI *p_glMultiTexCoord2svSGIS)( GLenum target, GLshort * v );
        void       (WINE_GLAPI *p_glMultiTexCoord2xOES)( GLenum texture, GLfixed s, GLfixed t );
        void       (WINE_GLAPI *p_glMultiTexCoord2xvOES)( GLenum texture, const GLfixed *coords );
        void       (WINE_GLAPI *p_glMultiTexCoord3bOES)( GLenum texture, GLbyte s, GLbyte t, GLbyte r );
        void       (WINE_GLAPI *p_glMultiTexCoord3bvOES)( GLenum texture, const GLbyte *coords );
        void       (WINE_GLAPI *p_glMultiTexCoord3d)( GLenum target, GLdouble s, GLdouble t, GLdouble r );
        void       (WINE_GLAPI *p_glMultiTexCoord3dARB)( GLenum target, GLdouble s, GLdouble t, GLdouble r );
        void       (WINE_GLAPI *p_glMultiTexCoord3dSGIS)( GLenum target, GLdouble s, GLdouble t, GLdouble r );
        void       (WINE_GLAPI *p_glMultiTexCoord3dv)( GLenum target, const GLdouble *v );
        void       (WINE_GLAPI *p_glMultiTexCoord3dvARB)( GLenum target, const GLdouble *v );
        void       (WINE_GLAPI *p_glMultiTexCoord3dvSGIS)( GLenum target, GLdouble * v );
        void       (WINE_GLAPI *p_glMultiTexCoord3f)( GLenum target, GLfloat s, GLfloat t, GLfloat r );
        void       (WINE_GLAPI *p_glMultiTexCoord3fARB)( GLenum target, GLfloat s, GLfloat t, GLfloat r );
        void       (WINE_GLAPI *p_glMultiTexCoord3fSGIS)( GLenum target, GLfloat s, GLfloat t, GLfloat r );
        void       (WINE_GLAPI *p_glMultiTexCoord3fv)( GLenum target, const GLfloat *v );
        void       (WINE_GLAPI *p_glMultiTexCoord3fvARB)( GLenum target, const GLfloat *v );
        void       (WINE_GLAPI *p_glMultiTexCoord3fvSGIS)( GLenum target, GLfloat * v );
        void       (WINE_GLAPI *p_glMultiTexCoord3hNV)( GLenum target, GLhalfNV s, GLhalfNV t, GLhalfNV r );
        void       (WINE_GLAPI *p_glMultiTexCoord3hvNV)( GLenum target, const GLhalfNV *v );
        void       (WINE_GLAPI *p_glMultiTexCoord3i)( GLenum target, GLint s, GLint t, GLint r );
        void       (WINE_GLAPI *p_glMultiTexCoord3iARB)( GLenum target, GLint s, GLint t, GLint r );
        void       (WINE_GLAPI *p_glMultiTexCoord3iSGIS)( GLenum target, GLint s, GLint t, GLint r );
        void       (WINE_GLAPI *p_glMultiTexCoord3iv)( GLenum target, const GLint *v );
        void       (WINE_GLAPI *p_glMultiTexCoord3ivARB)( GLenum target, const GLint *v );
        void       (WINE_GLAPI *p_glMultiTexCoord3ivSGIS)( GLenum target, GLint * v );
        void       (WINE_GLAPI *p_glMultiTexCoord3s)( GLenum target, GLshort s, GLshort t, GLshort r );
        void       (WINE_GLAPI *p_glMultiTexCoord3sARB)( GLenum target, GLshort s, GLshort t, GLshort r );
        void       (WINE_GLAPI *p_glMultiTexCoord3sSGIS)( GLenum target, GLshort s, GLshort t, GLshort r );
        void       (WINE_GLAPI *p_glMultiTexCoord3sv)( GLenum target, const GLshort *v );
        void       (WINE_GLAPI *p_glMultiTexCoord3svARB)( GLenum target, const GLshort *v );
        void       (WINE_GLAPI *p_glMultiTexCoord3svSGIS)( GLenum target, GLshort * v );
        void       (WINE_GLAPI *p_glMultiTexCoord3xOES)( GLenum texture, GLfixed s, GLfixed t, GLfixed r );
        void       (WINE_GLAPI *p_glMultiTexCoord3xvOES)( GLenum texture, const GLfixed *coords );
        void       (WINE_GLAPI *p_glMultiTexCoord4bOES)( GLenum texture, GLbyte s, GLbyte t, GLbyte r, GLbyte q );
        void       (WINE_GLAPI *p_glMultiTexCoord4bvOES)( GLenum texture, const GLbyte *coords );
        void       (WINE_GLAPI *p_glMultiTexCoord4d)( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q );
        void       (WINE_GLAPI *p_glMultiTexCoord4dARB)( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q );
        void       (WINE_GLAPI *p_glMultiTexCoord4dSGIS)( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q );
        void       (WINE_GLAPI *p_glMultiTexCoord4dv)( GLenum target, const GLdouble *v );
        void       (WINE_GLAPI *p_glMultiTexCoord4dvARB)( GLenum target, const GLdouble *v );
        void       (WINE_GLAPI *p_glMultiTexCoord4dvSGIS)( GLenum target, GLdouble * v );
        void       (WINE_GLAPI *p_glMultiTexCoord4f)( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q );
        void       (WINE_GLAPI *p_glMultiTexCoord4fARB)( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q );
        void       (WINE_GLAPI *p_glMultiTexCoord4fSGIS)( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q );
        void       (WINE_GLAPI *p_glMultiTexCoord4fv)( GLenum target, const GLfloat *v );
        void       (WINE_GLAPI *p_glMultiTexCoord4fvARB)( GLenum target, const GLfloat *v );
        void       (WINE_GLAPI *p_glMultiTexCoord4fvSGIS)( GLenum target, GLfloat * v );
        void       (WINE_GLAPI *p_glMultiTexCoord4hNV)( GLenum target, GLhalfNV s, GLhalfNV t, GLhalfNV r, GLhalfNV q );
        void       (WINE_GLAPI *p_glMultiTexCoord4hvNV)( GLenum target, const GLhalfNV *v );
        void       (WINE_GLAPI *p_glMultiTexCoord4i)( GLenum target, GLint s, GLint t, GLint r, GLint q );
        void       (WINE_GLAPI *p_glMultiTexCoord4iARB)( GLenum target, GLint s, GLint t, GLint r, GLint q );
        void       (WINE_GLAPI *p_glMultiTexCoord4iSGIS)( GLenum target, GLint s, GLint t, GLint r, GLint q );
        void       (WINE_GLAPI *p_glMultiTexCoord4iv)( GLenum target, const GLint *v );
        void       (WINE_GLAPI *p_glMultiTexCoord4ivARB)( GLenum target, const GLint *v );
        void       (WINE_GLAPI *p_glMultiTexCoord4ivSGIS)( GLenum target, GLint * v );
        void       (WINE_GLAPI *p_glMultiTexCoord4s)( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q );
        void       (WINE_GLAPI *p_glMultiTexCoord4sARB)( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q );
        void       (WINE_GLAPI *p_glMultiTexCoord4sSGIS)( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q );
        void       (WINE_GLAPI *p_glMultiTexCoord4sv)( GLenum target, const GLshort *v );
        void       (WINE_GLAPI *p_glMultiTexCoord4svARB)( GLenum target, const GLshort *v );
        void       (WINE_GLAPI *p_glMultiTexCoord4svSGIS)( GLenum target, GLshort * v );
        void       (WINE_GLAPI *p_glMultiTexCoord4xOES)( GLenum texture, GLfixed s, GLfixed t, GLfixed r, GLfixed q );
        void       (WINE_GLAPI *p_glMultiTexCoord4xvOES)( GLenum texture, const GLfixed *coords );
        void       (WINE_GLAPI *p_glMultiTexCoordP1ui)( GLenum texture, GLenum type, GLuint coords );
        void       (WINE_GLAPI *p_glMultiTexCoordP1uiv)( GLenum texture, GLenum type, const GLuint *coords );
        void       (WINE_GLAPI *p_glMultiTexCoordP2ui)( GLenum texture, GLenum type, GLuint coords );
        void       (WINE_GLAPI *p_glMultiTexCoordP2uiv)( GLenum texture, GLenum type, const GLuint *coords );
        void       (WINE_GLAPI *p_glMultiTexCoordP3ui)( GLenum texture, GLenum type, GLuint coords );
        void       (WINE_GLAPI *p_glMultiTexCoordP3uiv)( GLenum texture, GLenum type, const GLuint *coords );
        void       (WINE_GLAPI *p_glMultiTexCoordP4ui)( GLenum texture, GLenum type, GLuint coords );
        void       (WINE_GLAPI *p_glMultiTexCoordP4uiv)( GLenum texture, GLenum type, const GLuint *coords );
        void       (WINE_GLAPI *p_glMultiTexCoordPointerEXT)( GLenum texunit, GLint size, GLenum type, GLsizei stride, const void *pointer );
        void       (WINE_GLAPI *p_glMultiTexCoordPointerSGIS)( GLenum target, GLint size, GLenum type, GLsizei stride, GLvoid * pointer );
        void       (WINE_GLAPI *p_glMultiTexEnvfEXT)( GLenum texunit, GLenum target, GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glMultiTexEnvfvEXT)( GLenum texunit, GLenum target, GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glMultiTexEnviEXT)( GLenum texunit, GLenum target, GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glMultiTexEnvivEXT)( GLenum texunit, GLenum target, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glMultiTexGendEXT)( GLenum texunit, GLenum coord, GLenum pname, GLdouble param );
        void       (WINE_GLAPI *p_glMultiTexGendvEXT)( GLenum texunit, GLenum coord, GLenum pname, const GLdouble *params );
        void       (WINE_GLAPI *p_glMultiTexGenfEXT)( GLenum texunit, GLenum coord, GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glMultiTexGenfvEXT)( GLenum texunit, GLenum coord, GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glMultiTexGeniEXT)( GLenum texunit, GLenum coord, GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glMultiTexGenivEXT)( GLenum texunit, GLenum coord, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glMultiTexImage1DEXT)( GLenum texunit, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glMultiTexImage2DEXT)( GLenum texunit, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glMultiTexImage3DEXT)( GLenum texunit, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glMultiTexParameterIivEXT)( GLenum texunit, GLenum target, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glMultiTexParameterIuivEXT)( GLenum texunit, GLenum target, GLenum pname, const GLuint *params );
        void       (WINE_GLAPI *p_glMultiTexParameterfEXT)( GLenum texunit, GLenum target, GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glMultiTexParameterfvEXT)( GLenum texunit, GLenum target, GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glMultiTexParameteriEXT)( GLenum texunit, GLenum target, GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glMultiTexParameterivEXT)( GLenum texunit, GLenum target, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glMultiTexRenderbufferEXT)( GLenum texunit, GLenum target, GLuint renderbuffer );
        void       (WINE_GLAPI *p_glMultiTexSubImage1DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glMultiTexSubImage2DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glMultiTexSubImage3DEXT)( GLenum texunit, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glMulticastBarrierNV)(void);
        void       (WINE_GLAPI *p_glMulticastBlitFramebufferNV)( GLuint srcGpu, GLuint dstGpu, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter );
        void       (WINE_GLAPI *p_glMulticastBufferSubDataNV)( GLbitfield gpuMask, GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data );
        void       (WINE_GLAPI *p_glMulticastCopyBufferSubDataNV)( GLuint readGpu, GLbitfield writeGpuMask, GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size );
        void       (WINE_GLAPI *p_glMulticastCopyImageSubDataNV)( GLuint srcGpu, GLbitfield dstGpuMask, GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth );
        void       (WINE_GLAPI *p_glMulticastFramebufferSampleLocationsfvNV)( GLuint gpu, GLuint framebuffer, GLuint start, GLsizei count, const GLfloat *v );
        void       (WINE_GLAPI *p_glMulticastGetQueryObjecti64vNV)( GLuint gpu, GLuint id, GLenum pname, GLint64 *params );
        void       (WINE_GLAPI *p_glMulticastGetQueryObjectivNV)( GLuint gpu, GLuint id, GLenum pname, GLint *params );
        void       (WINE_GLAPI *p_glMulticastGetQueryObjectui64vNV)( GLuint gpu, GLuint id, GLenum pname, GLuint64 *params );
        void       (WINE_GLAPI *p_glMulticastGetQueryObjectuivNV)( GLuint gpu, GLuint id, GLenum pname, GLuint *params );
        void       (WINE_GLAPI *p_glMulticastScissorArrayvNVX)( GLuint gpu, GLuint first, GLsizei count, const GLint *v );
        void       (WINE_GLAPI *p_glMulticastViewportArrayvNVX)( GLuint gpu, GLuint first, GLsizei count, const GLfloat *v );
        void       (WINE_GLAPI *p_glMulticastViewportPositionWScaleNVX)( GLuint gpu, GLuint index, GLfloat xcoeff, GLfloat ycoeff );
        void       (WINE_GLAPI *p_glMulticastWaitSyncNV)( GLuint signalGpu, GLbitfield waitGpuMask );
        void       (WINE_GLAPI *p_glNamedBufferAttachMemoryNV)( GLuint buffer, GLuint memory, GLuint64 offset );
        void       (WINE_GLAPI *p_glNamedBufferData)( GLuint buffer, GLsizeiptr size, const void *data, GLenum usage );
        void       (WINE_GLAPI *p_glNamedBufferDataEXT)( GLuint buffer, GLsizeiptr size, const void *data, GLenum usage );
        void       (WINE_GLAPI *p_glNamedBufferPageCommitmentARB)( GLuint buffer, GLintptr offset, GLsizeiptr size, GLboolean commit );
        void       (WINE_GLAPI *p_glNamedBufferPageCommitmentEXT)( GLuint buffer, GLintptr offset, GLsizeiptr size, GLboolean commit );
        void       (WINE_GLAPI *p_glNamedBufferStorage)( GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags );
        void       (WINE_GLAPI *p_glNamedBufferStorageEXT)( GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags );
        void       (WINE_GLAPI *p_glNamedBufferStorageExternalEXT)( GLuint buffer, GLintptr offset, GLsizeiptr size, GLeglClientBufferEXT clientBuffer, GLbitfield flags );
        void       (WINE_GLAPI *p_glNamedBufferStorageMemEXT)( GLuint buffer, GLsizeiptr size, GLuint memory, GLuint64 offset );
        void       (WINE_GLAPI *p_glNamedBufferSubData)( GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data );
        void       (WINE_GLAPI *p_glNamedBufferSubDataEXT)( GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data );
        void       (WINE_GLAPI *p_glNamedCopyBufferSubDataEXT)( GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size );
        void       (WINE_GLAPI *p_glNamedFramebufferDrawBuffer)( GLuint framebuffer, GLenum buf );
        void       (WINE_GLAPI *p_glNamedFramebufferDrawBuffers)( GLuint framebuffer, GLsizei n, const GLenum *bufs );
        void       (WINE_GLAPI *p_glNamedFramebufferParameteri)( GLuint framebuffer, GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glNamedFramebufferParameteriEXT)( GLuint framebuffer, GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glNamedFramebufferReadBuffer)( GLuint framebuffer, GLenum src );
        void       (WINE_GLAPI *p_glNamedFramebufferRenderbuffer)( GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer );
        void       (WINE_GLAPI *p_glNamedFramebufferRenderbufferEXT)( GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer );
        void       (WINE_GLAPI *p_glNamedFramebufferSampleLocationsfvARB)( GLuint framebuffer, GLuint start, GLsizei count, const GLfloat *v );
        void       (WINE_GLAPI *p_glNamedFramebufferSampleLocationsfvNV)( GLuint framebuffer, GLuint start, GLsizei count, const GLfloat *v );
        void       (WINE_GLAPI *p_glNamedFramebufferSamplePositionsfvAMD)( GLuint framebuffer, GLuint numsamples, GLuint pixelindex, const GLfloat *values );
        void       (WINE_GLAPI *p_glNamedFramebufferTexture)( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level );
        void       (WINE_GLAPI *p_glNamedFramebufferTexture1DEXT)( GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level );
        void       (WINE_GLAPI *p_glNamedFramebufferTexture2DEXT)( GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level );
        void       (WINE_GLAPI *p_glNamedFramebufferTexture3DEXT)( GLuint framebuffer, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset );
        void       (WINE_GLAPI *p_glNamedFramebufferTextureEXT)( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level );
        void       (WINE_GLAPI *p_glNamedFramebufferTextureFaceEXT)( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLenum face );
        void       (WINE_GLAPI *p_glNamedFramebufferTextureLayer)( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer );
        void       (WINE_GLAPI *p_glNamedFramebufferTextureLayerEXT)( GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer );
        void       (WINE_GLAPI *p_glNamedProgramLocalParameter4dEXT)( GLuint program, GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
        void       (WINE_GLAPI *p_glNamedProgramLocalParameter4dvEXT)( GLuint program, GLenum target, GLuint index, const GLdouble *params );
        void       (WINE_GLAPI *p_glNamedProgramLocalParameter4fEXT)( GLuint program, GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
        void       (WINE_GLAPI *p_glNamedProgramLocalParameter4fvEXT)( GLuint program, GLenum target, GLuint index, const GLfloat *params );
        void       (WINE_GLAPI *p_glNamedProgramLocalParameterI4iEXT)( GLuint program, GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w );
        void       (WINE_GLAPI *p_glNamedProgramLocalParameterI4ivEXT)( GLuint program, GLenum target, GLuint index, const GLint *params );
        void       (WINE_GLAPI *p_glNamedProgramLocalParameterI4uiEXT)( GLuint program, GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w );
        void       (WINE_GLAPI *p_glNamedProgramLocalParameterI4uivEXT)( GLuint program, GLenum target, GLuint index, const GLuint *params );
        void       (WINE_GLAPI *p_glNamedProgramLocalParameters4fvEXT)( GLuint program, GLenum target, GLuint index, GLsizei count, const GLfloat *params );
        void       (WINE_GLAPI *p_glNamedProgramLocalParametersI4ivEXT)( GLuint program, GLenum target, GLuint index, GLsizei count, const GLint *params );
        void       (WINE_GLAPI *p_glNamedProgramLocalParametersI4uivEXT)( GLuint program, GLenum target, GLuint index, GLsizei count, const GLuint *params );
        void       (WINE_GLAPI *p_glNamedProgramStringEXT)( GLuint program, GLenum target, GLenum format, GLsizei len, const void *string );
        void       (WINE_GLAPI *p_glNamedRenderbufferStorage)( GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glNamedRenderbufferStorageEXT)( GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glNamedRenderbufferStorageMultisample)( GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glNamedRenderbufferStorageMultisampleAdvancedAMD)( GLuint renderbuffer, GLsizei samples, GLsizei storageSamples, GLenum internalformat, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glNamedRenderbufferStorageMultisampleCoverageEXT)( GLuint renderbuffer, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glNamedRenderbufferStorageMultisampleEXT)( GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glNamedStringARB)( GLenum type, GLint namelen, const GLchar *name, GLint stringlen, const GLchar *string );
        GLuint     (WINE_GLAPI *p_glNewBufferRegion)( GLenum type );
        GLuint     (WINE_GLAPI *p_glNewObjectBufferATI)( GLsizei size, const void *pointer, GLenum usage );
        void       (WINE_GLAPI *p_glNormal3fVertex3fSUN)( GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glNormal3fVertex3fvSUN)( const GLfloat *n, const GLfloat *v );
        void       (WINE_GLAPI *p_glNormal3hNV)( GLhalfNV nx, GLhalfNV ny, GLhalfNV nz );
        void       (WINE_GLAPI *p_glNormal3hvNV)( const GLhalfNV *v );
        void       (WINE_GLAPI *p_glNormal3xOES)( GLfixed nx, GLfixed ny, GLfixed nz );
        void       (WINE_GLAPI *p_glNormal3xvOES)( const GLfixed *coords );
        void       (WINE_GLAPI *p_glNormalFormatNV)( GLenum type, GLsizei stride );
        void       (WINE_GLAPI *p_glNormalP3ui)( GLenum type, GLuint coords );
        void       (WINE_GLAPI *p_glNormalP3uiv)( GLenum type, const GLuint *coords );
        void       (WINE_GLAPI *p_glNormalPointerEXT)( GLenum type, GLsizei stride, GLsizei count, const void *pointer );
        void       (WINE_GLAPI *p_glNormalPointerListIBM)( GLenum type, GLint stride, const void **pointer, GLint ptrstride );
        void       (WINE_GLAPI *p_glNormalPointervINTEL)( GLenum type, const void **pointer );
        void       (WINE_GLAPI *p_glNormalStream3bATI)( GLenum stream, GLbyte nx, GLbyte ny, GLbyte nz );
        void       (WINE_GLAPI *p_glNormalStream3bvATI)( GLenum stream, const GLbyte *coords );
        void       (WINE_GLAPI *p_glNormalStream3dATI)( GLenum stream, GLdouble nx, GLdouble ny, GLdouble nz );
        void       (WINE_GLAPI *p_glNormalStream3dvATI)( GLenum stream, const GLdouble *coords );
        void       (WINE_GLAPI *p_glNormalStream3fATI)( GLenum stream, GLfloat nx, GLfloat ny, GLfloat nz );
        void       (WINE_GLAPI *p_glNormalStream3fvATI)( GLenum stream, const GLfloat *coords );
        void       (WINE_GLAPI *p_glNormalStream3iATI)( GLenum stream, GLint nx, GLint ny, GLint nz );
        void       (WINE_GLAPI *p_glNormalStream3ivATI)( GLenum stream, const GLint *coords );
        void       (WINE_GLAPI *p_glNormalStream3sATI)( GLenum stream, GLshort nx, GLshort ny, GLshort nz );
        void       (WINE_GLAPI *p_glNormalStream3svATI)( GLenum stream, const GLshort *coords );
        void       (WINE_GLAPI *p_glObjectLabel)( GLenum identifier, GLuint name, GLsizei length, const GLchar *label );
        void       (WINE_GLAPI *p_glObjectPtrLabel)( const void *ptr, GLsizei length, const GLchar *label );
        GLenum     (WINE_GLAPI *p_glObjectPurgeableAPPLE)( GLenum objectType, GLuint name, GLenum option );
        GLenum     (WINE_GLAPI *p_glObjectUnpurgeableAPPLE)( GLenum objectType, GLuint name, GLenum option );
        void       (WINE_GLAPI *p_glOrthofOES)( GLfloat l, GLfloat r, GLfloat b, GLfloat t, GLfloat n, GLfloat f );
        void       (WINE_GLAPI *p_glOrthoxOES)( GLfixed l, GLfixed r, GLfixed b, GLfixed t, GLfixed n, GLfixed f );
        void       (WINE_GLAPI *p_glPNTrianglesfATI)( GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glPNTrianglesiATI)( GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glPassTexCoordATI)( GLuint dst, GLuint coord, GLenum swizzle );
        void       (WINE_GLAPI *p_glPassThroughxOES)( GLfixed token );
        void       (WINE_GLAPI *p_glPatchParameterfv)( GLenum pname, const GLfloat *values );
        void       (WINE_GLAPI *p_glPatchParameteri)( GLenum pname, GLint value );
        void       (WINE_GLAPI *p_glPathColorGenNV)( GLenum color, GLenum genMode, GLenum colorFormat, const GLfloat *coeffs );
        void       (WINE_GLAPI *p_glPathCommandsNV)( GLuint path, GLsizei numCommands, const GLubyte *commands, GLsizei numCoords, GLenum coordType, const void *coords );
        void       (WINE_GLAPI *p_glPathCoordsNV)( GLuint path, GLsizei numCoords, GLenum coordType, const void *coords );
        void       (WINE_GLAPI *p_glPathCoverDepthFuncNV)( GLenum func );
        void       (WINE_GLAPI *p_glPathDashArrayNV)( GLuint path, GLsizei dashCount, const GLfloat *dashArray );
        void       (WINE_GLAPI *p_glPathFogGenNV)( GLenum genMode );
        GLenum     (WINE_GLAPI *p_glPathGlyphIndexArrayNV)( GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale );
        GLenum     (WINE_GLAPI *p_glPathGlyphIndexRangeNV)( GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint pathParameterTemplate, GLfloat emScale, GLuint baseAndCount[2] );
        void       (WINE_GLAPI *p_glPathGlyphRangeNV)( GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLuint firstGlyph, GLsizei numGlyphs, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale );
        void       (WINE_GLAPI *p_glPathGlyphsNV)( GLuint firstPathName, GLenum fontTarget, const void *fontName, GLbitfield fontStyle, GLsizei numGlyphs, GLenum type, const void *charcodes, GLenum handleMissingGlyphs, GLuint pathParameterTemplate, GLfloat emScale );
        GLenum     (WINE_GLAPI *p_glPathMemoryGlyphIndexArrayNV)( GLuint firstPathName, GLenum fontTarget, GLsizeiptr fontSize, const void *fontData, GLsizei faceIndex, GLuint firstGlyphIndex, GLsizei numGlyphs, GLuint pathParameterTemplate, GLfloat emScale );
        void       (WINE_GLAPI *p_glPathParameterfNV)( GLuint path, GLenum pname, GLfloat value );
        void       (WINE_GLAPI *p_glPathParameterfvNV)( GLuint path, GLenum pname, const GLfloat *value );
        void       (WINE_GLAPI *p_glPathParameteriNV)( GLuint path, GLenum pname, GLint value );
        void       (WINE_GLAPI *p_glPathParameterivNV)( GLuint path, GLenum pname, const GLint *value );
        void       (WINE_GLAPI *p_glPathStencilDepthOffsetNV)( GLfloat factor, GLfloat units );
        void       (WINE_GLAPI *p_glPathStencilFuncNV)( GLenum func, GLint ref, GLuint mask );
        void       (WINE_GLAPI *p_glPathStringNV)( GLuint path, GLenum format, GLsizei length, const void *pathString );
        void       (WINE_GLAPI *p_glPathSubCommandsNV)( GLuint path, GLsizei commandStart, GLsizei commandsToDelete, GLsizei numCommands, const GLubyte *commands, GLsizei numCoords, GLenum coordType, const void *coords );
        void       (WINE_GLAPI *p_glPathSubCoordsNV)( GLuint path, GLsizei coordStart, GLsizei numCoords, GLenum coordType, const void *coords );
        void       (WINE_GLAPI *p_glPathTexGenNV)( GLenum texCoordSet, GLenum genMode, GLint components, const GLfloat *coeffs );
        void       (WINE_GLAPI *p_glPauseTransformFeedback)(void);
        void       (WINE_GLAPI *p_glPauseTransformFeedbackNV)(void);
        void       (WINE_GLAPI *p_glPixelDataRangeNV)( GLenum target, GLsizei length, const void *pointer );
        void       (WINE_GLAPI *p_glPixelMapx)( GLenum map, GLint size, const GLfixed *values );
        void       (WINE_GLAPI *p_glPixelStorex)( GLenum pname, GLfixed param );
        void       (WINE_GLAPI *p_glPixelTexGenParameterfSGIS)( GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glPixelTexGenParameterfvSGIS)( GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glPixelTexGenParameteriSGIS)( GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glPixelTexGenParameterivSGIS)( GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glPixelTexGenSGIX)( GLenum mode );
        void       (WINE_GLAPI *p_glPixelTransferxOES)( GLenum pname, GLfixed param );
        void       (WINE_GLAPI *p_glPixelTransformParameterfEXT)( GLenum target, GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glPixelTransformParameterfvEXT)( GLenum target, GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glPixelTransformParameteriEXT)( GLenum target, GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glPixelTransformParameterivEXT)( GLenum target, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glPixelZoomxOES)( GLfixed xfactor, GLfixed yfactor );
        GLboolean  (WINE_GLAPI *p_glPointAlongPathNV)( GLuint path, GLsizei startSegment, GLsizei numSegments, GLfloat distance, GLfloat *x, GLfloat *y, GLfloat *tangentX, GLfloat *tangentY );
        void       (WINE_GLAPI *p_glPointParameterf)( GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glPointParameterfARB)( GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glPointParameterfEXT)( GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glPointParameterfSGIS)( GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glPointParameterfv)( GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glPointParameterfvARB)( GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glPointParameterfvEXT)( GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glPointParameterfvSGIS)( GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glPointParameteri)( GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glPointParameteriNV)( GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glPointParameteriv)( GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glPointParameterivNV)( GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glPointParameterxvOES)( GLenum pname, const GLfixed *params );
        void       (WINE_GLAPI *p_glPointSizexOES)( GLfixed size );
        GLint      (WINE_GLAPI *p_glPollAsyncSGIX)( GLuint *markerp );
        GLint      (WINE_GLAPI *p_glPollInstrumentsSGIX)( GLint *marker_p );
        void       (WINE_GLAPI *p_glPolygonOffsetClamp)( GLfloat factor, GLfloat units, GLfloat clamp );
        void       (WINE_GLAPI *p_glPolygonOffsetClampEXT)( GLfloat factor, GLfloat units, GLfloat clamp );
        void       (WINE_GLAPI *p_glPolygonOffsetEXT)( GLfloat factor, GLfloat bias );
        void       (WINE_GLAPI *p_glPolygonOffsetxOES)( GLfixed factor, GLfixed units );
        void       (WINE_GLAPI *p_glPopDebugGroup)(void);
        void       (WINE_GLAPI *p_glPopGroupMarkerEXT)(void);
        void       (WINE_GLAPI *p_glPresentFrameDualFillNV)( GLuint video_slot, GLuint64EXT minPresentTime, GLuint beginPresentTimeId, GLuint presentDurationId, GLenum type, GLenum target0, GLuint fill0, GLenum target1, GLuint fill1, GLenum target2, GLuint fill2, GLenum target3, GLuint fill3 );
        void       (WINE_GLAPI *p_glPresentFrameKeyedNV)( GLuint video_slot, GLuint64EXT minPresentTime, GLuint beginPresentTimeId, GLuint presentDurationId, GLenum type, GLenum target0, GLuint fill0, GLuint key0, GLenum target1, GLuint fill1, GLuint key1 );
        void       (WINE_GLAPI *p_glPrimitiveBoundingBoxARB)( GLfloat minX, GLfloat minY, GLfloat minZ, GLfloat minW, GLfloat maxX, GLfloat maxY, GLfloat maxZ, GLfloat maxW );
        void       (WINE_GLAPI *p_glPrimitiveRestartIndex)( GLuint index );
        void       (WINE_GLAPI *p_glPrimitiveRestartIndexNV)( GLuint index );
        void       (WINE_GLAPI *p_glPrimitiveRestartNV)(void);
        void       (WINE_GLAPI *p_glPrioritizeTexturesEXT)( GLsizei n, const GLuint *textures, const GLclampf *priorities );
        void       (WINE_GLAPI *p_glPrioritizeTexturesxOES)( GLsizei n, const GLuint *textures, const GLfixed *priorities );
        void       (WINE_GLAPI *p_glProgramBinary)( GLuint program, GLenum binaryFormat, const void *binary, GLsizei length );
        void       (WINE_GLAPI *p_glProgramBufferParametersIivNV)( GLenum target, GLuint bindingIndex, GLuint wordIndex, GLsizei count, const GLint *params );
        void       (WINE_GLAPI *p_glProgramBufferParametersIuivNV)( GLenum target, GLuint bindingIndex, GLuint wordIndex, GLsizei count, const GLuint *params );
        void       (WINE_GLAPI *p_glProgramBufferParametersfvNV)( GLenum target, GLuint bindingIndex, GLuint wordIndex, GLsizei count, const GLfloat *params );
        void       (WINE_GLAPI *p_glProgramEnvParameter4dARB)( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
        void       (WINE_GLAPI *p_glProgramEnvParameter4dvARB)( GLenum target, GLuint index, const GLdouble *params );
        void       (WINE_GLAPI *p_glProgramEnvParameter4fARB)( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
        void       (WINE_GLAPI *p_glProgramEnvParameter4fvARB)( GLenum target, GLuint index, const GLfloat *params );
        void       (WINE_GLAPI *p_glProgramEnvParameterI4iNV)( GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w );
        void       (WINE_GLAPI *p_glProgramEnvParameterI4ivNV)( GLenum target, GLuint index, const GLint *params );
        void       (WINE_GLAPI *p_glProgramEnvParameterI4uiNV)( GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w );
        void       (WINE_GLAPI *p_glProgramEnvParameterI4uivNV)( GLenum target, GLuint index, const GLuint *params );
        void       (WINE_GLAPI *p_glProgramEnvParameters4fvEXT)( GLenum target, GLuint index, GLsizei count, const GLfloat *params );
        void       (WINE_GLAPI *p_glProgramEnvParametersI4ivNV)( GLenum target, GLuint index, GLsizei count, const GLint *params );
        void       (WINE_GLAPI *p_glProgramEnvParametersI4uivNV)( GLenum target, GLuint index, GLsizei count, const GLuint *params );
        void       (WINE_GLAPI *p_glProgramLocalParameter4dARB)( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
        void       (WINE_GLAPI *p_glProgramLocalParameter4dvARB)( GLenum target, GLuint index, const GLdouble *params );
        void       (WINE_GLAPI *p_glProgramLocalParameter4fARB)( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
        void       (WINE_GLAPI *p_glProgramLocalParameter4fvARB)( GLenum target, GLuint index, const GLfloat *params );
        void       (WINE_GLAPI *p_glProgramLocalParameterI4iNV)( GLenum target, GLuint index, GLint x, GLint y, GLint z, GLint w );
        void       (WINE_GLAPI *p_glProgramLocalParameterI4ivNV)( GLenum target, GLuint index, const GLint *params );
        void       (WINE_GLAPI *p_glProgramLocalParameterI4uiNV)( GLenum target, GLuint index, GLuint x, GLuint y, GLuint z, GLuint w );
        void       (WINE_GLAPI *p_glProgramLocalParameterI4uivNV)( GLenum target, GLuint index, const GLuint *params );
        void       (WINE_GLAPI *p_glProgramLocalParameters4fvEXT)( GLenum target, GLuint index, GLsizei count, const GLfloat *params );
        void       (WINE_GLAPI *p_glProgramLocalParametersI4ivNV)( GLenum target, GLuint index, GLsizei count, const GLint *params );
        void       (WINE_GLAPI *p_glProgramLocalParametersI4uivNV)( GLenum target, GLuint index, GLsizei count, const GLuint *params );
        void       (WINE_GLAPI *p_glProgramNamedParameter4dNV)( GLuint id, GLsizei len, const GLubyte *name, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
        void       (WINE_GLAPI *p_glProgramNamedParameter4dvNV)( GLuint id, GLsizei len, const GLubyte *name, const GLdouble *v );
        void       (WINE_GLAPI *p_glProgramNamedParameter4fNV)( GLuint id, GLsizei len, const GLubyte *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
        void       (WINE_GLAPI *p_glProgramNamedParameter4fvNV)( GLuint id, GLsizei len, const GLubyte *name, const GLfloat *v );
        void       (WINE_GLAPI *p_glProgramParameter4dNV)( GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
        void       (WINE_GLAPI *p_glProgramParameter4dvNV)( GLenum target, GLuint index, const GLdouble *v );
        void       (WINE_GLAPI *p_glProgramParameter4fNV)( GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
        void       (WINE_GLAPI *p_glProgramParameter4fvNV)( GLenum target, GLuint index, const GLfloat *v );
        void       (WINE_GLAPI *p_glProgramParameteri)( GLuint program, GLenum pname, GLint value );
        void       (WINE_GLAPI *p_glProgramParameteriARB)( GLuint program, GLenum pname, GLint value );
        void       (WINE_GLAPI *p_glProgramParameteriEXT)( GLuint program, GLenum pname, GLint value );
        void       (WINE_GLAPI *p_glProgramParameters4dvNV)( GLenum target, GLuint index, GLsizei count, const GLdouble *v );
        void       (WINE_GLAPI *p_glProgramParameters4fvNV)( GLenum target, GLuint index, GLsizei count, const GLfloat *v );
        void       (WINE_GLAPI *p_glProgramPathFragmentInputGenNV)( GLuint program, GLint location, GLenum genMode, GLint components, const GLfloat *coeffs );
        void       (WINE_GLAPI *p_glProgramStringARB)( GLenum target, GLenum format, GLsizei len, const void *string );
        void       (WINE_GLAPI *p_glProgramSubroutineParametersuivNV)( GLenum target, GLsizei count, const GLuint *params );
        void       (WINE_GLAPI *p_glProgramUniform1d)( GLuint program, GLint location, GLdouble v0 );
        void       (WINE_GLAPI *p_glProgramUniform1dEXT)( GLuint program, GLint location, GLdouble x );
        void       (WINE_GLAPI *p_glProgramUniform1dv)( GLuint program, GLint location, GLsizei count, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniform1dvEXT)( GLuint program, GLint location, GLsizei count, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniform1f)( GLuint program, GLint location, GLfloat v0 );
        void       (WINE_GLAPI *p_glProgramUniform1fEXT)( GLuint program, GLint location, GLfloat v0 );
        void       (WINE_GLAPI *p_glProgramUniform1fv)( GLuint program, GLint location, GLsizei count, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniform1fvEXT)( GLuint program, GLint location, GLsizei count, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniform1i)( GLuint program, GLint location, GLint v0 );
        void       (WINE_GLAPI *p_glProgramUniform1i64ARB)( GLuint program, GLint location, GLint64 x );
        void       (WINE_GLAPI *p_glProgramUniform1i64NV)( GLuint program, GLint location, GLint64EXT x );
        void       (WINE_GLAPI *p_glProgramUniform1i64vARB)( GLuint program, GLint location, GLsizei count, const GLint64 *value );
        void       (WINE_GLAPI *p_glProgramUniform1i64vNV)( GLuint program, GLint location, GLsizei count, const GLint64EXT *value );
        void       (WINE_GLAPI *p_glProgramUniform1iEXT)( GLuint program, GLint location, GLint v0 );
        void       (WINE_GLAPI *p_glProgramUniform1iv)( GLuint program, GLint location, GLsizei count, const GLint *value );
        void       (WINE_GLAPI *p_glProgramUniform1ivEXT)( GLuint program, GLint location, GLsizei count, const GLint *value );
        void       (WINE_GLAPI *p_glProgramUniform1ui)( GLuint program, GLint location, GLuint v0 );
        void       (WINE_GLAPI *p_glProgramUniform1ui64ARB)( GLuint program, GLint location, GLuint64 x );
        void       (WINE_GLAPI *p_glProgramUniform1ui64NV)( GLuint program, GLint location, GLuint64EXT x );
        void       (WINE_GLAPI *p_glProgramUniform1ui64vARB)( GLuint program, GLint location, GLsizei count, const GLuint64 *value );
        void       (WINE_GLAPI *p_glProgramUniform1ui64vNV)( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value );
        void       (WINE_GLAPI *p_glProgramUniform1uiEXT)( GLuint program, GLint location, GLuint v0 );
        void       (WINE_GLAPI *p_glProgramUniform1uiv)( GLuint program, GLint location, GLsizei count, const GLuint *value );
        void       (WINE_GLAPI *p_glProgramUniform1uivEXT)( GLuint program, GLint location, GLsizei count, const GLuint *value );
        void       (WINE_GLAPI *p_glProgramUniform2d)( GLuint program, GLint location, GLdouble v0, GLdouble v1 );
        void       (WINE_GLAPI *p_glProgramUniform2dEXT)( GLuint program, GLint location, GLdouble x, GLdouble y );
        void       (WINE_GLAPI *p_glProgramUniform2dv)( GLuint program, GLint location, GLsizei count, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniform2dvEXT)( GLuint program, GLint location, GLsizei count, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniform2f)( GLuint program, GLint location, GLfloat v0, GLfloat v1 );
        void       (WINE_GLAPI *p_glProgramUniform2fEXT)( GLuint program, GLint location, GLfloat v0, GLfloat v1 );
        void       (WINE_GLAPI *p_glProgramUniform2fv)( GLuint program, GLint location, GLsizei count, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniform2fvEXT)( GLuint program, GLint location, GLsizei count, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniform2i)( GLuint program, GLint location, GLint v0, GLint v1 );
        void       (WINE_GLAPI *p_glProgramUniform2i64ARB)( GLuint program, GLint location, GLint64 x, GLint64 y );
        void       (WINE_GLAPI *p_glProgramUniform2i64NV)( GLuint program, GLint location, GLint64EXT x, GLint64EXT y );
        void       (WINE_GLAPI *p_glProgramUniform2i64vARB)( GLuint program, GLint location, GLsizei count, const GLint64 *value );
        void       (WINE_GLAPI *p_glProgramUniform2i64vNV)( GLuint program, GLint location, GLsizei count, const GLint64EXT *value );
        void       (WINE_GLAPI *p_glProgramUniform2iEXT)( GLuint program, GLint location, GLint v0, GLint v1 );
        void       (WINE_GLAPI *p_glProgramUniform2iv)( GLuint program, GLint location, GLsizei count, const GLint *value );
        void       (WINE_GLAPI *p_glProgramUniform2ivEXT)( GLuint program, GLint location, GLsizei count, const GLint *value );
        void       (WINE_GLAPI *p_glProgramUniform2ui)( GLuint program, GLint location, GLuint v0, GLuint v1 );
        void       (WINE_GLAPI *p_glProgramUniform2ui64ARB)( GLuint program, GLint location, GLuint64 x, GLuint64 y );
        void       (WINE_GLAPI *p_glProgramUniform2ui64NV)( GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y );
        void       (WINE_GLAPI *p_glProgramUniform2ui64vARB)( GLuint program, GLint location, GLsizei count, const GLuint64 *value );
        void       (WINE_GLAPI *p_glProgramUniform2ui64vNV)( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value );
        void       (WINE_GLAPI *p_glProgramUniform2uiEXT)( GLuint program, GLint location, GLuint v0, GLuint v1 );
        void       (WINE_GLAPI *p_glProgramUniform2uiv)( GLuint program, GLint location, GLsizei count, const GLuint *value );
        void       (WINE_GLAPI *p_glProgramUniform2uivEXT)( GLuint program, GLint location, GLsizei count, const GLuint *value );
        void       (WINE_GLAPI *p_glProgramUniform3d)( GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2 );
        void       (WINE_GLAPI *p_glProgramUniform3dEXT)( GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z );
        void       (WINE_GLAPI *p_glProgramUniform3dv)( GLuint program, GLint location, GLsizei count, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniform3dvEXT)( GLuint program, GLint location, GLsizei count, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniform3f)( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2 );
        void       (WINE_GLAPI *p_glProgramUniform3fEXT)( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2 );
        void       (WINE_GLAPI *p_glProgramUniform3fv)( GLuint program, GLint location, GLsizei count, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniform3fvEXT)( GLuint program, GLint location, GLsizei count, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniform3i)( GLuint program, GLint location, GLint v0, GLint v1, GLint v2 );
        void       (WINE_GLAPI *p_glProgramUniform3i64ARB)( GLuint program, GLint location, GLint64 x, GLint64 y, GLint64 z );
        void       (WINE_GLAPI *p_glProgramUniform3i64NV)( GLuint program, GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z );
        void       (WINE_GLAPI *p_glProgramUniform3i64vARB)( GLuint program, GLint location, GLsizei count, const GLint64 *value );
        void       (WINE_GLAPI *p_glProgramUniform3i64vNV)( GLuint program, GLint location, GLsizei count, const GLint64EXT *value );
        void       (WINE_GLAPI *p_glProgramUniform3iEXT)( GLuint program, GLint location, GLint v0, GLint v1, GLint v2 );
        void       (WINE_GLAPI *p_glProgramUniform3iv)( GLuint program, GLint location, GLsizei count, const GLint *value );
        void       (WINE_GLAPI *p_glProgramUniform3ivEXT)( GLuint program, GLint location, GLsizei count, const GLint *value );
        void       (WINE_GLAPI *p_glProgramUniform3ui)( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2 );
        void       (WINE_GLAPI *p_glProgramUniform3ui64ARB)( GLuint program, GLint location, GLuint64 x, GLuint64 y, GLuint64 z );
        void       (WINE_GLAPI *p_glProgramUniform3ui64NV)( GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z );
        void       (WINE_GLAPI *p_glProgramUniform3ui64vARB)( GLuint program, GLint location, GLsizei count, const GLuint64 *value );
        void       (WINE_GLAPI *p_glProgramUniform3ui64vNV)( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value );
        void       (WINE_GLAPI *p_glProgramUniform3uiEXT)( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2 );
        void       (WINE_GLAPI *p_glProgramUniform3uiv)( GLuint program, GLint location, GLsizei count, const GLuint *value );
        void       (WINE_GLAPI *p_glProgramUniform3uivEXT)( GLuint program, GLint location, GLsizei count, const GLuint *value );
        void       (WINE_GLAPI *p_glProgramUniform4d)( GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3 );
        void       (WINE_GLAPI *p_glProgramUniform4dEXT)( GLuint program, GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
        void       (WINE_GLAPI *p_glProgramUniform4dv)( GLuint program, GLint location, GLsizei count, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniform4dvEXT)( GLuint program, GLint location, GLsizei count, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniform4f)( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 );
        void       (WINE_GLAPI *p_glProgramUniform4fEXT)( GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 );
        void       (WINE_GLAPI *p_glProgramUniform4fv)( GLuint program, GLint location, GLsizei count, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniform4fvEXT)( GLuint program, GLint location, GLsizei count, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniform4i)( GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3 );
        void       (WINE_GLAPI *p_glProgramUniform4i64ARB)( GLuint program, GLint location, GLint64 x, GLint64 y, GLint64 z, GLint64 w );
        void       (WINE_GLAPI *p_glProgramUniform4i64NV)( GLuint program, GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w );
        void       (WINE_GLAPI *p_glProgramUniform4i64vARB)( GLuint program, GLint location, GLsizei count, const GLint64 *value );
        void       (WINE_GLAPI *p_glProgramUniform4i64vNV)( GLuint program, GLint location, GLsizei count, const GLint64EXT *value );
        void       (WINE_GLAPI *p_glProgramUniform4iEXT)( GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3 );
        void       (WINE_GLAPI *p_glProgramUniform4iv)( GLuint program, GLint location, GLsizei count, const GLint *value );
        void       (WINE_GLAPI *p_glProgramUniform4ivEXT)( GLuint program, GLint location, GLsizei count, const GLint *value );
        void       (WINE_GLAPI *p_glProgramUniform4ui)( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 );
        void       (WINE_GLAPI *p_glProgramUniform4ui64ARB)( GLuint program, GLint location, GLuint64 x, GLuint64 y, GLuint64 z, GLuint64 w );
        void       (WINE_GLAPI *p_glProgramUniform4ui64NV)( GLuint program, GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w );
        void       (WINE_GLAPI *p_glProgramUniform4ui64vARB)( GLuint program, GLint location, GLsizei count, const GLuint64 *value );
        void       (WINE_GLAPI *p_glProgramUniform4ui64vNV)( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value );
        void       (WINE_GLAPI *p_glProgramUniform4uiEXT)( GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 );
        void       (WINE_GLAPI *p_glProgramUniform4uiv)( GLuint program, GLint location, GLsizei count, const GLuint *value );
        void       (WINE_GLAPI *p_glProgramUniform4uivEXT)( GLuint program, GLint location, GLsizei count, const GLuint *value );
        void       (WINE_GLAPI *p_glProgramUniformHandleui64ARB)( GLuint program, GLint location, GLuint64 value );
        void       (WINE_GLAPI *p_glProgramUniformHandleui64NV)( GLuint program, GLint location, GLuint64 value );
        void       (WINE_GLAPI *p_glProgramUniformHandleui64vARB)( GLuint program, GLint location, GLsizei count, const GLuint64 *values );
        void       (WINE_GLAPI *p_glProgramUniformHandleui64vNV)( GLuint program, GLint location, GLsizei count, const GLuint64 *values );
        void       (WINE_GLAPI *p_glProgramUniformMatrix2dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix2dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix2fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix2fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix2x3dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix2x3dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix2x3fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix2x3fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix2x4dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix2x4dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix2x4fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix2x4fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix3dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix3dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix3fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix3fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix3x2dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix3x2dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix3x2fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix3x2fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix3x4dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix3x4dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix3x4fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix3x4fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix4dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix4dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix4fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix4fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix4x2dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix4x2dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix4x2fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix4x2fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix4x3dv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix4x3dvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix4x3fv)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniformMatrix4x3fvEXT)( GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glProgramUniformui64NV)( GLuint program, GLint location, GLuint64EXT value );
        void       (WINE_GLAPI *p_glProgramUniformui64vNV)( GLuint program, GLint location, GLsizei count, const GLuint64EXT *value );
        void       (WINE_GLAPI *p_glProgramVertexLimitNV)( GLenum target, GLint limit );
        void       (WINE_GLAPI *p_glProvokingVertex)( GLenum mode );
        void       (WINE_GLAPI *p_glProvokingVertexEXT)( GLenum mode );
        void       (WINE_GLAPI *p_glPushClientAttribDefaultEXT)( GLbitfield mask );
        void       (WINE_GLAPI *p_glPushDebugGroup)( GLenum source, GLuint id, GLsizei length, const GLchar *message );
        void       (WINE_GLAPI *p_glPushGroupMarkerEXT)( GLsizei length, const GLchar *marker );
        void       (WINE_GLAPI *p_glQueryCounter)( GLuint id, GLenum target );
        GLbitfield (WINE_GLAPI *p_glQueryMatrixxOES)( GLfixed *mantissa, GLint *exponent );
        void       (WINE_GLAPI *p_glQueryObjectParameteruiAMD)( GLenum target, GLuint id, GLenum pname, GLuint param );
        GLint      (WINE_GLAPI *p_glQueryResourceNV)( GLenum queryType, GLint tagId, GLuint count, GLint *buffer );
        void       (WINE_GLAPI *p_glQueryResourceTagNV)( GLint tagId, const GLchar *tagString );
        void       (WINE_GLAPI *p_glRasterPos2xOES)( GLfixed x, GLfixed y );
        void       (WINE_GLAPI *p_glRasterPos2xvOES)( const GLfixed *coords );
        void       (WINE_GLAPI *p_glRasterPos3xOES)( GLfixed x, GLfixed y, GLfixed z );
        void       (WINE_GLAPI *p_glRasterPos3xvOES)( const GLfixed *coords );
        void       (WINE_GLAPI *p_glRasterPos4xOES)( GLfixed x, GLfixed y, GLfixed z, GLfixed w );
        void       (WINE_GLAPI *p_glRasterPos4xvOES)( const GLfixed *coords );
        void       (WINE_GLAPI *p_glRasterSamplesEXT)( GLuint samples, GLboolean fixedsamplelocations );
        void       (WINE_GLAPI *p_glReadBufferRegion)( GLenum region, GLint x, GLint y, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glReadInstrumentsSGIX)( GLint marker );
        void       (WINE_GLAPI *p_glReadnPixels)( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data );
        void       (WINE_GLAPI *p_glReadnPixelsARB)( GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data );
        void       (WINE_GLAPI *p_glRectxOES)( GLfixed x1, GLfixed y1, GLfixed x2, GLfixed y2 );
        void       (WINE_GLAPI *p_glRectxvOES)( const GLfixed *v1, const GLfixed *v2 );
        void       (WINE_GLAPI *p_glReferencePlaneSGIX)( const GLdouble *equation );
        GLboolean  (WINE_GLAPI *p_glReleaseKeyedMutexWin32EXT)( GLuint memory, GLuint64 key );
        void       (WINE_GLAPI *p_glReleaseShaderCompiler)(void);
        void       (WINE_GLAPI *p_glRenderGpuMaskNV)( GLbitfield mask );
        void       (WINE_GLAPI *p_glRenderbufferStorage)( GLenum target, GLenum internalformat, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glRenderbufferStorageEXT)( GLenum target, GLenum internalformat, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glRenderbufferStorageMultisample)( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glRenderbufferStorageMultisampleAdvancedAMD)( GLenum target, GLsizei samples, GLsizei storageSamples, GLenum internalformat, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glRenderbufferStorageMultisampleCoverageNV)( GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLenum internalformat, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glRenderbufferStorageMultisampleEXT)( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glReplacementCodePointerSUN)( GLenum type, GLsizei stride, const void **pointer );
        void       (WINE_GLAPI *p_glReplacementCodeubSUN)( GLubyte code );
        void       (WINE_GLAPI *p_glReplacementCodeubvSUN)( const GLubyte *code );
        void       (WINE_GLAPI *p_glReplacementCodeuiColor3fVertex3fSUN)( GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glReplacementCodeuiColor3fVertex3fvSUN)( const GLuint *rc, const GLfloat *c, const GLfloat *v );
        void       (WINE_GLAPI *p_glReplacementCodeuiColor4fNormal3fVertex3fSUN)( GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glReplacementCodeuiColor4fNormal3fVertex3fvSUN)( const GLuint *rc, const GLfloat *c, const GLfloat *n, const GLfloat *v );
        void       (WINE_GLAPI *p_glReplacementCodeuiColor4ubVertex3fSUN)( GLuint rc, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glReplacementCodeuiColor4ubVertex3fvSUN)( const GLuint *rc, const GLubyte *c, const GLfloat *v );
        void       (WINE_GLAPI *p_glReplacementCodeuiNormal3fVertex3fSUN)( GLuint rc, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glReplacementCodeuiNormal3fVertex3fvSUN)( const GLuint *rc, const GLfloat *n, const GLfloat *v );
        void       (WINE_GLAPI *p_glReplacementCodeuiSUN)( GLuint code );
        void       (WINE_GLAPI *p_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN)( GLuint rc, GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN)( const GLuint *rc, const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v );
        void       (WINE_GLAPI *p_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN)( GLuint rc, GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN)( const GLuint *rc, const GLfloat *tc, const GLfloat *n, const GLfloat *v );
        void       (WINE_GLAPI *p_glReplacementCodeuiTexCoord2fVertex3fSUN)( GLuint rc, GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glReplacementCodeuiTexCoord2fVertex3fvSUN)( const GLuint *rc, const GLfloat *tc, const GLfloat *v );
        void       (WINE_GLAPI *p_glReplacementCodeuiVertex3fSUN)( GLuint rc, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glReplacementCodeuiVertex3fvSUN)( const GLuint *rc, const GLfloat *v );
        void       (WINE_GLAPI *p_glReplacementCodeuivSUN)( const GLuint *code );
        void       (WINE_GLAPI *p_glReplacementCodeusSUN)( GLushort code );
        void       (WINE_GLAPI *p_glReplacementCodeusvSUN)( const GLushort *code );
        void       (WINE_GLAPI *p_glRequestResidentProgramsNV)( GLsizei n, const GLuint *programs );
        void       (WINE_GLAPI *p_glResetHistogram)( GLenum target );
        void       (WINE_GLAPI *p_glResetHistogramEXT)( GLenum target );
        void       (WINE_GLAPI *p_glResetMemoryObjectParameterNV)( GLuint memory, GLenum pname );
        void       (WINE_GLAPI *p_glResetMinmax)( GLenum target );
        void       (WINE_GLAPI *p_glResetMinmaxEXT)( GLenum target );
        void       (WINE_GLAPI *p_glResizeBuffersMESA)(void);
        void       (WINE_GLAPI *p_glResolveDepthValuesNV)(void);
        void       (WINE_GLAPI *p_glResumeTransformFeedback)(void);
        void       (WINE_GLAPI *p_glResumeTransformFeedbackNV)(void);
        void       (WINE_GLAPI *p_glRotatexOES)( GLfixed angle, GLfixed x, GLfixed y, GLfixed z );
        void       (WINE_GLAPI *p_glSampleCoverage)( GLfloat value, GLboolean invert );
        void       (WINE_GLAPI *p_glSampleCoverageARB)( GLfloat value, GLboolean invert );
        void       (WINE_GLAPI *p_glSampleMapATI)( GLuint dst, GLuint interp, GLenum swizzle );
        void       (WINE_GLAPI *p_glSampleMaskEXT)( GLclampf value, GLboolean invert );
        void       (WINE_GLAPI *p_glSampleMaskIndexedNV)( GLuint index, GLbitfield mask );
        void       (WINE_GLAPI *p_glSampleMaskSGIS)( GLclampf value, GLboolean invert );
        void       (WINE_GLAPI *p_glSampleMaski)( GLuint maskNumber, GLbitfield mask );
        void       (WINE_GLAPI *p_glSamplePatternEXT)( GLenum pattern );
        void       (WINE_GLAPI *p_glSamplePatternSGIS)( GLenum pattern );
        void       (WINE_GLAPI *p_glSamplerParameterIiv)( GLuint sampler, GLenum pname, const GLint *param );
        void       (WINE_GLAPI *p_glSamplerParameterIuiv)( GLuint sampler, GLenum pname, const GLuint *param );
        void       (WINE_GLAPI *p_glSamplerParameterf)( GLuint sampler, GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glSamplerParameterfv)( GLuint sampler, GLenum pname, const GLfloat *param );
        void       (WINE_GLAPI *p_glSamplerParameteri)( GLuint sampler, GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glSamplerParameteriv)( GLuint sampler, GLenum pname, const GLint *param );
        void       (WINE_GLAPI *p_glScalexOES)( GLfixed x, GLfixed y, GLfixed z );
        void       (WINE_GLAPI *p_glScissorArrayv)( GLuint first, GLsizei count, const GLint *v );
        void       (WINE_GLAPI *p_glScissorExclusiveArrayvNV)( GLuint first, GLsizei count, const GLint *v );
        void       (WINE_GLAPI *p_glScissorExclusiveNV)( GLint x, GLint y, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glScissorIndexed)( GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glScissorIndexedv)( GLuint index, const GLint *v );
        void       (WINE_GLAPI *p_glSecondaryColor3b)( GLbyte red, GLbyte green, GLbyte blue );
        void       (WINE_GLAPI *p_glSecondaryColor3bEXT)( GLbyte red, GLbyte green, GLbyte blue );
        void       (WINE_GLAPI *p_glSecondaryColor3bv)( const GLbyte *v );
        void       (WINE_GLAPI *p_glSecondaryColor3bvEXT)( const GLbyte *v );
        void       (WINE_GLAPI *p_glSecondaryColor3d)( GLdouble red, GLdouble green, GLdouble blue );
        void       (WINE_GLAPI *p_glSecondaryColor3dEXT)( GLdouble red, GLdouble green, GLdouble blue );
        void       (WINE_GLAPI *p_glSecondaryColor3dv)( const GLdouble *v );
        void       (WINE_GLAPI *p_glSecondaryColor3dvEXT)( const GLdouble *v );
        void       (WINE_GLAPI *p_glSecondaryColor3f)( GLfloat red, GLfloat green, GLfloat blue );
        void       (WINE_GLAPI *p_glSecondaryColor3fEXT)( GLfloat red, GLfloat green, GLfloat blue );
        void       (WINE_GLAPI *p_glSecondaryColor3fv)( const GLfloat *v );
        void       (WINE_GLAPI *p_glSecondaryColor3fvEXT)( const GLfloat *v );
        void       (WINE_GLAPI *p_glSecondaryColor3hNV)( GLhalfNV red, GLhalfNV green, GLhalfNV blue );
        void       (WINE_GLAPI *p_glSecondaryColor3hvNV)( const GLhalfNV *v );
        void       (WINE_GLAPI *p_glSecondaryColor3i)( GLint red, GLint green, GLint blue );
        void       (WINE_GLAPI *p_glSecondaryColor3iEXT)( GLint red, GLint green, GLint blue );
        void       (WINE_GLAPI *p_glSecondaryColor3iv)( const GLint *v );
        void       (WINE_GLAPI *p_glSecondaryColor3ivEXT)( const GLint *v );
        void       (WINE_GLAPI *p_glSecondaryColor3s)( GLshort red, GLshort green, GLshort blue );
        void       (WINE_GLAPI *p_glSecondaryColor3sEXT)( GLshort red, GLshort green, GLshort blue );
        void       (WINE_GLAPI *p_glSecondaryColor3sv)( const GLshort *v );
        void       (WINE_GLAPI *p_glSecondaryColor3svEXT)( const GLshort *v );
        void       (WINE_GLAPI *p_glSecondaryColor3ub)( GLubyte red, GLubyte green, GLubyte blue );
        void       (WINE_GLAPI *p_glSecondaryColor3ubEXT)( GLubyte red, GLubyte green, GLubyte blue );
        void       (WINE_GLAPI *p_glSecondaryColor3ubv)( const GLubyte *v );
        void       (WINE_GLAPI *p_glSecondaryColor3ubvEXT)( const GLubyte *v );
        void       (WINE_GLAPI *p_glSecondaryColor3ui)( GLuint red, GLuint green, GLuint blue );
        void       (WINE_GLAPI *p_glSecondaryColor3uiEXT)( GLuint red, GLuint green, GLuint blue );
        void       (WINE_GLAPI *p_glSecondaryColor3uiv)( const GLuint *v );
        void       (WINE_GLAPI *p_glSecondaryColor3uivEXT)( const GLuint *v );
        void       (WINE_GLAPI *p_glSecondaryColor3us)( GLushort red, GLushort green, GLushort blue );
        void       (WINE_GLAPI *p_glSecondaryColor3usEXT)( GLushort red, GLushort green, GLushort blue );
        void       (WINE_GLAPI *p_glSecondaryColor3usv)( const GLushort *v );
        void       (WINE_GLAPI *p_glSecondaryColor3usvEXT)( const GLushort *v );
        void       (WINE_GLAPI *p_glSecondaryColorFormatNV)( GLint size, GLenum type, GLsizei stride );
        void       (WINE_GLAPI *p_glSecondaryColorP3ui)( GLenum type, GLuint color );
        void       (WINE_GLAPI *p_glSecondaryColorP3uiv)( GLenum type, const GLuint *color );
        void       (WINE_GLAPI *p_glSecondaryColorPointer)( GLint size, GLenum type, GLsizei stride, const void *pointer );
        void       (WINE_GLAPI *p_glSecondaryColorPointerEXT)( GLint size, GLenum type, GLsizei stride, const void *pointer );
        void       (WINE_GLAPI *p_glSecondaryColorPointerListIBM)( GLint size, GLenum type, GLint stride, const void **pointer, GLint ptrstride );
        void       (WINE_GLAPI *p_glSelectPerfMonitorCountersAMD)( GLuint monitor, GLboolean enable, GLuint group, GLint numCounters, GLuint *counterList );
        void       (WINE_GLAPI *p_glSelectTextureCoordSetSGIS)( GLenum target );
        void       (WINE_GLAPI *p_glSelectTextureSGIS)( GLenum target );
        void       (WINE_GLAPI *p_glSemaphoreParameterui64vEXT)( GLuint semaphore, GLenum pname, const GLuint64 *params );
        void       (WINE_GLAPI *p_glSeparableFilter2D)( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *row, const void *column );
        void       (WINE_GLAPI *p_glSeparableFilter2DEXT)( GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *row, const void *column );
        void       (WINE_GLAPI *p_glSetFenceAPPLE)( GLuint fence );
        void       (WINE_GLAPI *p_glSetFenceNV)( GLuint fence, GLenum condition );
        void       (WINE_GLAPI *p_glSetFragmentShaderConstantATI)( GLuint dst, const GLfloat *value );
        void       (WINE_GLAPI *p_glSetInvariantEXT)( GLuint id, GLenum type, const void *addr );
        void       (WINE_GLAPI *p_glSetLocalConstantEXT)( GLuint id, GLenum type, const void *addr );
        void       (WINE_GLAPI *p_glSetMultisamplefvAMD)( GLenum pname, GLuint index, const GLfloat *val );
        void       (WINE_GLAPI *p_glShaderBinary)( GLsizei count, const GLuint *shaders, GLenum binaryformat, const void *binary, GLsizei length );
        void       (WINE_GLAPI *p_glShaderOp1EXT)( GLenum op, GLuint res, GLuint arg1 );
        void       (WINE_GLAPI *p_glShaderOp2EXT)( GLenum op, GLuint res, GLuint arg1, GLuint arg2 );
        void       (WINE_GLAPI *p_glShaderOp3EXT)( GLenum op, GLuint res, GLuint arg1, GLuint arg2, GLuint arg3 );
        void       (WINE_GLAPI *p_glShaderSource)( GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length );
        void       (WINE_GLAPI *p_glShaderSourceARB)( GLhandleARB shaderObj, GLsizei count, const GLcharARB **string, const GLint *length );
        void       (WINE_GLAPI *p_glShaderStorageBlockBinding)( GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding );
        void       (WINE_GLAPI *p_glShadingRateImageBarrierNV)( GLboolean synchronize );
        void       (WINE_GLAPI *p_glShadingRateImagePaletteNV)( GLuint viewport, GLuint first, GLsizei count, const GLenum *rates );
        void       (WINE_GLAPI *p_glShadingRateSampleOrderCustomNV)( GLenum rate, GLuint samples, const GLint *locations );
        void       (WINE_GLAPI *p_glShadingRateSampleOrderNV)( GLenum order );
        void       (WINE_GLAPI *p_glSharpenTexFuncSGIS)( GLenum target, GLsizei n, const GLfloat *points );
        void       (WINE_GLAPI *p_glSignalSemaphoreEXT)( GLuint semaphore, GLuint numBufferBarriers, const GLuint *buffers, GLuint numTextureBarriers, const GLuint *textures, const GLenum *dstLayouts );
        void       (WINE_GLAPI *p_glSignalSemaphoreui64NVX)( GLuint signalGpu, GLsizei fenceObjectCount, const GLuint *semaphoreArray, const GLuint64 *fenceValueArray );
        void       (WINE_GLAPI *p_glSignalVkFenceNV)( GLuint64 vkFence );
        void       (WINE_GLAPI *p_glSignalVkSemaphoreNV)( GLuint64 vkSemaphore );
        void       (WINE_GLAPI *p_glSpecializeShader)( GLuint shader, const GLchar *pEntryPoint, GLuint numSpecializationConstants, const GLuint *pConstantIndex, const GLuint *pConstantValue );
        void       (WINE_GLAPI *p_glSpecializeShaderARB)( GLuint shader, const GLchar *pEntryPoint, GLuint numSpecializationConstants, const GLuint *pConstantIndex, const GLuint *pConstantValue );
        void       (WINE_GLAPI *p_glSpriteParameterfSGIX)( GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glSpriteParameterfvSGIX)( GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glSpriteParameteriSGIX)( GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glSpriteParameterivSGIX)( GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glStartInstrumentsSGIX)(void);
        void       (WINE_GLAPI *p_glStateCaptureNV)( GLuint state, GLenum mode );
        void       (WINE_GLAPI *p_glStencilClearTagEXT)( GLsizei stencilTagBits, GLuint stencilClearTag );
        void       (WINE_GLAPI *p_glStencilFillPathInstancedNV)( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum transformType, const GLfloat *transformValues );
        void       (WINE_GLAPI *p_glStencilFillPathNV)( GLuint path, GLenum fillMode, GLuint mask );
        void       (WINE_GLAPI *p_glStencilFuncSeparate)( GLenum face, GLenum func, GLint ref, GLuint mask );
        void       (WINE_GLAPI *p_glStencilFuncSeparateATI)( GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask );
        void       (WINE_GLAPI *p_glStencilMaskSeparate)( GLenum face, GLuint mask );
        void       (WINE_GLAPI *p_glStencilOpSeparate)( GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass );
        void       (WINE_GLAPI *p_glStencilOpSeparateATI)( GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass );
        void       (WINE_GLAPI *p_glStencilOpValueAMD)( GLenum face, GLuint value );
        void       (WINE_GLAPI *p_glStencilStrokePathInstancedNV)( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLint reference, GLuint mask, GLenum transformType, const GLfloat *transformValues );
        void       (WINE_GLAPI *p_glStencilStrokePathNV)( GLuint path, GLint reference, GLuint mask );
        void       (WINE_GLAPI *p_glStencilThenCoverFillPathInstancedNV)( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLenum fillMode, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat *transformValues );
        void       (WINE_GLAPI *p_glStencilThenCoverFillPathNV)( GLuint path, GLenum fillMode, GLuint mask, GLenum coverMode );
        void       (WINE_GLAPI *p_glStencilThenCoverStrokePathInstancedNV)( GLsizei numPaths, GLenum pathNameType, const void *paths, GLuint pathBase, GLint reference, GLuint mask, GLenum coverMode, GLenum transformType, const GLfloat *transformValues );
        void       (WINE_GLAPI *p_glStencilThenCoverStrokePathNV)( GLuint path, GLint reference, GLuint mask, GLenum coverMode );
        void       (WINE_GLAPI *p_glStopInstrumentsSGIX)( GLint marker );
        void       (WINE_GLAPI *p_glStringMarkerGREMEDY)( GLsizei len, const void *string );
        void       (WINE_GLAPI *p_glSubpixelPrecisionBiasNV)( GLuint xbits, GLuint ybits );
        void       (WINE_GLAPI *p_glSwizzleEXT)( GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW );
        void       (WINE_GLAPI *p_glSyncTextureINTEL)( GLuint texture );
        void       (WINE_GLAPI *p_glTagSampleBufferSGIX)(void);
        void       (WINE_GLAPI *p_glTangent3bEXT)( GLbyte tx, GLbyte ty, GLbyte tz );
        void       (WINE_GLAPI *p_glTangent3bvEXT)( const GLbyte *v );
        void       (WINE_GLAPI *p_glTangent3dEXT)( GLdouble tx, GLdouble ty, GLdouble tz );
        void       (WINE_GLAPI *p_glTangent3dvEXT)( const GLdouble *v );
        void       (WINE_GLAPI *p_glTangent3fEXT)( GLfloat tx, GLfloat ty, GLfloat tz );
        void       (WINE_GLAPI *p_glTangent3fvEXT)( const GLfloat *v );
        void       (WINE_GLAPI *p_glTangent3iEXT)( GLint tx, GLint ty, GLint tz );
        void       (WINE_GLAPI *p_glTangent3ivEXT)( const GLint *v );
        void       (WINE_GLAPI *p_glTangent3sEXT)( GLshort tx, GLshort ty, GLshort tz );
        void       (WINE_GLAPI *p_glTangent3svEXT)( const GLshort *v );
        void       (WINE_GLAPI *p_glTangentPointerEXT)( GLenum type, GLsizei stride, const void *pointer );
        void       (WINE_GLAPI *p_glTbufferMask3DFX)( GLuint mask );
        void       (WINE_GLAPI *p_glTessellationFactorAMD)( GLfloat factor );
        void       (WINE_GLAPI *p_glTessellationModeAMD)( GLenum mode );
        GLboolean  (WINE_GLAPI *p_glTestFenceAPPLE)( GLuint fence );
        GLboolean  (WINE_GLAPI *p_glTestFenceNV)( GLuint fence );
        GLboolean  (WINE_GLAPI *p_glTestObjectAPPLE)( GLenum object, GLuint name );
        void       (WINE_GLAPI *p_glTexAttachMemoryNV)( GLenum target, GLuint memory, GLuint64 offset );
        void       (WINE_GLAPI *p_glTexBuffer)( GLenum target, GLenum internalformat, GLuint buffer );
        void       (WINE_GLAPI *p_glTexBufferARB)( GLenum target, GLenum internalformat, GLuint buffer );
        void       (WINE_GLAPI *p_glTexBufferEXT)( GLenum target, GLenum internalformat, GLuint buffer );
        void       (WINE_GLAPI *p_glTexBufferRange)( GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size );
        void       (WINE_GLAPI *p_glTexBumpParameterfvATI)( GLenum pname, const GLfloat *param );
        void       (WINE_GLAPI *p_glTexBumpParameterivATI)( GLenum pname, const GLint *param );
        void       (WINE_GLAPI *p_glTexCoord1bOES)( GLbyte s );
        void       (WINE_GLAPI *p_glTexCoord1bvOES)( const GLbyte *coords );
        void       (WINE_GLAPI *p_glTexCoord1hNV)( GLhalfNV s );
        void       (WINE_GLAPI *p_glTexCoord1hvNV)( const GLhalfNV *v );
        void       (WINE_GLAPI *p_glTexCoord1xOES)( GLfixed s );
        void       (WINE_GLAPI *p_glTexCoord1xvOES)( const GLfixed *coords );
        void       (WINE_GLAPI *p_glTexCoord2bOES)( GLbyte s, GLbyte t );
        void       (WINE_GLAPI *p_glTexCoord2bvOES)( const GLbyte *coords );
        void       (WINE_GLAPI *p_glTexCoord2fColor3fVertex3fSUN)( GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glTexCoord2fColor3fVertex3fvSUN)( const GLfloat *tc, const GLfloat *c, const GLfloat *v );
        void       (WINE_GLAPI *p_glTexCoord2fColor4fNormal3fVertex3fSUN)( GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glTexCoord2fColor4fNormal3fVertex3fvSUN)( const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v );
        void       (WINE_GLAPI *p_glTexCoord2fColor4ubVertex3fSUN)( GLfloat s, GLfloat t, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glTexCoord2fColor4ubVertex3fvSUN)( const GLfloat *tc, const GLubyte *c, const GLfloat *v );
        void       (WINE_GLAPI *p_glTexCoord2fNormal3fVertex3fSUN)( GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glTexCoord2fNormal3fVertex3fvSUN)( const GLfloat *tc, const GLfloat *n, const GLfloat *v );
        void       (WINE_GLAPI *p_glTexCoord2fVertex3fSUN)( GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glTexCoord2fVertex3fvSUN)( const GLfloat *tc, const GLfloat *v );
        void       (WINE_GLAPI *p_glTexCoord2hNV)( GLhalfNV s, GLhalfNV t );
        void       (WINE_GLAPI *p_glTexCoord2hvNV)( const GLhalfNV *v );
        void       (WINE_GLAPI *p_glTexCoord2xOES)( GLfixed s, GLfixed t );
        void       (WINE_GLAPI *p_glTexCoord2xvOES)( const GLfixed *coords );
        void       (WINE_GLAPI *p_glTexCoord3bOES)( GLbyte s, GLbyte t, GLbyte r );
        void       (WINE_GLAPI *p_glTexCoord3bvOES)( const GLbyte *coords );
        void       (WINE_GLAPI *p_glTexCoord3hNV)( GLhalfNV s, GLhalfNV t, GLhalfNV r );
        void       (WINE_GLAPI *p_glTexCoord3hvNV)( const GLhalfNV *v );
        void       (WINE_GLAPI *p_glTexCoord3xOES)( GLfixed s, GLfixed t, GLfixed r );
        void       (WINE_GLAPI *p_glTexCoord3xvOES)( const GLfixed *coords );
        void       (WINE_GLAPI *p_glTexCoord4bOES)( GLbyte s, GLbyte t, GLbyte r, GLbyte q );
        void       (WINE_GLAPI *p_glTexCoord4bvOES)( const GLbyte *coords );
        void       (WINE_GLAPI *p_glTexCoord4fColor4fNormal3fVertex4fSUN)( GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
        void       (WINE_GLAPI *p_glTexCoord4fColor4fNormal3fVertex4fvSUN)( const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v );
        void       (WINE_GLAPI *p_glTexCoord4fVertex4fSUN)( GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
        void       (WINE_GLAPI *p_glTexCoord4fVertex4fvSUN)( const GLfloat *tc, const GLfloat *v );
        void       (WINE_GLAPI *p_glTexCoord4hNV)( GLhalfNV s, GLhalfNV t, GLhalfNV r, GLhalfNV q );
        void       (WINE_GLAPI *p_glTexCoord4hvNV)( const GLhalfNV *v );
        void       (WINE_GLAPI *p_glTexCoord4xOES)( GLfixed s, GLfixed t, GLfixed r, GLfixed q );
        void       (WINE_GLAPI *p_glTexCoord4xvOES)( const GLfixed *coords );
        void       (WINE_GLAPI *p_glTexCoordFormatNV)( GLint size, GLenum type, GLsizei stride );
        void       (WINE_GLAPI *p_glTexCoordP1ui)( GLenum type, GLuint coords );
        void       (WINE_GLAPI *p_glTexCoordP1uiv)( GLenum type, const GLuint *coords );
        void       (WINE_GLAPI *p_glTexCoordP2ui)( GLenum type, GLuint coords );
        void       (WINE_GLAPI *p_glTexCoordP2uiv)( GLenum type, const GLuint *coords );
        void       (WINE_GLAPI *p_glTexCoordP3ui)( GLenum type, GLuint coords );
        void       (WINE_GLAPI *p_glTexCoordP3uiv)( GLenum type, const GLuint *coords );
        void       (WINE_GLAPI *p_glTexCoordP4ui)( GLenum type, GLuint coords );
        void       (WINE_GLAPI *p_glTexCoordP4uiv)( GLenum type, const GLuint *coords );
        void       (WINE_GLAPI *p_glTexCoordPointerEXT)( GLint size, GLenum type, GLsizei stride, GLsizei count, const void *pointer );
        void       (WINE_GLAPI *p_glTexCoordPointerListIBM)( GLint size, GLenum type, GLint stride, const void **pointer, GLint ptrstride );
        void       (WINE_GLAPI *p_glTexCoordPointervINTEL)( GLint size, GLenum type, const void **pointer );
        void       (WINE_GLAPI *p_glTexEnvxOES)( GLenum target, GLenum pname, GLfixed param );
        void       (WINE_GLAPI *p_glTexEnvxvOES)( GLenum target, GLenum pname, const GLfixed *params );
        void       (WINE_GLAPI *p_glTexFilterFuncSGIS)( GLenum target, GLenum filter, GLsizei n, const GLfloat *weights );
        void       (WINE_GLAPI *p_glTexGenxOES)( GLenum coord, GLenum pname, GLfixed param );
        void       (WINE_GLAPI *p_glTexGenxvOES)( GLenum coord, GLenum pname, const GLfixed *params );
        void       (WINE_GLAPI *p_glTexImage2DMultisample)( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations );
        void       (WINE_GLAPI *p_glTexImage2DMultisampleCoverageNV)( GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations );
        void       (WINE_GLAPI *p_glTexImage3D)( GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glTexImage3DEXT)( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glTexImage3DMultisample)( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations );
        void       (WINE_GLAPI *p_glTexImage3DMultisampleCoverageNV)( GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations );
        void       (WINE_GLAPI *p_glTexImage4DSGIS)( GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLint border, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glTexPageCommitmentARB)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLboolean commit );
        void       (WINE_GLAPI *p_glTexParameterIiv)( GLenum target, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glTexParameterIivEXT)( GLenum target, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glTexParameterIuiv)( GLenum target, GLenum pname, const GLuint *params );
        void       (WINE_GLAPI *p_glTexParameterIuivEXT)( GLenum target, GLenum pname, const GLuint *params );
        void       (WINE_GLAPI *p_glTexParameterxOES)( GLenum target, GLenum pname, GLfixed param );
        void       (WINE_GLAPI *p_glTexParameterxvOES)( GLenum target, GLenum pname, const GLfixed *params );
        void       (WINE_GLAPI *p_glTexRenderbufferNV)( GLenum target, GLuint renderbuffer );
        void       (WINE_GLAPI *p_glTexStorage1D)( GLenum target, GLsizei levels, GLenum internalformat, GLsizei width );
        void       (WINE_GLAPI *p_glTexStorage2D)( GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glTexStorage2DMultisample)( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations );
        void       (WINE_GLAPI *p_glTexStorage3D)( GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth );
        void       (WINE_GLAPI *p_glTexStorage3DMultisample)( GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations );
        void       (WINE_GLAPI *p_glTexStorageMem1DEXT)( GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLuint memory, GLuint64 offset );
        void       (WINE_GLAPI *p_glTexStorageMem2DEXT)( GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLuint memory, GLuint64 offset );
        void       (WINE_GLAPI *p_glTexStorageMem2DMultisampleEXT)( GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset );
        void       (WINE_GLAPI *p_glTexStorageMem3DEXT)( GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLuint memory, GLuint64 offset );
        void       (WINE_GLAPI *p_glTexStorageMem3DMultisampleEXT)( GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset );
        void       (WINE_GLAPI *p_glTexStorageSparseAMD)( GLenum target, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei layers, GLbitfield flags );
        void       (WINE_GLAPI *p_glTexSubImage1DEXT)( GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glTexSubImage2DEXT)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glTexSubImage3D)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glTexSubImage3DEXT)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glTexSubImage4DSGIS)( GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glTextureAttachMemoryNV)( GLuint texture, GLuint memory, GLuint64 offset );
        void       (WINE_GLAPI *p_glTextureBarrier)(void);
        void       (WINE_GLAPI *p_glTextureBarrierNV)(void);
        void       (WINE_GLAPI *p_glTextureBuffer)( GLuint texture, GLenum internalformat, GLuint buffer );
        void       (WINE_GLAPI *p_glTextureBufferEXT)( GLuint texture, GLenum target, GLenum internalformat, GLuint buffer );
        void       (WINE_GLAPI *p_glTextureBufferRange)( GLuint texture, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size );
        void       (WINE_GLAPI *p_glTextureBufferRangeEXT)( GLuint texture, GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size );
        void       (WINE_GLAPI *p_glTextureColorMaskSGIS)( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha );
        void       (WINE_GLAPI *p_glTextureImage1DEXT)( GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glTextureImage2DEXT)( GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glTextureImage2DMultisampleCoverageNV)( GLuint texture, GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations );
        void       (WINE_GLAPI *p_glTextureImage2DMultisampleNV)( GLuint texture, GLenum target, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations );
        void       (WINE_GLAPI *p_glTextureImage3DEXT)( GLuint texture, GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glTextureImage3DMultisampleCoverageNV)( GLuint texture, GLenum target, GLsizei coverageSamples, GLsizei colorSamples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations );
        void       (WINE_GLAPI *p_glTextureImage3DMultisampleNV)( GLuint texture, GLenum target, GLsizei samples, GLint internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations );
        void       (WINE_GLAPI *p_glTextureLightEXT)( GLenum pname );
        void       (WINE_GLAPI *p_glTextureMaterialEXT)( GLenum face, GLenum mode );
        void       (WINE_GLAPI *p_glTextureNormalEXT)( GLenum mode );
        void       (WINE_GLAPI *p_glTexturePageCommitmentEXT)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLboolean commit );
        void       (WINE_GLAPI *p_glTextureParameterIiv)( GLuint texture, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glTextureParameterIivEXT)( GLuint texture, GLenum target, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glTextureParameterIuiv)( GLuint texture, GLenum pname, const GLuint *params );
        void       (WINE_GLAPI *p_glTextureParameterIuivEXT)( GLuint texture, GLenum target, GLenum pname, const GLuint *params );
        void       (WINE_GLAPI *p_glTextureParameterf)( GLuint texture, GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glTextureParameterfEXT)( GLuint texture, GLenum target, GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glTextureParameterfv)( GLuint texture, GLenum pname, const GLfloat *param );
        void       (WINE_GLAPI *p_glTextureParameterfvEXT)( GLuint texture, GLenum target, GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glTextureParameteri)( GLuint texture, GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glTextureParameteriEXT)( GLuint texture, GLenum target, GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glTextureParameteriv)( GLuint texture, GLenum pname, const GLint *param );
        void       (WINE_GLAPI *p_glTextureParameterivEXT)( GLuint texture, GLenum target, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glTextureRangeAPPLE)( GLenum target, GLsizei length, const void *pointer );
        void       (WINE_GLAPI *p_glTextureRenderbufferEXT)( GLuint texture, GLenum target, GLuint renderbuffer );
        void       (WINE_GLAPI *p_glTextureStorage1D)( GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width );
        void       (WINE_GLAPI *p_glTextureStorage1DEXT)( GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width );
        void       (WINE_GLAPI *p_glTextureStorage2D)( GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glTextureStorage2DEXT)( GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height );
        void       (WINE_GLAPI *p_glTextureStorage2DMultisample)( GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations );
        void       (WINE_GLAPI *p_glTextureStorage2DMultisampleEXT)( GLuint texture, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations );
        void       (WINE_GLAPI *p_glTextureStorage3D)( GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth );
        void       (WINE_GLAPI *p_glTextureStorage3DEXT)( GLuint texture, GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth );
        void       (WINE_GLAPI *p_glTextureStorage3DMultisample)( GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations );
        void       (WINE_GLAPI *p_glTextureStorage3DMultisampleEXT)( GLuint texture, GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations );
        void       (WINE_GLAPI *p_glTextureStorageMem1DEXT)( GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLuint memory, GLuint64 offset );
        void       (WINE_GLAPI *p_glTextureStorageMem2DEXT)( GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLuint memory, GLuint64 offset );
        void       (WINE_GLAPI *p_glTextureStorageMem2DMultisampleEXT)( GLuint texture, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset );
        void       (WINE_GLAPI *p_glTextureStorageMem3DEXT)( GLuint texture, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLuint memory, GLuint64 offset );
        void       (WINE_GLAPI *p_glTextureStorageMem3DMultisampleEXT)( GLuint texture, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations, GLuint memory, GLuint64 offset );
        void       (WINE_GLAPI *p_glTextureStorageSparseAMD)( GLuint texture, GLenum target, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLsizei layers, GLbitfield flags );
        void       (WINE_GLAPI *p_glTextureSubImage1D)( GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glTextureSubImage1DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glTextureSubImage2D)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glTextureSubImage2DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glTextureSubImage3D)( GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glTextureSubImage3DEXT)( GLuint texture, GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels );
        void       (WINE_GLAPI *p_glTextureView)( GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers );
        void       (WINE_GLAPI *p_glTrackMatrixNV)( GLenum target, GLuint address, GLenum matrix, GLenum transform );
        void       (WINE_GLAPI *p_glTransformFeedbackAttribsNV)( GLsizei count, const GLint *attribs, GLenum bufferMode );
        void       (WINE_GLAPI *p_glTransformFeedbackBufferBase)( GLuint xfb, GLuint index, GLuint buffer );
        void       (WINE_GLAPI *p_glTransformFeedbackBufferRange)( GLuint xfb, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size );
        void       (WINE_GLAPI *p_glTransformFeedbackStreamAttribsNV)( GLsizei count, const GLint *attribs, GLsizei nbuffers, const GLint *bufstreams, GLenum bufferMode );
        void       (WINE_GLAPI *p_glTransformFeedbackVaryings)( GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode );
        void       (WINE_GLAPI *p_glTransformFeedbackVaryingsEXT)( GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode );
        void       (WINE_GLAPI *p_glTransformFeedbackVaryingsNV)( GLuint program, GLsizei count, const GLint *locations, GLenum bufferMode );
        void       (WINE_GLAPI *p_glTransformPathNV)( GLuint resultPath, GLuint srcPath, GLenum transformType, const GLfloat *transformValues );
        void       (WINE_GLAPI *p_glTranslatexOES)( GLfixed x, GLfixed y, GLfixed z );
        void       (WINE_GLAPI *p_glUniform1d)( GLint location, GLdouble x );
        void       (WINE_GLAPI *p_glUniform1dv)( GLint location, GLsizei count, const GLdouble *value );
        void       (WINE_GLAPI *p_glUniform1f)( GLint location, GLfloat v0 );
        void       (WINE_GLAPI *p_glUniform1fARB)( GLint location, GLfloat v0 );
        void       (WINE_GLAPI *p_glUniform1fv)( GLint location, GLsizei count, const GLfloat *value );
        void       (WINE_GLAPI *p_glUniform1fvARB)( GLint location, GLsizei count, const GLfloat *value );
        void       (WINE_GLAPI *p_glUniform1i)( GLint location, GLint v0 );
        void       (WINE_GLAPI *p_glUniform1i64ARB)( GLint location, GLint64 x );
        void       (WINE_GLAPI *p_glUniform1i64NV)( GLint location, GLint64EXT x );
        void       (WINE_GLAPI *p_glUniform1i64vARB)( GLint location, GLsizei count, const GLint64 *value );
        void       (WINE_GLAPI *p_glUniform1i64vNV)( GLint location, GLsizei count, const GLint64EXT *value );
        void       (WINE_GLAPI *p_glUniform1iARB)( GLint location, GLint v0 );
        void       (WINE_GLAPI *p_glUniform1iv)( GLint location, GLsizei count, const GLint *value );
        void       (WINE_GLAPI *p_glUniform1ivARB)( GLint location, GLsizei count, const GLint *value );
        void       (WINE_GLAPI *p_glUniform1ui)( GLint location, GLuint v0 );
        void       (WINE_GLAPI *p_glUniform1ui64ARB)( GLint location, GLuint64 x );
        void       (WINE_GLAPI *p_glUniform1ui64NV)( GLint location, GLuint64EXT x );
        void       (WINE_GLAPI *p_glUniform1ui64vARB)( GLint location, GLsizei count, const GLuint64 *value );
        void       (WINE_GLAPI *p_glUniform1ui64vNV)( GLint location, GLsizei count, const GLuint64EXT *value );
        void       (WINE_GLAPI *p_glUniform1uiEXT)( GLint location, GLuint v0 );
        void       (WINE_GLAPI *p_glUniform1uiv)( GLint location, GLsizei count, const GLuint *value );
        void       (WINE_GLAPI *p_glUniform1uivEXT)( GLint location, GLsizei count, const GLuint *value );
        void       (WINE_GLAPI *p_glUniform2d)( GLint location, GLdouble x, GLdouble y );
        void       (WINE_GLAPI *p_glUniform2dv)( GLint location, GLsizei count, const GLdouble *value );
        void       (WINE_GLAPI *p_glUniform2f)( GLint location, GLfloat v0, GLfloat v1 );
        void       (WINE_GLAPI *p_glUniform2fARB)( GLint location, GLfloat v0, GLfloat v1 );
        void       (WINE_GLAPI *p_glUniform2fv)( GLint location, GLsizei count, const GLfloat *value );
        void       (WINE_GLAPI *p_glUniform2fvARB)( GLint location, GLsizei count, const GLfloat *value );
        void       (WINE_GLAPI *p_glUniform2i)( GLint location, GLint v0, GLint v1 );
        void       (WINE_GLAPI *p_glUniform2i64ARB)( GLint location, GLint64 x, GLint64 y );
        void       (WINE_GLAPI *p_glUniform2i64NV)( GLint location, GLint64EXT x, GLint64EXT y );
        void       (WINE_GLAPI *p_glUniform2i64vARB)( GLint location, GLsizei count, const GLint64 *value );
        void       (WINE_GLAPI *p_glUniform2i64vNV)( GLint location, GLsizei count, const GLint64EXT *value );
        void       (WINE_GLAPI *p_glUniform2iARB)( GLint location, GLint v0, GLint v1 );
        void       (WINE_GLAPI *p_glUniform2iv)( GLint location, GLsizei count, const GLint *value );
        void       (WINE_GLAPI *p_glUniform2ivARB)( GLint location, GLsizei count, const GLint *value );
        void       (WINE_GLAPI *p_glUniform2ui)( GLint location, GLuint v0, GLuint v1 );
        void       (WINE_GLAPI *p_glUniform2ui64ARB)( GLint location, GLuint64 x, GLuint64 y );
        void       (WINE_GLAPI *p_glUniform2ui64NV)( GLint location, GLuint64EXT x, GLuint64EXT y );
        void       (WINE_GLAPI *p_glUniform2ui64vARB)( GLint location, GLsizei count, const GLuint64 *value );
        void       (WINE_GLAPI *p_glUniform2ui64vNV)( GLint location, GLsizei count, const GLuint64EXT *value );
        void       (WINE_GLAPI *p_glUniform2uiEXT)( GLint location, GLuint v0, GLuint v1 );
        void       (WINE_GLAPI *p_glUniform2uiv)( GLint location, GLsizei count, const GLuint *value );
        void       (WINE_GLAPI *p_glUniform2uivEXT)( GLint location, GLsizei count, const GLuint *value );
        void       (WINE_GLAPI *p_glUniform3d)( GLint location, GLdouble x, GLdouble y, GLdouble z );
        void       (WINE_GLAPI *p_glUniform3dv)( GLint location, GLsizei count, const GLdouble *value );
        void       (WINE_GLAPI *p_glUniform3f)( GLint location, GLfloat v0, GLfloat v1, GLfloat v2 );
        void       (WINE_GLAPI *p_glUniform3fARB)( GLint location, GLfloat v0, GLfloat v1, GLfloat v2 );
        void       (WINE_GLAPI *p_glUniform3fv)( GLint location, GLsizei count, const GLfloat *value );
        void       (WINE_GLAPI *p_glUniform3fvARB)( GLint location, GLsizei count, const GLfloat *value );
        void       (WINE_GLAPI *p_glUniform3i)( GLint location, GLint v0, GLint v1, GLint v2 );
        void       (WINE_GLAPI *p_glUniform3i64ARB)( GLint location, GLint64 x, GLint64 y, GLint64 z );
        void       (WINE_GLAPI *p_glUniform3i64NV)( GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z );
        void       (WINE_GLAPI *p_glUniform3i64vARB)( GLint location, GLsizei count, const GLint64 *value );
        void       (WINE_GLAPI *p_glUniform3i64vNV)( GLint location, GLsizei count, const GLint64EXT *value );
        void       (WINE_GLAPI *p_glUniform3iARB)( GLint location, GLint v0, GLint v1, GLint v2 );
        void       (WINE_GLAPI *p_glUniform3iv)( GLint location, GLsizei count, const GLint *value );
        void       (WINE_GLAPI *p_glUniform3ivARB)( GLint location, GLsizei count, const GLint *value );
        void       (WINE_GLAPI *p_glUniform3ui)( GLint location, GLuint v0, GLuint v1, GLuint v2 );
        void       (WINE_GLAPI *p_glUniform3ui64ARB)( GLint location, GLuint64 x, GLuint64 y, GLuint64 z );
        void       (WINE_GLAPI *p_glUniform3ui64NV)( GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z );
        void       (WINE_GLAPI *p_glUniform3ui64vARB)( GLint location, GLsizei count, const GLuint64 *value );
        void       (WINE_GLAPI *p_glUniform3ui64vNV)( GLint location, GLsizei count, const GLuint64EXT *value );
        void       (WINE_GLAPI *p_glUniform3uiEXT)( GLint location, GLuint v0, GLuint v1, GLuint v2 );
        void       (WINE_GLAPI *p_glUniform3uiv)( GLint location, GLsizei count, const GLuint *value );
        void       (WINE_GLAPI *p_glUniform3uivEXT)( GLint location, GLsizei count, const GLuint *value );
        void       (WINE_GLAPI *p_glUniform4d)( GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
        void       (WINE_GLAPI *p_glUniform4dv)( GLint location, GLsizei count, const GLdouble *value );
        void       (WINE_GLAPI *p_glUniform4f)( GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 );
        void       (WINE_GLAPI *p_glUniform4fARB)( GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3 );
        void       (WINE_GLAPI *p_glUniform4fv)( GLint location, GLsizei count, const GLfloat *value );
        void       (WINE_GLAPI *p_glUniform4fvARB)( GLint location, GLsizei count, const GLfloat *value );
        void       (WINE_GLAPI *p_glUniform4i)( GLint location, GLint v0, GLint v1, GLint v2, GLint v3 );
        void       (WINE_GLAPI *p_glUniform4i64ARB)( GLint location, GLint64 x, GLint64 y, GLint64 z, GLint64 w );
        void       (WINE_GLAPI *p_glUniform4i64NV)( GLint location, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w );
        void       (WINE_GLAPI *p_glUniform4i64vARB)( GLint location, GLsizei count, const GLint64 *value );
        void       (WINE_GLAPI *p_glUniform4i64vNV)( GLint location, GLsizei count, const GLint64EXT *value );
        void       (WINE_GLAPI *p_glUniform4iARB)( GLint location, GLint v0, GLint v1, GLint v2, GLint v3 );
        void       (WINE_GLAPI *p_glUniform4iv)( GLint location, GLsizei count, const GLint *value );
        void       (WINE_GLAPI *p_glUniform4ivARB)( GLint location, GLsizei count, const GLint *value );
        void       (WINE_GLAPI *p_glUniform4ui)( GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 );
        void       (WINE_GLAPI *p_glUniform4ui64ARB)( GLint location, GLuint64 x, GLuint64 y, GLuint64 z, GLuint64 w );
        void       (WINE_GLAPI *p_glUniform4ui64NV)( GLint location, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w );
        void       (WINE_GLAPI *p_glUniform4ui64vARB)( GLint location, GLsizei count, const GLuint64 *value );
        void       (WINE_GLAPI *p_glUniform4ui64vNV)( GLint location, GLsizei count, const GLuint64EXT *value );
        void       (WINE_GLAPI *p_glUniform4uiEXT)( GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3 );
        void       (WINE_GLAPI *p_glUniform4uiv)( GLint location, GLsizei count, const GLuint *value );
        void       (WINE_GLAPI *p_glUniform4uivEXT)( GLint location, GLsizei count, const GLuint *value );
        void       (WINE_GLAPI *p_glUniformBlockBinding)( GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding );
        void       (WINE_GLAPI *p_glUniformBufferEXT)( GLuint program, GLint location, GLuint buffer );
        void       (WINE_GLAPI *p_glUniformHandleui64ARB)( GLint location, GLuint64 value );
        void       (WINE_GLAPI *p_glUniformHandleui64NV)( GLint location, GLuint64 value );
        void       (WINE_GLAPI *p_glUniformHandleui64vARB)( GLint location, GLsizei count, const GLuint64 *value );
        void       (WINE_GLAPI *p_glUniformHandleui64vNV)( GLint location, GLsizei count, const GLuint64 *value );
        void       (WINE_GLAPI *p_glUniformMatrix2dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glUniformMatrix2fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glUniformMatrix2fvARB)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glUniformMatrix2x3dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glUniformMatrix2x3fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glUniformMatrix2x4dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glUniformMatrix2x4fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glUniformMatrix3dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glUniformMatrix3fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glUniformMatrix3fvARB)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glUniformMatrix3x2dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glUniformMatrix3x2fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glUniformMatrix3x4dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glUniformMatrix3x4fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glUniformMatrix4dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glUniformMatrix4fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glUniformMatrix4fvARB)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glUniformMatrix4x2dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glUniformMatrix4x2fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glUniformMatrix4x3dv)( GLint location, GLsizei count, GLboolean transpose, const GLdouble *value );
        void       (WINE_GLAPI *p_glUniformMatrix4x3fv)( GLint location, GLsizei count, GLboolean transpose, const GLfloat *value );
        void       (WINE_GLAPI *p_glUniformSubroutinesuiv)( GLenum shadertype, GLsizei count, const GLuint *indices );
        void       (WINE_GLAPI *p_glUniformui64NV)( GLint location, GLuint64EXT value );
        void       (WINE_GLAPI *p_glUniformui64vNV)( GLint location, GLsizei count, const GLuint64EXT *value );
        void       (WINE_GLAPI *p_glUnlockArraysEXT)(void);
        GLboolean  (WINE_GLAPI *p_glUnmapBuffer)( GLenum target );
        GLboolean  (WINE_GLAPI *p_glUnmapBufferARB)( GLenum target );
        GLboolean  (WINE_GLAPI *p_glUnmapNamedBuffer)( GLuint buffer );
        GLboolean  (WINE_GLAPI *p_glUnmapNamedBufferEXT)( GLuint buffer );
        void       (WINE_GLAPI *p_glUnmapObjectBufferATI)( GLuint buffer );
        void       (WINE_GLAPI *p_glUnmapTexture2DINTEL)( GLuint texture, GLint level );
        void       (WINE_GLAPI *p_glUpdateObjectBufferATI)( GLuint buffer, GLuint offset, GLsizei size, const void *pointer, GLenum preserve );
        void       (WINE_GLAPI *p_glUploadGpuMaskNVX)( GLbitfield mask );
        void       (WINE_GLAPI *p_glUseProgram)( GLuint program );
        void       (WINE_GLAPI *p_glUseProgramObjectARB)( GLhandleARB programObj );
        void       (WINE_GLAPI *p_glUseProgramStages)( GLuint pipeline, GLbitfield stages, GLuint program );
        void       (WINE_GLAPI *p_glUseShaderProgramEXT)( GLenum type, GLuint program );
        void       (WINE_GLAPI *p_glVDPAUFiniNV)(void);
        void       (WINE_GLAPI *p_glVDPAUGetSurfaceivNV)( GLvdpauSurfaceNV surface, GLenum pname, GLsizei count, GLsizei *length, GLint *values );
        void       (WINE_GLAPI *p_glVDPAUInitNV)( const void *vdpDevice, const void *getProcAddress );
        GLboolean  (WINE_GLAPI *p_glVDPAUIsSurfaceNV)( GLvdpauSurfaceNV surface );
        void       (WINE_GLAPI *p_glVDPAUMapSurfacesNV)( GLsizei numSurfaces, const GLvdpauSurfaceNV *surfaces );
        GLvdpauSurfaceNV (WINE_GLAPI *p_glVDPAURegisterOutputSurfaceNV)( const void *vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames );
        GLvdpauSurfaceNV (WINE_GLAPI *p_glVDPAURegisterVideoSurfaceNV)( const void *vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames );
        GLvdpauSurfaceNV (WINE_GLAPI *p_glVDPAURegisterVideoSurfaceWithPictureStructureNV)( const void *vdpSurface, GLenum target, GLsizei numTextureNames, const GLuint *textureNames, GLboolean isFrameStructure );
        void       (WINE_GLAPI *p_glVDPAUSurfaceAccessNV)( GLvdpauSurfaceNV surface, GLenum access );
        void       (WINE_GLAPI *p_glVDPAUUnmapSurfacesNV)( GLsizei numSurface, const GLvdpauSurfaceNV *surfaces );
        void       (WINE_GLAPI *p_glVDPAUUnregisterSurfaceNV)( GLvdpauSurfaceNV surface );
        void       (WINE_GLAPI *p_glValidateProgram)( GLuint program );
        void       (WINE_GLAPI *p_glValidateProgramARB)( GLhandleARB programObj );
        void       (WINE_GLAPI *p_glValidateProgramPipeline)( GLuint pipeline );
        void       (WINE_GLAPI *p_glVariantArrayObjectATI)( GLuint id, GLenum type, GLsizei stride, GLuint buffer, GLuint offset );
        void       (WINE_GLAPI *p_glVariantPointerEXT)( GLuint id, GLenum type, GLuint stride, const void *addr );
        void       (WINE_GLAPI *p_glVariantbvEXT)( GLuint id, const GLbyte *addr );
        void       (WINE_GLAPI *p_glVariantdvEXT)( GLuint id, const GLdouble *addr );
        void       (WINE_GLAPI *p_glVariantfvEXT)( GLuint id, const GLfloat *addr );
        void       (WINE_GLAPI *p_glVariantivEXT)( GLuint id, const GLint *addr );
        void       (WINE_GLAPI *p_glVariantsvEXT)( GLuint id, const GLshort *addr );
        void       (WINE_GLAPI *p_glVariantubvEXT)( GLuint id, const GLubyte *addr );
        void       (WINE_GLAPI *p_glVariantuivEXT)( GLuint id, const GLuint *addr );
        void       (WINE_GLAPI *p_glVariantusvEXT)( GLuint id, const GLushort *addr );
        void       (WINE_GLAPI *p_glVertex2bOES)( GLbyte x, GLbyte y );
        void       (WINE_GLAPI *p_glVertex2bvOES)( const GLbyte *coords );
        void       (WINE_GLAPI *p_glVertex2hNV)( GLhalfNV x, GLhalfNV y );
        void       (WINE_GLAPI *p_glVertex2hvNV)( const GLhalfNV *v );
        void       (WINE_GLAPI *p_glVertex2xOES)( GLfixed x );
        void       (WINE_GLAPI *p_glVertex2xvOES)( const GLfixed *coords );
        void       (WINE_GLAPI *p_glVertex3bOES)( GLbyte x, GLbyte y, GLbyte z );
        void       (WINE_GLAPI *p_glVertex3bvOES)( const GLbyte *coords );
        void       (WINE_GLAPI *p_glVertex3hNV)( GLhalfNV x, GLhalfNV y, GLhalfNV z );
        void       (WINE_GLAPI *p_glVertex3hvNV)( const GLhalfNV *v );
        void       (WINE_GLAPI *p_glVertex3xOES)( GLfixed x, GLfixed y );
        void       (WINE_GLAPI *p_glVertex3xvOES)( const GLfixed *coords );
        void       (WINE_GLAPI *p_glVertex4bOES)( GLbyte x, GLbyte y, GLbyte z, GLbyte w );
        void       (WINE_GLAPI *p_glVertex4bvOES)( const GLbyte *coords );
        void       (WINE_GLAPI *p_glVertex4hNV)( GLhalfNV x, GLhalfNV y, GLhalfNV z, GLhalfNV w );
        void       (WINE_GLAPI *p_glVertex4hvNV)( const GLhalfNV *v );
        void       (WINE_GLAPI *p_glVertex4xOES)( GLfixed x, GLfixed y, GLfixed z );
        void       (WINE_GLAPI *p_glVertex4xvOES)( const GLfixed *coords );
        void       (WINE_GLAPI *p_glVertexArrayAttribBinding)( GLuint vaobj, GLuint attribindex, GLuint bindingindex );
        void       (WINE_GLAPI *p_glVertexArrayAttribFormat)( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset );
        void       (WINE_GLAPI *p_glVertexArrayAttribIFormat)( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset );
        void       (WINE_GLAPI *p_glVertexArrayAttribLFormat)( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset );
        void       (WINE_GLAPI *p_glVertexArrayBindVertexBufferEXT)( GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride );
        void       (WINE_GLAPI *p_glVertexArrayBindingDivisor)( GLuint vaobj, GLuint bindingindex, GLuint divisor );
        void       (WINE_GLAPI *p_glVertexArrayColorOffsetEXT)( GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset );
        void       (WINE_GLAPI *p_glVertexArrayEdgeFlagOffsetEXT)( GLuint vaobj, GLuint buffer, GLsizei stride, GLintptr offset );
        void       (WINE_GLAPI *p_glVertexArrayElementBuffer)( GLuint vaobj, GLuint buffer );
        void       (WINE_GLAPI *p_glVertexArrayFogCoordOffsetEXT)( GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride, GLintptr offset );
        void       (WINE_GLAPI *p_glVertexArrayIndexOffsetEXT)( GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride, GLintptr offset );
        void       (WINE_GLAPI *p_glVertexArrayMultiTexCoordOffsetEXT)( GLuint vaobj, GLuint buffer, GLenum texunit, GLint size, GLenum type, GLsizei stride, GLintptr offset );
        void       (WINE_GLAPI *p_glVertexArrayNormalOffsetEXT)( GLuint vaobj, GLuint buffer, GLenum type, GLsizei stride, GLintptr offset );
        void       (WINE_GLAPI *p_glVertexArrayParameteriAPPLE)( GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glVertexArrayRangeAPPLE)( GLsizei length, void *pointer );
        void       (WINE_GLAPI *p_glVertexArrayRangeNV)( GLsizei length, const void *pointer );
        void       (WINE_GLAPI *p_glVertexArraySecondaryColorOffsetEXT)( GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset );
        void       (WINE_GLAPI *p_glVertexArrayTexCoordOffsetEXT)( GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset );
        void       (WINE_GLAPI *p_glVertexArrayVertexAttribBindingEXT)( GLuint vaobj, GLuint attribindex, GLuint bindingindex );
        void       (WINE_GLAPI *p_glVertexArrayVertexAttribDivisorEXT)( GLuint vaobj, GLuint index, GLuint divisor );
        void       (WINE_GLAPI *p_glVertexArrayVertexAttribFormatEXT)( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset );
        void       (WINE_GLAPI *p_glVertexArrayVertexAttribIFormatEXT)( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset );
        void       (WINE_GLAPI *p_glVertexArrayVertexAttribIOffsetEXT)( GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLsizei stride, GLintptr offset );
        void       (WINE_GLAPI *p_glVertexArrayVertexAttribLFormatEXT)( GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset );
        void       (WINE_GLAPI *p_glVertexArrayVertexAttribLOffsetEXT)( GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLsizei stride, GLintptr offset );
        void       (WINE_GLAPI *p_glVertexArrayVertexAttribOffsetEXT)( GLuint vaobj, GLuint buffer, GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLintptr offset );
        void       (WINE_GLAPI *p_glVertexArrayVertexBindingDivisorEXT)( GLuint vaobj, GLuint bindingindex, GLuint divisor );
        void       (WINE_GLAPI *p_glVertexArrayVertexBuffer)( GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride );
        void       (WINE_GLAPI *p_glVertexArrayVertexBuffers)( GLuint vaobj, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides );
        void       (WINE_GLAPI *p_glVertexArrayVertexOffsetEXT)( GLuint vaobj, GLuint buffer, GLint size, GLenum type, GLsizei stride, GLintptr offset );
        void       (WINE_GLAPI *p_glVertexAttrib1d)( GLuint index, GLdouble x );
        void       (WINE_GLAPI *p_glVertexAttrib1dARB)( GLuint index, GLdouble x );
        void       (WINE_GLAPI *p_glVertexAttrib1dNV)( GLuint index, GLdouble x );
        void       (WINE_GLAPI *p_glVertexAttrib1dv)( GLuint index, const GLdouble *v );
        void       (WINE_GLAPI *p_glVertexAttrib1dvARB)( GLuint index, const GLdouble *v );
        void       (WINE_GLAPI *p_glVertexAttrib1dvNV)( GLuint index, const GLdouble *v );
        void       (WINE_GLAPI *p_glVertexAttrib1f)( GLuint index, GLfloat x );
        void       (WINE_GLAPI *p_glVertexAttrib1fARB)( GLuint index, GLfloat x );
        void       (WINE_GLAPI *p_glVertexAttrib1fNV)( GLuint index, GLfloat x );
        void       (WINE_GLAPI *p_glVertexAttrib1fv)( GLuint index, const GLfloat *v );
        void       (WINE_GLAPI *p_glVertexAttrib1fvARB)( GLuint index, const GLfloat *v );
        void       (WINE_GLAPI *p_glVertexAttrib1fvNV)( GLuint index, const GLfloat *v );
        void       (WINE_GLAPI *p_glVertexAttrib1hNV)( GLuint index, GLhalfNV x );
        void       (WINE_GLAPI *p_glVertexAttrib1hvNV)( GLuint index, const GLhalfNV *v );
        void       (WINE_GLAPI *p_glVertexAttrib1s)( GLuint index, GLshort x );
        void       (WINE_GLAPI *p_glVertexAttrib1sARB)( GLuint index, GLshort x );
        void       (WINE_GLAPI *p_glVertexAttrib1sNV)( GLuint index, GLshort x );
        void       (WINE_GLAPI *p_glVertexAttrib1sv)( GLuint index, const GLshort *v );
        void       (WINE_GLAPI *p_glVertexAttrib1svARB)( GLuint index, const GLshort *v );
        void       (WINE_GLAPI *p_glVertexAttrib1svNV)( GLuint index, const GLshort *v );
        void       (WINE_GLAPI *p_glVertexAttrib2d)( GLuint index, GLdouble x, GLdouble y );
        void       (WINE_GLAPI *p_glVertexAttrib2dARB)( GLuint index, GLdouble x, GLdouble y );
        void       (WINE_GLAPI *p_glVertexAttrib2dNV)( GLuint index, GLdouble x, GLdouble y );
        void       (WINE_GLAPI *p_glVertexAttrib2dv)( GLuint index, const GLdouble *v );
        void       (WINE_GLAPI *p_glVertexAttrib2dvARB)( GLuint index, const GLdouble *v );
        void       (WINE_GLAPI *p_glVertexAttrib2dvNV)( GLuint index, const GLdouble *v );
        void       (WINE_GLAPI *p_glVertexAttrib2f)( GLuint index, GLfloat x, GLfloat y );
        void       (WINE_GLAPI *p_glVertexAttrib2fARB)( GLuint index, GLfloat x, GLfloat y );
        void       (WINE_GLAPI *p_glVertexAttrib2fNV)( GLuint index, GLfloat x, GLfloat y );
        void       (WINE_GLAPI *p_glVertexAttrib2fv)( GLuint index, const GLfloat *v );
        void       (WINE_GLAPI *p_glVertexAttrib2fvARB)( GLuint index, const GLfloat *v );
        void       (WINE_GLAPI *p_glVertexAttrib2fvNV)( GLuint index, const GLfloat *v );
        void       (WINE_GLAPI *p_glVertexAttrib2hNV)( GLuint index, GLhalfNV x, GLhalfNV y );
        void       (WINE_GLAPI *p_glVertexAttrib2hvNV)( GLuint index, const GLhalfNV *v );
        void       (WINE_GLAPI *p_glVertexAttrib2s)( GLuint index, GLshort x, GLshort y );
        void       (WINE_GLAPI *p_glVertexAttrib2sARB)( GLuint index, GLshort x, GLshort y );
        void       (WINE_GLAPI *p_glVertexAttrib2sNV)( GLuint index, GLshort x, GLshort y );
        void       (WINE_GLAPI *p_glVertexAttrib2sv)( GLuint index, const GLshort *v );
        void       (WINE_GLAPI *p_glVertexAttrib2svARB)( GLuint index, const GLshort *v );
        void       (WINE_GLAPI *p_glVertexAttrib2svNV)( GLuint index, const GLshort *v );
        void       (WINE_GLAPI *p_glVertexAttrib3d)( GLuint index, GLdouble x, GLdouble y, GLdouble z );
        void       (WINE_GLAPI *p_glVertexAttrib3dARB)( GLuint index, GLdouble x, GLdouble y, GLdouble z );
        void       (WINE_GLAPI *p_glVertexAttrib3dNV)( GLuint index, GLdouble x, GLdouble y, GLdouble z );
        void       (WINE_GLAPI *p_glVertexAttrib3dv)( GLuint index, const GLdouble *v );
        void       (WINE_GLAPI *p_glVertexAttrib3dvARB)( GLuint index, const GLdouble *v );
        void       (WINE_GLAPI *p_glVertexAttrib3dvNV)( GLuint index, const GLdouble *v );
        void       (WINE_GLAPI *p_glVertexAttrib3f)( GLuint index, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glVertexAttrib3fARB)( GLuint index, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glVertexAttrib3fNV)( GLuint index, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glVertexAttrib3fv)( GLuint index, const GLfloat *v );
        void       (WINE_GLAPI *p_glVertexAttrib3fvARB)( GLuint index, const GLfloat *v );
        void       (WINE_GLAPI *p_glVertexAttrib3fvNV)( GLuint index, const GLfloat *v );
        void       (WINE_GLAPI *p_glVertexAttrib3hNV)( GLuint index, GLhalfNV x, GLhalfNV y, GLhalfNV z );
        void       (WINE_GLAPI *p_glVertexAttrib3hvNV)( GLuint index, const GLhalfNV *v );
        void       (WINE_GLAPI *p_glVertexAttrib3s)( GLuint index, GLshort x, GLshort y, GLshort z );
        void       (WINE_GLAPI *p_glVertexAttrib3sARB)( GLuint index, GLshort x, GLshort y, GLshort z );
        void       (WINE_GLAPI *p_glVertexAttrib3sNV)( GLuint index, GLshort x, GLshort y, GLshort z );
        void       (WINE_GLAPI *p_glVertexAttrib3sv)( GLuint index, const GLshort *v );
        void       (WINE_GLAPI *p_glVertexAttrib3svARB)( GLuint index, const GLshort *v );
        void       (WINE_GLAPI *p_glVertexAttrib3svNV)( GLuint index, const GLshort *v );
        void       (WINE_GLAPI *p_glVertexAttrib4Nbv)( GLuint index, const GLbyte *v );
        void       (WINE_GLAPI *p_glVertexAttrib4NbvARB)( GLuint index, const GLbyte *v );
        void       (WINE_GLAPI *p_glVertexAttrib4Niv)( GLuint index, const GLint *v );
        void       (WINE_GLAPI *p_glVertexAttrib4NivARB)( GLuint index, const GLint *v );
        void       (WINE_GLAPI *p_glVertexAttrib4Nsv)( GLuint index, const GLshort *v );
        void       (WINE_GLAPI *p_glVertexAttrib4NsvARB)( GLuint index, const GLshort *v );
        void       (WINE_GLAPI *p_glVertexAttrib4Nub)( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w );
        void       (WINE_GLAPI *p_glVertexAttrib4NubARB)( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w );
        void       (WINE_GLAPI *p_glVertexAttrib4Nubv)( GLuint index, const GLubyte *v );
        void       (WINE_GLAPI *p_glVertexAttrib4NubvARB)( GLuint index, const GLubyte *v );
        void       (WINE_GLAPI *p_glVertexAttrib4Nuiv)( GLuint index, const GLuint *v );
        void       (WINE_GLAPI *p_glVertexAttrib4NuivARB)( GLuint index, const GLuint *v );
        void       (WINE_GLAPI *p_glVertexAttrib4Nusv)( GLuint index, const GLushort *v );
        void       (WINE_GLAPI *p_glVertexAttrib4NusvARB)( GLuint index, const GLushort *v );
        void       (WINE_GLAPI *p_glVertexAttrib4bv)( GLuint index, const GLbyte *v );
        void       (WINE_GLAPI *p_glVertexAttrib4bvARB)( GLuint index, const GLbyte *v );
        void       (WINE_GLAPI *p_glVertexAttrib4d)( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
        void       (WINE_GLAPI *p_glVertexAttrib4dARB)( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
        void       (WINE_GLAPI *p_glVertexAttrib4dNV)( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
        void       (WINE_GLAPI *p_glVertexAttrib4dv)( GLuint index, const GLdouble *v );
        void       (WINE_GLAPI *p_glVertexAttrib4dvARB)( GLuint index, const GLdouble *v );
        void       (WINE_GLAPI *p_glVertexAttrib4dvNV)( GLuint index, const GLdouble *v );
        void       (WINE_GLAPI *p_glVertexAttrib4f)( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
        void       (WINE_GLAPI *p_glVertexAttrib4fARB)( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
        void       (WINE_GLAPI *p_glVertexAttrib4fNV)( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
        void       (WINE_GLAPI *p_glVertexAttrib4fv)( GLuint index, const GLfloat *v );
        void       (WINE_GLAPI *p_glVertexAttrib4fvARB)( GLuint index, const GLfloat *v );
        void       (WINE_GLAPI *p_glVertexAttrib4fvNV)( GLuint index, const GLfloat *v );
        void       (WINE_GLAPI *p_glVertexAttrib4hNV)( GLuint index, GLhalfNV x, GLhalfNV y, GLhalfNV z, GLhalfNV w );
        void       (WINE_GLAPI *p_glVertexAttrib4hvNV)( GLuint index, const GLhalfNV *v );
        void       (WINE_GLAPI *p_glVertexAttrib4iv)( GLuint index, const GLint *v );
        void       (WINE_GLAPI *p_glVertexAttrib4ivARB)( GLuint index, const GLint *v );
        void       (WINE_GLAPI *p_glVertexAttrib4s)( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w );
        void       (WINE_GLAPI *p_glVertexAttrib4sARB)( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w );
        void       (WINE_GLAPI *p_glVertexAttrib4sNV)( GLuint index, GLshort x, GLshort y, GLshort z, GLshort w );
        void       (WINE_GLAPI *p_glVertexAttrib4sv)( GLuint index, const GLshort *v );
        void       (WINE_GLAPI *p_glVertexAttrib4svARB)( GLuint index, const GLshort *v );
        void       (WINE_GLAPI *p_glVertexAttrib4svNV)( GLuint index, const GLshort *v );
        void       (WINE_GLAPI *p_glVertexAttrib4ubNV)( GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w );
        void       (WINE_GLAPI *p_glVertexAttrib4ubv)( GLuint index, const GLubyte *v );
        void       (WINE_GLAPI *p_glVertexAttrib4ubvARB)( GLuint index, const GLubyte *v );
        void       (WINE_GLAPI *p_glVertexAttrib4ubvNV)( GLuint index, const GLubyte *v );
        void       (WINE_GLAPI *p_glVertexAttrib4uiv)( GLuint index, const GLuint *v );
        void       (WINE_GLAPI *p_glVertexAttrib4uivARB)( GLuint index, const GLuint *v );
        void       (WINE_GLAPI *p_glVertexAttrib4usv)( GLuint index, const GLushort *v );
        void       (WINE_GLAPI *p_glVertexAttrib4usvARB)( GLuint index, const GLushort *v );
        void       (WINE_GLAPI *p_glVertexAttribArrayObjectATI)( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLuint buffer, GLuint offset );
        void       (WINE_GLAPI *p_glVertexAttribBinding)( GLuint attribindex, GLuint bindingindex );
        void       (WINE_GLAPI *p_glVertexAttribDivisor)( GLuint index, GLuint divisor );
        void       (WINE_GLAPI *p_glVertexAttribDivisorARB)( GLuint index, GLuint divisor );
        void       (WINE_GLAPI *p_glVertexAttribFormat)( GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset );
        void       (WINE_GLAPI *p_glVertexAttribFormatNV)( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride );
        void       (WINE_GLAPI *p_glVertexAttribI1i)( GLuint index, GLint x );
        void       (WINE_GLAPI *p_glVertexAttribI1iEXT)( GLuint index, GLint x );
        void       (WINE_GLAPI *p_glVertexAttribI1iv)( GLuint index, const GLint *v );
        void       (WINE_GLAPI *p_glVertexAttribI1ivEXT)( GLuint index, const GLint *v );
        void       (WINE_GLAPI *p_glVertexAttribI1ui)( GLuint index, GLuint x );
        void       (WINE_GLAPI *p_glVertexAttribI1uiEXT)( GLuint index, GLuint x );
        void       (WINE_GLAPI *p_glVertexAttribI1uiv)( GLuint index, const GLuint *v );
        void       (WINE_GLAPI *p_glVertexAttribI1uivEXT)( GLuint index, const GLuint *v );
        void       (WINE_GLAPI *p_glVertexAttribI2i)( GLuint index, GLint x, GLint y );
        void       (WINE_GLAPI *p_glVertexAttribI2iEXT)( GLuint index, GLint x, GLint y );
        void       (WINE_GLAPI *p_glVertexAttribI2iv)( GLuint index, const GLint *v );
        void       (WINE_GLAPI *p_glVertexAttribI2ivEXT)( GLuint index, const GLint *v );
        void       (WINE_GLAPI *p_glVertexAttribI2ui)( GLuint index, GLuint x, GLuint y );
        void       (WINE_GLAPI *p_glVertexAttribI2uiEXT)( GLuint index, GLuint x, GLuint y );
        void       (WINE_GLAPI *p_glVertexAttribI2uiv)( GLuint index, const GLuint *v );
        void       (WINE_GLAPI *p_glVertexAttribI2uivEXT)( GLuint index, const GLuint *v );
        void       (WINE_GLAPI *p_glVertexAttribI3i)( GLuint index, GLint x, GLint y, GLint z );
        void       (WINE_GLAPI *p_glVertexAttribI3iEXT)( GLuint index, GLint x, GLint y, GLint z );
        void       (WINE_GLAPI *p_glVertexAttribI3iv)( GLuint index, const GLint *v );
        void       (WINE_GLAPI *p_glVertexAttribI3ivEXT)( GLuint index, const GLint *v );
        void       (WINE_GLAPI *p_glVertexAttribI3ui)( GLuint index, GLuint x, GLuint y, GLuint z );
        void       (WINE_GLAPI *p_glVertexAttribI3uiEXT)( GLuint index, GLuint x, GLuint y, GLuint z );
        void       (WINE_GLAPI *p_glVertexAttribI3uiv)( GLuint index, const GLuint *v );
        void       (WINE_GLAPI *p_glVertexAttribI3uivEXT)( GLuint index, const GLuint *v );
        void       (WINE_GLAPI *p_glVertexAttribI4bv)( GLuint index, const GLbyte *v );
        void       (WINE_GLAPI *p_glVertexAttribI4bvEXT)( GLuint index, const GLbyte *v );
        void       (WINE_GLAPI *p_glVertexAttribI4i)( GLuint index, GLint x, GLint y, GLint z, GLint w );
        void       (WINE_GLAPI *p_glVertexAttribI4iEXT)( GLuint index, GLint x, GLint y, GLint z, GLint w );
        void       (WINE_GLAPI *p_glVertexAttribI4iv)( GLuint index, const GLint *v );
        void       (WINE_GLAPI *p_glVertexAttribI4ivEXT)( GLuint index, const GLint *v );
        void       (WINE_GLAPI *p_glVertexAttribI4sv)( GLuint index, const GLshort *v );
        void       (WINE_GLAPI *p_glVertexAttribI4svEXT)( GLuint index, const GLshort *v );
        void       (WINE_GLAPI *p_glVertexAttribI4ubv)( GLuint index, const GLubyte *v );
        void       (WINE_GLAPI *p_glVertexAttribI4ubvEXT)( GLuint index, const GLubyte *v );
        void       (WINE_GLAPI *p_glVertexAttribI4ui)( GLuint index, GLuint x, GLuint y, GLuint z, GLuint w );
        void       (WINE_GLAPI *p_glVertexAttribI4uiEXT)( GLuint index, GLuint x, GLuint y, GLuint z, GLuint w );
        void       (WINE_GLAPI *p_glVertexAttribI4uiv)( GLuint index, const GLuint *v );
        void       (WINE_GLAPI *p_glVertexAttribI4uivEXT)( GLuint index, const GLuint *v );
        void       (WINE_GLAPI *p_glVertexAttribI4usv)( GLuint index, const GLushort *v );
        void       (WINE_GLAPI *p_glVertexAttribI4usvEXT)( GLuint index, const GLushort *v );
        void       (WINE_GLAPI *p_glVertexAttribIFormat)( GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset );
        void       (WINE_GLAPI *p_glVertexAttribIFormatNV)( GLuint index, GLint size, GLenum type, GLsizei stride );
        void       (WINE_GLAPI *p_glVertexAttribIPointer)( GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer );
        void       (WINE_GLAPI *p_glVertexAttribIPointerEXT)( GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer );
        void       (WINE_GLAPI *p_glVertexAttribL1d)( GLuint index, GLdouble x );
        void       (WINE_GLAPI *p_glVertexAttribL1dEXT)( GLuint index, GLdouble x );
        void       (WINE_GLAPI *p_glVertexAttribL1dv)( GLuint index, const GLdouble *v );
        void       (WINE_GLAPI *p_glVertexAttribL1dvEXT)( GLuint index, const GLdouble *v );
        void       (WINE_GLAPI *p_glVertexAttribL1i64NV)( GLuint index, GLint64EXT x );
        void       (WINE_GLAPI *p_glVertexAttribL1i64vNV)( GLuint index, const GLint64EXT *v );
        void       (WINE_GLAPI *p_glVertexAttribL1ui64ARB)( GLuint index, GLuint64EXT x );
        void       (WINE_GLAPI *p_glVertexAttribL1ui64NV)( GLuint index, GLuint64EXT x );
        void       (WINE_GLAPI *p_glVertexAttribL1ui64vARB)( GLuint index, const GLuint64EXT *v );
        void       (WINE_GLAPI *p_glVertexAttribL1ui64vNV)( GLuint index, const GLuint64EXT *v );
        void       (WINE_GLAPI *p_glVertexAttribL2d)( GLuint index, GLdouble x, GLdouble y );
        void       (WINE_GLAPI *p_glVertexAttribL2dEXT)( GLuint index, GLdouble x, GLdouble y );
        void       (WINE_GLAPI *p_glVertexAttribL2dv)( GLuint index, const GLdouble *v );
        void       (WINE_GLAPI *p_glVertexAttribL2dvEXT)( GLuint index, const GLdouble *v );
        void       (WINE_GLAPI *p_glVertexAttribL2i64NV)( GLuint index, GLint64EXT x, GLint64EXT y );
        void       (WINE_GLAPI *p_glVertexAttribL2i64vNV)( GLuint index, const GLint64EXT *v );
        void       (WINE_GLAPI *p_glVertexAttribL2ui64NV)( GLuint index, GLuint64EXT x, GLuint64EXT y );
        void       (WINE_GLAPI *p_glVertexAttribL2ui64vNV)( GLuint index, const GLuint64EXT *v );
        void       (WINE_GLAPI *p_glVertexAttribL3d)( GLuint index, GLdouble x, GLdouble y, GLdouble z );
        void       (WINE_GLAPI *p_glVertexAttribL3dEXT)( GLuint index, GLdouble x, GLdouble y, GLdouble z );
        void       (WINE_GLAPI *p_glVertexAttribL3dv)( GLuint index, const GLdouble *v );
        void       (WINE_GLAPI *p_glVertexAttribL3dvEXT)( GLuint index, const GLdouble *v );
        void       (WINE_GLAPI *p_glVertexAttribL3i64NV)( GLuint index, GLint64EXT x, GLint64EXT y, GLint64EXT z );
        void       (WINE_GLAPI *p_glVertexAttribL3i64vNV)( GLuint index, const GLint64EXT *v );
        void       (WINE_GLAPI *p_glVertexAttribL3ui64NV)( GLuint index, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z );
        void       (WINE_GLAPI *p_glVertexAttribL3ui64vNV)( GLuint index, const GLuint64EXT *v );
        void       (WINE_GLAPI *p_glVertexAttribL4d)( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
        void       (WINE_GLAPI *p_glVertexAttribL4dEXT)( GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
        void       (WINE_GLAPI *p_glVertexAttribL4dv)( GLuint index, const GLdouble *v );
        void       (WINE_GLAPI *p_glVertexAttribL4dvEXT)( GLuint index, const GLdouble *v );
        void       (WINE_GLAPI *p_glVertexAttribL4i64NV)( GLuint index, GLint64EXT x, GLint64EXT y, GLint64EXT z, GLint64EXT w );
        void       (WINE_GLAPI *p_glVertexAttribL4i64vNV)( GLuint index, const GLint64EXT *v );
        void       (WINE_GLAPI *p_glVertexAttribL4ui64NV)( GLuint index, GLuint64EXT x, GLuint64EXT y, GLuint64EXT z, GLuint64EXT w );
        void       (WINE_GLAPI *p_glVertexAttribL4ui64vNV)( GLuint index, const GLuint64EXT *v );
        void       (WINE_GLAPI *p_glVertexAttribLFormat)( GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset );
        void       (WINE_GLAPI *p_glVertexAttribLFormatNV)( GLuint index, GLint size, GLenum type, GLsizei stride );
        void       (WINE_GLAPI *p_glVertexAttribLPointer)( GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer );
        void       (WINE_GLAPI *p_glVertexAttribLPointerEXT)( GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer );
        void       (WINE_GLAPI *p_glVertexAttribP1ui)( GLuint index, GLenum type, GLboolean normalized, GLuint value );
        void       (WINE_GLAPI *p_glVertexAttribP1uiv)( GLuint index, GLenum type, GLboolean normalized, const GLuint *value );
        void       (WINE_GLAPI *p_glVertexAttribP2ui)( GLuint index, GLenum type, GLboolean normalized, GLuint value );
        void       (WINE_GLAPI *p_glVertexAttribP2uiv)( GLuint index, GLenum type, GLboolean normalized, const GLuint *value );
        void       (WINE_GLAPI *p_glVertexAttribP3ui)( GLuint index, GLenum type, GLboolean normalized, GLuint value );
        void       (WINE_GLAPI *p_glVertexAttribP3uiv)( GLuint index, GLenum type, GLboolean normalized, const GLuint *value );
        void       (WINE_GLAPI *p_glVertexAttribP4ui)( GLuint index, GLenum type, GLboolean normalized, GLuint value );
        void       (WINE_GLAPI *p_glVertexAttribP4uiv)( GLuint index, GLenum type, GLboolean normalized, const GLuint *value );
        void       (WINE_GLAPI *p_glVertexAttribParameteriAMD)( GLuint index, GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glVertexAttribPointer)( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer );
        void       (WINE_GLAPI *p_glVertexAttribPointerARB)( GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer );
        void       (WINE_GLAPI *p_glVertexAttribPointerNV)( GLuint index, GLint fsize, GLenum type, GLsizei stride, const void *pointer );
        void       (WINE_GLAPI *p_glVertexAttribs1dvNV)( GLuint index, GLsizei count, const GLdouble *v );
        void       (WINE_GLAPI *p_glVertexAttribs1fvNV)( GLuint index, GLsizei count, const GLfloat *v );
        void       (WINE_GLAPI *p_glVertexAttribs1hvNV)( GLuint index, GLsizei n, const GLhalfNV *v );
        void       (WINE_GLAPI *p_glVertexAttribs1svNV)( GLuint index, GLsizei count, const GLshort *v );
        void       (WINE_GLAPI *p_glVertexAttribs2dvNV)( GLuint index, GLsizei count, const GLdouble *v );
        void       (WINE_GLAPI *p_glVertexAttribs2fvNV)( GLuint index, GLsizei count, const GLfloat *v );
        void       (WINE_GLAPI *p_glVertexAttribs2hvNV)( GLuint index, GLsizei n, const GLhalfNV *v );
        void       (WINE_GLAPI *p_glVertexAttribs2svNV)( GLuint index, GLsizei count, const GLshort *v );
        void       (WINE_GLAPI *p_glVertexAttribs3dvNV)( GLuint index, GLsizei count, const GLdouble *v );
        void       (WINE_GLAPI *p_glVertexAttribs3fvNV)( GLuint index, GLsizei count, const GLfloat *v );
        void       (WINE_GLAPI *p_glVertexAttribs3hvNV)( GLuint index, GLsizei n, const GLhalfNV *v );
        void       (WINE_GLAPI *p_glVertexAttribs3svNV)( GLuint index, GLsizei count, const GLshort *v );
        void       (WINE_GLAPI *p_glVertexAttribs4dvNV)( GLuint index, GLsizei count, const GLdouble *v );
        void       (WINE_GLAPI *p_glVertexAttribs4fvNV)( GLuint index, GLsizei count, const GLfloat *v );
        void       (WINE_GLAPI *p_glVertexAttribs4hvNV)( GLuint index, GLsizei n, const GLhalfNV *v );
        void       (WINE_GLAPI *p_glVertexAttribs4svNV)( GLuint index, GLsizei count, const GLshort *v );
        void       (WINE_GLAPI *p_glVertexAttribs4ubvNV)( GLuint index, GLsizei count, const GLubyte *v );
        void       (WINE_GLAPI *p_glVertexBindingDivisor)( GLuint bindingindex, GLuint divisor );
        void       (WINE_GLAPI *p_glVertexBlendARB)( GLint count );
        void       (WINE_GLAPI *p_glVertexBlendEnvfATI)( GLenum pname, GLfloat param );
        void       (WINE_GLAPI *p_glVertexBlendEnviATI)( GLenum pname, GLint param );
        void       (WINE_GLAPI *p_glVertexFormatNV)( GLint size, GLenum type, GLsizei stride );
        void       (WINE_GLAPI *p_glVertexP2ui)( GLenum type, GLuint value );
        void       (WINE_GLAPI *p_glVertexP2uiv)( GLenum type, const GLuint *value );
        void       (WINE_GLAPI *p_glVertexP3ui)( GLenum type, GLuint value );
        void       (WINE_GLAPI *p_glVertexP3uiv)( GLenum type, const GLuint *value );
        void       (WINE_GLAPI *p_glVertexP4ui)( GLenum type, GLuint value );
        void       (WINE_GLAPI *p_glVertexP4uiv)( GLenum type, const GLuint *value );
        void       (WINE_GLAPI *p_glVertexPointerEXT)( GLint size, GLenum type, GLsizei stride, GLsizei count, const void *pointer );
        void       (WINE_GLAPI *p_glVertexPointerListIBM)( GLint size, GLenum type, GLint stride, const void **pointer, GLint ptrstride );
        void       (WINE_GLAPI *p_glVertexPointervINTEL)( GLint size, GLenum type, const void **pointer );
        void       (WINE_GLAPI *p_glVertexStream1dATI)( GLenum stream, GLdouble x );
        void       (WINE_GLAPI *p_glVertexStream1dvATI)( GLenum stream, const GLdouble *coords );
        void       (WINE_GLAPI *p_glVertexStream1fATI)( GLenum stream, GLfloat x );
        void       (WINE_GLAPI *p_glVertexStream1fvATI)( GLenum stream, const GLfloat *coords );
        void       (WINE_GLAPI *p_glVertexStream1iATI)( GLenum stream, GLint x );
        void       (WINE_GLAPI *p_glVertexStream1ivATI)( GLenum stream, const GLint *coords );
        void       (WINE_GLAPI *p_glVertexStream1sATI)( GLenum stream, GLshort x );
        void       (WINE_GLAPI *p_glVertexStream1svATI)( GLenum stream, const GLshort *coords );
        void       (WINE_GLAPI *p_glVertexStream2dATI)( GLenum stream, GLdouble x, GLdouble y );
        void       (WINE_GLAPI *p_glVertexStream2dvATI)( GLenum stream, const GLdouble *coords );
        void       (WINE_GLAPI *p_glVertexStream2fATI)( GLenum stream, GLfloat x, GLfloat y );
        void       (WINE_GLAPI *p_glVertexStream2fvATI)( GLenum stream, const GLfloat *coords );
        void       (WINE_GLAPI *p_glVertexStream2iATI)( GLenum stream, GLint x, GLint y );
        void       (WINE_GLAPI *p_glVertexStream2ivATI)( GLenum stream, const GLint *coords );
        void       (WINE_GLAPI *p_glVertexStream2sATI)( GLenum stream, GLshort x, GLshort y );
        void       (WINE_GLAPI *p_glVertexStream2svATI)( GLenum stream, const GLshort *coords );
        void       (WINE_GLAPI *p_glVertexStream3dATI)( GLenum stream, GLdouble x, GLdouble y, GLdouble z );
        void       (WINE_GLAPI *p_glVertexStream3dvATI)( GLenum stream, const GLdouble *coords );
        void       (WINE_GLAPI *p_glVertexStream3fATI)( GLenum stream, GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glVertexStream3fvATI)( GLenum stream, const GLfloat *coords );
        void       (WINE_GLAPI *p_glVertexStream3iATI)( GLenum stream, GLint x, GLint y, GLint z );
        void       (WINE_GLAPI *p_glVertexStream3ivATI)( GLenum stream, const GLint *coords );
        void       (WINE_GLAPI *p_glVertexStream3sATI)( GLenum stream, GLshort x, GLshort y, GLshort z );
        void       (WINE_GLAPI *p_glVertexStream3svATI)( GLenum stream, const GLshort *coords );
        void       (WINE_GLAPI *p_glVertexStream4dATI)( GLenum stream, GLdouble x, GLdouble y, GLdouble z, GLdouble w );
        void       (WINE_GLAPI *p_glVertexStream4dvATI)( GLenum stream, const GLdouble *coords );
        void       (WINE_GLAPI *p_glVertexStream4fATI)( GLenum stream, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
        void       (WINE_GLAPI *p_glVertexStream4fvATI)( GLenum stream, const GLfloat *coords );
        void       (WINE_GLAPI *p_glVertexStream4iATI)( GLenum stream, GLint x, GLint y, GLint z, GLint w );
        void       (WINE_GLAPI *p_glVertexStream4ivATI)( GLenum stream, const GLint *coords );
        void       (WINE_GLAPI *p_glVertexStream4sATI)( GLenum stream, GLshort x, GLshort y, GLshort z, GLshort w );
        void       (WINE_GLAPI *p_glVertexStream4svATI)( GLenum stream, const GLshort *coords );
        void       (WINE_GLAPI *p_glVertexWeightPointerEXT)( GLint size, GLenum type, GLsizei stride, const void *pointer );
        void       (WINE_GLAPI *p_glVertexWeightfEXT)( GLfloat weight );
        void       (WINE_GLAPI *p_glVertexWeightfvEXT)( const GLfloat *weight );
        void       (WINE_GLAPI *p_glVertexWeighthNV)( GLhalfNV weight );
        void       (WINE_GLAPI *p_glVertexWeighthvNV)( const GLhalfNV *weight );
        GLenum     (WINE_GLAPI *p_glVideoCaptureNV)( GLuint video_capture_slot, GLuint *sequence_num, GLuint64EXT *capture_time );
        void       (WINE_GLAPI *p_glVideoCaptureStreamParameterdvNV)( GLuint video_capture_slot, GLuint stream, GLenum pname, const GLdouble *params );
        void       (WINE_GLAPI *p_glVideoCaptureStreamParameterfvNV)( GLuint video_capture_slot, GLuint stream, GLenum pname, const GLfloat *params );
        void       (WINE_GLAPI *p_glVideoCaptureStreamParameterivNV)( GLuint video_capture_slot, GLuint stream, GLenum pname, const GLint *params );
        void       (WINE_GLAPI *p_glViewportArrayv)( GLuint first, GLsizei count, const GLfloat *v );
        void       (WINE_GLAPI *p_glViewportIndexedf)( GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h );
        void       (WINE_GLAPI *p_glViewportIndexedfv)( GLuint index, const GLfloat *v );
        void       (WINE_GLAPI *p_glViewportPositionWScaleNV)( GLuint index, GLfloat xcoeff, GLfloat ycoeff );
        void       (WINE_GLAPI *p_glViewportSwizzleNV)( GLuint index, GLenum swizzlex, GLenum swizzley, GLenum swizzlez, GLenum swizzlew );
        void       (WINE_GLAPI *p_glWaitSemaphoreEXT)( GLuint semaphore, GLuint numBufferBarriers, const GLuint *buffers, GLuint numTextureBarriers, const GLuint *textures, const GLenum *srcLayouts );
        void       (WINE_GLAPI *p_glWaitSemaphoreui64NVX)( GLuint waitGpu, GLsizei fenceObjectCount, const GLuint *semaphoreArray, const GLuint64 *fenceValueArray );
        void       (WINE_GLAPI *p_glWaitSync)( GLsync sync, GLbitfield flags, GLuint64 timeout );
        void       (WINE_GLAPI *p_glWaitVkSemaphoreNV)( GLuint64 vkSemaphore );
        void       (WINE_GLAPI *p_glWeightPathsNV)( GLuint resultPath, GLsizei numPaths, const GLuint *paths, const GLfloat *weights );
        void       (WINE_GLAPI *p_glWeightPointerARB)( GLint size, GLenum type, GLsizei stride, const void *pointer );
        void       (WINE_GLAPI *p_glWeightbvARB)( GLint size, const GLbyte *weights );
        void       (WINE_GLAPI *p_glWeightdvARB)( GLint size, const GLdouble *weights );
        void       (WINE_GLAPI *p_glWeightfvARB)( GLint size, const GLfloat *weights );
        void       (WINE_GLAPI *p_glWeightivARB)( GLint size, const GLint *weights );
        void       (WINE_GLAPI *p_glWeightsvARB)( GLint size, const GLshort *weights );
        void       (WINE_GLAPI *p_glWeightubvARB)( GLint size, const GLubyte *weights );
        void       (WINE_GLAPI *p_glWeightuivARB)( GLint size, const GLuint *weights );
        void       (WINE_GLAPI *p_glWeightusvARB)( GLint size, const GLushort *weights );
        void       (WINE_GLAPI *p_glWindowPos2d)( GLdouble x, GLdouble y );
        void       (WINE_GLAPI *p_glWindowPos2dARB)( GLdouble x, GLdouble y );
        void       (WINE_GLAPI *p_glWindowPos2dMESA)( GLdouble x, GLdouble y );
        void       (WINE_GLAPI *p_glWindowPos2dv)( const GLdouble *v );
        void       (WINE_GLAPI *p_glWindowPos2dvARB)( const GLdouble *v );
        void       (WINE_GLAPI *p_glWindowPos2dvMESA)( const GLdouble *v );
        void       (WINE_GLAPI *p_glWindowPos2f)( GLfloat x, GLfloat y );
        void       (WINE_GLAPI *p_glWindowPos2fARB)( GLfloat x, GLfloat y );
        void       (WINE_GLAPI *p_glWindowPos2fMESA)( GLfloat x, GLfloat y );
        void       (WINE_GLAPI *p_glWindowPos2fv)( const GLfloat *v );
        void       (WINE_GLAPI *p_glWindowPos2fvARB)( const GLfloat *v );
        void       (WINE_GLAPI *p_glWindowPos2fvMESA)( const GLfloat *v );
        void       (WINE_GLAPI *p_glWindowPos2i)( GLint x, GLint y );
        void       (WINE_GLAPI *p_glWindowPos2iARB)( GLint x, GLint y );
        void       (WINE_GLAPI *p_glWindowPos2iMESA)( GLint x, GLint y );
        void       (WINE_GLAPI *p_glWindowPos2iv)( const GLint *v );
        void       (WINE_GLAPI *p_glWindowPos2ivARB)( const GLint *v );
        void       (WINE_GLAPI *p_glWindowPos2ivMESA)( const GLint *v );
        void       (WINE_GLAPI *p_glWindowPos2s)( GLshort x, GLshort y );
        void       (WINE_GLAPI *p_glWindowPos2sARB)( GLshort x, GLshort y );
        void       (WINE_GLAPI *p_glWindowPos2sMESA)( GLshort x, GLshort y );
        void       (WINE_GLAPI *p_glWindowPos2sv)( const GLshort *v );
        void       (WINE_GLAPI *p_glWindowPos2svARB)( const GLshort *v );
        void       (WINE_GLAPI *p_glWindowPos2svMESA)( const GLshort *v );
        void       (WINE_GLAPI *p_glWindowPos3d)( GLdouble x, GLdouble y, GLdouble z );
        void       (WINE_GLAPI *p_glWindowPos3dARB)( GLdouble x, GLdouble y, GLdouble z );
        void       (WINE_GLAPI *p_glWindowPos3dMESA)( GLdouble x, GLdouble y, GLdouble z );
        void       (WINE_GLAPI *p_glWindowPos3dv)( const GLdouble *v );
        void       (WINE_GLAPI *p_glWindowPos3dvARB)( const GLdouble *v );
        void       (WINE_GLAPI *p_glWindowPos3dvMESA)( const GLdouble *v );
        void       (WINE_GLAPI *p_glWindowPos3f)( GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glWindowPos3fARB)( GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glWindowPos3fMESA)( GLfloat x, GLfloat y, GLfloat z );
        void       (WINE_GLAPI *p_glWindowPos3fv)( const GLfloat *v );
        void       (WINE_GLAPI *p_glWindowPos3fvARB)( const GLfloat *v );
        void       (WINE_GLAPI *p_glWindowPos3fvMESA)( const GLfloat *v );
        void       (WINE_GLAPI *p_glWindowPos3i)( GLint x, GLint y, GLint z );
        void       (WINE_GLAPI *p_glWindowPos3iARB)( GLint x, GLint y, GLint z );
        void       (WINE_GLAPI *p_glWindowPos3iMESA)( GLint x, GLint y, GLint z );
        void       (WINE_GLAPI *p_glWindowPos3iv)( const GLint *v );
        void       (WINE_GLAPI *p_glWindowPos3ivARB)( const GLint *v );
        void       (WINE_GLAPI *p_glWindowPos3ivMESA)( const GLint *v );
        void       (WINE_GLAPI *p_glWindowPos3s)( GLshort x, GLshort y, GLshort z );
        void       (WINE_GLAPI *p_glWindowPos3sARB)( GLshort x, GLshort y, GLshort z );
        void       (WINE_GLAPI *p_glWindowPos3sMESA)( GLshort x, GLshort y, GLshort z );
        void       (WINE_GLAPI *p_glWindowPos3sv)( const GLshort *v );
        void       (WINE_GLAPI *p_glWindowPos3svARB)( const GLshort *v );
        void       (WINE_GLAPI *p_glWindowPos3svMESA)( const GLshort *v );
        void       (WINE_GLAPI *p_glWindowPos4dMESA)( GLdouble x, GLdouble y, GLdouble z, GLdouble w );
        void       (WINE_GLAPI *p_glWindowPos4dvMESA)( const GLdouble *v );
        void       (WINE_GLAPI *p_glWindowPos4fMESA)( GLfloat x, GLfloat y, GLfloat z, GLfloat w );
        void       (WINE_GLAPI *p_glWindowPos4fvMESA)( const GLfloat *v );
        void       (WINE_GLAPI *p_glWindowPos4iMESA)( GLint x, GLint y, GLint z, GLint w );
        void       (WINE_GLAPI *p_glWindowPos4ivMESA)( const GLint *v );
        void       (WINE_GLAPI *p_glWindowPos4sMESA)( GLshort x, GLshort y, GLshort z, GLshort w );
        void       (WINE_GLAPI *p_glWindowPos4svMESA)( const GLshort *v );
        void       (WINE_GLAPI *p_glWindowRectanglesEXT)( GLenum mode, GLsizei count, const GLint *box );
        void       (WINE_GLAPI *p_glWriteMaskEXT)( GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW );
        void *     (WINE_GLAPI *p_wglAllocateMemoryNV)( GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority );
        BOOL       (WINE_GLAPI *p_wglBindTexImageARB)( struct wgl_pbuffer * hPbuffer, int iBuffer );
        BOOL       (WINE_GLAPI *p_wglChoosePixelFormatARB)( HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats );
        struct wgl_context * (WINE_GLAPI *p_wglCreateContextAttribsARB)( HDC hDC, struct wgl_context * hShareContext, const int *attribList );
        struct wgl_pbuffer * (WINE_GLAPI *p_wglCreatePbufferARB)( HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList );
        BOOL       (WINE_GLAPI *p_wglDestroyPbufferARB)( struct wgl_pbuffer * hPbuffer );
        void       (WINE_GLAPI *p_wglFreeMemoryNV)( void *pointer );
        HDC        (WINE_GLAPI *p_wglGetCurrentReadDCARB)(void);
        const char * (WINE_GLAPI *p_wglGetExtensionsStringARB)( HDC hdc );
        const char * (WINE_GLAPI *p_wglGetExtensionsStringEXT)(void);
        HDC        (WINE_GLAPI *p_wglGetPbufferDCARB)( struct wgl_pbuffer * hPbuffer );
        BOOL       (WINE_GLAPI *p_wglGetPixelFormatAttribfvARB)( HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues );
        BOOL       (WINE_GLAPI *p_wglGetPixelFormatAttribivARB)( HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues );
        int        (WINE_GLAPI *p_wglGetSwapIntervalEXT)(void);
        BOOL       (WINE_GLAPI *p_wglMakeContextCurrentARB)( HDC hDrawDC, HDC hReadDC, struct wgl_context * hglrc );
        BOOL       (WINE_GLAPI *p_wglQueryCurrentRendererIntegerWINE)( GLenum attribute, GLuint *value );
        const GLchar * (WINE_GLAPI *p_wglQueryCurrentRendererStringWINE)( GLenum attribute );
        BOOL       (WINE_GLAPI *p_wglQueryPbufferARB)( struct wgl_pbuffer * hPbuffer, int iAttribute, int *piValue );
        BOOL       (WINE_GLAPI *p_wglQueryRendererIntegerWINE)( HDC dc, GLint renderer, GLenum attribute, GLuint *value );
        const GLchar * (WINE_GLAPI *p_wglQueryRendererStringWINE)( HDC dc, GLint renderer, GLenum attribute );
        int        (WINE_GLAPI *p_wglReleasePbufferDCARB)( struct wgl_pbuffer * hPbuffer, HDC hDC );
        BOOL       (WINE_GLAPI *p_wglReleaseTexImageARB)( struct wgl_pbuffer * hPbuffer, int iBuffer );
        BOOL       (WINE_GLAPI *p_wglSetPbufferAttribARB)( struct wgl_pbuffer * hPbuffer, const int *piAttribList );
        BOOL       (WINE_GLAPI *p_wglSetPixelFormatWINE)( HDC hdc, int format );
        BOOL       (WINE_GLAPI *p_wglSwapIntervalEXT)( int interval );
    } ext;
};

#define ALL_WGL_FUNCS \
    USE_GL_FUNC(glAccum) \
    USE_GL_FUNC(glAlphaFunc) \
    USE_GL_FUNC(glAreTexturesResident) \
    USE_GL_FUNC(glArrayElement) \
    USE_GL_FUNC(glBegin) \
    USE_GL_FUNC(glBindTexture) \
    USE_GL_FUNC(glBitmap) \
    USE_GL_FUNC(glBlendFunc) \
    USE_GL_FUNC(glCallList) \
    USE_GL_FUNC(glCallLists) \
    USE_GL_FUNC(glClear) \
    USE_GL_FUNC(glClearAccum) \
    USE_GL_FUNC(glClearColor) \
    USE_GL_FUNC(glClearDepth) \
    USE_GL_FUNC(glClearIndex) \
    USE_GL_FUNC(glClearStencil) \
    USE_GL_FUNC(glClipPlane) \
    USE_GL_FUNC(glColor3b) \
    USE_GL_FUNC(glColor3bv) \
    USE_GL_FUNC(glColor3d) \
    USE_GL_FUNC(glColor3dv) \
    USE_GL_FUNC(glColor3f) \
    USE_GL_FUNC(glColor3fv) \
    USE_GL_FUNC(glColor3i) \
    USE_GL_FUNC(glColor3iv) \
    USE_GL_FUNC(glColor3s) \
    USE_GL_FUNC(glColor3sv) \
    USE_GL_FUNC(glColor3ub) \
    USE_GL_FUNC(glColor3ubv) \
    USE_GL_FUNC(glColor3ui) \
    USE_GL_FUNC(glColor3uiv) \
    USE_GL_FUNC(glColor3us) \
    USE_GL_FUNC(glColor3usv) \
    USE_GL_FUNC(glColor4b) \
    USE_GL_FUNC(glColor4bv) \
    USE_GL_FUNC(glColor4d) \
    USE_GL_FUNC(glColor4dv) \
    USE_GL_FUNC(glColor4f) \
    USE_GL_FUNC(glColor4fv) \
    USE_GL_FUNC(glColor4i) \
    USE_GL_FUNC(glColor4iv) \
    USE_GL_FUNC(glColor4s) \
    USE_GL_FUNC(glColor4sv) \
    USE_GL_FUNC(glColor4ub) \
    USE_GL_FUNC(glColor4ubv) \
    USE_GL_FUNC(glColor4ui) \
    USE_GL_FUNC(glColor4uiv) \
    USE_GL_FUNC(glColor4us) \
    USE_GL_FUNC(glColor4usv) \
    USE_GL_FUNC(glColorMask) \
    USE_GL_FUNC(glColorMaterial) \
    USE_GL_FUNC(glColorPointer) \
    USE_GL_FUNC(glCopyPixels) \
    USE_GL_FUNC(glCopyTexImage1D) \
    USE_GL_FUNC(glCopyTexImage2D) \
    USE_GL_FUNC(glCopyTexSubImage1D) \
    USE_GL_FUNC(glCopyTexSubImage2D) \
    USE_GL_FUNC(glCullFace) \
    USE_GL_FUNC(glDeleteLists) \
    USE_GL_FUNC(glDeleteTextures) \
    USE_GL_FUNC(glDepthFunc) \
    USE_GL_FUNC(glDepthMask) \
    USE_GL_FUNC(glDepthRange) \
    USE_GL_FUNC(glDisable) \
    USE_GL_FUNC(glDisableClientState) \
    USE_GL_FUNC(glDrawArrays) \
    USE_GL_FUNC(glDrawBuffer) \
    USE_GL_FUNC(glDrawElements) \
    USE_GL_FUNC(glDrawPixels) \
    USE_GL_FUNC(glEdgeFlag) \
    USE_GL_FUNC(glEdgeFlagPointer) \
    USE_GL_FUNC(glEdgeFlagv) \
    USE_GL_FUNC(glEnable) \
    USE_GL_FUNC(glEnableClientState) \
    USE_GL_FUNC(glEnd) \
    USE_GL_FUNC(glEndList) \
    USE_GL_FUNC(glEvalCoord1d) \
    USE_GL_FUNC(glEvalCoord1dv) \
    USE_GL_FUNC(glEvalCoord1f) \
    USE_GL_FUNC(glEvalCoord1fv) \
    USE_GL_FUNC(glEvalCoord2d) \
    USE_GL_FUNC(glEvalCoord2dv) \
    USE_GL_FUNC(glEvalCoord2f) \
    USE_GL_FUNC(glEvalCoord2fv) \
    USE_GL_FUNC(glEvalMesh1) \
    USE_GL_FUNC(glEvalMesh2) \
    USE_GL_FUNC(glEvalPoint1) \
    USE_GL_FUNC(glEvalPoint2) \
    USE_GL_FUNC(glFeedbackBuffer) \
    USE_GL_FUNC(glFinish) \
    USE_GL_FUNC(glFlush) \
    USE_GL_FUNC(glFogf) \
    USE_GL_FUNC(glFogfv) \
    USE_GL_FUNC(glFogi) \
    USE_GL_FUNC(glFogiv) \
    USE_GL_FUNC(glFrontFace) \
    USE_GL_FUNC(glFrustum) \
    USE_GL_FUNC(glGenLists) \
    USE_GL_FUNC(glGenTextures) \
    USE_GL_FUNC(glGetBooleanv) \
    USE_GL_FUNC(glGetClipPlane) \
    USE_GL_FUNC(glGetDoublev) \
    USE_GL_FUNC(glGetError) \
    USE_GL_FUNC(glGetFloatv) \
    USE_GL_FUNC(glGetIntegerv) \
    USE_GL_FUNC(glGetLightfv) \
    USE_GL_FUNC(glGetLightiv) \
    USE_GL_FUNC(glGetMapdv) \
    USE_GL_FUNC(glGetMapfv) \
    USE_GL_FUNC(glGetMapiv) \
    USE_GL_FUNC(glGetMaterialfv) \
    USE_GL_FUNC(glGetMaterialiv) \
    USE_GL_FUNC(glGetPixelMapfv) \
    USE_GL_FUNC(glGetPixelMapuiv) \
    USE_GL_FUNC(glGetPixelMapusv) \
    USE_GL_FUNC(glGetPointerv) \
    USE_GL_FUNC(glGetPolygonStipple) \
    USE_GL_FUNC(glGetString) \
    USE_GL_FUNC(glGetTexEnvfv) \
    USE_GL_FUNC(glGetTexEnviv) \
    USE_GL_FUNC(glGetTexGendv) \
    USE_GL_FUNC(glGetTexGenfv) \
    USE_GL_FUNC(glGetTexGeniv) \
    USE_GL_FUNC(glGetTexImage) \
    USE_GL_FUNC(glGetTexLevelParameterfv) \
    USE_GL_FUNC(glGetTexLevelParameteriv) \
    USE_GL_FUNC(glGetTexParameterfv) \
    USE_GL_FUNC(glGetTexParameteriv) \
    USE_GL_FUNC(glHint) \
    USE_GL_FUNC(glIndexMask) \
    USE_GL_FUNC(glIndexPointer) \
    USE_GL_FUNC(glIndexd) \
    USE_GL_FUNC(glIndexdv) \
    USE_GL_FUNC(glIndexf) \
    USE_GL_FUNC(glIndexfv) \
    USE_GL_FUNC(glIndexi) \
    USE_GL_FUNC(glIndexiv) \
    USE_GL_FUNC(glIndexs) \
    USE_GL_FUNC(glIndexsv) \
    USE_GL_FUNC(glIndexub) \
    USE_GL_FUNC(glIndexubv) \
    USE_GL_FUNC(glInitNames) \
    USE_GL_FUNC(glInterleavedArrays) \
    USE_GL_FUNC(glIsEnabled) \
    USE_GL_FUNC(glIsList) \
    USE_GL_FUNC(glIsTexture) \
    USE_GL_FUNC(glLightModelf) \
    USE_GL_FUNC(glLightModelfv) \
    USE_GL_FUNC(glLightModeli) \
    USE_GL_FUNC(glLightModeliv) \
    USE_GL_FUNC(glLightf) \
    USE_GL_FUNC(glLightfv) \
    USE_GL_FUNC(glLighti) \
    USE_GL_FUNC(glLightiv) \
    USE_GL_FUNC(glLineStipple) \
    USE_GL_FUNC(glLineWidth) \
    USE_GL_FUNC(glListBase) \
    USE_GL_FUNC(glLoadIdentity) \
    USE_GL_FUNC(glLoadMatrixd) \
    USE_GL_FUNC(glLoadMatrixf) \
    USE_GL_FUNC(glLoadName) \
    USE_GL_FUNC(glLogicOp) \
    USE_GL_FUNC(glMap1d) \
    USE_GL_FUNC(glMap1f) \
    USE_GL_FUNC(glMap2d) \
    USE_GL_FUNC(glMap2f) \
    USE_GL_FUNC(glMapGrid1d) \
    USE_GL_FUNC(glMapGrid1f) \
    USE_GL_FUNC(glMapGrid2d) \
    USE_GL_FUNC(glMapGrid2f) \
    USE_GL_FUNC(glMaterialf) \
    USE_GL_FUNC(glMaterialfv) \
    USE_GL_FUNC(glMateriali) \
    USE_GL_FUNC(glMaterialiv) \
    USE_GL_FUNC(glMatrixMode) \
    USE_GL_FUNC(glMultMatrixd) \
    USE_GL_FUNC(glMultMatrixf) \
    USE_GL_FUNC(glNewList) \
    USE_GL_FUNC(glNormal3b) \
    USE_GL_FUNC(glNormal3bv) \
    USE_GL_FUNC(glNormal3d) \
    USE_GL_FUNC(glNormal3dv) \
    USE_GL_FUNC(glNormal3f) \
    USE_GL_FUNC(glNormal3fv) \
    USE_GL_FUNC(glNormal3i) \
    USE_GL_FUNC(glNormal3iv) \
    USE_GL_FUNC(glNormal3s) \
    USE_GL_FUNC(glNormal3sv) \
    USE_GL_FUNC(glNormalPointer) \
    USE_GL_FUNC(glOrtho) \
    USE_GL_FUNC(glPassThrough) \
    USE_GL_FUNC(glPixelMapfv) \
    USE_GL_FUNC(glPixelMapuiv) \
    USE_GL_FUNC(glPixelMapusv) \
    USE_GL_FUNC(glPixelStoref) \
    USE_GL_FUNC(glPixelStorei) \
    USE_GL_FUNC(glPixelTransferf) \
    USE_GL_FUNC(glPixelTransferi) \
    USE_GL_FUNC(glPixelZoom) \
    USE_GL_FUNC(glPointSize) \
    USE_GL_FUNC(glPolygonMode) \
    USE_GL_FUNC(glPolygonOffset) \
    USE_GL_FUNC(glPolygonStipple) \
    USE_GL_FUNC(glPopAttrib) \
    USE_GL_FUNC(glPopClientAttrib) \
    USE_GL_FUNC(glPopMatrix) \
    USE_GL_FUNC(glPopName) \
    USE_GL_FUNC(glPrioritizeTextures) \
    USE_GL_FUNC(glPushAttrib) \
    USE_GL_FUNC(glPushClientAttrib) \
    USE_GL_FUNC(glPushMatrix) \
    USE_GL_FUNC(glPushName) \
    USE_GL_FUNC(glRasterPos2d) \
    USE_GL_FUNC(glRasterPos2dv) \
    USE_GL_FUNC(glRasterPos2f) \
    USE_GL_FUNC(glRasterPos2fv) \
    USE_GL_FUNC(glRasterPos2i) \
    USE_GL_FUNC(glRasterPos2iv) \
    USE_GL_FUNC(glRasterPos2s) \
    USE_GL_FUNC(glRasterPos2sv) \
    USE_GL_FUNC(glRasterPos3d) \
    USE_GL_FUNC(glRasterPos3dv) \
    USE_GL_FUNC(glRasterPos3f) \
    USE_GL_FUNC(glRasterPos3fv) \
    USE_GL_FUNC(glRasterPos3i) \
    USE_GL_FUNC(glRasterPos3iv) \
    USE_GL_FUNC(glRasterPos3s) \
    USE_GL_FUNC(glRasterPos3sv) \
    USE_GL_FUNC(glRasterPos4d) \
    USE_GL_FUNC(glRasterPos4dv) \
    USE_GL_FUNC(glRasterPos4f) \
    USE_GL_FUNC(glRasterPos4fv) \
    USE_GL_FUNC(glRasterPos4i) \
    USE_GL_FUNC(glRasterPos4iv) \
    USE_GL_FUNC(glRasterPos4s) \
    USE_GL_FUNC(glRasterPos4sv) \
    USE_GL_FUNC(glReadBuffer) \
    USE_GL_FUNC(glReadPixels) \
    USE_GL_FUNC(glRectd) \
    USE_GL_FUNC(glRectdv) \
    USE_GL_FUNC(glRectf) \
    USE_GL_FUNC(glRectfv) \
    USE_GL_FUNC(glRecti) \
    USE_GL_FUNC(glRectiv) \
    USE_GL_FUNC(glRects) \
    USE_GL_FUNC(glRectsv) \
    USE_GL_FUNC(glRenderMode) \
    USE_GL_FUNC(glRotated) \
    USE_GL_FUNC(glRotatef) \
    USE_GL_FUNC(glScaled) \
    USE_GL_FUNC(glScalef) \
    USE_GL_FUNC(glScissor) \
    USE_GL_FUNC(glSelectBuffer) \
    USE_GL_FUNC(glShadeModel) \
    USE_GL_FUNC(glStencilFunc) \
    USE_GL_FUNC(glStencilMask) \
    USE_GL_FUNC(glStencilOp) \
    USE_GL_FUNC(glTexCoord1d) \
    USE_GL_FUNC(glTexCoord1dv) \
    USE_GL_FUNC(glTexCoord1f) \
    USE_GL_FUNC(glTexCoord1fv) \
    USE_GL_FUNC(glTexCoord1i) \
    USE_GL_FUNC(glTexCoord1iv) \
    USE_GL_FUNC(glTexCoord1s) \
    USE_GL_FUNC(glTexCoord1sv) \
    USE_GL_FUNC(glTexCoord2d) \
    USE_GL_FUNC(glTexCoord2dv) \
    USE_GL_FUNC(glTexCoord2f) \
    USE_GL_FUNC(glTexCoord2fv) \
    USE_GL_FUNC(glTexCoord2i) \
    USE_GL_FUNC(glTexCoord2iv) \
    USE_GL_FUNC(glTexCoord2s) \
    USE_GL_FUNC(glTexCoord2sv) \
    USE_GL_FUNC(glTexCoord3d) \
    USE_GL_FUNC(glTexCoord3dv) \
    USE_GL_FUNC(glTexCoord3f) \
    USE_GL_FUNC(glTexCoord3fv) \
    USE_GL_FUNC(glTexCoord3i) \
    USE_GL_FUNC(glTexCoord3iv) \
    USE_GL_FUNC(glTexCoord3s) \
    USE_GL_FUNC(glTexCoord3sv) \
    USE_GL_FUNC(glTexCoord4d) \
    USE_GL_FUNC(glTexCoord4dv) \
    USE_GL_FUNC(glTexCoord4f) \
    USE_GL_FUNC(glTexCoord4fv) \
    USE_GL_FUNC(glTexCoord4i) \
    USE_GL_FUNC(glTexCoord4iv) \
    USE_GL_FUNC(glTexCoord4s) \
    USE_GL_FUNC(glTexCoord4sv) \
    USE_GL_FUNC(glTexCoordPointer) \
    USE_GL_FUNC(glTexEnvf) \
    USE_GL_FUNC(glTexEnvfv) \
    USE_GL_FUNC(glTexEnvi) \
    USE_GL_FUNC(glTexEnviv) \
    USE_GL_FUNC(glTexGend) \
    USE_GL_FUNC(glTexGendv) \
    USE_GL_FUNC(glTexGenf) \
    USE_GL_FUNC(glTexGenfv) \
    USE_GL_FUNC(glTexGeni) \
    USE_GL_FUNC(glTexGeniv) \
    USE_GL_FUNC(glTexImage1D) \
    USE_GL_FUNC(glTexImage2D) \
    USE_GL_FUNC(glTexParameterf) \
    USE_GL_FUNC(glTexParameterfv) \
    USE_GL_FUNC(glTexParameteri) \
    USE_GL_FUNC(glTexParameteriv) \
    USE_GL_FUNC(glTexSubImage1D) \
    USE_GL_FUNC(glTexSubImage2D) \
    USE_GL_FUNC(glTranslated) \
    USE_GL_FUNC(glTranslatef) \
    USE_GL_FUNC(glVertex2d) \
    USE_GL_FUNC(glVertex2dv) \
    USE_GL_FUNC(glVertex2f) \
    USE_GL_FUNC(glVertex2fv) \
    USE_GL_FUNC(glVertex2i) \
    USE_GL_FUNC(glVertex2iv) \
    USE_GL_FUNC(glVertex2s) \
    USE_GL_FUNC(glVertex2sv) \
    USE_GL_FUNC(glVertex3d) \
    USE_GL_FUNC(glVertex3dv) \
    USE_GL_FUNC(glVertex3f) \
    USE_GL_FUNC(glVertex3fv) \
    USE_GL_FUNC(glVertex3i) \
    USE_GL_FUNC(glVertex3iv) \
    USE_GL_FUNC(glVertex3s) \
    USE_GL_FUNC(glVertex3sv) \
    USE_GL_FUNC(glVertex4d) \
    USE_GL_FUNC(glVertex4dv) \
    USE_GL_FUNC(glVertex4f) \
    USE_GL_FUNC(glVertex4fv) \
    USE_GL_FUNC(glVertex4i) \
    USE_GL_FUNC(glVertex4iv) \
    USE_GL_FUNC(glVertex4s) \
    USE_GL_FUNC(glVertex4sv) \
    USE_GL_FUNC(glVertexPointer) \
    USE_GL_FUNC(glViewport)

#endif /* __WINE_WGL_DRIVER_H */
