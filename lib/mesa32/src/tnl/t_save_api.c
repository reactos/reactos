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



/* Display list compiler attempts to store lists of vertices with the
 * same vertex layout.  Additionally it attempts to minimize the need
 * for execute-time fixup of these vertex lists, allowing them to be
 * cached on hardware.
 *
 * There are still some circumstances where this can be thwarted, for
 * example by building a list that consists of one very long primitive
 * (eg Begin(Triangles), 1000 vertices, End), and calling that list
 * from inside a different begin/end object (Begin(Lines), CallList,
 * End).  
 *
 * In that case the code will have to replay the list as individual
 * commands through the Exec dispatch table, or fix up the copied
 * vertices at execute-time.
 *
 * The other case where fixup is required is when a vertex attribute
 * is introduced in the middle of a primitive.  Eg:
 *  Begin(Lines)
 *  TexCoord1f()           Vertex2f()
 *  TexCoord1f() Color3f() Vertex2f()
 *  End()
 *
 *  If the current value of Color isn't known at compile-time, this
 *  primitive will require fixup.
 *
 *
 * The list compiler currently doesn't attempt to compile lists
 * containing EvalCoord or EvalPoint commands.  On encountering one of
 * these, compilation falls back to opcodes.  
 *
 * This could be improved to fallback only when a mix of EvalCoord and
 * Vertex commands are issued within a single primitive.
 */


#include "glheader.h"
#include "context.h"
#include "dlist.h"
#include "enums.h"
#include "macros.h"
#include "api_validate.h"
#include "api_arrayelt.h"
#include "vtxfmt.h"
#include "t_save_api.h"
#include "dispatch.h"

/*
 * NOTE: Old 'parity' issue is gone, but copying can still be
 * wrong-footed on replay.
 */
static GLuint _save_copy_vertices( GLcontext *ctx, 
				   const struct tnl_vertex_list *node )
{
   TNLcontext *tnl = TNL_CONTEXT( ctx );
   const struct tnl_prim *prim = &node->prim[node->prim_count-1];
   GLuint nr = prim->count;
   GLuint sz = tnl->save.vertex_size;
   const GLfloat *src = node->buffer + prim->start * sz;
   GLfloat *dst = tnl->save.copied.buffer;
   GLuint ovf, i;

   if (prim->mode & PRIM_END)
      return 0;
	 
   switch( prim->mode & PRIM_MODE_MASK )
   {
   case GL_POINTS:
      return 0;
   case GL_LINES:
      ovf = nr&1;
      for (i = 0 ; i < ovf ; i++)
	 _mesa_memcpy( dst+i*sz, src+(nr-ovf+i)*sz, sz*sizeof(GLfloat) );
      return i;
   case GL_TRIANGLES:
      ovf = nr%3;
      for (i = 0 ; i < ovf ; i++)
	 _mesa_memcpy( dst+i*sz, src+(nr-ovf+i)*sz, sz*sizeof(GLfloat) );
      return i;
   case GL_QUADS:
      ovf = nr&3;
      for (i = 0 ; i < ovf ; i++)
	 _mesa_memcpy( dst+i*sz, src+(nr-ovf+i)*sz, sz*sizeof(GLfloat) );
      return i;
   case GL_LINE_STRIP:
      if (nr == 0) 
	 return 0;
      else {
	 _mesa_memcpy( dst, src+(nr-1)*sz, sz*sizeof(GLfloat) );
	 return 1;
      }
   case GL_LINE_LOOP:
   case GL_TRIANGLE_FAN:
   case GL_POLYGON:
      if (nr == 0) 
	 return 0;
      else if (nr == 1) {
	 _mesa_memcpy( dst, src+0, sz*sizeof(GLfloat) );
	 return 1;
      } else {
	 _mesa_memcpy( dst, src+0, sz*sizeof(GLfloat) );
	 _mesa_memcpy( dst+sz, src+(nr-1)*sz, sz*sizeof(GLfloat) );
	 return 2;
      }
   case GL_TRIANGLE_STRIP:
   case GL_QUAD_STRIP:
      switch (nr) {
      case 0: ovf = 0; break;
      case 1: ovf = 1; break;
      default: ovf = 2 + (nr&1); break;
      }
      for (i = 0 ; i < ovf ; i++)
	 _mesa_memcpy( dst+i*sz, src+(nr-ovf+i)*sz, sz*sizeof(GLfloat) );
      return i;
   default:
      assert(0);
      return 0;
   }
}


static void
build_normal_lengths( struct tnl_vertex_list *node )
{
   GLuint i;
   GLfloat *len;
   GLfloat *n = node->buffer;
   GLuint stride = node->vertex_size;
   GLuint count = node->count;

   len = node->normal_lengths = (GLfloat *) MALLOC( count * sizeof(GLfloat) );
   if (!len)
      return;

   /* Find the normal of the first vertex:
    */
   for (i = 0 ; i < _TNL_ATTRIB_NORMAL ; i++) 
      n += node->attrsz[i];

   for (i = 0 ; i < count ; i++, n += stride) {
      len[i] = LEN_3FV( n );
      if (len[i] > 0.0F) len[i] = 1.0F / len[i];
   } 
}

static struct tnl_vertex_store *alloc_vertex_store( GLcontext *ctx )
{
   struct tnl_vertex_store *store = MALLOC_STRUCT(tnl_vertex_store);
   (void) ctx;
   store->used = 0;
   store->refcount = 1;
   return store;
}

static struct tnl_primitive_store *alloc_prim_store( GLcontext *ctx )
{
   struct tnl_primitive_store *store = MALLOC_STRUCT(tnl_primitive_store);
   (void) ctx;
   store->used = 0;
   store->refcount = 1;
   return store;
}

static void _save_reset_counters( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   tnl->save.prim = tnl->save.prim_store->buffer + tnl->save.prim_store->used;
   tnl->save.buffer = (tnl->save.vertex_store->buffer + 
		       tnl->save.vertex_store->used);

   if (tnl->save.vertex_size)
      tnl->save.initial_counter = ((SAVE_BUFFER_SIZE - 
				    tnl->save.vertex_store->used) / 
				   tnl->save.vertex_size);
   else
      tnl->save.initial_counter = 0;

   if (tnl->save.initial_counter > ctx->Const.MaxArrayLockSize )
      tnl->save.initial_counter = ctx->Const.MaxArrayLockSize;

   tnl->save.counter = tnl->save.initial_counter;
   tnl->save.prim_count = 0;
   tnl->save.prim_max = SAVE_PRIM_SIZE - tnl->save.prim_store->used;
   tnl->save.copied.nr = 0;
   tnl->save.dangling_attr_ref = 0;
}


/* Insert the active immediate struct onto the display list currently
 * being built.
 */
