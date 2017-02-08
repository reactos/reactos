/*
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * constant of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, constant, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above constantright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR CONSTANTRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file opt_constant_propagation.cpp
 *
 * Tracks assignments of constants to channels of variables, and
 * usage of those constant channels with direct usage of the constants.
 *
 * This can lead to constant folding and algebraic optimizations in
 * those later expressions, while causing no increase in instruction
 * count (due to constants being generally free to load from a
 * constant push buffer or as instruction immediate values) and
 * possibly reducing register pressure.
 */

#include "ir.h"
#include "ir_visitor.h"
#include "ir_rvalue_visitor.h"
#include "ir_basic_block.h"
#include "ir_optimization.h"
#include "glsl_types.h"

class acp_entry : public exec_node
{
public:
   acp_entry(ir_variable *var, unsigned write_mask, ir_constant *constant)
   {
      assert(var);
      assert(constant);
      this->var = var;
      this->write_mask = write_mask;
      this->constant = constant;
      this->initial_values = write_mask;
   }

   acp_entry(const acp_entry *src)
   {
      this->var = src->var;
      this->write_mask = src->write_mask;
      this->constant = src->constant;
      this->initial_values = src->initial_values;
   }

   ir_variable *var;
   ir_constant *constant;
   unsigned write_mask;

   /** Mask of values initially available in the constant. */
   unsigned initial_values;
};


class kill_entry : public exec_node
{
public:
   kill_entry(ir_variable *var, unsigned write_mask)
   {
      assert(var);
      this->var = var;
      this->write_mask = write_mask;
   }

   ir_variable *var;
   unsigned write_mask;
};

class ir_constant_propagation_visitor : public ir_rvalue_visitor {
public:
   ir_constant_propagation_visitor()
   {
      progress = false;
      mem_ctx = ralloc_context(0);
      this->acp = new(mem_ctx) exec_list;
      this->kills = new(mem_ctx) exec_list;
   }
   ~ir_constant_propagation_visitor()
   {
      ralloc_free(mem_ctx);
   }

   virtual ir_visitor_status visit_enter(class ir_loop *);
   virtual ir_visitor_status visit_enter(class ir_function_signature *);
   virtual ir_visitor_status visit_enter(class ir_function *);
   virtual ir_visitor_status visit_leave(class ir_assignment *);
   virtual ir_visitor_status visit_enter(class ir_call *);
   virtual ir_visitor_status visit_enter(class ir_if *);

   void add_constant(ir_assignment *ir);
   void kill(ir_variable *ir, unsigned write_mask);
   void handle_if_block(exec_list *instructions);
   void handle_rvalue(ir_rvalue **rvalue);

   /** List of acp_entry: The available constants to propagate */
   exec_list *acp;

   /**
    * List of kill_entry: The masks of variables whose values were
    * killed in this block.
    */
   exec_list *kills;

   bool progress;

   bool killed_all;

   void *mem_ctx;
};


void
ir_constant_propagation_visitor::handle_rvalue(ir_rvalue **rvalue)
{
   if (this->in_assignee || !*rvalue)
      return;

   const glsl_type *type = (*rvalue)->type;
   if (!type->is_scalar() && !type->is_vector())
      return;

   ir_swizzle *swiz = NULL;
   ir_dereference_variable *deref = (*rvalue)->as_dereference_variable();
   if (!deref) {
      swiz = (*rvalue)->as_swizzle();
      if (!swiz)
	 return;

      deref = swiz->val->as_dereference_variable();
      if (!deref)
	 return;
   }

   ir_constant_data data;
   memset(&data, 0, sizeof(data));

   for (unsigned int i = 0; i < type->components(); i++) {
      int channel;
      acp_entry *found = NULL;

      if (swiz) {
	 switch (i) {
	 case 0: channel = swiz->mask.x; break;
	 case 1: channel = swiz->mask.y; break;
	 case 2: channel = swiz->mask.z; break;
	 case 3: channel = swiz->mask.w; break;
	 default: assert(!"shouldn't be reached"); channel = 0; break;
	 }
      } else {
	 channel = i;
      }

      foreach_iter(exec_list_iterator, iter, *this->acp) {
	 acp_entry *entry = (acp_entry *)iter.get();
	 if (entry->var == deref->var && entry->write_mask & (1 << channel)) {
	    found = entry;
	    break;
	 }
      }

      if (!found)
	 return;

      int rhs_channel = 0;
      for (int j = 0; j < 4; j++) {
	 if (j == channel)
	    break;
	 if (found->initial_values & (1 << j))
	    rhs_channel++;
      }

      switch (type->base_type) {
      case GLSL_TYPE_FLOAT:
	 data.f[i] = found->constant->value.f[rhs_channel];
	 break;
      case GLSL_TYPE_INT:
	 data.i[i] = found->constant->value.i[rhs_channel];
	 break;
      case GLSL_TYPE_UINT:
	 data.u[i] = found->constant->value.u[rhs_channel];
	 break;
      case GLSL_TYPE_BOOL:
	 data.b[i] = found->constant->value.b[rhs_channel];
	 break;
      default:
	 assert(!"not reached");
	 break;
      }
   }

   *rvalue = new(ralloc_parent(deref)) ir_constant(type, &data);
   this->progress = true;
}

