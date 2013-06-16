/*
 * Copyright © 2011 Intel Corporation
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

#include "main/core.h"
#include "ir.h"
#include "linker.h"
#include "ir_uniform.h"
#include "glsl_symbol_table.h"
#include "program/hash_table.h"

/**
 * \file link_uniforms.cpp
 * Assign locations for GLSL uniforms.
 *
 * \author Ian Romanick <ian.d.romanick@intel.com>
 */

/**
 * Count the backing storage requirements for a type
 */
static unsigned
values_for_type(const glsl_type *type)
{
   if (type->is_sampler()) {
      return 1;
   } else if (type->is_array() && type->fields.array->is_sampler()) {
      return type->array_size();
   } else {
      return type->component_slots();
   }
}

void
uniform_field_visitor::process(ir_variable *var)
{
   const glsl_type *t = var->type;

   /* Only strdup the name if we actually will need to modify it. */
   if (t->is_record() || (t->is_array() && t->fields.array->is_record())) {
      char *name = ralloc_strdup(NULL, var->name);
      recursion(var->type, &name, strlen(name));
      ralloc_free(name);
   } else {
      this->visit_field(t, var->name);
   }
}

void
uniform_field_visitor::recursion(const glsl_type *t, char **name,
				 unsigned name_length)
{
   /* Records need to have each field processed individually.
    *
    * Arrays of records need to have each array element processed
    * individually, then each field of the resulting array elements processed
    * individually.
    */
   if (t->is_record()) {
      for (unsigned i = 0; i < t->length; i++) {
	 const char *field = t->fields.structure[i].name;

	 /* Append '.field' to the current uniform name. */
	 ralloc_asprintf_rewrite_tail(name, name_length, ".%s", field);

	 recursion(t->fields.structure[i].type, name,
		   name_length + 1 + strlen(field));
      }
   } else if (t->is_array() && t->fields.array->is_record()) {
      for (unsigned i = 0; i < t->length; i++) {
	 char subscript[13];

	 /* Append the subscript to the current uniform name */
	 const unsigned subscript_length = snprintf(subscript, 13, "[%u]", i);
	 ralloc_asprintf_rewrite_tail(name, name_length, "%s", subscript);

	 recursion(t->fields.array, name, name_length + subscript_length);
      }
   } else {
      this->visit_field(t, *name);
   }
}

/**
 * Class to help calculate the storage requirements for a set of uniforms
 *
 * As uniforms are added to the active set the number of active uniforms and
 * the storage requirements for those uniforms are accumulated.  The active
 * uniforms are added the the hash table supplied to the constructor.
 *
 * If the same uniform is added multiple times (i.e., once for each shader
 * target), it will only be accounted once.
 */
class count_uniform_size : public uniform_field_visitor {
public:
   count_uniform_size(struct string_to_uint_map *map)
      : num_active_uniforms(0), num_values(0), num_shader_samplers(0),
	num_shader_uniform_components(0), map(map)
   {
      /* empty */
   }

   void start_shader()
   {
      this->num_shader_samplers = 0;
      this->num_shader_uniform_components = 0;
   }

   /**
    * Total number of active uniforms counted
    */
   unsigned num_active_uniforms;

   /**
    * Number of data values required to back the storage for the active uniforms
    */
   unsigned num_values;

   /**
    * Number of samplers used
    */
   unsigned num_shader_samplers;

   /**
    * Number of uniforms used in the current shader
    */
   unsigned num_shader_uniform_components;

private:
   virtual void visit_field(const glsl_type *type, const char *name)
   {
      assert(!type->is_record());
      assert(!(type->is_array() && type->fields.array->is_record()));

      /* Count the number of samplers regardless of whether the uniform is
       * already in the hash table.  The hash table prevents adding the same
       * uniform for multiple shader targets, but in this case we want to
       * count it for each shader target.
       */
      const unsigned values = values_for_type(type);
      if (type->contains_sampler()) {
	 this->num_shader_samplers +=
	    type->is_array() ? type->array_size() : 1;
      } else {
	 /* Accumulate the total number of uniform slots used by this shader.
	  * Note that samplers do not count against this limit because they
	  * don't use any storage on current hardware.
	  */
	 this->num_shader_uniform_components += values;
      }

      /* If the uniform is already in the map, there's nothing more to do.
       */
      unsigned id;
      if (this->map->get(id, name))
	 return;

      this->map->put(this->num_active_uniforms, name);

      /* Each leaf uniform occupies one entry in the list of active
       * uniforms.
       */
      this->num_active_uniforms++;
      this->num_values += values;
   }

