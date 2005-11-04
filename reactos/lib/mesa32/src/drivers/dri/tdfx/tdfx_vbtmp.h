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
 */

/* Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 *    Daniel Borca <dborca@users.sourceforge.net>
 */


#define VIEWPORT_X(dst,x) dst = s[0]  * x + s[12]
#define VIEWPORT_Y(dst,y) dst = s[5]  * y + s[13]
#define VIEWPORT_Z(dst,z) dst = s[10] * z + s[14]


static void TAG(emit)( GLcontext *ctx,
		       GLuint start, GLuint end,
		       void *dest )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLuint tmu0_source = fxMesa->tmu_source[0];
   GLuint tmu1_source = fxMesa->tmu_source[1];
   GLfloat (*tc0)[4], (*tc1)[4];
   GLfloat (*col)[4];
   GLuint tc0_stride, tc1_stride, col_stride;
   GLuint tc0_size, tc1_size, col_size;
   GLfloat (*proj)[4] = VB->NdcPtr->data; 
   GLuint proj_stride = VB->NdcPtr->stride;
   GLfloat (*fog)[4];
   GLuint fog_stride;
   tdfxVertex *v = (tdfxVertex *)dest;
   GLfloat u0scale,v0scale,u1scale,v1scale;
   const GLubyte *mask = VB->ClipMask;
   const GLfloat *s = fxMesa->hw_viewport;
   int i;

/*     fprintf(stderr, "%s\n", __FUNCTION__); */

   if (IND & TDFX_TEX0_BIT) {
      tc0_stride = VB->TexCoordPtr[tmu0_source]->stride;
      tc0 = VB->TexCoordPtr[tmu0_source]->data;
      u0scale = fxMesa->sScale0;
      v0scale = fxMesa->tScale0;
      if (IND & TDFX_PTEX_BIT)
	 tc0_size = VB->TexCoordPtr[tmu0_source]->size;
   }

   if (IND & TDFX_TEX1_BIT) {
      tc1 = VB->TexCoordPtr[tmu1_source]->data;
      tc1_stride = VB->TexCoordPtr[tmu1_source]->stride;
      u1scale = fxMesa->sScale1;
      v1scale = fxMesa->tScale1;
      if (IND & TDFX_PTEX_BIT)
	 tc1_size = VB->TexCoordPtr[tmu1_source]->size;
   }
   
   if (IND & TDFX_RGBA_BIT) {
      col = VB->ColorPtr[0]->data;
      col_stride = VB->ColorPtr[0]->stride;
      col_size = VB->ColorPtr[0]->size;
   }
   
   if (IND & TDFX_FOGC_BIT) {
      fog = VB->FogCoordPtr->data;
      fog_stride = VB->FogCoordPtr->stride;
   }

   {
      /* May have nonstandard strides:
       */
      if (start) {
	 proj =  (GLfloat (*)[4])((GLubyte *)proj + start * proj_stride);
	 if (IND & TDFX_TEX0_BIT)
	    tc0 =  (GLfloat (*)[4])((GLubyte *)tc0 + start * tc0_stride);
	 if (IND & TDFX_TEX1_BIT) 
	    tc1 =  (GLfloat (*)[4])((GLubyte *)tc1 + start * tc1_stride);
	 if (IND & TDFX_RGBA_BIT) 
	    STRIDE_4F(col, start * col_stride);
	 if (IND & TDFX_FOGC_BIT) 
	    STRIDE_4F(fog, start * fog_stride);
      }

      for (i=start; i < end; i++, v++) {
	 if (IND & TDFX_XYZ_BIT) {
	    if (mask[i] == 0) {
               /* unclipped */
	       VIEWPORT_X(v->x, proj[0][0]);
	       VIEWPORT_Y(v->y, proj[0][1]);
	       VIEWPORT_Z(v->z, proj[0][2]);
	       v->rhw = proj[0][3];	
	    } else {
               /* clipped */
               v->rhw = 1.0;
	    }
	    proj =  (GLfloat (*)[4])((GLubyte *)proj +  proj_stride);
	 }
	 if (IND & TDFX_RGBA_BIT) {
	    UNCLAMPED_FLOAT_TO_UBYTE(v->color[0], col[0][2]);
	    UNCLAMPED_FLOAT_TO_UBYTE(v->color[1], col[0][1]);
	    UNCLAMPED_FLOAT_TO_UBYTE(v->color[2], col[0][0]);
	    if (col_size == 4) {
	       UNCLAMPED_FLOAT_TO_UBYTE(v->color[3], col[0][3]);
	    } else {
	       v->color[3] = 255;
	    }
	    STRIDE_4F(col, col_stride);
	 }
	 if (IND & TDFX_FOGC_BIT) {
	    v->fog = CLAMP(fog[0][0], 0.0f, 1.0f);
	    STRIDE_4F(fog, fog_stride);
	 }
	 if (IND & TDFX_TEX0_BIT) {
	    GLfloat w = v->rhw;
	    v->tu0 = tc0[0][0] * u0scale * w;
	    v->tv0 = tc0[0][1] * v0scale * w;
	    if (IND & TDFX_PTEX_BIT) {
	       v->tq0 = w;
	       if (tc0_size == 4) 
		  v->tq0 = tc0[0][3] * w;
	    } 
	    tc0 =  (GLfloat (*)[4])((GLubyte *)tc0 +  tc0_stride);
	 }
	 if (IND & TDFX_TEX1_BIT) {
	    GLfloat w = v->rhw;
	    v->tu1 = tc1[0][0] * u1scale * w;
	    v->tv1 = tc1[0][1] * v1scale * w;
	    if (IND & TDFX_PTEX_BIT) {
	       v->tq1 = w;
	       if (tc1_size == 4) 
		  v->tq1 = tc1[0][3] * w;
	    }
	    tc1 =  (GLfloat (*)[4])((GLubyte *)tc1 +  tc1_stride);
	 } 
      }
   }
}


