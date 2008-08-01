/*
 * Intel i810 DRI driver for Mesa 3.5
 *
 * Copyright (C) 1999-2000  Keith Whitwell   All Rights Reserved.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL KEITH WHITWELL BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Author:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */


/*
 * Render unclipped vertex buffers by emitting vertices directly to
 * dma buffers.  Use strip/fan hardware acceleration where possible.
 *
 */
#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "imports.h"
#include "mtypes.h"

#include "tnl/t_context.h"

#include "i810screen.h"
#include "i810_dri.h"

#include "i810context.h"
#include "i810tris.h"
#include "i810state.h"
#include "i810vb.h"
#include "i810ioctl.h"

/*
 * Render unclipped vertex buffers by emitting vertices directly to
 * dma buffers.  Use strip/fan hardware primitives where possible.
 * Try to simulate missing primitives with indexed vertices.
 */
#define HAVE_POINTS      0
#define HAVE_LINES       1
#define HAVE_LINE_STRIPS 1
#define HAVE_TRIANGLES   1
#define HAVE_TRI_STRIPS  1
#define HAVE_TRI_STRIP_1 0	/* has it, template can't use it yet */
#define HAVE_TRI_FANS    1
#define HAVE_POLYGONS    1
#define HAVE_QUADS       0
#define HAVE_QUAD_STRIPS 0

#define HAVE_ELTS        0


static GLuint hw_prim[GL_POLYGON+1] = {
   0,
   PR_LINES,
   0,
   PR_LINESTRIP,
   PR_TRIANGLES,
   PR_TRISTRIP_0,
   PR_TRIFAN,
   0,
   0,
   PR_POLYGON
};

static const GLenum reduced_prim[GL_POLYGON+1] = {
   GL_POINTS,
   GL_LINES,
   GL_LINES,
   GL_LINES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES,
   GL_TRIANGLES
};




#define LOCAL_VARS i810ContextPtr imesa = I810_CONTEXT(ctx)
#define INIT( prim ) do {						\
   I810_STATECHANGE(imesa, 0);						\
   i810RasterPrimitive( ctx, reduced_prim[prim], hw_prim[prim] );	\
} while (0)
#define GET_CURRENT_VB_MAX_VERTS() \
  (((int)imesa->vertex_high - (int)imesa->vertex_low) / (imesa->vertex_size*4))
#define GET_SUBSEQUENT_VB_MAX_VERTS() \
  (I810_DMA_BUF_SZ-4) / (imesa->vertex_size * 4)

#define ALLOC_VERTS( nr ) \
  i810AllocDmaLow( imesa, (nr) * imesa->vertex_size * 4)
#define EMIT_VERTS( ctx, j, nr, buf ) \
  i810_emit_contiguous_verts(ctx, j, (j)+(nr), buf)

#define FLUSH()  I810_FIREVERTICES( imesa )


#define TAG(x) i810_##x
#include "tnl_dd/t_dd_dmatmp.h"


/**********************************************************************/
/*                          Render pipeline stage                     */
/**********************************************************************/


static GLboolean i810_run_render( GLcontext *ctx,
				  struct tnl_pipeline_stage *stage )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;

   /* Don't handle clipping or indexed vertices.
    */
   if (imesa->RenderIndex != 0 || 
       !i810_validate_render( ctx, VB )) {
      return GL_TRUE;
   }

   imesa->SetupNewInputs = VERT_BIT_POS;

   tnl->Driver.Render.Start( ctx );

   for (i = 0 ; i < VB->PrimitiveCount ; i++)
   {
      GLuint prim = VB->Primitive[i].mode;
      GLuint start = VB->Primitive[i].start;
      GLuint length = VB->Primitive[i].count;

      if (!length)
	 continue;

      i810_render_tab_verts[prim & PRIM_MODE_MASK]( ctx, start, start + length,
						    prim );
   }

   tnl->Driver.Render.Finish( ctx );

   return GL_FALSE;		/* finished the pipe */
}



const struct tnl_pipeline_stage _i810_render_stage =
{
   "i810 render",
   NULL,
   NULL,
   NULL,
   NULL,
   i810_run_render		/* run */
};
