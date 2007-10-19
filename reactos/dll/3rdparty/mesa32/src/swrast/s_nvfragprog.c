/*
 * Mesa 3-D graphics library
 * Version:  6.4
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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

/*
 * Regarding GL_NV_fragment_program:
 *
 * Portions of this software may use or implement intellectual
 * property owned and licensed by NVIDIA Corporation. NVIDIA disclaims
 * any and all warranties with respect to such intellectual property,
 * including any use thereof or modifications thereto.
 */

#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "nvfragprog.h"
#include "macros.h"
#include "program.h"

#include "s_nvfragprog.h"
#include "s_span.h"
#include "s_texture.h"


/* if 1, print some debugging info */
#define DEBUG_FRAG 0

/**
 * Fetch a texel.
 */
static void
fetch_texel( GLcontext *ctx, const GLfloat texcoord[4], GLfloat lambda,
             GLuint unit, GLfloat color[4] )
{
   GLchan rgba[4];
   SWcontext *swrast = SWRAST_CONTEXT(ctx);

   /* XXX use a float-valued TextureSample routine here!!! */
   swrast->TextureSample[unit](ctx, unit, ctx->Texture.Unit[unit]._Current,
                               1, (const GLfloat (*)[4]) texcoord,
                               &lambda, &rgba);
   color[0] = CHAN_TO_FLOAT(rgba[0]);
   color[1] = CHAN_TO_FLOAT(rgba[1]);
   color[2] = CHAN_TO_FLOAT(rgba[2]);
   color[3] = CHAN_TO_FLOAT(rgba[3]);
}


/**
 * Fetch a texel with the given partial derivatives to compute a level
 * of detail in the mipmap.
 */
static void
fetch_texel_deriv( GLcontext *ctx, const GLfloat texcoord[4],
                   const GLfloat texdx[4], const GLfloat texdy[4],
                   GLuint unit, GLfloat color[4] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const struct gl_texture_object *texObj = ctx->Texture.Unit[unit]._Current;
   const struct gl_texture_image *texImg = texObj->Image[0][texObj->BaseLevel];
   const GLfloat texW = (GLfloat) texImg->WidthScale;
   const GLfloat texH = (GLfloat) texImg->HeightScale;
   GLchan rgba[4];

   GLfloat lambda = _swrast_compute_lambda(texdx[0], texdy[0], /* ds/dx, ds/dy */
                                         texdx[1], texdy[1], /* dt/dx, dt/dy */
                                         texdx[3], texdy[2], /* dq/dx, dq/dy */
                                         texW, texH,
                                         texcoord[0], texcoord[1], texcoord[3],
                                         1.0F / texcoord[3]);

   swrast->TextureSample[unit](ctx, unit, ctx->Texture.Unit[unit]._Current,
                               1, (const GLfloat (*)[4]) texcoord,
                               &lambda, &rgba);
   color[0] = CHAN_TO_FLOAT(rgba[0]);
   color[1] = CHAN_TO_FLOAT(rgba[1]);
   color[2] = CHAN_TO_FLOAT(rgba[2]);
   color[3] = CHAN_TO_FLOAT(rgba[3]);
}


/**
 * Return a pointer to the 4-element float vector specified by the given
 * source register.
 */
static INLINE const GLfloat *
get_register_pointer( GLcontext *ctx,
                      const struct fp_src_register *source,
                      const struct fp_machine *machine,
                      const struct fragment_program *program )
{
   const GLfloat *src;
   switch (source->File) {
      case PROGRAM_TEMPORARY:
         ASSERT(source->Index < MAX_NV_FRAGMENT_PROGRAM_TEMPS);
         src = machine->Temporaries[source->Index];
         break;
      case PROGRAM_INPUT:
         ASSERT(source->Index < MAX_NV_FRAGMENT_PROGRAM_INPUTS);
         src = machine->Inputs[source->Index];
         break;
      case PROGRAM_OUTPUT:
         /* This is only for PRINT */
         ASSERT(source->Index < MAX_NV_FRAGMENT_PROGRAM_OUTPUTS);
         src = machine->Outputs[source->Index];
         break;
      case PROGRAM_LOCAL_PARAM:
         ASSERT(source->Index < MAX_PROGRAM_LOCAL_PARAMS);
         src = program->Base.LocalParams[source->Index];
         break;
      case PROGRAM_ENV_PARAM:
         ASSERT(source->Index < MAX_NV_FRAGMENT_PROGRAM_PARAMS);
         src = ctx->FragmentProgram.Parameters[source->Index];
         break;
      case PROGRAM_STATE_VAR:
         /* Fallthrough */
      case PROGRAM_NAMED_PARAM:
         ASSERT(source->Index < (GLint) program->Parameters->NumParameters);
         src = program->Parameters->ParameterValues[source->Index];
         break;
      default:
         _mesa_problem(ctx, "Invalid input register file %d in fetch_vector4", source->File);
         src = NULL;
   }
   return src;
}


/**
 * Fetch a 4-element float vector from the given source register.
 * Apply swizzling and negating as needed.
 */
