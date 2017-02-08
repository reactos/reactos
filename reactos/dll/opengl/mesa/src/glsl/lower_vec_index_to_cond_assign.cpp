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
 * \file lower_vec_index_to_cond_assign.cpp
 *
 * Turns indexing into vector types to a series of conditional moves
 * of each channel's swizzle into a temporary.
 *
 * Most GPUs don't have a native way to do this operation, and this
 * works around that.  For drivers using both this pass and
 * ir_vec_index_to_swizzle, there's a risk that this pass will happen
 * before sufficient constant folding to find that the array index is
 * constant.  However, we hope that other optimization passes,
 * particularly constant folding of assignment conditions and copy
 * propagation, will result in the same code in the end.
 */

#include "ir.h"
#include "ir_visitor.h"
#include "ir_optimization.h"
#include "glsl_types.h"

/**
 * Visitor class for replacing expressions with ir_constant values.
 */

class ir_vec_index_to_cond_assign_visitor : public ir_hierarchical_visitor {
public:
   ir_vec_index_to_cond_assign_visitor()
   {
      progress = false;
   }

   ir_rvalue *convert_vec_index_to_cond_assign(ir_rvalue *val);

   virtual ir_visitor_status visit_enter(ir_expression *);
   virtual ir_visitor_status visit_enter(ir_swizzle *);
   virtual ir_visitor_status visit_leave(ir_assignment *);
   virtual ir_visitor_status visit_enter(ir_return *);
   virtual ir_visitor_status visit_enter(ir_call *);
   virtual ir_visitor_status visit_enter(ir_if *);

   bool progress;
};

ir_rvalue *
ir_vec_index_to_cond_assign_visitor::convert_vec_index_to_cond_assign(ir_rvalue *ir)
{
   ir_dereference_array *orig_deref = ir->as_dereference_array();
   ir_assignment *assign;
   ir_variable *index, *var;
   ir_dereference *deref;
   int i;

   if (!orig_deref)
      return ir;

   if (orig_deref->array->type->is_matrix() ||
       orig_deref->array->type->is_array())
      return ir;

   void *mem_ctx = ralloc_parent(ir);

   assert(orig_deref->array_index->type->base_type == GLSL_TYPE_INT);

   exec_list list;

   /* Store the index to a temporary to avoid reusing its tree. */
   index = new(base_ir) ir_variable(glsl_type::int_type,
				    "vec_index_tmp_i",
				    ir_var_temporary);
   list.push_tail(index);
   deref = new(base_ir) ir_dereference_variable(index);
   assign = new(base_ir) ir_assignment(deref, orig_deref->array_index, NULL);
   list.push_tail(assign);

   /* Temporary where we store whichever value we swizzle out. */
   var = new(base_ir) ir_variable(ir->type, "vec_index_tmp_v",
				  ir_var_temporary);
   list.push_tail(var);

   /* Generate a single comparison condition "mask" for all of the components
    * in the vector.
    */
   ir_rvalue *const cond_deref =
      compare_index_block(&list, index, 0,
			  orig_deref->array->type->vector_elements,
			  mem_ctx);

   /* Generate a conditional move of each vector element to the temp. */
   for (i = 0; i < orig_deref->array->type->vector_elements; i++) {
      ir_rvalue *condition_swizzle =
	 new(base_ir) ir_swizzle(cond_deref->clone(ir, NULL), i, 0, 0, 0, 1);

      /* Just clone the rest of the deref chain when trying to get at the
       * underlying variable.
       */
      ir_rvalue *swizzle =
	 new(base_ir) ir_swizzle(orig_deref->array->clone(mem_ctx, NULL),
				 i, 0, 0, 0, 1);

      deref = new(base_ir) ir_dereference_variable(var);
      assign = new(base_ir) ir_assignment(deref, swizzle, condition_swizzle);
      list.push_tail(assign);
   }

   /* Put all of the new instructions in the IR stream before the old
    * instruction.
    */
   base_ir->insert_before(&list);

   this->progress = true;
   return new(base_ir) ir_dereference_variable(var);
}

ir_visitor_status
ir_vec_index_to_cond_assign_visitor::visit_enter(ir_expression *ir)
{
   unsigned int i;

   for (i = 0; i < ir->get_num_operands(); i++) {
      ir->operands[i] = convert_vec_index_to_cond_assign(ir->operands[i]);
   }

   return visit_continue;
}

