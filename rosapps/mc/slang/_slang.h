/* header file for S-Lang internal structures that users do not (should not)
   need.  Use slang.h for that purpose. */
/* Copyright (c) 1992, 1995 John E. Davis
 * All rights reserved.
 * 
 * You may distribute under the terms of either the GNU General Public
 * License or the Perl Artistic License.
 */


#include "config.h"

#include <string.h>

#include "jdmacros.h"

#ifdef VMS
# define SLANG_SYSTEM_NAME "_VMS"
#else
# if defined (__GO32__) || defined (__EMX__) || \
    defined (msdos) || defined (__os2__)
#  define SLANG_SYSTEM_NAME "_IBMPC"
# else
#  define SLANG_SYSTEM_NAME "_UNIX"
# endif
#endif  /* VMS */

#ifdef msdos
#define SLANG_MAX_SYMBOLS 500
#else
#define SLANG_MAX_SYMBOLS 2500
#endif
/* maximum number of global symbols--- slang builtin, functions, global vars */


/* These quantities are main_types for byte-compiled code.  They are used
 * by the inner_interp routine.  The ones commented out with a // are 
 * actually defined in slang.h because they are also used as the main_type in
 * the name table.
 */

/* // #define SLANG_LVARIABLE	0x01 */
#define SLANG_LOGICAL	0x02
#define SLANG_BINARY	0x03
/* // #define SLANG_INTRINSIC 	0x06 */
/* // #define SLANG_FUNCTION  	0x07 */
#define SLANG_LITERAL	0x08           /* constant objects */
#define SLANG_BLOCK      0x09
#define SLANG_EQS	0x0A
#define SLANG_UNARY	0x0B
#define SLANG_LUNARY	0x0C

/* // #define SLANG_GVARIABLE 	0x0D */
/* // #define SLANG_IVARIABLE 	0x0E */     /* intrinsic variables */
/* // #define SLANG_RVARIABLE	0x0F */     /* read only variable */

/* These 3 MUST be in this order too ! */
#define SLANG_RETURN	0x10
#define SLANG_BREAK	0x11
#define SLANG_CONTINUE	0x12

#define SLANG_EXCH	0x13
#define SLANG_LABEL	0x14
#define SLANG_LOBJPTR	0x15
#define SLANG_GOBJPTR	0x16
#define SLANG_X_ERROR	0x17
/* These must be in this order */
#define SLANG_X_USER0	0x18
#define SLANG_X_USER1	0x19
#define SLANG_X_USER2	0x1A
#define SLANG_X_USER3	0x1B
#define SLANG_X_USER4	0x1C

#ifdef SLANG_NOOP
# define SLANG_NOOP_DIRECTIVE 0x2F
#endif

/* If SLANG_DATA occurs as the main_type for an object in the interpreter's
 * byte-code, it currently refers to a STRING.
 */
/* // #define SLANG_DATA	0x30 */           /* real objects which may be destroyed */


/* Subtypes */
#define ERROR_BLOCK	0x01
/* gets executed if block encounters error other than stack related */
#define EXIT_BLOCK	0x02
#define USER_BLOCK0	0x03
#define USER_BLOCK1	0x04
#define USER_BLOCK2	0x05
#define USER_BLOCK3	0x06
#define USER_BLOCK4	0x07
/* The user blocks MUST be in the above order */

/* directive subtypes */
#define SLANG_LOOP_MASK	0x80
#define SLANG_LOOP	0x81
#define SLANG_WHILE	0x82
#define SLANG_FOR	0x83
#define SLANG_FOREVER	0x84
#define SLANG_CFOR	0x85
#define SLANG_DOWHILE	0x86

#define SLANG_IF_MASK	0x40
#define SLANG_IF		0x41
#define SLANG_IFNOT	0x42
#define SLANG_ELSE	0x43


/* local, global variable assignments
 * The order here is important.  See interp_variable_eqs to see how this
 * is exploited. */
#define SLANG_EQS_MASK	0x20
/* local variables */
/* Keep these in this order!! */
#define SLANG_LEQS	0x21
#define SLANG_LPEQS	0x22
#define SLANG_LMEQS	0x23
#define SLANG_LPP	0x24
#define SLANG_LMM	0x25
/* globals */
/* Keep this on this order!! */
#define SLANG_GEQS	0x26
#define SLANG_GPEQS	0x27
#define SLANG_GMEQS	0x28
#define SLANG_GPP	0x29
#define SLANG_GMM	0x2A
/* intrinsic variables */
#define SLANG_IEQS	0x2B
#define SLANG_IPEQS	0x2C
#define SLANG_IMEQS	0x2D
#define SLANG_IPP	0x2E
#define SLANG_IMM	0x2F


#define SLANG_ELSE_MASK	0x10
#define SLANG_ANDELSE	0x11
#define SLANG_ORELSE	0x12
#define SLANG_SWITCH	0x13

/* LOGICAL SUBTYPES (operate on integers) */
#define SLANG_MOD	16
#define SLANG_OR	17
#define SLANG_AND	18
#define SLANG_BAND	19
#define SLANG_BOR	20
#define SLANG_BXOR	21
#define SLANG_SHL	22
#define SLANG_SHR	23

