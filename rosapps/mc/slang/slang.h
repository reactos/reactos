#ifndef DAVIS_SLANG_H_
#define DAVIS_SLANG_H_
/* -*- mode: C; mode: fold; -*- */
/* Copyright (c) 1992, 1995 John E. Davis
 * All rights reserved.
 * 
 * You may distribute under the terms of either the GNU General Public
 * License or the Perl Artistic License.
 */
#define SLANG_VERSION 9938
/*{{{ System Dependent Macros and Typedefs */

#if defined(__WATCOMC__) && !defined(__QNX__)
#  ifndef msdos
#    define msdos
#  endif
#  ifndef  DOS386
#    define  DOS386
#  endif
#  ifndef FLOAT_TYPE
#    define FLOAT_TYPE
#  endif
#  ifndef pc_system
#    define pc_system
#  endif
#endif /* __watcomc__ */

#ifdef unix
# ifndef __unix__
#  define __unix__ 1
# endif
#endif

#ifndef __GO32__
# ifdef __unix__
#  define REAL_UNIX_SYSTEM
# endif
#endif

/* Set of the various defines for pc systems.  This includes OS/2 */
#ifdef __MSDOS__
# ifndef msdos
#   define msdos
# endif
# ifndef pc_system
#   define pc_system
# endif
#endif

#ifdef __GO32__
# ifndef pc_system
#   define pc_system
# endif
# ifdef REAL_UNIX_SYSTEM
#   undef REAL_UNIX_SYSTEM
# endif
#endif

#if defined(__EMX__) && defined(OS2)
# ifndef pc_system
#   define pc_system
# endif
# ifndef __os2__
#   define __os2__
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}
#endif
/* ---------------------------- Generic Macros ----------------------------- */

/*  __SC__ is defined for Symantec C++
   DOS386 is defined for -mx memory model, 32 bit DOS extender. */

#ifdef VOID
# undef VOID
#endif

#if defined(msdos) && !defined(DOS386) & !defined(__WIN32__) && !defined(__GO32__)
# ifdef __SC__
#  include <dos.h>
# endif
   typedef void *VOID_STAR;
# define VOID void
# include <alloc.h>
#else
# if defined (__cplusplus) || defined(__STDC__)
   typedef void *VOID_STAR;
#  define VOID void
# else
   typedef unsigned char *VOID_STAR;
#  define VOID unsigned char
# endif
#endif
   
#if 1
   typedef int (*FVOID_STAR)(void);
#else
# define FVOID_STAR VOID_STAR
#endif
   
#if defined(msdos) && !defined(DOS386) && !defined(__GO32__) && !defined(__WIN32__)
# define SLFREE(buf)  farfree((void far *)(buf))
# define SLMALLOC(x) farmalloc((unsigned long) (x))
# define SLREALLOC(buf, n) farrealloc((void far *) (buf), (unsigned long) (n))
# define SLCALLOC(n, m) farcalloc((unsigned long) (n), (unsigned long) (m))
#else
# if defined(VMS) && !defined(__DECC)
#  define SLFREE VAXC$FREE_OPT
#  define SLMALLOC VAXC$MALLOC_OPT
#  define SLREALLOC VAXC$REALLOC_OPT
#  define SLCALLOC VAXC$CALLOC_OPT
# else
#  define SLFREE(x) free((char *)(x))
#  define SLMALLOC malloc
#  if defined(__cplusplus) && !defined(__BEOS__)
#   define SLREALLOC(p,n) realloc((malloc_t) (p), (n))
#  else
#   define SLREALLOC realloc
#  endif
#  define SLCALLOC calloc
# endif
#endif

#ifdef SL_MALLOC_DEBUG
# undef SLMALLOC
# undef SLCALLOC
# undef SLREALLOC
# undef SLFREE
# define SLMALLOC(x) SLdebug_malloc((unsigned long) (x))
# define SLFREE(x) SLdebug_free((unsigned char *)(x))
# define SLCALLOC(n, m) SLdebug_calloc((unsigned long) (n), (unsigned long)(m))
# define SLREALLOC(p, x) SLdebug_realloc((unsigned char *)(p), (unsigned long)(x))
#endif /* SL_MALLOC_DEBUG */
   
   extern unsigned char *SLdebug_malloc (unsigned long);
   extern unsigned char *SLdebug_calloc (unsigned long, unsigned long);
   extern unsigned char *SLdebug_realloc (unsigned char *, unsigned long);
   extern void SLdebug_free (unsigned char *);
   extern void SLmalloc_dump_statistics (void);
   extern char *SLstrcpy(register char *, register char *);
   extern int SLstrcmp(register char *, register char *);
   extern char *SLstrncpy(char *, register char *, register  int);
   
   extern void SLmemset (char *, char, int);
   extern char *SLmemchr (register char *, register char, register int);
   extern char *SLmemcpy (char *, char *, int);
   extern int SLmemcmp (char *, char *, int);

#ifdef float64
# undef float64
#endif
   
#ifndef FLOAT64_TYPEDEFED
# define FLOAT64_TYPEDEFED
  typedef double float64;
#endif


/*}}}*/

/*{{{ Interpreter Typedefs */

#define SLANG_MAX_NAME_LEN 30
/* maximum length of an identifier */
/* first char in identifiers is the hash */

   /* Note that long is used for addresses instead of void *.  The reason for
    * this is that I have a gut feeling that sizeof (long) > sizeof(void *)
    * on some machines.  This is certainly the case for MSDOS where addresses
    * can be 16 bit.
    */
typedef struct SLang_Name_Type
  {
#ifdef SLANG_STATS
     int n;			       /* number of times referenced */
#endif
     char name[SLANG_MAX_NAME_LEN + 2]; /* [0] is hash */

     unsigned char sub_type;
     
/* Values for main_type may be as follows.  The particlular values are
 * for compatability.
 */
#define SLANG_LVARIABLE		0x01
#define SLANG_INTRINSIC 	0x06
#define SLANG_FUNCTION  	0x07
#define SLANG_GVARIABLE 	0x0D
#define SLANG_IVARIABLE 	0x0E           /* intrinsic variables */
/* Note!!! For Macro MAKE_VARIABLE below to work, SLANG_IVARIABLE Must
   be 1 less than SLANG_RVARIABLE!!! */
#define SLANG_RVARIABLE		0x0F	       /* read only variable */
     unsigned char main_type;
     long addr;
  }
