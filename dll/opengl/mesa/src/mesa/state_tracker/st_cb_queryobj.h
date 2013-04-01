/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef ST_CB_QUERYOBJ_H
#define ST_CB_QUERYOBJ_H


#include "main/mfeatures.h"
#include "main/mtypes.h"

/**
 * Subclass of gl_query_object
 */
struct st_query_object
{
   struct gl_query_object base;
   struct pipe_query *pq;
   unsigned type;  /**< PIPE_QUERY_x */
};


/**
 * Cast wrapper
 */
static INLINE struct st_query_object *
st_query_object(struct gl_query_object *q)
{
   return (struct st_query_object *) q;
}


#if FEATURE_queryobj

extern void
st_init_query_functions(struct dd_function_table *functions);

#else

static INLINE void
st_init_query_functions(struct dd_function_table *functions)
{
}

#endif /* FEATURE_queryobj */

#endif
