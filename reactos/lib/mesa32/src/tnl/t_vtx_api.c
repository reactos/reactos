/* $XFree86$ */
/**************************************************************************

Copyright 2002 Tungsten Graphics Inc., Cedar Park, Texas.

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
TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
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


static void init_attrfv( TNLcontext *tnl );


/* Close off the last primitive, execute the buffer, restart the
 * primitive.  
 */
static void _tnl_wrap_buffers( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   

   if (tnl->vtx.prim_count == 0) {
      tnl->vtx.copied.nr = 0;
      tnl->vtx.counter = tnl->vtx.initial_counter;
      tnl->vtx.vbptr = tnl->vtx.buffer;
   }
   else {
      GLuint last_prim = tnl->vtx.prim[tnl->vtx.prim_count-1].mode;
      GLuint last_count = tnl->vtx.prim[tnl->vtx.prim_count-1].count;

      if (ctx->Driver.CurrentExecPrimitive != GL_POLYGON+1) {
	 GLint i = tnl->vtx.prim_count - 1;
	 assert(i >= 0);
	 tnl->vtx.prim[i].count = ((tnl->vtx.initial_counter - 
				    tnl->vtx.counter) - 
				   tnl->vtx.prim[i].start);
      }

      /* Execute the buffer and save copied vertices.
       */
      if (tnl->vtx.counter != tnl->vtx.initial_counter)
	 _tnl_flush_vtx( ctx );
      else {
	 tnl->vtx.prim_count = 0;
	 tnl->vtx.copied.nr = 0;
      }

      /* Emit a glBegin to start the new list.
       */
      assert(tnl->vtx.prim_count == 0);

      if (ctx->Driver.CurrentExecPrimitive != GL_POLYGON+1) {
	 tnl->vtx.prim[0].mode = ctx->Driver.CurrentExecPrimitive;
	 tnl->vtx.prim[0].start = 0;
	 tnl->vtx.prim[0].count = 0;
	 tnl->vtx.prim_count++;
      
	 if (tnl->vtx.copied.nr == last_count)
	    tnl->vtx.prim[0].mode |= last_prim & PRIM_BEGIN;
      }
   }
}


static void _tnl_wrap_filled_vertex( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLfloat *data = tnl->vtx.copied.buffer;
   GLuint i;

   /* Run pipeline on current vertices, copy wrapped vertices
    * to tnl->copied.
    */
   _tnl_wrap_buffers( ctx );
   
   /* Copy stored stored vertices to start of new list. 
    */
   assert(tnl->vtx.counter > tnl->vtx.copied.nr);

   for (i = 0 ; i < tnl->vtx.copied.nr ; i++) {
      _mesa_memcpy( tnl->vtx.vbptr, data, 
		    tnl->vtx.vertex_size * sizeof(GLfloat));
      tnl->vtx.vbptr += tnl->vtx.vertex_size;
      data += tnl->vtx.vertex_size;
      tnl->vtx.counter--;
   }

   tnl->vtx.copied.nr = 0;
}


/*
 * Copy the active vertex's values to the ctx->Current fields.
 */
static void _tnl_copy_to_current( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx); 
   GLuint i;

   for (i = _TNL_ATTRIB_POS+1 ; i <= _TNL_ATTRIB_INDEX ; i++)
      if (tnl->vtx.attrsz[i]) {
         /* Note: the tnl->vtx.current[i] pointers points to
          * the ctx->Current fields.  The first 16 or so, anyway.
          */
	 ASSIGN_4V( tnl->vtx.current[i], 0, 0, 0, 1 );
	 COPY_SZ_4V(tnl->vtx.current[i], 
		    tnl->vtx.attrsz[i], 
		    tnl->vtx.attrptr[i]);
      }

   /* Edgeflag requires special treatment:
    */
   if (tnl->vtx.attrsz[_TNL_ATTRIB_EDGEFLAG]) 
      ctx->Current.EdgeFlag = 
	 (tnl->vtx.attrptr[_TNL_ATTRIB_EDGEFLAG][0] == 1.0);


   /* Colormaterial -- this kindof sucks.
    */
   if (ctx->Light.ColorMaterialEnabled) {
      _mesa_update_color_material(ctx, ctx->Current.Attrib[VERT_ATTRIB_COLOR0]);
   }

   if (tnl->vtx.have_materials) {
      tnl->Driver.NotifyMaterialChange( ctx );
   }
         
   ctx->Driver.NeedFlush &= ~FLUSH_UPDATE_CURRENT;
}


