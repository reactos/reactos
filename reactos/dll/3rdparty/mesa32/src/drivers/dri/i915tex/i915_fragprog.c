/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "glheader.h"
#include "macros.h"
#include "enums.h"

#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "intel_batchbuffer.h"

#include "i915_reg.h"
#include "i915_context.h"
#include "i915_program.h"

#include "prog_instruction.h"
#include "prog_parameter.h"
#include "program.h"
#include "programopt.h"



/* 1, -1/3!, 1/5!, -1/7! */
static const GLfloat sin_constants[4] = { 1.0,
   -1.0 / (3 * 2 * 1),
   1.0 / (5 * 4 * 3 * 2 * 1),
   -1.0 / (7 * 6 * 5 * 4 * 3 * 2 * 1)
};

/* 1, -1/2!, 1/4!, -1/6! */
static const GLfloat cos_constants[4] = { 1.0,
   -1.0 / (2 * 1),
   1.0 / (4 * 3 * 2 * 1),
   -1.0 / (6 * 5 * 4 * 3 * 2 * 1)
};

/**
 * Retrieve a ureg for the given source register.  Will emit
 * constants, apply swizzling and negation as needed.
 */
static GLuint
src_vector(struct i915_fragment_program *p,
           const struct prog_src_register *source,
           const struct gl_fragment_program *program)
{
   GLuint src;

   switch (source->File) {

      /* Registers:
       */
   case PROGRAM_TEMPORARY:
      if (source->Index >= I915_MAX_TEMPORARY) {
         i915_program_error(p, "Exceeded max temporary reg");
         return 0;
      }
      src = UREG(REG_TYPE_R, source->Index);
      break;
   case PROGRAM_INPUT:
      switch (source->Index) {
      case FRAG_ATTRIB_WPOS:
         src = i915_emit_decl(p, REG_TYPE_T, p->wpos_tex, D0_CHANNEL_ALL);
         break;
      case FRAG_ATTRIB_COL0:
         src = i915_emit_decl(p, REG_TYPE_T, T_DIFFUSE, D0_CHANNEL_ALL);
         break;
      case FRAG_ATTRIB_COL1:
         src = i915_emit_decl(p, REG_TYPE_T, T_SPECULAR, D0_CHANNEL_XYZ);
         src = swizzle(src, X, Y, Z, ONE);
         break;
      case FRAG_ATTRIB_FOGC:
         src = i915_emit_decl(p, REG_TYPE_T, T_FOG_W, D0_CHANNEL_W);
         src = swizzle(src, W, W, W, W);
         break;
      case FRAG_ATTRIB_TEX0:
      case FRAG_ATTRIB_TEX1:
      case FRAG_ATTRIB_TEX2:
      case FRAG_ATTRIB_TEX3:
      case FRAG_ATTRIB_TEX4:
      case FRAG_ATTRIB_TEX5:
      case FRAG_ATTRIB_TEX6:
      case FRAG_ATTRIB_TEX7:
         src = i915_emit_decl(p, REG_TYPE_T,
                              T_TEX0 + (source->Index - FRAG_ATTRIB_TEX0),
                              D0_CHANNEL_ALL);
         break;

      default:
         i915_program_error(p, "Bad source->Index");
         return 0;
      }
      break;

      /* Various paramters and env values.  All emitted to
       * hardware as program constants.
       */
   case PROGRAM_LOCAL_PARAM:
      src = i915_emit_param4fv(p, program->Base.LocalParams[source->Index]);
      break;

   case PROGRAM_ENV_PARAM:
      src =
         i915_emit_param4fv(p,
                            p->ctx->FragmentProgram.Parameters[source->
                                                               Index]);
      break;

   case PROGRAM_CONSTANT:
   case PROGRAM_STATE_VAR:
   case PROGRAM_NAMED_PARAM:
      src =
         i915_emit_param4fv(p,
                            program->Base.Parameters->ParameterValues[source->
                                                                      Index]);
      break;

   default:
      i915_program_error(p, "Bad source->File");
      return 0;
   }

   src = swizzle(src,
                 GET_SWZ(source->Swizzle, 0),
                 GET_SWZ(source->Swizzle, 1),
                 GET_SWZ(source->Swizzle, 2), GET_SWZ(source->Swizzle, 3));

   if (source->NegateBase)
      src = negate(src,
                   GET_BIT(source->NegateBase, 0),
                   GET_BIT(source->NegateBase, 1),
                   GET_BIT(source->NegateBase, 2),
                   GET_BIT(source->NegateBase, 3));

   return src;
}


