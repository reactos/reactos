/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/opengl32/apistubs.c
 * PURPOSE:              OpenGL32 lib, glXXX functions
 */

#include "opengl32.h"

static void GLAPIENTRY nop_NewList(GLuint list, GLenum mode)
{
    (void) list; (void) mode;
}

static void GLAPIENTRY nop_EndList(void)
{
}

static void GLAPIENTRY nop_CallList(GLuint list)
{
    (void) list;
}

static void GLAPIENTRY nop_CallLists(GLsizei n, GLenum type, const GLvoid * lists)
{
    (void) n; (void) type; (void) lists;
}

static void GLAPIENTRY nop_DeleteLists(GLuint list, GLsizei range)
{
    (void) list; (void) range;
}

static GLuint GLAPIENTRY nop_GenLists(GLsizei range)
{
    (void) range;
    return 0;
}

static void GLAPIENTRY nop_ListBase(GLuint base)
{
    (void) base;
}

static void GLAPIENTRY nop_Begin(GLenum mode)
{
    (void) mode;
}

static void GLAPIENTRY nop_Bitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte * bitmap)
{
    (void) width; (void) height; (void) xorig; (void) yorig; (void) xmove; (void) ymove; (void) bitmap;
}

static void GLAPIENTRY nop_Color3b(GLbyte red, GLbyte green, GLbyte blue)
{
    (void) red; (void) green; (void) blue;
}