SLang_Name_Type;


typedef struct SLang_Load_Type
{
   long name;			       /* file name, string address, ... */
   long handle;			       /* FILE *, string address, etc... */
   
   char *ptr;			       /* input pointer to next line in object
					* to be read. 
					*/
   /* Things below here are used by S-Lang. */
   int type;			       /* 'F' = file, 'S' = String, etc.. */
   char *buf;			       /* buffer for file, etc... */
   char *(*read)(struct SLang_Load_Type *);   /* function to call to read obj */
   int n;			       /* line number, etc... */
   char token[256];		       /* token to be parsed */
   int ofs;			       /* offset from buf where last read
					* took place 
					*/
   int top_level;		       /* 1 if at top level of parsing */
} SLang_Load_Type;

#if defined(ultrix) && !defined(__GNUC__)
# ifndef NO_PROTOTYPES
#  define NO_PROTOTYPES
# endif
#endif
   
#ifndef NO_PROTOTYPES 
# define _PROTO(x) x
#else
# define _PROTO(x) ()
#endif

typedef struct SL_OOBinary_Type
{
   unsigned char sub_type;	       /* partner type for binary op */
   
   /* The function take the binary op as first argument, the operand types
    * form the second and third parameters and the last two parameters are 
    * pointers to the objects themselves.  It is up to the function to push
    * the result on the stack.  It must return 1 if it handled the operation 
    * return zero if the operation is not defined.
    */
   int (*binary_function)_PROTO((int, unsigned char, unsigned char, 
				  VOID_STAR, VOID_STAR));

   struct SL_OOBinary_Type *next;
}
SL_OOBinary_Type;

typedef struct
{
   /* Methods */
   void (*destroy)_PROTO((VOID_STAR));
   /* called to delete/free the object */
   char *(*string)_PROTO((VOID_STAR));
   /* returns a string representation of the object */
   int (*unary_function)_PROTO((int, unsigned char, VOID_STAR));
   /* unary operation function */
   SL_OOBinary_Type *binary_ops;
   
   int (*copy_function)_PROTO((unsigned char, VOID_STAR));
   /* This function is called do make a copy of the object */
} SLang_Class_Type;

extern SLang_Class_Type *SLang_Registered_Types[256];
   
typedef struct
{
   unsigned char main_type;	       /* SLANG_RVARIABLE, etc.. */
   unsigned char sub_type;	       /* int, string, etc... */
   long *obj;			       /* address of user structure */

   /* Everything below is considered private */
   unsigned int count;		       /* number of references */
}
SLuser_Object_Type;

   
/*}}}*/
/*{{{ Interpreter Function Prototypes */

  extern volatile int SLang_Error;
  /* Non zero if error occurs.  Must be reset to zero to continue. */

  extern int SLang_Traceback;
  /* If non-zero, dump an S-Lang traceback upon error.  Available as 
     _traceback in S-Lang. */

  extern char *SLang_User_Prompt;
  /* Prompt to use when reading from stdin */
  extern int SLang_Version;

  extern void (*SLang_Error_Routine)(char *);
  /* Pointer to application dependent error messaging routine.  By default,
     messages are displayed on stderr. */

  extern void (*SLang_Exit_Error_Hook)(char *);
  extern void SLang_exit_error (char *);
  extern void (*SLang_Dump_Routine)(char *);
  /* Called if S-Lang traceback is enabled as well as other debugging 
     routines (e.g., trace).  By default, these messages go to stderr. */
  
  extern void (*SLang_Interrupt)(void);
  /* function to call whenever inner interpreter is entered.  This is 
     a good place to set SLang_Error to USER_BREAK. */

  extern void (*SLang_User_Clear_Error)(void);
  /* function that gets called when '_clear_error' is called. */
  extern int (*SLang_User_Open_Slang_Object)(SLang_Load_Type *); 
  extern int (*SLang_User_Close_Slang_Object)(SLang_Load_Type *);
  /* user defined loading routines. */


  /* If non null, these call C functions before and after a slang function. */
  extern void (*SLang_Enter_Function)(char *);
  extern void (*SLang_Exit_Function)(char *);


