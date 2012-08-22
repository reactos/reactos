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


#  if (defined(__GNUC__) && !defined(__CYGWIN__) && !defined(__MINGW32__)) || (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590) && defined(__ELF__))
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


#ifndef _GLAPI_SKIP_NORMAL_ENTRY_POINTS

KEYWORD1 void KEYWORD2 NAME(NewList)(GLuint list, GLenum mode)
{
    (void) list; (void) mode;
   DISPATCH(NewList, (list, mode), (F, "glNewList(%d, 0x%x);\n", list, mode));
}

KEYWORD1 void KEYWORD2 NAME(EndList)(void)
{
   DISPATCH(EndList, (), (F, "glEndList();\n"));
}

KEYWORD1 void KEYWORD2 NAME(CallList)(GLuint list)
{
    (void) list;
   DISPATCH(CallList, (list), (F, "glCallList(%d);\n", list));
}

KEYWORD1 void KEYWORD2 NAME(CallLists)(GLsizei n, GLenum type, const GLvoid * lists)
{
    (void) n; (void) type; (void) lists;
   DISPATCH(CallLists, (n, type, lists), (F, "glCallLists(%d, 0x%x, %p);\n", n, type, (const void *) lists));
}

KEYWORD1 void KEYWORD2 NAME(DeleteLists)(GLuint list, GLsizei range)
{
    (void) list; (void) range;
   DISPATCH(DeleteLists, (list, range), (F, "glDeleteLists(%d, %d);\n", list, range));
}

KEYWORD1 GLuint KEYWORD2 NAME(GenLists)(GLsizei range)
{
    (void) range;
   RETURN_DISPATCH(GenLists, (range), (F, "glGenLists(%d);\n", range));
}

KEYWORD1 void KEYWORD2 NAME(ListBase)(GLuint base)
{
    (void) base;
   DISPATCH(ListBase, (base), (F, "glListBase(%d);\n", base));
}

KEYWORD1 void KEYWORD2 NAME(Begin)(GLenum mode)
{
    (void) mode;
   DISPATCH(Begin, (mode), (F, "glBegin(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(Bitmap)(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte * bitmap)
{
    (void) width; (void) height; (void) xorig; (void) yorig; (void) xmove; (void) ymove; (void) bitmap;
   DISPATCH(Bitmap, (width, height, xorig, yorig, xmove, ymove, bitmap), (F, "glBitmap(%d, %d, %f, %f, %f, %f, %p);\n", width, height, xorig, yorig, xmove, ymove, (const void *) bitmap));
}

KEYWORD1 void KEYWORD2 NAME(Color3b)(GLbyte red, GLbyte green, GLbyte blue)
{
    (void) red; (void) green; (void) blue;
   DISPATCH(Color3b, (red, green, blue), (F, "glColor3b(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color3bv)(const GLbyte * v)
{
    (void) v;
   DISPATCH(Color3bv, (v), (F, "glColor3bv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color3d)(GLdouble red, GLdouble green, GLdouble blue)
{
    (void) red; (void) green; (void) blue;
   DISPATCH(Color3d, (red, green, blue), (F, "glColor3d(%f, %f, %f);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color3dv)(const GLdouble * v)
{
    (void) v;
   DISPATCH(Color3dv, (v), (F, "glColor3dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color3f)(GLfloat red, GLfloat green, GLfloat blue)
{
    (void) red; (void) green; (void) blue;
   DISPATCH(Color3f, (red, green, blue), (F, "glColor3f(%f, %f, %f);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color3fv)(const GLfloat * v)
{
    (void) v;
   DISPATCH(Color3fv, (v), (F, "glColor3fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color3i)(GLint red, GLint green, GLint blue)
{
    (void) red; (void) green; (void) blue;
   DISPATCH(Color3i, (red, green, blue), (F, "glColor3i(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color3iv)(const GLint * v)
{
    (void) v;
   DISPATCH(Color3iv, (v), (F, "glColor3iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color3s)(GLshort red, GLshort green, GLshort blue)
{
    (void) red; (void) green; (void) blue;
   DISPATCH(Color3s, (red, green, blue), (F, "glColor3s(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color3sv)(const GLshort * v)
{
    (void) v;
   DISPATCH(Color3sv, (v), (F, "glColor3sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color3ub)(GLubyte red, GLubyte green, GLubyte blue)
{
    (void) red; (void) green; (void) blue;
   DISPATCH(Color3ub, (red, green, blue), (F, "glColor3ub(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color3ubv)(const GLubyte * v)
{
    (void) v;
   DISPATCH(Color3ubv, (v), (F, "glColor3ubv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color3ui)(GLuint red, GLuint green, GLuint blue)
{
    (void) red; (void) green; (void) blue;
   DISPATCH(Color3ui, (red, green, blue), (F, "glColor3ui(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color3uiv)(const GLuint * v)
{
    (void) v;
   DISPATCH(Color3uiv, (v), (F, "glColor3uiv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color3us)(GLushort red, GLushort green, GLushort blue)
{
    (void) red; (void) green; (void) blue;
   DISPATCH(Color3us, (red, green, blue), (F, "glColor3us(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(Color3usv)(const GLushort * v)
{
    (void) v;
   DISPATCH(Color3usv, (v), (F, "glColor3usv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color4b)(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
    (void) red; (void) green; (void) blue; (void) alpha;
   DISPATCH(Color4b, (red, green, blue, alpha), (F, "glColor4b(%d, %d, %d, %d);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(Color4bv)(const GLbyte * v)
{
    (void) v;
   DISPATCH(Color4bv, (v), (F, "glColor4bv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color4d)(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
    (void) red; (void) green; (void) blue; (void) alpha;
   DISPATCH(Color4d, (red, green, blue, alpha), (F, "glColor4d(%f, %f, %f, %f);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(Color4dv)(const GLdouble * v)
{
    (void) v;
   DISPATCH(Color4dv, (v), (F, "glColor4dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color4f)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    (void) red; (void) green; (void) blue; (void) alpha;
   DISPATCH(Color4f, (red, green, blue, alpha), (F, "glColor4f(%f, %f, %f, %f);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(Color4fv)(const GLfloat * v)
{
    (void) v;
   DISPATCH(Color4fv, (v), (F, "glColor4fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color4i)(GLint red, GLint green, GLint blue, GLint alpha)
{
    (void) red; (void) green; (void) blue; (void) alpha;
   DISPATCH(Color4i, (red, green, blue, alpha), (F, "glColor4i(%d, %d, %d, %d);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(Color4iv)(const GLint * v)
{
    (void) v;
   DISPATCH(Color4iv, (v), (F, "glColor4iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color4s)(GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
    (void) red; (void) green; (void) blue; (void) alpha;
   DISPATCH(Color4s, (red, green, blue, alpha), (F, "glColor4s(%d, %d, %d, %d);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(Color4sv)(const GLshort * v)
{
    (void) v;
   DISPATCH(Color4sv, (v), (F, "glColor4sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color4ub)(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    (void) red; (void) green; (void) blue; (void) alpha;
   DISPATCH(Color4ub, (red, green, blue, alpha), (F, "glColor4ub(%d, %d, %d, %d);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(Color4ubv)(const GLubyte * v)
{
    (void) v;
   DISPATCH(Color4ubv, (v), (F, "glColor4ubv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color4ui)(GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
    (void) red; (void) green; (void) blue; (void) alpha;
   DISPATCH(Color4ui, (red, green, blue, alpha), (F, "glColor4ui(%d, %d, %d, %d);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(Color4uiv)(const GLuint * v)
{
    (void) v;
   DISPATCH(Color4uiv, (v), (F, "glColor4uiv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Color4us)(GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
    (void) red; (void) green; (void) blue; (void) alpha;
   DISPATCH(Color4us, (red, green, blue, alpha), (F, "glColor4us(%d, %d, %d, %d);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(Color4usv)(const GLushort * v)
{
    (void) v;
   DISPATCH(Color4usv, (v), (F, "glColor4usv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(EdgeFlag)(GLboolean flag)
{
    (void) flag;
   DISPATCH(EdgeFlag, (flag), (F, "glEdgeFlag(%d);\n", flag));
}

KEYWORD1 void KEYWORD2 NAME(EdgeFlagv)(const GLboolean * flag)
{
    (void) flag;
   DISPATCH(EdgeFlagv, (flag), (F, "glEdgeFlagv(%p);\n", (const void *) flag));
}

KEYWORD1 void KEYWORD2 NAME(End)(void)
{
   DISPATCH(End, (), (F, "glEnd();\n"));
}

KEYWORD1 void KEYWORD2 NAME(Indexd)(GLdouble c)
{
    (void) c;
   DISPATCH(Indexd, (c), (F, "glIndexd(%f);\n", c));
}

KEYWORD1 void KEYWORD2 NAME(Indexdv)(const GLdouble * c)
{
    (void) c;
   DISPATCH(Indexdv, (c), (F, "glIndexdv(%p);\n", (const void *) c));
}

KEYWORD1 void KEYWORD2 NAME(Indexf)(GLfloat c)
{
    (void) c;
   DISPATCH(Indexf, (c), (F, "glIndexf(%f);\n", c));
}

KEYWORD1 void KEYWORD2 NAME(Indexfv)(const GLfloat * c)
{
    (void) c;
   DISPATCH(Indexfv, (c), (F, "glIndexfv(%p);\n", (const void *) c));
}

KEYWORD1 void KEYWORD2 NAME(Indexi)(GLint c)
{
    (void) c;
   DISPATCH(Indexi, (c), (F, "glIndexi(%d);\n", c));
}

KEYWORD1 void KEYWORD2 NAME(Indexiv)(const GLint * c)
{
    (void) c;
   DISPATCH(Indexiv, (c), (F, "glIndexiv(%p);\n", (const void *) c));
}

KEYWORD1 void KEYWORD2 NAME(Indexs)(GLshort c)
{
    (void) c;
   DISPATCH(Indexs, (c), (F, "glIndexs(%d);\n", c));
}

KEYWORD1 void KEYWORD2 NAME(Indexsv)(const GLshort * c)
{
    (void) c;
   DISPATCH(Indexsv, (c), (F, "glIndexsv(%p);\n", (const void *) c));
}

KEYWORD1 void KEYWORD2 NAME(Normal3b)(GLbyte nx, GLbyte ny, GLbyte nz)
{
    (void) nx; (void) ny; (void) nz;
   DISPATCH(Normal3b, (nx, ny, nz), (F, "glNormal3b(%d, %d, %d);\n", nx, ny, nz));
}

KEYWORD1 void KEYWORD2 NAME(Normal3bv)(const GLbyte * v)
{
    (void) v;
   DISPATCH(Normal3bv, (v), (F, "glNormal3bv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Normal3d)(GLdouble nx, GLdouble ny, GLdouble nz)
{
    (void) nx; (void) ny; (void) nz;
   DISPATCH(Normal3d, (nx, ny, nz), (F, "glNormal3d(%f, %f, %f);\n", nx, ny, nz));
}

KEYWORD1 void KEYWORD2 NAME(Normal3dv)(const GLdouble * v)
{
    (void) v;
   DISPATCH(Normal3dv, (v), (F, "glNormal3dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Normal3f)(GLfloat nx, GLfloat ny, GLfloat nz)
{
    (void) nx; (void) ny; (void) nz;
   DISPATCH(Normal3f, (nx, ny, nz), (F, "glNormal3f(%f, %f, %f);\n", nx, ny, nz));
}

KEYWORD1 void KEYWORD2 NAME(Normal3fv)(const GLfloat * v)
{
    (void) v;
   DISPATCH(Normal3fv, (v), (F, "glNormal3fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Normal3i)(GLint nx, GLint ny, GLint nz)
{
    (void) nx; (void) ny; (void) nz;
   DISPATCH(Normal3i, (nx, ny, nz), (F, "glNormal3i(%d, %d, %d);\n", nx, ny, nz));
}

KEYWORD1 void KEYWORD2 NAME(Normal3iv)(const GLint * v)
{
    (void) v;
   DISPATCH(Normal3iv, (v), (F, "glNormal3iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Normal3s)(GLshort nx, GLshort ny, GLshort nz)
{
    (void) nx; (void) ny; (void) nz;
   DISPATCH(Normal3s, (nx, ny, nz), (F, "glNormal3s(%d, %d, %d);\n", nx, ny, nz));
}

KEYWORD1 void KEYWORD2 NAME(Normal3sv)(const GLshort * v)
{
    (void) v;
   DISPATCH(Normal3sv, (v), (F, "glNormal3sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2d)(GLdouble x, GLdouble y)
{
    (void) x; (void) y;
   DISPATCH(RasterPos2d, (x, y), (F, "glRasterPos2d(%f, %f);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2dv)(const GLdouble * v)
{
    (void) v;
   DISPATCH(RasterPos2dv, (v), (F, "glRasterPos2dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2f)(GLfloat x, GLfloat y)
{
    (void) x; (void) y;
   DISPATCH(RasterPos2f, (x, y), (F, "glRasterPos2f(%f, %f);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2fv)(const GLfloat * v)
{
    (void) v;
   DISPATCH(RasterPos2fv, (v), (F, "glRasterPos2fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2i)(GLint x, GLint y)
{
    (void) x; (void) y;
   DISPATCH(RasterPos2i, (x, y), (F, "glRasterPos2i(%d, %d);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2iv)(const GLint * v)
{
    (void) v;
   DISPATCH(RasterPos2iv, (v), (F, "glRasterPos2iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2s)(GLshort x, GLshort y)
{
    (void) x; (void) y;
   DISPATCH(RasterPos2s, (x, y), (F, "glRasterPos2s(%d, %d);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos2sv)(const GLshort * v)
{
    (void) v;
   DISPATCH(RasterPos2sv, (v), (F, "glRasterPos2sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3d)(GLdouble x, GLdouble y, GLdouble z)
{
    (void) x; (void) y; (void) z;
   DISPATCH(RasterPos3d, (x, y, z), (F, "glRasterPos3d(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3dv)(const GLdouble * v)
{
    (void) v;
   DISPATCH(RasterPos3dv, (v), (F, "glRasterPos3dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3f)(GLfloat x, GLfloat y, GLfloat z)
{
    (void) x; (void) y; (void) z;
   DISPATCH(RasterPos3f, (x, y, z), (F, "glRasterPos3f(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3fv)(const GLfloat * v)
{
    (void) v;
   DISPATCH(RasterPos3fv, (v), (F, "glRasterPos3fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3i)(GLint x, GLint y, GLint z)
{
    (void) x; (void) y; (void) z;
   DISPATCH(RasterPos3i, (x, y, z), (F, "glRasterPos3i(%d, %d, %d);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3iv)(const GLint * v)
{
    (void) v;
   DISPATCH(RasterPos3iv, (v), (F, "glRasterPos3iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3s)(GLshort x, GLshort y, GLshort z)
{
    (void) x; (void) y; (void) z;
   DISPATCH(RasterPos3s, (x, y, z), (F, "glRasterPos3s(%d, %d, %d);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos3sv)(const GLshort * v)
{
    (void) v;
   DISPATCH(RasterPos3sv, (v), (F, "glRasterPos3sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    (void) x; (void) y; (void) z; (void) w;
   DISPATCH(RasterPos4d, (x, y, z, w), (F, "glRasterPos4d(%f, %f, %f, %f);\n", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4dv)(const GLdouble * v)
{
    (void) v;
   DISPATCH(RasterPos4dv, (v), (F, "glRasterPos4dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    (void) x; (void) y; (void) z; (void) w;
   DISPATCH(RasterPos4f, (x, y, z, w), (F, "glRasterPos4f(%f, %f, %f, %f);\n", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4fv)(const GLfloat * v)
{
    (void) v;
   DISPATCH(RasterPos4fv, (v), (F, "glRasterPos4fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4i)(GLint x, GLint y, GLint z, GLint w)
{
    (void) x; (void) y; (void) z; (void) w;
   DISPATCH(RasterPos4i, (x, y, z, w), (F, "glRasterPos4i(%d, %d, %d, %d);\n", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4iv)(const GLint * v)
{
    (void) v;
   DISPATCH(RasterPos4iv, (v), (F, "glRasterPos4iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4s)(GLshort x, GLshort y, GLshort z, GLshort w)
{
    (void) x; (void) y; (void) z; (void) w;
   DISPATCH(RasterPos4s, (x, y, z, w), (F, "glRasterPos4s(%d, %d, %d, %d);\n", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(RasterPos4sv)(const GLshort * v)
{
    (void) v;
   DISPATCH(RasterPos4sv, (v), (F, "glRasterPos4sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Rectd)(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
    (void) x1; (void) y1; (void) x2; (void) y2;
   DISPATCH(Rectd, (x1, y1, x2, y2), (F, "glRectd(%f, %f, %f, %f);\n", x1, y1, x2, y2));
}

KEYWORD1 void KEYWORD2 NAME(Rectdv)(const GLdouble * v1, const GLdouble * v2)
{
    (void) v1; (void) v2;
   DISPATCH(Rectdv, (v1, v2), (F, "glRectdv(%p, %p);\n", (const void *) v1, (const void *) v2));
}

KEYWORD1 void KEYWORD2 NAME(Rectf)(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    (void) x1; (void) y1; (void) x2; (void) y2;
   DISPATCH(Rectf, (x1, y1, x2, y2), (F, "glRectf(%f, %f, %f, %f);\n", x1, y1, x2, y2));
}

KEYWORD1 void KEYWORD2 NAME(Rectfv)(const GLfloat * v1, const GLfloat * v2)
{
    (void) v1; (void) v2;
   DISPATCH(Rectfv, (v1, v2), (F, "glRectfv(%p, %p);\n", (const void *) v1, (const void *) v2));
}

KEYWORD1 void KEYWORD2 NAME(Recti)(GLint x1, GLint y1, GLint x2, GLint y2)
{
    (void) x1; (void) y1; (void) x2; (void) y2;
   DISPATCH(Recti, (x1, y1, x2, y2), (F, "glRecti(%d, %d, %d, %d);\n", x1, y1, x2, y2));
}

KEYWORD1 void KEYWORD2 NAME(Rectiv)(const GLint * v1, const GLint * v2)
{
    (void) v1; (void) v2;
   DISPATCH(Rectiv, (v1, v2), (F, "glRectiv(%p, %p);\n", (const void *) v1, (const void *) v2));
}

KEYWORD1 void KEYWORD2 NAME(Rects)(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
    (void) x1; (void) y1; (void) x2; (void) y2;
   DISPATCH(Rects, (x1, y1, x2, y2), (F, "glRects(%d, %d, %d, %d);\n", x1, y1, x2, y2));
}

KEYWORD1 void KEYWORD2 NAME(Rectsv)(const GLshort * v1, const GLshort * v2)
{
    (void) v1; (void) v2;
   DISPATCH(Rectsv, (v1, v2), (F, "glRectsv(%p, %p);\n", (const void *) v1, (const void *) v2));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1d)(GLdouble s)
{
    (void) s;
   DISPATCH(TexCoord1d, (s), (F, "glTexCoord1d(%f);\n", s));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1dv)(const GLdouble * v)
{
    (void) v;
   DISPATCH(TexCoord1dv, (v), (F, "glTexCoord1dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1f)(GLfloat s)
{
    (void) s;
   DISPATCH(TexCoord1f, (s), (F, "glTexCoord1f(%f);\n", s));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1fv)(const GLfloat * v)
{
    (void) v;
   DISPATCH(TexCoord1fv, (v), (F, "glTexCoord1fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1i)(GLint s)
{
    (void) s;
   DISPATCH(TexCoord1i, (s), (F, "glTexCoord1i(%d);\n", s));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1iv)(const GLint * v)
{
    (void) v;
   DISPATCH(TexCoord1iv, (v), (F, "glTexCoord1iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1s)(GLshort s)
{
    (void) s;
   DISPATCH(TexCoord1s, (s), (F, "glTexCoord1s(%d);\n", s));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord1sv)(const GLshort * v)
{
    (void) v;
   DISPATCH(TexCoord1sv, (v), (F, "glTexCoord1sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2d)(GLdouble s, GLdouble t)
{
    (void) s; (void) t;
   DISPATCH(TexCoord2d, (s, t), (F, "glTexCoord2d(%f, %f);\n", s, t));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2dv)(const GLdouble * v)
{
    (void) v;
   DISPATCH(TexCoord2dv, (v), (F, "glTexCoord2dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2f)(GLfloat s, GLfloat t)
{
    (void) s; (void) t;
   DISPATCH(TexCoord2f, (s, t), (F, "glTexCoord2f(%f, %f);\n", s, t));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2fv)(const GLfloat * v)
{
    (void) v;
   DISPATCH(TexCoord2fv, (v), (F, "glTexCoord2fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2i)(GLint s, GLint t)
{
    (void) s; (void) t;
   DISPATCH(TexCoord2i, (s, t), (F, "glTexCoord2i(%d, %d);\n", s, t));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2iv)(const GLint * v)
{
    (void) v;
   DISPATCH(TexCoord2iv, (v), (F, "glTexCoord2iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2s)(GLshort s, GLshort t)
{
    (void) s; (void) t;
   DISPATCH(TexCoord2s, (s, t), (F, "glTexCoord2s(%d, %d);\n", s, t));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord2sv)(const GLshort * v)
{
    (void) v;
   DISPATCH(TexCoord2sv, (v), (F, "glTexCoord2sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3d)(GLdouble s, GLdouble t, GLdouble r)
{
    (void) s; (void) t; (void) r;
   DISPATCH(TexCoord3d, (s, t, r), (F, "glTexCoord3d(%f, %f, %f);\n", s, t, r));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3dv)(const GLdouble * v)
{
    (void) v;
   DISPATCH(TexCoord3dv, (v), (F, "glTexCoord3dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3f)(GLfloat s, GLfloat t, GLfloat r)
{
    (void) s; (void) t; (void) r;
   DISPATCH(TexCoord3f, (s, t, r), (F, "glTexCoord3f(%f, %f, %f);\n", s, t, r));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3fv)(const GLfloat * v)
{
    (void) v;
   DISPATCH(TexCoord3fv, (v), (F, "glTexCoord3fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3i)(GLint s, GLint t, GLint r)
{
    (void) s; (void) t; (void) r;
   DISPATCH(TexCoord3i, (s, t, r), (F, "glTexCoord3i(%d, %d, %d);\n", s, t, r));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3iv)(const GLint * v)
{
    (void) v;
   DISPATCH(TexCoord3iv, (v), (F, "glTexCoord3iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3s)(GLshort s, GLshort t, GLshort r)
{
    (void) s; (void) t; (void) r;
   DISPATCH(TexCoord3s, (s, t, r), (F, "glTexCoord3s(%d, %d, %d);\n", s, t, r));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord3sv)(const GLshort * v)
{
    (void) v;
   DISPATCH(TexCoord3sv, (v), (F, "glTexCoord3sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4d)(GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
    (void) s; (void) t; (void) r; (void) q;
   DISPATCH(TexCoord4d, (s, t, r, q), (F, "glTexCoord4d(%f, %f, %f, %f);\n", s, t, r, q));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4dv)(const GLdouble * v)
{
    (void) v;
   DISPATCH(TexCoord4dv, (v), (F, "glTexCoord4dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4f)(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    (void) s; (void) t; (void) r; (void) q;
   DISPATCH(TexCoord4f, (s, t, r, q), (F, "glTexCoord4f(%f, %f, %f, %f);\n", s, t, r, q));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4fv)(const GLfloat * v)
{
    (void) v;
   DISPATCH(TexCoord4fv, (v), (F, "glTexCoord4fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4i)(GLint s, GLint t, GLint r, GLint q)
{
    (void) s; (void) t; (void) r; (void) q;
   DISPATCH(TexCoord4i, (s, t, r, q), (F, "glTexCoord4i(%d, %d, %d, %d);\n", s, t, r, q));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4iv)(const GLint * v)
{
    (void) v;
   DISPATCH(TexCoord4iv, (v), (F, "glTexCoord4iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4s)(GLshort s, GLshort t, GLshort r, GLshort q)
{
    (void) s; (void) t; (void) r; (void) q;
   DISPATCH(TexCoord4s, (s, t, r, q), (F, "glTexCoord4s(%d, %d, %d, %d);\n", s, t, r, q));
}

KEYWORD1 void KEYWORD2 NAME(TexCoord4sv)(const GLshort * v)
{
    (void) v;
   DISPATCH(TexCoord4sv, (v), (F, "glTexCoord4sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2d)(GLdouble x, GLdouble y)
{
    (void) x; (void) y;
   DISPATCH(Vertex2d, (x, y), (F, "glVertex2d(%f, %f);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2dv)(const GLdouble * v)
{
    (void) v;
   DISPATCH(Vertex2dv, (v), (F, "glVertex2dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2f)(GLfloat x, GLfloat y)
{
    (void) x; (void) y;
   DISPATCH(Vertex2f, (x, y), (F, "glVertex2f(%f, %f);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2fv)(const GLfloat * v)
{
    (void) v;
   DISPATCH(Vertex2fv, (v), (F, "glVertex2fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2i)(GLint x, GLint y)
{
    (void) x; (void) y;
   DISPATCH(Vertex2i, (x, y), (F, "glVertex2i(%d, %d);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2iv)(const GLint * v)
{
    (void) v;
   DISPATCH(Vertex2iv, (v), (F, "glVertex2iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2s)(GLshort x, GLshort y)
{
    (void) x; (void) y;
   DISPATCH(Vertex2s, (x, y), (F, "glVertex2s(%d, %d);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(Vertex2sv)(const GLshort * v)
{
    (void) v;
   DISPATCH(Vertex2sv, (v), (F, "glVertex2sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3d)(GLdouble x, GLdouble y, GLdouble z)
{
    (void) x; (void) y; (void) z;
   DISPATCH(Vertex3d, (x, y, z), (F, "glVertex3d(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3dv)(const GLdouble * v)
{
    (void) v;
   DISPATCH(Vertex3dv, (v), (F, "glVertex3dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3f)(GLfloat x, GLfloat y, GLfloat z)
{
    (void) x; (void) y; (void) z;
   DISPATCH(Vertex3f, (x, y, z), (F, "glVertex3f(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3fv)(const GLfloat * v)
{
    (void) v;
   DISPATCH(Vertex3fv, (v), (F, "glVertex3fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3i)(GLint x, GLint y, GLint z)
{
    (void) x; (void) y; (void) z;
   DISPATCH(Vertex3i, (x, y, z), (F, "glVertex3i(%d, %d, %d);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3iv)(const GLint * v)
{
    (void) v;
   DISPATCH(Vertex3iv, (v), (F, "glVertex3iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3s)(GLshort x, GLshort y, GLshort z)
{
    (void) x; (void) y; (void) z;
   DISPATCH(Vertex3s, (x, y, z), (F, "glVertex3s(%d, %d, %d);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Vertex3sv)(const GLshort * v)
{
    (void) v;
   DISPATCH(Vertex3sv, (v), (F, "glVertex3sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4d)(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    (void) x; (void) y; (void) z; (void) w;
   DISPATCH(Vertex4d, (x, y, z, w), (F, "glVertex4d(%f, %f, %f, %f);\n", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4dv)(const GLdouble * v)
{
    (void) v;
   DISPATCH(Vertex4dv, (v), (F, "glVertex4dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4f)(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    (void) x; (void) y; (void) z; (void) w;
   DISPATCH(Vertex4f, (x, y, z, w), (F, "glVertex4f(%f, %f, %f, %f);\n", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4fv)(const GLfloat * v)
{
    (void) v;
   DISPATCH(Vertex4fv, (v), (F, "glVertex4fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4i)(GLint x, GLint y, GLint z, GLint w)
{
    (void) x; (void) y; (void) z; (void) w;
   DISPATCH(Vertex4i, (x, y, z, w), (F, "glVertex4i(%d, %d, %d, %d);\n", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4iv)(const GLint * v)
{
    (void) v;
   DISPATCH(Vertex4iv, (v), (F, "glVertex4iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4s)(GLshort x, GLshort y, GLshort z, GLshort w)
{
    (void) x; (void) y; (void) z; (void) w;
   DISPATCH(Vertex4s, (x, y, z, w), (F, "glVertex4s(%d, %d, %d, %d);\n", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(Vertex4sv)(const GLshort * v)
{
    (void) v;
   DISPATCH(Vertex4sv, (v), (F, "glVertex4sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(ClipPlane)(GLenum plane, const GLdouble * equation)
{
    (void) plane; (void) equation;
   DISPATCH(ClipPlane, (plane, equation), (F, "glClipPlane(0x%x, %p);\n", plane, (const void *) equation));
}

KEYWORD1 void KEYWORD2 NAME(ColorMaterial)(GLenum face, GLenum mode)
{
    (void) face; (void) mode;
   DISPATCH(ColorMaterial, (face, mode), (F, "glColorMaterial(0x%x, 0x%x);\n", face, mode));
}

KEYWORD1 void KEYWORD2 NAME(CullFace)(GLenum mode)
{
    (void) mode;
   DISPATCH(CullFace, (mode), (F, "glCullFace(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(Fogf)(GLenum pname, GLfloat param)
{
    (void) pname; (void) param;
   DISPATCH(Fogf, (pname, param), (F, "glFogf(0x%x, %f);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(Fogfv)(GLenum pname, const GLfloat * params)
{
    (void) pname; (void) params;
   DISPATCH(Fogfv, (pname, params), (F, "glFogfv(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(Fogi)(GLenum pname, GLint param)
{
    (void) pname; (void) param;
   DISPATCH(Fogi, (pname, param), (F, "glFogi(0x%x, %d);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(Fogiv)(GLenum pname, const GLint * params)
{
    (void) pname; (void) params;
   DISPATCH(Fogiv, (pname, params), (F, "glFogiv(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(FrontFace)(GLenum mode)
{
    (void) mode;
   DISPATCH(FrontFace, (mode), (F, "glFrontFace(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(Hint)(GLenum target, GLenum mode)
{
    (void) target; (void) mode;
   DISPATCH(Hint, (target, mode), (F, "glHint(0x%x, 0x%x);\n", target, mode));
}

KEYWORD1 void KEYWORD2 NAME(Lightf)(GLenum light, GLenum pname, GLfloat param)
{
    (void) light; (void) pname; (void) param;
   DISPATCH(Lightf, (light, pname, param), (F, "glLightf(0x%x, 0x%x, %f);\n", light, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(Lightfv)(GLenum light, GLenum pname, const GLfloat * params)
{
    (void) light; (void) pname; (void) params;
   DISPATCH(Lightfv, (light, pname, params), (F, "glLightfv(0x%x, 0x%x, %p);\n", light, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(Lighti)(GLenum light, GLenum pname, GLint param)
{
    (void) light; (void) pname; (void) param;
   DISPATCH(Lighti, (light, pname, param), (F, "glLighti(0x%x, 0x%x, %d);\n", light, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(Lightiv)(GLenum light, GLenum pname, const GLint * params)
{
    (void) light; (void) pname; (void) params;
   DISPATCH(Lightiv, (light, pname, params), (F, "glLightiv(0x%x, 0x%x, %p);\n", light, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(LightModelf)(GLenum pname, GLfloat param)
{
    (void) pname; (void) param;
   DISPATCH(LightModelf, (pname, param), (F, "glLightModelf(0x%x, %f);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(LightModelfv)(GLenum pname, const GLfloat * params)
{
    (void) pname; (void) params;
   DISPATCH(LightModelfv, (pname, params), (F, "glLightModelfv(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(LightModeli)(GLenum pname, GLint param)
{
    (void) pname; (void) param;
   DISPATCH(LightModeli, (pname, param), (F, "glLightModeli(0x%x, %d);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(LightModeliv)(GLenum pname, const GLint * params)
{
    (void) pname; (void) params;
   DISPATCH(LightModeliv, (pname, params), (F, "glLightModeliv(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(LineStipple)(GLint factor, GLushort pattern)
{
    (void) factor; (void) pattern;
   DISPATCH(LineStipple, (factor, pattern), (F, "glLineStipple(%d, %d);\n", factor, pattern));
}

KEYWORD1 void KEYWORD2 NAME(LineWidth)(GLfloat width)
{
    (void) width;
   DISPATCH(LineWidth, (width), (F, "glLineWidth(%f);\n", width));
}

KEYWORD1 void KEYWORD2 NAME(Materialf)(GLenum face, GLenum pname, GLfloat param)
{
    (void) face; (void) pname; (void) param;
   DISPATCH(Materialf, (face, pname, param), (F, "glMaterialf(0x%x, 0x%x, %f);\n", face, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(Materialfv)(GLenum face, GLenum pname, const GLfloat * params)
{
    (void) face; (void) pname; (void) params;
   DISPATCH(Materialfv, (face, pname, params), (F, "glMaterialfv(0x%x, 0x%x, %p);\n", face, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(Materiali)(GLenum face, GLenum pname, GLint param)
{
    (void) face; (void) pname; (void) param;
   DISPATCH(Materiali, (face, pname, param), (F, "glMateriali(0x%x, 0x%x, %d);\n", face, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(Materialiv)(GLenum face, GLenum pname, const GLint * params)
{
    (void) face; (void) pname; (void) params;
   DISPATCH(Materialiv, (face, pname, params), (F, "glMaterialiv(0x%x, 0x%x, %p);\n", face, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(PointSize)(GLfloat size)
{
    (void) size;
   DISPATCH(PointSize, (size), (F, "glPointSize(%f);\n", size));
}

KEYWORD1 void KEYWORD2 NAME(PolygonMode)(GLenum face, GLenum mode)
{
    (void) face; (void) mode;
   DISPATCH(PolygonMode, (face, mode), (F, "glPolygonMode(0x%x, 0x%x);\n", face, mode));
}

KEYWORD1 void KEYWORD2 NAME(PolygonStipple)(const GLubyte * mask)
{
    (void) mask;
   DISPATCH(PolygonStipple, (mask), (F, "glPolygonStipple(%p);\n", (const void *) mask));
}

KEYWORD1 void KEYWORD2 NAME(Scissor)(GLint x, GLint y, GLsizei width, GLsizei height)
{
    (void) x; (void) y; (void) width; (void) height;
   DISPATCH(Scissor, (x, y, width, height), (F, "glScissor(%d, %d, %d, %d);\n", x, y, width, height));
}

KEYWORD1 void KEYWORD2 NAME(ShadeModel)(GLenum mode)
{
    (void) mode;
   DISPATCH(ShadeModel, (mode), (F, "glShadeModel(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(TexParameterf)(GLenum target, GLenum pname, GLfloat param)
{
    (void) target; (void) pname; (void) param;
   DISPATCH(TexParameterf, (target, pname, param), (F, "glTexParameterf(0x%x, 0x%x, %f);\n", target, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(TexParameterfv)(GLenum target, GLenum pname, const GLfloat * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(TexParameterfv, (target, pname, params), (F, "glTexParameterfv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(TexParameteri)(GLenum target, GLenum pname, GLint param)
{
    (void) target; (void) pname; (void) param;
   DISPATCH(TexParameteri, (target, pname, param), (F, "glTexParameteri(0x%x, 0x%x, %d);\n", target, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(TexParameteriv)(GLenum target, GLenum pname, const GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(TexParameteriv, (target, pname, params), (F, "glTexParameteriv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(TexImage1D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid * pixels)
{
    (void) target; (void) level; (void) internalformat; (void) width; (void) border; (void) format; (void) type; (void) pixels;
   DISPATCH(TexImage1D, (target, level, internalformat, width, border, format, type, pixels), (F, "glTexImage1D(0x%x, %d, %d, %d, %d, 0x%x, 0x%x, %p);\n", target, level, internalformat, width, border, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(TexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * pixels)
{
    (void) target; (void) level; (void) internalformat; (void) width; (void) height; (void) border; (void) format; (void) type; (void) pixels;
   DISPATCH(TexImage2D, (target, level, internalformat, width, height, border, format, type, pixels), (F, "glTexImage2D(0x%x, %d, %d, %d, %d, %d, 0x%x, 0x%x, %p);\n", target, level, internalformat, width, height, border, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(TexEnvf)(GLenum target, GLenum pname, GLfloat param)
{
    (void) target; (void) pname; (void) param;
   DISPATCH(TexEnvf, (target, pname, param), (F, "glTexEnvf(0x%x, 0x%x, %f);\n", target, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(TexEnvfv)(GLenum target, GLenum pname, const GLfloat * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(TexEnvfv, (target, pname, params), (F, "glTexEnvfv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(TexEnvi)(GLenum target, GLenum pname, GLint param)
{
    (void) target; (void) pname; (void) param;
   DISPATCH(TexEnvi, (target, pname, param), (F, "glTexEnvi(0x%x, 0x%x, %d);\n", target, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(TexEnviv)(GLenum target, GLenum pname, const GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(TexEnviv, (target, pname, params), (F, "glTexEnviv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(TexGend)(GLenum coord, GLenum pname, GLdouble param)
{
    (void) coord; (void) pname; (void) param;
   DISPATCH(TexGend, (coord, pname, param), (F, "glTexGend(0x%x, 0x%x, %f);\n", coord, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(TexGendv)(GLenum coord, GLenum pname, const GLdouble * params)
{
    (void) coord; (void) pname; (void) params;
   DISPATCH(TexGendv, (coord, pname, params), (F, "glTexGendv(0x%x, 0x%x, %p);\n", coord, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(TexGenf)(GLenum coord, GLenum pname, GLfloat param)
{
    (void) coord; (void) pname; (void) param;
   DISPATCH(TexGenf, (coord, pname, param), (F, "glTexGenf(0x%x, 0x%x, %f);\n", coord, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(TexGenfv)(GLenum coord, GLenum pname, const GLfloat * params)
{
    (void) coord; (void) pname; (void) params;
   DISPATCH(TexGenfv, (coord, pname, params), (F, "glTexGenfv(0x%x, 0x%x, %p);\n", coord, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(TexGeni)(GLenum coord, GLenum pname, GLint param)
{
    (void) coord; (void) pname; (void) param;
   DISPATCH(TexGeni, (coord, pname, param), (F, "glTexGeni(0x%x, 0x%x, %d);\n", coord, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(TexGeniv)(GLenum coord, GLenum pname, const GLint * params)
{
    (void) coord; (void) pname; (void) params;
   DISPATCH(TexGeniv, (coord, pname, params), (F, "glTexGeniv(0x%x, 0x%x, %p);\n", coord, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(FeedbackBuffer)(GLsizei size, GLenum type, GLfloat * buffer)
{
    (void) size; (void) type; (void) buffer;
   DISPATCH(FeedbackBuffer, (size, type, buffer), (F, "glFeedbackBuffer(%d, 0x%x, %p);\n", size, type, (const void *) buffer));
}

KEYWORD1 void KEYWORD2 NAME(SelectBuffer)(GLsizei size, GLuint * buffer)
{
    (void) size; (void) buffer;
   DISPATCH(SelectBuffer, (size, buffer), (F, "glSelectBuffer(%d, %p);\n", size, (const void *) buffer));
}

KEYWORD1 GLint KEYWORD2 NAME(RenderMode)(GLenum mode)
{
    (void) mode;
   RETURN_DISPATCH(RenderMode, (mode), (F, "glRenderMode(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(InitNames)(void)
{
   DISPATCH(InitNames, (), (F, "glInitNames();\n"));
}

KEYWORD1 void KEYWORD2 NAME(LoadName)(GLuint name)
{
    (void) name;
   DISPATCH(LoadName, (name), (F, "glLoadName(%d);\n", name));
}

KEYWORD1 void KEYWORD2 NAME(PassThrough)(GLfloat token)
{
    (void) token;
   DISPATCH(PassThrough, (token), (F, "glPassThrough(%f);\n", token));
}

KEYWORD1 void KEYWORD2 NAME(PopName)(void)
{
   DISPATCH(PopName, (), (F, "glPopName();\n"));
}

KEYWORD1 void KEYWORD2 NAME(PushName)(GLuint name)
{
    (void) name;
   DISPATCH(PushName, (name), (F, "glPushName(%d);\n", name));
}

KEYWORD1 void KEYWORD2 NAME(DrawBuffer)(GLenum mode)
{
    (void) mode;
   DISPATCH(DrawBuffer, (mode), (F, "glDrawBuffer(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(Clear)(GLbitfield mask)
{
    (void) mask;
   DISPATCH(Clear, (mask), (F, "glClear(%d);\n", mask));
}

KEYWORD1 void KEYWORD2 NAME(ClearAccum)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    (void) red; (void) green; (void) blue; (void) alpha;
   DISPATCH(ClearAccum, (red, green, blue, alpha), (F, "glClearAccum(%f, %f, %f, %f);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(ClearIndex)(GLfloat c)
{
    (void) c;
   DISPATCH(ClearIndex, (c), (F, "glClearIndex(%f);\n", c));
}

KEYWORD1 void KEYWORD2 NAME(ClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    (void) red; (void) green; (void) blue; (void) alpha;
   DISPATCH(ClearColor, (red, green, blue, alpha), (F, "glClearColor(%f, %f, %f, %f);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(ClearStencil)(GLint s)
{
    (void) s;
   DISPATCH(ClearStencil, (s), (F, "glClearStencil(%d);\n", s));
}

KEYWORD1 void KEYWORD2 NAME(ClearDepth)(GLclampd depth)
{
    (void) depth;
   DISPATCH(ClearDepth, (depth), (F, "glClearDepth(%f);\n", depth));
}

KEYWORD1 void KEYWORD2 NAME(StencilMask)(GLuint mask)
{
    (void) mask;
   DISPATCH(StencilMask, (mask), (F, "glStencilMask(%d);\n", mask));
}

KEYWORD1 void KEYWORD2 NAME(ColorMask)(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    (void) red; (void) green; (void) blue; (void) alpha;
   DISPATCH(ColorMask, (red, green, blue, alpha), (F, "glColorMask(%d, %d, %d, %d);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(DepthMask)(GLboolean flag)
{
    (void) flag;
   DISPATCH(DepthMask, (flag), (F, "glDepthMask(%d);\n", flag));
}

KEYWORD1 void KEYWORD2 NAME(IndexMask)(GLuint mask)
{
    (void) mask;
   DISPATCH(IndexMask, (mask), (F, "glIndexMask(%d);\n", mask));
}

KEYWORD1 void KEYWORD2 NAME(Accum)(GLenum op, GLfloat value)
{
    (void) op; (void) value;
   DISPATCH(Accum, (op, value), (F, "glAccum(0x%x, %f);\n", op, value));
}

KEYWORD1 void KEYWORD2 NAME(Disable)(GLenum cap)
{
    (void) cap;
   DISPATCH(Disable, (cap), (F, "glDisable(0x%x);\n", cap));
}

KEYWORD1 void KEYWORD2 NAME(Enable)(GLenum cap)
{
    (void) cap;
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
    (void) mask;
   DISPATCH(PushAttrib, (mask), (F, "glPushAttrib(%d);\n", mask));
}

KEYWORD1 void KEYWORD2 NAME(Map1d)(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble * points)
{
    (void) target; (void) u1; (void) u2; (void) stride; (void) order; (void) points;
   DISPATCH(Map1d, (target, u1, u2, stride, order, points), (F, "glMap1d(0x%x, %f, %f, %d, %d, %p);\n", target, u1, u2, stride, order, (const void *) points));
}

KEYWORD1 void KEYWORD2 NAME(Map1f)(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat * points)
{
    (void) target; (void) u1; (void) u2; (void) stride; (void) order; (void) points;
   DISPATCH(Map1f, (target, u1, u2, stride, order, points), (F, "glMap1f(0x%x, %f, %f, %d, %d, %p);\n", target, u1, u2, stride, order, (const void *) points));
}

KEYWORD1 void KEYWORD2 NAME(Map2d)(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble * points)
{
    (void) target; (void) u1; (void) u2; (void) ustride; (void) uorder; (void) v1; (void) v2; (void) vstride; (void) vorder; (void) points;
   DISPATCH(Map2d, (target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points), (F, "glMap2d(0x%x, %f, %f, %d, %d, %f, %f, %d, %d, %p);\n", target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, (const void *) points));
}

KEYWORD1 void KEYWORD2 NAME(Map2f)(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat * points)
{
    (void) target; (void) u1; (void) u2; (void) ustride; (void) uorder; (void) v1; (void) v2; (void) vstride; (void) vorder; (void) points;
   DISPATCH(Map2f, (target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points), (F, "glMap2f(0x%x, %f, %f, %d, %d, %f, %f, %d, %d, %p);\n", target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, (const void *) points));
}

KEYWORD1 void KEYWORD2 NAME(MapGrid1d)(GLint un, GLdouble u1, GLdouble u2)
{
    (void) un; (void) u1; (void) u2;
   DISPATCH(MapGrid1d, (un, u1, u2), (F, "glMapGrid1d(%d, %f, %f);\n", un, u1, u2));
}

KEYWORD1 void KEYWORD2 NAME(MapGrid1f)(GLint un, GLfloat u1, GLfloat u2)
{
    (void) un; (void) u1; (void) u2;
   DISPATCH(MapGrid1f, (un, u1, u2), (F, "glMapGrid1f(%d, %f, %f);\n", un, u1, u2));
}

KEYWORD1 void KEYWORD2 NAME(MapGrid2d)(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
    (void) un; (void) u1; (void) u2; (void) vn; (void) v1; (void) v2;
   DISPATCH(MapGrid2d, (un, u1, u2, vn, v1, v2), (F, "glMapGrid2d(%d, %f, %f, %d, %f, %f);\n", un, u1, u2, vn, v1, v2));
}

KEYWORD1 void KEYWORD2 NAME(MapGrid2f)(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
    (void) un; (void) u1; (void) u2; (void) vn; (void) v1; (void) v2;
   DISPATCH(MapGrid2f, (un, u1, u2, vn, v1, v2), (F, "glMapGrid2f(%d, %f, %f, %d, %f, %f);\n", un, u1, u2, vn, v1, v2));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord1d)(GLdouble u)
{
    (void) u;
   DISPATCH(EvalCoord1d, (u), (F, "glEvalCoord1d(%f);\n", u));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord1dv)(const GLdouble * u)
{
    (void) u;
   DISPATCH(EvalCoord1dv, (u), (F, "glEvalCoord1dv(%p);\n", (const void *) u));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord1f)(GLfloat u)
{
    (void) u;
   DISPATCH(EvalCoord1f, (u), (F, "glEvalCoord1f(%f);\n", u));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord1fv)(const GLfloat * u)
{
    (void) u;
   DISPATCH(EvalCoord1fv, (u), (F, "glEvalCoord1fv(%p);\n", (const void *) u));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord2d)(GLdouble u, GLdouble v)
{
    (void) u; (void) v;
   DISPATCH(EvalCoord2d, (u, v), (F, "glEvalCoord2d(%f, %f);\n", u, v));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord2dv)(const GLdouble * u)
{
    (void) u;
   DISPATCH(EvalCoord2dv, (u), (F, "glEvalCoord2dv(%p);\n", (const void *) u));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord2f)(GLfloat u, GLfloat v)
{
    (void) u; (void) v;
   DISPATCH(EvalCoord2f, (u, v), (F, "glEvalCoord2f(%f, %f);\n", u, v));
}

KEYWORD1 void KEYWORD2 NAME(EvalCoord2fv)(const GLfloat * u)
{
    (void) u;
   DISPATCH(EvalCoord2fv, (u), (F, "glEvalCoord2fv(%p);\n", (const void *) u));
}

KEYWORD1 void KEYWORD2 NAME(EvalMesh1)(GLenum mode, GLint i1, GLint i2)
{
    (void) mode; (void) i1; (void) i2;
   DISPATCH(EvalMesh1, (mode, i1, i2), (F, "glEvalMesh1(0x%x, %d, %d);\n", mode, i1, i2));
}

KEYWORD1 void KEYWORD2 NAME(EvalPoint1)(GLint i)
{
    (void) i;
   DISPATCH(EvalPoint1, (i), (F, "glEvalPoint1(%d);\n", i));
}

KEYWORD1 void KEYWORD2 NAME(EvalMesh2)(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
    (void) mode; (void) i1; (void) i2; (void) j1; (void) j2;
   DISPATCH(EvalMesh2, (mode, i1, i2, j1, j2), (F, "glEvalMesh2(0x%x, %d, %d, %d, %d);\n", mode, i1, i2, j1, j2));
}

KEYWORD1 void KEYWORD2 NAME(EvalPoint2)(GLint i, GLint j)
{
    (void) i; (void) j;
   DISPATCH(EvalPoint2, (i, j), (F, "glEvalPoint2(%d, %d);\n", i, j));
}

KEYWORD1 void KEYWORD2 NAME(AlphaFunc)(GLenum func, GLclampf ref)
{
    (void) func; (void) ref;
   DISPATCH(AlphaFunc, (func, ref), (F, "glAlphaFunc(0x%x, %f);\n", func, ref));
}

KEYWORD1 void KEYWORD2 NAME(BlendFunc)(GLenum sfactor, GLenum dfactor)
{
    (void) sfactor; (void) dfactor;
   DISPATCH(BlendFunc, (sfactor, dfactor), (F, "glBlendFunc(0x%x, 0x%x);\n", sfactor, dfactor));
}

KEYWORD1 void KEYWORD2 NAME(LogicOp)(GLenum opcode)
{
    (void) opcode;
   DISPATCH(LogicOp, (opcode), (F, "glLogicOp(0x%x);\n", opcode));
}

KEYWORD1 void KEYWORD2 NAME(StencilFunc)(GLenum func, GLint ref, GLuint mask)
{
    (void) func; (void) ref; (void) mask;
   DISPATCH(StencilFunc, (func, ref, mask), (F, "glStencilFunc(0x%x, %d, %d);\n", func, ref, mask));
}

KEYWORD1 void KEYWORD2 NAME(StencilOp)(GLenum fail, GLenum zfail, GLenum zpass)
{
    (void) fail; (void) zfail; (void) zpass;
   DISPATCH(StencilOp, (fail, zfail, zpass), (F, "glStencilOp(0x%x, 0x%x, 0x%x);\n", fail, zfail, zpass));
}

KEYWORD1 void KEYWORD2 NAME(DepthFunc)(GLenum func)
{
    (void) func;
   DISPATCH(DepthFunc, (func), (F, "glDepthFunc(0x%x);\n", func));
}

KEYWORD1 void KEYWORD2 NAME(PixelZoom)(GLfloat xfactor, GLfloat yfactor)
{
    (void) xfactor; (void) yfactor;
   DISPATCH(PixelZoom, (xfactor, yfactor), (F, "glPixelZoom(%f, %f);\n", xfactor, yfactor));
}

KEYWORD1 void KEYWORD2 NAME(PixelTransferf)(GLenum pname, GLfloat param)
{
    (void) pname; (void) param;
   DISPATCH(PixelTransferf, (pname, param), (F, "glPixelTransferf(0x%x, %f);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(PixelTransferi)(GLenum pname, GLint param)
{
    (void) pname; (void) param;
   DISPATCH(PixelTransferi, (pname, param), (F, "glPixelTransferi(0x%x, %d);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(PixelStoref)(GLenum pname, GLfloat param)
{
    (void) pname; (void) param;
   DISPATCH(PixelStoref, (pname, param), (F, "glPixelStoref(0x%x, %f);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(PixelStorei)(GLenum pname, GLint param)
{
    (void) pname; (void) param;
   DISPATCH(PixelStorei, (pname, param), (F, "glPixelStorei(0x%x, %d);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(PixelMapfv)(GLenum map, GLsizei mapsize, const GLfloat * values)
{
    (void) map; (void) mapsize; (void) values;
   DISPATCH(PixelMapfv, (map, mapsize, values), (F, "glPixelMapfv(0x%x, %d, %p);\n", map, mapsize, (const void *) values));
}

KEYWORD1 void KEYWORD2 NAME(PixelMapuiv)(GLenum map, GLsizei mapsize, const GLuint * values)
{
    (void) map; (void) mapsize; (void) values;
   DISPATCH(PixelMapuiv, (map, mapsize, values), (F, "glPixelMapuiv(0x%x, %d, %p);\n", map, mapsize, (const void *) values));
}

KEYWORD1 void KEYWORD2 NAME(PixelMapusv)(GLenum map, GLsizei mapsize, const GLushort * values)
{
    (void) map; (void) mapsize; (void) values;
   DISPATCH(PixelMapusv, (map, mapsize, values), (F, "glPixelMapusv(0x%x, %d, %p);\n", map, mapsize, (const void *) values));
}

KEYWORD1 void KEYWORD2 NAME(ReadBuffer)(GLenum mode)
{
    (void) mode;
   DISPATCH(ReadBuffer, (mode), (F, "glReadBuffer(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(CopyPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
    (void) x; (void) y; (void) width; (void) height; (void) type;
   DISPATCH(CopyPixels, (x, y, width, height, type), (F, "glCopyPixels(%d, %d, %d, %d, 0x%x);\n", x, y, width, height, type));
}

KEYWORD1 void KEYWORD2 NAME(ReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * pixels)
{
    (void) x; (void) y; (void) width; (void) height; (void) format; (void) type; (void) pixels;
   DISPATCH(ReadPixels, (x, y, width, height, format, type, pixels), (F, "glReadPixels(%d, %d, %d, %d, 0x%x, 0x%x, %p);\n", x, y, width, height, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(DrawPixels)(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels)
{
    (void) width; (void) height; (void) format; (void) type; (void) pixels;
   DISPATCH(DrawPixels, (width, height, format, type, pixels), (F, "glDrawPixels(%d, %d, 0x%x, 0x%x, %p);\n", width, height, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(GetBooleanv)(GLenum pname, GLboolean * params)
{
    (void) pname; (void) params;
   DISPATCH(GetBooleanv, (pname, params), (F, "glGetBooleanv(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetClipPlane)(GLenum plane, GLdouble * equation)
{
    (void) plane; (void) equation;
   DISPATCH(GetClipPlane, (plane, equation), (F, "glGetClipPlane(0x%x, %p);\n", plane, (const void *) equation));
}

KEYWORD1 void KEYWORD2 NAME(GetDoublev)(GLenum pname, GLdouble * params)
{
    (void) pname; (void) params;
   DISPATCH(GetDoublev, (pname, params), (F, "glGetDoublev(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 GLenum KEYWORD2 NAME(GetError)(void)
{
   RETURN_DISPATCH(GetError, (), (F, "glGetError();\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetFloatv)(GLenum pname, GLfloat * params)
{
    (void) pname; (void) params;
   DISPATCH(GetFloatv, (pname, params), (F, "glGetFloatv(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetIntegerv)(GLenum pname, GLint * params)
{
    (void) pname; (void) params;
   DISPATCH(GetIntegerv, (pname, params), (F, "glGetIntegerv(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetLightfv)(GLenum light, GLenum pname, GLfloat * params)
{
    (void) light; (void) pname; (void) params;
   DISPATCH(GetLightfv, (light, pname, params), (F, "glGetLightfv(0x%x, 0x%x, %p);\n", light, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetLightiv)(GLenum light, GLenum pname, GLint * params)
{
    (void) light; (void) pname; (void) params;
   DISPATCH(GetLightiv, (light, pname, params), (F, "glGetLightiv(0x%x, 0x%x, %p);\n", light, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetMapdv)(GLenum target, GLenum query, GLdouble * v)
{
    (void) target; (void) query; (void) v;
   DISPATCH(GetMapdv, (target, query, v), (F, "glGetMapdv(0x%x, 0x%x, %p);\n", target, query, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(GetMapfv)(GLenum target, GLenum query, GLfloat * v)
{
    (void) target; (void) query; (void) v;
   DISPATCH(GetMapfv, (target, query, v), (F, "glGetMapfv(0x%x, 0x%x, %p);\n", target, query, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(GetMapiv)(GLenum target, GLenum query, GLint * v)
{
    (void) target; (void) query; (void) v;
   DISPATCH(GetMapiv, (target, query, v), (F, "glGetMapiv(0x%x, 0x%x, %p);\n", target, query, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(GetMaterialfv)(GLenum face, GLenum pname, GLfloat * params)
{
    (void) face; (void) pname; (void) params;
   DISPATCH(GetMaterialfv, (face, pname, params), (F, "glGetMaterialfv(0x%x, 0x%x, %p);\n", face, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetMaterialiv)(GLenum face, GLenum pname, GLint * params)
{
    (void) face; (void) pname; (void) params;
   DISPATCH(GetMaterialiv, (face, pname, params), (F, "glGetMaterialiv(0x%x, 0x%x, %p);\n", face, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetPixelMapfv)(GLenum map, GLfloat * values)
{
    (void) map; (void) values;
   DISPATCH(GetPixelMapfv, (map, values), (F, "glGetPixelMapfv(0x%x, %p);\n", map, (const void *) values));
}

KEYWORD1 void KEYWORD2 NAME(GetPixelMapuiv)(GLenum map, GLuint * values)
{
    (void) map; (void) values;
   DISPATCH(GetPixelMapuiv, (map, values), (F, "glGetPixelMapuiv(0x%x, %p);\n", map, (const void *) values));
}

KEYWORD1 void KEYWORD2 NAME(GetPixelMapusv)(GLenum map, GLushort * values)
{
    (void) map; (void) values;
   DISPATCH(GetPixelMapusv, (map, values), (F, "glGetPixelMapusv(0x%x, %p);\n", map, (const void *) values));
}

KEYWORD1 void KEYWORD2 NAME(GetPolygonStipple)(GLubyte * mask)
{
    (void) mask;
   DISPATCH(GetPolygonStipple, (mask), (F, "glGetPolygonStipple(%p);\n", (const void *) mask));
}

KEYWORD1 const GLubyte * KEYWORD2 NAME(GetString)(GLenum name)
{
    (void) name;
   RETURN_DISPATCH(GetString, (name), (F, "glGetString(0x%x);\n", name));
}

KEYWORD1 void KEYWORD2 NAME(GetTexEnvfv)(GLenum target, GLenum pname, GLfloat * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetTexEnvfv, (target, pname, params), (F, "glGetTexEnvfv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexEnviv)(GLenum target, GLenum pname, GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetTexEnviv, (target, pname, params), (F, "glGetTexEnviv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexGendv)(GLenum coord, GLenum pname, GLdouble * params)
{
    (void) coord; (void) pname; (void) params;
   DISPATCH(GetTexGendv, (coord, pname, params), (F, "glGetTexGendv(0x%x, 0x%x, %p);\n", coord, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexGenfv)(GLenum coord, GLenum pname, GLfloat * params)
{
    (void) coord; (void) pname; (void) params;
   DISPATCH(GetTexGenfv, (coord, pname, params), (F, "glGetTexGenfv(0x%x, 0x%x, %p);\n", coord, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexGeniv)(GLenum coord, GLenum pname, GLint * params)
{
    (void) coord; (void) pname; (void) params;
   DISPATCH(GetTexGeniv, (coord, pname, params), (F, "glGetTexGeniv(0x%x, 0x%x, %p);\n", coord, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexImage)(GLenum target, GLint level, GLenum format, GLenum type, GLvoid * pixels)
{
    (void) target; (void) level; (void) format; (void) type; (void) pixels;
   DISPATCH(GetTexImage, (target, level, format, type, pixels), (F, "glGetTexImage(0x%x, %d, 0x%x, 0x%x, %p);\n", target, level, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(GetTexParameterfv)(GLenum target, GLenum pname, GLfloat * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetTexParameterfv, (target, pname, params), (F, "glGetTexParameterfv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexParameteriv)(GLenum target, GLenum pname, GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetTexParameteriv, (target, pname, params), (F, "glGetTexParameteriv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexLevelParameterfv)(GLenum target, GLint level, GLenum pname, GLfloat * params)
{
    (void) target; (void) level; (void) pname; (void) params;
   DISPATCH(GetTexLevelParameterfv, (target, level, pname, params), (F, "glGetTexLevelParameterfv(0x%x, %d, 0x%x, %p);\n", target, level, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexLevelParameteriv)(GLenum target, GLint level, GLenum pname, GLint * params)
{
    (void) target; (void) level; (void) pname; (void) params;
   DISPATCH(GetTexLevelParameteriv, (target, level, pname, params), (F, "glGetTexLevelParameteriv(0x%x, %d, 0x%x, %p);\n", target, level, pname, (const void *) params));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsEnabled)(GLenum cap)
{
    (void) cap;
   RETURN_DISPATCH(IsEnabled, (cap), (F, "glIsEnabled(0x%x);\n", cap));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsList)(GLuint list)
{
    (void) list;
   RETURN_DISPATCH(IsList, (list), (F, "glIsList(%d);\n", list));
}

KEYWORD1 void KEYWORD2 NAME(DepthRange)(GLclampd zNear, GLclampd zFar)
{
    (void) zNear; (void) zFar;
   DISPATCH(DepthRange, (zNear, zFar), (F, "glDepthRange(%f, %f);\n", zNear, zFar));
}

KEYWORD1 void KEYWORD2 NAME(Frustum)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    (void) left; (void) right; (void) bottom; (void) top; (void) zNear; (void) zFar;
   DISPATCH(Frustum, (left, right, bottom, top, zNear, zFar), (F, "glFrustum(%f, %f, %f, %f, %f, %f);\n", left, right, bottom, top, zNear, zFar));
}

KEYWORD1 void KEYWORD2 NAME(LoadIdentity)(void)
{
   DISPATCH(LoadIdentity, (), (F, "glLoadIdentity();\n"));
}

KEYWORD1 void KEYWORD2 NAME(LoadMatrixf)(const GLfloat * m)
{
    (void) m;
   DISPATCH(LoadMatrixf, (m), (F, "glLoadMatrixf(%p);\n", (const void *) m));
}

KEYWORD1 void KEYWORD2 NAME(LoadMatrixd)(const GLdouble * m)
{
    (void) m;
   DISPATCH(LoadMatrixd, (m), (F, "glLoadMatrixd(%p);\n", (const void *) m));
}

KEYWORD1 void KEYWORD2 NAME(MatrixMode)(GLenum mode)
{
    (void) mode;
   DISPATCH(MatrixMode, (mode), (F, "glMatrixMode(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(MultMatrixf)(const GLfloat * m)
{
    (void) m;
   DISPATCH(MultMatrixf, (m), (F, "glMultMatrixf(%p);\n", (const void *) m));
}

KEYWORD1 void KEYWORD2 NAME(MultMatrixd)(const GLdouble * m)
{
    (void) m;
   DISPATCH(MultMatrixd, (m), (F, "glMultMatrixd(%p);\n", (const void *) m));
}

KEYWORD1 void KEYWORD2 NAME(Ortho)(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    (void) left; (void) right; (void) bottom; (void) top; (void) zNear; (void) zFar;
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
    (void) angle; (void) x; (void) y; (void) z;
   DISPATCH(Rotated, (angle, x, y, z), (F, "glRotated(%f, %f, %f, %f);\n", angle, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Rotatef)(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    (void) angle; (void) x; (void) y; (void) z;
   DISPATCH(Rotatef, (angle, x, y, z), (F, "glRotatef(%f, %f, %f, %f);\n", angle, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Scaled)(GLdouble x, GLdouble y, GLdouble z)
{
    (void) x; (void) y; (void) z;
   DISPATCH(Scaled, (x, y, z), (F, "glScaled(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Scalef)(GLfloat x, GLfloat y, GLfloat z)
{
    (void) x; (void) y; (void) z;
   DISPATCH(Scalef, (x, y, z), (F, "glScalef(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Translated)(GLdouble x, GLdouble y, GLdouble z)
{
    (void) x; (void) y; (void) z;
   DISPATCH(Translated, (x, y, z), (F, "glTranslated(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Translatef)(GLfloat x, GLfloat y, GLfloat z)
{
    (void) x; (void) y; (void) z;
   DISPATCH(Translatef, (x, y, z), (F, "glTranslatef(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Viewport)(GLint x, GLint y, GLsizei width, GLsizei height)
{
    (void) x; (void) y; (void) width; (void) height;
   DISPATCH(Viewport, (x, y, width, height), (F, "glViewport(%d, %d, %d, %d);\n", x, y, width, height));
}

KEYWORD1 void KEYWORD2 NAME(ArrayElement)(GLint i)
{
    (void) i;
   DISPATCH(ArrayElement, (i), (F, "glArrayElement(%d);\n", i));
}

KEYWORD1 void KEYWORD2 NAME(ArrayElementEXT)(GLint i)
{
    (void) i;
   DISPATCH(ArrayElement, (i), (F, "glArrayElementEXT(%d);\n", i));
}

KEYWORD1 void KEYWORD2 NAME(BindTexture)(GLenum target, GLuint texture)
{
    (void) target; (void) texture;
   DISPATCH(BindTexture, (target, texture), (F, "glBindTexture(0x%x, %d);\n", target, texture));
}

KEYWORD1 void KEYWORD2 NAME(BindTextureEXT)(GLenum target, GLuint texture)
{
    (void) target; (void) texture;
   DISPATCH(BindTexture, (target, texture), (F, "glBindTextureEXT(0x%x, %d);\n", target, texture));
}

KEYWORD1 void KEYWORD2 NAME(ColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
    (void) size; (void) type; (void) stride; (void) pointer;
   DISPATCH(ColorPointer, (size, type, stride, pointer), (F, "glColorPointer(%d, 0x%x, %d, %p);\n", size, type, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(DisableClientState)(GLenum array)
{
    (void) array;
   DISPATCH(DisableClientState, (array), (F, "glDisableClientState(0x%x);\n", array));
}

KEYWORD1 void KEYWORD2 NAME(DrawArrays)(GLenum mode, GLint first, GLsizei count)
{
    (void) mode; (void) first; (void) count;
   DISPATCH(DrawArrays, (mode, first, count), (F, "glDrawArrays(0x%x, %d, %d);\n", mode, first, count));
}

KEYWORD1 void KEYWORD2 NAME(DrawArraysEXT)(GLenum mode, GLint first, GLsizei count)
{
    (void) mode; (void) first; (void) count;
   DISPATCH(DrawArrays, (mode, first, count), (F, "glDrawArraysEXT(0x%x, %d, %d);\n", mode, first, count));
}

KEYWORD1 void KEYWORD2 NAME(DrawElements)(GLenum mode, GLsizei count, GLenum type, const GLvoid * indices)
{
    (void) mode; (void) count; (void) type; (void) indices;
   DISPATCH(DrawElements, (mode, count, type, indices), (F, "glDrawElements(0x%x, %d, 0x%x, %p);\n", mode, count, type, (const void *) indices));
}

KEYWORD1 void KEYWORD2 NAME(EdgeFlagPointer)(GLsizei stride, const GLvoid * pointer)
{
    (void) stride; (void) pointer;
   DISPATCH(EdgeFlagPointer, (stride, pointer), (F, "glEdgeFlagPointer(%d, %p);\n", stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(EnableClientState)(GLenum array)
{
    (void) array;
   DISPATCH(EnableClientState, (array), (F, "glEnableClientState(0x%x);\n", array));
}

KEYWORD1 void KEYWORD2 NAME(IndexPointer)(GLenum type, GLsizei stride, const GLvoid * pointer)
{
    (void) type; (void) stride; (void) pointer;
   DISPATCH(IndexPointer, (type, stride, pointer), (F, "glIndexPointer(0x%x, %d, %p);\n", type, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(Indexub)(GLubyte c)
{
    (void) c;
   DISPATCH(Indexub, (c), (F, "glIndexub(%d);\n", c));
}

KEYWORD1 void KEYWORD2 NAME(Indexubv)(const GLubyte * c)
{
    (void) c;
   DISPATCH(Indexubv, (c), (F, "glIndexubv(%p);\n", (const void *) c));
}

KEYWORD1 void KEYWORD2 NAME(InterleavedArrays)(GLenum format, GLsizei stride, const GLvoid * pointer)
{
    (void) format; (void) stride; (void) pointer;
   DISPATCH(InterleavedArrays, (format, stride, pointer), (F, "glInterleavedArrays(0x%x, %d, %p);\n", format, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(NormalPointer)(GLenum type, GLsizei stride, const GLvoid * pointer)
{
    (void) type; (void) stride; (void) pointer;
   DISPATCH(NormalPointer, (type, stride, pointer), (F, "glNormalPointer(0x%x, %d, %p);\n", type, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(PolygonOffset)(GLfloat factor, GLfloat units)
{
    (void) factor; (void) units;
   DISPATCH(PolygonOffset, (factor, units), (F, "glPolygonOffset(%f, %f);\n", factor, units));
}

KEYWORD1 void KEYWORD2 NAME(TexCoordPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
    (void) size; (void) type; (void) stride; (void) pointer;
   DISPATCH(TexCoordPointer, (size, type, stride, pointer), (F, "glTexCoordPointer(%d, 0x%x, %d, %p);\n", size, type, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(VertexPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
    (void) size; (void) type; (void) stride; (void) pointer;
   DISPATCH(VertexPointer, (size, type, stride, pointer), (F, "glVertexPointer(%d, 0x%x, %d, %p);\n", size, type, stride, (const void *) pointer));
}

KEYWORD1 GLboolean KEYWORD2 NAME(AreTexturesResident)(GLsizei n, const GLuint * textures, GLboolean * residences)
{
    (void) n; (void) textures; (void) residences;
   RETURN_DISPATCH(AreTexturesResident, (n, textures, residences), (F, "glAreTexturesResident(%d, %p, %p);\n", n, (const void *) textures, (const void *) residences));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexImage1D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
    (void) target; (void) level; (void) internalformat; (void) x; (void) y; (void) width; (void) border;
   DISPATCH(CopyTexImage1D, (target, level, internalformat, x, y, width, border), (F, "glCopyTexImage1D(0x%x, %d, 0x%x, %d, %d, %d, %d);\n", target, level, internalformat, x, y, width, border));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexImage1DEXT)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
    (void) target; (void) level; (void) internalformat; (void) x; (void) y; (void) width; (void) border;
   DISPATCH(CopyTexImage1D, (target, level, internalformat, x, y, width, border), (F, "glCopyTexImage1DEXT(0x%x, %d, 0x%x, %d, %d, %d, %d);\n", target, level, internalformat, x, y, width, border));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    (void) target; (void) level; (void) internalformat; (void) x; (void) y; (void) width; (void) height; (void) border;
   DISPATCH(CopyTexImage2D, (target, level, internalformat, x, y, width, height, border), (F, "glCopyTexImage2D(0x%x, %d, 0x%x, %d, %d, %d, %d, %d);\n", target, level, internalformat, x, y, width, height, border));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexImage2DEXT)(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    (void) target; (void) level; (void) internalformat; (void) x; (void) y; (void) width; (void) height; (void) border;
   DISPATCH(CopyTexImage2D, (target, level, internalformat, x, y, width, height, border), (F, "glCopyTexImage2DEXT(0x%x, %d, 0x%x, %d, %d, %d, %d, %d);\n", target, level, internalformat, x, y, width, height, border));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
    (void) target; (void) level; (void) xoffset; (void) x; (void) y; (void) width;
   DISPATCH(CopyTexSubImage1D, (target, level, xoffset, x, y, width), (F, "glCopyTexSubImage1D(0x%x, %d, %d, %d, %d, %d);\n", target, level, xoffset, x, y, width));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexSubImage1DEXT)(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
    (void) target; (void) level; (void) xoffset; (void) x; (void) y; (void) width;
   DISPATCH(CopyTexSubImage1D, (target, level, xoffset, x, y, width), (F, "glCopyTexSubImage1DEXT(0x%x, %d, %d, %d, %d, %d);\n", target, level, xoffset, x, y, width));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    (void) target; (void) level; (void) xoffset; (void) yoffset; (void) x; (void) y; (void) width; (void) height;
   DISPATCH(CopyTexSubImage2D, (target, level, xoffset, yoffset, x, y, width, height), (F, "glCopyTexSubImage2D(0x%x, %d, %d, %d, %d, %d, %d, %d);\n", target, level, xoffset, yoffset, x, y, width, height));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexSubImage2DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    (void) target; (void) level; (void) xoffset; (void) yoffset; (void) x; (void) y; (void) width; (void) height;
   DISPATCH(CopyTexSubImage2D, (target, level, xoffset, yoffset, x, y, width, height), (F, "glCopyTexSubImage2DEXT(0x%x, %d, %d, %d, %d, %d, %d, %d);\n", target, level, xoffset, yoffset, x, y, width, height));
}

KEYWORD1 void KEYWORD2 NAME(DeleteTextures)(GLsizei n, const GLuint * textures)
{
    (void) n; (void) textures;
   DISPATCH(DeleteTextures, (n, textures), (F, "glDeleteTextures(%d, %p);\n", n, (const void *) textures));
}

KEYWORD1 void KEYWORD2 NAME(GenTextures)(GLsizei n, GLuint * textures)
{
    (void) n; (void) textures;
   DISPATCH(GenTextures, (n, textures), (F, "glGenTextures(%d, %p);\n", n, (const void *) textures));
}

KEYWORD1 void KEYWORD2 NAME(GetPointerv)(GLenum pname, GLvoid ** params)
{
    (void) pname; (void) params;
   DISPATCH(GetPointerv, (pname, params), (F, "glGetPointerv(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetPointervEXT)(GLenum pname, GLvoid ** params)
{
    (void) pname; (void) params;
   DISPATCH(GetPointerv, (pname, params), (F, "glGetPointervEXT(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsTexture)(GLuint texture)
{
    (void) texture;
   RETURN_DISPATCH(IsTexture, (texture), (F, "glIsTexture(%d);\n", texture));
}

KEYWORD1 void KEYWORD2 NAME(PrioritizeTextures)(GLsizei n, const GLuint * textures, const GLclampf * priorities)
{
    (void) n; (void) textures; (void) priorities;
   DISPATCH(PrioritizeTextures, (n, textures, priorities), (F, "glPrioritizeTextures(%d, %p, %p);\n", n, (const void *) textures, (const void *) priorities));
}

KEYWORD1 void KEYWORD2 NAME(PrioritizeTexturesEXT)(GLsizei n, const GLuint * textures, const GLclampf * priorities)
{
    (void) n; (void) textures; (void) priorities;
   DISPATCH(PrioritizeTextures, (n, textures, priorities), (F, "glPrioritizeTexturesEXT(%d, %p, %p);\n", n, (const void *) textures, (const void *) priorities));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid * pixels)
{
    (void) target; (void) level; (void) xoffset; (void) width; (void) format; (void) type; (void) pixels;
   DISPATCH(TexSubImage1D, (target, level, xoffset, width, format, type, pixels), (F, "glTexSubImage1D(0x%x, %d, %d, %d, 0x%x, 0x%x, %p);\n", target, level, xoffset, width, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage1DEXT)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid * pixels)
{
    (void) target; (void) level; (void) xoffset; (void) width; (void) format; (void) type; (void) pixels;
   DISPATCH(TexSubImage1D, (target, level, xoffset, width, format, type, pixels), (F, "glTexSubImage1DEXT(0x%x, %d, %d, %d, 0x%x, 0x%x, %p);\n", target, level, xoffset, width, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels)
{
    (void) target; (void) level; (void) xoffset; (void) yoffset; (void) width; (void) height; (void) format; (void) type; (void) pixels;
   DISPATCH(TexSubImage2D, (target, level, xoffset, yoffset, width, height, format, type, pixels), (F, "glTexSubImage2D(0x%x, %d, %d, %d, %d, %d, 0x%x, 0x%x, %p);\n", target, level, xoffset, yoffset, width, height, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage2DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels)
{
    (void) target; (void) level; (void) xoffset; (void) yoffset; (void) width; (void) height; (void) format; (void) type; (void) pixels;
   DISPATCH(TexSubImage2D, (target, level, xoffset, yoffset, width, height, format, type, pixels), (F, "glTexSubImage2DEXT(0x%x, %d, %d, %d, %d, %d, 0x%x, 0x%x, %p);\n", target, level, xoffset, yoffset, width, height, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(PopClientAttrib)(void)
{
   DISPATCH(PopClientAttrib, (), (F, "glPopClientAttrib();\n"));
}

KEYWORD1 void KEYWORD2 NAME(PushClientAttrib)(GLbitfield mask)
{
    (void) mask;
   DISPATCH(PushClientAttrib, (mask), (F, "glPushClientAttrib(%d);\n", mask));
}

KEYWORD1 void KEYWORD2 NAME(BlendColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    (void) red; (void) green; (void) blue; (void) alpha;
   DISPATCH(BlendColor, (red, green, blue, alpha), (F, "glBlendColor(%f, %f, %f, %f);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(BlendColorEXT)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    (void) red; (void) green; (void) blue; (void) alpha;
   DISPATCH(BlendColor, (red, green, blue, alpha), (F, "glBlendColorEXT(%f, %f, %f, %f);\n", red, green, blue, alpha));
}

KEYWORD1 void KEYWORD2 NAME(BlendEquation)(GLenum mode)
{
    (void) mode;
   DISPATCH(BlendEquation, (mode), (F, "glBlendEquation(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(BlendEquationEXT)(GLenum mode)
{
    (void) mode;
   DISPATCH(BlendEquation, (mode), (F, "glBlendEquationEXT(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(DrawRangeElements)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid * indices)
{
    (void) mode; (void) start; (void) end; (void) count; (void) type; (void) indices;
   DISPATCH(DrawRangeElements, (mode, start, end, count, type, indices), (F, "glDrawRangeElements(0x%x, %d, %d, %d, 0x%x, %p);\n", mode, start, end, count, type, (const void *) indices));
}

KEYWORD1 void KEYWORD2 NAME(DrawRangeElementsEXT)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid * indices)
{
    (void) mode; (void) start; (void) end; (void) count; (void) type; (void) indices;
   DISPATCH(DrawRangeElements, (mode, start, end, count, type, indices), (F, "glDrawRangeElementsEXT(0x%x, %d, %d, %d, 0x%x, %p);\n", mode, start, end, count, type, (const void *) indices));
}

KEYWORD1 void KEYWORD2 NAME(ColorTable)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * table)
{
    (void) target; (void) internalformat; (void) width; (void) format; (void) type; (void) table;
   DISPATCH(ColorTable, (target, internalformat, width, format, type, table), (F, "glColorTable(0x%x, 0x%x, %d, 0x%x, 0x%x, %p);\n", target, internalformat, width, format, type, (const void *) table));
}

KEYWORD1 void KEYWORD2 NAME(ColorTableEXT)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * table)
{
    (void) target; (void) internalformat; (void) width; (void) format; (void) type; (void) table;
   DISPATCH(ColorTable, (target, internalformat, width, format, type, table), (F, "glColorTableEXT(0x%x, 0x%x, %d, 0x%x, 0x%x, %p);\n", target, internalformat, width, format, type, (const void *) table));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_339)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * table);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_339)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * table)
{
    (void) target; (void) internalformat; (void) width; (void) format; (void) type; (void) table;
   DISPATCH(ColorTable, (target, internalformat, width, format, type, table), (F, "glColorTableSGI(0x%x, 0x%x, %d, 0x%x, 0x%x, %p);\n", target, internalformat, width, format, type, (const void *) table));
}

KEYWORD1 void KEYWORD2 NAME(ColorTableParameterfv)(GLenum target, GLenum pname, const GLfloat * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(ColorTableParameterfv, (target, pname, params), (F, "glColorTableParameterfv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_340)(GLenum target, GLenum pname, const GLfloat * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_340)(GLenum target, GLenum pname, const GLfloat * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(ColorTableParameterfv, (target, pname, params), (F, "glColorTableParameterfvSGI(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ColorTableParameteriv)(GLenum target, GLenum pname, const GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(ColorTableParameteriv, (target, pname, params), (F, "glColorTableParameteriv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_341)(GLenum target, GLenum pname, const GLint * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_341)(GLenum target, GLenum pname, const GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(ColorTableParameteriv, (target, pname, params), (F, "glColorTableParameterivSGI(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(CopyColorTable)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
    (void) target; (void) internalformat; (void) x; (void) y; (void) width;
   DISPATCH(CopyColorTable, (target, internalformat, x, y, width), (F, "glCopyColorTable(0x%x, 0x%x, %d, %d, %d);\n", target, internalformat, x, y, width));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_342)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_342)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
    (void) target; (void) internalformat; (void) x; (void) y; (void) width;
   DISPATCH(CopyColorTable, (target, internalformat, x, y, width), (F, "glCopyColorTableSGI(0x%x, 0x%x, %d, %d, %d);\n", target, internalformat, x, y, width));
}

KEYWORD1 void KEYWORD2 NAME(GetColorTable)(GLenum target, GLenum format, GLenum type, GLvoid * table)
{
    (void) target; (void) format; (void) type; (void) table;
   DISPATCH(GetColorTable, (target, format, type, table), (F, "glGetColorTable(0x%x, 0x%x, 0x%x, %p);\n", target, format, type, (const void *) table));
}

KEYWORD1 void KEYWORD2 NAME(GetColorTableParameterfv)(GLenum target, GLenum pname, GLfloat * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetColorTableParameterfv, (target, pname, params), (F, "glGetColorTableParameterfv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetColorTableParameteriv)(GLenum target, GLenum pname, GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetColorTableParameteriv, (target, pname, params), (F, "glGetColorTableParameteriv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ColorSubTable)(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid * data)
{
    (void) target; (void) start; (void) count; (void) format; (void) type; (void) data;
   DISPATCH(ColorSubTable, (target, start, count, format, type, data), (F, "glColorSubTable(0x%x, %d, %d, 0x%x, 0x%x, %p);\n", target, start, count, format, type, (const void *) data));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_346)(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid * data);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_346)(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid * data)
{
    (void) target; (void) start; (void) count; (void) format; (void) type; (void) data;
   DISPATCH(ColorSubTable, (target, start, count, format, type, data), (F, "glColorSubTableEXT(0x%x, %d, %d, 0x%x, 0x%x, %p);\n", target, start, count, format, type, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(CopyColorSubTable)(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width)
{
    (void) target; (void) start; (void) x; (void) y; (void) width;
   DISPATCH(CopyColorSubTable, (target, start, x, y, width), (F, "glCopyColorSubTable(0x%x, %d, %d, %d, %d);\n", target, start, x, y, width));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_347)(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_347)(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width)
{
    (void) target; (void) start; (void) x; (void) y; (void) width;
   DISPATCH(CopyColorSubTable, (target, start, x, y, width), (F, "glCopyColorSubTableEXT(0x%x, %d, %d, %d, %d);\n", target, start, x, y, width));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionFilter1D)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * image)
{
    (void) target; (void) internalformat; (void) width; (void) format; (void) type; (void) image;
   DISPATCH(ConvolutionFilter1D, (target, internalformat, width, format, type, image), (F, "glConvolutionFilter1D(0x%x, 0x%x, %d, 0x%x, 0x%x, %p);\n", target, internalformat, width, format, type, (const void *) image));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_348)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * image);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_348)(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid * image)
{
    (void) target; (void) internalformat; (void) width; (void) format; (void) type; (void) image;
   DISPATCH(ConvolutionFilter1D, (target, internalformat, width, format, type, image), (F, "glConvolutionFilter1DEXT(0x%x, 0x%x, %d, 0x%x, 0x%x, %p);\n", target, internalformat, width, format, type, (const void *) image));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionFilter2D)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * image)
{
    (void) target; (void) internalformat; (void) width; (void) height; (void) format; (void) type; (void) image;
   DISPATCH(ConvolutionFilter2D, (target, internalformat, width, height, format, type, image), (F, "glConvolutionFilter2D(0x%x, 0x%x, %d, %d, 0x%x, 0x%x, %p);\n", target, internalformat, width, height, format, type, (const void *) image));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_349)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * image);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_349)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * image)
{
    (void) target; (void) internalformat; (void) width; (void) height; (void) format; (void) type; (void) image;
   DISPATCH(ConvolutionFilter2D, (target, internalformat, width, height, format, type, image), (F, "glConvolutionFilter2DEXT(0x%x, 0x%x, %d, %d, 0x%x, 0x%x, %p);\n", target, internalformat, width, height, format, type, (const void *) image));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionParameterf)(GLenum target, GLenum pname, GLfloat params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(ConvolutionParameterf, (target, pname, params), (F, "glConvolutionParameterf(0x%x, 0x%x, %f);\n", target, pname, params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_350)(GLenum target, GLenum pname, GLfloat params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_350)(GLenum target, GLenum pname, GLfloat params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(ConvolutionParameterf, (target, pname, params), (F, "glConvolutionParameterfEXT(0x%x, 0x%x, %f);\n", target, pname, params));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionParameterfv)(GLenum target, GLenum pname, const GLfloat * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(ConvolutionParameterfv, (target, pname, params), (F, "glConvolutionParameterfv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_351)(GLenum target, GLenum pname, const GLfloat * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_351)(GLenum target, GLenum pname, const GLfloat * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(ConvolutionParameterfv, (target, pname, params), (F, "glConvolutionParameterfvEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionParameteri)(GLenum target, GLenum pname, GLint params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(ConvolutionParameteri, (target, pname, params), (F, "glConvolutionParameteri(0x%x, 0x%x, %d);\n", target, pname, params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_352)(GLenum target, GLenum pname, GLint params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_352)(GLenum target, GLenum pname, GLint params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(ConvolutionParameteri, (target, pname, params), (F, "glConvolutionParameteriEXT(0x%x, 0x%x, %d);\n", target, pname, params));
}

KEYWORD1 void KEYWORD2 NAME(ConvolutionParameteriv)(GLenum target, GLenum pname, const GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(ConvolutionParameteriv, (target, pname, params), (F, "glConvolutionParameteriv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_353)(GLenum target, GLenum pname, const GLint * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_353)(GLenum target, GLenum pname, const GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(ConvolutionParameteriv, (target, pname, params), (F, "glConvolutionParameterivEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(CopyConvolutionFilter1D)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
    (void) target; (void) internalformat; (void) x; (void) y; (void) width;
   DISPATCH(CopyConvolutionFilter1D, (target, internalformat, x, y, width), (F, "glCopyConvolutionFilter1D(0x%x, 0x%x, %d, %d, %d);\n", target, internalformat, x, y, width));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_354)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_354)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
    (void) target; (void) internalformat; (void) x; (void) y; (void) width;
   DISPATCH(CopyConvolutionFilter1D, (target, internalformat, x, y, width), (F, "glCopyConvolutionFilter1DEXT(0x%x, 0x%x, %d, %d, %d);\n", target, internalformat, x, y, width));
}

KEYWORD1 void KEYWORD2 NAME(CopyConvolutionFilter2D)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height)
{
    (void) target; (void) internalformat; (void) x; (void) y; (void) width; (void) height;
   DISPATCH(CopyConvolutionFilter2D, (target, internalformat, x, y, width, height), (F, "glCopyConvolutionFilter2D(0x%x, 0x%x, %d, %d, %d, %d);\n", target, internalformat, x, y, width, height));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_355)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_355)(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height)
{
    (void) target; (void) internalformat; (void) x; (void) y; (void) width; (void) height;
   DISPATCH(CopyConvolutionFilter2D, (target, internalformat, x, y, width, height), (F, "glCopyConvolutionFilter2DEXT(0x%x, 0x%x, %d, %d, %d, %d);\n", target, internalformat, x, y, width, height));
}

KEYWORD1 void KEYWORD2 NAME(GetConvolutionFilter)(GLenum target, GLenum format, GLenum type, GLvoid * image)
{
    (void) target; (void) format; (void) type; (void) image;
   DISPATCH(GetConvolutionFilter, (target, format, type, image), (F, "glGetConvolutionFilter(0x%x, 0x%x, 0x%x, %p);\n", target, format, type, (const void *) image));
}

KEYWORD1 void KEYWORD2 NAME(GetConvolutionParameterfv)(GLenum target, GLenum pname, GLfloat * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetConvolutionParameterfv, (target, pname, params), (F, "glGetConvolutionParameterfv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetConvolutionParameteriv)(GLenum target, GLenum pname, GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetConvolutionParameteriv, (target, pname, params), (F, "glGetConvolutionParameteriv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetSeparableFilter)(GLenum target, GLenum format, GLenum type, GLvoid * row, GLvoid * column, GLvoid * span)
{
    (void) target; (void) format; (void) type; (void) row; (void) column; (void) span;
   DISPATCH(GetSeparableFilter, (target, format, type, row, column, span), (F, "glGetSeparableFilter(0x%x, 0x%x, 0x%x, %p, %p, %p);\n", target, format, type, (const void *) row, (const void *) column, (const void *) span));
}

KEYWORD1 void KEYWORD2 NAME(SeparableFilter2D)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * row, const GLvoid * column)
{
    (void) target; (void) internalformat; (void) width; (void) height; (void) format; (void) type; (void) row; (void) column;
   DISPATCH(SeparableFilter2D, (target, internalformat, width, height, format, type, row, column), (F, "glSeparableFilter2D(0x%x, 0x%x, %d, %d, 0x%x, 0x%x, %p, %p);\n", target, internalformat, width, height, format, type, (const void *) row, (const void *) column));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_360)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * row, const GLvoid * column);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_360)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * row, const GLvoid * column)
{
    (void) target; (void) internalformat; (void) width; (void) height; (void) format; (void) type; (void) row; (void) column;
   DISPATCH(SeparableFilter2D, (target, internalformat, width, height, format, type, row, column), (F, "glSeparableFilter2DEXT(0x%x, 0x%x, %d, %d, 0x%x, 0x%x, %p, %p);\n", target, internalformat, width, height, format, type, (const void *) row, (const void *) column));
}

KEYWORD1 void KEYWORD2 NAME(GetHistogram)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values)
{
    (void) target; (void) reset; (void) format; (void) type; (void) values;
   DISPATCH(GetHistogram, (target, reset, format, type, values), (F, "glGetHistogram(0x%x, %d, 0x%x, 0x%x, %p);\n", target, reset, format, type, (const void *) values));
}

KEYWORD1 void KEYWORD2 NAME(GetHistogramParameterfv)(GLenum target, GLenum pname, GLfloat * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetHistogramParameterfv, (target, pname, params), (F, "glGetHistogramParameterfv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetHistogramParameteriv)(GLenum target, GLenum pname, GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetHistogramParameteriv, (target, pname, params), (F, "glGetHistogramParameteriv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetMinmax)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values)
{
    (void) target; (void) reset; (void) format; (void) type; (void) values;
   DISPATCH(GetMinmax, (target, reset, format, type, values), (F, "glGetMinmax(0x%x, %d, 0x%x, 0x%x, %p);\n", target, reset, format, type, (const void *) values));
}

KEYWORD1 void KEYWORD2 NAME(GetMinmaxParameterfv)(GLenum target, GLenum pname, GLfloat * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetMinmaxParameterfv, (target, pname, params), (F, "glGetMinmaxParameterfv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetMinmaxParameteriv)(GLenum target, GLenum pname, GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetMinmaxParameteriv, (target, pname, params), (F, "glGetMinmaxParameteriv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(Histogram)(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink)
{
    (void) target; (void) width; (void) internalformat; (void) sink;
   DISPATCH(Histogram, (target, width, internalformat, sink), (F, "glHistogram(0x%x, %d, 0x%x, %d);\n", target, width, internalformat, sink));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_367)(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_367)(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink)
{
    (void) target; (void) width; (void) internalformat; (void) sink;
   DISPATCH(Histogram, (target, width, internalformat, sink), (F, "glHistogramEXT(0x%x, %d, 0x%x, %d);\n", target, width, internalformat, sink));
}

KEYWORD1 void KEYWORD2 NAME(Minmax)(GLenum target, GLenum internalformat, GLboolean sink)
{
    (void) target; (void) internalformat; (void) sink;
   DISPATCH(Minmax, (target, internalformat, sink), (F, "glMinmax(0x%x, 0x%x, %d);\n", target, internalformat, sink));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_368)(GLenum target, GLenum internalformat, GLboolean sink);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_368)(GLenum target, GLenum internalformat, GLboolean sink)
{
    (void) target; (void) internalformat; (void) sink;
   DISPATCH(Minmax, (target, internalformat, sink), (F, "glMinmaxEXT(0x%x, 0x%x, %d);\n", target, internalformat, sink));
}

KEYWORD1 void KEYWORD2 NAME(ResetHistogram)(GLenum target)
{
    (void) target;
   DISPATCH(ResetHistogram, (target), (F, "glResetHistogram(0x%x);\n", target));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_369)(GLenum target);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_369)(GLenum target)
{
    (void) target;
   DISPATCH(ResetHistogram, (target), (F, "glResetHistogramEXT(0x%x);\n", target));
}

KEYWORD1 void KEYWORD2 NAME(ResetMinmax)(GLenum target)
{
    (void) target;
   DISPATCH(ResetMinmax, (target), (F, "glResetMinmax(0x%x);\n", target));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_370)(GLenum target);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_370)(GLenum target)
{
    (void) target;
   DISPATCH(ResetMinmax, (target), (F, "glResetMinmaxEXT(0x%x);\n", target));
}

KEYWORD1 void KEYWORD2 NAME(TexImage3D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid * pixels)
{
    (void) target; (void) level; (void) internalformat; (void) width; (void) height; (void) depth; (void) border; (void) format; (void) type; (void) pixels;
   DISPATCH(TexImage3D, (target, level, internalformat, width, height, depth, border, format, type, pixels), (F, "glTexImage3D(0x%x, %d, %d, %d, %d, %d, %d, 0x%x, 0x%x, %p);\n", target, level, internalformat, width, height, depth, border, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(TexImage3DEXT)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid * pixels)
{
    (void) target; (void) level; (void) internalformat; (void) width; (void) height; (void) depth; (void) border; (void) format; (void) type; (void) pixels;
   DISPATCH(TexImage3D, (target, level, internalformat, width, height, depth, border, format, type, pixels), (F, "glTexImage3DEXT(0x%x, %d, %d, %d, %d, %d, %d, 0x%x, 0x%x, %p);\n", target, level, internalformat, width, height, depth, border, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid * pixels)
{
    (void) target; (void) level; (void) xoffset; (void) yoffset; (void) zoffset; (void) width; (void) height; (void) depth; (void) format; (void) type; (void) pixels;
   DISPATCH(TexSubImage3D, (target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels), (F, "glTexSubImage3D(0x%x, %d, %d, %d, %d, %d, %d, %d, 0x%x, 0x%x, %p);\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(TexSubImage3DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid * pixels)
{
    (void) target; (void) level; (void) xoffset; (void) yoffset; (void) zoffset; (void) width; (void) height; (void) depth; (void) format; (void) type; (void) pixels;
   DISPATCH(TexSubImage3D, (target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, pixels), (F, "glTexSubImage3DEXT(0x%x, %d, %d, %d, %d, %d, %d, %d, 0x%x, 0x%x, %p);\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, (const void *) pixels));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    (void) target; (void) level; (void) xoffset; (void) yoffset; (void) zoffset; (void) x; (void) y; (void) width; (void) height;
   DISPATCH(CopyTexSubImage3D, (target, level, xoffset, yoffset, zoffset, x, y, width, height), (F, "glCopyTexSubImage3D(0x%x, %d, %d, %d, %d, %d, %d, %d, %d);\n", target, level, xoffset, yoffset, zoffset, x, y, width, height));
}

KEYWORD1 void KEYWORD2 NAME(CopyTexSubImage3DEXT)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    (void) target; (void) level; (void) xoffset; (void) yoffset; (void) zoffset; (void) x; (void) y; (void) width; (void) height;
   DISPATCH(CopyTexSubImage3D, (target, level, xoffset, yoffset, zoffset, x, y, width, height), (F, "glCopyTexSubImage3DEXT(0x%x, %d, %d, %d, %d, %d, %d, %d, %d);\n", target, level, xoffset, yoffset, zoffset, x, y, width, height));
}

KEYWORD1 void KEYWORD2 NAME(ActiveTexture)(GLenum texture)
{
    (void) texture;
   DISPATCH(ActiveTextureARB, (texture), (F, "glActiveTexture(0x%x);\n", texture));
}

KEYWORD1 void KEYWORD2 NAME(ActiveTextureARB)(GLenum texture)
{
    (void) texture;
   DISPATCH(ActiveTextureARB, (texture), (F, "glActiveTextureARB(0x%x);\n", texture));
}

KEYWORD1 void KEYWORD2 NAME(ClientActiveTexture)(GLenum texture)
{
    (void) texture;
   DISPATCH(ClientActiveTextureARB, (texture), (F, "glClientActiveTexture(0x%x);\n", texture));
}

KEYWORD1 void KEYWORD2 NAME(ClientActiveTextureARB)(GLenum texture)
{
    (void) texture;
   DISPATCH(ClientActiveTextureARB, (texture), (F, "glClientActiveTextureARB(0x%x);\n", texture));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1d)(GLenum target, GLdouble s)
{
    (void) target; (void) s;
   DISPATCH(MultiTexCoord1dARB, (target, s), (F, "glMultiTexCoord1d(0x%x, %f);\n", target, s));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1dARB)(GLenum target, GLdouble s)
{
    (void) target; (void) s;
   DISPATCH(MultiTexCoord1dARB, (target, s), (F, "glMultiTexCoord1dARB(0x%x, %f);\n", target, s));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1dv)(GLenum target, const GLdouble * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord1dvARB, (target, v), (F, "glMultiTexCoord1dv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1dvARB)(GLenum target, const GLdouble * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord1dvARB, (target, v), (F, "glMultiTexCoord1dvARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1f)(GLenum target, GLfloat s)
{
    (void) target; (void) s;
   DISPATCH(MultiTexCoord1fARB, (target, s), (F, "glMultiTexCoord1f(0x%x, %f);\n", target, s));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1fARB)(GLenum target, GLfloat s)
{
    (void) target; (void) s;
   DISPATCH(MultiTexCoord1fARB, (target, s), (F, "glMultiTexCoord1fARB(0x%x, %f);\n", target, s));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1fv)(GLenum target, const GLfloat * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord1fvARB, (target, v), (F, "glMultiTexCoord1fv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1fvARB)(GLenum target, const GLfloat * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord1fvARB, (target, v), (F, "glMultiTexCoord1fvARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1i)(GLenum target, GLint s)
{
    (void) target; (void) s;
   DISPATCH(MultiTexCoord1iARB, (target, s), (F, "glMultiTexCoord1i(0x%x, %d);\n", target, s));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1iARB)(GLenum target, GLint s)
{
    (void) target; (void) s;
   DISPATCH(MultiTexCoord1iARB, (target, s), (F, "glMultiTexCoord1iARB(0x%x, %d);\n", target, s));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1iv)(GLenum target, const GLint * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord1ivARB, (target, v), (F, "glMultiTexCoord1iv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1ivARB)(GLenum target, const GLint * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord1ivARB, (target, v), (F, "glMultiTexCoord1ivARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1s)(GLenum target, GLshort s)
{
    (void) target; (void) s;
   DISPATCH(MultiTexCoord1sARB, (target, s), (F, "glMultiTexCoord1s(0x%x, %d);\n", target, s));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1sARB)(GLenum target, GLshort s)
{
    (void) target; (void) s;
   DISPATCH(MultiTexCoord1sARB, (target, s), (F, "glMultiTexCoord1sARB(0x%x, %d);\n", target, s));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1sv)(GLenum target, const GLshort * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord1svARB, (target, v), (F, "glMultiTexCoord1sv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord1svARB)(GLenum target, const GLshort * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord1svARB, (target, v), (F, "glMultiTexCoord1svARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2d)(GLenum target, GLdouble s, GLdouble t)
{
    (void) target; (void) s; (void) t;
   DISPATCH(MultiTexCoord2dARB, (target, s, t), (F, "glMultiTexCoord2d(0x%x, %f, %f);\n", target, s, t));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2dARB)(GLenum target, GLdouble s, GLdouble t)
{
    (void) target; (void) s; (void) t;
   DISPATCH(MultiTexCoord2dARB, (target, s, t), (F, "glMultiTexCoord2dARB(0x%x, %f, %f);\n", target, s, t));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2dv)(GLenum target, const GLdouble * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord2dvARB, (target, v), (F, "glMultiTexCoord2dv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2dvARB)(GLenum target, const GLdouble * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord2dvARB, (target, v), (F, "glMultiTexCoord2dvARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2f)(GLenum target, GLfloat s, GLfloat t)
{
    (void) target; (void) s; (void) t;
   DISPATCH(MultiTexCoord2fARB, (target, s, t), (F, "glMultiTexCoord2f(0x%x, %f, %f);\n", target, s, t));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2fARB)(GLenum target, GLfloat s, GLfloat t)
{
    (void) target; (void) s; (void) t;
   DISPATCH(MultiTexCoord2fARB, (target, s, t), (F, "glMultiTexCoord2fARB(0x%x, %f, %f);\n", target, s, t));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2fv)(GLenum target, const GLfloat * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord2fvARB, (target, v), (F, "glMultiTexCoord2fv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2fvARB)(GLenum target, const GLfloat * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord2fvARB, (target, v), (F, "glMultiTexCoord2fvARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2i)(GLenum target, GLint s, GLint t)
{
    (void) target; (void) s; (void) t;
   DISPATCH(MultiTexCoord2iARB, (target, s, t), (F, "glMultiTexCoord2i(0x%x, %d, %d);\n", target, s, t));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2iARB)(GLenum target, GLint s, GLint t)
{
    (void) target; (void) s; (void) t;
   DISPATCH(MultiTexCoord2iARB, (target, s, t), (F, "glMultiTexCoord2iARB(0x%x, %d, %d);\n", target, s, t));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2iv)(GLenum target, const GLint * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord2ivARB, (target, v), (F, "glMultiTexCoord2iv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2ivARB)(GLenum target, const GLint * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord2ivARB, (target, v), (F, "glMultiTexCoord2ivARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2s)(GLenum target, GLshort s, GLshort t)
{
    (void) target; (void) s; (void) t;
   DISPATCH(MultiTexCoord2sARB, (target, s, t), (F, "glMultiTexCoord2s(0x%x, %d, %d);\n", target, s, t));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2sARB)(GLenum target, GLshort s, GLshort t)
{
    (void) target; (void) s; (void) t;
   DISPATCH(MultiTexCoord2sARB, (target, s, t), (F, "glMultiTexCoord2sARB(0x%x, %d, %d);\n", target, s, t));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2sv)(GLenum target, const GLshort * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord2svARB, (target, v), (F, "glMultiTexCoord2sv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord2svARB)(GLenum target, const GLshort * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord2svARB, (target, v), (F, "glMultiTexCoord2svARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3d)(GLenum target, GLdouble s, GLdouble t, GLdouble r)
{
    (void) target; (void) s; (void) t; (void) r;
   DISPATCH(MultiTexCoord3dARB, (target, s, t, r), (F, "glMultiTexCoord3d(0x%x, %f, %f, %f);\n", target, s, t, r));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3dARB)(GLenum target, GLdouble s, GLdouble t, GLdouble r)
{
    (void) target; (void) s; (void) t; (void) r;
   DISPATCH(MultiTexCoord3dARB, (target, s, t, r), (F, "glMultiTexCoord3dARB(0x%x, %f, %f, %f);\n", target, s, t, r));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3dv)(GLenum target, const GLdouble * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord3dvARB, (target, v), (F, "glMultiTexCoord3dv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3dvARB)(GLenum target, const GLdouble * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord3dvARB, (target, v), (F, "glMultiTexCoord3dvARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3f)(GLenum target, GLfloat s, GLfloat t, GLfloat r)
{
    (void) target; (void) s; (void) t; (void) r;
   DISPATCH(MultiTexCoord3fARB, (target, s, t, r), (F, "glMultiTexCoord3f(0x%x, %f, %f, %f);\n", target, s, t, r));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3fARB)(GLenum target, GLfloat s, GLfloat t, GLfloat r)
{
    (void) target; (void) s; (void) t; (void) r;
   DISPATCH(MultiTexCoord3fARB, (target, s, t, r), (F, "glMultiTexCoord3fARB(0x%x, %f, %f, %f);\n", target, s, t, r));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3fv)(GLenum target, const GLfloat * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord3fvARB, (target, v), (F, "glMultiTexCoord3fv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3fvARB)(GLenum target, const GLfloat * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord3fvARB, (target, v), (F, "glMultiTexCoord3fvARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3i)(GLenum target, GLint s, GLint t, GLint r)
{
    (void) target; (void) s; (void) t; (void) r;
   DISPATCH(MultiTexCoord3iARB, (target, s, t, r), (F, "glMultiTexCoord3i(0x%x, %d, %d, %d);\n", target, s, t, r));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3iARB)(GLenum target, GLint s, GLint t, GLint r)
{
    (void) target; (void) s; (void) t; (void) r;
   DISPATCH(MultiTexCoord3iARB, (target, s, t, r), (F, "glMultiTexCoord3iARB(0x%x, %d, %d, %d);\n", target, s, t, r));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3iv)(GLenum target, const GLint * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord3ivARB, (target, v), (F, "glMultiTexCoord3iv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3ivARB)(GLenum target, const GLint * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord3ivARB, (target, v), (F, "glMultiTexCoord3ivARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3s)(GLenum target, GLshort s, GLshort t, GLshort r)
{
    (void) target; (void) s; (void) t; (void) r;
   DISPATCH(MultiTexCoord3sARB, (target, s, t, r), (F, "glMultiTexCoord3s(0x%x, %d, %d, %d);\n", target, s, t, r));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3sARB)(GLenum target, GLshort s, GLshort t, GLshort r)
{
    (void) target; (void) s; (void) t; (void) r;
   DISPATCH(MultiTexCoord3sARB, (target, s, t, r), (F, "glMultiTexCoord3sARB(0x%x, %d, %d, %d);\n", target, s, t, r));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3sv)(GLenum target, const GLshort * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord3svARB, (target, v), (F, "glMultiTexCoord3sv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord3svARB)(GLenum target, const GLshort * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord3svARB, (target, v), (F, "glMultiTexCoord3svARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4d)(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
    (void) target; (void) s; (void) t; (void) r; (void) q;
   DISPATCH(MultiTexCoord4dARB, (target, s, t, r, q), (F, "glMultiTexCoord4d(0x%x, %f, %f, %f, %f);\n", target, s, t, r, q));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4dARB)(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
    (void) target; (void) s; (void) t; (void) r; (void) q;
   DISPATCH(MultiTexCoord4dARB, (target, s, t, r, q), (F, "glMultiTexCoord4dARB(0x%x, %f, %f, %f, %f);\n", target, s, t, r, q));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4dv)(GLenum target, const GLdouble * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord4dvARB, (target, v), (F, "glMultiTexCoord4dv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4dvARB)(GLenum target, const GLdouble * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord4dvARB, (target, v), (F, "glMultiTexCoord4dvARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4f)(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    (void) target; (void) s; (void) t; (void) r; (void) q;
   DISPATCH(MultiTexCoord4fARB, (target, s, t, r, q), (F, "glMultiTexCoord4f(0x%x, %f, %f, %f, %f);\n", target, s, t, r, q));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4fARB)(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    (void) target; (void) s; (void) t; (void) r; (void) q;
   DISPATCH(MultiTexCoord4fARB, (target, s, t, r, q), (F, "glMultiTexCoord4fARB(0x%x, %f, %f, %f, %f);\n", target, s, t, r, q));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4fv)(GLenum target, const GLfloat * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord4fvARB, (target, v), (F, "glMultiTexCoord4fv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4fvARB)(GLenum target, const GLfloat * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord4fvARB, (target, v), (F, "glMultiTexCoord4fvARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4i)(GLenum target, GLint s, GLint t, GLint r, GLint q)
{
    (void) target; (void) s; (void) t; (void) r; (void) q;
   DISPATCH(MultiTexCoord4iARB, (target, s, t, r, q), (F, "glMultiTexCoord4i(0x%x, %d, %d, %d, %d);\n", target, s, t, r, q));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4iARB)(GLenum target, GLint s, GLint t, GLint r, GLint q)
{
    (void) target; (void) s; (void) t; (void) r; (void) q;
   DISPATCH(MultiTexCoord4iARB, (target, s, t, r, q), (F, "glMultiTexCoord4iARB(0x%x, %d, %d, %d, %d);\n", target, s, t, r, q));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4iv)(GLenum target, const GLint * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord4ivARB, (target, v), (F, "glMultiTexCoord4iv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4ivARB)(GLenum target, const GLint * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord4ivARB, (target, v), (F, "glMultiTexCoord4ivARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4s)(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q)
{
    (void) target; (void) s; (void) t; (void) r; (void) q;
   DISPATCH(MultiTexCoord4sARB, (target, s, t, r, q), (F, "glMultiTexCoord4s(0x%x, %d, %d, %d, %d);\n", target, s, t, r, q));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4sARB)(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q)
{
    (void) target; (void) s; (void) t; (void) r; (void) q;
   DISPATCH(MultiTexCoord4sARB, (target, s, t, r, q), (F, "glMultiTexCoord4sARB(0x%x, %d, %d, %d, %d);\n", target, s, t, r, q));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4sv)(GLenum target, const GLshort * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord4svARB, (target, v), (F, "glMultiTexCoord4sv(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoord4svARB)(GLenum target, const GLshort * v)
{
    (void) target; (void) v;
   DISPATCH(MultiTexCoord4svARB, (target, v), (F, "glMultiTexCoord4svARB(0x%x, %p);\n", target, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(AttachShader)(GLuint program, GLuint shader)
{
    (void) program; (void) shader;
   DISPATCH(AttachShader, (program, shader), (F, "glAttachShader(%d, %d);\n", program, shader));
}

KEYWORD1 GLuint KEYWORD2 NAME(CreateProgram)(void)
{
   RETURN_DISPATCH(CreateProgram, (), (F, "glCreateProgram();\n"));
}

KEYWORD1 GLuint KEYWORD2 NAME(CreateShader)(GLenum type)
{
    (void) type;
   RETURN_DISPATCH(CreateShader, (type), (F, "glCreateShader(0x%x);\n", type));
}

KEYWORD1 void KEYWORD2 NAME(DeleteProgram)(GLuint program)
{
    (void) program;
   DISPATCH(DeleteProgram, (program), (F, "glDeleteProgram(%d);\n", program));
}

KEYWORD1 void KEYWORD2 NAME(DeleteShader)(GLuint program)
{
    (void) program;
   DISPATCH(DeleteShader, (program), (F, "glDeleteShader(%d);\n", program));
}

KEYWORD1 void KEYWORD2 NAME(DetachShader)(GLuint program, GLuint shader)
{
    (void) program; (void) shader;
   DISPATCH(DetachShader, (program, shader), (F, "glDetachShader(%d, %d);\n", program, shader));
}

KEYWORD1 void KEYWORD2 NAME(GetAttachedShaders)(GLuint program, GLsizei maxCount, GLsizei * count, GLuint * obj)
{
    (void) program; (void) maxCount; (void) count; (void) obj;
   DISPATCH(GetAttachedShaders, (program, maxCount, count, obj), (F, "glGetAttachedShaders(%d, %d, %p, %p);\n", program, maxCount, (const void *) count, (const void *) obj));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramInfoLog)(GLuint program, GLsizei bufSize, GLsizei * length, GLchar * infoLog)
{
    (void) program; (void) bufSize; (void) length; (void) infoLog;
   DISPATCH(GetProgramInfoLog, (program, bufSize, length, infoLog), (F, "glGetProgramInfoLog(%d, %d, %p, %p);\n", program, bufSize, (const void *) length, (const void *) infoLog));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramiv)(GLuint program, GLenum pname, GLint * params)
{
    (void) program; (void) pname; (void) params;
   DISPATCH(GetProgramiv, (program, pname, params), (F, "glGetProgramiv(%d, 0x%x, %p);\n", program, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetShaderInfoLog)(GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * infoLog)
{
    (void) shader; (void) bufSize; (void) length; (void) infoLog;
   DISPATCH(GetShaderInfoLog, (shader, bufSize, length, infoLog), (F, "glGetShaderInfoLog(%d, %d, %p, %p);\n", shader, bufSize, (const void *) length, (const void *) infoLog));
}

KEYWORD1 void KEYWORD2 NAME(GetShaderiv)(GLuint shader, GLenum pname, GLint * params)
{
    (void) shader; (void) pname; (void) params;
   DISPATCH(GetShaderiv, (shader, pname, params), (F, "glGetShaderiv(%d, 0x%x, %p);\n", shader, pname, (const void *) params));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsProgram)(GLuint program)
{
    (void) program;
   RETURN_DISPATCH(IsProgram, (program), (F, "glIsProgram(%d);\n", program));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsShader)(GLuint shader)
{
    (void) shader;
   RETURN_DISPATCH(IsShader, (shader), (F, "glIsShader(%d);\n", shader));
}

KEYWORD1 void KEYWORD2 NAME(StencilFuncSeparate)(GLenum face, GLenum func, GLint ref, GLuint mask)
{
    (void) face; (void) func; (void) ref; (void) mask;
   DISPATCH(StencilFuncSeparate, (face, func, ref, mask), (F, "glStencilFuncSeparate(0x%x, 0x%x, %d, %d);\n", face, func, ref, mask));
}

KEYWORD1 void KEYWORD2 NAME(StencilMaskSeparate)(GLenum face, GLuint mask)
{
    (void) face; (void) mask;
   DISPATCH(StencilMaskSeparate, (face, mask), (F, "glStencilMaskSeparate(0x%x, %d);\n", face, mask));
}

KEYWORD1 void KEYWORD2 NAME(StencilOpSeparate)(GLenum face, GLenum sfail, GLenum zfail, GLenum zpass)
{
    (void) face; (void) sfail; (void) zfail; (void) zpass;
   DISPATCH(StencilOpSeparate, (face, sfail, zfail, zpass), (F, "glStencilOpSeparate(0x%x, 0x%x, 0x%x, 0x%x);\n", face, sfail, zfail, zpass));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_423)(GLenum face, GLenum sfail, GLenum zfail, GLenum zpass);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_423)(GLenum face, GLenum sfail, GLenum zfail, GLenum zpass)
{
    (void) face; (void) sfail; (void) zfail; (void) zpass;
   DISPATCH(StencilOpSeparate, (face, sfail, zfail, zpass), (F, "glStencilOpSeparateATI(0x%x, 0x%x, 0x%x, 0x%x);\n", face, sfail, zfail, zpass));
}

KEYWORD1 void KEYWORD2 NAME(UniformMatrix2x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    (void) location; (void) count; (void) transpose; (void) value;
   DISPATCH(UniformMatrix2x3fv, (location, count, transpose, value), (F, "glUniformMatrix2x3fv(%d, %d, %d, %p);\n", location, count, transpose, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(UniformMatrix2x4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    (void) location; (void) count; (void) transpose; (void) value;
   DISPATCH(UniformMatrix2x4fv, (location, count, transpose, value), (F, "glUniformMatrix2x4fv(%d, %d, %d, %p);\n", location, count, transpose, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(UniformMatrix3x2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    (void) location; (void) count; (void) transpose; (void) value;
   DISPATCH(UniformMatrix3x2fv, (location, count, transpose, value), (F, "glUniformMatrix3x2fv(%d, %d, %d, %p);\n", location, count, transpose, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(UniformMatrix3x4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    (void) location; (void) count; (void) transpose; (void) value;
   DISPATCH(UniformMatrix3x4fv, (location, count, transpose, value), (F, "glUniformMatrix3x4fv(%d, %d, %d, %p);\n", location, count, transpose, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(UniformMatrix4x2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    (void) location; (void) count; (void) transpose; (void) value;
   DISPATCH(UniformMatrix4x2fv, (location, count, transpose, value), (F, "glUniformMatrix4x2fv(%d, %d, %d, %p);\n", location, count, transpose, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(UniformMatrix4x3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    (void) location; (void) count; (void) transpose; (void) value;
   DISPATCH(UniformMatrix4x3fv, (location, count, transpose, value), (F, "glUniformMatrix4x3fv(%d, %d, %d, %p);\n", location, count, transpose, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(ClampColor)(GLenum target, GLenum clamp)
{
    (void) target; (void) clamp;
   DISPATCH(ClampColor, (target, clamp), (F, "glClampColor(0x%x, 0x%x);\n", target, clamp));
}

KEYWORD1 void KEYWORD2 NAME(ClearBufferfi)(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil)
{
    (void) buffer; (void) drawbuffer; (void) depth; (void) stencil;
   DISPATCH(ClearBufferfi, (buffer, drawbuffer, depth, stencil), (F, "glClearBufferfi(0x%x, %d, %f, %d);\n", buffer, drawbuffer, depth, stencil));
}

KEYWORD1 void KEYWORD2 NAME(ClearBufferfv)(GLenum buffer, GLint drawbuffer, const GLfloat * value)
{
    (void) buffer; (void) drawbuffer; (void) value;
   DISPATCH(ClearBufferfv, (buffer, drawbuffer, value), (F, "glClearBufferfv(0x%x, %d, %p);\n", buffer, drawbuffer, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(ClearBufferiv)(GLenum buffer, GLint drawbuffer, const GLint * value)
{
    (void) buffer; (void) drawbuffer; (void) value;
   DISPATCH(ClearBufferiv, (buffer, drawbuffer, value), (F, "glClearBufferiv(0x%x, %d, %p);\n", buffer, drawbuffer, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(ClearBufferuiv)(GLenum buffer, GLint drawbuffer, const GLuint * value)
{
    (void) buffer; (void) drawbuffer; (void) value;
   DISPATCH(ClearBufferuiv, (buffer, drawbuffer, value), (F, "glClearBufferuiv(0x%x, %d, %p);\n", buffer, drawbuffer, (const void *) value));
}

KEYWORD1 const GLubyte * KEYWORD2 NAME(GetStringi)(GLenum name, GLuint index)
{
    (void) name; (void) index;
   RETURN_DISPATCH(GetStringi, (name, index), (F, "glGetStringi(0x%x, %d);\n", name, index));
}

KEYWORD1 void KEYWORD2 NAME(TexBuffer)(GLenum target, GLenum internalFormat, GLuint buffer)
{
    (void) target; (void) internalFormat; (void) buffer;
   DISPATCH(TexBuffer, (target, internalFormat, buffer), (F, "glTexBuffer(0x%x, 0x%x, %d);\n", target, internalFormat, buffer));
}

KEYWORD1 void KEYWORD2 NAME(FramebufferTexture)(GLenum target, GLenum attachment, GLuint texture, GLint level)
{
    (void) target; (void) attachment; (void) texture; (void) level;
   DISPATCH(FramebufferTexture, (target, attachment, texture, level), (F, "glFramebufferTexture(0x%x, 0x%x, %d, %d);\n", target, attachment, texture, level));
}

KEYWORD1 void KEYWORD2 NAME(GetBufferParameteri64v)(GLenum target, GLenum pname, GLint64 * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetBufferParameteri64v, (target, pname, params), (F, "glGetBufferParameteri64v(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetInteger64i_v)(GLenum cap, GLuint index, GLint64 * data)
{
    (void) cap; (void) index; (void) data;
   DISPATCH(GetInteger64i_v, (cap, index, data), (F, "glGetInteger64i_v(0x%x, %d, %p);\n", cap, index, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribDivisor)(GLuint index, GLuint divisor)
{
    (void) index; (void) divisor;
   DISPATCH(VertexAttribDivisor, (index, divisor), (F, "glVertexAttribDivisor(%d, %d);\n", index, divisor));
}

KEYWORD1 void KEYWORD2 NAME(LoadTransposeMatrixd)(const GLdouble * m)
{
    (void) m;
   DISPATCH(LoadTransposeMatrixdARB, (m), (F, "glLoadTransposeMatrixd(%p);\n", (const void *) m));
}

KEYWORD1 void KEYWORD2 NAME(LoadTransposeMatrixdARB)(const GLdouble * m)
{
    (void) m;
   DISPATCH(LoadTransposeMatrixdARB, (m), (F, "glLoadTransposeMatrixdARB(%p);\n", (const void *) m));
}

KEYWORD1 void KEYWORD2 NAME(LoadTransposeMatrixf)(const GLfloat * m)
{
    (void) m;
   DISPATCH(LoadTransposeMatrixfARB, (m), (F, "glLoadTransposeMatrixf(%p);\n", (const void *) m));
}

KEYWORD1 void KEYWORD2 NAME(LoadTransposeMatrixfARB)(const GLfloat * m)
{
    (void) m;
   DISPATCH(LoadTransposeMatrixfARB, (m), (F, "glLoadTransposeMatrixfARB(%p);\n", (const void *) m));
}

KEYWORD1 void KEYWORD2 NAME(MultTransposeMatrixd)(const GLdouble * m)
{
    (void) m;
   DISPATCH(MultTransposeMatrixdARB, (m), (F, "glMultTransposeMatrixd(%p);\n", (const void *) m));
}

KEYWORD1 void KEYWORD2 NAME(MultTransposeMatrixdARB)(const GLdouble * m)
{
    (void) m;
   DISPATCH(MultTransposeMatrixdARB, (m), (F, "glMultTransposeMatrixdARB(%p);\n", (const void *) m));
}

KEYWORD1 void KEYWORD2 NAME(MultTransposeMatrixf)(const GLfloat * m)
{
    (void) m;
   DISPATCH(MultTransposeMatrixfARB, (m), (F, "glMultTransposeMatrixf(%p);\n", (const void *) m));
}

KEYWORD1 void KEYWORD2 NAME(MultTransposeMatrixfARB)(const GLfloat * m)
{
    (void) m;
   DISPATCH(MultTransposeMatrixfARB, (m), (F, "glMultTransposeMatrixfARB(%p);\n", (const void *) m));
}

KEYWORD1 void KEYWORD2 NAME(SampleCoverage)(GLclampf value, GLboolean invert)
{
    (void) value; (void) invert;
   DISPATCH(SampleCoverageARB, (value, invert), (F, "glSampleCoverage(%f, %d);\n", value, invert));
}

KEYWORD1 void KEYWORD2 NAME(SampleCoverageARB)(GLclampf value, GLboolean invert)
{
    (void) value; (void) invert;
   DISPATCH(SampleCoverageARB, (value, invert), (F, "glSampleCoverageARB(%f, %d);\n", value, invert));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexImage1D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid * data)
{
    (void) target; (void) level; (void) internalformat; (void) width; (void) border; (void) imageSize; (void) data;
   DISPATCH(CompressedTexImage1DARB, (target, level, internalformat, width, border, imageSize, data), (F, "glCompressedTexImage1D(0x%x, %d, 0x%x, %d, %d, %d, %p);\n", target, level, internalformat, width, border, imageSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexImage1DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid * data)
{
    (void) target; (void) level; (void) internalformat; (void) width; (void) border; (void) imageSize; (void) data;
   DISPATCH(CompressedTexImage1DARB, (target, level, internalformat, width, border, imageSize, data), (F, "glCompressedTexImage1DARB(0x%x, %d, 0x%x, %d, %d, %d, %p);\n", target, level, internalformat, width, border, imageSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexImage2D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid * data)
{
    (void) target; (void) level; (void) internalformat; (void) width; (void) height; (void) border; (void) imageSize; (void) data;
   DISPATCH(CompressedTexImage2DARB, (target, level, internalformat, width, height, border, imageSize, data), (F, "glCompressedTexImage2D(0x%x, %d, 0x%x, %d, %d, %d, %d, %p);\n", target, level, internalformat, width, height, border, imageSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexImage2DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid * data)
{
    (void) target; (void) level; (void) internalformat; (void) width; (void) height; (void) border; (void) imageSize; (void) data;
   DISPATCH(CompressedTexImage2DARB, (target, level, internalformat, width, height, border, imageSize, data), (F, "glCompressedTexImage2DARB(0x%x, %d, 0x%x, %d, %d, %d, %d, %p);\n", target, level, internalformat, width, height, border, imageSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexImage3D)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid * data)
{
    (void) target; (void) level; (void) internalformat; (void) width; (void) height; (void) depth; (void) border; (void) imageSize; (void) data;
   DISPATCH(CompressedTexImage3DARB, (target, level, internalformat, width, height, depth, border, imageSize, data), (F, "glCompressedTexImage3D(0x%x, %d, 0x%x, %d, %d, %d, %d, %d, %p);\n", target, level, internalformat, width, height, depth, border, imageSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexImage3DARB)(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid * data)
{
    (void) target; (void) level; (void) internalformat; (void) width; (void) height; (void) depth; (void) border; (void) imageSize; (void) data;
   DISPATCH(CompressedTexImage3DARB, (target, level, internalformat, width, height, depth, border, imageSize, data), (F, "glCompressedTexImage3DARB(0x%x, %d, 0x%x, %d, %d, %d, %d, %d, %p);\n", target, level, internalformat, width, height, depth, border, imageSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexSubImage1D)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid * data)
{
    (void) target; (void) level; (void) xoffset; (void) width; (void) format; (void) imageSize; (void) data;
   DISPATCH(CompressedTexSubImage1DARB, (target, level, xoffset, width, format, imageSize, data), (F, "glCompressedTexSubImage1D(0x%x, %d, %d, %d, 0x%x, %d, %p);\n", target, level, xoffset, width, format, imageSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexSubImage1DARB)(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid * data)
{
    (void) target; (void) level; (void) xoffset; (void) width; (void) format; (void) imageSize; (void) data;
   DISPATCH(CompressedTexSubImage1DARB, (target, level, xoffset, width, format, imageSize, data), (F, "glCompressedTexSubImage1DARB(0x%x, %d, %d, %d, 0x%x, %d, %p);\n", target, level, xoffset, width, format, imageSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexSubImage2D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid * data)
{
    (void) target; (void) level; (void) xoffset; (void) yoffset; (void) width; (void) height; (void) format; (void) imageSize; (void) data;
   DISPATCH(CompressedTexSubImage2DARB, (target, level, xoffset, yoffset, width, height, format, imageSize, data), (F, "glCompressedTexSubImage2D(0x%x, %d, %d, %d, %d, %d, 0x%x, %d, %p);\n", target, level, xoffset, yoffset, width, height, format, imageSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexSubImage2DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid * data)
{
    (void) target; (void) level; (void) xoffset; (void) yoffset; (void) width; (void) height; (void) format; (void) imageSize; (void) data;
   DISPATCH(CompressedTexSubImage2DARB, (target, level, xoffset, yoffset, width, height, format, imageSize, data), (F, "glCompressedTexSubImage2DARB(0x%x, %d, %d, %d, %d, %d, 0x%x, %d, %p);\n", target, level, xoffset, yoffset, width, height, format, imageSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexSubImage3D)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid * data)
{
    (void) target; (void) level; (void) xoffset; (void) yoffset; (void) zoffset; (void) width; (void) height; (void) depth; (void) format; (void) imageSize; (void) data;
   DISPATCH(CompressedTexSubImage3DARB, (target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data), (F, "glCompressedTexSubImage3D(0x%x, %d, %d, %d, %d, %d, %d, %d, 0x%x, %d, %p);\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(CompressedTexSubImage3DARB)(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid * data)
{
    (void) target; (void) level; (void) xoffset; (void) yoffset; (void) zoffset; (void) width; (void) height; (void) depth; (void) format; (void) imageSize; (void) data;
   DISPATCH(CompressedTexSubImage3DARB, (target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, data), (F, "glCompressedTexSubImage3DARB(0x%x, %d, %d, %d, %d, %d, %d, %d, 0x%x, %d, %p);\n", target, level, xoffset, yoffset, zoffset, width, height, depth, format, imageSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(GetCompressedTexImage)(GLenum target, GLint level, GLvoid * img)
{
    (void) target; (void) level; (void) img;
   DISPATCH(GetCompressedTexImageARB, (target, level, img), (F, "glGetCompressedTexImage(0x%x, %d, %p);\n", target, level, (const void *) img));
}

KEYWORD1 void KEYWORD2 NAME(GetCompressedTexImageARB)(GLenum target, GLint level, GLvoid * img)
{
    (void) target; (void) level; (void) img;
   DISPATCH(GetCompressedTexImageARB, (target, level, img), (F, "glGetCompressedTexImageARB(0x%x, %d, %p);\n", target, level, (const void *) img));
}

KEYWORD1 void KEYWORD2 NAME(DisableVertexAttribArray)(GLuint index)
{
    (void) index;
   DISPATCH(DisableVertexAttribArrayARB, (index), (F, "glDisableVertexAttribArray(%d);\n", index));
}

KEYWORD1 void KEYWORD2 NAME(DisableVertexAttribArrayARB)(GLuint index)
{
    (void) index;
   DISPATCH(DisableVertexAttribArrayARB, (index), (F, "glDisableVertexAttribArrayARB(%d);\n", index));
}

KEYWORD1 void KEYWORD2 NAME(EnableVertexAttribArray)(GLuint index)
{
    (void) index;
   DISPATCH(EnableVertexAttribArrayARB, (index), (F, "glEnableVertexAttribArray(%d);\n", index));
}

KEYWORD1 void KEYWORD2 NAME(EnableVertexAttribArrayARB)(GLuint index)
{
    (void) index;
   DISPATCH(EnableVertexAttribArrayARB, (index), (F, "glEnableVertexAttribArrayARB(%d);\n", index));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramEnvParameterdvARB)(GLenum target, GLuint index, GLdouble * params)
{
    (void) target; (void) index; (void) params;
   DISPATCH(GetProgramEnvParameterdvARB, (target, index, params), (F, "glGetProgramEnvParameterdvARB(0x%x, %d, %p);\n", target, index, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramEnvParameterfvARB)(GLenum target, GLuint index, GLfloat * params)
{
    (void) target; (void) index; (void) params;
   DISPATCH(GetProgramEnvParameterfvARB, (target, index, params), (F, "glGetProgramEnvParameterfvARB(0x%x, %d, %p);\n", target, index, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramLocalParameterdvARB)(GLenum target, GLuint index, GLdouble * params)
{
    (void) target; (void) index; (void) params;
   DISPATCH(GetProgramLocalParameterdvARB, (target, index, params), (F, "glGetProgramLocalParameterdvARB(0x%x, %d, %p);\n", target, index, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramLocalParameterfvARB)(GLenum target, GLuint index, GLfloat * params)
{
    (void) target; (void) index; (void) params;
   DISPATCH(GetProgramLocalParameterfvARB, (target, index, params), (F, "glGetProgramLocalParameterfvARB(0x%x, %d, %p);\n", target, index, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramStringARB)(GLenum target, GLenum pname, GLvoid * string)
{
    (void) target; (void) pname; (void) string;
   DISPATCH(GetProgramStringARB, (target, pname, string), (F, "glGetProgramStringARB(0x%x, 0x%x, %p);\n", target, pname, (const void *) string));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramivARB)(GLenum target, GLenum pname, GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetProgramivARB, (target, pname, params), (F, "glGetProgramivARB(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribdv)(GLuint index, GLenum pname, GLdouble * params)
{
    (void) index; (void) pname; (void) params;
   DISPATCH(GetVertexAttribdvARB, (index, pname, params), (F, "glGetVertexAttribdv(%d, 0x%x, %p);\n", index, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribdvARB)(GLuint index, GLenum pname, GLdouble * params)
{
    (void) index; (void) pname; (void) params;
   DISPATCH(GetVertexAttribdvARB, (index, pname, params), (F, "glGetVertexAttribdvARB(%d, 0x%x, %p);\n", index, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribfv)(GLuint index, GLenum pname, GLfloat * params)
{
    (void) index; (void) pname; (void) params;
   DISPATCH(GetVertexAttribfvARB, (index, pname, params), (F, "glGetVertexAttribfv(%d, 0x%x, %p);\n", index, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribfvARB)(GLuint index, GLenum pname, GLfloat * params)
{
    (void) index; (void) pname; (void) params;
   DISPATCH(GetVertexAttribfvARB, (index, pname, params), (F, "glGetVertexAttribfvARB(%d, 0x%x, %p);\n", index, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribiv)(GLuint index, GLenum pname, GLint * params)
{
    (void) index; (void) pname; (void) params;
   DISPATCH(GetVertexAttribivARB, (index, pname, params), (F, "glGetVertexAttribiv(%d, 0x%x, %p);\n", index, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribivARB)(GLuint index, GLenum pname, GLint * params)
{
    (void) index; (void) pname; (void) params;
   DISPATCH(GetVertexAttribivARB, (index, pname, params), (F, "glGetVertexAttribivARB(%d, 0x%x, %p);\n", index, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ProgramEnvParameter4dARB)(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    (void) target; (void) index; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(ProgramEnvParameter4dARB, (target, index, x, y, z, w), (F, "glProgramEnvParameter4dARB(0x%x, %d, %f, %f, %f, %f);\n", target, index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(ProgramParameter4dNV)(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    (void) target; (void) index; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(ProgramEnvParameter4dARB, (target, index, x, y, z, w), (F, "glProgramParameter4dNV(0x%x, %d, %f, %f, %f, %f);\n", target, index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(ProgramEnvParameter4dvARB)(GLenum target, GLuint index, const GLdouble * params)
{
    (void) target; (void) index; (void) params;
   DISPATCH(ProgramEnvParameter4dvARB, (target, index, params), (F, "glProgramEnvParameter4dvARB(0x%x, %d, %p);\n", target, index, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ProgramParameter4dvNV)(GLenum target, GLuint index, const GLdouble * params)
{
    (void) target; (void) index; (void) params;
   DISPATCH(ProgramEnvParameter4dvARB, (target, index, params), (F, "glProgramParameter4dvNV(0x%x, %d, %p);\n", target, index, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ProgramEnvParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    (void) target; (void) index; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(ProgramEnvParameter4fARB, (target, index, x, y, z, w), (F, "glProgramEnvParameter4fARB(0x%x, %d, %f, %f, %f, %f);\n", target, index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(ProgramParameter4fNV)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    (void) target; (void) index; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(ProgramEnvParameter4fARB, (target, index, x, y, z, w), (F, "glProgramParameter4fNV(0x%x, %d, %f, %f, %f, %f);\n", target, index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(ProgramEnvParameter4fvARB)(GLenum target, GLuint index, const GLfloat * params)
{
    (void) target; (void) index; (void) params;
   DISPATCH(ProgramEnvParameter4fvARB, (target, index, params), (F, "glProgramEnvParameter4fvARB(0x%x, %d, %p);\n", target, index, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ProgramParameter4fvNV)(GLenum target, GLuint index, const GLfloat * params)
{
    (void) target; (void) index; (void) params;
   DISPATCH(ProgramEnvParameter4fvARB, (target, index, params), (F, "glProgramParameter4fvNV(0x%x, %d, %p);\n", target, index, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ProgramLocalParameter4dARB)(GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    (void) target; (void) index; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(ProgramLocalParameter4dARB, (target, index, x, y, z, w), (F, "glProgramLocalParameter4dARB(0x%x, %d, %f, %f, %f, %f);\n", target, index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(ProgramLocalParameter4dvARB)(GLenum target, GLuint index, const GLdouble * params)
{
    (void) target; (void) index; (void) params;
   DISPATCH(ProgramLocalParameter4dvARB, (target, index, params), (F, "glProgramLocalParameter4dvARB(0x%x, %d, %p);\n", target, index, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ProgramLocalParameter4fARB)(GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    (void) target; (void) index; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(ProgramLocalParameter4fARB, (target, index, x, y, z, w), (F, "glProgramLocalParameter4fARB(0x%x, %d, %f, %f, %f, %f);\n", target, index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(ProgramLocalParameter4fvARB)(GLenum target, GLuint index, const GLfloat * params)
{
    (void) target; (void) index; (void) params;
   DISPATCH(ProgramLocalParameter4fvARB, (target, index, params), (F, "glProgramLocalParameter4fvARB(0x%x, %d, %p);\n", target, index, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ProgramStringARB)(GLenum target, GLenum format, GLsizei len, const GLvoid * string)
{
    (void) target; (void) format; (void) len; (void) string;
   DISPATCH(ProgramStringARB, (target, format, len, string), (F, "glProgramStringARB(0x%x, 0x%x, %d, %p);\n", target, format, len, (const void *) string));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1d)(GLuint index, GLdouble x)
{
    (void) index; (void) x;
   DISPATCH(VertexAttrib1dARB, (index, x), (F, "glVertexAttrib1d(%d, %f);\n", index, x));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1dARB)(GLuint index, GLdouble x)
{
    (void) index; (void) x;
   DISPATCH(VertexAttrib1dARB, (index, x), (F, "glVertexAttrib1dARB(%d, %f);\n", index, x));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1dv)(GLuint index, const GLdouble * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib1dvARB, (index, v), (F, "glVertexAttrib1dv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1dvARB)(GLuint index, const GLdouble * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib1dvARB, (index, v), (F, "glVertexAttrib1dvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1f)(GLuint index, GLfloat x)
{
    (void) index; (void) x;
   DISPATCH(VertexAttrib1fARB, (index, x), (F, "glVertexAttrib1f(%d, %f);\n", index, x));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1fARB)(GLuint index, GLfloat x)
{
    (void) index; (void) x;
   DISPATCH(VertexAttrib1fARB, (index, x), (F, "glVertexAttrib1fARB(%d, %f);\n", index, x));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1fv)(GLuint index, const GLfloat * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib1fvARB, (index, v), (F, "glVertexAttrib1fv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1fvARB)(GLuint index, const GLfloat * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib1fvARB, (index, v), (F, "glVertexAttrib1fvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1s)(GLuint index, GLshort x)
{
    (void) index; (void) x;
   DISPATCH(VertexAttrib1sARB, (index, x), (F, "glVertexAttrib1s(%d, %d);\n", index, x));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1sARB)(GLuint index, GLshort x)
{
    (void) index; (void) x;
   DISPATCH(VertexAttrib1sARB, (index, x), (F, "glVertexAttrib1sARB(%d, %d);\n", index, x));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1sv)(GLuint index, const GLshort * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib1svARB, (index, v), (F, "glVertexAttrib1sv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1svARB)(GLuint index, const GLshort * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib1svARB, (index, v), (F, "glVertexAttrib1svARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2d)(GLuint index, GLdouble x, GLdouble y)
{
    (void) index; (void) x; (void) y;
   DISPATCH(VertexAttrib2dARB, (index, x, y), (F, "glVertexAttrib2d(%d, %f, %f);\n", index, x, y));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2dARB)(GLuint index, GLdouble x, GLdouble y)
{
    (void) index; (void) x; (void) y;
   DISPATCH(VertexAttrib2dARB, (index, x, y), (F, "glVertexAttrib2dARB(%d, %f, %f);\n", index, x, y));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2dv)(GLuint index, const GLdouble * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib2dvARB, (index, v), (F, "glVertexAttrib2dv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2dvARB)(GLuint index, const GLdouble * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib2dvARB, (index, v), (F, "glVertexAttrib2dvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2f)(GLuint index, GLfloat x, GLfloat y)
{
    (void) index; (void) x; (void) y;
   DISPATCH(VertexAttrib2fARB, (index, x, y), (F, "glVertexAttrib2f(%d, %f, %f);\n", index, x, y));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2fARB)(GLuint index, GLfloat x, GLfloat y)
{
    (void) index; (void) x; (void) y;
   DISPATCH(VertexAttrib2fARB, (index, x, y), (F, "glVertexAttrib2fARB(%d, %f, %f);\n", index, x, y));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2fv)(GLuint index, const GLfloat * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib2fvARB, (index, v), (F, "glVertexAttrib2fv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2fvARB)(GLuint index, const GLfloat * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib2fvARB, (index, v), (F, "glVertexAttrib2fvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2s)(GLuint index, GLshort x, GLshort y)
{
    (void) index; (void) x; (void) y;
   DISPATCH(VertexAttrib2sARB, (index, x, y), (F, "glVertexAttrib2s(%d, %d, %d);\n", index, x, y));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2sARB)(GLuint index, GLshort x, GLshort y)
{
    (void) index; (void) x; (void) y;
   DISPATCH(VertexAttrib2sARB, (index, x, y), (F, "glVertexAttrib2sARB(%d, %d, %d);\n", index, x, y));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2sv)(GLuint index, const GLshort * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib2svARB, (index, v), (F, "glVertexAttrib2sv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2svARB)(GLuint index, const GLshort * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib2svARB, (index, v), (F, "glVertexAttrib2svARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3d)(GLuint index, GLdouble x, GLdouble y, GLdouble z)
{
    (void) index; (void) x; (void) y; (void) z;
   DISPATCH(VertexAttrib3dARB, (index, x, y, z), (F, "glVertexAttrib3d(%d, %f, %f, %f);\n", index, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3dARB)(GLuint index, GLdouble x, GLdouble y, GLdouble z)
{
    (void) index; (void) x; (void) y; (void) z;
   DISPATCH(VertexAttrib3dARB, (index, x, y, z), (F, "glVertexAttrib3dARB(%d, %f, %f, %f);\n", index, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3dv)(GLuint index, const GLdouble * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib3dvARB, (index, v), (F, "glVertexAttrib3dv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3dvARB)(GLuint index, const GLdouble * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib3dvARB, (index, v), (F, "glVertexAttrib3dvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3f)(GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
    (void) index; (void) x; (void) y; (void) z;
   DISPATCH(VertexAttrib3fARB, (index, x, y, z), (F, "glVertexAttrib3f(%d, %f, %f, %f);\n", index, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3fARB)(GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
    (void) index; (void) x; (void) y; (void) z;
   DISPATCH(VertexAttrib3fARB, (index, x, y, z), (F, "glVertexAttrib3fARB(%d, %f, %f, %f);\n", index, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3fv)(GLuint index, const GLfloat * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib3fvARB, (index, v), (F, "glVertexAttrib3fv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3fvARB)(GLuint index, const GLfloat * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib3fvARB, (index, v), (F, "glVertexAttrib3fvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3s)(GLuint index, GLshort x, GLshort y, GLshort z)
{
    (void) index; (void) x; (void) y; (void) z;
   DISPATCH(VertexAttrib3sARB, (index, x, y, z), (F, "glVertexAttrib3s(%d, %d, %d, %d);\n", index, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3sARB)(GLuint index, GLshort x, GLshort y, GLshort z)
{
    (void) index; (void) x; (void) y; (void) z;
   DISPATCH(VertexAttrib3sARB, (index, x, y, z), (F, "glVertexAttrib3sARB(%d, %d, %d, %d);\n", index, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3sv)(GLuint index, const GLshort * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib3svARB, (index, v), (F, "glVertexAttrib3sv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3svARB)(GLuint index, const GLshort * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib3svARB, (index, v), (F, "glVertexAttrib3svARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4Nbv)(GLuint index, const GLbyte * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4NbvARB, (index, v), (F, "glVertexAttrib4Nbv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4NbvARB)(GLuint index, const GLbyte * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4NbvARB, (index, v), (F, "glVertexAttrib4NbvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4Niv)(GLuint index, const GLint * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4NivARB, (index, v), (F, "glVertexAttrib4Niv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4NivARB)(GLuint index, const GLint * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4NivARB, (index, v), (F, "glVertexAttrib4NivARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4Nsv)(GLuint index, const GLshort * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4NsvARB, (index, v), (F, "glVertexAttrib4Nsv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4NsvARB)(GLuint index, const GLshort * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4NsvARB, (index, v), (F, "glVertexAttrib4NsvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4Nub)(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w)
{
    (void) index; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(VertexAttrib4NubARB, (index, x, y, z, w), (F, "glVertexAttrib4Nub(%d, %d, %d, %d, %d);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4NubARB)(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w)
{
    (void) index; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(VertexAttrib4NubARB, (index, x, y, z, w), (F, "glVertexAttrib4NubARB(%d, %d, %d, %d, %d);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4Nubv)(GLuint index, const GLubyte * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4NubvARB, (index, v), (F, "glVertexAttrib4Nubv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4NubvARB)(GLuint index, const GLubyte * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4NubvARB, (index, v), (F, "glVertexAttrib4NubvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4Nuiv)(GLuint index, const GLuint * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4NuivARB, (index, v), (F, "glVertexAttrib4Nuiv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4NuivARB)(GLuint index, const GLuint * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4NuivARB, (index, v), (F, "glVertexAttrib4NuivARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4Nusv)(GLuint index, const GLushort * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4NusvARB, (index, v), (F, "glVertexAttrib4Nusv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4NusvARB)(GLuint index, const GLushort * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4NusvARB, (index, v), (F, "glVertexAttrib4NusvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4bv)(GLuint index, const GLbyte * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4bvARB, (index, v), (F, "glVertexAttrib4bv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4bvARB)(GLuint index, const GLbyte * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4bvARB, (index, v), (F, "glVertexAttrib4bvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4d)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    (void) index; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(VertexAttrib4dARB, (index, x, y, z, w), (F, "glVertexAttrib4d(%d, %f, %f, %f, %f);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4dARB)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    (void) index; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(VertexAttrib4dARB, (index, x, y, z, w), (F, "glVertexAttrib4dARB(%d, %f, %f, %f, %f);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4dv)(GLuint index, const GLdouble * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4dvARB, (index, v), (F, "glVertexAttrib4dv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4dvARB)(GLuint index, const GLdouble * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4dvARB, (index, v), (F, "glVertexAttrib4dvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4f)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    (void) index; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(VertexAttrib4fARB, (index, x, y, z, w), (F, "glVertexAttrib4f(%d, %f, %f, %f, %f);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4fARB)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    (void) index; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(VertexAttrib4fARB, (index, x, y, z, w), (F, "glVertexAttrib4fARB(%d, %f, %f, %f, %f);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4fv)(GLuint index, const GLfloat * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4fvARB, (index, v), (F, "glVertexAttrib4fv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4fvARB)(GLuint index, const GLfloat * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4fvARB, (index, v), (F, "glVertexAttrib4fvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4iv)(GLuint index, const GLint * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4ivARB, (index, v), (F, "glVertexAttrib4iv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4ivARB)(GLuint index, const GLint * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4ivARB, (index, v), (F, "glVertexAttrib4ivARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4s)(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w)
{
    (void) index; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(VertexAttrib4sARB, (index, x, y, z, w), (F, "glVertexAttrib4s(%d, %d, %d, %d, %d);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4sARB)(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w)
{
    (void) index; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(VertexAttrib4sARB, (index, x, y, z, w), (F, "glVertexAttrib4sARB(%d, %d, %d, %d, %d);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4sv)(GLuint index, const GLshort * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4svARB, (index, v), (F, "glVertexAttrib4sv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4svARB)(GLuint index, const GLshort * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4svARB, (index, v), (F, "glVertexAttrib4svARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4ubv)(GLuint index, const GLubyte * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4ubvARB, (index, v), (F, "glVertexAttrib4ubv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4ubvARB)(GLuint index, const GLubyte * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4ubvARB, (index, v), (F, "glVertexAttrib4ubvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4uiv)(GLuint index, const GLuint * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4uivARB, (index, v), (F, "glVertexAttrib4uiv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4uivARB)(GLuint index, const GLuint * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4uivARB, (index, v), (F, "glVertexAttrib4uivARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4usv)(GLuint index, const GLushort * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4usvARB, (index, v), (F, "glVertexAttrib4usv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4usvARB)(GLuint index, const GLushort * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4usvARB, (index, v), (F, "glVertexAttrib4usvARB(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribPointer)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer)
{
    (void) index; (void) size; (void) type; (void) normalized; (void) stride; (void) pointer;
   DISPATCH(VertexAttribPointerARB, (index, size, type, normalized, stride, pointer), (F, "glVertexAttribPointer(%d, %d, 0x%x, %d, %d, %p);\n", index, size, type, normalized, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribPointerARB)(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid * pointer)
{
    (void) index; (void) size; (void) type; (void) normalized; (void) stride; (void) pointer;
   DISPATCH(VertexAttribPointerARB, (index, size, type, normalized, stride, pointer), (F, "glVertexAttribPointerARB(%d, %d, 0x%x, %d, %d, %p);\n", index, size, type, normalized, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(BindBuffer)(GLenum target, GLuint buffer)
{
    (void) target; (void) buffer;
   DISPATCH(BindBufferARB, (target, buffer), (F, "glBindBuffer(0x%x, %d);\n", target, buffer));
}

KEYWORD1 void KEYWORD2 NAME(BindBufferARB)(GLenum target, GLuint buffer)
{
    (void) target; (void) buffer;
   DISPATCH(BindBufferARB, (target, buffer), (F, "glBindBufferARB(0x%x, %d);\n", target, buffer));
}

KEYWORD1 void KEYWORD2 NAME(BufferData)(GLenum target, GLsizeiptr size, const GLvoid * data, GLenum usage)
{
    (void) target; (void) size; (void) data; (void) usage;
   DISPATCH(BufferDataARB, (target, size, data, usage), (F, "glBufferData(0x%x, %d, %p, 0x%x);\n", target, size, (const void *) data, usage));
}

KEYWORD1 void KEYWORD2 NAME(BufferDataARB)(GLenum target, GLsizeiptrARB size, const GLvoid * data, GLenum usage)
{
    (void) target; (void) size; (void) data; (void) usage;
   DISPATCH(BufferDataARB, (target, size, data, usage), (F, "glBufferDataARB(0x%x, %d, %p, 0x%x);\n", target, size, (const void *) data, usage));
}

KEYWORD1 void KEYWORD2 NAME(BufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid * data)
{
    (void) target; (void) offset; (void) size; (void) data;
   DISPATCH(BufferSubDataARB, (target, offset, size, data), (F, "glBufferSubData(0x%x, %d, %d, %p);\n", target, offset, size, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(BufferSubDataARB)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid * data)
{
    (void) target; (void) offset; (void) size; (void) data;
   DISPATCH(BufferSubDataARB, (target, offset, size, data), (F, "glBufferSubDataARB(0x%x, %d, %d, %p);\n", target, offset, size, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(DeleteBuffers)(GLsizei n, const GLuint * buffer)
{
    (void) n; (void) buffer;
   DISPATCH(DeleteBuffersARB, (n, buffer), (F, "glDeleteBuffers(%d, %p);\n", n, (const void *) buffer));
}

KEYWORD1 void KEYWORD2 NAME(DeleteBuffersARB)(GLsizei n, const GLuint * buffer)
{
    (void) n; (void) buffer;
   DISPATCH(DeleteBuffersARB, (n, buffer), (F, "glDeleteBuffersARB(%d, %p);\n", n, (const void *) buffer));
}

KEYWORD1 void KEYWORD2 NAME(GenBuffers)(GLsizei n, GLuint * buffer)
{
    (void) n; (void) buffer;
   DISPATCH(GenBuffersARB, (n, buffer), (F, "glGenBuffers(%d, %p);\n", n, (const void *) buffer));
}

KEYWORD1 void KEYWORD2 NAME(GenBuffersARB)(GLsizei n, GLuint * buffer)
{
    (void) n; (void) buffer;
   DISPATCH(GenBuffersARB, (n, buffer), (F, "glGenBuffersARB(%d, %p);\n", n, (const void *) buffer));
}

KEYWORD1 void KEYWORD2 NAME(GetBufferParameteriv)(GLenum target, GLenum pname, GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetBufferParameterivARB, (target, pname, params), (F, "glGetBufferParameteriv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetBufferParameterivARB)(GLenum target, GLenum pname, GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetBufferParameterivARB, (target, pname, params), (F, "glGetBufferParameterivARB(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetBufferPointerv)(GLenum target, GLenum pname, GLvoid ** params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetBufferPointervARB, (target, pname, params), (F, "glGetBufferPointerv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetBufferPointervARB)(GLenum target, GLenum pname, GLvoid ** params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetBufferPointervARB, (target, pname, params), (F, "glGetBufferPointervARB(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetBufferSubData)(GLenum target, GLintptr offset, GLsizeiptr size, GLvoid * data)
{
    (void) target; (void) offset; (void) size; (void) data;
   DISPATCH(GetBufferSubDataARB, (target, offset, size, data), (F, "glGetBufferSubData(0x%x, %d, %d, %p);\n", target, offset, size, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(GetBufferSubDataARB)(GLenum target, GLintptrARB offset, GLsizeiptrARB size, GLvoid * data)
{
    (void) target; (void) offset; (void) size; (void) data;
   DISPATCH(GetBufferSubDataARB, (target, offset, size, data), (F, "glGetBufferSubDataARB(0x%x, %d, %d, %p);\n", target, offset, size, (const void *) data));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsBuffer)(GLuint buffer)
{
    (void) buffer;
   RETURN_DISPATCH(IsBufferARB, (buffer), (F, "glIsBuffer(%d);\n", buffer));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsBufferARB)(GLuint buffer)
{
    (void) buffer;
   RETURN_DISPATCH(IsBufferARB, (buffer), (F, "glIsBufferARB(%d);\n", buffer));
}

KEYWORD1 GLvoid * KEYWORD2 NAME(MapBuffer)(GLenum target, GLenum access)
{
    (void) target; (void) access;
   RETURN_DISPATCH(MapBufferARB, (target, access), (F, "glMapBuffer(0x%x, 0x%x);\n", target, access));
}

KEYWORD1 GLvoid * KEYWORD2 NAME(MapBufferARB)(GLenum target, GLenum access)
{
    (void) target; (void) access;
   RETURN_DISPATCH(MapBufferARB, (target, access), (F, "glMapBufferARB(0x%x, 0x%x);\n", target, access));
}

KEYWORD1 GLboolean KEYWORD2 NAME(UnmapBuffer)(GLenum target)
{
    (void) target;
   RETURN_DISPATCH(UnmapBufferARB, (target), (F, "glUnmapBuffer(0x%x);\n", target));
}

KEYWORD1 GLboolean KEYWORD2 NAME(UnmapBufferARB)(GLenum target)
{
    (void) target;
   RETURN_DISPATCH(UnmapBufferARB, (target), (F, "glUnmapBufferARB(0x%x);\n", target));
}

KEYWORD1 void KEYWORD2 NAME(BeginQuery)(GLenum target, GLuint id)
{
    (void) target; (void) id;
   DISPATCH(BeginQueryARB, (target, id), (F, "glBeginQuery(0x%x, %d);\n", target, id));
}

KEYWORD1 void KEYWORD2 NAME(BeginQueryARB)(GLenum target, GLuint id)
{
    (void) target; (void) id;
   DISPATCH(BeginQueryARB, (target, id), (F, "glBeginQueryARB(0x%x, %d);\n", target, id));
}

KEYWORD1 void KEYWORD2 NAME(DeleteQueries)(GLsizei n, const GLuint * ids)
{
    (void) n; (void) ids;
   DISPATCH(DeleteQueriesARB, (n, ids), (F, "glDeleteQueries(%d, %p);\n", n, (const void *) ids));
}

KEYWORD1 void KEYWORD2 NAME(DeleteQueriesARB)(GLsizei n, const GLuint * ids)
{
    (void) n; (void) ids;
   DISPATCH(DeleteQueriesARB, (n, ids), (F, "glDeleteQueriesARB(%d, %p);\n", n, (const void *) ids));
}

KEYWORD1 void KEYWORD2 NAME(EndQuery)(GLenum target)
{
    (void) target;
   DISPATCH(EndQueryARB, (target), (F, "glEndQuery(0x%x);\n", target));
}

KEYWORD1 void KEYWORD2 NAME(EndQueryARB)(GLenum target)
{
    (void) target;
   DISPATCH(EndQueryARB, (target), (F, "glEndQueryARB(0x%x);\n", target));
}

KEYWORD1 void KEYWORD2 NAME(GenQueries)(GLsizei n, GLuint * ids)
{
    (void) n; (void) ids;
   DISPATCH(GenQueriesARB, (n, ids), (F, "glGenQueries(%d, %p);\n", n, (const void *) ids));
}

KEYWORD1 void KEYWORD2 NAME(GenQueriesARB)(GLsizei n, GLuint * ids)
{
    (void) n; (void) ids;
   DISPATCH(GenQueriesARB, (n, ids), (F, "glGenQueriesARB(%d, %p);\n", n, (const void *) ids));
}

KEYWORD1 void KEYWORD2 NAME(GetQueryObjectiv)(GLuint id, GLenum pname, GLint * params)
{
    (void) id; (void) pname; (void) params;
   DISPATCH(GetQueryObjectivARB, (id, pname, params), (F, "glGetQueryObjectiv(%d, 0x%x, %p);\n", id, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetQueryObjectivARB)(GLuint id, GLenum pname, GLint * params)
{
    (void) id; (void) pname; (void) params;
   DISPATCH(GetQueryObjectivARB, (id, pname, params), (F, "glGetQueryObjectivARB(%d, 0x%x, %p);\n", id, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetQueryObjectuiv)(GLuint id, GLenum pname, GLuint * params)
{
    (void) id; (void) pname; (void) params;
   DISPATCH(GetQueryObjectuivARB, (id, pname, params), (F, "glGetQueryObjectuiv(%d, 0x%x, %p);\n", id, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetQueryObjectuivARB)(GLuint id, GLenum pname, GLuint * params)
{
    (void) id; (void) pname; (void) params;
   DISPATCH(GetQueryObjectuivARB, (id, pname, params), (F, "glGetQueryObjectuivARB(%d, 0x%x, %p);\n", id, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetQueryiv)(GLenum target, GLenum pname, GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetQueryivARB, (target, pname, params), (F, "glGetQueryiv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetQueryivARB)(GLenum target, GLenum pname, GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetQueryivARB, (target, pname, params), (F, "glGetQueryivARB(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsQuery)(GLuint id)
{
    (void) id;
   RETURN_DISPATCH(IsQueryARB, (id), (F, "glIsQuery(%d);\n", id));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsQueryARB)(GLuint id)
{
    (void) id;
   RETURN_DISPATCH(IsQueryARB, (id), (F, "glIsQueryARB(%d);\n", id));
}

KEYWORD1 void KEYWORD2 NAME(AttachObjectARB)(GLhandleARB containerObj, GLhandleARB obj)
{
    (void) containerObj; (void) obj;
   DISPATCH(AttachObjectARB, (containerObj, obj), (F, "glAttachObjectARB(%d, %d);\n", containerObj, obj));
}

KEYWORD1 void KEYWORD2 NAME(CompileShader)(GLuint shader)
{
    (void) shader;
   DISPATCH(CompileShaderARB, (shader), (F, "glCompileShader(%d);\n", shader));
}

KEYWORD1 void KEYWORD2 NAME(CompileShaderARB)(GLhandleARB shader)
{
    (void) shader;
   DISPATCH(CompileShaderARB, (shader), (F, "glCompileShaderARB(%d);\n", shader));
}

KEYWORD1 GLhandleARB KEYWORD2 NAME(CreateProgramObjectARB)(void)
{
   RETURN_DISPATCH(CreateProgramObjectARB, (), (F, "glCreateProgramObjectARB();\n"));
}

KEYWORD1 GLhandleARB KEYWORD2 NAME(CreateShaderObjectARB)(GLenum shaderType)
{
    (void) shaderType;
   RETURN_DISPATCH(CreateShaderObjectARB, (shaderType), (F, "glCreateShaderObjectARB(0x%x);\n", shaderType));
}

KEYWORD1 void KEYWORD2 NAME(DeleteObjectARB)(GLhandleARB obj)
{
    (void) obj;
   DISPATCH(DeleteObjectARB, (obj), (F, "glDeleteObjectARB(%d);\n", obj));
}

KEYWORD1 void KEYWORD2 NAME(DetachObjectARB)(GLhandleARB containerObj, GLhandleARB attachedObj)
{
    (void) containerObj; (void) attachedObj;
   DISPATCH(DetachObjectARB, (containerObj, attachedObj), (F, "glDetachObjectARB(%d, %d);\n", containerObj, attachedObj));
}

KEYWORD1 void KEYWORD2 NAME(GetActiveUniform)(GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLint * size, GLenum * type, GLchar * name)
{
    (void) program; (void) index; (void) bufSize; (void) length; (void) size; (void) type; (void) name;
   DISPATCH(GetActiveUniformARB, (program, index, bufSize, length, size, type, name), (F, "glGetActiveUniform(%d, %d, %d, %p, %p, %p, %p);\n", program, index, bufSize, (const void *) length, (const void *) size, (const void *) type, (const void *) name));
}

KEYWORD1 void KEYWORD2 NAME(GetActiveUniformARB)(GLhandleARB program, GLuint index, GLsizei bufSize, GLsizei * length, GLint * size, GLenum * type, GLcharARB * name)
{
    (void) program; (void) index; (void) bufSize; (void) length; (void) size; (void) type; (void) name;
   DISPATCH(GetActiveUniformARB, (program, index, bufSize, length, size, type, name), (F, "glGetActiveUniformARB(%d, %d, %d, %p, %p, %p, %p);\n", program, index, bufSize, (const void *) length, (const void *) size, (const void *) type, (const void *) name));
}

KEYWORD1 void KEYWORD2 NAME(GetAttachedObjectsARB)(GLhandleARB containerObj, GLsizei maxLength, GLsizei * length, GLhandleARB * infoLog)
{
    (void) containerObj; (void) maxLength; (void) length; (void) infoLog;
   DISPATCH(GetAttachedObjectsARB, (containerObj, maxLength, length, infoLog), (F, "glGetAttachedObjectsARB(%d, %d, %p, %p);\n", containerObj, maxLength, (const void *) length, (const void *) infoLog));
}

KEYWORD1 GLhandleARB KEYWORD2 NAME(GetHandleARB)(GLenum pname)
{
    (void) pname;
   RETURN_DISPATCH(GetHandleARB, (pname), (F, "glGetHandleARB(0x%x);\n", pname));
}

KEYWORD1 void KEYWORD2 NAME(GetInfoLogARB)(GLhandleARB obj, GLsizei maxLength, GLsizei * length, GLcharARB * infoLog)
{
    (void) obj; (void) maxLength; (void) length; (void) infoLog;
   DISPATCH(GetInfoLogARB, (obj, maxLength, length, infoLog), (F, "glGetInfoLogARB(%d, %d, %p, %p);\n", obj, maxLength, (const void *) length, (const void *) infoLog));
}

KEYWORD1 void KEYWORD2 NAME(GetObjectParameterfvARB)(GLhandleARB obj, GLenum pname, GLfloat * params)
{
    (void) obj; (void) pname; (void) params;
   DISPATCH(GetObjectParameterfvARB, (obj, pname, params), (F, "glGetObjectParameterfvARB(%d, 0x%x, %p);\n", obj, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetObjectParameterivARB)(GLhandleARB obj, GLenum pname, GLint * params)
{
    (void) obj; (void) pname; (void) params;
   DISPATCH(GetObjectParameterivARB, (obj, pname, params), (F, "glGetObjectParameterivARB(%d, 0x%x, %p);\n", obj, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetShaderSource)(GLuint shader, GLsizei bufSize, GLsizei * length, GLchar * source)
{
    (void) shader; (void) bufSize; (void) length; (void) source;
   DISPATCH(GetShaderSourceARB, (shader, bufSize, length, source), (F, "glGetShaderSource(%d, %d, %p, %p);\n", shader, bufSize, (const void *) length, (const void *) source));
}

KEYWORD1 void KEYWORD2 NAME(GetShaderSourceARB)(GLhandleARB shader, GLsizei bufSize, GLsizei * length, GLcharARB * source)
{
    (void) shader; (void) bufSize; (void) length; (void) source;
   DISPATCH(GetShaderSourceARB, (shader, bufSize, length, source), (F, "glGetShaderSourceARB(%d, %d, %p, %p);\n", shader, bufSize, (const void *) length, (const void *) source));
}

KEYWORD1 GLint KEYWORD2 NAME(GetUniformLocation)(GLuint program, const GLchar * name)
{
    (void) program; (void) name;
   RETURN_DISPATCH(GetUniformLocationARB, (program, name), (F, "glGetUniformLocation(%d, %p);\n", program, (const void *) name));
}

KEYWORD1 GLint KEYWORD2 NAME(GetUniformLocationARB)(GLhandleARB program, const GLcharARB * name)
{
    (void) program; (void) name;
   RETURN_DISPATCH(GetUniformLocationARB, (program, name), (F, "glGetUniformLocationARB(%d, %p);\n", program, (const void *) name));
}

KEYWORD1 void KEYWORD2 NAME(GetUniformfv)(GLuint program, GLint location, GLfloat * params)
{
    (void) program; (void) location; (void) params;
   DISPATCH(GetUniformfvARB, (program, location, params), (F, "glGetUniformfv(%d, %d, %p);\n", program, location, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetUniformfvARB)(GLhandleARB program, GLint location, GLfloat * params)
{
    (void) program; (void) location; (void) params;
   DISPATCH(GetUniformfvARB, (program, location, params), (F, "glGetUniformfvARB(%d, %d, %p);\n", program, location, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetUniformiv)(GLuint program, GLint location, GLint * params)
{
    (void) program; (void) location; (void) params;
   DISPATCH(GetUniformivARB, (program, location, params), (F, "glGetUniformiv(%d, %d, %p);\n", program, location, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetUniformivARB)(GLhandleARB program, GLint location, GLint * params)
{
    (void) program; (void) location; (void) params;
   DISPATCH(GetUniformivARB, (program, location, params), (F, "glGetUniformivARB(%d, %d, %p);\n", program, location, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(LinkProgram)(GLuint program)
{
    (void) program;
   DISPATCH(LinkProgramARB, (program), (F, "glLinkProgram(%d);\n", program));
}

KEYWORD1 void KEYWORD2 NAME(LinkProgramARB)(GLhandleARB program)
{
    (void) program;
   DISPATCH(LinkProgramARB, (program), (F, "glLinkProgramARB(%d);\n", program));
}

KEYWORD1 void KEYWORD2 NAME(ShaderSource)(GLuint shader, GLsizei count, const GLchar ** string, const GLint * length)
{
    (void) shader; (void) count; (void) string; (void) length;
   DISPATCH(ShaderSourceARB, (shader, count, string, length), (F, "glShaderSource(%d, %d, %p, %p);\n", shader, count, (const void *) string, (const void *) length));
}

KEYWORD1 void KEYWORD2 NAME(ShaderSourceARB)(GLhandleARB shader, GLsizei count, const GLcharARB ** string, const GLint * length)
{
    (void) shader; (void) count; (void) string; (void) length;
   DISPATCH(ShaderSourceARB, (shader, count, string, length), (F, "glShaderSourceARB(%d, %d, %p, %p);\n", shader, count, (const void *) string, (const void *) length));
}

KEYWORD1 void KEYWORD2 NAME(Uniform1f)(GLint location, GLfloat v0)
{
    (void) location; (void) v0;
   DISPATCH(Uniform1fARB, (location, v0), (F, "glUniform1f(%d, %f);\n", location, v0));
}

KEYWORD1 void KEYWORD2 NAME(Uniform1fARB)(GLint location, GLfloat v0)
{
    (void) location; (void) v0;
   DISPATCH(Uniform1fARB, (location, v0), (F, "glUniform1fARB(%d, %f);\n", location, v0));
}

KEYWORD1 void KEYWORD2 NAME(Uniform1fv)(GLint location, GLsizei count, const GLfloat * value)
{
    (void) location; (void) count; (void) value;
   DISPATCH(Uniform1fvARB, (location, count, value), (F, "glUniform1fv(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform1fvARB)(GLint location, GLsizei count, const GLfloat * value)
{
    (void) location; (void) count; (void) value;
   DISPATCH(Uniform1fvARB, (location, count, value), (F, "glUniform1fvARB(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform1i)(GLint location, GLint v0)
{
    (void) location; (void) v0;
   DISPATCH(Uniform1iARB, (location, v0), (F, "glUniform1i(%d, %d);\n", location, v0));
}

KEYWORD1 void KEYWORD2 NAME(Uniform1iARB)(GLint location, GLint v0)
{
    (void) location; (void) v0;
   DISPATCH(Uniform1iARB, (location, v0), (F, "glUniform1iARB(%d, %d);\n", location, v0));
}

KEYWORD1 void KEYWORD2 NAME(Uniform1iv)(GLint location, GLsizei count, const GLint * value)
{
    (void) location; (void) count; (void) value;
   DISPATCH(Uniform1ivARB, (location, count, value), (F, "glUniform1iv(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform1ivARB)(GLint location, GLsizei count, const GLint * value)
{
    (void) location; (void) count; (void) value;
   DISPATCH(Uniform1ivARB, (location, count, value), (F, "glUniform1ivARB(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform2f)(GLint location, GLfloat v0, GLfloat v1)
{
    (void) location; (void) v0; (void) v1;
   DISPATCH(Uniform2fARB, (location, v0, v1), (F, "glUniform2f(%d, %f, %f);\n", location, v0, v1));
}

KEYWORD1 void KEYWORD2 NAME(Uniform2fARB)(GLint location, GLfloat v0, GLfloat v1)
{
    (void) location; (void) v0; (void) v1;
   DISPATCH(Uniform2fARB, (location, v0, v1), (F, "glUniform2fARB(%d, %f, %f);\n", location, v0, v1));
}

KEYWORD1 void KEYWORD2 NAME(Uniform2fv)(GLint location, GLsizei count, const GLfloat * value)
{
    (void) location; (void) count; (void) value;
   DISPATCH(Uniform2fvARB, (location, count, value), (F, "glUniform2fv(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform2fvARB)(GLint location, GLsizei count, const GLfloat * value)
{
    (void) location; (void) count; (void) value;
   DISPATCH(Uniform2fvARB, (location, count, value), (F, "glUniform2fvARB(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform2i)(GLint location, GLint v0, GLint v1)
{
    (void) location; (void) v0; (void) v1;
   DISPATCH(Uniform2iARB, (location, v0, v1), (F, "glUniform2i(%d, %d, %d);\n", location, v0, v1));
}

KEYWORD1 void KEYWORD2 NAME(Uniform2iARB)(GLint location, GLint v0, GLint v1)
{
    (void) location; (void) v0; (void) v1;
   DISPATCH(Uniform2iARB, (location, v0, v1), (F, "glUniform2iARB(%d, %d, %d);\n", location, v0, v1));
}

KEYWORD1 void KEYWORD2 NAME(Uniform2iv)(GLint location, GLsizei count, const GLint * value)
{
    (void) location; (void) count; (void) value;
   DISPATCH(Uniform2ivARB, (location, count, value), (F, "glUniform2iv(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform2ivARB)(GLint location, GLsizei count, const GLint * value)
{
    (void) location; (void) count; (void) value;
   DISPATCH(Uniform2ivARB, (location, count, value), (F, "glUniform2ivARB(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
    (void) location; (void) v0; (void) v1; (void) v2;
   DISPATCH(Uniform3fARB, (location, v0, v1, v2), (F, "glUniform3f(%d, %f, %f, %f);\n", location, v0, v1, v2));
}

KEYWORD1 void KEYWORD2 NAME(Uniform3fARB)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
    (void) location; (void) v0; (void) v1; (void) v2;
   DISPATCH(Uniform3fARB, (location, v0, v1, v2), (F, "glUniform3fARB(%d, %f, %f, %f);\n", location, v0, v1, v2));
}

KEYWORD1 void KEYWORD2 NAME(Uniform3fv)(GLint location, GLsizei count, const GLfloat * value)
{
    (void) location; (void) count; (void) value;
   DISPATCH(Uniform3fvARB, (location, count, value), (F, "glUniform3fv(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform3fvARB)(GLint location, GLsizei count, const GLfloat * value)
{
    (void) location; (void) count; (void) value;
   DISPATCH(Uniform3fvARB, (location, count, value), (F, "glUniform3fvARB(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform3i)(GLint location, GLint v0, GLint v1, GLint v2)
{
    (void) location; (void) v0; (void) v1; (void) v2;
   DISPATCH(Uniform3iARB, (location, v0, v1, v2), (F, "glUniform3i(%d, %d, %d, %d);\n", location, v0, v1, v2));
}

KEYWORD1 void KEYWORD2 NAME(Uniform3iARB)(GLint location, GLint v0, GLint v1, GLint v2)
{
    (void) location; (void) v0; (void) v1; (void) v2;
   DISPATCH(Uniform3iARB, (location, v0, v1, v2), (F, "glUniform3iARB(%d, %d, %d, %d);\n", location, v0, v1, v2));
}

KEYWORD1 void KEYWORD2 NAME(Uniform3iv)(GLint location, GLsizei count, const GLint * value)
{
    (void) location; (void) count; (void) value;
   DISPATCH(Uniform3ivARB, (location, count, value), (F, "glUniform3iv(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform3ivARB)(GLint location, GLsizei count, const GLint * value)
{
    (void) location; (void) count; (void) value;
   DISPATCH(Uniform3ivARB, (location, count, value), (F, "glUniform3ivARB(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
    (void) location; (void) v0; (void) v1; (void) v2; (void) v3;
   DISPATCH(Uniform4fARB, (location, v0, v1, v2, v3), (F, "glUniform4f(%d, %f, %f, %f, %f);\n", location, v0, v1, v2, v3));
}

KEYWORD1 void KEYWORD2 NAME(Uniform4fARB)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
    (void) location; (void) v0; (void) v1; (void) v2; (void) v3;
   DISPATCH(Uniform4fARB, (location, v0, v1, v2, v3), (F, "glUniform4fARB(%d, %f, %f, %f, %f);\n", location, v0, v1, v2, v3));
}

KEYWORD1 void KEYWORD2 NAME(Uniform4fv)(GLint location, GLsizei count, const GLfloat * value)
{
    (void) location; (void) count; (void) value;
   DISPATCH(Uniform4fvARB, (location, count, value), (F, "glUniform4fv(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform4fvARB)(GLint location, GLsizei count, const GLfloat * value)
{
    (void) location; (void) count; (void) value;
   DISPATCH(Uniform4fvARB, (location, count, value), (F, "glUniform4fvARB(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform4i)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
{
    (void) location; (void) v0; (void) v1; (void) v2; (void) v3;
   DISPATCH(Uniform4iARB, (location, v0, v1, v2, v3), (F, "glUniform4i(%d, %d, %d, %d, %d);\n", location, v0, v1, v2, v3));
}

KEYWORD1 void KEYWORD2 NAME(Uniform4iARB)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
{
    (void) location; (void) v0; (void) v1; (void) v2; (void) v3;
   DISPATCH(Uniform4iARB, (location, v0, v1, v2, v3), (F, "glUniform4iARB(%d, %d, %d, %d, %d);\n", location, v0, v1, v2, v3));
}

KEYWORD1 void KEYWORD2 NAME(Uniform4iv)(GLint location, GLsizei count, const GLint * value)
{
    (void) location; (void) count; (void) value;
   DISPATCH(Uniform4ivARB, (location, count, value), (F, "glUniform4iv(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform4ivARB)(GLint location, GLsizei count, const GLint * value)
{
    (void) location; (void) count; (void) value;
   DISPATCH(Uniform4ivARB, (location, count, value), (F, "glUniform4ivARB(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(UniformMatrix2fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    (void) location; (void) count; (void) transpose; (void) value;
   DISPATCH(UniformMatrix2fvARB, (location, count, transpose, value), (F, "glUniformMatrix2fv(%d, %d, %d, %p);\n", location, count, transpose, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(UniformMatrix2fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    (void) location; (void) count; (void) transpose; (void) value;
   DISPATCH(UniformMatrix2fvARB, (location, count, transpose, value), (F, "glUniformMatrix2fvARB(%d, %d, %d, %p);\n", location, count, transpose, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(UniformMatrix3fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    (void) location; (void) count; (void) transpose; (void) value;
   DISPATCH(UniformMatrix3fvARB, (location, count, transpose, value), (F, "glUniformMatrix3fv(%d, %d, %d, %p);\n", location, count, transpose, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(UniformMatrix3fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    (void) location; (void) count; (void) transpose; (void) value;
   DISPATCH(UniformMatrix3fvARB, (location, count, transpose, value), (F, "glUniformMatrix3fvARB(%d, %d, %d, %p);\n", location, count, transpose, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(UniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    (void) location; (void) count; (void) transpose; (void) value;
   DISPATCH(UniformMatrix4fvARB, (location, count, transpose, value), (F, "glUniformMatrix4fv(%d, %d, %d, %p);\n", location, count, transpose, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(UniformMatrix4fvARB)(GLint location, GLsizei count, GLboolean transpose, const GLfloat * value)
{
    (void) location; (void) count; (void) transpose; (void) value;
   DISPATCH(UniformMatrix4fvARB, (location, count, transpose, value), (F, "glUniformMatrix4fvARB(%d, %d, %d, %p);\n", location, count, transpose, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(UseProgram)(GLuint program)
{
    (void) program;
   DISPATCH(UseProgramObjectARB, (program), (F, "glUseProgram(%d);\n", program));
}

KEYWORD1 void KEYWORD2 NAME(UseProgramObjectARB)(GLhandleARB program)
{
    (void) program;
   DISPATCH(UseProgramObjectARB, (program), (F, "glUseProgramObjectARB(%d);\n", program));
}

KEYWORD1 void KEYWORD2 NAME(ValidateProgram)(GLuint program)
{
    (void) program;
   DISPATCH(ValidateProgramARB, (program), (F, "glValidateProgram(%d);\n", program));
}

KEYWORD1 void KEYWORD2 NAME(ValidateProgramARB)(GLhandleARB program)
{
    (void) program;
   DISPATCH(ValidateProgramARB, (program), (F, "glValidateProgramARB(%d);\n", program));
}

KEYWORD1 void KEYWORD2 NAME(BindAttribLocation)(GLuint program, GLuint index, const GLchar * name)
{
    (void) program; (void) index; (void) name;
   DISPATCH(BindAttribLocationARB, (program, index, name), (F, "glBindAttribLocation(%d, %d, %p);\n", program, index, (const void *) name));
}

KEYWORD1 void KEYWORD2 NAME(BindAttribLocationARB)(GLhandleARB program, GLuint index, const GLcharARB * name)
{
    (void) program; (void) index; (void) name;
   DISPATCH(BindAttribLocationARB, (program, index, name), (F, "glBindAttribLocationARB(%d, %d, %p);\n", program, index, (const void *) name));
}

KEYWORD1 void KEYWORD2 NAME(GetActiveAttrib)(GLuint program, GLuint index, GLsizei  bufSize, GLsizei * length, GLint * size, GLenum * type, GLchar * name)
{
    (void) program; (void) index; (void) bufSize; (void) length; (void) size; (void) type; (void) name;
   DISPATCH(GetActiveAttribARB, (program, index, bufSize, length, size, type, name), (F, "glGetActiveAttrib(%d, %d, %d, %p, %p, %p, %p);\n", program, index, bufSize, (const void *) length, (const void *) size, (const void *) type, (const void *) name));
}

KEYWORD1 void KEYWORD2 NAME(GetActiveAttribARB)(GLhandleARB program, GLuint index, GLsizei bufSize, GLsizei * length, GLint * size, GLenum * type, GLcharARB * name)
{
    (void) program; (void) index; (void) bufSize; (void) length; (void) size; (void) type; (void) name;
   DISPATCH(GetActiveAttribARB, (program, index, bufSize, length, size, type, name), (F, "glGetActiveAttribARB(%d, %d, %d, %p, %p, %p, %p);\n", program, index, bufSize, (const void *) length, (const void *) size, (const void *) type, (const void *) name));
}

KEYWORD1 GLint KEYWORD2 NAME(GetAttribLocation)(GLuint program, const GLchar * name)
{
    (void) program; (void) name;
   RETURN_DISPATCH(GetAttribLocationARB, (program, name), (F, "glGetAttribLocation(%d, %p);\n", program, (const void *) name));
}

KEYWORD1 GLint KEYWORD2 NAME(GetAttribLocationARB)(GLhandleARB program, const GLcharARB * name)
{
    (void) program; (void) name;
   RETURN_DISPATCH(GetAttribLocationARB, (program, name), (F, "glGetAttribLocationARB(%d, %p);\n", program, (const void *) name));
}

KEYWORD1 void KEYWORD2 NAME(DrawBuffers)(GLsizei n, const GLenum * bufs)
{
    (void) n; (void) bufs;
   DISPATCH(DrawBuffersARB, (n, bufs), (F, "glDrawBuffers(%d, %p);\n", n, (const void *) bufs));
}

KEYWORD1 void KEYWORD2 NAME(DrawBuffersARB)(GLsizei n, const GLenum * bufs)
{
    (void) n; (void) bufs;
   DISPATCH(DrawBuffersARB, (n, bufs), (F, "glDrawBuffersARB(%d, %p);\n", n, (const void *) bufs));
}

KEYWORD1 void KEYWORD2 NAME(DrawBuffersATI)(GLsizei n, const GLenum * bufs)
{
    (void) n; (void) bufs;
   DISPATCH(DrawBuffersARB, (n, bufs), (F, "glDrawBuffersATI(%d, %p);\n", n, (const void *) bufs));
}

KEYWORD1 void KEYWORD2 NAME(DrawBuffersNV)(GLsizei n, const GLenum * bufs)
{
    (void) n; (void) bufs;
   DISPATCH(DrawBuffersARB, (n, bufs), (F, "glDrawBuffersNV(%d, %p);\n", n, (const void *) bufs));
}

KEYWORD1 void KEYWORD2 NAME(ClampColorARB)(GLenum target, GLenum clamp)
{
    (void) target; (void) clamp;
   DISPATCH(ClampColorARB, (target, clamp), (F, "glClampColorARB(0x%x, 0x%x);\n", target, clamp));
}

KEYWORD1 void KEYWORD2 NAME(DrawArraysInstancedARB)(GLenum mode, GLint first, GLsizei count, GLsizei primcount)
{
    (void) mode; (void) first; (void) count; (void) primcount;
   DISPATCH(DrawArraysInstancedARB, (mode, first, count, primcount), (F, "glDrawArraysInstancedARB(0x%x, %d, %d, %d);\n", mode, first, count, primcount));
}

KEYWORD1 void KEYWORD2 NAME(DrawArraysInstancedEXT)(GLenum mode, GLint first, GLsizei count, GLsizei primcount)
{
    (void) mode; (void) first; (void) count; (void) primcount;
   DISPATCH(DrawArraysInstancedARB, (mode, first, count, primcount), (F, "glDrawArraysInstancedEXT(0x%x, %d, %d, %d);\n", mode, first, count, primcount));
}

KEYWORD1 void KEYWORD2 NAME(DrawArraysInstanced)(GLenum mode, GLint first, GLsizei count, GLsizei primcount)
{
    (void) mode; (void) first; (void) count; (void) primcount;
   DISPATCH(DrawArraysInstancedARB, (mode, first, count, primcount), (F, "glDrawArraysInstanced(0x%x, %d, %d, %d);\n", mode, first, count, primcount));
}

KEYWORD1 void KEYWORD2 NAME(DrawElementsInstancedARB)(GLenum mode, GLsizei count, GLenum type, const GLvoid * indices, GLsizei primcount)
{
    (void) mode; (void) count; (void) type; (void) indices; (void) primcount;
   DISPATCH(DrawElementsInstancedARB, (mode, count, type, indices, primcount), (F, "glDrawElementsInstancedARB(0x%x, %d, 0x%x, %p, %d);\n", mode, count, type, (const void *) indices, primcount));
}

KEYWORD1 void KEYWORD2 NAME(DrawElementsInstancedEXT)(GLenum mode, GLsizei count, GLenum type, const GLvoid * indices, GLsizei primcount)
{
    (void) mode; (void) count; (void) type; (void) indices; (void) primcount;
   DISPATCH(DrawElementsInstancedARB, (mode, count, type, indices, primcount), (F, "glDrawElementsInstancedEXT(0x%x, %d, 0x%x, %p, %d);\n", mode, count, type, (const void *) indices, primcount));
}

KEYWORD1 void KEYWORD2 NAME(DrawElementsInstanced)(GLenum mode, GLsizei count, GLenum type, const GLvoid * indices, GLsizei primcount)
{
    (void) mode; (void) count; (void) type; (void) indices; (void) primcount;
   DISPATCH(DrawElementsInstancedARB, (mode, count, type, indices, primcount), (F, "glDrawElementsInstanced(0x%x, %d, 0x%x, %p, %d);\n", mode, count, type, (const void *) indices, primcount));
}

KEYWORD1 void KEYWORD2 NAME(RenderbufferStorageMultisample)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{
    (void) target; (void) samples; (void) internalformat; (void) width; (void) height;
   DISPATCH(RenderbufferStorageMultisample, (target, samples, internalformat, width, height), (F, "glRenderbufferStorageMultisample(0x%x, %d, 0x%x, %d, %d);\n", target, samples, internalformat, width, height));
}

KEYWORD1 void KEYWORD2 NAME(RenderbufferStorageMultisampleEXT)(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{
    (void) target; (void) samples; (void) internalformat; (void) width; (void) height;
   DISPATCH(RenderbufferStorageMultisample, (target, samples, internalformat, width, height), (F, "glRenderbufferStorageMultisampleEXT(0x%x, %d, 0x%x, %d, %d);\n", target, samples, internalformat, width, height));
}

KEYWORD1 void KEYWORD2 NAME(FramebufferTextureARB)(GLenum target, GLenum attachment, GLuint texture, GLint level)
{
    (void) target; (void) attachment; (void) texture; (void) level;
   DISPATCH(FramebufferTextureARB, (target, attachment, texture, level), (F, "glFramebufferTextureARB(0x%x, 0x%x, %d, %d);\n", target, attachment, texture, level));
}

KEYWORD1 void KEYWORD2 NAME(FramebufferTextureFaceARB)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLenum face)
{
    (void) target; (void) attachment; (void) texture; (void) level; (void) face;
   DISPATCH(FramebufferTextureFaceARB, (target, attachment, texture, level, face), (F, "glFramebufferTextureFaceARB(0x%x, 0x%x, %d, %d, 0x%x);\n", target, attachment, texture, level, face));
}

KEYWORD1 void KEYWORD2 NAME(ProgramParameteriARB)(GLuint program, GLenum pname, GLint value)
{
    (void) program; (void) pname; (void) value;
   DISPATCH(ProgramParameteriARB, (program, pname, value), (F, "glProgramParameteriARB(%d, 0x%x, %d);\n", program, pname, value));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribDivisorARB)(GLuint index, GLuint divisor)
{
    (void) index; (void) divisor;
   DISPATCH(VertexAttribDivisorARB, (index, divisor), (F, "glVertexAttribDivisorARB(%d, %d);\n", index, divisor));
}

KEYWORD1 void KEYWORD2 NAME(FlushMappedBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length)
{
    (void) target; (void) offset; (void) length;
   DISPATCH(FlushMappedBufferRange, (target, offset, length), (F, "glFlushMappedBufferRange(0x%x, %d, %d);\n", target, offset, length));
}

KEYWORD1 GLvoid * KEYWORD2 NAME(MapBufferRange)(GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access)
{
    (void) target; (void) offset; (void) length; (void) access;
   RETURN_DISPATCH(MapBufferRange, (target, offset, length, access), (F, "glMapBufferRange(0x%x, %d, %d, %d);\n", target, offset, length, access));
}

KEYWORD1 void KEYWORD2 NAME(TexBufferARB)(GLenum target, GLenum internalFormat, GLuint buffer)
{
    (void) target; (void) internalFormat; (void) buffer;
   DISPATCH(TexBufferARB, (target, internalFormat, buffer), (F, "glTexBufferARB(0x%x, 0x%x, %d);\n", target, internalFormat, buffer));
}

KEYWORD1 void KEYWORD2 NAME(BindVertexArray)(GLuint array)
{
    (void) array;
   DISPATCH(BindVertexArray, (array), (F, "glBindVertexArray(%d);\n", array));
}

KEYWORD1 void KEYWORD2 NAME(GenVertexArrays)(GLsizei n, GLuint * arrays)
{
    (void) n; (void) arrays;
   DISPATCH(GenVertexArrays, (n, arrays), (F, "glGenVertexArrays(%d, %p);\n", n, (const void *) arrays));
}

KEYWORD1 void KEYWORD2 NAME(CopyBufferSubData)(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size)
{
    (void) readTarget; (void) writeTarget; (void) readOffset; (void) writeOffset; (void) size;
   DISPATCH(CopyBufferSubData, (readTarget, writeTarget, readOffset, writeOffset, size), (F, "glCopyBufferSubData(0x%x, 0x%x, %d, %d, %d);\n", readTarget, writeTarget, readOffset, writeOffset, size));
}

KEYWORD1 GLenum KEYWORD2 NAME(ClientWaitSync)(GLsync sync, GLbitfield flags, GLuint64 timeout)
{
    (void) sync; (void) flags; (void) timeout;
   RETURN_DISPATCH(ClientWaitSync, (sync, flags, timeout), (F, "glClientWaitSync(%d, %d, %d);\n", sync, flags, timeout));
}

KEYWORD1 void KEYWORD2 NAME(DeleteSync)(GLsync sync)
{
    (void) sync;
   DISPATCH(DeleteSync, (sync), (F, "glDeleteSync(%d);\n", sync));
}

KEYWORD1 GLsync KEYWORD2 NAME(FenceSync)(GLenum condition, GLbitfield flags)
{
    (void) condition; (void) flags;
   RETURN_DISPATCH(FenceSync, (condition, flags), (F, "glFenceSync(0x%x, %d);\n", condition, flags));
}

KEYWORD1 void KEYWORD2 NAME(GetInteger64v)(GLenum pname, GLint64 * params)
{
    (void) pname; (void) params;
   DISPATCH(GetInteger64v, (pname, params), (F, "glGetInteger64v(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetSynciv)(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei * length, GLint * values)
{
    (void) sync; (void) pname; (void) bufSize; (void) length; (void) values;
   DISPATCH(GetSynciv, (sync, pname, bufSize, length, values), (F, "glGetSynciv(%d, 0x%x, %d, %p, %p);\n", sync, pname, bufSize, (const void *) length, (const void *) values));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsSync)(GLsync sync)
{
    (void) sync;
   RETURN_DISPATCH(IsSync, (sync), (F, "glIsSync(%d);\n", sync));
}

KEYWORD1 void KEYWORD2 NAME(WaitSync)(GLsync sync, GLbitfield flags, GLuint64 timeout)
{
    (void) sync; (void) flags; (void) timeout;
   DISPATCH(WaitSync, (sync, flags, timeout), (F, "glWaitSync(%d, %d, %d);\n", sync, flags, timeout));
}

KEYWORD1 void KEYWORD2 NAME(DrawElementsBaseVertex)(GLenum mode, GLsizei count, GLenum type, const GLvoid * indices, GLint basevertex)
{
    (void) mode; (void) count; (void) type; (void) indices; (void) basevertex;
   DISPATCH(DrawElementsBaseVertex, (mode, count, type, indices, basevertex), (F, "glDrawElementsBaseVertex(0x%x, %d, 0x%x, %p, %d);\n", mode, count, type, (const void *) indices, basevertex));
}

KEYWORD1 void KEYWORD2 NAME(DrawElementsInstancedBaseVertex)(GLenum mode, GLsizei count, GLenum type, const GLvoid * indices, GLsizei primcount, GLint basevertex)
{
    (void) mode; (void) count; (void) type; (void) indices; (void) primcount; (void) basevertex;
   DISPATCH(DrawElementsInstancedBaseVertex, (mode, count, type, indices, primcount, basevertex), (F, "glDrawElementsInstancedBaseVertex(0x%x, %d, 0x%x, %p, %d, %d);\n", mode, count, type, (const void *) indices, primcount, basevertex));
}

KEYWORD1 void KEYWORD2 NAME(DrawRangeElementsBaseVertex)(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid * indices, GLint basevertex)
{
    (void) mode; (void) start; (void) end; (void) count; (void) type; (void) indices; (void) basevertex;
   DISPATCH(DrawRangeElementsBaseVertex, (mode, start, end, count, type, indices, basevertex), (F, "glDrawRangeElementsBaseVertex(0x%x, %d, %d, %d, 0x%x, %p, %d);\n", mode, start, end, count, type, (const void *) indices, basevertex));
}

KEYWORD1 void KEYWORD2 NAME(MultiDrawElementsBaseVertex)(GLenum mode, const GLsizei * count, GLenum type, const GLvoid ** indices, GLsizei primcount, const GLint * basevertex)
{
    (void) mode; (void) count; (void) type; (void) indices; (void) primcount; (void) basevertex;
   DISPATCH(MultiDrawElementsBaseVertex, (mode, count, type, indices, primcount, basevertex), (F, "glMultiDrawElementsBaseVertex(0x%x, %p, 0x%x, %p, %d, %p);\n", mode, (const void *) count, type, (const void *) indices, primcount, (const void *) basevertex));
}

KEYWORD1 void KEYWORD2 NAME(BlendEquationSeparateiARB)(GLuint buf, GLenum modeRGB, GLenum modeA)
{
    (void) buf; (void) modeRGB; (void) modeA;
   DISPATCH(BlendEquationSeparateiARB, (buf, modeRGB, modeA), (F, "glBlendEquationSeparateiARB(%d, 0x%x, 0x%x);\n", buf, modeRGB, modeA));
}

KEYWORD1 void KEYWORD2 NAME(BlendEquationSeparateIndexedAMD)(GLuint buf, GLenum modeRGB, GLenum modeA)
{
    (void) buf; (void) modeRGB; (void) modeA;
   DISPATCH(BlendEquationSeparateiARB, (buf, modeRGB, modeA), (F, "glBlendEquationSeparateIndexedAMD(%d, 0x%x, 0x%x);\n", buf, modeRGB, modeA));
}

KEYWORD1 void KEYWORD2 NAME(BlendEquationiARB)(GLuint buf, GLenum mode)
{
    (void) buf; (void) mode;
   DISPATCH(BlendEquationiARB, (buf, mode), (F, "glBlendEquationiARB(%d, 0x%x);\n", buf, mode));
}

KEYWORD1 void KEYWORD2 NAME(BlendEquationIndexedAMD)(GLuint buf, GLenum mode)
{
    (void) buf; (void) mode;
   DISPATCH(BlendEquationiARB, (buf, mode), (F, "glBlendEquationIndexedAMD(%d, 0x%x);\n", buf, mode));
}

KEYWORD1 void KEYWORD2 NAME(BlendFuncSeparateiARB)(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcA, GLenum dstA)
{
    (void) buf; (void) srcRGB; (void) dstRGB; (void) srcA; (void) dstA;
   DISPATCH(BlendFuncSeparateiARB, (buf, srcRGB, dstRGB, srcA, dstA), (F, "glBlendFuncSeparateiARB(%d, 0x%x, 0x%x, 0x%x, 0x%x);\n", buf, srcRGB, dstRGB, srcA, dstA));
}

KEYWORD1 void KEYWORD2 NAME(BlendFuncSeparateIndexedAMD)(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcA, GLenum dstA)
{
    (void) buf; (void) srcRGB; (void) dstRGB; (void) srcA; (void) dstA;
   DISPATCH(BlendFuncSeparateiARB, (buf, srcRGB, dstRGB, srcA, dstA), (F, "glBlendFuncSeparateIndexedAMD(%d, 0x%x, 0x%x, 0x%x, 0x%x);\n", buf, srcRGB, dstRGB, srcA, dstA));
}

KEYWORD1 void KEYWORD2 NAME(BlendFunciARB)(GLuint buf, GLenum src, GLenum dst)
{
    (void) buf; (void) src; (void) dst;
   DISPATCH(BlendFunciARB, (buf, src, dst), (F, "glBlendFunciARB(%d, 0x%x, 0x%x);\n", buf, src, dst));
}

KEYWORD1 void KEYWORD2 NAME(BlendFuncIndexedAMD)(GLuint buf, GLenum src, GLenum dst)
{
    (void) buf; (void) src; (void) dst;
   DISPATCH(BlendFunciARB, (buf, src, dst), (F, "glBlendFuncIndexedAMD(%d, 0x%x, 0x%x);\n", buf, src, dst));
}

KEYWORD1 void KEYWORD2 NAME(BindSampler)(GLuint unit, GLuint sampler)
{
    (void) unit; (void) sampler;
   DISPATCH(BindSampler, (unit, sampler), (F, "glBindSampler(%d, %d);\n", unit, sampler));
}

KEYWORD1 void KEYWORD2 NAME(DeleteSamplers)(GLsizei count, const GLuint * samplers)
{
    (void) count; (void) samplers;
   DISPATCH(DeleteSamplers, (count, samplers), (F, "glDeleteSamplers(%d, %p);\n", count, (const void *) samplers));
}

KEYWORD1 void KEYWORD2 NAME(GenSamplers)(GLsizei count, GLuint * samplers)
{
    (void) count; (void) samplers;
   DISPATCH(GenSamplers, (count, samplers), (F, "glGenSamplers(%d, %p);\n", count, (const void *) samplers));
}

KEYWORD1 void KEYWORD2 NAME(GetSamplerParameterIiv)(GLuint sampler, GLenum pname, GLint * params)
{
    (void) sampler; (void) pname; (void) params;
   DISPATCH(GetSamplerParameterIiv, (sampler, pname, params), (F, "glGetSamplerParameterIiv(%d, 0x%x, %p);\n", sampler, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetSamplerParameterIuiv)(GLuint sampler, GLenum pname, GLuint * params)
{
    (void) sampler; (void) pname; (void) params;
   DISPATCH(GetSamplerParameterIuiv, (sampler, pname, params), (F, "glGetSamplerParameterIuiv(%d, 0x%x, %p);\n", sampler, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetSamplerParameterfv)(GLuint sampler, GLenum pname, GLfloat * params)
{
    (void) sampler; (void) pname; (void) params;
   DISPATCH(GetSamplerParameterfv, (sampler, pname, params), (F, "glGetSamplerParameterfv(%d, 0x%x, %p);\n", sampler, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetSamplerParameteriv)(GLuint sampler, GLenum pname, GLint * params)
{
    (void) sampler; (void) pname; (void) params;
   DISPATCH(GetSamplerParameteriv, (sampler, pname, params), (F, "glGetSamplerParameteriv(%d, 0x%x, %p);\n", sampler, pname, (const void *) params));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsSampler)(GLuint sampler)
{
    (void) sampler;
   RETURN_DISPATCH(IsSampler, (sampler), (F, "glIsSampler(%d);\n", sampler));
}

KEYWORD1 void KEYWORD2 NAME(SamplerParameterIiv)(GLuint sampler, GLenum pname, const GLint * params)
{
    (void) sampler; (void) pname; (void) params;
   DISPATCH(SamplerParameterIiv, (sampler, pname, params), (F, "glSamplerParameterIiv(%d, 0x%x, %p);\n", sampler, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(SamplerParameterIuiv)(GLuint sampler, GLenum pname, const GLuint * params)
{
    (void) sampler; (void) pname; (void) params;
   DISPATCH(SamplerParameterIuiv, (sampler, pname, params), (F, "glSamplerParameterIuiv(%d, 0x%x, %p);\n", sampler, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(SamplerParameterf)(GLuint sampler, GLenum pname, GLfloat param)
{
    (void) sampler; (void) pname; (void) param;
   DISPATCH(SamplerParameterf, (sampler, pname, param), (F, "glSamplerParameterf(%d, 0x%x, %f);\n", sampler, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(SamplerParameterfv)(GLuint sampler, GLenum pname, const GLfloat * params)
{
    (void) sampler; (void) pname; (void) params;
   DISPATCH(SamplerParameterfv, (sampler, pname, params), (F, "glSamplerParameterfv(%d, 0x%x, %p);\n", sampler, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(SamplerParameteri)(GLuint sampler, GLenum pname, GLint param)
{
    (void) sampler; (void) pname; (void) param;
   DISPATCH(SamplerParameteri, (sampler, pname, param), (F, "glSamplerParameteri(%d, 0x%x, %d);\n", sampler, pname, param));
}

KEYWORD1 void KEYWORD2 NAME(SamplerParameteriv)(GLuint sampler, GLenum pname, const GLint * params)
{
    (void) sampler; (void) pname; (void) params;
   DISPATCH(SamplerParameteriv, (sampler, pname, params), (F, "glSamplerParameteriv(%d, 0x%x, %p);\n", sampler, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ColorP3ui)(GLenum type, GLuint color)
{
    (void) type; (void) color;
   DISPATCH(ColorP3ui, (type, color), (F, "glColorP3ui(0x%x, %d);\n", type, color));
}

KEYWORD1 void KEYWORD2 NAME(ColorP3uiv)(GLenum type, const GLuint * color)
{
    (void) type; (void) color;
   DISPATCH(ColorP3uiv, (type, color), (F, "glColorP3uiv(0x%x, %p);\n", type, (const void *) color));
}

KEYWORD1 void KEYWORD2 NAME(ColorP4ui)(GLenum type, GLuint color)
{
    (void) type; (void) color;
   DISPATCH(ColorP4ui, (type, color), (F, "glColorP4ui(0x%x, %d);\n", type, color));
}

KEYWORD1 void KEYWORD2 NAME(ColorP4uiv)(GLenum type, const GLuint * color)
{
    (void) type; (void) color;
   DISPATCH(ColorP4uiv, (type, color), (F, "glColorP4uiv(0x%x, %p);\n", type, (const void *) color));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoordP1ui)(GLenum texture, GLenum type, GLuint coords)
{
    (void) texture; (void) type; (void) coords;
   DISPATCH(MultiTexCoordP1ui, (texture, type, coords), (F, "glMultiTexCoordP1ui(0x%x, 0x%x, %d);\n", texture, type, coords));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoordP1uiv)(GLenum texture, GLenum type, const GLuint * coords)
{
    (void) texture; (void) type; (void) coords;
   DISPATCH(MultiTexCoordP1uiv, (texture, type, coords), (F, "glMultiTexCoordP1uiv(0x%x, 0x%x, %p);\n", texture, type, (const void *) coords));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoordP2ui)(GLenum texture, GLenum type, GLuint coords)
{
    (void) texture; (void) type; (void) coords;
   DISPATCH(MultiTexCoordP2ui, (texture, type, coords), (F, "glMultiTexCoordP2ui(0x%x, 0x%x, %d);\n", texture, type, coords));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoordP2uiv)(GLenum texture, GLenum type, const GLuint * coords)
{
    (void) texture; (void) type; (void) coords;
   DISPATCH(MultiTexCoordP2uiv, (texture, type, coords), (F, "glMultiTexCoordP2uiv(0x%x, 0x%x, %p);\n", texture, type, (const void *) coords));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoordP3ui)(GLenum texture, GLenum type, GLuint coords)
{
    (void) texture; (void) type; (void) coords;
   DISPATCH(MultiTexCoordP3ui, (texture, type, coords), (F, "glMultiTexCoordP3ui(0x%x, 0x%x, %d);\n", texture, type, coords));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoordP3uiv)(GLenum texture, GLenum type, const GLuint * coords)
{
    (void) texture; (void) type; (void) coords;
   DISPATCH(MultiTexCoordP3uiv, (texture, type, coords), (F, "glMultiTexCoordP3uiv(0x%x, 0x%x, %p);\n", texture, type, (const void *) coords));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoordP4ui)(GLenum texture, GLenum type, GLuint coords)
{
    (void) texture; (void) type; (void) coords;
   DISPATCH(MultiTexCoordP4ui, (texture, type, coords), (F, "glMultiTexCoordP4ui(0x%x, 0x%x, %d);\n", texture, type, coords));
}

KEYWORD1 void KEYWORD2 NAME(MultiTexCoordP4uiv)(GLenum texture, GLenum type, const GLuint * coords)
{
    (void) texture; (void) type; (void) coords;
   DISPATCH(MultiTexCoordP4uiv, (texture, type, coords), (F, "glMultiTexCoordP4uiv(0x%x, 0x%x, %p);\n", texture, type, (const void *) coords));
}

KEYWORD1 void KEYWORD2 NAME(NormalP3ui)(GLenum type, GLuint coords)
{
    (void) type; (void) coords;
   DISPATCH(NormalP3ui, (type, coords), (F, "glNormalP3ui(0x%x, %d);\n", type, coords));
}

KEYWORD1 void KEYWORD2 NAME(NormalP3uiv)(GLenum type, const GLuint * coords)
{
    (void) type; (void) coords;
   DISPATCH(NormalP3uiv, (type, coords), (F, "glNormalP3uiv(0x%x, %p);\n", type, (const void *) coords));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColorP3ui)(GLenum type, GLuint color)
{
    (void) type; (void) color;
   DISPATCH(SecondaryColorP3ui, (type, color), (F, "glSecondaryColorP3ui(0x%x, %d);\n", type, color));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColorP3uiv)(GLenum type, const GLuint * color)
{
    (void) type; (void) color;
   DISPATCH(SecondaryColorP3uiv, (type, color), (F, "glSecondaryColorP3uiv(0x%x, %p);\n", type, (const void *) color));
}

KEYWORD1 void KEYWORD2 NAME(TexCoordP1ui)(GLenum type, GLuint coords)
{
    (void) type; (void) coords;
   DISPATCH(TexCoordP1ui, (type, coords), (F, "glTexCoordP1ui(0x%x, %d);\n", type, coords));
}

KEYWORD1 void KEYWORD2 NAME(TexCoordP1uiv)(GLenum type, const GLuint * coords)
{
    (void) type; (void) coords;
   DISPATCH(TexCoordP1uiv, (type, coords), (F, "glTexCoordP1uiv(0x%x, %p);\n", type, (const void *) coords));
}

KEYWORD1 void KEYWORD2 NAME(TexCoordP2ui)(GLenum type, GLuint coords)
{
    (void) type; (void) coords;
   DISPATCH(TexCoordP2ui, (type, coords), (F, "glTexCoordP2ui(0x%x, %d);\n", type, coords));
}

KEYWORD1 void KEYWORD2 NAME(TexCoordP2uiv)(GLenum type, const GLuint * coords)
{
    (void) type; (void) coords;
   DISPATCH(TexCoordP2uiv, (type, coords), (F, "glTexCoordP2uiv(0x%x, %p);\n", type, (const void *) coords));
}

KEYWORD1 void KEYWORD2 NAME(TexCoordP3ui)(GLenum type, GLuint coords)
{
    (void) type; (void) coords;
   DISPATCH(TexCoordP3ui, (type, coords), (F, "glTexCoordP3ui(0x%x, %d);\n", type, coords));
}

KEYWORD1 void KEYWORD2 NAME(TexCoordP3uiv)(GLenum type, const GLuint * coords)
{
    (void) type; (void) coords;
   DISPATCH(TexCoordP3uiv, (type, coords), (F, "glTexCoordP3uiv(0x%x, %p);\n", type, (const void *) coords));
}

KEYWORD1 void KEYWORD2 NAME(TexCoordP4ui)(GLenum type, GLuint coords)
{
    (void) type; (void) coords;
   DISPATCH(TexCoordP4ui, (type, coords), (F, "glTexCoordP4ui(0x%x, %d);\n", type, coords));
}

KEYWORD1 void KEYWORD2 NAME(TexCoordP4uiv)(GLenum type, const GLuint * coords)
{
    (void) type; (void) coords;
   DISPATCH(TexCoordP4uiv, (type, coords), (F, "glTexCoordP4uiv(0x%x, %p);\n", type, (const void *) coords));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribP1ui)(GLuint index, GLenum type, GLboolean normalized, GLuint value)
{
    (void) index; (void) type; (void) normalized; (void) value;
   DISPATCH(VertexAttribP1ui, (index, type, normalized, value), (F, "glVertexAttribP1ui(%d, 0x%x, %d, %d);\n", index, type, normalized, value));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribP1uiv)(GLuint index, GLenum type, GLboolean normalized, const GLuint * value)
{
    (void) index; (void) type; (void) normalized; (void) value;
   DISPATCH(VertexAttribP1uiv, (index, type, normalized, value), (F, "glVertexAttribP1uiv(%d, 0x%x, %d, %p);\n", index, type, normalized, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribP2ui)(GLuint index, GLenum type, GLboolean normalized, GLuint value)
{
    (void) index; (void) type; (void) normalized; (void) value;
   DISPATCH(VertexAttribP2ui, (index, type, normalized, value), (F, "glVertexAttribP2ui(%d, 0x%x, %d, %d);\n", index, type, normalized, value));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribP2uiv)(GLuint index, GLenum type, GLboolean normalized, const GLuint * value)
{
    (void) index; (void) type; (void) normalized; (void) value;
   DISPATCH(VertexAttribP2uiv, (index, type, normalized, value), (F, "glVertexAttribP2uiv(%d, 0x%x, %d, %p);\n", index, type, normalized, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribP3ui)(GLuint index, GLenum type, GLboolean normalized, GLuint value)
{
    (void) index; (void) type; (void) normalized; (void) value;
   DISPATCH(VertexAttribP3ui, (index, type, normalized, value), (F, "glVertexAttribP3ui(%d, 0x%x, %d, %d);\n", index, type, normalized, value));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribP3uiv)(GLuint index, GLenum type, GLboolean normalized, const GLuint * value)
{
    (void) index; (void) type; (void) normalized; (void) value;
   DISPATCH(VertexAttribP3uiv, (index, type, normalized, value), (F, "glVertexAttribP3uiv(%d, 0x%x, %d, %p);\n", index, type, normalized, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribP4ui)(GLuint index, GLenum type, GLboolean normalized, GLuint value)
{
    (void) index; (void) type; (void) normalized; (void) value;
   DISPATCH(VertexAttribP4ui, (index, type, normalized, value), (F, "glVertexAttribP4ui(%d, 0x%x, %d, %d);\n", index, type, normalized, value));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribP4uiv)(GLuint index, GLenum type, GLboolean normalized, const GLuint * value)
{
    (void) index; (void) type; (void) normalized; (void) value;
   DISPATCH(VertexAttribP4uiv, (index, type, normalized, value), (F, "glVertexAttribP4uiv(%d, 0x%x, %d, %p);\n", index, type, normalized, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(VertexP2ui)(GLenum type, GLuint value)
{
    (void) type; (void) value;
   DISPATCH(VertexP2ui, (type, value), (F, "glVertexP2ui(0x%x, %d);\n", type, value));
}

KEYWORD1 void KEYWORD2 NAME(VertexP2uiv)(GLenum type, const GLuint * value)
{
    (void) type; (void) value;
   DISPATCH(VertexP2uiv, (type, value), (F, "glVertexP2uiv(0x%x, %p);\n", type, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(VertexP3ui)(GLenum type, GLuint value)
{
    (void) type; (void) value;
   DISPATCH(VertexP3ui, (type, value), (F, "glVertexP3ui(0x%x, %d);\n", type, value));
}

KEYWORD1 void KEYWORD2 NAME(VertexP3uiv)(GLenum type, const GLuint * value)
{
    (void) type; (void) value;
   DISPATCH(VertexP3uiv, (type, value), (F, "glVertexP3uiv(0x%x, %p);\n", type, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(VertexP4ui)(GLenum type, GLuint value)
{
    (void) type; (void) value;
   DISPATCH(VertexP4ui, (type, value), (F, "glVertexP4ui(0x%x, %d);\n", type, value));
}

KEYWORD1 void KEYWORD2 NAME(VertexP4uiv)(GLenum type, const GLuint * value)
{
    (void) type; (void) value;
   DISPATCH(VertexP4uiv, (type, value), (F, "glVertexP4uiv(0x%x, %p);\n", type, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(BindTransformFeedback)(GLenum target, GLuint id)
{
    (void) target; (void) id;
   DISPATCH(BindTransformFeedback, (target, id), (F, "glBindTransformFeedback(0x%x, %d);\n", target, id));
}

KEYWORD1 void KEYWORD2 NAME(DeleteTransformFeedbacks)(GLsizei n, const GLuint * ids)
{
    (void) n; (void) ids;
   DISPATCH(DeleteTransformFeedbacks, (n, ids), (F, "glDeleteTransformFeedbacks(%d, %p);\n", n, (const void *) ids));
}

KEYWORD1 void KEYWORD2 NAME(DrawTransformFeedback)(GLenum mode, GLuint id)
{
    (void) mode; (void) id;
   DISPATCH(DrawTransformFeedback, (mode, id), (F, "glDrawTransformFeedback(0x%x, %d);\n", mode, id));
}

KEYWORD1 void KEYWORD2 NAME(GenTransformFeedbacks)(GLsizei n, GLuint * ids)
{
    (void) n; (void) ids;
   DISPATCH(GenTransformFeedbacks, (n, ids), (F, "glGenTransformFeedbacks(%d, %p);\n", n, (const void *) ids));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsTransformFeedback)(GLuint id)
{
    (void) id;
   RETURN_DISPATCH(IsTransformFeedback, (id), (F, "glIsTransformFeedback(%d);\n", id));
}

KEYWORD1 void KEYWORD2 NAME(PauseTransformFeedback)(void)
{
   DISPATCH(PauseTransformFeedback, (), (F, "glPauseTransformFeedback();\n"));
}

KEYWORD1 void KEYWORD2 NAME(ResumeTransformFeedback)(void)
{
   DISPATCH(ResumeTransformFeedback, (), (F, "glResumeTransformFeedback();\n"));
}

KEYWORD1 void KEYWORD2 NAME(ClearDepthf)(GLclampf depth)
{
    (void) depth;
   DISPATCH(ClearDepthf, (depth), (F, "glClearDepthf(%f);\n", depth));
}

KEYWORD1 void KEYWORD2 NAME(DepthRangef)(GLclampf zNear, GLclampf zFar)
{
    (void) zNear; (void) zFar;
   DISPATCH(DepthRangef, (zNear, zFar), (F, "glDepthRangef(%f, %f);\n", zNear, zFar));
}

KEYWORD1 void KEYWORD2 NAME(GetShaderPrecisionFormat)(GLenum shadertype, GLenum precisiontype, GLint * range, GLint * precision)
{
    (void) shadertype; (void) precisiontype; (void) range; (void) precision;
   DISPATCH(GetShaderPrecisionFormat, (shadertype, precisiontype, range, precision), (F, "glGetShaderPrecisionFormat(0x%x, 0x%x, %p, %p);\n", shadertype, precisiontype, (const void *) range, (const void *) precision));
}

KEYWORD1 void KEYWORD2 NAME(ReleaseShaderCompiler)(void)
{
   DISPATCH(ReleaseShaderCompiler, (), (F, "glReleaseShaderCompiler();\n"));
}

KEYWORD1 void KEYWORD2 NAME(ShaderBinary)(GLsizei n, const GLuint * shaders, GLenum binaryformat, const GLvoid * binary, GLsizei length)
{
    (void) n; (void) shaders; (void) binaryformat; (void) binary; (void) length;
   DISPATCH(ShaderBinary, (n, shaders, binaryformat, binary, length), (F, "glShaderBinary(%d, %p, 0x%x, %p, %d);\n", n, (const void *) shaders, binaryformat, (const void *) binary, length));
}

KEYWORD1 GLenum KEYWORD2 NAME(GetGraphicsResetStatusARB)(void)
{
   RETURN_DISPATCH(GetGraphicsResetStatusARB, (), (F, "glGetGraphicsResetStatusARB();\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetnColorTableARB)(GLenum target, GLenum format, GLenum type, GLsizei bufSize, GLvoid * table)
{
    (void) target; (void) format; (void) type; (void) bufSize; (void) table;
   DISPATCH(GetnColorTableARB, (target, format, type, bufSize, table), (F, "glGetnColorTableARB(0x%x, 0x%x, 0x%x, %d, %p);\n", target, format, type, bufSize, (const void *) table));
}

KEYWORD1 void KEYWORD2 NAME(GetnCompressedTexImageARB)(GLenum target, GLint lod, GLsizei bufSize, GLvoid * img)
{
    (void) target; (void) lod; (void) bufSize; (void) img;
   DISPATCH(GetnCompressedTexImageARB, (target, lod, bufSize, img), (F, "glGetnCompressedTexImageARB(0x%x, %d, %d, %p);\n", target, lod, bufSize, (const void *) img));
}

KEYWORD1 void KEYWORD2 NAME(GetnConvolutionFilterARB)(GLenum target, GLenum format, GLenum type, GLsizei bufSize, GLvoid * image)
{
    (void) target; (void) format; (void) type; (void) bufSize; (void) image;
   DISPATCH(GetnConvolutionFilterARB, (target, format, type, bufSize, image), (F, "glGetnConvolutionFilterARB(0x%x, 0x%x, 0x%x, %d, %p);\n", target, format, type, bufSize, (const void *) image));
}

KEYWORD1 void KEYWORD2 NAME(GetnHistogramARB)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, GLvoid * values)
{
    (void) target; (void) reset; (void) format; (void) type; (void) bufSize; (void) values;
   DISPATCH(GetnHistogramARB, (target, reset, format, type, bufSize, values), (F, "glGetnHistogramARB(0x%x, %d, 0x%x, 0x%x, %d, %p);\n", target, reset, format, type, bufSize, (const void *) values));
}

KEYWORD1 void KEYWORD2 NAME(GetnMapdvARB)(GLenum target, GLenum query, GLsizei bufSize, GLdouble * v)
{
    (void) target; (void) query; (void) bufSize; (void) v;
   DISPATCH(GetnMapdvARB, (target, query, bufSize, v), (F, "glGetnMapdvARB(0x%x, 0x%x, %d, %p);\n", target, query, bufSize, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(GetnMapfvARB)(GLenum target, GLenum query, GLsizei bufSize, GLfloat * v)
{
    (void) target; (void) query; (void) bufSize; (void) v;
   DISPATCH(GetnMapfvARB, (target, query, bufSize, v), (F, "glGetnMapfvARB(0x%x, 0x%x, %d, %p);\n", target, query, bufSize, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(GetnMapivARB)(GLenum target, GLenum query, GLsizei bufSize, GLint * v)
{
    (void) target; (void) query; (void) bufSize; (void) v;
   DISPATCH(GetnMapivARB, (target, query, bufSize, v), (F, "glGetnMapivARB(0x%x, 0x%x, %d, %p);\n", target, query, bufSize, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(GetnMinmaxARB)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLsizei bufSize, GLvoid * values)
{
    (void) target; (void) reset; (void) format; (void) type; (void) bufSize; (void) values;
   DISPATCH(GetnMinmaxARB, (target, reset, format, type, bufSize, values), (F, "glGetnMinmaxARB(0x%x, %d, 0x%x, 0x%x, %d, %p);\n", target, reset, format, type, bufSize, (const void *) values));
}

KEYWORD1 void KEYWORD2 NAME(GetnPixelMapfvARB)(GLenum map, GLsizei bufSize, GLfloat * values)
{
    (void) map; (void) bufSize; (void) values;
   DISPATCH(GetnPixelMapfvARB, (map, bufSize, values), (F, "glGetnPixelMapfvARB(0x%x, %d, %p);\n", map, bufSize, (const void *) values));
}

KEYWORD1 void KEYWORD2 NAME(GetnPixelMapuivARB)(GLenum map, GLsizei bufSize, GLuint * values)
{
    (void) map; (void) bufSize; (void) values;
   DISPATCH(GetnPixelMapuivARB, (map, bufSize, values), (F, "glGetnPixelMapuivARB(0x%x, %d, %p);\n", map, bufSize, (const void *) values));
}

KEYWORD1 void KEYWORD2 NAME(GetnPixelMapusvARB)(GLenum map, GLsizei bufSize, GLushort * values)
{
    (void) map; (void) bufSize; (void) values;
   DISPATCH(GetnPixelMapusvARB, (map, bufSize, values), (F, "glGetnPixelMapusvARB(0x%x, %d, %p);\n", map, bufSize, (const void *) values));
}

KEYWORD1 void KEYWORD2 NAME(GetnPolygonStippleARB)(GLsizei bufSize, GLubyte * pattern)
{
    (void) bufSize; (void) pattern;
   DISPATCH(GetnPolygonStippleARB, (bufSize, pattern), (F, "glGetnPolygonStippleARB(%d, %p);\n", bufSize, (const void *) pattern));
}

KEYWORD1 void KEYWORD2 NAME(GetnSeparableFilterARB)(GLenum target, GLenum format, GLenum type, GLsizei rowBufSize, GLvoid * row, GLsizei columnBufSize, GLvoid * column, GLvoid * span)
{
    (void) target; (void) format; (void) type; (void) rowBufSize; (void) row; (void) columnBufSize; (void) column; (void) span;
   DISPATCH(GetnSeparableFilterARB, (target, format, type, rowBufSize, row, columnBufSize, column, span), (F, "glGetnSeparableFilterARB(0x%x, 0x%x, 0x%x, %d, %p, %d, %p, %p);\n", target, format, type, rowBufSize, (const void *) row, columnBufSize, (const void *) column, (const void *) span));
}

KEYWORD1 void KEYWORD2 NAME(GetnTexImageARB)(GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, GLvoid * img)
{
    (void) target; (void) level; (void) format; (void) type; (void) bufSize; (void) img;
   DISPATCH(GetnTexImageARB, (target, level, format, type, bufSize, img), (F, "glGetnTexImageARB(0x%x, %d, 0x%x, 0x%x, %d, %p);\n", target, level, format, type, bufSize, (const void *) img));
}

KEYWORD1 void KEYWORD2 NAME(GetnUniformdvARB)(GLhandleARB program, GLint location, GLsizei bufSize, GLdouble * params)
{
    (void) program; (void) location; (void) bufSize; (void) params;
   DISPATCH(GetnUniformdvARB, (program, location, bufSize, params), (F, "glGetnUniformdvARB(%d, %d, %d, %p);\n", program, location, bufSize, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetnUniformfvARB)(GLhandleARB program, GLint location, GLsizei bufSize, GLfloat * params)
{
    (void) program; (void) location; (void) bufSize; (void) params;
   DISPATCH(GetnUniformfvARB, (program, location, bufSize, params), (F, "glGetnUniformfvARB(%d, %d, %d, %p);\n", program, location, bufSize, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetnUniformivARB)(GLhandleARB program, GLint location, GLsizei bufSize, GLint * params)
{
    (void) program; (void) location; (void) bufSize; (void) params;
   DISPATCH(GetnUniformivARB, (program, location, bufSize, params), (F, "glGetnUniformivARB(%d, %d, %d, %p);\n", program, location, bufSize, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetnUniformuivARB)(GLhandleARB program, GLint location, GLsizei bufSize, GLuint * params)
{
    (void) program; (void) location; (void) bufSize; (void) params;
   DISPATCH(GetnUniformuivARB, (program, location, bufSize, params), (F, "glGetnUniformuivARB(%d, %d, %d, %p);\n", program, location, bufSize, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ReadnPixelsARB)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, GLvoid * data)
{
    (void) x; (void) y; (void) width; (void) height; (void) format; (void) type; (void) bufSize; (void) data;
   DISPATCH(ReadnPixelsARB, (x, y, width, height, format, type, bufSize, data), (F, "glReadnPixelsARB(%d, %d, %d, %d, 0x%x, 0x%x, %d, %p);\n", x, y, width, height, format, type, bufSize, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(TexStorage1D)(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width)
{
    (void) target; (void) levels; (void) internalFormat; (void) width;
   DISPATCH(TexStorage1D, (target, levels, internalFormat, width), (F, "glTexStorage1D(0x%x, %d, 0x%x, %d);\n", target, levels, internalFormat, width));
}

KEYWORD1 void KEYWORD2 NAME(TexStorage2D)(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height)
{
    (void) target; (void) levels; (void) internalFormat; (void) width; (void) height;
   DISPATCH(TexStorage2D, (target, levels, internalFormat, width, height), (F, "glTexStorage2D(0x%x, %d, 0x%x, %d, %d);\n", target, levels, internalFormat, width, height));
}

KEYWORD1 void KEYWORD2 NAME(TexStorage3D)(GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth)
{
    (void) target; (void) levels; (void) internalFormat; (void) width; (void) height; (void) depth;
   DISPATCH(TexStorage3D, (target, levels, internalFormat, width, height, depth), (F, "glTexStorage3D(0x%x, %d, 0x%x, %d, %d, %d);\n", target, levels, internalFormat, width, height, depth));
}

KEYWORD1 void KEYWORD2 NAME(TextureStorage1DEXT)(GLuint texture, GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width)
{
    (void) texture; (void) target; (void) levels; (void) internalFormat; (void) width;
   DISPATCH(TextureStorage1DEXT, (texture, target, levels, internalFormat, width), (F, "glTextureStorage1DEXT(%d, 0x%x, %d, 0x%x, %d);\n", texture, target, levels, internalFormat, width));
}

KEYWORD1 void KEYWORD2 NAME(TextureStorage2DEXT)(GLuint texture, GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height)
{
    (void) texture; (void) target; (void) levels; (void) internalFormat; (void) width; (void) height;
   DISPATCH(TextureStorage2DEXT, (texture, target, levels, internalFormat, width, height), (F, "glTextureStorage2DEXT(%d, 0x%x, %d, 0x%x, %d, %d);\n", texture, target, levels, internalFormat, width, height));
}

KEYWORD1 void KEYWORD2 NAME(TextureStorage3DEXT)(GLuint texture, GLenum target, GLsizei levels, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth)
{
    (void) texture; (void) target; (void) levels; (void) internalFormat; (void) width; (void) height; (void) depth;
   DISPATCH(TextureStorage3DEXT, (texture, target, levels, internalFormat, width, height, depth), (F, "glTextureStorage3DEXT(%d, 0x%x, %d, 0x%x, %d, %d, %d);\n", texture, target, levels, internalFormat, width, height, depth));
}

KEYWORD1 void KEYWORD2 NAME(PolygonOffsetEXT)(GLfloat factor, GLfloat bias)
{
    (void) factor; (void) bias;
   DISPATCH(PolygonOffsetEXT, (factor, bias), (F, "glPolygonOffsetEXT(%f, %f);\n", factor, bias));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_692)(GLenum pname, GLfloat * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_692)(GLenum pname, GLfloat * params)
{
    (void) pname; (void) params;
   DISPATCH(GetPixelTexGenParameterfvSGIS, (pname, params), (F, "glGetPixelTexGenParameterfvSGIS(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_693)(GLenum pname, GLint * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_693)(GLenum pname, GLint * params)
{
    (void) pname; (void) params;
   DISPATCH(GetPixelTexGenParameterivSGIS, (pname, params), (F, "glGetPixelTexGenParameterivSGIS(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_694)(GLenum pname, GLfloat param);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_694)(GLenum pname, GLfloat param)
{
    (void) pname; (void) param;
   DISPATCH(PixelTexGenParameterfSGIS, (pname, param), (F, "glPixelTexGenParameterfSGIS(0x%x, %f);\n", pname, param));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_695)(GLenum pname, const GLfloat * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_695)(GLenum pname, const GLfloat * params)
{
    (void) pname; (void) params;
   DISPATCH(PixelTexGenParameterfvSGIS, (pname, params), (F, "glPixelTexGenParameterfvSGIS(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_696)(GLenum pname, GLint param);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_696)(GLenum pname, GLint param)
{
    (void) pname; (void) param;
   DISPATCH(PixelTexGenParameteriSGIS, (pname, param), (F, "glPixelTexGenParameteriSGIS(0x%x, %d);\n", pname, param));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_697)(GLenum pname, const GLint * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_697)(GLenum pname, const GLint * params)
{
    (void) pname; (void) params;
   DISPATCH(PixelTexGenParameterivSGIS, (pname, params), (F, "glPixelTexGenParameterivSGIS(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_698)(GLclampf value, GLboolean invert);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_698)(GLclampf value, GLboolean invert)
{
    (void) value; (void) invert;
   DISPATCH(SampleMaskSGIS, (value, invert), (F, "glSampleMaskSGIS(%f, %d);\n", value, invert));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_699)(GLenum pattern);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_699)(GLenum pattern)
{
    (void) pattern;
   DISPATCH(SamplePatternSGIS, (pattern), (F, "glSamplePatternSGIS(0x%x);\n", pattern));
}

KEYWORD1 void KEYWORD2 NAME(ColorPointerEXT)(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer)
{
    (void) size; (void) type; (void) stride; (void) count; (void) pointer;
   DISPATCH(ColorPointerEXT, (size, type, stride, count, pointer), (F, "glColorPointerEXT(%d, 0x%x, %d, %d, %p);\n", size, type, stride, count, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(EdgeFlagPointerEXT)(GLsizei stride, GLsizei count, const GLboolean * pointer)
{
    (void) stride; (void) count; (void) pointer;
   DISPATCH(EdgeFlagPointerEXT, (stride, count, pointer), (F, "glEdgeFlagPointerEXT(%d, %d, %p);\n", stride, count, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(IndexPointerEXT)(GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer)
{
    (void) type; (void) stride; (void) count; (void) pointer;
   DISPATCH(IndexPointerEXT, (type, stride, count, pointer), (F, "glIndexPointerEXT(0x%x, %d, %d, %p);\n", type, stride, count, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(NormalPointerEXT)(GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer)
{
    (void) type; (void) stride; (void) count; (void) pointer;
   DISPATCH(NormalPointerEXT, (type, stride, count, pointer), (F, "glNormalPointerEXT(0x%x, %d, %d, %p);\n", type, stride, count, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(TexCoordPointerEXT)(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer)
{
    (void) size; (void) type; (void) stride; (void) count; (void) pointer;
   DISPATCH(TexCoordPointerEXT, (size, type, stride, count, pointer), (F, "glTexCoordPointerEXT(%d, 0x%x, %d, %d, %p);\n", size, type, stride, count, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(VertexPointerEXT)(GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid * pointer)
{
    (void) size; (void) type; (void) stride; (void) count; (void) pointer;
   DISPATCH(VertexPointerEXT, (size, type, stride, count, pointer), (F, "glVertexPointerEXT(%d, 0x%x, %d, %d, %p);\n", size, type, stride, count, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(PointParameterf)(GLenum pname, GLfloat param)
{
    (void) pname; (void) param;
   DISPATCH(PointParameterfEXT, (pname, param), (F, "glPointParameterf(0x%x, %f);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(PointParameterfARB)(GLenum pname, GLfloat param)
{
    (void) pname; (void) param;
   DISPATCH(PointParameterfEXT, (pname, param), (F, "glPointParameterfARB(0x%x, %f);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(PointParameterfEXT)(GLenum pname, GLfloat param)
{
    (void) pname; (void) param;
   DISPATCH(PointParameterfEXT, (pname, param), (F, "glPointParameterfEXT(0x%x, %f);\n", pname, param));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_706)(GLenum pname, GLfloat param);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_706)(GLenum pname, GLfloat param)
{
    (void) pname; (void) param;
   DISPATCH(PointParameterfEXT, (pname, param), (F, "glPointParameterfSGIS(0x%x, %f);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(PointParameterfv)(GLenum pname, const GLfloat * params)
{
    (void) pname; (void) params;
   DISPATCH(PointParameterfvEXT, (pname, params), (F, "glPointParameterfv(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(PointParameterfvARB)(GLenum pname, const GLfloat * params)
{
    (void) pname; (void) params;
   DISPATCH(PointParameterfvEXT, (pname, params), (F, "glPointParameterfvARB(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(PointParameterfvEXT)(GLenum pname, const GLfloat * params)
{
    (void) pname; (void) params;
   DISPATCH(PointParameterfvEXT, (pname, params), (F, "glPointParameterfvEXT(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_707)(GLenum pname, const GLfloat * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_707)(GLenum pname, const GLfloat * params)
{
    (void) pname; (void) params;
   DISPATCH(PointParameterfvEXT, (pname, params), (F, "glPointParameterfvSGIS(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(LockArraysEXT)(GLint first, GLsizei count)
{
    (void) first; (void) count;
   DISPATCH(LockArraysEXT, (first, count), (F, "glLockArraysEXT(%d, %d);\n", first, count));
}

KEYWORD1 void KEYWORD2 NAME(UnlockArraysEXT)(void)
{
   DISPATCH(UnlockArraysEXT, (), (F, "glUnlockArraysEXT();\n"));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3b)(GLbyte red, GLbyte green, GLbyte blue)
{
    (void) red; (void) green; (void) blue;
   DISPATCH(SecondaryColor3bEXT, (red, green, blue), (F, "glSecondaryColor3b(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3bEXT)(GLbyte red, GLbyte green, GLbyte blue)
{
    (void) red; (void) green; (void) blue;
   DISPATCH(SecondaryColor3bEXT, (red, green, blue), (F, "glSecondaryColor3bEXT(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3bv)(const GLbyte * v)
{
    (void) v;
   DISPATCH(SecondaryColor3bvEXT, (v), (F, "glSecondaryColor3bv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3bvEXT)(const GLbyte * v)
{
    (void) v;
   DISPATCH(SecondaryColor3bvEXT, (v), (F, "glSecondaryColor3bvEXT(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3d)(GLdouble red, GLdouble green, GLdouble blue)
{
    (void) red; (void) green; (void) blue;
   DISPATCH(SecondaryColor3dEXT, (red, green, blue), (F, "glSecondaryColor3d(%f, %f, %f);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3dEXT)(GLdouble red, GLdouble green, GLdouble blue)
{
    (void) red; (void) green; (void) blue;
   DISPATCH(SecondaryColor3dEXT, (red, green, blue), (F, "glSecondaryColor3dEXT(%f, %f, %f);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3dv)(const GLdouble * v)
{
    (void) v;
   DISPATCH(SecondaryColor3dvEXT, (v), (F, "glSecondaryColor3dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3dvEXT)(const GLdouble * v)
{
    (void) v;
   DISPATCH(SecondaryColor3dvEXT, (v), (F, "glSecondaryColor3dvEXT(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3f)(GLfloat red, GLfloat green, GLfloat blue)
{
    (void) red; (void) green; (void) blue;
   DISPATCH(SecondaryColor3fEXT, (red, green, blue), (F, "glSecondaryColor3f(%f, %f, %f);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3fEXT)(GLfloat red, GLfloat green, GLfloat blue)
{
    (void) red; (void) green; (void) blue;
   DISPATCH(SecondaryColor3fEXT, (red, green, blue), (F, "glSecondaryColor3fEXT(%f, %f, %f);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3fv)(const GLfloat * v)
{
    (void) v;
   DISPATCH(SecondaryColor3fvEXT, (v), (F, "glSecondaryColor3fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3fvEXT)(const GLfloat * v)
{
    (void) v;
   DISPATCH(SecondaryColor3fvEXT, (v), (F, "glSecondaryColor3fvEXT(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3i)(GLint red, GLint green, GLint blue)
{
    (void) red; (void) green; (void) blue;
   DISPATCH(SecondaryColor3iEXT, (red, green, blue), (F, "glSecondaryColor3i(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3iEXT)(GLint red, GLint green, GLint blue)
{
    (void) red; (void) green; (void) blue;
   DISPATCH(SecondaryColor3iEXT, (red, green, blue), (F, "glSecondaryColor3iEXT(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3iv)(const GLint * v)
{
    (void) v;
   DISPATCH(SecondaryColor3ivEXT, (v), (F, "glSecondaryColor3iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3ivEXT)(const GLint * v)
{
    (void) v;
   DISPATCH(SecondaryColor3ivEXT, (v), (F, "glSecondaryColor3ivEXT(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3s)(GLshort red, GLshort green, GLshort blue)
{
    (void) red; (void) green; (void) blue;
   DISPATCH(SecondaryColor3sEXT, (red, green, blue), (F, "glSecondaryColor3s(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3sEXT)(GLshort red, GLshort green, GLshort blue)
{
    (void) red; (void) green; (void) blue;
   DISPATCH(SecondaryColor3sEXT, (red, green, blue), (F, "glSecondaryColor3sEXT(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3sv)(const GLshort * v)
{
    (void) v;
   DISPATCH(SecondaryColor3svEXT, (v), (F, "glSecondaryColor3sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3svEXT)(const GLshort * v)
{
    (void) v;
   DISPATCH(SecondaryColor3svEXT, (v), (F, "glSecondaryColor3svEXT(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3ub)(GLubyte red, GLubyte green, GLubyte blue)
{
    (void) red; (void) green; (void) blue;
   DISPATCH(SecondaryColor3ubEXT, (red, green, blue), (F, "glSecondaryColor3ub(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3ubEXT)(GLubyte red, GLubyte green, GLubyte blue)
{
    (void) red; (void) green; (void) blue;
   DISPATCH(SecondaryColor3ubEXT, (red, green, blue), (F, "glSecondaryColor3ubEXT(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3ubv)(const GLubyte * v)
{
    (void) v;
   DISPATCH(SecondaryColor3ubvEXT, (v), (F, "glSecondaryColor3ubv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3ubvEXT)(const GLubyte * v)
{
    (void) v;
   DISPATCH(SecondaryColor3ubvEXT, (v), (F, "glSecondaryColor3ubvEXT(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3ui)(GLuint red, GLuint green, GLuint blue)
{
    (void) red; (void) green; (void) blue;
   DISPATCH(SecondaryColor3uiEXT, (red, green, blue), (F, "glSecondaryColor3ui(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3uiEXT)(GLuint red, GLuint green, GLuint blue)
{
    (void) red; (void) green; (void) blue;
   DISPATCH(SecondaryColor3uiEXT, (red, green, blue), (F, "glSecondaryColor3uiEXT(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3uiv)(const GLuint * v)
{
    (void) v;
   DISPATCH(SecondaryColor3uivEXT, (v), (F, "glSecondaryColor3uiv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3uivEXT)(const GLuint * v)
{
    (void) v;
   DISPATCH(SecondaryColor3uivEXT, (v), (F, "glSecondaryColor3uivEXT(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3us)(GLushort red, GLushort green, GLushort blue)
{
    (void) red; (void) green; (void) blue;
   DISPATCH(SecondaryColor3usEXT, (red, green, blue), (F, "glSecondaryColor3us(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3usEXT)(GLushort red, GLushort green, GLushort blue)
{
    (void) red; (void) green; (void) blue;
   DISPATCH(SecondaryColor3usEXT, (red, green, blue), (F, "glSecondaryColor3usEXT(%d, %d, %d);\n", red, green, blue));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3usv)(const GLushort * v)
{
    (void) v;
   DISPATCH(SecondaryColor3usvEXT, (v), (F, "glSecondaryColor3usv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColor3usvEXT)(const GLushort * v)
{
    (void) v;
   DISPATCH(SecondaryColor3usvEXT, (v), (F, "glSecondaryColor3usvEXT(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColorPointer)(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
    (void) size; (void) type; (void) stride; (void) pointer;
   DISPATCH(SecondaryColorPointerEXT, (size, type, stride, pointer), (F, "glSecondaryColorPointer(%d, 0x%x, %d, %p);\n", size, type, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(SecondaryColorPointerEXT)(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
    (void) size; (void) type; (void) stride; (void) pointer;
   DISPATCH(SecondaryColorPointerEXT, (size, type, stride, pointer), (F, "glSecondaryColorPointerEXT(%d, 0x%x, %d, %p);\n", size, type, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(MultiDrawArrays)(GLenum mode, const GLint * first, const GLsizei * count, GLsizei primcount)
{
    (void) mode; (void) first; (void) count; (void) primcount;
   DISPATCH(MultiDrawArraysEXT, (mode, first, count, primcount), (F, "glMultiDrawArrays(0x%x, %p, %p, %d);\n", mode, (const void *) first, (const void *) count, primcount));
}

KEYWORD1 void KEYWORD2 NAME(MultiDrawArraysEXT)(GLenum mode, const GLint * first, const GLsizei * count, GLsizei primcount)
{
    (void) mode; (void) first; (void) count; (void) primcount;
   DISPATCH(MultiDrawArraysEXT, (mode, first, count, primcount), (F, "glMultiDrawArraysEXT(0x%x, %p, %p, %d);\n", mode, (const void *) first, (const void *) count, primcount));
}

KEYWORD1 void KEYWORD2 NAME(MultiDrawElements)(GLenum mode, const GLsizei * count, GLenum type, const GLvoid ** indices, GLsizei primcount)
{
    (void) mode; (void) count; (void) type; (void) indices; (void) primcount;
   DISPATCH(MultiDrawElementsEXT, (mode, count, type, indices, primcount), (F, "glMultiDrawElements(0x%x, %p, 0x%x, %p, %d);\n", mode, (const void *) count, type, (const void *) indices, primcount));
}

KEYWORD1 void KEYWORD2 NAME(MultiDrawElementsEXT)(GLenum mode, const GLsizei * count, GLenum type, const GLvoid ** indices, GLsizei primcount)
{
    (void) mode; (void) count; (void) type; (void) indices; (void) primcount;
   DISPATCH(MultiDrawElementsEXT, (mode, count, type, indices, primcount), (F, "glMultiDrawElementsEXT(0x%x, %p, 0x%x, %p, %d);\n", mode, (const void *) count, type, (const void *) indices, primcount));
}

KEYWORD1 void KEYWORD2 NAME(FogCoordPointer)(GLenum type, GLsizei stride, const GLvoid * pointer)
{
    (void) type; (void) stride; (void) pointer;
   DISPATCH(FogCoordPointerEXT, (type, stride, pointer), (F, "glFogCoordPointer(0x%x, %d, %p);\n", type, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(FogCoordPointerEXT)(GLenum type, GLsizei stride, const GLvoid * pointer)
{
    (void) type; (void) stride; (void) pointer;
   DISPATCH(FogCoordPointerEXT, (type, stride, pointer), (F, "glFogCoordPointerEXT(0x%x, %d, %p);\n", type, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(FogCoordd)(GLdouble coord)
{
    (void) coord;
   DISPATCH(FogCoorddEXT, (coord), (F, "glFogCoordd(%f);\n", coord));
}

KEYWORD1 void KEYWORD2 NAME(FogCoorddEXT)(GLdouble coord)
{
    (void) coord;
   DISPATCH(FogCoorddEXT, (coord), (F, "glFogCoorddEXT(%f);\n", coord));
}

KEYWORD1 void KEYWORD2 NAME(FogCoorddv)(const GLdouble * coord)
{
    (void) coord;
   DISPATCH(FogCoorddvEXT, (coord), (F, "glFogCoorddv(%p);\n", (const void *) coord));
}

KEYWORD1 void KEYWORD2 NAME(FogCoorddvEXT)(const GLdouble * coord)
{
    (void) coord;
   DISPATCH(FogCoorddvEXT, (coord), (F, "glFogCoorddvEXT(%p);\n", (const void *) coord));
}

KEYWORD1 void KEYWORD2 NAME(FogCoordf)(GLfloat coord)
{
    (void) coord;
   DISPATCH(FogCoordfEXT, (coord), (F, "glFogCoordf(%f);\n", coord));
}

KEYWORD1 void KEYWORD2 NAME(FogCoordfEXT)(GLfloat coord)
{
    (void) coord;
   DISPATCH(FogCoordfEXT, (coord), (F, "glFogCoordfEXT(%f);\n", coord));
}

KEYWORD1 void KEYWORD2 NAME(FogCoordfv)(const GLfloat * coord)
{
    (void) coord;
   DISPATCH(FogCoordfvEXT, (coord), (F, "glFogCoordfv(%p);\n", (const void *) coord));
}

KEYWORD1 void KEYWORD2 NAME(FogCoordfvEXT)(const GLfloat * coord)
{
    (void) coord;
   DISPATCH(FogCoordfvEXT, (coord), (F, "glFogCoordfvEXT(%p);\n", (const void *) coord));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_734)(GLenum mode);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_734)(GLenum mode)
{
    (void) mode;
   DISPATCH(PixelTexGenSGIX, (mode), (F, "glPixelTexGenSGIX(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(BlendFuncSeparate)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
{
    (void) sfactorRGB; (void) dfactorRGB; (void) sfactorAlpha; (void) dfactorAlpha;
   DISPATCH(BlendFuncSeparateEXT, (sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha), (F, "glBlendFuncSeparate(0x%x, 0x%x, 0x%x, 0x%x);\n", sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha));
}

KEYWORD1 void KEYWORD2 NAME(BlendFuncSeparateEXT)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
{
    (void) sfactorRGB; (void) dfactorRGB; (void) sfactorAlpha; (void) dfactorAlpha;
   DISPATCH(BlendFuncSeparateEXT, (sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha), (F, "glBlendFuncSeparateEXT(0x%x, 0x%x, 0x%x, 0x%x);\n", sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_735)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_735)(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
{
    (void) sfactorRGB; (void) dfactorRGB; (void) sfactorAlpha; (void) dfactorAlpha;
   DISPATCH(BlendFuncSeparateEXT, (sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha), (F, "glBlendFuncSeparateINGR(0x%x, 0x%x, 0x%x, 0x%x);\n", sfactorRGB, dfactorRGB, sfactorAlpha, dfactorAlpha));
}

KEYWORD1 void KEYWORD2 NAME(FlushVertexArrayRangeNV)(void)
{
   DISPATCH(FlushVertexArrayRangeNV, (), (F, "glFlushVertexArrayRangeNV();\n"));
}

KEYWORD1 void KEYWORD2 NAME(VertexArrayRangeNV)(GLsizei length, const GLvoid * pointer)
{
    (void) length; (void) pointer;
   DISPATCH(VertexArrayRangeNV, (length, pointer), (F, "glVertexArrayRangeNV(%d, %p);\n", length, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(CombinerInputNV)(GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage)
{
    (void) stage; (void) portion; (void) variable; (void) input; (void) mapping; (void) componentUsage;
   DISPATCH(CombinerInputNV, (stage, portion, variable, input, mapping, componentUsage), (F, "glCombinerInputNV(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x);\n", stage, portion, variable, input, mapping, componentUsage));
}

KEYWORD1 void KEYWORD2 NAME(CombinerOutputNV)(GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum)
{
    (void) stage; (void) portion; (void) abOutput; (void) cdOutput; (void) sumOutput; (void) scale; (void) bias; (void) abDotProduct; (void) cdDotProduct; (void) muxSum;
   DISPATCH(CombinerOutputNV, (stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum), (F, "glCombinerOutputNV(0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, 0x%x, %d, %d, %d);\n", stage, portion, abOutput, cdOutput, sumOutput, scale, bias, abDotProduct, cdDotProduct, muxSum));
}

KEYWORD1 void KEYWORD2 NAME(CombinerParameterfNV)(GLenum pname, GLfloat param)
{
    (void) pname; (void) param;
   DISPATCH(CombinerParameterfNV, (pname, param), (F, "glCombinerParameterfNV(0x%x, %f);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(CombinerParameterfvNV)(GLenum pname, const GLfloat * params)
{
    (void) pname; (void) params;
   DISPATCH(CombinerParameterfvNV, (pname, params), (F, "glCombinerParameterfvNV(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(CombinerParameteriNV)(GLenum pname, GLint param)
{
    (void) pname; (void) param;
   DISPATCH(CombinerParameteriNV, (pname, param), (F, "glCombinerParameteriNV(0x%x, %d);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(CombinerParameterivNV)(GLenum pname, const GLint * params)
{
    (void) pname; (void) params;
   DISPATCH(CombinerParameterivNV, (pname, params), (F, "glCombinerParameterivNV(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(FinalCombinerInputNV)(GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage)
{
    (void) variable; (void) input; (void) mapping; (void) componentUsage;
   DISPATCH(FinalCombinerInputNV, (variable, input, mapping, componentUsage), (F, "glFinalCombinerInputNV(0x%x, 0x%x, 0x%x, 0x%x);\n", variable, input, mapping, componentUsage));
}

KEYWORD1 void KEYWORD2 NAME(GetCombinerInputParameterfvNV)(GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat * params)
{
    (void) stage; (void) portion; (void) variable; (void) pname; (void) params;
   DISPATCH(GetCombinerInputParameterfvNV, (stage, portion, variable, pname, params), (F, "glGetCombinerInputParameterfvNV(0x%x, 0x%x, 0x%x, 0x%x, %p);\n", stage, portion, variable, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetCombinerInputParameterivNV)(GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint * params)
{
    (void) stage; (void) portion; (void) variable; (void) pname; (void) params;
   DISPATCH(GetCombinerInputParameterivNV, (stage, portion, variable, pname, params), (F, "glGetCombinerInputParameterivNV(0x%x, 0x%x, 0x%x, 0x%x, %p);\n", stage, portion, variable, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetCombinerOutputParameterfvNV)(GLenum stage, GLenum portion, GLenum pname, GLfloat * params)
{
    (void) stage; (void) portion; (void) pname; (void) params;
   DISPATCH(GetCombinerOutputParameterfvNV, (stage, portion, pname, params), (F, "glGetCombinerOutputParameterfvNV(0x%x, 0x%x, 0x%x, %p);\n", stage, portion, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetCombinerOutputParameterivNV)(GLenum stage, GLenum portion, GLenum pname, GLint * params)
{
    (void) stage; (void) portion; (void) pname; (void) params;
   DISPATCH(GetCombinerOutputParameterivNV, (stage, portion, pname, params), (F, "glGetCombinerOutputParameterivNV(0x%x, 0x%x, 0x%x, %p);\n", stage, portion, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetFinalCombinerInputParameterfvNV)(GLenum variable, GLenum pname, GLfloat * params)
{
    (void) variable; (void) pname; (void) params;
   DISPATCH(GetFinalCombinerInputParameterfvNV, (variable, pname, params), (F, "glGetFinalCombinerInputParameterfvNV(0x%x, 0x%x, %p);\n", variable, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetFinalCombinerInputParameterivNV)(GLenum variable, GLenum pname, GLint * params)
{
    (void) variable; (void) pname; (void) params;
   DISPATCH(GetFinalCombinerInputParameterivNV, (variable, pname, params), (F, "glGetFinalCombinerInputParameterivNV(0x%x, 0x%x, %p);\n", variable, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ResizeBuffersMESA)(void)
{
   DISPATCH(ResizeBuffersMESA, (), (F, "glResizeBuffersMESA();\n"));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2d)(GLdouble x, GLdouble y)
{
    (void) x; (void) y;
   DISPATCH(WindowPos2dMESA, (x, y), (F, "glWindowPos2d(%f, %f);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2dARB)(GLdouble x, GLdouble y)
{
    (void) x; (void) y;
   DISPATCH(WindowPos2dMESA, (x, y), (F, "glWindowPos2dARB(%f, %f);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2dMESA)(GLdouble x, GLdouble y)
{
    (void) x; (void) y;
   DISPATCH(WindowPos2dMESA, (x, y), (F, "glWindowPos2dMESA(%f, %f);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2dv)(const GLdouble * v)
{
    (void) v;
   DISPATCH(WindowPos2dvMESA, (v), (F, "glWindowPos2dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2dvARB)(const GLdouble * v)
{
    (void) v;
   DISPATCH(WindowPos2dvMESA, (v), (F, "glWindowPos2dvARB(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2dvMESA)(const GLdouble * v)
{
    (void) v;
   DISPATCH(WindowPos2dvMESA, (v), (F, "glWindowPos2dvMESA(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2f)(GLfloat x, GLfloat y)
{
    (void) x; (void) y;
   DISPATCH(WindowPos2fMESA, (x, y), (F, "glWindowPos2f(%f, %f);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2fARB)(GLfloat x, GLfloat y)
{
    (void) x; (void) y;
   DISPATCH(WindowPos2fMESA, (x, y), (F, "glWindowPos2fARB(%f, %f);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2fMESA)(GLfloat x, GLfloat y)
{
    (void) x; (void) y;
   DISPATCH(WindowPos2fMESA, (x, y), (F, "glWindowPos2fMESA(%f, %f);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2fv)(const GLfloat * v)
{
    (void) v;
   DISPATCH(WindowPos2fvMESA, (v), (F, "glWindowPos2fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2fvARB)(const GLfloat * v)
{
    (void) v;
   DISPATCH(WindowPos2fvMESA, (v), (F, "glWindowPos2fvARB(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2fvMESA)(const GLfloat * v)
{
    (void) v;
   DISPATCH(WindowPos2fvMESA, (v), (F, "glWindowPos2fvMESA(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2i)(GLint x, GLint y)
{
    (void) x; (void) y;
   DISPATCH(WindowPos2iMESA, (x, y), (F, "glWindowPos2i(%d, %d);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2iARB)(GLint x, GLint y)
{
    (void) x; (void) y;
   DISPATCH(WindowPos2iMESA, (x, y), (F, "glWindowPos2iARB(%d, %d);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2iMESA)(GLint x, GLint y)
{
    (void) x; (void) y;
   DISPATCH(WindowPos2iMESA, (x, y), (F, "glWindowPos2iMESA(%d, %d);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2iv)(const GLint * v)
{
    (void) v;
   DISPATCH(WindowPos2ivMESA, (v), (F, "glWindowPos2iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2ivARB)(const GLint * v)
{
    (void) v;
   DISPATCH(WindowPos2ivMESA, (v), (F, "glWindowPos2ivARB(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2ivMESA)(const GLint * v)
{
    (void) v;
   DISPATCH(WindowPos2ivMESA, (v), (F, "glWindowPos2ivMESA(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2s)(GLshort x, GLshort y)
{
    (void) x; (void) y;
   DISPATCH(WindowPos2sMESA, (x, y), (F, "glWindowPos2s(%d, %d);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2sARB)(GLshort x, GLshort y)
{
    (void) x; (void) y;
   DISPATCH(WindowPos2sMESA, (x, y), (F, "glWindowPos2sARB(%d, %d);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2sMESA)(GLshort x, GLshort y)
{
    (void) x; (void) y;
   DISPATCH(WindowPos2sMESA, (x, y), (F, "glWindowPos2sMESA(%d, %d);\n", x, y));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2sv)(const GLshort * v)
{
    (void) v;
   DISPATCH(WindowPos2svMESA, (v), (F, "glWindowPos2sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2svARB)(const GLshort * v)
{
    (void) v;
   DISPATCH(WindowPos2svMESA, (v), (F, "glWindowPos2svARB(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos2svMESA)(const GLshort * v)
{
    (void) v;
   DISPATCH(WindowPos2svMESA, (v), (F, "glWindowPos2svMESA(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3d)(GLdouble x, GLdouble y, GLdouble z)
{
    (void) x; (void) y; (void) z;
   DISPATCH(WindowPos3dMESA, (x, y, z), (F, "glWindowPos3d(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3dARB)(GLdouble x, GLdouble y, GLdouble z)
{
    (void) x; (void) y; (void) z;
   DISPATCH(WindowPos3dMESA, (x, y, z), (F, "glWindowPos3dARB(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3dMESA)(GLdouble x, GLdouble y, GLdouble z)
{
    (void) x; (void) y; (void) z;
   DISPATCH(WindowPos3dMESA, (x, y, z), (F, "glWindowPos3dMESA(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3dv)(const GLdouble * v)
{
    (void) v;
   DISPATCH(WindowPos3dvMESA, (v), (F, "glWindowPos3dv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3dvARB)(const GLdouble * v)
{
    (void) v;
   DISPATCH(WindowPos3dvMESA, (v), (F, "glWindowPos3dvARB(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3dvMESA)(const GLdouble * v)
{
    (void) v;
   DISPATCH(WindowPos3dvMESA, (v), (F, "glWindowPos3dvMESA(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3f)(GLfloat x, GLfloat y, GLfloat z)
{
    (void) x; (void) y; (void) z;
   DISPATCH(WindowPos3fMESA, (x, y, z), (F, "glWindowPos3f(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3fARB)(GLfloat x, GLfloat y, GLfloat z)
{
    (void) x; (void) y; (void) z;
   DISPATCH(WindowPos3fMESA, (x, y, z), (F, "glWindowPos3fARB(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3fMESA)(GLfloat x, GLfloat y, GLfloat z)
{
    (void) x; (void) y; (void) z;
   DISPATCH(WindowPos3fMESA, (x, y, z), (F, "glWindowPos3fMESA(%f, %f, %f);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3fv)(const GLfloat * v)
{
    (void) v;
   DISPATCH(WindowPos3fvMESA, (v), (F, "glWindowPos3fv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3fvARB)(const GLfloat * v)
{
    (void) v;
   DISPATCH(WindowPos3fvMESA, (v), (F, "glWindowPos3fvARB(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3fvMESA)(const GLfloat * v)
{
    (void) v;
   DISPATCH(WindowPos3fvMESA, (v), (F, "glWindowPos3fvMESA(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3i)(GLint x, GLint y, GLint z)
{
    (void) x; (void) y; (void) z;
   DISPATCH(WindowPos3iMESA, (x, y, z), (F, "glWindowPos3i(%d, %d, %d);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3iARB)(GLint x, GLint y, GLint z)
{
    (void) x; (void) y; (void) z;
   DISPATCH(WindowPos3iMESA, (x, y, z), (F, "glWindowPos3iARB(%d, %d, %d);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3iMESA)(GLint x, GLint y, GLint z)
{
    (void) x; (void) y; (void) z;
   DISPATCH(WindowPos3iMESA, (x, y, z), (F, "glWindowPos3iMESA(%d, %d, %d);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3iv)(const GLint * v)
{
    (void) v;
   DISPATCH(WindowPos3ivMESA, (v), (F, "glWindowPos3iv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3ivARB)(const GLint * v)
{
    (void) v;
   DISPATCH(WindowPos3ivMESA, (v), (F, "glWindowPos3ivARB(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3ivMESA)(const GLint * v)
{
    (void) v;
   DISPATCH(WindowPos3ivMESA, (v), (F, "glWindowPos3ivMESA(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3s)(GLshort x, GLshort y, GLshort z)
{
    (void) x; (void) y; (void) z;
   DISPATCH(WindowPos3sMESA, (x, y, z), (F, "glWindowPos3s(%d, %d, %d);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3sARB)(GLshort x, GLshort y, GLshort z)
{
    (void) x; (void) y; (void) z;
   DISPATCH(WindowPos3sMESA, (x, y, z), (F, "glWindowPos3sARB(%d, %d, %d);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3sMESA)(GLshort x, GLshort y, GLshort z)
{
    (void) x; (void) y; (void) z;
   DISPATCH(WindowPos3sMESA, (x, y, z), (F, "glWindowPos3sMESA(%d, %d, %d);\n", x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3sv)(const GLshort * v)
{
    (void) v;
   DISPATCH(WindowPos3svMESA, (v), (F, "glWindowPos3sv(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3svARB)(const GLshort * v)
{
    (void) v;
   DISPATCH(WindowPos3svMESA, (v), (F, "glWindowPos3svARB(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos3svMESA)(const GLshort * v)
{
    (void) v;
   DISPATCH(WindowPos3svMESA, (v), (F, "glWindowPos3svMESA(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4dMESA)(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    (void) x; (void) y; (void) z; (void) w;
   DISPATCH(WindowPos4dMESA, (x, y, z, w), (F, "glWindowPos4dMESA(%f, %f, %f, %f);\n", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4dvMESA)(const GLdouble * v)
{
    (void) v;
   DISPATCH(WindowPos4dvMESA, (v), (F, "glWindowPos4dvMESA(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4fMESA)(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    (void) x; (void) y; (void) z; (void) w;
   DISPATCH(WindowPos4fMESA, (x, y, z, w), (F, "glWindowPos4fMESA(%f, %f, %f, %f);\n", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4fvMESA)(const GLfloat * v)
{
    (void) v;
   DISPATCH(WindowPos4fvMESA, (v), (F, "glWindowPos4fvMESA(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4iMESA)(GLint x, GLint y, GLint z, GLint w)
{
    (void) x; (void) y; (void) z; (void) w;
   DISPATCH(WindowPos4iMESA, (x, y, z, w), (F, "glWindowPos4iMESA(%d, %d, %d, %d);\n", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4ivMESA)(const GLint * v)
{
    (void) v;
   DISPATCH(WindowPos4ivMESA, (v), (F, "glWindowPos4ivMESA(%p);\n", (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4sMESA)(GLshort x, GLshort y, GLshort z, GLshort w)
{
    (void) x; (void) y; (void) z; (void) w;
   DISPATCH(WindowPos4sMESA, (x, y, z, w), (F, "glWindowPos4sMESA(%d, %d, %d, %d);\n", x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(WindowPos4svMESA)(const GLshort * v)
{
    (void) v;
   DISPATCH(WindowPos4svMESA, (v), (F, "glWindowPos4svMESA(%p);\n", (const void *) v));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_776)(const GLenum * mode, const GLint * first, const GLsizei * count, GLsizei primcount, GLint modestride);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_776)(const GLenum * mode, const GLint * first, const GLsizei * count, GLsizei primcount, GLint modestride)
{
    (void) mode; (void) first; (void) count; (void) primcount; (void) modestride;
   DISPATCH(MultiModeDrawArraysIBM, (mode, first, count, primcount, modestride), (F, "glMultiModeDrawArraysIBM(%p, %p, %p, %d, %d);\n", (const void *) mode, (const void *) first, (const void *) count, primcount, modestride));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_777)(const GLenum * mode, const GLsizei * count, GLenum type, const GLvoid * const * indices, GLsizei primcount, GLint modestride);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_777)(const GLenum * mode, const GLsizei * count, GLenum type, const GLvoid * const * indices, GLsizei primcount, GLint modestride)
{
    (void) mode; (void) count; (void) type; (void) indices; (void) primcount; (void) modestride;
   DISPATCH(MultiModeDrawElementsIBM, (mode, count, type, indices, primcount, modestride), (F, "glMultiModeDrawElementsIBM(%p, %p, 0x%x, %p, %d, %d);\n", (const void *) mode, (const void *) count, type, (const void *) indices, primcount, modestride));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_778)(GLsizei n, const GLuint * fences);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_778)(GLsizei n, const GLuint * fences)
{
    (void) n; (void) fences;
   DISPATCH(DeleteFencesNV, (n, fences), (F, "glDeleteFencesNV(%d, %p);\n", n, (const void *) fences));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_779)(GLuint fence);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_779)(GLuint fence)
{
    (void) fence;
   DISPATCH(FinishFenceNV, (fence), (F, "glFinishFenceNV(%d);\n", fence));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_780)(GLsizei n, GLuint * fences);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_780)(GLsizei n, GLuint * fences)
{
    (void) n; (void) fences;
   DISPATCH(GenFencesNV, (n, fences), (F, "glGenFencesNV(%d, %p);\n", n, (const void *) fences));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_781)(GLuint fence, GLenum pname, GLint * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_781)(GLuint fence, GLenum pname, GLint * params)
{
    (void) fence; (void) pname; (void) params;
   DISPATCH(GetFenceivNV, (fence, pname, params), (F, "glGetFenceivNV(%d, 0x%x, %p);\n", fence, pname, (const void *) params));
}

KEYWORD1_ALT GLboolean KEYWORD2 NAME(_dispatch_stub_782)(GLuint fence);

KEYWORD1_ALT GLboolean KEYWORD2 NAME(_dispatch_stub_782)(GLuint fence)
{
    (void) fence;
   RETURN_DISPATCH(IsFenceNV, (fence), (F, "glIsFenceNV(%d);\n", fence));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_783)(GLuint fence, GLenum condition);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_783)(GLuint fence, GLenum condition)
{
    (void) fence; (void) condition;
   DISPATCH(SetFenceNV, (fence, condition), (F, "glSetFenceNV(%d, 0x%x);\n", fence, condition));
}

KEYWORD1_ALT GLboolean KEYWORD2 NAME(_dispatch_stub_784)(GLuint fence);

KEYWORD1_ALT GLboolean KEYWORD2 NAME(_dispatch_stub_784)(GLuint fence)
{
    (void) fence;
   RETURN_DISPATCH(TestFenceNV, (fence), (F, "glTestFenceNV(%d);\n", fence));
}

KEYWORD1 GLboolean KEYWORD2 NAME(AreProgramsResidentNV)(GLsizei n, const GLuint * ids, GLboolean * residences)
{
    (void) n; (void) ids; (void) residences;
   RETURN_DISPATCH(AreProgramsResidentNV, (n, ids, residences), (F, "glAreProgramsResidentNV(%d, %p, %p);\n", n, (const void *) ids, (const void *) residences));
}

KEYWORD1 void KEYWORD2 NAME(BindProgramARB)(GLenum target, GLuint program)
{
    (void) target; (void) program;
   DISPATCH(BindProgramNV, (target, program), (F, "glBindProgramARB(0x%x, %d);\n", target, program));
}

KEYWORD1 void KEYWORD2 NAME(BindProgramNV)(GLenum target, GLuint program)
{
    (void) target; (void) program;
   DISPATCH(BindProgramNV, (target, program), (F, "glBindProgramNV(0x%x, %d);\n", target, program));
}

KEYWORD1 void KEYWORD2 NAME(DeleteProgramsARB)(GLsizei n, const GLuint * programs)
{
    (void) n; (void) programs;
   DISPATCH(DeleteProgramsNV, (n, programs), (F, "glDeleteProgramsARB(%d, %p);\n", n, (const void *) programs));
}

KEYWORD1 void KEYWORD2 NAME(DeleteProgramsNV)(GLsizei n, const GLuint * programs)
{
    (void) n; (void) programs;
   DISPATCH(DeleteProgramsNV, (n, programs), (F, "glDeleteProgramsNV(%d, %p);\n", n, (const void *) programs));
}

KEYWORD1 void KEYWORD2 NAME(ExecuteProgramNV)(GLenum target, GLuint id, const GLfloat * params)
{
    (void) target; (void) id; (void) params;
   DISPATCH(ExecuteProgramNV, (target, id, params), (F, "glExecuteProgramNV(0x%x, %d, %p);\n", target, id, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GenProgramsARB)(GLsizei n, GLuint * programs)
{
    (void) n; (void) programs;
   DISPATCH(GenProgramsNV, (n, programs), (F, "glGenProgramsARB(%d, %p);\n", n, (const void *) programs));
}

KEYWORD1 void KEYWORD2 NAME(GenProgramsNV)(GLsizei n, GLuint * programs)
{
    (void) n; (void) programs;
   DISPATCH(GenProgramsNV, (n, programs), (F, "glGenProgramsNV(%d, %p);\n", n, (const void *) programs));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramParameterdvNV)(GLenum target, GLuint index, GLenum pname, GLdouble * params)
{
    (void) target; (void) index; (void) pname; (void) params;
   DISPATCH(GetProgramParameterdvNV, (target, index, pname, params), (F, "glGetProgramParameterdvNV(0x%x, %d, 0x%x, %p);\n", target, index, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramParameterfvNV)(GLenum target, GLuint index, GLenum pname, GLfloat * params)
{
    (void) target; (void) index; (void) pname; (void) params;
   DISPATCH(GetProgramParameterfvNV, (target, index, pname, params), (F, "glGetProgramParameterfvNV(0x%x, %d, 0x%x, %p);\n", target, index, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramStringNV)(GLuint id, GLenum pname, GLubyte * program)
{
    (void) id; (void) pname; (void) program;
   DISPATCH(GetProgramStringNV, (id, pname, program), (F, "glGetProgramStringNV(%d, 0x%x, %p);\n", id, pname, (const void *) program));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramivNV)(GLuint id, GLenum pname, GLint * params)
{
    (void) id; (void) pname; (void) params;
   DISPATCH(GetProgramivNV, (id, pname, params), (F, "glGetProgramivNV(%d, 0x%x, %p);\n", id, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetTrackMatrixivNV)(GLenum target, GLuint address, GLenum pname, GLint * params)
{
    (void) target; (void) address; (void) pname; (void) params;
   DISPATCH(GetTrackMatrixivNV, (target, address, pname, params), (F, "glGetTrackMatrixivNV(0x%x, %d, 0x%x, %p);\n", target, address, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribPointerv)(GLuint index, GLenum pname, GLvoid ** pointer)
{
    (void) index; (void) pname; (void) pointer;
   DISPATCH(GetVertexAttribPointervNV, (index, pname, pointer), (F, "glGetVertexAttribPointerv(%d, 0x%x, %p);\n", index, pname, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribPointervARB)(GLuint index, GLenum pname, GLvoid ** pointer)
{
    (void) index; (void) pname; (void) pointer;
   DISPATCH(GetVertexAttribPointervNV, (index, pname, pointer), (F, "glGetVertexAttribPointervARB(%d, 0x%x, %p);\n", index, pname, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribPointervNV)(GLuint index, GLenum pname, GLvoid ** pointer)
{
    (void) index; (void) pname; (void) pointer;
   DISPATCH(GetVertexAttribPointervNV, (index, pname, pointer), (F, "glGetVertexAttribPointervNV(%d, 0x%x, %p);\n", index, pname, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribdvNV)(GLuint index, GLenum pname, GLdouble * params)
{
    (void) index; (void) pname; (void) params;
   DISPATCH(GetVertexAttribdvNV, (index, pname, params), (F, "glGetVertexAttribdvNV(%d, 0x%x, %p);\n", index, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribfvNV)(GLuint index, GLenum pname, GLfloat * params)
{
    (void) index; (void) pname; (void) params;
   DISPATCH(GetVertexAttribfvNV, (index, pname, params), (F, "glGetVertexAttribfvNV(%d, 0x%x, %p);\n", index, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribivNV)(GLuint index, GLenum pname, GLint * params)
{
    (void) index; (void) pname; (void) params;
   DISPATCH(GetVertexAttribivNV, (index, pname, params), (F, "glGetVertexAttribivNV(%d, 0x%x, %p);\n", index, pname, (const void *) params));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsProgramARB)(GLuint program)
{
    (void) program;
   RETURN_DISPATCH(IsProgramNV, (program), (F, "glIsProgramARB(%d);\n", program));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsProgramNV)(GLuint program)
{
    (void) program;
   RETURN_DISPATCH(IsProgramNV, (program), (F, "glIsProgramNV(%d);\n", program));
}

KEYWORD1 void KEYWORD2 NAME(LoadProgramNV)(GLenum target, GLuint id, GLsizei len, const GLubyte * program)
{
    (void) target; (void) id; (void) len; (void) program;
   DISPATCH(LoadProgramNV, (target, id, len, program), (F, "glLoadProgramNV(0x%x, %d, %d, %p);\n", target, id, len, (const void *) program));
}

KEYWORD1 void KEYWORD2 NAME(ProgramParameters4dvNV)(GLenum target, GLuint index, GLsizei num, const GLdouble * params)
{
    (void) target; (void) index; (void) num; (void) params;
   DISPATCH(ProgramParameters4dvNV, (target, index, num, params), (F, "glProgramParameters4dvNV(0x%x, %d, %d, %p);\n", target, index, num, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ProgramParameters4fvNV)(GLenum target, GLuint index, GLsizei num, const GLfloat * params)
{
    (void) target; (void) index; (void) num; (void) params;
   DISPATCH(ProgramParameters4fvNV, (target, index, num, params), (F, "glProgramParameters4fvNV(0x%x, %d, %d, %p);\n", target, index, num, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(RequestResidentProgramsNV)(GLsizei n, const GLuint * ids)
{
    (void) n; (void) ids;
   DISPATCH(RequestResidentProgramsNV, (n, ids), (F, "glRequestResidentProgramsNV(%d, %p);\n", n, (const void *) ids));
}

KEYWORD1 void KEYWORD2 NAME(TrackMatrixNV)(GLenum target, GLuint address, GLenum matrix, GLenum transform)
{
    (void) target; (void) address; (void) matrix; (void) transform;
   DISPATCH(TrackMatrixNV, (target, address, matrix, transform), (F, "glTrackMatrixNV(0x%x, %d, 0x%x, 0x%x);\n", target, address, matrix, transform));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1dNV)(GLuint index, GLdouble x)
{
    (void) index; (void) x;
   DISPATCH(VertexAttrib1dNV, (index, x), (F, "glVertexAttrib1dNV(%d, %f);\n", index, x));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1dvNV)(GLuint index, const GLdouble * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib1dvNV, (index, v), (F, "glVertexAttrib1dvNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1fNV)(GLuint index, GLfloat x)
{
    (void) index; (void) x;
   DISPATCH(VertexAttrib1fNV, (index, x), (F, "glVertexAttrib1fNV(%d, %f);\n", index, x));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1fvNV)(GLuint index, const GLfloat * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib1fvNV, (index, v), (F, "glVertexAttrib1fvNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1sNV)(GLuint index, GLshort x)
{
    (void) index; (void) x;
   DISPATCH(VertexAttrib1sNV, (index, x), (F, "glVertexAttrib1sNV(%d, %d);\n", index, x));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib1svNV)(GLuint index, const GLshort * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib1svNV, (index, v), (F, "glVertexAttrib1svNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2dNV)(GLuint index, GLdouble x, GLdouble y)
{
    (void) index; (void) x; (void) y;
   DISPATCH(VertexAttrib2dNV, (index, x, y), (F, "glVertexAttrib2dNV(%d, %f, %f);\n", index, x, y));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2dvNV)(GLuint index, const GLdouble * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib2dvNV, (index, v), (F, "glVertexAttrib2dvNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2fNV)(GLuint index, GLfloat x, GLfloat y)
{
    (void) index; (void) x; (void) y;
   DISPATCH(VertexAttrib2fNV, (index, x, y), (F, "glVertexAttrib2fNV(%d, %f, %f);\n", index, x, y));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2fvNV)(GLuint index, const GLfloat * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib2fvNV, (index, v), (F, "glVertexAttrib2fvNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2sNV)(GLuint index, GLshort x, GLshort y)
{
    (void) index; (void) x; (void) y;
   DISPATCH(VertexAttrib2sNV, (index, x, y), (F, "glVertexAttrib2sNV(%d, %d, %d);\n", index, x, y));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib2svNV)(GLuint index, const GLshort * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib2svNV, (index, v), (F, "glVertexAttrib2svNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3dNV)(GLuint index, GLdouble x, GLdouble y, GLdouble z)
{
    (void) index; (void) x; (void) y; (void) z;
   DISPATCH(VertexAttrib3dNV, (index, x, y, z), (F, "glVertexAttrib3dNV(%d, %f, %f, %f);\n", index, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3dvNV)(GLuint index, const GLdouble * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib3dvNV, (index, v), (F, "glVertexAttrib3dvNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3fNV)(GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
    (void) index; (void) x; (void) y; (void) z;
   DISPATCH(VertexAttrib3fNV, (index, x, y, z), (F, "glVertexAttrib3fNV(%d, %f, %f, %f);\n", index, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3fvNV)(GLuint index, const GLfloat * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib3fvNV, (index, v), (F, "glVertexAttrib3fvNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3sNV)(GLuint index, GLshort x, GLshort y, GLshort z)
{
    (void) index; (void) x; (void) y; (void) z;
   DISPATCH(VertexAttrib3sNV, (index, x, y, z), (F, "glVertexAttrib3sNV(%d, %d, %d, %d);\n", index, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib3svNV)(GLuint index, const GLshort * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib3svNV, (index, v), (F, "glVertexAttrib3svNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4dNV)(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    (void) index; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(VertexAttrib4dNV, (index, x, y, z, w), (F, "glVertexAttrib4dNV(%d, %f, %f, %f, %f);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4dvNV)(GLuint index, const GLdouble * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4dvNV, (index, v), (F, "glVertexAttrib4dvNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4fNV)(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    (void) index; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(VertexAttrib4fNV, (index, x, y, z, w), (F, "glVertexAttrib4fNV(%d, %f, %f, %f, %f);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4fvNV)(GLuint index, const GLfloat * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4fvNV, (index, v), (F, "glVertexAttrib4fvNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4sNV)(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w)
{
    (void) index; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(VertexAttrib4sNV, (index, x, y, z, w), (F, "glVertexAttrib4sNV(%d, %d, %d, %d, %d);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4svNV)(GLuint index, const GLshort * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4svNV, (index, v), (F, "glVertexAttrib4svNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4ubNV)(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w)
{
    (void) index; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(VertexAttrib4ubNV, (index, x, y, z, w), (F, "glVertexAttrib4ubNV(%d, %d, %d, %d, %d);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttrib4ubvNV)(GLuint index, const GLubyte * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttrib4ubvNV, (index, v), (F, "glVertexAttrib4ubvNV(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribPointerNV)(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
    (void) index; (void) size; (void) type; (void) stride; (void) pointer;
   DISPATCH(VertexAttribPointerNV, (index, size, type, stride, pointer), (F, "glVertexAttribPointerNV(%d, %d, 0x%x, %d, %p);\n", index, size, type, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs1dvNV)(GLuint index, GLsizei n, const GLdouble * v)
{
    (void) index; (void) n; (void) v;
   DISPATCH(VertexAttribs1dvNV, (index, n, v), (F, "glVertexAttribs1dvNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs1fvNV)(GLuint index, GLsizei n, const GLfloat * v)
{
    (void) index; (void) n; (void) v;
   DISPATCH(VertexAttribs1fvNV, (index, n, v), (F, "glVertexAttribs1fvNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs1svNV)(GLuint index, GLsizei n, const GLshort * v)
{
    (void) index; (void) n; (void) v;
   DISPATCH(VertexAttribs1svNV, (index, n, v), (F, "glVertexAttribs1svNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs2dvNV)(GLuint index, GLsizei n, const GLdouble * v)
{
    (void) index; (void) n; (void) v;
   DISPATCH(VertexAttribs2dvNV, (index, n, v), (F, "glVertexAttribs2dvNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs2fvNV)(GLuint index, GLsizei n, const GLfloat * v)
{
    (void) index; (void) n; (void) v;
   DISPATCH(VertexAttribs2fvNV, (index, n, v), (F, "glVertexAttribs2fvNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs2svNV)(GLuint index, GLsizei n, const GLshort * v)
{
    (void) index; (void) n; (void) v;
   DISPATCH(VertexAttribs2svNV, (index, n, v), (F, "glVertexAttribs2svNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs3dvNV)(GLuint index, GLsizei n, const GLdouble * v)
{
    (void) index; (void) n; (void) v;
   DISPATCH(VertexAttribs3dvNV, (index, n, v), (F, "glVertexAttribs3dvNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs3fvNV)(GLuint index, GLsizei n, const GLfloat * v)
{
    (void) index; (void) n; (void) v;
   DISPATCH(VertexAttribs3fvNV, (index, n, v), (F, "glVertexAttribs3fvNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs3svNV)(GLuint index, GLsizei n, const GLshort * v)
{
    (void) index; (void) n; (void) v;
   DISPATCH(VertexAttribs3svNV, (index, n, v), (F, "glVertexAttribs3svNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs4dvNV)(GLuint index, GLsizei n, const GLdouble * v)
{
    (void) index; (void) n; (void) v;
   DISPATCH(VertexAttribs4dvNV, (index, n, v), (F, "glVertexAttribs4dvNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs4fvNV)(GLuint index, GLsizei n, const GLfloat * v)
{
    (void) index; (void) n; (void) v;
   DISPATCH(VertexAttribs4fvNV, (index, n, v), (F, "glVertexAttribs4fvNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs4svNV)(GLuint index, GLsizei n, const GLshort * v)
{
    (void) index; (void) n; (void) v;
   DISPATCH(VertexAttribs4svNV, (index, n, v), (F, "glVertexAttribs4svNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribs4ubvNV)(GLuint index, GLsizei n, const GLubyte * v)
{
    (void) index; (void) n; (void) v;
   DISPATCH(VertexAttribs4ubvNV, (index, n, v), (F, "glVertexAttribs4ubvNV(%d, %d, %p);\n", index, n, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(GetTexBumpParameterfvATI)(GLenum pname, GLfloat * param)
{
    (void) pname; (void) param;
   DISPATCH(GetTexBumpParameterfvATI, (pname, param), (F, "glGetTexBumpParameterfvATI(0x%x, %p);\n", pname, (const void *) param));
}

KEYWORD1 void KEYWORD2 NAME(GetTexBumpParameterivATI)(GLenum pname, GLint * param)
{
    (void) pname; (void) param;
   DISPATCH(GetTexBumpParameterivATI, (pname, param), (F, "glGetTexBumpParameterivATI(0x%x, %p);\n", pname, (const void *) param));
}

KEYWORD1 void KEYWORD2 NAME(TexBumpParameterfvATI)(GLenum pname, const GLfloat * param)
{
    (void) pname; (void) param;
   DISPATCH(TexBumpParameterfvATI, (pname, param), (F, "glTexBumpParameterfvATI(0x%x, %p);\n", pname, (const void *) param));
}

KEYWORD1 void KEYWORD2 NAME(TexBumpParameterivATI)(GLenum pname, const GLint * param)
{
    (void) pname; (void) param;
   DISPATCH(TexBumpParameterivATI, (pname, param), (F, "glTexBumpParameterivATI(0x%x, %p);\n", pname, (const void *) param));
}

KEYWORD1 void KEYWORD2 NAME(AlphaFragmentOp1ATI)(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod)
{
    (void) op; (void) dst; (void) dstMod; (void) arg1; (void) arg1Rep; (void) arg1Mod;
   DISPATCH(AlphaFragmentOp1ATI, (op, dst, dstMod, arg1, arg1Rep, arg1Mod), (F, "glAlphaFragmentOp1ATI(0x%x, %d, %d, %d, %d, %d);\n", op, dst, dstMod, arg1, arg1Rep, arg1Mod));
}

KEYWORD1 void KEYWORD2 NAME(AlphaFragmentOp2ATI)(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod)
{
    (void) op; (void) dst; (void) dstMod; (void) arg1; (void) arg1Rep; (void) arg1Mod; (void) arg2; (void) arg2Rep; (void) arg2Mod;
   DISPATCH(AlphaFragmentOp2ATI, (op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod), (F, "glAlphaFragmentOp2ATI(0x%x, %d, %d, %d, %d, %d, %d, %d, %d);\n", op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod));
}

KEYWORD1 void KEYWORD2 NAME(AlphaFragmentOp3ATI)(GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod)
{
    (void) op; (void) dst; (void) dstMod; (void) arg1; (void) arg1Rep; (void) arg1Mod; (void) arg2; (void) arg2Rep; (void) arg2Mod; (void) arg3; (void) arg3Rep; (void) arg3Mod;
   DISPATCH(AlphaFragmentOp3ATI, (op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod), (F, "glAlphaFragmentOp3ATI(0x%x, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d);\n", op, dst, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod));
}

KEYWORD1 void KEYWORD2 NAME(BeginFragmentShaderATI)(void)
{
   DISPATCH(BeginFragmentShaderATI, (), (F, "glBeginFragmentShaderATI();\n"));
}

KEYWORD1 void KEYWORD2 NAME(BindFragmentShaderATI)(GLuint id)
{
    (void) id;
   DISPATCH(BindFragmentShaderATI, (id), (F, "glBindFragmentShaderATI(%d);\n", id));
}

KEYWORD1 void KEYWORD2 NAME(ColorFragmentOp1ATI)(GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod)
{
    (void) op; (void) dst; (void) dstMask; (void) dstMod; (void) arg1; (void) arg1Rep; (void) arg1Mod;
   DISPATCH(ColorFragmentOp1ATI, (op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod), (F, "glColorFragmentOp1ATI(0x%x, %d, %d, %d, %d, %d, %d);\n", op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod));
}

KEYWORD1 void KEYWORD2 NAME(ColorFragmentOp2ATI)(GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod)
{
    (void) op; (void) dst; (void) dstMask; (void) dstMod; (void) arg1; (void) arg1Rep; (void) arg1Mod; (void) arg2; (void) arg2Rep; (void) arg2Mod;
   DISPATCH(ColorFragmentOp2ATI, (op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod), (F, "glColorFragmentOp2ATI(0x%x, %d, %d, %d, %d, %d, %d, %d, %d, %d);\n", op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod));
}

KEYWORD1 void KEYWORD2 NAME(ColorFragmentOp3ATI)(GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod)
{
    (void) op; (void) dst; (void) dstMask; (void) dstMod; (void) arg1; (void) arg1Rep; (void) arg1Mod; (void) arg2; (void) arg2Rep; (void) arg2Mod; (void) arg3; (void) arg3Rep; (void) arg3Mod;
   DISPATCH(ColorFragmentOp3ATI, (op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod), (F, "glColorFragmentOp3ATI(0x%x, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d);\n", op, dst, dstMask, dstMod, arg1, arg1Rep, arg1Mod, arg2, arg2Rep, arg2Mod, arg3, arg3Rep, arg3Mod));
}

KEYWORD1 void KEYWORD2 NAME(DeleteFragmentShaderATI)(GLuint id)
{
    (void) id;
   DISPATCH(DeleteFragmentShaderATI, (id), (F, "glDeleteFragmentShaderATI(%d);\n", id));
}

KEYWORD1 void KEYWORD2 NAME(EndFragmentShaderATI)(void)
{
   DISPATCH(EndFragmentShaderATI, (), (F, "glEndFragmentShaderATI();\n"));
}

KEYWORD1 GLuint KEYWORD2 NAME(GenFragmentShadersATI)(GLuint range)
{
    (void) range;
   RETURN_DISPATCH(GenFragmentShadersATI, (range), (F, "glGenFragmentShadersATI(%d);\n", range));
}

KEYWORD1 void KEYWORD2 NAME(PassTexCoordATI)(GLuint dst, GLuint coord, GLenum swizzle)
{
    (void) dst; (void) coord; (void) swizzle;
   DISPATCH(PassTexCoordATI, (dst, coord, swizzle), (F, "glPassTexCoordATI(%d, %d, 0x%x);\n", dst, coord, swizzle));
}

KEYWORD1 void KEYWORD2 NAME(SampleMapATI)(GLuint dst, GLuint interp, GLenum swizzle)
{
    (void) dst; (void) interp; (void) swizzle;
   DISPATCH(SampleMapATI, (dst, interp, swizzle), (F, "glSampleMapATI(%d, %d, 0x%x);\n", dst, interp, swizzle));
}

KEYWORD1 void KEYWORD2 NAME(SetFragmentShaderConstantATI)(GLuint dst, const GLfloat * value)
{
    (void) dst; (void) value;
   DISPATCH(SetFragmentShaderConstantATI, (dst, value), (F, "glSetFragmentShaderConstantATI(%d, %p);\n", dst, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(PointParameteri)(GLenum pname, GLint param)
{
    (void) pname; (void) param;
   DISPATCH(PointParameteriNV, (pname, param), (F, "glPointParameteri(0x%x, %d);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(PointParameteriNV)(GLenum pname, GLint param)
{
    (void) pname; (void) param;
   DISPATCH(PointParameteriNV, (pname, param), (F, "glPointParameteriNV(0x%x, %d);\n", pname, param));
}

KEYWORD1 void KEYWORD2 NAME(PointParameteriv)(GLenum pname, const GLint * params)
{
    (void) pname; (void) params;
   DISPATCH(PointParameterivNV, (pname, params), (F, "glPointParameteriv(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(PointParameterivNV)(GLenum pname, const GLint * params)
{
    (void) pname; (void) params;
   DISPATCH(PointParameterivNV, (pname, params), (F, "glPointParameterivNV(0x%x, %p);\n", pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_865)(GLenum face);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_865)(GLenum face)
{
    (void) face;
   DISPATCH(ActiveStencilFaceEXT, (face), (F, "glActiveStencilFaceEXT(0x%x);\n", face));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_866)(GLuint array);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_866)(GLuint array)
{
    (void) array;
   DISPATCH(BindVertexArrayAPPLE, (array), (F, "glBindVertexArrayAPPLE(%d);\n", array));
}

KEYWORD1 void KEYWORD2 NAME(DeleteVertexArrays)(GLsizei n, const GLuint * arrays)
{
    (void) n; (void) arrays;
   DISPATCH(DeleteVertexArraysAPPLE, (n, arrays), (F, "glDeleteVertexArrays(%d, %p);\n", n, (const void *) arrays));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_867)(GLsizei n, const GLuint * arrays);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_867)(GLsizei n, const GLuint * arrays)
{
    (void) n; (void) arrays;
   DISPATCH(DeleteVertexArraysAPPLE, (n, arrays), (F, "glDeleteVertexArraysAPPLE(%d, %p);\n", n, (const void *) arrays));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_868)(GLsizei n, GLuint * arrays);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_868)(GLsizei n, GLuint * arrays)
{
    (void) n; (void) arrays;
   DISPATCH(GenVertexArraysAPPLE, (n, arrays), (F, "glGenVertexArraysAPPLE(%d, %p);\n", n, (const void *) arrays));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsVertexArray)(GLuint array)
{
    (void) array;
   RETURN_DISPATCH(IsVertexArrayAPPLE, (array), (F, "glIsVertexArray(%d);\n", array));
}

KEYWORD1_ALT GLboolean KEYWORD2 NAME(_dispatch_stub_869)(GLuint array);

KEYWORD1_ALT GLboolean KEYWORD2 NAME(_dispatch_stub_869)(GLuint array)
{
    (void) array;
   RETURN_DISPATCH(IsVertexArrayAPPLE, (array), (F, "glIsVertexArrayAPPLE(%d);\n", array));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramNamedParameterdvNV)(GLuint id, GLsizei len, const GLubyte * name, GLdouble * params)
{
    (void) id; (void) len; (void) name; (void) params;
   DISPATCH(GetProgramNamedParameterdvNV, (id, len, name, params), (F, "glGetProgramNamedParameterdvNV(%d, %d, %p, %p);\n", id, len, (const void *) name, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetProgramNamedParameterfvNV)(GLuint id, GLsizei len, const GLubyte * name, GLfloat * params)
{
    (void) id; (void) len; (void) name; (void) params;
   DISPATCH(GetProgramNamedParameterfvNV, (id, len, name, params), (F, "glGetProgramNamedParameterfvNV(%d, %d, %p, %p);\n", id, len, (const void *) name, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(ProgramNamedParameter4dNV)(GLuint id, GLsizei len, const GLubyte * name, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    (void) id; (void) len; (void) name; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(ProgramNamedParameter4dNV, (id, len, name, x, y, z, w), (F, "glProgramNamedParameter4dNV(%d, %d, %p, %f, %f, %f, %f);\n", id, len, (const void *) name, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(ProgramNamedParameter4dvNV)(GLuint id, GLsizei len, const GLubyte * name, const GLdouble * v)
{
    (void) id; (void) len; (void) name; (void) v;
   DISPATCH(ProgramNamedParameter4dvNV, (id, len, name, v), (F, "glProgramNamedParameter4dvNV(%d, %d, %p, %p);\n", id, len, (const void *) name, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(ProgramNamedParameter4fNV)(GLuint id, GLsizei len, const GLubyte * name, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    (void) id; (void) len; (void) name; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(ProgramNamedParameter4fNV, (id, len, name, x, y, z, w), (F, "glProgramNamedParameter4fNV(%d, %d, %p, %f, %f, %f, %f);\n", id, len, (const void *) name, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(ProgramNamedParameter4fvNV)(GLuint id, GLsizei len, const GLubyte * name, const GLfloat * v)
{
    (void) id; (void) len; (void) name; (void) v;
   DISPATCH(ProgramNamedParameter4fvNV, (id, len, name, v), (F, "glProgramNamedParameter4fvNV(%d, %d, %p, %p);\n", id, len, (const void *) name, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(PrimitiveRestartIndexNV)(GLuint index)
{
    (void) index;
   DISPATCH(PrimitiveRestartIndexNV, (index), (F, "glPrimitiveRestartIndexNV(%d);\n", index));
}

KEYWORD1 void KEYWORD2 NAME(PrimitiveRestartIndex)(GLuint index)
{
    (void) index;
   DISPATCH(PrimitiveRestartIndexNV, (index), (F, "glPrimitiveRestartIndex(%d);\n", index));
}

KEYWORD1 void KEYWORD2 NAME(PrimitiveRestartNV)(void)
{
   DISPATCH(PrimitiveRestartNV, (), (F, "glPrimitiveRestartNV();\n"));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_878)(GLclampd zmin, GLclampd zmax);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_878)(GLclampd zmin, GLclampd zmax)
{
    (void) zmin; (void) zmax;
   DISPATCH(DepthBoundsEXT, (zmin, zmax), (F, "glDepthBoundsEXT(%f, %f);\n", zmin, zmax));
}

KEYWORD1 void KEYWORD2 NAME(BlendEquationSeparate)(GLenum modeRGB, GLenum modeA)
{
    (void) modeRGB; (void) modeA;
   DISPATCH(BlendEquationSeparateEXT, (modeRGB, modeA), (F, "glBlendEquationSeparate(0x%x, 0x%x);\n", modeRGB, modeA));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_879)(GLenum modeRGB, GLenum modeA);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_879)(GLenum modeRGB, GLenum modeA)
{
    (void) modeRGB; (void) modeA;
   DISPATCH(BlendEquationSeparateEXT, (modeRGB, modeA), (F, "glBlendEquationSeparateEXT(0x%x, 0x%x);\n", modeRGB, modeA));
}

KEYWORD1 void KEYWORD2 NAME(BindFramebuffer)(GLenum target, GLuint framebuffer)
{
    (void) target; (void) framebuffer;
   DISPATCH(BindFramebufferEXT, (target, framebuffer), (F, "glBindFramebuffer(0x%x, %d);\n", target, framebuffer));
}

KEYWORD1 void KEYWORD2 NAME(BindFramebufferEXT)(GLenum target, GLuint framebuffer)
{
    (void) target; (void) framebuffer;
   DISPATCH(BindFramebufferEXT, (target, framebuffer), (F, "glBindFramebufferEXT(0x%x, %d);\n", target, framebuffer));
}

KEYWORD1 void KEYWORD2 NAME(BindRenderbuffer)(GLenum target, GLuint renderbuffer)
{
    (void) target; (void) renderbuffer;
   DISPATCH(BindRenderbufferEXT, (target, renderbuffer), (F, "glBindRenderbuffer(0x%x, %d);\n", target, renderbuffer));
}

KEYWORD1 void KEYWORD2 NAME(BindRenderbufferEXT)(GLenum target, GLuint renderbuffer)
{
    (void) target; (void) renderbuffer;
   DISPATCH(BindRenderbufferEXT, (target, renderbuffer), (F, "glBindRenderbufferEXT(0x%x, %d);\n", target, renderbuffer));
}

KEYWORD1 GLenum KEYWORD2 NAME(CheckFramebufferStatus)(GLenum target)
{
    (void) target;
   RETURN_DISPATCH(CheckFramebufferStatusEXT, (target), (F, "glCheckFramebufferStatus(0x%x);\n", target));
}

KEYWORD1 GLenum KEYWORD2 NAME(CheckFramebufferStatusEXT)(GLenum target)
{
    (void) target;
   RETURN_DISPATCH(CheckFramebufferStatusEXT, (target), (F, "glCheckFramebufferStatusEXT(0x%x);\n", target));
}

KEYWORD1 void KEYWORD2 NAME(DeleteFramebuffers)(GLsizei n, const GLuint * framebuffers)
{
    (void) n; (void) framebuffers;
   DISPATCH(DeleteFramebuffersEXT, (n, framebuffers), (F, "glDeleteFramebuffers(%d, %p);\n", n, (const void *) framebuffers));
}

KEYWORD1 void KEYWORD2 NAME(DeleteFramebuffersEXT)(GLsizei n, const GLuint * framebuffers)
{
    (void) n; (void) framebuffers;
   DISPATCH(DeleteFramebuffersEXT, (n, framebuffers), (F, "glDeleteFramebuffersEXT(%d, %p);\n", n, (const void *) framebuffers));
}

KEYWORD1 void KEYWORD2 NAME(DeleteRenderbuffers)(GLsizei n, const GLuint * renderbuffers)
{
    (void) n; (void) renderbuffers;
   DISPATCH(DeleteRenderbuffersEXT, (n, renderbuffers), (F, "glDeleteRenderbuffers(%d, %p);\n", n, (const void *) renderbuffers));
}

KEYWORD1 void KEYWORD2 NAME(DeleteRenderbuffersEXT)(GLsizei n, const GLuint * renderbuffers)
{
    (void) n; (void) renderbuffers;
   DISPATCH(DeleteRenderbuffersEXT, (n, renderbuffers), (F, "glDeleteRenderbuffersEXT(%d, %p);\n", n, (const void *) renderbuffers));
}

KEYWORD1 void KEYWORD2 NAME(FramebufferRenderbuffer)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
    (void) target; (void) attachment; (void) renderbuffertarget; (void) renderbuffer;
   DISPATCH(FramebufferRenderbufferEXT, (target, attachment, renderbuffertarget, renderbuffer), (F, "glFramebufferRenderbuffer(0x%x, 0x%x, 0x%x, %d);\n", target, attachment, renderbuffertarget, renderbuffer));
}

KEYWORD1 void KEYWORD2 NAME(FramebufferRenderbufferEXT)(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
    (void) target; (void) attachment; (void) renderbuffertarget; (void) renderbuffer;
   DISPATCH(FramebufferRenderbufferEXT, (target, attachment, renderbuffertarget, renderbuffer), (F, "glFramebufferRenderbufferEXT(0x%x, 0x%x, 0x%x, %d);\n", target, attachment, renderbuffertarget, renderbuffer));
}

KEYWORD1 void KEYWORD2 NAME(FramebufferTexture1D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    (void) target; (void) attachment; (void) textarget; (void) texture; (void) level;
   DISPATCH(FramebufferTexture1DEXT, (target, attachment, textarget, texture, level), (F, "glFramebufferTexture1D(0x%x, 0x%x, 0x%x, %d, %d);\n", target, attachment, textarget, texture, level));
}

KEYWORD1 void KEYWORD2 NAME(FramebufferTexture1DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    (void) target; (void) attachment; (void) textarget; (void) texture; (void) level;
   DISPATCH(FramebufferTexture1DEXT, (target, attachment, textarget, texture, level), (F, "glFramebufferTexture1DEXT(0x%x, 0x%x, 0x%x, %d, %d);\n", target, attachment, textarget, texture, level));
}

KEYWORD1 void KEYWORD2 NAME(FramebufferTexture2D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    (void) target; (void) attachment; (void) textarget; (void) texture; (void) level;
   DISPATCH(FramebufferTexture2DEXT, (target, attachment, textarget, texture, level), (F, "glFramebufferTexture2D(0x%x, 0x%x, 0x%x, %d, %d);\n", target, attachment, textarget, texture, level));
}

KEYWORD1 void KEYWORD2 NAME(FramebufferTexture2DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    (void) target; (void) attachment; (void) textarget; (void) texture; (void) level;
   DISPATCH(FramebufferTexture2DEXT, (target, attachment, textarget, texture, level), (F, "glFramebufferTexture2DEXT(0x%x, 0x%x, 0x%x, %d, %d);\n", target, attachment, textarget, texture, level));
}

KEYWORD1 void KEYWORD2 NAME(FramebufferTexture3D)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset)
{
    (void) target; (void) attachment; (void) textarget; (void) texture; (void) level; (void) zoffset;
   DISPATCH(FramebufferTexture3DEXT, (target, attachment, textarget, texture, level, zoffset), (F, "glFramebufferTexture3D(0x%x, 0x%x, 0x%x, %d, %d, %d);\n", target, attachment, textarget, texture, level, zoffset));
}

KEYWORD1 void KEYWORD2 NAME(FramebufferTexture3DEXT)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset)
{
    (void) target; (void) attachment; (void) textarget; (void) texture; (void) level; (void) zoffset;
   DISPATCH(FramebufferTexture3DEXT, (target, attachment, textarget, texture, level, zoffset), (F, "glFramebufferTexture3DEXT(0x%x, 0x%x, 0x%x, %d, %d, %d);\n", target, attachment, textarget, texture, level, zoffset));
}

KEYWORD1 void KEYWORD2 NAME(GenFramebuffers)(GLsizei n, GLuint * framebuffers)
{
    (void) n; (void) framebuffers;
   DISPATCH(GenFramebuffersEXT, (n, framebuffers), (F, "glGenFramebuffers(%d, %p);\n", n, (const void *) framebuffers));
}

KEYWORD1 void KEYWORD2 NAME(GenFramebuffersEXT)(GLsizei n, GLuint * framebuffers)
{
    (void) n; (void) framebuffers;
   DISPATCH(GenFramebuffersEXT, (n, framebuffers), (F, "glGenFramebuffersEXT(%d, %p);\n", n, (const void *) framebuffers));
}

KEYWORD1 void KEYWORD2 NAME(GenRenderbuffers)(GLsizei n, GLuint * renderbuffers)
{
    (void) n; (void) renderbuffers;
   DISPATCH(GenRenderbuffersEXT, (n, renderbuffers), (F, "glGenRenderbuffers(%d, %p);\n", n, (const void *) renderbuffers));
}

KEYWORD1 void KEYWORD2 NAME(GenRenderbuffersEXT)(GLsizei n, GLuint * renderbuffers)
{
    (void) n; (void) renderbuffers;
   DISPATCH(GenRenderbuffersEXT, (n, renderbuffers), (F, "glGenRenderbuffersEXT(%d, %p);\n", n, (const void *) renderbuffers));
}

KEYWORD1 void KEYWORD2 NAME(GenerateMipmap)(GLenum target)
{
    (void) target;
   DISPATCH(GenerateMipmapEXT, (target), (F, "glGenerateMipmap(0x%x);\n", target));
}

KEYWORD1 void KEYWORD2 NAME(GenerateMipmapEXT)(GLenum target)
{
    (void) target;
   DISPATCH(GenerateMipmapEXT, (target), (F, "glGenerateMipmapEXT(0x%x);\n", target));
}

KEYWORD1 void KEYWORD2 NAME(GetFramebufferAttachmentParameteriv)(GLenum target, GLenum attachment, GLenum pname, GLint * params)
{
    (void) target; (void) attachment; (void) pname; (void) params;
   DISPATCH(GetFramebufferAttachmentParameterivEXT, (target, attachment, pname, params), (F, "glGetFramebufferAttachmentParameteriv(0x%x, 0x%x, 0x%x, %p);\n", target, attachment, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetFramebufferAttachmentParameterivEXT)(GLenum target, GLenum attachment, GLenum pname, GLint * params)
{
    (void) target; (void) attachment; (void) pname; (void) params;
   DISPATCH(GetFramebufferAttachmentParameterivEXT, (target, attachment, pname, params), (F, "glGetFramebufferAttachmentParameterivEXT(0x%x, 0x%x, 0x%x, %p);\n", target, attachment, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetRenderbufferParameteriv)(GLenum target, GLenum pname, GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetRenderbufferParameterivEXT, (target, pname, params), (F, "glGetRenderbufferParameteriv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetRenderbufferParameterivEXT)(GLenum target, GLenum pname, GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetRenderbufferParameterivEXT, (target, pname, params), (F, "glGetRenderbufferParameterivEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsFramebuffer)(GLuint framebuffer)
{
    (void) framebuffer;
   RETURN_DISPATCH(IsFramebufferEXT, (framebuffer), (F, "glIsFramebuffer(%d);\n", framebuffer));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsFramebufferEXT)(GLuint framebuffer)
{
    (void) framebuffer;
   RETURN_DISPATCH(IsFramebufferEXT, (framebuffer), (F, "glIsFramebufferEXT(%d);\n", framebuffer));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsRenderbuffer)(GLuint renderbuffer)
{
    (void) renderbuffer;
   RETURN_DISPATCH(IsRenderbufferEXT, (renderbuffer), (F, "glIsRenderbuffer(%d);\n", renderbuffer));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsRenderbufferEXT)(GLuint renderbuffer)
{
    (void) renderbuffer;
   RETURN_DISPATCH(IsRenderbufferEXT, (renderbuffer), (F, "glIsRenderbufferEXT(%d);\n", renderbuffer));
}

KEYWORD1 void KEYWORD2 NAME(RenderbufferStorage)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
    (void) target; (void) internalformat; (void) width; (void) height;
   DISPATCH(RenderbufferStorageEXT, (target, internalformat, width, height), (F, "glRenderbufferStorage(0x%x, 0x%x, %d, %d);\n", target, internalformat, width, height));
}

KEYWORD1 void KEYWORD2 NAME(RenderbufferStorageEXT)(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
    (void) target; (void) internalformat; (void) width; (void) height;
   DISPATCH(RenderbufferStorageEXT, (target, internalformat, width, height), (F, "glRenderbufferStorageEXT(0x%x, 0x%x, %d, %d);\n", target, internalformat, width, height));
}

KEYWORD1 void KEYWORD2 NAME(BlitFramebuffer)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
    (void) srcX0; (void) srcY0; (void) srcX1; (void) srcY1; (void) dstX0; (void) dstY0; (void) dstX1; (void) dstY1; (void) mask; (void) filter;
   DISPATCH(BlitFramebufferEXT, (srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter), (F, "glBlitFramebuffer(%d, %d, %d, %d, %d, %d, %d, %d, %d, 0x%x);\n", srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_897)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_897)(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
    (void) srcX0; (void) srcY0; (void) srcX1; (void) srcY1; (void) dstX0; (void) dstY0; (void) dstX1; (void) dstY1; (void) mask; (void) filter;
   DISPATCH(BlitFramebufferEXT, (srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter), (F, "glBlitFramebufferEXT(%d, %d, %d, %d, %d, %d, %d, %d, %d, 0x%x);\n", srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_898)(GLenum target, GLenum pname, GLint param);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_898)(GLenum target, GLenum pname, GLint param)
{
    (void) target; (void) pname; (void) param;
   DISPATCH(BufferParameteriAPPLE, (target, pname, param), (F, "glBufferParameteriAPPLE(0x%x, 0x%x, %d);\n", target, pname, param));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_899)(GLenum target, GLintptr offset, GLsizeiptr size);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_899)(GLenum target, GLintptr offset, GLsizeiptr size)
{
    (void) target; (void) offset; (void) size;
   DISPATCH(FlushMappedBufferRangeAPPLE, (target, offset, size), (F, "glFlushMappedBufferRangeAPPLE(0x%x, %d, %d);\n", target, offset, size));
}

KEYWORD1 void KEYWORD2 NAME(BindFragDataLocationEXT)(GLuint program, GLuint colorNumber, const GLchar * name)
{
    (void) program; (void) colorNumber; (void) name;
   DISPATCH(BindFragDataLocationEXT, (program, colorNumber, name), (F, "glBindFragDataLocationEXT(%d, %d, %p);\n", program, colorNumber, (const void *) name));
}

KEYWORD1 void KEYWORD2 NAME(BindFragDataLocation)(GLuint program, GLuint colorNumber, const GLchar * name)
{
    (void) program; (void) colorNumber; (void) name;
   DISPATCH(BindFragDataLocationEXT, (program, colorNumber, name), (F, "glBindFragDataLocation(%d, %d, %p);\n", program, colorNumber, (const void *) name));
}

KEYWORD1 GLint KEYWORD2 NAME(GetFragDataLocationEXT)(GLuint program, const GLchar * name)
{
    (void) program; (void) name;
   RETURN_DISPATCH(GetFragDataLocationEXT, (program, name), (F, "glGetFragDataLocationEXT(%d, %p);\n", program, (const void *) name));
}

KEYWORD1 GLint KEYWORD2 NAME(GetFragDataLocation)(GLuint program, const GLchar * name)
{
    (void) program; (void) name;
   RETURN_DISPATCH(GetFragDataLocationEXT, (program, name), (F, "glGetFragDataLocation(%d, %p);\n", program, (const void *) name));
}

KEYWORD1 void KEYWORD2 NAME(GetUniformuivEXT)(GLuint program, GLint location, GLuint * params)
{
    (void) program; (void) location; (void) params;
   DISPATCH(GetUniformuivEXT, (program, location, params), (F, "glGetUniformuivEXT(%d, %d, %p);\n", program, location, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetUniformuiv)(GLuint program, GLint location, GLuint * params)
{
    (void) program; (void) location; (void) params;
   DISPATCH(GetUniformuivEXT, (program, location, params), (F, "glGetUniformuiv(%d, %d, %p);\n", program, location, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribIivEXT)(GLuint index, GLenum pname, GLint * params)
{
    (void) index; (void) pname; (void) params;
   DISPATCH(GetVertexAttribIivEXT, (index, pname, params), (F, "glGetVertexAttribIivEXT(%d, 0x%x, %p);\n", index, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribIiv)(GLuint index, GLenum pname, GLint * params)
{
    (void) index; (void) pname; (void) params;
   DISPATCH(GetVertexAttribIivEXT, (index, pname, params), (F, "glGetVertexAttribIiv(%d, 0x%x, %p);\n", index, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribIuivEXT)(GLuint index, GLenum pname, GLuint * params)
{
    (void) index; (void) pname; (void) params;
   DISPATCH(GetVertexAttribIuivEXT, (index, pname, params), (F, "glGetVertexAttribIuivEXT(%d, 0x%x, %p);\n", index, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetVertexAttribIuiv)(GLuint index, GLenum pname, GLuint * params)
{
    (void) index; (void) pname; (void) params;
   DISPATCH(GetVertexAttribIuivEXT, (index, pname, params), (F, "glGetVertexAttribIuiv(%d, 0x%x, %p);\n", index, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(Uniform1uiEXT)(GLint location, GLuint x)
{
    (void) location; (void) x;
   DISPATCH(Uniform1uiEXT, (location, x), (F, "glUniform1uiEXT(%d, %d);\n", location, x));
}

KEYWORD1 void KEYWORD2 NAME(Uniform1ui)(GLint location, GLuint x)
{
    (void) location; (void) x;
   DISPATCH(Uniform1uiEXT, (location, x), (F, "glUniform1ui(%d, %d);\n", location, x));
}

KEYWORD1 void KEYWORD2 NAME(Uniform1uivEXT)(GLint location, GLsizei count, const GLuint * value)
{
    (void) location; (void) count; (void) value;
   DISPATCH(Uniform1uivEXT, (location, count, value), (F, "glUniform1uivEXT(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform1uiv)(GLint location, GLsizei count, const GLuint * value)
{
    (void) location; (void) count; (void) value;
   DISPATCH(Uniform1uivEXT, (location, count, value), (F, "glUniform1uiv(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform2uiEXT)(GLint location, GLuint x, GLuint y)
{
    (void) location; (void) x; (void) y;
   DISPATCH(Uniform2uiEXT, (location, x, y), (F, "glUniform2uiEXT(%d, %d, %d);\n", location, x, y));
}

KEYWORD1 void KEYWORD2 NAME(Uniform2ui)(GLint location, GLuint x, GLuint y)
{
    (void) location; (void) x; (void) y;
   DISPATCH(Uniform2uiEXT, (location, x, y), (F, "glUniform2ui(%d, %d, %d);\n", location, x, y));
}

KEYWORD1 void KEYWORD2 NAME(Uniform2uivEXT)(GLint location, GLsizei count, const GLuint * value)
{
    (void) location; (void) count; (void) value;
   DISPATCH(Uniform2uivEXT, (location, count, value), (F, "glUniform2uivEXT(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform2uiv)(GLint location, GLsizei count, const GLuint * value)
{
    (void) location; (void) count; (void) value;
   DISPATCH(Uniform2uivEXT, (location, count, value), (F, "glUniform2uiv(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform3uiEXT)(GLint location, GLuint x, GLuint y, GLuint z)
{
    (void) location; (void) x; (void) y; (void) z;
   DISPATCH(Uniform3uiEXT, (location, x, y, z), (F, "glUniform3uiEXT(%d, %d, %d, %d);\n", location, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Uniform3ui)(GLint location, GLuint x, GLuint y, GLuint z)
{
    (void) location; (void) x; (void) y; (void) z;
   DISPATCH(Uniform3uiEXT, (location, x, y, z), (F, "glUniform3ui(%d, %d, %d, %d);\n", location, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(Uniform3uivEXT)(GLint location, GLsizei count, const GLuint * value)
{
    (void) location; (void) count; (void) value;
   DISPATCH(Uniform3uivEXT, (location, count, value), (F, "glUniform3uivEXT(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform3uiv)(GLint location, GLsizei count, const GLuint * value)
{
    (void) location; (void) count; (void) value;
   DISPATCH(Uniform3uivEXT, (location, count, value), (F, "glUniform3uiv(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform4uiEXT)(GLint location, GLuint x, GLuint y, GLuint z, GLuint w)
{
    (void) location; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(Uniform4uiEXT, (location, x, y, z, w), (F, "glUniform4uiEXT(%d, %d, %d, %d, %d);\n", location, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(Uniform4ui)(GLint location, GLuint x, GLuint y, GLuint z, GLuint w)
{
    (void) location; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(Uniform4uiEXT, (location, x, y, z, w), (F, "glUniform4ui(%d, %d, %d, %d, %d);\n", location, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(Uniform4uivEXT)(GLint location, GLsizei count, const GLuint * value)
{
    (void) location; (void) count; (void) value;
   DISPATCH(Uniform4uivEXT, (location, count, value), (F, "glUniform4uivEXT(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(Uniform4uiv)(GLint location, GLsizei count, const GLuint * value)
{
    (void) location; (void) count; (void) value;
   DISPATCH(Uniform4uivEXT, (location, count, value), (F, "glUniform4uiv(%d, %d, %p);\n", location, count, (const void *) value));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI1iEXT)(GLuint index, GLint x)
{
    (void) index; (void) x;
   DISPATCH(VertexAttribI1iEXT, (index, x), (F, "glVertexAttribI1iEXT(%d, %d);\n", index, x));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI1i)(GLuint index, GLint x)
{
    (void) index; (void) x;
   DISPATCH(VertexAttribI1iEXT, (index, x), (F, "glVertexAttribI1i(%d, %d);\n", index, x));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI1ivEXT)(GLuint index, const GLint * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttribI1ivEXT, (index, v), (F, "glVertexAttribI1ivEXT(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI1iv)(GLuint index, const GLint * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttribI1ivEXT, (index, v), (F, "glVertexAttribI1iv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI1uiEXT)(GLuint index, GLuint x)
{
    (void) index; (void) x;
   DISPATCH(VertexAttribI1uiEXT, (index, x), (F, "glVertexAttribI1uiEXT(%d, %d);\n", index, x));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI1ui)(GLuint index, GLuint x)
{
    (void) index; (void) x;
   DISPATCH(VertexAttribI1uiEXT, (index, x), (F, "glVertexAttribI1ui(%d, %d);\n", index, x));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI1uivEXT)(GLuint index, const GLuint * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttribI1uivEXT, (index, v), (F, "glVertexAttribI1uivEXT(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI1uiv)(GLuint index, const GLuint * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttribI1uivEXT, (index, v), (F, "glVertexAttribI1uiv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI2iEXT)(GLuint index, GLint x, GLint y)
{
    (void) index; (void) x; (void) y;
   DISPATCH(VertexAttribI2iEXT, (index, x, y), (F, "glVertexAttribI2iEXT(%d, %d, %d);\n", index, x, y));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI2i)(GLuint index, GLint x, GLint y)
{
    (void) index; (void) x; (void) y;
   DISPATCH(VertexAttribI2iEXT, (index, x, y), (F, "glVertexAttribI2i(%d, %d, %d);\n", index, x, y));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI2ivEXT)(GLuint index, const GLint * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttribI2ivEXT, (index, v), (F, "glVertexAttribI2ivEXT(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI2iv)(GLuint index, const GLint * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttribI2ivEXT, (index, v), (F, "glVertexAttribI2iv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI2uiEXT)(GLuint index, GLuint x, GLuint y)
{
    (void) index; (void) x; (void) y;
   DISPATCH(VertexAttribI2uiEXT, (index, x, y), (F, "glVertexAttribI2uiEXT(%d, %d, %d);\n", index, x, y));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI2ui)(GLuint index, GLuint x, GLuint y)
{
    (void) index; (void) x; (void) y;
   DISPATCH(VertexAttribI2uiEXT, (index, x, y), (F, "glVertexAttribI2ui(%d, %d, %d);\n", index, x, y));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI2uivEXT)(GLuint index, const GLuint * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttribI2uivEXT, (index, v), (F, "glVertexAttribI2uivEXT(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI2uiv)(GLuint index, const GLuint * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttribI2uivEXT, (index, v), (F, "glVertexAttribI2uiv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI3iEXT)(GLuint index, GLint x, GLint y, GLint z)
{
    (void) index; (void) x; (void) y; (void) z;
   DISPATCH(VertexAttribI3iEXT, (index, x, y, z), (F, "glVertexAttribI3iEXT(%d, %d, %d, %d);\n", index, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI3i)(GLuint index, GLint x, GLint y, GLint z)
{
    (void) index; (void) x; (void) y; (void) z;
   DISPATCH(VertexAttribI3iEXT, (index, x, y, z), (F, "glVertexAttribI3i(%d, %d, %d, %d);\n", index, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI3ivEXT)(GLuint index, const GLint * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttribI3ivEXT, (index, v), (F, "glVertexAttribI3ivEXT(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI3iv)(GLuint index, const GLint * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttribI3ivEXT, (index, v), (F, "glVertexAttribI3iv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI3uiEXT)(GLuint index, GLuint x, GLuint y, GLuint z)
{
    (void) index; (void) x; (void) y; (void) z;
   DISPATCH(VertexAttribI3uiEXT, (index, x, y, z), (F, "glVertexAttribI3uiEXT(%d, %d, %d, %d);\n", index, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI3ui)(GLuint index, GLuint x, GLuint y, GLuint z)
{
    (void) index; (void) x; (void) y; (void) z;
   DISPATCH(VertexAttribI3uiEXT, (index, x, y, z), (F, "glVertexAttribI3ui(%d, %d, %d, %d);\n", index, x, y, z));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI3uivEXT)(GLuint index, const GLuint * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttribI3uivEXT, (index, v), (F, "glVertexAttribI3uivEXT(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI3uiv)(GLuint index, const GLuint * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttribI3uivEXT, (index, v), (F, "glVertexAttribI3uiv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI4bvEXT)(GLuint index, const GLbyte * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttribI4bvEXT, (index, v), (F, "glVertexAttribI4bvEXT(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI4bv)(GLuint index, const GLbyte * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttribI4bvEXT, (index, v), (F, "glVertexAttribI4bv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI4iEXT)(GLuint index, GLint x, GLint y, GLint z, GLint w)
{
    (void) index; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(VertexAttribI4iEXT, (index, x, y, z, w), (F, "glVertexAttribI4iEXT(%d, %d, %d, %d, %d);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI4i)(GLuint index, GLint x, GLint y, GLint z, GLint w)
{
    (void) index; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(VertexAttribI4iEXT, (index, x, y, z, w), (F, "glVertexAttribI4i(%d, %d, %d, %d, %d);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI4ivEXT)(GLuint index, const GLint * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttribI4ivEXT, (index, v), (F, "glVertexAttribI4ivEXT(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI4iv)(GLuint index, const GLint * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttribI4ivEXT, (index, v), (F, "glVertexAttribI4iv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI4svEXT)(GLuint index, const GLshort * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttribI4svEXT, (index, v), (F, "glVertexAttribI4svEXT(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI4sv)(GLuint index, const GLshort * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttribI4svEXT, (index, v), (F, "glVertexAttribI4sv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI4ubvEXT)(GLuint index, const GLubyte * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttribI4ubvEXT, (index, v), (F, "glVertexAttribI4ubvEXT(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI4ubv)(GLuint index, const GLubyte * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttribI4ubvEXT, (index, v), (F, "glVertexAttribI4ubv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI4uiEXT)(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w)
{
    (void) index; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(VertexAttribI4uiEXT, (index, x, y, z, w), (F, "glVertexAttribI4uiEXT(%d, %d, %d, %d, %d);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI4ui)(GLuint index, GLuint x, GLuint y, GLuint z, GLuint w)
{
    (void) index; (void) x; (void) y; (void) z; (void) w;
   DISPATCH(VertexAttribI4uiEXT, (index, x, y, z, w), (F, "glVertexAttribI4ui(%d, %d, %d, %d, %d);\n", index, x, y, z, w));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI4uivEXT)(GLuint index, const GLuint * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttribI4uivEXT, (index, v), (F, "glVertexAttribI4uivEXT(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI4uiv)(GLuint index, const GLuint * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttribI4uivEXT, (index, v), (F, "glVertexAttribI4uiv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI4usvEXT)(GLuint index, const GLushort * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttribI4usvEXT, (index, v), (F, "glVertexAttribI4usvEXT(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribI4usv)(GLuint index, const GLushort * v)
{
    (void) index; (void) v;
   DISPATCH(VertexAttribI4usvEXT, (index, v), (F, "glVertexAttribI4usv(%d, %p);\n", index, (const void *) v));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribIPointerEXT)(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
    (void) index; (void) size; (void) type; (void) stride; (void) pointer;
   DISPATCH(VertexAttribIPointerEXT, (index, size, type, stride, pointer), (F, "glVertexAttribIPointerEXT(%d, %d, 0x%x, %d, %p);\n", index, size, type, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(VertexAttribIPointer)(GLuint index, GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
    (void) index; (void) size; (void) type; (void) stride; (void) pointer;
   DISPATCH(VertexAttribIPointerEXT, (index, size, type, stride, pointer), (F, "glVertexAttribIPointer(%d, %d, 0x%x, %d, %p);\n", index, size, type, stride, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(FramebufferTextureLayer)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)
{
    (void) target; (void) attachment; (void) texture; (void) level; (void) layer;
   DISPATCH(FramebufferTextureLayerEXT, (target, attachment, texture, level, layer), (F, "glFramebufferTextureLayer(0x%x, 0x%x, %d, %d, %d);\n", target, attachment, texture, level, layer));
}

KEYWORD1 void KEYWORD2 NAME(FramebufferTextureLayerARB)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)
{
    (void) target; (void) attachment; (void) texture; (void) level; (void) layer;
   DISPATCH(FramebufferTextureLayerEXT, (target, attachment, texture, level, layer), (F, "glFramebufferTextureLayerARB(0x%x, 0x%x, %d, %d, %d);\n", target, attachment, texture, level, layer));
}

KEYWORD1 void KEYWORD2 NAME(FramebufferTextureLayerEXT)(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer)
{
    (void) target; (void) attachment; (void) texture; (void) level; (void) layer;
   DISPATCH(FramebufferTextureLayerEXT, (target, attachment, texture, level, layer), (F, "glFramebufferTextureLayerEXT(0x%x, 0x%x, %d, %d, %d);\n", target, attachment, texture, level, layer));
}

KEYWORD1 void KEYWORD2 NAME(ColorMaskIndexedEXT)(GLuint buf, GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
    (void) buf; (void) r; (void) g; (void) b; (void) a;
   DISPATCH(ColorMaskIndexedEXT, (buf, r, g, b, a), (F, "glColorMaskIndexedEXT(%d, %d, %d, %d, %d);\n", buf, r, g, b, a));
}

KEYWORD1 void KEYWORD2 NAME(ColorMaski)(GLuint buf, GLboolean r, GLboolean g, GLboolean b, GLboolean a)
{
    (void) buf; (void) r; (void) g; (void) b; (void) a;
   DISPATCH(ColorMaskIndexedEXT, (buf, r, g, b, a), (F, "glColorMaski(%d, %d, %d, %d, %d);\n", buf, r, g, b, a));
}

KEYWORD1 void KEYWORD2 NAME(DisableIndexedEXT)(GLenum target, GLuint index)
{
    (void) target; (void) index;
   DISPATCH(DisableIndexedEXT, (target, index), (F, "glDisableIndexedEXT(0x%x, %d);\n", target, index));
}

KEYWORD1 void KEYWORD2 NAME(Disablei)(GLenum target, GLuint index)
{
    (void) target; (void) index;
   DISPATCH(DisableIndexedEXT, (target, index), (F, "glDisablei(0x%x, %d);\n", target, index));
}

KEYWORD1 void KEYWORD2 NAME(EnableIndexedEXT)(GLenum target, GLuint index)
{
    (void) target; (void) index;
   DISPATCH(EnableIndexedEXT, (target, index), (F, "glEnableIndexedEXT(0x%x, %d);\n", target, index));
}

KEYWORD1 void KEYWORD2 NAME(Enablei)(GLenum target, GLuint index)
{
    (void) target; (void) index;
   DISPATCH(EnableIndexedEXT, (target, index), (F, "glEnablei(0x%x, %d);\n", target, index));
}

KEYWORD1 void KEYWORD2 NAME(GetBooleanIndexedvEXT)(GLenum value, GLuint index, GLboolean * data)
{
    (void) value; (void) index; (void) data;
   DISPATCH(GetBooleanIndexedvEXT, (value, index, data), (F, "glGetBooleanIndexedvEXT(0x%x, %d, %p);\n", value, index, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(GetBooleani_v)(GLenum value, GLuint index, GLboolean * data)
{
    (void) value; (void) index; (void) data;
   DISPATCH(GetBooleanIndexedvEXT, (value, index, data), (F, "glGetBooleani_v(0x%x, %d, %p);\n", value, index, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(GetIntegerIndexedvEXT)(GLenum value, GLuint index, GLint * data)
{
    (void) value; (void) index; (void) data;
   DISPATCH(GetIntegerIndexedvEXT, (value, index, data), (F, "glGetIntegerIndexedvEXT(0x%x, %d, %p);\n", value, index, (const void *) data));
}

KEYWORD1 void KEYWORD2 NAME(GetIntegeri_v)(GLenum value, GLuint index, GLint * data)
{
    (void) value; (void) index; (void) data;
   DISPATCH(GetIntegerIndexedvEXT, (value, index, data), (F, "glGetIntegeri_v(0x%x, %d, %p);\n", value, index, (const void *) data));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsEnabledIndexedEXT)(GLenum target, GLuint index)
{
    (void) target; (void) index;
   RETURN_DISPATCH(IsEnabledIndexedEXT, (target, index), (F, "glIsEnabledIndexedEXT(0x%x, %d);\n", target, index));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsEnabledi)(GLenum target, GLuint index)
{
    (void) target; (void) index;
   RETURN_DISPATCH(IsEnabledIndexedEXT, (target, index), (F, "glIsEnabledi(0x%x, %d);\n", target, index));
}

KEYWORD1 void KEYWORD2 NAME(ClearColorIiEXT)(GLint r, GLint g, GLint b, GLint a)
{
    (void) r; (void) g; (void) b; (void) a;
   DISPATCH(ClearColorIiEXT, (r, g, b, a), (F, "glClearColorIiEXT(%d, %d, %d, %d);\n", r, g, b, a));
}

KEYWORD1 void KEYWORD2 NAME(ClearColorIuiEXT)(GLuint r, GLuint g, GLuint b, GLuint a)
{
    (void) r; (void) g; (void) b; (void) a;
   DISPATCH(ClearColorIuiEXT, (r, g, b, a), (F, "glClearColorIuiEXT(%d, %d, %d, %d);\n", r, g, b, a));
}

KEYWORD1 void KEYWORD2 NAME(GetTexParameterIivEXT)(GLenum target, GLenum pname, GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetTexParameterIivEXT, (target, pname, params), (F, "glGetTexParameterIivEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexParameterIiv)(GLenum target, GLenum pname, GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetTexParameterIivEXT, (target, pname, params), (F, "glGetTexParameterIiv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexParameterIuivEXT)(GLenum target, GLenum pname, GLuint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetTexParameterIuivEXT, (target, pname, params), (F, "glGetTexParameterIuivEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetTexParameterIuiv)(GLenum target, GLenum pname, GLuint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetTexParameterIuivEXT, (target, pname, params), (F, "glGetTexParameterIuiv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(TexParameterIivEXT)(GLenum target, GLenum pname, const GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(TexParameterIivEXT, (target, pname, params), (F, "glTexParameterIivEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(TexParameterIiv)(GLenum target, GLenum pname, const GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(TexParameterIivEXT, (target, pname, params), (F, "glTexParameterIiv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(TexParameterIuivEXT)(GLenum target, GLenum pname, const GLuint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(TexParameterIuivEXT, (target, pname, params), (F, "glTexParameterIuivEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(TexParameterIuiv)(GLenum target, GLenum pname, const GLuint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(TexParameterIuivEXT, (target, pname, params), (F, "glTexParameterIuiv(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(BeginConditionalRenderNV)(GLuint query, GLenum mode)
{
    (void) query; (void) mode;
   DISPATCH(BeginConditionalRenderNV, (query, mode), (F, "glBeginConditionalRenderNV(%d, 0x%x);\n", query, mode));
}

KEYWORD1 void KEYWORD2 NAME(BeginConditionalRender)(GLuint query, GLenum mode)
{
    (void) query; (void) mode;
   DISPATCH(BeginConditionalRenderNV, (query, mode), (F, "glBeginConditionalRender(%d, 0x%x);\n", query, mode));
}

KEYWORD1 void KEYWORD2 NAME(EndConditionalRenderNV)(void)
{
   DISPATCH(EndConditionalRenderNV, (), (F, "glEndConditionalRenderNV();\n"));
}

KEYWORD1 void KEYWORD2 NAME(EndConditionalRender)(void)
{
   DISPATCH(EndConditionalRenderNV, (), (F, "glEndConditionalRender();\n"));
}

KEYWORD1 void KEYWORD2 NAME(BeginTransformFeedbackEXT)(GLenum mode)
{
    (void) mode;
   DISPATCH(BeginTransformFeedbackEXT, (mode), (F, "glBeginTransformFeedbackEXT(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(BeginTransformFeedback)(GLenum mode)
{
    (void) mode;
   DISPATCH(BeginTransformFeedbackEXT, (mode), (F, "glBeginTransformFeedback(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(BindBufferBaseEXT)(GLenum target, GLuint index, GLuint buffer)
{
    (void) target; (void) index; (void) buffer;
   DISPATCH(BindBufferBaseEXT, (target, index, buffer), (F, "glBindBufferBaseEXT(0x%x, %d, %d);\n", target, index, buffer));
}

KEYWORD1 void KEYWORD2 NAME(BindBufferBase)(GLenum target, GLuint index, GLuint buffer)
{
    (void) target; (void) index; (void) buffer;
   DISPATCH(BindBufferBaseEXT, (target, index, buffer), (F, "glBindBufferBase(0x%x, %d, %d);\n", target, index, buffer));
}

KEYWORD1 void KEYWORD2 NAME(BindBufferOffsetEXT)(GLenum target, GLuint index, GLuint buffer, GLintptr offset)
{
    (void) target; (void) index; (void) buffer; (void) offset;
   DISPATCH(BindBufferOffsetEXT, (target, index, buffer, offset), (F, "glBindBufferOffsetEXT(0x%x, %d, %d, %d);\n", target, index, buffer, offset));
}

KEYWORD1 void KEYWORD2 NAME(BindBufferRangeEXT)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size)
{
    (void) target; (void) index; (void) buffer; (void) offset; (void) size;
   DISPATCH(BindBufferRangeEXT, (target, index, buffer, offset, size), (F, "glBindBufferRangeEXT(0x%x, %d, %d, %d, %d);\n", target, index, buffer, offset, size));
}

KEYWORD1 void KEYWORD2 NAME(BindBufferRange)(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size)
{
    (void) target; (void) index; (void) buffer; (void) offset; (void) size;
   DISPATCH(BindBufferRangeEXT, (target, index, buffer, offset, size), (F, "glBindBufferRange(0x%x, %d, %d, %d, %d);\n", target, index, buffer, offset, size));
}

KEYWORD1 void KEYWORD2 NAME(EndTransformFeedbackEXT)(void)
{
   DISPATCH(EndTransformFeedbackEXT, (), (F, "glEndTransformFeedbackEXT();\n"));
}

KEYWORD1 void KEYWORD2 NAME(EndTransformFeedback)(void)
{
   DISPATCH(EndTransformFeedbackEXT, (), (F, "glEndTransformFeedback();\n"));
}

KEYWORD1 void KEYWORD2 NAME(GetTransformFeedbackVaryingEXT)(GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLsizei * size, GLenum * type, GLchar * name)
{
    (void) program; (void) index; (void) bufSize; (void) length; (void) size; (void) type; (void) name;
   DISPATCH(GetTransformFeedbackVaryingEXT, (program, index, bufSize, length, size, type, name), (F, "glGetTransformFeedbackVaryingEXT(%d, %d, %d, %p, %p, %p, %p);\n", program, index, bufSize, (const void *) length, (const void *) size, (const void *) type, (const void *) name));
}

KEYWORD1 void KEYWORD2 NAME(GetTransformFeedbackVarying)(GLuint program, GLuint index, GLsizei bufSize, GLsizei * length, GLsizei * size, GLenum * type, GLchar * name)
{
    (void) program; (void) index; (void) bufSize; (void) length; (void) size; (void) type; (void) name;
   DISPATCH(GetTransformFeedbackVaryingEXT, (program, index, bufSize, length, size, type, name), (F, "glGetTransformFeedbackVarying(%d, %d, %d, %p, %p, %p, %p);\n", program, index, bufSize, (const void *) length, (const void *) size, (const void *) type, (const void *) name));
}

KEYWORD1 void KEYWORD2 NAME(TransformFeedbackVaryingsEXT)(GLuint program, GLsizei count, const char ** varyings, GLenum bufferMode)
{
    (void) program; (void) count; (void) varyings; (void) bufferMode;
   DISPATCH(TransformFeedbackVaryingsEXT, (program, count, varyings, bufferMode), (F, "glTransformFeedbackVaryingsEXT(%d, %d, %p, 0x%x);\n", program, count, (const void *) varyings, bufferMode));
}

KEYWORD1 void KEYWORD2 NAME(TransformFeedbackVaryings)(GLuint program, GLsizei count, const GLchar* * varyings, GLenum bufferMode)
{
    (void) program; (void) count; (void) varyings; (void) bufferMode;
   DISPATCH(TransformFeedbackVaryingsEXT, (program, count, varyings, bufferMode), (F, "glTransformFeedbackVaryings(%d, %d, %p, 0x%x);\n", program, count, (const void *) varyings, bufferMode));
}

KEYWORD1 void KEYWORD2 NAME(ProvokingVertexEXT)(GLenum mode)
{
    (void) mode;
   DISPATCH(ProvokingVertexEXT, (mode), (F, "glProvokingVertexEXT(0x%x);\n", mode));
}

KEYWORD1 void KEYWORD2 NAME(ProvokingVertex)(GLenum mode)
{
    (void) mode;
   DISPATCH(ProvokingVertexEXT, (mode), (F, "glProvokingVertex(0x%x);\n", mode));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_957)(GLenum target, GLenum pname, GLvoid ** params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_957)(GLenum target, GLenum pname, GLvoid ** params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetTexParameterPointervAPPLE, (target, pname, params), (F, "glGetTexParameterPointervAPPLE(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_958)(GLenum target, GLsizei length, GLvoid * pointer);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_958)(GLenum target, GLsizei length, GLvoid * pointer)
{
    (void) target; (void) length; (void) pointer;
   DISPATCH(TextureRangeAPPLE, (target, length, pointer), (F, "glTextureRangeAPPLE(0x%x, %d, %p);\n", target, length, (const void *) pointer));
}

KEYWORD1 void KEYWORD2 NAME(GetObjectParameterivAPPLE)(GLenum objectType, GLuint name, GLenum pname, GLint * value)
{
    (void) objectType; (void) name; (void) pname; (void) value;
   DISPATCH(GetObjectParameterivAPPLE, (objectType, name, pname, value), (F, "glGetObjectParameterivAPPLE(0x%x, %d, 0x%x, %p);\n", objectType, name, pname, (const void *) value));
}

KEYWORD1 GLenum KEYWORD2 NAME(ObjectPurgeableAPPLE)(GLenum objectType, GLuint name, GLenum option)
{
    (void) objectType; (void) name; (void) option;
   RETURN_DISPATCH(ObjectPurgeableAPPLE, (objectType, name, option), (F, "glObjectPurgeableAPPLE(0x%x, %d, 0x%x);\n", objectType, name, option));
}

KEYWORD1 GLenum KEYWORD2 NAME(ObjectUnpurgeableAPPLE)(GLenum objectType, GLuint name, GLenum option)
{
    (void) objectType; (void) name; (void) option;
   RETURN_DISPATCH(ObjectUnpurgeableAPPLE, (objectType, name, option), (F, "glObjectUnpurgeableAPPLE(0x%x, %d, 0x%x);\n", objectType, name, option));
}

KEYWORD1 void KEYWORD2 NAME(ActiveProgramEXT)(GLuint program)
{
    (void) program;
   DISPATCH(ActiveProgramEXT, (program), (F, "glActiveProgramEXT(%d);\n", program));
}

KEYWORD1 GLuint KEYWORD2 NAME(CreateShaderProgramEXT)(GLenum type, const GLchar * string)
{
    (void) type; (void) string;
   RETURN_DISPATCH(CreateShaderProgramEXT, (type, string), (F, "glCreateShaderProgramEXT(0x%x, %p);\n", type, (const void *) string));
}

KEYWORD1 void KEYWORD2 NAME(UseShaderProgramEXT)(GLenum type, GLuint program)
{
    (void) type; (void) program;
   DISPATCH(UseShaderProgramEXT, (type, program), (F, "glUseShaderProgramEXT(0x%x, %d);\n", type, program));
}

KEYWORD1 void KEYWORD2 NAME(TextureBarrierNV)(void)
{
   DISPATCH(TextureBarrierNV, (), (F, "glTextureBarrierNV();\n"));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_966)(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_966)(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask)
{
    (void) frontfunc; (void) backfunc; (void) ref; (void) mask;
   DISPATCH(StencilFuncSeparateATI, (frontfunc, backfunc, ref, mask), (F, "glStencilFuncSeparateATI(0x%x, 0x%x, %d, %d);\n", frontfunc, backfunc, ref, mask));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_967)(GLenum target, GLuint index, GLsizei count, const GLfloat * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_967)(GLenum target, GLuint index, GLsizei count, const GLfloat * params)
{
    (void) target; (void) index; (void) count; (void) params;
   DISPATCH(ProgramEnvParameters4fvEXT, (target, index, count, params), (F, "glProgramEnvParameters4fvEXT(0x%x, %d, %d, %p);\n", target, index, count, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_968)(GLenum target, GLuint index, GLsizei count, const GLfloat * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_968)(GLenum target, GLuint index, GLsizei count, const GLfloat * params)
{
    (void) target; (void) index; (void) count; (void) params;
   DISPATCH(ProgramLocalParameters4fvEXT, (target, index, count, params), (F, "glProgramLocalParameters4fvEXT(0x%x, %d, %d, %p);\n", target, index, count, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_969)(GLuint id, GLenum pname, GLint64EXT * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_969)(GLuint id, GLenum pname, GLint64EXT * params)
{
    (void) id; (void) pname; (void) params;
   DISPATCH(GetQueryObjecti64vEXT, (id, pname, params), (F, "glGetQueryObjecti64vEXT(%d, 0x%x, %p);\n", id, pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_970)(GLuint id, GLenum pname, GLuint64EXT * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_970)(GLuint id, GLenum pname, GLuint64EXT * params)
{
    (void) id; (void) pname; (void) params;
   DISPATCH(GetQueryObjectui64vEXT, (id, pname, params), (F, "glGetQueryObjectui64vEXT(%d, 0x%x, %p);\n", id, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(EGLImageTargetRenderbufferStorageOES)(GLenum target, GLvoid * writeOffset)
{
    (void) target; (void) writeOffset;
   DISPATCH(EGLImageTargetRenderbufferStorageOES, (target, writeOffset), (F, "glEGLImageTargetRenderbufferStorageOES(0x%x, %p);\n", target, (const void *) writeOffset));
}

KEYWORD1 void KEYWORD2 NAME(EGLImageTargetTexture2DOES)(GLenum target, GLvoid * writeOffset)
{
    (void) target; (void) writeOffset;
   DISPATCH(EGLImageTargetTexture2DOES, (target, writeOffset), (F, "glEGLImageTargetTexture2DOES(0x%x, %p);\n", target, (const void *) writeOffset));
}


#endif /* _GLAPI_SKIP_NORMAL_ENTRY_POINTS */

/* these entry points might require different protocols */
#ifndef _GLAPI_SKIP_PROTO_ENTRY_POINTS

KEYWORD1 GLboolean KEYWORD2 NAME(AreTexturesResidentEXT)(GLsizei n, const GLuint * textures, GLboolean * residences)
{
    (void) n; (void) textures; (void) residences;
   RETURN_DISPATCH(AreTexturesResident, (n, textures, residences), (F, "glAreTexturesResidentEXT(%d, %p, %p);\n", n, (const void *) textures, (const void *) residences));
}

KEYWORD1 void KEYWORD2 NAME(DeleteTexturesEXT)(GLsizei n, const GLuint * textures)
{
    (void) n; (void) textures;
   DISPATCH(DeleteTextures, (n, textures), (F, "glDeleteTexturesEXT(%d, %p);\n", n, (const void *) textures));
}

KEYWORD1 void KEYWORD2 NAME(GenTexturesEXT)(GLsizei n, GLuint * textures)
{
    (void) n; (void) textures;
   DISPATCH(GenTextures, (n, textures), (F, "glGenTexturesEXT(%d, %p);\n", n, (const void *) textures));
}

KEYWORD1 GLboolean KEYWORD2 NAME(IsTextureEXT)(GLuint texture)
{
    (void) texture;
   RETURN_DISPATCH(IsTexture, (texture), (F, "glIsTextureEXT(%d);\n", texture));
}

KEYWORD1 void KEYWORD2 NAME(GetColorTableEXT)(GLenum target, GLenum format, GLenum type, GLvoid * table)
{
    (void) target; (void) format; (void) type; (void) table;
   DISPATCH(GetColorTable, (target, format, type, table), (F, "glGetColorTableEXT(0x%x, 0x%x, 0x%x, %p);\n", target, format, type, (const void *) table));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_343)(GLenum target, GLenum format, GLenum type, GLvoid * table);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_343)(GLenum target, GLenum format, GLenum type, GLvoid * table)
{
    (void) target; (void) format; (void) type; (void) table;
   DISPATCH(GetColorTable, (target, format, type, table), (F, "glGetColorTableSGI(0x%x, 0x%x, 0x%x, %p);\n", target, format, type, (const void *) table));
}

KEYWORD1 void KEYWORD2 NAME(GetColorTableParameterfvEXT)(GLenum target, GLenum pname, GLfloat * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetColorTableParameterfv, (target, pname, params), (F, "glGetColorTableParameterfvEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_344)(GLenum target, GLenum pname, GLfloat * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_344)(GLenum target, GLenum pname, GLfloat * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetColorTableParameterfv, (target, pname, params), (F, "glGetColorTableParameterfvSGI(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1 void KEYWORD2 NAME(GetColorTableParameterivEXT)(GLenum target, GLenum pname, GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetColorTableParameteriv, (target, pname, params), (F, "glGetColorTableParameterivEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_345)(GLenum target, GLenum pname, GLint * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_345)(GLenum target, GLenum pname, GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetColorTableParameteriv, (target, pname, params), (F, "glGetColorTableParameterivSGI(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_356)(GLenum target, GLenum format, GLenum type, GLvoid * image);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_356)(GLenum target, GLenum format, GLenum type, GLvoid * image)
{
    (void) target; (void) format; (void) type; (void) image;
   DISPATCH(GetConvolutionFilter, (target, format, type, image), (F, "glGetConvolutionFilterEXT(0x%x, 0x%x, 0x%x, %p);\n", target, format, type, (const void *) image));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_357)(GLenum target, GLenum pname, GLfloat * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_357)(GLenum target, GLenum pname, GLfloat * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetConvolutionParameterfv, (target, pname, params), (F, "glGetConvolutionParameterfvEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_358)(GLenum target, GLenum pname, GLint * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_358)(GLenum target, GLenum pname, GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetConvolutionParameteriv, (target, pname, params), (F, "glGetConvolutionParameterivEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_359)(GLenum target, GLenum format, GLenum type, GLvoid * row, GLvoid * column, GLvoid * span);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_359)(GLenum target, GLenum format, GLenum type, GLvoid * row, GLvoid * column, GLvoid * span)
{
    (void) target; (void) format; (void) type; (void) row; (void) column; (void) span;
   DISPATCH(GetSeparableFilter, (target, format, type, row, column, span), (F, "glGetSeparableFilterEXT(0x%x, 0x%x, 0x%x, %p, %p, %p);\n", target, format, type, (const void *) row, (const void *) column, (const void *) span));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_361)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_361)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values)
{
    (void) target; (void) reset; (void) format; (void) type; (void) values;
   DISPATCH(GetHistogram, (target, reset, format, type, values), (F, "glGetHistogramEXT(0x%x, %d, 0x%x, 0x%x, %p);\n", target, reset, format, type, (const void *) values));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_362)(GLenum target, GLenum pname, GLfloat * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_362)(GLenum target, GLenum pname, GLfloat * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetHistogramParameterfv, (target, pname, params), (F, "glGetHistogramParameterfvEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_363)(GLenum target, GLenum pname, GLint * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_363)(GLenum target, GLenum pname, GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetHistogramParameteriv, (target, pname, params), (F, "glGetHistogramParameterivEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_364)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_364)(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values)
{
    (void) target; (void) reset; (void) format; (void) type; (void) values;
   DISPATCH(GetMinmax, (target, reset, format, type, values), (F, "glGetMinmaxEXT(0x%x, %d, 0x%x, 0x%x, %p);\n", target, reset, format, type, (const void *) values));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_365)(GLenum target, GLenum pname, GLfloat * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_365)(GLenum target, GLenum pname, GLfloat * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetMinmaxParameterfv, (target, pname, params), (F, "glGetMinmaxParameterfvEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_366)(GLenum target, GLenum pname, GLint * params);

KEYWORD1_ALT void KEYWORD2 NAME(_dispatch_stub_366)(GLenum target, GLenum pname, GLint * params)
{
    (void) target; (void) pname; (void) params;
   DISPATCH(GetMinmaxParameteriv, (target, pname, params), (F, "glGetMinmaxParameterivEXT(0x%x, 0x%x, %p);\n", target, pname, (const void *) params));
}


#endif /* _GLAPI_SKIP_PROTO_ENTRY_POINTS */


#endif /* defined( NAME ) */

/*
 * This is how a dispatch table can be initialized with all the functions
 * we generated above.
 */
#ifdef DISPATCH_TABLE_NAME

#ifndef TABLE_ENTRY
#error TABLE_ENTRY must be defined
#endif

#ifdef _GLAPI_SKIP_NORMAL_ENTRY_POINTS
#error _GLAPI_SKIP_NORMAL_ENTRY_POINTS must not be defined
#endif

_glapi_proc DISPATCH_TABLE_NAME[] = {
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
   TABLE_ENTRY(ClampColor),
   TABLE_ENTRY(ClearBufferfi),
   TABLE_ENTRY(ClearBufferfv),
   TABLE_ENTRY(ClearBufferiv),
   TABLE_ENTRY(ClearBufferuiv),
   TABLE_ENTRY(GetStringi),
   TABLE_ENTRY(TexBuffer),
   TABLE_ENTRY(FramebufferTexture),
   TABLE_ENTRY(GetBufferParameteri64v),
   TABLE_ENTRY(GetInteger64i_v),
   TABLE_ENTRY(VertexAttribDivisor),
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
   TABLE_ENTRY(ClampColorARB),
   TABLE_ENTRY(DrawArraysInstancedARB),
   TABLE_ENTRY(DrawElementsInstancedARB),
   TABLE_ENTRY(RenderbufferStorageMultisample),
   TABLE_ENTRY(FramebufferTextureARB),
   TABLE_ENTRY(FramebufferTextureFaceARB),
   TABLE_ENTRY(ProgramParameteriARB),
   TABLE_ENTRY(VertexAttribDivisorARB),
   TABLE_ENTRY(FlushMappedBufferRange),
   TABLE_ENTRY(MapBufferRange),
   TABLE_ENTRY(TexBufferARB),
   TABLE_ENTRY(BindVertexArray),
   TABLE_ENTRY(GenVertexArrays),
   TABLE_ENTRY(CopyBufferSubData),
   TABLE_ENTRY(ClientWaitSync),
   TABLE_ENTRY(DeleteSync),
   TABLE_ENTRY(FenceSync),
   TABLE_ENTRY(GetInteger64v),
   TABLE_ENTRY(GetSynciv),
   TABLE_ENTRY(IsSync),
   TABLE_ENTRY(WaitSync),
   TABLE_ENTRY(DrawElementsBaseVertex),
   TABLE_ENTRY(DrawElementsInstancedBaseVertex),
   TABLE_ENTRY(DrawRangeElementsBaseVertex),
   TABLE_ENTRY(MultiDrawElementsBaseVertex),
   TABLE_ENTRY(BlendEquationSeparateiARB),
   TABLE_ENTRY(BlendEquationiARB),
   TABLE_ENTRY(BlendFuncSeparateiARB),
   TABLE_ENTRY(BlendFunciARB),
   TABLE_ENTRY(BindSampler),
   TABLE_ENTRY(DeleteSamplers),
   TABLE_ENTRY(GenSamplers),
   TABLE_ENTRY(GetSamplerParameterIiv),
   TABLE_ENTRY(GetSamplerParameterIuiv),
   TABLE_ENTRY(GetSamplerParameterfv),
   TABLE_ENTRY(GetSamplerParameteriv),
   TABLE_ENTRY(IsSampler),
   TABLE_ENTRY(SamplerParameterIiv),
   TABLE_ENTRY(SamplerParameterIuiv),
   TABLE_ENTRY(SamplerParameterf),
   TABLE_ENTRY(SamplerParameterfv),
   TABLE_ENTRY(SamplerParameteri),
   TABLE_ENTRY(SamplerParameteriv),
   TABLE_ENTRY(ColorP3ui),
   TABLE_ENTRY(ColorP3uiv),
   TABLE_ENTRY(ColorP4ui),
   TABLE_ENTRY(ColorP4uiv),
   TABLE_ENTRY(MultiTexCoordP1ui),
   TABLE_ENTRY(MultiTexCoordP1uiv),
   TABLE_ENTRY(MultiTexCoordP2ui),
   TABLE_ENTRY(MultiTexCoordP2uiv),
   TABLE_ENTRY(MultiTexCoordP3ui),
   TABLE_ENTRY(MultiTexCoordP3uiv),
   TABLE_ENTRY(MultiTexCoordP4ui),
   TABLE_ENTRY(MultiTexCoordP4uiv),
   TABLE_ENTRY(NormalP3ui),
   TABLE_ENTRY(NormalP3uiv),
   TABLE_ENTRY(SecondaryColorP3ui),
   TABLE_ENTRY(SecondaryColorP3uiv),
   TABLE_ENTRY(TexCoordP1ui),
   TABLE_ENTRY(TexCoordP1uiv),
   TABLE_ENTRY(TexCoordP2ui),
   TABLE_ENTRY(TexCoordP2uiv),
   TABLE_ENTRY(TexCoordP3ui),
   TABLE_ENTRY(TexCoordP3uiv),
   TABLE_ENTRY(TexCoordP4ui),
   TABLE_ENTRY(TexCoordP4uiv),
   TABLE_ENTRY(VertexAttribP1ui),
   TABLE_ENTRY(VertexAttribP1uiv),
   TABLE_ENTRY(VertexAttribP2ui),
   TABLE_ENTRY(VertexAttribP2uiv),
   TABLE_ENTRY(VertexAttribP3ui),
   TABLE_ENTRY(VertexAttribP3uiv),
   TABLE_ENTRY(VertexAttribP4ui),
   TABLE_ENTRY(VertexAttribP4uiv),
   TABLE_ENTRY(VertexP2ui),
   TABLE_ENTRY(VertexP2uiv),
   TABLE_ENTRY(VertexP3ui),
   TABLE_ENTRY(VertexP3uiv),
   TABLE_ENTRY(VertexP4ui),
   TABLE_ENTRY(VertexP4uiv),
   TABLE_ENTRY(BindTransformFeedback),
   TABLE_ENTRY(DeleteTransformFeedbacks),
   TABLE_ENTRY(DrawTransformFeedback),
   TABLE_ENTRY(GenTransformFeedbacks),
   TABLE_ENTRY(IsTransformFeedback),
   TABLE_ENTRY(PauseTransformFeedback),
   TABLE_ENTRY(ResumeTransformFeedback),
   TABLE_ENTRY(ClearDepthf),
   TABLE_ENTRY(DepthRangef),
   TABLE_ENTRY(GetShaderPrecisionFormat),
   TABLE_ENTRY(ReleaseShaderCompiler),
   TABLE_ENTRY(ShaderBinary),
   TABLE_ENTRY(GetGraphicsResetStatusARB),
   TABLE_ENTRY(GetnColorTableARB),
   TABLE_ENTRY(GetnCompressedTexImageARB),
   TABLE_ENTRY(GetnConvolutionFilterARB),
   TABLE_ENTRY(GetnHistogramARB),
   TABLE_ENTRY(GetnMapdvARB),
   TABLE_ENTRY(GetnMapfvARB),
   TABLE_ENTRY(GetnMapivARB),
   TABLE_ENTRY(GetnMinmaxARB),
   TABLE_ENTRY(GetnPixelMapfvARB),
   TABLE_ENTRY(GetnPixelMapuivARB),
   TABLE_ENTRY(GetnPixelMapusvARB),
   TABLE_ENTRY(GetnPolygonStippleARB),
   TABLE_ENTRY(GetnSeparableFilterARB),
   TABLE_ENTRY(GetnTexImageARB),
   TABLE_ENTRY(GetnUniformdvARB),
   TABLE_ENTRY(GetnUniformfvARB),
   TABLE_ENTRY(GetnUniformivARB),
   TABLE_ENTRY(GetnUniformuivARB),
   TABLE_ENTRY(ReadnPixelsARB),
   TABLE_ENTRY(TexStorage1D),
   TABLE_ENTRY(TexStorage2D),
   TABLE_ENTRY(TexStorage3D),
   TABLE_ENTRY(TextureStorage1DEXT),
   TABLE_ENTRY(TextureStorage2DEXT),
   TABLE_ENTRY(TextureStorage3DEXT),
   TABLE_ENTRY(PolygonOffsetEXT),
   TABLE_ENTRY(_dispatch_stub_692),
   TABLE_ENTRY(_dispatch_stub_693),
   TABLE_ENTRY(_dispatch_stub_694),
   TABLE_ENTRY(_dispatch_stub_695),
   TABLE_ENTRY(_dispatch_stub_696),
   TABLE_ENTRY(_dispatch_stub_697),
   TABLE_ENTRY(_dispatch_stub_698),
   TABLE_ENTRY(_dispatch_stub_699),
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
   TABLE_ENTRY(_dispatch_stub_734),
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
   TABLE_ENTRY(_dispatch_stub_776),
   TABLE_ENTRY(_dispatch_stub_777),
   TABLE_ENTRY(_dispatch_stub_778),
   TABLE_ENTRY(_dispatch_stub_779),
   TABLE_ENTRY(_dispatch_stub_780),
   TABLE_ENTRY(_dispatch_stub_781),
   TABLE_ENTRY(_dispatch_stub_782),
   TABLE_ENTRY(_dispatch_stub_783),
   TABLE_ENTRY(_dispatch_stub_784),
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
   TABLE_ENTRY(GetTexBumpParameterfvATI),
   TABLE_ENTRY(GetTexBumpParameterivATI),
   TABLE_ENTRY(TexBumpParameterfvATI),
   TABLE_ENTRY(TexBumpParameterivATI),
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
   TABLE_ENTRY(_dispatch_stub_865),
   TABLE_ENTRY(_dispatch_stub_866),
   TABLE_ENTRY(_dispatch_stub_867),
   TABLE_ENTRY(_dispatch_stub_868),
   TABLE_ENTRY(_dispatch_stub_869),
   TABLE_ENTRY(GetProgramNamedParameterdvNV),
   TABLE_ENTRY(GetProgramNamedParameterfvNV),
   TABLE_ENTRY(ProgramNamedParameter4dNV),
   TABLE_ENTRY(ProgramNamedParameter4dvNV),
   TABLE_ENTRY(ProgramNamedParameter4fNV),
   TABLE_ENTRY(ProgramNamedParameter4fvNV),
   TABLE_ENTRY(PrimitiveRestartIndexNV),
   TABLE_ENTRY(PrimitiveRestartNV),
   TABLE_ENTRY(_dispatch_stub_878),
   TABLE_ENTRY(_dispatch_stub_879),
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
   TABLE_ENTRY(_dispatch_stub_897),
   TABLE_ENTRY(_dispatch_stub_898),
   TABLE_ENTRY(_dispatch_stub_899),
   TABLE_ENTRY(BindFragDataLocationEXT),
   TABLE_ENTRY(GetFragDataLocationEXT),
   TABLE_ENTRY(GetUniformuivEXT),
   TABLE_ENTRY(GetVertexAttribIivEXT),
   TABLE_ENTRY(GetVertexAttribIuivEXT),
   TABLE_ENTRY(Uniform1uiEXT),
   TABLE_ENTRY(Uniform1uivEXT),
   TABLE_ENTRY(Uniform2uiEXT),
   TABLE_ENTRY(Uniform2uivEXT),
   TABLE_ENTRY(Uniform3uiEXT),
   TABLE_ENTRY(Uniform3uivEXT),
   TABLE_ENTRY(Uniform4uiEXT),
   TABLE_ENTRY(Uniform4uivEXT),
   TABLE_ENTRY(VertexAttribI1iEXT),
   TABLE_ENTRY(VertexAttribI1ivEXT),
   TABLE_ENTRY(VertexAttribI1uiEXT),
   TABLE_ENTRY(VertexAttribI1uivEXT),
   TABLE_ENTRY(VertexAttribI2iEXT),
   TABLE_ENTRY(VertexAttribI2ivEXT),
   TABLE_ENTRY(VertexAttribI2uiEXT),
   TABLE_ENTRY(VertexAttribI2uivEXT),
   TABLE_ENTRY(VertexAttribI3iEXT),
   TABLE_ENTRY(VertexAttribI3ivEXT),
   TABLE_ENTRY(VertexAttribI3uiEXT),
   TABLE_ENTRY(VertexAttribI3uivEXT),
   TABLE_ENTRY(VertexAttribI4bvEXT),
   TABLE_ENTRY(VertexAttribI4iEXT),
   TABLE_ENTRY(VertexAttribI4ivEXT),
   TABLE_ENTRY(VertexAttribI4svEXT),
   TABLE_ENTRY(VertexAttribI4ubvEXT),
   TABLE_ENTRY(VertexAttribI4uiEXT),
   TABLE_ENTRY(VertexAttribI4uivEXT),
   TABLE_ENTRY(VertexAttribI4usvEXT),
   TABLE_ENTRY(VertexAttribIPointerEXT),
   TABLE_ENTRY(FramebufferTextureLayerEXT),
   TABLE_ENTRY(ColorMaskIndexedEXT),
   TABLE_ENTRY(DisableIndexedEXT),
   TABLE_ENTRY(EnableIndexedEXT),
   TABLE_ENTRY(GetBooleanIndexedvEXT),
   TABLE_ENTRY(GetIntegerIndexedvEXT),
   TABLE_ENTRY(IsEnabledIndexedEXT),
   TABLE_ENTRY(ClearColorIiEXT),
   TABLE_ENTRY(ClearColorIuiEXT),
   TABLE_ENTRY(GetTexParameterIivEXT),
   TABLE_ENTRY(GetTexParameterIuivEXT),
   TABLE_ENTRY(TexParameterIivEXT),
   TABLE_ENTRY(TexParameterIuivEXT),
   TABLE_ENTRY(BeginConditionalRenderNV),
   TABLE_ENTRY(EndConditionalRenderNV),
   TABLE_ENTRY(BeginTransformFeedbackEXT),
   TABLE_ENTRY(BindBufferBaseEXT),
   TABLE_ENTRY(BindBufferOffsetEXT),
   TABLE_ENTRY(BindBufferRangeEXT),
   TABLE_ENTRY(EndTransformFeedbackEXT),
   TABLE_ENTRY(GetTransformFeedbackVaryingEXT),
   TABLE_ENTRY(TransformFeedbackVaryingsEXT),
   TABLE_ENTRY(ProvokingVertexEXT),
   TABLE_ENTRY(_dispatch_stub_957),
   TABLE_ENTRY(_dispatch_stub_958),
   TABLE_ENTRY(GetObjectParameterivAPPLE),
   TABLE_ENTRY(ObjectPurgeableAPPLE),
   TABLE_ENTRY(ObjectUnpurgeableAPPLE),
   TABLE_ENTRY(ActiveProgramEXT),
   TABLE_ENTRY(CreateShaderProgramEXT),
   TABLE_ENTRY(UseShaderProgramEXT),
   TABLE_ENTRY(TextureBarrierNV),
   TABLE_ENTRY(_dispatch_stub_966),
   TABLE_ENTRY(_dispatch_stub_967),
   TABLE_ENTRY(_dispatch_stub_968),
   TABLE_ENTRY(_dispatch_stub_969),
   TABLE_ENTRY(_dispatch_stub_970),
   TABLE_ENTRY(EGLImageTargetRenderbufferStorageOES),
   TABLE_ENTRY(EGLImageTargetTexture2DOES),
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
_glapi_proc UNUSED_TABLE_NAME[] = {
#ifndef _GLAPI_SKIP_NORMAL_ENTRY_POINTS
   TABLE_ENTRY(ArrayElementEXT),
   TABLE_ENTRY(BindTextureEXT),
   TABLE_ENTRY(DrawArraysEXT),
   TABLE_ENTRY(CopyTexImage1DEXT),
   TABLE_ENTRY(CopyTexImage2DEXT),
   TABLE_ENTRY(CopyTexSubImage1DEXT),
   TABLE_ENTRY(CopyTexSubImage2DEXT),
   TABLE_ENTRY(GetPointervEXT),
   TABLE_ENTRY(PrioritizeTexturesEXT),
   TABLE_ENTRY(TexSubImage1DEXT),
   TABLE_ENTRY(TexSubImage2DEXT),
   TABLE_ENTRY(BlendColorEXT),
   TABLE_ENTRY(BlendEquationEXT),
   TABLE_ENTRY(DrawRangeElementsEXT),
   TABLE_ENTRY(ColorTableEXT),
   TABLE_ENTRY(_dispatch_stub_339),
   TABLE_ENTRY(_dispatch_stub_340),
   TABLE_ENTRY(_dispatch_stub_341),
   TABLE_ENTRY(_dispatch_stub_342),
   TABLE_ENTRY(_dispatch_stub_346),
   TABLE_ENTRY(_dispatch_stub_347),
   TABLE_ENTRY(_dispatch_stub_348),
   TABLE_ENTRY(_dispatch_stub_349),
   TABLE_ENTRY(_dispatch_stub_350),
   TABLE_ENTRY(_dispatch_stub_351),
   TABLE_ENTRY(_dispatch_stub_352),
   TABLE_ENTRY(_dispatch_stub_353),
   TABLE_ENTRY(_dispatch_stub_354),
   TABLE_ENTRY(_dispatch_stub_355),
   TABLE_ENTRY(_dispatch_stub_360),
   TABLE_ENTRY(_dispatch_stub_367),
   TABLE_ENTRY(_dispatch_stub_368),
   TABLE_ENTRY(_dispatch_stub_369),
   TABLE_ENTRY(_dispatch_stub_370),
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
   TABLE_ENTRY(_dispatch_stub_423),
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
   TABLE_ENTRY(DrawBuffersNV),
   TABLE_ENTRY(DrawArraysInstancedEXT),
   TABLE_ENTRY(DrawArraysInstanced),
   TABLE_ENTRY(DrawElementsInstancedEXT),
   TABLE_ENTRY(DrawElementsInstanced),
   TABLE_ENTRY(RenderbufferStorageMultisampleEXT),
   TABLE_ENTRY(BlendEquationSeparateIndexedAMD),
   TABLE_ENTRY(BlendEquationIndexedAMD),
   TABLE_ENTRY(BlendFuncSeparateIndexedAMD),
   TABLE_ENTRY(BlendFuncIndexedAMD),
   TABLE_ENTRY(PointParameterf),
   TABLE_ENTRY(PointParameterfARB),
   TABLE_ENTRY(_dispatch_stub_706),
   TABLE_ENTRY(PointParameterfv),
   TABLE_ENTRY(PointParameterfvARB),
   TABLE_ENTRY(_dispatch_stub_707),
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
   TABLE_ENTRY(_dispatch_stub_735),
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
   TABLE_ENTRY(DeleteVertexArrays),
   TABLE_ENTRY(IsVertexArray),
   TABLE_ENTRY(PrimitiveRestartIndex),
   TABLE_ENTRY(BlendEquationSeparate),
   TABLE_ENTRY(BindFramebuffer),
   TABLE_ENTRY(BindRenderbuffer),
   TABLE_ENTRY(CheckFramebufferStatus),
   TABLE_ENTRY(DeleteFramebuffers),
   TABLE_ENTRY(DeleteRenderbuffers),
   TABLE_ENTRY(FramebufferRenderbuffer),
   TABLE_ENTRY(FramebufferTexture1D),
   TABLE_ENTRY(FramebufferTexture2D),
   TABLE_ENTRY(FramebufferTexture3D),
   TABLE_ENTRY(GenFramebuffers),
   TABLE_ENTRY(GenRenderbuffers),
   TABLE_ENTRY(GenerateMipmap),
   TABLE_ENTRY(GetFramebufferAttachmentParameteriv),
   TABLE_ENTRY(GetRenderbufferParameteriv),
   TABLE_ENTRY(IsFramebuffer),
   TABLE_ENTRY(IsRenderbuffer),
   TABLE_ENTRY(RenderbufferStorage),
   TABLE_ENTRY(BlitFramebuffer),
   TABLE_ENTRY(BindFragDataLocation),
   TABLE_ENTRY(GetFragDataLocation),
   TABLE_ENTRY(GetUniformuiv),
   TABLE_ENTRY(GetVertexAttribIiv),
   TABLE_ENTRY(GetVertexAttribIuiv),
   TABLE_ENTRY(Uniform1ui),
   TABLE_ENTRY(Uniform1uiv),
   TABLE_ENTRY(Uniform2ui),
   TABLE_ENTRY(Uniform2uiv),
   TABLE_ENTRY(Uniform3ui),
   TABLE_ENTRY(Uniform3uiv),
   TABLE_ENTRY(Uniform4ui),
   TABLE_ENTRY(Uniform4uiv),
   TABLE_ENTRY(VertexAttribI1i),
   TABLE_ENTRY(VertexAttribI1iv),
   TABLE_ENTRY(VertexAttribI1ui),
   TABLE_ENTRY(VertexAttribI1uiv),
   TABLE_ENTRY(VertexAttribI2i),
   TABLE_ENTRY(VertexAttribI2iv),
   TABLE_ENTRY(VertexAttribI2ui),
   TABLE_ENTRY(VertexAttribI2uiv),
   TABLE_ENTRY(VertexAttribI3i),
   TABLE_ENTRY(VertexAttribI3iv),
   TABLE_ENTRY(VertexAttribI3ui),
   TABLE_ENTRY(VertexAttribI3uiv),
   TABLE_ENTRY(VertexAttribI4bv),
   TABLE_ENTRY(VertexAttribI4i),
   TABLE_ENTRY(VertexAttribI4iv),
   TABLE_ENTRY(VertexAttribI4sv),
   TABLE_ENTRY(VertexAttribI4ubv),
   TABLE_ENTRY(VertexAttribI4ui),
   TABLE_ENTRY(VertexAttribI4uiv),
   TABLE_ENTRY(VertexAttribI4usv),
   TABLE_ENTRY(VertexAttribIPointer),
   TABLE_ENTRY(FramebufferTextureLayer),
   TABLE_ENTRY(FramebufferTextureLayerARB),
   TABLE_ENTRY(ColorMaski),
   TABLE_ENTRY(Disablei),
   TABLE_ENTRY(Enablei),
   TABLE_ENTRY(GetBooleani_v),
   TABLE_ENTRY(GetIntegeri_v),
   TABLE_ENTRY(IsEnabledi),
   TABLE_ENTRY(GetTexParameterIiv),
   TABLE_ENTRY(GetTexParameterIuiv),
   TABLE_ENTRY(TexParameterIiv),
   TABLE_ENTRY(TexParameterIuiv),
   TABLE_ENTRY(BeginConditionalRender),
   TABLE_ENTRY(EndConditionalRender),
   TABLE_ENTRY(BeginTransformFeedback),
   TABLE_ENTRY(BindBufferBase),
   TABLE_ENTRY(BindBufferRange),
   TABLE_ENTRY(EndTransformFeedback),
   TABLE_ENTRY(GetTransformFeedbackVarying),
   TABLE_ENTRY(TransformFeedbackVaryings),
   TABLE_ENTRY(ProvokingVertex),
#endif /* _GLAPI_SKIP_NORMAL_ENTRY_POINTS */
#ifndef _GLAPI_SKIP_PROTO_ENTRY_POINTS
   TABLE_ENTRY(AreTexturesResidentEXT),
   TABLE_ENTRY(DeleteTexturesEXT),
   TABLE_ENTRY(GenTexturesEXT),
   TABLE_ENTRY(IsTextureEXT),
   TABLE_ENTRY(GetColorTableEXT),
   TABLE_ENTRY(_dispatch_stub_343),
   TABLE_ENTRY(GetColorTableParameterfvEXT),
   TABLE_ENTRY(_dispatch_stub_344),
   TABLE_ENTRY(GetColorTableParameterivEXT),
   TABLE_ENTRY(_dispatch_stub_345),
   TABLE_ENTRY(_dispatch_stub_356),
   TABLE_ENTRY(_dispatch_stub_357),
   TABLE_ENTRY(_dispatch_stub_358),
   TABLE_ENTRY(_dispatch_stub_359),
   TABLE_ENTRY(_dispatch_stub_361),
   TABLE_ENTRY(_dispatch_stub_362),
   TABLE_ENTRY(_dispatch_stub_363),
   TABLE_ENTRY(_dispatch_stub_364),
   TABLE_ENTRY(_dispatch_stub_365),
   TABLE_ENTRY(_dispatch_stub_366),
#endif /* _GLAPI_SKIP_PROTO_ENTRY_POINTS */
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
