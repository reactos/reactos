/*
 * Optimized `switch' instruction dispatcher.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/ksrc/vmswitch.c,v $
 * $Id: vmswitch.c,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

#include "jsint.h"

/*
 * Types and definitions.
 */

#define SAVE_OP(a)				\
reloc[cp - fixed_code - 1] = &f->code[cpos];	\
f->code[cpos++].u.op = (a)

#define SAVE_INT8(a)	f->code[cpos++].u.i8 = (a)
#define SAVE_INT16(a)	f->code[cpos++].u.i16 = (a)
#define SAVE_INT32(a)	f->code[cpos++].u.i32 = (a)

#define ARG_INT32()	f->code[cpos].u.i32

#define READ_INT8(var)	(var) = (pc++)->u.i8
#define READ_INT16(var)	(var) = (pc++)->u.i16
#define READ_INT32(var)	(var) = (pc++)->u.i32

#define SETPC(ofs)		pc = (ofs)
#define SETPC_RELATIVE(ofs)	pc += (ofs)

#define CALL_USER_FUNC(f)	pc = ((Function *) (f))->code

#define DONE() goto done

#define ERROR(msg)		\
  do {				\
    JS_SAVE_REGS ();		\
    strcpy (vm->error, (msg));	\
    js_vm_error (vm);		\
    /* NOTREACHED */		\
  } while (0)


struct compiled_st
{
  union
  {
    void *ptr;
    JSUInt8 op;
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
 * Prototypes for static functions.
 */

static void function_destroy (void *ptr);

static Function *link_code (JSVirtualMachine *vm, unsigned char *code,
			    unsigned int code_len,
			    unsigned int consts_offset,
			    unsigned char *debug_info,
			    unsigned int debug_info_len,
			    unsigned int code_offset);

static void execute_code (JSVirtualMachine *vm, JSNode *object, Function *f,
			  unsigned int argc, JSNode *argv);


/*
 * Global functions.
 */

int
js_vm_switch_exec (JSVirtualMachine *vm, JSByteCode *bc, JSSymtabEntry *symtab,
		   unsigned int num_symtab_entries,
		   unsigned int consts_offset,
		   unsigned int anonymous_function_offset,
		   unsigned char *debug_info, unsigned int debug_info_len,
		   JSNode *object, JSNode *func,
		   unsigned int argc, JSNode *argv)
{
  int i;
  unsigned int ui;
  Function *global_f = NULL;
  Function *f;
  unsigned char *code = NULL;
  char buf[512];

  if (bc)
    {
      /* Executing byte-code. */

      /* Find the code section. */
      for (i = 0; i < bc->num_sects; i++)
	if (bc->sects[i].type == JS_BCST_CODE)
	  code = bc->sects[i].data;
      assert (code != NULL);

      /* Enter all functions to the known functions of the VM. */
      for (i = 0; i < num_symtab_entries; i++)
	{
	  /* Link the code to our environment. */
	  f = link_code (vm, code + symtab[i].offset,
			 symtab[i + 1].offset - symtab[i].offset,
			 consts_offset, debug_info, debug_info_len,
			 symtab[i].offset);
	  f->name = js_strdup (vm, symtab[i].name);

	  if (strcmp (symtab[i].name, JS_GLOBAL_NAME) == 0)
	    global_f = f;
	  else
	    {
	      int is_anonymous = 0;

	      /* Check for the anonymous function. */
	      if (symtab[i].name[0] == '.' && symtab[i].name[1] == 'F'
		  && symtab[i].name[2] == ':')
		is_anonymous = 1;

	      if (vm->verbose > 3)
		{
		  sprintf (buf, "VM: link: %s(): start=%d, length=%d",
			   symtab[i].name, symtab[i].offset,
			   symtab[i + 1].offset - symtab[i].offset);
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
			   (unsigned int) atoi (symtab[i].name + 3)
			   + anonymous_function_offset);
		  ui = js_vm_intern (vm, buf);
		}
	      else
		ui = js_vm_intern (vm, symtab[i].name);

	      vm->globals[ui].type = JS_FUNC;
	      vm->globals[ui].u.vfunction = js_vm_make_function (vm, f);
	    }
	}
    }
  else
    {
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

      execute_code (vm, object, f, argc, argv);
    }

