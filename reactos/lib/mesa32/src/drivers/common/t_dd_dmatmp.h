
/*
 * Mesa 3-D graphics library
 * Version:  3.5
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
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */


/* Template for render stages which build and emit vertices directly
 * to fixed-size dma buffers.  Useful for rendering strips and other
 * native primitives where clipping and per-vertex tweaks such as
 * those in t_dd_tritmp.h are not required.
 *
 * Produces code for both inline triangles and indexed triangles.
 * Where various primitive types are unaccelerated by hardware, the
 * code attempts to fallback to other primitive types (quadstrips to
 * tristrips, lineloops to linestrips), or to indexed vertices.
 * Ultimately, a FALLBACK() macro is invoked if there is no way to
 * render the primitive natively.
 */

#if !defined(HAVE_TRIANGLES)
#error "must have at least triangles to use render template"
#endif

#if !HAVE_ELTS
#define ELTS_VARS
#define ALLOC_ELTS( nr )
#define EMIT_ELT( offset, elt )
#define EMIT_TWO_ELTS( offset, elt0, elt1 )
#define INCR_ELTS( nr )
#define ELT_INIT(prim)
#define GET_CURRENT_VB_MAX_ELTS() 0
#define GET_SUBSEQUENT_VB_MAX_ELTS() 0
#define ALLOC_ELTS_NEW_PRIMITIVE(nr)
#define RELEASE_ELT_VERTS()
#define EMIT_INDEXED_VERTS( ctx, start, count )
#endif

#ifndef EMIT_TWO_ELTS
#define EMIT_TWO_ELTS( offset, elt0, elt1 )	\
do { 						\
   EMIT_ELT( offset, elt0 ); 			\
   EMIT_ELT( offset+1, elt1 ); 			\
} while (0)
#endif

#ifndef FINISH
#define FINISH
#endif

/**********************************************************************/
/*                  Render whole begin/end objects                    */
/**********************************************************************/



static GLboolean TAG(emit_elt_verts)( GLcontext *ctx,
				      GLuint start, GLuint count )
{
   if (HAVE_ELTS) {
      LOCAL_VARS;
      GLuint nr = count - start;

      if ( nr >= GET_SUBSEQUENT_VB_MAX_VERTS() ) /* assumes same packing for
						  * indexed and regualar verts
						  */
	 return GL_FALSE;

      NEW_PRIMITIVE(); /* finish last prim */
      EMIT_INDEXED_VERTS( ctx, start, count );
      return GL_TRUE;
   } else {
      return GL_FALSE;
   }
}

#if (HAVE_ELTS)
static void TAG(emit_elts)( GLcontext *ctx, GLuint *elts, GLuint nr )
{
   GLint i;
   LOCAL_VARS;
   ELTS_VARS;

   ALLOC_ELTS( nr );

   for ( i = 0 ; i < nr ; i+=2, elts += 2 ) {
      EMIT_TWO_ELTS( 0, elts[0], elts[1] );
      INCR_ELTS( 2 );
   }
}
#endif


/***********************************************************************
 *                    Render non-indexed primitives.
 ***********************************************************************/



static void TAG(render_points_verts)( GLcontext *ctx,
				      GLuint start,
				      GLuint count,
				      GLuint flags )
{
   if (HAVE_POINTS) {
      LOCAL_VARS;
      int dmasz = GET_SUBSEQUENT_VB_MAX_VERTS();
      int currentsz = GET_CURRENT_VB_MAX_VERTS();
      GLuint j, nr;

      INIT( GL_POINTS );

      if (currentsz < 8)
	 currentsz = dmasz;

      for (j = start; j < count; j += nr ) {
	 nr = MIN2( currentsz, count - j );
	 EMIT_VERTS( ctx, j, nr );
	 currentsz = dmasz;
      }

      FINISH;

   } else {
      VERT_FALLBACK( ctx, start, count, flags );
   }
}

