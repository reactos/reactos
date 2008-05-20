/*
 * Copyright 2001 by Alan Hourihane.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Authors:  Alan Hourihane, <alanh@tungstengraphics.com>
 *
 * 3DLabs Gamma driver.
 *
 */

#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "imports.h"
#include "mtypes.h"

#include "tnl/t_context.h"

#include "gamma_context.h"
#include "gamma_tris.h"
#include "gamma_vb.h"


/* !! Should template this eventually !! */

static void gamma_emit( GLcontext *ctx, GLuint start, GLuint end)
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLfloat (*coord)[4];
   GLuint coord_stride;
   GLfloat (*col)[4];
   GLuint col_stride;
   int i;
   GLuint tc0_stride = 0;
   GLfloat (*tc0)[4] = 0;
   GLuint tc0_size = 0;

   col = VB->ColorPtr[0]->data;
   col_stride = VB->ColorPtr[0]->stride;

   if (ctx->Texture.Unit[0]._ReallyEnabled) {
      tc0_stride = VB->TexCoordPtr[0]->stride;
      tc0 = VB->TexCoordPtr[0]->data;
      tc0_size = VB->TexCoordPtr[0]->size;
      coord = VB->ClipPtr->data;
      coord_stride = VB->ClipPtr->stride;
   } else {
      coord = VB->NdcPtr->data;
      coord_stride = VB->NdcPtr->stride;
   }

   if (ctx->Texture.Unit[0]._ReallyEnabled && tc0_size == 4) {
      for (i=start; i < end; i++) {
	    CHECK_DMA_BUFFER(gmesa, 9);
	    WRITEF(gmesa->buf, Tq4, tc0[i][3]);
	    WRITEF(gmesa->buf, Tr4, tc0[i][2]);
	    WRITEF(gmesa->buf, Tt4, tc0[i][0]);
	    WRITEF(gmesa->buf, Ts4, tc0[i][1]);
	    WRITE(gmesa->buf, PackedColor4, *(u_int32_t*)col[i]);
	    WRITEF(gmesa->buf, Vw, coord[i][3]);
	    WRITEF(gmesa->buf, Vz, coord[i][2]);
	    WRITEF(gmesa->buf, Vy, coord[i][1]);
	    WRITEF(gmesa->buf, Vx4, coord[i][0]);
      }
   } else if (ctx->Texture.Unit[0]._ReallyEnabled && tc0_size == 2) {
      for (i=start; i < end; i++) {
	    CHECK_DMA_BUFFER(gmesa, 7);
	    WRITEF(gmesa->buf, Tt2, tc0[i][0]);
	    WRITEF(gmesa->buf, Ts2, tc0[i][1]);
	    WRITE(gmesa->buf, PackedColor4, *(u_int32_t*)col[i]);
	    WRITEF(gmesa->buf, Vw, coord[i][3]);
	    WRITEF(gmesa->buf, Vz, coord[i][2]);
	    WRITEF(gmesa->buf, Vy, coord[i][1]);
	    WRITEF(gmesa->buf, Vx4, coord[i][0]);
      }
   } else {
      for (i=start; i < end; i++) {
	    CHECK_DMA_BUFFER(gmesa, 4);
	    WRITE(gmesa->buf, PackedColor4, *(u_int32_t*)col[i]);
	    WRITEF(gmesa->buf, Vz, coord[i][2]);
	    WRITEF(gmesa->buf, Vy, coord[i][1]);
	    WRITEF(gmesa->buf, Vx3, coord[i][0]);
      }
   }
}

#define HAVE_POINTS      1
#define HAVE_LINES       1
#define HAVE_LINE_STRIPS 1
#define HAVE_TRIANGLES   1
#define HAVE_TRI_STRIPS  1
#define HAVE_TRI_STRIP_1 0
#define HAVE_TRI_FANS    1
#define HAVE_QUADS       1
#define HAVE_QUAD_STRIPS 1
#define HAVE_POLYGONS    1

#define HAVE_ELTS        0


static const GLuint hw_prim[GL_POLYGON+1] = {
   B_PrimType_Points,
   B_PrimType_Lines,
   B_PrimType_LineLoop,
   B_PrimType_LineStrip,
   B_PrimType_Triangles,
   B_PrimType_TriangleStrip,
   B_PrimType_TriangleFan,
   B_PrimType_Quads,
   B_PrimType_QuadStrip,
   B_PrimType_Polygon
};

static __inline void gammaStartPrimitive( gammaContextPtr gmesa, GLenum prim )
{
   CHECK_DMA_BUFFER(gmesa, 1);
   WRITE(gmesa->buf, Begin, gmesa->Begin | hw_prim[prim]);
}

static __inline void gammaEndPrimitive( gammaContextPtr gmesa )
{
   GLcontext *ctx = gmesa->glCtx;

   if ( ctx->Line.SmoothFlag || 
        ctx->Polygon.SmoothFlag || 
        ctx->Point.SmoothFlag ) {
      CHECK_DMA_BUFFER(gmesa, 1);
      WRITE(gmesa->buf, FlushSpan, 0);
   }

   CHECK_DMA_BUFFER(gmesa, 1);
   WRITE(gmesa->buf, End, 0);
}

#define LOCAL_VARS gammaContextPtr gmesa = GAMMA_CONTEXT(ctx)
#define INIT( prim ) gammaStartPrimitive( gmesa, prim )
#define FLUSH() gammaEndPrimitive( gmesa )
#define GET_CURRENT_VB_MAX_VERTS() \
  (gmesa->bufSize - gmesa->bufCount) / 2
#define GET_SUBSEQUENT_VB_MAX_VERTS() \
  GAMMA_DMA_BUFFER_SIZE / 2

#define ALLOC_VERTS( nr ) (void *)0	/* todo: explicit alloc */
#define EMIT_VERTS( ctx, j, nr, buf ) (gamma_emit(ctx, j, (j)+(nr)), (void *)0)

#define TAG(x) gamma_##x
#include "tnl_dd/t_dd_dmatmp.h"


/**********************************************************************/
/*                          Render pipeline stage                     */
/**********************************************************************/


static GLboolean gamma_run_render( GLcontext *ctx,
				  struct tnl_pipeline_stage *stage )
{
   gammaContextPtr gmesa = GAMMA_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;
   tnl_render_func *tab;

				/* GH: THIS IS A HACK!!! */
   if (VB->ClipOrMask || gmesa->RenderIndex != 0)
      return GL_TRUE;		/* don't handle clipping here */

   /* We don't do elts */
   if (VB->Elts || !gamma_validate_render( ctx, VB ))
      return GL_TRUE;

   tab = TAG(render_tab_verts);

   tnl->Driver.Render.Start( ctx );

   for (i = 0 ; i < VB->PrimitiveCount ; i++)
   {
      GLuint prim = VB->Primitive[i].mode;
      GLuint start = VB->Primitive[i].start;
      GLuint length = VB->Primitive[i].count;

      if (!length)
	 continue;

      tab[prim & PRIM_MODE_MASK]( ctx, start, start + length, prim);
   }

   tnl->Driver.Render.Finish( ctx );

   return GL_FALSE;		/* finished the pipe */
}


const struct tnl_pipeline_stage _gamma_render_stage =
{
   "gamma render",
   NULL,
   NULL,
   NULL,
   NULL,
   gamma_run_render		/* run */
};
