/*
 * JavaScript interpreter main glue.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/src/js.c,v $
 * $Id: js.c,v 1.1 2004/01/10 20:38:18 arty Exp $
 */

#include "js.h"
#include "jsint.h"

/*
 * Types and definitions.
 */

/* Context for js_global_method_stub. */
struct js_global_method_context_st
{
  JSGlobalMethodProc proc;
  void *context;
  JSFreeProc free_proc;
  JSInterpPtr interp;
};

typedef struct js_global_method_context_st JSGlobalMethodContext;

/* Context for user I/O function streams. */
struct js_user_io_func_ctx_st
{
  JSIOFunc func;
  void *context;
  long position;
};

typedef struct js_user_io_func_ctx_st JSUserIOFuncCtx;

struct js_method_reg_st
{
  JSSymbol sym;
  char *name;
  unsigned int flags;
  JSMethodProc method;
};

typedef struct js_method_reg_st JSMethodReg;

struct js_property_reg_st
{
  JSSymbol sym;
  char *name;
  unsigned int flags;
  JSPropertyProc property;
};

typedef struct js_property_reg_st JSPropertyReg;

/* The class handle. */
struct js_class_st
{
  char *name;
  JSInterpPtr interp;

  /* Flags. */
  unsigned int no_auto_destroy : 1;
  unsigned int interned : 1;

  void *class_context;
  JSFreeProc class_context_destructor;

  JSConstructor constructor;

  unsigned int num_methods;
  JSMethodReg *methods;

  unsigned int num_properties;
  JSPropertyReg *properties;
};

/* Object instance context. */
struct js_object_instance_ctx_st
{
  void *instance_context;
  JSFreeProc instance_context_destructor;
};

typedef struct js_object_instance_ctx_st JSObjectInstanceCtx;


/*
 * Prototypes for static functions.
 */

/* The module for JS' core global methods. */
static void js_core_globals (JSInterpPtr interp);

/*
 * Helper function to evaluate source <source> with compiler function
 * <compiler_function>.
 */
static int js_eval_source (JSInterpPtr interp, JSNode *source,
			   char *compiler_function);

/*
 * Helper function to compile source <source> with compiler function
 * <compiler_function>.  If <assembler_file> is not NULL, the
 * assembler listing of the compilation is saved to that file.  If
 * <byte_code_file> is not NULL, the byte_code data is saved to that
 * file.  If <bc_return> is not NULL, the resulting byte_code data is
 * returned in it as a JavaScript string node.
 */
static int js_compile_source (JSInterpPtr interp, JSNode *source,
			      char *compiler_function, char *assembler_file,
			      char *byte_code_file, JSNode *bc_return);

/*
 * The stub function for global methods, created with the
 * js_create_global_method() API function.
 */
static void js_global_method_stub (JSVirtualMachine *vm,
				   JSBuiltinInfo *builtin_info,
				   void *instance_context,
				   JSNode *result_return,
				   JSNode *args);

/*
 * Destructor for the global methods, created with the
 * js_create_global_method() API function.
 */
static void js_global_method_delete (JSBuiltinInfo *builtin_info,
				     void *instance_context);

static JSIOStream *iostream_iofunc (JSIOFunc func, void *context,
				    int readp, int writep);


/*
 * Global functions.
 */

const JSCharPtr
js_version ()
{
  return VERSION;
}


void
js_init_default_options (JSInterpOptions *options)
{
  memset (options, 0, sizeof (*options));

  options->stack_size = 2048;
  options->dispatch_method = JS_VM_DISPATCH_JUMPS;

  options->warn_undef = 1;

  options->optimize_peephole = 1;
  options->optimize_jumps_to_jumps = 1;

  options->fd_count = (unsigned long) -1;
}


