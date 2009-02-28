/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Gareth Hughes
 */

#ifndef PRE_LOOPBACK
#define PRE_LOOPBACK( FUNC )
#endif

#include "glapi/dispatch.h"
#include "glapi/glapioffsets.h"

static void GLAPIENTRY TAG(ArrayElement)( GLint i )
{
   PRE_LOOPBACK( ArrayElement );
   CALL_ArrayElement(GET_DISPATCH(), ( i ));
}

static void GLAPIENTRY TAG(Color3f)( GLfloat r, GLfloat g, GLfloat b )
{
   PRE_LOOPBACK( Color3f );
   CALL_Color3f(GET_DISPATCH(), ( r, g, b ));
}

static void GLAPIENTRY TAG(Color3fv)( const GLfloat *v )
{
   PRE_LOOPBACK( Color3fv );
   CALL_Color3fv(GET_DISPATCH(), ( v ));
}

static void GLAPIENTRY TAG(Color4f)( GLfloat r, GLfloat g, GLfloat b, GLfloat a )
{
   PRE_LOOPBACK( Color4f );
   CALL_Color4f(GET_DISPATCH(), ( r, g, b, a ));
}

static void GLAPIENTRY TAG(Color4fv)( const GLfloat *v )
{
   PRE_LOOPBACK( Color4fv );
   CALL_Color4fv(GET_DISPATCH(), ( v ));
}

static void GLAPIENTRY TAG(EdgeFlag)( GLboolean e )
{
   PRE_LOOPBACK( EdgeFlag );
   CALL_EdgeFlag(GET_DISPATCH(), ( e ));
}

static void GLAPIENTRY TAG(EvalCoord1f)( GLfloat s )
{
   PRE_LOOPBACK( EvalCoord1f );
   CALL_EvalCoord1f(GET_DISPATCH(), ( s ));
}

static void GLAPIENTRY TAG(EvalCoord1fv)( const GLfloat *v )
{
   PRE_LOOPBACK( EvalCoord1fv );
   CALL_EvalCoord1fv(GET_DISPATCH(), ( v ));
}

static void GLAPIENTRY TAG(EvalCoord2f)( GLfloat s, GLfloat t )
{
   PRE_LOOPBACK( EvalCoord2f );
   CALL_EvalCoord2f(GET_DISPATCH(), ( s, t ));
}

static void GLAPIENTRY TAG(EvalCoord2fv)( const GLfloat *v )
{
   PRE_LOOPBACK( EvalCoord2fv );
   CALL_EvalCoord2fv(GET_DISPATCH(), ( v ));
}

static void GLAPIENTRY TAG(EvalPoint1)( GLint i )
{
   PRE_LOOPBACK( EvalPoint1 );
   CALL_EvalPoint1(GET_DISPATCH(), ( i ));
}

static void GLAPIENTRY TAG(EvalPoint2)( GLint i, GLint j )
{
   PRE_LOOPBACK( EvalPoint2 );
   CALL_EvalPoint2(GET_DISPATCH(), ( i, j ));
}

static void GLAPIENTRY TAG(FogCoordfEXT)( GLfloat f )
{
   PRE_LOOPBACK( FogCoordfEXT );
   CALL_FogCoordfEXT(GET_DISPATCH(), ( f ));
}

static void GLAPIENTRY TAG(FogCoordfvEXT)( const GLfloat *v )
{
   PRE_LOOPBACK( FogCoordfvEXT );
   CALL_FogCoordfvEXT(GET_DISPATCH(), ( v ));
}

static void GLAPIENTRY TAG(Indexf)( GLfloat f )
{
   PRE_LOOPBACK( Indexf );
   CALL_Indexf(GET_DISPATCH(), ( f ));
}

static void GLAPIENTRY TAG(Indexfv)( const GLfloat *v )
{
   PRE_LOOPBACK( Indexfv );
   CALL_Indexfv(GET_DISPATCH(), ( v ));
}

static void GLAPIENTRY TAG(Materialfv)( GLenum face, GLenum pname, const GLfloat *v )
{
   PRE_LOOPBACK( Materialfv );
   CALL_Materialfv(GET_DISPATCH(), ( face, pname, v ));
}