static void _tnl_copy_from_current( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx); 
   GLint i;

   for (i = _TNL_ATTRIB_POS+1 ; i <= _TNL_ATTRIB_INDEX ; i++) 
      switch (tnl->vtx.attrsz[i]) {
      case 4: tnl->vtx.attrptr[i][3] = tnl->vtx.current[i][3];
      case 3: tnl->vtx.attrptr[i][2] = tnl->vtx.current[i][2];
      case 2: tnl->vtx.attrptr[i][1] = tnl->vtx.current[i][1];
      case 1: tnl->vtx.attrptr[i][0] = tnl->vtx.current[i][0];
	 break;
      }

   /* Edgeflag requires special treatment:
    */
   if (tnl->vtx.attrsz[_TNL_ATTRIB_EDGEFLAG]) 
      tnl->vtx.attrptr[_TNL_ATTRIB_EDGEFLAG][0] = 
	 (GLfloat)ctx->Current.EdgeFlag;


   ctx->Driver.NeedFlush |= FLUSH_UPDATE_CURRENT;
}


/* Flush existing data, set new attrib size, replay copied vertices.
 */ 
static void _tnl_wrap_upgrade_vertex( GLcontext *ctx, 
				      GLuint attr,
				      GLuint newsz )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx); 
   GLuint oldsz;
   GLuint i;
   GLfloat *tmp;
   GLint lastcount = tnl->vtx.initial_counter - tnl->vtx.counter;


   /* Run pipeline on current vertices, copy wrapped vertices
    * to tnl->vtx.copied.
    */
   _tnl_wrap_buffers( ctx );


   /* Do a COPY_TO_CURRENT to ensure back-copying works for the case
    * when the attribute already exists in the vertex and is having
    * its size increased.  
    */
   _tnl_copy_to_current( ctx );


   /* Heuristic: Attempt to isolate attributes received outside
    * begin/end so that they don't bloat the vertices.
    */
   if (ctx->Driver.CurrentExecPrimitive == PRIM_OUTSIDE_BEGIN_END &&
       tnl->vtx.attrsz[attr] == 0 
       && lastcount > 8
      ) {
      init_attrfv( tnl );
   }

   /* Fix up sizes:
    */
   oldsz = tnl->vtx.attrsz[attr];
   tnl->vtx.attrsz[attr] = newsz;

   tnl->vtx.vertex_size += newsz - oldsz;
   tnl->vtx.counter = MIN2( VERT_BUFFER_SIZE / tnl->vtx.vertex_size,
			    ctx->Const.MaxArrayLockSize );
   tnl->vtx.initial_counter = tnl->vtx.counter;
   tnl->vtx.vbptr = tnl->vtx.buffer;


   /* Recalculate all the attrptr[] values
    */
   for (i = 0, tmp = tnl->vtx.vertex ; i < _TNL_ATTRIB_MAX ; i++) {
      if (tnl->vtx.attrsz[i]) {
	 tnl->vtx.attrptr[i] = tmp;
	 tmp += tnl->vtx.attrsz[i];
      }
      else 
	 tnl->vtx.attrptr[i] = 0; /* will not be dereferenced */
   }

   /* Copy from current to repopulate the vertex with correct values.
    */
   _tnl_copy_from_current( ctx );

   /* Replay stored vertices to translate them
    * to new format here.
    *
    * -- No need to replay - just copy piecewise
    */
   if (tnl->vtx.copied.nr)
   {
      GLfloat *data = tnl->vtx.copied.buffer;
      GLfloat *dest = tnl->vtx.buffer;
      GLuint j;

      for (i = 0 ; i < tnl->vtx.copied.nr ; i++) {
	 for (j = 0 ; j < _TNL_ATTRIB_MAX ; j++) {
	    if (tnl->vtx.attrsz[j]) {
	       if (j == attr) {
		  COPY_SZ_4V( dest, newsz, tnl->vtx.current[j] );
		  COPY_SZ_4V( dest, oldsz, data );
		  data += oldsz;
		  dest += newsz;
	       }
	       else {
		  GLuint sz = tnl->vtx.attrsz[j];
		  COPY_SZ_4V( dest, sz, data );
		  dest += sz;
		  data += sz;
	       }
	    }
	 }
      }

      tnl->vtx.vbptr = dest;
      tnl->vtx.counter -= tnl->vtx.copied.nr;
      tnl->vtx.copied.nr = 0;
   }
}