static void GLAPIENTRY nop_Color3bv(const GLbyte * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Color3d(GLdouble red, GLdouble green, GLdouble blue)
{
    (void) red; (void) green; (void) blue;
}

static void GLAPIENTRY nop_Color3dv(const GLdouble * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Color3f(GLfloat red, GLfloat green, GLfloat blue)
{
    (void) red; (void) green; (void) blue;
}

static void GLAPIENTRY nop_Color3fv(const GLfloat * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Color3i(GLint red, GLint green, GLint blue)
{
    (void) red; (void) green; (void) blue;
}

static void GLAPIENTRY nop_Color3iv(const GLint * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Color3s(GLshort red, GLshort green, GLshort blue)
{
    (void) red; (void) green; (void) blue;
}

static void GLAPIENTRY nop_Color3sv(const GLshort * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Color3ub(GLubyte red, GLubyte green, GLubyte blue)
{
    (void) red; (void) green; (void) blue;
}

static void GLAPIENTRY nop_Color3ubv(const GLubyte * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Color3ui(GLuint red, GLuint green, GLuint blue)
{
    (void) red; (void) green; (void) blue;
}

static void GLAPIENTRY nop_Color3uiv(const GLuint * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Color3us(GLushort red, GLushort green, GLushort blue)
{
    (void) red; (void) green; (void) blue;
}

static void GLAPIENTRY nop_Color3usv(const GLushort * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Color4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
    (void) red; (void) green; (void) blue; (void) alpha;
}

static void GLAPIENTRY nop_Color4bv(const GLbyte * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Color4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
    (void) red; (void) green; (void) blue; (void) alpha;
}

static void GLAPIENTRY nop_Color4dv(const GLdouble * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    (void) red; (void) green; (void) blue; (void) alpha;
}

static void GLAPIENTRY nop_Color4fv(const GLfloat * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Color4i(GLint red, GLint green, GLint blue, GLint alpha)
{
    (void) red; (void) green; (void) blue; (void) alpha;
}

static void GLAPIENTRY nop_Color4iv(const GLint * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Color4s(GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
    (void) red; (void) green; (void) blue; (void) alpha;
}

static void GLAPIENTRY nop_Color4sv(const GLshort * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Color4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    (void) red; (void) green; (void) blue; (void) alpha;
}

static void GLAPIENTRY nop_Color4ubv(const GLubyte * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Color4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
    (void) red; (void) green; (void) blue; (void) alpha;
}

static void GLAPIENTRY nop_Color4uiv(const GLuint * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Color4us(GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
    (void) red; (void) green; (void) blue; (void) alpha;
}

static void GLAPIENTRY nop_Color4usv(const GLushort * v)
{
    (void) v;
}

static void GLAPIENTRY nop_EdgeFlag(GLboolean flag)
{
    (void) flag;
}

static void GLAPIENTRY nop_EdgeFlagv(const GLboolean * flag)
{
    (void) flag;
}

static void GLAPIENTRY nop_End(void)
{
}

static void GLAPIENTRY nop_Indexd(GLdouble c)
{
    (void) c;
}

static void GLAPIENTRY nop_Indexdv(const GLdouble * c)
{
    (void) c;
}

static void GLAPIENTRY nop_Indexf(GLfloat c)
{
    (void) c;
}

static void GLAPIENTRY nop_Indexfv(const GLfloat * c)
{
    (void) c;
}

static void GLAPIENTRY nop_Indexi(GLint c)
{
    (void) c;
}

static void GLAPIENTRY nop_Indexiv(const GLint * c)
{
    (void) c;
}

static void GLAPIENTRY nop_Indexs(GLshort c)
{
    (void) c;
}

static void GLAPIENTRY nop_Indexsv(const GLshort * c)
{
    (void) c;
}

static void GLAPIENTRY nop_Normal3b(GLbyte nx, GLbyte ny, GLbyte nz)
{
    (void) nx; (void) ny; (void) nz;
}

static void GLAPIENTRY nop_Normal3bv(const GLbyte * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Normal3d(GLdouble nx, GLdouble ny, GLdouble nz)
{
    (void) nx; (void) ny; (void) nz;
}

static void GLAPIENTRY nop_Normal3dv(const GLdouble * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Normal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
    (void) nx; (void) ny; (void) nz;
}

static void GLAPIENTRY nop_Normal3fv(const GLfloat * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Normal3i(GLint nx, GLint ny, GLint nz)
{
    (void) nx; (void) ny; (void) nz;
}

static void GLAPIENTRY nop_Normal3iv(const GLint * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Normal3s(GLshort nx, GLshort ny, GLshort nz)
{
    (void) nx; (void) ny; (void) nz;
}

static void GLAPIENTRY nop_Normal3sv(const GLshort * v)
{
    (void) v;
}

static void GLAPIENTRY nop_RasterPos2d(GLdouble x, GLdouble y)
{
    (void) x; (void) y;
}

static void GLAPIENTRY nop_RasterPos2dv(const GLdouble * v)
{
    (void) v;
}

static void GLAPIENTRY nop_RasterPos2f(GLfloat x, GLfloat y)
{
    (void) x; (void) y;
}

static void GLAPIENTRY nop_RasterPos2fv(const GLfloat * v)
{
    (void) v;
}

static void GLAPIENTRY nop_RasterPos2i(GLint x, GLint y)
{
    (void) x; (void) y;
}

static void GLAPIENTRY nop_RasterPos2iv(const GLint * v)
{
    (void) v;
}

static void GLAPIENTRY nop_RasterPos2s(GLshort x, GLshort y)
{
    (void) x; (void) y;
}

static void GLAPIENTRY nop_RasterPos2sv(const GLshort * v)
{
    (void) v;
}

static void GLAPIENTRY nop_RasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
    (void) x; (void) y; (void) z;
}

static void GLAPIENTRY nop_RasterPos3dv(const GLdouble * v)
{
    (void) v;
}

static void GLAPIENTRY nop_RasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
    (void) x; (void) y; (void) z;
}

static void GLAPIENTRY nop_RasterPos3fv(const GLfloat * v)
{
    (void) v;
}

static void GLAPIENTRY nop_RasterPos3i(GLint x, GLint y, GLint z)
{
    (void) x; (void) y; (void) z;
}

static void GLAPIENTRY nop_RasterPos3iv(const GLint * v)
{
    (void) v;
}

static void GLAPIENTRY nop_RasterPos3s(GLshort x, GLshort y, GLshort z)
{
    (void) x; (void) y; (void) z;
}

static void GLAPIENTRY nop_RasterPos3sv(const GLshort * v)
{
    (void) v;
}

static void GLAPIENTRY nop_RasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    (void) x; (void) y; (void) z; (void) w;
}

static void GLAPIENTRY nop_RasterPos4dv(const GLdouble * v)
{
    (void) v;
}

static void GLAPIENTRY nop_RasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    (void) x; (void) y; (void) z; (void) w;
}

static void GLAPIENTRY nop_RasterPos4fv(const GLfloat * v)
{
    (void) v;
}

static void GLAPIENTRY nop_RasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
    (void) x; (void) y; (void) z; (void) w;
}

static void GLAPIENTRY nop_RasterPos4iv(const GLint * v)
{
    (void) v;
}

static void GLAPIENTRY nop_RasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    (void) x; (void) y; (void) z; (void) w;
}

static void GLAPIENTRY nop_RasterPos4sv(const GLshort * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Rectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
    (void) x1; (void) y1; (void) x2; (void) y2;
}

static void GLAPIENTRY nop_Rectdv(const GLdouble * v1, const GLdouble * v2)
{
    (void) v1; (void) v2;
}

static void GLAPIENTRY nop_Rectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    (void) x1; (void) y1; (void) x2; (void) y2;
}

static void GLAPIENTRY nop_Rectfv(const GLfloat * v1, const GLfloat * v2)
{
    (void) v1; (void) v2;
}

static void GLAPIENTRY nop_Recti(GLint x1, GLint y1, GLint x2, GLint y2)
{
    (void) x1; (void) y1; (void) x2; (void) y2;
}

static void GLAPIENTRY nop_Rectiv(const GLint * v1, const GLint * v2)
{
    (void) v1; (void) v2;
}

static void GLAPIENTRY nop_Rects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
    (void) x1; (void) y1; (void) x2; (void) y2;
}

static void GLAPIENTRY nop_Rectsv(const GLshort * v1, const GLshort * v2)
{
    (void) v1; (void) v2;
}

static void GLAPIENTRY nop_TexCoord1d(GLdouble s)
{
    (void) s;
}

static void GLAPIENTRY nop_TexCoord1dv(const GLdouble * v)
{
    (void) v;
}

static void GLAPIENTRY nop_TexCoord1f(GLfloat s)
{
    (void) s;
}

static void GLAPIENTRY nop_TexCoord1fv(const GLfloat * v)
{
    (void) v;
}

static void GLAPIENTRY nop_TexCoord1i(GLint s)
{
    (void) s;
}

static void GLAPIENTRY nop_TexCoord1iv(const GLint * v)
{
    (void) v;
}

static void GLAPIENTRY nop_TexCoord1s(GLshort s)
{
    (void) s;
}

static void GLAPIENTRY nop_TexCoord1sv(const GLshort * v)
{
    (void) v;
}

static void GLAPIENTRY nop_TexCoord2d(GLdouble s, GLdouble t)
{
    (void) s; (void) t;
}

static void GLAPIENTRY nop_TexCoord2dv(const GLdouble * v)
{
    (void) v;
}

static void GLAPIENTRY nop_TexCoord2f(GLfloat s, GLfloat t)
{
    (void) s; (void) t;
}

static void GLAPIENTRY nop_TexCoord2fv(const GLfloat * v)
{
    (void) v;
}

static void GLAPIENTRY nop_TexCoord2i(GLint s, GLint t)
{
    (void) s; (void) t;
}

static void GLAPIENTRY nop_TexCoord2iv(const GLint * v)
{
    (void) v;
}

static void GLAPIENTRY nop_TexCoord2s(GLshort s, GLshort t)
{
    (void) s; (void) t;
}

static void GLAPIENTRY nop_TexCoord2sv(const GLshort * v)
{
    (void) v;
}

static void GLAPIENTRY nop_TexCoord3d(GLdouble s, GLdouble t, GLdouble r)
{
    (void) s; (void) t; (void) r;
}

static void GLAPIENTRY nop_TexCoord3dv(const GLdouble * v)
{
    (void) v;
}

static void GLAPIENTRY nop_TexCoord3f(GLfloat s, GLfloat t, GLfloat r)
{
    (void) s; (void) t; (void) r;
}

static void GLAPIENTRY nop_TexCoord3fv(const GLfloat * v)
{
    (void) v;
}

static void GLAPIENTRY nop_TexCoord3i(GLint s, GLint t, GLint r)
{
    (void) s; (void) t; (void) r;
}

static void GLAPIENTRY nop_TexCoord3iv(const GLint * v)
{
    (void) v;
}

static void GLAPIENTRY nop_TexCoord3s(GLshort s, GLshort t, GLshort r)
{
    (void) s; (void) t; (void) r;
}

static void GLAPIENTRY nop_TexCoord3sv(const GLshort * v)
{
    (void) v;
}

static void GLAPIENTRY nop_TexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
    (void) s; (void) t; (void) r; (void) q;
}

static void GLAPIENTRY nop_TexCoord4dv(const GLdouble * v)
{
    (void) v;
}

static void GLAPIENTRY nop_TexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    (void) s; (void) t; (void) r; (void) q;
}

static void GLAPIENTRY nop_TexCoord4fv(const GLfloat * v)
{
    (void) v;
}

static void GLAPIENTRY nop_TexCoord4i(GLint s, GLint t, GLint r, GLint q)
{
    (void) s; (void) t; (void) r; (void) q;
}

static void GLAPIENTRY nop_TexCoord4iv(const GLint * v)
{
    (void) v;
}

static void GLAPIENTRY nop_TexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q)
{
    (void) s; (void) t; (void) r; (void) q;
}

static void GLAPIENTRY nop_TexCoord4sv(const GLshort * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Vertex2d(GLdouble x, GLdouble y)
{
    (void) x; (void) y;
}

static void GLAPIENTRY nop_Vertex2dv(const GLdouble * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Vertex2f(GLfloat x, GLfloat y)
{
    (void) x; (void) y;
}

static void GLAPIENTRY nop_Vertex2fv(const GLfloat * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Vertex2i(GLint x, GLint y)
{
    (void) x; (void) y;
}

static void GLAPIENTRY nop_Vertex2iv(const GLint * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Vertex2s(GLshort x, GLshort y)
{
    (void) x; (void) y;
}

static void GLAPIENTRY nop_Vertex2sv(const GLshort * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Vertex3d(GLdouble x, GLdouble y, GLdouble z)
{
    (void) x; (void) y; (void) z;
}

static void GLAPIENTRY nop_Vertex3dv(const GLdouble * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Vertex3f(GLfloat x, GLfloat y, GLfloat z)
{
    (void) x; (void) y; (void) z;
}

static void GLAPIENTRY nop_Vertex3fv(const GLfloat * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Vertex3i(GLint x, GLint y, GLint z)
{
    (void) x; (void) y; (void) z;
}

static void GLAPIENTRY nop_Vertex3iv(const GLint * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Vertex3s(GLshort x, GLshort y, GLshort z)
{
    (void) x; (void) y; (void) z;
}

static void GLAPIENTRY nop_Vertex3sv(const GLshort * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Vertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    (void) x; (void) y; (void) z; (void) w;
}

static void GLAPIENTRY nop_Vertex4dv(const GLdouble * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Vertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    (void) x; (void) y; (void) z; (void) w;
}

static void GLAPIENTRY nop_Vertex4fv(const GLfloat * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Vertex4i(GLint x, GLint y, GLint z, GLint w)
{
    (void) x; (void) y; (void) z; (void) w;
}

static void GLAPIENTRY nop_Vertex4iv(const GLint * v)
{
    (void) v;
}

static void GLAPIENTRY nop_Vertex4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    (void) x; (void) y; (void) z; (void) w;
}

static void GLAPIENTRY nop_Vertex4sv(const GLshort * v)
{
    (void) v;
}

static void GLAPIENTRY nop_ClipPlane(GLenum plane, const GLdouble * equation)
{
    (void) plane; (void) equation;
}

static void GLAPIENTRY nop_ColorMaterial(GLenum face, GLenum mode)
{
    (void) face; (void) mode;
}

static void GLAPIENTRY nop_CullFace(GLenum mode)
{
    (void) mode;
}

static void GLAPIENTRY nop_Fogf(GLenum pname, GLfloat param)
{
    (void) pname; (void) param;
}

static void GLAPIENTRY nop_Fogfv(GLenum pname, const GLfloat * params)
{
    (void) pname; (void) params;
}

static void GLAPIENTRY nop_Fogi(GLenum pname, GLint param)
{
    (void) pname; (void) param;
}

static void GLAPIENTRY nop_Fogiv(GLenum pname, const GLint * params)
{
    (void) pname; (void) params;
}

static void GLAPIENTRY nop_FrontFace(GLenum mode)
{
    (void) mode;
}

static void GLAPIENTRY nop_Hint(GLenum target, GLenum mode)
{
    (void) target; (void) mode;
}

static void GLAPIENTRY nop_Lightf(GLenum light, GLenum pname, GLfloat param)
{
    (void) light; (void) pname; (void) param;
}

static void GLAPIENTRY nop_Lightfv(GLenum light, GLenum pname, const GLfloat * params)
{
    (void) light; (void) pname; (void) params;
}

static void GLAPIENTRY nop_Lighti(GLenum light, GLenum pname, GLint param)
{
    (void) light; (void) pname; (void) param;
}

static void GLAPIENTRY nop_Lightiv(GLenum light, GLenum pname, const GLint * params)
{
    (void) light; (void) pname; (void) params;
}

static void GLAPIENTRY nop_LightModelf(GLenum pname, GLfloat param)
{
    (void) pname; (void) param;
}

static void GLAPIENTRY nop_LightModelfv(GLenum pname, const GLfloat * params)
{
    (void) pname; (void) params;
}

static void GLAPIENTRY nop_LightModeli(GLenum pname, GLint param)
{
    (void) pname; (void) param;
}

static void GLAPIENTRY nop_LightModeliv(GLenum pname, const GLint * params)
{
    (void) pname; (void) params;
}

static void GLAPIENTRY nop_LineStipple(GLint factor, GLushort pattern)
{
    (void) factor; (void) pattern;
}

static void GLAPIENTRY nop_LineWidth(GLfloat width)
{
    (void) width;
}

static void GLAPIENTRY nop_Materialf(GLenum face, GLenum pname, GLfloat param)
{
    (void) face; (void) pname; (void) param;
}

static void GLAPIENTRY nop_Materialfv(GLenum face, GLenum pname, const GLfloat * params)
{
    (void) face; (void) pname; (void) params;
}

static void GLAPIENTRY nop_Materiali(GLenum face, GLenum pname, GLint param)
{
    (void) face; (void) pname; (void) param;
}

static void GLAPIENTRY nop_Materialiv(GLenum face, GLenum pname, const GLint * params)
{
    (void) face; (void) pname; (void) params;
}

static void GLAPIENTRY nop_PointSize(GLfloat size)
{
    (void) size;
}

static void GLAPIENTRY nop_PolygonMode(GLenum face, GLenum mode)
{
    (void) face; (void) mode;
}

static void GLAPIENTRY nop_PolygonStipple(const GLubyte * mask)
{
    (void) mask;
}

static void GLAPIENTRY nop_Scissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    (void) x; (void) y; (void) width; (void) height;
}

static void GLAPIENTRY nop_ShadeModel(GLenum mode)
{
    (void) mode;
}

static void GLAPIENTRY nop_TexParameterf(GLenum target, GLenum pname, GLfloat param)
{
    (void) target; (void) pname; (void) param;
}

static void GLAPIENTRY nop_TexParameterfv(GLenum target, GLenum pname, const GLfloat * params)
{
    (void) target; (void) pname; (void) params;
}

static void GLAPIENTRY nop_TexParameteri(GLenum target, GLenum pname, GLint param)
{
    (void) target; (void) pname; (void) param;
}

static void GLAPIENTRY nop_TexParameteriv(GLenum target, GLenum pname, const GLint * params)
{
    (void) target; (void) pname; (void) params;
}

static void GLAPIENTRY nop_TexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid * pixels)
{
    (void) target; (void) level; (void) internalformat; (void) width; (void) border; (void) format; (void) type; (void) pixels;
}

static void GLAPIENTRY nop_TexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * pixels)
{
    (void) target; (void) level; (void) internalformat; (void) width; (void) height; (void) border; (void) format; (void) type; (void) pixels;
}

static void GLAPIENTRY nop_TexEnvf(GLenum target, GLenum pname, GLfloat param)
{
    (void) target; (void) pname; (void) param;
}

static void GLAPIENTRY nop_TexEnvfv(GLenum target, GLenum pname, const GLfloat * params)
{
    (void) target; (void) pname; (void) params;
}

static void GLAPIENTRY nop_TexEnvi(GLenum target, GLenum pname, GLint param)
{
    (void) target; (void) pname; (void) param;
}

static void GLAPIENTRY nop_TexEnviv(GLenum target, GLenum pname, const GLint * params)
{
    (void) target; (void) pname; (void) params;
}

static void GLAPIENTRY nop_TexGend(GLenum coord, GLenum pname, GLdouble param)
{
    (void) coord; (void) pname; (void) param;
}

static void GLAPIENTRY nop_TexGendv(GLenum coord, GLenum pname, const GLdouble * params)
{
    (void) coord; (void) pname; (void) params;
}

static void GLAPIENTRY nop_TexGenf(GLenum coord, GLenum pname, GLfloat param)
{
    (void) coord; (void) pname; (void) param;
}

static void GLAPIENTRY nop_TexGenfv(GLenum coord, GLenum pname, const GLfloat * params)
{
    (void) coord; (void) pname; (void) params;
}

static void GLAPIENTRY nop_TexGeni(GLenum coord, GLenum pname, GLint param)
{
    (void) coord; (void) pname; (void) param;
}

static void GLAPIENTRY nop_TexGeniv(GLenum coord, GLenum pname, const GLint * params)
{
    (void) coord; (void) pname; (void) params;
}

static void GLAPIENTRY nop_FeedbackBuffer(GLsizei size, GLenum type, GLfloat * buffer)
{
    (void) size; (void) type; (void) buffer;
}

static void GLAPIENTRY nop_SelectBuffer(GLsizei size, GLuint * buffer)
{
    (void) size; (void) buffer;
}

static GLint GLAPIENTRY nop_RenderMode(GLenum mode)
{
    (void) mode;
    return 0;
}

static void GLAPIENTRY nop_InitNames(void)
{
}

static void GLAPIENTRY nop_LoadName(GLuint name)
{
    (void) name;
}

static void GLAPIENTRY nop_PassThrough(GLfloat token)
{
    (void) token;
}

static void GLAPIENTRY nop_PopName(void)
{
}

static void GLAPIENTRY nop_PushName(GLuint name)
{
    (void) name;
}

static void GLAPIENTRY nop_DrawBuffer(GLenum mode)
{
    (void) mode;
}

static void GLAPIENTRY nop_Clear(GLbitfield mask)
{
    (void) mask;
}

static void GLAPIENTRY nop_ClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    (void) red; (void) green; (void) blue; (void) alpha;
}

static void GLAPIENTRY nop_ClearIndex(GLfloat c)
{
    (void) c;
}

static void GLAPIENTRY nop_ClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    (void) red; (void) green; (void) blue; (void) alpha;
}

static void GLAPIENTRY nop_ClearStencil(GLint s)
{
    (void) s;
}

static void GLAPIENTRY nop_ClearDepth(GLclampd depth)
{
    (void) depth;
}

static void GLAPIENTRY nop_StencilMask(GLuint mask)
{
    (void) mask;
}

static void GLAPIENTRY nop_ColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    (void) red; (void) green; (void) blue; (void) alpha;
}

static void GLAPIENTRY nop_DepthMask(GLboolean flag)
{
    (void) flag;
}

static void GLAPIENTRY nop_IndexMask(GLuint mask)
{
    (void) mask;
}

static void GLAPIENTRY nop_Accum(GLenum op, GLfloat value)
{
    (void) op; (void) value;
}

static void GLAPIENTRY nop_Disable(GLenum cap)
{
    (void) cap;
}

static void GLAPIENTRY nop_Enable(GLenum cap)
{
    (void) cap;
}

static void GLAPIENTRY nop_Finish(void)
{
}

static void GLAPIENTRY nop_Flush(void)
{
}

static void GLAPIENTRY nop_PopAttrib(void)
{
}

static void GLAPIENTRY nop_PushAttrib(GLbitfield mask)
{
    (void) mask;
}

static void GLAPIENTRY nop_Map1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble * points)
{
    (void) target; (void) u1; (void) u2; (void) stride; (void) order; (void) points;
}

static void GLAPIENTRY nop_Map1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat * points)
{
    (void) target; (void) u1; (void) u2; (void) stride; (void) order; (void) points;
}

static void GLAPIENTRY nop_Map2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble * points)
{
    (void) target; (void) u1; (void) u2; (void) ustride; (void) uorder; (void) v1; (void) v2; (void) vstride; (void) vorder; (void) points;
}

static void GLAPIENTRY nop_Map2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat * points)
{
    (void) target; (void) u1; (void) u2; (void) ustride; (void) uorder; (void) v1; (void) v2; (void) vstride; (void) vorder; (void) points;
}

static void GLAPIENTRY nop_MapGrid1d(GLint un, GLdouble u1, GLdouble u2)
{
    (void) un; (void) u1; (void) u2;
}

static void GLAPIENTRY nop_MapGrid1f(GLint un, GLfloat u1, GLfloat u2)
{
    (void) un; (void) u1; (void) u2;
}

