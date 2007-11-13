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


/* DO_XYZW:  Emit xyz and maybe w coordinates.
 * DO_RGBA:  Emit color.
 * DO_SPEC:  Emit specular color.
 * DO_FOG:   Emit fog coordinate in specular alpha.
 * DO_TEX0:  Emit tex0 u,v coordinates.
 * DO_TEX1:  Emit tex1 u,v coordinates.
 * DO_PTEX:  Emit tex0,1 q coordinates where possible.
 *
 * Additionally, this template assumes it is emitting *transformed*
 * vertices; the modifications to emit untransformed vertices (ie. to
 * t&l hardware) are probably too great to cooexist with the code
 * already in this file.
 */

#define VIEWPORT_X(x)  ((GLint) ((s[0]  * (x) + s[12]) * 4.0))
#define VIEWPORT_Y(y)  ((GLint) ((s[5]  * (y) + s[13]) * 4.0))
#define VIEWPORT_Z(z) (((GLuint) (s[10] * (z) + s[14])) << 15)

#ifndef LOCALVARS
#define LOCALVARS
#endif

static void TAG(emit)( GLcontext *ctx,
		       GLuint start, GLuint end,
		       void *dest,
		       GLuint stride )
{
   LOCALVARS
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
#if DO_TEX1
   GLfloat (*tc1)[4];
   GLuint tc1_stride;
#if DO_PTEX
   GLuint tc1_size;
#endif
#endif
#if DO_TEX0
   GLfloat (*tc0)[4];
   GLuint tc0_stride;
#if DO_PTEX
   GLuint tc0_size;
#endif
#endif
#if DO_SPEC
   GLfloat (*spec)[4];
   GLuint spec_stride;
#endif
#if DO_FOG
   GLfloat (*fog)[4];
   GLuint fog_stride;
#endif
#if DO_RGBA
   GLfloat (*col)[4];
   GLuint col_stride;
#endif
   GLfloat (*coord)[4];
   GLuint coord_stride;
   VERTEX *v = (VERTEX *)dest;
   const GLfloat *s = GET_VIEWPORT_MAT();
#if DO_TEX1 || DO_TEX0 || DO_XYZW
   const GLubyte *mask = VB->ClipMask;
#endif
   int i;

#if !DO_XYZW
   (void) s; /* Quiet compiler */
#endif
/*     fprintf(stderr, "%s(big) importable %d %d..%d\n",  */
/*  	   __FUNCTION__, VB->importable_data, start, end); */

#if DO_TEX1
   {
      const GLuint t1 = GET_TEXSOURCE(1);
      tc1 = VB->TexCoordPtr[t1]->data;
      tc1_stride = VB->TexCoordPtr[t1]->stride;
#if DO_PTEX
      tc1_size = VB->TexCoordPtr[t1]->size;
#endif
   }
#endif

#if DO_TEX0
   {
      const GLuint t0 = GET_TEXSOURCE(0);
      tc0 = VB->TexCoordPtr[t0]->data;
      tc0_stride = VB->TexCoordPtr[t0]->stride;
#if DO_PTEX
      tc0_size = VB->TexCoordPtr[t0]->size;
#endif
   }
#endif

#if DO_SPEC
   if (VB->SecondaryColorPtr[0]) {
      spec = VB->SecondaryColorPtr[0]->data;
      spec_stride = VB->SecondaryColorPtr[0]->stride;
   } else {
      spec = (GLfloat (*)[4])ctx->Current.Attrib[VERT_ATTRIB_COLOR1];
      spec_stride = 0;
   }
#endif

#if DO_FOG
   if (VB->FogCoordPtr) {
      fog = VB->FogCoordPtr->data;
      fog_stride = VB->FogCoordPtr->stride;
   } else {
      static GLfloat tmp[4] = {0, 0, 0, 0};
      fog = &tmp;
      fog_stride = 0;
   }
#endif

#if DO_RGBA
   col = VB->ColorPtr[0]->data;
   col_stride = VB->ColorPtr[0]->stride;
#endif

   coord = VB->NdcPtr->data;
   coord_stride = VB->NdcPtr->stride;

   if (start) {
#if DO_TEX1
         STRIDE_4F(tc1, start * tc1_stride);
#endif
#if DO_TEX0
         STRIDE_4F(tc0, start * tc0_stride);
#endif
#if DO_SPEC
	 STRIDE_4F(spec, start * spec_stride);
#endif
#if DO_FOG
	 STRIDE_4F(fog, start * fog_stride);
#endif
#if DO_RGBA
	 STRIDE_4F(col, start * col_stride);
#endif
	 STRIDE_4F(coord, start * coord_stride);
   }

   for (i=start; i < end; i++, v = (VERTEX *)((GLubyte *)v + stride)) {
	 CARD32 *p = (CARD32 *)v;
#if DO_TEX1 || DO_TEX0
	 GLfloat w;

	 if (mask[i] == 0) {
	    /* unclipped */
	    w = coord[0][3];
	 } else {
	    /* clipped */
	    w = 1.0;
	 }
#endif
	 
#if DO_TEX1
#if DO_PTEX
	 if (tc1_size == 4) {
#ifdef MACH64_PREMULT_TEXCOORDS
	    LE32_OUT_FLOAT( p++, w*tc1[0][0] );		/* VERTEX_?_SECONDARY_S */
	    LE32_OUT_FLOAT( p++, w*tc1[0][1] );		/* VERTEX_?_SECONDARY_T */
	    LE32_OUT_FLOAT( p++, w*tc1[0][3] );		/* VERTEX_?_SECONDARY_W */
#else /* !MACH64_PREMULT_TEXCOORDS */
	    float rhw = 1.0 / tc1[0][3];
	    LE32_OUT_FLOAT( p++, rhw*tc1[0][0] );	/* VERTEX_?_SECONDARY_S */
	    LE32_OUT_FLOAT( p++, rhw*tc1[0][1] );	/* VERTEX_?_SECONDARY_T */
	    LE32_OUT_FLOAT( p++, w*tc1[0][3] );		/* VERTEX_?_SECONDARY_W */	
#endif /* !MACH64_PREMULT_TEXCOORDS */
	 } else {
#endif /* DO_PTEX */
#ifdef MACH64_PREMULT_TEXCOORDS
	    LE32_OUT_FLOAT( p++, w*tc1[0][0] );		/* VERTEX_?_SECONDARY_S */
	    LE32_OUT_FLOAT( p++, w*tc1[0][1] );		/* VERTEX_?_SECONDARY_T */
	    LE32_OUT_FLOAT( p++, w );			/* VERTEX_?_SECONDARY_W */
#else /* !MACH64_PREMULT_TEXCOORDS */
	    LE32_OUT_FLOAT( p++, tc1[0][0] );		/* VERTEX_?_SECONDARY_S */
	    LE32_OUT_FLOAT( p++, tc1[0][1] );		/* VERTEX_?_SECONDARY_T */
	    LE32_OUT_FLOAT( p++, w );			/* VERTEX_?_SECONDARY_W */
#endif /* !MACH64_PREMULT_TEXCOORDS */
#if DO_PTEX
	 }
#endif /* DO_PTEX */
	 STRIDE_4F(tc1, tc1_stride);
#else /* !DO_TEX1 */
	 p += 3;
#endif /* !DO_TEX1 */
	    
#if DO_TEX0
#if DO_PTEX
	 if (tc0_size == 4) {
#ifdef MACH64_PREMULT_TEXCOORDS
	    LE32_OUT_FLOAT( p++, w*tc0[0][0] );			/* VERTEX_?_S */
	    LE32_OUT_FLOAT( p++, w*tc0[0][1] );			/* VERTEX_?_T */
	    LE32_OUT_FLOAT( p++, w*tc0[0][3] );			/* VERTEX_?_W */
#else /* !MACH64_PREMULT_TEXCOORDS */
	    float rhw = 1.0 / tc0[0][3];
	    LE32_OUT_FLOAT( p++, rhw*tc0[0][0] );		/* VERTEX_?_S */
	    LE32_OUT_FLOAT( p++, rhw*tc0[0][1] );		/* VERTEX_?_T */
	    LE32_OUT_FLOAT( p++, w*tc0[0][3] );			/* VERTEX_?_W */	
#endif /* !MACH64_PREMULT_TEXCOORDS */
	 } else {
#endif /* DO_PTEX */
#ifdef MACH64_PREMULT_TEXCOORDS
	    LE32_OUT_FLOAT( p++, w*tc0[0][0] );			/* VERTEX_?_S */
	    LE32_OUT_FLOAT( p++, w*tc0[0][1] );			/* VERTEX_?_T */
	    LE32_OUT_FLOAT( p++, w );				/* VERTEX_?_W */
#else /* !MACH64_PREMULT_TEXCOORDS */
	    LE32_OUT_FLOAT( p++, tc0[0][0] );			/* VERTEX_?_S */
	    LE32_OUT_FLOAT( p++, tc0[0][1] );			/* VERTEX_?_T */
	    LE32_OUT_FLOAT( p++, w );				/* VERTEX_?_W */
#endif /* !MACH64_PREMULT_TEXCOORDS */
#if DO_PTEX
	 }
#endif /* DO_PTEX */
	 STRIDE_4F(tc0, tc0_stride);
#else /* !DO_TEX0 */
	 p += 3;
#endif /* !DO_TEX0 */

#if DO_SPEC
	 UNCLAMPED_FLOAT_TO_UBYTE(((GLubyte *)p)[0],  spec[0][2]); 	/* VERTEX_?_SPEC_B */
	 UNCLAMPED_FLOAT_TO_UBYTE(((GLubyte *)p)[1],  spec[0][1]);	/* VERTEX_?_SPEC_G */
	 UNCLAMPED_FLOAT_TO_UBYTE(((GLubyte *)p)[2],  spec[0][0]);	/* VERTEX_?_SPEC_R */

	 STRIDE_4F(spec, spec_stride);
#endif
#if DO_FOG
	 UNCLAMPED_FLOAT_TO_UBYTE(((GLubyte *)p)[3], fog[0][0]);  /* VERTEX_?_SPEC_A */
	 /*	 ((GLubyte *)p)[3] = fog[0][0] * 255.0;	 */
	 STRIDE_4F(fog, fog_stride);
#endif
	 p++;
	    
#if DO_XYZW
	 if (mask[i] == 0) {
	    /* unclipped */
	    LE32_OUT( p++, VIEWPORT_Z( coord[0][2] ) );	/* VERTEX_?_Z */
	 } else {
#endif
	    p++;
#if DO_XYZW
	 }
#endif

#if DO_RGBA
	 UNCLAMPED_FLOAT_TO_UBYTE(((GLubyte *)p)[0], col[0][2]);
	 UNCLAMPED_FLOAT_TO_UBYTE(((GLubyte *)p)[1], col[0][1]);
	 UNCLAMPED_FLOAT_TO_UBYTE(((GLubyte *)p)[2], col[0][0]);
	 UNCLAMPED_FLOAT_TO_UBYTE(((GLubyte *)p)[3], col[0][3]);
	 p++;
	 STRIDE_4F(col, col_stride);
#else
	 p++;
#endif

#if DO_XYZW
	 if (mask[i] == 0) {
	    /* unclipped */
	    LE32_OUT( p,
		      (VIEWPORT_X( coord[0][0] ) << 16) |	/* VERTEX_?_X */
		      (VIEWPORT_Y( coord[0][1] ) & 0xffff) );	/* VERTEX_?_Y */
	    
	    if (MACH64_DEBUG & DEBUG_VERBOSE_PRIMS) {
	       fprintf( stderr, "%s: vert %d: %.2f %.2f %.2f %x\n",
			__FUNCTION__,
			i,
			(LE32_IN( p ) >> 16)/4.0,
			(LE32_IN( p ) & 0xffff)/4.0,
			LE32_IN( p - 2 )/65536.0,
			*(GLuint *)(p - 1) );
	    }
	 }
#endif
#if DO_TEX1 || DO_TEX0 || DO_XYZW
	 STRIDE_4F(coord, coord_stride);
#endif
	 
	 assert( p + 1 - (CARD32 *)v == 10 );
      }
}

