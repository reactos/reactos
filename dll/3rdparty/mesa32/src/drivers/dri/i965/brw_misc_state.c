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
#include "intel_regions.h"

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"





/***********************************************************************
 * Blend color
 */

static void upload_blend_constant_color(struct brw_context *brw)
{
   struct brw_blend_constant_color bcc;

   memset(&bcc, 0, sizeof(bcc));      
   bcc.header.opcode = CMD_BLEND_CONSTANT_COLOR;
   bcc.header.length = sizeof(bcc)/4-2;
   bcc.blend_constant_color[0] = brw->attribs.Color->BlendColor[0];
   bcc.blend_constant_color[1] = brw->attribs.Color->BlendColor[1];
   bcc.blend_constant_color[2] = brw->attribs.Color->BlendColor[2];
   bcc.blend_constant_color[3] = brw->attribs.Color->BlendColor[3];

   BRW_CACHED_BATCH_STRUCT(brw, &bcc);
}


const struct brw_tracked_state brw_blend_constant_color = {
   .dirty = {
      .mesa = _NEW_COLOR,
      .brw = 0,
      .cache = 0
   },
   .update = upload_blend_constant_color
};

/***********************************************************************
 * Drawing rectangle -- Need for AUB file only.
 */
static void upload_drawing_rect(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   __DRIdrawablePrivate *dPriv = intel->driDrawable;
   struct brw_drawrect bdr;
   int x1, y1;
   int x2, y2;

   /* If there is a single cliprect, set it here.  Otherwise iterate
    * over them in brw_draw_prim().
    */
   if (brw->intel.numClipRects > 1) 
      return; 
 
   x1 = brw->intel.pClipRects[0].x1;
   y1 = brw->intel.pClipRects[0].y1;
   x2 = brw->intel.pClipRects[0].x2;
   y2 = brw->intel.pClipRects[0].y2;
	 
   if (x1 < 0) x1 = 0;
   if (y1 < 0) y1 = 0;
   if (x2 > intel->intelScreen->width) x2 = intel->intelScreen->width;
   if (y2 > intel->intelScreen->height) y2 = intel->intelScreen->height;

   memset(&bdr, 0, sizeof(bdr));
   bdr.header.opcode = CMD_DRAW_RECT;
   bdr.header.length = sizeof(bdr)/4 - 2;
   bdr.xmin = x1;
   bdr.ymin = y1;
   bdr.xmax = x2;
   bdr.ymax = y2;
   bdr.xorg = dPriv->x;
   bdr.yorg = dPriv->y;

   /* Can't use BRW_CACHED_BATCH_STRUCT because this is also emitted
    * uncached in brw_draw.c:
    */
   BRW_BATCH_STRUCT(brw, &bdr);
}

const struct brw_tracked_state brw_drawing_rect = {
   .dirty = {
      .mesa = _NEW_WINDOW_POS,
      .brw = 0,
      .cache = 0
   },
   .update = upload_drawing_rect
};

/***********************************************************************
 * Binding table pointers
 */

static void upload_binding_table_pointers(struct brw_context *brw)
{
   struct brw_binding_table_pointers btp;
   memset(&btp, 0, sizeof(btp));

   /* The binding table has been emitted to the SS pool already, so we
    * know what its offset is.  When the batch buffer is fired, the
    * binding table and surface structs will get fixed up to point to
    * where the textures actually landed, but that won't change the
    * value of the offsets here:
    */
   btp.header.opcode = CMD_BINDING_TABLE_PTRS;
   btp.header.length = sizeof(btp)/4 - 2;
   btp.vs = 0;
   btp.gs = 0;
   btp.clp = 0;
   btp.sf = 0;
   btp.wm = brw->wm.bind_ss_offset;

   BRW_CACHED_BATCH_STRUCT(brw, &btp);
}