/* LUnary Subtypes */
#define SLANG_NOT	24
#define SLANG_BNOT	25

typedef struct SLBlock_Type
  {
     unsigned char main_type;
     unsigned char sub_type;
     union 
       {
	  struct SLBlock_Type *blk;
	  int i_blk;
	  SLang_Name_Type *n_blk;
	  char *s_blk;
#ifdef FLOAT_TYPE
	  float64 *f_blk;		       /*literal float is a pointer */
#endif
	  long l_blk;
       }
     b;
  }
SLBlock_Type;


typedef struct
{
   unsigned char main_type;	       /* block, intrinsic... */
   unsigned char sub_type;	       /* SLANG_WHILE, SLANG_DATA, ... */
   union
     {
	long l_val;
	char *s_val;
	int i_val;
	SLuser_Object_Type *uobj;
	SLang_Name_Type *n_val;
#ifdef FLOAT_TYPE
	float64 f_val;
#endif
     } v;
}  SLang_Object_Type;


extern void SLang_free_object (SLang_Object_Type *);

extern int SLang_pop_non_object (SLang_Object_Type *);

extern void _SLdo_error (char *, ...);
extern void SLcompile(char *);
extern void (*SLcompile_ptr)(char *);

typedef struct
{
   char *name;  int type;
} SLang_Name2_Type;

extern void SLstupid_hash(void);

typedef struct SLName_Table
{
   struct SLName_Table *next;	       /* next table */
   SLang_Name_Type *table;	       /* pointer to table */
   int n;			       /* entries in this table */
   char name[32];		       /* name of table */
   int ofs[256];		       /* offsets into table */
} SLName_Table;

extern SLName_Table *SLName_Table_Root;
#ifdef MSWINDOWS
extern SLang_Name_Type *SLang_Name_Table;
#else
extern SLang_Name_Type SLang_Name_Table[SLANG_MAX_SYMBOLS];
#endif

extern SLang_Name2_Type SL_Binary_Ops [];
extern SLang_Object_Type *SLStack_Pointer;
extern char *SLbyte_compile_name(char *);
extern int SLang_pop(SLang_Object_Type *);
extern char *SLexpand_escaped_char(char *, char *);
extern void SLexpand_escaped_string (char *, char *, char *);

extern SLang_Object_Type *_SLreverse_stack (int);
extern SLang_Name_Type *SLang_locate_name(char *);

/* returns a pointer to a MALLOCED string */
extern char *SLstringize_object (SLang_Object_Type *);

/* array types */
typedef struct SLArray_Type
{
   unsigned char type;		       /* int, float, etc... */
   int dim;			       /* # of dims (max 3) */
   int x, y, z;			       /* actual dims */
   union
     {
	unsigned char *c_ptr;
	unsigned char **s_ptr;
	int *i_ptr;
#ifdef FLOAT_TYPE
	float64 *f_ptr;
#endif
	SLuser_Object_Type **u_ptr;
     }
   buf;
   unsigned char flags;		       /* readonly, etc...  If this is non-zero,
					* the buf pointer will NOT be freed.
					* See SLarray_free_array.
					*/
} SLArray_Type;


/* Callback to delete array */
extern void SLarray_free_array (long *);


/* maximum size of run time stack */
#ifdef msdos
#define SLANG_MAX_STACK_LEN 500
#else
#define SLANG_MAX_STACK_LEN 2500
#endif

#ifdef MSWINDOWS
extern SLang_Object_Type *SLRun_Stack;
#else
extern SLang_Object_Type SLRun_Stack[SLANG_MAX_STACK_LEN];
#endif

extern SLang_Object_Type *SLStack_Pointer;

extern int SLang_Trace;
extern int SLstack_depth(void);

extern void SLang_trace_fun(char *);
extern void SLexecute_function(SLang_Name_Type *);
extern char *SLmake_string (char *);

extern int _SLeqs_name(char *, SLang_Name2_Type *);
extern void SLang_push(SLang_Object_Type *);
extern void SLang_push_float(float64);
extern void SLadd_variable(char *);
extern void SLang_clear_error(void);
extern void SLarray_info (void);
extern int SLPreprocess_Only;		        /* preprocess instead of 
						 * bytecompiling
						 */

extern void SLdo_pop (void);
extern unsigned int SLsys_getkey (void);
extern int SLsys_input_pending (int);

#ifdef REAL_UNIX_SYSTEM
extern int SLtt_tigetflag (char *, char **);
extern int SLtt_tigetnum (char *, char **);
extern char *SLtt_tigetstr (char *, char **);
extern char *SLtt_tigetent (char *);
#endif

#ifdef msdos
#define MAX_INPUT_BUFFER_LEN 40
#else
#define MAX_INPUT_BUFFER_LEN 1024
#endif
extern unsigned char SLang_Input_Buffer [MAX_INPUT_BUFFER_LEN];
extern unsigned int SLang_Input_Buffer_Len;

extern int SLregister_types (void);

#ifndef pc_system
extern char *SLtt_Graphics_Char_Pairs;
#endif				       /* NOT pc_system */

extern void _SLerrno_set_return_status (void);
extern char *_SLerrno_strerror (void);
extern int _SLerrno_Return_Status;

