/* $XFree86$ */ /* -*- mode: c; c-basic-offset: 3 -*- */
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
 * Original authors:
 *    Keith Whitwell <keithw@valinux.com>
 *
 * Adapted to Mach64 by:
 *    José Fonseca <j_r_fonseca@yahoo.co.uk>
 */

#include "math/m_translate.h"

#ifndef LOCALVARS
#define LOCALVARS
#endif

void TAG(translate_vertex)(GLcontext *ctx,
			   const VERTEX *src,
			   SWvertex *dst)
{
   LOCALVARS
   GLuint format = GET_VERTEX_FORMAT();
   UNVIEWPORT_VARS;
   CARD32 *p = (CARD32 *)src + 10 - mmesa->vertex_size;

   dst->win[3] = 1.0;
   
   switch ( format ) {
      case TEX1_VERTEX_FORMAT:
#ifdef MACH64_PREMULT_TEXCOORDS
	 {
	    float rhw = 1.0 / LE32_IN_FLOAT( p + 2 );
	    
	    dst->texcoord[1][0] = rhw*LE32_IN_FLOAT( p++ );
	    dst->texcoord[1][1] = rhw*LE32_IN_FLOAT( p++ );
	 }
#else
	 dst->texcoord[1][0] = LE32_IN_FLOAT( p++ );
	 dst->texcoord[1][1] = LE32_IN_FLOAT( p++ );
#endif
	 dst->texcoord[1][3] = 1.0;
	 p++;

      case TEX0_VERTEX_FORMAT:
#ifdef MACH64_PREMULT_TEXCOORDS
	 {
	    float rhw = 1.0 / LE32_IN_FLOAT( p + 2 );
	    
	    dst->texcoord[0][0] = rhw*LE32_IN_FLOAT( p++ );
	    dst->texcoord[0][1] = rhw*LE32_IN_FLOAT( p++ );
	 }
#else
	 dst->texcoord[0][0] = LE32_IN_FLOAT( p++ );
	 dst->texcoord[0][1] = LE32_IN_FLOAT( p++ );
#endif
	 dst->texcoord[0][3] = 1.0;
	 dst->win[3] = LE32_IN_FLOAT( p++ );
	
      case NOTEX_VERTEX_FORMAT:
	 dst->specular[2] = ((GLubyte *)p)[0];
	 dst->specular[1] = ((GLubyte *)p)[1];
	 dst->specular[0] = ((GLubyte *)p)[2];
	 dst->fog = ((GLubyte *)p)[3];
	 p++;

      case TINY_VERTEX_FORMAT:
	 dst->win[2] = UNVIEWPORT_Z( LE32_IN( p++ ) );

	 dst->color[2] = ((GLubyte *)p)[0];
	 dst->color[1] = ((GLubyte *)p)[1];
	 dst->color[0] = ((GLubyte *)p)[2];
	 dst->color[3] = ((GLubyte *)p)[3];
	 p++;
	 
	 {
	    GLuint xy = LE32_IN( p );
	    
	    dst->win[0] = UNVIEWPORT_X( (GLfloat)(GLshort)( xy >> 16 ) );
	    dst->win[1] = UNVIEWPORT_Y( (GLfloat)(GLshort)( xy & 0xffff ) );
	 }
   }

   assert( p + 1 - (CARD32 *)src == 10 );
	 
   dst->pointSize = ctx->Point._Size;
}



