/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2004-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009-2010  VMware, Inc.  All Rights Reserved.
 * Copyright © 2010 Intel Corporation
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
 * \file uniforms.c
 * Functions related to GLSL uniform variables.
 * \author Brian Paul
 */

/**
 * XXX things to do:
 * 1. Check that the right error code is generated for all _mesa_error() calls.
 * 2. Insert FLUSH_VERTICES calls in various places
 */

#include "main/glheader.h"
#include "main/context.h"
#include "main/dispatch.h"
#include "main/shaderapi.h"
#include "main/shaderobj.h"
#include "main/uniforms.h"
#include "ir_uniform.h"
#include "glsl_types.h"

/**
 * Update the vertex/fragment program's TexturesUsed array.
 *
 * This needs to be called after glUniform(set sampler var) is called.
 * A call to glUniform(samplerVar, value) causes a sampler to point to a
 * particular texture unit.  We know the sampler's texture target
 * (1D/2D/3D/etc) from compile time but the sampler's texture unit is
 * set by glUniform() calls.
 *
 * So, scan the program->SamplerUnits[] and program->SamplerTargets[]
 * information to update the prog->TexturesUsed[] values.
 * Each value of TexturesUsed[unit] is one of zero, TEXTURE_1D_INDEX,
 * TEXTURE_2D_INDEX, TEXTURE_3D_INDEX, etc.
 * We'll use that info for state validation before rendering.
 */
void
_mesa_update_shader_textures_used(struct gl_shader_program *shProg,
				  struct gl_program *prog)
{
   GLuint s;

   memset(prog->TexturesUsed, 0, sizeof(prog->TexturesUsed));

   for (s = 0; s < MAX_SAMPLERS; s++) {
      if (prog->SamplersUsed & (1 << s)) {
         GLuint unit = shProg->SamplerUnits[s];
         GLuint tgt = shProg->SamplerTargets[s];
         assert(unit < Elements(prog->TexturesUsed));
         assert(tgt < NUM_TEXTURE_TARGETS);
         prog->TexturesUsed[unit] |= (1 << tgt);
      }
   }
}

/**
 * Connect a piece of driver storage with a part of a uniform
 *
 * \param uni            The uniform with which the storage will be associated
 * \param element_stride Byte-stride between array elements.
 *                       \sa gl_uniform_driver_storage::element_stride.
 * \param vector_stride  Byte-stride between vectors (in a matrix).
 *                       \sa gl_uniform_driver_storage::vector_stride.
 * \param format         Conversion from native format to driver format
 *                       required by the driver.
 * \param data           Location to dump the data.
 */
void
_mesa_uniform_attach_driver_storage(struct gl_uniform_storage *uni,
				    unsigned element_stride,
				    unsigned vector_stride,
				    enum gl_uniform_driver_format format,
				    void *data)
{
   uni->driver_storage = (struct gl_uniform_driver_storage*)
      realloc(uni->driver_storage,
	      sizeof(struct gl_uniform_driver_storage)
	      * (uni->num_driver_storage + 1));

   uni->driver_storage[uni->num_driver_storage].element_stride = element_stride;
   uni->driver_storage[uni->num_driver_storage].vector_stride = vector_stride;
   uni->driver_storage[uni->num_driver_storage].format = (uint8_t) format;
   uni->driver_storage[uni->num_driver_storage].data = data;

   uni->num_driver_storage++;
}

/**
 * Sever all connections with all pieces of driver storage for all uniforms
 *
 * \warning
 * This function does \b not release any of the \c data pointers
 * previously passed in to \c _mesa_uniform_attach_driver_stoarge.
 */
void
_mesa_uniform_detach_all_driver_storage(struct gl_uniform_storage *uni)
{
   free(uni->driver_storage);
   uni->driver_storage = NULL;
   uni->num_driver_storage = 0;
}

void GLAPIENTRY
_mesa_Uniform1fARB(GLint location, GLfloat v0)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, 1, &v0, GL_FLOAT);
}

void GLAPIENTRY
_mesa_Uniform2fARB(GLint location, GLfloat v0, GLfloat v1)
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat v[2];
   v[0] = v0;
   v[1] = v1;
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, 1, v, GL_FLOAT_VEC2);
}

