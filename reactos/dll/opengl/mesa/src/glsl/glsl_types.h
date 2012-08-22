/* -*- c++ -*- */
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

#pragma once
#ifndef GLSL_TYPES_H
#define GLSL_TYPES_H

#include <string.h>
#include <assert.h>
#include "main/mtypes.h" /* for gl_texture_index, C++'s enum rules are broken */

#ifdef __cplusplus
extern "C" {
#endif

struct _mesa_glsl_parse_state;
struct glsl_symbol_table;

extern void
_mesa_glsl_initialize_types(struct _mesa_glsl_parse_state *state);

extern void
_mesa_glsl_release_types(void);

#ifdef __cplusplus
}
#endif

enum glsl_base_type {
   GLSL_TYPE_UINT = 0,
   GLSL_TYPE_INT,
   GLSL_TYPE_FLOAT,
   GLSL_TYPE_BOOL,
   GLSL_TYPE_SAMPLER,
   GLSL_TYPE_STRUCT,
   GLSL_TYPE_ARRAY,
   GLSL_TYPE_VOID,
   GLSL_TYPE_ERROR
};

enum glsl_sampler_dim {
   GLSL_SAMPLER_DIM_1D = 0,
   GLSL_SAMPLER_DIM_2D,
   GLSL_SAMPLER_DIM_3D,
   GLSL_SAMPLER_DIM_CUBE,
   GLSL_SAMPLER_DIM_RECT,
   GLSL_SAMPLER_DIM_BUF,
   GLSL_SAMPLER_DIM_EXTERNAL
};

#ifdef __cplusplus
#include "GL/gl.h"
#include "ralloc.h"

struct glsl_type {
   GLenum gl_type;
   glsl_base_type base_type;

   unsigned sampler_dimensionality:3; /**< \see glsl_sampler_dim */
   unsigned sampler_shadow:1;
   unsigned sampler_array:1;
   unsigned sampler_type:2;    /**< Type of data returned using this sampler.
				* only \c GLSL_TYPE_FLOAT, \c GLSL_TYPE_INT,
				* and \c GLSL_TYPE_UINT are valid.
				*/

   /* Callers of this ralloc-based new need not call delete. It's
    * easier to just ralloc_free 'mem_ctx' (or any of its ancestors). */
   static void* operator new(size_t size)
   {
      if (glsl_type::mem_ctx == NULL) {
	 glsl_type::mem_ctx = ralloc_context(NULL);
	 assert(glsl_type::mem_ctx != NULL);
      }

      void *type;

      type = ralloc_size(glsl_type::mem_ctx, size);
      assert(type != NULL);

      return type;
   }

   /* If the user *does* call delete, that's OK, we will just
    * ralloc_free in that case. */
   static void operator delete(void *type)
   {
      ralloc_free(type);
   }

   /**
    * \name Vector and matrix element counts
    *
    * For scalars, each of these values will be 1.  For non-numeric types
    * these will be 0.
    */
   /*@{*/
   unsigned vector_elements:3; /**< 1, 2, 3, or 4 vector elements. */
   unsigned matrix_columns:3;  /**< 1, 2, 3, or 4 matrix columns. */
   /*@}*/

   /**
    * Name of the data type
    *
    * This may be \c NULL for anonymous structures, for arrays, or for
    * function types.
    */
   const char *name;

   /**
    * For \c GLSL_TYPE_ARRAY, this is the length of the array.  For
    * \c GLSL_TYPE_STRUCT, it is the number of elements in the structure and
    * the number of values pointed to by \c fields.structure (below).
    */
   unsigned length;

   /**
    * Subtype of composite data types.
    */
   union {
      const struct glsl_type *array;            /**< Type of array elements. */
      const struct glsl_type *parameters;       /**< Parameters to function. */
      struct glsl_struct_field *structure;      /**< List of struct fields. */
   } fields;


   /**
    * \name Pointers to various public type singletons
    */
   /*@{*/
   static const glsl_type *const error_type;
   static const glsl_type *const void_type;
   static const glsl_type *const int_type;
   static const glsl_type *const ivec4_type;
   static const glsl_type *const uint_type;
   static const glsl_type *const uvec2_type;
   static const glsl_type *const uvec3_type;
   static const glsl_type *const uvec4_type;
   static const glsl_type *const float_type;
   static const glsl_type *const vec2_type;
   static const glsl_type *const vec3_type;
   static const glsl_type *const vec4_type;
   static const glsl_type *const bool_type;
   static const glsl_type *const mat2_type;
   static const glsl_type *const mat2x3_type;
   static const glsl_type *const mat2x4_type;
   static const glsl_type *const mat3x2_type;
   static const glsl_type *const mat3_type;
   static const glsl_type *const mat3x4_type;
   static const glsl_type *const mat4x2_type;
   static const glsl_type *const mat4x3_type;
   static const glsl_type *const mat4_type;
   /*@}*/