static void _tnl_fixup_vertex( GLcontext *ctx, GLuint attr, GLuint sz )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   static const GLfloat id[4] = { 0, 0, 0, 1 };
   int i;

   if (tnl->vtx.attrsz[attr] < sz) {
      /* New size is larger.  Need to flush existing vertices and get
       * an enlarged vertex format.
       */
      _tnl_wrap_upgrade_vertex( ctx, attr, sz );
   }
   else if (tnl->vtx.attrsz[attr] > sz) {
      /* New size is smaller - just need to fill in some
       * zeros.  Don't need to flush or wrap.
       */
      for (i = sz ; i <= tnl->vtx.attrsz[attr] ; i++)
	 tnl->vtx.attrptr[attr][i-1] = id[i-1];
   }
}




/* Helper function for 'CHOOSE' macro.  Do what's necessary when an
 * entrypoint is called for the first time.
 */
static void do_choose( GLuint attr, GLuint sz, 
			void (*fallback_attr_func)( const GLfloat *),
			void (*choose1)( const GLfloat *),
			void (*choose2)( const GLfloat *),
			void (*choose3)( const GLfloat *),
			void (*choose4)( const GLfloat *),
			const GLfloat *v )
{ 
   GET_CURRENT_CONTEXT( ctx ); 
   TNLcontext *tnl = TNL_CONTEXT(ctx); 

   if (tnl->vtx.attrsz[attr] != sz)
      _tnl_fixup_vertex( ctx, attr, sz );
 
   /* Does this belong here?  Necessitates resetting vtxfmt on each
    * flush (otherwise flags won't get reset afterwards).
    */
   if (attr == 0)
      ctx->Driver.NeedFlush |= FLUSH_STORED_VERTICES;
   else
      ctx->Driver.NeedFlush |= FLUSH_UPDATE_CURRENT;

   /* Reset any active pointers for this attribute 
    */
   tnl->vtx.tabfv[attr][0] = choose1;
   tnl->vtx.tabfv[attr][1] = choose2;
   tnl->vtx.tabfv[attr][2] = choose3;
   tnl->vtx.tabfv[attr][3] = choose4;

   /* Update the secondary dispatch table with the new function
    */
   tnl->vtx.tabfv[attr][sz-1] = fallback_attr_func;

   (*fallback_attr_func)(v);
}


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
static void choose_##ATTR##_##N( const GLfloat *v );	\
							\
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

#define CHOOSE( ATTR, N )				\
static void choose_##ATTR##_##N( const GLfloat *v )	\
{							\
   do_choose(ATTR, N,					\
	     attrib_##ATTR##_##N,			\
	     choose_##ATTR##_1,				\
	     choose_##ATTR##_2,				\
	     choose_##ATTR##_3,				\
	     choose_##ATTR##_4,				\
	     v );					\
}

#define INIT(ATTR)					\
static void init_##ATTR( TNLcontext *tnl )		\
{							\
   tnl->vtx.tabfv[ATTR][0] = choose_##ATTR##_1;		\
   tnl->vtx.tabfv[ATTR][1] = choose_##ATTR##_2;		\
   tnl->vtx.tabfv[ATTR][2] = choose_##ATTR##_3; 	\
   tnl->vtx.tabfv[ATTR][3] = choose_##ATTR##_4;		\
}
   

#define ATTRS( ATTRIB )				\
   ATTRFV( ATTRIB, 1 )				\
   ATTRFV( ATTRIB, 2 )				\
   ATTRFV( ATTRIB, 3 )				\
   ATTRFV( ATTRIB, 4 )				\
   CHOOSE( ATTRIB, 1 )				\
   CHOOSE( ATTRIB, 2 )				\
   CHOOSE( ATTRIB, 3 )				\
   CHOOSE( ATTRIB, 4 )				\
   INIT( ATTRIB )				\


