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
 * Adapted for use on the I830:
 *    Jeff Hartmann <jhartmann@2d3d.com>
 */
/* $XFree86: xc/lib/GL/mesa/src/drv/i830/i830_render.c,v 1.2 2002/12/10 01:26:53 dawes Exp $ */

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
#include "enums.h"

#include "tnl/t_context.h"

#include "i830_screen.h"
#include "i830_dri.h"

#include "i830_context.h"
#include "i830_tris.h"
#include "i830_state.h"
#include "i830_ioctl.h"

/*
 * Render unclipped vertex buffers by emitting vertices directly to
 * dma buffers.  Use strip/fan hardware primitives where possible.
 * Try to simulate missing primitives with indexed vertices.
 */
#define HAVE_POINTS      0  /* Has it, but can't use because subpixel has to
			     * be adjusted for points on the I830/I845G
			     */
#define HAVE_LINES       1
#define HAVE_LINE_STRIPS 1
#define HAVE_TRIANGLES   1
#define HAVE_TRI_STRIPS  1
#define HAVE_TRI_STRIP_1 0  /* has it, template can't use it yet */
#define HAVE_TRI_FANS    1
#define HAVE_POLYGONS    1
#define HAVE_QUADS       0
#define HAVE_QUAD_STRIPS 0

#define HAVE_ELTS        0

static GLuint hw_prim[GL_POLYGON+1] = {
   0,
   PRIM3D_LINELIST,
   PRIM3D_LINESTRIP,
   PRIM3D_LINESTRIP,
   PRIM3D_TRILIST,
   PRIM3D_TRISTRIP,
   PRIM3D_TRIFAN,
   0,
   0,
   PRIM3D_POLY
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

static const int scale_prim[GL_POLYGON+1] = {  
   0,				/* fallback case */
   1,
   2,
   2,
   1,
   3,
   3,
   0,				/* fallback case */
   0,				/* fallback case */
   3
};


#define LOCAL_VARS i830ContextPtr imesa = I830_CONTEXT(ctx)
#define INIT( prim ) do {                          			\
   I830_STATECHANGE(imesa, 0);                      			\
   i830RasterPrimitive( ctx, reduced_prim[prim], hw_prim[prim] );   	\
} while (0)

#define FLUSH()  I830_FIREVERTICES( imesa )
#define GET_CURRENT_VB_MAX_VERTS() \
  (((int)imesa->vertex_high - (int)imesa->vertex_low) / (imesa->vertex_size*4))
#define GET_SUBSEQUENT_VB_MAX_VERTS() \
  (I830_DMA_BUF_SZ-8) / (imesa->vertex_size * 4)
  

#define ALLOC_VERTS( nr ) \
  i830AllocDmaLow( imesa, (nr) * imesa->vertex_size * 4)
#define EMIT_VERTS( ctx, j, nr, buf ) \
  _tnl_emit_vertices_to_buffer(ctx, j, (j)+(nr), buf)
  
#define TAG(x) i830_##x
#include "tnl_dd/t_dd_dmatmp.h"
  
  
/**********************************************************************/
/*                          Render pipeline stage                     */
/**********************************************************************/

/* Heuristic for i830, which can only emit a single primitive per dma
 * buffer, and has only a small number of dma buffers.
 */
static GLboolean choose_render( struct vertex_buffer *VB, int bufsz )
{
   int nr_prims = 0;
   int nr_rprims = 0;
   int nr_rverts = 0;
   int rprim = 0;
   int i;

   
   for (i = 0 ; i < VB->PrimitiveCount ; i++)
   {
      GLuint prim = VB->Primitive[i].mode;
      GLuint length = VB->Primitive[i].count;

      if (!length)
	 continue;

      if (!hw_prim[prim & PRIM_MODE_MASK])
	 return GL_FALSE;

      nr_prims++;
      nr_rverts += length * scale_prim[prim & PRIM_MODE_MASK];

      if (reduced_prim[prim&PRIM_MODE_MASK] != rprim) {
	 nr_rprims++;
	 rprim = reduced_prim[prim&PRIM_MODE_MASK];
      }
   }

   nr_prims += i / bufsz; 
   nr_rprims += nr_rverts / bufsz; 

   if ((nr_prims > nr_rprims * 2) ||
       (nr_prims > nr_rprims + 3)) 
      return GL_FALSE;

   return GL_TRUE;
}


static GLboolean i830_run_render( GLcontext *ctx, 
				 struct tnl_pipeline_stage *stage )
{
   i830ContextPtr imesa = I830_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;
   /* Don't handle clipping or indexed vertices.
    */
   if (imesa->RenderIndex != 0 || 
       !i830_validate_render( ctx, VB ) ||
       !choose_render( VB, 200 )) { /* 200 is estimate of nr verts/buf */
      return GL_TRUE;
   }

   tnl->Driver.Render.Start( ctx );
   
   for (i = 0 ; i < VB->PrimitiveCount ; i++)
   {
      GLuint prim = VB->Primitive[i].mode;
      GLuint start = VB->Primitive[i].start;
      GLuint length = VB->Primitive[i].count;

      if (!length)
	 continue;

      i830_render_tab_verts[prim & PRIM_MODE_MASK]( ctx, start, start + length,
						    prim );
   }
      
   tnl->Driver.Render.Finish( ctx );

   return GL_FALSE;     /* finished the pipe */
}


const struct tnl_pipeline_stage _i830_render_stage =
{
   "i830 render",
   NULL,
   NULL,
   NULL,
   NULL,
   i830_run_render	/* run */
};
