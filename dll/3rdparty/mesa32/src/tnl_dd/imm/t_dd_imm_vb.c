
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
 *    Keith Whitwell <keith_whitwell@yahoo.com>
 */

/* Template to build clipping routines to support t_dd_imm_primtmp.h.
 *
 * The TAG(draw_line) and TAG(draw_triangle) routines are called in
 * clipping and fallback scenarios, and when the native hardware
 * primitive (eg polygons) is unavailable.
 */


#define CLIP_DOTPROD(K, A, B, C, D)		\
   (CLIP_X(K)*A + CLIP_Y(K)*B + 	\
    CLIP_Z(K)*C + CLIP_W(K)*D)

#define POLY_CLIP( PLANE, A, B, C, D )					\
do {									\
   if (mask & PLANE) {							\
      TNL_VERTEX **indata = inlist[in];					\
      TNL_VERTEX **outdata = inlist[in ^= 1];				\
      TNL_VERTEX *J = indata[0];					\
      GLfloat dpJ = CLIP_DOTPROD(J, A, B, C, D );			\
      GLuint outcount = 0;						\
      GLuint i;								\
									\
      indata[n] = indata[0]; /* prevent rotation of vertices */		\
      for (i = 1; i <= n; i++) {					\
	 TNL_VERTEX *I = indata[i];					\
	 GLfloat dpI = CLIP_DOTPROD(idx, A, B, C, D );			\
									\
	 if (!NEGATIVE(dpPrev)) {					\
	    outdata[outcount++] = J;					\
	 }								\
									\
	 if (DIFFERENT_SIGNS(dpI, dpJ)) {				\
            TNL_VERTEX *O = verts++;					\
            outdata[outcount++] = O;					\
	    if (NEGATIVE(dpI)) {					\
	       /* Going out of bounds.  Avoid division by zero as we	\
		* know dp != dpPrev from DIFFERENT_SIGNS, above.	\
		*/							\
	       GLfloat t = dpI / (dpI - dpJ);				\
      	       INTERP( ctx, t, O, I, J );				\
	    } else {							\
	       /* Coming back in.					\
		*/							\
	       GLfloat t = dpJ / (dpJ - dpI);				\
	       INTERP( ctx, t, O, J, I );				\
	    }								\
	 }								\
									\
	 J = I;								\
	 dpJ = dpI;							\
      }									\
									\
      if (outcount < 3)							\
	 return;							\
									\
      nr = outcount;							\
   }									\
} while (0)


#define LINE_CLIP(PLANE, A, B, C, D )			\
do {							\
   if (mask & PLANE) {					\
      GLfloat dpI = CLIP_DOTPROD( I, A, B, C, D );	\
      GLfloat dpJ = CLIP_DOTPROD( J, A, B, C, D );	\
							\
      if (DIFFERENT_SIGNS(dpI, dpJ)) {			\
         TNL_VERTEX *O = verts++;			\
	 if (NEGATIVE(dpJ)) {				\
	    GLfloat t = dpI / (dpI - dpJ);		\
	    INTERP( ctx, t, O, I, J );	\
            J = O;					\
	 } else {					\
  	    GLfloat t = dpJ / (dpJ - dpI);		\
	    INTERP( ctx, t, O, J, I );	\
            I = O;					\
	 }						\
      }							\
      else if (NEGATIVE(dpI))				\
	 return;					\
  }							\
} while (0)



/* Clip a line against the viewport and user clip planes.
 */
static void TAG(clip_draw_line)( GLcontext *ctx,
				 TNL_VERTEX *I,
				 TNL_VERTEX *J,
				 GLuint mask )
{
   LOCAL_VARS;
   GET_INTERP_FUNC;
   TNL_VERTEX tmp[MAX_CLIPPED_VERTICES];
   TNL_VERTEX *verts = tmp;
   TNL_VERTEX *pv = J;

   LINE_CLIP( CLIP_RIGHT_BIT,  -1,  0,  0, 1 );
   LINE_CLIP( CLIP_LEFT_BIT,    1,  0,  0, 1 );
   LINE_CLIP( CLIP_TOP_BIT,     0, -1,  0, 1 );
   LINE_CLIP( CLIP_BOTTOM_BIT,  0,  1,  0, 1 );
   LINE_CLIP( CLIP_FAR_BIT,     0,  0, -1, 1 );
   LINE_CLIP( CLIP_NEAR_BIT,    0,  0,  1, 1 );

   if ((ctx->_TriangleCaps & DD_FLATSHADE) && J != pv)
      COPY_PV( ctx, J, pv );

   DRAW_LINE( I, J );
}


/* Clip a triangle against the viewport and user clip planes.
 */
static void TAG(clip_draw_triangle)( GLcontext *ctx,
				     TNL_VERTEX *v0,
				     TNL_VERTEX *v1,
				     TNL_VERTEX *v2,
				     GLuint mask )
{
   LOCAL_VARS;
   GET_INTERP_FUNC;
   TNL_VERTEX tmp[MAX_CLIPPED_VERTICES];
   TNL_VERTEX *verts = tmp;
   TNL_VERTEX *(inlist[2][MAX_CLIPPED_VERTICES]);
   TNL_VERTEX **out;
   GLuint in = 0;
   GLuint n = 3;
   GLuint i;

   ASSIGN_3V(inlist, v2, v0, v1 ); /* pv rotated to slot zero */

   POLY_CLIP( CLIP_RIGHT_BIT,  -1,  0,  0, 1 );
   POLY_CLIP( CLIP_LEFT_BIT,    1,  0,  0, 1 );
   POLY_CLIP( CLIP_TOP_BIT,     0, -1,  0, 1 );
   POLY_CLIP( CLIP_BOTTOM_BIT,  0,  1,  0, 1 );
   POLY_CLIP( CLIP_FAR_BIT,     0,  0, -1, 1 );
   POLY_CLIP( CLIP_NEAR_BIT,    0,  0,  1, 1 );

   if ((ctx->_TriangleCaps & DD_FLATSHADE) && v2 != inlist[0]) 
      COPY_PV( ctx, inlist[0], v2 );

   out = inlist[in];
   DRAW_POLYGON( out, n );
}


static __inline void TAG(draw_triangle)( GLcontext *ctx,
					 TNL_VERTEX *v0,
					 TNL_VERTEX *v1,
					 TNL_VERTEX *v2 )
{
   LOCAL_VARS;
   GLubyte ormask = (v0->mask | v1->mask | v2->mask);

   if ( !ormask ) {
      DRAW_TRI( v0, v1, v2 );
   } else if ( !(v0->mask & v1->mask & v2->mask) ) {
      TAG(clip_draw_triangle)( ctx, v0, v1, v2, ormask );
   }
}

static __inline void TAG(draw_line)( GLcontext *ctx,
				     TNL_VERTEX *v0,
				     TNL_VERTEX *v1 )
{
   LOCAL_VARS;
   GLubyte ormask = (v0->mask | v1->mask);

   if ( !ormask ) {
      DRAW_LINE( v0, v1 );
   } else if ( !(v0->mask & v1->mask) ) {
      TAG(clip_draw_line)( ctx, v0, v1, ormask );
   }
}

