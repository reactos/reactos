/**
 * \file api_loopback.c
 *
 * \author Keith Whitwell <keith@tungstengraphics.com>
 */

/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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


#include "glheader.h"
#include "glapi.h"
#include "glapitable.h"
#include "macros.h"
#include "colormac.h"
#include "api_loopback.h"

/* KW: A set of functions to convert unusual Color/Normal/Vertex/etc
 * calls to a smaller set of driver-provided formats.  Currently just
 * go back to dispatch to find these (eg. call glNormal3f directly),
 * hence 'loopback'.
 *
 * The driver must supply all of the remaining entry points, which are
 * listed in dd.h.  The easiest way for a driver to do this is to
 * install the supplied software t&l module.
 */
#define COLORF(r,g,b,a)             glColor4f(r,g,b,a)
#define VERTEX2(x,y)	            glVertex2f(x,y)
#define VERTEX3(x,y,z)	            glVertex3f(x,y,z)
#define VERTEX4(x,y,z,w)            glVertex4f(x,y,z,w)
#define NORMAL(x,y,z)               glNormal3f(x,y,z)
#define TEXCOORD1(s)                glTexCoord1f(s)
#define TEXCOORD2(s,t)              glTexCoord2f(s,t)
#define TEXCOORD3(s,t,u)            glTexCoord3f(s,t,u)
#define TEXCOORD4(s,t,u,v)          glTexCoord4f(s,t,u,v)
#define INDEX(c)		    glIndexf(c)
#define MULTI_TEXCOORD1(z,s)	    glMultiTexCoord1fARB(z,s)
#define MULTI_TEXCOORD2(z,s,t)	    glMultiTexCoord2fARB(z,s,t)
#define MULTI_TEXCOORD3(z,s,t,u)    glMultiTexCoord3fARB(z,s,t,u)
#define MULTI_TEXCOORD4(z,s,t,u,v)  glMultiTexCoord4fARB(z,s,t,u,v)
#define EVALCOORD1(x)               glEvalCoord1f(x)
#define EVALCOORD2(x,y)             glEvalCoord2f(x,y)
#define MATERIALFV(a,b,c)           glMaterialfv(a,b,c)
#define RECTF(a,b,c,d)              glRectf(a,b,c,d)

/* Extension functions must be dereferenced through _glapi_Dispatch as
 * not all libGL.so's will have all the uptodate entrypoints.
 */
#define ATTRIB1(index,x)        _glapi_Dispatch->VertexAttrib1fNV(index,x)
#define ATTRIB2(index,x,y)      _glapi_Dispatch->VertexAttrib2fNV(index,x,y)
#define ATTRIB3(index,x,y,z)    _glapi_Dispatch->VertexAttrib3fNV(index,x,y,z)
#define ATTRIB4(index,x,y,z,w)  _glapi_Dispatch->VertexAttrib4fNV(index,x,y,z,w)
#define FOGCOORDF(x)            _glapi_Dispatch->FogCoordfEXT(x)
#define SECONDARYCOLORF(a,b,c)  _glapi_Dispatch->SecondaryColor3fEXT(a,b,c)

static void GLAPIENTRY
loopback_Color3b_f( GLbyte red, GLbyte green, GLbyte blue )
{
   COLORF( BYTE_TO_FLOAT(red),
	   BYTE_TO_FLOAT(green),
	   BYTE_TO_FLOAT(blue),
	   1.0 );
}

static void GLAPIENTRY
loopback_Color3d_f( GLdouble red, GLdouble green, GLdouble blue )
{
   COLORF( (GLfloat) red, (GLfloat) green, (GLfloat) blue, 1.0 );
}

static void GLAPIENTRY
loopback_Color3i_f( GLint red, GLint green, GLint blue )
{
   COLORF( INT_TO_FLOAT(red), INT_TO_FLOAT(green),
	   INT_TO_FLOAT(blue), 1.0);
}

static void GLAPIENTRY
loopback_Color3s_f( GLshort red, GLshort green, GLshort blue )
{
   COLORF( SHORT_TO_FLOAT(red), SHORT_TO_FLOAT(green),
	   SHORT_TO_FLOAT(blue), 1.0);
}

static void GLAPIENTRY
loopback_Color3ui_f( GLuint red, GLuint green, GLuint blue )
{
   COLORF( UINT_TO_FLOAT(red), UINT_TO_FLOAT(green),
	   UINT_TO_FLOAT(blue), 1.0 );
}

static void GLAPIENTRY
loopback_Color3us_f( GLushort red, GLushort green, GLushort blue )
{
   COLORF( USHORT_TO_FLOAT(red), USHORT_TO_FLOAT(green),
	   USHORT_TO_FLOAT(blue), 1.0 );
}

static void GLAPIENTRY
loopback_Color3ub_f( GLubyte red, GLubyte green, GLubyte blue )
{
   COLORF( UBYTE_TO_FLOAT(red), UBYTE_TO_FLOAT(green),
	   UBYTE_TO_FLOAT(blue), 1.0 );
}


static void GLAPIENTRY
loopback_Color3bv_f( const GLbyte *v )
{
   COLORF( BYTE_TO_FLOAT(v[0]), BYTE_TO_FLOAT(v[1]),
	   BYTE_TO_FLOAT(v[2]), 1.0 );
}

