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

#include "glsl_types.h"
#include "loop_analysis.h"
#include "ir_hierarchical_visitor.h"

static bool is_loop_terminator(ir_if *ir);

static bool all_expression_operands_are_loop_constant(ir_rvalue *,
						      hash_table *);

static ir_rvalue *get_basic_induction_increment(ir_assignment *, hash_table *);


loop_state::loop_state()
{
   this->ht = hash_table_ctor(0, hash_table_pointer_hash,
			      hash_table_pointer_compare);
   this->mem_ctx = ralloc_context(NULL);
   this->loop_found = false;
}


loop_state::~loop_state()
{
   hash_table_dtor(this->ht);
   ralloc_free(this->mem_ctx);
}


loop_variable_state *
loop_state::insert(ir_loop *ir)
{
   loop_variable_state *ls = new(this->mem_ctx) loop_variable_state;

   hash_table_insert(this->ht, ls, ir);
   this->loop_found = true;

   return ls;
}


loop_variable_state *
loop_state::get(const ir_loop *ir)
{
   return (loop_variable_state *) hash_table_find(this->ht, ir);
}


loop_variable *
loop_variable_state::get(const ir_variable *ir)
{
   return (loop_variable *) hash_table_find(this->var_hash, ir);
}


loop_variable *
loop_variable_state::insert(ir_variable *var)
{
   void *mem_ctx = ralloc_parent(this);
   loop_variable *lv = rzalloc(mem_ctx, loop_variable);

   lv->var = var;

   hash_table_insert(this->var_hash, lv, lv->var);
   this->variables.push_tail(lv);

   return lv;
}


loop_terminator *
loop_variable_state::insert(ir_if *if_stmt)
{
   void *mem_ctx = ralloc_parent(this);
   loop_terminator *t = rzalloc(mem_ctx, loop_terminator);

   t->ir = if_stmt;
   this->terminators.push_tail(t);

   return t;
}


class loop_analysis : public ir_hierarchical_visitor {
public:
   loop_analysis();

   virtual ir_visitor_status visit(ir_loop_jump *);
   virtual ir_visitor_status visit(ir_dereference_variable *);

   virtual ir_visitor_status visit_enter(ir_call *);

   virtual ir_visitor_status visit_enter(ir_loop *);
   virtual ir_visitor_status visit_leave(ir_loop *);
   virtual ir_visitor_status visit_enter(ir_assignment *);
   virtual ir_visitor_status visit_leave(ir_assignment *);
   virtual ir_visitor_status visit_enter(ir_if *);
   virtual ir_visitor_status visit_leave(ir_if *);

   loop_state *loops;

   int if_statement_depth;

   ir_assignment *current_assignment;

   exec_list state;
};


loop_analysis::loop_analysis()
{
   this->loops = new loop_state;

   this->if_statement_depth = 0;
   this->current_assignment = NULL;
}


ir_visitor_status
loop_analysis::visit(ir_loop_jump *ir)
{
   (void) ir;

   assert(!this->state.is_empty());

   loop_variable_state *const ls =
      (loop_variable_state *) this->state.get_head();

   ls->num_loop_jumps++;

   return visit_continue;
}


ir_visitor_status
loop_analysis::visit_enter(ir_call *ir)
{
   /* If we're not somewhere inside a loop, there's nothing to do. */
   if (this->state.is_empty())
      return visit_continue;

   loop_variable_state *const ls =
      (loop_variable_state *) this->state.get_head();

   ls->contains_calls = true;
   return visit_continue_with_parent;
}


ir_visitor_status
loop_analysis::visit(ir_dereference_variable *ir)
{
   /* If we're not somewhere inside a loop, there's nothing to do.
    */
   if (this->state.is_empty())
      return visit_continue;

   loop_variable_state *const ls =
      (loop_variable_state *) this->state.get_head();

   ir_variable *var = ir->variable_referenced();
   loop_variable *lv = ls->get(var);

   if (lv == NULL) {
      lv = ls->insert(var);
      lv->read_before_write = !this->in_assignee;
   }

   if (this->in_assignee) {
      assert(this->current_assignment != NULL);

      lv->conditional_assignment = (this->if_statement_depth > 0)
	 || (this->current_assignment->condition != NULL);

      if (lv->first_assignment == NULL) {
	 assert(lv->num_assignments == 0);

	 lv->first_assignment = this->current_assignment;
      }

      lv->num_assignments++;
   } else if (lv->first_assignment == this->current_assignment) {
      /* This catches the case where the variable is used in the RHS of an
       * assignment where it is also in the LHS.
       */
      lv->read_before_write = true;
   }

   return visit_continue;
}

