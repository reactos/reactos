/*
 * Copyright © 2011 Intel Corporation
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
 * \file lower_clip_distance.cpp
 *
 * This pass accounts for the difference between the way
 * gl_ClipDistance is declared in standard GLSL (as an array of
 * floats), and the way it is frequently implemented in hardware (as
 * a pair of vec4s, with four clip distances packed into each).
 *
 * The declaration of gl_ClipDistance is replaced with a declaration
 * of gl_ClipDistanceMESA, and any references to gl_ClipDistance are
 * translated to refer to gl_ClipDistanceMESA with the appropriate
 * swizzling of array indices.  For instance:
 *
 *   gl_ClipDistance[i]
 *
 * is translated into:
 *
 *   gl_ClipDistanceMESA[i>>2][i&3]
 *
 * Since some hardware may not internally represent gl_ClipDistance as a pair
 * of vec4's, this lowering pass is optional.  To enable it, set the
 * LowerClipDistance flag in gl_shader_compiler_options to true.
 */

#include "ir_hierarchical_visitor.h"
#include "ir.h"

class lower_clip_distance_visitor : public ir_hierarchical_visitor {
public:
   lower_clip_distance_visitor()
      : progress(false), old_clip_distance_var(NULL),
        new_clip_distance_var(NULL)
   {
   }

   virtual ir_visitor_status visit(ir_variable *);
   void create_indices(ir_rvalue*, ir_rvalue *&, ir_rvalue *&);
   virtual ir_visitor_status visit_leave(ir_dereference_array *);
   virtual ir_visitor_status visit_leave(ir_assignment *);
   void visit_new_assignment(ir_assignment *ir);
   virtual ir_visitor_status visit_leave(ir_call *);

   bool progress;

   /**
    * Pointer to the declaration of gl_ClipDistance, if found.
    */
   ir_variable *old_clip_distance_var;

   /**
    * Pointer to the newly-created gl_ClipDistanceMESA variable.
    */
   ir_variable *new_clip_distance_var;
};


/**
 * Replace any declaration of gl_ClipDistance as an array of floats with a
 * declaration of gl_ClipDistanceMESA as an array of vec4's.
 */
ir_visitor_status
lower_clip_distance_visitor::visit(ir_variable *ir)
{
   /* No point in looking for the declaration of gl_ClipDistance if
    * we've already found it.
    */
   if (this->old_clip_distance_var)
      return visit_continue;

   if (ir->name && strcmp(ir->name, "gl_ClipDistance") == 0) {
      this->progress = true;
      this->old_clip_distance_var = ir;
      assert (ir->type->is_array());
      assert (ir->type->element_type() == glsl_type::float_type);
      unsigned new_size = (ir->type->array_size() + 3) / 4;

      /* Clone the old var so that we inherit all of its properties */
      this->new_clip_distance_var = ir->clone(ralloc_parent(ir), NULL);

      /* And change the properties that we need to change */
      this->new_clip_distance_var->name
         = ralloc_strdup(this->new_clip_distance_var, "gl_ClipDistanceMESA");
      this->new_clip_distance_var->type
         = glsl_type::get_array_instance(glsl_type::vec4_type, new_size);
      this->new_clip_distance_var->max_array_access = ir->max_array_access / 4;

      ir->replace_with(this->new_clip_distance_var);
   }
   return visit_continue;
}


/**
 * Create the necessary GLSL rvalues to index into gl_ClipDistanceMESA based
 * on the rvalue previously used to index into gl_ClipDistance.
 *
 * \param array_index Selects one of the vec4's in gl_ClipDistanceMESA
 * \param swizzle_index Selects a component within the vec4 selected by
 *        array_index.
 */