/* Generate a lot of functions.  These are the actual worker
 * functions, which are equivalent to those generated via codegen
 * elsewhere.
 */
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

static void init_attrfv( TNLcontext *tnl )
{   
   if (tnl->vtx.vertex_size) {
      GLuint i;
      
      init_0( tnl );
      init_1( tnl );
      init_2( tnl );
      init_3( tnl );
      init_4( tnl );
      init_5( tnl );
      init_6( tnl );
      init_7( tnl );
      init_8( tnl );
      init_9( tnl );
      init_10( tnl );
      init_11( tnl );
      init_12( tnl );
      init_13( tnl );
      init_14( tnl );
      init_15( tnl );

      for (i = 0 ; i < _TNL_ATTRIB_MAX ; i++) 
	 tnl->vtx.attrsz[i] = 0;

      tnl->vtx.vertex_size = 0;
      tnl->vtx.have_materials = 0;
   }
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


static void enum_error( void )
{
   GET_CURRENT_CONTEXT( ctx );
   _mesa_error( ctx, GL_INVALID_ENUM, "glVertexAttrib" );
}

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

static void GLAPIENTRY _tnl_Vertex4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
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

static void GLAPIENTRY _tnl_TexCoord4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
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

static void GLAPIENTRY _tnl_Color4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   DISPATCH_ATTR4F( _TNL_ATTRIB_COLOR0, x, y, z, w );
}

static void GLAPIENTRY _tnl_Color4fv( const GLfloat *v )
{
   DISPATCH_ATTR4FV( _TNL_ATTRIB_COLOR0, v );
}

static void GLAPIENTRY _tnl_SecondaryColor3fEXT( GLfloat x, GLfloat y, GLfloat z )
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

static void GLAPIENTRY _tnl_MultiTexCoord1fv( GLenum target, const GLfloat *v )
{
   GLuint attr = (target & 0x7) + _TNL_ATTRIB_TEX0;
   DISPATCH_ATTR1FV( attr, v );
}

static void GLAPIENTRY _tnl_MultiTexCoord2f( GLenum target, GLfloat x, GLfloat y )
{
   GLuint attr = (target & 0x7) + _TNL_ATTRIB_TEX0;
   DISPATCH_ATTR2F( attr, x, y );
}

static void GLAPIENTRY _tnl_MultiTexCoord2fv( GLenum target, const GLfloat *v )
{
   GLuint attr = (target & 0x7) + _TNL_ATTRIB_TEX0;
   DISPATCH_ATTR2FV( attr, v );
}

static void GLAPIENTRY _tnl_MultiTexCoord3f( GLenum target, GLfloat x, GLfloat y,
				    GLfloat z)
{
   GLuint attr = (target & 0x7) + _TNL_ATTRIB_TEX0;
   DISPATCH_ATTR3F( attr, x, y, z );
}

static void GLAPIENTRY _tnl_MultiTexCoord3fv( GLenum target, const GLfloat *v )
{
   GLuint attr = (target & 0x7) + _TNL_ATTRIB_TEX0;
   DISPATCH_ATTR3FV( attr, v );
}

static void GLAPIENTRY _tnl_MultiTexCoord4f( GLenum target, GLfloat x, GLfloat y,
				    GLfloat z, GLfloat w )
{
   GLuint attr = (target & 0x7) + _TNL_ATTRIB_TEX0;
   DISPATCH_ATTR4F( attr, x, y, z, w );
}

static void GLAPIENTRY _tnl_MultiTexCoord4fv( GLenum target, const GLfloat *v )
{
   GLuint attr = (target & 0x7) + _TNL_ATTRIB_TEX0;
   DISPATCH_ATTR4FV( attr, v );
}

static void GLAPIENTRY _tnl_VertexAttrib1fNV( GLuint index, GLfloat x )
{
   if (index < VERT_ATTRIB_MAX)
      DISPATCH_ATTR1F( index, x );
   else
      enum_error(); 
}

static void GLAPIENTRY _tnl_VertexAttrib1fvNV( GLuint index, const GLfloat *v )
{
   if (index < VERT_ATTRIB_MAX)
      DISPATCH_ATTR1FV( index, v );
   else
      enum_error();
}

