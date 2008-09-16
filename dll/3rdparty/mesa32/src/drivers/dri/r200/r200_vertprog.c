/**************************************************************************

Copyright (C) 2005 Aapo Tahkola.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Aapo Tahkola <aet@rasterburn.org>
 *   Roland Scheidegger <rscheidegger_lists@hispeed.ch>
 */
#include "glheader.h"
#include "macros.h"
#include "enums.h"
#include "program.h"
#include "shader/prog_instruction.h"
#include "shader/prog_parameter.h"
#include "shader/prog_statevars.h"
#include "shader/programopt.h"
#include "tnl/tnl.h"

#include "r200_context.h"
#include "r200_vertprog.h"
#include "r200_ioctl.h"
#include "r200_tcl.h"

#if SWIZZLE_X != VSF_IN_COMPONENT_X || \
    SWIZZLE_Y != VSF_IN_COMPONENT_Y || \
    SWIZZLE_Z != VSF_IN_COMPONENT_Z || \
    SWIZZLE_W != VSF_IN_COMPONENT_W || \
    SWIZZLE_ZERO != VSF_IN_COMPONENT_ZERO || \
    SWIZZLE_ONE != VSF_IN_COMPONENT_ONE || \
    WRITEMASK_X != VSF_FLAG_X || \
    WRITEMASK_Y != VSF_FLAG_Y || \
    WRITEMASK_Z != VSF_FLAG_Z || \
    WRITEMASK_W != VSF_FLAG_W
#error Cannot change these!
#endif

#define SCALAR_FLAG (1<<31)
#define FLAG_MASK (1<<31)
#define OP_MASK (0xf)  /* we are unlikely to have more than 15 */
#define OPN(operator, ip) {#operator, OPCODE_##operator, ip}

static struct{
   char *name;
   int opcode;
   unsigned long ip; /* number of input operands and flags */
}op_names[]={
   OPN(ABS, 1),
   OPN(ADD, 2),
   OPN(ARL, 1|SCALAR_FLAG),
   OPN(DP3, 2),
   OPN(DP4, 2),
   OPN(DPH, 2),
   OPN(DST, 2),
   OPN(EX2, 1|SCALAR_FLAG),
   OPN(EXP, 1|SCALAR_FLAG),
   OPN(FLR, 1),
   OPN(FRC, 1),
   OPN(LG2, 1|SCALAR_FLAG),
   OPN(LIT, 1),
   OPN(LOG, 1|SCALAR_FLAG),
   OPN(MAD, 3),
   OPN(MAX, 2),
   OPN(MIN, 2),
   OPN(MOV, 1),
   OPN(MUL, 2),
   OPN(POW, 2|SCALAR_FLAG),
   OPN(RCP, 1|SCALAR_FLAG),
   OPN(RSQ, 1|SCALAR_FLAG),
   OPN(SGE, 2),
   OPN(SLT, 2),
   OPN(SUB, 2),
   OPN(SWZ, 1),
   OPN(XPD, 2),
   OPN(PRINT, 0),
   OPN(END, 0),
};
#undef OPN

static GLboolean r200VertexProgUpdateParams(GLcontext *ctx, struct r200_vertex_program *vp)
{
   r200ContextPtr rmesa = R200_CONTEXT( ctx );
   GLfloat *fcmd = (GLfloat *)&rmesa->hw.vpp[0].cmd[VPP_CMD_0 + 1];
   int pi;
   struct gl_vertex_program *mesa_vp = &vp->mesa_program;
   struct gl_program_parameter_list *paramList;
   drm_radeon_cmd_header_t tmp;

   R200_STATECHANGE( rmesa, vpp[0] );
   R200_STATECHANGE( rmesa, vpp[1] );
   assert(mesa_vp->Base.Parameters);
   _mesa_load_state_parameters(ctx, mesa_vp->Base.Parameters);
   paramList = mesa_vp->Base.Parameters;

   if(paramList->NumParameters > R200_VSF_MAX_PARAM){
      fprintf(stderr, "%s:Params exhausted\n", __FUNCTION__);
      return GL_FALSE;
   }

   for(pi = 0; pi < paramList->NumParameters; pi++) {
      switch(paramList->Parameters[pi].Type) {
      case PROGRAM_STATE_VAR:
      case PROGRAM_NAMED_PARAM:
      //fprintf(stderr, "%s", vp->Parameters->Parameters[pi].Name);
      case PROGRAM_CONSTANT:
	 *fcmd++ = paramList->ParameterValues[pi][0];
	 *fcmd++ = paramList->ParameterValues[pi][1];
	 *fcmd++ = paramList->ParameterValues[pi][2];
	 *fcmd++ = paramList->ParameterValues[pi][3];
	 break;
      default:
	 _mesa_problem(NULL, "Bad param type in %s", __FUNCTION__);
	 break;
      }
      if (pi == 95) {
	 fcmd = (GLfloat *)&rmesa->hw.vpp[1].cmd[VPP_CMD_0 + 1];
      }
   }
   /* hack up the cmd_size so not the whole state atom is emitted always. */
   rmesa->hw.vpp[0].cmd_size =
      1 + 4 * ((paramList->NumParameters > 96) ? 96 : paramList->NumParameters);
   tmp.i = rmesa->hw.vpp[0].cmd[VPP_CMD_0];
   tmp.veclinear.count = (paramList->NumParameters > 96) ? 96 : paramList->NumParameters;
   rmesa->hw.vpp[0].cmd[VPP_CMD_0] = tmp.i;
   if (paramList->NumParameters > 96) {
      rmesa->hw.vpp[1].cmd_size = 1 + 4 * (paramList->NumParameters - 96);
      tmp.i = rmesa->hw.vpp[1].cmd[VPP_CMD_0];
      tmp.veclinear.count = paramList->NumParameters - 96;
      rmesa->hw.vpp[1].cmd[VPP_CMD_0] = tmp.i;
   }
   return GL_TRUE;
}

static __inline unsigned long t_dst_mask(GLuint mask)
{
   /* WRITEMASK_* is equivalent to VSF_FLAG_* */
   return mask & VSF_FLAG_ALL;
}

static unsigned long t_dst(struct prog_dst_register *dst)
{
   switch(dst->File) {
   case PROGRAM_TEMPORARY:
      return ((dst->Index << R200_VPI_OUT_REG_INDEX_SHIFT)
	 | R200_VSF_OUT_CLASS_TMP);
   case PROGRAM_OUTPUT:
      switch (dst->Index) {
      case VERT_RESULT_HPOS:
	 return R200_VSF_OUT_CLASS_RESULT_POS;
      case VERT_RESULT_COL0:
	 return R200_VSF_OUT_CLASS_RESULT_COLOR;
      case VERT_RESULT_COL1:
	 return ((1 << R200_VPI_OUT_REG_INDEX_SHIFT)
	    | R200_VSF_OUT_CLASS_RESULT_COLOR);
      case VERT_RESULT_FOGC:
	 return R200_VSF_OUT_CLASS_RESULT_FOGC;
      case VERT_RESULT_TEX0:
      case VERT_RESULT_TEX1:
      case VERT_RESULT_TEX2:
      case VERT_RESULT_TEX3:
      case VERT_RESULT_TEX4:
      case VERT_RESULT_TEX5:
	 return (((dst->Index - VERT_RESULT_TEX0) << R200_VPI_OUT_REG_INDEX_SHIFT)
	    | R200_VSF_OUT_CLASS_RESULT_TEXC);
      case VERT_RESULT_PSIZ:
	 return R200_VSF_OUT_CLASS_RESULT_POINTSIZE;
      default:
	 fprintf(stderr, "problem in %s, unknown dst output reg %d\n", __FUNCTION__, dst->Index);
	 exit(0);
	 return 0;
      }
   case PROGRAM_ADDRESS:
      assert (dst->Index == 0);
      return R200_VSF_OUT_CLASS_ADDR;
   default:
      fprintf(stderr, "problem in %s, unknown register type %d\n", __FUNCTION__, dst->File);
      exit(0);
      return 0;
   }
}

