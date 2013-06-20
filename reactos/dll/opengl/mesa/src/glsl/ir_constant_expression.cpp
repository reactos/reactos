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
 * \file ir_constant_expression.cpp
 * Evaluate and process constant valued expressions
 *
 * In GLSL, constant valued expressions are used in several places.  These
 * must be processed and evaluated very early in the compilation process.
 *
 *    * Sizes of arrays
 *    * Initializers for uniforms
 *    * Initializers for \c const variables
 */

#include <math.h>
#include "main/core.h" /* for MAX2, MIN2, CLAMP */
#include "ir.h"
#include "ir_visitor.h"
#include "glsl_types.h"

/* Using C99 rounding functions for roundToEven() implementation is
 * difficult, because round(), rint, and nearbyint() are affected by
 * fesetenv(), which the application may have done for its own
 * purposes.  Mesa's IROUND macro is close to what we want, but it
 * rounds away from 0 on n + 0.5.
 */
static int
round_to_even(float val)
{
   int rounded = IROUND(val);

   if (val - floor(val) == 0.5) {
      if (rounded % 2 != 0)
	 rounded += val > 0 ? -1 : 1;
   }

   return rounded;
}

static float
dot(ir_constant *op0, ir_constant *op1)
{
   assert(op0->type->is_float() && op1->type->is_float());

   float result = 0;
   for (unsigned c = 0; c < op0->type->components(); c++)
      result += op0->value.f[c] * op1->value.f[c];

   return result;
}

