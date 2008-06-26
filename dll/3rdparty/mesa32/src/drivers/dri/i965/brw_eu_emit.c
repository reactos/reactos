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
#include "brw_defines.h"
#include "brw_eu.h"




/***********************************************************************
 * Internal helper for constructing instructions
 */

static void guess_execution_size( struct brw_instruction *insn,
				  struct brw_reg reg )
{
   if (reg.width == BRW_WIDTH_8 && 
       insn->header.compression_control == BRW_COMPRESSION_COMPRESSED) 
      insn->header.execution_size = BRW_EXECUTE_16;
   else
      insn->header.execution_size = reg.width;	/* note - definitions are compatible */
}


static void brw_set_dest( struct brw_instruction *insn,
			  struct brw_reg dest )
{
   insn->bits1.da1.dest_reg_file = dest.file;
   insn->bits1.da1.dest_reg_type = dest.type;
   insn->bits1.da1.dest_address_mode = dest.address_mode;

   if (dest.address_mode == BRW_ADDRESS_DIRECT) {   
      insn->bits1.da1.dest_reg_nr = dest.nr;

      if (insn->header.access_mode == BRW_ALIGN_1) {
	 insn->bits1.da1.dest_subreg_nr = dest.subnr;
	 insn->bits1.da1.dest_horiz_stride = BRW_HORIZONTAL_STRIDE_1;
      }
      else {
	 insn->bits1.da16.dest_subreg_nr = dest.subnr / 16;
	 insn->bits1.da16.dest_writemask = dest.dw1.bits.writemask;
      }
   }
   else {
      insn->bits1.ia1.dest_subreg_nr = dest.subnr;

      /* These are different sizes in align1 vs align16:
       */
      if (insn->header.access_mode == BRW_ALIGN_1) {
	 insn->bits1.ia1.dest_indirect_offset = dest.dw1.bits.indirect_offset;
	 insn->bits1.ia1.dest_horiz_stride = BRW_HORIZONTAL_STRIDE_1;
      }
      else {
	 insn->bits1.ia16.dest_indirect_offset = dest.dw1.bits.indirect_offset;
      }
   }

   /* NEW: Set the execution size based on dest.width and
    * insn->compression_control:
    */
   guess_execution_size(insn, dest);
}

static void brw_set_src0( struct brw_instruction *insn,
		      struct brw_reg reg )
{
   assert(reg.file != BRW_MESSAGE_REGISTER_FILE);

   insn->bits1.da1.src0_reg_file = reg.file;
   insn->bits1.da1.src0_reg_type = reg.type;
   insn->bits2.da1.src0_abs = reg.abs;
   insn->bits2.da1.src0_negate = reg.negate;
   insn->bits2.da1.src0_address_mode = reg.address_mode;

   if (reg.file == BRW_IMMEDIATE_VALUE) {
      insn->bits3.ud = reg.dw1.ud;
   
      /* Required to set some fields in src1 as well:
       */
      insn->bits1.da1.src1_reg_file = 0; /* arf */
      insn->bits1.da1.src1_reg_type = reg.type;
   }
   else 
   {
      if (reg.address_mode == BRW_ADDRESS_DIRECT) {
	 if (insn->header.access_mode == BRW_ALIGN_1) {
	    insn->bits2.da1.src0_subreg_nr = reg.subnr;
	    insn->bits2.da1.src0_reg_nr = reg.nr;
	 }
	 else {
	    insn->bits2.da16.src0_subreg_nr = reg.subnr / 16;
	    insn->bits2.da16.src0_reg_nr = reg.nr;
	 }
      }
      else {
	 insn->bits2.ia1.src0_subreg_nr = reg.subnr;

	 if (insn->header.access_mode == BRW_ALIGN_1) {
	    insn->bits2.ia1.src0_indirect_offset = reg.dw1.bits.indirect_offset; 
	 }
	 else {
	    insn->bits2.ia16.src0_subreg_nr = reg.dw1.bits.indirect_offset;
	 }
      }

      if (insn->header.access_mode == BRW_ALIGN_1) {
	 if (reg.width == BRW_WIDTH_1 && 
	     insn->header.execution_size == BRW_EXECUTE_1) {
	    insn->bits2.da1.src0_horiz_stride = BRW_HORIZONTAL_STRIDE_0;
	    insn->bits2.da1.src0_width = BRW_WIDTH_1;
	    insn->bits2.da1.src0_vert_stride = BRW_VERTICAL_STRIDE_0;
	 }
	 else {
	    insn->bits2.da1.src0_horiz_stride = reg.hstride;
	    insn->bits2.da1.src0_width = reg.width;
	    insn->bits2.da1.src0_vert_stride = reg.vstride;
	 }
      }
      else {
	 insn->bits2.da16.src0_swz_x = BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_X);
	 insn->bits2.da16.src0_swz_y = BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_Y);
	 insn->bits2.da16.src0_swz_z = BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_Z);
	 insn->bits2.da16.src0_swz_w = BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_W);

	 /* This is an oddity of the fact we're using the same
	  * descriptions for registers in align_16 as align_1:
	  */
	 if (reg.vstride == BRW_VERTICAL_STRIDE_8)
	    insn->bits2.da16.src0_vert_stride = BRW_VERTICAL_STRIDE_4;
	 else
	    insn->bits2.da16.src0_vert_stride = reg.vstride;
      }
   }
}


