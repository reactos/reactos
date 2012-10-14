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
 * \file opt_constant_folding.cpp
 * Replace constant-valued expressions with references to constant values.
 */

#include "ir.h"
#include "ir_visitor.h"
#include "ir_rvalue_visitor.h"
#include "ir_optimization.h"
#include "glsl_types.h"

/**
 * Visitor class for replacing expressions with ir_constant values.
 */

class ir_constant_folding_visitor : public ir_rvalue_visitor {
public:
   ir_constant_folding_visitor()
   {
      this->progress = false;
   }

   virtual ~ir_constant_folding_visitor()
   {
      /* empty */
   }

   virtual ir_visitor_status visit_enter(ir_assignment *ir);
   virtual ir_visitor_status visit_enter(ir_call *ir);

   virtual void handle_rvalue(ir_rvalue **rvalue);

   bool progress;
};

void
ir_constant_folding_visitor::handle_rvalue(ir_rvalue **rvalue)
{
   if (*rvalue == NULL || (*rvalue)->ir_type == ir_type_constant)
      return;

   /* Note that we do rvalue visitoring on leaving.  So if an
    * expression has a non-constant operand, no need to go looking
    * down it to find if it's constant.  This cuts the time of this
    * pass down drastically.
    */
   ir_expression *expr = (*rvalue)->as_expression();
   if (expr) {
      for (unsigned int i = 0; i < expr->get_num_operands(); i++) {
	 if (!expr->operands[i]->as_constant())
	    return;
      }
   }

   ir_constant *constant = (*rvalue)->constant_expression_value();
   if (constant) {
      *rvalue = constant;
      this->progress = true;
   } else {
      (*rvalue)->accept(this);
   }
}

ir_visitor_status
ir_constant_folding_visitor::visit_enter(ir_assignment *ir)
{
   ir->rhs->accept(this);
   handle_rvalue(&ir->rhs);

   if (ir->condition) {
      ir->condition->accept(this);
      handle_rvalue(&ir->condition);

      ir_constant *const_val = ir->condition->as_constant();
      /* If the condition is constant, either remove the condition or
       * remove the never-executed assignment.
       */
      if (const_val) {
	 if (const_val->value.b[0])
	    ir->condition = NULL;
	 else
	    ir->remove();
	 this->progress = true;
      }
   }

   /* Don't descend into the LHS because we want it to stay as a
    * variable dereference.  FINISHME: We probably should to get array
    * indices though.
    */
   return visit_continue_with_parent;
}

ir_visitor_status
ir_constant_folding_visitor::visit_enter(ir_call *ir)
{
   exec_list_iterator sig_iter = ir->get_callee()->parameters.iterator();
   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_rvalue *param_rval = (ir_rvalue *)iter.get();
      ir_variable *sig_param = (ir_variable *)sig_iter.get();

      if (sig_param->mode == ir_var_in || sig_param->mode == ir_var_const_in) {
	 ir_rvalue *new_param = param_rval;

	 handle_rvalue(&new_param);
	 if (new_param != param_rval) {
	    param_rval->replace_with(new_param);
	 }
      }
      sig_iter.next();
   }

   return visit_continue_with_parent;
}

bool
do_constant_folding(exec_list *instructions)
{
   ir_constant_folding_visitor constant_folding;

   visit_list_elements(&constant_folding, instructions);

   return constant_folding.progress;
}