static void GLAPIENTRY TAG(MultiTexCoord1fARB)( GLenum target, GLfloat a )
{
   PRE_LOOPBACK( MultiTexCoord1fARB );
   CALL_MultiTexCoord1fARB(GET_DISPATCH(), ( target, a ));
}

static void GLAPIENTRY TAG(MultiTexCoord1fvARB)( GLenum target, const GLfloat *tc )
{
   PRE_LOOPBACK( MultiTexCoord1fvARB );
   CALL_MultiTexCoord1fvARB(GET_DISPATCH(), ( target, tc ));
}

static void GLAPIENTRY TAG(MultiTexCoord2fARB)( GLenum target, GLfloat s, GLfloat t )
{
   PRE_LOOPBACK( MultiTexCoord2fARB );
   CALL_MultiTexCoord2fARB(GET_DISPATCH(), ( target, s, t ));
}

static void GLAPIENTRY TAG(MultiTexCoord2fvARB)( GLenum target, const GLfloat *tc )
{
   PRE_LOOPBACK( MultiTexCoord2fvARB );
   CALL_MultiTexCoord2fvARB(GET_DISPATCH(), ( target, tc ));
}

static void GLAPIENTRY TAG(MultiTexCoord3fARB)( GLenum target, GLfloat s,
				     GLfloat t, GLfloat r )
{
   PRE_LOOPBACK( MultiTexCoord3fARB );
   CALL_MultiTexCoord3fARB(GET_DISPATCH(), ( target, s, t, r ));
}

static void GLAPIENTRY TAG(MultiTexCoord3fvARB)( GLenum target, const GLfloat *tc )
{
   PRE_LOOPBACK( MultiTexCoord3fvARB );
   CALL_MultiTexCoord3fvARB(GET_DISPATCH(), ( target, tc ));
}

static void GLAPIENTRY TAG(MultiTexCoord4fARB)( GLenum target, GLfloat s,
				     GLfloat t, GLfloat r, GLfloat q )
{
   PRE_LOOPBACK( MultiTexCoord4fARB );
   CALL_MultiTexCoord4fARB(GET_DISPATCH(), ( target, s, t, r, q ));
}

static void GLAPIENTRY TAG(MultiTexCoord4fvARB)( GLenum target, const GLfloat *tc )
{
   PRE_LOOPBACK( MultiTexCoord4fvARB );
   CALL_MultiTexCoord4fvARB(GET_DISPATCH(), ( target, tc ));
}

static void GLAPIENTRY TAG(Normal3f)( GLfloat x, GLfloat y, GLfloat z )
{
   PRE_LOOPBACK( Normal3f );
   CALL_Normal3f(GET_DISPATCH(), ( x, y, z ));
}

static void GLAPIENTRY TAG(Normal3fv)( const GLfloat *v )
{
   PRE_LOOPBACK( Normal3fv );
   CALL_Normal3fv(GET_DISPATCH(), ( v ));
}

static void GLAPIENTRY TAG(SecondaryColor3fEXT)( GLfloat r, GLfloat g, GLfloat b )
{
   PRE_LOOPBACK( SecondaryColor3fEXT );
   CALL_SecondaryColor3fEXT(GET_DISPATCH(), ( r, g, b ));
}

static void GLAPIENTRY TAG(SecondaryColor3fvEXT)( const GLfloat *v )
{
   PRE_LOOPBACK( SecondaryColor3fvEXT );
   CALL_SecondaryColor3fvEXT(GET_DISPATCH(), ( v ));
}

static void GLAPIENTRY TAG(TexCoord1f)( GLfloat s )
{
   PRE_LOOPBACK( TexCoord1f );
   CALL_TexCoord1f(GET_DISPATCH(), ( s ));
}

static void GLAPIENTRY TAG(TexCoord1fv)( const GLfloat *tc )
{
   PRE_LOOPBACK( TexCoord1fv );
   CALL_TexCoord1fv(GET_DISPATCH(), ( tc ));
}

static void GLAPIENTRY TAG(TexCoord2f)( GLfloat s, GLfloat t )
{
   PRE_LOOPBACK( TexCoord2f );
   CALL_TexCoord2f(GET_DISPATCH(), ( s, t ));
}

