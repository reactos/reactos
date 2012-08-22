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

#include <limits.h>
#include "main/compiler.h"
#include "glsl_types.h"
#include "loop_analysis.h"
#include "ir_hierarchical_visitor.h"

/**
 * Find an initializer of a variable outside a loop
 *
 * Works backwards from the loop to find the pre-loop value of the variable.
 * This is used, for example, to find the initial value of loop induction
 * variables.
 *
 * \param loop  Loop where \c var is an induction variable
 * \param var   Variable whose initializer is to be found
 *
 * \return
 * The \c ir_rvalue assigned to the variable outside the loop.  May return
 * \c NULL if no initializer can be found.
 */
ir_rvalue *
find_initial_value(ir_loop *loop, ir_variable *var)
{
   for (exec_node *node = loop->prev;
	!node->is_head_sentinel();
	node = node->prev) {
      ir_instruction *ir = (ir_instruction *) node;

      switch (ir->ir_type) {
      case ir_type_call:
      case ir_type_loop:
      case ir_type_loop_jump:
      case ir_type_return:
      case ir_type_if:
	 return NULL;

      case ir_type_function:
      case ir_type_function_signature:
	 assert(!"Should not get here.");
	 return NULL;

      case ir_type_assignment: {
	 ir_assignment *assign = ir->as_assignment();
	 ir_variable *assignee = assign->lhs->whole_variable_referenced();

	 if (assignee == var)
	    return (assign->condition != NULL) ? NULL : assign->rhs;

	 break;
      }

      default:
	 break;
      }
   }

   return NULL;
}


int
calculate_iterations(ir_rvalue *from, ir_rvalue *to, ir_rvalue *increment,
		     enum ir_expression_operation op)
{
   if (from == NULL || to == NULL || increment == NULL)
      return -1;

   void *mem_ctx = ralloc_context(NULL);

   ir_expression *const sub =
      new(mem_ctx) ir_expression(ir_binop_sub, from->type, to, from);

   ir_expression *const div =
      new(mem_ctx) ir_expression(ir_binop_div, sub->type, sub, increment);

   ir_constant *iter = div->constant_expression_value();

   if (iter == NULL)
      return -1;

   if (!iter->type->is_integer()) {
      ir_rvalue *cast =
	 new(mem_ctx) ir_expression(ir_unop_f2i, glsl_type::int_type, iter,
				    NULL);

      iter = cast->constant_expression_value();
   }

   int iter_value = iter->get_int_component(0);

   /* Make sure that the calculated number of iterations satisfies the exit
    * condition.  This is needed to catch off-by-one errors and some types of
    * ill-formed loops.  For example, we need to detect that the following
    * loop does not have a maximum iteration count.
    *
    *    for (float x = 0.0; x != 0.9; x += 0.2)
    *        ;
    */
   const int bias[] = { -1, 0, 1 };
   bool valid_loop = false;

   for (unsigned i = 0; i < Elements(bias); i++) {
      iter = (increment->type->is_integer())
	 ? new(mem_ctx) ir_constant(iter_value + bias[i])
	 : new(mem_ctx) ir_constant(float(iter_value + bias[i]));

      ir_expression *const mul =
	 new(mem_ctx) ir_expression(ir_binop_mul, increment->type, iter,
				    increment);

      ir_expression *const add =
	 new(mem_ctx) ir_expression(ir_binop_add, mul->type, mul, from);

      ir_expression *const cmp =
	 new(mem_ctx) ir_expression(op, glsl_type::bool_type, add, to);

      ir_constant *const cmp_result = cmp->constant_expression_value();

      assert(cmp_result != NULL);
      if (cmp_result->get_bool_component(0)) {
	 iter_value += bias[i];
	 valid_loop = true;
	 break;
      }
   }

   ralloc_free(mem_ctx);
   return (valid_loop) ? iter_value : -1;
}


class loop_control_visitor : public ir_hierarchical_visitor {
public:
   loop_control_visitor(loop_state *state)
   {
      this->state = state;
      this->progress = false;
   }