static void GLAPIENTRY nop_MapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
    (void) un; (void) u1; (void) u2; (void) vn; (void) v1; (void) v2;
}

static void GLAPIENTRY nop_MapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
    (void) un; (void) u1; (void) u2; (void) vn; (void) v1; (void) v2;
}

static void GLAPIENTRY nop_EvalCoord1d(GLdouble u)
{
    (void) u;
}

static void GLAPIENTRY nop_EvalCoord1dv(const GLdouble * u)
{
    (void) u;
}

static void GLAPIENTRY nop_EvalCoord1f(GLfloat u)
{
    (void) u;
}

static void GLAPIENTRY nop_EvalCoord1fv(const GLfloat * u)
{
    (void) u;
}

static void GLAPIENTRY nop_EvalCoord2d(GLdouble u, GLdouble v)
{
    (void) u; (void) v;
}

static void GLAPIENTRY nop_EvalCoord2dv(const GLdouble * u)
{
    (void) u;
}

static void GLAPIENTRY nop_EvalCoord2f(GLfloat u, GLfloat v)
{
    (void) u; (void) v;
}

static void GLAPIENTRY nop_EvalCoord2fv(const GLfloat * u)
{
    (void) u;
}

static void GLAPIENTRY nop_EvalMesh1(GLenum mode, GLint i1, GLint i2)
{
    (void) mode; (void) i1; (void) i2;
}

static void GLAPIENTRY nop_EvalPoint1(GLint i)
{
    (void) i;
}

static void GLAPIENTRY nop_EvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
    (void) mode; (void) i1; (void) i2; (void) j1; (void) j2;
}

static void GLAPIENTRY nop_EvalPoint2(GLint i, GLint j)
{
    (void) i; (void) j;
}

static void GLAPIENTRY nop_AlphaFunc(GLenum func, GLclampf ref)
{
    (void) func; (void) ref;
}

static void GLAPIENTRY nop_BlendFunc(GLenum sfactor, GLenum dfactor)
{
    (void) sfactor; (void) dfactor;
}

static void GLAPIENTRY nop_LogicOp(GLenum opcode)
{
    (void) opcode;
}

static void GLAPIENTRY nop_StencilFunc(GLenum func, GLint ref, GLuint mask)
{
    (void) func; (void) ref; (void) mask;
}

static void GLAPIENTRY nop_StencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
    (void) fail; (void) zfail; (void) zpass;
}

static void GLAPIENTRY nop_DepthFunc(GLenum func)
{
    (void) func;
}

static void GLAPIENTRY nop_PixelZoom(GLfloat xfactor, GLfloat yfactor)
{
    (void) xfactor; (void) yfactor;
}

static void GLAPIENTRY nop_PixelTransferf(GLenum pname, GLfloat param)
{
    (void) pname; (void) param;
}

static void GLAPIENTRY nop_PixelTransferi(GLenum pname, GLint param)
{
    (void) pname; (void) param;
}

static void GLAPIENTRY nop_PixelStoref(GLenum pname, GLfloat param)
{
    (void) pname; (void) param;
}

static void GLAPIENTRY nop_PixelStorei(GLenum pname, GLint param)
{
    (void) pname; (void) param;
}

static void GLAPIENTRY nop_PixelMapfv(GLenum map, GLsizei mapsize, const GLfloat * values)
{
    (void) map; (void) mapsize; (void) values;
}

static void GLAPIENTRY nop_PixelMapuiv(GLenum map, GLsizei mapsize, const GLuint * values)
{
    (void) map; (void) mapsize; (void) values;
}

static void GLAPIENTRY nop_PixelMapusv(GLenum map, GLsizei mapsize, const GLushort * values)
{
    (void) map; (void) mapsize; (void) values;
}

static void GLAPIENTRY nop_ReadBuffer(GLenum mode)
{
    (void) mode;
}

static void GLAPIENTRY nop_CopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
    (void) x; (void) y; (void) width; (void) height; (void) type;
}

static void GLAPIENTRY nop_ReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * pixels)
{
    (void) x; (void) y; (void) width; (void) height; (void) format; (void) type; (void) pixels;
}

static void GLAPIENTRY nop_DrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels)
{
    (void) width; (void) height; (void) format; (void) type; (void) pixels;
}

static void GLAPIENTRY nop_GetBooleanv(GLenum pname, GLboolean * params)
{
    (void) pname; (void) params;
}

static void GLAPIENTRY nop_GetClipPlane(GLenum plane, GLdouble * equation)
{
    (void) plane; (void) equation;
}

static void GLAPIENTRY nop_GetDoublev(GLenum pname, GLdouble * params)
{
    (void) pname; (void) params;
}

static GLenum GLAPIENTRY nop_GetError(void)
{
    return 0;
}

static void GLAPIENTRY nop_GetFloatv(GLenum pname, GLfloat * params)
{
    (void) pname; (void) params;
}

static void GLAPIENTRY nop_GetIntegerv(GLenum pname, GLint * params)
{
    (void) pname; (void) params;
}

static void GLAPIENTRY nop_GetLightfv(GLenum light, GLenum pname, GLfloat * params)
{
    (void) light; (void) pname; (void) params;
}

static void GLAPIENTRY nop_GetLightiv(GLenum light, GLenum pname, GLint * params)
{
    (void) light; (void) pname; (void) params;
}

static void GLAPIENTRY nop_GetMapdv(GLenum target, GLenum query, GLdouble * v)
{
    (void) target; (void) query; (void) v;
}

static void GLAPIENTRY nop_GetMapfv(GLenum target, GLenum query, GLfloat * v)
{
    (void) target; (void) query; (void) v;
}

static void GLAPIENTRY nop_GetMapiv(GLenum target, GLenum query, GLint * v)
{
    (void) target; (void) query; (void) v;
}

static void GLAPIENTRY nop_GetMaterialfv(GLenum face, GLenum pname, GLfloat * params)
{
    (void) face; (void) pname; (void) params;
}

static void GLAPIENTRY nop_GetMaterialiv(GLenum face, GLenum pname, GLint * params)
{
    (void) face; (void) pname; (void) params;
}

static void GLAPIENTRY nop_GetPixelMapfv(GLenum map, GLfloat * values)
{
    (void) map; (void) values;
}

static void GLAPIENTRY nop_GetPixelMapuiv(GLenum map, GLuint * values)
{
    (void) map; (void) values;
}

static void GLAPIENTRY nop_GetPixelMapusv(GLenum map, GLushort * values)
{
    (void) map; (void) values;
}

static void GLAPIENTRY nop_GetPolygonStipple(GLubyte * mask)
{
    (void) mask;
}

static const GLubyte * GLAPIENTRY nop_GetString(GLenum name)
{
    (void) name;
    return (const GLubyte*)"";
}

static void GLAPIENTRY nop_GetTexEnvfv(GLenum target, GLenum pname, GLfloat * params)
{
    (void) target; (void) pname; (void) params;
}

static void GLAPIENTRY nop_GetTexEnviv(GLenum target, GLenum pname, GLint * params)
{
    (void) target; (void) pname; (void) params;
}

static void GLAPIENTRY nop_GetTexGendv(GLenum coord, GLenum pname, GLdouble * params)
{
    (void) coord; (void) pname; (void) params;
}

static void GLAPIENTRY nop_GetTexGenfv(GLenum coord, GLenum pname, GLfloat * params)
{
    (void) coord; (void) pname; (void) params;
}

static void GLAPIENTRY nop_GetTexGeniv(GLenum coord, GLenum pname, GLint * params)
{
    (void) coord; (void) pname; (void) params;
}

static void GLAPIENTRY nop_GetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid * pixels)
{
    (void) target; (void) level; (void) format; (void) type; (void) pixels;
}

static void GLAPIENTRY nop_GetTexParameterfv(GLenum target, GLenum pname, GLfloat * params)
{
    (void) target; (void) pname; (void) params;
}

static void GLAPIENTRY nop_GetTexParameteriv(GLenum target, GLenum pname, GLint * params)
{
    (void) target; (void) pname; (void) params;
}

static void GLAPIENTRY nop_GetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat * params)
{
    (void) target; (void) level; (void) pname; (void) params;
}

static void GLAPIENTRY nop_GetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint * params)
{
    (void) target; (void) level; (void) pname; (void) params;
}

static GLboolean GLAPIENTRY nop_IsEnabled(GLenum cap)
{
    (void) cap;
    return 0;
}

static GLboolean GLAPIENTRY nop_IsList(GLuint list)
{
    (void) list;
    return 0;
}

static void GLAPIENTRY nop_DepthRange(GLclampd zNear, GLclampd zFar)
{
    (void) zNear; (void) zFar;
}

static void GLAPIENTRY nop_Frustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    (void) left; (void) right; (void) bottom; (void) top; (void) zNear; (void) zFar;
}

static void GLAPIENTRY nop_LoadIdentity(void)
{
}

static void GLAPIENTRY nop_LoadMatrixf(const GLfloat * m)
{
    (void) m;
}

static void GLAPIENTRY nop_LoadMatrixd(const GLdouble * m)
{
    (void) m;
}

static void GLAPIENTRY nop_MatrixMode(GLenum mode)
{
    (void) mode;
}

static void GLAPIENTRY nop_MultMatrixf(const GLfloat * m)
{
    (void) m;
}

static void GLAPIENTRY nop_MultMatrixd(const GLdouble * m)
{
    (void) m;
}

static void GLAPIENTRY nop_Ortho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    (void) left; (void) right; (void) bottom; (void) top; (void) zNear; (void) zFar;
}

static void GLAPIENTRY nop_PopMatrix(void)
{
}

static void GLAPIENTRY nop_PushMatrix(void)
{
}

static void GLAPIENTRY nop_Rotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
    (void) angle; (void) x; (void) y; (void) z;
}

static void GLAPIENTRY nop_Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    (void) angle; (void) x; (void) y; (void) z;
}

static void GLAPIENTRY nop_Scaled(GLdouble x, GLdouble y, GLdouble z)
{
    (void) x; (void) y; (void) z;
}

static void GLAPIENTRY nop_Scalef(GLfloat x, GLfloat y, GLfloat z)
{
    (void) x; (void) y; (void) z;
}

static void GLAPIENTRY nop_Translated(GLdouble x, GLdouble y, GLdouble z)
{
    (void) x; (void) y; (void) z;
}

static void GLAPIENTRY nop_Translatef(GLfloat x, GLfloat y, GLfloat z)
{
    (void) x; (void) y; (void) z;
}

static void GLAPIENTRY nop_Viewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    (void) x; (void) y; (void) width; (void) height;
}

static void GLAPIENTRY nop_ArrayElement(GLint i)
{
    (void) i;
}

static void GLAPIENTRY nop_BindTexture(GLenum target, GLuint texture)
{
    (void) target; (void) texture;
}

static void GLAPIENTRY nop_ColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
    (void) size; (void) type; (void) stride; (void) pointer;
}

static void GLAPIENTRY nop_DisableClientState(GLenum array)
{
    (void) array;
}

static void GLAPIENTRY nop_DrawArrays(GLenum mode, GLint first, GLsizei count)
{
    (void) mode; (void) first; (void) count;
}

static void GLAPIENTRY nop_DrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid * indices)
{
    (void) mode; (void) count; (void) type; (void) indices;
}

static void GLAPIENTRY nop_EdgeFlagPointer(GLsizei stride, const GLvoid * pointer)
{
    (void) stride; (void) pointer;
}

static void GLAPIENTRY nop_EnableClientState(GLenum array)
{
    (void) array;
}

static void GLAPIENTRY nop_IndexPointer(GLenum type, GLsizei stride, const GLvoid * pointer)
{
    (void) type; (void) stride; (void) pointer;
}

static void GLAPIENTRY nop_Indexub(GLubyte c)
{
    (void) c;
}

static void GLAPIENTRY nop_Indexubv(const GLubyte * c)
{
    (void) c;
}

static void GLAPIENTRY nop_InterleavedArrays(GLenum format, GLsizei stride, const GLvoid * pointer)
{
    (void) format; (void) stride; (void) pointer;
}

static void GLAPIENTRY nop_NormalPointer(GLenum type, GLsizei stride, const GLvoid * pointer)
{
    (void) type; (void) stride; (void) pointer;
}

static void GLAPIENTRY nop_PolygonOffset(GLfloat factor, GLfloat units)
{
    (void) factor; (void) units;
}

static void GLAPIENTRY nop_TexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
    (void) size; (void) type; (void) stride; (void) pointer;
}

static void GLAPIENTRY nop_VertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
    (void) size; (void) type; (void) stride; (void) pointer;
}

static GLboolean GLAPIENTRY nop_AreTexturesResident(GLsizei n, const GLuint * textures, GLboolean * residences)
{
    (void) n; (void) textures; (void) residences;
    return 0;
}