static unsigned long t_src_class(enum register_file file)
{

   switch(file){
   case PROGRAM_TEMPORARY:
      return VSF_IN_CLASS_TMP;

   case PROGRAM_INPUT:
      return VSF_IN_CLASS_ATTR;

   case PROGRAM_LOCAL_PARAM:
   case PROGRAM_ENV_PARAM:
   case PROGRAM_NAMED_PARAM:
   case PROGRAM_STATE_VAR:
      return VSF_IN_CLASS_PARAM;
   /*
   case PROGRAM_OUTPUT:
   case PROGRAM_WRITE_ONLY:
   case PROGRAM_ADDRESS:
   */
   default:
      fprintf(stderr, "problem in %s", __FUNCTION__);
      exit(0);
   }
}

static __inline unsigned long t_swizzle(GLubyte swizzle)
{
/* this is in fact a NOP as the Mesa SWIZZLE_* are all identical to VSF_IN_COMPONENT_* */
   return swizzle;
}

#if 0
static void vp_dump_inputs(struct r200_vertex_program *vp, char *caller)
{
   int i;

   if(vp == NULL){
      fprintf(stderr, "vp null in call to %s from %s\n", __FUNCTION__, caller);
      return ;
   }

   fprintf(stderr, "%s:<", caller);
   for(i=0; i < VERT_ATTRIB_MAX; i++)
   fprintf(stderr, "%d ", vp->inputs[i]);
   fprintf(stderr, ">\n");

}
#endif

static unsigned long t_src_index(struct r200_vertex_program *vp, struct prog_src_register *src)
{
/*
   int i;
   int max_reg = -1;
*/
   if(src->File == PROGRAM_INPUT){
/*      if(vp->inputs[src->Index] != -1)
	 return vp->inputs[src->Index];

      for(i=0; i < VERT_ATTRIB_MAX; i++)
	 if(vp->inputs[i] > max_reg)
	    max_reg = vp->inputs[i];

      vp->inputs[src->Index] = max_reg+1;*/

      //vp_dump_inputs(vp, __FUNCTION__);	
      assert(vp->inputs[src->Index] != -1);
      return vp->inputs[src->Index];
   } else {
      if (src->Index < 0) {
	 fprintf(stderr, "WARNING negative offsets for indirect addressing do not work\n");
	 return 0;
      }
      return src->Index;
   }
}

static unsigned long t_src(struct r200_vertex_program *vp, struct prog_src_register *src)
{

   return MAKE_VSF_SOURCE(t_src_index(vp, src),
			t_swizzle(GET_SWZ(src->Swizzle, 0)),
			t_swizzle(GET_SWZ(src->Swizzle, 1)),
			t_swizzle(GET_SWZ(src->Swizzle, 2)),
			t_swizzle(GET_SWZ(src->Swizzle, 3)),
			t_src_class(src->File),
			src->NegateBase) | (src->RelAddr << 4);
}

static unsigned long t_src_scalar(struct r200_vertex_program *vp, struct prog_src_register *src)
{

   return MAKE_VSF_SOURCE(t_src_index(vp, src),
			t_swizzle(GET_SWZ(src->Swizzle, 0)),
			t_swizzle(GET_SWZ(src->Swizzle, 0)),
			t_swizzle(GET_SWZ(src->Swizzle, 0)),
			t_swizzle(GET_SWZ(src->Swizzle, 0)),
			t_src_class(src->File),
			src->NegateBase ? VSF_FLAG_ALL : VSF_FLAG_NONE) | (src->RelAddr << 4);
}

static unsigned long t_opcode(enum prog_opcode opcode)
{

   switch(opcode){
   case OPCODE_ADD: return R200_VPI_OUT_OP_ADD;
   /* FIXME: ARL works fine, but negative offsets won't work - fglrx just
    * seems to ignore neg offsets which isn't quite correct...
    */
   case OPCODE_ARL: return R200_VPI_OUT_OP_ARL;
   case OPCODE_DP4: return R200_VPI_OUT_OP_DOT;
   case OPCODE_DST: return R200_VPI_OUT_OP_DST;
   case OPCODE_EX2: return R200_VPI_OUT_OP_EX2;
   case OPCODE_EXP: return R200_VPI_OUT_OP_EXP;
   case OPCODE_FRC: return R200_VPI_OUT_OP_FRC;
   case OPCODE_LG2: return R200_VPI_OUT_OP_LG2;
   case OPCODE_LIT: return R200_VPI_OUT_OP_LIT;
   case OPCODE_LOG: return R200_VPI_OUT_OP_LOG;
   case OPCODE_MAX: return R200_VPI_OUT_OP_MAX;
   case OPCODE_MIN: return R200_VPI_OUT_OP_MIN;
   case OPCODE_MUL: return R200_VPI_OUT_OP_MUL;
   case OPCODE_RCP: return R200_VPI_OUT_OP_RCP;
   case OPCODE_RSQ: return R200_VPI_OUT_OP_RSQ;
   case OPCODE_SGE: return R200_VPI_OUT_OP_SGE;
   case OPCODE_SLT: return R200_VPI_OUT_OP_SLT;

   default: 
      fprintf(stderr, "%s: Should not be called with opcode %d!", __FUNCTION__, opcode);
   }
   exit(-1);
   return 0;
}

static unsigned long op_operands(enum prog_opcode opcode)
{
   int i;

   /* Can we trust mesas opcodes to be in order ? */
   for(i=0; i < sizeof(op_names) / sizeof(*op_names); i++)
      if(op_names[i].opcode == opcode)
	 return op_names[i].ip;

   fprintf(stderr, "op %d not found in op_names\n", opcode);
   exit(-1);
   return 0;
}

/* TODO: Get rid of t_src_class call */
#define CMP_SRCS(a, b) (((a.RelAddr != b.RelAddr) || (a.Index != b.Index)) && \
		       ((t_src_class(a.File) == VSF_IN_CLASS_PARAM && \
			 t_src_class(b.File) == VSF_IN_CLASS_PARAM) || \
			(t_src_class(a.File) == VSF_IN_CLASS_ATTR && \
			 t_src_class(b.File) == VSF_IN_CLASS_ATTR))) \

/* fglrx on rv250 codes up unused sources as follows:
   unused but necessary sources are same as previous source, zero-ed out.
   unnecessary sources are same as previous source but with VSF_IN_CLASS_NONE set.
   i.e. an add (2 args) has its 2nd arg (if you use it as mov) zero-ed out, and 3rd arg
   set to VSF_IN_CLASS_NONE. Not sure if strictly necessary. */

/* use these simpler definitions. Must obviously not be used with not yet set up regs.
   Those are NOT semantically equivalent to the r300 ones, requires code changes */
#define ZERO_SRC_0 (((o_inst->src0 & ~(0xfff << R200_VPI_IN_X_SHIFT)) \
				   | ((R200_VPI_IN_SELECT_ZERO << R200_VPI_IN_X_SHIFT) \
				   | (R200_VPI_IN_SELECT_ZERO << R200_VPI_IN_Y_SHIFT) \
				   | (R200_VPI_IN_SELECT_ZERO << R200_VPI_IN_Z_SHIFT) \
				   | (R200_VPI_IN_SELECT_ZERO << R200_VPI_IN_W_SHIFT))))

