/* Automatically generated from http://www.opengl.org/registry files; DO NOT EDIT! */

#ifndef __WINE_WGL_DRIVER_H
#define __WINE_WGL_DRIVER_H

#ifndef WINE_GLAPI
#define WINE_GLAPI
#endif

#define WINE_WGL_DRIVER_VERSION 13

struct wgl_context;
struct wgl_pbuffer;

struct opengl_funcs
{
    struct
    {
        BOOL       (WINE_GLAPI *p_wglCopyContext)(struct wgl_context *,struct wgl_context *,UINT);
        struct wgl_context * (WINE_GLAPI *p_wglCreateContext)(HDC);
        void       (WINE_GLAPI *p_wglDeleteContext)(struct wgl_context *);
        INT        (WINE_GLAPI *p_wglDescribePixelFormat)(HDC,INT,UINT,PIXELFORMATDESCRIPTOR *);
        INT        (WINE_GLAPI *p_wglGetPixelFormat)(HDC);
        PROC       (WINE_GLAPI *p_wglGetProcAddress)(LPCSTR);
        BOOL       (WINE_GLAPI *p_wglMakeCurrent)(HDC,struct wgl_context *);
        BOOL       (WINE_GLAPI *p_wglSetPixelFormat)(HDC,INT,const PIXELFORMATDESCRIPTOR *);
        BOOL       (WINE_GLAPI *p_wglShareLists)(struct wgl_context *,struct wgl_context *);
        BOOL       (WINE_GLAPI *p_wglSwapBuffers)(HDC);
    } wgl;