static void GLAPIENTRY nop_CopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
    (void) target; (void) level; (void) internalformat; (void) x; (void) y; (void) width; (void) border;
}

static void GLAPIENTRY nop_CopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    (void) target; (void) level; (void) internalformat; (void) x; (void) y; (void) width; (void) height; (void) border;
}

static void GLAPIENTRY nop_CopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
    (void) target; (void) level; (void) xoffset; (void) x; (void) y; (void) width;
}

static void GLAPIENTRY nop_CopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    (void) target; (void) level; (void) xoffset; (void) yoffset; (void) x; (void) y; (void) width; (void) height;
}

static void GLAPIENTRY nop_DeleteTextures(GLsizei n, const GLuint * textures)
{
    (void) n; (void) textures;
}

static void GLAPIENTRY nop_GenTextures(GLsizei n, GLuint * textures)
{
    (void) n; (void) textures;
}

static void GLAPIENTRY nop_GetPointerv(GLenum pname, GLvoid ** params)
{
    (void) pname; (void) params;
}

static GLboolean GLAPIENTRY nop_IsTexture(GLuint texture)
{
    (void) texture;
    return 0;
}

static void GLAPIENTRY nop_PrioritizeTextures(GLsizei n, const GLuint * textures, const GLclampf * priorities)
{
    (void) n; (void) textures; (void) priorities;
}

static void GLAPIENTRY nop_TexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid * pixels)
{
    (void) target; (void) level; (void) xoffset; (void) width; (void) format; (void) type; (void) pixels;
}

static void GLAPIENTRY nop_TexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels)
{
    (void) target; (void) level; (void) xoffset; (void) yoffset; (void) width; (void) height; (void) format; (void) type; (void) pixels;
}

static void GLAPIENTRY nop_PopClientAttrib(void)
{
}

static void GLAPIENTRY nop_PushClientAttrib(GLbitfield mask)
{
    (void) mask;
}

const GLCLTPROCTABLE StubTable =
{
    OPENGL_VERSION_110_ENTRIES,
    {
        nop_NewList,
        nop_EndList,
        nop_CallList,
        nop_CallLists,
        nop_DeleteLists,
        nop_GenLists,
        nop_ListBase,
        nop_Begin,
        nop_Bitmap,
        nop_Color3b,
        nop_Color3bv,
        nop_Color3d,
        nop_Color3dv,
        nop_Color3f,
        nop_Color3fv,
        nop_Color3i,
        nop_Color3iv,
        nop_Color3s,
        nop_Color3sv,
        nop_Color3ub,
        nop_Color3ubv,
        nop_Color3ui,
        nop_Color3uiv,
        nop_Color3us,
        nop_Color3usv,
        nop_Color4b,
        nop_Color4bv,
        nop_Color4d,
        nop_Color4dv,
        nop_Color4f,
        nop_Color4fv,
        nop_Color4i,
        nop_Color4iv,
        nop_Color4s,
        nop_Color4sv,
        nop_Color4ub,
        nop_Color4ubv,
        nop_Color4ui,
        nop_Color4uiv,
        nop_Color4us,
        nop_Color4usv,
        nop_EdgeFlag,
        nop_EdgeFlagv,
        nop_End,
        nop_Indexd,
        nop_Indexdv,
        nop_Indexf,
        nop_Indexfv,
        nop_Indexi,
        nop_Indexiv,
        nop_Indexs,
        nop_Indexsv,
        nop_Normal3b,
        nop_Normal3bv,
        nop_Normal3d,
        nop_Normal3dv,
        nop_Normal3f,
        nop_Normal3fv,
        nop_Normal3i,
        nop_Normal3iv,
        nop_Normal3s,
        nop_Normal3sv,
        nop_RasterPos2d,
        nop_RasterPos2dv,
        nop_RasterPos2f,
        nop_RasterPos2fv,
        nop_RasterPos2i,
        nop_RasterPos2iv,
        nop_RasterPos2s,
        nop_RasterPos2sv,
        nop_RasterPos3d,
        nop_RasterPos3dv,
        nop_RasterPos3f,
        nop_RasterPos3fv,
        nop_RasterPos3i,
        nop_RasterPos3iv,
        nop_RasterPos3s,
        nop_RasterPos3sv,
        nop_RasterPos4d,
        nop_RasterPos4dv,
        nop_RasterPos4f,
        nop_RasterPos4fv,
        nop_RasterPos4i,
        nop_RasterPos4iv,
        nop_RasterPos4s,
        nop_RasterPos4sv,
        nop_Rectd,
        nop_Rectdv,
        nop_Rectf,
        nop_Rectfv,
        nop_Recti,
        nop_Rectiv,
        nop_Rects,
        nop_Rectsv,
        nop_TexCoord1d,
        nop_TexCoord1dv,
        nop_TexCoord1f,
        nop_TexCoord1fv,
        nop_TexCoord1i,
        nop_TexCoord1iv,
        nop_TexCoord1s,
        nop_TexCoord1sv,
        nop_TexCoord2d,
        nop_TexCoord2dv,
        nop_TexCoord2f,
        nop_TexCoord2fv,
        nop_TexCoord2i,
        nop_TexCoord2iv,
        nop_TexCoord2s,
        nop_TexCoord2sv,
        nop_TexCoord3d,
        nop_TexCoord3dv,
        nop_TexCoord3f,
        nop_TexCoord3fv,
        nop_TexCoord3i,
        nop_TexCoord3iv,
        nop_TexCoord3s,
        nop_TexCoord3sv,
        nop_TexCoord4d,
        nop_TexCoord4dv,
        nop_TexCoord4f,
        nop_TexCoord4fv,
        nop_TexCoord4i,
        nop_TexCoord4iv,
        nop_TexCoord4s,
        nop_TexCoord4sv,
        nop_Vertex2d,
        nop_Vertex2dv,
        nop_Vertex2f,
        nop_Vertex2fv,
        nop_Vertex2i,
        nop_Vertex2iv,
        nop_Vertex2s,
        nop_Vertex2sv,
        nop_Vertex3d,
        nop_Vertex3dv,
        nop_Vertex3f,
        nop_Vertex3fv,
        nop_Vertex3i,
        nop_Vertex3iv,
        nop_Vertex3s,
        nop_Vertex3sv,
        nop_Vertex4d,
        nop_Vertex4dv,
        nop_Vertex4f,
        nop_Vertex4fv,
        nop_Vertex4i,
        nop_Vertex4iv,
        nop_Vertex4s,
        nop_Vertex4sv,
        nop_ClipPlane,
        nop_ColorMaterial,
        nop_CullFace,
        nop_Fogf,
        nop_Fogfv,
        nop_Fogi,
        nop_Fogiv,
        nop_FrontFace,
        nop_Hint,
        nop_Lightf,
        nop_Lightfv,
        nop_Lighti,
        nop_Lightiv,
        nop_LightModelf,
        nop_LightModelfv,
        nop_LightModeli,
        nop_LightModeliv,
        nop_LineStipple,
        nop_LineWidth,
        nop_Materialf,
        nop_Materialfv,
        nop_Materiali,
        nop_Materialiv,
        nop_PointSize,
        nop_PolygonMode,
        nop_PolygonStipple,
        nop_Scissor,
        nop_ShadeModel,
        nop_TexParameterf,
        nop_TexParameterfv,
        nop_TexParameteri,
        nop_TexParameteriv,
        nop_TexImage1D,
        nop_TexImage2D,
        nop_TexEnvf,
        nop_TexEnvfv,
        nop_TexEnvi,
        nop_TexEnviv,
        nop_TexGend,
        nop_TexGendv,
        nop_TexGenf,
        nop_TexGenfv,
        nop_TexGeni,
        nop_TexGeniv,
        nop_FeedbackBuffer,
        nop_SelectBuffer,
        nop_RenderMode,
        nop_InitNames,
        nop_LoadName,
        nop_PassThrough,
        nop_PopName,
        nop_PushName,
        nop_DrawBuffer,
        nop_Clear,
        nop_ClearAccum,
        nop_ClearIndex,
        nop_ClearColor,
        nop_ClearStencil,
        nop_ClearDepth,
        nop_StencilMask,
        nop_ColorMask,
        nop_DepthMask,
        nop_IndexMask,
        nop_Accum,
        nop_Disable,
        nop_Enable,
        nop_Finish,
        nop_Flush,
        nop_PopAttrib,
        nop_PushAttrib,
        nop_Map1d,
        nop_Map1f,
        nop_Map2d,
        nop_Map2f,
        nop_MapGrid1d,
        nop_MapGrid1f,
        nop_MapGrid2d,
        nop_MapGrid2f,
        nop_EvalCoord1d,
        nop_EvalCoord1dv,
        nop_EvalCoord1f,
        nop_EvalCoord1fv,
        nop_EvalCoord2d,
        nop_EvalCoord2dv,
        nop_EvalCoord2f,
        nop_EvalCoord2fv,
        nop_EvalMesh1,
        nop_EvalPoint1,
        nop_EvalMesh2,
        nop_EvalPoint2,
        nop_AlphaFunc,
        nop_BlendFunc,
        nop_LogicOp,
        nop_StencilFunc,
        nop_StencilOp,
        nop_DepthFunc,
        nop_PixelZoom,
        nop_PixelTransferf,
        nop_PixelTransferi,
        nop_PixelStoref,
        nop_PixelStorei,
        nop_PixelMapfv,
        nop_PixelMapuiv,
        nop_PixelMapusv,
        nop_ReadBuffer,
        nop_CopyPixels,
        nop_ReadPixels,
        nop_DrawPixels,
        nop_GetBooleanv,
        nop_GetClipPlane,
        nop_GetDoublev,
        nop_GetError,
        nop_GetFloatv,
        nop_GetIntegerv,
        nop_GetLightfv,
        nop_GetLightiv,
        nop_GetMapdv,
        nop_GetMapfv,
        nop_GetMapiv,
        nop_GetMaterialfv,
        nop_GetMaterialiv,
        nop_GetPixelMapfv,
        nop_GetPixelMapuiv,
        nop_GetPixelMapusv,
        nop_GetPolygonStipple,
        nop_GetString,
        nop_GetTexEnvfv,
        nop_GetTexEnviv,
        nop_GetTexGendv,
        nop_GetTexGenfv,
        nop_GetTexGeniv,
        nop_GetTexImage,
        nop_GetTexParameterfv,
        nop_GetTexParameteriv,
        nop_GetTexLevelParameterfv,
        nop_GetTexLevelParameteriv,
        nop_IsEnabled,
        nop_IsList,
        nop_DepthRange,
        nop_Frustum,
        nop_LoadIdentity,
        nop_LoadMatrixf,
        nop_LoadMatrixd,
        nop_MatrixMode,
        nop_MultMatrixf,
        nop_MultMatrixd,
        nop_Ortho,
        nop_PopMatrix,
        nop_PushMatrix,
        nop_Rotated,
        nop_Rotatef,
        nop_Scaled,
        nop_Scalef,
        nop_Translated,
        nop_Translatef,
        nop_Viewport,
        nop_ArrayElement,
        nop_BindTexture,
        nop_ColorPointer,
        nop_DisableClientState,
        nop_DrawArrays,
        nop_DrawElements,
        nop_EdgeFlagPointer,
        nop_EnableClientState,
        nop_IndexPointer,
        nop_Indexub,
        nop_Indexubv,
        nop_InterleavedArrays,
        nop_NormalPointer,
        nop_PolygonOffset,
        nop_TexCoordPointer,
        nop_VertexPointer,
        nop_AreTexturesResident,
        nop_CopyTexImage1D,
        nop_CopyTexImage2D,
        nop_CopyTexSubImage1D,
        nop_CopyTexSubImage2D,
        nop_DeleteTextures,
        nop_GenTextures,
        nop_GetPointerv,
        nop_IsTexture,
        nop_PrioritizeTextures,
        nop_TexSubImage1D,
        nop_TexSubImage2D,
        nop_PopClientAttrib,
        nop_PushClientAttrib
    }
};


#ifndef __i386__
void GLAPIENTRY glNewList(GLuint list, GLenum mode)
{
    IntGetCurrentDispatchTable()->NewList(list, mode);
}

void GLAPIENTRY glEndList(void)
{
    IntGetCurrentDispatchTable()->EndList();
}

void GLAPIENTRY glCallList(GLuint list)
{
    IntGetCurrentDispatchTable()->CallList(list);
}

void GLAPIENTRY glCallLists(GLsizei n, GLenum type, const GLvoid * lists)
{
    IntGetCurrentDispatchTable()->CallLists(n, type, lists);
}

void GLAPIENTRY glDeleteLists(GLuint list, GLsizei range)
{
    IntGetCurrentDispatchTable()->DeleteLists(list, range);
}

