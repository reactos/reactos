/*
 * Mesa 3-D graphics library
 * Version:  7.1
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

#define DEBUG_PARSING 0

/**
 * \file arbprogparse.c
 * ARB_*_program parser core
 * \author Karl Rasche
 */

/**
Notes on program parameters, etc.

The instructions we emit will use six kinds of source registers:

  PROGRAM_INPUT      - input registers
  PROGRAM_TEMPORARY  - temp registers
  PROGRAM_ADDRESS    - address/indirect register
  PROGRAM_SAMPLER    - texture sampler
  PROGRAM_CONSTANT   - indexes into program->Parameters, a known constant/literal
  PROGRAM_STATE_VAR  - indexes into program->Parameters, and may actually be:
                       + a state variable, like "state.fog.color", or
                       + a pointer to a "program.local[k]" parameter, or
                       + a pointer to a "program.env[k]" parameter

Basically, all the program.local[] and program.env[] values will get mapped
into the unified gl_program->Parameters array.  This solves the problem of
having three separate program parameter arrays.
*/


#include "main/glheader.h"
#include "main/imports.h"
#include "main/context.h"
#include "main/mtypes.h"
#include "arbprogparse.h"
#include "programopt.h"
#include "prog_parameter.h"
#include "prog_statevars.h"
#include "prog_instruction.h"
#include "program_parser.h"


void
_mesa_parse_arb_fragment_program(struct gl_context* ctx, GLenum target,
                                 const GLvoid *str, GLsizei len,
                                 struct gl_fragment_program *program)
{
   struct gl_program prog;
   struct asm_parser_state state;
   GLuint i;

   ASSERT(target == GL_FRAGMENT_PROGRAM_ARB);

   memset(&prog, 0, sizeof(prog));
   memset(&state, 0, sizeof(state));
   state.prog = &prog;

   if (!_mesa_parse_arb_program(ctx, target, (const GLubyte*) str, len,
				&state)) {
      /* Error in the program. Just return. */
      return;
   }

   if (program->Base.String != NULL)
      free(program->Base.String);

   /* Copy the relevant contents of the arb_program struct into the
    * fragment_program struct.
    */
   program->Base.String          = prog.String;
   program->Base.NumInstructions = prog.NumInstructions;
   program->Base.NumTemporaries  = prog.NumTemporaries;
   program->Base.NumParameters   = prog.NumParameters;
   program->Base.NumAttributes   = prog.NumAttributes;
   program->Base.NumAddressRegs  = prog.NumAddressRegs;
   program->Base.NumNativeInstructions = prog.NumNativeInstructions;
   program->Base.NumNativeTemporaries = prog.NumNativeTemporaries;
   program->Base.NumNativeParameters = prog.NumNativeParameters;
   program->Base.NumNativeAttributes = prog.NumNativeAttributes;
   program->Base.NumNativeAddressRegs = prog.NumNativeAddressRegs;
   program->Base.NumAluInstructions   = prog.NumAluInstructions;
   program->Base.NumTexInstructions   = prog.NumTexInstructions;
   program->Base.NumTexIndirections   = prog.NumTexIndirections;
   program->Base.NumNativeAluInstructions = prog.NumAluInstructions;
   program->Base.NumNativeTexInstructions = prog.NumTexInstructions;
   program->Base.NumNativeTexIndirections = prog.NumTexIndirections;
   program->Base.InputsRead      = prog.InputsRead;
   program->Base.OutputsWritten  = prog.OutputsWritten;
   program->Base.IndirectRegisterFiles = prog.IndirectRegisterFiles;
   for (i = 0; i < MAX_TEXTURE_IMAGE_UNITS; i++) {
      program->Base.TexturesUsed[i] = prog.TexturesUsed[i];
      if (prog.TexturesUsed[i])
         program->Base.SamplersUsed |= (1 << i);
   }
   program->Base.ShadowSamplers = prog.ShadowSamplers;
   program->OriginUpperLeft = state.option.OriginUpperLeft;
   program->PixelCenterInteger = state.option.PixelCenterInteger;

   program->UsesKill            = state.fragment.UsesKill;

   if (program->Base.Instructions)
      free(program->Base.Instructions);
   program->Base.Instructions = prog.Instructions;

   if (program->Base.Parameters)
      _mesa_free_parameter_list(program->Base.Parameters);
   program->Base.Parameters    = prog.Parameters;

   /* Append fog instructions now if the program has "OPTION ARB_fog_exp"
    * or similar.  We used to leave this up to drivers, but it appears
    * there's no hardware that wants to do fog in a discrete stage separate
    * from the fragment shader.
    */
   if (state.option.Fog != OPTION_NONE) {
      static const GLenum fog_modes[4] = {
	 GL_NONE, GL_EXP, GL_EXP2, GL_LINEAR
      };

      /* XXX: we should somehow recompile this to remove clamping if disabled
       * On the ATI driver, this is unclampled if fragment clamping is disabled
       */
      _mesa_append_fog_code(ctx, program, fog_modes[state.option.Fog], GL_TRUE);
   }

#if DEBUG_FP
   printf("____________Fragment program %u ________\n", program->Base.Id);
   _mesa_print_program(&program->Base);
#endif
}



/**
 * Parse the vertex program string.  If success, update the given
 * vertex_program object with the new program.  Else, leave the vertex_program
 * object unchanged.
 */
void
_mesa_parse_arb_vertex_program(struct gl_context *ctx, GLenum target,
			       const GLvoid *str, GLsizei len,
			       struct gl_vertex_program *program)
{
   struct gl_program prog;
   struct asm_parser_state state;

   ASSERT(target == GL_VERTEX_PROGRAM_ARB);

   memset(&prog, 0, sizeof(prog));
   memset(&state, 0, sizeof(state));
   state.prog = &prog;

   if (!_mesa_parse_arb_program(ctx, target, (const GLubyte*) str, len,
				&state)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glProgramString(bad program)");
      return;
   }

   if (program->Base.String != NULL)
      free(program->Base.String);

   /* Copy the relevant contents of the arb_program struct into the 
    * vertex_program struct.
    */
   program->Base.String          = prog.String;
   program->Base.NumInstructions = prog.NumInstructions;
   program->Base.NumTemporaries  = prog.NumTemporaries;
   program->Base.NumParameters   = prog.NumParameters;
   program->Base.NumAttributes   = prog.NumAttributes;
   program->Base.NumAddressRegs  = prog.NumAddressRegs;
   program->Base.NumNativeInstructions = prog.NumNativeInstructions;
   program->Base.NumNativeTemporaries = prog.NumNativeTemporaries;
   program->Base.NumNativeParameters = prog.NumNativeParameters;
   program->Base.NumNativeAttributes = prog.NumNativeAttributes;
   program->Base.NumNativeAddressRegs = prog.NumNativeAddressRegs;
   program->Base.InputsRead     = prog.InputsRead;
   program->Base.OutputsWritten = prog.OutputsWritten;
   program->Base.IndirectRegisterFiles = prog.IndirectRegisterFiles;
   program->IsPositionInvariant = (state.option.PositionInvariant)
      ? GL_TRUE : GL_FALSE;

   if (program->Base.Instructions)
      free(program->Base.Instructions);
   program->Base.Instructions = prog.Instructions;

   if (program->Base.Parameters)
      _mesa_free_parameter_list(program->Base.Parameters);
   program->Base.Parameters = prog.Parameters; 

#if DEBUG_VP
   printf("____________Vertex program %u __________\n", program->Base.Id);
   _mesa_print_program(&program->Base);
#endif
}
