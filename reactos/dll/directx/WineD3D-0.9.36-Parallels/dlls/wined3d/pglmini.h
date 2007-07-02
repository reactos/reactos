#ifndef __WINE_PGLMINI_H
#define __WINE_PGLMINI_H

BOOL pglInit();
VOID pglFini();


int WINAPI pglDescribePixelFormat(
    HDC hdc, int index, UINT size, LPPIXELFORMATDESCRIPTOR format);
BOOL WINAPI pglSetPixelFormat(
    HDC hdc, int index, const LPPIXELFORMATDESCRIPTOR format);
HGLRC WINAPI pglCreateContext(HDC hdc);
BOOL WINAPI pglDeleteContext(HGLRC hgl);
HGLRC WINAPI pglGetCurrentContext(VOID);
HDC WINAPI pglGetCurrentDC(VOID);
BOOL WINAPI pglMakeCurrent(HDC hdc, HGLRC hgl);
PROC WINAPI pglGetProcAddress(LPCSTR name);
BOOL WINAPI pglSwapBuffers(HDC hdc);

#define wglDescribePixelFormat pglDescribePixelFormat
#define wglSetPixelFormat pglSetPixelFormat
#define wglCreateContext pglCreateContext
#define wglDeleteContext pglDeleteContext
#define wglGetCurrentContext pglGetCurrentContext
#define wglGetCurrentDC pglGetCurrentDC
#define wglMakeCurrent pglMakeCurrent
#define wglGetProcAddress pglGetProcAddress
#define wglSwapBuffers pglSwapBuffers