ir_visitor_status
ir_constant_propagation_visitor::visit_enter(ir_function_signature *ir)
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
ir_constant_propagation_visitor::visit_leave(ir_assignment *ir)
{
   if (this->in_assignee)
      return visit_continue;

   kill(ir->lhs->variable_referenced(), ir->write_mask);

   add_constant(ir);

   return visit_continue;
}

ir_visitor_status
ir_constant_propagation_visitor::visit_enter(ir_function *ir)
{
   (void) ir;
   return visit_continue;
}

ir_visitor_status
ir_constant_propagation_visitor::visit_enter(ir_call *ir)
{
   /* Do constant propagation on call parameters, but skip any out params */
   exec_list_iterator sig_param_iter = ir->get_callee()->parameters.iterator();
   foreach_iter(exec_list_iterator, iter, ir->actual_parameters) {
      ir_variable *sig_param = (ir_variable *)sig_param_iter.get();
      ir_rvalue *param = (ir_rvalue *)iter.get();
      if (sig_param->mode != ir_var_out && sig_param->mode != ir_var_inout) {
	 ir_rvalue *new_param = param;
	 handle_rvalue(&new_param);
         if (new_param != param)
	    param->replace_with(new_param);
	 else
	    param->accept(this);
      }
      sig_param_iter.next();
   }

   /* Since we're unlinked, we don't (necssarily) know the side effects of
    * this call.  So kill all copies.
    */
   acp->make_empty();
   this->killed_all = true;

   return visit_continue_with_parent;
}

void
ir_constant_propagation_visitor::handle_if_block(exec_list *instructions)
{
   exec_list *orig_acp = this->acp;
   exec_list *orig_kills = this->kills;
   bool orig_killed_all = this->killed_all;

   this->acp = new(mem_ctx) exec_list;
   this->kills = new(mem_ctx) exec_list;
   this->killed_all = false;

   /* Populate the initial acp with a constant of the original */
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

   foreach_iter(exec_list_iterator, iter, *new_kills) {
      kill_entry *k = (kill_entry *)iter.get();
      kill(k->var, k->write_mask);
   }
}

ir_visitor_status
ir_constant_propagation_visitor::visit_enter(ir_if *ir)
{
   ir->condition->accept(this);
   handle_rvalue(&ir->condition);

   handle_if_block(&ir->then_instructions);
   handle_if_block(&ir->else_instructions);

   /* handle_if_block() already descended into the children. */
   return visit_continue_with_parent;
}

ir_visitor_status
ir_constant_propagation_visitor::visit_enter(ir_loop *ir)
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
      kill(k->var, k->write_mask);
   }

   /* already descended into the children. */
   return visit_continue_with_parent;
}

void
ir_constant_propagation_visitor::kill(ir_variable *var, unsigned write_mask)
{
   assert(var != NULL);

   /* We don't track non-vectors. */
   if (!var->type->is_vector() && !var->type->is_scalar())
      return;

   /* Remove any entries currently in the ACP for this kill. */
   foreach_iter(exec_list_iterator, iter, *this->acp) {
      acp_entry *entry = (acp_entry *)iter.get();

      if (entry->var == var) {
	 entry->write_mask &= ~write_mask;
	 if (entry->write_mask == 0)
	    entry->remove();
      }
   }

   /* Add this writemask of the variable to the list of killed
    * variables in this block.
    */
   foreach_iter(exec_list_iterator, iter, *this->kills) {
      kill_entry *entry = (kill_entry *)iter.get();

      if (entry->var == var) {
	 entry->write_mask |= write_mask;
	 return;
      }
   }
   /* Not already in the list.  Make new entry. */
   this->kills->push_tail(new(this->mem_ctx) kill_entry(var, write_mask));
}

/**
 * Adds an entry to the available constant list if it's a plain assignment
 * of a variable to a variable.
 */
void
ir_constant_propagation_visitor::add_constant(ir_assignment *ir)
{
   acp_entry *entry;

   if (ir->condition)
      return;

   if (!ir->write_mask)
      return;

   ir_dereference_variable *deref = ir->lhs->as_dereference_variable();
   ir_constant *constant = ir->rhs->as_constant();

   if (!deref || !constant)
      return;

   /* Only do constant propagation on vectors.  Constant matrices,
    * arrays, or structures would require more work elsewhere.
    */
   if (!deref->var->type->is_vector() && !deref->var->type->is_scalar())
      return;

   entry = new(this->mem_ctx) acp_entry(deref->var, ir->write_mask, constant);
   this->acp->push_tail(entry);
}

/**
 * Does a constant propagation pass on the code present in the instruction stream.
 */
bool
do_constant_propagation(exec_list *instructions)
{
   ir_constant_propagation_visitor v;

   visit_list_elements(&v, instructions);

   return v.progress;
}
