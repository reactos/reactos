/*
 * Mesa 3-D graphics library
 * Version:  7.3
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file prog_execute.c
 * Software interpreter for vertex/fragment programs.
 * \author Brian Paul
 */

/*
 * NOTE: we do everything in single-precision floating point; we don't
 * currently observe the single/half/fixed-precision qualifiers.
 *
 */


#include "main/glheader.h"
#include "main/colormac.h"
#include "main/context.h"
#include "program.h"
#include "prog_execute.h"
#include "prog_instruction.h"
#include "prog_parameter.h"
#include "prog_print.h"
#include "prog_noise.h"


/* debug predicate */
#define DEBUG_PROG 0


/**
 * Set x to positive or negative infinity.
 */
#if defined(USE_IEEE) || defined(_WIN32)
#define SET_POS_INFINITY(x)  ( *((GLuint *) (void *)&x) = 0x7F800000 )
#define SET_NEG_INFINITY(x)  ( *((GLuint *) (void *)&x) = 0xFF800000 )
#elif defined(VMS)
#define SET_POS_INFINITY(x)  x = __MAXFLOAT
#define SET_NEG_INFINITY(x)  x = -__MAXFLOAT
#else
#define SET_POS_INFINITY(x)  x = (GLfloat) HUGE_VAL
#define SET_NEG_INFINITY(x)  x = (GLfloat) -HUGE_VAL
#endif

#define SET_FLOAT_BITS(x, bits) ((fi_type *) (void *) &(x))->i = bits


static const GLfloat ZeroVec[4] = { 0.0F, 0.0F, 0.0F, 0.0F };



/**
 * Return a pointer to the 4-element float vector specified by the given
 * source register.
 */
static INLINE const GLfloat *
get_src_register_pointer(const struct prog_src_register *source,
                         const struct gl_program_machine *machine)
{
   const struct gl_program *prog = machine->CurProgram;
   GLint reg = source->Index;

   if (source->RelAddr) {
      /* add address register value to src index/offset */
      reg += machine->AddressReg[0][0];
      if (reg < 0) {
         return ZeroVec;
      }
   }

   switch (source->File) {
   case PROGRAM_TEMPORARY:
      if (reg >= MAX_PROGRAM_TEMPS)
         return ZeroVec;
      return machine->Temporaries[reg];

   case PROGRAM_INPUT:
      if (prog->Target == GL_VERTEX_PROGRAM_ARB) {
         if (reg >= VERT_ATTRIB_MAX)
            return ZeroVec;
         return machine->VertAttribs[reg];
      }
      else {
         if (reg >= FRAG_ATTRIB_MAX)
            return ZeroVec;
         return machine->Attribs[reg][machine->CurElement];
      }

   case PROGRAM_OUTPUT:
      if (reg >= MAX_PROGRAM_OUTPUTS)
         return ZeroVec;
      return machine->Outputs[reg];

   case PROGRAM_LOCAL_PARAM:
      if (reg >= MAX_PROGRAM_LOCAL_PARAMS)
         return ZeroVec;
      return machine->CurProgram->LocalParams[reg];

   case PROGRAM_ENV_PARAM:
      if (reg >= MAX_PROGRAM_ENV_PARAMS)
         return ZeroVec;
      return machine->EnvParams[reg];

   case PROGRAM_STATE_VAR:
      /* Fallthrough */
   case PROGRAM_CONSTANT:
      /* Fallthrough */
   case PROGRAM_UNIFORM:
      /* Fallthrough */
   case PROGRAM_NAMED_PARAM:
      if (reg >= (GLint) prog->Parameters->NumParameters)
         return ZeroVec;
      return prog->Parameters->ParameterValues[reg];

   default:
      _mesa_problem(NULL,
         "Invalid src register file %d in get_src_register_pointer()",
         source->File);
      return NULL;
   }
}


/**
 * Return a pointer to the 4-element float vector specified by the given
 * destination register.
 */
static INLINE GLfloat *
get_dst_register_pointer(const struct prog_dst_register *dest,
                         struct gl_program_machine *machine)
{
   static GLfloat dummyReg[4];
   GLint reg = dest->Index;

   if (dest->RelAddr) {
      /* add address register value to src index/offset */
      reg += machine->AddressReg[0][0];
      if (reg < 0) {
         return dummyReg;
      }
   }

   switch (dest->File) {
   case PROGRAM_TEMPORARY:
      if (reg >= MAX_PROGRAM_TEMPS)
         return dummyReg;
      return machine->Temporaries[reg];

   case PROGRAM_OUTPUT:
      if (reg >= MAX_PROGRAM_OUTPUTS)
         return dummyReg;
      return machine->Outputs[reg];

   case PROGRAM_WRITE_ONLY:
      return dummyReg;

   default:
      _mesa_problem(NULL,
         "Invalid dest register file %d in get_dst_register_pointer()",
         dest->File);
      return NULL;
   }
}



#if FEATURE_MESA_program_debug
static struct gl_program_machine *CurrentMachine = NULL;

/**
 * For GL_MESA_program_debug.
 * Return current value (4*GLfloat) of a program register.
 * Called via ctx->Driver.GetProgramRegister().
 */
void
_mesa_get_program_register(GLcontext *ctx, enum register_file file,
                           GLuint index, GLfloat val[4])
{
   if (CurrentMachine) {
      struct prog_src_register srcReg;
      const GLfloat *src;
      srcReg.File = file;
      srcReg.Index = index;
      src = get_src_register_pointer(&srcReg, CurrentMachine);
      COPY_4V(val, src);
   }
}
#endif /* FEATURE_MESA_program_debug */


/**
 * Fetch a 4-element float vector from the given source register.
 * Apply swizzling and negating as needed.
 */
static void
fetch_vector4(const struct prog_src_register *source,
              const struct gl_program_machine *machine, GLfloat result[4])
{
   const GLfloat *src = get_src_register_pointer(source, machine);
   ASSERT(src);

   if (source->Swizzle == SWIZZLE_NOOP) {
      /* no swizzling */
      COPY_4V(result, src);
   }
   else {
      ASSERT(GET_SWZ(source->Swizzle, 0) <= 3);
      ASSERT(GET_SWZ(source->Swizzle, 1) <= 3);
      ASSERT(GET_SWZ(source->Swizzle, 2) <= 3);
      ASSERT(GET_SWZ(source->Swizzle, 3) <= 3);
      result[0] = src[GET_SWZ(source->Swizzle, 0)];
      result[1] = src[GET_SWZ(source->Swizzle, 1)];
      result[2] = src[GET_SWZ(source->Swizzle, 2)];
      result[3] = src[GET_SWZ(source->Swizzle, 3)];
   }

   if (source->NegateBase) {
      result[0] = -result[0];
      result[1] = -result[1];
      result[2] = -result[2];
      result[3] = -result[3];
   }
   if (source->Abs) {
      result[0] = FABSF(result[0]);
      result[1] = FABSF(result[1]);
      result[2] = FABSF(result[2]);
      result[3] = FABSF(result[3]);
   }
   if (source->NegateAbs) {
      result[0] = -result[0];
      result[1] = -result[1];
      result[2] = -result[2];
      result[3] = -result[3];
   }
}


/**
 * Fetch a 4-element uint vector from the given source register.
 * Apply swizzling but not negation/abs.
 */
static void
fetch_vector4ui(const struct prog_src_register *source,
                const struct gl_program_machine *machine, GLuint result[4])
{
   const GLuint *src = (GLuint *) get_src_register_pointer(source, machine);
   ASSERT(src);

   if (source->Swizzle == SWIZZLE_NOOP) {
      /* no swizzling */
      COPY_4V(result, src);
   }
   else {
      ASSERT(GET_SWZ(source->Swizzle, 0) <= 3);
      ASSERT(GET_SWZ(source->Swizzle, 1) <= 3);
      ASSERT(GET_SWZ(source->Swizzle, 2) <= 3);
      ASSERT(GET_SWZ(source->Swizzle, 3) <= 3);
      result[0] = src[GET_SWZ(source->Swizzle, 0)];
      result[1] = src[GET_SWZ(source->Swizzle, 1)];
      result[2] = src[GET_SWZ(source->Swizzle, 2)];
      result[3] = src[GET_SWZ(source->Swizzle, 3)];
   }

   /* Note: no NegateBase, Abs, NegateAbs here */
}



