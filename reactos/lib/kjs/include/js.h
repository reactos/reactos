/*
 * Public API for the JavaScript interpreter.
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
 * $Source: /cygdrive/c/RCVS/CVS/ReactOS/reactos/lib/kjs/include/js.h,v $
 * $Id: js.h,v 1.1 2004/01/10 20:38:17 arty Exp $
 */

#ifndef JS_H
#define JS_H

#include <stdio.h>

#ifndef JS_DLLEXPORT

#ifdef WIN32
#define JS_DLLEXPORT   __declspec(dllexport)
#else /* not WIN32 */
#define JS_DLLEXPORT
#endif /* not WIN32 */

#endif /* not JS_DLLEXPORT */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Types and definitions.
 */

/* Flags for class methods and properties. */
#define JS_CF_STATIC	0x01
#define JS_CF_IMMUTABLE	0x02

/*
 * This is frustrating.  We have to define these, since MS's and
 * Borlands' compilers can't agree on how to handle the stupid export
 * declarations and function pointer return values.
 */
typedef char *JSCharPtr;
typedef void *JSVoidPtr;

/* Byte-code instruction dispatch methods. */
typedef enum
{
  JS_VM_DISPATCH_SWITCH_BASIC,
  JS_VM_DISPATCH_SWITCH,
  JS_VM_DISPATCH_JUMPS
} JSVMDispatchMethod;

typedef struct js_interp_st *JSInterpPtr;

/*
 * An I/O function that can be set to interpreter options to redirect
 * the system's default streams.
 */
typedef int (*JSIOFunc) (void *context, unsigned char *buffer,
			 unsigned int amount);

/*
 * Hook that is called at certain points during the byte-code
 * execution.  Note that the JS_HOOK_OPERAND_COUNT is only available,
 * if the interpreter was configured with `--enable-operand-hooks'
 * option.
 */

#define JS_EVENT_OPERAND_COUNT		1
#define JS_EVENT_GARBAGE_COLLECT	2

typedef int (*JSHook) (int event, void *context);


/* Interpreter options. */
struct js_interp_options_st
{
  unsigned int stack_size;
  JSVMDispatchMethod dispatch_method;
  unsigned int verbose;

  /* Virtual machine flags. */

  unsigned int no_compiler : 1;
  unsigned int stacktrace_on_error : 1;

  unsigned int secure_builtin_file : 1;
  unsigned int secure_builtin_system : 1;

  /* Misc compiler flags. */
  unsigned int annotate_assembler : 1;
  unsigned int debug_info : 1;
  unsigned int executable_bc_files : 1;

  /* Compiler warning flags. */
  unsigned int warn_unused_argument : 1;
  unsigned int warn_unused_variable : 1;
  unsigned int warn_undef : 1;	/* runtime option */
  unsigned int warn_shadow : 1;
  unsigned int warn_with_clobber : 1;
  unsigned int warn_missing_semicolon : 1;
  unsigned int warn_strict_ecma : 1;
  unsigned int warn_deprecated : 1;

  /* Compiler optimization flags. */
  unsigned int optimize_peephole : 1;
  unsigned int optimize_jumps_to_jumps : 1;
  unsigned int optimize_bc_size : 1;
  unsigned int optimize_heavy : 1;

  /*
   * The standard system streams.  If these are NULL, the streams are
   * bind to the system's stdin, stdout, and stderr files.
   */
  JSIOFunc s_stdin;
  JSIOFunc s_stdout;
  JSIOFunc s_stderr;
  void *s_context;

  /* The callback hook. */
  JSHook hook;
  void *hook_context;

  /*
   * Call the <hook> after the virtual machine has executed this many
   * operands.
   */
  unsigned int hook_operand_count_trigger;

  /* How many file descriptors the interpreter can allocate. */
  unsigned long fd_count;
};

typedef struct js_interp_options_st JSInterpOptions;


/* JavaScript public types. */