static GLuint
get_result_vector(struct i915_fragment_program *p,
                  const struct prog_instruction *inst)
{
   switch (inst->DstReg.File) {
   case PROGRAM_OUTPUT:
      switch (inst->DstReg.Index) {
      case FRAG_RESULT_COLR:
         return UREG(REG_TYPE_OC, 0);
      case FRAG_RESULT_DEPR:
         p->depth_written = 1;
         return UREG(REG_TYPE_OD, 0);
      default:
         i915_program_error(p, "Bad inst->DstReg.Index");
         return 0;
      }
   case PROGRAM_TEMPORARY:
      return UREG(REG_TYPE_R, inst->DstReg.Index);
   default:
      i915_program_error(p, "Bad inst->DstReg.File");
      return 0;
   }
}

static GLuint
get_result_flags(const struct prog_instruction *inst)
{
   GLuint flags = 0;

   if (inst->SaturateMode == SATURATE_ZERO_ONE)
      flags |= A0_DEST_SATURATE;
   if (inst->DstReg.WriteMask & WRITEMASK_X)
      flags |= A0_DEST_CHANNEL_X;
   if (inst->DstReg.WriteMask & WRITEMASK_Y)
      flags |= A0_DEST_CHANNEL_Y;
   if (inst->DstReg.WriteMask & WRITEMASK_Z)
      flags |= A0_DEST_CHANNEL_Z;
   if (inst->DstReg.WriteMask & WRITEMASK_W)
      flags |= A0_DEST_CHANNEL_W;

   return flags;
}

static GLuint
translate_tex_src_target(struct i915_fragment_program *p, GLubyte bit)
{
   switch (bit) {
   case TEXTURE_1D_INDEX:
      return D0_SAMPLE_TYPE_2D;
   case TEXTURE_2D_INDEX:
      return D0_SAMPLE_TYPE_2D;
   case TEXTURE_RECT_INDEX:
      return D0_SAMPLE_TYPE_2D;
   case TEXTURE_3D_INDEX:
      return D0_SAMPLE_TYPE_VOLUME;
   case TEXTURE_CUBE_INDEX:
      return D0_SAMPLE_TYPE_CUBE;
   default:
      i915_program_error(p, "TexSrcBit");
      return 0;
   }
}

#define EMIT_TEX( OP )						\
do {								\
   GLuint dim = translate_tex_src_target( p, inst->TexSrcTarget );	\
   GLuint sampler = i915_emit_decl(p, REG_TYPE_S,		\
				  inst->TexSrcUnit, dim);	\
   GLuint coord = src_vector( p, &inst->SrcReg[0], program);	\
   /* Texel lookup */						\
								\
   i915_emit_texld( p,						\
	       get_result_vector( p, inst ),			\
	       get_result_flags( inst ),			\
	       sampler,						\
	       coord,						\
	       OP);						\
} while (0)

#define EMIT_ARITH( OP, N )						\
do {									\
   i915_emit_arith( p,							\
	       OP,							\
	       get_result_vector( p, inst ), 				\
	       get_result_flags( inst ), 0,			\
	       (N<1)?0:src_vector( p, &inst->SrcReg[0], program),	\
	       (N<2)?0:src_vector( p, &inst->SrcReg[1], program),	\
	       (N<3)?0:src_vector( p, &inst->SrcReg[2], program));	\
} while (0)

#define EMIT_1ARG_ARITH( OP ) EMIT_ARITH( OP, 1 )
#define EMIT_2ARG_ARITH( OP ) EMIT_ARITH( OP, 2 )
#define EMIT_3ARG_ARITH( OP ) EMIT_ARITH( OP, 3 )


/* Possible concerns:
 *
 * SIN, COS -- could use another taylor step?
 * LIT      -- results seem a little different to sw mesa
 * LOG      -- different to mesa on negative numbers, but this is conformant.
 * 
 * Parse failures -- Mesa doesn't currently give a good indication
 * internally whether a particular program string parsed or not.  This
 * can lead to confusion -- hopefully we cope with it ok now.
 *
 */
