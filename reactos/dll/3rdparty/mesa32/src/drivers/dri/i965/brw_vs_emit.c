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
            

#include "program.h"
#include "macros.h"
#include "shader/prog_parameter.h"
#include "shader/prog_print.h"
#include "brw_context.h"
#include "brw_vs.h"



/* Do things as simply as possible.  Allocate and populate all regs
 * ahead of time.
 */
static void brw_vs_alloc_regs( struct brw_vs_compile *c )
{
   GLuint i, reg = 0, mrf;
   GLuint nr_params;

   /* r0 -- reserved as usual
    */
   c->r0 = brw_vec8_grf(reg, 0); reg++;

   /* User clip planes from curbe: 
    */
   if (c->key.nr_userclip) {
      for (i = 0; i < c->key.nr_userclip; i++) {
	 c->userplane[i] = stride( brw_vec4_grf(reg+3+i/2, (i%2) * 4), 0, 4, 1);
      }     

      /* Deal with curbe alignment:
       */
      reg += ((6+c->key.nr_userclip+3)/4)*2;
   }

   /* Vertex program parameters from curbe:
    */
   nr_params = c->vp->program.Base.Parameters->NumParameters;
   for (i = 0; i < nr_params; i++) {
      c->regs[PROGRAM_STATE_VAR][i] = stride( brw_vec4_grf(reg+i/2, (i%2) * 4), 0, 4, 1);
   }     
   reg += (nr_params+1)/2;

   c->prog_data.curb_read_length = reg - 1;



   /* Allocate input regs:  
    */
   c->nr_inputs = 0;
   for (i = 0; i < VERT_ATTRIB_MAX; i++) {
      if (c->prog_data.inputs_read & (1<<i)) {
	 c->nr_inputs++;
	 c->regs[PROGRAM_INPUT][i] = brw_vec8_grf(reg, 0);
	 reg++;
      }
   }     


   /* Allocate outputs: TODO: could organize the non-position outputs
    * to go straight into message regs.
    */
   c->nr_outputs = 0;
   c->first_output = reg;
   mrf = 4;
   for (i = 0; i < VERT_RESULT_MAX; i++) {
      if (c->prog_data.outputs_written & (1<<i)) {
	 c->nr_outputs++;
	 if (i == VERT_RESULT_HPOS) {
	    c->regs[PROGRAM_OUTPUT][i] = brw_vec8_grf(reg, 0);
	    reg++;
	 }
	 else if (i == VERT_RESULT_PSIZ) {
	    c->regs[PROGRAM_OUTPUT][i] = brw_vec8_grf(reg, 0);
	    reg++;
	    mrf++;		/* just a placeholder?  XXX fix later stages & remove this */
	 }
	 else {
	    c->regs[PROGRAM_OUTPUT][i] = brw_message_reg(mrf);
	    mrf++;
	 }
      }
   }     

   /* Allocate program temporaries:
    */
   for (i = 0; i < c->vp->program.Base.NumTemporaries; i++) {
      c->regs[PROGRAM_TEMPORARY][i] = brw_vec8_grf(reg, 0);
      reg++;
   }

   /* Address reg(s).  Don't try to use the internal address reg until
    * deref time.
    */
   for (i = 0; i < c->vp->program.Base.NumAddressRegs; i++) {
      c->regs[PROGRAM_ADDRESS][i] =  brw_reg(BRW_GENERAL_REGISTER_FILE,
					     reg,
					     0,
					     BRW_REGISTER_TYPE_D,
					     BRW_VERTICAL_STRIDE_8,
					     BRW_WIDTH_8,
					     BRW_HORIZONTAL_STRIDE_1,
					     BRW_SWIZZLE_XXXX,
					     WRITEMASK_X);
      reg++;
   }
 
   
   /* Some opcodes need an internal temporary:
    */
   c->first_tmp = reg;
   c->last_tmp = reg;		/* for allocation purposes */

   /* Each input reg holds data from two vertices.  The
    * urb_read_length is the number of registers read from *each*
    * vertex urb, so is half the amount:
    */
   c->prog_data.urb_read_length = (c->nr_inputs+1)/2;

   c->prog_data.urb_entry_size = (c->nr_outputs+2+3)/4;
   c->prog_data.total_grf = reg;
}


static struct brw_reg get_tmp( struct brw_vs_compile *c )
{
   struct brw_reg tmp = brw_vec8_grf(c->last_tmp, 0);