static void GLAPIENTRY _tnl_VertexAttrib2fNV( GLuint index, GLfloat x, GLfloat y )
{
   if (index < VERT_ATTRIB_MAX)
      DISPATCH_ATTR2F( index, x, y );
   else
      enum_error();
}

static void GLAPIENTRY _tnl_VertexAttrib2fvNV( GLuint index, const GLfloat *v )
{
   if (index < VERT_ATTRIB_MAX)
      DISPATCH_ATTR2FV( index, v );
   else
      enum_error();
}

static void GLAPIENTRY _tnl_VertexAttrib3fNV( GLuint index, GLfloat x, GLfloat y, 
				  GLfloat z )
{
   if (index < VERT_ATTRIB_MAX)
      DISPATCH_ATTR3F( index, x, y, z );
   else
      enum_error();
}

static void GLAPIENTRY _tnl_VertexAttrib3fvNV( GLuint index, const GLfloat *v )
{
   if (index < VERT_ATTRIB_MAX)
      DISPATCH_ATTR3FV( index, v );
   else
      enum_error();
}

static void GLAPIENTRY _tnl_VertexAttrib4fNV( GLuint index, GLfloat x, GLfloat y,
				  GLfloat z, GLfloat w )
{
   if (index < VERT_ATTRIB_MAX)
      DISPATCH_ATTR4F( index, x, y, z, w );
   else
      enum_error();
}

static void GLAPIENTRY _tnl_VertexAttrib4fvNV( GLuint index, const GLfloat *v )
{
   if (index < VERT_ATTRIB_MAX)
      DISPATCH_ATTR4FV( index, v );
   else
      enum_error();
}


/* Materials:  
 * 
 * These are treated as per-vertex attributes, at indices above where
 * the NV_vertex_program leaves off.  There are a lot of good things
 * about treating materials this way.  
 *
 * However: I don't want to double the number of generated functions
 * just to cope with this, so I unroll the 'C' varients of CHOOSE and
 * ATTRF into this function, and dispense with codegen and
 * second-level dispatch.
 *
 * There is no aliasing of material attributes with other entrypoints.
 */
#define MAT_ATTR( A, N, params )			\
do {							\
   if (tnl->vtx.attrsz[A] != N) {			\
      _tnl_fixup_vertex( ctx, A, N );			\
      tnl->vtx.have_materials = GL_TRUE;		\
   }							\
							\
   {							\
      GLfloat *dest = tnl->vtx.attrptr[A];		\
      if (N>0) dest[0] = params[0];			\
      if (N>1) dest[1] = params[1];			\
      if (N>2) dest[2] = params[2];			\
      if (N>3) dest[3] = params[3];			\
      ctx->Driver.NeedFlush |= FLUSH_UPDATE_CURRENT;	\
   }							\
} while (0)


#define MAT( ATTR, N, face, params )			\
do {							\
   if (face != GL_BACK)					\
      MAT_ATTR( ATTR, N, params ); /* front */		\
   if (face != GL_FRONT)				\
      MAT_ATTR( ATTR + 1, N, params ); /* back */	\
} while (0)


/* NOTE: Have to remove/deal-with colormaterial crossovers, probably
 * later on - in the meantime just store everything.  
 */
static void GLAPIENTRY _tnl_Materialfv( GLenum face, GLenum pname, 
			       const GLfloat *params )
{
   GET_CURRENT_CONTEXT( ctx ); 
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   switch (face) {
   case GL_FRONT:
   case GL_BACK:
   case GL_FRONT_AND_BACK:
      break;
      
   default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glMaterialfv" );
      return;
   }

   switch (pname) {
   case GL_EMISSION:
      MAT( _TNL_ATTRIB_MAT_FRONT_EMISSION, 4, face, params );
      break;
   case GL_AMBIENT:
      MAT( _TNL_ATTRIB_MAT_FRONT_AMBIENT, 4, face, params );
      break;
   case GL_DIFFUSE:
      MAT( _TNL_ATTRIB_MAT_FRONT_DIFFUSE, 4, face, params );
      break;
   case GL_SPECULAR:
      MAT( _TNL_ATTRIB_MAT_FRONT_SPECULAR, 4, face, params );
      break;
   case GL_SHININESS:
      MAT( _TNL_ATTRIB_MAT_FRONT_SHININESS, 1, face, params );
      break;
   case GL_COLOR_INDEXES:
      MAT( _TNL_ATTRIB_MAT_FRONT_INDEXES, 3, face, params );
      break;
   case GL_AMBIENT_AND_DIFFUSE:
      MAT( _TNL_ATTRIB_MAT_FRONT_AMBIENT, 4, face, params );
      MAT( _TNL_ATTRIB_MAT_FRONT_DIFFUSE, 4, face, params );
      break;
   default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glMaterialfv" );
      return;
   }
}


