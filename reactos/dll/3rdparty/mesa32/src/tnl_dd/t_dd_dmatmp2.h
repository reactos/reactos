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
 * \file t_dd_dmatmp2.h
 * Template for render stages which build and emit vertices directly
 * to fixed-size dma buffers.  Useful for rendering strips and other
 * native primitives where clipping and per-vertex tweaks such as
 * those in t_dd_tritmp.h are not required.
 *
 */

#if !HAVE_TRIANGLES || !HAVE_POINTS || !HAVE_LINES
#error "must have points, lines & triangles to use render template"
#endif

#if !HAVE_TRI_STRIPS || !HAVE_TRI_FANS
#error "must have tri strip and fans to use render template"
#endif

#if !HAVE_LINE_STRIPS
#error "must have line strips to use render template"
#endif

#if !HAVE_POLYGONS
#error "must have polygons to use render template"
#endif

#if !HAVE_ELTS
#error "must have elts to use render template"
#endif


#ifndef EMIT_TWO_ELTS
#define EMIT_TWO_ELTS( dest, offset, elt0, elt1 )	\
do { 						\
   (dest)[offset] = (elt0); 			\
   (dest)[offset+1] = (elt1); 			\
} while (0)
#endif


/**********************************************************************/
/*                  Render whole begin/end objects                    */
/**********************************************************************/


static ELT_TYPE *TAG(emit_elts)( GLcontext *ctx, 
			    ELT_TYPE *dest,
			    GLuint *elts, GLuint nr )
{
   GLint i;
   LOCAL_VARS;

   for ( i = 0 ; i+1 < nr ; i+=2, elts += 2 ) {
      EMIT_TWO_ELTS( dest, 0, elts[0], elts[1] );
      dest += 2;
   }
   if (i < nr) {
      EMIT_ELT( dest, 0, elts[0] );
      dest += 1;
   }
   
   return dest;
}

static ELT_TYPE *TAG(emit_consecutive_elts)( GLcontext *ctx, 
					ELT_TYPE *dest,
					GLuint start, GLuint nr )
{
   GLint i;
   LOCAL_VARS;

   for ( i = 0 ; i+1 < nr ; i+=2, start += 2 ) {
      EMIT_TWO_ELTS( dest, 0, start, start+1 );
      dest += 2;
   }
   if (i < nr) {
      EMIT_ELT( dest, 0, start );
      dest += 1;
   }

   return dest;
}

/***********************************************************************
 *                    Render non-indexed primitives.
 ***********************************************************************/



static void TAG(render_points_verts)( GLcontext *ctx,
				      GLuint start,
				      GLuint count,
				      GLuint flags )
{
   if (start < count) {
      LOCAL_VARS;
      if (0) fprintf(stderr, "%s\n", __FUNCTION__);
      EMIT_PRIM( ctx, GL_POINTS, HW_POINTS, start, count );
   }
}

static void TAG(render_lines_verts)( GLcontext *ctx,
				     GLuint start,
				     GLuint count,
				     GLuint flags )
{
   LOCAL_VARS;
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);
   count -= (count-start) & 1;

   if (start+1 >= count)
      return;

   if ((flags & PRIM_BEGIN) && ctx->Line.StippleFlag) {
      RESET_STIPPLE();
      AUTO_STIPPLE( GL_TRUE );
   }
      
   EMIT_PRIM( ctx, GL_LINES, HW_LINES, start, count );

   if ((flags & PRIM_END) && ctx->Line.StippleFlag)
      AUTO_STIPPLE( GL_FALSE );
}


static void TAG(render_line_strip_verts)( GLcontext *ctx,
					  GLuint start,
					  GLuint count,
					  GLuint flags )
{
   LOCAL_VARS;
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);

   if (start+1 >= count)
      return;

   if ((flags & PRIM_BEGIN) && ctx->Line.StippleFlag)
      RESET_STIPPLE();


   if (PREFER_DISCRETE_ELT_PRIM( count-start, HW_LINES ))
   {   
      int dmasz = GET_MAX_HW_ELTS();
      GLuint j, nr;

      ELT_INIT( GL_LINES, HW_LINES );

      /* Emit whole number of lines in each full buffer.
       */
      dmasz = dmasz/2;


      for (j = start; j + 1 < count; j += nr - 1 ) {
	 ELT_TYPE *dest;
	 GLint i;

	 nr = MIN2( dmasz, count - j );
	 dest = ALLOC_ELTS( (nr-1)*2 );
	    
	 for ( i = j ; i+1 < j+nr ; i+=1 ) {
	    EMIT_TWO_ELTS( dest, 0, (i+0), (i+1) );
	    dest += 2;
	 }

	 CLOSE_ELTS();
      }
   }
   else
      EMIT_PRIM( ctx, GL_LINE_STRIP, HW_LINE_STRIP, start, count );
}