GLuint GLAPIENTRY glGenLists(GLsizei range)
{
    return IntGetCurrentDispatchTable()->GenLists(range);
}

void GLAPIENTRY glListBase(GLuint base)
{
    IntGetCurrentDispatchTable()->ListBase(base);
}

void GLAPIENTRY glBegin(GLenum mode)
{
    IntGetCurrentDispatchTable()->Begin(mode);
}

void GLAPIENTRY glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte * bitmap)
{
    IntGetCurrentDispatchTable()->Bitmap(width, height, xorig, yorig, xmove, ymove, bitmap);
}

void GLAPIENTRY glColor3b(GLbyte red, GLbyte green, GLbyte blue)
{
    IntGetCurrentDispatchTable()->Color3b(red, green, blue);
}

void GLAPIENTRY glColor3bv(const GLbyte * v)
{
    IntGetCurrentDispatchTable()->Color3bv(v);
}

void GLAPIENTRY glColor3d(GLdouble red, GLdouble green, GLdouble blue)
{
    IntGetCurrentDispatchTable()->Color3d(red, green, blue);
}

void GLAPIENTRY glColor3dv(const GLdouble * v)
{
    IntGetCurrentDispatchTable()->Color3dv(v);
}

void GLAPIENTRY glColor3f(GLfloat red, GLfloat green, GLfloat blue)
{
    IntGetCurrentDispatchTable()->Color3f(red, green, blue);
}

void GLAPIENTRY glColor3fv(const GLfloat * v)
{
    IntGetCurrentDispatchTable()->Color3fv(v);
}

void GLAPIENTRY glColor3i(GLint red, GLint green, GLint blue)
{
    IntGetCurrentDispatchTable()->Color3i(red, green, blue);
}

void GLAPIENTRY glColor3iv(const GLint * v)
{
    IntGetCurrentDispatchTable()->Color3iv(v);
}

void GLAPIENTRY glColor3s(GLshort red, GLshort green, GLshort blue)
{
    IntGetCurrentDispatchTable()->Color3s(red, green, blue);
}

void GLAPIENTRY glColor3sv(const GLshort * v)
{
    IntGetCurrentDispatchTable()->Color3sv(v);
}

void GLAPIENTRY glColor3ub(GLubyte red, GLubyte green, GLubyte blue)
{
    IntGetCurrentDispatchTable()->Color3ub(red, green, blue);
}

void GLAPIENTRY glColor3ubv(const GLubyte * v)
{
    IntGetCurrentDispatchTable()->Color3ubv(v);
}

void GLAPIENTRY glColor3ui(GLuint red, GLuint green, GLuint blue)
{
    IntGetCurrentDispatchTable()->Color3ui(red, green, blue);
}

void GLAPIENTRY glColor3uiv(const GLuint * v)
{
    IntGetCurrentDispatchTable()->Color3uiv(v);
}

void GLAPIENTRY glColor3us(GLushort red, GLushort green, GLushort blue)
{
    IntGetCurrentDispatchTable()->Color3us(red, green, blue);
}

void GLAPIENTRY glColor3usv(const GLushort * v)
{
    IntGetCurrentDispatchTable()->Color3usv(v);
}

void GLAPIENTRY glColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
    IntGetCurrentDispatchTable()->Color4b(red, green, blue, alpha);
}

void GLAPIENTRY glColor4bv(const GLbyte * v)
{
    IntGetCurrentDispatchTable()->Color4bv(v);
}

void GLAPIENTRY glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
    IntGetCurrentDispatchTable()->Color4d(red, green, blue, alpha);
}

void GLAPIENTRY glColor4dv(const GLdouble * v)
{
    IntGetCurrentDispatchTable()->Color4dv(v);
}

void GLAPIENTRY glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    IntGetCurrentDispatchTable()->Color4f(red, green, blue, alpha);
}

void GLAPIENTRY glColor4fv(const GLfloat * v)
{
    IntGetCurrentDispatchTable()->Color4fv(v);
}

void GLAPIENTRY glColor4i(GLint red, GLint green, GLint blue, GLint alpha)
{
    IntGetCurrentDispatchTable()->Color4i(red, green, blue, alpha);
}

void GLAPIENTRY glColor4iv(const GLint * v)
{
    IntGetCurrentDispatchTable()->Color4iv(v);
}

void GLAPIENTRY glColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
    IntGetCurrentDispatchTable()->Color4s(red, green, blue, alpha);
}

void GLAPIENTRY glColor4sv(const GLshort * v)
{
    IntGetCurrentDispatchTable()->Color4sv(v);
}

void GLAPIENTRY glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    IntGetCurrentDispatchTable()->Color4ub(red, green, blue, alpha);
}

void GLAPIENTRY glColor4ubv(const GLubyte * v)
{
    IntGetCurrentDispatchTable()->Color4ubv(v);
}

void GLAPIENTRY glColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
    IntGetCurrentDispatchTable()->Color4ui(red, green, blue, alpha);
}

void GLAPIENTRY glColor4uiv(const GLuint * v)
{
    IntGetCurrentDispatchTable()->Color4uiv(v);
}

void GLAPIENTRY glColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
    IntGetCurrentDispatchTable()->Color4us(red, green, blue, alpha);
}

void GLAPIENTRY glColor4usv(const GLushort * v)
{
    IntGetCurrentDispatchTable()->Color4usv(v);
}

void GLAPIENTRY glEdgeFlag(GLboolean flag)
{
    IntGetCurrentDispatchTable()->EdgeFlag(flag);
}

void GLAPIENTRY glEdgeFlagv(const GLboolean * flag)
{
    IntGetCurrentDispatchTable()->EdgeFlagv(flag);
}

void GLAPIENTRY glEnd(void)
{
    IntGetCurrentDispatchTable()->End();
}

void GLAPIENTRY glIndexd(GLdouble c)
{
    IntGetCurrentDispatchTable()->Indexd(c);
}

void GLAPIENTRY glIndexdv(const GLdouble * c)
{
    IntGetCurrentDispatchTable()->Indexdv(c);
}

void GLAPIENTRY glIndexf(GLfloat c)
{
    IntGetCurrentDispatchTable()->Indexf(c);
}

void GLAPIENTRY glIndexfv(const GLfloat * c)
{
    IntGetCurrentDispatchTable()->Indexfv(c);
}

void GLAPIENTRY glIndexi(GLint c)
{
    IntGetCurrentDispatchTable()->Indexi(c);
}

void GLAPIENTRY glIndexiv(const GLint * c)
{
    IntGetCurrentDispatchTable()->Indexiv(c);
}

void GLAPIENTRY glIndexs(GLshort c)
{
    IntGetCurrentDispatchTable()->Indexs(c);
}

void GLAPIENTRY glIndexsv(const GLshort * c)
{
    IntGetCurrentDispatchTable()->Indexsv(c);
}

void GLAPIENTRY glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz)
{
    IntGetCurrentDispatchTable()->Normal3b(nx, ny, nz);
}

void GLAPIENTRY glNormal3bv(const GLbyte * v)
{
    IntGetCurrentDispatchTable()->Normal3bv(v);
}

void GLAPIENTRY glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz)
{
    IntGetCurrentDispatchTable()->Normal3d(nx, ny, nz);
}

void GLAPIENTRY glNormal3dv(const GLdouble * v)
{
    IntGetCurrentDispatchTable()->Normal3dv(v);
}

void GLAPIENTRY glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
    IntGetCurrentDispatchTable()->Normal3f(nx, ny, nz);
}

void GLAPIENTRY glNormal3fv(const GLfloat * v)
{
    IntGetCurrentDispatchTable()->Normal3fv(v);
}

void GLAPIENTRY glNormal3i(GLint nx, GLint ny, GLint nz)
{
    IntGetCurrentDispatchTable()->Normal3i(nx, ny, nz);
}

void GLAPIENTRY glNormal3iv(const GLint * v)
{
    IntGetCurrentDispatchTable()->Normal3iv(v);
}

void GLAPIENTRY glNormal3s(GLshort nx, GLshort ny, GLshort nz)
{
    IntGetCurrentDispatchTable()->Normal3s(nx, ny, nz);
}

void GLAPIENTRY glNormal3sv(const GLshort * v)
{
    IntGetCurrentDispatchTable()->Normal3sv(v);
}

void GLAPIENTRY glRasterPos2d(GLdouble x, GLdouble y)
{
    IntGetCurrentDispatchTable()->RasterPos2d(x, y);
}

void GLAPIENTRY glRasterPos2dv(const GLdouble * v)
{
    IntGetCurrentDispatchTable()->RasterPos2dv(v);
}

void GLAPIENTRY glRasterPos2f(GLfloat x, GLfloat y)
{
    IntGetCurrentDispatchTable()->RasterPos2f(x, y);
}

void GLAPIENTRY glRasterPos2fv(const GLfloat * v)
{
    IntGetCurrentDispatchTable()->RasterPos2fv(v);
}

void GLAPIENTRY glRasterPos2i(GLint x, GLint y)
{
    IntGetCurrentDispatchTable()->RasterPos2i(x, y);
}

void GLAPIENTRY glRasterPos2iv(const GLint * v)
{
    IntGetCurrentDispatchTable()->RasterPos2iv(v);
}

void GLAPIENTRY glRasterPos2s(GLshort x, GLshort y)
{
    IntGetCurrentDispatchTable()->RasterPos2s(x, y);
}

void GLAPIENTRY glRasterPos2sv(const GLshort * v)
{
    IntGetCurrentDispatchTable()->RasterPos2sv(v);
}

void GLAPIENTRY glRasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
    IntGetCurrentDispatchTable()->RasterPos3d(x, y, z);
}

void GLAPIENTRY glRasterPos3dv(const GLdouble * v)
{
    IntGetCurrentDispatchTable()->RasterPos3dv(v);
}

void GLAPIENTRY glRasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
    IntGetCurrentDispatchTable()->RasterPos3f(x, y, z);
}

void GLAPIENTRY glRasterPos3fv(const GLfloat * v)
{
    IntGetCurrentDispatchTable()->RasterPos3fv(v);
}

void GLAPIENTRY glRasterPos3i(GLint x, GLint y, GLint z)
{
    IntGetCurrentDispatchTable()->RasterPos3i(x, y, z);
}

void GLAPIENTRY glRasterPos3iv(const GLint * v)
{
    IntGetCurrentDispatchTable()->RasterPos3iv(v);
}

void GLAPIENTRY glRasterPos3s(GLshort x, GLshort y, GLshort z)
{
    IntGetCurrentDispatchTable()->RasterPos3s(x, y, z);
}

void GLAPIENTRY glRasterPos3sv(const GLshort * v)
{
    IntGetCurrentDispatchTable()->RasterPos3sv(v);
}

void GLAPIENTRY glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    IntGetCurrentDispatchTable()->RasterPos4d(x, y, z, w);
}

void GLAPIENTRY glRasterPos4dv(const GLdouble * v)
{
    IntGetCurrentDispatchTable()->RasterPos4dv(v);
}

void GLAPIENTRY glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    IntGetCurrentDispatchTable()->RasterPos4f(x, y, z, w);
}

void GLAPIENTRY glRasterPos4fv(const GLfloat * v)
{
    IntGetCurrentDispatchTable()->RasterPos4fv(v);
}

void GLAPIENTRY glRasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
    IntGetCurrentDispatchTable()->RasterPos4i(x, y, z, w);
}

void GLAPIENTRY glRasterPos4iv(const GLint * v)
{
    IntGetCurrentDispatchTable()->RasterPos4iv(v);
}

void GLAPIENTRY glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    IntGetCurrentDispatchTable()->RasterPos4s(x, y, z, w);
}

void GLAPIENTRY glRasterPos4sv(const GLshort * v)
{
    IntGetCurrentDispatchTable()->RasterPos4sv(v);
}

void GLAPIENTRY glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
    IntGetCurrentDispatchTable()->Rectd(x1, y1, x2, y2);
}

void GLAPIENTRY glRectdv(const GLdouble * v1, const GLdouble * v2)
{
    IntGetCurrentDispatchTable()->Rectdv(v1, v2);
}

void GLAPIENTRY glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    IntGetCurrentDispatchTable()->Rectf(x1, y1, x2, y2);
}

void GLAPIENTRY glRectfv(const GLfloat * v1, const GLfloat * v2)
{
    IntGetCurrentDispatchTable()->Rectfv(v1, v2);
}

void GLAPIENTRY glRecti(GLint x1, GLint y1, GLint x2, GLint y2)
{
    IntGetCurrentDispatchTable()->Recti(x1, y1, x2, y2);
}

void GLAPIENTRY glRectiv(const GLint * v1, const GLint * v2)
{
    IntGetCurrentDispatchTable()->Rectiv(v1, v2);
}

