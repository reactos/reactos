/*
 *
 * Copyright (C) 2004  David Airlie   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * DAVID AIRLIE BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "atifragshader.h"
#include "macros.h"
#include "program.h"

#include "s_atifragshader.h"
#include "s_nvfragprog.h"
#include "s_span.h"
#include "s_texture.h"

/**
 * Fetch a texel.
 */
static void
fetch_texel(GLcontext * ctx, const GLfloat texcoord[4], GLfloat lambda,
	    GLuint unit, GLfloat color[4])
{
   GLchan rgba[4];
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   /* XXX use a float-valued TextureSample routine here!!! */
   swrast->TextureSample[unit] (ctx, unit, ctx->Texture.Unit[unit]._Current,
				1, (const GLfloat(*)[4]) texcoord,
				&lambda, &rgba);
   color[0] = CHAN_TO_FLOAT(rgba[0]);
   color[1] = CHAN_TO_FLOAT(rgba[1]);
   color[2] = CHAN_TO_FLOAT(rgba[2]);
   color[3] = CHAN_TO_FLOAT(rgba[3]);
}

static void
apply_swizzle(struct atifs_machine *machine, GLuint reg, GLuint swizzle)
{
   GLfloat s, t, r, q;

   s = machine->Registers[reg][0];
   t = machine->Registers[reg][1];
   r = machine->Registers[reg][2];
   q = machine->Registers[reg][3];

   switch (swizzle) {
   case GL_SWIZZLE_STR_ATI:
      machine->Registers[reg][0] = s;
      machine->Registers[reg][1] = t;
      machine->Registers[reg][2] = r;
      break;
   case GL_SWIZZLE_STQ_ATI:
      machine->Registers[reg][0] = s;
      machine->Registers[reg][1] = t;
      machine->Registers[reg][2] = q;
      break;
   case GL_SWIZZLE_STR_DR_ATI:
      machine->Registers[reg][0] = s / r;
      machine->Registers[reg][1] = t / r;
      machine->Registers[reg][2] = 1 / r;
      break;
   case GL_SWIZZLE_STQ_DQ_ATI:
      machine->Registers[reg][0] = s / q;
      machine->Registers[reg][1] = t / q;
      machine->Registers[reg][2] = 1 / q;
      break;
   }
   machine->Registers[reg][3] = 0.0;
}

static void
apply_src_rep(GLint optype, GLuint rep, GLfloat * val)
{
   GLint i;
   GLint start, end;
   if (!rep)
      return;

   start = optype ? 3 : 0;
   end = optype ? 4 : 3;

   for (i = start; i < end; i++) {
      switch (rep) {
      case GL_RED:
	 val[i] = val[0];
	 break;
      case GL_GREEN:
	 val[i] = val[1];
	 break;
      case GL_BLUE:
	 val[i] = val[2];
	 break;
      case GL_ALPHA:
	 val[i] = val[3];
	 break;
      }
   }
}

static void
apply_src_mod(GLint optype, GLuint mod, GLfloat * val)
{
   GLint i;
   GLint start, end;

   if (!mod)
      return;

   start = optype ? 3 : 0;
   end = optype ? 4 : 3;

   for (i = start; i < end; i++) {
      if (mod & GL_COMP_BIT_ATI)
	 val[i] = 1 - val[i];

      if (mod & GL_BIAS_BIT_ATI)
	 val[i] = val[i] - 0.5;

      if (mod & GL_2X_BIT_ATI)
	 val[i] = 2 * val[i];

      if (mod & GL_NEGATE_BIT_ATI)
	 val[i] = -val[i];
   }
}

