/*
 * Common parts for the JavaScript virtual machine.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/vm.c,v $
 * $Id: vm.c,v 1.1 2004/01/10 20:38:18 arty Exp $
 */

#include "jsint.h"

/*
 * Types and definitions.
 */

/*
 * Collect garbage if we allocated more than GC_TRIGGER bytes of
 * memory since the last gc.
 */
#if SIZEOF_INT == 2
#define GC_TRIGGER (1L * 1024L * 1024L)
#else
#define GC_TRIGGER (2 * 1024 * 1024)
#endif

/*
 * Prototypes for static functions.
 */

static void intern_builtins (JSVirtualMachine *vm);


/*
 * Global functions.
 */

JSVirtualMachine *
js_vm_create (unsigned int stack_size, JSVMDispatchMethod dispatch_method,
	      unsigned int verbose, int stacktrace_on_error,
	      JSIOStream *s_stdin, JSIOStream *s_stdout, JSIOStream *s_stderr)
{
  JSVirtualMachine *vm;

  vm = js_calloc (NULL, 1, sizeof (*vm));
  if (vm == NULL)
    return NULL;

  vm->verbose = verbose;
  vm->stacktrace_on_error = stacktrace_on_error;
  vm->warn_undef = 1;

  /* Set the system streams. */
  vm->s_stdin = s_stdin;
  vm->s_stdout = s_stdout;
  vm->s_stderr = s_stderr;

  /* Resolve the dispatch method. */
  switch (dispatch_method)
    {
    case JS_VM_DISPATCH_SWITCH_BASIC:
#if ALL_DISPATCHERS
      vm->dispatch_method 		= dispatch_method;
      vm->dispatch_method_name		= "switch-basic";
      vm->dispatch_execute 		= js_vm_switch0_exec;
      vm->dispatch_func_name		= js_vm_switch0_func_name;
      vm->dispatch_debug_position 	= js_vm_switch0_debug_position;
#endif
      break;

    case JS_VM_DISPATCH_JUMPS:
#if __GNUC__ && !DISABLE_JUMPS
      vm->dispatch_method		= dispatch_method;
      vm->dispatch_method_name		= "jumps";
      vm->dispatch_execute		= js_vm_jumps_exec;
      vm->dispatch_func_name		= js_vm_jumps_func_name;
      vm->dispatch_debug_position	= js_vm_jumps_debug_position;
#endif /* not (__GNUC__ && !DISABLE_JUMPS) */
      break;

    case JS_VM_DISPATCH_SWITCH:
      /* This is the default, let the default catcher handle us. */
      break;
    }

  if (vm->dispatch_execute == NULL)
    {
      /* Set the default that is the optimized switch. */
      vm->dispatch_method		= JS_VM_DISPATCH_SWITCH;
      vm->dispatch_method_name		= "switch";
      vm->dispatch_execute		= js_vm_switch_exec;
      vm->dispatch_func_name		= js_vm_switch_func_name;
      vm->dispatch_debug_position	= js_vm_switch_debug_position;
    }

  vm->stack_size = stack_size;
  vm->stack = js_malloc (NULL, vm->stack_size * sizeof (*vm->stack));
  if (vm->stack == NULL)
    {
      js_free (vm);
      return NULL;
    }

  /* Set the initial stack pointer. */
  vm->sp = vm->stack + vm->stack_size - 1;

  vm->gc.trigger = GC_TRIGGER;

  /* We need a toplevel here. */
  {
    JSErrorHandlerFrame handler;
    int result = 1;

    memset (&handler, 0, sizeof (handler));
    handler.next = vm->error_handler;
    vm->error_handler = &handler;

    if (setjmp (vm->error_handler->error_jmp))
      /* An error occurred. */
      result = 0;
    else
      {
	/* Intern some commonly used symbols. */
	vm->syms.s___proto__	= js_vm_intern (vm, "__proto__");
	vm->syms.s_prototype	= js_vm_intern (vm, "prototype");
	vm->syms.s_toSource	= js_vm_intern (vm, "toSource");
	vm->syms.s_toString	= js_vm_intern (vm, "toString");
	vm->syms.s_valueOf	= js_vm_intern (vm, "valueOf");

	/* Intern system built-in objects. */
	intern_builtins (vm);
      }

    /* Pop the error handler. */
    vm->error_handler = vm->error_handler->next;

    if (result == 0)
      {
	/* Argh, the initialization failed. */
	js_vm_destroy (vm);
	return NULL;
      }
  }

  return vm;
}