static void TAG(render_lines_verts)( GLcontext *ctx,
				     GLuint start,
				     GLuint count,
				     GLuint flags )
{
   if (HAVE_LINES) {
      LOCAL_VARS;
      int dmasz = GET_SUBSEQUENT_VB_MAX_VERTS();
      int currentsz = GET_CURRENT_VB_MAX_VERTS();
      GLuint j, nr;

      INIT( GL_LINES );

      /* Emit whole number of lines in total and in each buffer:
       */
      count -= (count-start) & 1;
      currentsz -= currentsz & 1;
      dmasz -= dmasz & 1;

      if (currentsz < 8)
	 currentsz = dmasz;

      for (j = start; j < count; j += nr ) {
	 nr = MIN2( currentsz, count - j );
	 EMIT_VERTS( ctx, j, nr );
	 currentsz = dmasz;
      }

      FINISH;

   } else {
      VERT_FALLBACK( ctx, start, count, flags );
   }
}


static void TAG(render_line_strip_verts)( GLcontext *ctx,
					  GLuint start,
					  GLuint count,
					  GLuint flags )
{
   if (HAVE_LINE_STRIPS) {
      LOCAL_VARS;
      int dmasz = GET_SUBSEQUENT_VB_MAX_VERTS();
      int currentsz = GET_CURRENT_VB_MAX_VERTS();
      GLuint j, nr;

      NEW_PRIMITIVE(); /* always a new primitive */
      INIT( GL_LINE_STRIP );

      if (currentsz < 8)
	 currentsz = dmasz;

      for (j = start; j + 1 < count; j += nr - 1 ) {
	 nr = MIN2( currentsz, count - j );
	 EMIT_VERTS( ctx, j, nr );
	 currentsz = dmasz;
      }
 
      FINISH;

   } else {
      VERT_FALLBACK( ctx, start, count, flags );
   }
}


static void TAG(render_line_loop_verts)( GLcontext *ctx,
					 GLuint start,
					 GLuint count,
					 GLuint flags )
{
   if (HAVE_LINE_STRIPS) {
      LOCAL_VARS;
      int dmasz = GET_SUBSEQUENT_VB_MAX_VERTS();
      int currentsz = GET_CURRENT_VB_MAX_VERTS();
      GLuint j, nr;

      NEW_PRIMITIVE();
      INIT( GL_LINE_STRIP );

      if (flags & PRIM_BEGIN)
	 j = start;
      else
	 j = start + 1;

      /* Ensure last vertex won't wrap buffers:
       */
      currentsz--;
      dmasz--;

      if (currentsz < 8) {
	 NEW_BUFFER();
	 currentsz = dmasz;
      }

      if (j + 1 < count) {
	 for ( ; j + 1 < count; j += nr - 1 ) {
	    nr = MIN2( currentsz, count - j );
	    EMIT_VERTS( ctx, j, nr );
	    currentsz = dmasz;
	 }

	 if (start < count - 1 && (flags & PRIM_END))
	    EMIT_VERTS( ctx, start, 1 );
      }
      else if (start + 1 < count && (flags & PRIM_END)) {
	 EMIT_VERTS( ctx, start+1, 1 );
	 EMIT_VERTS( ctx, start, 1 );
      }

      FINISH;

   } else {
      VERT_FALLBACK( ctx, start, count, flags );
   }
}


static void TAG(render_triangles_verts)( GLcontext *ctx,
					 GLuint start,
					 GLuint count,
					 GLuint flags )
{
   LOCAL_VARS;
   int dmasz = (GET_SUBSEQUENT_VB_MAX_VERTS()/3) * 3;
   int currentsz = (GET_CURRENT_VB_MAX_VERTS()/3) * 3;
   GLuint j, nr;

   INIT(GL_TRIANGLES);

   /* Emit whole number of tris in total.  dmasz is already a multiple
    * of 3.
    */
   count -= (count-start)%3;

   if (currentsz < 8)
      currentsz = dmasz;

   for (j = start; j < count; j += nr) {
      nr = MIN2( currentsz, count - j );
      EMIT_VERTS( ctx, j, nr );
      currentsz = dmasz;
   }
   FINISH;
}



