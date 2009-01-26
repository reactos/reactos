/*
 * Mesa 3-D graphics library
 * Version:  7.3
 *
 * Copyright (C) 2004-2008  Brian Paul   All Rights Reserved.
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


#include "glheader.h"
#include "context.h"
#include "shaders.h"


/**
 * These are basically just wrappers/adaptors for calling the
 * ctx->Driver.foobar() GLSL-related functions.
 *
 * Things are biased toward the OpenGL 2.0 functions rather than the
 * ARB extensions (i.e. the ARB functions are layered on the 2.0 functions).
 *
 * The general idea here is to allow enough modularity such that a
 * completely different GLSL implemenation can be plugged in and co-exist
 * with Mesa's native GLSL code.
 */



void GLAPIENTRY
_mesa_AttachObjectARB(GLhandleARB program, GLhandleARB shader)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.AttachShader(ctx, program, shader);
}


void GLAPIENTRY
_mesa_AttachShader(GLuint program, GLuint shader)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.AttachShader(ctx, program, shader);
}


void GLAPIENTRY
_mesa_BindAttribLocationARB(GLhandleARB program, GLuint index,
                            const GLcharARB *name)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.BindAttribLocation(ctx, program, index, name);
}


void GLAPIENTRY
_mesa_CompileShaderARB(GLhandleARB shaderObj)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.CompileShader(ctx, shaderObj);
}


GLuint GLAPIENTRY
_mesa_CreateShader(GLenum type)
{
   GET_CURRENT_CONTEXT(ctx);
   return ctx->Driver.CreateShader(ctx, type);
}


GLhandleARB GLAPIENTRY
_mesa_CreateShaderObjectARB(GLenum type)
{
   GET_CURRENT_CONTEXT(ctx);
   return ctx->Driver.CreateShader(ctx, type);
}


GLuint GLAPIENTRY
_mesa_CreateProgram(void)
{
   GET_CURRENT_CONTEXT(ctx);
   return ctx->Driver.CreateProgram(ctx);
}


GLhandleARB GLAPIENTRY
_mesa_CreateProgramObjectARB(void)
{
   GET_CURRENT_CONTEXT(ctx);
   return ctx->Driver.CreateProgram(ctx);
}


void GLAPIENTRY
_mesa_DeleteObjectARB(GLhandleARB obj)
{
   if (obj) {
      GET_CURRENT_CONTEXT(ctx);
      if (ctx->Driver.IsProgram(ctx, obj)) {
         ctx->Driver.DeleteProgram2(ctx, obj);
      }
      else if (ctx->Driver.IsShader(ctx, obj)) {
         ctx->Driver.DeleteShader(ctx, obj);
      }
      else {
         /* error? */
      }
   }
}


void GLAPIENTRY
_mesa_DeleteProgram(GLuint name)
{
   if (name) {
      GET_CURRENT_CONTEXT(ctx);
      ctx->Driver.DeleteProgram2(ctx, name);
   }
}


void GLAPIENTRY
_mesa_DeleteShader(GLuint name)
{
   if (name) {
      GET_CURRENT_CONTEXT(ctx);
      ctx->Driver.DeleteShader(ctx, name);
   }
}


void GLAPIENTRY
_mesa_DetachObjectARB(GLhandleARB program, GLhandleARB shader)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.DetachShader(ctx, program, shader);
}


void GLAPIENTRY
_mesa_DetachShader(GLuint program, GLuint shader)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.DetachShader(ctx, program, shader);
}


void GLAPIENTRY
_mesa_GetActiveAttribARB(GLhandleARB program, GLuint index,
                         GLsizei maxLength, GLsizei * length, GLint * size,
                         GLenum * type, GLcharARB * name)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.GetActiveAttrib(ctx, program, index, maxLength, length, size,
                               type, name);
}


void GLAPIENTRY
_mesa_GetActiveUniformARB(GLhandleARB program, GLuint index,
                          GLsizei maxLength, GLsizei * length, GLint * size,
                          GLenum * type, GLcharARB * name)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.GetActiveUniform(ctx, program, index, maxLength, length, size,
                                type, name);
}


void GLAPIENTRY
_mesa_GetAttachedObjectsARB(GLhandleARB container, GLsizei maxCount,
                            GLsizei * count, GLhandleARB * obj)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.GetAttachedShaders(ctx, container, maxCount, count, obj);
}