static void _save_compile_vertex_list( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct tnl_vertex_list *node;

   /* Allocate space for this structure in the display list currently
    * being compiled.
    */
   node = (struct tnl_vertex_list *)
      _mesa_alloc_instruction(ctx, tnl->save.opcode_vertex_list, sizeof(*node));

   if (!node)
      return;

   /* Duplicate our template, increment refcounts to the storage structs:
    */
   _mesa_memcpy(node->attrsz, tnl->save.attrsz, sizeof(node->attrsz));
   node->vertex_size = tnl->save.vertex_size;
   node->buffer = tnl->save.buffer;
   node->count = tnl->save.initial_counter - tnl->save.counter;
   node->wrap_count = tnl->save.copied.nr;
   node->have_materials = tnl->save.have_materials;
   node->dangling_attr_ref = tnl->save.dangling_attr_ref;
   node->normal_lengths = NULL;
   node->prim = tnl->save.prim;
   node->prim_count = tnl->save.prim_count;
   node->vertex_store = tnl->save.vertex_store;
   node->prim_store = tnl->save.prim_store;

   node->vertex_store->refcount++;
   node->prim_store->refcount++;

   assert(node->attrsz[_TNL_ATTRIB_POS] != 0 ||
	  node->count == 0);

   if (tnl->save.dangling_attr_ref)
      ctx->ListState.CurrentList->flags |= MESA_DLIST_DANGLING_REFS;

   /* Maybe calculate normal lengths:
    */
   if (tnl->CalcDListNormalLengths && 
       node->attrsz[_TNL_ATTRIB_NORMAL] == 3 &&
       !(ctx->ListState.CurrentList->flags & MESA_DLIST_DANGLING_REFS))
      build_normal_lengths( node );


   tnl->save.vertex_store->used += tnl->save.vertex_size * node->count;
   tnl->save.prim_store->used += node->prim_count;

   /* Decide whether the storage structs are full, or can be used for
    * the next vertex lists as well.
    */
   if (tnl->save.vertex_store->used > 
       SAVE_BUFFER_SIZE - 16 * (tnl->save.vertex_size + 4)) {

      tnl->save.vertex_store->refcount--; 
      assert(tnl->save.vertex_store->refcount != 0);
      tnl->save.vertex_store = alloc_vertex_store( ctx );
      tnl->save.vbptr = tnl->save.vertex_store->buffer;
   } 

   if (tnl->save.prim_store->used > SAVE_PRIM_SIZE - 6) {
      tnl->save.prim_store->refcount--; 
      assert(tnl->save.prim_store->refcount != 0);
      tnl->save.prim_store = alloc_prim_store( ctx );
   } 

   /* Reset our structures for the next run of vertices:
    */
   _save_reset_counters( ctx );

   /* Copy duplicated vertices 
    */
   tnl->save.copied.nr = _save_copy_vertices( ctx, node );


   /* Deal with GL_COMPILE_AND_EXECUTE:
    */
   if (ctx->ExecuteFlag) {
      _tnl_playback_vertex_list( ctx, (void *) node );
   }
}


/* TODO -- If no new vertices have been stored, don't bother saving
 * it.
 */
static void _save_wrap_buffers( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLint i = tnl->save.prim_count - 1;
   GLenum mode;

   assert(i < (GLint) tnl->save.prim_max);
   assert(i >= 0);

   /* Close off in-progress primitive.
    */
   tnl->save.prim[i].count = ((tnl->save.initial_counter - tnl->save.counter) - 
			     tnl->save.prim[i].start);
   mode = tnl->save.prim[i].mode & ~(PRIM_BEGIN|PRIM_END);
   
   /* store the copied vertices, and allocate a new list.
    */
   _save_compile_vertex_list( ctx );

   /* Restart interrupted primitive
    */
   tnl->save.prim[0].mode = mode;
   tnl->save.prim[0].start = 0;
   tnl->save.prim[0].count = 0;
   tnl->save.prim_count = 1;
}



/* Called only when buffers are wrapped as the result of filling the
 * vertex_store struct.  
 */
static void _save_wrap_filled_vertex( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLfloat *data = tnl->save.copied.buffer;
   GLuint i;

   /* Emit a glEnd to close off the last vertex list.
    */
   _save_wrap_buffers( ctx );
   
    /* Copy stored stored vertices to start of new list.
    */
   assert(tnl->save.counter > tnl->save.copied.nr);

   for (i = 0 ; i < tnl->save.copied.nr ; i++) {
      _mesa_memcpy( tnl->save.vbptr, data, tnl->save.vertex_size * sizeof(GLfloat));
      data += tnl->save.vertex_size;
      tnl->save.vbptr += tnl->save.vertex_size;
      tnl->save.counter--;
   }
}


static void _save_copy_to_current( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx); 
   GLuint i;

   for (i = _TNL_ATTRIB_POS+1 ; i <= _TNL_ATTRIB_INDEX ; i++) {
      if (tnl->save.attrsz[i]) {
	 tnl->save.currentsz[i][0] = tnl->save.attrsz[i];
	 COPY_CLEAN_4V(tnl->save.current[i], 
		    tnl->save.attrsz[i], 
		    tnl->save.attrptr[i]);
      }
   }

   /* Edgeflag requires special treatment: 
    *
    * TODO: change edgeflag to GLfloat in Mesa.
    */
   if (tnl->save.attrsz[_TNL_ATTRIB_EDGEFLAG]) {
      ctx->ListState.ActiveEdgeFlag = 1;
      tnl->save.CurrentFloatEdgeFlag = 
	 tnl->save.attrptr[_TNL_ATTRIB_EDGEFLAG][0];
      ctx->ListState.CurrentEdgeFlag = 
	 (tnl->save.CurrentFloatEdgeFlag == 1.0);
   }
}


static void _save_copy_from_current( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx); 
   GLint i;

   for (i = _TNL_ATTRIB_POS+1 ; i <= _TNL_ATTRIB_INDEX ; i++) 
      switch (tnl->save.attrsz[i]) {
      case 4: tnl->save.attrptr[i][3] = tnl->save.current[i][3];
      case 3: tnl->save.attrptr[i][2] = tnl->save.current[i][2];
      case 2: tnl->save.attrptr[i][1] = tnl->save.current[i][1];
      case 1: tnl->save.attrptr[i][0] = tnl->save.current[i][0];
      case 0: break;
      }

   /* Edgeflag requires special treatment:
    */
   if (tnl->save.attrsz[_TNL_ATTRIB_EDGEFLAG]) {
      tnl->save.CurrentFloatEdgeFlag = (GLfloat)ctx->ListState.CurrentEdgeFlag;
      tnl->save.attrptr[_TNL_ATTRIB_EDGEFLAG][0] = tnl->save.CurrentFloatEdgeFlag;
   }
}




/* Flush existing data, set new attrib size, replay copied vertices.
 */ 
