/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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

#define DEBUG_FP 0

/**
 * \file arbfragparse.c
 * ARB_fragment_program parser.
 * \author Karl Rasche
 */

#include "glheader.h"
#include "context.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"
#include "arbparse.h"
#include "arbfragparse.h"

static void
debug_fp_inst(GLint num, struct fp_instruction *fp)
{
   GLint a;
 
   fprintf(stderr, "PROGRAM_OUTPUT: 0x%x\n",    PROGRAM_OUTPUT);
   fprintf(stderr, "PROGRAM_INPUT: 0x%x\n",     PROGRAM_INPUT);
   fprintf(stderr, "PROGRAM_TEMPORARY: 0x%x\n", PROGRAM_TEMPORARY);

   for (a=0; a<num; a++) {
      switch (fp[a].Opcode) {
         case FP_OPCODE_END:
            fprintf(stderr, "FP_OPCODE_END"); break;

         case  FP_OPCODE_ABS:
            fprintf(stderr, "FP_OPCODE_ABS"); break;

         case  FP_OPCODE_ADD:
            fprintf(stderr, "FP_OPCODE_ADD"); break;

         case  FP_OPCODE_CMP:
            fprintf(stderr, "FP_OPCODE_CMP"); break;

         case  FP_OPCODE_COS:
            fprintf(stderr, "FP_OPCODE_COS"); break;

         case  FP_OPCODE_DP3:
            fprintf(stderr, "FP_OPCODE_DP3"); break;

         case  FP_OPCODE_DP4:
            fprintf(stderr, "FP_OPCODE_DP4"); break;

         case  FP_OPCODE_DPH:
            fprintf(stderr, "FP_OPCODE_DPH"); break;

         case  FP_OPCODE_DST:
            fprintf(stderr, "FP_OPCODE_DST"); break;

         case  FP_OPCODE_EX2:
            fprintf(stderr, "FP_OPCODE_EX2"); break;

         case  FP_OPCODE_FLR:
            fprintf(stderr, "FP_OPCODE_FLR"); break;

         case  FP_OPCODE_FRC:
            fprintf(stderr, "FP_OPCODE_FRC"); break;

         case  FP_OPCODE_KIL:
            fprintf(stderr, "FP_OPCODE_KIL"); break;

         case  FP_OPCODE_LG2:
            fprintf(stderr, "FP_OPCODE_LG2"); break;

         case  FP_OPCODE_LIT:
            fprintf(stderr, "FP_OPCODE_LIT"); break;

         case  FP_OPCODE_LRP:
            fprintf(stderr, "FP_OPCODE_LRP"); break;

         case  FP_OPCODE_MAD:
            fprintf(stderr, "FP_OPCODE_MAD"); break;

         case  FP_OPCODE_MAX:
            fprintf(stderr, "FP_OPCODE_MAX"); break;

         case  FP_OPCODE_MIN:
            fprintf(stderr, "FP_OPCODE_MIN"); break;

         case  FP_OPCODE_MOV:
            fprintf(stderr, "FP_OPCODE_MOV"); break;

         case  FP_OPCODE_MUL:
            fprintf(stderr, "FP_OPCODE_MUL"); break;

         case  FP_OPCODE_POW:
            fprintf(stderr, "FP_OPCODE_POW"); break;

         case  FP_OPCODE_RCP:
            fprintf(stderr, "FP_OPCODE_RCP"); break;

         case  FP_OPCODE_RSQ:
            fprintf(stderr, "FP_OPCODE_RSQ"); break;

         case  FP_OPCODE_SCS:
            fprintf(stderr, "FP_OPCODE_SCS"); break;

         case  FP_OPCODE_SIN:
            fprintf(stderr, "FP_OPCODE_SIN"); break;

         case  FP_OPCODE_SLT:
            fprintf(stderr, "FP_OPCODE_SLT"); break;

         case  FP_OPCODE_SUB:
            fprintf(stderr, "FP_OPCODE_SUB"); break;

         case  FP_OPCODE_SWZ:
            fprintf(stderr, "FP_OPCODE_SWZ"); break;

         case  FP_OPCODE_TEX:
            fprintf(stderr, "FP_OPCODE_TEX"); break;

         case  FP_OPCODE_TXB:
            fprintf(stderr, "FP_OPCODE_TXB"); break;

         case  FP_OPCODE_TXP:
            fprintf(stderr, "FP_OPCODE_TXP"); break;

         case  FP_OPCODE_XPD:
            fprintf(stderr, "FP_OPCODE_XPD"); break;

         default:
            _mesa_warning(NULL, "Bad opcode in debug_fg_inst()");
      }

      fprintf(stderr, " D(0x%x:%d:%d%d%d%d) ", 
         fp[a].DstReg.File, fp[a].DstReg.Index,
         fp[a].DstReg.WriteMask[0], fp[a].DstReg.WriteMask[1], 
         fp[a].DstReg.WriteMask[2], fp[a].DstReg.WriteMask[3]); 
						 
      fprintf(stderr, "S1(0x%x:%d:%d%d%d%d) ", fp[a].SrcReg[0].File, fp[a].SrcReg[0].Index,
         fp[a].SrcReg[0].Swizzle[0],
         fp[a].SrcReg[0].Swizzle[1],
         fp[a].SrcReg[0].Swizzle[2],
         fp[a].SrcReg[0].Swizzle[3]);

      fprintf(stderr, "S2(0x%x:%d:%d%d%d%d) ", fp[a].SrcReg[1].File, fp[a].SrcReg[1].Index,
        fp[a].SrcReg[1].Swizzle[0],
        fp[a].SrcReg[1].Swizzle[1],
        fp[a].SrcReg[1].Swizzle[2],
        fp[a].SrcReg[1].Swizzle[3]);

      fprintf(stderr, "S3(0x%x:%d:%d%d%d%d)",  fp[a].SrcReg[2].File, fp[a].SrcReg[2].Index,
        fp[a].SrcReg[2].Swizzle[0],
        fp[a].SrcReg[2].Swizzle[1],
        fp[a].SrcReg[2].Swizzle[2],
        fp[a].SrcReg[2].Swizzle[3]);

      fprintf(stderr, "\n");
   }
}