void GLAPIENTRY glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
    IntGetCurrentDispatchTable()->Rects(x1, y1, x2, y2);
}

void GLAPIENTRY glRectsv(const GLshort * v1, const GLshort * v2)
{
    IntGetCurrentDispatchTable()->Rectsv(v1, v2);
}

void GLAPIENTRY glTexCoord1d(GLdouble s)
{
    IntGetCurrentDispatchTable()->TexCoord1d(s);
}

void GLAPIENTRY glTexCoord1dv(const GLdouble * v)
{
    IntGetCurrentDispatchTable()->TexCoord1dv(v);
}

void GLAPIENTRY glTexCoord1f(GLfloat s)
{
    IntGetCurrentDispatchTable()->TexCoord1f(s);
}

void GLAPIENTRY glTexCoord1fv(const GLfloat * v)
{
    IntGetCurrentDispatchTable()->TexCoord1fv(v);
}

void GLAPIENTRY glTexCoord1i(GLint s)
{
    IntGetCurrentDispatchTable()->TexCoord1i(s);
}

void GLAPIENTRY glTexCoord1iv(const GLint * v)
{
    IntGetCurrentDispatchTable()->TexCoord1iv(v);
}

void GLAPIENTRY glTexCoord1s(GLshort s)
{
    IntGetCurrentDispatchTable()->TexCoord1s(s);
}

void GLAPIENTRY glTexCoord1sv(const GLshort * v)
{
    IntGetCurrentDispatchTable()->TexCoord1sv(v);
}

void GLAPIENTRY glTexCoord2d(GLdouble s, GLdouble t)
{
    IntGetCurrentDispatchTable()->TexCoord2d(s, t);
}

void GLAPIENTRY glTexCoord2dv(const GLdouble * v)
{
    IntGetCurrentDispatchTable()->TexCoord2dv(v);
}

void GLAPIENTRY glTexCoord2f(GLfloat s, GLfloat t)
{
    IntGetCurrentDispatchTable()->TexCoord2f(s, t);
}

void GLAPIENTRY glTexCoord2fv(const GLfloat * v)
{
    IntGetCurrentDispatchTable()->TexCoord2fv(v);
}

void GLAPIENTRY glTexCoord2i(GLint s, GLint t)
{
    IntGetCurrentDispatchTable()->TexCoord2i(s, t);
}

void GLAPIENTRY glTexCoord2iv(const GLint * v)
{
    IntGetCurrentDispatchTable()->TexCoord2iv(v);
}

void GLAPIENTRY glTexCoord2s(GLshort s, GLshort t)
{
    IntGetCurrentDispatchTable()->TexCoord2s(s, t);
}

void GLAPIENTRY glTexCoord2sv(const GLshort * v)
{
    IntGetCurrentDispatchTable()->TexCoord2sv(v);
}

void GLAPIENTRY glTexCoord3d(GLdouble s, GLdouble t, GLdouble r)
{
    IntGetCurrentDispatchTable()->TexCoord3d(s, t, r);
}

void GLAPIENTRY glTexCoord3dv(const GLdouble * v)
{
    IntGetCurrentDispatchTable()->TexCoord3dv(v);
}

void GLAPIENTRY glTexCoord3f(GLfloat s, GLfloat t, GLfloat r)
{
    IntGetCurrentDispatchTable()->TexCoord3f(s, t, r);
}

void GLAPIENTRY glTexCoord3fv(const GLfloat * v)
{
    IntGetCurrentDispatchTable()->TexCoord3fv(v);
}

void GLAPIENTRY glTexCoord3i(GLint s, GLint t, GLint r)
{
    IntGetCurrentDispatchTable()->TexCoord3i(s, t, r);
}

void GLAPIENTRY glTexCoord3iv(const GLint * v)
{
    IntGetCurrentDispatchTable()->TexCoord3iv(v);
}

void GLAPIENTRY glTexCoord3s(GLshort s, GLshort t, GLshort r)
{
    IntGetCurrentDispatchTable()->TexCoord3s(s, t, r);
}

void GLAPIENTRY glTexCoord3sv(const GLshort * v)
{
    IntGetCurrentDispatchTable()->TexCoord3sv(v);
}

void GLAPIENTRY glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
    IntGetCurrentDispatchTable()->TexCoord4d(s, t, r, q);
}

void GLAPIENTRY glTexCoord4dv(const GLdouble * v)
{
    IntGetCurrentDispatchTable()->TexCoord4dv(v);
}

void GLAPIENTRY glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    IntGetCurrentDispatchTable()->TexCoord4f(s, t, r, q);
}

void GLAPIENTRY glTexCoord4fv(const GLfloat * v)
{
    IntGetCurrentDispatchTable()->TexCoord4fv(v);
}

void GLAPIENTRY glTexCoord4i(GLint s, GLint t, GLint r, GLint q)
{
    IntGetCurrentDispatchTable()->TexCoord4i(s, t, r, q);
}

void GLAPIENTRY glTexCoord4iv(const GLint * v)
{
    IntGetCurrentDispatchTable()->TexCoord4iv(v);
}

void GLAPIENTRY glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q)
{
    IntGetCurrentDispatchTable()->TexCoord4s(s, t, r, q);
}

void GLAPIENTRY glTexCoord4sv(const GLshort * v)
{
    IntGetCurrentDispatchTable()->TexCoord4sv(v);
}

void GLAPIENTRY glVertex2d(GLdouble x, GLdouble y)
{
    IntGetCurrentDispatchTable()->Vertex2d(x, y);
}

void GLAPIENTRY glVertex2dv(const GLdouble * v)
{
    IntGetCurrentDispatchTable()->Vertex2dv(v);
}

void GLAPIENTRY glVertex2f(GLfloat x, GLfloat y)
{
    IntGetCurrentDispatchTable()->Vertex2f(x, y);
}

void GLAPIENTRY glVertex2fv(const GLfloat * v)
{
    IntGetCurrentDispatchTable()->Vertex2fv(v);
}

void GLAPIENTRY glVertex2i(GLint x, GLint y)
{
    IntGetCurrentDispatchTable()->Vertex2i(x, y);
}

void GLAPIENTRY glVertex2iv(const GLint * v)
{
    IntGetCurrentDispatchTable()->Vertex2iv(v);
}

void GLAPIENTRY glVertex2s(GLshort x, GLshort y)
{
    IntGetCurrentDispatchTable()->Vertex2s(x, y);
}

void GLAPIENTRY glVertex2sv(const GLshort * v)
{
    IntGetCurrentDispatchTable()->Vertex2sv(v);
}

void GLAPIENTRY glVertex3d(GLdouble x, GLdouble y, GLdouble z)
{
    IntGetCurrentDispatchTable()->Vertex3d(x, y, z);
}

void GLAPIENTRY glVertex3dv(const GLdouble * v)
{
    IntGetCurrentDispatchTable()->Vertex3dv(v);
}

void GLAPIENTRY glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
    IntGetCurrentDispatchTable()->Vertex3f(x, y, z);
}

void GLAPIENTRY glVertex3fv(const GLfloat * v)
{
    IntGetCurrentDispatchTable()->Vertex3fv(v);
}

void GLAPIENTRY glVertex3i(GLint x, GLint y, GLint z)
{
    IntGetCurrentDispatchTable()->Vertex3i(x, y, z);
}

void GLAPIENTRY glVertex3iv(const GLint * v)
{
    IntGetCurrentDispatchTable()->Vertex3iv(v);
}

void GLAPIENTRY glVertex3s(GLshort x, GLshort y, GLshort z)
{
    IntGetCurrentDispatchTable()->Vertex3s(x, y, z);
}

void GLAPIENTRY glVertex3sv(const GLshort * v)
{
    IntGetCurrentDispatchTable()->Vertex3sv(v);
}

void GLAPIENTRY glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    IntGetCurrentDispatchTable()->Vertex4d(x, y, z, w);
}

void GLAPIENTRY glVertex4dv(const GLdouble * v)
{
    IntGetCurrentDispatchTable()->Vertex4dv(v);
}

void GLAPIENTRY glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    IntGetCurrentDispatchTable()->Vertex4f(x, y, z, w);
}

void GLAPIENTRY glVertex4fv(const GLfloat * v)
{
    IntGetCurrentDispatchTable()->Vertex4fv(v);
}

void GLAPIENTRY glVertex4i(GLint x, GLint y, GLint z, GLint w)
{
    IntGetCurrentDispatchTable()->Vertex4i(x, y, z, w);
}

void GLAPIENTRY glVertex4iv(const GLint * v)
{
    IntGetCurrentDispatchTable()->Vertex4iv(v);
}

void GLAPIENTRY glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    IntGetCurrentDispatchTable()->Vertex4s(x, y, z, w);
}

void GLAPIENTRY glVertex4sv(const GLshort * v)
{
    IntGetCurrentDispatchTable()->Vertex4sv(v);
}

void GLAPIENTRY glClipPlane(GLenum plane, const GLdouble * equation)
{
    IntGetCurrentDispatchTable()->ClipPlane(plane, equation);
}

void GLAPIENTRY glColorMaterial(GLenum face, GLenum mode)
{
    IntGetCurrentDispatchTable()->ColorMaterial(face, mode);
}

void GLAPIENTRY glCullFace(GLenum mode)
{
    IntGetCurrentDispatchTable()->CullFace(mode);
}

void GLAPIENTRY glFogf(GLenum pname, GLfloat param)
{
    IntGetCurrentDispatchTable()->Fogf(pname, param);
}

void GLAPIENTRY glFogfv(GLenum pname, const GLfloat * params)
{
    IntGetCurrentDispatchTable()->Fogfv(pname, params);
}

void GLAPIENTRY glFogi(GLenum pname, GLint param)
{
    IntGetCurrentDispatchTable()->Fogi(pname, param);
}

void GLAPIENTRY glFogiv(GLenum pname, const GLint * params)
{
    IntGetCurrentDispatchTable()->Fogiv(pname, params);
}

void GLAPIENTRY glFrontFace(GLenum mode)
{
    IntGetCurrentDispatchTable()->FrontFace(mode);
}

void GLAPIENTRY glHint(GLenum target, GLenum mode)
{
    IntGetCurrentDispatchTable()->Hint(target, mode);
}

void GLAPIENTRY glLightf(GLenum light, GLenum pname, GLfloat param)
{
    IntGetCurrentDispatchTable()->Lightf(light, pname, param);
}

void GLAPIENTRY glLightfv(GLenum light, GLenum pname, const GLfloat * params)
{
    IntGetCurrentDispatchTable()->Lightfv(light, pname, params);
}

void GLAPIENTRY glLighti(GLenum light, GLenum pname, GLint param)
{
    IntGetCurrentDispatchTable()->Lighti(light, pname, param);
}

void GLAPIENTRY glLightiv(GLenum light, GLenum pname, const GLint * params)
{
    IntGetCurrentDispatchTable()->Lightiv(light, pname, params);
}

void GLAPIENTRY glLightModelf(GLenum pname, GLfloat param)
{
    IntGetCurrentDispatchTable()->LightModelf(pname, param);
}

void GLAPIENTRY glLightModelfv(GLenum pname, const GLfloat * params)
{
    IntGetCurrentDispatchTable()->LightModelfv(pname, params);
}

void GLAPIENTRY glLightModeli(GLenum pname, GLint param)
{
    IntGetCurrentDispatchTable()->LightModeli(pname, param);
}

void GLAPIENTRY glLightModeliv(GLenum pname, const GLint * params)
{
    IntGetCurrentDispatchTable()->LightModeliv(pname, params);
}

void GLAPIENTRY glLineStipple(GLint factor, GLushort pattern)
{
    IntGetCurrentDispatchTable()->LineStipple(factor, pattern);
}

void GLAPIENTRY glLineWidth(GLfloat width)
{
    IntGetCurrentDispatchTable()->LineWidth(width);
}

void GLAPIENTRY glMaterialf(GLenum face, GLenum pname, GLfloat param)
{
    IntGetCurrentDispatchTable()->Materialf(face, pname, param);
}

void GLAPIENTRY glMaterialfv(GLenum face, GLenum pname, const GLfloat * params)
{
    IntGetCurrentDispatchTable()->Materialfv(face, pname, params);
}

void GLAPIENTRY glMateriali(GLenum face, GLenum pname, GLint param)
{
    IntGetCurrentDispatchTable()->Materiali(face, pname, param);
}

void GLAPIENTRY glMaterialiv(GLenum face, GLenum pname, const GLint * params)
{
    IntGetCurrentDispatchTable()->Materialiv(face, pname, params);
}

void GLAPIENTRY glPointSize(GLfloat size)
{
    IntGetCurrentDispatchTable()->PointSize(size);
}