#define ZERO_SRC_1 (((o_inst->src1 & ~(0xfff << R200_VPI_IN_X_SHIFT)) \
				   | ((R200_VPI_IN_SELECT_ZERO << R200_VPI_IN_X_SHIFT) \
				   | (R200_VPI_IN_SELECT_ZERO << R200_VPI_IN_Y_SHIFT) \
				   | (R200_VPI_IN_SELECT_ZERO << R200_VPI_IN_Z_SHIFT) \
				   | (R200_VPI_IN_SELECT_ZERO << R200_VPI_IN_W_SHIFT))))

#define ZERO_SRC_2 (((o_inst->src2 & ~(0xfff << R200_VPI_IN_X_SHIFT)) \
				   | ((R200_VPI_IN_SELECT_ZERO << R200_VPI_IN_X_SHIFT) \
				   | (R200_VPI_IN_SELECT_ZERO << R200_VPI_IN_Y_SHIFT) \
				   | (R200_VPI_IN_SELECT_ZERO << R200_VPI_IN_Z_SHIFT) \
				   | (R200_VPI_IN_SELECT_ZERO << R200_VPI_IN_W_SHIFT))))

#define UNUSED_SRC_0 ((o_inst->src0 & ~15) | 9)

#define UNUSED_SRC_1 ((o_inst->src1 & ~15) | 9)

#define UNUSED_SRC_2 ((o_inst->src2 & ~15) | 9)


/**
 * Generate an R200 vertex program from Mesa's internal representation.
 *
 * \return  GL_TRUE for success, GL_FALSE for failure.
 */
