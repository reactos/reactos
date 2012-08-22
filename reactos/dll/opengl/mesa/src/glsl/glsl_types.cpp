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

#include <stdio.h>
#include <stdlib.h>
#include "main/core.h" /* for Elements */
#include "glsl_symbol_table.h"
#include "glsl_parser_extras.h"
#include "glsl_types.h"
#include "builtin_types.h"
extern "C" {
#include "program/hash_table.h"
}

hash_table *glsl_type::array_types = NULL;
hash_table *glsl_type::record_types = NULL;
void *glsl_type::mem_ctx = NULL;

void
glsl_type::init_ralloc_type_ctx(void)
{
   if (glsl_type::mem_ctx == NULL) {
      glsl_type::mem_ctx = ralloc_autofree_context();
      assert(glsl_type::mem_ctx != NULL);
   }
}

glsl_type::glsl_type(GLenum gl_type,
		     glsl_base_type base_type, unsigned vector_elements,
		     unsigned matrix_columns, const char *name) :
   gl_type(gl_type),
   base_type(base_type),
   sampler_dimensionality(0), sampler_shadow(0), sampler_array(0),
   sampler_type(0),
   vector_elements(vector_elements), matrix_columns(matrix_columns),
   length(0)
{
   init_ralloc_type_ctx();
   this->name = ralloc_strdup(this->mem_ctx, name);
   /* Neither dimension is zero or both dimensions are zero.
    */
   assert((vector_elements == 0) == (matrix_columns == 0));
   memset(& fields, 0, sizeof(fields));
}

glsl_type::glsl_type(GLenum gl_type,
		     enum glsl_sampler_dim dim, bool shadow, bool array,
		     unsigned type, const char *name) :
   gl_type(gl_type),
   base_type(GLSL_TYPE_SAMPLER),
   sampler_dimensionality(dim), sampler_shadow(shadow),
   sampler_array(array), sampler_type(type),
   vector_elements(0), matrix_columns(0),
   length(0)
{
   init_ralloc_type_ctx();
   this->name = ralloc_strdup(this->mem_ctx, name);
   memset(& fields, 0, sizeof(fields));
}

glsl_type::glsl_type(const glsl_struct_field *fields, unsigned num_fields,
		     const char *name) :
   base_type(GLSL_TYPE_STRUCT),
   sampler_dimensionality(0), sampler_shadow(0), sampler_array(0),
   sampler_type(0),
   vector_elements(0), matrix_columns(0),
   length(num_fields)
{
   unsigned int i;

   init_ralloc_type_ctx();
   this->name = ralloc_strdup(this->mem_ctx, name);
   this->fields.structure = ralloc_array(this->mem_ctx,
					 glsl_struct_field, length);
   for (i = 0; i < length; i++) {
      this->fields.structure[i].type = fields[i].type;
      this->fields.structure[i].name = ralloc_strdup(this->fields.structure,
						     fields[i].name);
   }
}

static void
add_types_to_symbol_table(glsl_symbol_table *symtab,
			  const struct glsl_type *types,
			  unsigned num_types, bool warn)
{
   (void) warn;

   for (unsigned i = 0; i < num_types; i++) {
      symtab->add_type(types[i].name, & types[i]);
   }
}

bool
glsl_type::contains_sampler() const
{
   if (this->is_array()) {
      return this->fields.array->contains_sampler();
   } else if (this->is_record()) {
      for (unsigned int i = 0; i < this->length; i++) {
	 if (this->fields.structure[i].type->contains_sampler())
	    return true;
      }
      return false;
   } else {
      return this->is_sampler();
   }
}

gl_texture_index
glsl_type::sampler_index() const
{
   const glsl_type *const t = (this->is_array()) ? this->fields.array : this;

   assert(t->is_sampler());

   switch (t->sampler_dimensionality) {
   case GLSL_SAMPLER_DIM_1D:
      return (t->sampler_array) ? TEXTURE_1D_ARRAY_INDEX : TEXTURE_1D_INDEX;
   case GLSL_SAMPLER_DIM_2D:
      return (t->sampler_array) ? TEXTURE_2D_ARRAY_INDEX : TEXTURE_2D_INDEX;
   case GLSL_SAMPLER_DIM_3D:
      return TEXTURE_3D_INDEX;
   case GLSL_SAMPLER_DIM_CUBE:
      return TEXTURE_CUBE_INDEX;
   case GLSL_SAMPLER_DIM_RECT:
      return TEXTURE_RECT_INDEX;
   case GLSL_SAMPLER_DIM_BUF:
      assert(!"FINISHME: Implement ARB_texture_buffer_object");
      return TEXTURE_BUFFER_INDEX;
   case GLSL_SAMPLER_DIM_EXTERNAL:
      return TEXTURE_EXTERNAL_INDEX;
   default:
      assert(!"Should not get here.");
      return TEXTURE_BUFFER_INDEX;
   }
}