JSInterpPtr
js_create_interp (JSInterpOptions *options)
{
  JSInterpPtr interp = NULL;
  JSByteCode *bc;
  JSInterpOptions default_options;
  JSIOStream *s_stdin = NULL;
  JSIOStream *s_stdout = NULL;
  JSIOStream *s_stderr = NULL;

  /*
   * Sanity check to assure that the js.h and jsint.h APIs share a
   * same view to the world.
   */
  assert (sizeof (JSNode) == sizeof (JSType));

  interp = js_calloc (NULL, 1, sizeof (*interp));
  if (interp == NULL)
    return NULL;

  if (options == NULL)
    {
      js_init_default_options (&default_options);
      options = &default_options;
    }

  memcpy (&interp->options, options, sizeof (*options));

  /* The default system streams. */

  if (options->s_stdin)
    s_stdin = iostream_iofunc (options->s_stdin, options->s_context, 1, 0);
  else
    s_stdin = js_iostream_file (stdin, 1, 0, 0);

  if (s_stdin == NULL)
    goto error_out;

  if (options->s_stdout)
    s_stdout = iostream_iofunc (options->s_stdout, options->s_context, 0, 1);
  else
    s_stdout = js_iostream_file (stdout, 0, 1, 0);

  if (s_stdout == NULL)
    goto error_out;
  s_stdout->autoflush = 1;

  if (options->s_stderr)
    s_stderr = iostream_iofunc (options->s_stderr, options->s_context, 0, 1);
  else
    s_stderr = js_iostream_file (stderr, 0, 1, 0);

  if (s_stderr == NULL)
    goto error_out;
  s_stderr->autoflush = 1;

  /* Create virtual machine. */
  interp->vm = js_vm_create (options->stack_size,
			     options->dispatch_method,
			     options->verbose,
			     options->stacktrace_on_error,
			     s_stdin, s_stdout, s_stderr);
  if (interp->vm == NULL)
    goto error_out;

  /* Set some options. */
  interp->vm->warn_undef = options->warn_undef;

  /* Set the security options. */

  if (options->secure_builtin_file)
    interp->vm->security |= JS_VM_SECURE_FILE;
  if (options->secure_builtin_system)
    interp->vm->security |= JS_VM_SECURE_SYSTEM;

  /* Set the event hook. */
  interp->vm->hook			 = options->hook;
  interp->vm->hook_context		 = options->hook_context;
  interp->vm->hook_operand_count_trigger = options->hook_operand_count_trigger;

  /* The file descriptor limit. */
  interp->vm->fd_count = options->fd_count;

  if (!options->no_compiler)
    {
      int result;

      /* Define compiler to the virtual machine. */
      bc = js_bc_read_data (js_compiler_bytecode, js_compiler_bytecode_len);
      if (bc == NULL)
	goto error_out;

      result = js_vm_execute (interp->vm, bc);
      js_bc_free (bc);
      if (!result)
	goto error_out;
    }

  /* Initialize our extensions. */
  if (!js_define_module (interp, js_core_globals))
    goto error_out;

  /* Ok, we'r done. */

#if 0
#if JS_DEBUG_MEMORY_LEAKS
  /* Let's see how much memory an empty interpreter takes. */
  js_alloc_dump_blocks ();
#endif /* JS_DEBUG_MEMORY_LEAKS */
#endif

  return interp;


  /*
   * Error handling.
   */

 error_out:

  if (interp)
    {
      if (interp->vm)
	js_vm_destroy (interp->vm);
      js_free (interp);
    }

  if (s_stdin)
    js_iostream_close (s_stdin);
  if (s_stdout)
    js_iostream_close (s_stdout);
  if (s_stderr)
    js_iostream_close (s_stderr);

  return NULL;
}


void
js_destroy_interp (JSInterpPtr interp)
{
  js_vm_destroy (interp->vm);
  js_free (interp);

#if 0
#if JS_DEBUG_MEMORY_LEAKS
  /* Let's see how much memory we leak. */
  js_alloc_dump_blocks ();
#endif /* JS_DEBUG_MEMORY_LEAKS */
#endif
}


const JSCharPtr
js_error_message (JSInterpPtr interp)
{
  return interp->vm->error;
}


void
js_result (JSInterpPtr interp, JSType *result_return)
{
  memcpy (result_return, &interp->vm->exec_result, sizeof (*result_return));
}


int
js_eval (JSInterpPtr interp, char *code)
{
  JSNode source;

  js_vm_make_static_string (interp->vm, &source, code, strlen (code));
  return js_eval_source (interp, &source, "JSC$compile_string");
}


int
js_eval_data (JSInterpPtr interp, char *data, unsigned int datalen)
{
  JSNode source;

  js_vm_make_static_string (interp->vm, &source, data, datalen);
  return js_eval_source (interp, &source, "JSC$compile_string");
}


int
js_eval_file (JSInterpPtr interp, char *filename)
{
  char *cp;
  int result;

  cp = strrchr (filename, '.');
  if (cp && strcmp (cp, ".jsc") == 0)
    {
    run_bytecode:
      result = js_execute_byte_code_file (interp, filename);
    }
  else if (cp && strcmp (cp, ".js") == 0)
    {
    try_javascript:
      result = js_eval_javascript_file (interp, filename);
    }
  else
    {
      FILE *fp;

      /* Must look into the file. */

      fp = fopen (filename, "r");
      if (fp)
	{
	  int ch;

	  if ((ch = getc (fp)) == '#')
	    {
	      /* Skip the first sh-command line. */
	      while ((ch = getc (fp)) != EOF && ch != '\n')
		;
	      if (ch == EOF)
		{
		  fclose (fp);
		  goto try_javascript;
		}
	    }
	  else
	    ungetc (ch, fp);

	  /* Check if we can read the file magic. */
	  ch = getc (fp);
	  if (ch == 0xc0)
	    {
	      ch = getc (fp);
	      if (ch == 0x01)
		{
		  ch = getc (fp);
		  if (ch == 'J')
		    {
		      ch = getc (fp);
		      if (ch == 'S')
			{
			  /* Got it.  We find a valid byte-code file magic. */
			  fclose (fp);
			  goto run_bytecode;
			}
		    }
		}
	    }

	  fclose (fp);
	  /* FALLTHROUGH */
	}

      /*
       * If nothing else helps, we assume that the file contains JavaScript
       * source code that must be compiled.
       */
      goto try_javascript;
    }

  return result;
}


int
js_eval_javascript_file (JSInterpPtr interp, char *filename)
{
  JSNode source;

  js_vm_make_static_string (interp->vm, &source, filename, strlen (filename));
  return js_eval_source (interp, &source, "JSC$compile_file");
}


