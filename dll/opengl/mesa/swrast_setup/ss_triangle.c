/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "main/glheader.h"
#include "main/colormac.h"
#include "main/macros.h"
#include "main/mtypes.h"

#include "tnl/t_context.h"

#include "ss_triangle.h"
#include "ss_context.h"

#define SS_OFFSET_BIT	    0x1
#define SS_TWOSIDE_BIT	    0x2
#define SS_UNFILLED_BIT	    0x4
#define SS_MAX_TRIFUNC      0x8

static tnl_triangle_func tri_tab[SS_MAX_TRIFUNC];
static tnl_quad_func     quad_tab[SS_MAX_TRIFUNC];


/*
 * Render a triangle respecting edge flags.
 */
typedef void (* swsetup_edge_render_prim_tri)(struct gl_context *ctx,
                                              const GLubyte *ef,
                                              GLuint e0,
                                              GLuint e1,
                                              GLuint e2,
                                              const SWvertex *v0,
                                              const SWvertex *v1,
                                              const SWvertex *v2);

/*
 * Render a triangle using lines and respecting edge flags.
 */
static void
_swsetup_edge_render_line_tri(struct gl_context *ctx,
                              const GLubyte *ef,
                              GLuint e0,
                              GLuint e1,
                              GLuint e2,
                              const SWvertex *v0,
                              const SWvertex *v1,
                              const SWvertex *v2)
{
   SScontext *swsetup = SWSETUP_CONTEXT(ctx);

   if (swsetup->render_prim == GL_POLYGON) {
      if (ef[e2]) _swrast_Line( ctx, v2, v0 );
      if (ef[e0]) _swrast_Line( ctx, v0, v1 );
      if (ef[e1]) _swrast_Line( ctx, v1, v2 );
   } else {
      if (ef[e0]) _swrast_Line( ctx, v0, v1 );
      if (ef[e1]) _swrast_Line( ctx, v1, v2 );
      if (ef[e2]) _swrast_Line( ctx, v2, v0 );
   }
}

/*
 * Render a triangle using points and respecting edge flags.
 */
static void
_swsetup_edge_render_point_tri(struct gl_context *ctx,
                               const GLubyte *ef,
                               GLuint e0,
                               GLuint e1,
                               GLuint e2,
                               const SWvertex *v0,
                               const SWvertex *v1,
                               const SWvertex *v2)
{
   if (ef[e0]) _swrast_Point( ctx, v0 );
   if (ef[e1]) _swrast_Point( ctx, v1 );
   if (ef[e2]) _swrast_Point( ctx, v2 );

   _swrast_flush(ctx);
}

/*
 * Render a triangle respecting cull and shade model.
 */
static void _swsetup_render_tri(struct gl_context *ctx,
                                GLuint e0,
                                GLuint e1,
                                GLuint e2,
                                GLuint facing,
                                swsetup_edge_render_prim_tri render)
{
   SScontext *swsetup = SWSETUP_CONTEXT(ctx);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLubyte *ef = VB->EdgeFlag;
   SWvertex *verts = swsetup->verts;
   SWvertex *v0 = &verts[e0];
   SWvertex *v1 = &verts[e1];
   SWvertex *v2 = &verts[e2];

   /* cull testing */
   if (ctx->Polygon.CullFlag) {
      if (facing == 1 && ctx->Polygon.CullFaceMode != GL_FRONT)
         return;
      if (facing == 0 && ctx->Polygon.CullFaceMode != GL_BACK)
         return;
   }

   if (ctx->Light.ShadeModel == GL_FLAT) {
      GLchan c[2][4];

      /* save colors/indexes for v0, v1 vertices */
      COPY_CHAN4(c[0], v0->color);
      COPY_CHAN4(c[1], v1->color);

      /* copy v2 color/indexes to v0, v1 indexes */
      COPY_CHAN4(v0->color, v2->color);
      COPY_CHAN4(v1->color, v2->color);
      render(ctx, ef, e0, e1, e2, v0, v1, v2);

      COPY_CHAN4(v0->color, c[0]);
      COPY_CHAN4(v1->color, c[1]);
   }
   else {
      render(ctx, ef, e0, e1, e2, v0, v1, v2);
   }
}

#define SS_COLOR(a,b) UNCLAMPED_FLOAT_TO_RGBA_CHAN(a,b)
#define SS_SPEC(a,b) COPY_4V(a,b)
#define SS_IND(a,b) (a = b)

#define IND (0)
#define TAG(x) x##_rgba
#include "ss_tritmp.h"

#define IND (SS_OFFSET_BIT)
#define TAG(x) x##_offset_rgba
#include "ss_tritmp.h"

#define IND (SS_TWOSIDE_BIT)
#define TAG(x) x##_twoside_rgba
#include "ss_tritmp.h"

#define IND (SS_OFFSET_BIT|SS_TWOSIDE_BIT)
#define TAG(x) x##_offset_twoside_rgba
#include "ss_tritmp.h"

#define IND (SS_UNFILLED_BIT)
#define TAG(x) x##_unfilled_rgba
#include "ss_tritmp.h"

#define IND (SS_OFFSET_BIT|SS_UNFILLED_BIT)
#define TAG(x) x##_offset_unfilled_rgba
#include "ss_tritmp.h"

#define IND (SS_TWOSIDE_BIT|SS_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled_rgba
#include "ss_tritmp.h"

#define IND (SS_OFFSET_BIT|SS_TWOSIDE_BIT|SS_UNFILLED_BIT)
#define TAG(x) x##_offset_twoside_unfilled_rgba
#include "ss_tritmp.h"


void _swsetup_trifuncs_init( struct gl_context *ctx )
{
   (void) ctx;

   init_rgba();
   init_offset_rgba();
   init_twoside_rgba();
   init_offset_twoside_rgba();
   init_unfilled_rgba();
   init_offset_unfilled_rgba();
   init_twoside_unfilled_rgba();
   init_offset_twoside_unfilled_rgba();
}


static void swsetup_points( struct gl_context *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   SWvertex *verts = SWSETUP_CONTEXT(ctx)->verts;
   GLuint i;

   if (VB->Elts) {
      for (i = first; i < last; i++)
	 if (VB->ClipMask[VB->Elts[i]] == 0)
	    _swrast_Point( ctx, &verts[VB->Elts[i]] );
   }
   else {
      for (i = first; i < last; i++)
	 if (VB->ClipMask[i] == 0)
	    _swrast_Point( ctx, &verts[i] );
   }
}

static void swsetup_line( struct gl_context *ctx, GLuint v0, GLuint v1 )
{
   SWvertex *verts = SWSETUP_CONTEXT(ctx)->verts;
   _swrast_Line( ctx, &verts[v0], &verts[v1] );
}



void _swsetup_choose_trifuncs( struct gl_context *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint ind = 0;

   if (ctx->Polygon.OffsetPoint ||
       ctx->Polygon.OffsetLine ||
       ctx->Polygon.OffsetFill)
      ind |= SS_OFFSET_BIT;

   if (ctx->Light.Enabled && ctx->Light.Model.TwoSide)
      ind |= SS_TWOSIDE_BIT;

   /* We piggyback the two-sided stencil front/back determination on the
    * unfilled triangle path.
    */
   if (ctx->Polygon.FrontMode != GL_FILL ||
       ctx->Polygon.BackMode != GL_FILL)
      ind |= SS_UNFILLED_BIT;

   tnl->Driver.Render.Triangle = tri_tab[ind];
   tnl->Driver.Render.Quad = quad_tab[ind];
   tnl->Driver.Render.Line = swsetup_line;
   tnl->Driver.Render.Points = swsetup_points;
}
