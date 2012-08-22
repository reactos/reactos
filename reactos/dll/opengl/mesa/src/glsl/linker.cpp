/*
 * Copyright © 2010 Intel Corporation
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
 * \file linker.cpp
 * GLSL linker implementation
 *
 * Given a set of shaders that are to be linked to generate a final program,
 * there are three distinct stages.
 *
 * In the first stage shaders are partitioned into groups based on the shader
 * type.  All shaders of a particular type (e.g., vertex shaders) are linked
 * together.
 *
 *   - Undefined references in each shader are resolve to definitions in
 *     another shader.
 *   - Types and qualifiers of uniforms, outputs, and global variables defined
 *     in multiple shaders with the same name are verified to be the same.
 *   - Initializers for uniforms and global variables defined
 *     in multiple shaders with the same name are verified to be the same.
 *
 * The result, in the terminology of the GLSL spec, is a set of shader
 * executables for each processing unit.
 *
 * After the first stage is complete, a series of semantic checks are performed
 * on each of the shader executables.
 *
 *   - Each shader executable must define a \c main function.
 *   - Each vertex shader executable must write to \c gl_Position.
 *   - Each fragment shader executable must write to either \c gl_FragData or
 *     \c gl_FragColor.
 *
 * In the final stage individual shader executables are linked to create a
 * complete exectuable.
 *
 *   - Types of uniforms defined in multiple shader stages with the same name
 *     are verified to be the same.
 *   - Initializers for uniforms defined in multiple shader stages with the
 *     same name are verified to be the same.
 *   - Types and qualifiers of outputs defined in one stage are verified to
 *     be the same as the types and qualifiers of inputs defined with the same
 *     name in a later stage.
 *
 * \author Ian Romanick <ian.d.romanick@intel.com>
 */

#include "main/core.h"
#include "glsl_symbol_table.h"
#include "ir.h"
#include "program.h"
#include "program/hash_table.h"
#include "linker.h"
#include "ir_optimization.h"

extern "C" {
#include "main/shaderobj.h"
}

/**
 * Visitor that determines whether or not a variable is ever written.
 */
class find_assignment_visitor : public ir_hierarchical_visitor {
public:
   find_assignment_visitor(const char *name)
      : name(name), found(false)
   {
      /* empty */
   }

   virtual ir_visitor_status visit_enter(ir_assignment *ir)
   {
      ir_variable *const var = ir->lhs->variable_referenced();

      if (strcmp(name, var->name) == 0) {
	 found = true;
	 return visit_stop;
      }

      return visit_continue_with_parent;
   }

   virtual ir_visitor_status visit_enter(ir_call *ir)
   {
      exec_list_iterator sig_iter = ir->get_callee()->parameters.iterator();
      foreach_iter(exec_list_iterator, iter, *ir) {
	 ir_rvalue *param_rval = (ir_rvalue *)iter.get();
	 ir_variable *sig_param = (ir_variable *)sig_iter.get();

	 if (sig_param->mode == ir_var_out ||
	     sig_param->mode == ir_var_inout) {
	    ir_variable *var = param_rval->variable_referenced();
	    if (var && strcmp(name, var->name) == 0) {
	       found = true;
	       return visit_stop;
	    }
	 }
	 sig_iter.next();
      }

      return visit_continue_with_parent;
   }

   bool variable_found()
   {
      return found;
   }

private:
   const char *name;       /**< Find writes to a variable with this name. */
   bool found;             /**< Was a write to the variable found? */
};


/**
 * Visitor that determines whether or not a variable is ever read.
 */
class find_deref_visitor : public ir_hierarchical_visitor {
public:
   find_deref_visitor(const char *name)
      : name(name), found(false)
   {
      /* empty */
   }

   virtual ir_visitor_status visit(ir_dereference_variable *ir)
   {
      if (strcmp(this->name, ir->var->name) == 0) {
	 this->found = true;
	 return visit_stop;
      }

      return visit_continue;
   }

   bool variable_found() const
   {
      return this->found;
   }

private:
   const char *name;       /**< Find writes to a variable with this name. */
   bool found;             /**< Was a write to the variable found? */
};


void
linker_error(gl_shader_program *prog, const char *fmt, ...)
{
   va_list ap;

   ralloc_strcat(&prog->InfoLog, "error: ");
   va_start(ap, fmt);
   ralloc_vasprintf_append(&prog->InfoLog, fmt, ap);
   va_end(ap);

   prog->LinkStatus = false;
}


void
linker_warning(gl_shader_program *prog, const char *fmt, ...)
{
   va_list ap;

   ralloc_strcat(&prog->InfoLog, "error: ");
   va_start(ap, fmt);
   ralloc_vasprintf_append(&prog->InfoLog, fmt, ap);
   va_end(ap);

}


void
link_invalidate_variable_locations(gl_shader *sh, enum ir_variable_mode mode,
				   int generic_base)
{
   foreach_list(node, sh->ir) {
      ir_variable *const var = ((ir_instruction *) node)->as_variable();

      if ((var == NULL) || (var->mode != (unsigned) mode))
	 continue;

      /* Only assign locations for generic attributes / varyings / etc.
       */
      if ((var->location >= generic_base) && !var->explicit_location)
	  var->location = -1;
   }
}


/**
 * Determine the number of attribute slots required for a particular type
 *
 * This code is here because it implements the language rules of a specific
 * GLSL version.  Since it's a property of the language and not a property of
 * types in general, it doesn't really belong in glsl_type.
 */
unsigned
count_attribute_slots(const glsl_type *t)
{
   /* From page 31 (page 37 of the PDF) of the GLSL 1.50 spec:
    *
    *     "A scalar input counts the same amount against this limit as a vec4,
    *     so applications may want to consider packing groups of four
    *     unrelated float inputs together into a vector to better utilize the
    *     capabilities of the underlying hardware. A matrix input will use up
    *     multiple locations.  The number of locations used will equal the
    *     number of columns in the matrix."
    *
    * The spec does not explicitly say how arrays are counted.  However, it
    * should be safe to assume the total number of slots consumed by an array
    * is the number of entries in the array multiplied by the number of slots
    * consumed by a single element of the array.
    */

   if (t->is_array())
      return t->array_size() * count_attribute_slots(t->element_type());

   if (t->is_matrix())
      return t->matrix_columns;

   return 1;
}


/**
 * Verify that a vertex shader executable meets all semantic requirements.
 *
 * Also sets prog->Vert.UsesClipDistance and prog->Vert.ClipDistanceArraySize
 * as a side effect.
 *
 * \param shader  Vertex shader executable to be verified
 */
bool
validate_vertex_shader_executable(struct gl_shader_program *prog,
				  struct gl_shader *shader)
{
   if (shader == NULL)
      return true;

   find_assignment_visitor find("gl_Position");
   find.run(shader->ir);
   if (!find.variable_found()) {
      linker_error(prog, "vertex shader does not write to `gl_Position'\n");
      return false;
   }

   prog->Vert.ClipDistanceArraySize = 0;

   if (prog->Version >= 130) {
      /* From section 7.1 (Vertex Shader Special Variables) of the
       * GLSL 1.30 spec:
       *
       *   "It is an error for a shader to statically write both
       *   gl_ClipVertex and gl_ClipDistance."
       */
      find_assignment_visitor clip_vertex("gl_ClipVertex");
      find_assignment_visitor clip_distance("gl_ClipDistance");

      clip_vertex.run(shader->ir);
      clip_distance.run(shader->ir);
      if (clip_vertex.variable_found() && clip_distance.variable_found()) {
         linker_error(prog, "vertex shader writes to both `gl_ClipVertex' "
                      "and `gl_ClipDistance'\n");
         return false;
      }
      prog->Vert.UsesClipDistance = clip_distance.variable_found();
      ir_variable *clip_distance_var =
         shader->symbols->get_variable("gl_ClipDistance");
      if (clip_distance_var)
         prog->Vert.ClipDistanceArraySize = clip_distance_var->type->length;
   }

   return true;
}


/**
 * Verify that a fragment shader executable meets all semantic requirements
 *
 * \param shader  Fragment shader executable to be verified
 */
bool
validate_fragment_shader_executable(struct gl_shader_program *prog,
				    struct gl_shader *shader)
{
   if (shader == NULL)
      return true;

   find_assignment_visitor frag_color("gl_FragColor");
   find_assignment_visitor frag_data("gl_FragData");

   frag_color.run(shader->ir);
   frag_data.run(shader->ir);

   if (frag_color.variable_found() && frag_data.variable_found()) {
      linker_error(prog,  "fragment shader writes to both "
		   "`gl_FragColor' and `gl_FragData'\n");
      return false;
   }

   return true;
}


/**
 * Generate a string describing the mode of a variable
 */
static const char *
mode_string(const ir_variable *var)
{
   switch (var->mode) {
   case ir_var_auto:
      return (var->read_only) ? "global constant" : "global variable";

   case ir_var_uniform: return "uniform";
   case ir_var_in:      return "shader input";
   case ir_var_out:     return "shader output";
   case ir_var_inout:   return "shader inout";

   case ir_var_const_in:
   case ir_var_temporary:
   default:
      assert(!"Should not get here.");
      return "invalid variable";
   }
}


/**
 * Perform validation of global variables used across multiple shaders
 */
