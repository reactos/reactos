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
 * \file ir_import_prototypes.cpp
 * Import function prototypes from one IR tree into another.
 *
 * \author Ian Romanick
 */
#include "ir.h"
#include "glsl_symbol_table.h"

/**
 * Visitor used to import function prototypes
 *
 * Normally the \c clone method of either \c ir_function or
 * \c ir_function_signature could be used.  However, we don't want a complete
 * clone of the \c ir_function_signature.  We want everything \b except the
 * body of the function.
 */
class import_prototype_visitor : public ir_hierarchical_visitor {
public:
   /**
    */
   import_prototype_visitor(exec_list *list, glsl_symbol_table *symbols,
			    void *mem_ctx)
   {
      this->mem_ctx = mem_ctx;
      this->list = list;
      this->symbols = symbols;
      this->function = NULL;
   }

   virtual ir_visitor_status visit_enter(ir_function *ir)
   {
      assert(this->function == NULL);

      this->function = this->symbols->get_function(ir->name);
      if (!this->function) {
	 this->function = new(this->mem_ctx) ir_function(ir->name);

	 list->push_tail(this->function);

	 /* Add the new function to the symbol table.
	  */
	 this->symbols->add_function(this->function);
      }
      return visit_continue;
   }

   virtual ir_visitor_status visit_leave(ir_function *ir)
   {
      (void) ir;
      assert(this->function != NULL);

      this->function = NULL;
      return visit_continue;
   }

   ir_visitor_status visit_enter(ir_function_signature *ir)
   {
      assert(this->function != NULL);

      ir_function_signature *copy = ir->clone_prototype(mem_ctx, NULL);

      this->function->add_signature(copy);

      /* Do not process child nodes of the ir_function_signature.  There can
       * never be any nodes inside the ir_function_signature that we care
       * about.  Instead continue with the next sibling.
       */
      return visit_continue_with_parent;
   }

private:
   exec_list *list;
   ir_function *function;
   glsl_symbol_table *symbols;
   void *mem_ctx;
};


/**
 * Import function prototypes from one IR tree into another
 *
 * \param source   Source instruction stream containing functions whose
 *                 prototypes are to be imported
 * \param dest     Destination instruction stream where new \c ir_function and
 *                 \c ir_function_signature nodes will be stored
 * \param symbols  Symbol table where new functions will be stored
 * \param mem_ctx  ralloc memory context used for new allocations
 */
void
import_prototypes(const exec_list *source, exec_list *dest,
		  glsl_symbol_table *symbols, void *mem_ctx)
{
   import_prototype_visitor v(dest, symbols, mem_ctx);

   /* Making source be const is just extra documentation.
    */
   v.run(const_cast<exec_list *>(source));
}
