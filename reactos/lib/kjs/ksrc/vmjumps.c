/*
 * Optimized `jumps' instruction dispatcher.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/ksrc/vmjumps.c,v $
 * $Id: vmjumps.c,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

#include "jsint.h"

#if __GNUC__ && !DISABLE_JUMPS

/*
 * Types and definitions.
 */

#define SAVE_OP(a)				\
reloc[cp - code_start - 1] = &f->code[cpos];	\
f->code[cpos++].u.ptr = (a)

#define SAVE_INT8(a)	f->code[cpos++].u.i8 = (a)
#define SAVE_INT16(a)	f->code[cpos++].u.i16 = (a)
#define SAVE_INT32(a)	f->code[cpos++].u.i32 = (a)

#define ARG_INT32()	f->code[cpos].u.i32

#if BC_OPERAND_HOOKS

#define NEXT()								\
  do {									\
    if (++vm->hook_operand_count >= vm->hook_operand_count_trigger)	\
      {									\
	JS_CALL_HOOK (JS_VM_EVENT_OPERAND_COUNT);			\
	vm->hook_operand_count = 0;					\
      }									\
    goto *((pc++)->u.ptr);						\
  } while (0)

#else /* not BC_OPERAND_HOOKS */

#define NEXT() goto *((pc++)->u.ptr)

#endif /* not BC_OPERAND_HOOKS */

#define READ_INT8(var)  (var) = (pc++)->u.i8
#define READ_INT16(var) (var) = (pc++)->u.i16
#define READ_INT32(var) (var) = (pc++)->u.i32

#define SETPC(ofs)		pc = (ofs)
#define SETPC_RELATIVE(ofs)	pc += (ofs)

#define CALL_USER_FUNC(f) 	pc = ((Function *) (f))->code

#define DONE() goto done

#define ERROR(msg)		\
  do {				\
    JS_SAVE_REGS ();		\
    strcpy (vm->error, (msg));	\
    js_vm_error (vm);		\
    /* NOTREACHED */		\
  } while (0)

#if PROFILING
#define OPERAND(op)  vm->prof_op = (op)
#else
#define OPERAND(op)
#endif

struct compiled_st
{
  union
  {
    void *ptr;
    JSInt8 i8;
    JSInt16 i16;
    JSInt32 i32;
  } u;
};

typedef struct compiled_st Compiled;

/* Debug information. */
struct debug_info_st
{
  void *pc;
  unsigned int linenum;
};

typedef struct debug_info_st DebugInfo;

struct function_st
{
  JSHeapDestroyableCB destroy;

  char *name;
  Compiled *code;
  unsigned int length;

  struct
  {
    char *file;
    unsigned int num_info;
    DebugInfo *info;
  } debug;
};

typedef struct function_st Function;

/*
 * Static functions.
 */

static void
function_destroy (void *ptr)
{
  Function *f = ptr;

  /* Name. */
  js_free (f->name);

  /* Code. */
  js_free (f->code);

  /* Debug info. */
  if (f->debug.file)
    js_free (f->debug.file);
  if (f->debug.info)
    js_free (f->debug.info);
}


#endif /* not (__GNUC__ && !DISABLE_JUMPS) */

/*
 * Global functions.
 */

