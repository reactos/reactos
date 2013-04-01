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
 * \file opt_function_inlining.cpp
 *
 * Replaces calls to functions with the body of the function.
 */

#include "ir.h"
#include "ir_visitor.h"
#include "ir_function_inlining.h"
#include "ir_expression_flattening.h"
#include "glsl_types.h"
#include "program/hash_table.h"

static void
do_sampler_replacement(exec_list *instructions,
		       ir_variable *sampler,
		       ir_dereference *deref);

class ir_function_inlining_visitor : public ir_hierarchical_visitor {
public:
   ir_function_inlining_visitor()
   {
      progress = false;
   }

   virtual ~ir_function_inlining_visitor()
   {
      /* empty */
   }

   virtual ir_visitor_status visit_enter(ir_expression *);
   virtual ir_visitor_status visit_enter(ir_call *);
   virtual ir_visitor_status visit_enter(ir_assignment *);
   virtual ir_visitor_status visit_enter(ir_return *);
   virtual ir_visitor_status visit_enter(ir_texture *);
   virtual ir_visitor_status visit_enter(ir_swizzle *);

   bool progress;
};


bool
automatic_inlining_predicate(ir_instruction *ir)
{
   ir_call *call = ir->as_call();

   if (call && can_inline(call))
      return true;

   return false;
}

bool
do_function_inlining(exec_list *instructions)
{
   ir_function_inlining_visitor v;

   do_expression_flattening(instructions, automatic_inlining_predicate);

   v.run(instructions);

   return v.progress;
}

static void
replace_return_with_assignment(ir_instruction *ir, void *data)
{
   void *ctx = ralloc_parent(ir);
   ir_variable *retval = (ir_variable *)data;
   ir_return *ret = ir->as_return();

   if (ret) {
      if (ret->value) {
	 ir_rvalue *lhs = new(ctx) ir_dereference_variable(retval);
	 ret->replace_with(new(ctx) ir_assignment(lhs, ret->value, NULL));
      } else {
	 /* un-valued return has to be the last return, or we shouldn't
	  * have reached here. (see can_inline()).
	  */
	 assert(ret->next->is_tail_sentinel());
	 ret->remove();
      }
   }
}

ir_rvalue *
ir_call::generate_inline(ir_instruction *next_ir)
{
   void *ctx = ralloc_parent(this);
   ir_variable **parameters;
   int num_parameters;
   int i;
   ir_variable *retval = NULL;
   struct hash_table *ht;

   ht = hash_table_ctor(0, hash_table_pointer_hash, hash_table_pointer_compare);

   num_parameters = 0;
   foreach_iter(exec_list_iterator, iter_sig, this->callee->parameters)
      num_parameters++;

   parameters = new ir_variable *[num_parameters];

   /* Generate storage for the return value. */
   if (!this->callee->return_type->is_void()) {
      retval = new(ctx) ir_variable(this->callee->return_type, "_ret_val",
				    ir_var_auto);
      next_ir->insert_before(retval);
   }

   /* Generate the declarations for the parameters to our inlined code,
    * and set up the mapping of real function body variables to ours.
    */
   i = 0;
   exec_list_iterator sig_param_iter = this->callee->parameters.iterator();
   exec_list_iterator param_iter = this->actual_parameters.iterator();
   for (i = 0; i < num_parameters; i++) {
      ir_variable *sig_param = (ir_variable *) sig_param_iter.get();
      ir_rvalue *param = (ir_rvalue *) param_iter.get();

      /* Generate a new variable for the parameter. */
      if (sig_param->type->base_type == GLSL_TYPE_SAMPLER) {
	 /* For samplers, we want the inlined sampler references
	  * referencing the passed in sampler variable, since that
	  * will have the location information, which an assignment of
	  * a sampler wouldn't.  Fix it up below.
	  */
	 parameters[i] = NULL;
      } else {
	 parameters[i] = sig_param->clone(ctx, ht);
	 parameters[i]->mode = ir_var_auto;

	 /* Remove the read-only decoration becuase we're going to write
	  * directly to this variable.  If the cloned variable is left
	  * read-only and the inlined function is inside a loop, the loop
	  * analysis code will get confused.
	  */
	 parameters[i]->read_only = false;
	 next_ir->insert_before(parameters[i]);
      }

      /* Move the actual param into our param variable if it's an 'in' type. */
      if (parameters[i] && (sig_param->mode == ir_var_in ||
			    sig_param->mode == ir_var_const_in ||
			    sig_param->mode == ir_var_inout)) {
	 ir_assignment *assign;

	 assign = new(ctx) ir_assignment(new(ctx) ir_dereference_variable(parameters[i]),
					 param, NULL);
	 next_ir->insert_before(assign);
      }

      sig_param_iter.next();
      param_iter.next();
   }

   exec_list new_instructions;

   /* Generate the inlined body of the function to a new list */
   foreach_iter(exec_list_iterator, iter, callee->body) {
      ir_instruction *ir = (ir_instruction *)iter.get();
      ir_instruction *new_ir = ir->clone(ctx, ht);

      new_instructions.push_tail(new_ir);
      visit_tree(new_ir, replace_return_with_assignment, retval);
   }

   /* If any samplers were passed in, replace any deref of the sampler
    * with a deref of the sampler argument.
    */
   param_iter = this->actual_parameters.iterator();
   sig_param_iter = this->callee->parameters.iterator();
   for (i = 0; i < num_parameters; i++) {
      ir_instruction *const param = (ir_instruction *) param_iter.get();
      ir_variable *sig_param = (ir_variable *) sig_param_iter.get();

      if (sig_param->type->base_type == GLSL_TYPE_SAMPLER) {
	 ir_dereference *deref = param->as_dereference();

	 assert(deref);
	 do_sampler_replacement(&new_instructions, sig_param, deref);
      }
      param_iter.next();
      sig_param_iter.next();
   }

   /* Now push those new instructions in. */
   next_ir->insert_before(&new_instructions);

   /* Copy back the value of any 'out' parameters from the function body
    * variables to our own.
    */
   i = 0;
   param_iter = this->actual_parameters.iterator();
   sig_param_iter = this->callee->parameters.iterator();
   for (i = 0; i < num_parameters; i++) {
      ir_instruction *const param = (ir_instruction *) param_iter.get();
      const ir_variable *const sig_param = (ir_variable *) sig_param_iter.get();

      /* Move our param variable into the actual param if it's an 'out' type. */
      if (parameters[i] && (sig_param->mode == ir_var_out ||
			    sig_param->mode == ir_var_inout)) {
	 ir_assignment *assign;

	 assign = new(ctx) ir_assignment(param->clone(ctx, NULL)->as_rvalue(),
					 new(ctx) ir_dereference_variable(parameters[i]),
					 NULL);
	 next_ir->insert_before(assign);
      }

      param_iter.next();
      sig_param_iter.next();
   }

   delete [] parameters;

   hash_table_dtor(ht);

   if (retval)
      return new(ctx) ir_dereference_variable(retval);
   else
      return NULL;
}