static void brw_set_src1( struct brw_instruction *insn,
			  struct brw_reg reg )
{
   assert(reg.file != BRW_MESSAGE_REGISTER_FILE);

   insn->bits1.da1.src1_reg_file = reg.file;
   insn->bits1.da1.src1_reg_type = reg.type;
   insn->bits3.da1.src1_abs = reg.abs;
   insn->bits3.da1.src1_negate = reg.negate;

   /* Only src1 can be immediate in two-argument instructions.
    */
   assert(insn->bits1.da1.src0_reg_file != BRW_IMMEDIATE_VALUE);

   if (reg.file == BRW_IMMEDIATE_VALUE) {
      insn->bits3.ud = reg.dw1.ud;
   }
   else {
      /* This is a hardware restriction, which may or may not be lifted
       * in the future:
       */
      assert (reg.address_mode == BRW_ADDRESS_DIRECT);
      assert (reg.file == BRW_GENERAL_REGISTER_FILE);

      if (insn->header.access_mode == BRW_ALIGN_1) {
	 insn->bits3.da1.src1_subreg_nr = reg.subnr;
	 insn->bits3.da1.src1_reg_nr = reg.nr;
      }
      else {
	 insn->bits3.da16.src1_subreg_nr = reg.subnr / 16;
	 insn->bits3.da16.src1_reg_nr = reg.nr;
      }

      if (insn->header.access_mode == BRW_ALIGN_1) {
	 if (reg.width == BRW_WIDTH_1 && 
	     insn->header.execution_size == BRW_EXECUTE_1) {
	    insn->bits3.da1.src1_horiz_stride = BRW_HORIZONTAL_STRIDE_0;
	    insn->bits3.da1.src1_width = BRW_WIDTH_1;
	    insn->bits3.da1.src1_vert_stride = BRW_VERTICAL_STRIDE_0;
	 }
	 else {
	    insn->bits3.da1.src1_horiz_stride = reg.hstride;
	    insn->bits3.da1.src1_width = reg.width;
	    insn->bits3.da1.src1_vert_stride = reg.vstride;
	 }
      }
      else {
	 insn->bits3.da16.src1_swz_x = BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_X);
	 insn->bits3.da16.src1_swz_y = BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_Y);
	 insn->bits3.da16.src1_swz_z = BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_Z);
	 insn->bits3.da16.src1_swz_w = BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_W);

	 /* This is an oddity of the fact we're using the same
	  * descriptions for registers in align_16 as align_1:
	  */
	 if (reg.vstride == BRW_VERTICAL_STRIDE_8)
	    insn->bits3.da16.src1_vert_stride = BRW_VERTICAL_STRIDE_4;
	 else
	    insn->bits3.da16.src1_vert_stride = reg.vstride;
      }
   }
}



static void brw_set_math_message( struct brw_instruction *insn,
				  GLuint msg_length,
				  GLuint response_length,
				  GLuint function,
				  GLuint integer_type,
				  GLboolean low_precision,
				  GLboolean saturate,
				  GLuint dataType )
{
   brw_set_src1(insn, brw_imm_d(0));

   insn->bits3.math.function = function;
   insn->bits3.math.int_type = integer_type;
   insn->bits3.math.precision = low_precision;
   insn->bits3.math.saturate = saturate;
   insn->bits3.math.data_type = dataType;
   insn->bits3.math.response_length = response_length;
   insn->bits3.math.msg_length = msg_length;
   insn->bits3.math.msg_target = BRW_MESSAGE_TARGET_MATH;
   insn->bits3.math.end_of_thread = 0;
}

