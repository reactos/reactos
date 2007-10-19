/*
 * Mesa 3-D graphics library
 * Version:  6.3
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

/**
 * \file nvvertexec.c
 * Code to execute vertex programs.
 * \author Brian Paul
 */

#include "glheader.h"
#include "context.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"
#include "nvvertexec.h"
#include "nvvertprog.h"
#include "program.h"
#include "math/m_matrix.h"


static const GLfloat ZeroVec[4] = { 0.0F, 0.0F, 0.0F, 0.0F };


/**
 * Load/initialize the vertex program registers which need to be set
 * per-vertex.
 */
void
_mesa_init_vp_per_vertex_registers(GLcontext *ctx)
{
   /* Input registers get initialized from the current vertex attribs */
   MEMCPY(ctx->VertexProgram.Inputs, ctx->Current.Attrib,
          VERT_ATTRIB_MAX * 4 * sizeof(GLfloat));

   if (ctx->VertexProgram.Current->IsNVProgram) {
      GLuint i;
      /* Output/result regs are initialized to [0,0,0,1] */
      for (i = 0; i < MAX_NV_VERTEX_PROGRAM_OUTPUTS; i++) {
         ASSIGN_4V(ctx->VertexProgram.Outputs[i], 0.0F, 0.0F, 0.0F, 1.0F);
      }
      /* Temp regs are initialized to [0,0,0,0] */
      for (i = 0; i < MAX_NV_VERTEX_PROGRAM_TEMPS; i++) {
         ASSIGN_4V(ctx->VertexProgram.Temporaries[i], 0.0F, 0.0F, 0.0F, 0.0F);
      }
      ASSIGN_4V(ctx->VertexProgram.AddressReg, 0, 0, 0, 0);
   }
}



/**
 * Copy the 16 elements of a matrix into four consecutive program
 * registers starting at 'pos'.
 */
static void
load_matrix(GLfloat registers[][4], GLuint pos, const GLfloat mat[16])
{
   GLuint i;
   for (i = 0; i < 4; i++) {
      registers[pos + i][0] = mat[0 + i];
      registers[pos + i][1] = mat[4 + i];
      registers[pos + i][2] = mat[8 + i];
      registers[pos + i][3] = mat[12 + i];
   }
}


/**
 * As above, but transpose the matrix.
 */
static void
load_transpose_matrix(GLfloat registers[][4], GLuint pos,
                      const GLfloat mat[16])
{
   MEMCPY(registers[pos], mat, 16 * sizeof(GLfloat));
}


/**
 * Load program parameter registers with tracked matrices (if NV program)
 * or GL state values (if ARB program).
 * This needs to be done per glBegin/glEnd, not per-vertex.
 */