static void
upload_program(struct i915_fragment_program *p)
{
   const struct gl_fragment_program *program =
      p->ctx->FragmentProgram._Current;
   const struct prog_instruction *inst = program->Base.Instructions;

/*    _mesa_debug_fp_inst(program->Base.NumInstructions, inst); */

   /* Is this a parse-failed program?  Ensure a valid program is
    * loaded, as the flagging of an error isn't sufficient to stop
    * this being uploaded to hardware.
    */
   if (inst[0].Opcode == OPCODE_END) {
      GLuint tmp = i915_get_utemp(p);
      i915_emit_arith(p,
                      A0_MOV,
                      UREG(REG_TYPE_OC, 0),
                      A0_DEST_CHANNEL_ALL, 0,
                      swizzle(tmp, ONE, ZERO, ONE, ONE), 0, 0);
      return;
   }

   while (1) {
      GLuint src0, src1, src2, flags;
      GLuint tmp = 0;

      switch (inst->Opcode) {
      case OPCODE_ABS:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         i915_emit_arith(p,
                         A0_MAX,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
                         src0, negate(src0, 1, 1, 1, 1), 0);
         break;

      case OPCODE_ADD:
         EMIT_2ARG_ARITH(A0_ADD);
         break;

      case OPCODE_CMP:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         src1 = src_vector(p, &inst->SrcReg[1], program);
         src2 = src_vector(p, &inst->SrcReg[2], program);
         i915_emit_arith(p, A0_CMP, get_result_vector(p, inst), get_result_flags(inst), 0, src0, src2, src1);   /* NOTE: order of src2, src1 */
         break;

      case OPCODE_COS:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         tmp = i915_get_utemp(p);

         i915_emit_arith(p,
                         A0_MUL,
                         tmp, A0_DEST_CHANNEL_X, 0,
                         src0, i915_emit_const1f(p, 1.0 / (M_PI * 2)), 0);

         i915_emit_arith(p, A0_MOD, tmp, A0_DEST_CHANNEL_X, 0, tmp, 0, 0);

         /* By choosing different taylor constants, could get rid of this mul:
          */
         i915_emit_arith(p,
                         A0_MUL,
                         tmp, A0_DEST_CHANNEL_X, 0,
                         tmp, i915_emit_const1f(p, (M_PI * 2)), 0);

         /* 
          * t0.xy = MUL x.xx11, x.x1111  ; x^2, x, 1, 1
          * t0 = MUL t0.xyxy t0.xx11 ; x^4, x^3, x^2, 1
          * t0 = MUL t0.xxz1 t0.z111    ; x^6 x^4 x^2 1
          * result = DP4 t0, cos_constants
          */
         i915_emit_arith(p,
                         A0_MUL,
                         tmp, A0_DEST_CHANNEL_XY, 0,
                         swizzle(tmp, X, X, ONE, ONE),
                         swizzle(tmp, X, ONE, ONE, ONE), 0);

         i915_emit_arith(p,
                         A0_MUL,
                         tmp, A0_DEST_CHANNEL_XYZ, 0,
                         swizzle(tmp, X, Y, X, ONE),
                         swizzle(tmp, X, X, ONE, ONE), 0);

         i915_emit_arith(p,
                         A0_MUL,
                         tmp, A0_DEST_CHANNEL_XYZ, 0,
                         swizzle(tmp, X, X, Z, ONE),
                         swizzle(tmp, Z, ONE, ONE, ONE), 0);

         i915_emit_arith(p,
                         A0_DP4,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
                         swizzle(tmp, ONE, Z, Y, X),
                         i915_emit_const4fv(p, cos_constants), 0);

         break;

      case OPCODE_DP3:
         EMIT_2ARG_ARITH(A0_DP3);
         break;

      case OPCODE_DP4:
         EMIT_2ARG_ARITH(A0_DP4);
         break;

      case OPCODE_DPH:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         src1 = src_vector(p, &inst->SrcReg[1], program);

         i915_emit_arith(p,
                         A0_DP4,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
                         swizzle(src0, X, Y, Z, ONE), src1, 0);
         break;

      case OPCODE_DST:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         src1 = src_vector(p, &inst->SrcReg[1], program);

         /* result[0] = 1    * 1;
          * result[1] = a[1] * b[1];
          * result[2] = a[2] * 1;
          * result[3] = 1    * b[3];
          */
         i915_emit_arith(p,
                         A0_MUL,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
                         swizzle(src0, ONE, Y, Z, ONE),
                         swizzle(src1, ONE, Y, ONE, W), 0);
         break;

      case OPCODE_EX2:
         src0 = src_vector(p, &inst->SrcReg[0], program);

         i915_emit_arith(p,
                         A0_EXP,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
                         swizzle(src0, X, X, X, X), 0, 0);
         break;

      case OPCODE_FLR:
         EMIT_1ARG_ARITH(A0_FLR);
         break;

      case OPCODE_FRC:
         EMIT_1ARG_ARITH(A0_FRC);
         break;

      case OPCODE_KIL:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         tmp = i915_get_utemp(p);

         i915_emit_texld(p, tmp, A0_DEST_CHANNEL_ALL,   /* use a dummy dest reg */
                         0, src0, T0_TEXKILL);
         break;

      case OPCODE_LG2:
         src0 = src_vector(p, &inst->SrcReg[0], program);

         i915_emit_arith(p,
                         A0_LOG,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
                         swizzle(src0, X, X, X, X), 0, 0);
         break;

      case OPCODE_LIT:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         tmp = i915_get_utemp(p);

         /* tmp = max( a.xyzw, a.00zw )
          * XXX: Clamp tmp.w to -128..128
          * tmp.y = log(tmp.y)
          * tmp.y = tmp.w * tmp.y
          * tmp.y = exp(tmp.y)
          * result = cmp (a.11-x1, a.1x01, a.1xy1 )
          */
         i915_emit_arith(p, A0_MAX, tmp, A0_DEST_CHANNEL_ALL, 0,
                         src0, swizzle(src0, ZERO, ZERO, Z, W), 0);

         i915_emit_arith(p, A0_LOG, tmp, A0_DEST_CHANNEL_Y, 0,
                         swizzle(tmp, Y, Y, Y, Y), 0, 0);

         i915_emit_arith(p, A0_MUL, tmp, A0_DEST_CHANNEL_Y, 0,
                         swizzle(tmp, ZERO, Y, ZERO, ZERO),
                         swizzle(tmp, ZERO, W, ZERO, ZERO), 0);

         i915_emit_arith(p, A0_EXP, tmp, A0_DEST_CHANNEL_Y, 0,
                         swizzle(tmp, Y, Y, Y, Y), 0, 0);

         i915_emit_arith(p, A0_CMP,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
                         negate(swizzle(tmp, ONE, ONE, X, ONE), 0, 0, 1, 0),
                         swizzle(tmp, ONE, X, ZERO, ONE),
                         swizzle(tmp, ONE, X, Y, ONE));

         break;

      case OPCODE_LRP:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         src1 = src_vector(p, &inst->SrcReg[1], program);
         src2 = src_vector(p, &inst->SrcReg[2], program);
         flags = get_result_flags(inst);
         tmp = i915_get_utemp(p);

         /* b*a + c*(1-a)
          *
          * b*a + c - ca 
          *
          * tmp = b*a + c, 
          * result = (-c)*a + tmp 
          */
         i915_emit_arith(p, A0_MAD, tmp,
                         flags & A0_DEST_CHANNEL_ALL, 0, src1, src0, src2);

         i915_emit_arith(p, A0_MAD,
                         get_result_vector(p, inst),
                         flags, 0, negate(src2, 1, 1, 1, 1), src0, tmp);
         break;

      case OPCODE_MAD:
         EMIT_3ARG_ARITH(A0_MAD);
         break;

      case OPCODE_MAX:
         EMIT_2ARG_ARITH(A0_MAX);
         break;

      case OPCODE_MIN:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         src1 = src_vector(p, &inst->SrcReg[1], program);
         tmp = i915_get_utemp(p);
         flags = get_result_flags(inst);

         i915_emit_arith(p,
                         A0_MAX,
                         tmp, flags & A0_DEST_CHANNEL_ALL, 0,
                         negate(src0, 1, 1, 1, 1),
                         negate(src1, 1, 1, 1, 1), 0);

         i915_emit_arith(p,
                         A0_MOV,
                         get_result_vector(p, inst),
                         flags, 0, negate(tmp, 1, 1, 1, 1), 0, 0);
         break;

      case OPCODE_MOV:
         EMIT_1ARG_ARITH(A0_MOV);
         break;

      case OPCODE_MUL:
         EMIT_2ARG_ARITH(A0_MUL);
         break;

      case OPCODE_POW:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         src1 = src_vector(p, &inst->SrcReg[1], program);
         tmp = i915_get_utemp(p);
         flags = get_result_flags(inst);

         /* XXX: masking on intermediate values, here and elsewhere.
          */
         i915_emit_arith(p,
                         A0_LOG,
                         tmp, A0_DEST_CHANNEL_X, 0,
                         swizzle(src0, X, X, X, X), 0, 0);

         i915_emit_arith(p, A0_MUL, tmp, A0_DEST_CHANNEL_X, 0, tmp, src1, 0);


         i915_emit_arith(p,
                         A0_EXP,
                         get_result_vector(p, inst),
                         flags, 0, swizzle(tmp, X, X, X, X), 0, 0);

         break;

      case OPCODE_RCP:
         src0 = src_vector(p, &inst->SrcReg[0], program);

         i915_emit_arith(p,
                         A0_RCP,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
                         swizzle(src0, X, X, X, X), 0, 0);
         break;

      case OPCODE_RSQ:

         src0 = src_vector(p, &inst->SrcReg[0], program);

         i915_emit_arith(p,
                         A0_RSQ,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
                         swizzle(src0, X, X, X, X), 0, 0);
         break;

      case OPCODE_SCS:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         tmp = i915_get_utemp(p);

         /* 
          * t0.xy = MUL x.xx11, x.x1111  ; x^2, x, 1, 1
          * t0 = MUL t0.xyxy t0.xx11 ; x^4, x^3, x^2, x
          * t1 = MUL t0.xyyw t0.yz11    ; x^7 x^5 x^3 x
          * scs.x = DP4 t1, sin_constants
          * t1 = MUL t0.xxz1 t0.z111    ; x^6 x^4 x^2 1
          * scs.y = DP4 t1, cos_constants
          */
         i915_emit_arith(p,
                         A0_MUL,
                         tmp, A0_DEST_CHANNEL_XY, 0,
                         swizzle(src0, X, X, ONE, ONE),
                         swizzle(src0, X, ONE, ONE, ONE), 0);

         i915_emit_arith(p,
                         A0_MUL,
                         tmp, A0_DEST_CHANNEL_ALL, 0,
                         swizzle(tmp, X, Y, X, Y),
                         swizzle(tmp, X, X, ONE, ONE), 0);

         if (inst->DstReg.WriteMask & WRITEMASK_Y) {
            GLuint tmp1;

            if (inst->DstReg.WriteMask & WRITEMASK_X)
               tmp1 = i915_get_utemp(p);
            else
               tmp1 = tmp;

            i915_emit_arith(p,
                            A0_MUL,
                            tmp1, A0_DEST_CHANNEL_ALL, 0,
                            swizzle(tmp, X, Y, Y, W),
                            swizzle(tmp, X, Z, ONE, ONE), 0);

            i915_emit_arith(p,
                            A0_DP4,
                            get_result_vector(p, inst),
                            A0_DEST_CHANNEL_Y, 0,
                            swizzle(tmp1, W, Z, Y, X),
                            i915_emit_const4fv(p, sin_constants), 0);
         }

         if (inst->DstReg.WriteMask & WRITEMASK_X) {
            i915_emit_arith(p,
                            A0_MUL,
                            tmp, A0_DEST_CHANNEL_XYZ, 0,
                            swizzle(tmp, X, X, Z, ONE),
                            swizzle(tmp, Z, ONE, ONE, ONE), 0);

            i915_emit_arith(p,
                            A0_DP4,
                            get_result_vector(p, inst),
                            A0_DEST_CHANNEL_X, 0,
                            swizzle(tmp, ONE, Z, Y, X),
                            i915_emit_const4fv(p, cos_constants), 0);
         }
         break;

      case OPCODE_SGE:
         EMIT_2ARG_ARITH(A0_SGE);
         break;

      case OPCODE_SIN:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         tmp = i915_get_utemp(p);

         i915_emit_arith(p,
                         A0_MUL,
                         tmp, A0_DEST_CHANNEL_X, 0,
                         src0, i915_emit_const1f(p, 1.0 / (M_PI * 2)), 0);

         i915_emit_arith(p, A0_MOD, tmp, A0_DEST_CHANNEL_X, 0, tmp, 0, 0);

         /* By choosing different taylor constants, could get rid of this mul:
          */
         i915_emit_arith(p,
                         A0_MUL,
                         tmp, A0_DEST_CHANNEL_X, 0,
                         tmp, i915_emit_const1f(p, (M_PI * 2)), 0);

         /* 
          * t0.xy = MUL x.xx11, x.x1111  ; x^2, x, 1, 1
          * t0 = MUL t0.xyxy t0.xx11 ; x^4, x^3, x^2, x
          * t1 = MUL t0.xyyw t0.yz11    ; x^7 x^5 x^3 x
          * result = DP4 t1.wzyx, sin_constants
          */
         i915_emit_arith(p,
                         A0_MUL,
                         tmp, A0_DEST_CHANNEL_XY, 0,
                         swizzle(tmp, X, X, ONE, ONE),
                         swizzle(tmp, X, ONE, ONE, ONE), 0);

         i915_emit_arith(p,
                         A0_MUL,
                         tmp, A0_DEST_CHANNEL_ALL, 0,
                         swizzle(tmp, X, Y, X, Y),
                         swizzle(tmp, X, X, ONE, ONE), 0);

         i915_emit_arith(p,
                         A0_MUL,
                         tmp, A0_DEST_CHANNEL_ALL, 0,
                         swizzle(tmp, X, Y, Y, W),
                         swizzle(tmp, X, Z, ONE, ONE), 0);

         i915_emit_arith(p,
                         A0_DP4,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
                         swizzle(tmp, W, Z, Y, X),
                         i915_emit_const4fv(p, sin_constants), 0);
         break;

      case OPCODE_SLT:
         EMIT_2ARG_ARITH(A0_SLT);
         break;

      case OPCODE_SUB:
         src0 = src_vector(p, &inst->SrcReg[0], program);
         src1 = src_vector(p, &inst->SrcReg[1], program);

         i915_emit_arith(p,
                         A0_ADD,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
                         src0, negate(src1, 1, 1, 1, 1), 0);
         break;

      case OPCODE_SWZ:
         EMIT_1ARG_ARITH(A0_MOV);       /* extended swizzle handled natively */
         break;

      case OPCODE_TEX:
         EMIT_TEX(T0_TEXLD);
         break;

      case OPCODE_TXB:
         EMIT_TEX(T0_TEXLDB);
         break;

      case OPCODE_TXP:
         EMIT_TEX(T0_TEXLDP);
         break;

      case OPCODE_XPD:
         /* Cross product:
          *      result.x = src0.y * src1.z - src0.z * src1.y;
          *      result.y = src0.z * src1.x - src0.x * src1.z;
          *      result.z = src0.x * src1.y - src0.y * src1.x;
          *      result.w = undef;
          */
         src0 = src_vector(p, &inst->SrcReg[0], program);
         src1 = src_vector(p, &inst->SrcReg[1], program);
         tmp = i915_get_utemp(p);

         i915_emit_arith(p,
                         A0_MUL,
                         tmp, A0_DEST_CHANNEL_ALL, 0,
                         swizzle(src0, Z, X, Y, ONE),
                         swizzle(src1, Y, Z, X, ONE), 0);

         i915_emit_arith(p,
                         A0_MAD,
                         get_result_vector(p, inst),
                         get_result_flags(inst), 0,
                         swizzle(src0, Y, Z, X, ONE),
                         swizzle(src1, Z, X, Y, ONE),
                         negate(tmp, 1, 1, 1, 0));
         break;

      case OPCODE_END:
         return;

      default:
         i915_program_error(p, "bad opcode");
         return;
      }

      inst++;
      i915_release_utemps(p);
   }
}

