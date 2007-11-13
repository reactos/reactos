/* $XFree86$ */ /* -*- mode: c; c-basic-offset: 3 -*- */
/*
 * Copyright 2000 Gareth Hughes
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * GARETH HUGHES BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *	Gareth Hughes <gareth@valinux.com>
 *	Leif Delgass <ldelgass@retinalburn.net>
 *	José Fonseca <j_r_fonseca@yahoo.co.uk>
 */

#include "glheader.h"
#include "mtypes.h"
#include "colormac.h"
#include "macros.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#include "mach64_tris.h"
#include "mach64_state.h"
#include "mach64_context.h"
#include "mach64_vb.h"
#include "mach64_ioctl.h"

static const GLuint hw_prim[GL_POLYGON+1] = {
   MACH64_PRIM_POINTS,
   MACH64_PRIM_LINES,
   MACH64_PRIM_LINE_LOOP,
   MACH64_PRIM_LINE_STRIP,
   MACH64_PRIM_TRIANGLES,
   MACH64_PRIM_TRIANGLE_STRIP,
   MACH64_PRIM_TRIANGLE_FAN,
   MACH64_PRIM_QUADS,
   MACH64_PRIM_QUAD_STRIP,
   MACH64_PRIM_POLYGON,
};

static void mach64RasterPrimitive( GLcontext *ctx, GLuint hwprim );
static void mach64RenderPrimitive( GLcontext *ctx, GLenum prim );


/* FIXME: Remove this when native template is finished. */
#define MACH64_PRINT_BUFFER 0

/***********************************************************************
 *                    Emit primitives as inline vertices               *
 ***********************************************************************/

