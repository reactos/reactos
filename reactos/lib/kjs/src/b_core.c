/*
 * Core builtins for the JavaScript VM.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/b_core.c,v $
 * $Id: b_core.c,v 1.1 2004/01/10 20:38:18 arty Exp $
 */

/*
 * Global methods:
 *
 *  parseInt (string[, radix])
 *  parseFloat (string)
 *  escape (string)
 *  unescape (string)
 *  isNaN (any)
 *  isFinite (any)
 *  debug (any)
 *  error (string)
 *  float (any)
 *  int (any)
 *  isFloat (any)
 *  isInt (any)
 *  print (any[,...])
 */

#include "jsint.h"

/*
 * Types and definitions.
 */

#define EMIT_TO_RESULT(c)						\
  do {									\
    result_return->u.vstring->data =					\
    js_vm_realloc (vm, result_return->u.vstring->data,			\
		   result_return->u.vstring->len + 1);			\
   result_return->u.vstring->data[result_return->u.vstring->len] = (c); \
   result_return->u.vstring->len += 1;					\
 } while (0)


/*
 * Static functions.
 */

static void
parseInt_global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
			void *instance_context, JSNode *result_return,
			JSNode *args)
{
  JSInt32 base = 0;
  char *cp, *end;

  result_return->type = JS_INTEGER;

  if (args->u.vinteger != 1 && args->u.vinteger != 2)
    {
      sprintf (vm->error, "parseInt(): illegal amount of arguments");
      js_vm_error (vm);
    }
  if (args[1].type == JS_STRING)
    cp = js_string_to_c_string (vm, &args[1]);
  else
    {
      JSNode input;

      /* Convert the input to string. */
      js_vm_to_string (vm, &args[1], &input);
      cp = js_string_to_c_string (vm, &input);
    }
  if (args->u.vinteger == 2)
    {
      if (args[2].type == JS_INTEGER)
	base = args[2].u.vinteger;
      else
	base = js_vm_to_int32 (vm, &args[2]);
    }

  result_return->u.vinteger = strtol (cp, &end, base);
  js_free (cp);

  if (cp == end)
    result_return->type = JS_NAN;
}


static void
parseFloat_global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
			  void *instance_context, JSNode *result_return,
			  JSNode *args)
{
  char *cp, *end;

  result_return->type = JS_FLOAT;

  if (args->u.vinteger != 1)
    {
      sprintf (vm->error, "parseFloat(): illegal amount of arguments");
      js_vm_error (vm);
    }
  if (args[1].type == JS_STRING)
    cp = js_string_to_c_string (vm, &args[1]);
  else
    {
      JSNode input;

      /* Convert the input to string. */
      js_vm_to_string (vm, &args[1], &input);
      cp = js_string_to_c_string (vm, &input);
    }

  result_return->u.vfloat = strtod (cp, &end);
  js_free (cp);

  if (cp == end)
    /* Couldn't parse, return NaN. */
    result_return->type = JS_NAN;
}


static void
escape_global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
		      void *instance_context, JSNode *result_return,
		      JSNode *args)
{
  unsigned char *dp;
  unsigned int n, i;
  JSNode *source;
  JSNode source_n;

  if (args->u.vinteger != 1)
    {
      sprintf (vm->error, "escape(): illegal amount of arguments");
      js_vm_error (vm);
    }
  if (args[1].type == JS_STRING)
    source = &args[1];
  else
    {
      /* Convert the argument to string. */
      js_vm_to_string (vm, &args[1], &source_n);
      source = &source_n;
    }

  /*
   * Allocate the result string, Let's guess that we need at least
   * <source->u.vstring->len> bytes of data.
   */
  n = source->u.vstring->len;
  dp = source->u.vstring->data;
  js_vm_make_string (vm, result_return, NULL, n);
  result_return->u.vstring->len = 0;

  /*
   * Scan for characters requiring escapes.
   */
  for (i = 0; i < n; i += 1)
    {
      unsigned int c = dp[i];

      if (strchr ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789@*_+-./",
		  c))
	EMIT_TO_RESULT (c);
      else if (c > 0xFF)
	{
	  unsigned char buf[6];

	  sprintf (buf, "%04x", c);
	  EMIT_TO_RESULT ('%');
	  EMIT_TO_RESULT ('u');
	  EMIT_TO_RESULT (buf[0]);
	  EMIT_TO_RESULT (buf[1]);
	  EMIT_TO_RESULT (buf[2]);
	  EMIT_TO_RESULT (buf[3]);
      }
    else
      {
	unsigned char buf[4];
	sprintf (buf, "%02x", c);

	EMIT_TO_RESULT ('%');
	EMIT_TO_RESULT (buf[0]);
	EMIT_TO_RESULT (buf[1]);
      }
    }
}