static GLboolean r200_translate_vertex_program(GLcontext *ctx, struct r200_vertex_program *vp)
{
   struct gl_vertex_program *mesa_vp = &vp->mesa_program;
   struct prog_instruction *vpi;
   int i;
   VERTEX_SHADER_INSTRUCTION *o_inst;
   unsigned long operands;
   int are_srcs_scalar;
   unsigned long hw_op;
   int dofogfix = 0;
   int fog_temp_i = 0;
   int free_inputs;
   int array_count = 0;

   vp->native = GL_FALSE;
   vp->translated = GL_TRUE;
   vp->fogmode = ctx->Fog.Mode;

   if (mesa_vp->Base.NumInstructions == 0)
      return GL_FALSE;

#if 0
   if ((mesa_vp->Base.InputsRead &
      ~(VERT_BIT_POS | VERT_BIT_NORMAL | VERT_BIT_COLOR0 | VERT_BIT_COLOR1 |
      VERT_BIT_FOG | VERT_BIT_TEX0 | VERT_BIT_TEX1 | VERT_BIT_TEX2 |
      VERT_BIT_TEX3 | VERT_BIT_TEX4 | VERT_BIT_TEX5)) != 0) {
      if (R200_DEBUG & DEBUG_FALLBACKS) {
	 fprintf(stderr, "can't handle vert prog inputs 0x%x\n",
	    mesa_vp->Base.InputsRead);
      }
      return GL_FALSE;
   }
#endif

   if ((mesa_vp->Base.OutputsWritten &
      ~((1 << VERT_RESULT_HPOS) | (1 << VERT_RESULT_COL0) | (1 << VERT_RESULT_COL1) |
      (1 << VERT_RESULT_FOGC) | (1 << VERT_RESULT_TEX0) | (1 << VERT_RESULT_TEX1) |
      (1 << VERT_RESULT_TEX2) | (1 << VERT_RESULT_TEX3) | (1 << VERT_RESULT_TEX4) |
      (1 << VERT_RESULT_TEX5) | (1 << VERT_RESULT_PSIZ))) != 0) {
      if (R200_DEBUG & DEBUG_FALLBACKS) {
	 fprintf(stderr, "can't handle vert prog outputs 0x%x\n",
	    mesa_vp->Base.OutputsWritten);
      }
      return GL_FALSE;
   }

   if (mesa_vp->IsNVProgram) {
   /* subtle differences in spec like guaranteed initialized regs could cause
      headaches. Might want to remove the driconf option to enable it completely */
      return GL_FALSE;
   }
   /* Initial value should be last tmp reg that hw supports.
      Strangely enough r300 doesnt mind even though these would be out of range.
      Smart enough to realize that it doesnt need it? */
   int u_temp_i = R200_VSF_MAX_TEMPS - 1;
   struct prog_src_register src[3];
   struct prog_dst_register dst;

/* FIXME: is changing the prog safe to do here? */
   if (mesa_vp->IsPositionInvariant &&
      /* make sure we only do this once */
       !(mesa_vp->Base.OutputsWritten & (1 << VERT_RESULT_HPOS))) {
	 _mesa_insert_mvp_code(ctx, mesa_vp);
      }

   /* for fogc, can't change mesa_vp, as it would hose swtnl, and exp with
      base e isn't directly available neither. */
   if ((mesa_vp->Base.OutputsWritten & (1 << VERT_RESULT_FOGC)) && !vp->fogpidx) {
      struct gl_program_parameter_list *paramList;
      gl_state_index tokens[STATE_LENGTH] = { STATE_FOG_PARAMS, 0, 0, 0, 0 };
      paramList = mesa_vp->Base.Parameters;
      vp->fogpidx = _mesa_add_state_reference(paramList, tokens);
   }

   vp->pos_end = 0;
   mesa_vp->Base.NumNativeInstructions = 0;
   if (mesa_vp->Base.Parameters)
      mesa_vp->Base.NumNativeParameters = mesa_vp->Base.Parameters->NumParameters;
   else
      mesa_vp->Base.NumNativeParameters = 0;

   for(i = 0; i < VERT_ATTRIB_MAX; i++)
      vp->inputs[i] = -1;
   for(i = 0; i < 15; i++)
      vp->inputmap_rev[i] = 255;
   free_inputs = 0x2ffd;

/* fglrx uses fixed inputs as follows for conventional attribs.
   generic attribs use non-fixed assignment, fglrx will always use the
   lowest attrib values available. We'll just do the same.
   There are 12 generic attribs possible, corresponding to attrib 0, 2-11
   and 13 in a hw vertex prog.
   attr 1 and 12 aren't used for generic attribs as those cannot be made vec4
   (correspond to vertex normal/weight - maybe weight actually could be made vec4).
   Additionally, not more than 12 arrays in total are possible I think.
   attr 0 is pos, R200_VTX_XY1|R200_VTX_Z1|R200_VTX_W1 in R200_SE_VTX_FMT_0
   attr 2-5 use colors 0-3 (R200_VTX_FP_RGBA << R200_VTX_COLOR_0/1/2/3_SHIFT in R200_SE_VTX_FMT_0)
   attr 6-11 use tex 0-5 (4 << R200_VTX_TEX0/1/2/3/4/5_COMP_CNT_SHIFT in R200_SE_VTX_FMT_1)
   attr 13 uses vtx1 pos (R200_VTX_XY1|R200_VTX_Z1|R200_VTX_W1 in R200_SE_VTX_FMT_0)
*/

/* attr 4,5 and 13 are only used with generic attribs.
   Haven't seen attr 14 used, maybe that's for the hw pointsize vec1 (which is
   not possibe to use with vertex progs as it is lacking in vert prog specification) */
/* may look different when using idx buf / input_route instead of se_vtx_fmt? */
   if (mesa_vp->Base.InputsRead & VERT_BIT_POS) {
      vp->inputs[VERT_ATTRIB_POS] = 0;
      vp->inputmap_rev[0] = VERT_ATTRIB_POS;
      free_inputs &= ~(1 << 0);
      array_count++;
   }
   if (mesa_vp->Base.InputsRead & VERT_BIT_WEIGHT) {
      vp->inputs[VERT_ATTRIB_WEIGHT] = 12;
      vp->inputmap_rev[1] = VERT_ATTRIB_WEIGHT;
      array_count++;
   }
   if (mesa_vp->Base.InputsRead & VERT_BIT_NORMAL) {
      vp->inputs[VERT_ATTRIB_NORMAL] = 1;
      vp->inputmap_rev[2] = VERT_ATTRIB_NORMAL;
      array_count++;
   }
   if (mesa_vp->Base.InputsRead & VERT_BIT_COLOR0) {
      vp->inputs[VERT_ATTRIB_COLOR0] = 2;
      vp->inputmap_rev[4] = VERT_ATTRIB_COLOR0;
      free_inputs &= ~(1 << 2);
      array_count++;
   }
   if (mesa_vp->Base.InputsRead & VERT_BIT_COLOR1) {
      vp->inputs[VERT_ATTRIB_COLOR1] = 3;
      vp->inputmap_rev[5] = VERT_ATTRIB_COLOR1;
      free_inputs &= ~(1 << 3);
      array_count++;
   }
   if (mesa_vp->Base.InputsRead & VERT_BIT_FOG) {
      vp->inputs[VERT_ATTRIB_FOG] = 15; array_count++;
      vp->inputmap_rev[3] = VERT_ATTRIB_FOG;
      array_count++;
   }
   for (i = VERT_ATTRIB_TEX0; i <= VERT_ATTRIB_TEX5; i++) {
      if (mesa_vp->Base.InputsRead & (1 << i)) {
	 vp->inputs[i] = i - VERT_ATTRIB_TEX0 + 6;
	 vp->inputmap_rev[8 + i - VERT_ATTRIB_TEX0] = i;
	 free_inputs &= ~(1 << (i - VERT_ATTRIB_TEX0 + 6));
	 array_count++;
      }
   }
   /* using VERT_ATTRIB_TEX6/7 would be illegal */
   /* completely ignore aliasing? */
   for (i = VERT_ATTRIB_GENERIC0; i < VERT_ATTRIB_MAX; i++) {
      int j;
   /* completely ignore aliasing? */
      if (mesa_vp->Base.InputsRead & (1 << i)) {
	 array_count++;
	 if (array_count > 12) {
	    if (R200_DEBUG & DEBUG_FALLBACKS) {
	       fprintf(stderr, "more than 12 attribs used in vert prog\n");
	    }
	    return GL_FALSE;
	 }
	 for (j = 0; j < 14; j++) {
	    /* will always find one due to limited array_count */
	    if (free_inputs & (1 << j)) {
	       free_inputs &= ~(1 << j);
	       vp->inputs[i] = j;
	       if (j == 0) vp->inputmap_rev[j] = i; /* mapped to pos */
	       else if (j < 12) vp->inputmap_rev[j + 2] = i; /* mapped to col/tex */
	       else vp->inputmap_rev[j + 1] = i; /* mapped to pos1 */
	       break;
	    }
	 }
      }
   }

   if (!(mesa_vp->Base.OutputsWritten & (1 << VERT_RESULT_HPOS))) {
      if (R200_DEBUG & DEBUG_FALLBACKS) {
	 fprintf(stderr, "can't handle vert prog without position output\n");
      }
      return GL_FALSE;
   }
   if (free_inputs & 1) {
      if (R200_DEBUG & DEBUG_FALLBACKS) {
	 fprintf(stderr, "can't handle vert prog without position input\n");
      }
      return GL_FALSE;
   }

   o_inst = vp->instr;
   for (vpi = mesa_vp->Base.Instructions; vpi->Opcode != OPCODE_END; vpi++, o_inst++){
      operands = op_operands(vpi->Opcode);
      are_srcs_scalar = operands & SCALAR_FLAG;
      operands &= OP_MASK;

      for(i = 0; i < operands; i++) {
	 src[i] = vpi->SrcReg[i];
	 /* hack up default attrib values as per spec as swizzling.
	    normal, fog, secondary color. Crazy?
	    May need more if we don't submit vec4 elements? */
	 if (src[i].File == PROGRAM_INPUT) {
	    if (src[i].Index == VERT_ATTRIB_NORMAL) {
	       int j;
	       for (j = 0; j < 4; j++) {
		  if (GET_SWZ(src[i].Swizzle, j) == SWIZZLE_W) {
		     src[i].Swizzle &= ~(SWIZZLE_W << (j*3));
		     src[i].Swizzle |= SWIZZLE_ONE << (j*3);
		  }
	       }
	    }
	    else if (src[i].Index == VERT_ATTRIB_COLOR1) {
	       int j;
	       for (j = 0; j < 4; j++) {
		  if (GET_SWZ(src[i].Swizzle, j) == SWIZZLE_W) {
		     src[i].Swizzle &= ~(SWIZZLE_W << (j*3));
		     src[i].Swizzle |= SWIZZLE_ZERO << (j*3);
		  }
	       }
	    }
	    else if (src[i].Index == VERT_ATTRIB_FOG) {
	       int j;
	       for (j = 0; j < 4; j++) {
		  if (GET_SWZ(src[i].Swizzle, j) == SWIZZLE_W) {
		     src[i].Swizzle &= ~(SWIZZLE_W << (j*3));
		     src[i].Swizzle |= SWIZZLE_ONE << (j*3);
		  }
		  else if ((GET_SWZ(src[i].Swizzle, j) == SWIZZLE_Y) ||
			    GET_SWZ(src[i].Swizzle, j) == SWIZZLE_Z) {
		     src[i].Swizzle &= ~(SWIZZLE_W << (j*3));
		     src[i].Swizzle |= SWIZZLE_ZERO << (j*3);
		  }
	       }
	    }
	 }
      }

      if(operands == 3){
	 if( CMP_SRCS(src[1], src[2]) || CMP_SRCS(src[0], src[2]) ){
	    o_inst->op = MAKE_VSF_OP(R200_VPI_OUT_OP_ADD,
		(u_temp_i << R200_VPI_OUT_REG_INDEX_SHIFT) | R200_VSF_OUT_CLASS_TMP,
		VSF_FLAG_ALL);

	    o_inst->src0 = MAKE_VSF_SOURCE(t_src_index(vp, &src[2]),
		  SWIZZLE_X, SWIZZLE_Y,
		  SWIZZLE_Z, SWIZZLE_W,
		  t_src_class(src[2].File), VSF_FLAG_NONE) | (src[2].RelAddr << 4);

	    o_inst->src1 = ZERO_SRC_0;
	    o_inst->src2 = UNUSED_SRC_1;
	    o_inst++;

	    src[2].File = PROGRAM_TEMPORARY;
	    src[2].Index = u_temp_i;
	    src[2].RelAddr = 0;
	    u_temp_i--;
	 }
      }

      if(operands >= 2){
	 if( CMP_SRCS(src[1], src[0]) ){
	    o_inst->op = MAKE_VSF_OP(R200_VPI_OUT_OP_ADD,
		(u_temp_i << R200_VPI_OUT_REG_INDEX_SHIFT) | R200_VSF_OUT_CLASS_TMP,
		VSF_FLAG_ALL);

	    o_inst->src0 = MAKE_VSF_SOURCE(t_src_index(vp, &src[0]),
		  SWIZZLE_X, SWIZZLE_Y,
		  SWIZZLE_Z, SWIZZLE_W,
		  t_src_class(src[0].File), VSF_FLAG_NONE) | (src[0].RelAddr << 4);

	    o_inst->src1 = ZERO_SRC_0;
	    o_inst->src2 = UNUSED_SRC_1;
	    o_inst++;

	    src[0].File = PROGRAM_TEMPORARY;
	    src[0].Index = u_temp_i;
	    src[0].RelAddr = 0;
	    u_temp_i--;
	 }
      }

      dst = vpi->DstReg;
      if (dst.File == PROGRAM_OUTPUT &&
	  dst.Index == VERT_RESULT_FOGC &&
	  dst.WriteMask & WRITEMASK_X) {
	  fog_temp_i = u_temp_i;
	  dst.File = PROGRAM_TEMPORARY;
	  dst.Index = fog_temp_i;
	  dofogfix = 1;
	  u_temp_i--;
      }

      /* These ops need special handling. */
      switch(vpi->Opcode){
      case OPCODE_POW:
/* pow takes only one argument, first scalar is in slot x, 2nd in slot z (other slots don't matter).
   So may need to insert additional instruction */
	 if ((src[0].File == src[1].File) &&
	     (src[0].Index == src[1].Index)) {
	    o_inst->op = MAKE_VSF_OP(R200_VPI_OUT_OP_POW, t_dst(&dst),
		   t_dst_mask(dst.WriteMask));
	    o_inst->src0 = MAKE_VSF_SOURCE(t_src_index(vp, &src[0]),
		   t_swizzle(GET_SWZ(src[0].Swizzle, 0)),
		   SWIZZLE_ZERO,
		   t_swizzle(GET_SWZ(src[1].Swizzle, 0)),
		   SWIZZLE_ZERO,
		   t_src_class(src[0].File),
		   src[0].NegateBase) | (src[0].RelAddr << 4);
	    o_inst->src1 = UNUSED_SRC_0;
	    o_inst->src2 = UNUSED_SRC_0;
	 }
	 else {
	    o_inst->op = MAKE_VSF_OP(R200_VPI_OUT_OP_ADD,
		   (u_temp_i << R200_VPI_OUT_REG_INDEX_SHIFT) | R200_VSF_OUT_CLASS_TMP,
		   VSF_FLAG_ALL);
	    o_inst->src0 = MAKE_VSF_SOURCE(t_src_index(vp, &src[0]),
		   t_swizzle(GET_SWZ(src[0].Swizzle, 0)),
		   SWIZZLE_ZERO, SWIZZLE_ZERO, SWIZZLE_ZERO,
		   t_src_class(src[0].File),
		   src[0].NegateBase ? VSF_FLAG_ALL : VSF_FLAG_NONE) | (src[0].RelAddr << 4);
	    o_inst->src1 = MAKE_VSF_SOURCE(t_src_index(vp, &src[1]),
		   SWIZZLE_ZERO, SWIZZLE_ZERO,
		   t_swizzle(GET_SWZ(src[1].Swizzle, 0)), SWIZZLE_ZERO,
		   t_src_class(src[1].File),
		   src[1].NegateBase ? VSF_FLAG_ALL : VSF_FLAG_NONE) | (src[1].RelAddr << 4);
	    o_inst->src2 = UNUSED_SRC_1;
	    o_inst++;

	    o_inst->op = MAKE_VSF_OP(R200_VPI_OUT_OP_POW, t_dst(&dst),
		   t_dst_mask(dst.WriteMask));
	    o_inst->src0 = MAKE_VSF_SOURCE(u_temp_i,
		   VSF_IN_COMPONENT_X,
		   VSF_IN_COMPONENT_Y,
		   VSF_IN_COMPONENT_Z,
		   VSF_IN_COMPONENT_W,
		   VSF_IN_CLASS_TMP,
		   VSF_FLAG_NONE);
	    o_inst->src1 = UNUSED_SRC_0;
	    o_inst->src2 = UNUSED_SRC_0;
	    u_temp_i--;
	 }
	 goto next;

      case OPCODE_MOV://ADD RESULT 1.X Y Z W PARAM 0{} {X Y Z W} PARAM 0{} {ZERO ZERO ZERO ZERO} 
      case OPCODE_SWZ:
	 o_inst->op = MAKE_VSF_OP(R200_VPI_OUT_OP_ADD, t_dst(&dst),
		t_dst_mask(dst.WriteMask));
	 o_inst->src0 = t_src(vp, &src[0]);
	 o_inst->src1 = ZERO_SRC_0;
	 o_inst->src2 = UNUSED_SRC_1;
	 goto next;

      case OPCODE_MAD:
	 hw_op=(src[0].File == PROGRAM_TEMPORARY &&
	    src[1].File == PROGRAM_TEMPORARY &&
	    src[2].File == PROGRAM_TEMPORARY) ? R200_VPI_OUT_OP_MAD_2 : R200_VPI_OUT_OP_MAD;

	 o_inst->op = MAKE_VSF_OP(hw_op, t_dst(&dst),
	    t_dst_mask(dst.WriteMask));
	 o_inst->src0 = t_src(vp, &src[0]);
#if 0
if ((o_inst - vp->instr) == 31) {
/* fix up the broken vertex program of quake4 demo... */
o_inst->src1 = MAKE_VSF_SOURCE(t_src_index(vp, &src[1]),
			SWIZZLE_X, SWIZZLE_X, SWIZZLE_X, SWIZZLE_X,
			t_src_class(src[1].File),
			src[1].NegateBase) | (src[1].RelAddr << 4);
o_inst->src2 = MAKE_VSF_SOURCE(t_src_index(vp, &src[1]),
			SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y, SWIZZLE_Y,
			t_src_class(src[1].File),
			src[1].NegateBase) | (src[1].RelAddr << 4);
}
else {
	 o_inst->src1 = t_src(vp, &src[1]);
	 o_inst->src2 = t_src(vp, &src[2]);
}
#else
	 o_inst->src1 = t_src(vp, &src[1]);
	 o_inst->src2 = t_src(vp, &src[2]);
#endif
	 goto next;

      case OPCODE_DP3://DOT RESULT 1.X Y Z W PARAM 0{} {X Y Z ZERO} PARAM 0{} {X Y Z ZERO} 
	 o_inst->op = MAKE_VSF_OP(R200_VPI_OUT_OP_DOT, t_dst(&dst),
		t_dst_mask(dst.WriteMask));

	 o_inst->src0 = MAKE_VSF_SOURCE(t_src_index(vp, &src[0]),
		t_swizzle(GET_SWZ(src[0].Swizzle, 0)),
		t_swizzle(GET_SWZ(src[0].Swizzle, 1)),
		t_swizzle(GET_SWZ(src[0].Swizzle, 2)),
		SWIZZLE_ZERO,
		t_src_class(src[0].File),
		src[0].NegateBase) | (src[0].RelAddr << 4);

	 o_inst->src1 = MAKE_VSF_SOURCE(t_src_index(vp, &src[1]),
		t_swizzle(GET_SWZ(src[1].Swizzle, 0)),
		t_swizzle(GET_SWZ(src[1].Swizzle, 1)),
		t_swizzle(GET_SWZ(src[1].Swizzle, 2)),
		SWIZZLE_ZERO,
		t_src_class(src[1].File),
		src[1].NegateBase) | (src[1].RelAddr << 4);

	 o_inst->src2 = UNUSED_SRC_1;
	 goto next;

      case OPCODE_DPH://DOT RESULT 1.X Y Z W PARAM 0{} {X Y Z ONE} PARAM 0{} {X Y Z W} 
	 o_inst->op = MAKE_VSF_OP(R200_VPI_OUT_OP_DOT, t_dst(&dst),
		t_dst_mask(dst.WriteMask));

	 o_inst->src0 = MAKE_VSF_SOURCE(t_src_index(vp, &src[0]),
		t_swizzle(GET_SWZ(src[0].Swizzle, 0)),
		t_swizzle(GET_SWZ(src[0].Swizzle, 1)),
		t_swizzle(GET_SWZ(src[0].Swizzle, 2)),
		VSF_IN_COMPONENT_ONE,
		t_src_class(src[0].File),
		src[0].NegateBase) | (src[0].RelAddr << 4);
	 o_inst->src1 = t_src(vp, &src[1]);
	 o_inst->src2 = UNUSED_SRC_1;
	 goto next;

      case OPCODE_SUB://ADD RESULT 1.X Y Z W TMP 0{} {X Y Z W} PARAM 1{X Y Z W } {X Y Z W} neg Xneg Yneg Zneg W
	 o_inst->op = MAKE_VSF_OP(R200_VPI_OUT_OP_ADD, t_dst(&dst),
		t_dst_mask(dst.WriteMask));

	 o_inst->src0 = t_src(vp, &src[0]);
	 o_inst->src1 = MAKE_VSF_SOURCE(t_src_index(vp, &src[1]),
		t_swizzle(GET_SWZ(src[1].Swizzle, 0)),
		t_swizzle(GET_SWZ(src[1].Swizzle, 1)),
		t_swizzle(GET_SWZ(src[1].Swizzle, 2)),
		t_swizzle(GET_SWZ(src[1].Swizzle, 3)),
		t_src_class(src[1].File),
		(!src[1].NegateBase) ? VSF_FLAG_ALL : VSF_FLAG_NONE) | (src[1].RelAddr << 4);
	 o_inst->src2 = UNUSED_SRC_1;
	 goto next;

      case OPCODE_ABS://MAX RESULT 1.X Y Z W PARAM 0{} {X Y Z W} PARAM 0{X Y Z W } {X Y Z W} neg Xneg Yneg Zneg W
	 o_inst->op=MAKE_VSF_OP(R200_VPI_OUT_OP_MAX, t_dst(&dst),
		t_dst_mask(dst.WriteMask));

	 o_inst->src0=t_src(vp, &src[0]);
	 o_inst->src1=MAKE_VSF_SOURCE(t_src_index(vp, &src[0]),
		t_swizzle(GET_SWZ(src[0].Swizzle, 0)),
		t_swizzle(GET_SWZ(src[0].Swizzle, 1)),
		t_swizzle(GET_SWZ(src[0].Swizzle, 2)),
		t_swizzle(GET_SWZ(src[0].Swizzle, 3)),
		t_src_class(src[0].File),
		(!src[0].NegateBase) ? VSF_FLAG_ALL : VSF_FLAG_NONE) | (src[0].RelAddr << 4);
	 o_inst->src2 = UNUSED_SRC_1;
	 goto next;

      case OPCODE_FLR:
      /* FRC TMP 0.X Y Z W PARAM 0{} {X Y Z W} 
         ADD RESULT 1.X Y Z W PARAM 0{} {X Y Z W} TMP 0{X Y Z W } {X Y Z W} neg Xneg Yneg Zneg W */

	 o_inst->op = MAKE_VSF_OP(R200_VPI_OUT_OP_FRC,
	    (u_temp_i << R200_VPI_OUT_REG_INDEX_SHIFT) | R200_VSF_OUT_CLASS_TMP,
	    t_dst_mask(dst.WriteMask));

	 o_inst->src0 = t_src(vp, &src[0]);
	 o_inst->src1 = UNUSED_SRC_0;
	 o_inst->src2 = UNUSED_SRC_1;
	 o_inst++;

	 o_inst->op = MAKE_VSF_OP(R200_VPI_OUT_OP_ADD, t_dst(&dst),
		t_dst_mask(dst.WriteMask));

	 o_inst->src0 = t_src(vp, &src[0]);
	 o_inst->src1 = MAKE_VSF_SOURCE(u_temp_i,
		VSF_IN_COMPONENT_X,
		VSF_IN_COMPONENT_Y,
		VSF_IN_COMPONENT_Z,
		VSF_IN_COMPONENT_W,
		VSF_IN_CLASS_TMP,
		/* Not 100% sure about this */
		(!src[0].NegateBase) ? VSF_FLAG_ALL : VSF_FLAG_NONE/*VSF_FLAG_ALL*/);

	 o_inst->src2 = UNUSED_SRC_0;
	 u_temp_i--;
	 goto next;

      case OPCODE_XPD:
	 /* mul r0, r1.yzxw, r2.zxyw
	    mad r0, -r2.yzxw, r1.zxyw, r0
	    NOTE: might need MAD_2
	  */

	 o_inst->op = MAKE_VSF_OP(R200_VPI_OUT_OP_MUL,
	    (u_temp_i << R200_VPI_OUT_REG_INDEX_SHIFT) | R200_VSF_OUT_CLASS_TMP,
	    t_dst_mask(dst.WriteMask));

	 o_inst->src0 = MAKE_VSF_SOURCE(t_src_index(vp, &src[0]),
		t_swizzle(GET_SWZ(src[0].Swizzle, 1)), // y
		t_swizzle(GET_SWZ(src[0].Swizzle, 2)), // z
		t_swizzle(GET_SWZ(src[0].Swizzle, 0)), // x
		t_swizzle(GET_SWZ(src[0].Swizzle, 3)), // w
		t_src_class(src[0].File),
		src[0].NegateBase) | (src[0].RelAddr << 4);

	 o_inst->src1 = MAKE_VSF_SOURCE(t_src_index(vp, &src[1]),
		t_swizzle(GET_SWZ(src[1].Swizzle, 2)), // z
		t_swizzle(GET_SWZ(src[1].Swizzle, 0)), // x
		t_swizzle(GET_SWZ(src[1].Swizzle, 1)), // y
		t_swizzle(GET_SWZ(src[1].Swizzle, 3)), // w
		t_src_class(src[1].File),
		src[1].NegateBase) | (src[1].RelAddr << 4);

	 o_inst->src2 = UNUSED_SRC_1;
	 o_inst++;
	 u_temp_i--;

	 o_inst->op = MAKE_VSF_OP(R200_VPI_OUT_OP_MAD, t_dst(&dst),
		t_dst_mask(dst.WriteMask));

	 o_inst->src0 = MAKE_VSF_SOURCE(t_src_index(vp, &src[1]),
		t_swizzle(GET_SWZ(src[1].Swizzle, 1)), // y
		t_swizzle(GET_SWZ(src[1].Swizzle, 2)), // z
		t_swizzle(GET_SWZ(src[1].Swizzle, 0)), // x
		t_swizzle(GET_SWZ(src[1].Swizzle, 3)), // w
		t_src_class(src[1].File),
		(!src[1].NegateBase) ? VSF_FLAG_ALL : VSF_FLAG_NONE) | (src[1].RelAddr << 4);

	 o_inst->src1 = MAKE_VSF_SOURCE(t_src_index(vp, &src[0]),
		t_swizzle(GET_SWZ(src[0].Swizzle, 2)), // z
		t_swizzle(GET_SWZ(src[0].Swizzle, 0)), // x
		t_swizzle(GET_SWZ(src[0].Swizzle, 1)), // y
		t_swizzle(GET_SWZ(src[0].Swizzle, 3)), // w
		t_src_class(src[0].File),
		src[0].NegateBase) | (src[0].RelAddr << 4);

	 o_inst->src2 = MAKE_VSF_SOURCE(u_temp_i+1,
		VSF_IN_COMPONENT_X,
		VSF_IN_COMPONENT_Y,
		VSF_IN_COMPONENT_Z,
		VSF_IN_COMPONENT_W,
		VSF_IN_CLASS_TMP,
		VSF_FLAG_NONE);
	 goto next;

      case OPCODE_END:
	 assert(0);
      default:
	 break;
      }

      o_inst->op = MAKE_VSF_OP(t_opcode(vpi->Opcode), t_dst(&dst),
	    t_dst_mask(dst.WriteMask));

      if(are_srcs_scalar){
	 switch(operands){
	    case 1:
		o_inst->src0 = t_src_scalar(vp, &src[0]);
		o_inst->src1 = UNUSED_SRC_0;
		o_inst->src2 = UNUSED_SRC_1;
	    break;

	    case 2:
		o_inst->src0 = t_src_scalar(vp, &src[0]);
		o_inst->src1 = t_src_scalar(vp, &src[1]);
		o_inst->src2 = UNUSED_SRC_1;
	    break;

	    case 3:
		o_inst->src0 = t_src_scalar(vp, &src[0]);
		o_inst->src1 = t_src_scalar(vp, &src[1]);
		o_inst->src2 = t_src_scalar(vp, &src[2]);
	    break;

	    default:
		fprintf(stderr, "illegal number of operands %lu\n", operands);
		exit(-1);
	    break;
	 }
      } else {
	 switch(operands){
	    case 1:
		o_inst->src0 = t_src(vp, &src[0]);
		o_inst->src1 = UNUSED_SRC_0;
		o_inst->src2 = UNUSED_SRC_1;
	    break;

	    case 2:
		o_inst->src0 = t_src(vp, &src[0]);
		o_inst->src1 = t_src(vp, &src[1]);
		o_inst->src2 = UNUSED_SRC_1;
	    break;

	    case 3:
		o_inst->src0 = t_src(vp, &src[0]);
		o_inst->src1 = t_src(vp, &src[1]);
		o_inst->src2 = t_src(vp, &src[2]);
	    break;

	    default:
		fprintf(stderr, "illegal number of operands %lu\n", operands);
		exit(-1);
	    break;
	 }
      }
      next:

      if (dofogfix) {
	 o_inst++;
	 if (vp->fogmode == GL_EXP) {
	    o_inst->op = MAKE_VSF_OP(R200_VPI_OUT_OP_MUL,
		(fog_temp_i << R200_VPI_OUT_REG_INDEX_SHIFT) | R200_VSF_OUT_CLASS_TMP,
		VSF_FLAG_X);
	    o_inst->src0 = EASY_VSF_SOURCE(fog_temp_i, X, X, X, X, TMP, NONE);
	    o_inst->src1 = EASY_VSF_SOURCE(vp->fogpidx, X, X, X, X, PARAM, NONE);
	    o_inst->src2 = UNUSED_SRC_1;
	    o_inst++;
	    o_inst->op = MAKE_VSF_OP(R200_VPI_OUT_OP_EXP_E,
		R200_VSF_OUT_CLASS_RESULT_FOGC,
		VSF_FLAG_X);
	    o_inst->src0 = EASY_VSF_SOURCE(fog_temp_i, X, X, X, X, TMP, ALL);
	    o_inst->src1 = UNUSED_SRC_0;
	    o_inst->src2 = UNUSED_SRC_1;
	 }
	 else if (vp->fogmode == GL_EXP2) {
	    o_inst->op = MAKE_VSF_OP(R200_VPI_OUT_OP_MUL,
		(fog_temp_i << R200_VPI_OUT_REG_INDEX_SHIFT) | R200_VSF_OUT_CLASS_TMP,
		VSF_FLAG_X);
	    o_inst->src0 = EASY_VSF_SOURCE(fog_temp_i, X, X, X, X, TMP, NONE);
	    o_inst->src1 = EASY_VSF_SOURCE(vp->fogpidx, X, X, X, X, PARAM, NONE);
	    o_inst->src2 = UNUSED_SRC_1;
	    o_inst++;
	    o_inst->op = MAKE_VSF_OP(R200_VPI_OUT_OP_MUL,
		(fog_temp_i << R200_VPI_OUT_REG_INDEX_SHIFT) | R200_VSF_OUT_CLASS_TMP,
		VSF_FLAG_X);
	    o_inst->src0 = EASY_VSF_SOURCE(fog_temp_i, X, X, X, X, TMP, NONE);
	    o_inst->src1 = EASY_VSF_SOURCE(fog_temp_i, X, X, X, X, TMP, NONE);
	    o_inst->src2 = UNUSED_SRC_1;
	    o_inst++;
	    o_inst->op = MAKE_VSF_OP(R200_VPI_OUT_OP_EXP_E,
		R200_VSF_OUT_CLASS_RESULT_FOGC,
		VSF_FLAG_X);
	    o_inst->src0 = EASY_VSF_SOURCE(fog_temp_i, X, X, X, X, TMP, ALL);
	    o_inst->src1 = UNUSED_SRC_0;
	    o_inst->src2 = UNUSED_SRC_1;
	 }
	 else { /* fogmode == GL_LINEAR */
		/* could do that with single op (dot) if using params like
		   with fixed function pipeline fog */
	    o_inst->op = MAKE_VSF_OP(R200_VPI_OUT_OP_ADD,
		(fog_temp_i << R200_VPI_OUT_REG_INDEX_SHIFT) | R200_VSF_OUT_CLASS_TMP,
		VSF_FLAG_X);
	    o_inst->src0 = EASY_VSF_SOURCE(fog_temp_i, X, X, X, X, TMP, ALL);
	    o_inst->src1 = EASY_VSF_SOURCE(vp->fogpidx, Z, Z, Z, Z, PARAM, NONE);
	    o_inst->src2 = UNUSED_SRC_1;
	    o_inst++;
	    o_inst->op = MAKE_VSF_OP(R200_VPI_OUT_OP_MUL,
		R200_VSF_OUT_CLASS_RESULT_FOGC,
		VSF_FLAG_X);
	    o_inst->src0 = EASY_VSF_SOURCE(fog_temp_i, X, X, X, X, TMP, NONE);
	    o_inst->src1 = EASY_VSF_SOURCE(vp->fogpidx, W, W, W, W, PARAM, NONE);
	    o_inst->src2 = UNUSED_SRC_1;

	 }
         dofogfix = 0;
      }

      if (mesa_vp->Base.NumNativeTemporaries <
	 (mesa_vp->Base.NumTemporaries + (R200_VSF_MAX_TEMPS - 1 - u_temp_i))) {
	 mesa_vp->Base.NumNativeTemporaries =
	    mesa_vp->Base.NumTemporaries + (R200_VSF_MAX_TEMPS - 1 - u_temp_i);
      }
      if (u_temp_i < mesa_vp->Base.NumTemporaries) {
	 if (R200_DEBUG & DEBUG_FALLBACKS) {
	    fprintf(stderr, "Ran out of temps, num temps %d, us %d\n", mesa_vp->Base.NumTemporaries, u_temp_i);
	 }
	 return GL_FALSE;
      }
      u_temp_i = R200_VSF_MAX_TEMPS - 1;
      if(o_inst - vp->instr >= R200_VSF_MAX_INST) {
	 mesa_vp->Base.NumNativeInstructions = 129;
	 if (R200_DEBUG & DEBUG_FALLBACKS) {
	    fprintf(stderr, "more than 128 native instructions\n");
	 }
	 return GL_FALSE;
      }
      if ((o_inst->op & R200_VSF_OUT_CLASS_MASK) == R200_VSF_OUT_CLASS_RESULT_POS) {
	 vp->pos_end = (o_inst - vp->instr);
      }
   }

   vp->native = GL_TRUE;
   mesa_vp->Base.NumNativeInstructions = (o_inst - vp->instr);
#if 0
   fprintf(stderr, "hw program:\n");
   for(i=0; i < vp->program.length; i++)
      fprintf(stderr, "%08x\n", vp->instr[i]);
#endif
   return GL_TRUE;
}

