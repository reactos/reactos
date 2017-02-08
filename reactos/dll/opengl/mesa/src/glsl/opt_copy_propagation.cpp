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
 * \file opt_copy_propagation.cpp
 *
 * Moves usage of recently-copied variables to the previous copy of
 * the variable.
 *
 * This should reduce the number of MOV instructions in the generated
 * programs unless copy propagation is also done on the LIR, and may
 * help anyway by triggering other optimizations that live in the HIR.
 */

#include "ir.h"
#include "ir_visitor.h"
#include "ir_basic_block.h"
#include "ir_optimization.h"
#include "glsl_types.h"

class acp_entry : public exec_node
{
public:
   acp_entry(ir_variable *lhs, ir_variable *rhs)
   {
      assert(lhs);
      assert(rhs);
      this->lhs = lhs;
      this->rhs = rhs;
   }

   ir_variable *lhs;
   ir_variable *rhs;
};


class kill_entry : public exec_node
{
public:
   kill_entry(ir_variable *var)
   {
      assert(var);
      this->var = var;
   }

   ir_variable *var;
};

class ir_copy_propagation_visitor : public ir_hierarchical_visitor {
public:
   ir_copy_propagation_visitor()
   {
      progress = false;
      mem_ctx = ralloc_context(0);
      this->acp = new(mem_ctx) exec_list;
      this->kills = new(mem_ctx) exec_list;
   }
   ~ir_copy_propagation_visitor()
   {
      ralloc_free(mem_ctx);
   }

   virtual ir_visitor_status visit(class ir_dereference_variable *);
   virtual ir_visitor_status visit_enter(class ir_loop *);
   virtual ir_visitor_status visit_enter(class ir_function_signature *);
   virtual ir_visitor_status visit_enter(class ir_function *);
   virtual ir_visitor_status visit_leave(class ir_assignment *);
   virtual ir_visitor_status visit_enter(class ir_call *);
   virtual ir_visitor_status visit_enter(class ir_if *);

   void add_copy(ir_assignment *ir);
   void kill(ir_variable *ir);
   void handle_if_block(exec_list *instructions);

   /** List of acp_entry: The available copies to propagate */
   exec_list *acp;
   /**
    * List of kill_entry: The variables whose values were killed in this
    * block.
    */
   exec_list *kills;

   bool progress;

   bool killed_all;

   void *mem_ctx;
};

ir_visitor_status
ir_copy_propagation_visitor::visit_enter(ir_function_signature *ir)
{
   /* Treat entry into a function signature as a completely separate
    * block.  Any instructions at global scope will be shuffled into
    * main() at link time, so they're irrelevant to us.
    */
   exec_list *orig_acp = this->acp;
   exec_list *orig_kills = this->kills;
   bool orig_killed_all = this->killed_all;

   this->acp = new(mem_ctx) exec_list;
   this->kills = new(mem_ctx) exec_list;
   this->killed_all = false;

   visit_list_elements(this, &ir->body);

   this->kills = orig_kills;
   this->acp = orig_acp;
   this->killed_all = orig_killed_all;

   return visit_continue_with_parent;
}

ir_visitor_status
ir_copy_propagation_visitor::visit_leave(ir_assignment *ir)
{
   kill(ir->lhs->variable_referenced());

   add_copy(ir);

   return visit_continue;
}

ir_visitor_status
ir_copy_propagation_visitor::visit_enter(ir_function *ir)
{
   (void) ir;
   return visit_continue;
}

/**
 * Replaces dereferences of ACP RHS variables with ACP LHS variables.
 *
 * This is where the actual copy propagation occurs.  Note that the
 * rewriting of ir_dereference means that the ir_dereference instance
 * must not be shared by multiple IR operations!
 */
ir_visitor_status
ir_copy_propagation_visitor::visit(ir_dereference_variable *ir)
{
   if (this->in_assignee)
      return visit_continue;

   ir_variable *var = ir->var;

   foreach_iter(exec_list_iterator, iter, *this->acp) {
      acp_entry *entry = (acp_entry *)iter.get();

      if (var == entry->lhs) {
	 ir->var = entry->rhs;
	 this->progress = true;
	 break;
      }
   }

   return visit_continue;
}


ir_visitor_status
ir_copy_propagation_visitor::visit_enter(ir_call *ir)
{
   /* Do copy propagation on call parameters, but skip any out params */
   exec_list_iterator sig_param_iter = ir->get_callee()->parameters.iterator();
   foreach_iter(exec_list_iterator, iter, ir->actual_parameters) {
      ir_variable *sig_param = (ir_variable *)sig_param_iter.get();
      ir_instruction *ir = (ir_instruction *)iter.get();
      if (sig_param->mode != ir_var_out && sig_param->mode != ir_var_inout) {
         ir->accept(this);
      }
      sig_param_iter.next();
   }

   /* Since we're unlinked, we don't (necessarily) know the side effects of
    * this call.  So kill all copies.
    */
   acp->make_empty();
   this->killed_all = true;

   return visit_continue_with_parent;
}