ir_visitor_status
loop_analysis::visit_enter(ir_loop *ir)
{
   loop_variable_state *ls = this->loops->insert(ir);
   this->state.push_head(ls);

   return visit_continue;
}

ir_visitor_status
loop_analysis::visit_leave(ir_loop *ir)
{
   loop_variable_state *const ls =
      (loop_variable_state *) this->state.pop_head();

   /* Function calls may contain side effects.  These could alter any of our
    * variables in ways that cannot be known, and may even terminate shader
    * execution (say, calling discard in the fragment shader).  So we can't
    * rely on any of our analysis about assignments to variables.
    *
    * We could perform some conservative analysis (prove there's no statically
    * possible assignment, etc.) but it isn't worth it for now; function
    * inlining will allow us to unroll loops anyway.
    */
   if (ls->contains_calls)
      return visit_continue;

   foreach_list(node, &ir->body_instructions) {
      /* Skip over declarations at the start of a loop.
       */
      if (((ir_instruction *) node)->as_variable())
	 continue;

      ir_if *if_stmt = ((ir_instruction *) node)->as_if();

      if ((if_stmt != NULL) && is_loop_terminator(if_stmt))
	 ls->insert(if_stmt);
      else
	 break;
   }


   foreach_list_safe(node, &ls->variables) {
      loop_variable *lv = (loop_variable *) node;

      /* Move variables that are already marked as being loop constant to
       * a separate list.  These trivially don't need to be tested.
       */
      if (lv->is_loop_constant()) {
	 lv->remove();
	 ls->constants.push_tail(lv);
      }
   }

   /* Each variable assigned in the loop that isn't already marked as being loop
    * constant might still be loop constant.  The requirements at this point
    * are:
    *
    *    - Variable is written before it is read.
    *
    *    - Only one assignment to the variable.
    *
    *    - All operands on the RHS of the assignment are also loop constants.
    *
    * The last requirement is the reason for the progress loop.  A variable
    * marked as a loop constant on one pass may allow other variables to be
    * marked as loop constant on following passes.
    */
   bool progress;
   do {
      progress = false;

      foreach_list_safe(node, &ls->variables) {
	 loop_variable *lv = (loop_variable *) node;

	 if (lv->conditional_assignment || (lv->num_assignments > 1))
	    continue;

	 /* Process the RHS of the assignment.  If all of the variables
	  * accessed there are loop constants, then add this
	  */
	 ir_rvalue *const rhs = lv->first_assignment->rhs;
	 if (all_expression_operands_are_loop_constant(rhs, ls->var_hash)) {
	    lv->rhs_clean = true;

	    if (lv->is_loop_constant()) {
	       progress = true;

	       lv->remove();
	       ls->constants.push_tail(lv);
	    }
	 }
      }
   } while (progress);

   /* The remaining variables that are not loop invariant might be loop
    * induction variables.
    */
   foreach_list_safe(node, &ls->variables) {
      loop_variable *lv = (loop_variable *) node;

      /* If there is more than one assignment to a variable, it cannot be a
       * loop induction variable.  This isn't strictly true, but this is a
       * very simple induction variable detector, and it can't handle more
       * complex cases.
       */
      if (lv->num_assignments > 1)
	 continue;

      /* All of the variables with zero assignments in the loop are loop
       * invariant, and they should have already been filtered out.
       */
      assert(lv->num_assignments == 1);
      assert(lv->first_assignment != NULL);

      /* The assignmnet to the variable in the loop must be unconditional.
       */
      if (lv->conditional_assignment)
	 continue;

      /* Basic loop induction variables have a single assignment in the loop
       * that has the form 'VAR = VAR + i' or 'VAR = VAR - i' where i is a
       * loop invariant.
       */
      ir_rvalue *const inc =
	 get_basic_induction_increment(lv->first_assignment, ls->var_hash);
      if (inc != NULL) {
	 lv->iv_scale = NULL;
	 lv->biv = lv->var;
	 lv->increment = inc;

	 lv->remove();
	 ls->induction_variables.push_tail(lv);
      }
   }

   return visit_continue;
}

