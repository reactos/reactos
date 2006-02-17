
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
 *    Gareth Hughes <gareth@valinux.com>
 *    Keith Whitwell <keithw@valinux.com>
 */

/* Template for immediate mode vertex functions.
 */

#define DBG 0

#define VERTEX( ox, oy, oz, ow )
do {
   GET_CURRENT_VERTEX;
   GLfloat w;
   GLuint mask;
   const GLfloat * const m = ctx->_ModelProjectMatrix.m;

   if (DO_FULL_MATRIX) {
      VERTEX_CLIP(0) = m[0] * ox + m[4] * oy + m[8]  * oz + m[12] * ow;
      VERTEX_CLIP(1) = m[1] * ox + m[5] * oy + m[9]  * oz + m[13] * ow;
      VERTEX_CLIP(2) = m[2] * ox + m[6] * oy + m[10] * oz + m[14] * ow;
      VERTEX_CLIP(3) = m[3] * ox + m[7] * oy + m[11] * oz + m[15] * ow;
      w = VERTEX_CLIP(3);
   }
   else if (DO_NOROT_MATRIX) {
      VERTEX_CLIP(0) = m[0] * ox                          + m[12] * ow;
      VERTEX_CLIP(1) =             m[5] * oy              + m[13] * ow;
      VERTEX_CLIP(2) =                         m[10] * oz + m[14] * ow;
      VERTEX_CLIP(3) =                                              ow;
      w = ow;
   } 
   else {
      ASSERT (DO_IDENTITY_MATRIX);
      VERTEX_CLIP(0) = ox;
      VERTEX_CLIP(1) = oy;
      VERTEX_CLIP(2) = oz;
      VERTEX_CLIP(3) = ow;
      w = ow;
   }

   mask = 0;
   if (DO_CLIP_TEST) {
      if ( VERTEX_CLIP(0) >  w ) mask |= CLIP_RIGHT_BIT;
      if ( VERTEX_CLIP(0) < -w ) mask |= CLIP_LEFT_BIT;
      if ( VERTEX_CLIP(1) >  w ) mask |= CLIP_TOP_BIT;
      if ( VERTEX_CLIP(1) < -w ) mask |= CLIP_BOTTOM_BIT;
      if ( VERTEX_CLIP(2) >  w ) mask |= CLIP_FAR_BIT;
      if ( VERTEX_CLIP(2) < -w ) mask |= CLIP_NEAR_BIT;
      VERTEX_MASK(v) = mask;
   }

   if (!mask) {
      if (HAVE_VERTEX_WIN) {
	 if (!HAVE_HW_VIEWPORT) {
	    const GLfloat *s = GET_VIEWPORT_MATRIX();
	    if (HAVE_W && HAVE_HW_DIVIDE) {
	       VERTEX_WIN( 0 ) = s[0]  * VERTEX_CLIP( 0 ) + s[12];
	       VERTEX_WIN( 1 ) = s[5]  * VERTEX_CLIP( 1 ) + s[13];
	       VERTEX_WIN( 2 ) = s[10] * VERTEX_CLIP( 2 ) + s[14];
	       VERTEX_WIN( 3 ) = w;
	    }
	    else {
	       const GLfloat oow = 1.0/w; /* possibly opt away */
	       VERTEX_WIN( 0 ) = s[0]  * VERTEX_CLIP( 0 ) * oow + s[12];
	       VERTEX_WIN( 1 ) = s[5]  * VERTEX_CLIP( 1 ) * oow + s[13];
	       VERTEX_WIN( 2 ) = s[10] * VERTEX_CLIP( 2 ) * oow + s[14];
	       if (HAVE_W)
		  VERTEX_WIN( 3 ) = oow;
	    }
	 }
	 else if (HAVE_W && HAVE_HW_DIVIDE) {
	    if (!VERTEX_WIN_IS_VERTEX_CLIP) {
	       VERTEX_WIN( 0 ) = VERTEX_CLIP( 0 );
	       VERTEX_WIN( 1 ) = VERTEX_CLIP( 1 );
	       VERTEX_WIN( 2 ) = VERTEX_CLIP( 2 );
	       VERTEX_WIN( 3 ) = w;
	    }
	 }
	 else {
	    const GLfloat oow = 1.0/w; /* possibly opt away */
	    VERTEX_WIN( 0 ) = VERTEX_CLIP( 0 ) * oow;
	    VERTEX_WIN( 1 ) = VERTEX_CLIP( 1 ) * oow;
	    VERTEX_WIN( 2 ) = VERTEX_CLIP( 2 ) * oow;
	    if (HAVE_W)
	       VERTEX_WIN( 3 ) = oow;
	 }
      }
   } else if (!FALLBACK_OR_CLIPPING) {
      SET_CLIPPING();		/* transition to clipping */
   }

   COPY_VERTEX_FROM_CURRENT;
   BUILD_PRIM_FROM_VERTEX;
}

/* Let the compiler optimize away the constant operations:
 */
static void VTAG(Vertex2f)( GLfloat ox, GLfloat oy )
{
   /* Cliptest on clip[2] could also be eliminated...
    */
   VERTEX( ox, oy, 0, 1 );
}

static void VTAG(Vertex2fv)( const GLfloat *obj )
{
   /* Cliptest on clip[2] could also be eliminated...
    */
   VERTEX( obj[0], obj[1], 0, 1 );
}

static void VTAG(Vertex3f)( GLfloat ox, GLfloat oy, GLfloat oz )
{
   VERTEX( ox, oy, oz, 1 );
}

static void VTAG(Vertex3fv)( const GLfloat *obj )
{
   VERTEX( obj[0], obj[1], obj[2], 1 );
}

static void VTAG(Vertex4f)( GLfloat ox, GLfloat oy, GLfloat oz, GLfloat ow )
{
   VERTEX( ox, oy, oz, ow );
}

static void VTAG(Vertex4fv)( const GLfloat *obj )
{
   VERTEX( obj[0], obj[1], obj[2], obj[3] );
}


#undef DO_FULL_MATRIX
#undef VTAG
#undef VERTEX