   if (++c->last_tmp > c->prog_data.total_grf)
      c->prog_data.total_grf = c->last_tmp;

   return tmp;
}

static void release_tmp( struct brw_vs_compile *c, struct brw_reg tmp )
{
   if (tmp.nr == c->last_tmp-1)
      c->last_tmp--;
}
			       
static void release_tmps( struct brw_vs_compile *c )
{
   c->last_tmp = c->first_tmp;
}


static void unalias1( struct brw_vs_compile *c,
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      void (*func)( struct brw_vs_compile *,
				    struct brw_reg,
				    struct brw_reg ))
{
   if (dst.file == arg0.file && dst.nr == arg0.nr) {
      struct brw_compile *p = &c->func;
      struct brw_reg tmp = brw_writemask(get_tmp(c), dst.dw1.bits.writemask);
      func(c, tmp, arg0);
      brw_MOV(p, dst, tmp);
   }
   else {
      func(c, dst, arg0);
   }
}

static void unalias2( struct brw_vs_compile *c,
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1,
		      void (*func)( struct brw_vs_compile *,
				    struct brw_reg,
				    struct brw_reg,
				    struct brw_reg ))
{
   if ((dst.file == arg0.file && dst.nr == arg0.nr) &&
       (dst.file == arg1.file && dst.nr == arg1.nr)) {
      struct brw_compile *p = &c->func;
      struct brw_reg tmp = brw_writemask(get_tmp(c), dst.dw1.bits.writemask);
      func(c, tmp, arg0, arg1);
      brw_MOV(p, dst, tmp);
   }
   else {
      func(c, dst, arg0, arg1);
   }
}




static void emit_slt( struct brw_compile *p, 
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1 )
{
   /* Could be done with an if/else/endif, but this method uses half
    * the instructions.  Note that we are careful to reference the
    * arguments before writing the dest.  That means we emit the
    * instructions in an odd order and have to play with the flag
    * values.
    */
   brw_push_insn_state(p);
   brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_GE, arg0, arg1);

   /* Write all values to 1:
    */
   brw_set_predicate_control(p, BRW_PREDICATE_NONE);
   brw_MOV(p, dst, brw_imm_f(1.0));

   /* Where the test succeeded, overwite with zero:
    */
   brw_set_predicate_control(p, BRW_PREDICATE_NORMAL);
   brw_MOV(p, dst, brw_imm_f(0.0));
   brw_pop_insn_state(p);
}


static void emit_sge( struct brw_compile *p, 
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1 )
{
   brw_push_insn_state(p);
   brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_GE, arg0, arg1);

   /* Write all values to zero:
    */
   brw_set_predicate_control(p, BRW_PREDICATE_NONE);
   brw_MOV(p, dst, brw_imm_f(0));

   /* Where the test succeeded, overwite with 1:
    */
   brw_set_predicate_control(p, BRW_PREDICATE_NORMAL);
   brw_MOV(p, dst, brw_imm_f(1.0));
   brw_pop_insn_state(p);
}


static void emit_max( struct brw_compile *p, 
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1 )
{
   brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_L, arg0, arg1);
   brw_SEL(p, dst, arg1, arg0);
   brw_set_predicate_control(p, BRW_PREDICATE_NONE);
}

static void emit_min( struct brw_compile *p, 
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1 )
{
   brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_L, arg0, arg1);
   brw_SEL(p, dst, arg0, arg1);
   brw_set_predicate_control(p, BRW_PREDICATE_NONE);
}


static void emit_math1( struct brw_vs_compile *c,
			GLuint function,
			struct brw_reg dst,
			struct brw_reg arg0,
			GLuint precision)
{
   /* There are various odd behaviours with SEND on the simulator.  In
    * addition there are documented issues with the fact that the GEN4
    * processor doesn't do dependency control properly on SEND
    * results.  So, on balance, this kludge to get around failures
    * with writemasked math results looks like it might be necessary
    * whether that turns out to be a simulator bug or not:
    */
   struct brw_compile *p = &c->func;
   struct brw_reg tmp = dst;
   GLboolean need_tmp = (dst.dw1.bits.writemask != 0xf ||
			 dst.file != BRW_GENERAL_REGISTER_FILE);

   if (need_tmp) 
      tmp = get_tmp(c);

   brw_math(p, 
	    tmp,
	    function,
	    BRW_MATH_SATURATE_NONE,
	    2,
	    arg0,
	    BRW_MATH_DATA_SCALAR,
	    precision);

   if (need_tmp) {
      brw_MOV(p, dst, tmp);
      release_tmp(c, tmp);
   }
}

