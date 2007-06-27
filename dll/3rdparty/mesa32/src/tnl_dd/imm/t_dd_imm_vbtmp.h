
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

/* Template to build support for t_dd_imm_* tnl module using vertices
 * as defined in t_dd_vertex.h.
 *
 * See t_dd_vbtmp.h for definitions of arguments to this file.
 * Unfortunately it seems necessary to duplicate a lot of that code.
 */

#ifndef LOCALVARS
#define LOCALVARS
#endif



/* COPY_VERTEX_FROM_CURRENT in t_dd_imm_vapi.c
 */
static void TAG(emit_vfmt)( GLcontext *ctx, VERTEX *v )
{
   LOCALVARS
      ;

   /* This template assumes (like t_dd_vbtmp.h) that color is ubyte.
    */
   if (DO_TEX0 || DO_TEX1 || !HAVE_TINY_VERTICES)
   {
      const GLubyte *col = GET_HARDWARE_COLOR();
      if (HAVE_RGBA_COLOR) {
	 v->v.ui[4] = *(GLuint *)&col;
      } else {
	 v->v.color.blue  = col[2];
	 v->v.color.green = col[1];
	 v->v.color.red   = col[0];
	 v->v.color.alpha = col[3];
      }
   }
   else {
      if (HAVE_RGBA_COLOR) {
	 v->v.ui[3] = *(GLuint *)col;
      }
      else {
	 v->tv.color.blue  = col[2];
	 v->tv.color.green = col[1];
	 v->tv.color.red   = col[0];
	 v->tv.color.alpha = col[3];
      }
   }

   if (DO_TEX0) {
      GLfloat *tc = ctx->Current.Texture[0];
      v->v.u0 = tc[0];
      v->v.v0 = tc[1];
      if (DO_PTEX) {
	 if (HAVE_PTEX_VERTICES) {
	    v->pv.q0 = tc[3];
	 } 
	 else {
	    float rhw = 1.0 / tc[3];
	    v->v.w *= tc[3];
	    v->v.u0 *= rhw;
	    v->v.v0 *= rhw;
	 } 
      } 
   }
   if (DO_TEX1) {
      GLfloat *tc = ctx->Current.Texture[1];
      if (DO_PTEX) {
	 v->pv.u1 = tc[0];
	 v->pv.v1 = tc[1];
	 v->pv.q1 = tc[3];
      } 
      else {
	 v->v.u1 = tc[0];
	 v->v.v1 = tc[1];
      }
   } 
   else if (DO_PTEX) {
      *(GLuint *)&v->pv.q1 = 0;	/* avoid culling on radeon */
   }
   if (DO_TEX2) {
      GLfloat *tc = ctx->Current.Texture[2];
      if (DO_PTEX) {
	 v->pv.u2 = tc[0];
	 v->pv.v2 = tc[1];
	 v->pv.q2 = tc[3];
      } 
      else {
	 v->v.u2 = tc[0];
	 v->v.v2 = tc[1];
      }
   } 
   if (DO_TEX3) {
      GLfloat *tc = ctx->Current.Texture[3];
      if (DO_PTEX) {
	 v->pv.u3 = tc[0];
	 v->pv.v3 = tc[1];
	 v->pv.q3 = tc[3];
      } 
      else {
	 v->v.u3 = tc[0];
	 v->v.v3 = tc[1];
      }
   } 
}