void r200SetupVertexProg( GLcontext *ctx ) {
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   struct r200_vertex_program *vp = (struct r200_vertex_program *)ctx->VertexProgram.Current;
   GLboolean fallback;
   GLint i;

   if (!vp->translated || (ctx->Fog.Enabled && ctx->Fog.Mode != vp->fogmode)) {
      rmesa->curr_vp_hw = NULL;
      r200_translate_vertex_program(ctx, vp);
   }
   /* could optimize setting up vertex progs away for non-tcl hw */
   fallback = !(vp->native && r200VertexProgUpdateParams(ctx, vp) &&
      rmesa->r200Screen->drmSupportsVertexProgram);
   TCL_FALLBACK(ctx, R200_TCL_FALLBACK_VERTEX_PROGRAM, fallback);
   if (rmesa->TclFallback) return;

   R200_STATECHANGE( rmesa, vap );
   /* FIXME: fglrx sets R200_VAP_SINGLE_BUF_STATE_ENABLE too. Do we need it?
             maybe only when using more than 64 inst / 96 param? */
   rmesa->hw.vap.cmd[VAP_SE_VAP_CNTL] |= R200_VAP_PROG_VTX_SHADER_ENABLE /*| R200_VAP_SINGLE_BUF_STATE_ENABLE*/;

   R200_STATECHANGE( rmesa, pvs );

   rmesa->hw.pvs.cmd[PVS_CNTL_1] = (0 << R200_PVS_CNTL_1_PROGRAM_START_SHIFT) |
      ((vp->mesa_program.Base.NumNativeInstructions - 1) << R200_PVS_CNTL_1_PROGRAM_END_SHIFT) |
      (vp->pos_end << R200_PVS_CNTL_1_POS_END_SHIFT);
   rmesa->hw.pvs.cmd[PVS_CNTL_2] = (0 << R200_PVS_CNTL_2_PARAM_OFFSET_SHIFT) |
      (vp->mesa_program.Base.NumNativeParameters << R200_PVS_CNTL_2_PARAM_COUNT_SHIFT);

   /* maybe user clip planes just work with vertex progs... untested */
   if (ctx->Transform.ClipPlanesEnabled) {
      R200_STATECHANGE( rmesa, tcl );
      if (vp->mesa_program.IsPositionInvariant) {
	 rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] |= (ctx->Transform.ClipPlanesEnabled << 2);
      }
      else {
	 rmesa->hw.tcl.cmd[TCL_UCP_VERT_BLEND_CTL] &= ~(0xfc);
      }
   }

   if (vp != rmesa->curr_vp_hw) {
      GLuint count = vp->mesa_program.Base.NumNativeInstructions;
      drm_radeon_cmd_header_t tmp;

      R200_STATECHANGE( rmesa, vpi[0] );
      R200_STATECHANGE( rmesa, vpi[1] );

      /* FIXME: what about using a memcopy... */
      for (i = 0; (i < 64) && i < count; i++) {
	 rmesa->hw.vpi[0].cmd[VPI_OPDST_0 + 4 * i] = vp->instr[i].op;
	 rmesa->hw.vpi[0].cmd[VPI_SRC0_0 + 4 * i] = vp->instr[i].src0;
	 rmesa->hw.vpi[0].cmd[VPI_SRC1_0 + 4 * i] = vp->instr[i].src1;
	 rmesa->hw.vpi[0].cmd[VPI_SRC2_0 + 4 * i] = vp->instr[i].src2;
      }
      /* hack up the cmd_size so not the whole state atom is emitted always.
         This may require some more thought, we may emit half progs on lost state, but
         hopefully it won't matter?
         WARNING: must not use R200_DB_STATECHANGE, this will produce bogus (and rejected)
         packet emits (due to the mismatched cmd_size and count in cmd/last_cmd) */
      rmesa->hw.vpi[0].cmd_size = 1 + 4 * ((count > 64) ? 64 : count);
      tmp.i = rmesa->hw.vpi[0].cmd[VPI_CMD_0];
      tmp.veclinear.count = (count > 64) ? 64 : count;
      rmesa->hw.vpi[0].cmd[VPI_CMD_0] = tmp.i;
      if (count > 64) {
	 for (i = 0; i < (count - 64); i++) {
	    rmesa->hw.vpi[1].cmd[VPI_OPDST_0 + 4 * i] = vp->instr[i + 64].op;
	    rmesa->hw.vpi[1].cmd[VPI_SRC0_0 + 4 * i] = vp->instr[i + 64].src0;
	    rmesa->hw.vpi[1].cmd[VPI_SRC1_0 + 4 * i] = vp->instr[i + 64].src1;
	    rmesa->hw.vpi[1].cmd[VPI_SRC2_0 + 4 * i] = vp->instr[i + 64].src2;
	 }
	 rmesa->hw.vpi[1].cmd_size = 1 + 4 * (count - 64);
	 tmp.i = rmesa->hw.vpi[1].cmd[VPI_CMD_0];
	 tmp.veclinear.count = count - 64;
	 rmesa->hw.vpi[1].cmd[VPI_CMD_0] = tmp.i;
      }
      rmesa->curr_vp_hw = vp;
   }
}


