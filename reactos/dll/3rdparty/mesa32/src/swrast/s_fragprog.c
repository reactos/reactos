/*
 * Mesa 3-D graphics library
 * Version:  7.0.3
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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

#include "main/glheader.h"
#include "main/colormac.h"
#include "main/context.h"
#include "main/texstate.h"
#include "shader/prog_instruction.h"

#include "s_fragprog.h"
#include "s_span.h"


/**
 * Fetch a texel with given lod.
 * Called via machine->FetchTexelLod()
 */
static void
fetch_texel_lod( GLcontext *ctx, const GLfloat texcoord[4], GLfloat lambda,
                 GLuint unit, GLfloat color[4] )
{
   const struct gl_texture_object *texObj = ctx->Texture.Unit[unit]._Current;

   if (texObj) {
      SWcontext *swrast = SWRAST_CONTEXT(ctx);
      GLchan rgba[4];

      lambda = CLAMP(lambda, texObj->MinLod, texObj->MaxLod);

      /* XXX use a float-valued TextureSample routine here!!! */
      swrast->TextureSample[unit](ctx, texObj, 1,
                                  (const GLfloat (*)[4]) texcoord,
                                  &lambda, &rgba);
      color[0] = CHAN_TO_FLOAT(rgba[0]);
      color[1] = CHAN_TO_FLOAT(rgba[1]);
      color[2] = CHAN_TO_FLOAT(rgba[2]);
      color[3] = CHAN_TO_FLOAT(rgba[3]);
   }
   else {
      color[0] = color[1] = color[2] = 0.0F;
      color[3] = 1.0F;
   }
}


/**
 * Fetch a texel with the given partial derivatives to compute a level
 * of detail in the mipmap.
 * Called via machine->FetchTexelDeriv()
 */
static void
fetch_texel_deriv( GLcontext *ctx, const GLfloat texcoord[4],
                   const GLfloat texdx[4], const GLfloat texdy[4],
                   GLfloat lodBias, GLuint unit, GLfloat color[4] )
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const struct gl_texture_object *texObj = ctx->Texture.Unit[unit]._Current;

   if (texObj) {
      const struct gl_texture_image *texImg =
         texObj->Image[0][texObj->BaseLevel];
      const GLfloat texW = (GLfloat) texImg->WidthScale;
      const GLfloat texH = (GLfloat) texImg->HeightScale;
      GLfloat lambda;
      GLchan rgba[4];

      lambda = _swrast_compute_lambda(texdx[0], texdy[0], /* ds/dx, ds/dy */
                                      texdx[1], texdy[1], /* dt/dx, dt/dy */
                                      texdx[3], texdy[2], /* dq/dx, dq/dy */
                                      texW, texH,
                                      texcoord[0], texcoord[1], texcoord[3],
                                      1.0F / texcoord[3]) + lodBias;

      lambda = CLAMP(lambda, texObj->MinLod, texObj->MaxLod);

      /* XXX use a float-valued TextureSample routine here!!! */
      swrast->TextureSample[unit](ctx, texObj, 1,
                                  (const GLfloat (*)[4]) texcoord,
                                  &lambda, &rgba);
      color[0] = CHAN_TO_FLOAT(rgba[0]);
      color[1] = CHAN_TO_FLOAT(rgba[1]);
      color[2] = CHAN_TO_FLOAT(rgba[2]);
      color[3] = CHAN_TO_FLOAT(rgba[3]);
   }
   else {
      color[0] = color[1] = color[2] = 0.0F;
      color[3] = 1.0F;
   }
}


/**
 * Initialize the virtual fragment program machine state prior to running
 * fragment program on a fragment.  This involves initializing the input
 * registers, condition codes, etc.
 * \param machine  the virtual machine state to init
 * \param program  the fragment program we're about to run
 * \param span  the span of pixels we'll operate on
 * \param col  which element (column) of the span we'll operate on
 */