static void brw_set_urb_message( struct brw_instruction *insn,
				 GLboolean allocate,
				 GLboolean used,
				 GLuint msg_length,
				 GLuint response_length,
				 GLboolean end_of_thread,
				 GLboolean complete,
				 GLuint offset,
				 GLuint swizzle_control )
{
   brw_set_src1(insn, brw_imm_d(0));

   insn->bits3.urb.opcode = 0;	/* ? */
   insn->bits3.urb.offset = offset;
   insn->bits3.urb.swizzle_control = swizzle_control;
   insn->bits3.urb.allocate = allocate;
   insn->bits3.urb.used = used;	/* ? */
   insn->bits3.urb.complete = complete;
   insn->bits3.urb.response_length = response_length;
   insn->bits3.urb.msg_length = msg_length;
   insn->bits3.urb.msg_target = BRW_MESSAGE_TARGET_URB;
   insn->bits3.urb.end_of_thread = end_of_thread;
}

static void brw_set_dp_write_message( struct brw_instruction *insn,
				      GLuint binding_table_index,
				      GLuint msg_control,
				      GLuint msg_type,
				      GLuint msg_length,
				      GLuint pixel_scoreboard_clear,
				      GLuint response_length,
				      GLuint end_of_thread )
{
   brw_set_src1(insn, brw_imm_d(0));

   insn->bits3.dp_write.binding_table_index = binding_table_index;
   insn->bits3.dp_write.msg_control = msg_control;
   insn->bits3.dp_write.pixel_scoreboard_clear = pixel_scoreboard_clear;
   insn->bits3.dp_write.msg_type = msg_type;
   insn->bits3.dp_write.send_commit_msg = 0;
   insn->bits3.dp_write.response_length = response_length;
   insn->bits3.dp_write.msg_length = msg_length;
   insn->bits3.dp_write.msg_target = BRW_MESSAGE_TARGET_DATAPORT_WRITE;
   insn->bits3.urb.end_of_thread = end_of_thread;
}

static void brw_set_dp_read_message( struct brw_instruction *insn,
				      GLuint binding_table_index,
				      GLuint msg_control,
				      GLuint msg_type,
				      GLuint target_cache,
				      GLuint msg_length,
				      GLuint response_length,
				      GLuint end_of_thread )
{
   brw_set_src1(insn, brw_imm_d(0));

   insn->bits3.dp_read.binding_table_index = binding_table_index;
   insn->bits3.dp_read.msg_control = msg_control;
   insn->bits3.dp_read.msg_type = msg_type;
   insn->bits3.dp_read.target_cache = target_cache;
   insn->bits3.dp_read.response_length = response_length;
   insn->bits3.dp_read.msg_length = msg_length;
   insn->bits3.dp_read.msg_target = BRW_MESSAGE_TARGET_DATAPORT_READ;
   insn->bits3.dp_read.end_of_thread = end_of_thread;
}

static void brw_set_sampler_message( struct brw_instruction *insn,
				     GLuint binding_table_index,
				     GLuint sampler,
				     GLuint msg_type,
				     GLuint response_length,
				     GLuint msg_length,
				     GLboolean eot)
{
   brw_set_src1(insn, brw_imm_d(0));

   insn->bits3.sampler.binding_table_index = binding_table_index;
   insn->bits3.sampler.sampler = sampler;
   insn->bits3.sampler.msg_type = msg_type;
   insn->bits3.sampler.return_format = BRW_SAMPLER_RETURN_FORMAT_FLOAT32;
   insn->bits3.sampler.response_length = response_length;
   insn->bits3.sampler.msg_length = msg_length;
   insn->bits3.sampler.end_of_thread = eot;
   insn->bits3.sampler.msg_target = BRW_MESSAGE_TARGET_SAMPLER;
}



static struct brw_instruction *next_insn( struct brw_compile *p, 
					  GLuint opcode )
{
   struct brw_instruction *insn;

   assert(p->nr_insn + 1 < BRW_EU_MAX_INSN);

   insn = &p->store[p->nr_insn++];
   memcpy(insn, p->current, sizeof(*insn));

   /* Reset this one-shot flag: 
    */

   if (p->current->header.destreg__conditonalmod) {
      p->current->header.destreg__conditonalmod = 0;   
      p->current->header.predicate_control = BRW_PREDICATE_NORMAL;
   }

   insn->header.opcode = opcode;
   return insn;
}


static struct brw_instruction *brw_alu1( struct brw_compile *p,
					 GLuint opcode,
					 struct brw_reg dest,
					 struct brw_reg src )
{
   struct brw_instruction *insn = next_insn(p, opcode);
   brw_set_dest(insn, dest);
   brw_set_src0(insn, src);   
   return insn;
}

