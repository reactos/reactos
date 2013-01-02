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

#include "glsl_types.h"
#include "ir.h"

typedef enum {
   PARAMETER_LIST_NO_MATCH,
   PARAMETER_LIST_EXACT_MATCH,
   PARAMETER_LIST_INEXACT_MATCH /*< Match requires implicit conversion. */
} parameter_list_match_t;

/**
 * \brief Check if two parameter lists match.
 *
 * \param list_a Parameters of the function definition.
 * \param list_b Actual parameters passed to the function.
 * \see matching_signature()
 */
static parameter_list_match_t
parameter_lists_match(const exec_list *list_a, const exec_list *list_b)
{
   const exec_node *node_a = list_a->head;
   const exec_node *node_b = list_b->head;

   /* This is set to true if there is an inexact match requiring an implicit
    * conversion. */
   bool inexact_match = false;

   for (/* empty */
	; !node_a->is_tail_sentinel()
	; node_a = node_a->next, node_b = node_b->next) {
      /* If all of the parameters from the other parameter list have been
       * exhausted, the lists have different length and, by definition,
       * do not match.
       */
      if (node_b->is_tail_sentinel())
	 return PARAMETER_LIST_NO_MATCH;


      const ir_variable *const param = (ir_variable *) node_a;
      const ir_instruction *const actual = (ir_instruction *) node_b;

      if (param->type == actual->type)
	 continue;

      /* Try to find an implicit conversion from actual to param. */
      inexact_match = true;
      switch ((enum ir_variable_mode)(param->mode)) {
      case ir_var_auto:
      case ir_var_uniform:
      case ir_var_temporary:
	 /* These are all error conditions.  It is invalid for a parameter to
	  * a function to be declared as auto (not in, out, or inout) or
	  * as uniform.
	  */
	 assert(0);
	 return PARAMETER_LIST_NO_MATCH;

      case ir_var_const_in:
      case ir_var_in:
	 if (!actual->type->can_implicitly_convert_to(param->type))
	    return PARAMETER_LIST_NO_MATCH;
	 break;

      case ir_var_out:
	 if (!param->type->can_implicitly_convert_to(actual->type))
	    return PARAMETER_LIST_NO_MATCH;
	 break;

      case ir_var_inout:
	 /* Since there are no bi-directional automatic conversions (e.g.,
	  * there is int -> float but no float -> int), inout parameters must
	  * be exact matches.
	  */
	 return PARAMETER_LIST_NO_MATCH;

      default:
	 assert(false);
	 return PARAMETER_LIST_NO_MATCH;
      }
   }

   /* If all of the parameters from the other parameter list have been
    * exhausted, the lists have different length and, by definition, do not
    * match.
    */
   if (!node_b->is_tail_sentinel())
      return PARAMETER_LIST_NO_MATCH;

   if (inexact_match)
      return PARAMETER_LIST_INEXACT_MATCH;
   else
      return PARAMETER_LIST_EXACT_MATCH;
}


ir_function_signature *
ir_function::matching_signature(const exec_list *actual_parameters)
{
   bool is_exact;
   return matching_signature(actual_parameters, &is_exact);
}

ir_function_signature *
ir_function::matching_signature(const exec_list *actual_parameters,
			        bool *is_exact)
{
   ir_function_signature *match = NULL;
   bool multiple_inexact_matches = false;

   /* From page 42 (page 49 of the PDF) of the GLSL 1.20 spec:
    *
    * "If an exact match is found, the other signatures are ignored, and
    *  the exact match is used.  Otherwise, if no exact match is found, then
    *  the implicit conversions in Section 4.1.10 "Implicit Conversions" will
    *  be applied to the calling arguments if this can make their types match
    *  a signature.  In this case, it is a semantic error if there are
    *  multiple ways to apply these conversions to the actual arguments of a
    *  call such that the call can be made to match multiple signatures."
    */
   foreach_iter(exec_list_iterator, iter, signatures) {
      ir_function_signature *const sig =
	 (ir_function_signature *) iter.get();

      switch (parameter_lists_match(& sig->parameters, actual_parameters)) {
      case PARAMETER_LIST_EXACT_MATCH:
	 *is_exact = true;
	 return sig;
      case PARAMETER_LIST_INEXACT_MATCH:
	 if (match == NULL)
	    match = sig;
	 else
	    multiple_inexact_matches = true;
	 continue;
      case PARAMETER_LIST_NO_MATCH:
	 continue;
      default:
	 assert(false);
	 return NULL;
      }
   }

   /* There is no exact match (we would have returned it by now).  If there
    * are multiple inexact matches, the call is ambiguous, which is an error.
    *
    * FINISHME: Report a decent error.  Returning NULL will likely result in
    * FINISHME: a "no matching signature" error; it should report that the
    * FINISHME: call is ambiguous.  But reporting errors from here is hard.
    */
   *is_exact = false;

   if (multiple_inexact_matches)
      return NULL;

   return match;
}


static bool
parameter_lists_match_exact(const exec_list *list_a, const exec_list *list_b)
{
   const exec_node *node_a = list_a->head;
   const exec_node *node_b = list_b->head;

   for (/* empty */
	; !node_a->is_tail_sentinel() && !node_b->is_tail_sentinel()
	; node_a = node_a->next, node_b = node_b->next) {
      ir_variable *a = (ir_variable *) node_a;
      ir_variable *b = (ir_variable *) node_b;

      /* If the types of the parameters do not match, the parameters lists
       * are different.
       */
      if (a->type != b->type)
         return false;
   }

   /* Unless both lists are exhausted, they differ in length and, by
    * definition, do not match.
    */
   return (node_a->is_tail_sentinel() == node_b->is_tail_sentinel());
}

ir_function_signature *
ir_function::exact_matching_signature(const exec_list *actual_parameters)
{
   foreach_iter(exec_list_iterator, iter, signatures) {
      ir_function_signature *const sig =
	 (ir_function_signature *) iter.get();

      if (parameter_lists_match_exact(&sig->parameters, actual_parameters))
	 return sig;
   }
   return NULL;
}