void GLAPIENTRY
_mesa_GetAttachedShaders(GLuint program, GLsizei maxCount,
                         GLsizei *count, GLuint *obj)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.GetAttachedShaders(ctx, program, maxCount, count, obj);
}


GLint GLAPIENTRY
_mesa_GetAttribLocationARB(GLhandleARB program, const GLcharARB * name)
{
   GET_CURRENT_CONTEXT(ctx);
   return ctx->Driver.GetAttribLocation(ctx, program, name);
}


void GLAPIENTRY
_mesa_GetInfoLogARB(GLhandleARB object, GLsizei maxLength, GLsizei * length,
                    GLcharARB * infoLog)
{
   GET_CURRENT_CONTEXT(ctx);
   /* Implement in terms of GetProgramInfoLog, GetShaderInfoLog */
   if (ctx->Driver.IsProgram(ctx, object)) {
      ctx->Driver.GetProgramInfoLog(ctx, object, maxLength, length, infoLog);
   }
   else if (ctx->Driver.IsShader(ctx, object)) {
      ctx->Driver.GetShaderInfoLog(ctx, object, maxLength, length, infoLog);
   }
   else {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glGetInfoLogARB");
   }
}


void GLAPIENTRY
_mesa_GetObjectParameterivARB(GLhandleARB object, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   /* Implement in terms of GetProgramiv, GetShaderiv */
   if (ctx->Driver.IsProgram(ctx, object)) {
      if (pname == GL_OBJECT_TYPE_ARB) {
	 *params = GL_PROGRAM_OBJECT_ARB;
      }
      else {
	 ctx->Driver.GetProgramiv(ctx, object, pname, params);
      }
   }
   else if (ctx->Driver.IsShader(ctx, object)) {
      if (pname == GL_OBJECT_TYPE_ARB) {
	 *params = GL_SHADER_OBJECT_ARB;
      }
      else {
	 ctx->Driver.GetShaderiv(ctx, object, pname, params);
      }
   }
   else {
      _mesa_error(ctx, GL_INVALID_VALUE, "glGetObjectParameterivARB");
   }
}


void GLAPIENTRY
_mesa_GetObjectParameterfvARB(GLhandleARB object, GLenum pname,
                              GLfloat *params)
{
   GLint iparams[1];  /* XXX is one element enough? */
   _mesa_GetObjectParameterivARB(object, pname, iparams);
   params[0] = (GLfloat) iparams[0];
}


void GLAPIENTRY
_mesa_GetProgramiv(GLuint program, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.GetProgramiv(ctx, program, pname, params);
}


void GLAPIENTRY
_mesa_GetShaderiv(GLuint shader, GLenum pname, GLint *params)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.GetShaderiv(ctx, shader, pname, params);
}


void GLAPIENTRY
_mesa_GetProgramInfoLog(GLuint program, GLsizei bufSize,
                        GLsizei *length, GLchar *infoLog)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.GetProgramInfoLog(ctx, program, bufSize, length, infoLog);
}


void GLAPIENTRY
_mesa_GetShaderInfoLog(GLuint shader, GLsizei bufSize,
                       GLsizei *length, GLchar *infoLog)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.GetShaderInfoLog(ctx, shader, bufSize, length, infoLog);
}


void GLAPIENTRY
_mesa_GetShaderSourceARB(GLhandleARB shader, GLsizei maxLength,
                         GLsizei *length, GLcharARB *sourceOut)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.GetShaderSource(ctx, shader, maxLength, length, sourceOut);
}


void GLAPIENTRY
_mesa_GetUniformfvARB(GLhandleARB program, GLint location, GLfloat * params)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.GetUniformfv(ctx, program, location, params);
}


void GLAPIENTRY
_mesa_GetUniformivARB(GLhandleARB program, GLint location, GLint * params)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.GetUniformiv(ctx, program, location, params);
}



#if 0
GLint GLAPIENTRY
_mesa_GetUniformLocation(GLuint program, const GLcharARB *name)
{
   GET_CURRENT_CONTEXT(ctx);
   return ctx->Driver.GetUniformLocation(ctx, program, name);
}
#endif


