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

#include "brw_exec.h"


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
   struct brw_exec_context *exec = IMM_CONTEXT(ctx)->exec;			\
							\
   if ((ATTR) == 0) {					\
      GLuint i;						\
							\
      if (N>0) exec->vtx.vbptr[0] = v[0];		\
      if (N>1) exec->vtx.vbptr[1] = v[1];		\
      if (N>2) exec->vtx.vbptr[2] = v[2];		\
      if (N>3) exec->vtx.vbptr[3] = v[3];		\
							\
      for (i = N; i < exec->vtx.vertex_size; i++)	\
	 exec->vtx.vbptr[i] = exec->vtx.vertex[i];	\
							\
      exec->vtx.vbptr += exec->vtx.vertex_size;		\
      exec->ctx->Driver.NeedFlush |= FLUSH_STORED_VERTICES; \
							\
      if (++exec->vtx.vert_count >= exec->vtx.max_vert)	\
	 brw_exec_vtx_wrap( exec );		\
   }							\
   else {						\
      GLfloat *dest = exec->vtx.attrptr[ATTR];		\
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

void brw_exec_generic_attr_table_init( brw_attrfv_func (*tab)[4] )
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
   struct brw_exec_context *exec = IMM_CONTEXT(ctx)->exec; 		\
   exec->vtx.tabfv[ATTR][COUNT-1]( P );		\
} while (0)

#define DISPATCH_ATTR1FV( ATTR, V ) DISPATCH_ATTRFV( ATTR, 1, V )
#define DISPATCH_ATTR2FV( ATTR, V ) DISPATCH_ATTRFV( ATTR, 2, V )
#define DISPATCH_ATTR3FV( ATTR, V ) DISPATCH_ATTRFV( ATTR, 3, V )
#define DISPATCH_ATTR4FV( ATTR, V ) DISPATCH_ATTRFV( ATTR, 4, V )

#define DISPATCH_ATTR1F( ATTR, S ) DISPATCH_ATTRFV( ATTR, 1, &(S) )

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


static void GLAPIENTRY brw_Vertex2f( GLfloat x, GLfloat y )
{
   DISPATCH_ATTR2F( BRW_ATTRIB_POS, x, y );
}

static void GLAPIENTRY brw_Vertex2fv( const GLfloat *v )
{
   DISPATCH_ATTR2FV( BRW_ATTRIB_POS, v );
}

static void GLAPIENTRY brw_Vertex3f( GLfloat x, GLfloat y, GLfloat z )
{
   DISPATCH_ATTR3F( BRW_ATTRIB_POS, x, y, z );
}

static void GLAPIENTRY brw_Vertex3fv( const GLfloat *v )
{
   DISPATCH_ATTR3FV( BRW_ATTRIB_POS, v );
}

static void GLAPIENTRY brw_Vertex4f( GLfloat x, GLfloat y, GLfloat z, 
				      GLfloat w )
{
   DISPATCH_ATTR4F( BRW_ATTRIB_POS, x, y, z, w );
}

static void GLAPIENTRY brw_Vertex4fv( const GLfloat *v )
{
   DISPATCH_ATTR4FV( BRW_ATTRIB_POS, v );
}

static void GLAPIENTRY brw_TexCoord1f( GLfloat x )
{
   DISPATCH_ATTR1F( BRW_ATTRIB_TEX0, x );
}

static void GLAPIENTRY brw_TexCoord1fv( const GLfloat *v )
{
   DISPATCH_ATTR1FV( BRW_ATTRIB_TEX0, v );
}

static void GLAPIENTRY brw_TexCoord2f( GLfloat x, GLfloat y )
{
   DISPATCH_ATTR2F( BRW_ATTRIB_TEX0, x, y );
}

static void GLAPIENTRY brw_TexCoord2fv( const GLfloat *v )
{
   DISPATCH_ATTR2FV( BRW_ATTRIB_TEX0, v );
}

static void GLAPIENTRY brw_TexCoord3f( GLfloat x, GLfloat y, GLfloat z )
{
   DISPATCH_ATTR3F( BRW_ATTRIB_TEX0, x, y, z );
}

static void GLAPIENTRY brw_TexCoord3fv( const GLfloat *v )
{
   DISPATCH_ATTR3FV( BRW_ATTRIB_TEX0, v );
}

static void GLAPIENTRY brw_TexCoord4f( GLfloat x, GLfloat y, GLfloat z,
					GLfloat w )
{
   DISPATCH_ATTR4F( BRW_ATTRIB_TEX0, x, y, z, w );
}

