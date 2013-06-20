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

#include "main/core.h"
#include "glsl_symbol_table.h"
#include "glsl_parser_extras.h"
#include "ir.h"
#include "program.h"
#include "program/hash_table.h"
#include "linker.h"

static ir_function_signature *
find_matching_signature(const char *name, const exec_list *actual_parameters,
			gl_shader **shader_list, unsigned num_shaders,
			bool use_builtin);

class call_link_visitor : public ir_hierarchical_visitor {
public:
   call_link_visitor(gl_shader_program *prog, gl_shader *linked,
		     gl_shader **shader_list, unsigned num_shaders)
   {
      this->prog = prog;
      this->shader_list = shader_list;
      this->num_shaders = num_shaders;
      this->success = true;
      this->linked = linked;

      this->locals = hash_table_ctor(0, hash_table_pointer_hash,
				     hash_table_pointer_compare);
   }

   ~call_link_visitor()
   {
      hash_table_dtor(this->locals);
   }

   virtual ir_visitor_status visit(ir_variable *ir)
   {
      hash_table_insert(locals, ir, ir);
      return visit_continue;
   }

   virtual ir_visitor_status visit_enter(ir_call *ir)
   {
      /* If ir is an ir_call from a function that was imported from another
       * shader callee will point to an ir_function_signature in the original
       * shader.  In this case the function signature MUST NOT BE MODIFIED.
       * Doing so will modify the original shader.  This may prevent that
       * shader from being linkable in other programs.
       */
      const ir_function_signature *const callee = ir->get_callee();
      assert(callee != NULL);
      const char *const name = callee->function_name();

      /* Determine if the requested function signature already exists in the
       * final linked shader.  If it does, use it as the target of the call.
       */
      ir_function_signature *sig =
	 find_matching_signature(name, &callee->parameters, &linked, 1,
				 ir->use_builtin);
      if (sig != NULL) {
	 ir->set_callee(sig);
	 return visit_continue;
      }

      /* Try to find the signature in one of the other shaders that is being
       * linked.  If it's not found there, return an error.
       */
      sig = find_matching_signature(name, &ir->actual_parameters, shader_list,
				    num_shaders, ir->use_builtin);
      if (sig == NULL) {
	 /* FINISHME: Log the full signature of unresolved function.
	  */
	 linker_error(this->prog, "unresolved reference to function `%s'\n",
		      name);
	 this->success = false;
	 return visit_stop;
      }

      /* Find the prototype information in the linked shader.  Generate any
       * details that may be missing.
       */
      ir_function *f = linked->symbols->get_function(name);
      if (f == NULL) {
	 f = new(linked) ir_function(name);

	 /* Add the new function to the linked IR.  Put it at the end
          * so that it comes after any global variable declarations
          * that it refers to.
	  */
	 linked->symbols->add_function(f);
	 linked->ir->push_tail(f);
      }

      ir_function_signature *linked_sig =
	 f->exact_matching_signature(&callee->parameters);
      if ((linked_sig == NULL)
	  || ((linked_sig != NULL)
	      && (linked_sig->is_builtin != ir->use_builtin))) {
	 linked_sig = new(linked) ir_function_signature(callee->return_type);
	 f->add_signature(linked_sig);
      }

      /* At this point linked_sig and called may be the same.  If ir is an
       * ir_call from linked then linked_sig and callee will be
       * ir_function_signatures that have no definitions (is_defined is false).
       */
      assert(!linked_sig->is_defined);
      assert(linked_sig->body.is_empty());

      /* Create an in-place clone of the function definition.  This multistep
       * process introduces some complexity here, but it has some advantages.
       * The parameter list and the and function body are cloned separately.
       * The clone of the parameter list is used to prime the hashtable used
       * to replace variable references in the cloned body.
       *
       * The big advantage is that the ir_function_signature does not change.
       * This means that we don't have to process the rest of the IR tree to
       * patch ir_call nodes.  In addition, there is no way to remove or
       * replace signature stored in a function.  One could easily be added,
       * but this avoids the need.
       */
      struct hash_table *ht = hash_table_ctor(0, hash_table_pointer_hash,
					      hash_table_pointer_compare);
      exec_list formal_parameters;
      foreach_list_const(node, &sig->parameters) {
	 const ir_instruction *const original = (ir_instruction *) node;
	 assert(const_cast<ir_instruction *>(original)->as_variable());

	 ir_instruction *copy = original->clone(linked, ht);
	 formal_parameters.push_tail(copy);
      }

      linked_sig->replace_parameters(&formal_parameters);

      foreach_list_const(node, &sig->body) {
	 const ir_instruction *const original = (ir_instruction *) node;

	 ir_instruction *copy = original->clone(linked, ht);
	 linked_sig->body.push_tail(copy);
      }

      linked_sig->is_defined = true;
      hash_table_dtor(ht);

      /* Patch references inside the function to things outside the function
       * (i.e., function calls and global variables).
       */
      linked_sig->accept(this);

      ir->set_callee(linked_sig);

      return visit_continue;
   }

