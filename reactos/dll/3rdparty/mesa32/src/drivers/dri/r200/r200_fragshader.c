/**************************************************************************
 *
 * Copyright 2004 David Airlie
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL DAVID AIRLIE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/
#include "glheader.h"
#include "macros.h"
#include "enums.h"

#include "tnl/t_context.h"
#include "atifragshader.h"
#include "program.h"
#include "r200_context.h"
#include "r200_ioctl.h"
#include "r200_tex.h"

#define SET_INST(inst, type) afs_cmd[((inst<<2) + (type<<1) + 1)]
#define SET_INST_2(inst, type) afs_cmd[((inst<<2) + (type<<1) + 2)]

static void r200SetFragShaderArg( GLuint *afs_cmd, GLuint opnum, GLuint optype,
				const struct atifragshader_src_register srcReg,
				GLuint argPos, GLuint *tfactor )
{
   const GLuint index = srcReg.Index;
   const GLuint srcmod = srcReg.argMod;
   const GLuint srcrep = srcReg.argRep;
   GLuint reg0 = 0;
   GLuint reg2 = 0;
   GLuint useOddSrc = 0;

   switch(srcrep) {
   case GL_RED:
      reg2 |= R200_TXC_REPL_RED << (R200_TXC_REPL_ARG_A_SHIFT + (2*argPos));
      if (optype)
	 useOddSrc = 1;
      break;
   case GL_GREEN:
      reg2 |= R200_TXC_REPL_GREEN << (R200_TXC_REPL_ARG_A_SHIFT + (2*argPos));
      if (optype)
	 useOddSrc = 1;
      break;
   case GL_BLUE:
      if (!optype)
	 reg2 |= R200_TXC_REPL_BLUE << (R200_TXC_REPL_ARG_A_SHIFT + (2*argPos));
      else
	 useOddSrc = 1;
      break;
   case GL_ALPHA:
      if (!optype)
	 useOddSrc = 1;
      break;
   }

   if (index >= GL_REG_0_ATI && index <= GL_REG_5_ATI)
      reg0 |= (((index - GL_REG_0_ATI)*2) + 10 + useOddSrc) << (5*argPos);
   else if (index >= GL_CON_0_ATI && index <= GL_CON_7_ATI) {
      if ((*tfactor == 0) || (index == *tfactor)) {
	 reg0 |= (R200_TXC_ARG_A_TFACTOR_COLOR + useOddSrc) << (5*argPos);
	 reg2 |= (index - GL_CON_0_ATI) << R200_TXC_TFACTOR_SEL_SHIFT;
	 *tfactor = index;
      }
      else {
	 reg0 |= (R200_TXC_ARG_A_TFACTOR1_COLOR + useOddSrc) << (5*argPos);
	 reg2 |= (index - GL_CON_0_ATI) << R200_TXC_TFACTOR1_SEL_SHIFT;
      }
   }
   else if (index == GL_PRIMARY_COLOR_EXT) {
      reg0 |= (R200_TXC_ARG_A_DIFFUSE_COLOR + useOddSrc) << (5*argPos);
   }
   else if (index == GL_SECONDARY_INTERPOLATOR_ATI) {
      reg0 |= (R200_TXC_ARG_A_SPECULAR_COLOR + useOddSrc) << (5*argPos);
   }
   /* GL_ZERO is a noop, for GL_ONE we set the complement */
   else if (index == GL_ONE) {
      reg0 |= R200_TXC_COMP_ARG_A << (4*argPos);
   }

   if (srcmod & GL_COMP_BIT_ATI)
      reg0 ^= R200_TXC_COMP_ARG_A << (4*argPos);
   if (srcmod & GL_BIAS_BIT_ATI)
      reg0 |= R200_TXC_BIAS_ARG_A << (4*argPos);
   if (srcmod & GL_2X_BIT_ATI)
      reg0 |= R200_TXC_SCALE_ARG_A << (4*argPos);
   if (srcmod & GL_NEGATE_BIT_ATI)
      reg0 ^= R200_TXC_NEG_ARG_A << (4*argPos);

   SET_INST(opnum, optype) |= reg0;
   SET_INST_2(opnum, optype) |= reg2;
}

