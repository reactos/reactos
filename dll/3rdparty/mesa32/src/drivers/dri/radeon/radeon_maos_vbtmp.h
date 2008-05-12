/*
 * Mesa 3-D graphics library
 * Version:  4.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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

#ifndef LOCALVARS
#define LOCALVARS
#endif

#undef TCL_DEBUG
#ifndef TCL_DEBUG
#define TCL_DEBUG 0
#endif

static void TAG(emit)( GLcontext *ctx,
		       GLuint start, GLuint end,
		       void *dest )
{
   LOCALVARS
      struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLuint (*tc0)[4], (*tc1)[4], (*tc2)[4];
   GLfloat (*col)[4], (*spec)[4];
   GLfloat (*fog)[4];
   GLuint (*norm)[4];
   GLuint tc0_stride, tc1_stride, col_stride, spec_stride, fog_stride;
   GLuint tc2_stride, norm_stride;
   GLuint fill_tex = 0;
   GLuint rqcoordsnoswap = 0;
   GLuint (*coord)[4];
   GLuint coord_stride; /* object coordinates */
   int i;

   union emit_union *v = (union emit_union *)dest;

   if (RADEON_DEBUG & DEBUG_VERTS)
      fprintf(stderr, "%s\n", __FUNCTION__); 

   coord = (GLuint (*)[4])VB->ObjPtr->data;
   coord_stride = VB->ObjPtr->stride;

   if (DO_TEX2) {
      if (VB->TexCoordPtr[2]) {
	 const GLuint t2 = GET_TEXSOURCE(2);
	 tc2 = (GLuint (*)[4])VB->TexCoordPtr[t2]->data;
	 tc2_stride = VB->TexCoordPtr[t2]->stride;
	 if (DO_PTEX && VB->TexCoordPtr[t2]->size < 3) {
	    fill_tex |= (1<<2);
	 }
	 else if (DO_PTEX && VB->TexCoordPtr[t2]->size < 4) {
	    rqcoordsnoswap |= (1<<2);
	 }
      } else {
	 tc2 = (GLuint (*)[4])&ctx->Current.Attrib[VERT_ATTRIB_TEX2];
	 tc2_stride = 0;
      }
   }

   if (DO_TEX1) {
      if (VB->TexCoordPtr[1]) {
	 const GLuint t1 = GET_TEXSOURCE(1);
	 tc1 = (GLuint (*)[4])VB->TexCoordPtr[t1]->data;
	 tc1_stride = VB->TexCoordPtr[t1]->stride;
	 if (DO_PTEX && VB->TexCoordPtr[t1]->size < 3) {
	    fill_tex |= (1<<1);
	 }
	 else if (DO_PTEX && VB->TexCoordPtr[t1]->size < 4) {
	    rqcoordsnoswap |= (1<<1);
	 }
      } else {
	 tc1 = (GLuint (*)[4])&ctx->Current.Attrib[VERT_ATTRIB_TEX1];
	 tc1_stride = 0;
      }
   }

   if (DO_TEX0) {
      if (VB->TexCoordPtr[0]) {
	 const GLuint t0 = GET_TEXSOURCE(0);
	 tc0_stride = VB->TexCoordPtr[t0]->stride;
	 tc0 = (GLuint (*)[4])VB->TexCoordPtr[t0]->data;
	 if (DO_PTEX && VB->TexCoordPtr[t0]->size < 3) {
	    fill_tex |= (1<<0);
	 }
	 else if (DO_PTEX && VB->TexCoordPtr[t0]->size < 4) {
	    rqcoordsnoswap |= (1<<0);
	 }
      } else {
	 tc0 = (GLuint (*)[4])&ctx->Current.Attrib[VERT_ATTRIB_TEX0];
	 tc0_stride = 0;
      }
	 
   }

   if (DO_NORM) {
      if (VB->NormalPtr) {
	 norm_stride = VB->NormalPtr->stride;
	 norm = (GLuint (*)[4])VB->NormalPtr->data;
      } else {
	 norm_stride = 0;
	 norm = (GLuint (*)[4])&ctx->Current.Attrib[VERT_ATTRIB_NORMAL];
      }
   }

   if (DO_RGBA) {
      if (VB->ColorPtr[0]) {
	 col = VB->ColorPtr[0]->data;
	 col_stride = VB->ColorPtr[0]->stride;
      } else {
	 col = (GLfloat (*)[4])ctx->Current.Attrib[VERT_ATTRIB_COLOR0];
	 col_stride = 0;
      }
   }

   if (DO_SPEC_OR_FOG) {
      if (VB->SecondaryColorPtr[0]) {
	 spec = VB->SecondaryColorPtr[0]->data;
	 spec_stride = VB->SecondaryColorPtr[0]->stride;
      } else {
	 spec = (GLfloat (*)[4])ctx->Current.Attrib[VERT_ATTRIB_COLOR1];
	 spec_stride = 0;
      }
   }

   if (DO_SPEC_OR_FOG) {
      if (VB->FogCoordPtr) {
	 fog = VB->FogCoordPtr->data;
	 fog_stride = VB->FogCoordPtr->stride;
      } else {
	 fog = (GLfloat (*)[4])ctx->Current.Attrib[VERT_ATTRIB_FOG];
	 fog_stride = 0;
      }
   }
   
   
   if (start) {
      coord =  (GLuint (*)[4])((GLubyte *)coord + start * coord_stride);
      if (DO_TEX0)
	 tc0 =  (GLuint (*)[4])((GLubyte *)tc0 + start * tc0_stride);
      if (DO_TEX1) 
	 tc1 =  (GLuint (*)[4])((GLubyte *)tc1 + start * tc1_stride);
      if (DO_TEX2) 
	 tc2 =  (GLuint (*)[4])((GLubyte *)tc2 + start * tc2_stride);
      if (DO_NORM) 
	 norm =  (GLuint (*)[4])((GLubyte *)norm + start * norm_stride);
      if (DO_RGBA) 
	 STRIDE_4F(col, start * col_stride);
      if (DO_SPEC)
	 STRIDE_4F(spec, start * spec_stride);
      if (DO_FOG)
	 STRIDE_4F(fog, start * fog_stride);
   }


   {
      for (i=start; i < end; i++) {
	 
	 v[0].ui = coord[0][0];
	 v[1].ui = coord[0][1];
	 v[2].ui = coord[0][2];
	 if (DO_W) {
	    v[3].ui = coord[0][3];
	    v += 4;
	 } 
	 else
	    v += 3;
	 coord =  (GLuint (*)[4])((GLubyte *)coord +  coord_stride);

	 if (DO_NORM) {
	    v[0].ui = norm[0][0];
	    v[1].ui = norm[0][1];
	    v[2].ui = norm[0][2];
	    v += 3;
	    norm =  (GLuint (*)[4])((GLubyte *)norm +  norm_stride);
	 }
	 if (DO_RGBA) {
	    UNCLAMPED_FLOAT_TO_UBYTE(v[0].rgba.red, col[0][0]);
	    UNCLAMPED_FLOAT_TO_UBYTE(v[0].rgba.green, col[0][1]);
	    UNCLAMPED_FLOAT_TO_UBYTE(v[0].rgba.blue, col[0][2]);
	    UNCLAMPED_FLOAT_TO_UBYTE(v[0].rgba.alpha, col[0][3]);
	    STRIDE_4F(col, col_stride);
	    v++;
	 }
	 if (DO_SPEC_OR_FOG) {
	    if (DO_SPEC) {
	       UNCLAMPED_FLOAT_TO_UBYTE(v[0].rgba.red, spec[0][0]);
	       UNCLAMPED_FLOAT_TO_UBYTE(v[0].rgba.green, spec[0][1]);
	       UNCLAMPED_FLOAT_TO_UBYTE(v[0].rgba.blue, spec[0][2]);
	       STRIDE_4F(spec, spec_stride);
	    }
	    if (DO_FOG) {
	       UNCLAMPED_FLOAT_TO_UBYTE(v[0].rgba.alpha, radeonComputeFogBlendFactor(ctx, fog[0][0]));
	       STRIDE_4F(fog, fog_stride);
	    }
	    if (TCL_DEBUG) fprintf(stderr, "%x ", v[0].ui);
	    v++;
	 }
	 if (DO_TEX0) {
	    v[0].ui = tc0[0][0];
	    v[1].ui = tc0[0][1];
	    if (TCL_DEBUG) fprintf(stderr, "t0: %.2f %.2f ", v[0].f, v[1].f);
	    if (DO_PTEX) {
	       if (fill_tex & (1<<0))
		  v[2].f = 1.0;
	       else if (rqcoordsnoswap & (1<<0))
		  v[2].ui = tc0[0][2];
	       else
		  v[2].ui = tc0[0][3];
	       if (TCL_DEBUG) fprintf(stderr, "%.2f ", v[2].f);
	       v += 3;
	    } 
	    else
	       v += 2;
	    tc0 =  (GLuint (*)[4])((GLubyte *)tc0 +  tc0_stride);
	 }
	 if (DO_TEX1) {
	    v[0].ui = tc1[0][0];
	    v[1].ui = tc1[0][1];
	    if (TCL_DEBUG) fprintf(stderr, "t1: %.2f %.2f ", v[0].f, v[1].f);
	    if (DO_PTEX) {
	       if (fill_tex & (1<<1))
		  v[2].f = 1.0;
	       else if (rqcoordsnoswap & (1<<1))
		  v[2].ui = tc1[0][2];
	       else
		  v[2].ui = tc1[0][3];
	       if (TCL_DEBUG) fprintf(stderr, "%.2f ", v[2].f);
	       v += 3;
	    } 
	    else
	       v += 2;
	    tc1 =  (GLuint (*)[4])((GLubyte *)tc1 +  tc1_stride);
	 } 
	 if (DO_TEX2) {
	    v[0].ui = tc2[0][0];
	    v[1].ui = tc2[0][1];
	    if (TCL_DEBUG) fprintf(stderr, "t2: %.2f %.2f ", v[0].f, v[1].f);
	    if (DO_PTEX) {
	       if (fill_tex & (1<<2))
		  v[2].f = 1.0;
	       else if (rqcoordsnoswap & (1<<2))
		  v[2].ui = tc2[0][2];
	       else
		  v[2].ui = tc2[0][3];
	       if (TCL_DEBUG) fprintf(stderr, "%.2f ", v[2].f);
	       v += 3;
	    } 
	    else
	       v += 2;
	    tc2 =  (GLuint (*)[4])((GLubyte *)tc2 +  tc2_stride);
	 } 
	 if (TCL_DEBUG) fprintf(stderr, "\n");
      }
   }
}



static void TAG(init)( void )
{
   int sz = 3;
   if (DO_W) sz++;
   if (DO_NORM) sz += 3;
   if (DO_RGBA) sz++;
   if (DO_SPEC_OR_FOG) sz++;
   if (DO_TEX0) sz += 2;
   if (DO_TEX0 && DO_PTEX) sz++;
   if (DO_TEX1) sz += 2;
   if (DO_TEX1 && DO_PTEX) sz++;
   if (DO_TEX2) sz += 2;
   if (DO_TEX2 && DO_PTEX) sz++;

   setup_tab[IDX].emit = TAG(emit);
   setup_tab[IDX].vertex_format = IND;
   setup_tab[IDX].vertex_size = sz;
}


#undef IND
#undef TAG
#undef IDX
