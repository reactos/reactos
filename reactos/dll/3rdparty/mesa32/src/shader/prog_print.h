/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
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


#ifndef PROG_PRINT_H
#define PROG_PRINT_H


/**
 * The output style to use when printing programs.
 */
typedef enum {
   PROG_PRINT_ARB,
   PROG_PRINT_NV,
   PROG_PRINT_DEBUG
} gl_prog_print_mode;


extern const char *
_mesa_condcode_string(GLuint condcode);

extern const char *
_mesa_swizzle_string(GLuint swizzle, GLuint negateBase, GLboolean extended);

extern void
_mesa_print_alu_instruction(const struct prog_instruction *inst,
                            const char *opcode_string, GLuint numRegs);

extern void
_mesa_print_instruction(const struct prog_instruction *inst);

extern GLint
_mesa_print_instruction_opt(const struct prog_instruction *inst, GLint indent,
                            gl_prog_print_mode mode,
                            const struct gl_program *prog);

extern void
_mesa_print_program(const struct gl_program *prog);

extern void
_mesa_print_program_opt(const struct gl_program *prog, gl_prog_print_mode mode,
                        GLboolean lineNumbers);

extern void
_mesa_print_program_parameters(GLcontext *ctx, const struct gl_program *prog);

extern void
_mesa_print_parameter_list(const struct gl_program_parameter_list *list);


#endif /* PROG_PRINT_H */
