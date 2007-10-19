/**************************************************************************

Copyright 2004 Tungsten Graphics Inc., Cedar Park, Texas.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "vtxfmt.h"
#include "dlist.h"
#include "state.h"
#include "light.h"
#include "api_arrayelt.h"
#include "api_noop.h"
#include "t_vtx_api.h"


/* Versions of all the entrypoints for situations where codegen isn't
 * available.
 *
 * Note: Only one size for each attribute may be active at once.
 * Eg. if Color3f is installed/active, then Color4f may not be, even
 * if the vertex actually contains 4 color coordinates.  This is
 * because the 3f version won't otherwise set color[3] to 1.0 -- this
 * is the job of the chooser function when switching between Color4f
 * and Color3f.
 */
#define ATTRFV( ATTR, N )				\
static void attrib_##ATTR##_##N( const GLfloat *v )	\
{							\
   GET_CURRENT_CONTEXT( ctx );				\
   TNLcontext *tnl = TNL_CONTEXT(ctx);			\
							\
   if ((ATTR) == 0) {					\
      GLuint i;						\
							\
      if (N>0) tnl->vtx.vbptr[0] = v[0];		\
      if (N>1) tnl->vtx.vbptr[1] = v[1];		\
      if (N>2) tnl->vtx.vbptr[2] = v[2];		\
      if (N>3) tnl->vtx.vbptr[3] = v[3];		\
							\
      for (i = N; i < tnl->vtx.vertex_size; i++)	\
	 tnl->vtx.vbptr[i] = tnl->vtx.vertex[i];	\
							\
      tnl->vtx.vbptr += tnl->vtx.vertex_size;		\
							\
      if (--tnl->vtx.counter == 0)			\
	 _tnl_wrap_filled_vertex( ctx );		\
   }							\
   else {						\
      GLfloat *dest = tnl->vtx.attrptr[ATTR];		\
      if (N>0) dest[0] = v[0];				\
      if (N>1) dest[1] = v[1];				\
      if (N>2) dest[2] = v[2];				\
      if (N>3) dest[3] = v[3];				\
   }							\
}

#define INIT(TAB, ATTR)						\
   TAB[ATTR][0] = attrib_##ATTR##_1;				\
   TAB[ATTR][1] = attrib_##ATTR##_2;				\
   TAB[ATTR][2] = attrib_##ATTR##_3;				\
   TAB[ATTR][3] = attrib_##ATTR##_4;


#define ATTRS( ATTRIB )				\
   ATTRFV( ATTRIB, 1 )				\
   ATTRFV( ATTRIB, 2 )				\
   ATTRFV( ATTRIB, 3 )				\
   ATTRFV( ATTRIB, 4 )

ATTRS( 0 )
ATTRS( 1 )
ATTRS( 2 )
ATTRS( 3 )
ATTRS( 4 )
ATTRS( 5 )
ATTRS( 6 )
ATTRS( 7 )
ATTRS( 8 )
ATTRS( 9 )
ATTRS( 10 )
ATTRS( 11 )
ATTRS( 12 )
ATTRS( 13 )
ATTRS( 14 )
ATTRS( 15 )

void _tnl_generic_attr_table_init( tnl_attrfv_func (*tab)[4] )
{
   INIT( tab, 0 );
   INIT( tab, 1 );
   INIT( tab, 2 );
   INIT( tab, 3 );
   INIT( tab, 4 );
   INIT( tab, 5 );
   INIT( tab, 6 );
   INIT( tab, 7 );
   INIT( tab, 8 );
   INIT( tab, 9 );
   INIT( tab, 10 );
   INIT( tab, 11 );
   INIT( tab, 12 );
   INIT( tab, 13 );
   INIT( tab, 14 );
   INIT( tab, 15 );
}

/* These can be made efficient with codegen.  Further, by adding more
 * logic to do_choose(), the double-dispatch for legacy entrypoints
 * like glVertex3f() can be removed.
 */
#define DISPATCH_ATTRFV( ATTR, COUNT, P )	\
do {						\
   GET_CURRENT_CONTEXT( ctx ); 			\
   TNLcontext *tnl = TNL_CONTEXT(ctx); 		\
   tnl->vtx.tabfv[ATTR][COUNT-1]( P );		\
} while (0)

