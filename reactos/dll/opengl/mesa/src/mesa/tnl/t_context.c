/*
 * Mesa 3-D graphics library
 * Version:  7.2
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
#include "main/context.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/light.h"
#include "math/m_translate.h"
#include "math/m_xform.h"
#include "main/state.h"

#include "tnl.h"
#include "t_context.h"
#include "t_pipeline.h"

#include "vbo/vbo.h"

GLboolean
_tnl_CreateContext( struct gl_context *ctx )
{
   TNLcontext *tnl;

   /* Create the TNLcontext structure
    */
   ctx->swtnl_context = tnl = (TNLcontext *) CALLOC( sizeof(TNLcontext) );

   if (!tnl) {
      return GL_FALSE;
   }

   /* Initialize the VB.
    */
   tnl->vb.Size = ctx->Const.MaxArrayLockSize + MAX_CLIPPED_VERTICES;


   /* Initialize tnl state.
    */
   if (ctx->VertexProgram._MaintainTnlProgram) {
      _tnl_install_pipeline( ctx, _tnl_vp_pipeline );
   } else {
      _tnl_install_pipeline( ctx, _tnl_default_pipeline );
   }

   tnl->NeedNdcCoords = GL_TRUE;
   tnl->AllowVertexFog = GL_TRUE;
   tnl->AllowPixelFog = GL_TRUE;

   /* Set a few default values in the driver struct.
    */
   tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
   tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
   tnl->Driver.NotifyMaterialChange = _mesa_validate_all_lighting_tables;

   tnl->nr_blocks = 0;

   /* plug in the VBO drawing function */
   vbo_set_draw_func(ctx, _tnl_vbo_draw_prims);

   _math_init_transformation();
   _math_init_translate();

   return GL_TRUE;
}


void
_tnl_DestroyContext( struct gl_context *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   _tnl_destroy_pipeline( ctx );

   FREE(tnl);
   ctx->swtnl_context = NULL;
}


void
_tnl_InvalidateState( struct gl_context *ctx, GLuint new_state )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   const struct gl_vertex_program *vp = ctx->VertexProgram._Current;
   const struct gl_fragment_program *fp = ctx->FragmentProgram._Current;
   GLuint i;

   if (new_state & (_NEW_HINT | _NEW_PROGRAM)) {
      ASSERT(tnl->AllowVertexFog || tnl->AllowPixelFog);
      tnl->_DoVertexFog = ((tnl->AllowVertexFog && (ctx->Hint.Fog != GL_NICEST))
         || !tnl->AllowPixelFog) && !fp;
   }

   tnl->pipeline.new_state |= new_state;

   /* Calculate tnl->render_inputs.  This bitmask indicates which vertex
    * attributes need to be emitted to the rasterizer.
    */
   tnl->render_inputs_bitset = BITFIELD64_BIT(_TNL_ATTRIB_POS);

   if (!fp || (fp->Base.InputsRead & FRAG_BIT_COL0)) {
     tnl->render_inputs_bitset |= BITFIELD64_BIT(_TNL_ATTRIB_COLOR0);
   }

   if (_mesa_need_secondary_color(ctx))
     tnl->render_inputs_bitset |= BITFIELD64_BIT(_TNL_ATTRIB_COLOR1);

   for (i = 0; i < ctx->Const.MaxTextureCoordUnits; i++) {
     if (ctx->Texture._EnabledCoordUnits & (1 << i) ||
	 (fp && fp->Base.InputsRead & FRAG_BIT_TEX(i))) {
       tnl->render_inputs_bitset |= BITFIELD64_BIT(_TNL_ATTRIB_TEX(i));
     }
   }

   if (ctx->Fog.Enabled
       || (fp != NULL && (fp->Base.InputsRead & FRAG_BIT_FOGC) != 0)) {
      /* Either fixed-function fog or a fragment program needs fog coord.
       */
      tnl->render_inputs_bitset |= BITFIELD64_BIT(_TNL_ATTRIB_FOG);
   }

   if (ctx->Polygon.FrontMode != GL_FILL || 
       ctx->Polygon.BackMode != GL_FILL)
      tnl->render_inputs_bitset |= BITFIELD64_BIT(_TNL_ATTRIB_EDGEFLAG);

   if (ctx->RenderMode == GL_FEEDBACK)
      tnl->render_inputs_bitset |= BITFIELD64_BIT(_TNL_ATTRIB_TEX0);

   if (ctx->Point._Attenuated || ctx->VertexProgram.PointSizeEnabled)
      tnl->render_inputs_bitset |= BITFIELD64_BIT(_TNL_ATTRIB_POINTSIZE);

   /* check for varying vars which are written by the vertex program */
   if (vp) {
      GLuint i;
      for (i = 0; i < MAX_VARYING; i++) {
	 if (vp->Base.OutputsWritten & BITFIELD64_BIT(VERT_RESULT_VAR0 + i)) {
            tnl->render_inputs_bitset |= BITFIELD64_BIT(_TNL_ATTRIB_GENERIC(i));
         }
      }
   }
}


void
_tnl_wakeup( struct gl_context *ctx )
{
   /* Assume we haven't been getting state updates either:
    */
   _tnl_InvalidateState( ctx, ~0 );

#if 0
   if (ctx->Light.ColorMaterialEnabled) {
      _mesa_update_color_material( ctx, 
				   ctx->Current.Attrib[VERT_ATTRIB_COLOR0] );
   }
#endif
}




/**
 * Drivers call this function to tell the TCL module whether or not
 * it wants Normalized Device Coords (NDC) computed.  I.e. whether
 * we should "Divide-by-W".  Software renders will want that.
 */
void
_tnl_need_projected_coords( struct gl_context *ctx, GLboolean mode )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   tnl->NeedNdcCoords = mode;
}

void
_tnl_allow_vertex_fog( struct gl_context *ctx, GLboolean value )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   tnl->AllowVertexFog = value;
   tnl->_DoVertexFog = ((tnl->AllowVertexFog && (ctx->Hint.Fog != GL_NICEST))
      || !tnl->AllowPixelFog) && !ctx->FragmentProgram._Current;

}

void
_tnl_allow_pixel_fog( struct gl_context *ctx, GLboolean value )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   tnl->AllowPixelFog = value;
   tnl->_DoVertexFog = ((tnl->AllowVertexFog && (ctx->Hint.Fog != GL_NICEST))
      || !tnl->AllowPixelFog) && !ctx->FragmentProgram._Current;
}