static struct brw_instruction *brw_alu2(struct brw_compile *p,
					GLuint opcode,
					struct brw_reg dest,
					struct brw_reg src0,
					struct brw_reg src1 )
{
   struct brw_instruction *insn = next_insn(p, opcode);   
   brw_set_dest(insn, dest);
   brw_set_src0(insn, src0);
   brw_set_src1(insn, src1);
   return insn;
}


/***********************************************************************
 * Convenience routines.
 */
#define ALU1(OP)					\
struct brw_instruction *brw_##OP(struct brw_compile *p,			\
	      struct brw_reg dest,			\
	      struct brw_reg src0)   			\
{							\
   return brw_alu1(p, BRW_OPCODE_##OP, dest, src0);    	\
}

#define ALU2(OP)					\
struct brw_instruction *brw_##OP(struct brw_compile *p,			\
	      struct brw_reg dest,			\
	      struct brw_reg src0,			\
	      struct brw_reg src1)   			\
{							\
   return brw_alu2(p, BRW_OPCODE_##OP, dest, src0, src1);	\
}


ALU1(MOV)
ALU2(SEL)
ALU1(NOT)
ALU2(AND)
ALU2(OR)
ALU2(XOR)
ALU2(SHR)
ALU2(SHL)
ALU2(RSR)
ALU2(RSL)
ALU2(ASR)
ALU2(ADD)
ALU2(MUL)
ALU1(FRC)
ALU1(RNDD)
ALU2(MAC)
ALU2(MACH)
ALU1(LZD)
ALU2(DP4)
ALU2(DPH)
ALU2(DP3)
ALU2(DP2)
ALU2(LINE)




void brw_NOP(struct brw_compile *p)
{
   struct brw_instruction *insn = next_insn(p, BRW_OPCODE_NOP);   
   brw_set_dest(insn, retype(brw_vec4_grf(0,0), BRW_REGISTER_TYPE_UD));
   brw_set_src0(insn, retype(brw_vec4_grf(0,0), BRW_REGISTER_TYPE_UD));
   brw_set_src1(insn, brw_imm_ud(0x0));
}





/***********************************************************************
 * Comparisons, if/else/endif
 */

struct brw_instruction *brw_JMPI(struct brw_compile *p, 
	      struct brw_reg dest,
	      struct brw_reg src0,
	      struct brw_reg src1)
{
   struct brw_instruction *insn = brw_alu2(p, BRW_OPCODE_JMPI, dest, src0, src1);

   p->current->header.predicate_control = BRW_PREDICATE_NONE;

   return insn;
}

/* EU takes the value from the flag register and pushes it onto some
 * sort of a stack (presumably merging with any flag value already on
 * the stack).  Within an if block, the flags at the top of the stack
 * control execution on each channel of the unit, eg. on each of the
 * 16 pixel values in our wm programs.
 *
 * When the matching 'else' instruction is reached (presumably by
 * countdown of the instruction count patched in by our ELSE/ENDIF
 * functions), the relevent flags are inverted.
 *
 * When the matching 'endif' instruction is reached, the flags are
 * popped off.  If the stack is now empty, normal execution resumes.
 *
 * No attempt is made to deal with stack overflow (14 elements?).
 */
struct brw_instruction *brw_IF(struct brw_compile *p, GLuint execute_size)
{
   struct brw_instruction *insn;

   if (p->single_program_flow) {
      assert(execute_size == BRW_EXECUTE_1);

      insn = next_insn(p, BRW_OPCODE_ADD);
      insn->header.predicate_inverse = 1;
   } else {
      insn = next_insn(p, BRW_OPCODE_IF);
   }

   /* Override the defaults for this instruction:
    */
   brw_set_dest(insn, brw_ip_reg());
   brw_set_src0(insn, brw_ip_reg());
   brw_set_src1(insn, brw_imm_d(0x0));

   insn->header.execution_size = execute_size;
   insn->header.compression_control = BRW_COMPRESSION_NONE;
   insn->header.predicate_control = BRW_PREDICATE_NORMAL;
   insn->header.mask_control = BRW_MASK_ENABLE;

   p->current->header.predicate_control = BRW_PREDICATE_NONE;

   return insn;
}


struct brw_instruction *brw_ELSE(struct brw_compile *p, 
				 struct brw_instruction *if_insn)
{
   struct brw_instruction *insn;

