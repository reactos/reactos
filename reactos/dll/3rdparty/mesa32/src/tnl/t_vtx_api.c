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
#include "simple_list.h"

#include "dispatch.h"

static void reset_attrfv( TNLcontext *tnl );

static tnl_attrfv_func choose[_TNL_MAX_ATTR_CODEGEN+1][4]; /* +1 for ERROR_ATTRIB */
static tnl_attrfv_func generic_attr_func[_TNL_MAX_ATTR_CODEGEN][4];


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
      GLuint last_count;

      if (ctx->Driver.CurrentExecPrimitive != GL_POLYGON+1) {
	 GLint i = tnl->vtx.prim_count - 1;
	 assert(i >= 0);
	 tnl->vtx.prim[i].count = ((tnl->vtx.initial_counter -
				    tnl->vtx.counter) -
				   tnl->vtx.prim[i].start);
      }

      last_count = tnl->vtx.prim[tnl->vtx.prim_count-1].count;

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


/* Deal with buffer wrapping where provoked by the vertex buffer
 * filling up, as opposed to upgrade_vertex().
 *
 * Make it GLAPIENTRY, so we can tail from the codegen'ed Vertex*fv
 */
void GLAPIENTRY _tnl_wrap_filled_vertex( GLcontext *ctx )
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

   for (i = _TNL_ATTRIB_POS+1 ; i < _TNL_ATTRIB_INDEX ; i++) {
      if (tnl->vtx.attrsz[i]) {
         /* Note: the tnl->vtx.current[i] pointers points to
          * the ctx->Current fields.  The first 16 or so, anyway.
          */
	 COPY_CLEAN_4V(tnl->vtx.current[i],
		    tnl->vtx.attrsz[i],
		    tnl->vtx.attrptr[i]);
      }
   }

   /* color index is special (it's not a float[4] so COPY_CLEAN_4V above
    * will trash adjacent memory!)
    */
   if (tnl->vtx.attrsz[_TNL_ATTRIB_INDEX]) {
      ctx->Current.Index = tnl->vtx.attrptr[_TNL_ATTRIB_INDEX][0];
   }

   /* Edgeflag requires additional treatment:
    */
   if (tnl->vtx.attrsz[_TNL_ATTRIB_EDGEFLAG]) {
      ctx->Current.EdgeFlag =
	 (tnl->vtx.CurrentFloatEdgeFlag == 1.0);
   }

   /* Colormaterial -- this kindof sucks.
    */
   if (ctx->Light.ColorMaterialEnabled) {
      _mesa_update_color_material(ctx,
				  ctx->Current.Attrib[VERT_ATTRIB_COLOR0]);
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

   /* Edgeflag requires additional treatment:
    */
   tnl->vtx.CurrentFloatEdgeFlag =
      (GLfloat)ctx->Current.EdgeFlag;

   for (i = _TNL_ATTRIB_POS+1 ; i <= _TNL_ATTRIB_MAX ; i++)
      switch (tnl->vtx.attrsz[i]) {
      case 4: tnl->vtx.attrptr[i][3] = tnl->vtx.current[i][3];
      case 3: tnl->vtx.attrptr[i][2] = tnl->vtx.current[i][2];
      case 2: tnl->vtx.attrptr[i][1] = tnl->vtx.current[i][1];
      case 1: tnl->vtx.attrptr[i][0] = tnl->vtx.current[i][0];
	 break;
      }

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
       tnl->vtx.attrsz[attr] == 0 &&
       lastcount > 8 &&
       tnl->vtx.vertex_size) {
      reset_attrfv( tnl );
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
	 tnl->vtx.attrptr[i] = NULL; /* will not be dereferenced */
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
		  if (oldsz) {
		     COPY_CLEAN_4V( dest, oldsz, data );
		     data += oldsz;
		     dest += newsz;
		  } else {
		     COPY_SZ_4V( dest, newsz, tnl->vtx.current[j] );
		     dest += newsz;
		  }
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

   /* For codegen - attrptr's may have changed, so need to redo
    * codegen.  Might be a reasonable place to try & detect attributes
    * in the vertex which aren't being submitted any more.
    */
   for (i = 0 ; i < _TNL_ATTRIB_MAX ; i++)
      if (tnl->vtx.attrsz[i]) {
	 GLuint j = tnl->vtx.attrsz[i] - 1;

	 if (i < _TNL_MAX_ATTR_CODEGEN)
	    tnl->vtx.tabfv[i][j] = choose[i][j];
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

   /* Does setting NeedFlush belong here?  Necessitates resetting
    * vtxfmt on each flush (otherwise flags won't get reset
    * afterwards).
    */
   if (attr == 0)
      ctx->Driver.NeedFlush |= FLUSH_STORED_VERTICES;
   else
      ctx->Driver.NeedFlush |= FLUSH_UPDATE_CURRENT;
}

#ifdef USE_X86_ASM

static struct _tnl_dynfn *lookup( struct _tnl_dynfn *l, GLuint key )
{
   struct _tnl_dynfn *f;

   foreach( f, l ) {
      if (f->key == key)
	 return f;
   }

   return NULL;
}


static tnl_attrfv_func do_codegen( GLcontext *ctx, GLuint attr, GLuint sz )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct _tnl_dynfn *dfn = NULL;

   if (attr == 0) {
      GLuint key = tnl->vtx.vertex_size;

      dfn = lookup( &tnl->vtx.cache.Vertex[sz-1], key );

      if (!dfn)
	 dfn = tnl->vtx.gen.Vertex[sz-1]( ctx, key );
   }
   else {
      GLuint key = (GLuint) tnl->vtx.attrptr[attr];

      dfn = lookup( &tnl->vtx.cache.Attribute[sz-1], key );

      if (!dfn)
	 dfn = tnl->vtx.gen.Attribute[sz-1]( ctx, key );
   }

   if (dfn)
      return *(tnl_attrfv_func *) &dfn->code;
   else
      return NULL;
}

