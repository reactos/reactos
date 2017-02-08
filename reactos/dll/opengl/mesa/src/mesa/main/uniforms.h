/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2010  VMware, Inc.  All Rights Reserved.
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
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef UNIFORMS_H
#define UNIFORMS_H

#include "glheader.h"
#include "program/prog_parameter.h"
#include "../glsl/glsl_types.h"
#include "../glsl/ir_uniform.h"

#ifdef __cplusplus
extern "C" {
#endif


struct gl_program;
struct _glapi_table;

extern void GLAPIENTRY
_mesa_Uniform1fARB(GLint, GLfloat);

extern void GLAPIENTRY
_mesa_Uniform2fARB(GLint, GLfloat, GLfloat);

extern void GLAPIENTRY
_mesa_Uniform3fARB(GLint, GLfloat, GLfloat, GLfloat);

extern void GLAPIENTRY
_mesa_Uniform4fARB(GLint, GLfloat, GLfloat, GLfloat, GLfloat);

extern void GLAPIENTRY
_mesa_Uniform1iARB(GLint, GLint);

extern void GLAPIENTRY
_mesa_Uniform2iARB(GLint, GLint, GLint);

extern void GLAPIENTRY
_mesa_Uniform3iARB(GLint, GLint, GLint, GLint);

extern void GLAPIENTRY
_mesa_Uniform4iARB(GLint, GLint, GLint, GLint, GLint);

extern void GLAPIENTRY
_mesa_Uniform1fvARB(GLint, GLsizei, const GLfloat *);

extern void GLAPIENTRY
_mesa_Uniform2fvARB(GLint, GLsizei, const GLfloat *);

extern void GLAPIENTRY
_mesa_Uniform3fvARB(GLint, GLsizei, const GLfloat *);

extern void GLAPIENTRY
_mesa_Uniform4fvARB(GLint, GLsizei, const GLfloat *);

extern void GLAPIENTRY
_mesa_Uniform1ivARB(GLint, GLsizei, const GLint *);

extern void GLAPIENTRY
_mesa_Uniform2ivARB(GLint, GLsizei, const GLint *);

extern void GLAPIENTRY
_mesa_Uniform3ivARB(GLint, GLsizei, const GLint *);

extern void GLAPIENTRY
_mesa_Uniform4ivARB(GLint, GLsizei, const GLint *);

extern void GLAPIENTRY
_mesa_Uniform1ui(GLint location, GLuint v0);

extern void GLAPIENTRY
_mesa_Uniform2ui(GLint location, GLuint v0, GLuint v1);

extern void GLAPIENTRY
_mesa_Uniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2);

extern void GLAPIENTRY
_mesa_Uniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);

extern void GLAPIENTRY
_mesa_Uniform1uiv(GLint location, GLsizei count, const GLuint *value);

extern void GLAPIENTRY
_mesa_Uniform2uiv(GLint location, GLsizei count, const GLuint *value);

extern void GLAPIENTRY
_mesa_Uniform3uiv(GLint location, GLsizei count, const GLuint *value);

extern void GLAPIENTRY
_mesa_Uniform4uiv(GLint location, GLsizei count, const GLuint *value);


extern void GLAPIENTRY
_mesa_UniformMatrix2fvARB(GLint, GLsizei, GLboolean, const GLfloat *);

extern void GLAPIENTRY
_mesa_UniformMatrix3fvARB(GLint, GLsizei, GLboolean, const GLfloat *);

extern void GLAPIENTRY
_mesa_UniformMatrix4fvARB(GLint, GLsizei, GLboolean, const GLfloat *);

extern void GLAPIENTRY
_mesa_UniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value);

extern void GLAPIENTRY
_mesa_UniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value);

extern void GLAPIENTRY
_mesa_UniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value);

extern void GLAPIENTRY
_mesa_UniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value);

extern void GLAPIENTRY
_mesa_UniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value);

extern void GLAPIENTRY
_mesa_UniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value);


extern void GLAPIENTRY
_mesa_GetActiveUniformARB(GLhandleARB, GLuint, GLsizei, GLsizei *,
                          GLint *, GLenum *, GLcharARB *);

extern void GLAPIENTRY
_mesa_GetUniformfvARB(GLhandleARB, GLint, GLfloat *);

extern void GLAPIENTRY
_mesa_GetnUniformfvARB(GLhandleARB, GLint, GLsizei, GLfloat *);

extern void GLAPIENTRY
_mesa_GetUniformivARB(GLhandleARB, GLint, GLint *);