void GLAPIENTRY
_mesa_Uniform3fARB(GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat v[3];
   v[0] = v0;
   v[1] = v1;
   v[2] = v2;
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, 1, v, GL_FLOAT_VEC3);
}

void GLAPIENTRY
_mesa_Uniform4fARB(GLint location, GLfloat v0, GLfloat v1, GLfloat v2,
                   GLfloat v3)
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat v[4];
   v[0] = v0;
   v[1] = v1;
   v[2] = v2;
   v[3] = v3;
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, 1, v, GL_FLOAT_VEC4);
}

void GLAPIENTRY
_mesa_Uniform1iARB(GLint location, GLint v0)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, 1, &v0, GL_INT);
}

void GLAPIENTRY
_mesa_Uniform2iARB(GLint location, GLint v0, GLint v1)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint v[2];
   v[0] = v0;
   v[1] = v1;
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, 1, v, GL_INT_VEC2);
}

void GLAPIENTRY
_mesa_Uniform3iARB(GLint location, GLint v0, GLint v1, GLint v2)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint v[3];
   v[0] = v0;
   v[1] = v1;
   v[2] = v2;
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, 1, v, GL_INT_VEC3);
}

void GLAPIENTRY
_mesa_Uniform4iARB(GLint location, GLint v0, GLint v1, GLint v2, GLint v3)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint v[4];
   v[0] = v0;
   v[1] = v1;
   v[2] = v2;
   v[3] = v3;
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, 1, v, GL_INT_VEC4);
}

void GLAPIENTRY
_mesa_Uniform1fvARB(GLint location, GLsizei count, const GLfloat * value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, count, value, GL_FLOAT);
}

void GLAPIENTRY
_mesa_Uniform2fvARB(GLint location, GLsizei count, const GLfloat * value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, count, value, GL_FLOAT_VEC2);
}

void GLAPIENTRY
_mesa_Uniform3fvARB(GLint location, GLsizei count, const GLfloat * value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, count, value, GL_FLOAT_VEC3);
}

void GLAPIENTRY
_mesa_Uniform4fvARB(GLint location, GLsizei count, const GLfloat * value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, count, value, GL_FLOAT_VEC4);
}

void GLAPIENTRY
_mesa_Uniform1ivARB(GLint location, GLsizei count, const GLint * value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, count, value, GL_INT);
}

void GLAPIENTRY
_mesa_Uniform2ivARB(GLint location, GLsizei count, const GLint * value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, count, value, GL_INT_VEC2);
}

void GLAPIENTRY
_mesa_Uniform3ivARB(GLint location, GLsizei count, const GLint * value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, count, value, GL_INT_VEC3);
}

void GLAPIENTRY
_mesa_Uniform4ivARB(GLint location, GLsizei count, const GLint * value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, count, value, GL_INT_VEC4);
}


/** OpenGL 3.0 GLuint-valued functions **/
void GLAPIENTRY
_mesa_Uniform1ui(GLint location, GLuint v0)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, 1, &v0, GL_UNSIGNED_INT);
}

void GLAPIENTRY
_mesa_Uniform2ui(GLint location, GLuint v0, GLuint v1)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint v[2];
   v[0] = v0;
   v[1] = v1;
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, 1, v, GL_UNSIGNED_INT_VEC2);
}

void GLAPIENTRY
_mesa_Uniform3ui(GLint location, GLuint v0, GLuint v1, GLuint v2)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint v[3];
   v[0] = v0;
   v[1] = v1;
   v[2] = v2;
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, 1, v, GL_UNSIGNED_INT_VEC3);
}

void GLAPIENTRY
_mesa_Uniform4ui(GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3)
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint v[4];
   v[0] = v0;
   v[1] = v1;
   v[2] = v2;
   v[3] = v3;
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, 1, v, GL_UNSIGNED_INT_VEC4);
}

void GLAPIENTRY
_mesa_Uniform1uiv(GLint location, GLsizei count, const GLuint *value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, count, value, GL_UNSIGNED_INT);
}

void GLAPIENTRY
_mesa_Uniform2uiv(GLint location, GLsizei count, const GLuint *value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, count, value, GL_UNSIGNED_INT_VEC2);
}

void GLAPIENTRY
_mesa_Uniform3uiv(GLint location, GLsizei count, const GLuint *value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, count, value, GL_UNSIGNED_INT_VEC3);
}

