/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/opengl32/glfuncs.h
 * PURPOSE:              OpenGL32 lib, GLFUNCS_MACRO
 * PROGRAMMER:           gen_glfuncs_macro.sh
 * UPDATE HISTORY:
 *            !!! AUTOMATICALLY CREATED FROM gl.h !!!
 */

/* To use this macro define a macro X(name, ret, args).
 * It gets called for every glXXX function. For glVertex3f name would be "glVertex3f",
 * ret would be "void" and args would be "(GLfloat x, GLfloat y, GLfloat z)".
 * Don't forget to undefine X ;-)
 */

#define GLFUNCS_MACRO \
	X(glAccum, void, (GLenum op, GLfloat value)) \
	X(glActiveTexture, void, (GLenum texture)) \
	X(glAddSwapHintRectWIN, void, (GLint x, GLint y, GLsizei width, GLsizei height)) \
	X(glAlphaFunc, void, (GLenum func, GLclampf ref)) \
	X(glAreTexturesResident, GLboolean, (GLsizei n, const GLuint *textures, GLboolean *residences)) \
	X(glArrayElement, void, (GLint i)) \
	X(glBegin, void, (GLenum mode)) \
	X(glBindTexture, void, (GLenum target, GLuint texture)) \
	X(glBitmap, void, (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)) \
	X(glBlendColor, void, (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)) \
	X(glBlendEquation, void, (GLenum mode)) \
	X(glBlendFunc, void, (GLenum sfactor, GLenum dfactor)) \
	X(glBlendFuncSeparate, void, (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)) \
	X(glCallList, void, (GLuint list)) \
	X(glCallLists, void, (GLsizei n, GLenum type, const GLvoid *lists)) \
	X(glClear, void, (GLbitfield mask)) \
	X(glClearAccum, void, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)) \
	X(glClearColor, void, (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)) \
	X(glClearDepth, void, (GLclampd depth)) \
	X(glClearIndex, void, (GLfloat c)) \
	X(glClearStencil, void, (GLint s)) \
	X(glClientActiveTexture, void, (GLenum texture)) \
	X(glClipPlane, void, (GLenum plane, const GLdouble *equation)) \
	X(glColor3b, void, (GLbyte red, GLbyte green, GLbyte blue)) \
	X(glColor3bv, void, (const GLbyte *v)) \
	X(glColor3d, void, (GLdouble red, GLdouble green, GLdouble blue)) \
	X(glColor3dv, void, (const GLdouble *v)) \
	X(glColor3f, void, (GLfloat red, GLfloat green, GLfloat blue)) \
	X(glColor3fv, void, (const GLfloat *v)) \
	X(glColor3i, void, (GLint red, GLint green, GLint blue)) \
	X(glColor3iv, void, (const GLint *v)) \
	X(glColor3s, void, (GLshort red, GLshort green, GLshort blue)) \
	X(glColor3sv, void, (const GLshort *v)) \
	X(glColor3ub, void, (GLubyte red, GLubyte green, GLubyte blue)) \
	X(glColor3ubv, void, (const GLubyte *v)) \
	X(glColor3ui, void, (GLuint red, GLuint green, GLuint blue)) \
	X(glColor3uiv, void, (const GLuint *v)) \
	X(glColor3us, void, (GLushort red, GLushort green, GLushort blue)) \
	X(glColor3usv, void, (const GLushort *v)) \
	X(glColor4b, void, (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)) \
	X(glColor4bv, void, (const GLbyte *v)) \
	X(glColor4d, void, (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)) \
	X(glColor4dv, void, (const GLdouble *v)) \
	X(glColor4f, void, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)) \
	X(glColor4fv, void, (const GLfloat *v)) \
	X(glColor4i, void, (GLint red, GLint green, GLint blue, GLint alpha)) \
	X(glColor4iv, void, (const GLint *v)) \
	X(glColor4s, void, (GLshort red, GLshort green, GLshort blue, GLshort alpha)) \
	X(glColor4sv, void, (const GLshort *v)) \
	X(glColor4ub, void, (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)) \
	X(glColor4ubv, void, (const GLubyte *v)) \
	X(glColor4ui, void, (GLuint red, GLuint green, GLuint blue, GLuint alpha)) \
	X(glColor4uiv, void, (const GLuint *v)) \
	X(glColor4us, void, (GLushort red, GLushort green, GLushort blue, GLushort alpha)) \
	X(glColor4usv, void, (const GLushort *v)) \
	X(glColorMask, void, (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)) \
	X(glColorMaterial, void, (GLenum face, GLenum mode)) \
	X(glColorPointer, void, (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)) \
	X(glColorSubTable, void, (GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data)) \
	X(glColorTable, void, (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table)) \
	X(glColorTableParameterfv, void, (GLenum target, GLenum pname, const GLfloat *params)) \
	X(glColorTableParameteriv, void, (GLenum target, GLenum pname, const GLint *params)) \
	X(glCompressedTexImage1D, void, (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data)) \
	X(glCompressedTexImage2D, void, (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data)) \
	X(glCompressedTexImage3D, void, (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data)) \
	X(glCompressedTexSubImage1D, void, (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *data)) \
	X(glCompressedTexSubImage2D, void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data)) \
	X(glCompressedTexSubImage3D, void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data)) \
	X(glConvolutionFilter1D, void, (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image)) \
	X(glConvolutionFilter2D, void, (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image)) \
	X(glConvolutionParameterf, void, (GLenum target, GLenum pname, GLfloat params)) \
	X(glConvolutionParameterfv, void, (GLenum target, GLenum pname, const GLfloat *params)) \
	X(glConvolutionParameteri, void, (GLenum target, GLenum pname, GLint params)) \
	X(glConvolutionParameteriv, void, (GLenum target, GLenum pname, const GLint *params)) \
	X(glCopyColorSubTable, void, (GLenum target, GLsizei start, GLint x, GLint y, GLsizei width)) \
	X(glCopyColorTable, void, (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)) \
	X(glCopyConvolutionFilter1D, void, (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)) \
	X(glCopyConvolutionFilter2D, void, (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height)) \
	X(glCopyPixels, void, (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)) \
	X(glCopyTexImage1D, void, (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)) \
	X(glCopyTexImage2D, void, (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)) \
	X(glCopyTexSubImage1D, void, (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)) \
	X(glCopyTexSubImage2D, void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)) \
	X(glCopyTexSubImage3D, void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)) \
	X(glCullFace, void, (GLenum mode)) \
	X(glDeleteLists, void, (GLuint list, GLsizei range)) \
	X(glDeleteTextures, void, (GLsizei n, const GLuint *textures)) \
	X(glDepthFunc, void, (GLenum func)) \
	X(glDepthMask, void, (GLboolean flag)) \
	X(glDepthRange, void, (GLclampd zNear, GLclampd zFar)) \
	X(glDisable, void, (GLenum cap)) \
	X(glDisableClientState, void, (GLenum array)) \
	X(glDrawArrays, void, (GLenum mode, GLint first, GLsizei count)) \
	X(glDrawBuffer, void, (GLenum mode)) \
	X(glDrawElements, void, (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)) \
	X(glDrawPixels, void, (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)) \
	X(glDrawRangeElements, void, (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices)) \
	X(glEdgeFlag, void, (GLboolean flag)) \
	X(glEdgeFlagPointer, void, (GLsizei stride, const GLboolean *pointer)) \
	X(glEdgeFlagv, void, (const GLboolean *flag)) \
	X(glEnable, void, (GLenum cap)) \
	X(glEnableClientState, void, (GLenum array)) \
	X(glEnd, void, (void)) \
	X(glEndList, void, (void)) \
	X(glEvalCoord1d, void, (GLdouble u)) \
	X(glEvalCoord1dv, void, (const GLdouble *u)) \
	X(glEvalCoord1f, void, (GLfloat u)) \
	X(glEvalCoord1fv, void, (const GLfloat *u)) \
	X(glEvalCoord2d, void, (GLdouble u, GLdouble v)) \
	X(glEvalCoord2dv, void, (const GLdouble *u)) \
	X(glEvalCoord2f, void, (GLfloat u, GLfloat v)) \
	X(glEvalCoord2fv, void, (const GLfloat *u)) \
	X(glEvalMesh1, void, (GLenum mode, GLint i1, GLint i2)) \
	X(glEvalMesh2, void, (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)) \
	X(glEvalPoint1, void, (GLint i)) \
	X(glEvalPoint2, void, (GLint i, GLint j)) \
	X(glFeedbackBuffer, void, (GLsizei size, GLenum type, GLfloat *buffer)) \
	X(glFinish, void, (void)) \
	X(glFlush, void, (void)) \
	X(glFlushHold, GLuint, (void)) \
	X(glFogCoordPointer, void, (GLenum type, GLsizei stride, const GLvoid *pointer)) \
	X(glFogCoordd, void, (GLdouble fog)) \
	X(glFogCoorddv, void, (const GLdouble *fog)) \
	X(glFogCoordf, void, (GLfloat fog)) \
	X(glFogCoordfv, void, (const GLfloat *fog)) \
	X(glFogf, void, (GLenum pname, GLfloat param)) \
	X(glFogfv, void, (GLenum pname, const GLfloat *params)) \
	X(glFogi, void, (GLenum pname, GLint param)) \
	X(glFogiv, void, (GLenum pname, const GLint *params)) \
	X(glFrontFace, void, (GLenum mode)) \
	X(glFrustum, void, (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)) \
	X(glGenLists, GLuint, (GLsizei range)) \
	X(glGenTextures, void, (GLsizei n, GLuint *textures)) \
	X(glGetBooleanv, void, (GLenum pname, GLboolean *params)) \
	X(glGetClipPlane, void, (GLenum plane, GLdouble *equation)) \
	X(glGetColorTable, void, (GLenum target, GLenum format, GLenum type, GLvoid *table)) \
	X(glGetColorTableParameterfv, void, (GLenum target, GLenum pname, GLfloat *params)) \
	X(glGetColorTableParameteriv, void, (GLenum target, GLenum pname, GLint *params)) \
	X(glGetCompressedTexImage, void, (GLenum target, GLint lod, GLvoid *img)) \
	X(glGetConvolutionFilter, void, (GLenum target, GLenum format, GLenum type, GLvoid *image)) \
	X(glGetConvolutionParameterfv, void, (GLenum target, GLenum pname, GLfloat *params)) \
	X(glGetConvolutionParameteriv, void, (GLenum target, GLenum pname, GLint *params)) \
	X(glGetDoublev, void, (GLenum pname, GLdouble *params)) \
	X(glGetError, GLenum, (void)) \
	X(glGetFloatv, void, (GLenum pname, GLfloat *params)) \
	X(glGetHistogram, void, (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values)) \
	X(glGetHistogramParameterfv, void, (GLenum target, GLenum pname, GLfloat *params)) \
	X(glGetHistogramParameteriv, void, (GLenum target, GLenum pname, GLint *params)) \
	X(glGetIntegerv, void, (GLenum pname, GLint *params)) \
	X(glGetLightfv, void, (GLenum light, GLenum pname, GLfloat *params)) \
	X(glGetLightiv, void, (GLenum light, GLenum pname, GLint *params)) \
	X(glGetMapdv, void, (GLenum target, GLenum query, GLdouble *v)) \
	X(glGetMapfv, void, (GLenum target, GLenum query, GLfloat *v)) \
	X(glGetMapiv, void, (GLenum target, GLenum query, GLint *v)) \
	X(glGetMaterialfv, void, (GLenum face, GLenum pname, GLfloat *params)) \
	X(glGetMaterialiv, void, (GLenum face, GLenum pname, GLint *params)) \
	X(glGetMinmax, void, (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values)) \
	X(glGetMinmaxParameterfv, void, (GLenum target, GLenum pname, GLfloat *params)) \
	X(glGetMinmaxParameteriv, void, (GLenum target, GLenum pname, GLint *params)) \
	X(glGetPixelMapfv, void, (GLenum map, GLfloat *values)) \
	X(glGetPixelMapuiv, void, (GLenum map, GLuint *values)) \
	X(glGetPixelMapusv, void, (GLenum map, GLushort *values)) \
	X(glGetPointerv, void, (GLenum pname, GLvoid* *params)) \
	X(glGetPolygonStipple, void, (GLubyte *mask)) \
	X(glGetSeparableFilter, void, (GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span)) \
	X(glGetString, const, (GLenum name)) \
	X(glGetTexEnvfv, void, (GLenum target, GLenum pname, GLfloat *params)) \
	X(glGetTexEnviv, void, (GLenum target, GLenum pname, GLint *params)) \
	X(glGetTexGendv, void, (GLenum coord, GLenum pname, GLdouble *params)) \
	X(glGetTexGenfv, void, (GLenum coord, GLenum pname, GLfloat *params)) \
	X(glGetTexGeniv, void, (GLenum coord, GLenum pname, GLint *params)) \
	X(glGetTexImage, void, (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels)) \
	X(glGetTexLevelParameterfv, void, (GLenum target, GLint level, GLenum pname, GLfloat *params)) \
	X(glGetTexLevelParameteriv, void, (GLenum target, GLint level, GLenum pname, GLint *params)) \
	X(glGetTexParameterfv, void, (GLenum target, GLenum pname, GLfloat *params)) \
	X(glGetTexParameteriv, void, (GLenum target, GLenum pname, GLint *params)) \
	X(glHint, void, (GLenum target, GLenum mode)) \
	X(glHistogram, void, (GLenum target, GLsizei width, GLenum internalformat, GLboolean sink)) \
	X(glIndexMask, void, (GLuint mask)) \
	X(glIndexPointer, void, (GLenum type, GLsizei stride, const GLvoid *pointer)) \
	X(glIndexd, void, (GLdouble c)) \
	X(glIndexdv, void, (const GLdouble *c)) \
	X(glIndexf, void, (GLfloat c)) \
	X(glIndexfv, void, (const GLfloat *c)) \
	X(glIndexi, void, (GLint c)) \
	X(glIndexiv, void, (const GLint *c)) \
	X(glIndexs, void, (GLshort c)) \
	X(glIndexsv, void, (const GLshort *c)) \
	X(glIndexub, void, (GLubyte c)) \
	X(glIndexubv, void, (const GLubyte *c)) \
	X(glInitNames, void, (void)) \
	X(glInterleavedArrays, void, (GLenum format, GLsizei stride, const GLvoid *pointer)) \
	X(glIsEnabled, GLboolean, (GLenum cap)) \
	X(glIsList, GLboolean, (GLuint list)) \
	X(glIsTexture, GLboolean, (GLuint texture)) \
	X(glLightModelf, void, (GLenum pname, GLfloat param)) \
	X(glLightModelfv, void, (GLenum pname, const GLfloat *params)) \
	X(glLightModeli, void, (GLenum pname, GLint param)) \
	X(glLightModeliv, void, (GLenum pname, const GLint *params)) \
	X(glLightf, void, (GLenum light, GLenum pname, GLfloat param)) \
	X(glLightfv, void, (GLenum light, GLenum pname, const GLfloat *params)) \
	X(glLighti, void, (GLenum light, GLenum pname, GLint param)) \
	X(glLightiv, void, (GLenum light, GLenum pname, const GLint *params)) \
	X(glLineStipple, void, (GLint factor, GLushort pattern)) \
	X(glLineWidth, void, (GLfloat width)) \
	X(glListBase, void, (GLuint base)) \
	X(glLoadIdentity, void, (void)) \
	X(glLoadMatrixd, void, (const GLdouble *m)) \
	X(glLoadMatrixf, void, (const GLfloat *m)) \
	X(glLoadName, void, (GLuint name)) \
	X(glLoadTransposeMatrixd, void, (const GLdouble *m)) \
	X(glLoadTransposeMatrixf, void, (const GLfloat *m)) \
	X(glLogicOp, void, (GLenum opcode)) \
	X(glMap1d, void, (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points)) \
	X(glMap1f, void, (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points)) \
	X(glMap2d, void, (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points)) \
	X(glMap2f, void, (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points)) \
	X(glMapGrid1d, void, (GLint un, GLdouble u1, GLdouble u2)) \
	X(glMapGrid1f, void, (GLint un, GLfloat u1, GLfloat u2)) \
	X(glMapGrid2d, void, (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)) \
	X(glMapGrid2f, void, (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)) \
	X(glMaterialf, void, (GLenum face, GLenum pname, GLfloat param)) \
	X(glMaterialfv, void, (GLenum face, GLenum pname, const GLfloat *params)) \
	X(glMateriali, void, (GLenum face, GLenum pname, GLint param)) \
	X(glMaterialiv, void, (GLenum face, GLenum pname, const GLint *params)) \
	X(glMatrixMode, void, (GLenum mode)) \
	X(glMinmax, void, (GLenum target, GLenum internalformat, GLboolean sink)) \
	X(glMultMatrixd, void, (const GLdouble *m)) \
	X(glMultMatrixf, void, (const GLfloat *m)) \
	X(glMultTransposeMatrixd, void, (const GLdouble *m)) \
	X(glMultTransposeMatrixf, void, (const GLfloat *m)) \
	X(glMultiDrawArrays, void, (GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount)) \
	X(glMultiDrawElements, void, (GLenum mode, const GLsizei *count, GLenum type, const GLvoid* *indices, GLsizei primcount)) \
	X(glMultiTexCoord1d, void, (GLenum target, GLdouble s)) \
	X(glMultiTexCoord1dv, void, (GLenum target, const GLdouble *v)) \
	X(glMultiTexCoord1f, void, (GLenum target, GLfloat s)) \
	X(glMultiTexCoord1fv, void, (GLenum target, const GLfloat *v)) \
	X(glMultiTexCoord1i, void, (GLenum target, GLint s)) \
	X(glMultiTexCoord1iv, void, (GLenum target, const GLint *v)) \
	X(glMultiTexCoord1s, void, (GLenum target, GLshort s)) \
	X(glMultiTexCoord1sv, void, (GLenum target, const GLshort *v)) \
	X(glMultiTexCoord2d, void, (GLenum target, GLdouble s, GLdouble t)) \
	X(glMultiTexCoord2dv, void, (GLenum target, const GLdouble *v)) \
	X(glMultiTexCoord2f, void, (GLenum target, GLfloat s, GLfloat t)) \
	X(glMultiTexCoord2fv, void, (GLenum target, const GLfloat *v)) \
	X(glMultiTexCoord2i, void, (GLenum target, GLint s, GLint t)) \
	X(glMultiTexCoord2iv, void, (GLenum target, const GLint *v)) \
	X(glMultiTexCoord2s, void, (GLenum target, GLshort s, GLshort t)) \
	X(glMultiTexCoord2sv, void, (GLenum target, const GLshort *v)) \
	X(glMultiTexCoord3d, void, (GLenum target, GLdouble s, GLdouble t, GLdouble r)) \
	X(glMultiTexCoord3dv, void, (GLenum target, const GLdouble *v)) \
	X(glMultiTexCoord3f, void, (GLenum target, GLfloat s, GLfloat t, GLfloat r)) \
	X(glMultiTexCoord3fv, void, (GLenum target, const GLfloat *v)) \
	X(glMultiTexCoord3i, void, (GLenum target, GLint s, GLint t, GLint r)) \
	X(glMultiTexCoord3iv, void, (GLenum target, const GLint *v)) \
	X(glMultiTexCoord3s, void, (GLenum target, GLshort s, GLshort t, GLshort r)) \
	X(glMultiTexCoord3sv, void, (GLenum target, const GLshort *v)) \
	X(glMultiTexCoord4d, void, (GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q)) \
	X(glMultiTexCoord4dv, void, (GLenum target, const GLdouble *v)) \
	X(glMultiTexCoord4f, void, (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)) \
	X(glMultiTexCoord4fv, void, (GLenum target, const GLfloat *v)) \
	X(glMultiTexCoord4i, void, (GLenum target, GLint s, GLint t, GLint r, GLint q)) \
	X(glMultiTexCoord4iv, void, (GLenum target, const GLint *v)) \
	X(glMultiTexCoord4s, void, (GLenum target, GLshort s, GLshort t, GLshort r, GLshort q)) \
	X(glMultiTexCoord4sv, void, (GLenum target, const GLshort *v)) \
	X(glNewList, void, (GLuint list, GLenum mode)) \
	X(glNormal3b, void, (GLbyte nx, GLbyte ny, GLbyte nz)) \
	X(glNormal3bv, void, (const GLbyte *v)) \
	X(glNormal3d, void, (GLdouble nx, GLdouble ny, GLdouble nz)) \
	X(glNormal3dv, void, (const GLdouble *v)) \
	X(glNormal3f, void, (GLfloat nx, GLfloat ny, GLfloat nz)) \
	X(glNormal3fv, void, (const GLfloat *v)) \
	X(glNormal3i, void, (GLint nx, GLint ny, GLint nz)) \
	X(glNormal3iv, void, (const GLint *v)) \
	X(glNormal3s, void, (GLshort nx, GLshort ny, GLshort nz)) \
	X(glNormal3sv, void, (const GLshort *v)) \
	X(glNormalPointer, void, (GLenum type, GLsizei stride, const GLvoid *pointer)) \
	X(glOrtho, void, (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)) \
	X(glPassThrough, void, (GLfloat token)) \
	X(glPixelMapfv, void, (GLenum map, GLint mapsize, const GLfloat *values)) \
	X(glPixelMapuiv, void, (GLenum map, GLint mapsize, const GLuint *values)) \
	X(glPixelMapusv, void, (GLenum map, GLint mapsize, const GLushort *values)) \
	X(glPixelStoref, void, (GLenum pname, GLfloat param)) \
	X(glPixelStorei, void, (GLenum pname, GLint param)) \
	X(glPixelTransferf, void, (GLenum pname, GLfloat param)) \
	X(glPixelTransferi, void, (GLenum pname, GLint param)) \
	X(glPixelZoom, void, (GLfloat xfactor, GLfloat yfactor)) \
	X(glPointParameterf, void, (GLenum pname, GLfloat param)) \
	X(glPointParameterfv, void, (GLenum pname, const GLfloat *params)) \
	X(glPointParameteri, void, (GLenum pname, GLint param)) \
	X(glPointParameteriv, void, (GLenum pname, const GLint *params)) \
	X(glPointSize, void, (GLfloat size)) \
	X(glPolygonMode, void, (GLenum face, GLenum mode)) \
	X(glPolygonOffset, void, (GLfloat factor, GLfloat units)) \
	X(glPolygonStipple, void, (const GLubyte *mask)) \
	X(glPopAttrib, void, (void)) \
	X(glPopClientAttrib, void, (void)) \
	X(glPopMatrix, void, (void)) \
	X(glPopName, void, (void)) \
	X(glPrioritizeTextures, void, (GLsizei n, const GLuint *textures, const GLclampf *priorities)) \
	X(glPushAttrib, void, (GLbitfield mask)) \
	X(glPushClientAttrib, void, (GLbitfield mask)) \
	X(glPushMatrix, void, (void)) \
	X(glPushName, void, (GLuint name)) \
	X(glRasterPos2d, void, (GLdouble x, GLdouble y)) \
	X(glRasterPos2dv, void, (const GLdouble *v)) \
	X(glRasterPos2f, void, (GLfloat x, GLfloat y)) \
	X(glRasterPos2fv, void, (const GLfloat *v)) \
	X(glRasterPos2i, void, (GLint x, GLint y)) \
	X(glRasterPos2iv, void, (const GLint *v)) \
	X(glRasterPos2s, void, (GLshort x, GLshort y)) \
	X(glRasterPos2sv, void, (const GLshort *v)) \
	X(glRasterPos3d, void, (GLdouble x, GLdouble y, GLdouble z)) \
	X(glRasterPos3dv, void, (const GLdouble *v)) \
	X(glRasterPos3f, void, (GLfloat x, GLfloat y, GLfloat z)) \
	X(glRasterPos3fv, void, (const GLfloat *v)) \
	X(glRasterPos3i, void, (GLint x, GLint y, GLint z)) \
	X(glRasterPos3iv, void, (const GLint *v)) \
	X(glRasterPos3s, void, (GLshort x, GLshort y, GLshort z)) \
	X(glRasterPos3sv, void, (const GLshort *v)) \
	X(glRasterPos4d, void, (GLdouble x, GLdouble y, GLdouble z, GLdouble w)) \
	X(glRasterPos4dv, void, (const GLdouble *v)) \
	X(glRasterPos4f, void, (GLfloat x, GLfloat y, GLfloat z, GLfloat w)) \
	X(glRasterPos4fv, void, (const GLfloat *v)) \
	X(glRasterPos4i, void, (GLint x, GLint y, GLint z, GLint w)) \
	X(glRasterPos4iv, void, (const GLint *v)) \
	X(glRasterPos4s, void, (GLshort x, GLshort y, GLshort z, GLshort w)) \
	X(glRasterPos4sv, void, (const GLshort *v)) \
	X(glReadBuffer, void, (GLenum mode)) \
	X(glReadPixels, void, (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)) \
	X(glRectd, void, (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)) \
	X(glRectdv, void, (const GLdouble *v1, const GLdouble *v2)) \
	X(glRectf, void, (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)) \
	X(glRectfv, void, (const GLfloat *v1, const GLfloat *v2)) \
	X(glRecti, void, (GLint x1, GLint y1, GLint x2, GLint y2)) \
	X(glRectiv, void, (const GLint *v1, const GLint *v2)) \
	X(glRects, void, (GLshort x1, GLshort y1, GLshort x2, GLshort y2)) \
	X(glRectsv, void, (const GLshort *v1, const GLshort *v2)) \
	X(glReleaseFlushHold, GLenum, (GLuint id)) \
	X(glRenderMode, GLint, (GLenum mode)) \
	X(glResetHistogram, void, (GLenum target)) \
	X(glResetMinmax, void, (GLenum target)) \
	X(glRotated, void, (GLdouble angle, GLdouble x, GLdouble y, GLdouble z)) \
	X(glRotatef, void, (GLfloat angle, GLfloat x, GLfloat y, GLfloat z)) \
	X(glSampleCoverage, void, (GLclampf value, GLboolean invert)) \
	X(glScaled, void, (GLdouble x, GLdouble y, GLdouble z)) \
	X(glScalef, void, (GLfloat x, GLfloat y, GLfloat z)) \
	X(glScissor, void, (GLint x, GLint y, GLsizei width, GLsizei height)) \
	X(glSecondaryColor3b, void, (GLbyte red, GLbyte green, GLbyte blue)) \
	X(glSecondaryColor3bv, void, (const GLbyte *v)) \
	X(glSecondaryColor3d, void, (GLdouble red, GLdouble green, GLdouble blue)) \
	X(glSecondaryColor3dv, void, (const GLdouble *v)) \
	X(glSecondaryColor3f, void, (GLfloat red, GLfloat green, GLfloat blue)) \
	X(glSecondaryColor3fv, void, (const GLfloat *v)) \
	X(glSecondaryColor3i, void, (GLint red, GLint green, GLint blue)) \
	X(glSecondaryColor3iv, void, (const GLint *v)) \
	X(glSecondaryColor3s, void, (GLshort red, GLshort green, GLshort blue)) \
	X(glSecondaryColor3sv, void, (const GLshort *v)) \
	X(glSecondaryColor3ub, void, (GLubyte red, GLubyte green, GLubyte blue)) \
	X(glSecondaryColor3ubv, void, (const GLubyte *v)) \
	X(glSecondaryColor3ui, void, (GLuint red, GLuint green, GLuint blue)) \
	X(glSecondaryColor3uiv, void, (const GLuint *v)) \
	X(glSecondaryColor3us, void, (GLushort red, GLushort green, GLushort blue)) \
	X(glSecondaryColor3usv, void, (const GLushort *v)) \
	X(glSecondaryColorPointer, void, (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)) \
	X(glSelectBuffer, void, (GLsizei size, GLuint *buffer)) \
	X(glSeparableFilter2D, void, (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column)) \
	X(glShadeModel, void, (GLenum mode)) \
	X(glStencilFunc, void, (GLenum func, GLint ref, GLuint mask)) \
	X(glStencilMask, void, (GLuint mask)) \
	X(glStencilOp, void, (GLenum fail, GLenum zfail, GLenum zpass)) \
	X(glTexCoord1d, void, (GLdouble s)) \
	X(glTexCoord1dv, void, (const GLdouble *v)) \
	X(glTexCoord1f, void, (GLfloat s)) \
	X(glTexCoord1fv, void, (const GLfloat *v)) \
	X(glTexCoord1i, void, (GLint s)) \
	X(glTexCoord1iv, void, (const GLint *v)) \
	X(glTexCoord1s, void, (GLshort s)) \
	X(glTexCoord1sv, void, (const GLshort *v)) \
	X(glTexCoord2d, void, (GLdouble s, GLdouble t)) \
	X(glTexCoord2dv, void, (const GLdouble *v)) \
	X(glTexCoord2f, void, (GLfloat s, GLfloat t)) \
	X(glTexCoord2fv, void, (const GLfloat *v)) \
	X(glTexCoord2i, void, (GLint s, GLint t)) \
	X(glTexCoord2iv, void, (const GLint *v)) \
	X(glTexCoord2s, void, (GLshort s, GLshort t)) \
	X(glTexCoord2sv, void, (const GLshort *v)) \
	X(glTexCoord3d, void, (GLdouble s, GLdouble t, GLdouble r)) \
	X(glTexCoord3dv, void, (const GLdouble *v)) \
	X(glTexCoord3f, void, (GLfloat s, GLfloat t, GLfloat r)) \
	X(glTexCoord3fv, void, (const GLfloat *v)) \
	X(glTexCoord3i, void, (GLint s, GLint t, GLint r)) \
	X(glTexCoord3iv, void, (const GLint *v)) \
	X(glTexCoord3s, void, (GLshort s, GLshort t, GLshort r)) \
	X(glTexCoord3sv, void, (const GLshort *v)) \
	X(glTexCoord4d, void, (GLdouble s, GLdouble t, GLdouble r, GLdouble q)) \
	X(glTexCoord4dv, void, (const GLdouble *v)) \
	X(glTexCoord4f, void, (GLfloat s, GLfloat t, GLfloat r, GLfloat q)) \
	X(glTexCoord4fv, void, (const GLfloat *v)) \
	X(glTexCoord4i, void, (GLint s, GLint t, GLint r, GLint q)) \
	X(glTexCoord4iv, void, (const GLint *v)) \
	X(glTexCoord4s, void, (GLshort s, GLshort t, GLshort r, GLshort q)) \
	X(glTexCoord4sv, void, (const GLshort *v)) \
	X(glTexCoordPointer, void, (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)) \
	X(glTexEnvf, void, (GLenum target, GLenum pname, GLfloat param)) \
	X(glTexEnvfv, void, (GLenum target, GLenum pname, const GLfloat *params)) \
	X(glTexEnvi, void, (GLenum target, GLenum pname, GLint param)) \
	X(glTexEnviv, void, (GLenum target, GLenum pname, const GLint *params)) \
	X(glTexGend, void, (GLenum coord, GLenum pname, GLdouble param)) \
	X(glTexGendv, void, (GLenum coord, GLenum pname, const GLdouble *params)) \
	X(glTexGenf, void, (GLenum coord, GLenum pname, GLfloat param)) \
	X(glTexGenfv, void, (GLenum coord, GLenum pname, const GLfloat *params)) \
	X(glTexGeni, void, (GLenum coord, GLenum pname, GLint param)) \
	X(glTexGeniv, void, (GLenum coord, GLenum pname, const GLint *params)) \
	X(glTexImage1D, void, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels)) \
	X(glTexImage2D, void, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)) \
	X(glTexImage3D, void, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels)) \
	X(glTexParameterf, void, (GLenum target, GLenum pname, GLfloat param)) \
	X(glTexParameterfv, void, (GLenum target, GLenum pname, const GLfloat *params)) \
	X(glTexParameteri, void, (GLenum target, GLenum pname, GLint param)) \
	X(glTexParameteriv, void, (GLenum target, GLenum pname, const GLint *params)) \
	X(glTexSubImage1D, void, (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels)) \
	X(glTexSubImage2D, void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)) \
	X(glTexSubImage3D, void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels)) \
	X(glTranslated, void, (GLdouble x, GLdouble y, GLdouble z)) \
	X(glTranslatef, void, (GLfloat x, GLfloat y, GLfloat z)) \
	X(glValidBackBufferHintAutodesk, GLboolean, (GLint x, GLint y, GLsizei width, GLsizei height)) \
	X(glVertex2d, void, (GLdouble x, GLdouble y)) \
	X(glVertex2dv, void, (const GLdouble *v)) \
	X(glVertex2f, void, (GLfloat x, GLfloat y)) \
	X(glVertex2fv, void, (const GLfloat *v)) \
	X(glVertex2i, void, (GLint x, GLint y)) \
	X(glVertex2iv, void, (const GLint *v)) \
	X(glVertex2s, void, (GLshort x, GLshort y)) \
	X(glVertex2sv, void, (const GLshort *v)) \
	X(glVertex3d, void, (GLdouble x, GLdouble y, GLdouble z)) \
	X(glVertex3dv, void, (const GLdouble *v)) \
	X(glVertex3f, void, (GLfloat x, GLfloat y, GLfloat z)) \
	X(glVertex3fv, void, (const GLfloat *v)) \
	X(glVertex3i, void, (GLint x, GLint y, GLint z)) \
	X(glVertex3iv, void, (const GLint *v)) \
	X(glVertex3s, void, (GLshort x, GLshort y, GLshort z)) \
	X(glVertex3sv, void, (const GLshort *v)) \
	X(glVertex4d, void, (GLdouble x, GLdouble y, GLdouble z, GLdouble w)) \
	X(glVertex4dv, void, (const GLdouble *v)) \
	X(glVertex4f, void, (GLfloat x, GLfloat y, GLfloat z, GLfloat w)) \
	X(glVertex4fv, void, (const GLfloat *v)) \
	X(glVertex4i, void, (GLint x, GLint y, GLint z, GLint w)) \
	X(glVertex4iv, void, (const GLint *v)) \
	X(glVertex4s, void, (GLshort x, GLshort y, GLshort z, GLshort w)) \
	X(glVertex4sv, void, (const GLshort *v)) \
	X(glVertexPointer, void, (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)) \
	X(glViewport, void, (GLint x, GLint y, GLsizei width, GLsizei height)) \
	X(glWindowBackBufferHintAutodesk, void, (void)) \
	X(glWindowPos2d, void, (GLdouble x, GLdouble y)) \
	X(glWindowPos2dv, void, (const GLdouble *p)) \
	X(glWindowPos2f, void, (GLfloat x, GLfloat y)) \
	X(glWindowPos2fv, void, (const GLfloat *p)) \
	X(glWindowPos2i, void, (GLint x, GLint y)) \
	X(glWindowPos2iv, void, (const GLint *p)) \
	X(glWindowPos2s, void, (GLshort x, GLshort y)) \
	X(glWindowPos2sv, void, (const GLshort *p)) \
	X(glWindowPos3d, void, (GLdouble x, GLdouble y, GLdouble z)) \
	X(glWindowPos3dv, void, (const GLdouble *p)) \
	X(glWindowPos3f, void, (GLfloat x, GLfloat y, GLfloat z)) \
	X(glWindowPos3fv, void, (const GLfloat *p)) \
	X(glWindowPos3i, void, (GLint x, GLint y, GLint z)) \
	X(glWindowPos3iv, void, (const GLint *p)) \
	X(glWindowPos3s, void, (GLshort x, GLshort y, GLshort z)) \
	X(glWindowPos3sv, void, (const GLshort *p)) \


/* EOF */
