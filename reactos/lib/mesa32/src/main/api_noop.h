
/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
 */

#ifndef _API_NOOP_H
#define _API_NOOP_H


#include "glheader.h"
#include "mtypes.h"
#include "context.h"

/* In states where certain vertex components are required for t&l or
 * rasterization, we still need to keep track of the current values.
 * These functions provide this service by keeping uptodate the
 * 'ctx->Current' struct for all data elements not included in the
 * currently enabled hardware vertex.
 *
 */
extern void GLAPIENTRY _mesa_noop_EdgeFlag( GLboolean b );
extern void GLAPIENTRY _mesa_noop_EdgeFlagv( const GLboolean *b );

extern void GLAPIENTRY _mesa_noop_FogCoordfEXT( GLfloat a );
extern void GLAPIENTRY _mesa_noop_FogCoordfvEXT( const GLfloat *v );

extern void GLAPIENTRY _mesa_noop_Indexf( GLfloat i );
extern void GLAPIENTRY _mesa_noop_Indexfv( const GLfloat *v );

extern void GLAPIENTRY _mesa_noop_Normal3f( GLfloat a, GLfloat b, GLfloat c );
extern void GLAPIENTRY _mesa_noop_Normal3fv( const GLfloat *v );

extern void GLAPIENTRY _mesa_noop_Materialfv(  GLenum face, GLenum pname,  const GLfloat *param );

extern void _mesa_noop_Color4ub( GLubyte a, GLubyte b, GLubyte c, GLubyte d );
extern void _mesa_noop_Color4ubv( const GLubyte *v );
extern void GLAPIENTRY _mesa_noop_Color4f( GLfloat a, GLfloat b, GLfloat c, GLfloat d );
extern void GLAPIENTRY _mesa_noop_Color4fv( const GLfloat *v );
extern void _mesa_noop_Color3ub( GLubyte a, GLubyte b, GLubyte c );
extern void _mesa_noop_Color3ubv( const GLubyte *v );
extern void GLAPIENTRY _mesa_noop_Color3f( GLfloat a, GLfloat b, GLfloat c );
extern void GLAPIENTRY _mesa_noop_Color3fv( const GLfloat *v );

extern void GLAPIENTRY _mesa_noop_MultiTexCoord1fARB( GLenum target, GLfloat a );
extern void GLAPIENTRY _mesa_noop_MultiTexCoord1fvARB( GLenum target, const GLfloat *v );
extern void GLAPIENTRY _mesa_noop_MultiTexCoord2fARB( GLenum target, GLfloat a, GLfloat b );
extern void GLAPIENTRY _mesa_noop_MultiTexCoord2fvARB( GLenum target, const GLfloat *v );
extern void GLAPIENTRY _mesa_noop_MultiTexCoord3fARB( GLenum target, GLfloat a, GLfloat b, GLfloat c);
extern void GLAPIENTRY _mesa_noop_MultiTexCoord3fvARB( GLenum target, const GLfloat *v );
extern void GLAPIENTRY _mesa_noop_MultiTexCoord4fARB( GLenum target, GLfloat a, GLfloat b, GLfloat c, GLfloat d );
extern void GLAPIENTRY _mesa_noop_MultiTexCoord4fvARB( GLenum target, const GLfloat *v );

extern void GLAPIENTRY _mesa_noop_SecondaryColor3ubEXT( GLubyte a, GLubyte b, GLubyte c );
extern void GLAPIENTRY _mesa_noop_SecondaryColor3ubvEXT( const GLubyte *v );
extern void GLAPIENTRY _mesa_noop_SecondaryColor3fEXT( GLfloat a, GLfloat b, GLfloat c );
extern void GLAPIENTRY _mesa_noop_SecondaryColor3fvEXT( const GLfloat *v );

extern void GLAPIENTRY _mesa_noop_TexCoord1f( GLfloat a );
extern void GLAPIENTRY _mesa_noop_TexCoord1fv( const GLfloat *v );
extern void GLAPIENTRY _mesa_noop_TexCoord2f( GLfloat a, GLfloat b );
extern void GLAPIENTRY _mesa_noop_TexCoord2fv( const GLfloat *v );
extern void GLAPIENTRY _mesa_noop_TexCoord3f( GLfloat a, GLfloat b, GLfloat c );
extern void GLAPIENTRY _mesa_noop_TexCoord3fv( const GLfloat *v );
extern void GLAPIENTRY _mesa_noop_TexCoord4f( GLfloat a, GLfloat b, GLfloat c, GLfloat d );
extern void GLAPIENTRY _mesa_noop_TexCoord4fv( const GLfloat *v );