   struct string_to_uint_map *map;
};

/**
 * Class to help parcel out pieces of backing storage to uniforms
 *
 * Each uniform processed has some range of the \c gl_constant_value
 * structures associated with it.  The association is done by finding
 * the uniform in the \c string_to_uint_map and using the value from
 * the map to connect that slot in the \c gl_uniform_storage table
 * with the next available slot in the \c gl_constant_value array.
 *
 * \warning
 * This class assumes that every uniform that will be processed is
 * already in the \c string_to_uint_map.  In addition, it assumes that
 * the \c gl_uniform_storage and \c gl_constant_value arrays are "big
 * enough."
 */
class parcel_out_uniform_storage : public uniform_field_visitor {
public:
   parcel_out_uniform_storage(struct string_to_uint_map *map,
			      struct gl_uniform_storage *uniforms,
			      union gl_constant_value *values)
      : map(map), uniforms(uniforms), next_sampler(0), values(values)
   {
      memset(this->targets, 0, sizeof(this->targets));
   }

   void start_shader()
   {
      this->shader_samplers_used = 0;
      this->shader_shadow_samplers = 0;
   }

private:
   virtual void visit_field(const glsl_type *type, const char *name)
   {
      assert(!type->is_record());
      assert(!(type->is_array() && type->fields.array->is_record()));

      unsigned id;
      bool found = this->map->get(id, name);
      assert(found);

      if (!found)
	 return;

      /* If there is already storage associated with this uniform, it means
       * that it was set while processing an earlier shader stage.  For
       * example, we may be processing the uniform in the fragment shader, but
       * the uniform was already processed in the vertex shader.
       */
      if (this->uniforms[id].storage != NULL) {
	 /* If the uniform already has storage set from another shader stage,
	  * mark the samplers used for this shader stage.
	  */
	 if (type->contains_sampler()) {
	    const unsigned count = MAX2(1, this->uniforms[id].array_elements);
	    const unsigned shadow = (type->is_array())
	       ? type->fields.array->sampler_shadow : type->sampler_shadow;

	    for (unsigned i = 0; i < count; i++) {
	       const unsigned s = this->uniforms[id].sampler + i;

	       this->shader_samplers_used |= 1U << s;
	       this->shader_shadow_samplers |= shadow << s;
	    }
	 }

	 return;
      }

      const glsl_type *base_type;
      if (type->is_array()) {
	 this->uniforms[id].array_elements = type->length;
	 base_type = type->fields.array;
      } else {
	 this->uniforms[id].array_elements = 0;
	 base_type = type;
      }

      if (base_type->is_sampler()) {
	 this->uniforms[id].sampler = this->next_sampler;

	 /* Increment the sampler by 1 for non-arrays and by the number of
	  * array elements for arrays.
	  */
	 this->next_sampler += MAX2(1, this->uniforms[id].array_elements);

	 const gl_texture_index target = base_type->sampler_index();
	 const unsigned shadow = base_type->sampler_shadow;
	 for (unsigned i = this->uniforms[id].sampler
		 ; i < this->next_sampler
		 ; i++) {
	    this->targets[i] = target;
	    this->shader_samplers_used |= 1U << i;
	    this->shader_shadow_samplers |= shadow << i;
	 }

      } else {
	 this->uniforms[id].sampler = ~0;
      }

      this->uniforms[id].name = ralloc_strdup(this->uniforms, name);
      this->uniforms[id].type = base_type;
      this->uniforms[id].initialized = 0;
      this->uniforms[id].num_driver_storage = 0;
      this->uniforms[id].driver_storage = NULL;
      this->uniforms[id].storage = this->values;

      this->values += values_for_type(type);
   }

   struct string_to_uint_map *map;

