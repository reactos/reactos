/*
 * The builtin String object.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/b_string.c,v $
 * $Id: b_string.c,v 1.1 2004/01/10 20:38:18 arty Exp $
 */

/* TODO: global method: String (obj) => string */

#include "jsint.h"

/*
 * Types and definitions.
 */

#define UNPACK_NEED(n)							\
  do {									\
    if (bufpos + (n) > buflen)						\
      {									\
	sprintf (vm->error,						\
		 "String.%s(): too short string for the format",	\
		 js_vm_symname (vm, method));				\
	js_vm_error (vm);						\
      }									\
  } while (0)

#define UNPACK_EXPAND()						\
  do {								\
    js_vm_expand_array (vm, result_return, result_len + 1);	\
    rnode = &result_return->u.varray->data[result_len];		\
    result_len++;						\
  } while (0)

/* Class context. */
struct string_ctx_st
{
  JSSymbol s_length;

  JSSymbol s_append;
  JSSymbol s_charAt;
  JSSymbol s_charCodeAt;
  JSSymbol s_concat;
  JSSymbol s_crc32;
  JSSymbol s_fromCharCode;
  JSSymbol s_indexOf;
  JSSymbol s_lastIndexOf;
  JSSymbol s_match;
  JSSymbol s_pack;
  JSSymbol s_replace;
  JSSymbol s_search;
  JSSymbol s_slice;
  JSSymbol s_split;
  JSSymbol s_substr;
  JSSymbol s_substring;
  JSSymbol s_toLowerCase;
  JSSymbol s_toUpperCase;
  JSSymbol s_unpack;

  /* Data we need to implement the RegExp related stuffs. */
  JSBuiltinInfo *regexp_info;
};

typedef struct string_ctx_st StringCtx;


/*
 * Static functions.
 */

/* Global method proc. */
static void
global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	       void *instance_context, JSNode *result_return,
	       JSNode *args)
{
  if (args->u.vinteger == 0)
    js_vm_make_static_string (vm, result_return, "", 0);
  else if (args->u.vinteger == 1)
    js_vm_to_string (vm, &args[1], result_return);
  else
    {
      sprintf (vm->error, "String(): illegal amount of arguments");
      js_vm_error (vm);
    }
}