static void emit_math2( struct brw_vs_compile *c, 
			GLuint function,
			struct brw_reg dst,
			struct brw_reg arg0,
			struct brw_reg arg1,
			GLuint precision)
{
   struct brw_compile *p = &c->func;
   struct brw_reg tmp = dst;
   GLboolean need_tmp = (dst.dw1.bits.writemask != 0xf ||
			 dst.file != BRW_GENERAL_REGISTER_FILE);

   if (need_tmp) 
      tmp = get_tmp(c);

   brw_MOV(p, brw_message_reg(3), arg1);
   
   brw_math(p, 
	    tmp,
	    function,
	    BRW_MATH_SATURATE_NONE,
	    2,
 	    arg0,
	    BRW_MATH_DATA_SCALAR,
	    precision);

   if (need_tmp) {
      brw_MOV(p, dst, tmp);
      release_tmp(c, tmp);
   }
}
		     


static void emit_exp_noalias( struct brw_vs_compile *c,
			      struct brw_reg dst,
			      struct brw_reg arg0 )
{
   struct brw_compile *p = &c->func;
   

   if (dst.dw1.bits.writemask & WRITEMASK_X) {
      struct brw_reg tmp = get_tmp(c);
      struct brw_reg tmp_d = retype(tmp, BRW_REGISTER_TYPE_D);

      /* tmp_d = floor(arg0.x) */
      brw_RNDD(p, tmp_d, brw_swizzle1(arg0, 0));

      /* result[0] = 2.0 ^ tmp */

      /* Adjust exponent for floating point: 
       * exp += 127 
       */
      brw_ADD(p, brw_writemask(tmp_d, WRITEMASK_X), tmp_d, brw_imm_d(127));

      /* Install exponent and sign.  
       * Excess drops off the edge: 
       */
      brw_SHL(p, brw_writemask(retype(dst, BRW_REGISTER_TYPE_D), WRITEMASK_X), 
	      tmp_d, brw_imm_d(23));

      release_tmp(c, tmp);
   }

   if (dst.dw1.bits.writemask & WRITEMASK_Y) {
      /* result[1] = arg0.x - floor(arg0.x) */
      brw_FRC(p, brw_writemask(dst, WRITEMASK_Y), brw_swizzle1(arg0, 0));
   }
   
   if (dst.dw1.bits.writemask & WRITEMASK_Z) {
      /* As with the LOG instruction, we might be better off just
       * doing a taylor expansion here, seeing as we have to do all
       * the prep work.
       *
       * If mathbox partial precision is too low, consider also:
       * result[3] = result[0] * EXP(result[1])
       */
      emit_math1(c, 
		 BRW_MATH_FUNCTION_EXP, 
		 brw_writemask(dst, WRITEMASK_Z),
		 brw_swizzle1(arg0, 0), 
		 BRW_MATH_PRECISION_PARTIAL);
   }  

   if (dst.dw1.bits.writemask & WRITEMASK_W) {
      /* result[3] = 1.0; */
      brw_MOV(p, brw_writemask(dst, WRITEMASK_W), brw_imm_f(1));
   }
}


static void emit_log_noalias( struct brw_vs_compile *c,
			      struct brw_reg dst,
			      struct brw_reg arg0 )
{
   struct brw_compile *p = &c->func;
   struct brw_reg tmp = dst;
   struct brw_reg tmp_ud = retype(tmp, BRW_REGISTER_TYPE_UD);
   struct brw_reg arg0_ud = retype(arg0, BRW_REGISTER_TYPE_UD);
   GLboolean need_tmp = (dst.dw1.bits.writemask != 0xf ||
			 dst.file != BRW_GENERAL_REGISTER_FILE);

   if (need_tmp) {
      tmp = get_tmp(c);
      tmp_ud = retype(tmp, BRW_REGISTER_TYPE_UD);
   }
   
