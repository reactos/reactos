/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/opengl32/glfuncs.h
 * PURPOSE:              OpenGL32 lib, GLFUNCS_MACRO
 * PROGRAMMER:           gen_glfuncs_macro.sh
 * UPDATE HISTORY:
 *               !!! AUTOMATICALLY CREATED FROM glfuncs.csv !!!
 */

/* To use this macro define a macro X(name, ret, typeargs, args).
 * It gets called for every glXXX function. For glVertex3f name would be "glVertex3f",
 * ret would be "void", typeargs would be "(GLfloat x, GLfloat y, GLfloat z)" and
 * args would be "(x,y,z)".
 * Don't forget to undefine X ;-)
 */

#define GLFUNCS_MACRO \
	X(glAccum, void, (GLenum op, GLfloat value), (op,value)) \
	X(glActiveTexture, void, (GLenum texture), (texture)) \
	X(glAddSwapHintRectWIN, void, (GLint x, GLint y, GLsizei width, GLsizei height), (x,y,width,height)) \
	X(glAlphaFunc, void, (GLenum func, GLclampf ref), (func,ref)) \
	X(glAreTexturesResident, GLboolean, (GLsizei n, const GLuint *textures, GLboolean *residences), (n,textures,residences)) \
	X(glArrayElement, void, (GLint i), (i)) \
	X(glBegin, void, (GLenum mode), (mode)) \
	X(glBindTexture, void, (GLenum target, GLuint texture), (target,texture)) \
	X(glBitmap, void, (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap), (width,height,xorig,yorig,xmove,ymove,bitmap)) \
	X(glBlendColor, void, (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha), (red,green,blue,alpha)) \
	X(glBlendEquation, void, (GLenum mode), (mode)) \
	X(glBlendFunc, void, (GLenum sfactor, GLenum dfactor), (sfactor,dfactor)) \
	X(glBlendFuncSeparate, void, (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha), (sfactorRGB,dfactorRGB,sfactorAlpha,dfactorAlpha)) \
	X(glCallList, void, (GLuint list), (list)) \
	X(glCallLists, void, (GLsizei n, GLenum type, const GLvoid *lists), (n,type,lists)) \
	X(glClear, void, (GLbitfield mask), (mask)) \
	X(glClearAccum, void, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha), (red,green,blue,alpha)) \
	X(glClearColor, void, (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha), (red,green,blue,alpha)) \
	X(glClearDepth, void, (GLclampd depth), (depth)) \
	X(glClearIndex, void, (GLfloat c), (c)) \
	X(glClearStencil, void, (GLint s), (s)) \
	X(glClientActiveTexture, void, (GLenum texture), (texture)) \
	X(glClipPlane, void, (GLenum plane, const GLdouble *equation), (plane,equation)) \
	X(glColor3b, void, (GLbyte red, GLbyte green, GLbyte blue), (red,green,blue)) \
	X(glColor3bv, void, (const GLbyte *v), (v)) \
	X(glColor3d, void, (GLdouble red, GLdouble green, GLdouble blue), (red,green,blue)) \
	X(glColor3dv, void, (const GLdouble *v), (v)) \
	X(glColor3f, void, (GLfloat red, GLfloat green, GLfloat blue), (red,green,blue)) \
	X(glColor3fv, void, (const GLfloat *v), (v)) \
	X(glColor3i, void, (GLint red, GLint green, GLint blue), (red,green,blue)) \
	X(glColor3iv, void, (const GLint *v), (v)) \
	X(glColor3s, void, (GLshort red, GLshort green, GLshort blue), (red,green,blue)) \
	X(glColor3sv, void, (const GLshort *v), (v)) \
	X(glColor3ub, void, (GLubyte red, GLubyte green, GLubyte blue), (red,green,blue)) \
	X(glColor3ubv, void, (const GLubyte *v), (v)) \
	X(glColor3ui, void, (GLuint red, GLuint green, GLuint blue), (red,green,blue)) \
	X(glColor3uiv, void, (const GLuint *v), (v)) \
	X(glColor3us, void, (GLushort red, GLushort green, GLushort blue), (red,green,blue)) \
	X(glColor3usv, void, (const GLushort *v), (v)) \
	X(glColor4b, void, (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha), (red,green,blue,alpha)) \
	X(glColor4bv, void, (const GLbyte *v), (v)) \
	X(glColor4d, void, (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha), (red,green,blue,alpha)) \
	X(glColor4dv, void, (const GLdouble *v), (v)) \
	X(glColor4f, void, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha), (red,green,blue,alpha)) \
	X(glColor4fv, void, (const GLfloat *v), (v)) \
	X(glColor4i, void, (GLint red, GLint green, GLint blue, GLint alpha), (red,green,blue,alpha)) \
	X(glColor4iv, void, (const GLint *v), (v)) \
	X(glColor4s, void, (GLshort red, GLshort green, GLshort blue, GLshort alpha), (red,green,blue,alpha)) \
	X(glColor4sv, void, (const GLshort *v), (v)) \
	X(glColor4ub, void, (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha), (red,green,blue,alpha)) \
	X(glColor4ubv, void, (const GLubyte *v), (v)) \
	X(glColor4ui, void, (GLuint red, GLuint green, GLuint blue, GLuint alpha), (red,green,blue,alpha)) \
	X(glColor4uiv, void, (const GLuint *v), (v)) \
	X(glColor4us, void, (GLushort red, GLushort green, GLushort blue, GLushort alpha), (red,green,blue,alpha)) \
	X(glColor4usv, void, (const GLushort *v), (v)) \
	X(glColorMask, void, (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha), (red,green,blue,alpha)) \
	X(glColorMaterial, void, (GLenum face, GLenum mode), (face,mode)) \
	X(glColorPointer, void, (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer), (size,type,stride,pointer)) \
	X(glColorSubTable, void, (GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data), (target,start,count,format,type,data)) \
	X(glColorTable, void, (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table), (target,internalformat,width,format,type,table)) \
	X(glColorTableParameterfv, void, (GLenum target, GLenum pname, const GLfloat *params), (target,pname,params)) \
	X(glColorTableParameteriv, void, (GLenum target, GLenum pname, const GLint *params), (target,pname,params)) \
	X(glCompressedTexImage1D, void, (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data), (target,level,internalformat,width,border,imageSize,data)) \
	X(glCompressedTexImage2D, void, (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data), (target,level,internalformat,width,height,border,imageSize,data)) \
	X(glCompressedTexImage3D, void, (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data), (target,level,internalformat,width,height,depth,border,imageSize,data)) \
	X(glCompressedTexSubImage1D, void, (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *data), (target,level,xoffset,width,format,imageSize,data)) \
	X(glCompressedTexSubImage2D, void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data), (target,level,xoffset,yoffset,width,height,format,imageSize,data)) \
	X(glCompressedTexSubImage3D, void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data), (target,level,xoffset,yoffset,zoffset,width,height,depth,format,imageSize,data)) \
	X(glConvolutionFilter1D, void, (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image), (target,internalformat,width,format,type,image)) \
	X(glConvolutionFilter2D, void, (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image), (target,internalformat,width,height,format,type,image)) \
	X(glConvolutionParameterf, void, (GLenum target, GLenum pname, GLfloat params), (target,pname,params)) \
	X(glConvolutionParameterfv, void, (GLenum target, GLenum pname, const GLfloat *params), (target,pname,params)) \
	X(glConvolutionParameteri, void, (GLenum target, GLenum pname, GLint params), (target,pname,params)) \
	X(glConvolutionParameteriv, void, (GLenum target, GLenum pname, const GLint *params), (target,pname,params)) \
	X(glCopyColorSubTable, void, (GLenum target, GLsizei start, GLint x, GLint y, GLsizei width), (target,start,x,y,width)) \
	X(glCopyColorTable, void, (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width), (target,internalformat,x,y,width)) \
	X(glCopyConvolutionFilter1D, void, (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width), (target,internalformat,x,y,width)) \
	X(glCopyConvolutionFilter2D, void, (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height), (target,internalformat,x,y,width,height)) \
	X(glCopyPixels, void, (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type), (x,y,width,height,type)) \
	X(glCopyTexImage1D, void, (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border), (target,level,internalformat,x,y,width,border)) \
	X(glCopyTexImage2D, void, (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border), (target,level,internalformat,x,y,width,height,border)) \
	X(glCopyTexSubImage1D, void, (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width), (target,level,xoffset,x,y,width)) \
	X(glCopyTexSubImage2D, void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height), (target,level,xoffset,yoffset,x,y,width,height)) \
	X(glCopyTexSubImage3D, void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height), (target,level,xoffset,yoffset,zoffset,x,y,width,height)) \
	X(glCullFace, void, (GLenum mode), (mode)) \
	X(glDeleteLists, void, (GLuint list, GLsizei range), (list,range)) \
	X(glDeleteTextures, void, (GLsizei n, const GLuint *textures), (n,textures)) \
	X(glDepthFunc, void, (GLenum func), (func)) \
	X(glDepthMask, void, (GLboolean flag), (flag)) \
	X(glDepthRange, void, (GLclampd zNear, GLclampd zFar), (zNear,zFar)) \
	X(glDisable, void, (GLenum cap), (cap)) \
	X(glDisableClientState, void, (GLenum array), (array)) \
	X(glDrawArrays, void, (GLenum mode, GLint first, GLsizei count), (mode,first,count)) \
	X(glDrawBuffer, void, (GLenum mode), (mode)) \
	X(glDrawElements, void, (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices), (mode,count,type,indices)) \
	X(glDrawPixels, void, (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels), (width,height,format,type,pixels)) \
	X(glDrawRangeElements, void, (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices), (mode,start,end,count,type,indices)) \
	X(glEdgeFlag, void, (GLboolean flag), (flag)) \
	X(glEdgeFlagPointer, void, (GLsizei stride, const GLboolean *pointer), (stride,pointer)) \
	X(glEdgeFlagv, void, (const GLboolean *flag), (flag)) \
	X(glEnable, void, (GLenum cap), (cap)) \
	X(glEnableClientState, void, (GLenum array), (array)) \
	X(glEnd, void, (void), ()) \
	X(glEndList, void, (void), ()) \
	X(glEvalCoord1d, void, (GLdouble u), (u)) \
	X(glEvalCoord1dv, void, (const GLdouble *u), (u)) \
	X(glEvalCoord1f, void, (GLfloat u), (u)) \
	X(glEvalCoord1fv, void, (const GLfloat *u), (u)) \
	X(glEvalCoord2d, void, (GLdouble u, GLdouble v), (u,v)) \
	X(glEvalCoord2dv, void, (const GLdouble *u), (u)) \
	X(glEvalCoord2f, void, (GLfloat u, GLfloat v), (u,v)) \
	X(glEvalCoord2fv, void, (const GLfloat *u), (u)) \
	X(glEvalMesh1, void, (GLenum mode, GLint i1, GLint i2), (mode,i1,i2)) \
	X(glEvalMesh2, void, (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2), (mode,i1,i2,j1,j2)) \
	X(glEvalPoint1, void, (GLint i), (i)) \
	X(glEvalPoint2, void, (GLint i, GLint j), (i,j)) \
	X(glFeedbackBuffer, void, (GLsizei size, GLenum type, GLfloat *buffer), (size,type,buffer)) \
	X(glFinish, void, (void), ()) \
	X(glFlush, void, (void), ()) \
	X(glFlushHold, GLuint, (void), ()) \
	X(glFogCoordPointer, void, (GLenum type, GLsizei stride, const GLvoid *pointer), (type,stride,pointer)) \
	X(glFogCoordd, void, (GLdouble fog), (fog)) \
	X(glFogCoorddv, void, (const GLdouble *fog), (fog)) \
	X(glFogCoordf, void, (GLfloat fog), (fog)) \
	X(glFogCoordfv, void, (const GLfloat *fog), (fog)) \
	X(glFogf, void, (GLenum pname, GLfloat param), (pname,param)) \
	X(glFogfv, void, (GLenum pname, const GLfloat *params), (pname,params)) \
	X(glFogi, void, (GLenum pname, GLint param), (pname,param)) \
	X(glFogiv, void, (GLenum pname, const GLint *params), (pname,params)) \
	X(glFrontFace, void, (GLenum mode), (mode)) \
	X(glFrustum, void, (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar), (left,right,bottom,top,zNear,zFar)) \
	X(glGenLists, GLuint, (GLsizei range), (range)) \
	X(glGenTextures, void, (GLsizei n, GLuint *textures), (n,textures)) \
	X(glGetBooleanv, void, (GLenum pname, GLboolean *params), (pname,params)) \
	X(glGetClipPlane, void, (GLenum plane, GLdouble *equation), (plane,equation)) \
	X(glGetColorTable, void, (GLenum target, GLenum format, GLenum type, GLvoid *table), (target,format,type,table)) \
	X(glGetColorTableParameterfv, void, (GLenum target, GLenum pname, GLfloat *params), (target,pname,params)) \
	X(glGetColorTableParameteriv, void, (GLenum target, GLenum pname, GLint *params), (target,pname,params)) \
	X(glGetCompressedTexImage, void, (GLenum target, GLint lod, GLvoid *img), (target,lod,img)) \
	X(glGetConvolutionFilter, void, (GLenum target, GLenum format, GLenum type, GLvoid *image), (target,format,type,image)) \
	X(glGetConvolutionParameterfv, void, (GLenum target, GLenum pname, GLfloat *params), (target,pname,params)) \
	X(glGetConvolutionParameteriv, void, (GLenum target, GLenum pname, GLint *params), (target,pname,params)) \
	X(glGetDoublev, void, (GLenum pname, GLdouble *params), (pname,params)) \
	X(glGetError, GLenum, (void), ()) \
	X(glGetFloatv, void, (GLenum pname, GLfloat *params), (pname,params)) \
	X(glGetHistogram, void, (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values), (target,reset,format,type,values)) \
	X(glGetHistogramParameterfv, void, (GLenum target, GLenum pname, GLfloat *params), (target,pname,params)) \
	X(glGetHistogramParameteriv, void, (GLenum target, GLenum pname, GLint *params), (target,pname,params)) \
	X(glGetIntegerv, void, (GLenum pname, GLint *params), (pname,params)) \
	X(glGetLightfv, void, (GLenum light, GLenum pname, GLfloat *params), (light,pname,params)) \
	X(glGetLightiv, void, (GLenum light, GLenum pname, GLint *params), (light,pname,params)) \
	X(glGetMapdv, void, (GLenum target, GLenum query, GLdouble *v), (target,query,v)) \
	X(glGetMapfv, void, (GLenum target, GLenum query, GLfloat *v), (target,query,v)) \
	X(glGetMapiv, void, (GLenum target, GLenum query, GLint *v), (target,query,v)) \
	X(glGetMaterialfv, void, (GLenum face, GLenum pname, GLfloat *params), (face,pname,params)) \
	X(glGetMaterialiv, void, (GLenum face, GLenum pname, GLint *params), (face,pname,params)) \
	X(glGetMinmax, void, (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values), (target,reset,format,type,values)) \
	X(glGetMinmaxParameterfv, void, (GLenum target, GLenum pname, GLfloat *params), (target,pname,params)) \
	X(glGetMinmaxParameteriv, void, (GLenum target, GLenum pname, GLint *params), (target,pname,params)) \
	X(glGetPixelMapfv, void, (GLenum map, GLfloat *values), (map,values)) \
	X(glGetPixelMapuiv, void, (GLenum map, GLuint *values), (map,values)) \
	X(glGetPixelMapusv, void, (GLenum map, GLushort *values), (map,values)) \
	X(glGetPointerv, void, (GLenum pname, GLvoid* *params), (pname,params)) \
	X(glGetPolygonStipple, void, (GLubyte *mask), (mask)) \
	X(glGetSeparableFilter, void, (GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span), (target,format,type,row,column,span)) \
	X(glGetString, const, (GLenum name), (name)) \
	X(glGetTexEnvfv, void, (GLenum target, GLenum pname, GLfloat *params), (target,pname,params)) \
	X(glGetTexEnviv, void, (GLenum target, GLenum pname, GLint *params), (target,pname,params)) \
	X(glGetTexGendv, void, (GLenum coord, GLenum pname, GLdouble *params), (coord,pname,params)) \
	X(glGetTexGenfv, void, (GLenum coord, GLenum pname, GLfloat *params), (coord,pname,params)) \
	X(glGetTexGeniv, void, (GLenum coord, GLenum pname, GLint *params), (coord,pname,params)) \
	X(glGetTexImage, void, (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels), (target,level,format,type,pixels)) \
	X(glGetTexLevelParameterfv, void, (GLenum target, GLint level, GLenum pname, GLfloat *params), (target,level,pname,params)) \
	X(glGetTexLevelParameteriv, void, (GLenum target, GLint level, GLenum pname, GLint *params), (target,level,pname,params)) \
	X(glGetTexParameterfv, void, (GLenum target, GLenum pname, GLfloat *params), (target,pname,params)) \
	X(glGetTexParameteriv, void, (GLenum target, GLenum pname, GLint *params), (target,pname,params)) \
	X(glHint, void, (GLenum target, GLenum mode), (target,mode)) \
	X(glHistogram, void, (GLenum target, GLsizei width, GLenum internalformat, GLboolean sink), (target,width,internalformat,sink)) \
	X(glIndexMask, void, (GLuint mask), (mask)) \
	X(glIndexPointer, void, (GLenum type, GLsizei stride, const GLvoid *pointer), (type,stride,pointer)) \
	X(glIndexd, void, (GLdouble c), (c)) \
	X(glIndexdv, void, (const GLdouble *c), (c)) \
	X(glIndexf, void, (GLfloat c), (c)) \
	X(glIndexfv, void, (const GLfloat *c), (c)) \
	X(glIndexi, void, (GLint c), (c)) \
	X(glIndexiv, void, (const GLint *c), (c)) \
	X(glIndexs, void, (GLshort c), (c)) \
	X(glIndexsv, void, (const GLshort *c), (c)) \
	X(glIndexub, void, (GLubyte c), (c)) \
	X(glIndexubv, void, (const GLubyte *c), (c)) \
	X(glInitNames, void, (void), ()) \
	X(glInterleavedArrays, void, (GLenum format, GLsizei stride, const GLvoid *pointer), (format,stride,pointer)) \
	X(glIsEnabled, GLboolean, (GLenum cap), (cap)) \
	X(glIsList, GLboolean, (GLuint list), (list)) \
	X(glIsTexture, GLboolean, (GLuint texture), (texture)) \
	X(glLightModelf, void, (GLenum pname, GLfloat param), (pname,param)) \
	X(glLightModelfv, void, (GLenum pname, const GLfloat *params), (pname,params)) \
	X(glLightModeli, void, (GLenum pname, GLint param), (pname,param)) \
	X(glLightModeliv, void, (GLenum pname, const GLint *params), (pname,params)) \
	X(glLightf, void, (GLenum light, GLenum pname, GLfloat param), (light,pname,param)) \
	X(glLightfv, void, (GLenum light, GLenum pname, const GLfloat *params), (light,pname,params)) \
	X(glLighti, void, (GLenum light, GLenum pname, GLint param), (light,pname,param)) \
	X(glLightiv, void, (GLenum light, GLenum pname, const GLint *params), (light,pname,params)) \
	X(glLineStipple, void, (GLint factor, GLushort pattern), (factor,pattern)) \
	X(glLineWidth, void, (GLfloat width), (width)) \
	X(glListBase, void, (GLuint base), (base)) \
	X(glLoadIdentity, void, (void), ()) \
	X(glLoadMatrixd, void, (const GLdouble *m), (m)) \
	X(glLoadMatrixf, void, (const GLfloat *m), (m)) \
	X(glLoadName, void, (GLuint name), (name)) \
	X(glLoadTransposeMatrixd, void, (const GLdouble *m), (m)) \
	X(glLoadTransposeMatrixf, void, (const GLfloat *m), (m)) \
	X(glLogicOp, void, (GLenum opcode), (opcode)) \
	X(glMap1d, void, (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points), (target,u1,u2,stride,order,points)) \
	X(glMap1f, void, (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points), (target,u1,u2,stride,order,points)) \
	X(glMap2d, void, (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points), (target,u1,u2,ustride,uorder,v1,v2,vstride,vorder,points)) \
	X(glMap2f, void, (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points), (target,u1,u2,ustride,uorder,v1,v2,vstride,vorder,points)) \
	X(glMapGrid1d, void, (GLint un, GLdouble u1, GLdouble u2), (un,u1,u2)) \
	X(glMapGrid1f, void, (GLint un, GLfloat u1, GLfloat u2), (un,u1,u2)) \
	X(glMapGrid2d, void, (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2), (un,u1,u2,vn,v1,v2)) \
	X(glMapGrid2f, void, (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2), (un,u1,u2,vn,v1,v2)) \
	X(glMaterialf, void, (GLenum face, GLenum pname, GLfloat param), (face,pname,param)) \
	X(glMaterialfv, void, (GLenum face, GLenum pname, const GLfloat *params), (face,pname,params)) \
	X(glMateriali, void, (GLenum face, GLenum pname, GLint param), (face,pname,param)) \
	X(glMaterialiv, void, (GLenum face, GLenum pname, const GLint *params), (face,pname,params)) \
	X(glMatrixMode, void, (GLenum mode), (mode)) \
	X(glMinmax, void, (GLenum target, GLenum internalformat, GLboolean sink), (target,internalformat,sink)) \
	X(glMultMatrixd, void, (const GLdouble *m), (m)) \
	X(glMultMatrixf, void, (const GLfloat *m), (m)) \
	X(glMultTransposeMatrixd, void, (const GLdouble *m), (m)) \
	X(glMultTransposeMatrixf, void, (const GLfloat *m), (m)) \
	X(glMultiDrawArrays, void, (GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount), (mode,first,count,primcount)) \
	X(glMultiDrawElements, void, (GLenum mode, const GLsizei *count, GLenum type, const GLvoid* *indices, GLsizei primcount), (mode,count,type,indices,primcount)) \
	X(glMultiTexCoord1d, void, (GLenum target, GLdouble s), (target,s)) \
	X(glMultiTexCoord1dv, void, (GLenum target, const GLdouble *v), (target,v)) \
	X(glMultiTexCoord1f, void, (GLenum target, GLfloat s), (target,s)) \
	X(glMultiTexCoord1fv, void, (GLenum target, const GLfloat *v), (target,v)) \
	X(glMultiTexCoord1i, void, (GLenum target, GLint s), (target,s)) \
	X(glMultiTexCoord1iv, void, (GLenum target, const GLint *v), (target,v)) \
	X(glMultiTexCoord1s, void, (GLenum target, GLshort s), (target,s)) \
	X(glMultiTexCoord1sv, void, (GLenum target, const GLshort *v), (target,v)) \
	X(glMultiTexCoord2d, void, (GLenum target, GLdouble s, GLdouble t), (target,s,t)) \
	X(glMultiTexCoord2dv, void, (GLenum target, const GLdouble *v), (target,v)) \
	X(glMultiTexCoord2f, void, (GLenum target, GLfloat s, GLfloat t), (target,s,t)) \
	X(glMultiTexCoord2fv, void, (GLenum target, const GLfloat *v), (target,v)) \
	X(glMultiTexCoord2i, void, (GLenum target, GLint s, GLint t), (target,s,t)) \
	X(glMultiTexCoord2iv, void, (GLenum target, const GLint *v), (target,v)) \
	X(glMultiTexCoord2s, void, (GLenum target, GLshort s, GLshort t), (target,s,t)) \
	X(glMultiTexCoord2sv, void, (GLenum target, const GLshort *v), (target,v)) \
	X(glMultiTexCoord3d, void, (GLenum target, GLdouble s, GLdouble t, GLdouble r), (target,s,t,r)) \
	X(glMultiTexCoord3dv, void, (GLenum target, const GLdouble *v), (target,v)) \
	X(glMultiTexCoord3f, void, (GLenum target, GLfloat s, GLfloat t, GLfloat r), (target,s,t,r)) \
	X(glMultiTexCoord3fv, void, (GLenum target, const GLfloat *v), (target,v)) \
	X(glMultiTexCoord3i, void, (GLenum target, GLint s, GLint t, GLint r), (target,s,t,r)) \
	X(glMultiTexCoord3iv, void, (GLenum target, const GLint *v), (target,v)) \
	X(glMultiTexCoord3s, void, (GLenum target, GLshort s, GLshort t, GLshort r), (target,s,t,r)) \
	X(glMultiTexCoord3sv, void, (GLenum target, const GLshort *v), (target,v)) \
	X(glMultiTexCoord4d, void, (GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q), (target,s,t,r,q)) \
	X(glMultiTexCoord4dv, void, (GLenum target, const GLdouble *v), (target,v)) \
	X(glMultiTexCoord4f, void, (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q), (target,s,t,r,q)) \
	X(glMultiTexCoord4fv, void, (GLenum target, const GLfloat *v), (target,v)) \
	X(glMultiTexCoord4i, void, (GLenum target, GLint s, GLint t, GLint r, GLint q), (target,s,t,r,q)) \
	X(glMultiTexCoord4iv, void, (GLenum target, const GLint *v), (target,v)) \
	X(glMultiTexCoord4s, void, (GLenum target, GLshort s, GLshort t, GLshort r, GLshort q), (target,s,t,r,q)) \
	X(glMultiTexCoord4sv, void, (GLenum target, const GLshort *v), (target,v)) \
	X(glNewList, void, (GLuint list, GLenum mode), (list,mode)) \
	X(glNormal3b, void, (GLbyte nx, GLbyte ny, GLbyte nz), (nx,ny,nz)) \
	X(glNormal3bv, void, (const GLbyte *v), (v)) \
	X(glNormal3d, void, (GLdouble nx, GLdouble ny, GLdouble nz), (nx,ny,nz)) \
	X(glNormal3dv, void, (const GLdouble *v), (v)) \
	X(glNormal3f, void, (GLfloat nx, GLfloat ny, GLfloat nz), (nx,ny,nz)) \
	X(glNormal3fv, void, (const GLfloat *v), (v)) \
	X(glNormal3i, void, (GLint nx, GLint ny, GLint nz), (nx,ny,nz)) \
	X(glNormal3iv, void, (const GLint *v), (v)) \
	X(glNormal3s, void, (GLshort nx, GLshort ny, GLshort nz), (nx,ny,nz)) \
	X(glNormal3sv, void, (const GLshort *v), (v)) \
	X(glNormalPointer, void, (GLenum type, GLsizei stride, const GLvoid *pointer), (type,stride,pointer)) \
	X(glOrtho, void, (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar), (left,right,bottom,top,zNear,zFar)) \
	X(glPassThrough, void, (GLfloat token), (token)) \
	X(glPixelMapfv, void, (GLenum map, GLint mapsize, const GLfloat *values), (map,mapsize,values)) \
	X(glPixelMapuiv, void, (GLenum map, GLint mapsize, const GLuint *values), (map,mapsize,values)) \
	X(glPixelMapusv, void, (GLenum map, GLint mapsize, const GLushort *values), (map,mapsize,values)) \
	X(glPixelStoref, void, (GLenum pname, GLfloat param), (pname,param)) \
	X(glPixelStorei, void, (GLenum pname, GLint param), (pname,param)) \
	X(glPixelTransferf, void, (GLenum pname, GLfloat param), (pname,param)) \
	X(glPixelTransferi, void, (GLenum pname, GLint param), (pname,param)) \
	X(glPixelZoom, void, (GLfloat xfactor, GLfloat yfactor), (xfactor,yfactor)) \
	X(glPointParameterf, void, (GLenum pname, GLfloat param), (pname,param)) \
	X(glPointParameterfv, void, (GLenum pname, const GLfloat *params), (pname,params)) \
	X(glPointParameteri, void, (GLenum pname, GLint param), (pname,param)) \
	X(glPointParameteriv, void, (GLenum pname, const GLint *params), (pname,params)) \
	X(glPointSize, void, (GLfloat size), (size)) \
	X(glPolygonMode, void, (GLenum face, GLenum mode), (face,mode)) \
	X(glPolygonOffset, void, (GLfloat factor, GLfloat units), (factor,units)) \
	X(glPolygonStipple, void, (const GLubyte *mask), (mask)) \
	X(glPopAttrib, void, (void), ()) \
	X(glPopClientAttrib, void, (void), ()) \
	X(glPopMatrix, void, (void), ()) \
	X(glPopName, void, (void), ()) \
	X(glPrioritizeTextures, void, (GLsizei n, const GLuint *textures, const GLclampf *priorities), (n,textures,priorities)) \
	X(glPushAttrib, void, (GLbitfield mask), (mask)) \
	X(glPushClientAttrib, void, (GLbitfield mask), (mask)) \
	X(glPushMatrix, void, (void), ()) \
	X(glPushName, void, (GLuint name), (name)) \
	X(glRasterPos2d, void, (GLdouble x, GLdouble y), (x,y)) \
	X(glRasterPos2dv, void, (const GLdouble *v), (v)) \
	X(glRasterPos2f, void, (GLfloat x, GLfloat y), (x,y)) \
	X(glRasterPos2fv, void, (const GLfloat *v), (v)) \
	X(glRasterPos2i, void, (GLint x, GLint y), (x,y)) \
	X(glRasterPos2iv, void, (const GLint *v), (v)) \
	X(glRasterPos2s, void, (GLshort x, GLshort y), (x,y)) \
	X(glRasterPos2sv, void, (const GLshort *v), (v)) \
	X(glRasterPos3d, void, (GLdouble x, GLdouble y, GLdouble z), (x,y,z)) \
	X(glRasterPos3dv, void, (const GLdouble *v), (v)) \
	X(glRasterPos3f, void, (GLfloat x, GLfloat y, GLfloat z), (x,y,z)) \
	X(glRasterPos3fv, void, (const GLfloat *v), (v)) \
	X(glRasterPos3i, void, (GLint x, GLint y, GLint z), (x,y,z)) \
	X(glRasterPos3iv, void, (const GLint *v), (v)) \
	X(glRasterPos3s, void, (GLshort x, GLshort y, GLshort z), (x,y,z)) \
	X(glRasterPos3sv, void, (const GLshort *v), (v)) \
	X(glRasterPos4d, void, (GLdouble x, GLdouble y, GLdouble z, GLdouble w), (x,y,z,w)) \
	X(glRasterPos4dv, void, (const GLdouble *v), (v)) \
	X(glRasterPos4f, void, (GLfloat x, GLfloat y, GLfloat z, GLfloat w), (x,y,z,w)) \
	X(glRasterPos4fv, void, (const GLfloat *v), (v)) \
	X(glRasterPos4i, void, (GLint x, GLint y, GLint z, GLint w), (x,y,z,w)) \
	X(glRasterPos4iv, void, (const GLint *v), (v)) \
	X(glRasterPos4s, void, (GLshort x, GLshort y, GLshort z, GLshort w), (x,y,z,w)) \
	X(glRasterPos4sv, void, (const GLshort *v), (v)) \
	X(glReadBuffer, void, (GLenum mode), (mode)) \
	X(glReadPixels, void, (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels), (x,y,width,height,format,type,pixels)) \
	X(glRectd, void, (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2), (x1,y1,x2,y2)) \
	X(glRectdv, void, (const GLdouble *v1, const GLdouble *v2), (v1,v2)) \
	X(glRectf, void, (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2), (x1,y1,x2,y2)) \
	X(glRectfv, void, (const GLfloat *v1, const GLfloat *v2), (v1,v2)) \
	X(glRecti, void, (GLint x1, GLint y1, GLint x2, GLint y2), (x1,y1,x2,y2)) \
	X(glRectiv, void, (const GLint *v1, const GLint *v2), (v1,v2)) \
	X(glRects, void, (GLshort x1, GLshort y1, GLshort x2, GLshort y2), (x1,y1,x2,y2)) \
	X(glRectsv, void, (const GLshort *v1, const GLshort *v2), (v1,v2)) \
	X(glReleaseFlushHold, GLenum, (GLuint id), (id)) \
	X(glRenderMode, GLint, (GLenum mode), (mode)) \
	X(glResetHistogram, void, (GLenum target), (target)) \
	X(glResetMinmax, void, (GLenum target), (target)) \
	X(glRotated, void, (GLdouble angle, GLdouble x, GLdouble y, GLdouble z), (angle,x,y,z)) \
	X(glRotatef, void, (GLfloat angle, GLfloat x, GLfloat y, GLfloat z), (angle,x,y,z)) \
	X(glSampleCoverage, void, (GLclampf value, GLboolean invert), (value,invert)) \
	X(glScaled, void, (GLdouble x, GLdouble y, GLdouble z), (x,y,z)) \
	X(glScalef, void, (GLfloat x, GLfloat y, GLfloat z), (x,y,z)) \
	X(glScissor, void, (GLint x, GLint y, GLsizei width, GLsizei height), (x,y,width,height)) \
	X(glSecondaryColor3b, void, (GLbyte red, GLbyte green, GLbyte blue), (red,green,blue)) \
	X(glSecondaryColor3bv, void, (const GLbyte *v), (v)) \
	X(glSecondaryColor3d, void, (GLdouble red, GLdouble green, GLdouble blue), (red,green,blue)) \
	X(glSecondaryColor3dv, void, (const GLdouble *v), (v)) \
	X(glSecondaryColor3f, void, (GLfloat red, GLfloat green, GLfloat blue), (red,green,blue)) \
	X(glSecondaryColor3fv, void, (const GLfloat *v), (v)) \
	X(glSecondaryColor3i, void, (GLint red, GLint green, GLint blue), (red,green,blue)) \
	X(glSecondaryColor3iv, void, (const GLint *v), (v)) \
	X(glSecondaryColor3s, void, (GLshort red, GLshort green, GLshort blue), (red,green,blue)) \
	X(glSecondaryColor3sv, void, (const GLshort *v), (v)) \
	X(glSecondaryColor3ub, void, (GLubyte red, GLubyte green, GLubyte blue), (red,green,blue)) \
	X(glSecondaryColor3ubv, void, (const GLubyte *v), (v)) \
	X(glSecondaryColor3ui, void, (GLuint red, GLuint green, GLuint blue), (red,green,blue)) \
	X(glSecondaryColor3uiv, void, (const GLuint *v), (v)) \
	X(glSecondaryColor3us, void, (GLushort red, GLushort green, GLushort blue), (red,green,blue)) \
	X(glSecondaryColor3usv, void, (const GLushort *v), (v)) \
	X(glSecondaryColorPointer, void, (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer), (size,type,stride,pointer)) \
	X(glSelectBuffer, void, (GLsizei size, GLuint *buffer), (size,buffer)) \
	X(glSeparableFilter2D, void, (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column), (target,internalformat,width,height,format,type,row,column)) \
	X(glShadeModel, void, (GLenum mode), (mode)) \
	X(glStencilFunc, void, (GLenum func, GLint ref, GLuint mask), (func,ref,mask)) \
	X(glStencilMask, void, (GLuint mask), (mask)) \
	X(glStencilOp, void, (GLenum fail, GLenum zfail, GLenum zpass), (fail,zfail,zpass)) \
	X(glTexCoord1d, void, (GLdouble s), (s)) \
	X(glTexCoord1dv, void, (const GLdouble *v), (v)) \
	X(glTexCoord1f, void, (GLfloat s), (s)) \
	X(glTexCoord1fv, void, (const GLfloat *v), (v)) \
	X(glTexCoord1i, void, (GLint s), (s)) \
	X(glTexCoord1iv, void, (const GLint *v), (v)) \
	X(glTexCoord1s, void, (GLshort s), (s)) \
	X(glTexCoord1sv, void, (const GLshort *v), (v)) \
	X(glTexCoord2d, void, (GLdouble s, GLdouble t), (s,t)) \
	X(glTexCoord2dv, void, (const GLdouble *v), (v)) \
	X(glTexCoord2f, void, (GLfloat s, GLfloat t), (s,t)) \
	X(glTexCoord2fv, void, (const GLfloat *v), (v)) \
	X(glTexCoord2i, void, (GLint s, GLint t), (s,t)) \
	X(glTexCoord2iv, void, (const GLint *v), (v)) \
	X(glTexCoord2s, void, (GLshort s, GLshort t), (s,t)) \
	X(glTexCoord2sv, void, (const GLshort *v), (v)) \
	X(glTexCoord3d, void, (GLdouble s, GLdouble t, GLdouble r), (s,t,r)) \
	X(glTexCoord3dv, void, (const GLdouble *v), (v)) \
	X(glTexCoord3f, void, (GLfloat s, GLfloat t, GLfloat r), (s,t,r)) \
	X(glTexCoord3fv, void, (const GLfloat *v), (v)) \
	X(glTexCoord3i, void, (GLint s, GLint t, GLint r), (s,t,r)) \
	X(glTexCoord3iv, void, (const GLint *v), (v)) \
	X(glTexCoord3s, void, (GLshort s, GLshort t, GLshort r), (s,t,r)) \
	X(glTexCoord3sv, void, (const GLshort *v), (v)) \
	X(glTexCoord4d, void, (GLdouble s, GLdouble t, GLdouble r, GLdouble q), (s,t,r,q)) \
	X(glTexCoord4dv, void, (const GLdouble *v), (v)) \
	X(glTexCoord4f, void, (GLfloat s, GLfloat t, GLfloat r, GLfloat q), (s,t,r,q)) \
	X(glTexCoord4fv, void, (const GLfloat *v), (v)) \
	X(glTexCoord4i, void, (GLint s, GLint t, GLint r, GLint q), (s,t,r,q)) \
	X(glTexCoord4iv, void, (const GLint *v), (v)) \
	X(glTexCoord4s, void, (GLshort s, GLshort t, GLshort r, GLshort q), (s,t,r,q)) \
	X(glTexCoord4sv, void, (const GLshort *v), (v)) \
	X(glTexCoordPointer, void, (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer), (size,type,stride,pointer)) \
	X(glTexEnvf, void, (GLenum target, GLenum pname, GLfloat param), (target,pname,param)) \
	X(glTexEnvfv, void, (GLenum target, GLenum pname, const GLfloat *params), (target,pname,params)) \
	X(glTexEnvi, void, (GLenum target, GLenum pname, GLint param), (target,pname,param)) \
	X(glTexEnviv, void, (GLenum target, GLenum pname, const GLint *params), (target,pname,params)) \
	X(glTexGend, void, (GLenum coord, GLenum pname, GLdouble param), (coord,pname,param)) \
	X(glTexGendv, void, (GLenum coord, GLenum pname, const GLdouble *params), (coord,pname,params)) \
	X(glTexGenf, void, (GLenum coord, GLenum pname, GLfloat param), (coord,pname,param)) \
	X(glTexGenfv, void, (GLenum coord, GLenum pname, const GLfloat *params), (coord,pname,params)) \
	X(glTexGeni, void, (GLenum coord, GLenum pname, GLint param), (coord,pname,param)) \
	X(glTexGeniv, void, (GLenum coord, GLenum pname, const GLint *params), (coord,pname,params)) \
	X(glTexImage1D, void, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels), (target,level,internalformat,width,border,format,type,pixels)) \
	X(glTexImage2D, void, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels), (target,level,internalformat,width,height,border,format,type,pixels)) \
	X(glTexImage3D, void, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels), (target,level,internalformat,width,height,depth,border,format,type,pixels)) \
	X(glTexParameterf, void, (GLenum target, GLenum pname, GLfloat param), (target,pname,param)) \
	X(glTexParameterfv, void, (GLenum target, GLenum pname, const GLfloat *params), (target,pname,params)) \
	X(glTexParameteri, void, (GLenum target, GLenum pname, GLint param), (target,pname,param)) \
	X(glTexParameteriv, void, (GLenum target, GLenum pname, const GLint *params), (target,pname,params)) \
	X(glTexSubImage1D, void, (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels), (target,level,xoffset,width,format,type,pixels)) \
	X(glTexSubImage2D, void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels), (target,level,xoffset,yoffset,width,height,format,type,pixels)) \
	X(glTexSubImage3D, void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels), (target,level,xoffset,yoffset,zoffset,width,height,depth,format,type,pixels)) \
	X(glTranslated, void, (GLdouble x, GLdouble y, GLdouble z), (x,y,z)) \
	X(glTranslatef, void, (GLfloat x, GLfloat y, GLfloat z), (x,y,z)) \
	X(glValidBackBufferHintAutodesk, GLboolean, (GLint x, GLint y, GLsizei width, GLsizei height), (x,y,width,height)) \
	X(glVertex2d, void, (GLdouble x, GLdouble y), (x,y)) \
	X(glVertex2dv, void, (const GLdouble *v), (v)) \
	X(glVertex2f, void, (GLfloat x, GLfloat y), (x,y)) \
	X(glVertex2fv, void, (const GLfloat *v), (v)) \
	X(glVertex2i, void, (GLint x, GLint y), (x,y)) \
	X(glVertex2iv, void, (const GLint *v), (v)) \
	X(glVertex2s, void, (GLshort x, GLshort y), (x,y)) \
	X(glVertex2sv, void, (const GLshort *v), (v)) \
	X(glVertex3d, void, (GLdouble x, GLdouble y, GLdouble z), (x,y,z)) \
	X(glVertex3dv, void, (const GLdouble *v), (v)) \
	X(glVertex3f, void, (GLfloat x, GLfloat y, GLfloat z), (x,y,z)) \
	X(glVertex3fv, void, (const GLfloat *v), (v)) \
	X(glVertex3i, void, (GLint x, GLint y, GLint z), (x,y,z)) \
	X(glVertex3iv, void, (const GLint *v), (v)) \
	X(glVertex3s, void, (GLshort x, GLshort y, GLshort z), (x,y,z)) \
	X(glVertex3sv, void, (const GLshort *v), (v)) \
	X(glVertex4d, void, (GLdouble x, GLdouble y, GLdouble z, GLdouble w), (x,y,z,w)) \
	X(glVertex4dv, void, (const GLdouble *v), (v)) \
	X(glVertex4f, void, (GLfloat x, GLfloat y, GLfloat z, GLfloat w), (x,y,z,w)) \
	X(glVertex4fv, void, (const GLfloat *v), (v)) \
	X(glVertex4i, void, (GLint x, GLint y, GLint z, GLint w), (x,y,z,w)) \
	X(glVertex4iv, void, (const GLint *v), (v)) \
	X(glVertex4s, void, (GLshort x, GLshort y, GLshort z, GLshort w), (x,y,z,w)) \
	X(glVertex4sv, void, (const GLshort *v), (v)) \
	X(glVertexPointer, void, (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer), (size,type,stride,pointer)) \
	X(glViewport, void, (GLint x, GLint y, GLsizei width, GLsizei height), (x,y,width,height)) \
	X(glWindowBackBufferHintAutodesk, void, (void), ()) \
	X(glWindowPos2d, void, (GLdouble x, GLdouble y), (x,y)) \
	X(glWindowPos2dv, void, (const GLdouble *p), (p)) \
	X(glWindowPos2f, void, (GLfloat x, GLfloat y), (x,y)) \
	X(glWindowPos2fv, void, (const GLfloat *p), (p)) \
	X(glWindowPos2i, void, (GLint x, GLint y), (x,y)) \
	X(glWindowPos2iv, void, (const GLint *p), (p)) \
	X(glWindowPos2s, void, (GLshort x, GLshort y), (x,y)) \
	X(glWindowPos2sv, void, (const GLshort *p), (p)) \
	X(glWindowPos3d, void, (GLdouble x, GLdouble y, GLdouble z), (x,y,z)) \
	X(glWindowPos3dv, void, (const GLdouble *p), (p)) \
	X(glWindowPos3f, void, (GLfloat x, GLfloat y, GLfloat z), (x,y,z)) \
	X(glWindowPos3fv, void, (const GLfloat *p), (p)) \
	X(glWindowPos3i, void, (GLint x, GLint y, GLint z), (x,y,z)) \
	X(glWindowPos3iv, void, (const GLint *p), (p)) \
	X(glWindowPos3s, void, (GLshort x, GLshort y, GLshort z), (x,y,z)) \
	X(glWindowPos3sv, void, (const GLshort *p), (p))
