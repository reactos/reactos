/*
 * The builtin System object.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/ksrc/b_system.c,v $
 * $Id: b_system.c,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

/*
 * Static methods:
 *
 *   print ([ANY...]) => undefined
 *
 * Properties:			type		mutable
 *
 *   bits			integer
 *   canonicalHost		string
 *   canonicalHostCPU		string
 *   canonicalHostVendor	string
 *   canonicalHostOS		string
 *   errno			integer
 *   lineBreakSequence		string
 *   stderr			file
 *   stdin			file
 *   stdout			file
 */

#include "ddk/ntddk.h"
#include "jsint.h"
#include "kjs.h"
#include <rosrtl/string.h>

/*
 * Types and definitions.
 */

#define INSECURE()		\
  do {				\
    if (secure_mode)		\
      goto insecure_feature;	\
  } while (0)

/*
 * Static functions.
 */

/* Method proc */
static int
method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	void *instance_context, JSSymbol method, JSNode *result_return,
	JSNode *args)
{
  SystemCtx *ctx = builtin_info->obj_context;
  int i;
  char *cp;
  /* int secure_mode = vm->security & JS_VM_SECURE_SYSTEM; */

  /* The default result. */
  result_return->type = JS_UNDEFINED;
 
  if (method == ctx->s_mread) 
    {
      if (args->u.vinteger != 2)
	goto argument_error;
      if (args[1].type != JS_INTEGER)
	goto argument_type_error;
      if (args[2].type != JS_INTEGER)
	goto argument_type_error;
      
      if (args[1].u.vinteger == 0) /* String type */
	{
	  result_return->type = JS_STRING;
	  js_vm_make_string (vm, result_return, (char *)args[2].u.vinteger, 
			     strlen ((char *)args[2].u.vinteger));
	}
      else if( args[1].u.vinteger == 1) /* Byte */
	{
	  result_return->type = JS_INTEGER;
	  result_return->u.vinteger = *((BYTE *)args[2].u.vinteger);
	}
      else if( args[1].u.vinteger == 2) /* Word */
	{
	  result_return->type = JS_INTEGER;
	  result_return->u.vinteger = *((WORD *)args[2].u.vinteger);
	}
      else if( args[1].u.vinteger == 4) /* Dword */
	{
	  result_return->type = JS_INTEGER;
	  result_return->u.vinteger = *((DWORD *)args[2].u.vinteger);
	}
    }
  else if (method == ctx->s_mwrite) 
    {
      if (args->u.vinteger != 3)
	goto argument_error;
      if (args[1].type != JS_INTEGER)
	goto argument_type_error;
      if (args[2].type != JS_INTEGER)
	goto argument_type_error;
      
      if (args[1].u.vinteger == 0) /* String type */
	{
	  if (args[3].type != JS_STRING) 
	    goto argument_type_error;

	  result_return->type = JS_BOOLEAN;
	  result_return->u.vinteger = 1;
	  cp = js_string_to_c_string (vm, &args[3]);
	  strcpy((char *)args[2].u.vinteger, cp);
	  js_free(cp);
	}
      else if( args[1].u.vinteger == 1) /* Byte */
	{
	  result_return->type = JS_INTEGER;
	  *((BYTE *)args[2].u.vinteger) = args[3].u.vinteger;
	}
      else if( args[1].u.vinteger == 2) /* Word */
	{
	  result_return->type = JS_INTEGER;
	  *((WORD *)args[2].u.vinteger) = args[3].u.vinteger;
	}
      else if( args[1].u.vinteger == 4) /* Dword */
	{
	  result_return->type = JS_INTEGER;
	  *((DWORD *)args[2].u.vinteger) = args[3].u.vinteger;
	}
    }
  /* ********************************************************************** */
  else if (method == ctx->s_print)
    {
      JSIOStream *stream;

      if (method == ctx->s_print)
	stream = vm->s_stdout;
      else
	stream = vm->s_stderr;

      for (i = 1; i <= args->u.vinteger; i++)
	{
	  JSNode result;

	  js_vm_to_string (vm, &args[i], &result);
	  js_iostream_write (stream, result.u.vstring->data,
			     result.u.vstring->len);
	}
    }
  /* ********************************************************************** */
  else if (method == vm->syms.s_toString)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      js_vm_make_static_string (vm, result_return, "System", 6);
    }
  /* ********************************************************************** */
  else {
    JSSymbolList *cur = ctx->registered_symbols;
    while( cur ) {
      if( cur->symbol == method && cur->registered_function ) {
	  return cur->registered_function
	    (cur->context, result_return, args);
      }
      cur = cur->next;
    }
    return JS_PROPERTY_UNKNOWN;
  }

  return JS_PROPERTY_FOUND;


  /*
   * Error handling.
   */

 argument_error:
  sprintf (vm->error, "System.%s(): illegal amout of arguments",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

 argument_type_error:
  sprintf (vm->error, "System.%s(): illegal argument",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

 /* insecure_feature: */
  sprintf (vm->error, "System.%s(): not allowed in secure mode",
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
  SystemCtx *ctx = builtin_info->obj_context;

  if (property == ctx->s_bits)
    {
      if (set)
	goto immutable;

      node->type = JS_INTEGER;
#if SIZEOF_INT == 2
      node->u.vinteger = 16;
#else /* not SIZEOF_INT == 2 */

#if SIZEOF_LONG == 4
      node->u.vinteger = 32;
#else /* not SIZEOF_LONG == 4 */

#if SIZEOF_LONG == 8
      node->u.vinteger = 64;
#else /* not SIZEOF_LONG == 8 */

      /* Do not know. */
      node->u.vinteger = 0;

#endif /* not SIZEOF_LONG == 8 */
#endif /* not SIZEOF_LONG == 4 */
#endif /* not SIZEOF_INT == 2 */
    }
  else if (property == ctx->s_canonicalHost)
    {
      if (set)
	goto immutable;

      js_vm_make_static_string (vm, node, CANONICAL_HOST,
				strlen (CANONICAL_HOST));
    }
  else if (property == ctx->s_canonicalHostCPU)
    {
      if (set)
	goto immutable;

      js_vm_make_static_string (vm, node, CANONICAL_HOST_CPU,
				strlen (CANONICAL_HOST_CPU));
    }
  else if (property == ctx->s_canonicalHostVendor)
    {
      if (set)
	goto immutable;

      js_vm_make_static_string (vm, node, CANONICAL_HOST_VENDOR,
				strlen (CANONICAL_HOST_VENDOR));
    }
  else if (property == ctx->s_canonicalHostOS)
    {
      if (set)
	goto immutable;

      js_vm_make_static_string (vm, node, CANONICAL_HOST_OS,
				strlen (CANONICAL_HOST_OS));
    }
  else if (property == ctx->s_lineBreakSequence)
    {
      if (set)
	goto immutable;

      js_vm_make_static_string (vm, node, JS_HOST_LINE_BREAK,
				JS_HOST_LINE_BREAK_LEN);
    }
  else if (property == ctx->s_stderr)
    {
      if (set)
	goto immutable;

      JS_COPY (node, &ctx->pstderr);
    }
  else if (property == ctx->s_stdin)
    {
      if (set)
	goto immutable;

      JS_COPY (node, &ctx->pstdin);
    }
  else if (property == ctx->s_stdout)
    {
      if (set)
	goto immutable;

      JS_COPY (node, &ctx->pstdout);
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
  sprintf (vm->error, "System.%s: immutable property",
	   js_vm_symname (vm, property));
  js_vm_error (vm);

  /* NOTREACHED. */
  return 0;
}

/* Mark proc. */
static void
mark (JSBuiltinInfo *builtin_info, void *instance_context)
{
  SystemCtx *ctx = builtin_info->obj_context;

  js_vm_mark (&ctx->pstderr);
  js_vm_mark (&ctx->pstdin);
  js_vm_mark (&ctx->pstdout);
}


/*
 * Global functions.
 */

void
js_builtin_System (PKJS kjs) {
  JSNode *n;
  JSBuiltinInfo *info;
  SystemCtx *ctx;
  JSVirtualMachine *vm = kjs->vm;

  kjs->ctx = (SystemCtx *)js_calloc (vm, 1, sizeof (*ctx));
  ctx = kjs->ctx;
  
  ctx->s_print			= js_vm_intern (vm, "print");
  ctx->s_mread                  = js_vm_intern (vm, "mread");
  ctx->s_mwrite                 = js_vm_intern (vm, "mwrite");
  ctx->s_reg                    = js_vm_intern (vm, "reg");
  ctx->s_regdir                 = js_vm_intern (vm, "regdir");

  ctx->s_bits			= js_vm_intern (vm, "bits");
  ctx->s_canonicalHost		= js_vm_intern (vm, "canonicalHost");
  ctx->s_canonicalHostCPU	= js_vm_intern (vm, "canonicalHostCPU");
  ctx->s_canonicalHostVendor	= js_vm_intern (vm, "canonicalHostVendor");
  ctx->s_canonicalHostOS	= js_vm_intern (vm, "canonicalHostOS");
  ctx->s_errno			= js_vm_intern (vm, "errno");
  ctx->s_lineBreakSequence	= js_vm_intern (vm, "lineBreakSequence");
  ctx->s_stderr			= js_vm_intern (vm, "stderr");
  ctx->s_stdin			= js_vm_intern (vm, "stdin");
  ctx->s_stdout			= js_vm_intern (vm, "stdout");

  /* Object information. */

  info = js_vm_builtin_info_create (vm);

  info->method_proc 		= method;
  info->property_proc		= property;
  info->mark_proc		= mark;
  info->obj_context 		= ctx;
  info->obj_context_delete	= js_free;

  /* Define it. */
  n = &vm->globals[js_vm_intern (vm, "System")];
  js_vm_builtin_create (vm, n, info, NULL);

  /* Enter system properties. */
  js_builtin_File_new (vm, &ctx->pstderr, "stdout", vm->s_stderr, 1);
  js_builtin_File_new (vm, &ctx->pstdin, "stdin", vm->s_stdin, 1);
  js_builtin_File_new (vm, &ctx->pstdout, "stdout", vm->s_stdout, 1);
}
