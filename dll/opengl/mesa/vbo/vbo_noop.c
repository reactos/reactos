/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
 * Copyright (C) 2011  VMware, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * GLvertexformat no-op functions.  Used in out-of-memory situations.
 */

#include <precomp.h>

#if FEATURE_beginend


static void GLAPIENTRY
_mesa_noop_EdgeFlag(GLboolean b)
{
}

static void GLAPIENTRY
_mesa_noop_Indexf(GLfloat f)
{
}

static void GLAPIENTRY
_mesa_noop_Indexfv(const GLfloat * v)
{
}

static void GLAPIENTRY
_mesa_noop_FogCoordfEXT(GLfloat a)
{
}

static void GLAPIENTRY
_mesa_noop_FogCoordfvEXT(const GLfloat * v)
{
}

static void GLAPIENTRY
_mesa_noop_Normal3f(GLfloat a, GLfloat b, GLfloat c)
{
}

static void GLAPIENTRY
_mesa_noop_Normal3fv(const GLfloat * v)
{
}

static void GLAPIENTRY
_mesa_noop_Color4f(GLfloat a, GLfloat b, GLfloat c, GLfloat d)
{
}

static void GLAPIENTRY
_mesa_noop_Color4fv(const GLfloat * v)
{
}

static void GLAPIENTRY
_mesa_noop_Color3f(GLfloat a, GLfloat b, GLfloat c)
{
}

static void GLAPIENTRY
_mesa_noop_Color3fv(const GLfloat * v)
{
}

static void GLAPIENTRY
_mesa_noop_TexCoord1f(GLfloat a)
{
}

static void GLAPIENTRY
_mesa_noop_TexCoord1fv(const GLfloat * v)
{
}

static void GLAPIENTRY
_mesa_noop_TexCoord2f(GLfloat a, GLfloat b)
{
}

static void GLAPIENTRY
_mesa_noop_TexCoord2fv(const GLfloat * v)
{
}

static void GLAPIENTRY
_mesa_noop_TexCoord3f(GLfloat a, GLfloat b, GLfloat c)
{
}

static void GLAPIENTRY
_mesa_noop_TexCoord3fv(const GLfloat * v)
{
}

static void GLAPIENTRY
_mesa_noop_TexCoord4f(GLfloat a, GLfloat b, GLfloat c, GLfloat d)
{
}

static void GLAPIENTRY
_mesa_noop_TexCoord4fv(const GLfloat * v)
{
}

static void GLAPIENTRY
_mesa_noop_VertexAttrib1fNV(GLuint index, GLfloat x)
{
}

static void GLAPIENTRY
_mesa_noop_VertexAttrib1fvNV(GLuint index, const GLfloat * v)
{
}

static void GLAPIENTRY
_mesa_noop_VertexAttrib2fNV(GLuint index, GLfloat x, GLfloat y)
{
}

static void GLAPIENTRY
_mesa_noop_VertexAttrib2fvNV(GLuint index, const GLfloat * v)
{
}

static void GLAPIENTRY
_mesa_noop_VertexAttrib3fNV(GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
}

static void GLAPIENTRY
_mesa_noop_VertexAttrib3fvNV(GLuint index, const GLfloat * v)
{
}

static void GLAPIENTRY
_mesa_noop_VertexAttrib4fNV(GLuint index, GLfloat x,
                            GLfloat y, GLfloat z, GLfloat w)
{
}

static void GLAPIENTRY
_mesa_noop_VertexAttrib4fvNV(GLuint index, const GLfloat * v)
{
}

static void GLAPIENTRY
_mesa_noop_Materialfv(GLenum face, GLenum pname, const GLfloat * params)
{
}

static void GLAPIENTRY
_mesa_noop_Vertex2fv(const GLfloat * v)
{
}

static void GLAPIENTRY
_mesa_noop_Vertex3fv(const GLfloat * v)
{
}

static void GLAPIENTRY
_mesa_noop_Vertex4fv(const GLfloat * v)
{
}

static void GLAPIENTRY
_mesa_noop_Vertex2f(GLfloat a, GLfloat b)
{
}

static void GLAPIENTRY
_mesa_noop_Vertex3f(GLfloat a, GLfloat b, GLfloat c)
{
}

static void GLAPIENTRY
_mesa_noop_Vertex4f(GLfloat a, GLfloat b, GLfloat c, GLfloat d)
{
}


#if FEATURE_evaluators
static void GLAPIENTRY
_mesa_noop_EvalCoord1f(GLfloat a)
{
}

static void GLAPIENTRY
_mesa_noop_EvalCoord1fv(const GLfloat * v)
{
}

static void GLAPIENTRY
_mesa_noop_EvalCoord2f(GLfloat a, GLfloat b)
{
}

static void GLAPIENTRY
_mesa_noop_EvalCoord2fv(const GLfloat * v)
{
}