bool
cross_validate_globals(struct gl_shader_program *prog,
		       struct gl_shader **shader_list,
		       unsigned num_shaders,
		       bool uniforms_only)
{
   /* Examine all of the uniforms in all of the shaders and cross validate
    * them.
    */
   glsl_symbol_table variables;
   for (unsigned i = 0; i < num_shaders; i++) {
      if (shader_list[i] == NULL)
	 continue;

      foreach_list(node, shader_list[i]->ir) {
	 ir_variable *const var = ((ir_instruction *) node)->as_variable();

	 if (var == NULL)
	    continue;

	 if (uniforms_only && (var->mode != ir_var_uniform))
	    continue;

	 /* Don't cross validate temporaries that are at global scope.  These
	  * will eventually get pulled into the shaders 'main'.
	  */
	 if (var->mode == ir_var_temporary)
	    continue;

	 /* If a global with this name has already been seen, verify that the
	  * new instance has the same type.  In addition, if the globals have
	  * initializers, the values of the initializers must be the same.
	  */
	 ir_variable *const existing = variables.get_variable(var->name);
	 if (existing != NULL) {
	    if (var->type != existing->type) {
	       /* Consider the types to be "the same" if both types are arrays
		* of the same type and one of the arrays is implicitly sized.
		* In addition, set the type of the linked variable to the
		* explicitly sized array.
		*/
	       if (var->type->is_array()
		   && existing->type->is_array()
		   && (var->type->fields.array == existing->type->fields.array)
		   && ((var->type->length == 0)
		       || (existing->type->length == 0))) {
		  if (var->type->length != 0) {
		     existing->type = var->type;
		  }
	       } else {
		  linker_error(prog, "%s `%s' declared as type "
			       "`%s' and type `%s'\n",
			       mode_string(var),
			       var->name, var->type->name,
			       existing->type->name);
		  return false;
	       }
	    }

	    if (var->explicit_location) {
	       if (existing->explicit_location
		   && (var->location != existing->location)) {
		     linker_error(prog, "explicit locations for %s "
				  "`%s' have differing values\n",
				  mode_string(var), var->name);
		     return false;
	       }

	       existing->location = var->location;
	       existing->explicit_location = true;
	    }

	    /* Validate layout qualifiers for gl_FragDepth.
	     *
	     * From the AMD/ARB_conservative_depth specs:
	     *
	     *    "If gl_FragDepth is redeclared in any fragment shader in a
	     *    program, it must be redeclared in all fragment shaders in
	     *    that program that have static assignments to
	     *    gl_FragDepth. All redeclarations of gl_FragDepth in all
	     *    fragment shaders in a single program must have the same set
	     *    of qualifiers."
	     */
	    if (strcmp(var->name, "gl_FragDepth") == 0) {
	       bool layout_declared = var->depth_layout != ir_depth_layout_none;
	       bool layout_differs =
		  var->depth_layout != existing->depth_layout;

	       if (layout_declared && layout_differs) {
		  linker_error(prog,
			       "All redeclarations of gl_FragDepth in all "
			       "fragment shaders in a single program must have "
			       "the same set of qualifiers.");
	       }

	       if (var->used && layout_differs) {
		  linker_error(prog,
			       "If gl_FragDepth is redeclared with a layout "
			       "qualifier in any fragment shader, it must be "
			       "redeclared with the same layout qualifier in "
			       "all fragment shaders that have assignments to "
			       "gl_FragDepth");
	       }
	    }

	    /* Page 35 (page 41 of the PDF) of the GLSL 4.20 spec says:
	     *
	     *     "If a shared global has multiple initializers, the
	     *     initializers must all be constant expressions, and they
	     *     must all have the same value. Otherwise, a link error will
	     *     result. (A shared global having only one initializer does
	     *     not require that initializer to be a constant expression.)"
	     *
	     * Previous to 4.20 the GLSL spec simply said that initializers
	     * must have the same value.  In this case of non-constant
	     * initializers, this was impossible to determine.  As a result,
	     * no vendor actually implemented that behavior.  The 4.20
	     * behavior matches the implemented behavior of at least one other
	     * vendor, so we'll implement that for all GLSL versions.
	     */
	    if (var->constant_initializer != NULL) {
	       if (existing->constant_initializer != NULL) {
		  if (!var->constant_initializer->has_value(existing->constant_initializer)) {
		     linker_error(prog, "initializers for %s "
				  "`%s' have differing values\n",
				  mode_string(var), var->name);
		     return false;
		  }
	       } else {
		  /* If the first-seen instance of a particular uniform did not
		   * have an initializer but a later instance does, copy the
		   * initializer to the version stored in the symbol table.
		   */
		  /* FINISHME: This is wrong.  The constant_value field should
		   * FINISHME: not be modified!  Imagine a case where a shader
		   * FINISHME: without an initializer is linked in two different
		   * FINISHME: programs with shaders that have differing
		   * FINISHME: initializers.  Linking with the first will
		   * FINISHME: modify the shader, and linking with the second
		   * FINISHME: will fail.
		   */
		  existing->constant_initializer =
		     var->constant_initializer->clone(ralloc_parent(existing),
						      NULL);
	       }
	    }

	    if (var->has_initializer) {
	       if (existing->has_initializer
		   && (var->constant_initializer == NULL
		       || existing->constant_initializer == NULL)) {
		  linker_error(prog,
			       "shared global variable `%s' has multiple "
			       "non-constant initializers.\n",
			       var->name);
		  return false;
	       }

	       /* Some instance had an initializer, so keep track of that.  In
		* this location, all sorts of initializers (constant or
		* otherwise) will propagate the existence to the variable
		* stored in the symbol table.
		*/
	       existing->has_initializer = true;
	    }

	    if (existing->invariant != var->invariant) {
	       linker_error(prog, "declarations for %s `%s' have "
			    "mismatching invariant qualifiers\n",
			    mode_string(var), var->name);
	       return false;
	    }
            if (existing->centroid != var->centroid) {
               linker_error(prog, "declarations for %s `%s' have "
			    "mismatching centroid qualifiers\n",
			    mode_string(var), var->name);
               return false;
            }
	 } else
	    variables.add_variable(var);
      }
   }

   return true;
}


/**
 * Perform validation of uniforms used across multiple shader stages
 */
bool
cross_validate_uniforms(struct gl_shader_program *prog)
{
   return cross_validate_globals(prog, prog->_LinkedShaders,
				 MESA_SHADER_TYPES, true);
}


/**
 * Validate that outputs from one stage match inputs of another
 */
bool
cross_validate_outputs_to_inputs(struct gl_shader_program *prog,
				 gl_shader *producer, gl_shader *consumer)
{
   glsl_symbol_table parameters;
   /* FINISHME: Figure these out dynamically. */
   const char *const producer_stage = "vertex";
   const char *const consumer_stage = "fragment";

   /* Find all shader outputs in the "producer" stage.
    */
   foreach_list(node, producer->ir) {
      ir_variable *const var = ((ir_instruction *) node)->as_variable();

      /* FINISHME: For geometry shaders, this should also look for inout
       * FINISHME: variables.
       */
      if ((var == NULL) || (var->mode != ir_var_out))
	 continue;

      parameters.add_variable(var);
   }


   /* Find all shader inputs in the "consumer" stage.  Any variables that have
    * matching outputs already in the symbol table must have the same type and
    * qualifiers.
    */
   foreach_list(node, consumer->ir) {
      ir_variable *const input = ((ir_instruction *) node)->as_variable();

      /* FINISHME: For geometry shaders, this should also look for inout
       * FINISHME: variables.
       */
      if ((input == NULL) || (input->mode != ir_var_in))
	 continue;

      ir_variable *const output = parameters.get_variable(input->name);
      if (output != NULL) {
	 /* Check that the types match between stages.
	  */
	 if (input->type != output->type) {
	    /* There is a bit of a special case for gl_TexCoord.  This
	     * built-in is unsized by default.  Applications that variable
	     * access it must redeclare it with a size.  There is some
	     * language in the GLSL spec that implies the fragment shader
	     * and vertex shader do not have to agree on this size.  Other
	     * driver behave this way, and one or two applications seem to
	     * rely on it.
	     *
	     * Neither declaration needs to be modified here because the array
	     * sizes are fixed later when update_array_sizes is called.
	     *
	     * From page 48 (page 54 of the PDF) of the GLSL 1.10 spec:
	     *
	     *     "Unlike user-defined varying variables, the built-in
	     *     varying variables don't have a strict one-to-one
	     *     correspondence between the vertex language and the
	     *     fragment language."
	     */
	    if (!output->type->is_array()
		|| (strncmp("gl_", output->name, 3) != 0)) {
	       linker_error(prog,
			    "%s shader output `%s' declared as type `%s', "
			    "but %s shader input declared as type `%s'\n",
			    producer_stage, output->name,
			    output->type->name,
			    consumer_stage, input->type->name);
	       return false;
	    }
	 }

	 /* Check that all of the qualifiers match between stages.
	  */
	 if (input->centroid != output->centroid) {
	    linker_error(prog,
			 "%s shader output `%s' %s centroid qualifier, "
			 "but %s shader input %s centroid qualifier\n",
			 producer_stage,
			 output->name,
			 (output->centroid) ? "has" : "lacks",
			 consumer_stage,
			 (input->centroid) ? "has" : "lacks");
	    return false;
	 }

	 if (input->invariant != output->invariant) {
	    linker_error(prog,
			 "%s shader output `%s' %s invariant qualifier, "
			 "but %s shader input %s invariant qualifier\n",
			 producer_stage,
			 output->name,
			 (output->invariant) ? "has" : "lacks",
			 consumer_stage,
			 (input->invariant) ? "has" : "lacks");
	    return false;
	 }

	 if (input->interpolation != output->interpolation) {
	    linker_error(prog,
			 "%s shader output `%s' specifies %s "
			 "interpolation qualifier, "
			 "but %s shader input specifies %s "
			 "interpolation qualifier\n",
			 producer_stage,
			 output->name,
			 output->interpolation_string(),
			 consumer_stage,
			 input->interpolation_string());
	    return false;
	 }
      }
   }

   return true;
}