static void _save_upgrade_vertex( GLcontext *ctx, 
				 GLuint attr,
				 GLuint newsz )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint oldsz;
   GLuint i;
   GLfloat *tmp;

   /* Store the current run of vertices, and emit a GL_END.  Emit a
    * BEGIN in the new buffer.
    */
   if (tnl->save.initial_counter != tnl->save.counter) 
      _save_wrap_buffers( ctx );
   else
      assert( tnl->save.copied.nr == 0 );

   /* Do a COPY_TO_CURRENT to ensure back-copying works for the case
    * when the attribute already exists in the vertex and is having
    * its size increased.  
    */
   _save_copy_to_current( ctx );

   /* Fix up sizes:
    */
   oldsz = tnl->save.attrsz[attr];
   tnl->save.attrsz[attr] = newsz;

   tnl->save.vertex_size += newsz - oldsz;
   tnl->save.counter = ((SAVE_BUFFER_SIZE - tnl->save.vertex_store->used) / 
			tnl->save.vertex_size);
   if (tnl->save.counter > ctx->Const.MaxArrayLockSize )
      tnl->save.counter = ctx->Const.MaxArrayLockSize;
   tnl->save.initial_counter = tnl->save.counter;

   /* Recalculate all the attrptr[] values:
    */
   for (i = 0, tmp = tnl->save.vertex ; i < _TNL_ATTRIB_MAX ; i++) {
      if (tnl->save.attrsz[i]) {
	 tnl->save.attrptr[i] = tmp;
	 tmp += tnl->save.attrsz[i];
      }
      else 
	 tnl->save.attrptr[i] = NULL; /* will not be dereferenced. */
   }

   /* Copy from current to repopulate the vertex with correct values.
    */
   _save_copy_from_current( ctx );

   /* Replay stored vertices to translate them to new format here.
    *
    * If there are copied vertices and the new (upgraded) attribute
    * has not been defined before, this list is somewhat degenerate,
    * and will need fixup at runtime.
    */
   if (tnl->save.copied.nr)
   {
      GLfloat *data = tnl->save.copied.buffer;
      GLfloat *dest = tnl->save.buffer;
      GLuint j;

      /* Need to note this and fix up at runtime (or loopback):
       */
      if (tnl->save.currentsz[attr][0] == 0) {
	 assert(oldsz == 0);
	 tnl->save.dangling_attr_ref = GL_TRUE;

/* 	 _mesa_debug(NULL, "_save_upgrade_vertex: dangling reference attr %d\n",  */
/* 		     attr);  */

#if 0
	 /* The current strategy is to punt these degenerate cases
	  * through _tnl_loopback_vertex_list(), a lower-performance
	  * option.  To minimize the impact of this, artificially
	  * reduce the size of this vertex_list.
	  */
	 if (t->save.counter > 10) {
	    t->save.initial_counter = 10;
	    t->save.counter = 10;
	 }
#endif
      }

      for (i = 0 ; i < tnl->save.copied.nr ; i++) {
	 for (j = 0 ; j < _TNL_ATTRIB_MAX ; j++) {
	    if (tnl->save.attrsz[j]) {
	       if (j == attr) {
		  if (oldsz) {
		     COPY_CLEAN_4V( dest, oldsz, data );
		     data += oldsz;
		     dest += newsz;
		  }
		  else {
		     COPY_SZ_4V( dest, newsz, tnl->save.current[attr] );
		     dest += newsz;
		  }
	       }
	       else {
		  GLint sz = tnl->save.attrsz[j];
		  COPY_SZ_4V( dest, sz, data );
		  data += sz;
		  dest += sz;
	       }
	    }
	 }
      }

      tnl->save.vbptr = dest;
      tnl->save.counter -= tnl->save.copied.nr;
   }
}




/* Helper function for 'CHOOSE' macro.  Do what's necessary when an
 * entrypoint is called for the first time.
 */
static void do_choose( GLuint attr, GLuint sz, 
		       void (*attr_func)( const GLfloat *),
		       void (*choose1)( const GLfloat *),
		       void (*choose2)( const GLfloat *),
		       void (*choose3)( const GLfloat *),
		       void (*choose4)( const GLfloat *),
		       const GLfloat *v )
{ 
   GET_CURRENT_CONTEXT( ctx ); 
   TNLcontext *tnl = TNL_CONTEXT(ctx); 
   static GLfloat id[4] = { 0, 0, 0, 1 };
   int i;

   if (tnl->save.attrsz[attr] < sz) {
      /* New size is larger.  Need to flush existing vertices and get
       * an enlarged vertex format.
       */
      _save_upgrade_vertex( ctx, attr, sz );
   }
   else {
      /* New size is equal or smaller - just need to fill in some
       * zeros.
       */
      for (i = sz ; i <= tnl->save.attrsz[attr] ; i++)
	 tnl->save.attrptr[attr][i-1] = id[i-1];
   }

   /* Reset any active pointers for this attribute 
    */
   tnl->save.tabfv[attr][0] = choose1;
   tnl->save.tabfv[attr][1] = choose2;
   tnl->save.tabfv[attr][2] = choose3;
   tnl->save.tabfv[attr][3] = choose4;

   /* Update the secondary dispatch table with the new function
    */
   tnl->save.tabfv[attr][sz-1] = attr_func;

   (*attr_func)(v);
}



/* Only one size for each attribute may be active at once.  Eg. if
 * Color3f is installed/active, then Color4f may not be, even if the
 * vertex actually contains 4 color coordinates.  This is because the
 * 3f version won't otherwise set color[3] to 1.0 -- this is the job
 * of the chooser function when switching between Color4f and Color3f.
 */
#define ATTRFV( ATTR, N )					\
static void save_choose_##ATTR##_##N( const GLfloat *v );	\
								\
static void save_attrib_##ATTR##_##N( const GLfloat *v )	\
{								\
   GET_CURRENT_CONTEXT( ctx );					\
   TNLcontext *tnl = TNL_CONTEXT(ctx);				\
								\
   if ((ATTR) == 0) {						\
      GLuint i;							\
								\
      if (N>0) tnl->save.vbptr[0] = v[0];			\
      if (N>1) tnl->save.vbptr[1] = v[1];			\
      if (N>2) tnl->save.vbptr[2] = v[2];			\
      if (N>3) tnl->save.vbptr[3] = v[3];			\
								\
      for (i = N; i < tnl->save.vertex_size; i++)		\
	 tnl->save.vbptr[i] = tnl->save.vertex[i];		\
								\
      tnl->save.vbptr += tnl->save.vertex_size;			\
								\
      if (--tnl->save.counter == 0)				\
	 _save_wrap_filled_vertex( ctx );			\
   }								\
   else {							\
      GLfloat *dest = tnl->save.attrptr[ATTR];			\
      if (N>0) dest[0] = v[0];					\
      if (N>1) dest[1] = v[1];					\
      if (N>2) dest[2] = v[2];					\
      if (N>3) dest[3] = v[3];					\
   }								\
}

#define CHOOSE( ATTR, N )					\
static void save_choose_##ATTR##_##N( const GLfloat *v )	\
{								\
   do_choose(ATTR, N,						\
	     save_attrib_##ATTR##_##N,				\
	     save_choose_##ATTR##_1,				\
	     save_choose_##ATTR##_2,				\
	     save_choose_##ATTR##_3,				\
	     save_choose_##ATTR##_4,				\
	     v );						\
}

