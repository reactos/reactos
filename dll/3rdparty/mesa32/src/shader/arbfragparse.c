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
#include "program.h"
#include "arbprogparse.h"
#include "arbfragparse.h"

void
_mesa_debug_fp_inst(GLint num, struct fp_instruction *fp)
{
   GLint a;

   static const char *opcode_string[] = {
      "ABS",		/* ARB_f_p only */
      "ADD",
      "CMP",		/* ARB_f_p only */
      "COS",
      "DDX",		/* NV_f_p only */
      "DDY",		/* NV_f_p only */
      "DP3",
      "DP4",
      "DPH",		/* ARB_f_p only */
      "DST",
      "END",		/* private opcode */
      "EX2",
      "FLR",
      "FRC",
      "KIL",		/* ARB_f_p only */
      "KIL_NV",		/* NV_f_p only */
      "LG2",
      "LIT",
      "LRP",
      "MAD",
      "MAX",
      "MIN",
      "MOV",
      "MUL",
      "PK2H",		/* NV_f_p only */
      "PK2US",		/* NV_f_p only */
      "PK4B",		/* NV_f_p only */
      "PK4UB",		/* NV_f_p only */
      "POW",
      "PRINT",		/* Mesa only */
      "RCP",
      "RFL",		/* NV_f_p only */
      "RSQ",
      "SCS",		/* ARB_f_p only */
      "SEQ",		/* NV_f_p only */
      "SFL",		/* NV_f_p only */
      "SGE",		/* NV_f_p only */
      "SGT",		/* NV_f_p only */
      "SIN",
      "SLE",		/* NV_f_p only */
      "SLT",
      "SNE",		/* NV_f_p only */
      "STR",		/* NV_f_p only */
      "SUB",
      "SWZ",		/* ARB_f_p only */
      "TEX",
      "TXB",		/* ARB_f_p only */
      "TXD",		/* NV_f_p only */
      "TXP",		/* ARB_f_p only */
      "TXP_NV",		/* NV_f_p only */
      "UP2H",		/* NV_f_p only */
      "UP2US",		/* NV_f_p only */
      "UP4B",		/* NV_f_p only */
      "UP4UB",		/* NV_f_p only */
      "X2D",		/* NV_f_p only - 2d mat mul */
      "XPD",		/* ARB_f_p only - cross product */
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
      _mesa_printf("%s", opcode_string[fp[a].Opcode]);

      if (fp[a].Saturate)
	 _mesa_printf("_SAT");

      if (fp[a].DstReg.File != 0xf) {
	 if (fp[a].DstReg.WriteMask != 0xf ||
	     fp[a].SrcReg[0].NegateBase)
	    _mesa_printf(" %s[%d].%s%s%s%s ", file_string[fp[a].DstReg.File], fp[a].DstReg.Index,
			 GET_BIT(fp[a].DstReg.WriteMask, 0) ? "x" : "",
			 GET_BIT(fp[a].DstReg.WriteMask, 1) ? "y" : "",
			 GET_BIT(fp[a].DstReg.WriteMask, 2) ? "z" : "",
			 GET_BIT(fp[a].DstReg.WriteMask, 3) ? "w" : "");
	 else
	    _mesa_printf(" %s[%d] ", file_string[fp[a].DstReg.File], fp[a].DstReg.Index);
      }

      if (fp[a].SrcReg[0].File != 0xf) {
	 if (fp[a].SrcReg[0].Swizzle != SWIZZLE_NOOP ||
	     fp[a].SrcReg[0].NegateBase)
	    _mesa_printf("%s[%d].%s%c%c%c%c ", file_string[fp[a].SrcReg[0].File], fp[a].SrcReg[0].Index,
			 fp[a].SrcReg[0].NegateBase ? "-" : "",
			 swz[GET_SWZ(fp[a].SrcReg[0].Swizzle, 0)],
			 swz[GET_SWZ(fp[a].SrcReg[0].Swizzle, 1)],
			 swz[GET_SWZ(fp[a].SrcReg[0].Swizzle, 2)],
			 swz[GET_SWZ(fp[a].SrcReg[0].Swizzle, 3)]);
	 else
	    _mesa_printf("%s[%d] ", file_string[fp[a].SrcReg[0].File], fp[a].SrcReg[0].Index);
      }

      if (fp[a].SrcReg[1].File != 0xf) {
	 if (fp[a].SrcReg[1].Swizzle != SWIZZLE_NOOP ||
	     fp[a].SrcReg[1].NegateBase)
	    _mesa_printf("%s[%d].%s%c%c%c%c ", file_string[fp[a].SrcReg[1].File], fp[a].SrcReg[1].Index,
			 fp[a].SrcReg[1].NegateBase ? "-" : "",
			 swz[GET_SWZ(fp[a].SrcReg[1].Swizzle, 0)],
			 swz[GET_SWZ(fp[a].SrcReg[1].Swizzle, 1)],
			 swz[GET_SWZ(fp[a].SrcReg[1].Swizzle, 2)],
			 swz[GET_SWZ(fp[a].SrcReg[1].Swizzle, 3)]);
	 else
	    _mesa_printf("%s[%d] ", file_string[fp[a].SrcReg[1].File], fp[a].SrcReg[1].Index);
      }

      if (fp[a].SrcReg[2].File != 0xf) {
	 if (fp[a].SrcReg[2].Swizzle != SWIZZLE_NOOP ||
	     fp[a].SrcReg[2].NegateBase)
	    _mesa_printf("%s[%d].%s%c%c%c%c ", file_string[fp[a].SrcReg[2].File], fp[a].SrcReg[2].Index,
			 fp[a].SrcReg[1].NegateBase ? "-" : "",
			 swz[GET_SWZ(fp[a].SrcReg[2].Swizzle, 0)],
			 swz[GET_SWZ(fp[a].SrcReg[2].Swizzle, 1)],
			 swz[GET_SWZ(fp[a].SrcReg[2].Swizzle, 2)],
			 swz[GET_SWZ(fp[a].SrcReg[2].Swizzle, 3)]);
	 else
	    _mesa_printf("%s[%d] ", file_string[fp[a].SrcReg[2].File], fp[a].SrcReg[2].Index);
      }

      _mesa_printf("\n");
   }
}

void
_mesa_parse_arb_fragment_program(GLcontext * ctx, GLenum target,
                                 const GLubyte * str, GLsizei len,
                                 struct fragment_program *program)
{
   GLuint a, retval;
   struct arb_program ap;
   (void) target;

   /* set the program target before parsing */
   ap.Base.Target = GL_FRAGMENT_PROGRAM_ARB;

   retval = _mesa_parse_arb_program(ctx, str, len, &ap);

   /* XXX: Parse error. Cleanup things and return */
   if (retval)
   {
      program->Instructions = (struct fp_instruction *) _mesa_malloc (
                                     sizeof(struct fp_instruction) );
      program->Instructions[0].Opcode = FP_OPCODE_END;
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
   program->FogOption          = ap.FogOption;

#if DEBUG_FP
   _mesa_debug_fp_inst(ap.Base.NumInstructions, ap.FPInstructions);
#endif

   program->Instructions   = ap.FPInstructions;
}