#if defined(USE_X86_ASM)
#define DO_COPY_VERTEX( vb, vertsize, v, n, m )					\
do {										\
   register const CARD32 *__p __asm__( "esi" ) = (CARD32 *)v + 10 - vertsize;	\
   register int __s __asm__( "ecx" ) = vertsize;				\
   if ( vertsize > 7 ) {							\
      *vb++ = (2 << 16) | ADRINDEX( MACH64_VERTEX_##n##_SECONDARY_S );		\
      __asm__ __volatile__( "movsl ; movsl ; movsl"				\
			    : "=D" (vb), "=S" (__p)				\
			    : "0" (vb), "1" (__p) );				\
      __s -= 3;									\
   }										\
   *vb++ = ((__s - 1 + m) << 16) |						\
   	   (ADRINDEX( MACH64_VERTEX_##n##_X_Y ) - (__s - 1) );			\
   __asm__ __volatile__( "rep ; movsl"						\
			 : "=%c" (__s), "=D" (vb), "=S" (__p)			\
			 : "0" (__s), "1" (vb), "2" (__p) );			\
} while (0)
#else
#define DO_COPY_VERTEX( vb, vertsize, v, n, m )				\
do {									\
   CARD32 *__p = (CARD32 *)v + 10 - vertsize;				\
   int __s = vertsize;							\
   if ( vertsize > 7 ) {						\
      LE32_OUT( vb++, (2 << 16) |					\
	    	      ADRINDEX( MACH64_VERTEX_##n##_SECONDARY_S ) );	\
      *vb++ = *__p++;							\
      *vb++ = *__p++;							\
      *vb++ = *__p++;							\
      __s -= 3;								\
   }									\
   LE32_OUT( vb++, ((__s - 1 + m) << 16) |				\
	           (ADRINDEX( MACH64_VERTEX_##n##_X_Y ) - (__s - 1)) );	\
   while ( __s-- ) {							\
      *vb++ = *__p++;							\
   }									\
} while (0)
#endif

#define COPY_VERTEX( vb, vertsize, v, n )	DO_COPY_VERTEX( vb, vertsize, v, n, 0 )
#define COPY_VERTEX_OOA( vb, vertsize, v, n )	DO_COPY_VERTEX( vb, vertsize, v, n, 1 )


static __inline void mach64_draw_quad( mach64ContextPtr mmesa,
				       mach64VertexPtr v0,
				       mach64VertexPtr v1,
				       mach64VertexPtr v2,
				       mach64VertexPtr v3 )
{
#if MACH64_NATIVE_VTXFMT
   GLcontext *ctx = mmesa->glCtx;
   const GLuint vertsize = mmesa->vertex_size;
   GLint a;
   GLfloat ooa;
   GLuint xy;
   const GLuint xyoffset = 9;
   GLint xx[3], yy[3]; /* 2 fractional bits for hardware */
   unsigned vbsiz = (vertsize + (vertsize > 7 ? 2 : 1)) * 4 + 2;
   CARD32 *vb, *vbchk;

   if ( MACH64_DEBUG & DEBUG_VERBOSE_PRIMS ) {
      fprintf(stderr, "%s:\n", __FUNCTION__);
      fprintf(stderr,"Vertex 1:\n");
      mach64_print_vertex( ctx, v0 );
      fprintf(stderr,"Vertex 2:\n");
      mach64_print_vertex( ctx, v1 );
      fprintf(stderr,"Vertex 3:\n");
      mach64_print_vertex( ctx, v2 );
      fprintf(stderr,"Vertex 4:\n");
      mach64_print_vertex( ctx, v3 );
   }
   
   xy = LE32_IN( &v0->ui[xyoffset] );
   xx[0] = (GLshort)( xy >> 16 );
   yy[0] = (GLshort)( xy & 0xffff );
   
   xy = LE32_IN( &v1->ui[xyoffset] );
   xx[1] = (GLshort)( xy >> 16 );
   yy[1] = (GLshort)( xy & 0xffff );
   
   xy = LE32_IN( &v3->ui[xyoffset] );
   xx[2] = (GLshort)( xy >> 16 );
   yy[2] = (GLshort)( xy & 0xffff );
	   
   a = (xx[0] - xx[2]) * (yy[1] - yy[2]) -
       (yy[0] - yy[2]) * (xx[1] - xx[2]);

   if ( (mmesa->backface_sign &&
	((a < 0 && !signbit( mmesa->backface_sign )) || 
	(a > 0 && signbit( mmesa->backface_sign )))) ) {
      /* cull quad */
      if ( MACH64_DEBUG & DEBUG_VERBOSE_PRIMS )
	 fprintf(stderr,"Quad culled\n");
      return;
   }
   
   ooa = 16.0 / a;
   
   vb = (CARD32 *)mach64AllocDmaLow( mmesa, vbsiz * sizeof(CARD32) );
   vbchk = vb + vbsiz;

   COPY_VERTEX( vb, vertsize, v0, 1 );
   COPY_VERTEX( vb, vertsize, v1, 2 );
   COPY_VERTEX_OOA( vb, vertsize, v3, 3 );
   LE32_OUT( vb++, *(CARD32 *)&ooa );

   xy = LE32_IN( &v2->ui[xyoffset] );
   xx[0] = (GLshort)( xy >> 16 );
   yy[0] = (GLshort)( xy & 0xffff );
	   
   a = (xx[0] - xx[2]) * (yy[1] - yy[2]) -
       (yy[0] - yy[2]) * (xx[1] - xx[2]);
   
   ooa = 16.0 / a;
   
   COPY_VERTEX_OOA( vb, vertsize, v2, 1 );
   LE32_OUT( vb++, *(CARD32 *)&ooa );

   assert( vb == vbchk );
   
#if MACH64_PRINT_BUFFER
   {
      int i;
      fprintf(stderr, "quad:\n");
      for (i = 0; i < vbsiz; i++)
	 fprintf(stderr, "  %08lx\n", *(vb - vbsiz + i));
      fprintf(stderr, "\n");
   }
#endif
#else
   GLuint vertsize = mmesa->vertex_size;
   GLint coloridx;
   GLfloat ooa;
   GLint xx[3], yy[3]; /* 2 fractional bits for hardware */
   unsigned vbsiz = 
	 ((
	    1 +
	    (vertsize > 6 ? 2 : 0) +
	    (vertsize > 4 ? 2 : 0) +
	    3 +
	    (mmesa->multitex ? 4 : 0)
	 ) * 4 + 4);
   CARD32 *vb;
   unsigned vbidx = 0;

   if ( MACH64_DEBUG & DEBUG_VERBOSE_PRIMS ) {
      fprintf(stderr, "%s:\n", __FUNCTION__);
      fprintf(stderr,"Vertex 1: x: %.2f, y: %.2f, z: %.2f, w: %f\n\ts0: %f, t0: %f\n\ts1: %f, t1: %f\n", 
	      v0->v.x, v0->v.y, v0->v.z, v0->v.w, v0->v.u0, v0->v.v0, v0->v.u1, v0->v.v1);
      fprintf(stderr,"Vertex 2: x: %.2f, y: %.2f, z: %.2f, w: %f\n\ts0: %f, t0: %f\n\ts1: %f, t1: %f\n", 
	      v1->v.x, v1->v.y, v1->v.z, v1->v.w, v1->v.u0, v1->v.v0, v1->v.u1, v1->v.v1);
      fprintf(stderr,"Vertex 3: x: %.2f, y: %.2f, z: %.2f, w: %f\n\ts0: %f, t0: %f\n\ts1: %f, t1: %f\n", 
	      v2->v.x, v2->v.y, v2->v.z, v2->v.w, v2->v.u0, v2->v.v0, v2->v.u1, v2->v.v1);
      fprintf(stderr,"Vertex 4: x: %.2f, y: %.2f, z: %.2f, w: %f\n\ts0: %f, t0: %f\n\ts1: %f, t1: %f\n", 
	      v3->v.x, v3->v.y, v3->v.z, v3->v.w, v3->v.u0, v3->v.v0, v3->v.u1, v3->v.v1);
   }

#if MACH64_CLIENT_STATE_EMITS
   /* Enable for interleaved client-side state emits */
   LOCK_HARDWARE( mmesa );
   if ( mmesa->dirty ) {
      mach64EmitHwStateLocked( mmesa );
   }
   if ( mmesa->sarea->dirty ) {
      mach64UploadHwStateLocked( mmesa );
   }
   UNLOCK_HARDWARE( mmesa );
#endif

   xx[0] = (GLint)(v0->v.x * 4);
   yy[0] = (GLint)(v0->v.y * 4);

   xx[1] = (GLint)(v1->v.x * 4);
   yy[1] = (GLint)(v1->v.y * 4);

   xx[2] = (GLint)(v3->v.x * 4);
   yy[2] = (GLint)(v3->v.y * 4);

   ooa = 0.25 * 0.25 * ((xx[0] - xx[2]) * (yy[1] - yy[2]) -
			(yy[0] - yy[2]) * (xx[1] - xx[2]));
   
   if ( ooa * mmesa->backface_sign < 0 ) {
      /* cull quad */
      if ( MACH64_DEBUG & DEBUG_VERBOSE_PRIMS )
	 fprintf(stderr,"Quad culled\n");
      return;
   }
   
   vb = (CARD32 *)mach64AllocDmaLow( mmesa, vbsiz * 4 );
   
   ooa = 1.0 / ooa;

   coloridx = (vertsize > 4) ? 4: 3;

   /* setup for 3,5, or 7 sequential reg writes based on vertex format */
   switch (vertsize) {
   case 6:
      LE32_OUT( &vb[vbidx++], (4 << 16) | ADRINDEX(MACH64_VERTEX_1_W) );
      break;
   case 4:
      LE32_OUT( &vb[vbidx++], (2 << 16) | ADRINDEX(MACH64_VERTEX_1_Z) );
      break;
   default: /* vertsize >= 8 */
      LE32_OUT( &vb[vbidx++], (6 << 16) | ADRINDEX(MACH64_VERTEX_1_S) );
      break;
   }
   if (vertsize > 6) {
      LE32_OUT( &vb[vbidx++], v0->ui[6] ); /* MACH64_VERTEX_1_S */
      LE32_OUT( &vb[vbidx++], v0->ui[7] ); /* MACH64_VERTEX_1_T */
   }
   if (vertsize > 4) {
      LE32_OUT( &vb[vbidx++], v0->ui[3] ); /* MACH64_VERTEX_1_W */
      LE32_OUT( &vb[vbidx++], v0->ui[5] ); /* MACH64_VERTEX_1_SPEC_ARGB */
   }
   LE32_OUT( &vb[vbidx++], ((GLint)(v0->v.z) << 15) );         /* MACH64_VERTEX_1_Z */
   vb[vbidx++] = v0->ui[coloridx];                            /* MACH64_VERTEX_1_ARGB */
   LE32_OUT( &vb[vbidx++], (xx[0] << 16) | (yy[0] & 0xffff) ); /* MACH64_VERTEX_1_X_Y */

   if (mmesa->multitex) {
      /* setup for 3 sequential reg writes */
      LE32_OUT( &vb[vbidx++], (2 << 16) | ADRINDEX(MACH64_VERTEX_1_SECONDARY_S) );
      LE32_OUT( &vb[vbidx++], v0->ui[8] ); /* MACH64_VERTEX_1_SECONDARY_S */
      LE32_OUT( &vb[vbidx++], v0->ui[9] ); /* MACH64_VERTEX_1_SECONDARY_T */
      LE32_OUT( &vb[vbidx++], v0->ui[3] ); /* MACH64_VERTEX_1_SECONDARY_W */
   }

   /* setup for 3,5, or 7 sequential reg writes based on vertex format */
   switch (vertsize) {
   case 6:
      LE32_OUT( &vb[vbidx++], (4 << 16) | ADRINDEX(MACH64_VERTEX_2_W) );
      break;
   case 4:
      LE32_OUT( &vb[vbidx++], (2 << 16) | ADRINDEX(MACH64_VERTEX_2_Z) );
      break;
   default: /* vertsize >= 8 */
      LE32_OUT( &vb[vbidx++], (6 << 16) | ADRINDEX(MACH64_VERTEX_2_S) );
      break;
   }
   if (vertsize > 6) {
      LE32_OUT( &vb[vbidx++], v1->ui[6] ); /* MACH64_VERTEX_2_S */
      LE32_OUT( &vb[vbidx++], v1->ui[7] ); /* MACH64_VERTEX_2_T */
   }
   if (vertsize > 4) {
      LE32_OUT( &vb[vbidx++], v1->ui[3] ); /* MACH64_VERTEX_2_W */
      LE32_OUT( &vb[vbidx++], v1->ui[5] ); /* MACH64_VERTEX_2_SPEC_ARGB */
   }
   LE32_OUT( &vb[vbidx++], ((GLint)(v1->v.z) << 15) );         /* MACH64_VERTEX_2_Z */
   vb[vbidx++] = v1->ui[coloridx];                            /* MACH64_VERTEX_2_ARGB */
   LE32_OUT( &vb[vbidx++], (xx[1] << 16) | (yy[1] & 0xffff) ); /* MACH64_VERTEX_2_X_Y */

   if (mmesa->multitex) {
      /* setup for 3 sequential reg writes */
      LE32_OUT( &vb[vbidx++], (2 << 16) | ADRINDEX(MACH64_VERTEX_2_SECONDARY_S) );
      LE32_OUT( &vb[vbidx++], v1->ui[8] ); /* MACH64_VERTEX_2_SECONDARY_S */
      LE32_OUT( &vb[vbidx++], v1->ui[9] ); /* MACH64_VERTEX_2_SECONDARY_T */
      LE32_OUT( &vb[vbidx++], v1->ui[3] ); /* MACH64_VERTEX_2_SECONDARY_W */
   }

   /* setup for 3,5, or 7 sequential reg writes based on vertex format */
   switch (vertsize) {
   case 6:
      LE32_OUT( &vb[vbidx++], (4 << 16) | ADRINDEX(MACH64_VERTEX_3_W) );
      break;
   case 4:
      LE32_OUT( &vb[vbidx++], (2 << 16) | ADRINDEX(MACH64_VERTEX_3_Z) );
      break;
   default: /* vertsize >= 8 */
      LE32_OUT( &vb[vbidx++], (6 << 16) | ADRINDEX(MACH64_VERTEX_3_S) );
      break;
   }
   if (vertsize > 6) {
      LE32_OUT( &vb[vbidx++], v3->ui[6] ); /* MACH64_VERTEX_3_S */
      LE32_OUT( &vb[vbidx++], v3->ui[7] ); /* MACH64_VERTEX_3_T */
   }
   if (vertsize > 4) {
      LE32_OUT( &vb[vbidx++], v3->ui[3] ); /* MACH64_VERTEX_3_W */
      LE32_OUT( &vb[vbidx++], v3->ui[5] ); /* MACH64_VERTEX_3_SPEC_ARGB */
   }
   LE32_OUT( &vb[vbidx++], ((GLint)(v3->v.z) << 15) );         /* MACH64_VERTEX_3_Z */
   vb[vbidx++] = v3->ui[coloridx];                             /* MACH64_VERTEX_3_ARGB */
   LE32_OUT( &vb[vbidx++], (xx[2] << 16) | (yy[2] & 0xffff) ); /* MACH64_VERTEX_3_X_Y */

   if (mmesa->multitex) {
      /* setup for 3 sequential reg writes */
      LE32_OUT( &vb[vbidx++], (2 << 16) | ADRINDEX(MACH64_VERTEX_3_SECONDARY_S) );
      LE32_OUT( &vb[vbidx++], v3->ui[8] ); /* MACH64_VERTEX_3_SECONDARY_S */
      LE32_OUT( &vb[vbidx++], v3->ui[9] ); /* MACH64_VERTEX_3_SECONDARY_T */
      LE32_OUT( &vb[vbidx++], v3->ui[3] ); /* MACH64_VERTEX_3_SECONDARY_W */
   }

   LE32_OUT( &vb[vbidx++], ADRINDEX(MACH64_ONE_OVER_AREA_UC) );
   LE32_OUT( &vb[vbidx++], *(GLuint *)&ooa );

   xx[0] = (GLint)(v2->v.x * 4);
   yy[0] = (GLint)(v2->v.y * 4);

   ooa = 0.25 * 0.25 * ((xx[0] - xx[2]) * (yy[1] - yy[2]) -
			(yy[0] - yy[2]) * (xx[1] - xx[2]));
   ooa = 1.0 / ooa;

   /* setup for 3,5, or 7 sequential reg writes based on vertex format */
   switch (vertsize) {
   case 6:
      LE32_OUT( &vb[vbidx++], (4 << 16) | ADRINDEX(MACH64_VERTEX_1_W) );
      break;
   case 4:
      LE32_OUT( &vb[vbidx++], (2 << 16) | ADRINDEX(MACH64_VERTEX_1_Z) );
      break;
   default: /* vertsize >= 8 */
      LE32_OUT( &vb[vbidx++], (6 << 16) | ADRINDEX(MACH64_VERTEX_1_S) );
      break;
   }
   if (vertsize > 6) {
      LE32_OUT( &vb[vbidx++], v2->ui[6] ); /* MACH64_VERTEX_1_S */
      LE32_OUT( &vb[vbidx++], v2->ui[7] ); /* MACH64_VERTEX_1_T */
   }
   if (vertsize > 4) {
      LE32_OUT( &vb[vbidx++], v2->ui[3] ); /* MACH64_VERTEX_1_W */
      LE32_OUT( &vb[vbidx++], v2->ui[5] ); /* MACH64_VERTEX_1_SPEC_ARGB */
   }
   LE32_OUT( &vb[vbidx++], ((GLint)(v2->v.z) << 15) );         /* MACH64_VERTEX_1_Z */
   vb[vbidx++] = v2->ui[coloridx];                             /* MACH64_VERTEX_1_ARGB */
   LE32_OUT( &vb[vbidx++], (xx[0] << 16) | (yy[0] & 0xffff) ); /* MACH64_VERTEX_1_X_Y */

   if (mmesa->multitex) {
      /* setup for 3 sequential reg writes */
      LE32_OUT( &vb[vbidx++], (2 << 16) | ADRINDEX(MACH64_VERTEX_1_SECONDARY_S) );
      LE32_OUT( &vb[vbidx++], v2->ui[8] ); /* MACH64_VERTEX_1_SECONDARY_S */
      LE32_OUT( &vb[vbidx++], v2->ui[9] ); /* MACH64_VERTEX_1_SECONDARY_T */
      LE32_OUT( &vb[vbidx++], v2->ui[3] ); /* MACH64_VERTEX_1_SECONDARY_W */
   }

   LE32_OUT( &vb[vbidx++], ADRINDEX(MACH64_ONE_OVER_AREA_UC) );
   LE32_OUT( &vb[vbidx++], *(GLuint *)&ooa );

   assert(vbsiz == vbidx);

#if MACH64_PRINT_BUFFER
   {
      int i;
      fprintf(stderr, "quad:\n");
      for (i = 0; i < vbsiz; i++)
	 fprintf(stderr, "  %08lx\n", *(vb + i));
      fprintf(stderr, "\n");
   }
#endif
#endif
}

static __inline void mach64_draw_triangle( mach64ContextPtr mmesa,
					   mach64VertexPtr v0,
					   mach64VertexPtr v1,
					   mach64VertexPtr v2 )
{
#if MACH64_NATIVE_VTXFMT
   GLcontext *ctx = mmesa->glCtx;
   GLuint vertsize = mmesa->vertex_size;
   GLint a;
   GLfloat ooa;
   GLuint xy;
   const GLuint xyoffset = 9;
   GLint xx[3], yy[3]; /* 2 fractional bits for hardware */
   unsigned vbsiz = (vertsize + (vertsize > 7 ? 2 : 1)) * 3 + 1;
   CARD32 *vb, *vbchk;

   if ( MACH64_DEBUG & DEBUG_VERBOSE_PRIMS ) {
      fprintf(stderr, "%s:\n", __FUNCTION__);
      fprintf(stderr,"Vertex 1:\n");
      mach64_print_vertex( ctx, v0 );
      fprintf(stderr,"Vertex 2:\n");
      mach64_print_vertex( ctx, v1 );
      fprintf(stderr,"Vertex 3:\n");
      mach64_print_vertex( ctx, v2 );
   }
   
   xy = LE32_IN( &v0->ui[xyoffset] );
   xx[0] = (GLshort)( xy >> 16 );
   yy[0] = (GLshort)( xy & 0xffff );
   
   xy = LE32_IN( &v1->ui[xyoffset] );
   xx[1] = (GLshort)( xy >> 16 );
   yy[1] = (GLshort)( xy & 0xffff );
   
   xy = LE32_IN( &v2->ui[xyoffset] );
   xx[2] = (GLshort)( xy >> 16 );
   yy[2] = (GLshort)( xy & 0xffff );
	   
   a = (xx[0] - xx[2]) * (yy[1] - yy[2]) -
       (yy[0] - yy[2]) * (xx[1] - xx[2]);
   
   if ( mmesa->backface_sign &&
	((a < 0 && !signbit( mmesa->backface_sign )) || 
	(a > 0 && signbit( mmesa->backface_sign ))) ) {
      /* cull triangle */
      if ( MACH64_DEBUG & DEBUG_VERBOSE_PRIMS )
	 fprintf(stderr,"Triangle culled\n");
      return;
   }
   
   ooa = 16.0 / a;
   
   vb = (CARD32 *)mach64AllocDmaLow( mmesa, vbsiz * sizeof(CARD32) );
   vbchk = vb + vbsiz;

   COPY_VERTEX( vb, vertsize, v0, 1 );
   COPY_VERTEX( vb, vertsize, v1, 2 );
   COPY_VERTEX_OOA( vb, vertsize, v2, 3 );
   LE32_OUT( vb++, *(CARD32 *)&ooa );

   assert( vb == vbchk );

#if MACH64_PRINT_BUFFER
   {
      int i;
      fprintf(stderr, "tri:\n");
      for (i = 0; i < vbsiz; i++)
	 fprintf(stderr, "  %08lx\n", *(vb - vbsiz + i));
      fprintf(stderr, "\n");
   }
#endif
#else
   GLuint vertsize = mmesa->vertex_size;
   GLint coloridx;
   GLfloat ooa;
   GLint xx[3], yy[3]; /* 2 fractional bits for hardware */
   unsigned vbsiz = 
	 ((
	    1 +
	    (vertsize > 6 ? 2 : 0) +
	    (vertsize > 4 ? 2 : 0) +
	    3 +
	    (mmesa->multitex ? 4 : 0)
	 ) * 3 + 2);
   CARD32 *vb;
   unsigned vbidx = 0;

   if ( MACH64_DEBUG & DEBUG_VERBOSE_PRIMS ) {
      fprintf(stderr, "%s:\n", __FUNCTION__);
      fprintf(stderr,"Vertex 1: x: %.2f, y: %.2f, z: %.2f, w: %f\n\ts0: %f, t0: %f\n\ts1: %f, t1: %f\n", 
	      v0->v.x, v0->v.y, v0->v.z, v0->v.w, v0->v.u0, v0->v.v0, v0->v.u1, v0->v.v1);
      fprintf(stderr,"Vertex 2: x: %.2f, y: %.2f, z: %.2f, w: %f\n\ts0: %f, t0: %f\n\ts1: %f, t1: %f\n", 
	      v1->v.x, v1->v.y, v1->v.z, v1->v.w, v1->v.u0, v1->v.v0, v1->v.u1, v1->v.v1);
      fprintf(stderr,"Vertex 3: x: %.2f, y: %.2f, z: %.2f, w: %f\n\ts0: %f, t0: %f\n\ts1: %f, t1: %f\n", 
	      v2->v.x, v2->v.y, v2->v.z, v2->v.w, v2->v.u0, v2->v.v0, v2->v.u1, v2->v.v1);
   }

#if MACH64_CLIENT_STATE_EMITS
   /* Enable for interleaved client-side state emits */
   LOCK_HARDWARE( mmesa );
   if ( mmesa->dirty ) {
      mach64EmitHwStateLocked( mmesa );
   }
   if ( mmesa->sarea->dirty ) {
      mach64UploadHwStateLocked( mmesa );
   }
   UNLOCK_HARDWARE( mmesa );
#endif

   xx[0] = (GLint)(v0->v.x * 4);
   yy[0] = (GLint)(v0->v.y * 4);

   xx[1] = (GLint)(v1->v.x * 4);
   yy[1] = (GLint)(v1->v.y * 4);

   xx[2] = (GLint)(v2->v.x * 4);
   yy[2] = (GLint)(v2->v.y * 4);

   ooa = 0.25 * 0.25 * ((xx[0] - xx[2]) * (yy[1] - yy[2]) -
			(yy[0] - yy[2]) * (xx[1] - xx[2]));

   if ( ooa * mmesa->backface_sign < 0 ) {
      /* cull triangle */
       if ( MACH64_DEBUG & DEBUG_VERBOSE_PRIMS )
	 fprintf(stderr,"Triangle culled\n");
      return;
   }

   vb = (CARD32 *)mach64AllocDmaLow( mmesa, vbsiz * 4 );
   
   ooa = 1.0 / ooa;

   coloridx = (vertsize > 4) ? 4: 3;

   /* setup for 3,5, or 7 sequential reg writes based on vertex format */
   switch (vertsize) {
   case 6:
      LE32_OUT( &vb[vbidx++], (4 << 16) | ADRINDEX(MACH64_VERTEX_1_W) );
      break;
   case 4:
      LE32_OUT( &vb[vbidx++], (2 << 16) | ADRINDEX(MACH64_VERTEX_1_Z) );
      break;
   default: /* vertsize >= 8 */
      LE32_OUT( &vb[vbidx++], (6 << 16) | ADRINDEX(MACH64_VERTEX_1_S) );
      break;
   }
   if (vertsize > 6) {
      LE32_OUT( &vb[vbidx++], v0->ui[6] ); /* MACH64_VERTEX_1_S */
      LE32_OUT( &vb[vbidx++], v0->ui[7] ); /* MACH64_VERTEX_1_T */
   }
   if (vertsize > 4) {
      LE32_OUT( &vb[vbidx++], v0->ui[3] ); /* MACH64_VERTEX_1_W */
      LE32_OUT( &vb[vbidx++], v0->ui[5] ); /* MACH64_VERTEX_1_SPEC_ARGB */
   }
   LE32_OUT( &vb[vbidx++], ((GLint)(v0->v.z) << 15) );         /* MACH64_VERTEX_1_Z */
   vb[vbidx++] = v0->ui[coloridx];                             /* MACH64_VERTEX_1_ARGB */
   LE32_OUT( &vb[vbidx++], (xx[0] << 16) | (yy[0] & 0xffff) ); /* MACH64_VERTEX_1_X_Y */

   if (mmesa->multitex) {
      /* setup for 3 sequential reg writes */
      LE32_OUT( &vb[vbidx++], (2 << 16) | ADRINDEX(MACH64_VERTEX_1_SECONDARY_S) );
      LE32_OUT( &vb[vbidx++], v0->ui[8] ); /* MACH64_VERTEX_1_SECONDARY_S */
      LE32_OUT( &vb[vbidx++], v0->ui[9] ); /* MACH64_VERTEX_1_SECONDARY_T */
      LE32_OUT( &vb[vbidx++], v0->ui[3] ); /* MACH64_VERTEX_1_SECONDARY_W */
   }

   /* setup for 3,5, or 7 sequential reg writes based on vertex format */
   switch (vertsize) {
   case 6:
      LE32_OUT( &vb[vbidx++], (4 << 16) | ADRINDEX(MACH64_VERTEX_2_W) );
      break;
   case 4:
      LE32_OUT( &vb[vbidx++], (2 << 16) | ADRINDEX(MACH64_VERTEX_2_Z) );
      break;
   default: /* vertsize >= 8 */
      LE32_OUT( &vb[vbidx++], (6 << 16) | ADRINDEX(MACH64_VERTEX_2_S) );
      break;
   }
   if (vertsize > 6) {
      LE32_OUT( &vb[vbidx++], v1->ui[6] ); /* MACH64_VERTEX_2_S */
      LE32_OUT( &vb[vbidx++], v1->ui[7] ); /* MACH64_VERTEX_2_T */
   }
   if (vertsize > 4) {
      LE32_OUT( &vb[vbidx++], v1->ui[3] ); /* MACH64_VERTEX_2_W */
      LE32_OUT( &vb[vbidx++], v1->ui[5] ); /* MACH64_VERTEX_2_SPEC_ARGB */
   }
   LE32_OUT( &vb[vbidx++], ((GLint)(v1->v.z) << 15) );         /* MACH64_VERTEX_2_Z */
   vb[vbidx++] = v1->ui[coloridx];                             /* MACH64_VERTEX_2_ARGB */
   LE32_OUT( &vb[vbidx++], (xx[1] << 16) | (yy[1] & 0xffff) ); /* MACH64_VERTEX_2_X_Y */

   if (mmesa->multitex) {
      /* setup for 3 sequential reg writes */
      LE32_OUT( &vb[vbidx++], (2 << 16) | ADRINDEX(MACH64_VERTEX_2_SECONDARY_S) );
      LE32_OUT( &vb[vbidx++], v1->ui[8] ); /* MACH64_VERTEX_2_SECONDARY_S */
      LE32_OUT( &vb[vbidx++], v1->ui[9] ); /* MACH64_VERTEX_2_SECONDARY_T */
      LE32_OUT( &vb[vbidx++], v1->ui[3] ); /* MACH64_VERTEX_2_SECONDARY_W */
   }

   /* setup for 3,5, or 7 sequential reg writes based on vertex format */
   switch (vertsize) {
   case 6:
      LE32_OUT( &vb[vbidx++], (4 << 16) | ADRINDEX(MACH64_VERTEX_3_W) );
      break;
   case 4:
      LE32_OUT( &vb[vbidx++], (2 << 16) | ADRINDEX(MACH64_VERTEX_3_Z) );
      break;
   default: /* vertsize >= 8 */
      LE32_OUT( &vb[vbidx++], (6 << 16) | ADRINDEX(MACH64_VERTEX_3_S) );
      break;
   }
   if (vertsize > 6) {
      LE32_OUT( &vb[vbidx++], v2->ui[6] ); /* MACH64_VERTEX_3_S */
      LE32_OUT( &vb[vbidx++], v2->ui[7] ); /* MACH64_VERTEX_3_T */
   }
   if (vertsize > 4) {
      LE32_OUT( &vb[vbidx++], v2->ui[3] ); /* MACH64_VERTEX_3_W */
      LE32_OUT( &vb[vbidx++], v2->ui[5] ); /* MACH64_VERTEX_3_SPEC_ARGB */
   }
   LE32_OUT( &vb[vbidx++], ((GLint)(v2->v.z) << 15) );         /* MACH64_VERTEX_3_Z */
   vb[vbidx++] = v2->ui[coloridx];                             /* MACH64_VERTEX_3_ARGB */
   LE32_OUT( &vb[vbidx++], (xx[2] << 16) | (yy[2] & 0xffff) ); /* MACH64_VERTEX_3_X_Y */

   if (mmesa->multitex) {
      /* setup for 3 sequential reg writes */
      LE32_OUT( &vb[vbidx++], (2 << 16) | ADRINDEX(MACH64_VERTEX_3_SECONDARY_S) );
      LE32_OUT( &vb[vbidx++], v2->ui[8] ); /* MACH64_VERTEX_3_SECONDARY_S */
      LE32_OUT( &vb[vbidx++], v2->ui[9] ); /* MACH64_VERTEX_3_SECONDARY_T */
      LE32_OUT( &vb[vbidx++], v2->ui[3] ); /* MACH64_VERTEX_3_SECONDARY_W */
   }

   LE32_OUT( &vb[vbidx++], ADRINDEX(MACH64_ONE_OVER_AREA_UC) );
   LE32_OUT( &vb[vbidx++], *(GLuint *)&ooa );

   assert(vbsiz == vbidx);

#if MACH64_PRINT_BUFFER
   {
      int i;
      fprintf(stderr, "tri:\n");
      for (i = 0; i < vbsiz; ++i)
	 fprintf(stderr, "  %08lx\n", *(vb + i));
      fprintf(stderr, "\n");
   }
#endif
#endif
}

static __inline void mach64_draw_line( mach64ContextPtr mmesa,
				     mach64VertexPtr v0,
				     mach64VertexPtr v1 )
{
#if MACH64_NATIVE_VTXFMT
   GLcontext *ctx = mmesa->glCtx;
   const GLuint vertsize = mmesa->vertex_size;
   GLint width = (GLint)(mmesa->glCtx->Line._Width * 2.0); /* 2 fractional bits for hardware */
   GLfloat ooa;
   GLuint *pxy0, *pxy1;
   GLuint xy0old, xy0, xy1old, xy1;
   const GLuint xyoffset = 9;
   GLint x0, y0, x1, y1;
   GLint dx, dy, ix, iy;
   unsigned vbsiz = (vertsize + (vertsize > 7 ? 2 : 1)) * 4 + 2;
   CARD32 *vb, *vbchk;

   if ( MACH64_DEBUG & DEBUG_VERBOSE_PRIMS ) {
      fprintf(stderr, "%s:\n", __FUNCTION__);
      fprintf(stderr,"Vertex 1:\n");
      mach64_print_vertex( ctx, v0 );
      fprintf(stderr,"Vertex 2:\n");
      mach64_print_vertex( ctx, v1 );
   }
  
   if( !width )
      width = 1;	/* round to the nearest supported width */
      
   pxy0 = &v0->ui[xyoffset];
   xy0old = *pxy0;
   xy0 = LE32_IN( &xy0old );
   x0 = (GLshort)( xy0 >> 16 );
   y0 = (GLshort)( xy0 & 0xffff );
   
   pxy1 = &v1->ui[xyoffset];
   xy1old = *pxy1;
   xy1 = LE32_IN( &xy1old );
   x1 = (GLshort)( xy1 >> 16 );
   y1 = (GLshort)( xy1 & 0xffff );
   
   if ( (dx = x1 - x0) < 0 ) {
      dx = -dx;
   }
   if ( (dy = y1 - y0) < 0 ) {
      dy = -dy;
   }
   
   /* adjust vertices depending on line direction */
   if ( dx > dy ) {
      ix = 0;
      iy = width;
      ooa = 8.0 / ((x1 - x0) * width);
   } else {
      ix = width;
      iy = 0;
      ooa = 8.0 / ((y0 - y1) * width);
   }

   vb = (CARD32 *)mach64AllocDmaLow( mmesa, vbsiz * sizeof(CARD32) );
   vbchk = vb + vbsiz;

   LE32_OUT( pxy0, (( x0 - ix ) << 16) | (( y0 - iy ) & 0xffff) );
   COPY_VERTEX( vb, vertsize, v0, 1 );
   LE32_OUT( pxy1, (( x1 - ix ) << 16) | (( y1 - iy ) & 0xffff) );
   COPY_VERTEX( vb, vertsize, v1, 2 );
   LE32_OUT( pxy0, (( x0 + ix ) << 16) | (( y0 + iy ) & 0xffff) );
   COPY_VERTEX_OOA( vb, vertsize, v0, 3 );
   LE32_OUT( vb++, *(CARD32 *)&ooa );

   ooa = -ooa;
   
   LE32_OUT( pxy1, (( x1 + ix ) << 16) | (( y1 + iy ) & 0xffff) );
   COPY_VERTEX_OOA( vb, vertsize, v1, 1 );
   LE32_OUT( vb++, *(CARD32 *)&ooa );

   *pxy0 = xy0old;
   *pxy1 = xy1old;
#else /* !MACH64_NATIVE_VTXFMT */
   GLuint vertsize = mmesa->vertex_size;
   GLint coloridx;
   float width = 1.0; /* Only support 1 pix lines now */
   GLfloat ooa;
   GLint xx[3], yy[3]; /* 2 fractional bits for hardware */
   unsigned vbsiz = 
	 ((
	    1 +
	    (vertsize > 6 ? 2 : 0) +
	    (vertsize > 4 ? 2 : 0) +
	    3 +
	    (mmesa->multitex ? 4 : 0)
	 ) * 4 + 4);
   CARD32 *vb;
   unsigned vbidx = 0;
   
   GLfloat hw, dx, dy, ix, iy;
   GLfloat x0 = v0->v.x;
   GLfloat y0 = v0->v.y;
   GLfloat x1 = v1->v.x;
   GLfloat y1 = v1->v.y;

#if MACH64_CLIENT_STATE_EMITS
   /* Enable for interleaved client-side state emits */
   LOCK_HARDWARE( mmesa );
   if ( mmesa->dirty ) {
      mach64EmitHwStateLocked( mmesa );
   }
   if ( mmesa->sarea->dirty ) {
      mach64UploadHwStateLocked( mmesa );
   }
   UNLOCK_HARDWARE( mmesa );
#endif

   if ( MACH64_DEBUG & DEBUG_VERBOSE_PRIMS ) {
      fprintf(stderr, "%s:\n", __FUNCTION__);
      fprintf(stderr,"Vertex 1: x: %.2f, y: %.2f, z: %.2f, w: %f\n", 
	      v0->v.x, v0->v.y, v0->v.z, v0->v.w);
      fprintf(stderr,"Vertex 2: x: %.2f, y: %.2f, z: %.2f, w: %f\n", 
	      v1->v.x, v1->v.y, v1->v.z, v1->v.w);
   }

   hw = 0.5F * width;
   if (hw > 0.1F && hw < 0.5F) {
      hw = 0.5F;
   }

   /* adjust vertices depending on line direction */
   dx = v0->v.x - v1->v.x;
   dy = v0->v.y - v1->v.y;
   if (dx * dx > dy * dy) {
      /* X-major line */
      ix = 0.0F;
      iy = hw;
      if (x1 < x0) {
         x0 += 0.5F;
         x1 += 0.5F;
      }
      y0 -= 0.5F;
      y1 -= 0.5F;
   }
   else {
      /* Y-major line */
      ix = hw;
      iy = 0.0F;
      if (y1 > y0) {
         y0 -= 0.5F;
         y1 -= 0.5F;
      }
      x0 += 0.5F;
      x1 += 0.5F;
   }

   xx[0] = (GLint)((x0 - ix) * 4);
   yy[0] = (GLint)((y0 - iy) * 4);

   xx[1] = (GLint)((x1 - ix) * 4);
   yy[1] = (GLint)((y1 - iy) * 4);

   xx[2] = (GLint)((x0 + ix) * 4);
   yy[2] = (GLint)((y0 + iy) * 4);

   ooa = 0.25 * 0.25 * ((xx[0] - xx[2]) * (yy[1] - yy[2]) -
			(yy[0] - yy[2]) * (xx[1] - xx[2]));

   if ( ooa * mmesa->backface_sign < 0 ) {
      /* cull line */
      if ( MACH64_DEBUG & DEBUG_VERBOSE_PRIMS )
	 fprintf(stderr,"Line culled\n");
      return;
   }

   vb = (CARD32 *)mach64AllocDmaLow( mmesa, vbsiz * 4 );
   
   ooa = 1.0 / ooa;

   coloridx = (vertsize > 4) ? 4: 3;

   /* setup for 3,5, or 7 sequential reg writes based on vertex format */
   switch (vertsize) {
   case 6:
      LE32_OUT( &vb[vbidx++], (4 << 16) | ADRINDEX(MACH64_VERTEX_1_W) );
      break;
   case 4:
      LE32_OUT( &vb[vbidx++], (2 << 16) | ADRINDEX(MACH64_VERTEX_1_Z) );
      break;
   default: /* vertsize >= 8 */
      LE32_OUT( &vb[vbidx++], (6 << 16) | ADRINDEX(MACH64_VERTEX_1_S) );
      break;
   }
   if (vertsize > 6) {
      LE32_OUT( &vb[vbidx++], v0->ui[6] ); /* MACH64_VERTEX_1_S */
      LE32_OUT( &vb[vbidx++], v0->ui[7] ); /* MACH64_VERTEX_1_T */
   }
   if (vertsize > 4) {
      LE32_OUT( &vb[vbidx++], v0->ui[3] ); /* MACH64_VERTEX_1_W */
      LE32_OUT( &vb[vbidx++], v0->ui[5] ); /* MACH64_VERTEX_1_SPEC_ARGB */
   }
   LE32_OUT( &vb[vbidx++], ((GLint)(v0->v.z) << 15) );         /* MACH64_VERTEX_1_Z */
   vb[vbidx++] = v0->ui[coloridx];                             /* MACH64_VERTEX_1_ARGB */
   LE32_OUT( &vb[vbidx++], (xx[0] << 16) | (yy[0] & 0xffff) ); /* MACH64_VERTEX_1_X_Y */

   /* setup for 3,5, or 7 sequential reg writes based on vertex format */
   switch (vertsize) {
   case 6:
      LE32_OUT( &vb[vbidx++], (4 << 16) | ADRINDEX(MACH64_VERTEX_2_W) );
      break;
   case 4:
      LE32_OUT( &vb[vbidx++], (2 << 16) | ADRINDEX(MACH64_VERTEX_2_Z) );
      break;
   default: /* vertsize >= 8 */
      LE32_OUT( &vb[vbidx++], (6 << 16) | ADRINDEX(MACH64_VERTEX_2_S) );
      break;
   }
   if (vertsize > 6) {
      LE32_OUT( &vb[vbidx++], v1->ui[6] ); /* MACH64_VERTEX_2_S */
      LE32_OUT( &vb[vbidx++], v1->ui[7] ); /* MACH64_VERTEX_2_T */
   }
   if (vertsize > 4) {
      LE32_OUT( &vb[vbidx++], v1->ui[3] ); /* MACH64_VERTEX_2_W */
      LE32_OUT( &vb[vbidx++], v1->ui[5] ); /* MACH64_VERTEX_2_SPEC_ARGB */
   }
   LE32_OUT( &vb[vbidx++], ((GLint)(v1->v.z) << 15) );         /* MACH64_VERTEX_2_Z */
   vb[vbidx++] = v1->ui[coloridx];                             /* MACH64_VERTEX_2_ARGB */
   LE32_OUT( &vb[vbidx++], (xx[1] << 16) | (yy[1] & 0xffff) ); /* MACH64_VERTEX_2_X_Y */

   /* setup for 3,5, or 7 sequential reg writes based on vertex format */
   switch (vertsize) {
   case 6:
      LE32_OUT( &vb[vbidx++], (4 << 16) | ADRINDEX(MACH64_VERTEX_3_W) );
      break;
   case 4:
      LE32_OUT( &vb[vbidx++], (2 << 16) | ADRINDEX(MACH64_VERTEX_3_Z) );
      break;
   default: /* vertsize >= 8 */
      LE32_OUT( &vb[vbidx++], (6 << 16) | ADRINDEX(MACH64_VERTEX_3_S) );
      break;
   }
   if (vertsize > 6) {
      LE32_OUT( &vb[vbidx++], v0->ui[6] ); /* MACH64_VERTEX_3_S */
      LE32_OUT( &vb[vbidx++], v0->ui[7] ); /* MACH64_VERTEX_3_T */
   }
   if (vertsize > 4) {
      LE32_OUT( &vb[vbidx++], v0->ui[3] ); /* MACH64_VERTEX_3_W */
      LE32_OUT( &vb[vbidx++], v0->ui[5] ); /* MACH64_VERTEX_3_SPEC_ARGB */
   }
   LE32_OUT( &vb[vbidx++], ((GLint)(v0->v.z) << 15) );         /* MACH64_VERTEX_3_Z */
   vb[vbidx++] = v0->ui[coloridx];                             /* MACH64_VERTEX_3_ARGB */
   LE32_OUT( &vb[vbidx++], (xx[2] << 16) | (yy[2] & 0xffff) ); /* MACH64_VERTEX_3_X_Y */

   LE32_OUT( &vb[vbidx++], ADRINDEX(MACH64_ONE_OVER_AREA_UC) );
   LE32_OUT( &vb[vbidx++], *(GLuint *)&ooa );

   xx[0] = (GLint)((x1 + ix) * 4);
   yy[0] = (GLint)((y1 + iy) * 4);

   ooa = 0.25 * 0.25 * ((xx[0] - xx[2]) * (yy[1] - yy[2]) -
			(yy[0] - yy[2]) * (xx[1] - xx[2]));
   ooa = 1.0 / ooa;

   /* setup for 3,5, or 7 sequential reg writes based on vertex format */
   switch (vertsize) {
   case 6:
      LE32_OUT( &vb[vbidx++], (4 << 16) | ADRINDEX(MACH64_VERTEX_1_W) );
      break;
   case 4:
      LE32_OUT( &vb[vbidx++], (2 << 16) | ADRINDEX(MACH64_VERTEX_1_Z) );
      break;
   default: /* vertsize >= 8 */
      LE32_OUT( &vb[vbidx++], (6 << 16) | ADRINDEX(MACH64_VERTEX_1_S) );
      break;
   }
   if (vertsize > 6) {
      LE32_OUT( &vb[vbidx++], v1->ui[6] ); /* MACH64_VERTEX_1_S */
      LE32_OUT( &vb[vbidx++], v1->ui[7] ); /* MACH64_VERTEX_1_T */
   }
   if (vertsize > 4) {
      LE32_OUT( &vb[vbidx++], v1->ui[3] ); /* MACH64_VERTEX_1_W */
      LE32_OUT( &vb[vbidx++], v1->ui[5] ); /* MACH64_VERTEX_1_SPEC_ARGB */
   }
   LE32_OUT( &vb[vbidx++], ((GLint)(v1->v.z) << 15) );         /* MACH64_VERTEX_1_Z */
   vb[vbidx++] = v1->ui[coloridx];                             /* MACH64_VERTEX_1_ARGB */
   LE32_OUT( &vb[vbidx++], (xx[0] << 16) | (yy[0] & 0xffff) ); /* MACH64_VERTEX_1_X_Y */

   LE32_OUT( &vb[vbidx++], ADRINDEX(MACH64_ONE_OVER_AREA_UC) );
   LE32_OUT( &vb[vbidx++], *(GLuint *)&ooa );

   assert(vbsiz == vbidx);
#endif
}

static __inline void mach64_draw_point( mach64ContextPtr mmesa,
				      mach64VertexPtr v0 )
{
#if MACH64_NATIVE_VTXFMT
   GLcontext *ctx = mmesa->glCtx;
   const GLuint vertsize = mmesa->vertex_size;
   GLint sz = (GLint)(mmesa->glCtx->Point._Size * 2.0); /* 2 fractional bits for hardware */
   GLfloat ooa;
   GLuint *pxy;
   GLuint xyold, xy;
   const GLuint xyoffset = 9;
   GLint x, y;
   unsigned vbsiz = (vertsize + (vertsize > 7 ? 2 : 1)) * 4 + 2;
   CARD32 *vb, *vbchk;

   if ( MACH64_DEBUG & DEBUG_VERBOSE_PRIMS ) {
      fprintf(stderr, "%s:\n", __FUNCTION__);
      fprintf(stderr,"Vertex 1:\n");
      mach64_print_vertex( ctx, v0 );
   }
  
   if( !sz )
      sz = 1;	/* round to the nearest supported size */
      
   pxy = &v0->ui[xyoffset];
   xyold = *pxy;
   xy = LE32_IN( &xyold );
   x = (GLshort)( xy >> 16 );
   y = (GLshort)( xy & 0xffff );
   
   ooa = 4.0 / (sz * sz);
   
   vb = (CARD32 *)mach64AllocDmaLow( mmesa, vbsiz * sizeof(CARD32) );
   vbchk = vb + vbsiz;

   LE32_OUT( pxy, (( x - sz ) << 16) | (( y - sz ) & 0xffff) );
   COPY_VERTEX( vb, vertsize, v0, 1 );
   LE32_OUT( pxy, (( x + sz ) << 16) | (( y - sz ) & 0xffff) );
   COPY_VERTEX( vb, vertsize, v0, 2 );
   LE32_OUT( pxy, (( x - sz ) << 16) | (( y + sz ) & 0xffff) );
   COPY_VERTEX_OOA( vb, vertsize, v0, 3 );
   LE32_OUT( vb++, *(CARD32 *)&ooa );

   ooa = -ooa;
   
   LE32_OUT( pxy, (( x + sz ) << 16) | (( y + sz ) & 0xffff) );
   COPY_VERTEX_OOA( vb, vertsize, v0, 1 );
   LE32_OUT( vb++, *(CARD32 *)&ooa );

   *pxy = xyold;
#else /* !MACH64_NATIVE_VTXFMT */
   GLuint vertsize = mmesa->vertex_size; 
   GLint coloridx;
   float sz = 1.0; /* Only support 1 pix points now */
   GLfloat ooa;
   GLint xx[3], yy[3]; /* 2 fractional bits for hardware */
   unsigned vbsiz = 
	 ((
	    1 +
	    (vertsize > 6 ? 2 : 0) +
	    (vertsize > 4 ? 2 : 0) +
	    3 +
	    (mmesa->multitex ? 4 : 0)
	 ) * 4 + 4);
   CARD32 *vb;
   unsigned vbidx = 0;
   
   if ( MACH64_DEBUG & DEBUG_VERBOSE_PRIMS ) {
      fprintf(stderr, "%s:\n", __FUNCTION__);
      fprintf(stderr,"Vertex 1: x: %.2f, y: %.2f, z: %.2f, w: %f\n", 
	      v0->v.x, v0->v.y, v0->v.z, v0->v.w);
   }

#if MACH64_CLIENT_STATE_EMITS
   /* Enable for interleaved client-side state emits */
   LOCK_HARDWARE( mmesa );
   if ( mmesa->dirty ) {
      mach64EmitHwStateLocked( mmesa );
   }
   if ( mmesa->sarea->dirty ) {
      mach64UploadHwStateLocked( mmesa );
   }
   UNLOCK_HARDWARE( mmesa );
#endif

   xx[0] = (GLint)((v0->v.x - sz) * 4);
   yy[0] = (GLint)((v0->v.y - sz) * 4);

   xx[1] = (GLint)((v0->v.x + sz) * 4);
   yy[1] = (GLint)((v0->v.y - sz) * 4);

   xx[2] = (GLint)((v0->v.x - sz) * 4);
   yy[2] = (GLint)((v0->v.y + sz) * 4);

   ooa = 0.25 * 0.25 * ((xx[0] - xx[2]) * (yy[1] - yy[2]) -
			(yy[0] - yy[2]) * (xx[1] - xx[2]));

   if ( ooa * mmesa->backface_sign < 0 ) {
      /* cull quad */
      if ( MACH64_DEBUG & DEBUG_VERBOSE_PRIMS )
	 fprintf(stderr,"Point culled\n");
      return;
   }

   vb = (CARD32 *)mach64AllocDmaLow( mmesa, vbsiz * 4 );
   
   ooa = 1.0 / ooa;

   coloridx = (vertsize > 4) ? 4: 3;

   /* setup for 3,5, or 7 sequential reg writes based on vertex format */
   switch (vertsize) {
   case 6:
      LE32_OUT( &vb[vbidx++], (4 << 16) | ADRINDEX(MACH64_VERTEX_1_W) );
      break;
   case 4:
      LE32_OUT( &vb[vbidx++], (2 << 16) | ADRINDEX(MACH64_VERTEX_1_Z) );
      break;
   default: /* vertsize >= 8 */
      LE32_OUT( &vb[vbidx++], (6 << 16) | ADRINDEX(MACH64_VERTEX_1_S) );
      break;
   }
   if (vertsize > 6) {
      LE32_OUT( &vb[vbidx++], v0->ui[6] ); /* MACH64_VERTEX_1_S */
      LE32_OUT( &vb[vbidx++], v0->ui[7] ); /* MACH64_VERTEX_1_T */
   }
   if (vertsize > 4) {
      LE32_OUT( &vb[vbidx++], v0->ui[3] ); /* MACH64_VERTEX_1_W */
      LE32_OUT( &vb[vbidx++], v0->ui[5] ); /* MACH64_VERTEX_1_SPEC_ARGB */
   }
   LE32_OUT( &vb[vbidx++], ((GLint)(v0->v.z) << 15) );         /* MACH64_VERTEX_1_Z */
   vb[vbidx++] = v0->ui[coloridx];                             /* MACH64_VERTEX_1_ARGB */
   LE32_OUT( &vb[vbidx++], (xx[0] << 16) | (yy[0] & 0xffff) ); /* MACH64_VERTEX_1_X_Y */

   /* setup for 3,5, or 7 sequential reg writes based on vertex format */
   switch (vertsize) {
   case 6:
      LE32_OUT( &vb[vbidx++], (4 << 16) | ADRINDEX(MACH64_VERTEX_2_W) );
      break;
   case 4:
      LE32_OUT( &vb[vbidx++], (2 << 16) | ADRINDEX(MACH64_VERTEX_2_Z) );
      break;
   default: /* vertsize >= 8 */
      LE32_OUT( &vb[vbidx++], (6 << 16) | ADRINDEX(MACH64_VERTEX_2_S) );
      break;
   }
   if (vertsize > 6) {
      LE32_OUT( &vb[vbidx++], v0->ui[6] ); /* MACH64_VERTEX_2_S */
      LE32_OUT( &vb[vbidx++], v0->ui[7] ); /* MACH64_VERTEX_2_T */
   }
   if (vertsize > 4) {
      LE32_OUT( &vb[vbidx++], v0->ui[3] ); /* MACH64_VERTEX_2_W */
      LE32_OUT( &vb[vbidx++], v0->ui[5] ); /* MACH64_VERTEX_2_SPEC_ARGB */
   }
   LE32_OUT( &vb[vbidx++], ((GLint)(v0->v.z) << 15) );         /* MACH64_VERTEX_2_Z */
   vb[vbidx++] = v0->ui[coloridx];                             /* MACH64_VERTEX_2_ARGB */
   LE32_OUT( &vb[vbidx++], (xx[1] << 16) | (yy[1] & 0xffff) ); /* MACH64_VERTEX_2_X_Y */

   /* setup for 3,5, or 7 sequential reg writes based on vertex format */
   switch (vertsize) {
   case 6:
      LE32_OUT( &vb[vbidx++], (4 << 16) | ADRINDEX(MACH64_VERTEX_3_W) );
      break;
   case 4:
      LE32_OUT( &vb[vbidx++], (2 << 16) | ADRINDEX(MACH64_VERTEX_3_Z) );
      break;
   default: /* vertsize >= 8 */
      LE32_OUT( &vb[vbidx++], (6 << 16) | ADRINDEX(MACH64_VERTEX_3_S) );
      break;
   }
   if (vertsize > 6) {
      LE32_OUT( &vb[vbidx++], v0->ui[6] ); /* MACH64_VERTEX_3_S */
      LE32_OUT( &vb[vbidx++], v0->ui[7] ); /* MACH64_VERTEX_3_T */
   }
   if (vertsize > 4) {
      LE32_OUT( &vb[vbidx++], v0->ui[3] ); /* MACH64_VERTEX_3_W */
      LE32_OUT( &vb[vbidx++], v0->ui[5] ); /* MACH64_VERTEX_3_SPEC_ARGB */
   }
   LE32_OUT( &vb[vbidx++], ((GLint)(v0->v.z) << 15) );         /* MACH64_VERTEX_3_Z */
   vb[vbidx++] = v0->ui[coloridx];                             /* MACH64_VERTEX_3_ARGB */
   LE32_OUT( &vb[vbidx++], (xx[2] << 16) | (yy[2] & 0xffff) ); /* MACH64_VERTEX_3_X_Y */

   LE32_OUT( &vb[vbidx++], ADRINDEX(MACH64_ONE_OVER_AREA_UC) );
   LE32_OUT( &vb[vbidx++], *(GLuint *)&ooa );

   xx[0] = (GLint)((v0->v.x + sz) * 4);
   yy[0] = (GLint)((v0->v.y + sz) * 4);

   ooa = 0.25 * 0.25 * ((xx[0] - xx[2]) * (yy[1] - yy[2]) -
			(yy[0] - yy[2]) * (xx[1] - xx[2]));
   ooa = 1.0 / ooa;

   /* setup for 3,5, or 7 sequential reg writes based on vertex format */
   switch (vertsize) {
   case 6:
      LE32_OUT( &vb[vbidx++], (4 << 16) | ADRINDEX(MACH64_VERTEX_1_W) );
      break;
   case 4:
      LE32_OUT( &vb[vbidx++], (2 << 16) | ADRINDEX(MACH64_VERTEX_1_Z) );
      break;
   default: /* vertsize >= 8 */
      LE32_OUT( &vb[vbidx++], (6 << 16) | ADRINDEX(MACH64_VERTEX_1_S) );
      break;
   }
   if (vertsize > 6) {
      LE32_OUT( &vb[vbidx++], v0->ui[6] ); /* MACH64_VERTEX_1_S */
      LE32_OUT( &vb[vbidx++], v0->ui[7] ); /* MACH64_VERTEX_1_T */
   }
   if (vertsize > 4) {
      LE32_OUT( &vb[vbidx++], v0->ui[3] ); /* MACH64_VERTEX_1_W */
      LE32_OUT( &vb[vbidx++], v0->ui[5] ); /* MACH64_VERTEX_1_SPEC_ARGB */
   }
   LE32_OUT( &vb[vbidx++], ((GLint)(v0->v.z) << 15) );         /* MACH64_VERTEX_1_Z */
   vb[vbidx++] = v0->ui[coloridx];                             /* MACH64_VERTEX_1_ARGB */
   LE32_OUT( &vb[vbidx++], (xx[0] << 16) | (yy[0] & 0xffff) ); /* MACH64_VERTEX_1_X_Y */

   LE32_OUT( &vb[vbidx++], ADRINDEX(MACH64_ONE_OVER_AREA_UC) );
   LE32_OUT( &vb[vbidx++], *(GLuint *)&ooa );

   assert(vbsiz == vbidx);
#endif
}

/***********************************************************************
 *          Macros for t_dd_tritmp.h to draw basic primitives          *
 ***********************************************************************/

#define TRI( a, b, c )				\
do {						\
   if (DO_FALLBACK)				\
      mmesa->draw_tri( mmesa, a, b, c );	\
   else						\
      mach64_draw_triangle( mmesa, a, b, c );	\
} while (0)

#define QUAD( a, b, c, d )			\
do {						\
   if (DO_FALLBACK) {				\
      mmesa->draw_tri( mmesa, a, b, d );	\
      mmesa->draw_tri( mmesa, b, c, d );	\
   } else 					\
      mach64_draw_quad( mmesa, a, b, c, d );	\
} while (0)

#define LINE( v0, v1 )				\
do {						\
   if (DO_FALLBACK)				\
      mmesa->draw_line( mmesa, v0, v1 );	\
   else 					\
      mach64_draw_line( mmesa, v0, v1 );	\
} while (0)

#define POINT( v0 )				\
do {						\
   if (DO_FALLBACK)				\
      mmesa->draw_point( mmesa, v0 );		\
   else 					\
      mach64_draw_point( mmesa, v0 );		\
} while (0)


/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/

#define MACH64_OFFSET_BIT	0x01
#define MACH64_TWOSIDE_BIT	0x02
#define MACH64_UNFILLED_BIT	0x04
#define MACH64_FALLBACK_BIT	0x08
#define MACH64_MAX_TRIFUNC	0x10

static struct {
   tnl_points_func	        points;
   tnl_line_func		line;
   tnl_triangle_func	triangle;
   tnl_quad_func		quad;
} rast_tab[MACH64_MAX_TRIFUNC];


#define DO_FALLBACK (IND & MACH64_FALLBACK_BIT)
#define DO_OFFSET   (IND & MACH64_OFFSET_BIT)
#define DO_UNFILLED (IND & MACH64_UNFILLED_BIT)
#define DO_TWOSIDE  (IND & MACH64_TWOSIDE_BIT)
#define DO_FLAT      0
#define DO_TRI       1
#define DO_QUAD      1
#define DO_LINE      1
#define DO_POINTS    1
#define DO_FULL_QUAD 1

#define HAVE_RGBA   1
#define HAVE_SPEC   1
#define HAVE_BACK_COLORS  0
#define HAVE_HW_FLATSHADE 1
#define VERTEX mach64Vertex
#define TAB rast_tab

#if MACH64_NATIVE_VTXFMT

/* #define DEPTH_SCALE 65536.0 */
#define DEPTH_SCALE 1
#define UNFILLED_TRI unfilled_tri
#define UNFILLED_QUAD unfilled_quad
#define VERT_X(_v) ((GLfloat)(GLshort)(LE32_IN( &(_v)->ui[xyoffset] ) & 0xffff) / 4.0)
#define VERT_Y(_v) ((GLfloat)(GLshort)(LE32_IN( &(_v)->ui[xyoffset] ) >> 16) / 4.0)
#define VERT_Z(_v) ((GLfloat) LE32_IN( &(_v)->ui[zoffset] ))
#define INSANE_VERTICES
#define VERT_SET_Z(_v,val) LE32_OUT( &(_v)->ui[zoffset], (GLuint)(val) )
#define VERT_Z_ADD(_v,val) LE32_OUT( &(_v)->ui[zoffset], LE32_IN( &(_v)->ui[zoffset] ) + (GLuint)(val) )
#define AREA_IS_CCW( a ) ((a) < 0)
#define GET_VERTEX(e) (mmesa->verts + ((e) * mmesa->vertex_size * sizeof(int)))

#define MACH64_COLOR( dst, src )                \
do {						\
   UNCLAMPED_FLOAT_TO_UBYTE(dst[0], src[2]);	\
   UNCLAMPED_FLOAT_TO_UBYTE(dst[1], src[1]);				\
   UNCLAMPED_FLOAT_TO_UBYTE(dst[2], src[0]);				\
   UNCLAMPED_FLOAT_TO_UBYTE(dst[3], src[3]);				\
} while (0)

#define MACH64_SPEC( dst, src )			\
do {						\
   UNCLAMPED_FLOAT_TO_UBYTE(dst[0], src[2]);	\
   UNCLAMPED_FLOAT_TO_UBYTE(dst[1], src[1]);	\
   UNCLAMPED_FLOAT_TO_UBYTE(dst[2], src[0]);	\
} while (0)

#define VERT_SET_RGBA( v, c )    MACH64_COLOR( v->ub4[coloroffset], c )
#define VERT_COPY_RGBA( v0, v1 ) v0->ui[coloroffset] = v1->ui[coloroffset]
#define VERT_SAVE_RGBA( idx )    color[idx] = v[idx]->ui[coloroffset]
#define VERT_RESTORE_RGBA( idx ) v[idx]->ui[coloroffset] = color[idx]

#define VERT_SET_SPEC( v, c )    if (havespec) MACH64_SPEC( v->ub4[specoffset], c )
#define VERT_COPY_SPEC( v0, v1 ) if (havespec) COPY_3V( v0->ub4[specoffset], v1->ub4[specoffset] )
#define VERT_SAVE_SPEC( idx )    if (havespec) spec[idx] = v[idx]->ui[specoffset]
#define VERT_RESTORE_SPEC( idx ) if (havespec) v[idx]->ui[specoffset] = spec[idx]

#define LOCAL_VARS(n)						\
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);		\
   GLuint color[n], spec[n];					\
   GLuint vertex_size = mmesa->vertex_size;			\
   const GLuint xyoffset = 9;					\
   const GLuint coloroffset = 8;				\
   const GLuint zoffset = 7;					\
   const GLuint specoffset = 6;					\
   GLboolean havespec = vertex_size >= 4 ? 1 : 0;		\
   (void) color; (void) spec; (void) vertex_size; 		\
   (void) xyoffset; (void) coloroffset; (void) zoffset;		\
   (void) specoffset; (void) havespec;

#else

#define DEPTH_SCALE 1.0
#define UNFILLED_TRI unfilled_tri
#define UNFILLED_QUAD unfilled_quad
#define VERT_X(_v) _v->v.x
#define VERT_Y(_v) _v->v.y
#define VERT_Z(_v) _v->v.z
#define AREA_IS_CCW( a ) (a > 0)
#define GET_VERTEX(e) (mmesa->verts + ((e) * mmesa->vertex_size * sizeof(int)))

#define MACH64_COLOR( dst, src )                \
do {						\
   UNCLAMPED_FLOAT_TO_UBYTE(dst[0], src[2]);	\
   UNCLAMPED_FLOAT_TO_UBYTE(dst[1], src[1]);				\
   UNCLAMPED_FLOAT_TO_UBYTE(dst[2], src[0]);				\
   UNCLAMPED_FLOAT_TO_UBYTE(dst[3], src[3]);				\
} while (0)

#define MACH64_SPEC( dst, src )			\
do {						\
   UNCLAMPED_FLOAT_TO_UBYTE(dst[0], src[2]);	\
   UNCLAMPED_FLOAT_TO_UBYTE(dst[1], src[1]);	\
   UNCLAMPED_FLOAT_TO_UBYTE(dst[2], src[0]);	\
} while (0)

#define VERT_SET_RGBA( v, c )    MACH64_COLOR( v->ub4[coloroffset], c )
#define VERT_COPY_RGBA( v0, v1 ) v0->ui[coloroffset] = v1->ui[coloroffset]
#define VERT_SAVE_RGBA( idx )    color[idx] = v[idx]->ui[coloroffset]
#define VERT_RESTORE_RGBA( idx ) v[idx]->ui[coloroffset] = color[idx]

#define VERT_SET_SPEC( v, c )    if (havespec) MACH64_SPEC( v->ub4[5], c )
#define VERT_COPY_SPEC( v0, v1 ) if (havespec) COPY_3V(v0->ub4[5], v1->ub4[5])
#define VERT_SAVE_SPEC( idx )    if (havespec) spec[idx] = v[idx]->ui[5]
#define VERT_RESTORE_SPEC( idx ) if (havespec) v[idx]->ui[5] = spec[idx]

#define LOCAL_VARS(n)						\
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);		\
   GLuint color[n], spec[n];					\
   GLuint coloroffset = (mmesa->vertex_size == 4 ? 3 : 4);	\
   GLboolean havespec = (mmesa->vertex_size == 4 ? 0 : 1);	\
   (void) color; (void) spec; (void) coloroffset; (void) havespec;

#endif

/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/

#define RASTERIZE(x) if (mmesa->hw_primitive != hw_prim[x]) \
                        mach64RasterPrimitive( ctx, hw_prim[x] )
#define RENDER_PRIMITIVE mmesa->render_primitive
#define IND MACH64_FALLBACK_BIT
#define TAG(x) x
#include "tnl_dd/t_dd_unfilled.h"
#undef IND


/***********************************************************************
 *                      Generate GL render functions                   *
 ***********************************************************************/


#define IND (0)
#define TAG(x) x
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MACH64_OFFSET_BIT)
#define TAG(x) x##_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MACH64_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MACH64_TWOSIDE_BIT|MACH64_OFFSET_BIT)
#define TAG(x) x##_twoside_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MACH64_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MACH64_OFFSET_BIT|MACH64_UNFILLED_BIT)
#define TAG(x) x##_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MACH64_TWOSIDE_BIT|MACH64_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MACH64_TWOSIDE_BIT|MACH64_OFFSET_BIT|MACH64_UNFILLED_BIT)
#define TAG(x) x##_twoside_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MACH64_FALLBACK_BIT)
#define TAG(x) x##_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MACH64_OFFSET_BIT|MACH64_FALLBACK_BIT)
#define TAG(x) x##_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MACH64_TWOSIDE_BIT|MACH64_FALLBACK_BIT)
#define TAG(x) x##_twoside_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MACH64_TWOSIDE_BIT|MACH64_OFFSET_BIT|MACH64_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MACH64_UNFILLED_BIT|MACH64_FALLBACK_BIT)
#define TAG(x) x##_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MACH64_OFFSET_BIT|MACH64_UNFILLED_BIT|MACH64_FALLBACK_BIT)
#define TAG(x) x##_offset_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MACH64_TWOSIDE_BIT|MACH64_UNFILLED_BIT|MACH64_FALLBACK_BIT)
#define TAG(x) x##_twoside_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (MACH64_TWOSIDE_BIT|MACH64_OFFSET_BIT|MACH64_UNFILLED_BIT| \
	     MACH64_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"


static void init_rast_tab( void )
{
   init();
   init_offset();
   init_twoside();
   init_twoside_offset();
   init_unfilled();
   init_offset_unfilled();
   init_twoside_unfilled();
   init_twoside_offset_unfilled();
   init_fallback();
   init_offset_fallback();
   init_twoside_fallback();
   init_twoside_offset_fallback();
   init_unfilled_fallback();
   init_offset_unfilled_fallback();
   init_twoside_unfilled_fallback();
   init_twoside_offset_unfilled_fallback();
}


/***********************************************************************
 *                    Rasterization fallback helpers                   *
 ***********************************************************************/


/* This code is hit only when a mix of accelerated and unaccelerated
 * primitives are being drawn, and only for the unaccelerated
 * primitives.
 */
static void
mach64_fallback_tri( mach64ContextPtr mmesa,
		     mach64Vertex *v0,
		     mach64Vertex *v1,
		     mach64Vertex *v2 )
{
   GLcontext *ctx = mmesa->glCtx;
   SWvertex v[3];
   mach64_translate_vertex( ctx, v0, &v[0] );
   mach64_translate_vertex( ctx, v1, &v[1] );
   mach64_translate_vertex( ctx, v2, &v[2] );
   _swrast_Triangle( ctx, &v[0], &v[1], &v[2] );
}


static void
mach64_fallback_line( mach64ContextPtr mmesa,
		    mach64Vertex *v0,
		    mach64Vertex *v1 )
{
   GLcontext *ctx = mmesa->glCtx;
   SWvertex v[2];
   mach64_translate_vertex( ctx, v0, &v[0] );
   mach64_translate_vertex( ctx, v1, &v[1] );
   _swrast_Line( ctx, &v[0], &v[1] );
}


static void
mach64_fallback_point( mach64ContextPtr mmesa,
		     mach64Vertex *v0 )
{
   GLcontext *ctx = mmesa->glCtx;
   SWvertex v[1];
   mach64_translate_vertex( ctx, v0, &v[0] );
   _swrast_Point( ctx, &v[0] );
}



/**********************************************************************/
/*               Render unclipped begin/end objects                   */
/**********************************************************************/

#define VERT(x) (mach64Vertex *)(mach64verts + ((x) * vertsize * sizeof(int)))
#define RENDER_POINTS( start, count )		\
   for ( ; start < count ; start++)		\
      mach64_draw_point( mmesa, VERT(start) )
#define RENDER_LINE( v0, v1 ) \
   mach64_draw_line( mmesa, VERT(v0), VERT(v1) )
#define RENDER_TRI( v0, v1, v2 )  \
   mach64_draw_triangle( mmesa, VERT(v0), VERT(v1), VERT(v2) )
#define RENDER_QUAD( v0, v1, v2, v3 ) \
   mach64_draw_quad( mmesa, VERT(v0), VERT(v1), VERT(v2), VERT(v3) )
#define INIT(x) do {					\
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);	\
   mach64RenderPrimitive( ctx, x );			\
} while (0)
#undef LOCAL_VARS
#define LOCAL_VARS						\
    mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);		\
    const GLuint vertsize = mmesa->vertex_size;                 \
    const char *mach64verts = (char *)mmesa->verts;		\
    const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;	\
    (void) elt;
#define RESET_STIPPLE
#define RESET_OCCLUSION
#define PRESERVE_VB_DEFS
#define ELT(x) (x)
#define TAG(x) mach64_##x##_verts
#include "tnl/t_vb_rendertmp.h"
#undef ELT
#undef TAG
#define TAG(x) mach64_##x##_elts
#define ELT(x) elt[x]
#include "tnl/t_vb_rendertmp.h"


/**********************************************************************/
/*                    Render clipped primitives                       */
/**********************************************************************/

static void mach64RenderClippedPoly( GLcontext *ctx, const GLuint *elts,
				     GLuint n )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT( ctx );
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLuint prim = mmesa->render_primitive;

   /* Render the new vertices as an unclipped polygon.
    */
   {
      GLuint *tmp = VB->Elts;
      VB->Elts = (GLuint *)elts;
      tnl->Driver.Render.PrimTabElts[GL_POLYGON]( ctx, 0, n, PRIM_BEGIN|PRIM_END );
      VB->Elts = tmp;
   }

   /* Restore the render primitive
    */
   if (prim != GL_POLYGON)
      tnl->Driver.Render.PrimitiveNotify( ctx, prim );

}

static void mach64RenderClippedLine( GLcontext *ctx, GLuint ii, GLuint jj )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   tnl->Driver.Render.Line( ctx, ii, jj );
}