void
glsl_type::generate_100ES_types(glsl_symbol_table *symtab)
{
   add_types_to_symbol_table(symtab, builtin_core_types,
			     Elements(builtin_core_types),
			     false);
   add_types_to_symbol_table(symtab, builtin_structure_types,
			     Elements(builtin_structure_types),
			     false);
   add_types_to_symbol_table(symtab, void_type, 1, false);
}

void
glsl_type::generate_110_types(glsl_symbol_table *symtab)
{
   generate_100ES_types(symtab);

   add_types_to_symbol_table(symtab, builtin_110_types,
			     Elements(builtin_110_types),
			     false);
   add_types_to_symbol_table(symtab, &_sampler3D_type, 1, false);
   add_types_to_symbol_table(symtab, builtin_110_deprecated_structure_types,
			     Elements(builtin_110_deprecated_structure_types),
			     false);
}


void
glsl_type::generate_120_types(glsl_symbol_table *symtab)
{
   generate_110_types(symtab);

   add_types_to_symbol_table(symtab, builtin_120_types,
			     Elements(builtin_120_types), false);
}


void
glsl_type::generate_130_types(glsl_symbol_table *symtab)
{
   generate_120_types(symtab);

   add_types_to_symbol_table(symtab, builtin_130_types,
			     Elements(builtin_130_types), false);
   generate_EXT_texture_array_types(symtab, false);
}


void
glsl_type::generate_ARB_texture_rectangle_types(glsl_symbol_table *symtab,
						bool warn)
{
   add_types_to_symbol_table(symtab, builtin_ARB_texture_rectangle_types,
			     Elements(builtin_ARB_texture_rectangle_types),
			     warn);
}


void
glsl_type::generate_EXT_texture_array_types(glsl_symbol_table *symtab,
					    bool warn)
{
   add_types_to_symbol_table(symtab, builtin_EXT_texture_array_types,
			     Elements(builtin_EXT_texture_array_types),
			     warn);
}


void
glsl_type::generate_OES_texture_3D_types(glsl_symbol_table *symtab, bool warn)
{
   add_types_to_symbol_table(symtab, &_sampler3D_type, 1, warn);
}


void
glsl_type::generate_OES_EGL_image_external_types(glsl_symbol_table *symtab,
						 bool warn)
{
   add_types_to_symbol_table(symtab, builtin_OES_EGL_image_external_types,
			     Elements(builtin_OES_EGL_image_external_types),
			     warn);
}

void
_mesa_glsl_initialize_types(struct _mesa_glsl_parse_state *state)
{
   switch (state->language_version) {
   case 100:
      assert(state->es_shader);
      glsl_type::generate_100ES_types(state->symbols);
      break;
   case 110:
      glsl_type::generate_110_types(state->symbols);
      break;
   case 120:
      glsl_type::generate_120_types(state->symbols);
      break;
   case 130:
      glsl_type::generate_130_types(state->symbols);
      break;
   default:
      /* error */
      break;
   }

   if (state->ARB_texture_rectangle_enable) {
      glsl_type::generate_ARB_texture_rectangle_types(state->symbols,
					   state->ARB_texture_rectangle_warn);
   }
   if (state->OES_texture_3D_enable && state->language_version == 100) {
      glsl_type::generate_OES_texture_3D_types(state->symbols,
					       state->OES_texture_3D_warn);
   }

   if (state->EXT_texture_array_enable && state->language_version < 130) {
      // These are already included in 130; don't create twice.
      glsl_type::generate_EXT_texture_array_types(state->symbols,
				       state->EXT_texture_array_warn);
   }

   /* We cannot check for language_version == 100 here because we need the
    * types to support fixed-function program generation.  But this is fine
    * since the extension is never enabled for OpenGL contexts.
    */
   if (state->OES_EGL_image_external_enable) {
      glsl_type::generate_OES_EGL_image_external_types(state->symbols,
					       state->OES_EGL_image_external_warn);
   }
}