#define IDX_ATTR( A, IDX )				\
do {							\
   GET_CURRENT_CONTEXT( ctx );				\
   TNLcontext *tnl = TNL_CONTEXT(ctx);			\
							\
   if (tnl->vtx.attrsz[A] != 1) {			\
      _tnl_fixup_vertex( ctx, A, 1 );			\
   }							\
							\
   {							\
      GLfloat *dest = tnl->vtx.attrptr[A];		\
      dest[0] = IDX;				\
      ctx->Driver.NeedFlush |= FLUSH_UPDATE_CURRENT;	\
   }							\
} while (0)


static void GLAPIENTRY _tnl_EdgeFlag( GLboolean b )
{
   IDX_ATTR( _TNL_ATTRIB_EDGEFLAG, (GLfloat)b );
}

static void GLAPIENTRY _tnl_EdgeFlagv( const GLboolean *v )
{
   IDX_ATTR( _TNL_ATTRIB_EDGEFLAG, (GLfloat)v[0] );
}

static void GLAPIENTRY _tnl_Indexf( GLfloat f )
{
   IDX_ATTR( _TNL_ATTRIB_INDEX, f );
}

static void GLAPIENTRY _tnl_Indexfv( const GLfloat *v )
{
   IDX_ATTR( _TNL_ATTRIB_INDEX, v[0] );
}

/* Eval
 */
