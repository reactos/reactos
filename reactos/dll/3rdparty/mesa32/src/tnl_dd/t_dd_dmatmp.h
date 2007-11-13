/*
 * Mesa 3-D graphics library
 * Version:  6.5.1
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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


/**
 * \file t_dd_dmatmp.h
 * Template for render stages which build and emit vertices directly
 * to fixed-size dma buffers.  Useful for rendering strips and other
 * native primitives where clipping and per-vertex tweaks such as
 * those in t_dd_tritmp.h are not required.
 *
 * Produces code for both inline triangles and indexed triangles.
 * Where various primitive types are unaccelerated by hardware, the
 * code attempts to fallback to other primitive types (quadstrips to
 * tristrips, lineloops to linestrips), or to indexed vertices.
 */

#if !defined(HAVE_TRIANGLES)
#error "must have at least triangles to use render template"
#endif

#if !HAVE_ELTS
#define ELTS_VARS(buf)
#define ALLOC_ELTS(nr) 0
#define EMIT_ELT( offset, elt )
#define EMIT_TWO_ELTS( offset, elt0, elt1 )
#define INCR_ELTS( nr )
#define ELT_INIT(prim)
#define GET_CURRENT_VB_MAX_ELTS() 0
#define GET_SUBSEQUENT_VB_MAX_ELTS() 0
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


/**********************************************************************/
/*                  Render whole begin/end objects                    */
/**********************************************************************/




#if (HAVE_ELTS)
static void *TAG(emit_elts)( GLcontext *ctx, GLuint *elts, GLuint nr,
			     void *buf)
{
   GLint i;
   LOCAL_VARS;
   ELTS_VARS(buf);

   for ( i = 0 ; i+1 < nr ; i+=2, elts += 2 ) {
      EMIT_TWO_ELTS( 0, elts[0], elts[1] );
      INCR_ELTS( 2 );
   }
   
   if (i < nr) {
      EMIT_ELT( 0, elts[0] );
      INCR_ELTS( 1 );
   }

   return (void *)ELTPTR;
}
#endif

static __inline void *TAG(emit_verts)( GLcontext *ctx, GLuint start, 
				     GLuint count, void *buf )
{
   return EMIT_VERTS(ctx, start, count, buf);
}

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
      int currentsz;
      GLuint j, nr;

      INIT( GL_POINTS );

      currentsz = GET_CURRENT_VB_MAX_VERTS();
      if (currentsz < 8)
	 currentsz = dmasz;

      for (j = start; j < count; j += nr ) {
	 nr = MIN2( currentsz, count - j );
	 TAG(emit_verts)( ctx, j, nr, ALLOC_VERTS(nr) );
	 currentsz = dmasz;
      }

   } else {
      fprintf(stderr, "%s - cannot draw primitive\n", __FUNCTION__);
      return;
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
      int currentsz;
      GLuint j, nr;

      INIT( GL_LINES );

      /* Emit whole number of lines in total and in each buffer:
       */
      count -= (count-start) & 1;
      currentsz = GET_CURRENT_VB_MAX_VERTS();
      currentsz -= currentsz & 1;
      dmasz -= dmasz & 1;

      if (currentsz < 8)
	 currentsz = dmasz;

      for (j = start; j < count; j += nr ) {
	 nr = MIN2( currentsz, count - j );
	 TAG(emit_verts)( ctx, j, nr, ALLOC_VERTS(nr) );
	 currentsz = dmasz;
      }

   } else {
      fprintf(stderr, "%s - cannot draw primitive\n", __FUNCTION__);
      return;
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
      int currentsz;
      GLuint j, nr;

      INIT( GL_LINE_STRIP );

      currentsz = GET_CURRENT_VB_MAX_VERTS();
      if (currentsz < 8)
	 currentsz = dmasz;

      for (j = start; j + 1 < count; j += nr - 1 ) {
	 nr = MIN2( currentsz, count - j );
	 TAG(emit_verts)( ctx, j, nr, ALLOC_VERTS(nr) );
	 currentsz = dmasz;
      }
 
      FLUSH();

   } else {
      fprintf(stderr, "%s - cannot draw primitive\n", __FUNCTION__);
      return;
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
      int currentsz;
      GLuint j, nr;

      INIT( GL_LINE_STRIP );

      if (flags & PRIM_BEGIN)
	 j = start;
      else
	 j = start + 1;

      /* Ensure last vertex won't wrap buffers:
       */
      currentsz = GET_CURRENT_VB_MAX_VERTS();
      currentsz--;
      dmasz--;

      if (currentsz < 8) {
	 currentsz = dmasz;
      }

      if (j + 1 < count) {
	 for ( ; j + 1 < count; j += nr - 1 ) {
	    nr = MIN2( currentsz, count - j );

	    if (j + nr >= count &&
		start < count - 1 && 
		(flags & PRIM_END)) 
	    {
	       void *tmp;
	       tmp = ALLOC_VERTS(nr+1);
	       tmp = TAG(emit_verts)( ctx, j, nr, tmp );
	       tmp = TAG(emit_verts)( ctx, start, 1, tmp );
	    }
	    else {
	       TAG(emit_verts)( ctx, j, nr, ALLOC_VERTS(nr) );
	       currentsz = dmasz;
	    }
	 }

      }
      else if (start + 1 < count && (flags & PRIM_END)) {
	 void *tmp;
	 tmp = ALLOC_VERTS(2);
	 tmp = TAG(emit_verts)( ctx, start+1, 1, tmp );
	 tmp = TAG(emit_verts)( ctx, start, 1, tmp );
      }

      FLUSH();

   } else {
      fprintf(stderr, "%s - cannot draw primitive\n", __FUNCTION__);
      return;
   }
}


