
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
 *    Keith Whitwell <keithw@valinux.com>
 *    Gareth Hughes <gareth@valinux.com>
 */

/* Template for immediate mode vertices. 
 *
 * Probably instantiate once for each vertex format used:
 *   - TINY_VERTICES
 *   - TEX0_VERTICES
 *   - TEX1_VERTICES
 *   - PTEX_VERTICES
 *
 * Have to handle TEX->PTEX transition somehow.
 */

#define DBG 0



/* =============================================================
 * GL_POINTS
 */

static void TAG(flush_point_0)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   if ( !v0->mask ) {
      LOCAL_VARS;
      DRAW_POINT( v0 );
   }
}


/* =============================================================
 * GL_LINES
 */

static void TAG(flush_line_1)( GLcontext *ctx, TNL_VERTEX *v0 );

static void TAG(flush_line_0)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;
   FLUSH_VERTEX = TAG(flush_line_1);
   ACTIVE_VERTEX = IMM_VERTICES( 1 );
}

static void TAG(flush_line_1)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;
   TNL_VERTEX *v1 = v0 - 1;
   ACTIVE_VERTEX = IMM_VERTICES( 0 );
   FLUSH_VERTEX = TAG(flush_line_0);
   if (FALLBACK_OR_CLIPPING) 
      CLIP_OR_DRAW_LINE( ctx, v1, v0 ); 
   else
      DRAW_LINE( ctx, v1, v0 ); 
}


/* =============================================================
 * GL_LINE_LOOP
 */

static void TAG(flush_line_loop_2)( GLcontext *ctx, TNL_VERTEX *v0 );
static void TAG(flush_line_loop_1)( GLcontext *ctx, TNL_VERTEX *v0 );

static void TAG(flush_line_loop_0)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;

   ACTIVE_VERTEX = v0 + 1;
   FLUSH_VERTEX = TAG(flush_line_loop_1);
}

#define DRAW_LINELOOP_LINE( a, b )			\
   if (!HAVE_LINE_STRIP || FALLBACK_OR_CLIPPING) {	\
      CLIP_OR_DRAW_LINE( ctx, a, b );			\
   } else if (EXTEND_PRIM( 1 )) {			\
      EMIT_VERTEX( b );					\
   } else {						\
      BEGIN_PRIM( GL_LINE_STRIP, 2 ); 			\
      EMIT_VERTEX( a );					\
      EMIT_VERTEX( b );      				\
   }

static void TAG(flush_line_loop_1)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;
   TNL_VERTEX *v1 = v0 - 1;
   ACTIVE_VERTEX = v1;
   FLUSH_VERTEX = TAG(flush_line_loop_2);
   DRAW_LINELOOP_LINE( v1, v0 );
}

static void TAG(flush_line_loop_2)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;
   TNL_VERTEX *v1 = v0 + 1;
   ACTIVE_VERTEX = v1;
   FLUSH_VERTEX = TAG(flush_line_loop_1);
   DRAW_LINELOOP_LINE( v1, v0 );
}

static void TAG(end_line_loop)( GLcontext *ctx )
{
   LOCAL_VARS;

   if ( FLUSH_VERTEX != TAG(flush_line_loop_0) ) {
      TNL_VERTEX *v1 = ACTIVE_VERTEX;
      TNL_VERTEX *v0 = IMM_VERTICES( 0 );
      DRAW_LINELOOP_LINE( v1, v0 );
   }
}



/* =============================================================
 * GL_LINE_STRIP
 */

static void TAG(flush_line_strip_2)( GLcontext *ctx, TNL_VERTEX *v0 );
static void TAG(flush_line_strip_1)( GLcontext *ctx, TNL_VERTEX *v0 );

static void TAG(flush_line_strip_0)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;

   ACTIVE_VERTEX = v0 + 1;
   FLUSH_VERTEX = TAG(flush_line_strip_0b);
}


static void TAG(flush_line_strip_1)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;
   TNL_VERTEX *v1 = v0 - 1;

   ACTIVE_VERTEX = v1;
   FLUSH_VERTEX = TAG(flush_line_strip_2);

   if (!HAVE_LINE_STRIP || FALLBACK_OR_CLIPPING)
      CLIP_OR_DRAW_LINE( ctx, v1, v0 );
   else if (EXTEND_PRIM( 1 )) {
      EMIT_VERTEX( v0 );
   } else {
      BEGIN_PRIM( GL_LINE_STRIP, 2 ); 
      EMIT_VERTEX( v1 );
      EMIT_VERTEX( v0 );      
   }      
}

