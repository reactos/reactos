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
 * \file lower_noise.cpp
 * IR lower pass to remove noise opcodes.
 *
 * \author Ian Romanick <ian.d.romanick@intel.com>
 */

#include "ir.h"
#include "ir_rvalue_visitor.h"

class lower_noise_visitor : public ir_rvalue_visitor {
public:
   lower_noise_visitor() : progress(false)
   {
      /* empty */
   }

   void handle_rvalue(ir_rvalue **rvalue)
   {
      if (!*rvalue)
	 return;

      ir_expression *expr = (*rvalue)->as_expression();
      if (!expr)
	 return;

      /* In the future, ir_unop_noise may be replaced by a call to a function
       * that implements noise.  No hardware has a noise instruction.
       */
      if (expr->operation == ir_unop_noise) {
	 *rvalue = ir_constant::zero(ralloc_parent(expr), expr->type);
	 this->progress = true;
      }
   }

   bool progress;
};


bool
lower_noise(exec_list *instructions)
{
   lower_noise_visitor v;

   visit_list_elements(&v, instructions);

   return v.progress;
}
