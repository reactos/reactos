/*
 * Copyright © 2012 Vincent Lejeune
 * Copyright © 2012 Intel Corporation
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

#include "ir.h"
#include "program/hash_table.h"

/**
 * \file lower_output_reads.cpp
 *
 * In GLSL, shader output variables (such as varyings) can be both read and
 * written.  However, on some hardware, reading an output register causes
 * trouble.
 *
 * This pass creates temporary shadow copies of every (used) shader output,
 * and replaces all accesses to use those instead.  It also adds code to the
 * main() function to copy the final values to the actual shader outputs.
 */

class output_read_remover : public ir_hierarchical_visitor {
protected:
   /**
    * A hash table mapping from the original ir_variable shader outputs
    * (ir_var_out mode) to the new temporaries to be used instead.
    */
   hash_table *replacements;

   void *mem_ctx;
public:
   output_read_remover();
   ~output_read_remover();
   virtual ir_visitor_status visit(class ir_dereference_variable *);
   virtual ir_visitor_status visit_leave(class ir_return *);
   virtual ir_visitor_status visit_leave(class ir_function_signature *);
};

output_read_remover::output_read_remover()
{
   mem_ctx = ralloc_context(NULL);
   replacements =
      hash_table_ctor(0, hash_table_pointer_hash, hash_table_pointer_compare);
}

output_read_remover::~output_read_remover()
{
   hash_table_dtor(replacements);
   ralloc_free(mem_ctx);
}

ir_visitor_status
output_read_remover::visit(ir_dereference_variable *ir)
{
   if (ir->var->mode != ir_var_out)
      return visit_continue;

   ir_variable *temp = (ir_variable *) hash_table_find(replacements, ir->var);

   /* If we don't have an existing temporary, create one. */
   if (temp == NULL) {
      void *var_ctx = ralloc_parent(ir->var);
      temp = new(var_ctx) ir_variable(ir->var->type, ir->var->name,
                                      ir_var_temporary);
      hash_table_insert(replacements, temp, ir->var);
   }

   /* Update the dereference to use the temporary */
   ir->var = temp;

   return visit_continue;
}

/**
 * Create an assignment to copy a temporary value back to the actual output.
 */
static ir_assignment *
copy(void *ctx, ir_variable *output, ir_variable *temp)
{
   ir_dereference_variable *lhs = new(ctx) ir_dereference_variable(output);
   ir_dereference_variable *rhs = new(ctx) ir_dereference_variable(temp);
   return new(ctx) ir_assignment(lhs, rhs);
}

/** Insert a copy-back assignment before a "return" statement */
static void
emit_return_copy(const void *key, void *data, void *closure)
{
   ir_return *ir = (ir_return *) closure;
   ir->insert_before(copy(ir, (ir_variable *) key, (ir_variable *) data));
}

/** Insert a copy-back assignment at the end of the main() function */
static void
emit_main_copy(const void *key, void *data, void *closure)
{
   ir_function_signature *sig = (ir_function_signature *) closure;
   sig->body.push_tail(copy(sig, (ir_variable *) key, (ir_variable *) data));
}

ir_visitor_status
output_read_remover::visit_leave(ir_return *ir)
{
   hash_table_call_foreach(replacements, emit_return_copy, ir);
   return visit_continue;
}

ir_visitor_status
output_read_remover::visit_leave(ir_function_signature *sig)
{
   if (strcmp(sig->function_name(), "main") != 0)
      return visit_continue;

   hash_table_call_foreach(replacements, emit_main_copy, sig);
   return visit_continue;
}

void
lower_output_reads(exec_list *instructions)
{
   output_read_remover v;
   visit_list_elements(&v, instructions);
}
