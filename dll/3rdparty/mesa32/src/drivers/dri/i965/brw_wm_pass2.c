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
#include "brw_wm.h"


/* Use these to force spilling so that that functionality can be
 * tested with known-good examples rather than having to construct new
 * tests.
 */
#define TEST_PAYLOAD_SPILLS 0
#define TEST_DST_SPILLS 0

static void spill_value(struct brw_wm_compile *c,
			struct brw_wm_value *value);

static void prealloc_reg(struct brw_wm_compile *c,
			 struct brw_wm_value *value,
			 GLuint reg)
{
   if (value->lastuse) {
      /* Set nextuse to zero, it will be corrected by
       * update_register_usage().
       */
      c->pass2_grf[reg].value = value;
      c->pass2_grf[reg].nextuse = 0;

      value->resident = &c->pass2_grf[reg];
      value->hw_reg = brw_vec8_grf(reg*2, 0);

      if (TEST_PAYLOAD_SPILLS)
	 spill_value(c, value);
   }
}


/* Initialize all the register values.  Do the initial setup
 * calculations for interpolants.
 */
static void init_registers( struct brw_wm_compile *c )
{
   GLuint inputs = FRAG_BIT_WPOS | c->fp_interp_emitted;
   GLuint nr_interp_regs = 0;
   GLuint i = 0;
   GLuint j;

   for (j = 0; j < c->grf_limit; j++) 
      c->pass2_grf[j].nextuse = BRW_WM_MAX_INSN;

   for (j = 0; j < c->key.nr_depth_regs; j++) 
      prealloc_reg(c, &c->payload.depth[j], i++);

   for (j = 0; j < c->nr_creg; j++) 
      prealloc_reg(c, &c->creg[j], i++);

   for (j = 0; j < FRAG_ATTRIB_MAX; j++) 
      if (inputs & (1<<j)) {
	 nr_interp_regs++;
	 prealloc_reg(c, &c->payload.input_interp[j], i++);
      }

   assert(nr_interp_regs >= 1);

   c->prog_data.first_curbe_grf = c->key.nr_depth_regs * 2;
   c->prog_data.urb_read_length = nr_interp_regs * 2;
   c->prog_data.curb_read_length = c->nr_creg * 2;

   c->max_wm_grf = i * 2;
}


/* Update the nextuse value for each register in our file.
 */
static void update_register_usage(struct brw_wm_compile *c,
				  GLuint thisinsn)
{
   GLuint i;

   for (i = 1; i < c->grf_limit; i++) {
      struct brw_wm_grf *grf = &c->pass2_grf[i];

      /* Only search those which can change:
       */
      if (grf->nextuse < thisinsn) {
	 struct brw_wm_ref *ref = grf->value->lastuse;

	 /* Has last use of value been passed?
	  */
	 if (ref->insn < thisinsn) {
	    grf->value->resident = 0;
	    grf->value = 0;
	    grf->nextuse = BRW_WM_MAX_INSN;
	 }
	 else {
	    /* Else loop through chain to update:
	     */
	    while (ref->prevuse && ref->prevuse->insn >= thisinsn)
	       ref = ref->prevuse;

	    grf->nextuse = ref->insn;
	 }
      }
   }
}


static void spill_value(struct brw_wm_compile *c,
			struct brw_wm_value *value)
{	
   /* Allocate a spill slot.  Note that allocations start from 0x40 -
    * the first slot is reserved to mean "undef" in brw_wm_emit.c
    */
   if (!value->spill_slot) {  
      c->last_scratch += 0x40;	
      value->spill_slot = c->last_scratch;
   }

   /* The spill will be done in brw_wm_emit.c immediately after the
    * value is calculated, so we can just take this reg without any
    * further work.
    */
   value->resident->value = NULL;
   value->resident->nextuse = BRW_WM_MAX_INSN;
   value->resident = NULL;
}



/* Search for contiguous region with the most distant nearest
 * member.  Free regs count as very distant.
 *
 * TODO: implement spill-to-reg so that we can rearrange discontigous
 * free regs and then spill the oldest non-free regs in sequence.
 * This would mean inserting instructions in this pass.
 */