/**
 * Fetch the derivative with respect to X or Y for the given register.
 * XXX this currently only works for fragment program input attribs.
 */
static void
fetch_vector4_deriv(GLcontext * ctx,
                    const struct prog_src_register *source,
                    const struct gl_program_machine *machine,
                    char xOrY, GLfloat result[4])
{
   if (source->File == PROGRAM_INPUT &&
       source->Index < (GLint) machine->NumDeriv) {
      const GLint col = machine->CurElement;
      const GLfloat w = machine->Attribs[FRAG_ATTRIB_WPOS][col][3];
      const GLfloat invQ = 1.0f / w;
      GLfloat deriv[4];

      if (xOrY == 'X') {
         deriv[0] = machine->DerivX[source->Index][0] * invQ;
         deriv[1] = machine->DerivX[source->Index][1] * invQ;
         deriv[2] = machine->DerivX[source->Index][2] * invQ;
         deriv[3] = machine->DerivX[source->Index][3] * invQ;
      }
      else {
         deriv[0] = machine->DerivY[source->Index][0] * invQ;
         deriv[1] = machine->DerivY[source->Index][1] * invQ;
         deriv[2] = machine->DerivY[source->Index][2] * invQ;
         deriv[3] = machine->DerivY[source->Index][3] * invQ;
      }

      result[0] = deriv[GET_SWZ(source->Swizzle, 0)];
      result[1] = deriv[GET_SWZ(source->Swizzle, 1)];
      result[2] = deriv[GET_SWZ(source->Swizzle, 2)];
      result[3] = deriv[GET_SWZ(source->Swizzle, 3)];
      
      if (source->NegateBase) {
         result[0] = -result[0];
         result[1] = -result[1];
         result[2] = -result[2];
         result[3] = -result[3];
      }
      if (source->Abs) {
         result[0] = FABSF(result[0]);
         result[1] = FABSF(result[1]);
         result[2] = FABSF(result[2]);
         result[3] = FABSF(result[3]);
      }
      if (source->NegateAbs) {
         result[0] = -result[0];
         result[1] = -result[1];
         result[2] = -result[2];
         result[3] = -result[3];
      }
   }
   else {
      ASSIGN_4V(result, 0.0, 0.0, 0.0, 0.0);
   }
}


/**
 * As above, but only return result[0] element.
 */
static void
fetch_vector1(const struct prog_src_register *source,
              const struct gl_program_machine *machine, GLfloat result[4])
{
   const GLfloat *src = get_src_register_pointer(source, machine);
   ASSERT(src);

   result[0] = src[GET_SWZ(source->Swizzle, 0)];

   if (source->NegateBase) {
      result[0] = -result[0];
   }
   if (source->Abs) {
      result[0] = FABSF(result[0]);
   }
   if (source->NegateAbs) {
      result[0] = -result[0];
   }
}


/**
 * Fetch texel from texture.  Use partial derivatives when possible.
 */
static INLINE void
fetch_texel(GLcontext *ctx,
            const struct gl_program_machine *machine,
            const struct prog_instruction *inst,
            const GLfloat texcoord[4], GLfloat lodBias,
            GLfloat color[4])
{
   const GLuint unit = machine->Samplers[inst->TexSrcUnit];

   /* Note: we only have the right derivatives for fragment input attribs.
    */
   if (machine->NumDeriv > 0 &&
       inst->SrcReg[0].File == PROGRAM_INPUT &&
       inst->SrcReg[0].Index == FRAG_ATTRIB_TEX0 + inst->TexSrcUnit) {
      /* simple texture fetch for which we should have derivatives */
      GLuint attr = inst->SrcReg[0].Index;
      machine->FetchTexelDeriv(ctx, texcoord,
                               machine->DerivX[attr],
                               machine->DerivY[attr],
                               lodBias, unit, color);
   }
   else {
      machine->FetchTexelLod(ctx, texcoord, lodBias, unit, color);
   }
}


/**
 * Test value against zero and return GT, LT, EQ or UN if NaN.
 */
static INLINE GLuint
generate_cc(float value)
{
   if (value != value)
      return COND_UN;           /* NaN */
   if (value > 0.0F)
      return COND_GT;
   if (value < 0.0F)
      return COND_LT;
   return COND_EQ;
}


/**
 * Test if the ccMaskRule is satisfied by the given condition code.
 * Used to mask destination writes according to the current condition code.
 */
static INLINE GLboolean
test_cc(GLuint condCode, GLuint ccMaskRule)
{
   switch (ccMaskRule) {
   case COND_EQ: return (condCode == COND_EQ);
   case COND_NE: return (condCode != COND_EQ);
   case COND_LT: return (condCode == COND_LT);
   case COND_GE: return (condCode == COND_GT || condCode == COND_EQ);
   case COND_LE: return (condCode == COND_LT || condCode == COND_EQ);
   case COND_GT: return (condCode == COND_GT);
   case COND_TR: return GL_TRUE;
   case COND_FL: return GL_FALSE;
   default:      return GL_TRUE;
   }
}


/**
 * Evaluate the 4 condition codes against a predicate and return GL_TRUE
 * or GL_FALSE to indicate result.
 */
static INLINE GLboolean
eval_condition(const struct gl_program_machine *machine,
               const struct prog_instruction *inst)
{
   const GLuint swizzle = inst->DstReg.CondSwizzle;
   const GLuint condMask = inst->DstReg.CondMask;
   if (test_cc(machine->CondCodes[GET_SWZ(swizzle, 0)], condMask) ||
       test_cc(machine->CondCodes[GET_SWZ(swizzle, 1)], condMask) ||
       test_cc(machine->CondCodes[GET_SWZ(swizzle, 2)], condMask) ||
       test_cc(machine->CondCodes[GET_SWZ(swizzle, 3)], condMask)) {
      return GL_TRUE;
   }
   else {
      return GL_FALSE;
   }
}



/**
 * Store 4 floats into a register.  Observe the instructions saturate and
 * set-condition-code flags.
 */
static void
store_vector4(const struct prog_instruction *inst,
              struct gl_program_machine *machine, const GLfloat value[4])
{
   const struct prog_dst_register *dstReg = &(inst->DstReg);
   const GLboolean clamp = inst->SaturateMode == SATURATE_ZERO_ONE;
   GLuint writeMask = dstReg->WriteMask;
   GLfloat clampedValue[4];
   GLfloat *dst = get_dst_register_pointer(dstReg, machine);

#if 0
   if (value[0] > 1.0e10 ||
       IS_INF_OR_NAN(value[0]) ||
       IS_INF_OR_NAN(value[1]) ||
       IS_INF_OR_NAN(value[2]) || IS_INF_OR_NAN(value[3]))
      printf("store %g %g %g %g\n", value[0], value[1], value[2], value[3]);
#endif

   if (clamp) {
      clampedValue[0] = CLAMP(value[0], 0.0F, 1.0F);
      clampedValue[1] = CLAMP(value[1], 0.0F, 1.0F);
      clampedValue[2] = CLAMP(value[2], 0.0F, 1.0F);
      clampedValue[3] = CLAMP(value[3], 0.0F, 1.0F);
      value = clampedValue;
   }

