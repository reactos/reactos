/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
 * \file prog_uniform.c
 * Shader uniform functions.
 * \author Brian Paul
 */

#ifndef PROG_UNIFORM_H
#define PROG_UNIFORM_H

#include "main/mtypes.h"
#include "prog_statevars.h"


/**
 * Shader program uniform variable.
 * The glGetUniformLocation() and glUniform() commands will use this
 * information.
 * Note that a uniform such as "binormal" might be used in both the
 * vertex shader and the fragment shader.  When glUniform() is called to
 * set the uniform's value, it must be updated in both the vertex and
 * fragment shaders.  The uniform may be in different locations in the
 * two shaders so we keep track of that here.
 */
struct gl_uniform
{
   const char *Name;        /**< Null-terminated string */
   GLint VertPos;
   GLint FragPos;
#if 0
   GLenum DataType;         /**< GL_FLOAT, GL_FLOAT_VEC2, etc */
   GLuint Size;             /**< Number of components (1..4) */
#endif
};


/**
 * List of gl_uniforms
 */
struct gl_uniform_list
{
   GLuint Size;                 /**< allocated size of Uniforms array */
   GLuint NumUniforms;          /**< number of uniforms in the array */
   struct gl_uniform *Uniforms; /**< Array [Size] */
};


extern struct gl_uniform_list *
_mesa_new_uniform_list(void);

extern void
_mesa_free_uniform_list(struct gl_uniform_list *list);

extern GLboolean
_mesa_append_uniform(struct gl_uniform_list *list,
                     const char *name, GLenum target, GLuint progPos);

extern GLint
_mesa_lookup_uniform(const struct gl_uniform_list *list, const char *name);

extern GLint
_mesa_longest_uniform_name(const struct gl_uniform_list *list);

extern void
_mesa_print_uniforms(const struct gl_uniform_list *list);


#endif /* PROG_UNIFORM_H */