void
_mesa_init_vp_per_primitive_registers(GLcontext *ctx)
{
   if (ctx->VertexProgram.Current->IsNVProgram) {
      GLuint i;

      for (i = 0; i < MAX_NV_VERTEX_PROGRAM_PARAMS / 4; i++) {
         /* point 'mat' at source matrix */
         GLmatrix *mat;
         if (ctx->VertexProgram.TrackMatrix[i] == GL_MODELVIEW) {
            mat = ctx->ModelviewMatrixStack.Top;
         }
         else if (ctx->VertexProgram.TrackMatrix[i] == GL_PROJECTION) {
            mat = ctx->ProjectionMatrixStack.Top;
         }
         else if (ctx->VertexProgram.TrackMatrix[i] == GL_TEXTURE) {
            mat = ctx->TextureMatrixStack[ctx->Texture.CurrentUnit].Top;
         }
         else if (ctx->VertexProgram.TrackMatrix[i] == GL_COLOR) {
            mat = ctx->ColorMatrixStack.Top;
         }
         else if (ctx->VertexProgram.TrackMatrix[i]==GL_MODELVIEW_PROJECTION_NV) {
            /* XXX verify the combined matrix is up to date */
            mat = &ctx->_ModelProjectMatrix;
         }
         else if (ctx->VertexProgram.TrackMatrix[i] >= GL_MATRIX0_NV &&
                  ctx->VertexProgram.TrackMatrix[i] <= GL_MATRIX7_NV) {
            GLuint n = ctx->VertexProgram.TrackMatrix[i] - GL_MATRIX0_NV;
            ASSERT(n < MAX_PROGRAM_MATRICES);
            mat = ctx->ProgramMatrixStack[n].Top;
         }
         else {
            /* no matrix is tracked, but we leave the register values as-is */
            assert(ctx->VertexProgram.TrackMatrix[i] == GL_NONE);
            continue;
         }

         /* load the matrix */
         if (ctx->VertexProgram.TrackMatrixTransform[i] == GL_IDENTITY_NV) {
            load_matrix(ctx->VertexProgram.Parameters, i*4, mat->m);
         }
         else if (ctx->VertexProgram.TrackMatrixTransform[i] == GL_INVERSE_NV) {
            _math_matrix_analyse(mat); /* update the inverse */
            ASSERT(!_math_matrix_is_dirty(mat));
            load_matrix(ctx->VertexProgram.Parameters, i*4, mat->inv);
         }
         else if (ctx->VertexProgram.TrackMatrixTransform[i] == GL_TRANSPOSE_NV) {
            load_transpose_matrix(ctx->VertexProgram.Parameters, i*4, mat->m);
         }
         else {
            assert(ctx->VertexProgram.TrackMatrixTransform[i]
                   == GL_INVERSE_TRANSPOSE_NV);
            _math_matrix_analyse(mat); /* update the inverse */
            ASSERT(!_math_matrix_is_dirty(mat));
            load_transpose_matrix(ctx->VertexProgram.Parameters, i*4, mat->inv);
         }
      }
   }
   else {
      /* Using and ARB vertex program */
      if (ctx->VertexProgram.Current->Parameters) {
         /* Grab the state GL state and put into registers */
         _mesa_load_state_parameters(ctx,
                                     ctx->VertexProgram.Current->Parameters);
      }
   }
}



/**
 * For debugging.  Dump the current vertex program machine registers.
 */
void
_mesa_dump_vp_state( const struct gl_vertex_program_state *state )
{
   int i;
   _mesa_printf("VertexIn:\n");
   for (i = 0; i < MAX_NV_VERTEX_PROGRAM_INPUTS; i++) {
      _mesa_printf("%d: %f %f %f %f   ", i,
                   state->Inputs[i][0],
                   state->Inputs[i][1],
                   state->Inputs[i][2],
                   state->Inputs[i][3]);
   }
   _mesa_printf("\n");

   _mesa_printf("VertexOut:\n");
   for (i = 0; i < MAX_NV_VERTEX_PROGRAM_OUTPUTS; i++) {
      _mesa_printf("%d: %f %f %f %f   ", i,
                  state->Outputs[i][0],
                  state->Outputs[i][1],
                  state->Outputs[i][2],
                  state->Outputs[i][3]);
   }
   _mesa_printf("\n");

   _mesa_printf("Registers:\n");
   for (i = 0; i < MAX_NV_VERTEX_PROGRAM_TEMPS; i++) {
      _mesa_printf("%d: %f %f %f %f   ", i,
                  state->Temporaries[i][0],
                  state->Temporaries[i][1],
                  state->Temporaries[i][2],
                  state->Temporaries[i][3]);
   }
   _mesa_printf("\n");

   _mesa_printf("Parameters:\n");
   for (i = 0; i < MAX_NV_VERTEX_PROGRAM_PARAMS; i++) {
      _mesa_printf("%d: %f %f %f %f   ", i,
                  state->Parameters[i][0],
                  state->Parameters[i][1],
                  state->Parameters[i][2],
                  state->Parameters[i][3]);
   }
   _mesa_printf("\n");
}