static void GLAPIENTRY _tnl_EvalCoord1f( GLfloat u )
{
   GET_CURRENT_CONTEXT( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   /* TODO: use a CHOOSE() function for this: */
   {
      GLint i;
      if (tnl->vtx.eval.new_state) 
	 _tnl_update_eval( ctx );

      for (i = 0 ; i <= _TNL_ATTRIB_INDEX ; i++) {
	 if (tnl->vtx.eval.map1[i].map) 
	    if (tnl->vtx.attrsz[i] < tnl->vtx.eval.map1[i].sz)
	       _tnl_fixup_vertex( ctx, i, tnl->vtx.eval.map1[i].sz );
      }
   }


   _mesa_memcpy( tnl->vtx.copied.buffer, tnl->vtx.vertex, 
                 tnl->vtx.vertex_size * sizeof(GLfloat));

   _tnl_do_EvalCoord1f( ctx, u );

   _mesa_memcpy( tnl->vtx.vertex, tnl->vtx.copied.buffer,
                 tnl->vtx.vertex_size * sizeof(GLfloat));
}

static void GLAPIENTRY _tnl_EvalCoord2f( GLfloat u, GLfloat v )
{
   GET_CURRENT_CONTEXT( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   /* TODO: use a CHOOSE() function for this: */
   {
      GLint i;
      if (tnl->vtx.eval.new_state) 
	 _tnl_update_eval( ctx );

      for (i = 0 ; i <= _TNL_ATTRIB_INDEX ; i++) {
	 if (tnl->vtx.eval.map2[i].map) 
	    if (tnl->vtx.attrsz[i] < tnl->vtx.eval.map2[i].sz)
	       _tnl_fixup_vertex( ctx, i, tnl->vtx.eval.map2[i].sz );
      }

      if (ctx->Eval.AutoNormal) 
	 if (tnl->vtx.attrsz[_TNL_ATTRIB_NORMAL] < 3)
	    _tnl_fixup_vertex( ctx, _TNL_ATTRIB_NORMAL, 3 );
   }

   _mesa_memcpy( tnl->vtx.copied.buffer, tnl->vtx.vertex, 
                 tnl->vtx.vertex_size * sizeof(GLfloat));

   _tnl_do_EvalCoord2f( ctx, u, v );

   _mesa_memcpy( tnl->vtx.vertex, tnl->vtx.copied.buffer, 
                 tnl->vtx.vertex_size * sizeof(GLfloat));
}

static void GLAPIENTRY _tnl_EvalCoord1fv( const GLfloat *u )
{
   _tnl_EvalCoord1f( u[0] );
}

static void GLAPIENTRY _tnl_EvalCoord2fv( const GLfloat *u )
{
   _tnl_EvalCoord2f( u[0], u[1] );
}

static void GLAPIENTRY _tnl_EvalPoint1( GLint i )
{
   GET_CURRENT_CONTEXT( ctx );
   GLfloat du = ((ctx->Eval.MapGrid1u2 - ctx->Eval.MapGrid1u1) /
		 (GLfloat) ctx->Eval.MapGrid1un);
   GLfloat u = i * du + ctx->Eval.MapGrid1u1;

   _tnl_EvalCoord1f( u );
}


static void GLAPIENTRY _tnl_EvalPoint2( GLint i, GLint j )
{
   GET_CURRENT_CONTEXT( ctx );
   GLfloat du = ((ctx->Eval.MapGrid2u2 - ctx->Eval.MapGrid2u1) / 
		 (GLfloat) ctx->Eval.MapGrid2un);
   GLfloat dv = ((ctx->Eval.MapGrid2v2 - ctx->Eval.MapGrid2v1) / 
		 (GLfloat) ctx->Eval.MapGrid2vn);
   GLfloat u = i * du + ctx->Eval.MapGrid2u1;
   GLfloat v = j * dv + ctx->Eval.MapGrid2v1;

   _tnl_EvalCoord2f( u, v );
}


/* Build a list of primitives on the fly.  Keep
 * ctx->Driver.CurrentExecPrimitive uptodate as well.
 */
static void GLAPIENTRY _tnl_Begin( GLenum mode )
{
   GET_CURRENT_CONTEXT( ctx ); 

   if ((ctx->VertexProgram.Enabled
        && !ctx->VertexProgram.Current->Instructions) ||
       (ctx->FragmentProgram.Enabled
        && !ctx->FragmentProgram.Current->Instructions)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "glBegin (invalid vertex/fragment program)");
      return;
   }

   if (ctx->Driver.CurrentExecPrimitive == GL_POLYGON+1) {
      TNLcontext *tnl = TNL_CONTEXT(ctx); 
      int i;

      if (ctx->NewState) {
	 _mesa_update_state( ctx );
	 if (!(tnl->Driver.NotifyBegin && tnl->Driver.NotifyBegin( ctx, mode )))
	     ctx->Exec->Begin(mode);
	 return;
      }

      /* Heuristic: attempt to isolate attributes occuring outside
       * begin/end pairs.
       */
      if (tnl->vtx.vertex_size && !tnl->vtx.attrsz[0]) 
	 _tnl_FlushVertices( ctx, ~0 );

      i = tnl->vtx.prim_count++;
      tnl->vtx.prim[i].mode = mode | PRIM_BEGIN;
      tnl->vtx.prim[i].start = tnl->vtx.initial_counter - tnl->vtx.counter;
      tnl->vtx.prim[i].count = 0;

      ctx->Driver.CurrentExecPrimitive = mode;
   }
   else 
      _mesa_error( ctx, GL_INVALID_OPERATION, "glBegin" );
      
}

static void GLAPIENTRY _tnl_End( void )
{
   GET_CURRENT_CONTEXT( ctx ); 

   if (ctx->Driver.CurrentExecPrimitive != GL_POLYGON+1) {
      TNLcontext *tnl = TNL_CONTEXT(ctx); 
      int idx = tnl->vtx.initial_counter - tnl->vtx.counter;
      int i = tnl->vtx.prim_count - 1;

      tnl->vtx.prim[i].mode |= PRIM_END; 
      tnl->vtx.prim[i].count = idx - tnl->vtx.prim[i].start;

      ctx->Driver.CurrentExecPrimitive = GL_POLYGON+1;

      /* Two choices which effect the way vertex attributes are
       * carried over (or not) between adjacent primitives.
       */
#if 0
      if (tnl->vtx.prim_count == TNL_MAX_PRIM) 
	 _tnl_FlushVertices( ctx, ~0 );
#else
      if (tnl->vtx.prim_count == TNL_MAX_PRIM)
	 _tnl_flush_vtx( ctx );	
#endif

   }
   else 
      _mesa_error( ctx, GL_INVALID_OPERATION, "glEnd" );
}


static void _tnl_exec_vtxfmt_init( GLcontext *ctx )
{
   GLvertexformat *vfmt = &(TNL_CONTEXT(ctx)->exec_vtxfmt);
   vfmt->ArrayElement = _ae_loopback_array_elt;	        /* generic helper */
   vfmt->Begin = _tnl_Begin;
   vfmt->CallList = _mesa_CallList;
   vfmt->CallLists = _mesa_CallLists;
   vfmt->Color3f = _tnl_Color3f;
   vfmt->Color3fv = _tnl_Color3fv;
   vfmt->Color4f = _tnl_Color4f;
   vfmt->Color4fv = _tnl_Color4fv;
   vfmt->EdgeFlag = _tnl_EdgeFlag;
   vfmt->EdgeFlagv = _tnl_EdgeFlagv;
   vfmt->End = _tnl_End;
   vfmt->EvalCoord1f = _tnl_EvalCoord1f;
   vfmt->EvalCoord1fv = _tnl_EvalCoord1fv;
   vfmt->EvalCoord2f = _tnl_EvalCoord2f;
   vfmt->EvalCoord2fv = _tnl_EvalCoord2fv;
   vfmt->EvalPoint1 = _tnl_EvalPoint1;
   vfmt->EvalPoint2 = _tnl_EvalPoint2;
   vfmt->FogCoordfEXT = _tnl_FogCoordfEXT;
   vfmt->FogCoordfvEXT = _tnl_FogCoordfvEXT;
   vfmt->Indexf = _tnl_Indexf;
   vfmt->Indexfv = _tnl_Indexfv;
   vfmt->Materialfv = _tnl_Materialfv;
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

   vfmt->Rectf = _mesa_noop_Rectf;
   vfmt->EvalMesh1 = _mesa_noop_EvalMesh1;
   vfmt->EvalMesh2 = _mesa_noop_EvalMesh2;
}



void _tnl_FlushVertices( GLcontext *ctx, GLuint flags )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   if (ctx->Driver.CurrentExecPrimitive != PRIM_OUTSIDE_BEGIN_END)
      return;

   if (tnl->vtx.counter != tnl->vtx.initial_counter) {
      _tnl_flush_vtx( ctx );
   }

   {
      _tnl_copy_to_current( ctx );

      /* reset attrfv table
       */
      init_attrfv( tnl );
      flags |= FLUSH_UPDATE_CURRENT;
   }

   ctx->Driver.NeedFlush = 0;
}