/* A helper function for unescape(). */
static int
scanhexdigits (unsigned char *dp, int nd, unsigned int *cp)
{
  static const char digits[] = "0123456789abcdefABCDEF";
  int i;
  unsigned int d;

  *cp = 0;
  for (i = 0; i < nd; i += 1)
    {
      d = strchr (digits, dp[i]) - digits;
      if (d < 16)
	;
      else if (d < 22)
	d -= 6;
      else
	return 0;

      *cp <<= 4;
      *cp += d;
    }

  return 1;
}


static void
unescape_global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
			void *instance_context, JSNode *result_return,
			JSNode *args)
{
  unsigned char *dp;
  unsigned int n, i;
  JSNode *source;
  JSNode source_n;

  if (args->u.vinteger != 1)
    {
      sprintf (vm->error, "unescape(): illegal amount of arguments");
      js_vm_error (vm);
    }
  if (args[1].type == JS_STRING)
    source = &args[1];
  else
    {
      js_vm_to_string (vm, &args[1], &source_n);
      source = &source_n;
    }

  /*
   * Allocate the result string, Let's guess that we need at least
   * <source->u.vstring->len> bytes of data.
   */
  n = source->u.vstring->len;
  dp = source->u.vstring->data;
  js_vm_make_string (vm, result_return, NULL, n);
  result_return->u.vstring->len = 0;

  /*
   * Scan for escapes requiring characters.
   */
  for (i = 0; i < n;)
    {
      unsigned int c = dp[i];

      if (c != '%')
	i += 1;
      else if (i <= n - 6 && dp[i + 1] == 'u'
	       && scanhexdigits (dp + i + 2, 4, &c))
	i += 6;
      else if (i <= n - 3 && scanhexdigits (dp + i + 1, 2, &c))
	i += 3;
      else
	{
	  c = dp[i];
	  i += 1;
	}
      EMIT_TO_RESULT (c);
    }
}


static void
isNaN_global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
		     void *instance_context, JSNode *result_return,
		     JSNode *args)
{
  JSNode cvt;
  int result;

  if (args->u.vinteger != 1)
    {
      sprintf (vm->error, "isNaN(): illegal amount of arguments");
      js_vm_error (vm);
    }

  switch (args[1].type)
    {
    case JS_NAN:
      result = 1;
      break;

    case JS_INTEGER:
    case JS_FLOAT:
      result = 0;
      break;

    default:
      js_vm_to_number (vm, &args[1], &cvt);
      result = cvt.type == JS_NAN;
      break;
    }

  result_return->type = JS_BOOLEAN;
  result_return->u.vboolean = result;
}


static void
isFinite_global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
			void *instance_context, JSNode *result_return,
			JSNode *args)
{
  JSNode *source;
  JSNode cvt;
  int result;

  if (args->u.vinteger != 1)
    {
      sprintf (vm->error, "isFinite(): illegal amount of arguments");
      js_vm_error (vm);
    }

  if (args[1].type == JS_NAN || args[1].type == JS_INTEGER
      || args[1].type == JS_FLOAT)
    source = &args[1];
  else
    {
      js_vm_to_number (vm, &args[1], &cvt);
      source = &cvt;
    }

  switch (source->type)
    {
    case JS_NAN:
      result = 0;
      break;

    case JS_INTEGER:
      result = 1;
      break;

    case JS_FLOAT:
      if (JS_IS_POSITIVE_INFINITY (&args[1])
	  || JS_IS_NEGATIVE_INFINITY (&args[1]))
	result = 0;
      else
	result = 1;
      break;

    default:
      /* NOTREACHED */
      result = 0;
      break;
    }

  result_return->type = JS_BOOLEAN;
  result_return->u.vboolean = result;
}


static void
debug_global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
		     void *instance_context, JSNode *result_return,
		     JSNode *args)
{
  JSNode sitem;

  if (args->u.vinteger != 1)
    {
      sprintf (vm->error, "debug(): illegal amount of arguments");
      js_vm_error (vm);
    }

  /*
   * Maybe we should prefix the debug message with `Debug message:'
   * prompt.
   */
  js_vm_to_string (vm, &args[1], &sitem);
  fwrite (sitem.u.vstring->data, sitem.u.vstring->len, 1, stderr);

  result_return->type = JS_UNDEFINED;
}


static void
error_global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
		     void *instance_context,
		     JSNode *result_return, JSNode *args)
{
  unsigned int len;

  if (args->u.vinteger != 1)
    {
      sprintf (vm->error, "error(): illegal amount of arguments");
      js_vm_error (vm);
    }
  if (args[1].type != JS_STRING)
    {
      sprintf (vm->error, "error(): illegal argument");
      js_vm_error (vm);
    }

  len = args[1].u.vstring->len;
  if (len > sizeof (vm->error) - 1)
    len = sizeof (vm->error) - 1;

  memcpy (vm->error, args[1].u.vstring->data, len);
  vm->error[len] = '\0';

  /* Here we go... */
  js_vm_error (vm);

  /* NOTREACHED */
}