static void GLAPIENTRY TAG(TexCoord2fv)( const GLfloat *tc )
{
   PRE_LOOPBACK( TexCoord2fv );
   CALL_TexCoord2fv(GET_DISPATCH(), ( tc ));
}

static void GLAPIENTRY TAG(TexCoord3f)( GLfloat s, GLfloat t, GLfloat r )
{
   PRE_LOOPBACK( TexCoord3f );
   CALL_TexCoord3f(GET_DISPATCH(), ( s, t, r ));
}

static void GLAPIENTRY TAG(TexCoord3fv)( const GLfloat *tc )
{
   PRE_LOOPBACK( TexCoord3fv );
   CALL_TexCoord3fv(GET_DISPATCH(), ( tc ));
}

static void GLAPIENTRY TAG(TexCoord4f)( GLfloat s, GLfloat t, GLfloat r, GLfloat q )
{
   PRE_LOOPBACK( TexCoord4f );
   CALL_TexCoord4f(GET_DISPATCH(), ( s, t, r, q ));
}

static void GLAPIENTRY TAG(TexCoord4fv)( const GLfloat *tc )
{
   PRE_LOOPBACK( TexCoord4fv );
   CALL_TexCoord4fv(GET_DISPATCH(), ( tc ));
}

static void GLAPIENTRY TAG(Vertex2f)( GLfloat x, GLfloat y )
{
   PRE_LOOPBACK( Vertex2f );
   CALL_Vertex2f(GET_DISPATCH(), ( x, y ));
}

static void GLAPIENTRY TAG(Vertex2fv)( const GLfloat *v )
{
   PRE_LOOPBACK( Vertex2fv );
   CALL_Vertex2fv(GET_DISPATCH(), ( v ));
}

static void GLAPIENTRY TAG(Vertex3f)( GLfloat x, GLfloat y, GLfloat z )
{
   PRE_LOOPBACK( Vertex3f );
   CALL_Vertex3f(GET_DISPATCH(), ( x, y, z ));
}

static void GLAPIENTRY TAG(Vertex3fv)( const GLfloat *v )
{
   PRE_LOOPBACK( Vertex3fv );
   CALL_Vertex3fv(GET_DISPATCH(), ( v ));
}

static void GLAPIENTRY TAG(Vertex4f)( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   PRE_LOOPBACK( Vertex4f );
   CALL_Vertex4f(GET_DISPATCH(), ( x, y, z, w ));
}

static void GLAPIENTRY TAG(Vertex4fv)( const GLfloat *v )
{
   PRE_LOOPBACK( Vertex4fv );
   CALL_Vertex4fv(GET_DISPATCH(), ( v ));
}

static void GLAPIENTRY TAG(CallList)( GLuint i )
{
   PRE_LOOPBACK( CallList );
   CALL_CallList(GET_DISPATCH(), ( i ));
}

static void GLAPIENTRY TAG(CallLists)( GLsizei sz, GLenum type, const GLvoid *v )
{
   PRE_LOOPBACK( CallLists );
   CALL_CallLists(GET_DISPATCH(), ( sz, type, v ));
}

static void GLAPIENTRY TAG(Begin)( GLenum mode )
{
   PRE_LOOPBACK( Begin );
   CALL_Begin(GET_DISPATCH(), ( mode ));
}

static void GLAPIENTRY TAG(End)( void )
{
   PRE_LOOPBACK( End );
   CALL_End(GET_DISPATCH(), ());
}

static void GLAPIENTRY TAG(Rectf)( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 )
{
   PRE_LOOPBACK( Rectf );
   CALL_Rectf(GET_DISPATCH(), ( x1, y1, x2, y2 ));
}

static void GLAPIENTRY TAG(DrawArrays)( GLenum mode, GLint start, GLsizei count )
{
   PRE_LOOPBACK( DrawArrays );
   CALL_DrawArrays(GET_DISPATCH(), ( mode, start, count ));
}

static void GLAPIENTRY TAG(DrawElements)( GLenum mode, GLsizei count, GLenum type,
			       const GLvoid *indices )
{
   PRE_LOOPBACK( DrawElements );
   CALL_DrawElements(GET_DISPATCH(), ( mode, count, type, indices ));
}