#define JS_TYPE_UNDEFINED	0
#define JS_TYPE_NULL		1
#define JS_TYPE_BOOLEAN		2
#define JS_TYPE_INTEGER		3
#define JS_TYPE_STRING		4
#define JS_TYPE_DOUBLE		5
#define JS_TYPE_ARRAY		6
#define JS_TYPE_BUILTIN		11

/* String type. */
struct js_type_string_st
{
  unsigned int flags;
  unsigned char *data;
  unsigned int len;
};

typedef struct js_type_string_st JSTypeString;

/* Array type. */
struct js_type_array_st
{
  unsigned int length;
  struct js_type_st *data;
};

typedef struct js_type_array_st JSTypeArray;

/* The type closure. */
struct js_type_st
{
  int type;			/* One of the JS_TYPE_* values. */

  union
  {
    long i;			/* integer or boolean */
    JSTypeString *s;		/* string */
    double d;			/* double */
    JSTypeArray *array;		/* array */

    struct			/* Fillter to assure that this the type */
    {				/* structure has correct size. */
      unsigned int a1;
      unsigned int a2;
    } align;
  } u;
};

typedef struct js_type_st JSType;

/* Function type to free client data. */
typedef void (*JSFreeProc) (void *context);

/* Return types for methods. */
typedef enum
{
  JS_OK,
  JS_ERROR
} JSMethodResult;

/* Function type for global methods. */
typedef JSMethodResult (*JSGlobalMethodProc) (void *context,
					      JSInterpPtr interp, int argc,
					      JSType *argv,
					      JSType *result_return,
					      char *error_return);

/* Function type for JS modules. */
typedef void (*JSModuleInitProc) (JSInterpPtr interp);


/* Classes. */

typedef struct js_class_st *JSClassPtr;

typedef JSMethodResult (*JSMethodProc) (JSClassPtr cls, void *instance_context,
					JSInterpPtr interp, int argc,
					JSType *argv, JSType *result_return,
					char *error_return);

typedef JSMethodResult (*JSPropertyProc) (JSClassPtr cls,
					  void *instance_context,
					  JSInterpPtr interp, int setp,
					  JSType *value, char *error_return);

typedef JSMethodResult (*JSConstructor) (JSClassPtr cls, JSInterpPtr interp,
					 int argc, JSType *argv,
					 void **ictx_return,
					 JSFreeProc *ictx_destructor_return,
					 char *error_return);


/*
 * Prototypes global functions.
 */

/*
 * Interpreter handling.
 */

/*
 * Return a string that describes the JavaScript interpreter version
 * number.  The returned string is in format "MAJOR.MINOR.PATCH",
 * where MAJOR, MINOR, and PATCH are integer numbers.
 */
const JSCharPtr JS_DLLEXPORT js_version ();

/* Initialize interpreter options to system's default values. */
void JS_DLLEXPORT js_init_default_options (JSInterpOptions *options);

/* Create a new JavaScript interpreter. */
struct _KJS;
JSInterpPtr JS_DLLEXPORT js_create_interp (JSInterpOptions *options,
					   struct _KJS *kjs);

/* Destroy interpreter <interp>. */
void JS_DLLEXPORT js_destroy_interp (JSInterpPtr interp);

/* Return error message from the latest error. */
const JSCharPtr JS_DLLEXPORT js_error_message (JSInterpPtr interp);

/*
 * Get the result of the latest evaluation or execution in interpreter
 * <interp>.  The result is returned in <result_return>.  All data,
 * returned in <result_return>, belongs to the interpreter.  The
 * caller must not modify or changed it in any ways.
 */
void JS_DLLEXPORT js_result (JSInterpPtr interp, JSType *result_return);

/* Set the value of global variable <name> to <value>. */
void JS_DLLEXPORT js_set_var (JSInterpPtr interp, char *name, JSType *value);