static void TAG(render_tri_strip_verts)( GLcontext *ctx,
					 GLuint start,
					 GLuint count,
					 GLuint flags )
{
   if (HAVE_TRI_STRIPS) {
      LOCAL_VARS;
      GLuint j, nr;
      int dmasz = GET_SUBSEQUENT_VB_MAX_VERTS();
      int currentsz;

      INIT(GL_TRIANGLE_STRIP);
      NEW_PRIMITIVE();

      currentsz = GET_CURRENT_VB_MAX_VERTS();

      if (currentsz < 8) {
	 NEW_BUFFER();
	 currentsz = dmasz;
      }

      /* From here on emit even numbers of tris when wrapping over buffers:
       */
      dmasz -= (dmasz & 1);
      currentsz -= (currentsz & 1);

      for (j = start ; j + 2 < count; j += nr - 2 ) {
	 nr = MIN2( currentsz, count - j );
	 EMIT_VERTS( ctx, j, nr );
	 currentsz = dmasz;
      }

      FINISH;

   } else {
      VERT_FALLBACK( ctx, start, count, flags );
   }
}

static void TAG(render_tri_fan_verts)( GLcontext *ctx,
				       GLuint start,
				       GLuint count,
				       GLuint flags )
{
   if (HAVE_TRI_FANS) {
      LOCAL_VARS;
      GLuint j, nr;
      int dmasz = GET_SUBSEQUENT_VB_MAX_VERTS();
      int currentsz = GET_CURRENT_VB_MAX_VERTS();

      NEW_PRIMITIVE();
      INIT(GL_TRIANGLE_FAN);

      if (currentsz < 8) {
	 NEW_BUFFER();
	 currentsz = dmasz;
      }

      for (j = start + 1 ; j + 1 < count; j += nr - 1 ) {
	 nr = MIN2( currentsz, count - j + 1 );
	 EMIT_VERTS( ctx, start, 1 );
	 EMIT_VERTS( ctx, j, nr - 1 );
	 currentsz = dmasz;
      }

      FINISH;

   }
   else {
      /* Could write code to emit these as indexed vertices (for the
       * g400, for instance).
       */
      VERT_FALLBACK( ctx, start, count, flags );
   }
}


static void TAG(render_poly_verts)( GLcontext *ctx,
				    GLuint start,
				    GLuint count,
				    GLuint flags )
{
   if (HAVE_POLYGONS) {
      LOCAL_VARS;
      GLuint j, nr;
      int dmasz = GET_SUBSEQUENT_VB_MAX_VERTS();
      int currentsz = GET_CURRENT_VB_MAX_VERTS();

      NEW_PRIMITIVE();
      INIT(GL_POLYGON);

      if (currentsz < 8) {
	 NEW_BUFFER();
	 currentsz = dmasz;
      }

      for (j = start + 1 ; j + 1 < count ; j += nr - 1 ) {
	 nr = MIN2( currentsz, count - j + 1 );
	 EMIT_VERTS( ctx, start, 1 );
	 EMIT_VERTS( ctx, j, nr - 1 );
	 currentsz = dmasz;
      }

      FINISH;

   }
   else if (HAVE_TRI_FANS && !(ctx->_TriangleCaps & DD_FLATSHADE)) {
      TAG(render_tri_fan_verts)( ctx, start, count, flags );
   } else {
      VERT_FALLBACK( ctx, start, count, flags );
   }
}