const glsl_type *glsl_type::get_base_type() const
{
   switch (base_type) {
   case GLSL_TYPE_UINT:
      return uint_type;
   case GLSL_TYPE_INT:
      return int_type;
   case GLSL_TYPE_FLOAT:
      return float_type;
   case GLSL_TYPE_BOOL:
      return bool_type;
   default:
      return error_type;
   }
}


const glsl_type *glsl_type::get_scalar_type() const
{
   const glsl_type *type = this;

   /* Handle arrays */
   while (type->base_type == GLSL_TYPE_ARRAY)
      type = type->fields.array;

   /* Handle vectors and matrices */
   switch (type->base_type) {
   case GLSL_TYPE_UINT:
      return uint_type;
   case GLSL_TYPE_INT:
      return int_type;
   case GLSL_TYPE_FLOAT:
      return float_type;
   default:
      /* Handle everything else */
      return type;
   }
}


void
_mesa_glsl_release_types(void)
{
   if (glsl_type::array_types != NULL) {
      hash_table_dtor(glsl_type::array_types);
      glsl_type::array_types = NULL;
   }

   if (glsl_type::record_types != NULL) {
      hash_table_dtor(glsl_type::record_types);
      glsl_type::record_types = NULL;
   }
}


glsl_type::glsl_type(const glsl_type *array, unsigned length) :
   base_type(GLSL_TYPE_ARRAY),
   sampler_dimensionality(0), sampler_shadow(0), sampler_array(0),
   sampler_type(0),
   vector_elements(0), matrix_columns(0),
   name(NULL), length(length)
{
   this->fields.array = array;
   /* Inherit the gl type of the base. The GL type is used for
    * uniform/statevar handling in Mesa and the arrayness of the type
    * is represented by the size rather than the type.
    */
   this->gl_type = array->gl_type;

   /* Allow a maximum of 10 characters for the array size.  This is enough
    * for 32-bits of ~0.  The extra 3 are for the '[', ']', and terminating
    * NUL.
    */
   const unsigned name_length = strlen(array->name) + 10 + 3;
   char *const n = (char *) ralloc_size(this->mem_ctx, name_length);

   if (length == 0)
      snprintf(n, name_length, "%s[]", array->name);
   else
      snprintf(n, name_length, "%s[%u]", array->name, length);

   this->name = n;
}


const glsl_type *
glsl_type::get_instance(unsigned base_type, unsigned rows, unsigned columns)
{
   if (base_type == GLSL_TYPE_VOID)
      return void_type;

   if ((rows < 1) || (rows > 4) || (columns < 1) || (columns > 4))
      return error_type;

   /* Treat GLSL vectors as Nx1 matrices.
    */
   if (columns == 1) {
      switch (base_type) {
      case GLSL_TYPE_UINT:
	 return uint_type + (rows - 1);
      case GLSL_TYPE_INT:
	 return int_type + (rows - 1);
      case GLSL_TYPE_FLOAT:
	 return float_type + (rows - 1);
      case GLSL_TYPE_BOOL:
	 return bool_type + (rows - 1);
      default:
	 return error_type;
      }
   } else {
      if ((base_type != GLSL_TYPE_FLOAT) || (rows == 1))
	 return error_type;

      /* GLSL matrix types are named mat{COLUMNS}x{ROWS}.  Only the following
       * combinations are valid:
       *
       *   1 2 3 4
       * 1
       * 2   x x x
       * 3   x x x
       * 4   x x x
       */
#define IDX(c,r) (((c-1)*3) + (r-1))

      switch (IDX(columns, rows)) {
      case IDX(2,2): return mat2_type;
      case IDX(2,3): return mat2x3_type;
      case IDX(2,4): return mat2x4_type;
      case IDX(3,2): return mat3x2_type;
      case IDX(3,3): return mat3_type;
      case IDX(3,4): return mat3x4_type;
      case IDX(4,2): return mat4x2_type;
      case IDX(4,3): return mat4x3_type;
      case IDX(4,4): return mat4_type;
      default: return error_type;
      }
   }

   assert(!"Should not get here.");
   return error_type;
}


