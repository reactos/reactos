/* $XFree86: xc/lib/GL/mesa/src/drv/mga/mgarender.c,v 1.4 2002/10/30 12:51:36 alanh Exp $ */
/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *
 */


/*
 * Render unclipped vertex buffers by emitting vertices directly to
 * dma buffers.  Use strip/fan hardware primitives where possible.
 * Simulate missing primitives with indexed vertices.
 */
#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "imports.h"
#include "mtypes.h"

#include "tnl/t_context.h"

#include "mgacontext.h"
#include "mgatris.h"
#include "mgastate.h"
#include "mgaioctl.h"
#include "mgavb.h"

#define HAVE_POINTS      0
#define HAVE_LINES       0
#define HAVE_LINE_STRIPS 0
#define HAVE_TRIANGLES   1
#define HAVE_TRI_STRIPS  1
#define HAVE_TRI_STRIP_1 0
#define HAVE_TRI_FANS    1
#define HAVE_POLYGONS    0
#define HAVE_QUADS       0
#define HAVE_QUAD_STRIPS 0

#define HAVE_ELTS        0	/* for now */

static void mgaDmaPrimitive( GLcontext *ctx, GLenum prim )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   GLuint hwprim;

   switch (prim) {
   case GL_TRIANGLES:
      hwprim = MGA_WA_TRIANGLES;
      break;
   case GL_TRIANGLE_STRIP:
      if (mmesa->vertex_size == 8)
	 hwprim = MGA_WA_TRISTRIP_T0;
      else
	 hwprim = MGA_WA_TRISTRIP_T0T1;
      break;
   case GL_TRIANGLE_FAN:
      if (mmesa->vertex_size == 8)
	 hwprim = MGA_WA_TRIFAN_T0;
      else
	 hwprim = MGA_WA_TRIFAN_T0T1;
      break;
   default:
      return;
   }

   mgaRasterPrimitive( ctx, GL_TRIANGLES, hwprim );
}


#define LOCAL_VARS mgaContextPtr mmesa = MGA_CONTEXT(ctx) 
#define INIT( prim ) do {			\
   if (0) fprintf(stderr, "%s\n", __FUNCTION__);	\
   FLUSH_BATCH(mmesa);				\
   mgaDmaPrimitive( ctx, prim );		\
} while (0)
#define FLUSH()  FLUSH_BATCH( mmesa )
#define GET_CURRENT_VB_MAX_VERTS() \
   0 /* fix me */
#define GET_SUBSEQUENT_VB_MAX_VERTS() \
   MGA_BUFFER_SIZE / (mmesa->vertex_size * 4)


#define ALLOC_VERTS( nr ) \
  mgaAllocDmaLow( mmesa, (nr) * mmesa->vertex_size * 4)
#define EMIT_VERTS( ctx, j, nr, buf ) \
   mga_emit_contiguous_verts(ctx, j, (j)+(nr), buf)

 
#define TAG(x) mga_##x
#include "tnl_dd/t_dd_dmatmp.h"



/**********************************************************************/
/*                          Render pipeline stage                     */
/**********************************************************************/


static GLboolean mga_run_render( GLcontext *ctx,
				  struct tnl_pipeline_stage *stage )
{
   mgaContextPtr mmesa = MGA_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb; 
   GLuint i;

   /* Don't handle clipping or indexed vertices or vertex manipulations.
    */
   if (mmesa->RenderIndex != 0 || 
       !mga_validate_render( ctx, VB )) {
      return GL_TRUE;
   }
   
   tnl->Driver.Render.Start( ctx );
   mmesa->SetupNewInputs = ~0;      

   for (i = 0 ; i < VB->PrimitiveCount ; i++)
   {
      GLuint prim = VB->Primitive[i].mode;
      GLuint start = VB->Primitive[i].start;
      GLuint length = VB->Primitive[i].count;

      if (!length)
	 continue;

      mga_render_tab_verts[prim & PRIM_MODE_MASK]( ctx, start, start + length, 
						   prim);
   } 

   tnl->Driver.Render.Finish( ctx );

   return GL_FALSE;		/* finished the pipe */
}


const struct tnl_pipeline_stage _mga_render_stage = 
{ 
   "mga render",
   NULL, 
   NULL,
   NULL,
   NULL,
   mga_run_render		/* run */
};
