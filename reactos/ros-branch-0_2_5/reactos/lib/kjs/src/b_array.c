/*
 * The builtin Array object.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/b_array.c,v $
 * $Id: b_array.c,v 1.1 2004/01/10 20:38:18 arty Exp $
 */

/*
 * Mehtods:
 *
 *   concat (array) => array
 *   join ([glue]) => string
 *   pop () => last_element
 *   push (any...) => last_element_added
 *   reverse ()
 *   shift () => first_element
 *   slice (start, end) => array
 *   splice (index, how_many[, any...]) => array
 *   sort ([sort_function])
 *   toString () => string
 *   unshift (any...) => length_of_the_array
 *
 * Properties:
 *
 *   length
 */

#include "jsint.h"
#include "mrgsort.h"

/*
 * Types and definitions.
 */

/* Class context. */
struct array_ctx_st
{
  JSSymbol s_concat;
  JSSymbol s_join;
  JSSymbol s_pop;
  JSSymbol s_push;
  JSSymbol s_reverse;
  JSSymbol s_shift;
  JSSymbol s_slice;
  JSSymbol s_splice;
  JSSymbol s_sort;
  JSSymbol s_unshift;

  JSSymbol s_length;
};

typedef struct array_ctx_st ArrayCtx;

/* Context for array sorts with JavaScript functions. */
struct array_sort_ctx_st
{
  JSVirtualMachine *vm;
  JSNode *func;
  JSNode argv[3];
};

typedef struct array_sort_ctx_st ArraySortCtx;

/*
 * Prototypes for static functions.
 */

static void new_proc (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
		      JSNode *args, JSNode *result_return);


/*
 * Static functions.
 */

static int
sort_default_cmp_func (const void *aptr, const void *bptr, void *context)
{
  JSVirtualMachine *vm = context;
  const JSNode *a = aptr;
  const JSNode *b = bptr;
  JSNode astr, bstr;

  if (a->type == JS_UNDEFINED)
    return 1;
  if (b->type == JS_UNDEFINED)
    return -1;

  js_vm_to_string (vm, a, &astr);
  js_vm_to_string (vm, b, &bstr);

  return js_compare_strings (&astr, &bstr);
}


static int
sort_js_cmp_func (const void *aptr, const void *bptr, void *context)
{
  ArraySortCtx *ctx = context;
  const JSNode *a = aptr;
  const JSNode *b = bptr;

  /*
   * Finalize the argument array.  The argumnet count has already been set.
   * when the context were initialized.
   */
  JS_COPY (&ctx->argv[1], a);
  JS_COPY (&ctx->argv[2], b);

  /* Call the function. */
  if (!js_vm_apply (ctx->vm, NULL, ctx->func, 3, ctx->argv))
    /* Raise an error. */
    js_vm_error (ctx->vm);

  /* Fetch the return value. */
  if (ctx->vm->exec_result.type != JS_INTEGER)
    {
      sprintf (ctx->vm->error,
	       "Array.sort(): comparison function didn't return integer");
      js_vm_error (ctx->vm);
    }

  return ctx->vm->exec_result.u.vinteger;
}


/* Global method proc. */
static void
global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	       void *instance_context, JSNode *result_return,
	       JSNode *args)
{
  /* This does exactly the same as the new_proc. */
  new_proc (vm, builtin_info, args, result_return);
}