static void GLAPIENTRY
loopback_Color3dv_f( const GLdouble *v )
{
   COLORF( (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 1.0 );
}

static void GLAPIENTRY
loopback_Color3iv_f( const GLint *v )
{
   COLORF( INT_TO_FLOAT(v[0]), INT_TO_FLOAT(v[1]),
	   INT_TO_FLOAT(v[2]), INT_TO_FLOAT(v[3]) );
}

static void GLAPIENTRY
loopback_Color3sv_f( const GLshort *v )
{
   COLORF( SHORT_TO_FLOAT(v[0]), SHORT_TO_FLOAT(v[1]),
	   SHORT_TO_FLOAT(v[2]), 1.0 );
}

static void GLAPIENTRY
loopback_Color3uiv_f( const GLuint *v )
{
   COLORF( UINT_TO_FLOAT(v[0]), UINT_TO_FLOAT(v[1]),
	   UINT_TO_FLOAT(v[2]), 1.0 );
}

static void GLAPIENTRY
loopback_Color3usv_f( const GLushort *v )
{
   COLORF( USHORT_TO_FLOAT(v[0]), USHORT_TO_FLOAT(v[1]),
	   USHORT_TO_FLOAT(v[2]), 1.0 );
}

static void GLAPIENTRY
loopback_Color3ubv_f( const GLubyte *v )
{
   COLORF( UBYTE_TO_FLOAT(v[0]), UBYTE_TO_FLOAT(v[1]),
	   UBYTE_TO_FLOAT(v[2]), 1.0 );
}


static void GLAPIENTRY
loopback_Color4b_f( GLbyte red, GLbyte green, GLbyte blue,
			      GLbyte alpha )
{
   COLORF( BYTE_TO_FLOAT(red), BYTE_TO_FLOAT(green),
	   BYTE_TO_FLOAT(blue), BYTE_TO_FLOAT(alpha) );
}

static void GLAPIENTRY
loopback_Color4d_f( GLdouble red, GLdouble green, GLdouble blue,
			      GLdouble alpha )
{
   COLORF( (GLfloat) red, (GLfloat) green, (GLfloat) blue, (GLfloat) alpha );
}

static void GLAPIENTRY
loopback_Color4i_f( GLint red, GLint green, GLint blue, GLint alpha )
{
   COLORF( INT_TO_FLOAT(red), INT_TO_FLOAT(green),
	   INT_TO_FLOAT(blue), INT_TO_FLOAT(alpha) );
}

static void GLAPIENTRY
loopback_Color4s_f( GLshort red, GLshort green, GLshort blue,
			      GLshort alpha )
{
   COLORF( SHORT_TO_FLOAT(red), SHORT_TO_FLOAT(green),
	   SHORT_TO_FLOAT(blue), SHORT_TO_FLOAT(alpha) );
}

static void GLAPIENTRY
loopback_Color4ui_f( GLuint red, GLuint green, GLuint blue, GLuint alpha )
{
   COLORF( UINT_TO_FLOAT(red), UINT_TO_FLOAT(green),
	   UINT_TO_FLOAT(blue), UINT_TO_FLOAT(alpha) );
}

static void GLAPIENTRY
loopback_Color4us_f( GLushort red, GLushort green, GLushort blue, GLushort alpha )
{
   COLORF( USHORT_TO_FLOAT(red), USHORT_TO_FLOAT(green),
	   USHORT_TO_FLOAT(blue), USHORT_TO_FLOAT(alpha) );
}

static void GLAPIENTRY
loopback_Color4ub_f( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha )
{
   COLORF( UBYTE_TO_FLOAT(red), UBYTE_TO_FLOAT(green),
	   UBYTE_TO_FLOAT(blue), UBYTE_TO_FLOAT(alpha) );
}


static void GLAPIENTRY
loopback_Color4iv_f( const GLint *v )
{
   COLORF( INT_TO_FLOAT(v[0]), INT_TO_FLOAT(v[1]),
	   INT_TO_FLOAT(v[2]), INT_TO_FLOAT(v[3]) );
}


static void GLAPIENTRY
loopback_Color4bv_f( const GLbyte *v )
{
   COLORF( BYTE_TO_FLOAT(v[0]), BYTE_TO_FLOAT(v[1]),
	   BYTE_TO_FLOAT(v[2]), BYTE_TO_FLOAT(v[3]) );
}

static void GLAPIENTRY
loopback_Color4dv_f( const GLdouble *v )
{
   COLORF( (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3] );
}


static void GLAPIENTRY
loopback_Color4sv_f( const GLshort *v)
{
   COLORF( SHORT_TO_FLOAT(v[0]), SHORT_TO_FLOAT(v[1]),
	   SHORT_TO_FLOAT(v[2]), SHORT_TO_FLOAT(v[3]) );
}


static void GLAPIENTRY
loopback_Color4uiv_f( const GLuint *v)
{
   COLORF( UINT_TO_FLOAT(v[0]), UINT_TO_FLOAT(v[1]),
	   UINT_TO_FLOAT(v[2]), UINT_TO_FLOAT(v[3]) );
}

static void GLAPIENTRY
loopback_Color4usv_f( const GLushort *v)
{
   COLORF( USHORT_TO_FLOAT(v[0]), USHORT_TO_FLOAT(v[1]),
	   USHORT_TO_FLOAT(v[2]), USHORT_TO_FLOAT(v[3]) );
}

static void GLAPIENTRY
loopback_Color4ubv_f( const GLubyte *v)
{
   COLORF( UBYTE_TO_FLOAT(v[0]), UBYTE_TO_FLOAT(v[1]),
	   UBYTE_TO_FLOAT(v[2]), UBYTE_TO_FLOAT(v[3]) );
}

static void GLAPIENTRY
loopback_FogCoorddEXT( GLdouble d )
{
   FOGCOORDF( (GLfloat) d );
}

static void GLAPIENTRY
loopback_FogCoorddvEXT( const GLdouble *v )
{
   FOGCOORDF( (GLfloat) *v );
}


static void GLAPIENTRY
loopback_Indexd( GLdouble c )
{
   INDEX( (GLfloat) c );
}

static void GLAPIENTRY
loopback_Indexi( GLint c )
{
   INDEX( (GLfloat) c );
}

static void GLAPIENTRY
loopback_Indexs( GLshort c )
{
   INDEX( (GLfloat) c );
}

static void GLAPIENTRY
loopback_Indexub( GLubyte c )
{
   INDEX( (GLfloat) c );
}

static void GLAPIENTRY
loopback_Indexdv( const GLdouble *c )
{
   INDEX( (GLfloat) *c );
}

static void GLAPIENTRY
loopback_Indexiv( const GLint *c )
{
   INDEX( (GLfloat) *c );
}

static void GLAPIENTRY
loopback_Indexsv( const GLshort *c )
{
   INDEX( (GLfloat) *c );
}

static void GLAPIENTRY
loopback_Indexubv( const GLubyte *c )
{
   INDEX( (GLfloat) *c );
}

static void GLAPIENTRY
loopback_Normal3b( GLbyte nx, GLbyte ny, GLbyte nz )
{
   NORMAL( BYTE_TO_FLOAT(nx), BYTE_TO_FLOAT(ny), BYTE_TO_FLOAT(nz) );
}

static void GLAPIENTRY
loopback_Normal3d( GLdouble nx, GLdouble ny, GLdouble nz )
{
   NORMAL((GLfloat) nx, (GLfloat) ny, (GLfloat) nz);
}

static void GLAPIENTRY
loopback_Normal3i( GLint nx, GLint ny, GLint nz )
{
   NORMAL( INT_TO_FLOAT(nx), INT_TO_FLOAT(ny), INT_TO_FLOAT(nz) );
}

static void GLAPIENTRY
loopback_Normal3s( GLshort nx, GLshort ny, GLshort nz )
{
   NORMAL( SHORT_TO_FLOAT(nx), SHORT_TO_FLOAT(ny), SHORT_TO_FLOAT(nz) );
}

static void GLAPIENTRY
loopback_Normal3bv( const GLbyte *v )
{
   NORMAL( BYTE_TO_FLOAT(v[0]), BYTE_TO_FLOAT(v[1]), BYTE_TO_FLOAT(v[2]) );
}

static void GLAPIENTRY
loopback_Normal3dv( const GLdouble *v )
{
   NORMAL( (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}

static void GLAPIENTRY
loopback_Normal3iv( const GLint *v )
{
   NORMAL( INT_TO_FLOAT(v[0]), INT_TO_FLOAT(v[1]), INT_TO_FLOAT(v[2]) );
}

static void GLAPIENTRY
loopback_Normal3sv( const GLshort *v )
{
   NORMAL( SHORT_TO_FLOAT(v[0]), SHORT_TO_FLOAT(v[1]), SHORT_TO_FLOAT(v[2]) );
}

static void GLAPIENTRY
loopback_TexCoord1d( GLdouble s )
{
   TEXCOORD1((GLfloat) s);
}

static void GLAPIENTRY
loopback_TexCoord1i( GLint s )
{
   TEXCOORD1((GLfloat) s);
}

static void GLAPIENTRY
loopback_TexCoord1s( GLshort s )
{
   TEXCOORD1((GLfloat) s);
}

static void GLAPIENTRY
loopback_TexCoord2d( GLdouble s, GLdouble t )
{
   TEXCOORD2((GLfloat) s,(GLfloat) t);
}

static void GLAPIENTRY
loopback_TexCoord2s( GLshort s, GLshort t )
{
   TEXCOORD2((GLfloat) s,(GLfloat) t);
}

static void GLAPIENTRY
loopback_TexCoord2i( GLint s, GLint t )
{
   TEXCOORD2((GLfloat) s,(GLfloat) t);
}

static void GLAPIENTRY
loopback_TexCoord3d( GLdouble s, GLdouble t, GLdouble r )
{
   TEXCOORD3((GLfloat) s,(GLfloat) t,(GLfloat) r);
}

static void GLAPIENTRY
loopback_TexCoord3i( GLint s, GLint t, GLint r )
{
   TEXCOORD3((GLfloat) s,(GLfloat) t,(GLfloat) r);
}

static void GLAPIENTRY
loopback_TexCoord3s( GLshort s, GLshort t, GLshort r )
{
   TEXCOORD3((GLfloat) s,(GLfloat) t,(GLfloat) r);
}

static void GLAPIENTRY
loopback_TexCoord4d( GLdouble s, GLdouble t, GLdouble r, GLdouble q )
{
   TEXCOORD4((GLfloat) s,(GLfloat) t,(GLfloat) r,(GLfloat) q);
}

static void GLAPIENTRY
loopback_TexCoord4i( GLint s, GLint t, GLint r, GLint q )
{
   TEXCOORD4((GLfloat) s,(GLfloat) t,(GLfloat) r,(GLfloat) q);
}

static void GLAPIENTRY
loopback_TexCoord4s( GLshort s, GLshort t, GLshort r, GLshort q )
{
   TEXCOORD4((GLfloat) s,(GLfloat) t,(GLfloat) r,(GLfloat) q);
}

static void GLAPIENTRY
loopback_TexCoord1dv( const GLdouble *v )
{
   TEXCOORD1((GLfloat) v[0]);
}

static void GLAPIENTRY
loopback_TexCoord1iv( const GLint *v )
{
   TEXCOORD1((GLfloat) v[0]);
}

static void GLAPIENTRY
loopback_TexCoord1sv( const GLshort *v )
{
   TEXCOORD1((GLfloat) v[0]);
}

static void GLAPIENTRY
loopback_TexCoord2dv( const GLdouble *v )
{
   TEXCOORD2((GLfloat) v[0],(GLfloat) v[1]);
}

static void GLAPIENTRY
loopback_TexCoord2iv( const GLint *v )
{
   TEXCOORD2((GLfloat) v[0],(GLfloat) v[1]);
}

static void GLAPIENTRY
loopback_TexCoord2sv( const GLshort *v )
{
   TEXCOORD2((GLfloat) v[0],(GLfloat) v[1]);
}

static void GLAPIENTRY
loopback_TexCoord3dv( const GLdouble *v )
{
   TEXCOORD2((GLfloat) v[0],(GLfloat) v[1]);
}

static void GLAPIENTRY
loopback_TexCoord3iv( const GLint *v )
{
   TEXCOORD3((GLfloat) v[0],(GLfloat) v[1],(GLfloat) v[2]);
}

static void GLAPIENTRY
loopback_TexCoord3sv( const GLshort *v )
{
   TEXCOORD3((GLfloat) v[0],(GLfloat) v[1],(GLfloat) v[2]);
}

static void GLAPIENTRY
loopback_TexCoord4dv( const GLdouble *v )
{
   TEXCOORD4((GLfloat) v[0],(GLfloat) v[1],(GLfloat) v[2],(GLfloat) v[3]);
}

static void GLAPIENTRY
loopback_TexCoord4iv( const GLint *v )
{
   TEXCOORD4((GLfloat) v[0],(GLfloat) v[1],(GLfloat) v[2],(GLfloat) v[3]);
}

static void GLAPIENTRY
loopback_TexCoord4sv( const GLshort *v )
{
   TEXCOORD4((GLfloat) v[0],(GLfloat) v[1],(GLfloat) v[2],(GLfloat) v[3]);
}

static void GLAPIENTRY
loopback_Vertex2d( GLdouble x, GLdouble y )
{
   VERTEX2( (GLfloat) x, (GLfloat) y );
}

static void GLAPIENTRY
loopback_Vertex2i( GLint x, GLint y )
{
   VERTEX2( (GLfloat) x, (GLfloat) y );
}

static void GLAPIENTRY
loopback_Vertex2s( GLshort x, GLshort y )
{
   VERTEX2( (GLfloat) x, (GLfloat) y );
}

static void GLAPIENTRY
loopback_Vertex3d( GLdouble x, GLdouble y, GLdouble z )
{
   VERTEX3( (GLfloat) x, (GLfloat) y, (GLfloat) z );
}

static void GLAPIENTRY
loopback_Vertex3i( GLint x, GLint y, GLint z )
{
   VERTEX3( (GLfloat) x, (GLfloat) y, (GLfloat) z );
}

static void GLAPIENTRY
loopback_Vertex3s( GLshort x, GLshort y, GLshort z )
{
   VERTEX3( (GLfloat) x, (GLfloat) y, (GLfloat) z );
}

static void GLAPIENTRY
loopback_Vertex4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
   VERTEX4( (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w );
}

static void GLAPIENTRY
loopback_Vertex4i( GLint x, GLint y, GLint z, GLint w )
{
   VERTEX4( (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w );
}

static void GLAPIENTRY
loopback_Vertex4s( GLshort x, GLshort y, GLshort z, GLshort w )
{
   VERTEX4( (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w );
}

static void GLAPIENTRY
loopback_Vertex2dv( const GLdouble *v )
{
   VERTEX2( (GLfloat) v[0], (GLfloat) v[1] );
}

static void GLAPIENTRY
loopback_Vertex2iv( const GLint *v )
{
   VERTEX2( (GLfloat) v[0], (GLfloat) v[1] );
}

static void GLAPIENTRY
loopback_Vertex2sv( const GLshort *v )
{
   VERTEX2( (GLfloat) v[0], (GLfloat) v[1] );
}

static void GLAPIENTRY
loopback_Vertex3dv( const GLdouble *v )
{
   VERTEX3( (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}

static void GLAPIENTRY
loopback_Vertex3iv( const GLint *v )
{
   VERTEX3( (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}

static void GLAPIENTRY
loopback_Vertex3sv( const GLshort *v )
{
   VERTEX3( (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}

static void GLAPIENTRY
loopback_Vertex4dv( const GLdouble *v )
{
   VERTEX4( (GLfloat) v[0], (GLfloat) v[1],
	    (GLfloat) v[2], (GLfloat) v[3] );
}

static void GLAPIENTRY
loopback_Vertex4iv( const GLint *v )
{
   VERTEX4( (GLfloat) v[0], (GLfloat) v[1],
	    (GLfloat) v[2], (GLfloat) v[3] );
}

static void GLAPIENTRY
loopback_Vertex4sv( const GLshort *v )
{
   VERTEX4( (GLfloat) v[0], (GLfloat) v[1],
	    (GLfloat) v[2], (GLfloat) v[3] );
}

static void GLAPIENTRY
loopback_MultiTexCoord1dARB(GLenum target, GLdouble s)
{
   MULTI_TEXCOORD1( target, (GLfloat) s );
}

static void GLAPIENTRY
loopback_MultiTexCoord1dvARB(GLenum target, const GLdouble *v)
{
   MULTI_TEXCOORD1( target, (GLfloat) v[0] );
}

static void GLAPIENTRY
loopback_MultiTexCoord1iARB(GLenum target, GLint s)
{
   MULTI_TEXCOORD1( target, (GLfloat) s );
}

static void GLAPIENTRY
loopback_MultiTexCoord1ivARB(GLenum target, const GLint *v)
{
   MULTI_TEXCOORD1( target, (GLfloat) v[0] );
}

static void GLAPIENTRY
loopback_MultiTexCoord1sARB(GLenum target, GLshort s)
{
   MULTI_TEXCOORD1( target, (GLfloat) s );
}

static void GLAPIENTRY
loopback_MultiTexCoord1svARB(GLenum target, const GLshort *v)
{
   MULTI_TEXCOORD1( target, (GLfloat) v[0] );
}

static void GLAPIENTRY
loopback_MultiTexCoord2dARB(GLenum target, GLdouble s, GLdouble t)
{
   MULTI_TEXCOORD2( target, (GLfloat) s, (GLfloat) t );
}

static void GLAPIENTRY
loopback_MultiTexCoord2dvARB(GLenum target, const GLdouble *v)
{
   MULTI_TEXCOORD2( target, (GLfloat) v[0], (GLfloat) v[1] );
}

static void GLAPIENTRY
loopback_MultiTexCoord2iARB(GLenum target, GLint s, GLint t)
{
   MULTI_TEXCOORD2( target, (GLfloat) s, (GLfloat) t );
}

static void GLAPIENTRY
loopback_MultiTexCoord2ivARB(GLenum target, const GLint *v)
{
   MULTI_TEXCOORD2( target, (GLfloat) v[0], (GLfloat) v[1] );
}

static void GLAPIENTRY
loopback_MultiTexCoord2sARB(GLenum target, GLshort s, GLshort t)
{
   MULTI_TEXCOORD2( target, (GLfloat) s, (GLfloat) t );
}

static void GLAPIENTRY
loopback_MultiTexCoord2svARB(GLenum target, const GLshort *v)
{
   MULTI_TEXCOORD2( target, (GLfloat) v[0], (GLfloat) v[1] );
}

static void GLAPIENTRY
loopback_MultiTexCoord3dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r)
{
   MULTI_TEXCOORD3( target, (GLfloat) s, (GLfloat) t, (GLfloat) r );
}

static void GLAPIENTRY
loopback_MultiTexCoord3dvARB(GLenum target, const GLdouble *v)
{
   MULTI_TEXCOORD3( target, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}

static void GLAPIENTRY
loopback_MultiTexCoord3iARB(GLenum target, GLint s, GLint t, GLint r)
{
   MULTI_TEXCOORD3( target, (GLfloat) s, (GLfloat) t, (GLfloat) r );
}

static void GLAPIENTRY
loopback_MultiTexCoord3ivARB(GLenum target, const GLint *v)
{
   MULTI_TEXCOORD3( target, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}

static void GLAPIENTRY
loopback_MultiTexCoord3sARB(GLenum target, GLshort s, GLshort t, GLshort r)
{
   MULTI_TEXCOORD3( target, (GLfloat) s, (GLfloat) t, (GLfloat) r );
}

static void GLAPIENTRY
loopback_MultiTexCoord3svARB(GLenum target, const GLshort *v)
{
   MULTI_TEXCOORD3( target, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}

static void GLAPIENTRY
loopback_MultiTexCoord4dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
   MULTI_TEXCOORD4( target, (GLfloat) s, (GLfloat) t, 
		    (GLfloat) r, (GLfloat) q );
}

static void GLAPIENTRY
loopback_MultiTexCoord4dvARB(GLenum target, const GLdouble *v)
{
   MULTI_TEXCOORD4( target, (GLfloat) v[0], (GLfloat) v[1], 
		    (GLfloat) v[2], (GLfloat) v[3] );
}

static void GLAPIENTRY
loopback_MultiTexCoord4iARB(GLenum target, GLint s, GLint t, GLint r, GLint q)
{
   MULTI_TEXCOORD4( target, (GLfloat) s, (GLfloat) t,
		    (GLfloat) r, (GLfloat) q );
}

static void GLAPIENTRY
loopback_MultiTexCoord4ivARB(GLenum target, const GLint *v)
{
   MULTI_TEXCOORD4( target, (GLfloat) v[0], (GLfloat) v[1],
		    (GLfloat) v[2], (GLfloat) v[3] );
}

static void GLAPIENTRY
loopback_MultiTexCoord4sARB(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q)
{
   MULTI_TEXCOORD4( target, (GLfloat) s, (GLfloat) t,
		    (GLfloat) r, (GLfloat) q );
}

static void GLAPIENTRY
loopback_MultiTexCoord4svARB(GLenum target, const GLshort *v)
{
   MULTI_TEXCOORD4( target, (GLfloat) v[0], (GLfloat) v[1],
		    (GLfloat) v[2], (GLfloat) v[3] );
}

static void GLAPIENTRY
loopback_EvalCoord2dv( const GLdouble *u )
{
   EVALCOORD2( (GLfloat) u[0], (GLfloat) u[1] );
}

static void GLAPIENTRY
loopback_EvalCoord2fv( const GLfloat *u )
{
   EVALCOORD2( u[0], u[1] );
}

static void GLAPIENTRY
loopback_EvalCoord2d( GLdouble u, GLdouble v )
{
   EVALCOORD2( (GLfloat) u, (GLfloat) v );
}

static void GLAPIENTRY
loopback_EvalCoord1dv( const GLdouble *u )
{
   EVALCOORD1( (GLfloat) *u );
}

static void GLAPIENTRY
loopback_EvalCoord1fv( const GLfloat *u )
{
   EVALCOORD1( (GLfloat) *u );
}

static void GLAPIENTRY
loopback_EvalCoord1d( GLdouble u )
{
   EVALCOORD1( (GLfloat) u );
}

static void GLAPIENTRY
loopback_Materialf( GLenum face, GLenum pname, GLfloat param )
{
   GLfloat fparam[4];
   fparam[0] = param;
   MATERIALFV( face, pname, fparam );
}

static void GLAPIENTRY
loopback_Materiali(GLenum face, GLenum pname, GLint param )
{
   GLfloat p = (GLfloat) param;
   MATERIALFV(face, pname, &p);
}

static void GLAPIENTRY
loopback_Materialiv(GLenum face, GLenum pname, const GLint *params )
{
   GLfloat fparam[4];
   switch (pname) {
   case GL_AMBIENT:
   case GL_DIFFUSE:
   case GL_SPECULAR:
   case GL_EMISSION:
   case GL_AMBIENT_AND_DIFFUSE:
      fparam[0] = INT_TO_FLOAT( params[0] );
      fparam[1] = INT_TO_FLOAT( params[1] );
      fparam[2] = INT_TO_FLOAT( params[2] );
      fparam[3] = INT_TO_FLOAT( params[3] );
      break;
   case GL_SHININESS:
      fparam[0] = (GLfloat) params[0];
      break;
   case GL_COLOR_INDEXES:
      fparam[0] = (GLfloat) params[0];
      fparam[1] = (GLfloat) params[1];
      fparam[2] = (GLfloat) params[2];
      break;
   default:
      ;
   }
   MATERIALFV(face, pname, fparam);
}


static void GLAPIENTRY
loopback_Rectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
   RECTF((GLfloat) x1, (GLfloat) y1, (GLfloat) x2, (GLfloat) y2);
}

static void GLAPIENTRY
loopback_Rectdv(const GLdouble *v1, const GLdouble *v2)
{
   RECTF((GLfloat) v1[0], (GLfloat) v1[1], (GLfloat) v2[0], (GLfloat) v2[1]);
}

static void GLAPIENTRY
loopback_Rectfv(const GLfloat *v1, const GLfloat *v2)
{
   RECTF(v1[0], v1[1], v2[0], v2[1]);
}

static void GLAPIENTRY
loopback_Recti(GLint x1, GLint y1, GLint x2, GLint y2)
{
   RECTF((GLfloat) x1, (GLfloat) y1, (GLfloat) x2, (GLfloat) y2);
}

static void GLAPIENTRY
loopback_Rectiv(const GLint *v1, const GLint *v2)
{
   RECTF((GLfloat) v1[0], (GLfloat) v1[1], (GLfloat) v2[0], (GLfloat) v2[1]);
}

static void GLAPIENTRY
loopback_Rects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
   RECTF((GLfloat) x1, (GLfloat) y1, (GLfloat) x2, (GLfloat) y2);
}

static void GLAPIENTRY
loopback_Rectsv(const GLshort *v1, const GLshort *v2)
{
   RECTF((GLfloat) v1[0], (GLfloat) v1[1], (GLfloat) v2[0], (GLfloat) v2[1]);
}

static void GLAPIENTRY
loopback_SecondaryColor3bEXT_f( GLbyte red, GLbyte green, GLbyte blue )
{
   SECONDARYCOLORF( BYTE_TO_FLOAT(red),
		    BYTE_TO_FLOAT(green),
		    BYTE_TO_FLOAT(blue) );
}

static void GLAPIENTRY
loopback_SecondaryColor3dEXT_f( GLdouble red, GLdouble green, GLdouble blue )
{
   SECONDARYCOLORF( (GLfloat) red, (GLfloat) green, (GLfloat) blue );
}

static void GLAPIENTRY
loopback_SecondaryColor3iEXT_f( GLint red, GLint green, GLint blue )
{
   SECONDARYCOLORF( INT_TO_FLOAT(red),
		    INT_TO_FLOAT(green),
		    INT_TO_FLOAT(blue));
}

static void GLAPIENTRY
loopback_SecondaryColor3sEXT_f( GLshort red, GLshort green, GLshort blue )
{
   SECONDARYCOLORF(SHORT_TO_FLOAT(red),
                   SHORT_TO_FLOAT(green),
                   SHORT_TO_FLOAT(blue));
}

static void GLAPIENTRY
loopback_SecondaryColor3uiEXT_f( GLuint red, GLuint green, GLuint blue )
{
   SECONDARYCOLORF(UINT_TO_FLOAT(red),
                   UINT_TO_FLOAT(green),
                   UINT_TO_FLOAT(blue));
}

static void GLAPIENTRY
loopback_SecondaryColor3usEXT_f( GLushort red, GLushort green, GLushort blue )
{
   SECONDARYCOLORF(USHORT_TO_FLOAT(red),
                   USHORT_TO_FLOAT(green),
                   USHORT_TO_FLOAT(blue));
}

static void GLAPIENTRY
loopback_SecondaryColor3ubEXT_f( GLubyte red, GLubyte green, GLubyte blue )
{
   SECONDARYCOLORF(UBYTE_TO_FLOAT(red),
                   UBYTE_TO_FLOAT(green),
                   UBYTE_TO_FLOAT(blue));
}

static void GLAPIENTRY
loopback_SecondaryColor3bvEXT_f( const GLbyte *v )
{
   SECONDARYCOLORF(BYTE_TO_FLOAT(v[0]),
                   BYTE_TO_FLOAT(v[1]),
                   BYTE_TO_FLOAT(v[2]));
}

static void GLAPIENTRY
loopback_SecondaryColor3dvEXT_f( const GLdouble *v )
{
   SECONDARYCOLORF( (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}
static void GLAPIENTRY
loopback_SecondaryColor3ivEXT_f( const GLint *v )
{
   SECONDARYCOLORF(INT_TO_FLOAT(v[0]),
                   INT_TO_FLOAT(v[1]),
                   INT_TO_FLOAT(v[2]));
}

static void GLAPIENTRY
loopback_SecondaryColor3svEXT_f( const GLshort *v )
{
   SECONDARYCOLORF(SHORT_TO_FLOAT(v[0]),
                   SHORT_TO_FLOAT(v[1]),
                   SHORT_TO_FLOAT(v[2]));
}

static void GLAPIENTRY
loopback_SecondaryColor3uivEXT_f( const GLuint *v )
{
   SECONDARYCOLORF(UINT_TO_FLOAT(v[0]),
                   UINT_TO_FLOAT(v[1]),
                   UINT_TO_FLOAT(v[2]));
}

static void GLAPIENTRY
loopback_SecondaryColor3usvEXT_f( const GLushort *v )
{
   SECONDARYCOLORF(USHORT_TO_FLOAT(v[0]),
                   USHORT_TO_FLOAT(v[1]),
                   USHORT_TO_FLOAT(v[2]));
}

static void GLAPIENTRY
loopback_SecondaryColor3ubvEXT_f( const GLubyte *v )
{
   SECONDARYCOLORF(UBYTE_TO_FLOAT(v[0]),
                   UBYTE_TO_FLOAT(v[1]),
                   UBYTE_TO_FLOAT(v[2]));
}


/*
 * GL_NV_vertex_program
 */

static void GLAPIENTRY
loopback_VertexAttrib1sNV(GLuint index, GLshort x)
{
   ATTRIB1(index, (GLfloat) x);
}

static void GLAPIENTRY
loopback_VertexAttrib1fNV(GLuint index, GLfloat x)
{
   ATTRIB1(index, x);
}

static void GLAPIENTRY
loopback_VertexAttrib1dNV(GLuint index, GLdouble x)
{
   ATTRIB1(index, (GLfloat) x);
}

static void GLAPIENTRY
loopback_VertexAttrib2sNV(GLuint index, GLshort x, GLshort y)
{
   ATTRIB2(index, (GLfloat) x, y);
}

static void GLAPIENTRY
loopback_VertexAttrib2fNV(GLuint index, GLfloat x, GLfloat y)
{
   ATTRIB2(index, (GLfloat) x, y);
}

static void GLAPIENTRY
loopback_VertexAttrib2dNV(GLuint index, GLdouble x, GLdouble y)
{
   ATTRIB2(index, (GLfloat) x, (GLfloat) y);
}

static void GLAPIENTRY
loopback_VertexAttrib3sNV(GLuint index, GLshort x, GLshort y, GLshort z)
{
   ATTRIB3(index, (GLfloat) x, y, z);
}

static void GLAPIENTRY
loopback_VertexAttrib3fNV(GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
   ATTRIB3(index, x, y, z);
}

static void GLAPIENTRY
loopback_VertexAttrib3dNV(GLuint index, GLdouble x, GLdouble y, GLdouble z)
{
   ATTRIB4(index, (GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

static void GLAPIENTRY
loopback_VertexAttrib4sNV(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w)
{
   ATTRIB4(index, (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

static void GLAPIENTRY
loopback_VertexAttrib4dNV(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   ATTRIB4(index, (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

static void GLAPIENTRY
loopback_VertexAttrib4ubNV(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w)
{
   ATTRIB4(index, UBYTE_TO_FLOAT(x), UBYTE_TO_FLOAT(y),
	UBYTE_TO_FLOAT(z), UBYTE_TO_FLOAT(w));
}

static void GLAPIENTRY
loopback_VertexAttrib1svNV(GLuint index, const GLshort *v)
{
   ATTRIB1(index, (GLfloat) v[0]);
}

static void GLAPIENTRY
loopback_VertexAttrib1fvNV(GLuint index, const GLfloat *v)
{
   ATTRIB1(index, v[0]);
}

static void GLAPIENTRY
loopback_VertexAttrib1dvNV(GLuint index, const GLdouble *v)
{
   ATTRIB1(index, (GLfloat) v[0]);
}

static void GLAPIENTRY
loopback_VertexAttrib2svNV(GLuint index, const GLshort *v)
{
   ATTRIB2(index, (GLfloat) v[0], (GLfloat) v[1]);
}

static void GLAPIENTRY
loopback_VertexAttrib2fvNV(GLuint index, const GLfloat *v)
{
   ATTRIB2(index, v[0], v[1]);
}

static void GLAPIENTRY
loopback_VertexAttrib2dvNV(GLuint index, const GLdouble *v)
{
   ATTRIB2(index, (GLfloat) v[0], (GLfloat) v[1]);
}

static void GLAPIENTRY
loopback_VertexAttrib3svNV(GLuint index, const GLshort *v)
{
   ATTRIB2(index, (GLfloat) v[0], (GLfloat) v[1]);
}

static void GLAPIENTRY
loopback_VertexAttrib3fvNV(GLuint index, const GLfloat *v)
{
   ATTRIB3(index, v[0], v[1], v[2]);
}

static void GLAPIENTRY
loopback_VertexAttrib3dvNV(GLuint index, const GLdouble *v)
{
   ATTRIB3(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

static void GLAPIENTRY
loopback_VertexAttrib4svNV(GLuint index, const GLshort *v)
{
   ATTRIB4(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 
	  (GLfloat)v[3]);
}

static void GLAPIENTRY
loopback_VertexAttrib4fvNV(GLuint index, const GLfloat *v)
{
   ATTRIB4(index, v[0], v[1], v[2], v[3]);
}

static void GLAPIENTRY
loopback_VertexAttrib4dvNV(GLuint index, const GLdouble *v)
{
   ATTRIB4(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY
loopback_VertexAttrib4ubvNV(GLuint index, const GLubyte *v)
{
   ATTRIB4(index, UBYTE_TO_FLOAT(v[0]), UBYTE_TO_FLOAT(v[1]),
          UBYTE_TO_FLOAT(v[2]), UBYTE_TO_FLOAT(v[3]));
}


static void GLAPIENTRY
loopback_VertexAttribs1svNV(GLuint index, GLsizei n, const GLshort *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      loopback_VertexAttrib1svNV(index + i, v + i);
}

static void GLAPIENTRY
loopback_VertexAttribs1fvNV(GLuint index, GLsizei n, const GLfloat *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      loopback_VertexAttrib1fvNV(index + i, v + i);
}

static void GLAPIENTRY
loopback_VertexAttribs1dvNV(GLuint index, GLsizei n, const GLdouble *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      loopback_VertexAttrib1dvNV(index + i, v + i);
}

static void GLAPIENTRY
loopback_VertexAttribs2svNV(GLuint index, GLsizei n, const GLshort *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      loopback_VertexAttrib2svNV(index + i, v + 2 * i);
}

static void GLAPIENTRY
loopback_VertexAttribs2fvNV(GLuint index, GLsizei n, const GLfloat *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      loopback_VertexAttrib2fvNV(index + i, v + 2 * i);
}

static void GLAPIENTRY
loopback_VertexAttribs2dvNV(GLuint index, GLsizei n, const GLdouble *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      loopback_VertexAttrib2dvNV(index + i, v + 2 * i);
}

static void GLAPIENTRY
loopback_VertexAttribs3svNV(GLuint index, GLsizei n, const GLshort *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      loopback_VertexAttrib3svNV(index + i, v + 3 * i);
}

static void GLAPIENTRY
loopback_VertexAttribs3fvNV(GLuint index, GLsizei n, const GLfloat *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      loopback_VertexAttrib3fvNV(index + i, v + 3 * i);
}

static void GLAPIENTRY
loopback_VertexAttribs3dvNV(GLuint index, GLsizei n, const GLdouble *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      loopback_VertexAttrib3dvNV(index + i, v + 3 * i);
}

static void GLAPIENTRY
loopback_VertexAttribs4svNV(GLuint index, GLsizei n, const GLshort *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      loopback_VertexAttrib4svNV(index + i, v + 4 * i);
}

static void GLAPIENTRY
loopback_VertexAttribs4fvNV(GLuint index, GLsizei n, const GLfloat *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      loopback_VertexAttrib4fvNV(index + i, v + 4 * i);
}

static void GLAPIENTRY
loopback_VertexAttribs4dvNV(GLuint index, GLsizei n, const GLdouble *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      loopback_VertexAttrib4dvNV(index + i, v + 4 * i);
}

static void GLAPIENTRY
loopback_VertexAttribs4ubvNV(GLuint index, GLsizei n, const GLubyte *v)
{
   GLint i;
   for (i = n - 1; i >= 0; i--)
      loopback_VertexAttrib4ubvNV(index + i, v + 4 * i);
}


/*
 * GL_ARB_vertex_program
 */

static void GLAPIENTRY
loopback_VertexAttrib4bvARB(GLuint index, const GLbyte * v)
{
   ATTRIB4(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY
loopback_VertexAttrib4ivARB(GLuint index, const GLint * v)
{
   ATTRIB4(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY
loopback_VertexAttrib4ubvARB(GLuint index, const GLubyte * v)
{
   ATTRIB4(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY
loopback_VertexAttrib4usvARB(GLuint index, const GLushort * v)
{
   ATTRIB4(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY
loopback_VertexAttrib4uivARB(GLuint index, const GLuint * v)
{
   ATTRIB4(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY
loopback_VertexAttrib4NbvARB(GLuint index, const GLbyte * v)
{
   ATTRIB4(index, BYTE_TO_FLOAT(v[0]), BYTE_TO_FLOAT(v[1]),
          BYTE_TO_FLOAT(v[2]), BYTE_TO_FLOAT(v[3]));
}

static void GLAPIENTRY
loopback_VertexAttrib4NsvARB(GLuint index, const GLshort * v)
{
   ATTRIB4(index, SHORT_TO_FLOAT(v[0]), SHORT_TO_FLOAT(v[1]),
          SHORT_TO_FLOAT(v[2]), SHORT_TO_FLOAT(v[3]));
}

static void GLAPIENTRY
loopback_VertexAttrib4NivARB(GLuint index, const GLint * v)
{
   ATTRIB4(index, INT_TO_FLOAT(v[0]), INT_TO_FLOAT(v[1]),
          INT_TO_FLOAT(v[2]), INT_TO_FLOAT(v[3]));
}

static void GLAPIENTRY
loopback_VertexAttrib4NusvARB(GLuint index, const GLushort * v)
{
   ATTRIB4(index, USHORT_TO_FLOAT(v[0]), USHORT_TO_FLOAT(v[1]),
          USHORT_TO_FLOAT(v[2]), USHORT_TO_FLOAT(v[3]));
}

static void GLAPIENTRY
loopback_VertexAttrib4NuivARB(GLuint index, const GLuint * v)
{
   ATTRIB4(index, UINT_TO_FLOAT(v[0]), UINT_TO_FLOAT(v[1]),
          UINT_TO_FLOAT(v[2]), UINT_TO_FLOAT(v[3]));
}




/*
 * This code never registers handlers for any of the entry points
 * listed in vtxfmt.h.
 */
void
_mesa_loopback_init_api_table( struct _glapi_table *dest )
{
   dest->Color3b = loopback_Color3b_f;
   dest->Color3d = loopback_Color3d_f;
   dest->Color3i = loopback_Color3i_f;
   dest->Color3s = loopback_Color3s_f;
   dest->Color3ui = loopback_Color3ui_f;
   dest->Color3us = loopback_Color3us_f;
   dest->Color3ub = loopback_Color3ub_f;
   dest->Color4b = loopback_Color4b_f;
   dest->Color4d = loopback_Color4d_f;
   dest->Color4i = loopback_Color4i_f;
   dest->Color4s = loopback_Color4s_f;
   dest->Color4ui = loopback_Color4ui_f;
   dest->Color4us = loopback_Color4us_f;
   dest->Color4ub = loopback_Color4ub_f;
   dest->Color3bv = loopback_Color3bv_f;
   dest->Color3dv = loopback_Color3dv_f;
   dest->Color3iv = loopback_Color3iv_f;
   dest->Color3sv = loopback_Color3sv_f;
   dest->Color3uiv = loopback_Color3uiv_f;
   dest->Color3usv = loopback_Color3usv_f;
   dest->Color3ubv = loopback_Color3ubv_f;
   dest->Color4bv = loopback_Color4bv_f;
   dest->Color4dv = loopback_Color4dv_f;
   dest->Color4iv = loopback_Color4iv_f;
   dest->Color4sv = loopback_Color4sv_f;
   dest->Color4uiv = loopback_Color4uiv_f;
   dest->Color4usv = loopback_Color4usv_f;
   dest->Color4ubv = loopback_Color4ubv_f;

   dest->SecondaryColor3bEXT = loopback_SecondaryColor3bEXT_f;
   dest->SecondaryColor3dEXT = loopback_SecondaryColor3dEXT_f;
   dest->SecondaryColor3iEXT = loopback_SecondaryColor3iEXT_f;
   dest->SecondaryColor3sEXT = loopback_SecondaryColor3sEXT_f;
   dest->SecondaryColor3uiEXT = loopback_SecondaryColor3uiEXT_f;
   dest->SecondaryColor3usEXT = loopback_SecondaryColor3usEXT_f;
   dest->SecondaryColor3ubEXT = loopback_SecondaryColor3ubEXT_f;
   dest->SecondaryColor3bvEXT = loopback_SecondaryColor3bvEXT_f;
   dest->SecondaryColor3dvEXT = loopback_SecondaryColor3dvEXT_f;
   dest->SecondaryColor3ivEXT = loopback_SecondaryColor3ivEXT_f;
   dest->SecondaryColor3svEXT = loopback_SecondaryColor3svEXT_f;
   dest->SecondaryColor3uivEXT = loopback_SecondaryColor3uivEXT_f;
   dest->SecondaryColor3usvEXT = loopback_SecondaryColor3usvEXT_f;
   dest->SecondaryColor3ubvEXT = loopback_SecondaryColor3ubvEXT_f;
      
   dest->Indexd = loopback_Indexd;
   dest->Indexi = loopback_Indexi;
   dest->Indexs = loopback_Indexs;
   dest->Indexub = loopback_Indexub;
   dest->Indexdv = loopback_Indexdv;
   dest->Indexiv = loopback_Indexiv;
   dest->Indexsv = loopback_Indexsv;
   dest->Indexubv = loopback_Indexubv;
   dest->Normal3b = loopback_Normal3b;
   dest->Normal3d = loopback_Normal3d;
   dest->Normal3i = loopback_Normal3i;
   dest->Normal3s = loopback_Normal3s;
   dest->Normal3bv = loopback_Normal3bv;
   dest->Normal3dv = loopback_Normal3dv;
   dest->Normal3iv = loopback_Normal3iv;
   dest->Normal3sv = loopback_Normal3sv;
   dest->TexCoord1d = loopback_TexCoord1d;
   dest->TexCoord1i = loopback_TexCoord1i;
   dest->TexCoord1s = loopback_TexCoord1s;
   dest->TexCoord2d = loopback_TexCoord2d;
   dest->TexCoord2s = loopback_TexCoord2s;
   dest->TexCoord2i = loopback_TexCoord2i;
   dest->TexCoord3d = loopback_TexCoord3d;
   dest->TexCoord3i = loopback_TexCoord3i;
   dest->TexCoord3s = loopback_TexCoord3s;
   dest->TexCoord4d = loopback_TexCoord4d;
   dest->TexCoord4i = loopback_TexCoord4i;
   dest->TexCoord4s = loopback_TexCoord4s;
   dest->TexCoord1dv = loopback_TexCoord1dv;
   dest->TexCoord1iv = loopback_TexCoord1iv;
   dest->TexCoord1sv = loopback_TexCoord1sv;
   dest->TexCoord2dv = loopback_TexCoord2dv;
   dest->TexCoord2iv = loopback_TexCoord2iv;
   dest->TexCoord2sv = loopback_TexCoord2sv;
   dest->TexCoord3dv = loopback_TexCoord3dv;
   dest->TexCoord3iv = loopback_TexCoord3iv;
   dest->TexCoord3sv = loopback_TexCoord3sv;
   dest->TexCoord4dv = loopback_TexCoord4dv;
   dest->TexCoord4iv = loopback_TexCoord4iv;
   dest->TexCoord4sv = loopback_TexCoord4sv;
   dest->Vertex2d = loopback_Vertex2d;
   dest->Vertex2i = loopback_Vertex2i;
   dest->Vertex2s = loopback_Vertex2s;
   dest->Vertex3d = loopback_Vertex3d;
   dest->Vertex3i = loopback_Vertex3i;
   dest->Vertex3s = loopback_Vertex3s;
   dest->Vertex4d = loopback_Vertex4d;
   dest->Vertex4i = loopback_Vertex4i;
   dest->Vertex4s = loopback_Vertex4s;
   dest->Vertex2dv = loopback_Vertex2dv;
   dest->Vertex2iv = loopback_Vertex2iv;
   dest->Vertex2sv = loopback_Vertex2sv;
   dest->Vertex3dv = loopback_Vertex3dv;
   dest->Vertex3iv = loopback_Vertex3iv;
   dest->Vertex3sv = loopback_Vertex3sv;
   dest->Vertex4dv = loopback_Vertex4dv;
   dest->Vertex4iv = loopback_Vertex4iv;
   dest->Vertex4sv = loopback_Vertex4sv;
   dest->MultiTexCoord1dARB = loopback_MultiTexCoord1dARB;
   dest->MultiTexCoord1dvARB = loopback_MultiTexCoord1dvARB;
   dest->MultiTexCoord1iARB = loopback_MultiTexCoord1iARB;
   dest->MultiTexCoord1ivARB = loopback_MultiTexCoord1ivARB;
   dest->MultiTexCoord1sARB = loopback_MultiTexCoord1sARB;
   dest->MultiTexCoord1svARB = loopback_MultiTexCoord1svARB;
   dest->MultiTexCoord2dARB = loopback_MultiTexCoord2dARB;
   dest->MultiTexCoord2dvARB = loopback_MultiTexCoord2dvARB;
   dest->MultiTexCoord2iARB = loopback_MultiTexCoord2iARB;
   dest->MultiTexCoord2ivARB = loopback_MultiTexCoord2ivARB;
   dest->MultiTexCoord2sARB = loopback_MultiTexCoord2sARB;
   dest->MultiTexCoord2svARB = loopback_MultiTexCoord2svARB;
   dest->MultiTexCoord3dARB = loopback_MultiTexCoord3dARB;
   dest->MultiTexCoord3dvARB = loopback_MultiTexCoord3dvARB;
   dest->MultiTexCoord3iARB = loopback_MultiTexCoord3iARB;
   dest->MultiTexCoord3ivARB = loopback_MultiTexCoord3ivARB;
   dest->MultiTexCoord3sARB = loopback_MultiTexCoord3sARB;
   dest->MultiTexCoord3svARB = loopback_MultiTexCoord3svARB;
   dest->MultiTexCoord4dARB = loopback_MultiTexCoord4dARB;
   dest->MultiTexCoord4dvARB = loopback_MultiTexCoord4dvARB;
   dest->MultiTexCoord4iARB = loopback_MultiTexCoord4iARB;
   dest->MultiTexCoord4ivARB = loopback_MultiTexCoord4ivARB;
   dest->MultiTexCoord4sARB = loopback_MultiTexCoord4sARB;
   dest->MultiTexCoord4svARB = loopback_MultiTexCoord4svARB;
   dest->EvalCoord2dv = loopback_EvalCoord2dv;
   dest->EvalCoord2fv = loopback_EvalCoord2fv;
   dest->EvalCoord2d = loopback_EvalCoord2d;
   dest->EvalCoord1dv = loopback_EvalCoord1dv;
   dest->EvalCoord1fv = loopback_EvalCoord1fv;
   dest->EvalCoord1d = loopback_EvalCoord1d;
   dest->Materialf = loopback_Materialf;
   dest->Materiali = loopback_Materiali;
   dest->Materialiv = loopback_Materialiv;
   dest->Rectd = loopback_Rectd;
   dest->Rectdv = loopback_Rectdv;
   dest->Rectfv = loopback_Rectfv;
   dest->Recti = loopback_Recti;
   dest->Rectiv = loopback_Rectiv;
   dest->Rects = loopback_Rects;
   dest->Rectsv = loopback_Rectsv;
   dest->FogCoorddEXT = loopback_FogCoorddEXT;
   dest->FogCoorddvEXT = loopback_FogCoorddvEXT;

   dest->VertexAttrib1sNV = loopback_VertexAttrib1sNV;
   dest->VertexAttrib1fNV = loopback_VertexAttrib1fNV;
   dest->VertexAttrib1dNV = loopback_VertexAttrib1dNV;
   dest->VertexAttrib2sNV = loopback_VertexAttrib2sNV;
   dest->VertexAttrib2fNV = loopback_VertexAttrib2fNV;
   dest->VertexAttrib2dNV = loopback_VertexAttrib2dNV;
   dest->VertexAttrib3sNV = loopback_VertexAttrib3sNV;
   dest->VertexAttrib3fNV = loopback_VertexAttrib3fNV;
   dest->VertexAttrib3dNV = loopback_VertexAttrib3dNV;
   dest->VertexAttrib4sNV = loopback_VertexAttrib4sNV;
   dest->VertexAttrib4dNV = loopback_VertexAttrib4dNV;
   dest->VertexAttrib4ubNV = loopback_VertexAttrib4ubNV;

   dest->VertexAttrib1svNV = loopback_VertexAttrib1svNV;
   dest->VertexAttrib1fvNV = loopback_VertexAttrib1fvNV;
   dest->VertexAttrib1dvNV = loopback_VertexAttrib1dvNV;
   dest->VertexAttrib2svNV = loopback_VertexAttrib2svNV;
   dest->VertexAttrib2fvNV = loopback_VertexAttrib2fvNV;
   dest->VertexAttrib2dvNV = loopback_VertexAttrib2dvNV;
   dest->VertexAttrib3svNV = loopback_VertexAttrib3svNV;
   dest->VertexAttrib3fvNV = loopback_VertexAttrib3fvNV;
   dest->VertexAttrib3dvNV = loopback_VertexAttrib3dvNV;
   dest->VertexAttrib4svNV = loopback_VertexAttrib4svNV;
   dest->VertexAttrib4fvNV = loopback_VertexAttrib4fvNV;
   dest->VertexAttrib4dvNV = loopback_VertexAttrib4dvNV;
   dest->VertexAttrib4ubvNV = loopback_VertexAttrib4ubvNV;

   dest->VertexAttribs1svNV = loopback_VertexAttribs1svNV;
   dest->VertexAttribs1fvNV = loopback_VertexAttribs1fvNV;
   dest->VertexAttribs1dvNV = loopback_VertexAttribs1dvNV;
   dest->VertexAttribs2svNV = loopback_VertexAttribs2svNV;
   dest->VertexAttribs2fvNV = loopback_VertexAttribs2fvNV;
   dest->VertexAttribs2dvNV = loopback_VertexAttribs2dvNV;
   dest->VertexAttribs3svNV = loopback_VertexAttribs3svNV;
   dest->VertexAttribs3fvNV = loopback_VertexAttribs3fvNV;
   dest->VertexAttribs3dvNV = loopback_VertexAttribs3dvNV;
   dest->VertexAttribs4svNV = loopback_VertexAttribs4svNV;
   dest->VertexAttribs4fvNV = loopback_VertexAttribs4fvNV;
   dest->VertexAttribs4dvNV = loopback_VertexAttribs4dvNV;
   dest->VertexAttribs4ubvNV = loopback_VertexAttribs4ubvNV;

   dest->VertexAttrib4bvARB = loopback_VertexAttrib4bvARB;
   dest->VertexAttrib4ivARB = loopback_VertexAttrib4ivARB;
   dest->VertexAttrib4ubvARB = loopback_VertexAttrib4ubvARB;
   dest->VertexAttrib4usvARB = loopback_VertexAttrib4usvARB;
   dest->VertexAttrib4uivARB = loopback_VertexAttrib4uivARB;
   dest->VertexAttrib4NbvARB = loopback_VertexAttrib4NbvARB;
   dest->VertexAttrib4NsvARB = loopback_VertexAttrib4NsvARB;
   dest->VertexAttrib4NivARB = loopback_VertexAttrib4NivARB;
   dest->VertexAttrib4NusvARB = loopback_VertexAttrib4NusvARB;
   dest->VertexAttrib4NuivARB = loopback_VertexAttrib4NuivARB;
}
