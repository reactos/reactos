/*
 * Curses extension.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/xcurses.c,v $
 * $Id: xcurses.c,v 1.1 2004/01/10 20:38:18 arty Exp $
 */

#include "jsint.h"

#include <curses.h>

/* Curses context. */
struct curses_ctx_st
{
  JSSymbol s_addstr;
  JSSymbol s_attron;
  JSSymbol s_attroff;
  JSSymbol s_attset;
  JSSymbol s_beep;
  JSSymbol s_cbreak;
  JSSymbol s_clear;
  JSSymbol s_clrtobot;
  JSSymbol s_clrtoeol;
  JSSymbol s_echo;
  JSSymbol s_endwin;
  JSSymbol s_getch;
  JSSymbol s_initscr;
  JSSymbol s_keypad;
  JSSymbol s_move;
  JSSymbol s_mvaddstr;
  JSSymbol s_mvaddsubstr;
  JSSymbol s_mvgetch;
  JSSymbol s_nocbreak;
  JSSymbol s_noecho;
  JSSymbol s_refresh;
  JSSymbol s_standend;
  JSSymbol s_standout;

  JSSymbol s_LINES;
  JSSymbol s_COLS;
};

typedef struct curses_ctx_st CursesCtx;

/*
 * Static functions.
 */

/* Method proc. */
static int
method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	void *instance_context, JSSymbol method,
	JSNode *result_return, JSNode *args)
{
  CursesCtx *ctx = builtin_info->obj_context;
  char *cp;

  /* The default result. */
  result_return->type = JS_BOOLEAN;
  result_return->u.vboolean = 1;

  if (method == ctx->s_addstr)
    {
      if (args->u.vinteger != 1)
	goto argument_error;
      if (args[1].type != JS_STRING)
	goto argument_type_error;

      cp = js_string_to_c_string (vm, &args[1]);
      addstr (cp);
      js_free (cp);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_attron)
    {
      if (args->u.vinteger != 1)
	goto argument_error;
      if (args[1].type != JS_INTEGER)
	goto argument_type_error;

      attron (args[1].u.vinteger);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_attroff)
    {
      if (args->u.vinteger != 1)
	goto argument_error;
      if (args[1].type != JS_INTEGER)
	goto argument_type_error;

      attroff (args[1].u.vinteger);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_beep)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      beep ();
    }
  /* ********************************************************************** */
  else if (method == ctx->s_cbreak)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      cbreak ();
    }
  /* ********************************************************************** */
  else if (method == ctx->s_clear)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      clear ();
    }
  /* ********************************************************************** */
  else if (method == ctx->s_clrtobot)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      clrtobot ();
    }
  /* ********************************************************************** */
  else if (method == ctx->s_clrtoeol)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      clrtoeol ();
    }
  /* ********************************************************************** */
  else if (method == ctx->s_echo)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      echo ();
    }
  /* ********************************************************************** */
  else if (method == ctx->s_endwin)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      endwin ();
    }
  /* ********************************************************************** */
  else if (method == ctx->s_getch)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      result_return->type = JS_INTEGER;
      result_return->u.vinteger = getch ();
    }
  /* ********************************************************************** */
  else if (method == ctx->s_initscr)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      if (initscr () == (WINDOW *) ERR)
	result_return->u.vboolean = 0;
    }
  /* ********************************************************************** */
  else if (method == ctx->s_keypad)
    {
      if (args->u.vinteger != 1)
	goto argument_error;
      if (args[1].type != JS_BOOLEAN)
	goto argument_type_error;

      keypad (stdscr, args->u.vboolean);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_move)
    {
      if (args->u.vinteger != 2)
	goto argument_error;
      if (args[1].type != JS_INTEGER || args[2].type != JS_INTEGER)
	goto argument_type_error;

      move (args[1].u.vinteger, args[2].u.vinteger);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_mvaddstr)
    {
      if (args->u.vinteger != 3)
	goto argument_error;
      if (args[1].type != JS_INTEGER || args[2].type != JS_INTEGER
	  || args[3].type != JS_STRING)
	goto argument_type_error;

      cp = js_string_to_c_string (vm, &args[3]);
      mvaddstr (args[1].u.vinteger, args[2].u.vinteger, cp);
      js_free (cp);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_mvaddsubstr)
    {
      int start, length;

      if (args->u.vinteger != 4 && args->u.vinteger != 5)
	goto argument_error;
      if (args[1].type != JS_INTEGER || args[2].type != JS_INTEGER
	  || args[3].type != JS_STRING
	  || args[4].type != JS_INTEGER)
	goto argument_type_error;

      start = args[4].u.vinteger;

      if (args->u.vinteger == 5)
	{
	  if (args[5].type != JS_INTEGER)
	    goto argument_type_error;

	  length = args[5].u.vinteger;
	  if (length < 0)
	    length = 0;
	}
      else
	length = args[3].u.vstring->len;

      if (start < 0)
	start += args[3].u.vstring->len;
      if (start < 0)
	start = 0;
      if (start > args[3].u.vstring->len)
	start = args[3].u.vstring->len;

      if (start + length > args[3].u.vstring->len)
	length = args[3].u.vstring->len - start;

      cp = js_malloc (vm, length + 1);
      memcpy (cp, args[3].u.vstring->data + start, length);
      cp[length] = '\0';
      mvaddstr (args[1].u.vinteger, args[2].u.vinteger, cp);
      js_free (cp);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_mvgetch)
    {
      if (args->u.vinteger != 2)
	goto argument_error;
      if (args[1].type != JS_INTEGER || args[2].type != JS_INTEGER)
	goto argument_type_error;

      result_return->type = JS_INTEGER;
      result_return->u.vinteger = mvgetch (args[1].u.vinteger,
					   args[2].u.vinteger);
    }
  /* ********************************************************************** */
  else if (method == ctx->s_nocbreak)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      nocbreak ();
    }
  /* ********************************************************************** */
  else if (method == ctx->s_noecho)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      noecho ();
    }
  /* ********************************************************************** */
  else if (method == ctx->s_refresh)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      refresh ();
    }
  /* ********************************************************************** */
  else if (method == ctx->s_standend)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      standend ();
    }
  /* ********************************************************************** */
  else if (method == ctx->s_standout)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      standout ();
    }
  /* ********************************************************************** */
  else if (method == vm->syms.s_toString)
    {
      if (args->u.vinteger != 0)
	goto argument_error;

      js_vm_make_static_string (vm, result_return, "Curses", 6);
    }
  /* ********************************************************************** */
  else
    return JS_PROPERTY_UNKNOWN;

  return JS_PROPERTY_FOUND;


  /*
   * Error handling.
   */

 argument_error:
  sprintf (vm->error, "Curses.%s(): illegal amount of arguments",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

 argument_type_error:
  sprintf (vm->error, "Curses.%s(): illegal argument",
	   js_vm_symname (vm, method));
  js_vm_error (vm);

  /* NOTREACHED. */
  return 0;
}