ir_visitor_status
ir_vec_index_to_cond_assign_visitor::visit_enter(ir_swizzle *ir)
{
   /* Can't be hit from normal GLSL, since you can't swizzle a scalar (which
    * the result of indexing a vector is.  But maybe at some point we'll end up
    * using swizzling of scalars for vector construction.
    */
   ir->val = convert_vec_index_to_cond_assign(ir->val);

   return visit_continue;
}

ir_visitor_status
ir_vec_index_to_cond_assign_visitor::visit_leave(ir_assignment *ir)
{
   ir_variable *index, *var;
   ir_dereference_variable *deref;
   ir_assignment *assign;
   int i;

   ir->rhs = convert_vec_index_to_cond_assign(ir->rhs);
   if (ir->condition)
      ir->condition = convert_vec_index_to_cond_assign(ir->condition);

   /* Last, handle the LHS */
   ir_dereference_array *orig_deref = ir->lhs->as_dereference_array();

   if (!orig_deref ||
       orig_deref->array->type->is_matrix() ||
       orig_deref->array->type->is_array())
      return visit_continue;

   void *mem_ctx = ralloc_parent(ir);

   assert(orig_deref->array_index->type->base_type == GLSL_TYPE_INT);

   exec_list list;

   /* Store the index to a temporary to avoid reusing its tree. */
   index = new(ir) ir_variable(glsl_type::int_type, "vec_index_tmp_i",
			       ir_var_temporary);
   list.push_tail(index);
   deref = new(ir) ir_dereference_variable(index);
   assign = new(ir) ir_assignment(deref, orig_deref->array_index, NULL);
   list.push_tail(assign);

   /* Store the RHS to a temporary to avoid reusing its tree. */
   var = new(ir) ir_variable(ir->rhs->type, "vec_index_tmp_v",
			     ir_var_temporary);
   list.push_tail(var);
   deref = new(ir) ir_dereference_variable(var);
   assign = new(ir) ir_assignment(deref, ir->rhs, NULL);
   list.push_tail(assign);

   /* Generate a single comparison condition "mask" for all of the components
    * in the vector.
    */
   ir_rvalue *const cond_deref =
      compare_index_block(&list, index, 0,
			  orig_deref->array->type->vector_elements,
			  mem_ctx);

   /* Generate a conditional move of each vector element to the temp. */
   for (i = 0; i < orig_deref->array->type->vector_elements; i++) {
      ir_rvalue *condition_swizzle =
	 new(ir) ir_swizzle(cond_deref->clone(ir, NULL), i, 0, 0, 0, 1);


      /* Just clone the rest of the deref chain when trying to get at the
       * underlying variable.
       */
      ir_rvalue *swizzle =
	 new(ir) ir_swizzle(orig_deref->array->clone(mem_ctx, NULL),
			    i, 0, 0, 0, 1);

      deref = new(ir) ir_dereference_variable(var);
      assign = new(ir) ir_assignment(swizzle, deref, condition_swizzle);
      list.push_tail(assign);
   }

   /* If the original assignment has a condition, respect that original
    * condition!  This is acomplished by wrapping the new conditional
    * assignments in an if-statement that uses the original condition.
    */
   if (ir->condition != NULL) {
      /* No need to clone the condition because the IR that it hangs on is
       * going to be removed from the instruction sequence.
       */
      ir_if *if_stmt = new(mem_ctx) ir_if(ir->condition);

      list.move_nodes_to(&if_stmt->then_instructions);
      ir->insert_before(if_stmt);
   } else {
      ir->insert_before(&list);
   }

   ir->remove();

   this->progress = true;

   return visit_continue;
}

ir_visitor_status
ir_vec_index_to_cond_assign_visitor::visit_enter(ir_call *ir)
{
   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_rvalue *param = (ir_rvalue *)iter.get();
      ir_rvalue *new_param = convert_vec_index_to_cond_assign(param);

      if (new_param != param) {
	 param->replace_with(new_param);
      }
   }

   return visit_continue;
}

ir_visitor_status
ir_vec_index_to_cond_assign_visitor::visit_enter(ir_return *ir)
{
   if (ir->value) {
      ir->value = convert_vec_index_to_cond_assign(ir->value);
   }

   return visit_continue;
}

ir_visitor_status
ir_vec_index_to_cond_assign_visitor::visit_enter(ir_if *ir)
{
   ir->condition = convert_vec_index_to_cond_assign(ir->condition);

   return visit_continue;
}

bool
do_vec_index_to_cond_assign(exec_list *instructions)
{
   ir_vec_index_to_cond_assign_visitor v;

   visit_list_elements(&v, instructions);

   return v.progress;
}
