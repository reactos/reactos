/**
 * \file accum.h
 * Accumulation buffer operations.
 * 
 * \if subset
 * (No-op)
 *
 * \endif
 */

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



#ifndef ACCUM_H
#define ACCUM_H

#include "main/glheader.h"
#include "main/mfeatures.h"

struct _glapi_table;
struct gl_context;
struct gl_renderbuffer;

#if FEATURE_accum

extern void GLAPIENTRY
_mesa_ClearAccum( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );

extern void
_mesa_init_accum_dispatch(struct _glapi_table *disp);

extern void
_mesa_accum(struct gl_context *ctx, GLenum op, GLfloat value);

extern void
_mesa_clear_accum_buffer(struct gl_context *ctx);

#else /* FEATURE_accum */

#include "main/compiler.h"

static inline void
_mesa_ClearAccum( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
{
   /* this is used in _mesa_PopAttrib */
   ASSERT_NO_FEATURE();
}

static inline void
_mesa_init_accum_dispatch(struct _glapi_table *disp)
{
}

static inline void
_mesa_accum(struct gl_context *ctx, GLenum op, GLfloat value)
{
}

static inline void
_mesa_clear_accum_buffer(struct gl_context *ctx)
{
}


#endif /* FEATURE_accum */

extern void
_mesa_init_accum( struct gl_context *ctx );

#endif /* ACCUM_H */