#if MACH64_NATIVE_VTXFMT
static void mach64FastRenderClippedPoly( GLcontext *ctx, const GLuint *elts,
					 GLuint n )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT( ctx );
   const GLuint vertsize = mmesa->vertex_size;
   GLint a;
   union {
      GLfloat f;
      CARD32 u;
   } ooa;
   GLuint xy;
   const GLuint xyoffset = 9;
   GLint xx[3], yy[3]; /* 2 fractional bits for hardware */
   unsigned vbsiz = (vertsize + (vertsize > 7 ? 2 : 1)) * n + (n-2);
   CARD32 *vb, *vbchk;
   GLubyte *mach64verts = (GLubyte *)mmesa->verts;
   mach64VertexPtr v0, v1, v2;
   int i;
   
   v0 = (mach64VertexPtr)VERT(elts[1]);
   v1 = (mach64VertexPtr)VERT(elts[2]);
   v2 = (mach64VertexPtr)VERT(elts[0]);
      
   xy = LE32_IN( &v0->ui[xyoffset] );
   xx[0] = (GLshort)( xy >> 16 );
   yy[0] = (GLshort)( xy & 0xffff );
   
   xy = LE32_IN( &v1->ui[xyoffset] );
   xx[1] = (GLshort)( xy >> 16 );
   yy[1] = (GLshort)( xy & 0xffff );
   
   xy = LE32_IN( &v2->ui[xyoffset] );
   xx[2] = (GLshort)( xy >> 16 );
   yy[2] = (GLshort)( xy & 0xffff );
	   
   a = (xx[0] - xx[2]) * (yy[1] - yy[2]) -
       (yy[0] - yy[2]) * (xx[1] - xx[2]);

   if ( (mmesa->backface_sign &&
	((a < 0 && !signbit( mmesa->backface_sign )) || 
	(a > 0 && signbit( mmesa->backface_sign )))) ) {
      /* cull polygon */
      if ( MACH64_DEBUG & DEBUG_VERBOSE_PRIMS )
	 fprintf(stderr,"Polygon culled\n");
      return;
   }
   
   ooa.f = 16.0 / a;
   
   vb = (CARD32 *)mach64AllocDmaLow( mmesa, vbsiz * sizeof(CARD32) );
   vbchk = vb + vbsiz;

   COPY_VERTEX( vb, vertsize, v0, 1 );
   COPY_VERTEX( vb, vertsize, v1, 2 );
   COPY_VERTEX_OOA( vb, vertsize, v2, 3 );
   LE32_OUT( vb++, ooa.u );

   i = 3;
   while (1) {
      if (i >= n)
	 break;
      v0 = (mach64VertexPtr)VERT(elts[i]);
      i++;

      xy = LE32_IN( &v0->ui[xyoffset] );
      xx[0] = (GLshort)( xy >> 16 );
      yy[0] = (GLshort)( xy & 0xffff );
	      
      a = (xx[0] - xx[2]) * (yy[1] - yy[2]) -
	  (yy[0] - yy[2]) * (xx[1] - xx[2]);
      ooa.f = 16.0 / a;
   
      COPY_VERTEX_OOA( vb, vertsize, v0, 1 );
      LE32_OUT( vb++, ooa.u );
      
      if (i >= n)
	 break;
      v1 = (mach64VertexPtr)VERT(elts[i]);
      i++;

      xy = LE32_IN( &v1->ui[xyoffset] );
      xx[1] = (GLshort)( xy >> 16 );
      yy[1] = (GLshort)( xy & 0xffff );
	      
      a = (xx[0] - xx[2]) * (yy[1] - yy[2]) -
	  (yy[0] - yy[2]) * (xx[1] - xx[2]);
      ooa.f = 16.0 / a;
   
      COPY_VERTEX_OOA( vb, vertsize, v1, 2 );
      LE32_OUT( vb++, ooa.u );
   }

   assert( vb == vbchk );
}
#else
static void mach64FastRenderClippedPoly( GLcontext *ctx, const GLuint *elts,
					 GLuint n )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT( ctx );
   const GLuint vertsize = mmesa->vertex_size;
   GLubyte *mach64verts = (GLubyte *)mmesa->verts;
   const GLuint *start = (const GLuint *)VERT(elts[0]);
   int i;

   for (i = 2 ; i < n ; i++) {
      mach64_draw_triangle( mmesa, 
			    VERT(elts[i-1]), 
			    VERT(elts[i]), 
			    (mach64VertexPtr) start
			    );
   }
}
#endif /* MACH64_NATIVE_VTXFMT */

