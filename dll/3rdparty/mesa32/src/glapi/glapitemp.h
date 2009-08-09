/* DO NOT EDIT - This file generated automatically by gl_apitemp.py (from Mesa) script */

/*
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 * (C) Copyright IBM Corporation 2004
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL, IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#  if (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)) && defined(__ELF__)
#    define HIDDEN  __attribute__((visibility("hidden")))
#  else
#    define HIDDEN
#  endif

/*
 * This file is a template which generates the OpenGL API entry point
 * functions.  It should be included by a .c file which first defines
 * the following macros:
 *   KEYWORD1 - usually nothing, but might be __declspec(dllexport) on Win32
 *   KEYWORD2 - usually nothing, but might be __stdcall on Win32
 *   NAME(n)  - builds the final function name (usually add "gl" prefix)
 *   DISPATCH(func, args, msg) - code to do dispatch of named function.
 *                               msg is a printf-style debug message.
 *   RETURN_DISPATCH(func, args, msg) - code to do dispatch with a return value
 *
 * Here is an example which generates the usual OpenGL functions:
 *   #define KEYWORD1
 *   #define KEYWORD2
 *   #define NAME(func)  gl##func
 *   #define DISPATCH(func, args, msg)                           \
 *          struct _glapi_table *dispatch = CurrentDispatch;     \
 *          (*dispatch->func) args
 *   #define RETURN DISPATCH(func, args, msg)                    \
 *          struct _glapi_table *dispatch = CurrentDispatch;     \
 *          return (*dispatch->func) args
 *
 */


#if defined( NAME )
#ifndef KEYWORD1
#define KEYWORD1
#endif

#ifndef KEYWORD1_ALT
#define KEYWORD1_ALT HIDDEN
#endif

#ifndef KEYWORD2
#define KEYWORD2
#endif

#ifndef DISPATCH
#error DISPATCH must be defined
#endif

#ifndef RETURN_DISPATCH
#error RETURN_DISPATCH must be defined
#endif


KEYWORD1 void KEYWORD2 NAME(NewList)(GLuint list, GLenum mode)
{
   DISPATCH(NewList, (list, mode), (F, "glNewList(%d, 0x%x);\n", list, mode));
}

KEYWORD1 void KEYWORD2 NAME(EndList)(void)
{
   DISPATCH(EndList, (), (F, "glEndList();\n"));
}

KEYWORD1 void KEYWORD2 NAME(CallList)(GLuint list)
{
   DISPATCH(CallList, (list), (F, "glCallList(%d);\n", list));
}

KEYWORD1 void KEYWORD2 NAME(CallLists)(GLsizei n, GLenum type, const GLvoid * lists)
{
   DISPATCH(CallLists, (n, type, lists), (F, "glCallLists(%d, 0x%x, %p);\n", n, type, (const void *) lists));
}

KEYWORD1 void KEYWORD2 NAME(DeleteLists)(GLuint list, GLsizei range)
{
   DISPATCH(DeleteLists, (list, range), (F, "glDeleteLists(%d, %d);\n", list, range));
}

KEYWORD1 GLuint KEYWORD2 NAME(GenLists)(GLsizei range)
{
   RETURN_DISPATCH(GenLists, (range), (F, "glGenLists(%d);\n", range));
}

KEYWORD1 void KEYWORD2 NAME(ListBase)(GLuint base)
{
   DISPATCH(ListBase, (base), (F, "glListBase(%d);\n", base));
}

