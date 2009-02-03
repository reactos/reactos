/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/opengl32/glfuncs.h
 * PURPOSE:              OpenGL32 lib, GLFUNCS_MACRO
 * PROGRAMMER:           gen_glfuncs_macro.sh
 * UPDATE HISTORY:
 *               !!! AUTOMATICALLY CREATED FROM glfuncs.csv !!!
 */

/* To use this macro define a macro X(name, ret, typeargs, args, icdidx, tebidx, stack).
 * It gets called for every glXXX function. i.e. glVertex3f: name = "glVertex3f",
 * ret = "void", typeargs = "(GLfloat x, GLfloat y, GLfloat z)", args = "(x,y,z)",
 * icdidx = "136", tebidx = "98" and stack = "12".
 * Don't forget to undefine X ;-)
 */

#define GLFUNCS_MACRO \
    X(glAccum, void, (GLenum op, GLfloat value), (op,value),  213,  -1,  8) \
    X(glAlphaFunc, void, (GLenum func, GLclampf ref), (func,ref),  240,  -1,  8) \
    X(glAreTexturesResident, GLboolean, (GLsizei n, const GLuint *textures, GLboolean *residences), (n,textures,residences),  322,  -1,  12) \
    X(glArrayElement, void, (GLint i), (i),  306,  144,  4) \
    X(glBegin, void, (GLenum mode), (mode),  7,  2,  4) \
    X(glBindTexture, void, (GLenum target, GLuint texture), (target,texture),  307,  145,  8) \
    X(glBitmap, void, (GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap), (width,height,xorig,yorig,xmove,ymove,bitmap),  8,  -1,  28) \
    X(glBlendFunc, void, (GLenum sfactor, GLenum dfactor), (sfactor,dfactor),  241,  -1,  8) \
    X(glCallList, void, (GLuint list), (list),  2,  0,  4) \
    X(glCallLists, void, (GLsizei n, GLenum type, const GLvoid *lists), (n,type,lists),  3,  1,  12) \
    X(glClear, void, (GLbitfield mask), (mask),  203,  -1,  4) \
    X(glClearAccum, void, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha), (red,green,blue,alpha),  204,  -1,  16) \
    X(glClearColor, void, (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha), (red,green,blue,alpha),  206,  -1,  16) \
    X(glClearDepth, void, (GLclampd depth), (depth),  208,  -1,  8) \
    X(glClearIndex, void, (GLfloat c), (c),  205,  -1,  4) \
    X(glClearStencil, void, (GLint s), (s),  207,  -1,  4) \
    X(glClipPlane, void, (GLenum plane, const GLdouble *equation), (plane,equation),  150,  -1,  8) \
    X(glColor3b, void, (GLbyte red, GLbyte green, GLbyte blue), (red,green,blue),  9,  3,  12) \
    X(glColor3bv, void, (const GLbyte *v), (v),  10,  4,  4) \
    X(glColor3d, void, (GLdouble red, GLdouble green, GLdouble blue), (red,green,blue),  11,  5,  24) \
    X(glColor3dv, void, (const GLdouble *v), (v),  12,  6,  4) \
    X(glColor3f, void, (GLfloat red, GLfloat green, GLfloat blue), (red,green,blue),  13,  7,  12) \
    X(glColor3fv, void, (const GLfloat *v), (v),  14,  8,  4) \
    X(glColor3i, void, (GLint red, GLint green, GLint blue), (red,green,blue),  15,  9,  12) \
    X(glColor3iv, void, (const GLint *v), (v),  16,  10,  4) \
    X(glColor3s, void, (GLshort red, GLshort green, GLshort blue), (red,green,blue),  17,  11,  12) \
    X(glColor3sv, void, (const GLshort *v), (v),  18,  12,  4) \
    X(glColor3ub, void, (GLubyte red, GLubyte green, GLubyte blue), (red,green,blue),  19,  13,  12) \
    X(glColor3ubv, void, (const GLubyte *v), (v),  20,  14,  4) \
    X(glColor3ui, void, (GLuint red, GLuint green, GLuint blue), (red,green,blue),  21,  15,  12) \
    X(glColor3uiv, void, (const GLuint *v), (v),  22,  16,  4) \
    X(glColor3us, void, (GLushort red, GLushort green, GLushort blue), (red,green,blue),  23,  17,  12) \
    X(glColor3usv, void, (const GLushort *v), (v),  24,  18,  4) \
    X(glColor4b, void, (GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha), (red,green,blue,alpha),  25,  19,  16) \
    X(glColor4bv, void, (const GLbyte *v), (v),  26,  20,  4) \
    X(glColor4d, void, (GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha), (red,green,blue,alpha),  27,  21,  32) \
    X(glColor4dv, void, (const GLdouble *v), (v),  28,  22,  4) \
    X(glColor4f, void, (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha), (red,green,blue,alpha),  29,  23,  16) \
    X(glColor4fv, void, (const GLfloat *v), (v),  30,  24,  4) \
    X(glColor4i, void, (GLint red, GLint green, GLint blue, GLint alpha), (red,green,blue,alpha),  31,  25,  16) \
    X(glColor4iv, void, (const GLint *v), (v),  32,  26,  4) \
    X(glColor4s, void, (GLshort red, GLshort green, GLshort blue, GLshort alpha), (red,green,blue,alpha),  33,  27,  16) \
    X(glColor4sv, void, (const GLshort *v), (v),  34,  28,  4) \
    X(glColor4ub, void, (GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha), (red,green,blue,alpha),  35,  29,  16) \
    X(glColor4ubv, void, (const GLubyte *v), (v),  36,  30,  4) \
    X(glColor4ui, void, (GLuint red, GLuint green, GLuint blue, GLuint alpha), (red,green,blue,alpha),  37,  31,  16) \
    X(glColor4uiv, void, (const GLuint *v), (v),  38,  32,  4) \
    X(glColor4us, void, (GLushort red, GLushort green, GLushort blue, GLushort alpha), (red,green,blue,alpha),  39,  33,  16) \
    X(glColor4usv, void, (const GLushort *v), (v),  40,  34,  4) \
    X(glColorMask, void, (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha), (red,green,blue,alpha),  210,  -1,  16) \
    X(glColorMaterial, void, (GLenum face, GLenum mode), (face,mode),  151,  -1,  8) \
    X(glColorPointer, void, (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer), (size,type,stride,pointer),  308,  146,  16) \
    X(glCopyPixels, void, (GLint x, GLint y, GLsizei width, GLsizei height, GLenum type), (x,y,width,height,type),  255,  -1,  20) \
    X(glCopyTexImage1D, void, (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border), (target,level,internalformat,x,y,width,border),  323,  -1,  28) \
    X(glCopyTexImage2D, void, (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border), (target,level,internalformat,x,y,width,height,border),  324,  -1,  32) \
    X(glCopyTexSubImage1D, void, (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width), (target,level,xoffset,x,y,width),  325,  -1,  24) \
    X(glCopyTexSubImage2D, void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height), (target,level,xoffset,yoffset,x,y,width,height),  326,  -1,  32) \
    X(glCullFace, void, (GLenum mode), (mode),  152,  -1,  4) \
    X(glDeleteLists, void, (GLuint list, GLsizei range), (list,range),  4,  -1,  8) \
    X(glDeleteTextures, void, (GLsizei n, const GLuint *textures), (n,textures),  327,  -1,  8) \
    X(glDepthFunc, void, (GLenum func), (func),  245,  -1,  4) \
    X(glDepthMask, void, (GLboolean flag), (flag),  211,  -1,  4) \
    X(glDepthRange, void, (GLclampd zNear, GLclampd zFar), (zNear,zFar),  288,  -1,  16) \
    X(glDisable, void, (GLenum cap), (cap),  214,  116,  4) \
    X(glDisableClientState, void, (GLenum array), (array),  309,  147,  4) \
    X(glDrawArrays, void, (GLenum mode, GLint first, GLsizei count), (mode,first,count),  310,  148,  12) \
    X(glDrawBuffer, void, (GLenum mode), (mode),  202,  -1,  4) \
    X(glDrawElements, void, (GLenum mode, GLsizei count, GLenum type, const GLvoid *indices), (mode,count,type,indices),  311,  149,  16) \
    X(glDrawPixels, void, (GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels), (width,height,format,type,pixels),  257,  -1,  20) \
    X(glEdgeFlag, void, (GLboolean flag), (flag),  41,  35,  4) \
    X(glEdgeFlagPointer, void, (GLsizei stride, const GLboolean *pointer), (stride,pointer),  312,  150,  8) \
    X(glEdgeFlagv, void, (const GLboolean *flag), (flag),  42,  36,  4) \
    X(glEnable, void, (GLenum cap), (cap),  215,  117,  4) \
    X(glEnableClientState, void, (GLenum array), (array),  313,  151,  4) \
    X(glEnd, void, (void), (),  43,  37,  0) \
    X(glEndList, void, (void), (),  1,  -1,  0) \
    X(glEvalCoord1d, void, (GLdouble u), (u),  228,  120,  8) \
    X(glEvalCoord1dv, void, (const GLdouble *u), (u),  229,  121,  4) \
    X(glEvalCoord1f, void, (GLfloat u), (u),  230,  122,  4) \
    X(glEvalCoord1fv, void, (const GLfloat *u), (u),  231,  123,  4) \
    X(glEvalCoord2d, void, (GLdouble u, GLdouble v), (u,v),  232,  124,  16) \
    X(glEvalCoord2dv, void, (const GLdouble *u), (u),  233,  125,  4) \
    X(glEvalCoord2f, void, (GLfloat u, GLfloat v), (u,v),  234,  126,  8) \
    X(glEvalCoord2fv, void, (const GLfloat *u), (u),  235,  127,  4) \
    X(glEvalMesh1, void, (GLenum mode, GLint i1, GLint i2), (mode,i1,i2),  236,  -1,  12) \
    X(glEvalMesh2, void, (GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2), (mode,i1,i2,j1,j2),  238,  -1,  20) \
    X(glEvalPoint1, void, (GLint i), (i),  237,  128,  4) \
    X(glEvalPoint2, void, (GLint i, GLint j), (i,j),  239,  129,  8) \
    X(glFeedbackBuffer, void, (GLsizei size, GLenum type, GLfloat *buffer), (size,type,buffer),  194,  -1,  12) \
    X(glFinish, void, (void), (),  216,  -1,  0) \
    X(glFlush, void, (void), (),  217,  -1,  0) \
    X(glFogf, void, (GLenum pname, GLfloat param), (pname,param),  153,  -1,  8) \
    X(glFogfv, void, (GLenum pname, const GLfloat *params), (pname,params),  154,  -1,  8) \
    X(glFogi, void, (GLenum pname, GLint param), (pname,param),  155,  -1,  8) \
    X(glFogiv, void, (GLenum pname, const GLint *params), (pname,params),  156,  -1,  8) \
    X(glFrontFace, void, (GLenum mode), (mode),  157,  -1,  4) \
    X(glFrustum, void, (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar), (left,right,bottom,top,zNear,zFar),  289,  -1,  48) \
    X(glGenLists, GLuint, (GLsizei range), (range),  5,  -1,  4) \
    X(glGenTextures, void, (GLsizei n, GLuint *textures), (n,textures),  328,  -1,  8) \
    X(glGetBooleanv, void, (GLenum pname, GLboolean *params), (pname,params),  258,  -1,  8) \
    X(glGetClipPlane, void, (GLenum plane, GLdouble *equation), (plane,equation),  259,  -1,  8) \
    X(glGetDoublev, void, (GLenum pname, GLdouble *params), (pname,params),  260,  -1,  8) \
    X(glGetError, GLenum, (void), (),  261,  -1,  0) \
    X(glGetFloatv, void, (GLenum pname, GLfloat *params), (pname,params),  262,  -1,  8) \
    X(glGetIntegerv, void, (GLenum pname, GLint *params), (pname,params),  263,  -1,  8) \
    X(glGetLightfv, void, (GLenum light, GLenum pname, GLfloat *params), (light,pname,params),  264,  -1,  12) \
    X(glGetLightiv, void, (GLenum light, GLenum pname, GLint *params), (light,pname,params),  265,  -1,  12) \
    X(glGetMapdv, void, (GLenum target, GLenum query, GLdouble *v), (target,query,v),  266,  -1,  12) \
    X(glGetMapfv, void, (GLenum target, GLenum query, GLfloat *v), (target,query,v),  267,  -1,  12) \
    X(glGetMapiv, void, (GLenum target, GLenum query, GLint *v), (target,query,v),  268,  -1,  12) \
    X(glGetMaterialfv, void, (GLenum face, GLenum pname, GLfloat *params), (face,pname,params),  269,  -1,  12) \
    X(glGetMaterialiv, void, (GLenum face, GLenum pname, GLint *params), (face,pname,params),  270,  -1,  12) \
    X(glGetPixelMapfv, void, (GLenum map, GLfloat *values), (map,values),  271,  -1,  8) \
    X(glGetPixelMapuiv, void, (GLenum map, GLuint *values), (map,values),  272,  -1,  8) \
    X(glGetPixelMapusv, void, (GLenum map, GLushort *values), (map,values),  273,  -1,  8) \
    X(glGetPointerv, void, (GLenum pname, GLvoid* *params), (pname,params),  329,  160,  8) \
    X(glGetPolygonStipple, void, (GLubyte *mask), (mask),  274,  -1,  4) \
    X(glGetString, const GLubyte *, (GLenum name), (name),  275,  -1,  4) \
    X(glGetTexEnvfv, void, (GLenum target, GLenum pname, GLfloat *params), (target,pname,params),  276,  -1,  12) \
    X(glGetTexEnviv, void, (GLenum target, GLenum pname, GLint *params), (target,pname,params),  277,  -1,  12) \
    X(glGetTexGendv, void, (GLenum coord, GLenum pname, GLdouble *params), (coord,pname,params),  278,  -1,  12) \
    X(glGetTexGenfv, void, (GLenum coord, GLenum pname, GLfloat *params), (coord,pname,params),  279,  -1,  12) \
    X(glGetTexGeniv, void, (GLenum coord, GLenum pname, GLint *params), (coord,pname,params),  280,  -1,  12) \
    X(glGetTexImage, void, (GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels), (target,level,format,type,pixels),  281,  -1,  20) \
    X(glGetTexLevelParameterfv, void, (GLenum target, GLint level, GLenum pname, GLfloat *params), (target,level,pname,params),  284,  -1,  16) \
    X(glGetTexLevelParameteriv, void, (GLenum target, GLint level, GLenum pname, GLint *params), (target,level,pname,params),  285,  -1,  16) \
    X(glGetTexParameterfv, void, (GLenum target, GLenum pname, GLfloat *params), (target,pname,params),  282,  -1,  12) \
    X(glGetTexParameteriv, void, (GLenum target, GLenum pname, GLint *params), (target,pname,params),  283,  -1,  12) \
    X(glHint, void, (GLenum target, GLenum mode), (target,mode),  158,  -1,  8) \
    X(glIndexMask, void, (GLuint mask), (mask),  212,  -1,  4) \
    X(glIndexPointer, void, (GLenum type, GLsizei stride, const GLvoid *pointer), (type,stride,pointer),  314,  152,  12) \
    X(glIndexd, void, (GLdouble c), (c),  44,  38,  8) \
    X(glIndexdv, void, (const GLdouble *c), (c),  45,  39,  4) \
    X(glIndexf, void, (GLfloat c), (c),  46,  40,  4) \
    X(glIndexfv, void, (const GLfloat *c), (c),  47,  41,  4) \
    X(glIndexi, void, (GLint c), (c),  48,  42,  4) \
    X(glIndexiv, void, (const GLint *c), (c),  49,  43,  4) \
    X(glIndexs, void, (GLshort c), (c),  50,  44,  4) \
    X(glIndexsv, void, (const GLshort *c), (c),  51,  45,  4) \
    X(glIndexub, void, (GLubyte c), (c),  315,  153,  4) \
    X(glIndexubv, void, (const GLubyte *c), (c),  316,  154,  4) \
    X(glInitNames, void, (void), (),  197,  -1,  0) \
    X(glInterleavedArrays, void, (GLenum format, GLsizei stride, const GLvoid *pointer), (format,stride,pointer),  317,  155,  12) \
    X(glIsEnabled, GLboolean, (GLenum cap), (cap),  286,  -1,  4) \
    X(glIsList, GLboolean, (GLuint list), (list),  287,  -1,  4) \
    X(glIsTexture, GLboolean, (GLuint texture), (texture),  330,  -1,  4) \
    X(glLightModelf, void, (GLenum pname, GLfloat param), (pname,param),  163,  -1,  8) \
    X(glLightModelfv, void, (GLenum pname, const GLfloat *params), (pname,params),  164,  -1,  8) \
    X(glLightModeli, void, (GLenum pname, GLint param), (pname,param),  165,  -1,  8) \
    X(glLightModeliv, void, (GLenum pname, const GLint *params), (pname,params),  166,  -1,  8) \
    X(glLightf, void, (GLenum light, GLenum pname, GLfloat param), (light,pname,param),  159,  -1,  12) \
    X(glLightfv, void, (GLenum light, GLenum pname, const GLfloat *params), (light,pname,params),  160,  -1,  12) \
    X(glLighti, void, (GLenum light, GLenum pname, GLint param), (light,pname,param),  161,  -1,  12) \
    X(glLightiv, void, (GLenum light, GLenum pname, const GLint *params), (light,pname,params),  162,  -1,  12) \
    X(glLineStipple, void, (GLint factor, GLushort pattern), (factor,pattern),  167,  -1,  8) \
    X(glLineWidth, void, (GLfloat width), (width),  168,  -1,  4) \
    X(glListBase, void, (GLuint base), (base),  6,  -1,  4) \
    X(glLoadIdentity, void, (void), (),  290,  130,  0) \
    X(glLoadMatrixd, void, (const GLdouble *m), (m),  292,  132,  4) \
    X(glLoadMatrixf, void, (const GLfloat *m), (m),  291,  131,  4) \
    X(glLoadName, void, (GLuint name), (name),  198,  -1,  4) \
    X(glLogicOp, void, (GLenum opcode), (opcode),  242,  -1,  4) \
    X(glMap1d, void, (GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points), (target,u1,u2,stride,order,points),  220,  -1,  32) \
    X(glMap1f, void, (GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points), (target,u1,u2,stride,order,points),  221,  -1,  24) \
    X(glMap2d, void, (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points), (target,u1,u2,ustride,uorder,v1,v2,vstride,vorder,points),  222,  -1,  56) \
    X(glMap2f, void, (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points), (target,u1,u2,ustride,uorder,v1,v2,vstride,vorder,points),  223,  -1,  40) \
    X(glMapGrid1d, void, (GLint un, GLdouble u1, GLdouble u2), (un,u1,u2),  224,  -1,  20) \
    X(glMapGrid1f, void, (GLint un, GLfloat u1, GLfloat u2), (un,u1,u2),  225,  -1,  12) \
    X(glMapGrid2d, void, (GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2), (un,u1,u2,vn,v1,v2),  226,  -1,  40) \
    X(glMapGrid2f, void, (GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2), (un,u1,u2,vn,v1,v2),  227,  -1,  24) \
    X(glMaterialf, void, (GLenum face, GLenum pname, GLfloat param), (face,pname,param),  169,  112,  12) \
    X(glMaterialfv, void, (GLenum face, GLenum pname, const GLfloat *params), (face,pname,params),  170,  113,  12) \
    X(glMateriali, void, (GLenum face, GLenum pname, GLint param), (face,pname,param),  171,  114,  12) \
    X(glMaterialiv, void, (GLenum face, GLenum pname, const GLint *params), (face,pname,params),  172,  115,  12) \
    X(glMatrixMode, void, (GLenum mode), (mode),  293,  133,  4) \
    X(glMultMatrixd, void, (const GLdouble *m), (m),  295,  135,  4) \
    X(glMultMatrixf, void, (const GLfloat *m), (m),  294,  134,  4) \
    X(glNewList, void, (GLuint list, GLenum mode), (list,mode),  0,  -1,  8) \
    X(glNormal3b, void, (GLbyte nx, GLbyte ny, GLbyte nz), (nx,ny,nz),  52,  46,  12) \
    X(glNormal3bv, void, (const GLbyte *v), (v),  53,  47,  4) \
    X(glNormal3d, void, (GLdouble nx, GLdouble ny, GLdouble nz), (nx,ny,nz),  54,  48,  24) \
    X(glNormal3dv, void, (const GLdouble *v), (v),  55,  49,  4) \
    X(glNormal3f, void, (GLfloat nx, GLfloat ny, GLfloat nz), (nx,ny,nz),  56,  50,  12) \
    X(glNormal3fv, void, (const GLfloat *v), (v),  57,  51,  4) \
    X(glNormal3i, void, (GLint nx, GLint ny, GLint nz), (nx,ny,nz),  58,  52,  12) \
    X(glNormal3iv, void, (const GLint *v), (v),  59,  53,  4) \
    X(glNormal3s, void, (GLshort nx, GLshort ny, GLshort nz), (nx,ny,nz),  60,  54,  12) \
    X(glNormal3sv, void, (const GLshort *v), (v),  61,  55,  4) \
    X(glNormalPointer, void, (GLenum type, GLsizei stride, const GLvoid *pointer), (type,stride,pointer),  318,  156,  12) \
    X(glOrtho, void, (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar), (left,right,bottom,top,zNear,zFar),  296,  -1,  48) \
    X(glPassThrough, void, (GLfloat token), (token),  199,  -1,  4) \
    X(glPixelMapfv, void, (GLenum map, GLint mapsize, const GLfloat *values), (map,mapsize,values),  251,  -1,  12) \
    X(glPixelMapuiv, void, (GLenum map, GLint mapsize, const GLuint *values), (map,mapsize,values),  252,  -1,  12) \
    X(glPixelMapusv, void, (GLenum map, GLint mapsize, const GLushort *values), (map,mapsize,values),  253,  -1,  12) \
    X(glPixelStoref, void, (GLenum pname, GLfloat param), (pname,param),  249,  -1,  8) \
    X(glPixelStorei, void, (GLenum pname, GLint param), (pname,param),  250,  -1,  8) \
    X(glPixelTransferf, void, (GLenum pname, GLfloat param), (pname,param),  247,  -1,  8) \
    X(glPixelTransferi, void, (GLenum pname, GLint param), (pname,param),  248,  -1,  8) \
    X(glPixelZoom, void, (GLfloat xfactor, GLfloat yfactor), (xfactor,yfactor),  246,  -1,  8) \
    X(glPointSize, void, (GLfloat size), (size),  173,  -1,  4) \
    X(glPolygonMode, void, (GLenum face, GLenum mode), (face,mode),  174,  -1,  8) \
    X(glPolygonOffset, void, (GLfloat factor, GLfloat units), (factor,units),  319,  157,  8) \
    X(glPolygonStipple, void, (const GLubyte *mask), (mask),  175,  -1,  4) \
    X(glPopAttrib, void, (void), (),  218,  118,  0) \
    X(glPopClientAttrib, void, (void), (),  334,  161,  0) \
    X(glPopMatrix, void, (void), (),  297,  136,  0) \
    X(glPopName, void, (void), (),  200,  -1,  0) \
    X(glPrioritizeTextures, void, (GLsizei n, const GLuint *textures, const GLclampf *priorities), (n,textures,priorities),  331,  -1,  12) \
    X(glPushAttrib, void, (GLbitfield mask), (mask),  219,  119,  4) \
    X(glPushClientAttrib, void, (GLbitfield mask), (mask),  335,  162,  4) \
    X(glPushMatrix, void, (void), (),  298,  137,  0) \
    X(glPushName, void, (GLuint name), (name),  201,  -1,  4) \
    X(glRasterPos2d, void, (GLdouble x, GLdouble y), (x,y),  62,  -1,  16) \
    X(glRasterPos2dv, void, (const GLdouble *v), (v),  63,  -1,  4) \
    X(glRasterPos2f, void, (GLfloat x, GLfloat y), (x,y),  64,  -1,  8) \
    X(glRasterPos2fv, void, (const GLfloat *v), (v),  65,  -1,  4) \
    X(glRasterPos2i, void, (GLint x, GLint y), (x,y),  66,  -1,  8) \
    X(glRasterPos2iv, void, (const GLint *v), (v),  67,  -1,  4) \
    X(glRasterPos2s, void, (GLshort x, GLshort y), (x,y),  68,  -1,  8) \
    X(glRasterPos2sv, void, (const GLshort *v), (v),  69,  -1,  4) \
    X(glRasterPos3d, void, (GLdouble x, GLdouble y, GLdouble z), (x,y,z),  70,  -1,  24) \
    X(glRasterPos3dv, void, (const GLdouble *v), (v),  71,  -1,  4) \
    X(glRasterPos3f, void, (GLfloat x, GLfloat y, GLfloat z), (x,y,z),  72,  -1,  12) \
    X(glRasterPos3fv, void, (const GLfloat *v), (v),  73,  -1,  4) \
    X(glRasterPos3i, void, (GLint x, GLint y, GLint z), (x,y,z),  74,  -1,  12) \
    X(glRasterPos3iv, void, (const GLint *v), (v),  75,  -1,  4) \
    X(glRasterPos3s, void, (GLshort x, GLshort y, GLshort z), (x,y,z),  76,  -1,  12) \
    X(glRasterPos3sv, void, (const GLshort *v), (v),  77,  -1,  4) \
    X(glRasterPos4d, void, (GLdouble x, GLdouble y, GLdouble z, GLdouble w), (x,y,z,w),  78,  -1,  32) \
    X(glRasterPos4dv, void, (const GLdouble *v), (v),  79,  -1,  4) \
    X(glRasterPos4f, void, (GLfloat x, GLfloat y, GLfloat z, GLfloat w), (x,y,z,w),  80,  -1,  16) \
    X(glRasterPos4fv, void, (const GLfloat *v), (v),  81,  -1,  4) \
    X(glRasterPos4i, void, (GLint x, GLint y, GLint z, GLint w), (x,y,z,w),  82,  -1,  16) \
    X(glRasterPos4iv, void, (const GLint *v), (v),  83,  -1,  4) \
    X(glRasterPos4s, void, (GLshort x, GLshort y, GLshort z, GLshort w), (x,y,z,w),  84,  -1,  16) \
    X(glRasterPos4sv, void, (const GLshort *v), (v),  85,  -1,  4) \
    X(glReadBuffer, void, (GLenum mode), (mode),  254,  -1,  4) \
    X(glReadPixels, void, (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels), (x,y,width,height,format,type,pixels),  256,  -1,  28) \
    X(glRectd, void, (GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2), (x1,y1,x2,y2),  86,  -1,  32) \
    X(glRectdv, void, (const GLdouble *v1, const GLdouble *v2), (v1,v2),  87,  -1,  8) \
    X(glRectf, void, (GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2), (x1,y1,x2,y2),  88,  -1,  16) \
    X(glRectfv, void, (const GLfloat *v1, const GLfloat *v2), (v1,v2),  89,  -1,  8) \
    X(glRecti, void, (GLint x1, GLint y1, GLint x2, GLint y2), (x1,y1,x2,y2),  90,  -1,  16) \
    X(glRectiv, void, (const GLint *v1, const GLint *v2), (v1,v2),  91,  -1,  8) \
    X(glRects, void, (GLshort x1, GLshort y1, GLshort x2, GLshort y2), (x1,y1,x2,y2),  92,  -1,  16) \
    X(glRectsv, void, (const GLshort *v1, const GLshort *v2), (v1,v2),  93,  -1,  8) \
    X(glRenderMode, GLint, (GLenum mode), (mode),  196,  -1,  4) \
    X(glRotated, void, (GLdouble angle, GLdouble x, GLdouble y, GLdouble z), (angle,x,y,z),  299,  138,  32) \
    X(glRotatef, void, (GLfloat angle, GLfloat x, GLfloat y, GLfloat z), (angle,x,y,z),  300,  139,  16) \
    X(glScaled, void, (GLdouble x, GLdouble y, GLdouble z), (x,y,z),  301,  140,  24) \
    X(glScalef, void, (GLfloat x, GLfloat y, GLfloat z), (x,y,z),  302,  141,  12) \
    X(glScissor, void, (GLint x, GLint y, GLsizei width, GLsizei height), (x,y,width,height),  176,  -1,  16) \
    X(glSelectBuffer, void, (GLsizei size, GLuint *buffer), (size,buffer),  195,  -1,  8) \
    X(glShadeModel, void, (GLenum mode), (mode),  177,  -1,  4) \
    X(glStencilFunc, void, (GLenum func, GLint ref, GLuint mask), (func,ref,mask),  243,  -1,  12) \
    X(glStencilMask, void, (GLuint mask), (mask),  209,  -1,  4) \
    X(glStencilOp, void, (GLenum fail, GLenum zfail, GLenum zpass), (fail,zfail,zpass),  244,  -1,  12) \
    X(glTexCoord1d, void, (GLdouble s), (s),  94,  56,  8) \
    X(glTexCoord1dv, void, (const GLdouble *v), (v),  95,  57,  4) \
    X(glTexCoord1f, void, (GLfloat s), (s),  96,  58,  4) \
    X(glTexCoord1fv, void, (const GLfloat *v), (v),  97,  59,  4) \
    X(glTexCoord1i, void, (GLint s), (s),  98,  60,  4) \
    X(glTexCoord1iv, void, (const GLint *v), (v),  99,  61,  4) \
    X(glTexCoord1s, void, (GLshort s), (s),  100,  62,  4) \
    X(glTexCoord1sv, void, (const GLshort *v), (v),  101,  63,  4) \
    X(glTexCoord2d, void, (GLdouble s, GLdouble t), (s,t),  102,  64,  16) \
    X(glTexCoord2dv, void, (const GLdouble *v), (v),  103,  65,  4) \
    X(glTexCoord2f, void, (GLfloat s, GLfloat t), (s,t),  104,  66,  8) \
    X(glTexCoord2fv, void, (const GLfloat *v), (v),  105,  67,  4) \
    X(glTexCoord2i, void, (GLint s, GLint t), (s,t),  106,  68,  8) \
    X(glTexCoord2iv, void, (const GLint *v), (v),  107,  69,  4) \
    X(glTexCoord2s, void, (GLshort s, GLshort t), (s,t),  108,  70,  8) \
    X(glTexCoord2sv, void, (const GLshort *v), (v),  109,  71,  4) \
    X(glTexCoord3d, void, (GLdouble s, GLdouble t, GLdouble r), (s,t,r),  110,  72,  24) \
    X(glTexCoord3dv, void, (const GLdouble *v), (v),  111,  73,  4) \
    X(glTexCoord3f, void, (GLfloat s, GLfloat t, GLfloat r), (s,t,r),  112,  74,  12) \
    X(glTexCoord3fv, void, (const GLfloat *v), (v),  113,  75,  4) \
    X(glTexCoord3i, void, (GLint s, GLint t, GLint r), (s,t,r),  114,  76,  12) \
    X(glTexCoord3iv, void, (const GLint *v), (v),  115,  77,  4) \
    X(glTexCoord3s, void, (GLshort s, GLshort t, GLshort r), (s,t,r),  116,  78,  12) \
    X(glTexCoord3sv, void, (const GLshort *v), (v),  117,  79,  4) \
    X(glTexCoord4d, void, (GLdouble s, GLdouble t, GLdouble r, GLdouble q), (s,t,r,q),  118,  80,  32) \
    X(glTexCoord4dv, void, (const GLdouble *v), (v),  119,  81,  4) \
    X(glTexCoord4f, void, (GLfloat s, GLfloat t, GLfloat r, GLfloat q), (s,t,r,q),  120,  82,  16) \
    X(glTexCoord4fv, void, (const GLfloat *v), (v),  121,  83,  4) \
    X(glTexCoord4i, void, (GLint s, GLint t, GLint r, GLint q), (s,t,r,q),  122,  84,  16) \
    X(glTexCoord4iv, void, (const GLint *v), (v),  123,  85,  4) \
    X(glTexCoord4s, void, (GLshort s, GLshort t, GLshort r, GLshort q), (s,t,r,q),  124,  86,  16) \
    X(glTexCoord4sv, void, (const GLshort *v), (v),  125,  87,  4) \
    X(glTexCoordPointer, void, (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer), (size,type,stride,pointer),  320,  158,  16) \
    X(glTexEnvf, void, (GLenum target, GLenum pname, GLfloat param), (target,pname,param),  184,  -1,  12) \
    X(glTexEnvfv, void, (GLenum target, GLenum pname, const GLfloat *params), (target,pname,params),  185,  -1,  12) \
    X(glTexEnvi, void, (GLenum target, GLenum pname, GLint param), (target,pname,param),  186,  -1,  12) \
    X(glTexEnviv, void, (GLenum target, GLenum pname, const GLint *params), (target,pname,params),  187,  -1,  12) \
    X(glTexGend, void, (GLenum coord, GLenum pname, GLdouble param), (coord,pname,param),  188,  -1,  16) \
    X(glTexGendv, void, (GLenum coord, GLenum pname, const GLdouble *params), (coord,pname,params),  189,  -1,  12) \
    X(glTexGenf, void, (GLenum coord, GLenum pname, GLfloat param), (coord,pname,param),  190,  -1,  12) \
    X(glTexGenfv, void, (GLenum coord, GLenum pname, const GLfloat *params), (coord,pname,params),  191,  -1,  12) \
    X(glTexGeni, void, (GLenum coord, GLenum pname, GLint param), (coord,pname,param),  192,  -1,  12) \
    X(glTexGeniv, void, (GLenum coord, GLenum pname, const GLint *params), (coord,pname,params),  193,  -1,  12) \
    X(glTexImage1D, void, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels), (target,level,internalformat,width,border,format,type,pixels),  182,  -1,  32) \
    X(glTexImage2D, void, (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels), (target,level,internalformat,width,height,border,format,type,pixels),  183,  -1,  36) \
    X(glTexParameterf, void, (GLenum target, GLenum pname, GLfloat param), (target,pname,param),  178,  -1,  12) \
    X(glTexParameterfv, void, (GLenum target, GLenum pname, const GLfloat *params), (target,pname,params),  179,  -1,  12) \
    X(glTexParameteri, void, (GLenum target, GLenum pname, GLint param), (target,pname,param),  180,  -1,  12) \
    X(glTexParameteriv, void, (GLenum target, GLenum pname, const GLint *params), (target,pname,params),  181,  -1,  12) \
    X(glTexSubImage1D, void, (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels), (target,level,xoffset,width,format,type,pixels),  332,  -1,  28) \
    X(glTexSubImage2D, void, (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels), (target,level,xoffset,yoffset,width,height,format,type,pixels),  333,  -1,  36) \
    X(glTranslated, void, (GLdouble x, GLdouble y, GLdouble z), (x,y,z),  303,  142,  24) \
    X(glTranslatef, void, (GLfloat x, GLfloat y, GLfloat z), (x,y,z),  304,  143,  12) \
    X(glVertex2d, void, (GLdouble x, GLdouble y), (x,y),  126,  88,  16) \
    X(glVertex2dv, void, (const GLdouble *v), (v),  127,  89,  4) \
    X(glVertex2f, void, (GLfloat x, GLfloat y), (x,y),  128,  90,  8) \
    X(glVertex2fv, void, (const GLfloat *v), (v),  129,  91,  4) \
    X(glVertex2i, void, (GLint x, GLint y), (x,y),  130,  92,  8) \
    X(glVertex2iv, void, (const GLint *v), (v),  131,  93,  4) \
    X(glVertex2s, void, (GLshort x, GLshort y), (x,y),  132,  94,  8) \
    X(glVertex2sv, void, (const GLshort *v), (v),  133,  95,  4) \
    X(glVertex3d, void, (GLdouble x, GLdouble y, GLdouble z), (x,y,z),  134,  96,  24) \
    X(glVertex3dv, void, (const GLdouble *v), (v),  135,  97,  4) \
    X(glVertex3f, void, (GLfloat x, GLfloat y, GLfloat z), (x,y,z),  136,  98,  12) \
    X(glVertex3fv, void, (const GLfloat *v), (v),  137,  99,  4) \
    X(glVertex3i, void, (GLint x, GLint y, GLint z), (x,y,z),  138,  100,  12) \
    X(glVertex3iv, void, (const GLint *v), (v),  139,  101,  4) \
    X(glVertex3s, void, (GLshort x, GLshort y, GLshort z), (x,y,z),  140,  102,  12) \
    X(glVertex3sv, void, (const GLshort *v), (v),  141,  103,  4) \
    X(glVertex4d, void, (GLdouble x, GLdouble y, GLdouble z, GLdouble w), (x,y,z,w),  142,  104,  32) \
    X(glVertex4dv, void, (const GLdouble *v), (v),  143,  105,  4) \
    X(glVertex4f, void, (GLfloat x, GLfloat y, GLfloat z, GLfloat w), (x,y,z,w),  144,  106,  16) \
    X(glVertex4fv, void, (const GLfloat *v), (v),  145,  107,  4) \
    X(glVertex4i, void, (GLint x, GLint y, GLint z, GLint w), (x,y,z,w),  146,  108,  16) \
    X(glVertex4iv, void, (const GLint *v), (v),  147,  109,  4) \
    X(glVertex4s, void, (GLshort x, GLshort y, GLshort z, GLshort w), (x,y,z,w),  148,  110,  16) \
    X(glVertex4sv, void, (const GLshort *v), (v),  149,  111,  4) \
    X(glVertexPointer, void, (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer), (size,type,stride,pointer),  321,  159,  16) \
    X(glViewport, void, (GLint x, GLint y, GLsizei width, GLsizei height), (x,y,width,height),  305,  -1,  16) \


/* EOF */
