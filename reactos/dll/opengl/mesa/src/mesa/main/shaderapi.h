/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2004-2007  Brian Paul   All Rights Reserved.
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


#ifndef SHADERAPI_H
#define SHADERAPI_H


#include "glheader.h"


#ifdef __cplusplus
extern "C" {
#endif


struct _glapi_table;
struct gl_context;
struct gl_shader_program;

extern GLint
_mesa_sizeof_glsl_type(GLenum type);

extern void
_mesa_copy_string(GLchar *dst, GLsizei maxLength,
                  GLsizei *length, const GLchar *src);

extern void
_mesa_use_program(struct gl_context *ctx, struct gl_shader_program *shProg);

extern void
_mesa_active_program(struct gl_context *ctx, struct gl_shader_program *shProg,
		     const char *caller);

extern void
_mesa_init_shader_dispatch(struct _glapi_table *exec);

extern unsigned
_mesa_count_active_attribs(struct gl_shader_program *shProg);

extern size_t
_mesa_longest_attribute_name_length(struct gl_shader_program *shProg);

extern void GLAPIENTRY
_mesa_AttachObjectARB(GLhandleARB, GLhandleARB);

extern void  GLAPIENTRY
_mesa_CompileShaderARB(GLhandleARB);

extern GLhandleARB GLAPIENTRY
_mesa_CreateProgramObjectARB(void);

extern GLhandleARB GLAPIENTRY
_mesa_CreateShaderObjectARB(GLenum type);

extern void GLAPIENTRY
_mesa_DeleteObjectARB(GLhandleARB obj);

extern void GLAPIENTRY
_mesa_DetachObjectARB(GLhandleARB, GLhandleARB);

extern void GLAPIENTRY
_mesa_GetAttachedObjectsARB(GLhandleARB, GLsizei, GLsizei *, GLhandleARB *);

extern GLint GLAPIENTRY
_mesa_GetFragDataLocation(GLuint program, const GLchar *name);

extern GLhandleARB GLAPIENTRY
_mesa_GetHandleARB(GLenum pname);

extern void GLAPIENTRY
_mesa_GetInfoLogARB(GLhandleARB, GLsizei, GLsizei *, GLcharARB *);

extern void GLAPIENTRY
_mesa_GetObjectParameterfvARB(GLhandleARB, GLenum, GLfloat *);

extern void GLAPIENTRY
_mesa_GetObjectParameterivARB(GLhandleARB, GLenum, GLint *);

extern void GLAPIENTRY
_mesa_GetShaderSourceARB(GLhandleARB, GLsizei, GLsizei *, GLcharARB *);

extern GLboolean GLAPIENTRY
_mesa_IsProgram(GLuint name);

extern GLboolean GLAPIENTRY
_mesa_IsShader(GLuint name);

extern void GLAPIENTRY
_mesa_LinkProgramARB(GLhandleARB programObj);

extern void GLAPIENTRY
_mesa_ShaderSourceARB(GLhandleARB, GLsizei, const GLcharARB* *, const GLint *);

extern void GLAPIENTRY
_mesa_UseProgramObjectARB(GLhandleARB);

extern void GLAPIENTRY
_mesa_ValidateProgramARB(GLhandleARB);


extern void GLAPIENTRY
_mesa_BindAttribLocationARB(GLhandleARB, GLuint, const GLcharARB *);

extern void GLAPIENTRY
_mesa_BindFragDataLocation(GLuint program, GLuint colorNumber,
                           const GLchar *name);

extern void GLAPIENTRY
_mesa_GetActiveAttribARB(GLhandleARB, GLuint, GLsizei, GLsizei *, GLint *,
                         GLenum *, GLcharARB *);

extern GLint GLAPIENTRY
_mesa_GetAttribLocationARB(GLhandleARB, const GLcharARB *);



extern void GLAPIENTRY
_mesa_AttachShader(GLuint program, GLuint shader);

extern GLuint GLAPIENTRY
_mesa_CreateShader(GLenum);

extern GLuint GLAPIENTRY
_mesa_CreateProgram(void);

extern void GLAPIENTRY
_mesa_DeleteProgram(GLuint program);

extern void GLAPIENTRY
_mesa_DeleteShader(GLuint shader);

extern void GLAPIENTRY
_mesa_DetachShader(GLuint program, GLuint shader);

extern void GLAPIENTRY
_mesa_GetAttachedShaders(GLuint program, GLsizei maxCount,
                         GLsizei *count, GLuint *obj);

extern void GLAPIENTRY
_mesa_GetProgramiv(GLuint program, GLenum pname, GLint *params);

extern void GLAPIENTRY
_mesa_GetProgramInfoLog(GLuint program, GLsizei bufSize,
                        GLsizei *length, GLchar *infoLog);

extern void GLAPIENTRY
_mesa_GetShaderiv(GLuint shader, GLenum pname, GLint *params);

extern void GLAPIENTRY
_mesa_GetShaderInfoLog(GLuint shader, GLsizei bufSize,
                       GLsizei *length, GLchar *infoLog);


extern void GLAPIENTRY
_mesa_GetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype,
                               GLint *range, GLint *precision);

extern void GLAPIENTRY
_mesa_ReleaseShaderCompiler(void);

extern void GLAPIENTRY
_mesa_ShaderBinary(GLint n, const GLuint *shaders, GLenum binaryformat,
                   const void* binary, GLint length);

extern void GLAPIENTRY
_mesa_ProgramParameteriARB(GLuint program, GLenum pname,
                           GLint value);
void
_mesa_use_shader_program(struct gl_context *ctx, GLenum type,
			 struct gl_shader_program *shProg);

extern void GLAPIENTRY
_mesa_UseShaderProgramEXT(GLenum type, GLuint program);

extern void GLAPIENTRY
_mesa_ActiveProgramEXT(GLuint program);

extern GLuint GLAPIENTRY
_mesa_CreateShaderProgramEXT(GLenum type, const GLchar *string);


#ifdef __cplusplus
}
#endif

#endif /* SHADERAPI_H */
