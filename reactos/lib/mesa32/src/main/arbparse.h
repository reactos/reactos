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
 *
 * Authors:
 *    Brian Paul
 */


#ifndef ARBPARSE_H
#define ARBPARSE_H

#include "context.h"
#include "mtypes.h"
#include "nvvertprog.h"
#include "nvfragprog.h"

/**
 * This is basically a union of the vertex_program and fragment_program
 * structs that we can use to parse the program into
 *
 * XXX: this should go into mtypes.h?
 */
struct arb_program
{
	GLuint type; /* FRAGMENT_PROGRAM_ARB or VERTEX_PROGRAM_ARB */

	struct program Base;
   struct program_parameter_list *Parameters; 
	GLuint InputsRead;
	GLuint OutputsWritten;

	GLuint Position;       /* Just used for error reporting while parsing */
	GLuint MajorVersion;
	GLuint MinorVersion;

	/* ARB_vertex_program specifics */ 
	struct vp_instruction *VPInstructions;

	/* Options currently recognized by the parser */
	/* ARB_fp */
	GLboolean HintPrecisionFastest;
	GLboolean HintPrecisionNicest;
	GLboolean HintFogExp2;
	GLboolean HintFogExp;
	GLboolean HintFogLinear;

	/* ARB_fp & _vp */
	GLboolean HintPositionInvariant;

	/* ARB_fragment_program sepecifics */
	struct fp_instruction *FPInstructions;
   GLuint TexturesUsed[MAX_TEXTURE_IMAGE_UNITS]; 
   GLuint NumAluInstructions; 
   GLuint NumTexInstructions;
   GLuint NumTexIndirections;
};

extern GLuint 
_mesa_parse_arb_program( GLcontext *ctx, const GLubyte *str, GLsizei len, 
                                 struct arb_program *Program );
                          

#endif