   /* Perform mant = frexpf(fabsf(x), &exp), adjust exp and mnt
    * according to spec:
    *
    * These almost look likey they could be joined up, but not really
    * practical:
    *
    * result[0].f = (x.i & ((1<<31)-1) >> 23) - 127
    * result[1].i = (x.i & ((1<<23)-1)        + (127<<23)
    */
   if (dst.dw1.bits.writemask & WRITEMASK_XZ) {
      brw_AND(p, 
	      brw_writemask(tmp_ud, WRITEMASK_X),
	      brw_swizzle1(arg0_ud, 0),
	      brw_imm_ud((1U<<31)-1));

      brw_SHR(p, 
	      brw_writemask(tmp_ud, WRITEMASK_X), 
	      tmp_ud,
	      brw_imm_ud(23));

      brw_ADD(p, 
	      brw_writemask(tmp, WRITEMASK_X), 
	      retype(tmp_ud, BRW_REGISTER_TYPE_D),	/* does it matter? */
	      brw_imm_d(-127));
   }

   if (dst.dw1.bits.writemask & WRITEMASK_YZ) {
      brw_AND(p, 
	      brw_writemask(tmp_ud, WRITEMASK_Y),
	      brw_swizzle1(arg0_ud, 0),
	      brw_imm_ud((1<<23)-1));

      brw_OR(p, 
	     brw_writemask(tmp_ud, WRITEMASK_Y), 
	     tmp_ud,
	     brw_imm_ud(127<<23));
   }
   
   if (dst.dw1.bits.writemask & WRITEMASK_Z) {
      /* result[2] = result[0] + LOG2(result[1]); */

      /* Why bother?  The above is just a hint how to do this with a
       * taylor series.  Maybe we *should* use a taylor series as by
       * the time all the above has been done it's almost certainly
       * quicker than calling the mathbox, even with low precision.
       * 
       * Options are:
       *    - result[0] + mathbox.LOG2(result[1])
       *    - mathbox.LOG2(arg0.x)
       *    - result[0] + inline_taylor_approx(result[1])
       */
      emit_math1(c, 
		 BRW_MATH_FUNCTION_LOG, 
		 brw_writemask(tmp, WRITEMASK_Z), 
		 brw_swizzle1(tmp, 1), 
		 BRW_MATH_PRECISION_FULL);
      
      brw_ADD(p, 
	      brw_writemask(tmp, WRITEMASK_Z), 
	      brw_swizzle1(tmp, 2), 
	      brw_swizzle1(tmp, 0));
   }  

   if (dst.dw1.bits.writemask & WRITEMASK_W) {
      /* result[3] = 1.0; */
      brw_MOV(p, brw_writemask(tmp, WRITEMASK_W), brw_imm_f(1));
   }

   if (need_tmp) {
      brw_MOV(p, dst, tmp);
      release_tmp(c, tmp);
   }
}


      

/* Need to unalias - consider swizzles:   r0 = DST r0.xxxx r1
 */
static void emit_dst_noalias( struct brw_vs_compile *c, 
			      struct brw_reg dst,
			      struct brw_reg arg0,
			      struct brw_reg arg1)
{
   struct brw_compile *p = &c->func;

   /* There must be a better way to do this: 
    */
   if (dst.dw1.bits.writemask & WRITEMASK_X)
      brw_MOV(p, brw_writemask(dst, WRITEMASK_X), brw_imm_f(1.0));
   if (dst.dw1.bits.writemask & WRITEMASK_Y)
      brw_MUL(p, brw_writemask(dst, WRITEMASK_Y), arg0, arg1);
   if (dst.dw1.bits.writemask & WRITEMASK_Z)
      brw_MOV(p, brw_writemask(dst, WRITEMASK_Z), arg0);
   if (dst.dw1.bits.writemask & WRITEMASK_W)
      brw_MOV(p, brw_writemask(dst, WRITEMASK_W), arg1);
}

static void emit_xpd( struct brw_compile *p,
		      struct brw_reg dst,
		      struct brw_reg t,
		      struct brw_reg u)
{
   brw_MUL(p, brw_null_reg(), brw_swizzle(t, 1,2,0,3),  brw_swizzle(u,2,0,1,3));
   brw_MAC(p, dst,     negate(brw_swizzle(t, 2,0,1,3)), brw_swizzle(u,1,2,0,3));
}



static void emit_lit_noalias( struct brw_vs_compile *c, 
			      struct brw_reg dst,
			      struct brw_reg arg0 )
{
   struct brw_compile *p = &c->func;
   struct brw_instruction *if_insn;
   struct brw_reg tmp = dst;
   GLboolean need_tmp = (dst.file != BRW_GENERAL_REGISTER_FILE);