/* Rather than trying to intercept and jiggle depth writes during
 * emit, just move the value into its correct position at the end of
 * the program:
 */
static void
fixup_depth_write(struct i915_fragment_program *p)
{
   if (p->depth_written) {
      GLuint depth = UREG(REG_TYPE_OD, 0);

      i915_emit_arith(p,
                      A0_MOV,
                      depth, A0_DEST_CHANNEL_W, 0,
                      swizzle(depth, X, Y, Z, Z), 0, 0);
   }
}


static void
check_wpos(struct i915_fragment_program *p)
{
   GLuint inputs = p->FragProg.Base.InputsRead;
   GLint i;

   p->wpos_tex = -1;

   for (i = 0; i < p->ctx->Const.MaxTextureCoordUnits; i++) {
      if (inputs & FRAG_BIT_TEX(i))
         continue;
      else if (inputs & FRAG_BIT_WPOS) {
         p->wpos_tex = i;
         inputs &= ~FRAG_BIT_WPOS;
      }
   }

   if (inputs & FRAG_BIT_WPOS) {
      i915_program_error(p, "No free texcoord for wpos value");
   }
}


static void
translate_program(struct i915_fragment_program *p)
{
   struct i915_context *i915 = I915_CONTEXT(p->ctx);

   i915_init_program(i915, p);
   check_wpos(p);
   upload_program(p);
   fixup_depth_write(p);
   i915_fini_program(p);

   p->translated = 1;
}