/* Functions: */

   extern int init_SLang(void);   
   /* This function is mandatory and must be called by all applications */
   extern int init_SLfiles(void);
   /* called if fputs, fgets, etc are need in S-Lang */
   extern int init_SLmath(void);
   /* called if math functions sin, cos, etc... are needed. */

   extern int init_SLunix(void);
   /* unix system functions chmod, stat, etc... */

   extern int init_SLmatrix(void);
   
   extern int SLang_add_table(SLang_Name_Type *, char *);
   /* add application dependent function table p1 to S-Lang.  A name p2 less 
    *  than 32 characters must also be supplied.      
    * Returns 0 upon failure or 1 upon success. */

   extern int SLang_add_global_variable (char *);
   extern int SLang_load_object(SLang_Load_Type *);
   extern int SLang_load_file(char *);
   /* Load a file of S-Lang code for interpreting.  If the parameter is
   NULL, input comes from stdin. */

   extern void SLang_restart(int);
   /* should be called if an error occurs.  If the passed integer is
    * non-zero, items are popped off the stack; otherwise, the stack is 
    * left intact.  Any time the stack is believed to be trashed, this routine
    * should be called with a non-zero argument (e.g., if setjmp/longjmp is
    * called). */ 

   extern void SLang_byte_compile_file(char *, int *);
   /* takes a file of S-Lang code and ``byte-compiles'' it for faster
    * loading.  The new filename is equivalent to the old except that a `c' is
    * appended to the name.  (e.g., init.sl --> init.slc).  If the second 
    * parameter is non-zero, preprocess the file only.
    */

   extern void SLang_autoload(char *, char *);
   /* Automatically load S-Lang function p1 from file p2.  This function
      is also available via S-Lang */
   
   extern char *SLang_load_string(char *);
   /* Like SLang_load_file except input is from a null terminated string. */
   
   extern void SLang_do_pop(void);
   /* pops item off stack and frees any memory associated with it */
   
   extern int SLang_pop_integer(int *);
   /* pops integer *p0 from the stack.  Returns 0 upon success and non-zero
    * if the stack is empty or a type mismatch occurs, setting SLang_Error.
    */

   extern int SLpop_string (char **);
   extern int SLang_pop_string(char **, int *);
   /* pops string *p0 from stack.  If *p1 is non-zero, the string must be
    * freed after its use.  DO NOT FREE p0 if *p1 IS ZERO! Returns 0 upon
    * success */
      
   extern int SLang_pop_float(float64 *, int *, int *);
   /* Pops float *p1 from stack.  If *p3 is non-zero, *p1 was derived
      from the integer *p2. Returns zero upon success. */
      
   extern SLuser_Object_Type *SLang_pop_user_object (unsigned char);
   extern void SLang_free_user_object (SLuser_Object_Type *);
   extern void SLang_free_intrinsic_user_object (SLuser_Object_Type *);
   /* This is like SLang_free_user_object but is meant to free those
    * that have been declared as intrinsic variables by the application.
    * Normally an application would never need to call this.
    */

   extern void SLang_push_user_object (SLuser_Object_Type *);
   extern SLuser_Object_Type *SLang_create_user_object (unsigned char);
   
   extern int SLang_add_unary_op (unsigned char, FVOID_STAR);
   extern int SLang_add_binary_op (unsigned char, unsigned char, FVOID_STAR);
   extern int SLang_register_class (unsigned char, FVOID_STAR, FVOID_STAR);
   extern int SLang_add_copy_operation (unsigned char, FVOID_STAR);

   extern long *SLang_pop_pointer(unsigned char *, unsigned char *, int *);
   /* Returns a pointer to object of type *p1,*p2 on top of stack. 
      If *p3 is non-zero, the Object must be freed after use. */


   extern void SLang_push_float(float64);
   /* Push Float onto stack */

   extern void SLang_push_string(char *);
   /* Push string p1 onto stack */
   
   extern void SLang_push_integer(int);
   /* push integer p1 on stack */

   extern void SLang_push_malloced_string(char *);
   /* The normal SLang_push_string mallocs space for the string.  This one
      does not.  DO NOT FREE IT IF YOU USE THIS ROUTINE */

   extern int SLang_is_defined(char *);
   /* Return non-zero is p1 is defined otherwise returns 0. */
   
   extern int SLang_run_hooks(char *, char *, char *);
   /* calls S-Lang function p1 pushing strings p2 and p3 onto the stack
    * first.  If either string is NULL, it is not pushed. If p1 is not
    * defined, 0 is returned. */

   extern int SLang_execute_function(char *);
   /* Call S-Lang function p1.  Returns 0 if the function is not defined 
    * and 1 if it is.
    */

   extern char *SLang_find_name(char *);
   /* Return a pointer to p1 in table if it is defined.  Returns NULL
    * otherwise.  This is useful when one wants to avoid redundant strings. 
    */

   extern char *SLang_rpn_interpret(char *);
   /* Interpret string as reverse polish notation */

   extern void SLang_doerror(char *);
   /* set SLang_Error and display p1 as error message */
   
   extern SLuser_Object_Type *SLang_add_array(char *, long *, 
					      int, int, int, int, 
					      unsigned char, unsigned char);
   /* This function has not been tested thoroughly yet.  Its purpose is to 
    * allow a S-Lang procedure to access a C array. For example, suppose that 
    * you have an array of 100 ints defined as:
    *  
    *  int c_array[100];
    *
    * By calling something like:
    *
    *   SLang_add_array ("array_name", (long *) c_array, 1, 100, 0, 0,
    *   		 'i', SLANG_IVARIABLE);
    *
    * the array can be accessed by the name 'array_name'.  This function 
    * returns -1 upon failure.  The 3rd argument specifies the dimension 
    * of the array, the 4th, and 5th arguments specify how many elements
    * there are in the x,y, and z directions.  The last argument must 
    * be one of: 
    * 
    *        SLANG_IVARIABLE:  indicates array is writable
    *        SLANG_RVARIABLE:  indicates array is read only
    * 
    * Returns NULL upon failure.
    */


extern int SLang_free_array_handle (int);
/* This routine may be called by application to free array handle created by 
 * the application.  Returns 0 upon success, -1 if the handle is invalid and
 * -2 if the handle is not associated with a C array.
 */

   extern char *SLang_extract_list_element(char *, int *, int*);
   extern void SLexpand_escaped_string (register char *, register char *, 
					register char *);

extern SLang_Name_Type *SLang_get_function (char *);
/* The parameter is the name of a user defined S-Lang function.  This 
 * routine returns NULL if the function does not exist or it returns the
 * a pointer to it in an internal S-Lang table.  This pointer can be used
 * by 'SLexecute_function' to call the function directly from C.
 */
  
extern void SLexecute_function(SLang_Name_Type *);
/* This function allows an application to call a S-Lang function from within
 * the C program.  The parameter must be non-NULL and must have been 
 * previously obtained by a call to 'SLang_get_function'.
 */
extern void SLroll_stack (int *);
/* If argument *p is positive, the top |*p| objects on the stack are rolled
 * up.  If negative, the stack is rolled down.
 */

extern void SLmake_lut (unsigned char *, unsigned char *, unsigned char);

   extern int SLang_guess_type (char *);


/*}}}*/

/*{{{ Misc Functions */

extern char *SLmake_string (char *);
extern char *SLmake_nstring (char *, unsigned int);
/* Returns a null terminated string made from the first n characters of the
 * string.
 */