#define DISPATCH_ATTR1FV( ATTR, V ) DISPATCH_ATTRFV( ATTR, 1, V )
#define DISPATCH_ATTR2FV( ATTR, V ) DISPATCH_ATTRFV( ATTR, 2, V )
#define DISPATCH_ATTR3FV( ATTR, V ) DISPATCH_ATTRFV( ATTR, 3, V )
#define DISPATCH_ATTR4FV( ATTR, V ) DISPATCH_ATTRFV( ATTR, 4, V )

#define DISPATCH_ATTR1F( ATTR, S ) DISPATCH_ATTRFV( ATTR, 1, &(S) )

#if defined(USE_X86_ASM) && 0 /* will break register calling convention */
/* Naughty cheat:
 */
#define DISPATCH_ATTR2F( ATTR, S,T ) DISPATCH_ATTRFV( ATTR, 2, &(S) )
#define DISPATCH_ATTR3F( ATTR, S,T,R ) DISPATCH_ATTRFV( ATTR, 3, &(S) )
#define DISPATCH_ATTR4F( ATTR, S,T,R,Q ) DISPATCH_ATTRFV( ATTR, 4, &(S) )
#else
/* Safe:
 */
#define DISPATCH_ATTR2F( ATTR, S,T ) 		\
do { 						\
   GLfloat v[2]; 				\
   v[0] = S; v[1] = T;				\
   DISPATCH_ATTR2FV( ATTR, v );			\
} while (0)
#define DISPATCH_ATTR3F( ATTR, S,T,R ) 		\
do { 						\
   GLfloat v[3]; 				\
   v[0] = S; v[1] = T; v[2] = R;		\
   DISPATCH_ATTR3FV( ATTR, v );			\
} while (0)
#define DISPATCH_ATTR4F( ATTR, S,T,R,Q )	\
do { 						\
   GLfloat v[4]; 				\
   v[0] = S; v[1] = T; v[2] = R; v[3] = Q;	\
   DISPATCH_ATTR4FV( ATTR, v );			\
} while (0)
#endif


static void GLAPIENTRY _tnl_Vertex2f( GLfloat x, GLfloat y )
{
   DISPATCH_ATTR2F( _TNL_ATTRIB_POS, x, y );
}

static void GLAPIENTRY _tnl_Vertex2fv( const GLfloat *v )
{
   DISPATCH_ATTR2FV( _TNL_ATTRIB_POS, v );
}

static void GLAPIENTRY _tnl_Vertex3f( GLfloat x, GLfloat y, GLfloat z )
{
   DISPATCH_ATTR3F( _TNL_ATTRIB_POS, x, y, z );
}

static void GLAPIENTRY _tnl_Vertex3fv( const GLfloat *v )
{
   DISPATCH_ATTR3FV( _TNL_ATTRIB_POS, v );
}

static void GLAPIENTRY _tnl_Vertex4f( GLfloat x, GLfloat y, GLfloat z,
				      GLfloat w )
{
   DISPATCH_ATTR4F( _TNL_ATTRIB_POS, x, y, z, w );
}

static void GLAPIENTRY _tnl_Vertex4fv( const GLfloat *v )
{
   DISPATCH_ATTR4FV( _TNL_ATTRIB_POS, v );
}

static void GLAPIENTRY _tnl_TexCoord1f( GLfloat x )
{
   DISPATCH_ATTR1F( _TNL_ATTRIB_TEX0, x );
}

static void GLAPIENTRY _tnl_TexCoord1fv( const GLfloat *v )
{
   DISPATCH_ATTR1FV( _TNL_ATTRIB_TEX0, v );
}

static void GLAPIENTRY _tnl_TexCoord2f( GLfloat x, GLfloat y )
{
   DISPATCH_ATTR2F( _TNL_ATTRIB_TEX0, x, y );
}

static void GLAPIENTRY _tnl_TexCoord2fv( const GLfloat *v )
{
   DISPATCH_ATTR2FV( _TNL_ATTRIB_TEX0, v );
}

static void GLAPIENTRY _tnl_TexCoord3f( GLfloat x, GLfloat y, GLfloat z )
{
   DISPATCH_ATTR3F( _TNL_ATTRIB_TEX0, x, y, z );
}

static void GLAPIENTRY _tnl_TexCoord3fv( const GLfloat *v )
{
   DISPATCH_ATTR3FV( _TNL_ATTRIB_TEX0, v );
}

static void GLAPIENTRY _tnl_TexCoord4f( GLfloat x, GLfloat y, GLfloat z,
					GLfloat w )
{
   DISPATCH_ATTR4F( _TNL_ATTRIB_TEX0, x, y, z, w );
}