static GLboolean TAG(check_tex_sizes)( GLcontext *ctx )
{
/*     fprintf(stderr, "%s\n", __FUNCTION__); */

   if (IND & TDFX_PTEX_BIT)
      return GL_TRUE;
   
   if (IND & TDFX_TEX0_BIT) {
      struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

      if (IND & TDFX_TEX1_BIT) {
	 if (VB->TexCoordPtr[0] == 0)
	    VB->TexCoordPtr[0] = VB->TexCoordPtr[1];
	 
	 if (VB->TexCoordPtr[1]->size == 4)
	    return GL_FALSE;
      }

      if (VB->TexCoordPtr[0]->size == 4)
	 return GL_FALSE;
   }

   return GL_TRUE;
}


static void TAG(interp)( GLcontext *ctx,
			 GLfloat t, 
			 GLuint edst, GLuint eout, GLuint ein,
			 GLboolean force_boundary )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT( ctx );
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   const GLfloat *dstclip = VB->ClipPtr->data[edst];
   const GLfloat oow = (dstclip[3] == 0.0F) ? 1.0F : (1.0F / dstclip[3]);
   const GLfloat *s = fxMesa->hw_viewport;
   tdfxVertex *dst = fxMesa->verts + edst;
   const tdfxVertex *out = fxMesa->verts + eout;
   const tdfxVertex *in = fxMesa->verts + ein;
   const GLfloat wout = oow / out->rhw;
   const GLfloat win = oow / in->rhw;

   VIEWPORT_X(dst->x, dstclip[0] * oow);
   VIEWPORT_Y(dst->y, dstclip[1] * oow);
   VIEWPORT_Z(dst->z, dstclip[2] * oow);
   dst->rhw = oow;

   INTERP_UB( t, dst->color[0], out->color[0],   in->color[0] );
   INTERP_UB( t, dst->color[1], out->color[1],   in->color[1] );
   INTERP_UB( t, dst->color[2], out->color[2],   in->color[2] );
   INTERP_UB( t, dst->color[3], out->color[3],   in->color[3] );

   if (IND & TDFX_FOGC_BIT) {
      INTERP_F( t, dst->fog, out->fog, in->fog );
   }

   if (IND & TDFX_TEX0_BIT) {
      INTERP_F( t, dst->tu0, out->tu0 * wout, in->tu0 * win );
      INTERP_F( t, dst->tv0, out->tv0 * wout, in->tv0 * win );
      if (IND & TDFX_PTEX_BIT) {
         INTERP_F( t, dst->tq0, out->tq0 * wout, in->tq0 * win );
      }
   }
   if (IND & TDFX_TEX1_BIT) {
     INTERP_F( t, dst->tu1, out->tu1 * wout, in->tu1 * win );
     INTERP_F( t, dst->tv1, out->tv1 * wout, in->tv1 * win );
     if (IND & TDFX_PTEX_BIT) {
        INTERP_F( t, dst->tq1, out->tq1 * wout, in->tq1 * win );
     }
   }
}


static void TAG(init)( void )
{
/*     fprintf(stderr, "%s\n", __FUNCTION__); */

   setup_tab[IND].emit = TAG(emit);
   setup_tab[IND].check_tex_sizes = TAG(check_tex_sizes);
   setup_tab[IND].interp = TAG(interp);
   setup_tab[IND].copy_pv = copy_pv;

   if (IND & TDFX_TEX1_BIT) {
      if (IND & TDFX_PTEX_BIT) {
	 setup_tab[IND].vertex_format = TDFX_LAYOUT_PROJ2;
      }
      else {
	 setup_tab[IND].vertex_format = TDFX_LAYOUT_MULTI;
      }
   } 
   else if (IND & TDFX_TEX0_BIT) {
      if (IND & TDFX_PTEX_BIT) {
	 setup_tab[IND].vertex_format = TDFX_LAYOUT_PROJ1;
      } else {
	 setup_tab[IND].vertex_format = TDFX_LAYOUT_SINGLE;
      }
   }
   else if (IND & TDFX_W_BIT) {
      setup_tab[IND].vertex_format = TDFX_LAYOUT_NOTEX;
   } else {
      setup_tab[IND].vertex_format = TDFX_LAYOUT_TINY;
   }
}


#undef IND
#undef TAG