/* Method proc. */
static int
method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	void *instance_context, JSSymbol method, JSNode *result_return,
	JSNode *args)
{
  StringCtx *ctx = builtin_info->obj_context;
  JSNode *n = instance_context;
  unsigned int ui;
  int i;

  /*
   * Static methods.
   */

  if (method == ctx->s_fromCharCode)
    {
      js_vm_make_string (vm, result_return, NULL, args->u.vinteger);

      for (i = 0; i < args->u.vinteger; i++)
	{
	  if (args[1 + i].type != JS_INTEGER)
	    goto argument_type_error;

	  result_return->u.vstring->data[i]
	    = (unsigned char) args[1 + i].u.vinteger;
	}
    }
  /* ********************************************************************** */
  else if (method == ctx->s_pack)
    {
      unsigned int op;
      unsigned int arg = 2;
      JSUInt32 ui;
      double dval;
      unsigned char *buffer = NULL;
      unsigned int bufpos = 0;

      if (args->u.vinteger < 1)
	goto argument_error;
      if (args[1].type != JS_STRING)
	goto argument_type_error;

      for (op = 0; op < args[1].u.vstring->len; op++)
	{
	  if (arg >= args->u.vinteger + 1)
	    {
	      sprintf (vm->error, "String.%s(): too few arguments for format",
		       js_vm_symname (vm, method));
	      js_vm_error (vm);
	    }

	  switch (args[1].u.vstring->data[op])
	    {
	    case 'C':
	      if (args[arg].type != JS_INTEGER)
		goto argument_type_error;

	      buffer = js_vm_realloc (vm, buffer, bufpos + 1);
	      buffer[bufpos++] = (unsigned char) args[arg++].u.vinteger;
	      break;

	    case 'n':
	      if (args[arg].type != JS_INTEGER)
		goto argument_type_error;

	      ui = args[arg++].u.vinteger;

	      buffer = js_vm_realloc (vm, buffer, bufpos + 2);
	      buffer[bufpos++] = (unsigned char) ((ui & 0x0000ff00) >> 8);
	      buffer[bufpos++] = (unsigned char) (ui & 0x000000ff);
	      break;

	    case 'N':
	      if (args[arg].type != JS_INTEGER)
		goto argument_type_error;

	      ui = args[arg++].u.vinteger;

	      buffer = js_vm_realloc (vm, buffer, bufpos + 4);
	      buffer[bufpos++] = (unsigned char) ((ui & 0xff000000) >> 24);
	      buffer[bufpos++] = (unsigned char) ((ui & 0x00ff0000) >> 16);
	      buffer[bufpos++] = (unsigned char) ((ui & 0x0000ff00) >> 8);
	      buffer[bufpos++] = (unsigned char) ((ui & 0x000000ff));
	      break;

	    case 'd':
	      if (args[arg].type != JS_INTEGER && args[arg].type != JS_FLOAT)
		goto argument_type_error;

	      if (args[arg].type == JS_INTEGER)
		dval = (double) args[arg].u.vinteger;
	      else
		dval = args[arg].u.vfloat;
	      arg++;

	      buffer = js_vm_realloc (vm, buffer, bufpos + sizeof (double));
	      memcpy (buffer + bufpos, &dval, sizeof (double));
	      bufpos += sizeof (double);
	      break;

	    default:
	      /* Silently ignore it. */
	      break;
	    }
	}

      js_vm_make_static_string (vm, result_return, buffer, bufpos);
      result_return->u.vstring->staticp = 0;
    }
  /* ********************************************************************** */
  else if (method == vm->syms.s_toString)
    {
      if (n)
	JS_COPY (result_return, n);
      else
	js_vm_make_static_string (vm, result_return, "String", 6);
    }
  /* ********************************************************************** */
  else if (method == vm->syms.s_valueOf)
    {
      if (n)
	JS_COPY (result_return, n);
      else
	{
	  n = &vm->globals[js_vm_intern (vm, "String")];
	  JS_COPY (result_return, n);
	}
    }
  /* ********************************************************************** */
  else if (n)
    {
      /*
       * Instance methods.
       */

      if (method == ctx->s_append)
	{
	  if (args->u.vinteger != 1)
	    goto argument_error;

	  if (n->u.vstring->staticp)
	    {
	      sprintf (vm->error,
		       "String.%s(): can't append to a static string",
		       js_vm_symname (vm, method));
	      js_vm_error (vm);
	    }

	  if (args[1].type == JS_STRING)
	    {
	      /* Append a string. */
	      n->u.vstring->data = js_vm_realloc (vm, n->u.vstring->data,
						  n->u.vstring->len
						  + args[1].u.vstring->len);
	      memcpy (n->u.vstring->data + n->u.vstring->len,
		      args[1].u.vstring->data,
		      args[1].u.vstring->len);
	      n->u.vstring->len += args[1].u.vstring->len;
	    }
	  else if (args[1].type == JS_INTEGER)
	    {
	      /* Append a character. */
	      n->u.vstring->data = js_vm_realloc (vm, n->u.vstring->data,
						  n->u.vstring->len + 1);
	      n->u.vstring->data[n->u.vstring->len++]
		= (unsigned char) args[1].u.vinteger;
	    }
	  else
	    goto argument_type_error;
	}
      /* ***************************************************************** */
      else if (method == ctx->s_charAt)
	{
	  if (args->u.vinteger != 1)
	    goto argument_error;
	  if (args[1].type != JS_INTEGER)
	    goto argument_type_error;

	  js_vm_make_string (vm, result_return, NULL, 1);

	  ui = args[1].u.vinteger;

	  if (ui >= n->u.vstring->len)
	    result_return->u.vstring->len = 0;
	  else
	    result_return->u.vstring->data[0] = n->u.vstring->data[ui];
	}
      /* ***************************************************************** */
      else if (method == ctx->s_charCodeAt)
	{
	  if (args->u.vinteger == 0)
	    ui = 0;
	  else if (args->u.vinteger == 1)
	    {
	      if (args[1].type != JS_INTEGER)
		goto argument_type_error;
	      ui = args[1].u.vinteger;
	    }
	  else
	    goto argument_error;

	  if (ui >= n->u.vstring->len)
	    {
	      sprintf (vm->error, "String.%s(): index out of range",
		       js_vm_symname (vm, method));
	      js_vm_error (vm);
	    }

	  result_return->type = JS_INTEGER;
	  result_return->u.vinteger = n->u.vstring->data[ui];
	}
      /* ***************************************************************** */
      else if (method == ctx->s_concat)
	{
	  int nlen, pos;

	  /* Count the new length. */

	  nlen = n->u.vstring->len;

	  for (i = 0; i < args->u.vinteger; i++)
	    {
	      if (args[i + 1].type != JS_STRING)
		goto argument_type_error;
	      nlen += args[i + 1].u.vstring->len;
	    }

	  js_vm_make_string (vm, result_return, NULL, nlen);

	  memcpy (result_return->u.vstring->data, n->u.vstring->data,
		  n->u.vstring->len);

	  /* Append the argumens. */

	  pos = n->u.vstring->len;

	  for (i = 0; i < args->u.vinteger; i++)
	    {
	      memcpy (result_return->u.vstring->data + pos,
		      args[i + 1].u.vstring->data, args[i + 1].u.vstring->len);
	      pos += args[i + 1].u.vstring->len;
	    }
	}
      /* ***************************************************************** */
      else if (method == ctx->s_crc32)
	{
	  if (args->u.vinteger != 0)
	    goto argument_error;

	  result_return->type = JS_INTEGER;
	  result_return->u.vinteger = js_crc32 (n->u.vstring->data,
						n->u.vstring->len);
	}
      /* ***************************************************************** */
      else if (method == ctx->s_indexOf)
	{
	  int start_index = 0;

	  if (args->u.vinteger < 1 || args->u.vinteger > 2)
	    goto argument_error;
	  if (args[1].type != JS_STRING)
	    goto argument_type_error;

	  if (args->u.vinteger == 2)
	    {
	      if (args[2].type != JS_INTEGER)
		goto argument_type_error;

	      start_index = args[2].u.vinteger;
	    }

	  result_return->type = JS_INTEGER;
	  result_return->u.vinteger = -1;

	  if (start_index >= 0
	      && start_index + args[1].u.vstring->len <= n->u.vstring->len)
	    {
	      /* Use the Brute Force Luke! */
	      for (; start_index + args[1].u.vstring->len <= n->u.vstring->len;
		   start_index++)
		if (memcmp (n->u.vstring->data + start_index,
			    args[1].u.vstring->data,
			    args[1].u.vstring->len) == 0)
		  {
		    result_return->u.vinteger = start_index;
		    break;
		  }
	    }
	}
      /* ***************************************************************** */
      else if (method == ctx->s_lastIndexOf)
	{
	  int start_index;

	  if (args->u.vinteger < 1 || args->u.vinteger > 2)
	    goto argument_error;
	  if (args[1].type != JS_STRING)
	    goto argument_type_error;

	  if (args->u.vinteger == 2)
	    {
	      if (args[2].type != JS_INTEGER)
		goto argument_type_error;

	      start_index = args[2].u.vinteger;
	    }
	  else
	    start_index = n->u.vstring->len - args[1].u.vstring->len;

	  result_return->type = JS_INTEGER;
	  result_return->u.vinteger = -1;

	  if (start_index >= 0
	      && start_index + args[1].u.vstring->len <= n->u.vstring->len)
	    {
	      for (; start_index >= 0; start_index--)
		if (memcmp (n->u.vstring->data + start_index,
			    args[1].u.vstring->data,
			    args[1].u.vstring->len) == 0)
		  {
		    result_return->u.vinteger = start_index;
		    break;
		  }
	    }
	}
      /* ***************************************************************** */
      else if (method == ctx->s_match)
	{
	  if (args->u.vinteger != 1)
	    goto argument_error;

	  if (args[1].type != JS_BUILTIN
	      || args[1].u.vbuiltin->info != ctx->regexp_info)
	    goto argument_type_error;

	  js_builtin_RegExp_match (vm, n->u.vstring->data, n->u.vstring->len,
				   &args[1], result_return);
	}
      /* ***************************************************************** */
      else if (method == ctx->s_replace)
	{
	  if (args->u.vinteger != 2)
	    goto argument_error;

	  if (args[1].type != JS_BUILTIN
	      || args[1].u.vbuiltin->info != ctx->regexp_info
	      || args[2].type != JS_STRING)
	    goto argument_type_error;

	  js_builtin_RegExp_replace (vm, n->u.vstring->data, n->u.vstring->len,
				     &args[1], args[2].u.vstring->data,
				     args[2].u.vstring->len, result_return);
	}
      /* ***************************************************************** */
      else if (method == ctx->s_search)
	{
	  if (args->u.vinteger != 1)
	    goto argument_error;

	  if (args[1].type != JS_BUILTIN
	      || args[1].u.vbuiltin->info != ctx->regexp_info)
	    goto argument_type_error;

	  js_builtin_RegExp_search (vm, n->u.vstring->data, n->u.vstring->len,
				    &args[1], result_return);
	}
      /* ***************************************************************** */
      else if (method == ctx->s_slice)
	{
	  int start, end;

	  if (args->u.vinteger != 1 && args->u.vinteger != 2)
	    goto argument_error;

	  if (args[1].type != JS_INTEGER)
	    goto argument_type_error;

	  start = args[1].u.vinteger;
	  if (start < 0)
	    start += n->u.vstring->len;
	  if (start < 0)
	    start = 0;
	  if (start > n->u.vstring->len)
	    start = n->u.vstring->len;

	  if (args->u.vinteger == 2)
	    {
	      if (args[2].type != JS_INTEGER)
		goto argument_type_error;

	      end = args[2].u.vinteger;
	      if (end < 0)
		end += n->u.vstring->len;
	      if (end < 0)
		end = 0;
	      if (end > n->u.vstring->len)
		end = n->u.vstring->len;
	    }
	  else
	    end = n->u.vstring->len;

	  if (start > end)
	    end = start;

	  js_vm_make_string (vm, result_return, n->u.vstring->data + start,
			     end - start);
	}
      /* ***************************************************************** */
      else if (method == ctx->s_split)
	{
	  if (args->u.vinteger == 0)
	    {
	      js_vm_make_array (vm, result_return, 1);
	      js_vm_make_string (vm, &result_return->u.varray->data[0],
				 n->u.vstring->data, n->u.vstring->len);
	    }
	  else
	    {
	      unsigned int limit;

	      if (args->u.vinteger == 1)
		limit = -1;
	      else if (args->u.vinteger == 2)
		{
		  if (args[2].type != JS_INTEGER)
		    goto argument_type_error;

		  limit = args[2].u.vinteger;
		}
	      else
		goto argument_error;

	      if (args[1].type == JS_STRING)
		{
		  unsigned int start = 0, pos;
		  unsigned int alen = 0;

		  js_vm_make_array (vm, result_return, alen);

		  for (pos = 0;
		       (alen < limit
			&& pos + args[1].u.vstring->len <= n->u.vstring->len);
		       )
		    {
		      if (memcmp (n->u.vstring->data + pos,
				  args[1].u.vstring->data,
				  args[1].u.vstring->len) == 0)
			{
			  /* Found the separator. */
			  js_vm_expand_array (vm, result_return, alen + 1);
			  js_vm_make_string (vm,
					     &(result_return
					       ->u.varray->data[alen]),
					     n->u.vstring->data + start,
					     pos - start);
			  alen++;

			  if (args[1].u.vstring->len == 0)
			    {
			      start = pos;
			      pos++;
			    }
			  else
			    {
			      pos += args[1].u.vstring->len;
			      start = pos;
			    }
			}
		      else
			pos++;
		    }

		  if (alen < limit)
		    {
		      /* And finally, insert all leftovers. */
		      js_vm_expand_array (vm, result_return, alen + 1);
		      js_vm_make_string (vm,
					 &result_return->u.varray->data[alen],
					 n->u.vstring->data + start,
					 n->u.vstring->len - start);
		    }
		}
	      else if (args[1].type == JS_BUILTIN
		       && args[1].u.vbuiltin->info == ctx->regexp_info)
		{
		  js_builtin_RegExp_split (vm, n->u.vstring->data,
					   n->u.vstring->len, &args[1],
					   limit, result_return);
		}
	      else
		goto argument_type_error;
	    }
	}
      /* ***************************************************************** */
      else if (method == ctx->s_substr)
	{
	  int start, length;

	  if (args->u.vinteger != 1 && args->u.vinteger != 2)
	    goto argument_error;

	  if (args[1].type != JS_INTEGER)
	    goto argument_type_error;

	  start = args[1].u.vinteger;

	  if (args->u.vinteger == 2)
	    {
	      if (args[2].type != JS_INTEGER)
		goto argument_type_error;

	      length = args[2].u.vinteger;
	      if (length < 0)
		length = 0;
	    }
	  else
	    length = n->u.vstring->len;

	  if (start < 0)
	    start += n->u.vstring->len;
	  if (start < 0)
	    start = 0;
	  if (start > n->u.vstring->len)
	    start = n->u.vstring->len;

	  if (start + length > n->u.vstring->len)
	    length = n->u.vstring->len - start;

	  js_vm_make_string (vm, result_return, n->u.vstring->data + start,
			     length);
	}
      /* ***************************************************************** */
      else if (method == ctx->s_substring)
	{
	  int start, end;

	  if (args->u.vinteger != 1 && args->u.vinteger != 2)
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
	    end = n->u.vstring->len;

	  if (start < 0)
	    start = 0;

	  if (end > n->u.vstring->len)
	    end = n->u.vstring->len;

	  if (start > end)
	    {
	      sprintf (vm->error,
		       "String.%s(): start index is bigger than end",
		       js_vm_symname (vm, method));
	      js_vm_error (vm);
	    }

	  js_vm_make_string (vm, result_return, n->u.vstring->data + start,
			     end - start);
	}
      /* ***************************************************************** */
      else if (method == ctx->s_toLowerCase)
	{
	  if (args->u.vinteger != 0)
	    goto argument_type_error;

	  js_vm_make_string (vm, result_return, n->u.vstring->data,
			     n->u.vstring->len);

	  for (i = 0; i < result_return->u.vstring->len; i++)
	    result_return->u.vstring->data[i]
	      = js_latin1_tolower[result_return->u.vstring->data[i]];
	}
      /* ***************************************************************** */
      else if (method == ctx->s_toUpperCase)
	{
	  if (args->u.vinteger != 0)
	    goto argument_type_error;

	  js_vm_make_string (vm, result_return, n->u.vstring->data,
			     n->u.vstring->len);

	  for (i = 0; i < result_return->u.vstring->len; i++)
	    result_return->u.vstring->data[i]
	      = js_latin1_toupper[result_return->u.vstring->data[i]];
	}
      /* ***************************************************************** */
      else if (method == ctx->s_unpack)
	{
	  unsigned int op;
	  unsigned char *buffer;
	  unsigned int buflen;
	  unsigned int bufpos = 0;
	  JSUInt32 ui;
	  unsigned int result_len = 0;
	  JSNode *rnode;

	  if (args->u.vinteger != 1)
	    goto argument_error;
	  if (args[1].type != JS_STRING)
	    goto argument_type_error;

	  buffer = n->u.vstring->data;
	  buflen = n->u.vstring->len;

	  js_vm_make_array (vm, result_return, 0);

	  for (op = 0; op < args[1].u.vstring->len; op++)
	    {
	      switch (args[1].u.vstring->data[op])
		{
		case 'C':
		  UNPACK_NEED (1);
		  UNPACK_EXPAND ();
		  rnode->type = JS_INTEGER;
		  rnode->u.vinteger = buffer[bufpos++];
		  break;

		case 'n':
		  UNPACK_NEED (2);
		  UNPACK_EXPAND ();

		  ui = buffer[bufpos++];
		  ui <<= 8;
		  ui |= buffer[bufpos++];

		  rnode->type = JS_INTEGER;
		  rnode->u.vinteger = ui;
		  break;

		case 'N':
		  UNPACK_NEED (4);
		  UNPACK_EXPAND ();

		  ui = buffer[bufpos++];
		  ui <<= 8;
		  ui |= buffer[bufpos++];
		  ui <<= 8;
		  ui |= buffer[bufpos++];
		  ui <<= 8;
		  ui |= buffer[bufpos++];

		  rnode->type = JS_INTEGER;
		  rnode->u.vinteger = ui;
		  break;

		case 'd':
		  UNPACK_NEED (8);
		  UNPACK_EXPAND ();

		  rnode->type = JS_FLOAT;
		  memcpy (&rnode->u.vfloat, buffer + bufpos, 8);
		  bufpos += 8;
		  break;

		default:
		  /* Silently ignore it. */
		  break;
		}
	    }
	}
      /* ***************************************************************** */
      else
	return JS_PROPERTY_UNKNOWN;
    }
  else
    return JS_PROPERTY_UNKNOWN;

  return JS_PROPERTY_FOUND;


  /*
   * Error handling.
   */

 argument_error:
  sprintf (vm->error, "String.%s(): illegal amount of arguments",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

 argument_type_error:
  sprintf (vm->error, "String %s(): illegal argument",
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
  StringCtx *ctx = builtin_info->obj_context;
  JSNode *n = instance_context;

  if (n && property == ctx->s_length)
    {
      if (set)
	goto immutable;

      node->type = JS_INTEGER;
      node->u.vinteger = n->u.vstring->len;
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
  sprintf (vm->error, "String.%s: immutable property",
	   js_vm_symname (vm, property));
  js_vm_error (vm);

  /* NOTREACHED. */
  return 0;
}


/* New proc. */
static void
new_proc (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info, JSNode *args,
	  JSNode *result_return)
{
  JSNode source_n;
  JSNode *source;

  if (args->u.vinteger == 0)
    js_vm_make_string (vm, result_return, NULL, 0);
  else if (args->u.vinteger == 1)
    {
      if (args[1].type == JS_STRING)
	source = &args[1];
      else
	{
	  js_vm_to_string (vm, &args[1], &source_n);
	  source = &source_n;
	}

      js_vm_make_string (vm, result_return, source->u.vstring->data,
			 source->u.vstring->len);
    }
  else
    {
      sprintf (vm->error, "new String(): illegal amount of arguments");
      js_vm_error (vm);
    }

  /* Set the [[Prototype]] and [[Class]] properties. */
  /* XXX 15.8.2 */
}

/*
 * Global functions.
 */

void
js_builtin_String (JSVirtualMachine *vm)
{
  StringCtx *ctx;
  JSNode *n;
  JSBuiltinInfo *info;

  ctx = js_calloc (vm, 1, sizeof (*ctx));

  ctx->s_length		= js_vm_intern (vm, "length");

  ctx->s_append		= js_vm_intern (vm, "append");
  ctx->s_charAt		= js_vm_intern (vm, "charAt");
  ctx->s_charCodeAt	= js_vm_intern (vm, "charCodeAt");
  ctx->s_concat		= js_vm_intern (vm, "concat");
  ctx->s_crc32		= js_vm_intern (vm, "crc32");
  ctx->s_fromCharCode	= js_vm_intern (vm, "fromCharCode");
  ctx->s_indexOf	= js_vm_intern (vm, "indexOf");
  ctx->s_lastIndexOf	= js_vm_intern (vm, "lastIndexOf");
  ctx->s_match		= js_vm_intern (vm, "match");
  ctx->s_pack		= js_vm_intern (vm, "pack");
  ctx->s_replace	= js_vm_intern (vm, "replace");
  ctx->s_search		= js_vm_intern (vm, "search");
  ctx->s_slice		= js_vm_intern (vm, "slice");
  ctx->s_split		= js_vm_intern (vm, "split");
  ctx->s_substr		= js_vm_intern (vm, "substr");
  ctx->s_substring	= js_vm_intern (vm, "substring");
  ctx->s_toLowerCase	= js_vm_intern (vm, "toLowerCase");
  ctx->s_toUpperCase	= js_vm_intern (vm, "toUpperCase");
  ctx->s_unpack		= js_vm_intern (vm, "unpack");

  info = js_vm_builtin_info_create (vm);
  vm->prim[JS_STRING] = info;

  info->global_method_proc	= global_method;
  info->method_proc 		= method;
  info->property_proc		= property;
  info->new_proc		= new_proc;
  info->obj_context		= ctx;
  info->obj_context_delete	= js_free;

  /* Define it. */
  n = &vm->globals[js_vm_intern (vm, "String")];
  js_vm_builtin_create (vm, n, info, NULL);

  /* Fetch the JSBuiltinInfo of the RegExp object. */
  n = &vm->globals[js_vm_intern (vm, "RegExp")];
  ctx->regexp_info = n->u.vbuiltin->info;
}