static void GLAPIENTRY _tnl_TexCoord4fv( const GLfloat *v )
{
   DISPATCH_ATTR4FV( _TNL_ATTRIB_TEX0, v );
}

static void GLAPIENTRY _tnl_Normal3f( GLfloat x, GLfloat y, GLfloat z )
{
   DISPATCH_ATTR3F( _TNL_ATTRIB_NORMAL, x, y, z );
}

static void GLAPIENTRY _tnl_Normal3fv( const GLfloat *v )
{
   DISPATCH_ATTR3FV( _TNL_ATTRIB_NORMAL, v );
}

static void GLAPIENTRY _tnl_FogCoordfEXT( GLfloat x )
{
   DISPATCH_ATTR1F( _TNL_ATTRIB_FOG, x );
}

static void GLAPIENTRY _tnl_FogCoordfvEXT( const GLfloat *v )
{
   DISPATCH_ATTR1FV( _TNL_ATTRIB_FOG, v );
}

static void GLAPIENTRY _tnl_Color3f( GLfloat x, GLfloat y, GLfloat z )
{
   DISPATCH_ATTR3F( _TNL_ATTRIB_COLOR0, x, y, z );
}

static void GLAPIENTRY _tnl_Color3fv( const GLfloat *v )
{
   DISPATCH_ATTR3FV( _TNL_ATTRIB_COLOR0, v );
}

static void GLAPIENTRY _tnl_Color4f( GLfloat x, GLfloat y, GLfloat z,
				     GLfloat w )
{
   DISPATCH_ATTR4F( _TNL_ATTRIB_COLOR0, x, y, z, w );
}

static void GLAPIENTRY _tnl_Color4fv( const GLfloat *v )
{
   DISPATCH_ATTR4FV( _TNL_ATTRIB_COLOR0, v );
}

static void GLAPIENTRY _tnl_SecondaryColor3fEXT( GLfloat x, GLfloat y,
						 GLfloat z )
{
   DISPATCH_ATTR3F( _TNL_ATTRIB_COLOR1, x, y, z );
}

static void GLAPIENTRY _tnl_SecondaryColor3fvEXT( const GLfloat *v )
{
   DISPATCH_ATTR3FV( _TNL_ATTRIB_COLOR1, v );
}

static void GLAPIENTRY _tnl_MultiTexCoord1f( GLenum target, GLfloat x  )
{
   GLuint attr = (target & 0x7) + _TNL_ATTRIB_TEX0;
   DISPATCH_ATTR1F( attr, x );
}

static void GLAPIENTRY _tnl_MultiTexCoord1fv( GLenum target,
					      const GLfloat *v )
{
   GLuint attr = (target & 0x7) + _TNL_ATTRIB_TEX0;
   DISPATCH_ATTR1FV( attr, v );
}

static void GLAPIENTRY _tnl_MultiTexCoord2f( GLenum target, GLfloat x,
					     GLfloat y )
{
   GLuint attr = (target & 0x7) + _TNL_ATTRIB_TEX0;
   DISPATCH_ATTR2F( attr, x, y );
}

static void GLAPIENTRY _tnl_MultiTexCoord2fv( GLenum target,
					      const GLfloat *v )
{
   GLuint attr = (target & 0x7) + _TNL_ATTRIB_TEX0;
   DISPATCH_ATTR2FV( attr, v );
}

static void GLAPIENTRY _tnl_MultiTexCoord3f( GLenum target, GLfloat x,
					     GLfloat y, GLfloat z)
{
   GLuint attr = (target & 0x7) + _TNL_ATTRIB_TEX0;
   DISPATCH_ATTR3F( attr, x, y, z );
}

static void GLAPIENTRY _tnl_MultiTexCoord3fv( GLenum target,
					      const GLfloat *v )
{
   GLuint attr = (target & 0x7) + _TNL_ATTRIB_TEX0;
   DISPATCH_ATTR3FV( attr, v );
}

static void GLAPIENTRY _tnl_MultiTexCoord4f( GLenum target, GLfloat x,
					     GLfloat y, GLfloat z,
					     GLfloat w )
{
   GLuint attr = (target & 0x7) + _TNL_ATTRIB_TEX0;
   DISPATCH_ATTR4F( attr, x, y, z, w );
}

static void GLAPIENTRY _tnl_MultiTexCoord4fv( GLenum target,
					      const GLfloat *v )
{
   GLuint attr = (target & 0x7) + _TNL_ATTRIB_TEX0;
   DISPATCH_ATTR4FV( attr, v );
}


