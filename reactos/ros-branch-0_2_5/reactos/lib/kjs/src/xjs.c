/*
 * JavaScript extension.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/xjs.c,v $
 * $Id: xjs.c,v 1.1 2004/01/10 20:38:18 arty Exp $
 */

#include "js.h"
#include "jsint.h"

/*
 * Types and definitions.
 */

/* Class context. */
struct xjs_ctx_st
{
  /* Methods. */
  JSSymbol s_compile;
  JSSymbol s_eval;
  JSSymbol s_evalFile;
  JSSymbol s_evalJavaScriptFile;
  JSSymbol s_executeByteCodeFile;
  JSSymbol s_getVar;
  JSSymbol s_setVar;

  /* Properties. */
  JSSymbol s_errorMessage;
};

typedef struct xjs_ctx_st XJSCtx;

/* Instance context. */
struct xjs_instance_ctx_st
{
  JSInterpPtr interp;
};

typedef struct xjs_instance_ctx_st XJSInstanceCtx;


/*
 * Static functions.
 */

static void
copy_from_type_to_node (JSVirtualMachine *vm, JSNode *to, JSType *from)
{
  int i;

  switch (from->type)
    {
    case JS_TYPE_NULL:
      to->type = JS_NULL;
      break;

    case JS_TYPE_BOOLEAN:
      to->type = JS_BOOLEAN;
      to->u.vboolean = from->u.i;
      break;

    case JS_TYPE_INTEGER:
      to->type = JS_INTEGER;
      to->u.vinteger = from->u.i;
      break;

    case JS_TYPE_STRING:
      js_vm_make_string (vm, to, from->u.s->data, from->u.s->len);
      break;

    case JS_TYPE_DOUBLE:
      to->type = JS_FLOAT;
      to->u.vfloat = from->u.d;
      break;

    case JS_TYPE_ARRAY:
      js_vm_make_array (vm, to, from->u.array->length);
      for (i = 0; i < from->u.array->length; i++)
	copy_from_type_to_node (vm,
				&to->u.varray->data[i],
				&from->u.array->data[i]);
      break;

    default:
      to->type = JS_UNDEFINED;
      break;
    }
}


static void
copy_from_node_to_type (JSInterpPtr interp, JSType *to, JSNode *from)
{
  int i;

  switch (from->type)
    {
    case JS_NULL:
      to->type = JS_TYPE_NULL;
      break;

    case JS_BOOLEAN:
      to->type = JS_TYPE_BOOLEAN;
      to->u.i = from->u.vboolean;
      break;

    case JS_INTEGER:
      to->type = JS_TYPE_INTEGER;
      to->u.i = from->u.vinteger;
      break;

    case JS_STRING:
      js_type_make_string (interp, to, from->u.vstring->data,
			   from->u.vstring->len);
      break;

    case JS_FLOAT:
      to->type = JS_TYPE_DOUBLE;
      to->u.d = from->u.vfloat;
      break;

    case JS_ARRAY:
      js_type_make_array (interp, to, from->u.varray->length);
      for (i = 0; i < from->u.varray->length; i++)
	copy_from_node_to_type (interp,
				&to->u.array->data[i],
				&from->u.varray->data[i]);
      break;

    default:
      to->type = JS_TYPE_UNDEFINED;
      break;
    }
}