/**
 * Return a pointer to the 4-element float vector specified by the given
 * source register.
 */
static INLINE const GLfloat *
get_register_pointer( const struct vp_src_register *source,
                      const struct gl_vertex_program_state *state )
{
   if (source->RelAddr) {
      const GLint reg = source->Index + state->AddressReg[0];
      ASSERT( (source->File == PROGRAM_ENV_PARAM) ||
        (source->File == PROGRAM_STATE_VAR) );
      if (reg < 0 || reg > MAX_NV_VERTEX_PROGRAM_PARAMS)
         return ZeroVec;
      else if (source->File == PROGRAM_ENV_PARAM)
         return state->Parameters[reg];
      else
         return state->Current->Parameters->ParameterValues[reg];
   }
   else {
      switch (source->File) {
         case PROGRAM_TEMPORARY:
            ASSERT(source->Index < MAX_NV_VERTEX_PROGRAM_TEMPS);
            return state->Temporaries[source->Index];
         case PROGRAM_INPUT:
            ASSERT(source->Index < MAX_NV_VERTEX_PROGRAM_INPUTS);
            return state->Inputs[source->Index];
         case PROGRAM_OUTPUT:
            /* This is only needed for the PRINT instruction */
            ASSERT(source->Index < MAX_NV_VERTEX_PROGRAM_OUTPUTS);
            return state->Outputs[source->Index];
         case PROGRAM_LOCAL_PARAM:
            ASSERT(source->Index < MAX_PROGRAM_LOCAL_PARAMS);
            return state->Current->Base.LocalParams[source->Index];
         case PROGRAM_ENV_PARAM:
            ASSERT(source->Index < MAX_NV_VERTEX_PROGRAM_PARAMS);
            return state->Parameters[source->Index];
         case PROGRAM_STATE_VAR:
            ASSERT(source->Index < state->Current->Parameters->NumParameters);
            return state->Current->Parameters->ParameterValues[source->Index];
         default:
            _mesa_problem(NULL,
                          "Bad source register file in get_register_pointer");
            return NULL;
      }
   }
   return NULL;
}


/**
 * Fetch a 4-element float vector from the given source register.
 * Apply swizzling and negating as needed.
 */
static INLINE void
fetch_vector4( const struct vp_src_register *source,
               const struct gl_vertex_program_state *state,
               GLfloat result[4] )
{
   const GLfloat *src = get_register_pointer(source, state);

   if (source->Negate) {
      result[0] = -src[GET_SWZ(source->Swizzle, 0)];
      result[1] = -src[GET_SWZ(source->Swizzle, 1)];
      result[2] = -src[GET_SWZ(source->Swizzle, 2)];
      result[3] = -src[GET_SWZ(source->Swizzle, 3)];
   }
   else {
      result[0] = src[GET_SWZ(source->Swizzle, 0)];
      result[1] = src[GET_SWZ(source->Swizzle, 1)];
      result[2] = src[GET_SWZ(source->Swizzle, 2)];
      result[3] = src[GET_SWZ(source->Swizzle, 3)];
   }
}



/**
 * As above, but only return result[0] element.
 */
static INLINE void
fetch_vector1( const struct vp_src_register *source,
               const struct gl_vertex_program_state *state,
               GLfloat result[4] )
{
   const GLfloat *src = get_register_pointer(source, state);

   if (source->Negate) {
      result[0] = -src[GET_SWZ(source->Swizzle, 0)];
   }
   else {
      result[0] = src[GET_SWZ(source->Swizzle, 0)];
   }
}


/**
 * Store 4 floats into a register.
 */