/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/

#define _MACH64_NEW_RENDER_STATE (_DD_NEW_POINT_SMOOTH |	\
			          _DD_NEW_LINE_SMOOTH |		\
			          _DD_NEW_LINE_STIPPLE |	\
			          _DD_NEW_TRI_SMOOTH |		\
			          _DD_NEW_TRI_STIPPLE |		\
			          _NEW_POLYGONSTIPPLE |		\
			          _DD_NEW_TRI_UNFILLED |	\
			          _DD_NEW_TRI_LIGHT_TWOSIDE |	\
			          _DD_NEW_TRI_OFFSET)		\

#define POINT_FALLBACK (DD_POINT_SMOOTH)
#define LINE_FALLBACK (DD_LINE_SMOOTH|DD_LINE_STIPPLE)
#define TRI_FALLBACK (DD_TRI_SMOOTH|DD_TRI_STIPPLE)
#define ANY_FALLBACK_FLAGS (POINT_FALLBACK|LINE_FALLBACK|TRI_FALLBACK)
#define ANY_RASTER_FLAGS (DD_TRI_LIGHT_TWOSIDE|DD_TRI_OFFSET|DD_TRI_UNFILLED)


static void mach64ChooseRenderState(GLcontext *ctx)
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   GLuint flags = ctx->_TriangleCaps;
   GLuint index = 0;

   if (flags & (ANY_RASTER_FLAGS|ANY_FALLBACK_FLAGS)) {
      mmesa->draw_point = mach64_draw_point;
      mmesa->draw_line = mach64_draw_line;
      mmesa->draw_tri = mach64_draw_triangle;

      if (flags & ANY_RASTER_FLAGS) {
	 if (flags & DD_TRI_LIGHT_TWOSIDE) index |= MACH64_TWOSIDE_BIT;
	 if (flags & DD_TRI_OFFSET)        index |= MACH64_OFFSET_BIT;
	 if (flags & DD_TRI_UNFILLED)      index |= MACH64_UNFILLED_BIT;
      }

      /* Hook in fallbacks for specific primitives.
       */
      if (flags & (POINT_FALLBACK|LINE_FALLBACK|TRI_FALLBACK)) {
	 if (flags & POINT_FALLBACK) mmesa->draw_point = mach64_fallback_point;
	 if (flags & LINE_FALLBACK)  mmesa->draw_line = mach64_fallback_line;
	 if (flags & TRI_FALLBACK)   mmesa->draw_tri = mach64_fallback_tri;
	 index |= MACH64_FALLBACK_BIT;
      }
   }

   if (index != mmesa->RenderIndex) {
      TNLcontext *tnl = TNL_CONTEXT(ctx);
      tnl->Driver.Render.Points = rast_tab[index].points;
      tnl->Driver.Render.Line = rast_tab[index].line;
      tnl->Driver.Render.Triangle = rast_tab[index].triangle;
      tnl->Driver.Render.Quad = rast_tab[index].quad;

      if (index == 0) {
	 tnl->Driver.Render.PrimTabVerts = mach64_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = mach64_render_tab_elts;
	 tnl->Driver.Render.ClippedLine = rast_tab[index].line;
	 tnl->Driver.Render.ClippedPolygon = mach64FastRenderClippedPoly;
      } else {
	 tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
	 tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
	 tnl->Driver.Render.ClippedLine = mach64RenderClippedLine;
	 tnl->Driver.Render.ClippedPolygon = mach64RenderClippedPoly;
      }

      mmesa->RenderIndex = index;
   }
}