static void TAG(render_quad_strip_verts)( GLcontext *ctx,
					  GLuint start,
					  GLuint count,
					  GLuint flags )
{
   GLuint j, nr;

   if (HAVE_QUAD_STRIPS) {
      LOCAL_VARS;
      GLuint j, nr;
      int dmasz = GET_SUBSEQUENT_VB_MAX_VERTS();
      int currentsz;

      INIT(GL_QUAD_STRIP);
      NEW_PRIMITIVE();

      currentsz = GET_CURRENT_VB_MAX_VERTS();

      if (currentsz < 8) {
	 NEW_BUFFER();
	 currentsz = dmasz;
      }

      dmasz -= (dmasz & 2);
      currentsz -= (currentsz & 2);

      for (j = start ; j + 3 < count; j += nr - 2 ) {
	 nr = MIN2( currentsz, count - j );
	 EMIT_VERTS( ctx, j, nr );
	 currentsz = dmasz;
      }

      FINISH;

   } else if (HAVE_TRI_STRIPS && (ctx->_TriangleCaps & DD_FLATSHADE)) {
      if (TAG(emit_elt_verts)( ctx, start, count )) {
	 LOCAL_VARS;
	 int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
	 int currentsz;
	 GLuint j, nr;

	 /* Simulate flat-shaded quadstrips using indexed vertices:
	  */
	 NEW_PRIMITIVE();
	 ELT_INIT( GL_TRIANGLES );

	 currentsz = GET_CURRENT_VB_MAX_ELTS();

	 /* Emit whole number of quads in total, and in each buffer.
	  */
	 dmasz -= dmasz & 1;
	 count -= (count-start) & 1;
	 currentsz -= currentsz & 1;

	 if (currentsz < 12)
	    currentsz = dmasz;

	 currentsz = currentsz/6*2;
	 dmasz = dmasz/6*2;

	 for (j = start; j + 3 < count; j += nr - 2 ) {
	    nr = MIN2( currentsz, count - j );
	    if (nr >= 4) {
	       GLint quads = (nr/2)-1;
	       GLint i;
	       ELTS_VARS;

	       NEW_PRIMITIVE();
	       ALLOC_ELTS_NEW_PRIMITIVE( quads*6 );

	       for ( i = j-start ; i < j-start+quads*2 ; i+=2 ) {
		  EMIT_TWO_ELTS( 0, (i+0), (i+1) );
		  EMIT_TWO_ELTS( 2, (i+2), (i+1) );
		  EMIT_TWO_ELTS( 4, (i+3), (i+2) );
		  INCR_ELTS( 6 );
	       }

	       NEW_PRIMITIVE();
	    }
	    currentsz = dmasz;
	 }

	 RELEASE_ELT_VERTS();
      }
      else {
	 /* Vertices won't fit in a single buffer or elts not available,
	  * VERT_FALLBACK.
	  */
	 VERT_FALLBACK( ctx, start, count, flags );
      }
   }
   else if (HAVE_TRI_STRIPS) {
      LOCAL_VARS;
      int dmasz = GET_SUBSEQUENT_VB_MAX_VERTS();
      int currentsz = GET_CURRENT_VB_MAX_VERTS();

      /* Emit smooth-shaded quadstrips as tristrips:
       */
      NEW_PRIMITIVE();
      INIT( GL_TRIANGLE_STRIP );

      /* Emit whole number of quads in total, and in each buffer.
       */
      dmasz -= dmasz & 1;
      currentsz -= currentsz & 1;
      count -= (count-start) & 1;

      if (currentsz < 8) {
	 NEW_BUFFER();
	 currentsz = dmasz;
      }

      for (j = start; j + 3 < count; j += nr - 2 ) {
	 nr = MIN2( currentsz, count - j );
	 EMIT_VERTS( ctx, j, nr );
	 currentsz = dmasz;
      }

      FINISH;

   } else {
      VERT_FALLBACK( ctx, start, count, flags );
   }
}


