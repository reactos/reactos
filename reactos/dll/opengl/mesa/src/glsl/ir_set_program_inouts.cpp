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
 * \file ir_set_program_inouts.cpp
 *
 * Sets the InputsRead and OutputsWritten of Mesa programs.
 *
 * Additionally, for fragment shaders, sets the InterpQualifier array.
 *
 * Mesa programs (gl_program, not gl_shader_program) have a set of
 * flags indicating which varyings are read and written.  Computing
 * which are actually read from some sort of backend code can be
 * tricky when variable array indexing involved.  So this pass
 * provides support for setting InputsRead and OutputsWritten right
 * from the GLSL IR.
 */

#include "main/core.h" /* for struct gl_program */
#include "program/hash_table.h"
#include "ir.h"
#include "ir_visitor.h"
#include "glsl_types.h"

class ir_set_program_inouts_visitor : public ir_hierarchical_visitor {
public:
   ir_set_program_inouts_visitor(struct gl_program *prog,
                                 bool is_fragment_shader)
   {
      this->prog = prog;
      this->is_fragment_shader = is_fragment_shader;
      this->ht = hash_table_ctor(0,
				 hash_table_pointer_hash,
				 hash_table_pointer_compare);
   }
   ~ir_set_program_inouts_visitor()
   {
      hash_table_dtor(this->ht);
   }

   virtual ir_visitor_status visit_enter(ir_dereference_array *);
   virtual ir_visitor_status visit_enter(ir_function_signature *);
   virtual ir_visitor_status visit(ir_dereference_variable *);
   virtual ir_visitor_status visit(ir_variable *);

   struct gl_program *prog;
   struct hash_table *ht;
   bool is_fragment_shader;
};

static void
mark(struct gl_program *prog, ir_variable *var, int offset, int len,
     bool is_fragment_shader)
{
   /* As of GLSL 1.20, varyings can only be floats, floating-point
    * vectors or matrices, or arrays of them.  For Mesa programs using
    * InputsRead/OutputsWritten, everything but matrices uses one
    * slot, while matrices use a slot per column.  Presumably
    * something doing a more clever packing would use something other
    * than InputsRead/OutputsWritten.
    */

   for (int i = 0; i < len; i++) {
      GLbitfield64 bitfield = BITFIELD64_BIT(var->location + offset + i);
      if (var->mode == ir_var_in) {
	 prog->InputsRead |= bitfield;
         if (is_fragment_shader) {
            gl_fragment_program *fprog = (gl_fragment_program *) prog;
            fprog->InterpQualifier[var->location + offset + i] =
               (glsl_interp_qualifier) var->interpolation;
         }
      } else if (var->mode == ir_var_system_value) {
         prog->SystemValuesRead |= bitfield;
      } else {
	 prog->OutputsWritten |= bitfield;
      }
   }
}

/* Default handler: Mark all the locations in the variable as used. */
ir_visitor_status
ir_set_program_inouts_visitor::visit(ir_dereference_variable *ir)
{
   if (hash_table_find(this->ht, ir->var) == NULL)
      return visit_continue;

   if (ir->type->is_array()) {
      mark(this->prog, ir->var, 0,
	   ir->type->length * ir->type->fields.array->matrix_columns,
           this->is_fragment_shader);
   } else {
      mark(this->prog, ir->var, 0, ir->type->matrix_columns,
           this->is_fragment_shader);
   }

   return visit_continue;
}

ir_visitor_status
ir_set_program_inouts_visitor::visit_enter(ir_dereference_array *ir)
{
   ir_dereference_variable *deref_var;
   ir_constant *index = ir->array_index->as_constant();
   deref_var = ir->array->as_dereference_variable();
   ir_variable *var = NULL;

   /* Check that we're dereferencing a shader in or out */
   if (deref_var)
      var = (ir_variable *)hash_table_find(this->ht, deref_var->var);

   if (index && var) {
      int width = 1;

      if (deref_var->type->is_array() &&
	  deref_var->type->fields.array->is_matrix()) {
	 width = deref_var->type->fields.array->matrix_columns;
      }

      mark(this->prog, var, index->value.i[0] * width, width,
           this->is_fragment_shader);
      return visit_continue_with_parent;
   }

   return visit_continue;
}

ir_visitor_status
ir_set_program_inouts_visitor::visit(ir_variable *ir)
{
   if (ir->mode == ir_var_in ||
       ir->mode == ir_var_out ||
       ir->mode == ir_var_system_value) {
      hash_table_insert(this->ht, ir, ir);
   }

   return visit_continue;
}

ir_visitor_status
ir_set_program_inouts_visitor::visit_enter(ir_function_signature *ir)
{
   /* We don't want to descend into the function parameters and
    * consider them as shader inputs or outputs.
    */
   visit_list_elements(this, &ir->body);
   return visit_continue_with_parent;
}

void
do_set_program_inouts(exec_list *instructions, struct gl_program *prog,
                      bool is_fragment_shader)
{
   ir_set_program_inouts_visitor v(prog, is_fragment_shader);

   prog->InputsRead = 0;
   prog->OutputsWritten = 0;
   prog->SystemValuesRead = 0;
   if (is_fragment_shader) {
      memset(((gl_fragment_program *) prog)->InterpQualifier, 0,
             sizeof(((gl_fragment_program *) prog)->InterpQualifier));
   }
   visit_list_elements(&v, instructions);
}
