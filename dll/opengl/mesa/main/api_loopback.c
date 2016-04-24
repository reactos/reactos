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

#include <precomp.h>

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
#define EVALCOORD1(x)               CALL_EvalCoord1f(GET_DISPATCH(), (x))
#define EVALCOORD2(x,y)             CALL_EvalCoord2f(GET_DISPATCH(), (x,y))
#define MATERIALFV(a,b,c)           CALL_Materialfv(GET_DISPATCH(), (a,b,c))
#define RECTF(a,b,c,d)              CALL_Rectf(GET_DISPATCH(), (a,b,c,d))

#define FOGCOORDF(x)                CALL_FogCoordfEXT(GET_DISPATCH(), (x))

#define ATTRIB1NV(index,x)          CALL_VertexAttrib1fNV(GET_DISPATCH(), (index,x))
#define ATTRIB2NV(index,x,y)        CALL_VertexAttrib2fNV(GET_DISPATCH(), (index,x,y))
#define ATTRIB3NV(index,x,y,z)      CALL_VertexAttrib3fNV(GET_DISPATCH(), (index,x,y,z))
#define ATTRIB4NV(index,x,y,z,w)    CALL_VertexAttrib4fNV(GET_DISPATCH(), (index,x,y,z,w))


#if FEATURE_beginend


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


/*
 * GL_NV_vertex_program:
 * Always loop-back to one of the VertexAttrib[1234]f[v]NV functions.
 * Note that attribute indexes DO alias conventional vertex attributes.
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
}


#endif /* FEATURE_beginend */