   if (dstReg->CondMask != COND_TR) {
      /* condition codes may turn off some writes */
      if (writeMask & WRITEMASK_X) {
         if (!test_cc(machine->CondCodes[GET_SWZ(dstReg->CondSwizzle, 0)],
                      dstReg->CondMask))
            writeMask &= ~WRITEMASK_X;
      }
      if (writeMask & WRITEMASK_Y) {
         if (!test_cc(machine->CondCodes[GET_SWZ(dstReg->CondSwizzle, 1)],
                      dstReg->CondMask))
            writeMask &= ~WRITEMASK_Y;
      }
      if (writeMask & WRITEMASK_Z) {
         if (!test_cc(machine->CondCodes[GET_SWZ(dstReg->CondSwizzle, 2)],
                      dstReg->CondMask))
            writeMask &= ~WRITEMASK_Z;
      }
      if (writeMask & WRITEMASK_W) {
         if (!test_cc(machine->CondCodes[GET_SWZ(dstReg->CondSwizzle, 3)],
                      dstReg->CondMask))
            writeMask &= ~WRITEMASK_W;
      }
   }

   if (writeMask & WRITEMASK_X)
      dst[0] = value[0];
   if (writeMask & WRITEMASK_Y)
      dst[1] = value[1];
   if (writeMask & WRITEMASK_Z)
      dst[2] = value[2];
   if (writeMask & WRITEMASK_W)
      dst[3] = value[3];

   if (inst->CondUpdate) {
      if (writeMask & WRITEMASK_X)
         machine->CondCodes[0] = generate_cc(value[0]);
      if (writeMask & WRITEMASK_Y)
         machine->CondCodes[1] = generate_cc(value[1]);
      if (writeMask & WRITEMASK_Z)
         machine->CondCodes[2] = generate_cc(value[2]);
      if (writeMask & WRITEMASK_W)
         machine->CondCodes[3] = generate_cc(value[3]);
#if DEBUG_PROG
      printf("CondCodes=(%s,%s,%s,%s) for:\n",
             _mesa_condcode_string(machine->CondCodes[0]),
             _mesa_condcode_string(machine->CondCodes[1]),
             _mesa_condcode_string(machine->CondCodes[2]),
             _mesa_condcode_string(machine->CondCodes[3]));
#endif
   }
}


/**
 * Store 4 uints into a register.  Observe the set-condition-code flags.
 */
static void
store_vector4ui(const struct prog_instruction *inst,
                struct gl_program_machine *machine, const GLuint value[4])
{
   const struct prog_dst_register *dstReg = &(inst->DstReg);
   GLuint writeMask = dstReg->WriteMask;
   GLuint *dst = (GLuint *) get_dst_register_pointer(dstReg, machine);

   if (dstReg->CondMask != COND_TR) {
      /* condition codes may turn off some writes */
      if (writeMask & WRITEMASK_X) {
         if (!test_cc(machine->CondCodes[GET_SWZ(dstReg->CondSwizzle, 0)],
                      dstReg->CondMask))
            writeMask &= ~WRITEMASK_X;
      }
      if (writeMask & WRITEMASK_Y) {
         if (!test_cc(machine->CondCodes[GET_SWZ(dstReg->CondSwizzle, 1)],
                      dstReg->CondMask))
            writeMask &= ~WRITEMASK_Y;
      }
      if (writeMask & WRITEMASK_Z) {
         if (!test_cc(machine->CondCodes[GET_SWZ(dstReg->CondSwizzle, 2)],
                      dstReg->CondMask))
            writeMask &= ~WRITEMASK_Z;
      }
      if (writeMask & WRITEMASK_W) {
         if (!test_cc(machine->CondCodes[GET_SWZ(dstReg->CondSwizzle, 3)],
                      dstReg->CondMask))
            writeMask &= ~WRITEMASK_W;
      }
   }

   if (writeMask & WRITEMASK_X)
      dst[0] = value[0];
   if (writeMask & WRITEMASK_Y)
      dst[1] = value[1];
   if (writeMask & WRITEMASK_Z)
      dst[2] = value[2];
   if (writeMask & WRITEMASK_W)
      dst[3] = value[3];

   if (inst->CondUpdate) {
      if (writeMask & WRITEMASK_X)
         machine->CondCodes[0] = generate_cc(value[0]);
      if (writeMask & WRITEMASK_Y)
         machine->CondCodes[1] = generate_cc(value[1]);
      if (writeMask & WRITEMASK_Z)
         machine->CondCodes[2] = generate_cc(value[2]);
      if (writeMask & WRITEMASK_W)
         machine->CondCodes[3] = generate_cc(value[3]);
#if DEBUG_PROG
      printf("CondCodes=(%s,%s,%s,%s) for:\n",
             _mesa_condcode_string(machine->CondCodes[0]),
             _mesa_condcode_string(machine->CondCodes[1]),
             _mesa_condcode_string(machine->CondCodes[2]),
             _mesa_condcode_string(machine->CondCodes[3]));
#endif
   }
}



/**
 * Execute the given vertex/fragment program.
 *
 * \param ctx  rendering context
 * \param program  the program to execute
 * \param machine  machine state (must be initialized)
 * \return GL_TRUE if program completed or GL_FALSE if program executed KIL.
 */
GLboolean
_mesa_execute_program(GLcontext * ctx,
                      const struct gl_program *program,
                      struct gl_program_machine *machine)
{
   const GLuint numInst = program->NumInstructions;
   const GLuint maxExec = 10000;
   GLuint pc, numExec = 0;

   machine->CurProgram = program;

   if (DEBUG_PROG) {
      printf("execute program %u --------------------\n", program->Id);
   }

#if FEATURE_MESA_program_debug
   CurrentMachine = machine;
#endif

   if (program->Target == GL_VERTEX_PROGRAM_ARB) {
      machine->EnvParams = ctx->VertexProgram.Parameters;
   }
   else {
      machine->EnvParams = ctx->FragmentProgram.Parameters;
   }