int
js_execute_byte_code_file (JSInterpPtr interp, char *filename)
{
  JSByteCode *bc;
  FILE *fp;
  int result;

  fp = fopen (filename, "rb");
  if (fp == NULL)
    {
      /* Let's borrow vm's error buffer. */
      sprintf (interp->vm->error, "couldn't open byte-code file \"%s\": %s",
	       filename, strerror (errno));
      return 0;
    }

  bc = js_bc_read_file (fp);
  fclose (fp);

  if (bc == NULL)
    /* XXX Error message. */
    return 0;

  /* Execute it. */

  result = js_vm_execute (interp->vm, bc);
  js_bc_free (bc);

  return result;
}


int
js_apply (JSInterpPtr interp, char *name, unsigned int argc, JSType *argv)
{
  JSNode *args;
  unsigned int ui;
  int result;

  args = js_malloc (NULL, (argc + 1) * sizeof (JSNode));
  if (args == NULL)
    {
      sprintf (interp->vm->error, "VM: out of memory");
      return 0;
    }

  /* Set the argument count. */
  args[0].type = JS_INTEGER;
  args[0].u.vinteger = argc;

  /* Set the arguments. */
  for (ui = 0; ui < argc; ui++)
    JS_COPY (&args[ui + 1], (JSNode *) &argv[ui]);

  /* Call it. */
  result = js_vm_apply (interp->vm, name, NULL, argc + 1, args);

  js_free (args);

  return result;
}


int
js_compile (JSInterpPtr interp,  char *input_file, char *assembler_file,
	    char *byte_code_file)
{
  JSNode source;

  js_vm_make_static_string (interp->vm, &source, input_file,
			    strlen (input_file));
  return js_compile_source (interp, &source, "JSC$compile_file",
			    assembler_file, byte_code_file, NULL);
}


int
js_compile_to_byte_code (JSInterpPtr interp, char *input_file,
			 unsigned char **bc_return,
			 unsigned int *bc_len_return)
{
  JSNode source;
  int result;

  js_vm_make_static_string (interp->vm, &source, input_file,
			    strlen (input_file));
  result = js_compile_source (interp, &source, "JSC$compile_file",
			      NULL, NULL, &source);
  if (result == 0)
    return 0;

  /* Pass the data to the caller. */
  *bc_return = source.u.vstring->data;
  *bc_len_return = source.u.vstring->len;

  return result;
}


int
js_compile_data_to_byte_code (JSInterpPtr interp, char *data,
			      unsigned int datalen,
			      unsigned char **bc_return,
			      unsigned int *bc_len_return)
{
  JSNode source;
  int result;

  js_vm_make_static_string (interp->vm, &source, data, datalen);
  result = js_compile_source (interp, &source, "JSC$compile_string",
			      NULL, NULL, &source);
  if (result == 0)
    return 0;

  /* Pass the data to the caller. */
  *bc_return = source.u.vstring->data;
  *bc_len_return = source.u.vstring->len;

  return result;
}


int
js_execute_byte_code (JSInterpPtr interp, unsigned char *bc_data,
		      unsigned int bc_data_len)
{
  JSByteCode *bc;
  int result;

  bc = js_bc_read_data (bc_data, bc_data_len);
  if (bc == NULL)
    /* Not a valid byte-code data. */
    return 0;

  /* Execute it. */
  result = js_vm_execute (interp->vm, bc);
  js_bc_free (bc);

  return result;
}


/* Classes. */

JSClassPtr
js_class_create (void *class_context, JSFreeProc class_context_destructor,
		 int no_auto_destroy, JSConstructor constructor)
{
  JSClassPtr cls;

  cls = js_calloc (NULL, 1, sizeof (*cls));
  if (cls == NULL)
    return NULL;

  cls->class_context = class_context;
  cls->class_context_destructor = class_context_destructor;

  cls->no_auto_destroy = no_auto_destroy;
  cls->constructor = constructor;

  return cls;
}


void
js_class_destroy (JSClassPtr cls)
{
  if (cls == NULL)
    return;

  if (cls->class_context_destructor)
    (*cls->class_context_destructor) (cls->class_context);

  js_free (cls);
}


JSVoidPtr
js_class_context (JSClassPtr cls)
{
  if (cls)
    return cls->class_context;

  return NULL;
}


int
js_class_define_method (JSClassPtr cls, char *name, unsigned int flags,
			JSMethodProc method)
{
  JSMethodReg *nmethods;

  nmethods = js_realloc (NULL, cls->methods,
			 (cls->num_methods + 1) * sizeof (JSMethodReg));
  if (nmethods == NULL)
    return 0;

  cls->methods = nmethods;

  /*
   * The names are interned to symbols when the class is defined to the
   * interpreter.
   */

  cls->methods[cls->num_methods].name = js_strdup (NULL, name);
  if (cls->methods[cls->num_methods].name == NULL)
    return 0;

  cls->methods[cls->num_methods].flags = flags;
  cls->methods[cls->num_methods].method = method;

  cls->num_methods++;

  return 1;
}


int
js_class_define_property (JSClassPtr cls, char *name, unsigned int flags,
			  JSPropertyProc property)
{
  JSPropertyReg *nprops;

  nprops = js_realloc (NULL, cls->properties,
		       (cls->num_properties + 1) * sizeof (JSPropertyReg));
  if (nprops == NULL)
    return 0;

  cls->properties = nprops;

  cls->properties[cls->num_properties].name = js_strdup (NULL, name);
  if (cls->properties[cls->num_properties].name == NULL)
    return 0;

  cls->properties[cls->num_properties].flags = flags;
  cls->properties[cls->num_properties].property = property;

  cls->num_properties++;

  return 1;
}