#endif /* USE_X86_ASM */

/* Helper function for 'CHOOSE' macro.  Do what's necessary when an
 * entrypoint is called for the first time.
 */

static tnl_attrfv_func do_choose( GLuint attr, GLuint sz )
{
   GET_CURRENT_CONTEXT( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint oldsz = tnl->vtx.attrsz[attr];

   assert(attr < _TNL_MAX_ATTR_CODEGEN);

   if (oldsz != sz) {
      /* Reset any active pointers for this attribute
       */
      if (oldsz)
	 tnl->vtx.tabfv[attr][oldsz-1] = choose[attr][oldsz-1];

      _tnl_fixup_vertex( ctx, attr, sz );

   }


   /* Try to use codegen:
    */
#ifdef USE_X86_ASM
   if (tnl->AllowCodegen)
      tnl->vtx.tabfv[attr][sz-1] = do_codegen( ctx, attr, sz );
   else
#endif
      tnl->vtx.tabfv[attr][sz-1] = NULL;

   /* Else use generic version:
    */
   if (!tnl->vtx.tabfv[attr][sz-1])
      tnl->vtx.tabfv[attr][sz-1] = generic_attr_func[attr][sz-1];

   return tnl->vtx.tabfv[attr][sz-1];
}



#define CHOOSE( ATTR, N )				\
static void choose_##ATTR##_##N( const GLfloat *v )	\
{							\
   tnl_attrfv_func f = do_choose(ATTR, N);			\
   f( v );						\
}

#define CHOOSERS( ATTRIB ) \
   CHOOSE( ATTRIB, 1 )				\
   CHOOSE( ATTRIB, 2 )				\
   CHOOSE( ATTRIB, 3 )				\
   CHOOSE( ATTRIB, 4 )				\


#define INIT_CHOOSERS(ATTR)				\
   choose[ATTR][0] = choose_##ATTR##_1;				\
   choose[ATTR][1] = choose_##ATTR##_2;				\
   choose[ATTR][2] = choose_##ATTR##_3;				\
   choose[ATTR][3] = choose_##ATTR##_4;

CHOOSERS( 0 )
CHOOSERS( 1 )
CHOOSERS( 2 )
CHOOSERS( 3 )
CHOOSERS( 4 )
CHOOSERS( 5 )
CHOOSERS( 6 )
CHOOSERS( 7 )
CHOOSERS( 8 )
CHOOSERS( 9 )
CHOOSERS( 10 )
CHOOSERS( 11 )
CHOOSERS( 12 )
CHOOSERS( 13 )
CHOOSERS( 14 )
CHOOSERS( 15 )

static void error_attrib( const GLfloat *unused )
{
   GET_CURRENT_CONTEXT( ctx );
   (void) unused;
   _mesa_error( ctx, GL_INVALID_ENUM, "glVertexAttrib" );
}



static void reset_attrfv( TNLcontext *tnl )
{
   GLuint i;

   for (i = 0 ; i < _TNL_ATTRIB_MAX ; i++)
      if (tnl->vtx.attrsz[i]) {
	 GLint j = tnl->vtx.attrsz[i] - 1;
	 tnl->vtx.attrsz[i] = 0;

	 if (i < _TNL_MAX_ATTR_CODEGEN) {
            while (j >= 0) {
	       tnl->vtx.tabfv[i][j] = choose[i][j];
               j--;
            }
         }
      }

   tnl->vtx.vertex_size = 0;
   tnl->vtx.have_materials = 0;
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
#define OTHER_ATTR( A, N, params )		\
do {							\
   if (tnl->vtx.attrsz[A] != N) {			\
      _tnl_fixup_vertex( ctx, A, N );			\
   }							\
							\
   {							\
      GLfloat *dest = tnl->vtx.attrptr[A];		\
      if (N>0) dest[0] = (params)[0];			\
      if (N>1) dest[1] = (params)[1];			\
      if (N>2) dest[2] = (params)[2];			\
      if (N>3) dest[3] = (params)[3];			\
   }							\
} while (0)


#define MAT( ATTR, N, face, params )				\
do {								\
   if (face != GL_BACK)						\
      OTHER_ATTR( ATTR, N, params ); /* front */	\
   if (face != GL_FRONT)					\
      OTHER_ATTR( ATTR + 1, N, params ); /* back */	\
} while (0)


/* Colormaterial is dealt with later on.
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

   tnl->vtx.have_materials = GL_TRUE;
}


static void GLAPIENTRY _tnl_EdgeFlag( GLboolean b )
{
   GET_CURRENT_CONTEXT( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLfloat f = (GLfloat)b;

   OTHER_ATTR( _TNL_ATTRIB_EDGEFLAG, 1, &f );
}

static void GLAPIENTRY _tnl_EdgeFlagv( const GLboolean *v )
{
   GET_CURRENT_CONTEXT( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLfloat f = (GLfloat)v[0];

   OTHER_ATTR( _TNL_ATTRIB_EDGEFLAG, 1, &f );
}

static void GLAPIENTRY _tnl_Indexf( GLfloat f )
{
   GET_CURRENT_CONTEXT( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   OTHER_ATTR( _TNL_ATTRIB_INDEX, 1, &f );
}

static void GLAPIENTRY _tnl_Indexfv( const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   OTHER_ATTR( _TNL_ATTRIB_INDEX, 1, v );
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
	    if (tnl->vtx.attrsz[i] != tnl->vtx.eval.map1[i].sz)
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
	    if (tnl->vtx.attrsz[i] != tnl->vtx.eval.map2[i].sz)
	       _tnl_fixup_vertex( ctx, i, tnl->vtx.eval.map2[i].sz );
      }

      if (ctx->Eval.AutoNormal)
	 if (tnl->vtx.attrsz[_TNL_ATTRIB_NORMAL] != 3)
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

   if (ctx->Driver.CurrentExecPrimitive == GL_POLYGON+1) {
      TNLcontext *tnl = TNL_CONTEXT(ctx);
      int i;

      if (ctx->NewState) {
	 _mesa_update_state( ctx );

         if ((ctx->VertexProgram.Enabled && !ctx->VertexProgram._Enabled) ||
            (ctx->FragmentProgram.Enabled && !ctx->FragmentProgram._Enabled)) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glBegin (invalid vertex/fragment program)");
            return;
         }

	 if (!(tnl->Driver.NotifyBegin &&
	       tnl->Driver.NotifyBegin( ctx, mode )))
	     CALL_Begin(ctx->Exec, (mode));
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
   vfmt->EdgeFlag = _tnl_EdgeFlag;
   vfmt->EdgeFlagv = _tnl_EdgeFlagv;
   vfmt->End = _tnl_End;
   vfmt->EvalCoord1f = _tnl_EvalCoord1f;
   vfmt->EvalCoord1fv = _tnl_EvalCoord1fv;
   vfmt->EvalCoord2f = _tnl_EvalCoord2f;
   vfmt->EvalCoord2fv = _tnl_EvalCoord2fv;
   vfmt->EvalPoint1 = _tnl_EvalPoint1;
   vfmt->EvalPoint2 = _tnl_EvalPoint2;
   vfmt->Indexf = _tnl_Indexf;
   vfmt->Indexfv = _tnl_Indexfv;
   vfmt->Materialfv = _tnl_Materialfv;

   vfmt->Rectf = _mesa_noop_Rectf;
   vfmt->EvalMesh1 = _mesa_noop_EvalMesh1;
   vfmt->EvalMesh2 = _mesa_noop_EvalMesh2;
}



void _tnl_FlushVertices( GLcontext *ctx, GLuint flags )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   (void) flags;

   if (ctx->Driver.CurrentExecPrimitive != PRIM_OUTSIDE_BEGIN_END)
      return;

   if (tnl->vtx.counter != tnl->vtx.initial_counter) {
      _tnl_flush_vtx( ctx );
   }

   if (tnl->vtx.vertex_size) {
      _tnl_copy_to_current( ctx );
      reset_attrfv( tnl );
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
   tnl->vtx.current[_TNL_ATTRIB_EDGEFLAG] = &tnl->vtx.CurrentFloatEdgeFlag;
}

static struct _tnl_dynfn *no_codegen( GLcontext *ctx, int key )
{
   (void) ctx; (void) key;
   return NULL;
}

void _tnl_vtx_init( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct tnl_vertex_arrays *tmp = &tnl->vtx_inputs;
   GLuint i;
   static int firsttime = 1;

   if (firsttime) {
      firsttime = 0;

      INIT_CHOOSERS( 0 );
      INIT_CHOOSERS( 1 );
      INIT_CHOOSERS( 2 );
      INIT_CHOOSERS( 3 );
      INIT_CHOOSERS( 4 );
      INIT_CHOOSERS( 5 );
      INIT_CHOOSERS( 6 );
      INIT_CHOOSERS( 7 );
      INIT_CHOOSERS( 8 );
      INIT_CHOOSERS( 9 );
      INIT_CHOOSERS( 10 );
      INIT_CHOOSERS( 11 );
      INIT_CHOOSERS( 12 );
      INIT_CHOOSERS( 13 );
      INIT_CHOOSERS( 14 );
      INIT_CHOOSERS( 15 );

      choose[ERROR_ATTRIB][0] = error_attrib;
      choose[ERROR_ATTRIB][1] = error_attrib;
      choose[ERROR_ATTRIB][2] = error_attrib;
      choose[ERROR_ATTRIB][3] = error_attrib;

#ifdef USE_X86_ASM
      if (tnl->AllowCodegen) {
         _tnl_x86choosers(choose, do_choose); /* x86 INIT_CHOOSERS */
      }
#endif

      _tnl_generic_attr_table_init( generic_attr_func );
   }

   for (i = 0; i < _TNL_ATTRIB_INDEX; i++)
      _mesa_vector4f_init( &tmp->Attribs[i], 0, NULL);

   for (i = 0; i < 4; i++) {
      make_empty_list( &tnl->vtx.cache.Vertex[i] );
      make_empty_list( &tnl->vtx.cache.Attribute[i] );
      tnl->vtx.gen.Vertex[i] = no_codegen;
      tnl->vtx.gen.Attribute[i] = no_codegen;
   }