extern char *SLcurrent_time_string (void);

extern int SLatoi(unsigned char *);

extern int SLang_extract_token(char **, char *, int);
/* returns 0 upon failure and non-zero upon success.  The first parameter 
 * is a pointer to the input stream which this function will bump along.  
 * The second parameter is the buffer where the token is placed.  The third 
 * parameter is used internally by the S-Lang library and should be 0 for
 * user applications.
 */

/*}}}*/

/*{{{ SLang getkey interface Functions */
   
#ifdef REAL_UNIX_SYSTEM
extern int SLang_TT_Baud_Rate;
extern int SLang_TT_Read_FD;
#endif
   
extern int SLang_init_tty (int, int, int);
/* Initializes the tty for single character input.  If the first parameter *p1
 * is in the range 0-255, it will be used for the abort character;
 * otherwise, (unix only) if it is -1, the abort character will be the one
 * used by the terminal.  If the second parameter p2 is non-zero, flow
 * control is enabled.  If the last parmeter p3 is zero, output processing
 * is NOT turned on.  A value of zero is required for the screen management
 * routines. Returns 0 upon success. In addition, if SLang_TT_Baud_Rate ==
 * 0 when this function is called, SLang will attempt to determine the
 * terminals baud rate.  As far as the SLang library is concerned, if
 * SLang_TT_Baud_Rate is less than or equal to zero, the baud rate is
 * effectively infinite.
 */

extern void SLang_reset_tty (void);
/* Resets tty to what it was prior to a call to SLang_init_tty */
#ifdef REAL_UNIX_SYSTEM
extern void SLtty_set_suspend_state (int);
   /* If non-zero argument, terminal driver will be told to react to the
    * suspend character.  If 0, it will not.
    */
extern int (*SLang_getkey_intr_hook) (void);
#endif

#define SLANG_GETKEY_ERROR 0xFFFF
extern unsigned int SLang_getkey (void);
/* reads a single key from the tty.  If the read fails,  0xFFFF is returned. */

extern void SLang_ungetkey_string (unsigned char *, unsigned int);
extern void SLang_buffer_keystring (unsigned char *, unsigned int);
extern void SLang_ungetkey (unsigned char);
extern void SLang_flush_input (void);
extern int SLang_input_pending (int);
extern int SLang_Abort_Char;
/* The value of the character (0-255) used to trigger SIGINT */
extern int SLang_Ignore_User_Abort;
/* If non-zero, pressing the abort character will not result in USER_BREAK
 * SLang_Error. */

extern void SLang_set_abort_signal (void (*)(int));
/* If SIGINT is generated, the function p1 will be called.  If p1 is NULL
 * the SLang_default signal handler is called.  This sets SLang_Error to 
 * USER_BREAK.  I suspect most users will simply want to pass NULL.
 */

extern volatile int SLKeyBoard_Quit;

#ifdef VMS
/* If this function returns -1, ^Y will be added to input buffer. */
extern int (*SLtty_VMS_Ctrl_Y_Hook) (void);
#endif
/*}}}*/

/*{{{ SLang Keymap routines */

typedef struct SLKeymap_Function_Type
  {
      char *name;
      int (*f)(void);
  }
SLKeymap_Function_Type;

typedef struct SLang_Key_Type
  {
     unsigned char str[13];	       /* key sequence */
#define SLKEY_F_INTERPRET	0x01
#define SLKEY_F_INTRINSIC	0x02
#define SLKEY_F_KEYSYM		0x03
     unsigned char type;	       /* type of function */
#ifdef SLKEYMAP_OBSOLETE
     VOID_STAR f;			       /* function to invoke */
#else
     union
       {
	  char *s;
	  FVOID_STAR f;
	  unsigned int keysym;
       }
     f;
#endif
     struct SLang_Key_Type *next;      /* */
  }
SLang_Key_Type;

#define MAX_KEYMAP_NAME_LEN 8
typedef struct SLKeyMap_List_Type
{
   char name[MAX_KEYMAP_NAME_LEN + 1];
   SLang_Key_Type *keymap;
   SLKeymap_Function_Type *functions;  /* intrinsic functions */
}
SLKeyMap_List_Type;

/* This is arbitrary but I have got to start somewhere */
#ifdef msdos
#define SLANG_MAX_KEYMAPS 10
#else
#define SLANG_MAX_KEYMAPS 30
#endif

extern SLKeyMap_List_Type SLKeyMap_List[SLANG_MAX_KEYMAPS];   /* these better be inited to 0! */


extern char *SLang_process_keystring(char *);

#ifdef SLKEYMAP_OBSOLETE   
extern int SLang_define_key1(char *, VOID_STAR, unsigned int, SLKeyMap_List_Type *);
/* define key p1 in keymap p4 to invoke function p2.  If type p3 is given by
 * SLKEY_F_INTRINSIC, p2 is an intrinsic function, else it is a string to be
 * passed to the interpreter for evaluation.  The return value is important.
 * It returns 0 upon success, -1 upon malloc error, and -2 if the key is 
 * inconsistent.  SLang_Error is set upon error. */
#else
extern int SLkm_define_key (char *, FVOID_STAR, SLKeyMap_List_Type *);
#endif
   
extern int SLang_define_key(char *, char *, SLKeyMap_List_Type *);
/* Like define_key1 except that p2 is a string that is to be associated with 
 * a function in the functions field of p3.  This routine calls define_key1.
 */

extern int SLkm_define_keysym (char *, unsigned int, SLKeyMap_List_Type *);

extern void SLang_undefine_key(char *, SLKeyMap_List_Type *);

extern SLKeyMap_List_Type *SLang_create_keymap(char *, SLKeyMap_List_Type *);
/* create and returns a pointer to a new keymap named p1 created by copying
 * keymap p2.  If p2 is NULL, it is up to the calling routine to initialize
 * the keymap.
 */

extern char *SLang_make_keystring(unsigned char *);

