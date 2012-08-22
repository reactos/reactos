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
 * \file ir_variable_refcount.h
 *
 * Provides a visitor which produces a list of variables referenced,
 * how many times they were referenced and assigned, and whether they
 * were defined in the scope.
 */

#include "ir.h"
#include "ir_visitor.h"
#include "glsl_types.h"

class variable_entry : public exec_node
{
public:
   variable_entry(ir_variable *var);

   ir_variable *var; /* The key: the variable's pointer. */
   ir_assignment *assign; /* An assignment to the variable, if any */

   /** Number of times the variable is referenced, including assignments. */
   unsigned referenced_count;

   /** Number of times the variable is assigned. */
   unsigned assigned_count;

   bool declaration; /* If the variable had a decl in the instruction stream */
};

class ir_variable_refcount_visitor : public ir_hierarchical_visitor {
public:
   ir_variable_refcount_visitor(void)
   {
      this->mem_ctx = ralloc_context(NULL);
      this->variable_list.make_empty();
   }

   ~ir_variable_refcount_visitor(void)
   {
      ralloc_free(this->mem_ctx);
   }

   virtual ir_visitor_status visit(ir_variable *);
   virtual ir_visitor_status visit(ir_dereference_variable *);

   virtual ir_visitor_status visit_enter(ir_function_signature *);
   virtual ir_visitor_status visit_leave(ir_assignment *);

   variable_entry *get_variable_entry(ir_variable *var);

   /* List of variable_entry */
   exec_list variable_list;

   void *mem_ctx;
};
