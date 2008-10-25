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
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLuint tmu0_source = fxMesa->tmu_source[0];
   GLuint tmu1_source = fxMesa->tmu_source[1];
   GLfloat (*tc0)[4], (*tc1)[4];
   GLfloat (*col)[4], (*spec)[4];
   GLuint tc0_stride, tc1_stride, col_stride, spec_stride;
   GLuint tc0_size, tc1_size, col_size;
   GLfloat (*proj)[4] = VB->NdcPtr->data; 
   GLuint proj_stride = VB->NdcPtr->stride;
   GLfloat (*psize)[4];
   GLuint psize_stride;
   GLfloat (*fog)[4];
   GLuint fog_stride;
   GrVertex *v = (GrVertex *)dest;
   GLfloat u0scale,v0scale,u1scale,v1scale;
   const GLubyte *mask = VB->ClipMask;
   const GLfloat *const s = ctx->Viewport._WindowMap.m;
   int i;

   if (IND & SETUP_PSIZ) {
      psize = VB->PointSizePtr->data;
      psize_stride = VB->PointSizePtr->stride;
   }

   if (IND & SETUP_TMU0) {
      tc0 = VB->TexCoordPtr[tmu0_source]->data;
      tc0_stride = VB->TexCoordPtr[tmu0_source]->stride;
      u0scale = fxMesa->s0scale;
      v0scale = fxMesa->t0scale;
      if (IND & SETUP_PTEX)
	 tc0_size = VB->TexCoordPtr[tmu0_source]->size;
   }

   if (IND & SETUP_TMU1) {
      tc1 = VB->TexCoordPtr[tmu1_source]->data;
      tc1_stride = VB->TexCoordPtr[tmu1_source]->stride;
      u1scale = fxMesa->s1scale; /* wrong if tmu1_source == 0, possible? */
      v1scale = fxMesa->t1scale;
      if (IND & SETUP_PTEX)
	 tc1_size = VB->TexCoordPtr[tmu1_source]->size;
   }
   
   if (IND & SETUP_RGBA) {
      col = VB->ColorPtr[0]->data;
      col_stride = VB->ColorPtr[0]->stride;
      col_size = VB->ColorPtr[0]->size;
   }

   if (IND & SETUP_SPEC) {
      spec = VB->SecondaryColorPtr[0]->data;
      spec_stride = VB->SecondaryColorPtr[0]->stride;
   }

   if (IND & SETUP_FOGC) {
      fog = VB->FogCoordPtr->data;
      fog_stride = VB->FogCoordPtr->stride;
   }

   if (start) {
      proj =  (GLfloat (*)[4])((GLubyte *)proj + start * proj_stride);
      if (IND & SETUP_PSIZ)
         psize =  (GLfloat (*)[4])((GLubyte *)psize + start * psize_stride);
      if (IND & SETUP_TMU0)
	 tc0 =  (GLfloat (*)[4])((GLubyte *)tc0 + start * tc0_stride);
      if (IND & SETUP_TMU1) 
	 tc1 =  (GLfloat (*)[4])((GLubyte *)tc1 + start * tc1_stride);
      if (IND & SETUP_RGBA) 
	 STRIDE_4F(col, start * col_stride);
      if (IND & SETUP_SPEC)
	 STRIDE_4F(spec, start * spec_stride);
      if (IND & SETUP_FOGC)
	 fog =  (GLfloat (*)[4])((GLubyte *)fog + start * fog_stride);
   }

   for (i=start; i < end; i++, v++) {
      if (IND & SETUP_PSIZ) {
         v->psize = psize[0][0];
         psize =  (GLfloat (*)[4])((GLubyte *)psize +  psize_stride);
      }
   
      if (IND & SETUP_XYZW) {
         if (mask[i] == 0) {
	    /* unclipped */
	    VIEWPORT_X(v->x,   proj[0][0]);
	    VIEWPORT_Y(v->y,   proj[0][1]);
	    VIEWPORT_Z(v->ooz, proj[0][2]);
	    v->oow = proj[0][3];

	    if (IND & SETUP_SNAP) {
#if defined(USE_IEEE)
	       const float snapper = (3L << 18);
	       v->x += snapper;
	       v->x -= snapper;
	       v->y += snapper;
	       v->y -= snapper;
#else
	       v->x = ((int) (v->x * 16.0f)) * (1.0f / 16.0f);
	       v->y = ((int) (v->y * 16.0f)) * (1.0f / 16.0f);
#endif
	    }
         } else {
            /* clipped */
            v->oow = 1.0;
         }

	 proj =  (GLfloat (*)[4])((GLubyte *)proj +  proj_stride);
      }
      if (IND & SETUP_RGBA) {
#if FX_PACKEDCOLOR
         UNCLAMPED_FLOAT_TO_UBYTE(v->pargb[2], col[0][0]);
         UNCLAMPED_FLOAT_TO_UBYTE(v->pargb[1], col[0][1]);
         UNCLAMPED_FLOAT_TO_UBYTE(v->pargb[0], col[0][2]);
         if (col_size == 4) {
            UNCLAMPED_FLOAT_TO_UBYTE(v->pargb[3], col[0][3]);
         } else {
            v->pargb[3] = 255;
         }
#else  /* !FX_PACKEDCOLOR */
         CNORM(v->r, col[0][0]);
         CNORM(v->g, col[0][1]);
         CNORM(v->b, col[0][2]);
         if (col_size == 4) {
            CNORM(v->a, col[0][3]);
         } else {
            v->a = 255.0f;
         }
#endif /* !FX_PACKEDCOLOR */
	 STRIDE_4F(col, col_stride);
      }
      if (IND & SETUP_SPEC) {
#if FX_PACKEDCOLOR
	 UNCLAMPED_FLOAT_TO_UBYTE(v->pspec[2], spec[0][0]);
	 UNCLAMPED_FLOAT_TO_UBYTE(v->pspec[1], spec[0][1]);
	 UNCLAMPED_FLOAT_TO_UBYTE(v->pspec[0], spec[0][2]);
#else  /* !FX_PACKEDCOLOR */
         CNORM(v->r1, spec[0][0]);
         CNORM(v->g1, spec[0][1]);
         CNORM(v->b1, spec[0][2]);
#endif /* !FX_PACKEDCOLOR */
	 STRIDE_4F(spec, spec_stride);
      }
      if (IND & SETUP_FOGC) {
         v->fog = CLAMP(fog[0][0], 0.0f, 1.0f);
	 fog =  (GLfloat (*)[4])((GLubyte *)fog + fog_stride);
      }
      if (IND & SETUP_TMU0) {
	 GLfloat w = v->oow;
         v->tmuvtx[0].sow = tc0[0][0] * u0scale * w;
         v->tmuvtx[0].tow = tc0[0][1] * v0scale * w;
	 if (IND & SETUP_PTEX) {
	    v->tmuvtx[0].oow = w;
	    if (tc0_size == 4) 
	       v->tmuvtx[0].oow *= tc0[0][3];
	 } 
	 tc0 =  (GLfloat (*)[4])((GLubyte *)tc0 +  tc0_stride);
      }
      if (IND & SETUP_TMU1) {
	 GLfloat w = v->oow;
         v->tmuvtx[1].sow = tc1[0][0] * u1scale * w;
         v->tmuvtx[1].tow = tc1[0][1] * v1scale * w;
	 if (IND & SETUP_PTEX) {
	    v->tmuvtx[1].oow = w;
	    if (tc1_size == 4) 
	       v->tmuvtx[1].oow *= tc1[0][3];
	 } 
	 tc1 =  (GLfloat (*)[4])((GLubyte *)tc1 +  tc1_stride);
      } 
   }
}