extern SLang_Key_Type *SLang_do_key(SLKeyMap_List_Type *, int (*)(void));
/* read a key using keymap p1 with getkey function p2 */

extern
#ifdef SLKEYMAP_OBSOLETE
     VOID_STAR
#else
     FVOID_STAR
#endif
     SLang_find_key_function(char *, SLKeyMap_List_Type *);

extern SLKeyMap_List_Type *SLang_find_keymap(char *);

extern int SLang_Last_Key_Char;
extern int SLang_Key_TimeOut_Flag;


/*}}}*/

/*{{{ SLang Readline Interface */

typedef struct SLang_Read_Line_Type
{
   struct SLang_Read_Line_Type *prev, *next;
   unsigned char *buf;
   int buf_len;			       /* number of chars in the buffer */
   int num;			       /* num and misc are application specific*/
   int misc;
} SLang_Read_Line_Type;

/* Maximum size of display */
#define SLRL_DISPLAY_BUFFER_SIZE 256

typedef struct 
{
   SLang_Read_Line_Type *root, *tail, *last;
   unsigned char *buf;		       /* edit buffer */
   int buf_len;			       /* sizeof buffer */
   int point;			       /* current editing point */
   int tab;			       /* tab width */
   int len;			       /* current line size */
   
   /* display variables */
   int edit_width;		       /* length of display field */
   int curs_pos;			       /* current column */
   int start_column;		       /* column offset of display */
   int dhscroll;		       /* amount to use for horiz scroll */
   char *prompt;
   
   FVOID_STAR last_fun;		       /* last function executed by rl */

   /* These two contain an image of what is on the display */
   unsigned char upd_buf1[SLRL_DISPLAY_BUFFER_SIZE];
   unsigned char upd_buf2[SLRL_DISPLAY_BUFFER_SIZE];
   unsigned char *old_upd, *new_upd;   /* pointers to previous two buffers */
   int new_upd_len, old_upd_len;       /* length of output buffers */
   
   SLKeyMap_List_Type *keymap;

   /* tty variables */
   unsigned int flags;		       /*  */
#define SL_RLINE_NO_ECHO	1
#define SL_RLINE_USE_ANSI	2
   unsigned int (*getkey)(void);       /* getkey function -- required */
   void (*tt_goto_column)(int);
   void (*tt_insert)(char);
   void (*update_hook)(unsigned char *, int, int);
   /* The update hook is called with a pointer to a buffer p1 that contains
    * an image of what the update hook is suppoed to produce.  The length 
    * of the buffer is p2 and after the update, the cursor is to be placed
    * in column p3.
    */
} SLang_RLine_Info_Type;

extern int SLang_RL_EOF_Char;

extern SLang_Read_Line_Type * SLang_rline_save_line (SLang_RLine_Info_Type *);
extern int SLang_init_readline (SLang_RLine_Info_Type *);
extern int SLang_read_line (SLang_RLine_Info_Type *);
extern int SLang_rline_insert (char *);
extern void SLrline_redraw (SLang_RLine_Info_Type *);   
extern int SLang_Rline_Quit;

/*}}}*/

/*{{{ Low Level Screen Output Interface */

extern unsigned long SLtt_Num_Chars_Output;
extern int SLtt_Baud_Rate;

typedef unsigned long SLtt_Char_Type;

#define SLTT_BOLD_MASK	0x01000000
#define SLTT_BLINK_MASK	0x02000000
#define SLTT_ULINE_MASK	0x04000000
#define SLTT_REV_MASK	0x08000000
#define SLTT_ALTC_MASK  0x10000000

extern int SLtt_Screen_Rows;
extern int SLtt_Screen_Cols;
extern int SLtt_Term_Cannot_Insert;
extern int SLtt_Term_Cannot_Scroll;
extern int SLtt_Use_Ansi_Colors;
extern int SLtt_Ignore_Beep;
#if defined(REAL_UNIX_SYSTEM)
extern int SLtt_Force_Keypad_Init;
#endif
   
#ifndef __GO32__
#if defined(VMS) || defined(REAL_UNIX_SYSTEM)
extern int SLtt_Blink_Mode;
extern int SLtt_Use_Blink_For_ACS;
extern int SLtt_Newline_Ok;
extern int SLtt_Has_Alt_Charset;
extern int SLtt_Has_Status_Line;       /* if 0, NO.  If > 0, YES, IF -1, ?? */
# ifndef VMS
extern int SLtt_Try_Termcap;
# endif
#endif
#endif

#ifdef msdos
extern int SLtt_Msdos_Cheap_Video;
#endif
   
   
extern int SLtt_flush_output (void);
extern void SLtt_set_scroll_region(int, int);
extern void SLtt_reset_scroll_region(void);
extern void SLtt_reverse_video (int);
extern void SLtt_bold_video (void);
extern void SLtt_begin_insert(void);
extern void SLtt_end_insert(void);
extern void SLtt_del_eol(void);
extern void SLtt_goto_rc (int, int);
extern void SLtt_delete_nlines(int);
extern void SLtt_delete_char(void);
extern void SLtt_erase_line(void);
extern void SLtt_normal_video(void);
extern void SLtt_cls(void);
extern void SLtt_beep(void);
extern void SLtt_reverse_index(int);
extern void SLtt_smart_puts(unsigned short *, unsigned short *, int, int);
extern void SLtt_write_string (char *);
extern void SLtt_putchar(char);
extern void SLtt_init_video (void);
extern void SLtt_reset_video (void);
extern void SLtt_get_terminfo(void);
extern void SLtt_get_screen_size (void);
extern int SLtt_set_cursor_visibility (int);
   
