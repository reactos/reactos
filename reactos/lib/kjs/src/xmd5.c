/*
 * The MD5 extension.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/xmd5.c,v $
 * $Id: xmd5.c,v 1.1 2004/01/10 20:38:18 arty Exp $
 */

#include "jsint.h"
#include "md5.h"

/*
 * Types and definitions.
 */

/* Class context. */
struct md5_ctx_st
{
  /* Methods. */
  JSSymbol s_final;
  JSSymbol s_finalBinary;
  JSSymbol s_init;
  JSSymbol s_update;
};

typedef struct md5_ctx_st MD5Ctx;

/*
 * Static functions.
 */

/* Method proc. */
static int
method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	void *instance_context, JSSymbol method, JSNode *result_return,
	JSNode *args)
{
  MD5Ctx *ctx = builtin_info->obj_context;
  MD5_CTX *ictx = instance_context;
  unsigned char final[16];

  /* Static methods. */
  if (method == vm->syms.s_toString)
    {
      if (ictx)
	goto default_to_string;
      else
	js_vm_make_static_string (vm, result_return, "MD5", 3);
    }
  /* ********************************************************************** */
  else if (ictx)
    {
      /* Methods. */
      if (method == ctx->s_final)
	{
	  char buf[33];		/* +1 for the trailing '\0'. */
	  char *cp;
	  int i;

	default_to_string:

	  if (args->u.vinteger != 0)
	    goto argument_error;

	  MD5Final (final, ictx);
	  for (i = 0, cp = buf; i < 16; i++, cp += 2)
	    sprintf (cp, "%02X", final[i]);

	  js_vm_make_string (vm, result_return, buf, 32);
	}
      /* ***************************************************************** */
      else if (method == ctx->s_finalBinary)
	{
	  if (args->u.vinteger != 0)
	    goto argument_error;

	  MD5Final (final, ictx);
	  js_vm_make_string (vm, result_return, final, 16);
	}
      /* ***************************************************************** */
      else if (method == ctx->s_init)
	{
	  if (args->u.vinteger != 0)
	    goto argument_error;

	  MD5Init (ictx);
	  result_return->type = JS_UNDEFINED;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_update)
	{
	  if (args->u.vinteger != 1)
	    goto argument_error;
	  if (args[1].type != JS_STRING)
	    goto argument_type_error;

	  MD5Update (ictx, args[1].u.vstring->data, args[1].u.vstring->len);
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
  sprintf (vm->error, "MD5.%s(): illegal amount of arguments",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

 argument_type_error:
  sprintf (vm->error, "MD5.%s(): illegal argument",
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
  if (!set)
    node->type = JS_UNDEFINED;

  return JS_PROPERTY_UNKNOWN;
}

/* New proc. */
static void
new_proc (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info, JSNode *args,
	  JSNode *result_return)
{
  MD5_CTX *instance;

  if (args->u.vinteger != 0)
    {
      sprintf (vm->error, "new MD5(): illegal amount of arguments");
      js_vm_error (vm);
    }

  instance = js_calloc (vm, 1, sizeof (*instance));
  MD5Init (instance);

  js_vm_builtin_create (vm, result_return, builtin_info, instance);
}

/* Delete proc. */
static void
delete_proc (JSBuiltinInfo *builtin_info, void *instance_context)
{
  if (instance_context)
    js_free (instance_context);
}


/*
 * Global functions.
 */

void
js_ext_MD5 (JSInterpPtr interp)
{
  JSBuiltinInfo *info;
  MD5Ctx *ctx;
  JSNode *n;
  JSVirtualMachine *vm = interp->vm;

  ctx = js_calloc (vm, 1, sizeof (*ctx));

  ctx->s_final			= js_vm_intern (vm, "final");
  ctx->s_finalBinary		= js_vm_intern (vm, "finalBinary");
  ctx->s_init			= js_vm_intern (vm, "init");
  ctx->s_update			= js_vm_intern (vm, "update");

  /* Object information. */

  info = js_vm_builtin_info_create (vm);

  info->method_proc		= method;
  info->property_proc		= property;
  info->new_proc		= new_proc;
  info->delete_proc		= delete_proc;
  info->obj_context		= ctx;
  info->obj_context_delete	= js_free;

  /* Define it. */
  n = &vm->globals[js_vm_intern (vm, "MD5")];
  js_vm_builtin_create (vm, n, info, NULL);
}