void GLAPIENTRY
_mesa_Uniform4uiv(GLint location, GLsizei count, const GLuint *value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform(ctx, ctx->Shader.ActiveProgram, location, count, value, GL_UNSIGNED_INT_VEC4);
}



void GLAPIENTRY
_mesa_UniformMatrix2fvARB(GLint location, GLsizei count, GLboolean transpose,
                          const GLfloat * value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform_matrix(ctx, ctx->Shader.ActiveProgram,
			2, 2, location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix3fvARB(GLint location, GLsizei count, GLboolean transpose,
                          const GLfloat * value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform_matrix(ctx, ctx->Shader.ActiveProgram,
			3, 3, location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix4fvARB(GLint location, GLsizei count, GLboolean transpose,
                          const GLfloat * value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform_matrix(ctx, ctx->Shader.ActiveProgram,
			4, 4, location, count, transpose, value);
}


/**
 * Non-square UniformMatrix are OpenGL 2.1
 */
void GLAPIENTRY
_mesa_UniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform_matrix(ctx, ctx->Shader.ActiveProgram,
			2, 3, location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform_matrix(ctx, ctx->Shader.ActiveProgram,
			3, 2, location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform_matrix(ctx, ctx->Shader.ActiveProgram,
			2, 4, location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform_matrix(ctx, ctx->Shader.ActiveProgram,
			4, 2, location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform_matrix(ctx, ctx->Shader.ActiveProgram,
			3, 4, location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_uniform_matrix(ctx, ctx->Shader.ActiveProgram,
			4, 3, location, count, transpose, value);
}


void GLAPIENTRY
_mesa_GetnUniformfvARB(GLhandleARB program, GLint location,
                       GLsizei bufSize, GLfloat *params)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_get_uniform(ctx, program, location, bufSize, GLSL_TYPE_FLOAT, params);
}

void GLAPIENTRY
_mesa_GetUniformfvARB(GLhandleARB program, GLint location, GLfloat *params)
{
   _mesa_GetnUniformfvARB(program, location, INT_MAX, params);
}


void GLAPIENTRY
_mesa_GetnUniformivARB(GLhandleARB program, GLint location,
                       GLsizei bufSize, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_get_uniform(ctx, program, location, bufSize, GLSL_TYPE_INT, params);
}

void GLAPIENTRY
_mesa_GetUniformivARB(GLhandleARB program, GLint location, GLint *params)
{
   _mesa_GetnUniformivARB(program, location, INT_MAX, params);
}


/* GL3 */
void GLAPIENTRY
_mesa_GetnUniformuivARB(GLhandleARB program, GLint location,
                        GLsizei bufSize, GLuint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   _mesa_get_uniform(ctx, program, location, bufSize, GLSL_TYPE_UINT, params);
}

void GLAPIENTRY
_mesa_GetUniformuiv(GLhandleARB program, GLint location, GLuint *params)
{
   _mesa_GetnUniformuivARB(program, location, INT_MAX, params);
}


/* GL4 */
void GLAPIENTRY
_mesa_GetnUniformdvARB(GLhandleARB program, GLint location,
                        GLsizei bufSize, GLdouble *params)
{
   GET_CURRENT_CONTEXT(ctx);

   (void) program;
   (void) location;
   (void) bufSize;
   (void) params;

   /*
   _mesa_get_uniform(ctx, program, location, bufSize, GLSL_TYPE_DOUBLE, params);
   */
   _mesa_error(ctx, GL_INVALID_OPERATION, "glGetUniformdvARB"
               "(GL_ARB_gpu_shader_fp64 not implemented)");
}

void GLAPIENTRY
_mesa_GetUniformdv(GLhandleARB program, GLint location, GLdouble *params)
{
   _mesa_GetnUniformdvARB(program, location, INT_MAX, params);
}


GLint GLAPIENTRY
_mesa_GetUniformLocationARB(GLhandleARB programObj, const GLcharARB *name)
{
   struct gl_shader_program *shProg;

   GET_CURRENT_CONTEXT(ctx);

   shProg = _mesa_lookup_shader_program_err(ctx, programObj,
					    "glGetUniformLocation");
   if (!shProg)
      return -1;

   /* Page 80 (page 94 of the PDF) of the OpenGL 2.1 spec says:
    *
    *     "If program has not been successfully linked, the error
    *     INVALID_OPERATION is generated."
    */
   if (shProg->LinkStatus == GL_FALSE) {
      _mesa_error(ctx, GL_INVALID_OPERATION,
		  "glGetUniformLocation(program not linked)");
      return -1;
   }

   return _mesa_get_uniform_location(ctx, shProg, name);
}


/**
 * Plug in shader uniform-related functions into API dispatch table.
 */
void
_mesa_init_shader_uniform_dispatch(struct _glapi_table *exec)
{
#if FEATURE_GL
   SET_Uniform1fARB(exec, _mesa_Uniform1fARB);
   SET_Uniform2fARB(exec, _mesa_Uniform2fARB);
   SET_Uniform3fARB(exec, _mesa_Uniform3fARB);
   SET_Uniform4fARB(exec, _mesa_Uniform4fARB);
   SET_Uniform1iARB(exec, _mesa_Uniform1iARB);
   SET_Uniform2iARB(exec, _mesa_Uniform2iARB);
   SET_Uniform3iARB(exec, _mesa_Uniform3iARB);
   SET_Uniform4iARB(exec, _mesa_Uniform4iARB);
   SET_Uniform1fvARB(exec, _mesa_Uniform1fvARB);
   SET_Uniform2fvARB(exec, _mesa_Uniform2fvARB);
   SET_Uniform3fvARB(exec, _mesa_Uniform3fvARB);
   SET_Uniform4fvARB(exec, _mesa_Uniform4fvARB);
   SET_Uniform1ivARB(exec, _mesa_Uniform1ivARB);
   SET_Uniform2ivARB(exec, _mesa_Uniform2ivARB);
   SET_Uniform3ivARB(exec, _mesa_Uniform3ivARB);
   SET_Uniform4ivARB(exec, _mesa_Uniform4ivARB);
   SET_UniformMatrix2fvARB(exec, _mesa_UniformMatrix2fvARB);
   SET_UniformMatrix3fvARB(exec, _mesa_UniformMatrix3fvARB);
   SET_UniformMatrix4fvARB(exec, _mesa_UniformMatrix4fvARB);

   SET_GetActiveUniformARB(exec, _mesa_GetActiveUniformARB);
   SET_GetUniformLocationARB(exec, _mesa_GetUniformLocationARB);
   SET_GetUniformfvARB(exec, _mesa_GetUniformfvARB);
   SET_GetUniformivARB(exec, _mesa_GetUniformivARB);

   /* OpenGL 2.1 */
   SET_UniformMatrix2x3fv(exec, _mesa_UniformMatrix2x3fv);
   SET_UniformMatrix3x2fv(exec, _mesa_UniformMatrix3x2fv);
   SET_UniformMatrix2x4fv(exec, _mesa_UniformMatrix2x4fv);
   SET_UniformMatrix4x2fv(exec, _mesa_UniformMatrix4x2fv);
   SET_UniformMatrix3x4fv(exec, _mesa_UniformMatrix3x4fv);
   SET_UniformMatrix4x3fv(exec, _mesa_UniformMatrix4x3fv);

   /* OpenGL 3.0 */
   SET_Uniform1uiEXT(exec, _mesa_Uniform1ui);
   SET_Uniform2uiEXT(exec, _mesa_Uniform2ui);
   SET_Uniform3uiEXT(exec, _mesa_Uniform3ui);
   SET_Uniform4uiEXT(exec, _mesa_Uniform4ui);
   SET_Uniform1uivEXT(exec, _mesa_Uniform1uiv);
   SET_Uniform2uivEXT(exec, _mesa_Uniform2uiv);
   SET_Uniform3uivEXT(exec, _mesa_Uniform3uiv);
   SET_Uniform4uivEXT(exec, _mesa_Uniform4uiv);
   SET_GetUniformuivEXT(exec, _mesa_GetUniformuiv);

   /* GL_ARB_robustness */
   SET_GetnUniformfvARB(exec, _mesa_GetnUniformfvARB);
   SET_GetnUniformivARB(exec, _mesa_GetnUniformivARB);
   SET_GetnUniformuivARB(exec, _mesa_GetnUniformuivARB);
   SET_GetnUniformdvARB(exec, _mesa_GetnUniformdvARB); /* GL 4.0 */

#endif /* FEATURE_GL */
}
