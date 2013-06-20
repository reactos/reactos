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
 * \file opt_algebraic.cpp
 *
 * Takes advantage of association, commutivity, and other algebraic
 * properties to simplify expressions.
 */

#include "ir.h"
#include "ir_visitor.h"
#include "ir_rvalue_visitor.h"
#include "ir_optimization.h"
#include "glsl_types.h"

/**
 * Visitor class for replacing expressions with ir_constant values.
 */

class ir_algebraic_visitor : public ir_rvalue_visitor {
public:
   ir_algebraic_visitor()
   {
      this->progress = false;
      this->mem_ctx = NULL;
   }

   virtual ~ir_algebraic_visitor()
   {
   }

   ir_rvalue *handle_expression(ir_expression *ir);
   void handle_rvalue(ir_rvalue **rvalue);
   bool reassociate_constant(ir_expression *ir1,
			     int const_index,
			     ir_constant *constant,
			     ir_expression *ir2);
   void reassociate_operands(ir_expression *ir1,
			     int op1,
			     ir_expression *ir2,
			     int op2);
   ir_rvalue *swizzle_if_required(ir_expression *expr,
				  ir_rvalue *operand);

   void *mem_ctx;

   bool progress;
};

static inline bool
is_vec_zero(ir_constant *ir)
{
   return (ir == NULL) ? false : ir->is_zero();
}

static inline bool
is_vec_one(ir_constant *ir)
{
   return (ir == NULL) ? false : ir->is_one();
}

static void
update_type(ir_expression *ir)
{
   if (ir->operands[0]->type->is_vector())
      ir->type = ir->operands[0]->type;
   else
      ir->type = ir->operands[1]->type;
}

void
ir_algebraic_visitor::reassociate_operands(ir_expression *ir1,
					   int op1,
					   ir_expression *ir2,
					   int op2)
{
   ir_rvalue *temp = ir2->operands[op2];
   ir2->operands[op2] = ir1->operands[op1];
   ir1->operands[op1] = temp;

   /* Update the type of ir2.  The type of ir1 won't have changed --
    * base types matched, and at least one of the operands of the 2
    * binops is still a vector if any of them were.
    */
   update_type(ir2);

   this->progress = true;
}

/**
 * Reassociates a constant down a tree of adds or multiplies.
 *
 * Consider (2 * (a * (b * 0.5))).  We want to send up with a * b.
 */
bool
ir_algebraic_visitor::reassociate_constant(ir_expression *ir1, int const_index,
					   ir_constant *constant,
					   ir_expression *ir2)
{
   if (!ir2 || ir1->operation != ir2->operation)
      return false;

   /* Don't want to even think about matrices. */
   if (ir1->operands[0]->type->is_matrix() ||
       ir1->operands[1]->type->is_matrix() ||
       ir2->operands[0]->type->is_matrix() ||
       ir2->operands[1]->type->is_matrix())
      return false;

   ir_constant *ir2_const[2];
   ir2_const[0] = ir2->operands[0]->constant_expression_value();
   ir2_const[1] = ir2->operands[1]->constant_expression_value();

   if (ir2_const[0] && ir2_const[1])
      return false;

   if (ir2_const[0]) {
      reassociate_operands(ir1, const_index, ir2, 1);
      return true;
   } else if (ir2_const[1]) {
      reassociate_operands(ir1, const_index, ir2, 0);
      return true;
   }

   if (reassociate_constant(ir1, const_index, constant,
			    ir2->operands[0]->as_expression())) {
      update_type(ir2);
      return true;
   }

   if (reassociate_constant(ir1, const_index, constant,
			    ir2->operands[1]->as_expression())) {
      update_type(ir2);
      return true;
   }

   return false;
}

/* When eliminating an expression and just returning one of its operands,
 * we may need to swizzle that operand out to a vector if the expression was
 * vector type.
 */