/**********************************************************************/
/*                 Validate state at pipeline start                   */
/**********************************************************************/

static void mach64RunPipeline( GLcontext *ctx )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);

   if (mmesa->new_state)
      mach64DDUpdateHWState( ctx );

   if (!mmesa->Fallback && mmesa->NewGLState) {
      if (mmesa->NewGLState & _MACH64_NEW_VERTEX_STATE)
	 mach64ChooseVertexState( ctx );

      if (mmesa->NewGLState & _MACH64_NEW_RENDER_STATE)
	 mach64ChooseRenderState( ctx );

      mmesa->NewGLState = 0;
   }

   _tnl_run_pipeline( ctx );
}

/**********************************************************************/
/*                 High level hooks for t_vb_render.c                 */
/**********************************************************************/

/* This is called when Mesa switches between rendering triangle
 * primitives (such as GL_POLYGON, GL_QUADS, GL_TRIANGLE_STRIP, etc),
 * and lines, points and bitmaps.
 */

static void mach64RasterPrimitive( GLcontext *ctx, GLuint hwprim )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);

   mmesa->new_state |= MACH64_NEW_CONTEXT;
   mmesa->dirty |= MACH64_UPLOAD_CONTEXT;

   if (mmesa->hw_primitive != hwprim) {
      FLUSH_BATCH( mmesa );
      mmesa->hw_primitive = hwprim;
   }
}

