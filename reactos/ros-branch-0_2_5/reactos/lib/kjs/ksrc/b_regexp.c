/*
 * The builtin RegExp object.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/ksrc/b_regexp.c,v $
 * $Id: b_regexp.c,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

#include "jsint.h"
#include "regex.h"

/*
 * Types and definitions.
 */

/* These must be in sync with the values, found from `../jsc/asm.js'. */
#define JS_REGEXP_FLAG_G	0x01
#define JS_REGEXP_FLAG_I	0x02

/* Class context. */
struct regexp_ctx_st
{
  /* Static properties. */
  JSSymbol s_S1;
  JSSymbol s_S2;
  JSSymbol s_S3;
  JSSymbol s_S4;
  JSSymbol s_S5;
  JSSymbol s_S6;
  JSSymbol s_S7;
  JSSymbol s_S8;
  JSSymbol s_S9;
  JSSymbol s_S_;
  JSSymbol s_input;
  JSSymbol s_lastMatch;
  JSSymbol s_lastParen;
  JSSymbol s_leftContext;
  JSSymbol s_multiline;
  JSSymbol s_rightContext;

  /* Properties. */
  JSSymbol s_global;
  JSSymbol s_ignoreCase;
  JSSymbol s_lastIndex;
  JSSymbol s_source;

  /* Methods. */
  JSSymbol s_compile;
  JSSymbol s_exec;
  JSSymbol s_test;

  /* Data that is needed for the static properties. */

  JSNode input;
  struct re_registers regs;
};

typedef struct regexp_ctx_st RegexpCtx;

/* RegExp instance context. */
struct regexp_instance_ctx_st
{
  /* The source for this regexp. */
  char *source;
  unsigned int source_len;

  /* Flags. */
  unsigned int global : 1;
  unsigned int ignore_case : 1;
  unsigned int immutable : 1;

  /* Compiled pattern. */
  struct re_pattern_buffer compiled;

  /* The index from which the next match is started. */
  unsigned int last_index;
};

typedef struct regexp_instance_ctx_st RegexpInstanceCtx;


/*
 * Prorototypes for some static functions.
 */

static void new_proc (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
		      JSNode *args, JSNode *result_return);


/*
 * Static functions.
 */

/* A helper for RegExp.exec().  XXX Check the compliancy. */
static void
do_exec (JSVirtualMachine *vm, RegexpCtx *ctx, RegexpInstanceCtx *ictx,
	 char *input, unsigned int input_len, JSNode *result_return)
{
  int result;
  int i, j;

  result = re_search (&ictx->compiled, input, input_len,
		      ictx->global ? ictx->last_index : 0,
		      input_len, &ctx->regs);

  if (result < 0)
    {
      result_return->type = JS_NULL;
      return;
    }

  /* Success.  Count how many matches we had. */
  for (i = 0; i < ctx->regs.num_regs && ctx->regs.start[i] >= 0; i++)
    ;

  /* Create the result array and enter the sub-matches. */
  js_vm_make_array (vm, result_return, i);

  for (j = 0; j < i; j++)
    js_vm_make_string (vm, &result_return->u.varray->data[j],
		       input + ctx->regs.start[j],
		       ctx->regs.end[j] - ctx->regs.start[j]);

  ictx->last_index = ctx->regs.end[0];
}

