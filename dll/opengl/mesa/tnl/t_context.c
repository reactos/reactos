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

#include <precomp.h>

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
   _tnl_install_pipeline( ctx, _tnl_default_pipeline );

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

   if (new_state & _NEW_HINT) {
      ASSERT(tnl->AllowVertexFog || tnl->AllowPixelFog);
      tnl->_DoVertexFog = ((tnl->AllowVertexFog && (ctx->Hint.Fog != GL_NICEST))
         || !tnl->AllowPixelFog);
   }

   tnl->pipeline.new_state |= new_state;

   /* Calculate tnl->render_inputs.  This bitmask indicates which vertex
    * attributes need to be emitted to the rasterizer.
    */
   tnl->render_inputs_bitset = BITFIELD64_BIT(_TNL_ATTRIB_POS);

   tnl->render_inputs_bitset |= BITFIELD64_BIT(_TNL_ATTRIB_COLOR);

   if (ctx->Texture._EnabledCoord) {
      tnl->render_inputs_bitset |= BITFIELD64_BIT(_TNL_ATTRIB_TEX);
   }

   if (ctx->Fog.Enabled) {
      /* Either fixed-function fog or a fragment program needs fog coord.
       */
      tnl->render_inputs_bitset |= BITFIELD64_BIT(_TNL_ATTRIB_FOG);
   }

   if (ctx->Polygon.FrontMode != GL_FILL || 
       ctx->Polygon.BackMode != GL_FILL)
      tnl->render_inputs_bitset |= BITFIELD64_BIT(_TNL_ATTRIB_EDGEFLAG);

   if (ctx->RenderMode == GL_FEEDBACK)
      tnl->render_inputs_bitset |= BITFIELD64_BIT(_TNL_ATTRIB_TEX);

   if (ctx->Point._Attenuated)
      tnl->render_inputs_bitset |= BITFIELD64_BIT(_TNL_ATTRIB_POINTSIZE);
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
      || !tnl->AllowPixelFog);

}

void
_tnl_allow_pixel_fog( struct gl_context *ctx, GLboolean value )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   tnl->AllowPixelFog = value;
   tnl->_DoVertexFog = ((tnl->AllowVertexFog && (ctx->Hint.Fog != GL_NICEST))
      || !tnl->AllowPixelFog);
}