ir_visitor_status
ir_function_inlining_visitor::visit_enter(ir_expression *ir)
{
   (void) ir;
   return visit_continue_with_parent;
}


ir_visitor_status
ir_function_inlining_visitor::visit_enter(ir_return *ir)
{
   (void) ir;
   return visit_continue_with_parent;
}


ir_visitor_status
ir_function_inlining_visitor::visit_enter(ir_texture *ir)
{
   (void) ir;
   return visit_continue_with_parent;
}


ir_visitor_status
ir_function_inlining_visitor::visit_enter(ir_swizzle *ir)
{
   (void) ir;
   return visit_continue_with_parent;
}


ir_visitor_status
ir_function_inlining_visitor::visit_enter(ir_call *ir)
{
   if (can_inline(ir)) {
      /* If the call was part of some tree, then it should have been
       * flattened out or we shouldn't have seen it because of a
       * visit_continue_with_parent in this visitor.
       */
      assert(ir == base_ir);

      (void) ir->generate_inline(ir);
      ir->remove();
      this->progress = true;
   }

   return visit_continue;
}


ir_visitor_status
ir_function_inlining_visitor::visit_enter(ir_assignment *ir)
{
   ir_call *call = ir->rhs->as_call();
   if (!call || !can_inline(call))
      return visit_continue;

   /* generates the parameter setup, function body, and returns the return
    * value of the function
    */
   ir_rvalue *rhs = call->generate_inline(ir);
   assert(rhs);

   ir->rhs = rhs;
   this->progress = true;

   return visit_continue;
}

/**
 * Replaces references to the "sampler" variable with a clone of "deref."
 *
 * From the spec, samplers can appear in the tree as function
 * (non-out) parameters and as the result of array indexing and
 * structure field selection.  In our builtin implementation, they
 * also appear in the sampler field of an ir_tex instruction.
 */

class ir_sampler_replacement_visitor : public ir_hierarchical_visitor {
public:
   ir_sampler_replacement_visitor(ir_variable *sampler, ir_dereference *deref)
   {
      this->sampler = sampler;
      this->deref = deref;
   }

   virtual ~ir_sampler_replacement_visitor()
   {
   }

   virtual ir_visitor_status visit_leave(ir_call *);
   virtual ir_visitor_status visit_leave(ir_dereference_array *);
   virtual ir_visitor_status visit_leave(ir_dereference_record *);
   virtual ir_visitor_status visit_leave(ir_texture *);

   void replace_deref(ir_dereference **deref);
   void replace_rvalue(ir_rvalue **rvalue);

   ir_variable *sampler;
   ir_dereference *deref;
};

void
ir_sampler_replacement_visitor::replace_deref(ir_dereference **deref)
{
   ir_dereference_variable *deref_var = (*deref)->as_dereference_variable();
   if (deref_var && deref_var->var == this->sampler) {
      *deref = this->deref->clone(ralloc_parent(*deref), NULL);
   }
}

void
ir_sampler_replacement_visitor::replace_rvalue(ir_rvalue **rvalue)
{
   if (!*rvalue)
      return;

   ir_dereference *deref = (*rvalue)->as_dereference();

   if (!deref)
      return;

   replace_deref(&deref);
   *rvalue = deref;
}

ir_visitor_status
ir_sampler_replacement_visitor::visit_leave(ir_texture *ir)
{
   replace_deref(&ir->sampler);

   return visit_continue;
}

ir_visitor_status
ir_sampler_replacement_visitor::visit_leave(ir_dereference_array *ir)
{
   replace_rvalue(&ir->array);
   return visit_continue;
}

ir_visitor_status
ir_sampler_replacement_visitor::visit_leave(ir_dereference_record *ir)
{
   replace_rvalue(&ir->record);
   return visit_continue;
}

ir_visitor_status
ir_sampler_replacement_visitor::visit_leave(ir_call *ir)
{
   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_rvalue *param = (ir_rvalue *)iter.get();
      ir_rvalue *new_param = param;
      replace_rvalue(&new_param);

      if (new_param != param) {
	 param->replace_with(new_param);
      }
   }
   return visit_continue;
}

static void
do_sampler_replacement(exec_list *instructions,
		       ir_variable *sampler,
		       ir_dereference *deref)
{
   ir_sampler_replacement_visitor v(sampler, deref);

   visit_list_elements(&v, instructions);
}