GLhandleARB GLAPIENTRY
_mesa_GetHandleARB(GLenum pname)
{
   GET_CURRENT_CONTEXT(ctx);
   return ctx->Driver.GetHandle(ctx, pname);
}


GLint GLAPIENTRY
_mesa_GetUniformLocationARB(GLhandleARB programObj, const GLcharARB *name)
{
   GET_CURRENT_CONTEXT(ctx);
   return ctx->Driver.GetUniformLocation(ctx, programObj, name);
}


GLboolean GLAPIENTRY
_mesa_IsProgram(GLuint name)
{
   GET_CURRENT_CONTEXT(ctx);
   return ctx->Driver.IsProgram(ctx, name);
}


GLboolean GLAPIENTRY
_mesa_IsShader(GLuint name)
{
   GET_CURRENT_CONTEXT(ctx);
   return ctx->Driver.IsShader(ctx, name);
}


void GLAPIENTRY
_mesa_LinkProgramARB(GLhandleARB programObj)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.LinkProgram(ctx, programObj);
}


/**
 * Called via glShaderSource() and glShaderSourceARB() API functions.
 * Basically, concatenate the source code strings into one long string
 * and pass it to ctx->Driver.ShaderSource().
 */
void GLAPIENTRY
_mesa_ShaderSourceARB(GLhandleARB shaderObj, GLsizei count,
                      const GLcharARB ** string, const GLint * length)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint *offsets;
   GLsizei i, totalLength;
   GLcharARB *source;

   if (!shaderObj || string == NULL) {
      _mesa_error(ctx, GL_INVALID_VALUE, "glShaderSourceARB");
      return;
   }

   /*
    * This array holds offsets of where the appropriate string ends, thus the
    * last element will be set to the total length of the source code.
    */
   offsets = (GLint *) _mesa_malloc(count * sizeof(GLint));
   if (offsets == NULL) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glShaderSourceARB");
      return;
   }

   for (i = 0; i < count; i++) {
      if (string[i] == NULL) {
         _mesa_free((GLvoid *) offsets);
         _mesa_error(ctx, GL_INVALID_OPERATION, "glShaderSourceARB(null string)");
         return;
      }
      if (length == NULL || length[i] < 0)
         offsets[i] = _mesa_strlen(string[i]);
      else
         offsets[i] = length[i];
      /* accumulate string lengths */
      if (i > 0)
         offsets[i] += offsets[i - 1];
   }

   /* Total length of source string is sum off all strings plus two.
    * One extra byte for terminating zero, another extra byte to silence
    * valgrind warnings in the parser/grammer code.
    */
   totalLength = offsets[count - 1] + 2;
   source = (GLcharARB *) _mesa_malloc(totalLength * sizeof(GLcharARB));
   if (source == NULL) {
      _mesa_free((GLvoid *) offsets);
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glShaderSourceARB");
      return;
   }

   for (i = 0; i < count; i++) {
      GLint start = (i > 0) ? offsets[i - 1] : 0;
      _mesa_memcpy(source + start, string[i],
                   (offsets[i] - start) * sizeof(GLcharARB));
   }
   source[totalLength - 1] = '\0';
   source[totalLength - 2] = '\0';

   ctx->Driver.ShaderSource(ctx, shaderObj, source);

   _mesa_free(offsets);
}


void GLAPIENTRY
_mesa_Uniform1fARB(GLint location, GLfloat v0)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.Uniform(ctx, location, 1, &v0, GL_FLOAT);
}

void GLAPIENTRY
_mesa_Uniform2fARB(GLint location, GLfloat v0, GLfloat v1)
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat v[2];
   v[0] = v0;
   v[1] = v1;
   ctx->Driver.Uniform(ctx, location, 1, v, GL_FLOAT_VEC2);
}

void GLAPIENTRY
_mesa_Uniform3fARB(GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat v[3];
   v[0] = v0;
   v[1] = v1;
   v[2] = v2;
   ctx->Driver.Uniform(ctx, location, 1, v, GL_FLOAT_VEC3);
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
   ctx->Driver.Uniform(ctx, location, 1, v, GL_FLOAT_VEC4);
}

void GLAPIENTRY
_mesa_Uniform1iARB(GLint location, GLint v0)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.Uniform(ctx, location, 1, &v0, GL_INT);
}