static void GLAPIENTRY _tnl_VertexAttrib1fNV( GLuint index, GLfloat x )
{
   if (index >= VERT_ATTRIB_MAX) index = ERROR_ATTRIB;
   DISPATCH_ATTR1F( index, x );
}

static void GLAPIENTRY _tnl_VertexAttrib1fvNV( GLuint index,
					       const GLfloat *v )
{
   if (index >= VERT_ATTRIB_MAX) index = ERROR_ATTRIB;
   DISPATCH_ATTR1FV( index, v );
}

static void GLAPIENTRY _tnl_VertexAttrib2fNV( GLuint index, GLfloat x,
					      GLfloat y )
{
   if (index >= VERT_ATTRIB_MAX) index = ERROR_ATTRIB;
   DISPATCH_ATTR2F( index, x, y );
}

static void GLAPIENTRY _tnl_VertexAttrib2fvNV( GLuint index,
					       const GLfloat *v )
{
   if (index >= VERT_ATTRIB_MAX) index = ERROR_ATTRIB;
   DISPATCH_ATTR2FV( index, v );
}

static void GLAPIENTRY _tnl_VertexAttrib3fNV( GLuint index, GLfloat x,
					      GLfloat y, GLfloat z )
{
   if (index >= VERT_ATTRIB_MAX) index = ERROR_ATTRIB;
   DISPATCH_ATTR3F( index, x, y, z );
}

static void GLAPIENTRY _tnl_VertexAttrib3fvNV( GLuint index,
					       const GLfloat *v )
{
   if (index >= VERT_ATTRIB_MAX) index = ERROR_ATTRIB;
   DISPATCH_ATTR3FV( index, v );
}

static void GLAPIENTRY _tnl_VertexAttrib4fNV( GLuint index, GLfloat x,
					      GLfloat y, GLfloat z,
					      GLfloat w )
{
   if (index >= VERT_ATTRIB_MAX) index = ERROR_ATTRIB;
   DISPATCH_ATTR4F( index, x, y, z, w );
}

static void GLAPIENTRY _tnl_VertexAttrib4fvNV( GLuint index,
					       const GLfloat *v )
{
   if (index >= VERT_ATTRIB_MAX) index = ERROR_ATTRIB;
   DISPATCH_ATTR4FV( index, v );
}


/*
 * XXX adjust index
 */

static void GLAPIENTRY _tnl_VertexAttrib1fARB( GLuint index, GLfloat x )
{
   if (index >= VERT_ATTRIB_MAX) index = ERROR_ATTRIB;
   DISPATCH_ATTR1F( index, x );
}

static void GLAPIENTRY _tnl_VertexAttrib1fvARB( GLuint index,
					       const GLfloat *v )
{
   if (index >= VERT_ATTRIB_MAX) index = ERROR_ATTRIB;
   DISPATCH_ATTR1FV( index, v );
}

static void GLAPIENTRY _tnl_VertexAttrib2fARB( GLuint index, GLfloat x,
					      GLfloat y )
{
   if (index >= VERT_ATTRIB_MAX) index = ERROR_ATTRIB;
   DISPATCH_ATTR2F( index, x, y );
}

static void GLAPIENTRY _tnl_VertexAttrib2fvARB( GLuint index,
					       const GLfloat *v )
{
   if (index >= VERT_ATTRIB_MAX) index = ERROR_ATTRIB;
   DISPATCH_ATTR2FV( index, v );
}

static void GLAPIENTRY _tnl_VertexAttrib3fARB( GLuint index, GLfloat x,
					      GLfloat y, GLfloat z )
{
   if (index >= VERT_ATTRIB_MAX) index = ERROR_ATTRIB;
   DISPATCH_ATTR3F( index, x, y, z );
}

static void GLAPIENTRY _tnl_VertexAttrib3fvARB( GLuint index,
					       const GLfloat *v )
{
   if (index >= VERT_ATTRIB_MAX) index = ERROR_ATTRIB;
   DISPATCH_ATTR3FV( index, v );
}

static void GLAPIENTRY _tnl_VertexAttrib4fARB( GLuint index, GLfloat x,
					      GLfloat y, GLfloat z,
					      GLfloat w )
{
   if (index >= VERT_ATTRIB_MAX) index = ERROR_ATTRIB;
   DISPATCH_ATTR4F( index, x, y, z, w );
}