static void TAG(flush_line_strip_2)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;
   TNL_VERTEX *v1 = v0 + 1;

   ACTIVE_VERTEX = v1;
   FLUSH_VERTEX = TAG(flush_line_strip_1);

   if (!HAVE_LINE_STRIP || FALLBACK_OR_CLIPPING)
      CLIP_OR_DRAW_LINE( ctx, v1, v0 );
   else if (EXTEND_PRIM( 1 )) {
      EMIT_VERTEX( v0 );
   } else {
      BEGIN_PRIM( GL_LINE_STRIP, 2 ); 
      EMIT_VERTEX( v1 );
      EMIT_VERTEX( v0 );      
   }      
}



/* =============================================================
 * GL_TRIANGLES
 */

static void TAG(flush_triangle_2)( GLcontext *ctx, TNL_VERTEX *v0 );
static void TAG(flush_triangle_1)( GLcontext *ctx, TNL_VERTEX *v0 );

static void TAG(flush_triangle_0)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;

   if ( DBG ) fprintf( stderr, __FUNCTION__ "\n" );

   ACTIVE_VERTEX = v0 + 1;
   FLUSH_VERTEX = TAG(flush_triangle_1);
   BEGIN_PRIM( GL_TRIANGLES, 0 );
}

static void TAG(flush_triangle_1)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;

   if ( DBG ) fprintf( stderr, __FUNCTION__ "\n" );

   ACTIVE_VERTEX = v0 + 1;
   FLUSH_VERTEX = TAG(flush_triangle_2);
}

static void TAG(flush_triangle_2)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;
   TNL_VERTEX *v2 = v0 - 2;
   TNL_VERTEX *v1 = v0 - 1;

   if ( DBG ) fprintf( stderr, __FUNCTION__ "\n" );

   ACTIVE_VERTEX = v2;
   FLUSH_VERTEX = TAG(flush_triangle_0);

   /* nothing gained by trying to emit as hw primitives -- that
    * happens normally in this case.  
    */
   if (FALLBACK_OR_CLIPPING)
      CLIP_OR_DRAW_TRI( ctx, v2, v1, v0 );
   else
      DRAW_TRI( ctx, v2, v1, v0 );
}




/* =============================================================
 * GL_TRIANGLE_STRIP
 */

static void TAG(flush_tri_strip_3)( GLcontext *ctx, TNL_VERTEX *v0 );
static void TAG(flush_tri_strip_2)( GLcontext *ctx, TNL_VERTEX *v0 );
static void TAG(flush_tri_strip_1)( GLcontext *ctx, TNL_VERTEX *v0 );

static void TAG(flush_tri_strip_0)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;
   ACTIVE_VERTEX = IMM_VERTICES( 1 );
   FLUSH_VERTEX = TAG(flush_tri_strip_1);
}

static void TAG(flush_tri_strip_1)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;
   ACTIVE_VERTEX = IMM_VERTICES( 2 );
   FLUSH_VERTEX = TAG(flush_tri_strip_2);
}

#define DO_TRISTRIP_TRI( vert0, vert1 )				\
   if (!HAVE_TRI_STRIP || FALLBACK_OR_CLIPPING) {	\
      TNL_VERTEX *v2 = IMM_VERTICES( vert0 );			\
      TNL_VERTEX *v1 = IMM_VERTICES( vert1 );			\
      TAG(draw_tri)( ctx, v2, v1, v0 );				\
   } else if (EXTEND_PRIM( 1 )) {				\
      EMIT_VERTEX( v0 );					\
   } else {							\
      TNL_VERTEX *v2 = IMM_VERTICES( vert0 );			\
      TNL_VERTEX *v1 = IMM_VERTICES( vert1 );			\
      BEGIN_PRIM( GL_TRIANGLE_STRIP, 3 ); 			\
      EMIT_VERTEX( v2 );					\
      EMIT_VERTEX( v1 );      					\
      EMIT_VERTEX( v0 );      					\
   }      

static void TAG(flush_tri_strip_2)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;
   FLUSH_VERTEX = TAG(flush_tri_strip_3);
   ACTIVE_VERTEX = IMM_VERTICES( 3 );
   DO_TRISTRIP_TRI( 0, 1 );
}

static void TAG(flush_tri_strip_3)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;
   FLUSH_VERTEX = TAG(flush_tri_strip_4);
   ACTIVE_VERTEX = IMM_VERTICES( 0 );
   DO_TRISTRIP_TRI( 1, 2 );
}

static void TAG(flush_tri_strip_4)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;
   FLUSH_VERTEX = TAG(flush_tri_strip_5);
   ACTIVE_VERTEX = IMM_VERTICES( 1 );
   DO_TRISTRIP_TRI( 2, 3 );
}