static void
r200BindProgram(GLcontext *ctx, GLenum target, struct gl_program *prog)
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   switch(target){
   case GL_VERTEX_PROGRAM_ARB:
      rmesa->curr_vp_hw = NULL;
      break;
   default:
      _mesa_problem(ctx, "Target not supported yet!");
      break;
   }
}

static struct gl_program *
r200NewProgram(GLcontext *ctx, GLenum target, GLuint id)
{
   struct r200_vertex_program *vp;

   switch(target){
   case GL_VERTEX_PROGRAM_ARB:
      vp = CALLOC_STRUCT(r200_vertex_program);
      return _mesa_init_vertex_program(ctx, &vp->mesa_program, target, id);
   case GL_FRAGMENT_PROGRAM_ARB:
   case GL_FRAGMENT_PROGRAM_NV:
      return _mesa_init_fragment_program( ctx, CALLOC_STRUCT(gl_fragment_program), target, id );
   default:
      _mesa_problem(ctx, "Bad target in r200NewProgram");
   }
   return NULL;	
}


static void
r200DeleteProgram(GLcontext *ctx, struct gl_program *prog)
{
   _mesa_delete_program(ctx, prog);
}

static void
r200ProgramStringNotify(GLcontext *ctx, GLenum target, struct gl_program *prog)
{
   struct r200_vertex_program *vp = (void *)prog;
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   switch(target) {
   case GL_VERTEX_PROGRAM_ARB:
      vp->translated = GL_FALSE;
      vp->fogpidx = 0;
/*      memset(&vp->translated, 0, sizeof(struct r200_vertex_program) - sizeof(struct gl_vertex_program));*/
      r200_translate_vertex_program(ctx, vp);
      rmesa->curr_vp_hw = NULL;
      break;
   case GL_FRAGMENT_SHADER_ATI:
      rmesa->afs_loaded = NULL;
      break;
   }
   /* need this for tcl fallbacks */
   _tnl_program_string(ctx, target, prog);
}

static GLboolean
r200IsProgramNative(GLcontext *ctx, GLenum target, struct gl_program *prog)
{
   struct r200_vertex_program *vp = (void *)prog;

   switch(target){
   case GL_VERTEX_STATE_PROGRAM_NV:
   case GL_VERTEX_PROGRAM_ARB:
      if (!vp->translated) {
	 r200_translate_vertex_program(ctx, vp);
      }
     /* does not take parameters etc. into account */
      return vp->native;
   default:
      _mesa_problem(ctx, "Bad target in r200NewProgram");
   }
   return 0;
}

void r200InitShaderFuncs(struct dd_function_table *functions)
{
   functions->NewProgram = r200NewProgram;
   functions->BindProgram = r200BindProgram;
   functions->DeleteProgram = r200DeleteProgram;
   functions->ProgramStringNotify = r200ProgramStringNotify;
   functions->IsProgramNative = r200IsProgramNative;
}