ir_visitor_status
loop_analysis::visit_enter(ir_if *ir)
{
   (void) ir;

   if (!this->state.is_empty())
      this->if_statement_depth++;

   return visit_continue;
}

ir_visitor_status
loop_analysis::visit_leave(ir_if *ir)
{
   (void) ir;

   if (!this->state.is_empty())
      this->if_statement_depth--;

   return visit_continue;
}

ir_visitor_status
loop_analysis::visit_enter(ir_assignment *ir)
{
   /* If we're not somewhere inside a loop, there's nothing to do.
    */
   if (this->state.is_empty())
      return visit_continue_with_parent;

   this->current_assignment = ir;

   return visit_continue;
}

ir_visitor_status
loop_analysis::visit_leave(ir_assignment *ir)
{
   /* Since the visit_enter exits with visit_continue_with_parent for this
    * case, the loop state stack should never be empty here.
    */
   assert(!this->state.is_empty());

   assert(this->current_assignment == ir);
   this->current_assignment = NULL;

   return visit_continue;
}


class examine_rhs : public ir_hierarchical_visitor {
public:
   examine_rhs(hash_table *loop_variables)
   {
      this->only_uses_loop_constants = true;
      this->loop_variables = loop_variables;
   }

   virtual ir_visitor_status visit(ir_dereference_variable *ir)
   {
      loop_variable *lv =
	 (loop_variable *) hash_table_find(this->loop_variables, ir->var);

      assert(lv != NULL);

      if (lv->is_loop_constant()) {
	 return visit_continue;
      } else {
	 this->only_uses_loop_constants = false;
	 return visit_stop;
      }
   }

   hash_table *loop_variables;
   bool only_uses_loop_constants;
};


bool
all_expression_operands_are_loop_constant(ir_rvalue *ir, hash_table *variables)
{
   examine_rhs v(variables);

   ir->accept(&v);

   return v.only_uses_loop_constants;
}


ir_rvalue *
get_basic_induction_increment(ir_assignment *ir, hash_table *var_hash)
{
   /* The RHS must be a binary expression.
    */
   ir_expression *const rhs = ir->rhs->as_expression();
   if ((rhs == NULL)
       || ((rhs->operation != ir_binop_add)
	   && (rhs->operation != ir_binop_sub)))
      return NULL;

   /* One of the of operands of the expression must be the variable assigned.
    * If the operation is subtraction, the variable in question must be the
    * "left" operand.
    */
   ir_variable *const var = ir->lhs->variable_referenced();

   ir_variable *const op0 = rhs->operands[0]->variable_referenced();
   ir_variable *const op1 = rhs->operands[1]->variable_referenced();

   if (((op0 != var) && (op1 != var))
       || ((op1 == var) && (rhs->operation == ir_binop_sub)))
      return NULL;

   ir_rvalue *inc = (op0 == var) ? rhs->operands[1] : rhs->operands[0];

   if (inc->as_constant() == NULL) {
      ir_variable *const inc_var = inc->variable_referenced();
      if (inc_var != NULL) {
	 loop_variable *lv =
	    (loop_variable *) hash_table_find(var_hash, inc_var);

	 if (!lv->is_loop_constant())
	    inc = NULL;
      } else
	 inc = NULL;
   }

   if ((inc != NULL) && (rhs->operation == ir_binop_sub)) {
      void *mem_ctx = ralloc_parent(ir);

      inc = new(mem_ctx) ir_expression(ir_unop_neg,
				       inc->type,
				       inc->clone(mem_ctx, NULL),
				       NULL);
   }

   return inc;
}


/**
 * Detect whether an if-statement is a loop terminating condition
 *
 * Detects if-statements of the form
 *
 *  (if (expression bool ...) (break))
 */
bool
is_loop_terminator(ir_if *ir)
{
   if (!ir->else_instructions.is_empty())
      return false;

   ir_instruction *const inst =
      (ir_instruction *) ir->then_instructions.get_head();
   assert(inst != NULL);

   if (inst->ir_type != ir_type_loop_jump)
      return false;

   ir_loop_jump *const jump = (ir_loop_jump *) inst;
   if (jump->mode != ir_loop_jump::jump_break)
      return false;

   return true;
}


loop_state *
analyze_loop_variables(exec_list *instructions)
{
   loop_analysis v;

   v.run(instructions);
   return v.loops;
}