/* The stub functions for JSClass built-in objects. */

/* Method proc. */
static int
cls_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	    void *instance_context, JSSymbol method, JSNode *result_return,
	    JSNode *args)
{
  JSClassPtr cls = builtin_info->obj_context;
  JSObjectInstanceCtx *ictx = instance_context;
  int i;
  JSMethodResult result;
  char msg[1024];

  /* Let's see if we know the method. */
  for (i = 0; i < cls->num_methods; i++)
    if (cls->methods[i].sym == method)
      {
	/* Found it. */

	/* Check flags. */
	if ((cls->methods[i].flags & JS_CF_STATIC) == 0
	    && instance_context == NULL)
	  /* An instance method called from the `main' class. */
	  break;

	result = (*cls->methods[i].method) (cls,
					    (ictx
					     ? ictx->instance_context
					     : NULL),
					    cls->interp, args[0].u.vinteger,
					    (JSType *) &args[1],
					    (JSType *) result_return,
					    msg);
	if (result == JS_ERROR)
	  {
	    sprintf (vm->error, "%s.%s(): %s", cls->name,
		     cls->methods[i].name, msg);
	    js_vm_error (vm);
	  }

	return JS_PROPERTY_FOUND;
      }

  return JS_PROPERTY_UNKNOWN;
}

/* Property proc. */
static int
cls_property (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
	      void *instance_context, JSSymbol property, int set, JSNode *node)
{
  JSClassPtr cls = builtin_info->obj_context;
  JSObjectInstanceCtx *ictx = instance_context;
  JSMethodResult result;
  char msg[1024];
  int i;

  /* Find the property. */
  for (i = 0; i < cls->num_properties; i++)
    if (cls->properties[i].sym == property)
      {
	/* Found it. */

	/* Check flags. */

	if ((cls->properties[i].flags & JS_CF_STATIC) == 0
	    && instance_context == NULL)
	  break;

	if ((cls->properties[i].flags & JS_CF_IMMUTABLE) && set)
	  {
	    sprintf (vm->error, "%s.%s: immutable property",
		     cls->name, cls->properties[i].name);
	    js_vm_error (vm);
	  }

	result = (*cls->properties[i].property) (cls,
						 (ictx
						  ? ictx->instance_context
						  : NULL),
						 cls->interp, set,
						 (JSType *) node, msg);
	if (result == JS_ERROR)
	  {
	    sprintf (vm->error, "%s.%s: %s", cls->name,
		     cls->properties[i].name, msg);
	    js_vm_error (vm);
	  }

	return JS_PROPERTY_FOUND;
      }

  if (!set)
    node->type = JS_UNDEFINED;

  return JS_PROPERTY_UNKNOWN;
}

/* New proc. */
static void
cls_new_proc (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info, JSNode *args,
	      JSNode *result_return)
{
  JSClassPtr cls = builtin_info->obj_context;
  JSMethodResult result;
  char msg[1024];
  void *instance_context;
  JSFreeProc instance_context_destructor;
  JSObjectInstanceCtx *ictx;

  result = (*cls->constructor) (cls, cls->interp, args[0].u.vinteger,
				(JSType *) &args[1], &instance_context,
				&instance_context_destructor,
				msg);
  if (result == JS_ERROR)
    {
      sprintf (vm->error, "new %s(): %s", cls->name, msg);
      js_vm_error (vm);
    }

  ictx = js_calloc (vm, 1, sizeof (*ictx));
  ictx->instance_context = instance_context;
  ictx->instance_context_destructor = instance_context_destructor;

  js_vm_builtin_create (vm, result_return, builtin_info, ictx);
}


/* Delete proc. */
static void
cls_delete_proc (JSBuiltinInfo *builtin_info, void *instance_context)
{
  JSObjectInstanceCtx *ictx = instance_context;

  if (ictx)
    {
      if (ictx->instance_context_destructor)
	(*ictx->instance_context_destructor) (ictx->instance_context);

      js_free (ictx);
    }
}

/*
 * This is called to destroy the class handle, when there are no more
 * references to it.
 */
static void
js_class_destructor (void *context)
{
  JSClassPtr cls = context;

  if (cls->no_auto_destroy)
    return;

  js_class_destroy (cls);
}


static void
intern_symbols (JSVirtualMachine *vm, JSClassPtr cls)
{
  int i;

  for (i = 0; i < cls->num_methods; i++)
    cls->methods[i].sym = js_vm_intern (vm, cls->methods[i].name);

  for (i = 0; i < cls->num_properties; i++)
    cls->properties[i].sym = js_vm_intern (vm, cls->properties[i].name);

  cls->interned = 1;
}


static JSBuiltinInfo *
one_builtin_info_please (JSVirtualMachine *vm, JSClassPtr cls)
{
  JSBuiltinInfo *info;

  info = js_vm_builtin_info_create (vm);

  info->method_proc		= cls_method;
  info->property_proc		= cls_property;

  if (cls->constructor)
    {
      info->new_proc 		= cls_new_proc;
      info->delete_proc		= cls_delete_proc;
    }

  info->obj_context		= cls;
  info->obj_context_delete	= js_class_destructor;

  return info;
}