   if (need_tmp) 
      tmp = get_tmp(c);
   
   brw_MOV(p, brw_writemask(dst, WRITEMASK_YZ), brw_imm_f(0)); 
   brw_MOV(p, brw_writemask(dst, WRITEMASK_XW), brw_imm_f(1)); 

   /* Need to use BRW_EXECUTE_8 and also do an 8-wide compare in order
    * to get all channels active inside the IF.  In the clipping code
    * we run with NoMask, so it's not an option and we can use
    * BRW_EXECUTE_1 for all comparisions.
    */
   brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_G, brw_swizzle1(arg0,0), brw_imm_f(0));
   if_insn = brw_IF(p, BRW_EXECUTE_8);
   {
      brw_MOV(p, brw_writemask(dst, WRITEMASK_Y), brw_swizzle1(arg0,0));

      brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_G, brw_swizzle1(arg0,1), brw_imm_f(0));
      brw_MOV(p, brw_writemask(tmp, WRITEMASK_Z),  brw_swizzle1(arg0,1));
      brw_set_predicate_control(p, BRW_PREDICATE_NONE);

      emit_math2(c, 
		 BRW_MATH_FUNCTION_POW, 
		 brw_writemask(dst, WRITEMASK_Z),
		 brw_swizzle1(tmp, 2),
		 brw_swizzle1(arg0, 3),
		 BRW_MATH_PRECISION_PARTIAL);      
   }

   brw_ENDIF(p, if_insn);
}





/* TODO: relative addressing!
 */
static struct brw_reg get_reg( struct brw_vs_compile *c,
			       GLuint file,
			       GLuint index )
{

   switch (file) {
   case PROGRAM_TEMPORARY:
   case PROGRAM_INPUT:
   case PROGRAM_OUTPUT:
   case PROGRAM_STATE_VAR:
      assert(c->regs[file][index].nr != 0);
      return c->regs[file][index];
   case PROGRAM_ADDRESS:
      assert(index == 0);
      return c->regs[file][index];

   case PROGRAM_UNDEFINED:			/* undef values */
      return brw_null_reg();

   case PROGRAM_LOCAL_PARAM: 
   case PROGRAM_ENV_PARAM: 
   case PROGRAM_WRITE_ONLY:
   default:
      assert(0);
      return brw_null_reg();
   }
}



static struct brw_reg deref( struct brw_vs_compile *c,
			     struct brw_reg arg,
			     GLint offset)
{
   struct brw_compile *p = &c->func;
   struct brw_reg tmp = vec4(get_tmp(c));
   struct brw_reg vp_address = retype(vec1(get_reg(c, PROGRAM_ADDRESS, 0)), BRW_REGISTER_TYPE_UW);
   GLuint byte_offset = arg.nr * 32 + arg.subnr + offset * 16;
   struct brw_reg indirect = brw_vec4_indirect(0,0);

   {
      brw_push_insn_state(p);
      brw_set_access_mode(p, BRW_ALIGN_1);

      /* This is pretty clunky - load the address register twice and
       * fetch each 4-dword value in turn.  There must be a way to do
       * this in a single pass, but I couldn't get it to work.
       */
      brw_ADD(p, brw_address_reg(0), vp_address, brw_imm_d(byte_offset));
      brw_MOV(p, tmp, indirect);

      brw_ADD(p, brw_address_reg(0), suboffset(vp_address, 8), brw_imm_d(byte_offset));
      brw_MOV(p, suboffset(tmp, 4), indirect);

      brw_pop_insn_state(p);
   }
   
   return vec8(tmp);
}


static void emit_arl( struct brw_vs_compile *c,
		      struct brw_reg dst,
		      struct brw_reg arg0 )
{
   struct brw_compile *p = &c->func;
   struct brw_reg tmp = dst;
   GLboolean need_tmp = (dst.file != BRW_GENERAL_REGISTER_FILE);
   
   if (need_tmp) 
      tmp = get_tmp(c);

   brw_RNDD(p, tmp, arg0);
   brw_MUL(p, dst, tmp, brw_imm_d(16));

   if (need_tmp)
      release_tmp(c, tmp);
}


/* Will return mangled results for SWZ op.  The emit_swz() function
 * ignores this result and recalculates taking extended swizzles into
 * account.
 */