int
js_vm_jumps_exec (JSVirtualMachine *vm, JSByteCode *bc, JSSymtabEntry *symtab,
		  unsigned int num_symtab_entries, unsigned int consts_offset,
		  unsigned int anonymous_function_offset,
		  unsigned char *debug_info, unsigned int debug_info_len,
		  JSNode *object, JSNode *func,
		  unsigned int argc, JSNode *argv)
{
#if __GNUC__ && !DISABLE_JUMPS
  int s;
  unsigned int ui;
  Function *global_f = NULL;
  Function *f = NULL;
  unsigned char *code = NULL;
  JSNode *sp = NULL;
  JSNode *fp = NULL;
  Compiled *pc = NULL;
  char *debug_filename = "unknown";
  char buf[512];

  if (bc)
    {
      /* Executing byte-code. */

      /* Find the code section. */
      for (s = 0; s < bc->num_sects; s++)
	if (bc->sects[s].type == JS_BCST_CODE)
	  code = bc->sects[s].data;
      assert (code != NULL);

      /* Enter all functions to the known functions of the VM. */
      for (s = 0; s < num_symtab_entries; s++)
	{
	  /* We need one function. */
	  f = js_vm_alloc_destroyable (vm, sizeof (*f));
	  f->destroy = function_destroy;
	  f->name = js_strdup (vm, symtab[s].name);

	  if (strcmp (symtab[s].name, JS_GLOBAL_NAME) == 0)
	    global_f = f;
	  else
	    {
	      int is_anonymous = 0;

	      /* Check for the anonymous function. */
	      if (symtab[s].name[0] == '.' && symtab[s].name[1] == 'F'
		  && symtab[s].name[2] == ':')
		is_anonymous = 1;

	      if (vm->verbose > 3)
		{
		  sprintf (buf, "VM: link: %s(): start=%d, length=%d",
			   symtab[s].name, symtab[s].offset,
			   symtab[s + 1].offset - symtab[s].offset);
		  if (is_anonymous)
		    sprintf (buf + strlen (buf),
			     ", relocating with offset %u",
			     anonymous_function_offset);
		  strcat (buf, JS_HOST_LINE_BREAK);
		  js_iostream_write (vm->s_stderr, buf, strlen (buf));
		}

	      if (is_anonymous)
		{
		  sprintf (buf, ".F:%u",
			   (unsigned int) atoi (symtab[s].name + 3)
			   + anonymous_function_offset);
		  ui = js_vm_intern (vm, buf);
		}
	      else
		ui = js_vm_intern (vm, symtab[s].name);

	      vm->globals[ui].type = JS_FUNC;
	      vm->globals[ui].u.vfunction = js_vm_make_function (vm, f);
	    }

	  /* Link the code to our environment.*/
	  {
	    unsigned char *cp;
	    unsigned char *code_start, *code_end;
	    unsigned char *fixed_code;
	    JSInt32 i;
	    unsigned int cpos;
	    Compiled **reloc;
	    unsigned int length;

	    length = symtab[s + 1].offset - symtab[s].offset + 1;

	    /*
	     * Allocate space for our compiled code.  <length> is enought,
	     * but is is almost always too much.  Who cares?
	     */
	    f->code = js_malloc (vm, length * sizeof (Compiled));
	    reloc = js_calloc (vm, 1, length * sizeof (Compiled *));
	    fixed_code = js_malloc (vm, length);

	    memcpy (fixed_code, code + symtab[s].offset, length);
	    fixed_code[length - 1] = 1; /* op `done'. */

	    code_start =  fixed_code;
	    code_end = code_start + length;

	    /* Link phase 1: constants and symbols. */
	    cp = code_start;
	    cpos = 0;
	    while (cp < code_end)
	      {
		switch (*cp++)
		  {
		    /* include c1jumps.h */
#include "c1jumps.h"
		    /* end include c1jumps.h */
		  }
	      }
	    f->length = cpos;

	    /* Link phase 2: relative jumps. */
	    cp = code_start;
	    cpos = 0;
	    while (cp < code_end)
	      {
		switch (*cp++)
		  {
		    /* include c2jumps.h */
#include "c2jumps.h"
		    /* end include c2jumps.h */
		  }
	      }

	    /* Handle debug info. */
	    if (debug_info)
	      {
		unsigned int di_start = symtab[s].offset;
		unsigned int di_end = symtab[s + 1].offset;
		unsigned int ln;

		for (; debug_info_len > 0;)
		  {
		    switch (*debug_info)
		      {
		      case JS_DI_FILENAME:
			debug_info++;
			debug_info_len--;

			JS_BC_READ_INT32 (debug_info, ui);
			debug_info += 4;
			debug_info_len -= 4;

			f->debug.file = js_malloc (vm, ui + 1);
			memcpy (f->debug.file, debug_info, ui);
			f->debug.file[ui] = '\0';

			debug_filename = f->debug.file;

			debug_info += ui;
			debug_info_len -= ui;
			break;

		      case JS_DI_LINENUMBER:
			JS_BC_READ_INT32 (debug_info + 1, ui);
			if (ui > di_end)
			  goto debug_info_done;

			/* This belongs to us (maybe). */
			debug_info += 5;
			debug_info_len -= 5;

			JS_BC_READ_INT32 (debug_info, ln);
			debug_info += 4;
			debug_info_len -= 4;

			if (di_start <= ui && ui <= di_end)
			  {
			    ui -= di_start;
			    f->debug.info = js_realloc (vm, f->debug.info,
							(f->debug.num_info + 1)
							* sizeof (DebugInfo));

			    f->debug.info[f->debug.num_info].pc = reloc[ui];
			    f->debug.info[f->debug.num_info].linenum = ln;
			    f->debug.num_info++;
			  }
			break;

		      default:
			sprintf (buf,
				 "VM: unknown debug information type %d%s",
				 *debug_info, JS_HOST_LINE_BREAK);
			js_iostream_write (vm->s_stderr, buf, strlen (buf));
			js_iostream_flush (vm->s_stderr);

			abort ();
			break;
		      }
		  }

	      debug_info_done:
		if (f->debug.file == NULL)
		  f->debug.file = js_strdup (vm, debug_filename);
	      }

	    js_free (reloc);
	    js_free (fixed_code);
	  }
	}
    }
  else
    {
      int i;

      /* Applying arguments to function. */
      if (func->type != JS_FUNC)
	{
	  sprintf (vm->error, "illegal function in apply");
	  return 0;
	}

      if (vm->verbose > 1)
	{
	  sprintf (buf, "VM: calling function%s",
		   JS_HOST_LINE_BREAK);
	  js_iostream_write (vm->s_stderr, buf, strlen (buf));
	}
      f = func->u.vfunction->implementation;

      /* Init stack. */
      sp = vm->sp;

      /*
       * Save the applied function to the stack.  If our script
       * overwrites the function, the function will not be deleted
       * under us, since it is protected from the gc in the stack.
       */
      JS_COPY (JS_SP0, func);
      JS_PUSH ();

      /* Push arguments to the stack. */
      for (i = argc - 1; i >= 0; i--)
	{
	  JS_COPY (JS_SP0, &argv[i]);
	  JS_PUSH ();
	}

      /* This pointer. */
      if (object)
	JS_COPY (JS_SP0, object);
      else
	JS_SP0->type = JS_NULL;
      JS_PUSH ();

      /* Init fp and pc so our SUBROUTINE_CALL will work. */
      fp = NULL;
      pc = NULL;

      JS_SUBROUTINE_CALL (f);

      /* Run. */
      NEXT ();
    }

  if (global_f)
    {
      if (vm->verbose > 1)
	{
	  sprintf (buf, "VM: exec: %s%s", JS_GLOBAL_NAME,
		   JS_HOST_LINE_BREAK);
	  js_iostream_write (vm->s_stderr, buf, strlen (buf));
	}

      /* Create the initial stack frame by hand.  */
      sp = vm->sp;

      /*
       * Push the global function to the stack.  There it is protected
       * from the garbage collection, as long, as we are executing the
       * global code.  It is also removed automatically, when the
       * execution ends.
       */
      JS_SP0->type = JS_FUNC;
      JS_SP0->u.vfunction = js_vm_make_function (vm, global_f);
      JS_PUSH ();

      /* Empty this pointer. */
      JS_SP0->type = JS_NULL;
      JS_PUSH ();

      /* Init fp and pc so our JS_SUBROUTINE_CALL macro works. */
      fp = NULL;
      pc = NULL;

      JS_SUBROUTINE_CALL (global_f);

      /* Run. */
      NEXT ();
    }

  /* The smart done label. */

 done:

  /*
   * The return value from function calls and global evals is at JS_SP1.
   * If <sp> is NULL, then we were linking byte-code that didn't have
   * .global section.
   */
  if (sp)
    JS_COPY (&vm->exec_result, JS_SP1);
  else
    vm->exec_result.type = JS_UNDEFINED;

  /* All done. */
  return 1;

  /* And finally, include the operands. */
  {
    JSNode builtin_result;
    JSNode *function;
    JSInt32 i, j;
    JSInt8 i8;

    /* include ejumps.h */
#include "ejumps.h"
    /* end include ejumps.h */
  }
#else /* not (__GNUC__ && !DISABLE_JUMPS) */
  return 0;
#endif /* not (__GNUC__ && !DISABLE_JUMPS) */
}