const glsl_type *
glsl_type::get_array_instance(const glsl_type *base, unsigned array_size)
{

   if (array_types == NULL) {
      array_types = hash_table_ctor(64, hash_table_string_hash,
				    hash_table_string_compare);
   }

   /* Generate a name using the base type pointer in the key.  This is
    * done because the name of the base type may not be unique across
    * shaders.  For example, two shaders may have different record types
    * named 'foo'.
    */
   char key[128];
   snprintf(key, sizeof(key), "%p[%u]", (void *) base, array_size);

   const glsl_type *t = (glsl_type *) hash_table_find(array_types, key);
   if (t == NULL) {
      t = new glsl_type(base, array_size);

      hash_table_insert(array_types, (void *) t, ralloc_strdup(mem_ctx, key));
   }

   assert(t->base_type == GLSL_TYPE_ARRAY);
   assert(t->length == array_size);
   assert(t->fields.array == base);

   return t;
}


int
glsl_type::record_key_compare(const void *a, const void *b)
{
   const glsl_type *const key1 = (glsl_type *) a;
   const glsl_type *const key2 = (glsl_type *) b;

   /* Return zero is the types match (there is zero difference) or non-zero
    * otherwise.
    */
   if (strcmp(key1->name, key2->name) != 0)
      return 1;

   if (key1->length != key2->length)
      return 1;

   for (unsigned i = 0; i < key1->length; i++) {
      if (key1->fields.structure[i].type != key2->fields.structure[i].type)
	 return 1;
      if (strcmp(key1->fields.structure[i].name,
		 key2->fields.structure[i].name) != 0)
	 return 1;
   }

   return 0;
}


unsigned
glsl_type::record_key_hash(const void *a)
{
   const glsl_type *const key = (glsl_type *) a;
   char hash_key[128];
   unsigned size = 0;

   size = snprintf(hash_key, sizeof(hash_key), "%08x", key->length);

   for (unsigned i = 0; i < key->length; i++) {
      if (size >= sizeof(hash_key))
	 break;

      size += snprintf(& hash_key[size], sizeof(hash_key) - size,
		       "%p", (void *) key->fields.structure[i].type);
   }

   return hash_table_string_hash(& hash_key);
}


const glsl_type *
glsl_type::get_record_instance(const glsl_struct_field *fields,
			       unsigned num_fields,
			       const char *name)
{
   const glsl_type key(fields, num_fields, name);

   if (record_types == NULL) {
      record_types = hash_table_ctor(64, record_key_hash, record_key_compare);
   }

   const glsl_type *t = (glsl_type *) hash_table_find(record_types, & key);
   if (t == NULL) {
      t = new glsl_type(fields, num_fields, name);

      hash_table_insert(record_types, (void *) t, t);
   }

   assert(t->base_type == GLSL_TYPE_STRUCT);
   assert(t->length == num_fields);
   assert(strcmp(t->name, name) == 0);

   return t;
}


const glsl_type *
glsl_type::field_type(const char *name) const
{
   if (this->base_type != GLSL_TYPE_STRUCT)
      return error_type;

   for (unsigned i = 0; i < this->length; i++) {
      if (strcmp(name, this->fields.structure[i].name) == 0)
	 return this->fields.structure[i].type;
   }

   return error_type;
}


int
glsl_type::field_index(const char *name) const
{
   if (this->base_type != GLSL_TYPE_STRUCT)
      return -1;

   for (unsigned i = 0; i < this->length; i++) {
      if (strcmp(name, this->fields.structure[i].name) == 0)
	 return i;
   }

   return -1;
}


unsigned
glsl_type::component_slots() const
{
   switch (this->base_type) {
   case GLSL_TYPE_UINT:
   case GLSL_TYPE_INT:
   case GLSL_TYPE_FLOAT:
   case GLSL_TYPE_BOOL:
      return this->components();

   case GLSL_TYPE_STRUCT: {
      unsigned size = 0;

      for (unsigned i = 0; i < this->length; i++)
	 size += this->fields.structure[i].type->component_slots();

      return size;
   }

   case GLSL_TYPE_ARRAY:
      return this->length * this->fields.array->component_slots();

   default:
      return 0;
   }
}

bool
glsl_type::can_implicitly_convert_to(const glsl_type *desired) const
{
   if (this == desired)
      return true;

   /* There is no conversion among matrix types. */
   if (this->matrix_columns > 1 || desired->matrix_columns > 1)
      return false;

   /* int and uint can be converted to float. */
   return desired->is_float()
          && this->is_integer()
          && this->vector_elements == desired->vector_elements;
}
