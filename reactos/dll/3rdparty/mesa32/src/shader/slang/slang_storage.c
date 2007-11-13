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

/**
 * \file slang_storage.c
 * slang variable storage
 * \author Michal Krol
 */

#include "imports.h"
#include "slang_storage.h"
#include "slang_mem.h"

/* slang_storage_array */

GLboolean
slang_storage_array_construct(slang_storage_array * arr)
{
   arr->type = SLANG_STORE_AGGREGATE;
   arr->aggregate = NULL;
   arr->length = 0;
   return GL_TRUE;
}

GLvoid
slang_storage_array_destruct(slang_storage_array * arr)
{
   if (arr->aggregate != NULL) {
      slang_storage_aggregate_destruct(arr->aggregate);
      _slang_free(arr->aggregate);
   }
}

/* slang_storage_aggregate */

GLboolean
slang_storage_aggregate_construct(slang_storage_aggregate * agg)
{
   agg->arrays = NULL;
   agg->count = 0;
   return GL_TRUE;
}

GLvoid
slang_storage_aggregate_destruct(slang_storage_aggregate * agg)
{
   GLuint i;

   for (i = 0; i < agg->count; i++)
      slang_storage_array_destruct(agg->arrays + i);
   _slang_free(agg->arrays);
}

static slang_storage_array *
slang_storage_aggregate_push_new(slang_storage_aggregate * agg)
{
   slang_storage_array *arr = NULL;

   agg->arrays = (slang_storage_array *)
      _slang_realloc(agg->arrays,
                     agg->count * sizeof(slang_storage_array),
                     (agg->count + 1) * sizeof(slang_storage_array));
   if (agg->arrays != NULL) {
      arr = agg->arrays + agg->count;
      if (!slang_storage_array_construct(arr))
         return NULL;
      agg->count++;
   }
   return arr;
}

/* _slang_aggregate_variable() */

static GLboolean
aggregate_vector(slang_storage_aggregate * agg, slang_storage_type basic_type,
                 GLuint row_count)
{
   slang_storage_array *arr = slang_storage_aggregate_push_new(agg);
   if (arr == NULL)
      return GL_FALSE;
   arr->type = basic_type;
   arr->length = row_count;
   return GL_TRUE;
}

static GLboolean
aggregate_matrix(slang_storage_aggregate * agg, slang_storage_type basic_type,
                 GLuint columns, GLuint rows)
{
   slang_storage_array *arr = slang_storage_aggregate_push_new(agg);
   if (arr == NULL)
      return GL_FALSE;
   arr->type = SLANG_STORE_AGGREGATE;
   arr->length = columns;
   arr->aggregate = (slang_storage_aggregate *)
      _slang_alloc(sizeof(slang_storage_aggregate));
   if (arr->aggregate == NULL)
      return GL_FALSE;
   if (!slang_storage_aggregate_construct(arr->aggregate)) {
      _slang_free(arr->aggregate);
      arr->aggregate = NULL;
      return GL_FALSE;
   }
   if (!aggregate_vector(arr->aggregate, basic_type, rows))
      return GL_FALSE;
   return GL_TRUE;
}


static GLboolean
aggregate_variables(slang_storage_aggregate * agg,
                    slang_variable_scope * vars, slang_function_scope * funcs,
                    slang_struct_scope * structs,
                    slang_variable_scope * globals,
                    slang_atom_pool * atoms)
{
   GLuint i;

   for (i = 0; i < vars->num_variables; i++)
      if (!_slang_aggregate_variable(agg, &vars->variables[i]->type.specifier,
                                     vars->variables[i]->array_len, funcs,
                                     structs, globals, atoms))
         return GL_FALSE;
   return GL_TRUE;
}


