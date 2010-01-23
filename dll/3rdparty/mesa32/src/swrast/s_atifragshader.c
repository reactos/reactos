/*
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

#include "main/glheader.h"
#include "main/colormac.h"
#include "main/context.h"
#include "main/macros.h"
#include "shader/program.h"
#include "shader/atifragshader.h"
#include "swrast/s_atifragshader.h"


/**
 * State for executing ATI fragment shader.
 */
struct atifs_machine
{
   GLfloat Registers[6][4];         /** six temporary registers */
   GLfloat PrevPassRegisters[6][4];
   GLfloat Inputs[2][4];   /** Primary, secondary input colors */
};



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
   swrast->TextureSample[unit](ctx, ctx->Texture.Unit[unit]._Current,
                               1, (const GLfloat(*)[4]) texcoord,
                               &lambda, &rgba);
   color[0] = CHAN_TO_FLOAT(rgba[0]);
   color[1] = CHAN_TO_FLOAT(rgba[1]);
   color[2] = CHAN_TO_FLOAT(rgba[2]);
   color[3] = CHAN_TO_FLOAT(rgba[3]);
}

static void
apply_swizzle(GLfloat values[4], GLuint swizzle)
{
   GLfloat s, t, r, q;

   s = values[0];
   t = values[1];
   r = values[2];
   q = values[3];

   switch (swizzle) {
   case GL_SWIZZLE_STR_ATI:
      values[0] = s;
      values[1] = t;
      values[2] = r;
      break;
   case GL_SWIZZLE_STQ_ATI:
      values[0] = s;
      values[1] = t;
      values[2] = q;
      break;
   case GL_SWIZZLE_STR_DR_ATI:
      values[0] = s / r;
      values[1] = t / r;
      values[2] = 1 / r;
      break;
   case GL_SWIZZLE_STQ_DQ_ATI:
/* make sure q is not 0 to avoid problems later with infinite values (texture lookup)? */
      if (q == 0.0F) q = 0.000000001;
      values[0] = s / q;
      values[1] = t / q;
      values[2] = 1 / q;
      break;
   }
   values[3] = 0.0;
}