#ifdef USE_X86_ASM
   _tnl_InitX86Codegen( &tnl->vtx.gen );
#endif

   _tnl_current_init( ctx );
   _tnl_exec_vtxfmt_init( ctx );
   _tnl_generic_exec_vtxfmt_init( ctx );
#ifdef USE_X86_ASM
   if (tnl->AllowCodegen) {
      _tnl_x86_exec_vtxfmt_init( ctx ); /* x86 DISPATCH_ATTRFV */
   }
#endif

   _mesa_install_exec_vtxfmt( ctx, &tnl->exec_vtxfmt );

   memcpy( tnl->vtx.tabfv, choose, sizeof(choose) );

   for (i = 0 ; i < _TNL_ATTRIB_MAX ; i++)
      tnl->vtx.attrsz[i] = 0;

   tnl->vtx.vertex_size = 0;
   tnl->vtx.have_materials = 0;
}

static void free_funcs( struct _tnl_dynfn *l )
{
   struct _tnl_dynfn *f, *tmp;
   foreach_s (f, tmp, l) {
      remove_from_list( f );
      ALIGN_FREE( f->code );
      FREE( f );
   }
}


void _tnl_vtx_destroy( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint i;

   for (i = 0; i < 4; i++) {
      free_funcs( &tnl->vtx.cache.Vertex[i] );
      free_funcs( &tnl->vtx.cache.Attribute[i] );
   }
}

