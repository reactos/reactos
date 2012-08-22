/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 2004-2007  Brian Paul   All Rights Reserved.
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


#ifndef SHADEROBJ_H
#define SHADEROBJ_H


#include "main/compiler.h"
#include "main/glheader.h"
#include "main/mtypes.h"
#include "program/ir_to_mesa.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Internal functions
 */

extern void
_mesa_init_shader_state(struct gl_context * ctx);

extern void
_mesa_free_shader_state(struct gl_context *ctx);


extern void
_mesa_reference_shader(struct gl_context *ctx, struct gl_shader **ptr,
                       struct gl_shader *sh);

extern struct gl_shader *
_mesa_lookup_shader(struct gl_context *ctx, GLuint name);

extern struct gl_shader *
_mesa_lookup_shader_err(struct gl_context *ctx, GLuint name, const char *caller);



extern void
_mesa_reference_shader_program(struct gl_context *ctx,
                               struct gl_shader_program **ptr,
                               struct gl_shader_program *shProg);
extern void
_mesa_init_shader(struct gl_context *ctx, struct gl_shader *shader);

extern struct gl_shader *
_mesa_new_shader(struct gl_context *ctx, GLuint name, GLenum type);

extern void
_mesa_init_shader_program(struct gl_context *ctx, struct gl_shader_program *prog);

extern struct gl_shader_program *
_mesa_lookup_shader_program(struct gl_context *ctx, GLuint name);

extern struct gl_shader_program *
_mesa_lookup_shader_program_err(struct gl_context *ctx, GLuint name,
                                const char *caller);

extern void
_mesa_clear_shader_program_data(struct gl_context *ctx,
                                struct gl_shader_program *shProg);

extern void
_mesa_free_shader_program_data(struct gl_context *ctx,
                               struct gl_shader_program *shProg);



extern void
_mesa_init_shader_object_functions(struct dd_function_table *driver);

extern void
_mesa_init_shader_state(struct gl_context *ctx);

extern void
_mesa_free_shader_state(struct gl_context *ctx);


static inline gl_shader_type
_mesa_shader_type_to_index(GLenum v)
{
   switch (v) {
   case GL_VERTEX_SHADER:
      return MESA_SHADER_VERTEX;
   case GL_FRAGMENT_SHADER:
      return MESA_SHADER_FRAGMENT;
   case GL_GEOMETRY_SHADER:
      return MESA_SHADER_GEOMETRY;
   default:
      ASSERT(0 && "bad value in _mesa_shader_type_to_index()");
      return MESA_SHADER_TYPES;
   }
}


static inline GLenum
_mesa_shader_index_to_type(GLuint i)
{
   static const GLenum enums[MESA_SHADER_TYPES] = {
      GL_VERTEX_SHADER,
      GL_FRAGMENT_SHADER,
      GL_GEOMETRY_SHADER ,
   };
   if (i >= MESA_SHADER_TYPES)
      return 0;
   else
      return enums[i];
}


#ifdef __cplusplus
}
#endif

#endif /* SHADEROBJ_H */