struct pgl_iat_s {
	void WINE_APIENTRY(*pglAlphaFunc)(GLenum func, GLclampf ref);
	void WINE_APIENTRY(*pglBegin)(GLenum mode);
	void WINE_APIENTRY(*pglBindTexture)(GLenum target, GLuint texture);
	void WINE_APIENTRY(*pglBlendFunc)(GLenum sfactor, GLenum dfactor);
	void WINE_APIENTRY(*pglClear)(GLbitfield mask);
	void WINE_APIENTRY(*pglClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
	void WINE_APIENTRY(*pglClearDepth)(GLclampd depth);
	void WINE_APIENTRY(*pglClearIndex)(GLfloat c);
	void WINE_APIENTRY(*pglClearStencil)(GLint s);
	void WINE_APIENTRY(*pglClipPlane)(GLenum plane, const GLdouble *equation);
	void WINE_APIENTRY(*pglColor3d)(GLdouble red, GLdouble green, GLdouble blue);
	void WINE_APIENTRY(*pglColor3f)(GLfloat red, GLfloat green, GLfloat blue);
	void WINE_APIENTRY(*pglColor4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	void WINE_APIENTRY(*pglColor4ub)(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
	void WINE_APIENTRY(*pglColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
	void WINE_APIENTRY(*pglColorMaterial)(GLenum face, GLenum mode);
	void WINE_APIENTRY(*pglColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void WINE_APIENTRY(*pglCopyTexImage2D)(GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
	void WINE_APIENTRY(*pglCopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	void WINE_APIENTRY(*pglCullFace)(GLenum mode);
	void WINE_APIENTRY(*pglDeleteTextures)(GLsizei n, const GLuint *textures);
	void WINE_APIENTRY(*pglDepthFunc)(GLenum func);
	void WINE_APIENTRY(*pglDepthMask)(GLboolean flag);
	void WINE_APIENTRY(*pglDepthRange)(GLclampd zNear, GLclampd zFar);
	void WINE_APIENTRY(*pglDisable)(GLenum cap);
	void WINE_APIENTRY(*pglDisableClientState)(GLenum array);
	void WINE_APIENTRY(*pglDrawArrays)(GLenum mode, GLint first, GLsizei count);
	void WINE_APIENTRY(*pglDrawBuffer)(GLenum mode);
	void WINE_APIENTRY(*pglDrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
	void WINE_APIENTRY(*pglDrawPixels)(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
	void WINE_APIENTRY(*pglEnable)(GLenum cap);
	void WINE_APIENTRY(*pglEnableClientState)(GLenum array);
	void WINE_APIENTRY(*pglEnd)(void);
	void WINE_APIENTRY(*pglFlush)(void);
	void WINE_APIENTRY(*pglFogf)(GLenum pname, GLfloat param);
	void WINE_APIENTRY(*pglFogfv)(GLenum pname, const GLfloat *params);
	void WINE_APIENTRY(*pglFogi)(GLenum pname, GLint param);
	void WINE_APIENTRY(*pglFrontFace)(GLenum mode);
	void WINE_APIENTRY(*pglGenTextures)(GLsizei n, GLuint *textures);
	GLenum WINE_APIENTRY(*pglGetError)(void);
	void WINE_APIENTRY(*pglGetFloatv)(GLenum pname, GLfloat *params);
	void WINE_APIENTRY(*pglGetIntegerv)(GLenum pname, GLint *params);
	const GLubyte * WINE_APIENTRY(*pglGetString)(GLenum name);
	void WINE_APIENTRY(*pglGetTexImage)(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels);
	void WINE_APIENTRY(*pglHint)(GLenum target, GLenum mode);
	void WINE_APIENTRY(*pglLightModelfv)(GLenum pname, const GLfloat *params);
	void WINE_APIENTRY(*pglLightModeli)(GLenum pname, GLint param);
	void WINE_APIENTRY(*pglLightf)(GLenum light, GLenum pname, GLfloat param);
	void WINE_APIENTRY(*pglLightfv)(GLenum light, GLenum pname, const GLfloat *params);
	void WINE_APIENTRY(*pglLineStipple)(GLint factor, GLushort pattern);
	void WINE_APIENTRY(*pglLoadIdentity)(void);
	void WINE_APIENTRY(*pglLoadMatrixf)(const GLfloat *m);
	void WINE_APIENTRY(*pglMaterialf)(GLenum face, GLenum pname, GLfloat param);
	void WINE_APIENTRY(*pglMaterialfv)(GLenum face, GLenum pname, const GLfloat *params);
	void WINE_APIENTRY(*pglMatrixMode)(GLenum mode);
	void WINE_APIENTRY(*pglMultMatrixf)(const GLfloat *m);
	void WINE_APIENTRY(*pglNormal3f)(GLfloat nx, GLfloat ny, GLfloat nz);
	void WINE_APIENTRY(*pglNormalPointer)(GLenum type, GLsizei stride, const GLvoid *pointer);
	void WINE_APIENTRY(*pglOrtho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
	void WINE_APIENTRY(*pglPixelStorei)(GLenum pname, GLint param);
	void WINE_APIENTRY(*pglPixelZoom)(GLfloat xfactor, GLfloat yfactor);
	void WINE_APIENTRY(*pglPointSize)(GLfloat size);
	void WINE_APIENTRY(*pglPolygonMode)(GLenum face, GLenum mode);
	void WINE_APIENTRY(*pglPolygonOffset)(GLfloat factor, GLfloat units);
	void WINE_APIENTRY(*pglPopAttrib)(void);
	void WINE_APIENTRY(*pglPopMatrix)(void);
	void WINE_APIENTRY(*pglPrioritizeTextures)(GLsizei n, const GLuint *textures, const GLclampf *priorities);
	void WINE_APIENTRY(*pglPushAttrib)(GLbitfield mask);
	void WINE_APIENTRY(*pglPushMatrix)(void);
	void WINE_APIENTRY(*pglRasterPos3i)(GLint x, GLint y, GLint z);
	void WINE_APIENTRY(*pglRasterPos3iv)(const GLint *v);
	void WINE_APIENTRY(*pglReadBuffer)(GLenum mode);
	void WINE_APIENTRY(*pglReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
	void WINE_APIENTRY(*pglScissor)(GLint x, GLint y, GLsizei width, GLsizei height);
	void WINE_APIENTRY(*pglShadeModel)(GLenum mode);
	void WINE_APIENTRY(*pglStencilFunc)(GLenum func, GLint ref, GLuint mask);
	void WINE_APIENTRY(*pglStencilMask)(GLuint mask);
	void WINE_APIENTRY(*pglStencilOp)(GLenum fail, GLenum zfail, GLenum zpass);
	void WINE_APIENTRY(*pglTexCoord1f)(GLfloat s);
	void WINE_APIENTRY(*pglTexCoord2f)(GLfloat s, GLfloat t);
	void WINE_APIENTRY(*pglTexCoord3f)(GLfloat s, GLfloat t, GLfloat r);
	void WINE_APIENTRY(*pglTexCoord3iv)(const GLint *v);
	void WINE_APIENTRY(*pglTexCoord4f)(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
	void WINE_APIENTRY(*pglTexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void WINE_APIENTRY(*pglTexEnvf)(GLenum target, GLenum pname, GLfloat param);
	void WINE_APIENTRY(*pglTexEnvfv)(GLenum target, GLenum pname, const GLfloat *params);
	void WINE_APIENTRY(*pglTexEnvi)(GLenum target, GLenum pname, GLint param);
	void WINE_APIENTRY(*pglTexGenfv)(GLenum coord, GLenum pname, const GLfloat *params);
	void WINE_APIENTRY(*pglTexGeni)(GLenum coord, GLenum pname, GLint param);
	void WINE_APIENTRY(*pglTexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
	void WINE_APIENTRY(*pglTexParameterfv)(GLenum target, GLenum pname, const GLfloat *params);
	void WINE_APIENTRY(*pglTexParameteri)(GLenum target, GLenum pname, GLint param);
	void WINE_APIENTRY(*pglTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
	void WINE_APIENTRY(*pglTranslatef)(GLfloat x, GLfloat y, GLfloat z);
	void WINE_APIENTRY(*pglVertex2f)(GLfloat x, GLfloat y);
	void WINE_APIENTRY(*pglVertex2i)(GLint x, GLint y);
	void WINE_APIENTRY(*pglVertex3d)(GLdouble x, GLdouble y, GLdouble z);
	void WINE_APIENTRY(*pglVertex3f)(GLfloat x, GLfloat y, GLfloat z);
	void WINE_APIENTRY(*pglVertex4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	void WINE_APIENTRY(*pglVertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void WINE_APIENTRY(*pglViewport)(GLint x, GLint y, GLsizei width, GLsizei height);
};
extern struct pgl_iat_s pgl_iat;

#define glAlphaFunc pgl_iat.pglAlphaFunc
#define glBegin pgl_iat.pglBegin
#define glBindTexture pgl_iat.pglBindTexture
#define glBlendFunc pgl_iat.pglBlendFunc
#define glClear pgl_iat.pglClear
#define glClearColor pgl_iat.pglClearColor
#define glClearDepth pgl_iat.pglClearDepth
#define glClearIndex pgl_iat.pglClearIndex
#define glClearStencil pgl_iat.pglClearStencil
#define glClipPlane pgl_iat.pglClipPlane
#define glColor3d pgl_iat.pglColor3d
#define glColor3f pgl_iat.pglColor3f
#define glColor4f pgl_iat.pglColor4f
#define glColor4ub pgl_iat.pglColor4ub
#define glColorMask pgl_iat.pglColorMask
#define glColorMaterial pgl_iat.pglColorMaterial
#define glColorPointer pgl_iat.pglColorPointer
#define glCopyTexImage2D pgl_iat.pglCopyTexImage2D
#define glCopyTexSubImage2D pgl_iat.pglCopyTexSubImage2D
#define glCullFace pgl_iat.pglCullFace
#define glDeleteTextures pgl_iat.pglDeleteTextures
#define glDepthFunc pgl_iat.pglDepthFunc
#define glDepthMask pgl_iat.pglDepthMask
#define glDepthRange pgl_iat.pglDepthRange
#define glDisable pgl_iat.pglDisable
#define glDisableClientState pgl_iat.pglDisableClientState
#define glDrawArrays pgl_iat.pglDrawArrays
#define glDrawBuffer pgl_iat.pglDrawBuffer
#define glDrawElements pgl_iat.pglDrawElements
#define glDrawPixels pgl_iat.pglDrawPixels
#define glEnable pgl_iat.pglEnable
#define glEnableClientState pgl_iat.pglEnableClientState
#define glEnd pgl_iat.pglEnd
#define glFlush pgl_iat.pglFlush
#define glFogf pgl_iat.pglFogf
#define glFogfv pgl_iat.pglFogfv
#define glFogi pgl_iat.pglFogi
#define glFrontFace pgl_iat.pglFrontFace
#define glGenTextures pgl_iat.pglGenTextures
#define glGetError pgl_iat.pglGetError
#define glGetFloatv pgl_iat.pglGetFloatv
#define glGetIntegerv pgl_iat.pglGetIntegerv
#define glGetString pgl_iat.pglGetString
#define glGetTexImage pgl_iat.pglGetTexImage
#define glHint pgl_iat.pglHint
#define glLightModelfv pgl_iat.pglLightModelfv
#define glLightModeli pgl_iat.pglLightModeli
#define glLightf pgl_iat.pglLightf
#define glLightfv pgl_iat.pglLightfv
#define glLineStipple pgl_iat.pglLineStipple
#define glLoadIdentity pgl_iat.pglLoadIdentity
#define glLoadMatrixf pgl_iat.pglLoadMatrixf
#define glMaterialf pgl_iat.pglMaterialf
#define glMaterialfv pgl_iat.pglMaterialfv
#define glMatrixMode pgl_iat.pglMatrixMode
#define glMultMatrixf pgl_iat.pglMultMatrixf
#define glNormal3f pgl_iat.pglNormal3f
#define glNormalPointer pgl_iat.pglNormalPointer
#define glOrtho pgl_iat.pglOrtho
#define glPixelStorei pgl_iat.pglPixelStorei
#define glPixelZoom pgl_iat.pglPixelZoom
#define glPointSize pgl_iat.pglPointSize
#define glPolygonMode pgl_iat.pglPolygonMode
#define glPolygonOffset pgl_iat.pglPolygonOffset
#define glPopAttrib pgl_iat.pglPopAttrib
#define glPopMatrix pgl_iat.pglPopMatrix
#define glPrioritizeTextures pgl_iat.pglPrioritizeTextures
#define glPushAttrib pgl_iat.pglPushAttrib
#define glPushMatrix pgl_iat.pglPushMatrix
#define glRasterPos3i pgl_iat.pglRasterPos3i
#define glRasterPos3iv pgl_iat.pglRasterPos3iv
#define glReadBuffer pgl_iat.pglReadBuffer
#define glReadPixels pgl_iat.pglReadPixels
#define glScissor pgl_iat.pglScissor
#define glShadeModel pgl_iat.pglShadeModel
#define glStencilFunc pgl_iat.pglStencilFunc
#define glStencilMask pgl_iat.pglStencilMask
#define glStencilOp pgl_iat.pglStencilOp
#define glTexCoord1f pgl_iat.pglTexCoord1f
#define glTexCoord2f pgl_iat.pglTexCoord2f
#define glTexCoord3f pgl_iat.pglTexCoord3f
#define glTexCoord3iv pgl_iat.pglTexCoord3iv
#define glTexCoord4f pgl_iat.pglTexCoord4f
#define glTexCoordPointer pgl_iat.pglTexCoordPointer
#define	glTexEnvf pgl_iat.pglTexEnvf
#define	glTexEnvfv pgl_iat.pglTexEnvfv
#define	glTexEnvi pgl_iat.pglTexEnvi
#define glTexGenfv pgl_iat.pglTexGenfv
#define glTexGeni pgl_iat.pglTexGeni
#define glTexImage2D pgl_iat.pglTexImage2D
#define glTexParameterfv pgl_iat.pglTexParameterfv
#define glTexParameteri pgl_iat.pglTexParameteri
#define glTexSubImage2D pgl_iat.pglTexSubImage2D
#define glTranslatef pgl_iat.pglTranslatef
#define glVertex2f pgl_iat.pglVertex2f
#define glVertex2i pgl_iat.pglVertex2i
#define glVertex3d pgl_iat.pglVertex3d
#define glVertex3f pgl_iat.pglVertex3f
#define glVertex4f pgl_iat.pglVertex4f
#define glVertexPointer pgl_iat.pglVertexPointer
#define glViewport pgl_iat.pglViewport

#endif