#define INIT(ATTR)					\
static void save_init_##ATTR( TNLcontext *tnl )		\
{							\
   tnl->save.tabfv[ATTR][0] = save_choose_##ATTR##_1;	\
   tnl->save.tabfv[ATTR][1] = save_choose_##ATTR##_2;	\
   tnl->save.tabfv[ATTR][2] = save_choose_##ATTR##_3;	\
   tnl->save.tabfv[ATTR][3] = save_choose_##ATTR##_4;	\
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


static void _save_reset_vertex( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint i;

   save_init_0( tnl );
   save_init_1( tnl );
   save_init_2( tnl );
   save_init_3( tnl );
   save_init_4( tnl );
   save_init_5( tnl );
   save_init_6( tnl );
   save_init_7( tnl );
   save_init_8( tnl );
   save_init_9( tnl );
   save_init_10( tnl );
   save_init_11( tnl );
   save_init_12( tnl );
   save_init_13( tnl );
   save_init_14( tnl );
   save_init_15( tnl );
      
   for (i = 0 ; i < _TNL_ATTRIB_MAX ; i++)
      tnl->save.attrsz[i] = 0;
      
   tnl->save.vertex_size = 0;
   tnl->save.have_materials = 0;

   _save_reset_counters( ctx ); 
}



/* Cope with aliasing of classic Vertex, Normal, etc. and the fan-out
 * of glMultTexCoord and glProgramParamterNV by routing all these
 * through a second level dispatch table.
 */
#define DISPATCH_ATTRFV( ATTR, COUNT, P )	\
do {						\
   GET_CURRENT_CONTEXT( ctx ); 			\
   TNLcontext *tnl = TNL_CONTEXT(ctx); 		\
   tnl->save.tabfv[ATTR][COUNT-1]( P );		\
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


static void enum_error( void )
{
   GET_CURRENT_CONTEXT( ctx );
   _mesa_compile_error( ctx, GL_INVALID_ENUM, "glVertexAttrib" );
}

static void GLAPIENTRY _save_Vertex2f( GLfloat x, GLfloat y )
{
   DISPATCH_ATTR2F( _TNL_ATTRIB_POS, x, y );
}

static void GLAPIENTRY _save_Vertex2fv( const GLfloat *v )
{
   DISPATCH_ATTR2FV( _TNL_ATTRIB_POS, v );
}

static void GLAPIENTRY _save_Vertex3f( GLfloat x, GLfloat y, GLfloat z )
{
   DISPATCH_ATTR3F( _TNL_ATTRIB_POS, x, y, z );
}

static void GLAPIENTRY _save_Vertex3fv( const GLfloat *v )
{
   DISPATCH_ATTR3FV( _TNL_ATTRIB_POS, v );
}

static void GLAPIENTRY _save_Vertex4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   DISPATCH_ATTR4F( _TNL_ATTRIB_POS, x, y, z, w );
}

static void GLAPIENTRY _save_Vertex4fv( const GLfloat *v )
{
   DISPATCH_ATTR4FV( _TNL_ATTRIB_POS, v );
}

static void GLAPIENTRY _save_TexCoord1f( GLfloat x )
{
   DISPATCH_ATTR1F( _TNL_ATTRIB_TEX0, x );
}

static void GLAPIENTRY _save_TexCoord1fv( const GLfloat *v )
{
   DISPATCH_ATTR1FV( _TNL_ATTRIB_TEX0, v );
}

static void GLAPIENTRY _save_TexCoord2f( GLfloat x, GLfloat y )
{
   DISPATCH_ATTR2F( _TNL_ATTRIB_TEX0, x, y );
}

static void GLAPIENTRY _save_TexCoord2fv( const GLfloat *v )
{
   DISPATCH_ATTR2FV( _TNL_ATTRIB_TEX0, v );
}

static void GLAPIENTRY _save_TexCoord3f( GLfloat x, GLfloat y, GLfloat z )
{
   DISPATCH_ATTR3F( _TNL_ATTRIB_TEX0, x, y, z );
}

static void GLAPIENTRY _save_TexCoord3fv( const GLfloat *v )
{
   DISPATCH_ATTR3FV( _TNL_ATTRIB_TEX0, v );
}

static void GLAPIENTRY _save_TexCoord4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   DISPATCH_ATTR4F( _TNL_ATTRIB_TEX0, x, y, z, w );
}

static void GLAPIENTRY _save_TexCoord4fv( const GLfloat *v )
{
   DISPATCH_ATTR4FV( _TNL_ATTRIB_TEX0, v );
}

static void GLAPIENTRY _save_Normal3f( GLfloat x, GLfloat y, GLfloat z )
{
   DISPATCH_ATTR3F( _TNL_ATTRIB_NORMAL, x, y, z );
}

static void GLAPIENTRY _save_Normal3fv( const GLfloat *v )
{
   DISPATCH_ATTR3FV( _TNL_ATTRIB_NORMAL, v );
}

static void GLAPIENTRY _save_FogCoordfEXT( GLfloat x )
{
   DISPATCH_ATTR1F( _TNL_ATTRIB_FOG, x );
}

static void GLAPIENTRY _save_FogCoordfvEXT( const GLfloat *v )
{
   DISPATCH_ATTR1FV( _TNL_ATTRIB_FOG, v );
}

static void GLAPIENTRY _save_Color3f( GLfloat x, GLfloat y, GLfloat z )
{
   DISPATCH_ATTR3F( _TNL_ATTRIB_COLOR0, x, y, z );
}

static void GLAPIENTRY _save_Color3fv( const GLfloat *v )
{
   DISPATCH_ATTR3FV( _TNL_ATTRIB_COLOR0, v );
}

static void GLAPIENTRY _save_Color4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   DISPATCH_ATTR4F( _TNL_ATTRIB_COLOR0, x, y, z, w );
}

static void GLAPIENTRY _save_Color4fv( const GLfloat *v )
{
   DISPATCH_ATTR4FV( _TNL_ATTRIB_COLOR0, v );
}

static void GLAPIENTRY _save_SecondaryColor3fEXT( GLfloat x, GLfloat y, GLfloat z )
{
   DISPATCH_ATTR3F( _TNL_ATTRIB_COLOR1, x, y, z );
}

static void GLAPIENTRY _save_SecondaryColor3fvEXT( const GLfloat *v )
{
   DISPATCH_ATTR3FV( _TNL_ATTRIB_COLOR1, v );
}

static void GLAPIENTRY _save_MultiTexCoord1f( GLenum target, GLfloat x  )
{
   GLuint attr = (target & 0x7) + _TNL_ATTRIB_TEX0;
   DISPATCH_ATTR1F( attr, x );
}

static void GLAPIENTRY _save_MultiTexCoord1fv( GLenum target, const GLfloat *v )
{
   GLuint attr = (target & 0x7) + _TNL_ATTRIB_TEX0;
   DISPATCH_ATTR1FV( attr, v );
}

static void GLAPIENTRY _save_MultiTexCoord2f( GLenum target, GLfloat x, GLfloat y )
{
   GLuint attr = (target & 0x7) + _TNL_ATTRIB_TEX0;
   DISPATCH_ATTR2F( attr, x, y );
}

static void GLAPIENTRY _save_MultiTexCoord2fv( GLenum target, const GLfloat *v )
{
   GLuint attr = (target & 0x7) + _TNL_ATTRIB_TEX0;
   DISPATCH_ATTR2FV( attr, v );
}