static void TAG(render_quads_verts)( GLcontext *ctx,
				     GLuint start,
				     GLuint count,
				     GLuint flags )
{
   if (HAVE_QUADS) {
      LOCAL_VARS;
      int dmasz = (GET_SUBSEQUENT_VB_MAX_VERTS()/4) * 4;
      int currentsz = (GET_CURRENT_VB_MAX_VERTS()/4) * 4;
      GLuint j, nr;

      INIT(GL_QUADS);

      /* Emit whole number of quads in total.  dmasz is already a multiple
       * of 4.
       */
      count -= (count-start)%4;

      if (currentsz < 8)
         currentsz = dmasz;

      for (j = start; j < count; j += nr) {
         nr = MIN2( currentsz, count - j );
         EMIT_VERTS( ctx, j, nr );
         currentsz = dmasz;
      }
      FINISH;
   } else if (TAG(emit_elt_verts)( ctx, start, count )) {
      /* Hardware doesn't have a quad primitive type -- try to
       * simulate it using indexed vertices and the triangle
       * primitive:
       */
      LOCAL_VARS;
      int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
      int currentsz;
      GLuint j, nr;

      NEW_PRIMITIVE();
      ELT_INIT( GL_TRIANGLES );
      currentsz = GET_CURRENT_VB_MAX_ELTS();

      /* Emit whole number of quads in total, and in each buffer.
       */
      dmasz -= dmasz & 3;
      count -= (count-start) & 3;
      currentsz -= currentsz & 3;

      /* Adjust for rendering as triangles:
       */
      currentsz = currentsz/6*4;
      dmasz = dmasz/6*4;

      if (currentsz < 8)
	 currentsz = dmasz;

      for (j = start; j < count; j += nr ) {
	 nr = MIN2( currentsz, count - j );
	 if (nr >= 4) {
	    GLint quads = nr/4;
	    GLint i;
	    ELTS_VARS;

	    NEW_PRIMITIVE();
	    ALLOC_ELTS_NEW_PRIMITIVE( quads*6 );

	    for ( i = j-start ; i < j-start+quads*4 ; i+=4 ) {
	       EMIT_TWO_ELTS( 0, (i+0), (i+1) );
	       EMIT_TWO_ELTS( 2, (i+3), (i+1) );
	       EMIT_TWO_ELTS( 4, (i+2), (i+3) );
	       INCR_ELTS( 6 );
	    }

	    NEW_PRIMITIVE();
	 }
	 currentsz = dmasz;
      }

      RELEASE_ELT_VERTS();
   }
   else {
      /* Vertices won't fit in a single buffer, fallback.
       */
      VERT_FALLBACK( ctx, start, count, flags );
   }
}

static void TAG(render_noop)( GLcontext *ctx,
			      GLuint start,
			      GLuint count,
			      GLuint flags )
{
}




static render_func TAG(render_tab_verts)[GL_POLYGON+2] =
{
   TAG(render_points_verts),
   TAG(render_lines_verts),
   TAG(render_line_loop_verts),
   TAG(render_line_strip_verts),
   TAG(render_triangles_verts),
   TAG(render_tri_strip_verts),
   TAG(render_tri_fan_verts),
   TAG(render_quads_verts),
   TAG(render_quad_strip_verts),
   TAG(render_poly_verts),
   TAG(render_noop),
};


/****************************************************************************
 *                 Render elts using hardware indexed verts                 *
 ****************************************************************************/

#if (HAVE_ELTS)
static void TAG(render_points_elts)( GLcontext *ctx,
				     GLuint start,
				     GLuint count,
				     GLuint flags )
{
   if (HAVE_POINTS) {
      LOCAL_VARS;
      int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
      int currentsz;
      GLuint *elts = TNL_CONTEXT(ctx)->vb.Elts;
      GLuint j, nr;

      ELT_INIT( GL_POINTS );

      currentsz = GET_CURRENT_VB_MAX_ELTS();
      if (currentsz < 8)
	 currentsz = dmasz;

      for (j = start; j < count; j += nr ) {
	 nr = MIN2( currentsz, count - j );
	 TAG(emit_elts)( ctx, elts+j, nr );
	 NEW_PRIMITIVE();
	 currentsz = dmasz;
      }
   } else {
      ELT_FALLBACK( ctx, start, count, flags );
   }
}



