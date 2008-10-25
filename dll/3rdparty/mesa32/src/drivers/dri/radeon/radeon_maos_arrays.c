/* $XFree86: xc/lib/GL/mesa/src/drv/radeon/radeon_maos_arrays.c,v 1.1 2002/10/30 12:51:55 alanh Exp $ */
/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     Tungsten Graphics Inc., Cedar Park, Texas.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "imports.h"
#include "mtypes.h"
#include "macros.h"

#include "swrast_setup/swrast_setup.h"
#include "math/m_translate.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"

#include "radeon_context.h"
#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "radeon_swtcl.h"
#include "radeon_maos.h"
#include "radeon_tcl.h"

#if 0
/* Usage:
 *   - from radeon_tcl_render
 *   - call radeonEmitArrays to ensure uptodate arrays in dma
 *   - emit primitives (new type?) which reference the data
 *       -- need to use elts for lineloop, quads, quadstrip/flat
 *       -- other primitives are all well-formed (need tristrip-1,fake-poly)
 *
 */
static void emit_ubyte_rgba3( GLcontext *ctx,
		       struct radeon_dma_region *rvb,
		       char *data,
		       int stride,
		       int count )
{
   int i;
   radeon_color_t *out = (radeon_color_t *)(rvb->start + rvb->address);

   if (RADEON_DEBUG & DEBUG_VERTS)
      fprintf(stderr, "%s count %d stride %d out %p\n",
	      __FUNCTION__, count, stride, (void *)out);

   for (i = 0; i < count; i++) {
      out->red   = *data;
      out->green = *(data+1);
      out->blue  = *(data+2);
      out->alpha = 0xFF;
      out++;
      data += stride;
   }
}

static void emit_ubyte_rgba4( GLcontext *ctx,
			      struct radeon_dma_region *rvb,
			      char *data,
			      int stride,
			      int count )
{
   int i;
   int *out = (int *)(rvb->address + rvb->start);

   if (RADEON_DEBUG & DEBUG_VERTS)
      fprintf(stderr, "%s count %d stride %d\n",
	      __FUNCTION__, count, stride);

   if (stride == 4)
       COPY_DWORDS( out, data, count );
   else
      for (i = 0; i < count; i++) {
	 *out++ = LE32_TO_CPU(*(int *)data);
	 data += stride;
      }
}


static void emit_ubyte_rgba( GLcontext *ctx,
			     struct radeon_dma_region *rvb,
			     char *data,
			     int size,
			     int stride,
			     int count )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   if (RADEON_DEBUG & DEBUG_VERTS)
      fprintf(stderr, "%s %d/%d\n", __FUNCTION__, count, size);

   assert (!rvb->buf);

   if (stride == 0) {
      radeonAllocDmaRegion( rmesa, rvb, 4, 4 );
      count = 1;
      rvb->aos_start = GET_START(rvb);
      rvb->aos_stride = 0;
      rvb->aos_size = 1;
   }
   else {
      radeonAllocDmaRegion( rmesa, rvb, 4 * count, 4 );	/* alignment? */
      rvb->aos_start = GET_START(rvb);
      rvb->aos_stride = 1;
      rvb->aos_size = 1;
   }

   /* Emit the data
    */
   switch (size) {
   case 3:
      emit_ubyte_rgba3( ctx, rvb, data, stride, count );
      break;
   case 4:
      emit_ubyte_rgba4( ctx, rvb, data, stride, count );
      break;
   default:
      assert(0);
      exit(1);
      break;
   }
}
#endif

#if defined(USE_X86_ASM)
#define COPY_DWORDS( dst, src, nr )					\
do {									\
	int __tmp;							\
	__asm__ __volatile__( "rep ; movsl"				\
			      : "=%c" (__tmp), "=D" (dst), "=S" (__tmp)	\
			      : "0" (nr),				\
			        "D" ((long)dst),			\
			        "S" ((long)src) );			\
} while (0)
#else
#define COPY_DWORDS( dst, src, nr )		\
do {						\
   int j;					\
   for ( j = 0 ; j < nr ; j++ )			\
      dst[j] = ((int *)src)[j];			\
   dst += nr;					\
} while (0)
#endif

