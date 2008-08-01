/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */


#include "brw_context.h"
#include "brw_aub.h"
#include "brw_defines.h"
#include "brw_draw.h"
#include "brw_vs.h"
#include "imports.h"
#include "intel_tex.h"
#include "intel_blit.h"
#include "intel_batchbuffer.h"

#include "utils.h"
#include "api_noop.h"
#include "vtxfmt.h"

/***************************************
 * Mesa's Driver Functions
 ***************************************/

static const struct dri_extension brw_extensions[] =
{
    { "GL_ARB_depth_texture",              NULL },
    { "GL_ARB_fragment_program",           NULL },
    { "GL_ARB_shadow",                     NULL },
    { "GL_EXT_shadow_funcs",               NULL },
    /* ARB extn won't work if not enabled */
    { "GL_SGIX_depth_texture",             NULL },
    { "GL_ARB_texture_env_crossbar",       NULL },
    { NULL,                                NULL }
};


static void brwInitDriverFunctions( struct dd_function_table *functions )
{
   intelInitDriverFunctions( functions );
   brwInitTextureFuncs( functions );
   brwInitFragProgFuncs( functions );
}


static void brw_init_attribs( struct brw_context *brw )
{
   GLcontext *ctx = &brw->intel.ctx;

   brw->attribs.Color = &ctx->Color;
   brw->attribs.Depth = &ctx->Depth;
   brw->attribs.Fog = &ctx->Fog;
   brw->attribs.Hint = &ctx->Hint;
   brw->attribs.Light = &ctx->Light;
   brw->attribs.Line = &ctx->Line;
   brw->attribs.Point = &ctx->Point;
   brw->attribs.Polygon = &ctx->Polygon;
   brw->attribs.Scissor = &ctx->Scissor;
   brw->attribs.Stencil = &ctx->Stencil;
   brw->attribs.Texture = &ctx->Texture;
   brw->attribs.Transform = &ctx->Transform;
   brw->attribs.Viewport = &ctx->Viewport;
   brw->attribs.VertexProgram = &ctx->VertexProgram;
   brw->attribs.FragmentProgram = &ctx->FragmentProgram;
   brw->attribs.PolygonStipple = &ctx->PolygonStipple[0];
}


GLboolean brwCreateContext( const __GLcontextModes *mesaVis,
			    __DRIcontextPrivate *driContextPriv,
			    void *sharedContextPrivate)
{
   struct dd_function_table functions;
   struct brw_context *brw = (struct brw_context *) CALLOC_STRUCT(brw_context);
   struct intel_context *intel = &brw->intel;
   GLcontext *ctx = &intel->ctx;

   if (!brw) {
      _mesa_printf("%s: failed to alloc context\n", __FUNCTION__);
      return GL_FALSE;
   }

   brwInitVtbl( brw );
   brwInitDriverFunctions( &functions );

   if (!intelInitContext( intel, mesaVis, driContextPriv,
			  sharedContextPrivate, &functions )) {
      _mesa_printf("%s: failed to init intel context\n", __FUNCTION__);
      FREE(brw);
      return GL_FALSE;
   }

   ctx->Const.MaxTextureUnits = BRW_MAX_TEX_UNIT;
   ctx->Const.MaxTextureImageUnits = BRW_MAX_TEX_UNIT;
   ctx->Const.MaxTextureCoordUnits = BRW_MAX_TEX_UNIT;


   /* Advertise the full hardware capabilities.  The new memory
    * manager should cope much better with overload situations:
    */
   ctx->Const.MaxTextureLevels = 12;
   ctx->Const.Max3DTextureLevels = 9;
   ctx->Const.MaxCubeTextureLevels = 12;
   ctx->Const.MaxTextureRectSize = (1<<11);
   ctx->Const.MaxTextureUnits = BRW_MAX_TEX_UNIT;
   
/*    ctx->Const.MaxNativeVertexProgramTemps = 32; */


   driInitExtensions( ctx, brw_extensions, GL_FALSE );

   brw_aub_init( brw );

   brw_init_attribs( brw );
   brw_init_metaops( brw );
   brw_init_state( brw );

   brw->state.dirty.mesa = ~0;
   brw->state.dirty.brw = ~0;

   memset(&brw->wm.bind, ~0, sizeof(brw->wm.bind));

   brw->emit_state_always = 0;

   ctx->FragmentProgram._MaintainTexEnvProgram = 1;

   brw_draw_init( brw );

   brw_ProgramCacheInit( ctx );

   brw_FrameBufferTexInit( brw );

   {
      const char *filename = getenv("INTEL_REPLAY");
      if (filename) {
	 brw_playback_aubfile(brw, filename);
	 exit(0);
      }
   }

   return GL_TRUE;
}

