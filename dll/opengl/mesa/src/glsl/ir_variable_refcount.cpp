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
 * \file ir_variable_refcount.cpp
 *
 * Provides a visitor which produces a list of variables referenced,
 * how many times they were referenced and assigned, and whether they
 * were defined in the scope.
 */

#include "ir.h"
#include "ir_visitor.h"
#include "ir_variable_refcount.h"
#include "glsl_types.h"


// constructor
variable_entry::variable_entry(ir_variable *var)
{
   this->var = var;
   assign = NULL;
   assigned_count = 0;
   declaration = false;
   referenced_count = 0;
}


variable_entry *
ir_variable_refcount_visitor::get_variable_entry(ir_variable *var)
{
   assert(var);
   foreach_iter(exec_list_iterator, iter, this->variable_list) {
      variable_entry *entry = (variable_entry *)iter.get();
      if (entry->var == var)
	 return entry;
   }

   variable_entry *entry = new(mem_ctx) variable_entry(var);
   assert(entry->referenced_count == 0);
   this->variable_list.push_tail(entry);
   return entry;
}


ir_visitor_status
ir_variable_refcount_visitor::visit(ir_variable *ir)
{
   variable_entry *entry = this->get_variable_entry(ir);
   if (entry)
      entry->declaration = true;

   return visit_continue;
}


ir_visitor_status
ir_variable_refcount_visitor::visit(ir_dereference_variable *ir)
{
   ir_variable *const var = ir->variable_referenced();
   variable_entry *entry = this->get_variable_entry(var);

   if (entry)
      entry->referenced_count++;

   return visit_continue;
}


ir_visitor_status
ir_variable_refcount_visitor::visit_enter(ir_function_signature *ir)
{
   /* We don't want to descend into the function parameters and
    * dead-code eliminate them, so just accept the body here.
    */
   visit_list_elements(this, &ir->body);
   return visit_continue_with_parent;
}


ir_visitor_status
ir_variable_refcount_visitor::visit_leave(ir_assignment *ir)
{
   variable_entry *entry;
   entry = this->get_variable_entry(ir->lhs->variable_referenced());
   if (entry) {
      entry->assigned_count++;
      if (entry->assign == NULL)
	 entry->assign = ir;
   }

   return visit_continue;
}