static void TAG(render_lines_elts)( GLcontext *ctx,
				    GLuint start,
				    GLuint count,
				    GLuint flags )
{
   if (HAVE_LINES) {
      LOCAL_VARS;
      int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
      int currentsz;
      GLuint *elts = TNL_CONTEXT(ctx)->vb.Elts;
      GLuint j, nr;

      ELT_INIT( GL_LINES );

      /* Emit whole number of lines in total and in each buffer:
       */
      count -= (count-start) & 1;
      currentsz -= currentsz & 1;
      dmasz -= dmasz & 1;

      currentsz = GET_CURRENT_VB_MAX_ELTS();
      if (currentsz < 8)
	 currentsz = dmasz;

      for (j = start; j < count; j += nr ) {
	 nr = MIN2( currentsz, count - j );
	 TAG(emit_elts)( ctx, elts+j, nr );
	 NEW_PRIMITIVE();
	 currentsz = dmasz;
      }
   } else {
      ELT_FALLBACK( ctx, start, count, flags );
   }
}


static void TAG(render_line_strip_elts)( GLcontext *ctx,
					 GLuint start,
					 GLuint count,
					 GLuint flags )
{
   if (HAVE_LINE_STRIPS) {
      LOCAL_VARS;
      int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
      int currentsz;
      GLuint *elts = TNL_CONTEXT(ctx)->vb.Elts;
      GLuint j, nr;

      NEW_PRIMITIVE(); /* always a new primitive */
      ELT_INIT( GL_LINE_STRIP );

      currentsz = GET_CURRENT_VB_MAX_ELTS();
      if (currentsz < 8)
	 currentsz = dmasz;

      for (j = start; j + 1 < count; j += nr - 1 ) {
	 nr = MIN2( currentsz, count - j );
	 TAG(emit_elts)( ctx, elts+j, nr );
	 NEW_PRIMITIVE();
	 currentsz = dmasz;
      }
   } else {
      /* TODO: Try to emit as indexed lines.
       */
      ELT_FALLBACK( ctx, start, count, flags );
   }
}


static void TAG(render_line_loop_elts)( GLcontext *ctx,
					GLuint start,
					GLuint count,
					GLuint flags )
{
   if (HAVE_LINE_STRIPS) {
      LOCAL_VARS;
      int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
      int currentsz;
      GLuint *elts = TNL_CONTEXT(ctx)->vb.Elts;
      GLuint j, nr;

      NEW_PRIMITIVE();
      ELT_INIT( GL_LINE_STRIP );

      if (flags & PRIM_BEGIN)
	 j = start;
      else
	 j = start + 1;

      currentsz = GET_CURRENT_VB_MAX_ELTS();
      if (currentsz < 8) {
	 NEW_BUFFER();
	 currentsz = dmasz;
      }

      /* Ensure last vertex doesn't wrap:
       */
      currentsz--;
      dmasz--;

      for ( ; j + 1 < count; j += nr - 1 ) {
	 nr = MIN2( currentsz, count - j );
/*  	 NEW_PRIMITIVE(); */
	 TAG(emit_elts)( ctx, elts+j, nr );
	 currentsz = dmasz;
      }

      if (flags & PRIM_END)
	 TAG(emit_elts)( ctx, elts+start, 1 );

      NEW_PRIMITIVE();
   } else {
      /* TODO: Try to emit as indexed lines */
      ELT_FALLBACK( ctx, start, count, flags );
   }
}


/* For verts, we still eliminate the copy from main memory to dma
 * buffers.  For elts, this is probably no better (worse?) than the
 * standard path.
 */
static void TAG(render_triangles_elts)( GLcontext *ctx,
					GLuint start,
					GLuint count,
					GLuint flags )
{
   LOCAL_VARS;
   GLuint *elts = TNL_CONTEXT(ctx)->vb.Elts;
   int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS()/3*3;
   int currentsz;
   GLuint j, nr;

   NEW_PRIMITIVE();
   ELT_INIT( GL_TRIANGLES );

   currentsz = GET_CURRENT_VB_MAX_ELTS();

   /* Emit whole number of tris in total.  dmasz is already a multiple
    * of 3.
    */
   count -= (count-start)%3;
   currentsz -= currentsz%3;
   if (currentsz < 8)
      currentsz = dmasz;

   for (j = start; j < count; j += nr) {
      nr = MIN2( currentsz, count - j );
      TAG(emit_elts)( ctx, elts+j, nr );
      NEW_PRIMITIVE();
      currentsz = dmasz;
   }
}