static void GLAPIENTRY TAG(DrawRangeElements)( GLenum mode, GLuint start,
				    GLuint end, GLsizei count,
				    GLenum type, const GLvoid *indices )
{
   PRE_LOOPBACK( DrawRangeElements );
   CALL_DrawRangeElements(GET_DISPATCH(), ( mode, start, end, count, type, indices ));
}

static void GLAPIENTRY TAG(EvalMesh1)( GLenum mode, GLint i1, GLint i2 )
{
   PRE_LOOPBACK( EvalMesh1 );
   CALL_EvalMesh1(GET_DISPATCH(), ( mode, i1, i2 ));
}

static void GLAPIENTRY TAG(EvalMesh2)( GLenum mode, GLint i1, GLint i2,
			    GLint j1, GLint j2 )
{
   PRE_LOOPBACK( EvalMesh2 );
   CALL_EvalMesh2(GET_DISPATCH(), ( mode, i1, i2, j1, j2 ));
}

static void GLAPIENTRY TAG(VertexAttrib1fNV)( GLuint index, GLfloat x )
{
   PRE_LOOPBACK( VertexAttrib1fNV );
   CALL_VertexAttrib1fNV(GET_DISPATCH(), ( index, x ));
}

static void GLAPIENTRY TAG(VertexAttrib1fvNV)( GLuint index, const GLfloat *v )
{
   PRE_LOOPBACK( VertexAttrib1fvNV );
   CALL_VertexAttrib1fvNV(GET_DISPATCH(), ( index, v ));
}

static void GLAPIENTRY TAG(VertexAttrib2fNV)( GLuint index, GLfloat x, GLfloat y )
{
   PRE_LOOPBACK( VertexAttrib2fNV );
   CALL_VertexAttrib2fNV(GET_DISPATCH(), ( index, x, y ));
}

static void GLAPIENTRY TAG(VertexAttrib2fvNV)( GLuint index, const GLfloat *v )
{
   PRE_LOOPBACK( VertexAttrib2fvNV );
   CALL_VertexAttrib2fvNV(GET_DISPATCH(), ( index, v ));
}

static void GLAPIENTRY TAG(VertexAttrib3fNV)( GLuint index, GLfloat x, GLfloat y, GLfloat z )
{
   PRE_LOOPBACK( VertexAttrib3fNV );
   CALL_VertexAttrib3fNV(GET_DISPATCH(), ( index, x, y, z ));
}

static void GLAPIENTRY TAG(VertexAttrib3fvNV)( GLuint index, const GLfloat *v )
{
   PRE_LOOPBACK( VertexAttrib3fvNV );
   CALL_VertexAttrib3fvNV(GET_DISPATCH(), ( index, v ));
}

static void GLAPIENTRY TAG(VertexAttrib4fNV)( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   PRE_LOOPBACK( VertexAttrib4fNV );
   CALL_VertexAttrib4fNV(GET_DISPATCH(), ( index, x, y, z, w ));
}

static void GLAPIENTRY TAG(VertexAttrib4fvNV)( GLuint index, const GLfloat *v )
{
   PRE_LOOPBACK( VertexAttrib4fvNV );
   CALL_VertexAttrib4fvNV(GET_DISPATCH(), ( index, v ));
}


static void GLAPIENTRY TAG(VertexAttrib1fARB)( GLuint index, GLfloat x )
{
   PRE_LOOPBACK( VertexAttrib1fARB );
   CALL_VertexAttrib1fARB(GET_DISPATCH(), ( index, x ));
}

static void GLAPIENTRY TAG(VertexAttrib1fvARB)( GLuint index, const GLfloat *v )
{
   PRE_LOOPBACK( VertexAttrib1fvARB );
   CALL_VertexAttrib1fvARB(GET_DISPATCH(), ( index, v ));
}

static void GLAPIENTRY TAG(VertexAttrib2fARB)( GLuint index, GLfloat x, GLfloat y )
{
   PRE_LOOPBACK( VertexAttrib2fARB );
   CALL_VertexAttrib2fARB(GET_DISPATCH(), ( index, x, y ));
}

