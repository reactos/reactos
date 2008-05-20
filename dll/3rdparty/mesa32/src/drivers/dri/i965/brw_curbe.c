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



#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "enums.h"
#include "shader/prog_parameter.h"
#include "shader/prog_statevars.h"
#include "intel_batchbuffer.h"
#include "brw_context.h"
#include "brw_defines.h"
#include "brw_state.h"
#include "brw_util.h"
#include "brw_aub.h"


/* Partition the CURBE between the various users of constant values:
 */
static void calculate_curbe_offsets( struct brw_context *brw )
{
   /* CACHE_NEW_WM_PROG */
   GLuint nr_fp_regs = (brw->wm.prog_data->nr_params + 15) / 16;
   
   /* BRW_NEW_VERTEX_PROGRAM */
   struct brw_vertex_program *vp = (struct brw_vertex_program *)brw->vertex_program;
   GLuint nr_vp_regs = (vp->program.Base.Parameters->NumParameters * 4 + 15) / 16;
   GLuint nr_clip_regs = 0;
   GLuint total_regs;

   /* _NEW_TRANSFORM */
   if (brw->attribs.Transform->ClipPlanesEnabled) {
      GLuint nr_planes = 6 + brw_count_bits(brw->attribs.Transform->ClipPlanesEnabled);
      nr_clip_regs = (nr_planes * 4 + 15) / 16;
   }


   total_regs = nr_fp_regs + nr_vp_regs + nr_clip_regs;

   /* This can happen - what to do?  Probably rather than falling
    * back, the best thing to do is emit programs which code the
    * constants as immediate values.  Could do this either as a static
    * cap on WM and VS, or adaptively.
    *
    * Unfortunately, this is currently dependent on the results of the
    * program generation process (in the case of wm), so this would
    * introduce the need to re-generate programs in the event of a
    * curbe allocation failure.
    */
   /* Max size is 32 - just large enough to
    * hold the 128 parameters allowed by
    * the fragment and vertex program
    * api's.  It's not clear what happens
    * when both VP and FP want to use 128
    * parameters, though. 
    */
   assert(total_regs <= 32);

   /* Lazy resize:
    */
   if (nr_fp_regs > brw->curbe.wm_size ||
       nr_vp_regs > brw->curbe.vs_size ||
       nr_clip_regs != brw->curbe.clip_size ||
       (total_regs < brw->curbe.total_size / 4 &&
	brw->curbe.total_size > 16)) {

      GLuint reg = 0;

      /* Calculate a new layout: 
       */
      reg = 0;
      brw->curbe.wm_start = reg;
      brw->curbe.wm_size = nr_fp_regs; reg += nr_fp_regs;
      brw->curbe.clip_start = reg;
      brw->curbe.clip_size = nr_clip_regs; reg += nr_clip_regs;
      brw->curbe.vs_start = reg;
      brw->curbe.vs_size = nr_vp_regs; reg += nr_vp_regs;
      brw->curbe.total_size = reg;

      if (0)
	 _mesa_printf("curbe wm %d+%d clip %d+%d vs %d+%d\n",
		      brw->curbe.wm_start,
		      brw->curbe.wm_size,
		      brw->curbe.clip_start,
		      brw->curbe.clip_size,
		      brw->curbe.vs_start,
		      brw->curbe.vs_size );

      brw->state.dirty.brw |= BRW_NEW_CURBE_OFFSETS;
   }
}


const struct brw_tracked_state brw_curbe_offsets = {
   .dirty = {
      .mesa = _NEW_TRANSFORM,
      .brw  = BRW_NEW_VERTEX_PROGRAM,
      .cache = CACHE_NEW_WM_PROG
   },
   .update = calculate_curbe_offsets
};




/* Define the number of curbes within CS's urb allocation.  Multiple
 * urb entries -> multiple curbes.  These will be used by
 * fixed-function hardware in a double-buffering scheme to avoid a
 * pipeline stall each time the contents of the curbe is changed.
 */
void brw_upload_constant_buffer_state(struct brw_context *brw)
{
   struct brw_constant_buffer_state cbs; 
   memset(&cbs, 0, sizeof(cbs));

   /* It appears that this is the state packet for the CS unit, ie. the
    * urb entries detailed here are housed in the CS range from the
    * URB_FENCE command.
    */
   cbs.header.opcode = CMD_CONST_BUFFER_STATE;
   cbs.header.length = sizeof(cbs)/4 - 2;

   /* BRW_NEW_URB_FENCE */
   cbs.bits0.nr_urb_entries = brw->urb.nr_cs_entries;
   cbs.bits0.urb_entry_size = brw->urb.csize - 1;

   assert(brw->urb.nr_cs_entries);
   BRW_CACHED_BATCH_STRUCT(brw, &cbs);
}      