static void GLAPIENTRY brw_TexCoord4fv( const GLfloat *v )
{
   DISPATCH_ATTR4FV( BRW_ATTRIB_TEX0, v );
}

static void GLAPIENTRY brw_Normal3f( GLfloat x, GLfloat y, GLfloat z )
{
   DISPATCH_ATTR3F( BRW_ATTRIB_NORMAL, x, y, z );
}

static void GLAPIENTRY brw_Normal3fv( const GLfloat *v )
{
   DISPATCH_ATTR3FV( BRW_ATTRIB_NORMAL, v );
}

static void GLAPIENTRY brw_FogCoordfEXT( GLfloat x )
{
   DISPATCH_ATTR1F( BRW_ATTRIB_FOG, x );
}

static void GLAPIENTRY brw_FogCoordfvEXT( const GLfloat *v )
{
   DISPATCH_ATTR1FV( BRW_ATTRIB_FOG, v );
}

static void GLAPIENTRY brw_Color3f( GLfloat x, GLfloat y, GLfloat z )
{
   DISPATCH_ATTR3F( BRW_ATTRIB_COLOR0, x, y, z );
}

static void GLAPIENTRY brw_Color3fv( const GLfloat *v )
{
   DISPATCH_ATTR3FV( BRW_ATTRIB_COLOR0, v );
}

static void GLAPIENTRY brw_Color4f( GLfloat x, GLfloat y, GLfloat z, 
				     GLfloat w )
{
   DISPATCH_ATTR4F( BRW_ATTRIB_COLOR0, x, y, z, w );
}

static void GLAPIENTRY brw_Color4fv( const GLfloat *v )
{
   DISPATCH_ATTR4FV( BRW_ATTRIB_COLOR0, v );
}

static void GLAPIENTRY brw_SecondaryColor3fEXT( GLfloat x, GLfloat y, 
						 GLfloat z )
{
   DISPATCH_ATTR3F( BRW_ATTRIB_COLOR1, x, y, z );
}

static void GLAPIENTRY brw_SecondaryColor3fvEXT( const GLfloat *v )
{
   DISPATCH_ATTR3FV( BRW_ATTRIB_COLOR1, v );
}

static void GLAPIENTRY brw_MultiTexCoord1f( GLenum target, GLfloat x  )
{
   GLuint attr = (target & 0x7) + BRW_ATTRIB_TEX0;
   DISPATCH_ATTR1F( attr, x );
}

static void GLAPIENTRY brw_MultiTexCoord1fv( GLenum target,
					      const GLfloat *v )
{
   GLuint attr = (target & 0x7) + BRW_ATTRIB_TEX0;
   DISPATCH_ATTR1FV( attr, v );
}

static void GLAPIENTRY brw_MultiTexCoord2f( GLenum target, GLfloat x, 
					     GLfloat y )
{
   GLuint attr = (target & 0x7) + BRW_ATTRIB_TEX0;
   DISPATCH_ATTR2F( attr, x, y );
}

static void GLAPIENTRY brw_MultiTexCoord2fv( GLenum target, 
					      const GLfloat *v )
{
   GLuint attr = (target & 0x7) + BRW_ATTRIB_TEX0;
   DISPATCH_ATTR2FV( attr, v );
}

static void GLAPIENTRY brw_MultiTexCoord3f( GLenum target, GLfloat x, 
					     GLfloat y, GLfloat z)
{
   GLuint attr = (target & 0x7) + BRW_ATTRIB_TEX0;
   DISPATCH_ATTR3F( attr, x, y, z );
}

static void GLAPIENTRY brw_MultiTexCoord3fv( GLenum target, 
					      const GLfloat *v )
{
   GLuint attr = (target & 0x7) + BRW_ATTRIB_TEX0;
   DISPATCH_ATTR3FV( attr, v );
}

static void GLAPIENTRY brw_MultiTexCoord4f( GLenum target, GLfloat x, 
					     GLfloat y, GLfloat z,
					     GLfloat w )
{
   GLuint attr = (target & 0x7) + BRW_ATTRIB_TEX0;
   DISPATCH_ATTR4F( attr, x, y, z, w );
}

static void GLAPIENTRY brw_MultiTexCoord4fv( GLenum target, 
					      const GLfloat *v )
{
   GLuint attr = (target & 0x7) + BRW_ATTRIB_TEX0;
   DISPATCH_ATTR4FV( attr, v );
}