extern void GLAPIENTRY
_mesa_GetnUniformivARB(GLhandleARB, GLint, GLsizei, GLint *);

extern void GLAPIENTRY
_mesa_GetUniformuiv(GLhandleARB, GLint, GLuint *);

extern void GLAPIENTRY
_mesa_GetnUniformuivARB(GLhandleARB, GLint, GLsizei, GLuint *);

extern void GLAPIENTRY
_mesa_GetUniformdv(GLhandleARB, GLint, GLdouble *);

extern void GLAPIENTRY
_mesa_GetnUniformdvARB(GLhandleARB, GLint, GLsizei, GLdouble *);

extern GLint GLAPIENTRY
_mesa_GetUniformLocationARB(GLhandleARB, const GLcharARB *);

GLint
_mesa_get_uniform_location(struct gl_context *ctx, struct gl_shader_program *shProg,
			   const GLchar *name);

void
_mesa_uniform(struct gl_context *ctx, struct gl_shader_program *shader_program,
	      GLint location, GLsizei count,
              const GLvoid *values, GLenum type);

void
_mesa_uniform_matrix(struct gl_context *ctx, struct gl_shader_program *shProg,
		     GLuint cols, GLuint rows,
                     GLint location, GLsizei count,
                     GLboolean transpose, const GLfloat *values);

void
_mesa_get_uniform(struct gl_context *ctx, GLuint program, GLint location,
		  GLsizei bufSize, enum glsl_base_type returnType,
		  GLvoid *paramsOut);

extern void
_mesa_uniform_attach_driver_storage(struct gl_uniform_storage *,
				    unsigned element_stride,
				    unsigned vector_stride,
				    enum gl_uniform_driver_format format,
				    void *data);

extern void
_mesa_uniform_detach_all_driver_storage(struct gl_uniform_storage *uni);

extern void
_mesa_propagate_uniforms_to_driver_storage(struct gl_uniform_storage *uni,
					   unsigned array_index,
					   unsigned count);

extern void
_mesa_update_shader_textures_used(struct gl_shader_program *shProg,
				  struct gl_program *prog);

extern bool
_mesa_sampler_uniforms_are_valid(const struct gl_shader_program *shProg,
				 char *errMsg, size_t errMsgLength);

extern void
_mesa_init_shader_uniform_dispatch(struct _glapi_table *exec);

extern const struct gl_program_parameter *
get_uniform_parameter(struct gl_shader_program *shProg, GLint index);

struct gl_builtin_uniform_element {
   const char *field;
   int tokens[STATE_LENGTH];
   int swizzle;
};

struct gl_builtin_uniform_desc {
   const char *name;
   struct gl_builtin_uniform_element *elements;
   unsigned int num_elements;
};

extern const struct gl_builtin_uniform_desc _mesa_builtin_uniform_desc[];

/**
 * \name GLSL uniform arrays and structs require special handling.
 *
 * The GL_ARB_shader_objects spec says that if you use
 * glGetUniformLocation to get the location of an array, you CANNOT
 * access other elements of the array by adding an offset to the
 * returned location.  For example, you must call
 * glGetUniformLocation("foo[16]") if you want to set the 16th element
 * of the array with glUniform().
 *
 * HOWEVER, some other OpenGL drivers allow accessing array elements
 * by adding an offset to the returned array location.  And some apps
 * seem to depend on that behaviour.
 *
 * Mesa's gl_uniform_list doesn't directly support this since each
 * entry in the list describes one uniform variable, not one uniform
 * element.  We could insert dummy entries in the list for each array
 * element after [0] but that causes complications elsewhere.
 *
 * We solve this problem by encoding two values in the location that's
 * returned by glGetUniformLocation():
 *  a) index into gl_uniform_list::Uniforms[] for the uniform
 *  b) an array/field offset (0 for simple types)
 *
 * These two values are encoded in the high and low halves of a GLint.
 * By putting the uniform number in the high part and the offset in the
 * low part, we can support the unofficial ability to index into arrays
 * by adding offsets to the location value.
 */
/*@{*/
/**
 * Combine the uniform's base location and the offset
 */
static inline GLint
_mesa_uniform_merge_location_offset(unsigned base_location, unsigned offset)
{
   return (base_location << 16) | offset;
}

/**
 * Separate the uniform base location and parameter offset
 */
static inline void
_mesa_uniform_split_location_offset(GLint location, unsigned *base_location,
				    unsigned *offset)
{
   *offset = location & 0xffff;
   *base_location = location >> 16;
}
/*@}*/


#ifdef __cplusplus
}
#endif


#endif /* UNIFORMS_H */