/**
 * Populates a shaders symbol table with all global declarations
 */
static void
populate_symbol_table(gl_shader *sh)
{
   sh->symbols = new(sh) glsl_symbol_table;

   foreach_list(node, sh->ir) {
      ir_instruction *const inst = (ir_instruction *) node;
      ir_variable *var;
      ir_function *func;

      if ((func = inst->as_function()) != NULL) {
	 sh->symbols->add_function(func);
      } else if ((var = inst->as_variable()) != NULL) {
	 sh->symbols->add_variable(var);
      }
   }
}


/**
 * Remap variables referenced in an instruction tree
 *
 * This is used when instruction trees are cloned from one shader and placed in
 * another.  These trees will contain references to \c ir_variable nodes that
 * do not exist in the target shader.  This function finds these \c ir_variable
 * references and replaces the references with matching variables in the target
 * shader.
 *
 * If there is no matching variable in the target shader, a clone of the
 * \c ir_variable is made and added to the target shader.  The new variable is
 * added to \b both the instruction stream and the symbol table.
 *
 * \param inst         IR tree that is to be processed.
 * \param symbols      Symbol table containing global scope symbols in the
 *                     linked shader.
 * \param instructions Instruction stream where new variable declarations
 *                     should be added.
 */
void
remap_variables(ir_instruction *inst, struct gl_shader *target,
		hash_table *temps)
{
   class remap_visitor : public ir_hierarchical_visitor {
   public:
	 remap_visitor(struct gl_shader *target,
		    hash_table *temps)
      {
	 this->target = target;
	 this->symbols = target->symbols;
	 this->instructions = target->ir;
	 this->temps = temps;
      }

      virtual ir_visitor_status visit(ir_dereference_variable *ir)
      {
	 if (ir->var->mode == ir_var_temporary) {
	    ir_variable *var = (ir_variable *) hash_table_find(temps, ir->var);

	    assert(var != NULL);
	    ir->var = var;
	    return visit_continue;
	 }

	 ir_variable *const existing =
	    this->symbols->get_variable(ir->var->name);
	 if (existing != NULL)
	    ir->var = existing;
	 else {
	    ir_variable *copy = ir->var->clone(this->target, NULL);

	    this->symbols->add_variable(copy);
	    this->instructions->push_head(copy);
	    ir->var = copy;
	 }

	 return visit_continue;
      }

   private:
      struct gl_shader *target;
      glsl_symbol_table *symbols;
      exec_list *instructions;
      hash_table *temps;
   };

   remap_visitor v(target, temps);

   inst->accept(&v);
}


/**
 * Move non-declarations from one instruction stream to another
 *
 * The intended usage pattern of this function is to pass the pointer to the
 * head sentinel of a list (i.e., a pointer to the list cast to an \c exec_node
 * pointer) for \c last and \c false for \c make_copies on the first
 * call.  Successive calls pass the return value of the previous call for
 * \c last and \c true for \c make_copies.
 *
 * \param instructions Source instruction stream
 * \param last         Instruction after which new instructions should be
 *                     inserted in the target instruction stream
 * \param make_copies  Flag selecting whether instructions in \c instructions
 *                     should be copied (via \c ir_instruction::clone) into the
 *                     target list or moved.
 *
 * \return
 * The new "last" instruction in the target instruction stream.  This pointer
 * is suitable for use as the \c last parameter of a later call to this
 * function.
 */
exec_node *
move_non_declarations(exec_list *instructions, exec_node *last,
		      bool make_copies, gl_shader *target)
{
   hash_table *temps = NULL;

   if (make_copies)
      temps = hash_table_ctor(0, hash_table_pointer_hash,
			      hash_table_pointer_compare);

   foreach_list_safe(node, instructions) {
      ir_instruction *inst = (ir_instruction *) node;

      if (inst->as_function())
	 continue;

      ir_variable *var = inst->as_variable();
      if ((var != NULL) && (var->mode != ir_var_temporary))
	 continue;

      assert(inst->as_assignment()
	     || ((var != NULL) && (var->mode == ir_var_temporary)));

      if (make_copies) {
	 inst = inst->clone(target, NULL);

	 if (var != NULL)
	    hash_table_insert(temps, inst, var);
	 else
	    remap_variables(inst, target, temps);
      } else {
	 inst->remove();
      }

      last->insert_after(inst);
      last = inst;
   }

   if (make_copies)
      hash_table_dtor(temps);

   return last;
}

/**
 * Get the function signature for main from a shader
 */
static ir_function_signature *
get_main_function_signature(gl_shader *sh)
{
   ir_function *const f = sh->symbols->get_function("main");
   if (f != NULL) {
      exec_list void_parameters;

      /* Look for the 'void main()' signature and ensure that it's defined.
       * This keeps the linker from accidentally pick a shader that just
       * contains a prototype for main.
       *
       * We don't have to check for multiple definitions of main (in multiple
       * shaders) because that would have already been caught above.
       */
      ir_function_signature *sig = f->matching_signature(&void_parameters);
      if ((sig != NULL) && sig->is_defined) {
	 return sig;
      }
   }

   return NULL;
}


/**
 * Combine a group of shaders for a single stage to generate a linked shader
 *
 * \note
 * If this function is supplied a single shader, it is cloned, and the new
 * shader is returned.
 */
static struct gl_shader *
link_intrastage_shaders(void *mem_ctx,
			struct gl_context *ctx,
			struct gl_shader_program *prog,
			struct gl_shader **shader_list,
			unsigned num_shaders)
{
   /* Check that global variables defined in multiple shaders are consistent.
    */
   if (!cross_validate_globals(prog, shader_list, num_shaders, false))
      return NULL;

   /* Check that there is only a single definition of each function signature
    * across all shaders.
    */
   for (unsigned i = 0; i < (num_shaders - 1); i++) {
      foreach_list(node, shader_list[i]->ir) {
	 ir_function *const f = ((ir_instruction *) node)->as_function();

	 if (f == NULL)
	    continue;

	 for (unsigned j = i + 1; j < num_shaders; j++) {
	    ir_function *const other =
	       shader_list[j]->symbols->get_function(f->name);

	    /* If the other shader has no function (and therefore no function
	     * signatures) with the same name, skip to the next shader.
	     */
	    if (other == NULL)
	       continue;

	    foreach_iter (exec_list_iterator, iter, *f) {
	       ir_function_signature *sig =
		  (ir_function_signature *) iter.get();

	       if (!sig->is_defined || sig->is_builtin)
		  continue;

	       ir_function_signature *other_sig =
		  other->exact_matching_signature(& sig->parameters);

	       if ((other_sig != NULL) && other_sig->is_defined
		   && !other_sig->is_builtin) {
		  linker_error(prog, "function `%s' is multiply defined",
			       f->name);
		  return NULL;
	       }
	    }
	 }
      }
   }

   /* Find the shader that defines main, and make a clone of it.
    *
    * Starting with the clone, search for undefined references.  If one is
    * found, find the shader that defines it.  Clone the reference and add
    * it to the shader.  Repeat until there are no undefined references or
    * until a reference cannot be resolved.
    */
   gl_shader *main = NULL;
   for (unsigned i = 0; i < num_shaders; i++) {
      if (get_main_function_signature(shader_list[i]) != NULL) {
	 main = shader_list[i];
	 break;
      }
   }

   if (main == NULL) {
      linker_error(prog, "%s shader lacks `main'\n",
		   (shader_list[0]->Type == GL_VERTEX_SHADER)
		   ? "vertex" : "fragment");
      return NULL;
   }

   gl_shader *linked = ctx->Driver.NewShader(NULL, 0, main->Type);
   linked->ir = new(linked) exec_list;
   clone_ir_list(mem_ctx, linked->ir, main->ir);

   populate_symbol_table(linked);

   /* The a pointer to the main function in the final linked shader (i.e., the
    * copy of the original shader that contained the main function).
    */
   ir_function_signature *const main_sig = get_main_function_signature(linked);

   /* Move any instructions other than variable declarations or function
    * declarations into main.
    */
   exec_node *insertion_point =
      move_non_declarations(linked->ir, (exec_node *) &main_sig->body, false,
			    linked);

   for (unsigned i = 0; i < num_shaders; i++) {
      if (shader_list[i] == main)
	 continue;

      insertion_point = move_non_declarations(shader_list[i]->ir,
					      insertion_point, true, linked);
   }

   /* Resolve initializers for global variables in the linked shader.
    */
   unsigned num_linking_shaders = num_shaders;
   for (unsigned i = 0; i < num_shaders; i++)
      num_linking_shaders += shader_list[i]->num_builtins_to_link;

   gl_shader **linking_shaders =
      (gl_shader **) calloc(num_linking_shaders, sizeof(gl_shader *));

   memcpy(linking_shaders, shader_list,
	  sizeof(linking_shaders[0]) * num_shaders);

   unsigned idx = num_shaders;
   for (unsigned i = 0; i < num_shaders; i++) {
      memcpy(&linking_shaders[idx], shader_list[i]->builtins_to_link,
	     sizeof(linking_shaders[0]) * shader_list[i]->num_builtins_to_link);
      idx += shader_list[i]->num_builtins_to_link;
   }

   assert(idx == num_linking_shaders);

   if (!link_function_calls(prog, linked, linking_shaders,
			    num_linking_shaders)) {
      ctx->Driver.DeleteShader(ctx, linked);
      linked = NULL;
   }

   free(linking_shaders);

#ifdef DEBUG
   /* At this point linked should contain all of the linked IR, so
    * validate it to make sure nothing went wrong.
    */
   if (linked)
      validate_ir_tree(linked->ir);
#endif

   /* Make a pass over all variable declarations to ensure that arrays with
    * unspecified sizes have a size specified.  The size is inferred from the
    * max_array_access field.
    */
   if (linked != NULL) {
      class array_sizing_visitor : public ir_hierarchical_visitor {
      public:
	 virtual ir_visitor_status visit(ir_variable *var)
	 {
	    if (var->type->is_array() && (var->type->length == 0)) {
	       const glsl_type *type =
		  glsl_type::get_array_instance(var->type->fields.array,
						var->max_array_access + 1);

	       assert(type != NULL);
	       var->type = type;
	    }

	    return visit_continue;
	 }
      } v;

      v.run(linked->ir);
   }

   return linked;
}

