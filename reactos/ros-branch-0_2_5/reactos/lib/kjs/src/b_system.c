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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/b_system.c,v $
 * $Id: b_system.c,v 1.1 2004/01/10 20:38:18 arty Exp $
 */

/*
 * Static methods:
 *
 *   chdir (string) => boolean
 *   error ([ANY...]) => undefined
 *   exit (value)
 *   getcwd () => string
 *   getenv (string) => string / undefined
 *   popen (COMMAND, MODE) => file
 *   print ([ANY...]) => undefined
 *   sleep (int) => undefined
 *   strerror (errno) => string
 *   system (string) => integer
 *   usleep (int) => undefined
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

#include "jsint.h"

/*
 * Types and definitions.
 */

#define INSECURE()		\
  do {				\
    if (secure_mode)		\
      goto insecure_feature;	\
  } while (0)

struct system_ctx_st
{
  JSSymbol s_chdir;
  JSSymbol s_error;
  JSSymbol s_exit;
  JSSymbol s_getcwd;
  JSSymbol s_getenv;
  JSSymbol s_popen;
  JSSymbol s_print;
  JSSymbol s_sleep;
  JSSymbol s_strerror;
  JSSymbol s_system;
  JSSymbol s_usleep;

  JSSymbol s_bits;
  JSSymbol s_canonicalHost;
  JSSymbol s_canonicalHostCPU;
  JSSymbol s_canonicalHostVendor;
  JSSymbol s_canonicalHostOS;
  JSSymbol s_errno;
  JSSymbol s_lineBreakSequence;
  JSSymbol s_stderr;
  JSSymbol s_stdin;
  JSSymbol s_stdout;

  /* System file handles. */
  JSNode pstderr;
  JSNode pstdin;
  JSNode pstdout;
};

typedef struct system_ctx_st SystemCtx;


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
  int secure_mode = vm->security & JS_VM_SECURE_SYSTEM;

  /* The default result. */
  result_return->type = JS_UNDEFINED;

  if (method == ctx->s_chdir)
    {
      INSECURE ();

      if (args->u.vinteger != 1)
	goto argument_error;
      if (args[1].type != JS_STRING)
	goto argument_type_error;

      result_return->type= JS_BOOLEAN;

      cp = js_string_to_c_string (vm, &args[1]);
      result_return->u.vboolean = (chdir (cp) == 0);
      js_free (cp);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_exit)
    {
      INSECURE ();

      if (args->u.vinteger != 1)
	goto argument_error;
      if (args[1].type != JS_INTEGER)
	goto argument_type_error;

      /* Exit. */
      exit (args[1].u.vinteger);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_getcwd)
    {
      int size = 10 * 1024;

      INSECURE ();

      if (args->u.vinteger != 0)
	goto argument_error;

      cp = js_malloc (vm, size);
      if (!getcwd (cp, size))
	{
	  result_return->type = JS_BOOLEAN;
	  result_return->u.vboolean = 0;
	  js_free (cp);
	}
      else
	{
	  js_vm_make_string (vm, result_return, cp, strlen (cp));
	  js_free (cp);
	}
    }
  /* ********************************************************************** */
  else if (method == ctx->s_getenv)
    {
      char *val;

      if (args->u.vinteger != 1)
	goto argument_error;
      if (args[1].type != JS_STRING)
	goto argument_type_error;

      cp = js_string_to_c_string (vm, &args[1]);
      val = getenv (cp);
      js_free (cp);

      if (val)
	js_vm_make_string (vm, result_return, val, strlen (val));
    }
  /* ********************************************************************** */
  else if (method == ctx->s_popen)
    {
      char *cmd, *mode;
      FILE *fp;
      int readp = 0;

      INSECURE ();

      if (args->u.vinteger != 2)
	goto argument_error;
      if (args[1].type != JS_STRING || args[2].type != JS_STRING
	  || args[2].u.vstring->len > 10)
	goto argument_type_error;

      cmd = js_string_to_c_string (vm, &args[1]);
      mode = js_string_to_c_string (vm, &args[2]);

      for (i = 0; mode[i]; i++)
	if (mode[i] == 'r')
	  readp = 1;

      JS_VM_ALLOCATE_FD (vm, "System.popen()");
      fp = popen (cmd, mode);

      if (fp == NULL)
	JS_VM_FREE_FD (vm);
      else
	js_builtin_File_new (vm, result_return, cmd,
			     js_iostream_pipe (fp, readp), 0);

      js_free (cmd);
      js_free (mode);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_print || method == ctx->s_error)
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
  else if (method == ctx->s_sleep)
    {
      if (args->u.vinteger != 1)
	goto argument_error;
      if (args[1].type != JS_INTEGER)
	goto argument_type_error;

      sleep (args[1].u.vinteger);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_strerror)
    {
      if (args->u.vinteger != 1)
	goto argument_error;
      if (args[1].type != JS_INTEGER)
	goto argument_type_error;

      cp = strerror (args[1].u.vinteger);
      js_vm_make_string (vm, result_return, cp, strlen (cp));
    }
  /* ********************************************************************** */
  else if (method == ctx->s_system)
    {
      INSECURE ();

      if (args->u.vinteger != 1)
	goto argument_error;
      if (args[1].type != JS_STRING)
	goto argument_type_error;

      result_return->type = JS_INTEGER;

      cp = js_string_to_c_string (vm, &args[1]);
      result_return->u.vinteger = system (cp);
      js_free (cp);
    }
  /* ********************************************************************** */
  else if (method == vm->syms.s_toString)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      js_vm_make_static_string (vm, result_return, "System", 6);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_usleep)
    {
      if (args->u.vinteger != 1)
	goto argument_error;
      if (args[1].type != JS_INTEGER)
	goto argument_type_error;

      usleep (args[1].u.vinteger);
    }
  /* ********************************************************************** */
  else
    return JS_PROPERTY_UNKNOWN;

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

 insecure_feature:
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
  else if (property == ctx->s_errno)
    {
      if (set)
	goto immutable;

      node->type = JS_INTEGER;
      node->u.vinteger = errno;
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
js_builtin_System (JSVirtualMachine *vm)
{
  JSNode *n;
  JSBuiltinInfo *info;
  SystemCtx *ctx;

  ctx = js_calloc (vm, 1, sizeof (*ctx));

  ctx->s_chdir			= js_vm_intern (vm, "chdir");
  ctx->s_error			= js_vm_intern (vm, "error");
  ctx->s_exit			= js_vm_intern (vm, "exit");
  ctx->s_getcwd			= js_vm_intern (vm, "getcwd");
  ctx->s_getenv			= js_vm_intern (vm, "getenv");
  ctx->s_popen			= js_vm_intern (vm, "popen");
  ctx->s_print			= js_vm_intern (vm, "print");
  ctx->s_sleep			= js_vm_intern (vm, "sleep");
  ctx->s_strerror		= js_vm_intern (vm, "strerror");
  ctx->s_system			= js_vm_intern (vm, "system");
  ctx->s_usleep			= js_vm_intern (vm, "usleep");

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