static void GLAPIENTRY TAG(VertexAttrib2fvARB)( GLuint index, const GLfloat *v )
{
   PRE_LOOPBACK( VertexAttrib2fvARB );
   CALL_VertexAttrib2fvARB(GET_DISPATCH(), ( index, v ));
}

static void GLAPIENTRY TAG(VertexAttrib3fARB)( GLuint index, GLfloat x, GLfloat y, GLfloat z )
{
   PRE_LOOPBACK( VertexAttrib3fARB );
   CALL_VertexAttrib3fARB(GET_DISPATCH(), ( index, x, y, z ));
}

static void GLAPIENTRY TAG(VertexAttrib3fvARB)( GLuint index, const GLfloat *v )
{
   PRE_LOOPBACK( VertexAttrib3fvARB );
   CALL_VertexAttrib3fvARB(GET_DISPATCH(), ( index, v ));
}

static void GLAPIENTRY TAG(VertexAttrib4fARB)( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   PRE_LOOPBACK( VertexAttrib4fARB );
   CALL_VertexAttrib4fARB(GET_DISPATCH(), ( index, x, y, z, w ));
}

static void GLAPIENTRY TAG(VertexAttrib4fvARB)( GLuint index, const GLfloat *v )
{
   PRE_LOOPBACK( VertexAttrib4fvARB );
   CALL_VertexAttrib4fvARB(GET_DISPATCH(), ( index, v ));
}


static GLvertexformat TAG(vtxfmt) = {
   TAG(ArrayElement),
   TAG(Color3f),
   TAG(Color3fv),
   TAG(Color4f),
   TAG(Color4fv),
   TAG(EdgeFlag),
   TAG(EvalCoord1f),
   TAG(EvalCoord1fv),
   TAG(EvalCoord2f),
   TAG(EvalCoord2fv),
   TAG(EvalPoint1),
   TAG(EvalPoint2),
   TAG(FogCoordfEXT),
   TAG(FogCoordfvEXT),
   TAG(Indexf),
   TAG(Indexfv),
   TAG(Materialfv),
   TAG(MultiTexCoord1fARB),
   TAG(MultiTexCoord1fvARB),
   TAG(MultiTexCoord2fARB),
   TAG(MultiTexCoord2fvARB),
   TAG(MultiTexCoord3fARB),
   TAG(MultiTexCoord3fvARB),
   TAG(MultiTexCoord4fARB),
   TAG(MultiTexCoord4fvARB),
   TAG(Normal3f),
   TAG(Normal3fv),
   TAG(SecondaryColor3fEXT),
   TAG(SecondaryColor3fvEXT),
   TAG(TexCoord1f),
   TAG(TexCoord1fv),
   TAG(TexCoord2f),
   TAG(TexCoord2fv),
   TAG(TexCoord3f),
   TAG(TexCoord3fv),
   TAG(TexCoord4f),
   TAG(TexCoord4fv),
   TAG(Vertex2f),
   TAG(Vertex2fv),
   TAG(Vertex3f),
   TAG(Vertex3fv),
   TAG(Vertex4f),
   TAG(Vertex4fv),
   TAG(CallList),
   TAG(CallLists),
   TAG(Begin),
   TAG(End),
   TAG(VertexAttrib1fNV),
   TAG(VertexAttrib1fvNV),
   TAG(VertexAttrib2fNV),
   TAG(VertexAttrib2fvNV),
   TAG(VertexAttrib3fNV),
   TAG(VertexAttrib3fvNV),
   TAG(VertexAttrib4fNV),
   TAG(VertexAttrib4fvNV),
   TAG(VertexAttrib1fARB),
   TAG(VertexAttrib1fvARB),
   TAG(VertexAttrib2fARB),
   TAG(VertexAttrib2fvARB),
   TAG(VertexAttrib3fARB),
   TAG(VertexAttrib3fvARB),
   TAG(VertexAttrib4fARB),
   TAG(VertexAttrib4fvARB),
   TAG(Rectf),
   TAG(DrawArrays),
   TAG(DrawElements),
   TAG(DrawRangeElements),
   TAG(EvalMesh1),
   TAG(EvalMesh2)
};

#undef TAG
#undef PRE_LOOPBACK
