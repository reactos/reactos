/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 2005-2006  Brian Paul   All Rights Reserved.
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

#ifndef SLANG_STORAGE_H
#define SLANG_STORAGE_H

#include "slang_compile.h"


/*
 * Program variable data storage is kept completely transparent to the
 * front-end compiler. It is up to the back-end how the data is
 * actually allocated. The slang_storage_type enum provides the basic
 * information about how the memory is interpreted. This abstract
 * piece of memory is called a data slot. A data slot of a particular
 * type has a fixed size.
 *
 * For now, only the three basic types are supported, that is bool,
 * int and float. Other built-in types like vector or matrix can
 * easily be decomposed into a series of basic types.
 *
 * If the vec4 module is enabled, 4-component vectors of floats are
 * used when possible. 4x4 matrices are constructed of 4 vec4 slots.
 */
typedef enum slang_storage_type_
{
   /* core */
   SLANG_STORE_AGGREGATE,
   SLANG_STORE_BOOL,
   SLANG_STORE_INT,
   SLANG_STORE_FLOAT,
   /* vec4 */
   SLANG_STORE_VEC4
} slang_storage_type;


/**
 * The slang_storage_array structure groups data slots of the same
 * type into an array. This array has a fixed length. Arrays are
 * required to have a size equal to the sum of sizes of its
 * elements. They are also required to support indirect
 * addressing. That is, if B references first data slot in the array,
 * S is the size of the data slot and I is the integral index that is
 * not known at compile time, B+I*S references I-th data slot.
 *
 * This structure is also used to break down built-in data types that
 * are not supported directly.  Vectors, like vec3, are constructed
 * from arrays of their basic types. Matrices are formed of an array
 * of column vectors, which are in turn processed as other vectors.
 */
typedef struct slang_storage_array_
{
   slang_storage_type type;
   struct slang_storage_aggregate_ *aggregate;
   GLuint length;
} slang_storage_array;

GLboolean slang_storage_array_construct (slang_storage_array *);
GLvoid slang_storage_array_destruct (slang_storage_array *);


/**
 * The slang_storage_aggregate structure relaxes the indirect
 * addressing requirement for slang_storage_array
 * structure. Aggregates are always accessed statically - its member
 * addresses are well-known at compile time. For example, user-defined
 * types are implemented as aggregates. Aggregates can collect data of
 * a different type.
 */
typedef struct slang_storage_aggregate_
{
   slang_storage_array *arrays;
   GLuint count;
} slang_storage_aggregate;

GLboolean slang_storage_aggregate_construct (slang_storage_aggregate *);
GLvoid slang_storage_aggregate_destruct (slang_storage_aggregate *);


extern GLboolean
_slang_aggregate_variable(slang_storage_aggregate *agg,
                          slang_type_specifier *spec,
                          GLuint array_len,
                          slang_function_scope *funcs,
                          slang_struct_scope *structs,
                          slang_variable_scope *vars,
                          slang_atom_pool *atoms);

/*
 * Returns the size (in machine units) of the given storage type.
 * It is an error to pass-in SLANG_STORE_AGGREGATE.
 * Returns 0 on error.
 */
extern GLuint
_slang_sizeof_type (slang_storage_type);


/**
 * Returns total size (in machine units) of the given aggregate.
 * Returns 0 on error.
 */
extern GLuint
_slang_sizeof_aggregate (const slang_storage_aggregate *);


#if 0
/**
 * Converts structured aggregate to a flat one, with arrays of generic
 * type being one-element long.  Returns GL_TRUE on success.  Returns
 * GL_FALSE otherwise.
 */
extern GLboolean
_slang_flatten_aggregate (slang_storage_aggregate *,
                          const slang_storage_aggregate *);

#endif

#endif /* SLANG_STORAGE_H */