static void TAG(flush_tri_strip_5)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;
   FLUSH_VERTEX = TAG(flush_tri_strip_2);
   ACTIVE_VERTEX = IMM_VERTICES( 2 );
   DO_TRISTRIP_TRI( 0, 3 );
}



/* =============================================================
 * GL_TRIANGLE_FAN
 */

static void TAG(flush_tri_fan_2)( GLcontext *ctx, TNL_VERTEX *v0 );
static void TAG(flush_tri_fan_1)( GLcontext *ctx, TNL_VERTEX *v0 );

static void TAG(flush_tri_fan_0)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;

   ACTIVE_VERTEX = v0 + 1;
   FLUSH_VERTEX = TAG(flush_tri_fan_1);
}

static void TAG(flush_tri_fan_1)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;

   ACTIVE_VERTEX = v0 + 1;
   FLUSH_VERTEX = TAG(flush_tri_fan_2);
}

#define DO_TRIFAN_TRI( vert0, vert1 )				\
   if (!HAVE_TRI_FAN || FALLBACK_OR_CLIPPING) {	\
      TNL_VERTEX *v2 = IMM_VERTICES( vert0 );			\
      TNL_VERTEX *v1 = IMM_VERTICES( vert1 );			\
      TAG(draw_tri)( ctx, v2, v1, v0 );				\
   } else if (EXTEND_PRIM( 1 )) {				\
      EMIT_VERTEX( v0 );					\
   } else {							\
      TNL_VERTEX *v2 = IMM_VERTICES( vert0 );			\
      TNL_VERTEX *v1 = IMM_VERTICES( vert1 );			\
      BEGIN_PRIM( GL_TRIANGLE_FAN, 3 ); 			\
      EMIT_VERTEX( v2 );					\
      EMIT_VERTEX( v1 );      					\
      EMIT_VERTEX( v0 );      					\
   }      

static void TAG(flush_tri_fan_2)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;
   ACTIVE_VERTEX = IMM_VERTICES( 1 );
   FLUSH_VERTEX = TAG(flush_tri_fan_3 );
   DO_TRIFAN_TRI( 0, 1 );
}

static void TAG(flush_tri_fan_3)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;
   ACTIVE_VERTEX = IMM_VERTICES( 2 );
   FLUSH_VERTEX = TAG(flush_tri_fan_2 );
   DO_TRIFAN_TRI( 0, 2 );
}



/* =============================================================
 * GL_QUADS
 */

static void TAG(flush_quad_3)( GLcontext *ctx, TNL_VERTEX *v0 );
static void TAG(flush_quad_2)( GLcontext *ctx, TNL_VERTEX *v0 );
static void TAG(flush_quad_1)( GLcontext *ctx, TNL_VERTEX *v0 );

static void TAG(flush_quad_0)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;
   IMM_VERTEX( v0 ) = v0 + 1;
   FLUSH_VERTEX = TAG(flush_quad_1);
}

static void TAG(flush_quad_1)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;
   IMM_VERTEX( v0 ) = v0 + 1;
   FLUSH_VERTEX = TAG(flush_quad_2);
}

static void TAG(flush_quad_2)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;
   IMM_VERTEX( v0 ) = v0 + 1;
   FLUSH_VERTEX = TAG(flush_quad_3);
}

static void TAG(flush_quad_3)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;
   TNL_VERTEX *v3 = v0 - 3;
   TNL_VERTEX *v2 = v0 - 2;
   TNL_VERTEX *v1 = v0 - 1;

   IMM_VERTEX( v0 ) = v3;
   FLUSH_VERTEX = TAG(flush_quad_0);

   if (!HAVE_HW_QUADS || FALLBACK_OR_CLIPPING) {
      CLIP_OR_DRAW_TRI( ctx, v3, v2, v0 );
      CLIP_OR_DRAW_TRI( ctx, v2, v1, v0 );
   } else {
      EXTEND_PRIM_NF( GL_QUADS, 4 );
      EMIT_VERTEX( v3 );
      EMIT_VERTEX( v2 );
      EMIT_VERTEX( v1 );      
      EMIT_VERTEX( v0 );      
   }
}



/* =============================================================
 * GL_QUAD_STRIP
 */

static void TAG(flush_quad_strip_3)( GLcontext *ctx, TNL_VERTEX *v0 );
static void TAG(flush_quad_strip_2)( GLcontext *ctx, TNL_VERTEX *v0 );
static void TAG(flush_quad_strip_1)( GLcontext *ctx, TNL_VERTEX *v0 );

