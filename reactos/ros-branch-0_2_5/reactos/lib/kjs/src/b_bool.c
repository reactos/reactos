/*
 * The builtin Boolean object.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/b_bool.c,v $
 * $Id: b_bool.c,v 1.1 2004/01/10 20:38:18 arty Exp $
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
  result_return->type = JS_BOOLEAN;

  if (args->u.vinteger == 0)
    result_return->u.vboolean = 0;
  else if (args->u.vinteger == 1)
    result_return->u.vboolean = js_vm_to_boolean (vm, &args[1]);
  else
    {
      sprintf (vm->error, "Boolean(): illegal amount of arguments");
      js_vm_error (vm);
    }
}

/* Method proc. */
static int
method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	void *instance_context, JSSymbol method, JSNode *result_return,
	JSNode *args)
{
  JSNode *n = instance_context;
  char *cp;

  if (method == vm->syms.s_toString)
    {
      if (args->u.vinteger != 0)
	{
	  sprintf (vm->error, "Boolean.%s(): illegal amount of arguments",
		   js_vm_symname (vm, method));
	  js_vm_error (vm);
	}

      if (n)
	{
	  cp = n->u.vboolean ? "true" : "false";
	  js_vm_make_static_string (vm, result_return, cp, strlen (cp));
	}
      else
	js_vm_make_static_string (vm, result_return, "Boolean", 7);
    }
  /* ********************************************************************** */
  else if (method == vm->syms.s_valueOf)
    {
      if (n)
	JS_COPY (result_return, n);
      else
	{
	  n = &vm->globals[js_vm_intern (vm, "Boolean")];
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
  result_return->type = JS_BOOLEAN;

  if (args->u.vinteger == 0)
    result_return->u.vboolean = 0;
  else if (args->u.vinteger == 1)
    result_return->u.vboolean = js_vm_to_boolean (vm, &args[1]);
  else
    {
      sprintf (vm->error, "new Boolean(): illegal amount of arguments");
      js_vm_error (vm);
    }

  /* Set the [[Prototype]] and [[Class]] properties. */
  /* XXX 15.10.2.1 */
}


/*
 * Global functions.
 */

void
js_builtin_Boolean (JSVirtualMachine *vm)
{
  JSNode *n;
  JSBuiltinInfo *info;

  info = js_vm_builtin_info_create (vm);
  vm->prim[JS_BOOLEAN] = info;

  info->global_method_proc	= global_method;
  info->method_proc		= method;
  info->property_proc		= property;
  info->new_proc		= new_proc;

  /* Define it. */
  n = &vm->globals[js_vm_intern (vm, "Boolean")];
  js_vm_builtin_create (vm, n, info, NULL);
}