static struct brw_reg get_arg( struct brw_vs_compile *c,
			       struct prog_src_register src )
{
   struct brw_reg reg;

   if (src.File == PROGRAM_UNDEFINED)
      return brw_null_reg();

   if (src.RelAddr) 
      reg = deref(c, c->regs[PROGRAM_STATE_VAR][0], src.Index);
   else
      reg = get_reg(c, src.File, src.Index);

   /* Convert 3-bit swizzle to 2-bit.  
    */
   reg.dw1.bits.swizzle = BRW_SWIZZLE4(GET_SWZ(src.Swizzle, 0),
				       GET_SWZ(src.Swizzle, 1),
				       GET_SWZ(src.Swizzle, 2),
				       GET_SWZ(src.Swizzle, 3));

   /* Note this is ok for non-swizzle instructions: 
    */
   reg.negate = src.NegateBase ? 1 : 0;   

   return reg;
}


static struct brw_reg get_dst( struct brw_vs_compile *c,
			       struct prog_dst_register dst )
{
   struct brw_reg reg = get_reg(c, dst.File, dst.Index);

   reg.dw1.bits.writemask = dst.WriteMask;

   return reg;
}




static void emit_swz( struct brw_vs_compile *c, 
		      struct brw_reg dst,
		      struct prog_src_register src )
{
   struct brw_compile *p = &c->func;
   GLuint zeros_mask = 0;
   GLuint ones_mask = 0;
   GLuint src_mask = 0;
   GLubyte src_swz[4];
   GLboolean need_tmp = (src.NegateBase &&
			 dst.file != BRW_GENERAL_REGISTER_FILE);
   struct brw_reg tmp = dst;
   GLuint i;

   if (need_tmp)
      tmp = get_tmp(c);

   for (i = 0; i < 4; i++) {
      if (dst.dw1.bits.writemask & (1<<i)) {
	 GLubyte s = GET_SWZ(src.Swizzle, i);
	 switch (s) {
	 case SWIZZLE_X:
	 case SWIZZLE_Y:
	 case SWIZZLE_Z:
	 case SWIZZLE_W:
	    src_mask |= 1<<i;
	    src_swz[i] = s;
	    break;
	 case SWIZZLE_ZERO:
	    zeros_mask |= 1<<i;
	    break;
	 case SWIZZLE_ONE:
	    ones_mask |= 1<<i;
	    break;
	 }
      }
   }
   
   /* Do src first, in case dst aliases src:
    */
   if (src_mask) {
      struct brw_reg arg0;

      if (src.RelAddr) 
	 arg0 = deref(c, c->regs[PROGRAM_STATE_VAR][0], src.Index);
      else
	 arg0 = get_reg(c, src.File, src.Index);

      arg0 = brw_swizzle(arg0, 
			 src_swz[0], src_swz[1], 
			 src_swz[2], src_swz[3]);

      brw_MOV(p, brw_writemask(tmp, src_mask), arg0);
   } 
   
   if (zeros_mask) 
      brw_MOV(p, brw_writemask(tmp, zeros_mask), brw_imm_f(0));

   if (ones_mask) 
      brw_MOV(p, brw_writemask(tmp, ones_mask), brw_imm_f(1));

   if (src.NegateBase)
      brw_MOV(p, brw_writemask(tmp, src.NegateBase), negate(tmp));
   
   if (need_tmp) {
      brw_MOV(p, dst, tmp);
      release_tmp(c, tmp);
   }
}



/* Post-vertex-program processing.  Send the results to the URB.
 */
