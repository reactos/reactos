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
#include "main/imports.h"
#include "main/colormac.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "tnl/t_vertex.h"
#include "swrast_setup.h"
#include "ss_context.h"
#include "ss_triangle.h"


/* Need to check lighting state and vertex program state to know
 * if two-sided lighting is in effect.
 */
#define _SWSETUP_NEW_RENDERINDEX (_NEW_POLYGON|_NEW_LIGHT)


#define VARYING_EMIT_STYLE  EMIT_4F


GLboolean
_swsetup_CreateContext( struct gl_context *ctx )
{
   SScontext *swsetup = (SScontext *)CALLOC(sizeof(SScontext));

   if (!swsetup)
      return GL_FALSE;

   ctx->swsetup_context = swsetup;

   swsetup->NewState = ~0;
   _swsetup_trifuncs_init( ctx );

   _tnl_init_vertices( ctx, ctx->Const.MaxArrayLockSize + 12, 
		       sizeof(SWvertex) );


   return GL_TRUE;
}

void
_swsetup_DestroyContext( struct gl_context *ctx )
{
   SScontext *swsetup = SWSETUP_CONTEXT(ctx);

   if (swsetup) {
      FREE(swsetup);
      ctx->swsetup_context = 0;
   }

   _tnl_free_vertices( ctx );
}

static void
_swsetup_RenderPrimitive( struct gl_context *ctx, GLenum mode )
{
   SWSETUP_CONTEXT(ctx)->render_prim = mode;
   _swrast_render_primitive( ctx, mode );
}


/**
 * Helper macros for setup_vertex_format()
 */
#define SWZ ((SWvertex *)0)
#define SWOffset(MEMBER) (((char *)&(SWZ->MEMBER)) - ((char *)SWZ))

#define EMIT_ATTR( ATTR, STYLE, MEMBER )	\
do {						\
   map[e].attrib = (ATTR);			\
   map[e].format = (STYLE);			\
   map[e].offset = SWOffset(MEMBER);	       	\
   e++;						\
} while (0)


/**
 * Tell the tnl module how to build SWvertex objects for swrast.
 * We'll build the map[] array with that info and pass it to
 * _tnl_install_attrs().
 */
static void
setup_vertex_format(struct gl_context *ctx)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   SScontext *swsetup = SWSETUP_CONTEXT(ctx);
   GLboolean intColors = ctx->RenderMode == GL_RENDER
                      && CHAN_TYPE != GL_FLOAT;

   if (intColors != swsetup->intColors ||
       tnl->render_inputs_bitset != swsetup->last_index_bitset) {
      GLbitfield64 index_bitset = tnl->render_inputs_bitset;
      struct tnl_attr_map map[_TNL_ATTRIB_MAX];
      unsigned e = 0;

      swsetup->intColors = intColors;

      EMIT_ATTR( _TNL_ATTRIB_POS, EMIT_4F_VIEWPORT, attrib[FRAG_ATTRIB_WPOS] );

      if (index_bitset & BITFIELD64_BIT(_TNL_ATTRIB_COLOR)) {
         if (swsetup->intColors)
            EMIT_ATTR( _TNL_ATTRIB_COLOR, EMIT_4CHAN_4F_RGBA, color );
         else
            EMIT_ATTR( _TNL_ATTRIB_COLOR, EMIT_4F, attrib[FRAG_ATTRIB_COL]);
      }

      if (index_bitset & BITFIELD64_BIT(_TNL_ATTRIB_FOG)) {
         EMIT_ATTR( _TNL_ATTRIB_FOG, EMIT_1F, attrib[FRAG_ATTRIB_FOGC]);
      }

      if (index_bitset & BITFIELD64_BIT(_TNL_ATTRIB_TEX)) {
         EMIT_ATTR( _TNL_ATTRIB_TEX, EMIT_4F,
                    attrib[FRAG_ATTRIB_TEX] );
      }

      if (index_bitset & BITFIELD64_BIT(_TNL_ATTRIB_POINTSIZE))
         EMIT_ATTR( _TNL_ATTRIB_POINTSIZE, EMIT_1F, pointSize );

      _tnl_install_attrs( ctx, map, e,
                          ctx->Viewport._WindowMap.m,
                          sizeof(SWvertex) );

      swsetup->last_index_bitset = index_bitset;
   }
}


/**
 * Prepare to render a vertex buffer.
 * Called via tnl->Driver.Render.Start.
 */
static void
_swsetup_RenderStart( struct gl_context *ctx )
{
   SScontext *swsetup = SWSETUP_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;

   if (swsetup->NewState & _SWSETUP_NEW_RENDERINDEX) {
      _swsetup_choose_trifuncs(ctx);
   }

   swsetup->NewState = 0;

   _swrast_render_start(ctx);

   /* Important */
   VB->AttribPtr[VERT_ATTRIB_POS] = VB->NdcPtr;

   setup_vertex_format(ctx);
}


/*
 * We patch this function into tnl->Driver.Render.Finish.
 * It's called when we finish rendering a vertex buffer.
 */
static void
_swsetup_RenderFinish( struct gl_context *ctx )
{
   _swrast_render_finish( ctx );
}

void
_swsetup_InvalidateState( struct gl_context *ctx, GLuint new_state )
{
   SScontext *swsetup = SWSETUP_CONTEXT(ctx);
   swsetup->NewState |= new_state;
   _tnl_invalidate_vertex_state( ctx, new_state );
}


void
_swsetup_Wakeup( struct gl_context *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   SScontext *swsetup = SWSETUP_CONTEXT(ctx);

   tnl->Driver.Render.Start = _swsetup_RenderStart;
   tnl->Driver.Render.Finish = _swsetup_RenderFinish;
   tnl->Driver.Render.PrimitiveNotify = _swsetup_RenderPrimitive;
   tnl->Driver.Render.Interp = _tnl_interp;
   tnl->Driver.Render.CopyPV = _tnl_copy_pv;
   tnl->Driver.Render.ClippedPolygon = _tnl_RenderClippedPolygon; /* new */
   tnl->Driver.Render.ClippedLine = _tnl_RenderClippedLine; /* new */
   /* points */
   /* line */
   /* triangle */
   /* quad */
   tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
   tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
   tnl->Driver.Render.ResetLineStipple = _swrast_ResetLineStipple;
   tnl->Driver.Render.BuildVertices = _tnl_build_vertices;
   tnl->Driver.Render.Multipass = 0;

   _tnl_invalidate_vertices( ctx, ~0 );
   _tnl_need_projected_coords( ctx, GL_TRUE );
   _swsetup_InvalidateState( ctx, ~0 );

   swsetup->verts = (SWvertex *)tnl->clipspace.vertex_buf;
   swsetup->last_index_bitset = 0;
}
