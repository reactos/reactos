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


static GLuint get_tracked_mask(struct brw_wm_compile *c,
			       struct brw_wm_instruction *inst)
{
   GLuint i;
   for (i = 0; i < 4; i++) {
      if (inst->writemask & (1<<i)) {
	 if (!inst->dst[i]->contributes_to_output) {
	    inst->writemask &= ~(1<<i);
	    inst->dst[i] = 0;
	 }
      }
   }

   return inst->writemask;
}

/* Remove a reference from a value's usage chain.
 */
static void unlink_ref(struct brw_wm_ref *ref)
{
   struct brw_wm_value *value = ref->value;

   if (ref == value->lastuse) {
      value->lastuse = ref->prevuse;
   } else {
      struct brw_wm_ref *i = value->lastuse;
      while (i->prevuse != ref) i = i->prevuse;
      i->prevuse = ref->prevuse;
   }
}

static void track_arg(struct brw_wm_compile *c,
		      struct brw_wm_instruction *inst,
		      GLuint arg,
		      GLuint readmask)
{
   GLuint i;

   for (i = 0; i < 4; i++) {
      struct brw_wm_ref *ref = inst->src[arg][i];
      if (ref) {
	 if (readmask & (1<<i)) 
	    ref->value->contributes_to_output = 1;
	 else {
	    unlink_ref(ref);
	    inst->src[arg][i] = NULL;
	 }
      }
   }
}

static GLuint get_texcoord_mask( GLuint tex_idx )
{
   switch (tex_idx) {
   case TEXTURE_1D_INDEX: return WRITEMASK_X;
   case TEXTURE_2D_INDEX: return WRITEMASK_XY;
   case TEXTURE_3D_INDEX: return WRITEMASK_XYZ;
   case TEXTURE_CUBE_INDEX: return WRITEMASK_XYZ;
   case TEXTURE_RECT_INDEX: return WRITEMASK_XY;
   default: return 0;
   }
}

/* Step two: Basically this is dead code elimination.  
 *
 * Iterate backwards over instructions, noting which values
 * contribute to the final result.  Adjust writemasks to only
 * calculate these values.
 */
void brw_wm_pass1( struct brw_wm_compile *c )
{
   GLint insn;

   for (insn = c->nr_insns-1; insn >= 0; insn--) {
      struct brw_wm_instruction *inst = &c->instruction[insn];
      GLuint writemask;
      GLuint read0, read1, read2;

      if (inst->opcode == OPCODE_KIL) {
	 track_arg(c, inst, 0, WRITEMASK_XYZW); /* All args contribute to final */
	 continue;
      }

      if (inst->opcode == WM_FB_WRITE) {
	 track_arg(c, inst, 0, WRITEMASK_XYZW); 
	 track_arg(c, inst, 1, WRITEMASK_XYZW); 
	 if (c->key.source_depth_to_render_target &&
	     c->key.computes_depth)
	    track_arg(c, inst, 2, WRITEMASK_Z); 
	 else
	    track_arg(c, inst, 2, 0); 
	 continue;
      }

      /* Lookup all the registers which were written by this
       * instruction and get a mask of those that contribute to the output:
       */
      writemask = get_tracked_mask(c, inst);
      if (!writemask) {
	 GLuint arg;
	 for (arg = 0; arg < 3; arg++)
	    track_arg(c, inst, arg, 0);
	 continue;
      }

      read0 = 0;
      read1 = 0;
      read2 = 0;

      /* Mark all inputs which contribute to the marked outputs:
       */
      switch (inst->opcode) {
      case OPCODE_ABS:
      case OPCODE_FLR:
      case OPCODE_FRC:
      case OPCODE_MOV:
	 read0 = writemask;
	 break;

      case OPCODE_SUB:
      case OPCODE_SLT:
      case OPCODE_SGE:
      case OPCODE_ADD:
      case OPCODE_MAX:
      case OPCODE_MIN:
      case OPCODE_MUL:
	 read0 = writemask;
	 read1 = writemask;
	 break;

      case OPCODE_MAD:	
      case OPCODE_CMP:
      case OPCODE_LRP:
	 read0 = writemask;
	 read1 = writemask;	
	 read2 = writemask;	
	 break;

      case OPCODE_XPD: 
	 if (writemask & WRITEMASK_X) read0 |= WRITEMASK_YZ;	 
	 if (writemask & WRITEMASK_Y) read0 |= WRITEMASK_XZ;	 
	 if (writemask & WRITEMASK_Z) read0 |= WRITEMASK_XY;
	 read1 = read0;
	 break;

      case OPCODE_COS:
      case OPCODE_EX2:
      case OPCODE_LG2:
      case OPCODE_RCP:
      case OPCODE_RSQ:
      case OPCODE_SIN:
      case OPCODE_SCS:
      case WM_CINTERP:
      case WM_PIXELXY:
	 read0 = WRITEMASK_X;
	 break;

      case OPCODE_POW:
	 read0 = WRITEMASK_X;
	 read1 = WRITEMASK_X;
	 break;

      case OPCODE_TEX:
	 read0 = get_texcoord_mask(inst->tex_idx);

	 if (c->key.shadowtex_mask & (1<<inst->tex_unit))
	    read0 |= WRITEMASK_Z;
	 break;

      case OPCODE_TXB:
	 /* Shadow ignored for txb.
	  */
	 read0 = get_texcoord_mask(inst->tex_idx) | WRITEMASK_W;
	 break;

      case WM_WPOSXY:
	 read0 = writemask & WRITEMASK_XY;
	 break;

      case WM_DELTAXY:
	 read0 = writemask & WRITEMASK_XY;
	 read1 = WRITEMASK_X;
	 break;

      case WM_PIXELW:
	 read0 = WRITEMASK_X;
	 read1 = WRITEMASK_XY;
	 break;

      case WM_LINTERP:
	 read0 = WRITEMASK_X;
	 read1 = WRITEMASK_XY;
	 break;

      case WM_PINTERP:
	 read0 = WRITEMASK_X; /* interpolant */
	 read1 = WRITEMASK_XY; /* deltas */
	 read2 = WRITEMASK_W; /* pixel w */
	 break;

      case OPCODE_DP3:	
	 read0 = WRITEMASK_XYZ;
	 read1 = WRITEMASK_XYZ;
	 break;

      case OPCODE_DPH:
	 read0 = WRITEMASK_XYZ;
	 read1 = WRITEMASK_XYZW;
	 break;

      case OPCODE_DP4:
	 read0 = WRITEMASK_XYZW;
	 read1 = WRITEMASK_XYZW;
	 break;

      case OPCODE_LIT: 
	 read0 = WRITEMASK_XYW;
	 break;

      case OPCODE_SWZ:
      case OPCODE_DST:
      case OPCODE_TXP:
      default:
	 assert(0);
	 break;
      }

      track_arg(c, inst, 0, read0);
      track_arg(c, inst, 1, read1);
      track_arg(c, inst, 2, read2);
   }

   if (INTEL_DEBUG & DEBUG_WM) {
      brw_wm_print_program(c, "pass1");
   }
}



