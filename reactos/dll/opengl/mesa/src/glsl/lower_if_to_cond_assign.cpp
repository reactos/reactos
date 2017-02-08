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
 * \file lower_if_to_cond_assign.cpp
 *
 * This attempts to flatten if-statements to conditional assignments for
 * GPUs with limited or no flow control support.
 *
 * It can't handle other control flow being inside of its block, such
 * as calls or loops.  Hopefully loop unrolling and inlining will take
 * care of those.
 *
 * Drivers for GPUs with no control flow support should simply call
 *
 *    lower_if_to_cond_assign(instructions)
 *
 * to attempt to flatten all if-statements.
 *
 * Some GPUs (such as i965 prior to gen6) do support control flow, but have a
 * maximum nesting depth N.  Drivers for such hardware can call
 *
 *    lower_if_to_cond_assign(instructions, N)
 *
 * to attempt to flatten any if-statements appearing at depth > N.
 */

#include "glsl_types.h"
#include "ir.h"
#include "program/hash_table.h"

class ir_if_to_cond_assign_visitor : public ir_hierarchical_visitor {
public:
   ir_if_to_cond_assign_visitor(unsigned max_depth)
   {
      this->progress = false;
      this->max_depth = max_depth;
      this->depth = 0;

      this->condition_variables = hash_table_ctor(0, hash_table_pointer_hash,
						  hash_table_pointer_compare);
   }

   ~ir_if_to_cond_assign_visitor()
   {
      hash_table_dtor(this->condition_variables);
   }

   ir_visitor_status visit_enter(ir_if *);
   ir_visitor_status visit_leave(ir_if *);

   bool progress;
   unsigned max_depth;
   unsigned depth;

   struct hash_table *condition_variables;
};

bool
lower_if_to_cond_assign(exec_list *instructions, unsigned max_depth)
{
   if (max_depth == UINT_MAX)
      return false;

   ir_if_to_cond_assign_visitor v(max_depth);

   visit_list_elements(&v, instructions);

   return v.progress;
}

void
check_control_flow(ir_instruction *ir, void *data)
{
   bool *found_control_flow = (bool *)data;
   switch (ir->ir_type) {
   case ir_type_call:
   case ir_type_discard:
   case ir_type_loop:
   case ir_type_loop_jump:
   case ir_type_return:
      *found_control_flow = true;
      break;
   default:
      break;
   }
}

void
move_block_to_cond_assign(void *mem_ctx,
			  ir_if *if_ir, ir_rvalue *cond_expr,
			  exec_list *instructions,
			  struct hash_table *ht)
{
   foreach_list_safe(node, instructions) {
      ir_instruction *ir = (ir_instruction *) node;

      if (ir->ir_type == ir_type_assignment) {
	 ir_assignment *assign = (ir_assignment *)ir;

	 if (hash_table_find(ht, assign) == NULL) {
	    hash_table_insert(ht, assign, assign);

	    /* If the LHS of the assignment is a condition variable that was
	     * previously added, insert an additional assignment of false to
	     * the variable.
	     */
	    const bool assign_to_cv =
	       hash_table_find(ht, assign->lhs->variable_referenced()) != NULL;

	    if (!assign->condition) {
	       if (assign_to_cv) {
		  assign->rhs =
		     new(mem_ctx) ir_expression(ir_binop_logic_and,
						glsl_type::bool_type,
						cond_expr->clone(mem_ctx, NULL),
						assign->rhs);
	       } else {
		  assign->condition = cond_expr->clone(mem_ctx, NULL);
	       }
	    } else {
	       assign->condition =
		  new(mem_ctx) ir_expression(ir_binop_logic_and,
					     glsl_type::bool_type,
					     cond_expr->clone(mem_ctx, NULL),
					     assign->condition);
	    }
	 }
      }

      /* Now, move from the if block to the block surrounding it. */
      ir->remove();
      if_ir->insert_before(ir);
   }
}

ir_visitor_status
ir_if_to_cond_assign_visitor::visit_enter(ir_if *ir)
{
   (void) ir;
   this->depth++;

   return visit_continue;
}

ir_visitor_status
ir_if_to_cond_assign_visitor::visit_leave(ir_if *ir)
{
   /* Only flatten when beyond the GPU's maximum supported nesting depth. */
   if (this->depth-- <= this->max_depth)
      return visit_continue;

   bool found_control_flow = false;
   ir_assignment *assign;

   /* Check that both blocks don't contain anything we can't support. */
   foreach_iter(exec_list_iterator, then_iter, ir->then_instructions) {
      ir_instruction *then_ir = (ir_instruction *)then_iter.get();
      visit_tree(then_ir, check_control_flow, &found_control_flow);
   }
   foreach_iter(exec_list_iterator, else_iter, ir->else_instructions) {
      ir_instruction *else_ir = (ir_instruction *)else_iter.get();
      visit_tree(else_ir, check_control_flow, &found_control_flow);
   }
   if (found_control_flow)
      return visit_continue;

   void *mem_ctx = ralloc_parent(ir);

   /* Store the condition to a variable.  Move all of the instructions from
    * the then-clause of the if-statement.  Use the condition variable as a
    * condition for all assignments.
    */
   ir_variable *const then_var =
      new(mem_ctx) ir_variable(glsl_type::bool_type,
			       "if_to_cond_assign_then",
			       ir_var_temporary);
   ir->insert_before(then_var);

   ir_dereference_variable *then_cond =
      new(mem_ctx) ir_dereference_variable(then_var);

   assign = new(mem_ctx) ir_assignment(then_cond, ir->condition);
   ir->insert_before(assign);

   move_block_to_cond_assign(mem_ctx, ir, then_cond,
			     &ir->then_instructions,
			     this->condition_variables);

   /* Add the new condition variable to the hash table.  This allows us to
    * find this variable when lowering other (enclosing) if-statements.
    */
   hash_table_insert(this->condition_variables, then_var, then_var);

   /* If there are instructions in the else-clause, store the inverse of the
    * condition to a variable.  Move all of the instructions from the
    * else-clause if the if-statement.  Use the (inverse) condition variable
    * as a condition for all assignments.
    */
   if (!ir->else_instructions.is_empty()) {
      ir_variable *const else_var =
	 new(mem_ctx) ir_variable(glsl_type::bool_type,
				  "if_to_cond_assign_else",
				  ir_var_temporary);
      ir->insert_before(else_var);

      ir_dereference_variable *else_cond =
	 new(mem_ctx) ir_dereference_variable(else_var);

      ir_rvalue *inverse =
	 new(mem_ctx) ir_expression(ir_unop_logic_not,
				    then_cond->clone(mem_ctx, NULL));

      assign = new(mem_ctx) ir_assignment(else_cond, inverse);
      ir->insert_before(assign);

      move_block_to_cond_assign(mem_ctx, ir, else_cond,
				&ir->else_instructions,
				this->condition_variables);

      /* Add the new condition variable to the hash table.  This allows us to
       * find this variable when lowering other (enclosing) if-statements.
       */
      hash_table_insert(this->condition_variables, else_var, else_var);
   }

   ir->remove();

   this->progress = true;

   return visit_continue;
}
