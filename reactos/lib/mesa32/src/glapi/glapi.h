
/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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


#ifndef _GLAPI_H
#define _GLAPI_H


#include "GL/gl.h"

struct _glapi_table;

typedef void (*_glapi_warning_func)(void *ctx, const char *str, ...);


extern void *_glapi_Context;

extern struct _glapi_table *_glapi_Dispatch;


extern void
_glapi_noop_enable_warnings(GLboolean enable);

extern void
_glapi_set_warning_func(_glapi_warning_func func);

extern void
_glapi_check_multithread(void);


extern void
_glapi_set_context(void *context);


extern void *
_glapi_get_context(void);


extern void
_glapi_set_dispatch(struct _glapi_table *dispatch);


extern struct _glapi_table *
_glapi_get_dispatch(void);


extern int
_glapi_begin_dispatch_override(struct _glapi_table *override);


extern void
_glapi_end_dispatch_override(int layer);


struct _glapi_table *
_glapi_get_override_dispatch(int layer);


extern GLuint
_glapi_get_dispatch_table_size(void);


extern const char *
_glapi_get_version(void);


extern void
_glapi_check_table(const struct _glapi_table *table);


extern GLboolean
_glapi_add_entrypoint(const char *funcName, GLuint offset);


extern GLint
_glapi_get_proc_offset(const char *funcName);


extern const GLvoid *
_glapi_get_proc_address(const char *funcName);


extern const char *
_glapi_get_proc_name(GLuint offset);


#endif