#if (IND & SETUP_XYZW) && (IND & SETUP_RGBA)

static GLboolean TAG(check_tex_sizes)( GLcontext *ctx )
{
/*     fprintf(stderr, "%s\n", __FUNCTION__); */

   if (IND & SETUP_PTEX)
      return GL_TRUE;
   
   if (IND & SETUP_TMU0) {
      struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

      if (IND & SETUP_TMU1) {
	 if (VB->TexCoordPtr[0] == 0)
	    VB->TexCoordPtr[0] = VB->TexCoordPtr[1];
	 
	 if (VB->TexCoordPtr[1]->size == 4)
	    return GL_FALSE;
      }

      if (VB->TexCoordPtr[0] && VB->TexCoordPtr[0]->size == 4)
	 return GL_FALSE;
   }

   return GL_TRUE;
}

static void TAG(interp)( GLcontext *ctx,
			 GLfloat t, 
			 GLuint edst, GLuint eout, GLuint ein,
			 GLboolean force_boundary )
{
   fxMesaContext fxMesa = FX_CONTEXT( ctx );
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   const GLfloat *dstclip = VB->ClipPtr->data[edst];
   const GLfloat oow = (dstclip[3] == 0.0F) ? 1.0F : (1.0F / dstclip[3]);
   const GLfloat *const s = ctx->Viewport._WindowMap.m;
   GrVertex *fxverts = fxMesa->verts;
   GrVertex *dst = (GrVertex *) (fxverts + edst);
   const GrVertex *out = (const GrVertex *) (fxverts + eout);
   const GrVertex *in = (const GrVertex *) (fxverts + ein);
   const GLfloat wout = oow / out->oow;
   const GLfloat win = oow / in->oow;

   VIEWPORT_X(dst->x,   dstclip[0] * oow);
   VIEWPORT_Y(dst->y,   dstclip[1] * oow);
   VIEWPORT_Z(dst->ooz, dstclip[2] * oow);
   dst->oow = oow;	
   
   if (IND & SETUP_SNAP) {
#if defined(USE_IEEE)
      const float snapper = (3L << 18);
      dst->x += snapper;
      dst->x -= snapper;
      dst->y += snapper;
      dst->y -= snapper;
#else
      dst->x = ((int) (dst->x * 16.0f)) * (1.0f / 16.0f);
      dst->y = ((int) (dst->y * 16.0f)) * (1.0f / 16.0f);
#endif
   }

   
#if FX_PACKEDCOLOR
   INTERP_UB( t, dst->pargb[0], out->pargb[0], in->pargb[0] );
   INTERP_UB( t, dst->pargb[1], out->pargb[1], in->pargb[1] );
   INTERP_UB( t, dst->pargb[2], out->pargb[2], in->pargb[2] );
   INTERP_UB( t, dst->pargb[3], out->pargb[3], in->pargb[3] );
#else  /* !FX_PACKEDCOLOR */
   INTERP_F( t, dst->r, out->r, in->r );
   INTERP_F( t, dst->g, out->g, in->g );
   INTERP_F( t, dst->b, out->b, in->b );
   INTERP_F( t, dst->a, out->a, in->a );
#endif /* !FX_PACKEDCOLOR */

   if (IND & SETUP_SPEC) {
#if FX_PACKEDCOLOR
      INTERP_UB( t, dst->pspec[0], out->pspec[0], in->pspec[0] );
      INTERP_UB( t, dst->pspec[1], out->pspec[1], in->pspec[1] );
      INTERP_UB( t, dst->pspec[2], out->pspec[2], in->pspec[2] );
#else  /* !FX_PACKEDCOLOR */
      INTERP_F( t, dst->r1, out->r1, in->r1 );
      INTERP_F( t, dst->g1, out->g1, in->g1 );
      INTERP_F( t, dst->b1, out->b1, in->b1 );
#endif /* !FX_PACKEDCOLOR */
   }

   if (IND & SETUP_FOGC) {
      INTERP_F( t, dst->fog, out->fog, in->fog );
   }

   if (IND & SETUP_TMU0) {
      INTERP_F( t,
		dst->tmuvtx[0].sow,
		out->tmuvtx[0].sow * wout,
		in->tmuvtx[0].sow * win );
      INTERP_F( t,
		dst->tmuvtx[0].tow,
		out->tmuvtx[0].tow * wout,
		in->tmuvtx[0].tow * win );
      if (IND & SETUP_PTEX) {
	 INTERP_F( t,
		   dst->tmuvtx[0].oow, 
		   out->tmuvtx[0].oow * wout, 
		   in->tmuvtx[0].oow * win );
      }
   }

   if (IND & SETUP_TMU1) {
      INTERP_F( t,
		dst->tmuvtx[1].sow,
		out->tmuvtx[1].sow * wout,
		in->tmuvtx[1].sow * win );
      INTERP_F( t,
		dst->tmuvtx[1].tow,
		out->tmuvtx[1].tow * wout,
		in->tmuvtx[1].tow * win );
      if (IND & SETUP_PTEX) {
	 INTERP_F( t,
		   dst->tmuvtx[1].oow, 
		   out->tmuvtx[1].oow * wout, 
		   in->tmuvtx[1].oow * win );
      }
   }
}
#endif


static void TAG(init)( void )
{
   setup_tab[IND].emit = TAG(emit);
   
   if (IND & SETUP_SPEC) {
      setup_tab[IND].copy_pv = copy_pv2;
   } else {
      setup_tab[IND].copy_pv = copy_pv;
   }

#if ((IND & SETUP_XYZW) && (IND & SETUP_RGBA))
   setup_tab[IND].check_tex_sizes = TAG(check_tex_sizes);
   setup_tab[IND].interp = TAG(interp);

   setup_tab[IND].vertex_format = 0;
   if (IND & SETUP_PTEX) {
      setup_tab[IND].vertex_format |= GR_STWHINT_W_DIFF_TMU0;
   }

#if (IND & SETUP_TMU1)
   setup_tab[IND].vertex_format |= GR_STWHINT_ST_DIFF_TMU1;
   if (IND & SETUP_PTEX) {
      setup_tab[IND].vertex_format |= GR_STWHINT_W_DIFF_TMU1;
   }
#endif

#endif
}


#undef IND
#undef TAG