static void
apply_dst_mod(GLuint optype, GLuint mod, GLfloat * val)
{
   GLint i;
   GLint has_sat = mod & GL_SATURATE_BIT_ATI;
   GLint start, end;

   mod &= ~GL_SATURATE_BIT_ATI;

   start = optype ? 3 : 0;
   end = optype ? 4 : 3;

   for (i = start; i < end; i++) {
      switch (mod) {
      case GL_2X_BIT_ATI:
	 val[i] = 2 * val[i];
	 break;
      case GL_4X_BIT_ATI:
	 val[i] = 4 * val[i];
	 break;
      case GL_8X_BIT_ATI:
	 val[i] = 8 * val[i];
	 break;
      case GL_HALF_BIT_ATI:
	 val[i] = val[i] * 0.5;
	 break;
      case GL_QUARTER_BIT_ATI:
	 val[i] = val[i] * 0.25;
	 break;
      case GL_EIGHTH_BIT_ATI:
	 val[i] = val[i] * 0.125;
	 break;
      }

      if (has_sat) {
	 if (val[i] < 0.0)
	    val[i] = 0;
	 else if (val[i] > 1.0)
	    val[i] = 1.0;
      }
      else {
	 if (val[i] < -8.0)
	    val[i] = -8.0;
	 else if (val[i] > 8.0)
	    val[i] = 8.0;
      }
   }
}


static void
write_dst_addr(GLuint optype, GLuint mod, GLuint mask, GLfloat * src,
	       GLfloat * dst)
{
   GLint i;
   apply_dst_mod(optype, mod, src);

   if (optype == ATI_FRAGMENT_SHADER_COLOR_OP) {
      if (mask) {
	 if (mask & GL_RED_BIT_ATI)
	    dst[0] = src[0];

	 if (mask & GL_GREEN_BIT_ATI)
	    dst[1] = src[1];

	 if (mask & GL_BLUE_BIT_ATI)
	    dst[2] = src[2];
      }
      else {
	 for (i = 0; i < 3; i++)
	    dst[i] = src[i];
      }
   }
   else
      dst[3] = src[3];
}

static void
finish_pass(struct atifs_machine *machine)
{
   GLint i;

   for (i = 0; i < 6; i++) {
      COPY_4V(machine->PrevPassRegisters[i], machine->Registers[i]);
   }
}

/**
 * Execute the given fragment shader
 * NOTE: we do everything in single-precision floating point; we don't
 * currently observe the single/half/fixed-precision qualifiers.
 * \param ctx - rendering context
 * \param program - the fragment program to execute
 * \param machine - machine state (register file)
 * \param maxInst - max number of instructions to execute
 * \return GL_TRUE if program completed or GL_FALSE if program executed KIL.
 */

struct ati_fs_opcode_st ati_fs_opcodes[] = {
   {GL_ADD_ATI, 2},
   {GL_SUB_ATI, 2},
   {GL_MUL_ATI, 2},
   {GL_MAD_ATI, 3},
   {GL_LERP_ATI, 3},
   {GL_MOV_ATI, 1},
   {GL_CND_ATI, 3},
   {GL_CND0_ATI, 3},
   {GL_DOT2_ADD_ATI, 3},
   {GL_DOT3_ATI, 2},
   {GL_DOT4_ATI, 2}
};



static void
handle_pass_op(struct atifs_machine *machine, struct atifs_instruction *inst,
	       const struct sw_span *span, GLuint column)
{
   GLuint idx = inst->DstReg[0].Index - GL_REG_0_ATI;
   GLuint swizzle = inst->DstReg[0].Swizzle;
   GLuint pass_tex = inst->SrcReg[0][0].Index;

   /* if we get here after passing pass one then we are starting pass two - backup the registers */
   if (machine->pass == 1) {
      finish_pass(machine);
      machine->pass = 2;
   }
   if (pass_tex >= GL_TEXTURE0_ARB && pass_tex <= GL_TEXTURE7_ARB) {
      pass_tex -= GL_TEXTURE0_ARB;
      COPY_4V(machine->Registers[idx],
	      span->array->texcoords[pass_tex][column]);
   }
   else if (pass_tex >= GL_REG_0_ATI && pass_tex <= GL_REG_5_ATI
	    && machine->pass == 2) {
      pass_tex -= GL_REG_0_ATI;
      COPY_4V(machine->Registers[idx], machine->PrevPassRegisters[pass_tex]);
   }
   apply_swizzle(machine, idx, swizzle);

}