void
lower_clip_distance_visitor::create_indices(ir_rvalue *old_index,
                                            ir_rvalue *&array_index,
                                            ir_rvalue *&swizzle_index)
{
   void *ctx = ralloc_parent(old_index);

   /* Make sure old_index is a signed int so that the bitwise "shift" and
    * "and" operations below type check properly.
    */
   if (old_index->type != glsl_type::int_type) {
      assert (old_index->type == glsl_type::uint_type);
      old_index = new(ctx) ir_expression(ir_unop_u2i, old_index);
   }

   ir_constant *old_index_constant = old_index->constant_expression_value();
   if (old_index_constant) {
      /* gl_ClipDistance is being accessed via a constant index.  Don't bother
       * creating expressions to calculate the lowered indices.  Just create
       * constants.
       */
      int const_val = old_index_constant->get_int_component(0);
      array_index = new(ctx) ir_constant(const_val / 4);
      swizzle_index = new(ctx) ir_constant(const_val % 4);
   } else {
      /* Create a variable to hold the value of old_index (so that we
       * don't compute it twice).
       */
      ir_variable *old_index_var = new(ctx) ir_variable(
         glsl_type::int_type, "clip_distance_index", ir_var_temporary);
      this->base_ir->insert_before(old_index_var);
      this->base_ir->insert_before(new(ctx) ir_assignment(
         new(ctx) ir_dereference_variable(old_index_var), old_index));

      /* Create the expression clip_distance_index / 4.  Do this as a bit
       * shift because that's likely to be more efficient.
       */
      array_index = new(ctx) ir_expression(
         ir_binop_rshift, new(ctx) ir_dereference_variable(old_index_var),
         new(ctx) ir_constant(2));

      /* Create the expression clip_distance_index % 4.  Do this as a bitwise
       * AND because that's likely to be more efficient.
       */
      swizzle_index = new(ctx) ir_expression(
         ir_binop_bit_and, new(ctx) ir_dereference_variable(old_index_var),
         new(ctx) ir_constant(3));
   }
}


/**
 * Replace any expression that indexes into the gl_ClipDistance array with an
 * expression that indexes into one of the vec4's in gl_ClipDistanceMESA and
 * accesses the appropriate component.
 */
ir_visitor_status
lower_clip_distance_visitor::visit_leave(ir_dereference_array *ir)
{
   /* If the gl_ClipDistance var hasn't been declared yet, then
    * there's no way this deref can refer to it.
    */
   if (!this->old_clip_distance_var)
      return visit_continue;

   ir_dereference_variable *old_var_ref = ir->array->as_dereference_variable();
   if (old_var_ref && old_var_ref->var == this->old_clip_distance_var) {
      this->progress = true;
      ir_rvalue *array_index;
      ir_rvalue *swizzle_index;
      this->create_indices(ir->array_index, array_index, swizzle_index);
      void *mem_ctx = ralloc_parent(ir);
      ir->array = new(mem_ctx) ir_dereference_array(
         this->new_clip_distance_var, array_index);
      ir->array_index = swizzle_index;
   }

   return visit_continue;
}


/**
 * Replace any assignment having gl_ClipDistance (undereferenced) as its LHS
 * or RHS with a sequence of assignments, one for each component of the array.
 * Each of these assignments is lowered to refer to gl_ClipDistanceMESA as
 * appropriate.
 */
ir_visitor_status
lower_clip_distance_visitor::visit_leave(ir_assignment *ir)
{
   ir_dereference_variable *lhs_var = ir->lhs->as_dereference_variable();
   ir_dereference_variable *rhs_var = ir->rhs->as_dereference_variable();
   if ((lhs_var && lhs_var->var == this->old_clip_distance_var)
       || (rhs_var && rhs_var->var == this->old_clip_distance_var)) {
      /* LHS or RHS of the assignment is the entire gl_ClipDistance array.
       * Since we are reshaping gl_ClipDistance from an array of floats to an
       * array of vec4's, this isn't going to work as a bulk assignment
       * anymore, so unroll it to element-by-element assignments and lower
       * each of them.
       *
       * Note: to unroll into element-by-element assignments, we need to make
       * clones of the LHS and RHS.  This is only safe if the LHS and RHS are
       * side-effect free.  Fortunately, we know that they are, because the
       * only kind of rvalue that can have side effects is an ir_call, and
       * ir_calls only appear (a) as a statement on their own, or (b) as the
       * RHS of an assignment that stores the result of the call in a
       * temporary variable.
       */
      void *ctx = ralloc_parent(ir);
      int array_size = this->old_clip_distance_var->type->array_size();
      for (int i = 0; i < array_size; ++i) {
         ir_dereference_array *new_lhs = new(ctx) ir_dereference_array(
            ir->lhs->clone(ctx, NULL), new(ctx) ir_constant(i));
         new_lhs->accept(this);
         ir_dereference_array *new_rhs = new(ctx) ir_dereference_array(
            ir->rhs->clone(ctx, NULL), new(ctx) ir_constant(i));
         new_rhs->accept(this);
         this->base_ir->insert_before(
            new(ctx) ir_assignment(new_lhs, new_rhs));
      }
      ir->remove();
   }

   return visit_continue;
}