static void emit_vecfog( GLcontext *ctx,
			 struct radeon_dma_region *rvb,
			 char *data,
			 int stride,
			 int count )
{
   int i;
   GLfloat *out;

   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   if (RADEON_DEBUG & DEBUG_VERTS)
      fprintf(stderr, "%s count %d stride %d\n",
	      __FUNCTION__, count, stride);

   assert (!rvb->buf);

   if (stride == 0) {
      radeonAllocDmaRegion( rmesa, rvb, 4, 4 );
      count = 1;
      rvb->aos_start = GET_START(rvb);
      rvb->aos_stride = 0;
      rvb->aos_size = 1;
   }
   else {
      radeonAllocDmaRegion( rmesa, rvb, count * 4, 4 );	/* alignment? */
      rvb->aos_start = GET_START(rvb);
      rvb->aos_stride = 1;
      rvb->aos_size = 1;
   }

   /* Emit the data
    */
   out = (GLfloat *)(rvb->address + rvb->start);
   for (i = 0; i < count; i++) {
      out[0] = radeonComputeFogBlendFactor( ctx, *(GLfloat *)data );
      out++;
      data += stride;
   }
}

static void emit_vec4( GLcontext *ctx,
		       struct radeon_dma_region *rvb,
		       char *data,
		       int stride,
		       int count )
{
   int i;
   int *out = (int *)(rvb->address + rvb->start);

   if (RADEON_DEBUG & DEBUG_VERTS)
      fprintf(stderr, "%s count %d stride %d\n",
	      __FUNCTION__, count, stride);

   if (stride == 4)
      COPY_DWORDS( out, data, count );
   else
      for (i = 0; i < count; i++) {
	 out[0] = *(int *)data;
	 out++;
	 data += stride;
      }
}


static void emit_vec8( GLcontext *ctx,
		       struct radeon_dma_region *rvb,
		       char *data,
		       int stride,
		       int count )
{
   int i;
   int *out = (int *)(rvb->address + rvb->start);

   if (RADEON_DEBUG & DEBUG_VERTS)
      fprintf(stderr, "%s count %d stride %d\n",
	      __FUNCTION__, count, stride);

   if (stride == 8)
      COPY_DWORDS( out, data, count*2 );
   else
      for (i = 0; i < count; i++) {
	 out[0] = *(int *)data;
	 out[1] = *(int *)(data+4);
	 out += 2;
	 data += stride;
      }
}

static void emit_vec12( GLcontext *ctx,
		       struct radeon_dma_region *rvb,
		       char *data,
		       int stride,
		       int count )
{
   int i;
   int *out = (int *)(rvb->address + rvb->start);

   if (RADEON_DEBUG & DEBUG_VERTS)
      fprintf(stderr, "%s count %d stride %d out %p data %p\n",
	      __FUNCTION__, count, stride, (void *)out, (void *)data);

   if (stride == 12)
      COPY_DWORDS( out, data, count*3 );
   else
      for (i = 0; i < count; i++) {
	 out[0] = *(int *)data;
	 out[1] = *(int *)(data+4);
	 out[2] = *(int *)(data+8);
	 out += 3;
	 data += stride;
      }
}

static void emit_vec16( GLcontext *ctx,
			struct radeon_dma_region *rvb,
			char *data,
			int stride,
			int count )
{
   int i;
   int *out = (int *)(rvb->address + rvb->start);

   if (RADEON_DEBUG & DEBUG_VERTS)
      fprintf(stderr, "%s count %d stride %d\n",
	      __FUNCTION__, count, stride);

   if (stride == 16)
      COPY_DWORDS( out, data, count*4 );
   else
      for (i = 0; i < count; i++) {
	 out[0] = *(int *)data;
	 out[1] = *(int *)(data+4);
	 out[2] = *(int *)(data+8);
	 out[3] = *(int *)(data+12);
	 out += 4;
	 data += stride;
      }
}