   if (p->single_program_flow) {
      insn = next_insn(p, BRW_OPCODE_ADD);
   } else {
      insn = next_insn(p, BRW_OPCODE_ELSE);
   }

   brw_set_dest(insn, brw_ip_reg());
   brw_set_src0(insn, brw_ip_reg());
   brw_set_src1(insn, brw_imm_d(0x0));

   insn->header.compression_control = BRW_COMPRESSION_NONE;
   insn->header.execution_size = if_insn->header.execution_size;
   insn->header.mask_control = BRW_MASK_ENABLE;

   /* Patch the if instruction to point at this instruction.
    */
   if (p->single_program_flow) {
      assert(if_insn->header.opcode == BRW_OPCODE_ADD);

      if_insn->bits3.ud = (insn - if_insn + 1) * 16;
   } else {
      assert(if_insn->header.opcode == BRW_OPCODE_IF);

      if_insn->bits3.if_else.jump_count = insn - if_insn;
      if_insn->bits3.if_else.pop_count = 1;
      if_insn->bits3.if_else.pad0 = 0;
   }

   return insn;
}

void brw_ENDIF(struct brw_compile *p, 
	       struct brw_instruction *patch_insn)
{
   if (p->single_program_flow) {
      /* In single program flow mode, there's no need to execute an ENDIF,
       * since we don't need to do any stack operations, and if we're executing
       * currently, we want to just continue executing.
       */
      struct brw_instruction *next = &p->store[p->nr_insn];

      assert(patch_insn->header.opcode == BRW_OPCODE_ADD);

      patch_insn->bits3.ud = (next - patch_insn) * 16;
   } else {
      struct brw_instruction *insn = next_insn(p, BRW_OPCODE_ENDIF);

      brw_set_dest(insn, retype(brw_vec4_grf(0,0), BRW_REGISTER_TYPE_UD));
      brw_set_src0(insn, retype(brw_vec4_grf(0,0), BRW_REGISTER_TYPE_UD));
      brw_set_src1(insn, brw_imm_d(0x0));

      insn->header.compression_control = BRW_COMPRESSION_NONE;
      insn->header.execution_size = patch_insn->header.execution_size;
      insn->header.mask_control = BRW_MASK_ENABLE;

      assert(patch_insn->bits3.if_else.jump_count == 0);

      /* Patch the if or else instructions to point at this or the next
       * instruction respectively.
       */
      if (patch_insn->header.opcode == BRW_OPCODE_IF) {
	 /* Automagically turn it into an IFF:
	  */
	 patch_insn->header.opcode = BRW_OPCODE_IFF;
	 patch_insn->bits3.if_else.jump_count = insn - patch_insn + 1;
	 patch_insn->bits3.if_else.pop_count = 0;
	 patch_insn->bits3.if_else.pad0 = 0;
      } else if (patch_insn->header.opcode == BRW_OPCODE_ELSE) {
	 patch_insn->bits3.if_else.jump_count = insn - patch_insn + 1;
	 patch_insn->bits3.if_else.pop_count = 1;
	 patch_insn->bits3.if_else.pad0 = 0;
      } else {
	 assert(0);
      }

      /* Also pop item off the stack in the endif instruction:
       */
      insn->bits3.if_else.jump_count = 0;
      insn->bits3.if_else.pop_count = 1;
      insn->bits3.if_else.pad0 = 0;
   }
}

/* DO/WHILE loop:
 */
struct brw_instruction *brw_DO(struct brw_compile *p, GLuint execute_size)
{
   if (p->single_program_flow) {
      return &p->store[p->nr_insn];
   } else {
      struct brw_instruction *insn = next_insn(p, BRW_OPCODE_DO);

      /* Override the defaults for this instruction:
       */
      brw_set_dest(insn, retype(brw_vec1_grf(0,0), BRW_REGISTER_TYPE_UD));
      brw_set_src0(insn, retype(brw_vec1_grf(0,0), BRW_REGISTER_TYPE_UD));
      brw_set_src1(insn, retype(brw_vec1_grf(0,0), BRW_REGISTER_TYPE_UD));

      insn->header.compression_control = BRW_COMPRESSION_NONE;
      insn->header.execution_size = execute_size;
      /* insn->header.mask_control = BRW_MASK_ENABLE; */

      return insn;
   }
}



void brw_WHILE(struct brw_compile *p, 
	       struct brw_instruction *do_insn)
{
   struct brw_instruction *insn;

   if (p->single_program_flow)
      insn = next_insn(p, BRW_OPCODE_ADD);
   else
      insn = next_insn(p, BRW_OPCODE_WHILE);

