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
#include "brw_state.h"
#include "brw_defines.h"
#include "macros.h"

static void upload_sf_vp(struct brw_context *brw)
{
   struct brw_sf_viewport sfv;

   memset(&sfv, 0, sizeof(sfv));
   
   if (brw->intel.driDrawable) 
   {
      /* _NEW_VIEWPORT, BRW_NEW_METAOPS */

      if (!brw->metaops.active) {
	 const GLfloat *v = brw->intel.ctx.Viewport._WindowMap.m;
	 
	 sfv.viewport.m00 =   v[MAT_SX];
	 sfv.viewport.m11 = - v[MAT_SY];
	 sfv.viewport.m22 =   v[MAT_SZ] * brw->intel.depth_scale;
	 sfv.viewport.m30 =   v[MAT_TX];
	 sfv.viewport.m31 = - v[MAT_TY] + brw->intel.driDrawable->h;
	 sfv.viewport.m32 =   v[MAT_TZ] * brw->intel.depth_scale;
      }
      else {
	 sfv.viewport.m00 =   1;
	 sfv.viewport.m11 = - 1;
	 sfv.viewport.m22 =   1;
	 sfv.viewport.m30 =   0;
	 sfv.viewport.m31 =   brw->intel.driDrawable->h;
	 sfv.viewport.m32 =   0;
      }
   }

   /* XXX: what state for this? */
   if (brw->intel.driDrawable)
   {
      intelScreenPrivate *screen = brw->intel.intelScreen;
      /* _NEW_SCISSOR */
      GLint x = brw->attribs.Scissor->X;
      GLint y = brw->attribs.Scissor->Y;
      GLuint w = brw->attribs.Scissor->Width;
      GLuint h = brw->attribs.Scissor->Height;

      GLint x1 = x;
      GLint y1 = brw->intel.driDrawable->h - (y + h);
      GLint x2 = x + w - 1;
      GLint y2 = y1 + h - 1;

      if (x1 < 0) x1 = 0;
      if (y1 < 0) y1 = 0;
      if (x2 < 0) x2 = 0;
      if (y2 < 0) y2 = 0;

      if (x2 >= screen->width) x2 = screen->width-1;
      if (y2 >= screen->height) y2 = screen->height-1;
      if (x1 >= screen->width) x1 = screen->width-1;
      if (y1 >= screen->height) y1 = screen->height-1;
      
      sfv.scissor.xmin = x1;
      sfv.scissor.xmax = x2;
      sfv.scissor.ymin = y1;
      sfv.scissor.ymax = y2;
   }

   brw->sf.vp_gs_offset = brw_cache_data( &brw->cache[BRW_SF_VP], &sfv );
}

const struct brw_tracked_state brw_sf_vp = {
   .dirty = {
      .mesa  = (_NEW_VIEWPORT | 
		_NEW_SCISSOR),
      .brw   = BRW_NEW_METAOPS,
      .cache = 0
   },
   .update = upload_sf_vp
};



static void upload_sf_unit( struct brw_context *brw )
{
   struct brw_sf_unit_state sf;
   memset(&sf, 0, sizeof(sf));

   /* CACHE_NEW_SF_PROG */
   sf.thread0.grf_reg_count = ((brw->sf.prog_data->total_grf-1) & ~15) / 16;
   sf.thread0.kernel_start_pointer = brw->sf.prog_gs_offset >> 6;
   sf.thread3.urb_entry_read_length = brw->sf.prog_data->urb_read_length;

   sf.thread1.floating_point_mode = BRW_FLOATING_POINT_NON_IEEE_754;
   sf.thread3.dispatch_grf_start_reg = 3;
   sf.thread3.urb_entry_read_offset = 1;

   /* BRW_NEW_URB_FENCE */
   sf.thread4.nr_urb_entries = brw->urb.nr_sf_entries;
   sf.thread4.urb_entry_allocation_size = brw->urb.sfsize - 1;
   sf.thread4.max_threads = MIN2(12, brw->urb.nr_sf_entries / 2) - 1;

   if (INTEL_DEBUG & DEBUG_SINGLE_THREAD)
      sf.thread4.max_threads = 0; 

   if (INTEL_DEBUG & DEBUG_STATS)
      sf.thread4.stats_enable = 1; 

   /* CACHE_NEW_SF_VP */
   sf.sf5.sf_viewport_state_offset = brw->sf.vp_gs_offset >> 5;
   
   sf.sf5.viewport_transform = 1;
   
   /* _NEW_SCISSOR */
   if (brw->attribs.Scissor->Enabled) 
      sf.sf6.scissor = 1;  

   /* _NEW_POLYGON */
   if (brw->attribs.Polygon->FrontFace == GL_CCW)
      sf.sf5.front_winding = BRW_FRONTWINDING_CCW;
   else
      sf.sf5.front_winding = BRW_FRONTWINDING_CW;

   if (brw->attribs.Polygon->CullFlag) {
      switch (brw->attribs.Polygon->CullFaceMode) {
      case GL_FRONT:
	 sf.sf6.cull_mode = BRW_CULLMODE_FRONT;
	 break;
      case GL_BACK:
	 sf.sf6.cull_mode = BRW_CULLMODE_BACK;
	 break;
      case GL_FRONT_AND_BACK:
	 sf.sf6.cull_mode = BRW_CULLMODE_BOTH;
	 break;
      default:
	 assert(0);
	 break;
      }
   }
   else
      sf.sf6.cull_mode = BRW_CULLMODE_NONE;
      

   /* _NEW_LINE */
   sf.sf6.line_width = brw->attribs.Line->_Width * (1<<1);

   sf.sf6.line_endcap_aa_region_width = 1;
   if (brw->attribs.Line->SmoothFlag)
      sf.sf6.aa_enable = 1;
   else if (sf.sf6.line_width <= 0x2) 
       sf.sf6.line_width = 0; 

   /* _NEW_POINT */
   sf.sf6.point_rast_rule = 1;	/* opengl conventions */
   sf.sf7.point_size = brw->attribs.Point->_Size * (1<<3);
   sf.sf7.use_point_size_state = !brw->attribs.Point->_Attenuated;
      
   /* might be BRW_NEW_PRIMITIVE if we have to adjust pv for polygons:
    */
   sf.sf7.trifan_pv = 2;
   sf.sf7.linestrip_pv = 1;
   sf.sf7.tristrip_pv = 2;
   sf.sf7.line_last_pixel_enable = 0;

   /* Set bias for OpenGL rasterization rules:
    */
   sf.sf6.dest_org_vbias = 0x8;
   sf.sf6.dest_org_hbias = 0x8;

   brw->sf.state_gs_offset = brw_cache_data( &brw->cache[BRW_SF_UNIT], &sf );
}


const struct brw_tracked_state brw_sf_unit = {
   .dirty = {
      .mesa  = (_NEW_POLYGON | 
		_NEW_LINE | 
		_NEW_POINT | 
		_NEW_SCISSOR),
      .brw   = (BRW_NEW_URB_FENCE |
		BRW_NEW_METAOPS),
      .cache = (CACHE_NEW_SF_VP |
		CACHE_NEW_SF_PROG)
   },
   .update = upload_sf_unit
};


