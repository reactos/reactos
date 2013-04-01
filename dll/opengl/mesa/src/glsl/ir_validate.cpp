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
 * \file ir_validate.cpp
 *
 * Attempts to verify that various invariants of the IR tree are true.
 *
 * In particular, at the moment it makes sure that no single
 * ir_instruction node except for ir_variable appears multiple times
 * in the ir tree.  ir_variable does appear multiple times: Once as a
 * declaration in an exec_list, and multiple times as the endpoint of
 * a dereference chain.
 */

#include "ir.h"
#include "ir_hierarchical_visitor.h"
#include "program/hash_table.h"
#include "glsl_types.h"

class ir_validate : public ir_hierarchical_visitor {
public:
   ir_validate()
   {
      this->ht = hash_table_ctor(0, hash_table_pointer_hash,
				 hash_table_pointer_compare);

      this->current_function = NULL;

      this->callback = ir_validate::validate_ir;
      this->data = ht;
   }

   ~ir_validate()
   {
      hash_table_dtor(this->ht);
   }

   virtual ir_visitor_status visit(ir_variable *v);
   virtual ir_visitor_status visit(ir_dereference_variable *ir);

   virtual ir_visitor_status visit_enter(ir_if *ir);

   virtual ir_visitor_status visit_leave(ir_loop *ir);
   virtual ir_visitor_status visit_enter(ir_function *ir);
   virtual ir_visitor_status visit_leave(ir_function *ir);
   virtual ir_visitor_status visit_enter(ir_function_signature *ir);

   virtual ir_visitor_status visit_leave(ir_expression *ir);
   virtual ir_visitor_status visit_leave(ir_swizzle *ir);

   virtual ir_visitor_status visit_enter(ir_assignment *ir);
   virtual ir_visitor_status visit_enter(ir_call *ir);

   static void validate_ir(ir_instruction *ir, void *data);

   ir_function *current_function;

   struct hash_table *ht;
};


ir_visitor_status
ir_validate::visit(ir_dereference_variable *ir)
{
   if ((ir->var == NULL) || (ir->var->as_variable() == NULL)) {
      printf("ir_dereference_variable @ %p does not specify a variable %p\n",
	     (void *) ir, (void *) ir->var);
      abort();
   }

   if (hash_table_find(ht, ir->var) == NULL) {
      printf("ir_dereference_variable @ %p specifies undeclared variable "
	     "`%s' @ %p\n",
	     (void *) ir, ir->var->name, (void *) ir->var);
      abort();
   }

   this->validate_ir(ir, this->data);

   return visit_continue;
}

ir_visitor_status
ir_validate::visit_enter(ir_if *ir)
{
   if (ir->condition->type != glsl_type::bool_type) {
      printf("ir_if condition %s type instead of bool.\n",
	     ir->condition->type->name);
      ir->print();
      printf("\n");
      abort();
   }

   return visit_continue;
}


ir_visitor_status
ir_validate::visit_leave(ir_loop *ir)
{
   if (ir->counter != NULL) {
      if ((ir->from == NULL) || (ir->from == NULL) || (ir->increment == NULL)) {
	 printf("ir_loop has invalid loop controls:\n"
		"    counter:   %p\n"
		"    from:      %p\n"
		"    to:        %p\n"
		"    increment: %p\n",
		(void *) ir->counter, (void *) ir->from, (void *) ir->to,
                (void *) ir->increment);
	 abort();
      }

      if ((ir->cmp < ir_binop_less) || (ir->cmp > ir_binop_nequal)) {
	 printf("ir_loop has invalid comparitor %d\n", ir->cmp);
	 abort();
      }
   } else {
      if ((ir->from != NULL) || (ir->from != NULL) || (ir->increment != NULL)) {
	 printf("ir_loop has invalid loop controls:\n"
		"    counter:   %p\n"
		"    from:      %p\n"
		"    to:        %p\n"
		"    increment: %p\n",
		(void *) ir->counter, (void *) ir->from, (void *) ir->to,
                (void *) ir->increment);
	 abort();
      }
   }

   return visit_continue;
}


ir_visitor_status
ir_validate::visit_enter(ir_function *ir)
{
   /* Function definitions cannot be nested.
    */
   if (this->current_function != NULL) {
      printf("Function definition nested inside another function "
	     "definition:\n");
      printf("%s %p inside %s %p\n",
	     ir->name, (void *) ir,
	     this->current_function->name, (void *) this->current_function);
      abort();
   }

   /* Store the current function hierarchy being traversed.  This is used
    * by the function signature visitor to ensure that the signatures are
    * linked with the correct functions.
    */
   this->current_function = ir;

   this->validate_ir(ir, this->data);

   /* Verify that all of the things stored in the list of signatures are,
    * in fact, function signatures.
    */
   foreach_list(node, &ir->signatures) {
      ir_instruction *sig = (ir_instruction *) node;

      if (sig->ir_type != ir_type_function_signature) {
	 printf("Non-signature in signature list of function `%s'\n",
		ir->name);
	 abort();
      }
   }

   return visit_continue;
}