void
js_vm_destroy (JSVirtualMachine *vm)
{
  int i;
  JSHeapBlock *hb, *hb2;
  JSErrorHandlerFrame *f, *f2;
  JSHashBucket *hashb, *hashb_next;

  /* Free all objects from the heap. */
  js_vm_clear_heap (vm);

  /* Free the constants. */

  for (i = 0; i < vm->num_consts; i++)
    if (vm->consts[i].type == JS_STRING)
      js_free (vm->consts[i].u.vstring->data);
  js_free (vm->consts);

  /* Free the globals. */

  for (i = 0; i < JS_HASH_TABLE_SIZE; i++)
    for (hashb = vm->globals_hash[i]; hashb; hashb = hashb_next)
      {
	hashb_next = hashb->next;
	js_free (hashb->name);
	js_free (hashb);
      }
  js_free (vm->globals);

  /* Stack. */
  js_free (vm->stack);

  /* Heap blocks. */
  for (hb = vm->heap; hb; hb = hb2)
    {
      hb2 = hb->next;
      js_free (hb);
    }

  /* Error handlers. */
  for (f = vm->error_handler; f; f = f2)
    {
      f2 = f->next;
      js_free (f);
    }

#if PROFILING
#define NUM_OPS 68

  /* Dump profiling data to the stderr. */
  {
    unsigned int total = 0;
    int i;

    /* Count total interrupts. */
    for (i = 0; i <= NUM_OPS; i++)
      total += vm->prof_count[i];

    /* Dump individual statistics. */
    for (i = 0; i <= NUM_OPS; i++)
      {
	char buf[512];

	sprintf (buf, "%d\t%u\t%.2f%s",
		 i, vm->prof_count[i],
		 (double) vm->prof_count[i] / total * 100,
		 JS_HOST_LINE_BREAK);

	js_iostream_write (vm->s_stderr);
      }
  }
#endif /* PROFILING */

  /* Flush and free the default system streams. */

  js_iostream_close (vm->s_stdin);
  js_iostream_close (vm->s_stdout);
  js_iostream_close (vm->s_stderr);

  /* And finally, the VM handle. */
  js_free (vm);
}

#if PROFILING
/*
 * The support stuffs for the byte-code operand profiling.
 */

static JSVirtualMachine *profiling_vm = NULL;

static void
sig_alarm (int sig)
{
  if (profiling_vm && profiling_vm->prof_op < 100)
    profiling_vm->prof_count[profiling_vm->prof_op]++;

  signal (sig, sig_alarm);
}

/* Turn on the byte-code operand profiling. */
#define PROFILING_ON()			\
      profiling_vm = vm;		\
      vm->prof_op = 255;		\
      signal (SIGALRM, sig_alarm);	\
      ualarm (1, 1)

/* Turn off the byte-code operand profiling. */
#define PROFILING_OFF()			\
      vm->prof_op = 255;		\
      ualarm (0, 0);			\
      signal (SIGALRM, SIG_IGN);	\
      profiling_vm = NULL

#else /* not PROFILING */

#define PROFILING_ON()
#define PROFILING_OFF()

#endif /* not PROFILING */

