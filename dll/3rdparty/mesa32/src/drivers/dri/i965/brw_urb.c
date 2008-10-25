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
        


#include "intel_batchbuffer.h"
#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_hal.h"

#define VS 0
#define GS 1
#define CLP 2
#define SF 3
#define CS 4

/* XXX: Are the min_entry_size numbers useful?
 * XXX: Verify min_nr_entries, esp for VS.
 * XXX: Verify SF min_entry_size.
 */
static const struct {
   GLuint min_nr_entries;
   GLuint preferred_nr_entries;
   GLuint min_entry_size;
   GLuint max_entry_size;
} limits[CS+1] = {
   { 8, 32, 1, 5 },			/* vs */
   { 4, 8,  1, 5 },			/* gs */
   { 6, 8,  1, 5 },			/* clp */
   { 1, 8,  1, 12 },		        /* sf */
   { 1, 4,  1, 32 }			/* cs */
};


static GLboolean check_urb_layout( struct brw_context *brw )
{
   brw->urb.vs_start = 0;
   brw->urb.gs_start = brw->urb.nr_vs_entries * brw->urb.vsize;
   brw->urb.clip_start = brw->urb.gs_start + brw->urb.nr_gs_entries * brw->urb.vsize;
   brw->urb.sf_start = brw->urb.clip_start + brw->urb.nr_clip_entries * brw->urb.vsize;
   brw->urb.cs_start = brw->urb.sf_start + brw->urb.nr_sf_entries * brw->urb.sfsize;

   return brw->urb.cs_start + brw->urb.nr_cs_entries * brw->urb.csize <= 256;
}

/* Most minimal update, forces re-emit of URB fence packet after GS
 * unit turned on/off.
 */
static void recalculate_urb_fence( struct brw_context *brw )
{
   GLuint csize = brw->curbe.total_size;
   GLuint vsize = brw->vs.prog_data->urb_entry_size;
   GLuint sfsize = brw->sf.prog_data->urb_entry_size;

   static GLboolean (*hal_recalculate_urb_fence) (struct brw_context *brw);
   static GLboolean hal_tried;

   if (!hal_tried)
   {
      hal_recalculate_urb_fence = brw_hal_find_symbol ("intel_hal_recalculate_urb_fence");
      hal_tried = 1;
   }
   if (hal_recalculate_urb_fence)
   {
      if ((*hal_recalculate_urb_fence) (brw))
	 return;
   }
   
   if (csize < limits[CS].min_entry_size)
      csize = limits[CS].min_entry_size;

   if (vsize < limits[VS].min_entry_size)
      vsize = limits[VS].min_entry_size;

   if (sfsize < limits[SF].min_entry_size)
      sfsize = limits[SF].min_entry_size;

   if (brw->urb.vsize < vsize ||
       brw->urb.sfsize < sfsize ||
       brw->urb.csize < csize ||
       (brw->urb.constrained && (brw->urb.vsize > brw->urb.vsize ||
				 brw->urb.sfsize > brw->urb.sfsize ||
				 brw->urb.csize > brw->urb.csize))) {
      

      brw->urb.csize = csize;
      brw->urb.sfsize = sfsize;
      brw->urb.vsize = vsize;

      brw->urb.nr_vs_entries = limits[VS].preferred_nr_entries;	
      brw->urb.nr_gs_entries = limits[GS].preferred_nr_entries;	
      brw->urb.nr_clip_entries = limits[CLP].preferred_nr_entries;
      brw->urb.nr_sf_entries = limits[SF].preferred_nr_entries;	
      brw->urb.nr_cs_entries = limits[CS].preferred_nr_entries;	
      
      if (!check_urb_layout(brw)) {
	 brw->urb.nr_vs_entries = limits[VS].min_nr_entries;	
	 brw->urb.nr_gs_entries = limits[GS].min_nr_entries;	
	 brw->urb.nr_clip_entries = limits[CLP].min_nr_entries;
	 brw->urb.nr_sf_entries = limits[SF].min_nr_entries;	
	 brw->urb.nr_cs_entries = limits[CS].min_nr_entries;	

	 brw->urb.constrained = 1;
	 
	 if (!check_urb_layout(brw)) {
	    /* This is impossible, given the maximal sizes of urb
	     * entries and the values for minimum nr of entries
	     * provided above.
	     */
	    _mesa_printf("couldn't calculate URB layout!\n");
	    exit(1);
	 }
	 
	 if (INTEL_DEBUG & (DEBUG_URB|DEBUG_FALLBACKS))
	    _mesa_printf("URB CONSTRAINED\n");
      }
      else 
	 brw->urb.constrained = 0;

      if (INTEL_DEBUG & DEBUG_URB)
	 _mesa_printf("URB fence: %d ..VS.. %d ..GS.. %d ..CLP.. %d ..SF.. %d ..CS.. %d\n",
		      brw->urb.vs_start,
		      brw->urb.gs_start,
		      brw->urb.clip_start,
		      brw->urb.sf_start,
		      brw->urb.cs_start, 
		      256);
      
      brw->state.dirty.brw |= BRW_NEW_URB_FENCE;
   }
}


const struct brw_tracked_state brw_recalculate_urb_fence = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_CURBE_OFFSETS,
      .cache = (CACHE_NEW_VS_PROG |
		CACHE_NEW_SF_PROG)
   },
   .update = recalculate_urb_fence
};





void brw_upload_urb_fence(struct brw_context *brw)
{
   struct brw_urb_fence uf;
   memset(&uf, 0, sizeof(uf));

   uf.header.opcode = CMD_URB_FENCE;
   uf.header.length = sizeof(uf)/4-2;
   uf.header.vs_realloc = 1;
   uf.header.gs_realloc = 1;
   uf.header.clp_realloc = 1;
   uf.header.sf_realloc = 1;
   uf.header.vfe_realloc = 1;
   uf.header.cs_realloc = 1;

   /* The ordering below is correct, not the layout in the
    * instruction.
    *
    * There are 256 urb reg pairs in total.
    */
   uf.bits0.vs_fence  = brw->urb.gs_start;
   uf.bits0.gs_fence  = brw->urb.clip_start; 
   uf.bits0.clp_fence = brw->urb.sf_start; 
   uf.bits1.sf_fence  = brw->urb.cs_start; 
   uf.bits1.cs_fence  = 256;

   BRW_BATCH_STRUCT(brw, &uf);
}


#if 0
const struct brw_tracked_state brw_urb_fence = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_URB_FENCE | BRW_NEW_PSP,
      .cache = 0
   },
   .update = brw_upload_urb_fence
};
#endif