static void
handle_sample_op(GLcontext * ctx, struct atifs_machine *machine,
		 struct atifs_instruction *inst, const struct sw_span *span,
		 GLuint column)
{
   GLuint idx = inst->DstReg[0].Index - GL_REG_0_ATI;
   GLuint swizzle = inst->DstReg[0].Swizzle;
   GLuint sample_tex = inst->SrcReg[0][0].Index;

   /* if we get here after passing pass one then we are starting pass two - backup the registers */
   if (machine->pass == 1) {
      finish_pass(machine);
      machine->pass = 2;
   }

   if (sample_tex >= GL_TEXTURE0_ARB && sample_tex <= GL_TEXTURE7_ARB) {
      sample_tex -= GL_TEXTURE0_ARB;
      fetch_texel(ctx, span->array->texcoords[sample_tex][column], 0.0F,
		  sample_tex, machine->Registers[idx]);
   }
   else if (sample_tex >= GL_REG_0_ATI && sample_tex <= GL_REG_5_ATI) {
      /* this is wrong... */
      sample_tex -= GL_REG_0_ATI;
      fetch_texel(ctx, machine->Registers[sample_tex], 0, sample_tex,
		  machine->Registers[idx]);
   }

   apply_swizzle(machine, idx, swizzle);
}

#define SETUP_SRC_REG(optype, i, x)	     do {	\
    if (optype) \
      src[optype][i][3] = x[3]; \
    else \
      COPY_3V(src[optype][i], x); \
  } while (0)