   brw_set_dest(insn, brw_ip_reg());
   brw_set_src0(insn, brw_ip_reg());
   brw_set_src1(insn, brw_imm_d(0x0));

   insn->header.compression_control = BRW_COMPRESSION_NONE;

   if (p->single_program_flow) {
      insn->header.execution_size = BRW_EXECUTE_1;

      insn->bits3.d = (do_insn - insn) * 16;
   } else {
      insn->header.execution_size = do_insn->header.execution_size;

      assert(do_insn->header.opcode == BRW_OPCODE_DO);
      insn->bits3.if_else.jump_count = do_insn - insn;
      insn->bits3.if_else.pop_count = 0;
      insn->bits3.if_else.pad0 = 0;
   }

/*    insn->header.mask_control = BRW_MASK_ENABLE; */

   p->current->header.predicate_control = BRW_PREDICATE_NONE;   
}


/* FORWARD JUMPS:
 */
void brw_land_fwd_jump(struct brw_compile *p, 
		       struct brw_instruction *jmp_insn)
{
   struct brw_instruction *landing = &p->store[p->nr_insn];

   assert(jmp_insn->header.opcode == BRW_OPCODE_JMPI);
   assert(jmp_insn->bits1.da1.src1_reg_file = BRW_IMMEDIATE_VALUE);

   jmp_insn->bits3.ud = (landing - jmp_insn) - 1; 
}



/* To integrate with the above, it makes sense that the comparison
 * instruction should populate the flag register.  It might be simpler
 * just to use the flag reg for most WM tasks?
 */
void brw_CMP(struct brw_compile *p,
	     struct brw_reg dest,
	     GLuint conditional,
	     struct brw_reg src0,
	     struct brw_reg src1)
{
   struct brw_instruction *insn = next_insn(p, BRW_OPCODE_CMP);

   insn->header.destreg__conditonalmod = conditional;
   brw_set_dest(insn, dest);
   brw_set_src0(insn, src0);
   brw_set_src1(insn, src1);

/*    guess_execution_size(insn, src0); */