void
_mesa_parse_arb_fragment_program(GLcontext * ctx, GLenum target,
                                 const GLubyte * str, GLsizei len,
                                 struct fragment_program *program)
{
   GLuint a, retval;
   struct arb_program ap;
	
   retval = _mesa_parse_arb_program(ctx, str, len, &ap);

   /* XXX: Parse error. Cleanup things and return */	
   if (retval)
   {
      program->Instructions = (struct fp_instruction *)
         _mesa_malloc(sizeof(struct fp_instruction));
      program->Instructions[0].Opcode = FP_OPCODE_END;
      return;
   }

   /* XXX: Eh.. we parsed something that wasn't a fragment program. doh! */
   if (ap.type != GL_FRAGMENT_PROGRAM_ARB)
   {
      program->Instructions = (struct fp_instruction *)
         _mesa_malloc (sizeof(struct fp_instruction) );
      program->Instructions[0].Opcode = FP_OPCODE_END;
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "Parsed a non-fragment program as a fragment program");
      return;      
   }

   /* copy the relvant contents of the arb_program struct into the 
    * fragment_program struct
    */
   program->Base.String          = ap.Base.String;
   program->Base.NumInstructions = ap.Base.NumInstructions;
   program->Base.NumTemporaries  = ap.Base.NumTemporaries;
   program->Base.NumParameters   = ap.Base.NumParameters;
   program->Base.NumAttributes   = ap.Base.NumAttributes;
   program->Base.NumAddressRegs  = ap.Base.NumAddressRegs;
	
   program->InputsRead     = ap.InputsRead;
   program->OutputsWritten = ap.OutputsWritten;
   for (a=0; a<MAX_TEXTURE_IMAGE_UNITS; a++)
      program->TexturesUsed[a] = ap.TexturesUsed[a];
   program->NumAluInstructions = ap.NumAluInstructions;
   program->NumTexInstructions = ap.NumTexInstructions;
   program->NumTexIndirections = ap.NumTexIndirections;
   program->Parameters         = ap.Parameters;

#if DEBUG_FP
   debug_fp_inst(ap.Base.NumInstructions, ap.FPInstructions);
#else
   (void) debug_fp_inst;
#endif

   program->Instructions   = ap.FPInstructions;
}