ir_rvalue *
ir_algebraic_visitor::swizzle_if_required(ir_expression *expr,
					  ir_rvalue *operand)
{
   if (expr->type->is_vector() && operand->type->is_scalar()) {
      return new(mem_ctx) ir_swizzle(operand, 0, 0, 0, 0,
				     expr->type->vector_elements);
   } else
      return operand;
}

ir_rvalue *
ir_algebraic_visitor::handle_expression(ir_expression *ir)
{
   ir_constant *op_const[2] = {NULL, NULL};
   ir_expression *op_expr[2] = {NULL, NULL};
   ir_expression *temp;
   unsigned int i;

   assert(ir->get_num_operands() <= 2);
   for (i = 0; i < ir->get_num_operands(); i++) {
      if (ir->operands[i]->type->is_matrix())
	 return ir;

      op_const[i] = ir->operands[i]->constant_expression_value();
      op_expr[i] = ir->operands[i]->as_expression();
   }

   if (this->mem_ctx == NULL)
      this->mem_ctx = ralloc_parent(ir);

   switch (ir->operation) {
   case ir_unop_logic_not: {
      enum ir_expression_operation new_op = ir_unop_logic_not;

      if (op_expr[0] == NULL)
	 break;

      switch (op_expr[0]->operation) {
      case ir_binop_less:    new_op = ir_binop_gequal;  break;
      case ir_binop_greater: new_op = ir_binop_lequal;  break;
      case ir_binop_lequal:  new_op = ir_binop_greater; break;
      case ir_binop_gequal:  new_op = ir_binop_less;    break;
      case ir_binop_equal:   new_op = ir_binop_nequal;  break;
      case ir_binop_nequal:  new_op = ir_binop_equal;   break;
      case ir_binop_all_equal:   new_op = ir_binop_any_nequal;  break;
      case ir_binop_any_nequal:  new_op = ir_binop_all_equal;   break;

      default:
	 /* The default case handler is here to silence a warning from GCC.
	  */
	 break;
      }

      if (new_op != ir_unop_logic_not) {
	 this->progress = true;
	 return new(mem_ctx) ir_expression(new_op,
					   ir->type,
					   op_expr[0]->operands[0],
					   op_expr[0]->operands[1]);
      }

      break;
   }

   case ir_binop_add:
      if (is_vec_zero(op_const[0])) {
	 this->progress = true;
	 return swizzle_if_required(ir, ir->operands[1]);
      }
      if (is_vec_zero(op_const[1])) {
	 this->progress = true;
	 return swizzle_if_required(ir, ir->operands[0]);
      }

      /* Reassociate addition of constants so that we can do constant
       * folding.
       */
      if (op_const[0] && !op_const[1])
	 reassociate_constant(ir, 0, op_const[0],
			      ir->operands[1]->as_expression());
      if (op_const[1] && !op_const[0])
	 reassociate_constant(ir, 1, op_const[1],
			      ir->operands[0]->as_expression());
      break;

   case ir_binop_sub:
      if (is_vec_zero(op_const[0])) {
	 this->progress = true;
	 temp = new(mem_ctx) ir_expression(ir_unop_neg,
					   ir->operands[1]->type,
					   ir->operands[1],
					   NULL);
	 return swizzle_if_required(ir, temp);
      }
      if (is_vec_zero(op_const[1])) {
	 this->progress = true;
	 return swizzle_if_required(ir, ir->operands[0]);
      }
      break;

   case ir_binop_mul:
      if (is_vec_one(op_const[0])) {
	 this->progress = true;
	 return swizzle_if_required(ir, ir->operands[1]);
      }
      if (is_vec_one(op_const[1])) {
	 this->progress = true;
	 return swizzle_if_required(ir, ir->operands[0]);
      }

      if (is_vec_zero(op_const[0]) || is_vec_zero(op_const[1])) {
	 this->progress = true;
	 return ir_constant::zero(ir, ir->type);
      }

      /* Reassociate multiplication of constants so that we can do
       * constant folding.
       */
      if (op_const[0] && !op_const[1])
	 reassociate_constant(ir, 0, op_const[0],
			      ir->operands[1]->as_expression());
      if (op_const[1] && !op_const[0])
	 reassociate_constant(ir, 1, op_const[1],
			      ir->operands[0]->as_expression());

      break;

   case ir_binop_div:
      if (is_vec_one(op_const[0]) && ir->type->base_type == GLSL_TYPE_FLOAT) {
	 this->progress = true;
	 temp = new(mem_ctx) ir_expression(ir_unop_rcp,
					   ir->operands[1]->type,
					   ir->operands[1],
					   NULL);
	 return swizzle_if_required(ir, temp);
      }
      if (is_vec_one(op_const[1])) {
	 this->progress = true;
	 return swizzle_if_required(ir, ir->operands[0]);
      }
      break;

   case ir_binop_logic_and:
      /* FINISHME: Also simplify (a && a) to (a). */
      if (is_vec_one(op_const[0])) {
	 this->progress = true;
	 return ir->operands[1];
      } else if (is_vec_one(op_const[1])) {
	 this->progress = true;
	 return ir->operands[0];
      } else if (is_vec_zero(op_const[0]) || is_vec_zero(op_const[1])) {
	 this->progress = true;
	 return ir_constant::zero(mem_ctx, ir->type);
      }
      break;

   case ir_binop_logic_xor:
      /* FINISHME: Also simplify (a ^^ a) to (false). */
      if (is_vec_zero(op_const[0])) {
	 this->progress = true;
	 return ir->operands[1];
      } else if (is_vec_zero(op_const[1])) {
	 this->progress = true;
	 return ir->operands[0];
      } else if (is_vec_one(op_const[0])) {
	 this->progress = true;
	 return new(mem_ctx) ir_expression(ir_unop_logic_not, ir->type,
					   ir->operands[1], NULL);
      } else if (is_vec_one(op_const[1])) {
	 this->progress = true;
	 return new(mem_ctx) ir_expression(ir_unop_logic_not, ir->type,
					   ir->operands[0], NULL);
      }
      break;

   case ir_binop_logic_or:
      /* FINISHME: Also simplify (a || a) to (a). */
      if (is_vec_zero(op_const[0])) {
	 this->progress = true;
	 return ir->operands[1];
      } else if (is_vec_zero(op_const[1])) {
	 this->progress = true;
	 return ir->operands[0];
      } else if (is_vec_one(op_const[0]) || is_vec_one(op_const[1])) {
	 ir_constant_data data;

	 for (unsigned i = 0; i < 16; i++)
	    data.b[i] = true;

	 this->progress = true;
	 return new(mem_ctx) ir_constant(ir->type, &data);
      }
      break;

   case ir_unop_rcp:
      if (op_expr[0] && op_expr[0]->operation == ir_unop_rcp) {
	 this->progress = true;
	 return op_expr[0]->operands[0];
      }

      /* FINISHME: We should do rcp(rsq(x)) -> sqrt(x) for some
       * backends, except that some backends will have done sqrt ->
       * rcp(rsq(x)) and we don't want to undo it for them.
       */

      /* As far as we know, all backends are OK with rsq. */
      if (op_expr[0] && op_expr[0]->operation == ir_unop_sqrt) {
	 this->progress = true;
	 temp = new(mem_ctx) ir_expression(ir_unop_rsq,
					   op_expr[0]->operands[0]->type,
					   op_expr[0]->operands[0],
					   NULL);
	 return swizzle_if_required(ir, temp);
      }

      break;

   default:
      break;
   }

   return ir;
}

void
ir_algebraic_visitor::handle_rvalue(ir_rvalue **rvalue)
{
   if (!*rvalue)
      return;

   ir_expression *expr = (*rvalue)->as_expression();
   if (!expr || expr->operation == ir_quadop_vector)
      return;

   *rvalue = handle_expression(expr);
}

bool
do_algebraic(exec_list *instructions)
{
   ir_algebraic_visitor v;

   visit_list_elements(&v, instructions);

   return v.progress;
}
