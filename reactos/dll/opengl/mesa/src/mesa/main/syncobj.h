/*
 * Copyright © 2009 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file syncobj.h
 * Sync object management.
 *
 * \author Ian Romanick <ian.d.romanick@intel.com>
 */

#ifndef SYNCOBJ_H
#define SYNCOBJ_H

#include "glheader.h"
#include "mfeatures.h"

struct _glapi_table;
struct dd_function_table;
struct gl_context;
struct gl_sync_object;

#if FEATURE_ARB_sync

extern void
_mesa_init_sync_object_functions(struct dd_function_table *driver);

extern void
_mesa_init_sync_dispatch(struct _glapi_table *disp);

extern void
_mesa_init_sync(struct gl_context *);

extern void
_mesa_free_sync_data(struct gl_context *);

extern void
_mesa_ref_sync_object(struct gl_context *ctx, struct gl_sync_object *syncObj);

extern void
_mesa_unref_sync_object(struct gl_context *ctx, struct gl_sync_object *syncObj);

extern GLboolean GLAPIENTRY
_mesa_IsSync(GLsync sync);

extern void GLAPIENTRY
_mesa_DeleteSync(GLsync sync);

extern GLsync GLAPIENTRY
_mesa_FenceSync(GLenum condition, GLbitfield flags);

extern GLenum GLAPIENTRY
_mesa_ClientWaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout);

extern void GLAPIENTRY
_mesa_WaitSync(GLsync sync, GLbitfield flags, GLuint64 timeout);

extern void GLAPIENTRY
_mesa_GetSynciv(GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length,
		GLint *values);

#else /* FEATURE_ARB_sync */

#include "main/compiler.h"

static inline void
_mesa_init_sync_object_functions(struct dd_function_table *driver)
{
}

static inline void
_mesa_init_sync_dispatch(struct _glapi_table *disp)
{
}

static inline void
_mesa_init_sync(struct gl_context *ctx)
{
}

static inline void
_mesa_free_sync_data(struct gl_context *ctx)
{
}

static inline void
_mesa_ref_sync_object(struct gl_context *ctx, struct gl_sync_object *syncObj)
{
   ASSERT_NO_FEATURE();
}

static inline void
_mesa_unref_sync_object(struct gl_context *ctx, struct gl_sync_object *syncObj)
{
   ASSERT_NO_FEATURE();
}

#endif /* FEATURE_ARB_sync */

#endif /* SYNCOBJ_H */
