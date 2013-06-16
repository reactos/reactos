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


#ifndef TRANSFORM_FEEDBACK_H
#define TRANSFORM_FEEDBACK_H

#include "compiler.h"
#include "glheader.h"
#include "mfeatures.h"

struct _glapi_table;
struct dd_function_table;
struct gl_context;

extern void
_mesa_init_transform_feedback(struct gl_context *ctx);

extern void
_mesa_free_transform_feedback(struct gl_context *ctx);

#if FEATURE_EXT_transform_feedback

extern GLboolean
_mesa_validate_primitive_mode(struct gl_context *ctx, GLenum mode);

extern GLboolean
_mesa_validate_transform_feedback_buffers(struct gl_context *ctx);


extern void
_mesa_init_transform_feedback_functions(struct dd_function_table *driver);

extern void
_mesa_init_transform_feedback_dispatch(struct _glapi_table *disp);


/*** GL_EXT_transform_feedback ***/

extern void GLAPIENTRY
_mesa_BeginTransformFeedback(GLenum mode);

extern void GLAPIENTRY
_mesa_EndTransformFeedback(void);

extern void GLAPIENTRY
_mesa_BindBufferRange(GLenum target, GLuint index,
                      GLuint buffer, GLintptr offset, GLsizeiptr size);

extern void GLAPIENTRY
_mesa_BindBufferBase(GLenum target, GLuint index, GLuint buffer);

extern void GLAPIENTRY
_mesa_BindBufferOffsetEXT(GLenum target, GLuint index, GLuint buffer,
                          GLintptr offset);

extern void GLAPIENTRY
_mesa_TransformFeedbackVaryings(GLuint program, GLsizei count,
                                const GLchar **varyings, GLenum bufferMode);

extern void GLAPIENTRY
_mesa_GetTransformFeedbackVarying(GLuint program, GLuint index,
                                  GLsizei bufSize, GLsizei *length,
                                  GLsizei *size, GLenum *type, GLchar *name);



/*** GL_ARB_transform_feedback2 ***/

struct gl_transform_feedback_object *
_mesa_lookup_transform_feedback_object(struct gl_context *ctx, GLuint name);

extern void GLAPIENTRY
_mesa_GenTransformFeedbacks(GLsizei n, GLuint *names);

extern GLboolean GLAPIENTRY
_mesa_IsTransformFeedback(GLuint name);

extern void GLAPIENTRY
_mesa_BindTransformFeedback(GLenum target, GLuint name);

extern void GLAPIENTRY
_mesa_DeleteTransformFeedbacks(GLsizei n, const GLuint *names);

extern void GLAPIENTRY
_mesa_PauseTransformFeedback(void);

extern void GLAPIENTRY
_mesa_ResumeTransformFeedback(void);

#else /* FEATURE_EXT_transform_feedback */

static inline GLboolean
_mesa_validate_primitive_mode(struct gl_context *ctx, GLenum mode)
{
   return GL_TRUE;
}

static inline GLboolean
_mesa_validate_transform_feedback_buffers(struct gl_context *ctx)
{
   return GL_TRUE;
}

static inline void
_mesa_init_transform_feedback_functions(struct dd_function_table *driver)
{
}

static inline void
_mesa_init_transform_feedback_dispatch(struct _glapi_table *disp)
{
}

#endif /* FEATURE_EXT_transform_feedback */

#endif /* TRANSFORM_FEEDBACK_H */