void
ir_copy_propagation_visitor::handle_if_block(exec_list *instructions)
{
   exec_list *orig_acp = this->acp;
   exec_list *orig_kills = this->kills;
   bool orig_killed_all = this->killed_all;

   this->acp = new(mem_ctx) exec_list;
   this->kills = new(mem_ctx) exec_list;
   this->killed_all = false;

   /* Populate the initial acp with a copy of the original */
   foreach_iter(exec_list_iterator, iter, *orig_acp) {
      acp_entry *a = (acp_entry *)iter.get();
      this->acp->push_tail(new(this->mem_ctx) acp_entry(a->lhs, a->rhs));
   }

   visit_list_elements(this, instructions);

   if (this->killed_all) {
      orig_acp->make_empty();
   }

   exec_list *new_kills = this->kills;
   this->kills = orig_kills;
   this->acp = orig_acp;
   this->killed_all = this->killed_all || orig_killed_all;

   foreach_iter(exec_list_iterator, iter, *new_kills) {
      kill_entry *k = (kill_entry *)iter.get();
      kill(k->var);
   }
}

ir_visitor_status
ir_copy_propagation_visitor::visit_enter(ir_if *ir)
{
   ir->condition->accept(this);

   handle_if_block(&ir->then_instructions);
   handle_if_block(&ir->else_instructions);

   /* handle_if_block() already descended into the children. */
   return visit_continue_with_parent;
}

ir_visitor_status
ir_copy_propagation_visitor::visit_enter(ir_loop *ir)
{
   exec_list *orig_acp = this->acp;
   exec_list *orig_kills = this->kills;
   bool orig_killed_all = this->killed_all;

   /* FINISHME: For now, the initial acp for loops is totally empty.
    * We could go through once, then go through again with the acp
    * cloned minus the killed entries after the first run through.
    */
   this->acp = new(mem_ctx) exec_list;
   this->kills = new(mem_ctx) exec_list;
   this->killed_all = false;

   visit_list_elements(this, &ir->body_instructions);

   if (this->killed_all) {
      orig_acp->make_empty();
   }

   exec_list *new_kills = this->kills;
   this->kills = orig_kills;
   this->acp = orig_acp;
   this->killed_all = this->killed_all || orig_killed_all;

   foreach_iter(exec_list_iterator, iter, *new_kills) {
      kill_entry *k = (kill_entry *)iter.get();
      kill(k->var);
   }

   /* already descended into the children. */
   return visit_continue_with_parent;
}

void
ir_copy_propagation_visitor::kill(ir_variable *var)
{
   assert(var != NULL);

   /* Remove any entries currently in the ACP for this kill. */
   foreach_iter(exec_list_iterator, iter, *acp) {
      acp_entry *entry = (acp_entry *)iter.get();

      if (entry->lhs == var || entry->rhs == var) {
	 entry->remove();
      }
   }

   /* Add the LHS variable to the list of killed variables in this block.
    */
   this->kills->push_tail(new(this->mem_ctx) kill_entry(var));
}

/**
 * Adds an entry to the available copy list if it's a plain assignment
 * of a variable to a variable.
 */
void
ir_copy_propagation_visitor::add_copy(ir_assignment *ir)
{
   acp_entry *entry;

   if (ir->condition)
      return;

   ir_variable *lhs_var = ir->whole_variable_written();
   ir_variable *rhs_var = ir->rhs->whole_variable_referenced();

   if ((lhs_var != NULL) && (rhs_var != NULL)) {
      if (lhs_var == rhs_var) {
	 /* This is a dumb assignment, but we've conveniently noticed
	  * it here.  Removing it now would mess up the loop iteration
	  * calling us.  Just flag it to not execute, and someone else
	  * will clean up the mess.
	  */
	 ir->condition = new(ralloc_parent(ir)) ir_constant(false);
	 this->progress = true;
      } else {
	 entry = new(this->mem_ctx) acp_entry(lhs_var, rhs_var);
	 this->acp->push_tail(entry);
      }
   }
}

/**
 * Does a copy propagation pass on the code present in the instruction stream.
 */
bool
do_copy_propagation(exec_list *instructions)
{
   ir_copy_propagation_visitor v;

   visit_list_elements(&v, instructions);

   return v.progress;
}
