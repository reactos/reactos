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
 * \file opt_noop_swizzle.cpp
 *
 * If a swizzle doesn't change the order or count of components, then
 * remove the swizzle so that other optimization passes see the value
 * behind it.
 */

#include "ir.h"
#include "ir_visitor.h"
#include "ir_rvalue_visitor.h"
#include "ir_print_visitor.h"
#include "glsl_types.h"

class ir_noop_swizzle_visitor : public ir_rvalue_visitor {
public:
   ir_noop_swizzle_visitor()
   {
      this->progress = false;
   }

   void handle_rvalue(ir_rvalue **rvalue);
   bool progress;
};

void
ir_noop_swizzle_visitor::handle_rvalue(ir_rvalue **rvalue)
{
   if (!*rvalue)
      return;

   ir_swizzle *swiz = (*rvalue)->as_swizzle();
   if (!swiz || swiz->type != swiz->val->type)
      return;

   int elems = swiz->val->type->vector_elements;
   if (swiz->mask.x != 0)
      return;
   if (elems >= 2 && swiz->mask.y != 1)
      return;
   if (elems >= 3 && swiz->mask.z != 2)
      return;
   if (elems >= 4 && swiz->mask.w != 3)
      return;

   this->progress = true;
   *rvalue = swiz->val;
}

bool
do_noop_swizzle(exec_list *instructions)
{
   ir_noop_swizzle_visitor v;
   visit_list_elements(&v, instructions);

   return v.progress;
}