static void GLAPIENTRY _save_MultiTexCoord3f( GLenum target, GLfloat x, GLfloat y,
				    GLfloat z)
{
   GLuint attr = (target & 0x7) + _TNL_ATTRIB_TEX0;
   DISPATCH_ATTR3F( attr, x, y, z );
}

static void GLAPIENTRY _save_MultiTexCoord3fv( GLenum target, const GLfloat *v )
{
   GLuint attr = (target & 0x7) + _TNL_ATTRIB_TEX0;
   DISPATCH_ATTR3FV( attr, v );
}

static void GLAPIENTRY _save_MultiTexCoord4f( GLenum target, GLfloat x, GLfloat y,
				    GLfloat z, GLfloat w )
{
   GLuint attr = (target & 0x7) + _TNL_ATTRIB_TEX0;
   DISPATCH_ATTR4F( attr, x, y, z, w );
}

static void GLAPIENTRY _save_MultiTexCoord4fv( GLenum target, const GLfloat *v )
{
   GLuint attr = (target & 0x7) + _TNL_ATTRIB_TEX0;
   DISPATCH_ATTR4FV( attr, v );
}

static void GLAPIENTRY _save_VertexAttrib1fNV( GLuint index, GLfloat x )
{
   if (index < VERT_ATTRIB_MAX)
      DISPATCH_ATTR1F( index, x );
   else
      enum_error(); 
}

static void GLAPIENTRY _save_VertexAttrib1fvNV( GLuint index, const GLfloat *v )
{
   if (index < VERT_ATTRIB_MAX)
      DISPATCH_ATTR1FV( index, v );
   else
      enum_error();
}

static void GLAPIENTRY _save_VertexAttrib2fNV( GLuint index, GLfloat x, GLfloat y )
{
   if (index < VERT_ATTRIB_MAX)
      DISPATCH_ATTR2F( index, x, y );
   else
      enum_error();
}

static void GLAPIENTRY _save_VertexAttrib2fvNV( GLuint index, const GLfloat *v )
{
   if (index < VERT_ATTRIB_MAX)
      DISPATCH_ATTR2FV( index, v );
   else
      enum_error();
}

static void GLAPIENTRY _save_VertexAttrib3fNV( GLuint index, GLfloat x, GLfloat y, 
				  GLfloat z )
{
   if (index < VERT_ATTRIB_MAX)
      DISPATCH_ATTR3F( index, x, y, z );
   else
      enum_error();
}

static void GLAPIENTRY _save_VertexAttrib3fvNV( GLuint index, const GLfloat *v )
{
   if (index < VERT_ATTRIB_MAX)
      DISPATCH_ATTR3FV( index, v );
   else
      enum_error();
}

static void GLAPIENTRY _save_VertexAttrib4fNV( GLuint index, GLfloat x, GLfloat y,
				  GLfloat z, GLfloat w )
{
   if (index < VERT_ATTRIB_MAX)
      DISPATCH_ATTR4F( index, x, y, z, w );
   else
      enum_error();
}

static void GLAPIENTRY _save_VertexAttrib4fvNV( GLuint index, const GLfloat *v )
{
   if (index < VERT_ATTRIB_MAX)
      DISPATCH_ATTR4FV( index, v );
   else
      enum_error();
}


static void GLAPIENTRY
_save_VertexAttrib1fARB( GLuint index, GLfloat x )
{
   if (index < VERT_ATTRIB_MAX)
      DISPATCH_ATTR1F( index, x );
   else
      enum_error(); 
}

static void GLAPIENTRY
_save_VertexAttrib1fvARB( GLuint index, const GLfloat *v )
{
   if (index < VERT_ATTRIB_MAX)
      DISPATCH_ATTR1FV( index, v );
   else
      enum_error();
}

static void GLAPIENTRY
_save_VertexAttrib2fARB( GLuint index, GLfloat x, GLfloat y )
{
   if (index < VERT_ATTRIB_MAX)
      DISPATCH_ATTR2F( index, x, y );
   else
      enum_error();
}

static void GLAPIENTRY
_save_VertexAttrib2fvARB( GLuint index, const GLfloat *v )
{
   if (index < VERT_ATTRIB_MAX)
      DISPATCH_ATTR2FV( index, v );
   else
      enum_error();
}

static void GLAPIENTRY
_save_VertexAttrib3fARB( GLuint index, GLfloat x, GLfloat y, GLfloat z )
{
   if (index < VERT_ATTRIB_MAX)
      DISPATCH_ATTR3F( index, x, y, z );
   else
      enum_error();
}

static void GLAPIENTRY
_save_VertexAttrib3fvARB( GLuint index, const GLfloat *v )
{
   if (index < VERT_ATTRIB_MAX)
      DISPATCH_ATTR3FV( index, v );
   else
      enum_error();
}

static void GLAPIENTRY
_save_VertexAttrib4fARB( GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   if (index < VERT_ATTRIB_MAX)
      DISPATCH_ATTR4F( index, x, y, z, w );
   else
      enum_error();
}

static void GLAPIENTRY
_save_VertexAttrib4fvARB( GLuint index, const GLfloat *v )
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
   if (tnl->save.attrsz[A] < N) {			\
      _save_upgrade_vertex( ctx, A, N );		\
      tnl->save.have_materials = GL_TRUE;               \
   }							\
							\
   {							\
      GLfloat *dest = tnl->save.attrptr[A];	      	\
      if (N>0) dest[0] = params[0];			\
      if (N>1) dest[1] = params[1];			\
      if (N>2) dest[2] = params[2];			\
      if (N>3) dest[3] = params[3];			\
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
static void GLAPIENTRY _save_Materialfv( GLenum face, GLenum pname, 
			       const GLfloat *params )
{
   GET_CURRENT_CONTEXT( ctx ); 
   TNLcontext *tnl = TNL_CONTEXT(ctx);

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
      _mesa_compile_error( ctx, GL_INVALID_ENUM, "glMaterialfv" );
      return;
   }
}


#define IDX_ATTR( A, IDX )				\
do {							\
   GET_CURRENT_CONTEXT( ctx );				\
   TNLcontext *tnl = TNL_CONTEXT(ctx);			\
							\
   if (tnl->save.attrsz[A] < 1) {			\
      _save_upgrade_vertex( ctx, A, 1 );		\
   }							\
							\
   {							\
      GLfloat *dest = tnl->save.attrptr[A];		\
      dest[0] = IDX;				\
   }							\
} while (0)


static void GLAPIENTRY _save_EdgeFlag( GLboolean b )
{
   IDX_ATTR( _TNL_ATTRIB_EDGEFLAG, (GLfloat)b );
}

static void GLAPIENTRY _save_EdgeFlagv( const GLboolean *v )
{
   IDX_ATTR( _TNL_ATTRIB_EDGEFLAG, (GLfloat)(v[0]) );
}

static void GLAPIENTRY _save_Indexf( GLfloat f )
{
   IDX_ATTR( _TNL_ATTRIB_INDEX, f );
}

static void GLAPIENTRY _save_Indexfv( const GLfloat *f )
{
   IDX_ATTR( _TNL_ATTRIB_INDEX, f[0] );
}




