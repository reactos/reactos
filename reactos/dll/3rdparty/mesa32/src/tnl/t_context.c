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

#include "tnl.h"
#include "t_context.h"
#include "t_pipeline.h"
#include "t_vp_build.h"

#include "vbo/vbo.h"

GLboolean
_tnl_CreateContext( GLcontext *ctx )
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
      _tnl_ProgramCacheInit( ctx );
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

   return GL_TRUE;
}


void
_tnl_DestroyContext( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   _tnl_destroy_pipeline( ctx );

   if (ctx->VertexProgram._MaintainTnlProgram)
      _tnl_ProgramCacheDestroy( ctx );

   FREE(tnl);
   ctx->swtnl_context = NULL;
}


void
_tnl_InvalidateState( GLcontext *ctx, GLuint new_state )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   const struct gl_vertex_program *vp = ctx->VertexProgram._Current;
   const struct gl_fragment_program *fp = ctx->FragmentProgram._Current;

   if (new_state & (_NEW_HINT | _NEW_PROGRAM)) {
      ASSERT(tnl->AllowVertexFog || tnl->AllowPixelFog);
      tnl->_DoVertexFog = ((tnl->AllowVertexFog && (ctx->Hint.Fog != GL_NICEST))
         || !tnl->AllowPixelFog) && !fp;
   }

   tnl->pipeline.new_state |= new_state;

   /* Calculate tnl->render_inputs:
    */
   if (ctx->Visual.rgbMode) {
      GLuint i;

      RENDERINPUTS_ZERO( tnl->render_inputs_bitset );
      RENDERINPUTS_SET( tnl->render_inputs_bitset, _TNL_ATTRIB_POS );
      if (!fp || (fp->Base.InputsRead & FRAG_BIT_COL0)) {
         RENDERINPUTS_SET( tnl->render_inputs_bitset, _TNL_ATTRIB_COLOR0 );
      }
      for (i = 0; i < ctx->Const.MaxTextureCoordUnits; i++) {
         if (ctx->Texture._EnabledCoordUnits & (1 << i)) {
            RENDERINPUTS_SET( tnl->render_inputs_bitset, _TNL_ATTRIB_TEX(i) );
         }
      }

      if (NEED_SECONDARY_COLOR(ctx))
         RENDERINPUTS_SET( tnl->render_inputs_bitset, _TNL_ATTRIB_COLOR1 );
   }
   else {
      RENDERINPUTS_SET( tnl->render_inputs_bitset, _TNL_ATTRIB_POS );
      RENDERINPUTS_SET( tnl->render_inputs_bitset, _TNL_ATTRIB_COLOR_INDEX );
   }

   if (ctx->Fog.Enabled) {
      /* fixed-function fog */
      RENDERINPUTS_SET( tnl->render_inputs_bitset, _TNL_ATTRIB_FOG );
   }
   else if (ctx->FragmentProgram._Active || ctx->FragmentProgram._Current) {
      struct gl_fragment_program *fp = ctx->FragmentProgram._Current;
      if (fp) {
         if (fp->FogOption != GL_NONE || (fp->Base.InputsRead & FRAG_BIT_FOGC)) {
            /* fragment program needs fog coord */
            RENDERINPUTS_SET( tnl->render_inputs_bitset, _TNL_ATTRIB_FOG );
         }
      }
   }

   if (ctx->Polygon.FrontMode != GL_FILL || 
       ctx->Polygon.BackMode != GL_FILL)
      RENDERINPUTS_SET( tnl->render_inputs_bitset, _TNL_ATTRIB_EDGEFLAG );

   if (ctx->RenderMode == GL_FEEDBACK)
      RENDERINPUTS_SET( tnl->render_inputs_bitset, _TNL_ATTRIB_TEX0 );

   if (ctx->Point._Attenuated ||
       (ctx->VertexProgram._Enabled && ctx->VertexProgram.PointSizeEnabled))
      RENDERINPUTS_SET( tnl->render_inputs_bitset, _TNL_ATTRIB_POINTSIZE );

   /* check for varying vars which are written by the vertex program */
   if (vp) {
      GLuint i;
      for (i = 0; i < MAX_VARYING; i++) {
         if (vp->Base.OutputsWritten & (1 << (VERT_RESULT_VAR0 + i))) {
            RENDERINPUTS_SET(tnl->render_inputs_bitset,
                             _TNL_ATTRIB_GENERIC(i));
         }
      }
   }
}


void
_tnl_wakeup( GLcontext *ctx )
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
_tnl_need_projected_coords( GLcontext *ctx, GLboolean mode )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   tnl->NeedNdcCoords = mode;
}

void
_tnl_allow_vertex_fog( GLcontext *ctx, GLboolean value )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   tnl->AllowVertexFog = value;
   tnl->_DoVertexFog = ((tnl->AllowVertexFog && (ctx->Hint.Fog != GL_NICEST))
      || !tnl->AllowPixelFog) && !ctx->FragmentProgram._Current;

}

void
_tnl_allow_pixel_fog( GLcontext *ctx, GLboolean value )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   tnl->AllowPixelFog = value;
   tnl->_DoVertexFog = ((tnl->AllowVertexFog && (ctx->Hint.Fog != GL_NICEST))
      || !tnl->AllowPixelFog) && !ctx->FragmentProgram._Current;
}