static void
store_vector4( const struct vp_dst_register *dest,
               struct gl_vertex_program_state *state,
               const GLfloat value[4] )
{
   GLfloat *dst;
   switch (dest->File) {
      case PROGRAM_TEMPORARY:
         dst = state->Temporaries[dest->Index];
         break;
      case PROGRAM_OUTPUT:
         dst = state->Outputs[dest->Index];
         break;
      case PROGRAM_ENV_PARAM:
         {
            /* a slight hack */
            GET_CURRENT_CONTEXT(ctx);
            dst = ctx->VertexProgram.Parameters[dest->Index];
         }
         break;
      default:
         _mesa_problem(NULL, "Invalid register file in store_vector4(file=%d)",
                       dest->File);
         return;
   }

   if (dest->WriteMask & WRITEMASK_X)
      dst[0] = value[0];
   if (dest->WriteMask & WRITEMASK_Y)
      dst[1] = value[1];
   if (dest->WriteMask & WRITEMASK_Z)
      dst[2] = value[2];
   if (dest->WriteMask & WRITEMASK_W)
      dst[3] = value[3];
}


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


/**
 * Execute the given vertex program
 */
void
_mesa_exec_vertex_program(GLcontext *ctx, const struct vertex_program *program)
{
   struct gl_vertex_program_state *state = &ctx->VertexProgram;
   const struct vp_instruction *inst;

   ctx->_CurrentProgram = GL_VERTEX_PROGRAM_ARB; /* or NV, doesn't matter */

   /* If the program is position invariant, multiply the input
    * position and the MVP matrix and stick it into the output pos slot
    */
   if (ctx->VertexProgram.Current->IsPositionInvariant) {
      TRANSFORM_POINT( ctx->VertexProgram.Outputs[0],
                       ctx->_ModelProjectMatrix.m,
                       ctx->VertexProgram.Inputs[0]);

      /* XXX: This could go elsewhere */
      ctx->VertexProgram.Current->OutputsWritten |= 0x1;
   }
   for (inst = program->Instructions; ; inst++) {

      if (ctx->VertexProgram.CallbackEnabled &&
          ctx->VertexProgram.Callback) {
         ctx->VertexProgram.CurrentPosition = inst->StringPos;
         ctx->VertexProgram.Callback(program->Base.Target,
                                     ctx->VertexProgram.CallbackData);
      }

      switch (inst->Opcode) {
         case VP_OPCODE_MOV:
            {
               GLfloat t[4];
               fetch_vector4( &inst->SrcReg[0], state, t );
               store_vector4( &inst->DstReg, state, t );
            }
            break;
         case VP_OPCODE_LIT:
            {
               const GLfloat epsilon = 1.0F / 256.0F; /* per NV spec */
               GLfloat t[4], lit[4];
               fetch_vector4( &inst->SrcReg[0], state, t );
               t[0] = MAX2(t[0], 0.0F);
               t[1] = MAX2(t[1], 0.0F);
               t[3] = CLAMP(t[3], -(128.0F - epsilon), (128.0F - epsilon));
               lit[0] = 1.0;
               lit[1] = t[0];
               lit[2] = (t[0] > 0.0) ? (GLfloat) _mesa_pow(t[1], t[3]) : 0.0F;
               lit[3] = 1.0;
               store_vector4( &inst->DstReg, state, lit );
            }
            break;
         case VP_OPCODE_RCP:
            {
               GLfloat t[4];
               fetch_vector1( &inst->SrcReg[0], state, t );
               if (t[0] != 1.0F)
                  t[0] = 1.0F / t[0];  /* div by zero is infinity! */
               t[1] = t[2] = t[3] = t[0];
               store_vector4( &inst->DstReg, state, t );
            }
            break;
         case VP_OPCODE_RSQ:
            {
               GLfloat t[4];
               fetch_vector1( &inst->SrcReg[0], state, t );
               t[0] = INV_SQRTF(FABSF(t[0]));
               t[1] = t[2] = t[3] = t[0];
               store_vector4( &inst->DstReg, state, t );
            }
            break;
         case VP_OPCODE_EXP:
            {
               GLfloat t[4], q[4], floor_t0;
               fetch_vector1( &inst->SrcReg[0], state, t );
               floor_t0 = (float) floor(t[0]);
               if (floor_t0 > FLT_MAX_EXP) {
                  SET_POS_INFINITY(q[0]);
                  SET_POS_INFINITY(q[2]);
               }
               else if (floor_t0 < FLT_MIN_EXP) {
                  q[0] = 0.0F;
                  q[2] = 0.0F;
               }
               else {
#ifdef USE_IEEE
                  GLint ii = (GLint) floor_t0;
                  ii = (ii < 23) + 0x3f800000;
                  SET_FLOAT_BITS(q[0], ii);
                  q[0] = *((GLfloat *) (void *)&ii);
#else
                  q[0] = (GLfloat) pow(2.0, floor_t0);
#endif
                  q[2] = (GLfloat) (q[0] * LOG2(q[1]));
               }
               q[1] = t[0] - floor_t0;
               q[3] = 1.0F;
               store_vector4( &inst->DstReg, state, q );
            }
            break;
         case VP_OPCODE_LOG:
            {
               GLfloat t[4], q[4], abs_t0;
               fetch_vector1( &inst->SrcReg[0], state, t );
               abs_t0 = (GLfloat) fabs(t[0]);
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
                     double mantissa = frexp(t[0], &exponent);
                     q[0] = (GLfloat) (exponent - 1);
                     q[1] = (GLfloat) (2.0 * mantissa); /* map [.5, 1) -> [1, 2) */
                     q[2] = (GLfloat) (q[0] + LOG2(q[1]));
                  }
                  }
               else {
                  SET_NEG_INFINITY(q[0]);
                  q[1] = 1.0F;
                  SET_NEG_INFINITY(q[2]);
               }
               q[3] = 1.0;
               store_vector4( &inst->DstReg, state, q );
            }
            break;
         case VP_OPCODE_MUL:
            {
               GLfloat t[4], u[4], prod[4];
               fetch_vector4( &inst->SrcReg[0], state, t );
               fetch_vector4( &inst->SrcReg[1], state, u );
               prod[0] = t[0] * u[0];
               prod[1] = t[1] * u[1];
               prod[2] = t[2] * u[2];
               prod[3] = t[3] * u[3];
               store_vector4( &inst->DstReg, state, prod );
            }
            break;
         case VP_OPCODE_ADD:
            {
               GLfloat t[4], u[4], sum[4];
               fetch_vector4( &inst->SrcReg[0], state, t );
               fetch_vector4( &inst->SrcReg[1], state, u );
               sum[0] = t[0] + u[0];
               sum[1] = t[1] + u[1];
               sum[2] = t[2] + u[2];
               sum[3] = t[3] + u[3];
               store_vector4( &inst->DstReg, state, sum );
            }
            break;
         case VP_OPCODE_DP3:
            {
               GLfloat t[4], u[4], dot[4];
               fetch_vector4( &inst->SrcReg[0], state, t );
               fetch_vector4( &inst->SrcReg[1], state, u );
               dot[0] = t[0] * u[0] + t[1] * u[1] + t[2] * u[2];
               dot[1] = dot[2] = dot[3] = dot[0];
               store_vector4( &inst->DstReg, state, dot );
            }
            break;
         case VP_OPCODE_DP4:
            {
               GLfloat t[4], u[4], dot[4];
               fetch_vector4( &inst->SrcReg[0], state, t );
               fetch_vector4( &inst->SrcReg[1], state, u );
               dot[0] = t[0] * u[0] + t[1] * u[1] + t[2] * u[2] + t[3] * u[3];
               dot[1] = dot[2] = dot[3] = dot[0];
               store_vector4( &inst->DstReg, state, dot );
            }
            break;
         case VP_OPCODE_DST:
            {
               GLfloat t[4], u[4], dst[4];
               fetch_vector4( &inst->SrcReg[0], state, t );
               fetch_vector4( &inst->SrcReg[1], state, u );
               dst[0] = 1.0F;
               dst[1] = t[1] * u[1];
               dst[2] = t[2];
               dst[3] = u[3];
               store_vector4( &inst->DstReg, state, dst );
            }
            break;
         case VP_OPCODE_MIN:
            {
               GLfloat t[4], u[4], min[4];
               fetch_vector4( &inst->SrcReg[0], state, t );
               fetch_vector4( &inst->SrcReg[1], state, u );
               min[0] = (t[0] < u[0]) ? t[0] : u[0];
               min[1] = (t[1] < u[1]) ? t[1] : u[1];
               min[2] = (t[2] < u[2]) ? t[2] : u[2];
               min[3] = (t[3] < u[3]) ? t[3] : u[3];
               store_vector4( &inst->DstReg, state, min );
            }
            break;
         case VP_OPCODE_MAX:
            {
               GLfloat t[4], u[4], max[4];
               fetch_vector4( &inst->SrcReg[0], state, t );
               fetch_vector4( &inst->SrcReg[1], state, u );
               max[0] = (t[0] > u[0]) ? t[0] : u[0];
               max[1] = (t[1] > u[1]) ? t[1] : u[1];
               max[2] = (t[2] > u[2]) ? t[2] : u[2];
               max[3] = (t[3] > u[3]) ? t[3] : u[3];
               store_vector4( &inst->DstReg, state, max );
            }
            break;
         case VP_OPCODE_SLT:
            {
               GLfloat t[4], u[4], slt[4];
               fetch_vector4( &inst->SrcReg[0], state, t );
               fetch_vector4( &inst->SrcReg[1], state, u );
               slt[0] = (t[0] < u[0]) ? 1.0F : 0.0F;
               slt[1] = (t[1] < u[1]) ? 1.0F : 0.0F;
               slt[2] = (t[2] < u[2]) ? 1.0F : 0.0F;
               slt[3] = (t[3] < u[3]) ? 1.0F : 0.0F;
               store_vector4( &inst->DstReg, state, slt );
            }
            break;
         case VP_OPCODE_SGE:
            {
               GLfloat t[4], u[4], sge[4];
               fetch_vector4( &inst->SrcReg[0], state, t );
               fetch_vector4( &inst->SrcReg[1], state, u );
               sge[0] = (t[0] >= u[0]) ? 1.0F : 0.0F;
               sge[1] = (t[1] >= u[1]) ? 1.0F : 0.0F;
               sge[2] = (t[2] >= u[2]) ? 1.0F : 0.0F;
               sge[3] = (t[3] >= u[3]) ? 1.0F : 0.0F;
               store_vector4( &inst->DstReg, state, sge );
            }
            break;
         case VP_OPCODE_MAD:
            {
               GLfloat t[4], u[4], v[4], sum[4];
               fetch_vector4( &inst->SrcReg[0], state, t );
               fetch_vector4( &inst->SrcReg[1], state, u );
               fetch_vector4( &inst->SrcReg[2], state, v );
               sum[0] = t[0] * u[0] + v[0];
               sum[1] = t[1] * u[1] + v[1];
               sum[2] = t[2] * u[2] + v[2];
               sum[3] = t[3] * u[3] + v[3];
               store_vector4( &inst->DstReg, state, sum );
            }
            break;
         case VP_OPCODE_ARL:
            {
               GLfloat t[4];
               fetch_vector4( &inst->SrcReg[0], state, t );
               state->AddressReg[0] = (GLint) floor(t[0]);
            }
            break;
         case VP_OPCODE_DPH:
            {
               GLfloat t[4], u[4], dot[4];
               fetch_vector4( &inst->SrcReg[0], state, t );
               fetch_vector4( &inst->SrcReg[1], state, u );
               dot[0] = t[0] * u[0] + t[1] * u[1] + t[2] * u[2] + u[3];
               dot[1] = dot[2] = dot[3] = dot[0];
               store_vector4( &inst->DstReg, state, dot );
            }
            break;
         case VP_OPCODE_RCC:
            {
               GLfloat t[4], u;
               fetch_vector1( &inst->SrcReg[0], state, t );
               if (t[0] == 1.0F)
                  u = 1.0F;
               else
                  u = 1.0F / t[0];
               if (u > 0.0F) {
                  if (u > 1.884467e+019F) {
                     u = 1.884467e+019F;  /* IEEE 32-bit binary value 0x5F800000 */
                  }
                  else if (u < 5.42101e-020F) {
                     u = 5.42101e-020F;   /* IEEE 32-bit binary value 0x1F800000 */
                  }
               }
               else {
                  if (u < -1.884467e+019F) {
                     u = -1.884467e+019F; /* IEEE 32-bit binary value 0xDF800000 */
                  }
                  else if (u > -5.42101e-020F) {
                     u = -5.42101e-020F;  /* IEEE 32-bit binary value 0x9F800000 */
                  }
               }
               t[0] = t[1] = t[2] = t[3] = u;
               store_vector4( &inst->DstReg, state, t );
            }
            break;
         case VP_OPCODE_SUB: /* GL_NV_vertex_program1_1 */
            {
               GLfloat t[4], u[4], sum[4];
               fetch_vector4( &inst->SrcReg[0], state, t );
               fetch_vector4( &inst->SrcReg[1], state, u );
               sum[0] = t[0] - u[0];
               sum[1] = t[1] - u[1];
               sum[2] = t[2] - u[2];
               sum[3] = t[3] - u[3];
               store_vector4( &inst->DstReg, state, sum );
            }
            break;
         case VP_OPCODE_ABS: /* GL_NV_vertex_program1_1 */
            {
               GLfloat t[4];
               fetch_vector4( &inst->SrcReg[0], state, t );
               if (t[0] < 0.0)  t[0] = -t[0];
               if (t[1] < 0.0)  t[1] = -t[1];
               if (t[2] < 0.0)  t[2] = -t[2];
               if (t[3] < 0.0)  t[3] = -t[3];
               store_vector4( &inst->DstReg, state, t );
            }
            break;
         case VP_OPCODE_FLR: /* GL_ARB_vertex_program */
            {
               GLfloat t[4];
               fetch_vector4( &inst->SrcReg[0], state, t );
               t[0] = FLOORF(t[0]);
               t[1] = FLOORF(t[1]);
               t[2] = FLOORF(t[2]);
               t[3] = FLOORF(t[3]);
               store_vector4( &inst->DstReg, state, t );
            }
            break;
         case VP_OPCODE_FRC: /* GL_ARB_vertex_program */
            {
               GLfloat t[4];
               fetch_vector4( &inst->SrcReg[0], state, t );
               t[0] = t[0] - FLOORF(t[0]);
               t[1] = t[1] - FLOORF(t[1]);
               t[2] = t[2] - FLOORF(t[2]);
               t[3] = t[3] - FLOORF(t[3]);
               store_vector4( &inst->DstReg, state, t );
            }
            break;
         case VP_OPCODE_EX2: /* GL_ARB_vertex_program */
            {
               GLfloat t[4];
               fetch_vector1( &inst->SrcReg[0], state, t );
               t[0] = t[1] = t[2] = t[3] = (GLfloat)_mesa_pow(2.0, t[0]);
               store_vector4( &inst->DstReg, state, t );
            }
            break;
         case VP_OPCODE_LG2: /* GL_ARB_vertex_program */
            {
               GLfloat t[4];
               fetch_vector1( &inst->SrcReg[0], state, t );
               t[0] = t[1] = t[2] = t[3] = LOG2(t[0]);
               store_vector4( &inst->DstReg, state, t );
            }
            break;
         case VP_OPCODE_POW: /* GL_ARB_vertex_program */
            {
               GLfloat t[4], u[4];
               fetch_vector1( &inst->SrcReg[0], state, t );
               fetch_vector1( &inst->SrcReg[1], state, u );
               t[0] = t[1] = t[2] = t[3] = (GLfloat)_mesa_pow(t[0], u[0]);
               store_vector4( &inst->DstReg, state, t );
            }
            break;
         case VP_OPCODE_XPD: /* GL_ARB_vertex_program */
            {
               GLfloat t[4], u[4], cross[4];
               fetch_vector4( &inst->SrcReg[0], state, t );
               fetch_vector4( &inst->SrcReg[1], state, u );
               cross[0] = t[1] * u[2] - t[2] * u[1];
               cross[1] = t[2] * u[0] - t[0] * u[2];
               cross[2] = t[0] * u[1] - t[1] * u[0];
               store_vector4( &inst->DstReg, state, cross );
            }
            break;
         case VP_OPCODE_SWZ: /* GL_ARB_vertex_program */
            {
               const struct vp_src_register *source = &inst->SrcReg[0];
               const GLfloat *src = get_register_pointer(source, state);
               GLfloat result[4];
               GLuint i;

               /* do extended swizzling here */
               for (i = 0; i < 3; i++) {
                  if (GET_SWZ(source->Swizzle, i) == SWIZZLE_ZERO)
                     result[i] = 0.0;
                  else if (GET_SWZ(source->Swizzle, i) == SWIZZLE_ONE)
                     result[i] = -1.0;
                  else
                     result[i] = -src[GET_SWZ(source->Swizzle, i)];
                  if (source->Negate)
                     result[i] = -result[i];
               }
               store_vector4( &inst->DstReg, state, result );
            }
            break;
         case VP_OPCODE_PRINT:
            if (inst->SrcReg[0].File) {
               GLfloat t[4];
               fetch_vector4( &inst->SrcReg[0], state, t );
               _mesa_printf("%s%g, %g, %g, %g\n",
                            (char *) inst->Data, t[0], t[1], t[2], t[3]);
            }
            else {
               _mesa_printf("%s\n", (char *) inst->Data);
            }
            break;
         case VP_OPCODE_END:
            ctx->_CurrentProgram = 0;
            return;
         default:
            /* bad instruction opcode */
            _mesa_problem(ctx, "Bad VP Opcode in _mesa_exec_vertex_program");
            ctx->_CurrentProgram = 0;
            return;
      } /* switch */
   } /* for */

   ctx->_CurrentProgram = 0;
}