   /* Make it so that future instructions will use the computed flag
    * value until brw_set_predicate_control_flag_value() is called
    * again.  
    */
   if (dest.file == BRW_ARCHITECTURE_REGISTER_FILE &&
       dest.nr == 0) {
      p->current->header.predicate_control = BRW_PREDICATE_NORMAL;
      p->flag_value = 0xff;
   }
}



/***********************************************************************
 * Helpers for the various SEND message types:
 */

/* Invert 8 values
 */
void brw_math( struct brw_compile *p,
	       struct brw_reg dest,
	       GLuint function,
	       GLuint saturate,
	       GLuint msg_reg_nr,
	       struct brw_reg src,
	       GLuint data_type,
	       GLuint precision )
{
   struct brw_instruction *insn = next_insn(p, BRW_OPCODE_SEND);
   GLuint msg_length = (function == BRW_MATH_FUNCTION_POW) ? 2 : 1; 
   GLuint response_length = (function == BRW_MATH_FUNCTION_SINCOS) ? 2 : 1; 

   /* Example code doesn't set predicate_control for send
    * instructions.
    */
   insn->header.predicate_control = 0; 
   insn->header.destreg__conditonalmod = msg_reg_nr;

   brw_set_dest(insn, dest);
   brw_set_src0(insn, src);
   brw_set_math_message(insn, 
			msg_length, response_length, 
			function,
			BRW_MATH_INTEGER_UNSIGNED,
			precision,
			saturate,
			data_type);
}

/* Use 2 send instructions to invert 16 elements
 */
void brw_math_16( struct brw_compile *p,
		  struct brw_reg dest,
		  GLuint function,
		  GLuint saturate,
		  GLuint msg_reg_nr,
		  struct brw_reg src,
		  GLuint precision )
{
   struct brw_instruction *insn;
   GLuint msg_length = (function == BRW_MATH_FUNCTION_POW) ? 2 : 1; 
   GLuint response_length = (function == BRW_MATH_FUNCTION_SINCOS) ? 2 : 1; 

   /* First instruction:
    */
   brw_push_insn_state(p);
   brw_set_predicate_control_flag_value(p, 0xff);
   brw_set_compression_control(p, BRW_COMPRESSION_NONE);

   insn = next_insn(p, BRW_OPCODE_SEND);
   insn->header.destreg__conditonalmod = msg_reg_nr;

   brw_set_dest(insn, dest);
   brw_set_src0(insn, src);
   brw_set_math_message(insn, 
			msg_length, response_length, 
			function,
			BRW_MATH_INTEGER_UNSIGNED,
			precision,
			saturate,
			BRW_MATH_DATA_VECTOR);

   /* Second instruction:
    */
   insn = next_insn(p, BRW_OPCODE_SEND);
   insn->header.compression_control = BRW_COMPRESSION_2NDHALF;
   insn->header.destreg__conditonalmod = msg_reg_nr+1;

   brw_set_dest(insn, offset(dest,1));
   brw_set_src0(insn, src);
   brw_set_math_message(insn, 
			msg_length, response_length, 
			function,
			BRW_MATH_INTEGER_UNSIGNED,
			precision,
			saturate,
			BRW_MATH_DATA_VECTOR);

   brw_pop_insn_state(p);
}




void brw_dp_WRITE_16( struct brw_compile *p,
		      struct brw_reg src,
		      GLuint msg_reg_nr,
		      GLuint scratch_offset )
{
   {
      brw_push_insn_state(p);
      brw_set_mask_control(p, BRW_MASK_DISABLE);
      brw_set_compression_control(p, BRW_COMPRESSION_NONE);

      brw_MOV(p,
	      retype(brw_vec1_grf(0, 2), BRW_REGISTER_TYPE_D),
	      brw_imm_d(scratch_offset));
			   
      brw_pop_insn_state(p);
   }

   {
      GLuint msg_length = 3;
      struct brw_reg dest = retype(brw_null_reg(), BRW_REGISTER_TYPE_UW);
      struct brw_instruction *insn = next_insn(p, BRW_OPCODE_SEND);
   
      insn->header.predicate_control = 0; /* XXX */
      insn->header.compression_control = BRW_COMPRESSION_NONE; 
      insn->header.destreg__conditonalmod = msg_reg_nr;
  
      brw_set_dest(insn, dest);
      brw_set_src0(insn, src);

      brw_set_dp_write_message(insn,
			       255, /* bti */
			       BRW_DATAPORT_OWORD_BLOCK_4_OWORDS, /* msg_control */
			       BRW_DATAPORT_WRITE_MESSAGE_OWORD_BLOCK_WRITE, /* msg_type */
			       msg_length,
			       0, /* pixel scoreboard */
			       0, /* response_length */
			       0); /* eot */
   }

}


void brw_dp_READ_16( struct brw_compile *p,
		      struct brw_reg dest,
		      GLuint msg_reg_nr,
		      GLuint scratch_offset )
{
   {
      brw_push_insn_state(p);
      brw_set_compression_control(p, BRW_COMPRESSION_NONE);
      brw_set_mask_control(p, BRW_MASK_DISABLE);

      brw_MOV(p,
	      retype(brw_vec1_grf(0, 2), BRW_REGISTER_TYPE_D),
	      brw_imm_d(scratch_offset));
			   
      brw_pop_insn_state(p);
   }

   {
      struct brw_instruction *insn = next_insn(p, BRW_OPCODE_SEND);
   
      insn->header.predicate_control = 0; /* XXX */
      insn->header.compression_control = BRW_COMPRESSION_NONE; 
      insn->header.destreg__conditonalmod = msg_reg_nr;
  
      brw_set_dest(insn, dest);	/* UW? */
      brw_set_src0(insn, retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UW));

      brw_set_dp_read_message(insn,
			      255, /* bti */
			      3,  /* msg_control */
			      BRW_DATAPORT_READ_MESSAGE_OWORD_BLOCK_READ, /* msg_type */
			      1, /* target cache */
			      1, /* msg_length */
			      2, /* response_length */
			      0); /* eot */
   }
}


void brw_fb_WRITE(struct brw_compile *p,
		   struct brw_reg dest,
		   GLuint msg_reg_nr,
		   struct brw_reg src0,
		   GLuint binding_table_index,
		   GLuint msg_length,
		   GLuint response_length,
		   GLboolean eot)
{
   struct brw_instruction *insn = next_insn(p, BRW_OPCODE_SEND);
   
   insn->header.predicate_control = 0; /* XXX */
   insn->header.compression_control = BRW_COMPRESSION_NONE; 
   insn->header.destreg__conditonalmod = msg_reg_nr;
  
   brw_set_dest(insn, dest);
   brw_set_src0(insn, src0);
   brw_set_dp_write_message(insn,
			    binding_table_index,
			    BRW_DATAPORT_RENDER_TARGET_WRITE_SIMD16_SINGLE_SOURCE, /* msg_control */
			    BRW_DATAPORT_WRITE_MESSAGE_RENDER_TARGET_WRITE, /* msg_type */
			    msg_length,
			    1,	/* pixel scoreboard */
			    response_length, 
			    eot);
}