static void emit_vector( GLcontext *ctx,
			 struct radeon_dma_region *rvb,
			 char *data,
			 int size,
			 int stride,
			 int count )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   if (RADEON_DEBUG & DEBUG_VERTS)
      fprintf(stderr, "%s count %d size %d stride %d\n",
	      __FUNCTION__, count, size, stride);

   assert (!rvb->buf);

   if (stride == 0) {
      radeonAllocDmaRegion( rmesa, rvb, size * 4, 4 );
      count = 1;
      rvb->aos_start = GET_START(rvb);
      rvb->aos_stride = 0;
      rvb->aos_size = size;
   }
   else {
      radeonAllocDmaRegion( rmesa, rvb, size * count * 4, 4 );	/* alignment? */
      rvb->aos_start = GET_START(rvb);
      rvb->aos_stride = size;
      rvb->aos_size = size;
   }

   /* Emit the data
    */
   switch (size) {
   case 1:
      emit_vec4( ctx, rvb, data, stride, count );
      break;
   case 2:
      emit_vec8( ctx, rvb, data, stride, count );
      break;
   case 3:
      emit_vec12( ctx, rvb, data, stride, count );
      break;
   case 4:
      emit_vec16( ctx, rvb, data, stride, count );
      break;
   default:
      assert(0);
      exit(1);
      break;
   }

}



static void emit_s0_vec( GLcontext *ctx,
			 struct radeon_dma_region *rvb,
			 char *data,
			 int stride,
			 int count )
{
   int i;
   int *out = (int *)(rvb->address + rvb->start);

   if (RADEON_DEBUG & DEBUG_VERTS)
      fprintf(stderr, "%s count %d stride %d\n",
	      __FUNCTION__, count, stride);

   for (i = 0; i < count; i++) {
      out[0] = *(int *)data;
      out[1] = 0;
      out += 2;
      data += stride;
   }
}

static void emit_stq_vec( GLcontext *ctx,
			 struct radeon_dma_region *rvb,
			 char *data,
			 int stride,
			 int count )
{
   int i;
   int *out = (int *)(rvb->address + rvb->start);

   if (RADEON_DEBUG & DEBUG_VERTS)
      fprintf(stderr, "%s count %d stride %d\n",
	      __FUNCTION__, count, stride);

   for (i = 0; i < count; i++) {
      out[0] = *(int *)data;
      out[1] = *(int *)(data+4);
      out[2] = *(int *)(data+12);
      out += 3;
      data += stride;
   }
}




static void emit_tex_vector( GLcontext *ctx,
			     struct radeon_dma_region *rvb,
			     char *data,
			     int size,
			     int stride,
			     int count )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   int emitsize;

   if (RADEON_DEBUG & DEBUG_VERTS)
      fprintf(stderr, "%s %d/%d\n", __FUNCTION__, count, size);

   assert (!rvb->buf);

   switch (size) {
   case 4: emitsize = 3; break;
   case 3: emitsize = 3; break;
   default: emitsize = 2; break;
   }


   if (stride == 0) {
      radeonAllocDmaRegion( rmesa, rvb, 4 * emitsize, 4 );
      count = 1;
      rvb->aos_start = GET_START(rvb);
      rvb->aos_stride = 0;
      rvb->aos_size = emitsize;
   }
   else {
      radeonAllocDmaRegion( rmesa, rvb, 4 * emitsize * count, 4 );
      rvb->aos_start = GET_START(rvb);
      rvb->aos_stride = emitsize;
      rvb->aos_size = emitsize;
   }


   /* Emit the data
    */
   switch (size) {
   case 1:
      emit_s0_vec( ctx, rvb, data, stride, count ); 
      break;
   case 2:
      emit_vec8( ctx, rvb, data, stride, count );
      break;
   case 3:
      emit_vec12( ctx, rvb, data, stride, count );
      break;
   case 4:
      emit_stq_vec( ctx, rvb, data, stride, count );
      break;
   default:
      assert(0);
      exit(1);
      break;
   }
}