#if 0
const struct brw_tracked_state brw_constant_buffer_state = {
   .dirty = {
      .mesa = 0,
      .brw = BRW_NEW_URB_FENCE,
      .cache = 0
   },
   .update = brw_upload_constant_buffer_state
};
#endif


static GLfloat fixed_plane[6][4] = {
   { 0,    0,   -1, 1 },
   { 0,    0,    1, 1 },
   { 0,   -1,    0, 1 },
   { 0,    1,    0, 1 },
   {-1,    0,    0, 1 },
   { 1,    0,    0, 1 }
};

/* Upload a new set of constants.  Too much variability to go into the
 * cache mechanism, but maybe would benefit from a comparison against
 * the current uploaded set of constants.
 */
static void upload_constant_buffer(struct brw_context *brw)
{
   GLcontext *ctx = &brw->intel.ctx;
   struct brw_vertex_program *vp = (struct brw_vertex_program *)brw->vertex_program;
   struct brw_fragment_program *fp = (struct brw_fragment_program *)brw->fragment_program;
   struct brw_mem_pool *pool = &brw->pool[BRW_GS_POOL];
   GLuint sz = brw->curbe.total_size;
   GLuint bufsz = sz * 16 * sizeof(GLfloat);
   GLfloat *buf;
   GLuint i;

   /* Update our own dependency flags.  This works because this
    * function will also be called whenever fp or vp changes.
    */
   brw->curbe.tracked_state.dirty.mesa = (_NEW_TRANSFORM|_NEW_PROJECTION);
   brw->curbe.tracked_state.dirty.mesa |= vp->param_state;
   brw->curbe.tracked_state.dirty.mesa |= fp->param_state;

   if (sz == 0) {
      struct brw_constant_buffer cb;
      cb.header.opcode = CMD_CONST_BUFFER;
      cb.header.length = sizeof(cb)/4 - 2;
      cb.header.valid = 0;
      cb.bits0.buffer_length = 0;
      cb.bits0.buffer_address = 0;
      BRW_BATCH_STRUCT(brw, &cb);

      if (brw->curbe.last_buf) {
	 free(brw->curbe.last_buf);
	 brw->curbe.last_buf = NULL;
	 brw->curbe.last_bufsz  = 0;
      }
       
      return;
   }

   buf = (GLfloat *)malloc(bufsz);

   memset(buf, 0, bufsz);

   if (brw->curbe.wm_size) {
      GLuint offset = brw->curbe.wm_start * 16;

      _mesa_load_state_parameters(ctx, fp->program.Base.Parameters); 

      for (i = 0; i < brw->wm.prog_data->nr_params; i++) 
	 buf[offset + i] = brw->wm.prog_data->param[i][0];
   }


   /* The clipplanes are actually delivered to both CLIP and VS units.
    * VS uses them to calculate the outcode bitmasks.
    */
   if (brw->curbe.clip_size) {
      GLuint offset = brw->curbe.clip_start * 16;
      GLuint j;

      /* If any planes are going this way, send them all this way:
       */
      for (i = 0; i < 6; i++) {
	 buf[offset + i * 4 + 0] = fixed_plane[i][0];
	 buf[offset + i * 4 + 1] = fixed_plane[i][1];
	 buf[offset + i * 4 + 2] = fixed_plane[i][2];
	 buf[offset + i * 4 + 3] = fixed_plane[i][3];
      }

      /* Clip planes: _NEW_TRANSFORM plus _NEW_PROJECTION to get to
       * clip-space:
       */
      assert(MAX_CLIP_PLANES == 6);
      for (j = 0; j < MAX_CLIP_PLANES; j++) {
	 if (brw->attribs.Transform->ClipPlanesEnabled & (1<<j)) {
	    buf[offset + i * 4 + 0] = brw->attribs.Transform->_ClipUserPlane[j][0];
	    buf[offset + i * 4 + 1] = brw->attribs.Transform->_ClipUserPlane[j][1];
	    buf[offset + i * 4 + 2] = brw->attribs.Transform->_ClipUserPlane[j][2];
	    buf[offset + i * 4 + 3] = brw->attribs.Transform->_ClipUserPlane[j][3];
	    i++;
	 }
      }
   }


   if (brw->curbe.vs_size) {
      GLuint offset = brw->curbe.vs_start * 16;
      GLuint nr = vp->program.Base.Parameters->NumParameters;

      _mesa_load_state_parameters(ctx, vp->program.Base.Parameters); 

      for (i = 0; i < nr; i++) {
	 buf[offset + i * 4 + 0] = vp->program.Base.Parameters->ParameterValues[i][0];
	 buf[offset + i * 4 + 1] = vp->program.Base.Parameters->ParameterValues[i][1];
	 buf[offset + i * 4 + 2] = vp->program.Base.Parameters->ParameterValues[i][2];
	 buf[offset + i * 4 + 3] = vp->program.Base.Parameters->ParameterValues[i][3];
      }
   }

   if (0) {
      for (i = 0; i < sz*16; i+=4) 
	 _mesa_printf("curbe %d.%d: %f %f %f %f\n", i/8, i&4,
		      buf[i+0], buf[i+1], buf[i+2], buf[i+3]);

      _mesa_printf("last_buf %p buf %p sz %d/%d cmp %d\n",
		   brw->curbe.last_buf, buf,
		   bufsz, brw->curbe.last_bufsz,
		   brw->curbe.last_buf ? memcmp(buf, brw->curbe.last_buf, bufsz) : -1);
   }

   if (brw->curbe.last_buf &&
       bufsz == brw->curbe.last_bufsz &&
       memcmp(buf, brw->curbe.last_buf, bufsz) == 0) {
      free(buf);
/*       return; */
   } 
   else {
      if (brw->curbe.last_buf)
	 free(brw->curbe.last_buf);
      brw->curbe.last_buf = buf;
      brw->curbe.last_bufsz = bufsz;

      
      if (!brw_pool_alloc(pool, 
			  bufsz,
			  6,
			  &brw->curbe.gs_offset)) {
	 _mesa_printf("out of GS memory for curbe\n");
	 assert(0);
	 return;
      }
            

      /* Copy data to the buffer:
       */
      bmBufferSubDataAUB(&brw->intel,
			 pool->buffer,
			 brw->curbe.gs_offset, 
			 bufsz, 
			 buf,
			 DW_CONSTANT_BUFFER,
			 0);
   }

   /* TODO: only emit the constant_buffer packet when necessary, ie:
      - contents have changed
      - offset has changed
      - hw requirements due to other packets emitted.
   */
   {
      struct brw_constant_buffer cb;
      
      memset(&cb, 0, sizeof(cb));

      cb.header.opcode = CMD_CONST_BUFFER;
      cb.header.length = sizeof(cb)/4 - 2;
      cb.header.valid = 1;
      cb.bits0.buffer_length = sz - 1;
      cb.bits0.buffer_address = brw->curbe.gs_offset >> 6;
      
      /* Because this provokes an action (ie copy the constants into the
       * URB), it shouldn't be shortcircuited if identical to the
       * previous time - because eg. the urb destination may have
       * changed, or the urb contents different to last time.  
       *
       * Note that the data referred to is actually copied internally,
       * not just used in place according to passed pointer.
       *
       * It appears that the CS unit takes care of using each available
       * URB entry (Const URB Entry == CURBE) in turn, and issuing
       * flushes as necessary when doublebuffering of CURBEs isn't
       * possible.
       */
/*       intel_batchbuffer_align(brw->intel.batch, 64, sizeof(cb)); */
      BRW_BATCH_STRUCT(brw, &cb);
/*       intel_batchbuffer_align(brw->intel.batch, 64, 0); */
   }
}

/* This tracked state is unique in that the state it monitors varies
 * dynamically depending on the parameters tracked by the fragment and
 * vertex programs.  This is the template used as a starting point,
 * each context will maintain a copy of this internally and update as
 * required.
 */
const struct brw_tracked_state brw_constant_buffer = {
   .dirty = {
      .mesa = (_NEW_TRANSFORM|_NEW_PROJECTION),      /* plus fp and vp flags */
      .brw  = (BRW_NEW_FRAGMENT_PROGRAM |
	       BRW_NEW_VERTEX_PROGRAM |
	       BRW_NEW_URB_FENCE | /* Implicit - hardware requires this, not used above */
	       BRW_NEW_PSP | /* Implicit - hardware requires this, not used above */
	       BRW_NEW_CURBE_OFFSETS),
      .cache = (CACHE_NEW_WM_PROG) 
   },
   .update = upload_constant_buffer
};