/**
 * Update the sizes of linked shader uniform arrays to the maximum
 * array index used.
 *
 * From page 81 (page 95 of the PDF) of the OpenGL 2.1 spec:
 *
 *     If one or more elements of an array are active,
 *     GetActiveUniform will return the name of the array in name,
 *     subject to the restrictions listed above. The type of the array
 *     is returned in type. The size parameter contains the highest
 *     array element index used, plus one. The compiler or linker
 *     determines the highest index used.  There will be only one
 *     active uniform reported by the GL per uniform array.

 */
static void
update_array_sizes(struct gl_shader_program *prog)
{
   for (unsigned i = 0; i < MESA_SHADER_TYPES; i++) {
	 if (prog->_LinkedShaders[i] == NULL)
	    continue;

      foreach_list(node, prog->_LinkedShaders[i]->ir) {
	 ir_variable *const var = ((ir_instruction *) node)->as_variable();

	 if ((var == NULL) || (var->mode != ir_var_uniform &&
			       var->mode != ir_var_in &&
			       var->mode != ir_var_out) ||
	     !var->type->is_array())
	    continue;

	 unsigned int size = var->max_array_access;
	 for (unsigned j = 0; j < MESA_SHADER_TYPES; j++) {
	       if (prog->_LinkedShaders[j] == NULL)
		  continue;

	    foreach_list(node2, prog->_LinkedShaders[j]->ir) {
	       ir_variable *other_var = ((ir_instruction *) node2)->as_variable();
	       if (!other_var)
		  continue;

	       if (strcmp(var->name, other_var->name) == 0 &&
		   other_var->max_array_access > size) {
		  size = other_var->max_array_access;
	       }
	    }
	 }

	 if (size + 1 != var->type->fields.array->length) {
	    /* If this is a built-in uniform (i.e., it's backed by some
	     * fixed-function state), adjust the number of state slots to
	     * match the new array size.  The number of slots per array entry
	     * is not known.  It seems safe to assume that the total number of
	     * slots is an integer multiple of the number of array elements.
	     * Determine the number of slots per array element by dividing by
	     * the old (total) size.
	     */
	    if (var->num_state_slots > 0) {
	       var->num_state_slots = (size + 1)
		  * (var->num_state_slots / var->type->length);
	    }

	    var->type = glsl_type::get_array_instance(var->type->fields.array,
						      size + 1);
	    /* FINISHME: We should update the types of array
	     * dereferences of this variable now.
	     */
	 }
      }
   }
}

/**
 * Find a contiguous set of available bits in a bitmask.
 *
 * \param used_mask     Bits representing used (1) and unused (0) locations
 * \param needed_count  Number of contiguous bits needed.
 *
 * \return
 * Base location of the available bits on success or -1 on failure.
 */
int
find_available_slots(unsigned used_mask, unsigned needed_count)
{
   unsigned needed_mask = (1 << needed_count) - 1;
   const int max_bit_to_test = (8 * sizeof(used_mask)) - needed_count;

   /* The comparison to 32 is redundant, but without it GCC emits "warning:
    * cannot optimize possibly infinite loops" for the loop below.
    */
   if ((needed_count == 0) || (max_bit_to_test < 0) || (max_bit_to_test > 32))
      return -1;

   for (int i = 0; i <= max_bit_to_test; i++) {
      if ((needed_mask & ~used_mask) == needed_mask)
	 return i;

      needed_mask <<= 1;
   }

   return -1;
}


/**
 * Assign locations for either VS inputs for FS outputs
 *
 * \param prog          Shader program whose variables need locations assigned
 * \param target_index  Selector for the program target to receive location
 *                      assignmnets.  Must be either \c MESA_SHADER_VERTEX or
 *                      \c MESA_SHADER_FRAGMENT.
 * \param max_index     Maximum number of generic locations.  This corresponds
 *                      to either the maximum number of draw buffers or the
 *                      maximum number of generic attributes.
 *
 * \return
 * If locations are successfully assigned, true is returned.  Otherwise an
 * error is emitted to the shader link log and false is returned.
 */
bool
assign_attribute_or_color_locations(gl_shader_program *prog,
				    unsigned target_index,
				    unsigned max_index)
{
   /* Mark invalid locations as being used.
    */
   unsigned used_locations = (max_index >= 32)
      ? ~0 : ~((1 << max_index) - 1);

   assert((target_index == MESA_SHADER_VERTEX)
	  || (target_index == MESA_SHADER_FRAGMENT));

   gl_shader *const sh = prog->_LinkedShaders[target_index];
   if (sh == NULL)
      return true;

   /* Operate in a total of four passes.
    *
    * 1. Invalidate the location assignments for all vertex shader inputs.
    *
    * 2. Assign locations for inputs that have user-defined (via
    *    glBindVertexAttribLocation) locations and outputs that have
    *    user-defined locations (via glBindFragDataLocation).
    *
    * 3. Sort the attributes without assigned locations by number of slots
    *    required in decreasing order.  Fragmentation caused by attribute
    *    locations assigned by the application may prevent large attributes
    *    from having enough contiguous space.
    *
    * 4. Assign locations to any inputs without assigned locations.
    */

   const int generic_base = (target_index == MESA_SHADER_VERTEX)
      ? (int) VERT_ATTRIB_GENERIC0 : (int) FRAG_RESULT_DATA0;

   const enum ir_variable_mode direction =
      (target_index == MESA_SHADER_VERTEX) ? ir_var_in : ir_var_out;


   link_invalidate_variable_locations(sh, direction, generic_base);

   /* Temporary storage for the set of attributes that need locations assigned.
    */
   struct temp_attr {
      unsigned slots;
      ir_variable *var;

      /* Used below in the call to qsort. */
      static int compare(const void *a, const void *b)
      {
	 const temp_attr *const l = (const temp_attr *) a;
	 const temp_attr *const r = (const temp_attr *) b;

	 /* Reversed because we want a descending order sort below. */
	 return r->slots - l->slots;
      }
   } to_assign[16];

   unsigned num_attr = 0;

   foreach_list(node, sh->ir) {
      ir_variable *const var = ((ir_instruction *) node)->as_variable();

      if ((var == NULL) || (var->mode != (unsigned) direction))
	 continue;

      if (var->explicit_location) {
	 if ((var->location >= (int)(max_index + generic_base))
	     || (var->location < 0)) {
	    linker_error(prog,
			 "invalid explicit location %d specified for `%s'\n",
			 (var->location < 0)
			 ? var->location : var->location - generic_base,
			 var->name);
	    return false;
	 }
      } else if (target_index == MESA_SHADER_VERTEX) {
	 unsigned binding;

	 if (prog->AttributeBindings->get(binding, var->name)) {
	    assert(binding >= VERT_ATTRIB_GENERIC0);
	    var->location = binding;
	 }
      } else if (target_index == MESA_SHADER_FRAGMENT) {
	 unsigned binding;

	 if (prog->FragDataBindings->get(binding, var->name)) {
	    assert(binding >= FRAG_RESULT_DATA0);
	    var->location = binding;
	 }
      }

      /* If the variable is not a built-in and has a location statically
       * assigned in the shader (presumably via a layout qualifier), make sure
       * that it doesn't collide with other assigned locations.  Otherwise,
       * add it to the list of variables that need linker-assigned locations.
       */
      const unsigned slots = count_attribute_slots(var->type);
      if (var->location != -1) {
	 if (var->location >= generic_base) {
	    /* From page 61 of the OpenGL 4.0 spec:
	     *
	     *     "LinkProgram will fail if the attribute bindings assigned
	     *     by BindAttribLocation do not leave not enough space to
	     *     assign a location for an active matrix attribute or an
	     *     active attribute array, both of which require multiple
	     *     contiguous generic attributes."
	     *
	     * Previous versions of the spec contain similar language but omit
	     * the bit about attribute arrays.
	     *
	     * Page 61 of the OpenGL 4.0 spec also says:
	     *
	     *     "It is possible for an application to bind more than one
	     *     attribute name to the same location. This is referred to as
	     *     aliasing. This will only work if only one of the aliased
	     *     attributes is active in the executable program, or if no
	     *     path through the shader consumes more than one attribute of
	     *     a set of attributes aliased to the same location. A link
	     *     error can occur if the linker determines that every path
	     *     through the shader consumes multiple aliased attributes,
	     *     but implementations are not required to generate an error
	     *     in this case."
	     *
	     * These two paragraphs are either somewhat contradictory, or I
	     * don't fully understand one or both of them.
	     */
	    /* FINISHME: The code as currently written does not support
	     * FINISHME: attribute location aliasing (see comment above).
	     */
	    /* Mask representing the contiguous slots that will be used by
	     * this attribute.
	     */
	    const unsigned attr = var->location - generic_base;
	    const unsigned use_mask = (1 << slots) - 1;

	    /* Generate a link error if the set of bits requested for this
	     * attribute overlaps any previously allocated bits.
	     */
	    if ((~(use_mask << attr) & used_locations) != used_locations) {
	       linker_error(prog,
			    "insufficient contiguous attribute locations "
			    "available for vertex shader input `%s'",
			    var->name);
	       return false;
	    }

	    used_locations |= (use_mask << attr);
	 }

	 continue;
      }

      to_assign[num_attr].slots = slots;
      to_assign[num_attr].var = var;
      num_attr++;
   }

   /* If all of the attributes were assigned locations by the application (or
    * are built-in attributes with fixed locations), return early.  This should
    * be the common case.
    */
   if (num_attr == 0)
      return true;

   qsort(to_assign, num_attr, sizeof(to_assign[0]), temp_attr::compare);

   if (target_index == MESA_SHADER_VERTEX) {
      /* VERT_ATTRIB_GENERIC0 is a pseudo-alias for VERT_ATTRIB_POS.  It can
       * only be explicitly assigned by via glBindAttribLocation.  Mark it as
       * reserved to prevent it from being automatically allocated below.
       */
      find_deref_visitor find("gl_Vertex");
      find.run(sh->ir);
      if (find.variable_found())
	 used_locations |= (1 << 0);
   }

   for (unsigned i = 0; i < num_attr; i++) {
      /* Mask representing the contiguous slots that will be used by this
       * attribute.
       */
      const unsigned use_mask = (1 << to_assign[i].slots) - 1;

      int location = find_available_slots(used_locations, to_assign[i].slots);

      if (location < 0) {
	 const char *const string = (target_index == MESA_SHADER_VERTEX)
	    ? "vertex shader input" : "fragment shader output";

	 linker_error(prog,
		      "insufficient contiguous attribute locations "
		      "available for %s `%s'",
		      string, to_assign[i].var->name);
	 return false;
      }

      to_assign[i].var->location = generic_base + location;
      used_locations |= (use_mask << location);
   }

   return true;
}


