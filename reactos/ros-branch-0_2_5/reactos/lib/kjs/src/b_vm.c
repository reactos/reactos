/*
 * The builtin VM object.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/b_vm.c,v $
 * $Id: b_vm.c,v 1.1 2004/01/10 20:38:18 arty Exp $
 */

/*
 * Class:
 *
 * - static methods:
 *
 *   VM.garbageCollect ()
 *   VM.stackTrace ()
 *
 * - properties:		type		mutable
 *
 *   VM.dispatchMethod		string
 *   VM.fdCount                 integer
 *   VM.gcCount			integer
 *   VM.gcTrigger		integer		yes
 *   VM.heapAllocated		integer
 *   VM.heapFree		integer
 *   VM.heapSize		integer
 *   VM.numConstants		integer
 *   VM.numGlobals		integer
 *   VM.stackSize		integer
 *   VM.stacktraceOnError	boolean		yes
 *   VM.verbose			integer 	yes
 *   VM.verboseStacktrace	boolean		yes
 *   VM.version			string
 *   VM.versionMajor		integer
 *   VM.versionMinor		integer
 *   VM.versionPatch		integer
 *   VM.warnUndef		boolean		yes
 */

#include "jsint.h"

/*
 * Types and definitions.
 */

struct vm_ctx_st
{
  JSSymbol s_garbageCollect;
  JSSymbol s_stackTrace;

  JSSymbol s_dispatchMethod;
  JSSymbol s_fdCount;
  JSSymbol s_gcCount;
  JSSymbol s_gcTrigger;
  JSSymbol s_heapAllocated;
  JSSymbol s_heapFree;
  JSSymbol s_heapSize;
  JSSymbol s_numConstants;
  JSSymbol s_numGlobals;
  JSSymbol s_stackSize;
  JSSymbol s_stacktraceOnError;
  JSSymbol s_verbose;
  JSSymbol s_verboseStacktrace;
  JSSymbol s_version;
  JSSymbol s_versionMajor;
  JSSymbol s_versionMinor;
  JSSymbol s_versionPatch;
  JSSymbol s_warnUndef;
};

typedef struct vm_ctx_st VMCtx;


/*
 * Static functions.
 */