static void TAG(render_tri_strip_elts)( GLcontext *ctx,
					GLuint start,
					GLuint count,
					GLuint flags )
{
   if (HAVE_TRI_STRIPS) {
      LOCAL_VARS;
      GLuint j, nr;
      GLuint *elts = TNL_CONTEXT(ctx)->vb.Elts;
      int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
      int currentsz;

      NEW_PRIMITIVE();
      ELT_INIT( GL_TRIANGLE_STRIP );

      currentsz = GET_CURRENT_VB_MAX_ELTS();
      if (currentsz < 8) {
	 NEW_BUFFER();
	 currentsz = dmasz;
      }

      /* Keep the same winding over multiple buffers:
       */
      dmasz -= (dmasz & 1);
      currentsz -= (currentsz & 1);

      for (j = start ; j + 2 < count; j += nr - 2 ) {
	 nr = MIN2( currentsz, count - j );
	 TAG(emit_elts)( ctx, elts+j, nr );
	 NEW_PRIMITIVE();
	 currentsz = dmasz;
      }
   } else {
      /* TODO: try to emit as indexed triangles */
      ELT_FALLBACK( ctx, start, count, flags );
   }
}

static void TAG(render_tri_fan_elts)( GLcontext *ctx,
				      GLuint start,
				      GLuint count,
				      GLuint flags )
{
   if (HAVE_TRI_FANS) {
      LOCAL_VARS;
      GLuint *elts = TNL_CONTEXT(ctx)->vb.Elts;
      GLuint j, nr;
      int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
      int currentsz;

      NEW_PRIMITIVE();
      ELT_INIT( GL_TRIANGLE_FAN );

      currentsz = GET_CURRENT_VB_MAX_ELTS();
      if (currentsz < 8) {
	 NEW_BUFFER();
	 currentsz = dmasz;
      }

      for (j = start + 1 ; j + 1 < count; j += nr - 1 ) {
	 nr = MIN2( currentsz, count - j + 1 );
	 TAG(emit_elts)( ctx, elts+start, 1 );
	 TAG(emit_elts)( ctx, elts+j, nr - 1 );
	 NEW_PRIMITIVE();
	 currentsz = dmasz;
      }
   } else {
      /* TODO: try to emit as indexed triangles */
      ELT_FALLBACK( ctx, start, count, flags );
   }
}


static void TAG(render_poly_elts)( GLcontext *ctx,
				   GLuint start,
				   GLuint count,
				   GLuint flags )
{
   if (HAVE_POLYGONS && 0) {
   } else if (HAVE_TRI_FANS && !(ctx->_TriangleCaps & DD_FLATSHADE)) {
      LOCAL_VARS;
      GLuint *elts = TNL_CONTEXT(ctx)->vb.Elts;
      GLuint j, nr;
      int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
      int currentsz;

      NEW_PRIMITIVE();
      ELT_INIT( GL_TRIANGLE_FAN );

      currentsz = GET_CURRENT_VB_MAX_ELTS();
      if (currentsz < 8) {
	 NEW_BUFFER();
	 currentsz = dmasz;
      }

      for (j = start + 1 ; j + 1 < count ; j += nr - 1 ) {
	 nr = MIN2( currentsz, count - j + 1 );
	 TAG(emit_elts)( ctx, elts+start, 1 );
	 TAG(emit_elts)( ctx, elts+j, nr - 1 );
	 NEW_PRIMITIVE();
	 currentsz = dmasz;
      }
   } else {
      ELT_FALLBACK( ctx, start, count, flags );
   }
}

