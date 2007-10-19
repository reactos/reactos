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


#ifndef ARBPROGPARSE_H
#define ARBPROGPARSE_H

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
   GLenum PrecisionOption; /* GL_DONT_CARE, GL_NICEST or GL_FASTEST */
   GLenum FogOption;       /* GL_NONE, GL_LINEAR, GL_EXP or GL_EXP2 */

   /* ARB_fp & _vp */
   GLboolean HintPositionInvariant;

   /* ARB_fragment_program specifics */
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