static void _tnl_current_init( GLcontext *ctx ) 
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLint i;

   /* setup the pointers for the typical 16 vertex attributes */
   for (i = 0; i < VERT_ATTRIB_MAX; i++) 
      tnl->vtx.current[i] = ctx->Current.Attrib[i];

   /* setup pointers for the 12 material attributes */
   for (i = 0; i < MAT_ATTRIB_MAX; i++)
      tnl->vtx.current[_TNL_ATTRIB_MAT_FRONT_AMBIENT + i] = 
	 ctx->Light.Material.Attrib[i];

   tnl->vtx.current[_TNL_ATTRIB_INDEX] = &ctx->Current.Index;
}


void _tnl_vtx_init( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx); 
   struct tnl_vertex_arrays *tmp = &tnl->vtx_inputs;
   GLuint i;

   for (i = 0; i < _TNL_ATTRIB_INDEX; i++)
      _mesa_vector4f_init( &tmp->Attribs[i], 0, 0);

   _tnl_current_init( ctx );
   _tnl_exec_vtxfmt_init( ctx );

   _mesa_install_exec_vtxfmt( ctx, &tnl->exec_vtxfmt );
   tnl->vtx.vertex_size = 1; init_attrfv( tnl );
}



void _tnl_vtx_destroy( GLcontext *ctx )
{
}

