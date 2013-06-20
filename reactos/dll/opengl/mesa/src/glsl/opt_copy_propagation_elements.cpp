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
 * \file opt_copy_propagation_elements.cpp
 *
 * Replaces usage of recently-copied components of variables with the
 * previous copy of the variable.
 *
 * This pass can be compared with opt_copy_propagation, which operands
 * on arbitrary whole-variable copies.  However, in order to handle
 * the copy propagation of swizzled variables or writemasked writes,
 * we want to track things on a channel-wise basis.  I found that
 * trying to mix the swizzled/writemasked support here with the
 * whole-variable stuff in opt_copy_propagation.cpp just made a mess,
 * so this is separate despite the ACP handling being somewhat
 * similar.
 *
 * This should reduce the number of MOV instructions in the generated
 * programs unless copy propagation is also done on the LIR, and may
 * help anyway by triggering other optimizations that live in the HIR.
 */

#include "ir.h"
#include "ir_rvalue_visitor.h"
#include "ir_basic_block.h"
#include "ir_optimization.h"
#include "glsl_types.h"

static bool debug = false;

class acp_entry : public exec_node
{
public:
   acp_entry(ir_variable *lhs, ir_variable *rhs, int write_mask, int swizzle[4])
   {
      this->lhs = lhs;
      this->rhs = rhs;
      this->write_mask = write_mask;
      memcpy(this->swizzle, swizzle, sizeof(this->swizzle));
   }

   acp_entry(acp_entry *a)
   {
      this->lhs = a->lhs;
      this->rhs = a->rhs;
      this->write_mask = a->write_mask;
      memcpy(this->swizzle, a->swizzle, sizeof(this->swizzle));
   }

   ir_variable *lhs;
   ir_variable *rhs;
   unsigned int write_mask;
   int swizzle[4];
};


class kill_entry : public exec_node
{
public:
   kill_entry(ir_variable *var, int write_mask)
   {
      this->var = var;
      this->write_mask = write_mask;
   }

   ir_variable *var;
   unsigned int write_mask;
};

class ir_copy_propagation_elements_visitor : public ir_rvalue_visitor {
public:
   ir_copy_propagation_elements_visitor()
   {
      this->progress = false;
      this->mem_ctx = ralloc_context(NULL);
      this->shader_mem_ctx = NULL;
      this->acp = new(mem_ctx) exec_list;
      this->kills = new(mem_ctx) exec_list;
   }
   ~ir_copy_propagation_elements_visitor()
   {
      ralloc_free(mem_ctx);
   }

   virtual ir_visitor_status visit_enter(class ir_loop *);
   virtual ir_visitor_status visit_enter(class ir_function_signature *);
   virtual ir_visitor_status visit_leave(class ir_assignment *);
   virtual ir_visitor_status visit_enter(class ir_call *);
   virtual ir_visitor_status visit_enter(class ir_if *);
   virtual ir_visitor_status visit_leave(class ir_swizzle *);

   void handle_rvalue(ir_rvalue **rvalue);

   void add_copy(ir_assignment *ir);
   void kill(kill_entry *k);
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

   /* Context for our local data structures. */
   void *mem_ctx;
   /* Context for allocating new shader nodes. */
   void *shader_mem_ctx;
};

ir_visitor_status
ir_copy_propagation_elements_visitor::visit_enter(ir_function_signature *ir)
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
ir_copy_propagation_elements_visitor::visit_leave(ir_assignment *ir)
{
   ir_dereference_variable *lhs = ir->lhs->as_dereference_variable();
   ir_variable *var = ir->lhs->variable_referenced();

   if (var->type->is_scalar() || var->type->is_vector()) {
      kill_entry *k;

      if (lhs)
	 k = new(mem_ctx) kill_entry(var, ir->write_mask);
      else
	 k = new(mem_ctx) kill_entry(var, ~0);

      kill(k);
   }

   add_copy(ir);

   return visit_continue;
}

ir_visitor_status
ir_copy_propagation_elements_visitor::visit_leave(ir_swizzle *ir)
{
   /* Don't visit the values of swizzles since they are handled while
    * visiting the swizzle itself.
    */
   return visit_continue;
}

/**
 * Replaces dereferences of ACP RHS variables with ACP LHS variables.
 *
 * This is where the actual copy propagation occurs.  Note that the
 * rewriting of ir_dereference means that the ir_dereference instance
 * must not be shared by multiple IR operations!
 */
