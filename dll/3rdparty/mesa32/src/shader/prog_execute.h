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

#ifndef PROG_EXECUTE_H
#define PROG_EXECUTE_H

#include "main/config.h"


typedef void (*FetchTexelLodFunc)(GLcontext *ctx, const GLfloat texcoord[4],
                                  GLfloat lambda, GLuint unit, GLfloat color[4]);

typedef void (*FetchTexelDerivFunc)(GLcontext *ctx, const GLfloat texcoord[4],
                                    const GLfloat texdx[4],
                                    const GLfloat texdy[4],
                                    GLfloat lodBias,
                                    GLuint unit, GLfloat color[4]);


/**
 * Virtual machine state used during execution of vertex/fragment programs.
 */
struct gl_program_machine
{
   const struct gl_program *CurProgram;

   /** Fragment Input attributes */
   GLfloat (*Attribs)[MAX_WIDTH][4];
   GLfloat (*DerivX)[4];
   GLfloat (*DerivY)[4];
   GLuint NumDeriv; /**< Max index into DerivX/Y arrays */
   GLuint CurElement; /**< Index into Attribs arrays */

   /** Vertex Input attribs */
   GLfloat VertAttribs[VERT_ATTRIB_MAX][4];

   GLfloat Temporaries[MAX_PROGRAM_TEMPS][4];
   GLfloat Outputs[MAX_PROGRAM_OUTPUTS][4];
   GLfloat (*EnvParams)[4]; /**< Vertex or Fragment env parameters */
   GLuint CondCodes[4];  /**< COND_* value for x/y/z/w */
   GLint AddressReg[MAX_PROGRAM_ADDRESS_REGS][4];

   const GLubyte *Samplers;  /** Array mapping sampler var to tex unit */

   GLuint CallStack[MAX_PROGRAM_CALL_DEPTH]; /**< For CAL/RET instructions */
   GLuint StackDepth; /**< Index/ptr to top of CallStack[] */

   /** Texture fetch functions */
   FetchTexelLodFunc FetchTexelLod;
   FetchTexelDerivFunc FetchTexelDeriv;
};


extern void
_mesa_get_program_register(GLcontext *ctx, enum register_file file,
                           GLuint index, GLfloat val[4]);

extern GLboolean
_mesa_execute_program(GLcontext *ctx,
                      const struct gl_program *program,
                      struct gl_program_machine *machine);


#endif /* PROG_EXECUTE_H */
