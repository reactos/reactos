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
 * \file opt_dead_code_local.cpp
 *
 * Eliminates local dead assignments from the code.
 *
 * This operates on basic blocks, tracking assignments and finding if
 * they're used before the variable is completely reassigned.
 *
 * Compare this to ir_dead_code.cpp, which operates globally looking
 * for assignments to variables that are never read.
 */

#include "ir.h"
#include "ir_basic_block.h"
#include "ir_optimization.h"
#include "glsl_types.h"

static bool debug = false;

class assignment_entry : public exec_node
{
public:
   assignment_entry(ir_variable *lhs, ir_instruction *ir)
   {
      assert(lhs);
      assert(ir);
      this->lhs = lhs;
      this->ir = ir;
   }

   ir_variable *lhs;
   ir_instruction *ir;
};

class kill_for_derefs_visitor : public ir_hierarchical_visitor {
public:
   kill_for_derefs_visitor(exec_list *assignments)
   {
      this->assignments = assignments;
   }

   virtual ir_visitor_status visit(ir_dereference_variable *ir)
   {
      ir_variable *const var = ir->variable_referenced();

      foreach_iter(exec_list_iterator, iter, *this->assignments) {
	 assignment_entry *entry = (assignment_entry *)iter.get();

	 if (entry->lhs == var) {
	    if (debug)
	       printf("kill %s\n", entry->lhs->name);
	    entry->remove();
	 }
      }

      return visit_continue;
   }

private:
   exec_list *assignments;
};

class array_index_visit : public ir_hierarchical_visitor {
public:
   array_index_visit(ir_hierarchical_visitor *v)
   {
      this->visitor = v;
   }

   virtual ir_visitor_status visit_enter(class ir_dereference_array *ir)
   {
      ir->array_index->accept(visitor);
      return visit_continue;
   }

   static void run(ir_instruction *ir, ir_hierarchical_visitor *v)
   {
      array_index_visit top_visit(v);
      ir->accept(& top_visit);
   }

   ir_hierarchical_visitor *visitor;
};


/**
 * Adds an entry to the available copy list if it's a plain assignment
 * of a variable to a variable.
 */
static bool
process_assignment(void *ctx, ir_assignment *ir, exec_list *assignments)
{
   ir_variable *var = NULL;
   bool progress = false;
   kill_for_derefs_visitor v(assignments);

   /* Kill assignment entries for things used to produce this assignment. */
   ir->rhs->accept(&v);
   if (ir->condition) {
      ir->condition->accept(&v);
   }

   /* Kill assignment enties used as array indices.
    */
   array_index_visit::run(ir->lhs, &v);
   var = ir->lhs->variable_referenced();
   assert(var);

   /* Now, check if we did a whole-variable assignment. */
   if (!ir->condition && (ir->whole_variable_written() != NULL)) {
      /* We did a whole-variable assignment.  So, any instruction in
       * the assignment list with the same LHS is dead.
       */
      if (debug)
	 printf("looking for %s to remove\n", var->name);
      foreach_iter(exec_list_iterator, iter, *assignments) {
	 assignment_entry *entry = (assignment_entry *)iter.get();

	 if (entry->lhs == var) {
	    if (debug)
	       printf("removing %s\n", var->name);
	    entry->ir->remove();
	    entry->remove();
	    progress = true;
	 }
      }
   }

   /* Add this instruction to the assignment list available to be removed.
    * But not if the assignment has other side effects.
    */
   if (ir_has_call(ir))
      return progress;

   assignment_entry *entry = new(ctx) assignment_entry(var, ir);
   assignments->push_tail(entry);

   if (debug) {
      printf("add %s\n", var->name);

      printf("current entries\n");
      foreach_iter(exec_list_iterator, iter, *assignments) {
	 assignment_entry *entry = (assignment_entry *)iter.get();

	 printf("    %s\n", entry->lhs->name);
      }
   }

   return progress;
}

static void
dead_code_local_basic_block(ir_instruction *first,
			     ir_instruction *last,
			     void *data)
{
   ir_instruction *ir, *ir_next;
   /* List of avaialble_copy */
   exec_list assignments;
   bool *out_progress = (bool *)data;
   bool progress = false;

   void *ctx = ralloc_context(NULL);
   /* Safe looping, since process_assignment */
   for (ir = first, ir_next = (ir_instruction *)first->next;;
	ir = ir_next, ir_next = (ir_instruction *)ir->next) {
      ir_assignment *ir_assign = ir->as_assignment();

      if (debug) {
	 ir->print();
	 printf("\n");
      }

      if (ir_assign) {
	 progress = process_assignment(ctx, ir_assign, &assignments) || progress;
      } else {
	 kill_for_derefs_visitor kill(&assignments);
	 ir->accept(&kill);
      }

      if (ir == last)
	 break;
   }
   *out_progress = progress;
   ralloc_free(ctx);
}

/**
 * Does a copy propagation pass on the code present in the instruction stream.
 */
bool
do_dead_code_local(exec_list *instructions)
{
   bool progress = false;

   call_for_basic_blocks(instructions, dead_code_local_basic_block, &progress);

   return progress;
}