void GLAPIENTRY
_mesa_Uniform2iARB(GLint location, GLint v0, GLint v1)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint v[2];
   v[0] = v0;
   v[1] = v1;
   ctx->Driver.Uniform(ctx, location, 1, v, GL_INT_VEC2);
}

void GLAPIENTRY
_mesa_Uniform3iARB(GLint location, GLint v0, GLint v1, GLint v2)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint v[3];
   v[0] = v0;
   v[1] = v1;
   v[2] = v2;
   ctx->Driver.Uniform(ctx, location, 1, v, GL_INT_VEC3);
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
   ctx->Driver.Uniform(ctx, location, 1, v, GL_INT_VEC4);
}

void GLAPIENTRY
_mesa_Uniform1fvARB(GLint location, GLsizei count, const GLfloat * value)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.Uniform(ctx, location, count, value, GL_FLOAT);
}

void GLAPIENTRY
_mesa_Uniform2fvARB(GLint location, GLsizei count, const GLfloat * value)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.Uniform(ctx, location, count, value, GL_FLOAT_VEC2);
}

void GLAPIENTRY
_mesa_Uniform3fvARB(GLint location, GLsizei count, const GLfloat * value)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.Uniform(ctx, location, count, value, GL_FLOAT_VEC3);
}

void GLAPIENTRY
_mesa_Uniform4fvARB(GLint location, GLsizei count, const GLfloat * value)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.Uniform(ctx, location, count, value, GL_FLOAT_VEC4);
}

void GLAPIENTRY
_mesa_Uniform1ivARB(GLint location, GLsizei count, const GLint * value)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.Uniform(ctx, location, count, value, GL_INT);
}

void GLAPIENTRY
_mesa_Uniform2ivARB(GLint location, GLsizei count, const GLint * value)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.Uniform(ctx, location, count, value, GL_INT_VEC2);
}

void GLAPIENTRY
_mesa_Uniform3ivARB(GLint location, GLsizei count, const GLint * value)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.Uniform(ctx, location, count, value, GL_INT_VEC3);
}

void GLAPIENTRY
_mesa_Uniform4ivARB(GLint location, GLsizei count, const GLint * value)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.Uniform(ctx, location, count, value, GL_INT_VEC4);
}


void GLAPIENTRY
_mesa_UniformMatrix2fvARB(GLint location, GLsizei count, GLboolean transpose,
                          const GLfloat * value)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.UniformMatrix(ctx, 2, 2, GL_FLOAT_MAT2,
                             location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix3fvARB(GLint location, GLsizei count, GLboolean transpose,
                          const GLfloat * value)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.UniformMatrix(ctx, 3, 3, GL_FLOAT_MAT3,
                             location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix4fvARB(GLint location, GLsizei count, GLboolean transpose,
                          const GLfloat * value)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.UniformMatrix(ctx, 4, 4, GL_FLOAT_MAT4,
                             location, count, transpose, value);
}


/**
 * Non-square UniformMatrix are OpenGL 2.1
 */
void GLAPIENTRY
_mesa_UniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.UniformMatrix(ctx, 2, 3, GL_FLOAT_MAT2x3,
                             location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.UniformMatrix(ctx, 3, 2, GL_FLOAT_MAT3x2,
                             location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.UniformMatrix(ctx, 2, 4, GL_FLOAT_MAT2x4,
                             location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.UniformMatrix(ctx, 4, 2, GL_FLOAT_MAT4x2,
                             location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.UniformMatrix(ctx, 3, 4, GL_FLOAT_MAT3x4,
                             location, count, transpose, value);
}

void GLAPIENTRY
_mesa_UniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose,
                         const GLfloat *value)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.UniformMatrix(ctx, 4, 3, GL_FLOAT_MAT4x3,
                             location, count, transpose, value);
}


void GLAPIENTRY
_mesa_UseProgramObjectARB(GLhandleARB program)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, _NEW_PROGRAM);
   ctx->Driver.UseProgram(ctx, program);
}


void GLAPIENTRY
_mesa_ValidateProgramARB(GLhandleARB program)
{
   GET_CURRENT_CONTEXT(ctx);
   ctx->Driver.ValidateProgram(ctx, program);
}

