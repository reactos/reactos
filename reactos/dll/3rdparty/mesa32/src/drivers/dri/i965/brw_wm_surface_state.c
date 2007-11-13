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
                   

#include "mtypes.h"
#include "texformat.h"
#include "texstore.h"

#include "intel_mipmap_tree.h"
#include "intel_batchbuffer.h"
#include "intel_tex.h"


#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"


static GLuint translate_tex_target( GLenum target )
{
   switch (target) {
   case GL_TEXTURE_1D: 
      return BRW_SURFACE_1D;

   case GL_TEXTURE_RECTANGLE_NV: 
      return BRW_SURFACE_2D;

   case GL_TEXTURE_2D: 
      return BRW_SURFACE_2D;

   case GL_TEXTURE_3D: 
      return BRW_SURFACE_3D;

   case GL_TEXTURE_CUBE_MAP: 
      return BRW_SURFACE_CUBE;

   default: 
      assert(0); 
      return 0;
   }
}


static GLuint translate_tex_format( GLuint mesa_format )
{
   switch( mesa_format ) {
   case MESA_FORMAT_L8:
      return BRW_SURFACEFORMAT_L8_UNORM;

   case MESA_FORMAT_I8:
      return BRW_SURFACEFORMAT_I8_UNORM;

   case MESA_FORMAT_A8:
      return BRW_SURFACEFORMAT_A8_UNORM; 

   case MESA_FORMAT_AL88:
      return BRW_SURFACEFORMAT_L8A8_UNORM;

   case MESA_FORMAT_RGB888:
      assert(0);		/* not supported for sampling */
      return BRW_SURFACEFORMAT_R8G8B8_UNORM;      

   case MESA_FORMAT_ARGB8888:
      return BRW_SURFACEFORMAT_B8G8R8A8_UNORM;

   case MESA_FORMAT_RGBA8888_REV:
      return BRW_SURFACEFORMAT_R8G8B8A8_UNORM;

   case MESA_FORMAT_RGB565:
      return BRW_SURFACEFORMAT_B5G6R5_UNORM;

   case MESA_FORMAT_ARGB1555:
      return BRW_SURFACEFORMAT_B5G5R5A1_UNORM;

   case MESA_FORMAT_ARGB4444:
      return BRW_SURFACEFORMAT_B4G4R4A4_UNORM;

   case MESA_FORMAT_YCBCR_REV:
      return BRW_SURFACEFORMAT_YCRCB_NORMAL;

   case MESA_FORMAT_YCBCR:
      return BRW_SURFACEFORMAT_YCRCB_SWAPUVY;

   case MESA_FORMAT_RGB_FXT1:
   case MESA_FORMAT_RGBA_FXT1:
      return BRW_SURFACEFORMAT_FXT1;

   case MESA_FORMAT_Z16:
      return BRW_SURFACEFORMAT_L16_UNORM;

   case MESA_FORMAT_RGBA_DXT1:
   case MESA_FORMAT_RGB_DXT1:
      return BRW_SURFACEFORMAT_DXT1_RGB;

   default:
      assert(0);
      return 0;
   }
}

static
void brw_update_texture_surface( GLcontext *ctx, 
				 GLuint unit,
				 struct brw_surface_state *surf )
{
   struct intel_context *intel = intel_context(ctx);
   struct brw_context *brw = brw_context(ctx);
   struct gl_texture_object *tObj = brw->attribs.Texture->Unit[unit]._Current;
   struct intel_texture_object *intelObj = intel_texture_object(tObj);
   struct gl_texture_image *firstImage = tObj->Image[0][intelObj->firstLevel];

   memset(surf, 0, sizeof(*surf));

   surf->ss0.mipmap_layout_mode = BRW_SURFACE_MIPMAPLAYOUT_BELOW;   
   surf->ss0.surface_type = translate_tex_target(tObj->Target);
   surf->ss0.surface_format = translate_tex_format(firstImage->TexFormat->MesaFormat);

   /* This is ok for all textures with channel width 8bit or less:
    */
/*    surf->ss0.data_return_format = BRW_SURFACERETURNFORMAT_S1; */

   /* BRW_NEW_LOCK */
   surf->ss1.base_addr = bmBufferOffset(intel,
					intelObj->mt->region->buffer);

   surf->ss2.mip_count = intelObj->lastLevel - intelObj->firstLevel;
   surf->ss2.width = firstImage->Width - 1;
   surf->ss2.height = firstImage->Height - 1;