static void
track_params(struct i915_fragment_program *p)
{
   GLint i;

   if (p->nr_params)
      _mesa_load_state_parameters(p->ctx, p->FragProg.Base.Parameters);

   for (i = 0; i < p->nr_params; i++) {
      GLint reg = p->param[i].reg;
      COPY_4V(p->constant[reg], p->param[i].values);
   }

   p->params_uptodate = 1;
   p->on_hardware = 0;          /* overkill */
}


static void
i915BindProgram(GLcontext * ctx, GLenum target, struct gl_program *prog)
{
   if (target == GL_FRAGMENT_PROGRAM_ARB) {
      struct i915_context *i915 = I915_CONTEXT(ctx);
      struct i915_fragment_program *p = (struct i915_fragment_program *) prog;

      if (i915->current_program == p)
         return;

      if (i915->current_program) {
         i915->current_program->on_hardware = 0;
         i915->current_program->params_uptodate = 0;
      }

      i915->current_program = p;

      assert(p->on_hardware == 0);
      assert(p->params_uptodate == 0);

      /* Hack: make sure fog is correctly enabled according to this
       * fragment program's fog options.
       */
      ctx->Driver.Enable(ctx, GL_FRAGMENT_PROGRAM_ARB,
                         ctx->FragmentProgram.Enabled);
   }
}