#if defined(VMS) || defined(REAL_UNIX_SYSTEM)
extern void SLtt_enable_cursor_keys(void);
extern void SLtt_set_term_vtxxx(int *);
extern void SLtt_set_color_esc (int, char *);
extern void SLtt_wide_width(void);
extern void SLtt_narrow_width(void);
extern int SLtt_set_mouse_mode (int, int);
extern void SLtt_set_alt_char_set (int);
extern int SLtt_write_to_status_line (char *, int);
extern void SLtt_disable_status_line (void);
# ifdef REAL_UNIX_SYSTEM
   extern char *SLtt_tgetstr (char *);
   extern int SLtt_tgetnum (char *);
   extern int SLtt_tgetflag (char *);
   extern char *SLtt_tigetent (char *);
   extern char *SLtt_tigetstr (char *, char **);
   extern int SLtt_tigetnum (char *, char **);
# endif
#endif
   
extern SLtt_Char_Type SLtt_get_color_object (int);
extern void SLtt_set_color_object (int, SLtt_Char_Type);
extern void SLtt_set_color (int, char *, char *, char *);
extern void SLtt_set_mono (int, char *, SLtt_Char_Type);
extern void SLtt_add_color_attribute (int, SLtt_Char_Type);
extern void SLtt_set_color_fgbg (int, SLtt_Char_Type, SLtt_Char_Type);
     
/*}}}*/

/*{{{ SLang Preprocessor Interface */

typedef struct 
{
   int this_level;
   int exec_level;
   int prev_exec_level;
   char preprocess_char;
   char comment_char;
   unsigned char flags;
#define SLPREP_BLANK_LINES_OK	1
#define SLPREP_COMMENT_LINES_OK	2
}
SLPreprocess_Type;

extern int SLprep_open_prep (SLPreprocess_Type *);
extern void SLprep_close_prep (SLPreprocess_Type *);
extern int SLprep_line_ok (char *, SLPreprocess_Type *);
   extern int SLdefine_for_ifdef (char *);
   /* Adds a string to the SLang #ifdef preparsing defines. SLang already
      defines MSDOS, UNIX, and VMS on the appropriate system. */
extern int (*SLprep_exists_hook) (char *, char);
      
   
/*}}}*/

/*{{{ SLsmg Screen Management Functions */

#include <stdarg.h>
extern void SLsmg_fill_region (int, int, int, int, unsigned char);
#ifndef pc_system
extern void SLsmg_set_char_set (int);
extern int SLsmg_Scroll_Hash_Border;
#endif
extern void SLsmg_suspend_smg (void);
extern void SLsmg_resume_smg (void);
extern void SLsmg_erase_eol (void);
extern void SLsmg_gotorc (int, int);
extern void SLsmg_erase_eos (void);
extern void SLsmg_reverse_video (void);
extern void SLsmg_set_color (int);
extern void SLsmg_normal_video (void);
extern void SLsmg_printf (char *, ...);
extern void SLsmg_vprintf (char *, va_list);
extern void SLsmg_write_string (char *);
extern void SLsmg_write_nstring (char *, int);
extern void SLsmg_write_char (char);
extern void SLsmg_write_nchars (char *, int);
extern void SLsmg_write_wrapped_string (char *, int, int, int, int, int);
extern void SLsmg_cls (void);
extern void SLsmg_refresh (void);
extern void SLsmg_touch_lines (int, int);
extern int SLsmg_init_smg (void);
extern void SLsmg_reset_smg (void);
extern unsigned short SLsmg_char_at(void);
extern void SLsmg_set_screen_start (int *, int *);
extern void SLsmg_draw_hline (int);
extern void SLsmg_draw_vline (int);
extern void SLsmg_draw_object (int, int, unsigned char);
extern void SLsmg_draw_box (int, int, int, int);
extern int SLsmg_get_column(void);
extern int SLsmg_get_row(void);
extern void SLsmg_forward (int);
extern void SLsmg_write_color_chars (unsigned short *, unsigned int);
extern unsigned int SLsmg_read_raw (unsigned short *, unsigned int);
extern unsigned int SLsmg_write_raw (unsigned short *, unsigned int);
   
extern int SLsmg_Display_Eight_Bit;
extern int SLsmg_Tab_Width;
extern int SLsmg_Newline_Moves;
extern int SLsmg_Backspace_Moves;

#ifdef pc_system
# define SLSMG_HLINE_CHAR	0xC4
# define SLSMG_VLINE_CHAR	0xB3
# define SLSMG_ULCORN_CHAR	0xDA
# define SLSMG_URCORN_CHAR	0xBF
# define SLSMG_LLCORN_CHAR	0xC0
# define SLSMG_LRCORN_CHAR	0xD9
# define SLSMG_RTEE_CHAR	0xB4
# define SLSMG_LTEE_CHAR	0xC3
# define SLSMG_UTEE_CHAR	0xC2
# define SLSMG_DTEE_CHAR	0xC1
# define SLSMG_PLUS_CHAR	0xC5
/* There are several to choose from: 0xB0, 0xB1, and 0xB2 */
# define SLSMG_CKBRD_CHAR	0xB0
#else
# define SLSMG_HLINE_CHAR	'q'
# define SLSMG_VLINE_CHAR	'x'
# define SLSMG_ULCORN_CHAR	'l'
# define SLSMG_URCORN_CHAR	'k'
# define SLSMG_LLCORN_CHAR	'm'
# define SLSMG_LRCORN_CHAR	'j'
# define SLSMG_CKBRD_CHAR	'a'
# define SLSMG_RTEE_CHAR	'u'
# define SLSMG_LTEE_CHAR	't'
# define SLSMG_UTEE_CHAR	'w'
# define SLSMG_DTEE_CHAR	'v'
# define SLSMG_PLUS_CHAR	'n'
#endif

#ifndef pc_system
# define SLSMG_COLOR_BLACK		0x000000
# define SLSMG_COLOR_RED		0x000001
# define SLSMG_COLOR_GREEN		0x000002
# define SLSMG_COLOR_BROWN		0x000003
# define SLSMG_COLOR_BLUE		0x000004
# define SLSMG_COLOR_MAGENTA		0x000005
# define SLSMG_COLOR_CYAN		0x000006
# define SLSMG_COLOR_LGRAY		0x000007
# define SLSMG_COLOR_GRAY		0x000008
# define SLSMG_COLOR_BRIGHT_RED		0x000009
# define SLSMG_COLOR_BRIGHT_GREEN	0x00000A
# define SLSMG_COLOR_BRIGHT_BROWN	0x00000B
# define SLSMG_COLOR_BRIGHT_BLUE	0x00000C
# define SLSMG_COLOR_BRIGHT_CYAN	0x00000D
# define SLSMG_COLOR_BRIGHT_MAGENTA	0x00000E
# define SLSMG_COLOR_BRIGHT_WHITE	0x00000F
#endif