void GLAPIENTRY glPolygonMode(GLenum face, GLenum mode)
{
    IntGetCurrentDispatchTable()->PolygonMode(face, mode);
}

void GLAPIENTRY glPolygonStipple(const GLubyte * mask)
{
    IntGetCurrentDispatchTable()->PolygonStipple(mask);
}

void GLAPIENTRY glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    IntGetCurrentDispatchTable()->Scissor(x, y, width, height);
}

void GLAPIENTRY glShadeModel(GLenum mode)
{
    IntGetCurrentDispatchTable()->ShadeModel(mode);
}

void GLAPIENTRY glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
    IntGetCurrentDispatchTable()->TexParameterf(target, pname, param);
}

void GLAPIENTRY glTexParameterfv(GLenum target, GLenum pname, const GLfloat * params)
{
    IntGetCurrentDispatchTable()->TexParameterfv(target, pname, params);
}

void GLAPIENTRY glTexParameteri(GLenum target, GLenum pname, GLint param)
{
    IntGetCurrentDispatchTable()->TexParameteri(target, pname, param);
}

void GLAPIENTRY glTexParameteriv(GLenum target, GLenum pname, const GLint * params)
{
    IntGetCurrentDispatchTable()->TexParameteriv(target, pname, params);
}

void GLAPIENTRY glTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid * pixels)
{
    IntGetCurrentDispatchTable()->TexImage1D(target, level, internalformat, width, border, format, type, pixels);
}

void GLAPIENTRY glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * pixels)
{
    IntGetCurrentDispatchTable()->TexImage2D(target, level, internalformat, width, height, border, format, type, pixels);
}

void GLAPIENTRY glTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
    IntGetCurrentDispatchTable()->TexEnvf(target, pname, param);
}

void GLAPIENTRY glTexEnvfv(GLenum target, GLenum pname, const GLfloat * params)
{
    IntGetCurrentDispatchTable()->TexEnvfv(target, pname, params);
}

void GLAPIENTRY glTexEnvi(GLenum target, GLenum pname, GLint param)
{
    IntGetCurrentDispatchTable()->TexEnvi(target, pname, param);
}

void GLAPIENTRY glTexEnviv(GLenum target, GLenum pname, const GLint * params)
{
    IntGetCurrentDispatchTable()->TexEnviv(target, pname, params);
}

void GLAPIENTRY glTexGend(GLenum coord, GLenum pname, GLdouble param)
{
    IntGetCurrentDispatchTable()->TexGend(coord, pname, param);
}

void GLAPIENTRY glTexGendv(GLenum coord, GLenum pname, const GLdouble * params)
{
    IntGetCurrentDispatchTable()->TexGendv(coord, pname, params);
}

void GLAPIENTRY glTexGenf(GLenum coord, GLenum pname, GLfloat param)
{
    IntGetCurrentDispatchTable()->TexGenf(coord, pname, param);
}

void GLAPIENTRY glTexGenfv(GLenum coord, GLenum pname, const GLfloat * params)
{
    IntGetCurrentDispatchTable()->TexGenfv(coord, pname, params);
}

void GLAPIENTRY glTexGeni(GLenum coord, GLenum pname, GLint param)
{
    IntGetCurrentDispatchTable()->TexGeni(coord, pname, param);
}

void GLAPIENTRY glTexGeniv(GLenum coord, GLenum pname, const GLint * params)
{
    IntGetCurrentDispatchTable()->TexGeniv(coord, pname, params);
}

void GLAPIENTRY glFeedbackBuffer(GLsizei size, GLenum type, GLfloat * buffer)
{
    IntGetCurrentDispatchTable()->FeedbackBuffer(size, type, buffer);
}

void GLAPIENTRY glSelectBuffer(GLsizei size, GLuint * buffer)
{
    IntGetCurrentDispatchTable()->SelectBuffer(size, buffer);
}

GLint GLAPIENTRY glRenderMode(GLenum mode)
{
    return IntGetCurrentDispatchTable()->RenderMode(mode);
}

void GLAPIENTRY glInitNames(void)
{
    IntGetCurrentDispatchTable()->InitNames();
}

void GLAPIENTRY glLoadName(GLuint name)
{
    IntGetCurrentDispatchTable()->LoadName(name);
}

void GLAPIENTRY glPassThrough(GLfloat token)
{
    IntGetCurrentDispatchTable()->PassThrough(token);
}

void GLAPIENTRY glPopName(void)
{
    IntGetCurrentDispatchTable()->PopName();
}

void GLAPIENTRY glPushName(GLuint name)
{
    IntGetCurrentDispatchTable()->PushName(name);
}

void GLAPIENTRY glDrawBuffer(GLenum mode)
{
    IntGetCurrentDispatchTable()->DrawBuffer(mode);
}

void GLAPIENTRY glClear(GLbitfield mask)
{
    IntGetCurrentDispatchTable()->Clear(mask);
}

void GLAPIENTRY glClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    IntGetCurrentDispatchTable()->ClearAccum(red, green, blue, alpha);
}

void GLAPIENTRY glClearIndex(GLfloat c)
{
    IntGetCurrentDispatchTable()->ClearIndex(c);
}

void GLAPIENTRY glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    IntGetCurrentDispatchTable()->ClearColor(red, green, blue, alpha);
}

void GLAPIENTRY glClearStencil(GLint s)
{
    IntGetCurrentDispatchTable()->ClearStencil(s);
}

void GLAPIENTRY glClearDepth(GLclampd depth)
{
    IntGetCurrentDispatchTable()->ClearDepth(depth);
}

void GLAPIENTRY glStencilMask(GLuint mask)
{
    IntGetCurrentDispatchTable()->StencilMask(mask);
}

void GLAPIENTRY glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    IntGetCurrentDispatchTable()->ColorMask(red, green, blue, alpha);
}

void GLAPIENTRY glDepthMask(GLboolean flag)
{
    IntGetCurrentDispatchTable()->DepthMask(flag);
}

void GLAPIENTRY glIndexMask(GLuint mask)
{
    IntGetCurrentDispatchTable()->IndexMask(mask);
}

void GLAPIENTRY glAccum(GLenum op, GLfloat value)
{
    IntGetCurrentDispatchTable()->Accum(op, value);
}

void GLAPIENTRY glDisable(GLenum cap)
{
    IntGetCurrentDispatchTable()->Disable(cap);
}

void GLAPIENTRY glEnable(GLenum cap)
{
    IntGetCurrentDispatchTable()->Enable(cap);
}

void GLAPIENTRY glFinish(void)
{
    IntGetCurrentDispatchTable()->Finish();
}

void GLAPIENTRY glFlush(void)
{
    IntGetCurrentDispatchTable()->Flush();
}

void GLAPIENTRY glPopAttrib(void)
{
    IntGetCurrentDispatchTable()->PopAttrib();
}

void GLAPIENTRY glPushAttrib(GLbitfield mask)
{
    IntGetCurrentDispatchTable()->PushAttrib(mask);
}

void GLAPIENTRY glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble * points)
{
    IntGetCurrentDispatchTable()->Map1d(target, u1, u2, stride, order, points);
}

void GLAPIENTRY glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat * points)
{
    IntGetCurrentDispatchTable()->Map1f(target, u1, u2, stride, order, points);
}

void GLAPIENTRY glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble * points)
{
    IntGetCurrentDispatchTable()->Map2d(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

void GLAPIENTRY glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat * points)
{
    IntGetCurrentDispatchTable()->Map2f(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder, points);
}

void GLAPIENTRY glMapGrid1d(GLint un, GLdouble u1, GLdouble u2)
{
    IntGetCurrentDispatchTable()->MapGrid1d(un, u1, u2);
}

void GLAPIENTRY glMapGrid1f(GLint un, GLfloat u1, GLfloat u2)
{
    IntGetCurrentDispatchTable()->MapGrid1f(un, u1, u2);
}

void GLAPIENTRY glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
    IntGetCurrentDispatchTable()->MapGrid2d(un, u1, u2, vn, v1, v2);
}

void GLAPIENTRY glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
    IntGetCurrentDispatchTable()->MapGrid2f(un, u1, u2, vn, v1, v2);
}

void GLAPIENTRY glEvalCoord1d(GLdouble u)
{
    IntGetCurrentDispatchTable()->EvalCoord1d(u);
}

void GLAPIENTRY glEvalCoord1dv(const GLdouble * u)
{
    IntGetCurrentDispatchTable()->EvalCoord1dv(u);
}

void GLAPIENTRY glEvalCoord1f(GLfloat u)
{
    IntGetCurrentDispatchTable()->EvalCoord1f(u);
}

void GLAPIENTRY glEvalCoord1fv(const GLfloat * u)
{
    IntGetCurrentDispatchTable()->EvalCoord1fv(u);
}

void GLAPIENTRY glEvalCoord2d(GLdouble u, GLdouble v)
{
    IntGetCurrentDispatchTable()->EvalCoord2d(u, v);
}

void GLAPIENTRY glEvalCoord2dv(const GLdouble * u)
{
    IntGetCurrentDispatchTable()->EvalCoord2dv(u);
}

void GLAPIENTRY glEvalCoord2f(GLfloat u, GLfloat v)
{
    IntGetCurrentDispatchTable()->EvalCoord2f(u, v);
}

void GLAPIENTRY glEvalCoord2fv(const GLfloat * u)
{
    IntGetCurrentDispatchTable()->EvalCoord2fv(u);
}

void GLAPIENTRY glEvalMesh1(GLenum mode, GLint i1, GLint i2)
{
    IntGetCurrentDispatchTable()->EvalMesh1(mode, i1, i2);
}

void GLAPIENTRY glEvalPoint1(GLint i)
{
    IntGetCurrentDispatchTable()->EvalPoint1(i);
}

void GLAPIENTRY glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
    IntGetCurrentDispatchTable()->EvalMesh2(mode, i1, i2, j1, j2);
}

void GLAPIENTRY glEvalPoint2(GLint i, GLint j)
{
    IntGetCurrentDispatchTable()->EvalPoint2(i, j);
}

void GLAPIENTRY glAlphaFunc(GLenum func, GLclampf ref)
{
    IntGetCurrentDispatchTable()->AlphaFunc(func, ref);
}

void GLAPIENTRY glBlendFunc(GLenum sfactor, GLenum dfactor)
{
    IntGetCurrentDispatchTable()->BlendFunc(sfactor, dfactor);
}

void GLAPIENTRY glLogicOp(GLenum opcode)
{
    IntGetCurrentDispatchTable()->LogicOp(opcode);
}

void GLAPIENTRY glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
    IntGetCurrentDispatchTable()->StencilFunc(func, ref, mask);
}

void GLAPIENTRY glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
    IntGetCurrentDispatchTable()->StencilOp(fail, zfail, zpass);
}

void GLAPIENTRY glDepthFunc(GLenum func)
{
    IntGetCurrentDispatchTable()->DepthFunc(func);
}

void GLAPIENTRY glPixelZoom(GLfloat xfactor, GLfloat yfactor)
{
    IntGetCurrentDispatchTable()->PixelZoom(xfactor, yfactor);
}

void GLAPIENTRY glPixelTransferf(GLenum pname, GLfloat param)
{
    IntGetCurrentDispatchTable()->PixelTransferf(pname, param);
}

void GLAPIENTRY glPixelTransferi(GLenum pname, GLint param)
{
    IntGetCurrentDispatchTable()->PixelTransferi(pname, param);
}

void GLAPIENTRY glPixelStoref(GLenum pname, GLfloat param)
{
    IntGetCurrentDispatchTable()->PixelStoref(pname, param);
}

void GLAPIENTRY glPixelStorei(GLenum pname, GLint param)
{
    IntGetCurrentDispatchTable()->PixelStorei(pname, param);
}

void GLAPIENTRY glPixelMapfv(GLenum map, GLsizei mapsize, const GLfloat * values)
{
    IntGetCurrentDispatchTable()->PixelMapfv(map, mapsize, values);
}

void GLAPIENTRY glPixelMapuiv(GLenum map, GLsizei mapsize, const GLuint * values)
{
    IntGetCurrentDispatchTable()->PixelMapuiv(map, mapsize, values);
}

void GLAPIENTRY glPixelMapusv(GLenum map, GLsizei mapsize, const GLushort * values)
{
    IntGetCurrentDispatchTable()->PixelMapusv(map, mapsize, values);
}

void GLAPIENTRY glReadBuffer(GLenum mode)
{
    IntGetCurrentDispatchTable()->ReadBuffer(mode);
}

void GLAPIENTRY glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
    IntGetCurrentDispatchTable()->CopyPixels(x, y, width, height, type);
}

void GLAPIENTRY glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * pixels)
{
    IntGetCurrentDispatchTable()->ReadPixels(x, y, width, height, format, type, pixels);
}