static void TAG(render_triangles_verts)( GLcontext *ctx,
					 GLuint start,
					 GLuint count,
					 GLuint flags )
{
   LOCAL_VARS;
   int dmasz = (GET_SUBSEQUENT_VB_MAX_VERTS()/3) * 3;
   int currentsz;
   GLuint j, nr;

   INIT(GL_TRIANGLES);

   currentsz = (GET_CURRENT_VB_MAX_VERTS()/3) * 3;

   /* Emit whole number of tris in total.  dmasz is already a multiple
    * of 3.
    */
   count -= (count-start)%3;

   if (currentsz < 8)
      currentsz = dmasz;

   for (j = start; j < count; j += nr) {
      nr = MIN2( currentsz, count - j );
      TAG(emit_verts)( ctx, j, nr, ALLOC_VERTS(nr) );
      currentsz = dmasz;
   }
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

      currentsz = GET_CURRENT_VB_MAX_VERTS();

      if (currentsz < 8) {
	 currentsz = dmasz;
      }

      /* From here on emit even numbers of tris when wrapping over buffers:
       */
      dmasz -= (dmasz & 1);
      currentsz -= (currentsz & 1);

      for (j = start ; j + 2 < count; j += nr - 2 ) {
	 nr = MIN2( currentsz, count - j );
	 TAG(emit_verts)( ctx, j, nr, ALLOC_VERTS(nr) );
	 currentsz = dmasz;
      }

      FLUSH();

   } else {
      fprintf(stderr, "%s - cannot draw primitive\n", __FUNCTION__);
      return;
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
      int currentsz;

      INIT(GL_TRIANGLE_FAN);

      currentsz = GET_CURRENT_VB_MAX_VERTS();
      if (currentsz < 8) {
	 currentsz = dmasz;
      }

      for (j = start + 1 ; j + 1 < count; j += nr - 2 ) {
	 void *tmp;
	 nr = MIN2( currentsz, count - j + 1 );
	 tmp = ALLOC_VERTS( nr );
	 tmp = TAG(emit_verts)( ctx, start, 1, tmp );
	 tmp = TAG(emit_verts)( ctx, j, nr - 1, tmp );
	 currentsz = dmasz;
      }

      FLUSH();
   }
   else {
      /* Could write code to emit these as indexed vertices (for the
       * g400, for instance).
       */
      fprintf(stderr, "%s - cannot draw primitive\n", __FUNCTION__);
      return;
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
      int currentsz;

      INIT(GL_POLYGON);

      currentsz = GET_CURRENT_VB_MAX_VERTS();
      if (currentsz < 8) {
	 currentsz = dmasz;
      }

      for (j = start + 1 ; j + 1 < count ; j += nr - 2 ) {
	 void *tmp;
	 nr = MIN2( currentsz, count - j + 1 );
	 tmp = ALLOC_VERTS( nr );
	 tmp = TAG(emit_verts)( ctx, start, 1, tmp );
	 tmp = TAG(emit_verts)( ctx, j, nr - 1, tmp );
	 currentsz = dmasz;
      }

      FLUSH();
   }
   else if (HAVE_TRI_FANS && ctx->Light.ShadeModel == GL_SMOOTH) {
      TAG(render_tri_fan_verts)( ctx, start, count, flags );
   } else {
      fprintf(stderr, "%s - cannot draw primitive\n", __FUNCTION__);
      return;
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

      currentsz = GET_CURRENT_VB_MAX_VERTS();
      if (currentsz < 8) {
	 currentsz = dmasz;
      }

      dmasz -= (dmasz & 2);
      currentsz -= (currentsz & 2);

      for (j = start ; j + 3 < count; j += nr - 2 ) {
	 nr = MIN2( currentsz, count - j );
	 TAG(emit_verts)( ctx, j, nr, ALLOC_VERTS(nr) );
	 currentsz = dmasz;
      }

      FLUSH();

   } else if (HAVE_TRI_STRIPS && 
	      ctx->Light.ShadeModel == GL_FLAT &&
	      TNL_CONTEXT(ctx)->vb.ColorPtr[0]->stride) {
      if (HAVE_ELTS) {
	 LOCAL_VARS;
	 int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
	 int currentsz;
	 GLuint j, nr;

         EMIT_INDEXED_VERTS( ctx, start, count );

	 /* Simulate flat-shaded quadstrips using indexed vertices:
	  */
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
	       ELTS_VARS( ALLOC_ELTS( quads*6 ) );

	       for ( i = j-start ; i < j-start+quads*2 ; i+=2 ) {
		  EMIT_TWO_ELTS( 0, (i+0), (i+1) );
		  EMIT_TWO_ELTS( 2, (i+2), (i+1) );
		  EMIT_TWO_ELTS( 4, (i+3), (i+2) );
		  INCR_ELTS( 6 );
	       }

	       FLUSH();
	    }
	    currentsz = dmasz;
	 }

	 RELEASE_ELT_VERTS();
	 FLUSH();
      }
      else {
	 /* Vertices won't fit in a single buffer or elts not
	  * available - should never happen.
	  */
	 fprintf(stderr, "%s - cannot draw primitive\n", __FUNCTION__);
	 return;
      }
   }
   else if (HAVE_TRI_STRIPS) {
      LOCAL_VARS;
      int dmasz = GET_SUBSEQUENT_VB_MAX_VERTS();
      int currentsz;

      /* Emit smooth-shaded quadstrips as tristrips:
       */
      FLUSH();
      INIT( GL_TRIANGLE_STRIP );

      /* Emit whole number of quads in total, and in each buffer.
       */
      dmasz -= dmasz & 1;
      currentsz = GET_CURRENT_VB_MAX_VERTS();
      currentsz -= currentsz & 1;
      count -= (count-start) & 1;

      if (currentsz < 8) {
	 currentsz = dmasz;
      }

      for (j = start; j + 3 < count; j += nr - 2 ) {
	 nr = MIN2( currentsz, count - j );
	 TAG(emit_verts)( ctx, j, nr, ALLOC_VERTS(nr) );
	 currentsz = dmasz;
      }

      FLUSH();

   } else {
      fprintf(stderr, "%s - cannot draw primitive\n", __FUNCTION__);
      return;
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
      int currentsz;
      GLuint j, nr;

      INIT(GL_QUADS);

      /* Emit whole number of quads in total.  dmasz is already a multiple
       * of 4.
       */
      count -= (count-start)%4;

      currentsz = (GET_CURRENT_VB_MAX_VERTS()/4) * 4;
      if (currentsz < 8)
         currentsz = dmasz;

      for (j = start; j < count; j += nr) {
         nr = MIN2( currentsz, count - j );
         TAG(emit_verts)( ctx, j, nr, ALLOC_VERTS(nr) );
         currentsz = dmasz;
      }
   }
   else if (HAVE_ELTS) {
      /* Hardware doesn't have a quad primitive type -- try to
       * simulate it using indexed vertices and the triangle
       * primitive:
       */
      LOCAL_VARS;
      int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
      int currentsz;
      GLuint j, nr;

      EMIT_INDEXED_VERTS( ctx, start, count );

      FLUSH();
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
	    ELTS_VARS( ALLOC_ELTS( quads*6 ) );

	    for ( i = j-start ; i < j-start+quads*4 ; i+=4 ) {
	       EMIT_TWO_ELTS( 0, (i+0), (i+1) );
	       EMIT_TWO_ELTS( 2, (i+3), (i+1) );
	       EMIT_TWO_ELTS( 4, (i+2), (i+3) );
	       INCR_ELTS( 6 );
	    }

	    FLUSH();
	 }
	 currentsz = dmasz;
      }

      RELEASE_ELT_VERTS();
   }
   else if (HAVE_TRIANGLES) {
      /* Hardware doesn't have a quad primitive type -- try to
       * simulate it using triangle primitive.  This is a win for
       * gears, but is it useful in the broader world?
       */
      LOCAL_VARS;
      GLuint j;

      INIT(GL_TRIANGLES);

      for (j = start; j < count-3; j += 4) {
	 void *tmp = ALLOC_VERTS( 6 );
	 /* Send v0, v1, v3
	  */
	 tmp = EMIT_VERTS(ctx, j,     2, tmp);
	 tmp = EMIT_VERTS(ctx, j + 3, 1, tmp);
	 /* Send v1, v2, v3
	  */
	 tmp = EMIT_VERTS(ctx, j + 1, 3, tmp);
      }
   }
   else {
      /* Vertices won't fit in a single buffer, should never happen.
       */
      fprintf(stderr, "%s - cannot draw primitive\n", __FUNCTION__);
      return;
   }
}