/*}}}*/

/*{{{ SLang Keypad Interface */

#define SL_KEY_ERR		0xFFFF

#define SL_KEY_UP		0x101
#define SL_KEY_DOWN		0x102
#define SL_KEY_LEFT		0x103
#define SL_KEY_RIGHT		0x104
#define SL_KEY_PPAGE		0x105
#define SL_KEY_NPAGE		0x106
#define SL_KEY_HOME		0x107
#define SL_KEY_END		0x108
#define SL_KEY_A1		0x109
#define SL_KEY_A3		0x10A
#define SL_KEY_B2		0x10B
#define SL_KEY_C1		0x10C
#define SL_KEY_C3		0x10D
#define SL_KEY_REDO		0x10E
#define SL_KEY_UNDO		0x10F
#define SL_KEY_BACKSPACE	0x110
#define SL_KEY_ENTER		0x111
#define SL_KEY_IC		0x112
#define SL_KEY_DELETE		0x113

#define SL_KEY_F0		0x200
#define SL_KEY_F(X)		(SL_KEY_F0 + X)

/* I do not intend to use keysymps > 0x1000.  Applications can use those. */
/* Returns 0 upon success or -1 upon error. */
int SLkp_define_keysym (char *, unsigned int);

/* This function must be called AFTER SLtt_get_terminfo and not before. */
extern int SLkp_init (void);

/* This function uses SLang_getkey and assumes that what ever initialization 
 * is required for SLang_getkey has been performed.
 */
extern int SLkp_getkey (void);

/*}}}*/

/*{{{ SLang Scroll Interface */

typedef struct _SLscroll_Type
{
   struct _SLscroll_Type *next;
   struct _SLscroll_Type *prev;
   unsigned int flags;
}
SLscroll_Type;

typedef struct 
{
   unsigned int flags;
   SLscroll_Type *top_window_line;   /* list element at top of window */
   SLscroll_Type *bot_window_line;   /* list element at bottom of window */
   SLscroll_Type *current_line;    /* current list element */
   SLscroll_Type *lines;	       /* first list element */
   unsigned int nrows;		       /* number of rows in window */
   unsigned int hidden_mask;	       /* applied to flags in SLscroll_Type */
   unsigned int line_num;	       /* current line number (visible) */
   unsigned int num_lines;	       /* total number of lines (visible) */
   unsigned int window_row;	       /* row of current_line in window */
   unsigned int border;		       /* number of rows that form scroll border */
   int cannot_scroll;		       /* should window scroll or recenter */
}
SLscroll_Window_Type;

extern int SLscroll_find_top (SLscroll_Window_Type *);
extern int SLscroll_find_line_num (SLscroll_Window_Type *);
extern unsigned int SLscroll_next_n (SLscroll_Window_Type *, unsigned int);
extern unsigned int SLscroll_prev_n (SLscroll_Window_Type *, unsigned int);
extern int SLscroll_pageup (SLscroll_Window_Type *);
extern int SLscroll_pagedown (SLscroll_Window_Type *);

/*}}}*/

/*{{{ Signal Routines */

typedef void SLSig_Fun_Type (int);
extern SLSig_Fun_Type *SLsignal (int, SLSig_Fun_Type *);
extern SLSig_Fun_Type *SLsignal_intr (int, SLSig_Fun_Type *);
#ifndef pc_system
extern int SLsig_block_signals (void);
extern int SLsig_unblock_signals (void);
#endif
/*}}}*/

/*{{{ Interpreter Macro Definitions */

/* This value is a main_type just like the other main_types defined
 * near the definition of SLang_Name_Type.  Applications should avoid using
 * this so if you do not understands its role, do not use it.
 */
#define SLANG_DATA		0x30           /* real objects which may be destroyed */

/* Subtypes */

/* The definitions here are for objects that may be on the run-time stack.
 * They are actually sub_types of literal and data main_types.
 */
#define VOID_TYPE	1
#define INT_TYPE 	2
#ifdef FLOAT_TYPE
# undef FLOAT_TYPE
# define FLOAT_TYPE	3
#endif
#define CHAR_TYPE	4
#define INTP_TYPE	5
/* An object of INTP_TYPE should never really occur on the stack.  Rather,
 * the integer to which it refers will be there instead.  It is defined here
 * because it is a valid type for MAKE_VARIABLE.
 */

#define SLANG_OBJ_TYPE	6
/* SLANG_OBJ_TYPE refers to an object on the stack that is a pointer to
 * some other object.
 */

#if 0
/* This is not ready yet.  */
# define SLANG_NOOP	9
#endif
   
/* Everything above string should correspond to a pointer in the object 
 * structure.  See do_binary (slang.c) for exploitation of this fact.
 */
#define STRING_TYPE 	10
/* Array type MUST be the smallest number for SLuser_Object_Type structs */
#define ARRAY_TYPE	20
/* I am reserving values greater than or equal to user applications.  The
 * first 99 are used for S-Lang.
 */


/* Binary and Unary Subtypes */
/* Since the application can define new types and can overload the binary
 * and unary operators, these definitions must be present in this file.
 */
#define SLANG_PLUS	1
#define SLANG_MINUS	2
#define SLANG_TIMES	3
#define SLANG_DIVIDE	4
#define SLANG_EQ		5
#define SLANG_NE		6
#define SLANG_GT		7
#define SLANG_GE		8
#define SLANG_LT		9
#define SLANG_LE		10