static void mach64RenderPrimitive( GLcontext *ctx, GLenum prim )
{
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   GLuint hw = hw_prim[prim];

   mmesa->render_primitive = prim;

   if (prim >= GL_TRIANGLES && (ctx->_TriangleCaps & DD_TRI_UNFILLED))
      return;

   mach64RasterPrimitive( ctx, hw );
}


static void mach64RenderStart( GLcontext *ctx )
{
   /* Check for projective texturing.  Make sure all texcoord
    * pointers point to something.  (fix in mesa?)
    */
   mach64CheckTexSizes( ctx );
}

static void mach64RenderFinish( GLcontext *ctx )
{
   if (MACH64_CONTEXT(ctx)->RenderIndex & MACH64_FALLBACK_BIT)
      _swrast_flush( ctx );
}


/**********************************************************************/
/*           Transition to/from hardware rasterization.               */
/**********************************************************************/

static const char * const fallbackStrings[] = {
   "Texture mode",
   "glDrawBuffer(GL_FRONT_AND_BACK)",
   "glReadBuffer",
   "glEnable(GL_STENCIL) without hw stencil buffer",
   "glRenderMode(selection or feedback)",
   "glLogicOp (mode != GL_COPY)",
   "GL_SEPARATE_SPECULAR_COLOR",
   "glBlendEquation (mode != ADD)",
   "glBlendFunc",
   "Rasterization disable",
};


