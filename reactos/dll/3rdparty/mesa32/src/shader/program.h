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

/**
 * \file program.c
 * Vertex and fragment program support functions.
 * \author Brian Paul
 */


/**
 * \mainpage Mesa vertex and fragment program module
 *
 * This module or directory contains most of the code for vertex and
 * fragment programs and shaders, including state management, parsers,
 * and (some) software routines for executing programs
 */

#ifndef PROGRAM_H
#define PROGRAM_H

#include "mtypes.h"


extern struct gl_program _mesa_DummyProgram;


extern void
_mesa_init_program(GLcontext *ctx);

extern void
_mesa_free_program_data(GLcontext *ctx);

extern void
_mesa_update_default_objects_program(GLcontext *ctx);

extern void
_mesa_set_program_error(GLcontext *ctx, GLint pos, const char *string);

extern const GLubyte *
_mesa_find_line_column(const GLubyte *string, const GLubyte *pos,
                       GLint *line, GLint *col);


extern struct gl_program *
_mesa_init_vertex_program(GLcontext *ctx,
                          struct gl_vertex_program *prog,
                          GLenum target, GLuint id);

extern struct gl_program *
_mesa_init_fragment_program(GLcontext *ctx,
                            struct gl_fragment_program *prog,
                            GLenum target, GLuint id);

extern struct gl_program *
_mesa_new_program(GLcontext *ctx, GLenum target, GLuint id);

extern void
_mesa_delete_program(GLcontext *ctx, struct gl_program *prog);

extern struct gl_program *
_mesa_lookup_program(GLcontext *ctx, GLuint id);

extern void
_mesa_reference_program(GLcontext *ctx,
                        struct gl_program **ptr,
                        struct gl_program *prog);

static INLINE void
_mesa_reference_vertprog(GLcontext *ctx,
                         struct gl_vertex_program **ptr,
                         struct gl_vertex_program *prog)
{
   _mesa_reference_program(ctx, (struct gl_program **) ptr,
                           (struct gl_program *) prog);
}

static INLINE void
_mesa_reference_fragprog(GLcontext *ctx,
                         struct gl_fragment_program **ptr,
                         struct gl_fragment_program *prog)
{
   _mesa_reference_program(ctx, (struct gl_program **) ptr,
                           (struct gl_program *) prog);
}

extern struct gl_program *
_mesa_clone_program(GLcontext *ctx, const struct gl_program *prog);

extern  GLboolean
_mesa_insert_instructions(struct gl_program *prog, GLuint start, GLuint count);

extern  GLboolean
_mesa_delete_instructions(struct gl_program *prog, GLuint start, GLuint count);

extern struct gl_program *
_mesa_combine_programs(GLcontext *ctx,
                       const struct gl_program *progA,
                       const struct gl_program *progB);

extern GLint
_mesa_find_free_register(const struct gl_program *prog, GLuint regFile);



#endif /* PROGRAM_H */
