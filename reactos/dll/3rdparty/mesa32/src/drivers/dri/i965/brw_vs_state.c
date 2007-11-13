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

static void upload_vs_unit( struct brw_context *brw )
{
   struct brw_vs_unit_state vs;

   memset(&vs, 0, sizeof(vs));

   /* CACHE_NEW_VS_PROG */
   vs.thread0.kernel_start_pointer = brw->vs.prog_gs_offset >> 6;
   vs.thread0.grf_reg_count = ((brw->vs.prog_data->total_grf-1) & ~15) / 16;
   vs.thread3.urb_entry_read_length = brw->vs.prog_data->urb_read_length;
   vs.thread3.const_urb_entry_read_length = brw->vs.prog_data->curb_read_length;
   vs.thread3.dispatch_grf_start_reg = 1;


   /* BRW_NEW_URB_FENCE  */
   vs.thread4.nr_urb_entries = brw->urb.nr_vs_entries; 
   vs.thread4.urb_entry_allocation_size = brw->urb.vsize - 1;
   vs.thread4.max_threads = MIN2(
      MAX2(0, (brw->urb.nr_vs_entries - 6) / 2 - 1), 
      15);



   if (INTEL_DEBUG & DEBUG_SINGLE_THREAD)
      vs.thread4.max_threads = 0; 

   /* BRW_NEW_CURBE_OFFSETS, _NEW_TRANSFORM */
   if (brw->attribs.Transform->ClipPlanesEnabled) {
      /* Note that we read in the userclip planes as well, hence
       * clip_start:
       */
      vs.thread3.const_urb_entry_read_offset = brw->curbe.clip_start * 2;
   }
   else {
      vs.thread3.const_urb_entry_read_offset = brw->curbe.vs_start * 2;
   }

   vs.thread1.floating_point_mode = BRW_FLOATING_POINT_NON_IEEE_754;
   vs.thread3.urb_entry_read_offset = 0;

   /* No samplers for ARB_vp programs:
    */
   vs.vs5.sampler_count = 0;

   if (INTEL_DEBUG & DEBUG_STATS)
      vs.thread4.stats_enable = 1; 

   /* Vertex program always enabled: 
    */
   vs.vs6.vs_enable = 1;

   brw->vs.state_gs_offset = brw_cache_data( &brw->cache[BRW_VS_UNIT], &vs );
}


const struct brw_tracked_state brw_vs_unit = {
   .dirty = {
      .mesa  = _NEW_TRANSFORM,
      .brw   = (BRW_NEW_CURBE_OFFSETS |
		BRW_NEW_URB_FENCE),
      .cache = CACHE_NEW_VS_PROG
   },
   .update = upload_vs_unit
};
