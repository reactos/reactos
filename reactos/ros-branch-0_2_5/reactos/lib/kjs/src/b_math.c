/*
 * The builtin Math object.
 * Copyright (c) 1998 New Generation Software (NGS) Oy
 *
 * Author: Markku Rossi <mtr@ngs.fi>
 */

/*
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA
 */

/*
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/b_math.c,v $
 * $Id: b_math.c,v 1.1 2004/01/10 20:38:18 arty Exp $
 */

#include "jsint.h"
#include "rentrant.h"

/*
 * Types and definitions.
 */

#ifndef M_E
#define M_E		2.71828182845904523536028747135266250
#endif

#ifndef M_LN10
#define M_LN10		2.302585092994046
#endif

#ifndef M_LN2
#define M_LN2		0.693147180559945309417232121458176568
#endif

#ifndef M_LOG10E
#define M_LOG10E	0.434294481903251827651128918916605082
#endif

#ifndef M_LOG2E
#define M_LOG2E		1.44269504088896340735992468100189214
#endif

#ifndef M_PI
#define M_PI		3.14159265358979323846264338327950288
#endif

#ifndef M_SQRT1_2
#define M_SQRT1_2	0.707106781186547524400844362104849039
#endif

#ifndef M_SQRT2
#define M_SQRT2		1.41421356237309504880168872420969808
#endif


#define ONE_ARG()				\
  do {						\
    JSNode cvt;					\
						\
    if (args->u.vinteger != 1)			\
      goto argument_error;			\
						\
    js_vm_to_number (vm, &args[1], &cvt);	\
    if (cvt.type == JS_INTEGER)			\
      d = (double) cvt.u.vinteger;		\
    else if (cvt.type == JS_FLOAT)		\
      d = cvt.u.vfloat;				\
    else					\
      {						\
	/* Must be NaN. */			\
	result_return->type = JS_NAN;		\
	goto done;				\
      }						\
  } while (0)

#define TWO_ARGS()				\
  do {						\
    JSNode cvt;					\
						\
    if (args->u.vinteger != 2)			\
      goto argument_error;			\
						\
    js_vm_to_number (vm, &args[1], &cvt);	\
    if (cvt.type == JS_INTEGER)			\
      d = (double) cvt.u.vinteger;		\
    else if (cvt.type == JS_FLOAT)		\
      d = cvt.u.vfloat;				\
    else					\
      {						\
	/* Must be NaN. */			\
	result_return->type = JS_NAN;		\
	goto done;				\
      }						\
						\
    js_vm_to_number (vm, &args[2], &cvt);	\
    if (cvt.type == JS_INTEGER)			\
      d2 = (double) args[1].u.vinteger;		\
    else if (cvt.type == JS_FLOAT)		\
      d2 = cvt.u.vfloat;			\
    else					\
      {						\
	/* Must be NaN. */			\
	result_return->type = JS_NAN;		\
	goto done;				\
      }						\
						\
  } while (0)

/* Class context. */
struct math_ctx_st
{
  JSSymbol s_abs;
  JSSymbol s_acos;
  JSSymbol s_asin;
  JSSymbol s_atan;
  JSSymbol s_atan2;
  JSSymbol s_ceil;
  JSSymbol s_cos;
  JSSymbol s_exp;
  JSSymbol s_floor;
  JSSymbol s_log;
  JSSymbol s_max;
  JSSymbol s_min;
  JSSymbol s_pow;
  JSSymbol s_random;
  JSSymbol s_round;
  JSSymbol s_seed;
  JSSymbol s_sin;
  JSSymbol s_sqrt;
  JSSymbol s_tan;

  JSSymbol s_E;
  JSSymbol s_LN10;
  JSSymbol s_LN2;
  JSSymbol s_LOG10E;
  JSSymbol s_LOG2E;
  JSSymbol s_PI;
  JSSymbol s_SQRT1_2;
  JSSymbol s_SQRT2;

  void *drand48_context;
};

typedef struct math_ctx_st MathCtx;

/*
 * Static functions.
 */