void TAG(print_vertex)( GLcontext *ctx, const VERTEX *v )
{
   LOCALVARS
   GLuint format = GET_VERTEX_FORMAT();
   CARD32 *p = (CARD32 *)v + 10 - mmesa->vertex_size;
   
   switch ( format ) {
      case TEX1_VERTEX_FORMAT:
	 {
	    GLfloat u, v, w;
#ifdef MACH64_PREMULT_TEXCOORDS
	    float rhw = 1.0 / LE32_IN_FLOAT( p + 2 );
	    
	    u = rhw*LE32_IN_FLOAT( p++ );
	    v = rhw*LE32_IN_FLOAT( p++ );
#else
	    u = LE32_IN_FLOAT( p++ );
	    v = LE32_IN_FLOAT( p++ );
#endif
	    w = LE32_IN_FLOAT( p++ );
	    fprintf( stderr, "u1 %f v1 %f w1 %f\n", u, v, w );
	 }

      case TEX0_VERTEX_FORMAT:
	 {
	    GLfloat u, v, w;
#ifdef MACH64_PREMULT_TEXCOORDS
	    float rhw = 1.0 / LE32_IN_FLOAT( p + 2 );
	    
	    u = rhw*LE32_IN_FLOAT( p++ );
	    v = rhw*LE32_IN_FLOAT( p++ );
#else
	    u = LE32_IN_FLOAT( p++ );
	    v = LE32_IN_FLOAT( p++ );
#endif
	    w = LE32_IN_FLOAT( p++ );
	    fprintf( stderr, "u0 %f v0 %f w0 %f\n", u, v, w );
	 }
	
      case NOTEX_VERTEX_FORMAT:
	 {
	    GLubyte r, g, b, a;
	    
	    b = ((GLubyte *)p)[0];
	    g = ((GLubyte *)p)[1];
	    r = ((GLubyte *)p)[2];
	    a = ((GLubyte *)p)[3];
	    p++;
	    fprintf(stderr, "spec: r %d g %d b %d a %d\n", r, g, b, a);
	 }

      case TINY_VERTEX_FORMAT:
	 {
	    GLuint xy;
	    GLfloat x, y, z;
	    GLubyte r, g, b, a;
	    
	    z = LE32_IN( p++ ) / 65536.0;

	    b = ((GLubyte *)p)[0];
	    g = ((GLubyte *)p)[1];
	    r = ((GLubyte *)p)[2];
	    a = ((GLubyte *)p)[3];
	    p++;
	    xy = LE32_IN( p );
	    x = (GLfloat)(GLshort)( xy >> 16 ) / 4.0;
	    y = (GLfloat)(GLshort)( xy & 0xffff ) / 4.0;
	    
	    fprintf(stderr, "x %f y %f z %f\n", x, y, z);
	    fprintf(stderr, "r %d g %d b %d a %d\n", r, g, b, a);
	 }
   }
   
   assert( p + 1 - (CARD32 *)v == 10 );	 

   fprintf(stderr, "\n");
}

/* Interpolate the elements of the VB not included in typical hardware
 * vertices.  
 *
 * NOTE: All these arrays are guarenteed by tnl to be writeable and
 * have good stride.
 */
#ifndef INTERP_QUALIFIER 
#define INTERP_QUALIFIER static
#endif

#define GET_COLOR(ptr, idx) ((ptr)->data[idx])


INTERP_QUALIFIER void TAG(interp_extras)( GLcontext *ctx,
					  GLfloat t,
					  GLuint dst, GLuint out, GLuint in,
					  GLboolean force_boundary )
{
   LOCALVARS
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   if (VB->ColorPtr[1]) {
      assert(VB->ColorPtr[1]->stride == 4 * sizeof(GLfloat));
      
      INTERP_4F( t,
		    GET_COLOR(VB->ColorPtr[1], dst),
		    GET_COLOR(VB->ColorPtr[1], out),
		    GET_COLOR(VB->ColorPtr[1], in) );

      if (VB->SecondaryColorPtr[1]) {
	 INTERP_3F( t,
		       GET_COLOR(VB->SecondaryColorPtr[1], dst),
		       GET_COLOR(VB->SecondaryColorPtr[1], out),
		       GET_COLOR(VB->SecondaryColorPtr[1], in) );
      }
   }

   if (VB->EdgeFlag) {
      VB->EdgeFlag[dst] = VB->EdgeFlag[out] || force_boundary;
   }

   INTERP_VERTEX(ctx, t, dst, out, in, force_boundary);
}

INTERP_QUALIFIER void TAG(copy_pv_extras)( GLcontext *ctx, 
					   GLuint dst, GLuint src )
{
   LOCALVARS
      struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   if (VB->ColorPtr[1]) {
      COPY_4FV( GET_COLOR(VB->ColorPtr[1], dst), 
		GET_COLOR(VB->ColorPtr[1], src) );

      if (VB->SecondaryColorPtr[1]) {
	 COPY_4FV( GET_COLOR(VB->SecondaryColorPtr[1], dst), 
		   GET_COLOR(VB->SecondaryColorPtr[1], src) );
      }
   }

   COPY_PV_VERTEX(ctx, dst, src);
}


#undef INTERP_QUALIFIER
#undef GET_COLOR

#undef IND
#undef TAG