static GLboolean
execute_shader(GLcontext * ctx,
	       const struct ati_fragment_shader *shader, GLuint maxInst,
	       struct atifs_machine *machine, const struct sw_span *span,
	       GLuint column)
{
   GLuint pc;
   struct atifs_instruction *inst;
   GLint optype;
   GLint i;
   GLint dstreg;
   GLfloat src[2][3][4];
   GLfloat zeros[4] = { 0.0, 0.0, 0.0, 0.0 };
   GLfloat ones[4] = { 1.0, 1.0, 1.0, 1.0 };
   GLfloat dst[2][4], *dstp;

   for (pc = 0; pc < shader->Base.NumInstructions; pc++) {
      inst = &shader->Instructions[pc];

      if (inst->Opcode[0] == ATI_FRAGMENT_SHADER_PASS_OP)
	 handle_pass_op(machine, inst, span, column);
      else if (inst->Opcode[0] == ATI_FRAGMENT_SHADER_SAMPLE_OP)
	 handle_sample_op(ctx, machine, inst, span, column);
      else {
	 if (machine->pass == 0)
	    machine->pass = 1;

	 /* setup the source registers for color and alpha ops */
	 for (optype = 0; optype < 2; optype++) {
	    for (i = 0; i < inst->ArgCount[optype]; i++) {
	       GLint index = inst->SrcReg[optype][i].Index;

	       if (index >= GL_REG_0_ATI && index <= GL_REG_5_ATI)
		  SETUP_SRC_REG(optype, i,
				machine->Registers[index - GL_REG_0_ATI]);
	       else if (index >= GL_CON_0_ATI && index <= GL_CON_7_ATI)
		  SETUP_SRC_REG(optype, i,
				shader->Constants[index - GL_CON_0_ATI]);
	       else if (index == GL_ONE)
		  SETUP_SRC_REG(optype, i, ones);
	       else if (index == GL_ZERO)
		  SETUP_SRC_REG(optype, i, zeros);
	       else if (index == GL_PRIMARY_COLOR_EXT)
		  SETUP_SRC_REG(optype, i,
				machine->Inputs[ATI_FS_INPUT_PRIMARY]);
	       else if (index == GL_SECONDARY_INTERPOLATOR_ATI)
		  SETUP_SRC_REG(optype, i,
				machine->Inputs[ATI_FS_INPUT_SECONDARY]);

	       apply_src_rep(optype, inst->SrcReg[optype][i].argRep,
			     src[optype][i]);
	       apply_src_mod(optype, inst->SrcReg[optype][i].argMod,
			     src[optype][i]);
	    }
	 }

	 /* Execute the operations - color then alpha */
	 for (optype = 0; optype < 2; optype++) {
	    if (inst->Opcode[optype]) {
	       switch (inst->Opcode[optype]) {
	       case GL_ADD_ATI:
		  if (!optype)
		     for (i = 0; i < 3; i++) {
			dst[optype][i] =
			   src[optype][0][i] + src[optype][1][i];
		     }
		  else
		     dst[optype][3] = src[optype][0][3] + src[optype][1][3];
		  break;
	       case GL_SUB_ATI:
		  if (!optype)
		     for (i = 0; i < 3; i++) {
			dst[optype][i] =
			   src[optype][0][i] - src[optype][1][i];
		     }
		  else
		     dst[optype][3] = src[optype][0][3] - src[optype][1][3];
		  break;
	       case GL_MUL_ATI:
		  if (!optype)
		     for (i = 0; i < 3; i++) {
			dst[optype][i] =
			   src[optype][0][i] * src[optype][1][i];
		     }
		  else
		     dst[optype][3] = src[optype][0][3] * src[optype][1][3];
		  break;
	       case GL_MAD_ATI:
		  if (!optype)
		     for (i = 0; i < 3; i++) {
			dst[optype][i] =
			   src[optype][0][i] * src[optype][1][i] +
			   src[optype][2][i];
		     }
		  else
		     dst[optype][3] =
			src[optype][0][3] * src[optype][1][3] +
			src[optype][2][3];
		  break;
	       case GL_LERP_ATI:
		  if (!optype)
		     for (i = 0; i < 3; i++) {
			dst[optype][i] =
			   src[optype][0][i] * src[optype][1][i] + (1 -
								    src
								    [optype]
								    [0][i]) *
			   src[optype][2][i];
		     }
		  else
		     dst[optype][3] =
			src[optype][0][3] * src[optype][1][3] + (1 -
								 src[optype]
								 [0][3]) *
			src[optype][2][3];
		  break;

	       case GL_MOV_ATI:
		  if (!optype)
		     for (i = 0; i < 3; i++) {
			dst[optype][i] = src[optype][0][i];
		     }
		  else
		     dst[optype][3] = src[optype][0][3];
		  break;
	       case GL_CND_ATI:
		  if (!optype) {
		     for (i = 0; i < 3; i++) {
			dst[optype][i] =
			   (src[optype][2][i] >
			    0.5) ? src[optype][0][i] : src[optype][1][i];
		     }
		  }
		  else {
		     dst[optype][3] =
			(src[optype][2][3] >
			 0.5) ? src[optype][0][3] : src[optype][1][3];
		  }
		  break;

	       case GL_CND0_ATI:
		  if (!optype)
		     for (i = 0; i < 3; i++) {
			dst[optype][i] =
			   (src[optype][2][i] >=
			    0) ? src[optype][0][i] : src[optype][1][i];
		     }
		  else {
		     dst[optype][3] =
			(src[optype][2][3] >=
			 0) ? src[optype][0][3] : src[optype][1][3];
		  }
		  break;
	       case GL_DOT2_ADD_ATI:
		  {
		     GLfloat result;

		     /* DOT 2 always uses the source from the color op */
		     result = src[0][0][0] * src[0][1][0] +
			src[0][0][1] * src[0][1][1] + src[0][2][2];
		     if (!optype) {
			for (i = 0; i < 3; i++) {
			   dst[optype][i] = result;
			}
		     }
		     else
			dst[optype][3] = result;

		  }
		  break;
	       case GL_DOT3_ATI:
		  {
		     GLfloat result;

		     /* DOT 3 always uses the source from the color op */
		     result = src[0][0][0] * src[0][1][0] +
			src[0][0][1] * src[0][1][1] +
			src[0][0][2] * src[0][1][2];

		     if (!optype) {
			for (i = 0; i < 3; i++) {
			   dst[optype][i] = result;
			}
		     }
		     else
			dst[optype][3] = result;
		  }
		  break;
	       case GL_DOT4_ATI:
		  {
		     GLfloat result;

		     /* DOT 4 always uses the source from the color op */
		     result = src[optype][0][0] * src[0][1][0] +
			src[0][0][1] * src[0][1][1] +
			src[0][0][2] * src[0][1][2] +
			src[0][0][3] * src[0][1][3];
		     if (!optype) {
			for (i = 0; i < 3; i++) {
			   dst[optype][i] = result;
			}
		     }
		     else
			dst[optype][3] = result;
		  }
		  break;

	       }
	    }
	 }

	 /* write out the destination registers */
	 for (optype = 0; optype < 2; optype++) {
	    if (inst->Opcode[optype]) {
	       dstreg = inst->DstReg[optype].Index;
	       dstp = machine->Registers[dstreg - GL_REG_0_ATI];

	       write_dst_addr(optype, inst->DstReg[optype].dstMod,
			      inst->DstReg[optype].dstMask, dst[optype],
			      dstp);
	    }
	 }
      }
   }
   return GL_TRUE;
}