void GLAPIENTRY glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels)
{
    IntGetCurrentDispatchTable()->DrawPixels(width, height, format, type, pixels);
}

void GLAPIENTRY glGetBooleanv(GLenum pname, GLboolean * params)
{
    IntGetCurrentDispatchTable()->GetBooleanv(pname, params);
}

void GLAPIENTRY glGetClipPlane(GLenum plane, GLdouble * equation)
{
    IntGetCurrentDispatchTable()->GetClipPlane(plane, equation);
}

void GLAPIENTRY glGetDoublev(GLenum pname, GLdouble * params)
{
    IntGetCurrentDispatchTable()->GetDoublev(pname, params);
}

GLenum GLAPIENTRY glGetError(void)
{
    return IntGetCurrentDispatchTable()->GetError();
}

void GLAPIENTRY glGetFloatv(GLenum pname, GLfloat * params)
{
    IntGetCurrentDispatchTable()->GetFloatv(pname, params);
}

void GLAPIENTRY glGetIntegerv(GLenum pname, GLint * params)
{
    IntGetCurrentDispatchTable()->GetIntegerv(pname, params);
}

void GLAPIENTRY glGetLightfv(GLenum light, GLenum pname, GLfloat * params)
{
    IntGetCurrentDispatchTable()->GetLightfv(light, pname, params);
}

void GLAPIENTRY glGetLightiv(GLenum light, GLenum pname, GLint * params)
{
    IntGetCurrentDispatchTable()->GetLightiv(light, pname, params);
}

void GLAPIENTRY glGetMapdv(GLenum target, GLenum query, GLdouble * v)
{
    IntGetCurrentDispatchTable()->GetMapdv(target, query, v);
}

void GLAPIENTRY glGetMapfv(GLenum target, GLenum query, GLfloat * v)
{
    IntGetCurrentDispatchTable()->GetMapfv(target, query, v);
}

void GLAPIENTRY glGetMapiv(GLenum target, GLenum query, GLint * v)
{
    IntGetCurrentDispatchTable()->GetMapiv(target, query, v);
}

void GLAPIENTRY glGetMaterialfv(GLenum face, GLenum pname, GLfloat * params)
{
    IntGetCurrentDispatchTable()->GetMaterialfv(face, pname, params);
}

void GLAPIENTRY glGetMaterialiv(GLenum face, GLenum pname, GLint * params)
{
    IntGetCurrentDispatchTable()->GetMaterialiv(face, pname, params);
}

void GLAPIENTRY glGetPixelMapfv(GLenum map, GLfloat * values)
{
    IntGetCurrentDispatchTable()->GetPixelMapfv(map, values);
}

void GLAPIENTRY glGetPixelMapuiv(GLenum map, GLuint * values)
{
    IntGetCurrentDispatchTable()->GetPixelMapuiv(map, values);
}

void GLAPIENTRY glGetPixelMapusv(GLenum map, GLushort * values)
{
    IntGetCurrentDispatchTable()->GetPixelMapusv(map, values);
}

void GLAPIENTRY glGetPolygonStipple(GLubyte * mask)
{
    IntGetCurrentDispatchTable()->GetPolygonStipple(mask);
}

const GLubyte * GLAPIENTRY glGetString(GLenum name)
{
    return IntGetCurrentDispatchTable()->GetString(name);
}

void GLAPIENTRY glGetTexEnvfv(GLenum target, GLenum pname, GLfloat * params)
{
    IntGetCurrentDispatchTable()->GetTexEnvfv(target, pname, params);
}

void GLAPIENTRY glGetTexEnviv(GLenum target, GLenum pname, GLint * params)
{
    IntGetCurrentDispatchTable()->GetTexEnviv(target, pname, params);
}

void GLAPIENTRY glGetTexGendv(GLenum coord, GLenum pname, GLdouble * params)
{
    IntGetCurrentDispatchTable()->GetTexGendv(coord, pname, params);
}

void GLAPIENTRY glGetTexGenfv(GLenum coord, GLenum pname, GLfloat * params)
{
    IntGetCurrentDispatchTable()->GetTexGenfv(coord, pname, params);
}

void GLAPIENTRY glGetTexGeniv(GLenum coord, GLenum pname, GLint * params)
{
    IntGetCurrentDispatchTable()->GetTexGeniv(coord, pname, params);
}

void GLAPIENTRY glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid * pixels)
{
    IntGetCurrentDispatchTable()->GetTexImage(target, level, format, type, pixels);
}

void GLAPIENTRY glGetTexParameterfv(GLenum target, GLenum pname, GLfloat * params)
{
    IntGetCurrentDispatchTable()->GetTexParameterfv(target, pname, params);
}

void GLAPIENTRY glGetTexParameteriv(GLenum target, GLenum pname, GLint * params)
{
    IntGetCurrentDispatchTable()->GetTexParameteriv(target, pname, params);
}

void GLAPIENTRY glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat * params)
{
    IntGetCurrentDispatchTable()->GetTexLevelParameterfv(target, level, pname, params);
}

void GLAPIENTRY glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint * params)
{
    IntGetCurrentDispatchTable()->GetTexLevelParameteriv(target, level, pname, params);
}

GLboolean GLAPIENTRY glIsEnabled(GLenum cap)
{
    return IntGetCurrentDispatchTable()->IsEnabled(cap);
}

GLboolean GLAPIENTRY glIsList(GLuint list)
{
    return IntGetCurrentDispatchTable()->IsList(list);
}

void GLAPIENTRY glDepthRange(GLclampd zNear, GLclampd zFar)
{
    IntGetCurrentDispatchTable()->DepthRange(zNear, zFar);
}

void GLAPIENTRY glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    IntGetCurrentDispatchTable()->Frustum(left, right, bottom, top, zNear, zFar);
}

void GLAPIENTRY glLoadIdentity(void)
{
    IntGetCurrentDispatchTable()->LoadIdentity();
}

void GLAPIENTRY glLoadMatrixf(const GLfloat * m)
{
    IntGetCurrentDispatchTable()->LoadMatrixf(m);
}

void GLAPIENTRY glLoadMatrixd(const GLdouble * m)
{
    IntGetCurrentDispatchTable()->LoadMatrixd(m);
}

void GLAPIENTRY glMatrixMode(GLenum mode)
{
    IntGetCurrentDispatchTable()->MatrixMode(mode);
}

void GLAPIENTRY glMultMatrixf(const GLfloat * m)
{
    IntGetCurrentDispatchTable()->MultMatrixf(m);
}

void GLAPIENTRY glMultMatrixd(const GLdouble * m)
{
    IntGetCurrentDispatchTable()->MultMatrixd(m);
}

void GLAPIENTRY glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    IntGetCurrentDispatchTable()->Ortho(left, right, bottom, top, zNear, zFar);
}

void GLAPIENTRY glPopMatrix(void)
{
    IntGetCurrentDispatchTable()->PopMatrix();
}

void GLAPIENTRY glPushMatrix(void)
{
    IntGetCurrentDispatchTable()->PushMatrix();
}

void GLAPIENTRY glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
    IntGetCurrentDispatchTable()->Rotated(angle, x, y, z);
}

void GLAPIENTRY glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    IntGetCurrentDispatchTable()->Rotatef(angle, x, y, z);
}

void GLAPIENTRY glScaled(GLdouble x, GLdouble y, GLdouble z)
{
    IntGetCurrentDispatchTable()->Scaled(x, y, z);
}

void GLAPIENTRY glScalef(GLfloat x, GLfloat y, GLfloat z)
{
    IntGetCurrentDispatchTable()->Scalef(x, y, z);
}

void GLAPIENTRY glTranslated(GLdouble x, GLdouble y, GLdouble z)
{
    IntGetCurrentDispatchTable()->Translated(x, y, z);
}

void GLAPIENTRY glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
    IntGetCurrentDispatchTable()->Translatef(x, y, z);
}

void GLAPIENTRY glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    IntGetCurrentDispatchTable()->Viewport(x, y, width, height);
}

void GLAPIENTRY glArrayElement(GLint i)
{
    IntGetCurrentDispatchTable()->ArrayElement(i);
}

void GLAPIENTRY glBindTexture(GLenum target, GLuint texture)
{
    IntGetCurrentDispatchTable()->BindTexture(target, texture);
}

void GLAPIENTRY glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
    IntGetCurrentDispatchTable()->ColorPointer(size, type, stride, pointer);
}

void GLAPIENTRY glDisableClientState(GLenum array)
{
    IntGetCurrentDispatchTable()->DisableClientState(array);
}

void GLAPIENTRY glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    IntGetCurrentDispatchTable()->DrawArrays(mode, first, count);
}

void GLAPIENTRY glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid * indices)
{
    IntGetCurrentDispatchTable()->DrawElements(mode, count, type, indices);
}

void GLAPIENTRY glEdgeFlagPointer(GLsizei stride, const GLvoid * pointer)
{
    IntGetCurrentDispatchTable()->EdgeFlagPointer(stride, pointer);
}

void GLAPIENTRY glEnableClientState(GLenum array)
{
    IntGetCurrentDispatchTable()->EnableClientState(array);
}

void GLAPIENTRY glIndexPointer(GLenum type, GLsizei stride, const GLvoid * pointer)
{
    IntGetCurrentDispatchTable()->IndexPointer(type, stride, pointer);
}

void GLAPIENTRY glIndexub(GLubyte c)
{
    IntGetCurrentDispatchTable()->Indexub(c);
}

void GLAPIENTRY glIndexubv(const GLubyte * c)
{
    IntGetCurrentDispatchTable()->Indexubv(c);
}

void GLAPIENTRY glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid * pointer)
{
    IntGetCurrentDispatchTable()->InterleavedArrays(format, stride, pointer);
}

void GLAPIENTRY glNormalPointer(GLenum type, GLsizei stride, const GLvoid * pointer)
{
    IntGetCurrentDispatchTable()->NormalPointer(type, stride, pointer);
}

void GLAPIENTRY glPolygonOffset(GLfloat factor, GLfloat units)
{
    IntGetCurrentDispatchTable()->PolygonOffset(factor, units);
}

void GLAPIENTRY glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
    IntGetCurrentDispatchTable()->TexCoordPointer(size, type, stride, pointer);
}

void GLAPIENTRY glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid * pointer)
{
    IntGetCurrentDispatchTable()->VertexPointer(size, type, stride, pointer);
}

GLboolean GLAPIENTRY glAreTexturesResident(GLsizei n, const GLuint * textures, GLboolean * residences)
{
    return IntGetCurrentDispatchTable()->AreTexturesResident(n, textures, residences);
}

void GLAPIENTRY glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
    IntGetCurrentDispatchTable()->CopyTexImage1D(target, level, internalformat, x, y, width, border);
}

void GLAPIENTRY glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    IntGetCurrentDispatchTable()->CopyTexImage2D(target, level, internalformat, x, y, width, height, border);
}

void GLAPIENTRY glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
    IntGetCurrentDispatchTable()->CopyTexSubImage1D(target, level, xoffset, x, y, width);
}

void GLAPIENTRY glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    IntGetCurrentDispatchTable()->CopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height);
}

void GLAPIENTRY glDeleteTextures(GLsizei n, const GLuint * textures)
{
    IntGetCurrentDispatchTable()->DeleteTextures(n, textures);
}

void GLAPIENTRY glGenTextures(GLsizei n, GLuint * textures)
{
    IntGetCurrentDispatchTable()->GenTextures(n, textures);
}

void GLAPIENTRY glGetPointerv(GLenum pname, GLvoid ** params)
{
    IntGetCurrentDispatchTable()->GetPointerv(pname, params);
}

GLboolean GLAPIENTRY glIsTexture(GLuint texture)
{
    return IntGetCurrentDispatchTable()->IsTexture(texture);
}

void GLAPIENTRY glPrioritizeTextures(GLsizei n, const GLuint * textures, const GLclampf * priorities)
{
    IntGetCurrentDispatchTable()->PrioritizeTextures(n, textures, priorities);
}

void GLAPIENTRY glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid * pixels)
{
    IntGetCurrentDispatchTable()->TexSubImage1D(target, level, xoffset, width, format, type, pixels);
}

void GLAPIENTRY glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * pixels)
{
    IntGetCurrentDispatchTable()->TexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void GLAPIENTRY glPopClientAttrib(void)
{
    IntGetCurrentDispatchTable()->PopClientAttrib();
}

void GLAPIENTRY glPushClientAttrib(GLbitfield mask)
{
    IntGetCurrentDispatchTable()->PushClientAttrib(mask);
}
#endif //__i386__

/* Unknown debug function */
GLint GLAPIENTRY glDebugEntry(GLint unknown1, GLint unknown2)
{
    return 0;
}