/* Cope with EvalCoord/CallList called within a begin/end object:
 *     -- Flush current buffer
 *     -- Fallback to opcodes for the rest of the begin/end object.
 */
#define FALLBACK(ctx) 							\
do {									\
   TNLcontext *tnl = TNL_CONTEXT(ctx);					\
									\
   if (tnl->save.initial_counter != tnl->save.counter ||		\
       tnl->save.prim_count) 						\
      _save_compile_vertex_list( ctx );					\
									\
   _save_copy_to_current( ctx );					\
   _save_reset_vertex( ctx );						\
   _mesa_install_save_vtxfmt( ctx, &ctx->ListState.ListVtxfmt );	\
   ctx->Driver.SaveNeedFlush = 0;					\
} while (0)

static void GLAPIENTRY _save_EvalCoord1f( GLfloat u )
{
   GET_CURRENT_CONTEXT(ctx);
   FALLBACK(ctx);
   CALL_EvalCoord1f(ctx->Save, ( u ));
}

static void GLAPIENTRY _save_EvalCoord1fv( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   FALLBACK(ctx);
   CALL_EvalCoord1fv(ctx->Save, ( v ));
}

static void GLAPIENTRY _save_EvalCoord2f( GLfloat u, GLfloat v )
{
   GET_CURRENT_CONTEXT(ctx);
   FALLBACK(ctx);
   CALL_EvalCoord2f(ctx->Save, ( u, v ));
}

static void GLAPIENTRY _save_EvalCoord2fv( const GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   FALLBACK(ctx);
   CALL_EvalCoord2fv(ctx->Save, ( v ));
}

static void GLAPIENTRY _save_EvalPoint1( GLint i )
{
   GET_CURRENT_CONTEXT(ctx);
   FALLBACK(ctx);
   CALL_EvalPoint1(ctx->Save, ( i ));
}

static void GLAPIENTRY _save_EvalPoint2( GLint i, GLint j )
{
   GET_CURRENT_CONTEXT(ctx);
   FALLBACK(ctx);
   CALL_EvalPoint2(ctx->Save, ( i, j ));
}

static void GLAPIENTRY _save_CallList( GLuint l )
{
   GET_CURRENT_CONTEXT(ctx);
   FALLBACK(ctx);
   CALL_CallList(ctx->Save, ( l ));
}

static void GLAPIENTRY _save_CallLists( GLsizei n, GLenum type, const GLvoid *v )
{
   GET_CURRENT_CONTEXT(ctx);
   FALLBACK(ctx);
   CALL_CallLists(ctx->Save, ( n, type, v ));
}




/* This begin is hooked into ...  Updating of
 * ctx->Driver.CurrentSavePrimitive is already taken care of.
 */
static GLboolean _save_NotifyBegin( GLcontext *ctx, GLenum mode )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx); 

   if (1) {
      GLuint i = tnl->save.prim_count++;

      assert(i < tnl->save.prim_max);
      tnl->save.prim[i].mode = mode | PRIM_BEGIN;
      tnl->save.prim[i].start = tnl->save.initial_counter - tnl->save.counter;
      tnl->save.prim[i].count = 0;   

      _mesa_install_save_vtxfmt( ctx, &tnl->save_vtxfmt );      
      ctx->Driver.SaveNeedFlush = 1;
      return GL_TRUE;
   }
   else 
      return GL_FALSE;
}



static void GLAPIENTRY _save_End( void )
{
   GET_CURRENT_CONTEXT( ctx ); 
   TNLcontext *tnl = TNL_CONTEXT(ctx); 
   GLint i = tnl->save.prim_count - 1;

   ctx->Driver.CurrentSavePrimitive = PRIM_OUTSIDE_BEGIN_END;
   tnl->save.prim[i].mode |= PRIM_END;
   tnl->save.prim[i].count = ((tnl->save.initial_counter - tnl->save.counter) - 
			      tnl->save.prim[i].start);

   if (i == (GLint) tnl->save.prim_max - 1) {
      _save_compile_vertex_list( ctx );
      assert(tnl->save.copied.nr == 0);
   }

   /* Swap out this vertex format while outside begin/end.  Any color,
    * etc. received between here and the next begin will be compiled
    * as opcodes.
    */   
   _mesa_install_save_vtxfmt( ctx, &ctx->ListState.ListVtxfmt );
}


/* These are all errors as this vtxfmt is only installed inside
 * begin/end pairs.
 */
static void GLAPIENTRY _save_DrawElements(GLenum mode, GLsizei count, GLenum type,
			       const GLvoid *indices)
{
   GET_CURRENT_CONTEXT(ctx);
   (void) mode; (void) count; (void) type; (void) indices;
   _mesa_compile_error( ctx, GL_INVALID_OPERATION, "glDrawElements" );
}


static void GLAPIENTRY _save_DrawRangeElements(GLenum mode,
				    GLuint start, GLuint end,
				    GLsizei count, GLenum type,
				    const GLvoid *indices)
{
   GET_CURRENT_CONTEXT(ctx);
   (void) mode; (void) start; (void) end; (void) count; (void) type; (void) indices;
   _mesa_compile_error( ctx, GL_INVALID_OPERATION, "glDrawRangeElements" );
}

static void GLAPIENTRY _save_DrawArrays(GLenum mode, GLint start, GLsizei count)
{
   GET_CURRENT_CONTEXT(ctx);
   (void) mode; (void) start; (void) count;
   _mesa_compile_error( ctx, GL_INVALID_OPERATION, "glDrawArrays" );
}

static void GLAPIENTRY _save_Rectf( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 )
{
   GET_CURRENT_CONTEXT(ctx);
   (void) x1; (void) y1; (void) x2; (void) y2;
   _mesa_compile_error( ctx, GL_INVALID_OPERATION, "glRectf" );
}

static void GLAPIENTRY _save_EvalMesh1( GLenum mode, GLint i1, GLint i2 )
{
   GET_CURRENT_CONTEXT(ctx);
   (void) mode; (void) i1; (void) i2;
   _mesa_compile_error( ctx, GL_INVALID_OPERATION, "glEvalMesh1" );
}

static void GLAPIENTRY _save_EvalMesh2( GLenum mode, GLint i1, GLint i2,
				  GLint j1, GLint j2 )
{
   GET_CURRENT_CONTEXT(ctx);
   (void) mode; (void) i1; (void) i2; (void) j1; (void) j2;
   _mesa_compile_error( ctx, GL_INVALID_OPERATION, "glEvalMesh2" );
}

static void GLAPIENTRY _save_Begin( GLenum mode )
{
   GET_CURRENT_CONTEXT( ctx );
   (void) mode;
   _mesa_compile_error( ctx, GL_INVALID_OPERATION, "Recursive glBegin" );
}


/* Unlike the functions above, these are to be hooked into the vtxfmt
 * maintained in ctx->ListState, active when the list is known or
 * suspected to be outside any begin/end primitive.
 */