/* Property proc. */
static int
property (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	  void *instance_context, JSSymbol property, int set,
	  JSNode *node)
{
  CursesCtx *ctx = builtin_info->obj_context;

  if (property == ctx->s_LINES)
    {
      if (set)
	goto immutable;

      node->type = JS_INTEGER;
      node->u.vinteger = LINES;
    }
  else if (property == ctx->s_COLS)
    {
      if (set)
	goto immutable;

      node->type = JS_INTEGER;
      node->u.vinteger = COLS;
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
  sprintf (vm->error, "Curses.%s: immutable property",
	   js_vm_symname (vm, property));
  js_vm_error (vm);

  /* NOTREACHED. */
  return 0;
}

/*
 * Global functions.
 */

void
js_ext_curses (JSInterpPtr interp)
{
  CursesCtx *ctx;
  JSBuiltinInfo *info;
  JSNode *n;
  JSVirtualMachine *vm = interp->vm;

  /* Class context. */
  ctx = js_calloc (vm, 1, sizeof (*ctx));

  ctx->s_addstr 		= js_vm_intern (vm, "addstr");
  ctx->s_attron 		= js_vm_intern (vm, "attron");
  ctx->s_attroff 		= js_vm_intern (vm, "attroff");
  ctx->s_beep	 		= js_vm_intern (vm, "beep");
  ctx->s_cbreak 		= js_vm_intern (vm, "cbreak");
  ctx->s_clear	 		= js_vm_intern (vm, "clear");
  ctx->s_clrtobot 		= js_vm_intern (vm, "clrtobot");
  ctx->s_clrtoeol 		= js_vm_intern (vm, "clrtoeol");
  ctx->s_echo	 		= js_vm_intern (vm, "echo");
  ctx->s_endwin 		= js_vm_intern (vm, "endwin");
  ctx->s_getch	 		= js_vm_intern (vm, "getch");
  ctx->s_initscr 		= js_vm_intern (vm, "initscr");
  ctx->s_keypad 		= js_vm_intern (vm, "keypad");
  ctx->s_move	 		= js_vm_intern (vm, "move");
  ctx->s_mvaddstr 		= js_vm_intern (vm, "mvaddstr");
  ctx->s_mvaddsubstr 		= js_vm_intern (vm, "mvaddsubstr");
  ctx->s_mvgetch	 	= js_vm_intern (vm, "mvgetch");
  ctx->s_nocbreak 		= js_vm_intern (vm, "nocbreak");
  ctx->s_noecho 		= js_vm_intern (vm, "noecho");
  ctx->s_refresh 		= js_vm_intern (vm, "refresh");
  ctx->s_standend		= js_vm_intern (vm, "standend");
  ctx->s_standout		= js_vm_intern (vm, "standout");

  ctx->s_LINES	 		= js_vm_intern (vm, "LINES");
  ctx->s_COLS	 		= js_vm_intern (vm, "COLS");

  /* Object information. */
  info = js_vm_builtin_info_create (vm);

  info->method_proc		= method;
  info->property_proc		= property;
  info->obj_context		= ctx;
  info->obj_context_delete	= js_free;

  /* Define it. */
  n = &vm->globals[js_vm_intern (vm, "Curses")];
  js_vm_builtin_create (vm, n, info, NULL);
}