/**
Thoughts on vertex program optimization:

The obvious thing to do is to compile the vertex program into X86/SSE/3DNow!
assembly code.  That will probably be a lot of work.

Another approach might be to replace the vp_instruction->Opcode field with
a pointer to a specialized C function which executes the instruction.
In particular we can write functions which skip swizzling, negating,
masking, relative addressing, etc. when they're not needed.

For example:

void simple_add( struct vp_instruction *inst )
{
   GLfloat *sum = machine->Registers[inst->DstReg.Register];
   GLfloat *a = machine->Registers[inst->SrcReg[0].Register];
   GLfloat *b = machine->Registers[inst->SrcReg[1].Register];
   sum[0] = a[0] + b[0];
   sum[1] = a[1] + b[1];
   sum[2] = a[2] + b[2];
   sum[3] = a[3] + b[3];
}

*/

/*

KW:

A first step would be to 'vectorize' the programs in the same way as
the normal transformation code in the tnl module.  Thus each opcode
takes zero or more input vectors (registers) and produces one or more
output vectors.

These operations would intially be coded in C, with machine-specific
assembly following, as is currently the case for matrix
transformations in the math/ directory.  The preprocessing scheme for
selecting simpler operations Brian describes above would also work
here.

This should give reasonable performance without excessive effort.

*/