   /**
    * For numeric and boolean derrived types returns the basic scalar type
    *
    * If the type is a numeric or boolean scalar, vector, or matrix type,
    * this function gets the scalar type of the individual components.  For
    * all other types, including arrays of numeric or boolean types, the
    * error type is returned.
    */
   const glsl_type *get_base_type() const;

   /**
    * Get the basic scalar type which this type aggregates.
    *
    * If the type is a numeric or boolean scalar, vector, or matrix, or an
    * array of any of those, this function gets the scalar type of the
    * individual components.  For structs and arrays of structs, this function
    * returns the struct type.  For samplers and arrays of samplers, this
    * function returns the sampler type.
    */
   const glsl_type *get_scalar_type() const;

   /**
    * Query the type of elements in an array
    *
    * \return
    * Pointer to the type of elements in the array for array types, or \c NULL
    * for non-array types.
    */
   const glsl_type *element_type() const
   {
      return is_array() ? fields.array : NULL;
   }

   /**
    * Get the instance of a built-in scalar, vector, or matrix type
    */
   static const glsl_type *get_instance(unsigned base_type, unsigned rows,
					unsigned columns);

   /**
    * Get the instance of an array type
    */
   static const glsl_type *get_array_instance(const glsl_type *base,
					      unsigned elements);

   /**
    * Get the instance of a record type
    */
   static const glsl_type *get_record_instance(const glsl_struct_field *fields,
					       unsigned num_fields,
					       const char *name);

   /**
    * Query the total number of scalars that make up a scalar, vector or matrix
    */
   unsigned components() const
   {
      return vector_elements * matrix_columns;
   }

   /**
    * Calculate the number of components slots required to hold this type
    *
    * This is used to determine how many uniform or varying locations a type
    * might occupy.
    */
   unsigned component_slots() const;

   /**
    * \brief Can this type be implicitly converted to another?
    *
    * \return True if the types are identical or if this type can be converted
    *         to \c desired according to Section 4.1.10 of the GLSL spec.
    *
    * \verbatim
    * From page 25 (31 of the pdf) of the GLSL 1.50 spec, Section 4.1.10
    * Implicit Conversions:
    *
    *     In some situations, an expression and its type will be implicitly
    *     converted to a different type. The following table shows all allowed
    *     implicit conversions:
    *
    *     Type of expression | Can be implicitly converted to
    *     --------------------------------------------------
    *     int                  float
    *     uint
    *
    *     ivec2                vec2
    *     uvec2
    *
    *     ivec3                vec3
    *     uvec3
    *
    *     ivec4                vec4
    *     uvec4
    *
    *     There are no implicit array or structure conversions. For example,
    *     an array of int cannot be implicitly converted to an array of float.
    *     There are no implicit conversions between signed and unsigned
    *     integers.
    * \endverbatim
    */
   bool can_implicitly_convert_to(const glsl_type *desired) const;

   /**
    * Query whether or not a type is a scalar (non-vector and non-matrix).
    */
   bool is_scalar() const
   {
      return (vector_elements == 1)
	 && (base_type >= GLSL_TYPE_UINT)
	 && (base_type <= GLSL_TYPE_BOOL);
   }

   /**
    * Query whether or not a type is a vector
    */
   bool is_vector() const
   {
      return (vector_elements > 1)
	 && (matrix_columns == 1)
	 && (base_type >= GLSL_TYPE_UINT)
	 && (base_type <= GLSL_TYPE_BOOL);
   }

   /**
    * Query whether or not a type is a matrix
    */
   bool is_matrix() const
   {
      /* GLSL only has float matrices. */
      return (matrix_columns > 1) && (base_type == GLSL_TYPE_FLOAT);
   }

   /**
    * Query whether or not a type is a non-array numeric type
    */
   bool is_numeric() const
   {
      return (base_type >= GLSL_TYPE_UINT) && (base_type <= GLSL_TYPE_FLOAT);
   }

   /**
    * Query whether or not a type is an integral type
    */
   bool is_integer() const
   {
      return (base_type == GLSL_TYPE_UINT) || (base_type == GLSL_TYPE_INT);
   }

   /**
    * Query whether or not a type is a float type
    */
   bool is_float() const
   {
      return base_type == GLSL_TYPE_FLOAT;
   }

   /**
    * Query whether or not a type is a non-array boolean type
    */
   bool is_boolean() const
   {
      return base_type == GLSL_TYPE_BOOL;
   }

   /**
    * Query whether or not a type is a sampler
    */
   bool is_sampler() const
   {
      return base_type == GLSL_TYPE_SAMPLER;
   }

   /**
    * Query whether or not type is a sampler, or for struct and array
    * types, contains a sampler.
    */
   bool contains_sampler() const;

   /**
    * Get the Mesa texture target index for a sampler type.
    */
   gl_texture_index sampler_index() const;

   /**
    * Query whether or not a type is an array
    */
   bool is_array() const
   {
      return base_type == GLSL_TYPE_ARRAY;
   }