static void TAG(flush_quad_strip_0)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;

   IMM_VERTEX( v3 ) = v0;
   IMM_VERTEX( v0 ) = v0 + 1;
   FLUSH_VERTEX = TAG(flush_quad_strip_1);
}

static void TAG(flush_quad_strip_1)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;

   IMM_VERTEX( v2 ) = v0;
   IMM_VERTEX( v0 ) = v0 + 1;
   FLUSH_VERTEX = TAG(flush_quad_strip_2);
}

static void TAG(flush_quad_strip_2)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;

   IMM_VERTEX( v1 ) = v0;
   IMM_VERTEX( v0 ) = v0 + 1;
   FLUSH_VERTEX = TAG(flush_quad_strip_3);
}

static void TAG(flush_quad_strip_3)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;
   TNL_VERTEX *v3 = IMM_VERTEX( v3 );
   TNL_VERTEX *v2 = IMM_VERTEX( v2 );
   TNL_VERTEX *v1 = IMM_VERTEX( v1 );

   IMM_VERTEX( v0 ) = v3;
   IMM_VERTEX( v2 ) = v0;
   IMM_VERTEX( v3 ) = v1;
   FLUSH_VERTEX = TAG(flush_quad_strip_2);

   if (FALLBACK_OR_CLIPPING) {
      CLIP_OR_DRAW_TRI( ctx, v3, v2, v0 );
      CLIP_OR_DRAW_TRI( ctx, v2, v1, v0 );
   } else {
      DRAW_TRI( ctx, v3, v2, v0 );
      DRAW_TRI( ctx, v2, v1, v0 );
   }
}



/* =============================================================
 * GL_POLYGON
 */

static void TAG(flush_poly_2)( GLcontext *ctx, TNL_VERTEX *v0 );
static void TAG(flush_poly_1)( GLcontext *ctx, TNL_VERTEX *v0 );

static void TAG(flush_poly_0)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;
   ACTIVE_VERTEX = IMM_VERTICES( 1 );
   FLUSH_VERTEX = TAG(flush_poly_1);
}

static void TAG(flush_poly_1)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;
   ACTIVE_VERTEX = IMM_VERTICES( 2 );
   FLUSH_VERTEX = TAG(flush_poly_2);
}

#define DO_POLY_TRI( vert0, vert1 )				\
   if (!HAVE_POLYGONS || FALLBACK_OR_CLIPPING) {	\
      TNL_VERTEX *v2 = IMM_VERTICES( vert0 );			\
      TNL_VERTEX *v1 = IMM_VERTICES( vert1 );			\
      TAG(draw_tri)( ctx, v1, v0, v2 );				\
   } else if (EXTEND_PRIM( 1 )) {				\
      EMIT_VERTEX( v0 );					\
   } else {							\
      TNL_VERTEX *v2 = IMM_VERTICES( vert0 );			\
      TNL_VERTEX *v1 = IMM_VERTICES( vert1 );			\
      BEGIN_PRIM( GL_POLYGON, 3 );				\
      EMIT_VERTEX( v2 );					\
      EMIT_VERTEX( v1 );					\
      EMIT_VERTEX( v0 );					\
   }      

static void TAG(flush_poly_2)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;
   ACTIVE_VERTEX = IMM_VERTICES( 1 );
   FLUSH_VERTEX = TAG(flush_poly_3);
   DO_POLY_TRI( 0, 1 );
}

static void TAG(flush_poly_3)( GLcontext *ctx, TNL_VERTEX *v0 )
{
   LOCAL_VARS;
   ACTIVE_VERTEX = IMM_VERTICES( 2 );
   FLUSH_VERTEX = TAG(flush_poly_2);
   DO_POLY_TRI( 0, 2 );
}


void (*TAG(flush_tab)[GL_POLYGON+1])( GLcontext *, TNL_VERTEX * ) =
{
   TAG(flush_point),
   TAG(flush_line_0),
   TAG(flush_line_loop_0),
   TAG(flush_line_strip_0),
   TAG(flush_triangle_0),
   TAG(flush_tri_strip_0),
   TAG(flush_tri_fan_0),
   TAG(flush_quad_0),
   TAG(flush_quad_strip_0),
   TAG(flush_poly_0),
};


#ifndef PRESERVE_PRIM_DEFS
#undef LOCAL_VARS
#undef GET_INTERP_FUNC
#undef IMM_VERTEX
#undef IMM_VERTICES
#undef FLUSH_VERTEX
#endif
#undef PRESERVE_PRIM_DEFS
#undef EXTEND_PRIM
#undef EMIT_VERTEX
#undef EMIT_VERTEX_TRI
#undef EMIT_VERTEX_LINE
#undef EMIT_VERTEX_POINT
#undef TAG