static void GLAPIENTRY brw_VertexAttrib1fNV( GLuint index, GLfloat x )
{
   if (index >= BRW_ATTRIB_FIRST_MATERIAL) index = ERROR_ATTRIB;
   DISPATCH_ATTR1F( index, x );
}

static void GLAPIENTRY brw_VertexAttrib1fvNV( GLuint index, 
					       const GLfloat *v )
{
   if (index >= BRW_ATTRIB_FIRST_MATERIAL) index = ERROR_ATTRIB;
   DISPATCH_ATTR1FV( index, v );
}

static void GLAPIENTRY brw_VertexAttrib2fNV( GLuint index, GLfloat x, 
					      GLfloat y )
{
   if (index >= BRW_ATTRIB_FIRST_MATERIAL) index = ERROR_ATTRIB;
   DISPATCH_ATTR2F( index, x, y );
}

static void GLAPIENTRY brw_VertexAttrib2fvNV( GLuint index,
					       const GLfloat *v )
{
   if (index >= BRW_ATTRIB_FIRST_MATERIAL) index = ERROR_ATTRIB;
   DISPATCH_ATTR2FV( index, v );
}

static void GLAPIENTRY brw_VertexAttrib3fNV( GLuint index, GLfloat x,
					      GLfloat y, GLfloat z )
{
   if (index >= BRW_ATTRIB_FIRST_MATERIAL) index = ERROR_ATTRIB;
   DISPATCH_ATTR3F( index, x, y, z );
}

static void GLAPIENTRY brw_VertexAttrib3fvNV( GLuint index,
					       const GLfloat *v )
{
   if (index >= BRW_ATTRIB_FIRST_MATERIAL) index = ERROR_ATTRIB;
   DISPATCH_ATTR3FV( index, v );
}

static void GLAPIENTRY brw_VertexAttrib4fNV( GLuint index, GLfloat x,
					      GLfloat y, GLfloat z,
					      GLfloat w )
{
   if (index >= BRW_ATTRIB_FIRST_MATERIAL) index = ERROR_ATTRIB;
   DISPATCH_ATTR4F( index, x, y, z, w );
}

static void GLAPIENTRY brw_VertexAttrib4fvNV( GLuint index, 
					       const GLfloat *v )
{
   if (index >= BRW_ATTRIB_FIRST_MATERIAL) index = ERROR_ATTRIB;
   DISPATCH_ATTR4FV( index, v );
}


/*
 * XXX adjust index
 */

static void GLAPIENTRY brw_VertexAttrib1fARB( GLuint index, GLfloat x )
{
   if (index >= BRW_ATTRIB_FIRST_MATERIAL) index = ERROR_ATTRIB;
   DISPATCH_ATTR1F( index, x );
}

static void GLAPIENTRY brw_VertexAttrib1fvARB( GLuint index, 
					       const GLfloat *v )
{
   if (index >= BRW_ATTRIB_FIRST_MATERIAL) index = ERROR_ATTRIB;
   DISPATCH_ATTR1FV( index, v );
}

static void GLAPIENTRY brw_VertexAttrib2fARB( GLuint index, GLfloat x, 
					      GLfloat y )
{
   if (index >= BRW_ATTRIB_FIRST_MATERIAL) index = ERROR_ATTRIB;
   DISPATCH_ATTR2F( index, x, y );
}

static void GLAPIENTRY brw_VertexAttrib2fvARB( GLuint index,
					       const GLfloat *v )
{
   if (index >= BRW_ATTRIB_FIRST_MATERIAL) index = ERROR_ATTRIB;
   DISPATCH_ATTR2FV( index, v );
}

static void GLAPIENTRY brw_VertexAttrib3fARB( GLuint index, GLfloat x,
					      GLfloat y, GLfloat z )
{
   if (index >= BRW_ATTRIB_FIRST_MATERIAL) index = ERROR_ATTRIB;
   DISPATCH_ATTR3F( index, x, y, z );
}

static void GLAPIENTRY brw_VertexAttrib3fvARB( GLuint index,
					       const GLfloat *v )
{
   if (index >= BRW_ATTRIB_FIRST_MATERIAL) index = ERROR_ATTRIB;
   DISPATCH_ATTR3FV( index, v );
}