int
js_vm_execute (JSVirtualMachine *vm, JSByteCode *bc)
{
  int i, sect;
  unsigned int ui;
  unsigned char *cp;
  unsigned int consts_offset;
  char buf[256];
  JSSymtabEntry *symtab = NULL;
  unsigned int num_symtab_entries = 0;
  unsigned int code_len = 0;
  JSNode *saved_sp;
  JSErrorHandlerFrame *handler, *saved_handler;
  int result = 1;
  unsigned char *debug_info;
  unsigned int debug_info_len;
  unsigned int anonymous_function_offset;

  /* We need a toplevel over the whole function. */

  saved_sp = vm->sp;
  saved_handler = vm->error_handler;

  handler = js_calloc (NULL, 1, sizeof (*handler));
  if (handler == NULL)
    {
      sprintf (vm->error, "VM: out of memory");
      return 0;
    }
  handler->next = vm->error_handler;
  vm->error_handler = handler;

  if (setjmp (vm->error_handler->error_jmp))
    {
      /* Ok, we had an error down there somewhere. */
      result = 0;
    }
  else
    {
      /* The main stuffs for the execute. */

      /* Process constants. */
      consts_offset = vm->num_consts;
      anonymous_function_offset = vm->anonymous_function_next_id;

      for (sect = 0; sect < bc->num_sects; sect++)
	if (bc->sects[sect].type == JS_BCST_CONSTANTS)
	  {
	    cp = bc->sects[sect].data;

	    for (ui = 0; ui < bc->sects[sect].length;)
	      {
		JSNode *c;

		/* Check that we still have space for this constant. */
		if (vm->num_consts >= vm->consts_alloc)
		  {
		    vm->consts = js_realloc (vm, vm->consts,
					     (vm->consts_alloc + 1024)
					     * sizeof (JSNode));
		    vm->consts_alloc += 1024;
		  }
		c = &vm->consts[vm->num_consts++];

		/*  Process this constant. */
		c->type = (JSNodeType) cp[ui++];
		switch (c->type)
		  {
		  case JS_NULL:
		    break;

		  case JS_BOOLEAN:
		    c->u.vboolean = cp[ui++];
		    break;

		  case JS_STRING:
		    c->u.vstring = js_vm_alloc (vm, sizeof (*c->u.vstring));
		    c->u.vstring->staticp = 1;
		    c->u.vstring->prototype = NULL;

		    JS_BC_READ_INT32 (cp + ui, c->u.vstring->len);
		    ui += 4;

		    c->u.vstring->data = js_malloc (vm, c->u.vstring->len + 1);
		    memcpy (c->u.vstring->data, cp + ui, c->u.vstring->len);
		    c->u.vstring->data[c->u.vstring->len] = '\0';

		    ui += c->u.vstring->len;
		    break;

		  case JS_INTEGER:
		    JS_BC_READ_INT32 (cp + ui, c->u.vinteger);
		    ui += 4;
		    break;

		  case JS_FLOAT:
		    memcpy (&c->u.vfloat, cp + ui, 8);
		    ui += 8;
		    break;

		  case JS_SYMBOL:
		    for (i = 0; cp[ui]; ui++, i++)
		      buf[i] = cp[ui];
		    buf[i] = '\0';

		    /* Eat the trailing '\0' from the data. */
		    ui++;

		    if (buf[0] == '.' && buf[1] == 'F' && buf[2] == ':')
		      sprintf (buf + 3, "%u",
			       vm->anonymous_function_next_id++);

		    /* Intern symbol. */
		    c->u.vsymbol = js_vm_intern (vm, buf);
		    break;

		  case JS_BUILTIN:
		    /* Regular expression. */
		    {
		      unsigned char flags;
		      unsigned int length;

		      flags = cp[ui++];

		      JS_BC_READ_INT32 (cp + ui, length);
		      ui += 4;

		      js_builtin_RegExp_new (vm, cp + ui, length, flags, 1,
					     NULL, c);
		      ui += length;
		    }
		    break;

		  case JS_NAN:
		    /* Nothing here. */
		    break;

		  default:
		  case JS_IPTR:
		    sprintf (buf,
			     "js_vm_execute(): unknown constant type %d%s",
			     c->type,
			     JS_HOST_LINE_BREAK);

		    js_iostream_write (vm->s_stderr, buf, strlen (buf));
		    js_iostream_flush (vm->s_stderr);

		    abort ();
		    break;
		  }
	      }

	    /* All done with the constants. */
	    break;
	  }

      /* Check how long the code section is. */
      for (sect = 0; sect < bc->num_sects; sect++)
	if (bc->sects[sect].type == JS_BCST_CODE)
	  {
	    code_len = bc->sects[sect].length;
	    break;
	  }

      /* Process symbol table. */
      for (sect = 0; sect < bc->num_sects; sect++)
	if (bc->sects[sect].type == JS_BCST_SYMTAB)
	  {
	    JSSymtabEntry *se;
	    char buf[257];

	    cp = bc->sects[sect].data;

	    /* The number of symbols. */
	    JS_BC_READ_INT32 (cp, num_symtab_entries);

	    symtab = js_calloc (vm, num_symtab_entries + 1, sizeof (*symtab));

	    /* Make the terminator by hand. */
	    symtab[num_symtab_entries].offset = code_len;

	    /* Enter symbols. */
	    se = symtab;
	    for (ui = 4; ui < bc->sects[sect].length; se++)
	      {
		for (i = 0; cp[ui]; ui++, i++)
		  buf[i] = cp[ui];
		buf[i] = '\0';

		se->name = js_strdup (vm, buf);
		ui++;

		JS_BC_READ_INT32 (cp + ui, se->offset);
		ui += 4;
	      }
	    break;
	  }

      /* Check if we have debugging information. */
      debug_info = NULL;
      debug_info_len = 0;
      for (sect = 0; sect < bc->num_sects; sect++)
	if (bc->sects[sect].type == JS_BCST_DEBUG)
	  {
	    debug_info = bc->sects[sect].data;
	    debug_info_len = bc->sects[sect].length;
	  }

      /* Clear error message and old exec result. */
      vm->error[0] = '\0';
      vm->exec_result.type = JS_UNDEFINED;

      PROFILING_ON ();

      /* Execute. */
      result = (*vm->dispatch_execute) (vm, bc, symtab, num_symtab_entries,
					consts_offset,
					anonymous_function_offset,
					debug_info, debug_info_len,
					NULL, NULL, 0, NULL);
    }

  PROFILING_OFF ();

  if (symtab)
    {
      for (ui = 0; ui < num_symtab_entries; ui++)
	js_free (symtab[ui].name);
      js_free (symtab);
    }

  /* Pop all error handler frames from the handler chain. */
  for (; vm->error_handler != saved_handler; vm->error_handler = handler)
    {
      handler = vm->error_handler->next;
      js_free (vm->error_handler);
    }

  /* Restore virtual machine's idea about the stack top. */
  vm->sp = saved_sp;

  return result;
}