/**
 * Demote shader inputs and outputs that are not used in other stages
 */
void
demote_shader_inputs_and_outputs(gl_shader *sh, enum ir_variable_mode mode)
{
   foreach_list(node, sh->ir) {
      ir_variable *const var = ((ir_instruction *) node)->as_variable();

      if ((var == NULL) || (var->mode != int(mode)))
	 continue;

      /* A shader 'in' or 'out' variable is only really an input or output if
       * its value is used by other shader stages.  This will cause the variable
       * to have a location assigned.
       */
      if (var->location == -1) {
	 var->mode = ir_var_auto;
      }
   }
}


/**
 * Data structure tracking information about a transform feedback declaration
 * during linking.
 */
class tfeedback_decl
{
public:
   bool init(struct gl_context *ctx, struct gl_shader_program *prog,
             const void *mem_ctx, const char *input);
   static bool is_same(const tfeedback_decl &x, const tfeedback_decl &y);
   bool assign_location(struct gl_context *ctx, struct gl_shader_program *prog,
                        ir_variable *output_var);
   bool accumulate_num_outputs(struct gl_shader_program *prog, unsigned *count);
   bool store(struct gl_context *ctx, struct gl_shader_program *prog,
              struct gl_transform_feedback_info *info, unsigned buffer,
              unsigned varying, const unsigned max_outputs) const;


   /**
    * True if assign_location() has been called for this object.
    */
   bool is_assigned() const
   {
      return this->location != -1;
   }

   /**
    * Determine whether this object refers to the variable var.
    */
   bool matches_var(ir_variable *var) const
   {
      if (this->is_clip_distance_mesa)
         return strcmp(var->name, "gl_ClipDistanceMESA") == 0;
      else
         return strcmp(var->name, this->var_name) == 0;
   }

   /**
    * The total number of varying components taken up by this variable.  Only
    * valid if is_assigned() is true.
    */
   unsigned num_components() const
   {
      if (this->is_clip_distance_mesa)
         return this->size;
      else
         return this->vector_elements * this->matrix_columns * this->size;
   }

private:
   /**
    * The name that was supplied to glTransformFeedbackVaryings.  Used for
    * error reporting and glGetTransformFeedbackVarying().
    */
   const char *orig_name;

   /**
    * The name of the variable, parsed from orig_name.
    */
   const char *var_name;

   /**
    * True if the declaration in orig_name represents an array.
    */
   bool is_subscripted;

   /**
    * If is_subscripted is true, the subscript that was specified in orig_name.
    */
   unsigned array_subscript;

   /**
    * True if the variable is gl_ClipDistance and the driver lowers
    * gl_ClipDistance to gl_ClipDistanceMESA.
    */
   bool is_clip_distance_mesa;

   /**
    * The vertex shader output location that the linker assigned for this
    * variable.  -1 if a location hasn't been assigned yet.
    */
   int location;

   /**
    * If location != -1, the number of vector elements in this variable, or 1
    * if this variable is a scalar.
    */
   unsigned vector_elements;

   /**
    * If location != -1, the number of matrix columns in this variable, or 1
    * if this variable is not a matrix.
    */
   unsigned matrix_columns;

   /** Type of the varying returned by glGetTransformFeedbackVarying() */
   GLenum type;

   /**
    * If location != -1, the size that should be returned by
    * glGetTransformFeedbackVarying().
    */
   unsigned size;
};


/**
 * Initialize this object based on a string that was passed to
 * glTransformFeedbackVaryings.  If there is a parse error, the error is
 * reported using linker_error(), and false is returned.
 */
bool
tfeedback_decl::init(struct gl_context *ctx, struct gl_shader_program *prog,
                     const void *mem_ctx, const char *input)
{
   /* We don't have to be pedantic about what is a valid GLSL variable name,
    * because any variable with an invalid name can't exist in the IR anyway.
    */

   this->location = -1;
   this->orig_name = input;
   this->is_clip_distance_mesa = false;

   const char *bracket = strrchr(input, '[');

   if (bracket) {
      this->var_name = ralloc_strndup(mem_ctx, input, bracket - input);
      if (sscanf(bracket, "[%u]", &this->array_subscript) != 1) {
         linker_error(prog, "Cannot parse transform feedback varying %s", input);
         return false;
      }
      this->is_subscripted = true;
   } else {
      this->var_name = ralloc_strdup(mem_ctx, input);
      this->is_subscripted = false;
   }

   /* For drivers that lower gl_ClipDistance to gl_ClipDistanceMESA, this
    * class must behave specially to account for the fact that gl_ClipDistance
    * is converted from a float[8] to a vec4[2].
    */
   if (ctx->ShaderCompilerOptions[MESA_SHADER_VERTEX].LowerClipDistance &&
       strcmp(this->var_name, "gl_ClipDistance") == 0) {
      this->is_clip_distance_mesa = true;
   }

   return true;
}


/**
 * Determine whether two tfeedback_decl objects refer to the same variable and
 * array index (if applicable).
 */
bool
tfeedback_decl::is_same(const tfeedback_decl &x, const tfeedback_decl &y)
{
   if (strcmp(x.var_name, y.var_name) != 0)
      return false;
   if (x.is_subscripted != y.is_subscripted)
      return false;
   if (x.is_subscripted && x.array_subscript != y.array_subscript)
      return false;
   return true;
}


/**
 * Assign a location for this tfeedback_decl object based on the location
 * assignment in output_var.
 *
 * If an error occurs, the error is reported through linker_error() and false
 * is returned.
 */
bool
tfeedback_decl::assign_location(struct gl_context *ctx,
                                struct gl_shader_program *prog,
                                ir_variable *output_var)
{
   if (output_var->type->is_array()) {
      /* Array variable */
      const unsigned matrix_cols =
         output_var->type->fields.array->matrix_columns;
      unsigned actual_array_size = this->is_clip_distance_mesa ?
         prog->Vert.ClipDistanceArraySize : output_var->type->array_size();

      if (this->is_subscripted) {
         /* Check array bounds. */
         if (this->array_subscript >= actual_array_size) {
            linker_error(prog, "Transform feedback varying %s has index "
                         "%i, but the array size is %u.",
                         this->orig_name, this->array_subscript,
                         actual_array_size);
            return false;
         }
         if (this->is_clip_distance_mesa) {
            this->location =
               output_var->location + this->array_subscript / 4;
         } else {
            this->location =
               output_var->location + this->array_subscript * matrix_cols;
         }
         this->size = 1;
      } else {
         this->location = output_var->location;
         this->size = actual_array_size;
      }
      this->vector_elements = output_var->type->fields.array->vector_elements;
      this->matrix_columns = matrix_cols;
      if (this->is_clip_distance_mesa)
         this->type = GL_FLOAT;
      else
         this->type = output_var->type->fields.array->gl_type;
   } else {
      /* Regular variable (scalar, vector, or matrix) */
      if (this->is_subscripted) {
         linker_error(prog, "Transform feedback varying %s requested, "
                      "but %s is not an array.",
                      this->orig_name, this->var_name);
         return false;
      }
      this->location = output_var->location;
      this->size = 1;
      this->vector_elements = output_var->type->vector_elements;
      this->matrix_columns = output_var->type->matrix_columns;
      this->type = output_var->type->gl_type;
   }

   /* From GL_EXT_transform_feedback:
    *   A program will fail to link if:
    *
    *   * the total number of components to capture in any varying
    *     variable in <varyings> is greater than the constant
    *     MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS_EXT and the
    *     buffer mode is SEPARATE_ATTRIBS_EXT;
    */
   if (prog->TransformFeedback.BufferMode == GL_SEPARATE_ATTRIBS &&
       this->num_components() >
       ctx->Const.MaxTransformFeedbackSeparateComponents) {
      linker_error(prog, "Transform feedback varying %s exceeds "
                   "MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS.",
                   this->orig_name);
      return false;
   }

   return true;
}