int
js_define_class (JSInterpPtr interp, JSClassPtr cls, char *name)
{
  JSNode *n;
  JSVirtualMachine *vm = interp->vm;
  JSBuiltinInfo *info;

  /* XXX We need a top-level here */

  cls->name = js_strdup (vm, name);
  cls->interp = interp;

  if (!cls->interned)
    /* Intern the symbols and properties. */
    intern_symbols (interp->vm, cls);

  /* Define it to the interpreter. */

  info = one_builtin_info_please (vm, cls);

  n = &vm->globals[js_vm_intern (vm, name)];
  js_vm_builtin_create (vm, n, info, NULL);

  return 1;
}


int
js_instantiate_class (JSInterpPtr interp, JSClassPtr cls, void *ictx,
		      JSFreeProc ictx_destructor, JSType *result_return)
{
  JSObjectInstanceCtx *instance;
  JSVirtualMachine *vm = interp->vm;
  JSBuiltinInfo *info;

  if (!cls->interned)
    /* Intern the symbols and properties. */
    intern_symbols (vm, cls);

  /* Create an instance. */
  instance = js_calloc (vm, 1, sizeof (*instance));
  instance->instance_context = ictx;
  instance->instance_context_destructor = ictx_destructor;

  /* Create a fresh builtin info. */
  info = one_builtin_info_please (vm, cls);

  /* And create it.  */
  js_vm_builtin_create (vm, (JSNode *) result_return, info, instance);

  return 1;
}


const JSClassPtr
js_lookup_class (JSInterpPtr interp, char *name)
{
  JSNode *n;
  JSVirtualMachine *vm = interp->vm;

  n = &vm->globals[js_vm_intern (vm, name)];
  if (n->type != JS_BUILTIN)
    return NULL;

  if (n->u.vbuiltin->info->method_proc != cls_method)
    /* This is a wrong built-in. */
    return NULL;

  return (JSClassPtr) n->u.vbuiltin->info->obj_context;
}


int
js_isa (JSInterpPtr interp, JSType *object, JSClassPtr cls,
	void **instance_context_return)
{
  JSNode *n = (JSNode *) object;
  JSObjectInstanceCtx *instance;

  if (n->type != JS_BUILTIN || n->u.vbuiltin->info->obj_context != cls
      || n->u.vbuiltin->instance_context == NULL)
    return 0;

  if (instance_context_return)
    {
      instance = (JSObjectInstanceCtx *) n->u.vbuiltin->instance_context;
      *instance_context_return = instance->instance_context;
    }

  return 1;
}



/* Type functions. */

void
js_type_make_string (JSInterpPtr interp, JSType *type, unsigned char *data,
		     unsigned int length)
{
  JSNode *n = (JSNode *) type;

  js_vm_make_string (interp->vm, n, data, length);
}


void
js_type_make_array (JSInterpPtr interp, JSType *type, unsigned int length)
{
  JSNode *n = (JSNode *) type;

  js_vm_make_array (interp->vm, n, length);
}


void
js_set_var (JSInterpPtr interp, char *name, JSType *value)
{
  JSNode *n = &interp->vm->globals[js_vm_intern (interp->vm, name)];
  JS_COPY (n, (JSNode *) value);
}


void
js_get_var (JSInterpPtr interp, char *name, JSType *value)
{
  JSNode *n = &interp->vm->globals[js_vm_intern (interp->vm, name)];
  JS_COPY ((JSNode *) value, n);
}


void
js_get_options (JSInterpPtr interp, JSInterpOptions *options)
{
  memcpy (options, &interp->options, sizeof (*options));
}


void
js_set_options (JSInterpPtr interp, JSInterpOptions *options)
{
  memcpy (&interp->options, options, sizeof (*options));

  /* User can change the security options, */

  if (interp->options.secure_builtin_file)
    interp->vm->security |= JS_VM_SECURE_FILE;
  else
    interp->vm->security &= ~JS_VM_SECURE_FILE;

  if (interp->options.secure_builtin_system)
    interp->vm->security |= JS_VM_SECURE_SYSTEM;
  else
    interp->vm->security &= ~JS_VM_SECURE_SYSTEM;

  /* and the event hook. */
  interp->vm->hook			 = options->hook;
  interp->vm->hook_context		 = options->hook_context;
  interp->vm->hook_operand_count_trigger = options->hook_operand_count_trigger;
}


int
js_create_global_method (JSInterpPtr interp, char *name,
			 JSGlobalMethodProc proc, void *context,
			 JSFreeProc context_free_proc)
{
  JSNode *n = &interp->vm->globals[js_vm_intern (interp->vm, name)];
  JSVirtualMachine *vm = interp->vm;
  int result = 1;

  /* Need one toplevel here. */
  {
    JSErrorHandlerFrame handler;

    /* We must create the toplevel ourself. */
    memset (&handler, 0, sizeof (handler));
    handler.next = vm->error_handler;
    vm->error_handler = &handler;

    if (setjmp (vm->error_handler->error_jmp))
      /* An error occurred. */
      result = 0;
    else
      {
	JSBuiltinInfo *info;
	JSGlobalMethodContext *ctx;

	/* Context. */
	ctx = js_calloc (vm, 1, sizeof (*ctx));

	ctx->proc = proc;
	ctx->context = context;
	ctx->free_proc = context_free_proc;
	ctx->interp = interp;

	/* Info. */
	info = js_vm_builtin_info_create (vm);
	info->global_method_proc = js_global_method_stub;
	info->delete_proc = js_global_method_delete;

	/* Create the builtin. */
	js_vm_builtin_create (interp->vm, n, info, ctx);
      }

    /* Pop the error handler. */
    vm->error_handler = vm->error_handler->next;
  }

  return result;
}