const struct brw_tracked_state brw_binding_table_pointers = {
   .dirty = {
      .mesa = 0,
      .brw = 0,
      .cache = CACHE_NEW_SURF_BIND 
   },
   .update = upload_binding_table_pointers
};


/***********************************************************************
 * Pipelined state pointers.  This is the key state packet from which
 * the hardware chases pointers to all the uploaded state in VRAM.
 */
   
static void upload_pipelined_state_pointers(struct brw_context *brw )
{
   struct brw_pipelined_state_pointers psp;
   memset(&psp, 0, sizeof(psp));

   psp.header.opcode = CMD_PIPELINED_STATE_POINTERS;
   psp.header.length = sizeof(psp)/4 - 2;

   psp.vs.offset = brw->vs.state_gs_offset >> 5;
   psp.sf.offset = brw->sf.state_gs_offset >> 5;
   psp.wm.offset = brw->wm.state_gs_offset >> 5;
   psp.cc.offset = brw->cc.state_gs_offset >> 5;

   /* GS gets turned on and off regularly.  Need to re-emit URB fence
    * after this occurs.  
    */
   if (brw->gs.prog_active) {
      psp.gs.offset = brw->gs.state_gs_offset >> 5;
      psp.gs.enable = 1;
   }

   if (!brw->metaops.active) {
      psp.clp.offset = brw->clip.state_gs_offset >> 5;
      psp.clp.enable = 1;
   }


   if (BRW_CACHED_BATCH_STRUCT(brw, &psp))
      brw->state.dirty.brw |= BRW_NEW_PSP;
}

const struct brw_tracked_state brw_pipelined_state_pointers = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_METAOPS,
      .cache = (CACHE_NEW_VS_UNIT | 
		CACHE_NEW_GS_UNIT | 
		CACHE_NEW_GS_PROG | 
		CACHE_NEW_CLIP_UNIT | 
		CACHE_NEW_SF_UNIT | 
		CACHE_NEW_WM_UNIT | 
		CACHE_NEW_CC_UNIT)
   },
   .update = upload_pipelined_state_pointers
};

static void upload_psp_urb_cbs(struct brw_context *brw )
{
   upload_pipelined_state_pointers(brw);
   brw_upload_urb_fence(brw);
   brw_upload_constant_buffer_state(brw);
}


const struct brw_tracked_state brw_psp_urb_cbs = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_URB_FENCE | BRW_NEW_METAOPS,
      .cache = (CACHE_NEW_VS_UNIT | 
		CACHE_NEW_GS_UNIT | 
		CACHE_NEW_GS_PROG | 
		CACHE_NEW_CLIP_UNIT | 
		CACHE_NEW_SF_UNIT | 
		CACHE_NEW_WM_UNIT | 
		CACHE_NEW_CC_UNIT)
   },
   .update = upload_psp_urb_cbs
};




/***********************************************************************
 * Depthbuffer - currently constant, but rotation would change that.
 */

static void upload_depthbuffer(struct brw_context *brw)
{
   /* 0x79050003  Depth Buffer */
   struct intel_context *intel = &brw->intel;
   struct intel_region *region = brw->state.depth_region;
   struct brw_depthbuffer bd;
   memset(&bd, 0, sizeof(bd));

   bd.header.bits.opcode = CMD_DEPTH_BUFFER;
   bd.header.bits.length = sizeof(bd)/4-2;
   bd.dword1.bits.pitch = (region->pitch * region->cpp) - 1;
   
   switch (region->cpp) {
   case 2:
      bd.dword1.bits.format = BRW_DEPTHFORMAT_D16_UNORM;
      break;
   case 4:
      if (intel->depth_buffer_is_float)
	 bd.dword1.bits.format = BRW_DEPTHFORMAT_D32_FLOAT;
      else
	 bd.dword1.bits.format = BRW_DEPTHFORMAT_D24_UNORM_S8_UINT;
      break;
   default:
      assert(0);
      return;
   }

   bd.dword1.bits.depth_offset_disable = 0; /* coordinate offset */

   /* The depthbuffer can only use YMAJOR tiling...  This is a bit of
    * a shame as it clashes with the 2d blitter which only supports
    * XMAJOR tiling...  
    */
   bd.dword1.bits.tile_walk = BRW_TILEWALK_YMAJOR;
   bd.dword1.bits.tiled_surface = intel->depth_region->tiled;
   bd.dword1.bits.surface_type = BRW_SURFACE_2D;

   /* BRW_NEW_LOCK */
   bd.dword2_base_addr = bmBufferOffset(intel, region->buffer);    

   bd.dword3.bits.mipmap_layout = BRW_SURFACE_MIPMAPLAYOUT_BELOW;
   bd.dword3.bits.lod = 0;
   bd.dword3.bits.width = region->pitch - 1; /* XXX: width ? */
   bd.dword3.bits.height = region->height - 1;

   bd.dword4.bits.min_array_element = 0;
   bd.dword4.bits.depth = 0;
      
   BRW_CACHED_BATCH_STRUCT(brw, &bd);
}