bool
tfeedback_decl::accumulate_num_outputs(struct gl_shader_program *prog,
                                       unsigned *count)
{
   if (!this->is_assigned()) {
      /* From GL_EXT_transform_feedback:
       *   A program will fail to link if:
       *
       *   * any variable name specified in the <varyings> array is not
       *     declared as an output in the geometry shader (if present) or
       *     the vertex shader (if no geometry shader is present);
       */
      linker_error(prog, "Transform feedback varying %s undeclared.",
                   this->orig_name);
      return false;
   }

   unsigned translated_size = this->size;
   if (this->is_clip_distance_mesa)
      translated_size = (translated_size + 3) / 4;

   *count += translated_size * this->matrix_columns;

   return true;
}


/**
 * Update gl_transform_feedback_info to reflect this tfeedback_decl.
 *
 * If an error occurs, the error is reported through linker_error() and false
 * is returned.
 */
bool
tfeedback_decl::store(struct gl_context *ctx, struct gl_shader_program *prog,
                      struct gl_transform_feedback_info *info,
                      unsigned buffer,
                      unsigned varying, const unsigned max_outputs) const
{
   /* From GL_EXT_transform_feedback:
    *   A program will fail to link if:
    *
    *     * the total number of components to capture is greater than
    *       the constant MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS_EXT
    *       and the buffer mode is INTERLEAVED_ATTRIBS_EXT.
    */
   if (prog->TransformFeedback.BufferMode == GL_INTERLEAVED_ATTRIBS &&
       info->BufferStride[buffer] + this->num_components() >
       ctx->Const.MaxTransformFeedbackInterleavedComponents) {
      linker_error(prog, "The MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS "
                   "limit has been exceeded.");
      return false;
   }

   unsigned translated_size = this->size;
   if (this->is_clip_distance_mesa)
      translated_size = (translated_size + 3) / 4;
   unsigned components_so_far = 0;
   for (unsigned index = 0; index < translated_size; ++index) {
      for (unsigned v = 0; v < this->matrix_columns; ++v) {
         unsigned num_components = this->vector_elements;
         assert(info->NumOutputs < max_outputs);
         info->Outputs[info->NumOutputs].ComponentOffset = 0;
         if (this->is_clip_distance_mesa) {
            if (this->is_subscripted) {
               num_components = 1;
               info->Outputs[info->NumOutputs].ComponentOffset =
                  this->array_subscript % 4;
            } else {
               num_components = MIN2(4, this->size - components_so_far);
            }
         }
         info->Outputs[info->NumOutputs].OutputRegister =
            this->location + v + index * this->matrix_columns;
         info->Outputs[info->NumOutputs].NumComponents = num_components;
         info->Outputs[info->NumOutputs].OutputBuffer = buffer;
         info->Outputs[info->NumOutputs].DstOffset = info->BufferStride[buffer];
         ++info->NumOutputs;
         info->BufferStride[buffer] += num_components;
         components_so_far += num_components;
      }
   }
   assert(components_so_far == this->num_components());

   info->Varyings[varying].Name = ralloc_strdup(prog, this->orig_name);
   info->Varyings[varying].Type = this->type;
   info->Varyings[varying].Size = this->size;
   info->NumVarying++;

   return true;
}


/**
 * Parse all the transform feedback declarations that were passed to
 * glTransformFeedbackVaryings() and store them in tfeedback_decl objects.
 *
 * If an error occurs, the error is reported through linker_error() and false
 * is returned.
 */
static bool
parse_tfeedback_decls(struct gl_context *ctx, struct gl_shader_program *prog,
                      const void *mem_ctx, unsigned num_names,
                      char **varying_names, tfeedback_decl *decls)
{
   for (unsigned i = 0; i < num_names; ++i) {
      if (!decls[i].init(ctx, prog, mem_ctx, varying_names[i]))
         return false;
      /* From GL_EXT_transform_feedback:
       *   A program will fail to link if:
       *
       *   * any two entries in the <varyings> array specify the same varying
       *     variable;
       *
       * We interpret this to mean "any two entries in the <varyings> array
       * specify the same varying variable and array index", since transform
       * feedback of arrays would be useless otherwise.
       */
      for (unsigned j = 0; j < i; ++j) {
         if (tfeedback_decl::is_same(decls[i], decls[j])) {
            linker_error(prog, "Transform feedback varying %s specified "
                         "more than once.", varying_names[i]);
            return false;
         }
      }
   }
   return true;
}


/**
 * Assign a location for a variable that is produced in one pipeline stage
 * (the "producer") and consumed in the next stage (the "consumer").
 *
 * \param input_var is the input variable declaration in the consumer.
 *
 * \param output_var is the output variable declaration in the producer.
 *
 * \param input_index is the counter that keeps track of assigned input
 *        locations in the consumer.
 *
 * \param output_index is the counter that keeps track of assigned output
 *        locations in the producer.
 *
 * It is permissible for \c input_var to be NULL (this happens if a variable
 * is output by the producer and consumed by transform feedback, but not
 * consumed by the consumer).
 *
 * If the variable has already been assigned a location, this function has no
 * effect.
 */
void
assign_varying_location(ir_variable *input_var, ir_variable *output_var,
                        unsigned *input_index, unsigned *output_index)
{
   if (output_var->location != -1) {
      /* Location already assigned. */
      return;
   }

   if (input_var) {
      assert(input_var->location == -1);
      input_var->location = *input_index;
   }

   output_var->location = *output_index;

   /* FINISHME: Support for "varying" records in GLSL 1.50. */
   assert(!output_var->type->is_record());

   if (output_var->type->is_array()) {
      const unsigned slots = output_var->type->length
         * output_var->type->fields.array->matrix_columns;

      *output_index += slots;
      *input_index += slots;
   } else {
      const unsigned slots = output_var->type->matrix_columns;

      *output_index += slots;
      *input_index += slots;
   }
}


/**
 * Assign locations for all variables that are produced in one pipeline stage
 * (the "producer") and consumed in the next stage (the "consumer").
 *
 * Variables produced by the producer may also be consumed by transform
 * feedback.
 *
 * \param num_tfeedback_decls is the number of declarations indicating
 *        variables that may be consumed by transform feedback.
 *
 * \param tfeedback_decls is a pointer to an array of tfeedback_decl objects
 *        representing the result of parsing the strings passed to
 *        glTransformFeedbackVaryings().  assign_location() will be called for
 *        each of these objects that matches one of the outputs of the
 *        producer.
 *
 * When num_tfeedback_decls is nonzero, it is permissible for the consumer to
 * be NULL.  In this case, varying locations are assigned solely based on the
 * requirements of transform feedback.
 */
