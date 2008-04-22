/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

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
#include "tnl/t_vertex.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_tris.h"
#include "intel_batchbuffer.h"
#include "intel_reg.h"

/*
 * Render unclipped vertex buffers by emitting vertices directly to
 * dma buffers.  Use strip/fan hardware primitives where possible.
 * Try to simulate missing primitives with indexed vertices.
 */
#define HAVE_POINTS      0      /* Has it, but can't use because subpixel has to
                                 * be adjusted for points on the INTEL/I845G
                                 */
#define HAVE_LINES       1
#define HAVE_LINE_STRIPS 1
#define HAVE_TRIANGLES   1
#define HAVE_TRI_STRIPS  1
#define HAVE_TRI_STRIP_1 0      /* has it, template can't use it yet */
#define HAVE_TRI_FANS    1
#define HAVE_POLYGONS    1
#define HAVE_QUADS       0
#define HAVE_QUAD_STRIPS 0

#define HAVE_ELTS        0

static GLuint hw_prim[GL_POLYGON + 1] = {
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

static const GLenum reduced_prim[GL_POLYGON + 1] = {
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

static const int scale_prim[GL_POLYGON + 1] = {
   0,                           /* fallback case */
   1,
   2,
   2,
   1,
   3,
   3,
   0,                           /* fallback case */
   0,                           /* fallback case */
   3
};


static void
intelDmaPrimitive(struct intel_context *intel, GLenum prim)
{
   if (0)
      fprintf(stderr, "%s %s\n", __FUNCTION__, _mesa_lookup_enum_by_nr(prim));
   INTEL_FIREVERTICES(intel);
   intel->vtbl.reduced_primitive_state(intel, reduced_prim[prim]);
   intelStartInlinePrimitive(intel, hw_prim[prim], INTEL_BATCH_CLIPRECTS);
}


#define LOCAL_VARS struct intel_context *intel = intel_context(ctx)
#define INIT( prim ) 				\
do {						\
   intelDmaPrimitive( intel, prim );		\
} while (0)

#define FLUSH() INTEL_FIREVERTICES(intel)

#define GET_SUBSEQUENT_VB_MAX_VERTS() \
  ((intel->batch->size - 1500) / (intel->vertex_size*4))
#define GET_CURRENT_VB_MAX_VERTS() GET_SUBSEQUENT_VB_MAX_VERTS()

#define ALLOC_VERTS( nr ) \
   intelExtendInlinePrimitive( intel, (nr) * intel->vertex_size )

#define EMIT_VERTS( ctx, j, nr, buf ) \
  _tnl_emit_vertices_to_buffer(ctx, j, (j)+(nr), buf )

#define TAG(x) intel_##x
#include "tnl_dd/t_dd_dmatmp.h"


/**********************************************************************/
/*                          Render pipeline stage                     */
/**********************************************************************/

/* Heuristic to choose between the two render paths:  
 */
static GLboolean
choose_render(struct intel_context *intel, struct vertex_buffer *VB)
{
   int vertsz = intel->vertex_size;
   int cost_render = 0;
   int cost_fallback = 0;
   int nr_prims = 0;
   int nr_rprims = 0;
   int nr_rverts = 0;
   int rprim = intel->reduced_primitive;
   int i = 0;

   for (i = 0; i < VB->PrimitiveCount; i++) {
      GLuint prim = VB->Primitive[i].mode;
      GLuint length = VB->Primitive[i].count;

      if (!length)
         continue;

      nr_prims++;
      nr_rverts += length * scale_prim[prim & PRIM_MODE_MASK];

      if (reduced_prim[prim & PRIM_MODE_MASK] != rprim) {
         nr_rprims++;
         rprim = reduced_prim[prim & PRIM_MODE_MASK];
      }
   }

   /* One point for each generated primitive:
    */
   cost_render = nr_prims;
   cost_fallback = nr_rprims;

   /* One point for every 1024 dwords (4k) of dma:
    */
   cost_render += (vertsz * i) / 1024;
   cost_fallback += (vertsz * nr_rverts) / 1024;

   if (0)
      fprintf(stderr, "cost render: %d fallback: %d\n",
              cost_render, cost_fallback);

   if (cost_render > cost_fallback)
      return GL_FALSE;

   return GL_TRUE;
}


static GLboolean
intel_run_render(GLcontext * ctx, struct tnl_pipeline_stage *stage)
{
   struct intel_context *intel = intel_context(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;

   /* Don't handle clipping or indexed vertices.
    */
   if (intel->RenderIndex != 0 ||
       !intel_validate_render(ctx, VB) || !choose_render(intel, VB)) {
      return GL_TRUE;
   }

   tnl->clipspace.new_inputs |= VERT_BIT_POS;

   tnl->Driver.Render.Start(ctx);

   for (i = 0; i < VB->PrimitiveCount; i++) {
      GLuint prim = VB->Primitive[i].mode;
      GLuint start = VB->Primitive[i].start;
      GLuint length = VB->Primitive[i].count;

      if (!length)
         continue;

      intel_render_tab_verts[prim & PRIM_MODE_MASK] (ctx, start,
                                                     start + length, prim);
   }

   tnl->Driver.Render.Finish(ctx);

   INTEL_FIREVERTICES(intel);

   return GL_FALSE;             /* finished the pipe */
}

const struct tnl_pipeline_stage _intel_render_stage = {
   "intel render",
   NULL,
   NULL,
   NULL,
   NULL,
   intel_run_render             /* run */
};