/* Emit any changed arrays to new GART memory, re-emit a packet to
 * update the arrays.  
 */
void radeonEmitArrays( GLcontext *ctx, GLuint inputs )
{
   radeonContextPtr rmesa = RADEON_CONTEXT( ctx );
   struct vertex_buffer *VB = &TNL_CONTEXT( ctx )->vb;
   struct radeon_dma_region **component = rmesa->tcl.aos_components;
   GLuint nr = 0;
   GLuint vfmt = 0;
   GLuint count = VB->Count;
   GLuint vtx, unit;
   
#if 0
   if (RADEON_DEBUG & DEBUG_VERTS) 
      _tnl_print_vert_flags( __FUNCTION__, inputs );
#endif

   if (1) {
      if (!rmesa->tcl.obj.buf) 
	 emit_vector( ctx, 
		      &rmesa->tcl.obj, 
		      (char *)VB->ObjPtr->data,
		      VB->ObjPtr->size,
		      VB->ObjPtr->stride,
		      count);

      switch( VB->ObjPtr->size ) {
      case 4: vfmt |= RADEON_CP_VC_FRMT_W0;
      case 3: vfmt |= RADEON_CP_VC_FRMT_Z;
      case 2: vfmt |= RADEON_CP_VC_FRMT_XY;
      default:
         break;
      }
      component[nr++] = &rmesa->tcl.obj;
   }
   

   if (inputs & VERT_BIT_NORMAL) {
      if (!rmesa->tcl.norm.buf)
	 emit_vector( ctx, 
		      &(rmesa->tcl.norm), 
		      (char *)VB->NormalPtr->data,
		      3,
		      VB->NormalPtr->stride,
		      count);

      vfmt |= RADEON_CP_VC_FRMT_N0;
      component[nr++] = &rmesa->tcl.norm;
   }

   if (inputs & VERT_BIT_COLOR0) {
      int emitsize;
      if (VB->ColorPtr[0]->size == 4 &&
	  (VB->ColorPtr[0]->stride != 0 ||
	   VB->ColorPtr[0]->data[0][3] != 1.0)) {
	 vfmt |= RADEON_CP_VC_FRMT_FPCOLOR | RADEON_CP_VC_FRMT_FPALPHA;
	 emitsize = 4;
      }

      else {
	 vfmt |= RADEON_CP_VC_FRMT_FPCOLOR;
	 emitsize = 3;
      }

      if (!rmesa->tcl.rgba.buf)
	 emit_vector( ctx,
		      &(rmesa->tcl.rgba),
		      (char *)VB->ColorPtr[0]->data,
		      emitsize,
		      VB->ColorPtr[0]->stride,
		      count);


      component[nr++] = &rmesa->tcl.rgba;
   }


   if (inputs & VERT_BIT_COLOR1) {
      if (!rmesa->tcl.spec.buf) {

	 emit_vector( ctx,
		      &rmesa->tcl.spec,
		      (char *)VB->SecondaryColorPtr[0]->data,
		      3,
		      VB->SecondaryColorPtr[0]->stride,
		      count);
      }

      vfmt |= RADEON_CP_VC_FRMT_FPSPEC;
      component[nr++] = &rmesa->tcl.spec;
   }

/* FIXME: not sure if this is correct. May need to stitch this together with
   secondary color. It seems odd that for primary color color and alpha values
   are emitted together but for secondary color not. */
   if (inputs & VERT_BIT_FOG) {
      if (!rmesa->tcl.fog.buf)
	 emit_vecfog( ctx,
		      &(rmesa->tcl.fog),
		      (char *)VB->FogCoordPtr->data,
		      VB->FogCoordPtr->stride,
		      count);

      vfmt |= RADEON_CP_VC_FRMT_FPFOG;
      component[nr++] = &rmesa->tcl.fog;
   }


   vtx = (rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] &
	  ~(RADEON_TCL_VTX_Q0|RADEON_TCL_VTX_Q1|RADEON_TCL_VTX_Q2));
      
   for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
      if (inputs & VERT_BIT_TEX(unit)) {
	 if (!rmesa->tcl.tex[unit].buf)
	    emit_tex_vector( ctx,
			     &(rmesa->tcl.tex[unit]),
			     (char *)VB->TexCoordPtr[unit]->data,
			     VB->TexCoordPtr[unit]->size,
			     VB->TexCoordPtr[unit]->stride,
			     count );

	 vfmt |= RADEON_ST_BIT(unit);
         /* assume we need the 3rd coord if texgen is active for r/q OR at least
	    3 coords are submitted. This may not be 100% correct */
         if (VB->TexCoordPtr[unit]->size >= 3) {
	    vtx |= RADEON_Q_BIT(unit);
	    vfmt |= RADEON_Q_BIT(unit);
	 }
	 if ( (ctx->Texture.Unit[unit].TexGenEnabled & (R_BIT | Q_BIT)) )
	    vtx |= RADEON_Q_BIT(unit);
	 else if ((VB->TexCoordPtr[unit]->size >= 3) &&
	          ((ctx->Texture.Unit[unit]._ReallyEnabled & (TEXTURE_CUBE_BIT)) == 0)) {
	    GLuint swaptexmatcol = (VB->TexCoordPtr[unit]->size - 3);
	    if (((rmesa->NeedTexMatrix >> unit) & 1) &&
		 (swaptexmatcol != ((rmesa->TexMatColSwap >> unit) & 1)))
	       radeonUploadTexMatrix( rmesa, unit, swaptexmatcol ) ;
	 }
	 component[nr++] = &rmesa->tcl.tex[unit];
      }
   }

   if (vtx != rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT]) {
      RADEON_STATECHANGE( rmesa, tcl );
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] = vtx;
   }

   rmesa->tcl.nr_aos_components = nr;
   rmesa->tcl.vertex_format = vfmt;
}