static struct gl_program *
i915NewProgram(GLcontext * ctx, GLenum target, GLuint id)
{
   switch (target) {
   case GL_VERTEX_PROGRAM_ARB:
      return _mesa_init_vertex_program(ctx, CALLOC_STRUCT(gl_vertex_program),
                                       target, id);

   case GL_FRAGMENT_PROGRAM_ARB:{
         struct i915_fragment_program *prog =
            CALLOC_STRUCT(i915_fragment_program);
         if (prog) {
            i915_init_program(I915_CONTEXT(ctx), prog);

            return _mesa_init_fragment_program(ctx, &prog->FragProg,
                                               target, id);
         }
         else
            return NULL;
      }

   default:
      /* Just fallback:
       */
      return _mesa_new_program(ctx, target, id);
   }
}

static void
i915DeleteProgram(GLcontext * ctx, struct gl_program *prog)
{
   if (prog->Target == GL_FRAGMENT_PROGRAM_ARB) {
      struct i915_context *i915 = I915_CONTEXT(ctx);
      struct i915_fragment_program *p = (struct i915_fragment_program *) prog;

      if (i915->current_program == p)
         i915->current_program = 0;
   }

   _mesa_delete_program(ctx, prog);
}


static GLboolean
i915IsProgramNative(GLcontext * ctx, GLenum target, struct gl_program *prog)
{
   if (target == GL_FRAGMENT_PROGRAM_ARB) {
      struct i915_fragment_program *p = (struct i915_fragment_program *) prog;

      if (!p->translated)
         translate_program(p);

      return !p->error;
   }
   else
      return GL_TRUE;
}