/* Method proc. */
static int
method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	void *instance_context, JSSymbol method, JSNode *result_return,
	JSNode *args)
{
  RegexpCtx *ctx = builtin_info->obj_context;
  RegexpInstanceCtx *ictx = instance_context;
  int result;
  int i;

  /* Set the default return value. */
  result_return->type = JS_BOOLEAN;
  result_return->u.vboolean = 1;

  /* Static methods. */
  if (method == vm->syms.s_toString)
    {
      if (ictx)
	js_vm_make_string (vm, result_return, ictx->source, ictx->source_len);
      else
	js_vm_make_static_string (vm, result_return, "RegExp", 6);
    }
  /* ********************************************************************** */
  else if (ictx)
    {
      /* Methods */

      if (method == ctx->s_compile)
	{
	  int global = 0;
	  int ignore_case = 0;
	  const char *error;
	  JSNode *pattern;
	  JSNode pattern_cvt;

	  if (ictx->immutable)
	    goto immutable;

	  if (args->u.vinteger != 1 && args->u.vinteger != 2)
	    goto argument_error;

	  if (args[1].type == JS_STRING)
	    pattern = &args[1];
	  else
	    {
	      js_vm_to_string (vm, &args[1], &pattern_cvt);
	      pattern = &pattern_cvt;
	    }

	  if (args->u.vinteger == 2)
	    {
	      JSNode *flags;
	      JSNode cvt;

	      if (args[2].type == JS_STRING)
		flags = &args[2];
	      else
		{
		  js_vm_to_string (vm, &args[2], &cvt);
		  flags = &cvt;
		}

	      for (i = 0; i < flags->u.vstring->len; i++)
		switch (flags->u.vstring->data[i])
		  {
		  case 'g':
		    global = 1;
		    break;

		  case 'i':
		    ignore_case = 1;
		    break;

		  default:
		    sprintf (vm->error, "new RegExp(): illegal flag `%c'",
			     flags->u.vstring->data[i]);
		    js_vm_error (vm);
		    break;
		  }
	    }

	  if (ictx->source)
	    js_free (ictx->source);

	  ictx->source_len = pattern->u.vstring->len;
	  ictx->source = js_malloc (vm, ictx->source_len);
	  memcpy (ictx->source, pattern->u.vstring->data, ictx->source_len);

	  ictx->global = global;
	  ictx->ignore_case = ignore_case;

	  if (ictx->compiled.fastmap)
	    js_free (ictx->compiled.fastmap);

	  memset (&ictx->compiled, 0, sizeof (ictx->compiled));

	  if (ictx->ignore_case)
	    ictx->compiled.translate = js_latin1_tolower;

	  error = re_compile_pattern (ictx->source, ictx->source_len,
				      &ictx->compiled);
	  if (error)
	    {
	      sprintf (vm->error,
		       "RegExp.%s(): compilation of the expression failed: %s",
		       js_vm_symname (vm, method), error);
	      js_vm_error (vm);
	    }
	  ictx->compiled.fastmap = js_malloc (vm, 256);
	  re_compile_fastmap (&ictx->compiled);
	}
      /* ***************************************************************** */
      else if (method == ctx->s_exec)
	{
	  char *input;
	  unsigned int input_len;
	  JSNode *input_str;
	  JSNode cvt;

	  if (args->u.vinteger == 0)
	    {
	      if (ctx->input.type == JS_STRING)
		input_str = &ctx->input;
	      else
		{
		  js_vm_to_string (vm, &ctx->input, &cvt);
		  input_str = &cvt;
		}

	      input = input_str->u.vstring->data;
	      input_len = input_str->u.vstring->len;
	    }
	  else if (args->u.vinteger == 1)
	    {
	      if (args[1].type == JS_STRING)
		input_str = &args[1];
	      else
		{
		  js_vm_to_string (vm, &args[1], &cvt);
		  input_str = &cvt;
		}

	      input = input_str->u.vstring->data;
	      input_len = input_str->u.vstring->len;

	      /* Set the input property to the class context. */
	      JS_COPY (&ctx->input, input_str);
	    }
	  else
	    goto argument_error;

	  do_exec (vm, ctx, ictx, input, input_len, result_return);
	}
      /* ***************************************************************** */
      else if (method == ctx->s_test)
	{
	  char *input;
	  unsigned int input_len;
	  JSNode *input_str;
	  JSNode cvt;

	  if (args->u.vinteger == 0)
	    {
	      if (ctx->input.type == JS_STRING)
		input_str = &ctx->input;
	      else
		{
		  js_vm_to_string (vm, &ctx->input, &cvt);
		  input_str = &cvt;
		}

	      input = input_str->u.vstring->data;
	      input_len = input_str->u.vstring->len;
	    }
	  else if (args->u.vinteger == 1)
	    {
	      if (args[1].type == JS_STRING)
		input_str = &args[1];
	      else
		{
		  js_vm_to_string (vm, &args[1], &cvt);
		  input_str = &cvt;
		}

	      input = input_str->u.vstring->data;
	      input_len = input_str->u.vstring->len;

	      /* Set the input property to the class context. */
	      JS_COPY (&ctx->input, input_str);
	    }
	  else
	    goto argument_error;

	  result = re_search (&ictx->compiled, input, input_len,
			      ictx->global ? ictx->last_index : 0,
			      input_len, &ctx->regs);

	  result_return->type = JS_BOOLEAN;
	  result_return->u.vboolean = result >= 0;

	  if (result >= 0)
	    /* ctx->regs.num_regs can be 0.  Or can it??? */
	    ictx->last_index = ctx->regs.end[0];
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
  sprintf (vm->error, "RegExp.%s(): illegal amount of arguments",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

 /* argument_type_error: */
  sprintf (vm->error, "RegExp.%s(): illegal argument",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

 immutable:
  sprintf (vm->error, "RegExp.%s(): immutable object",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

  /* NOTREACHED. */
  return 0;
}

/* Global method proc. */
static void
global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	       void *instance_context, JSNode *result_return,
	       JSNode *args)
{
  RegexpCtx *ctx = builtin_info->obj_context;
  RegexpInstanceCtx *ictx = instance_context;
  char *input = NULL;		/* Initialized to keep the compiler quiet. */
  unsigned int input_len = 0;	/* Likewise. */

  if (ictx)
    {
      /* A RegExp instance was called as a function. */

      if (args->u.vinteger == 0)
	{
	  if (ctx->input.type != JS_STRING)
	    {
	      sprintf (vm->error, "RegExp(): RegExp.input is not a string");
	      js_vm_error (vm);
	    }
	  input = ctx->input.u.vstring->data;
	  input_len = ctx->input.u.vstring->len;
	}
      else if (args->u.vinteger == 1)
	{
	  if (args[1].type != JS_STRING)
	    {
	      sprintf (vm->error, "RegExp(): illegal argument");
	      js_vm_error (vm);
	    }

	  input = args[1].u.vstring->data;
	  input_len = args[1].u.vstring->len;

	  /* Set the input property to the class context. */
	  JS_COPY (&ctx->input, &args[1]);
	}
      else
	{
	  sprintf (vm->error, "RegExp(): illegal amount of arguments");
	  js_vm_error (vm);
	}

      do_exec (vm, ctx, ictx, input, input_len, result_return);
    }
  else
    {
      /*
       * The `RegExp' was called as a function.  We do exactly the
       * same the `new RegExp()' would do with our arguments.
       */
      new_proc (vm, builtin_info, args, result_return);
    }
}

/* Property proc. */
static int
property (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	  void *instance_context, JSSymbol property, int set, JSNode *node)
{
  RegexpCtx *ctx = builtin_info->obj_context;
  RegexpInstanceCtx *ictx = instance_context;
  int index;

  /* Static properties. */
  if (property == ctx->s_S1)
    {
      index = 1;

    dollar_index:

      if (set)
	goto immutable;

      if (ctx->input.type != JS_STRING
	  || ctx->regs.end[0] > ctx->input.u.vstring->len
	  || ctx->regs.start[index] < 0)
	node->type = JS_UNDEFINED;
      else
	js_vm_make_string (vm, node,
			   ctx->input.u.vstring->data
			   + ctx->regs.start[index],
			   ctx->regs.end[index] - ctx->regs.start[index]);
    }
  /* ********************************************************************** */
  else if (property == ctx->s_S2)
    {
      index = 2;
      goto dollar_index;
    }
  /* ********************************************************************** */
  else if (property == ctx->s_S3)
    {
      index = 3;
      goto dollar_index;
    }
  /* ********************************************************************** */
  else if (property == ctx->s_S4)
    {
      index = 4;
      goto dollar_index;
    }
  /* ********************************************************************** */
  else if (property == ctx->s_S5)
    {
      index = 5;
      goto dollar_index;
    }
  /* ********************************************************************** */
  else if (property == ctx->s_S6)
    {
      index = 6;
      goto dollar_index;
    }
  /* ********************************************************************** */
  else if (property == ctx->s_S7)
    {
      index = 7;
      goto dollar_index;
    }
  /* ********************************************************************** */
  else if (property == ctx->s_S8)
    {
      index = 8;
      goto dollar_index;
    }
  /* ********************************************************************** */
  else if (property == ctx->s_S9)
    {
      index = 9;
      goto dollar_index;
    }
  /* ********************************************************************** */
  else if (property == ctx->s_S_ || property == ctx->s_input)
    {
      if (set)
	{
	  if (node->type != JS_STRING)
	    goto argument_type_error;

	  JS_COPY (&ctx->input, node);
	}
      else
	JS_COPY (node, &ctx->input);
    }
  /* ********************************************************************** */
  else if (property == ctx->s_lastMatch)
    {
      if (set)
	goto immutable;

      if (ctx->input.type != JS_STRING
	  || ctx->regs.end[0] > ctx->input.u.vstring->len)
	node->type = JS_UNDEFINED;
      else
	js_vm_make_string (vm, node,
			   ctx->input.u.vstring->data + ctx->regs.start[0],
			   ctx->regs.end[0] - ctx->regs.start[0]);
    }
  /* ********************************************************************** */
  else if (property == ctx->s_lastParen)
    {
      if (set)
	goto immutable;

      if (ctx->input.type != JS_STRING
	  || ctx->regs.end[0] > ctx->input.u.vstring->len)
	node->type = JS_UNDEFINED;
      else
	{
	  int i;

	  for (i = 1; i < ctx->regs.num_regs && ctx->regs.start[i] >= 0; i++)
	    ;
	  i--;
	  if (i == 0)
	    node->type = JS_UNDEFINED;
	  else
	    js_vm_make_string (vm, node,
			       ctx->input.u.vstring->data
			       + ctx->regs.start[i],
			       ctx->regs.end[i] - ctx->regs.start[i]);
	}
    }
  /* ********************************************************************** */
  else if (property == ctx->s_leftContext)
    {
      if (set)
	goto immutable;

      if (ctx->input.type != JS_STRING
	  || ctx->regs.end[0] > ctx->input.u.vstring->len)
	node->type = JS_UNDEFINED;
      else
	js_vm_make_string (vm, node, ctx->input.u.vstring->data,
			   ctx->regs.start[0]);
    }
  /* ********************************************************************** */
  else if (property == ctx->s_multiline)
    {
      goto not_implemented_yet;
    }
  /* ********************************************************************** */
  else if (property == ctx->s_rightContext)
    {
      if (set)
	goto immutable;
      if (ctx->input.type != JS_STRING
	  || ctx->regs.end[0] > ctx->input.u.vstring->len)
	node->type = JS_UNDEFINED;
      else
	js_vm_make_string (vm, node,
			   ctx->input.u.vstring->data + ctx->regs.end[0],
			   ctx->input.u.vstring->len - ctx->regs.end[0]);
    }
  /* ********************************************************************** */
  else if (ictx)
    {
      /* Properties. */
      if (property == ctx->s_global)
	{
	  if (set)
	    goto immutable;

	  node->type = JS_BOOLEAN;
	  node->u.vboolean = ictx->global;
	}
      /* ***************************************************************** */
      else if (property == ctx->s_ignoreCase)
	{
	  if (set)
	    goto immutable;

	  node->type = JS_BOOLEAN;
	  node->u.vboolean = ictx->ignore_case;
	}
      /* ***************************************************************** */
      else if (property == ctx->s_lastIndex)
	{
	  if (set)
	    {
	      if (ictx->immutable)
		goto immutable_object;

	      if (node->type != JS_INTEGER)
		goto argument_type_error;

	      ictx->last_index = node->u.vinteger;
	    }
	  else
	    {
	      node->type = JS_INTEGER;
	      node->u.vinteger = ictx->last_index;
	    }
	}
      /* ***************************************************************** */
      else if (property == ctx->s_source)
	{
	  if (set)
	    goto immutable;

	  js_vm_make_string (vm, node, ictx->source, ictx->source_len);
	}
      /* ***************************************************************** */
      else
	{
	  if (!set)
	    node->type = JS_UNDEFINED;

	  return JS_PROPERTY_UNKNOWN;
	}
    }
  /* ********************************************************************** */
  else
    {
      if (!set)
	node->type = JS_UNDEFINED;

      return JS_PROPERTY_UNKNOWN;
    }

  return JS_PROPERTY_FOUND;


  /* Error handling. */

 argument_type_error:
  sprintf (vm->error, "RegExp.%s: illegal value",
	   js_vm_symname (vm, property));
  js_vm_error (vm);

 not_implemented_yet:
  sprintf (vm->error, "RegExp.%s: not implemented yet",
	   js_vm_symname (vm, property));
  js_vm_error (vm);

 immutable:
  sprintf (vm->error, "RegExp.%s: immutable property",
	   js_vm_symname (vm, property));
  js_vm_error (vm);

 immutable_object:
  sprintf (vm->error, "RegExp.%s: immutable object",
	   js_vm_symname (vm, property));
  js_vm_error (vm);

  /* NOTREACHED */
  return 0;
}

/* New proc. */
static void
new_proc (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info, JSNode *args,
	  JSNode *result_return)
{
  unsigned int flags = 0;
  char *source;
  unsigned int source_len;
  int i;

  if (args->u.vinteger > 2)
    {
      sprintf (vm->error, "new RegExp(): illegal amount of arguments");
      js_vm_error (vm);
    }

  if (args->u.vinteger == 0)
    {
      source = "";
      source_len = 0;
    }
  else
    {
      if (args[1].type != JS_STRING)
	{
	argument_type_error:
	  sprintf (vm->error, "new RegExp(): illegal argument");
	  js_vm_error (vm);
	}

      source = args[1].u.vstring->data;
      source_len = args[1].u.vstring->len;
    }

  if (args->u.vinteger == 2)
    {
      if (args[2].type != JS_STRING)
	goto argument_type_error;

      for (i = 0; i < args[2].u.vstring->len; i++)
	switch (args[2].u.vstring->data[i])
	  {
	  case 'g':
	    flags |= JS_REGEXP_FLAG_G;
	    break;

	  case 'i':
	    flags |= JS_REGEXP_FLAG_I;
	    break;

	  default:
	    sprintf (vm->error, "new RegExp(): illegal flag `%c'",
		     args[2].u.vstring->data[i]);
	    js_vm_error (vm);
	    break;
	  }
    }

  js_builtin_RegExp_new (vm, source, source_len, flags, 0, builtin_info,
			 result_return);
}

/* Delete proc. */
static void
delete_proc (JSBuiltinInfo *builtin_info, void *instance_context)
{
  RegexpInstanceCtx *ictx = instance_context;

  if (ictx)
    {
      js_free (ictx->source);

      if (ictx->compiled.buffer)
	js_free (ictx->compiled.buffer);
      if (ictx->compiled.fastmap)
	js_free (ictx->compiled.fastmap);

      js_free (ictx);
    }
}

/* Mark proc. */
static void
mark (JSBuiltinInfo *builtin_info, void *instance_context)
{
  RegexpCtx *ctx = builtin_info->obj_context;

  js_vm_mark (&ctx->input);
}

/*
 * Global functions.
 */

void
js_builtin_RegExp (JSVirtualMachine *vm)
{
  JSBuiltinInfo *info;
  RegexpCtx *ctx;
  JSNode *n;

  ctx = js_calloc (vm, 1, sizeof (*ctx));

  ctx->s_S1		= js_vm_intern (vm, "$1");
  ctx->s_S2		= js_vm_intern (vm, "$2");
  ctx->s_S3		= js_vm_intern (vm, "$3");
  ctx->s_S4		= js_vm_intern (vm, "$4");
  ctx->s_S5		= js_vm_intern (vm, "$5");
  ctx->s_S6		= js_vm_intern (vm, "$6");
  ctx->s_S7		= js_vm_intern (vm, "$7");
  ctx->s_S8		= js_vm_intern (vm, "$8");
  ctx->s_S9		= js_vm_intern (vm, "$9");
  ctx->s_S_		= js_vm_intern (vm, "$_");
  ctx->s_input		= js_vm_intern (vm, "input");
  ctx->s_lastMatch	= js_vm_intern (vm, "lastMatch");
  ctx->s_lastParen	= js_vm_intern (vm, "lastParen");
  ctx->s_leftContext	= js_vm_intern (vm, "leftContext");
  ctx->s_multiline	= js_vm_intern (vm, "multiline");
  ctx->s_rightContext	= js_vm_intern (vm, "rightContext");

  ctx->s_global		= js_vm_intern (vm, "global");
  ctx->s_ignoreCase	= js_vm_intern (vm, "ignoreCase");
  ctx->s_lastIndex	= js_vm_intern (vm, "lastIndex");
  ctx->s_source		= js_vm_intern (vm, "source");

  ctx->s_compile	= js_vm_intern (vm, "compile");
  ctx->s_exec		= js_vm_intern (vm, "exec");
  ctx->s_test		= js_vm_intern (vm, "test");

  /* Object information. */

  info = js_vm_builtin_info_create (vm);

  info->global_method_proc	= global_method;
  info->method_proc		= method;
  info->property_proc		= property;
  info->new_proc		= new_proc;
  info->delete_proc		= delete_proc;
  info->mark_proc		= mark;
  info->obj_context		= ctx;
  info->obj_context_delete	= js_free;

  /* Define it. */
  n = &vm->globals[js_vm_intern (vm, "RegExp")];
  js_vm_builtin_create (vm, n, info, NULL);
}


void
js_builtin_RegExp_new (JSVirtualMachine *vm, char *source,
		       unsigned int source_len, unsigned int flags,
		       int immutable, JSBuiltinInfo *info,
		       JSNode *result_return)
{
  RegexpInstanceCtx *instance;
  const char *error;

  instance = js_calloc (vm, 1, sizeof (*instance));

  instance->source_len = source_len;

  /* +1 to avoid zero allocation. */
  instance->source = js_malloc (vm, instance->source_len + 1);
  memcpy (instance->source, source, instance->source_len);

  instance->global 	= (flags & JS_REGEXP_FLAG_G) != 0;
  instance->ignore_case = (flags & JS_REGEXP_FLAG_I) != 0;
  instance->immutable 	= immutable;

  if (instance->ignore_case)
    instance->compiled.translate = js_latin1_tolower;

  error = re_compile_pattern (instance->source, instance->source_len,
			      &instance->compiled);
  if (error)
    {
      js_free (instance->source);
      js_free (instance);
      sprintf (vm->error,
	       "new RegExp(): compilation of the expression failed: %s",
	       error);
      js_vm_error (vm);
    }
  instance->compiled.fastmap = js_malloc (vm, 256);
  re_compile_fastmap (&instance->compiled);

  if (info == NULL)
    {
      JSNode *n;

      n = &vm->globals[js_vm_intern (vm, "RegExp")];
      info = n->u.vbuiltin->info;
    }

  /* Create a new object. */
  js_vm_builtin_create (vm, result_return, info, instance);
}


#define EMIT_TO_RESULT(md, mdl)					\
  do {								\
    result_return->u.vstring->data				\
      = js_vm_realloc (vm, result_return->u.vstring->data,	\
		       result_return->u.vstring->len + (mdl));	\
    memcpy (result_return->u.vstring->data			\
	    + result_return->u.vstring->len,			\
	    (md), (mdl));					\
    result_return->u.vstring->len += (mdl);			\
  } while (0)

void
js_builtin_RegExp_replace (JSVirtualMachine *vm, char *data,
			   unsigned int datalen, JSNode *regexp,
			   char *repl, unsigned int repllen,
			   JSNode *result_return)
{
  int i, j;
  RegexpInstanceCtx *ictx = regexp->u.vbuiltin->instance_context;
  struct re_registers regs = {0};
  unsigned int substs = 0;
  unsigned int pos = 0;

  /*
   * Allocate the result string, Let's guess that we need at least
   * <datalen> bytes of data.
   */
  js_vm_make_string (vm, result_return, NULL, datalen);
  result_return->u.vstring->len = 0;

  /* Do searches. */
  while (pos < datalen)
    {
      i = re_search (&ictx->compiled, data, datalen, pos, datalen - pos,
		     &regs);

      /* Check what we got. */
      if (i >= 0)
	{
	  /* Emit all up to the first matched character. */
	  EMIT_TO_RESULT (data + pos, regs.start[0] - pos);

	  /* Check for empty matches. */
	  if (regs.end[0] == regs.start[0])
	    {
	      pos = regs.end[0];

	      /* Still something left to search? */
	      if (pos < datalen)
		{
		  /* Go one character forward. */
		  EMIT_TO_RESULT (data + pos, 1);
		  pos++;
		}
	    }
	  else
	    {
	      int start;

	      /* Not an empty match. */
	      substs++;

	      /* Interpret replace string. */
	      start = 0;
	      for (i = 0; i < repllen; i++)
		{
		  if (repl[i] == '$')
		    {
		      if (i + 1 >= repllen)
			/* The last character is '$'.  Just emit it. */
			continue;

		      /* First, emit all we have collected so far. */
		      EMIT_TO_RESULT (repl + start, i - start);
		      start = i++;

		      /* Check tag. */
		      switch (repl[i])
			{
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			  /* n:th subexpression. */
			  j = repl[i] - '0';

			  if (regs.start[j] >= 0)
			    EMIT_TO_RESULT (data + regs.start[j],
					    regs.end[j] - regs.start[j]);

			  start = i + 1;
			  break;

			case '`':
			  /* Left context. */
			  EMIT_TO_RESULT (data, regs.start[0]);
			  start = i + 1;
			  break;

			case '\'':
			  /* Right context. */
			  EMIT_TO_RESULT (data + regs.end[0],
					  datalen - regs.end[0]);
			  start = i + 1;
			  break;

			case '&':
			  /* Last match. */
			  EMIT_TO_RESULT (data + regs.start[0],
					  regs.end[0] - regs.start[0]);
			  start = i + 1;
			  break;

			case '$':
			  /* The dollar sign. */
			  EMIT_TO_RESULT ("$", 1);
			  start = i + 1;
			  break;

			case '+':
			default:
			  /* Ignore. */
			  start = i - 1;
			  break;
			}
		    }
		}

	      /* Emit all leftovers. */
	      EMIT_TO_RESULT (repl + start, i - start);

	      /* Update search position. */
	      pos = regs.end[0];
	    }
	}
      else
	break;

      if (!ictx->global && substs > 0)
	/* Substitute only the first match. */
	break;
    }

  /* No more matches.  Emit the rest of the string to the result. */
  EMIT_TO_RESULT (data + pos, datalen - pos);

  if (regs.start)
    js_free (regs.start);
  if (regs.end)
    js_free (regs.end);
}


void
js_builtin_RegExp_match (JSVirtualMachine *vm, char *data,
			 unsigned int datalen, JSNode *regexp,
			 JSNode *result_return)
{
  do_exec (vm, regexp->u.vbuiltin->info->obj_context,
	   regexp->u.vbuiltin->instance_context, data, datalen,
	   result_return);
}


void
js_builtin_RegExp_search (JSVirtualMachine *vm, char *data,
			  unsigned int datalen, JSNode *regexp,
			  JSNode *result_return)
{
  RegexpCtx *ctx = regexp->u.vbuiltin->info->obj_context;
  RegexpInstanceCtx *ictx = regexp->u.vbuiltin->instance_context;

  result_return->type = JS_INTEGER;
  result_return->u.vinteger = re_search (&ictx->compiled, data, datalen,
					 ictx->global ? ictx->last_index : 0,
					 datalen, &ctx->regs);

  if (result_return->u.vinteger >= 0)
    ictx->last_index = ctx->regs.end[0];
}


void
js_builtin_RegExp_split (JSVirtualMachine *vm, char *data,
			 unsigned int datalen, JSNode *regexp,
			 unsigned int limit, JSNode *result_return)
{
  unsigned int start = 0, pos;
  unsigned int alen = 0;
  RegexpInstanceCtx *ictx = regexp->u.vbuiltin->instance_context;
  struct re_registers regs = {0};
  int i;

  js_vm_make_array (vm, result_return, alen);

  for (pos = 0; alen < limit && pos <= datalen; )
    {
      i = re_search (&ictx->compiled, data, datalen, pos, datalen - pos,
		     &regs);
      if (i < 0)
	{
	  pos = datalen;
	  break;
	}

      /* Found the separator. */
      js_vm_expand_array (vm, result_return, alen + 1);
      js_vm_make_string (vm, &result_return->u.varray->data[alen],
			 data + start, regs.start[0] - start);
      alen++;

      if (regs.end[0] == pos)
	{
	  /* We didn't advance in the string. */
	  start = regs.end[0];
	  pos++;
	}
      else
	pos = start = regs.end[0];
    }

  if (alen < limit)
    {
      /* Insert all leftovers. */
      js_vm_expand_array (vm, result_return, alen + 1);
      js_vm_make_string (vm, &result_return->u.varray->data[alen],
			 data + start, datalen - start);
    }

  if (regs.start)
    js_free (regs.start);
  if (regs.end)
    js_free (regs.end);
}
