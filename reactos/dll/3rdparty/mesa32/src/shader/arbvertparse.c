/*
 * Mesa 3-D graphics library
 * Version:  6.2
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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
#include "program.h"
#include "nvprogram.h"
#include "nvvertparse.h"
#include "nvvertprog.h"

#include "arbprogparse.h"


/**
 * XXX this is probably redundant.  We've already got code like this
 * in the nvvertparse.c file.  Combine/clean-up someday.
 */
void _mesa_debug_vp_inst(GLint num, struct vp_instruction *vp)
{
   GLint a;
   static const char *opcode_string[] = {
      "ABS",
      "ADD",
      "ARL",
      "DP3",
      "DP4",
      "DPH",
      "DST",
      "END",		/* Placeholder */
      "EX2",		/* ARB only */
      "EXP",
      "FLR",		/* ARB */
      "FRC",		/* ARB */
      "LG2",		/* ARB only */
      "LIT",
      "LOG",
      "MAD",
      "MAX",
      "MIN",
      "MOV",
      "MUL",
      "POW",		/* ARB only */
      "PRINT",		/* Mesa only */
      "RCC",
      "RCP",
      "RSQ",
      "SGE",
      "SLT",
      "SUB",
      "SWZ",		/* ARB only */
      "XPD"		/* ARB only */
   };

   static const char *file_string[] = {
      "TEMP",
      "INPUT",
      "OUTPUT",
      "LOCAL",
      "ENV",
      "NAMED",
      "STATE",
      "WRITE_ONLY",
      "ADDR"
   };

   static const char swz[] = "xyzw01??";

   for (a=0; a<num; a++) {
      _mesa_printf("%s", opcode_string[vp[a].Opcode]);

      if (vp[a].DstReg.File != 0xf) {
	 if (vp[a].DstReg.WriteMask != 0xf)
	    _mesa_printf(" %s[%d].%s%s%s%s ", file_string[vp[a].DstReg.File], vp[a].DstReg.Index,
			 GET_BIT(vp[a].DstReg.WriteMask, 0) ? "x" : "",
			 GET_BIT(vp[a].DstReg.WriteMask, 1) ? "y" : "",
			 GET_BIT(vp[a].DstReg.WriteMask, 2) ? "z" : "",
			 GET_BIT(vp[a].DstReg.WriteMask, 3) ? "w" : "");
	 else
	    _mesa_printf(" %s[%d] ", file_string[vp[a].DstReg.File], vp[a].DstReg.Index);
      }

      if (vp[a].SrcReg[0].File != 0xf) {
	 if (vp[a].SrcReg[0].Swizzle != SWIZZLE_NOOP ||
	     vp[a].SrcReg[0].Negate)
	    _mesa_printf("%s[%d].%s%c%c%c%c ", file_string[vp[a].SrcReg[0].File], vp[a].SrcReg[0].Index,
			 vp[a].SrcReg[0].Negate ? "-" : "",
			 swz[GET_SWZ(vp[a].SrcReg[0].Swizzle, 0)],
			 swz[GET_SWZ(vp[a].SrcReg[0].Swizzle, 1)],
			 swz[GET_SWZ(vp[a].SrcReg[0].Swizzle, 2)],
			 swz[GET_SWZ(vp[a].SrcReg[0].Swizzle, 3)]);
	 else
	    _mesa_printf("%s[%d] ", file_string[vp[a].SrcReg[0].File], vp[a].SrcReg[0].Index);
      }

      if (vp[a].SrcReg[1].File != 0xf) {
	 if (vp[a].SrcReg[1].Swizzle != SWIZZLE_NOOP ||
	     vp[a].SrcReg[1].Negate)
	    _mesa_printf("%s[%d].%s%c%c%c%c ", file_string[vp[a].SrcReg[1].File], vp[a].SrcReg[1].Index,
			 vp[a].SrcReg[1].Negate ? "-" : "",
			 swz[GET_SWZ(vp[a].SrcReg[1].Swizzle, 0)],
			 swz[GET_SWZ(vp[a].SrcReg[1].Swizzle, 1)],
			 swz[GET_SWZ(vp[a].SrcReg[1].Swizzle, 2)],
			 swz[GET_SWZ(vp[a].SrcReg[1].Swizzle, 3)]);
	 else
	    _mesa_printf("%s[%d] ", file_string[vp[a].SrcReg[1].File], vp[a].SrcReg[1].Index);
      }

      if (vp[a].SrcReg[2].File != 0xf) {
	 if (vp[a].SrcReg[2].Swizzle != SWIZZLE_NOOP ||
	     vp[a].SrcReg[2].Negate)
	    _mesa_printf("%s[%d].%s%c%c%c%c ", file_string[vp[a].SrcReg[2].File], vp[a].SrcReg[2].Index,
			 vp[a].SrcReg[2].Negate ? "-" : "",
			 swz[GET_SWZ(vp[a].SrcReg[2].Swizzle, 0)],
			 swz[GET_SWZ(vp[a].SrcReg[2].Swizzle, 1)],
			 swz[GET_SWZ(vp[a].SrcReg[2].Swizzle, 2)],
			 swz[GET_SWZ(vp[a].SrcReg[2].Swizzle, 3)]);
	 else
	    _mesa_printf("%s[%d] ", file_string[vp[a].SrcReg[2].File], vp[a].SrcReg[2].Index);
      }

      _mesa_printf("\n");
   }
}


void
_mesa_parse_arb_vertex_program(GLcontext * ctx, GLenum target,
			       const GLubyte * str, GLsizei len,
			       struct vertex_program *program)
{
   GLuint retval;
   struct arb_program ap;
   (void) target;

   /* set the program target before parsing */
   ap.Base.Target = GL_VERTEX_PROGRAM_ARB;

   retval = _mesa_parse_arb_program(ctx, str, len, &ap);

   /*  Parse error. Allocate a dummy program and return */
   if (retval)
   {
      program->Instructions = (struct vp_instruction *)
         _mesa_malloc ( sizeof(struct vp_instruction) );
      program->Instructions[0].Opcode = VP_OPCODE_END;
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
   _mesa_debug_vp_inst(ap.Base.NumInstructions, ap.VPInstructions);
#endif

}