ir_visitor_status
ir_validate::visit_leave(ir_function *ir)
{
   assert(ralloc_parent(ir->name) == ir);

   this->current_function = NULL;
   return visit_continue;
}

ir_visitor_status
ir_validate::visit_enter(ir_function_signature *ir)
{
   if (this->current_function != ir->function()) {
      printf("Function signature nested inside wrong function "
	     "definition:\n");
      printf("%p inside %s %p instead of %s %p\n",
	     (void *) ir,
	     this->current_function->name, (void *) this->current_function,
	     ir->function_name(), (void *) ir->function());
      abort();
   }

   if (ir->return_type == NULL) {
      printf("Function signature %p for function %s has NULL return type.\n",
	     (void *) ir, ir->function_name());
      abort();
   }

   this->validate_ir(ir, this->data);

   return visit_continue;
}

ir_visitor_status
ir_validate::visit_leave(ir_expression *ir)
{
   switch (ir->operation) {
   case ir_unop_bit_not:
      assert(ir->operands[0]->type == ir->type);
      break;
   case ir_unop_logic_not:
      assert(ir->type->base_type == GLSL_TYPE_BOOL);
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_BOOL);
      break;

   case ir_unop_neg:
   case ir_unop_abs:
   case ir_unop_sign:
   case ir_unop_rcp:
   case ir_unop_rsq:
   case ir_unop_sqrt:
      assert(ir->type == ir->operands[0]->type);
      break;

   case ir_unop_exp:
   case ir_unop_log:
   case ir_unop_exp2:
   case ir_unop_log2:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_FLOAT);
      assert(ir->type == ir->operands[0]->type);
      break;

   case ir_unop_f2i:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_FLOAT);
      assert(ir->type->base_type == GLSL_TYPE_INT);
      break;
   case ir_unop_i2f:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_INT);
      assert(ir->type->base_type == GLSL_TYPE_FLOAT);
      break;
   case ir_unop_f2b:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_FLOAT);
      assert(ir->type->base_type == GLSL_TYPE_BOOL);
      break;
   case ir_unop_b2f:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_BOOL);
      assert(ir->type->base_type == GLSL_TYPE_FLOAT);
      break;
   case ir_unop_i2b:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_INT);
      assert(ir->type->base_type == GLSL_TYPE_BOOL);
      break;
   case ir_unop_b2i:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_BOOL);
      assert(ir->type->base_type == GLSL_TYPE_INT);
      break;
   case ir_unop_u2f:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_UINT);
      assert(ir->type->base_type == GLSL_TYPE_FLOAT);
      break;
   case ir_unop_i2u:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_INT);
      assert(ir->type->base_type == GLSL_TYPE_UINT);
      break;
   case ir_unop_u2i:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_UINT);
      assert(ir->type->base_type == GLSL_TYPE_INT);
      break;

   case ir_unop_any:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_BOOL);
      assert(ir->type == glsl_type::bool_type);
      break;

   case ir_unop_trunc:
   case ir_unop_round_even:
   case ir_unop_ceil:
   case ir_unop_floor:
   case ir_unop_fract:
   case ir_unop_sin:
   case ir_unop_cos:
   case ir_unop_sin_reduced:
   case ir_unop_cos_reduced:
   case ir_unop_dFdx:
   case ir_unop_dFdy:
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_FLOAT);
      assert(ir->operands[0]->type == ir->type);
      break;

   case ir_unop_noise:
      /* XXX what can we assert here? */
      break;

   case ir_binop_add:
   case ir_binop_sub:
   case ir_binop_mul:
   case ir_binop_div:
   case ir_binop_mod:
   case ir_binop_min:
   case ir_binop_max:
   case ir_binop_pow:
      if (ir->operands[0]->type->is_scalar())
	 assert(ir->operands[1]->type == ir->type);
      else if (ir->operands[1]->type->is_scalar())
	 assert(ir->operands[0]->type == ir->type);
      else if (ir->operands[0]->type->is_vector() &&
	       ir->operands[1]->type->is_vector()) {
	 assert(ir->operands[0]->type == ir->operands[1]->type);
	 assert(ir->operands[0]->type == ir->type);
      }
      break;

   case ir_binop_less:
   case ir_binop_greater:
   case ir_binop_lequal:
   case ir_binop_gequal:
   case ir_binop_equal:
   case ir_binop_nequal:
      /* The semantics of the IR operators differ from the GLSL <, >, <=, >=,
       * ==, and != operators.  The IR operators perform a component-wise
       * comparison on scalar or vector types and return a boolean scalar or
       * vector type of the same size.
       */
      assert(ir->type->base_type == GLSL_TYPE_BOOL);
      assert(ir->operands[0]->type == ir->operands[1]->type);
      assert(ir->operands[0]->type->is_vector()
	     || ir->operands[0]->type->is_scalar());
      assert(ir->operands[0]->type->vector_elements
	     == ir->type->vector_elements);
      break;

   case ir_binop_all_equal:
   case ir_binop_any_nequal:
      /* GLSL == and != operate on scalars, vectors, matrices and arrays, and
       * return a scalar boolean.  The IR matches that.
       */
      assert(ir->type == glsl_type::bool_type);
      assert(ir->operands[0]->type == ir->operands[1]->type);
      break;

   case ir_binop_lshift:
   case ir_binop_rshift:
      assert(ir->operands[0]->type->is_integer() &&
             ir->operands[1]->type->is_integer());
      if (ir->operands[0]->type->is_scalar()) {
          assert(ir->operands[1]->type->is_scalar());
      }
      if (ir->operands[0]->type->is_vector() &&
          ir->operands[1]->type->is_vector()) {
          assert(ir->operands[0]->type->components() ==
                 ir->operands[1]->type->components());
      }
      assert(ir->type == ir->operands[0]->type);
      break;

   case ir_binop_bit_and:
   case ir_binop_bit_xor:
   case ir_binop_bit_or:
       assert(ir->operands[0]->type->base_type ==
              ir->operands[1]->type->base_type);
       assert(ir->type->is_integer());
       if (ir->operands[0]->type->is_vector() &&
           ir->operands[1]->type->is_vector()) {
           assert(ir->operands[0]->type->vector_elements ==
                  ir->operands[1]->type->vector_elements);
       }
       break;

   case ir_binop_logic_and:
   case ir_binop_logic_xor:
   case ir_binop_logic_or:
      assert(ir->type == glsl_type::bool_type);
      assert(ir->operands[0]->type == glsl_type::bool_type);
      assert(ir->operands[1]->type == glsl_type::bool_type);
      break;

   case ir_binop_dot:
      assert(ir->type == glsl_type::float_type);
      assert(ir->operands[0]->type->base_type == GLSL_TYPE_FLOAT);
      assert(ir->operands[0]->type->is_vector());
      assert(ir->operands[0]->type == ir->operands[1]->type);
      break;

   case ir_quadop_vector:
      /* The vector operator collects some number of scalars and generates a
       * vector from them.
       *
       *  - All of the operands must be scalar.
       *  - Number of operands must matche the size of the resulting vector.
       *  - Base type of the operands must match the base type of the result.
       */
      assert(ir->type->is_vector());
      switch (ir->type->vector_elements) {
      case 2:
	 assert(ir->operands[0]->type->is_scalar());
	 assert(ir->operands[0]->type->base_type == ir->type->base_type);
	 assert(ir->operands[1]->type->is_scalar());
	 assert(ir->operands[1]->type->base_type == ir->type->base_type);
	 assert(ir->operands[2] == NULL);
	 assert(ir->operands[3] == NULL);
	 break;
      case 3:
	 assert(ir->operands[0]->type->is_scalar());
	 assert(ir->operands[0]->type->base_type == ir->type->base_type);
	 assert(ir->operands[1]->type->is_scalar());
	 assert(ir->operands[1]->type->base_type == ir->type->base_type);
	 assert(ir->operands[2]->type->is_scalar());
	 assert(ir->operands[2]->type->base_type == ir->type->base_type);
	 assert(ir->operands[3] == NULL);
	 break;
      case 4:
	 assert(ir->operands[0]->type->is_scalar());
	 assert(ir->operands[0]->type->base_type == ir->type->base_type);
	 assert(ir->operands[1]->type->is_scalar());
	 assert(ir->operands[1]->type->base_type == ir->type->base_type);
	 assert(ir->operands[2]->type->is_scalar());
	 assert(ir->operands[2]->type->base_type == ir->type->base_type);
	 assert(ir->operands[3]->type->is_scalar());
	 assert(ir->operands[3]->type->base_type == ir->type->base_type);
	 break;
      default:
	 /* The is_vector assertion above should prevent execution from ever
	  * getting here.
	  */
	 assert(!"Should not get here.");
	 break;
      }
   }

   return visit_continue;
}

