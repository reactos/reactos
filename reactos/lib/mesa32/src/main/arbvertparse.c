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

#define DEBUG_VP 0

/**
 * \file arbvertparse.c
 * ARB_vertex_program parser.
 * \author Karl Rasche
 */

#include "glheader.h"
#include "context.h"
#include "arbvertparse.h"
#include "hash.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"
#include "nvprogram.h"
#include "nvvertparse.h"
#include "nvvertprog.h"

#include "arbparse.h"


static GLvoid
debug_vp_inst(GLint num, struct vp_instruction *vp)
{
   GLint a;

   for (a=0; a<num; a++) {
      switch (vp[a].Opcode) {
         case VP_OPCODE_MOV:
            fprintf(stderr, "VP_OPCODE_MOV"); break;

         case VP_OPCODE_LIT:
            fprintf(stderr, "VP_OPCODE_LIT"); break;

         case VP_OPCODE_RCP:
            fprintf(stderr, "VP_OPCODE_RCP"); break;

         case VP_OPCODE_RSQ:
            fprintf(stderr, "VP_OPCODE_RSQ"); break;

         case VP_OPCODE_EXP:
            fprintf(stderr, "VP_OPCODE_EXP"); break;

         case VP_OPCODE_LOG:
            fprintf(stderr, "VP_OPCODE_LOG"); break;

         case VP_OPCODE_MUL:
            fprintf(stderr, "VP_OPCODE_MUL"); break;

         case VP_OPCODE_ADD:
            fprintf(stderr, "VP_OPCODE_ADD"); break;
				
         case VP_OPCODE_DP3:
            fprintf(stderr, "VP_OPCODE_DP3"); break;

         case VP_OPCODE_DP4:
            fprintf(stderr, "VP_OPCODE_DP4"); break;

         case VP_OPCODE_DST:
            fprintf(stderr, "VP_OPCODE_DST"); break;

         case VP_OPCODE_MIN:
            fprintf(stderr, "VP_OPCODE_MIN"); break;

         case VP_OPCODE_MAX:
            fprintf(stderr, "VP_OPCODE_MAX"); break;

         case VP_OPCODE_SLT:
            fprintf(stderr, "VP_OPCODE_SLT"); break;

         case VP_OPCODE_SGE:
            fprintf(stderr, "VP_OPCODE_SGE"); break;

         case VP_OPCODE_MAD:
            fprintf(stderr, "VP_OPCODE_MAD"); break;

         case VP_OPCODE_ARL:
            fprintf(stderr, "VP_OPCODE_ARL"); break;

         case VP_OPCODE_DPH:
            fprintf(stderr, "VP_OPCODE_DPH"); break;

         case VP_OPCODE_RCC:
            fprintf(stderr, "VP_OPCODE_RCC"); break;

         case VP_OPCODE_SUB:
            fprintf(stderr, "VP_OPCODE_SUB"); break;

         case VP_OPCODE_ABS:
            fprintf(stderr, "VP_OPCODE_ABS"); break;

         case VP_OPCODE_FLR:
            fprintf(stderr, "VP_OPCODE_FLR"); break;

         case VP_OPCODE_FRC:
            fprintf(stderr, "VP_OPCODE_FRC"); break;

         case VP_OPCODE_EX2:
            fprintf(stderr, "VP_OPCODE_EX2"); break;

         case VP_OPCODE_LG2:
            fprintf(stderr, "VP_OPCODE_LG2"); break;

         case VP_OPCODE_POW:
            fprintf(stderr, "VP_OPCODE_POW"); break;

         case VP_OPCODE_XPD:
            fprintf(stderr, "VP_OPCODE_XPD"); break;

         case VP_OPCODE_SWZ:
            fprintf(stderr, "VP_OPCODE_SWZ"); break;
				
         case VP_OPCODE_END:
            fprintf(stderr, "VP_OPCODE_END"); break;
      }

      fprintf(stderr, " D(0x%x:%d:%d%d%d%d) ", vp[a].DstReg.File, vp[a].DstReg.Index,
          vp[a].DstReg.WriteMask[0],
          vp[a].DstReg.WriteMask[1],
          vp[a].DstReg.WriteMask[2],
          vp[a].DstReg.WriteMask[3]);
		
      fprintf(stderr, "S1(0x%x:%d:%d%d%d%d) ", vp[a].SrcReg[0].File, vp[a].SrcReg[0].Index,
          vp[a].SrcReg[0].Swizzle[0],
          vp[a].SrcReg[0].Swizzle[1],
          vp[a].SrcReg[0].Swizzle[2],
          vp[a].SrcReg[0].Swizzle[3]);

      fprintf(stderr, "S2(0x%x:%d:%d%d%d%d) ", vp[a].SrcReg[1].File, vp[a].SrcReg[1].Index,
          vp[a].SrcReg[1].Swizzle[0],
          vp[a].SrcReg[1].Swizzle[1],
          vp[a].SrcReg[1].Swizzle[2],
          vp[a].SrcReg[1].Swizzle[3]);

      fprintf(stderr, "S3(0x%x:%d:%d%d%d%d)",  vp[a].SrcReg[2].File, vp[a].SrcReg[2].Index,	
          vp[a].SrcReg[2].Swizzle[0],
          vp[a].SrcReg[2].Swizzle[1],
          vp[a].SrcReg[2].Swizzle[2],
          vp[a].SrcReg[2].Swizzle[3]);

      fprintf(stderr, "\n");
   }
}


void
_mesa_parse_arb_vertex_program(GLcontext * ctx, GLenum target,
			       const GLubyte * str, GLsizei len,
			       struct vertex_program *program)
{
   GLuint retval;
   struct arb_program ap;
	
   retval = _mesa_parse_arb_program(ctx, str, len, &ap);

   /*  Parse error. Allocate a dummy program and return */	
   if (retval)
   {
      program->Instructions = (struct vp_instruction *)
         _mesa_malloc(sizeof(struct vp_instruction) );
      program->Instructions[0].Opcode = VP_OPCODE_END;
      return;
   }

   /* Eh.. we parsed something that wasn't a vertex program. doh! */
   if (ap.type != GL_VERTEX_PROGRAM_ARB)
   {
      program->Instructions = (struct vp_instruction *)
         _mesa_malloc(sizeof(struct vp_instruction));
      program->Instructions[0].Opcode = VP_OPCODE_END;
      _mesa_error(ctx, GL_INVALID_OPERATION,
                  "Parsed a non-vertex program as a vertex program");
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

   program->IsPositionInvariant = ap.HintPositionInvariant;
   program->InputsRead     = ap.InputsRead;
   program->OutputsWritten = ap.OutputsWritten;
   program->Parameters     = ap.Parameters; 
   program->Instructions   = ap.VPInstructions;

#if DEBUG_VP
   debug_vp_inst(ap.Base.NumInstructions, ap.VPInstructions);
#else
   (void) debug_vp_inst;
#endif

}
