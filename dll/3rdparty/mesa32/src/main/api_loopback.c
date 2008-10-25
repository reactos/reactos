/**
 * \file api_loopback.c
 *
 * \author Keith Whitwell <keith@tungstengraphics.com>
 */

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
 */


#include "glheader.h"
#include "macros.h"
#include "colormac.h"
#include "api_loopback.h"
#include "mtypes.h"
#include "glapi/glapi.h"
#include "glapi/glapitable.h"
#include "glapi/glthread.h"
#include "glapi/dispatch.h"

/* KW: A set of functions to convert unusual Color/Normal/Vertex/etc
 * calls to a smaller set of driver-provided formats.  Currently just
 * go back to dispatch to find these (eg. call glNormal3f directly),
 * hence 'loopback'.
 *
 * The driver must supply all of the remaining entry points, which are
 * listed in dd.h.  The easiest way for a driver to do this is to
 * install the supplied software t&l module.
 */
#define COLORF(r,g,b,a)             CALL_Color4f(GET_DISPATCH(), (r,g,b,a))
#define VERTEX2(x,y)	            CALL_Vertex2f(GET_DISPATCH(), (x,y))
#define VERTEX3(x,y,z)	            CALL_Vertex3f(GET_DISPATCH(), (x,y,z))
#define VERTEX4(x,y,z,w)            CALL_Vertex4f(GET_DISPATCH(), (x,y,z,w))
#define NORMAL(x,y,z)               CALL_Normal3f(GET_DISPATCH(), (x,y,z))
#define TEXCOORD1(s)                CALL_TexCoord1f(GET_DISPATCH(), (s))
#define TEXCOORD2(s,t)              CALL_TexCoord2f(GET_DISPATCH(), (s,t))
#define TEXCOORD3(s,t,u)            CALL_TexCoord3f(GET_DISPATCH(), (s,t,u))
#define TEXCOORD4(s,t,u,v)          CALL_TexCoord4f(GET_DISPATCH(), (s,t,u,v))
#define INDEX(c)		    CALL_Indexf(GET_DISPATCH(), (c))
#define MULTI_TEXCOORD1(z,s)	    CALL_MultiTexCoord1fARB(GET_DISPATCH(), (z,s))
#define MULTI_TEXCOORD2(z,s,t)	    CALL_MultiTexCoord2fARB(GET_DISPATCH(), (z,s,t))
#define MULTI_TEXCOORD3(z,s,t,u)    CALL_MultiTexCoord3fARB(GET_DISPATCH(), (z,s,t,u))
#define MULTI_TEXCOORD4(z,s,t,u,v)  CALL_MultiTexCoord4fARB(GET_DISPATCH(), (z,s,t,u,v))
#define EVALCOORD1(x)               CALL_EvalCoord1f(GET_DISPATCH(), (x))
#define EVALCOORD2(x,y)             CALL_EvalCoord2f(GET_DISPATCH(), (x,y))
#define MATERIALFV(a,b,c)           CALL_Materialfv(GET_DISPATCH(), (a,b,c))
#define RECTF(a,b,c,d)              CALL_Rectf(GET_DISPATCH(), (a,b,c,d))

