/*
 * The builtin Object object.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/b_object.c,v $
 * $Id: b_object.c,v 1.1 2004/01/10 20:38:18 arty Exp $
 */

#include "jsint.h"

/*
 * Static functions.
 */

/* Global method proc. */
static void
global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	       void *instance_context, JSNode *result_return,
	       JSNode *args)
{
  if (args->u.vinteger > 1)
    {
      sprintf (vm->error, "Object(): illegal amount of arguments");
      js_vm_error (vm);
    }
  if (args->u.vinteger == 0
      || (args->u.vinteger == 1
	  && (args[1].type == JS_NULL
	      || args[1].type == JS_UNDEFINED)))
    {
      /* Create a fresh new object. */
      result_return->type = JS_OBJECT;
      result_return->u.vobject = js_vm_object_new (vm);
    }
  else
    {
      /* We have one argument.  Call ToObject() for it. */
      js_vm_to_object (vm, &args[1], result_return);
    }
}

/* Method proc. */
static int
method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	void *instance_context, JSSymbol method, JSNode *result_return,
	JSNode *args)
{
  JSNode *n = instance_context;

  if (method == vm->syms.s_toSource)
    {
      char *source;

      if (instance_context)
	{
	  result_return->type = JS_UNDEFINED;
	  /* XXX 15.2.4.3 */
	}
      else
	{
	  source = "new Object()";
	  js_vm_make_static_string (vm, result_return, source,
				    strlen (source));
	}
    }
  /* ********************************************************************** */
  else if (method == vm->syms.s_toString)
    {
      if (instance_context)
	js_vm_make_static_string (vm, result_return, "[object Object]", 15);
      else
	js_vm_make_static_string (vm, result_return, "Object", 6);
    }
  /* ********************************************************************** */
  else if (method == vm->syms.s_valueOf)
    {
      if (instance_context)
	JS_COPY (result_return, n);
      else
	{
	  n = &vm->globals[js_vm_intern (vm, "Object")];
	  JS_COPY (result_return, n);
	}
    }
  /* ********************************************************************** */
  else
    return JS_PROPERTY_UNKNOWN;

  return JS_PROPERTY_FOUND;
}


/* Property proc. */
static int
property (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	  void *instance_context, JSSymbol property, int set, JSNode *node)
{
  if (!set)
    node->type = JS_UNDEFINED;

  return JS_PROPERTY_UNKNOWN;
}

/* New proc. */
static void
new_proc (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info, JSNode *args,
	  JSNode *result_return)
{
  if (args->u.vinteger == 0)
    {
    return_native_object:
      result_return->type = JS_OBJECT;
      result_return->u.vobject = js_vm_object_new (vm);

      /* Set the [[Prototype]] and [[Class]] properties. */
      /* XXX 15.2.2.2 */
    }
  else if (args->u.vinteger == 1)
    {
      switch (args[1].type)
	{
	case JS_OBJECT:
	  JS_COPY (result_return, &args[1]);
	  break;

	case JS_STRING:
	case JS_BOOLEAN:
	case JS_INTEGER:
	case JS_FLOAT:
	case JS_NAN:
	  js_vm_to_object (vm, &args[1], result_return);
	  break;

	case JS_NULL:
	case JS_UNDEFINED:
	  goto return_native_object;
	  break;

	default:
	  /* The rest are implementation dependent. */
	  JS_COPY (result_return, &args[1]);
	  break;
	}
    }
  else
    {
      sprintf (vm->error, "new Object(): illegal amount of arguments");
      js_vm_error (vm);
    }
}


/*
 * Global functions.
 */

void
js_builtin_Object (JSVirtualMachine *vm)
{
  JSNode *n;
  JSBuiltinInfo *info;

  info = js_vm_builtin_info_create (vm);
  vm->prim[JS_OBJECT] = info;

  info->global_method_proc	= global_method;
  info->method_proc		= method;
  info->property_proc		= property;
  info->new_proc		= new_proc;

  /* Define it. */
  n = &vm->globals[js_vm_intern (vm, "Object")];
  js_vm_builtin_create (vm, n, info, NULL);
}