   for (pc = 0; pc < numInst; pc++) {
      const struct prog_instruction *inst = program->Instructions + pc;

#if FEATURE_MESA_program_debug
      if (ctx->FragmentProgram.CallbackEnabled &&
          ctx->FragmentProgram.Callback) {
         ctx->FragmentProgram.CurrentPosition = inst->StringPos;
         ctx->FragmentProgram.Callback(program->Target,
                                       ctx->FragmentProgram.CallbackData);
      }
#endif

      if (DEBUG_PROG) {
         _mesa_print_instruction(inst);
      }

      switch (inst->Opcode) {
      case OPCODE_ABS:
         {
            GLfloat a[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            result[0] = FABSF(a[0]);
            result[1] = FABSF(a[1]);
            result[2] = FABSF(a[2]);
            result[3] = FABSF(a[3]);
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_ADD:
         {
            GLfloat a[4], b[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            fetch_vector4(&inst->SrcReg[1], machine, b);
            result[0] = a[0] + b[0];
            result[1] = a[1] + b[1];
            result[2] = a[2] + b[2];
            result[3] = a[3] + b[3];
            store_vector4(inst, machine, result);
            if (DEBUG_PROG) {
               printf("ADD (%g %g %g %g) = (%g %g %g %g) + (%g %g %g %g)\n",
                      result[0], result[1], result[2], result[3],
                      a[0], a[1], a[2], a[3], b[0], b[1], b[2], b[3]);
            }
         }
         break;
      case OPCODE_AND:     /* bitwise AND */
         {
            GLuint a[4], b[4], result[4];
            fetch_vector4ui(&inst->SrcReg[0], machine, a);
            fetch_vector4ui(&inst->SrcReg[1], machine, b);
            result[0] = a[0] & b[0];
            result[1] = a[1] & b[1];
            result[2] = a[2] & b[2];
            result[3] = a[3] & b[3];
            store_vector4ui(inst, machine, result);
         }
         break;
      case OPCODE_ARL:
         {
            GLfloat t[4];
            fetch_vector4(&inst->SrcReg[0], machine, t);
            machine->AddressReg[0][0] = IFLOOR(t[0]);
         }
         break;
      case OPCODE_BGNLOOP:
         /* no-op */
         break;
      case OPCODE_ENDLOOP:
         /* subtract 1 here since pc is incremented by for(pc) loop */
         pc = inst->BranchTarget - 1;   /* go to matching BNGLOOP */
         break;
      case OPCODE_BGNSUB:      /* begin subroutine */
         break;
      case OPCODE_ENDSUB:      /* end subroutine */
         break;
      case OPCODE_BRA:         /* branch (conditional) */
         /* fall-through */
      case OPCODE_BRK:         /* break out of loop (conditional) */
         /* fall-through */
      case OPCODE_CONT:        /* continue loop (conditional) */
         if (eval_condition(machine, inst)) {
            /* take branch */
            /* Subtract 1 here since we'll do pc++ at end of for-loop */
            pc = inst->BranchTarget - 1;
         }
         break;
      case OPCODE_CAL:         /* Call subroutine (conditional) */
         if (eval_condition(machine, inst)) {
            /* call the subroutine */
            if (machine->StackDepth >= MAX_PROGRAM_CALL_DEPTH) {
               return GL_TRUE;  /* Per GL_NV_vertex_program2 spec */
            }
            machine->CallStack[machine->StackDepth++] = pc + 1; /* next inst */
            /* Subtract 1 here since we'll do pc++ at end of for-loop */
            pc = inst->BranchTarget - 1;
         }
         break;
      case OPCODE_CMP:
         {
            GLfloat a[4], b[4], c[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            fetch_vector4(&inst->SrcReg[1], machine, b);
            fetch_vector4(&inst->SrcReg[2], machine, c);
            result[0] = a[0] < 0.0F ? b[0] : c[0];
            result[1] = a[1] < 0.0F ? b[1] : c[1];
            result[2] = a[2] < 0.0F ? b[2] : c[2];
            result[3] = a[3] < 0.0F ? b[3] : c[3];
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_COS:
         {
            GLfloat a[4], result[4];
            fetch_vector1(&inst->SrcReg[0], machine, a);
            result[0] = result[1] = result[2] = result[3]
               = (GLfloat) _mesa_cos(a[0]);
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_DDX:         /* Partial derivative with respect to X */
         {
            GLfloat result[4];
            fetch_vector4_deriv(ctx, &inst->SrcReg[0], machine,
                                'X', result);
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_DDY:         /* Partial derivative with respect to Y */
         {
            GLfloat result[4];
            fetch_vector4_deriv(ctx, &inst->SrcReg[0], machine,
                                'Y', result);
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_DP2:
         {
            GLfloat a[4], b[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            fetch_vector4(&inst->SrcReg[1], machine, b);
            result[0] = result[1] = result[2] = result[3] = DOT2(a, b);
            store_vector4(inst, machine, result);
            if (DEBUG_PROG) {
               printf("DP2 %g = (%g %g) . (%g %g)\n",
                      result[0], a[0], a[1], b[0], b[1]);
            }
         }
         break;
      case OPCODE_DP2A:
         {
            GLfloat a[4], b[4], c, result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            fetch_vector4(&inst->SrcReg[1], machine, b);
            fetch_vector1(&inst->SrcReg[1], machine, &c);
            result[0] = result[1] = result[2] = result[3] = DOT2(a, b) + c;
            store_vector4(inst, machine, result);
            if (DEBUG_PROG) {
               printf("DP2A %g = (%g %g) . (%g %g) + %g\n",
                      result[0], a[0], a[1], b[0], b[1], c);
            }
         }
         break;
      case OPCODE_DP3:
         {
            GLfloat a[4], b[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            fetch_vector4(&inst->SrcReg[1], machine, b);
            result[0] = result[1] = result[2] = result[3] = DOT3(a, b);
            store_vector4(inst, machine, result);
            if (DEBUG_PROG) {
               printf("DP3 %g = (%g %g %g) . (%g %g %g)\n",
                      result[0], a[0], a[1], a[2], b[0], b[1], b[2]);
            }
         }
         break;
      case OPCODE_DP4:
         {
            GLfloat a[4], b[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            fetch_vector4(&inst->SrcReg[1], machine, b);
            result[0] = result[1] = result[2] = result[3] = DOT4(a, b);
            store_vector4(inst, machine, result);
            if (DEBUG_PROG) {
               printf("DP4 %g = (%g, %g %g %g) . (%g, %g %g %g)\n",
                      result[0], a[0], a[1], a[2], a[3],
                      b[0], b[1], b[2], b[3]);
            }
         }
         break;
      case OPCODE_DPH:
         {
            GLfloat a[4], b[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            fetch_vector4(&inst->SrcReg[1], machine, b);
            result[0] = result[1] = result[2] = result[3] = DOT3(a, b) + b[3];
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_DST:         /* Distance vector */
         {
            GLfloat a[4], b[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            fetch_vector4(&inst->SrcReg[1], machine, b);
            result[0] = 1.0F;
            result[1] = a[1] * b[1];
            result[2] = a[2];
            result[3] = b[3];
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_EXP:
         {
            GLfloat t[4], q[4], floor_t0;
            fetch_vector1(&inst->SrcReg[0], machine, t);
            floor_t0 = FLOORF(t[0]);
            if (floor_t0 > FLT_MAX_EXP) {
               SET_POS_INFINITY(q[0]);
               SET_POS_INFINITY(q[2]);
            }
            else if (floor_t0 < FLT_MIN_EXP) {
               q[0] = 0.0F;
               q[2] = 0.0F;
            }
            else {
               q[0] = LDEXPF(1.0, (int) floor_t0);
               /* Note: GL_NV_vertex_program expects 
                * result.z = result.x * APPX(result.y)
                * We do what the ARB extension says.
                */
               q[2] = (GLfloat) pow(2.0, t[0]);
            }
            q[1] = t[0] - floor_t0;
            q[3] = 1.0F;
            store_vector4( inst, machine, q );
         }
         break;
      case OPCODE_EX2:         /* Exponential base 2 */
         {
            GLfloat a[4], result[4];
            fetch_vector1(&inst->SrcReg[0], machine, a);
            result[0] = result[1] = result[2] = result[3] =
               (GLfloat) _mesa_pow(2.0, a[0]);
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_FLR:
         {
            GLfloat a[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            result[0] = FLOORF(a[0]);
            result[1] = FLOORF(a[1]);
            result[2] = FLOORF(a[2]);
            result[3] = FLOORF(a[3]);
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_FRC:
         {
            GLfloat a[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            result[0] = a[0] - FLOORF(a[0]);
            result[1] = a[1] - FLOORF(a[1]);
            result[2] = a[2] - FLOORF(a[2]);
            result[3] = a[3] - FLOORF(a[3]);
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_IF:
         {
            GLboolean cond;
            /* eval condition */
            if (inst->SrcReg[0].File != PROGRAM_UNDEFINED) {
               GLfloat a[4];
               fetch_vector1(&inst->SrcReg[0], machine, a);
               cond = (a[0] != 0.0);
            }
            else {
               cond = eval_condition(machine, inst);
            }
            if (DEBUG_PROG) {
               printf("IF: %d\n", cond);
            }
            /* do if/else */
            if (cond) {
               /* do if-clause (just continue execution) */
            }
            else {
               /* go to the instruction after ELSE or ENDIF */
               assert(inst->BranchTarget >= 0);
               pc = inst->BranchTarget - 1;
            }
         }
         break;
      case OPCODE_ELSE:
         /* goto ENDIF */
         assert(inst->BranchTarget >= 0);
         pc = inst->BranchTarget - 1;
         break;
      case OPCODE_ENDIF:
         /* nothing */
         break;
      case OPCODE_KIL_NV:      /* NV_f_p only (conditional) */
         if (eval_condition(machine, inst)) {
            return GL_FALSE;
         }
         break;
      case OPCODE_KIL:         /* ARB_f_p only */
         {
            GLfloat a[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            if (a[0] < 0.0F || a[1] < 0.0F || a[2] < 0.0F || a[3] < 0.0F) {
               return GL_FALSE;
            }
         }
         break;
      case OPCODE_LG2:         /* log base 2 */
         {
            GLfloat a[4], result[4];
            fetch_vector1(&inst->SrcReg[0], machine, a);
	    /* The fast LOG2 macro doesn't meet the precision requirements.
	     */
            result[0] = result[1] = result[2] = result[3] =
		(log(a[0]) * 1.442695F);
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_LIT:
         {
            const GLfloat epsilon = 1.0F / 256.0F;      /* from NV VP spec */
            GLfloat a[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            a[0] = MAX2(a[0], 0.0F);
            a[1] = MAX2(a[1], 0.0F);
            /* XXX ARB version clamps a[3], NV version doesn't */
            a[3] = CLAMP(a[3], -(128.0F - epsilon), (128.0F - epsilon));
            result[0] = 1.0F;
            result[1] = a[0];
            /* XXX we could probably just use pow() here */
            if (a[0] > 0.0F) {
               if (a[1] == 0.0 && a[3] == 0.0)
                  result[2] = 1.0;
               else
                  result[2] = EXPF(a[3] * LOGF(a[1]));
            }
            else {
               result[2] = 0.0;
            }
            result[3] = 1.0F;
            store_vector4(inst, machine, result);
            if (DEBUG_PROG) {
               printf("LIT (%g %g %g %g) : (%g %g %g %g)\n",
                      result[0], result[1], result[2], result[3],
                      a[0], a[1], a[2], a[3]);
            }
         }
         break;
      case OPCODE_LOG:
         {
            GLfloat t[4], q[4], abs_t0;
            fetch_vector1(&inst->SrcReg[0], machine, t);
            abs_t0 = FABSF(t[0]);
            if (abs_t0 != 0.0F) {
               /* Since we really can't handle infinite values on VMS
                * like other OSes we'll use __MAXFLOAT to represent
                * infinity.  This may need some tweaking.
                */
#ifdef VMS
               if (abs_t0 == __MAXFLOAT)
#else
               if (IS_INF_OR_NAN(abs_t0))
#endif
               {
                  SET_POS_INFINITY(q[0]);
                  q[1] = 1.0F;
                  SET_POS_INFINITY(q[2]);
               }
               else {
                  int exponent;
                  GLfloat mantissa = FREXPF(t[0], &exponent);
                  q[0] = (GLfloat) (exponent - 1);
                  q[1] = (GLfloat) (2.0 * mantissa); /* map [.5, 1) -> [1, 2) */

		  /* The fast LOG2 macro doesn't meet the precision
		   * requirements.
		   */
                  q[2] = (log(t[0]) * 1.442695F);
               }
            }
            else {
               SET_NEG_INFINITY(q[0]);
               q[1] = 1.0F;
               SET_NEG_INFINITY(q[2]);
            }
            q[3] = 1.0;
            store_vector4(inst, machine, q);
         }
         break;
      case OPCODE_LRP:
         {
            GLfloat a[4], b[4], c[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            fetch_vector4(&inst->SrcReg[1], machine, b);
            fetch_vector4(&inst->SrcReg[2], machine, c);
            result[0] = a[0] * b[0] + (1.0F - a[0]) * c[0];
            result[1] = a[1] * b[1] + (1.0F - a[1]) * c[1];
            result[2] = a[2] * b[2] + (1.0F - a[2]) * c[2];
            result[3] = a[3] * b[3] + (1.0F - a[3]) * c[3];
            store_vector4(inst, machine, result);
            if (DEBUG_PROG) {
               printf("LRP (%g %g %g %g) = (%g %g %g %g), "
                      "(%g %g %g %g), (%g %g %g %g)\n",
                      result[0], result[1], result[2], result[3],
                      a[0], a[1], a[2], a[3],
                      b[0], b[1], b[2], b[3], c[0], c[1], c[2], c[3]);
            }
         }
         break;
      case OPCODE_MAD:
         {
            GLfloat a[4], b[4], c[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            fetch_vector4(&inst->SrcReg[1], machine, b);
            fetch_vector4(&inst->SrcReg[2], machine, c);
            result[0] = a[0] * b[0] + c[0];
            result[1] = a[1] * b[1] + c[1];
            result[2] = a[2] * b[2] + c[2];
            result[3] = a[3] * b[3] + c[3];
            store_vector4(inst, machine, result);
            if (DEBUG_PROG) {
               printf("MAD (%g %g %g %g) = (%g %g %g %g) * "
                      "(%g %g %g %g) + (%g %g %g %g)\n",
                      result[0], result[1], result[2], result[3],
                      a[0], a[1], a[2], a[3],
                      b[0], b[1], b[2], b[3], c[0], c[1], c[2], c[3]);
            }
         }
         break;
      case OPCODE_MAX:
         {
            GLfloat a[4], b[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            fetch_vector4(&inst->SrcReg[1], machine, b);
            result[0] = MAX2(a[0], b[0]);
            result[1] = MAX2(a[1], b[1]);
            result[2] = MAX2(a[2], b[2]);
            result[3] = MAX2(a[3], b[3]);
            store_vector4(inst, machine, result);
            if (DEBUG_PROG) {
               printf("MAX (%g %g %g %g) = (%g %g %g %g), (%g %g %g %g)\n",
                      result[0], result[1], result[2], result[3],
                      a[0], a[1], a[2], a[3], b[0], b[1], b[2], b[3]);
            }
         }
         break;
      case OPCODE_MIN:
         {
            GLfloat a[4], b[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            fetch_vector4(&inst->SrcReg[1], machine, b);
            result[0] = MIN2(a[0], b[0]);
            result[1] = MIN2(a[1], b[1]);
            result[2] = MIN2(a[2], b[2]);
            result[3] = MIN2(a[3], b[3]);
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_MOV:
         {
            GLfloat result[4];
            fetch_vector4(&inst->SrcReg[0], machine, result);
            store_vector4(inst, machine, result);
            if (DEBUG_PROG) {
               printf("MOV (%g %g %g %g)\n",
                      result[0], result[1], result[2], result[3]);
            }
         }
         break;
      case OPCODE_MUL:
         {
            GLfloat a[4], b[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            fetch_vector4(&inst->SrcReg[1], machine, b);
            result[0] = a[0] * b[0];
            result[1] = a[1] * b[1];
            result[2] = a[2] * b[2];
            result[3] = a[3] * b[3];
            store_vector4(inst, machine, result);
            if (DEBUG_PROG) {
               printf("MUL (%g %g %g %g) = (%g %g %g %g) * (%g %g %g %g)\n",
                      result[0], result[1], result[2], result[3],
                      a[0], a[1], a[2], a[3], b[0], b[1], b[2], b[3]);
            }
         }
         break;
      case OPCODE_NOISE1:
         {
            GLfloat a[4], result[4];
            fetch_vector1(&inst->SrcReg[0], machine, a);
            result[0] =
               result[1] =
               result[2] =
               result[3] = _mesa_noise1(a[0]);
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_NOISE2:
         {
            GLfloat a[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            result[0] =
               result[1] =
               result[2] = result[3] = _mesa_noise2(a[0], a[1]);
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_NOISE3:
         {
            GLfloat a[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            result[0] =
               result[1] =
               result[2] =
               result[3] = _mesa_noise3(a[0], a[1], a[2]);
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_NOISE4:
         {
            GLfloat a[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            result[0] =
               result[1] =
               result[2] =
               result[3] = _mesa_noise4(a[0], a[1], a[2], a[3]);
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_NOP:
         break;
      case OPCODE_NOT:         /* bitwise NOT */
         {
            GLuint a[4], result[4];
            fetch_vector4ui(&inst->SrcReg[0], machine, a);
            result[0] = ~a[0];
            result[1] = ~a[1];
            result[2] = ~a[2];
            result[3] = ~a[3];
            store_vector4ui(inst, machine, result);
         }
         break;
      case OPCODE_NRM3:        /* 3-component normalization */
         {
            GLfloat a[4], result[4];
            GLfloat tmp;
            fetch_vector4(&inst->SrcReg[0], machine, a);
            tmp = a[0] * a[0] + a[1] * a[1] + a[2] * a[2];
            if (tmp != 0.0F)
               tmp = INV_SQRTF(tmp);
            result[0] = tmp * a[0];
            result[1] = tmp * a[1];
            result[2] = tmp * a[2];
            result[3] = 0.0;  /* undefined, but prevent valgrind warnings */
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_NRM4:        /* 4-component normalization */
         {
            GLfloat a[4], result[4];
            GLfloat tmp;
            fetch_vector4(&inst->SrcReg[0], machine, a);
            tmp = a[0] * a[0] + a[1] * a[1] + a[2] * a[2] + a[3] * a[3];
            if (tmp != 0.0F)
               tmp = INV_SQRTF(tmp);
            result[0] = tmp * a[0];
            result[1] = tmp * a[1];
            result[2] = tmp * a[2];
            result[3] = tmp * a[3];
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_OR:          /* bitwise OR */
         {
            GLuint a[4], b[4], result[4];
            fetch_vector4ui(&inst->SrcReg[0], machine, a);
            fetch_vector4ui(&inst->SrcReg[1], machine, b);
            result[0] = a[0] | b[0];
            result[1] = a[1] | b[1];
            result[2] = a[2] | b[2];
            result[3] = a[3] | b[3];
            store_vector4ui(inst, machine, result);
         }
         break;
      case OPCODE_PK2H:        /* pack two 16-bit floats in one 32-bit float */
         {
            GLfloat a[4];
            GLuint result[4];
            GLhalfNV hx, hy;
            fetch_vector4(&inst->SrcReg[0], machine, a);
            hx = _mesa_float_to_half(a[0]);
            hy = _mesa_float_to_half(a[1]);
            result[0] =
            result[1] =
            result[2] =
            result[3] = hx | (hy << 16);
            store_vector4ui(inst, machine, result);
         }
         break;
      case OPCODE_PK2US:       /* pack two GLushorts into one 32-bit float */
         {
            GLfloat a[4];
            GLuint result[4], usx, usy;
            fetch_vector4(&inst->SrcReg[0], machine, a);
            a[0] = CLAMP(a[0], 0.0F, 1.0F);
            a[1] = CLAMP(a[1], 0.0F, 1.0F);
            usx = IROUND(a[0] * 65535.0F);
            usy = IROUND(a[1] * 65535.0F);
            result[0] =
            result[1] =
            result[2] =
            result[3] = usx | (usy << 16);
            store_vector4ui(inst, machine, result);
         }
         break;
      case OPCODE_PK4B:        /* pack four GLbytes into one 32-bit float */
         {
            GLfloat a[4];
            GLuint result[4], ubx, uby, ubz, ubw;
            fetch_vector4(&inst->SrcReg[0], machine, a);
            a[0] = CLAMP(a[0], -128.0F / 127.0F, 1.0F);
            a[1] = CLAMP(a[1], -128.0F / 127.0F, 1.0F);
            a[2] = CLAMP(a[2], -128.0F / 127.0F, 1.0F);
            a[3] = CLAMP(a[3], -128.0F / 127.0F, 1.0F);
            ubx = IROUND(127.0F * a[0] + 128.0F);
            uby = IROUND(127.0F * a[1] + 128.0F);
            ubz = IROUND(127.0F * a[2] + 128.0F);
            ubw = IROUND(127.0F * a[3] + 128.0F);
            result[0] =
            result[1] =
            result[2] =
            result[3] = ubx | (uby << 8) | (ubz << 16) | (ubw << 24);
            store_vector4ui(inst, machine, result);
         }
         break;
      case OPCODE_PK4UB:       /* pack four GLubytes into one 32-bit float */
         {
            GLfloat a[4];
            GLuint result[4], ubx, uby, ubz, ubw;
            fetch_vector4(&inst->SrcReg[0], machine, a);
            a[0] = CLAMP(a[0], 0.0F, 1.0F);
            a[1] = CLAMP(a[1], 0.0F, 1.0F);
            a[2] = CLAMP(a[2], 0.0F, 1.0F);
            a[3] = CLAMP(a[3], 0.0F, 1.0F);
            ubx = IROUND(255.0F * a[0]);
            uby = IROUND(255.0F * a[1]);
            ubz = IROUND(255.0F * a[2]);
            ubw = IROUND(255.0F * a[3]);
            result[0] =
            result[1] =
            result[2] =
            result[3] = ubx | (uby << 8) | (ubz << 16) | (ubw << 24);
            store_vector4ui(inst, machine, result);
         }
         break;
      case OPCODE_POW:
         {
            GLfloat a[4], b[4], result[4];
            fetch_vector1(&inst->SrcReg[0], machine, a);
            fetch_vector1(&inst->SrcReg[1], machine, b);
            result[0] = result[1] = result[2] = result[3]
               = (GLfloat) _mesa_pow(a[0], b[0]);
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_RCP:
         {
            GLfloat a[4], result[4];
            fetch_vector1(&inst->SrcReg[0], machine, a);
            if (DEBUG_PROG) {
               if (a[0] == 0)
                  printf("RCP(0)\n");
               else if (IS_INF_OR_NAN(a[0]))
                  printf("RCP(inf)\n");
            }
            result[0] = result[1] = result[2] = result[3] = 1.0F / a[0];
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_RET:         /* return from subroutine (conditional) */
         if (eval_condition(machine, inst)) {
            if (machine->StackDepth == 0) {
               return GL_TRUE;  /* Per GL_NV_vertex_program2 spec */
            }
            /* subtract one because of pc++ in the for loop */
            pc = machine->CallStack[--machine->StackDepth] - 1;
         }
         break;
      case OPCODE_RFL:         /* reflection vector */
         {
            GLfloat axis[4], dir[4], result[4], tmpX, tmpW;
            fetch_vector4(&inst->SrcReg[0], machine, axis);
            fetch_vector4(&inst->SrcReg[1], machine, dir);
            tmpW = DOT3(axis, axis);
            tmpX = (2.0F * DOT3(axis, dir)) / tmpW;
            result[0] = tmpX * axis[0] - dir[0];
            result[1] = tmpX * axis[1] - dir[1];
            result[2] = tmpX * axis[2] - dir[2];
            /* result[3] is never written! XXX enforce in parser! */
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_RSQ:         /* 1 / sqrt() */
         {
            GLfloat a[4], result[4];
            fetch_vector1(&inst->SrcReg[0], machine, a);
            a[0] = FABSF(a[0]);
            result[0] = result[1] = result[2] = result[3] = INV_SQRTF(a[0]);
            store_vector4(inst, machine, result);
            if (DEBUG_PROG) {
               printf("RSQ %g = 1/sqrt(|%g|)\n", result[0], a[0]);
            }
         }
         break;
      case OPCODE_SCS:         /* sine and cos */
         {
            GLfloat a[4], result[4];
            fetch_vector1(&inst->SrcReg[0], machine, a);
            result[0] = (GLfloat) _mesa_cos(a[0]);
            result[1] = (GLfloat) _mesa_sin(a[0]);
            result[2] = 0.0;    /* undefined! */
            result[3] = 0.0;    /* undefined! */
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_SEQ:         /* set on equal */
         {
            GLfloat a[4], b[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            fetch_vector4(&inst->SrcReg[1], machine, b);
            result[0] = (a[0] == b[0]) ? 1.0F : 0.0F;
            result[1] = (a[1] == b[1]) ? 1.0F : 0.0F;
            result[2] = (a[2] == b[2]) ? 1.0F : 0.0F;
            result[3] = (a[3] == b[3]) ? 1.0F : 0.0F;
            store_vector4(inst, machine, result);
            if (DEBUG_PROG) {
               printf("SEQ (%g %g %g %g) = (%g %g %g %g) == (%g %g %g %g)\n",
                      result[0], result[1], result[2], result[3],
                      a[0], a[1], a[2], a[3],
                      b[0], b[1], b[2], b[3]);
            }
         }
         break;
      case OPCODE_SFL:         /* set false, operands ignored */
         {
            static const GLfloat result[4] = { 0.0F, 0.0F, 0.0F, 0.0F };
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_SGE:         /* set on greater or equal */
         {
            GLfloat a[4], b[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            fetch_vector4(&inst->SrcReg[1], machine, b);
            result[0] = (a[0] >= b[0]) ? 1.0F : 0.0F;
            result[1] = (a[1] >= b[1]) ? 1.0F : 0.0F;
            result[2] = (a[2] >= b[2]) ? 1.0F : 0.0F;
            result[3] = (a[3] >= b[3]) ? 1.0F : 0.0F;
            store_vector4(inst, machine, result);
            if (DEBUG_PROG) {
               printf("SGE (%g %g %g %g) = (%g %g %g %g) >= (%g %g %g %g)\n",
                      result[0], result[1], result[2], result[3],
                      a[0], a[1], a[2], a[3],
                      b[0], b[1], b[2], b[3]);
            }
         }
         break;
      case OPCODE_SGT:         /* set on greater */
         {
            GLfloat a[4], b[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            fetch_vector4(&inst->SrcReg[1], machine, b);
            result[0] = (a[0] > b[0]) ? 1.0F : 0.0F;
            result[1] = (a[1] > b[1]) ? 1.0F : 0.0F;
            result[2] = (a[2] > b[2]) ? 1.0F : 0.0F;
            result[3] = (a[3] > b[3]) ? 1.0F : 0.0F;
            store_vector4(inst, machine, result);
            if (DEBUG_PROG) {
               printf("SGT (%g %g %g %g) = (%g %g %g %g) > (%g %g %g %g)\n",
                      result[0], result[1], result[2], result[3],
                      a[0], a[1], a[2], a[3],
                      b[0], b[1], b[2], b[3]);
            }
         }
         break;
      case OPCODE_SIN:
         {
            GLfloat a[4], result[4];
            fetch_vector1(&inst->SrcReg[0], machine, a);
            result[0] = result[1] = result[2] = result[3]
               = (GLfloat) _mesa_sin(a[0]);
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_SLE:         /* set on less or equal */
         {
            GLfloat a[4], b[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            fetch_vector4(&inst->SrcReg[1], machine, b);
            result[0] = (a[0] <= b[0]) ? 1.0F : 0.0F;
            result[1] = (a[1] <= b[1]) ? 1.0F : 0.0F;
            result[2] = (a[2] <= b[2]) ? 1.0F : 0.0F;
            result[3] = (a[3] <= b[3]) ? 1.0F : 0.0F;
            store_vector4(inst, machine, result);
            if (DEBUG_PROG) {
               printf("SLE (%g %g %g %g) = (%g %g %g %g) <= (%g %g %g %g)\n",
                      result[0], result[1], result[2], result[3],
                      a[0], a[1], a[2], a[3],
                      b[0], b[1], b[2], b[3]);
            }
         }
         break;
      case OPCODE_SLT:         /* set on less */
         {
            GLfloat a[4], b[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            fetch_vector4(&inst->SrcReg[1], machine, b);
            result[0] = (a[0] < b[0]) ? 1.0F : 0.0F;
            result[1] = (a[1] < b[1]) ? 1.0F : 0.0F;
            result[2] = (a[2] < b[2]) ? 1.0F : 0.0F;
            result[3] = (a[3] < b[3]) ? 1.0F : 0.0F;
            store_vector4(inst, machine, result);
            if (DEBUG_PROG) {
               printf("SLT (%g %g %g %g) = (%g %g %g %g) < (%g %g %g %g)\n",
                      result[0], result[1], result[2], result[3],
                      a[0], a[1], a[2], a[3],
                      b[0], b[1], b[2], b[3]);
            }
         }
         break;
      case OPCODE_SNE:         /* set on not equal */
         {
            GLfloat a[4], b[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            fetch_vector4(&inst->SrcReg[1], machine, b);
            result[0] = (a[0] != b[0]) ? 1.0F : 0.0F;
            result[1] = (a[1] != b[1]) ? 1.0F : 0.0F;
            result[2] = (a[2] != b[2]) ? 1.0F : 0.0F;
            result[3] = (a[3] != b[3]) ? 1.0F : 0.0F;
            store_vector4(inst, machine, result);
            if (DEBUG_PROG) {
               printf("SNE (%g %g %g %g) = (%g %g %g %g) != (%g %g %g %g)\n",
                      result[0], result[1], result[2], result[3],
                      a[0], a[1], a[2], a[3],
                      b[0], b[1], b[2], b[3]);
            }
         }
         break;
      case OPCODE_SSG:         /* set sign (-1, 0 or +1) */
         {
            GLfloat a[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            result[0] = (GLfloat) ((a[0] > 0.0F) - (a[0] < 0.0F));
            result[1] = (GLfloat) ((a[1] > 0.0F) - (a[1] < 0.0F));
            result[2] = (GLfloat) ((a[2] > 0.0F) - (a[2] < 0.0F));
            result[3] = (GLfloat) ((a[3] > 0.0F) - (a[3] < 0.0F));
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_STR:         /* set true, operands ignored */
         {
            static const GLfloat result[4] = { 1.0F, 1.0F, 1.0F, 1.0F };
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_SUB:
         {
            GLfloat a[4], b[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            fetch_vector4(&inst->SrcReg[1], machine, b);
            result[0] = a[0] - b[0];
            result[1] = a[1] - b[1];
            result[2] = a[2] - b[2];
            result[3] = a[3] - b[3];
            store_vector4(inst, machine, result);
            if (DEBUG_PROG) {
               printf("SUB (%g %g %g %g) = (%g %g %g %g) - (%g %g %g %g)\n",
                      result[0], result[1], result[2], result[3],
                      a[0], a[1], a[2], a[3], b[0], b[1], b[2], b[3]);
            }
         }
         break;
      case OPCODE_SWZ:         /* extended swizzle */
         {
            const struct prog_src_register *source = &inst->SrcReg[0];
            const GLfloat *src = get_src_register_pointer(source, machine);
            GLfloat result[4];
            GLuint i;
            for (i = 0; i < 4; i++) {
               const GLuint swz = GET_SWZ(source->Swizzle, i);
               if (swz == SWIZZLE_ZERO)
                  result[i] = 0.0;
               else if (swz == SWIZZLE_ONE)
                  result[i] = 1.0;
               else {
                  ASSERT(swz >= 0);
                  ASSERT(swz <= 3);
                  result[i] = src[swz];
               }
               if (source->NegateBase & (1 << i))
                  result[i] = -result[i];
            }
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_TEX:         /* Both ARB and NV frag prog */
         /* Simple texel lookup */
         {
            GLfloat texcoord[4], color[4];
            fetch_vector4(&inst->SrcReg[0], machine, texcoord);

            fetch_texel(ctx, machine, inst, texcoord, 0.0, color);

            if (DEBUG_PROG) {
               printf("TEX (%g, %g, %g, %g) = texture[%d][%g, %g, %g, %g]\n",
                      color[0], color[1], color[2], color[3],
                      inst->TexSrcUnit,
                      texcoord[0], texcoord[1], texcoord[2], texcoord[3]);
            }
            store_vector4(inst, machine, color);
         }
         break;
      case OPCODE_TXB:         /* GL_ARB_fragment_program only */
         /* Texel lookup with LOD bias */
         {
            const struct gl_texture_unit *texUnit
               = &ctx->Texture.Unit[inst->TexSrcUnit];
            GLfloat texcoord[4], color[4], lodBias;

            fetch_vector4(&inst->SrcReg[0], machine, texcoord);

            /* texcoord[3] is the bias to add to lambda */
            lodBias = texUnit->LodBias + texcoord[3];
            if (texUnit->_Current) {
               lodBias += texUnit->_Current->LodBias;
            }

            fetch_texel(ctx, machine, inst, texcoord, lodBias, color);

            store_vector4(inst, machine, color);
         }
         break;
      case OPCODE_TXD:         /* GL_NV_fragment_program only */
         /* Texture lookup w/ partial derivatives for LOD */
         {
            GLfloat texcoord[4], dtdx[4], dtdy[4], color[4];
            fetch_vector4(&inst->SrcReg[0], machine, texcoord);
            fetch_vector4(&inst->SrcReg[1], machine, dtdx);
            fetch_vector4(&inst->SrcReg[2], machine, dtdy);
            machine->FetchTexelDeriv(ctx, texcoord, dtdx, dtdy,
                                     0.0, /* lodBias */
                                     inst->TexSrcUnit, color);
            store_vector4(inst, machine, color);
         }
         break;
      case OPCODE_TXP:         /* GL_ARB_fragment_program only */
         /* Texture lookup w/ projective divide */
         {
            GLfloat texcoord[4], color[4];

            fetch_vector4(&inst->SrcReg[0], machine, texcoord);
            /* Not so sure about this test - if texcoord[3] is
             * zero, we'd probably be fine except for an ASSERT in
             * IROUND_POS() which gets triggered by the inf values created.
             */
            if (texcoord[3] != 0.0) {
               texcoord[0] /= texcoord[3];
               texcoord[1] /= texcoord[3];
               texcoord[2] /= texcoord[3];
            }

            fetch_texel(ctx, machine, inst, texcoord, 0.0, color);

            store_vector4(inst, machine, color);
         }
         break;
      case OPCODE_TXP_NV:      /* GL_NV_fragment_program only */
         /* Texture lookup w/ projective divide, as above, but do not
          * do the divide by w if sampling from a cube map.
          */
         {
            GLfloat texcoord[4], color[4];

            fetch_vector4(&inst->SrcReg[0], machine, texcoord);
            if (inst->TexSrcTarget != TEXTURE_CUBE_INDEX &&
                texcoord[3] != 0.0) {
               texcoord[0] /= texcoord[3];
               texcoord[1] /= texcoord[3];
               texcoord[2] /= texcoord[3];
            }

            fetch_texel(ctx, machine, inst, texcoord, 0.0, color);

            store_vector4(inst, machine, color);
         }
         break;
      case OPCODE_TRUNC:       /* truncate toward zero */
         {
            GLfloat a[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            result[0] = (GLfloat) (GLint) a[0];
            result[1] = (GLfloat) (GLint) a[1];
            result[2] = (GLfloat) (GLint) a[2];
            result[3] = (GLfloat) (GLint) a[3];
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_UP2H:        /* unpack two 16-bit floats */
         {
            GLfloat a[4], result[4];
            const GLuint *rawBits = (const GLuint *) a;
            GLhalfNV hx, hy;
            fetch_vector1(&inst->SrcReg[0], machine, a);
            hx = rawBits[0] & 0xffff;
            hy = rawBits[0] >> 16;
            result[0] = result[2] = _mesa_half_to_float(hx);
            result[1] = result[3] = _mesa_half_to_float(hy);
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_UP2US:       /* unpack two GLushorts */
         {
            GLfloat a[4], result[4];
            const GLuint *rawBits = (const GLuint *) a;
            GLushort usx, usy;
            fetch_vector1(&inst->SrcReg[0], machine, a);
            usx = rawBits[0] & 0xffff;
            usy = rawBits[0] >> 16;
            result[0] = result[2] = usx * (1.0f / 65535.0f);
            result[1] = result[3] = usy * (1.0f / 65535.0f);
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_UP4B:        /* unpack four GLbytes */
         {
            GLfloat a[4], result[4];
            const GLuint *rawBits = (const GLuint *) a;
            fetch_vector1(&inst->SrcReg[0], machine, a);
            result[0] = (((rawBits[0] >> 0) & 0xff) - 128) / 127.0F;
            result[1] = (((rawBits[0] >> 8) & 0xff) - 128) / 127.0F;
            result[2] = (((rawBits[0] >> 16) & 0xff) - 128) / 127.0F;
            result[3] = (((rawBits[0] >> 24) & 0xff) - 128) / 127.0F;
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_UP4UB:       /* unpack four GLubytes */
         {
            GLfloat a[4], result[4];
            const GLuint *rawBits = (const GLuint *) a;
            fetch_vector1(&inst->SrcReg[0], machine, a);
            result[0] = ((rawBits[0] >> 0) & 0xff) / 255.0F;
            result[1] = ((rawBits[0] >> 8) & 0xff) / 255.0F;
            result[2] = ((rawBits[0] >> 16) & 0xff) / 255.0F;
            result[3] = ((rawBits[0] >> 24) & 0xff) / 255.0F;
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_XOR:         /* bitwise XOR */
         {
            GLuint a[4], b[4], result[4];
            fetch_vector4ui(&inst->SrcReg[0], machine, a);
            fetch_vector4ui(&inst->SrcReg[1], machine, b);
            result[0] = a[0] ^ b[0];
            result[1] = a[1] ^ b[1];
            result[2] = a[2] ^ b[2];
            result[3] = a[3] ^ b[3];
            store_vector4ui(inst, machine, result);
         }
         break;
      case OPCODE_XPD:         /* cross product */
         {
            GLfloat a[4], b[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            fetch_vector4(&inst->SrcReg[1], machine, b);
            result[0] = a[1] * b[2] - a[2] * b[1];
            result[1] = a[2] * b[0] - a[0] * b[2];
            result[2] = a[0] * b[1] - a[1] * b[0];
            result[3] = 1.0;
            store_vector4(inst, machine, result);
            if (DEBUG_PROG) {
               printf("XPD (%g %g %g %g) = (%g %g %g) X (%g %g %g)\n",
                      result[0], result[1], result[2], result[3],
                      a[0], a[1], a[2], b[0], b[1], b[2]);
            }
         }
         break;
      case OPCODE_X2D:         /* 2-D matrix transform */
         {
            GLfloat a[4], b[4], c[4], result[4];
            fetch_vector4(&inst->SrcReg[0], machine, a);
            fetch_vector4(&inst->SrcReg[1], machine, b);
            fetch_vector4(&inst->SrcReg[2], machine, c);
            result[0] = a[0] + b[0] * c[0] + b[1] * c[1];
            result[1] = a[1] + b[0] * c[2] + b[1] * c[3];
            result[2] = a[2] + b[0] * c[0] + b[1] * c[1];
            result[3] = a[3] + b[0] * c[2] + b[1] * c[3];
            store_vector4(inst, machine, result);
         }
         break;
      case OPCODE_PRINT:
         {
            if (inst->SrcReg[0].File != -1) {
               GLfloat a[4];
               fetch_vector4(&inst->SrcReg[0], machine, a);
               _mesa_printf("%s%g, %g, %g, %g\n", (const char *) inst->Data,
                            a[0], a[1], a[2], a[3]);
            }
            else {
               _mesa_printf("%s\n", (const char *) inst->Data);
            }
         }
         break;
      case OPCODE_END:
         return GL_TRUE;
      default:
         _mesa_problem(ctx, "Bad opcode %d in _mesa_execute_program",
                       inst->Opcode);
         return GL_TRUE;        /* return value doesn't matter */
      }

      numExec++;
      if (numExec > maxExec) {
         _mesa_problem(ctx, "Infinite loop detected in fragment program");
         return GL_TRUE;
      }

   } /* for pc */

#if FEATURE_MESA_program_debug
   CurrentMachine = NULL;
#endif

   return GL_TRUE;
}