#if DO_XYZW && DO_RGBA

static GLboolean TAG(check_tex_sizes)( GLcontext *ctx )
{
   LOCALVARS
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   /* Force 'missing' texcoords to something valid.
    */
   if (DO_TEX1 && VB->TexCoordPtr[0] == 0)
      VB->TexCoordPtr[0] = VB->TexCoordPtr[1];

   if (DO_PTEX)
      return GL_TRUE;

   /* No hardware support for projective texture.  Can fake it for
    * TEX0 only.
    */
   if ((DO_TEX1 && VB->TexCoordPtr[GET_TEXSOURCE(1)]->size == 4)) {
      PTEX_FALLBACK();
      return GL_FALSE;
   }

   if (DO_TEX0 && VB->TexCoordPtr[GET_TEXSOURCE(0)]->size == 4) {
      if (DO_TEX1) {
	 PTEX_FALLBACK();
      }
      return GL_FALSE;
   }

   return GL_TRUE;
}


static void TAG(interp)( GLcontext *ctx,
			 GLfloat t,
			 GLuint edst, GLuint eout, GLuint ein,
			 GLboolean force_boundary )
{
   LOCALVARS
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLubyte *ddverts = GET_VERTEX_STORE();
   GLuint size = GET_VERTEX_SIZE();
   const GLfloat *dstclip = VB->ClipPtr->data[edst];
   GLfloat w;
   const GLfloat *s = GET_VIEWPORT_MAT();

   CARD32 *dst = (CARD32 *)(ddverts + (edst * size));
   CARD32 *in  = (CARD32 *)(ddverts + (ein  * size));
   CARD32 *out = (CARD32 *)(ddverts + (eout * size));

   (void)s;

   w = (dstclip[3] == 0.0F) ? 1.0 : (1.0 / dstclip[3]);

#if DO_TEX1
   {
      GLfloat temp;
#if DO_PTEX
      GLfloat wout = VB->NdcPtr->data[eout][3];
      GLfloat win = VB->NdcPtr->data[ein][3];
      GLfloat qout = LE32_IN_FLOAT( out + 2 ) / wout;
      GLfloat qin = LE32_IN_FLOAT( in + 2 ) / win;
      GLfloat qdst, rqdst;

      INTERP_F( t, qdst, qout, qin );
      rqdst = 1.0 / qdst;
      
      INTERP_F( t, temp, LE32_IN_FLOAT( out ) * qout, LE32_IN_FLOAT( in ) * qin );
      LE32_OUT_FLOAT( dst, temp*rqdst );			/* VERTEX_?_SECONDARY_S */
      dst++; out++; in++;
      
      INTERP_F( t, temp, LE32_IN_FLOAT( out ) * qout, LE32_IN_FLOAT( in ) * qin );
      LE32_OUT_FLOAT( dst, temp*rqdst );			/* VERTEX_?_SECONDARY_T */
      dst++; out++; in++;
      
      LE32_OUT_FLOAT( dst, w*rqdst );				/* VERTEX_?_SECONDARY_W */
      dst++; out++; in++;
#else /* !DO_PTEX */
#ifdef MACH64_PREMULT_TEXCOORDS
      GLfloat qout = w / LE32_IN_FLOAT( out + 2 );
      GLfloat qin = w / LE32_IN_FLOAT( in + 2 );
      
      INTERP_F( t, temp, LE32_IN_FLOAT( out ) * qout, LE32_IN_FLOAT( in ) * qin );
      LE32_OUT_FLOAT( dst, temp );				/* VERTEX_?_SECONDARY_S */
      dst++; out++; in++;
      
      INTERP_F( t, temp, LE32_IN_FLOAT( out ) * qout, LE32_IN_FLOAT( in ) * qin );
      LE32_OUT_FLOAT( dst, temp );				/* VERTEX_?_SECONDARY_T */
      dst++; out++; in++;
#else /* !MACH64_PREMULT_TEXCOORDS */
      INTERP_F( t, temp, LE32_IN_FLOAT( out ), LE32_IN_FLOAT( in ) );
      LE32_OUT_FLOAT( dst, temp );				/* VERTEX_?_SECONDARY_S */
      dst++; out++; in++;
      
      INTERP_F( t, temp, LE32_IN_FLOAT( out ), LE32_IN_FLOAT( in ) );
      LE32_OUT_FLOAT( dst, temp );				/* VERTEX_?_SECONDARY_T */
      dst++; out++; in++;
#endif /* !MACH64_PREMULT_TEXCOORDS */
      LE32_OUT_FLOAT( dst, w );					/* VERTEX_?_SECONDARY_W */
      dst++; out++; in++;
#endif /* !DO_PTEX */
   }
#else /* !DO_TEX1 */
   dst += 3; out += 3; in += 3;
#endif /* !DO_TEX1 */

#if DO_TEX0
   {
      GLfloat temp;
#if DO_PTEX
      GLfloat wout = VB->NdcPtr->data[eout][3];
      GLfloat win = VB->NdcPtr->data[ein][3];
      GLfloat qout = LE32_IN_FLOAT( out + 2 ) / wout;
      GLfloat qin = LE32_IN_FLOAT( in + 2 ) / win;
      GLfloat qdst, rqdst;

      INTERP_F( t, qdst, qout, qin );
      rqdst = 1.0 / qdst;
      
      INTERP_F( t, temp, LE32_IN_FLOAT( out ) * qout, LE32_IN_FLOAT( in ) * qin );
      LE32_OUT_FLOAT( dst, temp*rqdst );			/* VERTEX_?_S */
      dst++; out++; in++;
      
      INTERP_F( t, temp, LE32_IN_FLOAT( out ) * qout, LE32_IN_FLOAT( in ) * qin );
      LE32_OUT_FLOAT( dst, temp*rqdst );			/* VERTEX_?_T */
      dst++; out++; in++;
      
      LE32_OUT_FLOAT( dst, w*rqdst );				/* VERTEX_?_W */
      dst++; out++; in++;
#else /* !DO_PTEX */
#ifdef MACH64_PREMULT_TEXCOORDS
      GLfloat qout = w / LE32_IN_FLOAT( out + 2 );
      GLfloat qin = w / LE32_IN_FLOAT( in + 2 );
      
      INTERP_F( t, temp, LE32_IN_FLOAT( out ) * qout, LE32_IN_FLOAT( in ) * qin );
      LE32_OUT_FLOAT( dst, temp );				/* VERTEX_?_S */
      dst++; out++; in++;
      
      INTERP_F( t, temp, LE32_IN_FLOAT( out ) * qout, LE32_IN_FLOAT( in ) * qin );
      LE32_OUT_FLOAT( dst, temp );				/* VERTEX_?_T */
      dst++; out++; in++;
#else /* !MACH64_PREMULT_TEXCOORDS */
      INTERP_F( t, temp, LE32_IN_FLOAT( out ), LE32_IN_FLOAT( in ) );
      LE32_OUT_FLOAT( dst, temp );				/* VERTEX_?_S */
      dst++; out++; in++;
      
      INTERP_F( t, temp, LE32_IN_FLOAT( out ), LE32_IN_FLOAT( in ) );
      LE32_OUT_FLOAT( dst, temp );				/* VERTEX_?_T */
      dst++; out++; in++;
#endif /* !MACH64_PREMULT_TEXCOORDS */
      LE32_OUT_FLOAT( dst, w );					/* VERTEX_?_W */
      dst++; out++; in++;
#endif /* !DO_PTEX */
   }
#else /* !DO_TEX0 */
   dst += 3; out += 3; in += 3;
#endif /* !DO_TEX0 */
   
#if DO_SPEC
   INTERP_UB( t, ((GLubyte *)dst)[0], ((GLubyte *)out)[0], ((GLubyte *)in)[0] );	/* VERTEX_?_SPEC_B */
   INTERP_UB( t, ((GLubyte *)dst)[1], ((GLubyte *)out)[1], ((GLubyte *)in)[1] );	/* VERTEX_?_SPEC_G */
   INTERP_UB( t, ((GLubyte *)dst)[2], ((GLubyte *)out)[2], ((GLubyte *)in)[2] );	/* VERTEX_?_SPEC_R */
#endif
   
#if DO_FOG
   INTERP_UB( t, ((GLubyte *)dst)[3], ((GLubyte *)out)[3], ((GLubyte *)in)[3] );	/* VERTEX_?_SPEC_A */
#endif /* DO_FOG */

   dst++; out++; in++;

   LE32_OUT( dst, VIEWPORT_Z( dstclip[2] * w ) );		/* VERTEX_?_Z */
   dst++; out++; in++;
  
   INTERP_UB( t, ((GLubyte *)dst)[0], ((GLubyte *)out)[0], ((GLubyte *)in)[0] );	/* VERTEX_?_B */
   INTERP_UB( t, ((GLubyte *)dst)[1], ((GLubyte *)out)[1], ((GLubyte *)in)[1] );	/* VERTEX_?_G */
   INTERP_UB( t, ((GLubyte *)dst)[2], ((GLubyte *)out)[2], ((GLubyte *)in)[2] );	/* VERTEX_?_R */
   INTERP_UB( t, ((GLubyte *)dst)[3], ((GLubyte *)out)[3], ((GLubyte *)in)[3] );	/* VERTEX_?_A */
   dst++; /*out++; in++;*/

   LE32_OUT( dst,
	     (VIEWPORT_X( dstclip[0] * w ) << 16) |		/* VERTEX_?_X */
	     (VIEWPORT_Y( dstclip[1] * w ) & 0xffff) );		/* VERTEX_?_Y */

   assert( dst + 1 - (CARD32 *)(ddverts + (edst * size)) == 10 );
   assert( in  + 2 - (CARD32 *)(ddverts + (ein  * size)) == 10 );
   assert( out + 2 - (CARD32 *)(ddverts + (eout * size)) == 10 );

   if (MACH64_DEBUG & DEBUG_VERBOSE_PRIMS) {
      fprintf( stderr, "%s: dst vert: %.2f %.2f %.2f %x\n",
	       __FUNCTION__,
	       (GLshort)(LE32_IN( dst ) >> 16)/4.0,
	       (GLshort)(LE32_IN( dst ) & 0xffff)/4.0,
	       LE32_IN( dst - 2 )/65536.0,
	       *(GLuint *)(dst - 1) );
   }
}