bool
assign_varying_locations(struct gl_context *ctx,
			 struct gl_shader_program *prog,
			 gl_shader *producer, gl_shader *consumer,
                         unsigned num_tfeedback_decls,
                         tfeedback_decl *tfeedback_decls)
{
   /* FINISHME: Set dynamically when geometry shader support is added. */
   unsigned output_index = VERT_RESULT_VAR0;
   unsigned input_index = FRAG_ATTRIB_VAR0;

   /* Operate in a total of three passes.
    *
    * 1. Assign locations for any matching inputs and outputs.
    *
    * 2. Mark output variables in the producer that do not have locations as
    *    not being outputs.  This lets the optimizer eliminate them.
    *
    * 3. Mark input variables in the consumer that do not have locations as
    *    not being inputs.  This lets the optimizer eliminate them.
    */

   link_invalidate_variable_locations(producer, ir_var_out, VERT_RESULT_VAR0);
   if (consumer)
      link_invalidate_variable_locations(consumer, ir_var_in, FRAG_ATTRIB_VAR0);

   foreach_list(node, producer->ir) {
      ir_variable *const output_var = ((ir_instruction *) node)->as_variable();

      if ((output_var == NULL) || (output_var->mode != ir_var_out))
	 continue;

      ir_variable *input_var =
	 consumer ? consumer->symbols->get_variable(output_var->name) : NULL;

      if (input_var && input_var->mode != ir_var_in)
         input_var = NULL;

      if (input_var) {
         assign_varying_location(input_var, output_var, &input_index,
                                 &output_index);
      }

      for (unsigned i = 0; i < num_tfeedback_decls; ++i) {
         if (!tfeedback_decls[i].is_assigned() &&
             tfeedback_decls[i].matches_var(output_var)) {
            if (output_var->location == -1) {
               assign_varying_location(input_var, output_var, &input_index,
                                       &output_index);
            }
            if (!tfeedback_decls[i].assign_location(ctx, prog, output_var))
               return false;
         }
      }
   }

   unsigned varying_vectors = 0;

   if (consumer) {
      foreach_list(node, consumer->ir) {
         ir_variable *const var = ((ir_instruction *) node)->as_variable();

         if ((var == NULL) || (var->mode != ir_var_in))
            continue;

         if (var->location == -1) {
            if (prog->Version <= 120) {
               /* On page 25 (page 31 of the PDF) of the GLSL 1.20 spec:
                *
                *     Only those varying variables used (i.e. read) in
                *     the fragment shader executable must be written to
                *     by the vertex shader executable; declaring
                *     superfluous varying variables in a vertex shader is
                *     permissible.
                *
                * We interpret this text as meaning that the VS must
                * write the variable for the FS to read it.  See
                * "glsl1-varying read but not written" in piglit.
                */

               linker_error(prog, "fragment shader varying %s not written "
                            "by vertex shader\n.", var->name);
            }

            /* An 'in' variable is only really a shader input if its
             * value is written by the previous stage.
             */
            var->mode = ir_var_auto;
         } else {
            /* The packing rules are used for vertex shader inputs are also
             * used for fragment shader inputs.
             */
            varying_vectors += count_attribute_slots(var->type);
         }
      }
   }

   if (ctx->API == API_OPENGLES2 || prog->Version == 100) {
      if (varying_vectors > ctx->Const.MaxVarying) {
         if (ctx->Const.GLSLSkipStrictMaxVaryingLimitCheck) {
            linker_warning(prog, "shader uses too many varying vectors "
                           "(%u > %u), but the driver will try to optimize "
                           "them out; this is non-portable out-of-spec "
                           "behavior\n",
                           varying_vectors, ctx->Const.MaxVarying);
         } else {
            linker_error(prog, "shader uses too many varying vectors "
                         "(%u > %u)\n",
                         varying_vectors, ctx->Const.MaxVarying);
            return false;
         }
      }
   } else {
      const unsigned float_components = varying_vectors * 4;
      if (float_components > ctx->Const.MaxVarying * 4) {
         if (ctx->Const.GLSLSkipStrictMaxVaryingLimitCheck) {
            linker_warning(prog, "shader uses too many varying components "
                           "(%u > %u), but the driver will try to optimize "
                           "them out; this is non-portable out-of-spec "
                           "behavior\n",
                           float_components, ctx->Const.MaxVarying * 4);
         } else {
            linker_error(prog, "shader uses too many varying components "
                         "(%u > %u)\n",
                         float_components, ctx->Const.MaxVarying * 4);
            return false;
         }
      }
   }

   return true;
}


/**
 * Store transform feedback location assignments into
 * prog->LinkedTransformFeedback based on the data stored in tfeedback_decls.
 *
 * If an error occurs, the error is reported through linker_error() and false
 * is returned.
 */
static bool
store_tfeedback_info(struct gl_context *ctx, struct gl_shader_program *prog,
                     unsigned num_tfeedback_decls,
                     tfeedback_decl *tfeedback_decls)
{
   bool separate_attribs_mode =
      prog->TransformFeedback.BufferMode == GL_SEPARATE_ATTRIBS;

   ralloc_free(prog->LinkedTransformFeedback.Varyings);
   ralloc_free(prog->LinkedTransformFeedback.Outputs);

   memset(&prog->LinkedTransformFeedback, 0,
          sizeof(prog->LinkedTransformFeedback));

   prog->LinkedTransformFeedback.NumBuffers =
      separate_attribs_mode ? num_tfeedback_decls : 1;

   prog->LinkedTransformFeedback.Varyings =
      rzalloc_array(prog,
		    struct gl_transform_feedback_varying_info,
		    num_tfeedback_decls);

   unsigned num_outputs = 0;
   for (unsigned i = 0; i < num_tfeedback_decls; ++i)
      if (!tfeedback_decls[i].accumulate_num_outputs(prog, &num_outputs))
         return false;

   prog->LinkedTransformFeedback.Outputs =
      rzalloc_array(prog,
                    struct gl_transform_feedback_output,
                    num_outputs);

   for (unsigned i = 0; i < num_tfeedback_decls; ++i) {
      unsigned buffer = separate_attribs_mode ? i : 0;
      if (!tfeedback_decls[i].store(ctx, prog, &prog->LinkedTransformFeedback,
                                    buffer, i, num_outputs))
         return false;
   }
   assert(prog->LinkedTransformFeedback.NumOutputs == num_outputs);

   return true;
}

/**
 * Store the gl_FragDepth layout in the gl_shader_program struct.
 */
static void
store_fragdepth_layout(struct gl_shader_program *prog)
{
   if (prog->_LinkedShaders[MESA_SHADER_FRAGMENT] == NULL) {
      return;
   }

   struct exec_list *ir = prog->_LinkedShaders[MESA_SHADER_FRAGMENT]->ir;

   /* We don't look up the gl_FragDepth symbol directly because if
    * gl_FragDepth is not used in the shader, it's removed from the IR.
    * However, the symbol won't be removed from the symbol table.
    *
    * We're only interested in the cases where the variable is NOT removed
    * from the IR.
    */
   foreach_list(node, ir) {
      ir_variable *const var = ((ir_instruction *) node)->as_variable();

      if (var == NULL || var->mode != ir_var_out) {
         continue;
      }

      if (strcmp(var->name, "gl_FragDepth") == 0) {
         switch (var->depth_layout) {
         case ir_depth_layout_none:
            prog->FragDepthLayout = FRAG_DEPTH_LAYOUT_NONE;
            return;
         case ir_depth_layout_any:
            prog->FragDepthLayout = FRAG_DEPTH_LAYOUT_ANY;
            return;
         case ir_depth_layout_greater:
            prog->FragDepthLayout = FRAG_DEPTH_LAYOUT_GREATER;
            return;
         case ir_depth_layout_less:
            prog->FragDepthLayout = FRAG_DEPTH_LAYOUT_LESS;
            return;
         case ir_depth_layout_unchanged:
            prog->FragDepthLayout = FRAG_DEPTH_LAYOUT_UNCHANGED;
            return;
         default:
            assert(0);
            return;
         }
      }
   }
}

/**
 * Validate the resources used by a program versus the implementation limits
 */
static bool
check_resources(struct gl_context *ctx, struct gl_shader_program *prog)
{
   static const char *const shader_names[MESA_SHADER_TYPES] = {
      "vertex", "fragment", "geometry"
   };

   const unsigned max_samplers[MESA_SHADER_TYPES] = {
      ctx->Const.MaxVertexTextureImageUnits,
      ctx->Const.MaxTextureImageUnits,
      ctx->Const.MaxGeometryTextureImageUnits
   };

   const unsigned max_uniform_components[MESA_SHADER_TYPES] = {
      ctx->Const.VertexProgram.MaxUniformComponents,
      ctx->Const.FragmentProgram.MaxUniformComponents,
      0          /* FINISHME: Geometry shaders. */
   };

   for (unsigned i = 0; i < MESA_SHADER_TYPES; i++) {
      struct gl_shader *sh = prog->_LinkedShaders[i];

      if (sh == NULL)
	 continue;

      if (sh->num_samplers > max_samplers[i]) {
	 linker_error(prog, "Too many %s shader texture samplers",
		      shader_names[i]);
      }

      if (sh->num_uniform_components > max_uniform_components[i]) {
         if (ctx->Const.GLSLSkipStrictMaxUniformLimitCheck) {
            linker_warning(prog, "Too many %s shader uniform components, "
                           "but the driver will try to optimize them out; "
                           "this is non-portable out-of-spec behavior\n",
                           shader_names[i]);
         } else {
            linker_error(prog, "Too many %s shader uniform components",
                         shader_names[i]);
         }
      }
   }

   return prog->LinkStatus;
}