static void
apply_src_rep(GLint optype, GLuint rep, GLfloat * val)
{
   GLint i;
   GLint start, end;
   if (!rep)
      return;

   start = optype ? 3 : 0;
   end = 4;

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
   end = 4;

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
handle_pass_op(struct atifs_machine *machine, struct atifs_setupinst *texinst,
	       const SWspan *span, GLuint column, GLuint idx)
{
   GLuint swizzle = texinst->swizzle;
   GLuint pass_tex = texinst->src;

   if (pass_tex >= GL_TEXTURE0_ARB && pass_tex <= GL_TEXTURE7_ARB) {
      pass_tex -= GL_TEXTURE0_ARB;
      COPY_4V(machine->Registers[idx],
	      span->array->attribs[FRAG_ATTRIB_TEX0 + pass_tex][column]);
   }
   else if (pass_tex >= GL_REG_0_ATI && pass_tex <= GL_REG_5_ATI) {
      pass_tex -= GL_REG_0_ATI;
      COPY_4V(machine->Registers[idx], machine->PrevPassRegisters[pass_tex]);
   }
   apply_swizzle(machine->Registers[idx], swizzle);

}

static void
handle_sample_op(GLcontext * ctx, struct atifs_machine *machine,
		 struct atifs_setupinst *texinst, const SWspan *span,
		 GLuint column, GLuint idx)
{
/* sample from unit idx using texinst->src as coords */
   GLuint swizzle = texinst->swizzle;
   GLuint coord_source = texinst->src;
   GLfloat tex_coords[4];

   if (coord_source >= GL_TEXTURE0_ARB && coord_source <= GL_TEXTURE7_ARB) {
      coord_source -= GL_TEXTURE0_ARB;
      COPY_4V(tex_coords,
              span->array->attribs[FRAG_ATTRIB_TEX0 + coord_source][column]);
   }
   else if (coord_source >= GL_REG_0_ATI && coord_source <= GL_REG_5_ATI) {
      coord_source -= GL_REG_0_ATI;
      COPY_4V(tex_coords, machine->PrevPassRegisters[coord_source]);
   }
   apply_swizzle(tex_coords, swizzle);
   fetch_texel(ctx, tex_coords, 0.0F, idx, machine->Registers[idx]);
}

#define SETUP_SRC_REG(optype, i, x)		\
do {						\
   COPY_4V(src[optype][i], x); 			\
} while (0)



/**
 * Execute the given fragment shader.
 * NOTE: we do everything in single-precision floating point
 * \param ctx - rendering context
 * \param shader - the shader to execute
 * \param machine - virtual machine state
 * \param span - the SWspan we're operating on
 * \param column - which pixel [i] we're operating on in the span
 */
static void
execute_shader(GLcontext *ctx, const struct ati_fragment_shader *shader,
	       struct atifs_machine *machine, const SWspan *span,
               GLuint column)
{
   GLuint pc;
   struct atifs_instruction *inst;
   struct atifs_setupinst *texinst;
   GLint optype;
   GLuint i;
   GLint j, pass;
   GLint dstreg;
   GLfloat src[2][3][4];
   GLfloat zeros[4] = { 0.0, 0.0, 0.0, 0.0 };
   GLfloat ones[4] = { 1.0, 1.0, 1.0, 1.0 };
   GLfloat dst[2][4], *dstp;

   for (pass = 0; pass < shader->NumPasses; pass++) {
      if (pass > 0)
	 finish_pass(machine);
      for (j = 0; j < MAX_NUM_FRAGMENT_REGISTERS_ATI; j++) {
	 texinst = &shader->SetupInst[pass][j];
	 if (texinst->Opcode == ATI_FRAGMENT_SHADER_PASS_OP)
	    handle_pass_op(machine, texinst, span, column, j);
	 else if (texinst->Opcode == ATI_FRAGMENT_SHADER_SAMPLE_OP)
	    handle_sample_op(ctx, machine, texinst, span, column, j);
      }

      for (pc = 0; pc < shader->numArithInstr[pass]; pc++) {
	 inst = &shader->Instructions[pass][pc];

	 /* setup the source registers for color and alpha ops */
	 for (optype = 0; optype < 2; optype++) {
 	    for (i = 0; i < inst->ArgCount[optype]; i++) {
	       GLint index = inst->SrcReg[optype][i].Index;

	       if (index >= GL_REG_0_ATI && index <= GL_REG_5_ATI)
		  SETUP_SRC_REG(optype, i,
				machine->Registers[index - GL_REG_0_ATI]);
	       else if (index >= GL_CON_0_ATI && index <= GL_CON_7_ATI) {
		  if (shader->LocalConstDef & (1 << (index - GL_CON_0_ATI))) {
		     SETUP_SRC_REG(optype, i,
				shader->Constants[index - GL_CON_0_ATI]);
		  } else {
		     SETUP_SRC_REG(optype, i,
				ctx->ATIFragmentShader.GlobalConstants[index - GL_CON_0_ATI]);
		  }
	       }
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
		     /* could save recalculation of dot products for alpha inst */
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
		     result = src[0][0][0] * src[0][1][0] +
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

	       if ((optype == 0) || ((inst->Opcode[1] != GL_DOT2_ADD_ATI) &&
		  (inst->Opcode[1] != GL_DOT3_ATI) && (inst->Opcode[1] != GL_DOT4_ATI)))
	          write_dst_addr(optype, inst->DstReg[optype].dstMod,
			      inst->DstReg[optype].dstMask, dst[optype],
			      dstp);
	       else
		  write_dst_addr(1, inst->DstReg[0].dstMod, 0, dst[1], dstp);
	    }
	 }
      }
   }
}


/**
 * Init fragment shader virtual machine state.
 */
static void
init_machine(GLcontext * ctx, struct atifs_machine *machine,
	     const struct ati_fragment_shader *shader,
	     const SWspan *span, GLuint col)
{
   GLfloat (*inputs)[4] = machine->Inputs;
   GLint i, j;

   for (i = 0; i < 6; i++) {
      for (j = 0; j < 4; j++)
	 machine->Registers[i][j] = 0.0;
   }

   COPY_4V(inputs[ATI_FS_INPUT_PRIMARY], span->array->attribs[FRAG_ATTRIB_COL0][col]);
   COPY_4V(inputs[ATI_FS_INPUT_SECONDARY], span->array->attribs[FRAG_ATTRIB_COL1][col]);
}



/**
 * Execute the current ATI shader program, operating on the given span.
 */
void
_swrast_exec_fragment_shader(GLcontext * ctx, SWspan *span)
{
   const struct ati_fragment_shader *shader = ctx->ATIFragmentShader.Current;
   struct atifs_machine machine;
   GLuint i;

   /* incoming colors should be floats */
   ASSERT(span->array->ChanType == GL_FLOAT);

   ctx->_CurrentProgram = GL_FRAGMENT_SHADER_ATI;

   for (i = 0; i < span->end; i++) {
      if (span->array->mask[i]) {
	 init_machine(ctx, &machine, shader, span, i);

	 execute_shader(ctx, shader, &machine, span, i);

         /* store result color */
	 {
	    const GLfloat *colOut = machine.Registers[0];
            /*fprintf(stderr,"outputs %f %f %f %f\n",
              colOut[0], colOut[1], colOut[2], colOut[3]); */
            COPY_4V(span->array->attribs[FRAG_ATTRIB_COL0][i], colOut);
	 }
      }
   }

   ctx->_CurrentProgram = 0;
}