/* Method proc */
static int
method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	void *instance_context, JSSymbol method, JSNode *result_return,
	JSNode *args)
{
  VMCtx *ctx = builtin_info->obj_context;

  /* The default return value is undefined. */
  result_return->type = JS_UNDEFINED;

  if (method == ctx->s_garbageCollect)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      /* Ok, let's trigger a garbage collection. */
      vm->gc.bytes_allocated = vm->gc.trigger + 1;
    }
  /* ********************************************************************** */
  else if (method == ctx->s_stackTrace)
    {
      unsigned int limit;

      if (args->u.vinteger == 0)
	limit = -1;
      else if (args->u.vinteger == 1)
	{
	  if (args[1].type != JS_INTEGER)
	    goto argument_type_error;

	  limit = args[1].u.vinteger;
	}
      else
	goto argument_error;

      js_vm_stacktrace (vm, limit);
    }
  /* ********************************************************************** */
  else if (method == vm->syms.s_toString)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      js_vm_make_static_string (vm, result_return, "VM", 2);
    }
  /* ********************************************************************** */
  else
    return JS_PROPERTY_UNKNOWN;

  return JS_PROPERTY_FOUND;


  /*
   * Error handling.
   */

 argument_error:
  sprintf (vm->error, "VM.%s(): illegal amout of arguments",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

 argument_type_error:
  sprintf (vm->error, "VM.%s(): illegal argument",
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
  VMCtx *ctx = builtin_info->obj_context;

  if (property == ctx->s_dispatchMethod)
    {
      if (set)
	goto immutable;

      js_vm_make_static_string (vm, node,
				vm->dispatch_method_name,
				strlen (vm->dispatch_method_name));
    }
  /* ***************************************************************** */
  else if (property == ctx->s_fdCount)
    {
      if (set)
	goto immutable;

      node->type = JS_INTEGER;
      node->u.vinteger = vm->fd_count;
    }
  /* ***************************************************************** */
  else if (property == ctx->s_gcCount)
    {
      if (set)
	goto immutable;

      node->type = JS_INTEGER;
      node->u.vinteger = vm->gc.count;
    }
  /* ***************************************************************** */
  else if (property == ctx->s_gcTrigger)
    {
      if (set)
	{
	  if (node->type != JS_INTEGER)
	    goto value_error;
	  vm->gc.trigger = node->u.vinteger;
	}
      else
	{
	  node->type = JS_INTEGER;
	  node->u.vinteger = vm->gc.trigger;
	}
    }
  /* ***************************************************************** */
  else if (property == ctx->s_heapAllocated)
    {
      if (set)
	goto immutable;

      node->type = JS_INTEGER;
      node->u.vinteger = vm->gc.bytes_allocated;
    }
  /* ***************************************************************** */
  else if (property == ctx->s_heapFree)
    {
      if (set)
	goto immutable;

      node->type = JS_INTEGER;
      node->u.vinteger = vm->gc.bytes_free;
    }
  /* ***************************************************************** */
  else if (property == ctx->s_heapSize)
    {
      if (set)
	goto immutable;

      node->type = JS_INTEGER;
      node->u.vinteger = vm->heap_size;
    }
  /* ***************************************************************** */
  else if (property == ctx->s_numConstants)
    {
      if (set)
	goto immutable;

      node->type = JS_INTEGER;
      node->u.vinteger = vm->num_consts;
    }
  /* ***************************************************************** */
  else if (property == ctx->s_numGlobals)
    {
      if (set)
	goto immutable;

      node->type = JS_INTEGER;
      node->u.vinteger = vm->num_globals;
    }
  /* ***************************************************************** */
  else if (property == ctx->s_stackSize)
    {
      if (set)
	goto immutable;

      node->type = JS_INTEGER;
      node->u.vinteger = vm->stack_size;
    }
  /* ***************************************************************** */
  else if (property == ctx->s_stacktraceOnError)
    {
      if (set)
	{
	  if (node->type != JS_BOOLEAN)
	    goto value_error;
	  vm->stacktrace_on_error = node->u.vboolean;
	}
      else
	{
	  node->type = JS_BOOLEAN;
	  node->u.vboolean = vm->stacktrace_on_error;
	}
    }
  /* ***************************************************************** */
  else if (property == ctx->s_verbose)
    {
      if (set)
	{
	  if (node->type != JS_INTEGER)
	    goto value_error;
	  vm->verbose = node->u.vinteger;
	}
      else
	{
	  node->type = JS_INTEGER;
	  node->u.vinteger = vm->verbose;
	}
    }
  /* ***************************************************************** */
  else if (property == ctx->s_verboseStacktrace)
    {
      if (set)
	{
	  if (node->type != JS_BOOLEAN)
	    goto value_error;
	  vm->verbose_stacktrace = node->u.vboolean;
	}
      else
	{
	  node->type = JS_BOOLEAN;
	  node->u.vboolean = vm->verbose_stacktrace;
	}
    }
  /* ***************************************************************** */
  else if (property == ctx->s_version)
    {
      if (set)
	goto immutable;

      js_vm_make_static_string (vm, node, VERSION, strlen (VERSION));
    }
  /* ***************************************************************** */
  else if (property == ctx->s_versionMajor)
    {
      if (set)
	goto immutable;

      node->type = JS_INTEGER;
      node->u.vinteger = atoi (VERSION);
    }
  /* ***************************************************************** */
  else if (property == ctx->s_versionMinor)
    {
      if (set)
	goto immutable;

      node->type = JS_INTEGER;
      node->u.vinteger = atoi (VERSION + 2);
    }
  /* ***************************************************************** */
  else if (property == ctx->s_versionPatch)
    {
      if (set)
	goto immutable;

      node->type = JS_INTEGER;
      node->u.vinteger = atoi (VERSION + 4);
    }
  /* ***************************************************************** */
  else if (property == ctx->s_warnUndef)
    {
      if (set)
	{
	  if (node->type != JS_INTEGER)
	    goto value_error;
	  vm->warn_undef = node->u.vinteger != 0;
	}
      else
	{
	  node->type = JS_INTEGER;
	  node->u.vinteger = vm->warn_undef;
	}
    }
  /* ***************************************************************** */
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

 value_error:
  sprintf (vm->error, "VM.%s: illegal value",
	   js_vm_symname (vm, property));
  js_vm_error (vm);

 immutable:
  sprintf (vm->error, "VM.%s: immutable property",
	   js_vm_symname (vm, property));
  js_vm_error (vm);

  /* NOTREACHED. */
  return 0;
}


/*
 * Global functions.
 */

void
js_builtin_VM (JSVirtualMachine *vm)
{
  JSNode *n;
  JSBuiltinInfo *info;
  VMCtx *ctx;

  ctx = js_calloc (vm, 1, sizeof (*ctx));

  ctx->s_garbageCollect		= js_vm_intern (vm, "garbageCollect");
  ctx->s_stackTrace		= js_vm_intern (vm, "stackTrace");

  ctx->s_dispatchMethod		= js_vm_intern (vm, "dispatchMethod");
  ctx->s_fdCount		= js_vm_intern (vm, "fdCount");
  ctx->s_gcCount		= js_vm_intern (vm, "gcCount");
  ctx->s_gcTrigger		= js_vm_intern (vm, "gcTrigger");
  ctx->s_heapAllocated		= js_vm_intern (vm, "heapAllocated");
  ctx->s_heapFree		= js_vm_intern (vm, "heapFree");
  ctx->s_heapSize		= js_vm_intern (vm, "heapSize");
  ctx->s_numConstants		= js_vm_intern (vm, "numConstants");
  ctx->s_numGlobals		= js_vm_intern (vm, "numGlobals");
  ctx->s_stackSize		= js_vm_intern (vm, "stackSize");
  ctx->s_stacktraceOnError	= js_vm_intern (vm, "stacktraceOnError");
  ctx->s_verbose		= js_vm_intern (vm, "verbose");
  ctx->s_verboseStacktrace	= js_vm_intern (vm, "verboseStacktrace");
  ctx->s_version		= js_vm_intern (vm, "version");
  ctx->s_versionMajor		= js_vm_intern (vm, "versionMajor");
  ctx->s_versionMinor		= js_vm_intern (vm, "versionMinor");
  ctx->s_versionPatch		= js_vm_intern (vm, "versionPatch");
  ctx->s_warnUndef		= js_vm_intern (vm, "warnUndef");

  /* Object information. */

  info = js_vm_builtin_info_create (vm);

  info->method_proc 		= method;
  info->property_proc 		= property;
  info->obj_context 		= ctx;
  info->obj_context_delete	= js_free;

  /* Define it. */
  n = &vm->globals[js_vm_intern (vm, "VM")];
  js_vm_builtin_create (vm, n, info, NULL);
}