static void TAG(interp)( GLcontext *ctx,
			 GLfloat t,
			 TNL_VERTEX *dst,
			 TNL_VERTEX *in,
			 TNL_VERTEX *out )
{
   LOCALVARS
   const GLfloat *s = GET_VIEWPORT_MAT();
   GLfloat w;

   (void)s;

   if (HAVE_HW_DIVIDE) {
      VIEWPORT_X( dst->v.v.x, dst->clip[0] );
      VIEWPORT_Y( dst->v.v.y, dst->clip[1] );
      VIEWPORT_Z( dst->v.v.z, dst->clip[2] );
      w = dstclip[3];
   }
   else {
      w = 1.0 / dst->clip[3];
      VIEWPORT_X( dst->v.v.x, dst->clip[0] * w );
      VIEWPORT_Y( dst->v.v.y, dst->clip[1] * w );
      VIEWPORT_Z( dst->v.v.z, dst->clip[2] * w );
   }

   if (HAVE_HW_DIVIDE || DO_TEX0) {

      dst->v.v.w = w;

      INTERP_UB( t, dst->v.ub4[4][0], out->v.ub4[4][0], in->v.ub4[4][0] );
      INTERP_UB( t, dst->v.ub4[4][1], out->v.ub4[4][1], in->v.ub4[4][1] );
      INTERP_UB( t, dst->v.ub4[4][2], out->v.ub4[4][2], in->v.ub4[4][2] );
      INTERP_UB( t, dst->v.ub4[4][3], out->v.ub4[4][3], in->v.ub4[4][3] );

      if (DO_TEX0) {
	 if (DO_PTEX) {
	    if (HAVE_PTEX_VERTICES) {
	       INTERP_F( t, dst->v.pv.u0, out->v.pv.u0, in->v.pv.u0 );
	       INTERP_F( t, dst->v.pv.v0, out->v.pv.v0, in->v.pv.v0 );
	       INTERP_F( t, dst->v.pv.q0, out->v.pv.q0, in->v.pv.q0 );
	    } else {
	       GLfloat wout = out->clip[3]; /* projected clip */
	       GLfloat win = in->clip[3]; /* projected clip */
	       GLfloat qout = out->v.pv.w / wout;
	       GLfloat qin = in->v.pv.w / win;
	       GLfloat qdst, rqdst;

	       ASSERT( !HAVE_HW_DIVIDE ); /* assert win, wout projected clip */

	       INTERP_F( t, dst->v.v.u0, out->v.v.u0 * qout, in->v.v.u0 * qin );
	       INTERP_F( t, dst->v.v.v0, out->v.v.v0 * qout, in->v.v.v0 * qin );
	       INTERP_F( t, qdst, qout, qin );

	       rqdst = 1.0 / qdst;
	       dst->v.v.u0 *= rqdst;
	       dst->v.v.v0 *= rqdst;
	       dst->v.v.w *= rqdst;
	    }
	 }
	 else {
	    INTERP_F( t, dst->v.v.u0, out->v.v.u0, in->v.v.u0 );
	    INTERP_F( t, dst->v.v.v0, out->v.v.v0, in->v.v.v0 );
	 }
      }
      if (DO_TEX1) {
	 if (DO_PTEX) {
	    INTERP_F( t, dst->v.pv.u1, out->v.pv.u1, in->v.pv.u1 );
	    INTERP_F( t, dst->v.pv.v1, out->v.pv.v1, in->v.pv.v1 );
	    INTERP_F( t, dst->v.pv.q1, out->v.pv.q1, in->v.pv.q1 );
	 } else {
	    INTERP_F( t, dst->v.v.u1, out->v.v.u1, in->v.v.u1 );
	    INTERP_F( t, dst->v.v.v1, out->v.v.v1, in->v.v.v1 );
	 }
      }
      else if (DO_PTEX) {
	 dst->v.pv.q0 = 0.0;	/* must be a valid float on radeon */
      }
      if (DO_TEX2) {
	 if (DO_PTEX) {
	    INTERP_F( t, dst->v.pv.u2, out->v.pv.u2, in->v.pv.u2 );
	    INTERP_F( t, dst->v.pv.v2, out->v.pv.v2, in->v.pv.v2 );
	    INTERP_F( t, dst->v.pv.q2, out->v.pv.q2, in->v.pv.q2 );
	 } else {
	    INTERP_F( t, dst->v.v.u2, out->v.v.u2, in->v.v.u2 );
	    INTERP_F( t, dst->v.v.v2, out->v.v.v2, in->v.v.v2 );
	 }
      }
      if (DO_TEX3) {
	 if (DO_PTEX) {
	    INTERP_F( t, dst->v.pv.u3, out->v.pv.u3, in->v.pv.u3 );
	    INTERP_F( t, dst->v.pv.v3, out->v.pv.v3, in->v.pv.v3 );
	    INTERP_F( t, dst->v.pv.q3, out->v.pv.q3, in->v.pv.q3 );
	 } else {
	    INTERP_F( t, dst->v.v.u3, out->v.v.u3, in->v.v.u3 );
	    INTERP_F( t, dst->v.v.v3, out->v.v.v3, in->v.v.v3 );
	 }
      }
   } else {
      /* 4-dword vertex.  Color is in v[3] and there is no oow coordinate.
       */
      INTERP_UB( t, dst->v.ub4[3][0], out->v.ub4[3][0], in->v.ub4[3][0] );
      INTERP_UB( t, dst->v.ub4[3][1], out->v.ub4[3][1], in->v.ub4[3][1] );
      INTERP_UB( t, dst->v.ub4[3][2], out->v.ub4[3][2], in->v.ub4[3][2] );
      INTERP_UB( t, dst->v.ub4[3][3], out->v.ub4[3][3], in->v.ub4[3][3] );
   }
}


static __inline void TAG(copy_pv)( GLcontext *ctx,
				   TNL_VERTEX *dst, 
				   TNL_VERTEX *src )
{
   if (DO_TEX0 || DO_TEX1 || !HAVE_TINY_VERTICES) {
      dst->v.v.ui[4] = src->v.v.ui[4];
   }
   else {
      dst->v.v.ui[3] = src->v.v.ui[3];
   }
}



static void TAG(init)( void )
{
   setup_tab[IND].emit = TAG(emit_vfmt);
   setup_tab[IND].interp = TAG(interp_vfmt);
}


#undef IND
#undef TAG



