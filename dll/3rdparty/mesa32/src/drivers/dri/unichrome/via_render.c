/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


/*
 * Render unclipped vertex buffers by emitting vertices directly to
 * dma buffers.  Use strip/fan hardware acceleration where possible.
 *
 */
#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "mtypes.h"

#include "tnl/t_context.h"

#include "via_context.h"
#include "via_tris.h"
#include "via_state.h"
#include "via_ioctl.h"

/*
 * Render unclipped vertex buffers by emitting vertices directly to
 * dma buffers.  Use strip/fan hardware primitives where possible.
 * Try to simulate missing primitives with indexed vertices.
 */
#define HAVE_POINTS      1
#define HAVE_LINES       1
#define HAVE_LINE_STRIPS 1
#define HAVE_LINE_LOOP   1
#define HAVE_TRIANGLES   1
#define HAVE_TRI_STRIPS  1
#define HAVE_TRI_STRIP_1 0  
#define HAVE_TRI_FANS    1
#define HAVE_POLYGONS    1
#define HAVE_QUADS       0
#define HAVE_QUAD_STRIPS 0

#define HAVE_ELTS        0

#define LOCAL_VARS struct via_context *vmesa = VIA_CONTEXT(ctx)
#define INIT(prim) do {					\
   viaRasterPrimitive(ctx, prim, prim);	\
} while (0)
#define GET_CURRENT_VB_MAX_VERTS() \
    ((VIA_DMA_BUF_SZ - (512 + (int)vmesa->dmaLow)) / (vmesa->vertexSize * 4))
#define GET_SUBSEQUENT_VB_MAX_VERTS() \
    (VIA_DMA_BUF_SZ - 512) / (vmesa->vertexSize * 4)

#define ALLOC_VERTS( nr ) \
    viaExtendPrimitive( vmesa, (nr) * vmesa->vertexSize * 4)

#define EMIT_VERTS(ctx, j, nr, buf) \
    _tnl_emit_vertices_to_buffer(ctx, j, (j)+(nr), buf )  
    
#define FLUSH() VIA_FINISH_PRIM( vmesa )

#define TAG(x) via_fast##x
#include "tnl_dd/t_dd_dmatmp.h"
#undef TAG
#undef LOCAL_VARS
#undef INIT

/**********************************************************************/
/*                          Fast Render pipeline stage                */
/**********************************************************************/
static GLboolean via_run_fastrender(GLcontext *ctx,
                                    struct tnl_pipeline_stage *stage)
{
    struct via_context *vmesa = VIA_CONTEXT(ctx);
    TNLcontext *tnl = TNL_CONTEXT(ctx);
    struct vertex_buffer *VB = &tnl->vb;
    GLuint i;
    

    tnl->Driver.Render.Start(ctx);
    
    if (VB->ClipOrMask || 
	vmesa->renderIndex != 0 || 
	!via_fastvalidate_render( ctx, VB )) {
	tnl->Driver.Render.Finish(ctx);
        return GL_TRUE;
    }

    tnl->clipspace.new_inputs |= VERT_BIT_POS;

    for (i = 0; i < VB->PrimitiveCount; ++i) {
        GLuint mode = VB->Primitive[i].mode;
        GLuint start = VB->Primitive[i].start;
        GLuint length = VB->Primitive[i].count;
        if (length)
            via_fastrender_tab_verts[mode & PRIM_MODE_MASK](ctx, start, start+length, mode);
    }

    tnl->Driver.Render.Finish(ctx);

    return GL_FALSE;            /* finished the pipe */
}

const struct tnl_pipeline_stage _via_fastrender_stage =
{
    "via fast render",
    NULL,
    NULL,
    NULL,
    NULL,
    via_run_fastrender           /* run */
};