static void emit_vertex_write( struct brw_vs_compile *c)
{
   struct brw_compile *p = &c->func;
   struct brw_reg m0 = brw_message_reg(0);
   struct brw_reg pos = c->regs[PROGRAM_OUTPUT][VERT_RESULT_HPOS];
   struct brw_reg ndc;

   if (c->key.copy_edgeflag) {
      brw_MOV(p, 
	      get_reg(c, PROGRAM_OUTPUT, VERT_RESULT_EDGE),
	      get_reg(c, PROGRAM_INPUT, VERT_ATTRIB_EDGEFLAG));
   }


   /* Build ndc coords?   TODO: Shortcircuit when w is known to be one.
    */
   if (!c->key.know_w_is_one) {
      ndc = get_tmp(c);
      emit_math1(c, BRW_MATH_FUNCTION_INV, ndc, brw_swizzle1(pos, 3), BRW_MATH_PRECISION_FULL);
      brw_MUL(p, brw_writemask(ndc, WRITEMASK_XYZ), pos, ndc);
   }
   else {
      ndc = pos;
   }

   /* This includes the workaround for -ve rhw, so is no longer an
    * optional step:
    */
   if ((c->prog_data.outputs_written & (1<<VERT_RESULT_PSIZ)) ||
       c->key.nr_userclip ||
       !c->key.know_w_is_one)
   {
      struct brw_reg header1 = retype(get_tmp(c), BRW_REGISTER_TYPE_UD);
      GLuint i;

      brw_MOV(p, header1, brw_imm_ud(0));

      brw_set_access_mode(p, BRW_ALIGN_16);	

      if (c->prog_data.outputs_written & (1<<VERT_RESULT_PSIZ)) {
	 struct brw_reg psiz = c->regs[PROGRAM_OUTPUT][VERT_RESULT_PSIZ];
	 brw_MUL(p, brw_writemask(header1, WRITEMASK_W), brw_swizzle1(psiz, 0), brw_imm_f(1<<11));
	 brw_AND(p, brw_writemask(header1, WRITEMASK_W), header1, brw_imm_ud(0x7ff<<8));
      }


      for (i = 0; i < c->key.nr_userclip; i++) {
	 brw_set_conditionalmod(p, BRW_CONDITIONAL_L);
	 brw_DP4(p, brw_null_reg(), pos, c->userplane[i]);
	 brw_OR(p, brw_writemask(header1, WRITEMASK_W), header1, brw_imm_ud(1<<i));
	 brw_set_predicate_control(p, BRW_PREDICATE_NONE);
      }


      /* i965 clipping workaround: 
       * 1) Test for -ve rhw
       * 2) If set, 
       *      set ndc = (0,0,0,0)
       *      set ucp[6] = 1
       *
       * Later, clipping will detect ucp[6] and ensure the primitive is
       * clipped against all fixed planes.
       */
      if (!c->key.know_w_is_one) {
	 brw_CMP(p,
		 vec8(brw_null_reg()),
		 BRW_CONDITIONAL_L,
		 brw_swizzle1(ndc, 3),
		 brw_imm_f(0));
   
	 brw_OR(p, brw_writemask(header1, WRITEMASK_W), header1, brw_imm_ud(1<<6));
	 brw_MOV(p, ndc, brw_imm_f(0));
	 brw_set_predicate_control(p, BRW_PREDICATE_NONE);
      }

      brw_set_access_mode(p, BRW_ALIGN_1);	/* why? */
      brw_MOV(p, retype(brw_message_reg(1), BRW_REGISTER_TYPE_UD), header1);
      brw_set_access_mode(p, BRW_ALIGN_16);

      release_tmp(c, header1);
   }
   else {
      brw_MOV(p, retype(brw_message_reg(1), BRW_REGISTER_TYPE_UD), brw_imm_ud(0));
   }


   /* Emit the (interleaved) headers for the two vertices - an 8-reg
    * of zeros followed by two sets of NDC coordinates:
    */
   brw_set_access_mode(p, BRW_ALIGN_1);
   brw_MOV(p, offset(m0, 2), ndc);
   brw_MOV(p, offset(m0, 3), pos);
   

   brw_urb_WRITE(p, 
		 brw_null_reg(), /* dest */
		 0,		/* starting mrf reg nr */
		 c->r0,		/* src */
		 0,		/* allocate */
		 1,		/* used */
		 c->nr_outputs + 3, /* msg len */
		 0,		/* response len */
		 1, 		/* eot */
		 1, 		/* writes complete */
		 0, 		/* urb destination offset */
		 BRW_URB_SWIZZLE_INTERLEAVE);

}




/* Emit the fragment program instructions here.
 */