#endif /* DO_RGBA && DO_XYZW */


static void TAG(copy_pv)( GLcontext *ctx, GLuint edst, GLuint esrc )
{
#if DO_SPEC || DO_FOG || DO_RGBA
   LOCALVARS   
   GLubyte *verts = GET_VERTEX_STORE();
   GLuint size = GET_VERTEX_SIZE();
   GLuint *dst = (GLuint *)(verts + (edst * size));
   GLuint *src = (GLuint *)(verts + (esrc * size));
#endif

#if DO_SPEC || DO_FOG
   dst[6] = src[6];			/* VERTEX_?_SPEC_ARGB */
#endif

#if DO_RGBA
   dst[8] = src[8];			/* VERTEX_?_ARGB */
#endif
}

static void TAG(init)( void )
{
   setup_tab[IND].emit = TAG(emit);

#if DO_XYZW && DO_RGBA
   setup_tab[IND].check_tex_sizes = TAG(check_tex_sizes);
   setup_tab[IND].interp = TAG(interp);
#endif

   setup_tab[IND].copy_pv = TAG(copy_pv);

#if DO_TEX1
   setup_tab[IND].vertex_format = TEX1_VERTEX_FORMAT;
   setup_tab[IND].vertex_size = 10;
#elif DO_TEX0
   setup_tab[IND].vertex_format = TEX0_VERTEX_FORMAT;
   setup_tab[IND].vertex_size = 7;
#elif DO_SPEC || DO_FOG
   setup_tab[IND].vertex_format = NOTEX_VERTEX_FORMAT;
   setup_tab[IND].vertex_size = 4;
#else
   setup_tab[IND].vertex_format = TINY_VERTEX_FORMAT;
   setup_tab[IND].vertex_size = 3;
#endif

}


#undef IND
#undef TAG