static void GLAPIENTRY _save_OBE_Rectf( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 )
{
   GET_CURRENT_CONTEXT(ctx);
   _save_NotifyBegin( ctx, GL_QUADS | PRIM_WEAK );
   CALL_Vertex2f(GET_DISPATCH(), ( x1, y1 ));
   CALL_Vertex2f(GET_DISPATCH(), ( x2, y1 ));
   CALL_Vertex2f(GET_DISPATCH(), ( x2, y2 ));
   CALL_Vertex2f(GET_DISPATCH(), ( x1, y2 ));
   CALL_End(GET_DISPATCH(), ());
}


static void GLAPIENTRY _save_OBE_DrawArrays(GLenum mode, GLint start, GLsizei count)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;

   if (!_mesa_validate_DrawArrays( ctx, mode, start, count ))
      return;

   _save_NotifyBegin( ctx, mode | PRIM_WEAK );
   for (i = 0; i < count; i++)
       CALL_ArrayElement(GET_DISPATCH(), (start + i));
   CALL_End(GET_DISPATCH(), ());
}


static void GLAPIENTRY _save_OBE_DrawElements(GLenum mode, GLsizei count, GLenum type,
				   const GLvoid *indices)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;

   if (!_mesa_validate_DrawElements( ctx, mode, count, type, indices ))
      return;

   _save_NotifyBegin( ctx, mode | PRIM_WEAK );

   switch (type) {
   case GL_UNSIGNED_BYTE:
      for (i = 0 ; i < count ; i++)
	  CALL_ArrayElement(GET_DISPATCH(), ( ((GLubyte *)indices)[i] ));
      break;
   case GL_UNSIGNED_SHORT:
      for (i = 0 ; i < count ; i++)
	  CALL_ArrayElement(GET_DISPATCH(), ( ((GLushort *)indices)[i] ));
      break;
   case GL_UNSIGNED_INT:
      for (i = 0 ; i < count ; i++)
	  CALL_ArrayElement(GET_DISPATCH(), ( ((GLuint *)indices)[i] ));
      break;
   default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glDrawElements(type)" );
      break;
   }

   CALL_End(GET_DISPATCH(), ());
}

static void GLAPIENTRY _save_OBE_DrawRangeElements(GLenum mode,
					GLuint start, GLuint end,
					GLsizei count, GLenum type,
					const GLvoid *indices)
{
   GET_CURRENT_CONTEXT(ctx);
   if (_mesa_validate_DrawRangeElements( ctx, mode,
					 start, end,
					 count, type, indices ))
      _save_OBE_DrawElements( mode, count, type, indices );
}





static void _save_vtxfmt_init( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLvertexformat *vfmt = &tnl->save_vtxfmt;

   vfmt->ArrayElement = _ae_loopback_array_elt;	        /* generic helper */
   vfmt->Begin = _save_Begin;
   vfmt->Color3f = _save_Color3f;
   vfmt->Color3fv = _save_Color3fv;
   vfmt->Color4f = _save_Color4f;
   vfmt->Color4fv = _save_Color4fv;
   vfmt->EdgeFlag = _save_EdgeFlag;
   vfmt->EdgeFlagv = _save_EdgeFlagv;
   vfmt->End = _save_End;
   vfmt->FogCoordfEXT = _save_FogCoordfEXT;
   vfmt->FogCoordfvEXT = _save_FogCoordfvEXT;
   vfmt->Indexf = _save_Indexf;
   vfmt->Indexfv = _save_Indexfv;
   vfmt->Materialfv = _save_Materialfv;
   vfmt->MultiTexCoord1fARB = _save_MultiTexCoord1f;
   vfmt->MultiTexCoord1fvARB = _save_MultiTexCoord1fv;
   vfmt->MultiTexCoord2fARB = _save_MultiTexCoord2f;
   vfmt->MultiTexCoord2fvARB = _save_MultiTexCoord2fv;
   vfmt->MultiTexCoord3fARB = _save_MultiTexCoord3f;
   vfmt->MultiTexCoord3fvARB = _save_MultiTexCoord3fv;
   vfmt->MultiTexCoord4fARB = _save_MultiTexCoord4f;
   vfmt->MultiTexCoord4fvARB = _save_MultiTexCoord4fv;
   vfmt->Normal3f = _save_Normal3f;
   vfmt->Normal3fv = _save_Normal3fv;
   vfmt->SecondaryColor3fEXT = _save_SecondaryColor3fEXT;
   vfmt->SecondaryColor3fvEXT = _save_SecondaryColor3fvEXT;
   vfmt->TexCoord1f = _save_TexCoord1f;
   vfmt->TexCoord1fv = _save_TexCoord1fv;
   vfmt->TexCoord2f = _save_TexCoord2f;
   vfmt->TexCoord2fv = _save_TexCoord2fv;
   vfmt->TexCoord3f = _save_TexCoord3f;
   vfmt->TexCoord3fv = _save_TexCoord3fv;
   vfmt->TexCoord4f = _save_TexCoord4f;
   vfmt->TexCoord4fv = _save_TexCoord4fv;
   vfmt->Vertex2f = _save_Vertex2f;
   vfmt->Vertex2fv = _save_Vertex2fv;
   vfmt->Vertex3f = _save_Vertex3f;
   vfmt->Vertex3fv = _save_Vertex3fv;
   vfmt->Vertex4f = _save_Vertex4f;
   vfmt->Vertex4fv = _save_Vertex4fv;
   vfmt->VertexAttrib1fNV = _save_VertexAttrib1fNV;
   vfmt->VertexAttrib1fvNV = _save_VertexAttrib1fvNV;
   vfmt->VertexAttrib2fNV = _save_VertexAttrib2fNV;
   vfmt->VertexAttrib2fvNV = _save_VertexAttrib2fvNV;
   vfmt->VertexAttrib3fNV = _save_VertexAttrib3fNV;
   vfmt->VertexAttrib3fvNV = _save_VertexAttrib3fvNV;
   vfmt->VertexAttrib4fNV = _save_VertexAttrib4fNV;
   vfmt->VertexAttrib4fvNV = _save_VertexAttrib4fvNV;
   vfmt->VertexAttrib1fARB = _save_VertexAttrib1fARB;
   vfmt->VertexAttrib1fvARB = _save_VertexAttrib1fvARB;
   vfmt->VertexAttrib2fARB = _save_VertexAttrib2fARB;
   vfmt->VertexAttrib2fvARB = _save_VertexAttrib2fvARB;
   vfmt->VertexAttrib3fARB = _save_VertexAttrib3fARB;
   vfmt->VertexAttrib3fvARB = _save_VertexAttrib3fvARB;
   vfmt->VertexAttrib4fARB = _save_VertexAttrib4fARB;
   vfmt->VertexAttrib4fvARB = _save_VertexAttrib4fvARB;
   
   /* This will all require us to fallback to saving the list as opcodes:
    */ 
   vfmt->CallList = _save_CallList; /* inside begin/end */
   vfmt->CallLists = _save_CallLists; /* inside begin/end */
   vfmt->EvalCoord1f = _save_EvalCoord1f;
   vfmt->EvalCoord1fv = _save_EvalCoord1fv;
   vfmt->EvalCoord2f = _save_EvalCoord2f;
   vfmt->EvalCoord2fv = _save_EvalCoord2fv;
   vfmt->EvalPoint1 = _save_EvalPoint1;
   vfmt->EvalPoint2 = _save_EvalPoint2;

   /* These are all errors as we at least know we are in some sort of
    * begin/end pair:
    */
   vfmt->EvalMesh1 = _save_EvalMesh1;	
   vfmt->EvalMesh2 = _save_EvalMesh2;
   vfmt->Begin = _save_Begin;
   vfmt->Rectf = _save_Rectf;
   vfmt->DrawArrays = _save_DrawArrays;
   vfmt->DrawElements = _save_DrawElements;
   vfmt->DrawRangeElements = _save_DrawRangeElements;

}