int
js_define_module (JSInterpPtr interp, JSModuleInitProc init_proc)
{
  JSErrorHandlerFrame handler;
  JSVirtualMachine *vm = interp->vm;
  int result = 1;

  /* Just call the init proc in a toplevel. */

  memset (&handler, 0, sizeof (handler));
  handler.next = vm->error_handler;
  vm->error_handler = &handler;

  if (setjmp (vm->error_handler->error_jmp))
    /* An error occurred. */
    result = 0;
  else
    /* Call the module init proc. */
    (*init_proc) (interp);

  /* Pop the error handler. */
  vm->error_handler = vm->error_handler->next;

  return result;
}



/*
 * Static functions.
 */

static int
js_eval_source (JSInterpPtr interp, JSNode *source, char *compiler_function)
{
  JSNode argv[5];
  int i = 0;
  int result;
  JSByteCode *bc;

  /* Let's compile the code. */

  /* Argument count. */
  argv[i].type = JS_INTEGER;
  argv[i].u.vinteger = 4;
  i++;

  /* Source to compiler. */
  JS_COPY (&argv[i], source);
  i++;

  /* Flags. */
  argv[i].type = JS_INTEGER;
  argv[i].u.vinteger = 0;

  if (interp->options.verbose)
    argv[i].u.vinteger = JSC_FLAG_VERBOSE;

  argv[i].u.vinteger |= JSC_FLAG_GENERATE_DEBUG_INFO;

  argv[i].u.vinteger |= JSC_FLAG_OPTIMIZE_PEEPHOLE;
  argv[i].u.vinteger |= JSC_FLAG_OPTIMIZE_JUMPS;
  argv[i].u.vinteger |= JSC_FLAG_WARN_WITH_CLOBBER;
  i++;

  /* Assembler file. */
  argv[i].type = JS_NULL;
  i++;

  /* Byte-code file. */
  argv[i].type = JS_NULL;
  i++;

  /* Call the compiler entry point. */
  result = js_vm_apply (interp->vm, compiler_function, NULL, i, argv);
  if (result == 0)
    return 0;

  /*
   * The resulting byte-code file is now at vm->exec_result.
   *
   * Note!  The byte-code is a string allocated form the vm heap.
   * The garbage collector can free it when it wants since the result
   * isn't protected.  However, we have no risk here because we
   * first convert the byte-code data block to our internal
   * JSByteCode block that shares no memory with the original data.
   */

  assert (interp->vm->exec_result.type == JS_STRING);

  bc = js_bc_read_data (interp->vm->exec_result.u.vstring->data,
			interp->vm->exec_result.u.vstring->len);

  /* And finally, execute it. */
  result = js_vm_execute (interp->vm, bc);

  /* Free the byte-code. */
  js_bc_free (bc);

  return result;
}


static int
js_compile_source (JSInterpPtr interp,  JSNode *source,
		   char *compiler_function, char *assembler_file,
		   char *byte_code_file, JSNode *bc_return)
{
  JSNode argv[5];
  int i = 0;
  int result;

  /* Init arguments. */

  argv[i].type = JS_INTEGER;
  argv[i].u.vinteger = 4;
  i++;

  /* Source to compiler. */
  JS_COPY (&argv[1], source);
  i++;

  /* Flags. */
  argv[i].type = JS_INTEGER;
  argv[i].u.vinteger = 0;

  if (interp->options.verbose)
    argv[i].u.vinteger |= JSC_FLAG_VERBOSE;
  if (interp->options.annotate_assembler)
    argv[i].u.vinteger |= JSC_FLAG_ANNOTATE_ASSEMBLER;
  if (interp->options.debug_info)
    argv[i].u.vinteger |= JSC_FLAG_GENERATE_DEBUG_INFO;
  if (interp->options.executable_bc_files)
    argv[i].u.vinteger |= JSC_FLAG_GENERATE_EXECUTABLE_BC_FILES;

  if (interp->options.warn_unused_argument)
    argv[i].u.vinteger |= JSC_FLAG_WARN_UNUSED_ARGUMENT;
  if (interp->options.warn_unused_variable)
    argv[i].u.vinteger |= JSC_FLAG_WARN_UNUSED_VARIABLE;
  if (interp->options.warn_shadow)
    argv[i].u.vinteger |= JSC_FLAG_WARN_SHADOW;
  if (interp->options.warn_with_clobber)
    argv[i].u.vinteger |= JSC_FLAG_WARN_WITH_CLOBBER;
  if (interp->options.warn_missing_semicolon)
    argv[i].u.vinteger |= JSC_FLAG_WARN_MISSING_SEMICOLON;
  if (interp->options.warn_strict_ecma)
    argv[i].u.vinteger |= JSC_FLAG_WARN_STRICT_ECMA;
  if (interp->options.warn_deprecated)
    argv[i].u.vinteger |= JSC_FLAG_WARN_DEPRECATED;

  if (interp->options.optimize_peephole)
    argv[i].u.vinteger |= JSC_FLAG_OPTIMIZE_PEEPHOLE;
  if (interp->options.optimize_jumps_to_jumps)
    argv[i].u.vinteger |= JSC_FLAG_OPTIMIZE_JUMPS;
  if (interp->options.optimize_bc_size)
    argv[i].u.vinteger |= JSC_FLAG_OPTIMIZE_BC_SIZE;
  if (interp->options.optimize_heavy)
    argv[i].u.vinteger |= JSC_FLAG_OPTIMIZE_HEAVY;

  i++;

  /* Assembler file. */
  if (assembler_file)
    js_vm_make_static_string (interp->vm, &argv[i], assembler_file,
			      strlen (assembler_file));
  else
    argv[i].type = JS_NULL;
  i++;

  /* Byte-code file. */
  if (byte_code_file)
    js_vm_make_static_string (interp->vm, &argv[i], byte_code_file,
			      strlen (byte_code_file));
  else
    argv[i].type = JS_NULL;
  i++;

  /* Call the compiler entry point. */
  result = js_vm_apply (interp->vm, compiler_function, NULL, i, argv);
  if (result == 0)
    return 0;

  if (bc_return)
    /* User wanted to get the resulting byte-code data.  Here it is. */
    JS_COPY (bc_return, &interp->vm->exec_result);

  return result;
}