GLboolean
_slang_aggregate_variable(slang_storage_aggregate * agg,
                          slang_type_specifier * spec, GLuint array_len,
                          slang_function_scope * funcs,
                          slang_struct_scope * structs,
                          slang_variable_scope * vars,
                          slang_atom_pool * atoms)
{
   switch (spec->type) {
   case SLANG_SPEC_BOOL:
      return aggregate_vector(agg, SLANG_STORE_BOOL, 1);
   case SLANG_SPEC_BVEC2:
      return aggregate_vector(agg, SLANG_STORE_BOOL, 2);
   case SLANG_SPEC_BVEC3:
      return aggregate_vector(agg, SLANG_STORE_BOOL, 3);
   case SLANG_SPEC_BVEC4:
      return aggregate_vector(agg, SLANG_STORE_BOOL, 4);
   case SLANG_SPEC_INT:
      return aggregate_vector(agg, SLANG_STORE_INT, 1);
   case SLANG_SPEC_IVEC2:
      return aggregate_vector(agg, SLANG_STORE_INT, 2);
   case SLANG_SPEC_IVEC3:
      return aggregate_vector(agg, SLANG_STORE_INT, 3);
   case SLANG_SPEC_IVEC4:
      return aggregate_vector(agg, SLANG_STORE_INT, 4);
   case SLANG_SPEC_FLOAT:
      return aggregate_vector(agg, SLANG_STORE_FLOAT, 1);
   case SLANG_SPEC_VEC2:
      return aggregate_vector(agg, SLANG_STORE_FLOAT, 2);
   case SLANG_SPEC_VEC3:
      return aggregate_vector(agg, SLANG_STORE_FLOAT, 3);
   case SLANG_SPEC_VEC4:
      return aggregate_vector(agg, SLANG_STORE_FLOAT, 4);
   case SLANG_SPEC_MAT2:
      return aggregate_matrix(agg, SLANG_STORE_FLOAT, 2, 2);
   case SLANG_SPEC_MAT3:
      return aggregate_matrix(agg, SLANG_STORE_FLOAT, 3, 3);
   case SLANG_SPEC_MAT4:
      return aggregate_matrix(agg, SLANG_STORE_FLOAT, 4, 4);

   case SLANG_SPEC_MAT23:
      return aggregate_matrix(agg, SLANG_STORE_FLOAT, 2, 3);
   case SLANG_SPEC_MAT32:
      return aggregate_matrix(agg, SLANG_STORE_FLOAT, 3, 2);
   case SLANG_SPEC_MAT24:
      return aggregate_matrix(agg, SLANG_STORE_FLOAT, 2, 4);
   case SLANG_SPEC_MAT42:
      return aggregate_matrix(agg, SLANG_STORE_FLOAT, 4, 2);
   case SLANG_SPEC_MAT34:
      return aggregate_matrix(agg, SLANG_STORE_FLOAT, 3, 4);
   case SLANG_SPEC_MAT43:
      return aggregate_matrix(agg, SLANG_STORE_FLOAT, 4, 3);

   case SLANG_SPEC_SAMPLER1D:
   case SLANG_SPEC_SAMPLER2D:
   case SLANG_SPEC_SAMPLER3D:
   case SLANG_SPEC_SAMPLERCUBE:
   case SLANG_SPEC_SAMPLER1DSHADOW:
   case SLANG_SPEC_SAMPLER2DSHADOW:
   case SLANG_SPEC_SAMPLER2DRECT:
   case SLANG_SPEC_SAMPLER2DRECTSHADOW:
      return aggregate_vector(agg, SLANG_STORE_INT, 1);
   case SLANG_SPEC_STRUCT:
      return aggregate_variables(agg, spec->_struct->fields, funcs, structs,
                                 vars, atoms);
   case SLANG_SPEC_ARRAY:
      {
         slang_storage_array *arr;

         arr = slang_storage_aggregate_push_new(agg);
         if (arr == NULL)
            return GL_FALSE;
         arr->type = SLANG_STORE_AGGREGATE;
         arr->aggregate = (slang_storage_aggregate *)
            _slang_alloc(sizeof(slang_storage_aggregate));
         if (arr->aggregate == NULL)
            return GL_FALSE;
         if (!slang_storage_aggregate_construct(arr->aggregate)) {
            _slang_free(arr->aggregate);
            arr->aggregate = NULL;
            return GL_FALSE;
         }
         if (!_slang_aggregate_variable(arr->aggregate, spec->_array, 0,
                                        funcs, structs, vars, atoms))
            return GL_FALSE;
         arr->length = array_len;
         /* TODO: check if 0 < arr->length <= 65535 */
      }
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


GLuint
_slang_sizeof_type(slang_storage_type type)
{
   if (type == SLANG_STORE_AGGREGATE)
      return 0;
   if (type == SLANG_STORE_VEC4)
      return 4 * sizeof(GLfloat);
   return sizeof(GLfloat);
}


GLuint
_slang_sizeof_aggregate(const slang_storage_aggregate * agg)
{
   GLuint i, size = 0;

   for (i = 0; i < agg->count; i++) {
      slang_storage_array *arr = &agg->arrays[i];
      GLuint element_size;

      if (arr->type == SLANG_STORE_AGGREGATE)
         element_size = _slang_sizeof_aggregate(arr->aggregate);
      else
         element_size = _slang_sizeof_type(arr->type);
      size += element_size * arr->length;
   }
   return size;
}


#if 0
GLboolean
_slang_flatten_aggregate(slang_storage_aggregate * flat,
                         const slang_storage_aggregate * agg)
{
   GLuint i;

   for (i = 0; i < agg->count; i++) {
      GLuint j;

      for (j = 0; j < agg->arrays[i].length; j++) {
         if (agg->arrays[i].type == SLANG_STORE_AGGREGATE) {
            if (!_slang_flatten_aggregate(flat, agg->arrays[i].aggregate))
               return GL_FALSE;
         }
         else {
            GLuint k, count;
            slang_storage_type type;

            if (agg->arrays[i].type == SLANG_STORE_VEC4) {
               count = 4;
               type = SLANG_STORE_FLOAT;
            }
            else {
               count = 1;
               type = agg->arrays[i].type;
            }

            for (k = 0; k < count; k++) {
               slang_storage_array *arr;

               arr = slang_storage_aggregate_push_new(flat);
               if (arr == NULL)
                  return GL_FALSE;
               arr->type = type;
               arr->length = 1;
            }
         }
      }
   }
   return GL_TRUE;
}
#endif