const struct brw_tracked_state brw_depthbuffer = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_CONTEXT | BRW_NEW_LOCK,
      .cache = 0
   },
   .update = upload_depthbuffer
};



/***********************************************************************
 * Polygon stipple packet
 */

static void upload_polygon_stipple(struct brw_context *brw)
{
   struct brw_polygon_stipple bps;
   GLuint i;

   memset(&bps, 0, sizeof(bps));
   bps.header.opcode = CMD_POLY_STIPPLE_PATTERN;
   bps.header.length = sizeof(bps)/4-2;

   for (i = 0; i < 32; i++)
      bps.stipple[i] = brw->attribs.PolygonStipple[31 - i]; /* invert */

   BRW_CACHED_BATCH_STRUCT(brw, &bps);
}

const struct brw_tracked_state brw_polygon_stipple = {
   .dirty = {
      .mesa = _NEW_POLYGONSTIPPLE,
      .brw = 0,
      .cache = 0
   },
   .update = upload_polygon_stipple
};


/***********************************************************************
 * Polygon stipple offset packet
 */

static void upload_polygon_stipple_offset(struct brw_context *brw)
{
   __DRIdrawablePrivate *dPriv = brw->intel.driDrawable;
   struct brw_polygon_stipple_offset bpso;

   memset(&bpso, 0, sizeof(bpso));
   bpso.header.opcode = CMD_POLY_STIPPLE_OFFSET;
   bpso.header.length = sizeof(bpso)/4-2;

   bpso.bits0.x_offset = (32 - (dPriv->x & 31)) & 31;
   bpso.bits0.y_offset = (32 - ((dPriv->y + dPriv->h) & 31)) & 31;

   BRW_CACHED_BATCH_STRUCT(brw, &bpso);
}

const struct brw_tracked_state brw_polygon_stipple_offset = {
   .dirty = {
      .mesa = _NEW_WINDOW_POS,
      .brw = 0,
      .cache = 0
   },
   .update = upload_polygon_stipple_offset
};

/***********************************************************************
 * Line stipple packet
 */

static void upload_line_stipple(struct brw_context *brw)
{
   struct brw_line_stipple bls;
   GLfloat tmp;
   GLint tmpi;

   memset(&bls, 0, sizeof(bls));
   bls.header.opcode = CMD_LINE_STIPPLE_PATTERN;
   bls.header.length = sizeof(bls)/4 - 2;

   bls.bits0.pattern = brw->attribs.Line->StipplePattern;
   bls.bits1.repeat_count = brw->attribs.Line->StippleFactor;

   tmp = 1.0 / (GLfloat) brw->attribs.Line->StippleFactor;
   tmpi = tmp * (1<<13);


   bls.bits1.inverse_repeat_count = tmpi;

   BRW_CACHED_BATCH_STRUCT(brw, &bls);
}

