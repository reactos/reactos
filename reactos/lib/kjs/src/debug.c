/*
 * Debugging utilities.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/debug.c,v $
 * $Id: debug.c,v 1.1 2004/01/10 20:38:18 arty Exp $
 */

#include "jsint.h"

/*
 * Global functions.
 */

#define STRING_MAX_PRINT_LEN	10

void
js_vm_stacktrace (JSVirtualMachine *vm, unsigned int num_frames)
{
  unsigned int frame = 0;
  JSNode *sp = vm->sp;
  void *pc = vm->pc;
  JSNode *fp;
  char buf[512];
  int i;

  sprintf (buf, "VM: stacktrace: stacksize=%d, used=%d%s",
	   vm->stack_size,
	   (vm->stack + vm->stack_size - sp),
	   JS_HOST_LINE_BREAK);
  js_iostream_write (vm->s_stderr, buf, strlen (buf));

  /* STACKFRAME */

  /* Find frame pointer. */
  for (fp = sp + 1; fp->type != JS_IPTR; fp++)
    ;

  /* The first iptr is the return address. */
  fp++;

  /* The second iptr is the with pointer. */
  fp++;

  /* The third item is a JS_ARGS_FIX node. */
  assert (fp->type == JS_ARGS_FIX);
  fp++;

  while (fp && frame < num_frames)
    {
      JSNode *n;
      const char *func_name = js_vm_func_name (vm, pc);

      sprintf (buf, "#%-3u %s%s:", frame++, func_name,
	       func_name[0] == '.' ? "" : "()");
      js_iostream_write (vm->s_stderr, buf, strlen (buf));

      if (vm->verbose_stacktrace)
	{
	  sprintf (buf,
		   " ra=0x%lx, wp=0x%lx, af=%d:%d, ofp=0x%lx",
		   (unsigned long) (fp - 3)->u.iptr,
		   (unsigned long) JS_WITHPTR->u.iptr,
		   JS_ARGS_FIXP->u.args_fix.argc,
		   JS_ARGS_FIXP->u.args_fix.delta,
		   (unsigned long) fp->u.iptr);
	  js_iostream_write (vm->s_stderr, buf, strlen (buf));
	}

      for (n = sp + 1; n != fp - 3; n++)
	{
	  switch (n->type)
	    {
	    case JS_UNDEFINED:
	      sprintf (buf, " undefined");
	      break;

	    case JS_NULL:
	      sprintf (buf, " null");
	      break;

	    case JS_BOOLEAN:
	      sprintf (buf, " %s", n->u.vboolean ? "true" : "false");
	      break;

	    case JS_INTEGER:
	      sprintf (buf, " %ld", n->u.vinteger);
	      break;

	    case JS_STRING:
	      if (n->u.vstring->len > STRING_MAX_PRINT_LEN)
		sprintf (buf, " \"%.*s...\"",
			 STRING_MAX_PRINT_LEN,
			 n->u.vstring->data);
	      else
		sprintf (buf, " \"%.*s\"",
			 (int) n->u.vstring->len,
			 n->u.vstring->data);
	      break;

	    case JS_FLOAT:
	      sprintf (buf, " %g", n->u.vfloat);
	      break;

	    case JS_ARRAY:
	      sprintf (buf, " array");
	      break;

	    case JS_OBJECT:
	      sprintf (buf, " object");
	      break;

	    case JS_SYMBOL:
	      sprintf (buf, " %s", js_vm_symname (vm, n->u.vsymbol));
	      break;

	    case JS_BUILTIN:
	      sprintf (buf, " builtin");
	      break;

	    case JS_FUNC:
	      sprintf (buf, " function");
	      break;

	    case JS_IPTR:
	      sprintf (buf, " 0x%lx", (unsigned long) n->u.iptr);
	      break;

	    case JS_ARGS_FIX:
	      sprintf (buf, " <num=%d, delta=%d>", n->u.args_fix.argc,
		       n->u.args_fix.delta);
	      break;

	    default:
	      sprintf (buf, " type=%d???", n->type);
	      break;
	    }

	  js_iostream_write (vm->s_stderr, buf, strlen (buf));
	}

      js_iostream_write (vm->s_stderr, JS_HOST_LINE_BREAK,
			 JS_HOST_LINE_BREAK_LEN);

      /* Move to the caller. */
      sp = fp;
      pc = fp[-3].u.iptr;
      fp = fp->u.iptr;
    }
}