static void
fetch_vector4( GLcontext *ctx,
               const struct fp_src_register *source,
               const struct fp_machine *machine,
               const struct fragment_program *program,
               GLfloat result[4] )
{
   const GLfloat *src = get_register_pointer(ctx, source, machine, program);
   ASSERT(src);

   result[0] = src[GET_SWZ(source->Swizzle, 0)];
   result[1] = src[GET_SWZ(source->Swizzle, 1)];
   result[2] = src[GET_SWZ(source->Swizzle, 2)];
   result[3] = src[GET_SWZ(source->Swizzle, 3)];

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
 * Fetch the derivative with respect to X for the given register.
 * \return GL_TRUE if it was easily computed or GL_FALSE if we
 * need to execute another instance of the program (ugh)!
 */
static GLboolean
fetch_vector4_deriv( GLcontext *ctx,
                     const struct fp_src_register *source,
                     const struct sw_span *span,
                     char xOrY, GLint column, GLfloat result[4] )
{
   GLfloat src[4];

   ASSERT(xOrY == 'X' || xOrY == 'Y');

   switch (source->Index) {
   case FRAG_ATTRIB_WPOS:
      if (xOrY == 'X') {
         src[0] = 1.0;
         src[1] = 0.0;
         src[2] = span->dzdx / ctx->DrawBuffer->_DepthMaxF;
         src[3] = span->dwdx;
      }
      else {
         src[0] = 0.0;
         src[1] = 1.0;
         src[2] = span->dzdy / ctx->DrawBuffer->_DepthMaxF;
         src[3] = span->dwdy;
      }
      break;
   case FRAG_ATTRIB_COL0:
      if (xOrY == 'X') {
         src[0] = span->drdx * (1.0F / CHAN_MAXF);
         src[1] = span->dgdx * (1.0F / CHAN_MAXF);
         src[2] = span->dbdx * (1.0F / CHAN_MAXF);
         src[3] = span->dadx * (1.0F / CHAN_MAXF);
      }
      else {
         src[0] = span->drdy * (1.0F / CHAN_MAXF);
         src[1] = span->dgdy * (1.0F / CHAN_MAXF);
         src[2] = span->dbdy * (1.0F / CHAN_MAXF);
         src[3] = span->dady * (1.0F / CHAN_MAXF);
      }
      break;
   case FRAG_ATTRIB_COL1:
      if (xOrY == 'X') {
         src[0] = span->dsrdx * (1.0F / CHAN_MAXF);
         src[1] = span->dsgdx * (1.0F / CHAN_MAXF);
         src[2] = span->dsbdx * (1.0F / CHAN_MAXF);
         src[3] = 0.0; /* XXX need this */
      }
      else {
         src[0] = span->dsrdy * (1.0F / CHAN_MAXF);
         src[1] = span->dsgdy * (1.0F / CHAN_MAXF);
         src[2] = span->dsbdy * (1.0F / CHAN_MAXF);
         src[3] = 0.0; /* XXX need this */
      }
      break;
   case FRAG_ATTRIB_FOGC:
      if (xOrY == 'X') {
         src[0] = span->dfogdx;
         src[1] = 0.0;
         src[2] = 0.0;
         src[3] = 0.0;
      }
      else {
         src[0] = span->dfogdy;
         src[1] = 0.0;
         src[2] = 0.0;
         src[3] = 0.0;
      }
      break;
   case FRAG_ATTRIB_TEX0:
   case FRAG_ATTRIB_TEX1:
   case FRAG_ATTRIB_TEX2:
   case FRAG_ATTRIB_TEX3:
   case FRAG_ATTRIB_TEX4:
   case FRAG_ATTRIB_TEX5:
   case FRAG_ATTRIB_TEX6:
   case FRAG_ATTRIB_TEX7:
      if (xOrY == 'X') {
         const GLuint u = source->Index - FRAG_ATTRIB_TEX0;
         /* this is a little tricky - I think I've got it right */
         const GLfloat invQ = 1.0f / (span->tex[u][3]
                                      + span->texStepX[u][3] * column);
         src[0] = span->texStepX[u][0] * invQ;
         src[1] = span->texStepX[u][1] * invQ;
         src[2] = span->texStepX[u][2] * invQ;
         src[3] = span->texStepX[u][3] * invQ;
      }
      else {
         const GLuint u = source->Index - FRAG_ATTRIB_TEX0;
         /* Tricky, as above, but in Y direction */
         const GLfloat invQ = 1.0f / (span->tex[u][3] + span->texStepY[u][3]);
         src[0] = span->texStepY[u][0] * invQ;
         src[1] = span->texStepY[u][1] * invQ;
         src[2] = span->texStepY[u][2] * invQ;
         src[3] = span->texStepY[u][3] * invQ;
      }
      break;
   default:
      return GL_FALSE;
   }

   result[0] = src[GET_SWZ(source->Swizzle, 0)];
   result[1] = src[GET_SWZ(source->Swizzle, 1)];
   result[2] = src[GET_SWZ(source->Swizzle, 2)];
   result[3] = src[GET_SWZ(source->Swizzle, 3)];

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
   return GL_TRUE;
}


/**
 * As above, but only return result[0] element.
 */
static void
fetch_vector1( GLcontext *ctx,
               const struct fp_src_register *source,
               const struct fp_machine *machine,
               const struct fragment_program *program,
               GLfloat result[4] )
{
   const GLfloat *src = get_register_pointer(ctx, source, machine, program);
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
 * Test value against zero and return GT, LT, EQ or UN if NaN.
 */
static INLINE GLuint
generate_cc( float value )
{
   if (value != value)
      return COND_UN;  /* NaN */
   if (value > 0.0F)
      return COND_GT;
   if (value < 0.0F)
      return COND_LT;
   return COND_EQ;
}


/**
 * Test if the ccMaskRule is satisfied by the given condition code.
 * Used to mask destination writes according to the current condition codee.
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
 * Store 4 floats into a register.  Observe the instructions saturate and
 * set-condition-code flags.
 */
static void
store_vector4( const struct fp_instruction *inst,
               struct fp_machine *machine,
               const GLfloat value[4] )
{
   const struct fp_dst_register *dest = &(inst->DstReg);
   const GLboolean clamp = inst->Saturate;
   const GLboolean updateCC = inst->UpdateCondRegister;
   GLfloat *dstReg;
   GLfloat dummyReg[4];
   GLfloat clampedValue[4];
   GLboolean condWriteMask[4];
   GLuint writeMask = dest->WriteMask;

   switch (dest->File) {
      case PROGRAM_OUTPUT:
         dstReg = machine->Outputs[dest->Index];
         break;
      case PROGRAM_TEMPORARY:
         dstReg = machine->Temporaries[dest->Index];
         break;
      case PROGRAM_WRITE_ONLY:
         dstReg = dummyReg;
         return;
      default:
         _mesa_problem(NULL, "bad register file in store_vector4(fp)");
         return;
   }

#if DEBUG_FRAG
   if (value[0] > 1.0e10 ||
       IS_INF_OR_NAN(value[0]) ||
       IS_INF_OR_NAN(value[1]) ||
       IS_INF_OR_NAN(value[2]) ||
       IS_INF_OR_NAN(value[3])  )
      printf("store %g %g %g %g\n", value[0], value[1], value[2], value[3]);
#endif

   if (clamp) {
      clampedValue[0] = CLAMP(value[0], 0.0F, 1.0F);
      clampedValue[1] = CLAMP(value[1], 0.0F, 1.0F);
      clampedValue[2] = CLAMP(value[2], 0.0F, 1.0F);
      clampedValue[3] = CLAMP(value[3], 0.0F, 1.0F);
      value = clampedValue;
   }

   if (dest->CondMask != COND_TR) {
      condWriteMask[0] = GET_BIT(writeMask, 0)
         && test_cc(machine->CondCodes[GET_SWZ(dest->CondSwizzle, 0)], dest->CondMask);
      condWriteMask[1] = GET_BIT(writeMask, 1)
         && test_cc(machine->CondCodes[GET_SWZ(dest->CondSwizzle, 1)], dest->CondMask);
      condWriteMask[2] = GET_BIT(writeMask, 2)
         && test_cc(machine->CondCodes[GET_SWZ(dest->CondSwizzle, 2)], dest->CondMask);
      condWriteMask[3] = GET_BIT(writeMask, 3)
         && test_cc(machine->CondCodes[GET_SWZ(dest->CondSwizzle, 3)], dest->CondMask);

      writeMask = ((condWriteMask[0] << 0) |
		   (condWriteMask[1] << 1) |
		   (condWriteMask[2] << 2) |
		   (condWriteMask[3] << 3));
   }

   if (GET_BIT(writeMask, 0)) {
      dstReg[0] = value[0];
      if (updateCC)
         machine->CondCodes[0] = generate_cc(value[0]);
   }
   if (GET_BIT(writeMask, 1)) {
      dstReg[1] = value[1];
      if (updateCC)
         machine->CondCodes[1] = generate_cc(value[1]);
   }
   if (GET_BIT(writeMask, 2)) {
      dstReg[2] = value[2];
      if (updateCC)
         machine->CondCodes[2] = generate_cc(value[2]);
   }
   if (GET_BIT(writeMask, 3)) {
      dstReg[3] = value[3];
      if (updateCC)
         machine->CondCodes[3] = generate_cc(value[3]);
   }
}


/**
 * Initialize a new machine state instance from an existing one, adding
 * the partial derivatives onto the input registers.
 * Used to implement DDX and DDY instructions in non-trivial cases.
 */
static void
init_machine_deriv( GLcontext *ctx,
                    const struct fp_machine *machine,
                    const struct fragment_program *program,
                    const struct sw_span *span, char xOrY,
                    struct fp_machine *dMachine )
{
   GLuint u;

   ASSERT(xOrY == 'X' || xOrY == 'Y');

   /* copy existing machine */
   _mesa_memcpy(dMachine, machine, sizeof(struct fp_machine));

   if (program->Base.Target == GL_FRAGMENT_PROGRAM_NV) {
      /* Clear temporary registers (undefined for ARB_f_p) */
      _mesa_bzero( (void*) machine->Temporaries,
                   MAX_NV_FRAGMENT_PROGRAM_TEMPS * 4 * sizeof(GLfloat));
   }

   /* Add derivatives */
   if (program->InputsRead & (1 << FRAG_ATTRIB_WPOS)) {
      GLfloat *wpos = (GLfloat*) machine->Inputs[FRAG_ATTRIB_WPOS];
      if (xOrY == 'X') {
         wpos[0] += 1.0F;
         wpos[1] += 0.0F;
         wpos[2] += span->dzdx;
         wpos[3] += span->dwdx;
      }
      else {
         wpos[0] += 0.0F;
         wpos[1] += 1.0F;
         wpos[2] += span->dzdy;
         wpos[3] += span->dwdy;
      }
   }
   if (program->InputsRead & (1 << FRAG_ATTRIB_COL0)) {
      GLfloat *col0 = (GLfloat*) machine->Inputs[FRAG_ATTRIB_COL0];
      if (xOrY == 'X') {
         col0[0] += span->drdx * (1.0F / CHAN_MAXF);
         col0[1] += span->dgdx * (1.0F / CHAN_MAXF);
         col0[2] += span->dbdx * (1.0F / CHAN_MAXF);
         col0[3] += span->dadx * (1.0F / CHAN_MAXF);
      }
      else {
         col0[0] += span->drdy * (1.0F / CHAN_MAXF);
         col0[1] += span->dgdy * (1.0F / CHAN_MAXF);
         col0[2] += span->dbdy * (1.0F / CHAN_MAXF);
         col0[3] += span->dady * (1.0F / CHAN_MAXF);
      }
   }
   if (program->InputsRead & (1 << FRAG_ATTRIB_COL1)) {
      GLfloat *col1 = (GLfloat*) machine->Inputs[FRAG_ATTRIB_COL1];
      if (xOrY == 'X') {
         col1[0] += span->dsrdx * (1.0F / CHAN_MAXF);
         col1[1] += span->dsgdx * (1.0F / CHAN_MAXF);
         col1[2] += span->dsbdx * (1.0F / CHAN_MAXF);
         col1[3] += 0.0; /*XXX fix */
      }
      else {
         col1[0] += span->dsrdy * (1.0F / CHAN_MAXF);
         col1[1] += span->dsgdy * (1.0F / CHAN_MAXF);
         col1[2] += span->dsbdy * (1.0F / CHAN_MAXF);
         col1[3] += 0.0; /*XXX fix */
      }
   }
   if (program->InputsRead & (1 << FRAG_ATTRIB_FOGC)) {
      GLfloat *fogc = (GLfloat*) machine->Inputs[FRAG_ATTRIB_FOGC];
      if (xOrY == 'X') {
         fogc[0] += span->dfogdx;
      }
      else {
         fogc[0] += span->dfogdy;
      }
   }
   for (u = 0; u < ctx->Const.MaxTextureCoordUnits; u++) {
      if (program->InputsRead & (1 << (FRAG_ATTRIB_TEX0 + u))) {
         GLfloat *tex = (GLfloat*) machine->Inputs[FRAG_ATTRIB_TEX0 + u];
         /* XXX perspective-correct interpolation */
         if (xOrY == 'X') {
            tex[0] += span->texStepX[u][0];
            tex[1] += span->texStepX[u][1];
            tex[2] += span->texStepX[u][2];
            tex[3] += span->texStepX[u][3];
         }
         else {
            tex[0] += span->texStepY[u][0];
            tex[1] += span->texStepY[u][1];
            tex[2] += span->texStepY[u][2];
            tex[3] += span->texStepY[u][3];
         }
      }
   }

   /* init condition codes */
   dMachine->CondCodes[0] = COND_EQ;
   dMachine->CondCodes[1] = COND_EQ;
   dMachine->CondCodes[2] = COND_EQ;
   dMachine->CondCodes[3] = COND_EQ;
}


/**
 * Execute the given vertex program.
 * NOTE: we do everything in single-precision floating point; we don't
 * currently observe the single/half/fixed-precision qualifiers.
 * \param ctx - rendering context
 * \param program - the fragment program to execute
 * \param machine - machine state (register file)
 * \param maxInst - max number of instructions to execute
 * \return GL_TRUE if program completed or GL_FALSE if program executed KIL.
 */
static GLboolean
execute_program( GLcontext *ctx,
                 const struct fragment_program *program, GLuint maxInst,
                 struct fp_machine *machine, const struct sw_span *span,
                 GLuint column )
{
   GLuint pc;

#if DEBUG_FRAG
   printf("execute fragment program --------------------\n");
#endif

   for (pc = 0; pc < maxInst; pc++) {
      const struct fp_instruction *inst = program->Instructions + pc;

      if (ctx->FragmentProgram.CallbackEnabled &&
          ctx->FragmentProgram.Callback) {
         ctx->FragmentProgram.CurrentPosition = inst->StringPos;
         ctx->FragmentProgram.Callback(program->Base.Target,
                                       ctx->FragmentProgram.CallbackData);
      }

      switch (inst->Opcode) {
         case FP_OPCODE_ABS:
            {
               GLfloat a[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] = FABSF(a[0]);
               result[1] = FABSF(a[1]);
               result[2] = FABSF(a[2]);
               result[3] = FABSF(a[3]);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_ADD:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = a[0] + b[0];
               result[1] = a[1] + b[1];
               result[2] = a[2] + b[2];
               result[3] = a[3] + b[3];
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_CMP:
            {
               GLfloat a[4], b[4], c[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               fetch_vector4( ctx, &inst->SrcReg[2], machine, program, c );
               result[0] = a[0] < 0.0F ? b[0] : c[0];
               result[1] = a[1] < 0.0F ? b[1] : c[1];
               result[2] = a[2] < 0.0F ? b[2] : c[2];
               result[3] = a[3] < 0.0F ? b[3] : c[3];
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_COS:
            {
               GLfloat a[4], result[4];
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] = result[1] = result[2] = result[3] = (GLfloat)_mesa_cos(a[0]);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_DDX: /* Partial derivative with respect to X */
            {
               GLfloat a[4], aNext[4], result[4];
               struct fp_machine dMachine;
               if (!fetch_vector4_deriv(ctx, &inst->SrcReg[0], span, 'X',
                                        column, result)) {
                  /* This is tricky.  Make a copy of the current machine state,
                   * increment the input registers by the dx or dy partial
                   * derivatives, then re-execute the program up to the
                   * preceeding instruction, then fetch the source register.
                   * Finally, find the difference in the register values for
                   * the original and derivative runs.
                   */
                  fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a);
                  init_machine_deriv(ctx, machine, program, span,
                                     'X', &dMachine);
                  execute_program(ctx, program, pc, &dMachine, span, column);
                  fetch_vector4( ctx, &inst->SrcReg[0], &dMachine, program, aNext );
                  result[0] = aNext[0] - a[0];
                  result[1] = aNext[1] - a[1];
                  result[2] = aNext[2] - a[2];
                  result[3] = aNext[3] - a[3];
               }
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_DDY: /* Partial derivative with respect to Y */
            {
               GLfloat a[4], aNext[4], result[4];
               struct fp_machine dMachine;
               if (!fetch_vector4_deriv(ctx, &inst->SrcReg[0], span, 'Y',
                                        column, result)) {
                  init_machine_deriv(ctx, machine, program, span,
                                     'Y', &dMachine);
                  fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a);
                  execute_program(ctx, program, pc, &dMachine, span, column);
                  fetch_vector4( ctx, &inst->SrcReg[0], &dMachine, program, aNext );
                  result[0] = aNext[0] - a[0];
                  result[1] = aNext[1] - a[1];
                  result[2] = aNext[2] - a[2];
                  result[3] = aNext[3] - a[3];
               }
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_DP3:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = result[1] = result[2] = result[3] =
                  a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
               store_vector4( inst, machine, result );
#if DEBUG_FRAG
               printf("DP3 %g = (%g %g %g) . (%g %g %g)\n",
                      result[0], a[0], a[1], a[2], b[0], b[1], b[2]);
#endif
            }
            break;
         case FP_OPCODE_DP4:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = result[1] = result[2] = result[3] =
                  a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
               store_vector4( inst, machine, result );
#if DEBUG_FRAG
               printf("DP4 %g = (%g, %g %g %g) . (%g, %g %g %g)\n",
                      result[0], a[0], a[1], a[2], a[3], b[0], b[1], b[2], b[3]);
#endif
            }
            break;
         case FP_OPCODE_DPH:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = result[1] = result[2] = result[3] =
                  a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + b[3];
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_DST: /* Distance vector */
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = 1.0F;
               result[1] = a[1] * b[1];
               result[2] = a[2];
               result[3] = b[3];
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_EX2: /* Exponential base 2 */
            {
               GLfloat a[4], result[4];
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] = result[1] = result[2] = result[3] =
                  (GLfloat) _mesa_pow(2.0, a[0]);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_FLR:
            {
               GLfloat a[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] = FLOORF(a[0]);
               result[1] = FLOORF(a[1]);
               result[2] = FLOORF(a[2]);
               result[3] = FLOORF(a[3]);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_FRC:
            {
               GLfloat a[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] = a[0] - FLOORF(a[0]);
               result[1] = a[1] - FLOORF(a[1]);
               result[2] = a[2] - FLOORF(a[2]);
               result[3] = a[3] - FLOORF(a[3]);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_KIL_NV: /* NV_f_p only */
            {
               const GLuint swizzle = inst->DstReg.CondSwizzle;
               const GLuint condMask = inst->DstReg.CondMask;
               if (test_cc(machine->CondCodes[GET_SWZ(swizzle, 0)], condMask) ||
                   test_cc(machine->CondCodes[GET_SWZ(swizzle, 1)], condMask) ||
                   test_cc(machine->CondCodes[GET_SWZ(swizzle, 2)], condMask) ||
                   test_cc(machine->CondCodes[GET_SWZ(swizzle, 3)], condMask)) {
                  return GL_FALSE;
               }
            }
            break;
         case FP_OPCODE_KIL: /* ARB_f_p only */
            {
               GLfloat a[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               if (a[0] < 0.0F || a[1] < 0.0F || a[2] < 0.0F || a[3] < 0.0F) {
                  return GL_FALSE;
               }
            }
            break;
         case FP_OPCODE_LG2:  /* log base 2 */
            {
               GLfloat a[4], result[4];
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] = result[1] = result[2] = result[3]
                  = LOG2(a[0]);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_LIT:
            {
               const GLfloat epsilon = 1.0F / 256.0F; /* from NV VP spec */
               GLfloat a[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               a[0] = MAX2(a[0], 0.0F);
               a[1] = MAX2(a[1], 0.0F);
               /* XXX ARB version clamps a[3], NV version doesn't */
               a[3] = CLAMP(a[3], -(128.0F - epsilon), (128.0F - epsilon));
               result[0] = 1.0F;
               result[1] = a[0];
               /* XXX we could probably just use pow() here */
               result[2] = (a[0] > 0.0F) ? (GLfloat) exp(a[3] * log(a[1])) : 0.0F;
               result[3] = 1.0F;
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_LRP:
            {
               GLfloat a[4], b[4], c[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               fetch_vector4( ctx, &inst->SrcReg[2], machine, program, c );
               result[0] = a[0] * b[0] + (1.0F - a[0]) * c[0];
               result[1] = a[1] * b[1] + (1.0F - a[1]) * c[1];
               result[2] = a[2] * b[2] + (1.0F - a[2]) * c[2];
               result[3] = a[3] * b[3] + (1.0F - a[3]) * c[3];
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_MAD:
            {
               GLfloat a[4], b[4], c[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               fetch_vector4( ctx, &inst->SrcReg[2], machine, program, c );
               result[0] = a[0] * b[0] + c[0];
               result[1] = a[1] * b[1] + c[1];
               result[2] = a[2] * b[2] + c[2];
               result[3] = a[3] * b[3] + c[3];
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_MAX:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = MAX2(a[0], b[0]);
               result[1] = MAX2(a[1], b[1]);
               result[2] = MAX2(a[2], b[2]);
               result[3] = MAX2(a[3], b[3]);
               store_vector4( inst, machine, result );
#if DEBUG_FRAG
               printf("MAX (%g %g %g %g) = (%g %g %g %g), (%g %g %g %g)\n",
                      result[0], result[1], result[2], result[3],
                      a[0], a[1], a[2], a[3],
                      b[0], b[1], b[2], b[3]);
#endif
            }
            break;
         case FP_OPCODE_MIN:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = MIN2(a[0], b[0]);
               result[1] = MIN2(a[1], b[1]);
               result[2] = MIN2(a[2], b[2]);
               result[3] = MIN2(a[3], b[3]);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_MOV:
            {
               GLfloat result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, result );
               store_vector4( inst, machine, result );
#if DEBUG_FRAG
               printf("MOV (%g %g %g %g)\n",
                      result[0], result[1], result[2], result[3]);
#endif
            }
            break;
         case FP_OPCODE_MUL:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = a[0] * b[0];
               result[1] = a[1] * b[1];
               result[2] = a[2] * b[2];
               result[3] = a[3] * b[3];
               store_vector4( inst, machine, result );
#if DEBUG_FRAG
               printf("MUL (%g %g %g %g) = (%g %g %g %g) * (%g %g %g %g)\n",
                      result[0], result[1], result[2], result[3],
                      a[0], a[1], a[2], a[3],
                      b[0], b[1], b[2], b[3]);
#endif
            }
            break;
         case FP_OPCODE_PK2H: /* pack two 16-bit floats in one 32-bit float */
            {
               GLfloat a[4], result[4];
               GLhalfNV hx, hy;
               GLuint *rawResult = (GLuint *) result;
               GLuint twoHalves;
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               hx = _mesa_float_to_half(a[0]);
               hy = _mesa_float_to_half(a[1]);
               twoHalves = hx | (hy << 16);
               rawResult[0] = rawResult[1] = rawResult[2] = rawResult[3]
                  = twoHalves;
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_PK2US: /* pack two GLushorts into one 32-bit float */
            {
               GLfloat a[4], result[4];
               GLuint usx, usy, *rawResult = (GLuint *) result;
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               a[0] = CLAMP(a[0], 0.0F, 1.0F);
               a[1] = CLAMP(a[1], 0.0F, 1.0F);
               usx = IROUND(a[0] * 65535.0F);
               usy = IROUND(a[1] * 65535.0F);
               rawResult[0] = rawResult[1] = rawResult[2] = rawResult[3]
                  = usx | (usy << 16);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_PK4B: /* pack four GLbytes into one 32-bit float */
            {
               GLfloat a[4], result[4];
               GLuint ubx, uby, ubz, ubw, *rawResult = (GLuint *) result;
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               a[0] = CLAMP(a[0], -128.0F / 127.0F, 1.0F);
               a[1] = CLAMP(a[1], -128.0F / 127.0F, 1.0F);
               a[2] = CLAMP(a[2], -128.0F / 127.0F, 1.0F);
               a[3] = CLAMP(a[3], -128.0F / 127.0F, 1.0F);
               ubx = IROUND(127.0F * a[0] + 128.0F);
               uby = IROUND(127.0F * a[1] + 128.0F);
               ubz = IROUND(127.0F * a[2] + 128.0F);
               ubw = IROUND(127.0F * a[3] + 128.0F);
               rawResult[0] = rawResult[1] = rawResult[2] = rawResult[3]
                  = ubx | (uby << 8) | (ubz << 16) | (ubw << 24);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_PK4UB: /* pack four GLubytes into one 32-bit float */
            {
               GLfloat a[4], result[4];
               GLuint ubx, uby, ubz, ubw, *rawResult = (GLuint *) result;
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               a[0] = CLAMP(a[0], 0.0F, 1.0F);
               a[1] = CLAMP(a[1], 0.0F, 1.0F);
               a[2] = CLAMP(a[2], 0.0F, 1.0F);
               a[3] = CLAMP(a[3], 0.0F, 1.0F);
               ubx = IROUND(255.0F * a[0]);
               uby = IROUND(255.0F * a[1]);
               ubz = IROUND(255.0F * a[2]);
               ubw = IROUND(255.0F * a[3]);
               rawResult[0] = rawResult[1] = rawResult[2] = rawResult[3]
                  = ubx | (uby << 8) | (ubz << 16) | (ubw << 24);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_POW:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector1( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = result[1] = result[2] = result[3]
                  = (GLfloat)_mesa_pow(a[0], b[0]);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_RCP:
            {
               GLfloat a[4], result[4];
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
#if DEBUG_FRAG
               if (a[0] == 0)
                  printf("RCP(0)\n");
               else if (IS_INF_OR_NAN(a[0]))
                  printf("RCP(inf)\n");
#endif
               result[0] = result[1] = result[2] = result[3]
                  = 1.0F / a[0];
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_RFL:
            {
               GLfloat axis[4], dir[4], result[4], tmp[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, axis );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, dir );
               tmp[3] = axis[0] * axis[0]
                      + axis[1] * axis[1]
                      + axis[2] * axis[2];
               tmp[0] = (2.0F * (axis[0] * dir[0] +
                                 axis[1] * dir[1] +
                                 axis[2] * dir[2])) / tmp[3];
               result[0] = tmp[0] * axis[0] - dir[0];
               result[1] = tmp[0] * axis[1] - dir[1];
               result[2] = tmp[0] * axis[2] - dir[2];
               /* result[3] is never written! XXX enforce in parser! */
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_RSQ: /* 1 / sqrt() */
            {
               GLfloat a[4], result[4];
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] = result[1] = result[2] = result[3] = INV_SQRTF(a[0]);
               store_vector4( inst, machine, result );
#if DEBUG_FRAG
               printf("RSQ %g = 1/sqrt(%g)\n", result[0], a[0]);
#endif
            }
            break;
         case FP_OPCODE_SCS: /* sine and cos */
            {
               GLfloat a[4], result[4];
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] = (GLfloat)cos(a[0]);
               result[1] = (GLfloat)sin(a[0]);
               result[2] = 0.0;  /* undefined! */
               result[3] = 0.0;  /* undefined! */
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_SEQ: /* set on equal */
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = (a[0] == b[0]) ? 1.0F : 0.0F;
               result[1] = (a[1] == b[1]) ? 1.0F : 0.0F;
               result[2] = (a[2] == b[2]) ? 1.0F : 0.0F;
               result[3] = (a[3] == b[3]) ? 1.0F : 0.0F;
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_SFL: /* set false, operands ignored */
            {
               static const GLfloat result[4] = { 0.0F, 0.0F, 0.0F, 0.0F };
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_SGE: /* set on greater or equal */
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = (a[0] >= b[0]) ? 1.0F : 0.0F;
               result[1] = (a[1] >= b[1]) ? 1.0F : 0.0F;
               result[2] = (a[2] >= b[2]) ? 1.0F : 0.0F;
               result[3] = (a[3] >= b[3]) ? 1.0F : 0.0F;
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_SGT: /* set on greater */
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = (a[0] > b[0]) ? 1.0F : 0.0F;
               result[1] = (a[1] > b[1]) ? 1.0F : 0.0F;
               result[2] = (a[2] > b[2]) ? 1.0F : 0.0F;
               result[3] = (a[3] > b[3]) ? 1.0F : 0.0F;
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_SIN:
            {
               GLfloat a[4], result[4];
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] = result[1] = result[2] =
		       result[3] = (GLfloat)_mesa_sin(a[0]);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_SLE: /* set on less or equal */
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = (a[0] <= b[0]) ? 1.0F : 0.0F;
               result[1] = (a[1] <= b[1]) ? 1.0F : 0.0F;
               result[2] = (a[2] <= b[2]) ? 1.0F : 0.0F;
               result[3] = (a[3] <= b[3]) ? 1.0F : 0.0F;
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_SLT: /* set on less */
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = (a[0] < b[0]) ? 1.0F : 0.0F;
               result[1] = (a[1] < b[1]) ? 1.0F : 0.0F;
               result[2] = (a[2] < b[2]) ? 1.0F : 0.0F;
               result[3] = (a[3] < b[3]) ? 1.0F : 0.0F;
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_SNE: /* set on not equal */
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = (a[0] != b[0]) ? 1.0F : 0.0F;
               result[1] = (a[1] != b[1]) ? 1.0F : 0.0F;
               result[2] = (a[2] != b[2]) ? 1.0F : 0.0F;
               result[3] = (a[3] != b[3]) ? 1.0F : 0.0F;
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_STR: /* set true, operands ignored */
            {
               static const GLfloat result[4] = { 1.0F, 1.0F, 1.0F, 1.0F };
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_SUB:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = a[0] - b[0];
               result[1] = a[1] - b[1];
               result[2] = a[2] - b[2];
               result[3] = a[3] - b[3];
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_SWZ:
            {
               const struct fp_src_register *source = &inst->SrcReg[0];
               const GLfloat *src = get_register_pointer(ctx, source,
                                                         machine, program);
               GLfloat result[4];
               GLuint i;

               /* do extended swizzling here */
               for (i = 0; i < 4; i++) {
                  if (GET_SWZ(source->Swizzle, i) == SWIZZLE_ZERO)
                     result[i] = 0.0;
                  else if (GET_SWZ(source->Swizzle, i) == SWIZZLE_ONE)
                     result[i] = 1.0;
                  else
                     result[i] = src[GET_SWZ(source->Swizzle, i)];

                  if (source->NegateBase & (1 << i))
                     result[i] = -result[i];
               }
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_TEX: /* Both ARB and NV frag prog */
            /* Texel lookup */
            {
               GLfloat texcoord[4], color[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, texcoord );
               /* Note: we pass 0 for LOD.  The ARB extension requires it
                * while the NV extension says it's implementation dependant.
                */
               /* KW: Previously lambda was passed as zero, but I
		* believe this is incorrect, the spec seems to
		* indicate rather that lambda should not be
		* changed/biased, unlike TXB where texcoord[3] is
		* added to the lambda calculations.  The lambda should
		* still be calculated normally for TEX & TXP though,
		* not set to zero.  Otherwise it's very difficult to
		* implement normal GL semantics through the fragment
		* shader.
		*/
               fetch_texel( ctx, texcoord,
			    span->array->lambda[inst->TexSrcUnit][column],
			    inst->TexSrcUnit, color );
               store_vector4( inst, machine, color );
            }
            break;
         case FP_OPCODE_TXB: /* GL_ARB_fragment_program only */
            /* Texel lookup with LOD bias */
            {
               GLfloat texcoord[4], color[4], bias, lambda;

               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, texcoord );
               /* texcoord[3] is the bias to add to lambda */
               bias = ctx->Texture.Unit[inst->TexSrcUnit].LodBias
                    + ctx->Texture.Unit[inst->TexSrcUnit]._Current->LodBias
                    + texcoord[3];
               lambda = span->array->lambda[inst->TexSrcUnit][column] + bias;
               fetch_texel( ctx, texcoord, lambda,
                            inst->TexSrcUnit, color );
               store_vector4( inst, machine, color );
            }
            break;
         case FP_OPCODE_TXD: /* GL_NV_fragment_program only */
            /* Texture lookup w/ partial derivatives for LOD */
            {
               GLfloat texcoord[4], dtdx[4], dtdy[4], color[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, texcoord );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, dtdx );
               fetch_vector4( ctx, &inst->SrcReg[2], machine, program, dtdy );
               fetch_texel_deriv( ctx, texcoord, dtdx, dtdy, inst->TexSrcUnit,
                                  color );
               store_vector4( inst, machine, color );
            }
            break;
         case FP_OPCODE_TXP: /* GL_ARB_fragment_program only */
            /* Texture lookup w/ projective divide */
            {
               GLfloat texcoord[4], color[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, texcoord );
	       /* Not so sure about this test - if texcoord[3] is
		* zero, we'd probably be fine except for an ASSERT in
		* IROUND_POS() which gets triggered by the inf values created.
		*/
	       if (texcoord[3] != 0.0) {
		  texcoord[0] /= texcoord[3];
		  texcoord[1] /= texcoord[3];
		  texcoord[2] /= texcoord[3];
	       }
               /* KW: Previously lambda was passed as zero, but I
		* believe this is incorrect, the spec seems to
		* indicate rather that lambda should not be
		* changed/biased, unlike TXB where texcoord[3] is
		* added to the lambda calculations.  The lambda should
		* still be calculated normally for TEX & TXP though,
		* not set to zero.
		*/
               fetch_texel( ctx, texcoord,
			    span->array->lambda[inst->TexSrcUnit][column],
			    inst->TexSrcUnit, color );
               store_vector4( inst, machine, color );
            }
            break;
         case FP_OPCODE_TXP_NV: /* GL_NV_fragment_program only */
            /* Texture lookup w/ projective divide */
            {
               GLfloat texcoord[4], color[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, texcoord );
               if (inst->TexSrcIdx != TEXTURE_CUBE_INDEX &&
		   texcoord[3] != 0.0) {
                  texcoord[0] /= texcoord[3];
                  texcoord[1] /= texcoord[3];
                  texcoord[2] /= texcoord[3];
               }
               fetch_texel( ctx, texcoord,
                            span->array->lambda[inst->TexSrcUnit][column],
                            inst->TexSrcUnit, color );
               store_vector4( inst, machine, color );
            }
            break;
         case FP_OPCODE_UP2H: /* unpack two 16-bit floats */
            {
               GLfloat a[4], result[4];
               const GLuint *rawBits = (const GLuint *) a;
               GLhalfNV hx, hy;
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
               hx = rawBits[0] & 0xffff;
               hy = rawBits[0] >> 16;
               result[0] = result[2] = _mesa_half_to_float(hx);
               result[1] = result[3] = _mesa_half_to_float(hy);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_UP2US: /* unpack two GLushorts */
            {
               GLfloat a[4], result[4];
               const GLuint *rawBits = (const GLuint *) a;
               GLushort usx, usy;
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
               usx = rawBits[0] & 0xffff;
               usy = rawBits[0] >> 16;
               result[0] = result[2] = usx * (1.0f / 65535.0f);
               result[1] = result[3] = usy * (1.0f / 65535.0f);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_UP4B: /* unpack four GLbytes */
            {
               GLfloat a[4], result[4];
               const GLuint *rawBits = (const GLuint *) a;
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] = (((rawBits[0] >>  0) & 0xff) - 128) / 127.0F;
               result[1] = (((rawBits[0] >>  8) & 0xff) - 128) / 127.0F;
               result[2] = (((rawBits[0] >> 16) & 0xff) - 128) / 127.0F;
               result[3] = (((rawBits[0] >> 24) & 0xff) - 128) / 127.0F;
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_UP4UB: /* unpack four GLubytes */
            {
               GLfloat a[4], result[4];
               const GLuint *rawBits = (const GLuint *) a;
               fetch_vector1( ctx, &inst->SrcReg[0], machine, program, a );
               result[0] = ((rawBits[0] >>  0) & 0xff) / 255.0F;
               result[1] = ((rawBits[0] >>  8) & 0xff) / 255.0F;
               result[2] = ((rawBits[0] >> 16) & 0xff) / 255.0F;
               result[3] = ((rawBits[0] >> 24) & 0xff) / 255.0F;
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_XPD: /* cross product */
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               result[0] = a[1] * b[2] - a[2] * b[1];
               result[1] = a[2] * b[0] - a[0] * b[2];
               result[2] = a[0] * b[1] - a[1] * b[0];
               result[3] = 1.0;
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_X2D: /* 2-D matrix transform */
            {
               GLfloat a[4], b[4], c[4], result[4];
               fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a );
               fetch_vector4( ctx, &inst->SrcReg[1], machine, program, b );
               fetch_vector4( ctx, &inst->SrcReg[2], machine, program, c );
               result[0] = a[0] + b[0] * c[0] + b[1] * c[1];
               result[1] = a[1] + b[0] * c[2] + b[1] * c[3];
               result[2] = a[2] + b[0] * c[0] + b[1] * c[1];
               result[3] = a[3] + b[0] * c[2] + b[1] * c[3];
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_PRINT:
            {
               if (inst->SrcReg[0].File != -1) {
                  GLfloat a[4];
                  fetch_vector4( ctx, &inst->SrcReg[0], machine, program, a);
                  _mesa_printf("%s%g, %g, %g, %g\n", (const char *) inst->Data,
                               a[0], a[1], a[2], a[3]);
               }
               else {
                  _mesa_printf("%s\n", (const char *) inst->Data);
               }
            }
            break;
         case FP_OPCODE_END:
            return GL_TRUE;
         default:
            _mesa_problem(ctx, "Bad opcode %d in _mesa_exec_fragment_program",
                          inst->Opcode);
            return GL_TRUE; /* return value doesn't matter */
      }
   }
   return GL_TRUE;
}


static void
init_machine( GLcontext *ctx, struct fp_machine *machine,
              const struct fragment_program *program,
              const struct sw_span *span, GLuint col )
{
   GLuint inputsRead = program->InputsRead;
   GLuint u;

   if (ctx->FragmentProgram.CallbackEnabled)
      inputsRead = ~0;

   if (program->Base.Target == GL_FRAGMENT_PROGRAM_NV) {
      /* Clear temporary registers (undefined for ARB_f_p) */
      _mesa_bzero(machine->Temporaries,
                  MAX_NV_FRAGMENT_PROGRAM_TEMPS * 4 * sizeof(GLfloat));
   }

   /* Load input registers */
   if (inputsRead & (1 << FRAG_ATTRIB_WPOS)) {
      GLfloat *wpos = machine->Inputs[FRAG_ATTRIB_WPOS];
      ASSERT(span->arrayMask & SPAN_Z);
      wpos[0] = (GLfloat) span->x + col;
      wpos[1] = (GLfloat) span->y;
      wpos[2] = (GLfloat) span->array->z[col] / ctx->DrawBuffer->_DepthMaxF;
      wpos[3] = span->w + col * span->dwdx;
   }
   if (inputsRead & (1 << FRAG_ATTRIB_COL0)) {
      GLfloat *col0 = machine->Inputs[FRAG_ATTRIB_COL0];
      ASSERT(span->arrayMask & SPAN_RGBA);
      col0[0] = CHAN_TO_FLOAT(span->array->rgba[col][RCOMP]);
      col0[1] = CHAN_TO_FLOAT(span->array->rgba[col][GCOMP]);
      col0[2] = CHAN_TO_FLOAT(span->array->rgba[col][BCOMP]);
      col0[3] = CHAN_TO_FLOAT(span->array->rgba[col][ACOMP]);
   }
   if (inputsRead & (1 << FRAG_ATTRIB_COL1)) {
      GLfloat *col1 = machine->Inputs[FRAG_ATTRIB_COL1];
      col1[0] = CHAN_TO_FLOAT(span->array->spec[col][RCOMP]);
      col1[1] = CHAN_TO_FLOAT(span->array->spec[col][GCOMP]);
      col1[2] = CHAN_TO_FLOAT(span->array->spec[col][BCOMP]);
      col1[3] = CHAN_TO_FLOAT(span->array->spec[col][ACOMP]);
   }
   if (inputsRead & (1 << FRAG_ATTRIB_FOGC)) {
      GLfloat *fogc = machine->Inputs[FRAG_ATTRIB_FOGC];
      ASSERT(span->arrayMask & SPAN_FOG);
      fogc[0] = span->array->fog[col];
      fogc[1] = 0.0F;
      fogc[2] = 0.0F;
      fogc[3] = 0.0F;
   }
   for (u = 0; u < ctx->Const.MaxTextureCoordUnits; u++) {
      if (inputsRead & (1 << (FRAG_ATTRIB_TEX0 + u))) {
         GLfloat *tex = machine->Inputs[FRAG_ATTRIB_TEX0 + u];
         /*ASSERT(ctx->Texture._EnabledCoordUnits & (1 << u));*/
         COPY_4V(tex, span->array->texcoords[u][col]);
         /*ASSERT(tex[0] != 0 || tex[1] != 0 || tex[2] != 0);*/
      }
   }

   /* init condition codes */
   machine->CondCodes[0] = COND_EQ;
   machine->CondCodes[1] = COND_EQ;
   machine->CondCodes[2] = COND_EQ;
   machine->CondCodes[3] = COND_EQ;
}



/**
 * Execute the current fragment program, operating on the given span.
 */
void
_swrast_exec_fragment_program( GLcontext *ctx, struct sw_span *span )
{
   const struct fragment_program *program = ctx->FragmentProgram._Current;
   GLuint i;

   ctx->_CurrentProgram = GL_FRAGMENT_PROGRAM_ARB; /* or NV, doesn't matter */

   if (program->Parameters) {
      _mesa_load_state_parameters(ctx, program->Parameters);
   }

   for (i = 0; i < span->end; i++) {
      if (span->array->mask[i]) {
         init_machine(ctx, &ctx->FragmentProgram.Machine,
                      ctx->FragmentProgram._Current, span, i);

#ifdef USE_TCC
         if (!_swrast_execute_codegen_program(ctx, program, ~0,
					      &ctx->FragmentProgram.Machine,
					      span, i)) {
            span->array->mask[i] = GL_FALSE;  /* killed fragment */
            span->writeAll = GL_FALSE;
         }
#else
         if (!execute_program(ctx, program, ~0,
                              &ctx->FragmentProgram.Machine, span, i)) {
            span->array->mask[i] = GL_FALSE;  /* killed fragment */
            span->writeAll = GL_FALSE;
         }
#endif

         /* Store output registers */
         {
            const GLfloat *colOut
               = ctx->FragmentProgram.Machine.Outputs[FRAG_OUTPUT_COLR];
            UNCLAMPED_FLOAT_TO_CHAN(span->array->rgba[i][RCOMP], colOut[0]);
            UNCLAMPED_FLOAT_TO_CHAN(span->array->rgba[i][GCOMP], colOut[1]);
            UNCLAMPED_FLOAT_TO_CHAN(span->array->rgba[i][BCOMP], colOut[2]);
            UNCLAMPED_FLOAT_TO_CHAN(span->array->rgba[i][ACOMP], colOut[3]);
         }
         /* depth value */
         if (program->OutputsWritten & (1 << FRAG_OUTPUT_DEPR)) {
            const GLfloat depth
               = ctx->FragmentProgram.Machine.Outputs[FRAG_OUTPUT_DEPR][2];
            span->array->z[i] = IROUND(depth * ctx->DrawBuffer->_DepthMaxF);
         }
      }
   }

   if (program->OutputsWritten & (1 << FRAG_OUTPUT_DEPR)) {
      span->interpMask &= ~SPAN_Z;
      span->arrayMask |= SPAN_Z;
   }

   ctx->_CurrentProgram = 0;
}

