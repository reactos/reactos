/*
 * Mesa 3-D graphics library
 * Version:  7.3
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
 * \file prog_parameter.c
 * Program parameter lists and functions.
 * \author Brian Paul
 */

#ifndef PROG_PARAMETER_H
#define PROG_PARAMETER_H

#include "main/mtypes.h"
#include "prog_statevars.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Program parameter flags
 */
/*@{*/
#define PROG_PARAM_BIT_CENTROID   0x1  /**< for varying vars (GLSL 1.20) */
#define PROG_PARAM_BIT_INVARIANT  0x2  /**< for varying vars (GLSL 1.20) */
#define PROG_PARAM_BIT_FLAT       0x4  /**< for varying vars (GLSL 1.30) */
#define PROG_PARAM_BIT_LINEAR     0x8  /**< for varying vars (GLSL 1.30) */
#define PROG_PARAM_BIT_CYL_WRAP  0x10  /**< XXX gallium debug */
/*@}*/


/**
 * Actual data for constant values of parameters.
 */
typedef union gl_constant_value
{
   GLfloat f;
   GLint b;
   GLint i;
   GLuint u;
} gl_constant_value;


/**
 * Program parameter.
 * Used by shaders/programs for uniforms, constants, varying vars, etc.
 */
struct gl_program_parameter
{
   const char *Name;        /**< Null-terminated string */
   gl_register_file Type;   /**< PROGRAM_NAMED_PARAM, CONSTANT or STATE_VAR */
   GLenum DataType;         /**< GL_FLOAT, GL_FLOAT_VEC2, etc */
   /**
    * Number of components (1..4), or more.
    * If the number of components is greater than 4,
    * this parameter is part of a larger uniform like a GLSL matrix or array.
    * The next program parameter's Size will be Size-4 of this parameter.
    */
   GLuint Size;
   GLboolean Initialized;   /**< debug: Has the ParameterValue[] been set? */
   GLbitfield Flags;        /**< Bitmask of PROG_PARAM_*_BIT */
   /**
    * A sequence of STATE_* tokens and integers to identify GL state.
    */
   gl_state_index StateIndexes[STATE_LENGTH];
};


/**
 * List of gl_program_parameter instances.
 */
struct gl_program_parameter_list
{
   GLuint Size;           /**< allocated size of Parameters, ParameterValues */
   GLuint NumParameters;  /**< number of parameters in arrays */
   struct gl_program_parameter *Parameters; /**< Array [Size] */
   gl_constant_value (*ParameterValues)[4]; /**< Array [Size] of constant[4] */
   GLbitfield StateFlags; /**< _NEW_* flags indicating which state changes
                               might invalidate ParameterValues[] */
};


extern struct gl_program_parameter_list *
_mesa_new_parameter_list(void);

extern struct gl_program_parameter_list *
_mesa_new_parameter_list_sized(unsigned size);

extern void
_mesa_free_parameter_list(struct gl_program_parameter_list *paramList);

extern struct gl_program_parameter_list *
_mesa_clone_parameter_list(const struct gl_program_parameter_list *list);

extern struct gl_program_parameter_list *
_mesa_combine_parameter_lists(const struct gl_program_parameter_list *a,
                              const struct gl_program_parameter_list *b);

static inline GLuint
_mesa_num_parameters(const struct gl_program_parameter_list *list)
{
   return list ? list->NumParameters : 0;
}

extern GLint
_mesa_add_parameter(struct gl_program_parameter_list *paramList,
                    gl_register_file type, const char *name,
                    GLuint size, GLenum datatype,
                    const gl_constant_value *values,
                    const gl_state_index state[STATE_LENGTH],
                    GLbitfield flags);

extern GLint
_mesa_add_named_parameter(struct gl_program_parameter_list *paramList,
                          const char *name, const gl_constant_value values[4]);

extern GLint
_mesa_add_named_constant(struct gl_program_parameter_list *paramList,
                         const char *name, const gl_constant_value values[4],
                         GLuint size);

extern GLint
_mesa_add_typed_unnamed_constant(struct gl_program_parameter_list *paramList,
                           const gl_constant_value values[4], GLuint size,
                           GLenum datatype, GLuint *swizzleOut);

extern GLint
_mesa_add_unnamed_constant(struct gl_program_parameter_list *paramList,
                           const gl_constant_value values[4], GLuint size,
                           GLuint *swizzleOut);

extern GLint
_mesa_add_varying(struct gl_program_parameter_list *paramList,
                  const char *name, GLuint size, GLenum datatype,
                  GLbitfield flags);

extern GLint
_mesa_add_attribute(struct gl_program_parameter_list *paramList,
                    const char *name, GLint size, GLenum datatype, GLint attrib);

extern GLint
_mesa_add_state_reference(struct gl_program_parameter_list *paramList,
                          const gl_state_index stateTokens[STATE_LENGTH]);

extern gl_constant_value *
_mesa_lookup_parameter_value(const struct gl_program_parameter_list *paramList,
                             GLsizei nameLen, const char *name);

extern GLint
_mesa_lookup_parameter_index(const struct gl_program_parameter_list *paramList,
                             GLsizei nameLen, const char *name);

extern GLboolean
_mesa_lookup_parameter_constant(const struct gl_program_parameter_list *list,
                                const gl_constant_value v[], GLuint vSize,
                                GLint *posOut, GLuint *swizzleOut);

extern GLuint
_mesa_num_parameters_of_type(const struct gl_program_parameter_list *list,
                             gl_register_file type);


#ifdef __cplusplus
}
#endif

#endif /* PROG_PARAMETER_H */