static const char *getFallbackString(GLuint bit)
{
   int i = 0;
   while (bit > 1) {
      i++;
      bit >>= 1;
   }
   return fallbackStrings[i];
}

void mach64Fallback( GLcontext *ctx, GLuint bit, GLboolean mode )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   mach64ContextPtr mmesa = MACH64_CONTEXT(ctx);
   GLuint oldfallback = mmesa->Fallback;

   if (mode) {
      mmesa->Fallback |= bit;
      if (oldfallback == 0) {
	 FLUSH_BATCH( mmesa );
	 _swsetup_Wakeup( ctx );
	 mmesa->RenderIndex = ~0;
	 if (MACH64_DEBUG & DEBUG_VERBOSE_FALLBACK) {
	    fprintf(stderr, "Mach64 begin rasterization fallback: 0x%x %s\n",
		    bit, getFallbackString(bit));
	 }
      }
   }
   else {
      mmesa->Fallback &= ~bit;
      if (oldfallback == bit) {
	 _swrast_flush( ctx );
	 tnl->Driver.Render.Start = mach64RenderStart;
	 tnl->Driver.Render.PrimitiveNotify = mach64RenderPrimitive;
	 tnl->Driver.Render.Finish = mach64RenderFinish;
	 tnl->Driver.Render.BuildVertices = mach64BuildVertices;
	 mmesa->NewGLState |= (_MACH64_NEW_RENDER_STATE|
			       _MACH64_NEW_VERTEX_STATE);
	 if (MACH64_DEBUG & DEBUG_VERBOSE_FALLBACK) {
	    fprintf(stderr, "Mach64 end rasterization fallback: 0x%x %s\n",
		    bit, getFallbackString(bit));
	 }
      }
   }
}

/**********************************************************************/
/*                            Initialization.                         */
/**********************************************************************/

void mach64InitTriFuncs( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   static int firsttime = 1;

   if (firsttime) {
      init_rast_tab();
      firsttime = 0;
   }

   tnl->Driver.RunPipeline = mach64RunPipeline;
   tnl->Driver.Render.Start = mach64RenderStart;
   tnl->Driver.Render.Finish = mach64RenderFinish;
   tnl->Driver.Render.PrimitiveNotify = mach64RenderPrimitive;
   tnl->Driver.Render.ResetLineStipple = _swrast_ResetLineStipple;
   tnl->Driver.Render.BuildVertices = mach64BuildVertices;
}
