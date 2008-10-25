/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 2004-2006  Brian Paul   All Rights Reserved.
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


#ifndef SHADER_API_H
#define SHADER_API_H


#include "glheader.h"
#include "mtypes.h"


/**
 * Internal functions
 */

extern void
_mesa_init_shader_state(GLcontext * ctx);

extern void
_mesa_free_shader_state(GLcontext *ctx);

/*
extern struct gl_shader_program *
_mesa_new_shader_program(GLcontext *ctx, GLuint name);
*/
extern void
_mesa_clear_shader_program_data(GLcontext *ctx,
                                struct gl_shader_program *shProg);

extern void
_mesa_free_shader_program_data(GLcontext *ctx,
                               struct gl_shader_program *shProg);

extern void
_mesa_free_shader_program(GLcontext *ctx, struct gl_shader_program *shProg);

extern void
_mesa_reference_shader_program(GLcontext *ctx,
                               struct gl_shader_program **ptr,
                               struct gl_shader_program *shProg);

extern struct gl_shader_program *
_mesa_lookup_shader_program(GLcontext *ctx, GLuint name);


extern struct gl_shader *
_mesa_new_shader(GLcontext *ctx, GLuint name, GLenum type);

extern void
_mesa_free_shader(GLcontext *ctx, struct gl_shader *sh);

extern void
_mesa_reference_shader(GLcontext *ctx, struct gl_shader **ptr,
                       struct gl_shader *sh);

extern struct gl_shader *
_mesa_lookup_shader(GLcontext *ctx, GLuint name);


extern void
_mesa_use_program(GLcontext *ctx, GLuint program);


extern void
_mesa_init_glsl_driver_functions(struct dd_function_table *driver);


#endif /* SHADER_API_H */