   struct gl_uniform_storage *uniforms;
   unsigned next_sampler;

public:
   union gl_constant_value *values;

   gl_texture_index targets[MAX_SAMPLERS];

   /**
    * Mask of samplers used by the current shader stage.
    */
   unsigned shader_samplers_used;

   /**
    * Mask of samplers used by the current shader stage for shadows.
    */
   unsigned shader_shadow_samplers;
};

void
link_assign_uniform_locations(struct gl_shader_program *prog)
{
   ralloc_free(prog->UniformStorage);
   prog->UniformStorage = NULL;
   prog->NumUserUniformStorage = 0;

   if (prog->UniformHash != NULL) {
      prog->UniformHash->clear();
   } else {
      prog->UniformHash = new string_to_uint_map;
   }

   for (unsigned i = 0; i < Elements(prog->SamplerUnits); i++) {
      prog->SamplerUnits[i] = i;
   }

   /* First pass: Count the uniform resources used by the user-defined
    * uniforms.  While this happens, each active uniform will have an index
    * assigned to it.
    *
    * Note: this is *NOT* the index that is returned to the application by
    * glGetUniformLocation.
    */
   count_uniform_size uniform_size(prog->UniformHash);
   for (unsigned i = 0; i < MESA_SHADER_TYPES; i++) {
      if (prog->_LinkedShaders[i] == NULL)
	 continue;

      /* Reset various per-shader target counts.
       */
      uniform_size.start_shader();

      foreach_list(node, prog->_LinkedShaders[i]->ir) {
	 ir_variable *const var = ((ir_instruction *) node)->as_variable();

	 if ((var == NULL) || (var->mode != ir_var_uniform))
	    continue;

	 /* FINISHME: Update code to process built-in uniforms!
	  */
	 if (strncmp("gl_", var->name, 3) == 0)
	    continue;

	 uniform_size.process(var);
      }

      prog->_LinkedShaders[i]->num_samplers = uniform_size.num_shader_samplers;
      prog->_LinkedShaders[i]->num_uniform_components =
	 uniform_size.num_shader_uniform_components;
   }

   const unsigned num_user_uniforms = uniform_size.num_active_uniforms;
   const unsigned num_data_slots = uniform_size.num_values;

   /* On the outside chance that there were no uniforms, bail out.
    */
   if (num_user_uniforms == 0)
      return;

   struct gl_uniform_storage *uniforms =
      rzalloc_array(prog, struct gl_uniform_storage, num_user_uniforms);
   union gl_constant_value *data =
      rzalloc_array(uniforms, union gl_constant_value, num_data_slots);
#ifndef NDEBUG
   union gl_constant_value *data_end = &data[num_data_slots];
#endif

   parcel_out_uniform_storage parcel(prog->UniformHash, uniforms, data);

   for (unsigned i = 0; i < MESA_SHADER_TYPES; i++) {
      if (prog->_LinkedShaders[i] == NULL)
	 continue;

      /* Reset various per-shader target counts.
       */
      parcel.start_shader();

      foreach_list(node, prog->_LinkedShaders[i]->ir) {
	 ir_variable *const var = ((ir_instruction *) node)->as_variable();

	 if ((var == NULL) || (var->mode != ir_var_uniform))
	    continue;

	 /* FINISHME: Update code to process built-in uniforms!
	  */
	 if (strncmp("gl_", var->name, 3) == 0)
	    continue;

	 parcel.process(var);
      }

      prog->_LinkedShaders[i]->active_samplers = parcel.shader_samplers_used;
      prog->_LinkedShaders[i]->shadow_samplers = parcel.shader_shadow_samplers;
   }

   assert(sizeof(prog->SamplerTargets) == sizeof(parcel.targets));
   memcpy(prog->SamplerTargets, parcel.targets, sizeof(prog->SamplerTargets));

#ifndef NDEBUG
   for (unsigned i = 0; i < num_user_uniforms; i++) {
      assert(uniforms[i].storage != NULL);
   }

   assert(parcel.values == data_end);
#endif

   prog->NumUserUniformStorage = num_user_uniforms;
   prog->UniformStorage = uniforms;

   return;
}
