/*
 * The builtin Function object.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/ksrc/b_func.c,v $
 * $Id: b_func.c,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

#include "jsint.h"

/*
 * Static functions.
 */

/* Method proc. */
static int
method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	void *instance_context, JSSymbol method, JSNode *result_return,
	JSNode *args)
{
  return JS_PROPERTY_UNKNOWN;
}

/* Property proc. */
static int
property (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	  void *instance_context, JSSymbol property, int set, JSNode *node)
{
  /* JSNode *n = instance_context; */

  if (!set)
    node->type = JS_UNDEFINED;

  return JS_PROPERTY_UNKNOWN;
}


/*
 * Global functions.
 */

void
js_builtin_Function (JSVirtualMachine *vm)
{
  JSNode *n;
  JSBuiltinInfo *info;

  info = js_vm_builtin_info_create (vm);
  vm->prim[JS_FUNC] = info;

  info->method_proc	= method;
  info->property_proc	= property;

  /* Define it. */
  n = &vm->globals[js_vm_intern (vm, "Function")];
  js_vm_builtin_create (vm, n, info, NULL);
}