static void
i915ProgramStringNotify(GLcontext * ctx,
                        GLenum target, struct gl_program *prog)
{
   if (target == GL_FRAGMENT_PROGRAM_ARB) {
      struct i915_fragment_program *p = (struct i915_fragment_program *) prog;
      p->translated = 0;

      /* Hack: make sure fog is correctly enabled according to this
       * fragment program's fog options.
       */
      ctx->Driver.Enable(ctx, GL_FRAGMENT_PROGRAM_ARB,
                         ctx->FragmentProgram.Enabled);

      if (p->FragProg.FogOption) {
         /* add extra instructions to do fog, then turn off FogOption field */
         _mesa_append_fog_code(ctx, &p->FragProg);
         p->FragProg.FogOption = GL_NONE;
      }
   }

   _tnl_program_string(ctx, target, prog);
}


void
i915ValidateFragmentProgram(struct i915_context *i915)
{
   GLcontext *ctx = &i915->intel.ctx;
   struct intel_context *intel = intel_context(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;

   struct i915_fragment_program *p =
      (struct i915_fragment_program *) ctx->FragmentProgram._Current;

   const GLuint inputsRead = p->FragProg.Base.InputsRead;
   GLuint s4 = i915->state.Ctx[I915_CTXREG_LIS4] & ~S4_VFMT_MASK;
   GLuint s2 = S2_TEXCOORD_NONE;
   int i, offset = 0;

   if (i915->current_program != p) {
      if (i915->current_program) {
         i915->current_program->on_hardware = 0;
         i915->current_program->params_uptodate = 0;
      }

      i915->current_program = p;
   }


   /* Important:
    */
   VB->AttribPtr[VERT_ATTRIB_POS] = VB->NdcPtr;

   if (!p->translated)
      translate_program(p);

   intel->vertex_attr_count = 0;
   intel->wpos_offset = 0;
   intel->wpos_size = 0;
   intel->coloroffset = 0;
   intel->specoffset = 0;

   if (inputsRead & FRAG_BITS_TEX_ANY) {
      EMIT_ATTR(_TNL_ATTRIB_POS, EMIT_4F_VIEWPORT, S4_VFMT_XYZW, 16);
   }
   else {
      EMIT_ATTR(_TNL_ATTRIB_POS, EMIT_3F_VIEWPORT, S4_VFMT_XYZ, 12);
   }

   if (inputsRead & FRAG_BIT_COL0) {
      intel->coloroffset = offset / 4;
      EMIT_ATTR(_TNL_ATTRIB_COLOR0, EMIT_4UB_4F_BGRA, S4_VFMT_COLOR, 4);
   }

   if ((inputsRead & (FRAG_BIT_COL1 | FRAG_BIT_FOGC)) ||
       i915->vertex_fog != I915_FOG_NONE) {

      if (inputsRead & FRAG_BIT_COL1) {
         intel->specoffset = offset / 4;
         EMIT_ATTR(_TNL_ATTRIB_COLOR1, EMIT_3UB_3F_BGR, S4_VFMT_SPEC_FOG, 3);
      }
      else
         EMIT_PAD(3);

      if ((inputsRead & FRAG_BIT_FOGC) || i915->vertex_fog != I915_FOG_NONE)
         EMIT_ATTR(_TNL_ATTRIB_FOG, EMIT_1UB_1F, S4_VFMT_SPEC_FOG, 1);
      else
         EMIT_PAD(1);
   }

   /* XXX this was disabled, but enabling this code helped fix the Glean
    * tfragprog1 fog tests.
    */
#if 1
   if ((inputsRead & FRAG_BIT_FOGC) || i915->vertex_fog != I915_FOG_NONE) {
      EMIT_ATTR(_TNL_ATTRIB_FOG, EMIT_1F, S4_VFMT_FOG_PARAM, 4);
   }
#endif

   for (i = 0; i < p->ctx->Const.MaxTextureCoordUnits; i++) {
      if (inputsRead & FRAG_BIT_TEX(i)) {
         int sz = VB->TexCoordPtr[i]->size;

         s2 &= ~S2_TEXCOORD_FMT(i, S2_TEXCOORD_FMT0_MASK);
         s2 |= S2_TEXCOORD_FMT(i, SZ_TO_HW(sz));

         EMIT_ATTR(_TNL_ATTRIB_TEX0 + i, EMIT_SZ(sz), 0, sz * 4);
      }
      else if (i == p->wpos_tex) {

         /* If WPOS is required, duplicate the XYZ position data in an
          * unused texture coordinate:
          */
         s2 &= ~S2_TEXCOORD_FMT(i, S2_TEXCOORD_FMT0_MASK);
         s2 |= S2_TEXCOORD_FMT(i, SZ_TO_HW(3));

         intel->wpos_offset = offset;
         intel->wpos_size = 3 * sizeof(GLuint);

         EMIT_PAD(intel->wpos_size);
      }
   }

   if (s2 != i915->state.Ctx[I915_CTXREG_LIS2] ||
       s4 != i915->state.Ctx[I915_CTXREG_LIS4]) {
      int k;

      I915_STATECHANGE(i915, I915_UPLOAD_CTX);

      /* Must do this *after* statechange, so as not to affect
       * buffered vertices reliant on the old state:
       */
      intel->vertex_size = _tnl_install_attrs(&intel->ctx,
                                              intel->vertex_attrs,
                                              intel->vertex_attr_count,
                                              intel->ViewportMatrix.m, 0);

      intel->vertex_size >>= 2;

      i915->state.Ctx[I915_CTXREG_LIS2] = s2;
      i915->state.Ctx[I915_CTXREG_LIS4] = s4;

      k = intel->vtbl.check_vertex_size(intel, intel->vertex_size);
      assert(k);
   }

   if (!p->params_uptodate)
      track_params(p);

   if (!p->on_hardware)
      i915_upload_program(i915, p);
}

void
i915InitFragProgFuncs(struct dd_function_table *functions)
{
   functions->BindProgram = i915BindProgram;
   functions->NewProgram = i915NewProgram;
   functions->DeleteProgram = i915DeleteProgram;
   functions->IsProgramNative = i915IsProgramNative;
   functions->ProgramStringNotify = i915ProgramStringNotify;
}