static void
float_global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
		     void *instance_context, JSNode *result_return,
		     JSNode *args)
{
  double fval;
  char *cp, *end;

  if (args->u.vinteger != 1)
    {
      sprintf (vm->error, "float(): illegal amount of arguments");
      js_vm_error (vm);
    }

  switch (args[1].type)
    {
    case JS_BOOLEAN:
      fval = (double) (args[1].u.vboolean != 0);
      break;

    case JS_INTEGER:
      fval = (double) args[1].u.vinteger;
      break;

    case JS_STRING:
      cp = js_string_to_c_string (vm, &args[1]);
      fval = strtod (cp, &end);
      js_free (cp);

      if (cp == end)
	fval = 0.0;
      break;

    case JS_FLOAT:
      fval = args[1].u.vfloat;
      break;

    case JS_ARRAY:
      fval = (double) args[1].u.varray->length;
      break;

    default:
      fval = 0.0;
      break;
    }

  result_return->type = JS_FLOAT;
  result_return->u.vfloat = fval;
}


static void
int_global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
		   void *instance_context, JSNode *result_return,
		   JSNode *args)
{
  long ival;
  char *cp, *end;

  if (args->u.vinteger != 1)
    {
      sprintf (vm->error, "int(): illegal amount of arguments");
      js_vm_error (vm);
    }

  switch (args[1].type)
    {
    case JS_BOOLEAN:
      ival = (long) (args[1].u.vboolean != 0);
      break;

    case JS_INTEGER:
      ival = args[1].u.vinteger;
      break;

    case JS_STRING:
      cp = js_string_to_c_string (vm, &args[1]);
      ival = strtol (cp, &end, 0);
      js_free (cp);

      if (cp == end)
	ival = 0;
      break;

    case JS_FLOAT:
      ival = (long) args[1].u.vfloat;
      break;

    case JS_ARRAY:
      ival = (long) args[1].u.varray->length;
      break;

    default:
      ival = 0;
      break;
    }

  result_return->type = JS_INTEGER;
  result_return->u.vinteger = ival;
}


static void
isFloat_global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
		       void *instance_context, JSNode *result_return,
		       JSNode *args)
{
  /* The default result is false.  */
  result_return->type = JS_BOOLEAN;
  result_return->u.vboolean = 0;

  if (args->u.vinteger != 1)
    {
      sprintf (vm->error, "isFloat(): illegal amount of arguments");
      js_vm_error (vm);
    }

  if (args[1].type == JS_FLOAT)
    result_return->u.vboolean = 1;
}


static void
isInt_global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
		     void *instance_context, JSNode *result_return,
		     JSNode *args)
{
  /* The default result is false.  */
  result_return->type = JS_BOOLEAN;
  result_return->u.vboolean = 0;

  if (args->u.vinteger != 1)
    {
      sprintf (vm->error, "isInt(): illegal amount of arguments");
      js_vm_error (vm);
    }

  if (args[1].type == JS_INTEGER)
    result_return->u.vboolean = 1;
}


static void
print_global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
		     void *instance_context, JSNode *result_return,
		     JSNode *args)
{
  int i;

  /* The result is undefined.  */
  result_return->type = JS_UNDEFINED;

  for (i = 1; i <= args->u.vinteger; i++)
    {
      JSNode result;

      js_vm_to_string (vm, &args[i], &result);
      js_iostream_write (vm->s_stdout, result.u.vstring->data,
			 result.u.vstring->len);

      if (i + 1 <= args->u.vinteger)
	js_iostream_write (vm->s_stdout, " ", 1);
    }

  js_iostream_write (vm->s_stdout, JS_HOST_LINE_BREAK, JS_HOST_LINE_BREAK_LEN);
}


/*
 * Global functions.
 */

static struct
{
  char *name;
  JSBuiltinGlobalMethod method;
} global_methods[] =
{
  {"parseInt",		parseInt_global_method},
  {"parseFloat",	parseFloat_global_method},
  {"escape",		escape_global_method},
  {"unescape",		unescape_global_method},
  {"isNaN",		isNaN_global_method},
  {"isFinite",		isFinite_global_method},
  {"debug",		debug_global_method},
  {"error",		error_global_method},
  {"float",		float_global_method},
  {"int",		int_global_method},
  {"isFloat",		isFloat_global_method},
  {"isInt",		isInt_global_method},
  {"print",		print_global_method},

  {NULL, NULL},
};


void
js_builtin_core (JSVirtualMachine *vm)
{
  int i;
  JSNode *n;

  /* Properties. */

  n = &vm->globals[js_vm_intern (vm, "NaN")];
  n->type = JS_NAN;

  n = &vm->globals[js_vm_intern (vm, "Infinity")];
  JS_MAKE_POSITIVE_INFINITY (n);

  /* Global methods. */
  for (i = 0; global_methods[i].name; i++)
    {
      JSBuiltinInfo *info;

      info = js_vm_builtin_info_create (vm);
      info->global_method_proc = global_methods[i].method;

      n = &vm->globals[js_vm_intern (vm, global_methods[i].name)];
      js_vm_builtin_create (vm, n, info, NULL);
    }
}
