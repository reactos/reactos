/*
 * The builtin Directory object.
 * Copyright (c) 1998-1999 New Generation Software (NGS) Oy
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/b_dir.c,v $
 * $Id: b_dir.c,v 1.1 2004/01/10 20:38:18 arty Exp $
 */

#include "jsint.h"

/*
 * Types and definitions.
 */

/* Class context. */
struct dir_ctx_st
{
  /* Methods. */
  JSSymbol s_close;
  JSSymbol s_open;
  JSSymbol s_read;
  JSSymbol s_rewind;
  JSSymbol s_seek;
  JSSymbol s_tell;
};

typedef struct dir_ctx_st DirCtx;

/* Instance context. */
struct dir_instance_ctx_st
{
  DIR *dir;
  char *path;

  /* The virtual machine handle is needed for the delete_proc. */
  JSVirtualMachine *vm;
};

typedef struct dir_instance_ctx_st DirInstanceCtx;


/*
 * Static functions.
 */

/* Method proc. */
static int
method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	void *instance_context, JSSymbol method, JSNode *result_return,
	JSNode *args)
{
  DirCtx *ctx = builtin_info->obj_context;
  DirInstanceCtx *ictx = instance_context;

  /* Static methods. */
  if (method == vm->syms.s_toString)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      if (ictx)
	js_vm_make_string (vm, result_return, ictx->path, strlen (ictx->path));
      else
	js_vm_make_static_string (vm, result_return, "Directory", 9);
    }
  /* ********************************************************************** */
  else if (ictx)
    {
      /*
       * Instance methods.
       */
      if (method == ctx->s_close)
	{
	  if (args->u.vinteger != 0)
	    goto argument_error;

	  if (ictx->dir)
	    if (closedir (ictx->dir) >= 0)
	      {
		ictx->dir = NULL;
		JS_VM_FREE_FD (vm);
	      }

	  result_return->type = JS_BOOLEAN;
	  result_return->u.vboolean = ictx->dir == NULL;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_open)
	{
	  if (args->u.vinteger != 0)
	    goto argument_error;

	  if (ictx->dir == NULL)
	    {
	      JS_VM_ALLOCATE_FD (vm, "Directory.open()");
	      ictx->dir = opendir (ictx->path);

	      if (ictx->dir == NULL)
		/* Directory opening failed. */
		JS_VM_FREE_FD (vm);
	    }

	  result_return->type = JS_BOOLEAN;
	  result_return->u.vboolean = ictx->dir != NULL;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_read)
	{
	  struct dirent *de;

	  if (args->u.vinteger != 0)
	    goto argument_error;
	  if (ictx->dir == NULL)
	    goto not_open;

	  de = readdir (ictx->dir);
	  if (de)
	    js_vm_make_string (vm, result_return, de->d_name,
			       strlen (de->d_name));
	  else
	    {
	      result_return->type = JS_BOOLEAN;
	      result_return->u.vboolean = 0;
	    }
	}
      /* ***************************************************************** */
      else if (method == ctx->s_rewind)
	{
	  if (args->u.vinteger != 0)
	    goto argument_error;
	  if (ictx->dir == NULL)
	    goto not_open;

	  rewinddir (ictx->dir);
	  result_return->type = JS_UNDEFINED;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_seek)
	{
	  if (args->u.vinteger != 1)
	    goto argument_error;
	  if (args[1].type != JS_INTEGER)
	    goto argument_type_error;
	  if (ictx->dir == NULL)
	    goto not_open;

	  seekdir (ictx->dir, args[1].u.vinteger);
	  result_return->type = JS_UNDEFINED;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_tell)
	{
	  if (args->u.vinteger != 0)
	    goto argument_error;
	  if (ictx->dir == NULL)
	    goto not_open;

	  result_return->u.vinteger = telldir (ictx->dir);
	  if (result_return->u.vinteger < 0)
	    {
	      result_return->type = JS_BOOLEAN;
	      result_return->u.vboolean = 0;
	    }
	  else
	    result_return->type = JS_INTEGER;
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
  sprintf (vm->error, "Directory.%s(): illegal amount of arguments",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

 argument_type_error:
  sprintf (vm->error, "Directory.%s(): illegal argument",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

 not_open:
  sprintf (vm->error, "Directory.%s(): directory is no opened",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

 insecure_feature:
  sprintf (vm->error, "Directory.%s(): not allowed in secure mode",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

  /* NOTREACHED */
  return 0;
}


/* Property proc. */
static int
property (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	  void *instance_context, JSSymbol property, int set, JSNode *node)
{
  /* We have no properties. */

  if (!set)
    node->type = JS_UNDEFINED;

  return JS_PROPERTY_UNKNOWN;
}

/* New proc. */
static void
new_proc (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info, JSNode *args,
	  JSNode *result_return)
{
  DirInstanceCtx *instance;
  if (args->u.vinteger != 1)
    {
      sprintf (vm->error, "new Directory(): illegal amount of arguments");
      js_vm_error (vm);
    }
  if (args[1].type != JS_STRING)
    {
      sprintf (vm->error, "new Directory(): illegal argument");
      js_vm_error (vm);
    }

  instance = js_calloc (vm, 1, sizeof (*instance));
  instance->path = js_string_to_c_string (vm, &args[1]);
  instance->vm = vm;

  js_vm_builtin_create (vm, result_return, builtin_info, instance);
}

/* Delete proc. */
static void
delete_proc (JSBuiltinInfo *builtin_info, void *instance_context)
{
  DirInstanceCtx *ictx = instance_context;

  if (ictx)
    {
      if (ictx->dir)
	{
	  closedir (ictx->dir);
	  JS_VM_FREE_FD (ictx->vm);
	}

      js_free (ictx->path);
      js_free (ictx);
    }
}


/*
 * Global functions.
 */

void
js_builtin_Directory (JSVirtualMachine *vm)
{
  JSNode *n;
  JSBuiltinInfo *info;
  DirCtx *ctx;

  ctx = js_calloc (vm, 1, sizeof (*ctx));

  ctx->s_close		= js_vm_intern (vm, "close");
  ctx->s_open		= js_vm_intern (vm, "open");
  ctx->s_read		= js_vm_intern (vm, "read");
  ctx->s_rewind		= js_vm_intern (vm, "rewind");
  ctx->s_seek		= js_vm_intern (vm, "seek");
  ctx->s_tell		= js_vm_intern (vm, "tell");

  /* Object information. */

  info = js_vm_builtin_info_create (vm);

  info->method_proc 		= method;
  info->property_proc 		= property;
  info->new_proc 		= new_proc;
  info->delete_proc 		= delete_proc;
  info->obj_context 		= ctx;
  info->obj_context_delete	= js_free;

  /* Define it. */
  n = &vm->globals[js_vm_intern (vm, "Directory")];
  js_vm_builtin_create (vm, n, info, NULL);
}