int
js_vm_apply (JSVirtualMachine *vm, char *func_name, JSNode *func,
	     unsigned int argc, JSNode *argv)
{
  int result = 1;
  JSNode *saved_sp;
  JSErrorHandlerFrame *handler, *saved_handler;

  /* Initialize error handler. */

  saved_sp = vm->sp;
  saved_handler = vm->error_handler;

  handler = js_calloc (NULL, 1, sizeof (*handler));
  if (handler == NULL)
    {
      sprintf (vm->error, "VM: out of memory");
      return 0;
    }
  handler->next = vm->error_handler;
  vm->error_handler = handler;

  if (setjmp (vm->error_handler->error_jmp))
    {
      /* An error occurred. */
      result = 0;
    }
  else
    {
      /* Clear error message and old exec result. */
      vm->error[0] = '\0';
      vm->exec_result.type = JS_UNDEFINED;

      if (func_name)
	/* Lookup the function. */
	func = &vm->globals[js_vm_intern (vm, func_name)];

      /* Check what kind of function should be called. */
      if (func->type == JS_FUNC)
	{
	  PROFILING_ON ();

	  /* Call function. */
	  result = (*vm->dispatch_execute) (vm, NULL, NULL, 0, 0, 0,
					    NULL, 0,
					    NULL, func, argc, argv);
	}
      else if (func->type == JS_BUILTIN
	       && func->u.vbuiltin->info->global_method_proc != NULL)
	{
	  (*func->u.vbuiltin->info->global_method_proc) (
					vm,
					func->u.vbuiltin->info,
					func->u.vbuiltin->instance_context,
					&vm->exec_result,
					argv);
	}
      else
	{
	  if (func_name)
	    sprintf (vm->error, "undefined function `%s' in apply",
		     func_name);
	  else
	    sprintf (vm->error, "undefiend function in apply");

	  result = 0;
	}
    }

  PROFILING_OFF ();

  /* Pop all error handler frames from the handler chain. */
  for (; vm->error_handler != saved_handler; vm->error_handler = handler)
    {
      handler = vm->error_handler->next;
      js_free (vm->error_handler);
    }

  /* Restore virtual machine's idea about the stack top. */
  vm->sp = saved_sp;

  return result;
}