#define ATTRIB1NV(index,x)          CALL_VertexAttrib1fNV(GET_DISPATCH(), (index,x))
#define ATTRIB2NV(index,x,y)        CALL_VertexAttrib2fNV(GET_DISPATCH(), (index,x,y))
#define ATTRIB3NV(index,x,y,z)      CALL_VertexAttrib3fNV(GET_DISPATCH(), (index,x,y,z))
#define ATTRIB4NV(index,x,y,z,w)    CALL_VertexAttrib4fNV(GET_DISPATCH(), (index,x,y,z,w))
#define ATTRIB1ARB(index,x)         CALL_VertexAttrib1fARB(GET_DISPATCH(), (index,x))
#define ATTRIB2ARB(index,x,y)       CALL_VertexAttrib2fARB(GET_DISPATCH(), (index,x,y))
#define ATTRIB3ARB(index,x,y,z)     CALL_VertexAttrib3fARB(GET_DISPATCH(), (index,x,y,z))
#define ATTRIB4ARB(index,x,y,z,w)   CALL_VertexAttrib4fARB(GET_DISPATCH(), (index,x,y,z,w))
#define FOGCOORDF(x)                CALL_FogCoordfEXT(GET_DISPATCH(), (x))
#define SECONDARYCOLORF(a,b,c)      CALL_SecondaryColor3fEXT(GET_DISPATCH(), (a,b,c))

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
	   INT_TO_FLOAT(v[2]), 1.0 );
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
loopback_EdgeFlagv(const GLboolean *flag)
{
   CALL_EdgeFlag(GET_DISPATCH(), (*flag));
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
   TEXCOORD3((GLfloat) v[0],(GLfloat) v[1],(GLfloat) v[2]);
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
 * GL_NV_vertex_program:
 * Always loop-back to one of the VertexAttrib[1234]f[v]NV functions.
 */

static void GLAPIENTRY
loopback_VertexAttrib1sNV(GLuint index, GLshort x)
{
   ATTRIB1NV(index, (GLfloat) x);
}

static void GLAPIENTRY
loopback_VertexAttrib1dNV(GLuint index, GLdouble x)
{
   ATTRIB1NV(index, (GLfloat) x);
}

static void GLAPIENTRY
loopback_VertexAttrib2sNV(GLuint index, GLshort x, GLshort y)
{
   ATTRIB2NV(index, (GLfloat) x, y);
}

static void GLAPIENTRY
loopback_VertexAttrib2dNV(GLuint index, GLdouble x, GLdouble y)
{
   ATTRIB2NV(index, (GLfloat) x, (GLfloat) y);
}

static void GLAPIENTRY
loopback_VertexAttrib3sNV(GLuint index, GLshort x, GLshort y, GLshort z)
{
   ATTRIB3NV(index, (GLfloat) x, (GLfloat) y, (GLfloat) z);
}

static void GLAPIENTRY
loopback_VertexAttrib3dNV(GLuint index, GLdouble x, GLdouble y, GLdouble z)
{
   ATTRIB4NV(index, (GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

static void GLAPIENTRY
loopback_VertexAttrib4sNV(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w)
{
   ATTRIB4NV(index, (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

static void GLAPIENTRY
loopback_VertexAttrib4dNV(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   ATTRIB4NV(index, (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

static void GLAPIENTRY
loopback_VertexAttrib4ubNV(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w)
{
   ATTRIB4NV(index, UBYTE_TO_FLOAT(x), UBYTE_TO_FLOAT(y),
	UBYTE_TO_FLOAT(z), UBYTE_TO_FLOAT(w));
}

static void GLAPIENTRY
loopback_VertexAttrib1svNV(GLuint index, const GLshort *v)
{
   ATTRIB1NV(index, (GLfloat) v[0]);
}

static void GLAPIENTRY
loopback_VertexAttrib1dvNV(GLuint index, const GLdouble *v)
{
   ATTRIB1NV(index, (GLfloat) v[0]);
}

static void GLAPIENTRY
loopback_VertexAttrib2svNV(GLuint index, const GLshort *v)
{
   ATTRIB2NV(index, (GLfloat) v[0], (GLfloat) v[1]);
}

static void GLAPIENTRY
loopback_VertexAttrib2dvNV(GLuint index, const GLdouble *v)
{
   ATTRIB2NV(index, (GLfloat) v[0], (GLfloat) v[1]);
}

static void GLAPIENTRY
loopback_VertexAttrib3svNV(GLuint index, const GLshort *v)
{
   ATTRIB3NV(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

static void GLAPIENTRY
loopback_VertexAttrib3dvNV(GLuint index, const GLdouble *v)
{
   ATTRIB3NV(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

static void GLAPIENTRY
loopback_VertexAttrib4svNV(GLuint index, const GLshort *v)
{
   ATTRIB4NV(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 
	  (GLfloat)v[3]);
}

static void GLAPIENTRY
loopback_VertexAttrib4dvNV(GLuint index, const GLdouble *v)
{
   ATTRIB4NV(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY
loopback_VertexAttrib4ubvNV(GLuint index, const GLubyte *v)
{
   ATTRIB4NV(index, UBYTE_TO_FLOAT(v[0]), UBYTE_TO_FLOAT(v[1]),
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
      ATTRIB1NV(index + i, v[i]);
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
      ATTRIB2NV(index + i, v[2 * i], v[2 * i + 1]);
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
      ATTRIB3NV(index + i, v[3 * i], v[3 * i + 1], v[3 * i + 2]);
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
      ATTRIB4NV(index + i, v[4 * i], v[4 * i + 1], v[4 * i + 2], v[4 * i + 3]);
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
 * Always loop-back to one of the VertexAttrib[1234]f[v]ARB functions.
 */

static void GLAPIENTRY
loopback_VertexAttrib1sARB(GLuint index, GLshort x)
{
   ATTRIB1ARB(index, (GLfloat) x);
}

static void GLAPIENTRY
loopback_VertexAttrib1dARB(GLuint index, GLdouble x)
{
   ATTRIB1ARB(index, (GLfloat) x);
}

static void GLAPIENTRY
loopback_VertexAttrib2sARB(GLuint index, GLshort x, GLshort y)
{
   ATTRIB2ARB(index, (GLfloat) x, y);
}

static void GLAPIENTRY
loopback_VertexAttrib2dARB(GLuint index, GLdouble x, GLdouble y)
{
   ATTRIB2ARB(index, (GLfloat) x, (GLfloat) y);
}

static void GLAPIENTRY
loopback_VertexAttrib3sARB(GLuint index, GLshort x, GLshort y, GLshort z)
{
   ATTRIB3ARB(index, (GLfloat) x, (GLfloat) y, (GLfloat) z);
}

static void GLAPIENTRY
loopback_VertexAttrib3dARB(GLuint index, GLdouble x, GLdouble y, GLdouble z)
{
   ATTRIB4ARB(index, (GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F);
}

static void GLAPIENTRY
loopback_VertexAttrib4sARB(GLuint index, GLshort x, GLshort y, GLshort z, GLshort w)
{
   ATTRIB4ARB(index, (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

static void GLAPIENTRY
loopback_VertexAttrib4dARB(GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   ATTRIB4ARB(index, (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w);
}

static void GLAPIENTRY
loopback_VertexAttrib1svARB(GLuint index, const GLshort *v)
{
   ATTRIB1ARB(index, (GLfloat) v[0]);
}

static void GLAPIENTRY
loopback_VertexAttrib1dvARB(GLuint index, const GLdouble *v)
{
   ATTRIB1ARB(index, (GLfloat) v[0]);
}

static void GLAPIENTRY
loopback_VertexAttrib2svARB(GLuint index, const GLshort *v)
{
   ATTRIB2ARB(index, (GLfloat) v[0], (GLfloat) v[1]);
}

static void GLAPIENTRY
loopback_VertexAttrib2dvARB(GLuint index, const GLdouble *v)
{
   ATTRIB2ARB(index, (GLfloat) v[0], (GLfloat) v[1]);
}

static void GLAPIENTRY
loopback_VertexAttrib3svARB(GLuint index, const GLshort *v)
{
   ATTRIB3ARB(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

static void GLAPIENTRY
loopback_VertexAttrib3dvARB(GLuint index, const GLdouble *v)
{
   ATTRIB3ARB(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2]);
}

static void GLAPIENTRY
loopback_VertexAttrib4svARB(GLuint index, const GLshort *v)
{
   ATTRIB4ARB(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], 
	  (GLfloat)v[3]);
}

static void GLAPIENTRY
loopback_VertexAttrib4dvARB(GLuint index, const GLdouble *v)
{
   ATTRIB4ARB(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY
loopback_VertexAttrib4bvARB(GLuint index, const GLbyte * v)
{
   ATTRIB4ARB(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY
loopback_VertexAttrib4ivARB(GLuint index, const GLint * v)
{
   ATTRIB4ARB(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY
loopback_VertexAttrib4ubvARB(GLuint index, const GLubyte * v)
{
   ATTRIB4ARB(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY
loopback_VertexAttrib4usvARB(GLuint index, const GLushort * v)
{
   ATTRIB4ARB(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY
loopback_VertexAttrib4uivARB(GLuint index, const GLuint * v)
{
   ATTRIB4ARB(index, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2], (GLfloat) v[3]);
}

static void GLAPIENTRY
loopback_VertexAttrib4NbvARB(GLuint index, const GLbyte * v)
{
   ATTRIB4ARB(index, BYTE_TO_FLOAT(v[0]), BYTE_TO_FLOAT(v[1]),
          BYTE_TO_FLOAT(v[2]), BYTE_TO_FLOAT(v[3]));
}

static void GLAPIENTRY
loopback_VertexAttrib4NsvARB(GLuint index, const GLshort * v)
{
   ATTRIB4ARB(index, SHORT_TO_FLOAT(v[0]), SHORT_TO_FLOAT(v[1]),
          SHORT_TO_FLOAT(v[2]), SHORT_TO_FLOAT(v[3]));
}

static void GLAPIENTRY
loopback_VertexAttrib4NivARB(GLuint index, const GLint * v)
{
   ATTRIB4ARB(index, INT_TO_FLOAT(v[0]), INT_TO_FLOAT(v[1]),
          INT_TO_FLOAT(v[2]), INT_TO_FLOAT(v[3]));
}

static void GLAPIENTRY
loopback_VertexAttrib4NubARB(GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w)
{
   ATTRIB4ARB(index, UBYTE_TO_FLOAT(x), UBYTE_TO_FLOAT(y),
              UBYTE_TO_FLOAT(z), UBYTE_TO_FLOAT(w));
}

static void GLAPIENTRY
loopback_VertexAttrib4NubvARB(GLuint index, const GLubyte * v)
{
   ATTRIB4ARB(index, UBYTE_TO_FLOAT(v[0]), UBYTE_TO_FLOAT(v[1]),
          UBYTE_TO_FLOAT(v[2]), UBYTE_TO_FLOAT(v[3]));
}

static void GLAPIENTRY
loopback_VertexAttrib4NusvARB(GLuint index, const GLushort * v)
{
   ATTRIB4ARB(index, USHORT_TO_FLOAT(v[0]), USHORT_TO_FLOAT(v[1]),
          USHORT_TO_FLOAT(v[2]), USHORT_TO_FLOAT(v[3]));
}

static void GLAPIENTRY
loopback_VertexAttrib4NuivARB(GLuint index, const GLuint * v)
{
   ATTRIB4ARB(index, UINT_TO_FLOAT(v[0]), UINT_TO_FLOAT(v[1]),
          UINT_TO_FLOAT(v[2]), UINT_TO_FLOAT(v[3]));
}




/*
 * This code never registers handlers for any of the entry points
 * listed in vtxfmt.h.
 */
void
_mesa_loopback_init_api_table( struct _glapi_table *dest )
{
   SET_Color3b(dest, loopback_Color3b_f);
   SET_Color3d(dest, loopback_Color3d_f);
   SET_Color3i(dest, loopback_Color3i_f);
   SET_Color3s(dest, loopback_Color3s_f);
   SET_Color3ui(dest, loopback_Color3ui_f);
   SET_Color3us(dest, loopback_Color3us_f);
   SET_Color3ub(dest, loopback_Color3ub_f);
   SET_Color4b(dest, loopback_Color4b_f);
   SET_Color4d(dest, loopback_Color4d_f);
   SET_Color4i(dest, loopback_Color4i_f);
   SET_Color4s(dest, loopback_Color4s_f);
   SET_Color4ui(dest, loopback_Color4ui_f);
   SET_Color4us(dest, loopback_Color4us_f);
   SET_Color4ub(dest, loopback_Color4ub_f);
   SET_Color3bv(dest, loopback_Color3bv_f);
   SET_Color3dv(dest, loopback_Color3dv_f);
   SET_Color3iv(dest, loopback_Color3iv_f);
   SET_Color3sv(dest, loopback_Color3sv_f);
   SET_Color3uiv(dest, loopback_Color3uiv_f);
   SET_Color3usv(dest, loopback_Color3usv_f);
   SET_Color3ubv(dest, loopback_Color3ubv_f);
   SET_Color4bv(dest, loopback_Color4bv_f);
   SET_Color4dv(dest, loopback_Color4dv_f);
   SET_Color4iv(dest, loopback_Color4iv_f);
   SET_Color4sv(dest, loopback_Color4sv_f);
   SET_Color4uiv(dest, loopback_Color4uiv_f);
   SET_Color4usv(dest, loopback_Color4usv_f);
   SET_Color4ubv(dest, loopback_Color4ubv_f);

   SET_SecondaryColor3bEXT(dest, loopback_SecondaryColor3bEXT_f);
   SET_SecondaryColor3dEXT(dest, loopback_SecondaryColor3dEXT_f);
   SET_SecondaryColor3iEXT(dest, loopback_SecondaryColor3iEXT_f);
   SET_SecondaryColor3sEXT(dest, loopback_SecondaryColor3sEXT_f);
   SET_SecondaryColor3uiEXT(dest, loopback_SecondaryColor3uiEXT_f);
   SET_SecondaryColor3usEXT(dest, loopback_SecondaryColor3usEXT_f);
   SET_SecondaryColor3ubEXT(dest, loopback_SecondaryColor3ubEXT_f);
   SET_SecondaryColor3bvEXT(dest, loopback_SecondaryColor3bvEXT_f);
   SET_SecondaryColor3dvEXT(dest, loopback_SecondaryColor3dvEXT_f);
   SET_SecondaryColor3ivEXT(dest, loopback_SecondaryColor3ivEXT_f);
   SET_SecondaryColor3svEXT(dest, loopback_SecondaryColor3svEXT_f);
   SET_SecondaryColor3uivEXT(dest, loopback_SecondaryColor3uivEXT_f);
   SET_SecondaryColor3usvEXT(dest, loopback_SecondaryColor3usvEXT_f);
   SET_SecondaryColor3ubvEXT(dest, loopback_SecondaryColor3ubvEXT_f);
      
   SET_EdgeFlagv(dest, loopback_EdgeFlagv);

   SET_Indexd(dest, loopback_Indexd);
   SET_Indexi(dest, loopback_Indexi);
   SET_Indexs(dest, loopback_Indexs);
   SET_Indexub(dest, loopback_Indexub);
   SET_Indexdv(dest, loopback_Indexdv);
   SET_Indexiv(dest, loopback_Indexiv);
   SET_Indexsv(dest, loopback_Indexsv);
   SET_Indexubv(dest, loopback_Indexubv);
   SET_Normal3b(dest, loopback_Normal3b);
   SET_Normal3d(dest, loopback_Normal3d);
   SET_Normal3i(dest, loopback_Normal3i);
   SET_Normal3s(dest, loopback_Normal3s);
   SET_Normal3bv(dest, loopback_Normal3bv);
   SET_Normal3dv(dest, loopback_Normal3dv);
   SET_Normal3iv(dest, loopback_Normal3iv);
   SET_Normal3sv(dest, loopback_Normal3sv);
   SET_TexCoord1d(dest, loopback_TexCoord1d);
   SET_TexCoord1i(dest, loopback_TexCoord1i);
   SET_TexCoord1s(dest, loopback_TexCoord1s);
   SET_TexCoord2d(dest, loopback_TexCoord2d);
   SET_TexCoord2s(dest, loopback_TexCoord2s);
   SET_TexCoord2i(dest, loopback_TexCoord2i);
   SET_TexCoord3d(dest, loopback_TexCoord3d);
   SET_TexCoord3i(dest, loopback_TexCoord3i);
   SET_TexCoord3s(dest, loopback_TexCoord3s);
   SET_TexCoord4d(dest, loopback_TexCoord4d);
   SET_TexCoord4i(dest, loopback_TexCoord4i);
   SET_TexCoord4s(dest, loopback_TexCoord4s);
   SET_TexCoord1dv(dest, loopback_TexCoord1dv);
   SET_TexCoord1iv(dest, loopback_TexCoord1iv);
   SET_TexCoord1sv(dest, loopback_TexCoord1sv);
   SET_TexCoord2dv(dest, loopback_TexCoord2dv);
   SET_TexCoord2iv(dest, loopback_TexCoord2iv);
   SET_TexCoord2sv(dest, loopback_TexCoord2sv);
   SET_TexCoord3dv(dest, loopback_TexCoord3dv);
   SET_TexCoord3iv(dest, loopback_TexCoord3iv);
   SET_TexCoord3sv(dest, loopback_TexCoord3sv);
   SET_TexCoord4dv(dest, loopback_TexCoord4dv);
   SET_TexCoord4iv(dest, loopback_TexCoord4iv);
   SET_TexCoord4sv(dest, loopback_TexCoord4sv);
   SET_Vertex2d(dest, loopback_Vertex2d);
   SET_Vertex2i(dest, loopback_Vertex2i);
   SET_Vertex2s(dest, loopback_Vertex2s);
   SET_Vertex3d(dest, loopback_Vertex3d);
   SET_Vertex3i(dest, loopback_Vertex3i);
   SET_Vertex3s(dest, loopback_Vertex3s);
   SET_Vertex4d(dest, loopback_Vertex4d);
   SET_Vertex4i(dest, loopback_Vertex4i);
   SET_Vertex4s(dest, loopback_Vertex4s);
   SET_Vertex2dv(dest, loopback_Vertex2dv);
   SET_Vertex2iv(dest, loopback_Vertex2iv);
   SET_Vertex2sv(dest, loopback_Vertex2sv);
   SET_Vertex3dv(dest, loopback_Vertex3dv);
   SET_Vertex3iv(dest, loopback_Vertex3iv);
   SET_Vertex3sv(dest, loopback_Vertex3sv);
   SET_Vertex4dv(dest, loopback_Vertex4dv);
   SET_Vertex4iv(dest, loopback_Vertex4iv);
   SET_Vertex4sv(dest, loopback_Vertex4sv);
   SET_MultiTexCoord1dARB(dest, loopback_MultiTexCoord1dARB);
   SET_MultiTexCoord1dvARB(dest, loopback_MultiTexCoord1dvARB);
   SET_MultiTexCoord1iARB(dest, loopback_MultiTexCoord1iARB);
   SET_MultiTexCoord1ivARB(dest, loopback_MultiTexCoord1ivARB);
   SET_MultiTexCoord1sARB(dest, loopback_MultiTexCoord1sARB);
   SET_MultiTexCoord1svARB(dest, loopback_MultiTexCoord1svARB);
   SET_MultiTexCoord2dARB(dest, loopback_MultiTexCoord2dARB);
   SET_MultiTexCoord2dvARB(dest, loopback_MultiTexCoord2dvARB);
   SET_MultiTexCoord2iARB(dest, loopback_MultiTexCoord2iARB);
   SET_MultiTexCoord2ivARB(dest, loopback_MultiTexCoord2ivARB);
   SET_MultiTexCoord2sARB(dest, loopback_MultiTexCoord2sARB);
   SET_MultiTexCoord2svARB(dest, loopback_MultiTexCoord2svARB);
   SET_MultiTexCoord3dARB(dest, loopback_MultiTexCoord3dARB);
   SET_MultiTexCoord3dvARB(dest, loopback_MultiTexCoord3dvARB);
   SET_MultiTexCoord3iARB(dest, loopback_MultiTexCoord3iARB);
   SET_MultiTexCoord3ivARB(dest, loopback_MultiTexCoord3ivARB);
   SET_MultiTexCoord3sARB(dest, loopback_MultiTexCoord3sARB);
   SET_MultiTexCoord3svARB(dest, loopback_MultiTexCoord3svARB);
   SET_MultiTexCoord4dARB(dest, loopback_MultiTexCoord4dARB);
   SET_MultiTexCoord4dvARB(dest, loopback_MultiTexCoord4dvARB);
   SET_MultiTexCoord4iARB(dest, loopback_MultiTexCoord4iARB);
   SET_MultiTexCoord4ivARB(dest, loopback_MultiTexCoord4ivARB);
   SET_MultiTexCoord4sARB(dest, loopback_MultiTexCoord4sARB);
   SET_MultiTexCoord4svARB(dest, loopback_MultiTexCoord4svARB);
   SET_EvalCoord2dv(dest, loopback_EvalCoord2dv);
   SET_EvalCoord2fv(dest, loopback_EvalCoord2fv);
   SET_EvalCoord2d(dest, loopback_EvalCoord2d);
   SET_EvalCoord1dv(dest, loopback_EvalCoord1dv);
   SET_EvalCoord1fv(dest, loopback_EvalCoord1fv);
   SET_EvalCoord1d(dest, loopback_EvalCoord1d);
   SET_Materialf(dest, loopback_Materialf);
   SET_Materiali(dest, loopback_Materiali);
   SET_Materialiv(dest, loopback_Materialiv);
   SET_Rectd(dest, loopback_Rectd);
   SET_Rectdv(dest, loopback_Rectdv);
   SET_Rectfv(dest, loopback_Rectfv);
   SET_Recti(dest, loopback_Recti);
   SET_Rectiv(dest, loopback_Rectiv);
   SET_Rects(dest, loopback_Rects);
   SET_Rectsv(dest, loopback_Rectsv);
   SET_FogCoorddEXT(dest, loopback_FogCoorddEXT);
   SET_FogCoorddvEXT(dest, loopback_FogCoorddvEXT);

   SET_VertexAttrib1sNV(dest, loopback_VertexAttrib1sNV);
   SET_VertexAttrib1dNV(dest, loopback_VertexAttrib1dNV);
   SET_VertexAttrib2sNV(dest, loopback_VertexAttrib2sNV);
   SET_VertexAttrib2dNV(dest, loopback_VertexAttrib2dNV);
   SET_VertexAttrib3sNV(dest, loopback_VertexAttrib3sNV);
   SET_VertexAttrib3dNV(dest, loopback_VertexAttrib3dNV);
   SET_VertexAttrib4sNV(dest, loopback_VertexAttrib4sNV);
   SET_VertexAttrib4dNV(dest, loopback_VertexAttrib4dNV);
   SET_VertexAttrib4ubNV(dest, loopback_VertexAttrib4ubNV);
   SET_VertexAttrib1svNV(dest, loopback_VertexAttrib1svNV);
   SET_VertexAttrib1dvNV(dest, loopback_VertexAttrib1dvNV);
   SET_VertexAttrib2svNV(dest, loopback_VertexAttrib2svNV);
   SET_VertexAttrib2dvNV(dest, loopback_VertexAttrib2dvNV);
   SET_VertexAttrib3svNV(dest, loopback_VertexAttrib3svNV);
   SET_VertexAttrib3dvNV(dest, loopback_VertexAttrib3dvNV);
   SET_VertexAttrib4svNV(dest, loopback_VertexAttrib4svNV);
   SET_VertexAttrib4dvNV(dest, loopback_VertexAttrib4dvNV);
   SET_VertexAttrib4ubvNV(dest, loopback_VertexAttrib4ubvNV);
   SET_VertexAttribs1svNV(dest, loopback_VertexAttribs1svNV);
   SET_VertexAttribs1fvNV(dest, loopback_VertexAttribs1fvNV);
   SET_VertexAttribs1dvNV(dest, loopback_VertexAttribs1dvNV);
   SET_VertexAttribs2svNV(dest, loopback_VertexAttribs2svNV);
   SET_VertexAttribs2fvNV(dest, loopback_VertexAttribs2fvNV);
   SET_VertexAttribs2dvNV(dest, loopback_VertexAttribs2dvNV);
   SET_VertexAttribs3svNV(dest, loopback_VertexAttribs3svNV);
   SET_VertexAttribs3fvNV(dest, loopback_VertexAttribs3fvNV);
   SET_VertexAttribs3dvNV(dest, loopback_VertexAttribs3dvNV);
   SET_VertexAttribs4svNV(dest, loopback_VertexAttribs4svNV);
   SET_VertexAttribs4fvNV(dest, loopback_VertexAttribs4fvNV);
   SET_VertexAttribs4dvNV(dest, loopback_VertexAttribs4dvNV);
   SET_VertexAttribs4ubvNV(dest, loopback_VertexAttribs4ubvNV);

   SET_VertexAttrib1sARB(dest, loopback_VertexAttrib1sARB);
   SET_VertexAttrib1dARB(dest, loopback_VertexAttrib1dARB);
   SET_VertexAttrib2sARB(dest, loopback_VertexAttrib2sARB);
   SET_VertexAttrib2dARB(dest, loopback_VertexAttrib2dARB);
   SET_VertexAttrib3sARB(dest, loopback_VertexAttrib3sARB);
   SET_VertexAttrib3dARB(dest, loopback_VertexAttrib3dARB);
   SET_VertexAttrib4sARB(dest, loopback_VertexAttrib4sARB);
   SET_VertexAttrib4dARB(dest, loopback_VertexAttrib4dARB);
   SET_VertexAttrib1svARB(dest, loopback_VertexAttrib1svARB);
   SET_VertexAttrib1dvARB(dest, loopback_VertexAttrib1dvARB);
   SET_VertexAttrib2svARB(dest, loopback_VertexAttrib2svARB);
   SET_VertexAttrib2dvARB(dest, loopback_VertexAttrib2dvARB);
   SET_VertexAttrib3svARB(dest, loopback_VertexAttrib3svARB);
   SET_VertexAttrib3dvARB(dest, loopback_VertexAttrib3dvARB);
   SET_VertexAttrib4svARB(dest, loopback_VertexAttrib4svARB);
   SET_VertexAttrib4dvARB(dest, loopback_VertexAttrib4dvARB);
   SET_VertexAttrib4NubARB(dest, loopback_VertexAttrib4NubARB);
   SET_VertexAttrib4NubvARB(dest, loopback_VertexAttrib4NubvARB);
   SET_VertexAttrib4bvARB(dest, loopback_VertexAttrib4bvARB);
   SET_VertexAttrib4ivARB(dest, loopback_VertexAttrib4ivARB);
   SET_VertexAttrib4ubvARB(dest, loopback_VertexAttrib4ubvARB);
   SET_VertexAttrib4usvARB(dest, loopback_VertexAttrib4usvARB);
   SET_VertexAttrib4uivARB(dest, loopback_VertexAttrib4uivARB);
   SET_VertexAttrib4NbvARB(dest, loopback_VertexAttrib4NbvARB);
   SET_VertexAttrib4NsvARB(dest, loopback_VertexAttrib4NsvARB);
   SET_VertexAttrib4NivARB(dest, loopback_VertexAttrib4NivARB);
   SET_VertexAttrib4NusvARB(dest, loopback_VertexAttrib4NusvARB);
   SET_VertexAttrib4NuivARB(dest, loopback_VertexAttrib4NuivARB);
}