static GLuint dstmask_table[8] =
{
   R200_TXC_OUTPUT_MASK_RGB,
   R200_TXC_OUTPUT_MASK_R,
   R200_TXC_OUTPUT_MASK_G,
   R200_TXC_OUTPUT_MASK_RG,
   R200_TXC_OUTPUT_MASK_B,
   R200_TXC_OUTPUT_MASK_RB,
   R200_TXC_OUTPUT_MASK_GB,
   R200_TXC_OUTPUT_MASK_RGB
};

static void r200UpdateFSArith( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   GLuint *afs_cmd;
   const struct ati_fragment_shader *shader = ctx->ATIFragmentShader.Current;
   GLuint pass;

   R200_STATECHANGE( rmesa, afs[0] );
   R200_STATECHANGE( rmesa, afs[1] );

   if (shader->NumPasses < 2) {
      afs_cmd = (GLuint *) rmesa->hw.afs[1].cmd;
   }
   else {
      afs_cmd = (GLuint *) rmesa->hw.afs[0].cmd;
   }
   for (pass = 0; pass < shader->NumPasses; pass++) {
      GLuint opnum = 0;
      GLuint pc;
      for (pc = 0; pc < shader->numArithInstr[pass]; pc++) {
         GLuint optype;
	 struct atifs_instruction *inst = &shader->Instructions[pass][pc];

	 SET_INST(opnum, 0) = 0;
	 SET_INST_2(opnum, 0) = 0;
	 SET_INST(opnum, 1) = 0;
	 SET_INST_2(opnum, 1) = 0;

	 for (optype = 0; optype < 2; optype++) {
	    GLuint tfactor = 0;

	    if (inst->Opcode[optype]) {
	       switch (inst->Opcode[optype]) {
	       /* these are all MADD in disguise
		  MADD is A * B + C
		  so for GL_ADD use arg B/C and make A complement 0
		  for GL_SUB use arg B/C, negate C and make A complement 0
		  for GL_MOV use arg C
		  for GL_MUL use arg A
		  for GL_MAD all good */
	       case GL_SUB_ATI:
		  /* negate C */
		  SET_INST(opnum, optype) |= R200_TXC_NEG_ARG_C;
		  /* fallthrough */
	       case GL_ADD_ATI:
		  r200SetFragShaderArg(afs_cmd, opnum, optype,
					inst->SrcReg[optype][0], 1, &tfactor);
		  r200SetFragShaderArg(afs_cmd, opnum, optype,
					inst->SrcReg[optype][1], 2, &tfactor);
		  /* A = complement 0 */
		  SET_INST(opnum, optype) |= R200_TXC_COMP_ARG_A;
		  SET_INST(opnum, optype) |= R200_TXC_OP_MADD;
		  break;
	       case GL_MOV_ATI:
		  /* put arg0 in C */
		  r200SetFragShaderArg(afs_cmd, opnum, optype,
					inst->SrcReg[optype][0], 2, &tfactor);
		  SET_INST(opnum, optype) |= R200_TXC_OP_MADD;
		  break;
	       case GL_MAD_ATI:
		  r200SetFragShaderArg(afs_cmd, opnum, optype,
					inst->SrcReg[optype][2], 2, &tfactor);
		  /* fallthrough */
	       case GL_MUL_ATI:
		  r200SetFragShaderArg(afs_cmd, opnum, optype,
					inst->SrcReg[optype][0], 0, &tfactor);
		  r200SetFragShaderArg(afs_cmd, opnum, optype,
					inst->SrcReg[optype][1], 1, &tfactor);
		  SET_INST(opnum, optype) |= R200_TXC_OP_MADD;
		  break;
	       case GL_LERP_ATI:
		  /* arg order is not native chip order, swap A and C */
		  r200SetFragShaderArg(afs_cmd, opnum, optype,
					inst->SrcReg[optype][0], 2, &tfactor);
		  r200SetFragShaderArg(afs_cmd, opnum, optype,
					inst->SrcReg[optype][1], 1, &tfactor);
		  r200SetFragShaderArg(afs_cmd, opnum, optype,
					inst->SrcReg[optype][2], 0, &tfactor);
		  SET_INST(opnum, optype) |= R200_TXC_OP_LERP;
		  break;
	       case GL_CND_ATI:
		  r200SetFragShaderArg(afs_cmd, opnum, optype,
					inst->SrcReg[optype][0], 0, &tfactor);
		  r200SetFragShaderArg(afs_cmd, opnum, optype,
					inst->SrcReg[optype][1], 1, &tfactor);
		  r200SetFragShaderArg(afs_cmd, opnum, optype,
					inst->SrcReg[optype][2], 2, &tfactor);
		  SET_INST(opnum, optype) |= R200_TXC_OP_CONDITIONAL;
		  break;
	       case GL_CND0_ATI:
		  r200SetFragShaderArg(afs_cmd, opnum, optype,
					inst->SrcReg[optype][0], 0, &tfactor);
		  r200SetFragShaderArg(afs_cmd, opnum, optype,
					inst->SrcReg[optype][1], 1, &tfactor);
		  r200SetFragShaderArg(afs_cmd, opnum, optype,
					inst->SrcReg[optype][2], 2, &tfactor);
		  SET_INST(opnum, optype) |= R200_TXC_OP_CND0;
		  break;
		  /* cannot specify dot ops as alpha ops directly */
	       case GL_DOT2_ADD_ATI:
		  if (optype)
		     SET_INST_2(opnum, 1) |= R200_TXA_DOT_ALPHA;
		  else {
		     r200SetFragShaderArg(afs_cmd, opnum, 0,
					inst->SrcReg[0][0], 0, &tfactor);
		     r200SetFragShaderArg(afs_cmd, opnum, 0,
					inst->SrcReg[0][1], 1, &tfactor);
		     r200SetFragShaderArg(afs_cmd, opnum, 0,
					inst->SrcReg[0][2], 2, &tfactor);
		     SET_INST(opnum, 0) |= R200_TXC_OP_DOT2_ADD;
		  }
		  break;
	       case GL_DOT3_ATI:
		  if (optype)
		     SET_INST_2(opnum, 1) |= R200_TXA_DOT_ALPHA;
		  else {
		     r200SetFragShaderArg(afs_cmd, opnum, 0,
					inst->SrcReg[0][0], 0, &tfactor);
		     r200SetFragShaderArg(afs_cmd, opnum, 0,
					inst->SrcReg[0][1], 1, &tfactor);
		     SET_INST(opnum, 0) |= R200_TXC_OP_DOT3;
		  }
		  break;
	       case GL_DOT4_ATI:
	       /* experimental verification: for dot4 setup of alpha args is needed
		  (dstmod is ignored, though, so dot2/dot3 should be safe)
		  the hardware apparently does R1*R2 + G1*G2 + B1*B2 + A3*A4
		  but the API doesn't allow it */
		  if (optype)
		     SET_INST_2(opnum, 1) |= R200_TXA_DOT_ALPHA;
		  else {
		     r200SetFragShaderArg(afs_cmd, opnum, 0,
					inst->SrcReg[0][0], 0, &tfactor);
		     r200SetFragShaderArg(afs_cmd, opnum, 0,
					inst->SrcReg[0][1], 1, &tfactor);
		     r200SetFragShaderArg(afs_cmd, opnum, 1,
					inst->SrcReg[0][0], 0, &tfactor);
		     r200SetFragShaderArg(afs_cmd, opnum, 1,
					inst->SrcReg[0][1], 1, &tfactor);
		     SET_INST(opnum, optype) |= R200_TXC_OP_DOT4;
		  }
		  break;
	       }
	    }

	    /* destination */
	    if (inst->DstReg[optype].Index) {
	       GLuint dstreg = inst->DstReg[optype].Index - GL_REG_0_ATI;
	       GLuint dstmask = inst->DstReg[optype].dstMask;
	       GLuint sat = inst->DstReg[optype].dstMod & GL_SATURATE_BIT_ATI;
	       GLuint dstmod = inst->DstReg[optype].dstMod;

	       dstmod &= ~GL_SATURATE_BIT_ATI;

	       SET_INST_2(opnum, optype) |= (dstreg + 1) << R200_TXC_OUTPUT_REG_SHIFT;
	       SET_INST_2(opnum, optype) |= dstmask_table[dstmask];

		/* fglrx does clamp the last instructions to 0_1 it seems */
		/* this won't necessarily catch the last instruction
		   which writes to reg0 */
	       if (sat || (pc == (shader->numArithInstr[pass] - 1) &&
			((pass == 1) || (shader->NumPasses == 1))))
		  SET_INST_2(opnum, optype) |= R200_TXC_CLAMP_0_1;
	       else
		/*should we clamp or not? spec is vague, I would suppose yes but fglrx doesn't */
		  SET_INST_2(opnum, optype) |= R200_TXC_CLAMP_8_8;
/*		  SET_INST_2(opnum, optype) |= R200_TXC_CLAMP_WRAP;*/
	       switch(dstmod) {
	       case GL_2X_BIT_ATI:
		  SET_INST_2(opnum, optype) |= R200_TXC_SCALE_2X;
		  break;
	       case GL_4X_BIT_ATI:
		  SET_INST_2(opnum, optype) |= R200_TXC_SCALE_4X;
		  break;
	       case GL_8X_BIT_ATI:
		  SET_INST_2(opnum, optype) |= R200_TXC_SCALE_8X;
		  break;
	       case GL_HALF_BIT_ATI:
		  SET_INST_2(opnum, optype) |= R200_TXC_SCALE_INV2;
		  break;
	       case GL_QUARTER_BIT_ATI:
		  SET_INST_2(opnum, optype) |= R200_TXC_SCALE_INV4;
		  break;
	       case GL_EIGHTH_BIT_ATI:
		  SET_INST_2(opnum, optype) |= R200_TXC_SCALE_INV8;
		  break;
	       default:
		  break;
	       }
	    }
	 }
/*	 fprintf(stderr, "pass %d nr %d inst 0x%.8x 0x%.8x 0x%.8x 0x%.8x\n",
		pass, opnum, SET_INST(opnum, 0), SET_INST_2(opnum, 0),
		SET_INST(opnum, 1), SET_INST_2(opnum, 1));*/
         opnum++;
      }
      afs_cmd = (GLuint *) rmesa->hw.afs[1].cmd;
   }
   rmesa->afs_loaded = ctx->ATIFragmentShader.Current;
}