/* Get the value of global variable <name> to <value>. */
void JS_DLLEXPORT js_get_var (JSInterpPtr interp, char *name, JSType *value);

/* Get the options of interpreter <interp> to <options>.  */
void JS_DLLEXPORT js_get_options (JSInterpPtr interp,
				  JSInterpOptions *options);

/* Modify the options of interpreter <interp> according to <options>. */
void JS_DLLEXPORT js_set_options (JSInterpPtr interp,
				  JSInterpOptions *options);

/*
 * Evaluation and compilation.
 */

/* Evalutate JavaScript code <code> with interpreter <interp>. */
int JS_DLLEXPORT js_eval (JSInterpPtr interp, char *code);

/* Evaluate JavaScript code <data, datalen> with interpreter <interp>. */
int JS_DLLEXPORT js_eval_data (JSInterpPtr interp, char *data,
			       unsigned int datalen);

/*
 * Evaluate file <filename> with interpreter <interp>.  File
 * <filename> can contain JavaScript of byte-code.  The function
 * return 1 if the evaluation was successful or 0 otherwise.  If the
 * evaluation failed, the error message can be retrieved with the
 * function js_error_message().
 */
int JS_DLLEXPORT js_eval_file (JSInterpPtr interp, char *filename);

/* Eval JavaScript file <filename> with interpreter <interp>. */
int JS_DLLEXPORT js_eval_javascript_file (JSInterpPtr interp, char *filename);

/* Execute a byte-code file <filename> with interpreter <interp>. */
int JS_DLLEXPORT js_execute_byte_code_file (JSInterpPtr interp,
					    char *filename);

/*
 * Call funtion <name> with arguments <argc, argv>.  The return value
 * of the function <name> can be retrieved with the js_result()
 * function.
 */
int JS_DLLEXPORT js_apply (JSInterpPtr interp, char *name,
			   unsigned int argc, JSType *argv);

/* Compile JavaScript file <input_file> to assembler or byte-code. */
int JS_DLLEXPORT js_compile (JSInterpPtr interp, char *input_file,
			     char *assembler_file, char *byte_code_file);

/*
 * Compile JavaScript file <input_file> to byte-code and return the
 * resulting byte-code data in <bc_return, bc_len_return>.  The
 * returned byte-code data <bc_return> belongs to the interpreter
 * and it must be saved by the caller *before* any other JS functions
 * is called.  If the data is not saved, its contents will be invalid
 * at the next garbage collection.
 */
int JS_DLLEXPORT js_compile_to_byte_code (JSInterpPtr interp, char *input_file,
					  unsigned char **bc_return,
					  unsigned int *bc_len_return);

/*
 * Compile JavaScript code <data, datalen> to byte-code and return the
 * resulting byte-code data in <bc_return, bc_len_return>.
 */
int JS_DLLEXPORT js_compile_data_to_byte_code (JSInterpPtr interp, char *data,
					       unsigned int datalen,
					       unsigned char **bc_return,
					       unsigned int *bc_len_return);

/*
 * Execute byte-code data <bc, bc_len>.  The byte-code data is the
 * contents of a byte-code file, or a copy of the data returned by the
 * js_compile{,_data}_to_byte_code() functions.
 *
 * Note! You can't use the data from the js_compile{,_data}_to_byte_code()
 * functions as an input for this function.  Instead, you must take a
 * private copy of the data and pass that copy to the function:
 *
 *  if (js_compile_to_byte_code (interp, "./foo.js", &bc, &bclen))
 *    {
 *      char *bc_copy = xmalloc (bclen);
 *
 *      memcpy (bc_copy, bc, bclen);
 *      js_execute_byte_code (interp, bc_copy, bclen);
 *      xfree (bc_copy);
 *    }
 */
int JS_DLLEXPORT js_execute_byte_code (JSInterpPtr interp, unsigned char *bc,
				       unsigned int bc_len);


/*
 * Type handling.
 */