/**
 * Set up base_ir properly and call visit_leave() on a newly created
 * ir_assignment node.  This is used in cases where we have to insert an
 * ir_assignment in a place where we know the hierarchical visitor won't see
 * it.
 */
void
lower_clip_distance_visitor::visit_new_assignment(ir_assignment *ir)
{
   ir_instruction *old_base_ir = this->base_ir;
   this->base_ir = ir;
   ir->accept(this);
   this->base_ir = old_base_ir;
}


/**
 * If gl_ClipDistance appears as an argument in an ir_call expression, replace
 * it with a temporary variable, and make sure the ir_call is preceded and/or
 * followed by assignments that copy the contents of the temporary variable to
 * and/or from gl_ClipDistance.  Each of these assignments is then lowered to
 * refer to gl_ClipDistanceMESA.
 */
ir_visitor_status
lower_clip_distance_visitor::visit_leave(ir_call *ir)
{
   void *ctx = ralloc_parent(ir);

   const exec_node *formal_param_node = ir->get_callee()->parameters.head;
   const exec_node *actual_param_node = ir->actual_parameters.head;
   while (!actual_param_node->is_tail_sentinel()) {
      ir_variable *formal_param = (ir_variable *) formal_param_node;
      ir_rvalue *actual_param = (ir_rvalue *) actual_param_node;

      /* Advance formal_param_node and actual_param_node now so that we can
       * safely replace actual_param with another node, if necessary, below.
       */
      formal_param_node = formal_param_node->next;
      actual_param_node = actual_param_node->next;

      ir_dereference_variable *deref = actual_param->as_dereference_variable();
      if (deref && deref->var == this->old_clip_distance_var) {
         /* User is trying to pass the whole gl_ClipDistance array to a
          * function call.  Since we are reshaping gl_ClipDistance from an
          * array of floats to an array of vec4's, this isn't going to work
          * anymore, so use a temporary array instead.
          */
         ir_variable *temp_clip_distance = new(ctx) ir_variable(
            actual_param->type, "temp_clip_distance", ir_var_temporary);
         this->base_ir->insert_before(temp_clip_distance);
         actual_param->replace_with(
            new(ctx) ir_dereference_variable(temp_clip_distance));
         if (formal_param->mode == ir_var_in
             || formal_param->mode == ir_var_inout) {
            /* Copy from gl_ClipDistance to the temporary before the call.
             * Since we are going to insert this copy before the current
             * instruction, we need to visit it afterwards to make sure it
             * gets lowered.
             */
            ir_assignment *new_assignment = new(ctx) ir_assignment(
               new(ctx) ir_dereference_variable(temp_clip_distance),
               new(ctx) ir_dereference_variable(old_clip_distance_var));
            this->base_ir->insert_before(new_assignment);
            this->visit_new_assignment(new_assignment);
         }
         if (formal_param->mode == ir_var_out
             || formal_param->mode == ir_var_inout) {
            /* Copy from the temporary to gl_ClipDistance after the call.
             * Since visit_list_elements() has already decided which
             * instruction it's going to visit next, we need to visit
             * afterwards to make sure it gets lowered.
             */
            ir_assignment *new_assignment = new(ctx) ir_assignment(
               new(ctx) ir_dereference_variable(old_clip_distance_var),
               new(ctx) ir_dereference_variable(temp_clip_distance));
            this->base_ir->insert_after(new_assignment);
            this->visit_new_assignment(new_assignment);
         }
      }
   }

   return visit_continue;
}


bool
lower_clip_distance(exec_list *instructions)
{
   lower_clip_distance_visitor v;

   visit_list_elements(&v, instructions);

   return v.progress;
}