static void GLAPIENTRY brw_VertexAttrib4fARB( GLuint index, GLfloat x,
					      GLfloat y, GLfloat z,
					      GLfloat w )
{
   if (index >= BRW_ATTRIB_FIRST_MATERIAL) index = ERROR_ATTRIB;
   DISPATCH_ATTR4F( index, x, y, z, w );
}

static void GLAPIENTRY brw_VertexAttrib4fvARB( GLuint index, 
					       const GLfloat *v )
{
   if (index >= BRW_ATTRIB_FIRST_MATERIAL) index = ERROR_ATTRIB;
   DISPATCH_ATTR4FV( index, v );
}


/* Install the generic versions of the 2nd level dispatch
 * functions.  Some of these have a codegen alternative.
 */
void brw_exec_vtx_generic_init( struct brw_exec_context *exec )
{
   GLvertexformat *vfmt = &exec->vtxfmt;

   vfmt->Color3f = brw_Color3f;
   vfmt->Color3fv = brw_Color3fv;
   vfmt->Color4f = brw_Color4f;
   vfmt->Color4fv = brw_Color4fv;
   vfmt->FogCoordfEXT = brw_FogCoordfEXT;
   vfmt->FogCoordfvEXT = brw_FogCoordfvEXT;
   vfmt->MultiTexCoord1fARB = brw_MultiTexCoord1f;
   vfmt->MultiTexCoord1fvARB = brw_MultiTexCoord1fv;
   vfmt->MultiTexCoord2fARB = brw_MultiTexCoord2f;
   vfmt->MultiTexCoord2fvARB = brw_MultiTexCoord2fv;
   vfmt->MultiTexCoord3fARB = brw_MultiTexCoord3f;
   vfmt->MultiTexCoord3fvARB = brw_MultiTexCoord3fv;
   vfmt->MultiTexCoord4fARB = brw_MultiTexCoord4f;
   vfmt->MultiTexCoord4fvARB = brw_MultiTexCoord4fv;
   vfmt->Normal3f = brw_Normal3f;
   vfmt->Normal3fv = brw_Normal3fv;
   vfmt->SecondaryColor3fEXT = brw_SecondaryColor3fEXT;
   vfmt->SecondaryColor3fvEXT = brw_SecondaryColor3fvEXT;
   vfmt->TexCoord1f = brw_TexCoord1f;
   vfmt->TexCoord1fv = brw_TexCoord1fv;
   vfmt->TexCoord2f = brw_TexCoord2f;
   vfmt->TexCoord2fv = brw_TexCoord2fv;
   vfmt->TexCoord3f = brw_TexCoord3f;
   vfmt->TexCoord3fv = brw_TexCoord3fv;
   vfmt->TexCoord4f = brw_TexCoord4f;
   vfmt->TexCoord4fv = brw_TexCoord4fv;
   vfmt->Vertex2f = brw_Vertex2f;
   vfmt->Vertex2fv = brw_Vertex2fv;
   vfmt->Vertex3f = brw_Vertex3f;
   vfmt->Vertex3fv = brw_Vertex3fv;
   vfmt->Vertex4f = brw_Vertex4f;
   vfmt->Vertex4fv = brw_Vertex4fv;
   vfmt->VertexAttrib1fNV = brw_VertexAttrib1fNV;
   vfmt->VertexAttrib1fvNV = brw_VertexAttrib1fvNV;
   vfmt->VertexAttrib2fNV = brw_VertexAttrib2fNV;
   vfmt->VertexAttrib2fvNV = brw_VertexAttrib2fvNV;
   vfmt->VertexAttrib3fNV = brw_VertexAttrib3fNV;
   vfmt->VertexAttrib3fvNV = brw_VertexAttrib3fvNV;
   vfmt->VertexAttrib4fNV = brw_VertexAttrib4fNV;
   vfmt->VertexAttrib4fvNV = brw_VertexAttrib4fvNV;
   vfmt->VertexAttrib1fARB = brw_VertexAttrib1fARB;
   vfmt->VertexAttrib1fvARB = brw_VertexAttrib1fvARB;
   vfmt->VertexAttrib2fARB = brw_VertexAttrib2fARB;
   vfmt->VertexAttrib2fvARB = brw_VertexAttrib2fvARB;
   vfmt->VertexAttrib3fARB = brw_VertexAttrib3fARB;
   vfmt->VertexAttrib3fvARB = brw_VertexAttrib3fvARB;
   vfmt->VertexAttrib4fARB = brw_VertexAttrib4fARB;
   vfmt->VertexAttrib4fvARB = brw_VertexAttrib4fvARB;
}