static void TAG(render_noop)( GLcontext *ctx,
			      GLuint start,
			      GLuint count,
			      GLuint flags )
{
}




static tnl_render_func TAG(render_tab_verts)[GL_POLYGON+2] =
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
	 TAG(emit_elts)( ctx, elts+j, nr, ALLOC_ELTS(nr) );
	 FLUSH();
	 currentsz = dmasz;
      }
   } else {
      fprintf(stderr, "%s - cannot draw primitive\n", __FUNCTION__);
      return;
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
	 TAG(emit_elts)( ctx, elts+j, nr, ALLOC_ELTS(nr) );
	 FLUSH();
	 currentsz = dmasz;
      }
   } else {
      fprintf(stderr, "%s - cannot draw primitive\n", __FUNCTION__);
      return;
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

      FLUSH(); /* always a new primitive */
      ELT_INIT( GL_LINE_STRIP );

      currentsz = GET_CURRENT_VB_MAX_ELTS();
      if (currentsz < 8)
	 currentsz = dmasz;

      for (j = start; j + 1 < count; j += nr - 1 ) {
	 nr = MIN2( currentsz, count - j );
	 TAG(emit_elts)( ctx, elts+j, nr, ALLOC_ELTS(nr) );
	 FLUSH();
	 currentsz = dmasz;
      }
   } else {
      /* TODO: Try to emit as indexed lines.
       */
      fprintf(stderr, "%s - cannot draw primitive\n", __FUNCTION__);
      return;
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

      FLUSH();
      ELT_INIT( GL_LINE_STRIP );

      if (flags & PRIM_BEGIN)
	 j = start;
      else
	 j = start + 1;

      currentsz = GET_CURRENT_VB_MAX_ELTS();
      if (currentsz < 8) {
	 currentsz = dmasz;
      }

      /* Ensure last vertex doesn't wrap:
       */
      currentsz--;
      dmasz--;

      if (j + 1 < count) {
	 for ( ; j + 1 < count; j += nr - 1 ) {
	    nr = MIN2( currentsz, count - j );

	    if (j + nr >= count &&
		start < count - 1 && 
		(flags & PRIM_END)) 
	    {
	       void *tmp;
	       tmp = ALLOC_ELTS(nr+1);
	       tmp = TAG(emit_elts)( ctx, elts+j, nr, tmp );
	       tmp = TAG(emit_elts)( ctx, elts+start, 1, tmp );
	    }
	    else {
	       TAG(emit_elts)( ctx, elts+j, nr, ALLOC_ELTS(nr) );
	       currentsz = dmasz;
	    }
	 }

      }
      else if (start + 1 < count && (flags & PRIM_END)) {
	 void *tmp;
	 tmp = ALLOC_ELTS(2);
	 tmp = TAG(emit_elts)( ctx, elts+start+1, 1, tmp );
	 tmp = TAG(emit_elts)( ctx, elts+start, 1, tmp );
      }

      FLUSH();
   } else {
      /* TODO: Try to emit as indexed lines */
      fprintf(stderr, "%s - cannot draw primitive\n", __FUNCTION__);
      return;
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

   FLUSH();
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
      TAG(emit_elts)( ctx, elts+j, nr, ALLOC_ELTS(nr) );
      FLUSH();
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

      FLUSH();
      ELT_INIT( GL_TRIANGLE_STRIP );

      currentsz = GET_CURRENT_VB_MAX_ELTS();
      if (currentsz < 8) {
	 currentsz = dmasz;
      }

      /* Keep the same winding over multiple buffers:
       */
      dmasz -= (dmasz & 1);
      currentsz -= (currentsz & 1);

      for (j = start ; j + 2 < count; j += nr - 2 ) {
	 nr = MIN2( currentsz, count - j );
	 TAG(emit_elts)( ctx, elts+j, nr, ALLOC_ELTS(nr) );
	 FLUSH();
	 currentsz = dmasz;
      }
   } else {
      /* TODO: try to emit as indexed triangles */
      fprintf(stderr, "%s - cannot draw primitive\n", __FUNCTION__);
      return;
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

      FLUSH();
      ELT_INIT( GL_TRIANGLE_FAN );

      currentsz = GET_CURRENT_VB_MAX_ELTS();
      if (currentsz < 8) {
	 currentsz = dmasz;
      }

      for (j = start + 1 ; j + 1 < count; j += nr - 2 ) {
	 void *tmp;
	 nr = MIN2( currentsz, count - j + 1 );
	 tmp = ALLOC_ELTS( nr );
	 tmp = TAG(emit_elts)( ctx, elts+start, 1, tmp );
	 tmp = TAG(emit_elts)( ctx, elts+j, nr - 1, tmp );
	 FLUSH();
	 currentsz = dmasz;
      }
   } else {
      /* TODO: try to emit as indexed triangles */
      fprintf(stderr, "%s - cannot draw primitive\n", __FUNCTION__);
      return;
   }
}