/* Method proc. */
static int
method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	void *instance_context, JSSymbol method, JSNode *result_return,
	JSNode *args)
{
  XJSCtx *ctx = builtin_info->obj_context;
  XJSInstanceCtx *instance = instance_context;

  /* The default result is false. */
  result_return->type = JS_BOOLEAN;
  result_return->u.vboolean = 0;

  /*
   * Static methods.
   */
  if (method == vm->syms.s_toString)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      if (instance)
	js_vm_make_static_string (vm, result_return, "JSInterp", 8);
      else
	js_vm_make_static_string (vm, result_return, "JS", 2);
    }
  /* ********************************************************************** */
  else if (instance)
    {
      /*
       * Instance methods.
       */

      if (method == ctx->s_compile)
	{
	  if (args->u.vinteger != 3)
	    goto argument_error;

	  if (args[1].type != JS_STRING
	      || (args[2].type != JS_NULL && args[2].type != JS_STRING)
	      || (args[3].type != JS_NULL && args[3].type != JS_STRING))
	    goto argument_type_error;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_eval)
	{
	  if (args->u.vinteger != 1)
	    goto argument_error;

	  if (args[1].type != JS_STRING)
	    goto argument_type_error;

	  result_return->type = JS_BOOLEAN;
	  result_return->u.vboolean
	    = (js_eval_data (instance->interp, args[1].u.vstring->data,
			     args[1].u.vstring->len) != 0);
	}
      /* ***************************************************************** */
      else if (method == ctx->s_evalFile)
	{
	  char *path;

	  if (args->u.vinteger != 1)
	    goto argument_error;

	  if (args[1].type != JS_STRING)
	    goto argument_type_error;

	  path = js_string_to_c_string (vm, &args[1]);

	  result_return->type = JS_BOOLEAN;
	  result_return->u.vboolean = (js_eval_file (instance->interp, path)
				       != 0);

	  js_free (path);
	}
      /* ***************************************************************** */
      else if (method == ctx->s_evalJavaScriptFile)
	{
	  char *path;

	  if (args->u.vinteger != 1)
	    goto argument_error;

	  if (args[1].type != JS_STRING)
	    goto argument_type_error;

	  path = js_string_to_c_string (vm, &args[1]);

	  result_return->type = JS_BOOLEAN;
	  result_return->u.vboolean
	    = (js_eval_javascript_file (instance->interp, path) != 0);

	  js_free (path);
	}
      /* ***************************************************************** */
      else if (method == ctx->s_executeByteCodeFile)
	{
	  char *path;

	  if (args->u.vinteger != 1)
	    goto argument_error;

	  if (args[1].type != JS_STRING)
	    goto argument_type_error;

	  path = js_string_to_c_string (vm, &args[1]);

	  result_return->type = JS_BOOLEAN;
	  result_return->u.vboolean
	    = (js_execute_byte_code_file (instance->interp, path) != 0);

	  js_free (path);
	}
      /* ***************************************************************** */
      else if (method == ctx->s_getVar)
	{
	  char *cp;
	  JSType value;

	  if (args->u.vinteger != 1)
	    goto argument_error;

	  if (args[1].type != JS_STRING)
	    goto argument_type_error;

	  cp = js_string_to_c_string (vm, &args[1]);
	  js_get_var (instance->interp, cp, &value);
	  js_free (cp);

	  copy_from_type_to_node (vm, result_return, &value);
	}
      /* ***************************************************************** */
      else if (method == ctx->s_setVar)
	{
	  char *cp;
	  JSType value;

	  if (args->u.vinteger != 2)
	    goto argument_error;

	  if (args[1].type != JS_STRING)
	    goto argument_type_error;

	  copy_from_node_to_type (instance->interp, &value, &args[2]);

	  cp = js_string_to_c_string (vm, &args[1]);
	  js_set_var (instance->interp, cp, &value);
	  js_free (cp);

	  result_return->type = JS_UNDEFINED;
	}
      /* ***************************************************************** */
      else
	return JS_PROPERTY_UNKNOWN;
    }
  /* ********************************************************************** */
  else
    return JS_PROPERTY_UNKNOWN;

  return JS_PROPERTY_FOUND;


  /*
   * Error handling.
   */

 argument_error:
  sprintf (vm->error, "JS.%s(): illegal amount of arguments",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

 argument_type_error:
  sprintf (vm->error, "JS.%s(): illegal argument",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

  /* NOTREACHED */
  return 0;
}


/* Property proc. */
static int
property (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	  void *instance_context, JSSymbol method, int set, JSNode *node)
{
  XJSCtx *ctx = builtin_info->obj_context;
  XJSInstanceCtx *instance = instance_context;

  if (method == ctx->s_errorMessage)
    {
      char *cp = instance->interp->vm->error;

      if (set)
	goto immutable;

      js_vm_make_string (vm, node, cp, strlen (cp));
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
  sprintf (vm->error, "JS.%s: immutable property",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

  /* NOTREACHED. */
  return 0;
}


/* New proc. */
static void
new_proc (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info, JSNode *args,
	  JSNode *result_return)
{
  XJSInstanceCtx *instance;
  JSInterpOptions options;

  if (args->u.vinteger != 0)
    {
      sprintf (vm->error, "new JS(): illegal amount of arguments");
      js_vm_error (vm);
    }

  instance = js_calloc (vm, 1, sizeof (*instance));

  js_init_default_options (&options);
  instance->interp = js_create_interp (&options);

  js_vm_builtin_create (vm, result_return, builtin_info, instance);
}

/* Delete proc. */
static void
delete_proc (JSBuiltinInfo *builtin_info, void *instance_context)
{
  XJSInstanceCtx *instance = instance_context;

  if (instance)
    {
      js_destroy_interp (instance->interp);
      js_free (instance);
    }
}


/*
 * Global functions.
 */

void
js_ext_JS (JSInterpPtr interp)
{
  JSNode *n;
  JSBuiltinInfo *info;
  JSSymbol sym;
  XJSCtx *ctx;
  JSVirtualMachine *vm = interp->vm;

  ctx = js_calloc (vm, 1, sizeof (*ctx));

  ctx->s_compile		= js_vm_intern (vm, "compile");
  ctx->s_eval			= js_vm_intern (vm, "eval");
  ctx->s_evalFile		= js_vm_intern (vm, "evalFile");
  ctx->s_evalJavaScriptFile	= js_vm_intern (vm, "evalJavaScriptFile");
  ctx->s_executeByteCodeFile	= js_vm_intern (vm, "executeByteCodeFile");
  ctx->s_getVar			= js_vm_intern (vm, "getVar");
  ctx->s_setVar			= js_vm_intern (vm, "setVar");

  ctx->s_errorMessage		= js_vm_intern (vm, "errorMessage");

  /* Object information. */

  info = js_vm_builtin_info_create (vm);

  info->method_proc		= method;
  info->property_proc		= property;
  info->new_proc		= new_proc;
  info->delete_proc		= delete_proc;
  info->obj_context		= ctx;
  info->obj_context_delete	= js_free;

  /* Define it. */
  sym = js_vm_intern (vm, "JS");
  n = &vm->globals[sym];

  js_vm_builtin_create (vm, n, info, NULL);
}