static void
init_machine(GLcontext *ctx, struct gl_program_machine *machine,
             const struct gl_fragment_program *program,
             const SWspan *span, GLuint col)
{
   if (program->Base.Target == GL_FRAGMENT_PROGRAM_NV) {
      /* Clear temporary registers (undefined for ARB_f_p) */
      _mesa_bzero(machine->Temporaries,
                  MAX_PROGRAM_TEMPS * 4 * sizeof(GLfloat));
   }

   /* Setup pointer to input attributes */
   machine->Attribs = span->array->attribs;

   machine->DerivX = (GLfloat (*)[4]) span->attrStepX;
   machine->DerivY = (GLfloat (*)[4]) span->attrStepY;
   machine->NumDeriv = FRAG_ATTRIB_MAX;

   machine->Samplers = program->Base.SamplerUnits;

   /* if running a GLSL program (not ARB_fragment_program) */
   if (ctx->Shader.CurrentProgram) {
      /* Store front/back facing value in register FOGC.Y */
      machine->Attribs[FRAG_ATTRIB_FOGC][col][1] = 1.0 - span->facing;
      /* Note FOGC.ZW is gl_PointCoord if drawing a sprite */
   }

   machine->CurElement = col;

   /* init condition codes */
   machine->CondCodes[0] = COND_EQ;
   machine->CondCodes[1] = COND_EQ;
   machine->CondCodes[2] = COND_EQ;
   machine->CondCodes[3] = COND_EQ;

   /* init call stack */
   machine->StackDepth = 0;

   machine->FetchTexelLod = fetch_texel_lod;
   machine->FetchTexelDeriv = fetch_texel_deriv;
}


/**
 * Run fragment program on the pixels in span from 'start' to 'end' - 1.
 */
static void
run_program(GLcontext *ctx, SWspan *span, GLuint start, GLuint end)
{
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const struct gl_fragment_program *program = ctx->FragmentProgram._Current;
   const GLbitfield outputsWritten = program->Base.OutputsWritten;
   struct gl_program_machine *machine = &swrast->FragProgMachine;
   GLuint i;

   for (i = start; i < end; i++) {
      if (span->array->mask[i]) {
         init_machine(ctx, machine, program, span, i);

         if (_mesa_execute_program(ctx, &program->Base, machine)) {

            /* Store result color */
            if (outputsWritten & (1 << FRAG_RESULT_COLR)) {
               COPY_4V(span->array->attribs[FRAG_ATTRIB_COL0][i],
                       machine->Outputs[FRAG_RESULT_COLR]);
            }
            else {
               /* Multiple drawbuffers / render targets
                * Note that colors beyond 0 and 1 will overwrite other
                * attributes, such as FOGC, TEX0, TEX1, etc.  That's OK.
                */
               GLuint buf;
               for (buf = 0; buf < ctx->DrawBuffer->_NumColorDrawBuffers; buf++) {
                  if (outputsWritten & (1 << (FRAG_RESULT_DATA0 + buf))) {
                     COPY_4V(span->array->attribs[FRAG_ATTRIB_COL0 + buf][i],
                             machine->Outputs[FRAG_RESULT_DATA0 + buf]);
                  }
               }
            }

            /* Store result depth/z */
            if (outputsWritten & (1 << FRAG_RESULT_DEPR)) {
               const GLfloat depth = machine->Outputs[FRAG_RESULT_DEPR][2];
               if (depth <= 0.0)
                  span->array->z[i] = 0;
               else if (depth >= 1.0)
                  span->array->z[i] = ctx->DrawBuffer->_DepthMax;
               else
                  span->array->z[i] = IROUND(depth * ctx->DrawBuffer->_DepthMaxF);
            }
         }
         else {
            /* killed fragment */
            span->array->mask[i] = GL_FALSE;
            span->writeAll = GL_FALSE;
         }
      }
   }
}


/**
 * Execute the current fragment program for all the fragments
 * in the given span.
 */
void
_swrast_exec_fragment_program( GLcontext *ctx, SWspan *span )
{
   const struct gl_fragment_program *program = ctx->FragmentProgram._Current;

   /* incoming colors should be floats */
   if (program->Base.InputsRead & FRAG_BIT_COL0) {
      ASSERT(span->array->ChanType == GL_FLOAT);
   }

   ctx->_CurrentProgram = GL_FRAGMENT_PROGRAM_ARB; /* or NV, doesn't matter */

   run_program(ctx, span, 0, span->end);

   if (program->Base.OutputsWritten & (1 << FRAG_RESULT_COLR)) {
      span->interpMask &= ~SPAN_RGBA;
      span->arrayMask |= SPAN_RGBA;
   }

   if (program->Base.OutputsWritten & (1 << FRAG_RESULT_DEPR)) {
      span->interpMask &= ~SPAN_Z;
      span->arrayMask |= SPAN_Z;
   }

   ctx->_CurrentProgram = 0;
}