static void TAG(render_line_loop_verts)( GLcontext *ctx,
					 GLuint start,
					 GLuint count,
					 GLuint flags )
{
   LOCAL_VARS;
   GLuint j, nr;
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);

   if (flags & PRIM_BEGIN) {
      j = start;
      if (ctx->Line.StippleFlag)
	 RESET_STIPPLE( );
   }
   else
      j = start + 1;

   if (flags & PRIM_END) {

      if (start+1 >= count)
	 return;

      if (PREFER_DISCRETE_ELT_PRIM( count-start, HW_LINES )) {
	 int dmasz = GET_MAX_HW_ELTS();

	 ELT_INIT( GL_LINES, HW_LINES );

	 /* Emit whole number of lines in each full buffer.
	  */
	 dmasz = dmasz/2;

	 /* Ensure last vertex doesn't wrap:
	  */
	 dmasz--;

	 for (; j + 1 < count;  ) {
	    GLint i;
	    ELT_TYPE *dest;

	    nr = MIN2( dmasz, count - j );
	    dest = ALLOC_ELTS( nr*2 );	/* allocs room for 1 more line */

	    for ( i = 0 ; i < nr - 1 ; i+=1 ) {
	       EMIT_TWO_ELTS( dest, 0, (j+i), (j+i+1) );
	       dest += 2;
	    }

	    j += nr - 1;

	    /* Emit 1 more line into space alloced above */
	    if (j + 1 >= count) {
 	       EMIT_TWO_ELTS( dest, 0, (j), (start) ); 
 	       dest += 2; 
 	    }
 
	    CLOSE_ELTS();
	 }
      }
      else
      {
	 int dmasz = GET_MAX_HW_ELTS() - 1;

	 ELT_INIT( GL_LINE_STRIP, HW_LINE_STRIP );

	 for ( ; j + 1 < count;  ) {
	    nr = MIN2( dmasz, count - j );
	    if (j + nr < count) {
	       ELT_TYPE *dest = ALLOC_ELTS( nr );
	       dest = TAG(emit_consecutive_elts)( ctx, dest, j, nr );
	       j += nr - 1;
	       CLOSE_ELTS();
	    }
	    else if (nr) {
	       ELT_TYPE *dest = ALLOC_ELTS( nr + 1 );
	       dest = TAG(emit_consecutive_elts)( ctx, dest, j, nr );
	       dest = TAG(emit_consecutive_elts)( ctx, dest, start, 1 );
	       j += nr;
	       CLOSE_ELTS();
	    }
	 }   
      }
   } else {
      TAG(render_line_strip_verts)( ctx, j, count, flags );
   }
}


static void TAG(render_triangles_verts)( GLcontext *ctx,
					 GLuint start,
					 GLuint count,
					 GLuint flags )
{
   LOCAL_VARS;
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);

   count -= (count-start)%3;

   if (start+2 >= count) {
      return;
   }

   /* need a PREFER_DISCRETE_ELT_PRIM here too..
    */
   EMIT_PRIM( ctx, GL_TRIANGLES, HW_TRIANGLES, start, count );
}