/*
 * Create a new string type from <data> that has <length> bytes of data.
 * All fields of <type> belong to the interpreter.
 */
void JS_DLLEXPORT js_type_make_string (JSInterpPtr interp, JSType *type,
				       unsigned char *data,
				       unsigned int length);

/* Create a new array with capacity of <length> items. */
void JS_DLLEXPORT js_type_make_array (JSInterpPtr interp, JSType *type,
				      unsigned int length);


/*
 * Global methods.
 */

/*
 * Create a new global method to the interpreter <interp>.  The
 * method's name will be <name> and its implementation is function
 * <proc>.  The argument <context> is a user-specified data that is
 * passed to the command <proc>.  When the command is deleted, the
 * argument <context_free_proc> is called for data <context> to free
 * up all resources the <context> might have.  The function returns
 * 1 if the operation was successful or 0 otherwise.
 */
int JS_DLLEXPORT js_create_global_method (JSInterpPtr interp, char *name,
					  JSGlobalMethodProc proc,
					  void *context,
					  JSFreeProc context_free_proc);


/*
 * Classes.
 */

/*
 * Create a new class with class context data <class_context>.  The
 * context data is destroyed with <class_context_destructor>.  If the
 * argument <no_auto_destroy> is not 0, the JavaScript interpreter
 * will *not* destroy the class when the interpreter is destroyed.  In
 * that case, it is the caller's responsibility to call
 * js_class_destroy() for the returned class handle, after the
 * interpreter has been destroyed.  If the argument <constructor> is
 * not NULL, it is used to instantiate the class when a `new CLASS
 * (ARGS);' expression is evaluated in the JavaScript code.
 */
JSClassPtr JS_DLLEXPORT js_class_create (void *class_context,
					 JSFreeProc class_context_destructor,
					 int no_auto_destroy,
					 JSConstructor constuctor);

void JS_DLLEXPORT js_class_destroy (JSClassPtr cls);

JSVoidPtr JS_DLLEXPORT js_class_context (JSClassPtr cls);

int JS_DLLEXPORT js_class_define_method (JSClassPtr cls, char *name,
					 unsigned int flags,
					 JSMethodProc method);

int JS_DLLEXPORT js_class_define_property (JSClassPtr cls, char *name,
					   unsigned int flags,
					   JSPropertyProc property);

int JS_DLLEXPORT js_define_class (JSInterpPtr interp, JSClassPtr cls,
				  char *name);

int JS_DLLEXPORT js_instantiate_class (JSInterpPtr interp, JSClassPtr cls,
				       void *ictx, JSFreeProc ictx_destructor,
				       JSType *result_return);

const JSClassPtr JS_DLLEXPORT js_lookup_class (JSInterpPtr interp,
					       char *name);

/*
 * Check if object <object> is an instance of class <cls>.  The
 * function returns a boolean success status.  If the argument
 * <instance_context_return> is not NULL, it will be set to the
 * instance context of object <object>.
 */
int JS_DLLEXPORT js_isa (JSInterpPtr interp, JSType *object, JSClassPtr cls,
			 void **instance_context_return);


/*
 * Modules.
 */

/*
 * Call the module initialization function <init_proc>.  The function
 * return 1 if the module was successfully initialized or 0 otherwise.
 * In case of error, the error message can be fetched with the
 * js_error_mesage() function.
 */
int JS_DLLEXPORT js_define_module (JSInterpPtr interp,
				   JSModuleInitProc init_proc);


/*
 * The default modules.
 */

/* JavaScript interpreter extension. */
void JS_DLLEXPORT js_ext_JS (JSInterpPtr interp);

/* Curses extension. */
void JS_DLLEXPORT js_ext_curses (JSInterpPtr interp);

/* MD5 extension. */
void JS_DLLEXPORT js_ext_MD5 (JSInterpPtr interp);

#ifdef __cplusplus
}
#endif

#endif /* not JS_H */