static GLuint search_contiguous_regs(struct brw_wm_compile *c,
				     GLuint nr,
				     GLuint thisinsn)
{
   struct brw_wm_grf *grf = c->pass2_grf;
   GLuint furthest = 0;
   GLuint reg = 0;
   GLuint i, j;

   /* Start search at 1: r0 is special and can't be used or spilled.
    */
   for (i = 1; i < c->grf_limit && furthest < BRW_WM_MAX_INSN; i++) {
      GLuint group_nextuse = BRW_WM_MAX_INSN;

      for (j = 0; j < nr; j++) {
	 if (grf[i+j].nextuse < group_nextuse)
	    group_nextuse = grf[i+j].nextuse;
      }
	 
      if (group_nextuse > furthest) {
	 furthest = group_nextuse;
	 reg = i;
      }
   }

   assert(furthest != thisinsn);
   
   /* Any non-empty regs will need to be spilled:
    */
   for (j = 0; j < nr; j++) 
      if (grf[reg+j].value)
	 spill_value(c, grf[reg+j].value);

   return reg;
}


static void alloc_contiguous_dest(struct brw_wm_compile *c, 
				  struct brw_wm_value *dst[],
				  GLuint nr,
				  GLuint thisinsn)
{
   GLuint reg = search_contiguous_regs(c, nr, thisinsn);
   GLuint i;

   for (i = 0; i < nr; i++) {
      if (!dst[i]) {
	 /* Need to grab a dummy value in TEX case.  Don't introduce
	  * it into the tracking scheme.
	  */
	 dst[i] = &c->vreg[c->nr_vreg++];
      }
      else {
	 assert(!dst[i]->resident);
	 assert(c->pass2_grf[reg+i].nextuse != thisinsn);

	 c->pass2_grf[reg+i].value = dst[i];
	 c->pass2_grf[reg+i].nextuse = thisinsn;

	 dst[i]->resident = &c->pass2_grf[reg+i];
      }

      dst[i]->hw_reg = brw_vec8_grf((reg+i)*2, 0);
   }

   if ((reg+nr)*2 > c->max_wm_grf)
      c->max_wm_grf = (reg+nr) * 2;
}


static void load_args(struct brw_wm_compile *c, 
		      struct brw_wm_instruction *inst)
{   
   GLuint thisinsn = inst - c->instruction;
   GLuint i,j;

   for (i = 0; i < 3; i++) {
      for (j = 0; j < 4; j++) {
	 struct brw_wm_ref *ref = inst->src[i][j];

	 if (ref) {
	    if (!ref->value->resident) {
	       /* Need to bring the value in from scratch space.  The code for
		* this will be done in brw_wm_emit.c, here we just do the
		* register allocation and mark the ref as requiring a fill.
		*/
	       GLuint reg = search_contiguous_regs(c, 1, thisinsn);
            
	       c->pass2_grf[reg].value = ref->value;
	       c->pass2_grf[reg].nextuse = thisinsn;
	    
	       ref->value->resident = &c->pass2_grf[reg];

	       /* Note that a fill is required:
		*/
	       ref->unspill_reg = reg*2;
	    }
	    
	    /* Adjust the hw_reg to point at the value's current location:
	     */
	    assert(ref->value == ref->value->resident->value);
	    ref->hw_reg.nr += (ref->value->resident - c->pass2_grf) * 2;
	 }
      }
   }
}



/* Step 3: Work forwards once again.  Perform register allocations,
 * taking into account instructions like TEX which require contiguous
 * result registers.  Where necessary spill registers to scratch space
 * and reload later.
 */
void brw_wm_pass2( struct brw_wm_compile *c )
{
   GLuint insn;
   GLuint i;

   init_registers(c);

   for (insn = 0; insn < c->nr_insns; insn++) {
      struct brw_wm_instruction *inst = &c->instruction[insn];
      
      /* Update registers' nextuse values:
       */
      update_register_usage(c, insn);

      /* May need to unspill some args.
       */
      load_args(c, inst);

      /* Allocate registers to hold results:
       */
      switch (inst->opcode) {
      case OPCODE_TEX:
      case OPCODE_TXB:
      case OPCODE_TXP:
	 alloc_contiguous_dest(c, inst->dst, 4, insn);
	 break;

      default:
	 for (i = 0; i < 4; i++) {
	    if (inst->writemask & (1<<i)) {
	       assert(inst->dst[i]);
	       alloc_contiguous_dest(c, &inst->dst[i], 1, insn);
	    }
	 }
	 break;
      }

      if (TEST_DST_SPILLS && inst->opcode != WM_PIXELXY)
	 for (i = 0; i < 4; i++)	
	    if (inst->dst[i])
	       spill_value(c, inst->dst[i]);

   }

   if (INTEL_DEBUG & DEBUG_WM) {
      brw_wm_print_program(c, "pass2");
   }

   c->state = PASS2_DONE;

   if (INTEL_DEBUG & DEBUG_WM) {
      brw_wm_print_program(c, "pass2/done");
   }
}