static void TAG(render_tri_strip_verts)( GLcontext *ctx,
					 GLuint start,
					 GLuint count,
					 GLuint flags )
{
   LOCAL_VARS;
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);

   if (start + 2 >= count)
      return;

   if (PREFER_DISCRETE_ELT_PRIM( count-start, HW_TRIANGLES ))
   {   
      int dmasz = GET_MAX_HW_ELTS();
      int parity = 0;
      GLuint j, nr;

      ELT_INIT( GL_TRIANGLES, HW_TRIANGLES );

      /* Emit even number of tris in each full buffer.
       */
      dmasz = dmasz/3;
      dmasz -= dmasz & 1;

      for (j = start; j + 2 < count; j += nr - 2 ) {
	 ELT_TYPE *dest;
	 GLint i;

	 nr = MIN2( dmasz, count - j );
	 dest = ALLOC_ELTS( (nr-2)*3 );
	    
	 for ( i = j ; i+2 < j+nr ; i++, parity^=1 ) {
	    EMIT_ELT( dest, 0, (i+0+parity) );
	    EMIT_ELT( dest, 1, (i+1-parity) );
	    EMIT_ELT( dest, 2, (i+2) );
	    dest += 3;
	 }

	 CLOSE_ELTS();
      }
   }
   else
      EMIT_PRIM( ctx, GL_TRIANGLE_STRIP, HW_TRIANGLE_STRIP_0, start, count );
}

static void TAG(render_tri_fan_verts)( GLcontext *ctx,
				       GLuint start,
				       GLuint count,
				       GLuint flags )
{
   LOCAL_VARS;
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);

   if (start+2 >= count) 
      return;

   if (PREFER_DISCRETE_ELT_PRIM( count-start, HW_TRIANGLES ))
   {   
      int dmasz = GET_MAX_HW_ELTS();
      GLuint j, nr;

      ELT_INIT( GL_TRIANGLES, HW_TRIANGLES );

      dmasz = dmasz/3;

      for (j = start + 1; j + 1 < count; j += nr - 1 ) {
	 ELT_TYPE *dest;
	 GLint i;

	 nr = MIN2( dmasz, count - j );
	 dest = ALLOC_ELTS( (nr-1)*3 );
	    
	 for ( i = j ; i+1 < j+nr ; i++ ) {
	    EMIT_ELT( dest, 0, (start) );
	    EMIT_ELT( dest, 1, (i) );
	    EMIT_ELT( dest, 2, (i+1) );
	    dest += 3;
	 }
	 
	 CLOSE_ELTS();
      }
   }
   else {
      EMIT_PRIM( ctx, GL_TRIANGLE_FAN, HW_TRIANGLE_FAN, start, count );
   }
}


static void TAG(render_poly_verts)( GLcontext *ctx,
				    GLuint start,
				    GLuint count,
				    GLuint flags )
{
   LOCAL_VARS;
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);

   if (start+2 >= count) 
      return;

   EMIT_PRIM( ctx, GL_POLYGON, HW_POLYGON, start, count );
}

static void TAG(render_quad_strip_verts)( GLcontext *ctx,
					  GLuint start,
					  GLuint count,
					  GLuint flags )
{
   LOCAL_VARS;
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);

   count -= (count-start) & 1;

   if (start+3 >= count) 
      return;

   if (HAVE_QUAD_STRIPS) {
      EMIT_PRIM( ctx, GL_QUAD_STRIP, HW_QUAD_STRIP, start, count );
   } 
   else if (ctx->Light.ShadeModel == GL_FLAT) {
      LOCAL_VARS;
      int dmasz = GET_MAX_HW_ELTS();
      GLuint j, nr;

      ELT_INIT( GL_TRIANGLES, HW_TRIANGLES );

      /* Emit whole number of quads in total, and in each buffer.
       */
      dmasz = (dmasz/6)*2;

      for (j = start; j + 3 < count; j += nr - 2 ) {
	 ELT_TYPE *dest;
	 GLint quads, i;

	 nr = MIN2( dmasz, count - j );
	 quads = (nr/2)-1;
	 dest = ALLOC_ELTS( quads*6 );
	    
	 for ( i = j ; i < j+quads*2 ; i+=2 ) {
	    EMIT_TWO_ELTS( dest, 0, (i+0), (i+1) );
	    EMIT_TWO_ELTS( dest, 2, (i+2), (i+1) );
	    EMIT_TWO_ELTS( dest, 4, (i+3), (i+2) );
	    dest += 6;
	 }

	 CLOSE_ELTS();
      }
   }
   else {
      EMIT_PRIM( ctx, GL_TRIANGLE_STRIP, HW_TRIANGLE_STRIP_0, start, count );
   }
}