static void TAG(render_poly_elts)( GLcontext *ctx,
				   GLuint start,
				   GLuint count,
				   GLuint flags )
{
   if (HAVE_POLYGONS) {
      LOCAL_VARS;
      GLuint *elts = TNL_CONTEXT(ctx)->vb.Elts;
      GLuint j, nr;
      int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS();
      int currentsz;

      FLUSH();
      ELT_INIT( GL_POLYGON );

      currentsz = GET_CURRENT_VB_MAX_ELTS();
      if (currentsz < 8) {
	 currentsz = dmasz;
      }

      for (j = start + 1 ; j + 1 < count; j += nr - 2 ) {
	 void *tmp;
	 nr = MIN2( currentsz, count - j + 1 );
	 tmp = ALLOC_ELTS( nr );
	 tmp = TAG(emit_elts)( ctx, elts+start, 1, tmp );
	 tmp = TAG(emit_elts)( ctx, elts+j, nr - 1, tmp );
	 FLUSH();
	 currentsz = dmasz;
      }
   } else if (HAVE_TRI_FANS && ctx->Light.ShadeModel == GL_SMOOTH) {
      TAG(render_tri_fan_verts)( ctx, start, count, flags );
   } else {
      fprintf(stderr, "%s - cannot draw primitive\n", __FUNCTION__);
      return;
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

      FLUSH();
      currentsz = GET_CURRENT_VB_MAX_ELTS();

      /* Emit whole number of quads in total, and in each buffer.
       */
      dmasz -= dmasz & 1;
      count -= (count-start) & 1;
      currentsz -= currentsz & 1;

      if (currentsz < 12)
	 currentsz = dmasz;

      if (ctx->Light.ShadeModel == GL_FLAT) {
	 ELT_INIT( GL_TRIANGLES );

	 currentsz = currentsz/6*2;
	 dmasz = dmasz/6*2;

	 for (j = start; j + 3 < count; j += nr - 2 ) {
	    nr = MIN2( currentsz, count - j );

	    if (nr >= 4)
	    {
	       GLint i;
	       GLint quads = (nr/2)-1;
	       ELTS_VARS( ALLOC_ELTS( quads*6 ) );

	       for ( i = j-start ; i < j-start+quads ; i++, elts += 2 ) {
		  EMIT_TWO_ELTS( 0, elts[0], elts[1] );
		  EMIT_TWO_ELTS( 2, elts[2], elts[1] );
		  EMIT_TWO_ELTS( 4, elts[3], elts[2] );
		  INCR_ELTS( 6 );
	       }

	       FLUSH();
	    }

	    currentsz = dmasz;
	 }
      }
      else {
	 ELT_INIT( GL_TRIANGLE_STRIP );

	 for (j = start; j + 3 < count; j += nr - 2 ) {
	    nr = MIN2( currentsz, count - j );
	    TAG(emit_elts)( ctx, elts+j, nr, ALLOC_ELTS(nr) );
	    FLUSH();
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
   if (HAVE_QUADS) {
      LOCAL_VARS;
      GLuint *elts = TNL_CONTEXT(ctx)->vb.Elts;
      int dmasz = GET_SUBSEQUENT_VB_MAX_ELTS()/4*4;
      int currentsz;
      GLuint j, nr;

      FLUSH();
      ELT_INIT( GL_TRIANGLES );

      currentsz = GET_CURRENT_VB_MAX_ELTS()/4*4;

      count -= (count-start)%4;

      if (currentsz < 8)
	 currentsz = dmasz;

      for (j = start; j < count; j += nr) {
	 nr = MIN2( currentsz, count - j );
	 TAG(emit_elts)( ctx, elts+j, nr, ALLOC_ELTS(nr) );
	 FLUSH();
	 currentsz = dmasz;
      }
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
	    ELTS_VARS( ALLOC_ELTS( quads * 6 ) );

	    for ( i = j-start ; i < j-start+quads ; i++, elts += 4 ) {
	       EMIT_TWO_ELTS( 0, elts[0], elts[1] );
	       EMIT_TWO_ELTS( 2, elts[3], elts[1] );
	       EMIT_TWO_ELTS( 4, elts[2], elts[3] );
	       INCR_ELTS( 6 );
	    }

	    FLUSH();
	 }

	 currentsz = dmasz;
      }
   }
}



static tnl_render_func TAG(render_tab_elts)[GL_POLYGON+2] =
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



/* Pre-check the primitives in the VB to prevent the need for
 * fallbacks later on.
 */
static GLboolean TAG(validate_render)( GLcontext *ctx,
				       struct vertex_buffer *VB )
{
   GLint i;

   if (VB->ClipOrMask & ~CLIP_CULL_BIT)
      return GL_FALSE;

   if (VB->Elts && !HAVE_ELTS)
      return GL_FALSE;

   for (i = 0 ; i < VB->PrimitiveCount ; i++) {
      GLuint prim = VB->Primitive[i].mode;
      GLuint count = VB->Primitive[i].count;
      GLboolean ok = GL_FALSE;

      if (!count)
	 continue;

      switch (prim & PRIM_MODE_MASK) {
      case GL_POINTS:
	 ok = HAVE_POINTS;
	 break;
      case GL_LINES:
	 ok = HAVE_LINES && !ctx->Line.StippleFlag;
	 break;
      case GL_LINE_STRIP:
	 ok = HAVE_LINE_STRIPS && !ctx->Line.StippleFlag;
	 break;
      case GL_LINE_LOOP:
	 ok = HAVE_LINE_STRIPS && !ctx->Line.StippleFlag;
	 break;
      case GL_TRIANGLES:
	 ok = HAVE_TRIANGLES;
	 break;
      case GL_TRIANGLE_STRIP:
	 ok = HAVE_TRI_STRIPS;
	 break;
      case GL_TRIANGLE_FAN:
	 ok = HAVE_TRI_FANS;
	 break;
      case GL_POLYGON:
	 if (HAVE_POLYGONS) {
	    ok = GL_TRUE;
	 }
	 else {
	    ok = (HAVE_TRI_FANS && ctx->Light.ShadeModel == GL_SMOOTH);
         }
	 break;
      case GL_QUAD_STRIP:
	 if (VB->Elts) {
	    ok = HAVE_TRI_STRIPS;
	 }
	 else if (HAVE_QUAD_STRIPS) {
	    ok = GL_TRUE;
	 } else if (HAVE_TRI_STRIPS && 
		    ctx->Light.ShadeModel == GL_FLAT &&
		    VB->ColorPtr[0]->stride != 0) {
	    if (HAVE_ELTS) {
	       ok = (GLint) count < GET_SUBSEQUENT_VB_MAX_ELTS();
	    }
	    else {
	       ok = GL_FALSE;
	    }
	 }
	 else 
	    ok = HAVE_TRI_STRIPS;
	 break;
      case GL_QUADS:
	 if (HAVE_QUADS) {
	    ok = GL_TRUE;
	 } else if (HAVE_ELTS) {
	    ok = (GLint) count < GET_SUBSEQUENT_VB_MAX_ELTS();
	 }
	 else {
	    ok = HAVE_TRIANGLES; /* flatshading is ok. */
	 }
	 break;
      default:
	 break;
      }
      
      if (!ok) {
/* 	 fprintf(stderr, "not ok %s\n", _mesa_lookup_enum_by_nr(prim & PRIM_MODE_MASK)); */
	 return GL_FALSE;
      }
   }

   return GL_TRUE;
}