extern void GLAPIENTRY _mesa_noop_Vertex2f( GLfloat a, GLfloat b );
extern void GLAPIENTRY _mesa_noop_Vertex2fv( const GLfloat *v );
extern void GLAPIENTRY _mesa_noop_Vertex3f( GLfloat a, GLfloat b, GLfloat c );
extern void GLAPIENTRY _mesa_noop_Vertex3fv( const GLfloat *v );
extern void GLAPIENTRY _mesa_noop_Vertex4f( GLfloat a, GLfloat b, GLfloat c, GLfloat d );
extern void GLAPIENTRY _mesa_noop_Vertex4fv( const GLfloat *v );

extern void GLAPIENTRY _mesa_noop_VertexAttrib1fNV( GLuint index, GLfloat x );
extern void GLAPIENTRY _mesa_noop_VertexAttrib1fvNV( GLuint index, const GLfloat *v );
extern void GLAPIENTRY _mesa_noop_VertexAttrib2fNV( GLuint index, GLfloat x, GLfloat y );
extern void GLAPIENTRY _mesa_noop_VertexAttrib2fvNV( GLuint index, const GLfloat *v );
extern void GLAPIENTRY _mesa_noop_VertexAttrib3fNV( GLuint index, GLfloat x, GLfloat y, GLfloat z );
extern void GLAPIENTRY _mesa_noop_VertexAttrib3fvNV( GLuint index, const GLfloat *v );
extern void GLAPIENTRY _mesa_noop_VertexAttrib4fNV( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );

extern void GLAPIENTRY _mesa_noop_VertexAttrib4fvNV( GLuint index, const GLfloat *v );
extern void GLAPIENTRY _mesa_noop_VertexAttrib1fARB( GLuint index, GLfloat x );
extern void GLAPIENTRY _mesa_noop_VertexAttrib1fvARB( GLuint index, const GLfloat *v );
extern void GLAPIENTRY _mesa_noop_VertexAttrib2fARB( GLuint index, GLfloat x, GLfloat y );
extern void GLAPIENTRY _mesa_noop_VertexAttrib2fvARB( GLuint index, const GLfloat *v );
extern void GLAPIENTRY _mesa_noop_VertexAttrib3fARB( GLuint index, GLfloat x, GLfloat y, GLfloat z );
extern void GLAPIENTRY _mesa_noop_VertexAttrib3fvARB( GLuint index, const GLfloat *v );
extern void GLAPIENTRY _mesa_noop_VertexAttrib4fARB( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w );
extern void GLAPIENTRY _mesa_noop_VertexAttrib4fvARB( GLuint index, const GLfloat *v );


extern void GLAPIENTRY _mesa_noop_End( void );
extern void GLAPIENTRY _mesa_noop_Begin( GLenum mode );
extern void GLAPIENTRY _mesa_noop_EvalPoint2( GLint a, GLint b );
extern void GLAPIENTRY _mesa_noop_EvalPoint1( GLint a );
extern void GLAPIENTRY _mesa_noop_EvalCoord2fv( const GLfloat *v );
extern void GLAPIENTRY _mesa_noop_EvalCoord2f( GLfloat a, GLfloat b );
extern void GLAPIENTRY _mesa_noop_EvalCoord1fv( const GLfloat *v );
extern void GLAPIENTRY _mesa_noop_EvalCoord1f( GLfloat a );

extern void _mesa_noop_vtxfmt_init( GLvertexformat *vfmt );

extern void GLAPIENTRY _mesa_noop_EvalMesh2( GLenum mode, GLint i1, GLint i2, 
				  GLint j1, GLint j2 );

extern void GLAPIENTRY _mesa_noop_EvalMesh1( GLenum mode, GLint i1, GLint i2 );


/* Not strictly a noop -- translate Rectf down to Begin/End and
 * vertices.  Closer to the loopback operations, but doesn't meet the
 * criteria for inclusion there (cannot be used in the Save table).
 */
extern void GLAPIENTRY _mesa_noop_Rectf( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 );


extern void GLAPIENTRY _mesa_noop_DrawArrays(GLenum mode, GLint start, GLsizei count);
extern void GLAPIENTRY _mesa_noop_DrawElements(GLenum mode, GLsizei count, GLenum type,
				    const GLvoid *indices);
extern void GLAPIENTRY _mesa_noop_DrawRangeElements(GLenum mode,
					 GLuint start, GLuint end,
					 GLsizei count, GLenum type,
					 const GLvoid *indices);



#endif