static void TAG(render_quad_strip_elts)( GLcontext *ctx,
					 GLuint start,
					 GLuint count,
					 GLuint flags )
{
   if (HAVE_QUAD_STRIPS && 0) {
   }
   else if (HAVE_TRI_STRIPS) {
      LOCAL_VARS;
      GLuint *elts = TNL_CONTEXT(ctx)->vb.Elts;
      int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
      int currentsz;
      GLuint j, nr;

      NEW_PRIMITIVE();
      currentsz = GET_CURRENT_VB_MAX_ELTS();

      /* Emit whole number of quads in total, and in each buffer.
       */
      dmasz -= dmasz & 1;
      count -= (count-start) & 1;
      currentsz -= currentsz & 1;

      if (currentsz < 12)
	 currentsz = dmasz;

      if (ctx->_TriangleCaps & DD_FLATSHADE) {
	 ELT_INIT( GL_TRIANGLES );

	 currentsz = currentsz/6*2;
	 dmasz = dmasz/6*2;

	 for (j = start; j + 3 < count; j += nr - 2 ) {
	    nr = MIN2( currentsz, count - j );

	    if (nr >= 4)
	    {
	       GLint i;
	       GLint quads = (nr/2)-1;
	       ELTS_VARS;


	       NEW_PRIMITIVE();
	       ALLOC_ELTS_NEW_PRIMITIVE( quads*6 );

	       for ( i = j-start ; i < j-start+quads ; i++, elts += 2 ) {
		  EMIT_TWO_ELTS( 0, elts[0], elts[1] );
		  EMIT_TWO_ELTS( 2, elts[2], elts[1] );
		  EMIT_TWO_ELTS( 4, elts[3], elts[2] );
		  INCR_ELTS( 6 );
	       }

	       NEW_PRIMITIVE();
	    }

	    currentsz = dmasz;
	 }
      }
      else {
	 ELT_INIT( GL_TRIANGLE_STRIP );

	 for (j = start; j + 3 < count; j += nr - 2 ) {
	    nr = MIN2( currentsz, count - j );
	    TAG(emit_elts)( ctx, elts+j, nr );
	    NEW_PRIMITIVE();
	    currentsz = dmasz;
	 }
      }
   }
}


static void TAG(render_quads_elts)( GLcontext *ctx,
				    GLuint start,
				    GLuint count,
				    GLuint flags )
{
   if (HAVE_QUADS && 0) {
   } else {
      LOCAL_VARS;
      GLuint *elts = TNL_CONTEXT(ctx)->vb.Elts;
      int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
      int currentsz;
      GLuint j, nr;

      ELT_INIT( GL_TRIANGLES );
      currentsz = GET_CURRENT_VB_MAX_ELTS();

      /* Emit whole number of quads in total, and in each buffer.
       */
      dmasz -= dmasz & 3;
      count -= (count-start) & 3;
      currentsz -= currentsz & 3;

      /* Adjust for rendering as triangles:
       */
      currentsz = currentsz/6*4;
      dmasz = dmasz/6*4;

      if (currentsz < 8)
	 currentsz = dmasz;

      for (j = start; j + 3 < count; j += nr - 2 ) {
	 nr = MIN2( currentsz, count - j );

	 if (nr >= 4)
	 {
	    GLint quads = nr/4;
	    GLint i;
	    ELTS_VARS;
	    NEW_PRIMITIVE();
	    ALLOC_ELTS_NEW_PRIMITIVE( quads * 6 );

	    for ( i = j-start ; i < j-start+quads ; i++, elts += 4 ) {
	       EMIT_TWO_ELTS( 0, elts[0], elts[1] );
	       EMIT_TWO_ELTS( 2, elts[3], elts[1] );
	       EMIT_TWO_ELTS( 4, elts[2], elts[3] );
	       INCR_ELTS( 6 );
	    }
	 }

	 NEW_PRIMITIVE();
	 currentsz = dmasz;
      }
   }
}



static render_func TAG(render_tab_elts)[GL_POLYGON+2] =
{
   TAG(render_points_elts),
   TAG(render_lines_elts),
   TAG(render_line_loop_elts),
   TAG(render_line_strip_elts),
   TAG(render_triangles_elts),
   TAG(render_tri_strip_elts),
   TAG(render_tri_fan_elts),
   TAG(render_quads_elts),
   TAG(render_quad_strip_elts),
   TAG(render_poly_elts),
   TAG(render_noop),
};
#endif