    struct
    {
        void       (WINE_GLAPI *p_glAccum)(GLenum,GLfloat);
        void       (WINE_GLAPI *p_glAlphaFunc)(GLenum,GLfloat);
        GLboolean  (WINE_GLAPI *p_glAreTexturesResident)(GLsizei,const GLuint*,GLboolean*);
        void       (WINE_GLAPI *p_glArrayElement)(GLint);
        void       (WINE_GLAPI *p_glBegin)(GLenum);
        void       (WINE_GLAPI *p_glBindTexture)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glBitmap)(GLsizei,GLsizei,GLfloat,GLfloat,GLfloat,GLfloat,const GLubyte*);
        void       (WINE_GLAPI *p_glBlendFunc)(GLenum,GLenum);
        void       (WINE_GLAPI *p_glCallList)(GLuint);
        void       (WINE_GLAPI *p_glCallLists)(GLsizei,GLenum,const void*);
        void       (WINE_GLAPI *p_glClear)(GLbitfield);
        void       (WINE_GLAPI *p_glClearAccum)(GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glClearColor)(GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glClearDepth)(GLdouble);
        void       (WINE_GLAPI *p_glClearIndex)(GLfloat);
        void       (WINE_GLAPI *p_glClearStencil)(GLint);
        void       (WINE_GLAPI *p_glClipPlane)(GLenum,const GLdouble*);
        void       (WINE_GLAPI *p_glColor3b)(GLbyte,GLbyte,GLbyte);
        void       (WINE_GLAPI *p_glColor3bv)(const GLbyte*);
        void       (WINE_GLAPI *p_glColor3d)(GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glColor3dv)(const GLdouble*);
        void       (WINE_GLAPI *p_glColor3f)(GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glColor3fv)(const GLfloat*);
        void       (WINE_GLAPI *p_glColor3i)(GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glColor3iv)(const GLint*);
        void       (WINE_GLAPI *p_glColor3s)(GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glColor3sv)(const GLshort*);
        void       (WINE_GLAPI *p_glColor3ub)(GLubyte,GLubyte,GLubyte);
        void       (WINE_GLAPI *p_glColor3ubv)(const GLubyte*);
        void       (WINE_GLAPI *p_glColor3ui)(GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glColor3uiv)(const GLuint*);
        void       (WINE_GLAPI *p_glColor3us)(GLushort,GLushort,GLushort);
        void       (WINE_GLAPI *p_glColor3usv)(const GLushort*);
        void       (WINE_GLAPI *p_glColor4b)(GLbyte,GLbyte,GLbyte,GLbyte);
        void       (WINE_GLAPI *p_glColor4bv)(const GLbyte*);
        void       (WINE_GLAPI *p_glColor4d)(GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glColor4dv)(const GLdouble*);
        void       (WINE_GLAPI *p_glColor4f)(GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glColor4fv)(const GLfloat*);
        void       (WINE_GLAPI *p_glColor4i)(GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glColor4iv)(const GLint*);
        void       (WINE_GLAPI *p_glColor4s)(GLshort,GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glColor4sv)(const GLshort*);
        void       (WINE_GLAPI *p_glColor4ub)(GLubyte,GLubyte,GLubyte,GLubyte);
        void       (WINE_GLAPI *p_glColor4ubv)(const GLubyte*);
        void       (WINE_GLAPI *p_glColor4ui)(GLuint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glColor4uiv)(const GLuint*);
        void       (WINE_GLAPI *p_glColor4us)(GLushort,GLushort,GLushort,GLushort);
        void       (WINE_GLAPI *p_glColor4usv)(const GLushort*);
        void       (WINE_GLAPI *p_glColorMask)(GLboolean,GLboolean,GLboolean,GLboolean);
        void       (WINE_GLAPI *p_glColorMaterial)(GLenum,GLenum);
        void       (WINE_GLAPI *p_glColorPointer)(GLint,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCopyPixels)(GLint,GLint,GLsizei,GLsizei,GLenum);
        void       (WINE_GLAPI *p_glCopyTexImage1D)(GLenum,GLint,GLenum,GLint,GLint,GLsizei,GLint);
        void       (WINE_GLAPI *p_glCopyTexImage2D)(GLenum,GLint,GLenum,GLint,GLint,GLsizei,GLsizei,GLint);
        void       (WINE_GLAPI *p_glCopyTexSubImage1D)(GLenum,GLint,GLint,GLint,GLint,GLsizei);
        void       (WINE_GLAPI *p_glCopyTexSubImage2D)(GLenum,GLint,GLint,GLint,GLint,GLint,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glCullFace)(GLenum);
        void       (WINE_GLAPI *p_glDeleteLists)(GLuint,GLsizei);
        void       (WINE_GLAPI *p_glDeleteTextures)(GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glDepthFunc)(GLenum);
        void       (WINE_GLAPI *p_glDepthMask)(GLboolean);
        void       (WINE_GLAPI *p_glDepthRange)(GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glDisable)(GLenum);
        void       (WINE_GLAPI *p_glDisableClientState)(GLenum);
        void       (WINE_GLAPI *p_glDrawArrays)(GLenum,GLint,GLsizei);
        void       (WINE_GLAPI *p_glDrawBuffer)(GLenum);
        void       (WINE_GLAPI *p_glDrawElements)(GLenum,GLsizei,GLenum,const void*);
        void       (WINE_GLAPI *p_glDrawPixels)(GLsizei,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glEdgeFlag)(GLboolean);
        void       (WINE_GLAPI *p_glEdgeFlagPointer)(GLsizei,const void*);
        void       (WINE_GLAPI *p_glEdgeFlagv)(const GLboolean*);
        void       (WINE_GLAPI *p_glEnable)(GLenum);
        void       (WINE_GLAPI *p_glEnableClientState)(GLenum);
        void       (WINE_GLAPI *p_glEnd)(void);
        void       (WINE_GLAPI *p_glEndList)(void);
        void       (WINE_GLAPI *p_glEvalCoord1d)(GLdouble);
        void       (WINE_GLAPI *p_glEvalCoord1dv)(const GLdouble*);
        void       (WINE_GLAPI *p_glEvalCoord1f)(GLfloat);
        void       (WINE_GLAPI *p_glEvalCoord1fv)(const GLfloat*);
        void       (WINE_GLAPI *p_glEvalCoord2d)(GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glEvalCoord2dv)(const GLdouble*);
        void       (WINE_GLAPI *p_glEvalCoord2f)(GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glEvalCoord2fv)(const GLfloat*);
        void       (WINE_GLAPI *p_glEvalMesh1)(GLenum,GLint,GLint);
        void       (WINE_GLAPI *p_glEvalMesh2)(GLenum,GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glEvalPoint1)(GLint);
        void       (WINE_GLAPI *p_glEvalPoint2)(GLint,GLint);
        void       (WINE_GLAPI *p_glFeedbackBuffer)(GLsizei,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glFinish)(void);
        void       (WINE_GLAPI *p_glFlush)(void);
        void       (WINE_GLAPI *p_glFogf)(GLenum,GLfloat);
        void       (WINE_GLAPI *p_glFogfv)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glFogi)(GLenum,GLint);
        void       (WINE_GLAPI *p_glFogiv)(GLenum,const GLint*);
        void       (WINE_GLAPI *p_glFrontFace)(GLenum);
        void       (WINE_GLAPI *p_glFrustum)(GLdouble,GLdouble,GLdouble,GLdouble,GLdouble,GLdouble);
        GLuint     (WINE_GLAPI *p_glGenLists)(GLsizei);
        void       (WINE_GLAPI *p_glGenTextures)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glGetBooleanv)(GLenum,GLboolean*);
        void       (WINE_GLAPI *p_glGetClipPlane)(GLenum,GLdouble*);
        void       (WINE_GLAPI *p_glGetDoublev)(GLenum,GLdouble*);
        GLenum     (WINE_GLAPI *p_glGetError)(void);
        void       (WINE_GLAPI *p_glGetFloatv)(GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetIntegerv)(GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetLightfv)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetLightiv)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetMapdv)(GLenum,GLenum,GLdouble*);
        void       (WINE_GLAPI *p_glGetMapfv)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetMapiv)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetMaterialfv)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetMaterialiv)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetPixelMapfv)(GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetPixelMapuiv)(GLenum,GLuint*);
        void       (WINE_GLAPI *p_glGetPixelMapusv)(GLenum,GLushort*);
        void       (WINE_GLAPI *p_glGetPointerv)(GLenum,void**);
        void       (WINE_GLAPI *p_glGetPolygonStipple)(GLubyte*);
        const GLubyte* (WINE_GLAPI *p_glGetString)(GLenum);
        void       (WINE_GLAPI *p_glGetTexEnvfv)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetTexEnviv)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetTexGendv)(GLenum,GLenum,GLdouble*);
        void       (WINE_GLAPI *p_glGetTexGenfv)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetTexGeniv)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetTexImage)(GLenum,GLint,GLenum,GLenum,void*);
        void       (WINE_GLAPI *p_glGetTexLevelParameterfv)(GLenum,GLint,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetTexLevelParameteriv)(GLenum,GLint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetTexParameterfv)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetTexParameteriv)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glHint)(GLenum,GLenum);
        void       (WINE_GLAPI *p_glIndexMask)(GLuint);
        void       (WINE_GLAPI *p_glIndexPointer)(GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glIndexd)(GLdouble);
        void       (WINE_GLAPI *p_glIndexdv)(const GLdouble*);
        void       (WINE_GLAPI *p_glIndexf)(GLfloat);
        void       (WINE_GLAPI *p_glIndexfv)(const GLfloat*);
        void       (WINE_GLAPI *p_glIndexi)(GLint);
        void       (WINE_GLAPI *p_glIndexiv)(const GLint*);
        void       (WINE_GLAPI *p_glIndexs)(GLshort);
        void       (WINE_GLAPI *p_glIndexsv)(const GLshort*);
        void       (WINE_GLAPI *p_glIndexub)(GLubyte);
        void       (WINE_GLAPI *p_glIndexubv)(const GLubyte*);
        void       (WINE_GLAPI *p_glInitNames)(void);
        void       (WINE_GLAPI *p_glInterleavedArrays)(GLenum,GLsizei,const void*);
        GLboolean  (WINE_GLAPI *p_glIsEnabled)(GLenum);
        GLboolean  (WINE_GLAPI *p_glIsList)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsTexture)(GLuint);
        void       (WINE_GLAPI *p_glLightModelf)(GLenum,GLfloat);
        void       (WINE_GLAPI *p_glLightModelfv)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glLightModeli)(GLenum,GLint);
        void       (WINE_GLAPI *p_glLightModeliv)(GLenum,const GLint*);
        void       (WINE_GLAPI *p_glLightf)(GLenum,GLenum,GLfloat);
        void       (WINE_GLAPI *p_glLightfv)(GLenum,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glLighti)(GLenum,GLenum,GLint);
        void       (WINE_GLAPI *p_glLightiv)(GLenum,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glLineStipple)(GLint,GLushort);
        void       (WINE_GLAPI *p_glLineWidth)(GLfloat);
        void       (WINE_GLAPI *p_glListBase)(GLuint);
        void       (WINE_GLAPI *p_glLoadIdentity)(void);
        void       (WINE_GLAPI *p_glLoadMatrixd)(const GLdouble*);
        void       (WINE_GLAPI *p_glLoadMatrixf)(const GLfloat*);
        void       (WINE_GLAPI *p_glLoadName)(GLuint);
        void       (WINE_GLAPI *p_glLogicOp)(GLenum);
        void       (WINE_GLAPI *p_glMap1d)(GLenum,GLdouble,GLdouble,GLint,GLint,const GLdouble*);
        void       (WINE_GLAPI *p_glMap1f)(GLenum,GLfloat,GLfloat,GLint,GLint,const GLfloat*);
        void       (WINE_GLAPI *p_glMap2d)(GLenum,GLdouble,GLdouble,GLint,GLint,GLdouble,GLdouble,GLint,GLint,const GLdouble*);
        void       (WINE_GLAPI *p_glMap2f)(GLenum,GLfloat,GLfloat,GLint,GLint,GLfloat,GLfloat,GLint,GLint,const GLfloat*);
        void       (WINE_GLAPI *p_glMapGrid1d)(GLint,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glMapGrid1f)(GLint,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glMapGrid2d)(GLint,GLdouble,GLdouble,GLint,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glMapGrid2f)(GLint,GLfloat,GLfloat,GLint,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glMaterialf)(GLenum,GLenum,GLfloat);
        void       (WINE_GLAPI *p_glMaterialfv)(GLenum,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glMateriali)(GLenum,GLenum,GLint);
        void       (WINE_GLAPI *p_glMaterialiv)(GLenum,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glMatrixMode)(GLenum);
        void       (WINE_GLAPI *p_glMultMatrixd)(const GLdouble*);
        void       (WINE_GLAPI *p_glMultMatrixf)(const GLfloat*);
        void       (WINE_GLAPI *p_glNewList)(GLuint,GLenum);
        void       (WINE_GLAPI *p_glNormal3b)(GLbyte,GLbyte,GLbyte);
        void       (WINE_GLAPI *p_glNormal3bv)(const GLbyte*);
        void       (WINE_GLAPI *p_glNormal3d)(GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glNormal3dv)(const GLdouble*);
        void       (WINE_GLAPI *p_glNormal3f)(GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glNormal3fv)(const GLfloat*);
        void       (WINE_GLAPI *p_glNormal3i)(GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glNormal3iv)(const GLint*);
        void       (WINE_GLAPI *p_glNormal3s)(GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glNormal3sv)(const GLshort*);
        void       (WINE_GLAPI *p_glNormalPointer)(GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glOrtho)(GLdouble,GLdouble,GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glPassThrough)(GLfloat);
        void       (WINE_GLAPI *p_glPixelMapfv)(GLenum,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glPixelMapuiv)(GLenum,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glPixelMapusv)(GLenum,GLsizei,const GLushort*);
        void       (WINE_GLAPI *p_glPixelStoref)(GLenum,GLfloat);
        void       (WINE_GLAPI *p_glPixelStorei)(GLenum,GLint);
        void       (WINE_GLAPI *p_glPixelTransferf)(GLenum,GLfloat);
        void       (WINE_GLAPI *p_glPixelTransferi)(GLenum,GLint);
        void       (WINE_GLAPI *p_glPixelZoom)(GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glPointSize)(GLfloat);
        void       (WINE_GLAPI *p_glPolygonMode)(GLenum,GLenum);
        void       (WINE_GLAPI *p_glPolygonOffset)(GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glPolygonStipple)(const GLubyte*);
        void       (WINE_GLAPI *p_glPopAttrib)(void);
        void       (WINE_GLAPI *p_glPopClientAttrib)(void);
        void       (WINE_GLAPI *p_glPopMatrix)(void);
        void       (WINE_GLAPI *p_glPopName)(void);
        void       (WINE_GLAPI *p_glPrioritizeTextures)(GLsizei,const GLuint*,const GLfloat*);
        void       (WINE_GLAPI *p_glPushAttrib)(GLbitfield);
        void       (WINE_GLAPI *p_glPushClientAttrib)(GLbitfield);
        void       (WINE_GLAPI *p_glPushMatrix)(void);
        void       (WINE_GLAPI *p_glPushName)(GLuint);
        void       (WINE_GLAPI *p_glRasterPos2d)(GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glRasterPos2dv)(const GLdouble*);
        void       (WINE_GLAPI *p_glRasterPos2f)(GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glRasterPos2fv)(const GLfloat*);
        void       (WINE_GLAPI *p_glRasterPos2i)(GLint,GLint);
        void       (WINE_GLAPI *p_glRasterPos2iv)(const GLint*);
        void       (WINE_GLAPI *p_glRasterPos2s)(GLshort,GLshort);
        void       (WINE_GLAPI *p_glRasterPos2sv)(const GLshort*);
        void       (WINE_GLAPI *p_glRasterPos3d)(GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glRasterPos3dv)(const GLdouble*);
        void       (WINE_GLAPI *p_glRasterPos3f)(GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glRasterPos3fv)(const GLfloat*);
        void       (WINE_GLAPI *p_glRasterPos3i)(GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glRasterPos3iv)(const GLint*);
        void       (WINE_GLAPI *p_glRasterPos3s)(GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glRasterPos3sv)(const GLshort*);
        void       (WINE_GLAPI *p_glRasterPos4d)(GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glRasterPos4dv)(const GLdouble*);
        void       (WINE_GLAPI *p_glRasterPos4f)(GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glRasterPos4fv)(const GLfloat*);
        void       (WINE_GLAPI *p_glRasterPos4i)(GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glRasterPos4iv)(const GLint*);
        void       (WINE_GLAPI *p_glRasterPos4s)(GLshort,GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glRasterPos4sv)(const GLshort*);
        void       (WINE_GLAPI *p_glReadBuffer)(GLenum);
        void       (WINE_GLAPI *p_glReadPixels)(GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,void*);
        void       (WINE_GLAPI *p_glRectd)(GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glRectdv)(const GLdouble*,const GLdouble*);
        void       (WINE_GLAPI *p_glRectf)(GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glRectfv)(const GLfloat*,const GLfloat*);
        void       (WINE_GLAPI *p_glRecti)(GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glRectiv)(const GLint*,const GLint*);
        void       (WINE_GLAPI *p_glRects)(GLshort,GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glRectsv)(const GLshort*,const GLshort*);
        GLint      (WINE_GLAPI *p_glRenderMode)(GLenum);
        void       (WINE_GLAPI *p_glRotated)(GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glRotatef)(GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glScaled)(GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glScalef)(GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glScissor)(GLint,GLint,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glSelectBuffer)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glShadeModel)(GLenum);
        void       (WINE_GLAPI *p_glStencilFunc)(GLenum,GLint,GLuint);
        void       (WINE_GLAPI *p_glStencilMask)(GLuint);
        void       (WINE_GLAPI *p_glStencilOp)(GLenum,GLenum,GLenum);
        void       (WINE_GLAPI *p_glTexCoord1d)(GLdouble);
        void       (WINE_GLAPI *p_glTexCoord1dv)(const GLdouble*);
        void       (WINE_GLAPI *p_glTexCoord1f)(GLfloat);
        void       (WINE_GLAPI *p_glTexCoord1fv)(const GLfloat*);
        void       (WINE_GLAPI *p_glTexCoord1i)(GLint);
        void       (WINE_GLAPI *p_glTexCoord1iv)(const GLint*);
        void       (WINE_GLAPI *p_glTexCoord1s)(GLshort);
        void       (WINE_GLAPI *p_glTexCoord1sv)(const GLshort*);
        void       (WINE_GLAPI *p_glTexCoord2d)(GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glTexCoord2dv)(const GLdouble*);
        void       (WINE_GLAPI *p_glTexCoord2f)(GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glTexCoord2fv)(const GLfloat*);
        void       (WINE_GLAPI *p_glTexCoord2i)(GLint,GLint);
        void       (WINE_GLAPI *p_glTexCoord2iv)(const GLint*);
        void       (WINE_GLAPI *p_glTexCoord2s)(GLshort,GLshort);
        void       (WINE_GLAPI *p_glTexCoord2sv)(const GLshort*);
        void       (WINE_GLAPI *p_glTexCoord3d)(GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glTexCoord3dv)(const GLdouble*);
        void       (WINE_GLAPI *p_glTexCoord3f)(GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glTexCoord3fv)(const GLfloat*);
        void       (WINE_GLAPI *p_glTexCoord3i)(GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glTexCoord3iv)(const GLint*);
        void       (WINE_GLAPI *p_glTexCoord3s)(GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glTexCoord3sv)(const GLshort*);
        void       (WINE_GLAPI *p_glTexCoord4d)(GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glTexCoord4dv)(const GLdouble*);
        void       (WINE_GLAPI *p_glTexCoord4f)(GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glTexCoord4fv)(const GLfloat*);
        void       (WINE_GLAPI *p_glTexCoord4i)(GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glTexCoord4iv)(const GLint*);
        void       (WINE_GLAPI *p_glTexCoord4s)(GLshort,GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glTexCoord4sv)(const GLshort*);
        void       (WINE_GLAPI *p_glTexCoordPointer)(GLint,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glTexEnvf)(GLenum,GLenum,GLfloat);
        void       (WINE_GLAPI *p_glTexEnvfv)(GLenum,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glTexEnvi)(GLenum,GLenum,GLint);
        void       (WINE_GLAPI *p_glTexEnviv)(GLenum,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glTexGend)(GLenum,GLenum,GLdouble);
        void       (WINE_GLAPI *p_glTexGendv)(GLenum,GLenum,const GLdouble*);
        void       (WINE_GLAPI *p_glTexGenf)(GLenum,GLenum,GLfloat);
        void       (WINE_GLAPI *p_glTexGenfv)(GLenum,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glTexGeni)(GLenum,GLenum,GLint);
        void       (WINE_GLAPI *p_glTexGeniv)(GLenum,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glTexImage1D)(GLenum,GLint,GLint,GLsizei,GLint,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glTexImage2D)(GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glTexParameterf)(GLenum,GLenum,GLfloat);
        void       (WINE_GLAPI *p_glTexParameterfv)(GLenum,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glTexParameteri)(GLenum,GLenum,GLint);
        void       (WINE_GLAPI *p_glTexParameteriv)(GLenum,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glTexSubImage1D)(GLenum,GLint,GLint,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glTexSubImage2D)(GLenum,GLint,GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glTranslated)(GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glTranslatef)(GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glVertex2d)(GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glVertex2dv)(const GLdouble*);
        void       (WINE_GLAPI *p_glVertex2f)(GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glVertex2fv)(const GLfloat*);
        void       (WINE_GLAPI *p_glVertex2i)(GLint,GLint);
        void       (WINE_GLAPI *p_glVertex2iv)(const GLint*);
        void       (WINE_GLAPI *p_glVertex2s)(GLshort,GLshort);
        void       (WINE_GLAPI *p_glVertex2sv)(const GLshort*);
        void       (WINE_GLAPI *p_glVertex3d)(GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glVertex3dv)(const GLdouble*);
        void       (WINE_GLAPI *p_glVertex3f)(GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glVertex3fv)(const GLfloat*);
        void       (WINE_GLAPI *p_glVertex3i)(GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glVertex3iv)(const GLint*);
        void       (WINE_GLAPI *p_glVertex3s)(GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glVertex3sv)(const GLshort*);
        void       (WINE_GLAPI *p_glVertex4d)(GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glVertex4dv)(const GLdouble*);
        void       (WINE_GLAPI *p_glVertex4f)(GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glVertex4fv)(const GLfloat*);
        void       (WINE_GLAPI *p_glVertex4i)(GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glVertex4iv)(const GLint*);
        void       (WINE_GLAPI *p_glVertex4s)(GLshort,GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glVertex4sv)(const GLshort*);
        void       (WINE_GLAPI *p_glVertexPointer)(GLint,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glViewport)(GLint,GLint,GLsizei,GLsizei);
    } gl;

    struct
    {
        void       (WINE_GLAPI *p_glAccumxOES)(GLenum,GLfixed);
        void       (WINE_GLAPI *p_glActiveProgramEXT)(GLuint);
        void       (WINE_GLAPI *p_glActiveShaderProgram)(GLuint,GLuint);
        void       (WINE_GLAPI *p_glActiveStencilFaceEXT)(GLenum);
        void       (WINE_GLAPI *p_glActiveTexture)(GLenum);
        void       (WINE_GLAPI *p_glActiveTextureARB)(GLenum);
        void       (WINE_GLAPI *p_glActiveVaryingNV)(GLuint,const GLchar*);
        void       (WINE_GLAPI *p_glAlphaFragmentOp1ATI)(GLenum,GLuint,GLuint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glAlphaFragmentOp2ATI)(GLenum,GLuint,GLuint,GLuint,GLuint,GLuint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glAlphaFragmentOp3ATI)(GLenum,GLuint,GLuint,GLuint,GLuint,GLuint,GLuint,GLuint,GLuint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glAlphaFuncxOES)(GLenum,GLfixed);
        void       (WINE_GLAPI *p_glApplyTextureEXT)(GLenum);
        GLboolean  (WINE_GLAPI *p_glAreProgramsResidentNV)(GLsizei,const GLuint*,GLboolean*);
        GLboolean  (WINE_GLAPI *p_glAreTexturesResidentEXT)(GLsizei,const GLuint*,GLboolean*);
        void       (WINE_GLAPI *p_glArrayElementEXT)(GLint);
        void       (WINE_GLAPI *p_glArrayObjectATI)(GLenum,GLint,GLenum,GLsizei,GLuint,GLuint);
        void       (WINE_GLAPI *p_glAsyncMarkerSGIX)(GLuint);
        void       (WINE_GLAPI *p_glAttachObjectARB)(GLhandleARB,GLhandleARB);
        void       (WINE_GLAPI *p_glAttachShader)(GLuint,GLuint);
        void       (WINE_GLAPI *p_glBeginConditionalRender)(GLuint,GLenum);
        void       (WINE_GLAPI *p_glBeginConditionalRenderNV)(GLuint,GLenum);
        void       (WINE_GLAPI *p_glBeginConditionalRenderNVX)(GLuint);
        void       (WINE_GLAPI *p_glBeginFragmentShaderATI)(void);
        void       (WINE_GLAPI *p_glBeginOcclusionQueryNV)(GLuint);
        void       (WINE_GLAPI *p_glBeginPerfMonitorAMD)(GLuint);
        void       (WINE_GLAPI *p_glBeginPerfQueryINTEL)(GLuint);
        void       (WINE_GLAPI *p_glBeginQuery)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glBeginQueryARB)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glBeginQueryIndexed)(GLenum,GLuint,GLuint);
        void       (WINE_GLAPI *p_glBeginTransformFeedback)(GLenum);
        void       (WINE_GLAPI *p_glBeginTransformFeedbackEXT)(GLenum);
        void       (WINE_GLAPI *p_glBeginTransformFeedbackNV)(GLenum);
        void       (WINE_GLAPI *p_glBeginVertexShaderEXT)(void);
        void       (WINE_GLAPI *p_glBeginVideoCaptureNV)(GLuint);
        void       (WINE_GLAPI *p_glBindAttribLocation)(GLuint,GLuint,const GLchar*);
        void       (WINE_GLAPI *p_glBindAttribLocationARB)(GLhandleARB,GLuint,const GLcharARB*);
        void       (WINE_GLAPI *p_glBindBuffer)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glBindBufferARB)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glBindBufferBase)(GLenum,GLuint,GLuint);
        void       (WINE_GLAPI *p_glBindBufferBaseEXT)(GLenum,GLuint,GLuint);
        void       (WINE_GLAPI *p_glBindBufferBaseNV)(GLenum,GLuint,GLuint);
        void       (WINE_GLAPI *p_glBindBufferOffsetEXT)(GLenum,GLuint,GLuint,GLintptr);
        void       (WINE_GLAPI *p_glBindBufferOffsetNV)(GLenum,GLuint,GLuint,GLintptr);
        void       (WINE_GLAPI *p_glBindBufferRange)(GLenum,GLuint,GLuint,GLintptr,GLsizeiptr);
        void       (WINE_GLAPI *p_glBindBufferRangeEXT)(GLenum,GLuint,GLuint,GLintptr,GLsizeiptr);
        void       (WINE_GLAPI *p_glBindBufferRangeNV)(GLenum,GLuint,GLuint,GLintptr,GLsizeiptr);
        void       (WINE_GLAPI *p_glBindBuffersBase)(GLenum,GLuint,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glBindBuffersRange)(GLenum,GLuint,GLsizei,const GLuint*,const GLintptr*,const GLsizeiptr*);
        void       (WINE_GLAPI *p_glBindFragDataLocation)(GLuint,GLuint,const GLchar*);
        void       (WINE_GLAPI *p_glBindFragDataLocationEXT)(GLuint,GLuint,const GLchar*);
        void       (WINE_GLAPI *p_glBindFragDataLocationIndexed)(GLuint,GLuint,GLuint,const GLchar*);
        void       (WINE_GLAPI *p_glBindFragmentShaderATI)(GLuint);
        void       (WINE_GLAPI *p_glBindFramebuffer)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glBindFramebufferEXT)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glBindImageTexture)(GLuint,GLuint,GLint,GLboolean,GLint,GLenum,GLenum);
        void       (WINE_GLAPI *p_glBindImageTextureEXT)(GLuint,GLuint,GLint,GLboolean,GLint,GLenum,GLint);
        void       (WINE_GLAPI *p_glBindImageTextures)(GLuint,GLsizei,const GLuint*);
        GLuint     (WINE_GLAPI *p_glBindLightParameterEXT)(GLenum,GLenum);
        GLuint     (WINE_GLAPI *p_glBindMaterialParameterEXT)(GLenum,GLenum);
        void       (WINE_GLAPI *p_glBindMultiTextureEXT)(GLenum,GLenum,GLuint);
        GLuint     (WINE_GLAPI *p_glBindParameterEXT)(GLenum);
        void       (WINE_GLAPI *p_glBindProgramARB)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glBindProgramNV)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glBindProgramPipeline)(GLuint);
        void       (WINE_GLAPI *p_glBindRenderbuffer)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glBindRenderbufferEXT)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glBindSampler)(GLuint,GLuint);
        void       (WINE_GLAPI *p_glBindSamplers)(GLuint,GLsizei,const GLuint*);
        GLuint     (WINE_GLAPI *p_glBindTexGenParameterEXT)(GLenum,GLenum,GLenum);
        void       (WINE_GLAPI *p_glBindTextureEXT)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glBindTextureUnit)(GLuint,GLuint);
        GLuint     (WINE_GLAPI *p_glBindTextureUnitParameterEXT)(GLenum,GLenum);
        void       (WINE_GLAPI *p_glBindTextures)(GLuint,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glBindTransformFeedback)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glBindTransformFeedbackNV)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glBindVertexArray)(GLuint);
        void       (WINE_GLAPI *p_glBindVertexArrayAPPLE)(GLuint);
        void       (WINE_GLAPI *p_glBindVertexBuffer)(GLuint,GLuint,GLintptr,GLsizei);
        void       (WINE_GLAPI *p_glBindVertexBuffers)(GLuint,GLsizei,const GLuint*,const GLintptr*,const GLsizei*);
        void       (WINE_GLAPI *p_glBindVertexShaderEXT)(GLuint);
        void       (WINE_GLAPI *p_glBindVideoCaptureStreamBufferNV)(GLuint,GLuint,GLenum,GLintptrARB);
        void       (WINE_GLAPI *p_glBindVideoCaptureStreamTextureNV)(GLuint,GLuint,GLenum,GLenum,GLuint);
        void       (WINE_GLAPI *p_glBinormal3bEXT)(GLbyte,GLbyte,GLbyte);
        void       (WINE_GLAPI *p_glBinormal3bvEXT)(const GLbyte*);
        void       (WINE_GLAPI *p_glBinormal3dEXT)(GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glBinormal3dvEXT)(const GLdouble*);
        void       (WINE_GLAPI *p_glBinormal3fEXT)(GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glBinormal3fvEXT)(const GLfloat*);
        void       (WINE_GLAPI *p_glBinormal3iEXT)(GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glBinormal3ivEXT)(const GLint*);
        void       (WINE_GLAPI *p_glBinormal3sEXT)(GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glBinormal3svEXT)(const GLshort*);
        void       (WINE_GLAPI *p_glBinormalPointerEXT)(GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glBitmapxOES)(GLsizei,GLsizei,GLfixed,GLfixed,GLfixed,GLfixed,const GLubyte*);
        void       (WINE_GLAPI *p_glBlendBarrierKHR)(void);
        void       (WINE_GLAPI *p_glBlendBarrierNV)(void);
        void       (WINE_GLAPI *p_glBlendColor)(GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glBlendColorEXT)(GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glBlendColorxOES)(GLfixed,GLfixed,GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glBlendEquation)(GLenum);
        void       (WINE_GLAPI *p_glBlendEquationEXT)(GLenum);
        void       (WINE_GLAPI *p_glBlendEquationIndexedAMD)(GLuint,GLenum);
        void       (WINE_GLAPI *p_glBlendEquationSeparate)(GLenum,GLenum);
        void       (WINE_GLAPI *p_glBlendEquationSeparateEXT)(GLenum,GLenum);
        void       (WINE_GLAPI *p_glBlendEquationSeparateIndexedAMD)(GLuint,GLenum,GLenum);
        void       (WINE_GLAPI *p_glBlendEquationSeparatei)(GLuint,GLenum,GLenum);
        void       (WINE_GLAPI *p_glBlendEquationSeparateiARB)(GLuint,GLenum,GLenum);
        void       (WINE_GLAPI *p_glBlendEquationi)(GLuint,GLenum);
        void       (WINE_GLAPI *p_glBlendEquationiARB)(GLuint,GLenum);
        void       (WINE_GLAPI *p_glBlendFuncIndexedAMD)(GLuint,GLenum,GLenum);
        void       (WINE_GLAPI *p_glBlendFuncSeparate)(GLenum,GLenum,GLenum,GLenum);
        void       (WINE_GLAPI *p_glBlendFuncSeparateEXT)(GLenum,GLenum,GLenum,GLenum);
        void       (WINE_GLAPI *p_glBlendFuncSeparateINGR)(GLenum,GLenum,GLenum,GLenum);
        void       (WINE_GLAPI *p_glBlendFuncSeparateIndexedAMD)(GLuint,GLenum,GLenum,GLenum,GLenum);
        void       (WINE_GLAPI *p_glBlendFuncSeparatei)(GLuint,GLenum,GLenum,GLenum,GLenum);
        void       (WINE_GLAPI *p_glBlendFuncSeparateiARB)(GLuint,GLenum,GLenum,GLenum,GLenum);
        void       (WINE_GLAPI *p_glBlendFunci)(GLuint,GLenum,GLenum);
        void       (WINE_GLAPI *p_glBlendFunciARB)(GLuint,GLenum,GLenum);
        void       (WINE_GLAPI *p_glBlendParameteriNV)(GLenum,GLint);
        void       (WINE_GLAPI *p_glBlitFramebuffer)(GLint,GLint,GLint,GLint,GLint,GLint,GLint,GLint,GLbitfield,GLenum);
        void       (WINE_GLAPI *p_glBlitFramebufferEXT)(GLint,GLint,GLint,GLint,GLint,GLint,GLint,GLint,GLbitfield,GLenum);
        void       (WINE_GLAPI *p_glBlitNamedFramebuffer)(GLuint,GLuint,GLint,GLint,GLint,GLint,GLint,GLint,GLint,GLint,GLbitfield,GLenum);
        void       (WINE_GLAPI *p_glBufferAddressRangeNV)(GLenum,GLuint,GLuint64EXT,GLsizeiptr);
        void       (WINE_GLAPI *p_glBufferData)(GLenum,GLsizeiptr,const void*,GLenum);
        void       (WINE_GLAPI *p_glBufferDataARB)(GLenum,GLsizeiptrARB,const void*,GLenum);
        void       (WINE_GLAPI *p_glBufferPageCommitmentARB)(GLenum,GLintptr,GLsizeiptr,GLboolean);
        void       (WINE_GLAPI *p_glBufferParameteriAPPLE)(GLenum,GLenum,GLint);
        GLuint     (WINE_GLAPI *p_glBufferRegionEnabled)(void);
        void       (WINE_GLAPI *p_glBufferStorage)(GLenum,GLsizeiptr,const void*,GLbitfield);
        void       (WINE_GLAPI *p_glBufferSubData)(GLenum,GLintptr,GLsizeiptr,const void*);
        void       (WINE_GLAPI *p_glBufferSubDataARB)(GLenum,GLintptrARB,GLsizeiptrARB,const void*);
        void       (WINE_GLAPI *p_glCallCommandListNV)(GLuint);
        GLenum     (WINE_GLAPI *p_glCheckFramebufferStatus)(GLenum);
        GLenum     (WINE_GLAPI *p_glCheckFramebufferStatusEXT)(GLenum);
        GLenum     (WINE_GLAPI *p_glCheckNamedFramebufferStatus)(GLuint,GLenum);
        GLenum     (WINE_GLAPI *p_glCheckNamedFramebufferStatusEXT)(GLuint,GLenum);
        void       (WINE_GLAPI *p_glClampColor)(GLenum,GLenum);
        void       (WINE_GLAPI *p_glClampColorARB)(GLenum,GLenum);
        void       (WINE_GLAPI *p_glClearAccumxOES)(GLfixed,GLfixed,GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glClearBufferData)(GLenum,GLenum,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glClearBufferSubData)(GLenum,GLenum,GLintptr,GLsizeiptr,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glClearBufferfi)(GLenum,GLint,GLfloat,GLint);
        void       (WINE_GLAPI *p_glClearBufferfv)(GLenum,GLint,const GLfloat*);
        void       (WINE_GLAPI *p_glClearBufferiv)(GLenum,GLint,const GLint*);
        void       (WINE_GLAPI *p_glClearBufferuiv)(GLenum,GLint,const GLuint*);
        void       (WINE_GLAPI *p_glClearColorIiEXT)(GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glClearColorIuiEXT)(GLuint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glClearColorxOES)(GLfixed,GLfixed,GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glClearDepthdNV)(GLdouble);
        void       (WINE_GLAPI *p_glClearDepthf)(GLfloat);
        void       (WINE_GLAPI *p_glClearDepthfOES)(GLclampf);
        void       (WINE_GLAPI *p_glClearDepthxOES)(GLfixed);
        void       (WINE_GLAPI *p_glClearNamedBufferData)(GLuint,GLenum,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glClearNamedBufferDataEXT)(GLuint,GLenum,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glClearNamedBufferSubData)(GLuint,GLenum,GLintptr,GLsizeiptr,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glClearNamedBufferSubDataEXT)(GLuint,GLenum,GLsizeiptr,GLsizeiptr,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glClearNamedFramebufferfi)(GLuint,GLenum,const GLfloat,GLint);
        void       (WINE_GLAPI *p_glClearNamedFramebufferfv)(GLuint,GLenum,GLint,const GLfloat*);
        void       (WINE_GLAPI *p_glClearNamedFramebufferiv)(GLuint,GLenum,GLint,const GLint*);
        void       (WINE_GLAPI *p_glClearNamedFramebufferuiv)(GLuint,GLenum,GLint,const GLuint*);
        void       (WINE_GLAPI *p_glClearTexImage)(GLuint,GLint,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glClearTexSubImage)(GLuint,GLint,GLint,GLint,GLint,GLsizei,GLsizei,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glClientActiveTexture)(GLenum);
        void       (WINE_GLAPI *p_glClientActiveTextureARB)(GLenum);
        void       (WINE_GLAPI *p_glClientActiveVertexStreamATI)(GLenum);
        void       (WINE_GLAPI *p_glClientAttribDefaultEXT)(GLbitfield);
        GLenum     (WINE_GLAPI *p_glClientWaitSync)(GLsync,GLbitfield,GLuint64);
        void       (WINE_GLAPI *p_glClipControl)(GLenum,GLenum);
        void       (WINE_GLAPI *p_glClipPlanefOES)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glClipPlanexOES)(GLenum,const GLfixed*);
        void       (WINE_GLAPI *p_glColor3fVertex3fSUN)(GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glColor3fVertex3fvSUN)(const GLfloat*,const GLfloat*);
        void       (WINE_GLAPI *p_glColor3hNV)(GLhalfNV,GLhalfNV,GLhalfNV);
        void       (WINE_GLAPI *p_glColor3hvNV)(const GLhalfNV*);
        void       (WINE_GLAPI *p_glColor3xOES)(GLfixed,GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glColor3xvOES)(const GLfixed*);
        void       (WINE_GLAPI *p_glColor4fNormal3fVertex3fSUN)(GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glColor4fNormal3fVertex3fvSUN)(const GLfloat*,const GLfloat*,const GLfloat*);
        void       (WINE_GLAPI *p_glColor4hNV)(GLhalfNV,GLhalfNV,GLhalfNV,GLhalfNV);
        void       (WINE_GLAPI *p_glColor4hvNV)(const GLhalfNV*);
        void       (WINE_GLAPI *p_glColor4ubVertex2fSUN)(GLubyte,GLubyte,GLubyte,GLubyte,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glColor4ubVertex2fvSUN)(const GLubyte*,const GLfloat*);
        void       (WINE_GLAPI *p_glColor4ubVertex3fSUN)(GLubyte,GLubyte,GLubyte,GLubyte,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glColor4ubVertex3fvSUN)(const GLubyte*,const GLfloat*);
        void       (WINE_GLAPI *p_glColor4xOES)(GLfixed,GLfixed,GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glColor4xvOES)(const GLfixed*);
        void       (WINE_GLAPI *p_glColorFormatNV)(GLint,GLenum,GLsizei);
        void       (WINE_GLAPI *p_glColorFragmentOp1ATI)(GLenum,GLuint,GLuint,GLuint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glColorFragmentOp2ATI)(GLenum,GLuint,GLuint,GLuint,GLuint,GLuint,GLuint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glColorFragmentOp3ATI)(GLenum,GLuint,GLuint,GLuint,GLuint,GLuint,GLuint,GLuint,GLuint,GLuint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glColorMaskIndexedEXT)(GLuint,GLboolean,GLboolean,GLboolean,GLboolean);
        void       (WINE_GLAPI *p_glColorMaski)(GLuint,GLboolean,GLboolean,GLboolean,GLboolean);
        void       (WINE_GLAPI *p_glColorP3ui)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glColorP3uiv)(GLenum,const GLuint*);
        void       (WINE_GLAPI *p_glColorP4ui)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glColorP4uiv)(GLenum,const GLuint*);
        void       (WINE_GLAPI *p_glColorPointerEXT)(GLint,GLenum,GLsizei,GLsizei,const void*);
        void       (WINE_GLAPI *p_glColorPointerListIBM)(GLint,GLenum,GLint,const void**,GLint);
        void       (WINE_GLAPI *p_glColorPointervINTEL)(GLint,GLenum,const void**);
        void       (WINE_GLAPI *p_glColorSubTable)(GLenum,GLsizei,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glColorSubTableEXT)(GLenum,GLsizei,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glColorTable)(GLenum,GLenum,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glColorTableEXT)(GLenum,GLenum,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glColorTableParameterfv)(GLenum,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glColorTableParameterfvSGI)(GLenum,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glColorTableParameteriv)(GLenum,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glColorTableParameterivSGI)(GLenum,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glColorTableSGI)(GLenum,GLenum,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glCombinerInputNV)(GLenum,GLenum,GLenum,GLenum,GLenum,GLenum);
        void       (WINE_GLAPI *p_glCombinerOutputNV)(GLenum,GLenum,GLenum,GLenum,GLenum,GLenum,GLenum,GLboolean,GLboolean,GLboolean);
        void       (WINE_GLAPI *p_glCombinerParameterfNV)(GLenum,GLfloat);
        void       (WINE_GLAPI *p_glCombinerParameterfvNV)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glCombinerParameteriNV)(GLenum,GLint);
        void       (WINE_GLAPI *p_glCombinerParameterivNV)(GLenum,const GLint*);
        void       (WINE_GLAPI *p_glCombinerStageParameterfvNV)(GLenum,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glCommandListSegmentsNV)(GLuint,GLuint);
        void       (WINE_GLAPI *p_glCompileCommandListNV)(GLuint);
        void       (WINE_GLAPI *p_glCompileShader)(GLuint);
        void       (WINE_GLAPI *p_glCompileShaderARB)(GLhandleARB);
        void       (WINE_GLAPI *p_glCompileShaderIncludeARB)(GLuint,GLsizei,const GLchar*const*,const GLint*);
        void       (WINE_GLAPI *p_glCompressedMultiTexImage1DEXT)(GLenum,GLenum,GLint,GLenum,GLsizei,GLint,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedMultiTexImage2DEXT)(GLenum,GLenum,GLint,GLenum,GLsizei,GLsizei,GLint,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedMultiTexImage3DEXT)(GLenum,GLenum,GLint,GLenum,GLsizei,GLsizei,GLsizei,GLint,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedMultiTexSubImage1DEXT)(GLenum,GLenum,GLint,GLint,GLsizei,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedMultiTexSubImage2DEXT)(GLenum,GLenum,GLint,GLint,GLint,GLsizei,GLsizei,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedMultiTexSubImage3DEXT)(GLenum,GLenum,GLint,GLint,GLint,GLint,GLsizei,GLsizei,GLsizei,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedTexImage1D)(GLenum,GLint,GLenum,GLsizei,GLint,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedTexImage1DARB)(GLenum,GLint,GLenum,GLsizei,GLint,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedTexImage2D)(GLenum,GLint,GLenum,GLsizei,GLsizei,GLint,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedTexImage2DARB)(GLenum,GLint,GLenum,GLsizei,GLsizei,GLint,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedTexImage3D)(GLenum,GLint,GLenum,GLsizei,GLsizei,GLsizei,GLint,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedTexImage3DARB)(GLenum,GLint,GLenum,GLsizei,GLsizei,GLsizei,GLint,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedTexSubImage1D)(GLenum,GLint,GLint,GLsizei,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedTexSubImage1DARB)(GLenum,GLint,GLint,GLsizei,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedTexSubImage2D)(GLenum,GLint,GLint,GLint,GLsizei,GLsizei,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedTexSubImage2DARB)(GLenum,GLint,GLint,GLint,GLsizei,GLsizei,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedTexSubImage3D)(GLenum,GLint,GLint,GLint,GLint,GLsizei,GLsizei,GLsizei,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedTexSubImage3DARB)(GLenum,GLint,GLint,GLint,GLint,GLsizei,GLsizei,GLsizei,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedTextureImage1DEXT)(GLuint,GLenum,GLint,GLenum,GLsizei,GLint,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedTextureImage2DEXT)(GLuint,GLenum,GLint,GLenum,GLsizei,GLsizei,GLint,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedTextureImage3DEXT)(GLuint,GLenum,GLint,GLenum,GLsizei,GLsizei,GLsizei,GLint,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedTextureSubImage1D)(GLuint,GLint,GLint,GLsizei,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedTextureSubImage1DEXT)(GLuint,GLenum,GLint,GLint,GLsizei,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedTextureSubImage2D)(GLuint,GLint,GLint,GLint,GLsizei,GLsizei,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedTextureSubImage2DEXT)(GLuint,GLenum,GLint,GLint,GLint,GLsizei,GLsizei,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedTextureSubImage3D)(GLuint,GLint,GLint,GLint,GLint,GLsizei,GLsizei,GLsizei,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glCompressedTextureSubImage3DEXT)(GLuint,GLenum,GLint,GLint,GLint,GLint,GLsizei,GLsizei,GLsizei,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glConvolutionFilter1D)(GLenum,GLenum,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glConvolutionFilter1DEXT)(GLenum,GLenum,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glConvolutionFilter2D)(GLenum,GLenum,GLsizei,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glConvolutionFilter2DEXT)(GLenum,GLenum,GLsizei,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glConvolutionParameterf)(GLenum,GLenum,GLfloat);
        void       (WINE_GLAPI *p_glConvolutionParameterfEXT)(GLenum,GLenum,GLfloat);
        void       (WINE_GLAPI *p_glConvolutionParameterfv)(GLenum,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glConvolutionParameterfvEXT)(GLenum,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glConvolutionParameteri)(GLenum,GLenum,GLint);
        void       (WINE_GLAPI *p_glConvolutionParameteriEXT)(GLenum,GLenum,GLint);
        void       (WINE_GLAPI *p_glConvolutionParameteriv)(GLenum,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glConvolutionParameterivEXT)(GLenum,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glConvolutionParameterxOES)(GLenum,GLenum,GLfixed);
        void       (WINE_GLAPI *p_glConvolutionParameterxvOES)(GLenum,GLenum,const GLfixed*);
        void       (WINE_GLAPI *p_glCopyBufferSubData)(GLenum,GLenum,GLintptr,GLintptr,GLsizeiptr);
        void       (WINE_GLAPI *p_glCopyColorSubTable)(GLenum,GLsizei,GLint,GLint,GLsizei);
        void       (WINE_GLAPI *p_glCopyColorSubTableEXT)(GLenum,GLsizei,GLint,GLint,GLsizei);
        void       (WINE_GLAPI *p_glCopyColorTable)(GLenum,GLenum,GLint,GLint,GLsizei);
        void       (WINE_GLAPI *p_glCopyColorTableSGI)(GLenum,GLenum,GLint,GLint,GLsizei);
        void       (WINE_GLAPI *p_glCopyConvolutionFilter1D)(GLenum,GLenum,GLint,GLint,GLsizei);
        void       (WINE_GLAPI *p_glCopyConvolutionFilter1DEXT)(GLenum,GLenum,GLint,GLint,GLsizei);
        void       (WINE_GLAPI *p_glCopyConvolutionFilter2D)(GLenum,GLenum,GLint,GLint,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glCopyConvolutionFilter2DEXT)(GLenum,GLenum,GLint,GLint,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glCopyImageSubData)(GLuint,GLenum,GLint,GLint,GLint,GLint,GLuint,GLenum,GLint,GLint,GLint,GLint,GLsizei,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glCopyImageSubDataNV)(GLuint,GLenum,GLint,GLint,GLint,GLint,GLuint,GLenum,GLint,GLint,GLint,GLint,GLsizei,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glCopyMultiTexImage1DEXT)(GLenum,GLenum,GLint,GLenum,GLint,GLint,GLsizei,GLint);
        void       (WINE_GLAPI *p_glCopyMultiTexImage2DEXT)(GLenum,GLenum,GLint,GLenum,GLint,GLint,GLsizei,GLsizei,GLint);
        void       (WINE_GLAPI *p_glCopyMultiTexSubImage1DEXT)(GLenum,GLenum,GLint,GLint,GLint,GLint,GLsizei);
        void       (WINE_GLAPI *p_glCopyMultiTexSubImage2DEXT)(GLenum,GLenum,GLint,GLint,GLint,GLint,GLint,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glCopyMultiTexSubImage3DEXT)(GLenum,GLenum,GLint,GLint,GLint,GLint,GLint,GLint,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glCopyNamedBufferSubData)(GLuint,GLuint,GLintptr,GLintptr,GLsizeiptr);
        void       (WINE_GLAPI *p_glCopyPathNV)(GLuint,GLuint);
        void       (WINE_GLAPI *p_glCopyTexImage1DEXT)(GLenum,GLint,GLenum,GLint,GLint,GLsizei,GLint);
        void       (WINE_GLAPI *p_glCopyTexImage2DEXT)(GLenum,GLint,GLenum,GLint,GLint,GLsizei,GLsizei,GLint);
        void       (WINE_GLAPI *p_glCopyTexSubImage1DEXT)(GLenum,GLint,GLint,GLint,GLint,GLsizei);
        void       (WINE_GLAPI *p_glCopyTexSubImage2DEXT)(GLenum,GLint,GLint,GLint,GLint,GLint,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glCopyTexSubImage3D)(GLenum,GLint,GLint,GLint,GLint,GLint,GLint,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glCopyTexSubImage3DEXT)(GLenum,GLint,GLint,GLint,GLint,GLint,GLint,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glCopyTextureImage1DEXT)(GLuint,GLenum,GLint,GLenum,GLint,GLint,GLsizei,GLint);
        void       (WINE_GLAPI *p_glCopyTextureImage2DEXT)(GLuint,GLenum,GLint,GLenum,GLint,GLint,GLsizei,GLsizei,GLint);
        void       (WINE_GLAPI *p_glCopyTextureSubImage1D)(GLuint,GLint,GLint,GLint,GLint,GLsizei);
        void       (WINE_GLAPI *p_glCopyTextureSubImage1DEXT)(GLuint,GLenum,GLint,GLint,GLint,GLint,GLsizei);
        void       (WINE_GLAPI *p_glCopyTextureSubImage2D)(GLuint,GLint,GLint,GLint,GLint,GLint,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glCopyTextureSubImage2DEXT)(GLuint,GLenum,GLint,GLint,GLint,GLint,GLint,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glCopyTextureSubImage3D)(GLuint,GLint,GLint,GLint,GLint,GLint,GLint,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glCopyTextureSubImage3DEXT)(GLuint,GLenum,GLint,GLint,GLint,GLint,GLint,GLint,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glCoverFillPathInstancedNV)(GLsizei,GLenum,const void*,GLuint,GLenum,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glCoverFillPathNV)(GLuint,GLenum);
        void       (WINE_GLAPI *p_glCoverStrokePathInstancedNV)(GLsizei,GLenum,const void*,GLuint,GLenum,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glCoverStrokePathNV)(GLuint,GLenum);
        void       (WINE_GLAPI *p_glCoverageModulationNV)(GLenum);
        void       (WINE_GLAPI *p_glCoverageModulationTableNV)(GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glCreateBuffers)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glCreateCommandListsNV)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glCreateFramebuffers)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glCreatePerfQueryINTEL)(GLuint,GLuint*);
        GLuint     (WINE_GLAPI *p_glCreateProgram)(void);
        GLhandleARB (WINE_GLAPI *p_glCreateProgramObjectARB)(void);
        void       (WINE_GLAPI *p_glCreateProgramPipelines)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glCreateQueries)(GLenum,GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glCreateRenderbuffers)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glCreateSamplers)(GLsizei,GLuint*);
        GLuint     (WINE_GLAPI *p_glCreateShader)(GLenum);
        GLhandleARB (WINE_GLAPI *p_glCreateShaderObjectARB)(GLenum);
        GLuint     (WINE_GLAPI *p_glCreateShaderProgramEXT)(GLenum,const GLchar*);
        GLuint     (WINE_GLAPI *p_glCreateShaderProgramv)(GLenum,GLsizei,const GLchar*const*);
        void       (WINE_GLAPI *p_glCreateStatesNV)(GLsizei,GLuint*);
        GLsync     (WINE_GLAPI *p_glCreateSyncFromCLeventARB)(void*,void*,GLbitfield);
        void       (WINE_GLAPI *p_glCreateTextures)(GLenum,GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glCreateTransformFeedbacks)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glCreateVertexArrays)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glCullParameterdvEXT)(GLenum,GLdouble*);
        void       (WINE_GLAPI *p_glCullParameterfvEXT)(GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glCurrentPaletteMatrixARB)(GLint);
        void       (WINE_GLAPI *p_glDebugMessageCallback)(void *,const void*);
        void       (WINE_GLAPI *p_glDebugMessageCallbackAMD)(void *,void*);
        void       (WINE_GLAPI *p_glDebugMessageCallbackARB)(void *,const void*);
        void       (WINE_GLAPI *p_glDebugMessageControl)(GLenum,GLenum,GLenum,GLsizei,const GLuint*,GLboolean);
        void       (WINE_GLAPI *p_glDebugMessageControlARB)(GLenum,GLenum,GLenum,GLsizei,const GLuint*,GLboolean);
        void       (WINE_GLAPI *p_glDebugMessageEnableAMD)(GLenum,GLenum,GLsizei,const GLuint*,GLboolean);
        void       (WINE_GLAPI *p_glDebugMessageInsert)(GLenum,GLenum,GLuint,GLenum,GLsizei,const GLchar*);
        void       (WINE_GLAPI *p_glDebugMessageInsertAMD)(GLenum,GLenum,GLuint,GLsizei,const GLchar*);
        void       (WINE_GLAPI *p_glDebugMessageInsertARB)(GLenum,GLenum,GLuint,GLenum,GLsizei,const GLchar*);
        void       (WINE_GLAPI *p_glDeformSGIX)(GLbitfield);
        void       (WINE_GLAPI *p_glDeformationMap3dSGIX)(GLenum,GLdouble,GLdouble,GLint,GLint,GLdouble,GLdouble,GLint,GLint,GLdouble,GLdouble,GLint,GLint,const GLdouble*);
        void       (WINE_GLAPI *p_glDeformationMap3fSGIX)(GLenum,GLfloat,GLfloat,GLint,GLint,GLfloat,GLfloat,GLint,GLint,GLfloat,GLfloat,GLint,GLint,const GLfloat*);
        void       (WINE_GLAPI *p_glDeleteAsyncMarkersSGIX)(GLuint,GLsizei);
        void       (WINE_GLAPI *p_glDeleteBufferRegion)(GLenum);
        void       (WINE_GLAPI *p_glDeleteBuffers)(GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glDeleteBuffersARB)(GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glDeleteCommandListsNV)(GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glDeleteFencesAPPLE)(GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glDeleteFencesNV)(GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glDeleteFragmentShaderATI)(GLuint);
        void       (WINE_GLAPI *p_glDeleteFramebuffers)(GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glDeleteFramebuffersEXT)(GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glDeleteNamedStringARB)(GLint,const GLchar*);
        void       (WINE_GLAPI *p_glDeleteNamesAMD)(GLenum,GLuint,const GLuint*);
        void       (WINE_GLAPI *p_glDeleteObjectARB)(GLhandleARB);
        void       (WINE_GLAPI *p_glDeleteObjectBufferATI)(GLuint);
        void       (WINE_GLAPI *p_glDeleteOcclusionQueriesNV)(GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glDeletePathsNV)(GLuint,GLsizei);
        void       (WINE_GLAPI *p_glDeletePerfMonitorsAMD)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glDeletePerfQueryINTEL)(GLuint);
        void       (WINE_GLAPI *p_glDeleteProgram)(GLuint);
        void       (WINE_GLAPI *p_glDeleteProgramPipelines)(GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glDeleteProgramsARB)(GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glDeleteProgramsNV)(GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glDeleteQueries)(GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glDeleteQueriesARB)(GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glDeleteRenderbuffers)(GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glDeleteRenderbuffersEXT)(GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glDeleteSamplers)(GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glDeleteShader)(GLuint);
        void       (WINE_GLAPI *p_glDeleteStatesNV)(GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glDeleteSync)(GLsync);
        void       (WINE_GLAPI *p_glDeleteTexturesEXT)(GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glDeleteTransformFeedbacks)(GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glDeleteTransformFeedbacksNV)(GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glDeleteVertexArrays)(GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glDeleteVertexArraysAPPLE)(GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glDeleteVertexShaderEXT)(GLuint);
        void       (WINE_GLAPI *p_glDepthBoundsEXT)(GLclampd,GLclampd);
        void       (WINE_GLAPI *p_glDepthBoundsdNV)(GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glDepthRangeArrayv)(GLuint,GLsizei,const GLdouble*);
        void       (WINE_GLAPI *p_glDepthRangeIndexed)(GLuint,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glDepthRangedNV)(GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glDepthRangef)(GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glDepthRangefOES)(GLclampf,GLclampf);
        void       (WINE_GLAPI *p_glDepthRangexOES)(GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glDetachObjectARB)(GLhandleARB,GLhandleARB);
        void       (WINE_GLAPI *p_glDetachShader)(GLuint,GLuint);
        void       (WINE_GLAPI *p_glDetailTexFuncSGIS)(GLenum,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glDisableClientStateIndexedEXT)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glDisableClientStateiEXT)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glDisableIndexedEXT)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glDisableVariantClientStateEXT)(GLuint);
        void       (WINE_GLAPI *p_glDisableVertexArrayAttrib)(GLuint,GLuint);
        void       (WINE_GLAPI *p_glDisableVertexArrayAttribEXT)(GLuint,GLuint);
        void       (WINE_GLAPI *p_glDisableVertexArrayEXT)(GLuint,GLenum);
        void       (WINE_GLAPI *p_glDisableVertexAttribAPPLE)(GLuint,GLenum);
        void       (WINE_GLAPI *p_glDisableVertexAttribArray)(GLuint);
        void       (WINE_GLAPI *p_glDisableVertexAttribArrayARB)(GLuint);
        void       (WINE_GLAPI *p_glDisablei)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glDispatchCompute)(GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glDispatchComputeGroupSizeARB)(GLuint,GLuint,GLuint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glDispatchComputeIndirect)(GLintptr);
        void       (WINE_GLAPI *p_glDrawArraysEXT)(GLenum,GLint,GLsizei);
        void       (WINE_GLAPI *p_glDrawArraysIndirect)(GLenum,const void*);
        void       (WINE_GLAPI *p_glDrawArraysInstanced)(GLenum,GLint,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glDrawArraysInstancedARB)(GLenum,GLint,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glDrawArraysInstancedBaseInstance)(GLenum,GLint,GLsizei,GLsizei,GLuint);
        void       (WINE_GLAPI *p_glDrawArraysInstancedEXT)(GLenum,GLint,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glDrawBufferRegion)(GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLint);
        void       (WINE_GLAPI *p_glDrawBuffers)(GLsizei,const GLenum*);
        void       (WINE_GLAPI *p_glDrawBuffersARB)(GLsizei,const GLenum*);
        void       (WINE_GLAPI *p_glDrawBuffersATI)(GLsizei,const GLenum*);
        void       (WINE_GLAPI *p_glDrawCommandsAddressNV)(GLenum,const GLuint64*,const GLsizei*,GLuint);
        void       (WINE_GLAPI *p_glDrawCommandsNV)(GLenum,GLuint,const GLintptr*,const GLsizei*,GLuint);
        void       (WINE_GLAPI *p_glDrawCommandsStatesAddressNV)(const GLuint64*,const GLsizei*,const GLuint*,const GLuint*,GLuint);
        void       (WINE_GLAPI *p_glDrawCommandsStatesNV)(GLuint,const GLintptr*,const GLsizei*,const GLuint*,const GLuint*,GLuint);
        void       (WINE_GLAPI *p_glDrawElementArrayAPPLE)(GLenum,GLint,GLsizei);
        void       (WINE_GLAPI *p_glDrawElementArrayATI)(GLenum,GLsizei);
        void       (WINE_GLAPI *p_glDrawElementsBaseVertex)(GLenum,GLsizei,GLenum,const void*,GLint);
        void       (WINE_GLAPI *p_glDrawElementsIndirect)(GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glDrawElementsInstanced)(GLenum,GLsizei,GLenum,const void*,GLsizei);
        void       (WINE_GLAPI *p_glDrawElementsInstancedARB)(GLenum,GLsizei,GLenum,const void*,GLsizei);
        void       (WINE_GLAPI *p_glDrawElementsInstancedBaseInstance)(GLenum,GLsizei,GLenum,const void*,GLsizei,GLuint);
        void       (WINE_GLAPI *p_glDrawElementsInstancedBaseVertex)(GLenum,GLsizei,GLenum,const void*,GLsizei,GLint);
        void       (WINE_GLAPI *p_glDrawElementsInstancedBaseVertexBaseInstance)(GLenum,GLsizei,GLenum,const void*,GLsizei,GLint,GLuint);
        void       (WINE_GLAPI *p_glDrawElementsInstancedEXT)(GLenum,GLsizei,GLenum,const void*,GLsizei);
        void       (WINE_GLAPI *p_glDrawMeshArraysSUN)(GLenum,GLint,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glDrawRangeElementArrayAPPLE)(GLenum,GLuint,GLuint,GLint,GLsizei);
        void       (WINE_GLAPI *p_glDrawRangeElementArrayATI)(GLenum,GLuint,GLuint,GLsizei);
        void       (WINE_GLAPI *p_glDrawRangeElements)(GLenum,GLuint,GLuint,GLsizei,GLenum,const void*);
        void       (WINE_GLAPI *p_glDrawRangeElementsBaseVertex)(GLenum,GLuint,GLuint,GLsizei,GLenum,const void*,GLint);
        void       (WINE_GLAPI *p_glDrawRangeElementsEXT)(GLenum,GLuint,GLuint,GLsizei,GLenum,const void*);
        void       (WINE_GLAPI *p_glDrawTextureNV)(GLuint,GLuint,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glDrawTransformFeedback)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glDrawTransformFeedbackInstanced)(GLenum,GLuint,GLsizei);
        void       (WINE_GLAPI *p_glDrawTransformFeedbackNV)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glDrawTransformFeedbackStream)(GLenum,GLuint,GLuint);
        void       (WINE_GLAPI *p_glDrawTransformFeedbackStreamInstanced)(GLenum,GLuint,GLuint,GLsizei);
        void       (WINE_GLAPI *p_glEdgeFlagFormatNV)(GLsizei);
        void       (WINE_GLAPI *p_glEdgeFlagPointerEXT)(GLsizei,GLsizei,const GLboolean*);
        void       (WINE_GLAPI *p_glEdgeFlagPointerListIBM)(GLint,const GLboolean**,GLint);
        void       (WINE_GLAPI *p_glElementPointerAPPLE)(GLenum,const void*);
        void       (WINE_GLAPI *p_glElementPointerATI)(GLenum,const void*);
        void       (WINE_GLAPI *p_glEnableClientStateIndexedEXT)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glEnableClientStateiEXT)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glEnableIndexedEXT)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glEnableVariantClientStateEXT)(GLuint);
        void       (WINE_GLAPI *p_glEnableVertexArrayAttrib)(GLuint,GLuint);
        void       (WINE_GLAPI *p_glEnableVertexArrayAttribEXT)(GLuint,GLuint);
        void       (WINE_GLAPI *p_glEnableVertexArrayEXT)(GLuint,GLenum);
        void       (WINE_GLAPI *p_glEnableVertexAttribAPPLE)(GLuint,GLenum);
        void       (WINE_GLAPI *p_glEnableVertexAttribArray)(GLuint);
        void       (WINE_GLAPI *p_glEnableVertexAttribArrayARB)(GLuint);
        void       (WINE_GLAPI *p_glEnablei)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glEndConditionalRender)(void);
        void       (WINE_GLAPI *p_glEndConditionalRenderNV)(void);
        void       (WINE_GLAPI *p_glEndConditionalRenderNVX)(void);
        void       (WINE_GLAPI *p_glEndFragmentShaderATI)(void);
        void       (WINE_GLAPI *p_glEndOcclusionQueryNV)(void);
        void       (WINE_GLAPI *p_glEndPerfMonitorAMD)(GLuint);
        void       (WINE_GLAPI *p_glEndPerfQueryINTEL)(GLuint);
        void       (WINE_GLAPI *p_glEndQuery)(GLenum);
        void       (WINE_GLAPI *p_glEndQueryARB)(GLenum);
        void       (WINE_GLAPI *p_glEndQueryIndexed)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glEndTransformFeedback)(void);
        void       (WINE_GLAPI *p_glEndTransformFeedbackEXT)(void);
        void       (WINE_GLAPI *p_glEndTransformFeedbackNV)(void);
        void       (WINE_GLAPI *p_glEndVertexShaderEXT)(void);
        void       (WINE_GLAPI *p_glEndVideoCaptureNV)(GLuint);
        void       (WINE_GLAPI *p_glEvalCoord1xOES)(GLfixed);
        void       (WINE_GLAPI *p_glEvalCoord1xvOES)(const GLfixed*);
        void       (WINE_GLAPI *p_glEvalCoord2xOES)(GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glEvalCoord2xvOES)(const GLfixed*);
        void       (WINE_GLAPI *p_glEvalMapsNV)(GLenum,GLenum);
        void       (WINE_GLAPI *p_glExecuteProgramNV)(GLenum,GLuint,const GLfloat*);
        void       (WINE_GLAPI *p_glExtractComponentEXT)(GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glFeedbackBufferxOES)(GLsizei,GLenum,const GLfixed*);
        GLsync     (WINE_GLAPI *p_glFenceSync)(GLenum,GLbitfield);
        void       (WINE_GLAPI *p_glFinalCombinerInputNV)(GLenum,GLenum,GLenum,GLenum);
        GLint      (WINE_GLAPI *p_glFinishAsyncSGIX)(GLuint*);
        void       (WINE_GLAPI *p_glFinishFenceAPPLE)(GLuint);
        void       (WINE_GLAPI *p_glFinishFenceNV)(GLuint);
        void       (WINE_GLAPI *p_glFinishObjectAPPLE)(GLenum,GLint);
        void       (WINE_GLAPI *p_glFinishTextureSUNX)(void);
        void       (WINE_GLAPI *p_glFlushMappedBufferRange)(GLenum,GLintptr,GLsizeiptr);
        void       (WINE_GLAPI *p_glFlushMappedBufferRangeAPPLE)(GLenum,GLintptr,GLsizeiptr);
        void       (WINE_GLAPI *p_glFlushMappedNamedBufferRange)(GLuint,GLintptr,GLsizeiptr);
        void       (WINE_GLAPI *p_glFlushMappedNamedBufferRangeEXT)(GLuint,GLintptr,GLsizeiptr);
        void       (WINE_GLAPI *p_glFlushPixelDataRangeNV)(GLenum);
        void       (WINE_GLAPI *p_glFlushRasterSGIX)(void);
        void       (WINE_GLAPI *p_glFlushStaticDataIBM)(GLenum);
        void       (WINE_GLAPI *p_glFlushVertexArrayRangeAPPLE)(GLsizei,void*);
        void       (WINE_GLAPI *p_glFlushVertexArrayRangeNV)(void);
        void       (WINE_GLAPI *p_glFogCoordFormatNV)(GLenum,GLsizei);
        void       (WINE_GLAPI *p_glFogCoordPointer)(GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glFogCoordPointerEXT)(GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glFogCoordPointerListIBM)(GLenum,GLint,const void**,GLint);
        void       (WINE_GLAPI *p_glFogCoordd)(GLdouble);
        void       (WINE_GLAPI *p_glFogCoorddEXT)(GLdouble);
        void       (WINE_GLAPI *p_glFogCoorddv)(const GLdouble*);
        void       (WINE_GLAPI *p_glFogCoorddvEXT)(const GLdouble*);
        void       (WINE_GLAPI *p_glFogCoordf)(GLfloat);
        void       (WINE_GLAPI *p_glFogCoordfEXT)(GLfloat);
        void       (WINE_GLAPI *p_glFogCoordfv)(const GLfloat*);
        void       (WINE_GLAPI *p_glFogCoordfvEXT)(const GLfloat*);
        void       (WINE_GLAPI *p_glFogCoordhNV)(GLhalfNV);
        void       (WINE_GLAPI *p_glFogCoordhvNV)(const GLhalfNV*);
        void       (WINE_GLAPI *p_glFogFuncSGIS)(GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glFogxOES)(GLenum,GLfixed);
        void       (WINE_GLAPI *p_glFogxvOES)(GLenum,const GLfixed*);
        void       (WINE_GLAPI *p_glFragmentColorMaterialSGIX)(GLenum,GLenum);
        void       (WINE_GLAPI *p_glFragmentCoverageColorNV)(GLuint);
        void       (WINE_GLAPI *p_glFragmentLightModelfSGIX)(GLenum,GLfloat);
        void       (WINE_GLAPI *p_glFragmentLightModelfvSGIX)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glFragmentLightModeliSGIX)(GLenum,GLint);
        void       (WINE_GLAPI *p_glFragmentLightModelivSGIX)(GLenum,const GLint*);
        void       (WINE_GLAPI *p_glFragmentLightfSGIX)(GLenum,GLenum,GLfloat);
        void       (WINE_GLAPI *p_glFragmentLightfvSGIX)(GLenum,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glFragmentLightiSGIX)(GLenum,GLenum,GLint);
        void       (WINE_GLAPI *p_glFragmentLightivSGIX)(GLenum,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glFragmentMaterialfSGIX)(GLenum,GLenum,GLfloat);
        void       (WINE_GLAPI *p_glFragmentMaterialfvSGIX)(GLenum,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glFragmentMaterialiSGIX)(GLenum,GLenum,GLint);
        void       (WINE_GLAPI *p_glFragmentMaterialivSGIX)(GLenum,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glFrameTerminatorGREMEDY)(void);
        void       (WINE_GLAPI *p_glFrameZoomSGIX)(GLint);
        void       (WINE_GLAPI *p_glFramebufferDrawBufferEXT)(GLuint,GLenum);
        void       (WINE_GLAPI *p_glFramebufferDrawBuffersEXT)(GLuint,GLsizei,const GLenum*);
        void       (WINE_GLAPI *p_glFramebufferParameteri)(GLenum,GLenum,GLint);
        void       (WINE_GLAPI *p_glFramebufferReadBufferEXT)(GLuint,GLenum);
        void       (WINE_GLAPI *p_glFramebufferRenderbuffer)(GLenum,GLenum,GLenum,GLuint);
        void       (WINE_GLAPI *p_glFramebufferRenderbufferEXT)(GLenum,GLenum,GLenum,GLuint);
        void       (WINE_GLAPI *p_glFramebufferSampleLocationsfvNV)(GLenum,GLuint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glFramebufferTexture)(GLenum,GLenum,GLuint,GLint);
        void       (WINE_GLAPI *p_glFramebufferTexture1D)(GLenum,GLenum,GLenum,GLuint,GLint);
        void       (WINE_GLAPI *p_glFramebufferTexture1DEXT)(GLenum,GLenum,GLenum,GLuint,GLint);
        void       (WINE_GLAPI *p_glFramebufferTexture2D)(GLenum,GLenum,GLenum,GLuint,GLint);
        void       (WINE_GLAPI *p_glFramebufferTexture2DEXT)(GLenum,GLenum,GLenum,GLuint,GLint);
        void       (WINE_GLAPI *p_glFramebufferTexture3D)(GLenum,GLenum,GLenum,GLuint,GLint,GLint);
        void       (WINE_GLAPI *p_glFramebufferTexture3DEXT)(GLenum,GLenum,GLenum,GLuint,GLint,GLint);
        void       (WINE_GLAPI *p_glFramebufferTextureARB)(GLenum,GLenum,GLuint,GLint);
        void       (WINE_GLAPI *p_glFramebufferTextureEXT)(GLenum,GLenum,GLuint,GLint);
        void       (WINE_GLAPI *p_glFramebufferTextureFaceARB)(GLenum,GLenum,GLuint,GLint,GLenum);
        void       (WINE_GLAPI *p_glFramebufferTextureFaceEXT)(GLenum,GLenum,GLuint,GLint,GLenum);
        void       (WINE_GLAPI *p_glFramebufferTextureLayer)(GLenum,GLenum,GLuint,GLint,GLint);
        void       (WINE_GLAPI *p_glFramebufferTextureLayerARB)(GLenum,GLenum,GLuint,GLint,GLint);
        void       (WINE_GLAPI *p_glFramebufferTextureLayerEXT)(GLenum,GLenum,GLuint,GLint,GLint);
        void       (WINE_GLAPI *p_glFreeObjectBufferATI)(GLuint);
        void       (WINE_GLAPI *p_glFrustumfOES)(GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glFrustumxOES)(GLfixed,GLfixed,GLfixed,GLfixed,GLfixed,GLfixed);
        GLuint     (WINE_GLAPI *p_glGenAsyncMarkersSGIX)(GLsizei);
        void       (WINE_GLAPI *p_glGenBuffers)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glGenBuffersARB)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glGenFencesAPPLE)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glGenFencesNV)(GLsizei,GLuint*);
        GLuint     (WINE_GLAPI *p_glGenFragmentShadersATI)(GLuint);
        void       (WINE_GLAPI *p_glGenFramebuffers)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glGenFramebuffersEXT)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glGenNamesAMD)(GLenum,GLuint,GLuint*);
        void       (WINE_GLAPI *p_glGenOcclusionQueriesNV)(GLsizei,GLuint*);
        GLuint     (WINE_GLAPI *p_glGenPathsNV)(GLsizei);
        void       (WINE_GLAPI *p_glGenPerfMonitorsAMD)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glGenProgramPipelines)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glGenProgramsARB)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glGenProgramsNV)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glGenQueries)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glGenQueriesARB)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glGenRenderbuffers)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glGenRenderbuffersEXT)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glGenSamplers)(GLsizei,GLuint*);
        GLuint     (WINE_GLAPI *p_glGenSymbolsEXT)(GLenum,GLenum,GLenum,GLuint);
        void       (WINE_GLAPI *p_glGenTexturesEXT)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glGenTransformFeedbacks)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glGenTransformFeedbacksNV)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glGenVertexArrays)(GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glGenVertexArraysAPPLE)(GLsizei,GLuint*);
        GLuint     (WINE_GLAPI *p_glGenVertexShadersEXT)(GLuint);
        void       (WINE_GLAPI *p_glGenerateMipmap)(GLenum);
        void       (WINE_GLAPI *p_glGenerateMipmapEXT)(GLenum);
        void       (WINE_GLAPI *p_glGenerateMultiTexMipmapEXT)(GLenum,GLenum);
        void       (WINE_GLAPI *p_glGenerateTextureMipmap)(GLuint);
        void       (WINE_GLAPI *p_glGenerateTextureMipmapEXT)(GLuint,GLenum);
        void       (WINE_GLAPI *p_glGetActiveAtomicCounterBufferiv)(GLuint,GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetActiveAttrib)(GLuint,GLuint,GLsizei,GLsizei*,GLint*,GLenum*,GLchar*);
        void       (WINE_GLAPI *p_glGetActiveAttribARB)(GLhandleARB,GLuint,GLsizei,GLsizei*,GLint*,GLenum*,GLcharARB*);
        void       (WINE_GLAPI *p_glGetActiveSubroutineName)(GLuint,GLenum,GLuint,GLsizei,GLsizei*,GLchar*);
        void       (WINE_GLAPI *p_glGetActiveSubroutineUniformName)(GLuint,GLenum,GLuint,GLsizei,GLsizei*,GLchar*);
        void       (WINE_GLAPI *p_glGetActiveSubroutineUniformiv)(GLuint,GLenum,GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetActiveUniform)(GLuint,GLuint,GLsizei,GLsizei*,GLint*,GLenum*,GLchar*);
        void       (WINE_GLAPI *p_glGetActiveUniformARB)(GLhandleARB,GLuint,GLsizei,GLsizei*,GLint*,GLenum*,GLcharARB*);
        void       (WINE_GLAPI *p_glGetActiveUniformBlockName)(GLuint,GLuint,GLsizei,GLsizei*,GLchar*);
        void       (WINE_GLAPI *p_glGetActiveUniformBlockiv)(GLuint,GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetActiveUniformName)(GLuint,GLuint,GLsizei,GLsizei*,GLchar*);
        void       (WINE_GLAPI *p_glGetActiveUniformsiv)(GLuint,GLsizei,const GLuint*,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetActiveVaryingNV)(GLuint,GLuint,GLsizei,GLsizei*,GLsizei*,GLenum*,GLchar*);
        void       (WINE_GLAPI *p_glGetArrayObjectfvATI)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetArrayObjectivATI)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetAttachedObjectsARB)(GLhandleARB,GLsizei,GLsizei*,GLhandleARB*);
        void       (WINE_GLAPI *p_glGetAttachedShaders)(GLuint,GLsizei,GLsizei*,GLuint*);
        GLint      (WINE_GLAPI *p_glGetAttribLocation)(GLuint,const GLchar*);
        GLint      (WINE_GLAPI *p_glGetAttribLocationARB)(GLhandleARB,const GLcharARB*);
        void       (WINE_GLAPI *p_glGetBooleanIndexedvEXT)(GLenum,GLuint,GLboolean*);
        void       (WINE_GLAPI *p_glGetBooleani_v)(GLenum,GLuint,GLboolean*);
        void       (WINE_GLAPI *p_glGetBufferParameteri64v)(GLenum,GLenum,GLint64*);
        void       (WINE_GLAPI *p_glGetBufferParameteriv)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetBufferParameterivARB)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetBufferParameterui64vNV)(GLenum,GLenum,GLuint64EXT*);
        void       (WINE_GLAPI *p_glGetBufferPointerv)(GLenum,GLenum,void**);
        void       (WINE_GLAPI *p_glGetBufferPointervARB)(GLenum,GLenum,void**);
        void       (WINE_GLAPI *p_glGetBufferSubData)(GLenum,GLintptr,GLsizeiptr,void*);
        void       (WINE_GLAPI *p_glGetBufferSubDataARB)(GLenum,GLintptrARB,GLsizeiptrARB,void*);
        void       (WINE_GLAPI *p_glGetClipPlanefOES)(GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetClipPlanexOES)(GLenum,GLfixed*);
        void       (WINE_GLAPI *p_glGetColorTable)(GLenum,GLenum,GLenum,void*);
        void       (WINE_GLAPI *p_glGetColorTableEXT)(GLenum,GLenum,GLenum,void*);
        void       (WINE_GLAPI *p_glGetColorTableParameterfv)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetColorTableParameterfvEXT)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetColorTableParameterfvSGI)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetColorTableParameteriv)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetColorTableParameterivEXT)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetColorTableParameterivSGI)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetColorTableSGI)(GLenum,GLenum,GLenum,void*);
        void       (WINE_GLAPI *p_glGetCombinerInputParameterfvNV)(GLenum,GLenum,GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetCombinerInputParameterivNV)(GLenum,GLenum,GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetCombinerOutputParameterfvNV)(GLenum,GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetCombinerOutputParameterivNV)(GLenum,GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetCombinerStageParameterfvNV)(GLenum,GLenum,GLfloat*);
        GLuint     (WINE_GLAPI *p_glGetCommandHeaderNV)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glGetCompressedMultiTexImageEXT)(GLenum,GLenum,GLint,void*);
        void       (WINE_GLAPI *p_glGetCompressedTexImage)(GLenum,GLint,void*);
        void       (WINE_GLAPI *p_glGetCompressedTexImageARB)(GLenum,GLint,void*);
        void       (WINE_GLAPI *p_glGetCompressedTextureImage)(GLuint,GLint,GLsizei,void*);
        void       (WINE_GLAPI *p_glGetCompressedTextureImageEXT)(GLuint,GLenum,GLint,void*);
        void       (WINE_GLAPI *p_glGetCompressedTextureSubImage)(GLuint,GLint,GLint,GLint,GLint,GLsizei,GLsizei,GLsizei,GLsizei,void*);
        void       (WINE_GLAPI *p_glGetConvolutionFilter)(GLenum,GLenum,GLenum,void*);
        void       (WINE_GLAPI *p_glGetConvolutionFilterEXT)(GLenum,GLenum,GLenum,void*);
        void       (WINE_GLAPI *p_glGetConvolutionParameterfv)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetConvolutionParameterfvEXT)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetConvolutionParameteriv)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetConvolutionParameterivEXT)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetConvolutionParameterxvOES)(GLenum,GLenum,GLfixed*);
        void       (WINE_GLAPI *p_glGetCoverageModulationTableNV)(GLsizei,GLfloat*);
        GLuint     (WINE_GLAPI *p_glGetDebugMessageLog)(GLuint,GLsizei,GLenum*,GLenum*,GLuint*,GLenum*,GLsizei*,GLchar*);
        GLuint     (WINE_GLAPI *p_glGetDebugMessageLogAMD)(GLuint,GLsizei,GLenum*,GLuint*,GLuint*,GLsizei*,GLchar*);
        GLuint     (WINE_GLAPI *p_glGetDebugMessageLogARB)(GLuint,GLsizei,GLenum*,GLenum*,GLuint*,GLenum*,GLsizei*,GLchar*);
        void       (WINE_GLAPI *p_glGetDetailTexFuncSGIS)(GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetDoubleIndexedvEXT)(GLenum,GLuint,GLdouble*);
        void       (WINE_GLAPI *p_glGetDoublei_v)(GLenum,GLuint,GLdouble*);
        void       (WINE_GLAPI *p_glGetDoublei_vEXT)(GLenum,GLuint,GLdouble*);
        void       (WINE_GLAPI *p_glGetFenceivNV)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetFinalCombinerInputParameterfvNV)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetFinalCombinerInputParameterivNV)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetFirstPerfQueryIdINTEL)(GLuint*);
        void       (WINE_GLAPI *p_glGetFixedvOES)(GLenum,GLfixed*);
        void       (WINE_GLAPI *p_glGetFloatIndexedvEXT)(GLenum,GLuint,GLfloat*);
        void       (WINE_GLAPI *p_glGetFloati_v)(GLenum,GLuint,GLfloat*);
        void       (WINE_GLAPI *p_glGetFloati_vEXT)(GLenum,GLuint,GLfloat*);
        void       (WINE_GLAPI *p_glGetFogFuncSGIS)(GLfloat*);
        GLint      (WINE_GLAPI *p_glGetFragDataIndex)(GLuint,const GLchar*);
        GLint      (WINE_GLAPI *p_glGetFragDataLocation)(GLuint,const GLchar*);
        GLint      (WINE_GLAPI *p_glGetFragDataLocationEXT)(GLuint,const GLchar*);
        void       (WINE_GLAPI *p_glGetFragmentLightfvSGIX)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetFragmentLightivSGIX)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetFragmentMaterialfvSGIX)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetFragmentMaterialivSGIX)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetFramebufferAttachmentParameteriv)(GLenum,GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetFramebufferAttachmentParameterivEXT)(GLenum,GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetFramebufferParameteriv)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetFramebufferParameterivEXT)(GLuint,GLenum,GLint*);
        GLenum     (WINE_GLAPI *p_glGetGraphicsResetStatus)(void);
        GLenum     (WINE_GLAPI *p_glGetGraphicsResetStatusARB)(void);
        GLhandleARB (WINE_GLAPI *p_glGetHandleARB)(GLenum);
        void       (WINE_GLAPI *p_glGetHistogram)(GLenum,GLboolean,GLenum,GLenum,void*);
        void       (WINE_GLAPI *p_glGetHistogramEXT)(GLenum,GLboolean,GLenum,GLenum,void*);
        void       (WINE_GLAPI *p_glGetHistogramParameterfv)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetHistogramParameterfvEXT)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetHistogramParameteriv)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetHistogramParameterivEXT)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetHistogramParameterxvOES)(GLenum,GLenum,GLfixed*);
        GLuint64   (WINE_GLAPI *p_glGetImageHandleARB)(GLuint,GLint,GLboolean,GLint,GLenum);
        GLuint64   (WINE_GLAPI *p_glGetImageHandleNV)(GLuint,GLint,GLboolean,GLint,GLenum);
        void       (WINE_GLAPI *p_glGetImageTransformParameterfvHP)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetImageTransformParameterivHP)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetInfoLogARB)(GLhandleARB,GLsizei,GLsizei*,GLcharARB*);
        GLint      (WINE_GLAPI *p_glGetInstrumentsSGIX)(void);
        void       (WINE_GLAPI *p_glGetInteger64i_v)(GLenum,GLuint,GLint64*);
        void       (WINE_GLAPI *p_glGetInteger64v)(GLenum,GLint64*);
        void       (WINE_GLAPI *p_glGetIntegerIndexedvEXT)(GLenum,GLuint,GLint*);
        void       (WINE_GLAPI *p_glGetIntegeri_v)(GLenum,GLuint,GLint*);
        void       (WINE_GLAPI *p_glGetIntegerui64i_vNV)(GLenum,GLuint,GLuint64EXT*);
        void       (WINE_GLAPI *p_glGetIntegerui64vNV)(GLenum,GLuint64EXT*);
        void       (WINE_GLAPI *p_glGetInternalformatSampleivNV)(GLenum,GLenum,GLsizei,GLenum,GLsizei,GLint*);
        void       (WINE_GLAPI *p_glGetInternalformati64v)(GLenum,GLenum,GLenum,GLsizei,GLint64*);
        void       (WINE_GLAPI *p_glGetInternalformativ)(GLenum,GLenum,GLenum,GLsizei,GLint*);
        void       (WINE_GLAPI *p_glGetInvariantBooleanvEXT)(GLuint,GLenum,GLboolean*);
        void       (WINE_GLAPI *p_glGetInvariantFloatvEXT)(GLuint,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetInvariantIntegervEXT)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetLightxOES)(GLenum,GLenum,GLfixed*);
        void       (WINE_GLAPI *p_glGetListParameterfvSGIX)(GLuint,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetListParameterivSGIX)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetLocalConstantBooleanvEXT)(GLuint,GLenum,GLboolean*);
        void       (WINE_GLAPI *p_glGetLocalConstantFloatvEXT)(GLuint,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetLocalConstantIntegervEXT)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetMapAttribParameterfvNV)(GLenum,GLuint,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetMapAttribParameterivNV)(GLenum,GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetMapControlPointsNV)(GLenum,GLuint,GLenum,GLsizei,GLsizei,GLboolean,void*);
        void       (WINE_GLAPI *p_glGetMapParameterfvNV)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetMapParameterivNV)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetMapxvOES)(GLenum,GLenum,GLfixed*);
        void       (WINE_GLAPI *p_glGetMaterialxOES)(GLenum,GLenum,GLfixed);
        void       (WINE_GLAPI *p_glGetMinmax)(GLenum,GLboolean,GLenum,GLenum,void*);
        void       (WINE_GLAPI *p_glGetMinmaxEXT)(GLenum,GLboolean,GLenum,GLenum,void*);
        void       (WINE_GLAPI *p_glGetMinmaxParameterfv)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetMinmaxParameterfvEXT)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetMinmaxParameteriv)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetMinmaxParameterivEXT)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetMultiTexEnvfvEXT)(GLenum,GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetMultiTexEnvivEXT)(GLenum,GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetMultiTexGendvEXT)(GLenum,GLenum,GLenum,GLdouble*);
        void       (WINE_GLAPI *p_glGetMultiTexGenfvEXT)(GLenum,GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetMultiTexGenivEXT)(GLenum,GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetMultiTexImageEXT)(GLenum,GLenum,GLint,GLenum,GLenum,void*);
        void       (WINE_GLAPI *p_glGetMultiTexLevelParameterfvEXT)(GLenum,GLenum,GLint,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetMultiTexLevelParameterivEXT)(GLenum,GLenum,GLint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetMultiTexParameterIivEXT)(GLenum,GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetMultiTexParameterIuivEXT)(GLenum,GLenum,GLenum,GLuint*);
        void       (WINE_GLAPI *p_glGetMultiTexParameterfvEXT)(GLenum,GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetMultiTexParameterivEXT)(GLenum,GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetMultisamplefv)(GLenum,GLuint,GLfloat*);
        void       (WINE_GLAPI *p_glGetMultisamplefvNV)(GLenum,GLuint,GLfloat*);
        void       (WINE_GLAPI *p_glGetNamedBufferParameteri64v)(GLuint,GLenum,GLint64*);
        void       (WINE_GLAPI *p_glGetNamedBufferParameteriv)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetNamedBufferParameterivEXT)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetNamedBufferParameterui64vNV)(GLuint,GLenum,GLuint64EXT*);
        void       (WINE_GLAPI *p_glGetNamedBufferPointerv)(GLuint,GLenum,void**);
        void       (WINE_GLAPI *p_glGetNamedBufferPointervEXT)(GLuint,GLenum,void**);
        void       (WINE_GLAPI *p_glGetNamedBufferSubData)(GLuint,GLintptr,GLsizeiptr,void*);
        void       (WINE_GLAPI *p_glGetNamedBufferSubDataEXT)(GLuint,GLintptr,GLsizeiptr,void*);
        void       (WINE_GLAPI *p_glGetNamedFramebufferAttachmentParameteriv)(GLuint,GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetNamedFramebufferAttachmentParameterivEXT)(GLuint,GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetNamedFramebufferParameteriv)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetNamedFramebufferParameterivEXT)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetNamedProgramLocalParameterIivEXT)(GLuint,GLenum,GLuint,GLint*);
        void       (WINE_GLAPI *p_glGetNamedProgramLocalParameterIuivEXT)(GLuint,GLenum,GLuint,GLuint*);
        void       (WINE_GLAPI *p_glGetNamedProgramLocalParameterdvEXT)(GLuint,GLenum,GLuint,GLdouble*);
        void       (WINE_GLAPI *p_glGetNamedProgramLocalParameterfvEXT)(GLuint,GLenum,GLuint,GLfloat*);
        void       (WINE_GLAPI *p_glGetNamedProgramStringEXT)(GLuint,GLenum,GLenum,void*);
        void       (WINE_GLAPI *p_glGetNamedProgramivEXT)(GLuint,GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetNamedRenderbufferParameteriv)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetNamedRenderbufferParameterivEXT)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetNamedStringARB)(GLint,const GLchar*,GLsizei,GLint*,GLchar*);
        void       (WINE_GLAPI *p_glGetNamedStringivARB)(GLint,const GLchar*,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetNextPerfQueryIdINTEL)(GLuint,GLuint*);
        void       (WINE_GLAPI *p_glGetObjectBufferfvATI)(GLuint,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetObjectBufferivATI)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetObjectLabel)(GLenum,GLuint,GLsizei,GLsizei*,GLchar*);
        void       (WINE_GLAPI *p_glGetObjectLabelEXT)(GLenum,GLuint,GLsizei,GLsizei*,GLchar*);
        void       (WINE_GLAPI *p_glGetObjectParameterfvARB)(GLhandleARB,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetObjectParameterivAPPLE)(GLenum,GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetObjectParameterivARB)(GLhandleARB,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetObjectPtrLabel)(const void*,GLsizei,GLsizei*,GLchar*);
        void       (WINE_GLAPI *p_glGetOcclusionQueryivNV)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetOcclusionQueryuivNV)(GLuint,GLenum,GLuint*);
        void       (WINE_GLAPI *p_glGetPathColorGenfvNV)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetPathColorGenivNV)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetPathCommandsNV)(GLuint,GLubyte*);
        void       (WINE_GLAPI *p_glGetPathCoordsNV)(GLuint,GLfloat*);
        void       (WINE_GLAPI *p_glGetPathDashArrayNV)(GLuint,GLfloat*);
        GLfloat    (WINE_GLAPI *p_glGetPathLengthNV)(GLuint,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glGetPathMetricRangeNV)(GLbitfield,GLuint,GLsizei,GLsizei,GLfloat*);
        void       (WINE_GLAPI *p_glGetPathMetricsNV)(GLbitfield,GLsizei,GLenum,const void*,GLuint,GLsizei,GLfloat*);
        void       (WINE_GLAPI *p_glGetPathParameterfvNV)(GLuint,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetPathParameterivNV)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetPathSpacingNV)(GLenum,GLsizei,GLenum,const void*,GLuint,GLfloat,GLfloat,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetPathTexGenfvNV)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetPathTexGenivNV)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetPerfCounterInfoINTEL)(GLuint,GLuint,GLuint,GLchar*,GLuint,GLchar*,GLuint*,GLuint*,GLuint*,GLuint*,GLuint64*);
        void       (WINE_GLAPI *p_glGetPerfMonitorCounterDataAMD)(GLuint,GLenum,GLsizei,GLuint*,GLint*);
        void       (WINE_GLAPI *p_glGetPerfMonitorCounterInfoAMD)(GLuint,GLuint,GLenum,void*);
        void       (WINE_GLAPI *p_glGetPerfMonitorCounterStringAMD)(GLuint,GLuint,GLsizei,GLsizei*,GLchar*);
        void       (WINE_GLAPI *p_glGetPerfMonitorCountersAMD)(GLuint,GLint*,GLint*,GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glGetPerfMonitorGroupStringAMD)(GLuint,GLsizei,GLsizei*,GLchar*);
        void       (WINE_GLAPI *p_glGetPerfMonitorGroupsAMD)(GLint*,GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glGetPerfQueryDataINTEL)(GLuint,GLuint,GLsizei,GLvoid*,GLuint*);
        void       (WINE_GLAPI *p_glGetPerfQueryIdByNameINTEL)(GLchar*,GLuint*);
        void       (WINE_GLAPI *p_glGetPerfQueryInfoINTEL)(GLuint,GLuint,GLchar*,GLuint*,GLuint*,GLuint*,GLuint*);
        void       (WINE_GLAPI *p_glGetPixelMapxv)(GLenum,GLint,GLfixed*);
        void       (WINE_GLAPI *p_glGetPixelTexGenParameterfvSGIS)(GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetPixelTexGenParameterivSGIS)(GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetPixelTransformParameterfvEXT)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetPixelTransformParameterivEXT)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetPointerIndexedvEXT)(GLenum,GLuint,void**);
        void       (WINE_GLAPI *p_glGetPointeri_vEXT)(GLenum,GLuint,void**);
        void       (WINE_GLAPI *p_glGetPointervEXT)(GLenum,void**);
        void       (WINE_GLAPI *p_glGetProgramBinary)(GLuint,GLsizei,GLsizei*,GLenum*,void*);
        void       (WINE_GLAPI *p_glGetProgramEnvParameterIivNV)(GLenum,GLuint,GLint*);
        void       (WINE_GLAPI *p_glGetProgramEnvParameterIuivNV)(GLenum,GLuint,GLuint*);
        void       (WINE_GLAPI *p_glGetProgramEnvParameterdvARB)(GLenum,GLuint,GLdouble*);
        void       (WINE_GLAPI *p_glGetProgramEnvParameterfvARB)(GLenum,GLuint,GLfloat*);
        void       (WINE_GLAPI *p_glGetProgramInfoLog)(GLuint,GLsizei,GLsizei*,GLchar*);
        void       (WINE_GLAPI *p_glGetProgramInterfaceiv)(GLuint,GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetProgramLocalParameterIivNV)(GLenum,GLuint,GLint*);
        void       (WINE_GLAPI *p_glGetProgramLocalParameterIuivNV)(GLenum,GLuint,GLuint*);
        void       (WINE_GLAPI *p_glGetProgramLocalParameterdvARB)(GLenum,GLuint,GLdouble*);
        void       (WINE_GLAPI *p_glGetProgramLocalParameterfvARB)(GLenum,GLuint,GLfloat*);
        void       (WINE_GLAPI *p_glGetProgramNamedParameterdvNV)(GLuint,GLsizei,const GLubyte*,GLdouble*);
        void       (WINE_GLAPI *p_glGetProgramNamedParameterfvNV)(GLuint,GLsizei,const GLubyte*,GLfloat*);
        void       (WINE_GLAPI *p_glGetProgramParameterdvNV)(GLenum,GLuint,GLenum,GLdouble*);
        void       (WINE_GLAPI *p_glGetProgramParameterfvNV)(GLenum,GLuint,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetProgramPipelineInfoLog)(GLuint,GLsizei,GLsizei*,GLchar*);
        void       (WINE_GLAPI *p_glGetProgramPipelineiv)(GLuint,GLenum,GLint*);
        GLuint     (WINE_GLAPI *p_glGetProgramResourceIndex)(GLuint,GLenum,const GLchar*);
        GLint      (WINE_GLAPI *p_glGetProgramResourceLocation)(GLuint,GLenum,const GLchar*);
        GLint      (WINE_GLAPI *p_glGetProgramResourceLocationIndex)(GLuint,GLenum,const GLchar*);
        void       (WINE_GLAPI *p_glGetProgramResourceName)(GLuint,GLenum,GLuint,GLsizei,GLsizei*,GLchar*);
        void       (WINE_GLAPI *p_glGetProgramResourcefvNV)(GLuint,GLenum,GLuint,GLsizei,const GLenum*,GLsizei,GLsizei*,GLfloat*);
        void       (WINE_GLAPI *p_glGetProgramResourceiv)(GLuint,GLenum,GLuint,GLsizei,const GLenum*,GLsizei,GLsizei*,GLint*);
        void       (WINE_GLAPI *p_glGetProgramStageiv)(GLuint,GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetProgramStringARB)(GLenum,GLenum,void*);
        void       (WINE_GLAPI *p_glGetProgramStringNV)(GLuint,GLenum,GLubyte*);
        void       (WINE_GLAPI *p_glGetProgramSubroutineParameteruivNV)(GLenum,GLuint,GLuint*);
        void       (WINE_GLAPI *p_glGetProgramiv)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetProgramivARB)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetProgramivNV)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetQueryBufferObjecti64v)(GLuint,GLuint,GLenum,GLintptr);
        void       (WINE_GLAPI *p_glGetQueryBufferObjectiv)(GLuint,GLuint,GLenum,GLintptr);
        void       (WINE_GLAPI *p_glGetQueryBufferObjectui64v)(GLuint,GLuint,GLenum,GLintptr);
        void       (WINE_GLAPI *p_glGetQueryBufferObjectuiv)(GLuint,GLuint,GLenum,GLintptr);
        void       (WINE_GLAPI *p_glGetQueryIndexediv)(GLenum,GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetQueryObjecti64v)(GLuint,GLenum,GLint64*);
        void       (WINE_GLAPI *p_glGetQueryObjecti64vEXT)(GLuint,GLenum,GLint64*);
        void       (WINE_GLAPI *p_glGetQueryObjectiv)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetQueryObjectivARB)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetQueryObjectui64v)(GLuint,GLenum,GLuint64*);
        void       (WINE_GLAPI *p_glGetQueryObjectui64vEXT)(GLuint,GLenum,GLuint64*);
        void       (WINE_GLAPI *p_glGetQueryObjectuiv)(GLuint,GLenum,GLuint*);
        void       (WINE_GLAPI *p_glGetQueryObjectuivARB)(GLuint,GLenum,GLuint*);
        void       (WINE_GLAPI *p_glGetQueryiv)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetQueryivARB)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetRenderbufferParameteriv)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetRenderbufferParameterivEXT)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetSamplerParameterIiv)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetSamplerParameterIuiv)(GLuint,GLenum,GLuint*);
        void       (WINE_GLAPI *p_glGetSamplerParameterfv)(GLuint,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetSamplerParameteriv)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetSeparableFilter)(GLenum,GLenum,GLenum,void*,void*,void*);
        void       (WINE_GLAPI *p_glGetSeparableFilterEXT)(GLenum,GLenum,GLenum,void*,void*,void*);
        void       (WINE_GLAPI *p_glGetShaderInfoLog)(GLuint,GLsizei,GLsizei*,GLchar*);
        void       (WINE_GLAPI *p_glGetShaderPrecisionFormat)(GLenum,GLenum,GLint*,GLint*);
        void       (WINE_GLAPI *p_glGetShaderSource)(GLuint,GLsizei,GLsizei*,GLchar*);
        void       (WINE_GLAPI *p_glGetShaderSourceARB)(GLhandleARB,GLsizei,GLsizei*,GLcharARB*);
        void       (WINE_GLAPI *p_glGetShaderiv)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetSharpenTexFuncSGIS)(GLenum,GLfloat*);
        GLushort   (WINE_GLAPI *p_glGetStageIndexNV)(GLenum);
        const GLubyte* (WINE_GLAPI *p_glGetStringi)(GLenum,GLuint);
        GLuint     (WINE_GLAPI *p_glGetSubroutineIndex)(GLuint,GLenum,const GLchar*);
        GLint      (WINE_GLAPI *p_glGetSubroutineUniformLocation)(GLuint,GLenum,const GLchar*);
        void       (WINE_GLAPI *p_glGetSynciv)(GLsync,GLenum,GLsizei,GLsizei*,GLint*);
        void       (WINE_GLAPI *p_glGetTexBumpParameterfvATI)(GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetTexBumpParameterivATI)(GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetTexEnvxvOES)(GLenum,GLenum,GLfixed*);
        void       (WINE_GLAPI *p_glGetTexFilterFuncSGIS)(GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetTexGenxvOES)(GLenum,GLenum,GLfixed*);
        void       (WINE_GLAPI *p_glGetTexLevelParameterxvOES)(GLenum,GLint,GLenum,GLfixed*);
        void       (WINE_GLAPI *p_glGetTexParameterIiv)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetTexParameterIivEXT)(GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetTexParameterIuiv)(GLenum,GLenum,GLuint*);
        void       (WINE_GLAPI *p_glGetTexParameterIuivEXT)(GLenum,GLenum,GLuint*);
        void       (WINE_GLAPI *p_glGetTexParameterPointervAPPLE)(GLenum,GLenum,void**);
        void       (WINE_GLAPI *p_glGetTexParameterxvOES)(GLenum,GLenum,GLfixed*);
        GLuint64   (WINE_GLAPI *p_glGetTextureHandleARB)(GLuint);
        GLuint64   (WINE_GLAPI *p_glGetTextureHandleNV)(GLuint);
        void       (WINE_GLAPI *p_glGetTextureImage)(GLuint,GLint,GLenum,GLenum,GLsizei,void*);
        void       (WINE_GLAPI *p_glGetTextureImageEXT)(GLuint,GLenum,GLint,GLenum,GLenum,void*);
        void       (WINE_GLAPI *p_glGetTextureLevelParameterfv)(GLuint,GLint,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetTextureLevelParameterfvEXT)(GLuint,GLenum,GLint,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetTextureLevelParameteriv)(GLuint,GLint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetTextureLevelParameterivEXT)(GLuint,GLenum,GLint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetTextureParameterIiv)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetTextureParameterIivEXT)(GLuint,GLenum,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetTextureParameterIuiv)(GLuint,GLenum,GLuint*);
        void       (WINE_GLAPI *p_glGetTextureParameterIuivEXT)(GLuint,GLenum,GLenum,GLuint*);
        void       (WINE_GLAPI *p_glGetTextureParameterfv)(GLuint,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetTextureParameterfvEXT)(GLuint,GLenum,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetTextureParameteriv)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetTextureParameterivEXT)(GLuint,GLenum,GLenum,GLint*);
        GLuint64   (WINE_GLAPI *p_glGetTextureSamplerHandleARB)(GLuint,GLuint);
        GLuint64   (WINE_GLAPI *p_glGetTextureSamplerHandleNV)(GLuint,GLuint);
        void       (WINE_GLAPI *p_glGetTextureSubImage)(GLuint,GLint,GLint,GLint,GLint,GLsizei,GLsizei,GLsizei,GLenum,GLenum,GLsizei,void*);
        void       (WINE_GLAPI *p_glGetTrackMatrixivNV)(GLenum,GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetTransformFeedbackVarying)(GLuint,GLuint,GLsizei,GLsizei*,GLsizei*,GLenum*,GLchar*);
        void       (WINE_GLAPI *p_glGetTransformFeedbackVaryingEXT)(GLuint,GLuint,GLsizei,GLsizei*,GLsizei*,GLenum*,GLchar*);
        void       (WINE_GLAPI *p_glGetTransformFeedbackVaryingNV)(GLuint,GLuint,GLint*);
        void       (WINE_GLAPI *p_glGetTransformFeedbacki64_v)(GLuint,GLenum,GLuint,GLint64*);
        void       (WINE_GLAPI *p_glGetTransformFeedbacki_v)(GLuint,GLenum,GLuint,GLint*);
        void       (WINE_GLAPI *p_glGetTransformFeedbackiv)(GLuint,GLenum,GLint*);
        GLuint     (WINE_GLAPI *p_glGetUniformBlockIndex)(GLuint,const GLchar*);
        GLint      (WINE_GLAPI *p_glGetUniformBufferSizeEXT)(GLuint,GLint);
        void       (WINE_GLAPI *p_glGetUniformIndices)(GLuint,GLsizei,const GLchar*const*,GLuint*);
        GLint      (WINE_GLAPI *p_glGetUniformLocation)(GLuint,const GLchar*);
        GLint      (WINE_GLAPI *p_glGetUniformLocationARB)(GLhandleARB,const GLcharARB*);
        GLintptr   (WINE_GLAPI *p_glGetUniformOffsetEXT)(GLuint,GLint);
        void       (WINE_GLAPI *p_glGetUniformSubroutineuiv)(GLenum,GLint,GLuint*);
        void       (WINE_GLAPI *p_glGetUniformdv)(GLuint,GLint,GLdouble*);
        void       (WINE_GLAPI *p_glGetUniformfv)(GLuint,GLint,GLfloat*);
        void       (WINE_GLAPI *p_glGetUniformfvARB)(GLhandleARB,GLint,GLfloat*);
        void       (WINE_GLAPI *p_glGetUniformi64vNV)(GLuint,GLint,GLint64EXT*);
        void       (WINE_GLAPI *p_glGetUniformiv)(GLuint,GLint,GLint*);
        void       (WINE_GLAPI *p_glGetUniformivARB)(GLhandleARB,GLint,GLint*);
        void       (WINE_GLAPI *p_glGetUniformui64vNV)(GLuint,GLint,GLuint64EXT*);
        void       (WINE_GLAPI *p_glGetUniformuiv)(GLuint,GLint,GLuint*);
        void       (WINE_GLAPI *p_glGetUniformuivEXT)(GLuint,GLint,GLuint*);
        void       (WINE_GLAPI *p_glGetVariantArrayObjectfvATI)(GLuint,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetVariantArrayObjectivATI)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetVariantBooleanvEXT)(GLuint,GLenum,GLboolean*);
        void       (WINE_GLAPI *p_glGetVariantFloatvEXT)(GLuint,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetVariantIntegervEXT)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetVariantPointervEXT)(GLuint,GLenum,void**);
        GLint      (WINE_GLAPI *p_glGetVaryingLocationNV)(GLuint,const GLchar*);
        void       (WINE_GLAPI *p_glGetVertexArrayIndexed64iv)(GLuint,GLuint,GLenum,GLint64*);
        void       (WINE_GLAPI *p_glGetVertexArrayIndexediv)(GLuint,GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetVertexArrayIntegeri_vEXT)(GLuint,GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetVertexArrayIntegervEXT)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetVertexArrayPointeri_vEXT)(GLuint,GLuint,GLenum,void**);
        void       (WINE_GLAPI *p_glGetVertexArrayPointervEXT)(GLuint,GLenum,void**);
        void       (WINE_GLAPI *p_glGetVertexArrayiv)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetVertexAttribArrayObjectfvATI)(GLuint,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetVertexAttribArrayObjectivATI)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetVertexAttribIiv)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetVertexAttribIivEXT)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetVertexAttribIuiv)(GLuint,GLenum,GLuint*);
        void       (WINE_GLAPI *p_glGetVertexAttribIuivEXT)(GLuint,GLenum,GLuint*);
        void       (WINE_GLAPI *p_glGetVertexAttribLdv)(GLuint,GLenum,GLdouble*);
        void       (WINE_GLAPI *p_glGetVertexAttribLdvEXT)(GLuint,GLenum,GLdouble*);
        void       (WINE_GLAPI *p_glGetVertexAttribLi64vNV)(GLuint,GLenum,GLint64EXT*);
        void       (WINE_GLAPI *p_glGetVertexAttribLui64vARB)(GLuint,GLenum,GLuint64EXT*);
        void       (WINE_GLAPI *p_glGetVertexAttribLui64vNV)(GLuint,GLenum,GLuint64EXT*);
        void       (WINE_GLAPI *p_glGetVertexAttribPointerv)(GLuint,GLenum,void**);
        void       (WINE_GLAPI *p_glGetVertexAttribPointervARB)(GLuint,GLenum,void**);
        void       (WINE_GLAPI *p_glGetVertexAttribPointervNV)(GLuint,GLenum,void**);
        void       (WINE_GLAPI *p_glGetVertexAttribdv)(GLuint,GLenum,GLdouble*);
        void       (WINE_GLAPI *p_glGetVertexAttribdvARB)(GLuint,GLenum,GLdouble*);
        void       (WINE_GLAPI *p_glGetVertexAttribdvNV)(GLuint,GLenum,GLdouble*);
        void       (WINE_GLAPI *p_glGetVertexAttribfv)(GLuint,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetVertexAttribfvARB)(GLuint,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetVertexAttribfvNV)(GLuint,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetVertexAttribiv)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetVertexAttribivARB)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetVertexAttribivNV)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetVideoCaptureStreamdvNV)(GLuint,GLuint,GLenum,GLdouble*);
        void       (WINE_GLAPI *p_glGetVideoCaptureStreamfvNV)(GLuint,GLuint,GLenum,GLfloat*);
        void       (WINE_GLAPI *p_glGetVideoCaptureStreamivNV)(GLuint,GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetVideoCaptureivNV)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetVideoi64vNV)(GLuint,GLenum,GLint64EXT*);
        void       (WINE_GLAPI *p_glGetVideoivNV)(GLuint,GLenum,GLint*);
        void       (WINE_GLAPI *p_glGetVideoui64vNV)(GLuint,GLenum,GLuint64EXT*);
        void       (WINE_GLAPI *p_glGetVideouivNV)(GLuint,GLenum,GLuint*);
        void       (WINE_GLAPI *p_glGetnColorTable)(GLenum,GLenum,GLenum,GLsizei,void*);
        void       (WINE_GLAPI *p_glGetnColorTableARB)(GLenum,GLenum,GLenum,GLsizei,void*);
        void       (WINE_GLAPI *p_glGetnCompressedTexImage)(GLenum,GLint,GLsizei,void*);
        void       (WINE_GLAPI *p_glGetnCompressedTexImageARB)(GLenum,GLint,GLsizei,void*);
        void       (WINE_GLAPI *p_glGetnConvolutionFilter)(GLenum,GLenum,GLenum,GLsizei,void*);
        void       (WINE_GLAPI *p_glGetnConvolutionFilterARB)(GLenum,GLenum,GLenum,GLsizei,void*);
        void       (WINE_GLAPI *p_glGetnHistogram)(GLenum,GLboolean,GLenum,GLenum,GLsizei,void*);
        void       (WINE_GLAPI *p_glGetnHistogramARB)(GLenum,GLboolean,GLenum,GLenum,GLsizei,void*);
        void       (WINE_GLAPI *p_glGetnMapdv)(GLenum,GLenum,GLsizei,GLdouble*);
        void       (WINE_GLAPI *p_glGetnMapdvARB)(GLenum,GLenum,GLsizei,GLdouble*);
        void       (WINE_GLAPI *p_glGetnMapfv)(GLenum,GLenum,GLsizei,GLfloat*);
        void       (WINE_GLAPI *p_glGetnMapfvARB)(GLenum,GLenum,GLsizei,GLfloat*);
        void       (WINE_GLAPI *p_glGetnMapiv)(GLenum,GLenum,GLsizei,GLint*);
        void       (WINE_GLAPI *p_glGetnMapivARB)(GLenum,GLenum,GLsizei,GLint*);
        void       (WINE_GLAPI *p_glGetnMinmax)(GLenum,GLboolean,GLenum,GLenum,GLsizei,void*);
        void       (WINE_GLAPI *p_glGetnMinmaxARB)(GLenum,GLboolean,GLenum,GLenum,GLsizei,void*);
        void       (WINE_GLAPI *p_glGetnPixelMapfv)(GLenum,GLsizei,GLfloat*);
        void       (WINE_GLAPI *p_glGetnPixelMapfvARB)(GLenum,GLsizei,GLfloat*);
        void       (WINE_GLAPI *p_glGetnPixelMapuiv)(GLenum,GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glGetnPixelMapuivARB)(GLenum,GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glGetnPixelMapusv)(GLenum,GLsizei,GLushort*);
        void       (WINE_GLAPI *p_glGetnPixelMapusvARB)(GLenum,GLsizei,GLushort*);
        void       (WINE_GLAPI *p_glGetnPolygonStipple)(GLsizei,GLubyte*);
        void       (WINE_GLAPI *p_glGetnPolygonStippleARB)(GLsizei,GLubyte*);
        void       (WINE_GLAPI *p_glGetnSeparableFilter)(GLenum,GLenum,GLenum,GLsizei,void*,GLsizei,void*,void*);
        void       (WINE_GLAPI *p_glGetnSeparableFilterARB)(GLenum,GLenum,GLenum,GLsizei,void*,GLsizei,void*,void*);
        void       (WINE_GLAPI *p_glGetnTexImage)(GLenum,GLint,GLenum,GLenum,GLsizei,void*);
        void       (WINE_GLAPI *p_glGetnTexImageARB)(GLenum,GLint,GLenum,GLenum,GLsizei,void*);
        void       (WINE_GLAPI *p_glGetnUniformdv)(GLuint,GLint,GLsizei,GLdouble*);
        void       (WINE_GLAPI *p_glGetnUniformdvARB)(GLuint,GLint,GLsizei,GLdouble*);
        void       (WINE_GLAPI *p_glGetnUniformfv)(GLuint,GLint,GLsizei,GLfloat*);
        void       (WINE_GLAPI *p_glGetnUniformfvARB)(GLuint,GLint,GLsizei,GLfloat*);
        void       (WINE_GLAPI *p_glGetnUniformiv)(GLuint,GLint,GLsizei,GLint*);
        void       (WINE_GLAPI *p_glGetnUniformivARB)(GLuint,GLint,GLsizei,GLint*);
        void       (WINE_GLAPI *p_glGetnUniformuiv)(GLuint,GLint,GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glGetnUniformuivARB)(GLuint,GLint,GLsizei,GLuint*);
        void       (WINE_GLAPI *p_glGlobalAlphaFactorbSUN)(GLbyte);
        void       (WINE_GLAPI *p_glGlobalAlphaFactordSUN)(GLdouble);
        void       (WINE_GLAPI *p_glGlobalAlphaFactorfSUN)(GLfloat);
        void       (WINE_GLAPI *p_glGlobalAlphaFactoriSUN)(GLint);
        void       (WINE_GLAPI *p_glGlobalAlphaFactorsSUN)(GLshort);
        void       (WINE_GLAPI *p_glGlobalAlphaFactorubSUN)(GLubyte);
        void       (WINE_GLAPI *p_glGlobalAlphaFactoruiSUN)(GLuint);
        void       (WINE_GLAPI *p_glGlobalAlphaFactorusSUN)(GLushort);
        void       (WINE_GLAPI *p_glHintPGI)(GLenum,GLint);
        void       (WINE_GLAPI *p_glHistogram)(GLenum,GLsizei,GLenum,GLboolean);
        void       (WINE_GLAPI *p_glHistogramEXT)(GLenum,GLsizei,GLenum,GLboolean);
        void       (WINE_GLAPI *p_glIglooInterfaceSGIX)(GLenum,const void*);
        void       (WINE_GLAPI *p_glImageTransformParameterfHP)(GLenum,GLenum,GLfloat);
        void       (WINE_GLAPI *p_glImageTransformParameterfvHP)(GLenum,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glImageTransformParameteriHP)(GLenum,GLenum,GLint);
        void       (WINE_GLAPI *p_glImageTransformParameterivHP)(GLenum,GLenum,const GLint*);
        GLsync     (WINE_GLAPI *p_glImportSyncEXT)(GLenum,GLintptr,GLbitfield);
        void       (WINE_GLAPI *p_glIndexFormatNV)(GLenum,GLsizei);
        void       (WINE_GLAPI *p_glIndexFuncEXT)(GLenum,GLclampf);
        void       (WINE_GLAPI *p_glIndexMaterialEXT)(GLenum,GLenum);
        void       (WINE_GLAPI *p_glIndexPointerEXT)(GLenum,GLsizei,GLsizei,const void*);
        void       (WINE_GLAPI *p_glIndexPointerListIBM)(GLenum,GLint,const void**,GLint);
        void       (WINE_GLAPI *p_glIndexxOES)(GLfixed);
        void       (WINE_GLAPI *p_glIndexxvOES)(const GLfixed*);
        void       (WINE_GLAPI *p_glInsertComponentEXT)(GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glInsertEventMarkerEXT)(GLsizei,const GLchar*);
        void       (WINE_GLAPI *p_glInstrumentsBufferSGIX)(GLsizei,GLint*);
        void       (WINE_GLAPI *p_glInterpolatePathsNV)(GLuint,GLuint,GLuint,GLfloat);
        void       (WINE_GLAPI *p_glInvalidateBufferData)(GLuint);
        void       (WINE_GLAPI *p_glInvalidateBufferSubData)(GLuint,GLintptr,GLsizeiptr);
        void       (WINE_GLAPI *p_glInvalidateFramebuffer)(GLenum,GLsizei,const GLenum*);
        void       (WINE_GLAPI *p_glInvalidateNamedFramebufferData)(GLuint,GLsizei,const GLenum*);
        void       (WINE_GLAPI *p_glInvalidateNamedFramebufferSubData)(GLuint,GLsizei,const GLenum*,GLint,GLint,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glInvalidateSubFramebuffer)(GLenum,GLsizei,const GLenum*,GLint,GLint,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glInvalidateTexImage)(GLuint,GLint);
        void       (WINE_GLAPI *p_glInvalidateTexSubImage)(GLuint,GLint,GLint,GLint,GLint,GLsizei,GLsizei,GLsizei);
        GLboolean  (WINE_GLAPI *p_glIsAsyncMarkerSGIX)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsBuffer)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsBufferARB)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsBufferResidentNV)(GLenum);
        GLboolean  (WINE_GLAPI *p_glIsCommandListNV)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsEnabledIndexedEXT)(GLenum,GLuint);
        GLboolean  (WINE_GLAPI *p_glIsEnabledi)(GLenum,GLuint);
        GLboolean  (WINE_GLAPI *p_glIsFenceAPPLE)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsFenceNV)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsFramebuffer)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsFramebufferEXT)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsImageHandleResidentARB)(GLuint64);
        GLboolean  (WINE_GLAPI *p_glIsImageHandleResidentNV)(GLuint64);
        GLboolean  (WINE_GLAPI *p_glIsNameAMD)(GLenum,GLuint);
        GLboolean  (WINE_GLAPI *p_glIsNamedBufferResidentNV)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsNamedStringARB)(GLint,const GLchar*);
        GLboolean  (WINE_GLAPI *p_glIsObjectBufferATI)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsOcclusionQueryNV)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsPathNV)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsPointInFillPathNV)(GLuint,GLuint,GLfloat,GLfloat);
        GLboolean  (WINE_GLAPI *p_glIsPointInStrokePathNV)(GLuint,GLfloat,GLfloat);
        GLboolean  (WINE_GLAPI *p_glIsProgram)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsProgramARB)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsProgramNV)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsProgramPipeline)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsQuery)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsQueryARB)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsRenderbuffer)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsRenderbufferEXT)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsSampler)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsShader)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsStateNV)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsSync)(GLsync);
        GLboolean  (WINE_GLAPI *p_glIsTextureEXT)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsTextureHandleResidentARB)(GLuint64);
        GLboolean  (WINE_GLAPI *p_glIsTextureHandleResidentNV)(GLuint64);
        GLboolean  (WINE_GLAPI *p_glIsTransformFeedback)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsTransformFeedbackNV)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsVariantEnabledEXT)(GLuint,GLenum);
        GLboolean  (WINE_GLAPI *p_glIsVertexArray)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsVertexArrayAPPLE)(GLuint);
        GLboolean  (WINE_GLAPI *p_glIsVertexAttribEnabledAPPLE)(GLuint,GLenum);
        void       (WINE_GLAPI *p_glLabelObjectEXT)(GLenum,GLuint,GLsizei,const GLchar*);
        void       (WINE_GLAPI *p_glLightEnviSGIX)(GLenum,GLint);
        void       (WINE_GLAPI *p_glLightModelxOES)(GLenum,GLfixed);
        void       (WINE_GLAPI *p_glLightModelxvOES)(GLenum,const GLfixed*);
        void       (WINE_GLAPI *p_glLightxOES)(GLenum,GLenum,GLfixed);
        void       (WINE_GLAPI *p_glLightxvOES)(GLenum,GLenum,const GLfixed*);
        void       (WINE_GLAPI *p_glLineWidthxOES)(GLfixed);
        void       (WINE_GLAPI *p_glLinkProgram)(GLuint);
        void       (WINE_GLAPI *p_glLinkProgramARB)(GLhandleARB);
        void       (WINE_GLAPI *p_glListDrawCommandsStatesClientNV)(GLuint,GLuint,const void**,const GLsizei*,const GLuint*,const GLuint*,GLuint);
        void       (WINE_GLAPI *p_glListParameterfSGIX)(GLuint,GLenum,GLfloat);
        void       (WINE_GLAPI *p_glListParameterfvSGIX)(GLuint,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glListParameteriSGIX)(GLuint,GLenum,GLint);
        void       (WINE_GLAPI *p_glListParameterivSGIX)(GLuint,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glLoadIdentityDeformationMapSGIX)(GLbitfield);
        void       (WINE_GLAPI *p_glLoadMatrixxOES)(const GLfixed*);
        void       (WINE_GLAPI *p_glLoadProgramNV)(GLenum,GLuint,GLsizei,const GLubyte*);
        void       (WINE_GLAPI *p_glLoadTransposeMatrixd)(const GLdouble*);
        void       (WINE_GLAPI *p_glLoadTransposeMatrixdARB)(const GLdouble*);
        void       (WINE_GLAPI *p_glLoadTransposeMatrixf)(const GLfloat*);
        void       (WINE_GLAPI *p_glLoadTransposeMatrixfARB)(const GLfloat*);
        void       (WINE_GLAPI *p_glLoadTransposeMatrixxOES)(const GLfixed*);
        void       (WINE_GLAPI *p_glLockArraysEXT)(GLint,GLsizei);
        void       (WINE_GLAPI *p_glMTexCoord2fSGIS)(GLenum,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glMTexCoord2fvSGIS)(GLenum,GLfloat *);
        void       (WINE_GLAPI *p_glMakeBufferNonResidentNV)(GLenum);
        void       (WINE_GLAPI *p_glMakeBufferResidentNV)(GLenum,GLenum);
        void       (WINE_GLAPI *p_glMakeImageHandleNonResidentARB)(GLuint64);
        void       (WINE_GLAPI *p_glMakeImageHandleNonResidentNV)(GLuint64);
        void       (WINE_GLAPI *p_glMakeImageHandleResidentARB)(GLuint64,GLenum);
        void       (WINE_GLAPI *p_glMakeImageHandleResidentNV)(GLuint64,GLenum);
        void       (WINE_GLAPI *p_glMakeNamedBufferNonResidentNV)(GLuint);
        void       (WINE_GLAPI *p_glMakeNamedBufferResidentNV)(GLuint,GLenum);
        void       (WINE_GLAPI *p_glMakeTextureHandleNonResidentARB)(GLuint64);
        void       (WINE_GLAPI *p_glMakeTextureHandleNonResidentNV)(GLuint64);
        void       (WINE_GLAPI *p_glMakeTextureHandleResidentARB)(GLuint64);
        void       (WINE_GLAPI *p_glMakeTextureHandleResidentNV)(GLuint64);
        void       (WINE_GLAPI *p_glMap1xOES)(GLenum,GLfixed,GLfixed,GLint,GLint,GLfixed);
        void       (WINE_GLAPI *p_glMap2xOES)(GLenum,GLfixed,GLfixed,GLint,GLint,GLfixed,GLfixed,GLint,GLint,GLfixed);
        void*      (WINE_GLAPI *p_glMapBuffer)(GLenum,GLenum);
        void*      (WINE_GLAPI *p_glMapBufferARB)(GLenum,GLenum);
        void*      (WINE_GLAPI *p_glMapBufferRange)(GLenum,GLintptr,GLsizeiptr,GLbitfield);
        void       (WINE_GLAPI *p_glMapControlPointsNV)(GLenum,GLuint,GLenum,GLsizei,GLsizei,GLint,GLint,GLboolean,const void*);
        void       (WINE_GLAPI *p_glMapGrid1xOES)(GLint,GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glMapGrid2xOES)(GLint,GLfixed,GLfixed,GLfixed,GLfixed);
        void*      (WINE_GLAPI *p_glMapNamedBuffer)(GLuint,GLenum);
        void*      (WINE_GLAPI *p_glMapNamedBufferEXT)(GLuint,GLenum);
        void*      (WINE_GLAPI *p_glMapNamedBufferRange)(GLuint,GLintptr,GLsizeiptr,GLbitfield);
        void*      (WINE_GLAPI *p_glMapNamedBufferRangeEXT)(GLuint,GLintptr,GLsizeiptr,GLbitfield);
        void*      (WINE_GLAPI *p_glMapObjectBufferATI)(GLuint);
        void       (WINE_GLAPI *p_glMapParameterfvNV)(GLenum,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glMapParameterivNV)(GLenum,GLenum,const GLint*);
        void*      (WINE_GLAPI *p_glMapTexture2DINTEL)(GLuint,GLint,GLbitfield,GLint*,GLenum*);
        void       (WINE_GLAPI *p_glMapVertexAttrib1dAPPLE)(GLuint,GLuint,GLdouble,GLdouble,GLint,GLint,const GLdouble*);
        void       (WINE_GLAPI *p_glMapVertexAttrib1fAPPLE)(GLuint,GLuint,GLfloat,GLfloat,GLint,GLint,const GLfloat*);
        void       (WINE_GLAPI *p_glMapVertexAttrib2dAPPLE)(GLuint,GLuint,GLdouble,GLdouble,GLint,GLint,GLdouble,GLdouble,GLint,GLint,const GLdouble*);
        void       (WINE_GLAPI *p_glMapVertexAttrib2fAPPLE)(GLuint,GLuint,GLfloat,GLfloat,GLint,GLint,GLfloat,GLfloat,GLint,GLint,const GLfloat*);
        void       (WINE_GLAPI *p_glMaterialxOES)(GLenum,GLenum,GLfixed);
        void       (WINE_GLAPI *p_glMaterialxvOES)(GLenum,GLenum,const GLfixed*);
        void       (WINE_GLAPI *p_glMatrixFrustumEXT)(GLenum,GLdouble,GLdouble,GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glMatrixIndexPointerARB)(GLint,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glMatrixIndexubvARB)(GLint,const GLubyte*);
        void       (WINE_GLAPI *p_glMatrixIndexuivARB)(GLint,const GLuint*);
        void       (WINE_GLAPI *p_glMatrixIndexusvARB)(GLint,const GLushort*);
        void       (WINE_GLAPI *p_glMatrixLoad3x2fNV)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glMatrixLoad3x3fNV)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glMatrixLoadIdentityEXT)(GLenum);
        void       (WINE_GLAPI *p_glMatrixLoadTranspose3x3fNV)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glMatrixLoadTransposedEXT)(GLenum,const GLdouble*);
        void       (WINE_GLAPI *p_glMatrixLoadTransposefEXT)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glMatrixLoaddEXT)(GLenum,const GLdouble*);
        void       (WINE_GLAPI *p_glMatrixLoadfEXT)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glMatrixMult3x2fNV)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glMatrixMult3x3fNV)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glMatrixMultTranspose3x3fNV)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glMatrixMultTransposedEXT)(GLenum,const GLdouble*);
        void       (WINE_GLAPI *p_glMatrixMultTransposefEXT)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glMatrixMultdEXT)(GLenum,const GLdouble*);
        void       (WINE_GLAPI *p_glMatrixMultfEXT)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glMatrixOrthoEXT)(GLenum,GLdouble,GLdouble,GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glMatrixPopEXT)(GLenum);
        void       (WINE_GLAPI *p_glMatrixPushEXT)(GLenum);
        void       (WINE_GLAPI *p_glMatrixRotatedEXT)(GLenum,GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glMatrixRotatefEXT)(GLenum,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glMatrixScaledEXT)(GLenum,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glMatrixScalefEXT)(GLenum,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glMatrixTranslatedEXT)(GLenum,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glMatrixTranslatefEXT)(GLenum,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glMemoryBarrier)(GLbitfield);
        void       (WINE_GLAPI *p_glMemoryBarrierByRegion)(GLbitfield);
        void       (WINE_GLAPI *p_glMemoryBarrierEXT)(GLbitfield);
        void       (WINE_GLAPI *p_glMinSampleShading)(GLfloat);
        void       (WINE_GLAPI *p_glMinSampleShadingARB)(GLfloat);
        void       (WINE_GLAPI *p_glMinmax)(GLenum,GLenum,GLboolean);
        void       (WINE_GLAPI *p_glMinmaxEXT)(GLenum,GLenum,GLboolean);
        void       (WINE_GLAPI *p_glMultMatrixxOES)(const GLfixed*);
        void       (WINE_GLAPI *p_glMultTransposeMatrixd)(const GLdouble*);
        void       (WINE_GLAPI *p_glMultTransposeMatrixdARB)(const GLdouble*);
        void       (WINE_GLAPI *p_glMultTransposeMatrixf)(const GLfloat*);
        void       (WINE_GLAPI *p_glMultTransposeMatrixfARB)(const GLfloat*);
        void       (WINE_GLAPI *p_glMultTransposeMatrixxOES)(const GLfixed*);
        void       (WINE_GLAPI *p_glMultiDrawArrays)(GLenum,const GLint*,const GLsizei*,GLsizei);
        void       (WINE_GLAPI *p_glMultiDrawArraysEXT)(GLenum,const GLint*,const GLsizei*,GLsizei);
        void       (WINE_GLAPI *p_glMultiDrawArraysIndirect)(GLenum,const void*,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glMultiDrawArraysIndirectAMD)(GLenum,const void*,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glMultiDrawArraysIndirectBindlessCountNV)(GLenum,const void*,GLsizei,GLsizei,GLsizei,GLint);
        void       (WINE_GLAPI *p_glMultiDrawArraysIndirectBindlessNV)(GLenum,const void*,GLsizei,GLsizei,GLint);
        void       (WINE_GLAPI *p_glMultiDrawArraysIndirectCountARB)(GLenum,GLintptr,GLintptr,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glMultiDrawElementArrayAPPLE)(GLenum,const GLint*,const GLsizei*,GLsizei);
        void       (WINE_GLAPI *p_glMultiDrawElements)(GLenum,const GLsizei*,GLenum,const void*const*,GLsizei);
        void       (WINE_GLAPI *p_glMultiDrawElementsBaseVertex)(GLenum,const GLsizei*,GLenum,const void*const*,GLsizei,const GLint*);
        void       (WINE_GLAPI *p_glMultiDrawElementsEXT)(GLenum,const GLsizei*,GLenum,const void*const*,GLsizei);
        void       (WINE_GLAPI *p_glMultiDrawElementsIndirect)(GLenum,GLenum,const void*,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glMultiDrawElementsIndirectAMD)(GLenum,GLenum,const void*,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glMultiDrawElementsIndirectBindlessCountNV)(GLenum,GLenum,const void*,GLsizei,GLsizei,GLsizei,GLint);
        void       (WINE_GLAPI *p_glMultiDrawElementsIndirectBindlessNV)(GLenum,GLenum,const void*,GLsizei,GLsizei,GLint);
        void       (WINE_GLAPI *p_glMultiDrawElementsIndirectCountARB)(GLenum,GLenum,GLintptr,GLintptr,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glMultiDrawRangeElementArrayAPPLE)(GLenum,GLuint,GLuint,const GLint*,const GLsizei*,GLsizei);
        void       (WINE_GLAPI *p_glMultiModeDrawArraysIBM)(const GLenum*,const GLint*,const GLsizei*,GLsizei,GLint);
        void       (WINE_GLAPI *p_glMultiModeDrawElementsIBM)(const GLenum*,const GLsizei*,GLenum,const void*const*,GLsizei,GLint);
        void       (WINE_GLAPI *p_glMultiTexBufferEXT)(GLenum,GLenum,GLenum,GLuint);
        void       (WINE_GLAPI *p_glMultiTexCoord1bOES)(GLenum,GLbyte);
        void       (WINE_GLAPI *p_glMultiTexCoord1bvOES)(GLenum,const GLbyte*);
        void       (WINE_GLAPI *p_glMultiTexCoord1d)(GLenum,GLdouble);
        void       (WINE_GLAPI *p_glMultiTexCoord1dARB)(GLenum,GLdouble);
        void       (WINE_GLAPI *p_glMultiTexCoord1dSGIS)(GLenum,GLdouble);
        void       (WINE_GLAPI *p_glMultiTexCoord1dv)(GLenum,const GLdouble*);
        void       (WINE_GLAPI *p_glMultiTexCoord1dvARB)(GLenum,const GLdouble*);
        void       (WINE_GLAPI *p_glMultiTexCoord1dvSGIS)(GLenum,GLdouble *);
        void       (WINE_GLAPI *p_glMultiTexCoord1f)(GLenum,GLfloat);
        void       (WINE_GLAPI *p_glMultiTexCoord1fARB)(GLenum,GLfloat);
        void       (WINE_GLAPI *p_glMultiTexCoord1fSGIS)(GLenum,GLfloat);
        void       (WINE_GLAPI *p_glMultiTexCoord1fv)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glMultiTexCoord1fvARB)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glMultiTexCoord1fvSGIS)(GLenum,const GLfloat *);
        void       (WINE_GLAPI *p_glMultiTexCoord1hNV)(GLenum,GLhalfNV);
        void       (WINE_GLAPI *p_glMultiTexCoord1hvNV)(GLenum,const GLhalfNV*);
        void       (WINE_GLAPI *p_glMultiTexCoord1i)(GLenum,GLint);
        void       (WINE_GLAPI *p_glMultiTexCoord1iARB)(GLenum,GLint);
        void       (WINE_GLAPI *p_glMultiTexCoord1iSGIS)(GLenum,GLint);
        void       (WINE_GLAPI *p_glMultiTexCoord1iv)(GLenum,const GLint*);
        void       (WINE_GLAPI *p_glMultiTexCoord1ivARB)(GLenum,const GLint*);
        void       (WINE_GLAPI *p_glMultiTexCoord1ivSGIS)(GLenum,GLint *);
        void       (WINE_GLAPI *p_glMultiTexCoord1s)(GLenum,GLshort);
        void       (WINE_GLAPI *p_glMultiTexCoord1sARB)(GLenum,GLshort);
        void       (WINE_GLAPI *p_glMultiTexCoord1sSGIS)(GLenum,GLshort);
        void       (WINE_GLAPI *p_glMultiTexCoord1sv)(GLenum,const GLshort*);
        void       (WINE_GLAPI *p_glMultiTexCoord1svARB)(GLenum,const GLshort*);
        void       (WINE_GLAPI *p_glMultiTexCoord1svSGIS)(GLenum,GLshort *);
        void       (WINE_GLAPI *p_glMultiTexCoord1xOES)(GLenum,GLfixed);
        void       (WINE_GLAPI *p_glMultiTexCoord1xvOES)(GLenum,const GLfixed*);
        void       (WINE_GLAPI *p_glMultiTexCoord2bOES)(GLenum,GLbyte,GLbyte);
        void       (WINE_GLAPI *p_glMultiTexCoord2bvOES)(GLenum,const GLbyte*);
        void       (WINE_GLAPI *p_glMultiTexCoord2d)(GLenum,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glMultiTexCoord2dARB)(GLenum,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glMultiTexCoord2dSGIS)(GLenum,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glMultiTexCoord2dv)(GLenum,const GLdouble*);
        void       (WINE_GLAPI *p_glMultiTexCoord2dvARB)(GLenum,const GLdouble*);
        void       (WINE_GLAPI *p_glMultiTexCoord2dvSGIS)(GLenum,GLdouble *);
        void       (WINE_GLAPI *p_glMultiTexCoord2f)(GLenum,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glMultiTexCoord2fARB)(GLenum,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glMultiTexCoord2fSGIS)(GLenum,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glMultiTexCoord2fv)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glMultiTexCoord2fvARB)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glMultiTexCoord2fvSGIS)(GLenum,GLfloat *);
        void       (WINE_GLAPI *p_glMultiTexCoord2hNV)(GLenum,GLhalfNV,GLhalfNV);
        void       (WINE_GLAPI *p_glMultiTexCoord2hvNV)(GLenum,const GLhalfNV*);
        void       (WINE_GLAPI *p_glMultiTexCoord2i)(GLenum,GLint,GLint);
        void       (WINE_GLAPI *p_glMultiTexCoord2iARB)(GLenum,GLint,GLint);
        void       (WINE_GLAPI *p_glMultiTexCoord2iSGIS)(GLenum,GLint,GLint);
        void       (WINE_GLAPI *p_glMultiTexCoord2iv)(GLenum,const GLint*);
        void       (WINE_GLAPI *p_glMultiTexCoord2ivARB)(GLenum,const GLint*);
        void       (WINE_GLAPI *p_glMultiTexCoord2ivSGIS)(GLenum,GLint *);
        void       (WINE_GLAPI *p_glMultiTexCoord2s)(GLenum,GLshort,GLshort);
        void       (WINE_GLAPI *p_glMultiTexCoord2sARB)(GLenum,GLshort,GLshort);
        void       (WINE_GLAPI *p_glMultiTexCoord2sSGIS)(GLenum,GLshort,GLshort);
        void       (WINE_GLAPI *p_glMultiTexCoord2sv)(GLenum,const GLshort*);
        void       (WINE_GLAPI *p_glMultiTexCoord2svARB)(GLenum,const GLshort*);
        void       (WINE_GLAPI *p_glMultiTexCoord2svSGIS)(GLenum,GLshort *);
        void       (WINE_GLAPI *p_glMultiTexCoord2xOES)(GLenum,GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glMultiTexCoord2xvOES)(GLenum,const GLfixed*);
        void       (WINE_GLAPI *p_glMultiTexCoord3bOES)(GLenum,GLbyte,GLbyte,GLbyte);
        void       (WINE_GLAPI *p_glMultiTexCoord3bvOES)(GLenum,const GLbyte*);
        void       (WINE_GLAPI *p_glMultiTexCoord3d)(GLenum,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glMultiTexCoord3dARB)(GLenum,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glMultiTexCoord3dSGIS)(GLenum,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glMultiTexCoord3dv)(GLenum,const GLdouble*);
        void       (WINE_GLAPI *p_glMultiTexCoord3dvARB)(GLenum,const GLdouble*);
        void       (WINE_GLAPI *p_glMultiTexCoord3dvSGIS)(GLenum,GLdouble *);
        void       (WINE_GLAPI *p_glMultiTexCoord3f)(GLenum,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glMultiTexCoord3fARB)(GLenum,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glMultiTexCoord3fSGIS)(GLenum,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glMultiTexCoord3fv)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glMultiTexCoord3fvARB)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glMultiTexCoord3fvSGIS)(GLenum,GLfloat *);
        void       (WINE_GLAPI *p_glMultiTexCoord3hNV)(GLenum,GLhalfNV,GLhalfNV,GLhalfNV);
        void       (WINE_GLAPI *p_glMultiTexCoord3hvNV)(GLenum,const GLhalfNV*);
        void       (WINE_GLAPI *p_glMultiTexCoord3i)(GLenum,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glMultiTexCoord3iARB)(GLenum,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glMultiTexCoord3iSGIS)(GLenum,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glMultiTexCoord3iv)(GLenum,const GLint*);
        void       (WINE_GLAPI *p_glMultiTexCoord3ivARB)(GLenum,const GLint*);
        void       (WINE_GLAPI *p_glMultiTexCoord3ivSGIS)(GLenum,GLint *);
        void       (WINE_GLAPI *p_glMultiTexCoord3s)(GLenum,GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glMultiTexCoord3sARB)(GLenum,GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glMultiTexCoord3sSGIS)(GLenum,GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glMultiTexCoord3sv)(GLenum,const GLshort*);
        void       (WINE_GLAPI *p_glMultiTexCoord3svARB)(GLenum,const GLshort*);
        void       (WINE_GLAPI *p_glMultiTexCoord3svSGIS)(GLenum,GLshort *);
        void       (WINE_GLAPI *p_glMultiTexCoord3xOES)(GLenum,GLfixed,GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glMultiTexCoord3xvOES)(GLenum,const GLfixed*);
        void       (WINE_GLAPI *p_glMultiTexCoord4bOES)(GLenum,GLbyte,GLbyte,GLbyte,GLbyte);
        void       (WINE_GLAPI *p_glMultiTexCoord4bvOES)(GLenum,const GLbyte*);
        void       (WINE_GLAPI *p_glMultiTexCoord4d)(GLenum,GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glMultiTexCoord4dARB)(GLenum,GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glMultiTexCoord4dSGIS)(GLenum,GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glMultiTexCoord4dv)(GLenum,const GLdouble*);
        void       (WINE_GLAPI *p_glMultiTexCoord4dvARB)(GLenum,const GLdouble*);
        void       (WINE_GLAPI *p_glMultiTexCoord4dvSGIS)(GLenum,GLdouble *);
        void       (WINE_GLAPI *p_glMultiTexCoord4f)(GLenum,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glMultiTexCoord4fARB)(GLenum,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glMultiTexCoord4fSGIS)(GLenum,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glMultiTexCoord4fv)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glMultiTexCoord4fvARB)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glMultiTexCoord4fvSGIS)(GLenum,GLfloat *);
        void       (WINE_GLAPI *p_glMultiTexCoord4hNV)(GLenum,GLhalfNV,GLhalfNV,GLhalfNV,GLhalfNV);
        void       (WINE_GLAPI *p_glMultiTexCoord4hvNV)(GLenum,const GLhalfNV*);
        void       (WINE_GLAPI *p_glMultiTexCoord4i)(GLenum,GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glMultiTexCoord4iARB)(GLenum,GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glMultiTexCoord4iSGIS)(GLenum,GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glMultiTexCoord4iv)(GLenum,const GLint*);
        void       (WINE_GLAPI *p_glMultiTexCoord4ivARB)(GLenum,const GLint*);
        void       (WINE_GLAPI *p_glMultiTexCoord4ivSGIS)(GLenum,GLint *);
        void       (WINE_GLAPI *p_glMultiTexCoord4s)(GLenum,GLshort,GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glMultiTexCoord4sARB)(GLenum,GLshort,GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glMultiTexCoord4sSGIS)(GLenum,GLshort,GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glMultiTexCoord4sv)(GLenum,const GLshort*);
        void       (WINE_GLAPI *p_glMultiTexCoord4svARB)(GLenum,const GLshort*);
        void       (WINE_GLAPI *p_glMultiTexCoord4svSGIS)(GLenum,GLshort *);
        void       (WINE_GLAPI *p_glMultiTexCoord4xOES)(GLenum,GLfixed,GLfixed,GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glMultiTexCoord4xvOES)(GLenum,const GLfixed*);
        void       (WINE_GLAPI *p_glMultiTexCoordP1ui)(GLenum,GLenum,GLuint);
        void       (WINE_GLAPI *p_glMultiTexCoordP1uiv)(GLenum,GLenum,const GLuint*);
        void       (WINE_GLAPI *p_glMultiTexCoordP2ui)(GLenum,GLenum,GLuint);
        void       (WINE_GLAPI *p_glMultiTexCoordP2uiv)(GLenum,GLenum,const GLuint*);
        void       (WINE_GLAPI *p_glMultiTexCoordP3ui)(GLenum,GLenum,GLuint);
        void       (WINE_GLAPI *p_glMultiTexCoordP3uiv)(GLenum,GLenum,const GLuint*);
        void       (WINE_GLAPI *p_glMultiTexCoordP4ui)(GLenum,GLenum,GLuint);
        void       (WINE_GLAPI *p_glMultiTexCoordP4uiv)(GLenum,GLenum,const GLuint*);
        void       (WINE_GLAPI *p_glMultiTexCoordPointerEXT)(GLenum,GLint,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glMultiTexCoordPointerSGIS)(GLenum,GLint,GLenum,GLsizei,GLvoid *);
        void       (WINE_GLAPI *p_glMultiTexEnvfEXT)(GLenum,GLenum,GLenum,GLfloat);
        void       (WINE_GLAPI *p_glMultiTexEnvfvEXT)(GLenum,GLenum,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glMultiTexEnviEXT)(GLenum,GLenum,GLenum,GLint);
        void       (WINE_GLAPI *p_glMultiTexEnvivEXT)(GLenum,GLenum,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glMultiTexGendEXT)(GLenum,GLenum,GLenum,GLdouble);
        void       (WINE_GLAPI *p_glMultiTexGendvEXT)(GLenum,GLenum,GLenum,const GLdouble*);
        void       (WINE_GLAPI *p_glMultiTexGenfEXT)(GLenum,GLenum,GLenum,GLfloat);
        void       (WINE_GLAPI *p_glMultiTexGenfvEXT)(GLenum,GLenum,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glMultiTexGeniEXT)(GLenum,GLenum,GLenum,GLint);
        void       (WINE_GLAPI *p_glMultiTexGenivEXT)(GLenum,GLenum,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glMultiTexImage1DEXT)(GLenum,GLenum,GLint,GLint,GLsizei,GLint,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glMultiTexImage2DEXT)(GLenum,GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glMultiTexImage3DEXT)(GLenum,GLenum,GLint,GLint,GLsizei,GLsizei,GLsizei,GLint,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glMultiTexParameterIivEXT)(GLenum,GLenum,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glMultiTexParameterIuivEXT)(GLenum,GLenum,GLenum,const GLuint*);
        void       (WINE_GLAPI *p_glMultiTexParameterfEXT)(GLenum,GLenum,GLenum,GLfloat);
        void       (WINE_GLAPI *p_glMultiTexParameterfvEXT)(GLenum,GLenum,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glMultiTexParameteriEXT)(GLenum,GLenum,GLenum,GLint);
        void       (WINE_GLAPI *p_glMultiTexParameterivEXT)(GLenum,GLenum,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glMultiTexRenderbufferEXT)(GLenum,GLenum,GLuint);
        void       (WINE_GLAPI *p_glMultiTexSubImage1DEXT)(GLenum,GLenum,GLint,GLint,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glMultiTexSubImage2DEXT)(GLenum,GLenum,GLint,GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glMultiTexSubImage3DEXT)(GLenum,GLenum,GLint,GLint,GLint,GLint,GLsizei,GLsizei,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glNamedBufferData)(GLuint,GLsizeiptr,const void*,GLenum);
        void       (WINE_GLAPI *p_glNamedBufferDataEXT)(GLuint,GLsizeiptr,const void*,GLenum);
        void       (WINE_GLAPI *p_glNamedBufferPageCommitmentARB)(GLuint,GLintptr,GLsizeiptr,GLboolean);
        void       (WINE_GLAPI *p_glNamedBufferPageCommitmentEXT)(GLuint,GLintptr,GLsizeiptr,GLboolean);
        void       (WINE_GLAPI *p_glNamedBufferStorage)(GLuint,GLsizeiptr,const void*,GLbitfield);
        void       (WINE_GLAPI *p_glNamedBufferStorageEXT)(GLuint,GLsizeiptr,const void*,GLbitfield);
        void       (WINE_GLAPI *p_glNamedBufferSubData)(GLuint,GLintptr,GLsizeiptr,const void*);
        void       (WINE_GLAPI *p_glNamedBufferSubDataEXT)(GLuint,GLintptr,GLsizeiptr,const void*);
        void       (WINE_GLAPI *p_glNamedCopyBufferSubDataEXT)(GLuint,GLuint,GLintptr,GLintptr,GLsizeiptr);
        void       (WINE_GLAPI *p_glNamedFramebufferDrawBuffer)(GLuint,GLenum);
        void       (WINE_GLAPI *p_glNamedFramebufferDrawBuffers)(GLuint,GLsizei,const GLenum*);
        void       (WINE_GLAPI *p_glNamedFramebufferParameteri)(GLuint,GLenum,GLint);
        void       (WINE_GLAPI *p_glNamedFramebufferParameteriEXT)(GLuint,GLenum,GLint);
        void       (WINE_GLAPI *p_glNamedFramebufferReadBuffer)(GLuint,GLenum);
        void       (WINE_GLAPI *p_glNamedFramebufferRenderbuffer)(GLuint,GLenum,GLenum,GLuint);
        void       (WINE_GLAPI *p_glNamedFramebufferRenderbufferEXT)(GLuint,GLenum,GLenum,GLuint);
        void       (WINE_GLAPI *p_glNamedFramebufferSampleLocationsfvNV)(GLuint,GLuint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glNamedFramebufferTexture)(GLuint,GLenum,GLuint,GLint);
        void       (WINE_GLAPI *p_glNamedFramebufferTexture1DEXT)(GLuint,GLenum,GLenum,GLuint,GLint);
        void       (WINE_GLAPI *p_glNamedFramebufferTexture2DEXT)(GLuint,GLenum,GLenum,GLuint,GLint);
        void       (WINE_GLAPI *p_glNamedFramebufferTexture3DEXT)(GLuint,GLenum,GLenum,GLuint,GLint,GLint);
        void       (WINE_GLAPI *p_glNamedFramebufferTextureEXT)(GLuint,GLenum,GLuint,GLint);
        void       (WINE_GLAPI *p_glNamedFramebufferTextureFaceEXT)(GLuint,GLenum,GLuint,GLint,GLenum);
        void       (WINE_GLAPI *p_glNamedFramebufferTextureLayer)(GLuint,GLenum,GLuint,GLint,GLint);
        void       (WINE_GLAPI *p_glNamedFramebufferTextureLayerEXT)(GLuint,GLenum,GLuint,GLint,GLint);
        void       (WINE_GLAPI *p_glNamedProgramLocalParameter4dEXT)(GLuint,GLenum,GLuint,GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glNamedProgramLocalParameter4dvEXT)(GLuint,GLenum,GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glNamedProgramLocalParameter4fEXT)(GLuint,GLenum,GLuint,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glNamedProgramLocalParameter4fvEXT)(GLuint,GLenum,GLuint,const GLfloat*);
        void       (WINE_GLAPI *p_glNamedProgramLocalParameterI4iEXT)(GLuint,GLenum,GLuint,GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glNamedProgramLocalParameterI4ivEXT)(GLuint,GLenum,GLuint,const GLint*);
        void       (WINE_GLAPI *p_glNamedProgramLocalParameterI4uiEXT)(GLuint,GLenum,GLuint,GLuint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glNamedProgramLocalParameterI4uivEXT)(GLuint,GLenum,GLuint,const GLuint*);
        void       (WINE_GLAPI *p_glNamedProgramLocalParameters4fvEXT)(GLuint,GLenum,GLuint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glNamedProgramLocalParametersI4ivEXT)(GLuint,GLenum,GLuint,GLsizei,const GLint*);
        void       (WINE_GLAPI *p_glNamedProgramLocalParametersI4uivEXT)(GLuint,GLenum,GLuint,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glNamedProgramStringEXT)(GLuint,GLenum,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glNamedRenderbufferStorage)(GLuint,GLenum,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glNamedRenderbufferStorageEXT)(GLuint,GLenum,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glNamedRenderbufferStorageMultisample)(GLuint,GLsizei,GLenum,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glNamedRenderbufferStorageMultisampleCoverageEXT)(GLuint,GLsizei,GLsizei,GLenum,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glNamedRenderbufferStorageMultisampleEXT)(GLuint,GLsizei,GLenum,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glNamedStringARB)(GLenum,GLint,const GLchar*,GLint,const GLchar*);
        GLuint     (WINE_GLAPI *p_glNewBufferRegion)(GLenum);
        GLuint     (WINE_GLAPI *p_glNewObjectBufferATI)(GLsizei,const void*,GLenum);
        void       (WINE_GLAPI *p_glNormal3fVertex3fSUN)(GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glNormal3fVertex3fvSUN)(const GLfloat*,const GLfloat*);
        void       (WINE_GLAPI *p_glNormal3hNV)(GLhalfNV,GLhalfNV,GLhalfNV);
        void       (WINE_GLAPI *p_glNormal3hvNV)(const GLhalfNV*);
        void       (WINE_GLAPI *p_glNormal3xOES)(GLfixed,GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glNormal3xvOES)(const GLfixed*);
        void       (WINE_GLAPI *p_glNormalFormatNV)(GLenum,GLsizei);
        void       (WINE_GLAPI *p_glNormalP3ui)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glNormalP3uiv)(GLenum,const GLuint*);
        void       (WINE_GLAPI *p_glNormalPointerEXT)(GLenum,GLsizei,GLsizei,const void*);
        void       (WINE_GLAPI *p_glNormalPointerListIBM)(GLenum,GLint,const void**,GLint);
        void       (WINE_GLAPI *p_glNormalPointervINTEL)(GLenum,const void**);
        void       (WINE_GLAPI *p_glNormalStream3bATI)(GLenum,GLbyte,GLbyte,GLbyte);
        void       (WINE_GLAPI *p_glNormalStream3bvATI)(GLenum,const GLbyte*);
        void       (WINE_GLAPI *p_glNormalStream3dATI)(GLenum,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glNormalStream3dvATI)(GLenum,const GLdouble*);
        void       (WINE_GLAPI *p_glNormalStream3fATI)(GLenum,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glNormalStream3fvATI)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glNormalStream3iATI)(GLenum,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glNormalStream3ivATI)(GLenum,const GLint*);
        void       (WINE_GLAPI *p_glNormalStream3sATI)(GLenum,GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glNormalStream3svATI)(GLenum,const GLshort*);
        void       (WINE_GLAPI *p_glObjectLabel)(GLenum,GLuint,GLsizei,const GLchar*);
        void       (WINE_GLAPI *p_glObjectPtrLabel)(const void*,GLsizei,const GLchar*);
        GLenum     (WINE_GLAPI *p_glObjectPurgeableAPPLE)(GLenum,GLuint,GLenum);
        GLenum     (WINE_GLAPI *p_glObjectUnpurgeableAPPLE)(GLenum,GLuint,GLenum);
        void       (WINE_GLAPI *p_glOrthofOES)(GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glOrthoxOES)(GLfixed,GLfixed,GLfixed,GLfixed,GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glPNTrianglesfATI)(GLenum,GLfloat);
        void       (WINE_GLAPI *p_glPNTrianglesiATI)(GLenum,GLint);
        void       (WINE_GLAPI *p_glPassTexCoordATI)(GLuint,GLuint,GLenum);
        void       (WINE_GLAPI *p_glPassThroughxOES)(GLfixed);
        void       (WINE_GLAPI *p_glPatchParameterfv)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glPatchParameteri)(GLenum,GLint);
        void       (WINE_GLAPI *p_glPathColorGenNV)(GLenum,GLenum,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glPathCommandsNV)(GLuint,GLsizei,const GLubyte*,GLsizei,GLenum,const void*);
        void       (WINE_GLAPI *p_glPathCoordsNV)(GLuint,GLsizei,GLenum,const void*);
        void       (WINE_GLAPI *p_glPathCoverDepthFuncNV)(GLenum);
        void       (WINE_GLAPI *p_glPathDashArrayNV)(GLuint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glPathFogGenNV)(GLenum);
        GLenum     (WINE_GLAPI *p_glPathGlyphIndexArrayNV)(GLuint,GLenum,const void*,GLbitfield,GLuint,GLsizei,GLuint,GLfloat);
        GLenum     (WINE_GLAPI *p_glPathGlyphIndexRangeNV)(GLenum,const void*,GLbitfield,GLuint,GLfloat,GLuint[2]);
        void       (WINE_GLAPI *p_glPathGlyphRangeNV)(GLuint,GLenum,const void*,GLbitfield,GLuint,GLsizei,GLenum,GLuint,GLfloat);
        void       (WINE_GLAPI *p_glPathGlyphsNV)(GLuint,GLenum,const void*,GLbitfield,GLsizei,GLenum,const void*,GLenum,GLuint,GLfloat);
        GLenum     (WINE_GLAPI *p_glPathMemoryGlyphIndexArrayNV)(GLuint,GLenum,GLsizeiptr,const void*,GLsizei,GLuint,GLsizei,GLuint,GLfloat);
        void       (WINE_GLAPI *p_glPathParameterfNV)(GLuint,GLenum,GLfloat);
        void       (WINE_GLAPI *p_glPathParameterfvNV)(GLuint,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glPathParameteriNV)(GLuint,GLenum,GLint);
        void       (WINE_GLAPI *p_glPathParameterivNV)(GLuint,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glPathStencilDepthOffsetNV)(GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glPathStencilFuncNV)(GLenum,GLint,GLuint);
        void       (WINE_GLAPI *p_glPathStringNV)(GLuint,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glPathSubCommandsNV)(GLuint,GLsizei,GLsizei,GLsizei,const GLubyte*,GLsizei,GLenum,const void*);
        void       (WINE_GLAPI *p_glPathSubCoordsNV)(GLuint,GLsizei,GLsizei,GLenum,const void*);
        void       (WINE_GLAPI *p_glPathTexGenNV)(GLenum,GLenum,GLint,const GLfloat*);
        void       (WINE_GLAPI *p_glPauseTransformFeedback)(void);
        void       (WINE_GLAPI *p_glPauseTransformFeedbackNV)(void);
        void       (WINE_GLAPI *p_glPixelDataRangeNV)(GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glPixelMapx)(GLenum,GLint,const GLfixed*);
        void       (WINE_GLAPI *p_glPixelStorex)(GLenum,GLfixed);
        void       (WINE_GLAPI *p_glPixelTexGenParameterfSGIS)(GLenum,GLfloat);
        void       (WINE_GLAPI *p_glPixelTexGenParameterfvSGIS)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glPixelTexGenParameteriSGIS)(GLenum,GLint);
        void       (WINE_GLAPI *p_glPixelTexGenParameterivSGIS)(GLenum,const GLint*);
        void       (WINE_GLAPI *p_glPixelTexGenSGIX)(GLenum);
        void       (WINE_GLAPI *p_glPixelTransferxOES)(GLenum,GLfixed);
        void       (WINE_GLAPI *p_glPixelTransformParameterfEXT)(GLenum,GLenum,GLfloat);
        void       (WINE_GLAPI *p_glPixelTransformParameterfvEXT)(GLenum,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glPixelTransformParameteriEXT)(GLenum,GLenum,GLint);
        void       (WINE_GLAPI *p_glPixelTransformParameterivEXT)(GLenum,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glPixelZoomxOES)(GLfixed,GLfixed);
        GLboolean  (WINE_GLAPI *p_glPointAlongPathNV)(GLuint,GLsizei,GLsizei,GLfloat,GLfloat*,GLfloat*,GLfloat*,GLfloat*);
        void       (WINE_GLAPI *p_glPointParameterf)(GLenum,GLfloat);
        void       (WINE_GLAPI *p_glPointParameterfARB)(GLenum,GLfloat);
        void       (WINE_GLAPI *p_glPointParameterfEXT)(GLenum,GLfloat);
        void       (WINE_GLAPI *p_glPointParameterfSGIS)(GLenum,GLfloat);
        void       (WINE_GLAPI *p_glPointParameterfv)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glPointParameterfvARB)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glPointParameterfvEXT)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glPointParameterfvSGIS)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glPointParameteri)(GLenum,GLint);
        void       (WINE_GLAPI *p_glPointParameteriNV)(GLenum,GLint);
        void       (WINE_GLAPI *p_glPointParameteriv)(GLenum,const GLint*);
        void       (WINE_GLAPI *p_glPointParameterivNV)(GLenum,const GLint*);
        void       (WINE_GLAPI *p_glPointParameterxvOES)(GLenum,const GLfixed*);
        void       (WINE_GLAPI *p_glPointSizexOES)(GLfixed);
        GLint      (WINE_GLAPI *p_glPollAsyncSGIX)(GLuint*);
        GLint      (WINE_GLAPI *p_glPollInstrumentsSGIX)(GLint*);
        void       (WINE_GLAPI *p_glPolygonOffsetClampEXT)(GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glPolygonOffsetEXT)(GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glPolygonOffsetxOES)(GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glPopDebugGroup)(void);
        void       (WINE_GLAPI *p_glPopGroupMarkerEXT)(void);
        void       (WINE_GLAPI *p_glPresentFrameDualFillNV)(GLuint,GLuint64EXT,GLuint,GLuint,GLenum,GLenum,GLuint,GLenum,GLuint,GLenum,GLuint,GLenum,GLuint);
        void       (WINE_GLAPI *p_glPresentFrameKeyedNV)(GLuint,GLuint64EXT,GLuint,GLuint,GLenum,GLenum,GLuint,GLuint,GLenum,GLuint,GLuint);
        void       (WINE_GLAPI *p_glPrimitiveRestartIndex)(GLuint);
        void       (WINE_GLAPI *p_glPrimitiveRestartIndexNV)(GLuint);
        void       (WINE_GLAPI *p_glPrimitiveRestartNV)(void);
        void       (WINE_GLAPI *p_glPrioritizeTexturesEXT)(GLsizei,const GLuint*,const GLclampf*);
        void       (WINE_GLAPI *p_glPrioritizeTexturesxOES)(GLsizei,const GLuint*,const GLfixed*);
        void       (WINE_GLAPI *p_glProgramBinary)(GLuint,GLenum,const void*,GLsizei);
        void       (WINE_GLAPI *p_glProgramBufferParametersIivNV)(GLenum,GLuint,GLuint,GLsizei,const GLint*);
        void       (WINE_GLAPI *p_glProgramBufferParametersIuivNV)(GLenum,GLuint,GLuint,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glProgramBufferParametersfvNV)(GLenum,GLuint,GLuint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramEnvParameter4dARB)(GLenum,GLuint,GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glProgramEnvParameter4dvARB)(GLenum,GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramEnvParameter4fARB)(GLenum,GLuint,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glProgramEnvParameter4fvARB)(GLenum,GLuint,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramEnvParameterI4iNV)(GLenum,GLuint,GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glProgramEnvParameterI4ivNV)(GLenum,GLuint,const GLint*);
        void       (WINE_GLAPI *p_glProgramEnvParameterI4uiNV)(GLenum,GLuint,GLuint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glProgramEnvParameterI4uivNV)(GLenum,GLuint,const GLuint*);
        void       (WINE_GLAPI *p_glProgramEnvParameters4fvEXT)(GLenum,GLuint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramEnvParametersI4ivNV)(GLenum,GLuint,GLsizei,const GLint*);
        void       (WINE_GLAPI *p_glProgramEnvParametersI4uivNV)(GLenum,GLuint,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glProgramLocalParameter4dARB)(GLenum,GLuint,GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glProgramLocalParameter4dvARB)(GLenum,GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramLocalParameter4fARB)(GLenum,GLuint,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glProgramLocalParameter4fvARB)(GLenum,GLuint,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramLocalParameterI4iNV)(GLenum,GLuint,GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glProgramLocalParameterI4ivNV)(GLenum,GLuint,const GLint*);
        void       (WINE_GLAPI *p_glProgramLocalParameterI4uiNV)(GLenum,GLuint,GLuint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glProgramLocalParameterI4uivNV)(GLenum,GLuint,const GLuint*);
        void       (WINE_GLAPI *p_glProgramLocalParameters4fvEXT)(GLenum,GLuint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramLocalParametersI4ivNV)(GLenum,GLuint,GLsizei,const GLint*);
        void       (WINE_GLAPI *p_glProgramLocalParametersI4uivNV)(GLenum,GLuint,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glProgramNamedParameter4dNV)(GLuint,GLsizei,const GLubyte*,GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glProgramNamedParameter4dvNV)(GLuint,GLsizei,const GLubyte*,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramNamedParameter4fNV)(GLuint,GLsizei,const GLubyte*,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glProgramNamedParameter4fvNV)(GLuint,GLsizei,const GLubyte*,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramParameter4dNV)(GLenum,GLuint,GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glProgramParameter4dvNV)(GLenum,GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramParameter4fNV)(GLenum,GLuint,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glProgramParameter4fvNV)(GLenum,GLuint,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramParameteri)(GLuint,GLenum,GLint);
        void       (WINE_GLAPI *p_glProgramParameteriARB)(GLuint,GLenum,GLint);
        void       (WINE_GLAPI *p_glProgramParameteriEXT)(GLuint,GLenum,GLint);
        void       (WINE_GLAPI *p_glProgramParameters4dvNV)(GLenum,GLuint,GLsizei,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramParameters4fvNV)(GLenum,GLuint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramPathFragmentInputGenNV)(GLuint,GLint,GLenum,GLint,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramStringARB)(GLenum,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glProgramSubroutineParametersuivNV)(GLenum,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glProgramUniform1d)(GLuint,GLint,GLdouble);
        void       (WINE_GLAPI *p_glProgramUniform1dEXT)(GLuint,GLint,GLdouble);
        void       (WINE_GLAPI *p_glProgramUniform1dv)(GLuint,GLint,GLsizei,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniform1dvEXT)(GLuint,GLint,GLsizei,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniform1f)(GLuint,GLint,GLfloat);
        void       (WINE_GLAPI *p_glProgramUniform1fEXT)(GLuint,GLint,GLfloat);
        void       (WINE_GLAPI *p_glProgramUniform1fv)(GLuint,GLint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniform1fvEXT)(GLuint,GLint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniform1i)(GLuint,GLint,GLint);
        void       (WINE_GLAPI *p_glProgramUniform1i64NV)(GLuint,GLint,GLint64EXT);
        void       (WINE_GLAPI *p_glProgramUniform1i64vNV)(GLuint,GLint,GLsizei,const GLint64EXT*);
        void       (WINE_GLAPI *p_glProgramUniform1iEXT)(GLuint,GLint,GLint);
        void       (WINE_GLAPI *p_glProgramUniform1iv)(GLuint,GLint,GLsizei,const GLint*);
        void       (WINE_GLAPI *p_glProgramUniform1ivEXT)(GLuint,GLint,GLsizei,const GLint*);
        void       (WINE_GLAPI *p_glProgramUniform1ui)(GLuint,GLint,GLuint);
        void       (WINE_GLAPI *p_glProgramUniform1ui64NV)(GLuint,GLint,GLuint64EXT);
        void       (WINE_GLAPI *p_glProgramUniform1ui64vNV)(GLuint,GLint,GLsizei,const GLuint64EXT*);
        void       (WINE_GLAPI *p_glProgramUniform1uiEXT)(GLuint,GLint,GLuint);
        void       (WINE_GLAPI *p_glProgramUniform1uiv)(GLuint,GLint,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glProgramUniform1uivEXT)(GLuint,GLint,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glProgramUniform2d)(GLuint,GLint,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glProgramUniform2dEXT)(GLuint,GLint,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glProgramUniform2dv)(GLuint,GLint,GLsizei,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniform2dvEXT)(GLuint,GLint,GLsizei,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniform2f)(GLuint,GLint,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glProgramUniform2fEXT)(GLuint,GLint,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glProgramUniform2fv)(GLuint,GLint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniform2fvEXT)(GLuint,GLint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniform2i)(GLuint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glProgramUniform2i64NV)(GLuint,GLint,GLint64EXT,GLint64EXT);
        void       (WINE_GLAPI *p_glProgramUniform2i64vNV)(GLuint,GLint,GLsizei,const GLint64EXT*);
        void       (WINE_GLAPI *p_glProgramUniform2iEXT)(GLuint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glProgramUniform2iv)(GLuint,GLint,GLsizei,const GLint*);
        void       (WINE_GLAPI *p_glProgramUniform2ivEXT)(GLuint,GLint,GLsizei,const GLint*);
        void       (WINE_GLAPI *p_glProgramUniform2ui)(GLuint,GLint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glProgramUniform2ui64NV)(GLuint,GLint,GLuint64EXT,GLuint64EXT);
        void       (WINE_GLAPI *p_glProgramUniform2ui64vNV)(GLuint,GLint,GLsizei,const GLuint64EXT*);
        void       (WINE_GLAPI *p_glProgramUniform2uiEXT)(GLuint,GLint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glProgramUniform2uiv)(GLuint,GLint,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glProgramUniform2uivEXT)(GLuint,GLint,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glProgramUniform3d)(GLuint,GLint,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glProgramUniform3dEXT)(GLuint,GLint,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glProgramUniform3dv)(GLuint,GLint,GLsizei,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniform3dvEXT)(GLuint,GLint,GLsizei,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniform3f)(GLuint,GLint,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glProgramUniform3fEXT)(GLuint,GLint,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glProgramUniform3fv)(GLuint,GLint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniform3fvEXT)(GLuint,GLint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniform3i)(GLuint,GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glProgramUniform3i64NV)(GLuint,GLint,GLint64EXT,GLint64EXT,GLint64EXT);
        void       (WINE_GLAPI *p_glProgramUniform3i64vNV)(GLuint,GLint,GLsizei,const GLint64EXT*);
        void       (WINE_GLAPI *p_glProgramUniform3iEXT)(GLuint,GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glProgramUniform3iv)(GLuint,GLint,GLsizei,const GLint*);
        void       (WINE_GLAPI *p_glProgramUniform3ivEXT)(GLuint,GLint,GLsizei,const GLint*);
        void       (WINE_GLAPI *p_glProgramUniform3ui)(GLuint,GLint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glProgramUniform3ui64NV)(GLuint,GLint,GLuint64EXT,GLuint64EXT,GLuint64EXT);
        void       (WINE_GLAPI *p_glProgramUniform3ui64vNV)(GLuint,GLint,GLsizei,const GLuint64EXT*);
        void       (WINE_GLAPI *p_glProgramUniform3uiEXT)(GLuint,GLint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glProgramUniform3uiv)(GLuint,GLint,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glProgramUniform3uivEXT)(GLuint,GLint,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glProgramUniform4d)(GLuint,GLint,GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glProgramUniform4dEXT)(GLuint,GLint,GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glProgramUniform4dv)(GLuint,GLint,GLsizei,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniform4dvEXT)(GLuint,GLint,GLsizei,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniform4f)(GLuint,GLint,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glProgramUniform4fEXT)(GLuint,GLint,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glProgramUniform4fv)(GLuint,GLint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniform4fvEXT)(GLuint,GLint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniform4i)(GLuint,GLint,GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glProgramUniform4i64NV)(GLuint,GLint,GLint64EXT,GLint64EXT,GLint64EXT,GLint64EXT);
        void       (WINE_GLAPI *p_glProgramUniform4i64vNV)(GLuint,GLint,GLsizei,const GLint64EXT*);
        void       (WINE_GLAPI *p_glProgramUniform4iEXT)(GLuint,GLint,GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glProgramUniform4iv)(GLuint,GLint,GLsizei,const GLint*);
        void       (WINE_GLAPI *p_glProgramUniform4ivEXT)(GLuint,GLint,GLsizei,const GLint*);
        void       (WINE_GLAPI *p_glProgramUniform4ui)(GLuint,GLint,GLuint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glProgramUniform4ui64NV)(GLuint,GLint,GLuint64EXT,GLuint64EXT,GLuint64EXT,GLuint64EXT);
        void       (WINE_GLAPI *p_glProgramUniform4ui64vNV)(GLuint,GLint,GLsizei,const GLuint64EXT*);
        void       (WINE_GLAPI *p_glProgramUniform4uiEXT)(GLuint,GLint,GLuint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glProgramUniform4uiv)(GLuint,GLint,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glProgramUniform4uivEXT)(GLuint,GLint,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glProgramUniformHandleui64ARB)(GLuint,GLint,GLuint64);
        void       (WINE_GLAPI *p_glProgramUniformHandleui64NV)(GLuint,GLint,GLuint64);
        void       (WINE_GLAPI *p_glProgramUniformHandleui64vARB)(GLuint,GLint,GLsizei,const GLuint64*);
        void       (WINE_GLAPI *p_glProgramUniformHandleui64vNV)(GLuint,GLint,GLsizei,const GLuint64*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix2dv)(GLuint,GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix2dvEXT)(GLuint,GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix2fv)(GLuint,GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix2fvEXT)(GLuint,GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix2x3dv)(GLuint,GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix2x3dvEXT)(GLuint,GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix2x3fv)(GLuint,GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix2x3fvEXT)(GLuint,GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix2x4dv)(GLuint,GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix2x4dvEXT)(GLuint,GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix2x4fv)(GLuint,GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix2x4fvEXT)(GLuint,GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix3dv)(GLuint,GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix3dvEXT)(GLuint,GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix3fv)(GLuint,GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix3fvEXT)(GLuint,GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix3x2dv)(GLuint,GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix3x2dvEXT)(GLuint,GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix3x2fv)(GLuint,GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix3x2fvEXT)(GLuint,GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix3x4dv)(GLuint,GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix3x4dvEXT)(GLuint,GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix3x4fv)(GLuint,GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix3x4fvEXT)(GLuint,GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix4dv)(GLuint,GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix4dvEXT)(GLuint,GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix4fv)(GLuint,GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix4fvEXT)(GLuint,GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix4x2dv)(GLuint,GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix4x2dvEXT)(GLuint,GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix4x2fv)(GLuint,GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix4x2fvEXT)(GLuint,GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix4x3dv)(GLuint,GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix4x3dvEXT)(GLuint,GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix4x3fv)(GLuint,GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniformMatrix4x3fvEXT)(GLuint,GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glProgramUniformui64NV)(GLuint,GLint,GLuint64EXT);
        void       (WINE_GLAPI *p_glProgramUniformui64vNV)(GLuint,GLint,GLsizei,const GLuint64EXT*);
        void       (WINE_GLAPI *p_glProgramVertexLimitNV)(GLenum,GLint);
        void       (WINE_GLAPI *p_glProvokingVertex)(GLenum);
        void       (WINE_GLAPI *p_glProvokingVertexEXT)(GLenum);
        void       (WINE_GLAPI *p_glPushClientAttribDefaultEXT)(GLbitfield);
        void       (WINE_GLAPI *p_glPushDebugGroup)(GLenum,GLuint,GLsizei,const GLchar*);
        void       (WINE_GLAPI *p_glPushGroupMarkerEXT)(GLsizei,const GLchar*);
        void       (WINE_GLAPI *p_glQueryCounter)(GLuint,GLenum);
        GLbitfield (WINE_GLAPI *p_glQueryMatrixxOES)(GLfixed*,GLint*);
        void       (WINE_GLAPI *p_glQueryObjectParameteruiAMD)(GLenum,GLuint,GLenum,GLuint);
        void       (WINE_GLAPI *p_glRasterPos2xOES)(GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glRasterPos2xvOES)(const GLfixed*);
        void       (WINE_GLAPI *p_glRasterPos3xOES)(GLfixed,GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glRasterPos3xvOES)(const GLfixed*);
        void       (WINE_GLAPI *p_glRasterPos4xOES)(GLfixed,GLfixed,GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glRasterPos4xvOES)(const GLfixed*);
        void       (WINE_GLAPI *p_glRasterSamplesEXT)(GLuint,GLboolean);
        void       (WINE_GLAPI *p_glReadBufferRegion)(GLenum,GLint,GLint,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glReadInstrumentsSGIX)(GLint);
        void       (WINE_GLAPI *p_glReadnPixels)(GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,GLsizei,void*);
        void       (WINE_GLAPI *p_glReadnPixelsARB)(GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,GLsizei,void*);
        void       (WINE_GLAPI *p_glRectxOES)(GLfixed,GLfixed,GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glRectxvOES)(const GLfixed*,const GLfixed*);
        void       (WINE_GLAPI *p_glReferencePlaneSGIX)(const GLdouble*);
        void       (WINE_GLAPI *p_glReleaseShaderCompiler)(void);
        void       (WINE_GLAPI *p_glRenderbufferStorage)(GLenum,GLenum,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glRenderbufferStorageEXT)(GLenum,GLenum,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glRenderbufferStorageMultisample)(GLenum,GLsizei,GLenum,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glRenderbufferStorageMultisampleCoverageNV)(GLenum,GLsizei,GLsizei,GLenum,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glRenderbufferStorageMultisampleEXT)(GLenum,GLsizei,GLenum,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glReplacementCodePointerSUN)(GLenum,GLsizei,const void**);
        void       (WINE_GLAPI *p_glReplacementCodeubSUN)(GLubyte);
        void       (WINE_GLAPI *p_glReplacementCodeubvSUN)(const GLubyte*);
        void       (WINE_GLAPI *p_glReplacementCodeuiColor3fVertex3fSUN)(GLuint,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glReplacementCodeuiColor3fVertex3fvSUN)(const GLuint*,const GLfloat*,const GLfloat*);
        void       (WINE_GLAPI *p_glReplacementCodeuiColor4fNormal3fVertex3fSUN)(GLuint,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glReplacementCodeuiColor4fNormal3fVertex3fvSUN)(const GLuint*,const GLfloat*,const GLfloat*,const GLfloat*);
        void       (WINE_GLAPI *p_glReplacementCodeuiColor4ubVertex3fSUN)(GLuint,GLubyte,GLubyte,GLubyte,GLubyte,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glReplacementCodeuiColor4ubVertex3fvSUN)(const GLuint*,const GLubyte*,const GLfloat*);
        void       (WINE_GLAPI *p_glReplacementCodeuiNormal3fVertex3fSUN)(GLuint,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glReplacementCodeuiNormal3fVertex3fvSUN)(const GLuint*,const GLfloat*,const GLfloat*);
        void       (WINE_GLAPI *p_glReplacementCodeuiSUN)(GLuint);
        void       (WINE_GLAPI *p_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN)(GLuint,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN)(const GLuint*,const GLfloat*,const GLfloat*,const GLfloat*,const GLfloat*);
        void       (WINE_GLAPI *p_glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN)(GLuint,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN)(const GLuint*,const GLfloat*,const GLfloat*,const GLfloat*);
        void       (WINE_GLAPI *p_glReplacementCodeuiTexCoord2fVertex3fSUN)(GLuint,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glReplacementCodeuiTexCoord2fVertex3fvSUN)(const GLuint*,const GLfloat*,const GLfloat*);
        void       (WINE_GLAPI *p_glReplacementCodeuiVertex3fSUN)(GLuint,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glReplacementCodeuiVertex3fvSUN)(const GLuint*,const GLfloat*);
        void       (WINE_GLAPI *p_glReplacementCodeuivSUN)(const GLuint*);
        void       (WINE_GLAPI *p_glReplacementCodeusSUN)(GLushort);
        void       (WINE_GLAPI *p_glReplacementCodeusvSUN)(const GLushort*);
        void       (WINE_GLAPI *p_glRequestResidentProgramsNV)(GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glResetHistogram)(GLenum);
        void       (WINE_GLAPI *p_glResetHistogramEXT)(GLenum);
        void       (WINE_GLAPI *p_glResetMinmax)(GLenum);
        void       (WINE_GLAPI *p_glResetMinmaxEXT)(GLenum);
        void       (WINE_GLAPI *p_glResizeBuffersMESA)(void);
        void       (WINE_GLAPI *p_glResolveDepthValuesNV)(void);
        void       (WINE_GLAPI *p_glResumeTransformFeedback)(void);
        void       (WINE_GLAPI *p_glResumeTransformFeedbackNV)(void);
        void       (WINE_GLAPI *p_glRotatexOES)(GLfixed,GLfixed,GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glSampleCoverage)(GLfloat,GLboolean);
        void       (WINE_GLAPI *p_glSampleCoverageARB)(GLfloat,GLboolean);
        void       (WINE_GLAPI *p_glSampleCoverageOES)(GLfixed,GLboolean);
        void       (WINE_GLAPI *p_glSampleMapATI)(GLuint,GLuint,GLenum);
        void       (WINE_GLAPI *p_glSampleMaskEXT)(GLclampf,GLboolean);
        void       (WINE_GLAPI *p_glSampleMaskIndexedNV)(GLuint,GLbitfield);
        void       (WINE_GLAPI *p_glSampleMaskSGIS)(GLclampf,GLboolean);
        void       (WINE_GLAPI *p_glSampleMaski)(GLuint,GLbitfield);
        void       (WINE_GLAPI *p_glSamplePatternEXT)(GLenum);
        void       (WINE_GLAPI *p_glSamplePatternSGIS)(GLenum);
        void       (WINE_GLAPI *p_glSamplerParameterIiv)(GLuint,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glSamplerParameterIuiv)(GLuint,GLenum,const GLuint*);
        void       (WINE_GLAPI *p_glSamplerParameterf)(GLuint,GLenum,GLfloat);
        void       (WINE_GLAPI *p_glSamplerParameterfv)(GLuint,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glSamplerParameteri)(GLuint,GLenum,GLint);
        void       (WINE_GLAPI *p_glSamplerParameteriv)(GLuint,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glScalexOES)(GLfixed,GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glScissorArrayv)(GLuint,GLsizei,const GLint*);
        void       (WINE_GLAPI *p_glScissorIndexed)(GLuint,GLint,GLint,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glScissorIndexedv)(GLuint,const GLint*);
        void       (WINE_GLAPI *p_glSecondaryColor3b)(GLbyte,GLbyte,GLbyte);
        void       (WINE_GLAPI *p_glSecondaryColor3bEXT)(GLbyte,GLbyte,GLbyte);
        void       (WINE_GLAPI *p_glSecondaryColor3bv)(const GLbyte*);
        void       (WINE_GLAPI *p_glSecondaryColor3bvEXT)(const GLbyte*);
        void       (WINE_GLAPI *p_glSecondaryColor3d)(GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glSecondaryColor3dEXT)(GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glSecondaryColor3dv)(const GLdouble*);
        void       (WINE_GLAPI *p_glSecondaryColor3dvEXT)(const GLdouble*);
        void       (WINE_GLAPI *p_glSecondaryColor3f)(GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glSecondaryColor3fEXT)(GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glSecondaryColor3fv)(const GLfloat*);
        void       (WINE_GLAPI *p_glSecondaryColor3fvEXT)(const GLfloat*);
        void       (WINE_GLAPI *p_glSecondaryColor3hNV)(GLhalfNV,GLhalfNV,GLhalfNV);
        void       (WINE_GLAPI *p_glSecondaryColor3hvNV)(const GLhalfNV*);
        void       (WINE_GLAPI *p_glSecondaryColor3i)(GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glSecondaryColor3iEXT)(GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glSecondaryColor3iv)(const GLint*);
        void       (WINE_GLAPI *p_glSecondaryColor3ivEXT)(const GLint*);
        void       (WINE_GLAPI *p_glSecondaryColor3s)(GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glSecondaryColor3sEXT)(GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glSecondaryColor3sv)(const GLshort*);
        void       (WINE_GLAPI *p_glSecondaryColor3svEXT)(const GLshort*);
        void       (WINE_GLAPI *p_glSecondaryColor3ub)(GLubyte,GLubyte,GLubyte);
        void       (WINE_GLAPI *p_glSecondaryColor3ubEXT)(GLubyte,GLubyte,GLubyte);
        void       (WINE_GLAPI *p_glSecondaryColor3ubv)(const GLubyte*);
        void       (WINE_GLAPI *p_glSecondaryColor3ubvEXT)(const GLubyte*);
        void       (WINE_GLAPI *p_glSecondaryColor3ui)(GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glSecondaryColor3uiEXT)(GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glSecondaryColor3uiv)(const GLuint*);
        void       (WINE_GLAPI *p_glSecondaryColor3uivEXT)(const GLuint*);
        void       (WINE_GLAPI *p_glSecondaryColor3us)(GLushort,GLushort,GLushort);
        void       (WINE_GLAPI *p_glSecondaryColor3usEXT)(GLushort,GLushort,GLushort);
        void       (WINE_GLAPI *p_glSecondaryColor3usv)(const GLushort*);
        void       (WINE_GLAPI *p_glSecondaryColor3usvEXT)(const GLushort*);
        void       (WINE_GLAPI *p_glSecondaryColorFormatNV)(GLint,GLenum,GLsizei);
        void       (WINE_GLAPI *p_glSecondaryColorP3ui)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glSecondaryColorP3uiv)(GLenum,const GLuint*);
        void       (WINE_GLAPI *p_glSecondaryColorPointer)(GLint,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glSecondaryColorPointerEXT)(GLint,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glSecondaryColorPointerListIBM)(GLint,GLenum,GLint,const void**,GLint);
        void       (WINE_GLAPI *p_glSelectPerfMonitorCountersAMD)(GLuint,GLboolean,GLuint,GLint,GLuint*);
        void       (WINE_GLAPI *p_glSelectTextureCoordSetSGIS)(GLenum);
        void       (WINE_GLAPI *p_glSelectTextureSGIS)(GLenum);
        void       (WINE_GLAPI *p_glSeparableFilter2D)(GLenum,GLenum,GLsizei,GLsizei,GLenum,GLenum,const void*,const void*);
        void       (WINE_GLAPI *p_glSeparableFilter2DEXT)(GLenum,GLenum,GLsizei,GLsizei,GLenum,GLenum,const void*,const void*);
        void       (WINE_GLAPI *p_glSetFenceAPPLE)(GLuint);
        void       (WINE_GLAPI *p_glSetFenceNV)(GLuint,GLenum);
        void       (WINE_GLAPI *p_glSetFragmentShaderConstantATI)(GLuint,const GLfloat*);
        void       (WINE_GLAPI *p_glSetInvariantEXT)(GLuint,GLenum,const void*);
        void       (WINE_GLAPI *p_glSetLocalConstantEXT)(GLuint,GLenum,const void*);
        void       (WINE_GLAPI *p_glSetMultisamplefvAMD)(GLenum,GLuint,const GLfloat*);
        void       (WINE_GLAPI *p_glShaderBinary)(GLsizei,const GLuint*,GLenum,const void*,GLsizei);
        void       (WINE_GLAPI *p_glShaderOp1EXT)(GLenum,GLuint,GLuint);
        void       (WINE_GLAPI *p_glShaderOp2EXT)(GLenum,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glShaderOp3EXT)(GLenum,GLuint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glShaderSource)(GLuint,GLsizei,const GLchar*const*,const GLint*);
        void       (WINE_GLAPI *p_glShaderSourceARB)(GLhandleARB,GLsizei,const GLcharARB**,const GLint*);
        void       (WINE_GLAPI *p_glShaderStorageBlockBinding)(GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glSharpenTexFuncSGIS)(GLenum,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glSpriteParameterfSGIX)(GLenum,GLfloat);
        void       (WINE_GLAPI *p_glSpriteParameterfvSGIX)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glSpriteParameteriSGIX)(GLenum,GLint);
        void       (WINE_GLAPI *p_glSpriteParameterivSGIX)(GLenum,const GLint*);
        void       (WINE_GLAPI *p_glStartInstrumentsSGIX)(void);
        void       (WINE_GLAPI *p_glStateCaptureNV)(GLuint,GLenum);
        void       (WINE_GLAPI *p_glStencilClearTagEXT)(GLsizei,GLuint);
        void       (WINE_GLAPI *p_glStencilFillPathInstancedNV)(GLsizei,GLenum,const void*,GLuint,GLenum,GLuint,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glStencilFillPathNV)(GLuint,GLenum,GLuint);
        void       (WINE_GLAPI *p_glStencilFuncSeparate)(GLenum,GLenum,GLint,GLuint);
        void       (WINE_GLAPI *p_glStencilFuncSeparateATI)(GLenum,GLenum,GLint,GLuint);
        void       (WINE_GLAPI *p_glStencilMaskSeparate)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glStencilOpSeparate)(GLenum,GLenum,GLenum,GLenum);
        void       (WINE_GLAPI *p_glStencilOpSeparateATI)(GLenum,GLenum,GLenum,GLenum);
        void       (WINE_GLAPI *p_glStencilOpValueAMD)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glStencilStrokePathInstancedNV)(GLsizei,GLenum,const void*,GLuint,GLint,GLuint,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glStencilStrokePathNV)(GLuint,GLint,GLuint);
        void       (WINE_GLAPI *p_glStencilThenCoverFillPathInstancedNV)(GLsizei,GLenum,const void*,GLuint,GLenum,GLuint,GLenum,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glStencilThenCoverFillPathNV)(GLuint,GLenum,GLuint,GLenum);
        void       (WINE_GLAPI *p_glStencilThenCoverStrokePathInstancedNV)(GLsizei,GLenum,const void*,GLuint,GLint,GLuint,GLenum,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glStencilThenCoverStrokePathNV)(GLuint,GLint,GLuint,GLenum);
        void       (WINE_GLAPI *p_glStopInstrumentsSGIX)(GLint);
        void       (WINE_GLAPI *p_glStringMarkerGREMEDY)(GLsizei,const void*);
        void       (WINE_GLAPI *p_glSubpixelPrecisionBiasNV)(GLuint,GLuint);
        void       (WINE_GLAPI *p_glSwizzleEXT)(GLuint,GLuint,GLenum,GLenum,GLenum,GLenum);
        void       (WINE_GLAPI *p_glSyncTextureINTEL)(GLuint);
        void       (WINE_GLAPI *p_glTagSampleBufferSGIX)(void);
        void       (WINE_GLAPI *p_glTangent3bEXT)(GLbyte,GLbyte,GLbyte);
        void       (WINE_GLAPI *p_glTangent3bvEXT)(const GLbyte*);
        void       (WINE_GLAPI *p_glTangent3dEXT)(GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glTangent3dvEXT)(const GLdouble*);
        void       (WINE_GLAPI *p_glTangent3fEXT)(GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glTangent3fvEXT)(const GLfloat*);
        void       (WINE_GLAPI *p_glTangent3iEXT)(GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glTangent3ivEXT)(const GLint*);
        void       (WINE_GLAPI *p_glTangent3sEXT)(GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glTangent3svEXT)(const GLshort*);
        void       (WINE_GLAPI *p_glTangentPointerEXT)(GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glTbufferMask3DFX)(GLuint);
        void       (WINE_GLAPI *p_glTessellationFactorAMD)(GLfloat);
        void       (WINE_GLAPI *p_glTessellationModeAMD)(GLenum);
        GLboolean  (WINE_GLAPI *p_glTestFenceAPPLE)(GLuint);
        GLboolean  (WINE_GLAPI *p_glTestFenceNV)(GLuint);
        GLboolean  (WINE_GLAPI *p_glTestObjectAPPLE)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glTexBuffer)(GLenum,GLenum,GLuint);
        void       (WINE_GLAPI *p_glTexBufferARB)(GLenum,GLenum,GLuint);
        void       (WINE_GLAPI *p_glTexBufferEXT)(GLenum,GLenum,GLuint);
        void       (WINE_GLAPI *p_glTexBufferRange)(GLenum,GLenum,GLuint,GLintptr,GLsizeiptr);
        void       (WINE_GLAPI *p_glTexBumpParameterfvATI)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glTexBumpParameterivATI)(GLenum,const GLint*);
        void       (WINE_GLAPI *p_glTexCoord1bOES)(GLbyte);
        void       (WINE_GLAPI *p_glTexCoord1bvOES)(const GLbyte*);
        void       (WINE_GLAPI *p_glTexCoord1hNV)(GLhalfNV);
        void       (WINE_GLAPI *p_glTexCoord1hvNV)(const GLhalfNV*);
        void       (WINE_GLAPI *p_glTexCoord1xOES)(GLfixed);
        void       (WINE_GLAPI *p_glTexCoord1xvOES)(const GLfixed*);
        void       (WINE_GLAPI *p_glTexCoord2bOES)(GLbyte,GLbyte);
        void       (WINE_GLAPI *p_glTexCoord2bvOES)(const GLbyte*);
        void       (WINE_GLAPI *p_glTexCoord2fColor3fVertex3fSUN)(GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glTexCoord2fColor3fVertex3fvSUN)(const GLfloat*,const GLfloat*,const GLfloat*);
        void       (WINE_GLAPI *p_glTexCoord2fColor4fNormal3fVertex3fSUN)(GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glTexCoord2fColor4fNormal3fVertex3fvSUN)(const GLfloat*,const GLfloat*,const GLfloat*,const GLfloat*);
        void       (WINE_GLAPI *p_glTexCoord2fColor4ubVertex3fSUN)(GLfloat,GLfloat,GLubyte,GLubyte,GLubyte,GLubyte,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glTexCoord2fColor4ubVertex3fvSUN)(const GLfloat*,const GLubyte*,const GLfloat*);
        void       (WINE_GLAPI *p_glTexCoord2fNormal3fVertex3fSUN)(GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glTexCoord2fNormal3fVertex3fvSUN)(const GLfloat*,const GLfloat*,const GLfloat*);
        void       (WINE_GLAPI *p_glTexCoord2fVertex3fSUN)(GLfloat,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glTexCoord2fVertex3fvSUN)(const GLfloat*,const GLfloat*);
        void       (WINE_GLAPI *p_glTexCoord2hNV)(GLhalfNV,GLhalfNV);
        void       (WINE_GLAPI *p_glTexCoord2hvNV)(const GLhalfNV*);
        void       (WINE_GLAPI *p_glTexCoord2xOES)(GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glTexCoord2xvOES)(const GLfixed*);
        void       (WINE_GLAPI *p_glTexCoord3bOES)(GLbyte,GLbyte,GLbyte);
        void       (WINE_GLAPI *p_glTexCoord3bvOES)(const GLbyte*);
        void       (WINE_GLAPI *p_glTexCoord3hNV)(GLhalfNV,GLhalfNV,GLhalfNV);
        void       (WINE_GLAPI *p_glTexCoord3hvNV)(const GLhalfNV*);
        void       (WINE_GLAPI *p_glTexCoord3xOES)(GLfixed,GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glTexCoord3xvOES)(const GLfixed*);
        void       (WINE_GLAPI *p_glTexCoord4bOES)(GLbyte,GLbyte,GLbyte,GLbyte);
        void       (WINE_GLAPI *p_glTexCoord4bvOES)(const GLbyte*);
        void       (WINE_GLAPI *p_glTexCoord4fColor4fNormal3fVertex4fSUN)(GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glTexCoord4fColor4fNormal3fVertex4fvSUN)(const GLfloat*,const GLfloat*,const GLfloat*,const GLfloat*);
        void       (WINE_GLAPI *p_glTexCoord4fVertex4fSUN)(GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glTexCoord4fVertex4fvSUN)(const GLfloat*,const GLfloat*);
        void       (WINE_GLAPI *p_glTexCoord4hNV)(GLhalfNV,GLhalfNV,GLhalfNV,GLhalfNV);
        void       (WINE_GLAPI *p_glTexCoord4hvNV)(const GLhalfNV*);
        void       (WINE_GLAPI *p_glTexCoord4xOES)(GLfixed,GLfixed,GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glTexCoord4xvOES)(const GLfixed*);
        void       (WINE_GLAPI *p_glTexCoordFormatNV)(GLint,GLenum,GLsizei);
        void       (WINE_GLAPI *p_glTexCoordP1ui)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glTexCoordP1uiv)(GLenum,const GLuint*);
        void       (WINE_GLAPI *p_glTexCoordP2ui)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glTexCoordP2uiv)(GLenum,const GLuint*);
        void       (WINE_GLAPI *p_glTexCoordP3ui)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glTexCoordP3uiv)(GLenum,const GLuint*);
        void       (WINE_GLAPI *p_glTexCoordP4ui)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glTexCoordP4uiv)(GLenum,const GLuint*);
        void       (WINE_GLAPI *p_glTexCoordPointerEXT)(GLint,GLenum,GLsizei,GLsizei,const void*);
        void       (WINE_GLAPI *p_glTexCoordPointerListIBM)(GLint,GLenum,GLint,const void**,GLint);
        void       (WINE_GLAPI *p_glTexCoordPointervINTEL)(GLint,GLenum,const void**);
        void       (WINE_GLAPI *p_glTexEnvxOES)(GLenum,GLenum,GLfixed);
        void       (WINE_GLAPI *p_glTexEnvxvOES)(GLenum,GLenum,const GLfixed*);
        void       (WINE_GLAPI *p_glTexFilterFuncSGIS)(GLenum,GLenum,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glTexGenxOES)(GLenum,GLenum,GLfixed);
        void       (WINE_GLAPI *p_glTexGenxvOES)(GLenum,GLenum,const GLfixed*);
        void       (WINE_GLAPI *p_glTexImage2DMultisample)(GLenum,GLsizei,GLenum,GLsizei,GLsizei,GLboolean);
        void       (WINE_GLAPI *p_glTexImage2DMultisampleCoverageNV)(GLenum,GLsizei,GLsizei,GLint,GLsizei,GLsizei,GLboolean);
        void       (WINE_GLAPI *p_glTexImage3D)(GLenum,GLint,GLint,GLsizei,GLsizei,GLsizei,GLint,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glTexImage3DEXT)(GLenum,GLint,GLenum,GLsizei,GLsizei,GLsizei,GLint,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glTexImage3DMultisample)(GLenum,GLsizei,GLenum,GLsizei,GLsizei,GLsizei,GLboolean);
        void       (WINE_GLAPI *p_glTexImage3DMultisampleCoverageNV)(GLenum,GLsizei,GLsizei,GLint,GLsizei,GLsizei,GLsizei,GLboolean);
        void       (WINE_GLAPI *p_glTexImage4DSGIS)(GLenum,GLint,GLenum,GLsizei,GLsizei,GLsizei,GLsizei,GLint,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glTexPageCommitmentARB)(GLenum,GLint,GLint,GLint,GLint,GLsizei,GLsizei,GLsizei,GLboolean);
        void       (WINE_GLAPI *p_glTexParameterIiv)(GLenum,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glTexParameterIivEXT)(GLenum,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glTexParameterIuiv)(GLenum,GLenum,const GLuint*);
        void       (WINE_GLAPI *p_glTexParameterIuivEXT)(GLenum,GLenum,const GLuint*);
        void       (WINE_GLAPI *p_glTexParameterxOES)(GLenum,GLenum,GLfixed);
        void       (WINE_GLAPI *p_glTexParameterxvOES)(GLenum,GLenum,const GLfixed*);
        void       (WINE_GLAPI *p_glTexRenderbufferNV)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glTexStorage1D)(GLenum,GLsizei,GLenum,GLsizei);
        void       (WINE_GLAPI *p_glTexStorage2D)(GLenum,GLsizei,GLenum,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glTexStorage2DMultisample)(GLenum,GLsizei,GLenum,GLsizei,GLsizei,GLboolean);
        void       (WINE_GLAPI *p_glTexStorage3D)(GLenum,GLsizei,GLenum,GLsizei,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glTexStorage3DMultisample)(GLenum,GLsizei,GLenum,GLsizei,GLsizei,GLsizei,GLboolean);
        void       (WINE_GLAPI *p_glTexStorageSparseAMD)(GLenum,GLenum,GLsizei,GLsizei,GLsizei,GLsizei,GLbitfield);
        void       (WINE_GLAPI *p_glTexSubImage1DEXT)(GLenum,GLint,GLint,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glTexSubImage2DEXT)(GLenum,GLint,GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glTexSubImage3D)(GLenum,GLint,GLint,GLint,GLint,GLsizei,GLsizei,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glTexSubImage3DEXT)(GLenum,GLint,GLint,GLint,GLint,GLsizei,GLsizei,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glTexSubImage4DSGIS)(GLenum,GLint,GLint,GLint,GLint,GLint,GLsizei,GLsizei,GLsizei,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glTextureBarrier)(void);
        void       (WINE_GLAPI *p_glTextureBarrierNV)(void);
        void       (WINE_GLAPI *p_glTextureBuffer)(GLuint,GLenum,GLuint);
        void       (WINE_GLAPI *p_glTextureBufferEXT)(GLuint,GLenum,GLenum,GLuint);
        void       (WINE_GLAPI *p_glTextureBufferRange)(GLuint,GLenum,GLuint,GLintptr,GLsizeiptr);
        void       (WINE_GLAPI *p_glTextureBufferRangeEXT)(GLuint,GLenum,GLenum,GLuint,GLintptr,GLsizeiptr);
        void       (WINE_GLAPI *p_glTextureColorMaskSGIS)(GLboolean,GLboolean,GLboolean,GLboolean);
        void       (WINE_GLAPI *p_glTextureImage1DEXT)(GLuint,GLenum,GLint,GLint,GLsizei,GLint,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glTextureImage2DEXT)(GLuint,GLenum,GLint,GLint,GLsizei,GLsizei,GLint,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glTextureImage2DMultisampleCoverageNV)(GLuint,GLenum,GLsizei,GLsizei,GLint,GLsizei,GLsizei,GLboolean);
        void       (WINE_GLAPI *p_glTextureImage2DMultisampleNV)(GLuint,GLenum,GLsizei,GLint,GLsizei,GLsizei,GLboolean);
        void       (WINE_GLAPI *p_glTextureImage3DEXT)(GLuint,GLenum,GLint,GLint,GLsizei,GLsizei,GLsizei,GLint,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glTextureImage3DMultisampleCoverageNV)(GLuint,GLenum,GLsizei,GLsizei,GLint,GLsizei,GLsizei,GLsizei,GLboolean);
        void       (WINE_GLAPI *p_glTextureImage3DMultisampleNV)(GLuint,GLenum,GLsizei,GLint,GLsizei,GLsizei,GLsizei,GLboolean);
        void       (WINE_GLAPI *p_glTextureLightEXT)(GLenum);
        void       (WINE_GLAPI *p_glTextureMaterialEXT)(GLenum,GLenum);
        void       (WINE_GLAPI *p_glTextureNormalEXT)(GLenum);
        void       (WINE_GLAPI *p_glTexturePageCommitmentEXT)(GLuint,GLint,GLint,GLint,GLint,GLsizei,GLsizei,GLsizei,GLboolean);
        void       (WINE_GLAPI *p_glTextureParameterIiv)(GLuint,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glTextureParameterIivEXT)(GLuint,GLenum,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glTextureParameterIuiv)(GLuint,GLenum,const GLuint*);
        void       (WINE_GLAPI *p_glTextureParameterIuivEXT)(GLuint,GLenum,GLenum,const GLuint*);
        void       (WINE_GLAPI *p_glTextureParameterf)(GLuint,GLenum,GLfloat);
        void       (WINE_GLAPI *p_glTextureParameterfEXT)(GLuint,GLenum,GLenum,GLfloat);
        void       (WINE_GLAPI *p_glTextureParameterfv)(GLuint,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glTextureParameterfvEXT)(GLuint,GLenum,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glTextureParameteri)(GLuint,GLenum,GLint);
        void       (WINE_GLAPI *p_glTextureParameteriEXT)(GLuint,GLenum,GLenum,GLint);
        void       (WINE_GLAPI *p_glTextureParameteriv)(GLuint,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glTextureParameterivEXT)(GLuint,GLenum,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glTextureRangeAPPLE)(GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glTextureRenderbufferEXT)(GLuint,GLenum,GLuint);
        void       (WINE_GLAPI *p_glTextureStorage1D)(GLuint,GLsizei,GLenum,GLsizei);
        void       (WINE_GLAPI *p_glTextureStorage1DEXT)(GLuint,GLenum,GLsizei,GLenum,GLsizei);
        void       (WINE_GLAPI *p_glTextureStorage2D)(GLuint,GLsizei,GLenum,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glTextureStorage2DEXT)(GLuint,GLenum,GLsizei,GLenum,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glTextureStorage2DMultisample)(GLuint,GLsizei,GLenum,GLsizei,GLsizei,GLboolean);
        void       (WINE_GLAPI *p_glTextureStorage2DMultisampleEXT)(GLuint,GLenum,GLsizei,GLenum,GLsizei,GLsizei,GLboolean);
        void       (WINE_GLAPI *p_glTextureStorage3D)(GLuint,GLsizei,GLenum,GLsizei,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glTextureStorage3DEXT)(GLuint,GLenum,GLsizei,GLenum,GLsizei,GLsizei,GLsizei);
        void       (WINE_GLAPI *p_glTextureStorage3DMultisample)(GLuint,GLsizei,GLenum,GLsizei,GLsizei,GLsizei,GLboolean);
        void       (WINE_GLAPI *p_glTextureStorage3DMultisampleEXT)(GLuint,GLenum,GLsizei,GLenum,GLsizei,GLsizei,GLsizei,GLboolean);
        void       (WINE_GLAPI *p_glTextureStorageSparseAMD)(GLuint,GLenum,GLenum,GLsizei,GLsizei,GLsizei,GLsizei,GLbitfield);
        void       (WINE_GLAPI *p_glTextureSubImage1D)(GLuint,GLint,GLint,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glTextureSubImage1DEXT)(GLuint,GLenum,GLint,GLint,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glTextureSubImage2D)(GLuint,GLint,GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glTextureSubImage2DEXT)(GLuint,GLenum,GLint,GLint,GLint,GLsizei,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glTextureSubImage3D)(GLuint,GLint,GLint,GLint,GLint,GLsizei,GLsizei,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glTextureSubImage3DEXT)(GLuint,GLenum,GLint,GLint,GLint,GLint,GLsizei,GLsizei,GLsizei,GLenum,GLenum,const void*);
        void       (WINE_GLAPI *p_glTextureView)(GLuint,GLenum,GLuint,GLenum,GLuint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glTrackMatrixNV)(GLenum,GLuint,GLenum,GLenum);
        void       (WINE_GLAPI *p_glTransformFeedbackAttribsNV)(GLsizei,const GLint*,GLenum);
        void       (WINE_GLAPI *p_glTransformFeedbackBufferBase)(GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glTransformFeedbackBufferRange)(GLuint,GLuint,GLuint,GLintptr,GLsizeiptr);
        void       (WINE_GLAPI *p_glTransformFeedbackStreamAttribsNV)(GLsizei,const GLint*,GLsizei,const GLint*,GLenum);
        void       (WINE_GLAPI *p_glTransformFeedbackVaryings)(GLuint,GLsizei,const GLchar*const*,GLenum);
        void       (WINE_GLAPI *p_glTransformFeedbackVaryingsEXT)(GLuint,GLsizei,const GLchar*const*,GLenum);
        void       (WINE_GLAPI *p_glTransformFeedbackVaryingsNV)(GLuint,GLsizei,const GLint*,GLenum);
        void       (WINE_GLAPI *p_glTransformPathNV)(GLuint,GLuint,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glTranslatexOES)(GLfixed,GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glUniform1d)(GLint,GLdouble);
        void       (WINE_GLAPI *p_glUniform1dv)(GLint,GLsizei,const GLdouble*);
        void       (WINE_GLAPI *p_glUniform1f)(GLint,GLfloat);
        void       (WINE_GLAPI *p_glUniform1fARB)(GLint,GLfloat);
        void       (WINE_GLAPI *p_glUniform1fv)(GLint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glUniform1fvARB)(GLint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glUniform1i)(GLint,GLint);
        void       (WINE_GLAPI *p_glUniform1i64NV)(GLint,GLint64EXT);
        void       (WINE_GLAPI *p_glUniform1i64vNV)(GLint,GLsizei,const GLint64EXT*);
        void       (WINE_GLAPI *p_glUniform1iARB)(GLint,GLint);
        void       (WINE_GLAPI *p_glUniform1iv)(GLint,GLsizei,const GLint*);
        void       (WINE_GLAPI *p_glUniform1ivARB)(GLint,GLsizei,const GLint*);
        void       (WINE_GLAPI *p_glUniform1ui)(GLint,GLuint);
        void       (WINE_GLAPI *p_glUniform1ui64NV)(GLint,GLuint64EXT);
        void       (WINE_GLAPI *p_glUniform1ui64vNV)(GLint,GLsizei,const GLuint64EXT*);
        void       (WINE_GLAPI *p_glUniform1uiEXT)(GLint,GLuint);
        void       (WINE_GLAPI *p_glUniform1uiv)(GLint,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glUniform1uivEXT)(GLint,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glUniform2d)(GLint,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glUniform2dv)(GLint,GLsizei,const GLdouble*);
        void       (WINE_GLAPI *p_glUniform2f)(GLint,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glUniform2fARB)(GLint,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glUniform2fv)(GLint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glUniform2fvARB)(GLint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glUniform2i)(GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glUniform2i64NV)(GLint,GLint64EXT,GLint64EXT);
        void       (WINE_GLAPI *p_glUniform2i64vNV)(GLint,GLsizei,const GLint64EXT*);
        void       (WINE_GLAPI *p_glUniform2iARB)(GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glUniform2iv)(GLint,GLsizei,const GLint*);
        void       (WINE_GLAPI *p_glUniform2ivARB)(GLint,GLsizei,const GLint*);
        void       (WINE_GLAPI *p_glUniform2ui)(GLint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glUniform2ui64NV)(GLint,GLuint64EXT,GLuint64EXT);
        void       (WINE_GLAPI *p_glUniform2ui64vNV)(GLint,GLsizei,const GLuint64EXT*);
        void       (WINE_GLAPI *p_glUniform2uiEXT)(GLint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glUniform2uiv)(GLint,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glUniform2uivEXT)(GLint,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glUniform3d)(GLint,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glUniform3dv)(GLint,GLsizei,const GLdouble*);
        void       (WINE_GLAPI *p_glUniform3f)(GLint,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glUniform3fARB)(GLint,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glUniform3fv)(GLint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glUniform3fvARB)(GLint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glUniform3i)(GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glUniform3i64NV)(GLint,GLint64EXT,GLint64EXT,GLint64EXT);
        void       (WINE_GLAPI *p_glUniform3i64vNV)(GLint,GLsizei,const GLint64EXT*);
        void       (WINE_GLAPI *p_glUniform3iARB)(GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glUniform3iv)(GLint,GLsizei,const GLint*);
        void       (WINE_GLAPI *p_glUniform3ivARB)(GLint,GLsizei,const GLint*);
        void       (WINE_GLAPI *p_glUniform3ui)(GLint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glUniform3ui64NV)(GLint,GLuint64EXT,GLuint64EXT,GLuint64EXT);
        void       (WINE_GLAPI *p_glUniform3ui64vNV)(GLint,GLsizei,const GLuint64EXT*);
        void       (WINE_GLAPI *p_glUniform3uiEXT)(GLint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glUniform3uiv)(GLint,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glUniform3uivEXT)(GLint,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glUniform4d)(GLint,GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glUniform4dv)(GLint,GLsizei,const GLdouble*);
        void       (WINE_GLAPI *p_glUniform4f)(GLint,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glUniform4fARB)(GLint,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glUniform4fv)(GLint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glUniform4fvARB)(GLint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glUniform4i)(GLint,GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glUniform4i64NV)(GLint,GLint64EXT,GLint64EXT,GLint64EXT,GLint64EXT);
        void       (WINE_GLAPI *p_glUniform4i64vNV)(GLint,GLsizei,const GLint64EXT*);
        void       (WINE_GLAPI *p_glUniform4iARB)(GLint,GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glUniform4iv)(GLint,GLsizei,const GLint*);
        void       (WINE_GLAPI *p_glUniform4ivARB)(GLint,GLsizei,const GLint*);
        void       (WINE_GLAPI *p_glUniform4ui)(GLint,GLuint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glUniform4ui64NV)(GLint,GLuint64EXT,GLuint64EXT,GLuint64EXT,GLuint64EXT);
        void       (WINE_GLAPI *p_glUniform4ui64vNV)(GLint,GLsizei,const GLuint64EXT*);
        void       (WINE_GLAPI *p_glUniform4uiEXT)(GLint,GLuint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glUniform4uiv)(GLint,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glUniform4uivEXT)(GLint,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glUniformBlockBinding)(GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glUniformBufferEXT)(GLuint,GLint,GLuint);
        void       (WINE_GLAPI *p_glUniformHandleui64ARB)(GLint,GLuint64);
        void       (WINE_GLAPI *p_glUniformHandleui64NV)(GLint,GLuint64);
        void       (WINE_GLAPI *p_glUniformHandleui64vARB)(GLint,GLsizei,const GLuint64*);
        void       (WINE_GLAPI *p_glUniformHandleui64vNV)(GLint,GLsizei,const GLuint64*);
        void       (WINE_GLAPI *p_glUniformMatrix2dv)(GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glUniformMatrix2fv)(GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glUniformMatrix2fvARB)(GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glUniformMatrix2x3dv)(GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glUniformMatrix2x3fv)(GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glUniformMatrix2x4dv)(GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glUniformMatrix2x4fv)(GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glUniformMatrix3dv)(GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glUniformMatrix3fv)(GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glUniformMatrix3fvARB)(GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glUniformMatrix3x2dv)(GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glUniformMatrix3x2fv)(GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glUniformMatrix3x4dv)(GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glUniformMatrix3x4fv)(GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glUniformMatrix4dv)(GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glUniformMatrix4fv)(GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glUniformMatrix4fvARB)(GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glUniformMatrix4x2dv)(GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glUniformMatrix4x2fv)(GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glUniformMatrix4x3dv)(GLint,GLsizei,GLboolean,const GLdouble*);
        void       (WINE_GLAPI *p_glUniformMatrix4x3fv)(GLint,GLsizei,GLboolean,const GLfloat*);
        void       (WINE_GLAPI *p_glUniformSubroutinesuiv)(GLenum,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glUniformui64NV)(GLint,GLuint64EXT);
        void       (WINE_GLAPI *p_glUniformui64vNV)(GLint,GLsizei,const GLuint64EXT*);
        void       (WINE_GLAPI *p_glUnlockArraysEXT)(void);
        GLboolean  (WINE_GLAPI *p_glUnmapBuffer)(GLenum);
        GLboolean  (WINE_GLAPI *p_glUnmapBufferARB)(GLenum);
        GLboolean  (WINE_GLAPI *p_glUnmapNamedBuffer)(GLuint);
        GLboolean  (WINE_GLAPI *p_glUnmapNamedBufferEXT)(GLuint);
        void       (WINE_GLAPI *p_glUnmapObjectBufferATI)(GLuint);
        void       (WINE_GLAPI *p_glUnmapTexture2DINTEL)(GLuint,GLint);
        void       (WINE_GLAPI *p_glUpdateObjectBufferATI)(GLuint,GLuint,GLsizei,const void*,GLenum);
        void       (WINE_GLAPI *p_glUseProgram)(GLuint);
        void       (WINE_GLAPI *p_glUseProgramObjectARB)(GLhandleARB);
        void       (WINE_GLAPI *p_glUseProgramStages)(GLuint,GLbitfield,GLuint);
        void       (WINE_GLAPI *p_glUseShaderProgramEXT)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glVDPAUFiniNV)(void);
        void       (WINE_GLAPI *p_glVDPAUGetSurfaceivNV)(GLvdpauSurfaceNV,GLenum,GLsizei,GLsizei*,GLint*);
        void       (WINE_GLAPI *p_glVDPAUInitNV)(const void*,const void*);
        GLboolean  (WINE_GLAPI *p_glVDPAUIsSurfaceNV)(GLvdpauSurfaceNV);
        void       (WINE_GLAPI *p_glVDPAUMapSurfacesNV)(GLsizei,const GLvdpauSurfaceNV*);
        GLvdpauSurfaceNV (WINE_GLAPI *p_glVDPAURegisterOutputSurfaceNV)(const void*,GLenum,GLsizei,const GLuint*);
        GLvdpauSurfaceNV (WINE_GLAPI *p_glVDPAURegisterVideoSurfaceNV)(const void*,GLenum,GLsizei,const GLuint*);
        void       (WINE_GLAPI *p_glVDPAUSurfaceAccessNV)(GLvdpauSurfaceNV,GLenum);
        void       (WINE_GLAPI *p_glVDPAUUnmapSurfacesNV)(GLsizei,const GLvdpauSurfaceNV*);
        void       (WINE_GLAPI *p_glVDPAUUnregisterSurfaceNV)(GLvdpauSurfaceNV);
        void       (WINE_GLAPI *p_glValidateProgram)(GLuint);
        void       (WINE_GLAPI *p_glValidateProgramARB)(GLhandleARB);
        void       (WINE_GLAPI *p_glValidateProgramPipeline)(GLuint);
        void       (WINE_GLAPI *p_glVariantArrayObjectATI)(GLuint,GLenum,GLsizei,GLuint,GLuint);
        void       (WINE_GLAPI *p_glVariantPointerEXT)(GLuint,GLenum,GLuint,const void*);
        void       (WINE_GLAPI *p_glVariantbvEXT)(GLuint,const GLbyte*);
        void       (WINE_GLAPI *p_glVariantdvEXT)(GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glVariantfvEXT)(GLuint,const GLfloat*);
        void       (WINE_GLAPI *p_glVariantivEXT)(GLuint,const GLint*);
        void       (WINE_GLAPI *p_glVariantsvEXT)(GLuint,const GLshort*);
        void       (WINE_GLAPI *p_glVariantubvEXT)(GLuint,const GLubyte*);
        void       (WINE_GLAPI *p_glVariantuivEXT)(GLuint,const GLuint*);
        void       (WINE_GLAPI *p_glVariantusvEXT)(GLuint,const GLushort*);
        void       (WINE_GLAPI *p_glVertex2bOES)(GLbyte,GLbyte);
        void       (WINE_GLAPI *p_glVertex2bvOES)(const GLbyte*);
        void       (WINE_GLAPI *p_glVertex2hNV)(GLhalfNV,GLhalfNV);
        void       (WINE_GLAPI *p_glVertex2hvNV)(const GLhalfNV*);
        void       (WINE_GLAPI *p_glVertex2xOES)(GLfixed);
        void       (WINE_GLAPI *p_glVertex2xvOES)(const GLfixed*);
        void       (WINE_GLAPI *p_glVertex3bOES)(GLbyte,GLbyte,GLbyte);
        void       (WINE_GLAPI *p_glVertex3bvOES)(const GLbyte*);
        void       (WINE_GLAPI *p_glVertex3hNV)(GLhalfNV,GLhalfNV,GLhalfNV);
        void       (WINE_GLAPI *p_glVertex3hvNV)(const GLhalfNV*);
        void       (WINE_GLAPI *p_glVertex3xOES)(GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glVertex3xvOES)(const GLfixed*);
        void       (WINE_GLAPI *p_glVertex4bOES)(GLbyte,GLbyte,GLbyte,GLbyte);
        void       (WINE_GLAPI *p_glVertex4bvOES)(const GLbyte*);
        void       (WINE_GLAPI *p_glVertex4hNV)(GLhalfNV,GLhalfNV,GLhalfNV,GLhalfNV);
        void       (WINE_GLAPI *p_glVertex4hvNV)(const GLhalfNV*);
        void       (WINE_GLAPI *p_glVertex4xOES)(GLfixed,GLfixed,GLfixed);
        void       (WINE_GLAPI *p_glVertex4xvOES)(const GLfixed*);
        void       (WINE_GLAPI *p_glVertexArrayAttribBinding)(GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glVertexArrayAttribFormat)(GLuint,GLuint,GLint,GLenum,GLboolean,GLuint);
        void       (WINE_GLAPI *p_glVertexArrayAttribIFormat)(GLuint,GLuint,GLint,GLenum,GLuint);
        void       (WINE_GLAPI *p_glVertexArrayAttribLFormat)(GLuint,GLuint,GLint,GLenum,GLuint);
        void       (WINE_GLAPI *p_glVertexArrayBindVertexBufferEXT)(GLuint,GLuint,GLuint,GLintptr,GLsizei);
        void       (WINE_GLAPI *p_glVertexArrayBindingDivisor)(GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glVertexArrayColorOffsetEXT)(GLuint,GLuint,GLint,GLenum,GLsizei,GLintptr);
        void       (WINE_GLAPI *p_glVertexArrayEdgeFlagOffsetEXT)(GLuint,GLuint,GLsizei,GLintptr);
        void       (WINE_GLAPI *p_glVertexArrayElementBuffer)(GLuint,GLuint);
        void       (WINE_GLAPI *p_glVertexArrayFogCoordOffsetEXT)(GLuint,GLuint,GLenum,GLsizei,GLintptr);
        void       (WINE_GLAPI *p_glVertexArrayIndexOffsetEXT)(GLuint,GLuint,GLenum,GLsizei,GLintptr);
        void       (WINE_GLAPI *p_glVertexArrayMultiTexCoordOffsetEXT)(GLuint,GLuint,GLenum,GLint,GLenum,GLsizei,GLintptr);
        void       (WINE_GLAPI *p_glVertexArrayNormalOffsetEXT)(GLuint,GLuint,GLenum,GLsizei,GLintptr);
        void       (WINE_GLAPI *p_glVertexArrayParameteriAPPLE)(GLenum,GLint);
        void       (WINE_GLAPI *p_glVertexArrayRangeAPPLE)(GLsizei,void*);
        void       (WINE_GLAPI *p_glVertexArrayRangeNV)(GLsizei,const void*);
        void       (WINE_GLAPI *p_glVertexArraySecondaryColorOffsetEXT)(GLuint,GLuint,GLint,GLenum,GLsizei,GLintptr);
        void       (WINE_GLAPI *p_glVertexArrayTexCoordOffsetEXT)(GLuint,GLuint,GLint,GLenum,GLsizei,GLintptr);
        void       (WINE_GLAPI *p_glVertexArrayVertexAttribBindingEXT)(GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glVertexArrayVertexAttribDivisorEXT)(GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glVertexArrayVertexAttribFormatEXT)(GLuint,GLuint,GLint,GLenum,GLboolean,GLuint);
        void       (WINE_GLAPI *p_glVertexArrayVertexAttribIFormatEXT)(GLuint,GLuint,GLint,GLenum,GLuint);
        void       (WINE_GLAPI *p_glVertexArrayVertexAttribIOffsetEXT)(GLuint,GLuint,GLuint,GLint,GLenum,GLsizei,GLintptr);
        void       (WINE_GLAPI *p_glVertexArrayVertexAttribLFormatEXT)(GLuint,GLuint,GLint,GLenum,GLuint);
        void       (WINE_GLAPI *p_glVertexArrayVertexAttribLOffsetEXT)(GLuint,GLuint,GLuint,GLint,GLenum,GLsizei,GLintptr);
        void       (WINE_GLAPI *p_glVertexArrayVertexAttribOffsetEXT)(GLuint,GLuint,GLuint,GLint,GLenum,GLboolean,GLsizei,GLintptr);
        void       (WINE_GLAPI *p_glVertexArrayVertexBindingDivisorEXT)(GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glVertexArrayVertexBuffer)(GLuint,GLuint,GLuint,GLintptr,GLsizei);
        void       (WINE_GLAPI *p_glVertexArrayVertexBuffers)(GLuint,GLuint,GLsizei,const GLuint*,const GLintptr*,const GLsizei*);
        void       (WINE_GLAPI *p_glVertexArrayVertexOffsetEXT)(GLuint,GLuint,GLint,GLenum,GLsizei,GLintptr);
        void       (WINE_GLAPI *p_glVertexAttrib1d)(GLuint,GLdouble);
        void       (WINE_GLAPI *p_glVertexAttrib1dARB)(GLuint,GLdouble);
        void       (WINE_GLAPI *p_glVertexAttrib1dNV)(GLuint,GLdouble);
        void       (WINE_GLAPI *p_glVertexAttrib1dv)(GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexAttrib1dvARB)(GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexAttrib1dvNV)(GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexAttrib1f)(GLuint,GLfloat);
        void       (WINE_GLAPI *p_glVertexAttrib1fARB)(GLuint,GLfloat);
        void       (WINE_GLAPI *p_glVertexAttrib1fNV)(GLuint,GLfloat);
        void       (WINE_GLAPI *p_glVertexAttrib1fv)(GLuint,const GLfloat*);
        void       (WINE_GLAPI *p_glVertexAttrib1fvARB)(GLuint,const GLfloat*);
        void       (WINE_GLAPI *p_glVertexAttrib1fvNV)(GLuint,const GLfloat*);
        void       (WINE_GLAPI *p_glVertexAttrib1hNV)(GLuint,GLhalfNV);
        void       (WINE_GLAPI *p_glVertexAttrib1hvNV)(GLuint,const GLhalfNV*);
        void       (WINE_GLAPI *p_glVertexAttrib1s)(GLuint,GLshort);
        void       (WINE_GLAPI *p_glVertexAttrib1sARB)(GLuint,GLshort);
        void       (WINE_GLAPI *p_glVertexAttrib1sNV)(GLuint,GLshort);
        void       (WINE_GLAPI *p_glVertexAttrib1sv)(GLuint,const GLshort*);
        void       (WINE_GLAPI *p_glVertexAttrib1svARB)(GLuint,const GLshort*);
        void       (WINE_GLAPI *p_glVertexAttrib1svNV)(GLuint,const GLshort*);
        void       (WINE_GLAPI *p_glVertexAttrib2d)(GLuint,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glVertexAttrib2dARB)(GLuint,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glVertexAttrib2dNV)(GLuint,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glVertexAttrib2dv)(GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexAttrib2dvARB)(GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexAttrib2dvNV)(GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexAttrib2f)(GLuint,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glVertexAttrib2fARB)(GLuint,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glVertexAttrib2fNV)(GLuint,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glVertexAttrib2fv)(GLuint,const GLfloat*);
        void       (WINE_GLAPI *p_glVertexAttrib2fvARB)(GLuint,const GLfloat*);
        void       (WINE_GLAPI *p_glVertexAttrib2fvNV)(GLuint,const GLfloat*);
        void       (WINE_GLAPI *p_glVertexAttrib2hNV)(GLuint,GLhalfNV,GLhalfNV);
        void       (WINE_GLAPI *p_glVertexAttrib2hvNV)(GLuint,const GLhalfNV*);
        void       (WINE_GLAPI *p_glVertexAttrib2s)(GLuint,GLshort,GLshort);
        void       (WINE_GLAPI *p_glVertexAttrib2sARB)(GLuint,GLshort,GLshort);
        void       (WINE_GLAPI *p_glVertexAttrib2sNV)(GLuint,GLshort,GLshort);
        void       (WINE_GLAPI *p_glVertexAttrib2sv)(GLuint,const GLshort*);
        void       (WINE_GLAPI *p_glVertexAttrib2svARB)(GLuint,const GLshort*);
        void       (WINE_GLAPI *p_glVertexAttrib2svNV)(GLuint,const GLshort*);
        void       (WINE_GLAPI *p_glVertexAttrib3d)(GLuint,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glVertexAttrib3dARB)(GLuint,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glVertexAttrib3dNV)(GLuint,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glVertexAttrib3dv)(GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexAttrib3dvARB)(GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexAttrib3dvNV)(GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexAttrib3f)(GLuint,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glVertexAttrib3fARB)(GLuint,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glVertexAttrib3fNV)(GLuint,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glVertexAttrib3fv)(GLuint,const GLfloat*);
        void       (WINE_GLAPI *p_glVertexAttrib3fvARB)(GLuint,const GLfloat*);
        void       (WINE_GLAPI *p_glVertexAttrib3fvNV)(GLuint,const GLfloat*);
        void       (WINE_GLAPI *p_glVertexAttrib3hNV)(GLuint,GLhalfNV,GLhalfNV,GLhalfNV);
        void       (WINE_GLAPI *p_glVertexAttrib3hvNV)(GLuint,const GLhalfNV*);
        void       (WINE_GLAPI *p_glVertexAttrib3s)(GLuint,GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glVertexAttrib3sARB)(GLuint,GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glVertexAttrib3sNV)(GLuint,GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glVertexAttrib3sv)(GLuint,const GLshort*);
        void       (WINE_GLAPI *p_glVertexAttrib3svARB)(GLuint,const GLshort*);
        void       (WINE_GLAPI *p_glVertexAttrib3svNV)(GLuint,const GLshort*);
        void       (WINE_GLAPI *p_glVertexAttrib4Nbv)(GLuint,const GLbyte*);
        void       (WINE_GLAPI *p_glVertexAttrib4NbvARB)(GLuint,const GLbyte*);
        void       (WINE_GLAPI *p_glVertexAttrib4Niv)(GLuint,const GLint*);
        void       (WINE_GLAPI *p_glVertexAttrib4NivARB)(GLuint,const GLint*);
        void       (WINE_GLAPI *p_glVertexAttrib4Nsv)(GLuint,const GLshort*);
        void       (WINE_GLAPI *p_glVertexAttrib4NsvARB)(GLuint,const GLshort*);
        void       (WINE_GLAPI *p_glVertexAttrib4Nub)(GLuint,GLubyte,GLubyte,GLubyte,GLubyte);
        void       (WINE_GLAPI *p_glVertexAttrib4NubARB)(GLuint,GLubyte,GLubyte,GLubyte,GLubyte);
        void       (WINE_GLAPI *p_glVertexAttrib4Nubv)(GLuint,const GLubyte*);
        void       (WINE_GLAPI *p_glVertexAttrib4NubvARB)(GLuint,const GLubyte*);
        void       (WINE_GLAPI *p_glVertexAttrib4Nuiv)(GLuint,const GLuint*);
        void       (WINE_GLAPI *p_glVertexAttrib4NuivARB)(GLuint,const GLuint*);
        void       (WINE_GLAPI *p_glVertexAttrib4Nusv)(GLuint,const GLushort*);
        void       (WINE_GLAPI *p_glVertexAttrib4NusvARB)(GLuint,const GLushort*);
        void       (WINE_GLAPI *p_glVertexAttrib4bv)(GLuint,const GLbyte*);
        void       (WINE_GLAPI *p_glVertexAttrib4bvARB)(GLuint,const GLbyte*);
        void       (WINE_GLAPI *p_glVertexAttrib4d)(GLuint,GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glVertexAttrib4dARB)(GLuint,GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glVertexAttrib4dNV)(GLuint,GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glVertexAttrib4dv)(GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexAttrib4dvARB)(GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexAttrib4dvNV)(GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexAttrib4f)(GLuint,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glVertexAttrib4fARB)(GLuint,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glVertexAttrib4fNV)(GLuint,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glVertexAttrib4fv)(GLuint,const GLfloat*);
        void       (WINE_GLAPI *p_glVertexAttrib4fvARB)(GLuint,const GLfloat*);
        void       (WINE_GLAPI *p_glVertexAttrib4fvNV)(GLuint,const GLfloat*);
        void       (WINE_GLAPI *p_glVertexAttrib4hNV)(GLuint,GLhalfNV,GLhalfNV,GLhalfNV,GLhalfNV);
        void       (WINE_GLAPI *p_glVertexAttrib4hvNV)(GLuint,const GLhalfNV*);
        void       (WINE_GLAPI *p_glVertexAttrib4iv)(GLuint,const GLint*);
        void       (WINE_GLAPI *p_glVertexAttrib4ivARB)(GLuint,const GLint*);
        void       (WINE_GLAPI *p_glVertexAttrib4s)(GLuint,GLshort,GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glVertexAttrib4sARB)(GLuint,GLshort,GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glVertexAttrib4sNV)(GLuint,GLshort,GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glVertexAttrib4sv)(GLuint,const GLshort*);
        void       (WINE_GLAPI *p_glVertexAttrib4svARB)(GLuint,const GLshort*);
        void       (WINE_GLAPI *p_glVertexAttrib4svNV)(GLuint,const GLshort*);
        void       (WINE_GLAPI *p_glVertexAttrib4ubNV)(GLuint,GLubyte,GLubyte,GLubyte,GLubyte);
        void       (WINE_GLAPI *p_glVertexAttrib4ubv)(GLuint,const GLubyte*);
        void       (WINE_GLAPI *p_glVertexAttrib4ubvARB)(GLuint,const GLubyte*);
        void       (WINE_GLAPI *p_glVertexAttrib4ubvNV)(GLuint,const GLubyte*);
        void       (WINE_GLAPI *p_glVertexAttrib4uiv)(GLuint,const GLuint*);
        void       (WINE_GLAPI *p_glVertexAttrib4uivARB)(GLuint,const GLuint*);
        void       (WINE_GLAPI *p_glVertexAttrib4usv)(GLuint,const GLushort*);
        void       (WINE_GLAPI *p_glVertexAttrib4usvARB)(GLuint,const GLushort*);
        void       (WINE_GLAPI *p_glVertexAttribArrayObjectATI)(GLuint,GLint,GLenum,GLboolean,GLsizei,GLuint,GLuint);
        void       (WINE_GLAPI *p_glVertexAttribBinding)(GLuint,GLuint);
        void       (WINE_GLAPI *p_glVertexAttribDivisor)(GLuint,GLuint);
        void       (WINE_GLAPI *p_glVertexAttribDivisorARB)(GLuint,GLuint);
        void       (WINE_GLAPI *p_glVertexAttribFormat)(GLuint,GLint,GLenum,GLboolean,GLuint);
        void       (WINE_GLAPI *p_glVertexAttribFormatNV)(GLuint,GLint,GLenum,GLboolean,GLsizei);
        void       (WINE_GLAPI *p_glVertexAttribI1i)(GLuint,GLint);
        void       (WINE_GLAPI *p_glVertexAttribI1iEXT)(GLuint,GLint);
        void       (WINE_GLAPI *p_glVertexAttribI1iv)(GLuint,const GLint*);
        void       (WINE_GLAPI *p_glVertexAttribI1ivEXT)(GLuint,const GLint*);
        void       (WINE_GLAPI *p_glVertexAttribI1ui)(GLuint,GLuint);
        void       (WINE_GLAPI *p_glVertexAttribI1uiEXT)(GLuint,GLuint);
        void       (WINE_GLAPI *p_glVertexAttribI1uiv)(GLuint,const GLuint*);
        void       (WINE_GLAPI *p_glVertexAttribI1uivEXT)(GLuint,const GLuint*);
        void       (WINE_GLAPI *p_glVertexAttribI2i)(GLuint,GLint,GLint);
        void       (WINE_GLAPI *p_glVertexAttribI2iEXT)(GLuint,GLint,GLint);
        void       (WINE_GLAPI *p_glVertexAttribI2iv)(GLuint,const GLint*);
        void       (WINE_GLAPI *p_glVertexAttribI2ivEXT)(GLuint,const GLint*);
        void       (WINE_GLAPI *p_glVertexAttribI2ui)(GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glVertexAttribI2uiEXT)(GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glVertexAttribI2uiv)(GLuint,const GLuint*);
        void       (WINE_GLAPI *p_glVertexAttribI2uivEXT)(GLuint,const GLuint*);
        void       (WINE_GLAPI *p_glVertexAttribI3i)(GLuint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glVertexAttribI3iEXT)(GLuint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glVertexAttribI3iv)(GLuint,const GLint*);
        void       (WINE_GLAPI *p_glVertexAttribI3ivEXT)(GLuint,const GLint*);
        void       (WINE_GLAPI *p_glVertexAttribI3ui)(GLuint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glVertexAttribI3uiEXT)(GLuint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glVertexAttribI3uiv)(GLuint,const GLuint*);
        void       (WINE_GLAPI *p_glVertexAttribI3uivEXT)(GLuint,const GLuint*);
        void       (WINE_GLAPI *p_glVertexAttribI4bv)(GLuint,const GLbyte*);
        void       (WINE_GLAPI *p_glVertexAttribI4bvEXT)(GLuint,const GLbyte*);
        void       (WINE_GLAPI *p_glVertexAttribI4i)(GLuint,GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glVertexAttribI4iEXT)(GLuint,GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glVertexAttribI4iv)(GLuint,const GLint*);
        void       (WINE_GLAPI *p_glVertexAttribI4ivEXT)(GLuint,const GLint*);
        void       (WINE_GLAPI *p_glVertexAttribI4sv)(GLuint,const GLshort*);
        void       (WINE_GLAPI *p_glVertexAttribI4svEXT)(GLuint,const GLshort*);
        void       (WINE_GLAPI *p_glVertexAttribI4ubv)(GLuint,const GLubyte*);
        void       (WINE_GLAPI *p_glVertexAttribI4ubvEXT)(GLuint,const GLubyte*);
        void       (WINE_GLAPI *p_glVertexAttribI4ui)(GLuint,GLuint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glVertexAttribI4uiEXT)(GLuint,GLuint,GLuint,GLuint,GLuint);
        void       (WINE_GLAPI *p_glVertexAttribI4uiv)(GLuint,const GLuint*);
        void       (WINE_GLAPI *p_glVertexAttribI4uivEXT)(GLuint,const GLuint*);
        void       (WINE_GLAPI *p_glVertexAttribI4usv)(GLuint,const GLushort*);
        void       (WINE_GLAPI *p_glVertexAttribI4usvEXT)(GLuint,const GLushort*);
        void       (WINE_GLAPI *p_glVertexAttribIFormat)(GLuint,GLint,GLenum,GLuint);
        void       (WINE_GLAPI *p_glVertexAttribIFormatNV)(GLuint,GLint,GLenum,GLsizei);
        void       (WINE_GLAPI *p_glVertexAttribIPointer)(GLuint,GLint,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glVertexAttribIPointerEXT)(GLuint,GLint,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glVertexAttribL1d)(GLuint,GLdouble);
        void       (WINE_GLAPI *p_glVertexAttribL1dEXT)(GLuint,GLdouble);
        void       (WINE_GLAPI *p_glVertexAttribL1dv)(GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexAttribL1dvEXT)(GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexAttribL1i64NV)(GLuint,GLint64EXT);
        void       (WINE_GLAPI *p_glVertexAttribL1i64vNV)(GLuint,const GLint64EXT*);
        void       (WINE_GLAPI *p_glVertexAttribL1ui64ARB)(GLuint,GLuint64EXT);
        void       (WINE_GLAPI *p_glVertexAttribL1ui64NV)(GLuint,GLuint64EXT);
        void       (WINE_GLAPI *p_glVertexAttribL1ui64vARB)(GLuint,const GLuint64EXT*);
        void       (WINE_GLAPI *p_glVertexAttribL1ui64vNV)(GLuint,const GLuint64EXT*);
        void       (WINE_GLAPI *p_glVertexAttribL2d)(GLuint,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glVertexAttribL2dEXT)(GLuint,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glVertexAttribL2dv)(GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexAttribL2dvEXT)(GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexAttribL2i64NV)(GLuint,GLint64EXT,GLint64EXT);
        void       (WINE_GLAPI *p_glVertexAttribL2i64vNV)(GLuint,const GLint64EXT*);
        void       (WINE_GLAPI *p_glVertexAttribL2ui64NV)(GLuint,GLuint64EXT,GLuint64EXT);
        void       (WINE_GLAPI *p_glVertexAttribL2ui64vNV)(GLuint,const GLuint64EXT*);
        void       (WINE_GLAPI *p_glVertexAttribL3d)(GLuint,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glVertexAttribL3dEXT)(GLuint,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glVertexAttribL3dv)(GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexAttribL3dvEXT)(GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexAttribL3i64NV)(GLuint,GLint64EXT,GLint64EXT,GLint64EXT);
        void       (WINE_GLAPI *p_glVertexAttribL3i64vNV)(GLuint,const GLint64EXT*);
        void       (WINE_GLAPI *p_glVertexAttribL3ui64NV)(GLuint,GLuint64EXT,GLuint64EXT,GLuint64EXT);
        void       (WINE_GLAPI *p_glVertexAttribL3ui64vNV)(GLuint,const GLuint64EXT*);
        void       (WINE_GLAPI *p_glVertexAttribL4d)(GLuint,GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glVertexAttribL4dEXT)(GLuint,GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glVertexAttribL4dv)(GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexAttribL4dvEXT)(GLuint,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexAttribL4i64NV)(GLuint,GLint64EXT,GLint64EXT,GLint64EXT,GLint64EXT);
        void       (WINE_GLAPI *p_glVertexAttribL4i64vNV)(GLuint,const GLint64EXT*);
        void       (WINE_GLAPI *p_glVertexAttribL4ui64NV)(GLuint,GLuint64EXT,GLuint64EXT,GLuint64EXT,GLuint64EXT);
        void       (WINE_GLAPI *p_glVertexAttribL4ui64vNV)(GLuint,const GLuint64EXT*);
        void       (WINE_GLAPI *p_glVertexAttribLFormat)(GLuint,GLint,GLenum,GLuint);
        void       (WINE_GLAPI *p_glVertexAttribLFormatNV)(GLuint,GLint,GLenum,GLsizei);
        void       (WINE_GLAPI *p_glVertexAttribLPointer)(GLuint,GLint,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glVertexAttribLPointerEXT)(GLuint,GLint,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glVertexAttribP1ui)(GLuint,GLenum,GLboolean,GLuint);
        void       (WINE_GLAPI *p_glVertexAttribP1uiv)(GLuint,GLenum,GLboolean,const GLuint*);
        void       (WINE_GLAPI *p_glVertexAttribP2ui)(GLuint,GLenum,GLboolean,GLuint);
        void       (WINE_GLAPI *p_glVertexAttribP2uiv)(GLuint,GLenum,GLboolean,const GLuint*);
        void       (WINE_GLAPI *p_glVertexAttribP3ui)(GLuint,GLenum,GLboolean,GLuint);
        void       (WINE_GLAPI *p_glVertexAttribP3uiv)(GLuint,GLenum,GLboolean,const GLuint*);
        void       (WINE_GLAPI *p_glVertexAttribP4ui)(GLuint,GLenum,GLboolean,GLuint);
        void       (WINE_GLAPI *p_glVertexAttribP4uiv)(GLuint,GLenum,GLboolean,const GLuint*);
        void       (WINE_GLAPI *p_glVertexAttribParameteriAMD)(GLuint,GLenum,GLint);
        void       (WINE_GLAPI *p_glVertexAttribPointer)(GLuint,GLint,GLenum,GLboolean,GLsizei,const void*);
        void       (WINE_GLAPI *p_glVertexAttribPointerARB)(GLuint,GLint,GLenum,GLboolean,GLsizei,const void*);
        void       (WINE_GLAPI *p_glVertexAttribPointerNV)(GLuint,GLint,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glVertexAttribs1dvNV)(GLuint,GLsizei,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexAttribs1fvNV)(GLuint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glVertexAttribs1hvNV)(GLuint,GLsizei,const GLhalfNV*);
        void       (WINE_GLAPI *p_glVertexAttribs1svNV)(GLuint,GLsizei,const GLshort*);
        void       (WINE_GLAPI *p_glVertexAttribs2dvNV)(GLuint,GLsizei,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexAttribs2fvNV)(GLuint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glVertexAttribs2hvNV)(GLuint,GLsizei,const GLhalfNV*);
        void       (WINE_GLAPI *p_glVertexAttribs2svNV)(GLuint,GLsizei,const GLshort*);
        void       (WINE_GLAPI *p_glVertexAttribs3dvNV)(GLuint,GLsizei,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexAttribs3fvNV)(GLuint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glVertexAttribs3hvNV)(GLuint,GLsizei,const GLhalfNV*);
        void       (WINE_GLAPI *p_glVertexAttribs3svNV)(GLuint,GLsizei,const GLshort*);
        void       (WINE_GLAPI *p_glVertexAttribs4dvNV)(GLuint,GLsizei,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexAttribs4fvNV)(GLuint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glVertexAttribs4hvNV)(GLuint,GLsizei,const GLhalfNV*);
        void       (WINE_GLAPI *p_glVertexAttribs4svNV)(GLuint,GLsizei,const GLshort*);
        void       (WINE_GLAPI *p_glVertexAttribs4ubvNV)(GLuint,GLsizei,const GLubyte*);
        void       (WINE_GLAPI *p_glVertexBindingDivisor)(GLuint,GLuint);
        void       (WINE_GLAPI *p_glVertexBlendARB)(GLint);
        void       (WINE_GLAPI *p_glVertexBlendEnvfATI)(GLenum,GLfloat);
        void       (WINE_GLAPI *p_glVertexBlendEnviATI)(GLenum,GLint);
        void       (WINE_GLAPI *p_glVertexFormatNV)(GLint,GLenum,GLsizei);
        void       (WINE_GLAPI *p_glVertexP2ui)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glVertexP2uiv)(GLenum,const GLuint*);
        void       (WINE_GLAPI *p_glVertexP3ui)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glVertexP3uiv)(GLenum,const GLuint*);
        void       (WINE_GLAPI *p_glVertexP4ui)(GLenum,GLuint);
        void       (WINE_GLAPI *p_glVertexP4uiv)(GLenum,const GLuint*);
        void       (WINE_GLAPI *p_glVertexPointerEXT)(GLint,GLenum,GLsizei,GLsizei,const void*);
        void       (WINE_GLAPI *p_glVertexPointerListIBM)(GLint,GLenum,GLint,const void**,GLint);
        void       (WINE_GLAPI *p_glVertexPointervINTEL)(GLint,GLenum,const void**);
        void       (WINE_GLAPI *p_glVertexStream1dATI)(GLenum,GLdouble);
        void       (WINE_GLAPI *p_glVertexStream1dvATI)(GLenum,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexStream1fATI)(GLenum,GLfloat);
        void       (WINE_GLAPI *p_glVertexStream1fvATI)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glVertexStream1iATI)(GLenum,GLint);
        void       (WINE_GLAPI *p_glVertexStream1ivATI)(GLenum,const GLint*);
        void       (WINE_GLAPI *p_glVertexStream1sATI)(GLenum,GLshort);
        void       (WINE_GLAPI *p_glVertexStream1svATI)(GLenum,const GLshort*);
        void       (WINE_GLAPI *p_glVertexStream2dATI)(GLenum,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glVertexStream2dvATI)(GLenum,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexStream2fATI)(GLenum,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glVertexStream2fvATI)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glVertexStream2iATI)(GLenum,GLint,GLint);
        void       (WINE_GLAPI *p_glVertexStream2ivATI)(GLenum,const GLint*);
        void       (WINE_GLAPI *p_glVertexStream2sATI)(GLenum,GLshort,GLshort);
        void       (WINE_GLAPI *p_glVertexStream2svATI)(GLenum,const GLshort*);
        void       (WINE_GLAPI *p_glVertexStream3dATI)(GLenum,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glVertexStream3dvATI)(GLenum,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexStream3fATI)(GLenum,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glVertexStream3fvATI)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glVertexStream3iATI)(GLenum,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glVertexStream3ivATI)(GLenum,const GLint*);
        void       (WINE_GLAPI *p_glVertexStream3sATI)(GLenum,GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glVertexStream3svATI)(GLenum,const GLshort*);
        void       (WINE_GLAPI *p_glVertexStream4dATI)(GLenum,GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glVertexStream4dvATI)(GLenum,const GLdouble*);
        void       (WINE_GLAPI *p_glVertexStream4fATI)(GLenum,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glVertexStream4fvATI)(GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glVertexStream4iATI)(GLenum,GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glVertexStream4ivATI)(GLenum,const GLint*);
        void       (WINE_GLAPI *p_glVertexStream4sATI)(GLenum,GLshort,GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glVertexStream4svATI)(GLenum,const GLshort*);
        void       (WINE_GLAPI *p_glVertexWeightPointerEXT)(GLint,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glVertexWeightfEXT)(GLfloat);
        void       (WINE_GLAPI *p_glVertexWeightfvEXT)(const GLfloat*);
        void       (WINE_GLAPI *p_glVertexWeighthNV)(GLhalfNV);
        void       (WINE_GLAPI *p_glVertexWeighthvNV)(const GLhalfNV*);
        GLenum     (WINE_GLAPI *p_glVideoCaptureNV)(GLuint,GLuint*,GLuint64EXT*);
        void       (WINE_GLAPI *p_glVideoCaptureStreamParameterdvNV)(GLuint,GLuint,GLenum,const GLdouble*);
        void       (WINE_GLAPI *p_glVideoCaptureStreamParameterfvNV)(GLuint,GLuint,GLenum,const GLfloat*);
        void       (WINE_GLAPI *p_glVideoCaptureStreamParameterivNV)(GLuint,GLuint,GLenum,const GLint*);
        void       (WINE_GLAPI *p_glViewportArrayv)(GLuint,GLsizei,const GLfloat*);
        void       (WINE_GLAPI *p_glViewportIndexedf)(GLuint,GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glViewportIndexedfv)(GLuint,const GLfloat*);
        void       (WINE_GLAPI *p_glWaitSync)(GLsync,GLbitfield,GLuint64);
        void       (WINE_GLAPI *p_glWeightPathsNV)(GLuint,GLsizei,const GLuint*,const GLfloat*);
        void       (WINE_GLAPI *p_glWeightPointerARB)(GLint,GLenum,GLsizei,const void*);
        void       (WINE_GLAPI *p_glWeightbvARB)(GLint,const GLbyte*);
        void       (WINE_GLAPI *p_glWeightdvARB)(GLint,const GLdouble*);
        void       (WINE_GLAPI *p_glWeightfvARB)(GLint,const GLfloat*);
        void       (WINE_GLAPI *p_glWeightivARB)(GLint,const GLint*);
        void       (WINE_GLAPI *p_glWeightsvARB)(GLint,const GLshort*);
        void       (WINE_GLAPI *p_glWeightubvARB)(GLint,const GLubyte*);
        void       (WINE_GLAPI *p_glWeightuivARB)(GLint,const GLuint*);
        void       (WINE_GLAPI *p_glWeightusvARB)(GLint,const GLushort*);
        void       (WINE_GLAPI *p_glWindowPos2d)(GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glWindowPos2dARB)(GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glWindowPos2dMESA)(GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glWindowPos2dv)(const GLdouble*);
        void       (WINE_GLAPI *p_glWindowPos2dvARB)(const GLdouble*);
        void       (WINE_GLAPI *p_glWindowPos2dvMESA)(const GLdouble*);
        void       (WINE_GLAPI *p_glWindowPos2f)(GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glWindowPos2fARB)(GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glWindowPos2fMESA)(GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glWindowPos2fv)(const GLfloat*);
        void       (WINE_GLAPI *p_glWindowPos2fvARB)(const GLfloat*);
        void       (WINE_GLAPI *p_glWindowPos2fvMESA)(const GLfloat*);
        void       (WINE_GLAPI *p_glWindowPos2i)(GLint,GLint);
        void       (WINE_GLAPI *p_glWindowPos2iARB)(GLint,GLint);
        void       (WINE_GLAPI *p_glWindowPos2iMESA)(GLint,GLint);
        void       (WINE_GLAPI *p_glWindowPos2iv)(const GLint*);
        void       (WINE_GLAPI *p_glWindowPos2ivARB)(const GLint*);
        void       (WINE_GLAPI *p_glWindowPos2ivMESA)(const GLint*);
        void       (WINE_GLAPI *p_glWindowPos2s)(GLshort,GLshort);
        void       (WINE_GLAPI *p_glWindowPos2sARB)(GLshort,GLshort);
        void       (WINE_GLAPI *p_glWindowPos2sMESA)(GLshort,GLshort);
        void       (WINE_GLAPI *p_glWindowPos2sv)(const GLshort*);
        void       (WINE_GLAPI *p_glWindowPos2svARB)(const GLshort*);
        void       (WINE_GLAPI *p_glWindowPos2svMESA)(const GLshort*);
        void       (WINE_GLAPI *p_glWindowPos3d)(GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glWindowPos3dARB)(GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glWindowPos3dMESA)(GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glWindowPos3dv)(const GLdouble*);
        void       (WINE_GLAPI *p_glWindowPos3dvARB)(const GLdouble*);
        void       (WINE_GLAPI *p_glWindowPos3dvMESA)(const GLdouble*);
        void       (WINE_GLAPI *p_glWindowPos3f)(GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glWindowPos3fARB)(GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glWindowPos3fMESA)(GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glWindowPos3fv)(const GLfloat*);
        void       (WINE_GLAPI *p_glWindowPos3fvARB)(const GLfloat*);
        void       (WINE_GLAPI *p_glWindowPos3fvMESA)(const GLfloat*);
        void       (WINE_GLAPI *p_glWindowPos3i)(GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glWindowPos3iARB)(GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glWindowPos3iMESA)(GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glWindowPos3iv)(const GLint*);
        void       (WINE_GLAPI *p_glWindowPos3ivARB)(const GLint*);
        void       (WINE_GLAPI *p_glWindowPos3ivMESA)(const GLint*);
        void       (WINE_GLAPI *p_glWindowPos3s)(GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glWindowPos3sARB)(GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glWindowPos3sMESA)(GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glWindowPos3sv)(const GLshort*);
        void       (WINE_GLAPI *p_glWindowPos3svARB)(const GLshort*);
        void       (WINE_GLAPI *p_glWindowPos3svMESA)(const GLshort*);
        void       (WINE_GLAPI *p_glWindowPos4dMESA)(GLdouble,GLdouble,GLdouble,GLdouble);
        void       (WINE_GLAPI *p_glWindowPos4dvMESA)(const GLdouble*);
        void       (WINE_GLAPI *p_glWindowPos4fMESA)(GLfloat,GLfloat,GLfloat,GLfloat);
        void       (WINE_GLAPI *p_glWindowPos4fvMESA)(const GLfloat*);
        void       (WINE_GLAPI *p_glWindowPos4iMESA)(GLint,GLint,GLint,GLint);
        void       (WINE_GLAPI *p_glWindowPos4ivMESA)(const GLint*);
        void       (WINE_GLAPI *p_glWindowPos4sMESA)(GLshort,GLshort,GLshort,GLshort);
        void       (WINE_GLAPI *p_glWindowPos4svMESA)(const GLshort*);
        void       (WINE_GLAPI *p_glWriteMaskEXT)(GLuint,GLuint,GLenum,GLenum,GLenum,GLenum);
        void*      (WINE_GLAPI *p_wglAllocateMemoryNV)(GLsizei,GLfloat,GLfloat,GLfloat);
        BOOL       (WINE_GLAPI *p_wglBindTexImageARB)(struct wgl_pbuffer *,int);
        BOOL       (WINE_GLAPI *p_wglChoosePixelFormatARB)(HDC,const int*,const FLOAT*,UINT,int*,UINT*);
        struct wgl_context * (WINE_GLAPI *p_wglCreateContextAttribsARB)(HDC,struct wgl_context *,const int*);
        struct wgl_pbuffer * (WINE_GLAPI *p_wglCreatePbufferARB)(HDC,int,int,int,const int*);
        BOOL       (WINE_GLAPI *p_wglDestroyPbufferARB)(struct wgl_pbuffer *);
        void       (WINE_GLAPI *p_wglFreeMemoryNV)(void*);
        HDC        (WINE_GLAPI *p_wglGetCurrentReadDCARB)(void);
        const char* (WINE_GLAPI *p_wglGetExtensionsStringARB)(HDC);
        const char* (WINE_GLAPI *p_wglGetExtensionsStringEXT)(void);
        HDC        (WINE_GLAPI *p_wglGetPbufferDCARB)(struct wgl_pbuffer *);
        BOOL       (WINE_GLAPI *p_wglGetPixelFormatAttribfvARB)(HDC,int,int,UINT,const int*,FLOAT*);
        BOOL       (WINE_GLAPI *p_wglGetPixelFormatAttribivARB)(HDC,int,int,UINT,const int*,int*);
        int        (WINE_GLAPI *p_wglGetSwapIntervalEXT)(void);
        BOOL       (WINE_GLAPI *p_wglMakeContextCurrentARB)(HDC,HDC,struct wgl_context *);
        BOOL       (WINE_GLAPI *p_wglQueryPbufferARB)(struct wgl_pbuffer *,int,int*);
        int        (WINE_GLAPI *p_wglReleasePbufferDCARB)(struct wgl_pbuffer *,HDC);
        BOOL       (WINE_GLAPI *p_wglReleaseTexImageARB)(struct wgl_pbuffer *,int);
        BOOL       (WINE_GLAPI *p_wglSetPbufferAttribARB)(struct wgl_pbuffer *,const int*);
        BOOL       (WINE_GLAPI *p_wglSetPixelFormatWINE)(HDC,int);
        BOOL       (WINE_GLAPI *p_wglSwapIntervalEXT)(int);
        BOOL       (WINE_GLAPI *p_wglGetPCIInfoWINE)(unsigned int *, unsigned int *);
        BOOL       (WINE_GLAPI *p_wglGetMemoryInfoWINE)(unsigned int *);
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

extern struct opengl_funcs * CDECL __wine_get_wgl_driver( HDC hdc, UINT version );
extern BOOL CDECL __wine_set_pixel_format( HWND hwnd, int format );

#endif /* __WINE_WGL_DRIVER_H */