static void TAG(render_quads_verts)( GLcontext *ctx,
				     GLuint start,
				     GLuint count,
				     GLuint flags )
{
   LOCAL_VARS;
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);
   count -= (count-start)%4;

   if (start+3 >= count) 
      return;

   if (HAVE_QUADS) {
      EMIT_PRIM( ctx, GL_QUADS, HW_QUADS, start, count );
   } 
   else {
      /* Hardware doesn't have a quad primitive type -- simulate it
       * using indexed vertices and the triangle primitive: 
       */
      LOCAL_VARS;
      int dmasz = GET_MAX_HW_ELTS();
      GLuint j, nr;

      ELT_INIT( GL_TRIANGLES, HW_TRIANGLES );

      /* Adjust for rendering as triangles:
       */
      dmasz = (dmasz/6)*4;

      for (j = start; j < count; j += nr ) {
	 ELT_TYPE *dest;
	 GLint quads, i;

	 nr = MIN2( dmasz, count - j );
	 quads = nr/4;
	 dest = ALLOC_ELTS( quads*6 );

	 for ( i = j ; i < j+quads*4 ; i+=4 ) {
	    EMIT_TWO_ELTS( dest, 0, (i+0), (i+1) );
	    EMIT_TWO_ELTS( dest, 2, (i+3), (i+1) );
	    EMIT_TWO_ELTS( dest, 4, (i+2), (i+3) );
	    dest += 6;
	 }

	 CLOSE_ELTS();
      }
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

static void TAG(render_points_elts)( GLcontext *ctx,
				     GLuint start,
				     GLuint count,
				     GLuint flags )
{
   LOCAL_VARS;
   int dmasz = GET_MAX_HW_ELTS();
   GLuint *elts = GET_MESA_ELTS();
   GLuint j, nr;
   ELT_TYPE *dest;

   ELT_INIT( GL_POINTS, HW_POINTS );

   for (j = start; j < count; j += nr ) {
      nr = MIN2( dmasz, count - j );
      dest = ALLOC_ELTS( nr );
      dest = TAG(emit_elts)( ctx, dest, elts+j, nr );
      CLOSE_ELTS();
   }
}



static void TAG(render_lines_elts)( GLcontext *ctx,
				    GLuint start,
				    GLuint count,
				    GLuint flags )
{
   LOCAL_VARS;
   int dmasz = GET_MAX_HW_ELTS();
   GLuint *elts = GET_MESA_ELTS();
   GLuint j, nr;
   ELT_TYPE *dest;

   if (start+1 >= count)
      return;

   if ((flags & PRIM_BEGIN) && ctx->Line.StippleFlag) {
      RESET_STIPPLE();
      AUTO_STIPPLE( GL_TRUE );
   }

   ELT_INIT( GL_LINES, HW_LINES );

   /* Emit whole number of lines in total and in each buffer:
    */
   count -= (count-start) & 1;
   dmasz -= dmasz & 1;

   for (j = start; j < count; j += nr ) {
      nr = MIN2( dmasz, count - j );
      dest = ALLOC_ELTS( nr );
      dest = TAG(emit_elts)( ctx, dest, elts+j, nr );
      CLOSE_ELTS();
   }

   if ((flags & PRIM_END) && ctx->Line.StippleFlag)
      AUTO_STIPPLE( GL_FALSE );
}


static void TAG(render_line_strip_elts)( GLcontext *ctx,
					 GLuint start,
					 GLuint count,
					 GLuint flags )
{
   LOCAL_VARS;
   int dmasz = GET_MAX_HW_ELTS();
   GLuint *elts = GET_MESA_ELTS();
   GLuint j, nr;
   ELT_TYPE *dest;

   if (start+1 >= count)
      return;

   ELT_INIT( GL_LINE_STRIP, HW_LINE_STRIP );

   if ((flags & PRIM_BEGIN) && ctx->Line.StippleFlag)
      RESET_STIPPLE();

   for (j = start; j + 1 < count; j += nr - 1 ) {
      nr = MIN2( dmasz, count - j );
      dest = ALLOC_ELTS( nr );
      dest = TAG(emit_elts)( ctx, dest, elts+j, nr );
      CLOSE_ELTS();
   }
}


static void TAG(render_line_loop_elts)( GLcontext *ctx,
					GLuint start,
					GLuint count,
					GLuint flags )
{
   LOCAL_VARS;
   int dmasz = GET_MAX_HW_ELTS();
   GLuint *elts = GET_MESA_ELTS();
   GLuint j, nr;
   ELT_TYPE *dest;

   if (0) fprintf(stderr, "%s\n", __FUNCTION__);

   if (flags & PRIM_BEGIN)
      j = start;
   else
      j = start + 1;

   
   if (flags & PRIM_END) {
      if (start+1 >= count)
	 return;
   } 
   else {
      if (j+1 >= count)
	 return;
   }

   ELT_INIT( GL_LINE_STRIP, HW_LINE_STRIP );

   if ((flags & PRIM_BEGIN) && ctx->Line.StippleFlag)
      RESET_STIPPLE();

   
   /* Ensure last vertex doesn't wrap:
    */
   dmasz--;

   for ( ; j + 1 < count; ) {
      nr = MIN2( dmasz, count - j );
      dest = ALLOC_ELTS( nr+1 );	/* Reserve possible space for last elt */
      dest = TAG(emit_elts)( ctx, dest, elts+j, nr );
      j += nr - 1;
      if (j + 1 >= count && (flags & PRIM_END)) {
	 dest = TAG(emit_elts)( ctx, dest, elts+start, 1 );
      }
      CLOSE_ELTS();
   }
}


static void TAG(render_triangles_elts)( GLcontext *ctx,
					GLuint start,
					GLuint count,
					GLuint flags )
{
   LOCAL_VARS;
   GLuint *elts = GET_MESA_ELTS();
   int dmasz = GET_MAX_HW_ELTS()/3*3;
   GLuint j, nr;
   ELT_TYPE *dest;

   if (start+2 >= count)
      return;

   ELT_INIT( GL_TRIANGLES, HW_TRIANGLES );


   /* Emit whole number of tris in total.  dmasz is already a multiple
    * of 3.
    */
   count -= (count-start)%3;

   for (j = start; j < count; j += nr) {
      nr = MIN2( dmasz, count - j );
      dest = ALLOC_ELTS( nr );
      dest = TAG(emit_elts)( ctx, dest, elts+j, nr );
      CLOSE_ELTS();
   }
}



static void TAG(render_tri_strip_elts)( GLcontext *ctx,
					GLuint start,
					GLuint count,
					GLuint flags )
{
   LOCAL_VARS;
   GLuint j, nr;
   GLuint *elts = GET_MESA_ELTS();
   int dmasz = GET_MAX_HW_ELTS();
   ELT_TYPE *dest;

   if (start+2 >= count)
      return;

   ELT_INIT( GL_TRIANGLE_STRIP, HW_TRIANGLE_STRIP_0 );

   /* Keep the same winding over multiple buffers:
    */
   dmasz -= (dmasz & 1);

   for (j = start ; j + 2 < count; j += nr - 2 ) {
      nr = MIN2( dmasz, count - j );

      dest = ALLOC_ELTS( nr );
      dest = TAG(emit_elts)( ctx, dest, elts+j, nr );
      CLOSE_ELTS();
   }
}

static void TAG(render_tri_fan_elts)( GLcontext *ctx,
				      GLuint start,
				      GLuint count,
				      GLuint flags )
{
   LOCAL_VARS;
   GLuint *elts = GET_MESA_ELTS();
   GLuint j, nr;
   int dmasz = GET_MAX_HW_ELTS();
   ELT_TYPE *dest;

   if (start+2 >= count)
      return;

   ELT_INIT( GL_TRIANGLE_FAN, HW_TRIANGLE_FAN );

   for (j = start + 1 ; j + 1 < count; j += nr - 1 ) {
      nr = MIN2( dmasz, count - j + 1 );
      dest = ALLOC_ELTS( nr );
      dest = TAG(emit_elts)( ctx, dest, elts+start, 1 );
      dest = TAG(emit_elts)( ctx, dest, elts+j, nr - 1 );
      CLOSE_ELTS();
   }
}


static void TAG(render_poly_elts)( GLcontext *ctx,
				   GLuint start,
				   GLuint count,
				   GLuint flags )
{
   LOCAL_VARS;
   GLuint *elts = GET_MESA_ELTS();
   GLuint j, nr;
   int dmasz = GET_MAX_HW_ELTS();
   ELT_TYPE *dest;

   if (start+2 >= count)
      return;

   ELT_INIT( GL_POLYGON, HW_POLYGON );

   for (j = start + 1 ; j + 1 < count ; j += nr - 1 ) {
      nr = MIN2( dmasz, count - j + 1 );
      dest = ALLOC_ELTS( nr );
      dest = TAG(emit_elts)( ctx, dest, elts+start, 1 );
      dest = TAG(emit_elts)( ctx, dest, elts+j, nr - 1 );
      CLOSE_ELTS();
   }
}

static void TAG(render_quad_strip_elts)( GLcontext *ctx,
					 GLuint start,
					 GLuint count,
					 GLuint flags )
{
   if (start+3 >= count)
      return;

   if (HAVE_QUAD_STRIPS && 0) {
   }
   else {
      LOCAL_VARS;
      GLuint *elts = GET_MESA_ELTS();
      int dmasz = GET_MAX_HW_ELTS();
      GLuint j, nr;
      ELT_TYPE *dest;

      /* Emit whole number of quads in total, and in each buffer.
       */
      dmasz -= dmasz & 1;
      count -= (count-start) & 1;

      if (ctx->Light.ShadeModel == GL_FLAT) {
	 ELT_INIT( GL_TRIANGLES, HW_TRIANGLES );

	 dmasz = dmasz/6*2;

	 for (j = start; j + 3 < count; j += nr - 2 ) {
	    nr = MIN2( dmasz, count - j );

	    if (nr >= 4)
	    {
	       GLint quads = (nr/2)-1;
	       ELT_TYPE *dest = ALLOC_ELTS( quads*6 );
	       GLint i;

	       for ( i = j-start ; i < j-start+quads ; i++, elts += 2 ) {
		  EMIT_TWO_ELTS( dest, 0, elts[0], elts[1] );
		  EMIT_TWO_ELTS( dest, 2, elts[2], elts[1] );
		  EMIT_TWO_ELTS( dest, 4, elts[3], elts[2] );
		  dest += 6;
	       }

	       CLOSE_ELTS();
	    }
	 }
      }
      else {
	 ELT_INIT( GL_TRIANGLE_STRIP, HW_TRIANGLE_STRIP_0 );

	 for (j = start; j + 3 < count; j += nr - 2 ) {
	    nr = MIN2( dmasz, count - j );
	    dest = ALLOC_ELTS( nr );
	    dest = TAG(emit_elts)( ctx, dest, elts+j, nr );
	    CLOSE_ELTS();
	 }
      }
   }
}


static void TAG(render_quads_elts)( GLcontext *ctx,
				    GLuint start,
				    GLuint count,
				    GLuint flags )
{
   if (start+3 >= count)
      return;

   if (HAVE_QUADS && 0) {
   } else {
      LOCAL_VARS;
      GLuint *elts = GET_MESA_ELTS();
      int dmasz = GET_MAX_HW_ELTS();
      GLuint j, nr;

      ELT_INIT( GL_TRIANGLES, HW_TRIANGLES );

      /* Emit whole number of quads in total, and in each buffer.
       */
      dmasz -= dmasz & 3;
      count -= (count-start) & 3;

      /* Adjust for rendering as triangles:
       */
      dmasz = dmasz/6*4;

      for (j = start; j + 3 < count; j += nr ) {
	 nr = MIN2( dmasz, count - j );

	 {
	    GLint quads = nr/4;
	    ELT_TYPE *dest = ALLOC_ELTS( quads * 6 );
	    GLint i;

	    for ( i = j-start ; i < j-start+quads ; i++, elts += 4 ) {
	       EMIT_TWO_ELTS( dest, 0, elts[0], elts[1] );
	       EMIT_TWO_ELTS( dest, 2, elts[3], elts[1] );
	       EMIT_TWO_ELTS( dest, 4, elts[2], elts[3] );
	       dest += 6;
	    }

	    CLOSE_ELTS();
	 }
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
