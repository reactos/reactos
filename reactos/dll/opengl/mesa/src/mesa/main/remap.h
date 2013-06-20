/*
 * Mesa 3-D graphics library
 * Version:  7.7
 *
 * Copyright (C) 2009 Chia-I Wu <olv@0xlab.org>
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#ifndef REMAP_H
#define REMAP_H


#include "main/compiler.h"
#include "main/mfeatures.h"

struct gl_function_pool_remap {
   int pool_index;
   int remap_index;
};

struct gl_function_remap {
   int func_index;
   int dispatch_offset; /* for sanity check */
};


#if FEATURE_remap_table

extern int
driDispatchRemapTable[];

extern const char *
_mesa_get_function_spec(int func_index);

extern int
_mesa_map_function_spec(const char *spec);

extern void
_mesa_map_function_array(const struct gl_function_remap *func_array);

extern void
_mesa_map_static_functions(void);

extern void
_mesa_init_remap_table(void);

#else /* FEATURE_remap_table */

static inline const char *
_mesa_get_function_spec(int func_index)
{
   return NULL;
}

static inline int
_mesa_map_function_spec(const char *spec)
{
   return -1;
}

static inline void
_mesa_map_function_array(const struct gl_function_remap *func_array)
{
}

static inline void
_mesa_map_static_functions(void)
{
}


static inline void
_mesa_init_remap_table(void)
{
}

#endif /* FEATURE_remap_table */


#endif /* REMAP_H */