static void GLAPIENTRY _tnl_VertexAttrib4fvARB( GLuint index,
					       const GLfloat *v )
{
   if (index >= VERT_ATTRIB_MAX) index = ERROR_ATTRIB;
   DISPATCH_ATTR4FV( index, v );
}


/* Install the generic versions of the 2nd level dispatch
 * functions.  Some of these have a codegen alternative.
 */
void _tnl_generic_exec_vtxfmt_init( GLcontext *ctx )
{
   GLvertexformat *vfmt = &(TNL_CONTEXT(ctx)->exec_vtxfmt);

   vfmt->Color3f = _tnl_Color3f;
   vfmt->Color3fv = _tnl_Color3fv;
   vfmt->Color4f = _tnl_Color4f;
   vfmt->Color4fv = _tnl_Color4fv;
   vfmt->FogCoordfEXT = _tnl_FogCoordfEXT;
   vfmt->FogCoordfvEXT = _tnl_FogCoordfvEXT;
   vfmt->MultiTexCoord1fARB = _tnl_MultiTexCoord1f;
   vfmt->MultiTexCoord1fvARB = _tnl_MultiTexCoord1fv;
   vfmt->MultiTexCoord2fARB = _tnl_MultiTexCoord2f;
   vfmt->MultiTexCoord2fvARB = _tnl_MultiTexCoord2fv;
   vfmt->MultiTexCoord3fARB = _tnl_MultiTexCoord3f;
   vfmt->MultiTexCoord3fvARB = _tnl_MultiTexCoord3fv;
   vfmt->MultiTexCoord4fARB = _tnl_MultiTexCoord4f;
   vfmt->MultiTexCoord4fvARB = _tnl_MultiTexCoord4fv;
   vfmt->Normal3f = _tnl_Normal3f;
   vfmt->Normal3fv = _tnl_Normal3fv;
   vfmt->SecondaryColor3fEXT = _tnl_SecondaryColor3fEXT;
   vfmt->SecondaryColor3fvEXT = _tnl_SecondaryColor3fvEXT;
   vfmt->TexCoord1f = _tnl_TexCoord1f;
   vfmt->TexCoord1fv = _tnl_TexCoord1fv;
   vfmt->TexCoord2f = _tnl_TexCoord2f;
   vfmt->TexCoord2fv = _tnl_TexCoord2fv;
   vfmt->TexCoord3f = _tnl_TexCoord3f;
   vfmt->TexCoord3fv = _tnl_TexCoord3fv;
   vfmt->TexCoord4f = _tnl_TexCoord4f;
   vfmt->TexCoord4fv = _tnl_TexCoord4fv;
   vfmt->Vertex2f = _tnl_Vertex2f;
   vfmt->Vertex2fv = _tnl_Vertex2fv;
   vfmt->Vertex3f = _tnl_Vertex3f;
   vfmt->Vertex3fv = _tnl_Vertex3fv;
   vfmt->Vertex4f = _tnl_Vertex4f;
   vfmt->Vertex4fv = _tnl_Vertex4fv;
   vfmt->VertexAttrib1fNV = _tnl_VertexAttrib1fNV;
   vfmt->VertexAttrib1fvNV = _tnl_VertexAttrib1fvNV;
   vfmt->VertexAttrib2fNV = _tnl_VertexAttrib2fNV;
   vfmt->VertexAttrib2fvNV = _tnl_VertexAttrib2fvNV;
   vfmt->VertexAttrib3fNV = _tnl_VertexAttrib3fNV;
   vfmt->VertexAttrib3fvNV = _tnl_VertexAttrib3fvNV;
   vfmt->VertexAttrib4fNV = _tnl_VertexAttrib4fNV;
   vfmt->VertexAttrib4fvNV = _tnl_VertexAttrib4fvNV;
   vfmt->VertexAttrib1fARB = _tnl_VertexAttrib1fARB;
   vfmt->VertexAttrib1fvARB = _tnl_VertexAttrib1fvARB;
   vfmt->VertexAttrib2fARB = _tnl_VertexAttrib2fARB;
   vfmt->VertexAttrib2fvARB = _tnl_VertexAttrib2fvARB;
   vfmt->VertexAttrib3fARB = _tnl_VertexAttrib3fARB;
   vfmt->VertexAttrib3fvARB = _tnl_VertexAttrib3fvARB;
   vfmt->VertexAttrib4fARB = _tnl_VertexAttrib4fARB;
   vfmt->VertexAttrib4fvARB = _tnl_VertexAttrib4fvARB;
}