void _tnl_SaveFlushVertices( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   /* Noop when we are actually active:
    */
   if (ctx->Driver.CurrentSavePrimitive == PRIM_INSIDE_UNKNOWN_PRIM ||
       ctx->Driver.CurrentSavePrimitive <= GL_POLYGON)
      return;

   if (tnl->save.initial_counter != tnl->save.counter ||
       tnl->save.prim_count) 
      _save_compile_vertex_list( ctx );
   
   _save_copy_to_current( ctx );
   _save_reset_vertex( ctx );
   ctx->Driver.SaveNeedFlush = 0;
}

void _tnl_NewList( GLcontext *ctx, GLuint list, GLenum mode )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   (void) list; (void) mode;

   if (!tnl->save.prim_store)
      tnl->save.prim_store = alloc_prim_store( ctx );

   if (!tnl->save.vertex_store) {
      tnl->save.vertex_store = alloc_vertex_store( ctx );
      tnl->save.vbptr = tnl->save.vertex_store->buffer;
   }
   
   _save_reset_vertex( ctx );
   ctx->Driver.SaveNeedFlush = 0;
}

void _tnl_EndList( GLcontext *ctx )
{
   (void) ctx;
   assert(TNL_CONTEXT(ctx)->save.vertex_size == 0);
}
 
void _tnl_BeginCallList( GLcontext *ctx, struct mesa_display_list *dlist )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   tnl->save.replay_flags |= dlist->flags;
   tnl->save.replay_flags |= tnl->LoopbackDListCassettes;
}

void _tnl_EndCallList( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   
   if (ctx->ListState.CallDepth == 1)
      tnl->save.replay_flags = 0;
}


static void _tnl_destroy_vertex_list( GLcontext *ctx, void *data )
{
   struct tnl_vertex_list *node = (struct tnl_vertex_list *)data;
   (void) ctx;

   if ( --node->vertex_store->refcount == 0 )
      FREE( node->vertex_store );

   if ( --node->prim_store->refcount == 0 )
      FREE( node->prim_store );

   if ( node->normal_lengths )
      FREE( node->normal_lengths );
}


static void _tnl_print_vertex_list( GLcontext *ctx, void *data )
{
   struct tnl_vertex_list *node = (struct tnl_vertex_list *)data;
   GLuint i;
   (void) ctx;

   _mesa_debug(NULL, "TNL-VERTEX-LIST, %u vertices %d primitives, %d vertsize\n",
               node->count,
	       node->prim_count,
	       node->vertex_size);

   for (i = 0 ; i < node->prim_count ; i++) {
      struct tnl_prim *prim = &node->prim[i];
      _mesa_debug(NULL, "   prim %d: %s %d..%d %s %s\n",
		  i, 
		  _mesa_lookup_enum_by_nr(prim->mode & PRIM_MODE_MASK),
		  prim->start, 
		  prim->start + prim->count,
		  (prim->mode & PRIM_BEGIN) ? "BEGIN" : "(wrap)",
		  (prim->mode & PRIM_END) ? "END" : "(wrap)");
   }
}


static void _save_current_init( GLcontext *ctx ) 
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLint i;

   for (i = 0; i < _TNL_ATTRIB_MAT_FRONT_AMBIENT; i++) {
      ASSERT(i < VERT_ATTRIB_MAX);
      tnl->save.currentsz[i] = &ctx->ListState.ActiveAttribSize[i];
      tnl->save.current[i] = ctx->ListState.CurrentAttrib[i];
   }

   for (i = _TNL_ATTRIB_MAT_FRONT_AMBIENT; i < _TNL_ATTRIB_INDEX; i++) {
      const GLuint j = i - _TNL_ATTRIB_MAT_FRONT_AMBIENT;
      ASSERT(j < MAT_ATTRIB_MAX);
      tnl->save.currentsz[i] = &ctx->ListState.ActiveMaterialSize[j];
      tnl->save.current[i] = ctx->ListState.CurrentMaterial[j];
   }

   tnl->save.currentsz[_TNL_ATTRIB_INDEX] = &ctx->ListState.ActiveIndex;
   tnl->save.current[_TNL_ATTRIB_INDEX] = &ctx->ListState.CurrentIndex;

   tnl->save.currentsz[_TNL_ATTRIB_EDGEFLAG] = &ctx->ListState.ActiveEdgeFlag;
   tnl->save.current[_TNL_ATTRIB_EDGEFLAG] = &tnl->save.CurrentFloatEdgeFlag;
}

/**
 * Initialize the display list compiler
 */
void _tnl_save_init( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct tnl_vertex_arrays *tmp = &tnl->save_inputs;
   GLuint i;


   for (i = 0; i < _TNL_ATTRIB_MAX; i++)
      _mesa_vector4f_init( &tmp->Attribs[i], 0, NULL);

   tnl->save.opcode_vertex_list =
      _mesa_alloc_opcode( ctx,
			  sizeof(struct tnl_vertex_list),
			  _tnl_playback_vertex_list,
			  _tnl_destroy_vertex_list,
			  _tnl_print_vertex_list );

   ctx->Driver.NotifySaveBegin = _save_NotifyBegin;

   _save_vtxfmt_init( ctx );
   _save_current_init( ctx );

   /* Hook our array functions into the outside-begin-end vtxfmt in 
    * ctx->ListState.
    */
   ctx->ListState.ListVtxfmt.Rectf = _save_OBE_Rectf;
   ctx->ListState.ListVtxfmt.DrawArrays = _save_OBE_DrawArrays;
   ctx->ListState.ListVtxfmt.DrawElements = _save_OBE_DrawElements;
   ctx->ListState.ListVtxfmt.DrawRangeElements = _save_OBE_DrawRangeElements;
   _mesa_install_save_vtxfmt( ctx, &ctx->ListState.ListVtxfmt );
}


/**
 * Deallocate the immediate-mode buffer for the given context, if
 * its reference count goes to zero.
 */
void _tnl_save_destroy( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   /* Decrement the refcounts.  References may still be held by
    * display lists yet to be destroyed, so it may not yet be time to
    * free these items.
    */
   if (tnl->save.prim_store &&
       --tnl->save.prim_store->refcount == 0 )
      FREE( tnl->save.prim_store );

   if (tnl->save.vertex_store &&
       --tnl->save.vertex_store->refcount == 0 )
      FREE( tnl->save.vertex_store );
}