static void r200UpdateFSRouting( GLcontext *ctx ) {
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   const struct ati_fragment_shader *shader = ctx->ATIFragmentShader.Current;
   GLuint reg;

   R200_STATECHANGE( rmesa, ctx );
   R200_STATECHANGE( rmesa, cst );

   for (reg = 0; reg < R200_MAX_TEXTURE_UNITS; reg++) {
      if (shader->swizzlerq & (1 << (2 * reg)))
	 /* r coord */
	 set_re_cntl_d3d( ctx, reg, 1);
	 /* q coord */
      else set_re_cntl_d3d( ctx, reg, 0);
   }

   rmesa->hw.ctx.cmd[CTX_PP_CNTL] &= ~(R200_MULTI_PASS_ENABLE |
				       R200_TEX_BLEND_ENABLE_MASK |
				       R200_TEX_ENABLE_MASK);
   rmesa->hw.cst.cmd[CST_PP_CNTL_X] &= ~(R200_PPX_PFS_INST_ENABLE_MASK |
					 R200_PPX_TEX_ENABLE_MASK |
					 R200_PPX_OUTPUT_REG_MASK);

   /* first pass registers use slots 8 - 15
      but single pass shaders use slots 0 - 7 */
   if (shader->NumPasses < 2) {
      rmesa->hw.ctx.cmd[CTX_PP_CNTL] |= shader->numArithInstr[0] == 8 ?
	 0xff << (R200_TEX_BLEND_0_ENABLE_SHIFT - 1) :
	 (0xff >> (8 - shader->numArithInstr[0])) << R200_TEX_BLEND_0_ENABLE_SHIFT;
   } else {
      rmesa->hw.ctx.cmd[CTX_PP_CNTL] |= R200_MULTI_PASS_ENABLE;
      rmesa->hw.ctx.cmd[CTX_PP_CNTL] |= shader->numArithInstr[1] == 8 ?
	 0xff << (R200_TEX_BLEND_0_ENABLE_SHIFT - 1) :
	 (0xff >> (8 - shader->numArithInstr[1])) << R200_TEX_BLEND_0_ENABLE_SHIFT;
      rmesa->hw.cst.cmd[CST_PP_CNTL_X] |=
	 (0xff >> (8 - shader->numArithInstr[0])) << R200_PPX_FPS_INST0_ENABLE_SHIFT;
   }

   if (shader->NumPasses < 2) {
      for (reg = 0; reg < R200_MAX_TEXTURE_UNITS; reg++) {
	 GLbitfield targetbit = ctx->Texture.Unit[reg]._ReallyEnabled;
         R200_STATECHANGE( rmesa, tex[reg] );
	 rmesa->hw.tex[reg].cmd[TEX_PP_TXMULTI_CTL] = 0;
	 if (shader->SetupInst[0][reg].Opcode) {
	    GLuint txformat = rmesa->hw.tex[reg].cmd[TEX_PP_TXFORMAT]
		& ~(R200_TXFORMAT_ST_ROUTE_MASK | R200_TXFORMAT_LOOKUP_DISABLE);
	    GLuint txformat_x = rmesa->hw.tex[reg].cmd[TEX_PP_TXFORMAT_X] & ~R200_TEXCOORD_MASK;
	    txformat |= (shader->SetupInst[0][reg].src - GL_TEXTURE0_ARB)
		<< R200_TXFORMAT_ST_ROUTE_SHIFT;
	    /* fix up texcoords for proj/non-proj 2d (3d and cube are not defined when
	       using projection so don't have to worry there).
	       When passing coords, need R200_TEXCOORD_VOLUME, otherwise loose a coord */
	    /* FIXME: someone might rely on default tex coords r/q, which we unfortunately
	       don't provide (we have the same problem without shaders) */
	    if (shader->SetupInst[0][reg].Opcode == ATI_FRAGMENT_SHADER_PASS_OP) {
	       txformat |= R200_TXFORMAT_LOOKUP_DISABLE;
	       if (shader->SetupInst[0][reg].swizzle == GL_SWIZZLE_STR_ATI ||
		  shader->SetupInst[0][reg].swizzle == GL_SWIZZLE_STQ_ATI) {
		  txformat_x |= R200_TEXCOORD_VOLUME;
	       }
	       else {
		  txformat_x |= R200_TEXCOORD_PROJ;
	       }
	       rmesa->hw.ctx.cmd[CTX_PP_CNTL] |= R200_TEX_0_ENABLE << reg;
	    }
	    else if (targetbit == TEXTURE_3D_BIT) {
	       txformat_x |= R200_TEXCOORD_VOLUME;
	    }
	    else if (targetbit == TEXTURE_CUBE_BIT) {
	       txformat_x |= R200_TEXCOORD_CUBIC_ENV;
	    }
	    else if (shader->SetupInst[0][reg].swizzle == GL_SWIZZLE_STR_ATI ||
	       shader->SetupInst[0][reg].swizzle == GL_SWIZZLE_STQ_ATI) {
	       txformat_x |= R200_TEXCOORD_NONPROJ;
	    }
	    else {
	       txformat_x |= R200_TEXCOORD_PROJ;
	    }
	    rmesa->hw.tex[reg].cmd[TEX_PP_TXFORMAT] = txformat;
	    rmesa->hw.tex[reg].cmd[TEX_PP_TXFORMAT_X] = txformat_x;
	    /* enabling texturing when unit isn't correctly configured may not be safe */
	    if (targetbit)
	       rmesa->hw.ctx.cmd[CTX_PP_CNTL] |= R200_TEX_0_ENABLE << reg;
	 }
      }

   } else {
      /* setup 1st pass */
      for (reg = 0; reg < R200_MAX_TEXTURE_UNITS; reg++) {
	 GLbitfield targetbit = ctx->Texture.Unit[reg]._ReallyEnabled;
	 R200_STATECHANGE( rmesa, tex[reg] );
	 GLuint txformat_multi = 0;
	 if (shader->SetupInst[0][reg].Opcode) {
	    txformat_multi |= (shader->SetupInst[0][reg].src - GL_TEXTURE0_ARB)
		<< R200_PASS1_ST_ROUTE_SHIFT;
	    if (shader->SetupInst[0][reg].Opcode == ATI_FRAGMENT_SHADER_PASS_OP) {
	       txformat_multi |= R200_PASS1_TXFORMAT_LOOKUP_DISABLE;
	       if (shader->SetupInst[0][reg].swizzle == GL_SWIZZLE_STR_ATI ||
		  shader->SetupInst[0][reg].swizzle == GL_SWIZZLE_STQ_ATI) {
		  txformat_multi |= R200_PASS1_TEXCOORD_VOLUME;
	       }
	       else {
		  txformat_multi |= R200_PASS1_TEXCOORD_PROJ;
	       }
	       rmesa->hw.cst.cmd[CST_PP_CNTL_X] |= R200_PPX_TEX_0_ENABLE << reg;
	    }
	    else if (targetbit == TEXTURE_3D_BIT) {
	       txformat_multi |= R200_PASS1_TEXCOORD_VOLUME;
	    }
	    else if (targetbit == TEXTURE_CUBE_BIT) {
	       txformat_multi |= R200_PASS1_TEXCOORD_CUBIC_ENV;
	    }
	    else if (shader->SetupInst[0][reg].swizzle == GL_SWIZZLE_STR_ATI ||
		  shader->SetupInst[0][reg].swizzle == GL_SWIZZLE_STQ_ATI) {
		  txformat_multi |= R200_PASS1_TEXCOORD_NONPROJ;
	    }
	    else {
	       txformat_multi |= R200_PASS1_TEXCOORD_PROJ;
	    }
	    if (targetbit)
	       rmesa->hw.cst.cmd[CST_PP_CNTL_X] |= R200_PPX_TEX_0_ENABLE << reg;
	 }
         rmesa->hw.tex[reg].cmd[TEX_PP_TXMULTI_CTL] = txformat_multi;
      }

      /* setup 2nd pass */
      for (reg=0; reg < R200_MAX_TEXTURE_UNITS; reg++) {
	 GLbitfield targetbit = ctx->Texture.Unit[reg]._ReallyEnabled;
	 if (shader->SetupInst[1][reg].Opcode) {
	    GLuint coord = shader->SetupInst[1][reg].src;
	    GLuint txformat = rmesa->hw.tex[reg].cmd[TEX_PP_TXFORMAT]
		& ~(R200_TXFORMAT_ST_ROUTE_MASK | R200_TXFORMAT_LOOKUP_DISABLE);
	    GLuint txformat_x = rmesa->hw.tex[reg].cmd[TEX_PP_TXFORMAT_X] & ~R200_TEXCOORD_MASK;
	    R200_STATECHANGE( rmesa, tex[reg] );
	    if (shader->SetupInst[1][reg].Opcode == ATI_FRAGMENT_SHADER_PASS_OP) {
	       txformat |= R200_TXFORMAT_LOOKUP_DISABLE;
	       txformat_x |= R200_TEXCOORD_VOLUME;
	       if (shader->SetupInst[1][reg].swizzle == GL_SWIZZLE_STR_ATI ||
		  shader->SetupInst[1][reg].swizzle == GL_SWIZZLE_STQ_ATI) {
	          txformat_x |= R200_TEXCOORD_VOLUME;
	       }
	       else {
		  txformat_x |= R200_TEXCOORD_PROJ;
	       }
	       rmesa->hw.ctx.cmd[CTX_PP_CNTL] |= R200_TEX_0_ENABLE << reg;
	    }
	    else if (targetbit == TEXTURE_3D_BIT) {
	       txformat_x |= R200_TEXCOORD_VOLUME;
	    }
	    else if (targetbit == TEXTURE_CUBE_BIT) {
	       txformat_x |= R200_TEXCOORD_CUBIC_ENV;
	    }
	    else if (shader->SetupInst[1][reg].swizzle == GL_SWIZZLE_STR_ATI ||
	       shader->SetupInst[1][reg].swizzle == GL_SWIZZLE_STQ_ATI) {
	       txformat_x |= R200_TEXCOORD_NONPROJ;
	    }
	    else {
	       txformat_x |= R200_TEXCOORD_PROJ;
	    }
	    if (coord >= GL_REG_0_ATI) {
	       GLuint txformat_multi = rmesa->hw.tex[reg].cmd[TEX_PP_TXMULTI_CTL];
	       txformat_multi |= (coord - GL_REG_0_ATI + 2) << R200_PASS2_COORDS_REG_SHIFT;
	       rmesa->hw.tex[reg].cmd[TEX_PP_TXMULTI_CTL] = txformat_multi;
	       rmesa->hw.cst.cmd[CST_PP_CNTL_X] |= 1 <<
		  (R200_PPX_OUTPUT_REG_0_SHIFT + coord - GL_REG_0_ATI);
	    } else {
	       txformat |= (coord - GL_TEXTURE0_ARB) << R200_TXFORMAT_ST_ROUTE_SHIFT;
	    }
	    rmesa->hw.tex[reg].cmd[TEX_PP_TXFORMAT_X] = txformat_x;
	    rmesa->hw.tex[reg].cmd[TEX_PP_TXFORMAT] = txformat;
	    if (targetbit)
	       rmesa->hw.ctx.cmd[CTX_PP_CNTL] |= R200_TEX_0_ENABLE << reg;
	 }
      }
   }
}