int
js_vm_call_method (JSVirtualMachine *vm, JSNode *object,
		   const char *method_name, unsigned int argc, JSNode *argv)
{
  int result = 1;
  JSNode *saved_sp;
  JSErrorHandlerFrame *handler, *saved_handler;
  JSSymbol symbol;

  /* Initialize error handler. */

  saved_sp = vm->sp;
  saved_handler = vm->error_handler;

  handler = js_calloc (NULL, 1, sizeof (*handler));
  if (handler == NULL)
    {
      sprintf (vm->error, "VM: out of memory");
      return 0;
    }
  handler->next = vm->error_handler;
  vm->error_handler = handler;

  if (setjmp (vm->error_handler->error_jmp))
    {
      /* An error occurred. */
      result = 0;
    }
  else
    {
      /* Intern the method name. */
      symbol = js_vm_intern (vm, method_name);

      /* Clear error message and old exec result. */
      vm->error[0] = '\0';
      vm->exec_result.type = JS_UNDEFINED;

      /* What kind of object was called? */

      if (object->type == JS_BUILTIN)
	{
	  if (object->u.vbuiltin->info->method_proc)
	    {
	      if ((*object->u.vbuiltin->info->method_proc) (
					vm,
					object->u.vbuiltin->info,
					object->u.vbuiltin->instance_context,
					symbol,
					&vm->exec_result, argv)
		  == JS_PROPERTY_UNKNOWN)
		{
		  sprintf (vm->error, "call_method: unknown method");
		  result = 0;
		}
	    }
	  else
	    {
	      sprintf (vm->error, "illegal builtin object for call_method");
	      result = 0;
	    }
	}
      else if (object->type == JS_OBJECT)
	{
	  JSNode method;

	  /* Fetch the method's implementation. */
	  if (js_vm_object_load_property (vm, object->u.vobject, symbol,
					  &method)
	      == JS_PROPERTY_FOUND)
	    {
	      /* The property has been defined in the object. */
	      if (method.type != JS_FUNC)
		{
		  sprintf (vm->error, "call_method: unknown method");
		  result = 0;
		}
	      else
		{
		  PROFILING_ON ();
		  result = (*vm->dispatch_execute) (vm, NULL, NULL, 0, 0, 0,
						    NULL, 0,
						    object, &method, argc,
						    argv);
		}
	    }
	  else
	    /* Let the built-in Object handle this. */
	    goto to_builtin_please;
	}
      else if (vm->prim[object->type])
	{
	  /* The primitive language types. */
	to_builtin_please:
	  if ((*vm->prim[object->type]->method_proc) (vm,
						      vm->prim[object->type],
						      object, symbol,
						      &vm->exec_result,
						      argv)
	      == JS_PROPERTY_UNKNOWN)
	    {
	      sprintf (vm->error, "call_method: unknown method");
	      result = 0;
	    }
	}
      else
	{
	  sprintf (vm->error, "illegal object for call_method");
	  result = 0;
	}
    }

  PROFILING_OFF ();

  /* Pop all error handler frames from the handler chain. */
  for (; vm->error_handler != saved_handler; vm->error_handler = handler)
    {
      handler = vm->error_handler->next;
      js_free (vm->error_handler);
    }

  /* Restore virtual machine's idea about the stack top. */
  vm->sp = saved_sp;

  return result;
}


const char *
js_vm_func_name (JSVirtualMachine *vm, void *pc)
{
  return (*vm->dispatch_func_name) (vm, pc);
}


const char *
js_vm_debug_position (JSVirtualMachine *vm, unsigned int *linenum_return)
{
  return (*vm->dispatch_debug_position) (vm, linenum_return);
}


unsigned int
js_vm_intern_with_len (JSVirtualMachine *vm, const char *name,
		       unsigned int len)
{
  JSHashBucket *b;
  unsigned int pos = js_count_hash (name, len) % JS_HASH_TABLE_SIZE;

  for (b = vm->globals_hash[pos]; b; b = b->next)
    if (strcmp (b->name, name) == 0)
      return b->u.ui;

  b = js_malloc (vm, sizeof (*b));
  b->name = js_strdup (vm, name);

  b->next = vm->globals_hash[pos];
  vm->globals_hash[pos] = b;

  /* Alloc space from the globals array. */
  if (vm->num_globals >= vm->globals_alloc)
    {
      vm->globals = js_realloc (vm, vm->globals,
				(vm->globals_alloc + 1024) * sizeof (JSNode));
      vm->globals_alloc += 1024;
    }

  /* Initialize symbol's name spaces. */
  vm->globals[vm->num_globals].type = JS_UNDEFINED;
  b->u.ui = vm->num_globals++;

  return b->u.ui;
}


const char *
js_vm_symname (JSVirtualMachine *vm, JSSymbol sym)
{
  int i;
  JSHashBucket *b;

  for (i = 0; i < JS_HASH_TABLE_SIZE; i++)
    for (b = vm->globals_hash[i]; b; b = b->next)
      if (b->u.ui == sym)
	return b->name;

  return "???";
}


