/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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


#ifndef OCCLUDE_H
#define OCCLUDE_H


extern struct gl_query_object *
_mesa_new_query_object(GLcontext *ctx, GLuint id);

extern void
_mesa_init_query(GLcontext *ctx);

extern void
_mesa_free_query_data(GLcontext *ctx);

extern void GLAPIENTRY
_mesa_GenQueriesARB(GLsizei n, GLuint *ids);

extern void GLAPIENTRY
_mesa_DeleteQueriesARB(GLsizei n, const GLuint *ids);

extern GLboolean GLAPIENTRY
_mesa_IsQueryARB(GLuint id);

extern void GLAPIENTRY
_mesa_BeginQueryARB(GLenum target, GLuint id);

extern void GLAPIENTRY
_mesa_EndQueryARB(GLenum target);

extern void GLAPIENTRY
_mesa_GetQueryivARB(GLenum target, GLenum pname, GLint *params);

extern void GLAPIENTRY
_mesa_GetQueryObjectivARB(GLuint id, GLenum pname, GLint *params);

extern void GLAPIENTRY
_mesa_GetQueryObjectuivARB(GLuint id, GLenum pname, GLuint *params);

extern void GLAPIENTRY
_mesa_GetQueryObjecti64vEXT(GLuint id, GLenum pname, GLint64EXT *params);

extern void GLAPIENTRY
_mesa_GetQueryObjectui64vEXT(GLuint id, GLenum pname, GLuint64EXT *params);


#endif /* OCCLUDE_H */