  if (global_f)
    {
      if (vm->verbose > 1)
	{
	  sprintf (buf, "VM: exec: %s%s", global_f->name,
		   JS_HOST_LINE_BREAK);
	  js_iostream_write (vm->s_stderr, buf, strlen (buf));
	}

      /* Execute. */
      execute_code (vm, NULL, global_f, 0, NULL);
    }

  return 1;
}


const char *
js_vm_switch_func_name (JSVirtualMachine *vm, void *program_counter)
{
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
}


const char *
js_vm_switch_debug_position (JSVirtualMachine *vm,
			     unsigned int *linenum_return)
{
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
}


/*
 * Static functions.
 */

static void
function_destroy (void *ptr)
{
  Function *f = ptr;

  js_free (f->name);
  js_free (f->code);

  if (f->debug.file)
    js_free (f->debug.file);
  if (f->debug.info)
    js_free (f->debug.info);
}


static Function *
link_code (JSVirtualMachine *vm, unsigned char *code, unsigned int code_len,
	   unsigned int consts_offset, unsigned char *debug_info,
	   unsigned int debug_info_len, unsigned int code_offset)
{
  unsigned char *cp, *end;
  JSInt32 i;
  Compiled **reloc;
  unsigned int cpos;
  Function *f;
  unsigned char *fixed_code;
  unsigned int ui;
  char *debug_filename = "unknown";
  char buf[512];

  /* Terminate the code with op `done'. */
  fixed_code = js_malloc (vm, code_len + 1);
  memcpy (fixed_code, code, code_len);
  fixed_code[code_len] = 1;	/* op `done' */

  cp = fixed_code;
  end = fixed_code + code_len + 1;

  /* Alloc function closure. */
  f = js_vm_alloc_destroyable (vm, sizeof (*f));
  f->destroy = function_destroy;

  /* Allocate space for our compiled code.  <length> is enought. */
  f->code = js_malloc (vm, (code_len + 1) * sizeof (Compiled));
  reloc = js_calloc (vm, code_len + 1, sizeof (Compiled *));

  /* Link phase 1: constants and symbols. */
  cpos = 0;
  while (cp < end)
    {
      switch (*cp++)
	{
	  /* include c1switch.h */
#include "c1switch.h"
	  /* end include c1switch.h */
	}
    }
  f->length = cpos;

  /* Link phase 2: relative jumps. */
  cp = fixed_code;
  cpos = 0;
  while (cp < end)
    {
      switch (*cp++)
	{
	  /* include c2switch.h */
#include "c2switch.h"
	  /* end include c2switch.h */
	}
    }
  /* Handle debug info. */
  if (debug_info)
    {
      unsigned int di_start = code_offset;
      unsigned int di_end = code_offset + code_len;
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

  return f;
}


/*
 * Execute byte code by using the `switch' dispatch technique.
 */

static void
execute_code (JSVirtualMachine *vm, JSNode *object, Function *f,
	      unsigned int argc, JSNode *argv)
{
  JSNode *sp;
  JSNode *fp;
  JSNode *function;
  JSNode builtin_result;
  Compiled *pc;
  JSInt32 i, j;
  JSInt8 i8;
  char buf[512];

  /* Create the initial stack frame by hand. */
  sp = vm->sp;

  /* Protect the function from gc. */
  JS_SP0->type = JS_FUNC;
  JS_SP0->u.vfunction = js_vm_make_function (vm, f);
  JS_PUSH ();

  /* Push arguments to the stack. */
  i = argc;
  for (i--; i >= 0; i--)
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

  /* Ok, now we are ready to run. */

  while (1)
    {
      switch ((pc++)->u.op)
	{
	  /* include eswitch.h */
#include "eswitch.h"
	  /* end include eswitch.h */

	default:
	  sprintf (buf, "execute_code: unknown opcode %d%s",
		   (pc - 1)->u.op, JS_HOST_LINE_BREAK);
	  js_iostream_write (vm->s_stderr, buf, strlen (buf));
	  js_iostream_flush (vm->s_stderr);

	  abort ();
	  break;
	}
    }

 done:

  /* All done. */

  JS_COPY (&vm->exec_result, JS_SP1);
}