void brw_SAMPLE(struct brw_compile *p,
		struct brw_reg dest,
		GLuint msg_reg_nr,
		struct brw_reg src0,
		GLuint binding_table_index,
		GLuint sampler,
		GLuint writemask,
		GLuint msg_type,
		GLuint response_length,
		GLuint msg_length,
		GLboolean eot)
{
   GLboolean need_stall = 0;
   
   if(writemask == 0) {
/*       _mesa_printf("%s: zero writemask??\n", __FUNCTION__); */
      return;
   }
   
   /* Hardware doesn't do destination dependency checking on send
    * instructions properly.  Add a workaround which generates the
    * dependency by other means.  In practice it seems like this bug
    * only crops up for texture samples, and only where registers are
    * written by the send and then written again later without being
    * read in between.  Luckily for us, we already track that
    * information and use it to modify the writemask for the
    * instruction, so that is a guide for whether a workaround is
    * needed.
    */
   if (writemask != WRITEMASK_XYZW) {
      GLuint dst_offset = 0;
      GLuint i, newmask = 0, len = 0;

      for (i = 0; i < 4; i++) {
	 if (writemask & (1<<i))
	    break;
	 dst_offset += 2;
      }
      for (; i < 4; i++) {
	 if (!(writemask & (1<<i)))
	    break;
	 newmask |= 1<<i;
	 len++;
      }

      if (newmask != writemask) {
	 need_stall = 1;
/* 	 _mesa_printf("need stall %x %x\n", newmask , writemask); */
      }
      else {
	 struct brw_reg m1 = brw_message_reg(msg_reg_nr);
	 
	 newmask = ~newmask & WRITEMASK_XYZW;

	 brw_push_insn_state(p);

	 brw_set_compression_control(p, BRW_COMPRESSION_NONE);
	 brw_set_mask_control(p, BRW_MASK_DISABLE);

	 brw_MOV(p, m1, brw_vec8_grf(0,0));	 
  	 brw_MOV(p, get_element_ud(m1, 2), brw_imm_ud(newmask << 12)); 

	 brw_pop_insn_state(p);

  	 src0 = retype(brw_null_reg(), BRW_REGISTER_TYPE_UW); 
	 dest = offset(dest, dst_offset);
	 response_length = len * 2;
      }
   }

   {
      struct brw_instruction *insn = next_insn(p, BRW_OPCODE_SEND);
   
      insn->header.predicate_control = 0; /* XXX */
      insn->header.compression_control = BRW_COMPRESSION_NONE;
      insn->header.destreg__conditonalmod = msg_reg_nr;

      brw_set_dest(insn, dest);
      brw_set_src0(insn, src0);
      brw_set_sampler_message(insn,
			      binding_table_index,
			      sampler,
			      msg_type,
			      response_length, 
			      msg_length,
			      eot);
   }

   if (need_stall)
   {
      struct brw_reg reg = vec8(offset(dest, response_length-1));

      /*  mov (8) r9.0<1>:f    r9.0<8;8,1>:f    { Align1 }
       */
      brw_push_insn_state(p);
      brw_set_compression_control(p, GL_FALSE);
      brw_MOV(p, reg, reg);	      
      brw_pop_insn_state(p);
   }

}

/* All these variables are pretty confusing - we might be better off
 * using bitmasks and macros for this, in the old style.  Or perhaps
 * just having the caller instantiate the fields in dword3 itself.
 */
void brw_urb_WRITE(struct brw_compile *p,
		   struct brw_reg dest,
		   GLuint msg_reg_nr,
		   struct brw_reg src0,
		   GLboolean allocate,
		   GLboolean used,
		   GLuint msg_length,
		   GLuint response_length,
		   GLboolean eot,
		   GLboolean writes_complete,
		   GLuint offset,
		   GLuint swizzle)
{
   struct brw_instruction *insn = next_insn(p, BRW_OPCODE_SEND);

   assert(msg_length < 16);

   brw_set_dest(insn, dest);
   brw_set_src0(insn, src0);
   brw_set_src1(insn, brw_imm_d(0));

   insn->header.destreg__conditonalmod = msg_reg_nr;

   brw_set_urb_message(insn,
		       allocate,
		       used,
		       msg_length,
		       response_length, 
		       eot, 
		       writes_complete, 
		       offset,
		       swizzle);
}