/* UNARY subtypes  (may be overloaded) */
#define SLANG_ABS	11
#define SLANG_SIGN	12
#define SLANG_SQR	13
#define SLANG_MUL2	14
#define SLANG_CHS	15

/* error codes, severe errors are less than 0 */
#define SL_INVALID_PARM		-6
#define SL_MALLOC_ERROR		-5
#define INTERNAL_ERROR		-4
#define UNKNOWN_ERROR		-3
#define STACK_OVERFLOW		-1
#define STACK_UNDERFLOW		-2
#define INTRINSIC_ERROR		1
/* Intrinsic error is an error generated by intrinsic functions */
#define USER_BREAK		2
#define UNDEFINED_NAME		3
#define SYNTAX_ERROR		4
#define DUPLICATE_DEFINITION	5
#define TYPE_MISMATCH		6
#define READONLY_ERROR		7
#define DIVIDE_ERROR		8
/* object could not be opened */
#define SL_OBJ_NOPEN		9
/* unknown object */
#define SL_OBJ_UNKNOWN		10

extern char *SLang_Error_Message;

extern void SLadd_name(char *, long, unsigned char, unsigned char);
extern void SLadd_at_handler (long *, char *);

#define SLANG_MAKE_ARGS(out, in) ((unsigned char)(out) | ((unsigned short) (in) << 4))

#ifdef SLANG_STATS

#define MAKE_INTRINSIC(n, f, out, in)        \
    {0, n, (out | (in << 4)), SLANG_INTRINSIC, (long) f}
       
#define MAKE_VARIABLE(n, v, t, r)     \
    {0, n, t, (SLANG_IVARIABLE + r), (long) v}

#else
#define MAKE_INTRINSIC(n, f, out, in)        \
    {n, (out | (in << 4)), SLANG_INTRINSIC, (long) f}
       
#define MAKE_VARIABLE(n, v, t, r)     \
    {n, t, (SLANG_IVARIABLE + r), (long) v}
#endif

#define SLANG_END_TABLE  MAKE_INTRINSIC("", 0, 0, 0)


/*}}}*/

/*{{{ Upper/Lowercase Functions */

extern void SLang_define_case(int *, int *);
extern void SLang_init_case_tables (void);

extern unsigned char Chg_UCase_Lut[256];
extern unsigned char Chg_LCase_Lut[256];
#define UPPER_CASE(x) (Chg_UCase_Lut[(unsigned char) (x)])
#define LOWER_CASE(x) (Chg_LCase_Lut[(unsigned char) (x)])
#define CHANGE_CASE(x) (((x) == Chg_LCase_Lut[(unsigned char) (x)]) ?\
			Chg_UCase_Lut[(unsigned char) (x)] : Chg_LCase_Lut[(unsigned char) (x)])


/*}}}*/

/*{{{ Regular Expression Interface */

typedef struct
{
   unsigned char *pat;		       /* regular expression pattern */
   unsigned char *buf;		       /* buffer for compiled regexp */
   unsigned int buf_len;	       /* length of buffer */
   int case_sensitive;		       /* 1 if match is case sensitive  */
   int must_match;		       /* 1 if line must contain substring */
   int must_match_bol;		       /* true if it must match beginning of line */
   unsigned char must_match_str[16];   /* 15 char null term substring */
   int osearch;			       /* 1 if ordinary search suffices */
   unsigned int min_length;	       /* minimum length the match must be */
   int beg_matches[10];		       /* offset of start of \( */
   unsigned int end_matches[10];       /* length of nth submatch
					* Note that the entire match corresponds
					* to \0 
					*/
   int offset;			       /* offset to be added to beg_matches */
} SLRegexp_Type;

extern unsigned char *SLang_regexp_match(unsigned char *, 
					 unsigned int, 
					 SLRegexp_Type *);
extern int SLang_regexp_compile (SLRegexp_Type *);
extern char *SLregexp_quote_string (char *, char *, unsigned int);


/*}}}*/

/*{{{ SLang Command Interface */

#define SLCMD_MAX_ARGS 10
struct _SLcmd_Cmd_Type; /* Pre-declaration is needed below */
typedef struct
{
   struct _SLcmd_Cmd_Type *table;
   int argc;
   char *string_args[SLCMD_MAX_ARGS];
   int int_args[SLCMD_MAX_ARGS];
   float64 float_args[SLCMD_MAX_ARGS];
   unsigned char arg_type[SLCMD_MAX_ARGS];
} SLcmd_Cmd_Table_Type;


typedef struct _SLcmd_Cmd_Type
{
   int (*cmdfun)(int, SLcmd_Cmd_Table_Type *);
   char cmd[32];
   char arg_type[SLCMD_MAX_ARGS];
} SLcmd_Cmd_Type;

extern int SLcmd_execute_string (char *, SLcmd_Cmd_Table_Type *);

/*}}}*/

/*{{{ SLang Search Interface */

typedef struct
{
   int cs;			       /* case sensitive */
   unsigned char key[256];
   int ind[256];
   int key_len;
   int dir;
} SLsearch_Type;

extern int SLsearch_init (char *, int, int, SLsearch_Type *);
/* This routine must first be called before any search can take place. 
 * The second parameter specifies the direction of the search: greater than 
 * zero for a forwrd search and less than zero for a backward search.  The 
 * third parameter specifies whether the search is case sensitive or not.
 * The last parameter is a pointer to a structure that is filled by this 
 * function and it is this structure that must be passed to SLsearch.
 */

unsigned char *SLsearch (unsigned char *, unsigned char *, SLsearch_Type *);
/* To use this routine, you must first call 'SLsearch_init'.  Then the first 
 * two parameters p1 and p2 serve to define the region over which the search
 * is to take place.  The third parameter is the structure that was previously
 * initialized by SLsearch_init.
 * 
 * The routine returns a pointer to the match if found otherwise it returns 
 * NULL.
 */

/*}}}*/

#if 0
{
#endif
#ifdef __cplusplus
}
#endif

#endif  /* _DAVIS_SLANG_H_ */