static void
init_machine(GLcontext * ctx, struct atifs_machine *machine,
	     const struct ati_fragment_shader *shader,
	     const struct sw_span *span, GLuint col)
{
   GLint i, j;

   for (i = 0; i < 6; i++) {
      for (j = 0; j < 4; j++)
	 ctx->ATIFragmentShader.Machine.Registers[i][j] = 0.0;

   }

   ctx->ATIFragmentShader.Machine.Inputs[ATI_FS_INPUT_PRIMARY][0] =
      CHAN_TO_FLOAT(span->array->rgba[col][0]);
   ctx->ATIFragmentShader.Machine.Inputs[ATI_FS_INPUT_PRIMARY][1] =
      CHAN_TO_FLOAT(span->array->rgba[col][1]);
   ctx->ATIFragmentShader.Machine.Inputs[ATI_FS_INPUT_PRIMARY][2] =
      CHAN_TO_FLOAT(span->array->rgba[col][2]);
   ctx->ATIFragmentShader.Machine.Inputs[ATI_FS_INPUT_PRIMARY][3] =
      CHAN_TO_FLOAT(span->array->rgba[col][3]);

   ctx->ATIFragmentShader.Machine.Inputs[ATI_FS_INPUT_SECONDARY][0] =
      CHAN_TO_FLOAT(span->array->spec[col][0]);
   ctx->ATIFragmentShader.Machine.Inputs[ATI_FS_INPUT_SECONDARY][1] =
      CHAN_TO_FLOAT(span->array->spec[col][1]);
   ctx->ATIFragmentShader.Machine.Inputs[ATI_FS_INPUT_SECONDARY][2] =
      CHAN_TO_FLOAT(span->array->spec[col][2]);
   ctx->ATIFragmentShader.Machine.Inputs[ATI_FS_INPUT_SECONDARY][3] =
      CHAN_TO_FLOAT(span->array->spec[col][3]);

   ctx->ATIFragmentShader.Machine.pass = 0;
}



/**
 * Execute the current fragment program, operating on the given span.
 */
void
_swrast_exec_fragment_shader(GLcontext * ctx, struct sw_span *span)
{
   const struct ati_fragment_shader *shader = ctx->ATIFragmentShader.Current;
   GLuint i;

   ctx->_CurrentProgram = GL_FRAGMENT_SHADER_ATI;

   for (i = 0; i < span->end; i++) {
      if (span->array->mask[i]) {
	 init_machine(ctx, &ctx->ATIFragmentShader.Machine,
		      ctx->ATIFragmentShader.Current, span, i);

	 if (execute_shader(ctx, shader, ~0,
			    &ctx->ATIFragmentShader.Machine, span, i)) {
	    span->array->mask[i] = GL_FALSE;
	 }

	 {
	    const GLfloat *colOut =
	       ctx->ATIFragmentShader.Machine.Registers[0];

	    /*fprintf(stderr,"outputs %f %f %f %f\n", colOut[0], colOut[1], colOut[2], colOut[3]); */
	    UNCLAMPED_FLOAT_TO_CHAN(span->array->rgba[i][RCOMP], colOut[0]);
	    UNCLAMPED_FLOAT_TO_CHAN(span->array->rgba[i][GCOMP], colOut[1]);
	    UNCLAMPED_FLOAT_TO_CHAN(span->array->rgba[i][BCOMP], colOut[2]);
	    UNCLAMPED_FLOAT_TO_CHAN(span->array->rgba[i][ACOMP], colOut[3]);
	 }
      }

   }


   ctx->_CurrentProgram = 0;

}