static void GLAPIENTRY
_mesa_noop_EvalPoint1(GLint a)
{
}

static void GLAPIENTRY
_mesa_noop_EvalPoint2(GLint a, GLint b)
{
}
#endif /* FEATURE_evaluators */


static void GLAPIENTRY
_mesa_noop_Begin(GLenum mode)
{
}

static void GLAPIENTRY
_mesa_noop_End(void)
{
}


static void GLAPIENTRY
_mesa_noop_Rectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
}


static void GLAPIENTRY
_mesa_noop_DrawArrays(GLenum mode, GLint start, GLsizei count)
{
}

static void GLAPIENTRY
_mesa_noop_DrawElements(GLenum mode, GLsizei count, GLenum type,
                        const GLvoid * indices)
{
}

static void GLAPIENTRY
_mesa_noop_EvalMesh1(GLenum mode, GLint i1, GLint i2)
{
}

static void GLAPIENTRY
_mesa_noop_EvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
}


/**
 * Build a vertexformat of functions that are no-ops.
 * These are used in out-of-memory situations when we have no VBO
 * to put the vertex data into.
 */
void
_mesa_noop_vtxfmt_init(GLvertexformat * vfmt)
{
   _MESA_INIT_ARRAYELT_VTXFMT(vfmt, _ae_);

   vfmt->Begin = _mesa_noop_Begin;

   _MESA_INIT_DLIST_VTXFMT(vfmt, _mesa_);

   vfmt->Color3f = _mesa_noop_Color3f;
   vfmt->Color3fv = _mesa_noop_Color3fv;
   vfmt->Color4f = _mesa_noop_Color4f;
   vfmt->Color4fv = _mesa_noop_Color4fv;
   vfmt->EdgeFlag = _mesa_noop_EdgeFlag;
   vfmt->End = _mesa_noop_End;

   _MESA_INIT_EVAL_VTXFMT(vfmt, _mesa_noop_);

   vfmt->FogCoordfEXT = _mesa_noop_FogCoordfEXT;
   vfmt->FogCoordfvEXT = _mesa_noop_FogCoordfvEXT;
   vfmt->Indexf = _mesa_noop_Indexf;
   vfmt->Indexfv = _mesa_noop_Indexfv;
   vfmt->Materialfv = _mesa_noop_Materialfv;
   vfmt->Normal3f = _mesa_noop_Normal3f;
   vfmt->Normal3fv = _mesa_noop_Normal3fv;
   vfmt->TexCoord1f = _mesa_noop_TexCoord1f;
   vfmt->TexCoord1fv = _mesa_noop_TexCoord1fv;
   vfmt->TexCoord2f = _mesa_noop_TexCoord2f;
   vfmt->TexCoord2fv = _mesa_noop_TexCoord2fv;
   vfmt->TexCoord3f = _mesa_noop_TexCoord3f;
   vfmt->TexCoord3fv = _mesa_noop_TexCoord3fv;
   vfmt->TexCoord4f = _mesa_noop_TexCoord4f;
   vfmt->TexCoord4fv = _mesa_noop_TexCoord4fv;
   vfmt->Vertex2f = _mesa_noop_Vertex2f;
   vfmt->Vertex2fv = _mesa_noop_Vertex2fv;
   vfmt->Vertex3f = _mesa_noop_Vertex3f;
   vfmt->Vertex3fv = _mesa_noop_Vertex3fv;
   vfmt->Vertex4f = _mesa_noop_Vertex4f;
   vfmt->Vertex4fv = _mesa_noop_Vertex4fv;
   vfmt->VertexAttrib1fNV = _mesa_noop_VertexAttrib1fNV;
   vfmt->VertexAttrib1fvNV = _mesa_noop_VertexAttrib1fvNV;
   vfmt->VertexAttrib2fNV = _mesa_noop_VertexAttrib2fNV;
   vfmt->VertexAttrib2fvNV = _mesa_noop_VertexAttrib2fvNV;
   vfmt->VertexAttrib3fNV = _mesa_noop_VertexAttrib3fNV;
   vfmt->VertexAttrib3fvNV = _mesa_noop_VertexAttrib3fvNV;
   vfmt->VertexAttrib4fNV = _mesa_noop_VertexAttrib4fNV;
   vfmt->VertexAttrib4fvNV = _mesa_noop_VertexAttrib4fvNV;

   vfmt->Rectf = _mesa_noop_Rectf;

   vfmt->DrawArrays = _mesa_noop_DrawArrays;
   vfmt->DrawElements = _mesa_noop_DrawElements;
}


/**
 * Is the given dispatch table using the no-op functions?
 */
GLboolean
_mesa_using_noop_vtxfmt(const struct _glapi_table *dispatch)
{
   return GET_Begin((struct _glapi_table *) dispatch) == _mesa_noop_Begin;
}


#endif /* FEATURE_beginend */