static void r200UpdateFSConstants( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);
   const struct ati_fragment_shader *shader = ctx->ATIFragmentShader.Current;
   GLuint i;

   /* update constants */
   R200_STATECHANGE(rmesa, atf);
   for (i = 0; i < 8; i++)
   {
      GLubyte con_byte[4];
      if ((shader->LocalConstDef >> i) & 1) {
	 CLAMPED_FLOAT_TO_UBYTE(con_byte[0], shader->Constants[i][0]);
	 CLAMPED_FLOAT_TO_UBYTE(con_byte[1], shader->Constants[i][1]);
	 CLAMPED_FLOAT_TO_UBYTE(con_byte[2], shader->Constants[i][2]);
	 CLAMPED_FLOAT_TO_UBYTE(con_byte[3], shader->Constants[i][3]);
      }
      else {
	 CLAMPED_FLOAT_TO_UBYTE(con_byte[0], ctx->ATIFragmentShader.GlobalConstants[i][0]);
	 CLAMPED_FLOAT_TO_UBYTE(con_byte[1], ctx->ATIFragmentShader.GlobalConstants[i][1]);
	 CLAMPED_FLOAT_TO_UBYTE(con_byte[2], ctx->ATIFragmentShader.GlobalConstants[i][2]);
	 CLAMPED_FLOAT_TO_UBYTE(con_byte[3], ctx->ATIFragmentShader.GlobalConstants[i][3]);
      }
      rmesa->hw.atf.cmd[ATF_TFACTOR_0 + i] = r200PackColor (
	 4, con_byte[0], con_byte[1], con_byte[2], con_byte[3] );
   }
}

/* update routing, constants and arithmetic
 * constants need to be updated always (globals can change, no separate notification)
 * routing needs to be updated always too (non-shader code will overwrite state, plus
 * some of the routing depends on what sort of texture is bound)
 * for both of them, we need to update anyway because of disabling/enabling ati_fs which
 * we'd need to track otherwise
 * arithmetic is only updated if current shader changes (and probably the data should be
 * stored in some DriverData object attached to the mesa atifs object, i.e. binding a
 * shader wouldn't force us to "recompile" the shader).
 */
void r200UpdateFragmentShader( GLcontext *ctx )
{
   r200ContextPtr rmesa = R200_CONTEXT(ctx);

   r200UpdateFSConstants( ctx );
   r200UpdateFSRouting( ctx );
   if (rmesa->afs_loaded != ctx->ATIFragmentShader.Current)
      r200UpdateFSArith( ctx );
}