ir_constant *
ir_expression::constant_expression_value()
{
   if (this->type->is_error())
      return NULL;

   ir_constant *op[Elements(this->operands)] = { NULL, };
   ir_constant_data data;

   memset(&data, 0, sizeof(data));

   for (unsigned operand = 0; operand < this->get_num_operands(); operand++) {
      op[operand] = this->operands[operand]->constant_expression_value();
      if (!op[operand])
	 return NULL;
   }

   if (op[1] != NULL)
      assert(op[0]->type->base_type == op[1]->type->base_type ||
	     this->operation == ir_binop_lshift ||
	     this->operation == ir_binop_rshift);

   bool op0_scalar = op[0]->type->is_scalar();
   bool op1_scalar = op[1] != NULL && op[1]->type->is_scalar();

   /* When iterating over a vector or matrix's components, we want to increase
    * the loop counter.  However, for scalars, we want to stay at 0.
    */
   unsigned c0_inc = op0_scalar ? 0 : 1;
   unsigned c1_inc = op1_scalar ? 0 : 1;
   unsigned components;
   if (op1_scalar || !op[1]) {
      components = op[0]->type->components();
   } else {
      components = op[1]->type->components();
   }

   void *ctx = ralloc_parent(this);

   /* Handle array operations here, rather than below. */
   if (op[0]->type->is_array()) {
      assert(op[1] != NULL && op[1]->type->is_array());
      switch (this->operation) {
      case ir_binop_all_equal:
	 return new(ctx) ir_constant(op[0]->has_value(op[1]));
      case ir_binop_any_nequal:
	 return new(ctx) ir_constant(!op[0]->has_value(op[1]));
      default:
	 break;
      }
      return NULL;
   }

   switch (this->operation) {
   case ir_unop_bit_not:
       switch (op[0]->type->base_type) {
       case GLSL_TYPE_INT:
           for (unsigned c = 0; c < components; c++)
               data.i[c] = ~ op[0]->value.i[c];
           break;
       case GLSL_TYPE_UINT:
           for (unsigned c = 0; c < components; c++)
               data.u[c] = ~ op[0]->value.u[c];
           break;
       default:
           assert(0);
       }
       break;

   case ir_unop_logic_not:
      assert(op[0]->type->base_type == GLSL_TYPE_BOOL);
      for (unsigned c = 0; c < op[0]->type->components(); c++)
	 data.b[c] = !op[0]->value.b[c];
      break;

   case ir_unop_f2i:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 data.i[c] = (int) op[0]->value.f[c];
      }
      break;
   case ir_unop_i2f:
      assert(op[0]->type->base_type == GLSL_TYPE_INT);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 data.f[c] = (float) op[0]->value.i[c];
      }
      break;
   case ir_unop_u2f:
      assert(op[0]->type->base_type == GLSL_TYPE_UINT);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 data.f[c] = (float) op[0]->value.u[c];
      }
      break;
   case ir_unop_b2f:
      assert(op[0]->type->base_type == GLSL_TYPE_BOOL);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 data.f[c] = op[0]->value.b[c] ? 1.0F : 0.0F;
      }
      break;
   case ir_unop_f2b:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 data.b[c] = op[0]->value.f[c] != 0.0F ? true : false;
      }
      break;
   case ir_unop_b2i:
      assert(op[0]->type->base_type == GLSL_TYPE_BOOL);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 data.u[c] = op[0]->value.b[c] ? 1 : 0;
      }
      break;
   case ir_unop_i2b:
      assert(op[0]->type->is_integer());
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 data.b[c] = op[0]->value.u[c] ? true : false;
      }
      break;
   case ir_unop_u2i:
      assert(op[0]->type->base_type == GLSL_TYPE_UINT);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 data.i[c] = op[0]->value.u[c];
      }
      break;
   case ir_unop_i2u:
      assert(op[0]->type->base_type == GLSL_TYPE_INT);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 data.u[c] = op[0]->value.i[c];
      }
      break;
   case ir_unop_any:
      assert(op[0]->type->is_boolean());
      data.b[0] = false;
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 if (op[0]->value.b[c])
	    data.b[0] = true;
      }
      break;

   case ir_unop_trunc:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 data.f[c] = truncf(op[0]->value.f[c]);
      }
      break;

   case ir_unop_round_even:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 data.f[c] = round_to_even(op[0]->value.f[c]);
      }
      break;

   case ir_unop_ceil:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 data.f[c] = ceilf(op[0]->value.f[c]);
      }
      break;

   case ir_unop_floor:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 data.f[c] = floorf(op[0]->value.f[c]);
      }
      break;

   case ir_unop_fract:
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 switch (this->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.u[c] = 0;
	    break;
	 case GLSL_TYPE_INT:
	    data.i[c] = 0;
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.f[c] = op[0]->value.f[c] - floor(op[0]->value.f[c]);
	    break;
	 default:
	    assert(0);
	 }
      }
      break;

   case ir_unop_sin:
   case ir_unop_sin_reduced:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 data.f[c] = sinf(op[0]->value.f[c]);
      }
      break;

   case ir_unop_cos:
   case ir_unop_cos_reduced:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 data.f[c] = cosf(op[0]->value.f[c]);
      }
      break;

   case ir_unop_neg:
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 switch (this->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.u[c] = -((int) op[0]->value.u[c]);
	    break;
	 case GLSL_TYPE_INT:
	    data.i[c] = -op[0]->value.i[c];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.f[c] = -op[0]->value.f[c];
	    break;
	 default:
	    assert(0);
	 }
      }
      break;

   case ir_unop_abs:
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 switch (this->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.u[c] = op[0]->value.u[c];
	    break;
	 case GLSL_TYPE_INT:
	    data.i[c] = op[0]->value.i[c];
	    if (data.i[c] < 0)
	       data.i[c] = -data.i[c];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.f[c] = fabs(op[0]->value.f[c]);
	    break;
	 default:
	    assert(0);
	 }
      }
      break;

   case ir_unop_sign:
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 switch (this->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.u[c] = op[0]->value.i[c] > 0;
	    break;
	 case GLSL_TYPE_INT:
	    data.i[c] = (op[0]->value.i[c] > 0) - (op[0]->value.i[c] < 0);
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.f[c] = float((op[0]->value.f[c] > 0)-(op[0]->value.f[c] < 0));
	    break;
	 default:
	    assert(0);
	 }
      }
      break;

   case ir_unop_rcp:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 switch (this->type->base_type) {
	 case GLSL_TYPE_UINT:
	    if (op[0]->value.u[c] != 0.0)
	       data.u[c] = 1 / op[0]->value.u[c];
	    break;
	 case GLSL_TYPE_INT:
	    if (op[0]->value.i[c] != 0.0)
	       data.i[c] = 1 / op[0]->value.i[c];
	    break;
	 case GLSL_TYPE_FLOAT:
	    if (op[0]->value.f[c] != 0.0)
	       data.f[c] = 1.0F / op[0]->value.f[c];
	    break;
	 default:
	    assert(0);
	 }
      }
      break;

   case ir_unop_rsq:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 data.f[c] = 1.0F / sqrtf(op[0]->value.f[c]);
      }
      break;

   case ir_unop_sqrt:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 data.f[c] = sqrtf(op[0]->value.f[c]);
      }
      break;

   case ir_unop_exp:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 data.f[c] = expf(op[0]->value.f[c]);
      }
      break;

   case ir_unop_exp2:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 data.f[c] = exp2f(op[0]->value.f[c]);
      }
      break;

   case ir_unop_log:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 data.f[c] = logf(op[0]->value.f[c]);
      }
      break;

   case ir_unop_log2:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 data.f[c] = log2f(op[0]->value.f[c]);
      }
      break;

   case ir_unop_dFdx:
   case ir_unop_dFdy:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 data.f[c] = 0.0;
      }
      break;

   case ir_binop_pow:
      assert(op[0]->type->base_type == GLSL_TYPE_FLOAT);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 data.f[c] = powf(op[0]->value.f[c], op[1]->value.f[c]);
      }
      break;

   case ir_binop_dot:
      data.f[0] = dot(op[0], op[1]);
      break;

   case ir_binop_min:
      assert(op[0]->type == op[1]->type || op0_scalar || op1_scalar);
      for (unsigned c = 0, c0 = 0, c1 = 0;
	   c < components;
	   c0 += c0_inc, c1 += c1_inc, c++) {

	 switch (op[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.u[c] = MIN2(op[0]->value.u[c0], op[1]->value.u[c1]);
	    break;
	 case GLSL_TYPE_INT:
	    data.i[c] = MIN2(op[0]->value.i[c0], op[1]->value.i[c1]);
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.f[c] = MIN2(op[0]->value.f[c0], op[1]->value.f[c1]);
	    break;
	 default:
	    assert(0);
	 }
      }

      break;
   case ir_binop_max:
      assert(op[0]->type == op[1]->type || op0_scalar || op1_scalar);
      for (unsigned c = 0, c0 = 0, c1 = 0;
	   c < components;
	   c0 += c0_inc, c1 += c1_inc, c++) {

	 switch (op[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.u[c] = MAX2(op[0]->value.u[c0], op[1]->value.u[c1]);
	    break;
	 case GLSL_TYPE_INT:
	    data.i[c] = MAX2(op[0]->value.i[c0], op[1]->value.i[c1]);
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.f[c] = MAX2(op[0]->value.f[c0], op[1]->value.f[c1]);
	    break;
	 default:
	    assert(0);
	 }
      }
      break;

   case ir_binop_add:
      assert(op[0]->type == op[1]->type || op0_scalar || op1_scalar);
      for (unsigned c = 0, c0 = 0, c1 = 0;
	   c < components;
	   c0 += c0_inc, c1 += c1_inc, c++) {

	 switch (op[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.u[c] = op[0]->value.u[c0] + op[1]->value.u[c1];
	    break;
	 case GLSL_TYPE_INT:
	    data.i[c] = op[0]->value.i[c0] + op[1]->value.i[c1];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.f[c] = op[0]->value.f[c0] + op[1]->value.f[c1];
	    break;
	 default:
	    assert(0);
	 }
      }

      break;
   case ir_binop_sub:
      assert(op[0]->type == op[1]->type || op0_scalar || op1_scalar);
      for (unsigned c = 0, c0 = 0, c1 = 0;
	   c < components;
	   c0 += c0_inc, c1 += c1_inc, c++) {

	 switch (op[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.u[c] = op[0]->value.u[c0] - op[1]->value.u[c1];
	    break;
	 case GLSL_TYPE_INT:
	    data.i[c] = op[0]->value.i[c0] - op[1]->value.i[c1];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.f[c] = op[0]->value.f[c0] - op[1]->value.f[c1];
	    break;
	 default:
	    assert(0);
	 }
      }

      break;
   case ir_binop_mul:
      /* Check for equal types, or unequal types involving scalars */
      if ((op[0]->type == op[1]->type && !op[0]->type->is_matrix())
	  || op0_scalar || op1_scalar) {
	 for (unsigned c = 0, c0 = 0, c1 = 0;
	      c < components;
	      c0 += c0_inc, c1 += c1_inc, c++) {

	    switch (op[0]->type->base_type) {
	    case GLSL_TYPE_UINT:
	       data.u[c] = op[0]->value.u[c0] * op[1]->value.u[c1];
	       break;
	    case GLSL_TYPE_INT:
	       data.i[c] = op[0]->value.i[c0] * op[1]->value.i[c1];
	       break;
	    case GLSL_TYPE_FLOAT:
	       data.f[c] = op[0]->value.f[c0] * op[1]->value.f[c1];
	       break;
	    default:
	       assert(0);
	    }
	 }
      } else {
	 assert(op[0]->type->is_matrix() || op[1]->type->is_matrix());

	 /* Multiply an N-by-M matrix with an M-by-P matrix.  Since either
	  * matrix can be a GLSL vector, either N or P can be 1.
	  *
	  * For vec*mat, the vector is treated as a row vector.  This
	  * means the vector is a 1-row x M-column matrix.
	  *
	  * For mat*vec, the vector is treated as a column vector.  Since
	  * matrix_columns is 1 for vectors, this just works.
	  */
	 const unsigned n = op[0]->type->is_vector()
	    ? 1 : op[0]->type->vector_elements;
	 const unsigned m = op[1]->type->vector_elements;
	 const unsigned p = op[1]->type->matrix_columns;
	 for (unsigned j = 0; j < p; j++) {
	    for (unsigned i = 0; i < n; i++) {
	       for (unsigned k = 0; k < m; k++) {
		  data.f[i+n*j] += op[0]->value.f[i+n*k]*op[1]->value.f[k+m*j];
	       }
	    }
	 }
      }

      break;
   case ir_binop_div:
      /* FINISHME: Emit warning when division-by-zero is detected. */
      assert(op[0]->type == op[1]->type || op0_scalar || op1_scalar);
      for (unsigned c = 0, c0 = 0, c1 = 0;
	   c < components;
	   c0 += c0_inc, c1 += c1_inc, c++) {

	 switch (op[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    if (op[1]->value.u[c1] == 0) {
	       data.u[c] = 0;
	    } else {
	       data.u[c] = op[0]->value.u[c0] / op[1]->value.u[c1];
	    }
	    break;
	 case GLSL_TYPE_INT:
	    if (op[1]->value.i[c1] == 0) {
	       data.i[c] = 0;
	    } else {
	       data.i[c] = op[0]->value.i[c0] / op[1]->value.i[c1];
	    }
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.f[c] = op[0]->value.f[c0] / op[1]->value.f[c1];
	    break;
	 default:
	    assert(0);
	 }
      }

      break;
   case ir_binop_mod:
      /* FINISHME: Emit warning when division-by-zero is detected. */
      assert(op[0]->type == op[1]->type || op0_scalar || op1_scalar);
      for (unsigned c = 0, c0 = 0, c1 = 0;
	   c < components;
	   c0 += c0_inc, c1 += c1_inc, c++) {

	 switch (op[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    if (op[1]->value.u[c1] == 0) {
	       data.u[c] = 0;
	    } else {
	       data.u[c] = op[0]->value.u[c0] % op[1]->value.u[c1];
	    }
	    break;
	 case GLSL_TYPE_INT:
	    if (op[1]->value.i[c1] == 0) {
	       data.i[c] = 0;
	    } else {
	       data.i[c] = op[0]->value.i[c0] % op[1]->value.i[c1];
	    }
	    break;
	 case GLSL_TYPE_FLOAT:
	    /* We don't use fmod because it rounds toward zero; GLSL specifies
	     * the use of floor.
	     */
	    data.f[c] = op[0]->value.f[c0] - op[1]->value.f[c1]
	       * floorf(op[0]->value.f[c0] / op[1]->value.f[c1]);
	    break;
	 default:
	    assert(0);
	 }
      }

      break;

   case ir_binop_logic_and:
      assert(op[0]->type->base_type == GLSL_TYPE_BOOL);
      for (unsigned c = 0; c < op[0]->type->components(); c++)
	 data.b[c] = op[0]->value.b[c] && op[1]->value.b[c];
      break;
   case ir_binop_logic_xor:
      assert(op[0]->type->base_type == GLSL_TYPE_BOOL);
      for (unsigned c = 0; c < op[0]->type->components(); c++)
	 data.b[c] = op[0]->value.b[c] ^ op[1]->value.b[c];
      break;
   case ir_binop_logic_or:
      assert(op[0]->type->base_type == GLSL_TYPE_BOOL);
      for (unsigned c = 0; c < op[0]->type->components(); c++)
	 data.b[c] = op[0]->value.b[c] || op[1]->value.b[c];
      break;

   case ir_binop_less:
      assert(op[0]->type == op[1]->type);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 switch (op[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.b[c] = op[0]->value.u[c] < op[1]->value.u[c];
	    break;
	 case GLSL_TYPE_INT:
	    data.b[c] = op[0]->value.i[c] < op[1]->value.i[c];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.b[c] = op[0]->value.f[c] < op[1]->value.f[c];
	    break;
	 default:
	    assert(0);
	 }
      }
      break;
   case ir_binop_greater:
      assert(op[0]->type == op[1]->type);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 switch (op[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.b[c] = op[0]->value.u[c] > op[1]->value.u[c];
	    break;
	 case GLSL_TYPE_INT:
	    data.b[c] = op[0]->value.i[c] > op[1]->value.i[c];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.b[c] = op[0]->value.f[c] > op[1]->value.f[c];
	    break;
	 default:
	    assert(0);
	 }
      }
      break;
   case ir_binop_lequal:
      assert(op[0]->type == op[1]->type);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 switch (op[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.b[c] = op[0]->value.u[c] <= op[1]->value.u[c];
	    break;
	 case GLSL_TYPE_INT:
	    data.b[c] = op[0]->value.i[c] <= op[1]->value.i[c];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.b[c] = op[0]->value.f[c] <= op[1]->value.f[c];
	    break;
	 default:
	    assert(0);
	 }
      }
      break;
   case ir_binop_gequal:
      assert(op[0]->type == op[1]->type);
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 switch (op[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.b[c] = op[0]->value.u[c] >= op[1]->value.u[c];
	    break;
	 case GLSL_TYPE_INT:
	    data.b[c] = op[0]->value.i[c] >= op[1]->value.i[c];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.b[c] = op[0]->value.f[c] >= op[1]->value.f[c];
	    break;
	 default:
	    assert(0);
	 }
      }
      break;
   case ir_binop_equal:
      assert(op[0]->type == op[1]->type);
      for (unsigned c = 0; c < components; c++) {
	 switch (op[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.b[c] = op[0]->value.u[c] == op[1]->value.u[c];
	    break;
	 case GLSL_TYPE_INT:
	    data.b[c] = op[0]->value.i[c] == op[1]->value.i[c];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.b[c] = op[0]->value.f[c] == op[1]->value.f[c];
	    break;
	 case GLSL_TYPE_BOOL:
	    data.b[c] = op[0]->value.b[c] == op[1]->value.b[c];
	    break;
	 default:
	    assert(0);
	 }
      }
      break;
   case ir_binop_nequal:
      assert(op[0]->type == op[1]->type);
      for (unsigned c = 0; c < components; c++) {
	 switch (op[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.b[c] = op[0]->value.u[c] != op[1]->value.u[c];
	    break;
	 case GLSL_TYPE_INT:
	    data.b[c] = op[0]->value.i[c] != op[1]->value.i[c];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.b[c] = op[0]->value.f[c] != op[1]->value.f[c];
	    break;
	 case GLSL_TYPE_BOOL:
	    data.b[c] = op[0]->value.b[c] != op[1]->value.b[c];
	    break;
	 default:
	    assert(0);
	 }
      }
      break;
   case ir_binop_all_equal:
      data.b[0] = op[0]->has_value(op[1]);
      break;
   case ir_binop_any_nequal:
      data.b[0] = !op[0]->has_value(op[1]);
      break;

   case ir_binop_lshift:
      for (unsigned c = 0, c0 = 0, c1 = 0;
           c < components;
           c0 += c0_inc, c1 += c1_inc, c++) {

          if (op[0]->type->base_type == GLSL_TYPE_INT &&
              op[1]->type->base_type == GLSL_TYPE_INT) {
              data.i[c] = op[0]->value.i[c0] << op[1]->value.i[c1];

          } else if (op[0]->type->base_type == GLSL_TYPE_INT &&
                     op[1]->type->base_type == GLSL_TYPE_UINT) {
              data.i[c] = op[0]->value.i[c0] << op[1]->value.u[c1];

          } else if (op[0]->type->base_type == GLSL_TYPE_UINT &&
                     op[1]->type->base_type == GLSL_TYPE_INT) {
              data.u[c] = op[0]->value.u[c0] << op[1]->value.i[c1];

          } else if (op[0]->type->base_type == GLSL_TYPE_UINT &&
                     op[1]->type->base_type == GLSL_TYPE_UINT) {
              data.u[c] = op[0]->value.u[c0] << op[1]->value.u[c1];
          }
      }
      break;

   case ir_binop_rshift:
       for (unsigned c = 0, c0 = 0, c1 = 0;
            c < components;
            c0 += c0_inc, c1 += c1_inc, c++) {

           if (op[0]->type->base_type == GLSL_TYPE_INT &&
               op[1]->type->base_type == GLSL_TYPE_INT) {
               data.i[c] = op[0]->value.i[c0] >> op[1]->value.i[c1];

           } else if (op[0]->type->base_type == GLSL_TYPE_INT &&
                      op[1]->type->base_type == GLSL_TYPE_UINT) {
               data.i[c] = op[0]->value.i[c0] >> op[1]->value.u[c1];

           } else if (op[0]->type->base_type == GLSL_TYPE_UINT &&
                      op[1]->type->base_type == GLSL_TYPE_INT) {
               data.u[c] = op[0]->value.u[c0] >> op[1]->value.i[c1];

           } else if (op[0]->type->base_type == GLSL_TYPE_UINT &&
                      op[1]->type->base_type == GLSL_TYPE_UINT) {
               data.u[c] = op[0]->value.u[c0] >> op[1]->value.u[c1];
           }
       }
       break;

   case ir_binop_bit_and:
      for (unsigned c = 0, c0 = 0, c1 = 0;
           c < components;
           c0 += c0_inc, c1 += c1_inc, c++) {

          switch (op[0]->type->base_type) {
          case GLSL_TYPE_INT:
              data.i[c] = op[0]->value.i[c0] & op[1]->value.i[c1];
              break;
          case GLSL_TYPE_UINT:
              data.u[c] = op[0]->value.u[c0] & op[1]->value.u[c1];
              break;
          default:
              assert(0);
          }
      }
      break;

   case ir_binop_bit_or:
      for (unsigned c = 0, c0 = 0, c1 = 0;
           c < components;
           c0 += c0_inc, c1 += c1_inc, c++) {

          switch (op[0]->type->base_type) {
          case GLSL_TYPE_INT:
              data.i[c] = op[0]->value.i[c0] | op[1]->value.i[c1];
              break;
          case GLSL_TYPE_UINT:
              data.u[c] = op[0]->value.u[c0] | op[1]->value.u[c1];
              break;
          default:
              assert(0);
          }
      }
      break;

   case ir_binop_bit_xor:
      for (unsigned c = 0, c0 = 0, c1 = 0;
           c < components;
           c0 += c0_inc, c1 += c1_inc, c++) {

          switch (op[0]->type->base_type) {
          case GLSL_TYPE_INT:
              data.i[c] = op[0]->value.i[c0] ^ op[1]->value.i[c1];
              break;
          case GLSL_TYPE_UINT:
              data.u[c] = op[0]->value.u[c0] ^ op[1]->value.u[c1];
              break;
          default:
              assert(0);
          }
      }
      break;

   case ir_quadop_vector:
      for (unsigned c = 0; c < this->type->vector_elements; c++) {
	 switch (this->type->base_type) {
	 case GLSL_TYPE_INT:
	    data.i[c] = op[c]->value.i[0];
	    break;
	 case GLSL_TYPE_UINT:
	    data.u[c] = op[c]->value.u[0];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.f[c] = op[c]->value.f[0];
	    break;
	 default:
	    assert(0);
	 }
      }
      break;

   default:
      /* FINISHME: Should handle all expression types. */
      return NULL;
   }

   return new(ctx) ir_constant(this->type, &data);
}


ir_constant *
ir_texture::constant_expression_value()
{
   /* texture lookups aren't constant expressions */
   return NULL;
}


ir_constant *
ir_swizzle::constant_expression_value()
{
   ir_constant *v = this->val->constant_expression_value();

   if (v != NULL) {
      ir_constant_data data = { { 0 } };

      const unsigned swiz_idx[4] = {
	 this->mask.x, this->mask.y, this->mask.z, this->mask.w
      };

      for (unsigned i = 0; i < this->mask.num_components; i++) {
	 switch (v->type->base_type) {
	 case GLSL_TYPE_UINT:
	 case GLSL_TYPE_INT:   data.u[i] = v->value.u[swiz_idx[i]]; break;
	 case GLSL_TYPE_FLOAT: data.f[i] = v->value.f[swiz_idx[i]]; break;
	 case GLSL_TYPE_BOOL:  data.b[i] = v->value.b[swiz_idx[i]]; break;
	 default:              assert(!"Should not get here."); break;
	 }
      }

      void *ctx = ralloc_parent(this);
      return new(ctx) ir_constant(this->type, &data);
   }
   return NULL;
}


ir_constant *
ir_dereference_variable::constant_expression_value()
{
   /* This may occur during compile and var->type is glsl_type::error_type */
   if (!var)
      return NULL;

   /* The constant_value of a uniform variable is its initializer,
    * not the lifetime constant value of the uniform.
    */
   if (var->mode == ir_var_uniform)
      return NULL;

   if (!var->constant_value)
      return NULL;

   return var->constant_value->clone(ralloc_parent(var), NULL);
}


ir_constant *
ir_dereference_array::constant_expression_value()
{
   ir_constant *array = this->array->constant_expression_value();
   ir_constant *idx = this->array_index->constant_expression_value();

   if ((array != NULL) && (idx != NULL)) {
      void *ctx = ralloc_parent(this);
      if (array->type->is_matrix()) {
	 /* Array access of a matrix results in a vector.
	  */
	 const unsigned column = idx->value.u[0];

	 const glsl_type *const column_type = array->type->column_type();

	 /* Offset in the constant matrix to the first element of the column
	  * to be extracted.
	  */
	 const unsigned mat_idx = column * column_type->vector_elements;

	 ir_constant_data data = { { 0 } };

	 switch (column_type->base_type) {
	 case GLSL_TYPE_UINT:
	 case GLSL_TYPE_INT:
	    for (unsigned i = 0; i < column_type->vector_elements; i++)
	       data.u[i] = array->value.u[mat_idx + i];

	    break;

	 case GLSL_TYPE_FLOAT:
	    for (unsigned i = 0; i < column_type->vector_elements; i++)
	       data.f[i] = array->value.f[mat_idx + i];

	    break;

	 default:
	    assert(!"Should not get here.");
	    break;
	 }

	 return new(ctx) ir_constant(column_type, &data);
      } else if (array->type->is_vector()) {
	 const unsigned component = idx->value.u[0];

	 return new(ctx) ir_constant(array, component);
      } else {
	 const unsigned index = idx->value.u[0];
	 return array->get_array_element(index)->clone(ctx, NULL);
      }
   }
   return NULL;
}


ir_constant *
ir_dereference_record::constant_expression_value()
{
   ir_constant *v = this->record->constant_expression_value();

   return (v != NULL) ? v->get_record_field(this->field) : NULL;
}


ir_constant *
ir_assignment::constant_expression_value()
{
   /* FINISHME: Handle CEs involving assignment (return RHS) */
   return NULL;
}


ir_constant *
ir_constant::constant_expression_value()
{
   return this;
}


ir_constant *
ir_call::constant_expression_value()
{
   if (this->type == glsl_type::error_type)
      return NULL;

   /* From the GLSL 1.20 spec, page 23:
    * "Function calls to user-defined functions (non-built-in functions)
    *  cannot be used to form constant expressions."
    */
   if (!this->callee->is_builtin)
      return NULL;

   unsigned num_parameters = 0;

   /* Check if all parameters are constant */
   ir_constant *op[3];
   foreach_list(n, &this->actual_parameters) {
      ir_constant *constant = ((ir_rvalue *) n)->constant_expression_value();
      if (constant == NULL)
	 return NULL;

      op[num_parameters] = constant;

      assert(num_parameters < 3);
      num_parameters++;
   }

   /* Individual cases below can either:
    * - Assign "expr" a new ir_expression to evaluate (for basic opcodes)
    * - Fill "data" with appopriate constant data
    * - Return an ir_constant directly.
    */
   void *mem_ctx = ralloc_parent(this);
   ir_expression *expr = NULL;

   ir_constant_data data;
   memset(&data, 0, sizeof(data));

   const char *callee = this->callee_name();
   if (strcmp(callee, "abs") == 0) {
      expr = new(mem_ctx) ir_expression(ir_unop_abs, type, op[0], NULL);
   } else if (strcmp(callee, "all") == 0) {
      assert(op[0]->type->is_boolean());
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 if (!op[0]->value.b[c])
	    return new(mem_ctx) ir_constant(false);
      }
      return new(mem_ctx) ir_constant(true);
   } else if (strcmp(callee, "any") == 0) {
      assert(op[0]->type->is_boolean());
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 if (op[0]->value.b[c])
	    return new(mem_ctx) ir_constant(true);
      }
      return new(mem_ctx) ir_constant(false);
   } else if (strcmp(callee, "acos") == 0) {
      assert(op[0]->type->is_float());
      for (unsigned c = 0; c < op[0]->type->components(); c++)
	 data.f[c] = acosf(op[0]->value.f[c]);
   } else if (strcmp(callee, "acosh") == 0) {
      assert(op[0]->type->is_float());
      for (unsigned c = 0; c < op[0]->type->components(); c++)
	 data.f[c] = acoshf(op[0]->value.f[c]);
   } else if (strcmp(callee, "asin") == 0) {
      assert(op[0]->type->is_float());
      for (unsigned c = 0; c < op[0]->type->components(); c++)
	 data.f[c] = asinf(op[0]->value.f[c]);
   } else if (strcmp(callee, "asinh") == 0) {
      assert(op[0]->type->is_float());
      for (unsigned c = 0; c < op[0]->type->components(); c++)
	 data.f[c] = asinhf(op[0]->value.f[c]);
   } else if (strcmp(callee, "atan") == 0) {
      assert(op[0]->type->is_float());
      if (num_parameters == 2) {
	 assert(op[1]->type->is_float());
	 for (unsigned c = 0; c < op[0]->type->components(); c++)
	    data.f[c] = atan2f(op[0]->value.f[c], op[1]->value.f[c]);
      } else {
	 for (unsigned c = 0; c < op[0]->type->components(); c++)
	    data.f[c] = atanf(op[0]->value.f[c]);
      }
   } else if (strcmp(callee, "atanh") == 0) {
      assert(op[0]->type->is_float());
      for (unsigned c = 0; c < op[0]->type->components(); c++)
	 data.f[c] = atanhf(op[0]->value.f[c]);
   } else if (strcmp(callee, "dFdx") == 0 || strcmp(callee, "dFdy") == 0) {
      return ir_constant::zero(mem_ctx, this->type);
   } else if (strcmp(callee, "ceil") == 0) {
      expr = new(mem_ctx) ir_expression(ir_unop_ceil, type, op[0], NULL);
   } else if (strcmp(callee, "clamp") == 0) {
      assert(num_parameters == 3);
      unsigned c1_inc = op[1]->type->is_scalar() ? 0 : 1;
      unsigned c2_inc = op[2]->type->is_scalar() ? 0 : 1;
      for (unsigned c = 0, c1 = 0, c2 = 0;
	   c < op[0]->type->components();
	   c1 += c1_inc, c2 += c2_inc, c++) {

	 switch (op[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.u[c] = CLAMP(op[0]->value.u[c], op[1]->value.u[c1],
			      op[2]->value.u[c2]);
	    break;
	 case GLSL_TYPE_INT:
	    data.i[c] = CLAMP(op[0]->value.i[c], op[1]->value.i[c1],
			      op[2]->value.i[c2]);
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.f[c] = CLAMP(op[0]->value.f[c], op[1]->value.f[c1],
			      op[2]->value.f[c2]);
	    break;
	 default:
	    assert(!"Should not get here.");
	 }
      }
   } else if (strcmp(callee, "cos") == 0) {
      expr = new(mem_ctx) ir_expression(ir_unop_cos, type, op[0], NULL);
   } else if (strcmp(callee, "cosh") == 0) {
      assert(op[0]->type->is_float());
      for (unsigned c = 0; c < op[0]->type->components(); c++)
	 data.f[c] = coshf(op[0]->value.f[c]);
   } else if (strcmp(callee, "cross") == 0) {
      assert(op[0]->type == glsl_type::vec3_type);
      assert(op[1]->type == glsl_type::vec3_type);
      data.f[0] = (op[0]->value.f[1] * op[1]->value.f[2] -
		   op[1]->value.f[1] * op[0]->value.f[2]);
      data.f[1] = (op[0]->value.f[2] * op[1]->value.f[0] -
		   op[1]->value.f[2] * op[0]->value.f[0]);
      data.f[2] = (op[0]->value.f[0] * op[1]->value.f[1] -
		   op[1]->value.f[0] * op[0]->value.f[1]);
   } else if (strcmp(callee, "degrees") == 0) {
      assert(op[0]->type->is_float());
      for (unsigned c = 0; c < op[0]->type->components(); c++)
	 data.f[c] = 180.0F / M_PI * op[0]->value.f[c];
   } else if (strcmp(callee, "distance") == 0) {
      assert(op[0]->type->is_float() && op[1]->type->is_float());
      float length_squared = 0.0;
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 float t = op[0]->value.f[c] - op[1]->value.f[c];
	 length_squared += t * t;
      }
      return new(mem_ctx) ir_constant(sqrtf(length_squared));
   } else if (strcmp(callee, "dot") == 0) {
      return new(mem_ctx) ir_constant(dot(op[0], op[1]));
   } else if (strcmp(callee, "equal") == 0) {
      assert(op[0]->type->is_vector() && op[1] && op[1]->type->is_vector());
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 switch (op[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.b[c] = op[0]->value.u[c] == op[1]->value.u[c];
	    break;
	 case GLSL_TYPE_INT:
	    data.b[c] = op[0]->value.i[c] == op[1]->value.i[c];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.b[c] = op[0]->value.f[c] == op[1]->value.f[c];
	    break;
	 case GLSL_TYPE_BOOL:
	    data.b[c] = op[0]->value.b[c] == op[1]->value.b[c];
	    break;
	 default:
	    assert(!"Should not get here.");
	 }
      }
   } else if (strcmp(callee, "exp") == 0) {
      expr = new(mem_ctx) ir_expression(ir_unop_exp, type, op[0], NULL);
   } else if (strcmp(callee, "exp2") == 0) {
      expr = new(mem_ctx) ir_expression(ir_unop_exp2, type, op[0], NULL);
   } else if (strcmp(callee, "faceforward") == 0) {
      if (dot(op[2], op[1]) < 0)
	 return op[0];
      for (unsigned c = 0; c < op[0]->type->components(); c++)
	 data.f[c] = -op[0]->value.f[c];
   } else if (strcmp(callee, "floor") == 0) {
      expr = new(mem_ctx) ir_expression(ir_unop_floor, type, op[0], NULL);
   } else if (strcmp(callee, "fract") == 0) {
      expr = new(mem_ctx) ir_expression(ir_unop_fract, type, op[0], NULL);
   } else if (strcmp(callee, "fwidth") == 0) {
      return ir_constant::zero(mem_ctx, this->type);
   } else if (strcmp(callee, "greaterThan") == 0) {
      assert(op[0]->type->is_vector() && op[1] && op[1]->type->is_vector());
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 switch (op[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.b[c] = op[0]->value.u[c] > op[1]->value.u[c];
	    break;
	 case GLSL_TYPE_INT:
	    data.b[c] = op[0]->value.i[c] > op[1]->value.i[c];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.b[c] = op[0]->value.f[c] > op[1]->value.f[c];
	    break;
	 default:
	    assert(!"Should not get here.");
	 }
      }
   } else if (strcmp(callee, "greaterThanEqual") == 0) {
      assert(op[0]->type->is_vector() && op[1] && op[1]->type->is_vector());
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 switch (op[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.b[c] = op[0]->value.u[c] >= op[1]->value.u[c];
	    break;
	 case GLSL_TYPE_INT:
	    data.b[c] = op[0]->value.i[c] >= op[1]->value.i[c];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.b[c] = op[0]->value.f[c] >= op[1]->value.f[c];
	    break;
	 default:
	    assert(!"Should not get here.");
	 }
      }
   } else if (strcmp(callee, "inversesqrt") == 0) {
      expr = new(mem_ctx) ir_expression(ir_unop_rsq, type, op[0], NULL);
   } else if (strcmp(callee, "length") == 0) {
      return new(mem_ctx) ir_constant(sqrtf(dot(op[0], op[0])));
   } else if (strcmp(callee, "lessThan") == 0) {
      assert(op[0]->type->is_vector() && op[1] && op[1]->type->is_vector());
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 switch (op[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.b[c] = op[0]->value.u[c] < op[1]->value.u[c];
	    break;
	 case GLSL_TYPE_INT:
	    data.b[c] = op[0]->value.i[c] < op[1]->value.i[c];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.b[c] = op[0]->value.f[c] < op[1]->value.f[c];
	    break;
	 default:
	    assert(!"Should not get here.");
	 }
      }
   } else if (strcmp(callee, "lessThanEqual") == 0) {
      assert(op[0]->type->is_vector() && op[1] && op[1]->type->is_vector());
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 switch (op[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.b[c] = op[0]->value.u[c] <= op[1]->value.u[c];
	    break;
	 case GLSL_TYPE_INT:
	    data.b[c] = op[0]->value.i[c] <= op[1]->value.i[c];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.b[c] = op[0]->value.f[c] <= op[1]->value.f[c];
	    break;
	 default:
	    assert(!"Should not get here.");
	 }
      }
   } else if (strcmp(callee, "log") == 0) {
      expr = new(mem_ctx) ir_expression(ir_unop_log, type, op[0], NULL);
   } else if (strcmp(callee, "log2") == 0) {
      expr = new(mem_ctx) ir_expression(ir_unop_log2, type, op[0], NULL);
   } else if (strcmp(callee, "matrixCompMult") == 0) {
      assert(op[0]->type->is_float() && op[1]->type->is_float());
      for (unsigned c = 0; c < op[0]->type->components(); c++)
	 data.f[c] = op[0]->value.f[c] * op[1]->value.f[c];
   } else if (strcmp(callee, "max") == 0) {
      expr = new(mem_ctx) ir_expression(ir_binop_max, type, op[0], op[1]);
   } else if (strcmp(callee, "min") == 0) {
      expr = new(mem_ctx) ir_expression(ir_binop_min, type, op[0], op[1]);
   } else if (strcmp(callee, "mix") == 0) {
      assert(op[0]->type->is_float() && op[1]->type->is_float());
      if (op[2]->type->is_float()) {
	 unsigned c2_inc = op[2]->type->is_scalar() ? 0 : 1;
	 unsigned components = op[0]->type->components();
	 for (unsigned c = 0, c2 = 0; c < components; c2 += c2_inc, c++) {
	    data.f[c] = op[0]->value.f[c] * (1 - op[2]->value.f[c2]) +
			op[1]->value.f[c] * op[2]->value.f[c2];
	 }
      } else {
	 assert(op[2]->type->is_boolean());
	 for (unsigned c = 0; c < op[0]->type->components(); c++)
	    data.f[c] = op[op[2]->value.b[c] ? 1 : 0]->value.f[c];
      }
   } else if (strcmp(callee, "mod") == 0) {
      expr = new(mem_ctx) ir_expression(ir_binop_mod, type, op[0], op[1]);
   } else if (strcmp(callee, "normalize") == 0) {
      assert(op[0]->type->is_float());
      float length = sqrtf(dot(op[0], op[0]));

      if (length == 0)
	 return ir_constant::zero(mem_ctx, this->type);

      for (unsigned c = 0; c < op[0]->type->components(); c++)
	 data.f[c] = op[0]->value.f[c] / length;
   } else if (strcmp(callee, "not") == 0) {
      expr = new(mem_ctx) ir_expression(ir_unop_logic_not, type, op[0], NULL);
   } else if (strcmp(callee, "notEqual") == 0) {
      assert(op[0]->type->is_vector() && op[1] && op[1]->type->is_vector());
      for (unsigned c = 0; c < op[0]->type->components(); c++) {
	 switch (op[0]->type->base_type) {
	 case GLSL_TYPE_UINT:
	    data.b[c] = op[0]->value.u[c] != op[1]->value.u[c];
	    break;
	 case GLSL_TYPE_INT:
	    data.b[c] = op[0]->value.i[c] != op[1]->value.i[c];
	    break;
	 case GLSL_TYPE_FLOAT:
	    data.b[c] = op[0]->value.f[c] != op[1]->value.f[c];
	    break;
	 case GLSL_TYPE_BOOL:
	    data.b[c] = op[0]->value.b[c] != op[1]->value.b[c];
	    break;
	 default:
	    assert(!"Should not get here.");
	 }
      }
   } else if (strcmp(callee, "outerProduct") == 0) {
      assert(op[0]->type->is_vector() && op[1]->type->is_vector());
      const unsigned m = op[0]->type->vector_elements;
      const unsigned n = op[1]->type->vector_elements;
      for (unsigned j = 0; j < n; j++) {
	 for (unsigned i = 0; i < m; i++) {
	    data.f[i+m*j] = op[0]->value.f[i] * op[1]->value.f[j];
	 }
      }
   } else if (strcmp(callee, "pow") == 0) {
      expr = new(mem_ctx) ir_expression(ir_binop_pow, type, op[0], op[1]);
   } else if (strcmp(callee, "radians") == 0) {
      assert(op[0]->type->is_float());
      for (unsigned c = 0; c < op[0]->type->components(); c++)
	 data.f[c] = M_PI / 180.0F * op[0]->value.f[c];
   } else if (strcmp(callee, "reflect") == 0) {
      assert(op[0]->type->is_float());
      float dot_NI = dot(op[1], op[0]);
      for (unsigned c = 0; c < op[0]->type->components(); c++)
	 data.f[c] = op[0]->value.f[c] - 2 * dot_NI * op[1]->value.f[c];
   } else if (strcmp(callee, "refract") == 0) {
      const float eta = op[2]->value.f[0];
      const float dot_NI = dot(op[1], op[0]);
      const float k = 1.0F - eta * eta * (1.0F - dot_NI * dot_NI);
      if (k < 0.0) {
	 return ir_constant::zero(mem_ctx, this->type);
      } else {
	 for (unsigned c = 0; c < type->components(); c++) {
	    data.f[c] = eta * op[0]->value.f[c] - (eta * dot_NI + sqrtf(k))
			    * op[1]->value.f[c];
	 }
      }
   } else if (strcmp(callee, "round") == 0 ||
	      strcmp(callee, "roundEven") == 0) {
      expr = new(mem_ctx) ir_expression(ir_unop_round_even, op[0]);
   } else if (strcmp(callee, "sign") == 0) {
      expr = new(mem_ctx) ir_expression(ir_unop_sign, type, op[0], NULL);
   } else if (strcmp(callee, "sin") == 0) {
      expr = new(mem_ctx) ir_expression(ir_unop_sin, type, op[0], NULL);
   } else if (strcmp(callee, "sinh") == 0) {
      assert(op[0]->type->is_float());
      for (unsigned c = 0; c < op[0]->type->components(); c++)
	 data.f[c] = sinhf(op[0]->value.f[c]);
   } else if (strcmp(callee, "smoothstep") == 0) {
      assert(num_parameters == 3);
      assert(op[1]->type == op[0]->type);
      unsigned edge_inc = op[0]->type->is_scalar() ? 0 : 1;
      for (unsigned c = 0, e = 0; c < type->components(); e += edge_inc, c++) {
	 const float edge0 = op[0]->value.f[e];
	 const float edge1 = op[1]->value.f[e];
	 if (edge0 == edge1) {
	    data.f[c] = 0.0; /* Avoid a crash - results are undefined anyway */
	 } else {
	    const float numerator = op[2]->value.f[c] - edge0;
	    const float denominator = edge1 - edge0;
	    const float t = CLAMP(numerator/denominator, 0, 1);
	    data.f[c] = t * t * (3 - 2 * t);
	 }
      }
   } else if (strcmp(callee, "sqrt") == 0) {
      expr = new(mem_ctx) ir_expression(ir_unop_sqrt, type, op[0], NULL);
   } else if (strcmp(callee, "step") == 0) {
      assert(op[0]->type->is_float() && op[1]->type->is_float());
      /* op[0] (edge) may be either a scalar or a vector */
      const unsigned c0_inc = op[0]->type->is_scalar() ? 0 : 1;
      for (unsigned c = 0, c0 = 0; c < type->components(); c0 += c0_inc, c++)
	 data.f[c] = (op[1]->value.f[c] < op[0]->value.f[c0]) ? 0.0F : 1.0F;
   } else if (strcmp(callee, "tan") == 0) {
      assert(op[0]->type->is_float());
      for (unsigned c = 0; c < op[0]->type->components(); c++)
	 data.f[c] = tanf(op[0]->value.f[c]);
   } else if (strcmp(callee, "tanh") == 0) {
      assert(op[0]->type->is_float());
      for (unsigned c = 0; c < op[0]->type->components(); c++)
	 data.f[c] = tanhf(op[0]->value.f[c]);
   } else if (strcmp(callee, "transpose") == 0) {
      assert(op[0]->type->is_matrix());
      const unsigned n = op[0]->type->vector_elements;
      const unsigned m = op[0]->type->matrix_columns;
      for (unsigned j = 0; j < m; j++) {
	 for (unsigned i = 0; i < n; i++) {
	    data.f[m*i+j] += op[0]->value.f[i+n*j];
	 }
      }
   } else if (strcmp(callee, "trunc") == 0) {
      expr = new(mem_ctx) ir_expression(ir_unop_trunc, op[0]);
   } else {
      /* Unsupported builtin - some are not allowed in constant expressions. */
      return NULL;
   }

   if (expr != NULL)
      return expr->constant_expression_value();

   return new(mem_ctx) ir_constant(this->type, &data);
}