ir_visitor_status
ir_validate::visit_leave(ir_swizzle *ir)
{
   int chans[4] = {ir->mask.x, ir->mask.y, ir->mask.z, ir->mask.w};

   for (unsigned int i = 0; i < ir->type->vector_elements; i++) {
      if (chans[i] >= ir->val->type->vector_elements) {
	 printf("ir_swizzle @ %p specifies a channel not present "
		"in the value.\n", (void *) ir);
	 ir->print();
	 abort();
      }
   }

   return visit_continue;
}

ir_visitor_status
ir_validate::visit(ir_variable *ir)
{
   /* An ir_variable is the one thing that can (and will) appear multiple times
    * in an IR tree.  It is added to the hashtable so that it can be used
    * in the ir_dereference_variable handler to ensure that a variable is
    * declared before it is dereferenced.
    */
   if (ir->name)
      assert(ralloc_parent(ir->name) == ir);

   hash_table_insert(ht, ir, ir);


   /* If a variable is an array, verify that the maximum array index is in
    * bounds.  There was once an error in AST-to-HIR conversion that set this
    * to be out of bounds.
    */
   if (ir->type->array_size() > 0) {
      if (ir->max_array_access >= ir->type->length) {
	 printf("ir_variable has maximum access out of bounds (%d vs %d)\n",
		ir->max_array_access, ir->type->length - 1);
	 ir->print();
	 abort();
      }
   }

   if (ir->constant_initializer != NULL && !ir->has_initializer) {
      printf("ir_variable didn't have an initializer, but has a constant "
	     "initializer value.\n");
      ir->print();
      abort();
   }

   return visit_continue;
}