/* Method proc. */
static int
method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	void *instance_context, JSSymbol method, JSNode *result_return,
	JSNode *args)
{
  ArrayCtx *ctx = builtin_info->obj_context;
  JSNode *n = instance_context;
  int i;

  /* XXX 15.7.4.3 toSource(). */

  /* Handle static methods here. */
  if (instance_context == NULL)
    {
      if (method == vm->syms.s_toString)
	js_vm_make_static_string (vm, result_return, "Array", 5);
      /* ************************************************************ */
      else
	return JS_PROPERTY_UNKNOWN;

      return JS_PROPERTY_FOUND;
    }

  /* Handle the instance methods. */

  /* Set the default result type. */
  result_return->type = JS_UNDEFINED;

  if (method == ctx->s_concat)
    {
      int nlen;
      int pos;

      /* Count the new len; */

      nlen = n->u.varray->length;
      for (i = 0; i < args->u.vinteger; i++)
	{
	  if (args[i + 1].type != JS_ARRAY)
	    goto argument_error;

	  nlen += args[i + 1].u.varray->length;
	}

      js_vm_make_array (vm, result_return, nlen);

      /* Insert the items. */
      memcpy (result_return->u.varray->data, n->u.varray->data,
	      n->u.varray->length * sizeof (JSNode));
      pos = n->u.varray->length;

      for (i = 0; i < args->u.vinteger; i++)
	{
	  memcpy (&result_return->u.varray->data[pos],
		  args[i + 1].u.varray->data,
		  args[i + 1].u.varray->length * sizeof (JSNode));
	  pos += args[i + 1].u.varray->length;
	}
    }
  /* ********************************************************************** */
  else if (method == ctx->s_join || method == vm->syms.s_toString)
    {
      char *glue = NULL;

      if (method == vm->syms.s_toString)
	{
	  if (args->u.vinteger != 0)
	    goto argument_error;
	}
      else
	{
	  if (args->u.vinteger == 0)
	    ;
	  else if (args->u.vinteger == 1)
	    {
	      JSNode glue_result;

	      js_vm_to_string (vm, &args[1], &glue_result);
	      glue = js_string_to_c_string (vm, &glue_result);
	    }
	  else
	    goto argument_error;
	}

      /* Ok, ready to run. */
      if (n->u.varray->length == 0)
	js_vm_make_static_string (vm, result_return, "", 0);
      else
	{
	  int len;
	  int glue_len = glue ? strlen (glue) : 1;

	  /* Estimate the result length. */
	  len = (n->u.varray->length * 5
		 + (n->u.varray->length - 1) * glue_len);

	  js_vm_make_string (vm, result_return, NULL, len);
	  result_return->u.vstring->len = 0;

	  /* Do the join. */
	  for (i = 0; i < n->u.varray->length; i++)
	    {
	      JSNode sitem;
	      int delta;

	      js_vm_to_string (vm, &n->u.varray->data[i], &sitem);
	      delta = sitem.u.vstring->len;

	      if (i + 1 < n->u.varray->length)
		delta += glue_len;

	      result_return->u.vstring->data
		= js_vm_realloc (vm, result_return->u.vstring->data,
				 result_return->u.vstring->len + delta);

	      memcpy (result_return->u.vstring->data
		      + result_return->u.vstring->len,
		      sitem.u.vstring->data,
		      sitem.u.vstring->len);
	      result_return->u.vstring->len += sitem.u.vstring->len;

	      if (i + 1 < n->u.varray->length)
		{
		  if (glue)
		    {
		      memcpy (result_return->u.vstring->data
			      + result_return->u.vstring->len,
			      glue, glue_len);
		      result_return->u.vstring->len += glue_len;
		    }
		  else
		    result_return->u.vstring->data
		      [result_return->u.vstring->len++] = ',';
		}
	    }
	}

      if (glue)
	js_free (glue);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_pop)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      if (n->u.varray->length == 0)
	result_return->type = JS_UNDEFINED;
      else
	{
	  JS_COPY (result_return, &n->u.varray->data[n->u.varray->length - 1]);
	  n->u.varray->length--;
	}
    }
  /* ********************************************************************** */
  else if (method == ctx->s_push)
    {
      int old_len;

      if (args->u.vinteger == 0)
	goto argument_error;

      old_len = n->u.varray->length;
      js_vm_expand_array (vm, n, n->u.varray->length + args->u.vinteger);

      for (i = 0; i < args->u.vinteger; i++)
	JS_COPY (&n->u.varray->data[old_len + i], &args[i + 1]);

      JS_COPY (result_return, &args[i]);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_reverse)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      for (i = 0; i < n->u.varray->length / 2; i++)
	{
	  JSNode tmp;

	  JS_COPY (&tmp, &n->u.varray->data[i]);
	  JS_COPY (&n->u.varray->data[i],
		   &n->u.varray->data[n->u.varray->length - i - 1]);
	  JS_COPY (&n->u.varray->data[n->u.varray->length - i - 1], &tmp);
	}
    }
  /* ********************************************************************** */
  else if (method == ctx->s_shift)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      if (n->u.varray->length == 0)
	result_return->type = JS_UNDEFINED;
      else
	{
	  JS_COPY (result_return, &n->u.varray->data[0]);
	  memmove (&n->u.varray->data[0], &n->u.varray->data[1],
		   (n->u.varray->length - 1) * sizeof (JSNode));
	  n->u.varray->length--;
	}
    }
  /* ********************************************************************** */
  else if (method == ctx->s_slice)
    {
      int start, end;

      if (args->u.vinteger < 1 || args->u.vinteger > 2)
	goto argument_error;

      if (args[1].type != JS_INTEGER)
	goto argument_type_error;

      start = args[1].u.vinteger;

      if (args->u.vinteger == 2)
	{
	  if (args[2].type != JS_INTEGER)
	    goto argument_type_error;

	  end = args[2].u.vinteger;
	}
      else
	end = n->u.varray->length;

      if (end < 0)
	end += n->u.varray->length;
      if (end < 0)
	end = start;

      js_vm_make_array (vm, result_return, end - start);

      /* Copy items. */
      for (i = 0; i < end - start; i++)
	JS_COPY (&result_return->u.varray->data[i],
		 &n->u.varray->data[start + i]);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_splice)
    {
      unsigned int new_length;
      unsigned int old_length;
      int delta;

      if (args->u.vinteger < 2)
	goto argument_error;
      if (args[1].type != JS_INTEGER || args[2].type != JS_INTEGER)
	goto argument_type_error;

      if (args[2].u.vinteger == 0 && args->u.vinteger == 2)
	/* No deletions: must specify at least one item to insert. */
	goto argument_error;

      old_length = new_length = n->u.varray->length;
      if (args[1].u.vinteger < new_length)
	{
	  if (args[2].u.vinteger > new_length - args[1].u.vinteger)
	    {
	      args[2].u.vinteger = new_length - args[1].u.vinteger;
	      new_length = args[1].u.vinteger;
	    }
	  else
	    new_length -= args[2].u.vinteger;
	}
      else
	{
	  new_length = args[1].u.vinteger;
	  args[2].u.vinteger = 0;
	}

      new_length += args->u.vinteger - 2;

      if (new_length > n->u.varray->length)
	js_vm_expand_array (vm, n, new_length);
      else
	/* Cut the array. */
	n->u.varray->length = new_length;

      /* Do the stuffs we must do. */

      /* Create the result array. */
      if (args[2].u.vinteger == 0)
	result_return->type = JS_UNDEFINED;
      else
	{
	  js_vm_make_array (vm, result_return, args[2].u.vinteger);
	  for (i = 0; i < args[2].u.vinteger; i++)
	    JS_COPY (&result_return->u.varray->data[i],
		     &n->u.varray->data[args[1].u.vinteger + i]);
	}

      /* Delete and move. */
      delta = args->u.vinteger - 2 - args[2].u.vinteger;
      memmove (&n->u.varray->data[args[1].u.vinteger + args[2].u.vinteger
				 + delta],
	       &n->u.varray->data[args[1].u.vinteger + args[2].u.vinteger],
	       (old_length - (args[1].u.vinteger + args[2].u.vinteger))
	       * sizeof (JSNode));

      /* Insert. */
      for (i = 0; i < args->u.vinteger - 2; i++)
	JS_COPY (&n->u.varray->data[args[1].u.vinteger + i], &args[i + 3]);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_sort)
    {
      MergesortCompFunc func;
      ArraySortCtx array_sort_ctx;
      void *func_ctx = NULL;	/* Initialized to keep compiler quiet. */

      if (args->u.vinteger == 0)
	{
	  func = sort_default_cmp_func;
	  func_ctx = vm;
	}
      else if (args->u.vinteger == 1)
	{
	  if (args[1].type != JS_FUNC && args[1].type != JS_BUILTIN)
	    goto argument_type_error;

	  func = sort_js_cmp_func;

	  /* Init context. */
	  array_sort_ctx.vm = vm;
	  array_sort_ctx.func = &args[1];

	  /* Init the argc part of the argument vector here. */
	  array_sort_ctx.argv[0].type = JS_INTEGER;
	  array_sort_ctx.argv[0].u.vinteger = 3;

	  func_ctx = &array_sort_ctx;
	}
      else
	goto argument_error;

      mergesort_r (n->u.varray->data, n->u.varray->length, sizeof (JSNode),
		   func, func_ctx);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_unshift)
    {
      int old_len;

      if (args->u.vinteger == 0)
	goto argument_error;

      old_len = n->u.varray->length;
      js_vm_expand_array (vm, n, n->u.varray->length + args->u.vinteger);

      memmove (&n->u.varray->data[args->u.vinteger], n->u.varray->data,
	       old_len * sizeof (JSNode));

      for (i = 0; i < args->u.vinteger; i++)
	JS_COPY (&n->u.varray->data[i], &args[args->u.vinteger - i]);

      result_return->type = JS_INTEGER;
      result_return->u.vinteger = n->u.varray->length;
    }
  /* ********************************************************************** */
  else
    return JS_PROPERTY_UNKNOWN;

  return JS_PROPERTY_FOUND;


  /*
   * Error handling.
   */

 argument_error:
  sprintf (vm->error, "Array.%s(): illegal amount of arguments",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

 argument_type_error:
  sprintf (vm->error, "Array.%s(): illegal argument",
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
  ArrayCtx *ctx = builtin_info->obj_context;
  JSNode *n = instance_context;

  if (property == ctx->s_length)
    {
      if (set)
	goto immutable;

      node->type = JS_INTEGER;
      node->u.vinteger = n->u.varray->length;
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
  sprintf (vm->error, "Array.%s: immutable property",
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
  int i;

  if (args->u.vinteger == 1 && args[1].type == JS_INTEGER)
    {
      /* Create a fixed length array. */
      js_vm_make_array (vm, result_return, args[1].u.vinteger);
    }
  else
    {
      if (args->u.vinteger < 0)
	/* We are called from the array initializer. */
	args->u.vinteger = -args->u.vinteger;

      js_vm_make_array (vm, result_return, args->u.vinteger);
      for (i = 0; i < args->u.vinteger; i++)
	JS_COPY (&result_return->u.varray->data[i], &args[i + 1]);
    }
  /* Set the [[Prototype]] and [[Class]] properties. */
  /* XXX 15.7.2.1 */
}


/*
 * Global functions.
 */

void
js_builtin_Array (JSVirtualMachine *vm)
{
  ArrayCtx *ctx;
  JSNode *n;
  JSBuiltinInfo *info;

  ctx = js_calloc (vm, 1, sizeof (*ctx));

  ctx->s_concat		= js_vm_intern (vm, "concat");
  ctx->s_join		= js_vm_intern (vm, "join");
  ctx->s_pop		= js_vm_intern (vm, "pop");
  ctx->s_push		= js_vm_intern (vm, "push");
  ctx->s_reverse	= js_vm_intern (vm, "reverse");
  ctx->s_shift		= js_vm_intern (vm, "shift");
  ctx->s_slice		= js_vm_intern (vm, "slice");
  ctx->s_splice		= js_vm_intern (vm, "splice");
  ctx->s_sort		= js_vm_intern (vm, "sort");
  ctx->s_unshift	= js_vm_intern (vm, "unshift");

  ctx->s_length		= js_vm_intern (vm, "length");

  info = js_vm_builtin_info_create (vm);
  vm->prim[JS_ARRAY] = info;

  info->global_method_proc	= global_method;
  info->method_proc		= method;
  info->property_proc		= property;
  info->new_proc		= new_proc;
  info->obj_context		= ctx;
  info->obj_context_delete	= js_free;

  /* Define it. */
  n = &vm->globals[js_vm_intern (vm, "Array")];
  js_vm_builtin_create (vm, n, info, NULL);
}