void brw_vs_emit( struct brw_vs_compile *c )
{
   struct brw_compile *p = &c->func;
   GLuint nr_insns = c->vp->program.Base.NumInstructions;
   GLuint insn;


   if (INTEL_DEBUG & DEBUG_VS) {
      _mesa_printf("\n\n\nvs-emit:\n");
      _mesa_print_program(&c->vp->program.Base); 
      _mesa_printf("\n");
   }

   brw_set_compression_control(p, BRW_COMPRESSION_NONE);
   brw_set_access_mode(p, BRW_ALIGN_16);
   
   /* Static register allocation
    */
   brw_vs_alloc_regs(c);

   for (insn = 0; insn < nr_insns; insn++) {

      struct prog_instruction *inst = &c->vp->program.Base.Instructions[insn];
      struct brw_reg args[3], dst;
      GLuint i;
      
      /* Get argument regs.  SWZ is special and does this itself.
       */
      if (inst->Opcode != OPCODE_SWZ)
	 for (i = 0; i < 3; i++) 
	    args[i] = get_arg(c, inst->SrcReg[i]);

      /* Get dest regs.  Note that it is possible for a reg to be both
       * dst and arg, given the static allocation of registers.  So
       * care needs to be taken emitting multi-operation instructions.
       */
      dst = get_dst(c, inst->DstReg);

      
      switch (inst->Opcode) {
      case OPCODE_ABS:
	 brw_MOV(p, dst, brw_abs(args[0]));
	 break;
      case OPCODE_ADD:
	 brw_ADD(p, dst, args[0], args[1]);
	 break;
      case OPCODE_DP3:
	 brw_DP3(p, dst, args[0], args[1]);
	 break;
      case OPCODE_DP4:
	 brw_DP4(p, dst, args[0], args[1]);
	 break;
      case OPCODE_DPH:
	 brw_DPH(p, dst, args[0], args[1]);
	 break;
      case OPCODE_DST:
	 unalias2(c, dst, args[0], args[1], emit_dst_noalias); 
	 break;
      case OPCODE_EXP:
	 unalias1(c, dst, args[0], emit_exp_noalias);
	 break;
      case OPCODE_EX2:
	 emit_math1(c, BRW_MATH_FUNCTION_EXP, dst, args[0], BRW_MATH_PRECISION_FULL);
	 break;
      case OPCODE_ARL:
	 emit_arl(c, dst, args[0]);
	 break;
      case OPCODE_FLR:
	 brw_RNDD(p, dst, args[0]);
	 break;
      case OPCODE_FRC:
	 brw_FRC(p, dst, args[0]);
	 break;
      case OPCODE_LOG:
	 unalias1(c, dst, args[0], emit_log_noalias);
	 break;
      case OPCODE_LG2:
	 emit_math1(c, BRW_MATH_FUNCTION_LOG, dst, args[0], BRW_MATH_PRECISION_FULL);
	 break;
      case OPCODE_LIT:
	 unalias1(c, dst, args[0], emit_lit_noalias);
	 break;
      case OPCODE_MAD:
	 brw_MOV(p, brw_acc_reg(), args[2]);
	 brw_MAC(p, dst, args[0], args[1]);
	 break;
      case OPCODE_MAX:
	 emit_max(p, dst, args[0], args[1]);
	 break;
      case OPCODE_MIN:
	 emit_min(p, dst, args[0], args[1]);
	 break;
      case OPCODE_MOV:
	 brw_MOV(p, dst, args[0]);
	 break;
      case OPCODE_MUL:
	 brw_MUL(p, dst, args[0], args[1]);
	 break;
      case OPCODE_POW:
	 emit_math2(c, BRW_MATH_FUNCTION_POW, dst, args[0], args[1], BRW_MATH_PRECISION_FULL); 
	 break;
      case OPCODE_RCP:
	 emit_math1(c, BRW_MATH_FUNCTION_INV, dst, args[0], BRW_MATH_PRECISION_FULL);
	 break;
      case OPCODE_RSQ:
	 emit_math1(c, BRW_MATH_FUNCTION_RSQ, dst, args[0], BRW_MATH_PRECISION_FULL);
	 break;
      case OPCODE_SGE:
	 emit_sge(p, dst, args[0], args[1]);
	 break;
      case OPCODE_SLT:
	 emit_slt(p, dst, args[0], args[1]);
	 break;
      case OPCODE_SUB:
	 brw_ADD(p, dst, args[0], negate(args[1]));
	 break;
      case OPCODE_SWZ:
	 /* The args[0] value can't be used here as it won't have
	  * correctly encoded the full swizzle:
	  */
	 emit_swz(c, dst, inst->SrcReg[0] );
	 break;
      case OPCODE_XPD:
	 emit_xpd(p, dst, args[0], args[1]);
	 break;
      case OPCODE_END:	
      case OPCODE_PRINT:
	 break;
      default:
	 break;
      }

      release_tmps(c);
   }

   emit_vertex_write(c);

}





