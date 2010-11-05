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

#ifndef SLANG_TYPEINFO_H
#define SLANG_TYPEINFO_H 1

#include "main/imports.h"
#include "main/mtypes.h"
#include "slang_log.h"
#include "slang_utility.h"
#include "slang_vartable.h"


struct slang_operation_;

struct slang_name_space_;



/**
 * Holds complete information about vector swizzle - the <swizzle>
 * array contains vector component source indices, where 0 is "x", 1
 * is "y", 2 is "z" and 3 is "w".
 * Example: "xwz" --> { 3, { 0, 3, 2, not used } }.
 */
typedef struct slang_swizzle_
{
   GLuint num_components;
   GLuint swizzle[4];
} slang_swizzle;

extern GLboolean
_slang_is_swizzle(const char *field, GLuint rows, slang_swizzle *swz);


typedef enum slang_type_variant_
{
   SLANG_VARIANT,    /* the default */
   SLANG_INVARIANT   /* indicates the "invariant" keyword */
} slang_type_variant;


typedef enum slang_type_centroid_
{
   SLANG_CENTER,    /* the default */
   SLANG_CENTROID   /* indicates the "centroid" keyword */
} slang_type_centroid;


typedef enum slang_type_qualifier_
{
   SLANG_QUAL_NONE,
   SLANG_QUAL_CONST,
   SLANG_QUAL_ATTRIBUTE,
   SLANG_QUAL_VARYING,
   SLANG_QUAL_UNIFORM,
   SLANG_QUAL_OUT,
   SLANG_QUAL_INOUT,
   SLANG_QUAL_FIXEDOUTPUT,      /* internal */
   SLANG_QUAL_FIXEDINPUT        /* internal */
} slang_type_qualifier;


typedef enum slang_type_precision_
{
   SLANG_PREC_DEFAULT,
   SLANG_PREC_LOW,
   SLANG_PREC_MEDIUM,
   SLANG_PREC_HIGH
} slang_type_precision;


/**
 * The basic shading language types (float, vec4, mat3, etc)
 */
typedef enum slang_type_specifier_type_
{
   SLANG_SPEC_VOID,
   SLANG_SPEC_BOOL,
   SLANG_SPEC_BVEC2,
   SLANG_SPEC_BVEC3,
   SLANG_SPEC_BVEC4,
   SLANG_SPEC_INT,
   SLANG_SPEC_IVEC2,
   SLANG_SPEC_IVEC3,
   SLANG_SPEC_IVEC4,
   SLANG_SPEC_FLOAT,
   SLANG_SPEC_VEC2,
   SLANG_SPEC_VEC3,
   SLANG_SPEC_VEC4,
   SLANG_SPEC_MAT2,
   SLANG_SPEC_MAT3,
   SLANG_SPEC_MAT4,
   SLANG_SPEC_MAT23,
   SLANG_SPEC_MAT32,
   SLANG_SPEC_MAT24,
   SLANG_SPEC_MAT42,
   SLANG_SPEC_MAT34,
   SLANG_SPEC_MAT43,
   SLANG_SPEC_SAMPLER1D,
   SLANG_SPEC_SAMPLER2D,
   SLANG_SPEC_SAMPLER3D,
   SLANG_SPEC_SAMPLERCUBE,
   SLANG_SPEC_SAMPLER2DRECT,
   SLANG_SPEC_SAMPLER1DSHADOW,
   SLANG_SPEC_SAMPLER2DSHADOW,
   SLANG_SPEC_SAMPLER2DRECTSHADOW,
   SLANG_SPEC_STRUCT,
   SLANG_SPEC_ARRAY
} slang_type_specifier_type;


extern slang_type_specifier_type
slang_type_specifier_type_from_string(const char *);

extern const char *
slang_type_specifier_type_to_string(slang_type_specifier_type);


/**
 * Describes more sophisticated types, like structs and arrays.
 */
typedef struct slang_type_specifier_
{
   slang_type_specifier_type type;
   struct slang_struct_ *_struct;         /**< if type == SLANG_SPEC_STRUCT */
   struct slang_type_specifier_ *_array;  /**< if type == SLANG_SPEC_ARRAY */
} slang_type_specifier;


extern GLvoid
slang_type_specifier_ctr(slang_type_specifier *);

extern GLvoid
slang_type_specifier_dtr(slang_type_specifier *);

extern slang_type_specifier *
slang_type_specifier_new(slang_type_specifier_type type,
                         struct slang_struct_ *_struct,
                         struct slang_type_specifier_ *_array);


extern GLboolean
slang_type_specifier_copy(slang_type_specifier *, const slang_type_specifier *);

extern GLboolean
slang_type_specifier_equal(const slang_type_specifier *,
                           const slang_type_specifier *);


extern GLboolean
slang_type_specifier_compatible(const slang_type_specifier * x,
                                const slang_type_specifier * y);


typedef struct slang_fully_specified_type_
{
   slang_type_qualifier qualifier;
   slang_type_specifier specifier;
   slang_type_precision precision;
   slang_type_variant variant;
   slang_type_centroid centroid;
   GLint array_len;           /**< -1 if not an array type */
} slang_fully_specified_type;

extern int
slang_fully_specified_type_construct(slang_fully_specified_type *);

extern void
slang_fully_specified_type_destruct(slang_fully_specified_type *);

extern int
slang_fully_specified_type_copy(slang_fully_specified_type *,
				const slang_fully_specified_type *);



typedef struct slang_typeinfo_
{
   GLboolean can_be_referenced;
   GLboolean is_swizzled;
   slang_swizzle swz;
   slang_type_specifier spec;
   GLuint array_len;
} slang_typeinfo;

extern GLboolean
slang_typeinfo_construct(slang_typeinfo *);

extern GLvoid
slang_typeinfo_destruct(slang_typeinfo *);


extern GLboolean
_slang_typeof_operation(struct slang_operation_ *,
                         const struct slang_name_space_ *,
                         slang_typeinfo *, slang_atom_pool *,
                         slang_info_log *log);

extern GLboolean
_slang_type_is_matrix(slang_type_specifier_type);

extern GLboolean
_slang_type_is_vector(slang_type_specifier_type);

extern GLboolean
_slang_type_is_float_vec_mat(slang_type_specifier_type);

extern slang_type_specifier_type
_slang_type_base(slang_type_specifier_type);

extern GLuint
_slang_type_dim(slang_type_specifier_type);

extern GLenum
_slang_gltype_from_specifier(const slang_type_specifier *type);

#endif