   virtual ir_visitor_status visit(ir_dereference_variable *ir)
   {
      if (hash_table_find(locals, ir->var) == NULL) {
	 /* The non-function variable must be a global, so try to find the
	  * variable in the shader's symbol table.  If the variable is not
	  * found, then it's a global that *MUST* be defined in the original
	  * shader.
	  */
	 ir_variable *var = linked->symbols->get_variable(ir->var->name);
	 if (var == NULL) {
	    /* Clone the ir_variable that the dereference already has and add
	     * it to the linked shader.
	     */
	    var = ir->var->clone(linked, NULL);
	    linked->symbols->add_variable(var);
	    linked->ir->push_head(var);
	 } else if (var->type->is_array()) {
	    /* It is possible to have a global array declared in multiple
	     * shaders without a size.  The array is implicitly sized by the
	     * maximal access to it in *any* shader.  Because of this, we
	     * need to track the maximal access to the array as linking pulls
	     * more functions in that access the array.
	     */
	    var->max_array_access =
	       MAX2(var->max_array_access, ir->var->max_array_access);

	    if (var->type->length == 0 && ir->var->type->length != 0)
	       var->type = ir->var->type;
	 }

	 ir->var = var;
      }

      return visit_continue;
   }

   /** Was function linking successful? */
   bool success;

private:
   /**
    * Shader program being linked
    *
    * This is only used for logging error messages.
    */
   gl_shader_program *prog;

   /** List of shaders available for linking. */
   gl_shader **shader_list;

   /** Number of shaders available for linking. */
   unsigned num_shaders;

   /**
    * Final linked shader
    *
    * This is used two ways.  It is used to find global variables in the
    * linked shader that are accessed by the function.  It is also used to add
    * global variables from the shader where the function originated.
    */
   gl_shader *linked;

   /**
    * Table of variables local to the function.
    */
   hash_table *locals;
};


/**
 * Searches a list of shaders for a particular function definition
 */
ir_function_signature *
find_matching_signature(const char *name, const exec_list *actual_parameters,
			gl_shader **shader_list, unsigned num_shaders,
			bool use_builtin)
{
   for (unsigned i = 0; i < num_shaders; i++) {
      ir_function *const f = shader_list[i]->symbols->get_function(name);

      if (f == NULL)
	 continue;

      ir_function_signature *sig = f->matching_signature(actual_parameters);

      if ((sig == NULL) || !sig->is_defined)
	 continue;

      /* If this function expects to bind to a built-in function and the
       * signature that we found isn't a built-in, keep looking.  Also keep
       * looking if we expect a non-built-in but found a built-in.
       */
      if (use_builtin != sig->is_builtin)
	    continue;

      return sig;
   }

   return NULL;
}


bool
link_function_calls(gl_shader_program *prog, gl_shader *main,
		    gl_shader **shader_list, unsigned num_shaders)
{
   call_link_visitor v(prog, main, shader_list, num_shaders);

   v.run(main->ir);
   return v.success;
}