void
ir_copy_propagation_elements_visitor::handle_rvalue(ir_rvalue **ir)
{
   int swizzle_chan[4];
   ir_dereference_variable *deref_var;
   ir_variable *source[4] = {NULL, NULL, NULL, NULL};
   int source_chan[4];
   int chans;

   if (!*ir)
      return;

   ir_swizzle *swizzle = (*ir)->as_swizzle();
   if (swizzle) {
      deref_var = swizzle->val->as_dereference_variable();
      if (!deref_var)
	 return;

      swizzle_chan[0] = swizzle->mask.x;
      swizzle_chan[1] = swizzle->mask.y;
      swizzle_chan[2] = swizzle->mask.z;
      swizzle_chan[3] = swizzle->mask.w;
      chans = swizzle->type->vector_elements;
   } else {
      deref_var = (*ir)->as_dereference_variable();
      if (!deref_var)
	 return;

      swizzle_chan[0] = 0;
      swizzle_chan[1] = 1;
      swizzle_chan[2] = 2;
      swizzle_chan[3] = 3;
      chans = deref_var->type->vector_elements;
   }

   if (this->in_assignee)
      return;

   ir_variable *var = deref_var->var;

   /* Try to find ACP entries covering swizzle_chan[], hoping they're
    * the same source variable.
    */
   foreach_iter(exec_list_iterator, iter, *this->acp) {
      acp_entry *entry = (acp_entry *)iter.get();

      if (var == entry->lhs) {
	 for (int c = 0; c < chans; c++) {
	    if (entry->write_mask & (1 << swizzle_chan[c])) {
	       source[c] = entry->rhs;
	       source_chan[c] = entry->swizzle[swizzle_chan[c]];
	    }
	 }
      }
   }

   /* Make sure all channels are copying from the same source variable. */
   if (!source[0])
      return;
   for (int c = 1; c < chans; c++) {
      if (source[c] != source[0])
	 return;
   }

   if (!shader_mem_ctx)
      shader_mem_ctx = ralloc_parent(deref_var);

   if (debug) {
      printf("Copy propagation from:\n");
      (*ir)->print();
   }

   deref_var = new(shader_mem_ctx) ir_dereference_variable(source[0]);
   *ir = new(shader_mem_ctx) ir_swizzle(deref_var,
					source_chan[0],
					source_chan[1],
					source_chan[2],
					source_chan[3],
					chans);

   if (debug) {
      printf("to:\n");
      (*ir)->print();
      printf("\n");
   }
}


ir_visitor_status
ir_copy_propagation_elements_visitor::visit_enter(ir_call *ir)
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
ir_copy_propagation_elements_visitor::handle_if_block(exec_list *instructions)
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
      this->acp->push_tail(new(this->mem_ctx) acp_entry(a));
   }

   visit_list_elements(this, instructions);

   if (this->killed_all) {
      orig_acp->make_empty();
   }

   exec_list *new_kills = this->kills;
   this->kills = orig_kills;
   this->acp = orig_acp;
   this->killed_all = this->killed_all || orig_killed_all;

   /* Move the new kills into the parent block's list, removing them
    * from the parent's ACP list in the process.
    */
   foreach_list_safe(node, new_kills) {
      kill_entry *k = (kill_entry *)node;
      kill(k);
   }
}

ir_visitor_status
ir_copy_propagation_elements_visitor::visit_enter(ir_if *ir)
{
   ir->condition->accept(this);

   handle_if_block(&ir->then_instructions);
   handle_if_block(&ir->else_instructions);

   /* handle_if_block() already descended into the children. */
   return visit_continue_with_parent;
}

ir_visitor_status
ir_copy_propagation_elements_visitor::visit_enter(ir_loop *ir)
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

   foreach_list_safe(node, new_kills) {
      kill_entry *k = (kill_entry *)node;
      kill(k);
   }

   /* already descended into the children. */
   return visit_continue_with_parent;
}

/* Remove any entries currently in the ACP for this kill. */
void
ir_copy_propagation_elements_visitor::kill(kill_entry *k)
{
   foreach_list_safe(node, acp) {
      acp_entry *entry = (acp_entry *)node;

      if (entry->lhs == k->var) {
	 entry->write_mask = entry->write_mask & ~k->write_mask;
	 if (entry->write_mask == 0) {
	    entry->remove();
	    continue;
	 }
      }
      if (entry->rhs == k->var) {
	 entry->remove();
      }
   }

   /* If we were on a list, remove ourselves before inserting */
   if (k->next)
      k->remove();

   this->kills->push_tail(k);
}

/**
 * Adds directly-copied channels between vector variables to the available
 * copy propagation list.
 */
void
ir_copy_propagation_elements_visitor::add_copy(ir_assignment *ir)
{
   acp_entry *entry;
   int orig_swizzle[4] = {0, 1, 2, 3};
   int swizzle[4];

   if (ir->condition)
      return;

   ir_dereference_variable *lhs = ir->lhs->as_dereference_variable();
   if (!lhs || !(lhs->type->is_scalar() || lhs->type->is_vector()))
      return;

   ir_dereference_variable *rhs = ir->rhs->as_dereference_variable();
   if (!rhs) {
      ir_swizzle *swiz = ir->rhs->as_swizzle();
      if (!swiz)
	 return;

      rhs = swiz->val->as_dereference_variable();
      if (!rhs)
	 return;

      orig_swizzle[0] = swiz->mask.x;
      orig_swizzle[1] = swiz->mask.y;
      orig_swizzle[2] = swiz->mask.z;
      orig_swizzle[3] = swiz->mask.w;
   }

   /* Move the swizzle channels out to the positions they match in the
    * destination.  We don't want to have to rewrite the swizzle[]
    * array every time we clear a bit of the write_mask.
    */
   int j = 0;
   for (int i = 0; i < 4; i++) {
      if (ir->write_mask & (1 << i))
	 swizzle[i] = orig_swizzle[j++];
   }

   int write_mask = ir->write_mask;
   if (lhs->var == rhs->var) {
      /* If this is a copy from the variable to itself, then we need
       * to be sure not to include the updated channels from this
       * instruction in the set of new source channels to be
       * copy-propagated from.
       */
      for (int i = 0; i < 4; i++) {
	 if (ir->write_mask & (1 << orig_swizzle[i]))
	    write_mask &= ~(1 << i);
      }
   }

   entry = new(this->mem_ctx) acp_entry(lhs->var, rhs->var, write_mask,
					swizzle);
   this->acp->push_tail(entry);
}

bool
do_copy_propagation_elements(exec_list *instructions)
{
   ir_copy_propagation_elements_visitor v;

   visit_list_elements(&v, instructions);

   return v.progress;
}