KEYWORD1 void KEYWORD2 NAME(Begin)(GLenum mode)
{
   DISPATCH(Begin, (mode), (F, "glBegin(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(Bitmap)(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte * bitmap)
{
   DISPATCH(Bitmap, (width, height, xorig, yorig, xmove, ymove, bitmap), (F, "glBitmap(%d, %d, %f, %f, %f, %f, %p);\n", width, height, xorig, yorig, xmove, ymove, (const void *) bitmap));
}

KEYWORD1 void KEYWORD2 NAME(Color3b)(GLbyte red, GLbyte green, GLbyte blue)
{
   DISPATCH(Color3b, (red, green, blue), (F, "glColor3b(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color3bv)(const GLbyte * v)
{
   DISPATCH(Color3bv, (v), (F, "glColor3bv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color3d)(GLdouble red, GLdouble green, GLdouble blue)
{
   DISPATCH(Color3d, (red, green, blue), (F, "glColor3d(%f, %f, %f);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color3dv)(const GLdouble * v)
{
   DISPATCH(Color3dv, (v), (F, "glColor3dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color3f)(GLfloat red, GLfloat green, GLfloat blue)
{
   DISPATCH(Color3f, (red, green, blue), (F, "glColor3f(%f, %f, %f);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color3fv)(const GLfloat * v)
{
   DISPATCH(Color3fv, (v), (F, "glColor3fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color3i)(GLint red, GLint green, GLint blue)
{
   DISPATCH(Color3i, (red, green, blue), (F, "glColor3i(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color3iv)(const GLint * v)
{
   DISPATCH(Color3iv, (v), (F, "glColor3iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color3s)(GLshort red, GLshort green, GLshort blue)
{
   DISPATCH(Color3s, (red, green, blue), (F, "glColor3s(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color3sv)(const GLshort * v)
{
   DISPATCH(Color3sv, (v), (F, "glColor3sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color3ub)(GLubyte red, GLubyte green, GLubyte blue)
{
   DISPATCH(Color3ub, (red, green, blue), (F, "glColor3ub(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color3ubv)(const GLubyte * v)
{
   DISPATCH(Color3ubv, (v), (F, "glColor3ubv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color3ui)(GLuint red, GLuint green, GLuint blue)
{
   DISPATCH(Color3ui, (red, green, blue), (F, "glColor3ui(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color3uiv)(const GLuint * v)
{
   DISPATCH(Color3uiv, (v), (F, "glColor3uiv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color3us)(GLushort red, GLushort green, GLushort blue)
{
   DISPATCH(Color3us, (red, green, blue), (F, "glColor3us(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color3usv)(const GLushort * v)
{
   DISPATCH(Color3usv, (v), (F, "glColor3usv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color4b)(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
   DISPATCH(Color4b, (red, green, blue, alpha), (F, "glColor4b(%d, %d, %d, %d);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(Color4bv)(const GLbyte * v)
{
   DISPATCH(Color4bv, (v), (F, "glColor4bv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color4d)(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
   DISPATCH(Color4d, (red, green, blue, alpha), (F, "glColor4d(%f, %f, %f, %f);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(Color4dv)(const GLdouble * v)
{
   DISPATCH(Color4dv, (v), (F, "glColor4dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
   DISPATCH(Color4f, (red, green, blue, alpha), (F, "glColor4f(%f, %f, %f, %f);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(Color4fv)(const GLfloat * v)
{
   DISPATCH(Color4fv, (v), (F, "glColor4fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color4i)(GLint red, GLint green, GLint blue, GLint alpha)
{
   DISPATCH(Color4i, (red, green, blue, alpha), (F, "glColor4i(%d, %d, %d, %d);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(Color4iv)(const GLint * v)
{
   DISPATCH(Color4iv, (v), (F, "glColor4iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color4s)(GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
   DISPATCH(Color4s, (red, green, blue, alpha), (F, "glColor4s(%d, %d, %d, %d);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(Color4sv)(const GLshort * v)
{
   DISPATCH(Color4sv, (v), (F, "glColor4sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color4ub)(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
   DISPATCH(Color4ub, (red, green, blue, alpha), (F, "glColor4ub(%d, %d, %d, %d);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(Color4ubv)(const GLubyte * v)
{
   DISPATCH(Color4ubv, (v), (F, "glColor4ubv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color4ui)(GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
   DISPATCH(Color4ui, (red, green, blue, alpha), (F, "glColor4ui(%d, %d, %d, %d);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(Color4uiv)(const GLuint * v)
{
   DISPATCH(Color4uiv, (v), (F, "glColor4uiv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color4us)(GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
   DISPATCH(Color4us, (red, green, blue, alpha), (F, "glColor4us(%d, %d, %d, %d);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(Color4usv)(const GLushort * v)
{
   DISPATCH(Color4usv, (v), (F, "glColor4usv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(EdgeFlag)(GLboolean flag)
{
   DISPATCH(EdgeFlag, (flag), (F, "glEdgeFlag(%d);\n", flag));
}

KEYWORD1 void KEYWORD2 NAME(EdgeFlagv)(const GLboolean * flag)
{
   DISPATCH(EdgeFlagv, (flag), (F, "glEdgeFlagv(%p);\n", (const void *) flag));
}

KEYWORD1 void KEYWORD2 NAME(End)(void)
{
   DISPATCH(End, (), (F, "glEnd();\n"));
}

KEYWORD1 void KEYWORD2 NAME(Indexd)(GLdouble c)
{
   DISPATCH(Indexd, (c), (F, "glIndexd(%f);\n", c));
}

KEYWORD1 void KEYWORD2 NAME(Indexdv)(const GLdouble * c)
{
   DISPATCH(Indexdv, (c), (F, "glIndexdv(%p);\n", (const void *) c));
}

KEYWORD1 void KEYWORD2 NAME(Indexf)(GLfloat c)
{
   DISPATCH(Indexf, (c), (F, "glIndexf(%f);\n", c));
}

KEYWORD1 void KEYWORD2 NAME(Indexfv)(const GLfloat * c)
{
   DISPATCH(Indexfv, (c), (F, "glIndexfv(%p);\n", (const void *) c));
}

KEYWORD1 void KEYWORD2 NAME(Indexi)(GLint c)
{
   DISPATCH(Indexi, (c), (F, "glIndexi(%d);\n", c));
}

KEYWORD1 void KEYWORD2 NAME(Indexiv)(const GLint * c)
{
   DISPATCH(Indexiv, (c), (F, "glIndexiv(%p);\n", (const void *) c));
}

KEYWORD1 void KEYWORD2 NAME(Indexs)(GLshort c)
{
   DISPATCH(Indexs, (c), (F, "glIndexs(%d);\n", c));
}

KEYWORD1 void KEYWORD2 NAME(Indexsv)(const GLshort * c)
{
   DISPATCH(Indexsv, (c), (F, "glIndexsv(%p);\n", (const void *) c));
}

KEYWORD1 void KEYWORD2 NAME(Normal3b)(GLbyte nx, GLbyte ny, GLbyte nz)
{
   DISPATCH(Normal3b, (nx, ny, nz), (F, "glNormal3b(%d, %d, %d);\n", nx, ny, nz));
}

KEYWORD1 void KEYWORD2 NAME(Normal3bv)(const GLbyte * v)
{
   DISPATCH(Normal3bv, (v), (F, "glNormal3bv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Normal3d)(GLdouble nx, GLdouble ny, GLdouble nz)
{
   DISPATCH(Normal3d, (nx, ny, nz), (F, "glNormal3d(%f, %f, %f);\n", nx, ny, nz));
}

KEYWORD1 void KEYWORD2 NAME(Normal3dv)(const GLdouble * v)
{
   DISPATCH(Normal3dv, (v), (F, "glNormal3dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Normal3f)(GLfloat nx, GLfloat ny, GLfloat nz)
{
   DISPATCH(Normal3f, (nx, ny, nz), (F, "glNormal3f(%f, %f, %f);\n", nx, ny, nz));
}

KEYWORD1 void KEYWORD2 NAME(Normal3fv)(const GLfloat * v)
{
   DISPATCH(Normal3fv, (v), (F, "glNormal3fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Normal3i)(GLint nx, GLint ny, GLint nz)
{
   DISPATCH(Normal3i, (nx, ny, nz), (F, "glNormal3i(%d, %d, %d);\n", nx, ny, nz));
}

KEYWORD1 void KEYWORD2 NAME(Normal3iv)(const GLint * v)
{
   DISPATCH(Normal3iv, (v), (F, "glNormal3iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Normal3s)(GLshort nx, GLshort ny, GLshort nz)
{
   DISPATCH(Normal3s, (nx, ny, nz), (F, "glNormal3s(%d, %d, %d);\n", nx, ny, nz));
}

KEYWORD1 void KEYWORD2 NAME(Normal3sv)(const GLshort * v)
{
   DISPATCH(Normal3sv, (v), (F, "glNormal3sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2d)(GLdouble x, GLdouble y)
{
   DISPATCH(RasterPos2d, (x, y), (F, "glRasterPos2d(%f, %f);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2dv)(const GLdouble * v)
{
   DISPATCH(RasterPos2dv, (v), (F, "glRasterPos2dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2f)(GLfloat x, GLfloat y)
{
   DISPATCH(RasterPos2f, (x, y), (F, "glRasterPos2f(%f, %f);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2fv)(const GLfloat * v)
{
   DISPATCH(RasterPos2fv, (v), (F, "glRasterPos2fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2i)(GLint x, GLint y)
{
   DISPATCH(RasterPos2i, (x, y), (F, "glRasterPos2i(%d, %d);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2iv)(const GLint * v)
{
   DISPATCH(RasterPos2iv, (v), (F, "glRasterPos2iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2s)(GLshort x, GLshort y)
{
   DISPATCH(RasterPos2s, (x, y), (F, "glRasterPos2s(%d, %d);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2sv)(const GLshort * v)
{
   DISPATCH(RasterPos2sv, (v), (F, "glRasterPos2sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3d)(GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(RasterPos3d, (x, y, z), (F, "glRasterPos3d(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3dv)(const GLdouble * v)
{
   DISPATCH(RasterPos3dv, (v), (F, "glRasterPos3dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3f)(GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(RasterPos3f, (x, y, z), (F, "glRasterPos3f(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3fv)(const GLfloat * v)
{
   DISPATCH(RasterPos3fv, (v), (F, "glRasterPos3fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3i)(GLint x, GLint y, GLint z)
{
   DISPATCH(RasterPos3i, (x, y, z), (F, "glRasterPos3i(%d, %d, %d);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3iv)(const GLint * v)
{
   DISPATCH(RasterPos3iv, (v), (F, "glRasterPos3iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3s)(GLshort x, GLshort y, GLshort z)
{
   DISPATCH(RasterPos3s, (x, y, z), (F, "glRasterPos3s(%d, %d, %d);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3sv)(const GLshort * v)
{
   DISPATCH(RasterPos3sv, (v), (F, "glRasterPos3sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   DISPATCH(RasterPos4d, (x, y, z, w), (F, "glRasterPos4d(%f, %f, %f, %f);\n", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4dv)(const GLdouble * v)
{
   DISPATCH(RasterPos4dv, (v), (F, "glRasterPos4dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   DISPATCH(RasterPos4f, (x, y, z, w), (F, "glRasterPos4f(%f, %f, %f, %f);\n", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4fv)(const GLfloat * v)
{
   DISPATCH(RasterPos4fv, (v), (F, "glRasterPos4fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4i)(GLint x, GLint y, GLint z, GLint w)
{
   DISPATCH(RasterPos4i, (x, y, z, w), (F, "glRasterPos4i(%d, %d, %d, %d);\n", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4iv)(const GLint * v)
{
   DISPATCH(RasterPos4iv, (v), (F, "glRasterPos4iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4s)(GLshort x, GLshort y, GLshort z, GLshort w)
{
   DISPATCH(RasterPos4s, (x, y, z, w), (F, "glRasterPos4s(%d, %d, %d, %d);\n", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4sv)(const GLshort * v)
{
   DISPATCH(RasterPos4sv, (v), (F, "glRasterPos4sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Rectd)(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
   DISPATCH(Rectd, (x1, y1, x2, y2), (F, "glRectd(%f, %f, %f, %f);\n", x1, y1, x2, y2));
}

KEYWORD1 void KEYWORD2 NAME(Rectdv)(const GLdouble * v1, const GLdouble * v2)
{
   DISPATCH(Rectdv, (v1, v2), (F, "glRectdv(%p, %p);\n", (const void *) v1, (const void *) v2));
}

KEYWORD1 void KEYWORD2 NAME(Rectf)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
   DISPATCH(Rectf, (x1, y1, x2, y2), (F, "glRectf(%f, %f, %f, %f);\n", x1, y1, x2, y2));
}

KEYWORD1 void KEYWORD2 NAME(Rectfv)(const GLfloat * v1, const GLfloat * v2)
{
   DISPATCH(Rectfv, (v1, v2), (F, "glRectfv(%p, %p);\n", (const void *) v1, (const void *) v2));
}

KEYWORD1 void KEYWORD2 NAME(Recti)(GLint x1, GLint y1, GLint x2, GLint y2)
{
   DISPATCH(Recti, (x1, y1, x2, y2), (F, "glRecti(%d, %d, %d, %d);\n", x1, y1, x2, y2));
}

KEYWORD1 void KEYWORD2 NAME(Rectiv)(const GLint * v1, const GLint * v2)
{
   DISPATCH(Rectiv, (v1, v2), (F, "glRectiv(%p, %p);\n", (const void *) v1, (const void *) v2));
}

KEYWORD1 void KEYWORD2 NAME(Rects)(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
   DISPATCH(Rects, (x1, y1, x2, y2), (F, "glRects(%d, %d, %d, %d);\n", x1, y1, x2, y2));
}

KEYWORD1 void KEYWORD2 NAME(Rectsv)(const GLshort * v1, const GLshort * v2)
{
   DISPATCH(Rectsv, (v1, v2), (F, "glRectsv(%p, %p);\n", (const void *) v1, (const void *) v2));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1d)(GLdouble s)
{
   DISPATCH(TexCoord1d, (s), (F, "glTexCoord1d(%f);\n", s));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1dv)(const GLdouble * v)
{
   DISPATCH(TexCoord1dv, (v), (F, "glTexCoord1dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1f)(GLfloat s)
{
   DISPATCH(TexCoord1f, (s), (F, "glTexCoord1f(%f);\n", s));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1fv)(const GLfloat * v)
{
   DISPATCH(TexCoord1fv, (v), (F, "glTexCoord1fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1i)(GLint s)
{
   DISPATCH(TexCoord1i, (s), (F, "glTexCoord1i(%d);\n", s));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1iv)(const GLint * v)
{
   DISPATCH(TexCoord1iv, (v), (F, "glTexCoord1iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1s)(GLshort s)
{
   DISPATCH(TexCoord1s, (s), (F, "glTexCoord1s(%d);\n", s));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1sv)(const GLshort * v)
{
   DISPATCH(TexCoord1sv, (v), (F, "glTexCoord1sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2d)(GLdouble s, GLdouble t)
{
   DISPATCH(TexCoord2d, (s, t), (F, "glTexCoord2d(%f, %f);\n", s, t));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2dv)(const GLdouble * v)
{
   DISPATCH(TexCoord2dv, (v), (F, "glTexCoord2dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2f)(GLfloat s, GLfloat t)
{
   DISPATCH(TexCoord2f, (s, t), (F, "glTexCoord2f(%f, %f);\n", s, t));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2fv)(const GLfloat * v)
{
   DISPATCH(TexCoord2fv, (v), (F, "glTexCoord2fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2i)(GLint s, GLint t)
{
   DISPATCH(TexCoord2i, (s, t), (F, "glTexCoord2i(%d, %d);\n", s, t));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2iv)(const GLint * v)
{
   DISPATCH(TexCoord2iv, (v), (F, "glTexCoord2iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2s)(GLshort s, GLshort t)
{
   DISPATCH(TexCoord2s, (s, t), (F, "glTexCoord2s(%d, %d);\n", s, t));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2sv)(const GLshort * v)
{
   DISPATCH(TexCoord2sv, (v), (F, "glTexCoord2sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3d)(GLdouble s, GLdouble t, GLdouble r)
{
   DISPATCH(TexCoord3d, (s, t, r), (F, "glTexCoord3d(%f, %f, %f);\n", s, t, r));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3dv)(const GLdouble * v)
{
   DISPATCH(TexCoord3dv, (v), (F, "glTexCoord3dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3f)(GLfloat s, GLfloat t, GLfloat r)
{
   DISPATCH(TexCoord3f, (s, t, r), (F, "glTexCoord3f(%f, %f, %f);\n", s, t, r));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3fv)(const GLfloat * v)
{
   DISPATCH(TexCoord3fv, (v), (F, "glTexCoord3fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3i)(GLint s, GLint t, GLint r)
{
   DISPATCH(TexCoord3i, (s, t, r), (F, "glTexCoord3i(%d, %d, %d);\n", s, t, r));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3iv)(const GLint * v)
{
   DISPATCH(TexCoord3iv, (v), (F, "glTexCoord3iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3s)(GLshort s, GLshort t, GLshort r)
{
   DISPATCH(TexCoord3s, (s, t, r), (F, "glTexCoord3s(%d, %d, %d);\n", s, t, r));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3sv)(const GLshort * v)
{
   DISPATCH(TexCoord3sv, (v), (F, "glTexCoord3sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4d)(GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
   DISPATCH(TexCoord4d, (s, t, r, q), (F, "glTexCoord4d(%f, %f, %f, %f);\n", s, t, r, q));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4dv)(const GLdouble * v)
{
   DISPATCH(TexCoord4dv, (v), (F, "glTexCoord4dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4f)(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
   DISPATCH(TexCoord4f, (s, t, r, q), (F, "glTexCoord4f(%f, %f, %f, %f);\n", s, t, r, q));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4fv)(const GLfloat * v)
{
   DISPATCH(TexCoord4fv, (v), (F, "glTexCoord4fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4i)(GLint s, GLint t, GLint r, GLint q)
{
   DISPATCH(TexCoord4i, (s, t, r, q), (F, "glTexCoord4i(%d, %d, %d, %d);\n", s, t, r, q));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4iv)(const GLint * v)
{
   DISPATCH(TexCoord4iv, (v), (F, "glTexCoord4iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4s)(GLshort s, GLshort t, GLshort r, GLshort q)
{
   DISPATCH(TexCoord4s, (s, t, r, q), (F, "glTexCoord4s(%d, %d, %d, %d);\n", s, t, r, q));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4sv)(const GLshort * v)
{
   DISPATCH(TexCoord4sv, (v), (F, "glTexCoord4sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2d)(GLdouble x, GLdouble y)
{
   DISPATCH(Vertex2d, (x, y), (F, "glVertex2d(%f, %f);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2dv)(const GLdouble * v)
{
   DISPATCH(Vertex2dv, (v), (F, "glVertex2dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2f)(GLfloat x, GLfloat y)
{
   DISPATCH(Vertex2f, (x, y), (F, "glVertex2f(%f, %f);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2fv)(const GLfloat * v)
{
   DISPATCH(Vertex2fv, (v), (F, "glVertex2fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2i)(GLint x, GLint y)
{
   DISPATCH(Vertex2i, (x, y), (F, "glVertex2i(%d, %d);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2iv)(const GLint * v)
{
   DISPATCH(Vertex2iv, (v), (F, "glVertex2iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2s)(GLshort x, GLshort y)
{
   DISPATCH(Vertex2s, (x, y), (F, "glVertex2s(%d, %d);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2sv)(const GLshort * v)
{
   DISPATCH(Vertex2sv, (v), (F, "glVertex2sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3d)(GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(Vertex3d, (x, y, z), (F, "glVertex3d(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3dv)(const GLdouble * v)
{
   DISPATCH(Vertex3dv, (v), (F, "glVertex3dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3f)(GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(Vertex3f, (x, y, z), (F, "glVertex3f(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3fv)(const GLfloat * v)
{
   DISPATCH(Vertex3fv, (v), (F, "glVertex3fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3i)(GLint x, GLint y, GLint z)
{
   DISPATCH(Vertex3i, (x, y, z), (F, "glVertex3i(%d, %d, %d);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3iv)(const GLint * v)
{
   DISPATCH(Vertex3iv, (v), (F, "glVertex3iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3s)(GLshort x, GLshort y, GLshort z)
{
   DISPATCH(Vertex3s, (x, y, z), (F, "glVertex3s(%d, %d, %d);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3sv)(const GLshort * v)
{
   DISPATCH(Vertex3sv, (v), (F, "glVertex3sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   DISPATCH(Vertex4d, (x, y, z, w), (F, "glVertex4d(%f, %f, %f, %f);\n", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4dv)(const GLdouble * v)
{
   DISPATCH(Vertex4dv, (v), (F, "glVertex4dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   DISPATCH(Vertex4f, (x, y, z, w), (F, "glVertex4f(%f, %f, %f, %f);\n", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4fv)(const GLfloat * v)
{
   DISPATCH(Vertex4fv, (v), (F, "glVertex4fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4i)(GLint x, GLint y, GLint z, GLint w)
{
   DISPATCH(Vertex4i, (x, y, z, w), (F, "glVertex4i(%d, %d, %d, %d);\n", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4iv)(const GLint * v)
{
   DISPATCH(Vertex4iv, (v), (F, "glVertex4iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4s)(GLshort x, GLshort y, GLshort z, GLshort w)
{
   DISPATCH(Vertex4s, (x, y, z, w), (F, "glVertex4s(%d, %d, %d, %d);\n", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4sv)(const GLshort * v)
{
   DISPATCH(Vertex4sv, (v), (F, "glVertex4sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(ClipPlane)(GLenum plane, const GLdouble * equation)
{
   DISPATCH(ClipPlane, (plane, equation), (F, "glClipPlane(0x%x, %p);\n", plane, (const void *) equation));
}

KEYWORD1 void KEYWORD2 NAME(ColorMaterial)(GLenum face, GLenum mode)
{
   DISPATCH(ColorMaterial, (face, mode), (F, "glColorMaterial(0x%x, 0x%x);\n", face, mode));
}

KEYWORD1 void KEYWORD2 NAME(CullFace)(GLenum mode)
{
   DISPATCH(CullFace, (mode), (F, "glCullFace(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(Fogf)(GLenum pname, GLfloat param)
{
   DISPATCH(Fogf, (pname, param), (F, "glFogf(0x%x, %f);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(Fogfv)(GLenum pname, const GLfloat * params)
{
   DISPATCH(Fogfv, (pname, params), (F, "glFogfv(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(Fogi)(GLenum pname, GLint param)
{
   DISPATCH(Fogi, (pname, param), (F, "glFogi(0x%x, %d);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(Fogiv)(GLenum pname, const GLint * params)
{
   DISPATCH(Fogiv, (pname, params), (F, "glFogiv(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(FrontFace)(GLenum mode)
{
   DISPATCH(FrontFace, (mode), (F, "glFrontFace(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(Hint)(GLenum target, GLenum mode)
{
   DISPATCH(Hint, (target, mode), (F, "glHint(0x%x, 0x%x);\n", target, mode));
}

KEYWORD1 void KEYWORD2 NAME(Lightf)(GLenum light, GLenum pname, GLfloat param)
{
   DISPATCH(Lightf, (light, pname, param), (F, "glLightf(0x%x, 0x%x, %f);\n", light, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(Lightfv)(GLenum light, GLenum pname, const GLfloat * params)
{
   DISPATCH(Lightfv, (light, pname, params), (F, "glLightfv(0x%x, 0x%x, %p);\n", light, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(Lighti)(GLenum light, GLenum pname, GLint param)
{
   DISPATCH(Lighti, (light, pname, param), (F, "glLighti(0x%x, 0x%x, %d);\n", light, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(Lightiv)(GLenum light, GLenum pname, const GLint * params)
{
   DISPATCH(Lightiv, (light, pname, params), (F, "glLightiv(0x%x, 0x%x, %p);\n", light, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(LightModelf)(GLenum pname, GLfloat param)
{
   DISPATCH(LightModelf, (pname, param), (F, "glLightModelf(0x%x, %f);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(LightModelfv)(GLenum pname, const GLfloat * params)
{
   DISPATCH(LightModelfv, (pname, params), (F, "glLightModelfv(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(LightModeli)(GLenum pname, GLint param)
{
   DISPATCH(LightModeli, (pname, param), (F, "glLightModeli(0x%x, %d);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(LightModeliv)(GLenum pname, const GLint * params)
{
   DISPATCH(LightModeliv, (pname, params), (F, "glLightModeliv(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(LineStipple)(GLint factor, GLushort pattern)
{
   DISPATCH(LineStipple, (factor, pattern), (F, "glLineStipple(%d, %d);\n", factor, pattern));
}

KEYWORD1 void KEYWORD2 NAME(LineWidth)(GLfloat width)
{
   DISPATCH(LineWidth, (width), (F, "glLineWidth(%f);\n", width));
}

KEYWORD1 void KEYWORD2 NAME(Materialf)(GLenum face, GLenum pname, GLfloat param)
{
   DISPATCH(Materialf, (face, pname, param), (F, "glMaterialf(0x%x, 0x%x, %f);\n", face, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(Materialfv)(GLenum face, GLenum pname, const GLfloat * params)
{
   DISPATCH(Materialfv, (face, pname, params), (F, "glMaterialfv(0x%x, 0x%x, %p);\n", face, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(Materiali)(GLenum face, GLenum pname, GLint param)
{
   DISPATCH(Materiali, (face, pname, param), (F, "glMateriali(0x%x, 0x%x, %d);\n", face, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(Materialiv)(GLenum face, GLenum pname, const GLint * params)
{
   DISPATCH(Materialiv, (face, pname, params), (F, "glMaterialiv(0x%x, 0x%x, %p);\n", face, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(PointSize)(GLfloat size)
{
   DISPATCH(PointSize, (size), (F, "glPointSize(%f);\n", size));
}

KEYWORD1 void KEYWORD2 NAME(PolygonMode)(GLenum face, GLenum mode)
{
   DISPATCH(PolygonMode, (face, mode), (F, "glPolygonMode(0x%x, 0x%x);\n", face, mode));
}

KEYWORD1 void KEYWORD2 NAME(PolygonStipple)(const GLubyte * mask)
{
   DISPATCH(PolygonStipple, (mask), (F, "glPolygonStipple(%p);\n", (const void *) mask));
}

KEYWORD1 void KEYWORD2 NAME(Scissor)(GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(Scissor, (x, y, width, height), (F, "glScissor(%d, %d, %d, %d);\n", x, y, width, height));
}

KEYWORD1 void KEYWORD2 NAME(ShadeModel)(GLenum mode)
{
   DISPATCH(ShadeModel, (mode), (F, "glShadeModel(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(TexParameterf)(GLenum target, GLenum pname, GLfloat param)
{
   DISPATCH(TexParameterf, (target, pname, param), (F, "glTexParameterf(0x%x, 0x%x, %f);\n", target, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(TexParameterfv)(GLenum target, GLenum pname, const GLfloat * params)
{
   DISPATCH(TexParameterfv, (target, pname, params), (F, "glTexParameterfv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(TexParameteri)(GLenum target, GLenum pname, GLint param)
{
   DISPATCH(TexParameteri, (target, pname, param), (F, "glTexParameteri(0x%x, 0x%x, %d);\n", target, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(TexParameteriv)(GLenum target, GLenum pname, const GLint * params)
{
   DISPATCH(TexParameteriv, (target, pname, params), (F, "glTexParameteriv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(TexImage1D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid * pixels)
{
   DISPATCH(TexImage1D, (target, level, internalformat, width, border, format, type, pixels), (F, "glTexImage1D(0x%x, %d, %d, %d, %d, 0x%x, 0x%x, %p);\n", target, level, internalformat, width, border, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(TexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * pixels)
{
   DISPATCH(TexImage2D, (target, level, internalformat, width, height, border, format, type, pixels), (F, "glTexImage2D(0x%x, %d, %d, %d, %d, %d, 0x%x, 0x%x, %p);\n", target, level, internalformat, width, height, border, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(TexEnvf)(GLenum target, GLenum pname, GLfloat param)
{
   DISPATCH(TexEnvf, (target, pname, param), (F, "glTexEnvf(0x%x, 0x%x, %f);\n", target, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(TexEnvfv)(GLenum target, GLenum pname, const GLfloat * params)
{
   DISPATCH(TexEnvfv, (target, pname, params), (F, "glTexEnvfv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(TexEnvi)(GLenum target, GLenum pname, GLint param)
{
   DISPATCH(TexEnvi, (target, pname, param), (F, "glTexEnvi(0x%x, 0x%x, %d);\n", target, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(TexEnviv)(GLenum target, GLenum pname, const GLint * params)
{
   DISPATCH(TexEnviv, (target, pname, params), (F, "glTexEnviv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(TexGend)(GLenum coord, GLenum pname, GLdouble param)
{
   DISPATCH(TexGend, (coord, pname, param), (F, "glTexGend(0x%x, 0x%x, %f);\n", coord, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(TexGendv)(GLenum coord, GLenum pname, const GLdouble * params)
{
   DISPATCH(TexGendv, (coord, pname, params), (F, "glTexGendv(0x%x, 0x%x, %p);\n", coord, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(TexGenf)(GLenum coord, GLenum pname, GLfloat param)
{
   DISPATCH(TexGenf, (coord, pname, param), (F, "glTexGenf(0x%x, 0x%x, %f);\n", coord, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(TexGenfv)(GLenum coord, GLenum pname, const GLfloat * params)
{
   DISPATCH(TexGenfv, (coord, pname, params), (F, "glTexGenfv(0x%x, 0x%x, %p);\n", coord, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(TexGeni)(GLenum coord, GLenum pname, GLint param)
{
   DISPATCH(TexGeni, (coord, pname, param), (F, "glTexGeni(0x%x, 0x%x, %d);\n", coord, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(TexGeniv)(GLenum coord, GLenum pname, const GLint * params)
{
   DISPATCH(TexGeniv, (coord, pname, params), (F, "glTexGeniv(0x%x, 0x%x, %p);\n", coord, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(FeedbackBuffer)(GLsizei size, GLenum type, GLfloat * buffer)
{
   DISPATCH(FeedbackBuffer, (size, type, buffer), (F, "glFeedbackBuffer(%d, 0x%x, %p);\n", size, type, (const void *) buffer));
}

KEYWORD1 void KEYWORD2 NAME(SelectBuffer)(GLsizei size, GLuint * buffer)
{
   DISPATCH(SelectBuffer, (size, buffer), (F, "glSelectBuffer(%d, %p);\n", size, (const void *) buffer));
}

KEYWORD1 GLint KEYWORD2 NAME(RenderMode)(GLenum mode)
{
   RETURN_DISPATCH(RenderMode, (mode), (F, "glRenderMode(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(InitNames)(void)
{
   DISPATCH(InitNames, (), (F, "glInitNames();\n"));
}

KEYWORD1 void KEYWORD2 NAME(LoadName)(GLuint name)
{
   DISPATCH(LoadName, (name), (F, "glLoadName(%d);\n", name));
}

KEYWORD1 void KEYWORD2 NAME(PassThrough)(GLfloat token)
{
   DISPATCH(PassThrough, (token), (F, "glPassThrough(%f);\n", token));
}

KEYWORD1 void KEYWORD2 NAME(PopName)(void)
{
   DISPATCH(PopName, (), (F, "glPopName();\n"));
}

KEYWORD1 void KEYWORD2 NAME(PushName)(GLuint name)
{
   DISPATCH(PushName, (name), (F, "glPushName(%d);\n", name));
}

KEYWORD1 void KEYWORD2 NAME(DrawBuffer)(GLenum mode)
{
   DISPATCH(DrawBuffer, (mode), (F, "glDrawBuffer(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(Clear)(GLbitfield mask)
{
   DISPATCH(Clear, (mask), (F, "glClear(%d);\n", mask));
}

KEYWORD1 void KEYWORD2 NAME(ClearAccum)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
   DISPATCH(ClearAccum, (red, green, blue, alpha), (F, "glClearAccum(%f, %f, %f, %f);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(ClearIndex)(GLfloat c)
{
   DISPATCH(ClearIndex, (c), (F, "glClearIndex(%f);\n", c));
}

KEYWORD1 void KEYWORD2 NAME(ClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
   DISPATCH(ClearColor, (red, green, blue, alpha), (F, "glClearColor(%f, %f, %f, %f);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(ClearStencil)(GLint s)
{
   DISPATCH(ClearStencil, (s), (F, "glClearStencil(%d);\n", s));
}

KEYWORD1 void KEYWORD2 NAME(ClearDepth)(GLclampd depth)
{
   DISPATCH(ClearDepth, (depth), (F, "glClearDepth(%f);\n", depth));
}

KEYWORD1 void KEYWORD2 NAME(StencilMask)(GLuint mask)
{
   DISPATCH(StencilMask, (mask), (F, "glStencilMask(%d);\n", mask));
}

KEYWORD1 void KEYWORD2 NAME(ColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
   DISPATCH(ColorMask, (red, green, blue, alpha), (F, "glColorMask(%d, %d, %d, %d);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(DepthMask)(GLboolean flag)
{
   DISPATCH(DepthMask, (flag), (F, "glDepthMask(%d);\n", flag));
}

KEYWORD1 void KEYWORD2 NAME(IndexMask)(GLuint mask)
{
   DISPATCH(IndexMask, (mask), (F, "glIndexMask(%d);\n", mask));
}

KEYWORD1 void KEYWORD2 NAME(Accum)(GLenum op, GLfloat value)
{
   DISPATCH(Accum, (op, value), (F, "glAccum(0x%x, %f);\n", op, value));
}

KEYWORD1 void KEYWORD2 NAME(Disable)(GLenum cap)
{
   DISPATCH(Disable, (cap), (F, "glDisable(0x%x);\n", cap));
}

KEYWORD1 void KEYWORD2 NAME(Enable)(GLenum cap)
{
   DISPATCH(Enable, (cap), (F, "glEnable(0x%x);\n", cap));
}

KEYWORD1 void KEYWORD2 NAME(Finish)(void)
{
   DISPATCH(Finish, (), (F, "glFinish();\n"));
}

KEYWORD1 void KEYWORD2 NAME(Flush)(void)
{
   DISPATCH(Flush, (), (F, "glFlush();\n"));
}

KEYWORD1 void KEYWORD2 NAME(PopAttrib)(void)
{
   DISPATCH(PopAttrib, (), (F, "glPopAttrib();\n"));
}

KEYWORD1 void KEYWORD2 NAME(PushAttrib)(GLbitfield mask)
{
   DISPATCH(PushAttrib, (mask), (F, "glPushAttrib(%d);\n", mask));
}

KEYWORD1 void KEYWORD2 NAME(Map1d)(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble * points)
{
   DISPATCH(Map1d, (target, u1, u2, stride, order, points), (F, "glMap1d(0x%x, %f, %f, %d, %d, %p);\n", target, u1, u2, stride, order, (const void *) points));
}

KEYWORD1 void KEYWORD2 NAME(Map1f)(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat * points)
{
   DISPATCH(Map1f, (target, u1, u2, stride, order, points), (F, "glMap1f(0x%x, %f, %f, %d, %d, %p);\n", target, u1, u2, stride, order, (const void *) points));
}

KEYWORD1 void KEYWORD2 NAME(Map2d)(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble * points)
{
   DISPATCH(Map2d, (target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points), (F, "glMap2d(0x%x, %f, %f, %d, %d, %f, %f, %d, %d, %p);\n", target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, (const void *) points));
}

KEYWORD1 void KEYWORD2 NAME(Map2f)(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat * points)
{
   DISPATCH(Map2f, (target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points), (F, "glMap2f(0x%x, %f, %f, %d, %d, %f, %f, %d, %d, %p);\n", target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, (const void *) points));
}

KEYWORD1 void KEYWORD2 NAME(MapGrid1d)(GLint un, GLdouble u1, GLdouble u2)
{
   DISPATCH(MapGrid1d, (un, u1, u2), (F, "glMapGrid1d(%d, %f, %f);\n", un, u1, u2));
}

KEYWORD1 void KEYWORD2 NAME(MapGrid1f)(GLint un, GLfloat u1, GLfloat u2)
{
   DISPATCH(MapGrid1f, (un, u1, u2), (F, "glMapGrid1f(%d, %f, %f);\n", un, u1, u2));
}

KEYWORD1 void KEYWORD2 NAME(MapGrid2d)(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
   DISPATCH(MapGrid2d, (un, u1, u2, vn, v1, v2), (F, "glMapGrid2d(%d, %f, %f, %d, %f, %f);\n", un, u1, u2, vn, v1, v2));
}

KEYWORD1 void KEYWORD2 NAME(MapGrid2f)(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
   DISPATCH(MapGrid2f, (un, u1, u2, vn, v1, v2), (F, "glMapGrid2f(%d, %f, %f, %d, %f, %f);\n", un, u1, u2, vn, v1, v2));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord1d)(GLdouble u)
{
   DISPATCH(EvalCoord1d, (u), (F, "glEvalCoord1d(%f);\n", u));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord1dv)(const GLdouble * u)
{
   DISPATCH(EvalCoord1dv, (u), (F, "glEvalCoord1dv(%p);\n", (const void *) u));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord1f)(GLfloat u)
{
   DISPATCH(EvalCoord1f, (u), (F, "glEvalCoord1f(%f);\n", u));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord1fv)(const GLfloat * u)
{
   DISPATCH(EvalCoord1fv, (u), (F, "glEvalCoord1fv(%p);\n", (const void *) u));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord2d)(GLdouble u, GLdouble v)
{
   DISPATCH(EvalCoord2d, (u, v), (F, "glEvalCoord2d(%f, %f);\n", u, v));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord2dv)(const GLdouble * u)
{
   DISPATCH(EvalCoord2dv, (u), (F, "glEvalCoord2dv(%p);\n", (const void *) u));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord2f)(GLfloat u, GLfloat v)
{
   DISPATCH(EvalCoord2f, (u, v), (F, "glEvalCoord2f(%f, %f);\n", u, v));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord2fv)(const GLfloat * u)
{
   DISPATCH(EvalCoord2fv, (u), (F, "glEvalCoord2fv(%p);\n", (const void *) u));
}

KEYWORD1 void KEYWORD2 NAME(EvalMesh1)(GLenum mode, GLint i1, GLint i2)
{
   DISPATCH(EvalMesh1, (mode, i1, i2), (F, "glEvalMesh1(0x%x, %d, %d);\n", mode, i1, i2));
}

KEYWORD1 void KEYWORD2 NAME(EvalPoint1)(GLint i)
{
   DISPATCH(EvalPoint1, (i), (F, "glEvalPoint1(%d);\n", i));
}

KEYWORD1 void KEYWORD2 NAME(EvalMesh2)(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
   DISPATCH(EvalMesh2, (mode, i1, i2, j1, j2), (F, "glEvalMesh2(0x%x, %d, %d, %d, %d);\n", mode, i1, i2, j1, j2));
}

KEYWORD1 void KEYWORD2 NAME(EvalPoint2)(GLint i, GLint j)
{
   DISPATCH(EvalPoint2, (i, j), (F, "glEvalPoint2(%d, %d);\n", i, j));
}

KEYWORD1 void KEYWORD2 NAME(AlphaFunc)(GLenum func, GLclampf ref)
{
   DISPATCH(AlphaFunc, (func, ref), (F, "glAlphaFunc(0x%x, %f);\n", func, ref));
}

KEYWORD1 void KEYWORD2 NAME(BlendFunc)(GLenum sfactor, GLenum dfactor)
{
   DISPATCH(BlendFunc, (sfactor, dfactor), (F, "glBlendFunc(0x%x, 0x%x);\n", sfactor, dfactor));
}

KEYWORD1 void KEYWORD2 NAME(LogicOp)(GLenum opcode)
{
   DISPATCH(LogicOp, (opcode), (F, "glLogicOp(0x%x);\n", opcode));
}

KEYWORD1 void KEYWORD2 NAME(StencilFunc)(GLenum func, GLint ref, GLuint mask)
{
   DISPATCH(StencilFunc, (func, ref, mask), (F, "glStencilFunc(0x%x, %d, %d);\n", func, ref, mask));
}

KEYWORD1 void KEYWORD2 NAME(StencilOp)(GLenum fail, GLenum zfail, GLenum zpass)
{
   DISPATCH(StencilOp, (fail, zfail, zpass), (F, "glStencilOp(0x%x, 0x%x, 0x%x);\n", fail, zfail, zpass));
}

KEYWORD1 void KEYWORD2 NAME(DepthFunc)(GLenum func)
{
   DISPATCH(DepthFunc, (func), (F, "glDepthFunc(0x%x);\n", func));
}

KEYWORD1 void KEYWORD2 NAME(PixelZoom)(GLfloat xfactor, GLfloat yfactor)
{
   DISPATCH(PixelZoom, (xfactor, yfactor), (F, "glPixelZoom(%f, %f);\n", xfactor, yfactor));
}

KEYWORD1 void KEYWORD2 NAME(PixelTransferf)(GLenum pname, GLfloat param)
{
   DISPATCH(PixelTransferf, (pname, param), (F, "glPixelTransferf(0x%x, %f);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(PixelTransferi)(GLenum pname, GLint param)
{
   DISPATCH(PixelTransferi, (pname, param), (F, "glPixelTransferi(0x%x, %d);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(PixelStoref)(GLenum pname, GLfloat param)
{
   DISPATCH(PixelStoref, (pname, param), (F, "glPixelStoref(0x%x, %f);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(PixelStorei)(GLenum pname, GLint param)
{
   DISPATCH(PixelStorei, (pname, param), (F, "glPixelStorei(0x%x, %d);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(PixelMapfv)(GLenum map, GLsizei mapsize, const GLfloat * values)
{
   DISPATCH(PixelMapfv, (map, mapsize, values), (F, "glPixelMapfv(0x%x, %d, %p);\n", map, mapsize, (const void *) values));
}

KEYWORD1 void KEYWORD2 NAME(PixelMapuiv)(GLenum map, GLsizei mapsize, const GLuint * values)
{
   DISPATCH(PixelMapuiv, (map, mapsize, values), (F, "glPixelMapuiv(0x%x, %d, %p);\n", map, mapsize, (const void *) values));
}

KEYWORD1 void KEYWORD2 NAME(PixelMapusv)(GLenum map, GLsizei mapsize, const GLushort * values)
{
   DISPATCH(PixelMapusv, (map, mapsize, values), (F, "glPixelMapusv(0x%x, %d, %p);\n", map, mapsize, (const void *) values));
}

KEYWORD1 void KEYWORD2 NAME(ReadBuffer)(GLenum mode)
{
   DISPATCH(ReadBuffer, (mode), (F, "glReadBuffer(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(CopyPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
   DISPATCH(CopyPixels, (x, y, width, height, type), (F, "glCopyPixels(%d, %d, %d, %d, 0x%x);\n", x, y, width, height, type));
}

KEYWORD1 void KEYWORD2 NAME(ReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * pixels)
{
   DISPATCH(ReadPixels, (x, y, width, height, format, type, pixels), (F, "glReadPixels(%d, %d, %d, %d, 0x%x, 0x%x, %p);\n", x, y, width, height, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(DrawPixels)(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels)
{
   DISPATCH(DrawPixels, (width, height, format, type, pixels), (F, "glDrawPixels(%d, %d, 0x%x, 0x%x, %p);\n", width, height, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(GetBooleanv)(GLenum pname, GLboolean * params)
{
   DISPATCH(GetBooleanv, (pname, params), (F, "glGetBooleanv(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetClipPlane)(GLenum plane, GLdouble * equation)
{
   DISPATCH(GetClipPlane, (plane, equation), (F, "glGetClipPlane(0x%x, %p);\n", plane, (const void *) equation));
}

KEYWORD1 void KEYWORD2 NAME(GetDoublev)(GLenum pname, GLdouble * params)
{
   DISPATCH(GetDoublev, (pname, params), (F, "glGetDoublev(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 GLenum KEYWORD2 NAME(GetError)(void)
{
   RETURN_DISPATCH(GetError, (), (F, "glGetError();\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetFloatv)(GLenum pname, GLfloat * params)
{
   DISPATCH(GetFloatv, (pname, params), (F, "glGetFloatv(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetIntegerv)(GLenum pname, GLint * params)
{
   DISPATCH(GetIntegerv, (pname, params), (F, "glGetIntegerv(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetLightfv)(GLenum light, GLenum pname, GLfloat * params)
{
   DISPATCH(GetLightfv, (light, pname, params), (F, "glGetLightfv(0x%x, 0x%x, %p);\n", light, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetLightiv)(GLenum light, GLenum pname, GLint * params)
{
   DISPATCH(GetLightiv, (light, pname, params), (F, "glGetLightiv(0x%x, 0x%x, %p);\n", light, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetMapdv)(GLenum target, GLenum query, GLdouble * v)
{
   DISPATCH(GetMapdv, (target, query, v), (F, "glGetMapdv(0x%x, 0x%x, %p);\n", target, query, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(GetMapfv)(GLenum target, GLenum query, GLfloat * v)
{
   DISPATCH(GetMapfv, (target, query, v), (F, "glGetMapfv(0x%x, 0x%x, %p);\n", target, query, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(GetMapiv)(GLenum target, GLenum query, GLint * v)
{
   DISPATCH(GetMapiv, (target, query, v), (F, "glGetMapiv(0x%x, 0x%x, %p);\n", target, query, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(GetMaterialfv)(GLenum face, GLenum pname, GLfloat * params)
{
   DISPATCH(GetMaterialfv, (face, pname, params), (F, "glGetMaterialfv(0x%x, 0x%x, %p);\n", face, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetMaterialiv)(GLenum face, GLenum pname, GLint * params)
{
   DISPATCH(GetMaterialiv, (face, pname, params), (F, "glGetMaterialiv(0x%x, 0x%x, %p);\n", face, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetPixelMapfv)(GLenum map, GLfloat * values)
{
   DISPATCH(GetPixelMapfv, (map, values), (F, "glGetPixelMapfv(0x%x, %p);\n", map, (const void *) values));
}

KEYWORD1 void KEYWORD2 NAME(GetPixelMapuiv)(GLenum map, GLuint * values)
{
   DISPATCH(GetPixelMapuiv, (map, values), (F, "glGetPixelMapuiv(0x%x, %p);\n", map, (const void *) values));
}

KEYWORD1 void KEYWORD2 NAME(GetPixelMapusv)(GLenum map, GLushort * values)
{
   DISPATCH(GetPixelMapusv, (map, values), (F, "glGetPixelMapusv(0x%x, %p);\n", map, (const void *) values));
}

KEYWORD1 void KEYWORD2 NAME(GetPolygonStipple)(GLubyte * mask)
{
   DISPATCH(GetPolygonStipple, (mask), (F, "glGetPolygonStipple(%p);\n", (const void *) mask));
}

KEYWORD1 const GLubyte * KEYWORD2 NAME(GetString)(GLenum name)
{
   RETURN_DISPATCH(GetString, (name), (F, "glGetString(0x%x);\n", name));
}

KEYWORD1 void KEYWORD2 NAME(GetTexEnvfv)(GLenum target, GLenum pname, GLfloat * params)
{
   DISPATCH(GetTexEnvfv, (target, pname, params), (F, "glGetTexEnvfv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexEnviv)(GLenum target, GLenum pname, GLint * params)
{
   DISPATCH(GetTexEnviv, (target, pname, params), (F, "glGetTexEnviv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexGendv)(GLenum coord, GLenum pname, GLdouble * params)
{
   DISPATCH(GetTexGendv, (coord, pname, params), (F, "glGetTexGendv(0x%x, 0x%x, %p);\n", coord, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexGenfv)(GLenum coord, GLenum pname, GLfloat * params)
{
   DISPATCH(GetTexGenfv, (coord, pname, params), (F, "glGetTexGenfv(0x%x, 0x%x, %p);\n", coord, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexGeniv)(GLenum coord, GLenum pname, GLint * params)
{
   DISPATCH(GetTexGeniv, (coord, pname, params), (F, "glGetTexGeniv(0x%x, 0x%x, %p);\n", coord, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexImage)(GLenum target, GLint level, GLenum format, GLenum type, GLvoid * pixels)
{
   DISPATCH(GetTexImage, (target, level, format, type, pixels), (F, "glGetTexImage(0x%x, %d, 0x%x, 0x%x, %p);\n", target, level, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(GetTexParameterfv)(GLenum target, GLenum pname, GLfloat * params)
{
   DISPATCH(GetTexParameterfv, (target, pname, params), (F, "glGetTexParameterfv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexParameteriv)(GLenum target, GLenum pname, GLint * params)
{
   DISPATCH(GetTexParameteriv, (target, pname, params), (F, "glGetTexParameteriv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexLevelParameterfv)(GLenum target, GLint level, GLenum pname, GLfloat * params)
{
   DISPATCH(GetTexLevelParameterfv, (target, level, pname, params), (F, "glGetTexLevelParameterfv(0x%x, %d, 0x%x, %p);\n", target, level, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexLevelParameteriv)(GLenum target, GLint level, GLenum pname, GLint * params)
{
   DISPATCH(GetTexLevelParameteriv, (target, level, pname, params), (F, "glGetTexLevelParameteriv(0x%x, %d, 0x%x, %p);\n", target, level, pname, (const void *) params));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsEnabled)(GLenum cap)
{
   RETURN_DISPATCH(IsEnabled, (cap), (F, "glIsEnabled(0x%x);\n", cap));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsList)(GLuint list)
{
   RETURN_DISPATCH(IsList, (list), (F, "glIsList(%d);\n", list));
}

KEYWORD1 void KEYWORD2 NAME(DepthRange)(GLclampd zNear, GLclampd zFar)
{
   DISPATCH(DepthRange, (zNear, zFar), (F, "glDepthRange(%f, %f);\n", zNear, zFar));
}

KEYWORD1 void KEYWORD2 NAME(Frustum)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
   DISPATCH(Frustum, (left, right, bottom, top, zNear, zFar), (F, "glFrustum(%f, %f, %f, %f, %f, %f);\n", left, right, bottom, top, zNear, zFar));
}

KEYWORD1 void KEYWORD2 NAME(LoadIdentity)(void)
{
   DISPATCH(LoadIdentity, (), (F, "glLoadIdentity();\n"));
}

KEYWORD1 void KEYWORD2 NAME(LoadMatrixf)(const GLfloat * m)
{
   DISPATCH(LoadMatrixf, (m), (F, "glLoadMatrixf(%p);\n", (const void *) m));
}

KEYWORD1 void KEYWORD2 NAME(LoadMatrixd)(const GLdouble * m)
{
   DISPATCH(LoadMatrixd, (m), (F, "glLoadMatrixd(%p);\n", (const void *) m));
}

KEYWORD1 void KEYWORD2 NAME(MatrixMode)(GLenum mode)
{
   DISPATCH(MatrixMode, (mode), (F, "glMatrixMode(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(MultMatrixf)(const GLfloat * m)
{
   DISPATCH(MultMatrixf, (m), (F, "glMultMatrixf(%p);\n", (const void *) m));
}

KEYWORD1 void KEYWORD2 NAME(MultMatrixd)(const GLdouble * m)
{
   DISPATCH(MultMatrixd, (m), (F, "glMultMatrixd(%p);\n", (const void *) m));
}

KEYWORD1 void KEYWORD2 NAME(Ortho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
   DISPATCH(Ortho, (left, right, bottom, top, zNear, zFar), (F, "glOrtho(%f, %f, %f, %f, %f, %f);\n", left, right, bottom, top, zNear, zFar));
}

KEYWORD1 void KEYWORD2 NAME(PopMatrix)(void)
{
   DISPATCH(PopMatrix, (), (F, "glPopMatrix();\n"));
}

KEYWORD1 void KEYWORD2 NAME(PushMatrix)(void)
{
   DISPATCH(PushMatrix, (), (F, "glPushMatrix();\n"));
}

KEYWORD1 void KEYWORD2 NAME(Rotated)(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(Rotated, (angle, x, y, z), (F, "glRotated(%f, %f, %f, %f);\n", angle, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Rotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(Rotatef, (angle, x, y, z), (F, "glRotatef(%f, %f, %f, %f);\n", angle, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Scaled)(GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(Scaled, (x, y, z), (F, "glScaled(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Scalef)(GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(Scalef, (x, y, z), (F, "glScalef(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Translated)(GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(Translated, (x, y, z), (F, "glTranslated(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Translatef)(GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(Translatef, (x, y, z), (F, "glTranslatef(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Viewport)(GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(Viewport, (x, y, width, height), (F, "glViewport(%d, %d, %d, %d);\n", x, y, width, height));
}

KEYWORD1 void KEYWORD2 NAME(ArrayElement)(GLint i)
{
   DISPATCH(ArrayElement, (i), (F, "glArrayElement(%d);\n", i));
}

KEYWORD1 void KEYWORD2 NAME(ArrayElementEXT)(GLint i)
{
   DISPATCH(ArrayElement, (i), (F, "glArrayElementEXT(%d);\n", i));
}

KEYWORD1 void KEYWORD2 NAME(BindTexture)(GLenum target, GLuint texture)
{
   DISPATCH(BindTexture, (target, texture), (F, "glBindTexture(0x%x, %d);\n", target, texture));
}

KEYWORD1 void KEYWORD2 NAME(BindTextureEXT)(GLenum target, GLuint texture)
{
   DISPATCH(BindTexture, (target, texture), (F, "glBindTextureEXT(0x%x, %d);\n", target, texture));
}

KEYWORD1 void KEYWORD2 NAME(ColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
   DISPATCH(ColorPointer, (size, type, stride, pointer), (F, "glColorPointer(%d, 0x%x, %d, %p);\n", size, type, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(DisableClientState)(GLenum array)
{
   DISPATCH(DisableClientState, (array), (F, "glDisableClientState(0x%x);\n", array));
}

KEYWORD1 void KEYWORD2 NAME(DrawArrays)(GLenum mode, GLint first, GLsizei count)
{
   DISPATCH(DrawArrays, (mode, first, count), (F, "glDrawArrays(0x%x, %d, %d);\n", mode, first, count));
}

KEYWORD1 void KEYWORD2 NAME(DrawArraysEXT)(GLenum mode, GLint first, GLsizei count)
{
   DISPATCH(DrawArrays, (mode, first, count), (F, "glDrawArraysEXT(0x%x, %d, %d);\n", mode, first, count));
}

KEYWORD1 void KEYWORD2 NAME(DrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid * indices)
{
   DISPATCH(DrawElements, (mode, count, type, indices), (F, "glDrawElements(0x%x, %d, 0x%x, %p);\n", mode, count, type, (const void *) indices));
}

KEYWORD1 void KEYWORD2 NAME(EdgeFlagPointer)(GLsizei stride, const GLvoid * pointer)
{
   DISPATCH(EdgeFlagPointer, (stride, pointer), (F, "glEdgeFlagPointer(%d, %p);\n", stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(EnableClientState)(GLenum array)
{
   DISPATCH(EnableClientState, (array), (F, "glEnableClientState(0x%x);\n", array));
}

KEYWORD1 void KEYWORD2 NAME(IndexPointer)(GLenum type, GLsizei stride, const GLvoid * pointer)
{
   DISPATCH(IndexPointer, (type, stride, pointer), (F, "glIndexPointer(0x%x, %d, %p);\n", type, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(Indexub)(GLubyte c)
{
   DISPATCH(Indexub, (c), (F, "glIndexub(%d);\n", c));
}

KEYWORD1 void KEYWORD2 NAME(Indexubv)(const GLubyte * c)
{
   DISPATCH(Indexubv, (c), (F, "glIndexubv(%p);\n", (const void *) c));
}

KEYWORD1 void KEYWORD2 NAME(InterleavedArrays)(GLenum format, GLsizei stride, const GLvoid * pointer)
{
   DISPATCH(InterleavedArrays, (format, stride, pointer), (F, "glInterleavedArrays(0x%x, %d, %p);\n", format, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(NormalPointer)(GLenum type, GLsizei stride, const GLvoid * pointer)
{
   DISPATCH(NormalPointer, (type, stride, pointer), (F, "glNormalPointer(0x%x, %d, %p);\n", type, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(PolygonOffset)(GLfloat factor, GLfloat units)
{
   DISPATCH(PolygonOffset, (factor, units), (F, "glPolygonOffset(%f, %f);\n", factor, units));
}

KEYWORD1 void KEYWORD2 NAME(TexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
   DISPATCH(TexCoordPointer, (size, type, stride, pointer), (F, "glTexCoordPointer(%d, 0x%x, %d, %p);\n", size, type, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(VertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
   DISPATCH(VertexPointer, (size, type, stride, pointer), (F, "glVertexPointer(%d, 0x%x, %d, %p);\n", size, type, stride, (const void *) pointer));
}

KEYWORD1 GLboolean KEYWORD2 NAME(AreTexturesResident)(GLsizei n, const GLuint * textures, GLboolean * residences)
{
   RETURN_DISPATCH(AreTexturesResident, (n, textures, residences), (F, "glAreTexturesResident(%d, %p, %p);\n", n, (const void *) textures, (const void *) residences));
}

#ifndef GLX_INDIRECT_RENDERING
KEYWORD1 GLboolean KEYWORD2 NAME(AreTexturesResidentEXT)(GLsizei n, const GLuint * textures, GLboolean * residences)
{
   RETURN_DISPATCH(AreTexturesResident, (n, textures, residences), (F, "glAreTexturesResidentEXT(%d, %p, %p);\n", n, (const void *) textures, (const void *) residences));
}
#endif /* GLX_INDIRECT_RENDERING */

KEYWORD1 void KEYWORD2 NAME(CopyTexImage1D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
   DISPATCH(CopyTexImage1D, (target, level, internalformat, x, y, width, border), (F, "glCopyTexImage1D(0x%x, %d, 0x%x, %d, %d, %d, %d);\n", target, level, internalformat, x, y, width, border));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexImage1DEXT)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
   DISPATCH(CopyTexImage1D, (target, level, internalformat, x, y, width, border), (F, "glCopyTexImage1DEXT(0x%x, %d, 0x%x, %d, %d, %d, %d);\n", target, level, internalformat, x, y, width, border));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
   DISPATCH(CopyTexImage2D, (target, level, internalformat, x, y, width, height, border), (F, "glCopyTexImage2D(0x%x, %d, 0x%x, %d, %d, %d, %d, %d);\n", target, level, internalformat, x, y, width, height, border));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexImage2DEXT)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
   DISPATCH(CopyTexImage2D, (target, level, internalformat, x, y, width, height, border), (F, "glCopyTexImage2DEXT(0x%x, %d, 0x%x, %d, %d, %d, %d, %d);\n", target, level, internalformat, x, y, width, height, border));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyTexSubImage1D, (target, level, xoffset, x, y, width), (F, "glCopyTexSubImage1D(0x%x, %d, %d, %d, %d, %d);\n", target, level, xoffset, x, y, width));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexSubImage1DEXT)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyTexSubImage1D, (target, level, xoffset, x, y, width), (F, "glCopyTexSubImage1DEXT(0x%x, %d, %d, %d, %d, %d);\n", target, level, xoffset, x, y, width));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(CopyTexSubImage2D, (target, level, xoffset, yoffset, x, y, width, height), (F, "glCopyTexSubImage2D(0x%x, %d, %d, %d, %d, %d, %d, %d);\n", target, level, xoffset, yoffset, x, y, width, height));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexSubImage2DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(CopyTexSubImage2D, (target, level, xoffset, yoffset, x, y, width, height), (F, "glCopyTexSubImage2DEXT(0x%x, %d, %d, %d, %d, %d, %d, %d);\n", target, level, xoffset, yoffset, x, y, width, height));
}

KEYWORD1 void KEYWORD2 NAME(DeleteTextures)(GLsizei n, const GLuint * textures)
{
   DISPATCH(DeleteTextures, (n, textures), (F, "glDeleteTextures(%d, %p);\n", n, (const void *) textures));
}

#ifndef GLX_INDIRECT_RENDERING
KEYWORD1 void KEYWORD2 NAME(DeleteTexturesEXT)(GLsizei n, const GLuint * textures)
{
   DISPATCH(DeleteTextures, (n, textures), (F, "glDeleteTexturesEXT(%d, %p);\n", n, (const void *) textures));
}
#endif /* GLX_INDIRECT_RENDERING */

KEYWORD1 void KEYWORD2 NAME(GenTextures)(GLsizei n, GLuint * textures)
{
   DISPATCH(GenTextures, (n, textures), (F, "glGenTextures(%d, %p);\n", n, (const void *) textures));
}

#ifndef GLX_INDIRECT_RENDERING
KEYWORD1 void KEYWORD2 NAME(GenTexturesEXT)(GLsizei n, GLuint * textures)
{
   DISPATCH(GenTextures, (n, textures), (F, "glGenTexturesEXT(%d, %p);\n", n, (const void *) textures));
}
#endif /* GLX_INDIRECT_RENDERING */

KEYWORD1 void KEYWORD2 NAME(GetPointerv)(GLenum pname, GLvoid ** params)
{
   DISPATCH(GetPointerv, (pname, params), (F, "glGetPointerv(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetPointervEXT)(GLenum pname, GLvoid ** params)
{
   DISPATCH(GetPointerv, (pname, params), (F, "glGetPointervEXT(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsTexture)(GLuint texture)
{
   RETURN_DISPATCH(IsTexture, (texture), (F, "glIsTexture(%d);\n", texture));
}

#ifndef GLX_INDIRECT_RENDERING
KEYWORD1 GLboolean KEYWORD2 NAME(IsTextureEXT)(GLuint texture)
{
   RETURN_DISPATCH(IsTexture, (texture), (F, "glIsTextureEXT(%d);\n", texture));
}
#endif /* GLX_INDIRECT_RENDERING */

KEYWORD1 void KEYWORD2 NAME(PrioritizeTextures)(GLsizei n, const GLuint * textures, const GLclampf * priorities)
{
   DISPATCH(PrioritizeTextures, (n, textures, priorities), (F, "glPrioritizeTextures(%d, %p, %p);\n", n, (const void *) textures, (const void *) priorities));
}

KEYWORD1 void KEYWORD2 NAME(PrioritizeTexturesEXT)(GLsizei n, const GLuint * textures, const GLclampf * priorities)
{
   DISPATCH(PrioritizeTextures, (n, textures, priorities), (F, "glPrioritizeTexturesEXT(%d, %p, %p);\n", n, (const void *) textures, (const void *) priorities));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid * pixels)
{
   DISPATCH(TexSubImage1D, (target, level, xoffset, width, format, type, pixels), (F, "glTexSubImage1D(0x%x, %d, %d, %d, 0x%x, 0x%x, %p);\n", target, level, xoffset, width, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage1DEXT)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid * pixels)
{
   DISPATCH(TexSubImage1D, (target, level, xoffset, width, format, type, pixels), (F, "glTexSubImage1DEXT(0x%x, %d, %d, %d, 0x%x, 0x%x, %p);\n", target, level, xoffset, width, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels)
{
   DISPATCH(TexSubImage2D, (target, level, xoffset, yoffset, width, height, format, type, pixels), (F, "glTexSubImage2D(0x%x, %d, %d, %d, %d, %d, 0x%x, 0x%x, %p);\n", target, level, xoffset, yoffset, width, height, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage2DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels)
{
   DISPATCH(TexSubImage2D, (target, level, xoffset, yoffset, width, height, format, type, pixels), (F, "glTexSubImage2DEXT(0x%x, %d, %d, %d, %d, %d, 0x%x, 0x%x, %p);\n", target, level, xoffset, yoffset, width, height, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(PopClientAttrib)(void)
{
   DISPATCH(PopClientAttrib, (), (F, "glPopClientAttrib();\n"));
}

KEYWORD1 void KEYWORD2 NAME(PushClientAttrib)(GLbitfield mask)
{
   DISPATCH(PushClientAttrib, (mask), (F, "glPushClientAttrib(%d);\n", mask));
}

KEYWORD1 void KEYWORD2 NAME(BlendColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
   DISPATCH(BlendColor, (red, green, blue, alpha), (F, "glBlendColor(%f, %f, %f, %f);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(BlendColorEXT)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
   DISPATCH(BlendColor, (red, green, blue, alpha), (F, "glBlendColorEXT(%f, %f, %f, %f);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(BlendEquation)(GLenum mode)
{
   DISPATCH(BlendEquation, (mode), (F, "glBlendEquation(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(BlendEquationEXT)(GLenum mode)
{
   DISPATCH(BlendEquation, (mode), (F, "glBlendEquationEXT(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(DrawRangeElements)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid * indices)
{
   DISPATCH(DrawRangeElements, (mode, start, end, count, type, indices), (F, "glDrawRangeElements(0x%x, %d, %d, %d, 0x%x, %p);\n", mode, start, end, count, type, (const void *) indices));
}

KEYWORD1 void KEYWORD2 NAME(DrawRangeElementsEXT)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid * indices)
{
   DISPATCH(DrawRangeElements, (mode, start, end, count, type, indices), (F, "glDrawRangeElementsEXT(0x%x, %d, %d, %d, 0x%x, %p);\n", mode, start, end, count, type, (const void *) indices));
}

KEYWORD1 void KEYWORD2 NAME(ColorTable)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * table)
{
   DISPATCH(ColorTable, (target, internalformat, width, format, type, table), (F, "glColorTable(0x%x, 0x%x, %d, 0x%x, 0x%x, %p);\n", target, internalformat, width, format, type, (const void *) table));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_339)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * table);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_339)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * table)
{
   DISPATCH(ColorTable, (target, internalformat, width, format, type, table), (F, "glColorTableSGI(0x%x, 0x%x, %d, 0x%x, 0x%x, %p);\n", target, internalformat, width, format, type, (const void *) table));
}

KEYWORD1 void KEYWORD2 NAME(ColorTableEXT)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * table)
{
   DISPATCH(ColorTable, (target, internalformat, width, format, type, table), (F, "glColorTableEXT(0x%x, 0x%x, %d, 0x%x, 0x%x, %p);\n", target, internalformat, width, format, type, (const void *) table));
}

KEYWORD1 void KEYWORD2 NAME(ColorTableParameterfv)(GLenum target, GLenum pname, const GLfloat * params)
{
   DISPATCH(ColorTableParameterfv, (target, pname, params), (F, "glColorTableParameterfv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_340)(GLenum target, GLenum pname, const GLfloat * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_340)(GLenum target, GLenum pname, const GLfloat * params)
{
   DISPATCH(ColorTableParameterfv, (target, pname, params), (F, "glColorTableParameterfvSGI(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ColorTableParameteriv)(GLenum target, GLenum pname, const GLint * params)
{
   DISPATCH(ColorTableParameteriv, (target, pname, params), (F, "glColorTableParameteriv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_341)(GLenum target, GLenum pname, const GLint * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_341)(GLenum target, GLenum pname, const GLint * params)
{
   DISPATCH(ColorTableParameteriv, (target, pname, params), (F, "glColorTableParameterivSGI(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(CopyColorTable)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyColorTable, (target, internalformat, x, y, width), (F, "glCopyColorTable(0x%x, 0x%x, %d, %d, %d);\n", target, internalformat, x, y, width));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_342)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_342)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyColorTable, (target, internalformat, x, y, width), (F, "glCopyColorTableSGI(0x%x, 0x%x, %d, %d, %d);\n", target, internalformat, x, y, width));
}

KEYWORD1 void KEYWORD2 NAME(GetColorTable)(GLenum target, GLenum format, GLenum type, GLvoid * table)
{
   DISPATCH(GetColorTable, (target, format, type, table), (F, "glGetColorTable(0x%x, 0x%x, 0x%x, %p);\n", target, format, type, (const void *) table));
}

#ifndef GLX_INDIRECT_RENDERING
KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_343)(GLenum target, GLenum format, GLenum type, GLvoid * table);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_343)(GLenum target, GLenum format, GLenum type, GLvoid * table)
{
   DISPATCH(GetColorTable, (target, format, type, table), (F, "glGetColorTableSGI(0x%x, 0x%x, 0x%x, %p);\n", target, format, type, (const void *) table));
}
#endif /* GLX_INDIRECT_RENDERING */

#ifndef GLX_INDIRECT_RENDERING
KEYWORD1 void KEYWORD2 NAME(GetColorTableEXT)(GLenum target, GLenum format, GLenum type, GLvoid * table)
{
   DISPATCH(GetColorTable, (target, format, type, table), (F, "glGetColorTableEXT(0x%x, 0x%x, 0x%x, %p);\n", target, format, type, (const void *) table));
}
#endif /* GLX_INDIRECT_RENDERING */

KEYWORD1 void KEYWORD2 NAME(GetColorTableParameterfv)(GLenum target, GLenum pname, GLfloat * params)
{
   DISPATCH(GetColorTableParameterfv, (target, pname, params), (F, "glGetColorTableParameterfv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

#ifndef GLX_INDIRECT_RENDERING
KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_344)(GLenum target, GLenum pname, GLfloat * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_344)(GLenum target, GLenum pname, GLfloat * params)
{
   DISPATCH(GetColorTableParameterfv, (target, pname, params), (F, "glGetColorTableParameterfvSGI(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}
#endif /* GLX_INDIRECT_RENDERING */

#ifndef GLX_INDIRECT_RENDERING
KEYWORD1 void KEYWORD2 NAME(GetColorTableParameterfvEXT)(GLenum target, GLenum pname, GLfloat * params)
{
   DISPATCH(GetColorTableParameterfv, (target, pname, params), (F, "glGetColorTableParameterfvEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}
#endif /* GLX_INDIRECT_RENDERING */

KEYWORD1 void KEYWORD2 NAME(GetColorTableParameteriv)(GLenum target, GLenum pname, GLint * params)
{
   DISPATCH(GetColorTableParameteriv, (target, pname, params), (F, "glGetColorTableParameteriv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

#ifndef GLX_INDIRECT_RENDERING
KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_345)(GLenum target, GLenum pname, GLint * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_345)(GLenum target, GLenum pname, GLint * params)
{
   DISPATCH(GetColorTableParameteriv, (target, pname, params), (F, "glGetColorTableParameterivSGI(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}
#endif /* GLX_INDIRECT_RENDERING */

#ifndef GLX_INDIRECT_RENDERING
KEYWORD1 void KEYWORD2 NAME(GetColorTableParameterivEXT)(GLenum target, GLenum pname, GLint * params)
{
   DISPATCH(GetColorTableParameteriv, (target, pname, params), (F, "glGetColorTableParameterivEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}
#endif /* GLX_INDIRECT_RENDERING */

KEYWORD1 void KEYWORD2 NAME(ColorSubTable)(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid * data)
{
   DISPATCH(ColorSubTable, (target, start, count, format, type, data), (F, "glColorSubTable(0x%x, %d, %d, 0x%x, 0x%x, %p);\n", target, start, count, format, type, (const void *) data));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_346)(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid * data);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_346)(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid * data)
{
   DISPATCH(ColorSubTable, (target, start, count, format, type, data), (F, "glColorSubTableEXT(0x%x, %d, %d, 0x%x, 0x%x, %p);\n", target, start, count, format, type, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(CopyColorSubTable)(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyColorSubTable, (target, start, x, y, width), (F, "glCopyColorSubTable(0x%x, %d, %d, %d, %d);\n", target, start, x, y, width));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_347)(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_347)(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyColorSubTable, (target, start, x, y, width), (F, "glCopyColorSubTableEXT(0x%x, %d, %d, %d, %d);\n", target, start, x, y, width));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionFilter1D)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * image)
{
   DISPATCH(ConvolutionFilter1D, (target, internalformat, width, format, type, image), (F, "glConvolutionFilter1D(0x%x, 0x%x, %d, 0x%x, 0x%x, %p);\n", target, internalformat, width, format, type, (const void *) image));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_348)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * image);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_348)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * image)
{
   DISPATCH(ConvolutionFilter1D, (target, internalformat, width, format, type, image), (F, "glConvolutionFilter1DEXT(0x%x, 0x%x, %d, 0x%x, 0x%x, %p);\n", target, internalformat, width, format, type, (const void *) image));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionFilter2D)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * image)
{
   DISPATCH(ConvolutionFilter2D, (target, internalformat, width, height, format, type, image), (F, "glConvolutionFilter2D(0x%x, 0x%x, %d, %d, 0x%x, 0x%x, %p);\n", target, internalformat, width, height, format, type, (const void *) image));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_349)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * image);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_349)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * image)
{
   DISPATCH(ConvolutionFilter2D, (target, internalformat, width, height, format, type, image), (F, "glConvolutionFilter2DEXT(0x%x, 0x%x, %d, %d, 0x%x, 0x%x, %p);\n", target, internalformat, width, height, format, type, (const void *) image));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionParameterf)(GLenum target, GLenum pname, GLfloat params)
{
   DISPATCH(ConvolutionParameterf, (target, pname, params), (F, "glConvolutionParameterf(0x%x, 0x%x, %f);\n", target, pname, params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_350)(GLenum target, GLenum pname, GLfloat params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_350)(GLenum target, GLenum pname, GLfloat params)
{
   DISPATCH(ConvolutionParameterf, (target, pname, params), (F, "glConvolutionParameterfEXT(0x%x, 0x%x, %f);\n", target, pname, params));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionParameterfv)(GLenum target, GLenum pname, const GLfloat * params)
{
   DISPATCH(ConvolutionParameterfv, (target, pname, params), (F, "glConvolutionParameterfv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_351)(GLenum target, GLenum pname, const GLfloat * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_351)(GLenum target, GLenum pname, const GLfloat * params)
{
   DISPATCH(ConvolutionParameterfv, (target, pname, params), (F, "glConvolutionParameterfvEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionParameteri)(GLenum target, GLenum pname, GLint params)
{
   DISPATCH(ConvolutionParameteri, (target, pname, params), (F, "glConvolutionParameteri(0x%x, 0x%x, %d);\n", target, pname, params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_352)(GLenum target, GLenum pname, GLint params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_352)(GLenum target, GLenum pname, GLint params)
{
   DISPATCH(ConvolutionParameteri, (target, pname, params), (F, "glConvolutionParameteriEXT(0x%x, 0x%x, %d);\n", target, pname, params));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionParameteriv)(GLenum target, GLenum pname, const GLint * params)
{
   DISPATCH(ConvolutionParameteriv, (target, pname, params), (F, "glConvolutionParameteriv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_353)(GLenum target, GLenum pname, const GLint * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_353)(GLenum target, GLenum pname, const GLint * params)
{
   DISPATCH(ConvolutionParameteriv, (target, pname, params), (F, "glConvolutionParameterivEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(CopyConvolutionFilter1D)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyConvolutionFilter1D, (target, internalformat, x, y, width), (F, "glCopyConvolutionFilter1D(0x%x, 0x%x, %d, %d, %d);\n", target, internalformat, x, y, width));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_354)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_354)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
   DISPATCH(CopyConvolutionFilter1D, (target, internalformat, x, y, width), (F, "glCopyConvolutionFilter1DEXT(0x%x, 0x%x, %d, %d, %d);\n", target, internalformat, x, y, width));
}

KEYWORD1 void KEYWORD2 NAME(CopyConvolutionFilter2D)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(CopyConvolutionFilter2D, (target, internalformat, x, y, width, height), (F, "glCopyConvolutionFilter2D(0x%x, 0x%x, %d, %d, %d, %d);\n", target, internalformat, x, y, width, height));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_355)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_355)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(CopyConvolutionFilter2D, (target, internalformat, x, y, width, height), (F, "glCopyConvolutionFilter2DEXT(0x%x, 0x%x, %d, %d, %d, %d);\n", target, internalformat, x, y, width, height));
}

KEYWORD1 void KEYWORD2 NAME(GetConvolutionFilter)(GLenum target, GLenum format, GLenum type, GLvoid * image)
{
   DISPATCH(GetConvolutionFilter, (target, format, type, image), (F, "glGetConvolutionFilter(0x%x, 0x%x, 0x%x, %p);\n", target, format, type, (const void *) image));
}

#ifndef GLX_INDIRECT_RENDERING
KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_356)(GLenum target, GLenum format, GLenum type, GLvoid * image);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_356)(GLenum target, GLenum format, GLenum type, GLvoid * image)
{
   DISPATCH(GetConvolutionFilter, (target, format, type, image), (F, "glGetConvolutionFilterEXT(0x%x, 0x%x, 0x%x, %p);\n", target, format, type, (const void *) image));
}
#endif /* GLX_INDIRECT_RENDERING */

KEYWORD1 void KEYWORD2 NAME(GetConvolutionParameterfv)(GLenum target, GLenum pname, GLfloat * params)
{
   DISPATCH(GetConvolutionParameterfv, (target, pname, params), (F, "glGetConvolutionParameterfv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

#ifndef GLX_INDIRECT_RENDERING
KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_357)(GLenum target, GLenum pname, GLfloat * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_357)(GLenum target, GLenum pname, GLfloat * params)
{
   DISPATCH(GetConvolutionParameterfv, (target, pname, params), (F, "glGetConvolutionParameterfvEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}
#endif /* GLX_INDIRECT_RENDERING */

KEYWORD1 void KEYWORD2 NAME(GetConvolutionParameteriv)(GLenum target, GLenum pname, GLint * params)
{
   DISPATCH(GetConvolutionParameteriv, (target, pname, params), (F, "glGetConvolutionParameteriv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

#ifndef GLX_INDIRECT_RENDERING
KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_358)(GLenum target, GLenum pname, GLint * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_358)(GLenum target, GLenum pname, GLint * params)
{
   DISPATCH(GetConvolutionParameteriv, (target, pname, params), (F, "glGetConvolutionParameterivEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}
#endif /* GLX_INDIRECT_RENDERING */

KEYWORD1 void KEYWORD2 NAME(GetSeparableFilter)(GLenum target, GLenum format, GLenum type, GLvoid * row, GLvoid * column, GLvoid * span)
{
   DISPATCH(GetSeparableFilter, (target, format, type, row, column, span), (F, "glGetSeparableFilter(0x%x, 0x%x, 0x%x, %p, %p, %p);\n", target, format, type, (const void *) row, (const void *) column, (const void *) span));
}

#ifndef GLX_INDIRECT_RENDERING
KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_359)(GLenum target, GLenum format, GLenum type, GLvoid * row, GLvoid * column, GLvoid * span);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_359)(GLenum target, GLenum format, GLenum type, GLvoid * row, GLvoid * column, GLvoid * span)
{
   DISPATCH(GetSeparableFilter, (target, format, type, row, column, span), (F, "glGetSeparableFilterEXT(0x%x, 0x%x, 0x%x, %p, %p, %p);\n", target, format, type, (const void *) row, (const void *) column, (const void *) span));
}
#endif /* GLX_INDIRECT_RENDERING */

KEYWORD1 void KEYWORD2 NAME(SeparableFilter2D)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * row, const GLvoid * column)
{
   DISPATCH(SeparableFilter2D, (target, internalformat, width, height, format, type, row, column), (F, "glSeparableFilter2D(0x%x, 0x%x, %d, %d, 0x%x, 0x%x, %p, %p);\n", target, internalformat, width, height, format, type, (const void *) row, (const void *) column));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_360)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * row, const GLvoid * column);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_360)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * row, const GLvoid * column)
{
   DISPATCH(SeparableFilter2D, (target, internalformat, width, height, format, type, row, column), (F, "glSeparableFilter2DEXT(0x%x, 0x%x, %d, %d, 0x%x, 0x%x, %p, %p);\n", target, internalformat, width, height, format, type, (const void *) row, (const void *) column));
}

KEYWORD1 void KEYWORD2 NAME(GetHistogram)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values)
{
   DISPATCH(GetHistogram, (target, reset, format, type, values), (F, "glGetHistogram(0x%x, %d, 0x%x, 0x%x, %p);\n", target, reset, format, type, (const void *) values));
}

#ifndef GLX_INDIRECT_RENDERING
KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_361)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_361)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values)
{
   DISPATCH(GetHistogram, (target, reset, format, type, values), (F, "glGetHistogramEXT(0x%x, %d, 0x%x, 0x%x, %p);\n", target, reset, format, type, (const void *) values));
}
#endif /* GLX_INDIRECT_RENDERING */

KEYWORD1 void KEYWORD2 NAME(GetHistogramParameterfv)(GLenum target, GLenum pname, GLfloat * params)
{
   DISPATCH(GetHistogramParameterfv, (target, pname, params), (F, "glGetHistogramParameterfv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

#ifndef GLX_INDIRECT_RENDERING
KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_362)(GLenum target, GLenum pname, GLfloat * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_362)(GLenum target, GLenum pname, GLfloat * params)
{
   DISPATCH(GetHistogramParameterfv, (target, pname, params), (F, "glGetHistogramParameterfvEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}
#endif /* GLX_INDIRECT_RENDERING */

KEYWORD1 void KEYWORD2 NAME(GetHistogramParameteriv)(GLenum target, GLenum pname, GLint * params)
{
   DISPATCH(GetHistogramParameteriv, (target, pname, params), (F, "glGetHistogramParameteriv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

#ifndef GLX_INDIRECT_RENDERING
KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_363)(GLenum target, GLenum pname, GLint * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_363)(GLenum target, GLenum pname, GLint * params)
{
   DISPATCH(GetHistogramParameteriv, (target, pname, params), (F, "glGetHistogramParameterivEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}
#endif /* GLX_INDIRECT_RENDERING */

KEYWORD1 void KEYWORD2 NAME(GetMinmax)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values)
{
   DISPATCH(GetMinmax, (target, reset, format, type, values), (F, "glGetMinmax(0x%x, %d, 0x%x, 0x%x, %p);\n", target, reset, format, type, (const void *) values));
}

#ifndef GLX_INDIRECT_RENDERING
KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_364)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_364)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values)
{
   DISPATCH(GetMinmax, (target, reset, format, type, values), (F, "glGetMinmaxEXT(0x%x, %d, 0x%x, 0x%x, %p);\n", target, reset, format, type, (const void *) values));
}
#endif /* GLX_INDIRECT_RENDERING */

KEYWORD1 void KEYWORD2 NAME(GetMinmaxParameterfv)(GLenum target, GLenum pname, GLfloat * params)
{
   DISPATCH(GetMinmaxParameterfv, (target, pname, params), (F, "glGetMinmaxParameterfv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

#ifndef GLX_INDIRECT_RENDERING
KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_365)(GLenum target, GLenum pname, GLfloat * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_365)(GLenum target, GLenum pname, GLfloat * params)
{
   DISPATCH(GetMinmaxParameterfv, (target, pname, params), (F, "glGetMinmaxParameterfvEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}
#endif /* GLX_INDIRECT_RENDERING */

KEYWORD1 void KEYWORD2 NAME(GetMinmaxParameteriv)(GLenum target, GLenum pname, GLint * params)
{
   DISPATCH(GetMinmaxParameteriv, (target, pname, params), (F, "glGetMinmaxParameteriv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

#ifndef GLX_INDIRECT_RENDERING
KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_366)(GLenum target, GLenum pname, GLint * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_366)(GLenum target, GLenum pname, GLint * params)
{
   DISPATCH(GetMinmaxParameteriv, (target, pname, params), (F, "glGetMinmaxParameterivEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}
#endif /* GLX_INDIRECT_RENDERING */

KEYWORD1 void KEYWORD2 NAME(Histogram)(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink)
{
   DISPATCH(Histogram, (target, width, internalformat, sink), (F, "glHistogram(0x%x, %d, 0x%x, %d);\n", target, width, internalformat, sink));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_367)(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_367)(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink)
{
   DISPATCH(Histogram, (target, width, internalformat, sink), (F, "glHistogramEXT(0x%x, %d, 0x%x, %d);\n", target, width, internalformat, sink));
}

KEYWORD1 void KEYWORD2 NAME(Minmax)(GLenum target, GLenum internalformat, GLboolean sink)
{
   DISPATCH(Minmax, (target, internalformat, sink), (F, "glMinmax(0x%x, 0x%x, %d);\n", target, internalformat, sink));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_368)(GLenum target, GLenum internalformat, GLboolean sink);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_368)(GLenum target, GLenum internalformat, GLboolean sink)
{
   DISPATCH(Minmax, (target, internalformat, sink), (F, "glMinmaxEXT(0x%x, 0x%x, %d);\n", target, internalformat, sink));
}

KEYWORD1 void KEYWORD2 NAME(ResetHistogram)(GLenum target)
{
   DISPATCH(ResetHistogram, (target), (F, "glResetHistogram(0x%x);\n", target));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_369)(GLenum target);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_369)(GLenum target)
{
   DISPATCH(ResetHistogram, (target), (F, "glResetHistogramEXT(0x%x);\n", target));
}

KEYWORD1 void KEYWORD2 NAME(ResetMinmax)(GLenum target)
{
   DISPATCH(ResetMinmax, (target), (F, "glResetMinmax(0x%x);\n", target));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_370)(GLenum target);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_370)(GLenum target)
{
   DISPATCH(ResetMinmax, (target), (F, "glResetMinmaxEXT(0x%x);\n", target));
}

KEYWORD1 void KEYWORD2 NAME(TexImage3D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid * pixels)
{
   DISPATCH(TexImage3D, (target, level, internalformat, width, height, depth, border, format, type, pixels), (F, "glTexImage3D(0x%x, %d, %d, %d, %d, %d, %d, 0x%x, 0x%x, %p);\n", target, level, internalformat, width, height, depth, border, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(TexImage3DEXT)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid * pixels)
{
   DISPATCH(TexImage3D, (target, level, internalformat, width, height, depth, border, format, type, pixels), (F, "glTexImage3DEXT(0x%x, %d, %d, %d, %d, %d, %d, 0x%x, 0x%x, %p);\n", target, level, internalformat, width, height, depth, border, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid * pixels)
{
   DISPATCH(TexSubImage3D, (target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels), (F, "glTexSubImage3D(0x%x, %d, %d, %d, %d, %d, %d, %d, 0x%x, 0x%x, %p);\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage3DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid * pixels)
{
   DISPATCH(TexSubImage3D, (target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels), (F, "glTexSubImage3DEXT(0x%x, %d, %d, %d, %d, %d, %d, %d, 0x%x, 0x%x, %p);\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(CopyTexSubImage3D, (target, level, xoffset, yoffset, zoffset, x, y, width, height), (F, "glCopyTexSubImage3D(0x%x, %d, %d, %d, %d, %d, %d, %d, %d);\n", target, level, xoffset, yoffset, zoffset, x, y, width, height));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexSubImage3DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
   DISPATCH(CopyTexSubImage3D, (target, level, xoffset, yoffset, zoffset, x, y, width, height), (F, "glCopyTexSubImage3DEXT(0x%x, %d, %d, %d, %d, %d, %d, %d, %d);\n", target, level, xoffset, yoffset, zoffset, x, y, width, height));
}

KEYWORD1 void KEYWORD2 NAME(ActiveTexture)(GLenum texture)
{
   DISPATCH(ActiveTextureARB, (texture), (F, "glActiveTexture(0x%x);\n", texture));
}

KEYWORD1 void KEYWORD2 NAME(ActiveTextureARB)(GLenum texture)
{
   DISPATCH(ActiveTextureARB, (texture), (F, "glActiveTextureARB(0x%x);\n", texture));
}

KEYWORD1 void KEYWORD2 NAME(ClientActiveTexture)(GLenum texture)
{
   DISPATCH(ClientActiveTextureARB, (texture), (F, "glClientActiveTexture(0x%x);\n", texture));
}

KEYWORD1 void KEYWORD2 NAME(ClientActiveTextureARB)(GLenum texture)
{
   DISPATCH(ClientActiveTextureARB, (texture), (F, "glClientActiveTextureARB(0x%x);\n", texture));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1d)(GLenum target, GLdouble s)
{
   DISPATCH(MultiTexCoord1dARB, (target, s), (F, "glMultiTexCoord1d(0x%x, %f);\n", target, s));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1dARB)(GLenum target, GLdouble s)
{
   DISPATCH(MultiTexCoord1dARB, (target, s), (F, "glMultiTexCoord1dARB(0x%x, %f);\n", target, s));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1dv)(GLenum target, const GLdouble * v)
{
   DISPATCH(MultiTexCoord1dvARB, (target, v), (F, "glMultiTexCoord1dv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1dvARB)(GLenum target, const GLdouble * v)
{
   DISPATCH(MultiTexCoord1dvARB, (target, v), (F, "glMultiTexCoord1dvARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1f)(GLenum target, GLfloat s)
{
   DISPATCH(MultiTexCoord1fARB, (target, s), (F, "glMultiTexCoord1f(0x%x, %f);\n", target, s));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1fARB)(GLenum target, GLfloat s)
{
   DISPATCH(MultiTexCoord1fARB, (target, s), (F, "glMultiTexCoord1fARB(0x%x, %f);\n", target, s));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1fv)(GLenum target, const GLfloat * v)
{
   DISPATCH(MultiTexCoord1fvARB, (target, v), (F, "glMultiTexCoord1fv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1fvARB)(GLenum target, const GLfloat * v)
{
   DISPATCH(MultiTexCoord1fvARB, (target, v), (F, "glMultiTexCoord1fvARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1i)(GLenum target, GLint s)
{
   DISPATCH(MultiTexCoord1iARB, (target, s), (F, "glMultiTexCoord1i(0x%x, %d);\n", target, s));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1iARB)(GLenum target, GLint s)
{
   DISPATCH(MultiTexCoord1iARB, (target, s), (F, "glMultiTexCoord1iARB(0x%x, %d);\n", target, s));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1iv)(GLenum target, const GLint * v)
{
   DISPATCH(MultiTexCoord1ivARB, (target, v), (F, "glMultiTexCoord1iv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1ivARB)(GLenum target, const GLint * v)
{
   DISPATCH(MultiTexCoord1ivARB, (target, v), (F, "glMultiTexCoord1ivARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1s)(GLenum target, GLshort s)
{
   DISPATCH(MultiTexCoord1sARB, (target, s), (F, "glMultiTexCoord1s(0x%x, %d);\n", target, s));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1sARB)(GLenum target, GLshort s)
{
   DISPATCH(MultiTexCoord1sARB, (target, s), (F, "glMultiTexCoord1sARB(0x%x, %d);\n", target, s));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1sv)(GLenum target, const GLshort * v)
{
   DISPATCH(MultiTexCoord1svARB, (target, v), (F, "glMultiTexCoord1sv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1svARB)(GLenum target, const GLshort * v)
{
   DISPATCH(MultiTexCoord1svARB, (target, v), (F, "glMultiTexCoord1svARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2d)(GLenum target, GLdouble s, GLdouble t)
{
   DISPATCH(MultiTexCoord2dARB, (target, s, t), (F, "glMultiTexCoord2d(0x%x, %f, %f);\n", target, s, t));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2dARB)(GLenum target, GLdouble s, GLdouble t)
{
   DISPATCH(MultiTexCoord2dARB, (target, s, t), (F, "glMultiTexCoord2dARB(0x%x, %f, %f);\n", target, s, t));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2dv)(GLenum target, const GLdouble * v)
{
   DISPATCH(MultiTexCoord2dvARB, (target, v), (F, "glMultiTexCoord2dv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2dvARB)(GLenum target, const GLdouble * v)
{
   DISPATCH(MultiTexCoord2dvARB, (target, v), (F, "glMultiTexCoord2dvARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2f)(GLenum target, GLfloat s, GLfloat t)
{
   DISPATCH(MultiTexCoord2fARB, (target, s, t), (F, "glMultiTexCoord2f(0x%x, %f, %f);\n", target, s, t));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2fARB)(GLenum target, GLfloat s, GLfloat t)
{
   DISPATCH(MultiTexCoord2fARB, (target, s, t), (F, "glMultiTexCoord2fARB(0x%x, %f, %f);\n", target, s, t));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2fv)(GLenum target, const GLfloat * v)
{
   DISPATCH(MultiTexCoord2fvARB, (target, v), (F, "glMultiTexCoord2fv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2fvARB)(GLenum target, const GLfloat * v)
{
   DISPATCH(MultiTexCoord2fvARB, (target, v), (F, "glMultiTexCoord2fvARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2i)(GLenum target, GLint s, GLint t)
{
   DISPATCH(MultiTexCoord2iARB, (target, s, t), (F, "glMultiTexCoord2i(0x%x, %d, %d);\n", target, s, t));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2iARB)(GLenum target, GLint s, GLint t)
{
   DISPATCH(MultiTexCoord2iARB, (target, s, t), (F, "glMultiTexCoord2iARB(0x%x, %d, %d);\n", target, s, t));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2iv)(GLenum target, const GLint * v)
{
   DISPATCH(MultiTexCoord2ivARB, (target, v), (F, "glMultiTexCoord2iv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2ivARB)(GLenum target, const GLint * v)
{
   DISPATCH(MultiTexCoord2ivARB, (target, v), (F, "glMultiTexCoord2ivARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2s)(GLenum target, GLshort s, GLshort t)
{
   DISPATCH(MultiTexCoord2sARB, (target, s, t), (F, "glMultiTexCoord2s(0x%x, %d, %d);\n", target, s, t));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2sARB)(GLenum target, GLshort s, GLshort t)
{
   DISPATCH(MultiTexCoord2sARB, (target, s, t), (F, "glMultiTexCoord2sARB(0x%x, %d, %d);\n", target, s, t));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2sv)(GLenum target, const GLshort * v)
{
   DISPATCH(MultiTexCoord2svARB, (target, v), (F, "glMultiTexCoord2sv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2svARB)(GLenum target, const GLshort * v)
{
   DISPATCH(MultiTexCoord2svARB, (target, v), (F, "glMultiTexCoord2svARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3d)(GLenum target, GLdouble s, GLdouble t, GLdouble r)
{
   DISPATCH(MultiTexCoord3dARB, (target, s, t, r), (F, "glMultiTexCoord3d(0x%x, %f, %f, %f);\n", target, s, t, r));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3dARB)(GLenum target, GLdouble s, GLdouble t, GLdouble r)
{
   DISPATCH(MultiTexCoord3dARB, (target, s, t, r), (F, "glMultiTexCoord3dARB(0x%x, %f, %f, %f);\n", target, s, t, r));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3dv)(GLenum target, const GLdouble * v)
{
   DISPATCH(MultiTexCoord3dvARB, (target, v), (F, "glMultiTexCoord3dv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3dvARB)(GLenum target, const GLdouble * v)
{
   DISPATCH(MultiTexCoord3dvARB, (target, v), (F, "glMultiTexCoord3dvARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3f)(GLenum target, GLfloat s, GLfloat t, GLfloat r)
{
   DISPATCH(MultiTexCoord3fARB, (target, s, t, r), (F, "glMultiTexCoord3f(0x%x, %f, %f, %f);\n", target, s, t, r));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3fARB)(GLenum target, GLfloat s, GLfloat t, GLfloat r)
{
   DISPATCH(MultiTexCoord3fARB, (target, s, t, r), (F, "glMultiTexCoord3fARB(0x%x, %f, %f, %f);\n", target, s, t, r));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3fv)(GLenum target, const GLfloat * v)
{
   DISPATCH(MultiTexCoord3fvARB, (target, v), (F, "glMultiTexCoord3fv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3fvARB)(GLenum target, const GLfloat * v)
{
   DISPATCH(MultiTexCoord3fvARB, (target, v), (F, "glMultiTexCoord3fvARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3i)(GLenum target, GLint s, GLint t, GLint r)
{
   DISPATCH(MultiTexCoord3iARB, (target, s, t, r), (F, "glMultiTexCoord3i(0x%x, %d, %d, %d);\n", target, s, t, r));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3iARB)(GLenum target, GLint s, GLint t, GLint r)
{
   DISPATCH(MultiTexCoord3iARB, (target, s, t, r), (F, "glMultiTexCoord3iARB(0x%x, %d, %d, %d);\n", target, s, t, r));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3iv)(GLenum target, const GLint * v)
{
   DISPATCH(MultiTexCoord3ivARB, (target, v), (F, "glMultiTexCoord3iv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3ivARB)(GLenum target, const GLint * v)
{
   DISPATCH(MultiTexCoord3ivARB, (target, v), (F, "glMultiTexCoord3ivARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3s)(GLenum target, GLshort s, GLshort t, GLshort r)
{
   DISPATCH(MultiTexCoord3sARB, (target, s, t, r), (F, "glMultiTexCoord3s(0x%x, %d, %d, %d);\n", target, s, t, r));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3sARB)(GLenum target, GLshort s, GLshort t, GLshort r)
{
   DISPATCH(MultiTexCoord3sARB, (target, s, t, r), (F, "glMultiTexCoord3sARB(0x%x, %d, %d, %d);\n", target, s, t, r));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3sv)(GLenum target, const GLshort * v)
{
   DISPATCH(MultiTexCoord3svARB, (target, v), (F, "glMultiTexCoord3sv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3svARB)(GLenum target, const GLshort * v)
{
   DISPATCH(MultiTexCoord3svARB, (target, v), (F, "glMultiTexCoord3svARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4d)(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
   DISPATCH(MultiTexCoord4dARB, (target, s, t, r, q), (F, "glMultiTexCoord4d(0x%x, %f, %f, %f, %f);\n", target, s, t, r, q));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4dARB)(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
   DISPATCH(MultiTexCoord4dARB, (target, s, t, r, q), (F, "glMultiTexCoord4dARB(0x%x, %f, %f, %f, %f);\n", target, s, t, r, q));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4dv)(GLenum target, const GLdouble * v)
{
   DISPATCH(MultiTexCoord4dvARB, (target, v), (F, "glMultiTexCoord4dv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4dvARB)(GLenum target, const GLdouble * v)
{
   DISPATCH(MultiTexCoord4dvARB, (target, v), (F, "glMultiTexCoord4dvARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4f)(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
   DISPATCH(MultiTexCoord4fARB, (target, s, t, r, q), (F, "glMultiTexCoord4f(0x%x, %f, %f, %f, %f);\n", target, s, t, r, q));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4fARB)(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
   DISPATCH(MultiTexCoord4fARB, (target, s, t, r, q), (F, "glMultiTexCoord4fARB(0x%x, %f, %f, %f, %f);\n", target, s, t, r, q));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4fv)(GLenum target, const GLfloat * v)
{
   DISPATCH(MultiTexCoord4fvARB, (target, v), (F, "glMultiTexCoord4fv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4fvARB)(GLenum target, const GLfloat * v)
{
   DISPATCH(MultiTexCoord4fvARB, (target, v), (F, "glMultiTexCoord4fvARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4i)(GLenum target, GLint s, GLint t, GLint r, GLint q)
{
   DISPATCH(MultiTexCoord4iARB, (target, s, t, r, q), (F, "glMultiTexCoord4i(0x%x, %d, %d, %d, %d);\n", target, s, t, r, q));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4iARB)(GLenum target, GLint s, GLint t, GLint r, GLint q)
{
   DISPATCH(MultiTexCoord4iARB, (target, s, t, r, q), (F, "glMultiTexCoord4iARB(0x%x, %d, %d, %d, %d);\n", target, s, t, r, q));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4iv)(GLenum target, const GLint * v)
{
   DISPATCH(MultiTexCoord4ivARB, (target, v), (F, "glMultiTexCoord4iv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4ivARB)(GLenum target, const GLint * v)
{
   DISPATCH(MultiTexCoord4ivARB, (target, v), (F, "glMultiTexCoord4ivARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4s)(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q)
{
   DISPATCH(MultiTexCoord4sARB, (target, s, t, r, q), (F, "glMultiTexCoord4s(0x%x, %d, %d, %d, %d);\n", target, s, t, r, q));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4sARB)(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q)
{
   DISPATCH(MultiTexCoord4sARB, (target, s, t, r, q), (F, "glMultiTexCoord4sARB(0x%x, %d, %d, %d, %d);\n", target, s, t, r, q));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4sv)(GLenum target, const GLshort * v)
{
   DISPATCH(MultiTexCoord4svARB, (target, v), (F, "glMultiTexCoord4sv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4svARB)(GLenum target, const GLshort * v)
{
   DISPATCH(MultiTexCoord4svARB, (target, v), (F, "glMultiTexCoord4svARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(AttachShader)(GLuint program, GLuint shader)
{
   DISPATCH(AttachShader, (program, shader), (F, "glAttachShader(%d, %d);\n", program, shader));
}

KEYWORD1 GLuint KEYWORD2 NAME(CreateProgram)(void)
{
   RETURN_DISPATCH(CreateProgram, (), (F, "glCreateProgram();\n"));
}

KEYWORD1 GLuint KEYWORD2 NAME(CreateShader)(GLenum type)
{
   RETURN_DISPATCH(CreateShader, (type), (F, "glCreateShader(0x%x);\n", type));
}

KEYWORD1 void KEYWORD2 NAME(DeleteProgram)(GLuint program)
{
   DISPATCH(DeleteProgram, (program), (F, "glDeleteProgram(%d);\n", program));
}

KEYWORD1 void KEYWORD2 NAME(DeleteShader)(GLuint program)
{
   DISPATCH(DeleteShader, (program), (F, "glDeleteShader(%d);\n", program));
}

KEYWORD1 void KEYWORD2 NAME(DetachShader)(GLuint program, GLuint shader)
{
   DISPATCH(DetachShader, (program, shader), (F, "glDetachShader(%d, %d);\n", program, shader));
}

KEYWORD1 void KEYWORD2 NAME(GetAttachedShaders)(GLuint program, GLsizei maxCount, GLsizei * count, GLuint * obj)
{
   DISPATCH(GetAttachedShaders, (program, maxCount, count, obj), (F, "glGetAttachedShaders(%d, %d, %p, %p);\n", program, maxCount, (const void *) count, (const void *) obj));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei * length, GLchar * infoLog)
{
   DISPATCH(GetProgramInfoLog, (program, bufSize, length, infoLog), (F, "glGetProgramInfoLog(%d, %d, %p, %p);\n", program, bufSize, (const void *) length, (const void *) infoLog));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramiv)(GLuint program, GLenum pname, GLint * params)
{
   DISPATCH(GetProgramiv, (program, pname, params), (F, "glGetProgramiv(%d, 0x%x, %p);\n", program, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * infoLog)
{
   DISPATCH(GetShaderInfoLog, (shader, bufSize, length, infoLog), (F, "glGetShaderInfoLog(%d, %d, %p, %p);\n", shader, bufSize, (const void *) length, (const void *) infoLog));
}

KEYWORD1 void KEYWORD2 NAME(GetShaderiv)(GLuint shader, GLenum pname, GLint * params)
{
   DISPATCH(GetShaderiv, (shader, pname, params), (F, "glGetShaderiv(%d, 0x%x, %p);\n", shader, pname, (const void *) params));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsProgram)(GLuint program)
{
   RETURN_DISPATCH(IsProgram, (program), (F, "glIsProgram(%d);\n", program));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsShader)(GLuint shader)
{
   RETURN_DISPATCH(IsShader, (shader), (F, "glIsShader(%d);\n", shader));
}

KEYWORD1 void KEYWORD2 NAME(StencilFuncSeparate)(GLenum face, GLenum func, GLint ref, GLuint mask)
{
   DISPATCH(StencilFuncSeparate, (face, func, ref, mask), (F, "glStencilFuncSeparate(0x%x, 0x%x, %d, %d);\n", face, func, ref, mask));
}

KEYWORD1 void KEYWORD2 NAME(StencilMaskSeparate)(GLenum face, GLuint mask)
{
   DISPATCH(StencilMaskSeparate, (face, mask), (F, "glStencilMaskSeparate(0x%x, %d);\n", face, mask));
}

KEYWORD1 void KEYWORD2 NAME(StencilOpSeparate)(GLenum face, GLenum sfail, GLenum zfail, GLenum zpass)
{
   DISPATCH(StencilOpSeparate, (face, sfail, zfail, zpass), (F, "glStencilOpSeparate(0x%x, 0x%x, 0x%x, 0x%x);\n", face, sfail, zfail, zpass));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_423)(GLenum face, GLenum sfail, GLenum zfail, GLenum zpass);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_423)(GLenum face, GLenum sfail, GLenum zfail, GLenum zpass)
{
   DISPATCH(StencilOpSeparate, (face, sfail, zfail, zpass), (F, "glStencilOpSeparateATI(0x%x, 0x%x, 0x%x, 0x%x);\n", face, sfail, zfail, zpass));
}

KEYWORD1 void KEYWORD2 NAME(UniformMatrix2x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
   DISPATCH(UniformMatrix2x3fv, (location, count, transpose, value), (F, "glUniformMatrix2x3fv(%d, %d, %d, %p);\n", location, count, transpose, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(UniformMatrix2x4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
   DISPATCH(UniformMatrix2x4fv, (location, count, transpose, value), (F, "glUniformMatrix2x4fv(%d, %d, %d, %p);\n", location, count, transpose, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(UniformMatrix3x2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
   DISPATCH(UniformMatrix3x2fv, (location, count, transpose, value), (F, "glUniformMatrix3x2fv(%d, %d, %d, %p);\n", location, count, transpose, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(UniformMatrix3x4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
   DISPATCH(UniformMatrix3x4fv, (location, count, transpose, value), (F, "glUniformMatrix3x4fv(%d, %d, %d, %p);\n", location, count, transpose, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(UniformMatrix4x2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
   DISPATCH(UniformMatrix4x2fv, (location, count, transpose, value), (F, "glUniformMatrix4x2fv(%d, %d, %d, %p);\n", location, count, transpose, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(UniformMatrix4x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
   DISPATCH(UniformMatrix4x3fv, (location, count, transpose, value), (F, "glUniformMatrix4x3fv(%d, %d, %d, %p);\n", location, count, transpose, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(LoadTransposeMatrixd)(const GLdouble * m)
{
   DISPATCH(LoadTransposeMatrixdARB, (m), (F, "glLoadTransposeMatrixd(%p);\n", (const void *) m));
}

KEYWORD1 void KEYWORD2 NAME(LoadTransposeMatrixdARB)(const GLdouble * m)
{
   DISPATCH(LoadTransposeMatrixdARB, (m), (F, "glLoadTransposeMatrixdARB(%p);\n", (const void *) m));
}

KEYWORD1 void KEYWORD2 NAME(LoadTransposeMatrixf)(const GLfloat * m)
{
   DISPATCH(LoadTransposeMatrixfARB, (m), (F, "glLoadTransposeMatrixf(%p);\n", (const void *) m));
}

KEYWORD1 void KEYWORD2 NAME(LoadTransposeMatrixfARB)(const GLfloat * m)
{
   DISPATCH(LoadTransposeMatrixfARB, (m), (F, "glLoadTransposeMatrixfARB(%p);\n", (const void *) m));
}

KEYWORD1 void KEYWORD2 NAME(MultTransposeMatrixd)(const GLdouble * m)
{
   DISPATCH(MultTransposeMatrixdARB, (m), (F, "glMultTransposeMatrixd(%p);\n", (const void *) m));
}

KEYWORD1 void KEYWORD2 NAME(MultTransposeMatrixdARB)(const GLdouble * m)
{
   DISPATCH(MultTransposeMatrixdARB, (m), (F, "glMultTransposeMatrixdARB(%p);\n", (const void *) m));
}

KEYWORD1 void KEYWORD2 NAME(MultTransposeMatrixf)(const GLfloat * m)
{
   DISPATCH(MultTransposeMatrixfARB, (m), (F, "glMultTransposeMatrixf(%p);\n", (const void *) m));
}

KEYWORD1 void KEYWORD2 NAME(MultTransposeMatrixfARB)(const GLfloat * m)
{
   DISPATCH(MultTransposeMatrixfARB, (m), (F, "glMultTransposeMatrixfARB(%p);\n", (const void *) m));
}

KEYWORD1 void KEYWORD2 NAME(SampleCoverage)(GLclampf value, GLboolean invert)
{
   DISPATCH(SampleCoverageARB, (value, invert), (F, "glSampleCoverage(%f, %d);\n", value, invert));
}

KEYWORD1 void KEYWORD2 NAME(SampleCoverageARB)(GLclampf value, GLboolean invert)
{
   DISPATCH(SampleCoverageARB, (value, invert), (F, "glSampleCoverageARB(%f, %d);\n", value, invert));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexImage1D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid * data)
{
   DISPATCH(CompressedTexImage1DARB, (target, level, internalformat, width, border, imageSize, data), (F, "glCompressedTexImage1D(0x%x, %d, 0x%x, %d, %d, %d, %p);\n", target, level, internalformat, width, border, imageSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexImage1DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid * data)
{
   DISPATCH(CompressedTexImage1DARB, (target, level, internalformat, width, border, imageSize, data), (F, "glCompressedTexImage1DARB(0x%x, %d, 0x%x, %d, %d, %d, %p);\n", target, level, internalformat, width, border, imageSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid * data)
{
   DISPATCH(CompressedTexImage2DARB, (target, level, internalformat, width, height, border, imageSize, data), (F, "glCompressedTexImage2D(0x%x, %d, 0x%x, %d, %d, %d, %d, %p);\n", target, level, internalformat, width, height, border, imageSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexImage2DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid * data)
{
   DISPATCH(CompressedTexImage2DARB, (target, level, internalformat, width, height, border, imageSize, data), (F, "glCompressedTexImage2DARB(0x%x, %d, 0x%x, %d, %d, %d, %d, %p);\n", target, level, internalformat, width, height, border, imageSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexImage3D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid * data)
{
   DISPATCH(CompressedTexImage3DARB, (target, level, internalformat, width, height, depth, border, imageSize, data), (F, "glCompressedTexImage3D(0x%x, %d, 0x%x, %d, %d, %d, %d, %d, %p);\n", target, level, internalformat, width, height, depth, border, imageSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexImage3DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid * data)
{
   DISPATCH(CompressedTexImage3DARB, (target, level, internalformat, width, height, depth, border, imageSize, data), (F, "glCompressedTexImage3DARB(0x%x, %d, 0x%x, %d, %d, %d, %d, %d, %p);\n", target, level, internalformat, width, height, depth, border, imageSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid * data)
{
   DISPATCH(CompressedTexSubImage1DARB, (target, level, xoffset, width, format, imageSize, data), (F, "glCompressedTexSubImage1D(0x%x, %d, %d, %d, 0x%x, %d, %p);\n", target, level, xoffset, width, format, imageSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexSubImage1DARB)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid * data)
{
   DISPATCH(CompressedTexSubImage1DARB, (target, level, xoffset, width, format, imageSize, data), (F, "glCompressedTexSubImage1DARB(0x%x, %d, %d, %d, 0x%x, %d, %p);\n", target, level, xoffset, width, format, imageSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid * data)
{
   DISPATCH(CompressedTexSubImage2DARB, (target, level, xoffset, yoffset, width, height, format, imageSize, data), (F, "glCompressedTexSubImage2D(0x%x, %d, %d, %d, %d, %d, 0x%x, %d, %p);\n", target, level, xoffset, yoffset, width, height, format, imageSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexSubImage2DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid * data)
{
   DISPATCH(CompressedTexSubImage2DARB, (target, level, xoffset, yoffset, width, height, format, imageSize, data), (F, "glCompressedTexSubImage2DARB(0x%x, %d, %d, %d, %d, %d, 0x%x, %d, %p);\n", target, level, xoffset, yoffset, width, height, format, imageSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid * data)
{
   DISPATCH(CompressedTexSubImage3DARB, (target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data), (F, "glCompressedTexSubImage3D(0x%x, %d, %d, %d, %d, %d, %d, %d, 0x%x, %d, %p);\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexSubImage3DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid * data)
{
   DISPATCH(CompressedTexSubImage3DARB, (target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data), (F, "glCompressedTexSubImage3DARB(0x%x, %d, %d, %d, %d, %d, %d, %d, 0x%x, %d, %p);\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(GetCompressedTexImage)(GLenum target, GLint level, GLvoid * img)
{
   DISPATCH(GetCompressedTexImageARB, (target, level, img), (F, "glGetCompressedTexImage(0x%x, %d, %p);\n", target, level, (const void *) img));
}

KEYWORD1 void KEYWORD2 NAME(GetCompressedTexImageARB)(GLenum target, GLint level, GLvoid * img)
{
   DISPATCH(GetCompressedTexImageARB, (target, level, img), (F, "glGetCompressedTexImageARB(0x%x, %d, %p);\n", target, level, (const void *) img));
}

KEYWORD1 void KEYWORD2 NAME(DisableVertexAttribArray)(GLuint index)
{
   DISPATCH(DisableVertexAttribArrayARB, (index), (F, "glDisableVertexAttribArray(%d);\n", index));
}

KEYWORD1 void KEYWORD2 NAME(DisableVertexAttribArrayARB)(GLuint index)
{
   DISPATCH(DisableVertexAttribArrayARB, (index), (F, "glDisableVertexAttribArrayARB(%d);\n", index));
}

KEYWORD1 void KEYWORD2 NAME(EnableVertexAttribArray)(GLuint index)
{
   DISPATCH(EnableVertexAttribArrayARB, (index), (F, "glEnableVertexAttribArray(%d);\n", index));
}

KEYWORD1 void KEYWORD2 NAME(EnableVertexAttribArrayARB)(GLuint index)
{
   DISPATCH(EnableVertexAttribArrayARB, (index), (F, "glEnableVertexAttribArrayARB(%d);\n", index));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramEnvParameterdvARB)(GLenum target, GLuint index, GLdouble * params)
{
   DISPATCH(GetProgramEnvParameterdvARB, (target, index, params), (F, "glGetProgramEnvParameterdvARB(0x%x, %d, %p);\n", target, index, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramEnvParameterfvARB)(GLenum target, GLuint index, GLfloat * params)
{
   DISPATCH(GetProgramEnvParameterfvARB, (target, index, params), (F, "glGetProgramEnvParameterfvARB(0x%x, %d, %p);\n", target, index, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramLocalParameterdvARB)(GLenum target, GLuint index, GLdouble * params)
{
   DISPATCH(GetProgramLocalParameterdvARB, (target, index, params), (F, "glGetProgramLocalParameterdvARB(0x%x, %d, %p);\n", target, index, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramLocalParameterfvARB)(GLenum target, GLuint index, GLfloat * params)
{
   DISPATCH(GetProgramLocalParameterfvARB, (target, index, params), (F, "glGetProgramLocalParameterfvARB(0x%x, %d, %p);\n", target, index, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramStringARB)(GLenum target, GLenum pname, GLvoid * string)
{
   DISPATCH(GetProgramStringARB, (target, pname, string), (F, "glGetProgramStringARB(0x%x, 0x%x, %p);\n", target, pname, (const void *) string));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramivARB)(GLenum target, GLenum pname, GLint * params)
{
   DISPATCH(GetProgramivARB, (target, pname, params), (F, "glGetProgramivARB(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribdv)(GLuint index, GLenum pname, GLdouble * params)
{
   DISPATCH(GetVertexAttribdvARB, (index, pname, params), (F, "glGetVertexAttribdv(%d, 0x%x, %p);\n", index, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribdvARB)(GLuint index, GLenum pname, GLdouble * params)
{
   DISPATCH(GetVertexAttribdvARB, (index, pname, params), (F, "glGetVertexAttribdvARB(%d, 0x%x, %p);\n", index, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribfv)(GLuint index, GLenum pname, GLfloat * params)
{
   DISPATCH(GetVertexAttribfvARB, (index, pname, params), (F, "glGetVertexAttribfv(%d, 0x%x, %p);\n", index, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribfvARB)(GLuint index, GLenum pname, GLfloat * params)
{
   DISPATCH(GetVertexAttribfvARB, (index, pname, params), (F, "glGetVertexAttribfvARB(%d, 0x%x, %p);\n", index, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribiv)(GLuint index, GLenum pname, GLint * params)
{
   DISPATCH(GetVertexAttribivARB, (index, pname, params), (F, "glGetVertexAttribiv(%d, 0x%x, %p);\n", index, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribivARB)(GLuint index, GLenum pname, GLint * params)
{
   DISPATCH(GetVertexAttribivARB, (index, pname, params), (F, "glGetVertexAttribivARB(%d, 0x%x, %p);\n", index, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ProgramEnvParameter4dARB)(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   DISPATCH(ProgramEnvParameter4dARB, (target, index, x, y, z, w), (F, "glProgramEnvParameter4dARB(0x%x, %d, %f, %f, %f, %f);\n", target, index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(ProgramParameter4dNV)(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   DISPATCH(ProgramEnvParameter4dARB, (target, index, x, y, z, w), (F, "glProgramParameter4dNV(0x%x, %d, %f, %f, %f, %f);\n", target, index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(ProgramEnvParameter4dvARB)(GLenum target, GLuint index, const GLdouble * params)
{
   DISPATCH(ProgramEnvParameter4dvARB, (target, index, params), (F, "glProgramEnvParameter4dvARB(0x%x, %d, %p);\n", target, index, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ProgramParameter4dvNV)(GLenum target, GLuint index, const GLdouble * params)
{
   DISPATCH(ProgramEnvParameter4dvARB, (target, index, params), (F, "glProgramParameter4dvNV(0x%x, %d, %p);\n", target, index, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ProgramEnvParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   DISPATCH(ProgramEnvParameter4fARB, (target, index, x, y, z, w), (F, "glProgramEnvParameter4fARB(0x%x, %d, %f, %f, %f, %f);\n", target, index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(ProgramParameter4fNV)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   DISPATCH(ProgramEnvParameter4fARB, (target, index, x, y, z, w), (F, "glProgramParameter4fNV(0x%x, %d, %f, %f, %f, %f);\n", target, index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(ProgramEnvParameter4fvARB)(GLenum target, GLuint index, const GLfloat * params)
{
   DISPATCH(ProgramEnvParameter4fvARB, (target, index, params), (F, "glProgramEnvParameter4fvARB(0x%x, %d, %p);\n", target, index, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ProgramParameter4fvNV)(GLenum target, GLuint index, const GLfloat * params)
{
   DISPATCH(ProgramEnvParameter4fvARB, (target, index, params), (F, "glProgramParameter4fvNV(0x%x, %d, %p);\n", target, index, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ProgramLocalParameter4dARB)(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   DISPATCH(ProgramLocalParameter4dARB, (target, index, x, y, z, w), (F, "glProgramLocalParameter4dARB(0x%x, %d, %f, %f, %f, %f);\n", target, index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(ProgramLocalParameter4dvARB)(GLenum target, GLuint index, const GLdouble * params)
{
   DISPATCH(ProgramLocalParameter4dvARB, (target, index, params), (F, "glProgramLocalParameter4dvARB(0x%x, %d, %p);\n", target, index, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ProgramLocalParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   DISPATCH(ProgramLocalParameter4fARB, (target, index, x, y, z, w), (F, "glProgramLocalParameter4fARB(0x%x, %d, %f, %f, %f, %f);\n", target, index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(ProgramLocalParameter4fvARB)(GLenum target, GLuint index, const GLfloat * params)
{
   DISPATCH(ProgramLocalParameter4fvARB, (target, index, params), (F, "glProgramLocalParameter4fvARB(0x%x, %d, %p);\n", target, index, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ProgramStringARB)(GLenum target, GLenum format, GLsizei len, const GLvoid * string)
{
   DISPATCH(ProgramStringARB, (target, format, len, string), (F, "glProgramStringARB(0x%x, 0x%x, %d, %p);\n", target, format, len, (const void *) string));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1d)(GLuint index, GLdouble x)
{
   DISPATCH(VertexAttrib1dARB, (index, x), (F, "glVertexAttrib1d(%d, %f);\n", index, x));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1dARB)(GLuint index, GLdouble x)
{
   DISPATCH(VertexAttrib1dARB, (index, x), (F, "glVertexAttrib1dARB(%d, %f);\n", index, x));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1dv)(GLuint index, const GLdouble * v)
{
   DISPATCH(VertexAttrib1dvARB, (index, v), (F, "glVertexAttrib1dv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1dvARB)(GLuint index, const GLdouble * v)
{
   DISPATCH(VertexAttrib1dvARB, (index, v), (F, "glVertexAttrib1dvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1f)(GLuint index, GLfloat x)
{
   DISPATCH(VertexAttrib1fARB, (index, x), (F, "glVertexAttrib1f(%d, %f);\n", index, x));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1fARB)(GLuint index, GLfloat x)
{
   DISPATCH(VertexAttrib1fARB, (index, x), (F, "glVertexAttrib1fARB(%d, %f);\n", index, x));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1fv)(GLuint index, const GLfloat * v)
{
   DISPATCH(VertexAttrib1fvARB, (index, v), (F, "glVertexAttrib1fv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1fvARB)(GLuint index, const GLfloat * v)
{
   DISPATCH(VertexAttrib1fvARB, (index, v), (F, "glVertexAttrib1fvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1s)(GLuint index, GLshort x)
{
   DISPATCH(VertexAttrib1sARB, (index, x), (F, "glVertexAttrib1s(%d, %d);\n", index, x));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1sARB)(GLuint index, GLshort x)
{
   DISPATCH(VertexAttrib1sARB, (index, x), (F, "glVertexAttrib1sARB(%d, %d);\n", index, x));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1sv)(GLuint index, const GLshort * v)
{
   DISPATCH(VertexAttrib1svARB, (index, v), (F, "glVertexAttrib1sv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1svARB)(GLuint index, const GLshort * v)
{
   DISPATCH(VertexAttrib1svARB, (index, v), (F, "glVertexAttrib1svARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2d)(GLuint index, GLdouble x, GLdouble y)
{
   DISPATCH(VertexAttrib2dARB, (index, x, y), (F, "glVertexAttrib2d(%d, %f, %f);\n", index, x, y));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2dARB)(GLuint index, GLdouble x, GLdouble y)
{
   DISPATCH(VertexAttrib2dARB, (index, x, y), (F, "glVertexAttrib2dARB(%d, %f, %f);\n", index, x, y));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2dv)(GLuint index, const GLdouble * v)
{
   DISPATCH(VertexAttrib2dvARB, (index, v), (F, "glVertexAttrib2dv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2dvARB)(GLuint index, const GLdouble * v)
{
   DISPATCH(VertexAttrib2dvARB, (index, v), (F, "glVertexAttrib2dvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2f)(GLuint index, GLfloat x, GLfloat y)
{
   DISPATCH(VertexAttrib2fARB, (index, x, y), (F, "glVertexAttrib2f(%d, %f, %f);\n", index, x, y));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2fARB)(GLuint index, GLfloat x, GLfloat y)
{
   DISPATCH(VertexAttrib2fARB, (index, x, y), (F, "glVertexAttrib2fARB(%d, %f, %f);\n", index, x, y));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2fv)(GLuint index, const GLfloat * v)
{
   DISPATCH(VertexAttrib2fvARB, (index, v), (F, "glVertexAttrib2fv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2fvARB)(GLuint index, const GLfloat * v)
{
   DISPATCH(VertexAttrib2fvARB, (index, v), (F, "glVertexAttrib2fvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2s)(GLuint index, GLshort x, GLshort y)
{
   DISPATCH(VertexAttrib2sARB, (index, x, y), (F, "glVertexAttrib2s(%d, %d, %d);\n", index, x, y));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2sARB)(GLuint index, GLshort x, GLshort y)
{
   DISPATCH(VertexAttrib2sARB, (index, x, y), (F, "glVertexAttrib2sARB(%d, %d, %d);\n", index, x, y));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2sv)(GLuint index, const GLshort * v)
{
   DISPATCH(VertexAttrib2svARB, (index, v), (F, "glVertexAttrib2sv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2svARB)(GLuint index, const GLshort * v)
{
   DISPATCH(VertexAttrib2svARB, (index, v), (F, "glVertexAttrib2svARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3d)(GLuint index, GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(VertexAttrib3dARB, (index, x, y, z), (F, "glVertexAttrib3d(%d, %f, %f, %f);\n", index, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3dARB)(GLuint index, GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(VertexAttrib3dARB, (index, x, y, z), (F, "glVertexAttrib3dARB(%d, %f, %f, %f);\n", index, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3dv)(GLuint index, const GLdouble * v)
{
   DISPATCH(VertexAttrib3dvARB, (index, v), (F, "glVertexAttrib3dv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3dvARB)(GLuint index, const GLdouble * v)
{
   DISPATCH(VertexAttrib3dvARB, (index, v), (F, "glVertexAttrib3dvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3f)(GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(VertexAttrib3fARB, (index, x, y, z), (F, "glVertexAttrib3f(%d, %f, %f, %f);\n", index, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3fARB)(GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(VertexAttrib3fARB, (index, x, y, z), (F, "glVertexAttrib3fARB(%d, %f, %f, %f);\n", index, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3fv)(GLuint index, const GLfloat * v)
{
   DISPATCH(VertexAttrib3fvARB, (index, v), (F, "glVertexAttrib3fv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3fvARB)(GLuint index, const GLfloat * v)
{
   DISPATCH(VertexAttrib3fvARB, (index, v), (F, "glVertexAttrib3fvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3s)(GLuint index, GLshort x, GLshort y, GLshort z)
{
   DISPATCH(VertexAttrib3sARB, (index, x, y, z), (F, "glVertexAttrib3s(%d, %d, %d, %d);\n", index, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3sARB)(GLuint index, GLshort x, GLshort y, GLshort z)
{
   DISPATCH(VertexAttrib3sARB, (index, x, y, z), (F, "glVertexAttrib3sARB(%d, %d, %d, %d);\n", index, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3sv)(GLuint index, const GLshort * v)
{
   DISPATCH(VertexAttrib3svARB, (index, v), (F, "glVertexAttrib3sv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3svARB)(GLuint index, const GLshort * v)
{
   DISPATCH(VertexAttrib3svARB, (index, v), (F, "glVertexAttrib3svARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4Nbv)(GLuint index, const GLbyte * v)
{
   DISPATCH(VertexAttrib4NbvARB, (index, v), (F, "glVertexAttrib4Nbv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4NbvARB)(GLuint index, const GLbyte * v)
{
   DISPATCH(VertexAttrib4NbvARB, (index, v), (F, "glVertexAttrib4NbvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4Niv)(GLuint index, const GLint * v)
{
   DISPATCH(VertexAttrib4NivARB, (index, v), (F, "glVertexAttrib4Niv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4NivARB)(GLuint index, const GLint * v)
{
   DISPATCH(VertexAttrib4NivARB, (index, v), (F, "glVertexAttrib4NivARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4Nsv)(GLuint index, const GLshort * v)
{
   DISPATCH(VertexAttrib4NsvARB, (index, v), (F, "glVertexAttrib4Nsv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4NsvARB)(GLuint index, const GLshort * v)
{
   DISPATCH(VertexAttrib4NsvARB, (index, v), (F, "glVertexAttrib4NsvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4Nub)(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w)
{
   DISPATCH(VertexAttrib4NubARB, (index, x, y, z, w), (F, "glVertexAttrib4Nub(%d, %d, %d, %d, %d);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4NubARB)(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w)
{
   DISPATCH(VertexAttrib4NubARB, (index, x, y, z, w), (F, "glVertexAttrib4NubARB(%d, %d, %d, %d, %d);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4Nubv)(GLuint index, const GLubyte * v)
{
   DISPATCH(VertexAttrib4NubvARB, (index, v), (F, "glVertexAttrib4Nubv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4NubvARB)(GLuint index, const GLubyte * v)
{
   DISPATCH(VertexAttrib4NubvARB, (index, v), (F, "glVertexAttrib4NubvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4Nuiv)(GLuint index, const GLuint * v)
{
   DISPATCH(VertexAttrib4NuivARB, (index, v), (F, "glVertexAttrib4Nuiv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4NuivARB)(GLuint index, const GLuint * v)
{
   DISPATCH(VertexAttrib4NuivARB, (index, v), (F, "glVertexAttrib4NuivARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4Nusv)(GLuint index, const GLushort * v)
{
   DISPATCH(VertexAttrib4NusvARB, (index, v), (F, "glVertexAttrib4Nusv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4NusvARB)(GLuint index, const GLushort * v)
{
   DISPATCH(VertexAttrib4NusvARB, (index, v), (F, "glVertexAttrib4NusvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4bv)(GLuint index, const GLbyte * v)
{
   DISPATCH(VertexAttrib4bvARB, (index, v), (F, "glVertexAttrib4bv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4bvARB)(GLuint index, const GLbyte * v)
{
   DISPATCH(VertexAttrib4bvARB, (index, v), (F, "glVertexAttrib4bvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4d)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   DISPATCH(VertexAttrib4dARB, (index, x, y, z, w), (F, "glVertexAttrib4d(%d, %f, %f, %f, %f);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4dARB)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   DISPATCH(VertexAttrib4dARB, (index, x, y, z, w), (F, "glVertexAttrib4dARB(%d, %f, %f, %f, %f);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4dv)(GLuint index, const GLdouble * v)
{
   DISPATCH(VertexAttrib4dvARB, (index, v), (F, "glVertexAttrib4dv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4dvARB)(GLuint index, const GLdouble * v)
{
   DISPATCH(VertexAttrib4dvARB, (index, v), (F, "glVertexAttrib4dvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4f)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   DISPATCH(VertexAttrib4fARB, (index, x, y, z, w), (F, "glVertexAttrib4f(%d, %f, %f, %f, %f);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4fARB)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   DISPATCH(VertexAttrib4fARB, (index, x, y, z, w), (F, "glVertexAttrib4fARB(%d, %f, %f, %f, %f);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4fv)(GLuint index, const GLfloat * v)
{
   DISPATCH(VertexAttrib4fvARB, (index, v), (F, "glVertexAttrib4fv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4fvARB)(GLuint index, const GLfloat * v)
{
   DISPATCH(VertexAttrib4fvARB, (index, v), (F, "glVertexAttrib4fvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4iv)(GLuint index, const GLint * v)
{
   DISPATCH(VertexAttrib4ivARB, (index, v), (F, "glVertexAttrib4iv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4ivARB)(GLuint index, const GLint * v)
{
   DISPATCH(VertexAttrib4ivARB, (index, v), (F, "glVertexAttrib4ivARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4s)(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w)
{
   DISPATCH(VertexAttrib4sARB, (index, x, y, z, w), (F, "glVertexAttrib4s(%d, %d, %d, %d, %d);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4sARB)(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w)
{
   DISPATCH(VertexAttrib4sARB, (index, x, y, z, w), (F, "glVertexAttrib4sARB(%d, %d, %d, %d, %d);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4sv)(GLuint index, const GLshort * v)
{
   DISPATCH(VertexAttrib4svARB, (index, v), (F, "glVertexAttrib4sv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4svARB)(GLuint index, const GLshort * v)
{
   DISPATCH(VertexAttrib4svARB, (index, v), (F, "glVertexAttrib4svARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4ubv)(GLuint index, const GLubyte * v)
{
   DISPATCH(VertexAttrib4ubvARB, (index, v), (F, "glVertexAttrib4ubv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4ubvARB)(GLuint index, const GLubyte * v)
{
   DISPATCH(VertexAttrib4ubvARB, (index, v), (F, "glVertexAttrib4ubvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4uiv)(GLuint index, const GLuint * v)
{
   DISPATCH(VertexAttrib4uivARB, (index, v), (F, "glVertexAttrib4uiv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4uivARB)(GLuint index, const GLuint * v)
{
   DISPATCH(VertexAttrib4uivARB, (index, v), (F, "glVertexAttrib4uivARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4usv)(GLuint index, const GLushort * v)
{
   DISPATCH(VertexAttrib4usvARB, (index, v), (F, "glVertexAttrib4usv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4usvARB)(GLuint index, const GLushort * v)
{
   DISPATCH(VertexAttrib4usvARB, (index, v), (F, "glVertexAttrib4usvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer)
{
   DISPATCH(VertexAttribPointerARB, (index, size, type, normalized, stride, pointer), (F, "glVertexAttribPointer(%d, %d, 0x%x, %d, %d, %p);\n", index, size, type, normalized, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribPointerARB)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer)
{
   DISPATCH(VertexAttribPointerARB, (index, size, type, normalized, stride, pointer), (F, "glVertexAttribPointerARB(%d, %d, 0x%x, %d, %d, %p);\n", index, size, type, normalized, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(BindBuffer)(GLenum target, GLuint buffer)
{
   DISPATCH(BindBufferARB, (target, buffer), (F, "glBindBuffer(0x%x, %d);\n", target, buffer));
}

KEYWORD1 void KEYWORD2 NAME(BindBufferARB)(GLenum target, GLuint buffer)
{
   DISPATCH(BindBufferARB, (target, buffer), (F, "glBindBufferARB(0x%x, %d);\n", target, buffer));
}

KEYWORD1 void KEYWORD2 NAME(BufferData)(GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage)
{
   DISPATCH(BufferDataARB, (target, size, data, usage), (F, "glBufferData(0x%x, %d, %p, 0x%x);\n", target, size, (const void *) data, usage));
}

KEYWORD1 void KEYWORD2 NAME(BufferDataARB)(GLenum target, GLsizeiptrARB size, const GLvoid * data, GLenum usage)
{
   DISPATCH(BufferDataARB, (target, size, data, usage), (F, "glBufferDataARB(0x%x, %d, %p, 0x%x);\n", target, size, (const void *) data, usage));
}

KEYWORD1 void KEYWORD2 NAME(BufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid * data)
{
   DISPATCH(BufferSubDataARB, (target, offset, size, data), (F, "glBufferSubData(0x%x, %d, %d, %p);\n", target, offset, size, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(BufferSubDataARB)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid * data)
{
   DISPATCH(BufferSubDataARB, (target, offset, size, data), (F, "glBufferSubDataARB(0x%x, %d, %d, %p);\n", target, offset, size, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(DeleteBuffers)(GLsizei n, const GLuint * buffer)
{
   DISPATCH(DeleteBuffersARB, (n, buffer), (F, "glDeleteBuffers(%d, %p);\n", n, (const void *) buffer));
}

KEYWORD1 void KEYWORD2 NAME(DeleteBuffersARB)(GLsizei n, const GLuint * buffer)
{
   DISPATCH(DeleteBuffersARB, (n, buffer), (F, "glDeleteBuffersARB(%d, %p);\n", n, (const void *) buffer));
}

KEYWORD1 void KEYWORD2 NAME(GenBuffers)(GLsizei n, GLuint * buffer)
{
   DISPATCH(GenBuffersARB, (n, buffer), (F, "glGenBuffers(%d, %p);\n", n, (const void *) buffer));
}

KEYWORD1 void KEYWORD2 NAME(GenBuffersARB)(GLsizei n, GLuint * buffer)
{
   DISPATCH(GenBuffersARB, (n, buffer), (F, "glGenBuffersARB(%d, %p);\n", n, (const void *) buffer));
}

KEYWORD1 void KEYWORD2 NAME(GetBufferParameteriv)(GLenum target, GLenum pname, GLint * params)
{
   DISPATCH(GetBufferParameterivARB, (target, pname, params), (F, "glGetBufferParameteriv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetBufferParameterivARB)(GLenum target, GLenum pname, GLint * params)
{
   DISPATCH(GetBufferParameterivARB, (target, pname, params), (F, "glGetBufferParameterivARB(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetBufferPointerv)(GLenum target, GLenum pname, GLvoid ** params)
{
   DISPATCH(GetBufferPointervARB, (target, pname, params), (F, "glGetBufferPointerv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetBufferPointervARB)(GLenum target, GLenum pname, GLvoid ** params)
{
   DISPATCH(GetBufferPointervARB, (target, pname, params), (F, "glGetBufferPointervARB(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, GLvoid * data)
{
   DISPATCH(GetBufferSubDataARB, (target, offset, size, data), (F, "glGetBufferSubData(0x%x, %d, %d, %p);\n", target, offset, size, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(GetBufferSubDataARB)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, GLvoid * data)
{
   DISPATCH(GetBufferSubDataARB, (target, offset, size, data), (F, "glGetBufferSubDataARB(0x%x, %d, %d, %p);\n", target, offset, size, (const void *) data));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsBuffer)(GLuint buffer)
{
   RETURN_DISPATCH(IsBufferARB, (buffer), (F, "glIsBuffer(%d);\n", buffer));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsBufferARB)(GLuint buffer)
{
   RETURN_DISPATCH(IsBufferARB, (buffer), (F, "glIsBufferARB(%d);\n", buffer));
}

KEYWORD1 GLvoid * KEYWORD2 NAME(MapBuffer)(GLenum target, GLenum access)
{
   RETURN_DISPATCH(MapBufferARB, (target, access), (F, "glMapBuffer(0x%x, 0x%x);\n", target, access));
}

KEYWORD1 GLvoid * KEYWORD2 NAME(MapBufferARB)(GLenum target, GLenum access)
{
   RETURN_DISPATCH(MapBufferARB, (target, access), (F, "glMapBufferARB(0x%x, 0x%x);\n", target, access));
}

KEYWORD1 GLboolean KEYWORD2 NAME(UnmapBuffer)(GLenum target)
{
   RETURN_DISPATCH(UnmapBufferARB, (target), (F, "glUnmapBuffer(0x%x);\n", target));
}

KEYWORD1 GLboolean KEYWORD2 NAME(UnmapBufferARB)(GLenum target)
{
   RETURN_DISPATCH(UnmapBufferARB, (target), (F, "glUnmapBufferARB(0x%x);\n", target));
}

KEYWORD1 void KEYWORD2 NAME(BeginQuery)(GLenum target, GLuint id)
{
   DISPATCH(BeginQueryARB, (target, id), (F, "glBeginQuery(0x%x, %d);\n", target, id));
}

KEYWORD1 void KEYWORD2 NAME(BeginQueryARB)(GLenum target, GLuint id)
{
   DISPATCH(BeginQueryARB, (target, id), (F, "glBeginQueryARB(0x%x, %d);\n", target, id));
}

KEYWORD1 void KEYWORD2 NAME(DeleteQueries)(GLsizei n, const GLuint * ids)
{
   DISPATCH(DeleteQueriesARB, (n, ids), (F, "glDeleteQueries(%d, %p);\n", n, (const void *) ids));
}

KEYWORD1 void KEYWORD2 NAME(DeleteQueriesARB)(GLsizei n, const GLuint * ids)
{
   DISPATCH(DeleteQueriesARB, (n, ids), (F, "glDeleteQueriesARB(%d, %p);\n", n, (const void *) ids));
}

KEYWORD1 void KEYWORD2 NAME(EndQuery)(GLenum target)
{
   DISPATCH(EndQueryARB, (target), (F, "glEndQuery(0x%x);\n", target));
}

KEYWORD1 void KEYWORD2 NAME(EndQueryARB)(GLenum target)
{
   DISPATCH(EndQueryARB, (target), (F, "glEndQueryARB(0x%x);\n", target));
}

KEYWORD1 void KEYWORD2 NAME(GenQueries)(GLsizei n, GLuint * ids)
{
   DISPATCH(GenQueriesARB, (n, ids), (F, "glGenQueries(%d, %p);\n", n, (const void *) ids));
}

KEYWORD1 void KEYWORD2 NAME(GenQueriesARB)(GLsizei n, GLuint * ids)
{
   DISPATCH(GenQueriesARB, (n, ids), (F, "glGenQueriesARB(%d, %p);\n", n, (const void *) ids));
}

KEYWORD1 void KEYWORD2 NAME(GetQueryObjectiv)(GLuint id, GLenum pname, GLint * params)
{
   DISPATCH(GetQueryObjectivARB, (id, pname, params), (F, "glGetQueryObjectiv(%d, 0x%x, %p);\n", id, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetQueryObjectivARB)(GLuint id, GLenum pname, GLint * params)
{
   DISPATCH(GetQueryObjectivARB, (id, pname, params), (F, "glGetQueryObjectivARB(%d, 0x%x, %p);\n", id, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetQueryObjectuiv)(GLuint id, GLenum pname, GLuint * params)
{
   DISPATCH(GetQueryObjectuivARB, (id, pname, params), (F, "glGetQueryObjectuiv(%d, 0x%x, %p);\n", id, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetQueryObjectuivARB)(GLuint id, GLenum pname, GLuint * params)
{
   DISPATCH(GetQueryObjectuivARB, (id, pname, params), (F, "glGetQueryObjectuivARB(%d, 0x%x, %p);\n", id, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetQueryiv)(GLenum target, GLenum pname, GLint * params)
{
   DISPATCH(GetQueryivARB, (target, pname, params), (F, "glGetQueryiv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetQueryivARB)(GLenum target, GLenum pname, GLint * params)
{
   DISPATCH(GetQueryivARB, (target, pname, params), (F, "glGetQueryivARB(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsQuery)(GLuint id)
{
   RETURN_DISPATCH(IsQueryARB, (id), (F, "glIsQuery(%d);\n", id));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsQueryARB)(GLuint id)
{
   RETURN_DISPATCH(IsQueryARB, (id), (F, "glIsQueryARB(%d);\n", id));
}

KEYWORD1 void KEYWORD2 NAME(AttachObjectARB)(GLhandleARB containerObj, GLhandleARB obj)
{
   DISPATCH(AttachObjectARB, (containerObj, obj), (F, "glAttachObjectARB(%d, %d);\n", containerObj, obj));
}

KEYWORD1 void KEYWORD2 NAME(CompileShader)(GLuint shader)
{
   DISPATCH(CompileShaderARB, (shader), (F, "glCompileShader(%d);\n", shader));
}

KEYWORD1 void KEYWORD2 NAME(CompileShaderARB)(GLhandleARB shader)
{
   DISPATCH(CompileShaderARB, (shader), (F, "glCompileShaderARB(%d);\n", shader));
}

KEYWORD1 GLhandleARB KEYWORD2 NAME(CreateProgramObjectARB)(void)
{
   RETURN_DISPATCH(CreateProgramObjectARB, (), (F, "glCreateProgramObjectARB();\n"));
}

KEYWORD1 GLhandleARB KEYWORD2 NAME(CreateShaderObjectARB)(GLenum shaderType)
{
   RETURN_DISPATCH(CreateShaderObjectARB, (shaderType), (F, "glCreateShaderObjectARB(0x%x);\n", shaderType));
}

KEYWORD1 void KEYWORD2 NAME(DeleteObjectARB)(GLhandleARB obj)
{
   DISPATCH(DeleteObjectARB, (obj), (F, "glDeleteObjectARB(%d);\n", obj));
}

KEYWORD1 void KEYWORD2 NAME(DetachObjectARB)(GLhandleARB containerObj, GLhandleARB attachedObj)
{
   DISPATCH(DetachObjectARB, (containerObj, attachedObj), (F, "glDetachObjectARB(%d, %d);\n", containerObj, attachedObj));
}

KEYWORD1 void KEYWORD2 NAME(GetActiveUniform)(GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLint * size, GLenum * type, GLchar * name)
{
   DISPATCH(GetActiveUniformARB, (program, index, bufSize, length, size, type, name), (F, "glGetActiveUniform(%d, %d, %d, %p, %p, %p, %p);\n", program, index, bufSize, (const void *) length, (const void *) size, (const void *) type, (const void *) name));
}

KEYWORD1 void KEYWORD2 NAME(GetActiveUniformARB)(GLhandleARB program, GLuint index, GLsizei bufSize, GLsizei * length, GLint * size, GLenum * type, GLcharARB * name)
{
   DISPATCH(GetActiveUniformARB, (program, index, bufSize, length, size, type, name), (F, "glGetActiveUniformARB(%d, %d, %d, %p, %p, %p, %p);\n", program, index, bufSize, (const void *) length, (const void *) size, (const void *) type, (const void *) name));
}

KEYWORD1 void KEYWORD2 NAME(GetAttachedObjectsARB)(GLhandleARB containerObj, GLsizei maxLength, GLsizei * length, GLhandleARB * infoLog)
{
   DISPATCH(GetAttachedObjectsARB, (containerObj, maxLength, length, infoLog), (F, "glGetAttachedObjectsARB(%d, %d, %p, %p);\n", containerObj, maxLength, (const void *) length, (const void *) infoLog));
}

KEYWORD1 GLhandleARB KEYWORD2 NAME(GetHandleARB)(GLenum pname)
{
   RETURN_DISPATCH(GetHandleARB, (pname), (F, "glGetHandleARB(0x%x);\n", pname));
}

KEYWORD1 void KEYWORD2 NAME(GetInfoLogARB)(GLhandleARB obj, GLsizei maxLength, GLsizei * length, GLcharARB * infoLog)
{
   DISPATCH(GetInfoLogARB, (obj, maxLength, length, infoLog), (F, "glGetInfoLogARB(%d, %d, %p, %p);\n", obj, maxLength, (const void *) length, (const void *) infoLog));
}

KEYWORD1 void KEYWORD2 NAME(GetObjectParameterfvARB)(GLhandleARB obj, GLenum pname, GLfloat * params)
{
   DISPATCH(GetObjectParameterfvARB, (obj, pname, params), (F, "glGetObjectParameterfvARB(%d, 0x%x, %p);\n", obj, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetObjectParameterivARB)(GLhandleARB obj, GLenum pname, GLint * params)
{
   DISPATCH(GetObjectParameterivARB, (obj, pname, params), (F, "glGetObjectParameterivARB(%d, 0x%x, %p);\n", obj, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetShaderSource)(GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * source)
{
   DISPATCH(GetShaderSourceARB, (shader, bufSize, length, source), (F, "glGetShaderSource(%d, %d, %p, %p);\n", shader, bufSize, (const void *) length, (const void *) source));
}

KEYWORD1 void KEYWORD2 NAME(GetShaderSourceARB)(GLhandleARB shader, GLsizei bufSize, GLsizei * length, GLcharARB * source)
{
   DISPATCH(GetShaderSourceARB, (shader, bufSize, length, source), (F, "glGetShaderSourceARB(%d, %d, %p, %p);\n", shader, bufSize, (const void *) length, (const void *) source));
}

KEYWORD1 GLint KEYWORD2 NAME(GetUniformLocation)(GLuint program, const GLchar * name)
{
   RETURN_DISPATCH(GetUniformLocationARB, (program, name), (F, "glGetUniformLocation(%d, %p);\n", program, (const void *) name));
}

KEYWORD1 GLint KEYWORD2 NAME(GetUniformLocationARB)(GLhandleARB program, const GLcharARB * name)
{
   RETURN_DISPATCH(GetUniformLocationARB, (program, name), (F, "glGetUniformLocationARB(%d, %p);\n", program, (const void *) name));
}

KEYWORD1 void KEYWORD2 NAME(GetUniformfv)(GLuint program, GLint location, GLfloat * params)
{
   DISPATCH(GetUniformfvARB, (program, location, params), (F, "glGetUniformfv(%d, %d, %p);\n", program, location, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetUniformfvARB)(GLhandleARB program, GLint location, GLfloat * params)
{
   DISPATCH(GetUniformfvARB, (program, location, params), (F, "glGetUniformfvARB(%d, %d, %p);\n", program, location, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetUniformiv)(GLuint program, GLint location, GLint * params)
{
   DISPATCH(GetUniformivARB, (program, location, params), (F, "glGetUniformiv(%d, %d, %p);\n", program, location, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetUniformivARB)(GLhandleARB program, GLint location, GLint * params)
{
   DISPATCH(GetUniformivARB, (program, location, params), (F, "glGetUniformivARB(%d, %d, %p);\n", program, location, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(LinkProgram)(GLuint program)
{
   DISPATCH(LinkProgramARB, (program), (F, "glLinkProgram(%d);\n", program));
}

KEYWORD1 void KEYWORD2 NAME(LinkProgramARB)(GLhandleARB program)
{
   DISPATCH(LinkProgramARB, (program), (F, "glLinkProgramARB(%d);\n", program));
}

KEYWORD1 void KEYWORD2 NAME(ShaderSource)(GLuint shader, GLsizei count, const GLchar ** string, const GLint * length)
{
   DISPATCH(ShaderSourceARB, (shader, count, string, length), (F, "glShaderSource(%d, %d, %p, %p);\n", shader, count, (const void *) string, (const void *) length));
}

KEYWORD1 void KEYWORD2 NAME(ShaderSourceARB)(GLhandleARB shader, GLsizei count, const GLcharARB ** string, const GLint * length)
{
   DISPATCH(ShaderSourceARB, (shader, count, string, length), (F, "glShaderSourceARB(%d, %d, %p, %p);\n", shader, count, (const void *) string, (const void *) length));
}

KEYWORD1 void KEYWORD2 NAME(Uniform1f)(GLint location, GLfloat v0)
{
   DISPATCH(Uniform1fARB, (location, v0), (F, "glUniform1f(%d, %f);\n", location, v0));
}

KEYWORD1 void KEYWORD2 NAME(Uniform1fARB)(GLint location, GLfloat v0)
{
   DISPATCH(Uniform1fARB, (location, v0), (F, "glUniform1fARB(%d, %f);\n", location, v0));
}

KEYWORD1 void KEYWORD2 NAME(Uniform1fv)(GLint location, GLsizei count, const GLfloat * value)
{
   DISPATCH(Uniform1fvARB, (location, count, value), (F, "glUniform1fv(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform1fvARB)(GLint location, GLsizei count, const GLfloat * value)
{
   DISPATCH(Uniform1fvARB, (location, count, value), (F, "glUniform1fvARB(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform1i)(GLint location, GLint v0)
{
   DISPATCH(Uniform1iARB, (location, v0), (F, "glUniform1i(%d, %d);\n", location, v0));
}

KEYWORD1 void KEYWORD2 NAME(Uniform1iARB)(GLint location, GLint v0)
{
   DISPATCH(Uniform1iARB, (location, v0), (F, "glUniform1iARB(%d, %d);\n", location, v0));
}

KEYWORD1 void KEYWORD2 NAME(Uniform1iv)(GLint location, GLsizei count, const GLint * value)
{
   DISPATCH(Uniform1ivARB, (location, count, value), (F, "glUniform1iv(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform1ivARB)(GLint location, GLsizei count, const GLint * value)
{
   DISPATCH(Uniform1ivARB, (location, count, value), (F, "glUniform1ivARB(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform2f)(GLint location, GLfloat v0, GLfloat v1)
{
   DISPATCH(Uniform2fARB, (location, v0, v1), (F, "glUniform2f(%d, %f, %f);\n", location, v0, v1));
}

KEYWORD1 void KEYWORD2 NAME(Uniform2fARB)(GLint location, GLfloat v0, GLfloat v1)
{
   DISPATCH(Uniform2fARB, (location, v0, v1), (F, "glUniform2fARB(%d, %f, %f);\n", location, v0, v1));
}

KEYWORD1 void KEYWORD2 NAME(Uniform2fv)(GLint location, GLsizei count, const GLfloat * value)
{
   DISPATCH(Uniform2fvARB, (location, count, value), (F, "glUniform2fv(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform2fvARB)(GLint location, GLsizei count, const GLfloat * value)
{
   DISPATCH(Uniform2fvARB, (location, count, value), (F, "glUniform2fvARB(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform2i)(GLint location, GLint v0, GLint v1)
{
   DISPATCH(Uniform2iARB, (location, v0, v1), (F, "glUniform2i(%d, %d, %d);\n", location, v0, v1));
}

KEYWORD1 void KEYWORD2 NAME(Uniform2iARB)(GLint location, GLint v0, GLint v1)
{
   DISPATCH(Uniform2iARB, (location, v0, v1), (F, "glUniform2iARB(%d, %d, %d);\n", location, v0, v1));
}

KEYWORD1 void KEYWORD2 NAME(Uniform2iv)(GLint location, GLsizei count, const GLint * value)
{
   DISPATCH(Uniform2ivARB, (location, count, value), (F, "glUniform2iv(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform2ivARB)(GLint location, GLsizei count, const GLint * value)
{
   DISPATCH(Uniform2ivARB, (location, count, value), (F, "glUniform2ivARB(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
   DISPATCH(Uniform3fARB, (location, v0, v1, v2), (F, "glUniform3f(%d, %f, %f, %f);\n", location, v0, v1, v2));
}

KEYWORD1 void KEYWORD2 NAME(Uniform3fARB)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
   DISPATCH(Uniform3fARB, (location, v0, v1, v2), (F, "glUniform3fARB(%d, %f, %f, %f);\n", location, v0, v1, v2));
}

KEYWORD1 void KEYWORD2 NAME(Uniform3fv)(GLint location, GLsizei count, const GLfloat * value)
{
   DISPATCH(Uniform3fvARB, (location, count, value), (F, "glUniform3fv(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform3fvARB)(GLint location, GLsizei count, const GLfloat * value)
{
   DISPATCH(Uniform3fvARB, (location, count, value), (F, "glUniform3fvARB(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform3i)(GLint location, GLint v0, GLint v1, GLint v2)
{
   DISPATCH(Uniform3iARB, (location, v0, v1, v2), (F, "glUniform3i(%d, %d, %d, %d);\n", location, v0, v1, v2));
}

KEYWORD1 void KEYWORD2 NAME(Uniform3iARB)(GLint location, GLint v0, GLint v1, GLint v2)
{
   DISPATCH(Uniform3iARB, (location, v0, v1, v2), (F, "glUniform3iARB(%d, %d, %d, %d);\n", location, v0, v1, v2));
}

KEYWORD1 void KEYWORD2 NAME(Uniform3iv)(GLint location, GLsizei count, const GLint * value)
{
   DISPATCH(Uniform3ivARB, (location, count, value), (F, "glUniform3iv(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform3ivARB)(GLint location, GLsizei count, const GLint * value)
{
   DISPATCH(Uniform3ivARB, (location, count, value), (F, "glUniform3ivARB(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
   DISPATCH(Uniform4fARB, (location, v0, v1, v2, v3), (F, "glUniform4f(%d, %f, %f, %f, %f);\n", location, v0, v1, v2, v3));
}

KEYWORD1 void KEYWORD2 NAME(Uniform4fARB)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
   DISPATCH(Uniform4fARB, (location, v0, v1, v2, v3), (F, "glUniform4fARB(%d, %f, %f, %f, %f);\n", location, v0, v1, v2, v3));
}

KEYWORD1 void KEYWORD2 NAME(Uniform4fv)(GLint location, GLsizei count, const GLfloat * value)
{
   DISPATCH(Uniform4fvARB, (location, count, value), (F, "glUniform4fv(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform4fvARB)(GLint location, GLsizei count, const GLfloat * value)
{
   DISPATCH(Uniform4fvARB, (location, count, value), (F, "glUniform4fvARB(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform4i)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
{
   DISPATCH(Uniform4iARB, (location, v0, v1, v2, v3), (F, "glUniform4i(%d, %d, %d, %d, %d);\n", location, v0, v1, v2, v3));
}

KEYWORD1 void KEYWORD2 NAME(Uniform4iARB)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
{
   DISPATCH(Uniform4iARB, (location, v0, v1, v2, v3), (F, "glUniform4iARB(%d, %d, %d, %d, %d);\n", location, v0, v1, v2, v3));
}

KEYWORD1 void KEYWORD2 NAME(Uniform4iv)(GLint location, GLsizei count, const GLint * value)
{
   DISPATCH(Uniform4ivARB, (location, count, value), (F, "glUniform4iv(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform4ivARB)(GLint location, GLsizei count, const GLint * value)
{
   DISPATCH(Uniform4ivARB, (location, count, value), (F, "glUniform4ivARB(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(UniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
   DISPATCH(UniformMatrix2fvARB, (location, count, transpose, value), (F, "glUniformMatrix2fv(%d, %d, %d, %p);\n", location, count, transpose, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(UniformMatrix2fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
   DISPATCH(UniformMatrix2fvARB, (location, count, transpose, value), (F, "glUniformMatrix2fvARB(%d, %d, %d, %p);\n", location, count, transpose, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(UniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
   DISPATCH(UniformMatrix3fvARB, (location, count, transpose, value), (F, "glUniformMatrix3fv(%d, %d, %d, %p);\n", location, count, transpose, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(UniformMatrix3fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
   DISPATCH(UniformMatrix3fvARB, (location, count, transpose, value), (F, "glUniformMatrix3fvARB(%d, %d, %d, %p);\n", location, count, transpose, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(UniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
   DISPATCH(UniformMatrix4fvARB, (location, count, transpose, value), (F, "glUniformMatrix4fv(%d, %d, %d, %p);\n", location, count, transpose, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(UniformMatrix4fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
   DISPATCH(UniformMatrix4fvARB, (location, count, transpose, value), (F, "glUniformMatrix4fvARB(%d, %d, %d, %p);\n", location, count, transpose, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(UseProgram)(GLuint program)
{
   DISPATCH(UseProgramObjectARB, (program), (F, "glUseProgram(%d);\n", program));
}

KEYWORD1 void KEYWORD2 NAME(UseProgramObjectARB)(GLhandleARB program)
{
   DISPATCH(UseProgramObjectARB, (program), (F, "glUseProgramObjectARB(%d);\n", program));
}

KEYWORD1 void KEYWORD2 NAME(ValidateProgram)(GLuint program)
{
   DISPATCH(ValidateProgramARB, (program), (F, "glValidateProgram(%d);\n", program));
}

KEYWORD1 void KEYWORD2 NAME(ValidateProgramARB)(GLhandleARB program)
{
   DISPATCH(ValidateProgramARB, (program), (F, "glValidateProgramARB(%d);\n", program));
}

KEYWORD1 void KEYWORD2 NAME(BindAttribLocation)(GLuint program, GLuint index, const GLchar * name)
{
   DISPATCH(BindAttribLocationARB, (program, index, name), (F, "glBindAttribLocation(%d, %d, %p);\n", program, index, (const void *) name));
}

KEYWORD1 void KEYWORD2 NAME(BindAttribLocationARB)(GLhandleARB program, GLuint index, const GLcharARB * name)
{
   DISPATCH(BindAttribLocationARB, (program, index, name), (F, "glBindAttribLocationARB(%d, %d, %p);\n", program, index, (const void *) name));
}

KEYWORD1 void KEYWORD2 NAME(GetActiveAttrib)(GLuint program, GLuint index, GLsizei  bufSize, GLsizei * length, GLint * size, GLenum * type, GLchar * name)
{
   DISPATCH(GetActiveAttribARB, (program, index, bufSize, length, size, type, name), (F, "glGetActiveAttrib(%d, %d, %d, %p, %p, %p, %p);\n", program, index, bufSize, (const void *) length, (const void *) size, (const void *) type, (const void *) name));
}

KEYWORD1 void KEYWORD2 NAME(GetActiveAttribARB)(GLhandleARB program, GLuint index, GLsizei bufSize, GLsizei * length, GLint * size, GLenum * type, GLcharARB * name)
{
   DISPATCH(GetActiveAttribARB, (program, index, bufSize, length, size, type, name), (F, "glGetActiveAttribARB(%d, %d, %d, %p, %p, %p, %p);\n", program, index, bufSize, (const void *) length, (const void *) size, (const void *) type, (const void *) name));
}

KEYWORD1 GLint KEYWORD2 NAME(GetAttribLocation)(GLuint program, const GLchar * name)
{
   RETURN_DISPATCH(GetAttribLocationARB, (program, name), (F, "glGetAttribLocation(%d, %p);\n", program, (const void *) name));
}

KEYWORD1 GLint KEYWORD2 NAME(GetAttribLocationARB)(GLhandleARB program, const GLcharARB * name)
{
   RETURN_DISPATCH(GetAttribLocationARB, (program, name), (F, "glGetAttribLocationARB(%d, %p);\n", program, (const void *) name));
}

KEYWORD1 void KEYWORD2 NAME(DrawBuffers)(GLsizei n, const GLenum * bufs)
{
   DISPATCH(DrawBuffersARB, (n, bufs), (F, "glDrawBuffers(%d, %p);\n", n, (const void *) bufs));
}

KEYWORD1 void KEYWORD2 NAME(DrawBuffersARB)(GLsizei n, const GLenum * bufs)
{
   DISPATCH(DrawBuffersARB, (n, bufs), (F, "glDrawBuffersARB(%d, %p);\n", n, (const void *) bufs));
}

KEYWORD1 void KEYWORD2 NAME(DrawBuffersATI)(GLsizei n, const GLenum * bufs)
{
   DISPATCH(DrawBuffersARB, (n, bufs), (F, "glDrawBuffersATI(%d, %p);\n", n, (const void *) bufs));
}

KEYWORD1 void KEYWORD2 NAME(PolygonOffsetEXT)(GLfloat factor, GLfloat bias)
{
   DISPATCH(PolygonOffsetEXT, (factor, bias), (F, "glPolygonOffsetEXT(%f, %f);\n", factor, bias));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_562)(GLenum pname, GLfloat * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_562)(GLenum pname, GLfloat * params)
{
   DISPATCH(GetPixelTexGenParameterfvSGIS, (pname, params), (F, "glGetPixelTexGenParameterfvSGIS(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_563)(GLenum pname, GLint * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_563)(GLenum pname, GLint * params)
{
   DISPATCH(GetPixelTexGenParameterivSGIS, (pname, params), (F, "glGetPixelTexGenParameterivSGIS(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_564)(GLenum pname, GLfloat param);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_564)(GLenum pname, GLfloat param)
{
   DISPATCH(PixelTexGenParameterfSGIS, (pname, param), (F, "glPixelTexGenParameterfSGIS(0x%x, %f);\n", pname, param));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_565)(GLenum pname, const GLfloat * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_565)(GLenum pname, const GLfloat * params)
{
   DISPATCH(PixelTexGenParameterfvSGIS, (pname, params), (F, "glPixelTexGenParameterfvSGIS(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_566)(GLenum pname, GLint param);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_566)(GLenum pname, GLint param)
{
   DISPATCH(PixelTexGenParameteriSGIS, (pname, param), (F, "glPixelTexGenParameteriSGIS(0x%x, %d);\n", pname, param));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_567)(GLenum pname, const GLint * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_567)(GLenum pname, const GLint * params)
{
   DISPATCH(PixelTexGenParameterivSGIS, (pname, params), (F, "glPixelTexGenParameterivSGIS(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_568)(GLclampf value, GLboolean invert);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_568)(GLclampf value, GLboolean invert)
{
   DISPATCH(SampleMaskSGIS, (value, invert), (F, "glSampleMaskSGIS(%f, %d);\n", value, invert));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_569)(GLenum pattern);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_569)(GLenum pattern)
{
   DISPATCH(SamplePatternSGIS, (pattern), (F, "glSamplePatternSGIS(0x%x);\n", pattern));
}

KEYWORD1 void KEYWORD2 NAME(ColorPointerEXT)(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer)
{
   DISPATCH(ColorPointerEXT, (size, type, stride, count, pointer), (F, "glColorPointerEXT(%d, 0x%x, %d, %d, %p);\n", size, type, stride, count, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(EdgeFlagPointerEXT)(GLsizei stride, GLsizei count, const GLboolean * pointer)
{
   DISPATCH(EdgeFlagPointerEXT, (stride, count, pointer), (F, "glEdgeFlagPointerEXT(%d, %d, %p);\n", stride, count, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(IndexPointerEXT)(GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer)
{
   DISPATCH(IndexPointerEXT, (type, stride, count, pointer), (F, "glIndexPointerEXT(0x%x, %d, %d, %p);\n", type, stride, count, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(NormalPointerEXT)(GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer)
{
   DISPATCH(NormalPointerEXT, (type, stride, count, pointer), (F, "glNormalPointerEXT(0x%x, %d, %d, %p);\n", type, stride, count, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(TexCoordPointerEXT)(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer)
{
   DISPATCH(TexCoordPointerEXT, (size, type, stride, count, pointer), (F, "glTexCoordPointerEXT(%d, 0x%x, %d, %d, %p);\n", size, type, stride, count, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(VertexPointerEXT)(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer)
{
   DISPATCH(VertexPointerEXT, (size, type, stride, count, pointer), (F, "glVertexPointerEXT(%d, 0x%x, %d, %d, %p);\n", size, type, stride, count, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(PointParameterf)(GLenum pname, GLfloat param)
{
   DISPATCH(PointParameterfEXT, (pname, param), (F, "glPointParameterf(0x%x, %f);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(PointParameterfARB)(GLenum pname, GLfloat param)
{
   DISPATCH(PointParameterfEXT, (pname, param), (F, "glPointParameterfARB(0x%x, %f);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(PointParameterfEXT)(GLenum pname, GLfloat param)
{
   DISPATCH(PointParameterfEXT, (pname, param), (F, "glPointParameterfEXT(0x%x, %f);\n", pname, param));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_576)(GLenum pname, GLfloat param);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_576)(GLenum pname, GLfloat param)
{
   DISPATCH(PointParameterfEXT, (pname, param), (F, "glPointParameterfSGIS(0x%x, %f);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(PointParameterfv)(GLenum pname, const GLfloat * params)
{
   DISPATCH(PointParameterfvEXT, (pname, params), (F, "glPointParameterfv(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(PointParameterfvARB)(GLenum pname, const GLfloat * params)
{
   DISPATCH(PointParameterfvEXT, (pname, params), (F, "glPointParameterfvARB(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(PointParameterfvEXT)(GLenum pname, const GLfloat * params)
{
   DISPATCH(PointParameterfvEXT, (pname, params), (F, "glPointParameterfvEXT(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_577)(GLenum pname, const GLfloat * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_577)(GLenum pname, const GLfloat * params)
{
   DISPATCH(PointParameterfvEXT, (pname, params), (F, "glPointParameterfvSGIS(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(LockArraysEXT)(GLint first, GLsizei count)
{
   DISPATCH(LockArraysEXT, (first, count), (F, "glLockArraysEXT(%d, %d);\n", first, count));
}

KEYWORD1 void KEYWORD2 NAME(UnlockArraysEXT)(void)
{
   DISPATCH(UnlockArraysEXT, (), (F, "glUnlockArraysEXT();\n"));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_580)(GLenum pname, GLdouble * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_580)(GLenum pname, GLdouble * params)
{
   DISPATCH(CullParameterdvEXT, (pname, params), (F, "glCullParameterdvEXT(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_581)(GLenum pname, GLfloat * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_581)(GLenum pname, GLfloat * params)
{
   DISPATCH(CullParameterfvEXT, (pname, params), (F, "glCullParameterfvEXT(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3b)(GLbyte red, GLbyte green, GLbyte blue)
{
   DISPATCH(SecondaryColor3bEXT, (red, green, blue), (F, "glSecondaryColor3b(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3bEXT)(GLbyte red, GLbyte green, GLbyte blue)
{
   DISPATCH(SecondaryColor3bEXT, (red, green, blue), (F, "glSecondaryColor3bEXT(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3bv)(const GLbyte * v)
{
   DISPATCH(SecondaryColor3bvEXT, (v), (F, "glSecondaryColor3bv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3bvEXT)(const GLbyte * v)
{
   DISPATCH(SecondaryColor3bvEXT, (v), (F, "glSecondaryColor3bvEXT(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3d)(GLdouble red, GLdouble green, GLdouble blue)
{
   DISPATCH(SecondaryColor3dEXT, (red, green, blue), (F, "glSecondaryColor3d(%f, %f, %f);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3dEXT)(GLdouble red, GLdouble green, GLdouble blue)
{
   DISPATCH(SecondaryColor3dEXT, (red, green, blue), (F, "glSecondaryColor3dEXT(%f, %f, %f);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3dv)(const GLdouble * v)
{
   DISPATCH(SecondaryColor3dvEXT, (v), (F, "glSecondaryColor3dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3dvEXT)(const GLdouble * v)
{
   DISPATCH(SecondaryColor3dvEXT, (v), (F, "glSecondaryColor3dvEXT(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3f)(GLfloat red, GLfloat green, GLfloat blue)
{
   DISPATCH(SecondaryColor3fEXT, (red, green, blue), (F, "glSecondaryColor3f(%f, %f, %f);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3fEXT)(GLfloat red, GLfloat green, GLfloat blue)
{
   DISPATCH(SecondaryColor3fEXT, (red, green, blue), (F, "glSecondaryColor3fEXT(%f, %f, %f);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3fv)(const GLfloat * v)
{
   DISPATCH(SecondaryColor3fvEXT, (v), (F, "glSecondaryColor3fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3fvEXT)(const GLfloat * v)
{
   DISPATCH(SecondaryColor3fvEXT, (v), (F, "glSecondaryColor3fvEXT(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3i)(GLint red, GLint green, GLint blue)
{
   DISPATCH(SecondaryColor3iEXT, (red, green, blue), (F, "glSecondaryColor3i(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3iEXT)(GLint red, GLint green, GLint blue)
{
   DISPATCH(SecondaryColor3iEXT, (red, green, blue), (F, "glSecondaryColor3iEXT(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3iv)(const GLint * v)
{
   DISPATCH(SecondaryColor3ivEXT, (v), (F, "glSecondaryColor3iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3ivEXT)(const GLint * v)
{
   DISPATCH(SecondaryColor3ivEXT, (v), (F, "glSecondaryColor3ivEXT(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3s)(GLshort red, GLshort green, GLshort blue)
{
   DISPATCH(SecondaryColor3sEXT, (red, green, blue), (F, "glSecondaryColor3s(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3sEXT)(GLshort red, GLshort green, GLshort blue)
{
   DISPATCH(SecondaryColor3sEXT, (red, green, blue), (F, "glSecondaryColor3sEXT(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3sv)(const GLshort * v)
{
   DISPATCH(SecondaryColor3svEXT, (v), (F, "glSecondaryColor3sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3svEXT)(const GLshort * v)
{
   DISPATCH(SecondaryColor3svEXT, (v), (F, "glSecondaryColor3svEXT(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3ub)(GLubyte red, GLubyte green, GLubyte blue)
{
   DISPATCH(SecondaryColor3ubEXT, (red, green, blue), (F, "glSecondaryColor3ub(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3ubEXT)(GLubyte red, GLubyte green, GLubyte blue)
{
   DISPATCH(SecondaryColor3ubEXT, (red, green, blue), (F, "glSecondaryColor3ubEXT(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3ubv)(const GLubyte * v)
{
   DISPATCH(SecondaryColor3ubvEXT, (v), (F, "glSecondaryColor3ubv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3ubvEXT)(const GLubyte * v)
{
   DISPATCH(SecondaryColor3ubvEXT, (v), (F, "glSecondaryColor3ubvEXT(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3ui)(GLuint red, GLuint green, GLuint blue)
{
   DISPATCH(SecondaryColor3uiEXT, (red, green, blue), (F, "glSecondaryColor3ui(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3uiEXT)(GLuint red, GLuint green, GLuint blue)
{
   DISPATCH(SecondaryColor3uiEXT, (red, green, blue), (F, "glSecondaryColor3uiEXT(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3uiv)(const GLuint * v)
{
   DISPATCH(SecondaryColor3uivEXT, (v), (F, "glSecondaryColor3uiv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3uivEXT)(const GLuint * v)
{
   DISPATCH(SecondaryColor3uivEXT, (v), (F, "glSecondaryColor3uivEXT(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3us)(GLushort red, GLushort green, GLushort blue)
{
   DISPATCH(SecondaryColor3usEXT, (red, green, blue), (F, "glSecondaryColor3us(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3usEXT)(GLushort red, GLushort green, GLushort blue)
{
   DISPATCH(SecondaryColor3usEXT, (red, green, blue), (F, "glSecondaryColor3usEXT(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3usv)(const GLushort * v)
{
   DISPATCH(SecondaryColor3usvEXT, (v), (F, "glSecondaryColor3usv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3usvEXT)(const GLushort * v)
{
   DISPATCH(SecondaryColor3usvEXT, (v), (F, "glSecondaryColor3usvEXT(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
   DISPATCH(SecondaryColorPointerEXT, (size, type, stride, pointer), (F, "glSecondaryColorPointer(%d, 0x%x, %d, %p);\n", size, type, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColorPointerEXT)(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
   DISPATCH(SecondaryColorPointerEXT, (size, type, stride, pointer), (F, "glSecondaryColorPointerEXT(%d, 0x%x, %d, %p);\n", size, type, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(MultiDrawArrays)(GLenum mode, GLint * first, GLsizei * count, GLsizei primcount)
{
   DISPATCH(MultiDrawArraysEXT, (mode, first, count, primcount), (F, "glMultiDrawArrays(0x%x, %p, %p, %d);\n", mode, (const void *) first, (const void *) count, primcount));
}

KEYWORD1 void KEYWORD2 NAME(MultiDrawArraysEXT)(GLenum mode, GLint * first, GLsizei * count, GLsizei primcount)
{
   DISPATCH(MultiDrawArraysEXT, (mode, first, count, primcount), (F, "glMultiDrawArraysEXT(0x%x, %p, %p, %d);\n", mode, (const void *) first, (const void *) count, primcount));
}

KEYWORD1 void KEYWORD2 NAME(MultiDrawElements)(GLenum mode, const GLsizei * count, GLenum type, const GLvoid ** indices, GLsizei primcount)
{
   DISPATCH(MultiDrawElementsEXT, (mode, count, type, indices, primcount), (F, "glMultiDrawElements(0x%x, %p, 0x%x, %p, %d);\n", mode, (const void *) count, type, (const void *) indices, primcount));
}

KEYWORD1 void KEYWORD2 NAME(MultiDrawElementsEXT)(GLenum mode, const GLsizei * count, GLenum type, const GLvoid ** indices, GLsizei primcount)
{
   DISPATCH(MultiDrawElementsEXT, (mode, count, type, indices, primcount), (F, "glMultiDrawElementsEXT(0x%x, %p, 0x%x, %p, %d);\n", mode, (const void *) count, type, (const void *) indices, primcount));
}

KEYWORD1 void KEYWORD2 NAME(FogCoordPointer)(GLenum type, GLsizei stride, const GLvoid * pointer)
{
   DISPATCH(FogCoordPointerEXT, (type, stride, pointer), (F, "glFogCoordPointer(0x%x, %d, %p);\n", type, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(FogCoordPointerEXT)(GLenum type, GLsizei stride, const GLvoid * pointer)
{
   DISPATCH(FogCoordPointerEXT, (type, stride, pointer), (F, "glFogCoordPointerEXT(0x%x, %d, %p);\n", type, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(FogCoordd)(GLdouble coord)
{
   DISPATCH(FogCoorddEXT, (coord), (F, "glFogCoordd(%f);\n", coord));
}

KEYWORD1 void KEYWORD2 NAME(FogCoorddEXT)(GLdouble coord)
{
   DISPATCH(FogCoorddEXT, (coord), (F, "glFogCoorddEXT(%f);\n", coord));
}

KEYWORD1 void KEYWORD2 NAME(FogCoorddv)(const GLdouble * coord)
{
   DISPATCH(FogCoorddvEXT, (coord), (F, "glFogCoorddv(%p);\n", (const void *) coord));
}

KEYWORD1 void KEYWORD2 NAME(FogCoorddvEXT)(const GLdouble * coord)
{
   DISPATCH(FogCoorddvEXT, (coord), (F, "glFogCoorddvEXT(%p);\n", (const void *) coord));
}

KEYWORD1 void KEYWORD2 NAME(FogCoordf)(GLfloat coord)
{
   DISPATCH(FogCoordfEXT, (coord), (F, "glFogCoordf(%f);\n", coord));
}

KEYWORD1 void KEYWORD2 NAME(FogCoordfEXT)(GLfloat coord)
{
   DISPATCH(FogCoordfEXT, (coord), (F, "glFogCoordfEXT(%f);\n", coord));
}

KEYWORD1 void KEYWORD2 NAME(FogCoordfv)(const GLfloat * coord)
{
   DISPATCH(FogCoordfvEXT, (coord), (F, "glFogCoordfv(%p);\n", (const void *) coord));
}

KEYWORD1 void KEYWORD2 NAME(FogCoordfvEXT)(const GLfloat * coord)
{
   DISPATCH(FogCoordfvEXT, (coord), (F, "glFogCoordfvEXT(%p);\n", (const void *) coord));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_606)(GLenum mode);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_606)(GLenum mode)
{
   DISPATCH(PixelTexGenSGIX, (mode), (F, "glPixelTexGenSGIX(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(BlendFuncSeparate)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
{
   DISPATCH(BlendFuncSeparateEXT, (sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha), (F, "glBlendFuncSeparate(0x%x, 0x%x, 0x%x, 0x%x);\n", sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha));
}

KEYWORD1 void KEYWORD2 NAME(BlendFuncSeparateEXT)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
{
   DISPATCH(BlendFuncSeparateEXT, (sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha), (F, "glBlendFuncSeparateEXT(0x%x, 0x%x, 0x%x, 0x%x);\n", sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_607)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_607)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
{
   DISPATCH(BlendFuncSeparateEXT, (sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha), (F, "glBlendFuncSeparateINGR(0x%x, 0x%x, 0x%x, 0x%x);\n", sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha));
}

KEYWORD1 void KEYWORD2 NAME(FlushVertexArrayRangeNV)(void)
{
   DISPATCH(FlushVertexArrayRangeNV, (), (F, "glFlushVertexArrayRangeNV();\n"));
}

KEYWORD1 void KEYWORD2 NAME(VertexArrayRangeNV)(GLsizei length, const GLvoid * pointer)
{
   DISPATCH(VertexArrayRangeNV, (length, pointer), (F, "glVertexArrayRangeNV(%d, %p);\n", length, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(CombinerInputNV)(GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage)
{
   DISPATCH(CombinerInputNV, (stage, portion, variable, input, mapping, componentUsage), (F, "glCombinerInputNV(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x);\n", stage, portion, variable, input, mapping, componentUsage));
}

KEYWORD1 void KEYWORD2 NAME(CombinerOutputNV)(GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum)
{
   DISPATCH(CombinerOutputNV, (stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum), (F, "glCombinerOutputNV(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, %d, %d, %d);\n", stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum));
}

KEYWORD1 void KEYWORD2 NAME(CombinerParameterfNV)(GLenum pname, GLfloat param)
{
   DISPATCH(CombinerParameterfNV, (pname, param), (F, "glCombinerParameterfNV(0x%x, %f);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(CombinerParameterfvNV)(GLenum pname, const GLfloat * params)
{
   DISPATCH(CombinerParameterfvNV, (pname, params), (F, "glCombinerParameterfvNV(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(CombinerParameteriNV)(GLenum pname, GLint param)
{
   DISPATCH(CombinerParameteriNV, (pname, param), (F, "glCombinerParameteriNV(0x%x, %d);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(CombinerParameterivNV)(GLenum pname, const GLint * params)
{
   DISPATCH(CombinerParameterivNV, (pname, params), (F, "glCombinerParameterivNV(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(FinalCombinerInputNV)(GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage)
{
   DISPATCH(FinalCombinerInputNV, (variable, input, mapping, componentUsage), (F, "glFinalCombinerInputNV(0x%x, 0x%x, 0x%x, 0x%x);\n", variable, input, mapping, componentUsage));
}

KEYWORD1 void KEYWORD2 NAME(GetCombinerInputParameterfvNV)(GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat * params)
{
   DISPATCH(GetCombinerInputParameterfvNV, (stage, portion, variable, pname, params), (F, "glGetCombinerInputParameterfvNV(0x%x, 0x%x, 0x%x, 0x%x, %p);\n", stage, portion, variable, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetCombinerInputParameterivNV)(GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint * params)
{
   DISPATCH(GetCombinerInputParameterivNV, (stage, portion, variable, pname, params), (F, "glGetCombinerInputParameterivNV(0x%x, 0x%x, 0x%x, 0x%x, %p);\n", stage, portion, variable, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetCombinerOutputParameterfvNV)(GLenum stage, GLenum portion, GLenum pname, GLfloat * params)
{
   DISPATCH(GetCombinerOutputParameterfvNV, (stage, portion, pname, params), (F, "glGetCombinerOutputParameterfvNV(0x%x, 0x%x, 0x%x, %p);\n", stage, portion, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetCombinerOutputParameterivNV)(GLenum stage, GLenum portion, GLenum pname, GLint * params)
{
   DISPATCH(GetCombinerOutputParameterivNV, (stage, portion, pname, params), (F, "glGetCombinerOutputParameterivNV(0x%x, 0x%x, 0x%x, %p);\n", stage, portion, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetFinalCombinerInputParameterfvNV)(GLenum variable, GLenum pname, GLfloat * params)
{
   DISPATCH(GetFinalCombinerInputParameterfvNV, (variable, pname, params), (F, "glGetFinalCombinerInputParameterfvNV(0x%x, 0x%x, %p);\n", variable, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetFinalCombinerInputParameterivNV)(GLenum variable, GLenum pname, GLint * params)
{
   DISPATCH(GetFinalCombinerInputParameterivNV, (variable, pname, params), (F, "glGetFinalCombinerInputParameterivNV(0x%x, 0x%x, %p);\n", variable, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ResizeBuffersMESA)(void)
{
   DISPATCH(ResizeBuffersMESA, (), (F, "glResizeBuffersMESA();\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2d)(GLdouble x, GLdouble y)
{
   DISPATCH(WindowPos2dMESA, (x, y), (F, "glWindowPos2d(%f, %f);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2dARB)(GLdouble x, GLdouble y)
{
   DISPATCH(WindowPos2dMESA, (x, y), (F, "glWindowPos2dARB(%f, %f);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2dMESA)(GLdouble x, GLdouble y)
{
   DISPATCH(WindowPos2dMESA, (x, y), (F, "glWindowPos2dMESA(%f, %f);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2dv)(const GLdouble * v)
{
   DISPATCH(WindowPos2dvMESA, (v), (F, "glWindowPos2dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2dvARB)(const GLdouble * v)
{
   DISPATCH(WindowPos2dvMESA, (v), (F, "glWindowPos2dvARB(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2dvMESA)(const GLdouble * v)
{
   DISPATCH(WindowPos2dvMESA, (v), (F, "glWindowPos2dvMESA(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2f)(GLfloat x, GLfloat y)
{
   DISPATCH(WindowPos2fMESA, (x, y), (F, "glWindowPos2f(%f, %f);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2fARB)(GLfloat x, GLfloat y)
{
   DISPATCH(WindowPos2fMESA, (x, y), (F, "glWindowPos2fARB(%f, %f);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2fMESA)(GLfloat x, GLfloat y)
{
   DISPATCH(WindowPos2fMESA, (x, y), (F, "glWindowPos2fMESA(%f, %f);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2fv)(const GLfloat * v)
{
   DISPATCH(WindowPos2fvMESA, (v), (F, "glWindowPos2fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2fvARB)(const GLfloat * v)
{
   DISPATCH(WindowPos2fvMESA, (v), (F, "glWindowPos2fvARB(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2fvMESA)(const GLfloat * v)
{
   DISPATCH(WindowPos2fvMESA, (v), (F, "glWindowPos2fvMESA(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2i)(GLint x, GLint y)
{
   DISPATCH(WindowPos2iMESA, (x, y), (F, "glWindowPos2i(%d, %d);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2iARB)(GLint x, GLint y)
{
   DISPATCH(WindowPos2iMESA, (x, y), (F, "glWindowPos2iARB(%d, %d);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2iMESA)(GLint x, GLint y)
{
   DISPATCH(WindowPos2iMESA, (x, y), (F, "glWindowPos2iMESA(%d, %d);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2iv)(const GLint * v)
{
   DISPATCH(WindowPos2ivMESA, (v), (F, "glWindowPos2iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2ivARB)(const GLint * v)
{
   DISPATCH(WindowPos2ivMESA, (v), (F, "glWindowPos2ivARB(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2ivMESA)(const GLint * v)
{
   DISPATCH(WindowPos2ivMESA, (v), (F, "glWindowPos2ivMESA(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2s)(GLshort x, GLshort y)
{
   DISPATCH(WindowPos2sMESA, (x, y), (F, "glWindowPos2s(%d, %d);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2sARB)(GLshort x, GLshort y)
{
   DISPATCH(WindowPos2sMESA, (x, y), (F, "glWindowPos2sARB(%d, %d);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2sMESA)(GLshort x, GLshort y)
{
   DISPATCH(WindowPos2sMESA, (x, y), (F, "glWindowPos2sMESA(%d, %d);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2sv)(const GLshort * v)
{
   DISPATCH(WindowPos2svMESA, (v), (F, "glWindowPos2sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2svARB)(const GLshort * v)
{
   DISPATCH(WindowPos2svMESA, (v), (F, "glWindowPos2svARB(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2svMESA)(const GLshort * v)
{
   DISPATCH(WindowPos2svMESA, (v), (F, "glWindowPos2svMESA(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3d)(GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(WindowPos3dMESA, (x, y, z), (F, "glWindowPos3d(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3dARB)(GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(WindowPos3dMESA, (x, y, z), (F, "glWindowPos3dARB(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3dMESA)(GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(WindowPos3dMESA, (x, y, z), (F, "glWindowPos3dMESA(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3dv)(const GLdouble * v)
{
   DISPATCH(WindowPos3dvMESA, (v), (F, "glWindowPos3dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3dvARB)(const GLdouble * v)
{
   DISPATCH(WindowPos3dvMESA, (v), (F, "glWindowPos3dvARB(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3dvMESA)(const GLdouble * v)
{
   DISPATCH(WindowPos3dvMESA, (v), (F, "glWindowPos3dvMESA(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3f)(GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(WindowPos3fMESA, (x, y, z), (F, "glWindowPos3f(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3fARB)(GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(WindowPos3fMESA, (x, y, z), (F, "glWindowPos3fARB(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3fMESA)(GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(WindowPos3fMESA, (x, y, z), (F, "glWindowPos3fMESA(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3fv)(const GLfloat * v)
{
   DISPATCH(WindowPos3fvMESA, (v), (F, "glWindowPos3fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3fvARB)(const GLfloat * v)
{
   DISPATCH(WindowPos3fvMESA, (v), (F, "glWindowPos3fvARB(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3fvMESA)(const GLfloat * v)
{
   DISPATCH(WindowPos3fvMESA, (v), (F, "glWindowPos3fvMESA(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3i)(GLint x, GLint y, GLint z)
{
   DISPATCH(WindowPos3iMESA, (x, y, z), (F, "glWindowPos3i(%d, %d, %d);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3iARB)(GLint x, GLint y, GLint z)
{
   DISPATCH(WindowPos3iMESA, (x, y, z), (F, "glWindowPos3iARB(%d, %d, %d);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3iMESA)(GLint x, GLint y, GLint z)
{
   DISPATCH(WindowPos3iMESA, (x, y, z), (F, "glWindowPos3iMESA(%d, %d, %d);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3iv)(const GLint * v)
{
   DISPATCH(WindowPos3ivMESA, (v), (F, "glWindowPos3iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3ivARB)(const GLint * v)
{
   DISPATCH(WindowPos3ivMESA, (v), (F, "glWindowPos3ivARB(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3ivMESA)(const GLint * v)
{
   DISPATCH(WindowPos3ivMESA, (v), (F, "glWindowPos3ivMESA(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3s)(GLshort x, GLshort y, GLshort z)
{
   DISPATCH(WindowPos3sMESA, (x, y, z), (F, "glWindowPos3s(%d, %d, %d);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3sARB)(GLshort x, GLshort y, GLshort z)
{
   DISPATCH(WindowPos3sMESA, (x, y, z), (F, "glWindowPos3sARB(%d, %d, %d);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3sMESA)(GLshort x, GLshort y, GLshort z)
{
   DISPATCH(WindowPos3sMESA, (x, y, z), (F, "glWindowPos3sMESA(%d, %d, %d);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3sv)(const GLshort * v)
{
   DISPATCH(WindowPos3svMESA, (v), (F, "glWindowPos3sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3svARB)(const GLshort * v)
{
   DISPATCH(WindowPos3svMESA, (v), (F, "glWindowPos3svARB(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3svMESA)(const GLshort * v)
{
   DISPATCH(WindowPos3svMESA, (v), (F, "glWindowPos3svMESA(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4dMESA)(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   DISPATCH(WindowPos4dMESA, (x, y, z, w), (F, "glWindowPos4dMESA(%f, %f, %f, %f);\n", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4dvMESA)(const GLdouble * v)
{
   DISPATCH(WindowPos4dvMESA, (v), (F, "glWindowPos4dvMESA(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4fMESA)(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   DISPATCH(WindowPos4fMESA, (x, y, z, w), (F, "glWindowPos4fMESA(%f, %f, %f, %f);\n", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4fvMESA)(const GLfloat * v)
{
   DISPATCH(WindowPos4fvMESA, (v), (F, "glWindowPos4fvMESA(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4iMESA)(GLint x, GLint y, GLint z, GLint w)
{
   DISPATCH(WindowPos4iMESA, (x, y, z, w), (F, "glWindowPos4iMESA(%d, %d, %d, %d);\n", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4ivMESA)(const GLint * v)
{
   DISPATCH(WindowPos4ivMESA, (v), (F, "glWindowPos4ivMESA(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4sMESA)(GLshort x, GLshort y, GLshort z, GLshort w)
{
   DISPATCH(WindowPos4sMESA, (x, y, z, w), (F, "glWindowPos4sMESA(%d, %d, %d, %d);\n", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4svMESA)(const GLshort * v)
{
   DISPATCH(WindowPos4svMESA, (v), (F, "glWindowPos4svMESA(%p);\n", (const void *) v));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_648)(const GLenum * mode, const GLint * first, const GLsizei * count, GLsizei primcount, GLint modestride);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_648)(const GLenum * mode, const GLint * first, const GLsizei * count, GLsizei primcount, GLint modestride)
{
   DISPATCH(MultiModeDrawArraysIBM, (mode, first, count, primcount, modestride), (F, "glMultiModeDrawArraysIBM(%p, %p, %p, %d, %d);\n", (const void *) mode, (const void *) first, (const void *) count, primcount, modestride));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_649)(const GLenum * mode, const GLsizei * count, GLenum type, const GLvoid * const * indices, GLsizei primcount, GLint modestride);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_649)(const GLenum * mode, const GLsizei * count, GLenum type, const GLvoid * const * indices, GLsizei primcount, GLint modestride)
{
   DISPATCH(MultiModeDrawElementsIBM, (mode, count, type, indices, primcount, modestride), (F, "glMultiModeDrawElementsIBM(%p, %p, 0x%x, %p, %d, %d);\n", (const void *) mode, (const void *) count, type, (const void *) indices, primcount, modestride));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_650)(GLsizei n, const GLuint * fences);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_650)(GLsizei n, const GLuint * fences)
{
   DISPATCH(DeleteFencesNV, (n, fences), (F, "glDeleteFencesNV(%d, %p);\n", n, (const void *) fences));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_651)(GLuint fence);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_651)(GLuint fence)
{
   DISPATCH(FinishFenceNV, (fence), (F, "glFinishFenceNV(%d);\n", fence));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_652)(GLsizei n, GLuint * fences);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_652)(GLsizei n, GLuint * fences)
{
   DISPATCH(GenFencesNV, (n, fences), (F, "glGenFencesNV(%d, %p);\n", n, (const void *) fences));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_653)(GLuint fence, GLenum pname, GLint * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_653)(GLuint fence, GLenum pname, GLint * params)
{
   DISPATCH(GetFenceivNV, (fence, pname, params), (F, "glGetFenceivNV(%d, 0x%x, %p);\n", fence, pname, (const void *) params));
}

KEYWORD1_ALT GLboolean KEYWORD2 NAME(_dispatch_stub_654)(GLuint fence);

KEYWORD1_ALT GLboolean KEYWORD2 NAME(_dispatch_stub_654)(GLuint fence)
{
   RETURN_DISPATCH(IsFenceNV, (fence), (F, "glIsFenceNV(%d);\n", fence));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_655)(GLuint fence, GLenum condition);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_655)(GLuint fence, GLenum condition)
{
   DISPATCH(SetFenceNV, (fence, condition), (F, "glSetFenceNV(%d, 0x%x);\n", fence, condition));
}

KEYWORD1_ALT GLboolean KEYWORD2 NAME(_dispatch_stub_656)(GLuint fence);

KEYWORD1_ALT GLboolean KEYWORD2 NAME(_dispatch_stub_656)(GLuint fence)
{
   RETURN_DISPATCH(TestFenceNV, (fence), (F, "glTestFenceNV(%d);\n", fence));
}

KEYWORD1 GLboolean KEYWORD2 NAME(AreProgramsResidentNV)(GLsizei n, const GLuint * ids, GLboolean * residences)
{
   RETURN_DISPATCH(AreProgramsResidentNV, (n, ids, residences), (F, "glAreProgramsResidentNV(%d, %p, %p);\n", n, (const void *) ids, (const void *) residences));
}

KEYWORD1 void KEYWORD2 NAME(BindProgramARB)(GLenum target, GLuint program)
{
   DISPATCH(BindProgramNV, (target, program), (F, "glBindProgramARB(0x%x, %d);\n", target, program));
}

KEYWORD1 void KEYWORD2 NAME(BindProgramNV)(GLenum target, GLuint program)
{
   DISPATCH(BindProgramNV, (target, program), (F, "glBindProgramNV(0x%x, %d);\n", target, program));
}

KEYWORD1 void KEYWORD2 NAME(DeleteProgramsARB)(GLsizei n, const GLuint * programs)
{
   DISPATCH(DeleteProgramsNV, (n, programs), (F, "glDeleteProgramsARB(%d, %p);\n", n, (const void *) programs));
}

KEYWORD1 void KEYWORD2 NAME(DeleteProgramsNV)(GLsizei n, const GLuint * programs)
{
   DISPATCH(DeleteProgramsNV, (n, programs), (F, "glDeleteProgramsNV(%d, %p);\n", n, (const void *) programs));
}

KEYWORD1 void KEYWORD2 NAME(ExecuteProgramNV)(GLenum target, GLuint id, const GLfloat * params)
{
   DISPATCH(ExecuteProgramNV, (target, id, params), (F, "glExecuteProgramNV(0x%x, %d, %p);\n", target, id, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GenProgramsARB)(GLsizei n, GLuint * programs)
{
   DISPATCH(GenProgramsNV, (n, programs), (F, "glGenProgramsARB(%d, %p);\n", n, (const void *) programs));
}

KEYWORD1 void KEYWORD2 NAME(GenProgramsNV)(GLsizei n, GLuint * programs)
{
   DISPATCH(GenProgramsNV, (n, programs), (F, "glGenProgramsNV(%d, %p);\n", n, (const void *) programs));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramParameterdvNV)(GLenum target, GLuint index, GLenum pname, GLdouble * params)
{
   DISPATCH(GetProgramParameterdvNV, (target, index, pname, params), (F, "glGetProgramParameterdvNV(0x%x, %d, 0x%x, %p);\n", target, index, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramParameterfvNV)(GLenum target, GLuint index, GLenum pname, GLfloat * params)
{
   DISPATCH(GetProgramParameterfvNV, (target, index, pname, params), (F, "glGetProgramParameterfvNV(0x%x, %d, 0x%x, %p);\n", target, index, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramStringNV)(GLuint id, GLenum pname, GLubyte * program)
{
   DISPATCH(GetProgramStringNV, (id, pname, program), (F, "glGetProgramStringNV(%d, 0x%x, %p);\n", id, pname, (const void *) program));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramivNV)(GLuint id, GLenum pname, GLint * params)
{
   DISPATCH(GetProgramivNV, (id, pname, params), (F, "glGetProgramivNV(%d, 0x%x, %p);\n", id, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetTrackMatrixivNV)(GLenum target, GLuint address, GLenum pname, GLint * params)
{
   DISPATCH(GetTrackMatrixivNV, (target, address, pname, params), (F, "glGetTrackMatrixivNV(0x%x, %d, 0x%x, %p);\n", target, address, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribPointerv)(GLuint index, GLenum pname, GLvoid ** pointer)
{
   DISPATCH(GetVertexAttribPointervNV, (index, pname, pointer), (F, "glGetVertexAttribPointerv(%d, 0x%x, %p);\n", index, pname, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribPointervARB)(GLuint index, GLenum pname, GLvoid ** pointer)
{
   DISPATCH(GetVertexAttribPointervNV, (index, pname, pointer), (F, "glGetVertexAttribPointervARB(%d, 0x%x, %p);\n", index, pname, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribPointervNV)(GLuint index, GLenum pname, GLvoid ** pointer)
{
   DISPATCH(GetVertexAttribPointervNV, (index, pname, pointer), (F, "glGetVertexAttribPointervNV(%d, 0x%x, %p);\n", index, pname, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribdvNV)(GLuint index, GLenum pname, GLdouble * params)
{
   DISPATCH(GetVertexAttribdvNV, (index, pname, params), (F, "glGetVertexAttribdvNV(%d, 0x%x, %p);\n", index, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribfvNV)(GLuint index, GLenum pname, GLfloat * params)
{
   DISPATCH(GetVertexAttribfvNV, (index, pname, params), (F, "glGetVertexAttribfvNV(%d, 0x%x, %p);\n", index, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribivNV)(GLuint index, GLenum pname, GLint * params)
{
   DISPATCH(GetVertexAttribivNV, (index, pname, params), (F, "glGetVertexAttribivNV(%d, 0x%x, %p);\n", index, pname, (const void *) params));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsProgramARB)(GLuint program)
{
   RETURN_DISPATCH(IsProgramNV, (program), (F, "glIsProgramARB(%d);\n", program));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsProgramNV)(GLuint program)
{
   RETURN_DISPATCH(IsProgramNV, (program), (F, "glIsProgramNV(%d);\n", program));
}

KEYWORD1 void KEYWORD2 NAME(LoadProgramNV)(GLenum target, GLuint id, GLsizei len, const GLubyte * program)
{
   DISPATCH(LoadProgramNV, (target, id, len, program), (F, "glLoadProgramNV(0x%x, %d, %d, %p);\n", target, id, len, (const void *) program));
}

KEYWORD1 void KEYWORD2 NAME(ProgramParameters4dvNV)(GLenum target, GLuint index, GLuint num, const GLdouble * params)
{
   DISPATCH(ProgramParameters4dvNV, (target, index, num, params), (F, "glProgramParameters4dvNV(0x%x, %d, %d, %p);\n", target, index, num, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ProgramParameters4fvNV)(GLenum target, GLuint index, GLuint num, const GLfloat * params)
{
   DISPATCH(ProgramParameters4fvNV, (target, index, num, params), (F, "glProgramParameters4fvNV(0x%x, %d, %d, %p);\n", target, index, num, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(RequestResidentProgramsNV)(GLsizei n, const GLuint * ids)
{
   DISPATCH(RequestResidentProgramsNV, (n, ids), (F, "glRequestResidentProgramsNV(%d, %p);\n", n, (const void *) ids));
}

KEYWORD1 void KEYWORD2 NAME(TrackMatrixNV)(GLenum target, GLuint address, GLenum matrix, GLenum transform)
{
   DISPATCH(TrackMatrixNV, (target, address, matrix, transform), (F, "glTrackMatrixNV(0x%x, %d, 0x%x, 0x%x);\n", target, address, matrix, transform));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1dNV)(GLuint index, GLdouble x)
{
   DISPATCH(VertexAttrib1dNV, (index, x), (F, "glVertexAttrib1dNV(%d, %f);\n", index, x));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1dvNV)(GLuint index, const GLdouble * v)
{
   DISPATCH(VertexAttrib1dvNV, (index, v), (F, "glVertexAttrib1dvNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1fNV)(GLuint index, GLfloat x)
{
   DISPATCH(VertexAttrib1fNV, (index, x), (F, "glVertexAttrib1fNV(%d, %f);\n", index, x));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1fvNV)(GLuint index, const GLfloat * v)
{
   DISPATCH(VertexAttrib1fvNV, (index, v), (F, "glVertexAttrib1fvNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1sNV)(GLuint index, GLshort x)
{
   DISPATCH(VertexAttrib1sNV, (index, x), (F, "glVertexAttrib1sNV(%d, %d);\n", index, x));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1svNV)(GLuint index, const GLshort * v)
{
   DISPATCH(VertexAttrib1svNV, (index, v), (F, "glVertexAttrib1svNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2dNV)(GLuint index, GLdouble x, GLdouble y)
{
   DISPATCH(VertexAttrib2dNV, (index, x, y), (F, "glVertexAttrib2dNV(%d, %f, %f);\n", index, x, y));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2dvNV)(GLuint index, const GLdouble * v)
{
   DISPATCH(VertexAttrib2dvNV, (index, v), (F, "glVertexAttrib2dvNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2fNV)(GLuint index, GLfloat x, GLfloat y)
{
   DISPATCH(VertexAttrib2fNV, (index, x, y), (F, "glVertexAttrib2fNV(%d, %f, %f);\n", index, x, y));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2fvNV)(GLuint index, const GLfloat * v)
{
   DISPATCH(VertexAttrib2fvNV, (index, v), (F, "glVertexAttrib2fvNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2sNV)(GLuint index, GLshort x, GLshort y)
{
   DISPATCH(VertexAttrib2sNV, (index, x, y), (F, "glVertexAttrib2sNV(%d, %d, %d);\n", index, x, y));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2svNV)(GLuint index, const GLshort * v)
{
   DISPATCH(VertexAttrib2svNV, (index, v), (F, "glVertexAttrib2svNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3dNV)(GLuint index, GLdouble x, GLdouble y, GLdouble z)
{
   DISPATCH(VertexAttrib3dNV, (index, x, y, z), (F, "glVertexAttrib3dNV(%d, %f, %f, %f);\n", index, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3dvNV)(GLuint index, const GLdouble * v)
{
   DISPATCH(VertexAttrib3dvNV, (index, v), (F, "glVertexAttrib3dvNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3fNV)(GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
   DISPATCH(VertexAttrib3fNV, (index, x, y, z), (F, "glVertexAttrib3fNV(%d, %f, %f, %f);\n", index, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3fvNV)(GLuint index, const GLfloat * v)
{
   DISPATCH(VertexAttrib3fvNV, (index, v), (F, "glVertexAttrib3fvNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3sNV)(GLuint index, GLshort x, GLshort y, GLshort z)
{
   DISPATCH(VertexAttrib3sNV, (index, x, y, z), (F, "glVertexAttrib3sNV(%d, %d, %d, %d);\n", index, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3svNV)(GLuint index, const GLshort * v)
{
   DISPATCH(VertexAttrib3svNV, (index, v), (F, "glVertexAttrib3svNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4dNV)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   DISPATCH(VertexAttrib4dNV, (index, x, y, z, w), (F, "glVertexAttrib4dNV(%d, %f, %f, %f, %f);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4dvNV)(GLuint index, const GLdouble * v)
{
   DISPATCH(VertexAttrib4dvNV, (index, v), (F, "glVertexAttrib4dvNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4fNV)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   DISPATCH(VertexAttrib4fNV, (index, x, y, z, w), (F, "glVertexAttrib4fNV(%d, %f, %f, %f, %f);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4fvNV)(GLuint index, const GLfloat * v)
{
   DISPATCH(VertexAttrib4fvNV, (index, v), (F, "glVertexAttrib4fvNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4sNV)(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w)
{
   DISPATCH(VertexAttrib4sNV, (index, x, y, z, w), (F, "glVertexAttrib4sNV(%d, %d, %d, %d, %d);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4svNV)(GLuint index, const GLshort * v)
{
   DISPATCH(VertexAttrib4svNV, (index, v), (F, "glVertexAttrib4svNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4ubNV)(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w)
{
   DISPATCH(VertexAttrib4ubNV, (index, x, y, z, w), (F, "glVertexAttrib4ubNV(%d, %d, %d, %d, %d);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4ubvNV)(GLuint index, const GLubyte * v)
{
   DISPATCH(VertexAttrib4ubvNV, (index, v), (F, "glVertexAttrib4ubvNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribPointerNV)(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
   DISPATCH(VertexAttribPointerNV, (index, size, type, stride, pointer), (F, "glVertexAttribPointerNV(%d, %d, 0x%x, %d, %p);\n", index, size, type, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs1dvNV)(GLuint index, GLsizei n, const GLdouble * v)
{
   DISPATCH(VertexAttribs1dvNV, (index, n, v), (F, "glVertexAttribs1dvNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs1fvNV)(GLuint index, GLsizei n, const GLfloat * v)
{
   DISPATCH(VertexAttribs1fvNV, (index, n, v), (F, "glVertexAttribs1fvNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs1svNV)(GLuint index, GLsizei n, const GLshort * v)
{
   DISPATCH(VertexAttribs1svNV, (index, n, v), (F, "glVertexAttribs1svNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs2dvNV)(GLuint index, GLsizei n, const GLdouble * v)
{
   DISPATCH(VertexAttribs2dvNV, (index, n, v), (F, "glVertexAttribs2dvNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs2fvNV)(GLuint index, GLsizei n, const GLfloat * v)
{
   DISPATCH(VertexAttribs2fvNV, (index, n, v), (F, "glVertexAttribs2fvNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs2svNV)(GLuint index, GLsizei n, const GLshort * v)
{
   DISPATCH(VertexAttribs2svNV, (index, n, v), (F, "glVertexAttribs2svNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs3dvNV)(GLuint index, GLsizei n, const GLdouble * v)
{
   DISPATCH(VertexAttribs3dvNV, (index, n, v), (F, "glVertexAttribs3dvNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs3fvNV)(GLuint index, GLsizei n, const GLfloat * v)
{
   DISPATCH(VertexAttribs3fvNV, (index, n, v), (F, "glVertexAttribs3fvNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs3svNV)(GLuint index, GLsizei n, const GLshort * v)
{
   DISPATCH(VertexAttribs3svNV, (index, n, v), (F, "glVertexAttribs3svNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs4dvNV)(GLuint index, GLsizei n, const GLdouble * v)
{
   DISPATCH(VertexAttribs4dvNV, (index, n, v), (F, "glVertexAttribs4dvNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs4fvNV)(GLuint index, GLsizei n, const GLfloat * v)
{
   DISPATCH(VertexAttribs4fvNV, (index, n, v), (F, "glVertexAttribs4fvNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs4svNV)(GLuint index, GLsizei n, const GLshort * v)
{
   DISPATCH(VertexAttribs4svNV, (index, n, v), (F, "glVertexAttribs4svNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs4ubvNV)(GLuint index, GLsizei n, const GLubyte * v)
{
   DISPATCH(VertexAttribs4ubvNV, (index, n, v), (F, "glVertexAttribs4ubvNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(AlphaFragmentOp1ATI)(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod)
{
   DISPATCH(AlphaFragmentOp1ATI, (op, dst, dstMod, arg1, arg1Rep, arg1Mod), (F, "glAlphaFragmentOp1ATI(0x%x, %d, %d, %d, %d, %d);\n", op, dst, dstMod, arg1, arg1Rep, arg1Mod));
}

KEYWORD1 void KEYWORD2 NAME(AlphaFragmentOp2ATI)(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod)
{
   DISPATCH(AlphaFragmentOp2ATI, (op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod), (F, "glAlphaFragmentOp2ATI(0x%x, %d, %d, %d, %d, %d, %d, %d, %d);\n", op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod));
}

KEYWORD1 void KEYWORD2 NAME(AlphaFragmentOp3ATI)(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod)
{
   DISPATCH(AlphaFragmentOp3ATI, (op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod), (F, "glAlphaFragmentOp3ATI(0x%x, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d);\n", op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod));
}

KEYWORD1 void KEYWORD2 NAME(BeginFragmentShaderATI)(void)
{
   DISPATCH(BeginFragmentShaderATI, (), (F, "glBeginFragmentShaderATI();\n"));
}

KEYWORD1 void KEYWORD2 NAME(BindFragmentShaderATI)(GLuint id)
{
   DISPATCH(BindFragmentShaderATI, (id), (F, "glBindFragmentShaderATI(%d);\n", id));
}

KEYWORD1 void KEYWORD2 NAME(ColorFragmentOp1ATI)(GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod)
{
   DISPATCH(ColorFragmentOp1ATI, (op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod), (F, "glColorFragmentOp1ATI(0x%x, %d, %d, %d, %d, %d, %d);\n", op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod));
}

KEYWORD1 void KEYWORD2 NAME(ColorFragmentOp2ATI)(GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod)
{
   DISPATCH(ColorFragmentOp2ATI, (op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod), (F, "glColorFragmentOp2ATI(0x%x, %d, %d, %d, %d, %d, %d, %d, %d, %d);\n", op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod));
}

KEYWORD1 void KEYWORD2 NAME(ColorFragmentOp3ATI)(GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod)
{
   DISPATCH(ColorFragmentOp3ATI, (op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod), (F, "glColorFragmentOp3ATI(0x%x, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d);\n", op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod));
}

KEYWORD1 void KEYWORD2 NAME(DeleteFragmentShaderATI)(GLuint id)
{
   DISPATCH(DeleteFragmentShaderATI, (id), (F, "glDeleteFragmentShaderATI(%d);\n", id));
}

KEYWORD1 void KEYWORD2 NAME(EndFragmentShaderATI)(void)
{
   DISPATCH(EndFragmentShaderATI, (), (F, "glEndFragmentShaderATI();\n"));
}

KEYWORD1 GLuint KEYWORD2 NAME(GenFragmentShadersATI)(GLuint range)
{
   RETURN_DISPATCH(GenFragmentShadersATI, (range), (F, "glGenFragmentShadersATI(%d);\n", range));
}

KEYWORD1 void KEYWORD2 NAME(PassTexCoordATI)(GLuint dst, GLuint coord, GLenum swizzle)
{
   DISPATCH(PassTexCoordATI, (dst, coord, swizzle), (F, "glPassTexCoordATI(%d, %d, 0x%x);\n", dst, coord, swizzle));
}

KEYWORD1 void KEYWORD2 NAME(SampleMapATI)(GLuint dst, GLuint interp, GLenum swizzle)
{
   DISPATCH(SampleMapATI, (dst, interp, swizzle), (F, "glSampleMapATI(%d, %d, 0x%x);\n", dst, interp, swizzle));
}

KEYWORD1 void KEYWORD2 NAME(SetFragmentShaderConstantATI)(GLuint dst, const GLfloat * value)
{
   DISPATCH(SetFragmentShaderConstantATI, (dst, value), (F, "glSetFragmentShaderConstantATI(%d, %p);\n", dst, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(PointParameteri)(GLenum pname, GLint param)
{
   DISPATCH(PointParameteriNV, (pname, param), (F, "glPointParameteri(0x%x, %d);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(PointParameteriNV)(GLenum pname, GLint param)
{
   DISPATCH(PointParameteriNV, (pname, param), (F, "glPointParameteriNV(0x%x, %d);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(PointParameteriv)(GLenum pname, const GLint * params)
{
   DISPATCH(PointParameterivNV, (pname, params), (F, "glPointParameteriv(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(PointParameterivNV)(GLenum pname, const GLint * params)
{
   DISPATCH(PointParameterivNV, (pname, params), (F, "glPointParameterivNV(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_733)(GLenum face);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_733)(GLenum face)
{
   DISPATCH(ActiveStencilFaceEXT, (face), (F, "glActiveStencilFaceEXT(0x%x);\n", face));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_734)(GLuint array);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_734)(GLuint array)
{
   DISPATCH(BindVertexArrayAPPLE, (array), (F, "glBindVertexArrayAPPLE(%d);\n", array));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_735)(GLsizei n, const GLuint * arrays);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_735)(GLsizei n, const GLuint * arrays)
{
   DISPATCH(DeleteVertexArraysAPPLE, (n, arrays), (F, "glDeleteVertexArraysAPPLE(%d, %p);\n", n, (const void *) arrays));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_736)(GLsizei n, GLuint * arrays);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_736)(GLsizei n, GLuint * arrays)
{
   DISPATCH(GenVertexArraysAPPLE, (n, arrays), (F, "glGenVertexArraysAPPLE(%d, %p);\n", n, (const void *) arrays));
}

KEYWORD1_ALT GLboolean KEYWORD2 NAME(_dispatch_stub_737)(GLuint array);

KEYWORD1_ALT GLboolean KEYWORD2 NAME(_dispatch_stub_737)(GLuint array)
{
   RETURN_DISPATCH(IsVertexArrayAPPLE, (array), (F, "glIsVertexArrayAPPLE(%d);\n", array));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramNamedParameterdvNV)(GLuint id, GLsizei len, const GLubyte * name, GLdouble * params)
{
   DISPATCH(GetProgramNamedParameterdvNV, (id, len, name, params), (F, "glGetProgramNamedParameterdvNV(%d, %d, %p, %p);\n", id, len, (const void *) name, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramNamedParameterfvNV)(GLuint id, GLsizei len, const GLubyte * name, GLfloat * params)
{
   DISPATCH(GetProgramNamedParameterfvNV, (id, len, name, params), (F, "glGetProgramNamedParameterfvNV(%d, %d, %p, %p);\n", id, len, (const void *) name, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ProgramNamedParameter4dNV)(GLuint id, GLsizei len, const GLubyte * name, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   DISPATCH(ProgramNamedParameter4dNV, (id, len, name, x, y, z, w), (F, "glProgramNamedParameter4dNV(%d, %d, %p, %f, %f, %f, %f);\n", id, len, (const void *) name, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(ProgramNamedParameter4dvNV)(GLuint id, GLsizei len, const GLubyte * name, const GLdouble * v)
{
   DISPATCH(ProgramNamedParameter4dvNV, (id, len, name, v), (F, "glProgramNamedParameter4dvNV(%d, %d, %p, %p);\n", id, len, (const void *) name, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(ProgramNamedParameter4fNV)(GLuint id, GLsizei len, const GLubyte * name, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   DISPATCH(ProgramNamedParameter4fNV, (id, len, name, x, y, z, w), (F, "glProgramNamedParameter4fNV(%d, %d, %p, %f, %f, %f, %f);\n", id, len, (const void *) name, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(ProgramNamedParameter4fvNV)(GLuint id, GLsizei len, const GLubyte * name, const GLfloat * v)
{
   DISPATCH(ProgramNamedParameter4fvNV, (id, len, name, v), (F, "glProgramNamedParameter4fvNV(%d, %d, %p, %p);\n", id, len, (const void *) name, (const void *) v));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_744)(GLclampd zmin, GLclampd zmax);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_744)(GLclampd zmin, GLclampd zmax)
{
   DISPATCH(DepthBoundsEXT, (zmin, zmax), (F, "glDepthBoundsEXT(%f, %f);\n", zmin, zmax));
}

KEYWORD1 void KEYWORD2 NAME(BlendEquationSeparate)(GLenum modeRGB, GLenum modeA)
{
   DISPATCH(BlendEquationSeparateEXT, (modeRGB, modeA), (F, "glBlendEquationSeparate(0x%x, 0x%x);\n", modeRGB, modeA));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_745)(GLenum modeRGB, GLenum modeA);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_745)(GLenum modeRGB, GLenum modeA)
{
   DISPATCH(BlendEquationSeparateEXT, (modeRGB, modeA), (F, "glBlendEquationSeparateEXT(0x%x, 0x%x);\n", modeRGB, modeA));
}

KEYWORD1 void KEYWORD2 NAME(BindFramebufferEXT)(GLenum target, GLuint framebuffer)
{
   DISPATCH(BindFramebufferEXT, (target, framebuffer), (F, "glBindFramebufferEXT(0x%x, %d);\n", target, framebuffer));
}

KEYWORD1 void KEYWORD2 NAME(BindRenderbufferEXT)(GLenum target, GLuint renderbuffer)
{
   DISPATCH(BindRenderbufferEXT, (target, renderbuffer), (F, "glBindRenderbufferEXT(0x%x, %d);\n", target, renderbuffer));
}

KEYWORD1 GLenum KEYWORD2 NAME(CheckFramebufferStatusEXT)(GLenum target)
{
   RETURN_DISPATCH(CheckFramebufferStatusEXT, (target), (F, "glCheckFramebufferStatusEXT(0x%x);\n", target));
}

KEYWORD1 void KEYWORD2 NAME(DeleteFramebuffersEXT)(GLsizei n, const GLuint * framebuffers)
{
   DISPATCH(DeleteFramebuffersEXT, (n, framebuffers), (F, "glDeleteFramebuffersEXT(%d, %p);\n", n, (const void *) framebuffers));
}

KEYWORD1 void KEYWORD2 NAME(DeleteRenderbuffersEXT)(GLsizei n, const GLuint * renderbuffers)
{
   DISPATCH(DeleteRenderbuffersEXT, (n, renderbuffers), (F, "glDeleteRenderbuffersEXT(%d, %p);\n", n, (const void *) renderbuffers));
}

KEYWORD1 void KEYWORD2 NAME(FramebufferRenderbufferEXT)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
   DISPATCH(FramebufferRenderbufferEXT, (target, attachment, renderbuffertarget, renderbuffer), (F, "glFramebufferRenderbufferEXT(0x%x, 0x%x, 0x%x, %d);\n", target, attachment, renderbuffertarget, renderbuffer));
}

KEYWORD1 void KEYWORD2 NAME(FramebufferTexture1DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
   DISPATCH(FramebufferTexture1DEXT, (target, attachment, textarget, texture, level), (F, "glFramebufferTexture1DEXT(0x%x, 0x%x, 0x%x, %d, %d);\n", target, attachment, textarget, texture, level));
}

KEYWORD1 void KEYWORD2 NAME(FramebufferTexture2DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
   DISPATCH(FramebufferTexture2DEXT, (target, attachment, textarget, texture, level), (F, "glFramebufferTexture2DEXT(0x%x, 0x%x, 0x%x, %d, %d);\n", target, attachment, textarget, texture, level));
}

KEYWORD1 void KEYWORD2 NAME(FramebufferTexture3DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset)
{
   DISPATCH(FramebufferTexture3DEXT, (target, attachment, textarget, texture, level, zoffset), (F, "glFramebufferTexture3DEXT(0x%x, 0x%x, 0x%x, %d, %d, %d);\n", target, attachment, textarget, texture, level, zoffset));
}

KEYWORD1 void KEYWORD2 NAME(GenFramebuffersEXT)(GLsizei n, GLuint * framebuffers)
{
   DISPATCH(GenFramebuffersEXT, (n, framebuffers), (F, "glGenFramebuffersEXT(%d, %p);\n", n, (const void *) framebuffers));
}

KEYWORD1 void KEYWORD2 NAME(GenRenderbuffersEXT)(GLsizei n, GLuint * renderbuffers)
{
   DISPATCH(GenRenderbuffersEXT, (n, renderbuffers), (F, "glGenRenderbuffersEXT(%d, %p);\n", n, (const void *) renderbuffers));
}

KEYWORD1 void KEYWORD2 NAME(GenerateMipmapEXT)(GLenum target)
{
   DISPATCH(GenerateMipmapEXT, (target), (F, "glGenerateMipmapEXT(0x%x);\n", target));
}

KEYWORD1 void KEYWORD2 NAME(GetFramebufferAttachmentParameterivEXT)(GLenum target, GLenum attachment, GLenum pname, GLint * params)
{
   DISPATCH(GetFramebufferAttachmentParameterivEXT, (target, attachment, pname, params), (F, "glGetFramebufferAttachmentParameterivEXT(0x%x, 0x%x, 0x%x, %p);\n", target, attachment, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetRenderbufferParameterivEXT)(GLenum target, GLenum pname, GLint * params)
{
   DISPATCH(GetRenderbufferParameterivEXT, (target, pname, params), (F, "glGetRenderbufferParameterivEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsFramebufferEXT)(GLuint framebuffer)
{
   RETURN_DISPATCH(IsFramebufferEXT, (framebuffer), (F, "glIsFramebufferEXT(%d);\n", framebuffer));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsRenderbufferEXT)(GLuint renderbuffer)
{
   RETURN_DISPATCH(IsRenderbufferEXT, (renderbuffer), (F, "glIsRenderbufferEXT(%d);\n", renderbuffer));
}

KEYWORD1 void KEYWORD2 NAME(RenderbufferStorageEXT)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
   DISPATCH(RenderbufferStorageEXT, (target, internalformat, width, height), (F, "glRenderbufferStorageEXT(0x%x, 0x%x, %d, %d);\n", target, internalformat, width, height));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_763)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_763)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
   DISPATCH(BlitFramebufferEXT, (srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter), (F, "glBlitFramebufferEXT(%d, %d, %d, %d, %d, %d, %d, %d, %d, 0x%x);\n", srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter));
}

KEYWORD1 void KEYWORD2 NAME(FramebufferTextureLayerEXT)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)
{
   DISPATCH(FramebufferTextureLayerEXT, (target, attachment, texture, level, layer), (F, "glFramebufferTextureLayerEXT(0x%x, 0x%x, %d, %d, %d);\n", target, attachment, texture, level, layer));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_765)(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_765)(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask)
{
   DISPATCH(StencilFuncSeparateATI, (frontfunc, backfunc, ref, mask), (F, "glStencilFuncSeparateATI(0x%x, 0x%x, %d, %d);\n", frontfunc, backfunc, ref, mask));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_766)(GLenum target, GLuint index, GLsizei count, const GLfloat * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_766)(GLenum target, GLuint index, GLsizei count, const GLfloat * params)
{
   DISPATCH(ProgramEnvParameters4fvEXT, (target, index, count, params), (F, "glProgramEnvParameters4fvEXT(0x%x, %d, %d, %p);\n", target, index, count, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_767)(GLenum target, GLuint index, GLsizei count, const GLfloat * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_767)(GLenum target, GLuint index, GLsizei count, const GLfloat * params)
{
   DISPATCH(ProgramLocalParameters4fvEXT, (target, index, count, params), (F, "glProgramLocalParameters4fvEXT(0x%x, %d, %d, %p);\n", target, index, count, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_768)(GLuint id, GLenum pname, GLint64EXT * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_768)(GLuint id, GLenum pname, GLint64EXT * params)
{
   DISPATCH(GetQueryObjecti64vEXT, (id, pname, params), (F, "glGetQueryObjecti64vEXT(%d, 0x%x, %p);\n", id, pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_769)(GLuint id, GLenum pname, GLuint64EXT * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_769)(GLuint id, GLenum pname, GLuint64EXT * params)
{
   DISPATCH(GetQueryObjectui64vEXT, (id, pname, params), (F, "glGetQueryObjectui64vEXT(%d, 0x%x, %p);\n", id, pname, (const void *) params));
}


#endif /* defined( NAME ) */

/*
 * This is how a dispatch table can be initialized with all the functions
 * we generated above.
 */
#ifdef DISPATCH_TABLE_NAME

#ifndef TABLE_ENTRY
#error TABLE_ENTRY must be defined
#endif

static _glapi_proc DISPATCH_TABLE_NAME[] = {
   TABLE_ENTRY(NewList),
   TABLE_ENTRY(EndList),
   TABLE_ENTRY(CallList),
   TABLE_ENTRY(CallLists),
   TABLE_ENTRY(DeleteLists),
   TABLE_ENTRY(GenLists),
   TABLE_ENTRY(ListBase),
   TABLE_ENTRY(Begin),
   TABLE_ENTRY(Bitmap),
   TABLE_ENTRY(Color3b),
   TABLE_ENTRY(Color3bv),
   TABLE_ENTRY(Color3d),
   TABLE_ENTRY(Color3dv),
   TABLE_ENTRY(Color3f),
   TABLE_ENTRY(Color3fv),
   TABLE_ENTRY(Color3i),
   TABLE_ENTRY(Color3iv),
   TABLE_ENTRY(Color3s),
   TABLE_ENTRY(Color3sv),
   TABLE_ENTRY(Color3ub),
   TABLE_ENTRY(Color3ubv),
   TABLE_ENTRY(Color3ui),
   TABLE_ENTRY(Color3uiv),
   TABLE_ENTRY(Color3us),
   TABLE_ENTRY(Color3usv),
   TABLE_ENTRY(Color4b),
   TABLE_ENTRY(Color4bv),
   TABLE_ENTRY(Color4d),
   TABLE_ENTRY(Color4dv),
   TABLE_ENTRY(Color4f),
   TABLE_ENTRY(Color4fv),
   TABLE_ENTRY(Color4i),
   TABLE_ENTRY(Color4iv),
   TABLE_ENTRY(Color4s),
   TABLE_ENTRY(Color4sv),
   TABLE_ENTRY(Color4ub),
   TABLE_ENTRY(Color4ubv),
   TABLE_ENTRY(Color4ui),
   TABLE_ENTRY(Color4uiv),
   TABLE_ENTRY(Color4us),
   TABLE_ENTRY(Color4usv),
   TABLE_ENTRY(EdgeFlag),
   TABLE_ENTRY(EdgeFlagv),
   TABLE_ENTRY(End),
   TABLE_ENTRY(Indexd),
   TABLE_ENTRY(Indexdv),
   TABLE_ENTRY(Indexf),
   TABLE_ENTRY(Indexfv),
   TABLE_ENTRY(Indexi),
   TABLE_ENTRY(Indexiv),
   TABLE_ENTRY(Indexs),
   TABLE_ENTRY(Indexsv),
   TABLE_ENTRY(Normal3b),
   TABLE_ENTRY(Normal3bv),
   TABLE_ENTRY(Normal3d),
   TABLE_ENTRY(Normal3dv),
   TABLE_ENTRY(Normal3f),
   TABLE_ENTRY(Normal3fv),
   TABLE_ENTRY(Normal3i),
   TABLE_ENTRY(Normal3iv),
   TABLE_ENTRY(Normal3s),
   TABLE_ENTRY(Normal3sv),
   TABLE_ENTRY(RasterPos2d),
   TABLE_ENTRY(RasterPos2dv),
   TABLE_ENTRY(RasterPos2f),
   TABLE_ENTRY(RasterPos2fv),
   TABLE_ENTRY(RasterPos2i),
   TABLE_ENTRY(RasterPos2iv),
   TABLE_ENTRY(RasterPos2s),
   TABLE_ENTRY(RasterPos2sv),
   TABLE_ENTRY(RasterPos3d),
   TABLE_ENTRY(RasterPos3dv),
   TABLE_ENTRY(RasterPos3f),
   TABLE_ENTRY(RasterPos3fv),
   TABLE_ENTRY(RasterPos3i),
   TABLE_ENTRY(RasterPos3iv),
   TABLE_ENTRY(RasterPos3s),
   TABLE_ENTRY(RasterPos3sv),
   TABLE_ENTRY(RasterPos4d),
   TABLE_ENTRY(RasterPos4dv),
   TABLE_ENTRY(RasterPos4f),
   TABLE_ENTRY(RasterPos4fv),
   TABLE_ENTRY(RasterPos4i),
   TABLE_ENTRY(RasterPos4iv),
   TABLE_ENTRY(RasterPos4s),
   TABLE_ENTRY(RasterPos4sv),
   TABLE_ENTRY(Rectd),
   TABLE_ENTRY(Rectdv),
   TABLE_ENTRY(Rectf),
   TABLE_ENTRY(Rectfv),
   TABLE_ENTRY(Recti),
   TABLE_ENTRY(Rectiv),
   TABLE_ENTRY(Rects),
   TABLE_ENTRY(Rectsv),
   TABLE_ENTRY(TexCoord1d),
   TABLE_ENTRY(TexCoord1dv),
   TABLE_ENTRY(TexCoord1f),
   TABLE_ENTRY(TexCoord1fv),
   TABLE_ENTRY(TexCoord1i),
   TABLE_ENTRY(TexCoord1iv),
   TABLE_ENTRY(TexCoord1s),
   TABLE_ENTRY(TexCoord1sv),
   TABLE_ENTRY(TexCoord2d),
   TABLE_ENTRY(TexCoord2dv),
   TABLE_ENTRY(TexCoord2f),
   TABLE_ENTRY(TexCoord2fv),
   TABLE_ENTRY(TexCoord2i),
   TABLE_ENTRY(TexCoord2iv),
   TABLE_ENTRY(TexCoord2s),
   TABLE_ENTRY(TexCoord2sv),
   TABLE_ENTRY(TexCoord3d),
   TABLE_ENTRY(TexCoord3dv),
   TABLE_ENTRY(TexCoord3f),
   TABLE_ENTRY(TexCoord3fv),
   TABLE_ENTRY(TexCoord3i),
   TABLE_ENTRY(TexCoord3iv),
   TABLE_ENTRY(TexCoord3s),
   TABLE_ENTRY(TexCoord3sv),
   TABLE_ENTRY(TexCoord4d),
   TABLE_ENTRY(TexCoord4dv),
   TABLE_ENTRY(TexCoord4f),
   TABLE_ENTRY(TexCoord4fv),
   TABLE_ENTRY(TexCoord4i),
   TABLE_ENTRY(TexCoord4iv),
   TABLE_ENTRY(TexCoord4s),
   TABLE_ENTRY(TexCoord4sv),
   TABLE_ENTRY(Vertex2d),
   TABLE_ENTRY(Vertex2dv),
   TABLE_ENTRY(Vertex2f),
   TABLE_ENTRY(Vertex2fv),
   TABLE_ENTRY(Vertex2i),
   TABLE_ENTRY(Vertex2iv),
   TABLE_ENTRY(Vertex2s),
   TABLE_ENTRY(Vertex2sv),
   TABLE_ENTRY(Vertex3d),
   TABLE_ENTRY(Vertex3dv),
   TABLE_ENTRY(Vertex3f),
   TABLE_ENTRY(Vertex3fv),
   TABLE_ENTRY(Vertex3i),
   TABLE_ENTRY(Vertex3iv),
   TABLE_ENTRY(Vertex3s),
   TABLE_ENTRY(Vertex3sv),
   TABLE_ENTRY(Vertex4d),
   TABLE_ENTRY(Vertex4dv),
   TABLE_ENTRY(Vertex4f),
   TABLE_ENTRY(Vertex4fv),
   TABLE_ENTRY(Vertex4i),
   TABLE_ENTRY(Vertex4iv),
   TABLE_ENTRY(Vertex4s),
   TABLE_ENTRY(Vertex4sv),
   TABLE_ENTRY(ClipPlane),
   TABLE_ENTRY(ColorMaterial),
   TABLE_ENTRY(CullFace),
   TABLE_ENTRY(Fogf),
   TABLE_ENTRY(Fogfv),
   TABLE_ENTRY(Fogi),
   TABLE_ENTRY(Fogiv),
   TABLE_ENTRY(FrontFace),
   TABLE_ENTRY(Hint),
   TABLE_ENTRY(Lightf),
   TABLE_ENTRY(Lightfv),
   TABLE_ENTRY(Lighti),
   TABLE_ENTRY(Lightiv),
   TABLE_ENTRY(LightModelf),
   TABLE_ENTRY(LightModelfv),
   TABLE_ENTRY(LightModeli),
   TABLE_ENTRY(LightModeliv),
   TABLE_ENTRY(LineStipple),
   TABLE_ENTRY(LineWidth),
   TABLE_ENTRY(Materialf),
   TABLE_ENTRY(Materialfv),
   TABLE_ENTRY(Materiali),
   TABLE_ENTRY(Materialiv),
   TABLE_ENTRY(PointSize),
   TABLE_ENTRY(PolygonMode),
   TABLE_ENTRY(PolygonStipple),
   TABLE_ENTRY(Scissor),
   TABLE_ENTRY(ShadeModel),
   TABLE_ENTRY(TexParameterf),
   TABLE_ENTRY(TexParameterfv),
   TABLE_ENTRY(TexParameteri),
   TABLE_ENTRY(TexParameteriv),
   TABLE_ENTRY(TexImage1D),
   TABLE_ENTRY(TexImage2D),
   TABLE_ENTRY(TexEnvf),
   TABLE_ENTRY(TexEnvfv),
   TABLE_ENTRY(TexEnvi),
   TABLE_ENTRY(TexEnviv),
   TABLE_ENTRY(TexGend),
   TABLE_ENTRY(TexGendv),
   TABLE_ENTRY(TexGenf),
   TABLE_ENTRY(TexGenfv),
   TABLE_ENTRY(TexGeni),
   TABLE_ENTRY(TexGeniv),
   TABLE_ENTRY(FeedbackBuffer),
   TABLE_ENTRY(SelectBuffer),
   TABLE_ENTRY(RenderMode),
   TABLE_ENTRY(InitNames),
   TABLE_ENTRY(LoadName),
   TABLE_ENTRY(PassThrough),
   TABLE_ENTRY(PopName),
   TABLE_ENTRY(PushName),
   TABLE_ENTRY(DrawBuffer),
   TABLE_ENTRY(Clear),
   TABLE_ENTRY(ClearAccum),
   TABLE_ENTRY(ClearIndex),
   TABLE_ENTRY(ClearColor),
   TABLE_ENTRY(ClearStencil),
   TABLE_ENTRY(ClearDepth),
   TABLE_ENTRY(StencilMask),
   TABLE_ENTRY(ColorMask),
   TABLE_ENTRY(DepthMask),
   TABLE_ENTRY(IndexMask),
   TABLE_ENTRY(Accum),
   TABLE_ENTRY(Disable),
   TABLE_ENTRY(Enable),
   TABLE_ENTRY(Finish),
   TABLE_ENTRY(Flush),
   TABLE_ENTRY(PopAttrib),
   TABLE_ENTRY(PushAttrib),
   TABLE_ENTRY(Map1d),
   TABLE_ENTRY(Map1f),
   TABLE_ENTRY(Map2d),
   TABLE_ENTRY(Map2f),
   TABLE_ENTRY(MapGrid1d),
   TABLE_ENTRY(MapGrid1f),
   TABLE_ENTRY(MapGrid2d),
   TABLE_ENTRY(MapGrid2f),
   TABLE_ENTRY(EvalCoord1d),
   TABLE_ENTRY(EvalCoord1dv),
   TABLE_ENTRY(EvalCoord1f),
   TABLE_ENTRY(EvalCoord1fv),
   TABLE_ENTRY(EvalCoord2d),
   TABLE_ENTRY(EvalCoord2dv),
   TABLE_ENTRY(EvalCoord2f),
   TABLE_ENTRY(EvalCoord2fv),
   TABLE_ENTRY(EvalMesh1),
   TABLE_ENTRY(EvalPoint1),
   TABLE_ENTRY(EvalMesh2),
   TABLE_ENTRY(EvalPoint2),
   TABLE_ENTRY(AlphaFunc),
   TABLE_ENTRY(BlendFunc),
   TABLE_ENTRY(LogicOp),
   TABLE_ENTRY(StencilFunc),
   TABLE_ENTRY(StencilOp),
   TABLE_ENTRY(DepthFunc),
   TABLE_ENTRY(PixelZoom),
   TABLE_ENTRY(PixelTransferf),
   TABLE_ENTRY(PixelTransferi),
   TABLE_ENTRY(PixelStoref),
   TABLE_ENTRY(PixelStorei),
   TABLE_ENTRY(PixelMapfv),
   TABLE_ENTRY(PixelMapuiv),
   TABLE_ENTRY(PixelMapusv),
   TABLE_ENTRY(ReadBuffer),
   TABLE_ENTRY(CopyPixels),
   TABLE_ENTRY(ReadPixels),
   TABLE_ENTRY(DrawPixels),
   TABLE_ENTRY(GetBooleanv),
   TABLE_ENTRY(GetClipPlane),
   TABLE_ENTRY(GetDoublev),
   TABLE_ENTRY(GetError),
   TABLE_ENTRY(GetFloatv),
   TABLE_ENTRY(GetIntegerv),
   TABLE_ENTRY(GetLightfv),
   TABLE_ENTRY(GetLightiv),
   TABLE_ENTRY(GetMapdv),
   TABLE_ENTRY(GetMapfv),
   TABLE_ENTRY(GetMapiv),
   TABLE_ENTRY(GetMaterialfv),
   TABLE_ENTRY(GetMaterialiv),
   TABLE_ENTRY(GetPixelMapfv),
   TABLE_ENTRY(GetPixelMapuiv),
   TABLE_ENTRY(GetPixelMapusv),
   TABLE_ENTRY(GetPolygonStipple),
   TABLE_ENTRY(GetString),
   TABLE_ENTRY(GetTexEnvfv),
   TABLE_ENTRY(GetTexEnviv),
   TABLE_ENTRY(GetTexGendv),
   TABLE_ENTRY(GetTexGenfv),
   TABLE_ENTRY(GetTexGeniv),
   TABLE_ENTRY(GetTexImage),
   TABLE_ENTRY(GetTexParameterfv),
   TABLE_ENTRY(GetTexParameteriv),
   TABLE_ENTRY(GetTexLevelParameterfv),
   TABLE_ENTRY(GetTexLevelParameteriv),
   TABLE_ENTRY(IsEnabled),
   TABLE_ENTRY(IsList),
   TABLE_ENTRY(DepthRange),
   TABLE_ENTRY(Frustum),
   TABLE_ENTRY(LoadIdentity),
   TABLE_ENTRY(LoadMatrixf),
   TABLE_ENTRY(LoadMatrixd),
   TABLE_ENTRY(MatrixMode),
   TABLE_ENTRY(MultMatrixf),
   TABLE_ENTRY(MultMatrixd),
   TABLE_ENTRY(Ortho),
   TABLE_ENTRY(PopMatrix),
   TABLE_ENTRY(PushMatrix),
   TABLE_ENTRY(Rotated),
   TABLE_ENTRY(Rotatef),
   TABLE_ENTRY(Scaled),
   TABLE_ENTRY(Scalef),
   TABLE_ENTRY(Translated),
   TABLE_ENTRY(Translatef),
   TABLE_ENTRY(Viewport),
   TABLE_ENTRY(ArrayElement),
   TABLE_ENTRY(BindTexture),
   TABLE_ENTRY(ColorPointer),
   TABLE_ENTRY(DisableClientState),
   TABLE_ENTRY(DrawArrays),
   TABLE_ENTRY(DrawElements),
   TABLE_ENTRY(EdgeFlagPointer),
   TABLE_ENTRY(EnableClientState),
   TABLE_ENTRY(IndexPointer),
   TABLE_ENTRY(Indexub),
   TABLE_ENTRY(Indexubv),
   TABLE_ENTRY(InterleavedArrays),
   TABLE_ENTRY(NormalPointer),
   TABLE_ENTRY(PolygonOffset),
   TABLE_ENTRY(TexCoordPointer),
   TABLE_ENTRY(VertexPointer),
   TABLE_ENTRY(AreTexturesResident),
   TABLE_ENTRY(CopyTexImage1D),
   TABLE_ENTRY(CopyTexImage2D),
   TABLE_ENTRY(CopyTexSubImage1D),
   TABLE_ENTRY(CopyTexSubImage2D),
   TABLE_ENTRY(DeleteTextures),
   TABLE_ENTRY(GenTextures),
   TABLE_ENTRY(GetPointerv),
   TABLE_ENTRY(IsTexture),
   TABLE_ENTRY(PrioritizeTextures),
   TABLE_ENTRY(TexSubImage1D),
   TABLE_ENTRY(TexSubImage2D),
   TABLE_ENTRY(PopClientAttrib),
   TABLE_ENTRY(PushClientAttrib),
   TABLE_ENTRY(BlendColor),
   TABLE_ENTRY(BlendEquation),
   TABLE_ENTRY(DrawRangeElements),
   TABLE_ENTRY(ColorTable),
   TABLE_ENTRY(ColorTableParameterfv),
   TABLE_ENTRY(ColorTableParameteriv),
   TABLE_ENTRY(CopyColorTable),
   TABLE_ENTRY(GetColorTable),
   TABLE_ENTRY(GetColorTableParameterfv),
   TABLE_ENTRY(GetColorTableParameteriv),
   TABLE_ENTRY(ColorSubTable),
   TABLE_ENTRY(CopyColorSubTable),
   TABLE_ENTRY(ConvolutionFilter1D),
   TABLE_ENTRY(ConvolutionFilter2D),
   TABLE_ENTRY(ConvolutionParameterf),
   TABLE_ENTRY(ConvolutionParameterfv),
   TABLE_ENTRY(ConvolutionParameteri),
   TABLE_ENTRY(ConvolutionParameteriv),
   TABLE_ENTRY(CopyConvolutionFilter1D),
   TABLE_ENTRY(CopyConvolutionFilter2D),
   TABLE_ENTRY(GetConvolutionFilter),
   TABLE_ENTRY(GetConvolutionParameterfv),
   TABLE_ENTRY(GetConvolutionParameteriv),
   TABLE_ENTRY(GetSeparableFilter),
   TABLE_ENTRY(SeparableFilter2D),
   TABLE_ENTRY(GetHistogram),
   TABLE_ENTRY(GetHistogramParameterfv),
   TABLE_ENTRY(GetHistogramParameteriv),
   TABLE_ENTRY(GetMinmax),
   TABLE_ENTRY(GetMinmaxParameterfv),
   TABLE_ENTRY(GetMinmaxParameteriv),
   TABLE_ENTRY(Histogram),
   TABLE_ENTRY(Minmax),
   TABLE_ENTRY(ResetHistogram),
   TABLE_ENTRY(ResetMinmax),
   TABLE_ENTRY(TexImage3D),
   TABLE_ENTRY(TexSubImage3D),
   TABLE_ENTRY(CopyTexSubImage3D),
   TABLE_ENTRY(ActiveTextureARB),
   TABLE_ENTRY(ClientActiveTextureARB),
   TABLE_ENTRY(MultiTexCoord1dARB),
   TABLE_ENTRY(MultiTexCoord1dvARB),
   TABLE_ENTRY(MultiTexCoord1fARB),
   TABLE_ENTRY(MultiTexCoord1fvARB),
   TABLE_ENTRY(MultiTexCoord1iARB),
   TABLE_ENTRY(MultiTexCoord1ivARB),
   TABLE_ENTRY(MultiTexCoord1sARB),
   TABLE_ENTRY(MultiTexCoord1svARB),
   TABLE_ENTRY(MultiTexCoord2dARB),
   TABLE_ENTRY(MultiTexCoord2dvARB),
   TABLE_ENTRY(MultiTexCoord2fARB),
   TABLE_ENTRY(MultiTexCoord2fvARB),
   TABLE_ENTRY(MultiTexCoord2iARB),
   TABLE_ENTRY(MultiTexCoord2ivARB),
   TABLE_ENTRY(MultiTexCoord2sARB),
   TABLE_ENTRY(MultiTexCoord2svARB),
   TABLE_ENTRY(MultiTexCoord3dARB),
   TABLE_ENTRY(MultiTexCoord3dvARB),
   TABLE_ENTRY(MultiTexCoord3fARB),
   TABLE_ENTRY(MultiTexCoord3fvARB),
   TABLE_ENTRY(MultiTexCoord3iARB),
   TABLE_ENTRY(MultiTexCoord3ivARB),
   TABLE_ENTRY(MultiTexCoord3sARB),
   TABLE_ENTRY(MultiTexCoord3svARB),
   TABLE_ENTRY(MultiTexCoord4dARB),
   TABLE_ENTRY(MultiTexCoord4dvARB),
   TABLE_ENTRY(MultiTexCoord4fARB),
   TABLE_ENTRY(MultiTexCoord4fvARB),
   TABLE_ENTRY(MultiTexCoord4iARB),
   TABLE_ENTRY(MultiTexCoord4ivARB),
   TABLE_ENTRY(MultiTexCoord4sARB),
   TABLE_ENTRY(MultiTexCoord4svARB),
   TABLE_ENTRY(AttachShader),
   TABLE_ENTRY(CreateProgram),
   TABLE_ENTRY(CreateShader),
   TABLE_ENTRY(DeleteProgram),
   TABLE_ENTRY(DeleteShader),
   TABLE_ENTRY(DetachShader),
   TABLE_ENTRY(GetAttachedShaders),
   TABLE_ENTRY(GetProgramInfoLog),
   TABLE_ENTRY(GetProgramiv),
   TABLE_ENTRY(GetShaderInfoLog),
   TABLE_ENTRY(GetShaderiv),
   TABLE_ENTRY(IsProgram),
   TABLE_ENTRY(IsShader),
   TABLE_ENTRY(StencilFuncSeparate),
   TABLE_ENTRY(StencilMaskSeparate),
   TABLE_ENTRY(StencilOpSeparate),
   TABLE_ENTRY(UniformMatrix2x3fv),
   TABLE_ENTRY(UniformMatrix2x4fv),
   TABLE_ENTRY(UniformMatrix3x2fv),
   TABLE_ENTRY(UniformMatrix3x4fv),
   TABLE_ENTRY(UniformMatrix4x2fv),
   TABLE_ENTRY(UniformMatrix4x3fv),
   TABLE_ENTRY(LoadTransposeMatrixdARB),
   TABLE_ENTRY(LoadTransposeMatrixfARB),
   TABLE_ENTRY(MultTransposeMatrixdARB),
   TABLE_ENTRY(MultTransposeMatrixfARB),
   TABLE_ENTRY(SampleCoverageARB),
   TABLE_ENTRY(CompressedTexImage1DARB),
   TABLE_ENTRY(CompressedTexImage2DARB),
   TABLE_ENTRY(CompressedTexImage3DARB),
   TABLE_ENTRY(CompressedTexSubImage1DARB),
   TABLE_ENTRY(CompressedTexSubImage2DARB),
   TABLE_ENTRY(CompressedTexSubImage3DARB),
   TABLE_ENTRY(GetCompressedTexImageARB),
   TABLE_ENTRY(DisableVertexAttribArrayARB),
   TABLE_ENTRY(EnableVertexAttribArrayARB),
   TABLE_ENTRY(GetProgramEnvParameterdvARB),
   TABLE_ENTRY(GetProgramEnvParameterfvARB),
   TABLE_ENTRY(GetProgramLocalParameterdvARB),
   TABLE_ENTRY(GetProgramLocalParameterfvARB),
   TABLE_ENTRY(GetProgramStringARB),
   TABLE_ENTRY(GetProgramivARB),
   TABLE_ENTRY(GetVertexAttribdvARB),
   TABLE_ENTRY(GetVertexAttribfvARB),
   TABLE_ENTRY(GetVertexAttribivARB),
   TABLE_ENTRY(ProgramEnvParameter4dARB),
   TABLE_ENTRY(ProgramEnvParameter4dvARB),
   TABLE_ENTRY(ProgramEnvParameter4fARB),
   TABLE_ENTRY(ProgramEnvParameter4fvARB),
   TABLE_ENTRY(ProgramLocalParameter4dARB),
   TABLE_ENTRY(ProgramLocalParameter4dvARB),
   TABLE_ENTRY(ProgramLocalParameter4fARB),
   TABLE_ENTRY(ProgramLocalParameter4fvARB),
   TABLE_ENTRY(ProgramStringARB),
   TABLE_ENTRY(VertexAttrib1dARB),
   TABLE_ENTRY(VertexAttrib1dvARB),
   TABLE_ENTRY(VertexAttrib1fARB),
   TABLE_ENTRY(VertexAttrib1fvARB),
   TABLE_ENTRY(VertexAttrib1sARB),
   TABLE_ENTRY(VertexAttrib1svARB),
   TABLE_ENTRY(VertexAttrib2dARB),
   TABLE_ENTRY(VertexAttrib2dvARB),
   TABLE_ENTRY(VertexAttrib2fARB),
   TABLE_ENTRY(VertexAttrib2fvARB),
   TABLE_ENTRY(VertexAttrib2sARB),
   TABLE_ENTRY(VertexAttrib2svARB),
   TABLE_ENTRY(VertexAttrib3dARB),
   TABLE_ENTRY(VertexAttrib3dvARB),
   TABLE_ENTRY(VertexAttrib3fARB),
   TABLE_ENTRY(VertexAttrib3fvARB),
   TABLE_ENTRY(VertexAttrib3sARB),
   TABLE_ENTRY(VertexAttrib3svARB),
   TABLE_ENTRY(VertexAttrib4NbvARB),
   TABLE_ENTRY(VertexAttrib4NivARB),
   TABLE_ENTRY(VertexAttrib4NsvARB),
   TABLE_ENTRY(VertexAttrib4NubARB),
   TABLE_ENTRY(VertexAttrib4NubvARB),
   TABLE_ENTRY(VertexAttrib4NuivARB),
   TABLE_ENTRY(VertexAttrib4NusvARB),
   TABLE_ENTRY(VertexAttrib4bvARB),
   TABLE_ENTRY(VertexAttrib4dARB),
   TABLE_ENTRY(VertexAttrib4dvARB),
   TABLE_ENTRY(VertexAttrib4fARB),
   TABLE_ENTRY(VertexAttrib4fvARB),
   TABLE_ENTRY(VertexAttrib4ivARB),
   TABLE_ENTRY(VertexAttrib4sARB),
   TABLE_ENTRY(VertexAttrib4svARB),
   TABLE_ENTRY(VertexAttrib4ubvARB),
   TABLE_ENTRY(VertexAttrib4uivARB),
   TABLE_ENTRY(VertexAttrib4usvARB),
   TABLE_ENTRY(VertexAttribPointerARB),
   TABLE_ENTRY(BindBufferARB),
   TABLE_ENTRY(BufferDataARB),
   TABLE_ENTRY(BufferSubDataARB),
   TABLE_ENTRY(DeleteBuffersARB),
   TABLE_ENTRY(GenBuffersARB),
   TABLE_ENTRY(GetBufferParameterivARB),
   TABLE_ENTRY(GetBufferPointervARB),
   TABLE_ENTRY(GetBufferSubDataARB),
   TABLE_ENTRY(IsBufferARB),
   TABLE_ENTRY(MapBufferARB),
   TABLE_ENTRY(UnmapBufferARB),
   TABLE_ENTRY(BeginQueryARB),
   TABLE_ENTRY(DeleteQueriesARB),
   TABLE_ENTRY(EndQueryARB),
   TABLE_ENTRY(GenQueriesARB),
   TABLE_ENTRY(GetQueryObjectivARB),
   TABLE_ENTRY(GetQueryObjectuivARB),
   TABLE_ENTRY(GetQueryivARB),
   TABLE_ENTRY(IsQueryARB),
   TABLE_ENTRY(AttachObjectARB),
   TABLE_ENTRY(CompileShaderARB),
   TABLE_ENTRY(CreateProgramObjectARB),
   TABLE_ENTRY(CreateShaderObjectARB),
   TABLE_ENTRY(DeleteObjectARB),
   TABLE_ENTRY(DetachObjectARB),
   TABLE_ENTRY(GetActiveUniformARB),
   TABLE_ENTRY(GetAttachedObjectsARB),
   TABLE_ENTRY(GetHandleARB),
   TABLE_ENTRY(GetInfoLogARB),
   TABLE_ENTRY(GetObjectParameterfvARB),
   TABLE_ENTRY(GetObjectParameterivARB),
   TABLE_ENTRY(GetShaderSourceARB),
   TABLE_ENTRY(GetUniformLocationARB),
   TABLE_ENTRY(GetUniformfvARB),
   TABLE_ENTRY(GetUniformivARB),
   TABLE_ENTRY(LinkProgramARB),
   TABLE_ENTRY(ShaderSourceARB),
   TABLE_ENTRY(Uniform1fARB),
   TABLE_ENTRY(Uniform1fvARB),
   TABLE_ENTRY(Uniform1iARB),
   TABLE_ENTRY(Uniform1ivARB),
   TABLE_ENTRY(Uniform2fARB),
   TABLE_ENTRY(Uniform2fvARB),
   TABLE_ENTRY(Uniform2iARB),
   TABLE_ENTRY(Uniform2ivARB),
   TABLE_ENTRY(Uniform3fARB),
   TABLE_ENTRY(Uniform3fvARB),
   TABLE_ENTRY(Uniform3iARB),
   TABLE_ENTRY(Uniform3ivARB),
   TABLE_ENTRY(Uniform4fARB),
   TABLE_ENTRY(Uniform4fvARB),
   TABLE_ENTRY(Uniform4iARB),
   TABLE_ENTRY(Uniform4ivARB),
   TABLE_ENTRY(UniformMatrix2fvARB),
   TABLE_ENTRY(UniformMatrix3fvARB),
   TABLE_ENTRY(UniformMatrix4fvARB),
   TABLE_ENTRY(UseProgramObjectARB),
   TABLE_ENTRY(ValidateProgramARB),
   TABLE_ENTRY(BindAttribLocationARB),
   TABLE_ENTRY(GetActiveAttribARB),
   TABLE_ENTRY(GetAttribLocationARB),
   TABLE_ENTRY(DrawBuffersARB),
   TABLE_ENTRY(PolygonOffsetEXT),
   TABLE_ENTRY(_dispatch_stub_562),
   TABLE_ENTRY(_dispatch_stub_563),
   TABLE_ENTRY(_dispatch_stub_564),
   TABLE_ENTRY(_dispatch_stub_565),
   TABLE_ENTRY(_dispatch_stub_566),
   TABLE_ENTRY(_dispatch_stub_567),
   TABLE_ENTRY(_dispatch_stub_568),
   TABLE_ENTRY(_dispatch_stub_569),
   TABLE_ENTRY(ColorPointerEXT),
   TABLE_ENTRY(EdgeFlagPointerEXT),
   TABLE_ENTRY(IndexPointerEXT),
   TABLE_ENTRY(NormalPointerEXT),
   TABLE_ENTRY(TexCoordPointerEXT),
   TABLE_ENTRY(VertexPointerEXT),
   TABLE_ENTRY(PointParameterfEXT),
   TABLE_ENTRY(PointParameterfvEXT),
   TABLE_ENTRY(LockArraysEXT),
   TABLE_ENTRY(UnlockArraysEXT),
   TABLE_ENTRY(_dispatch_stub_580),
   TABLE_ENTRY(_dispatch_stub_581),
   TABLE_ENTRY(SecondaryColor3bEXT),
   TABLE_ENTRY(SecondaryColor3bvEXT),
   TABLE_ENTRY(SecondaryColor3dEXT),
   TABLE_ENTRY(SecondaryColor3dvEXT),
   TABLE_ENTRY(SecondaryColor3fEXT),
   TABLE_ENTRY(SecondaryColor3fvEXT),
   TABLE_ENTRY(SecondaryColor3iEXT),
   TABLE_ENTRY(SecondaryColor3ivEXT),
   TABLE_ENTRY(SecondaryColor3sEXT),
   TABLE_ENTRY(SecondaryColor3svEXT),
   TABLE_ENTRY(SecondaryColor3ubEXT),
   TABLE_ENTRY(SecondaryColor3ubvEXT),
   TABLE_ENTRY(SecondaryColor3uiEXT),
   TABLE_ENTRY(SecondaryColor3uivEXT),
   TABLE_ENTRY(SecondaryColor3usEXT),
   TABLE_ENTRY(SecondaryColor3usvEXT),
   TABLE_ENTRY(SecondaryColorPointerEXT),
   TABLE_ENTRY(MultiDrawArraysEXT),
   TABLE_ENTRY(MultiDrawElementsEXT),
   TABLE_ENTRY(FogCoordPointerEXT),
   TABLE_ENTRY(FogCoorddEXT),
   TABLE_ENTRY(FogCoorddvEXT),
   TABLE_ENTRY(FogCoordfEXT),
   TABLE_ENTRY(FogCoordfvEXT),
   TABLE_ENTRY(_dispatch_stub_606),
   TABLE_ENTRY(BlendFuncSeparateEXT),
   TABLE_ENTRY(FlushVertexArrayRangeNV),
   TABLE_ENTRY(VertexArrayRangeNV),
   TABLE_ENTRY(CombinerInputNV),
   TABLE_ENTRY(CombinerOutputNV),
   TABLE_ENTRY(CombinerParameterfNV),
   TABLE_ENTRY(CombinerParameterfvNV),
   TABLE_ENTRY(CombinerParameteriNV),
   TABLE_ENTRY(CombinerParameterivNV),
   TABLE_ENTRY(FinalCombinerInputNV),
   TABLE_ENTRY(GetCombinerInputParameterfvNV),
   TABLE_ENTRY(GetCombinerInputParameterivNV),
   TABLE_ENTRY(GetCombinerOutputParameterfvNV),
   TABLE_ENTRY(GetCombinerOutputParameterivNV),
   TABLE_ENTRY(GetFinalCombinerInputParameterfvNV),
   TABLE_ENTRY(GetFinalCombinerInputParameterivNV),
   TABLE_ENTRY(ResizeBuffersMESA),
   TABLE_ENTRY(WindowPos2dMESA),
   TABLE_ENTRY(WindowPos2dvMESA),
   TABLE_ENTRY(WindowPos2fMESA),
   TABLE_ENTRY(WindowPos2fvMESA),
   TABLE_ENTRY(WindowPos2iMESA),
   TABLE_ENTRY(WindowPos2ivMESA),
   TABLE_ENTRY(WindowPos2sMESA),
   TABLE_ENTRY(WindowPos2svMESA),
   TABLE_ENTRY(WindowPos3dMESA),
   TABLE_ENTRY(WindowPos3dvMESA),
   TABLE_ENTRY(WindowPos3fMESA),
   TABLE_ENTRY(WindowPos3fvMESA),
   TABLE_ENTRY(WindowPos3iMESA),
   TABLE_ENTRY(WindowPos3ivMESA),
   TABLE_ENTRY(WindowPos3sMESA),
   TABLE_ENTRY(WindowPos3svMESA),
   TABLE_ENTRY(WindowPos4dMESA),
   TABLE_ENTRY(WindowPos4dvMESA),
   TABLE_ENTRY(WindowPos4fMESA),
   TABLE_ENTRY(WindowPos4fvMESA),
   TABLE_ENTRY(WindowPos4iMESA),
   TABLE_ENTRY(WindowPos4ivMESA),
   TABLE_ENTRY(WindowPos4sMESA),
   TABLE_ENTRY(WindowPos4svMESA),
   TABLE_ENTRY(_dispatch_stub_648),
   TABLE_ENTRY(_dispatch_stub_649),
   TABLE_ENTRY(_dispatch_stub_650),
   TABLE_ENTRY(_dispatch_stub_651),
   TABLE_ENTRY(_dispatch_stub_652),
   TABLE_ENTRY(_dispatch_stub_653),
   TABLE_ENTRY(_dispatch_stub_654),
   TABLE_ENTRY(_dispatch_stub_655),
   TABLE_ENTRY(_dispatch_stub_656),
   TABLE_ENTRY(AreProgramsResidentNV),
   TABLE_ENTRY(BindProgramNV),
   TABLE_ENTRY(DeleteProgramsNV),
   TABLE_ENTRY(ExecuteProgramNV),
   TABLE_ENTRY(GenProgramsNV),
   TABLE_ENTRY(GetProgramParameterdvNV),
   TABLE_ENTRY(GetProgramParameterfvNV),
   TABLE_ENTRY(GetProgramStringNV),
   TABLE_ENTRY(GetProgramivNV),
   TABLE_ENTRY(GetTrackMatrixivNV),
   TABLE_ENTRY(GetVertexAttribPointervNV),
   TABLE_ENTRY(GetVertexAttribdvNV),
   TABLE_ENTRY(GetVertexAttribfvNV),
   TABLE_ENTRY(GetVertexAttribivNV),
   TABLE_ENTRY(IsProgramNV),
   TABLE_ENTRY(LoadProgramNV),
   TABLE_ENTRY(ProgramParameters4dvNV),
   TABLE_ENTRY(ProgramParameters4fvNV),
   TABLE_ENTRY(RequestResidentProgramsNV),
   TABLE_ENTRY(TrackMatrixNV),
   TABLE_ENTRY(VertexAttrib1dNV),
   TABLE_ENTRY(VertexAttrib1dvNV),
   TABLE_ENTRY(VertexAttrib1fNV),
   TABLE_ENTRY(VertexAttrib1fvNV),
   TABLE_ENTRY(VertexAttrib1sNV),
   TABLE_ENTRY(VertexAttrib1svNV),
   TABLE_ENTRY(VertexAttrib2dNV),
   TABLE_ENTRY(VertexAttrib2dvNV),
   TABLE_ENTRY(VertexAttrib2fNV),
   TABLE_ENTRY(VertexAttrib2fvNV),
   TABLE_ENTRY(VertexAttrib2sNV),
   TABLE_ENTRY(VertexAttrib2svNV),
   TABLE_ENTRY(VertexAttrib3dNV),
   TABLE_ENTRY(VertexAttrib3dvNV),
   TABLE_ENTRY(VertexAttrib3fNV),
   TABLE_ENTRY(VertexAttrib3fvNV),
   TABLE_ENTRY(VertexAttrib3sNV),
   TABLE_ENTRY(VertexAttrib3svNV),
   TABLE_ENTRY(VertexAttrib4dNV),
   TABLE_ENTRY(VertexAttrib4dvNV),
   TABLE_ENTRY(VertexAttrib4fNV),
   TABLE_ENTRY(VertexAttrib4fvNV),
   TABLE_ENTRY(VertexAttrib4sNV),
   TABLE_ENTRY(VertexAttrib4svNV),
   TABLE_ENTRY(VertexAttrib4ubNV),
   TABLE_ENTRY(VertexAttrib4ubvNV),
   TABLE_ENTRY(VertexAttribPointerNV),
   TABLE_ENTRY(VertexAttribs1dvNV),
   TABLE_ENTRY(VertexAttribs1fvNV),
   TABLE_ENTRY(VertexAttribs1svNV),
   TABLE_ENTRY(VertexAttribs2dvNV),
   TABLE_ENTRY(VertexAttribs2fvNV),
   TABLE_ENTRY(VertexAttribs2svNV),
   TABLE_ENTRY(VertexAttribs3dvNV),
   TABLE_ENTRY(VertexAttribs3fvNV),
   TABLE_ENTRY(VertexAttribs3svNV),
   TABLE_ENTRY(VertexAttribs4dvNV),
   TABLE_ENTRY(VertexAttribs4fvNV),
   TABLE_ENTRY(VertexAttribs4svNV),
   TABLE_ENTRY(VertexAttribs4ubvNV),
   TABLE_ENTRY(AlphaFragmentOp1ATI),
   TABLE_ENTRY(AlphaFragmentOp2ATI),
   TABLE_ENTRY(AlphaFragmentOp3ATI),
   TABLE_ENTRY(BeginFragmentShaderATI),
   TABLE_ENTRY(BindFragmentShaderATI),
   TABLE_ENTRY(ColorFragmentOp1ATI),
   TABLE_ENTRY(ColorFragmentOp2ATI),
   TABLE_ENTRY(ColorFragmentOp3ATI),
   TABLE_ENTRY(DeleteFragmentShaderATI),
   TABLE_ENTRY(EndFragmentShaderATI),
   TABLE_ENTRY(GenFragmentShadersATI),
   TABLE_ENTRY(PassTexCoordATI),
   TABLE_ENTRY(SampleMapATI),
   TABLE_ENTRY(SetFragmentShaderConstantATI),
   TABLE_ENTRY(PointParameteriNV),
   TABLE_ENTRY(PointParameterivNV),
   TABLE_ENTRY(_dispatch_stub_733),
   TABLE_ENTRY(_dispatch_stub_734),
   TABLE_ENTRY(_dispatch_stub_735),
   TABLE_ENTRY(_dispatch_stub_736),
   TABLE_ENTRY(_dispatch_stub_737),
   TABLE_ENTRY(GetProgramNamedParameterdvNV),
   TABLE_ENTRY(GetProgramNamedParameterfvNV),
   TABLE_ENTRY(ProgramNamedParameter4dNV),
   TABLE_ENTRY(ProgramNamedParameter4dvNV),
   TABLE_ENTRY(ProgramNamedParameter4fNV),
   TABLE_ENTRY(ProgramNamedParameter4fvNV),
   TABLE_ENTRY(_dispatch_stub_744),
   TABLE_ENTRY(_dispatch_stub_745),
   TABLE_ENTRY(BindFramebufferEXT),
   TABLE_ENTRY(BindRenderbufferEXT),
   TABLE_ENTRY(CheckFramebufferStatusEXT),
   TABLE_ENTRY(DeleteFramebuffersEXT),
   TABLE_ENTRY(DeleteRenderbuffersEXT),
   TABLE_ENTRY(FramebufferRenderbufferEXT),
   TABLE_ENTRY(FramebufferTexture1DEXT),
   TABLE_ENTRY(FramebufferTexture2DEXT),
   TABLE_ENTRY(FramebufferTexture3DEXT),
   TABLE_ENTRY(GenFramebuffersEXT),
   TABLE_ENTRY(GenRenderbuffersEXT),
   TABLE_ENTRY(GenerateMipmapEXT),
   TABLE_ENTRY(GetFramebufferAttachmentParameterivEXT),
   TABLE_ENTRY(GetRenderbufferParameterivEXT),
   TABLE_ENTRY(IsFramebufferEXT),
   TABLE_ENTRY(IsRenderbufferEXT),
   TABLE_ENTRY(RenderbufferStorageEXT),
   TABLE_ENTRY(_dispatch_stub_763),
   TABLE_ENTRY(FramebufferTextureLayerEXT),
   TABLE_ENTRY(_dispatch_stub_765),
   TABLE_ENTRY(_dispatch_stub_766),
   TABLE_ENTRY(_dispatch_stub_767),
   TABLE_ENTRY(_dispatch_stub_768),
   TABLE_ENTRY(_dispatch_stub_769),
   /* A whole bunch of no-op functions.  These might be called
    * when someone tries to call a dynamically-registered
    * extension function without a current rendering context.
    */
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
   TABLE_ENTRY(Unused),
};
#endif /* DISPATCH_TABLE_NAME */


/*
 * This is just used to silence compiler warnings.
 * We list the functions which are not otherwise used.
 */
#ifdef UNUSED_TABLE_NAME
static _glapi_proc UNUSED_TABLE_NAME[] = {
   TABLE_ENTRY(ArrayElementEXT),
   TABLE_ENTRY(BindTextureEXT),
   TABLE_ENTRY(DrawArraysEXT),
#ifndef GLX_INDIRECT_RENDERING
   TABLE_ENTRY(AreTexturesResidentEXT),
#endif
   TABLE_ENTRY(CopyTexImage1DEXT),
   TABLE_ENTRY(CopyTexImage2DEXT),
   TABLE_ENTRY(CopyTexSubImage1DEXT),
   TABLE_ENTRY(CopyTexSubImage2DEXT),
#ifndef GLX_INDIRECT_RENDERING
   TABLE_ENTRY(DeleteTexturesEXT),
#endif
#ifndef GLX_INDIRECT_RENDERING
   TABLE_ENTRY(GenTexturesEXT),
#endif
   TABLE_ENTRY(GetPointervEXT),
#ifndef GLX_INDIRECT_RENDERING
   TABLE_ENTRY(IsTextureEXT),
#endif
   TABLE_ENTRY(PrioritizeTexturesEXT),
   TABLE_ENTRY(TexSubImage1DEXT),
   TABLE_ENTRY(TexSubImage2DEXT),
   TABLE_ENTRY(BlendColorEXT),
   TABLE_ENTRY(BlendEquationEXT),
   TABLE_ENTRY(DrawRangeElementsEXT),
   TABLE_ENTRY(ColorTableEXT),
#ifndef GLX_INDIRECT_RENDERING
   TABLE_ENTRY(GetColorTableEXT),
#endif
#ifndef GLX_INDIRECT_RENDERING
   TABLE_ENTRY(GetColorTableParameterfvEXT),
#endif
#ifndef GLX_INDIRECT_RENDERING
   TABLE_ENTRY(GetColorTableParameterivEXT),
#endif
   TABLE_ENTRY(TexImage3DEXT),
   TABLE_ENTRY(TexSubImage3DEXT),
   TABLE_ENTRY(CopyTexSubImage3DEXT),
   TABLE_ENTRY(ActiveTexture),
   TABLE_ENTRY(ClientActiveTexture),
   TABLE_ENTRY(MultiTexCoord1d),
   TABLE_ENTRY(MultiTexCoord1dv),
   TABLE_ENTRY(MultiTexCoord1f),
   TABLE_ENTRY(MultiTexCoord1fv),
   TABLE_ENTRY(MultiTexCoord1i),
   TABLE_ENTRY(MultiTexCoord1iv),
   TABLE_ENTRY(MultiTexCoord1s),
   TABLE_ENTRY(MultiTexCoord1sv),
   TABLE_ENTRY(MultiTexCoord2d),
   TABLE_ENTRY(MultiTexCoord2dv),
   TABLE_ENTRY(MultiTexCoord2f),
   TABLE_ENTRY(MultiTexCoord2fv),
   TABLE_ENTRY(MultiTexCoord2i),
   TABLE_ENTRY(MultiTexCoord2iv),
   TABLE_ENTRY(MultiTexCoord2s),
   TABLE_ENTRY(MultiTexCoord2sv),
   TABLE_ENTRY(MultiTexCoord3d),
   TABLE_ENTRY(MultiTexCoord3dv),
   TABLE_ENTRY(MultiTexCoord3f),
   TABLE_ENTRY(MultiTexCoord3fv),
   TABLE_ENTRY(MultiTexCoord3i),
   TABLE_ENTRY(MultiTexCoord3iv),
   TABLE_ENTRY(MultiTexCoord3s),
   TABLE_ENTRY(MultiTexCoord3sv),
   TABLE_ENTRY(MultiTexCoord4d),
   TABLE_ENTRY(MultiTexCoord4dv),
   TABLE_ENTRY(MultiTexCoord4f),
   TABLE_ENTRY(MultiTexCoord4fv),
   TABLE_ENTRY(MultiTexCoord4i),
   TABLE_ENTRY(MultiTexCoord4iv),
   TABLE_ENTRY(MultiTexCoord4s),
   TABLE_ENTRY(MultiTexCoord4sv),
   TABLE_ENTRY(LoadTransposeMatrixd),
   TABLE_ENTRY(LoadTransposeMatrixf),
   TABLE_ENTRY(MultTransposeMatrixd),
   TABLE_ENTRY(MultTransposeMatrixf),
   TABLE_ENTRY(SampleCoverage),
   TABLE_ENTRY(CompressedTexImage1D),
   TABLE_ENTRY(CompressedTexImage2D),
   TABLE_ENTRY(CompressedTexImage3D),
   TABLE_ENTRY(CompressedTexSubImage1D),
   TABLE_ENTRY(CompressedTexSubImage2D),
   TABLE_ENTRY(CompressedTexSubImage3D),
   TABLE_ENTRY(GetCompressedTexImage),
   TABLE_ENTRY(DisableVertexAttribArray),
   TABLE_ENTRY(EnableVertexAttribArray),
   TABLE_ENTRY(GetVertexAttribdv),
   TABLE_ENTRY(GetVertexAttribfv),
   TABLE_ENTRY(GetVertexAttribiv),
   TABLE_ENTRY(ProgramParameter4dNV),
   TABLE_ENTRY(ProgramParameter4dvNV),
   TABLE_ENTRY(ProgramParameter4fNV),
   TABLE_ENTRY(ProgramParameter4fvNV),
   TABLE_ENTRY(VertexAttrib1d),
   TABLE_ENTRY(VertexAttrib1dv),
   TABLE_ENTRY(VertexAttrib1f),
   TABLE_ENTRY(VertexAttrib1fv),
   TABLE_ENTRY(VertexAttrib1s),
   TABLE_ENTRY(VertexAttrib1sv),
   TABLE_ENTRY(VertexAttrib2d),
   TABLE_ENTRY(VertexAttrib2dv),
   TABLE_ENTRY(VertexAttrib2f),
   TABLE_ENTRY(VertexAttrib2fv),
   TABLE_ENTRY(VertexAttrib2s),
   TABLE_ENTRY(VertexAttrib2sv),
   TABLE_ENTRY(VertexAttrib3d),
   TABLE_ENTRY(VertexAttrib3dv),
   TABLE_ENTRY(VertexAttrib3f),
   TABLE_ENTRY(VertexAttrib3fv),
   TABLE_ENTRY(VertexAttrib3s),
   TABLE_ENTRY(VertexAttrib3sv),
   TABLE_ENTRY(VertexAttrib4Nbv),
   TABLE_ENTRY(VertexAttrib4Niv),
   TABLE_ENTRY(VertexAttrib4Nsv),
   TABLE_ENTRY(VertexAttrib4Nub),
   TABLE_ENTRY(VertexAttrib4Nubv),
   TABLE_ENTRY(VertexAttrib4Nuiv),
   TABLE_ENTRY(VertexAttrib4Nusv),
   TABLE_ENTRY(VertexAttrib4bv),
   TABLE_ENTRY(VertexAttrib4d),
   TABLE_ENTRY(VertexAttrib4dv),
   TABLE_ENTRY(VertexAttrib4f),
   TABLE_ENTRY(VertexAttrib4fv),
   TABLE_ENTRY(VertexAttrib4iv),
   TABLE_ENTRY(VertexAttrib4s),
   TABLE_ENTRY(VertexAttrib4sv),
   TABLE_ENTRY(VertexAttrib4ubv),
   TABLE_ENTRY(VertexAttrib4uiv),
   TABLE_ENTRY(VertexAttrib4usv),
   TABLE_ENTRY(VertexAttribPointer),
   TABLE_ENTRY(BindBuffer),
   TABLE_ENTRY(BufferData),
   TABLE_ENTRY(BufferSubData),
   TABLE_ENTRY(DeleteBuffers),
   TABLE_ENTRY(GenBuffers),
   TABLE_ENTRY(GetBufferParameteriv),
   TABLE_ENTRY(GetBufferPointerv),
   TABLE_ENTRY(GetBufferSubData),
   TABLE_ENTRY(IsBuffer),
   TABLE_ENTRY(MapBuffer),
   TABLE_ENTRY(UnmapBuffer),
   TABLE_ENTRY(BeginQuery),
   TABLE_ENTRY(DeleteQueries),
   TABLE_ENTRY(EndQuery),
   TABLE_ENTRY(GenQueries),
   TABLE_ENTRY(GetQueryObjectiv),
   TABLE_ENTRY(GetQueryObjectuiv),
   TABLE_ENTRY(GetQueryiv),
   TABLE_ENTRY(IsQuery),
   TABLE_ENTRY(CompileShader),
   TABLE_ENTRY(GetActiveUniform),
   TABLE_ENTRY(GetShaderSource),
   TABLE_ENTRY(GetUniformLocation),
   TABLE_ENTRY(GetUniformfv),
   TABLE_ENTRY(GetUniformiv),
   TABLE_ENTRY(LinkProgram),
   TABLE_ENTRY(ShaderSource),
   TABLE_ENTRY(Uniform1f),
   TABLE_ENTRY(Uniform1fv),
   TABLE_ENTRY(Uniform1i),
   TABLE_ENTRY(Uniform1iv),
   TABLE_ENTRY(Uniform2f),
   TABLE_ENTRY(Uniform2fv),
   TABLE_ENTRY(Uniform2i),
   TABLE_ENTRY(Uniform2iv),
   TABLE_ENTRY(Uniform3f),
   TABLE_ENTRY(Uniform3fv),
   TABLE_ENTRY(Uniform3i),
   TABLE_ENTRY(Uniform3iv),
   TABLE_ENTRY(Uniform4f),
   TABLE_ENTRY(Uniform4fv),
   TABLE_ENTRY(Uniform4i),
   TABLE_ENTRY(Uniform4iv),
   TABLE_ENTRY(UniformMatrix2fv),
   TABLE_ENTRY(UniformMatrix3fv),
   TABLE_ENTRY(UniformMatrix4fv),
   TABLE_ENTRY(UseProgram),
   TABLE_ENTRY(ValidateProgram),
   TABLE_ENTRY(BindAttribLocation),
   TABLE_ENTRY(GetActiveAttrib),
   TABLE_ENTRY(GetAttribLocation),
   TABLE_ENTRY(DrawBuffers),
   TABLE_ENTRY(DrawBuffersATI),
   TABLE_ENTRY(PointParameterf),
   TABLE_ENTRY(PointParameterfARB),
   TABLE_ENTRY(PointParameterfv),
   TABLE_ENTRY(PointParameterfvARB),
   TABLE_ENTRY(SecondaryColor3b),
   TABLE_ENTRY(SecondaryColor3bv),
   TABLE_ENTRY(SecondaryColor3d),
   TABLE_ENTRY(SecondaryColor3dv),
   TABLE_ENTRY(SecondaryColor3f),
   TABLE_ENTRY(SecondaryColor3fv),
   TABLE_ENTRY(SecondaryColor3i),
   TABLE_ENTRY(SecondaryColor3iv),
   TABLE_ENTRY(SecondaryColor3s),
   TABLE_ENTRY(SecondaryColor3sv),
   TABLE_ENTRY(SecondaryColor3ub),
   TABLE_ENTRY(SecondaryColor3ubv),
   TABLE_ENTRY(SecondaryColor3ui),
   TABLE_ENTRY(SecondaryColor3uiv),
   TABLE_ENTRY(SecondaryColor3us),
   TABLE_ENTRY(SecondaryColor3usv),
   TABLE_ENTRY(SecondaryColorPointer),
   TABLE_ENTRY(MultiDrawArrays),
   TABLE_ENTRY(MultiDrawElements),
   TABLE_ENTRY(FogCoordPointer),
   TABLE_ENTRY(FogCoordd),
   TABLE_ENTRY(FogCoorddv),
   TABLE_ENTRY(FogCoordf),
   TABLE_ENTRY(FogCoordfv),
   TABLE_ENTRY(BlendFuncSeparate),
   TABLE_ENTRY(WindowPos2d),
   TABLE_ENTRY(WindowPos2dARB),
   TABLE_ENTRY(WindowPos2dv),
   TABLE_ENTRY(WindowPos2dvARB),
   TABLE_ENTRY(WindowPos2f),
   TABLE_ENTRY(WindowPos2fARB),
   TABLE_ENTRY(WindowPos2fv),
   TABLE_ENTRY(WindowPos2fvARB),
   TABLE_ENTRY(WindowPos2i),
   TABLE_ENTRY(WindowPos2iARB),
   TABLE_ENTRY(WindowPos2iv),
   TABLE_ENTRY(WindowPos2ivARB),
   TABLE_ENTRY(WindowPos2s),
   TABLE_ENTRY(WindowPos2sARB),
   TABLE_ENTRY(WindowPos2sv),
   TABLE_ENTRY(WindowPos2svARB),
   TABLE_ENTRY(WindowPos3d),
   TABLE_ENTRY(WindowPos3dARB),
   TABLE_ENTRY(WindowPos3dv),
   TABLE_ENTRY(WindowPos3dvARB),
   TABLE_ENTRY(WindowPos3f),
   TABLE_ENTRY(WindowPos3fARB),
   TABLE_ENTRY(WindowPos3fv),
   TABLE_ENTRY(WindowPos3fvARB),
   TABLE_ENTRY(WindowPos3i),
   TABLE_ENTRY(WindowPos3iARB),
   TABLE_ENTRY(WindowPos3iv),
   TABLE_ENTRY(WindowPos3ivARB),
   TABLE_ENTRY(WindowPos3s),
   TABLE_ENTRY(WindowPos3sARB),
   TABLE_ENTRY(WindowPos3sv),
   TABLE_ENTRY(WindowPos3svARB),
   TABLE_ENTRY(BindProgramARB),
   TABLE_ENTRY(DeleteProgramsARB),
   TABLE_ENTRY(GenProgramsARB),
   TABLE_ENTRY(GetVertexAttribPointerv),
   TABLE_ENTRY(GetVertexAttribPointervARB),
   TABLE_ENTRY(IsProgramARB),
   TABLE_ENTRY(PointParameteri),
   TABLE_ENTRY(PointParameteriv),
   TABLE_ENTRY(BlendEquationSeparate),
};
#endif /*UNUSED_TABLE_NAME*/


#  undef KEYWORD1
#  undef KEYWORD1_ALT
#  undef KEYWORD2
#  undef NAME
#  undef DISPATCH
#  undef RETURN_DISPATCH
#  undef DISPATCH_TABLE_NAME
#  undef UNUSED_TABLE_NAME
#  undef TABLE_ENTRY
#  undef HIDDEN