ir_visitor_status
ir_validate::visit_enter(ir_assignment *ir)
{
   const ir_dereference *const lhs = ir->lhs;
   if (lhs->type->is_scalar() || lhs->type->is_vector()) {
      if (ir->write_mask == 0) {
	 printf("Assignment LHS is %s, but write mask is 0:\n",
		lhs->type->is_scalar() ? "scalar" : "vector");
	 ir->print();
	 abort();
      }

      int lhs_components = 0;
      for (int i = 0; i < 4; i++) {
	 if (ir->write_mask & (1 << i))
	    lhs_components++;
      }

      if (lhs_components != ir->rhs->type->vector_elements) {
	 printf("Assignment count of LHS write mask channels enabled not\n"
		"matching RHS vector size (%d LHS, %d RHS).\n",
		lhs_components, ir->rhs->type->vector_elements);
	 ir->print();
	 abort();
      }
   }

   this->validate_ir(ir, this->data);

   return visit_continue;
}

ir_visitor_status
ir_validate::visit_enter(ir_call *ir)
{
   ir_function_signature *const callee = ir->get_callee();

   if (callee->ir_type != ir_type_function_signature) {
      printf("IR called by ir_call is not ir_function_signature!\n");
      abort();
   }

   const exec_node *formal_param_node = callee->parameters.head;
   const exec_node *actual_param_node = ir->actual_parameters.head;
   while (true) {
      if (formal_param_node->is_tail_sentinel()
          != actual_param_node->is_tail_sentinel()) {
         printf("ir_call has the wrong number of parameters:\n");
         goto dump_ir;
      }
      if (formal_param_node->is_tail_sentinel()) {
         break;
      }
      const ir_variable *formal_param
         = (const ir_variable *) formal_param_node;
      const ir_rvalue *actual_param
         = (const ir_rvalue *) actual_param_node;
      if (formal_param->type != actual_param->type) {
         printf("ir_call parameter type mismatch:\n");
         goto dump_ir;
      }
      if (formal_param->mode == ir_var_out
          || formal_param->mode == ir_var_inout) {
         if (!actual_param->is_lvalue()) {
            printf("ir_call out/inout parameters must be lvalues:\n");
            goto dump_ir;
         }
      }
      formal_param_node = formal_param_node->next;
      actual_param_node = actual_param_node->next;
   }

   return visit_continue;

dump_ir:
   ir->print();
   printf("callee:\n");
   callee->print();
   abort();
   return visit_stop;
}

void
ir_validate::validate_ir(ir_instruction *ir, void *data)
{
   struct hash_table *ht = (struct hash_table *) data;

   if (hash_table_find(ht, ir)) {
      printf("Instruction node present twice in ir tree:\n");
      ir->print();
      printf("\n");
      abort();
   }
   hash_table_insert(ht, ir, ir);
}

void
check_node_type(ir_instruction *ir, void *data)
{
   (void) data;

   if (ir->ir_type <= ir_type_unset || ir->ir_type >= ir_type_max) {
      printf("Instruction node with unset type\n");
      ir->print(); printf("\n");
   }
   assert(ir->type != glsl_type::error_type);
}

void
validate_ir_tree(exec_list *instructions)
{
   ir_validate v;

   v.run(instructions);

   foreach_iter(exec_list_iterator, iter, *instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();

      visit_tree(ir, check_node_type, NULL);
   }
}