void radeonReleaseArrays( GLcontext *ctx, GLuint newinputs )
{
   radeonContextPtr rmesa = RADEON_CONTEXT( ctx );
   GLuint unit;

#if 0
   if (RADEON_DEBUG & DEBUG_VERTS) 
      _tnl_print_vert_flags( __FUNCTION__, newinputs );
#endif

   if (newinputs & VERT_BIT_POS) 
     radeonReleaseDmaRegion( rmesa, &rmesa->tcl.obj, __FUNCTION__ );

   if (newinputs & VERT_BIT_NORMAL) 
      radeonReleaseDmaRegion( rmesa, &rmesa->tcl.norm, __FUNCTION__ );

   if (newinputs & VERT_BIT_COLOR0) 
      radeonReleaseDmaRegion( rmesa, &rmesa->tcl.rgba, __FUNCTION__ );

   if (newinputs & VERT_BIT_COLOR1) 
      radeonReleaseDmaRegion( rmesa, &rmesa->tcl.spec, __FUNCTION__ );
      
   if (newinputs & VERT_BIT_FOG)
      radeonReleaseDmaRegion( rmesa, &rmesa->tcl.fog, __FUNCTION__ );

   for (unit = 0 ; unit < ctx->Const.MaxTextureUnits; unit++) {
      if (newinputs & VERT_BIT_TEX(unit))
         radeonReleaseDmaRegion( rmesa, &rmesa->tcl.tex[unit], __FUNCTION__ );
   }
}