/*
 * Global methods.
 */

static void
eval_global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
		    void *instance_context, JSNode *result_return,
		    JSNode *args)
{
  JSInterpPtr interp = instance_context;

  if (args->u.vinteger != 1)
    {
      sprintf (vm->error, "eval(): illegal amount of arguments");
      js_vm_error (vm);
    }
  if (args[1].type != JS_STRING)
    {
      /* Return it to the caller. */
      JS_COPY (result_return, &args[1]);
      return;
    }

  /*
   * Ok, we'r ready to eval it.  The source strings is our argument, so,
   * it is in the stack and therefore, protected for gc.
   */
  if (!js_eval_source (interp, &args[1], "JSC$compile_string"))
    {
      /* The evaluation failed.  Throw it as an error to our caller. */
      js_vm_error (vm);
    }

  /* Pass the return value to our caller. */
  JS_COPY (result_return, &vm->exec_result);
}


static void
load_global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
		    void *instance_context,
		    JSNode *result_return, JSNode *args)
{
  JSInterpPtr interp = instance_context;
  int i;
  int result;

  if (args->u.vinteger == 0)
    {
      sprintf (vm->error, "load(): no arguments given");
      js_vm_error (vm);
    }

  for (i = 1; i <= args->u.vinteger; i++)
    {
      char *cp;

      if (args[i].type != JS_STRING)
	{
	  sprintf (vm->error, "load(): illegal argument");
	  js_vm_error (vm);
	}

      cp = js_string_to_c_string (vm, &args[i]);
      result = js_eval_file (interp, cp);
      js_free (cp);

      if (!result)
	js_vm_error (vm);
    }

  result_return->type = JS_BOOLEAN;
  result_return->u.vboolean = 1;
}


static void
load_class_global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
			  void *instance_context,
			  JSNode *result_return, JSNode *args)
{
  JSInterpPtr interp = instance_context;
  int i;

  if (args->u.vinteger == 0)
    {
      sprintf (vm->error, "loadClass(): no arguments given");
      js_vm_error (vm);
    }

  for (i = 1; i <= args->u.vinteger; i++)
    {
      char *cp, *cp2;
      void *lib;
      void (*func) (JSInterpPtr interp);
      char *func_name;
      char buf[512];

      if (args[i].type != JS_STRING)
	{
	  sprintf (vm->error, "loadClass(): illegal argument");
	  js_vm_error (vm);
	}

      cp = js_string_to_c_string (vm, &args[i]);

      /* Extract the function name. */
      func_name = strrchr (cp, ':');
      if (func_name == NULL)
	{
	  func_name = strrchr (cp, '/');
	  if (func_name == NULL)
	    func_name = cp;
	  else
	    func_name++;
	}
      else
	{
	  *func_name = '\0';
	  func_name++;
	}

      /* Try to open the library. */
      lib = js_dl_open (cp, buf, sizeof (buf));
      if (lib == NULL)
	{
	  sprintf (vm->error, "loadClass(): couldn't open library `%s': %s",
		   cp, buf);
	  js_vm_error (vm);
	}

      /*
       * Strip all suffixes from the library name: if the <func_name>
       * is extracted from it, this will convert the library name
       * `foo.so.x.y' to the canonical entry point name `foo'.
       */
      cp2 = strchr (cp, '.');
      if (cp2)
	*cp2 = '\0';

      func = js_dl_sym (lib, func_name, buf, sizeof (buf));
      if (func == NULL)
	{
	  sprintf (vm->error,
		   "loadClass(): couldn't find the init function `%s': %s",
		   func_name, buf);
	  js_vm_error (vm);
	}

      /* All done with this argument. */
      js_free (cp);

      /*
       * And finally, call the library entry point.  All possible errors
       * will throw us to the containing top-level.
       */
      (*func) (interp);
    }

  result_return->type = JS_UNDEFINED;
}


