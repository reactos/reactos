/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2007  Brian Paul   All Rights Reserved.
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


#ifndef QUERYOBJ_H
#define QUERYOBJ_H


#include "main/mfeatures.h"
#include "main/mtypes.h"
#include "main/hash.h"


#if FEATURE_queryobj

static inline struct gl_query_object *
_mesa_lookup_query_object(struct gl_context *ctx, GLuint id)
{
   return (struct gl_query_object *)
      _mesa_HashLookup(ctx->Query.QueryObjects, id);
}


extern void
_mesa_init_query_object_functions(struct dd_function_table *driver);

extern void
_mesa_init_queryobj_dispatch(struct _glapi_table *disp);

#else /* FEATURE_queryobj */

static inline struct gl_query_object *
_mesa_lookup_query_object(struct gl_context *ctx, GLuint id)
{
   return NULL;
}

static inline void
_mesa_init_query_object_functions(struct dd_function_table *driver)
{
}

static inline void
_mesa_init_queryobj_dispatch(struct _glapi_table *disp)
{
}

#endif /* FEATURE_queryobj */

extern void
_mesa_init_queryobj(struct gl_context *ctx);

extern void
_mesa_free_queryobj_data(struct gl_context *ctx);


#endif /* QUERYOBJ_H */