void
link_shaders(struct gl_context *ctx, struct gl_shader_program *prog)
{
   tfeedback_decl *tfeedback_decls = NULL;
   unsigned num_tfeedback_decls = prog->TransformFeedback.NumVarying;

   void *mem_ctx = ralloc_context(NULL); // temporary linker context

   prog->LinkStatus = false;
   prog->Validated = false;
   prog->_Used = false;

   if (prog->InfoLog != NULL)
      ralloc_free(prog->InfoLog);

   prog->InfoLog = ralloc_strdup(NULL, "");

   /* Separate the shaders into groups based on their type.
    */
   struct gl_shader **vert_shader_list;
   unsigned num_vert_shaders = 0;
   struct gl_shader **frag_shader_list;
   unsigned num_frag_shaders = 0;

   vert_shader_list = (struct gl_shader **)
      calloc(2 * prog->NumShaders, sizeof(struct gl_shader *));
   frag_shader_list =  &vert_shader_list[prog->NumShaders];

   unsigned min_version = UINT_MAX;
   unsigned max_version = 0;
   for (unsigned i = 0; i < prog->NumShaders; i++) {
      min_version = MIN2(min_version, prog->Shaders[i]->Version);
      max_version = MAX2(max_version, prog->Shaders[i]->Version);

      switch (prog->Shaders[i]->Type) {
      case GL_VERTEX_SHADER:
	 vert_shader_list[num_vert_shaders] = prog->Shaders[i];
	 num_vert_shaders++;
	 break;
      case GL_FRAGMENT_SHADER:
	 frag_shader_list[num_frag_shaders] = prog->Shaders[i];
	 num_frag_shaders++;
	 break;
      case GL_GEOMETRY_SHADER:
	 /* FINISHME: Support geometry shaders. */
	 assert(prog->Shaders[i]->Type != GL_GEOMETRY_SHADER);
	 break;
      }
   }

   /* Previous to GLSL version 1.30, different compilation units could mix and
    * match shading language versions.  With GLSL 1.30 and later, the versions
    * of all shaders must match.
    */
   assert(min_version >= 100);
   assert(max_version <= 130);
   if ((max_version >= 130 || min_version == 100)
       && min_version != max_version) {
      linker_error(prog, "all shaders must use same shading "
		   "language version\n");
      goto done;
   }

   prog->Version = max_version;

   for (unsigned int i = 0; i < MESA_SHADER_TYPES; i++) {
      if (prog->_LinkedShaders[i] != NULL)
	 ctx->Driver.DeleteShader(ctx, prog->_LinkedShaders[i]);

      prog->_LinkedShaders[i] = NULL;
   }

   /* Link all shaders for a particular stage and validate the result.
    */
   if (num_vert_shaders > 0) {
      gl_shader *const sh =
	 link_intrastage_shaders(mem_ctx, ctx, prog, vert_shader_list,
				 num_vert_shaders);

      if (sh == NULL)
	 goto done;

      if (!validate_vertex_shader_executable(prog, sh))
	 goto done;

      _mesa_reference_shader(ctx, &prog->_LinkedShaders[MESA_SHADER_VERTEX],
			     sh);
   }

   if (num_frag_shaders > 0) {
      gl_shader *const sh =
	 link_intrastage_shaders(mem_ctx, ctx, prog, frag_shader_list,
				 num_frag_shaders);

      if (sh == NULL)
	 goto done;

      if (!validate_fragment_shader_executable(prog, sh))
	 goto done;

      _mesa_reference_shader(ctx, &prog->_LinkedShaders[MESA_SHADER_FRAGMENT],
			     sh);
   }

   /* Here begins the inter-stage linking phase.  Some initial validation is
    * performed, then locations are assigned for uniforms, attributes, and
    * varyings.
    */
   if (cross_validate_uniforms(prog)) {
      unsigned prev;

      for (prev = 0; prev < MESA_SHADER_TYPES; prev++) {
	 if (prog->_LinkedShaders[prev] != NULL)
	    break;
      }

      /* Validate the inputs of each stage with the output of the preceding
       * stage.
       */
      for (unsigned i = prev + 1; i < MESA_SHADER_TYPES; i++) {
	 if (prog->_LinkedShaders[i] == NULL)
	    continue;

	 if (!cross_validate_outputs_to_inputs(prog,
					       prog->_LinkedShaders[prev],
					       prog->_LinkedShaders[i]))
	    goto done;

	 prev = i;
      }

      prog->LinkStatus = true;
   }

   /* Do common optimization before assigning storage for attributes,
    * uniforms, and varyings.  Later optimization could possibly make
    * some of that unused.
    */
   for (unsigned i = 0; i < MESA_SHADER_TYPES; i++) {
      if (prog->_LinkedShaders[i] == NULL)
	 continue;

      detect_recursion_linked(prog, prog->_LinkedShaders[i]->ir);
      if (!prog->LinkStatus)
	 goto done;

      if (ctx->ShaderCompilerOptions[i].LowerClipDistance)
         lower_clip_distance(prog->_LinkedShaders[i]->ir);

      unsigned max_unroll = ctx->ShaderCompilerOptions[i].MaxUnrollIterations;

      while (do_common_optimization(prog->_LinkedShaders[i]->ir, true, false, max_unroll))
	 ;
   }

   /* FINISHME: The value of the max_attribute_index parameter is
    * FINISHME: implementation dependent based on the value of
    * FINISHME: GL_MAX_VERTEX_ATTRIBS.  GL_MAX_VERTEX_ATTRIBS must be
    * FINISHME: at least 16, so hardcode 16 for now.
    */
   if (!assign_attribute_or_color_locations(prog, MESA_SHADER_VERTEX, 16)) {
      goto done;
   }

   if (!assign_attribute_or_color_locations(prog, MESA_SHADER_FRAGMENT, ctx->Const.MaxDrawBuffers)) {
      goto done;
   }

   unsigned prev;
   for (prev = 0; prev < MESA_SHADER_TYPES; prev++) {
      if (prog->_LinkedShaders[prev] != NULL)
	 break;
   }

   if (num_tfeedback_decls != 0) {
      /* From GL_EXT_transform_feedback:
       *   A program will fail to link if:
       *
       *   * the <count> specified by TransformFeedbackVaryingsEXT is
       *     non-zero, but the program object has no vertex or geometry
       *     shader;
       */
      if (prev >= MESA_SHADER_FRAGMENT) {
         linker_error(prog, "Transform feedback varyings specified, but "
                      "no vertex or geometry shader is present.");
         goto done;
      }

      tfeedback_decls = ralloc_array(mem_ctx, tfeedback_decl,
                                     prog->TransformFeedback.NumVarying);
      if (!parse_tfeedback_decls(ctx, prog, mem_ctx, num_tfeedback_decls,
                                 prog->TransformFeedback.VaryingNames,
                                 tfeedback_decls))
         goto done;
   }

   for (unsigned i = prev + 1; i < MESA_SHADER_TYPES; i++) {
      if (prog->_LinkedShaders[i] == NULL)
	 continue;

      if (!assign_varying_locations(
             ctx, prog, prog->_LinkedShaders[prev], prog->_LinkedShaders[i],
             i == MESA_SHADER_FRAGMENT ? num_tfeedback_decls : 0,
             tfeedback_decls))
	 goto done;

      prev = i;
   }

   if (prev != MESA_SHADER_FRAGMENT && num_tfeedback_decls != 0) {
      /* There was no fragment shader, but we still have to assign varying
       * locations for use by transform feedback.
       */
      if (!assign_varying_locations(
             ctx, prog, prog->_LinkedShaders[prev], NULL, num_tfeedback_decls,
             tfeedback_decls))
         goto done;
   }

   if (!store_tfeedback_info(ctx, prog, num_tfeedback_decls, tfeedback_decls))
      goto done;

   if (prog->_LinkedShaders[MESA_SHADER_VERTEX] != NULL) {
      demote_shader_inputs_and_outputs(prog->_LinkedShaders[MESA_SHADER_VERTEX],
				       ir_var_out);

      /* Eliminate code that is now dead due to unused vertex outputs being
       * demoted.
       */
      while (do_dead_code(prog->_LinkedShaders[MESA_SHADER_VERTEX]->ir, false))
	 ;
   }

   if (prog->_LinkedShaders[MESA_SHADER_GEOMETRY] != NULL) {
      gl_shader *const sh = prog->_LinkedShaders[MESA_SHADER_GEOMETRY];

      demote_shader_inputs_and_outputs(sh, ir_var_in);
      demote_shader_inputs_and_outputs(sh, ir_var_inout);
      demote_shader_inputs_and_outputs(sh, ir_var_out);

      /* Eliminate code that is now dead due to unused geometry outputs being
       * demoted.
       */
      while (do_dead_code(prog->_LinkedShaders[MESA_SHADER_GEOMETRY]->ir, false))
	 ;
   }

   if (prog->_LinkedShaders[MESA_SHADER_FRAGMENT] != NULL) {
      gl_shader *const sh = prog->_LinkedShaders[MESA_SHADER_FRAGMENT];

      demote_shader_inputs_and_outputs(sh, ir_var_in);

      /* Eliminate code that is now dead due to unused fragment inputs being
       * demoted.  This shouldn't actually do anything other than remove
       * declarations of the (now unused) global variables.
       */
      while (do_dead_code(prog->_LinkedShaders[MESA_SHADER_FRAGMENT]->ir, false))
	 ;
   }

   update_array_sizes(prog);
   link_assign_uniform_locations(prog);
   store_fragdepth_layout(prog);

   if (!check_resources(ctx, prog))
      goto done;

   /* OpenGL ES requires that a vertex shader and a fragment shader both be
    * present in a linked program.  By checking for use of shading language
    * version 1.00, we also catch the GL_ARB_ES2_compatibility case.
    */
   if (!prog->InternalSeparateShader &&
       (ctx->API == API_OPENGLES2 || prog->Version == 100)) {
      if (prog->_LinkedShaders[MESA_SHADER_VERTEX] == NULL) {
	 linker_error(prog, "program lacks a vertex shader\n");
      } else if (prog->_LinkedShaders[MESA_SHADER_FRAGMENT] == NULL) {
	 linker_error(prog, "program lacks a fragment shader\n");
      }
   }

   /* FINISHME: Assign fragment shader output locations. */

done:
   free(vert_shader_list);

   for (unsigned i = 0; i < MESA_SHADER_TYPES; i++) {
      if (prog->_LinkedShaders[i] == NULL)
	 continue;

      /* Retain any live IR, but trash the rest. */
      reparent_ir(prog->_LinkedShaders[i]->ir, prog->_LinkedShaders[i]->ir);

      /* The symbol table in the linked shaders may contain references to
       * variables that were removed (e.g., unused uniforms).  Since it may
       * contain junk, there is no possible valid use.  Delete it and set the
       * pointer to NULL.
       */
      delete prog->_LinkedShaders[i]->symbols;
      prog->_LinkedShaders[i]->symbols = NULL;
   }

   ralloc_free(mem_ctx);
}