static void
call_method_global_method (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
			   void *instance_context,
			   JSNode *result_return, JSNode *args)
{
  JSInterpPtr interp = instance_context;
  JSNode *argv;
  int i;
  int result;
  char *cp;

  if (args->u.vinteger != 3)
    {
      sprintf (vm->error, "callMethod(): illegal amount of arguments");
      js_vm_error (vm);
    }
  if (args[2].type != JS_STRING)
    {
    illegal_argument:
      sprintf (vm->error, "callMethod(): illegal argument");
      js_vm_error (vm);
    }
  if (args[3].type != JS_ARRAY)
    goto illegal_argument;

  /* Create the argument array. */
  argv = js_malloc (vm, (args[3].u.varray->length + 1) * sizeof (JSNode));

  /* The argument count. */
  argv[0].type = JS_INTEGER;
  argv[0].u.vinteger = args[3].u.varray->length;

  for (i = 0; i < args[3].u.varray->length; i++)
    JS_COPY (&argv[i + 1], &args[3].u.varray->data[i]);

  /* Method name to C string. */
  cp = js_string_to_c_string (vm, &args[2]);

  /* Call it. */
  result = js_vm_call_method (vm, &args[1], cp, args[3].u.varray->length + 1,
			      argv);

  /* Cleanup. */
  js_free (cp);
  js_free (argv);

  if (result)
    JS_COPY (result_return, &vm->exec_result);
  else
    /* The error message is already there. */
    js_vm_error (vm);
}


static void
js_core_globals (JSInterpPtr interp)
{
  JSNode *n;
  JSBuiltinInfo *info;
  JSVirtualMachine *vm = interp->vm;

  if (!interp->options.no_compiler)
    {
      /* Command `eval'. */

      info = js_vm_builtin_info_create (vm);
      info->global_method_proc = eval_global_method;

      n = &interp->vm->globals[js_vm_intern (interp->vm, "eval")];

      js_vm_builtin_create (interp->vm, n, info, interp);
    }

  /* Command `load'. */

  info = js_vm_builtin_info_create (vm);
  info->global_method_proc = load_global_method;

  n = &interp->vm->globals[js_vm_intern (interp->vm, "load")];
  js_vm_builtin_create (interp->vm, n, info, interp);

  /* Command `loadClass'. */

  info = js_vm_builtin_info_create (vm);
  info->global_method_proc = load_class_global_method;

  n = &interp->vm->globals[js_vm_intern (interp->vm, "loadClass")];
  js_vm_builtin_create (interp->vm, n, info, interp);

  /* Command `callMethod'. */

  info = js_vm_builtin_info_create (vm);
  info->global_method_proc = call_method_global_method;

  n = &interp->vm->globals[js_vm_intern (interp->vm, "callMethod")];
  js_vm_builtin_create (interp->vm, n, info, interp);
}


static void
js_global_method_stub (JSVirtualMachine *vm, JSBuiltinInfo *builtin_info,
		       void *instance_context, JSNode *result_return,
		       JSNode *args)
{
  JSMethodResult result;
  JSGlobalMethodContext *ctx = instance_context;

  /* Set the default result. */
  result_return->type = JS_UNDEFINED;

  /* Call the user supplied function. */
  result = (*ctx->proc) (ctx->context, ctx->interp, args->u.vinteger,
			 (JSType *) &args[1], (JSType *) result_return,
			 vm->error);
  if (result != JS_OK)
    js_vm_error (ctx->interp->vm);
}


static void
js_global_method_delete (JSBuiltinInfo *builtin_info, void *instance_context)
{
  JSGlobalMethodContext *ctx = instance_context;

  if (ctx)
    {
      if (ctx->free_proc)
	(*ctx->free_proc) (ctx->context);

      js_free (ctx);
    }
}


/* I/O Stream to user I/O function. */

static int
iofunc_io (void *context, unsigned char *buffer, unsigned int todo,
	   int *error_return)
{
  JSUserIOFuncCtx *ctx = context;
  int moved;

  *error_return = 0;

  moved = (*ctx->func) (ctx->context, buffer, todo);
  if (moved >= 0)
    ctx->position += moved;

  return moved;
}


static int
iofunc_seek (void *context, long offset, int whence)
{
  return -1;
}


static long
iofunc_get_position (void *context)
{
  JSUserIOFuncCtx *ctx = context;

  return ctx->position;
}


static long
iofunc_get_length (void *context)
{
  return -1;
}


static void
iofunc_close (void *context)
{
  js_free (context);
}


static JSIOStream *
iostream_iofunc (JSIOFunc func, void *context, int readp, int writep)
{
  JSIOStream *stream = js_iostream_new ();
  JSUserIOFuncCtx *ctx;

  if (stream == NULL)
    return NULL;

  ctx = js_malloc (NULL, sizeof (*ctx));
  if (ctx == NULL)
    {
      (void) js_iostream_close (stream);
      return NULL;
    }

  /* Init context. */
  ctx->func = func;
  ctx->context = context;
  ctx->position = 0;

  if (readp)
    stream->read = iofunc_io;
  if (writep)
    stream->write = iofunc_io;

  stream->seek		= iofunc_seek;
  stream->get_position	= iofunc_get_position;
  stream->get_length	= iofunc_get_length;
  stream->close		= iofunc_close;
  stream->context 	= ctx;

  return stream;
}