const char *
js_vm_jumps_func_name (JSVirtualMachine *vm, void *program_counter)
{
#if __GNUC__ && !DISABLE_JUMPS
  int i;
  Function *f;
  Compiled *pc = program_counter;
  JSNode *sp = vm->sp;

  /* Check the globals. */
  for (i = 0; i < vm->num_globals; i++)
    if (vm->globals[i].type == JS_FUNC)
      {
	f = (Function *) vm->globals[i].u.vfunction->implementation;
	if (f->code < pc && pc < f->code + f->length)
	  return f->name;
      }

  /* No luck.  Let's try the stack. */
  for (sp++; sp < vm->stack + vm->stack_size; sp++)
    if (sp->type == JS_FUNC)
      {
	f = (Function *) sp->u.vfunction->implementation;
	if (f->code < pc && pc < f->code + f->length)
	  return f->name;
      }

  /* Still no matches.  This shouldn't be reached... ok, who cares? */
  return JS_GLOBAL_NAME;

#else /* not (__GNUC__ && !DISABLE_JUMPS) */
  return "";
#endif /* not (__GNUC__ && !DISABLE_JUMPS) */
}


const char *
js_vm_jumps_debug_position (JSVirtualMachine *vm, unsigned int *linenum_return)
{
#if __GNUC__ && !DISABLE_JUMPS
  int i;
  Function *f;
  void *program_counter = vm->pc;
  Compiled *pc = vm->pc;
  JSNode *sp = vm->sp;
  unsigned int linenum = 0;

  /* Check the globals. */
  for (i = 0; i < vm->num_globals; i++)
    if (vm->globals[i].type == JS_FUNC)
      {
	f = (Function *) vm->globals[i].u.vfunction->implementation;
	if (f->code < pc && pc < f->code + f->length)
	  {
	  found:

	    /* Ok, found it. */
	    if (f->debug.file == NULL)
	      /* No debugging information available for this function. */
	      return NULL;

	    /* Find the correct pc position. */
	    for (i = 0; i < f->debug.num_info; i++)
	      {
		if (f->debug.info[i].pc > program_counter)
		  break;

		linenum = f->debug.info[i].linenum;
	      }

	    *linenum_return = linenum;
	    return f->debug.file;
	  }
      }

  /* No luck.  Let's try the stack. */
  for (sp++; sp < vm->stack + vm->stack_size; sp++)
    if (sp->type == JS_FUNC)
      {
	f = (Function *) sp->u.vfunction->implementation;
	if (f->code < pc && pc < f->code + f->length)
	  /* Found it. */
	  goto found;
      }

  /* Couldn't find the function we are executing. */
  return NULL;

#else /* not (__GNUC__ && !DISABLE_JUMPS) */
  return NULL;
#endif /* not (__GNUC__ && !DISABLE_JUMPS) */
}