   surf->ss3.tile_walk = BRW_TILEWALK_XMAJOR;
   surf->ss3.tiled_surface = intelObj->mt->region->tiled; /* always zero */
   surf->ss3.pitch = (intelObj->mt->pitch * intelObj->mt->cpp) - 1;
   surf->ss3.depth = firstImage->Depth - 1;

   surf->ss4.min_lod = 0;
 
   if (tObj->Target == GL_TEXTURE_CUBE_MAP) {
      surf->ss0.cube_pos_x = 1;
      surf->ss0.cube_pos_y = 1;
      surf->ss0.cube_pos_z = 1;
      surf->ss0.cube_neg_x = 1;
      surf->ss0.cube_neg_y = 1;
      surf->ss0.cube_neg_z = 1;
   }
}



#define OFFSET(TYPE, FIELD) ( (GLuint)&(((TYPE *)0)->FIELD) )


static void upload_wm_surfaces(struct brw_context *brw )
{
   GLcontext *ctx = &brw->intel.ctx;
   struct intel_context *intel = &brw->intel;
   struct brw_surface_binding_table bind;
   GLuint i;

   memcpy(&bind, &brw->wm.bind, sizeof(bind));
      
   {
      struct brw_surface_state surf;
      struct intel_region *region = brw->state.draw_region;

      memset(&surf, 0, sizeof(surf));

      if (region->cpp == 4)
	 surf.ss0.surface_format = BRW_SURFACEFORMAT_B8G8R8A8_UNORM;
      else 
	 surf.ss0.surface_format = BRW_SURFACEFORMAT_B5G6R5_UNORM;

      surf.ss0.surface_type = BRW_SURFACE_2D;

      /* _NEW_COLOR */
      surf.ss0.color_blend = (!brw->attribs.Color->_LogicOpEnabled &&
			      brw->attribs.Color->BlendEnabled);


      surf.ss0.writedisable_red =   !brw->attribs.Color->ColorMask[0];
      surf.ss0.writedisable_green = !brw->attribs.Color->ColorMask[1];
      surf.ss0.writedisable_blue =  !brw->attribs.Color->ColorMask[2];
      surf.ss0.writedisable_alpha = !brw->attribs.Color->ColorMask[3];

      /* BRW_NEW_LOCK */
      surf.ss1.base_addr = bmBufferOffset(&brw->intel, region->buffer);


      surf.ss2.width = region->pitch - 1; /* XXX: not really! */
      surf.ss2.height = region->height - 1;
      surf.ss3.tile_walk = BRW_TILEWALK_XMAJOR;
      surf.ss3.tiled_surface = region->tiled;
      surf.ss3.pitch = (region->pitch * region->cpp) - 1;

      brw->wm.bind.surf_ss_offset[0] = brw_cache_data( &brw->cache[BRW_SS_SURFACE], &surf );
      brw->wm.nr_surfaces = 1;
   }


   for (i = 0; i < BRW_MAX_TEX_UNIT; i++) {
      struct gl_texture_unit *texUnit = &brw->attribs.Texture->Unit[i];

      /* _NEW_TEXTURE, BRW_NEW_TEXDATA 
       */
      if (texUnit->_ReallyEnabled &&
	  intel_finalize_mipmap_tree(intel,texUnit->_Current)) {

	 struct brw_surface_state surf;

	 brw_update_texture_surface(ctx, i, &surf);

	 brw->wm.bind.surf_ss_offset[i+1] = brw_cache_data( &brw->cache[BRW_SS_SURFACE], &surf );
	 brw->wm.nr_surfaces = i+2;
      }
      else if( texUnit->_ReallyEnabled &&
	       texUnit->_Current == intel->frame_buffer_texobj )
      {
	 brw->wm.bind.surf_ss_offset[i+1] = brw->wm.bind.surf_ss_offset[0];
	 brw->wm.nr_surfaces = i+2;
      }    
      else {
	 brw->wm.bind.surf_ss_offset[i+1] = 0;
      }
   }

   brw->wm.bind_ss_offset = brw_cache_data( &brw->cache[BRW_SS_SURF_BIND],
					    &brw->wm.bind );
}

const struct brw_tracked_state brw_wm_surfaces = {
   .dirty = {
      .mesa = _NEW_COLOR | _NEW_TEXTURE | _NEW_BUFFERS,
      .brw = (BRW_NEW_CONTEXT | 
	      BRW_NEW_LOCK),	/* required for bmBufferOffset */
      .cache = 0
   },
   .update = upload_wm_surfaces
};