const struct brw_tracked_state brw_line_stipple = {
   .dirty = {
      .mesa = _NEW_LINE,
      .brw = 0,
      .cache = 0
   },
   .update = upload_line_stipple
};



/***********************************************************************
 * Misc constant state packets
 */

static void upload_pipe_control(struct brw_context *brw)
{
   struct brw_pipe_control pc;

   return;

   memset(&pc, 0, sizeof(pc));

   pc.header.opcode = CMD_PIPE_CONTROL;
   pc.header.length = sizeof(pc)/4 - 2;
   pc.header.post_sync_operation = PIPE_CONTROL_NOWRITE;

   pc.header.instruction_state_cache_flush_enable = 1;

   pc.bits1.dest_addr_type = PIPE_CONTROL_GTTWRITE_GLOBAL;

   BRW_BATCH_STRUCT(brw, &pc);
}

const struct brw_tracked_state brw_pipe_control = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .update = upload_pipe_control
};


/***********************************************************************
 * Misc invarient state packets
 */

static void upload_invarient_state( struct brw_context *brw )
{
   {
      /* 0x61040000  Pipeline Select */
      /*     PipelineSelect            : 0 */
      struct brw_pipeline_select ps;

      memset(&ps, 0, sizeof(ps));
      ps.header.opcode = CMD_PIPELINE_SELECT;
      ps.header.pipeline_select = 0;
      BRW_BATCH_STRUCT(brw, &ps);
   }

   {
      struct brw_global_depth_offset_clamp gdo;
      memset(&gdo, 0, sizeof(gdo));

      /* Disable depth offset clamping. 
       */
      gdo.header.opcode = CMD_GLOBAL_DEPTH_OFFSET_CLAMP;
      gdo.header.length = sizeof(gdo)/4 - 2;
      gdo.depth_offset_clamp = 0.0;

      BRW_BATCH_STRUCT(brw, &gdo);
   }


   /* 0x61020000  State Instruction Pointer */
   {
      struct brw_system_instruction_pointer sip;
      memset(&sip, 0, sizeof(sip));

      sip.header.opcode = CMD_STATE_INSN_POINTER;
      sip.header.length = 0;
      sip.bits0.pad = 0;
      sip.bits0.system_instruction_pointer = 0;
      BRW_BATCH_STRUCT(brw, &sip);
   }


   {
      struct brw_vf_statistics vfs;
      memset(&vfs, 0, sizeof(vfs));

      vfs.opcode = CMD_VF_STATISTICS;
      if (INTEL_DEBUG & DEBUG_STATS)
	 vfs.statistics_enable = 1; 

      BRW_BATCH_STRUCT(brw, &vfs);
   }
}

const struct brw_tracked_state brw_invarient_state = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .update = upload_invarient_state
};


/* State pool addresses:
 */
static void upload_state_base_address( struct brw_context *brw )
{
   struct intel_context *intel = &brw->intel;
   struct brw_state_base_address sba;
      
   memset(&sba, 0, sizeof(sba));

   sba.header.opcode = CMD_STATE_BASE_ADDRESS;
   sba.header.length = 0x4;

   /* BRW_NEW_LOCK */
   sba.bits0.general_state_address = bmBufferOffset(intel, brw->pool[BRW_GS_POOL].buffer) >> 5;
   sba.bits0.modify_enable = 1;

   /* BRW_NEW_LOCK */
   sba.bits1.surface_state_address = bmBufferOffset(intel, brw->pool[BRW_SS_POOL].buffer) >> 5;
   sba.bits1.modify_enable = 1;

   sba.bits2.modify_enable = 1;
   sba.bits3.modify_enable = 1;
   sba.bits4.modify_enable = 1;

   BRW_CACHED_BATCH_STRUCT(brw, &sba);
}


const struct brw_tracked_state brw_state_base_address = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_CONTEXT | BRW_NEW_LOCK,
      .cache = 0
   },
   .update = upload_state_base_address
};