/* Method proc. */
static int
method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	void *instance_context, JSSymbol method, JSNode *result_return,
	JSNode *args)
{
  MathCtx *ctx = builtin_info->obj_context;
  double d, d2;
  int i;

  /* The default return value. */
  result_return->type = JS_FLOAT;

  if (method == ctx->s_abs)
    {
      ONE_ARG ();
      result_return->u.vfloat = fabs (d);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_acos)
    {
      ONE_ARG ();
      result_return->u.vfloat = acos (d);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_asin)
    {
      ONE_ARG ();
      result_return->u.vfloat = asin (d);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_atan)
    {
      ONE_ARG ();
      result_return->u.vfloat = atan (d);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_atan2)
    {
      TWO_ARGS ();
      result_return->u.vfloat = atan2 (d, d2);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_ceil)
    {
      ONE_ARG ();
      result_return->u.vfloat = ceil (d);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_cos)
    {
      ONE_ARG ();
      result_return->u.vfloat = cos (d);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_exp)
    {
      ONE_ARG ();
      result_return->u.vfloat = exp (d);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_floor)
    {
      ONE_ARG ();
      result_return->u.vfloat = floor (d);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_log)
    {
      ONE_ARG ();
      result_return->u.vfloat = log (d);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_max || method == ctx->s_min)
    {
      JSNode cvt;

      if (args->u.vinteger < 1)
	goto argument_error;

      /* Take the initial argument. */
      js_vm_to_number (vm, &args[1], &cvt);
      if (cvt.type == JS_NAN)
	{
	  result_return->type = JS_NAN;
	  goto done;
	}
      if (cvt.type == JS_INTEGER)
	d = (double) cvt.u.vinteger;
      else
	d = cvt.u.vfloat;

      /* Handle the rest. */
      for (i = 1; i < args->u.vinteger; i++)
	{
	  js_vm_to_number (vm, &args[1], &cvt);
	  if (cvt.type == JS_NAN)
	    {
	      result_return->type = JS_NAN;
	      goto done;
	    }
	  if (cvt.type == JS_INTEGER)
	    d2 = (double) cvt.u.vinteger;
	  else
	    d2 = cvt.u.vfloat;

	  if (method == ctx->s_max)
	    {
	      if (d2 > d)
		d = d2;
	    }
	  else
	    {
	      if (d2 < d)
		d = d2;
	    }
	}

      result_return->type = JS_FLOAT;
      result_return->u.vfloat = d;
    }
  /* ********************************************************************** */
  else if (method == ctx->s_pow)
    {
      TWO_ARGS ();
      result_return->u.vfloat = pow (d, d2);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_random)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      /* js_srand48 (ctx->drand48_context, time (NULL)); */
      js_drand48 (ctx->drand48_context, &result_return->u.vfloat);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_round)
    {
      ONE_ARG ();
      result_return->type = JS_INTEGER;
      result_return->u.vinteger = (long) (d + 0.5);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_seed)
    {
      ONE_ARG ();
      js_srand48 (ctx->drand48_context, (long) d);
      result_return->type = JS_UNDEFINED;
    }
  /* ********************************************************************** */
  else if (method == ctx->s_sin)
    {
      ONE_ARG ();
      result_return->u.vfloat = sin (d);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_sqrt)
    {
      ONE_ARG ();
      result_return->u.vfloat = sqrt (d);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_tan)
    {
      ONE_ARG ();
      result_return->u.vfloat = tan (d);
    }
  /* ********************************************************************** */
  else if (method == vm->syms.s_toString)
    {
      js_vm_make_static_string (vm, result_return, "Math", 4);
    }
  /* ********************************************************************** */
  else
    return JS_PROPERTY_UNKNOWN;

 done:

  return JS_PROPERTY_FOUND;


  /*
   * Error handling.
   */

 argument_error:
  sprintf (vm->error, "Math.%s(): illegal amount of arguments",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

 argument_type_error:
  sprintf (vm->error, "Math.%s(): illegal argument",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

  /* NOTREACHED. */
  return 0;
}

/* Property proc. */
static int
property (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	  void *instance_context, JSSymbol property, int set, JSNode *node)
{
  MathCtx *ctx = builtin_info->obj_context;

  /* The default result is float. */
  node->type = JS_FLOAT;

  if (property == ctx->s_E)
    {
      if (set)
	goto immutable;

      node->u.vfloat = M_E;
    }
  else if (property == ctx->s_LN10)
    {
      if (set)
	goto immutable;

      node->u.vfloat = M_LN10;
    }
  else if (property == ctx->s_LN2)
    {
      if (set)
	goto immutable;

      node->u.vfloat = M_LN2;
    }
  else if (property == ctx->s_LOG10E)
    {
      if (set)
	goto immutable;

      node->u.vfloat = M_LOG10E;
    }
  else if (property == ctx->s_LOG2E)
    {
      if (set)
	goto immutable;

      node->u.vfloat = M_LOG2E;
    }
  else if (property == ctx->s_PI)
    {
      if (set)
	goto immutable;

      node->u.vfloat = M_PI;
    }
  else if (property == ctx->s_SQRT1_2)
    {
      if (set)
	goto immutable;

      node->u.vfloat = M_SQRT1_2;
    }
  else if (property == ctx->s_SQRT2)
    {
      if (set)
	goto immutable;

      node->u.vfloat = M_SQRT2;
    }
  else
    {
      if (!set)
	node->type = JS_UNDEFINED;

      return JS_PROPERTY_UNKNOWN;
    }

  return JS_PROPERTY_FOUND;


  /*
   * Error handling.
   */

 immutable:
  sprintf (vm->error, "Math.%s: immutable property",
	   js_vm_symname (vm, property));
  js_vm_error (vm);

  /* NOTREACHED. */
  return 0;
}


/*
 * Global functions.
 */

void
js_builtin_Math (JSVirtualMachine *vm)
{
  JSBuiltinInfo *info;
  MathCtx *ctx;
  JSNode *n;

  ctx = js_calloc (vm, 1, sizeof (*ctx));

  ctx->s_abs		= js_vm_intern (vm, "abs");
  ctx->s_acos		= js_vm_intern (vm, "acos");
  ctx->s_asin		= js_vm_intern (vm, "asin");
  ctx->s_atan		= js_vm_intern (vm, "atan");
  ctx->s_atan2		= js_vm_intern (vm, "atan2");
  ctx->s_ceil		= js_vm_intern (vm, "ceil");
  ctx->s_cos		= js_vm_intern (vm, "cos");
  ctx->s_exp		= js_vm_intern (vm, "exp");
  ctx->s_floor		= js_vm_intern (vm, "floor");
  ctx->s_log		= js_vm_intern (vm, "log");
  ctx->s_max		= js_vm_intern (vm, "max");
  ctx->s_min		= js_vm_intern (vm, "min");
  ctx->s_pow		= js_vm_intern (vm, "pow");
  ctx->s_random		= js_vm_intern (vm, "random");
  ctx->s_round		= js_vm_intern (vm, "round");
  ctx->s_seed		= js_vm_intern (vm, "seed");
  ctx->s_sin		= js_vm_intern (vm, "sin");
  ctx->s_sqrt		= js_vm_intern (vm, "sqrt");
  ctx->s_tan		= js_vm_intern (vm, "tan");

  ctx->s_E		= js_vm_intern (vm, "E");
  ctx->s_LN10		= js_vm_intern (vm, "LN10");
  ctx->s_LN2		= js_vm_intern (vm, "LN2");
  ctx->s_LOG10E		= js_vm_intern (vm, "LOG10E");
  ctx->s_LOG2E		= js_vm_intern (vm, "LOG2E");
  ctx->s_PI		= js_vm_intern (vm, "PI");
  ctx->s_SQRT1_2	= js_vm_intern (vm, "SQRT1_2");
  ctx->s_SQRT2		= js_vm_intern (vm, "SQRT2");

  ctx->drand48_context = js_drand48_create (vm);

  /* Object information. */

  info = js_vm_builtin_info_create (vm);

  info->method_proc		= method;
  info->property_proc		= property;
  info->obj_context		= ctx;
  info->obj_context_delete	= js_free;

  /* Define it. */
  n = &vm->globals[js_vm_intern (vm, "Math")];
  js_vm_builtin_create (vm, n, info, NULL);
}