void
js_vm_error (JSVirtualMachine *vm)
{
  const char *file;
  unsigned int ln;
  char error[1024];

  file = js_vm_debug_position (vm, &ln);
  if (file)
    {
      sprintf (error, "%s:%u: %s", file, ln, vm->error);
      strcpy (vm->error, error);
    }

  if (vm->stacktrace_on_error)
    {
      sprintf (error, "VM: error: %s%s", vm->error,
	       JS_HOST_LINE_BREAK);
      js_iostream_write (vm->s_stderr, error, strlen (error));

      js_vm_stacktrace (vm, (unsigned int) -1);
    }

  if (vm->error_handler->sp)
    /*
     * We are jumping to a catch-block.  Format our error message to
     * the `thrown' node.
     */
    js_vm_make_string (vm, &vm->error_handler->thrown,
		       vm->error, strlen (vm->error));

  longjmp (vm->error_handler->error_jmp, 1);

  /* NOTREACHED (I hope). */

  sprintf (error, "VM: no valid error handler initialized%s",
	   JS_HOST_LINE_BREAK);
  js_iostream_write (vm->s_stderr, error, strlen (error));
  js_iostream_flush (vm->s_stderr);

  abort ();
}

/* Delete proc for garbaged built-in objects. */
static void
destroy_builtin (void *ptr)
{
  JSBuiltin *bi = ptr;

  if (bi->info->delete_proc)
    (*bi->info->delete_proc) (bi->info, bi->instance_context);
}

/* Delete proc for garbaged built-in info. */
static void
destroy_builtin_info (void *ptr)
{
  JSBuiltinInfo *i = ptr;

  if (i->obj_context_delete)
    (*i->obj_context_delete) (i->obj_context);
}


JSBuiltinInfo *
js_vm_builtin_info_create (JSVirtualMachine *vm)
{
  JSNode prototype;
  JSBuiltinInfo *i = js_vm_alloc_destroyable (vm, sizeof (*i));

  i->destroy = destroy_builtin_info;
  i->prototype = js_vm_object_new (vm);

  /*
   * Set the __proto__ property to null.  We have no prototype object
   * above us.
   */
  prototype.type = JS_NULL;
  js_vm_object_store_property (vm, i->prototype, vm->syms.s___proto__,
			       &prototype);

  return i;
}


void
js_vm_builtin_create (JSVirtualMachine *vm, JSNode *result,
		      JSBuiltinInfo *info, void *instance_context)
{
  result->type = JS_BUILTIN;
  result->u.vbuiltin = js_vm_alloc_destroyable (vm, sizeof (JSBuiltin));
  result->u.vbuiltin->destroy = destroy_builtin;
  result->u.vbuiltin->info = info;

  if (instance_context)
    {
      JSNode prototype;

      result->u.vbuiltin->instance_context = instance_context;
      result->u.vbuiltin->prototype = js_vm_object_new (vm);

      /* Set the __proto__ chain. */

      prototype.type = JS_OBJECT;
      prototype.u.vobject = info->prototype;

      js_vm_object_store_property (vm, result->u.vbuiltin->prototype,
				   vm->syms.s___proto__, &prototype);
    }
}


/*
 * Static functions.
 */

extern void js_builtin_core (JSVirtualMachine *vm);

extern void js_builtin_Array (JSVirtualMachine *vm);
extern void js_builtin_Boolean (JSVirtualMachine *vm);
extern void js_builtin_Function (JSVirtualMachine *vm);
extern void js_builtin_Number (JSVirtualMachine *vm);
extern void js_builtin_Object (JSVirtualMachine *vm);
extern void js_builtin_String (JSVirtualMachine *vm);

extern void js_builtin_Date (JSVirtualMachine *vm);
extern void js_builtin_Directory (JSVirtualMachine *vm);
extern void js_builtin_File (JSVirtualMachine *vm);
extern void js_builtin_Math (JSVirtualMachine *vm);
extern void js_builtin_RegExp (JSVirtualMachine *vm);
extern void js_builtin_System (JSVirtualMachine *vm);
extern void js_builtin_VM (JSVirtualMachine *vm);


static void
intern_builtins (JSVirtualMachine *vm)
{
  /*
   * The initialization order is significant.  The RegExp object must be
   * initialized before String.
   */

  /* The core global methods. */
  js_builtin_core (vm);

  /* Our builtin extensions. */
  js_builtin_Date (vm);
  js_builtin_Directory (vm);
  js_builtin_File (vm);
  js_builtin_Math (vm);
  js_builtin_RegExp (vm);
  js_builtin_System (vm);
  js_builtin_VM (vm);

  /* Language objects. */
  js_builtin_Array (vm);
  js_builtin_Boolean (vm);
  js_builtin_Function (vm);
  js_builtin_Number (vm);
  js_builtin_Object (vm);
  js_builtin_String (vm);
}
