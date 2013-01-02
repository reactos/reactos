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
 * \file opt_discard_simplification.cpp
 *
 * This pass simplifies if-statements and loops containing unconditional
 * discards.
 *
 * Case 1: Both branches contain unconditional discards:
 * -----------------------------------------------------
 *
 *    if (cond) {
 *	 s1;
 *	 discard;
 *	 s2;
 *    } else {
 *	 s3;
 *	 discard;
 *	 s4;
 *    }
 *
 * becomes:
 *
 *    discard
 *
 * Case 2: The "then" clause contains an unconditional discard:
 * ------------------------------------------------------------
 *
 *    if (cond) {
 *       s1;
 *       discard;
 *       s2;
 *    } else {
 *	 s3;
 *    }
 *
 * becomes:
 *
 *    if (cond) {
 *	 discard;
 *    } else {
 *	 s3;
 *    }
 *
 * Case 3: The "else" clause contains an unconditional discard:
 * ------------------------------------------------------------
 *
 *    if (cond) {
 *       s1;
 *    } else {
 *       s2;
 *       discard;
 *	 s3;
 *    }
 *
 * becomes:
 *
 *    if (cond) {
 *	 s1;
 *    } else {
 *	 discard;
 *    }
 */

#include "glsl_types.h"
#include "ir.h"

class discard_simplifier : public ir_hierarchical_visitor {
public:
   discard_simplifier()
   {
      this->progress = false;
   }

   ir_visitor_status visit_enter(ir_if *);
   ir_visitor_status visit_enter(ir_loop *);
   ir_visitor_status visit_enter(ir_assignment *);

   bool progress;
};

static ir_discard *
find_unconditional_discard(exec_list &instructions)
{
   foreach_list(n, &instructions) {
      ir_instruction *ir = (ir_instruction *)n;

      if (ir->ir_type == ir_type_return ||
	  ir->ir_type == ir_type_loop_jump)
	 return NULL;

      /* So far, this code doesn't know how to look inside of flow
       * control to see if a discard later on at this level is
       * unconditional.
       */
      if (ir->ir_type == ir_type_if ||
	  ir->ir_type == ir_type_loop)
	 return NULL;

      ir_discard *discard = ir->as_discard();
      if (discard != NULL && discard->condition == NULL)
	 return discard;
   }
   return NULL;
}

static bool
is_only_instruction(ir_discard *discard)
{
   return (discard->prev->is_head_sentinel() &&
	   discard->next->is_tail_sentinel());
}

/* We only care about the top level instructions, so don't descend
 * into expressions.
 */
ir_visitor_status
discard_simplifier::visit_enter(ir_assignment *ir)
{
   (void) ir;
   return visit_continue_with_parent;
}

ir_visitor_status
discard_simplifier::visit_enter(ir_if *ir)
{
   ir_discard *then_discard = find_unconditional_discard(ir->then_instructions);
   ir_discard *else_discard = find_unconditional_discard(ir->else_instructions);

   if (then_discard == NULL && else_discard == NULL)
      return visit_continue;

   /* If both branches result in discard, replace whole if with discard. */
   if (then_discard != NULL && else_discard != NULL) {
      this->progress = true;
      ir->replace_with(then_discard);
      return visit_continue_with_parent;
   }

   /* Otherwise, one branch has a discard. */
   if (then_discard != NULL && !is_only_instruction(then_discard)) {
      this->progress = true;
      ir->then_instructions.make_empty();
      ir->then_instructions.push_tail(then_discard);
   } else if (else_discard != NULL && !is_only_instruction(else_discard)) {
      this->progress = true;
      ir->else_instructions.make_empty();
      ir->else_instructions.push_tail(else_discard);
   }

   visit_list_elements(this, &ir->then_instructions);
   return visit_continue_with_parent;
}

ir_visitor_status
discard_simplifier::visit_enter(ir_loop *ir)
{
   ir_discard *discard = find_unconditional_discard(ir->body_instructions);

   if (discard) {
      ir->replace_with(discard);
      return visit_continue_with_parent;
   }

   return visit_continue;
}

bool
do_discard_simplification(exec_list *instructions)
{
   /* Look for a top-level unconditional discard */
   ir_discard *discard = find_unconditional_discard(*instructions);
   if (discard != NULL) {
      instructions->make_empty();
      instructions->push_tail(discard);
      return true;
   }

   discard_simplifier v;

   visit_list_elements(&v, instructions);

   return v.progress;
}