   /**
    * Query whether or not a type is a record
    */
   bool is_record() const
   {
      return base_type == GLSL_TYPE_STRUCT;
   }

   /**
    * Query whether or not a type is the void type singleton.
    */
   bool is_void() const
   {
      return base_type == GLSL_TYPE_VOID;
   }

   /**
    * Query whether or not a type is the error type singleton.
    */
   bool is_error() const
   {
      return base_type == GLSL_TYPE_ERROR;
   }

   /**
    * Query the full type of a matrix row
    *
    * \return
    * If the type is not a matrix, \c glsl_type::error_type is returned.
    * Otherwise a type matching the rows of the matrix is returned.
    */
   const glsl_type *row_type() const
   {
      return is_matrix()
	 ? get_instance(base_type, matrix_columns, 1)
	 : error_type;
   }

   /**
    * Query the full type of a matrix column
    *
    * \return
    * If the type is not a matrix, \c glsl_type::error_type is returned.
    * Otherwise a type matching the columns of the matrix is returned.
    */
   const glsl_type *column_type() const
   {
      return is_matrix()
	 ? get_instance(base_type, vector_elements, 1)
	 : error_type;
   }


   /**
    * Get the type of a structure field
    *
    * \return
    * Pointer to the type of the named field.  If the type is not a structure
    * or the named field does not exist, \c glsl_type::error_type is returned.
    */
   const glsl_type *field_type(const char *name) const;


   /**
    * Get the location of a filed within a record type
    */
   int field_index(const char *name) const;


   /**
    * Query the number of elements in an array type
    *
    * \return
    * The number of elements in the array for array types or -1 for non-array
    * types.  If the number of elements in the array has not yet been declared,
    * zero is returned.
    */
   int array_size() const
   {
      return is_array() ? length : -1;
   }

private:
   /**
    * ralloc context for all glsl_type allocations
    *
    * Set on the first call to \c glsl_type::new.
    */
   static void *mem_ctx;

   void init_ralloc_type_ctx(void);

   /** Constructor for vector and matrix types */
   glsl_type(GLenum gl_type,
	     glsl_base_type base_type, unsigned vector_elements,
	     unsigned matrix_columns, const char *name);

   /** Constructor for sampler types */
   glsl_type(GLenum gl_type,
	     enum glsl_sampler_dim dim, bool shadow, bool array,
	     unsigned type, const char *name);

   /** Constructor for record types */
   glsl_type(const glsl_struct_field *fields, unsigned num_fields,
	     const char *name);

   /** Constructor for array types */
   glsl_type(const glsl_type *array, unsigned length);

   /** Hash table containing the known array types. */
   static struct hash_table *array_types;

   /** Hash table containing the known record types. */
   static struct hash_table *record_types;

   static int record_key_compare(const void *a, const void *b);
   static unsigned record_key_hash(const void *key);

   /**
    * \name Pointers to various type singletons
    */
   /*@{*/
   static const glsl_type _error_type;
   static const glsl_type _void_type;
   static const glsl_type _sampler3D_type;
   static const glsl_type builtin_core_types[];
   static const glsl_type builtin_structure_types[];
   static const glsl_type builtin_110_deprecated_structure_types[];
   static const glsl_type builtin_110_types[];
   static const glsl_type builtin_120_types[];
   static const glsl_type builtin_130_types[];
   static const glsl_type builtin_ARB_texture_rectangle_types[];
   static const glsl_type builtin_EXT_texture_array_types[];
   static const glsl_type builtin_EXT_texture_buffer_object_types[];
   static const glsl_type builtin_OES_EGL_image_external_types[];
   /*@}*/

   /**
    * \name Methods to populate a symbol table with built-in types.
    *
    * \internal
    * This is one of the truely annoying things about C++.  Methods that are
    * completely internal and private to a type still have to be advertised to
    * the world in a public header file.
    */
   /*@{*/
   static void generate_100ES_types(glsl_symbol_table *);
   static void generate_110_types(glsl_symbol_table *);
   static void generate_120_types(glsl_symbol_table *);
   static void generate_130_types(glsl_symbol_table *);
   static void generate_ARB_texture_rectangle_types(glsl_symbol_table *, bool);
   static void generate_EXT_texture_array_types(glsl_symbol_table *, bool);
   static void generate_OES_texture_3D_types(glsl_symbol_table *, bool);
   static void generate_OES_EGL_image_external_types(glsl_symbol_table *, bool);
   /*@}*/

   /**
    * \name Friend functions.
    *
    * These functions are friends because they must have C linkage and the
    * need to call various private methods or access various private static
    * data.
    */
   /*@{*/
   friend void _mesa_glsl_initialize_types(struct _mesa_glsl_parse_state *);
   friend void _mesa_glsl_release_types(void);
   /*@}*/
};

struct glsl_struct_field {
   const struct glsl_type *type;
   const char *name;
};

#endif /* __cplusplus */

#endif /* GLSL_TYPES_H */