   virtual ir_visitor_status visit_leave(ir_loop *ir);

   loop_state *state;

   bool progress;
};


ir_visitor_status
loop_control_visitor::visit_leave(ir_loop *ir)
{
   loop_variable_state *const ls = this->state->get(ir);

   /* If we've entered a loop that hasn't been analyzed, something really,
    * really bad has happened.
    */
   if (ls == NULL) {
      assert(ls != NULL);
      return visit_continue;
   }

   /* Search the loop terminating conditions for one of the form 'i < c' where
    * i is a loop induction variable, c is a constant, and < is any relative
    * operator.
    */
   int max_iterations = ls->max_iterations;

   if(ir->from && ir->to && ir->increment)
      max_iterations = calculate_iterations(ir->from, ir->to, ir->increment, (ir_expression_operation)ir->cmp);

   if(max_iterations < 0)
      max_iterations = INT_MAX;

   foreach_list(node, &ls->terminators) {
      loop_terminator *t = (loop_terminator *) node;
      ir_if *if_stmt = t->ir;

      /* If-statements can be either 'if (expr)' or 'if (deref)'.  We only care
       * about the former here.
       */
      ir_expression *cond = if_stmt->condition->as_expression();
      if (cond == NULL)
	 continue;

      switch (cond->operation) {
      case ir_binop_less:
      case ir_binop_greater:
      case ir_binop_lequal:
      case ir_binop_gequal: {
	 /* The expressions that we care about will either be of the form
	  * 'counter < limit' or 'limit < counter'.  Figure out which is
	  * which.
	  */
	 ir_rvalue *counter = cond->operands[0]->as_dereference_variable();
	 ir_constant *limit = cond->operands[1]->as_constant();
	 enum ir_expression_operation cmp = cond->operation;

	 if (limit == NULL) {
	    counter = cond->operands[1]->as_dereference_variable();
	    limit = cond->operands[0]->as_constant();

	    switch (cmp) {
	    case ir_binop_less:    cmp = ir_binop_gequal;  break;
	    case ir_binop_greater: cmp = ir_binop_lequal;  break;
	    case ir_binop_lequal:  cmp = ir_binop_greater; break;
	    case ir_binop_gequal:  cmp = ir_binop_less;    break;
	    default: assert(!"Should not get here.");
	    }
	 }

	 if ((counter == NULL) || (limit == NULL))
	    break;

	 ir_variable *var = counter->variable_referenced();

	 ir_rvalue *init = find_initial_value(ir, var);

	 foreach_list(iv_node, &ls->induction_variables) {
	    loop_variable *lv = (loop_variable *) iv_node;

	    if (lv->var == var) {
	       const int iterations = calculate_iterations(init, limit,
							   lv->increment,
							   cmp);
	       if (iterations >= 0) {
		  /* If the new iteration count is lower than the previously
		   * believed iteration count, update the loop control values.
		   */
		  if (iterations < max_iterations) {
		     ir->from = init->clone(ir, NULL);
		     ir->to = limit->clone(ir, NULL);
		     ir->increment = lv->increment->clone(ir, NULL);
		     ir->counter = lv->var;
		     ir->cmp = cmp;

		     max_iterations = iterations;
		  }

		  /* Remove the conditional break statement.  The loop
		   * controls are now set such that the exit condition will be
		   * satisfied.
		   */
		  if_stmt->remove();

		  assert(ls->num_loop_jumps > 0);
		  ls->num_loop_jumps--;

		  this->progress = true;
	       }

	       break;
	    }
	 }
	 break;
      }

      default:
	 break;
      }
   }

   /* If we have proven the one of the loop exit conditions is satisifed before
    * running the loop once, remove the loop.
    */
   if (max_iterations == 0)
      ir->remove();
   else
      ls->max_iterations = max_iterations;

   return visit_continue;
}


bool
set_loop_controls(exec_list *instructions, loop_state *ls)
{
   loop_control_visitor v(ls);

   v.run(instructions);

   return v.progress;
}
